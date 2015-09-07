/*
 * $Id: flex_ctr_common.c,v 1.93 Broadcom SDK $
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
 * File:        flex_ctr_common.c
 * Purpose:     Manage commaon functionality for flex counter implementation
 */

#include <shared/bsl.h>

#include <bcm_int/esw/flex_ctr.h>
#include <bcm_int/esw/stat.h>
#include <bcm_int/esw/virtual.h> 
#include <shared/idxres_afl.h>
#include <soc/scache.h>

#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/trident2.h>
#include <bcm_int/esw/triumph2.h>
#endif


uint8     bcm_stat_flex_packet_attr_type_names_t[][64] = {
          "bcmStatFlexPacketAttrTypeUncompressed",
          "bcmStatFlexPacketAttrTypeCompressed",
          "bcmStatFlexPacketAttrTypeUdf"};
static soc_reg_t _pool_ctr_register[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
                                        [BCM_STAT_FLEX_COUNTER_MAX_POOL]={
                 {ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_0r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_1r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_2r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_3r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_4r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_5r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_6r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_7r,
#ifdef BCM_TRIUMPH3_SUPPORT
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_8r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_9r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_10r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_11r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_12r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_13r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_14r,
                  ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_15r},
#else
                  0,0,0,0,0,0,0,0 /* Kept it for future updates */},
#endif
                 {EGR_FLEX_CTR_COUNTER_UPDATE_CONTROL_0r,
                  EGR_FLEX_CTR_COUNTER_UPDATE_CONTROL_1r,
                  EGR_FLEX_CTR_COUNTER_UPDATE_CONTROL_2r,
                  EGR_FLEX_CTR_COUNTER_UPDATE_CONTROL_3r,
                  EGR_FLEX_CTR_COUNTER_UPDATE_CONTROL_4r,
                  EGR_FLEX_CTR_COUNTER_UPDATE_CONTROL_5r,
                  EGR_FLEX_CTR_COUNTER_UPDATE_CONTROL_6r,
                  EGR_FLEX_CTR_COUNTER_UPDATE_CONTROL_7r,
                  0,0,0,0,0,0,0,0 /* Kept it for future updates */}
                 };
static soc_mem_t _ctr_offset_table[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
                                  [BCM_STAT_FLEX_COUNTER_MAX_POOL]={
                 {ING_FLEX_CTR_OFFSET_TABLE_0m,
                  ING_FLEX_CTR_OFFSET_TABLE_1m,
                  ING_FLEX_CTR_OFFSET_TABLE_2m,
                  ING_FLEX_CTR_OFFSET_TABLE_3m,
                  ING_FLEX_CTR_OFFSET_TABLE_4m,
                  ING_FLEX_CTR_OFFSET_TABLE_5m,
                  ING_FLEX_CTR_OFFSET_TABLE_6m,
                  ING_FLEX_CTR_OFFSET_TABLE_7m,
#ifdef BCM_TRIUMPH3_SUPPORT
                  ING_FLEX_CTR_OFFSET_TABLE_8m,
                  ING_FLEX_CTR_OFFSET_TABLE_9m,
                  ING_FLEX_CTR_OFFSET_TABLE_10m,
                  ING_FLEX_CTR_OFFSET_TABLE_11m,
                  ING_FLEX_CTR_OFFSET_TABLE_12m,
                  ING_FLEX_CTR_OFFSET_TABLE_13m,
                  ING_FLEX_CTR_OFFSET_TABLE_14m,
                  ING_FLEX_CTR_OFFSET_TABLE_15m},
#else
                  0,0,0,0,0,0,0,0 /* Kept it for future updates */},
#endif
                 {EGR_FLEX_CTR_OFFSET_TABLE_0m,
                  EGR_FLEX_CTR_OFFSET_TABLE_1m,
                  EGR_FLEX_CTR_OFFSET_TABLE_2m,
                  EGR_FLEX_CTR_OFFSET_TABLE_3m,
                  EGR_FLEX_CTR_OFFSET_TABLE_4m,
                  EGR_FLEX_CTR_OFFSET_TABLE_5m,
                  EGR_FLEX_CTR_OFFSET_TABLE_6m,
                  EGR_FLEX_CTR_OFFSET_TABLE_7m,
                  0,0,0,0,0,0,0,0 /* Kept it for future updates */}
                 };
static soc_mem_t _ctr_counter_table[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
                                  [BCM_STAT_FLEX_COUNTER_MAX_POOL]={
                 {ING_FLEX_CTR_COUNTER_TABLE_0m,
                  ING_FLEX_CTR_COUNTER_TABLE_1m,
                  ING_FLEX_CTR_COUNTER_TABLE_2m,
                  ING_FLEX_CTR_COUNTER_TABLE_3m,
                  ING_FLEX_CTR_COUNTER_TABLE_4m,
                  ING_FLEX_CTR_COUNTER_TABLE_5m,
                  ING_FLEX_CTR_COUNTER_TABLE_6m,
                  ING_FLEX_CTR_COUNTER_TABLE_7m,
#ifdef BCM_TRIUMPH3_SUPPORT
                  ING_FLEX_CTR_COUNTER_TABLE_8m,
                  ING_FLEX_CTR_COUNTER_TABLE_9m,
                  ING_FLEX_CTR_COUNTER_TABLE_10m,
                  ING_FLEX_CTR_COUNTER_TABLE_11m,
                  ING_FLEX_CTR_COUNTER_TABLE_12m,
                  ING_FLEX_CTR_COUNTER_TABLE_13m,
                  ING_FLEX_CTR_COUNTER_TABLE_14m,
                  ING_FLEX_CTR_COUNTER_TABLE_15m},
#else
                  0,0,0,0,0,0,0,0 /* Kept it for future updates */},
#endif
                 {EGR_FLEX_CTR_COUNTER_TABLE_0m,
                  EGR_FLEX_CTR_COUNTER_TABLE_1m,
                  EGR_FLEX_CTR_COUNTER_TABLE_2m,
                  EGR_FLEX_CTR_COUNTER_TABLE_3m,
                  EGR_FLEX_CTR_COUNTER_TABLE_4m,
                  EGR_FLEX_CTR_COUNTER_TABLE_5m,
                  EGR_FLEX_CTR_COUNTER_TABLE_6m,
                  EGR_FLEX_CTR_COUNTER_TABLE_7m,
                  0,0,0,0,0,0,0,0 /* Kept it for future updates */}
                 };

#if defined(BCM_TRIDENT2_SUPPORT)
static soc_mem_t _ctr_counter_table_x[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
                                  [BCM_STAT_FLEX_COUNTER_MAX_POOL]={
                 {ING_FLEX_CTR_COUNTER_TABLE_0_Xm,
                  ING_FLEX_CTR_COUNTER_TABLE_1_Xm,
                  ING_FLEX_CTR_COUNTER_TABLE_2_Xm,
                  ING_FLEX_CTR_COUNTER_TABLE_3_Xm,
                  ING_FLEX_CTR_COUNTER_TABLE_4_Xm,
                  ING_FLEX_CTR_COUNTER_TABLE_5_Xm,
                  ING_FLEX_CTR_COUNTER_TABLE_6_Xm,
                  ING_FLEX_CTR_COUNTER_TABLE_7_Xm},
                 {EGR_FLEX_CTR_COUNTER_TABLE_0_Xm,
                  EGR_FLEX_CTR_COUNTER_TABLE_1_Xm,
                  EGR_FLEX_CTR_COUNTER_TABLE_2_Xm,
                  EGR_FLEX_CTR_COUNTER_TABLE_3_Xm}
                 };

static soc_mem_t _ctr_counter_table_y[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
                                  [BCM_STAT_FLEX_COUNTER_MAX_POOL]={
                 {ING_FLEX_CTR_COUNTER_TABLE_0_Ym,
                  ING_FLEX_CTR_COUNTER_TABLE_1_Ym,
                  ING_FLEX_CTR_COUNTER_TABLE_2_Ym,
                  ING_FLEX_CTR_COUNTER_TABLE_3_Ym,
                  ING_FLEX_CTR_COUNTER_TABLE_4_Ym,
                  ING_FLEX_CTR_COUNTER_TABLE_5_Ym,
                  ING_FLEX_CTR_COUNTER_TABLE_6_Ym,
                  ING_FLEX_CTR_COUNTER_TABLE_7_Ym},
                 {EGR_FLEX_CTR_COUNTER_TABLE_0_Ym,
                  EGR_FLEX_CTR_COUNTER_TABLE_1_Ym,
                  EGR_FLEX_CTR_COUNTER_TABLE_2_Ym,
                  EGR_FLEX_CTR_COUNTER_TABLE_3_Ym}
                 };
#endif  /* BCM_TRIDENT2_SUPPORT */

static soc_reg_t _pkt_selector_key_reg[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
                                      [8]= {
                 {ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_0r,
                  ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_1r,
                  ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_2r,
                  ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_3r,
                  ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_4r,
                  ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_5r,
                  ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_6r,
                  ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_7r},
                 {EGR_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_0r,
                  EGR_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_1r,
                  EGR_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_2r,
                  EGR_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_3r,
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
                  EGR_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_4r,
                  EGR_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_5r,
                  EGR_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_6r,
                  EGR_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_7r}
#else 
                  0,0,0,0}
#endif
                };

static soc_field_t _pkt_selector_x_en_field_name[8]= {
                   SELECTOR_0_ENf,
                   SELECTOR_1_ENf,
                   SELECTOR_2_ENf,
                   SELECTOR_3_ENf,
                   SELECTOR_4_ENf,
                   SELECTOR_5_ENf,
                   SELECTOR_6_ENf,
                   SELECTOR_7_ENf,
                   };
static soc_field_t _pkt_selector_for_bit_x_field_name[8] = {
                   SELECTOR_FOR_BIT_0f,
                   SELECTOR_FOR_BIT_1f,
                   SELECTOR_FOR_BIT_2f,
                   SELECTOR_FOR_BIT_3f,
                   SELECTOR_FOR_BIT_4f,
                   SELECTOR_FOR_BIT_5f,
                   SELECTOR_FOR_BIT_6f,
                   SELECTOR_FOR_BIT_7f,
                   };

static bcm_stat_flex_ingress_mode_t *flex_ingress_modes[SOC_MAX_NUM_DEVICES]={NULL};
static bcm_stat_flex_egress_mode_t  *flex_egress_modes[SOC_MAX_NUM_DEVICES]={NULL};


static shr_aidxres_list_handle_t flex_aidxres_list_handle
                                 [SOC_MAX_NUM_DEVICES]
                                 [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
                                 [BCM_STAT_FLEX_COUNTER_MAX_POOL];
static bcm_stat_flex_pool_stat_t flex_pool_stat
                                 [SOC_MAX_NUM_DEVICES]
                                 [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
                                 [BCM_STAT_FLEX_COUNTER_MAX_POOL];
static uint16 *flex_base_index_reference_count
                                 [SOC_MAX_NUM_DEVICES]
                                 [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
                                 [BCM_STAT_FLEX_COUNTER_MAX_POOL];

static sal_mutex_t flex_stat_mutex[SOC_MAX_NUM_DEVICES] = {NULL};

/* Both ing_flex_ctr_counter_table_0_entry_t and egr_flex_ctr_table_0_entry_t
   have same contents so using one entry only */
static ing_flex_ctr_counter_table_0_entry_t
       *flex_temp_counter[SOC_MAX_NUM_DEVICES]
                         [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]={{NULL}};
#if defined(BCM_TRIDENT2_SUPPORT)
static ing_flex_ctr_counter_table_0_entry_t
       *flex_temp_counter_dual[SOC_MAX_NUM_DEVICES]
                              [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]={{NULL}};
static uint32 *flex_pkt_counter_x
               [BCM_MAX_NUM_UNITS]
               [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
               [BCM_STAT_FLEX_COUNTER_MAX_POOL]
               [_BCM_FLEX_STAT_BUFFS] = {{{{NULL}}}};
static uint64 *flex_byte_counter_x
               [SOC_MAX_NUM_DEVICES]
               [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
               [BCM_STAT_FLEX_COUNTER_MAX_POOL]
               [_BCM_FLEX_STAT_BUFFS] = {{{{NULL}}}};

static uint32 *flex_pkt_counter_y
               [BCM_MAX_NUM_UNITS]
               [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
               [BCM_STAT_FLEX_COUNTER_MAX_POOL]
               [_BCM_FLEX_STAT_BUFFS] = {{{{NULL}}}};
static uint64 *flex_byte_counter_y
               [SOC_MAX_NUM_DEVICES]
               [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
               [BCM_STAT_FLEX_COUNTER_MAX_POOL]
               [_BCM_FLEX_STAT_BUFFS] = {{{{NULL}}}};
#endif

static uint64 *flex_byte_counter
               [SOC_MAX_NUM_DEVICES]
               [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
               [BCM_STAT_FLEX_COUNTER_MAX_POOL]={{{NULL}}};
static uint32 *flex_packet_counter
               [SOC_MAX_NUM_DEVICES]
               [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
               [BCM_STAT_FLEX_COUNTER_MAX_POOL]={{{NULL}}};
static uint64 *flex_packet64_counter
               [SOC_MAX_NUM_DEVICES]
               [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
               [BCM_STAT_FLEX_COUNTER_MAX_POOL]={{{NULL}}};

#define BCM_STAT_FLEX_COUNTER_LOCK(unit) \
        sal_mutex_take(flex_stat_mutex[unit], sal_mutex_FOREVER);
#define BCM_STAT_FLEX_COUNTER_UNLOCK(unit) \
        sal_mutex_give(flex_stat_mutex[unit]);

static uint8 flex_directions[][8]={
             "Ingress",
             "Egress" };
#ifdef BCM_WARM_BOOT_SUPPORT
static uint8 _flex_group_mode_total_counters_info
              [BCM_STAT_FLEX_COUNTER_MAX_DIRECTION]
              [bcmStatGroupModeCng+1]={
               /* #####################################      */
               /* INGRESS GROUP MODE ==> Total Counters      */
               /* #####################################      */
               {/* bcmStatGroupModeSingle */                 1,
                /* bcmStatGroupModeTrafficType */            3,
                /* bcmStatGroupModeDlfAll */                 2,
                /* bcmStatGroupModeDlfIntPri */              17,
                /* bcmStatGroupModeTyped */                  4,
                /* bcmStatGroupModeTypedAll */               5,
                /* bcmStatGroupModeTypedIntPri */            20,
                /* bcmStatGroupModeSingleWithControl */      2,
                /* bcmStatGroupModeTrafficTypeWithControl */ 4,
                /* bcmStatGroupModeDlfAllWithControl */      3,
                /* bcmStatGroupModeDlfIntPriWithControl */   18,
                /* bcmStatGroupModeTypedWithControl */       5,
                /* bcmStatGroupModeTypedAllWithControl */    6,
                /* bcmStatGroupModeTypedIntPriWithControl */ 21,
                /* bcmStatGroupModeDot1P */                  8,
                /* bcmStatGroupModeIntPri */                 16,
                /* bcmStatGroupModeIntPriCng */              64,
                /* bcmStatGroupModeSvpType */                2,
                /* bcmStatGroupModeDscp */                   64,
                /* bcmStatGroupModeDvpType */                0,
                /* bcmStatGroupModeCng */                    4},
               /* #####################################      */
               /* EGRESS GROUP MODE ==> Total Counters       */
               /* #####################################      */
               {/* bcmStatGroupModeSingle */                 1,
                /* bcmStatGroupModeTrafficType */            2,
                /* bcmStatGroupModeDlfAll */                 0,
                /* bcmStatGroupModeDlfIntPri */              0,
                /* bcmStatGroupModeTyped */                  0,
                /* bcmStatGroupModeTypedAll */               0,
                /* bcmStatGroupModeTypedIntPri */            18,
                /* bcmStatGroupModeSingleWithControl */      0,
                /* bcmStatGroupModeTrafficTypeWithControl */ 0,
                /* bcmStatGroupModeDlfAllWithControl */      0,
                /* bcmStatGroupModeDlfIntPriWithControl */   0,
                /* bcmStatGroupModeTypedWithControl */       0,
                /* bcmStatGroupModeTypedAllWithControl */    0,
                /* bcmStatGroupModeTypedIntPriWithControl */ 0,
                /* bcmStatGroupModeDot1P */                  8,
                /* bcmStatGroupModeIntPri */                 16,
                /* bcmStatGroupModeIntPriCng */              64,
                /* bcmStatGroupModeSvpType */                2,
                /* bcmStatGroupModeDscp */                   64,
                /* bcmStatGroupModeDvpType */                2,
                /* bcmStatGroupModeCng */                    4},
              };
#endif

typedef struct uncmprsd_attr_bits_selector_s {
    uint32 attr_bits;
    uint8  attr_name[20];
}uncmprsd_attr_bits_selector_t;

static uncmprsd_attr_bits_selector_t ing_uncmprsd_attr_bits_selector[]={
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_CNG_ATTR_BITS,           "CNG"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_IFP_CNG_ATTR_BITS,       "IFP CNG"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS,       "INT PRI"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_VLAN_FORMAT_ATTR_BITS,   "VLAN"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS,   "OUTER.1P"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS,   "INNER.1P"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INGRESS_PORT_ATTR_BITS,  "PORT"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS,           "TOS_DSCP"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_TOS_ECN_ATTR_BITS,           "TOS_ECN"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,"PktRes"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS,      "SVP"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_DROP_ATTR_BITS,          "DROP"},
       {BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS,        "IP"}
};
static uncmprsd_attr_bits_selector_t egr_uncmprsd_attr_bits_selector[]={
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_CNG_ATTR_BITS,           "CNG"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS,       "INT PRI"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_VLAN_FORMAT_ATTR_BITS,   "VLAN"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS,   "OUTER.1P"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS,   "INNER.1P"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_EGRESS_PORT_ATTR_BITS,   "PORT"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS,           "TOS_DSCP"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_TOS_ECN_ATTR_BITS,           "TOS_ECN"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,"PktRes"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS,      "SVP"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_DVP_TYPE_ATTR_BITS,      "DVP"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_DROP_ATTR_BITS,          "DROP"},
       {BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS,        "IP"}
};

#define _DEFINE_GET_INGRESS_VALUE(field_name,field_value) \
static  uint8 _bcm_esw_get_ing_##field_name##_value( \
                bcm_stat_flex_ing_pkt_attr_bits_t *ing_pkt_attr_bits) { \
                   return field_value; \
} \

#define DEFINE_GET_INGRESS_VALUE(field_name) \
        _DEFINE_GET_INGRESS_VALUE(field_name,ing_pkt_attr_bits->field_name)

DEFINE_GET_INGRESS_VALUE(cng)
DEFINE_GET_INGRESS_VALUE(ifp_cng)
DEFINE_GET_INGRESS_VALUE(int_pri)
DEFINE_GET_INGRESS_VALUE(vlan_format)
DEFINE_GET_INGRESS_VALUE(outer_dot1p)
DEFINE_GET_INGRESS_VALUE(inner_dot1p)
DEFINE_GET_INGRESS_VALUE(ing_port)
DEFINE_GET_INGRESS_VALUE(tos_dscp)
DEFINE_GET_INGRESS_VALUE(tos_ecn)
DEFINE_GET_INGRESS_VALUE(pkt_resolution)
DEFINE_GET_INGRESS_VALUE(svp_type)
DEFINE_GET_INGRESS_VALUE(drop)
DEFINE_GET_INGRESS_VALUE(ip_pkt)

typedef struct _bcm_esw_get_ing_func_f {
    uint8 (*func)(bcm_stat_flex_ing_pkt_attr_bits_t *ing_pkt_attr_bits);
    uint8 func_desc[20];
}_bcm_esw_get_ing_func_t;
static _bcm_esw_get_ing_func_t _bcm_esw_get_ing_func[]={
    {_bcm_esw_get_ing_cng_value,"cng"},
    {_bcm_esw_get_ing_ifp_cng_value,"ifp_cng"},
    {_bcm_esw_get_ing_int_pri_value,"int_pri"},
    {_bcm_esw_get_ing_vlan_format_value,"vlan_format"},
    {_bcm_esw_get_ing_outer_dot1p_value,"outer_dot1p"},
    {_bcm_esw_get_ing_inner_dot1p_value,"inner_dot1p"},
    {_bcm_esw_get_ing_ing_port_value,"ing_port"},
    {_bcm_esw_get_ing_tos_dscp_value,"tos_dscp"},
    {_bcm_esw_get_ing_tos_ecn_value,"tos_ecn"},
    {_bcm_esw_get_ing_pkt_resolution_value,"pkt_resolutino"},
    {_bcm_esw_get_ing_svp_type_value,"svp"},
    {_bcm_esw_get_ing_drop_value,"drop"},
    {_bcm_esw_get_ing_ip_pkt_value,"ip_pkt"}
};

#define _DEFINE_GET_EGRESS_VALUE(field_name,field_value) \
static  uint8 _bcm_esw_get_egr_##field_name##_value( \
                bcm_stat_flex_egr_pkt_attr_bits_t *egr_pkt_attr_bits) { \
                   return field_value; \
} \

#define DEFINE_GET_EGRESS_VALUE(field_name) \
        _DEFINE_GET_EGRESS_VALUE(field_name,egr_pkt_attr_bits->field_name)

DEFINE_GET_EGRESS_VALUE(cng)
DEFINE_GET_EGRESS_VALUE(int_pri)
DEFINE_GET_EGRESS_VALUE(vlan_format)
DEFINE_GET_EGRESS_VALUE(outer_dot1p)
DEFINE_GET_EGRESS_VALUE(inner_dot1p)
DEFINE_GET_EGRESS_VALUE(egr_port)
DEFINE_GET_EGRESS_VALUE(tos_dscp)
DEFINE_GET_EGRESS_VALUE(tos_ecn)
DEFINE_GET_EGRESS_VALUE(pkt_resolution)
DEFINE_GET_EGRESS_VALUE(svp_type)
DEFINE_GET_EGRESS_VALUE(dvp_type)
DEFINE_GET_EGRESS_VALUE(drop)
DEFINE_GET_EGRESS_VALUE(ip_pkt)

typedef struct _bcm_esw_get_egr_func_f {
    uint8 (*func)(bcm_stat_flex_egr_pkt_attr_bits_t *egr_pkt_attr_bits);
    uint8 func_desc[20];
}_bcm_esw_get_egr_func_t;
static _bcm_esw_get_egr_func_t _bcm_esw_get_egr_func[]={
    {_bcm_esw_get_egr_cng_value,"cng"},
    {_bcm_esw_get_egr_int_pri_value,"int_pri"},
    {_bcm_esw_get_egr_vlan_format_value,"vlan_format"},
    {_bcm_esw_get_egr_outer_dot1p_value,"outer_dot1p"},
    {_bcm_esw_get_egr_inner_dot1p_value,"inner_dot1p"},
    {_bcm_esw_get_egr_egr_port_value,"egr_port"},
    {_bcm_esw_get_egr_tos_dscp_value,"tos_dscp"},
    {_bcm_esw_get_egr_tos_ecn_value,"tos_ecn"},
    {_bcm_esw_get_egr_pkt_resolution_value,"pkt_resolutino"},
    {_bcm_esw_get_egr_svp_type_value,"svp"},
    {_bcm_esw_get_egr_dvp_type_value,"dvp"},
    {_bcm_esw_get_egr_drop_value,"drop"},
    {_bcm_esw_get_egr_ip_pkt_value,"ip_pkt"}
};

typedef struct ing_flex_object_default_f {
    char obj_desc[20];
    soc_mem_t obj_mem;
    uint32 pool_number;
    uint32 pool_criteria;
}ing_flex_object_default_t;


static ing_flex_object_default_t ing_flex_object_default[]={
    {
        "port", 
        PORT_TABm, 
        FLEX_COUNTER_DEFAULT_PORT_TABLE_POOL_NUMBER, 
        FLEX_COUNTER_POOL_USED_BY_PORT_TABLE
    },
    {
        "vlan", 
        VLAN_TABm, 
        FLEX_COUNTER_DEFAULT_VLAN_TABLE_POOL_NUMBER, 
        FLEX_COUNTER_POOL_USED_BY_VLAN_TABLE
    },
    {
        "vlanxlate", 
        VLAN_XLATEm, 
        FLEX_COUNTER_DEFAULT_VLAN_XLATE_TABLE_POOL_NUMBER,
        FLEX_COUNTER_POOL_USED_BY_VLAN_XLATE_TABLE
    },        
    {
        "vfi",
        VFIm, 
        FLEX_COUNTER_DEFAULT_VFI_TABLE_POOL_NUMBER,
        FLEX_COUNTER_POOL_USED_BY_VFI_TABLE
    },
    {
        "l3intf", 
        L3_IIFm, 
        FLEX_COUNTER_DEFAULT_L3_IIF_TABLE_POOL_NUMBER,
        FLEX_COUNTER_POOL_USED_BY_L3_IIF_TABLE
    },
    {
        "vrf", 
        VRFm, 
        FLEX_COUNTER_DEFAULT_VRF_TABLE_POOL_NUMBER, 
        FLEX_COUNTER_POOL_USED_BY_VRF_TABLE
    },
    {
        "policy", 
        VFP_POLICY_TABLEm, 
        FLEX_COUNTER_DEFAULT_VFP_POLICY_TABLE_POOL_NUMBER,
        FLEX_COUNTER_POOL_USED_BY_VFP_POLICY_TABLE
    },
    {
        "mplsvclabel", 
        SOURCE_VPm, 
        FLEX_COUNTER_DEFAULT_SOURCE_VP_TABLE_POOL_NUMBER,
        FLEX_COUNTER_POOL_USED_BY_SOURCE_VP_TABLE
    },
    {
        "mplsswitchlabel", 
        MPLS_ENTRYm, 
        FLEX_COUNTER_DEFAULT_MPLS_ENTRY_TABLE_POOL_NUMBER, 
        FLEX_COUNTER_POOL_USED_BY_MPLS_ENTRY_TABLE
    },
    {
        "mplsfrrlabel", 
        L3_TUNNELm, 
        FLEX_COUNTER_DEFAULT_VLAN_TABLE_POOL_NUMBER,
        FLEX_COUNTER_POOL_USED_BY_L3_TUNNEL_TABLE
    },
    {
        "l3host", 
        EXT_IPV4_UCAST_WIDEm, 
        FLEX_COUNTER_DEFAULT_L3_ENTRY_TABLE_POOL_NUMBER, 
        FLEX_COUNTER_POOL_USED_BY_L3_ENTRY_TABLE
    }, 
#if defined(BCM_TRIUMPH3_SUPPORT)    
    {
        "trill", 
        MPLS_ENTRY_EXTDm, 
        FLEX_COUNTER_DEFAULT_MPLS_ENTRY_TABLE_POOL_NUMBER,
        FLEX_COUNTER_POOL_USED_BY_MPLS_ENTRY_TABLE
    },
    {
        "mimlookupid", 
        MPLS_ENTRY_EXTDm, 
        FLEX_COUNTER_DEFAULT_MPLS_ENTRY_TABLE_POOL_NUMBER, 
        FLEX_COUNTER_POOL_USED_BY_MPLS_ENTRY_TABLE
    },
#endif
    {
        "l2gre", 
        VLAN_XLATEm, 
        FLEX_COUNTER_DEFAULT_VLAN_XLATE_TABLE_POOL_NUMBER, 
        FLEX_COUNTER_POOL_USED_BY_VLAN_XLATE_TABLE
    },
    {
        "extpolicy", 
        VFP_POLICY_TABLEm, 
        FLEX_COUNTER_DEFAULT_VFP_POLICY_TABLE_POOL_NUMBER, 
        FLEX_COUNTER_POOL_USED_BY_VFP_POLICY_TABLE
    },
    {
        "vxlan", 
        SOURCE_VPm, 
        FLEX_COUNTER_DEFAULT_SOURCE_VP_TABLE_POOL_NUMBER,
        FLEX_COUNTER_POOL_USED_BY_SOURCE_VP_TABLE
    },
#if defined(BCM_TRIDENT2_SUPPORT)    
    {
        "vsan", 
        ING_VSANm, 
        FLEX_COUNTER_DEFAULT_FCOE_TABLE_POOL_NUMBER,
        FLEX_COUNTER_POOL_USED_BY_VSAN_TABLE
    },
    {
        "fcoe", 
        L3_ENTRY_IPV4_MULTICASTm, 
        FLEX_COUNTER_DEFAULT_FCOE_TABLE_POOL_NUMBER, 
        FLEX_COUNTER_POOL_USED_BY_FCOE_TABLE
    },
    {
        "l3route", 
        L3_DEFIPm, 
        FLEX_COUNTER_DEFAULT_L3_ROUTE_TABLE_POOL_NUMBER,
        FLEX_COUNTER_POOL_USED_BY_L3_ROUTE_TABLE
    },
#endif    
    {
        "niv", 
        SOURCE_VPm, 
        FLEX_COUNTER_DEFAULT_SOURCE_VP_TABLE_POOL_NUMBER, 
        FLEX_COUNTER_POOL_USED_BY_SOURCE_VP_TABLE
    },
#if defined(BCM_TRIDENT2_SUPPORT)    
    {
        "ipmc", 
        L3_ENTRY_IPV4_MULTICASTm, 
        FLEX_COUNTER_DEFAULT_FCOE_TABLE_POOL_NUMBER, 
        FLEX_COUNTER_POOL_USED_BY_FCOE_TABLE
    }
#endif
}; 

typedef struct ing_flex_object_config_f {
    soc_mem_t obj_mem;
    uint32 config_pool;
    uint32 share_criteria;
    uint32 exclude_criteria;
}ing_flex_object_config_t;

#define INVALID_CONFIG 0

static ing_flex_object_config_t ing_flex_object_config[]={
    {PORT_TABm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},
    {VFP_POLICY_TABLEm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},
    {VLAN_TABm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},
    {VFIm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},
    {VRFm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},
    {MPLS_ENTRYm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},
    {VLAN_XLATEm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},
    {L3_IIFm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},
    {L3_TUNNELm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},
    {L3_ENTRY_2m, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},
    {L3_DEFIPm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},               
    {L3_ENTRY_IPV4_MULTICASTm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},               
    {ING_VSANm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},        
    {EXT_FP_POLICYm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG},
    {SOURCE_VPm, INVALID_CONFIG, INVALID_CONFIG, INVALID_CONFIG}
};

static uint8 flex_objects[][64]={
             "bcmStatObjectIngPort",
             "bcmStatObjectIngVlan",
             "bcmStatObjectIngVlanXlate",
             "bcmStatObjectIngVfi",
             "bcmStatObjectIngL3Intf",
             "bcmStatObjectIngVrf",
             "bcmStatObjectIngPolicy",
             "bcmStatObjectIngMplsVcLabel",
             "bcmStatObjectIngMplsSwitchLabel",
             "bcmStatObjectEgrPort",
             "bcmStatObjectEgrVlan",
             "bcmStatObjectEgrVlanXlate",
             "bcmStatObjectEgrVfi",
             "bcmStatObjectEgrL3Intf",
             "bcmStatObjectIngMplsFrrLabel",
             "bcmStatObjectIngL3Host",
             "bcmStatObjectIngTrill",
             "bcmStatObjectIngMimLookupId",
             "bcmStatObjectIngL2Gre",
             "bcmStatObjectIngEXTPolicy",
             "bcmStatObjectIngVxlan",
             "bcmStatObjectIngVsan",
             "bcmStatObjectIngFcoe",
             "bcmStatObjectIngL3Route",
             "bcmStatObjectEgrWlan",
             "bcmStatObjectEgrMim",
             "bcmStatObjectEgrMimLookupId",
             "bcmStatObjectEgrL2Gre",
             "bcmStatObjectEgrVxlan",
             "bcmStatObjectEgrL3Nat",
             "bcmStatObjectIngNiv",
             "bcmStatObjectEgrNiv",
             "bcmStatObjectIngIpmc" };             

static uint8 flex_group_modes[][48]={
             "bcmStatGroupModeSingle = 0",
             "bcmStatGroupModeTrafficType = 1",
             "bcmStatGroupModeDlfAll = 2",
             "bcmStatGroupModeDlfIntPri = 3",
             "bcmStatGroupModeTyped = 4",
             "bcmStatGroupModeTypedAll = 5",
             "bcmStatGroupModeTypedIntPri = 6",
             "bcmStatGroupModeSingleWithControl = 7",
             "bcmStatGroupModeTrafficTypeWithControl = 8",
             "bcmStatGroupModeDlfAllWithControl = 9",
             "bcmStatGroupModeDlfIntPriWithControl = 10",
             "bcmStatGroupModeTypedWithControl = 11",
             "bcmStatGroupModeTypedAllWithControl = 12",
             "bcmStatGroupModeTypedIntPriWithControl = 13",
             "bcmStatGroupModeDot1P = 14",
             "bcmStatGroupModeIntPri = 15",
             "bcmStatGroupModeIntPriCng = 16",
             "bcmStatGroupModeSvpType = 17",
             "bcmStatGroupModeDscp = 18",
             "bcmStatGroupModeDvpType = 19",
             "bcmStatGroupModeCng = 20" , 
             "bcmStatGroupModeFlex1 = 21" , 
             "bcmStatGroupModeFlex2 = 22" , 
             "bcmStatGroupModeFlex3 = 23" , 
             "bcmStatGroupModeFlex4 = 24" };

static uint32 flex_used_by_table[]={
              FLEX_COUNTER_POOL_USED_BY_PORT_TABLE,
              FLEX_COUNTER_POOL_USED_BY_VLAN_TABLE,
              FLEX_COUNTER_POOL_USED_BY_VLAN_XLATE_TABLE,
              FLEX_COUNTER_POOL_USED_BY_VFI_TABLE,
              FLEX_COUNTER_POOL_USED_BY_L3_IIF_TABLE,
              FLEX_COUNTER_POOL_USED_BY_VRF_TABLE,
              FLEX_COUNTER_POOL_USED_BY_VFP_POLICY_TABLE,
              FLEX_COUNTER_POOL_USED_BY_SOURCE_VP_TABLE,
              FLEX_COUNTER_POOL_USED_BY_MPLS_ENTRY_TABLE,
              FLEX_COUNTER_POOL_USED_BY_EGR_PORT_TABLE,
              FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_TABLE,
              FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_XLATE_TABLE,
              FLEX_COUNTER_POOL_USED_BY_EGR_VFI_TABLE,
              FLEX_COUNTER_POOL_USED_BY_EGR_L3_NEXT_HOP_TABLE };

static uint32 *local_scache_ptr[SOC_MAX_NUM_DEVICES]={NULL};
static uint32 local_scache_size=0;

#ifdef BCM_WARM_BOOT_SUPPORT
#define BCM_WB_VERSION_1_3                SOC_SCACHE_VERSION(1,3)
static soc_scache_handle_t handle=0;
static uint32              flex_scache_allocated_size=0;
static uint32              *flex_scache_ptr[SOC_MAX_NUM_DEVICES]={NULL};
#endif

typedef struct flex_counter_fields {
               uint32    offset_mode;
               uint32    pool_number;
               uint32    base_idx;
}flex_counter_fields_t;

static flex_counter_fields_t mpls_entry_flex_counter_fields[]={
                             /* MPLS View=0 */
                             {MPLS__FLEX_CTR_OFFSET_MODEf,
                              MPLS__FLEX_CTR_POOL_NUMBERf,
                              MPLS__FLEX_CTR_BASE_COUNTER_IDXf},
                             /* MIM_NVP View=1 */
                             {MIM_NVP__FLEX_CTR_OFFSET_MODEf,
                              MIM_NVP__FLEX_CTR_POOL_NUMBERf,
                              MIM_NVP__FLEX_CTR_BASE_COUNTER_IDXf},
                             /* MIM_ISID   View=2 */
                             {MIM_ISID__FLEX_CTR_OFFSET_MODEf,
                              MIM_ISID__FLEX_CTR_POOL_NUMBERf,
                              MIM_ISID__FLEX_CTR_BASE_COUNTER_IDXf},
#ifdef BCM_TRIUMPH3_SUPPORT
                             {TRILL__FLEX_CTR_OFFSET_MODEf,
                              TRILL__FLEX_CTR_POOL_NUMBERf,
                              TRILL__FLEX_CTR_BASE_COUNTER_IDXf},
                             {IPV4UC__FLEX_CTR_OFFSET_MODEf,
                              IPV4UC__FLEX_CTR_POOL_NUMBERf,
                              IPV4UC__FLEX_CTR_BASE_COUNTER_IDXf},
                             {IPV6UC__FLEX_CTR_OFFSET_MODEf,
                              IPV6UC__FLEX_CTR_POOL_NUMBERf,
                              IPV6UC__FLEX_CTR_BASE_COUNTER_IDXf} };
#else
                             {INVALIDf,
                              INVALIDf,
                              INVALIDf},
                             {INVALIDf,
                              INVALIDf,
                              INVALIDf},
                             {INVALIDf,
                              INVALIDf,
                              INVALIDf} };
#endif


static flex_counter_fields_t egr_l3_next_hop_flex_counter_fields[]={
                             /* L3 View=0 */
                             {FLEX_CTR_OFFSET_MODEf,
                              FLEX_CTR_POOL_NUMBERf,
                              FLEX_CTR_BASE_COUNTER_IDXf},
                             {L3__FLEX_CTR_OFFSET_MODEf,
                              L3__FLEX_CTR_POOL_NUMBERf,
                              L3__FLEX_CTR_BASE_COUNTER_IDXf},
#ifdef BCM_TRIUMPH3_SUPPORT
                             {WLAN__FLEX_CTR_OFFSET_MODEf,
                              WLAN__FLEX_CTR_POOL_NUMBERf,
                              WLAN__FLEX_CTR_BASE_COUNTER_IDXf},
#else
                             {INVALIDf,
                              INVALIDf,
                              INVALIDf},
#endif
                             {PROXY__FLEX_CTR_OFFSET_MODEf,
                              PROXY__FLEX_CTR_POOL_NUMBERf,
                              PROXY__FLEX_CTR_BASE_COUNTER_IDXf},
                             /* MPLS View=1 */
                             {MPLS__FLEX_CTR_OFFSET_MODEf,
                              MPLS__FLEX_CTR_POOL_NUMBERf,
                              MPLS__FLEX_CTR_BASE_COUNTER_IDXf},
                             /* SD_TAG View=2 */
                             {SD_TAG__FLEX_CTR_OFFSET_MODEf,
                              SD_TAG__FLEX_CTR_POOL_NUMBERf,
                              SD_TAG__FLEX_CTR_BASE_COUNTER_IDXf},
                             /* MIM View=3 */
                             {MIM__FLEX_CTR_OFFSET_MODEf,
                              MIM__FLEX_CTR_POOL_NUMBERf,
                              MIM__FLEX_CTR_BASE_COUNTER_IDXf} };

static flex_counter_fields_t egr_vlan_xlate_flex_counter_fields[]={
                             /* Default VLAN_XLATE,VLAN_XLATE_DVP View=0 */
                             {XLATE__FLEX_CTR_OFFSET_MODEf,
                              XLATE__FLEX_CTR_POOL_NUMBERf,
                              XLATE__FLEX_CTR_BASE_COUNTER_IDXf},
                             /* ISID_DVP_XLATE View=4 */
                             {MIM_ISID__FLEX_CTR_OFFSET_MODEf,
                              MIM_ISID__FLEX_CTR_POOL_NUMBERf,
                              MIM_ISID__FLEX_CTR_BASE_COUNTER_IDXf} };

static flex_counter_fields_t vlan_xlate_flex_counter_fields[]={
                             /* XLATE type = 13 */
                             {XLATE__FLEX_CTR_OFFSET_MODEf,
                              XLATE__FLEX_CTR_POOL_NUMBERf,
                              XLATE__FLEX_CTR_BASE_COUNTER_IDXf},
                             /* L2GRE_DIP type = 13 */
                             {L2GRE_DIP__FLEX_CTR_OFFSET_MODEf,
                              L2GRE_DIP__FLEX_CTR_POOL_NUMBERf,
                              L2GRE_DIP__FLEX_CTR_BASE_COUNTER_IDXf},
                              /* VXLAN_DIP type = 18 */
                             {VXLAN_DIP__FLEX_CTR_OFFSET_MODEf,
                              VXLAN_DIP__FLEX_CTR_POOL_NUMBERf,
                              VXLAN_DIP__FLEX_CTR_BASE_COUNTER_IDXf} };

static flex_counter_fields_t egr_dvp_attribute_counter_fields[]={
                             /* TRILL vp type = 1 */
                             {TRILL__FLEX_CTR_OFFSET_MODEf,
                              TRILL__FLEX_CTR_POOL_NUMBERf,
                              TRILL__FLEX_CTR_BASE_COUNTER_IDXf},
                             /* vxlan type = 2 */
                             {VXLAN__FLEX_CTR_OFFSET_MODEf,
                              VXLAN__FLEX_CTR_POOL_NUMBERf,
                              VXLAN__FLEX_CTR_BASE_COUNTER_IDXf},
                              /* L2Gre vp type = 3 */
                             {L2GRE__FLEX_CTR_OFFSET_MODEf,
                              L2GRE__FLEX_CTR_POOL_NUMBERf,
                              L2GRE__FLEX_CTR_BASE_COUNTER_IDXf} };

#ifdef BCM_TRIDENT2_SUPPORT

static flex_counter_fields_t l3_entry_ipv4_multicast_counter_fields[] = {
                             /* IPV4_UNICAST_EXT, type = 1 */
                             {IPV4UC_EXT__FLEX_CTR_OFFSET_MODEf,
                              IPV4UC_EXT__FLEX_CTR_POOL_NUMBERf,
                              IPV4UC_EXT__FLEX_CTR_BASE_COUNTER_IDXf},
                             /* IPV4_MULTICAST, type = 4 */
                             {IPV4MC__FLEX_CTR_OFFSET_MODEf,
                              IPV4MC__FLEX_CTR_POOL_NUMBERf,
                              IPV4MC__FLEX_CTR_BASE_COUNTER_IDXf},
                             /* FCOE_EXT, type = 13, 15, 17 */ 
                             {FCOE_EXT__FLEX_CTR_OFFSET_MODEf,
                              FCOE_EXT__FLEX_CTR_POOL_NUMBERf,
                              FCOE_EXT__FLEX_CTR_BASE_COUNTER_IDXf} };

static flex_counter_fields_t l3_entry_ipv6_multicast_counter_fields[] = {
                             /* IPV6_UNICAST_EXT, type = 3 */
                             {IPV6UC_EXT__FLEX_CTR_OFFSET_MODEf,
                              IPV6UC_EXT__FLEX_CTR_POOL_NUMBERf,
                              IPV6UC_EXT__FLEX_CTR_BASE_COUNTER_IDXf},
                             /* IPV6_MULTICAST, type = 5 */
                             {IPV6MC__FLEX_CTR_OFFSET_MODEf,
                              IPV6MC__FLEX_CTR_POOL_NUMBERf,
                              IPV6MC__FLEX_CTR_BASE_COUNTER_IDXf} };

#endif

#define BCM_STAT_FLEX_WARMBOOT_READ_CHUNK 256

/* Flex counter collect routine periodically invoked by counter thread */
STATIC void 
_bcm_esw_stat_flex_counter_collect(int                       unit,
                                   bcm_stat_flex_direction_t cnt_direction,
                                   uint32                    flexctr_pool,
                                   uint32                    counter_idx); 

/*
 * Function:
 *      _bcm_esw_get_flex_counter_fields
 * Description:
 *      Gets flex counter fields(FLEX_CTR_OFFSET_MODEf,FLEX_CTR_POOL_NUMBERf
 *      and FLEX_CTR_BASE_COUNTER_IDXf) name for given table.
 *      This is important when a table has several views for flex counter
 *      field and EntryType or KeyType fieled is checked.
 * Parameters:
 *      unit                - (IN) unit number
 *      table               - (IN) Accounting Table 
 *      data                - (IN) Accounting Table Data FLEX_CTR_OFFSET_MODE
 *      offset_mode_field   - (OUT) Field for FLEX_CTR_OFFSET_MODE
 *      pool_number_field   - (OUT) Field for FLEX_CTR_POOL_NUMBER
 *      base_idx_field      - (OUT) Field for FLEX_CTR_BASE_COUNTER_IDX
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

static bcm_error_t 
_bcm_esw_get_flex_counter_fields(
    int         unit,
    uint32     index,
    soc_mem_t   table,
    void        *data,
    soc_field_t *offset_mode_field,
    soc_field_t *pool_number_field,
    soc_field_t *base_idx_field)
{
    uint32 key_type=0;
    uint32 key_type_index=0;
    soc_field_t view_field;
    soc_mem_info_t *memp;
    int rv = BCM_E_NONE;

    *offset_mode_field=FLEX_CTR_OFFSET_MODEf;
    *pool_number_field=FLEX_CTR_POOL_NUMBERf;
    *base_idx_field=FLEX_CTR_BASE_COUNTER_IDXf;
    switch(table) {
    case MPLS_ENTRYm:
    case MPLS_ENTRY_EXTDm:
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "MPLS_ENTRYm   ")));
         memp = &SOC_MEM_INFO(unit, table);
         if (soc_mem_field_valid(unit,table,KEY_TYPEf)) {
             view_field = KEY_TYPEf;
         } else if (soc_mem_field_valid(unit,table,KEY_TYPE_0f)) {
             view_field = KEY_TYPE_0f;
         } else if (soc_mem_field_valid(unit,table,KEY_TYPE_1f)) {
             view_field = KEY_TYPE_1f;
         } else if (soc_mem_field_valid(unit,table,ENTRY_TYPEf)) {
             view_field = ENTRY_TYPEf;
         } else {
             return BCM_E_CONFIG;
         }
         key_type=soc_mem_field32_get(unit,table,data,view_field);
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "key_type %d "),
                    key_type));
         if (sal_strcmp(memp->views[key_type],"MPLS") == 0) {
             key_type_index = 0; 
         } else if (sal_strcmp(memp->views[key_type],"MIM_NVP") == 0) {
             key_type_index = 1; 
         } else if (sal_strcmp(memp->views[key_type],"MIM_ISID") == 0) {
             key_type_index = 2; 
         } else if (sal_strcmp(memp->views[key_type],"TRILL") == 0) {
             key_type_index = 3; 
         } else if (sal_strcmp(memp->views[key_type],"IPV4UC") == 0) {
             key_type_index = 4; 
         } else if (sal_strcmp(memp->views[key_type],"IPV6UC") == 0) {
             key_type_index = 5; 
         } else {
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "KEY TYPE NOT OK %d"),
                        key_type));
             return BCM_E_CONFIG;
         }
         *offset_mode_field= mpls_entry_flex_counter_fields[key_type_index].
                             offset_mode;
         *pool_number_field= mpls_entry_flex_counter_fields[key_type_index].
                             pool_number;
         *base_idx_field=    mpls_entry_flex_counter_fields[key_type_index].
                             base_idx;
         break;
    case EGR_L3_NEXT_HOPm:
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "EGR_L3_NEXT_HOP   =>")));
         memp = &SOC_MEM_INFO(unit, table);
         if (soc_mem_field_valid(unit,table,ENTRY_TYPEf)) {
             view_field = ENTRY_TYPEf;
         } else {
             return BCM_E_CONFIG;
         }
         key_type=soc_mem_field32_get(unit,table,data,view_field);
         if (sal_strcmp(memp->views[key_type],"LEGACY") == 0) {
             key_type_index = 0; 
         } else if (sal_strcmp(memp->views[key_type],"L3") == 0) {
             key_type_index = 1; 
         } else if (sal_strcmp(memp->views[key_type],"WLAN") == 0) {
             key_type_index = 2; 
         } else if (sal_strcmp(memp->views[key_type],"PROXY") == 0) {
             key_type_index = 3; 
         } else if (sal_strcmp(memp->views[key_type],"MPLS") == 0) {
             key_type_index = 4; 
         } else if (sal_strcmp(memp->views[key_type],"SD_TAG") == 0) {
             key_type_index = 5; 
         } else if (sal_strcmp(memp->views[key_type],"MIM") == 0) {
             key_type_index = 6; 
         } else {
             return BCM_E_CONFIG;
         }
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "key_type %d "),
                    key_type));
         *offset_mode_field=egr_l3_next_hop_flex_counter_fields[key_type_index].
                             offset_mode;
         *pool_number_field=egr_l3_next_hop_flex_counter_fields[key_type_index].
                             pool_number;
         *base_idx_field   =egr_l3_next_hop_flex_counter_fields[key_type_index].
                             base_idx;
         break;
    case EGR_VLAN_XLATEm:
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "EGR_VLAN_XLATEm ==")));
         memp = &SOC_MEM_INFO(unit, table);
         if (soc_mem_field_valid(unit,table,ENTRY_TYPEf)) {
             view_field = ENTRY_TYPEf;
         } else if (soc_mem_field_valid(unit,table,KEY_TYPEf)) {
             view_field = KEY_TYPEf;
         } else {
             return BCM_E_CONFIG;
         }
         key_type=soc_mem_field32_get(unit,table,data,view_field);
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "key_type %d "),
                    key_type));
         if (sal_strcmp(memp->views[key_type],"XLATE") == 0) {
             key_type_index = 0; 
         } else if (sal_strcmp(memp->views[key_type],"MIM_ISID") == 0) {
             key_type_index = 1; 
         } else {
             return BCM_E_CONFIG;
         }
         *offset_mode_field= egr_vlan_xlate_flex_counter_fields[key_type_index].
                             offset_mode;
         *pool_number_field= egr_vlan_xlate_flex_counter_fields[key_type_index].
                             pool_number;
         *base_idx_field   = egr_vlan_xlate_flex_counter_fields[key_type_index].
                             base_idx;
         break;
    case VLAN_XLATEm:
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "VLAN_XLATEm ==")));
         memp = &SOC_MEM_INFO(unit, table);
         if (soc_mem_field_valid(unit,table,ENTRY_TYPEf)) {
             view_field = ENTRY_TYPEf;
         } else if (soc_mem_field_valid(unit,table,KEY_TYPEf)) {
             view_field = KEY_TYPEf;
         } else {
             return BCM_E_CONFIG;
         }
         key_type=soc_mem_field32_get(unit,table,data,view_field);

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
             /* In TR3, VLAN_XLATE (vlan_xlate_1) has even entry types,
              *  which supports flex counters 
              */
             if (key_type & 0x1) {
                return BCM_E_CONFIG;
             }
        }
#endif

         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "key_type %d "),
                    key_type));
         if (sal_strcmp(memp->views[key_type],"XLATE") == 0) {
             key_type_index = 0; 
         } else if (sal_strcmp(memp->views[key_type],"L2GRE_DIP") == 0) {
             key_type_index = 1; 
         } else if (sal_strcmp(memp->views[key_type],"VXLAN_DIP") == 0) {
             key_type_index = 2; 
         } else {
             return BCM_E_CONFIG;
         }
         *offset_mode_field= vlan_xlate_flex_counter_fields[key_type_index].
                             offset_mode;
         *pool_number_field= vlan_xlate_flex_counter_fields[key_type_index].
                             pool_number;
         *base_idx_field   = vlan_xlate_flex_counter_fields[key_type_index].
                             base_idx;
         break;
    case VLAN_XLATE_EXTDm:
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "VLAN_XLATE_EXTDm   ")));
         memp = &SOC_MEM_INFO(unit, table);
         if (soc_mem_field_valid(unit,table,KEY_TYPE_0f)) {
             view_field = KEY_TYPE_0f;
         } else if (soc_mem_field_valid(unit,table,KEY_TYPE_1f)) {
             view_field = KEY_TYPE_1f;
         } else {
             return BCM_E_CONFIG;
         }
         key_type=soc_mem_field32_get(unit,table,data,view_field);
         /* VLAN_XLATE_EXTD has only odd entry types */
         if (!(key_type & 0x1)) {
            return BCM_E_CONFIG;
         }
         break;
    case EGR_DVP_ATTRIBUTE_1m:
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "EGR_DVP_ATTRIBUTE_1m   ")));
         memp = &SOC_MEM_INFO(unit, table);
         {
            egr_dvp_attribute_entry_t  egr_dvp_attribute;
            int vp_type;

            sal_memset(&egr_dvp_attribute, 0, sizeof(egr_dvp_attribute_entry_t));
            rv = soc_mem_read(unit, EGR_DVP_ATTRIBUTEm, MEM_BLOCK_ANY, 
                              index, &egr_dvp_attribute);
            vp_type = soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTEm, 
                                         &egr_dvp_attribute, VP_TYPEf);
            key_type_index = 0;
            if (vp_type == 2) { /* vxlan */ 
                key_type_index = 1;
            } else if (vp_type == 3) { /* l2gre */
                key_type_index = 2;
            }
        }
         if (key_type_index) {
             *offset_mode_field= egr_dvp_attribute_counter_fields[key_type_index].
                                 offset_mode;
             *pool_number_field= egr_dvp_attribute_counter_fields[key_type_index].
                                 pool_number;
             *base_idx_field   = egr_dvp_attribute_counter_fields[key_type_index].
                                 base_idx;
        }
        break;
#if defined(BCM_KATANA2_SUPPORT)
    case EGR_VLANm:
        if (SOC_IS_KATANA2(unit)) {
            *offset_mode_field=EGR_FLEX_CTR_OFFSET_MODEf;
            *pool_number_field=EGR_FLEX_CTR_POOL_NUMBERf;
            *base_idx_field=EGR_FLEX_CTR_BASE_COUNTER_IDXf;
        }
        break;
#endif
#ifdef BCM_TRIDENT2_SUPPORT

    case L3_ENTRY_IPV4_MULTICASTm:

         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "L3_ENTRY_IPV4_MULTICAST  ")));
         memp = &SOC_MEM_INFO(unit, table);
         if (soc_mem_field_valid(unit,table,KEY_TYPE_0f)) {
             view_field = KEY_TYPE_0f;
         } else {
             return BCM_E_CONFIG;
         }

         key_type = soc_mem_field32_get(unit, table, data, view_field);
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "key_type %d "),
                    key_type));

         if (key_type == TD2_L3_HASH_KEY_TYPE_V4UC_EXT) {
             key_type_index = 0;
         } else if (key_type == TD2_L3_HASH_KEY_TYPE_V4MC) {
             key_type_index = 1;
         } else if ((key_type == TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN_EXT) ||
                    (key_type == TD2_L3_HASH_KEY_TYPE_FCOE_HOST_EXT) ||
                    (key_type == TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP_EXT)) {
             key_type_index = 2;
         } else {
             return BCM_E_CONFIG;
         }
         *offset_mode_field = 
             l3_entry_ipv4_multicast_counter_fields[key_type_index].offset_mode;
         *pool_number_field = 
             l3_entry_ipv4_multicast_counter_fields[key_type_index].pool_number;
         *base_idx_field   = 
             l3_entry_ipv4_multicast_counter_fields[key_type_index].base_idx;
         break;

    case L3_ENTRY_IPV6_MULTICASTm:

         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "L3_ENTRY_IPV6_MULTICAST  ")));
         memp = &SOC_MEM_INFO(unit, table);
         if (soc_mem_field_valid(unit,table,KEY_TYPE_0f)) {
             view_field = KEY_TYPE_0f;
         } else {
             return BCM_E_CONFIG;
         }

         key_type = soc_mem_field32_get(unit, table, data, view_field);
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "key_type %d "),
                    key_type));

         if (key_type == TD2_L3_HASH_KEY_TYPE_V6UC_EXT) {
             key_type_index = 0;
         } else if (key_type == TD2_L3_HASH_KEY_TYPE_V6MC) {
             key_type_index = 1;
         } else {
             return BCM_E_CONFIG;
         }
         *offset_mode_field = 
             l3_entry_ipv6_multicast_counter_fields[key_type_index].offset_mode;
         *pool_number_field = 
             l3_entry_ipv6_multicast_counter_fields[key_type_index].pool_number;
         *base_idx_field   = 
             l3_entry_ipv6_multicast_counter_fields[key_type_index].base_idx;
         break;

    case EGR_NAT_PACKET_EDIT_INFOm:
         if (index & 1) {
             *offset_mode_field = FLEX_CTR_OFFSET_MODE_1f;
             *pool_number_field = FLEX_CTR_POOL_NUMBER_1f;
             *base_idx_field = FLEX_CTR_BASE_COUNTER_IDX_1f;    
         } else {
             *offset_mode_field = FLEX_CTR_OFFSET_MODE_0f;
             *pool_number_field = FLEX_CTR_POOL_NUMBER_0f;
             *base_idx_field = FLEX_CTR_BASE_COUNTER_IDX_0f; 
         }
         break;    

    case L3_DEFIPm:
         if (index & 0x1) {
             *offset_mode_field = FLEX_CTR_OFFSET_MODE1f;
             *pool_number_field = FLEX_CTR_POOL_NUMBER1f;
             *base_idx_field = FLEX_CTR_BASE_COUNTER_IDX1f;    
         } else {
             *offset_mode_field = FLEX_CTR_OFFSET_MODE0f;
             *pool_number_field = FLEX_CTR_POOL_NUMBER0f;
             *base_idx_field = FLEX_CTR_BASE_COUNTER_IDX0f; 
         }
         break;
#endif
    default: 
         break;
    }
    /* Just a safety check */
    if ((soc_mem_field_valid(unit,table,*offset_mode_field) == FALSE) ||
        (soc_mem_field_valid(unit,table,*pool_number_field) == FALSE) ||
        (soc_mem_field_valid(unit,table,*base_idx_field) == FALSE)) {
        LOG_WARN(BSL_LS_BCM_FLEXCTR,
                 (BSL_META_U(unit,
                             "INTERNAL Error i.e. "
                              "required offset,pool,base_idx fields are not valid \n")));
        return BCM_E_INTERNAL;
    }
    return rv;
}
/*
 * Function:
 *      _bcm_esw_get_flex_counter_fields_values
 * Description:
 *      Gets flex counter fields(FLEX_CTR_OFFSET_MODEf,FLEX_CTR_POOL_NUMBERf
 *      and FLEX_CTR_BASE_COUNTER_IDXf) values for given table.
 *      This is important when a table has several views for flex counter
 *      field and EntryType or KeyType fieled is checked.
 * Parameters:
 *      unit                - (IN) unit number
 *      table               - (IN) Accounting Table 
 *      data                - (IN) Accounting Table Data FLEX_CTR_OFFSET_MODE
 *      offset_mode         - (OUT) FLEX_CTR_OFFSET_MODE field value
 *      pool_number         - (OUT) FLEX_CTR_POOL_NUMBER field value
 *      base_idx            - (OUT) FLEX_CTR_BASE_COUNTER_IDX field value
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static bcm_error_t 
_bcm_esw_get_flex_counter_fields_values(
    int       unit,
    uint32    index,
    soc_mem_t table,
    void      *data,
    uint32    *offset_mode,
    uint32    *pool_number,
    uint32    *base_idx)
{
    soc_field_t offset_mode_field=FLEX_CTR_OFFSET_MODEf;
    soc_field_t pool_number_field=FLEX_CTR_POOL_NUMBERf;
    soc_field_t base_idx_field=FLEX_CTR_BASE_COUNTER_IDXf;
    BCM_IF_ERROR_RETURN(_bcm_esw_get_flex_counter_fields(
                        unit,
                        index,
                        table,
                        data, 
                        &offset_mode_field,
                        &pool_number_field,
                        &base_idx_field));
    *offset_mode =  soc_mem_field32_get(unit,table,data,offset_mode_field);
    *pool_number =  soc_mem_field32_get(unit,table,data,pool_number_field);
    *base_idx =  soc_mem_field32_get(unit,table,data,base_idx_field);
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_set_flex_counter_fields_values
 * Description:
 *      Sets flex counter fields(FLEX_CTR_OFFSET_MODEf,FLEX_CTR_POOL_NUMBERf
 *      and FLEX_CTR_BASE_COUNTER_IDXf) values for given table.
 *      This is important when a table has several views for flex counter
 *      field and EntryType or KeyType fieled is checked.
 * Parameters:
 *      unit                - (IN) unit number
 *      table               - (IN) Accounting Table 
 *      data                - (IN) Accounting Table Data FLEX_CTR_OFFSET_MODE
 *      offset_mode         - (IN) FLEX_CTR_OFFSET_MODE field value
 *      pool_number         - (IN) FLEX_CTR_POOL_NUMBER field value
 *      base_idx            - (IN) FLEX_CTR_BASE_COUNTER_IDX field value
 *
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t 
_bcm_esw_set_flex_counter_fields_values(
    int       unit,
    uint32    index,
    soc_mem_t table,
    void      *data,
    uint32    offset_mode,
    uint32    pool_number,
    uint32    base_idx)
{
    soc_field_t offset_mode_field=FLEX_CTR_OFFSET_MODEf;
    soc_field_t pool_number_field=FLEX_CTR_POOL_NUMBERf;
    soc_field_t base_idx_field=FLEX_CTR_BASE_COUNTER_IDXf;
    BCM_IF_ERROR_RETURN(_bcm_esw_get_flex_counter_fields(
                        unit,
                        index,
                        table,
                        data, 
                        &offset_mode_field,
                        &pool_number_field,
                        &base_idx_field));
    soc_mem_field32_set(unit,table,data,offset_mode_field,offset_mode);
    soc_mem_field32_set(unit,table,data,pool_number_field,pool_number);
    soc_mem_field32_set(unit,table,data,base_idx_field,base_idx);
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_insert_stat_id
 * Description:
 *      Inserts stat id in local scache table. Useful for WARM-BOOT purpose
 * Parameters:
 *      scache_ptr            - (IN) Local scache table pointer
 *      stat_counter_id       - (IN) Flex Stat Counter Id
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static bcm_error_t 
_bcm_esw_stat_flex_insert_stat_id(uint32 *scache_ptr,uint32 stat_counter_id)
{
    uint32 index=0;
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META("Inserting %d "),
               stat_counter_id));
    for (index=0;index<BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE;index++) {
         if (scache_ptr[index] == 0) {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META("Inserted \n")));
             scache_ptr[index] = stat_counter_id;
             break;
         }
    }
    if (index == BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE) {
        return BCM_E_FAIL;
    }
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_delete_stat_id
 * Description:
 *      Deletes stat id from local scache table. Useful for WARM-BOOT purpose
 * Parameters:
 *      scache_ptr            - (IN) Local scache table pointer
 *      stat_counter_id       - (IN) Flex Stat Counter Id
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static bcm_error_t 
_bcm_esw_stat_flex_delete_stat_id(uint32 *scache_ptr,uint32 stat_counter_id)
{
    uint32 index=0;
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META("Deleting ID:%d "),
               stat_counter_id));
    for (index=0;index<BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE;index++) {
         if (scache_ptr[index] == stat_counter_id) {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META("Deleted \n")));
             scache_ptr[index] = 0;
             break;
         }
    }
    if (index == BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE) {
        return BCM_E_FAIL;
    }
    return BCM_E_NONE;
}
#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      _bcm_esw_stat_flex_install_stat_id
 * Description:
 *      Install(ie. configures h/w) as per retrieved stat-id. 
 *      Useful for WARM-BOOT purpose
 * Parameters:
 *      unit                  - (IN) unit number
 *      scache_ptr            - (IN) Local scache table pointer
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static 
bcm_error_t _bcm_esw_stat_flex_install_stat_id(int unit,uint32  *scache_ptr)
{
    uint32                    index=0;
    uint32                    stat_counter_id=0;
    uint32                    pool_number=0;
    uint32                    base_index=0;
    bcm_stat_flex_mode_t      offset_mode=0;
    bcm_stat_object_t         object=bcmStatObjectIngPort;
    bcm_stat_group_mode_t     group_mode= bcmStatGroupModeSingle;
    bcm_stat_flex_direction_t direction=bcmStatFlexDirectionIngress;
    uint32                    total_counters=0;
    bcm_stat_flex_group_mode_related_info_t *flex_group_mode_related_info =
                                 (bcm_stat_flex_group_mode_related_info_t *)
                                 ( scache_ptr +
                                   BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE);
    uint32 mode=0;

    if (_bcm_esw_stat_sync_version_above_equal(unit,BCM_WB_VERSION_1_3)) {
        LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                    (BSL_META_U(unit,
                                "NEW Version\n")));
        for (mode=0; mode < BCM_STAT_FLEX_COUNTER_MAX_MODE ; mode++) {
            if (flex_group_mode_related_info->flags != 0) {
                _bcm_esw_stat_flex_update_ingress_flex_info(unit, mode, 
                    flex_group_mode_related_info->flags,
                    flex_group_mode_related_info->num_selectors,
                    flex_group_mode_related_info->attr_selectors);
                _bcm_esw_stat_flex_set_group_mode(
                                 unit,bcmStatFlexDirectionIngress,
                                 mode, bcmStatGroupModeFlex1 + mode);
                flex_ingress_modes[unit][mode].total_counters = 
                                   flex_group_mode_related_info->total_counters;
            }
            flex_group_mode_related_info++;
       }
       for (mode=0; mode < BCM_STAT_FLEX_COUNTER_MAX_MODE ; mode++) {
            if (flex_group_mode_related_info->flags != 0) {
                _bcm_esw_stat_flex_update_egress_flex_info(unit, mode, 
                    flex_group_mode_related_info->flags,
                    flex_group_mode_related_info->num_selectors,
                    flex_group_mode_related_info->attr_selectors);
                _bcm_esw_stat_flex_set_group_mode(
                                 unit,bcmStatFlexDirectionEgress,
                                 mode, bcmStatGroupModeFlex1 + mode);
                flex_egress_modes[unit][mode].total_counters = 
                                   flex_group_mode_related_info->total_counters;
            }
            flex_group_mode_related_info++;
       }
    } else {
        LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                    (BSL_META_U(unit,
                                "OLD Version so skipping NEW FLEX Layout..\n")));
    }

    for (index=0;index<BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE;index++) {
         if (scache_ptr[index] != 0) {
             stat_counter_id = scache_ptr[index];
             _bcm_esw_stat_get_counter_id_info(
                      stat_counter_id,&group_mode,&object,
                      &offset_mode,&pool_number,&base_index);
             if (_bcm_esw_stat_validate_object(unit,object,&direction) != 
                 BCM_E_NONE) {
                 LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "Invalid object %d so skipping it \n"),
                            object));
                 continue;
             }
             if (direction == bcmStatFlexDirectionIngress) {
                  /* Quite possible..no attachment till now */
                  if (flex_ingress_modes[unit][offset_mode].total_counters
                      == 0) {
                       flex_ingress_modes[unit][offset_mode].total_counters =
                            _flex_group_mode_total_counters_info
                            [bcmStatFlexDirectionIngress][group_mode];
                       flex_ingress_modes[unit][offset_mode].group_mode = 
                            group_mode;
                  }
                  total_counters = flex_ingress_modes[unit][offset_mode].
                                   total_counters;
                  flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                                [pool_number].used_entries   += total_counters;
                  flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                                [pool_number].used_by_tables |= 
                                flex_used_by_table[object];
                  SHR_BITSET(flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                                [pool_number].used_by_objects, object);
             } else {
                  direction = bcmStatFlexDirectionEgress;
                  /* Quite possible..no attachment till now */
                  if (flex_egress_modes[unit][offset_mode].total_counters
                      == 0) {
                       flex_egress_modes[unit][offset_mode].total_counters =
                            _flex_group_mode_total_counters_info
                            [bcmStatFlexDirectionEgress][group_mode];
                       flex_egress_modes[unit][offset_mode].group_mode = 
                            group_mode;
                  }
                  total_counters = flex_egress_modes[unit][offset_mode].
                                   total_counters;
                  flex_pool_stat[unit][bcmStatFlexDirectionEgress]
                                [pool_number].used_entries   += total_counters;
                  flex_pool_stat[unit][bcmStatFlexDirectionEgress]
                                [pool_number].used_by_tables |=
                                flex_used_by_table[object];
             }
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Installing: mode:%d group_mode:%d pool:%d"
                                    "object:%d base:%d\n"),offset_mode,group_mode,
                        pool_number, object,base_index));
             if (total_counters == 0) {
                 LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "Counter=0.Mode not configured in h/w."
                                        "skipping it\n")));
                 continue;
             }
             shr_aidxres_list_reserve_block(
                 flex_aidxres_list_handle[unit][direction][pool_number],
                 base_index,
                 total_counters);
             BCM_STAT_FLEX_COUNTER_LOCK(unit);
             if (direction == bcmStatFlexDirectionIngress) {
                 flex_ingress_modes[unit][offset_mode].reference_count++;
             } else {
                 flex_egress_modes[unit][offset_mode].reference_count++;
             }
             BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
         }
    }
    return BCM_E_NONE;
}
#endif

/*
 * Function:
 *      _bcm_esw_stat_flex_table_index_map
 * Description:
 *      Mapped the table index to table entry index on configured h/w.
 *      Useful in those tabe who has multi-items in each entry.
 * Parameters:
 *      unit                  - (IN) unit number
 *      table                 - (IN) Flex Table
 *      index                 - (IN) Flex Table Index
 * Return Value:
 *      index_mapped          - (OUT) Flex Table Entry Index
 * Notes:
 *
 */
STATIC int 
_bcm_esw_stat_flex_table_index_map(
    int        unit,
    soc_mem_t  table,
    int        index)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        /* Two Items in Each Entry */
        if (table == EGR_NAT_PACKET_EDIT_INFOm ||
            table == L3_DEFIPm) {
            return index >> 1;
        }
    }
#endif

    return index;
}

/*
 * Function:
 *      _bcm_esw_stat_flex_retrieve_group_mode
 * Description:
 *      Retrieves Flex group mode based on configured h/w.
 *      Useful in warm boot case
 * Parameters:
 *      unit                  - (IN) unit number
 *      direction             - (IN) Flex Data flow direction
 *      offset_mode           - (IN) Flex offset mode
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static bcm_error_t 
_bcm_esw_stat_flex_retrieve_group_mode(
    int                       unit,
    bcm_stat_flex_direction_t direction,
    uint32                    offset_mode)
{
    bcm_stat_flex_ing_attr_t                    *ing_attr=NULL;
    bcm_stat_flex_ing_uncmprsd_attr_selectors_t *ing_uncmprsd_attr_selectors =
                                                NULL;
    bcm_stat_flex_ing_cmprsd_attr_selectors_t   *ing_cmprsd_attr_selectors=NULL;
    bcm_stat_flex_ing_pkt_attr_bits_t           *ing_cmprsd_pkt_attr_bits=NULL;

    bcm_stat_flex_egr_attr_t                    *egr_attr=NULL;
    bcm_stat_flex_egr_uncmprsd_attr_selectors_t *egr_uncmprsd_attr_selectors =
                                                NULL;
    bcm_stat_flex_egr_cmprsd_attr_selectors_t   *egr_cmprsd_attr_selectors=NULL;
    bcm_stat_flex_egr_pkt_attr_bits_t           *egr_cmprsd_pkt_attr_bits=NULL;

    uint32 unknown_pkt_val    = _bcm_esw_stat_flex_get_pkt_res_value(
                                unit,_UNKNOWN_PKT);
    uint32 control_pkt_val    =  _bcm_esw_stat_flex_get_pkt_res_value(
                                unit,_CONTROL_PKT);
    uint32 l2uc_pkt_val       = _bcm_esw_stat_flex_get_pkt_res_value(
                                unit,_L2UC_PKT);
    uint32 known_l2mc_pkt_val = _bcm_esw_stat_flex_get_pkt_res_value(
                                unit,_KNOWN_L2MC_PKT);

    if (direction == bcmStatFlexDirectionIngress) {
        if (flex_ingress_modes[unit][offset_mode].available == 0) {
            return BCM_E_NONE;
        }
        ing_attr                    = &(flex_ingress_modes[unit][offset_mode].
                                        ing_attr);
        ing_uncmprsd_attr_selectors = &(ing_attr->uncmprsd_attr_selectors);
        ing_cmprsd_attr_selectors   = &(ing_attr->cmprsd_attr_selectors);
        ing_cmprsd_pkt_attr_bits    = &(ing_cmprsd_attr_selectors->
                                        pkt_attr_bits);

        switch(flex_ingress_modes[unit][offset_mode].total_counters) {
        case 1: 
             /* bcmStatGroupModeSingle */
             return _bcm_esw_stat_flex_set_group_mode(
                                 unit,bcmStatFlexDirectionIngress,
                                 offset_mode,bcmStatGroupModeSingle);
        case 2: 
             /* bcmStatGroupModeDlfAll,bcmStatGroupModeSingleWithControl,
                bcmStatGroupModeSvpType */
             if (ing_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS) {
                 return _bcm_esw_stat_flex_set_group_mode(
                                 unit,bcmStatFlexDirectionIngress,
                                 offset_mode,bcmStatGroupModeSvpType);
             }
             if (ing_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS) {
                 if ((ing_uncmprsd_attr_selectors->
                      offset_table_map[unknown_pkt_val].offset == 1) &&
                     (ing_uncmprsd_attr_selectors->
                      offset_table_map[control_pkt_val].offset == 1)) {
                      return _bcm_esw_stat_flex_set_group_mode(
                                 unit,
                                 bcmStatFlexDirectionIngress,
                                 offset_mode,
                                 bcmStatGroupModeDlfAllWithControl);
                 }
                 if ((ing_uncmprsd_attr_selectors->
                      offset_table_map[unknown_pkt_val].offset == 0) &&
                     (ing_uncmprsd_attr_selectors->
                      offset_table_map[control_pkt_val].offset == 1)) {
                      return _bcm_esw_stat_flex_set_group_mode(
                                 unit,
                                 bcmStatFlexDirectionIngress,
                                 offset_mode,
                                 bcmStatGroupModeSingleWithControl);
                 }
             }
             return BCM_E_PARAM;
        case 3: 
             /* bcmStatGroupModeTrafficType,bcmStatGroupModeDlfAllWithControl */
             if (ing_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS) {
                 if (ing_uncmprsd_attr_selectors->
                     offset_table_map[known_l2mc_pkt_val].offset == 2) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionIngress,
                                offset_mode,
                                bcmStatGroupModeDlfAllWithControl);
                 }
                 if (ing_uncmprsd_attr_selectors->
                     offset_table_map[known_l2mc_pkt_val].offset == 1) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionIngress,
                                offset_mode,
                                bcmStatGroupModeTrafficType);
                 }
             }
             return BCM_E_PARAM;
        case 4: 
             /* bcmStatGroupModeTyped, bcmStatGroupModeTrafficTypeWithControl */
             if (ing_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS) {
                 if (ing_uncmprsd_attr_selectors->
                     offset_table_map[known_l2mc_pkt_val].offset == 2) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionIngress,
                                offset_mode,
                                bcmStatGroupModeTyped);
                 }
                 if (ing_uncmprsd_attr_selectors->
                     offset_table_map[known_l2mc_pkt_val].offset == 1) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionIngress,
                                offset_mode,
                                bcmStatGroupModeTrafficTypeWithControl);
                 }
             } else {
                 if (ing_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                     BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_CNG_ATTR_BITS) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionIngress,
                                offset_mode,
                                bcmStatGroupModeCng);
                 } 
             }
             return BCM_E_PARAM;
        case 5: 
             /* bcmStatGroupModeTypedAll , bcmStatGroupModeTypedWithControl */
             if (ing_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS) {
                 if (ing_uncmprsd_attr_selectors->
                     offset_table_map[l2uc_pkt_val].offset == 1) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionIngress,
                                offset_mode,
                                bcmStatGroupModeTyped);
                 }
                 if (ing_uncmprsd_attr_selectors->
                     offset_table_map[l2uc_pkt_val].offset == 2) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionIngress,
                                offset_mode,
                                bcmStatGroupModeTrafficTypeWithControl);
                 }
             }
             return BCM_E_PARAM;
        case 6: 
             /* bcmStatGroupModeTypedAllWithControl */
            if (ing_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS) {
                return _bcm_esw_stat_flex_set_group_mode(
                           unit,
                           bcmStatFlexDirectionIngress,
                           offset_mode,
                           bcmStatGroupModeTypedAllWithControl);
                }
             return BCM_E_PARAM;
        case 8: 
             /* bcmStatGroupModeDot1P */
             if (ing_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS) {
                 return _bcm_esw_stat_flex_set_group_mode(
                            unit,
                            bcmStatFlexDirectionIngress,
                            offset_mode,
                            bcmStatGroupModeDot1P);
             }
             return BCM_E_PARAM;
        case 16:
             /* bcmStatGroupModeIntPri */
             if (ing_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS) {
                 return _bcm_esw_stat_flex_set_group_mode(
                            unit,
                            bcmStatFlexDirectionIngress,
                            offset_mode,
                            bcmStatGroupModeIntPri);
             }
             return BCM_E_PARAM;
        case 17: 
             /* bcmStatGroupModeDlfIntPri */
             if (ing_attr->packet_attr_type == 
                 bcmStatFlexPacketAttrTypeCompressed) {
                 if ((ing_cmprsd_pkt_attr_bits->int_pri == 4) &&
                     (ing_cmprsd_pkt_attr_bits->pkt_resolution == 1)) {
                      return _bcm_esw_stat_flex_set_group_mode(
                                 unit,
                                 bcmStatFlexDirectionIngress,
                                 offset_mode,
                                 bcmStatGroupModeDlfIntPri);
                 }
             }
             return BCM_E_PARAM;
        case 18: 
             /* bcmStatGroupModeDlfIntPriWithControl */
             if (ing_attr->packet_attr_type == 
                 bcmStatFlexPacketAttrTypeCompressed) {
                 if ((ing_cmprsd_pkt_attr_bits->int_pri == 4) &&
                     (ing_cmprsd_pkt_attr_bits->pkt_resolution == 2)) {
                      return _bcm_esw_stat_flex_set_group_mode(
                                 unit,
                                 bcmStatFlexDirectionIngress,
                                 offset_mode,
                                 bcmStatGroupModeDlfIntPriWithControl);
                 }
             }
             return BCM_E_PARAM;
        case 20: 
             /* bcmStatGroupModeTypedIntPri */
             if (ing_attr->packet_attr_type == 
                 bcmStatFlexPacketAttrTypeCompressed) {
                 if ((ing_cmprsd_pkt_attr_bits->int_pri == 4) &&
                     (ing_cmprsd_pkt_attr_bits->pkt_resolution == 3)) {
                      return _bcm_esw_stat_flex_set_group_mode(
                                 unit,
                                 bcmStatFlexDirectionIngress,
                                 offset_mode,
                                 bcmStatGroupModeTypedIntPri);
                 }
             }
             return BCM_E_PARAM;
        case 21: 
             /* bcmStatGroupModeTypedIntPriWithControl */
             if (ing_attr->packet_attr_type == 
                 bcmStatFlexPacketAttrTypeCompressed) {
                 if ((ing_cmprsd_pkt_attr_bits->int_pri == 4) &&
                     (ing_cmprsd_pkt_attr_bits->pkt_resolution == 3)) {
                      return _bcm_esw_stat_flex_set_group_mode(
                                 unit,
                                 bcmStatFlexDirectionIngress,
                                 offset_mode,
                                 bcmStatGroupModeTypedIntPriWithControl);
                 }
             }
             return BCM_E_PARAM;
        case 64: 
             /* bcmStatGroupModeIntPriCng , bcmStatGroupModeDscp */
             if (ing_attr->packet_attr_type ==  
                 bcmStatFlexPacketAttrTypeUncompressed) {
                 if (ing_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                     BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionIngress,
                                offset_mode,
                                bcmStatGroupModeDscp);
                 }
                 if (ing_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                     (BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_CNG_ATTR_BITS|
                      BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS)) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionIngress,
                                offset_mode,
                                bcmStatGroupModeIntPriCng);
                 }
             }
             return BCM_E_PARAM;
        default:
             LOG_CLI((BSL_META_U(unit,
                                 "INFO:No fixed Counter:%d so probably a FlexGroup\n"),
                      flex_ingress_modes[unit][offset_mode].total_counters));
             return BCM_E_NONE;
        }
    } else {
        if (flex_egress_modes[unit][offset_mode].available == 0) {
            return BCM_E_NONE;
        }
        egr_attr                    = &(flex_egress_modes[unit][offset_mode].
                                        egr_attr);
        egr_uncmprsd_attr_selectors = &(egr_attr->uncmprsd_attr_selectors);
        egr_cmprsd_attr_selectors   = &(egr_attr->cmprsd_attr_selectors);
        egr_cmprsd_pkt_attr_bits    = &(egr_cmprsd_attr_selectors->
                                        pkt_attr_bits);
        switch(flex_egress_modes[unit][offset_mode].total_counters) {
        case 1: 
             /* bcmStatGroupModeSingle, bcmStatGroupModeDlfAll ,
                bcmStatGroupModeSingleWithControl */
             if ((egr_attr->packet_attr_type ==  
                  bcmStatFlexPacketAttrTypeUncompressed) &&
                 (egr_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                  BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS)){
                  return _bcm_esw_stat_flex_set_group_mode(
                             unit,
                             bcmStatFlexDirectionEgress,
                             offset_mode,
                             bcmStatGroupModeSingle);
             }
             return BCM_E_PARAM;
        case 2: 
             /* bcmStatGroupModeTrafficType, bcmStatGroupModeTyped , 
                bcmStatGroupModeTypedAll,
                bcmStatGroupModeTrafficTypeWithControl,
                bcmStatGroupModeDlfAllWithControl,
                bcmStatGroupModeTypedWithControl, 
                bcmStatGroupModeTypedAllWithControl, bcmStatGroupModeSvpType , 
                bcmStatGroupModeDvpType  */
             if (egr_attr->packet_attr_type ==  
                 bcmStatFlexPacketAttrTypeUncompressed) {
                 if (egr_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                     BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_DVP_TYPE_ATTR_BITS) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                         unit,bcmStatFlexDirectionEgress,
                                         offset_mode,bcmStatGroupModeDvpType);
                    }
                 if (egr_uncmprsd_attr_selectors->
                     uncmprsd_attr_bits_selector ==
                     BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionEgress,
                                offset_mode,
                                bcmStatGroupModeSvpType);
                 }
                 if (egr_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                  BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionEgress,
                                offset_mode,
                                bcmStatGroupModeTrafficType);
                 }
             }
             return BCM_E_PARAM;
        case 4: 
             if (egr_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_CNG_ATTR_BITS) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionEgress,
                                offset_mode,
                                bcmStatGroupModeCng);
             }
             return BCM_E_PARAM;
        case 8: 
             /* bcmStatGroupModeDot1P */
             if (egr_attr->packet_attr_type ==  
                 bcmStatFlexPacketAttrTypeUncompressed) {
                 if (egr_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                     BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionEgress,
                                offset_mode,
                                bcmStatGroupModeDot1P);
                 }
             }
             return BCM_E_PARAM;
        case 16:
             /* bcmStatGroupModeIntPri */
             if (egr_attr->packet_attr_type ==  
                 bcmStatFlexPacketAttrTypeUncompressed) {
                 if (egr_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                     BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionEgress,
                                offset_mode,
                                bcmStatGroupModeIntPri);
                 }
             }
             return BCM_E_PARAM;
        case 18: 
             /* bcmStatGroupModeTypedIntPri */
             if (egr_attr->packet_attr_type ==  
                 bcmStatFlexPacketAttrTypeCompressed) {
                 if ((egr_cmprsd_pkt_attr_bits->int_pri == 4) &&
                     (egr_cmprsd_pkt_attr_bits->pkt_resolution == 1)) {
                      return _bcm_esw_stat_flex_set_group_mode(
                                 unit,
                                 bcmStatFlexDirectionEgress,
                                 offset_mode,
                                 bcmStatGroupModeTypedIntPri);
                  }
             }
             return BCM_E_PARAM; 
        case 64: 
             /* bcmStatGroupModeIntPriCng , bcmStatGroupModeDscp */
             if (egr_attr->packet_attr_type ==  
                 bcmStatFlexPacketAttrTypeUncompressed) {
                 if (egr_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                     BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionEgress,
                                offset_mode,
                                bcmStatGroupModeDscp);
                 }
                 if (egr_uncmprsd_attr_selectors->uncmprsd_attr_bits_selector ==
                     (BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_CNG_ATTR_BITS|
                      BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS)) {
                     return _bcm_esw_stat_flex_set_group_mode(
                                unit,
                                bcmStatFlexDirectionEgress,
                                offset_mode,
                                bcmStatGroupModeIntPriCng);
                 }
             }
             return BCM_E_PARAM;
        default:
             LOG_CLI((BSL_META_U(unit,
                                 "INFO:No fixed Counter:%d so probably a FlexGroup\n"),
                      flex_ingress_modes[unit][offset_mode].total_counters));
             return BCM_E_NONE;
        }
    }
    return BCM_E_PARAM;
}


/*
 * Function:
 *      _bcm_esw_stat_flex_enable_pool
 * Description:
 *      Enable/Disable ingress/egress flex counter pool.
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static bcm_error_t _bcm_esw_stat_flex_enable_pool(
            int                       unit,
            bcm_stat_flex_direction_t direction,
            soc_reg_t                 flex_pool_ctr_update_control_reg,
            uint32                    enable)
{
    uint32  flex_pool_ctr_update_control_reg_value=0;
    uint32  enable_value=1;
    uint32  num_pools[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32  index=0;

    num_pools[bcmStatFlexDirectionIngress]=
             SOC_INFO(unit).num_flex_ingress_pools;
    num_pools[bcmStatFlexDirectionEgress] =
             SOC_INFO(unit).num_flex_egress_pools;
    if (direction >= BCM_STAT_FLEX_COUNTER_MAX_DIRECTION) {
        return BCM_E_PARAM;
    }
    for(index=0;
        index < num_pools[direction];
        index++) {
        if (_pool_ctr_register[direction][index] == 
            flex_pool_ctr_update_control_reg ) {
            break;
        }
    }
    if (index == num_pools[direction]) {
         return BCM_E_PARAM;
    }
    if ( enable ) {
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "...Enabling pool:%s \n"),
                   SOC_REG_NAME(unit, flex_pool_ctr_update_control_reg)));
    } else {
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "...Disabling pool:%s \n"),
                   SOC_REG_NAME(unit, flex_pool_ctr_update_control_reg)));
    }
    /* First Get complete value of
       EGR/ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_?r value */
    SOC_IF_ERROR_RETURN(soc_reg32_get(
                        unit,
                        flex_pool_ctr_update_control_reg,
                        REG_PORT_ANY, 0,
                        &flex_pool_ctr_update_control_reg_value));
    /* Next set field value for
       EGR/ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_?r:COUNTER_POOL_ENABLE field*/
    if (enable) {
        enable_value=1;
    }else {
        enable_value=0;
    }
    soc_reg_field_set(unit,
                      flex_pool_ctr_update_control_reg,
                      &flex_pool_ctr_update_control_reg_value,
                      COUNTER_POOL_ENABLEf,
                      enable_value);
    /* Finally set value for
       EGR/ING_FLEX_CTR_COUNTER_UPDATE_CONTROL_?r:COUNTER_POOL_ENABLE field*/
    SOC_IF_ERROR_RETURN(soc_reg32_set(unit,
                                      flex_pool_ctr_update_control_reg,
                                      REG_PORT_ANY, 
                                      0,
                                      flex_pool_ctr_update_control_reg_value));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_stat_flex_retrieve_total_counters
 * Description:
 *      Retries total counter based on flex h/w configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static uint32 
_bcm_esw_stat_flex_retrieve_total_counters(
    int                       unit,
    bcm_stat_flex_direction_t direction,
    uint32                    pool_number,
    uint32                    offset_mode)
{
    uint32                             index=0;
    ing_flex_ctr_offset_table_0_entry_t *flex_ctr_offset_table_entry=NULL;
    uint32                             count_enable=0;
    bcm_stat_flex_offset_table_entry_t *sw_offset_table=NULL;
    uint32                             offset=0;
    uint32                             total_counters_flag=0;
    uint32                             total_counters=0;
    uint32                             alloc_size=0;


    /* Actually we need only 256 offsets only but ...*/
    alloc_size =  soc_mem_index_count(unit,ING_FLEX_CTR_OFFSET_TABLE_0m) *
                  sizeof(ing_flex_ctr_offset_table_0_entry_t);

    flex_ctr_offset_table_entry = soc_cm_salloc(unit,alloc_size,
                                                "flex_ctr_offset_table_entry");
    if (flex_ctr_offset_table_entry == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Memory Allocation failed:flex_ctr_offset_table_entry\n")));
        return  BCM_E_INTERNAL;
    }
    /* 0-255,256-511,512-...*/
    if (soc_mem_read_range(unit,
                     _ctr_offset_table[direction][pool_number],
                     MEM_BLOCK_ANY,
                     (offset_mode <<8),
                     (offset_mode <<8)+ (256) - 1,
                     flex_ctr_offset_table_entry) != SOC_E_NONE) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Memory Reading failed:flex_ctr_offset_table_entry \n")));
        soc_cm_sfree(unit,flex_ctr_offset_table_entry);
        return 0;
    }
    if (direction == bcmStatFlexDirectionIngress) {
        switch(flex_ingress_modes[unit][offset_mode].ing_attr.
               packet_attr_type) {
        case bcmStatFlexPacketAttrTypeUncompressed:
             sw_offset_table = &flex_ingress_modes[unit][offset_mode].ing_attr.
                                uncmprsd_attr_selectors.offset_table_map[0];
             break;
        case bcmStatFlexPacketAttrTypeCompressed:
             sw_offset_table = &flex_ingress_modes[unit][offset_mode].ing_attr.
                                cmprsd_attr_selectors.offset_table_map[0];
             break;
        case bcmStatFlexPacketAttrTypeUdf:
        default:
             /* With current implemControl must not reach over here */
             return BCM_E_PARAM;
    }
    } else {
        switch(flex_egress_modes[unit][offset_mode].egr_attr.packet_attr_type) {
        case bcmStatFlexPacketAttrTypeUncompressed:
             sw_offset_table = &flex_egress_modes[unit][offset_mode].egr_attr.
                                uncmprsd_attr_selectors.offset_table_map[0];
             break;
        case bcmStatFlexPacketAttrTypeCompressed:
             sw_offset_table = &flex_egress_modes[unit][offset_mode].egr_attr.
                                cmprsd_attr_selectors.offset_table_map[0];
             break;
        case bcmStatFlexPacketAttrTypeUdf:
        default:
             /* With current implemControl must not reach over here */
             return BCM_E_PARAM;
        }
    }
    for (index=0;index<(256);index++) {
         count_enable=0;
         /*First Get complete value of EGR/ING_FLEX_CTR_OFFSET_TABLE_?m value */
         soc_mem_field_get(unit,
                           _ctr_offset_table[direction][pool_number],
                           (uint32 *)&flex_ctr_offset_table_entry[index],
                           COUNT_ENABLEf,&count_enable);
         soc_mem_field_get(unit,
                           _ctr_offset_table[direction][pool_number],
                           (uint32 *)&flex_ctr_offset_table_entry[index],
                           OFFSETf,
                           &offset);
         if (count_enable) {
             total_counters_flag=1;
             if (total_counters < offset) {
                 total_counters = offset;
             }
         }
         sw_offset_table[index].offset = offset;
         sw_offset_table[index].count_enable = count_enable;
    }
    soc_cm_sfree(unit,flex_ctr_offset_table_entry);
    return (total_counters+total_counters_flag);
}

/*
 * Function:
 *      _bcm_esw_stat_flex_check_egress_table
 * Description:
 *      Checkes flex egress table. If table were configured for flex counter
 *      updates s/w copy accordingly.    
 *
 * Parameters:
 *      unit                  - (IN) unit number
 *      egress_table          - (IN) Flex Egress  Table
 *      index                 - (IN) Flex Egress  Table Index
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only.
 *
 */
static void
_bcm_esw_stat_flex_check_egress_table(
    int       unit,
    soc_mem_t egress_table,
    int       min_index,
    int       max_index)
{
    uint32            offset_mode=0;
    uint32            pool_number=0;
    uint32            base_idx=0;
    uint32            egress_entry_data_size=0;
    void              *egress_entry_data=NULL;
    void              *egress_entry_full_data=NULL;
    bcm_stat_object_t object=bcmStatObjectEgrPort;
    int               index=0; 

    if (!((egress_table == EGR_VLANm) ||
          (egress_table == EGR_VFIm)  ||
          (egress_table == EGR_L3_NEXT_HOPm)  ||
#if defined(BCM_TRIDENT2_SUPPORT)
          (egress_table == EGR_DVP_ATTRIBUTE_1m)  ||
          (egress_table == EGR_NAT_PACKET_EDIT_INFOm)  ||
#endif /* BCM_TRIDENT2_SUPPORT */
          (egress_table == EGR_VLAN_XLATEm)  ||
          (egress_table == EGR_PORTm))) {
           LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                     (BSL_META_U(unit,
                                 "Invalid Flex Counter Ingress Memory %s\n"),
                      SOC_MEM_UFNAME(unit, egress_table)));
           return;
    }
    if (max_index > soc_mem_index_max(unit,egress_table)) {
        max_index = soc_mem_index_max(unit,egress_table);
    }
    egress_entry_data_size= WORDS2BYTES(BYTES2WORDS(
                                        SOC_MEM_INFO(unit,egress_table).bytes));
    egress_entry_full_data = soc_cm_salloc(unit,egress_entry_data_size *
                                  (max_index - min_index + 1),"egress_table");
    if (egress_entry_full_data == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Failed to allocate memory for Table:%s "),
                   SOC_MEM_UFNAME(unit, egress_table)));
        return ;
    }
    sal_memset(egress_entry_full_data,0,(egress_entry_data_size *
                                  (max_index - min_index + 1)));

    if (soc_mem_read_range(unit, egress_table, MEM_BLOCK_ANY,
                min_index,max_index, egress_entry_full_data) != SOC_E_NONE) {
        soc_cm_sfree(unit,egress_entry_full_data);
        return;
    }
    BCM_STAT_FLEX_COUNTER_LOCK(unit);
    for(index=0; index <= (max_index - min_index) ; index++) {
        egress_entry_data = soc_mem_table_idx_to_pointer(unit, egress_table, 
                            void *, egress_entry_full_data, index);
        if (soc_mem_field_valid(unit,egress_table,VALIDf)) {
            if (soc_mem_field32_get(unit,egress_table,egress_entry_data,
                VALIDf)==0) {
                continue;
            }
        }
        _bcm_esw_get_flex_counter_fields_values(
                          unit,index+min_index,egress_table,
                          egress_entry_data,
                          &offset_mode,&pool_number,&base_idx);
        if ((offset_mode == 0) && (base_idx == 0)) {
             continue;
        }
        if (egress_table == EGR_VFIm) {
            object = bcmStatObjectEgrVfi;
        } else if (egress_table == EGR_L3_NEXT_HOPm) {
            object = bcmStatObjectEgrL3Intf;
        } else if (_bcm_esw_stat_flex_get_egress_object(unit,egress_table,
            index+min_index,egress_entry_data,&object) != BCM_E_NONE) {
            continue;
        }
        flex_base_index_reference_count[unit][bcmStatFlexDirectionEgress]
                                       [pool_number][base_idx]++;
        if (flex_base_index_reference_count[unit][bcmStatFlexDirectionEgress]
                                           [pool_number][base_idx] == 1) {
            flex_egress_modes[unit][offset_mode].reference_count++;
        }
        if (flex_egress_modes[unit][offset_mode].total_counters == 0) {
            flex_egress_modes[unit][offset_mode].total_counters =
               _bcm_esw_stat_flex_retrieve_total_counters(
                  unit, bcmStatFlexDirectionEgress, pool_number, offset_mode); 
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Max_offset_table_value %d\n"),
                       flex_egress_modes[unit][offset_mode].
                       total_counters));
        }
        shr_aidxres_list_reserve_block(
            flex_aidxres_list_handle
            [unit][bcmStatFlexDirectionEgress][pool_number],
            base_idx,
            flex_egress_modes[unit][offset_mode].total_counters);
        flex_pool_stat[unit][bcmStatFlexDirectionEgress][pool_number].
                      used_entries += flex_egress_modes[unit][offset_mode].
                                                       total_counters;
        flex_pool_stat[unit][bcmStatFlexDirectionEgress][pool_number].
                      attached_entries += flex_egress_modes[unit][offset_mode].
                                                       total_counters;
        flex_pool_stat[unit][bcmStatFlexDirectionEgress][pool_number].
                      used_by_tables |= flex_used_by_table[object];
        SHR_BITSET(flex_pool_stat[unit][bcmStatFlexDirectionEgress]
                      [pool_number].used_by_objects, object);
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Table:%s index=%d mode:%d pool_number:%d base_idx:%d\n"),
                   SOC_MEM_UFNAME(unit, egress_table),
                   index+min_index,offset_mode,pool_number,base_idx));
    }
    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
    soc_cm_sfree(unit,egress_entry_full_data);
    return;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_ingress_table
 * Description:
 *      Checkes flex ingress table. If table were configured for flex counter
 *      updates s/w copy accordingly.    
 *
 * Parameters:
 *      unit                  - (IN) unit number
 *      ingress_table         - (IN) Flex Ingress  Table
 *      index                 - (IN) Flex Ingress  Table Index
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only.
 *
 */
static void 
_bcm_esw_stat_flex_check_ingress_table(
    int       unit,
    soc_mem_t ingress_table,
    int       min_index,
    int       max_index)
{
    uint32            offset_mode=0;
    uint32            pool_number=0;
    uint32            base_idx=0;
    uint32            ingress_entry_data_size=0;
    void              *ingress_entry_data=NULL;
    void              *ingress_entry_full_data=NULL;
    bcm_stat_object_t object=bcmStatObjectIngPort;
    int               index=0;

    if (!((ingress_table == PORT_TABm) ||
          (ingress_table == VLAN_XLATEm)  ||
#if defined(BCM_TRIUMPH3_SUPPORT)
          (ingress_table == VLAN_XLATE_EXTDm)  ||
          (ingress_table == MPLS_ENTRY_EXTDm)  ||
          (ingress_table == EXT_FP_POLICYm)    ||
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
          (ingress_table == ING_VSANm)  ||
          (ingress_table == L3_ENTRY_IPV4_MULTICASTm)  ||
          (ingress_table == L3_ENTRY_IPV6_MULTICASTm)  ||          
          (ingress_table == L3_DEFIPm) ||
          (ingress_table == L3_DEFIP_PAIR_128m)  ||
          (ingress_table == L3_DEFIP_ALPM_IPV4_1m) ||
          (ingress_table == L3_DEFIP_ALPM_IPV6_64_1m)  ||
          (ingress_table == L3_DEFIP_ALPM_IPV6_128m) ||
          (ingress_table == L3_DEFIP_AUX_SCRATCHm) ||
#endif /* BCM_TRIDENT2_SUPPORT */
          (ingress_table == VFP_POLICY_TABLEm)  ||
          (ingress_table == MPLS_ENTRYm)  ||
          (ingress_table == SOURCE_VPm)  ||
          (ingress_table == L3_IIFm)  ||
          (ingress_table == VRFm)  ||
          (ingress_table == VFIm)  ||
          (ingress_table == VLAN_TABm))) {
           LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                     (BSL_META_U(unit,
                                 "Invalid Flex Counter Ingress Memory %s\n"),
                      SOC_MEM_UFNAME(unit, ingress_table)));
           return;
    }
    ingress_entry_data_size = WORDS2BYTES(BYTES2WORDS(
                              SOC_MEM_INFO(unit, ingress_table).bytes));
    if (max_index > soc_mem_index_max(unit,ingress_table)) {
        max_index = soc_mem_index_max(unit,ingress_table);
    }
 
    ingress_entry_full_data = soc_cm_salloc(unit,ingress_entry_data_size *
                                  (max_index - min_index + 1),"ingress_table");
    if (ingress_entry_full_data == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Failed to allocate memory for Table:%s "),
                   SOC_MEM_UFNAME(unit, ingress_table)));
        return ;
    }
    sal_memset(ingress_entry_full_data,0,(ingress_entry_data_size *
                                  (max_index - min_index + 1)));

    if (soc_mem_read_range(unit, ingress_table, MEM_BLOCK_ANY,
                min_index,max_index, ingress_entry_full_data) != SOC_E_NONE) {
        soc_cm_sfree(unit,ingress_entry_full_data);
        return ;
    }

    BCM_STAT_FLEX_COUNTER_LOCK(unit);
            
   for(index=0; index <= (max_index - min_index) ; index++) {
       ingress_entry_data = soc_mem_table_idx_to_pointer(unit, ingress_table, 
                            void *, ingress_entry_full_data, index);
        if (soc_mem_field_valid(unit,ingress_table,VALIDf)) {
            if (soc_mem_field32_get(unit,ingress_table,ingress_entry_data,
                VALIDf)==0) {
                continue;
            }
        } 
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            if (soc_mem_field_valid(unit,ingress_table,VALID_0f)) {
                if (soc_mem_field32_get(unit,ingress_table,ingress_entry_data,
                    VALID_0f)==0) {
                    continue;
                }
            } 
            if (soc_mem_field_valid(unit,ingress_table,VALID_1f)) {
                if (soc_mem_field32_get(unit,ingress_table,ingress_entry_data,
                    VALID_1f)==0) {
                    continue;
                }
            }     
        }
#endif
        _bcm_esw_get_flex_counter_fields_values(
               unit,index+min_index,ingress_table,ingress_entry_data,
               &offset_mode,&pool_number,&base_idx);
        if ((offset_mode == 0) && (base_idx == 0)) {
             continue;
        }
        /* object for these two tables rely on virtual module which is not
         * initialized yet, For the purpose here, just need to find any 
         * valid object using the table
         */
        if (ingress_table == SOURCE_VPm) {
            object = bcmStatObjectIngMplsVcLabel;
        } else if (ingress_table == VFIm) {
            object = bcmStatObjectIngVfi;
        } else if (_bcm_esw_stat_flex_get_ingress_object(
            unit,ingress_table,index+min_index,ingress_entry_data,&object) !=BCM_E_NONE) {
            continue;
        }
        flex_base_index_reference_count[unit][bcmStatFlexDirectionIngress]
                                       [pool_number][base_idx]++;
        if (flex_base_index_reference_count[unit][bcmStatFlexDirectionIngress]
                                           [pool_number][base_idx] == 1) {
            flex_ingress_modes[unit][offset_mode].reference_count++;
        }
        if (flex_ingress_modes[unit][offset_mode].total_counters == 0) {
            flex_ingress_modes[unit][offset_mode].total_counters =
                   _bcm_esw_stat_flex_retrieve_total_counters(
                    unit, bcmStatFlexDirectionIngress,
                    pool_number, offset_mode)  ;
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Max_offset_table_value %d\n"),
                       flex_ingress_modes[unit][offset_mode].
                       total_counters));
        }
        shr_aidxres_list_reserve_block(
            flex_aidxres_list_handle[unit][bcmStatFlexDirectionIngress]
                                    [pool_number],
            base_idx,
            flex_ingress_modes[unit][offset_mode].
            total_counters);
        flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                      [pool_number].used_entries +=
                      flex_ingress_modes[unit][offset_mode].total_counters;
        flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                      [pool_number].attached_entries +=
                      flex_ingress_modes[unit][offset_mode].total_counters;
        flex_pool_stat[unit][bcmStatFlexDirectionIngress][pool_number].
                      used_by_tables |= flex_used_by_table[object];
        SHR_BITSET(flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                      [pool_number].used_by_objects, object);
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Table:%s:index=%d mode:%d pool_number:%d base_idx:%d\n"),
                   SOC_MEM_UFNAME(unit, ingress_table),
                   index+min_index,offset_mode,pool_number, base_idx));
    }
    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
    soc_cm_sfree(unit,ingress_entry_full_data);
    return;
}

/*
 * Function:
 *      _bcm_esw_stat_flex_init_uncmprsd_mode
 * Description:
 *      Configures flex hw for uncompressed mode
 * Parameters:
 *      unit                           - (IN) unit number
 *      mode                           - (IN) Flex Offset Mode
 *      direction                      - (IN) Flex Data flow direction(ing/egr)
 *      selector_count                 - (IN) Flex Key selector count
 *      selector_x_en_field_value      - (IN) Flex Key selector fields
 *      selector_for_bit_x_field_value - (IN) Flex Key selector field bits
 *
 * Return Value:
 *      None
 * Notes:
 *
 */
static void 
_bcm_esw_stat_flex_init_uncmprsd_mode(
    int                       unit,
    int                       mode,
    bcm_stat_flex_direction_t direction,
    uint32                    selector_count,
    uint32                    selector_x_en_field_value[8],
    uint32                    selector_for_bit_x_field_value[8])
{
    uint32  uncmprsd_attr_bits_selector=0;
    uint32  index=0;

    if (direction == bcmStatFlexDirectionIngress) {
        flex_ingress_modes[unit][mode].available=1;
        /*Will be changed WhileReadingTables */
        flex_ingress_modes[unit][mode].reference_count=0;
        /* selector_count can not be used for deciding total counters !!! */
        /* flex_ingress_modes[unit][mode].total_counters=(1<< selector_count);*/
        flex_ingress_modes[unit][mode].ing_attr.packet_attr_type =
            bcmStatFlexPacketAttrTypeUncompressed;
        for (uncmprsd_attr_bits_selector=0,index=0;index<8;index++) {
             if(selector_x_en_field_value[index] != 0) {
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.ip_pkt_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_uncmprsd_bits_g.ip_pkt_pos+
                       ing_pkt_attr_uncmprsd_bits_g.ip_pkt-1))) { /* 0..0 */
                    uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.drop_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_uncmprsd_bits_g.drop_pos+
                       ing_pkt_attr_uncmprsd_bits_g.drop-1))) { /* 1..1 */
                    uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_DROP_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.svp_type_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_uncmprsd_bits_g.svp_type_pos+
                       ing_pkt_attr_uncmprsd_bits_g.svp_type-1))) { /* 3..8 */
                    uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.pkt_resolution_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_uncmprsd_bits_g.pkt_resolution_pos+
                       ing_pkt_attr_uncmprsd_bits_g.pkt_resolution-1))) { /* 3..8 */
                     uncmprsd_attr_bits_selector|=
                   BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.tos_dscp_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (ing_pkt_attr_uncmprsd_bits_g.tos_dscp_pos+ing_pkt_attr_uncmprsd_bits_g.tos_dscp-1))) {
                     /* 9..16 */ 
                     uncmprsd_attr_bits_selector|=
                       BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.tos_ecn_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (ing_pkt_attr_uncmprsd_bits_g.tos_ecn_pos+ing_pkt_attr_uncmprsd_bits_g.tos_ecn-1))) {
                     /* 9..16 */ 
                     uncmprsd_attr_bits_selector|=
                       BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_TOS_ECN_ATTR_BITS;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.ing_port_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (ing_pkt_attr_uncmprsd_bits_g.ing_port_pos+
                      ing_pkt_attr_uncmprsd_bits_g.ing_port-1))) { /* 17 .. 22 */
                     uncmprsd_attr_bits_selector|=
                      BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INGRESS_PORT_ATTR_BITS;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.inner_dot1p_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (ing_pkt_attr_uncmprsd_bits_g.inner_dot1p_pos +
                      ing_pkt_attr_uncmprsd_bits_g.inner_dot1p - 1))) { /* 23..25 */
                     uncmprsd_attr_bits_selector|=
                       BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.outer_dot1p_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (ing_pkt_attr_uncmprsd_bits_g.outer_dot1p_pos +
                      ing_pkt_attr_uncmprsd_bits_g.outer_dot1p - 1))) { /* 26..28 */
                     uncmprsd_attr_bits_selector|=
                       BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.vlan_format_pos) &&
                    (selector_for_bit_x_field_value[index] <=
                     (ing_pkt_attr_uncmprsd_bits_g.vlan_format_pos + 
                      ing_pkt_attr_uncmprsd_bits_g.vlan_format - 1))) { /* 29..30 */
                     uncmprsd_attr_bits_selector|=
                       BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_VLAN_FORMAT_ATTR_BITS;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.int_pri_pos) &&
                    (selector_for_bit_x_field_value[index] <=
                     (ing_pkt_attr_uncmprsd_bits_g.int_pri_pos + 
                      ing_pkt_attr_uncmprsd_bits_g.int_pri -1))) { /* 31 .. 34 */
                     uncmprsd_attr_bits_selector|=
                       BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >=
                     ing_pkt_attr_uncmprsd_bits_g.ifp_cng_pos ) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (ing_pkt_attr_uncmprsd_bits_g.ifp_cng_pos +
                     ing_pkt_attr_uncmprsd_bits_g.ifp_cng - 1))) { /* 35..36 */
                     uncmprsd_attr_bits_selector|=
                       BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_IFP_CNG_ATTR_BITS;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_uncmprsd_bits_g.cng_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (ing_pkt_attr_uncmprsd_bits_g.cng_pos +
                      ing_pkt_attr_uncmprsd_bits_g.cng - 1))) { /* 37 .. 38 */
                     uncmprsd_attr_bits_selector|=
                       BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_CNG_ATTR_BITS;
                     continue;
                }
             }
        }
        flex_ingress_modes[unit][mode].ing_attr.uncmprsd_attr_selectors.
                     uncmprsd_attr_bits_selector = uncmprsd_attr_bits_selector;
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "uncmprsd_attr_bits_selector:%x \n"),
                   uncmprsd_attr_bits_selector));
        return;
    }
    if (direction == bcmStatFlexDirectionEgress) {
        flex_egress_modes[unit][mode].available=1;
        /* Will be changed WhileReadingTables */
        flex_egress_modes[unit][mode].reference_count=0;
        flex_egress_modes[unit][mode].egr_attr.packet_attr_type =
                                      bcmStatFlexPacketAttrTypeUncompressed;
        for (uncmprsd_attr_bits_selector=0,index=0;index<8;index++) {
             if(selector_x_en_field_value[index] != 0) {
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.ip_pkt_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_uncmprsd_bits_g.ip_pkt_pos+
                       egr_pkt_attr_uncmprsd_bits_g.ip_pkt-1))) { /* 0..0 */
                    uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.drop_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_uncmprsd_bits_g.drop_pos+
                       egr_pkt_attr_uncmprsd_bits_g.drop-1))) { /* 1..1 */
                    uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_DROP_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.dvp_type_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_uncmprsd_bits_g.dvp_type_pos+
                       egr_pkt_attr_uncmprsd_bits_g.dvp_type-1))) { /* 3..8 */
                    uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_DVP_TYPE_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.svp_type_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_uncmprsd_bits_g.svp_type_pos+
                       egr_pkt_attr_uncmprsd_bits_g.svp_type-1))) { /* 3..8 */
                    uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.pkt_resolution_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_uncmprsd_bits_g.pkt_resolution_pos+
                       egr_pkt_attr_uncmprsd_bits_g.pkt_resolution-1))) { /* 4..4 */
                    uncmprsd_attr_bits_selector |=
                    BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.tos_dscp_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (egr_pkt_attr_uncmprsd_bits_g.tos_dscp_pos+egr_pkt_attr_uncmprsd_bits_g.tos_dscp-1))) {
                     /* 5..12 */
                     uncmprsd_attr_bits_selector |=
                       BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.tos_ecn_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (egr_pkt_attr_uncmprsd_bits_g.tos_ecn_pos+egr_pkt_attr_uncmprsd_bits_g.tos_ecn-1))) {
                     /* 5..12 */
                     uncmprsd_attr_bits_selector |=
                       BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_TOS_ECN_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.egr_port_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (egr_pkt_attr_uncmprsd_bits_g.egr_port_pos+
                      egr_pkt_attr_uncmprsd_bits_g.egr_port-1))) { /* 13..18 */
                     uncmprsd_attr_bits_selector |=
                       BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_EGRESS_PORT_ATTR_BITS;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.inner_dot1p_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (egr_pkt_attr_uncmprsd_bits_g.inner_dot1p_pos+
                      egr_pkt_attr_uncmprsd_bits_g.inner_dot1p-1))) { /* 19..21*/
                     uncmprsd_attr_bits_selector |=
                       BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.outer_dot1p_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (egr_pkt_attr_uncmprsd_bits_g.outer_dot1p_pos+
                      egr_pkt_attr_uncmprsd_bits_g.outer_dot1p-1))) { /* 22..24*/
                     uncmprsd_attr_bits_selector |=
                       BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.vlan_format_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (egr_pkt_attr_uncmprsd_bits_g.vlan_format_pos+
                      egr_pkt_attr_uncmprsd_bits_g.vlan_format-1))) { /* 25..26 */
                     uncmprsd_attr_bits_selector |=
                       BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_VLAN_FORMAT_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.int_pri_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (egr_pkt_attr_uncmprsd_bits_g.int_pri_pos +
                      egr_pkt_attr_uncmprsd_bits_g.int_pri - 1))) { /* 27..30 */
                     uncmprsd_attr_bits_selector |=
                       BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_uncmprsd_bits_g.cng_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                     (egr_pkt_attr_uncmprsd_bits_g.cng_pos + 
                      egr_pkt_attr_uncmprsd_bits_g.cng - 1))) { /* 31..32 */
                     uncmprsd_attr_bits_selector |=
                       BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_CNG_ATTR_BITS;
                     continue;
                }
            }
        }
        flex_egress_modes[unit][mode].egr_attr.uncmprsd_attr_selectors.
                    uncmprsd_attr_bits_selector = uncmprsd_attr_bits_selector;
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "uncmprsd_attr_bits_selector:%x \n"),
                   uncmprsd_attr_bits_selector));
        return;
    }
    LOG_ERROR(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Ooops. Control Must not reach over here \n")));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_init_cmprsd_mode
 * Description:
 *      Configures flex hw for compressed mode
 * Parameters:
 *      unit                           - (IN) unit number
 *      mode                           - (IN) Flex Offset Mode
 *      direction                      - (IN) Flex Data flow direction(ing/egr)
 *      selector_count                 - (IN) Flex Key selector count
 *      selector_x_en_field_value      - (IN) Flex Key selector fields
 *      selector_for_bit_x_field_value - (IN) Flex Key selector field bits
 *
 * Return Value:
 *      None
 * Notes:
 *
 */
static void 
_bcm_esw_stat_flex_init_cmprsd_mode(
    int                       unit,
    int                       mode,
    bcm_stat_flex_direction_t direction,
    uint32                    selector_count,
    uint32                    selector_x_en_field_value[8],
    uint32                    selector_for_bit_x_field_value[8])
{
    uint32  index=0;

    uint32                            alloc_size=0;
    uint32                            map_value=0;
    ing_flex_ctr_pkt_pri_map_entry_t  *ing_pkt_pri_map_dma=NULL;
    ing_flex_ctr_pkt_res_map_entry_t  *ing_pkt_res_map_dma=NULL;
    ing_flex_ctr_port_map_entry_t     *ing_port_map_dma=NULL;
    ing_flex_ctr_pri_cng_map_entry_t  *ing_pri_cng_map_dma=NULL;
    ing_flex_ctr_tos_map_entry_t      *ing_tos_map_dma=NULL;
    egr_flex_ctr_pkt_pri_map_entry_t  *egr_pkt_pri_map_dma=NULL;
    egr_flex_ctr_pkt_res_map_entry_t  *egr_pkt_res_map_dma=NULL;
    egr_flex_ctr_port_map_entry_t     *egr_port_map_dma=NULL;
    egr_flex_ctr_pri_cng_map_entry_t  *egr_pri_cng_map_dma=NULL;
    egr_flex_ctr_tos_map_entry_t      *egr_tos_map_dma=NULL;

    if (direction == bcmStatFlexDirectionIngress) {
       /* memset(&flex_ingress_modes[unit][mode],0,
         sizeof(flex_ingress_modes[unit][mode])); */
       flex_ingress_modes[unit][mode].available=1;
       /*Will be changed WhileReadingTables */
       flex_ingress_modes[unit][mode].reference_count=0;
       flex_ingress_modes[unit][mode].total_counters= 0;
       flex_ingress_modes[unit][mode].ing_attr.packet_attr_type =
            bcmStatFlexPacketAttrTypeCompressed;
       for (index=0;index<8;index++) {
            if(selector_x_en_field_value[index] != 0) {
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.ip_pkt_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.ip_pkt_pos+
                       ing_pkt_attr_cmprsd_bits_g.ip_pkt-1))) { /* 0..0 */
                   flex_ingress_modes[unit][mode].ing_attr.
                    cmprsd_attr_selectors.pkt_attr_bits.ip_pkt = 1;
                   continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.drop_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.drop_pos+
                       ing_pkt_attr_cmprsd_bits_g.drop-1))) { /* 1..1 */
                   flex_ingress_modes[unit][mode].ing_attr.
                    cmprsd_attr_selectors.pkt_attr_bits.drop = 1;
                   continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.svp_type_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.svp_type_pos+
                       ing_pkt_attr_cmprsd_bits_g.svp_type-1))) { /* 1..1 */
                   flex_ingress_modes[unit][mode].ing_attr.
                    cmprsd_attr_selectors.pkt_attr_bits.svp_type = 1;
                   continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.pkt_resolution_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.pkt_resolution_pos+
                       ing_pkt_attr_cmprsd_bits_g.pkt_resolution-1))) { /* 3..8 */
                    flex_ingress_modes[unit][mode].ing_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.pkt_resolution++;
                    continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.tos_dscp_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.tos_dscp_pos+
                       ing_pkt_attr_cmprsd_bits_g.tos_dscp-1))) { /* 9..16 */
                    flex_ingress_modes[unit][mode].ing_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.tos_dscp++;
                    continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.tos_ecn_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.tos_ecn_pos+
                       ing_pkt_attr_cmprsd_bits_g.tos_ecn-1))) { /* 9..16 */
                    flex_ingress_modes[unit][mode].ing_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.tos_ecn++;
                    continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.ing_port_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.ing_port_pos+
                       ing_pkt_attr_cmprsd_bits_g.ing_port-1))) { /* 17..22 */
                    flex_ingress_modes[unit][mode].ing_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.ing_port++;
                    continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.inner_dot1p_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.inner_dot1p_pos+
                       ing_pkt_attr_cmprsd_bits_g.inner_dot1p-1))) { /* 23..25 */
                    flex_ingress_modes[unit][mode].ing_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.inner_dot1p++;
                    continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.outer_dot1p_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.outer_dot1p_pos+
                       ing_pkt_attr_cmprsd_bits_g.outer_dot1p-1))) { /* 26..28 */
                   flex_ingress_modes[unit][mode].ing_attr.
                    cmprsd_attr_selectors.pkt_attr_bits.outer_dot1p++;
                   continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.vlan_format_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.vlan_format_pos+
                       ing_pkt_attr_cmprsd_bits_g.vlan_format-1))) { /* 29..30 */
                    flex_ingress_modes[unit][mode].ing_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.vlan_format++;
                    continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.int_pri_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.int_pri_pos+
                       ing_pkt_attr_cmprsd_bits_g.int_pri-1))) { /* 31..34 */
                    flex_ingress_modes[unit][mode].ing_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.int_pri++;
                    continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.ifp_cng_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.ifp_cng_pos+
                       ing_pkt_attr_cmprsd_bits_g.ifp_cng-1))) { /* 35..36 */
                   flex_ingress_modes[unit][mode].ing_attr.
                    cmprsd_attr_selectors.pkt_attr_bits.ifp_cng++;
                   continue;
               }
               if ((selector_for_bit_x_field_value[index] >= 
                     ing_pkt_attr_cmprsd_bits_g.cng_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (ing_pkt_attr_cmprsd_bits_g.cng_pos+
                       ing_pkt_attr_cmprsd_bits_g.cng-1))) { /* 37..38 */
                   flex_ingress_modes[unit][mode].ing_attr.
                    cmprsd_attr_selectors.pkt_attr_bits.cng++;
                   continue;
               }
           }
       }
       if ((flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
            pkt_attr_bits.cng ) ||
           (flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
            pkt_attr_bits.ifp_cng ) ||
           (flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
            pkt_attr_bits.int_pri ))         {
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "cng:%d \n"),
                       flex_ingress_modes[unit][mode].ing_attr.
                       cmprsd_attr_selectors.pkt_attr_bits.cng));
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "ifp_cng:%d \n"),
                       flex_ingress_modes[unit][mode].ing_attr.
                       cmprsd_attr_selectors.pkt_attr_bits.ifp_cng));
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "int_pri:%d \n"),
                       flex_ingress_modes[unit][mode].ing_attr.
                       cmprsd_attr_selectors.pkt_attr_bits.int_pri));
            alloc_size = soc_mem_index_count(unit,ING_FLEX_CTR_PRI_CNG_MAPm) *
                         sizeof(ing_flex_ctr_pri_cng_map_entry_t);
            ing_pri_cng_map_dma = soc_cm_salloc( unit, alloc_size,
                                                 "ING_FLEX_CTR_PRI_CNG_MAPm");
            if (ing_pri_cng_map_dma == NULL) {
                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "ING_FLEX_CTR_PRI_CNG_MAPm:DMAAllocationFail\n")));
                return;
            }
            if (soc_mem_read_range(
                    unit,
                    ING_FLEX_CTR_PRI_CNG_MAPm,
                    MEM_BLOCK_ANY,
                    soc_mem_index_min(unit,ING_FLEX_CTR_PRI_CNG_MAPm),
                    soc_mem_index_max(unit,ING_FLEX_CTR_PRI_CNG_MAPm),
                    ing_pri_cng_map_dma) != SOC_E_NONE) {
                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "ING_FLEX_CTR_PRI_CNG_MAPm:Read failuer \n")));
                soc_cm_sfree(unit,ing_pri_cng_map_dma);
                return;
            }
            for (index=0;
                 index< soc_mem_index_count(unit,ING_FLEX_CTR_PRI_CNG_MAPm);
                 index++) {
                 soc_mem_field_get( unit, ING_FLEX_CTR_PRI_CNG_MAPm,
                      (uint32 *)&ing_pri_cng_map_dma[index], PRI_CNG_FNf,
                      &map_value);
                 flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
                                   pri_cnf_attr_map[index]=(uint8)map_value;
                 if (map_value) {
                     LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                               (BSL_META_U(unit,
                                           "Index:%dValue:%d\n"),
                                index,map_value));
                 }
            }
            soc_cm_sfree(unit,ing_pri_cng_map_dma);
       }
       if ((flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
            pkt_attr_bits.vlan_format) ||
           (flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
            pkt_attr_bits.outer_dot1p) ||
           (flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
            pkt_attr_bits.inner_dot1p)) {
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "vlan_format:%d \n"),
                       flex_ingress_modes[unit][mode].ing_attr.
                       cmprsd_attr_selectors.pkt_attr_bits.vlan_format));
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "outer_dot1p:%d \n"),
                       flex_ingress_modes[unit][mode].ing_attr.
                       cmprsd_attr_selectors.pkt_attr_bits.outer_dot1p));
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "inner_dot1p:%d \n"),
                       flex_ingress_modes[unit][mode].ing_attr.
                       cmprsd_attr_selectors.pkt_attr_bits.inner_dot1p));
            alloc_size = soc_mem_index_count(unit,ING_FLEX_CTR_PKT_PRI_MAPm) *
                         sizeof(ing_flex_ctr_pkt_pri_map_entry_t);
            ing_pkt_pri_map_dma = soc_cm_salloc( unit, alloc_size,
                                                 "ING_FLEX_CTR_PKT_PRI_MAPm");
            if (ing_pkt_pri_map_dma == NULL) {
                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "ING_FLEX_CTR_PKT_PRI_MAPm:DMAAllocationFail\n")));
                return;
            }
            if (soc_mem_read_range(
                    unit,
                    ING_FLEX_CTR_PKT_PRI_MAPm,
                    MEM_BLOCK_ANY,
                    soc_mem_index_min(unit,ING_FLEX_CTR_PKT_PRI_MAPm),
                    soc_mem_index_max(unit,ING_FLEX_CTR_PKT_PRI_MAPm),
                    ing_pkt_pri_map_dma) != SOC_E_NONE) {
                LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "ING_FLEX_CTR_PKT_PRI_MAPm:Read failuer \n")));
                soc_cm_sfree(unit,ing_pkt_pri_map_dma);
                return;
            }
            for (index=0;
                 index< soc_mem_index_count(unit,ING_FLEX_CTR_PKT_PRI_MAPm);
                 index++) {
                 soc_mem_field_get( unit, ING_FLEX_CTR_PKT_PRI_MAPm,
                      (uint32 *)&ing_pkt_pri_map_dma[index], PKT_PRI_FNf,
                      &map_value);
                 flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
                                   pkt_pri_attr_map[index]=(uint8)map_value;
                 if (map_value){
                     LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                               (BSL_META_U(unit,
                                           "Index:%dValue:%d\n"),
                                index,map_value));
                 }
            }
            soc_cm_sfree(unit,ing_pkt_pri_map_dma);
       }
       if (flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
           pkt_attr_bits.ing_port) {
           LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                     (BSL_META_U(unit,
                                 "ing_port:%d \n"),
                      flex_ingress_modes[unit][mode].
                      ing_attr.cmprsd_attr_selectors.
                      pkt_attr_bits.ing_port));
           alloc_size = soc_mem_index_count(unit,ING_FLEX_CTR_PORT_MAPm) *
                         sizeof(ing_flex_ctr_port_map_entry_t);
           ing_port_map_dma = soc_cm_salloc( unit, alloc_size,
                                             "ING_FLEX_CTR_PORT_MAPm");
           if (ing_port_map_dma == NULL) {
               LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "ING_FLEX_CTR_PORT_MAPm:DMA Allocation failuer\n")));
               return;
           }
           if (soc_mem_read_range(
                   unit,
                   ING_FLEX_CTR_PORT_MAPm,
                   MEM_BLOCK_ANY,
                   soc_mem_index_min(unit,ING_FLEX_CTR_PORT_MAPm),
                   soc_mem_index_max(unit,ING_FLEX_CTR_PORT_MAPm),
                   ing_port_map_dma) != SOC_E_NONE) {
               LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "ING_FLEX_CTR_PORT_MAPm:Read failuer \n")));
               soc_cm_sfree(unit,ing_port_map_dma);
               return;
           }
           for (index=0;
                index< soc_mem_index_count(unit,ING_FLEX_CTR_PORT_MAPm);
                index++) {
                soc_mem_field_get( unit, ING_FLEX_CTR_PORT_MAPm,
                      (uint32 *)&ing_port_map_dma[index], PRI_CNG_FNf,
                      &map_value);
                flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
                                  port_attr_map[index]=(uint8)map_value;
                if (map_value) {
                    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                              (BSL_META_U(unit,
                                          "Index:%dValue:%d\n"),
                               index,map_value));
                }
           }
           soc_cm_sfree(unit,ing_port_map_dma);
       }
       if ((flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
            pkt_attr_bits.tos_dscp ) ||
           (flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
            pkt_attr_bits.tos_ecn)) {
           LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                     (BSL_META_U(unit,
                                 "tos_dscp:%d tos_ecn:%d \n"),
                      flex_ingress_modes[unit][mode].ing_attr.
                      cmprsd_attr_selectors.pkt_attr_bits.tos_dscp,
                      flex_ingress_modes[unit][mode].ing_attr.
                      cmprsd_attr_selectors.pkt_attr_bits.tos_ecn));
           alloc_size = soc_mem_index_count(unit,ING_FLEX_CTR_TOS_MAPm) *
                        sizeof(ing_flex_ctr_tos_map_entry_t);
           ing_tos_map_dma = soc_cm_salloc( unit, alloc_size,
                                            "ING_FLEX_CTR_TOS_MAPm");
           if (ing_tos_map_dma == NULL) {
               LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "ING_FLEX_CTR_TOS_MAP:DMA Allocation failuer \n")));
               return;
           }
           if(soc_mem_read_range(
                  unit,
                  ING_FLEX_CTR_TOS_MAPm,
                  MEM_BLOCK_ANY,
                  soc_mem_index_min(unit,ING_FLEX_CTR_TOS_MAPm),
                  soc_mem_index_max(unit,ING_FLEX_CTR_TOS_MAPm),
                  ing_tos_map_dma) != SOC_E_NONE) {
              LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                        (BSL_META_U(unit,
                                    "ING_FLEX_CTR_TOS_MAPm:Read failuer \n")));
              soc_cm_sfree(unit,ing_tos_map_dma);
              return;
           }
           for (index=0;
                index< soc_mem_index_count(unit,ING_FLEX_CTR_TOS_MAPm);
                index++) {
                soc_mem_field_get( unit, ING_FLEX_CTR_TOS_MAPm,
                      (uint32 *)&ing_tos_map_dma[index], TOS_FNf,
                      &map_value);
                flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
                                   tos_attr_map[index]=(uint8)map_value;
                if (map_value) {
                   LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                             (BSL_META_U(unit,
                                         "Index:%dValue:%d\n"),
                              index,map_value));
                }
            }
            soc_cm_sfree(unit,ing_tos_map_dma);
       }
       if ((flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
             pkt_attr_bits.pkt_resolution) ||
           (flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
             pkt_attr_bits.svp_type) ||
           (flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
             pkt_attr_bits.drop)) {
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "pkt_resolution:%d \n"),
                       flex_ingress_modes[unit][mode].ing_attr.
                       cmprsd_attr_selectors.pkt_attr_bits.pkt_resolution));
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "svp_type:%d \n"),
                       flex_ingress_modes[unit][mode].ing_attr.
                       cmprsd_attr_selectors.pkt_attr_bits.svp_type));
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "drop:%d \n"),
                       flex_ingress_modes[unit][mode].ing_attr.
                       cmprsd_attr_selectors.pkt_attr_bits.drop));
            alloc_size = soc_mem_index_count(unit,ING_FLEX_CTR_PKT_RES_MAPm) *
                         sizeof(ing_flex_ctr_pkt_res_map_entry_t);
            ing_pkt_res_map_dma = soc_cm_salloc( unit, alloc_size,
                                                 "ING_FLEX_CTR_PKT_RES_MAPm");
            if (ing_pkt_res_map_dma == NULL) {
                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "ING_FLEX_CTR_PKT_RES_MAPm:DMA AllocationFail\n")));
                return;
            }
            if(soc_mem_read_range(
                   unit,
                   ING_FLEX_CTR_PKT_RES_MAPm,
                   MEM_BLOCK_ANY,
                   soc_mem_index_min(unit,ING_FLEX_CTR_PKT_RES_MAPm),
                   soc_mem_index_max(unit,ING_FLEX_CTR_PKT_RES_MAPm),
                   ing_pkt_res_map_dma) != SOC_E_NONE) {
               LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "ING_FLEX_CTR_PKT_RES_MAPm:Read failuer \n")));
               soc_cm_sfree(unit,ing_pkt_res_map_dma);
               return;
            }
            for(index=0;
                index< soc_mem_index_count(unit,ING_FLEX_CTR_PKT_RES_MAPm);
                index++) {
                soc_mem_field_get( unit, ING_FLEX_CTR_PKT_RES_MAPm,
                     (uint32 *)&ing_pkt_res_map_dma[index], PKT_RES_FNf,
                     &map_value);
                flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
                                   pkt_res_attr_map[index]=(uint8)map_value;
                if (map_value) {
                    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                              (BSL_META_U(unit,
                                          "Index:%dValue:%d\n"),
                               index,map_value));
                }
            }
            soc_cm_sfree(unit,ing_pkt_res_map_dma);
       }
       if (flex_ingress_modes[unit][mode].ing_attr.cmprsd_attr_selectors.
           pkt_attr_bits.ip_pkt) {
           LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                     (BSL_META_U(unit,
                                 "ip_pkt:%d \n"),
                      flex_ingress_modes[unit][mode].ing_attr.
                      cmprsd_attr_selectors.pkt_attr_bits.ip_pkt));
       }
       return;
    }
    if (direction == bcmStatFlexDirectionEgress) {
        flex_egress_modes[unit][mode].available=1;
        /*Will be changed WhileReadingTables */
        flex_egress_modes[unit][mode].reference_count=0;
        flex_egress_modes[unit][mode].egr_attr.packet_attr_type =
                                      bcmStatFlexPacketAttrTypeCompressed;
        for (index=0;index<8;index++) {
             if(selector_x_en_field_value[index] != 0) {
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.ip_pkt_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.ip_pkt_pos+
                       egr_pkt_attr_cmprsd_bits_g.ip_pkt-1))) { /* 0..0 */
                    flex_egress_modes[unit][mode].egr_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.ip_pkt = 1;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.drop_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.drop_pos+
                       egr_pkt_attr_cmprsd_bits_g.drop-1))) { /* 1..1 */
                    flex_egress_modes[unit][mode].egr_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.drop = 1;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.dvp_type_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.dvp_type_pos+
                       egr_pkt_attr_cmprsd_bits_g.dvp_type-1))) { /* 2..2 */
                    flex_egress_modes[unit][mode].egr_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.dvp_type = 1;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.svp_type_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.svp_type_pos+
                       egr_pkt_attr_cmprsd_bits_g.svp_type-1))) { /* 3..3 */
                    flex_egress_modes[unit][mode].egr_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.svp_type = 1;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.pkt_resolution_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.pkt_resolution_pos+
                       egr_pkt_attr_cmprsd_bits_g.pkt_resolution-1))) { /* 4..4 */
                    flex_egress_modes[unit][mode].egr_attr.
                     cmprsd_attr_selectors.
                    pkt_attr_bits.pkt_resolution = 1;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.tos_dscp_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.tos_dscp_pos+
                       egr_pkt_attr_cmprsd_bits_g.tos_dscp-1))) { /* 5..12 */
                    flex_egress_modes[unit][mode].egr_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.tos_dscp++;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.tos_ecn_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.tos_ecn_pos+
                       egr_pkt_attr_cmprsd_bits_g.tos_ecn-1))) { /* 5..12 */
                    flex_egress_modes[unit][mode].egr_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.tos_ecn++;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.egr_port_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.egr_port_pos+
                       egr_pkt_attr_cmprsd_bits_g.egr_port-1))) { /* 13..18 */
                    flex_egress_modes[unit][mode].egr_attr.
                     cmprsd_attr_selectors.pkt_attr_bits.egr_port++;
                    continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.inner_dot1p_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.inner_dot1p_pos+
                       egr_pkt_attr_cmprsd_bits_g.inner_dot1p-1))) { /* 19..21 */
                     flex_egress_modes[unit][mode].egr_attr.
                      cmprsd_attr_selectors.pkt_attr_bits.inner_dot1p++;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.outer_dot1p_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.outer_dot1p_pos+
                       egr_pkt_attr_cmprsd_bits_g.outer_dot1p-1))) { /* 22..24 */
                     flex_egress_modes[unit][mode].egr_attr.
                      cmprsd_attr_selectors.pkt_attr_bits.outer_dot1p++;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.vlan_format_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.vlan_format_pos+
                       egr_pkt_attr_cmprsd_bits_g.vlan_format-1))) { /* 25..26 */
                     flex_egress_modes[unit][mode].egr_attr.
                      cmprsd_attr_selectors.pkt_attr_bits.vlan_format++;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.int_pri_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.int_pri_pos+
                       egr_pkt_attr_cmprsd_bits_g.int_pri-1))) { /* 27..30 */
                     flex_egress_modes[unit][mode].egr_attr.
                      cmprsd_attr_selectors.pkt_attr_bits.int_pri++;
                     continue;
                }
                if ((selector_for_bit_x_field_value[index] >= 
                     egr_pkt_attr_cmprsd_bits_g.cng_pos) &&
                    (selector_for_bit_x_field_value[index] <= 
                      (egr_pkt_attr_cmprsd_bits_g.cng_pos+
                       egr_pkt_attr_cmprsd_bits_g.cng-1))) { /* 31..32 */
                     flex_egress_modes[unit][mode].egr_attr.
                      cmprsd_attr_selectors.pkt_attr_bits.cng++;
                     continue;
                }
            }
        }
        if ((flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
              pkt_attr_bits.cng) ||
            (flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
              pkt_attr_bits.int_pri) ) {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "cng:%d \n"),
                        flex_egress_modes[unit][mode].egr_attr.
                        cmprsd_attr_selectors.pkt_attr_bits.cng));
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "int_pri:%d \n"),
                        flex_egress_modes[unit][mode]. 
                        egr_attr.cmprsd_attr_selectors.pkt_attr_bits.
                        int_pri));
             alloc_size = soc_mem_index_count(unit,EGR_FLEX_CTR_PRI_CNG_MAPm) *
                          sizeof(egr_flex_ctr_pri_cng_map_entry_t);
             egr_pri_cng_map_dma = soc_cm_salloc( unit, alloc_size,
                                                  "EGR_FLEX_CTR_PRI_CNG_MAPm");
             if (egr_pri_cng_map_dma == NULL) {
                 LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "EGR_FLEX_CTR_PRI_CNG_MAPm:DMAAllocationFail\n")));
                 return;
             }

             if (soc_mem_read_range(
                     unit,
                     EGR_FLEX_CTR_PRI_CNG_MAPm,
                     MEM_BLOCK_ANY,
                     soc_mem_index_min(unit,EGR_FLEX_CTR_PRI_CNG_MAPm),
                     soc_mem_index_max(unit,EGR_FLEX_CTR_PRI_CNG_MAPm),
                     egr_pri_cng_map_dma) != SOC_E_NONE) {
                 LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "EGR_FLEX_CTR_PRI_CNG_MAPm:Read failuer \n")));
                 soc_cm_sfree(unit,egr_pri_cng_map_dma);
                 return;
             }
             for (index=0;
                  index< soc_mem_index_count(unit,EGR_FLEX_CTR_PRI_CNG_MAPm);
                  index++) {
                  soc_mem_field_get( unit, EGR_FLEX_CTR_PRI_CNG_MAPm,
                       (uint32 *)&egr_pri_cng_map_dma[index], PRI_CNG_FNf,
                       &map_value);
                  flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
                                   pri_cnf_attr_map[index]=(uint8)map_value;
                  if (map_value) {
                      LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                                (BSL_META_U(unit,
                                            "Index:%dValue:%d\n"),
                                 index,map_value));
                  }
             }
             soc_cm_sfree(unit,egr_pri_cng_map_dma);
        }
        if ((flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
             pkt_attr_bits.vlan_format) ||
            (flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
             pkt_attr_bits.outer_dot1p) ||
            (flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
             pkt_attr_bits.inner_dot1p)) {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "vlan_format:%d \n"),
                        flex_egress_modes[unit][mode].egr_attr.
                        cmprsd_attr_selectors.pkt_attr_bits.vlan_format));
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "outer_dot1p:%d \n"),
                        flex_egress_modes[unit][mode].egr_attr.
                        cmprsd_attr_selectors.pkt_attr_bits.outer_dot1p));
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "inner_dot1p:%d \n"),
                        flex_egress_modes[unit][mode].egr_attr.
                        cmprsd_attr_selectors.pkt_attr_bits.inner_dot1p));
             alloc_size = soc_mem_index_count(unit,EGR_FLEX_CTR_PKT_PRI_MAPm) *
                          sizeof(egr_flex_ctr_pkt_pri_map_entry_t);
             egr_pkt_pri_map_dma = soc_cm_salloc( unit, alloc_size,
                                                 "EGR_FLEX_CTR_PKT_PRI_MAPm");
             if (egr_pkt_pri_map_dma == NULL) {
                 LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "EGR_FLEX_CTR_PKT_PRI_MAPm:DMAAllocationFail\n")));
                 return;
             }
             if (soc_mem_read_range(
                     unit,
                     EGR_FLEX_CTR_PKT_PRI_MAPm,
                     MEM_BLOCK_ANY,
                     soc_mem_index_min(unit,EGR_FLEX_CTR_PKT_PRI_MAPm),
                     soc_mem_index_max(unit,EGR_FLEX_CTR_PKT_PRI_MAPm),
                     egr_pkt_pri_map_dma) != SOC_E_NONE) {
                 LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "EGR_FLEX_CTR_PKT_PRI_MAPm:Read failuer \n")));
                 soc_cm_sfree(unit,egr_pkt_pri_map_dma);
                 return;
             }
             for (index=0;
                  index< soc_mem_index_count(unit,EGR_FLEX_CTR_PKT_PRI_MAPm);
                  index++) {
                  soc_mem_field_get( unit, EGR_FLEX_CTR_PKT_PRI_MAPm,
                       (uint32 *)&egr_pkt_pri_map_dma[index], PKT_PRI_FNf,
                       &map_value);
                  flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
                                   pkt_pri_attr_map[index]=(uint8)map_value;
                  if (map_value) {
                      LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                                (BSL_META_U(unit,
                                            "Index:%dValue:%d\n"),
                                 index,map_value));
                  }
             }
             soc_cm_sfree(unit,egr_pkt_pri_map_dma);
        }
        if (flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
            pkt_attr_bits.egr_port) {
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "egr_port:%d \n"),
                       flex_egress_modes[unit][mode].
                       egr_attr.cmprsd_attr_selectors.
                       pkt_attr_bits.egr_port));
            alloc_size = soc_mem_index_count(unit,EGR_FLEX_CTR_PORT_MAPm) *
                         sizeof(egr_flex_ctr_port_map_entry_t);
            egr_port_map_dma = soc_cm_salloc( unit, alloc_size,
                                              "EGR_FLEX_CTR_PORT_MAPm");
            if (egr_port_map_dma == NULL) {
                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "EGR_FLEX_CTR_PORT_MAPm:DMA AllocationFail\n")));
                return;
            }
            if (soc_mem_read_range(
                    unit,
                    EGR_FLEX_CTR_PORT_MAPm,
                    MEM_BLOCK_ANY,
                    soc_mem_index_min(unit,EGR_FLEX_CTR_PORT_MAPm),
                    soc_mem_index_max(unit,EGR_FLEX_CTR_PORT_MAPm),
                    egr_port_map_dma) != SOC_E_NONE) {
                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "EGR_FLEX_CTR_PKT_PRI_MAPm:Read failuer \n")));
                soc_cm_sfree(unit,egr_port_map_dma);
                return;
            }
            for (index=0;
                 index< soc_mem_index_count(unit,EGR_FLEX_CTR_PORT_MAPm);
                 index++) {
                 soc_mem_field_get( unit, EGR_FLEX_CTR_PORT_MAPm,
                      (uint32 *)&egr_port_map_dma[index], PORT_FNf,
                      &map_value);
                 flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
                                  port_attr_map[index] = (uint8)map_value;
                 if (map_value) {
                     LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                               (BSL_META_U(unit,
                                           "Index:%dValue:%d\n"),
                                index,map_value));
                 }
            }
            soc_cm_sfree(unit,egr_port_map_dma);
        }
        if ((flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
             pkt_attr_bits.tos_dscp)  ||
            (flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
             pkt_attr_bits.tos_ecn)) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "tos_dscp:%d tos_ecn:%d \n"),
                       flex_egress_modes[unit][mode].egr_attr.
                       cmprsd_attr_selectors.pkt_attr_bits.tos_dscp,
                       flex_egress_modes[unit][mode].egr_attr.
                       cmprsd_attr_selectors.pkt_attr_bits.tos_ecn));
            alloc_size = soc_mem_index_count(unit,EGR_FLEX_CTR_TOS_MAPm) *
                         sizeof(egr_flex_ctr_tos_map_entry_t);
            egr_tos_map_dma = soc_cm_salloc( unit, alloc_size,
                                             "EGR_FLEX_CTR_TOS_MAPm");
            if (egr_tos_map_dma == NULL) {
                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "EGR_FLEX_CTR_TOS_MAPm:DMA Allocation failuer\n")));
                return;
            }
            if(soc_mem_read_range(
                   unit,
                   EGR_FLEX_CTR_TOS_MAPm,
                   MEM_BLOCK_ANY,
                   soc_mem_index_min(unit,EGR_FLEX_CTR_TOS_MAPm),
                   soc_mem_index_max(unit,EGR_FLEX_CTR_TOS_MAPm),
                   egr_tos_map_dma) != SOC_E_NONE) {
               LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "EGR_FLEX_CTR_TOS_MAPm:Read failuer \n")));
               soc_cm_sfree(unit,egr_tos_map_dma);
               return;
            }
            for (index=0;
                 index< soc_mem_index_count(unit,EGR_FLEX_CTR_TOS_MAPm);
                 index++) {
                 soc_mem_field_get( unit, EGR_FLEX_CTR_TOS_MAPm,
                      (uint32 *)&egr_tos_map_dma[index], TOS_FNf, &map_value);
                 flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
                                  tos_attr_map[index] = (uint8)map_value;
                 if (map_value) {
                     LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                               (BSL_META_U(unit,
                                           "Index:%dValue:%d\n"),
                                index,map_value));
                 }
            }
            soc_cm_sfree(unit,egr_tos_map_dma);
        }
        if ((flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
             pkt_attr_bits.pkt_resolution) ||
            (flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
             pkt_attr_bits.svp_type) ||
            (flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
             pkt_attr_bits.dvp_type) ||
            (flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
             pkt_attr_bits.drop) ) {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "pkt_resolution:%d \n"),
                        flex_egress_modes[unit][mode].egr_attr.
                        cmprsd_attr_selectors.pkt_attr_bits.
                        pkt_resolution));
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "svp_type:%d \n"),
                        flex_egress_modes[unit][mode].egr_attr.
                        cmprsd_attr_selectors.pkt_attr_bits.svp_type));
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "dvp_type:%d \n"),
                        flex_egress_modes[unit][mode].egr_attr.
                        cmprsd_attr_selectors.pkt_attr_bits.dvp_type));
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "drop:%d \n"),
                        flex_egress_modes[unit][mode].egr_attr.
                        cmprsd_attr_selectors.pkt_attr_bits.drop));
             alloc_size = soc_mem_index_count(unit,EGR_FLEX_CTR_PKT_RES_MAPm) *
                          sizeof(egr_flex_ctr_pkt_res_map_entry_t);
             egr_pkt_res_map_dma = soc_cm_salloc( unit, alloc_size,
                                                 "EGR_FLEX_CTR_PKT_RES_MAPm");
             if (egr_pkt_res_map_dma == NULL) {
                 LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "EGR_FLEX_CTR_PKT_RES_MAPm:DMAAllocationFail\n")));
                 return;
             }
             if (soc_mem_read_range(
                     unit,
                     EGR_FLEX_CTR_PKT_RES_MAPm,
                     MEM_BLOCK_ANY,
                     soc_mem_index_min(unit,EGR_FLEX_CTR_PKT_RES_MAPm),
                     soc_mem_index_max(unit,EGR_FLEX_CTR_PKT_RES_MAPm),
                     egr_pkt_res_map_dma) != BCM_E_NONE) {
                 LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "EGR_FLEX_CTR_TOS_MAPm:Read failuer \n")));
                 soc_cm_sfree(unit,egr_pkt_res_map_dma);
                 return;
             }
             for (index=0;
                  index< soc_mem_index_count(unit,EGR_FLEX_CTR_PKT_RES_MAPm);
                  index++) {
                  soc_mem_field_get( unit, EGR_FLEX_CTR_PKT_RES_MAPm,
                      (uint32 *)&egr_pkt_res_map_dma[index], PKT_RES_FNf,
                      &map_value);
                  flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
                                   pkt_res_attr_map[index]= (uint8)map_value;

                  if (map_value) {
                      LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                                (BSL_META_U(unit,
                                            "Index:%dValue:%d\n"),
                                 index,map_value));
                  }
             }
             soc_cm_sfree(unit,egr_pkt_res_map_dma);
        }
        if (flex_egress_modes[unit][mode].egr_attr.cmprsd_attr_selectors.
            pkt_attr_bits.ip_pkt ) {
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "ip_pkt:%d \n"),
                       flex_egress_modes[unit][mode].
                       egr_attr.cmprsd_attr_selectors.
                       pkt_attr_bits.ip_pkt));
        }
        return;
    }
    LOG_ERROR(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Ooops. Control Must not reach over here \n")));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_init_udf_mode
 * Description:
 *      Configures flex hw for udf mode
 * Parameters:
 *      unit                           - (IN) unit number
 *      mode                           - (IN) Flex Offset Mode
 *      direction                      - (IN) Flex Data flow direction(ing/egr)
 *      selector_count                 - (IN) Flex Key selector count
 *      selector_x_en_field_value      - (IN) Flex Key selector fields
 *      selector_for_bit_x_field_value - (IN) Flex Key selector field bits
 *
 * Return Value:
 *      None
 * Notes:
 *      Currently not being used!
 *
 */
static void 
_bcm_esw_stat_flex_init_udf_mode(
    int                       unit,
    int                       mode,
    bcm_stat_flex_direction_t direction,
    uint32                    selector_count,
    uint32                    selector_x_en_field_value[8],
    uint32                    selector_for_bit_x_field_value[8])
{
    uint16  udf0=0;
    uint16  udf1=0;
    uint32  index=0;
    for (udf0=0,udf1=0,index=0;index<8;index++) {
         if (selector_for_bit_x_field_value[index] != 0) {
             if (selector_for_bit_x_field_value[index] <=16) {
                 udf0 |= (1 << (selector_for_bit_x_field_value[index]-1));
             } else {
                 udf1 |= (1 << (selector_for_bit_x_field_value[index]- 16 - 1));
             }
         }
    }
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "UDF0 : 0x%04x UDF1 0x%04x \n"),
               udf0,udf1));
    if (direction == bcmStatFlexDirectionIngress) {
        flex_ingress_modes[unit][mode].available=1;
        /*Will be changed WhileReadingTables */
        flex_ingress_modes[unit][mode].reference_count=0;
        flex_ingress_modes[unit][mode].total_counters= ( 1 << selector_count);
        flex_ingress_modes[unit][mode].ing_attr.packet_attr_type =
             bcmStatFlexPacketAttrTypeUdf;
        flex_ingress_modes[unit][mode].ing_attr.udf_pkt_attr_selectors.
             udf_pkt_attr_bits.udf0 = udf0;
        flex_ingress_modes[unit][mode].ing_attr.udf_pkt_attr_selectors.
             udf_pkt_attr_bits.udf1 = udf1;
        return;
    }
    if (direction == bcmStatFlexDirectionEgress) {
        flex_egress_modes[unit][mode].available=1;
        /*Will be changed WhileReadingTables */
        flex_egress_modes[unit][mode].reference_count=0;
        flex_egress_modes[unit][mode].total_counters= ( 1 << selector_count);
        flex_egress_modes[unit][mode].egr_attr.packet_attr_type =
             bcmStatFlexPacketAttrTypeUdf;
        flex_egress_modes[unit][mode].egr_attr.udf_pkt_attr_selectors.
             udf_pkt_attr_bits.udf0 = udf0;
        flex_egress_modes[unit][mode].egr_attr.udf_pkt_attr_selectors.
             udf_pkt_attr_bits.udf1 = udf1;
        return;
    }
    LOG_ERROR(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Ooops. Control Must not reach over here \n")));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_egress_l3_next_hop_table
 * Description:
 *      Checks egress_l3_next_hop table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void 
_bcm_esw_stat_flex_check_egress_l3_next_hop_table(int unit)
{
    uint32    index=0;
    /*egr_l3_next_hop_entry_t  egr_l3_next_hop_entry_v; */
    do {
        _bcm_esw_stat_flex_check_egress_table(unit,EGR_L3_NEXT_HOPm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while (index < soc_mem_index_count(unit,EGR_L3_NEXT_HOPm));

    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked EGRESS:EGR_L3_NEXT_HOP %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_egress_vlan_table
 * Description:
 *      Checks egress_vlan table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 *
 */
static void 
_bcm_esw_stat_flex_check_egress_vlan_table(int unit)
{
    uint32    index=0;
    /*egr_vlan_entry_t  egr_vlan_entry_v;*/
    do {
        _bcm_esw_stat_flex_check_egress_table(unit,EGR_VLANm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while (index< soc_mem_index_count(unit,EGR_VLANm));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked EGRESS:EGR_VLAN %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_ingress_vlan_table
 * Description:
 *      Checks ingress vlan table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 *
 */
static void 
_bcm_esw_stat_flex_check_ingress_vlan_table(int unit)
{
    uint32    index=0;
    /*vlan_tab_entry_t  vlan_tab_entry_v;*/
    do {
        _bcm_esw_stat_flex_check_ingress_table(unit,VLAN_TABm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while(index<soc_mem_index_count(unit,VLAN_TABm));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked INGRESS:VLAN_TAB %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_ingress_vrf_table
 * Description:
 *      Checks ingress vrf table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void 
_bcm_esw_stat_flex_check_ingress_vrf_table(int unit)
{
    uint32    index=0;
    /*vrf_entry_t  vrf_entry_v;*/
    do {
        _bcm_esw_stat_flex_check_ingress_table(unit,VRFm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while (index<soc_mem_index_count(unit,VRFm));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked INGRESS:VRF %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_egress_vfi_table
 * Description:
 *      Checks egress vfi table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void
_bcm_esw_stat_flex_check_egress_vfi_table(int unit)
{
    uint32    index=0;
    /*egr_vfi_entry_t  egr_vfi_entry_v;*/
    do { 
        _bcm_esw_stat_flex_check_egress_table(unit,EGR_VFIm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while (index<soc_mem_index_count(unit,EGR_VFIm));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked EGRESS:EGR_VFI %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_ingress_vfi_table
 * Description:
 *      Checks ingress vfi table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void 
 _bcm_esw_stat_flex_check_ingress_vfi_table(int unit)
{
    uint32    index=0;
    /*vfi_entry_t  vfi_entry_v;*/
    do {
        _bcm_esw_stat_flex_check_ingress_table(unit,VFIm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while (index<soc_mem_index_count(unit,VFIm));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked INGRESS:VFI %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_ingress_l3_iif_table
 * Description:
 *      Checks ingress l3_iif table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void 
_bcm_esw_stat_flex_check_ingress_l3_iif_table(int unit)
{
    uint32    index=0;
    /*iif_entry_t  iif_entry_v;*/
    do {
        _bcm_esw_stat_flex_check_ingress_table(unit,L3_IIFm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while (index<soc_mem_index_count(unit,L3_IIFm));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked INGRESS:L3_IIF %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_ingress_source_vp_table
 * Description:
 *      Checks ingress source_vp table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void 
_bcm_esw_stat_flex_check_ingress_source_vp_table(int unit)
{
    uint32    index=0;
    /*source_vp_entry_t  source_vp_entry_v;*/
    do {
        _bcm_esw_stat_flex_check_ingress_table(unit,SOURCE_VPm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while (index<soc_mem_index_count(unit,SOURCE_VPm));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked INGRESS:SOURCE_VP %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_ingress_mpls_entry_table
 * Description:
 *      Checks ingress mpls_entry table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void
_bcm_esw_stat_flex_check_ingress_mpls_entry_table(int unit)
{
    uint32    index=0;
    /*mpls_entry_entry_t  mpls_entry_entry_v;*/
    if (!SOC_IS_TRIUMPH3(unit)) {
        do {
            _bcm_esw_stat_flex_check_ingress_table(unit,MPLS_ENTRYm,
                 index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
            index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
        } while (index<soc_mem_index_count(unit,MPLS_ENTRYm));
    }
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        index = 0;
        do {
            _bcm_esw_stat_flex_check_ingress_table(unit,MPLS_ENTRY_EXTDm,
                 index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
            index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
        } while (index<soc_mem_index_count(unit,MPLS_ENTRY_EXTDm));
    }
#endif
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked INGRESS:MPLS_ENTRY %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_ingress_vfp_policy_table
 * Description:
 *      Checks ingress vfp_policy table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void
_bcm_esw_stat_flex_check_ingress_vfp_policy_table(int unit)
{
    uint32    index=0;
    /*vfp_policy_table_entry_t  vfp_policy_table_entry_v;*/
    do {
        _bcm_esw_stat_flex_check_ingress_table(unit,VFP_POLICY_TABLEm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while (index<soc_mem_index_count(unit,VFP_POLICY_TABLEm));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked INGRESS:VFP_POLICY_TABLE %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_egress_vlan_xlate_table
 * Description:
 *      Checks egress vlan_xlate table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void 
_bcm_esw_stat_flex_check_egress_vlan_xlate_table(int unit)
{
    uint32    index=0;
    /*egr_vlan_xlate_entry_t  egr_vlan_xlate_entry_v;*/
    do {
        _bcm_esw_stat_flex_check_egress_table(unit,EGR_VLAN_XLATEm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while (index<soc_mem_index_count(unit,EGR_VLAN_XLATEm));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked EGRESS:EGR_VLAN_XLATE %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_ingress_vlan_xlate_table
 * Description:
 *      Checks ingress vlan_xlate table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void
_bcm_esw_stat_flex_check_ingress_vlan_xlate_table(int unit)
{
    uint32    index=0;
    /*vlan_xlate_entry_t  vlan_xlate_entry_v;*/
    if (!SOC_IS_TRIUMPH3(unit)) {
        do {
            _bcm_esw_stat_flex_check_ingress_table(unit,VLAN_XLATEm,
                 index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
            index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
        } while (index<soc_mem_index_count(unit,VLAN_XLATEm));
    }
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        index= 0;
        do {
            _bcm_esw_stat_flex_check_ingress_table(unit,VLAN_XLATE_EXTDm,
                 index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
            index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
        } while (index<soc_mem_index_count(unit,VLAN_XLATE_EXTDm));
    }
#endif
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked INGRESS:VLAN_XLATE_TABLE %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_egress_port_table
 * Description:
 *      Checks egress port table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void
_bcm_esw_stat_flex_check_egress_port_table(int unit)
{
    uint32    index=0;
    /*egr_port_entry_t  egr_port_entry_v;*/
    do {
        _bcm_esw_stat_flex_check_egress_table(unit,EGR_PORTm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while (index<soc_mem_index_count(unit,EGR_PORTm));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked EGRESS:EGR_PORT %d entries..\n"),
               index-1));
}
/*
 * Function:
 *      _bcm_esw_stat_flex_check_ingress_port_table
 * Description:
 *      Checks ingress port table for flex configuration
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      Applicable in warm boot scenario only!
 */
static void
_bcm_esw_stat_flex_check_ingress_port_table(int unit)
{
    uint32    index=0;
    /*port_tab_entry_t  port_tab_entry_v;*/
    do {
        _bcm_esw_stat_flex_check_ingress_table(unit,PORT_TABm,
             index,index + BCM_STAT_FLEX_WARMBOOT_READ_CHUNK - 1);
        index += BCM_STAT_FLEX_WARMBOOT_READ_CHUNK;
    } while(index<soc_mem_index_count(unit,PORT_TABm));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Checked INGRESS:PORT_TABLE %d entries..\n"),
               index-1));
}

/* Helper function for _bcm_esw_stat_flex_set */
STATIC int 
_bcm_esw_stat_flex_set_mem_update(int unit, soc_mem_t table, uint32 index,
                                  soc_mem_t mem, uint32 start, uint32 entry_cnt,
                                  void *entry, int dir, uint32 pool_id, int byte_flag
                                  )
{
    int i;
    uint32 zero=0;
    uint32 one=1;
    uint32 max_packet_mask = 0;
    uint64 max_byte_mask;
    uint32 hw_val[2];
    bcm_stat_flex_counter_value_t flex_values  = {0};
#if defined(BCM_TRIDENT2_SUPPORT) 
    soc_mem_t mem_dual = 0;
#endif

    COMPILER_64_ZERO(max_byte_mask);
    COMPILER_64_SET(max_byte_mask, zero, one);
    COMPILER_64_SHL(max_byte_mask,soc_mem_field_length(unit, mem, BYTE_COUNTERf));
    COMPILER_64_SUB_32(max_byte_mask, one);

    max_packet_mask = (1 << soc_mem_field_length(unit, mem, PACKET_COUNTERf));
    max_packet_mask -= 1;

#define FLEX_SET_BYTE_COUNTER_UPDATE \
    do { \
        /* Mask to possible max values */ \
        COMPILER_64_AND(flex_values.byte_counter_value, max_byte_mask); \
        /* Update Soft Copy */ \
        COMPILER_64_SET(flex_byte_counter[unit][dir][pool_id][start+i], \
                        COMPILER_64_HI(flex_values.byte_counter_value), \
                        COMPILER_64_LO(flex_values.byte_counter_value)); \
        /* Change Read Hw Copy */ \
        hw_val[0] = COMPILER_64_HI(flex_values.byte_counter_value); \
        hw_val[1] = COMPILER_64_LO(flex_values.byte_counter_value); \
        soc_mem_field_set(unit, mem, (uint32 *)&entry_temp[i], \
                          BYTE_COUNTERf, hw_val); \
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR, \
                  (BSL_META_U(unit, \
                              "Byte Count Value\t:TABLE:%sINDEX:%d COUTER-%d" \
                               "(@Pool:%dDirection:%dActualOffset%d)" \
                               " : %x:%x \n"),SOC_MEM_UFNAME(unit, table), \
                   index, i, pool_id, dir, start + i, \
                   COMPILER_64_HI(flex_values.byte_counter_value), \
                   COMPILER_64_LO(flex_values.byte_counter_value))); \
    } while (0)

#define FLEX_SET_BYTE_COUNTER_UPDATE_TD2 \
    if (SOC_IS_TD2_TT2(unit)) { \
        int j; \
        for (j = 0; j < _BCM_FLEX_STAT_BUFFS; j++) { \
            COMPILER_64_SET(flex_byte_counter_x[unit][dir][pool_id][j][start+i], \
                            COMPILER_64_HI(flex_values.byte_counter_value), \
                            COMPILER_64_LO(flex_values.byte_counter_value)); \
            COMPILER_64_ZERO(flex_byte_counter_y[unit][dir][pool_id][j][start+i]); \
        } \
    }

#define FLEX_SET_PACKET_COUNTER_UPDATE \
    do { \
        flex_values.pkt_counter_value &= max_packet_mask; \
        /* Update Soft 64-bits Copy */ \
        COMPILER_64_SET(flex_packet64_counter[unit][dir][pool_id][start+i], \
                    0, flex_values.pkt_counter_value); \
        /* Update Soft Copy */ \
        flex_packet_counter[unit][dir][pool_id][start+i] = \
                    flex_values.pkt_counter_value; \
        /* Change Read Hw Copy */ \
        soc_mem_field_set(unit, mem, (uint32 *)&entry_temp[i], \
                          PACKET_COUNTERf, &flex_values.pkt_counter_value); \
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR, \
                  (BSL_META_U(unit, \
                              "Packet Count Value\t:TABLE:%sINDEX:%d" \
                               "COUTER-%d (@Pool:%dDirection:%d" \
                               "ActualOffset%d) : %x \n"), SOC_MEM_UFNAME(unit, table), \
                   index, i, pool_id, dir, start + i, \
                   flex_values.pkt_counter_value)); \
    } while (0)

#define FLEX_SET_PACKET_COUNTER_UPDATE_TD2 \
    if (SOC_IS_TD2_TT2(unit)) { \
        int j; \
        for (j = 0; j < _BCM_FLEX_STAT_BUFFS; j++) { \
            flex_pkt_counter_x[unit][dir][pool_id][j][start+i] \
                = flex_values.pkt_counter_value; \
            flex_pkt_counter_y[unit][dir][pool_id][j][start+i] = 0; \
        } \
    }

    /* Type of entry is ing_flex_ctr_counter_table_0_entry_t, and entry also
     * could be used for egress flex counter whose type should be
     * egr_flex_ctr_counter_table_0_entry_t, but since their entry size is the
     * same, so we don't need to distinguish them */

    /* If entry_cnt less than 8, memory range operation is not efficient */
    if (entry_cnt <= 8) {
        for (i = 0 ; i < entry_cnt ; i++) {
            ing_flex_ctr_counter_table_0_entry_t *entry_temp = entry;
            if (byte_flag != -1) {
                BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY,
                                                 start + i, &entry_temp[i]));
            }

            /* Bytes Counter update */
            if (byte_flag == 1 || byte_flag == -1) {
                FLEX_SET_BYTE_COUNTER_UPDATE;
#if defined(BCM_TRIDENT2_SUPPORT)
                FLEX_SET_BYTE_COUNTER_UPDATE_TD2;
#endif 
            }

            /* Packet Counter update */
            if (byte_flag == 0 || byte_flag == -1) {
                FLEX_SET_PACKET_COUNTER_UPDATE;
#if defined(BCM_TRIDENT2_SUPPORT) 
                FLEX_SET_PACKET_COUNTER_UPDATE_TD2;
#endif 
            }

            /* TD2 counter value is set in X memory, clear Y memory */
#if defined(BCM_TRIDENT2_SUPPORT)
            if (SOC_IS_TD2_TT2(unit)) { 
                mem = _ctr_counter_table_x[dir][pool_id];
            }
#endif
            BCM_IF_ERROR_RETURN(soc_mem_write(unit, mem, MEM_BLOCK_ANY,
                                              start + i, &entry_temp[i]));
#if defined(BCM_TRIDENT2_SUPPORT)
            if (SOC_IS_TD2_TT2(unit)) { 
                mem_dual =_ctr_counter_table_y[dir][pool_id];
                sal_memset(&entry_temp[i], 0, 
                        sizeof(ing_flex_ctr_counter_table_0_entry_t));
                BCM_IF_ERROR_RETURN(soc_mem_write(unit, mem_dual, MEM_BLOCK_ANY,
                                                  start + i, &entry_temp[i]));
            }
#endif
        }
        return BCM_E_NONE;
    }

    /* If we want to update both byte and packet fields,
     * we don't need to read the original value */
    if (byte_flag != -1) {
        if (soc_mem_read_range(unit, mem, MEM_BLOCK_ANY, start,
                               start+entry_cnt-1, entry) != BCM_E_NONE) {
            return BCM_E_INTERNAL;
        }
    }

    for (i = 0 ; i < entry_cnt ; i++) {
        ing_flex_ctr_counter_table_0_entry_t *entry_temp = entry;
        /* Bytes Counter update */
        if (byte_flag == 1 || byte_flag == -1) {
            FLEX_SET_BYTE_COUNTER_UPDATE;
#if defined(BCM_TRIDENT2_SUPPORT)
            FLEX_SET_BYTE_COUNTER_UPDATE_TD2;
#endif            
        }

        /* Packet Counter update */
        if (byte_flag == 0 || byte_flag == -1) {
            FLEX_SET_PACKET_COUNTER_UPDATE;
#if defined(BCM_TRIDENT2_SUPPORT) 
            FLEX_SET_PACKET_COUNTER_UPDATE_TD2;
#endif            
        }
    }
    
    /* TD2 counter value is set in X memory, clear Y memory */
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) { 
        mem = _ctr_counter_table_x[dir][pool_id];
    }
#endif
    /* Update Hw Copy */
    if (soc_mem_write_range(unit, mem, MEM_BLOCK_ALL, start, start+entry_cnt-1,
                            entry) != BCM_E_NONE) {
        return BCM_E_INTERNAL;
    }

#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) { 
        mem_dual =_ctr_counter_table_y[dir][pool_id];
        /* Update Hw Copy */
        sal_memset(entry, 0, entry_cnt * sizeof(ing_flex_ctr_counter_table_0_entry_t));
        if (soc_mem_write_range(unit, mem_dual, MEM_BLOCK_ALL, start, start+entry_cnt-1,
                                entry) != BCM_E_NONE) {
            return BCM_E_INTERNAL;
        }
    }
#endif
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_stat_flex_set
 * Description:
 *      Set flex counter values
 * Parameters:
 *      unit                  - (IN) unit number
 *      index                 - (IN) Flex Accounting Table Index
 *      table                 - (IN) Flex Accounting Table 
 *      byte_flag             - (IN) Byte/Packet Flag
 *      flex_ctr_offset_info  - (IN) Information about flex counter values
 *      flex_entries          - (IN) Information about flex entries
 *      offset_mode           - (IN) FLEX_CTR_OFFSET_MODE field value
 *      pool_number           - (IN) FLEX_CTR_POOL_NUMBER field value
 *      base_idx              - (IN) FLEX_CTR_BASE_COUNTER_IDX field value
 *      direction             - (IN) Flex Data flow direction
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      
 */
static bcm_error_t 
_bcm_esw_stat_flex_set(
    int                             unit,
    uint32                          index,
    soc_mem_t                       table,
    uint32                          byte_flag,
    bcm_stat_flex_ctr_offset_info_t flex_ctr_offset_info,
    uint32                          *flex_entries,
    uint32                          offset_mode,
    uint32                          pool_number,
    uint32                          base_idx ,
    bcm_stat_flex_direction_t       direction)
{
    soc_mem_t                 mem;
    uint32                    total_entries=0;
    uint32                    offset_index=0;

    if (flex_temp_counter[unit][direction] == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Unit %d Not initilized or attached yet\n"),
                   unit));
        return BCM_E_CONFIG;
    }

    if (direction == bcmStatFlexDirectionIngress) {
        total_entries = flex_ingress_modes[unit][offset_mode].total_counters;
    } else {
        total_entries = flex_egress_modes[unit][offset_mode].total_counters;
    }
    offset_index = 0;

    *flex_entries = total_entries;
    BCM_STAT_FLEX_COUNTER_LOCK(unit);
    mem = _ctr_counter_table[direction][pool_number];

    if (BCM_FAILURE(_bcm_esw_stat_flex_set_mem_update(unit, table, index,
                            mem, base_idx+offset_index, total_entries,
                            flex_temp_counter[unit][direction], direction,
                            pool_number, byte_flag))) {
        BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
        return BCM_E_INTERNAL;
    }

    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_ext_stat_flex_set
 * Description:
 *      Set flex counter values for external fp policy 
 * Parameters:
 *      unit                  - (IN) unit number
 *      pool_number           - (IN) Flex Pool Number for Accounting Object
 *      base_idx              - (IN) Flex Base Index for Accounting Object
 *      offset_mode           - (IN) Flex offset mode for Accounting Object
 *      table                 - (IN) Flex Accounting Table 
 *      byte_flag             - (IN) Byte/Packet Flag
 *      flex_ctr_offset_info  - (IN) Information about flex counter values
 *      flex_entries          - (IN) Information about flex entries
 *      flex_values           - (IN) Information about flex entries values
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      
 */
static bcm_error_t
_bcm_esw_ext_stat_flex_set(
    int                             unit,
    uint32                          pool_number,
    uint32                          base_idx,
    bcm_stat_flex_mode_t            offset_mode,
    soc_mem_t                       table,
    uint32                          byte_flag,
    bcm_stat_flex_ctr_offset_info_t flex_ctr_offset_info,
    uint32                          *flex_entries,
    bcm_stat_flex_counter_value_t   *flex_values)
{
    soc_mem_t                 mem;
    uint32                    loop=0;
    bcm_stat_flex_direction_t direction = bcmStatFlexDirectionIngress;
    uint32                    total_entries=0;
    uint32                    offset_index=0;
    uint32                    zero=0;
    uint32                    one=1;
    uint32                    max_packet_mask=0;
    uint64                    max_byte_mask;
    uint32                    hw_val[2];

    mem = _ctr_counter_table[direction][0];
    COMPILER_64_SET(max_byte_mask, zero, one);
    COMPILER_64_SHL(max_byte_mask,soc_mem_field_length(unit,mem,BYTE_COUNTERf));
    COMPILER_64_SUB_32(max_byte_mask,one);

    max_packet_mask = (1 << soc_mem_field_length(unit,mem,PACKET_COUNTERf));
    max_packet_mask -= 1;

    if ((offset_mode == 0) && (base_idx == 0)) {
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "EXT_FP_POLICY table: pool_number:%d: IsNotConfigured"
                                " for flex counter \n"),
                    pool_number));
         /* Either Not configured or deallocated before */
         return BCM_E_NOT_FOUND;
    }

    if (direction == bcmStatFlexDirectionIngress) {
        total_entries= flex_ingress_modes[unit][offset_mode].total_counters;
    } else {
        total_entries = flex_egress_modes[unit][offset_mode].total_counters;
    }
    if (flex_ctr_offset_info.all_counters_flag) {
        offset_index = 0;
    } else {
        offset_index = flex_ctr_offset_info.offset_index;
        if (offset_index >= total_entries) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Wrong OFFSET_INDEX.Must be <Total Counters %d \n"),
                       total_entries));
            return BCM_E_PARAM;
        }
        total_entries = 1;
    }

    *flex_entries = total_entries;
    BCM_STAT_FLEX_COUNTER_LOCK(unit);
    mem = _ctr_counter_table[direction][pool_number];
    if (soc_mem_read_range(unit,
                           mem,
                           MEM_BLOCK_ANY,
                           base_idx+offset_index,
                           base_idx+offset_index+total_entries-1,
                           flex_temp_counter[unit][direction]) != BCM_E_NONE) {
        BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
        return BCM_E_INTERNAL;
    }

    for (loop = 0 ; loop < total_entries ; loop++) {
         if (byte_flag == 1) {
             /* Mask to possible max values */
             COMPILER_64_AND(flex_values[loop].byte_counter_value,
                                 max_byte_mask);
             /* Update Soft Copy */
             COMPILER_64_SET(flex_byte_counter[unit][direction]
                                 [pool_number][base_idx+offset_index+loop],
                                 COMPILER_64_HI(flex_values[loop].
                                                byte_counter_value),
                                 COMPILER_64_LO(flex_values[loop].
                                                byte_counter_value));
             /* Change Read Hw Copy */
             hw_val[0] = COMPILER_64_HI(flex_values[loop].
                                            byte_counter_value);
             hw_val[1] = COMPILER_64_LO(flex_values[loop].
                                            byte_counter_value);
             soc_mem_field_set(
                         unit,mem,
                         (uint32 *)&flex_temp_counter[unit][direction][loop],
                         BYTE_COUNTERf, hw_val);
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Byte Count Value\t:TABLE:EXT_FP_POLICY COUNTER-%d"
                                    "(@ Pool:%d Direction:%d ActualOffset%d)"
                                    " : %x:%x \n"),
                        loop, pool_number, direction,
                        base_idx+offset_index+loop,
                        COMPILER_64_HI(flex_values[loop].
                        byte_counter_value),
                        COMPILER_64_LO(flex_values[loop].
                        byte_counter_value)));
        } else {
            flex_values[loop].pkt_counter_value &= max_packet_mask;
            /* Update Soft Copy 64-bit */
            COMPILER_64_SET(flex_packet64_counter[unit][direction]
                            [pool_number][base_idx+offset_index+loop],
                            0,
                            flex_values[loop].
                            pkt_counter_value);

            /* Update Soft Copy */
            flex_packet_counter[unit][direction][pool_number]
                                   [base_idx+offset_index+loop] =
                                   flex_values[loop].pkt_counter_value;
            /* Change Read Hw Copy */
            soc_mem_field_set(
                            unit,
                            mem,
                            (uint32 *)&flex_temp_counter[unit][direction][loop],
                            PACKET_COUNTERf,
                            &flex_values[loop].pkt_counter_value);
                    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                              (BSL_META_U(unit,
                                          "Packet Count Value\t:TABLE:EXT_FP_POLICY "
                                           "COUNTER-%d (@ Pool:%d Direction:%d "
                                           "ActualOffset%d) : %x \n"),
                               loop, pool_number,direction,
                               base_idx+offset_index+loop,
                               flex_values[loop].pkt_counter_value));
                }
        }

    /* Update Hw Copy */
    if (soc_mem_write_range(unit,mem,MEM_BLOCK_ALL,
                    base_idx+offset_index,
                    base_idx+offset_index+total_entries-1,
                    flex_temp_counter[unit][direction]) != BCM_E_NONE) {
        BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
        return BCM_E_INTERNAL;
    }
    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
    return BCM_E_NONE;
}


#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      _bcm_esw_stat_flex_sync
 * Description:
 *      Sync flex s/w copy to h/w copy
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_sync(int unit)
{
    bcm_stat_flex_group_mode_related_info_t *flex_group_mode_related_info =
                                 (bcm_stat_flex_group_mode_related_info_t *)
                                 (&flex_scache_ptr[unit][0] + 
                                   BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE);
    uint32 mode=0;
    uint32 sel=0;
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "_bcm_esw_stat_flex_sync \n")));
    if ((handle == 0) || 
        (flex_scache_allocated_size == 0) ||
        (flex_scache_ptr[unit] == NULL)) {
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Scache memory was not allocate in init!! \n")));
         return BCM_E_CONFIG;
    }
    sal_memcpy(&flex_scache_ptr[unit][0],
               &local_scache_ptr[unit][0],
               BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE);
    for (mode=0; mode < BCM_STAT_FLEX_COUNTER_MAX_MODE ; mode++) {
         if (flex_ingress_modes[unit][mode].flags != 0) {
             flex_group_mode_related_info->flags = flex_ingress_modes[unit]
                                                   [mode].flags;
             flex_group_mode_related_info->num_selectors = 
                                                   flex_ingress_modes[unit]
                                                   [mode].num_selectors;
             flex_group_mode_related_info->total_counters = 
                                                   flex_ingress_modes[unit]
                                                   [mode].total_counters;
             for (sel=0; 
                  sel < flex_group_mode_related_info->num_selectors;
                  sel++) {
                  flex_group_mode_related_info->attr_selectors[sel] = 
                                                   flex_ingress_modes[unit]
                                                   [mode].attr_selectors[sel];
             }
         }
         flex_group_mode_related_info++;
    }
    for (mode=0; mode < BCM_STAT_FLEX_COUNTER_MAX_MODE ; mode++) {
         if (flex_egress_modes[unit][mode].flags != 0) {
             flex_group_mode_related_info->flags = flex_egress_modes[unit]
                                                   [mode].flags;
             flex_group_mode_related_info->num_selectors = 
                                                   flex_egress_modes[unit]
                                                   [mode].num_selectors;
             flex_group_mode_related_info->total_counters = 
                                                   flex_egress_modes[unit]
                                                   [mode].total_counters;
             for (sel=0; 
                  sel < flex_group_mode_related_info->num_selectors;
                  sel++) {
                  flex_group_mode_related_info->attr_selectors[sel] = 
                                                   flex_egress_modes[unit]
                                                   [mode].attr_selectors[sel];
             }
         }
         flex_group_mode_related_info++;
    }
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "OK \n")));
    return BCM_E_NONE;
}
#endif
/*
 * Function:
 *      _bcm_esw_stat_flex_cleanup
 * Description:
 *      Clean and free all allocated  flex resources
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_cleanup(int unit)
{
    bcm_stat_flex_direction_t    direction=bcmStatFlexDirectionIngress;
    uint32      pool_id=0;

    uint32      num_pools[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
#if defined(BCM_TRIDENT2_SUPPORT)
    int         buffer_id;
#endif /* BCM_TRIDENT2_SUPPORT */

    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "_bcm_esw_stat_flex_cleanup \n")));
    num_pools[bcmStatFlexDirectionIngress]=SOC_INFO(unit).
                                           num_flex_ingress_pools;
    num_pools[bcmStatFlexDirectionEgress] =SOC_INFO(unit).
                                           num_flex_egress_pools;
    if (flex_stat_mutex[unit] != NULL) {
        BCM_STAT_FLEX_COUNTER_LOCK(unit);
        soc_counter_extra_unregister(unit, _bcm_esw_stat_flex_callback);
        BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
        sal_mutex_destroy(flex_stat_mutex[unit]);
        flex_stat_mutex[unit] = NULL;
    }
    for (direction=bcmStatFlexDirectionIngress;direction<2;direction++) {
#if defined(BCM_TRIDENT2_SUPPORT)
         if (SOC_IS_TD2_TT2(unit)) {
             if (flex_temp_counter_dual[unit][direction] != NULL){
                 soc_cm_sfree(unit,flex_temp_counter_dual[unit][direction]);
                 flex_temp_counter_dual[unit][direction]=NULL;
             }
         }
#endif /* BCM_TRIDENT2_SUPPORT*/

         if (flex_temp_counter[unit][direction] != NULL){
             soc_cm_sfree(unit,flex_temp_counter[unit][direction]);
             flex_temp_counter[unit][direction]=NULL;
         }
         for (pool_id=0;pool_id<num_pools[direction];pool_id++) {
              _bcm_esw_stat_flex_enable_pool(
                       unit, 
                       direction,
                       _pool_ctr_register[direction][pool_id],
                       0);
              if (flex_aidxres_list_handle[unit][direction][pool_id] != 0) {
                  shr_aidxres_list_destroy(flex_aidxres_list_handle[unit]
                                           [direction][pool_id]);
                  flex_aidxres_list_handle[unit][direction][pool_id] = 0;
              }
              flex_pool_stat[unit][direction][pool_id].used_by_tables = 0;
              flex_pool_stat[unit][direction][pool_id].used_entries = 0;
              flex_pool_stat[unit][direction][pool_id].attached_entries = 0;
              SHR_BITCLR_RANGE(flex_pool_stat[unit][direction][pool_id].used_by_objects, 0, bcmStatObjectMaxValue);
              if (flex_byte_counter[unit][direction][pool_id] != NULL) {
                  soc_cm_sfree(unit,
                               flex_byte_counter[unit][direction][pool_id]); 
                  flex_byte_counter[unit][direction][pool_id] = NULL;
              }
              if (flex_packet_counter[unit][direction][pool_id] != NULL) {
                  soc_cm_sfree(unit,
                               flex_packet_counter[unit][direction][pool_id]); 
                  flex_packet_counter[unit][direction][pool_id] = NULL;
              }
              if (flex_packet64_counter[unit][direction][pool_id] != NULL) {
                  soc_cm_sfree(unit,
                               flex_packet64_counter[unit][direction][pool_id]); 
                  flex_packet64_counter[unit][direction][pool_id] = NULL;
              }

              if(flex_base_index_reference_count[unit][direction][pool_id] 
                 != NULL) {
                 sal_free(flex_base_index_reference_count[unit][direction]
                          [pool_id]);
                 flex_base_index_reference_count[unit][direction][pool_id]=NULL;
              }
#if defined(BCM_TRIDENT2_SUPPORT)
            if (SOC_IS_TD2_TT2(unit)) {
                for (buffer_id = 0; buffer_id < _BCM_FLEX_STAT_BUFFS;
                     buffer_id++) {
                    if (flex_byte_counter_x[unit][direction][pool_id][buffer_id]
                                                                      != NULL) {
                        soc_cm_sfree(unit,
                                     flex_byte_counter_x[unit][direction]
                                     [pool_id][buffer_id]); 
                        flex_byte_counter_x[unit][direction][pool_id][buffer_id]
                            = NULL;
                    }
                    if (flex_byte_counter_y[unit][direction][pool_id][buffer_id]
                                                                      != NULL) {
                        soc_cm_sfree(unit,
                                     flex_byte_counter_y[unit][direction]
                                     [pool_id][buffer_id]); 
                        flex_byte_counter_y[unit][direction][pool_id][buffer_id]
                            = NULL;
                    }
                    if (flex_pkt_counter_x[unit][direction][pool_id][buffer_id]
                                                                      != NULL) {
                        soc_cm_sfree(unit,
                                     flex_pkt_counter_x[unit][direction]
                                     [pool_id][buffer_id]); 
                        flex_pkt_counter_x[unit][direction][pool_id][buffer_id]
                            = NULL;
                    }
                    if (flex_pkt_counter_y[unit][direction][pool_id][buffer_id]
                                                                      != NULL) {
                        soc_cm_sfree(unit,
                                     flex_pkt_counter_y[unit][direction]
                                     [pool_id][buffer_id]); 
                        flex_pkt_counter_y[unit][direction][pool_id][buffer_id]
                            = NULL;
                    }
                }
            }
#endif /* BCM_TRIDENT2_SUPPORT*/
         }
    }
    if (local_scache_ptr[unit] != NULL) {
        sal_free(local_scache_ptr[unit]);
        local_scache_ptr[unit] = NULL;
    }
    if (flex_ingress_modes[unit] != NULL) {
        sal_free(flex_ingress_modes[unit]);
        flex_ingress_modes[unit] = NULL;
    }
    if (flex_egress_modes[unit] != NULL) {
        sal_free(flex_egress_modes[unit]);
        flex_egress_modes[unit] = NULL;
    }
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "OK \n")));
    return BCM_E_NONE;
}

STATIC bcm_error_t _bcm_esw_stat_flex_object_config_init(int unit)
{
    int i;
    int count = COUNTOF(ing_flex_object_config);

    for (i = 0; i < count; i++) {
        ing_flex_object_config[i].config_pool = INVALID_CONFIG;
        ing_flex_object_config[i].share_criteria = INVALID_CONFIG;
        ing_flex_object_config[i].exclude_criteria = INVALID_CONFIG;
    }
    
    return BCM_E_NONE;
}


STATIC bcm_error_t _bcm_esw_stat_flex_object_config_get(
                int unit,
                soc_mem_t mem,
                ing_flex_object_config_t *object_config
                )
{
    int i;
    soc_mem_t config_mem;
    int count = COUNTOF(ing_flex_object_config);

    switch(mem) {
        case MPLS_ENTRYm: 
#if defined(BCM_TRIUMPH3_SUPPORT)
        case MPLS_ENTRY_EXTDm: 
#endif
            config_mem = MPLS_ENTRYm;
            break;
        case VLAN_XLATEm:
#if defined(BCM_TRIUMPH3_SUPPORT)
        case VLAN_XLATE_EXTDm:        
#endif         
            config_mem = VLAN_XLATEm;
            break;
        case L3_ENTRY_2m:
        case L3_ENTRY_4m:
        case EXT_IPV4_UCAST_WIDEm:
        case EXT_IPV6_128_UCAST_WIDEm: 
            config_mem = L3_ENTRY_2m;
            break;
#if defined(BCM_TRIDENT2_SUPPORT)
        case L3_DEFIPm:
        case L3_DEFIP_PAIR_128m:
        case L3_DEFIP_ALPM_IPV4_1m:
        case L3_DEFIP_ALPM_IPV6_64_1m:
        case L3_DEFIP_ALPM_IPV6_128m:
        case L3_DEFIP_AUX_SCRATCHm:
            config_mem = L3_DEFIPm;
            break;            
        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
            config_mem = L3_ENTRY_IPV4_MULTICASTm;
            break;
#endif            
        default:
            config_mem = mem;
            break;
    }

    for (i = 0; i < count; i++) {
        if (config_mem == ing_flex_object_config[i].obj_mem) {
            *object_config = ing_flex_object_config[i];
            break;
        }
    }

    if (i >= count) {
        object_config = NULL;
        return BCM_E_NOT_FOUND;
    }
    
    return BCM_E_NONE;
}

STATIC bcm_error_t _bcm_esw_stat_flex_object_config_set(
                int unit,
                soc_mem_t mem,
                ing_flex_object_config_t object_config
                )
{
    int i;
    soc_mem_t config_mem;
    int count = COUNTOF(ing_flex_object_config);

    switch(mem) {
        case MPLS_ENTRYm: 
#if defined(BCM_TRIUMPH3_SUPPORT)
        case MPLS_ENTRY_EXTDm: 
#endif
            config_mem = MPLS_ENTRYm;
            break;
        case VLAN_XLATEm:
#if defined(BCM_TRIUMPH3_SUPPORT)
        case VLAN_XLATE_EXTDm:        
#endif         
            config_mem = VLAN_XLATEm;
            break;
        case L3_ENTRY_2m:
        case L3_ENTRY_4m:
        case EXT_IPV4_UCAST_WIDEm:
        case EXT_IPV6_128_UCAST_WIDEm: 
            config_mem = L3_ENTRY_2m;
            break;
#if defined(BCM_TRIDENT2_SUPPORT)
        case L3_DEFIPm:
        case L3_DEFIP_PAIR_128m:
        case L3_DEFIP_ALPM_IPV4_1m:
        case L3_DEFIP_ALPM_IPV6_64_1m:
        case L3_DEFIP_ALPM_IPV6_128m:
        case L3_DEFIP_AUX_SCRATCHm:
            config_mem = L3_DEFIPm;
            break;            
        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
            config_mem = L3_ENTRY_IPV4_MULTICASTm;
            break;
#endif            
        default:
            config_mem = mem;
            break;
    }

    for (i = 0; i < count; i++) {
        if (config_mem == ing_flex_object_config[i].obj_mem) {
            ing_flex_object_config[i] = object_config;
            break;
        }
    }

    if (i >= count) {
        return BCM_E_NOT_FOUND;
    }
    
    return BCM_E_NONE;
}


STATIC bcm_error_t _bcm_esw_stat_flex_object_update(
                int unit, 
                SHR_BITDCL *object_bmp,
                int share)
{
    int i, j, num = 0;
    uint32 config_criteria, config_pool = 0;
    soc_mem_t mem;
    int count = COUNTOF(ing_flex_object_default);
    ing_flex_object_config_t object_config;
        
    for (i = 0; i < count; i++) {
        if (SHR_BITGET(object_bmp, i)) {
            num++;
            /*set config pool for sharing*/
            config_pool = ing_flex_object_default[i].pool_number;

            BCM_IF_ERROR_RETURN(
                _bcm_esw_stat_flex_object_config_get(unit, 
                    ing_flex_object_default[i].obj_mem, &object_config));
            
            /*already set*/
            if ((share && (object_config.share_criteria != 0)) ||
                (!share && (object_config.exclude_criteria != 0))) {
                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "\"%s\" has already been set or "
                                      "duplicated object in HW\n"), 
                        ing_flex_object_default[i].obj_desc));
                return BCM_E_INTERNAL;
            }
        }
    }

    if (num <= 1) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Too little objects in the property string. "
                              "Object count %d\n"), 
                num));
        return BCM_E_INTERNAL;        
    }

    for (i = 0; i < count; i++) {
        config_criteria = 0;
        if (SHR_BITGET(object_bmp, i)) {
            mem = ing_flex_object_default[i].obj_mem;
            for (j = 0; j < count; j++) {
                if (j == i) {
                    continue;
                }
                
                if (SHR_BITGET(object_bmp, j) &&
                    (mem != ing_flex_object_default[j].obj_mem)) {
                    config_criteria |= 
                        ing_flex_object_default[j].pool_criteria;
                }
            }

            BCM_IF_ERROR_RETURN(
                _bcm_esw_stat_flex_object_config_get(unit, 
                    mem, &object_config));

            if (share) {
                object_config.share_criteria |= config_criteria;
                object_config.config_pool = config_pool;
            }
            else {
                object_config.exclude_criteria |= config_criteria;
            }

            BCM_IF_ERROR_RETURN(
                _bcm_esw_stat_flex_object_config_set(unit, 
                    mem, object_config));            

        }
    }

    return BCM_E_NONE;
}


STATIC bcm_error_t _bcm_esw_stat_flex_property_get(
                int unit, 
                char *property_str) 
{
    char *sub_str = property_str;
    int i, j, share, str_len, share_len, split_len;
    int count = COUNTOF(ing_flex_object_default);
    ing_flex_object_default_t *ing_flex_obj;
    SHR_BITDCL object_bmp[_SHR_BITDCLSIZE(COUNTOF(ing_flex_object_default))];
    SHR_BITDCL object_share[_SHR_BITDCLSIZE(COUNTOF(ing_flex_object_default))];
    SHR_BITDCL object_exclude[_SHR_BITDCLSIZE(COUNTOF(ing_flex_object_default))]; 

    sal_memset(object_share, 0, _SHR_BITDCLSIZE(count) * sizeof(SHR_BITDCL));
    sal_memset(object_exclude, 0, _SHR_BITDCLSIZE(count) * sizeof(SHR_BITDCL));

    share_len = sal_strlen("share");
    split_len = sal_strlen("split");

    /*initialize the object config*/
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_object_config_init(unit));
    
    /*share(vlan,vfi);split(vlan,ipmc)
     *Specifies ingress objects sharing same pool or using exclusive pool.
     *example:                                                         
     *ing_share_flex_counter_pool=split(vlan,vfi) make vlan, vfi use exclusive pool.
     *ing_share_flex_counter_pool=share(vlan,vfi) make vlan, vfi use same pool.
     *an object can appear in both share case and split case.           
     *but in each case it can appear one time at most.                  
     *use token between each groups and use comma between each objects.
     *eg: ing_share_flex_counter_pool=split(A,B,C);share(C,D);   
     *some objects may use the same HW table, so only the first one will take effect.
     *all valid object name: ',                                          
     *port, vlan, vlanxlate, vfi, l3intf, vrf, policy, mplsvclabel, mplsswitchlabel, mplsfrrlabel,
     *l3host, trill, mimlookupid, l2gre, extpolicy, vxlan, vsan, fcoe, l3route, niv, ipmc.
     */
    
    /*Parse the property syntex*/
    while (*sub_str != '\0') {        
        if (*sub_str == ';' || *sub_str == ' ') {
            sub_str ++;
            continue;
        }   

        /*Initialization*/
        sal_memset(object_bmp, 0, _SHR_BITDCLSIZE(count) * sizeof(SHR_BITDCL));
        i = 0;

        if (!sal_memcmp(sub_str, "share", share_len)) {
            share = TRUE;
            sub_str += share_len;
        }
        else if (!sal_memcmp(sub_str, "split", split_len)) {
            share = FALSE;
            sub_str += split_len;
        }
        else {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "ERROR! Missing share information. "
                                  "Soc property \"%s\"\n"), 
                       property_str));
            return BCM_E_FAIL;
        }

        if (*sub_str == '(') {
            sub_str++;          
        }
        else {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "ERROR! Illegal syntax. Soc property \"%s\"\n"), 
                       property_str)); 
            return BCM_E_FAIL;  
        }
             
        while (i < count) {
            ing_flex_obj = &ing_flex_object_default[i];
            str_len = sal_strlen(ing_flex_obj->obj_desc);
            if (!sal_memcmp(sub_str, 
                    ing_flex_obj->obj_desc, str_len)) {
                SHR_BITSET(object_bmp, i);
                sub_str += str_len;
                
                if (*sub_str == ')') {
                    /*Check if there are duplicated objects*/
                    for (j = 0; j < count; j++) {
                        if (share) {
                            if (SHR_BITGET(object_bmp, j) && 
                                SHR_BITGET(object_share, j)) {
                                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                                          (BSL_META_U(unit,
                                                      "ERROR! Duplicated object. "
                                                      "Soc property \"%s\"\n"), 
                                           property_str)); 
                                 return BCM_E_FAIL;                                
                            }
                        }
                        else {
                            if (SHR_BITGET(object_bmp, j) && 
                                SHR_BITGET(object_exclude, j)) {
                                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                                          (BSL_META_U(unit,
                                                      "ERROR! Duplicated object. "
                                                      "Soc property \"%s\"\n"), 
                                                    property_str)); 
                                 return BCM_E_FAIL;                                
                            }
                        }
                    }

                    /*Do the further process*/
                    BCM_IF_ERROR_RETURN(
                        _bcm_esw_stat_flex_object_update(unit, object_bmp, share));
                    /*save the configuration stat*/
                    if (share) {
                        SHR_BITOR_RANGE
                            (object_bmp, object_share, 0, count, object_share);
                    }
                    else {
                        SHR_BITOR_RANGE
                            (object_bmp, object_exclude, 0, count, object_exclude);
                    }
                    sub_str++;
                    break;
                }
                else if (*sub_str == ',') {
                    /*parse next object*/
                    sub_str ++;
                    i = 0;
                    continue;
                }
                else {
                    LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                              (BSL_META_U(unit,
                                          "ERROR! Illegal string information. "
                                          "Soc property \"%s\"\n"), 
                               property_str));
                    return BCM_E_FAIL;                    
                }
            }         
            i++;
        }

        if(i >= count) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "ERROR! Object not found. Soc property \"%s\" \"%s\"\n"), 
                       property_str, sub_str));
            return BCM_E_NOT_FOUND;
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_stat_flex_init
 * Description:
 *      Initialize and allocate all required flex resources
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_init(int unit)
{
    bcm_stat_flex_direction_t    direction=bcmStatFlexDirectionIngress;
    /* sal_usecs_t stime; */
    uint32    mode=0;

    uint32    selector_count=0;
    uint32    index=0;
    uint32    selector_x_en_field_value[8]={0};
    uint32    selector_for_bit_x_field_value[8]={0};
    uint32    IsAnyModeConfigured=0;
    uint32    pool_id=0;

    uint32    num_pools[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32    size_pool[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32    alloc_size=0;
    uint64    selector_key_value;
#ifdef BCM_WARM_BOOT_SUPPORT
    int       stable_size=0;
    int       stable_used_size=0;
#endif
    bcm_error_t rv=BCM_E_NONE;

#if defined(BCM_TRIDENT2_SUPPORT)
    int       buffer_id = 0;
#endif /* BCM_TRIDENT2_SUPPORT*/
	char *config_str;


    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "_bcm_esw_stat_flex_init \n")));
    _bcm_esw_stat_flex_init_pkt_attr_bits(unit);
    _bcm_esw_stat_flex_init_pkt_res_fields(unit);

    _bcm_esw_stat_flex_cleanup(unit);

    COMPILER_64_ZERO(selector_key_value);

    num_pools[bcmStatFlexDirectionIngress]= SOC_INFO(unit).
                                            num_flex_ingress_pools;
    num_pools[bcmStatFlexDirectionEgress] = SOC_INFO(unit).
                                            num_flex_egress_pools;
    size_pool[bcmStatFlexDirectionIngress]= SOC_INFO(unit).
                                            size_flex_ingress_pool;
    size_pool[bcmStatFlexDirectionEgress] = SOC_INFO(unit).
                                            size_flex_egress_pool;

    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "INFO: ingress_pools:%d num_flex_egress_pools:%di "
                           "size_flex_ingress_pool:%d size_flex_egress_pool:%d"
                           "MAX_DIRECTION %d MAX_MODE %d MAX_POOL %d\n"),
               num_pools[bcmStatFlexDirectionIngress],
               num_pools[bcmStatFlexDirectionEgress],
               size_pool[bcmStatFlexDirectionIngress],
               size_pool[bcmStatFlexDirectionEgress],
               BCM_STAT_FLEX_COUNTER_MAX_DIRECTION,
               BCM_STAT_FLEX_COUNTER_MAX_MODE,
               BCM_STAT_FLEX_COUNTER_MAX_POOL));
    if ((num_pools[bcmStatFlexDirectionIngress] == 0) ||
        (num_pools[bcmStatFlexDirectionEgress] == 0)) {
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "INFO:Number of CounterPools missing.PleaseDefine it\n")));
         return BCM_E_INTERNAL;
    }
    if ((num_pools[bcmStatFlexDirectionIngress] >
         BCM_STAT_FLEX_COUNTER_MAX_POOL) ||
        (num_pools[bcmStatFlexDirectionEgress] >
         BCM_STAT_FLEX_COUNTER_MAX_POOL)) {
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "INFO: Number of pools exceeding its max value %d \n"),
                    BCM_STAT_FLEX_COUNTER_MAX_POOL));
         return BCM_E_INTERNAL;
    }
    if (flex_stat_mutex[unit] == NULL) {
        flex_stat_mutex[unit]= sal_mutex_create("Advanced flex counter mutex");
        if (flex_stat_mutex[unit] == NULL) {
            _bcm_esw_stat_flex_cleanup(unit);
            return BCM_E_MEMORY;
        }
    }
    flex_ingress_modes[unit] = (bcm_stat_flex_ingress_mode_t *) sal_alloc(
                                sizeof(bcm_stat_flex_ingress_mode_t) *
                                BCM_STAT_FLEX_COUNTER_MAX_MODE,
                                "flex_ingress_modes");
    if (flex_ingress_modes[unit] == NULL) {
        _bcm_esw_stat_flex_cleanup(unit);
        return BCM_E_MEMORY;
    }
    flex_egress_modes[unit] = (bcm_stat_flex_egress_mode_t *) sal_alloc(
                                sizeof(bcm_stat_flex_egress_mode_t) *
                                BCM_STAT_FLEX_COUNTER_MAX_MODE,
                                "flex_egress_modes");
    if (flex_egress_modes[unit] == NULL) {
        _bcm_esw_stat_flex_cleanup(unit);
        return BCM_E_MEMORY;
    }

    /* Initialize Ingress Mode structures*/
    for (mode=0;mode<BCM_STAT_FLEX_COUNTER_MAX_MODE;mode++) {
         sal_memset(&flex_ingress_modes[unit][mode],0,
                    sizeof(bcm_stat_flex_ingress_mode_t));
    }
    /* Initialize Egress Mode structures*/
    for (mode=0;mode<BCM_STAT_FLEX_COUNTER_MAX_MODE;mode++) {
         sal_memset(&flex_egress_modes[unit][mode],0,
                    sizeof(bcm_stat_flex_egress_mode_t));
    }
    for (direction=bcmStatFlexDirectionIngress;direction<2;direction++) {
         alloc_size= sizeof(ing_flex_ctr_counter_table_0_entry_t)*size_pool
                           [direction];
#if defined(BCM_TRIDENT2_SUPPORT)
         if (SOC_IS_TD2_TT2(unit)) {
             if (flex_temp_counter_dual[unit][direction] != NULL){
                 LOG_WARN(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "WARN:Freeing flex_temp_counter AllocatedMemory\n")));
                 soc_cm_sfree(unit,flex_temp_counter_dual[unit][direction]);
             }
             flex_temp_counter_dual[unit][direction] = soc_cm_salloc(
                                                unit,
                                                alloc_size,
                                                "Advanced FlexTemp packet counter");
             if (flex_temp_counter_dual[unit][direction] == NULL) {
                 LOG_WARN(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "Advanced Flex current PacketCounterAllocationFail"
                                       "for unit:%d, dir:%d pool:%d\n"),
                           unit,direction,pool_id));
                 _bcm_esw_stat_flex_cleanup(unit);
                 return BCM_E_MEMORY;
             }
             sal_memset(flex_temp_counter_dual[unit][direction], 0, alloc_size);
         }
#endif /* BCM_TRIDENT2_SUPPORT*/

         if (flex_temp_counter[unit][direction] != NULL){
             LOG_WARN(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "WARN:Freeing flex_temp_counter AllocatedMemory\n")));
             soc_cm_sfree(unit,flex_temp_counter[unit][direction]);
         }
         flex_temp_counter[unit][direction] = soc_cm_salloc(
                                            unit,
                                            alloc_size,
                                            "Advanced FlexTemp packet counter");
         if (flex_temp_counter[unit][direction] == NULL) {
             LOG_WARN(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Advanced Flex current PacketCounterAllocationFail"
                                   "for unit:%d, dir:%d pool:%d\n"),
                       unit,direction,pool_id));
             _bcm_esw_stat_flex_cleanup(unit);
             return BCM_E_MEMORY;
         }
         sal_memset(flex_temp_counter[unit][direction], 0, alloc_size);
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Temp counter size:%d \n"),
                    alloc_size));
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Byte counter size:%d \n"),
                    (int)sizeof(uint64)*size_pool[direction]));
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Packet counter size:%d\n"),
                    (int)sizeof(uint32)*size_pool[direction]));
         for (pool_id=0;pool_id<num_pools[direction];pool_id++) {
              LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                        (BSL_META_U(unit,
                                    ".")));
              /* Disable all counter pools */
              _bcm_esw_stat_flex_enable_pool(
                      unit, 
                      direction,
                      _pool_ctr_register[direction][pool_id],
                      0);
              /* Destroy if any exist */
              shr_aidxres_list_destroy(flex_aidxres_list_handle[unit]
                                       [direction][pool_id]);
              /* Create it */
              if (shr_aidxres_list_create(
                     &flex_aidxres_list_handle[unit][direction][pool_id],
                     0,size_pool[direction]-1,
                     0,size_pool[direction]-1,
                     8, /*  
                         * MAX 256 counters per-block:
                         * No. of blocks = 
                         * (No. of counters Per-Flex CTR Pool / 256) 
                         */
                     "flex-counter") != BCM_E_NONE) {
                  LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                            (BSL_META_U(unit,
                                        "Unrecoverable error. "
                                         "Couldn'tCreate AllignedList:FlexCounter\n")));
                  _bcm_esw_stat_flex_cleanup(unit);
                  return BCM_E_INTERNAL;
              }
              /* Reserver first two counters entries i.e. not to be used. */
              /* Flexible counter updates only if counter_base_index is != 0 */
              /* Refer: Arch-spec section 4.1 */
              shr_aidxres_list_reserve_block(
                          flex_aidxres_list_handle[unit][direction][pool_id],
                          0,
                          (1 << 1));
              flex_pool_stat[unit][direction][pool_id].used_by_tables = 0;
              flex_pool_stat[unit][direction][pool_id].used_entries = 0;
              flex_pool_stat[unit][direction][pool_id].attached_entries = 0;
              SHR_BITCLR_RANGE(flex_pool_stat[unit][direction][pool_id].used_by_objects, 0, bcmStatObjectMaxValue);
              if (flex_byte_counter[unit][direction][pool_id] != NULL) {
                  LOG_WARN(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "WARN:Freeing AllocatedByteCountersMemory\n"))); 
                  soc_cm_sfree(unit,
                               flex_byte_counter[unit][direction][pool_id]); 
              }
              flex_byte_counter[unit][direction][pool_id] = 
                   soc_cm_salloc(unit,sizeof(uint64)*size_pool[direction],
                                 "Advanced Flex byte counter");
              if (flex_byte_counter[unit][direction][pool_id] == NULL) {
                  LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                            (BSL_META_U(unit,
                                        "Advanced Flex ByteCounterAllocationFailed for"
                                         "unit:%d,dir:%d pool:%d:\n"), 
                             unit, direction, pool_id));
                  _bcm_esw_stat_flex_cleanup(unit);
                  return BCM_E_MEMORY;
              }
              sal_memset(flex_byte_counter[unit][direction][pool_id], 0,
                         sizeof(uint64)*size_pool[direction]);

              if (flex_packet_counter[unit][direction][pool_id] != NULL) {
                  LOG_WARN(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "WARN:FreeingAllocatedPacketCountersMemory\n"))); 
                  soc_cm_sfree(unit,
                               flex_packet_counter[unit][direction][pool_id]); 
              }
              flex_packet_counter[unit][direction][pool_id] =
                   soc_cm_salloc(unit, sizeof(uint32)*size_pool[direction],
                                 "Advanced Flex packet counter");
              if (flex_packet_counter[unit][direction][pool_id] == NULL) {
                  LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                            (BSL_META_U(unit,
                                        "Advanced Flex ByteCounterAllocation failed for"
                                         "unit:%d, dir:%d pool:%d\n"),
                             unit,direction, pool_id));
                  _bcm_esw_stat_flex_cleanup(unit);
                  return BCM_E_MEMORY;
              }
              sal_memset(flex_packet_counter[unit][direction][pool_id], 0,
                         sizeof(uint32)*size_pool[direction]);

              if (flex_packet64_counter[unit][direction][pool_id] != NULL) {
                  LOG_WARN(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "WARN:FreeingAllocatedPacketCountersMemory\n"))); 
                  soc_cm_sfree(unit,
                               flex_packet64_counter[unit][direction][pool_id]); 
              }
              flex_packet64_counter[unit][direction][pool_id] =
                   soc_cm_salloc(unit, sizeof(uint64)*size_pool[direction],
                                 "Advanced Flex packet counter");
              if (flex_packet64_counter[unit][direction][pool_id] == NULL) {
                  LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                            (BSL_META_U(unit,
                                        "Advanced Flex ByteCounterAllocation failed for"
                                         "unit:%d, dir:%d pool:%d\n"),
                             unit,direction, pool_id));
                  _bcm_esw_stat_flex_cleanup(unit);
                  return BCM_E_MEMORY;
              }
              sal_memset(flex_packet64_counter[unit][direction][pool_id], 0,
                         sizeof(uint64)*size_pool[direction]);

              if (flex_base_index_reference_count[unit][direction]
                                                 [pool_id] != NULL) {
                  LOG_WARN(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "WARN:Freeing Allocated"
                                        "flex_base_index_reference_count memory\n"))); 
                  sal_free(flex_base_index_reference_count[unit]
                                                          [direction][pool_id]);
              }
              flex_base_index_reference_count[unit][direction][pool_id] = 
                   sal_alloc(sizeof(uint16)*size_pool[direction],
                             "BaseIndexAllocation");
              if (flex_packet_counter[unit][direction][pool_id] == NULL){
                  LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                            (BSL_META_U(unit,
                                        "BaseIndex allocation failed for"
                                         "unit:%d, dir:%d pool:%d\n"),
                             unit,direction, pool_id));
                  _bcm_esw_stat_flex_cleanup(unit);
                  return BCM_E_MEMORY;
              }
              sal_memset(flex_base_index_reference_count[unit][direction]
                         [pool_id], 0,sizeof(uint16)*size_pool[direction]);

#if defined(BCM_TRIDENT2_SUPPORT)
            if (SOC_IS_TD2_TT2(unit)) {
                for (buffer_id = 0; buffer_id < _BCM_FLEX_STAT_BUFFS;
                     buffer_id++) {
                    if (flex_byte_counter_x[unit][direction][pool_id][buffer_id]
                                                                      != NULL) {
                        LOG_WARN(BSL_LS_BCM_FLEXCTR,
                                 (BSL_META_U(unit,
                                             "WARN:Freeing Allocated Byte Counters"
                                              " Memory\n"))); 
                        soc_cm_sfree(unit,
                                     flex_byte_counter_x[unit][direction]
                                     [pool_id][buffer_id]); 
                    }
                    flex_byte_counter_x[unit][direction][pool_id][buffer_id] = 
                         soc_cm_salloc(unit, 
                                       sizeof(uint64) * size_pool[direction],
                                       "Advanced Flex byte counter x");
                    if (flex_byte_counter_x[unit][direction][pool_id][buffer_id]
                                                                      == NULL) {
                         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "Advanced Flex ByteCounterAllocation"
                                                " Failed "
                                                "for unit:%d,dir:%d pool:%d buff:%d:\n"), 
                                    unit, direction, pool_id, buffer_id));
                         _bcm_esw_stat_flex_cleanup(unit);
                         return BCM_E_MEMORY;
                    }
                    sal_memset(flex_byte_counter_x[unit][direction][pool_id]
                               [buffer_id],
                               0, sizeof(uint64)*size_pool[direction]);
                 
                    if (flex_byte_counter_y[unit][direction][pool_id][buffer_id]
                                                                      != NULL) {
                        LOG_WARN(BSL_LS_BCM_FLEXCTR,
                                 (BSL_META_U(unit,
                                             "WARN:Freeing Allocated Byte Counters"
                                              " Memory\n"))); 
                        soc_cm_sfree(unit,
                         flex_byte_counter_y[unit][direction][pool_id][buffer_id]); 
                    }
                    flex_byte_counter_y[unit][direction][pool_id][buffer_id] = 
                          soc_cm_salloc(unit,
                                        sizeof(uint64)*size_pool[direction],
                                        "Advanced Flex byte counter y");
                    if (flex_byte_counter_y[unit][direction][pool_id][buffer_id]
                                                                      == NULL) {
                        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                                  (BSL_META_U(unit,
                                              "Advanced Flex ByteCounterAllocationFailed"
                                               " for unit:%d,dir:%d pool:%d: buff:%d:\n"), 
                                   unit, direction, pool_id, buffer_id));
                        _bcm_esw_stat_flex_cleanup(unit);
                        return BCM_E_MEMORY;
                    }
                    sal_memset(flex_byte_counter_y[unit][direction][pool_id]
                               [buffer_id],
                               0, sizeof(uint64)*size_pool[direction]);

                    if (flex_pkt_counter_x[unit][direction][pool_id][buffer_id]
                                                                      != NULL) {
                        LOG_WARN(BSL_LS_BCM_FLEXCTR,
                                 (BSL_META_U(unit,
                                             "WARN:Freeing AllocatedByteCountersMemory"
                                              "\n"))); 
                        soc_cm_sfree(unit,
                                     flex_pkt_counter_x[unit][direction]
                                     [pool_id][buffer_id]); 
                    }
                    flex_pkt_counter_x[unit][direction][pool_id][buffer_id] = 
                         soc_cm_salloc(unit,
                                       sizeof(uint32)*size_pool[direction],
                                       "Advanced Flex packet counter x");
                    if (flex_pkt_counter_x[unit][direction][pool_id][buffer_id]
                                                                      == NULL) {
                        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                                  (BSL_META_U(unit,
                                              "Advanced Flex ByteCounterAllocationFailed"
                                               " for unit:%d,dir:%d pool:%d: buff:%d:\n"), 
                                   unit, direction, pool_id, buffer_id));
                        _bcm_esw_stat_flex_cleanup(unit);
                        return BCM_E_MEMORY;
                    }
                    sal_memset(flex_pkt_counter_x[unit][direction][pool_id]
                               [buffer_id], 0, sizeof(uint32)*size_pool[direction]);

                    if (flex_pkt_counter_y[unit][direction][pool_id][buffer_id]
                                                                      != NULL) {
                        LOG_WARN(BSL_LS_BCM_FLEXCTR,
                                 (BSL_META_U(unit,
                                             "WARN:Freeing AllocatedByteCountersMemory"
                                              "\n"))); 
                        soc_cm_sfree(unit,
                                     flex_pkt_counter_y[unit][direction]
                                     [pool_id][buffer_id]); 
                    }
                    flex_pkt_counter_y[unit][direction][pool_id][buffer_id] = 
                         soc_cm_salloc(unit,
                                       sizeof(uint32)*size_pool[direction],
                                       "Advanced Flex packet counter y");
                    if (flex_pkt_counter_y[unit][direction][pool_id][buffer_id]
                                                                      == NULL) {
                        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                                  (BSL_META_U(unit,
                                              "Advanced Flex ByteCounterAllocationFailed"
                                               " for unit:%d,dir:%d pool:%d: buff:%d:\n"), 
                                   unit, direction, pool_id, buffer_id));
                        _bcm_esw_stat_flex_cleanup(unit);
                        return BCM_E_MEMORY;
                    }
                    sal_memset(flex_pkt_counter_y[unit][direction][pool_id]
                               [buffer_id],
                               0, sizeof(uint32)*size_pool[direction]);
                }
            }
#endif /* BCM_TRIDENT2_SUPPORT*/
         }
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "\n")));
    }

    /* First retrieve mode related information */
    if (SOC_WARM_BOOT(unit)) {
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "WARM booting...")));
        for (direction=bcmStatFlexDirectionIngress;direction<2;direction++) {
             for (mode=0;mode<BCM_STAT_FLEX_COUNTER_MAX_MODE;mode++) {
                if (SOC_REG_IS_64(
                            unit, _pkt_selector_key_reg[direction][mode])) {
                    SOC_IF_ERROR_RETURN(soc_reg64_get(
                      unit,
                      _pkt_selector_key_reg[direction][mode],
                      REG_PORT_ANY, 
                      0,
                                &selector_key_value));
                 for (selector_count=0,index=0;index<8;index++) {
                      selector_x_en_field_value[index]=
                                 soc_reg64_field32_get(
                                     unit,
                                    _pkt_selector_key_reg[direction][mode],
                                     selector_key_value,
                                     _pkt_selector_x_en_field_name[index]);
                      selector_count +=selector_x_en_field_value[index];
                      selector_for_bit_x_field_value[index]=0;
                      if (selector_x_en_field_value[index] != 0) {
                          selector_for_bit_x_field_value[index]=
                          soc_reg64_field32_get(
                              unit,
                              _pkt_selector_key_reg[direction][mode],
                              selector_key_value,
                              _pkt_selector_for_bit_x_field_name[index]);
                      }
                 }
                 if (selector_count > 0) {
                     LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                               (BSL_META_U(unit,
                                           "Selector count :%d .. "),
                                selector_count));
                     IsAnyModeConfigured=1;
                     if (soc_reg64_field32_get(
                             unit, 
                             _pkt_selector_key_reg[direction][mode],
                             selector_key_value,USE_UDF_KEYf) == 1) {
                         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "Direction:%d-Mode:%d UDF MODE is "
                                                "configured \n"),direction,mode));
                         _bcm_esw_stat_flex_init_udf_mode(
                                  unit,
                                  mode,
                                  direction,
                                  selector_count,
                                  selector_x_en_field_value,
                                  selector_for_bit_x_field_value);
                     } else {
                         if (soc_reg64_field32_get(
                             unit, 
                             _pkt_selector_key_reg[direction][mode],
                             selector_key_value,USE_COMPRESSED_PKT_KEYf) == 1) {
                             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                                       (BSL_META_U(unit,
                                                   "Direction:%d-Mode:%d"
                                                    "COMPRESSED MODE is "
                                                    "configured \n"),direction,mode));
                             _bcm_esw_stat_flex_init_cmprsd_mode(
                                      unit,
                                      mode,
                                      direction,
                                      selector_count,
                                      selector_x_en_field_value,
                                      selector_for_bit_x_field_value);
                         } else {
                             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                                       (BSL_META_U(unit,
                                                   "Direction:%d-Mode:%d"
                                                    "UNCOMPRESSED MODE is "
                                                    "configured \n"),direction,mode));
                             _bcm_esw_stat_flex_init_uncmprsd_mode(
                                      unit,
                                      mode,
                                      direction,
                                      selector_count,
                                      selector_x_en_field_value,
                                      selector_for_bit_x_field_value);
                         }
                     }
                 } else {
                     LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                               (BSL_META_U(unit,
                                           "Direction:%d-Mode:%d is unconfigured \n"),
                                direction,mode));
                 }
             }
        }
    }
    }
    /* stime = sal_time_usecs(); */
    if (IsAnyModeConfigured) {
        /* Get Ingress table info */

        /* 1) Ingress: Port Table */
        _bcm_esw_stat_flex_check_ingress_port_table(unit);
        /* LOG_CLI((BSL_META_U(unit,
                               "FLEX: 1.Taken Time: %10d usec\n "), 
                    SAL_USECS_SUB(sal_time_usecs(), stime))); */

        /* 2) Ingress: VLAN_XLATE Table */
        _bcm_esw_stat_flex_check_ingress_vlan_xlate_table(unit);
        /* 3) Ingress: VFP_POLICY_TABLE Table */
        _bcm_esw_stat_flex_check_ingress_vfp_policy_table(unit);
        /* 4) Ingress: MPLS_ENTRY Table */
        _bcm_esw_stat_flex_check_ingress_mpls_entry_table(unit);
        /* 5) Ingress: SOURCE_VP Table */
        _bcm_esw_stat_flex_check_ingress_source_vp_table(unit);
        /* 6) Ingress: L3_IIF */
        _bcm_esw_stat_flex_check_ingress_l3_iif_table(unit);
        /* 7) Ingress: VFI */
        _bcm_esw_stat_flex_check_ingress_vfi_table(unit);
        /* 8) Ingress: VRF */
        _bcm_esw_stat_flex_check_ingress_vrf_table(unit);
        /* 9) Ingress: VLAN */
        _bcm_esw_stat_flex_check_ingress_vlan_table(unit);

        /* Get Egress table info */

        /* 1) EGR_L3_NEXT_HOP */
        _bcm_esw_stat_flex_check_egress_l3_next_hop_table(unit);
        /* 2) EGR_VFI */
        _bcm_esw_stat_flex_check_egress_vfi_table(unit);
        /* 3) EGR_PORT */
        _bcm_esw_stat_flex_check_egress_port_table(unit);
        /* 4) EGR_VLAN */
        _bcm_esw_stat_flex_check_egress_vlan_table(unit);
        /* 5) EGR_VLAN_XLATE */
        _bcm_esw_stat_flex_check_egress_vlan_xlate_table(unit);
        for (direction=bcmStatFlexDirectionIngress;direction<2;direction++) {
             for (mode=0;mode<BCM_STAT_FLEX_COUNTER_MAX_MODE;mode++) {
                  _bcm_esw_stat_flex_retrieve_group_mode(unit, direction,mode);
             }
        }
    }
    soc_counter_extra_register(unit, _bcm_esw_stat_flex_callback);

    config_str = soc_property_get_str(unit, spn_ING_SHARE_FLEX_COUNTER_POOL);
    if (config_str != NULL) {
        rv = _bcm_esw_stat_flex_property_get(unit, config_str);
        if (rv != BCM_E_NONE) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "ERR: Parsing property string failed")));
            _bcm_esw_stat_flex_cleanup(unit);
            return rv;
        }
    }
    
    if(local_scache_ptr[unit] != NULL) {
       LOG_WARN(BSL_LS_BCM_FLEXCTR,
                (BSL_META_U(unit,
                            "WARN: Freeing flex_scache_ptr existing memory")));
       sal_free(local_scache_ptr[unit]);
    }
    local_scache_size= sizeof(uint32)*BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE;
    local_scache_ptr[unit] = sal_alloc(local_scache_size,"Flex scache memory");
    if (local_scache_ptr[unit]  == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "ERR: Couldnot allocate flex_scache_ptr existing memory")));
        _bcm_esw_stat_flex_cleanup(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(local_scache_ptr[unit],0,local_scache_size);
#ifdef BCM_WARM_BOOT_SUPPORT
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "WARM Booting... \n")));
    /* rv = soc_scache_recover(unit); */
    flex_scache_allocated_size=0;
    SOC_SCACHE_HANDLE_SET(handle, unit, BCM_MODULE_STAT, 
                          _BCM_STAT_WARM_BOOT_CHUNK_FLEX);
    SOC_SCACHE_MODULE_MAX_PARTITIONS_SET(unit, BCM_MODULE_STAT, 1);
    if (!SOC_WARM_BOOT(unit)) { 
        local_scache_size +=  sizeof(bcm_stat_flex_group_mode_related_info_t) *
                              BCM_STAT_FLEX_COUNTER_MAX_MODE *
                              BCM_STAT_FLEX_COUNTER_MAX_DIRECTION ;
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "ColdBoot: Modified Local Scache Size %d \n"),
                   local_scache_size));
        SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
        if (stable_size == 0) {
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "STABLE size is zero.Probably NotConfigured yet\n")));
            return BCM_E_NONE;
        }
        SOC_IF_ERROR_RETURN(soc_stable_used_get(unit, &stable_used_size));
        if ((stable_size - stable_used_size) < local_scache_size) {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Not enough scache memory left...\n")));
             _bcm_esw_stat_flex_cleanup(unit);
             return BCM_E_CONFIG;
        }
        rv = soc_scache_alloc(unit, handle,local_scache_size);
        if (!((rv== BCM_E_NONE) || (rv == BCM_E_EXISTS))) {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Seems to be some internal problem:.\n")));
             _bcm_esw_stat_flex_cleanup(unit);
             return rv;
        }
        SOC_IF_ERROR_RETURN(soc_scache_ptr_get(
                            unit,
                            handle,
                            (uint8 **)&flex_scache_ptr[unit],
                            &flex_scache_allocated_size));
    } else {
        rv = soc_scache_ptr_get(
                            unit, 
                            handle,
                            (uint8 **)&flex_scache_ptr[unit],
                            &flex_scache_allocated_size);
        /* You may get  BCM_E_NOT_FOUND for level 1 warm boot */
        if (rv == BCM_E_NOT_FOUND) {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Seems to be Level-1 Warm boot...continuing..\n")));
             LOG_CLI((BSL_META_U(unit,
                                 "Seems to be Level-1 Warm boot...continuing..\n")));
             return BCM_E_NONE;
        }
        if (rv != BCM_E_NONE) {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Seems to be some internal problem:.\n")));
             _bcm_esw_stat_flex_cleanup(unit);
             return rv;
        }
        if (_bcm_esw_stat_sync_version_above_equal(unit,BCM_WB_VERSION_1_3)) {
            local_scache_size += 
                              sizeof(bcm_stat_flex_group_mode_related_info_t) *
                              BCM_STAT_FLEX_COUNTER_MAX_MODE *
                              BCM_STAT_FLEX_COUNTER_MAX_DIRECTION ;
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "WarmBoot: Modified Local Scache Size %d\n"),
                       local_scache_size));
        }
        sal_memcpy(&local_scache_ptr[unit][0],
                   &flex_scache_ptr[unit][0],
                   BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE);
    }
    if (flex_scache_ptr[unit] == NULL) {
           _bcm_esw_stat_flex_cleanup(unit);
           return BCM_E_MEMORY;
    }
    if (flex_scache_allocated_size != local_scache_size) {
        _bcm_esw_stat_flex_cleanup(unit);
        return BCM_E_INTERNAL;
    }
    if (SOC_WARM_BOOT(unit)) { 
        BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_install_stat_id(
                                     unit,flex_scache_ptr[unit]));
    }
    /* soc_scache_commit(unit); */
#else
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "COLD Booting... \n")));
#endif
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "OK \n")));

    /* Just an Info */
    if (SOC_CONTROL(unit)->tableDmaMutex == NULL) { /* enabled */
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "WARNING:DMA will not be used for BulkMemoryReading\n")));
    } 
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_stat_flex_update_udf_selector_keys
 * Description:
 *      Update UDF selector keys
 * Parameters:
 *      unit                      - (IN) unit number
 *      direction                 - (IN) Flex Data Flow Direction
 *      pkt_attr_selector_key_reg - (IN) Flex Packet Attribute Seletor Key Reg
 *      udf_pkt_attr_selectors    - (IN) Flex Udf Packet Seletors
 *      total_udf_bits            - (OUT) Flex Udf Reserved bits
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      Currently not being used
 */
bcm_error_t _bcm_esw_stat_flex_update_udf_selector_keys(
            int                                    unit,
            bcm_stat_flex_direction_t              direction,
            soc_reg_t                              pkt_attr_selector_key_reg,
            bcm_stat_flex_udf_pkt_attr_selectors_t *udf_pkt_attr_selectors,
            uint32                                 *total_udf_bits)
{
    uint32 udf_valid_bits=0;
    uint8  udf_value_bit_position=0;
    uint8  key_bit_selector_position=0;
    uint16 udf_value=0;
    uint64 pkt_attr_selector_key_reg_value;
    uint32 index=0;

    COMPILER_64_ZERO(pkt_attr_selector_key_reg_value);

    (*total_udf_bits)=0;
    if (direction >= BCM_STAT_FLEX_COUNTER_MAX_DIRECTION) {
        return BCM_E_PARAM;
    }
    for(index=0; index < 8; index++) {
        if (_pkt_selector_key_reg[direction][index] == 
            pkt_attr_selector_key_reg ) {
            break;
        }
    }
    if (index == 8) {
         return BCM_E_PARAM;
    }

    /* Sanity Check: Get total number of udf bits */
    udf_value = udf_pkt_attr_selectors->udf_pkt_attr_bits.udf0;
    for (udf_value_bit_position=0;
         udf_value_bit_position<16;
         udf_value_bit_position++) {
         if (udf_value & 0x1) {
             (*total_udf_bits)++;
         }
         udf_value = udf_value >> 1;
    }
    if (*total_udf_bits > 8) {
        return BCM_E_PARAM;
    }
    udf_value = udf_pkt_attr_selectors->udf_pkt_attr_bits.udf1;
    for (udf_value_bit_position=0;
         udf_value_bit_position<16;
         udf_value_bit_position++) {
         if(udf_value & 0x1) {
            (*total_udf_bits)++;
         }
         udf_value = udf_value >> 1;
    }
    if ((*total_udf_bits) > 8) {
         return BCM_E_PARAM;
    }

    /*First Get complete value of ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_?r value */
    SOC_IF_ERROR_RETURN(soc_reg_get(    
                        unit,
                        pkt_attr_selector_key_reg,
                        REG_PORT_ANY,
                        0,
                        &pkt_attr_selector_key_reg_value));

    /* Next set field value for
       ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_?r:SELECTOR_KEY field*/
    soc_reg64_field32_set(unit, pkt_attr_selector_key_reg,
                          &pkt_attr_selector_key_reg_value, USE_UDF_KEYf, 1);
    if (udf_pkt_attr_selectors->udf_pkt_attr_bits.udf0 != 0) {
        udf_valid_bits |= 0x1;
    }
    if (udf_pkt_attr_selectors->udf_pkt_attr_bits.udf1 != 0) {
        udf_valid_bits |= 0x2;
    }
    soc_reg64_field32_set(unit, pkt_attr_selector_key_reg,
                          &pkt_attr_selector_key_reg_value, 
                          USER_SPECIFIED_UDF_VALIDf, udf_valid_bits);

    /* Now update selector keys */

    udf_value = udf_pkt_attr_selectors->udf_pkt_attr_bits.udf0;
    for (udf_value_bit_position=0;
         udf_value_bit_position<16;
         udf_value_bit_position++) {
         if (udf_value & 0x1) {
           BCM_IF_ERROR_RETURN(
               _bcm_esw_stat_flex_update_selector_keys_enable_fields(
                        unit,
                        pkt_attr_selector_key_reg,
                        &pkt_attr_selector_key_reg_value,
                        udf_value_bit_position+1,
                        1,     /* Total Bits */
                        0x1,     /* MASK */
                        &key_bit_selector_position));
         }
         udf_value = udf_value >> 1;
    }
    udf_value = udf_pkt_attr_selectors->udf_pkt_attr_bits.udf1;
    for (;udf_value_bit_position<32; udf_value_bit_position++) {
         if(udf_value & 0x1) {
            BCM_IF_ERROR_RETURN(
               _bcm_esw_stat_flex_update_selector_keys_enable_fields(
                        unit,
                        pkt_attr_selector_key_reg,
                        &pkt_attr_selector_key_reg_value,
                        udf_value_bit_position+1,
                        1,    /* Total Bits */
                        0x1,    /* MASK */
                        &key_bit_selector_position));
         }
         udf_value = udf_value >> 1;
    }
    /* Finally set value for ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_?r */
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, pkt_attr_selector_key_reg,
                                    REG_PORT_ANY, 0, 
                                    pkt_attr_selector_key_reg_value));
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_update_offset_table
 * Description:
 *      Update flex Offset table
 * Parameters:
 *      unit                      - (IN) unit number
 *      direction                 - (IN) Flex Data Flow Direction
 *      flex_ctr_offset_table_mem - (IN) Flex Offset Table
 *      mode                      - (IN) Flex Mode
 *      total_counters            - (IN) Flex Total Counters
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_update_offset_table(
            int                                unit,
            bcm_stat_flex_direction_t          direction,
            soc_mem_t                          flex_ctr_offset_table_mem,
            bcm_stat_flex_mode_t               mode,
            uint32                             total_counters,
            bcm_stat_flex_offset_table_entry_t offset_table_map[256])
{
    uint32                              index=0;
    uint32                              zero=0;
    uint32                              count_enable=1;
    uint32                              offset_value=0;
    ing_flex_ctr_offset_table_0_entry_t *offset_table_entry_v=NULL;
    uint32                              num_pools[
                                        BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    num_pools[bcmStatFlexDirectionIngress] = SOC_INFO(unit).
                                             num_flex_ingress_pools;
    num_pools[bcmStatFlexDirectionEgress]  = SOC_INFO(unit).
                                             num_flex_egress_pools;

    if (direction >= BCM_STAT_FLEX_COUNTER_MAX_DIRECTION) {
        return BCM_E_PARAM;
    }
    for(index=0; index < num_pools[direction]; index++) {
        if (_ctr_offset_table[direction][index] == flex_ctr_offset_table_mem ) {
            break;
        }
    }
    if (index == num_pools[direction]) {
         return BCM_E_PARAM;
    }
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "...Updating offset_table:%s:%d \n"),
               SOC_MEM_UFNAME(unit,flex_ctr_offset_table_mem),
               (int)sizeof(ing_flex_ctr_offset_table_0_entry_t)));
    offset_table_entry_v = soc_cm_salloc(unit,
                               sizeof(ing_flex_ctr_offset_table_0_entry_t)*256,
                               "offset_table_entry");
    if (offset_table_entry_v == NULL) {
        return BCM_E_MEMORY;
    }
    if (soc_mem_read_range(    unit,
                flex_ctr_offset_table_mem,
                MEM_BLOCK_ANY,
                (mode <<8),
                ((mode <<8)+256)-1,
                offset_table_entry_v ) != BCM_E_NONE){
        soc_cm_sfree(unit,offset_table_entry_v);
        return BCM_E_INTERNAL;
    }
    /*for(index=0;index<255;index++) */
    for (index=0;index<total_counters;index++) {
         /* Set OFFSETf */
         if (offset_table_map == NULL) {
             offset_value = index;
             count_enable = 1;
         } else {
             offset_value = offset_table_map[index].offset;
             count_enable = offset_table_map[index].count_enable;
         }
         soc_mem_field_set(
                 unit,
                 flex_ctr_offset_table_mem,
                 (uint32 *)&offset_table_entry_v[index],
                 OFFSETf,
                 &offset_value);
         /* Set COUNT_ENABLEf */
         soc_mem_field_set(
                 unit,
                 flex_ctr_offset_table_mem,
                 (uint32 *)&offset_table_entry_v[index],
                 COUNT_ENABLEf,
                 &count_enable);
    }
    /* Clear remaining entries */
    for (;index<256;index++) {
         /* Set OFFSETf=zero */
         soc_mem_field_set(
                 unit,
                 flex_ctr_offset_table_mem,
                 (uint32 *)&offset_table_entry_v[index],
                 OFFSETf,
                 &zero);
         /* Set COUNT_ENABLEf=zero */
         soc_mem_field_set(
                 unit,
                 flex_ctr_offset_table_mem,
                 (uint32 *)&offset_table_entry_v[index],
                 COUNT_ENABLEf,
                 &zero);
    }
    if (soc_mem_write_range(    unit,
                flex_ctr_offset_table_mem,
                MEM_BLOCK_ALL,
                (mode <<8),
                ((mode <<8)+256)-1,
                offset_table_entry_v ) != BCM_E_NONE){
        soc_cm_sfree(unit,offset_table_entry_v);
        return BCM_E_INTERNAL;
    }
    soc_cm_sfree(unit,offset_table_entry_v);
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_update_selector_keys_enable_fields
 * Description:
 *      Enable flex selector keys fields
 * Parameters:
 * unit                              - (IN) unit number
 * pkt_attr_selector_key_reg         - (IN) Flex Packet Attribute Selector
 * pkt_attr_selector_key_reg_value   - (IN) Flex Packet Attribute SelectorVal
 * ctr_pkt_attr_bit_position         - (IN) Flex Packet CounterAttr Bit Position
 * ctr_pkt_attr_total_bits           - (IN) Flex Packet Counter Attr Total Bits
 * pkt_attr_field_mask_v             - (IN) Flex Packet Attr Field Mask
 * ctr_current_bit_selector_position - (IN/OUT) Current BitSelector Position
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_update_selector_keys_enable_fields(
           int       unit,
           soc_reg_t pkt_attr_selector_key_reg,
           uint64    *pkt_attr_selector_key_reg_value,
           uint32    ctr_pkt_attr_bit_position,
           uint32    ctr_pkt_attr_total_bits,
           uint8     pkt_attr_field_mask_v,
           uint8     *ctr_current_bit_selector_position)
{
    uint32 index=0;
    uint8  field_mask=0;
    uint8  total_field_bits=0;
    uint8  field_index[8]={0};

    if ((*ctr_current_bit_selector_position) + ctr_pkt_attr_total_bits > 8) {
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Total bits exceeding 8 \n")));
         return BCM_E_INTERNAL;
    }
    field_mask = pkt_attr_field_mask_v;
    total_field_bits = 0;

    for (index=0;index < 8 ; index++) {
        if (field_mask & 0x1) {
            field_index[index] = index;
            total_field_bits++;
        }
        field_mask = field_mask >> 1;
    }
    if (total_field_bits != ctr_pkt_attr_total_bits ) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Total bits exceeding not matching with mask bits \n")));
        return BCM_E_INTERNAL;
    }

    for (index=0;index<ctr_pkt_attr_total_bits;index++) {
         soc_reg64_field32_set(
                   unit,
                   pkt_attr_selector_key_reg,
                   pkt_attr_selector_key_reg_value,
                   _pkt_selector_x_en_field_name[
                    *ctr_current_bit_selector_position],
                   1);
         soc_reg64_field32_set(
                   unit,
                   pkt_attr_selector_key_reg,
                   pkt_attr_selector_key_reg_value,
                   _pkt_selector_for_bit_x_field_name[
                    *ctr_current_bit_selector_position],
                   ctr_pkt_attr_bit_position+field_index[index]);
         /*ctr_pkt_attr_bit_position+index); */
         (*ctr_current_bit_selector_position) +=1;
    }
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_egress_reserve_mode
 * Description:
 *      Reserve flex egress mode
 * Parameters:
 *      unit                  - (IN) unit number
 *      mode                  - (IN) Flex Offset Mode
 *      total_counters        - (IN) Flex Total Counters for given mode
 *      egr_attr              - (IN) Flex Egress Attributes
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_egress_reserve_mode(
            int                      unit,
            bcm_stat_flex_mode_t     mode,
            uint32                   total_counters,
            bcm_stat_flex_egr_attr_t *egr_attr)
{
    if (mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   mode));
        return BCM_E_PARAM;
    }
    flex_egress_modes[unit][mode].available=1;
    flex_egress_modes[unit][mode].total_counters=total_counters;
    flex_egress_modes[unit][mode].egr_attr=*egr_attr;
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_ingress_reserve_mode
 * Description:
 *      Reserve flex ingress mode
 * Parameters:
 *      unit                  - (IN) unit number
 *      mode                  - (IN) Flex Offset Mode
 *      total_counters        - (IN) Flex Total Counters for given mode
 *      ing_attr              - (IN) Flex Ingress Attributes
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_ingress_reserve_mode(
            int                      unit,
            bcm_stat_flex_mode_t     mode,
            uint32                   total_counters,
            bcm_stat_flex_ing_attr_t *ing_attr)
{
    if (mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   mode));
        return BCM_E_PARAM;
    }
    flex_ingress_modes[unit][mode].available=1;
    flex_ingress_modes[unit][mode].total_counters=total_counters;
    flex_ingress_modes[unit][mode].ing_attr=*ing_attr;
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_unreserve_mode
 * Description:
 *      UnReserve flex (ingress/egress) mode
 * Parameters:
 *      unit                  - (IN) unit number
 *      direction             - (IN) Flex Data Flow Direction
 *      mode                  - (IN) Flex Offset Mode
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_unreserve_mode(
            int                       unit,
            bcm_stat_flex_direction_t direction,
            bcm_stat_flex_mode_t      mode)
{
    uint32                                    index=0;
    bcm_stat_flex_packet_attr_type_t          packet_attr_type;
    bcm_stat_flex_ing_cmprsd_attr_selectors_t *ing_cmprsd_attr_selectors= NULL;
    bcm_stat_flex_ing_pkt_attr_bits_t         ing_pkt_attr_bits={0};
    bcm_stat_flex_egr_cmprsd_attr_selectors_t *egr_cmprsd_attr_selectors= NULL;
    bcm_stat_flex_egr_pkt_attr_bits_t         egr_pkt_attr_bits={0};
    uint8                                     zero=0;
    uint64                                    pkt_attr_selector_key_reg_value;

    uint32 num_pools[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    COMPILER_64_ZERO(pkt_attr_selector_key_reg_value);

    num_pools[bcmStatFlexDirectionIngress] = SOC_INFO(unit).
                                             num_flex_ingress_pools;
    num_pools[bcmStatFlexDirectionEgress] = SOC_INFO(unit).
                                             num_flex_egress_pools;

    if (mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   mode));
        return BCM_E_PARAM;
    }
    switch(direction) {
    case bcmStatFlexDirectionIngress:
         if (flex_ingress_modes[unit][mode].available == 0) {
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "flex counter mode %d not configured yet\n"),
                        mode));
             return BCM_E_NOT_FOUND;
         }
         if (flex_ingress_modes[unit][mode].reference_count != 0) {
             /* not an error in case two or more counters are created before 
              * deleting one.
              */
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "FlexCounterMode:%d:IsBeingUsed.ReferenceCount:%d:\n"),
                        mode,flex_ingress_modes[unit][mode].reference_count));
             return BCM_E_INTERNAL;
         }
         /* Step2:  Reset selector keys */
         SOC_IF_ERROR_RETURN(soc_reg_set(
                             unit,
                             _pkt_selector_key_reg[bcmStatFlexDirectionIngress]
                             [mode],
                             REG_PORT_ANY,
                             0,
                             pkt_attr_selector_key_reg_value));
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
         if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) {
             SOC_IF_ERROR_RETURN(soc_reg_set(
                                 unit,
                                 _pkt_selector_key_reg[bcmStatFlexDirectionIngress]
                                 [mode+BCM_STAT_FLEX_COUNTER_MAX_MODE],
                                 REG_PORT_ANY,
                                 0,
                                 pkt_attr_selector_key_reg_value));
         }
#endif
         /* Step3: Reset Offset table filling */
         for (index=0;index<num_pools[bcmStatFlexDirectionIngress]; index++) {
              BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_update_offset_table(
                                  unit,
                                  bcmStatFlexDirectionIngress,
                                  _ctr_offset_table
                                    [bcmStatFlexDirectionIngress][index],
                                  mode,
                                  0,
                                  NULL));
         }
         /* Step4: Cleanup based on mode */
         packet_attr_type= flex_ingress_modes[unit][mode].
                            ing_attr.packet_attr_type;
         switch(packet_attr_type) {
         case bcmStatFlexPacketAttrTypeUncompressed:
              LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                        (BSL_META_U(unit,
                                    "\n Unreserving Ingress uncmprsd mode \n")));
              break;
         case bcmStatFlexPacketAttrTypeUdf:
              LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                        (BSL_META_U(unit,
                                    "\n Unreserving Ingress udf mode \n")));
              break;
         case bcmStatFlexPacketAttrTypeCompressed:
              ing_cmprsd_attr_selectors= &(flex_ingress_modes[unit][mode].
                                           ing_attr.cmprsd_attr_selectors);
              ing_pkt_attr_bits=ing_cmprsd_attr_selectors->pkt_attr_bits;
              zero=0;

              LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                        (BSL_META_U(unit,
                                    "\n Unreserving Ingress cmprsd mode \n")));

              /* Step3: Cleanup map array */
              if ((ing_pkt_attr_bits.cng != 0) ||
                  (ing_pkt_attr_bits.ifp_cng)  ||
                  (ing_pkt_attr_bits.int_pri)) {
                  for (index=0;
                       index< soc_mem_index_count(unit,ING_FLEX_CTR_PRI_CNG_MAPm);
                       index++) {
                       soc_mem_write(unit,ING_FLEX_CTR_PRI_CNG_MAPm,
                                     MEM_BLOCK_ALL, index, &zero);
                  }
              }
              if ((ing_pkt_attr_bits.vlan_format != 0) ||
                  (ing_pkt_attr_bits.outer_dot1p) ||
                  (ing_pkt_attr_bits.inner_dot1p)) {
                   for (index=0;
                        index< soc_mem_index_count(unit,ING_FLEX_CTR_PKT_PRI_MAPm);
                        index++) {
                        soc_mem_write(unit, ING_FLEX_CTR_PKT_PRI_MAPm,
                                      MEM_BLOCK_ALL, index, &zero);
                   }
              }
              if (ing_pkt_attr_bits.ing_port != 0) {
                  for (index=0;
                       index< soc_mem_index_count(unit,ING_FLEX_CTR_PORT_MAPm);
                       index++) {
                   soc_mem_write(unit, ING_FLEX_CTR_PORT_MAPm,
                                MEM_BLOCK_ALL, index, &zero);
                  }
              }
              if ((ing_pkt_attr_bits.tos_dscp != 0) ||
                  (ing_pkt_attr_bits.tos_ecn != 0)) {
                  for (index=0;
                       index< soc_mem_index_count(unit,ING_FLEX_CTR_TOS_MAPm);
                       index++) {
                       soc_mem_write(unit, ING_FLEX_CTR_TOS_MAPm,
                                     MEM_BLOCK_ALL, index, &zero);
                  }
              }
              if ((ing_pkt_attr_bits.pkt_resolution != 0) ||
                  (ing_pkt_attr_bits.svp_type) ||
                  (ing_pkt_attr_bits.drop)) {
                   for (index=0;
                        index< soc_mem_index_count(unit,ING_FLEX_CTR_PKT_RES_MAPm);
                        index++) {
                        soc_mem_write(unit, ING_FLEX_CTR_PKT_RES_MAPm,
                                      MEM_BLOCK_ALL, index, &zero);
                   }
              }
              break;
         }
         flex_ingress_modes[unit][mode].available=0;
         flex_ingress_modes[unit][mode].total_counters=0;
         if (flex_ingress_modes[unit][mode].attr_selectors  != NULL) {
             sal_free(flex_ingress_modes[unit][mode].attr_selectors);
             flex_ingress_modes[unit][mode].flags = 0;
             flex_ingress_modes[unit][mode].num_selectors = 0;
             flex_ingress_modes[unit][mode].attr_selectors = NULL;
             LOG_CLI((BSL_META_U(unit,
                                 "Ingress: Freed Attr Selectors \n")));
         }
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "\n Done \n")));
         break;
    case bcmStatFlexDirectionEgress:
         if (flex_egress_modes[unit][mode].available==0) {
             return BCM_E_NOT_FOUND;
         }
         if (flex_egress_modes[unit][mode].reference_count != 0) {
             return BCM_E_INTERNAL;
         }
         /* Step2:  Reset selector keys */
         SOC_IF_ERROR_RETURN(soc_reg_set(unit,
                             _pkt_selector_key_reg[bcmStatFlexDirectionEgress]
                                                  [mode],
                             REG_PORT_ANY,
                             0,
                             pkt_attr_selector_key_reg_value));
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
         if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) {
             SOC_IF_ERROR_RETURN(soc_reg_set(unit,
                                 _pkt_selector_key_reg[bcmStatFlexDirectionEgress]
                                 [mode+BCM_STAT_FLEX_COUNTER_MAX_MODE],
                                 REG_PORT_ANY,
                                 0,
                                 pkt_attr_selector_key_reg_value));
          }
#endif
         /* Step3: Reset Offset table filling */
         for (index=0; index < num_pools[bcmStatFlexDirectionEgress]; index++) {
              BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_update_offset_table(
                                   unit,bcmStatFlexDirectionEgress,
                                   _ctr_offset_table[bcmStatFlexDirectionEgress]
                                                    [index],
                                   mode,
                                   0,
                                   NULL));
         }
         /* Step4: Cleanup based on mode */
         packet_attr_type = flex_egress_modes[unit][mode].egr_attr.
                            packet_attr_type;
         switch(packet_attr_type) {
         case bcmStatFlexPacketAttrTypeUncompressed:
              LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                        (BSL_META_U(unit,
                                    "\nUnreserving Egress uncmprsd mode \n")));
              break;
         case bcmStatFlexPacketAttrTypeUdf:
              LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                        (BSL_META_U(unit,
                                    "\n Unreserving Egress UDF mode \n")));
              break;
         case bcmStatFlexPacketAttrTypeCompressed:
              egr_cmprsd_attr_selectors= &(flex_egress_modes[unit][mode].
                                           egr_attr.cmprsd_attr_selectors);
              egr_pkt_attr_bits= egr_cmprsd_attr_selectors->pkt_attr_bits;
              LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                        (BSL_META_U(unit,
                                    "\n Unreserving Egress cmprsd mode \n")));

              /* Step3: Cleanup map array */
              if ((egr_pkt_attr_bits.cng != 0) ||
                  (egr_pkt_attr_bits.int_pri)) {
                   for(index=0;
                       index< soc_mem_index_count(unit,EGR_FLEX_CTR_PRI_CNG_MAPm);
                       index++) {
                       soc_mem_write(unit, EGR_FLEX_CTR_PRI_CNG_MAPm,
                                     MEM_BLOCK_ALL, index, &zero);
                   }
              }
              if ((egr_pkt_attr_bits.vlan_format != 0) ||
                  (egr_pkt_attr_bits.outer_dot1p) ||
                  (egr_pkt_attr_bits.inner_dot1p)) {
                   for(index=0;
                       index< soc_mem_index_count(unit,EGR_FLEX_CTR_PKT_PRI_MAPm);
                       index++) {
                       soc_mem_write(unit, EGR_FLEX_CTR_PKT_PRI_MAPm,
                                     MEM_BLOCK_ALL, index, &zero);
                   }
              }
              if (egr_pkt_attr_bits.egr_port != 0) {
                  for(index=0;
                      index< soc_mem_index_count(unit,EGR_FLEX_CTR_PORT_MAPm);
                      index++) {
                      soc_mem_write(unit, EGR_FLEX_CTR_PORT_MAPm,
                                    MEM_BLOCK_ALL, index, &zero);
                  }
              }
              if ((egr_pkt_attr_bits.tos_dscp != 0) ||
                  (egr_pkt_attr_bits.tos_ecn != 0)) {
                  for(index=0;
                      index< soc_mem_index_count(unit,EGR_FLEX_CTR_TOS_MAPm);
                      index++) {
                            soc_mem_write(
                                unit,
                                EGR_FLEX_CTR_TOS_MAPm,
                                MEM_BLOCK_ALL,
                                index,
                                &zero);
                           }
                        }
              if ((egr_pkt_attr_bits.pkt_resolution != 0) ||
                  (egr_pkt_attr_bits.svp_type) ||
                  (egr_pkt_attr_bits.dvp_type) ||
                  (egr_pkt_attr_bits.drop)) {
                   for(index=0;
                       index< soc_mem_index_count(unit,EGR_FLEX_CTR_PKT_RES_MAPm);
                       index++) {
                        soc_mem_write(unit, EGR_FLEX_CTR_PKT_RES_MAPm,
                                      MEM_BLOCK_ALL, index, &zero);
                   }
              }
              break;
         }
         flex_egress_modes[unit][mode].available=0;
         flex_egress_modes[unit][mode].total_counters=0;
         if (flex_egress_modes[unit][mode].attr_selectors  != NULL) {
             sal_free(flex_egress_modes[unit][mode].attr_selectors);
             flex_egress_modes[unit][mode].flags = 0;
             flex_egress_modes[unit][mode].num_selectors = 0;
             flex_egress_modes[unit][mode].attr_selectors = NULL;
             LOG_CLI((BSL_META_U(unit,
                                 "Egress: Freed Attr Selectors \n")));
         }
         LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "\n Done \n")));
         break;
    default:
         return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}
bcm_error_t _bcm_esw_stat_flex_update_ingress_flex_info(
            int                                 unit,
            bcm_stat_flex_mode_t                mode,
            uint32                              flags,
            uint32                              num_selectors,
            bcm_stat_group_mode_attr_selector_t *attr_selectors)
{
    if (mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   mode));
        return BCM_E_PARAM;
    }
    if (flex_ingress_modes[unit][mode].available==0) {
#if 0
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "flex counter mode %d not configured yet\n"),
                   mode));
#endif
        return BCM_E_NOT_FOUND;
    }
    flex_ingress_modes[unit][mode].flags = flags;
    flex_ingress_modes[unit][mode].num_selectors = num_selectors;
    if ((flex_ingress_modes[unit][mode].attr_selectors = 
         sal_alloc(num_selectors*sizeof(bcm_stat_group_mode_attr_selector_t),
                  "attr_selectors")) == NULL) {
         LOG_CLI((BSL_META_U(unit,
                             "Memory allocation failed \n")));
         return BCM_E_MEMORY;
    }
    sal_memcpy(flex_ingress_modes[unit][mode].attr_selectors,
               attr_selectors, 
               num_selectors * sizeof(bcm_stat_group_mode_attr_selector_t));
    return BCM_E_NONE;
}
bcm_error_t _bcm_esw_stat_flex_update_egress_flex_info(
            int                                 unit,
            bcm_stat_flex_mode_t                mode,
            uint32                              flags,
            uint32                              num_selectors,
            bcm_stat_group_mode_attr_selector_t *attr_selectors)
{
    if (mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   mode));
        return BCM_E_PARAM;
    }
    if (flex_egress_modes[unit][mode].available==0) {
#if 0
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "flex counter mode %d not configured yet\n"),
                   mode));
#endif
        return BCM_E_NOT_FOUND;
    }
    flex_egress_modes[unit][mode].flags = flags;
    flex_egress_modes[unit][mode].num_selectors = num_selectors;
    if ((flex_egress_modes[unit][mode].attr_selectors = 
         sal_alloc(num_selectors*sizeof(bcm_stat_group_mode_attr_selector_t),
                  "attr_selectors")) == NULL) {
         LOG_CLI((BSL_META_U(unit,
                             "Memory allocation failed \n")));
         return BCM_E_MEMORY;
    }
    sal_memcpy(flex_egress_modes[unit][mode].attr_selectors,
               attr_selectors, 
               num_selectors * sizeof(bcm_stat_group_mode_attr_selector_t));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_stat_flex_get_ingress_mode_info
 * Description:
 *      Get Ingress Mode related information
 * Parameters:
 *      unit                  - (IN) unit number
 *      mode                  - (IN) Flex Offset Mode
 *      flex_ingress_mode     - (OUT) Flex Ingress Mode Info
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_get_ingress_mode_info(
            int                          unit,
            bcm_stat_flex_mode_t         mode,
            bcm_stat_flex_ingress_mode_t *flex_ingress_mode)
{
    if (mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   mode));
        return BCM_E_PARAM;
    }
    if (flex_ingress_modes[unit][mode].available==0) {
#if 0
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "flex counter mode %d not configured yet\n"),
                   mode));
#endif
        return BCM_E_NOT_FOUND;
    }
    *flex_ingress_mode= flex_ingress_modes[unit][mode];
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_get_egress_mode_info
 * Description:
 *      Get Egress Mode related information
 * Parameters:
 *      unit                  - (IN) unit number
 *      mode                  - (IN) Flex Offset Mode
 *      flex_egress_mode      - (OUT) Flex Egress Mode Info
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_get_egress_mode_info(
            int                         unit,
            bcm_stat_flex_mode_t        mode,
            bcm_stat_flex_egress_mode_t *flex_egress_mode)
{
    if (mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   mode));
        return BCM_E_PARAM;
    }
    if (flex_egress_modes[unit][mode].available==0) {
#if 0
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "flex counter mode %d not configured yet\n"),
                   mode));
#endif
        return BCM_E_NOT_FOUND;
    }
    *flex_egress_mode = flex_egress_modes[unit][mode];
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_get_available_mode
 * Description:
 *      Get Free available Offset Mode
 * Parameters:
 *      unit                  - (IN) unit number
 *      direction             - (IN) Flex Data Flow Direction
 *      mode                  - (OUT) Flex Offset Mode
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_get_available_mode(
            int                       unit,
            bcm_stat_flex_direction_t direction,
            bcm_stat_flex_mode_t      *mode)
{
    uint32 mode_index=1;
    switch(direction) {
    case bcmStatFlexDirectionIngress:
         for (mode_index=0;
              mode_index < BCM_STAT_FLEX_COUNTER_MAX_MODE ;
              mode_index++) {
              if (flex_ingress_modes[unit][mode_index].available==0) {
                  *mode=mode_index;
                  return BCM_E_NONE;
              }
         }
         break;
    case bcmStatFlexDirectionEgress:
         for (mode_index=0;
              mode_index < BCM_STAT_FLEX_COUNTER_MAX_MODE ;
              mode_index++) {
              if (flex_egress_modes[unit][mode_index].available==0) {
                  *mode=mode_index;
                  return BCM_E_NONE;
              }
         }
         break;
    default: 
         return BCM_E_PARAM;
    }
    return BCM_E_FULL;
}
/*
 * Function:
 *      _bcm_esw_stat_counter_set
 * Description:
 *      Set Flex Counter Values
 * Parameters:
 *      unit                  - (IN) unit number
 *      index                 - (IN) Flex Accounting Table Index
 *      table                 - (IN) Flex Accounting Table 
 *      byte_flag             - (IN) Byte/Packet Flag
 *      counter               - (IN) Counter Index
 *      values                - (IN) Counter Value
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_counter_set(
            int              unit,
            uint32           index,
            soc_mem_t        table,
            uint32           byte_flag,
            uint32           counter_index, 
            bcm_stat_value_t *value)
{
    soc_mem_t                 mem;
#if defined(BCM_TRIDENT2_SUPPORT)
    soc_mem_t                 mem_dual = 0;
    int                       buffer_id = 0;
#endif /* BCM_TRIDENT2_SUPPORT*/
    uint32                    offset_mode=0;
    uint32                    pool_number=0;
    uint32                    base_idx=0;
    bcm_stat_flex_direction_t direction;
    uint32                    total_entries=0;
    uint32                    offset_index=0;
    uint32                    zero=0;
    uint32                    one=1;
    uint32                    entry_data_size=0;
    void                      *entry_data=NULL;
    uint32                    max_packet_mask=0;
    uint64                    max_byte_mask;
    uint32                    hw_val[2];

    COMPILER_64_ZERO(max_byte_mask);


    switch(table) {
    case PORT_TABm:
    case VLAN_XLATEm:
#if defined(BCM_TRIUMPH3_SUPPORT)
    case VLAN_XLATE_EXTDm:
    case MPLS_ENTRY_EXTDm:
    case L3_TUNNELm:
    case L3_ENTRY_2m:
    case L3_ENTRY_4m:
    case EXT_IPV4_UCAST_WIDEm:
    case EXT_IPV6_128_UCAST_WIDEm:
    case EXT_FP_POLICYm:
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    case ING_VSANm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
    case L3_DEFIPm:
    case L3_DEFIP_PAIR_128m:
    case L3_DEFIP_ALPM_IPV4_1m:
    case L3_DEFIP_ALPM_IPV6_64_1m:
    case L3_DEFIP_ALPM_IPV6_128m:
    case L3_DEFIP_AUX_SCRATCHm:
#endif
    case VFP_POLICY_TABLEm:
    case MPLS_ENTRYm:
    case SOURCE_VPm:
    case L3_IIFm:
    case VRFm:
    case VFIm:
    case VLAN_TABm:
         direction= bcmStatFlexDirectionIngress;
         break;
    case EGR_VLANm:
    case EGR_VFIm:
    case EGR_L3_NEXT_HOPm:
    case EGR_VLAN_XLATEm:
    case EGR_PORTm:
#if defined(BCM_TRIUMPH3_SUPPORT)
    case EGR_DVP_ATTRIBUTE_1m:
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    case EGR_NAT_PACKET_EDIT_INFOm:
#endif
         direction= bcmStatFlexDirectionEgress;
         break;
    default:
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Invalid Flex Counter Memory %s\n"),
                    SOC_MEM_UFNAME(unit, table)));
         return BCM_E_PARAM;
    }
    mem = _ctr_counter_table[direction][0];
    COMPILER_64_SET(max_byte_mask, zero, one);
    COMPILER_64_SHL(max_byte_mask,soc_mem_field_length(unit,mem,BYTE_COUNTERf));
    COMPILER_64_SUB_32(max_byte_mask,one);

    max_packet_mask = (1 << soc_mem_field_length(unit,mem,PACKET_COUNTERf));
    max_packet_mask -= 1;

    entry_data_size = WORDS2BYTES(BYTES2WORDS(SOC_MEM_INFO(unit,table).bytes));
    entry_data = sal_alloc(entry_data_size,"flex-counter-table");
    if (entry_data == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Failed to allocate memory for Table:%s "),
                   SOC_MEM_UFNAME(unit, table)));
        return BCM_E_INTERNAL;
    }
    if (flex_temp_counter[unit][direction] == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Unit %d Not initilized or attached yet\n"),
                   unit));
        sal_free(entry_data);
        return BCM_E_CONFIG;
    }
    sal_memset(entry_data,0,entry_data_size);

    if (soc_mem_read(unit,table,MEM_BLOCK_ANY,
                _bcm_esw_stat_flex_table_index_map(unit,table,index),
                entry_data) == SOC_E_NONE) {
        if (soc_mem_field_valid(unit,table,VALIDf)) {
            if (soc_mem_field32_get(unit,table,entry_data,VALIDf)==0) {
                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "Table %s  with index %d is Not valid \n"),
                           SOC_MEM_UFNAME(unit, table),index));
                sal_free(entry_data);
                return BCM_E_PARAM;
            }
        }
        _bcm_esw_get_flex_counter_fields_values(
                 unit,index,table,entry_data, &offset_mode,&pool_number,&base_idx);
        if ((offset_mode == 0) && (base_idx == 0)) {
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Table:%s:Index:%d:IsNotConfiguredForFlexCounter \n"),
                        SOC_MEM_UFNAME(unit, table),index));
             sal_free(entry_data);
             /* Either Not configured or deallocated before */
             return BCM_E_NOT_FOUND;
        }
        if (direction == bcmStatFlexDirectionIngress) {
            total_entries= flex_ingress_modes[unit][offset_mode].total_counters;
        } else {
            total_entries = flex_egress_modes[unit][offset_mode].total_counters;
        }
        offset_index = counter_index;
        if (offset_index >= total_entries) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Wrong OFFSET_INDEX. Must be < Total Counters %d \n"),
                       total_entries));
            return BCM_E_PARAM;
        }
        BCM_STAT_FLEX_COUNTER_LOCK(unit);
#if defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TD2_TT2(unit)) {
            mem = _ctr_counter_table_x[direction][pool_number];
            mem_dual = _ctr_counter_table_y[direction][pool_number];
        } else
#endif
        {
            mem = _ctr_counter_table[direction][pool_number];
        }
        if (soc_mem_read(
                    unit,
                    mem,
                    MEM_BLOCK_ANY,
                    base_idx+offset_index,
                    flex_temp_counter[unit][direction]) != BCM_E_NONE){
            BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
            sal_free(entry_data);
            return BCM_E_INTERNAL;
        }
#if defined(BCM_TRIDENT2_SUPPORT)
        if (mem_dual) {
            if (soc_mem_read(
                    unit,
                    mem_dual,
                    MEM_BLOCK_ANY,
                    base_idx+offset_index,
                    flex_temp_counter_dual[unit][direction]) != BCM_E_NONE) {
                BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
                sal_free(entry_data);
                return BCM_E_INTERNAL;
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT*/

        if (byte_flag == 1) {
            /* Mask to possible max values */
            COMPILER_64_AND(value->bytes, max_byte_mask);
            /* Update Soft Copy */
            COMPILER_64_SET(flex_byte_counter[unit][direction][pool_number]
                                             [base_idx+offset_index],
                            COMPILER_64_HI(value->bytes),
                            COMPILER_64_LO(value->bytes));
            /* Change Read Hw Copy */
            hw_val[0] = COMPILER_64_LO(value->bytes);
            hw_val[1] = COMPILER_64_HI(value->bytes);
            soc_mem_field_set(
                    unit,
                    mem,
                    (uint32 *)&flex_temp_counter[unit][direction][0],
                    BYTE_COUNTERf,
                    hw_val);
#if defined(BCM_TRIDENT2_SUPPORT)
            if (mem_dual) {
                /* assume total counter value are set in X pipe hardware counter
                 * table. Clear the Y pipe table
                 */
                for (buffer_id = 0; buffer_id < _BCM_FLEX_STAT_BUFFS;
                     buffer_id++) {
                    COMPILER_64_SET(flex_byte_counter_x[unit][direction]
                                    [pool_number][buffer_id]
                                    [base_idx+offset_index],
                                    COMPILER_64_HI(value->bytes),
                                    COMPILER_64_LO(value->bytes));
                    COMPILER_64_ZERO(flex_byte_counter_y[unit][direction]
                                     [pool_number][buffer_id]
                                     [base_idx+offset_index]);
                }
                hw_val[0] = 0;
                hw_val[1] = 0;
                soc_mem_field_set(
                    unit,
                    mem_dual,
                    (uint32 *)&flex_temp_counter_dual[unit][direction][0],
                    BYTE_COUNTERf,
                    hw_val);
            }
#endif /* BCM_TRIDENT2_SUPPORT*/
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Byte Count Value\t:TABLE:%sINDEX:%d "
                                   "(@Pool:%dDirection:%dActualOffset%d) : %x:%x \n"),
                       SOC_MEM_UFNAME(unit, table),
                       index,
                       pool_number,
                       direction,
                       base_idx+offset_index,
                       COMPILER_64_HI(value->bytes),
                       COMPILER_64_LO(value->bytes)));
        } else {
            value->packets &= max_packet_mask;
            /* Update Soft Copy */
            flex_packet_counter[unit][direction][pool_number]
                               [base_idx+offset_index] = value->packets;

            /* Update Soft Copy */
            COMPILER_64_SET(flex_packet64_counter[unit][direction][pool_number]
                                             [base_idx+offset_index],0,
                             value->packets);
            /* Change Read Hw Copy */
            soc_mem_field_set(
                    unit,
                    mem,
                    (uint32 *)&flex_temp_counter[unit][direction][0],
                    PACKET_COUNTERf,
                    &(value->packets));
#if defined(BCM_TRIDENT2_SUPPORT)
            if (mem_dual) {
                for (buffer_id = 0; buffer_id < _BCM_FLEX_STAT_BUFFS;
                     buffer_id++) {
                    flex_pkt_counter_x[unit][direction][pool_number]
                                      [buffer_id][base_idx+offset_index] 
                                      = value->packets;
                    flex_pkt_counter_y[unit][direction][pool_number]
                                      [buffer_id][base_idx+offset_index]
                                      = 0;
                }
                hw_val[0] = 0;
                soc_mem_field_set(
                    unit,
                    mem_dual,
                    (uint32 *)&flex_temp_counter_dual[unit][direction][0],
                    PACKET_COUNTERf,
                    &hw_val[0]);
            }
#endif /* BCM_TRIDENT2_SUPPORT*/
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Packet Count Value\t:TABLE:%sINDEX:%d "
                                   "(@Pool:%dDirection:%dActualOffset%d) : %x \n"),
                       SOC_MEM_UFNAME(unit, table),
                       index,
                       pool_number,
                       direction,
                       base_idx+offset_index,
                       value->packets));
        }
        /* Update Hw Copy */
        if (soc_mem_write(unit,mem,MEM_BLOCK_ALL,
                    base_idx+offset_index,
                    &(flex_temp_counter[unit][direction][0])) != BCM_E_NONE){
            BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
            sal_free(entry_data);
            return BCM_E_INTERNAL;
        }
#if defined(BCM_TRIDENT2_SUPPORT)
        if (mem_dual) {
            if (soc_mem_write(unit,mem_dual,MEM_BLOCK_ALL,
                    base_idx+offset_index,
                    &(flex_temp_counter_dual[unit][direction][0])) != BCM_E_NONE){
            BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
            sal_free(entry_data);
            return BCM_E_INTERNAL;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT*/
        BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
        sal_free(entry_data);
        return BCM_E_NONE;
    }
    sal_free(entry_data);
    return BCM_E_FAIL;
}
/*
 * Function:
 *      _bcm_esw_stat_counter_get
 * Description:
 *      Get Flex Counter Values
 *      if sync_mode is set, sync the sw accumulated count
 *      with hw count value first, else return sw count.  
 *
 * Parameters:
 *      unit                  - (IN) unit number
 *      sync_mode             - (IN) hwcount is to be synced to sw count 
 *      index                 - (IN) Flex Accounting Table Index
 *      table                 - (IN) Flex Accounting Table 
 *      byte_flag             - (IN) Byte/Packet Flag
 *      counter               - (IN) Counter Index
 *      values                - (OUT) Counter Value
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_counter_get(
            int              unit,
            int              sync_mode,  
            uint32           index,
            soc_mem_t        table,
            uint32           byte_flag,
            uint32           counter_index, 
            bcm_stat_value_t *value)
{
    uint32                    loop=0;
    uint32                    offset_mode=0;
    uint32                    pool_number=0;
    uint32                    base_idx=0;
    bcm_stat_flex_direction_t direction;
    uint32                    total_entries=0;
    uint32                    offset_index=0;
    uint32                    entry_data_size=0;
    void                      *entry_data=NULL;

    switch(table) {
    case PORT_TABm:
    case VLAN_XLATEm:
#if defined(BCM_TRIUMPH3_SUPPORT)
    case VLAN_XLATE_EXTDm:
    case MPLS_ENTRY_EXTDm:
    case L3_TUNNELm:
    case L3_ENTRY_2m:
    case L3_ENTRY_4m:
    case EXT_IPV4_UCAST_WIDEm:
    case EXT_IPV6_128_UCAST_WIDEm:
    case EXT_FP_POLICYm:
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    case ING_VSANm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
    case L3_DEFIPm:
    case L3_DEFIP_PAIR_128m:
    case L3_DEFIP_ALPM_IPV4_1m:
    case L3_DEFIP_ALPM_IPV6_64_1m:
    case L3_DEFIP_ALPM_IPV6_128m:
    case L3_DEFIP_AUX_SCRATCHm:
#endif
    case VFP_POLICY_TABLEm:
    case MPLS_ENTRYm:
    case SOURCE_VPm:
    case L3_IIFm:
    case VRFm:
    case VFIm:
    case VLAN_TABm:
         direction= bcmStatFlexDirectionIngress;
         break;
    case EGR_VLANm:
    case EGR_VFIm:
    case EGR_L3_NEXT_HOPm:
    case EGR_VLAN_XLATEm:
    case EGR_PORTm:
#if defined(BCM_TRIUMPH3_SUPPORT)
    case EGR_DVP_ATTRIBUTE_1m:
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    case EGR_NAT_PACKET_EDIT_INFOm:
#endif
         direction= bcmStatFlexDirectionEgress;
         break;
    default:
         LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                     (BSL_META_U(unit,
                                 "Invalid Flex Counter Memory %s\n"),
                      SOC_MEM_UFNAME(unit, table)));
         return BCM_E_PARAM;
    }
    entry_data_size = WORDS2BYTES(BYTES2WORDS(SOC_MEM_INFO(unit,table).bytes));
    entry_data = sal_alloc(entry_data_size,"flex-counter-table");
    if (entry_data == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Failed to allocate memory for Table:%s "),
                   SOC_MEM_UFNAME(unit, table)));
        return BCM_E_INTERNAL;
    }
    sal_memset(entry_data,0,entry_data_size);

    if (soc_mem_read(unit,table,MEM_BLOCK_ANY,
                _bcm_esw_stat_flex_table_index_map(unit,table,index),
                entry_data) == SOC_E_NONE) {
        if (soc_mem_field_valid(unit,table,VALIDf)) {
            if (soc_mem_field32_get(unit,table,entry_data,VALIDf)==0) {
                LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                            (BSL_META_U(unit,
                                        "Table %s  with index %d is Not valid \n"),
                             SOC_MEM_UFNAME(unit, table),index));
                sal_free(entry_data);
                return BCM_E_PARAM;
            }
        }
        _bcm_esw_get_flex_counter_fields_values(
                 unit,
                 index,
                 table,
                 entry_data,
                 &offset_mode,
                 &pool_number,
                 &base_idx);
        if ((offset_mode == 0) && (base_idx == 0)) {
             LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "Table:%s:Index:%d:is NotConfiguredForFlexCounter \n"),
                          SOC_MEM_UFNAME(unit, table),index));
             sal_free(entry_data);
             /* Either Not configured or deallocated before */
             return BCM_E_NOT_FOUND;
         }
         if (direction == bcmStatFlexDirectionIngress) {
             total_entries = flex_ingress_modes[unit][offset_mode].
                             total_counters;
         } else {
             total_entries = flex_egress_modes[unit][offset_mode].
                             total_counters;
         }
         offset_index = counter_index;
         if (offset_index >= total_entries) {
             LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "Invalid offset_index[%d]. total_entries[%d]\n"),
                          offset_index, total_entries));
             return BCM_E_PARAM;
         }
         BCM_STAT_FLEX_COUNTER_LOCK(unit);
         
         /* sync the software counter with hardware counter */
         if (sync_mode == 1) {
             _bcm_esw_stat_flex_counter_collect(unit, direction, 
                                                pool_number, 
                                                base_idx + offset_index + loop);
         }

         bcm_stat_value_t_init(value);
         if (byte_flag == 1) {
             COMPILER_64_SET(value->bytes,
                      COMPILER_64_HI(flex_byte_counter[unit]
                                     [direction][pool_number]
                                     [base_idx+offset_index+loop]),
                      COMPILER_64_LO(flex_byte_counter[unit]
                                     [direction][pool_number]
                                     [base_idx+offset_index+loop]));
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Byte Count Value\t:TABLE:%sINDEX:%d COUTER-%d"
                                    "(@Pool:%dDirection:%dActualOffset%d) : %x:%x \n"),
                        SOC_MEM_UFNAME(unit, table),
                        index,
                        loop,
                        pool_number,
                        direction,
                        base_idx+offset_index+loop,
                        COMPILER_64_HI(value->bytes),
                        COMPILER_64_LO(value->bytes)));
         } else {
             value->packets= flex_packet_counter[unit][direction][pool_number]
                                                [base_idx+offset_index+loop];
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Packet Count Value\t:TABLE:%sINDEX:%d COUTER-%d"
                                    "(@Pool:%dDirection:%dActualOffset%d) : %x \n"),
                        SOC_MEM_UFNAME(unit, table),
                        index,
                        loop, 
                        pool_number,
                        direction,
                        base_idx+offset_index+loop,
                        value->packets));
             COMPILER_64_SET(value->packets64,
                      COMPILER_64_HI(flex_packet64_counter[unit]
                                     [direction][pool_number]
                                     [base_idx+offset_index+loop]),
                      COMPILER_64_LO(flex_packet64_counter[unit]
                                     [direction][pool_number]
                                     [base_idx+offset_index+loop]));
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Packet64 Count Value\t:TABLE:%sINDEX:%d COUNTER-%d"
                                    "(@Pool:%dDirection:%dActualOffset%d) : %x:%x \n"),
                        SOC_MEM_UFNAME(unit, table),
                        index,
                        loop,
                        pool_number,
                        direction,
                        base_idx+offset_index+loop,
                        COMPILER_64_HI(value->packets64),
                        COMPILER_64_LO(value->packets64)));
 
         }
         BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
         sal_free(entry_data);
         return BCM_E_NONE;
    }
    sal_free(entry_data);
    return BCM_E_FAIL;
}
/*
 * Function:
 *      _bcm_esw_stat_counter_raw_get
 * Description:
 *      Get Flex Counter Values
 * Parameters:
 *      unit                  - (IN) unit number
 *      stat_counter_id       - (IN) Stat Counter Id
 *      byte_flag             - (IN) Byte/Packet Flag
 *      counter               - (IN) Counter Index
 *      values                - (OUT) Counter Value
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes: Must be called carefully.
 *        Preference should be given to _bcm_esw_stat_counter_get()
 *
 */
bcm_error_t _bcm_esw_stat_counter_raw_get(
            int              unit,
            int              sync_mode,
            uint32           stat_counter_id,
            uint32           byte_flag,
            uint32           counter_index, 
            bcm_stat_value_t *value)
{
    uint32                    offset_mode=0;
    uint32                    pool_number=0;
    uint32                    base_idx=0;
    uint32                    total_entries=0;
    uint32                    offset_index=0;
    bcm_stat_flex_direction_t direction=bcmStatFlexDirectionIngress;
    bcm_stat_object_t         object=bcmStatObjectIngPort;
    bcm_stat_group_mode_t     group_mode= bcmStatGroupModeSingle;

    _bcm_esw_stat_get_counter_id_info(stat_counter_id,&group_mode,&object,
                                      &offset_mode,&pool_number,&base_idx);
    /* Validate object id first */
    if (_bcm_esw_stat_validate_object(unit,object,&direction) != BCM_E_NONE) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid bcm_stat_object_t passed %d \n"),
                   object));
        return BCM_E_PARAM;
    }
    /* Validate group_mode */
    if (_bcm_esw_stat_validate_group(unit,group_mode) != BCM_E_NONE) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid bcm_stat_group_mode_t passed %d \n"),
                   group_mode));
        return BCM_E_PARAM;
    }
    if (direction == bcmStatFlexDirectionIngress) {
        total_entries = flex_ingress_modes[unit][offset_mode].
                                          total_counters;
    } else {
        total_entries = flex_egress_modes[unit][offset_mode].
                                         total_counters;
    }
    if (flex_base_index_reference_count[unit][direction]
                                       [pool_number][base_idx] == 0) {
        return BCM_E_PARAM;
    }
    offset_index = counter_index;
    if (offset_index >= total_entries) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Wrong OFFSET_INDEX.Must be < Total Counters %d \n"),
                   total_entries));
        return BCM_E_PARAM;
    }
    BCM_STAT_FLEX_COUNTER_LOCK(unit);

    /* sync the software counter with hardware counter */
    if (sync_mode == 1) {
            _bcm_esw_stat_flex_counter_collect(unit, direction,
                                               pool_number, base_idx + offset_index);
    }
    if (byte_flag == 1) {
        COMPILER_64_SET(value->bytes,
                 COMPILER_64_HI(flex_byte_counter[unit]
                                [direction][pool_number]
                                [base_idx+offset_index]),
                 COMPILER_64_LO(flex_byte_counter[unit]
                                [direction][pool_number]
                                [base_idx+offset_index]));
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Byte Count Value\t:COUTER-%d"
                               "(@Pool:%dDirection:%dActualOffset%d) : %x:%x \n"),
                   offset_index,
                   pool_number,
                   direction,
                   base_idx+offset_index,
                   COMPILER_64_HI(value->bytes),
                   COMPILER_64_LO(value->bytes)));
    } else {
        value->packets= flex_packet_counter[unit][direction][pool_number]
                                           [base_idx+offset_index];
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Packet Count Value\t:COUTER-%d"
                               "(@Pool:%dDirection:%dActualOffset%d) : %x \n"),
                   offset_index, 
                   pool_number,
                   direction,
                   base_idx+offset_index,
                   value->packets));
        COMPILER_64_SET(value->packets64,
                 COMPILER_64_HI(flex_packet64_counter[unit]
                                [direction][pool_number]
                                [base_idx+offset_index]),
                 COMPILER_64_LO(flex_packet64_counter[unit]
                                [direction][pool_number]
                                [base_idx+offset_index]));
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Byte Count Value\t:COUNTER-%d"
                               "(@Pool:%dDirection:%dActualOffset%d) : %x:%x \n"),
                   offset_index,
                   pool_number,
                   direction,
                   base_idx+offset_index,
                   COMPILER_64_HI(value->packets64),
                   COMPILER_64_LO(value->packets64)));

    }
    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_stat_counter_raw_set
 * Description:
 *      Set Flex Counter Values
 * Parameters:
 *      unit                  - (IN) unit number
 *      stat_counter_id       - (IN) Stat Counter Id
 *      byte_flag             - (IN) Byte/Packet Flag
 *      counter               - (IN) Counter Index
 *      values                - (IN) Counter Value
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes: Must be called carefully.
 *        Preference should be given to _bcm_esw_stat_counter_set()
 *
 */
bcm_error_t _bcm_esw_stat_counter_raw_set(
            int              unit,
            uint32           stat_counter_id,
            uint32           byte_flag,
            uint32           counter_index, 
            bcm_stat_value_t *value)
{
    soc_mem_t                 mem;
#if defined(BCM_TRIDENT2_SUPPORT)
    soc_mem_t                 mem_dual = 0;
#endif /* BCM_TRIDENT2_SUPPORT*/

    uint32                    zero=0;
    uint32                    one=1;
    uint32                    offset_mode=0;
    uint32                    pool_number=0;
    uint32                    base_idx=0;
    uint32                    total_entries=0;
    uint32                    offset_index=0;
    uint32                    max_packet_mask=0;
    uint64                    max_byte_mask;
    uint32                    hw_val[2];     
    bcm_stat_flex_direction_t direction=bcmStatFlexDirectionIngress;
    bcm_stat_object_t         object=bcmStatObjectIngPort;
    bcm_stat_group_mode_t     group_mode= bcmStatGroupModeSingle;

    _bcm_esw_stat_get_counter_id_info(stat_counter_id,&group_mode,&object,
                                      &offset_mode,&pool_number,&base_idx);
    /* Validate object id first */
    if (_bcm_esw_stat_validate_object(unit,object,&direction) != BCM_E_NONE) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid bcm_stat_object_t passed %d \n"),
                   object));
        return BCM_E_PARAM;
    }
    /* Validate group_mode */
    if (_bcm_esw_stat_validate_group(unit,group_mode) != BCM_E_NONE) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid bcm_stat_group_mode_t passed %d \n"),
                   group_mode));
        return BCM_E_PARAM;
    }
    if (direction == bcmStatFlexDirectionIngress) {
        total_entries = flex_ingress_modes[unit][offset_mode].
                                          total_counters;
    } else {
        total_entries = flex_egress_modes[unit][offset_mode].
                                         total_counters;
    }
    if (flex_base_index_reference_count[unit][direction]
                                       [pool_number][base_idx] == 0) {
        return BCM_E_PARAM;
    }
    offset_index = counter_index;
    if (offset_index >= total_entries) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Wrong OFFSET_INDEX.Must be < Total Counters %d \n"),
                   total_entries));
        return BCM_E_PARAM;
    }
    if (flex_temp_counter[unit][direction] == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Unit %d Not initilized or attached yet\n"),
                   unit));
        return BCM_E_CONFIG;
    }
    mem = _ctr_counter_table[direction][0];
    COMPILER_64_SET(max_byte_mask, zero, one);
    COMPILER_64_SHL(max_byte_mask,soc_mem_field_length(unit,mem,BYTE_COUNTERf));
    COMPILER_64_SUB_32(max_byte_mask,one);

    max_packet_mask = (1 << soc_mem_field_length(unit,mem,PACKET_COUNTERf));
    max_packet_mask -= 1;

    BCM_STAT_FLEX_COUNTER_LOCK(unit);
#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit, soc_feature_advanced_flex_counter)
        && soc_feature(unit, soc_feature_two_ingress_pipes)) {
        mem = _ctr_counter_table_x[direction][pool_number];
        mem_dual = _ctr_counter_table_y[direction][pool_number];
    } else 
#endif
    {
        mem = _ctr_counter_table[direction][pool_number];
    }
    if (soc_mem_read(
                    unit,
                    mem,
                    MEM_BLOCK_ANY,
                    base_idx+offset_index,
                    flex_temp_counter[unit][direction]) != BCM_E_NONE){
            BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
            return BCM_E_INTERNAL;
    }
#if defined(BCM_TRIDENT2_SUPPORT)
    if (mem_dual) {
        if (soc_mem_read(
                    unit,
                    mem_dual,
                    MEM_BLOCK_ANY,
                    base_idx+offset_index,
                    flex_temp_counter_dual[unit][direction]) != BCM_E_NONE) {
            BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
            return BCM_E_INTERNAL;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT*/
    if (byte_flag == 1) {
        /* Mask to possible max values */
        COMPILER_64_AND(value->bytes, max_byte_mask);
        /* Update Soft Copy */
        COMPILER_64_SET(flex_byte_counter[unit][direction][pool_number]
                                         [base_idx+offset_index],
                        COMPILER_64_HI(value->bytes),
                         COMPILER_64_LO(value->bytes));
        /* Change Read Hw Copy */
        hw_val[0] = COMPILER_64_LO(value->bytes);
        hw_val[1] = COMPILER_64_HI(value->bytes);
        soc_mem_field_set(
                unit,
                mem,
                (uint32 *)&flex_temp_counter[unit][direction][offset_index],
                BYTE_COUNTERf,
                hw_val);

#if defined(BCM_TRIDENT2_SUPPORT)
        if (mem_dual) {
            int buffer_id;
            for (buffer_id = 0; buffer_id < _BCM_FLEX_STAT_BUFFS;
                 buffer_id++) {
                COMPILER_64_SET(flex_byte_counter_x[unit][direction]
                                [pool_number][buffer_id][base_idx+offset_index],
                                COMPILER_64_HI(value->bytes),
                                COMPILER_64_LO(value->bytes));
                COMPILER_64_SET(flex_byte_counter_y[unit][direction]
                                [pool_number][buffer_id][base_idx+offset_index],
                                COMPILER_64_HI(value->bytes),
                                COMPILER_64_LO(value->bytes));
            }
            soc_mem_field_set(
                    unit,
                    mem_dual,
                    (uint32 *)&flex_temp_counter_dual[unit][direction][offset_index],
                    BYTE_COUNTERf,
                    hw_val);
        }
#endif /* BCM_TRIDENT2_SUPPORT */
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Byte Count Value\t:COUTER-%d"
                               "(@Pool:%dDirection:%dActualOffset%d) : %x:%x \n"),
                   offset_index,
                   pool_number,
                   direction,
                   base_idx+offset_index,
                   COMPILER_64_HI(value->bytes),
                   COMPILER_64_LO(value->bytes)));
    } else {
        value->packets &= max_packet_mask;
        /* Update Soft Copy */
        flex_packet_counter[unit][direction][pool_number]
                           [base_idx+offset_index] = value->packets;
        /* Update Soft Copy */
        COMPILER_64_SET(flex_packet64_counter[unit][direction][pool_number]
                                         [base_idx+offset_index],0,
                         value->packets);

        /* Change Read Hw Copy */
        soc_mem_field_set(
                unit,
                mem,
                (uint32 *)&flex_temp_counter[unit][direction][offset_index],
                PACKET_COUNTERf,
                &(value->packets));
#if defined(BCM_TRIDENT2_SUPPORT)
        if (mem_dual) {
            int buffer_id;
            for (buffer_id = 0; buffer_id < _BCM_FLEX_STAT_BUFFS;
                 buffer_id++) {
                flex_pkt_counter_x[unit][direction][pool_number]
                                  [buffer_id][base_idx+offset_index] =
                                   value->packets;
                flex_pkt_counter_y[unit][direction][pool_number]
                                  [buffer_id][base_idx+offset_index] =
                                   value->packets;
            }
            soc_mem_field_set(
                    unit,
                    mem_dual,
                    (uint32 *)&flex_temp_counter_dual[unit][direction][offset_index],
                    PACKET_COUNTERf,
                    &(value->packets));
        }
#endif /* BCM_TRIDENT2_SUPPORT */
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Packet Count Value\t:COUTER-%d"
                               "(@Pool:%dDirection:%dActualOffset%d) : %x \n"),
                   offset_index, 
                   pool_number,
                   direction,
                   base_idx+offset_index,
                   value->packets));
    }
    /* Update Hw Copy */
    if (soc_mem_write(unit,mem,MEM_BLOCK_ALL,
                base_idx+offset_index,
                flex_temp_counter[unit][direction]) != BCM_E_NONE){
        BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
        return BCM_E_INTERNAL;
    }
#if defined(BCM_TRIDENT2_SUPPORT)
    if (mem_dual) {
        if (soc_mem_write(unit,mem_dual,MEM_BLOCK_ALL,
                    base_idx+offset_index,
                    flex_temp_counter[unit][direction]) != BCM_E_NONE){
            BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
            return BCM_E_INTERNAL;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT*/
    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_flex_stat_data32_rollover
 * Description:
 *      deal with the n-bits(n <= 32) data rollover case
 *      and add the diff to the 64-bits data.
 * Parameters:
 *      pre                  - (IN) previous n-bits data
 *      cur                  - (IN) current n-bits data
 *      roll_size          - (IN) the size when n-bits data rollovers
 *      sum                - (OUT) the summary of diff between pre and cur.
 * Return Value:
 *      None
 * Notes:
 *      None
 */
STATIC void
_bcm_flex_stat_data32_rollover(uint32 pre, uint32 cur, 
                               uint64 roll_size, uint64 *sum) 
{
    uint64  cur64, pre64;

    if (cur == pre) {
        return;
    } else if (cur > pre) {
        COMPILER_64_SET(cur64, 0, (cur - pre));
    } else {
        LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                    (BSL_META("Roll over  happend \n")));
        COMPILER_64_SET(cur64, 0, cur);
        COMPILER_64_SET(pre64, 0, pre);
        COMPILER_64_ADD_64(cur64, roll_size);
        COMPILER_64_SUB_64(cur64, pre64);        
    }

    COMPILER_64_ADD_64(*sum, cur64);
    return;
}  

/*
 * Function:
 *      _bcm_flex_stat_data64_rollover
 * Description:
 *      deal with the n-bits(32< n < 64) data rollover case
 *      and add the diff to the 64-bits data.
 * Parameters:
 *      pre                  - (IN) previous n-bits data
 *      cur                  - (IN) current n-bits data
 *      roll_size          - (IN) the size when n-bits data rollovers
 *      sum                - (OUT) the summary of diff between pre and cur.
 * Return Value:
 *      None
 * Notes:
 *      None
 */
STATIC void
_bcm_flex_stat_data64_rollover(uint64 pre, uint64 cur, 
                               uint64 roll_size, uint64 *sum) 
{
    uint64  max_byte_mask;

    if (sum == NULL) {
        return;
    }

    COMPILER_64_ZERO(max_byte_mask);
    COMPILER_64_SET(max_byte_mask,
                    COMPILER_64_HI(roll_size),
                    COMPILER_64_LO(roll_size));

    COMPILER_64_SUB_32(max_byte_mask,1);

    COMPILER_64_AND(pre, max_byte_mask);
    COMPILER_64_AND(cur, max_byte_mask);
    if (COMPILER_64_GT(pre, cur)) {
        LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                    (BSL_META("Roll over  happend \n")));
        LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                    (BSL_META("...Read Byte Count    : %x:%x\n"),
                     COMPILER_64_HI(cur),
                     COMPILER_64_LO(cur)));
        COMPILER_64_ADD_64(cur, roll_size);
        COMPILER_64_SUB_64(cur,pre);
        LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                    (BSL_META("...Diffed Byte Count    : %x:%x\n"),
                     COMPILER_64_HI(cur),
                     COMPILER_64_LO(cur)));
    } else {
        COMPILER_64_SUB_64(cur, pre);
    }
    /* Add difference (if it is) */
    if (!COMPILER_64_IS_ZERO(cur)) {
        COMPILER_64_ADD_64(*sum, cur);
        LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                    (BSL_META("New Byte Count Value : %x:%x\n"),
                     COMPILER_64_HI(*sum),
                     COMPILER_64_LO(*sum)));
    }
    return;
}  

/*
 * Function:
 *      _bcm_esw_stat_flex_counter_collect
 * Description:
 *      Flex Counter accumulation routine called periodically
 *      to sync s/w copy with h/w copy.
 * Parameters:
 *      unit                  - (IN) unit number
 *      cnt_direction         - (IN) counter direction
 *                                   if -1 accumulate for all counters
 *                                      for ingress and egress else
 *                                   retrieve a specific counter 
 *      flexctr_poll          - (IN) Ingress or Egress flex counter pool
 *                                   if -1 Both Ingress and Egress pools
 *                                   else Ingress or Egress pool.
 *      counter_idx           - (IN) counter index.
 *                                   if -1 accumulate for all counters
 *                                   else retrieve a specific counter.
 * Return Value:
 *      None
 * Notes:
 *       This routine accumulates all the flex counters and
 *       update the software copy of the flex counters.
 *       Counter thread invokes periodically this routine.
 *       If counter index is not -1 then only a specific counter
 *       value is read from harware and synced to the software copy. 
 *       used by all the counter_sync_get API's
 *
 */
STATIC void 
_bcm_esw_stat_flex_counter_collect(int                       unit,
                                   bcm_stat_flex_direction_t cnt_direction,
                                   uint32                    flexctr_pool,
                                   uint32                    counter_idx) 
{
    soc_mem_t mem;
#if defined(BCM_TRIDENT2_SUPPORT)
    soc_mem_t mem_dual = 0;
#endif /* BCM_TRIDENT2_SUPPORT*/

    uint32    num_pools[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32    pool_id = 0;
    bcm_stat_flex_direction_t direction = bcmStatFlexDirectionIngress;
    uint32    packet_count=0;
    uint32    index;
    uint32    zero=0;
    uint32    one=1;
    uint32    flex_ctr_update_control_reg_value=0;
    uint64    byte_count;
    uint64    prev_masked_byte_count;
    uint64    max_byte_size;
    uint64    max_byte_mask;
    soc_memacc_t memacc_pkt_x;
    soc_memacc_t memacc_byte_x;
#if defined(BCM_TRIDENT2_SUPPORT)
    soc_memacc_t memacc_pkt_y;
    soc_memacc_t memacc_byte_y;
    uint64       *byte_cur_ptr_x, *byte_cur_ptr_y;
    uint32       *pkt_cur_ptr_x, *pkt_cur_ptr_y;
    static uint8 toggle = 0;
#endif
    uint64    packet64_count;
    uint64    prev_masked_packet64_count;
    uint64    max_packet64_size;
    uint64    max_packet64_mask;
    uint32    mem_index_min;
    uint32    mem_index_max;
    uint32    max_directions = BCM_STAT_FLEX_COUNTER_MAX_DIRECTION;

    COMPILER_64_ZERO(byte_count);
    COMPILER_64_ZERO(prev_masked_byte_count);
    COMPILER_64_ZERO(max_byte_size);
    COMPILER_64_ZERO(max_byte_mask);

    COMPILER_64_ZERO(packet64_count);
    COMPILER_64_ZERO(prev_masked_packet64_count);
    COMPILER_64_ZERO(max_packet64_size);
    COMPILER_64_ZERO(max_packet64_mask);


    num_pools[bcmStatFlexDirectionIngress]= SOC_INFO(unit).
                                            num_flex_ingress_pools;
    num_pools[bcmStatFlexDirectionEgress] = SOC_INFO(unit).
                                            num_flex_egress_pools;
    /*
    size_pool[bcmStatFlexDirectionIngress]= SOC_INFO(unit).
                                            size_flex_ingress_pool;
    size_pool[bcmStatFlexDirectionEgress] = SOC_INFO(unit).
                                            size_flex_egress_pool;
     */

    /* 
     * if counter_idx != -1, request is to sync 
     * sw count with hw count of a specific counter
     */
    if (counter_idx != -1) {
        if ((cnt_direction < bcmStatFlexDirectionIngress) ||
            (cnt_direction > bcmStatFlexDirectionEgress)) {
            return; 
        }
           
        direction = cnt_direction; 
        max_directions = direction + 1; 

        /* coverity[unsigned_compare : FALSE] */
        if ((flexctr_pool > num_pools[direction]) || (flexctr_pool < 0)) {
            return;
        }
        pool_id = flexctr_pool;
        num_pools[direction] = pool_id + 1; 
    } else {
        flexctr_pool = 0;
    }

    mem = _ctr_counter_table[direction][pool_id];

    COMPILER_64_SET(max_byte_size, zero, one);
    COMPILER_64_SHL(max_byte_size,soc_mem_field_length(unit,mem,BYTE_COUNTERf));

    COMPILER_64_SET(max_byte_mask,
                    COMPILER_64_HI(max_byte_size),
                    COMPILER_64_LO(max_byte_size));
    COMPILER_64_SUB_32(max_byte_mask,one);


    COMPILER_64_SET(max_packet64_size, zero, one);
    COMPILER_64_SHL(max_packet64_size,soc_mem_field_length(unit,mem,PACKET_COUNTERf));

    COMPILER_64_SET(max_packet64_mask,
                    COMPILER_64_HI(max_packet64_size),
                    COMPILER_64_LO(max_packet64_size));
    COMPILER_64_SUB_32(max_packet64_mask,one);

    for ( ; direction < max_directions; direction++) {
        if (flex_temp_counter[unit][direction] == NULL) {
            LOG_WARN(BSL_LS_BCM_FLEXCTR,
                     (BSL_META_U(unit,
                                 "Unit %d Not Initilized or attached \n"),
                      unit));
            continue;
        }
#if defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TD2_TT2(unit)) {
            if (flex_temp_counter_dual[unit][direction] == NULL) {
                LOG_WARN(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "Unit %d Not Initilized or attached \n"),
                          unit));
                continue;
            }
        }
#endif
        for (pool_id = flexctr_pool; pool_id < num_pools[direction]; pool_id++){
            if (soc_reg32_get(
                            unit,
                            _pool_ctr_register[direction][pool_id],
                            REG_PORT_ANY,
                            0,
                            &flex_ctr_update_control_reg_value) != SOC_E_NONE) {
                continue;
            }
            if (soc_reg_field_get(
                            unit,
                            _pool_ctr_register[direction][pool_id],
                            flex_ctr_update_control_reg_value,
                            COUNTER_POOL_ENABLEf) == 0) {
                continue;
            }
#if 0
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "%d-%d.."),
                       direction,pool_id));
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
            if (SOC_IS_TD2_TT2(unit)) {
                mem = _ctr_counter_table_x[direction][pool_id];
                mem_dual = _ctr_counter_table_y[direction][pool_id];

            } else
#endif
            {
                mem = _ctr_counter_table[direction][pool_id];
            }

            mem_index_min = soc_mem_index_min(unit, mem);
            mem_index_max = soc_mem_index_max(unit, mem);
            
            /* 
             * if counter_idx != -1, retrieve specific
             * counter index from the flex counter memory pool.   
             */
            if (counter_idx != -1) {
                if ((counter_idx < mem_index_min) ||
                    (counter_idx > mem_index_max)) {
                    return; 
                }   
                mem_index_min = mem_index_max = counter_idx;
            }

            /* read all the counters or a specific counter from memory */
            if (counter_idx == -1 ) {
                if (soc_mem_read_range(unit, mem, MEM_BLOCK_ANY,
                                       mem_index_min, mem_index_max,
                                       flex_temp_counter[unit][direction])
                                       != BCM_E_NONE) {
                    return;
                }
            } else {
                if (soc_mem_read(unit, mem, MEM_BLOCK_ANY,
                                 counter_idx,
                                &flex_temp_counter[unit][direction][counter_idx])
                                 != BCM_E_NONE) {
                    return;
                }
            }

            if (((soc_memacc_init(unit, mem, PACKET_COUNTERf, &memacc_pkt_x))) ||
                ((soc_memacc_init(unit, mem, BYTE_COUNTERf, &memacc_byte_x)))) {
                  return;
            }

#if defined(BCM_TRIDENT2_SUPPORT)
            if (SOC_IS_TD2_TT2(unit)) {
                /* read all the counters or a specific counter from memory */
                if (counter_idx == -1 ) {
                    if (soc_mem_read_range(
                                       unit, mem_dual, MEM_BLOCK_ANY,
                                       mem_index_min, mem_index_max,
                                       flex_temp_counter_dual[unit][direction])
                                       != BCM_E_NONE) {
                        return;
                    }
                } else {
                    if (soc_mem_read(unit, mem_dual, MEM_BLOCK_ANY,
                          counter_idx,
                          &flex_temp_counter_dual[unit][direction][counter_idx])
                                 != BCM_E_NONE) {
                        return;
                    }
                }

                if (((soc_memacc_init(unit, mem_dual, PACKET_COUNTERf, &memacc_pkt_y))) ||
                    ((soc_memacc_init(unit, mem_dual, BYTE_COUNTERf, &memacc_byte_y)))) {
                    return;
                }
                
                if (flex_byte_counter_x[unit][direction][pool_id][toggle]
                                                                  == NULL) {
                    LOG_WARN(BSL_LS_BCM_FLEXCTR,
                             (BSL_META_U(unit,
                                         "Unit %d Not Initilized or attached \n"),
                              unit));
                    continue;
                }
                
                if (flex_pkt_counter_x[unit][direction][pool_id][toggle]
                                                                    == NULL) {
                    LOG_WARN(BSL_LS_BCM_FLEXCTR,
                             (BSL_META_U(unit,
                                         "Unit %d Not Initilized or attached \n"),
                              unit));
                    continue;
                }
                 
                if (flex_byte_counter_y[unit][direction][pool_id][toggle]
                                                                  == NULL) {
                    LOG_WARN(BSL_LS_BCM_FLEXCTR,
                             (BSL_META_U(unit,
                                         "Unit %d Not Initilized or attached \n"),
                              unit));
                    continue;
                }
                
                if (flex_pkt_counter_y[unit][direction][pool_id][toggle]
                                                                    == NULL) {
                    LOG_WARN(BSL_LS_BCM_FLEXCTR,
                             (BSL_META_U(unit,
                                         "Unit %d Not Initilized or attached \n"),
                              unit));
                    continue;
                }
            }
#endif /* BCM_TRIDENT2_SUPPORT*/

            for (index = mem_index_min; index <= mem_index_max; index++) {

                packet_count = soc_memacc_field32_get(&memacc_pkt_x,
                            (uint32 *)&flex_temp_counter[unit][direction][index]);
#if defined(BCM_TRIDENT2_SUPPORT)
                if (SOC_IS_TD2_TT2(unit)) {
                    uint32 dual_packet_count;
                    pkt_cur_ptr_x = 
                        flex_pkt_counter_x[unit][direction][pool_id][toggle]
                        + index;
                    *pkt_cur_ptr_x = packet_count;                     
                    _bcm_flex_stat_data32_rollover(flex_pkt_counter_x[unit] 
                                                   [direction][pool_id][!toggle]
                                                   [index],
                                                   *pkt_cur_ptr_x,
                                                   max_packet64_size,
                                                   flex_packet64_counter[unit]
                                                   [direction][pool_id] + index
                                                   );
                     
                    dual_packet_count = soc_memacc_field32_get(&memacc_pkt_y,
                                (uint32 *)&flex_temp_counter_dual[unit][direction][index]);
                    pkt_cur_ptr_y = 
                        flex_pkt_counter_y[unit][direction][pool_id][toggle]
                        + index;                    
                    *pkt_cur_ptr_y = dual_packet_count;                    
                    _bcm_flex_stat_data32_rollover(flex_pkt_counter_y[unit] 
                                                   [direction][pool_id]
                                                   [!toggle][index],
                                                   *pkt_cur_ptr_y,
                                                   max_packet64_size,
                                                   flex_packet64_counter[unit]
                                                   [direction][pool_id] + index
                                                   );
                    packet_count += dual_packet_count;
                }
#endif /* BCM_TRIDENT2_SUPPORT*/
                if (flex_packet_counter[unit][direction][pool_id][index]
                    != packet_count) {
                    LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                                (BSL_META_U(unit,
                                            "Direction:%dPool:%d==>"
                                             "Old Packet Count Value\t:"
                                             "Index:%d %x\tNew Packet Count Value %x\n"),
                                 direction,
                                 pool_id,
                                 index,
                                 flex_packet_counter[unit]
                                 [direction][pool_id][index],
                                 packet_count));
                    flex_packet_counter[unit][direction][pool_id][index] =
                               packet_count;
                }

                soc_memacc_field64_get(&memacc_byte_x,
                            (uint32 *)&flex_temp_counter[unit][direction][index],
                            &byte_count);

#if defined(BCM_TRIDENT2_SUPPORT)
                if (SOC_IS_TD2_TT2(unit)) {
                    uint64 dual_byte_count;
                    
                    byte_cur_ptr_x = 
                        flex_byte_counter_x[unit][direction][pool_id][toggle]
                        + index;
                    COMPILER_64_SET(*byte_cur_ptr_x,
                                    COMPILER_64_HI(byte_count),
                                    COMPILER_64_LO(byte_count));
                    _bcm_flex_stat_data64_rollover(flex_byte_counter_x[unit] 
                                                   [direction][pool_id]
                                                   [!toggle][index],
                                                   *byte_cur_ptr_x,
                                                   max_byte_size,
                                                   flex_byte_counter[unit]
                                                   [direction][pool_id] + index
                                                   );
                    
                    soc_memacc_field64_get(&memacc_byte_y,
                        (uint32 *)&flex_temp_counter_dual[unit][direction][index],
                        &dual_byte_count);
                    byte_cur_ptr_y = 
                        flex_byte_counter_y[unit][direction][pool_id][toggle]
                        + index;
                    COMPILER_64_SET(*byte_cur_ptr_y,
                                    COMPILER_64_HI(dual_byte_count),
                                    COMPILER_64_LO(dual_byte_count));
                    _bcm_flex_stat_data64_rollover(flex_byte_counter_y[unit] 
                                                   [direction][pool_id]
                                                   [!toggle][index],
                                                   *byte_cur_ptr_y,
                                                   max_byte_size,
                                                   flex_byte_counter[unit]
                                                   [direction][pool_id] + index
                                                   );                   
                    continue;
                }
#endif /* BCM_TRIDENT2_SUPPORT*/


                   COMPILER_64_SET(packet64_count,0, packet_count);
                   COMPILER_64_SET(prev_masked_packet64_count,
                                   COMPILER_64_HI(flex_packet64_counter[unit]
                                                  [direction][pool_id][index]),
                                   COMPILER_64_LO(flex_packet64_counter[unit]
                                                  [direction][pool_id][index]));
                   COMPILER_64_AND(prev_masked_packet64_count,max_packet64_mask);
                   if (COMPILER_64_GT(prev_masked_packet64_count, packet64_count)) {
                       LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "Roll over  happend \n")));
                       LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "...Read Packet64 Count    : %x:%x\n"),
                                    COMPILER_64_HI(packet64_count),
                                    COMPILER_64_LO(packet64_count)));
                       COMPILER_64_ADD_64(packet64_count,max_packet64_size);
                       COMPILER_64_SUB_64(packet64_count,prev_masked_packet64_count);
                       LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "...Diffed 64-Packet Count    : %x:%x\n"),
                                    COMPILER_64_HI(packet64_count),
                                    COMPILER_64_LO(packet64_count)));
                   } else {
                       COMPILER_64_SUB_64(packet64_count,prev_masked_packet64_count);
                   }
                   /* Add difference (if it is) */
                   if (!COMPILER_64_IS_ZERO(packet64_count)) {
                       LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "Direction:%dPool:%d==>"
                                                "Old 64-Packet Count Value\t"
                                                ":Index:%d %x:%x\t"),
                                    direction,pool_id,index,
                                    COMPILER_64_HI(flex_packet64_counter[unit]
                                    [direction][pool_id][index]),
                                    COMPILER_64_LO(flex_packet64_counter[unit]
                                    [direction][pool_id][index])));
                       COMPILER_64_ADD_64(flex_packet64_counter[unit]
                                          [direction][pool_id][index],
                                          packet64_count);
                       LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "New 64-Packet Value : %x:%x\n"),
                                    COMPILER_64_HI(flex_packet64_counter[unit]
                                    [direction][pool_id][index]),
                                    COMPILER_64_LO(flex_packet64_counter[unit][direction]
                                    [pool_id][index])));
                   }



                   COMPILER_64_SET(prev_masked_byte_count,
                                   COMPILER_64_HI(flex_byte_counter[unit]
                                                  [direction][pool_id][index]),
                                   COMPILER_64_LO(flex_byte_counter[unit]
                                                  [direction][pool_id][index]));
                   COMPILER_64_AND(prev_masked_byte_count,max_byte_mask);
                   if (COMPILER_64_GT(prev_masked_byte_count, byte_count)) {
                       LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "Roll over  happend \n")));
                       LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "...Read Byte Count    : %x:%x\n"),
                                    COMPILER_64_HI(byte_count),
                                    COMPILER_64_LO(byte_count)));
                       COMPILER_64_ADD_64(byte_count,max_byte_size);
                       COMPILER_64_SUB_64(byte_count,prev_masked_byte_count);
                       LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "...Diffed Byte Count    : %x:%x\n"),
                                    COMPILER_64_HI(byte_count),
                                    COMPILER_64_LO(byte_count)));
                   } else {
                       COMPILER_64_SUB_64(byte_count,prev_masked_byte_count);
                   }
                   /* Add difference (if it is) */
                   if (!COMPILER_64_IS_ZERO(byte_count)) {
                       LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "Direction:%dPool:%d==>"
                                                "Old Byte Count Value\t"
                                                ":Index:%d %x:%x\t"),
                                    direction,pool_id,index,
                                    COMPILER_64_HI(flex_byte_counter[unit]
                                    [direction][pool_id][index]),
                                    COMPILER_64_LO(flex_byte_counter[unit]
                                    [direction][pool_id][index])));
                       COMPILER_64_ADD_64(flex_byte_counter[unit]
                                          [direction][pool_id][index],
                                          byte_count);
                       LOG_VERBOSE(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "New Byte Count Value : %x:%x\n"),
                                    COMPILER_64_HI(flex_byte_counter[unit]
                                    [direction][pool_id][index]),
                                    COMPILER_64_LO(flex_byte_counter[unit][direction]
                                    [pool_id][index])));
                   }
              }
         }
    }
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        toggle = !toggle;
    }
#endif /* BCM_TRIDENT2_SUPPORT*/
    return;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_callback
 * Description:
 *      Flex Counter threads callback function. It is called periodically
 *      to sync s/w copy with h/w copy.
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
void _bcm_esw_stat_flex_callback(int unit)
{
    BCM_STAT_FLEX_COUNTER_LOCK(unit);
    _bcm_esw_stat_flex_counter_collect(unit, -1, -1, -1);
    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);

    return;
}

/*
 * Function:
 *      _bcm_esw_stat_flex_get_table_info
 * Description:
 *      Get Table related Information based on accounting object value
 * Parameters:
 *      object                - (IN) Flex Accounting Object
 *      table                 - (OUT) Flex Accounting Table
 *      direction             - (OUT) Flex Data flow direction
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_get_table_info(
            int                       unit,
            bcm_stat_object_t         object,
            uint32                    expected_num_tables,
            uint32                    *actual_num_tables,
            soc_mem_t                 *table,
            bcm_stat_flex_direction_t *direction)             
{
    if (expected_num_tables < 1) {
        return BCM_E_PARAM; 
    }
    switch(object) {
    case bcmStatObjectIngPort:
         *table=PORT_TABm;
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngVlan:
         *table=VLAN_TABm;
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngVlanXlate:
#if defined(BCM_TRIUMPH3_SUPPORT)
         if (SOC_IS_TRIUMPH3(unit)) { 
             *table=VLAN_XLATE_EXTDm;
             *direction= bcmStatFlexDirectionIngress;
         } else
#endif
         { 
             *table=VLAN_XLATEm;
             *direction= bcmStatFlexDirectionIngress;
         }
         break;
    case bcmStatObjectIngVfi:
         *table=VFIm;
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngL3Intf:
         *table=L3_IIFm;
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngVrf:
         *table=VRFm;
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngPolicy:
         *table=VFP_POLICY_TABLEm;
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngNiv:
    case bcmStatObjectIngMplsVcLabel:
         *table=SOURCE_VPm;
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngMplsSwitchLabel:
#if defined(BCM_TRIUMPH3_SUPPORT)
         if (SOC_IS_TRIUMPH3(unit)) { 
             *table=MPLS_ENTRY_EXTDm;
             *direction= bcmStatFlexDirectionIngress;
         } else
#endif
         { 
             *table=MPLS_ENTRYm;
             *direction= bcmStatFlexDirectionIngress;
         }
         break;
#if defined(BCM_TRIUMPH3_SUPPORT)
    case bcmStatObjectIngMplsFrrLabel:
         *table=L3_TUNNELm;
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngL3Host:

        if (!soc_feature(unit, soc_feature_l3_extended_host_entry)) {
            return BCM_E_PARAM;
        }

         if (SOC_IS_TD2_TT2(unit)) {
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)++]=L3_ENTRY_IPV4_MULTICASTm;
             }
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)]=L3_ENTRY_IPV6_MULTICASTm;
             }             
         } else {
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)++]=EXT_IPV4_UCAST_WIDEm;
             }
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)++]=EXT_IPV6_128_UCAST_WIDEm;
             }
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)++]=L3_ENTRY_2m;
             }
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)]=L3_ENTRY_4m;
             }
         }
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngTrill:
         *table=MPLS_ENTRY_EXTDm;
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngMimLookupId:
         *table=MPLS_ENTRY_EXTDm;
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngL2Gre:
         if ((*actual_num_tables) < expected_num_tables) {
              table[(*actual_num_tables)++]=SOURCE_VPm;
         }
         if ((*actual_num_tables) < expected_num_tables) {
              table[(*actual_num_tables)]=VFIm;
         }
         *direction= bcmStatFlexDirectionIngress;
         break;
    case bcmStatObjectIngEXTPolicy:
         *table=EXT_FP_POLICYm;
         *direction= bcmStatFlexDirectionIngress;
         break;
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    case  bcmStatObjectIngVxlan:
        if (SOC_IS_TD2_TT2(unit)) {
             if ((*actual_num_tables) < expected_num_tables) {
                  table[(*actual_num_tables)++]=SOURCE_VPm;
             }
             if ((*actual_num_tables) < expected_num_tables) {
                  table[(*actual_num_tables)]=VFIm;
             }
             *direction= bcmStatFlexDirectionIngress;
        }
         break;
    case  bcmStatObjectIngVsan:
        if (SOC_IS_TD2_TT2(unit)) {
             *table=ING_VSANm;
             *direction= bcmStatFlexDirectionIngress;
        }
         break;
    case  bcmStatObjectIngFcoe:
        if (SOC_IS_TD2_TT2(unit)) {
             *table=L3_ENTRY_IPV4_MULTICASTm;
             *direction= bcmStatFlexDirectionIngress;
        }
         break;
    case  bcmStatObjectIngL3Route:
        if (SOC_IS_TD2_TT2(unit)) {
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)++]=L3_DEFIPm;
             }
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)++]=L3_DEFIP_PAIR_128m;
             }
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)++]=L3_DEFIP_ALPM_IPV4_1m;
             }
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)++]=L3_DEFIP_ALPM_IPV6_64_1m;
             }
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)++]=L3_DEFIP_ALPM_IPV6_128m;
             }
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)]=L3_DEFIP_AUX_SCRATCHm;
             }
             *direction= bcmStatFlexDirectionIngress;
        }
         break;
    case bcmStatObjectIngIpmc:
         if (SOC_IS_TD2_TT2(unit)) {
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)++]=L3_ENTRY_IPV4_MULTICASTm;
             }
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)]=L3_ENTRY_IPV6_MULTICASTm;
             }             
         }
         *direction= bcmStatFlexDirectionIngress;
         break;  
    case bcmStatObjectIngVxlanDip:
         if (SOC_IS_TD2_TT2(unit)) {
             if ((*actual_num_tables) < expected_num_tables) {
                 table[(*actual_num_tables)++]=VLAN_XLATEm;
             }             
             *direction= bcmStatFlexDirectionIngress;
         }
         break;           
#endif /* BCM_TRIDENT2_SUPPORT */

    case bcmStatObjectEgrPort:
         *table=EGR_PORTm;
         *direction= bcmStatFlexDirectionEgress;
         break;
    case bcmStatObjectEgrVlan:
         *table=EGR_VLANm;
         *direction= bcmStatFlexDirectionEgress;
         break;
    case bcmStatObjectEgrVlanXlate:
         *table=EGR_VLAN_XLATEm;
         *direction= bcmStatFlexDirectionEgress;
         break;
    case bcmStatObjectEgrVfi:
         *table=EGR_VFIm;
         *direction= bcmStatFlexDirectionEgress;
         break;
    case bcmStatObjectEgrNiv:
    case bcmStatObjectEgrL3Intf:
         *table=EGR_L3_NEXT_HOPm;
         *direction= bcmStatFlexDirectionEgress;
         break;
    case bcmStatObjectEgrWlan:
         *table=EGR_L3_NEXT_HOPm;
         *direction= bcmStatFlexDirectionEgress;
         break;
    case bcmStatObjectEgrMim:
         *table=EGR_L3_NEXT_HOPm;
         *direction= bcmStatFlexDirectionEgress;
         break;
    case bcmStatObjectEgrMimLookupId:
         *table=EGR_VLAN_XLATEm;
         *direction= bcmStatFlexDirectionEgress;
         break;
    case bcmStatObjectEgrL2Gre:
         if ((*actual_num_tables) < expected_num_tables) {
              table[(*actual_num_tables)++]=EGR_DVP_ATTRIBUTE_1m;
         }
         if ((*actual_num_tables) < expected_num_tables) {
              table[(*actual_num_tables)]=EGR_VFIm;
         }
         *direction= bcmStatFlexDirectionEgress;
         break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case  bcmStatObjectEgrVxlan:
        if (SOC_IS_TD2_TT2(unit)) {
         if ((*actual_num_tables) < expected_num_tables) {
              table[(*actual_num_tables)++]=EGR_DVP_ATTRIBUTE_1m;
         }
         if ((*actual_num_tables) < expected_num_tables) {
              table[(*actual_num_tables)++]=EGR_L3_NEXT_HOPm;
         }
         if ((*actual_num_tables) < expected_num_tables) {
              table[(*actual_num_tables)]=EGR_VFIm;
         }
         *direction= bcmStatFlexDirectionEgress;
        }
         break;
    case  bcmStatObjectEgrL3Nat:
        if (SOC_IS_TD2_TT2(unit)) {
            *table=EGR_NAT_PACKET_EDIT_INFOm;
            *direction= bcmStatFlexDirectionEgress;
        }
         break;
#endif /* BCM_TRIDENT2_SUPPORT */
    default:
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Invalid Object is passed %d\n"),
                    object));
         return BCM_E_PARAM;
    }
    (*actual_num_tables)++;
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_detach_egress_table_counters
 * Description:
 *      Detach i.e. Disable Egresss accounting table's statistics
 * Parameters:
 *      unit                  - (IN) unit number
 *      egress_table          - (IN) Flex Accounting Table
 *      index                 - (IN) Flex Accounting Table's Index
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_detach_egress_table_counters(
            int       unit,
            soc_mem_t egress_table,
            uint32    index)
{
    uint32                          offset_mode=0;
    uint32                          pool_number=0;
    uint32                          base_idx=0;
    uint32                          egress_entry_data_size=0;
    uint32                          flex_entries=0;
    bcm_stat_flex_ctr_offset_info_t flex_ctr_offset_info={0};
    void                            *egress_entry_data=NULL;
    bcm_stat_object_t               object=bcmStatObjectEgrPort;
    uint32                          stat_counter_id=0;

    if (!((egress_table == EGR_VLANm) ||
          (egress_table == EGR_VFIm)  ||
          (egress_table == EGR_L3_NEXT_HOPm) ||
          (egress_table == EGR_VLAN_XLATEm)  ||
#if defined(BCM_TRIUMPH3_SUPPORT)
          (egress_table == EGR_DVP_ATTRIBUTE_1m) ||
#endif 
#if defined(BCM_TRIDENT2_SUPPORT)
          (egress_table == EGR_NAT_PACKET_EDIT_INFOm) ||
#endif
          (egress_table == EGR_PORTm))) {
           LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                     (BSL_META_U(unit,
                                 "Invalid Flex Counter Egress Memory %s\n"),
                      SOC_MEM_UFNAME(unit, egress_table)));
           return BCM_E_PARAM;
    }
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Deallocating EGRESS counter for Table %s with index %d \n"),
               SOC_MEM_UFNAME(unit, egress_table),index));
    egress_entry_data_size = WORDS2BYTES(BYTES2WORDS(
                             SOC_MEM_INFO(unit, egress_table).bytes));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Deallocating EgressCounter Table:%s:with" 
                           "index:%d:ENTRY_BYTES:%d\n"),
               SOC_MEM_UFNAME(unit, egress_table),
               index,egress_entry_data_size));
    egress_entry_data = sal_alloc(egress_entry_data_size,"egress_table");
    if (egress_entry_data == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Failed to allocate memory for Table:%s "),
                   SOC_MEM_UFNAME(unit, egress_table)));
         return BCM_E_INTERNAL;
    }
    sal_memset(egress_entry_data,0,egress_entry_data_size);

    if (soc_mem_read(unit, egress_table, MEM_BLOCK_ANY,
                 _bcm_esw_stat_flex_table_index_map(unit,egress_table,index),
                 egress_entry_data) == SOC_E_NONE) {
        if (soc_mem_field_valid(unit, egress_table, VALIDf)) {
            if (soc_mem_field32_get(unit, egress_table, egress_entry_data,
                                    VALIDf)==0) {
                LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "Table %s  with index %d is Not valid \n"),
                           SOC_MEM_UFNAME(unit, egress_table), index));
                sal_free(egress_entry_data);
                return BCM_E_PARAM;
            }
        }
        _bcm_esw_get_flex_counter_fields_values(
                 unit,index,egress_table,egress_entry_data,
                 &offset_mode,&pool_number,&base_idx);
        if ((offset_mode == 0) && (base_idx == 0)) {
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Table:%s:Index:%d:is NotConfiguredForFlexCtrYet\n"),
                        SOC_MEM_UFNAME(unit, egress_table), index));
             sal_free(egress_entry_data);
             return BCM_E_NOT_FOUND;/*Either NotConfigured/DeallocatedBefore */
        }

        /* Decrement reference counts */
        BCM_STAT_FLEX_COUNTER_LOCK(unit);
        flex_base_index_reference_count[unit][bcmStatFlexDirectionEgress]
                                       [pool_number][base_idx]--;
        flex_pool_stat[unit][bcmStatFlexDirectionEgress][pool_number].
                       attached_entries -= flex_egress_modes[unit]
                                           [offset_mode].total_counters;
        BCM_STAT_FLEX_COUNTER_UNLOCK(unit);


        /* Clear Counter Values when reference count is zero */
        if ((flex_base_index_reference_count[unit]
                [bcmStatFlexDirectionEgress] [pool_number][base_idx]) == 0 ) {
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Clearing Counter Tables %s contents:Offset:%d Len:%d\n"),
                       SOC_MEM_UFNAME(unit, egress_table),
                       base_idx,
                       flex_egress_modes[unit][offset_mode].total_counters));
            flex_ctr_offset_info.all_counters_flag =  1;
            _bcm_esw_stat_flex_set(unit, index, egress_table, -1,
                                   flex_ctr_offset_info,
                                   &flex_entries,
                                   offset_mode,pool_number,base_idx,
                                   bcmStatFlexDirectionEgress);
        }

        /* Reset flex fields */
        _bcm_esw_set_flex_counter_fields_values(unit,
                                                index,
                                                egress_table,
                                                egress_entry_data,0,0,0);
        if (soc_mem_write(unit, egress_table, MEM_BLOCK_ALL,
                      _bcm_esw_stat_flex_table_index_map(unit,egress_table,index),
                      egress_entry_data) != SOC_E_NONE) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Table:%s:Index:%d: encounter some problem \n"),
                       SOC_MEM_UFNAME(unit, egress_table), index));
            sal_free(egress_entry_data);
            return    BCM_E_INTERNAL;
        }
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Deallocated Table:%s:Index:%d:mode:%d"
                               "reference_count %d \n"),
                   SOC_MEM_UFNAME(unit, egress_table),index,offset_mode,
                   flex_base_index_reference_count[unit]
                   [bcmStatFlexDirectionEgress][pool_number][base_idx]));
        if(_bcm_esw_stat_flex_get_egress_object(unit,egress_table,
           index,egress_entry_data,&object) != BCM_E_NONE) {
           sal_free(egress_entry_data);
           return BCM_E_INTERNAL;
        }
        sal_free(egress_entry_data);

        /* Disable flex pool for this stat on no flex entries */
        if (flex_pool_stat[unit][bcmStatFlexDirectionEgress][pool_number].
                           attached_entries  == 0) {
            _bcm_esw_stat_flex_enable_pool(
                    unit,bcmStatFlexDirectionEgress,
                    _pool_ctr_register[bcmStatFlexDirectionEgress][pool_number],
                    0);
        }
        _bcm_esw_stat_get_counter_id(
                      flex_egress_modes[unit][offset_mode].group_mode,
                      object,offset_mode,pool_number,
                      base_idx,&stat_counter_id);
        if (flex_base_index_reference_count[unit][bcmStatFlexDirectionEgress]
                                           [pool_number][base_idx] == 0) {
            if (_bcm_esw_stat_flex_insert_stat_id(
                        local_scache_ptr[unit],stat_counter_id) != BCM_E_NONE) {
                LOG_WARN(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "WARMBOOT:Couldnot add entry in scache memory."
                                      "Attach it\n")));
            }
        }
        return BCM_E_NONE;
    }
    sal_free(egress_entry_data);
    return    BCM_E_NOT_FOUND;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_destroy_egress_table_counters
 * Description:
 *      Destroy Egresss accounting table's statistics completely
 * Parameters:
 *      unit                  - (IN) unit number
 *      egress_table          - (IN) Flex Accounting Table
 *      offset_mode           - (IN) Flex offset mode for Accounting Object
 *      base_idx              - (IN) Flex Base Index for Accounting Object
 *      pool_number           - (IN) Flex Pool Number for Accounting Object
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_destroy_egress_table_counters(
            int                  unit,
            soc_mem_t            egress_table,
            bcm_stat_object_t    object,
            bcm_stat_flex_mode_t offset_mode,
            uint32               base_idx,
            uint32               pool_number)
{
    uint32                          free_count=0;
    uint32                          alloc_count=0;
    uint32                          largest_free;
    uint32                          used_by_table=0;
    uint32                          stat_counter_id=0;

    if (flex_egress_modes[unit][offset_mode].available==0) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "flex CounterMode:%d:Not configured yet\n"),
                   offset_mode));
        return BCM_E_NOT_FOUND;
    }
    if (shr_aidxres_list_elem_state(flex_aidxres_list_handle
                                    [unit][bcmStatFlexDirectionEgress]
                                    [pool_number],base_idx) != BCM_E_EXISTS) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Wrong base index %u \n"),
                   base_idx));
        return BCM_E_NOT_FOUND;
    }
    if (flex_base_index_reference_count[unit]
        [bcmStatFlexDirectionEgress][pool_number][base_idx] != 0) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Reference count is  %d.. Please detach entries first..\n"),
                   flex_base_index_reference_count[unit]
                   [bcmStatFlexDirectionEgress][pool_number][base_idx]));
        return BCM_E_FAIL;
    }
    switch(egress_table) {
    case EGR_VLANm:
         used_by_table=FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_TABLE;
         break;
    case EGR_VFIm:
         used_by_table=FLEX_COUNTER_POOL_USED_BY_EGR_VFI_TABLE;
         break;
    case EGR_L3_NEXT_HOPm:
         used_by_table=FLEX_COUNTER_POOL_USED_BY_EGR_L3_NEXT_HOP_TABLE;
         break;
    case EGR_VLAN_XLATEm:
         used_by_table=FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_XLATE_TABLE;
         break;
    case EGR_PORTm:
         used_by_table=FLEX_COUNTER_POOL_USED_BY_EGR_PORT_TABLE;
         break;
#if defined(BCM_TRIUMPH3_SUPPORT)
    case EGR_DVP_ATTRIBUTE_1m:
         used_by_table=FLEX_COUNTER_POOL_USED_BY_EGR_DVP_ATTRIBUTE_1_TABLE;
         break;
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    case EGR_NAT_PACKET_EDIT_INFOm:
         used_by_table=FLEX_COUNTER_POOL_USED_BY_EGR_L3_NAT_TABLE;
         break;
#endif
    default:
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Invalid Flex Counter Egress Memory %s\n"),
                    SOC_MEM_UFNAME(unit, egress_table)));
         return BCM_E_PARAM;
    }

    /* Free pool list */
    if (shr_aidxres_list_free(flex_aidxres_list_handle
                              [unit][bcmStatFlexDirectionEgress][pool_number],
                              base_idx) != BCM_E_NONE) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Freeing memory Table:%s:encounter problem due entry not found or due to some other issue  \n"),
                   SOC_MEM_UFNAME(unit, egress_table)));
        return BCM_E_NOT_FOUND;
    }
    _bcm_esw_stat_get_counter_id(
                  flex_egress_modes[unit][offset_mode].group_mode,
                  object,offset_mode,pool_number,base_idx,&stat_counter_id);
    if (_bcm_esw_stat_flex_delete_stat_id(
                 local_scache_ptr[unit],stat_counter_id) != BCM_E_NONE) {
        LOG_WARN(BSL_LS_BCM_FLEXCTR,
                 (BSL_META_U(unit,
                             "WARMBOOT: Couldnot Delete entry in scache memory.\n")));
    }
    shr_aidxres_list_state(flex_aidxres_list_handle
                           [unit][bcmStatFlexDirectionEgress][pool_number],
                           NULL,NULL,NULL,NULL,
                           &free_count,&alloc_count,&largest_free,NULL);
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Pool status free_count:%d alloc_count:%d largest_free:%d"
                           "used_by_tables:%d used_entries:%d\n"),
               free_count,alloc_count,largest_free,
               flex_pool_stat[unit][bcmStatFlexDirectionEgress]
               [pool_number].used_by_tables,
               flex_pool_stat[unit][bcmStatFlexDirectionEgress]
               [pool_number].used_entries));
    flex_pool_stat[unit][bcmStatFlexDirectionEgress][pool_number].
                  used_entries -= flex_egress_modes[unit]
                                  [offset_mode].total_counters;
    if (flex_pool_stat[unit][bcmStatFlexDirectionEgress][pool_number].
        used_entries == 0) {
        flex_pool_stat[unit][bcmStatFlexDirectionEgress]
                      [pool_number].used_by_tables &= ~used_by_table;
        SHR_BITCLR(flex_pool_stat[unit][bcmStatFlexDirectionEgress]
                      [pool_number].used_by_objects, object);
    }
    BCM_STAT_FLEX_COUNTER_LOCK(unit);
    flex_egress_modes[unit][offset_mode].reference_count--;
    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_detach_ingress_table_counters
 * Description:
 *      Detach i.e. Disable Igresss accounting table's statistics
 * Parameters:
 *      unit                  - (IN) unit number
 *      ingress_table         - (IN) Flex Accounting Table
 *      index                 - (IN) Flex Accounting Table's Index
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_detach_ingress_table_counters(
            int       unit,
            soc_mem_t ingress_table,
            uint32    index)
{
    uint32                          offset_mode=0;
    uint32                          pool_number=0;
    uint32                          base_idx=0;
    uint32                          ingress_entry_data_size=0;
    uint32                          flex_entries=0;
    bcm_stat_flex_ctr_offset_info_t flex_ctr_offset_info={0};
    void                            *ingress_entry_data=NULL;
    bcm_stat_object_t               object=bcmStatObjectIngPort;
    uint32                          stat_counter_id=0;

    if (!((ingress_table == PORT_TABm) ||
         (ingress_table == VLAN_XLATEm)  ||
#if defined(BCM_TRIUMPH3_SUPPORT)
         (ingress_table == VLAN_XLATE_EXTDm)  ||
         (ingress_table == MPLS_ENTRY_EXTDm)  ||
         (ingress_table == L3_TUNNELm)  ||
         (ingress_table == L3_ENTRY_2m)  ||
         (ingress_table == L3_ENTRY_4m)  ||
         (ingress_table == EXT_IPV4_UCAST_WIDEm)  ||
         (ingress_table == EXT_IPV6_128_UCAST_WIDEm)  ||
         (ingress_table == EXT_FP_POLICYm)  ||
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
         (ingress_table == ING_VSANm) ||
         (ingress_table == L3_ENTRY_IPV4_MULTICASTm) ||
         (ingress_table == L3_ENTRY_IPV6_MULTICASTm) ||
         (ingress_table == L3_DEFIPm) ||
         (ingress_table == L3_DEFIP_PAIR_128m) ||
         (ingress_table == L3_DEFIP_ALPM_IPV4_1m) ||
         (ingress_table == L3_DEFIP_ALPM_IPV6_64_1m) ||
         (ingress_table == L3_DEFIP_ALPM_IPV6_128m) ||
         (ingress_table == L3_DEFIP_AUX_SCRATCHm) ||
#endif
         (ingress_table == VFP_POLICY_TABLEm)  ||
         (ingress_table == MPLS_ENTRYm)  ||
         (ingress_table == SOURCE_VPm)  ||
         (ingress_table == L3_IIFm)  ||
         (ingress_table == VRFm)  ||
         (ingress_table == VFIm)  ||
         (ingress_table == VLAN_TABm))) {
          LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                    (BSL_META_U(unit,
                                "Invalid Flex Counter Ingress Memory %s\n"),
                     SOC_MEM_UFNAME(unit, ingress_table)));
          return BCM_E_PARAM;
    }
    ingress_entry_data_size = WORDS2BYTES(BYTES2WORDS(
                              SOC_MEM_INFO(unit, ingress_table).bytes));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Deallocating IngressCounter Table:%s:Index:%d:"
                           " ENTRY_BYTES:%d \n"),
               SOC_MEM_UFNAME(unit, ingress_table),
               index,ingress_entry_data_size));
    ingress_entry_data = sal_alloc(ingress_entry_data_size,"ingress_table");
    if (ingress_entry_data == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Failed to allocate memory for Table:%s "),
                   SOC_MEM_UFNAME(unit, ingress_table)));
        return BCM_E_INTERNAL;
    }
    sal_memset(ingress_entry_data,0,ingress_entry_data_size);
    if (soc_mem_read(unit, ingress_table, MEM_BLOCK_ANY,
                 _bcm_esw_stat_flex_table_index_map(unit,ingress_table,index),
                 ingress_entry_data) == SOC_E_NONE) {
        if (soc_mem_field_valid(unit,ingress_table,VALIDf)) {
            if (soc_mem_field32_get(unit,ingress_table,ingress_entry_data,
                                    VALIDf) == 0) {
                LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "Table %s  with index %d is Not valid \n"),
                           SOC_MEM_UFNAME(unit, ingress_table),index));
                sal_free(ingress_entry_data);
                return BCM_E_PARAM;
            }
        }
        _bcm_esw_get_flex_counter_fields_values(
                 unit,index,ingress_table,ingress_entry_data,
                 &offset_mode,&pool_number,&base_idx);
        if ((offset_mode == 0) && (base_idx == 0)) {
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Table:%s:Index %d IsNotConfiguredForFlexCounter\n"),
                        SOC_MEM_UFNAME(unit, ingress_table),index));
             sal_free(ingress_entry_data);
             return BCM_E_NOT_FOUND;/*Either NotConfigured/deallocated before*/
        }
        
        /* Decrement reference count */
        BCM_STAT_FLEX_COUNTER_LOCK(unit);
        flex_base_index_reference_count[unit][bcmStatFlexDirectionIngress]
                                       [pool_number][base_idx]--;
        flex_pool_stat[unit][bcmStatFlexDirectionIngress][pool_number].
                       attached_entries -= flex_ingress_modes[unit]
                                           [offset_mode].total_counters;
        BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
 
        /* Clear Counter Values */
        if ((flex_base_index_reference_count[unit]
                [bcmStatFlexDirectionIngress] [pool_number][base_idx]) == 0 ) {
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Clearing Counter Tables %s contents:Offset:%d Len:%d\n"),
                       SOC_MEM_UFNAME(unit, ingress_table),
                       base_idx,
                       flex_ingress_modes[unit][offset_mode].total_counters));
            flex_ctr_offset_info.all_counters_flag =  1;
            _bcm_esw_stat_flex_set(unit,index,ingress_table, -1,
                                   flex_ctr_offset_info,&flex_entries,
                                   offset_mode,pool_number,base_idx,
                                   bcmStatFlexDirectionIngress);
        }

        /* Reset Table Values */
        _bcm_esw_set_flex_counter_fields_values(
                 unit,index,ingress_table,ingress_entry_data,0,0,0);
        if (soc_mem_write(unit,ingress_table, MEM_BLOCK_ALL,
                          _bcm_esw_stat_flex_table_index_map(unit,ingress_table,index),
                          ingress_entry_data) != SOC_E_NONE) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Table:%s:Index %d encounter some problem \n"),
                       SOC_MEM_UFNAME(unit, ingress_table),index));
            sal_free(ingress_entry_data);
            return    BCM_E_INTERNAL;
        }

       LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                 (BSL_META_U(unit,
                             "Deallocated for Table:%sIndex:%d:"
                              "mode %d reference_count %d\n"),
                  SOC_MEM_UFNAME(unit, ingress_table),index,offset_mode,
                  flex_base_index_reference_count[unit]
                  [bcmStatFlexDirectionIngress][pool_number][base_idx]));
        if(_bcm_esw_stat_flex_get_ingress_object(
           unit,ingress_table,index,ingress_entry_data,&object) != BCM_E_NONE) {
           sal_free(ingress_entry_data);
           return BCM_E_INTERNAL;
        }
        sal_free(ingress_entry_data);
        
        /* Disable flex pool for this stat on no flex entries */
        if (flex_pool_stat[unit][bcmStatFlexDirectionIngress][pool_number].
                           attached_entries == 0) {
            _bcm_esw_stat_flex_enable_pool(
                     unit,
                     bcmStatFlexDirectionIngress,
                     _pool_ctr_register[bcmStatFlexDirectionIngress]
                                       [pool_number],
                     0);
        }
        _bcm_esw_stat_get_counter_id(
                      flex_ingress_modes[unit][offset_mode].group_mode,
                      object,offset_mode,pool_number,base_idx,&stat_counter_id);
        if (flex_base_index_reference_count[unit][bcmStatFlexDirectionIngress]
                                           [pool_number][base_idx] == 0) {
            if (_bcm_esw_stat_flex_insert_stat_id(
                        local_scache_ptr[unit],stat_counter_id) != BCM_E_NONE) {
                LOG_WARN(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "WARMBOOT: Couldnot add entry in scache memory."
                                      "Attach it\n")));
            }
        }
        return BCM_E_NONE;
    }
    sal_free(ingress_entry_data);
    return    BCM_E_NOT_FOUND;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_destroy_ingress_table_counters
 * Description:
 *      Destroy Igresss accounting table's statistics completely
 * Parameters:
 *      unit                  - (IN) unit number
 *      ingress_table         - (IN) Flex Accounting Table
 *      offset_mode           - (IN) Flex offset mode for Accounting Object
 *      base_idx              - (IN) Flex Base Index for Accounting Object
 *      pool_number           - (IN) Flex Pool Number for Accounting Object
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_destroy_ingress_table_counters(
            int                  unit,
            soc_mem_t            ingress_table,
            bcm_stat_object_t    object,
            bcm_stat_flex_mode_t offset_mode,
            uint32               base_idx,
            uint32               pool_number)
{
    uint32                          free_count=0;
    uint32                          alloc_count=0;
    uint32                          largest_free=0;
    uint32                          used_by_table=0;
    uint32                          stat_counter_id=0;

    if (flex_ingress_modes[unit][offset_mode].available==0) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "flex counter mode %d not configured yet\n"),
                   offset_mode));
        return BCM_E_NOT_FOUND;
    }
    if (shr_aidxres_list_elem_state(
            flex_aidxres_list_handle[unit][bcmStatFlexDirectionIngress]
            [pool_number],
            base_idx) != BCM_E_EXISTS) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Wrong base index %u \n"),
                   base_idx));
        return BCM_E_NOT_FOUND;
    }
    if (flex_base_index_reference_count[unit][bcmStatFlexDirectionIngress]
                                       [pool_number][base_idx] != 0) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Reference count is  %d.. Please detach entries first..\n"),
                   flex_ingress_modes[unit][offset_mode].reference_count));
        return BCM_E_FAIL;
    }
    switch(ingress_table) {
    case PORT_TABm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_PORT_TABLE;
         break;
    case VFP_POLICY_TABLEm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_VFP_POLICY_TABLE;
         break;
         /* VLAN and VFI shares same pool */
    case VLAN_TABm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_VLAN_TABLE;
         break;
    case VFIm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_VFI_TABLE;
         break;
         /* VRF and MPLS_VC_LABEL shares same pool */
    case VRFm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_VRF_TABLE;
         break;
    case MPLS_ENTRYm: 
#if defined(BCM_TRIUMPH3_SUPPORT)
    case MPLS_ENTRY_EXTDm: 
#endif
         used_by_table = FLEX_COUNTER_POOL_USED_BY_MPLS_ENTRY_TABLE;
         break;
    case VLAN_XLATEm:
#if defined(BCM_TRIUMPH3_SUPPORT)
    case VLAN_XLATE_EXTDm:
#endif
         used_by_table = FLEX_COUNTER_POOL_USED_BY_VLAN_XLATE_TABLE;
         break;
         /* L3_IIF and SOURCE_VP shares same pool*/
    case L3_IIFm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_L3_IIF_TABLE;
         break;
    case SOURCE_VPm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_SOURCE_VP_TABLE;
         break;
#if defined(BCM_TRIUMPH3_SUPPORT)
    case L3_TUNNELm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_L3_TUNNEL_TABLE;
         break;
    case L3_ENTRY_2m:
    case L3_ENTRY_4m:
    case EXT_IPV4_UCAST_WIDEm:
    case EXT_IPV6_128_UCAST_WIDEm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_L3_ENTRY_TABLE;
         break;
    case EXT_FP_POLICYm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_EXT_FP_POLICY_TABLE;
         break;
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    case ING_VSANm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_VSAN_TABLE;
         break;
    case L3_ENTRY_IPV4_MULTICASTm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_FCOE_TABLE;
         break;
    case L3_DEFIPm:
    case L3_DEFIP_PAIR_128m:
    case L3_DEFIP_ALPM_IPV4_1m:
    case L3_DEFIP_ALPM_IPV6_64_1m:
    case L3_DEFIP_ALPM_IPV6_128m:
    case L3_DEFIP_AUX_SCRATCHm:
         used_by_table = FLEX_COUNTER_POOL_USED_BY_L3_ROUTE_TABLE;
         break;
#endif
    default:
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Invalid Table is passed %d \n"),
                    ingress_table));
         return BCM_E_INTERNAL;
    }
    if (shr_aidxres_list_free(flex_aidxres_list_handle
                              [unit][bcmStatFlexDirectionIngress][pool_number],
                              base_idx) != BCM_E_NONE) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Freeing memory Table:%s:encounter some problem due to entry not found or some other issue  \n"),
                   SOC_MEM_UFNAME(unit, ingress_table)));
        return    BCM_E_NOT_FOUND;
    }
    _bcm_esw_stat_get_counter_id(
                  flex_ingress_modes[unit][offset_mode].group_mode,
                  object,offset_mode,pool_number,base_idx,&stat_counter_id);
    if (_bcm_esw_stat_flex_delete_stat_id(
                 local_scache_ptr[unit],stat_counter_id) != BCM_E_NONE) {
        LOG_WARN(BSL_LS_BCM_FLEXCTR,
                 (BSL_META_U(unit,
                             "WARMBOOT: Couldnot Delete entry in scache memory.\n")));
    }
    shr_aidxres_list_state(flex_aidxres_list_handle
                           [unit][bcmStatFlexDirectionIngress][pool_number],
                           NULL,NULL,NULL,NULL,
                           &free_count,&alloc_count,&largest_free,NULL);
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Current Pool status free_count:%d alloc_count:%d"
                           "largest_free:%d used_by_tables:%d used_entries:%d\n"),
               free_count,alloc_count,largest_free,
               flex_pool_stat[unit][bcmStatFlexDirectionIngress]
               [pool_number].used_by_tables,
               flex_pool_stat[unit][bcmStatFlexDirectionIngress]
               [pool_number].used_entries));
    flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                  [pool_number].used_entries -= flex_ingress_modes[unit]
                                                [offset_mode].total_counters;
    if (flex_pool_stat[unit][bcmStatFlexDirectionIngress][pool_number].
        used_entries == 0) {
        flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                      [pool_number].used_by_tables &= ~used_by_table;
        SHR_BITCLR(flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                      [pool_number].used_by_objects, object);
    }
    BCM_STAT_FLEX_COUNTER_LOCK(unit);
    flex_ingress_modes[unit][offset_mode].reference_count--;
    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
    return    BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_attach_egress_table_counters
 * Description:
 *      Atach i.e. Enable Egresss accounting table's statistics
 * Parameters:
 *      unit                  - (IN) unit number
 *      egress_table          - (IN) Flex Accounting Table
 *      index                 - (IN) Flex Accounting Table's Index
 *      mode                  - (IN) Flex offset mode for Accounting Object
 *      base_idx              - (IN) Flex Base Index for Accounting Object
 *      pool_number           - (IN) Flex Pool Number for Accounting Object
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_attach_egress_table_counters(
            int                  unit,
            soc_mem_t            egress_table,
            uint32               index,
            bcm_stat_flex_mode_t mode,
            uint32               base_idx,
            uint32               pool_number)
{
    uint32               egress_entry_data_size=0;
    void                 *egress_entry_data=NULL;
    bcm_stat_flex_mode_t offset_mode_l={0};
    bcm_stat_object_t    object=bcmStatObjectEgrPort;
    uint32               stat_counter_id=0;
    uint32               base_idx_l=0;
    uint32               pool_number_l=0;

    if (mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   mode));
        return BCM_E_PARAM;
    }
    if (flex_egress_modes[unit][mode].available==0) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "flex CounterMode:%d:Not configured yet\n"),
                   mode));
        return BCM_E_NOT_FOUND;
    }
    if (shr_aidxres_list_elem_state(flex_aidxres_list_handle
                                    [unit][bcmStatFlexDirectionEgress]
                                    [pool_number],base_idx) != BCM_E_EXISTS) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Wrong base index %u \n"),
                   base_idx));
        return BCM_E_NOT_FOUND;
    }
    egress_entry_data_size = WORDS2BYTES(BYTES2WORDS(
                             SOC_MEM_INFO(unit, egress_table).bytes));
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          ".Allocating EgressCounter Table:%s:Index:%d:Mode:%d"
                           " ENTRY_BYTES %d\n"), 
               SOC_MEM_UFNAME(unit,egress_table),index,mode,
               egress_entry_data_size));
    egress_entry_data = sal_alloc(egress_entry_data_size,"egress_table");
    if (egress_entry_data == NULL) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Failed to allocate memory for Table:%s "),
                   SOC_MEM_UFNAME(unit, egress_table)));
        return BCM_E_INTERNAL;
    }
    sal_memset(egress_entry_data,0,SOC_MEM_INFO(unit, egress_table).bytes);

    if (soc_mem_read(unit, egress_table, MEM_BLOCK_ANY,
                _bcm_esw_stat_flex_table_index_map(unit,egress_table,index),
                egress_entry_data) != SOC_E_NONE) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Read failure for Table %s with index %d \n"),
                   SOC_MEM_UFNAME(unit, egress_table),index));
        sal_free(egress_entry_data);
        return BCM_E_INTERNAL;
    }
    if (soc_mem_field_valid(unit,egress_table,VALIDf)) {
        if (soc_mem_field32_get(unit,egress_table,egress_entry_data,
                                VALIDf)==0) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Table %s  with index %d is Not valid \n"),
                       SOC_MEM_UFNAME(unit, egress_table),index));
            sal_free(egress_entry_data);
            return BCM_E_PARAM;
        }
    }
    _bcm_esw_get_flex_counter_fields_values(
                 unit,index,egress_table,egress_entry_data,
                 &offset_mode_l,&pool_number_l,&base_idx_l);
    if (base_idx_l != 0) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Table:%s HasAlreadyAllocatedWithIndex:%d base %d mode %d."
                               "First dealloc it \n"),SOC_MEM_UFNAME(unit, egress_table),
                   index,base_idx_l,offset_mode_l));
        sal_free(egress_entry_data);
        return BCM_E_EXISTS;/*Either Not configured or deallocated before*/
    }
    _bcm_esw_set_flex_counter_fields_values(
             unit,index,egress_table,egress_entry_data,mode,pool_number,base_idx);
    if (soc_mem_write(unit,egress_table, MEM_BLOCK_ALL,
                       _bcm_esw_stat_flex_table_index_map(unit,egress_table,index),
                       egress_entry_data) != SOC_E_NONE) {
        sal_free(egress_entry_data);
        return BCM_E_INTERNAL;
    }
    if(_bcm_esw_stat_flex_get_egress_object(unit,egress_table,
       index,egress_entry_data,&object) != BCM_E_NONE) {
       sal_free(egress_entry_data);
       return BCM_E_INTERNAL;
    }
    sal_free(egress_entry_data);
    _bcm_esw_stat_get_counter_id(
                  flex_egress_modes[unit][mode].group_mode,
                  object,mode,pool_number,base_idx,&stat_counter_id);
    if (flex_base_index_reference_count[unit][bcmStatFlexDirectionEgress]
                                       [pool_number][base_idx] == 0) {
        if (_bcm_esw_stat_flex_delete_stat_id(local_scache_ptr[unit],
                                              stat_counter_id) != BCM_E_NONE) {
            LOG_WARN(BSL_LS_BCM_FLEXCTR,
                     (BSL_META_U(unit,
                                 "WARMBOOT:Couldnot Delete entry in scache memory\n")));
        }
    }
    BCM_STAT_FLEX_COUNTER_LOCK(unit);
    flex_base_index_reference_count[unit][bcmStatFlexDirectionEgress]
                                   [pool_number][base_idx]++;
    if (flex_pool_stat[unit][bcmStatFlexDirectionEgress][pool_number].
         attached_entries == 0) {                              

        _bcm_esw_stat_flex_enable_pool(
                 unit,bcmStatFlexDirectionEgress,
                 _pool_ctr_register[bcmStatFlexDirectionEgress][pool_number],1);
    }
    flex_pool_stat[unit][bcmStatFlexDirectionEgress][pool_number].
                       attached_entries += flex_egress_modes[unit]
                                           [mode].total_counters;
    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_pool_operation
 * Description:
 *      Allocated or Deallocate Pool Counters
 * Parameters:
 *      unit                  - (IN) unit number
 *      flex_pool_attributes  - (IN) Flex Pool Attributes
 *           module_type           - (IN) Flex Module Type (Not Used Currently)
 *           pool_size             - (IN) Flex Pool Size
 *           flags                 - (IN) Combination of below flags
 *                                        1)Either
 *                                               BCM_FLEX_INGRESS_POOL(default)
 *                                                    OR
 *                                               BCM_FLEX_EGRESS_POOL
 *                                        2)Either
 *                                               BCM_FLEX_ALLOCATE_POOL(default)
 *                                                    OR
 *                                               BCM_FLEX_DEALLOCATE_POOL
 *                                        3)Either
 *                                               BCM_FLEX_SHARED_POOL(default)
 *                                                    OR
 *                                               BCM_FLEX_NOT_SHARED_POOL
 *                                          (NotApplicable in deallocation case)
 *           pool_id               - (IN/OUT) Flex Pool ID
 *           offset                - (IN/OUT) Flex Pool Base Index
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t  _bcm_esw_stat_flex_pool_operation(
              int                            unit,
              bcm_stat_flex_pool_attribute_t *flex_pool_attribute)
{
    bcm_stat_flex_direction_t direction = bcmStatFlexDirectionIngress;
    uint32                    num_flex_pools=SOC_INFO(unit).
                                             num_flex_ingress_pools;
    uint32                    size_pool= SOC_INFO(unit).size_flex_ingress_pool;
    uint32                    block_factor=15;
    uint32                    pool_number_l=0;
    uint32                    default_pool_number=0;
    uint32                    used_by_table_selection_criteria=
                              TABLE_INDEPENDENT_POOL_MASK;
    uint32                    shared_pool=1;
    uint32                    base_idx_l=0;
    uint32                    alloc_count=0;

    if (flex_pool_attribute == NULL) {
        return BCM_E_PARAM;
    }
    if ((flex_pool_attribute->flags & BCM_FLEX_EGRESS_POOL) == 
         BCM_FLEX_EGRESS_POOL) {
         direction = bcmStatFlexDirectionEgress;
         num_flex_pools=SOC_INFO(unit).num_flex_egress_pools;
         size_pool= SOC_INFO(unit).size_flex_egress_pool;
    } 
    if ((flex_pool_attribute->flags & BCM_FLEX_NOT_SHARED_POOL) ==
         BCM_FLEX_NOT_SHARED_POOL) {
         shared_pool=0;
    }
    if (flex_pool_attribute->pool_size > size_pool) {
        return BCM_E_PARAM;
    }
    if ((flex_pool_attribute->flags & BCM_FLEX_DEALLOCATE_POOL) ==
         BCM_FLEX_DEALLOCATE_POOL) {
         pool_number_l =  (flex_pool_attribute->pool_id);
         if (pool_number_l > num_flex_pools) {
             return BCM_E_PARAM;
         }
         if (flex_pool_stat[unit][direction][pool_number_l].
             used_by_tables != TABLE_INDEPENDENT_POOL_MASK) {
             return BCM_E_PARAM;
         }
         BCM_STAT_FLEX_COUNTER_LOCK(unit);
         if (shr_aidxres_list_free(
             flex_aidxres_list_handle[unit][direction][pool_number_l],
             flex_pool_attribute->offset) != BCM_E_NONE) {
             BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
             return BCM_E_PARAM;
        }
        flex_pool_stat[unit][direction][pool_number_l].
                      used_entries -= flex_pool_attribute->pool_size;
        shr_aidxres_list_state(flex_aidxres_list_handle
                               [unit][direction][pool_number_l],
                               NULL,NULL,NULL,NULL,
                               NULL,&alloc_count,NULL,NULL);
        if ((flex_pool_stat[unit][direction][pool_number_l].
             used_entries == 0) ||
             (alloc_count == 0)) {
             _bcm_esw_stat_flex_enable_pool(
                  unit,
                  direction,
                  _pool_ctr_register[direction][pool_number_l],0);
             flex_pool_stat[unit][direction][pool_number_l].used_entries=0;
             flex_pool_stat[unit][direction][pool_number_l].used_by_tables = 0;
             SHR_BITCLR_RANGE(flex_pool_stat[unit][direction][pool_number_l].
                                used_by_objects, 0, bcmStatObjectMaxValue);
             /* Destroy Existing One */
             shr_aidxres_list_destroy(flex_aidxres_list_handle[unit]
                                      [direction][pool_number_l]);
             /* Re Create it back to original one */
             if (shr_aidxres_list_create(&flex_aidxres_list_handle
                 [unit][direction][pool_number_l],
                 0,size_pool-1,
                 0,size_pool-1,
                 8, /* Max 256 counters */
                 "flex-counter") != BCM_E_NONE) {
                 LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                           (BSL_META_U(unit,
                                       "Unrecoverable error. Couldn'tCreate "
                                        "AllignedList:FlexCounter\n")));
                 BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
                 return BCM_E_INTERNAL;
             }
             shr_aidxres_list_reserve_block(
                 flex_aidxres_list_handle[unit][direction][pool_number_l],
                 0, (1 << 1));
        }
        BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
    } else {
        BCM_STAT_FLEX_COUNTER_LOCK(unit);
        do {
           if ((flex_pool_stat[unit][direction][pool_number_l].
                used_by_tables == 0) ||
                ((shared_pool == 1) && 
                 (flex_pool_stat[unit][direction][pool_number_l].
                  used_by_tables & used_by_table_selection_criteria))) {

                if ((flex_pool_stat[unit][direction][pool_number_l].
                     used_by_tables == 0)) {
                     /* Destroy Existing One */
                     shr_aidxres_list_destroy(flex_aidxres_list_handle[unit]
                                              [direction][pool_number_l]);
                     /* Re Create it */
                     if (shr_aidxres_list_create(&flex_aidxres_list_handle
                         [unit][direction][pool_number_l],
                         0,size_pool-1,
                         0,size_pool-1,
                         block_factor,
                         "flex-counter") != BCM_E_NONE) {
                         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                                   (BSL_META_U(unit,
                                               "Unrecoverable error. Couldn'tCreate "
                                                "AllignedList:FlexCounter\n")));
                         BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
                         return BCM_E_INTERNAL;
                   }
                }
                shr_aidxres_list_state(flex_aidxres_list_handle
                                       [unit][direction][pool_number_l],
                                       NULL,NULL,NULL,NULL,
                                       NULL,&alloc_count,NULL,NULL);
                if (shr_aidxres_list_alloc_block(
                    flex_aidxres_list_handle[unit][direction][pool_number_l], 
                    flex_pool_attribute->pool_size, 
                    &base_idx_l) == BCM_E_NONE) {
                    if ((flex_pool_stat[unit][direction][pool_number_l].
                         used_entries == 0) ||
                         (alloc_count == 0)) {
                         _bcm_esw_stat_flex_enable_pool(
                              unit,
                              direction,
                              _pool_ctr_register[direction][pool_number_l],1);
                    }
                    flex_pool_stat[unit][direction][pool_number_l].
                         used_by_tables = TABLE_INDEPENDENT_POOL_MASK;
                    flex_pool_stat[unit][direction][pool_number_l].
                         used_entries += flex_pool_attribute->pool_size;
                    flex_pool_attribute->offset  = (int)base_idx_l;
                    flex_pool_attribute->pool_id = (int)pool_number_l;
                }
            } else {
               BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
               return BCM_E_RESOURCE;
            }
            pool_number_l = (pool_number_l+1) % num_flex_pools;
        } while(pool_number_l != default_pool_number);
        BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_stat_flex_create_egress_table_counters
 * Description:
 *      Create and Reserve Flex Counter Space for Egresss accounting table 
 * Parameters:
 *      unit                  - (IN) unit number
 *      egress_table          - (IN) Flex Accounting Table
 *      mode                  - (IN) Flex offset mode for Accounting Object
 *      base_idx              - (IN) Flex Base Index for Accounting Object
 *      pool_number           - (IN) Flex Pool Number for Accounting Object
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_create_egress_table_counters(
            int                  unit,
            soc_mem_t            egress_table,
            bcm_stat_object_t    object,
            bcm_stat_flex_mode_t mode,
            uint32               *base_idx,
            uint32               *pool_number)
{
    uint32            base_idx_l=0;
    uint32            pool_number_l=0;
    uint32            default_pool_number=0;
    uint32            used_by_table=0;
    uint32            used_by_table_selection_criteria=0;
    uint32            free_count=0;
    uint32            alloc_count=0;
    uint32            largest_free=0;
    uint32            stat_counter_id=0;
    uint32            num_flex_egress_pools = SOC_INFO(unit).
                                              num_flex_egress_pools;
    if (mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   mode));
        return BCM_E_PARAM;
    }
    if (flex_egress_modes[unit][mode].available==0) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "flex CounterMode:%d:Not configured yet\n"),
                   mode));
        return BCM_E_NOT_FOUND;
    }
    /* Below case statement can be avoided by passing arguements for pool_number
       and selection criteria But keeping it for better understanding 
       at same place */
    switch(egress_table) {
    case EGR_VLANm:
         default_pool_number = pool_number_l =
             FLEX_COUNTER_DEFAULT_EGR_VLAN_TABLE_POOL_NUMBER;
         used_by_table_selection_criteria = used_by_table =
             FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_TABLE;
         break;
    case EGR_VFIm:
         default_pool_number = pool_number_l =
             FLEX_COUNTER_DEFAULT_EGR_VFI_TABLE_POOL_NUMBER;
         used_by_table_selection_criteria = used_by_table =
             FLEX_COUNTER_POOL_USED_BY_EGR_VFI_TABLE;
         break;
    case EGR_L3_NEXT_HOPm:
         default_pool_number = pool_number_l =
             FLEX_COUNTER_DEFAULT_EGR_L3_NEXT_HOP_TABLE_POOL_NUMBER;
         used_by_table_selection_criteria = used_by_table =
             FLEX_COUNTER_POOL_USED_BY_EGR_L3_NEXT_HOP_TABLE;
         break;
    case EGR_VLAN_XLATEm:
#if defined(BCM_TRIDENT2_SUPPORT)
         if (SOC_IS_TD2_TT2(unit)) {
             default_pool_number = pool_number_l =
                 FLEX_COUNTER_TD2_DEFAULT_EGR_VLAN_XLATE_TABLE_POOL_NUMBER;
         } else
#endif
         {
             default_pool_number = pool_number_l =
                 FLEX_COUNTER_DEFAULT_EGR_VLAN_XLATE_TABLE_POOL_NUMBER;
         }        
         used_by_table_selection_criteria = used_by_table =
             FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_XLATE_TABLE;
         break;
   case EGR_PORTm:
         default_pool_number = pool_number_l =
             FLEX_COUNTER_DEFAULT_EGR_PORT_TABLE_POOL_NUMBER;
         used_by_table_selection_criteria = used_by_table =
             FLEX_COUNTER_POOL_USED_BY_EGR_PORT_TABLE;
         break;
   case EGR_DVP_ATTRIBUTE_1m:
#if defined(BCM_TRIDENT2_SUPPORT)
         if (SOC_IS_TD2_TT2(unit)) {
             default_pool_number = pool_number_l =
                 FLEX_COUNTER_TD2_DEFAULT_EGR_DVP_ATTRIBUTE_1_TABLE_POOL_NUMBER;
         } else
#endif
         {
             default_pool_number = pool_number_l =
                 FLEX_COUNTER_DEFAULT_EGR_DVP_ATTRIBUTE_1_TABLE_POOL_NUMBER;
         }     
         used_by_table_selection_criteria = used_by_table =
             FLEX_COUNTER_POOL_USED_BY_EGR_DVP_ATTRIBUTE_1_TABLE;
         break;
   case EGR_NAT_PACKET_EDIT_INFOm:
#if defined(BCM_TRIDENT2_SUPPORT)
         if (SOC_IS_TD2_TT2(unit)) {
            default_pool_number = pool_number_l =
                FLEX_COUNTER_TD2_DEFAULT_L3_NAT_TABLE_POOL_NUMBER;
         } else
#endif
         {
             default_pool_number = pool_number_l =
                 FLEX_COUNTER_DEFAULT_L3_NAT_TABLE_POOL_NUMBER;
         } 
         used_by_table_selection_criteria = used_by_table =
             FLEX_COUNTER_POOL_USED_BY_EGR_L3_NAT_TABLE;
         break;

   default:
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Invalid Table is passed %d \n"),
                    egress_table));
         return BCM_E_INTERNAL;
   }
   do {
      /* Either free or being used by port table only */
      if ((flex_pool_stat[unit][bcmStatFlexDirectionEgress]
           [pool_number_l].used_by_tables == 0) ||
          (flex_pool_stat[unit][bcmStatFlexDirectionEgress]
           [pool_number_l].used_by_tables & used_by_table_selection_criteria)) {
           if (shr_aidxres_list_alloc_block(flex_aidxres_list_handle
                   [unit][bcmStatFlexDirectionEgress][pool_number_l],
                   flex_egress_modes[unit][mode].total_counters,
                   &base_idx_l) == BCM_E_NONE) {
               flex_pool_stat[unit][bcmStatFlexDirectionEgress]
                    [pool_number_l].used_by_tables |= used_by_table;
               SHR_BITSET(flex_pool_stat[unit][bcmStatFlexDirectionEgress]
                    [pool_number_l].used_by_objects, object);
               flex_pool_stat[unit][bcmStatFlexDirectionEgress]
                    [pool_number_l].used_entries += flex_egress_modes
                                                    [unit][mode].total_counters;
               LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "Allocated  counter Table:%s with pool_number:%d"
                                      "mode:%d base_idx:%d ref_count %d\n"),
                          SOC_MEM_UFNAME(unit, egress_table),
                          pool_number_l,mode,base_idx_l,
                          flex_egress_modes[unit][mode].reference_count));
               shr_aidxres_list_state(
                   flex_aidxres_list_handle[unit][bcmStatFlexDirectionEgress]
                                           [pool_number_l],
                   NULL,NULL,NULL,NULL,&free_count,&alloc_count,&largest_free,
                   NULL);
               LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "Current Pool status free_count:%d alloc_count:%d"
                                      "largest_free:%d used_by_tables:%d"
                                      "used_entries:%d\n"),
                          free_count,alloc_count,largest_free,
                          flex_pool_stat[unit][bcmStatFlexDirectionEgress]
                          [pool_number_l].used_by_tables,
                          flex_pool_stat[unit][bcmStatFlexDirectionEgress]
                          [pool_number_l].used_entries));
               *base_idx = base_idx_l;
               *pool_number = pool_number_l;
               _bcm_esw_stat_get_counter_id(
                             flex_egress_modes[unit][mode].group_mode,
                             object,mode,pool_number_l,base_idx_l,
                             &stat_counter_id);
               BCM_STAT_FLEX_COUNTER_LOCK(unit);
               flex_egress_modes[unit][mode].reference_count++;
               BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
               if (_bcm_esw_stat_flex_insert_stat_id(
                   local_scache_ptr[unit], stat_counter_id) != BCM_E_NONE) {
                   LOG_WARN(BSL_LS_BCM_FLEXCTR,
                            (BSL_META_U(unit,
                                        "WARMBOOT:Couldnot add entry in scache memory"
                                         "Attach it\n")));
               }
               return BCM_E_NONE;
           }
      }
      pool_number_l = (pool_number_l+1) % num_flex_egress_pools;
    } while(pool_number_l != default_pool_number);
    LOG_ERROR(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Pools exhausted for Table:%s\n"),
               SOC_MEM_UFNAME(unit, egress_table)));
    return BCM_E_FAIL; /*or BCM_E_RESOURCE */
}
/*
 * Function:
 *      _bcm_esw_stat_flex_attach_ingress_table_counters_update
 * Description:
 *      Attach i.e. Update Ingresss accounting table's statistics
 * Parameters:
 *      unit                  - (IN) unit number
 *      pool_number           - (IN) Flex Pool Number for Accounting Object
 *      base_idx              - (IN) Flex Base Index for Accounting Object
 *      mode                  - (IN) Flex offset mode for Accounting Object
 * Return Value:
 *      BCM_E_NONE
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_attach_ingress_table_counters_update(
            int                  unit,
            uint32               pool_number, 
            uint32               base_idx, 
            bcm_stat_flex_mode_t mode) 
{
    if (pool_number < 0 || pool_number > 15) {
        /* Return with no update */
        return BCM_E_NONE;
    }

    BCM_STAT_FLEX_COUNTER_LOCK(unit);
    flex_base_index_reference_count[unit][bcmStatFlexDirectionIngress]
                                   [pool_number][base_idx]++;
    if (flex_pool_stat[unit][bcmStatFlexDirectionIngress][pool_number].
            attached_entries == 0) {

        _bcm_esw_stat_flex_enable_pool(
               unit,bcmStatFlexDirectionIngress,
               _pool_ctr_register[bcmStatFlexDirectionIngress][pool_number],1);
    }   
    flex_pool_stat[unit][bcmStatFlexDirectionIngress][pool_number].
                       attached_entries += flex_ingress_modes[unit]
                                           [mode].total_counters;
    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);

    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_detach_ingress_table_counters_update
 * Description:
 *      Manage flex reference count, clear counter values in the hardware 
 *      and disable flex pool for the stat applicable
 * Parameters:
 *      unit                  - (IN) unit number
 *      pool_number           - (IN) Flex Pool Number for Accounting Object
 *      base_idx              - (IN) Flex Base Index for Accounting Object
 *      mode                  - (IN) Flex offset mode for Accounting Object
 * Return Value:
 *      BCM_E_NONE
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_detach_ingress_table_counters_update(
            int                  unit,
            uint32               pool_number, 
            uint32               base_idx, 
            bcm_stat_flex_mode_t mode) 
{
    uint32                          flex_entries=0;
    uint32                          alloc_size=0;
    bcm_stat_flex_ctr_offset_info_t flex_ctr_offset_info={0};
    bcm_stat_flex_counter_value_t   *flex_counter_value=NULL;

    if (pool_number < 0 || pool_number > 15) {
        /* Return with no update */
        return BCM_E_NONE;
    }
    /* Decrement reference count */
    BCM_STAT_FLEX_COUNTER_LOCK(unit);
    flex_base_index_reference_count[unit][bcmStatFlexDirectionIngress]
                                       [pool_number][base_idx]--;
    flex_pool_stat[unit][bcmStatFlexDirectionIngress][pool_number].
                       attached_entries -= flex_ingress_modes[unit]
                                           [mode].total_counters;
    BCM_STAT_FLEX_COUNTER_UNLOCK(unit);

    /* Clear Counter Values */
    if ((flex_base_index_reference_count[unit]
            [bcmStatFlexDirectionIngress] [pool_number][base_idx]) == 0 ) {

        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Clearing Counter Tables for EXT_FP_POLICY: Offset:%d "
                               "Len:%d\n"),
                   base_idx,
                   flex_ingress_modes[unit][mode].total_counters));

        flex_ctr_offset_info.all_counters_flag =  1;
        alloc_size = sizeof(bcm_stat_flex_counter_value_t) *
                      flex_ingress_modes[unit][mode].total_counters;

        flex_counter_value = sal_alloc(alloc_size,"counter-table-values");
        if (flex_counter_value == NULL) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Failed: AllocateCounterMemoryForTable:EXT_FP_POLICY: "
                                   "pool_number:%d mode:%d base_idx:%d\n"),
                       pool_number, mode, base_idx)); 
            return BCM_E_INTERNAL;
        }

        sal_memset(flex_counter_value,0,alloc_size);
        _bcm_esw_ext_stat_flex_set(unit, pool_number, base_idx, mode, 
                                   EXT_FP_POLICYm, 1, flex_ctr_offset_info, 
                                   &flex_entries, flex_counter_value);
        _bcm_esw_ext_stat_flex_set(unit, pool_number, base_idx,mode, 
                                   EXT_FP_POLICYm, 0, flex_ctr_offset_info, 
                                   &flex_entries, flex_counter_value);
        sal_free(flex_counter_value);
    }

    /* Disable flex pool for this stat on no flex entries */
    if (flex_pool_stat[unit][bcmStatFlexDirectionIngress][pool_number].
                           attached_entries == 0) {
        _bcm_esw_stat_flex_enable_pool(
                 unit,
                 bcmStatFlexDirectionIngress,
                 _pool_ctr_register[bcmStatFlexDirectionIngress][pool_number],
                 0);
    }
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_attach_ingress_table_counters1
 * Description:
 *      Atach i.e. Enable Ingresss accounting table's statistics
 * Parameters:
 *      unit                  - (IN) unit number
 *      ingress_table         - (IN) Flex Accounting Table
 *      index                 - (IN) Flex Accounting Table's Index
 *      mode                  - (IN) Flex offset mode for Accounting Object
 *      base_idx              - (IN) Flex Base Index for Accounting Object
 *      pool_number           - (IN) Flex Pool Number for Accounting Object
 *      ingress_entry_data1   - (IN) Entry Data(Null or Valid)
 *                                   Null, table will be modified
 *                                   NON-Null, table won't be modified
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *          ###############################################################
 *          BE CAREFULL while using this routine(i.e. if possible don't use)
 *          ###############################################################
 *          Currently being used from field module to avoid two write operations
 *
 */
bcm_error_t _bcm_esw_stat_flex_attach_ingress_table_counters1(
            int                  unit,
            soc_mem_t            ingress_table,
            uint32               index,
            bcm_stat_flex_mode_t mode,
            uint32               base_idx,
            uint32               pool_number,
            void                 *ingress_entry_data1)
{
    uint32               ingress_entry_data_size=0;
    void                 *ingress_entry_data=NULL;
    void                 *ingress_entry_data_temp=NULL;
    bcm_stat_flex_mode_t offset_mode_l={0};
    bcm_stat_object_t    object=bcmStatObjectIngPort;
    uint32               stat_counter_id=0;
    uint32               base_idx_l=0;
    uint32               pool_number_l=0;

    if (mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   mode));
        return BCM_E_PARAM;
    }
    if (flex_ingress_modes[unit][mode].available==0) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "flex counter mode %d not configured yet\n"),
                   mode));
        return BCM_E_NOT_FOUND;
    }
    if (shr_aidxres_list_elem_state(
            flex_aidxres_list_handle[unit][bcmStatFlexDirectionIngress]
            [pool_number],
            base_idx) != BCM_E_EXISTS) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Wrong base index %u \n"),
                   base_idx));
        return BCM_E_NOT_FOUND;
    }

    ingress_entry_data_size = WORDS2BYTES(BYTES2WORDS(
                              SOC_MEM_INFO(unit, ingress_table).bytes));
    if (ingress_entry_data1  == NULL ) {
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              ".Attaching INGRESS counter for Table:%s with index:%d"
                               "mode:%d ENTRY_BYTES %d \n"),
                   SOC_MEM_UFNAME(unit,ingress_table),index,mode,
                   ingress_entry_data_size));
        ingress_entry_data = sal_alloc(ingress_entry_data_size,"ingress_table");
        if (ingress_entry_data == NULL) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Failed to allocate memory for Table:%s "),
                       SOC_MEM_UFNAME(unit, ingress_table)));
            return BCM_E_INTERNAL;
        }
        sal_memset(ingress_entry_data,0,
                   SOC_MEM_INFO(unit, ingress_table).bytes);
        if (soc_mem_read(unit, ingress_table, MEM_BLOCK_ANY,
                         _bcm_esw_stat_flex_table_index_map(unit,ingress_table,index),
                         ingress_entry_data) != SOC_E_NONE) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Read failure for Table %s with index %d \n"),
                       SOC_MEM_UFNAME(unit, ingress_table),index));
            sal_free(ingress_entry_data);
            return    BCM_E_INTERNAL;
        }
        ingress_entry_data_temp = ingress_entry_data;
    } else {
        ingress_entry_data_temp = ingress_entry_data1;
    }
    if (soc_mem_field_valid(unit,ingress_table,VALIDf)) {
        if (soc_mem_field32_get(unit,ingress_table,ingress_entry_data_temp,
            VALIDf)==0) {
            LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "Table %s  with index %d is Not valid \n"),
                       SOC_MEM_UFNAME(unit, ingress_table),index));
            if (ingress_entry_data1 == NULL) {
            sal_free(ingress_entry_data);
            }
            return BCM_E_PARAM;
        }
    }
    _bcm_esw_get_flex_counter_fields_values(
               unit,index,ingress_table,ingress_entry_data_temp,
               &offset_mode_l,&pool_number_l,&base_idx_l);
    if (base_idx_l != 0) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Table:%s Has already allocated with index:%d"
                               "base %d mode %d."
                               "First dealloc it \n"), SOC_MEM_UFNAME(unit, ingress_table),
                   index,base_idx_l,offset_mode_l));
        if (ingress_entry_data1 == NULL) {
            sal_free(ingress_entry_data);
        }
        return BCM_E_EXISTS;/*Either NotConfigured or deallocated before */
    }
    _bcm_esw_set_flex_counter_fields_values(
         unit,index,ingress_table,ingress_entry_data_temp,mode,pool_number,base_idx);
    if (ingress_entry_data1 == NULL) {
        if (soc_mem_write(unit, ingress_table, MEM_BLOCK_ALL,
                          _bcm_esw_stat_flex_table_index_map(unit,ingress_table,index),
                          ingress_entry_data_temp) != SOC_E_NONE) {
            sal_free(ingress_entry_data);
            return    BCM_E_INTERNAL;
        }
    }
    if(_bcm_esw_stat_flex_get_ingress_object(
       unit,ingress_table,index,ingress_entry_data_temp,&object) !=BCM_E_NONE) {
       if (ingress_entry_data1 == NULL) {
           sal_free(ingress_entry_data);
       }
       return BCM_E_INTERNAL;
    }
    if (ingress_entry_data1 == NULL) {
        sal_free(ingress_entry_data);
    }
    _bcm_esw_stat_get_counter_id(
                  flex_ingress_modes[unit][mode].group_mode,
                  object,mode,pool_number,base_idx,&stat_counter_id);
    if (flex_base_index_reference_count[unit][bcmStatFlexDirectionIngress]
                                       [pool_number][base_idx] == 0) {
        if (_bcm_esw_stat_flex_delete_stat_id(
            local_scache_ptr[unit],stat_counter_id) != BCM_E_NONE) {
            LOG_WARN(BSL_LS_BCM_FLEXCTR,
                     (BSL_META_U(unit,
                                 "WARMBOOT:Couldnot Delete entry in scache memory\n")));
        }
    }

    if (_bcm_esw_stat_flex_attach_ingress_table_counters_update(
        unit, pool_number, base_idx, mode) != BCM_E_NONE) {
        return BCM_E_INTERNAL;
    }

    return BCM_E_NONE; 
}
/*
 * Function:
 *      _bcm_esw_stat_flex_attach_ingress_table_counters
 * Description:
 *      Atach i.e. Enable Ingresss accounting table's statistics
 * Parameters:
 *      unit                  - (IN) unit number
 *      ingress_table         - (IN) Flex Accounting Table
 *      index                 - (IN) Flex Accounting Table's Index
 *      mode                  - (IN) Flex offset mode for Accounting Object
 *      base_idx              - (IN) Flex Base Index for Accounting Object
 *      pool_number           - (IN) Flex Pool Number for Accounting Object
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      Calls _bcm_esw_stat_flex_attach_ingress_table_counters1() with
 *      NULL as ingress_entry_data i.e. ingress table will be modified
 *
 */
bcm_error_t _bcm_esw_stat_flex_attach_ingress_table_counters(
            int                  unit,
            soc_mem_t            ingress_table,
            uint32               index,
            bcm_stat_flex_mode_t mode,
            uint32               base_idx,
            uint32               pool_number)
{
   return _bcm_esw_stat_flex_attach_ingress_table_counters1(
          unit, ingress_table, index, mode, base_idx, pool_number, NULL); 

}

/*
 * Function:
 *      _bcm_esw_stat_flex_create_ingress_table_counters
 * Description:
 *      Create and Reserve Flex Counter Space for Ingresss accounting table 
 * Parameters:
 *      unit                  - (IN) unit number
 *      ingress_table         - (IN) Flex Accounting Table
 *      mode                  - (IN) Flex offset mode for Accounting Object
 *      base_idx              - (IN) Flex Base Index for Accounting Object
 *      pool_number           - (IN) Flex Pool Number for Accounting Object
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_create_ingress_table_counters(
            int                  unit,
            soc_mem_t            ingress_table,
            bcm_stat_object_t    object,
            bcm_stat_flex_mode_t mode,
            uint32               *base_idx,
            uint32               *pool_number)
{
    uint32            base_idx_l=0;
    uint32            pool_number_l=0;
    uint32            default_pool_number=0;
    uint32            used_by_table=0;
    uint32            used_by_table_selection_criteria=0;
    uint32            free_count=0;
    uint32            alloc_count=0;
    uint32            largest_free=0;
    uint32            stat_counter_id=0;
    uint32            num_flex_ingress_pools=SOC_INFO(unit).
                                             num_flex_ingress_pools;
    ing_flex_object_config_t object_config;

    if (mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   mode));
        return BCM_E_PARAM;
    }
    if (flex_ingress_modes[unit][mode].available==0) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "flex counter mode %d not configured yet\n"),
                   mode));
        return BCM_E_NOT_FOUND;
    }

    /* Below case statement can be avoided by passing arguements for 
       pool_number and selection criteria But keeping it for better 
       understanding at same place */
    switch(ingress_table) {
    case PORT_TABm:
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_PORT_TABLE_POOL_NUMBER;
         used_by_table_selection_criteria=used_by_table =
                 FLEX_COUNTER_POOL_USED_BY_PORT_TABLE;
         break;
    case VFP_POLICY_TABLEm:
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_VFP_POLICY_TABLE_POOL_NUMBER;
         used_by_table_selection_criteria=used_by_table =
                 FLEX_COUNTER_POOL_USED_BY_VFP_POLICY_TABLE;
         break;
        /* VLAN and VFI shares same pool */
    case VLAN_TABm:
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_VLAN_TABLE_POOL_NUMBER;
         used_by_table = FLEX_COUNTER_POOL_USED_BY_VLAN_TABLE;
         used_by_table_selection_criteria=
                 (FLEX_COUNTER_POOL_USED_BY_VLAN_TABLE |
                  FLEX_COUNTER_POOL_USED_BY_VFI_TABLE);
         break;
    case VFIm:
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_VFI_TABLE_POOL_NUMBER;
         used_by_table = FLEX_COUNTER_POOL_USED_BY_VFI_TABLE;
         used_by_table_selection_criteria=
                 (FLEX_COUNTER_POOL_USED_BY_VLAN_TABLE |
                  FLEX_COUNTER_POOL_USED_BY_VFI_TABLE );
         break;
        /* VRF and MPLS_VC_LABEL shares same pool */
    case VRFm:
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_VRF_TABLE_POOL_NUMBER;
         used_by_table = FLEX_COUNTER_POOL_USED_BY_VRF_TABLE;
         used_by_table_selection_criteria=FLEX_COUNTER_POOL_USED_BY_VRF_TABLE;
         break;

        /* VLAN_XLATE and MPLS_TABLE shares same pool */
    case MPLS_ENTRYm: 
#if defined(BCM_TRIUMPH3_SUPPORT)
    case MPLS_ENTRY_EXTDm: 
#endif
         default_pool_number=pool_number_l=
               FLEX_COUNTER_DEFAULT_MPLS_ENTRY_TABLE_POOL_NUMBER;
         used_by_table =
               FLEX_COUNTER_POOL_USED_BY_MPLS_ENTRY_TABLE;
         used_by_table_selection_criteria=
               (FLEX_COUNTER_POOL_USED_BY_VLAN_XLATE_TABLE|
                FLEX_COUNTER_POOL_USED_BY_MPLS_ENTRY_TABLE);
         break;
    case VLAN_XLATEm:
#if defined(BCM_TRIUMPH3_SUPPORT)
    case VLAN_XLATE_EXTDm:
#endif
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_VLAN_XLATE_TABLE_POOL_NUMBER;
         used_by_table = FLEX_COUNTER_POOL_USED_BY_VLAN_XLATE_TABLE;
         used_by_table_selection_criteria=
                 (FLEX_COUNTER_POOL_USED_BY_VLAN_XLATE_TABLE|
                  FLEX_COUNTER_POOL_USED_BY_MPLS_ENTRY_TABLE);
         break;
        /* L3_IIF and SOURCE_VP shares same pool*/
    case L3_IIFm:
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_L3_IIF_TABLE_POOL_NUMBER;
         /* Time being keeping separate default pool for l3_iif and source_vp */
         used_by_table = FLEX_COUNTER_POOL_USED_BY_L3_IIF_TABLE;
         /* But IfRequired L3_IIF & SOURCE_VP counters can be shared in 
            same pool*/
         used_by_table_selection_criteria=
                 FLEX_COUNTER_POOL_USED_BY_L3_IIF_TABLE |
                 FLEX_COUNTER_POOL_USED_BY_SOURCE_VP_TABLE ;
         break;
    case L3_TUNNELm:
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_L3_TUNNEL_TABLE_POOL_NUMBER;
         used_by_table = FLEX_COUNTER_POOL_USED_BY_L3_TUNNEL_TABLE;
         used_by_table_selection_criteria=
                 FLEX_COUNTER_POOL_USED_BY_L3_TUNNEL_TABLE;
         break;
    case L3_ENTRY_2m:
    case L3_ENTRY_4m:
    case EXT_IPV4_UCAST_WIDEm:
    case EXT_IPV6_128_UCAST_WIDEm:
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_L3_ENTRY_TABLE_POOL_NUMBER;
         used_by_table = FLEX_COUNTER_POOL_USED_BY_L3_ENTRY_TABLE;
         used_by_table_selection_criteria=
                 FLEX_COUNTER_POOL_USED_BY_L3_ENTRY_TABLE;
         break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case L3_DEFIPm:
    case L3_DEFIP_PAIR_128m:
    case L3_DEFIP_ALPM_IPV4_1m:
    case L3_DEFIP_ALPM_IPV6_64_1m:
    case L3_DEFIP_ALPM_IPV6_128m:
    case L3_DEFIP_AUX_SCRATCHm:
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_L3_ROUTE_TABLE_POOL_NUMBER;
         used_by_table = FLEX_COUNTER_POOL_USED_BY_L3_ROUTE_TABLE;
         used_by_table_selection_criteria=
                 FLEX_COUNTER_POOL_USED_BY_L3_ROUTE_TABLE;
         break;
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:        
         default_pool_number=pool_number_l=
                FLEX_COUNTER_DEFAULT_FCOE_TABLE_POOL_NUMBER;
         used_by_table = FLEX_COUNTER_POOL_USED_BY_FCOE_TABLE;
         used_by_table_selection_criteria=
                 FLEX_COUNTER_POOL_USED_BY_FCOE_TABLE;
         break;
    case ING_VSANm:
         default_pool_number=pool_number_l=
                FLEX_COUNTER_DEFAULT_FCOE_TABLE_POOL_NUMBER;
         used_by_table = FLEX_COUNTER_POOL_USED_BY_VSAN_TABLE;
         used_by_table_selection_criteria=
                 FLEX_COUNTER_POOL_USED_BY_VSAN_TABLE;
         break;
#endif
    case EXT_FP_POLICYm:
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_EXT_FP_POLICY_TABLE_POOL_NUMBER;
         used_by_table = FLEX_COUNTER_POOL_USED_BY_EXT_FP_POLICY_TABLE;
         used_by_table_selection_criteria=
                 FLEX_COUNTER_POOL_USED_BY_EXT_FP_POLICY_TABLE;
         break;
    case SOURCE_VPm:
         default_pool_number=pool_number_l=
                 FLEX_COUNTER_DEFAULT_SOURCE_VP_TABLE_POOL_NUMBER;
         /* Time being keeping separate default pool for source vp and l3_iif */
         used_by_table = FLEX_COUNTER_POOL_USED_BY_SOURCE_VP_TABLE;
         /* But IfRequired SOURCE_VP & L3_IIF counters CanBeShared in same 
            pool!*/
         used_by_table_selection_criteria=
                 FLEX_COUNTER_POOL_USED_BY_L3_IIF_TABLE |
                 FLEX_COUNTER_POOL_USED_BY_SOURCE_VP_TABLE ;
         break;
    default:
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Invalid Table is passed %d \n"),
                    ingress_table));
         return BCM_E_INTERNAL;
    }

    BCM_IF_ERROR_RETURN(
        _bcm_esw_stat_flex_object_config_get(unit, 
            ingress_table, &object_config));

    if (object_config.share_criteria != INVALID_CONFIG) {
        used_by_table_selection_criteria |= object_config.share_criteria;
        default_pool_number=pool_number_l = object_config.config_pool;
    }
    if (object_config.exclude_criteria != INVALID_CONFIG) {       
        used_by_table_selection_criteria &= ~(object_config.exclude_criteria);
    }

    do {
       /* Either free or being used by port table only */
       if ((flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                          [pool_number_l].used_by_tables == 0) ||
           (flex_pool_stat[unit][bcmStatFlexDirectionIngress][pool_number_l].
            used_by_tables & used_by_table_selection_criteria)) {
           if (shr_aidxres_list_alloc_block(flex_aidxres_list_handle[unit]
                   [bcmStatFlexDirectionIngress][pool_number_l],
                   flex_ingress_modes
                   [unit][mode].total_counters,
                   &base_idx_l) == BCM_E_NONE) {
               flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                    [pool_number_l].used_by_tables |= used_by_table;
               SHR_BITSET(flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                    [pool_number_l].used_by_objects, object);
               flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                    [pool_number_l].used_entries += flex_ingress_modes
                                                    [unit][mode].total_counters;
               LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "Allocated  counter for Table:%s "
                                      "pool_number:%d mode:%d base_idx:%d ref_cnt:%d\n"),
                          SOC_MEM_UFNAME(unit, ingress_table),
                          pool_number_l,mode,base_idx_l,
                          flex_ingress_modes[unit][mode].reference_count));
               shr_aidxres_list_state(flex_aidxres_list_handle[unit]
                   [bcmStatFlexDirectionIngress][pool_number_l],
                   NULL,NULL,NULL,NULL,&free_count,&alloc_count,&largest_free,
                   NULL);
               LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                         (BSL_META_U(unit,
                                     "Current Pool status free_count:%d alloc_count:%d"
                                      "largest_free:%d used_by_tables:%d"
                                      "used_entries:%d\n"),
                          free_count,alloc_count,largest_free,
                          flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                          [pool_number_l].used_by_tables,
                          flex_pool_stat[unit][bcmStatFlexDirectionIngress]
                          [pool_number_l].used_entries));
               *pool_number    = pool_number_l;
               *base_idx       = base_idx_l;
               _bcm_esw_stat_get_counter_id(
                             flex_ingress_modes[unit][mode].group_mode,
                             object,mode,pool_number_l,base_idx_l,
                             &stat_counter_id);
               BCM_STAT_FLEX_COUNTER_LOCK(unit);
               flex_ingress_modes[unit][mode].reference_count++;
               BCM_STAT_FLEX_COUNTER_UNLOCK(unit);
               if (_bcm_esw_stat_flex_insert_stat_id(
                   local_scache_ptr[unit],stat_counter_id) != BCM_E_NONE) {
                   LOG_WARN(BSL_LS_BCM_FLEXCTR,
                            (BSL_META_U(unit,
                                        "WARMBOOT:Couldnot add entry in scache memory"
                                         ".Attach it\n")));
               }
               return BCM_E_NONE;
           }
       }
       pool_number_l = (pool_number_l+1) % num_flex_ingress_pools;
    }while(pool_number_l != default_pool_number);
    LOG_ERROR(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Pools exhausted for Table:%s\n"),
               SOC_MEM_UFNAME(unit,ingress_table)));
    return BCM_E_FAIL; /*or BCM_E_RESOURCE*/
}
/*
 * Function:
 *      _bcm_esw_stat_flex_set_group_mode
 * Description:
 *      Set Flex group mode  in s/w copy  for reference
 * Parameters:
 *      unit                  - (IN) unit number
 *      direction             - (IN) Flex Data Flow Direction
 *      offset_mode           - (IN) Flex Offset Mode
 *      group_mode            - (IN) Flex Group Mode
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_set_group_mode(
            int                       unit,
            bcm_stat_flex_direction_t direction,
            uint32                    offset_mode,
            bcm_stat_group_mode_t     group_mode)
{
    /* Better to check */
    if(!((group_mode >= bcmStatGroupModeSingle) &&
         (group_mode <= (bcm_stat_group_mode_t) bcmStatGroupModeFlex4))) {
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Invalid bcm_stat_group_mode_t passed %d \n"),
                    group_mode));
         return BCM_E_PARAM;
    }
    if (direction == bcmStatFlexDirectionIngress) {
        flex_ingress_modes[unit][offset_mode].group_mode = group_mode;
    } else {
        flex_egress_modes[unit][offset_mode].group_mode = group_mode;
    }
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_reset_group_mode
 * Description:
 *      ReSet Flex group mode  in s/w copy  for reference
 * Parameters:
 *      unit                  - (IN) unit number
 *      direction             - (IN) Flex Data Flow Direction
 *      offset_mode           - (IN) Flex Offset Mode
 *      group_mode            - (IN) Flex Group Mode
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_reset_group_mode(
            int                       unit,
            bcm_stat_flex_direction_t direction,
            uint32                    offset_mode,
            bcm_stat_group_mode_t     group_mode)
{
    /* Better to check */
    if(!((group_mode >= bcmStatGroupModeSingle) &&
         (group_mode <= (bcm_stat_group_mode_t) bcmStatGroupModeFlex4))) {
         LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                   (BSL_META_U(unit,
                               "Invalid bcm_stat_group_mode_t passed %d \n"),
                    group_mode));
         return BCM_E_PARAM;
    }
    if (direction == bcmStatFlexDirectionIngress) {
        flex_ingress_modes[unit][offset_mode].group_mode = 0;
    } else {
        flex_egress_modes[unit][offset_mode].group_mode = 0;
    }
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_show_mode_info
 * Description:
 *      Show Mode information
 * Parameters:
 *      unit                  - (IN) unit number
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      For Debugging purpose only
 */
void _bcm_esw_stat_flex_show_mode_info(int unit)
{
    uint32                    mode=0;
    LOG_CLI((BSL_META_U(unit,
                        "#####################  INGRESS  #########################\n")));
    LOG_CLI((BSL_META_U(unit,
                        "Mode:\t\tReference_Count\t\tTotal_Counters\t\tGroup_Mode \n")));
    for (mode=0;mode<BCM_STAT_FLEX_COUNTER_MAX_MODE;mode++) {
         if (flex_ingress_modes[unit][mode].available) {
             LOG_CLI((BSL_META_U(unit,
                                 "%u\t\t%u\t\t%u\t\t%s\n"), mode,
                      flex_ingress_modes[unit][mode].reference_count,
                      flex_ingress_modes[unit][mode].total_counters,
                      flex_group_modes[flex_ingress_modes[unit][mode].
                      group_mode]));
         } else {
             LOG_CLI((BSL_META_U(unit,
                                 "%u===UNCONFIGURED====\n"), mode));
         }
    }
    LOG_CLI((BSL_META_U(unit,
                        "#####################  EGRESS  ##########################\n")));
    LOG_CLI((BSL_META_U(unit,
                        "Mode:\t\tReference_Count\t\tTotal_Counters\t\tGroup_Mode \n")));
    for (mode=0;mode<BCM_STAT_FLEX_COUNTER_MAX_MODE;mode++) {
         if (flex_egress_modes[unit][mode].available) {
             LOG_CLI((BSL_META_U(unit,
                                 "%u\t\t%u\t\t%u\t\t%s\n"), mode,
                      flex_egress_modes[unit][mode].reference_count,
                      flex_egress_modes[unit][mode].total_counters,
                      flex_group_modes[flex_egress_modes[unit][mode].group_mode]));
         } else {
             LOG_CLI((BSL_META_U(unit,
                                 "%u===UNCONFIGURED====\n"), mode));
         }
    }
}
/*
 * Function:
 *      _bcm_esw_stat_flex_get_ingress_object
 * Description:
 *      Get Ingress Object  corresponding to given Ingress Flex Table
 * Parameters:
 *      ingress_table                  - (IN) Flex Ingress Table
 *      object                         - (OUT) Flex Ingress Object
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_get_ingress_object(
            int               unit,
            soc_mem_t         ingress_table,
            uint32            table_index,
            void              *ingress_entry,
            bcm_stat_object_t *object)
{
    soc_mem_info_t *memp;
    void           *entry=NULL;
    uint32         ingress_entry_data_size=0;
    uint32         key_type = 0;

    memp = &SOC_MEM_INFO(unit, ingress_table);
  
    if (memp->flags & SOC_MEM_FLAG_MULTIVIEW) {
        if (ingress_entry != NULL) {
            entry = ingress_entry;
        } else {
            ingress_entry_data_size = WORDS2BYTES(BYTES2WORDS(
                                      SOC_MEM_INFO(unit, ingress_table).bytes));
            entry = sal_alloc(ingress_entry_data_size,"ingress_table");
            if (entry == NULL) {
                return BCM_E_MEMORY;
            }
            if (soc_mem_read(unit, ingress_table, MEM_BLOCK_ANY,table_index,
                             entry) != SOC_E_NONE) {
                sal_free(entry);
                return BCM_E_INTERNAL;
            }
        }
        if (soc_mem_field_valid(unit, ingress_table, KEY_TYPEf)) {
            soc_mem_field_get(unit, ingress_table, entry, KEY_TYPEf, &key_type);
        } else if (soc_mem_field_valid(unit, ingress_table, KEY_TYPE_0f)) {
            soc_mem_field_get(unit, ingress_table, entry, 
                              KEY_TYPE_0f, &key_type);
        } else if (soc_mem_field_valid(unit, ingress_table,ENTRY_TYPEf)) {
            soc_mem_field_get(unit, ingress_table, entry, 
                              ENTRY_TYPEf, &key_type);
        }
        if (ingress_entry == NULL) {
            sal_free(entry);
        }
    }
    switch(ingress_table) {
    case PORT_TABm: 
         *object=bcmStatObjectIngPort;
         break;
    case VLAN_TABm: 
         *object=bcmStatObjectIngVlan;
         break;
    case VLAN_XLATEm: 
#if defined(BCM_TRIDENT2_SUPPORT)
         if (sal_strcmp(memp->views[key_type],"VXLAN_DIP")  == 0) {
             *object=bcmStatObjectIngVxlan;
         } else
#endif /* BCM_TRIDENT2_SUPPORT */
         if (sal_strcmp(memp->views[key_type],"L2GRE_DIP")  == 0) {
             *object = bcmStatObjectIngL2Gre;
         } else {
             *object=bcmStatObjectIngVlanXlate;
         }
         break;
#if defined(BCM_TRIUMPH3_SUPPORT)
    case VLAN_XLATE_EXTDm: 
         *object=bcmStatObjectIngVlanXlate;
         break;
#endif
    case VFIm: 
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
        if (_bcm_vfi_used_get(unit, table_index, _bcmVfiTypeL2Gre)) {
             *object = bcmStatObjectIngL2Gre;
        } else if (_bcm_vfi_used_get(unit, table_index, _bcmVfiTypeVxlan)) {
            *object = bcmStatObjectIngVxlan;
        } else
#endif
        {
            *object=bcmStatObjectIngVfi;
        }
        break;
    case L3_IIFm: 
         *object=bcmStatObjectIngL3Intf;
         break;
    case VRFm: 
         *object=bcmStatObjectIngVrf;
         break;
    case SOURCE_VPm: 
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
        if (_bcm_vp_used_get(unit, table_index,_bcmVpTypeL2Gre)) {
             *object = bcmStatObjectIngL2Gre;
        } else if (_bcm_vp_used_get(unit, table_index,_bcmVpTypeVxlan)) {
            *object=bcmStatObjectIngVxlan;
        } else if (_bcm_vp_used_get(unit, table_index,_bcmVpTypeNiv)) {
            *object=bcmStatObjectIngNiv;
        } else
#endif
        {
            *object=bcmStatObjectIngMplsVcLabel;
        }
         break;
    case MPLS_ENTRYm: 
         *object=bcmStatObjectIngMplsSwitchLabel;
         break;
#if defined(BCM_TRIUMPH3_SUPPORT)
    case MPLS_ENTRY_EXTDm: 
         if (sal_strcmp(memp->views[key_type],"TRILL") == 0) {
             *object=bcmStatObjectIngTrill;
         } else if (sal_strcmp(memp->views[key_type],"MIM_ISID") == 0) {
             *object=bcmStatObjectIngMimLookupId;
         } else {
             *object=bcmStatObjectIngMplsSwitchLabel;
         }
         break;
#endif
    case VFP_POLICY_TABLEm:
         *object=bcmStatObjectIngPolicy;
         break;
    case EXT_FP_POLICYm:
         *object=bcmStatObjectIngEXTPolicy;
         break;
    case L3_TUNNELm:
         *object=bcmStatObjectIngMplsFrrLabel;
         break;
    case L3_ENTRY_2m:
         *object=bcmStatObjectIngL3Host;
         break;
    case L3_ENTRY_4m:
         *object=bcmStatObjectIngL3Host;
         break;
    case EXT_IPV4_UCAST_WIDEm:
         *object=bcmStatObjectIngL3Host;
         break;
    case EXT_IPV6_128_UCAST_WIDEm:
         *object=bcmStatObjectIngL3Host;
         break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case ING_VSANm:
        if (SOC_IS_TD2_TT2(unit)) {
            *object = bcmStatObjectIngVsan;
        }
         break;
    case L3_ENTRY_IPV4_MULTICASTm:
        if (SOC_IS_TD2_TT2(unit)) {
            if (sal_strcmp(memp->views[key_type],"FCOE_EXT") == 0) {
                *object=bcmStatObjectIngFcoe;
            } else if (sal_strcmp(memp->views[key_type],"IPV4UC_EXT") == 0) {
                *object=bcmStatObjectIngL3Host;
            } else if (sal_strcmp(memp->views[key_type],"IPV4MC") == 0) {
                *object=bcmStatObjectIngIpmc;
            } 
        }
         break;
    case L3_ENTRY_IPV6_MULTICASTm:
        if (SOC_IS_TD2_TT2(unit)) {
            if (sal_strcmp(memp->views[key_type],"IPV6UC_EXT") == 0) {
                *object=bcmStatObjectIngL3Host;
            } else if (sal_strcmp(memp->views[key_type],"IPV6MC") == 0) {
                *object=bcmStatObjectIngIpmc;
            }
        }
         break;
    case L3_DEFIPm:
    case L3_DEFIP_PAIR_128m:
    case L3_DEFIP_ALPM_IPV4_1m:
    case L3_DEFIP_ALPM_IPV6_64_1m:
    case L3_DEFIP_ALPM_IPV6_128m:
    case L3_DEFIP_AUX_SCRATCHm:
        if (SOC_IS_TD2_TT2(unit)) {
            *object = bcmStatObjectIngL3Route;
        }
         break;
#endif /* BCM_TRIDENT2_SUPPORT */

    default:
         return BCM_E_INTERNAL;
    }
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_get_egress_object
 * Description:
 *      Get Egress Object  corresponding to given Egress Flex Table
 * Parameters:
 *      egress_table                   - (IN) Flex Egress Table
 *      object                         - (OUT) Flex Egress Object
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_get_egress_object(
            int               unit,
            soc_mem_t         egress_table,
            uint32            table_index,
            void              *egress_entry,
            bcm_stat_object_t *object)
{
    soc_mem_info_t *memp;
    void           *entry=NULL;
    uint32         egress_entry_data_size=0;
    uint32         key_type = 0;
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    uint32         dvp = 0;
#endif
    memp = &SOC_MEM_INFO(unit, egress_table);

    if (memp->flags & SOC_MEM_FLAG_MULTIVIEW) {
        if (egress_entry != NULL) {
            entry = egress_entry;
        } else {
            egress_entry_data_size = WORDS2BYTES(BYTES2WORDS(
                                      SOC_MEM_INFO(unit, egress_table).bytes));
            entry = sal_alloc(egress_entry_data_size,"egress_table");
            if (entry == NULL) {
                return BCM_E_MEMORY;
            }
            if (soc_mem_read(unit, egress_table, MEM_BLOCK_ANY,table_index,
                             entry) != SOC_E_NONE) {
                sal_free(entry);
                return BCM_E_INTERNAL;
            }
        }
        
        if (soc_mem_field_valid(unit, egress_table, KEY_TYPEf)) {
            soc_mem_field_get(unit, egress_table, entry, KEY_TYPEf, &key_type);
        } else if (soc_mem_field_valid(unit, egress_table, KEY_TYPE_0f)) {
            soc_mem_field_get(unit, egress_table, entry,
                              KEY_TYPE_0f, &key_type);
        } else if (soc_mem_field_valid(unit, egress_table,ENTRY_TYPEf)) {
            soc_mem_field_get(unit, egress_table, entry,
                              ENTRY_TYPEf, &key_type);
        }
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
        if ((egress_table == EGR_L3_NEXT_HOPm) &&
            (sal_strcmp(memp->views[key_type],"SD_TAG") == 0)) {
            soc_mem_field_get(unit, egress_table, entry,SD_TAG__DVPf, &dvp);
        }
#endif
        if (egress_entry == NULL) {
            sal_free(entry);
        }
    }

    switch(egress_table) {
    case EGR_PORTm:
         *object=bcmStatObjectEgrPort;
         break;
    case EGR_VLANm:
         *object=bcmStatObjectEgrVlan;
         break;
    case EGR_VLAN_XLATEm:
         if (sal_strcmp(memp->views[key_type],"MIM_ISID") == 0) {
             *object=bcmStatObjectEgrMimLookupId;
         } else {
             *object=bcmStatObjectEgrVlanXlate;
         }
         break;
    case EGR_VFIm:
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
        if (_bcm_vfi_used_get(unit, table_index, _bcmVfiTypeL2Gre)) {
            *object = bcmStatObjectIngL2Gre;
        } else if (_bcm_vfi_used_get(unit, table_index, _bcmVfiTypeVxlan)) {
            *object=bcmStatObjectEgrVxlan;
        } else
#endif
        {
            *object=bcmStatObjectEgrVfi;
        }
         break;
    case EGR_L3_NEXT_HOPm:
         if (sal_strcmp(memp->views[key_type],"WLAN") == 0) {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "WLAN view")));
             *object=bcmStatObjectEgrWlan;
         } else if (sal_strcmp(memp->views[key_type],"MIM") == 0) {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "MIM view")));
             *object=bcmStatObjectEgrMim;
        } 
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
        else if (sal_strcmp(memp->views[key_type],"SD_TAG") == 0) {              
            LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                      (BSL_META_U(unit,
                                  "SD_TAG view\n")));
            if (_bcm_vp_used_get(unit, dvp,_bcmVpTypeVxlan)) {
                *object=bcmStatObjectEgrVxlan;    
            } else if (_bcm_vp_used_get(unit, dvp,_bcmVpTypeNiv)) { 
                *object=bcmStatObjectEgrNiv; 
            } else { 
                *object=bcmStatObjectEgrL3Intf; 
            } 
         } 
#endif
		 else {
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Other view %s \n"),
                        memp->views[key_type]));
             *object=bcmStatObjectEgrL3Intf;
         }

         break;
    case EGR_DVP_ATTRIBUTE_1m:
        egress_table = EGR_DVP_ATTRIBUTEm;
        memp = &SOC_MEM_INFO(unit, egress_table);
        if (memp->flags & SOC_MEM_FLAG_MULTIVIEW) {
            egress_entry_data_size = WORDS2BYTES(BYTES2WORDS(
                                      SOC_MEM_INFO(unit, egress_table).bytes));
            entry = sal_alloc(egress_entry_data_size,"egress_table");
            if (entry == NULL) {
                return BCM_E_MEMORY;
            }
            if (soc_mem_read(unit, egress_table, MEM_BLOCK_ANY,table_index,
                             entry) != SOC_E_NONE) {
                sal_free(entry);
                return BCM_E_INTERNAL;
            }
                
            if (soc_mem_field_valid(unit, egress_table, VP_TYPEf)) {
                soc_mem_field_get(unit, egress_table, entry, VP_TYPEf, &key_type);
            }
            sal_free(entry);
        }
#if defined(BCM_TRIDENT2_SUPPORT)
         if (sal_strcmp(memp->views[key_type],"VXLAN") == 0) {
            *object = bcmStatObjectEgrVxlan;
         } else 
#endif /* BCM_TRIDENT2_SUPPORT */
         if (sal_strcmp(memp->views[key_type],"L2GRE") == 0) {
            *object = bcmStatObjectEgrL2Gre;
         }
         break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case  EGR_NAT_PACKET_EDIT_INFOm:
        if (SOC_IS_TD2_TT2(unit)) {
            *object = bcmStatObjectEgrL3Nat;
        }
        break;
#endif /* BCM_TRIDENT2_SUPPORT */

    default:
         return BCM_E_INTERNAL;
    }
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_get_counter_id
 * Description:
 *      Get Stat Counter Id for given accounting table and index
 * Parameters
 *      unit                  - (IN) unit number
 *      num_of_tables         - (IN) Number of Accounting Tables
 *      table_info            - (IN) Tables Info(Name,direction and index)
 *      num_stat_counter_ids  - (OUT) Number of Stat Counter Ids
 *      stat_counter_id       - (OUT) Stat Counter Ids
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_flex_get_counter_id(
            int                        unit,
            uint32                     num_of_tables,
            bcm_stat_flex_table_info_t *table_info,
            uint32                     *num_stat_counter_ids,
            uint32                     *stat_counter_id)
{
    int                        index=0;
    uint32                     offset_mode=0;
    uint32                     pool_number=0;
    uint32                     base_idx=0;
    void                       *entry_data=NULL;
    uint32                     entry_data_size=0;
    bcm_stat_object_t          object=bcmStatObjectIngPort;
    bcm_stat_group_mode_t      group= bcmStatGroupModeSingle;

    for (index=0;index < num_of_tables ;index++) {
         entry_data_size = WORDS2BYTES(BYTES2WORDS(
                           SOC_MEM_INFO(unit,table_info[index].table).bytes));
         entry_data = sal_alloc(entry_data_size,"vrf_table");
         sal_memset(entry_data,0,
                    SOC_MEM_INFO(unit, table_info[index].table).bytes);
         if (soc_mem_read(unit, table_info[index].table, MEM_BLOCK_ANY,
                       _bcm_esw_stat_flex_table_index_map(unit,
                                 table_info[index].table,
                                 table_info[index].index), entry_data) == SOC_E_NONE) {
             if (soc_mem_field_valid(unit,table_info[index].table,VALIDf)) {
                 if (soc_mem_field32_get(unit,table_info[index].table,
                                         entry_data,VALIDf)==0) {
                     sal_free(entry_data);
                     continue ;
                 }
             }
             _bcm_esw_get_flex_counter_fields_values(
                      unit, table_info[index].index,table_info[index].table , 
                      entry_data,&offset_mode, &pool_number, &base_idx);
             if ((offset_mode == 0) && (base_idx == 0)) {
                  sal_free(entry_data);
                  continue;
             }

             if (table_info[index].direction == bcmStatFlexDirectionIngress) {
                 if (_bcm_esw_stat_flex_get_ingress_object(
                     unit,table_info[index].table,table_info[index].index,
                     entry_data,&object) != BCM_E_NONE) {
                     sal_free(entry_data);
                     return BCM_E_NONE;
                 }
                 group=flex_ingress_modes[unit][offset_mode].group_mode;
             } else {
                 if (_bcm_esw_stat_flex_get_egress_object(
                     unit,table_info[index].table,table_info[index].index,
                     entry_data,&object) != BCM_E_NONE) {
                     sal_free(entry_data);
                     return BCM_E_NONE;
                 }
                 group=flex_egress_modes[unit][offset_mode].group_mode;
             }
             sal_free(entry_data);
             _bcm_esw_stat_get_counter_id(
                             group,object,
                             offset_mode,pool_number,base_idx,
                             &stat_counter_id[index]);
             (*num_stat_counter_ids)++;
         }
    }
    if ((*num_stat_counter_ids) == 0) {
         return BCM_E_NOT_FOUND;
    }
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_get_ingress_object
 * Description:
 *      Get Ingress Object available in give table
 * Parameters:
 *      unit                  - (IN) unit number
 *      pool_number           - (IN) Flex Pool number
 *      num_objects           - (OUT) Number of Flex Object found in given pool
 *      object                - (OUT) Flex Objects found in given pool
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      For debuging purpose only
 *
 */
void _bcm_esw_stat_get_ingress_object(
     int               unit,
     uint32            pool_number,
     uint32            *num_objects,
     bcm_stat_object_t *object)
{
     int index=0;
     uint32            ingress_table_masks[]={
                       FLEX_COUNTER_POOL_USED_BY_PORT_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_VFP_POLICY_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_VLAN_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_VFI_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_VRF_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_MPLS_ENTRY_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_VLAN_XLATE_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_L3_IIF_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_SOURCE_VP_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_L3_TUNNEL_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_L3_ENTRY_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_EXT_FP_POLICY_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_FCOE_TABLE
     }; 
     bcm_stat_object_t ingress_table_objects[]={
                       bcmStatObjectIngPort,
                       bcmStatObjectIngPolicy,
                       bcmStatObjectIngVlan,
                       bcmStatObjectIngVfi,
                       bcmStatObjectIngVrf,
                       bcmStatObjectIngMplsSwitchLabel,
                       bcmStatObjectIngVlanXlate,
                       bcmStatObjectIngL3Intf,
                       bcmStatObjectIngMplsVcLabel,
                       bcmStatObjectIngMplsFrrLabel,
                       bcmStatObjectIngL3Host,
                       bcmStatObjectIngEXTPolicy,
                       bcmStatObjectIngIpmc
     };
     *num_objects=0;
     for(index=0;
         index<sizeof(ingress_table_objects)/sizeof(ingress_table_objects[0]);
         index++) {
         if (flex_pool_stat[unit][bcmStatFlexDirectionIngress][pool_number].
                           used_by_tables & ingress_table_masks[index]) {
             object[*num_objects]=ingress_table_objects[index];
             (*num_objects)++;
         }
     }
}
/*
 * Function:
 *      _bcm_esw_stat_get_egress_object
 * Description:
 *      Get Egress Object available in give table
 * Parameters:
 *      unit                  - (IN) unit number
 *      pool_number           - (IN) Flex Pool number
 *      num_objects           - (OUT) Number of Flex Object found in given pool
 *      object                - (OUT) Flex Objects found in given pool
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      For debuging purpose only
 *
 */
void _bcm_esw_stat_get_egress_object(
     int               unit,
     uint32            pool_number,
     uint32            *num_objects,
     bcm_stat_object_t *object)
{
     int index=0;
     uint32            egress_table_masks[]={
                       FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_EGR_VFI_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_EGR_L3_NEXT_HOP_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_XLATE_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_EGR_PORT_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_EGR_DVP_ATTRIBUTE_1_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_EGR_VLAN_XLATE_TABLE,
                       FLEX_COUNTER_POOL_USED_BY_EGR_L3_NAT_TABLE
     };
     bcm_stat_object_t egress_table_objects[]={
                       bcmStatObjectEgrVlan,
                       bcmStatObjectEgrVfi,
                       bcmStatObjectEgrL3Intf,
                       bcmStatObjectEgrVlanXlate,
                       bcmStatObjectEgrPort,
                       bcmStatObjectEgrL2Gre,
                       bcmStatObjectEgrVxlan,
                       bcmStatObjectEgrL3Nat
     };
     *num_objects=0;
     for(index=0;
         index<sizeof(egress_table_objects)/sizeof(egress_table_objects[0]);
         index++) {
         if (flex_pool_stat[unit][bcmStatFlexDirectionEgress][pool_number].
                           used_by_tables & egress_table_masks[index]) {
             object[*num_objects]=egress_table_objects[index];
             (*num_objects)++;
         }
     }
}
/*
 * Function:
 *      _bcm_esw_stat_validate_group
 * Description:
 *      Validates passed stat group 
 * Parameters:
 *      unit                  - (IN) unit number
 *      group                 - (IN) Flex Group Mode
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_validate_group(
            int                   unit,
            bcm_stat_group_mode_t group)
{
    if(!((group >= bcmStatGroupModeSingle) &&
         (group <= (bcm_stat_group_mode_t) bcmStatGroupModeFlex4))) {
          LOG_CLI((BSL_META_U(unit,
                              "Invalid bcm_stat_group_mode_t passed %d \n"),group));
          return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_stat_validate_object
 * Description:
 *      Validates passed stat object and returns corresponding object's 
 *      direction 
 * Parameters:
 *      unit                  - (IN) unit number
 *      object                - (IN) Flex Accounting object
 *      direction             - (OUT) Flex Object's direction
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t _bcm_esw_stat_validate_object(
            int                       unit,
            bcm_stat_object_t         object, 
            bcm_stat_flex_direction_t *direction)
{
    if ((object >= bcmStatObjectIngPort) &&
        (object <= bcmStatObjectIngMplsSwitchLabel)) {
        *direction=bcmStatFlexDirectionIngress;
        return BCM_E_NONE;   
    }
#if defined(BCM_TRIUMPH3_SUPPORT)
    if ((SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) &&
        (object >= bcmStatObjectIngMplsFrrLabel) &&
        (object <= bcmStatObjectIngEXTPolicy)) {
        *direction=bcmStatFlexDirectionIngress;
        return BCM_E_NONE;   
    }
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit) &&
        (object >= bcmStatObjectIngVxlan) &&
        (object <= bcmStatObjectIngL3Route)) {
        *direction=bcmStatFlexDirectionIngress;
        return BCM_E_NONE;   
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    /* additional valid ingress objects check */ 
	if (object == bcmStatObjectIngNiv) { 
	    *direction=bcmStatFlexDirectionIngress; 
        return BCM_E_NONE;    
	} 

#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit) &&
        (object == bcmStatObjectIngIpmc)) {
        *direction=bcmStatFlexDirectionIngress;
        return BCM_E_NONE;   
    }
#endif /* BCM_TRIDENT2_SUPPORT */

#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit) &&
        (object == bcmStatObjectIngVxlanDip)) {
        *direction=bcmStatFlexDirectionIngress;
        return BCM_E_NONE;   
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    if ((object >= bcmStatObjectEgrPort) &&
        (object <= bcmStatObjectEgrL3Intf)) {
        *direction=bcmStatFlexDirectionEgress;
        return BCM_E_NONE;   
    }
#if defined(BCM_TRIUMPH3_SUPPORT)
    if ((SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) &&
        (object >= bcmStatObjectEgrWlan) &&
        (object <= bcmStatObjectEgrL2Gre)) {
        *direction=bcmStatFlexDirectionEgress;
        return BCM_E_NONE;   
    }
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit) &&
        (object >= bcmStatObjectEgrL2Gre) &&
        (object <= bcmStatObjectEgrL3Nat)) {
        *direction=bcmStatFlexDirectionEgress;
        return BCM_E_NONE;   
    }
#endif /* BCM_TRIDENT2_SUPPORT */
	
	/* additional valid egress objects check */ 
	if (object == bcmStatObjectEgrNiv) { 
	    *direction=bcmStatFlexDirectionEgress; 
	    return BCM_E_NONE;    
	} 

    LOG_CLI((BSL_META_U(unit,
                        "Invalid bcm_stat_object_t passed %d \n"),object));
    return BCM_E_PARAM;   
}

/*
 * Function:
 *	bcm_esw_stat_group_mode_id_create
 * Description:
 *	Create Customized Stat Group mode for given Counter Attributes
 * Parameters:
 *	
 *  unit             : (IN)  UNIT number      
 *  direction        : (IN)  Direction (Ingress/egress) 
 *  num_pools        : (IN)  Passing a 0, then actual_num_pools will return the number of pools that are available. 
 *                         return the number of pools that are available. 
 *                         Passing n will return up to n pools in the return array
 *  actual_num_pools : (OUT)  Returns actual no of pools available for a given direction
 *  flex_pool_stat   : (INOUT)  array that provides the pool info
 *
 * Returns:
 *	BCM_E_*
 *	
 */

int _bcm_esw_stat_flex_pool_info_multi_get(
    int unit,
    bcm_stat_flex_direction_t direction,
    uint32 num_pools,
    uint32 *actual_num_pools,
    bcm_stat_flex_pool_stat_info_t *flex_pool_stat_info)
{
    uint32  num_pools_arr[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32  size_pool[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32  poolIndx=0;
    uint32  lIndx=0;

    if((direction != bcmStatFlexDirectionIngress) && 
	   (direction != bcmStatFlexDirectionIngress) ) {
        return BCM_E_PARAM;
    }

    num_pools_arr[bcmStatFlexDirectionIngress] = SOC_INFO(unit).
                                                 num_flex_ingress_pools;
    num_pools_arr[bcmStatFlexDirectionEgress]  = SOC_INFO(unit).
                                                 num_flex_egress_pools;
    size_pool[bcmStatFlexDirectionIngress] = SOC_INFO(unit).
                                             size_flex_ingress_pool;
    size_pool[bcmStatFlexDirectionEgress]  = SOC_INFO(unit).
                                             size_flex_egress_pool;

    /*chk the value of num_pools*/
    if(num_pools == 0) {
    /*return the actual_num_of pools available for the given direction */
 	    *actual_num_pools = num_pools_arr[direction];
    }  else { /*fill the structure and send it back */
        if(num_pools > num_pools_arr[direction] ) {
	        return BCM_E_PARAM;
	    }
	    for(poolIndx = 0; poolIndx < num_pools; poolIndx++)
	    {							 
	        flex_pool_stat_info[poolIndx].pool_id       = poolIndx;
	        flex_pool_stat_info[poolIndx].total_entries = size_pool[direction];
	        flex_pool_stat_info[poolIndx].used_entries = flex_pool_stat[unit]
                                                         [direction][poolIndx].
                                                         used_entries;
	        flex_pool_stat_info[poolIndx].attached_entries=flex_pool_stat[unit]
                                                           [direction]
                                                           [poolIndx].
                                                           attached_entries;
	        flex_pool_stat_info[poolIndx].
                free_entries = flex_pool_stat_info[poolIndx].total_entries - 
                               flex_pool_stat_info[poolIndx].used_entries;
            for(lIndx = 0 ; lIndx < _SHR_BITDCLSIZE(bcmStatObjectMaxValue);
                lIndx++)
            {
                flex_pool_stat_info[poolIndx].
                    used_by_objects[lIndx]     =  flex_pool_stat[unit]
                                                  [direction][poolIndx].
                                                  used_by_objects[lIndx];
            }
	    }
    } /* end of if(num_pools == 0) */
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_stat_id_get_all
 * Description:
 *      Get all stat ids attached to the stat object.
 * Parameters:
 *      unit            - (IN) unit number
 *      object          - (IN) stat object
 *      stat_max        - (IN) max stat id count
 *      stat_array      - (OUT) array of stat ids
 *      stat_count      - (OUT) actual stat id count
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t 
_bcm_esw_stat_id_get_all(
    int unit, 
    bcm_stat_object_t object, 
    int stat_max, 
    uint32 *stat_array, 
    int *stat_count)
{
    uint32   index, array_index = 0;
    uint32   pool_number, base_index;
    uint32   num_objects = 0;
    uint32   stat_counter_id = 0;
    bcm_stat_flex_mode_t    offset_mode = 0;
    bcm_stat_object_t       object_l;
    bcm_stat_group_mode_t   group_l;
    bcm_stat_flex_direction_t direction;
    uint32                    num_pools[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32                    size_pool[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    bcm_stat_object_t         object_list[
                                 BCM_STAT_FLEX_COUNTER_MAX_EGRESS_TABLE +
                                 BCM_STAT_FLEX_COUNTER_MAX_INGRESS_TABLE];

    /* Parameter validation checks */
    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    if ((NULL == stat_count) || (stat_max < 0) ){
        return BCM_E_PARAM;
    }

    /* Validate object */
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_object(unit, object, &direction));

    /*
     * Loop through ingress and egress counter pools to get base/group index,
     * to retrieve all attached stat objects.
     */
    num_pools[bcmStatFlexDirectionIngress] = SOC_INFO(unit).
                                             num_flex_ingress_pools;
    num_pools[bcmStatFlexDirectionEgress]  = SOC_INFO(unit).
                                             num_flex_egress_pools;
    size_pool[bcmStatFlexDirectionIngress] = SOC_INFO(unit).
                                             size_flex_ingress_pool;
    size_pool[bcmStatFlexDirectionEgress]  = SOC_INFO(unit).
                                             size_flex_egress_pool;

    /* Loop all ingress and Egress Pools */
    for (direction = 0; direction < BCM_STAT_FLEX_COUNTER_MAX_DIRECTION;
         direction++ ) 
    {
        /* Loop all offset/Group Modes */
        for (offset_mode = 0; offset_mode < BCM_STAT_FLEX_COUNTER_MAX_MODE;
              offset_mode++) 
        {
            if (direction == bcmStatFlexDirectionIngress) {
                if (flex_ingress_modes[unit][offset_mode].available == 0) {
                      continue;
                }
                group_l=flex_ingress_modes[unit][offset_mode].group_mode;
            } else {
                if (flex_egress_modes[unit][offset_mode].available == 0 ) {
                      continue;
                }
                group_l=flex_egress_modes[unit][offset_mode].group_mode;
            }
            for (pool_number = 0; pool_number < num_pools[direction]; 
                 pool_number++) 
            {
                for (base_index = 0; base_index < size_pool[direction];
                     base_index++) 
                {
                    if (flex_base_index_reference_count[unit][direction]
                                        [pool_number][base_index] == 0) {
                        continue;
                    }
                    /* Get All Attached objects */
                    if(direction == bcmStatFlexDirectionIngress) {
                       _bcm_esw_stat_get_ingress_object(
                                unit, 
                                pool_number, 
                                &num_objects,
                                &object_list[0]);
                    } else { /* Egress direction */
                       _bcm_esw_stat_get_egress_object(
                                unit, 
                                pool_number, 
                                &num_objects,
                                &object_list[0]);
                    }
                    /*
                     * Loop through for the given object to get the
                     * attached stat counter ids 
                     */
                    for(index = 0; index < num_objects; index++) 
                    {
                        object_l = object_list[index];
                        if (object_l != object) {
                            continue;
                        }
                        _bcm_esw_stat_get_counter_id(
                                    group_l, object_l, offset_mode,
                                    pool_number, base_index,
                                    &stat_counter_id);
                        if ((stat_max > 0) && (NULL != stat_array)) {
                            /* Assign the attached stat_id for the object */
                            stat_array[array_index] = stat_counter_id; 
                            stat_max--;
                        }
                        array_index++;
                    }
                } /* Pool counter base */
            } /* Pools */
        }   /* Offset/Group modes */
    } /* Pool directions */
    *stat_count = array_index;
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_stat_group_dump_info
 * Description:
 *      Dump Useful Info about configured group
 * Parameters:
 *      unit                  - (IN) unit number
 *      all_flag              - (IN) If 1, object and group_mode are ignored
 *      object                - (IN) Flex Accounting object
 *      group                 - (IN) Flex Group Mode
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
void _bcm_esw_stat_group_dump_info(
     int                   unit,
     int                   all_flag,
     bcm_stat_object_t     object,
     bcm_stat_group_mode_t group)
{
    uint32                    num_objects=0;
    bcm_stat_object_t         ing_object_list[
                                 BCM_STAT_FLEX_COUNTER_MAX_INGRESS_TABLE];
    bcm_stat_object_t         egr_object_list[
                                 BCM_STAT_FLEX_COUNTER_MAX_EGRESS_TABLE];
    uint32                    index=0;
    uint32                    stat_counter_id=0;
    uint32                    pool_number=0;
    uint32                    base_index=0;
    bcm_stat_flex_mode_t      offset_mode=0;
    bcm_stat_object_t         object_l=bcmStatObjectIngPort;
    bcm_stat_group_mode_t     group_l= bcmStatGroupModeSingle;
    bcm_stat_flex_direction_t direction=bcmStatFlexDirectionIngress;
    uint32                    total_counters=0;
    uint32                    num_pools[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32                    size_pool[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    bcm_stat_flex_ing_attr_t  *ing_attr=NULL;
    bcm_stat_flex_egr_attr_t  *egr_attr=NULL;
    uint32                    attr_index=0;
    uint32                    total_attrs=0;
    uint8                     attr_value=0;

    if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
        LOG_CLI((BSL_META_U(unit,
                            "Not Available ...\n")));
        return;
    }                            

    LOG_CLI((BSL_META_U(unit,
                        "Not attached(MAX=%d) Stat counter Id info \n"),
             BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE));
    for (index=0;index<BCM_STAT_FLEX_COUNTER_MAX_SCACHE_SIZE;index++) {
         if (local_scache_ptr[unit][index] != 0) {
             stat_counter_id = local_scache_ptr[unit][index];
             _bcm_esw_stat_get_counter_id_info(
                           stat_counter_id,&group_l,&object_l,
                           &offset_mode,&pool_number,&base_index);
             if (_bcm_esw_stat_validate_object(unit,object_l,&direction) 
                 != BCM_E_NONE) {
                 LOG_CLI((BSL_META_U(unit,
                                     "\tInvalid object %d so skipping it \n"),object_l));
                 continue;
             }
             if (direction == bcmStatFlexDirectionIngress) {
                  total_counters = flex_ingress_modes[unit][offset_mode].
                                   total_counters;
             } else {
                  total_counters = flex_egress_modes[unit][offset_mode].
                                  total_counters;
             }
             if ((all_flag == TRUE) || 
                 ((object_l == object) &&
                  (group_l == group))) {
                   LOG_CLI((BSL_META_U(unit,
                                       "\tstat_counter_id = %d=0x%x \n"),
                            stat_counter_id,stat_counter_id));
                   LOG_CLI((BSL_META_U(unit,
                                       "\t\tDirection:%s mode:%d group_mode:%s"
                                       "\n\t\tpool:%d object:%s base:%d"
                                       " total_counters=%d\n"),
                            flex_directions[direction],offset_mode,
                            flex_group_modes[group_l],
                            pool_number,flex_objects[object_l],
                            base_index,total_counters));
             }
         }
    }
    LOG_CLI((BSL_META_U(unit,
                        "Atached Stat counter Id info \n")));
    num_pools[bcmStatFlexDirectionIngress] = SOC_INFO(unit).
                                             num_flex_ingress_pools;
    num_pools[bcmStatFlexDirectionEgress]  = SOC_INFO(unit).
                                             num_flex_egress_pools;
    size_pool[bcmStatFlexDirectionIngress] = SOC_INFO(unit).
                                             size_flex_ingress_pool;
    size_pool[bcmStatFlexDirectionEgress]  = SOC_INFO(unit).
                                             size_flex_egress_pool;

    for (direction=0;
         direction<BCM_STAT_FLEX_COUNTER_MAX_DIRECTION;
         direction++) {
         for (offset_mode=0;
              offset_mode<BCM_STAT_FLEX_COUNTER_MAX_MODE;
              offset_mode++) {
              LOG_CLI((BSL_META_U(unit,
                                  "=============================================\n")));
              if (direction == bcmStatFlexDirectionIngress) {
                  if (flex_ingress_modes[unit][offset_mode].available == 0) {
                      continue;
                  }
                  total_counters=flex_ingress_modes[unit][offset_mode].
                                                   total_counters;
                  group_l=flex_ingress_modes[unit][offset_mode].group_mode;
                  ing_attr = &flex_ingress_modes[unit][offset_mode].ing_attr;
                  switch(ing_attr->packet_attr_type) {
                  case bcmStatFlexPacketAttrTypeUncompressed:
                       LOG_CLI((BSL_META_U(unit,
                                           "IngressPacketAttributMode:Uncompressed\n")));
                       LOG_CLI((BSL_META_U(unit,
                                           "Attr_bits_selector:%x \n"),
                                ing_attr->uncmprsd_attr_selectors.
                                uncmprsd_attr_bits_selector));
                       total_attrs = sizeof(ing_uncmprsd_attr_bits_selector)/
                                     sizeof(ing_uncmprsd_attr_bits_selector[0]);
                       for(attr_index=0; attr_index<total_attrs; attr_index++) {
                           if(ing_attr->uncmprsd_attr_selectors.
                              uncmprsd_attr_bits_selector & 
                              ing_uncmprsd_attr_bits_selector[attr_index].
                              attr_bits)
                              LOG_CLI((BSL_META_U(unit,
                                                  "-->%s bit used\n"),
                                       ing_uncmprsd_attr_bits_selector[attr_index].
                                       attr_name));
                       }
                       break;
                  case bcmStatFlexPacketAttrTypeCompressed:
                       LOG_CLI((BSL_META_U(unit,
                                           "IngressPacket Attribut Mode:Compressed\n")));
                       total_attrs=sizeof(_bcm_esw_get_ing_func)/
                                   sizeof(_bcm_esw_get_ing_func[0]);
                       for(attr_index=0;attr_index<total_attrs;attr_index++) {
                          if((attr_value=_bcm_esw_get_ing_func[attr_index].func(
                             &(ing_attr->cmprsd_attr_selectors.
                               pkt_attr_bits)))){
                             LOG_CLI((BSL_META_U(unit,
                                                 "-->%s:%d\n"),
                                      _bcm_esw_get_ing_func[attr_index].func_desc,
                                      attr_value));
                          }
                       }
                       break;
                  default:
                       LOG_CLI((BSL_META_U(unit,
                                           "Not Implemented yet")));
                  }
              } else {
                  if (flex_egress_modes[unit][offset_mode].available == 0 ) {
                      continue;
                  }
                  egr_attr = &flex_egress_modes[unit][offset_mode].egr_attr;
                  switch(egr_attr->packet_attr_type) {
                  case bcmStatFlexPacketAttrTypeUncompressed:
                       LOG_CLI((BSL_META_U(unit,
                                           "EgressPacketAttributMode:Uncompressed\n")));
                       LOG_CLI((BSL_META_U(unit,
                                           "Attr_bits_selector:%x \n"),
                                egr_attr->uncmprsd_attr_selectors.
                                uncmprsd_attr_bits_selector));
                       total_attrs = sizeof(egr_uncmprsd_attr_bits_selector)/
                                     sizeof(egr_uncmprsd_attr_bits_selector[0]);
                       for(attr_index=0; attr_index<total_attrs; attr_index++) {
                           if(egr_attr->uncmprsd_attr_selectors.
                              uncmprsd_attr_bits_selector & 
                              egr_uncmprsd_attr_bits_selector[attr_index].
                              attr_bits)
                              LOG_CLI((BSL_META_U(unit,
                                                  "-->%s bit used\n"),
                                       egr_uncmprsd_attr_bits_selector[attr_index].
                                       attr_name));
                       }
                       break;
                  case bcmStatFlexPacketAttrTypeCompressed:
                       LOG_CLI((BSL_META_U(unit,
                                           "EgrressPacket Attribut Mode:Compressed\n")));
                       total_attrs=sizeof(_bcm_esw_get_egr_func)/
                                   sizeof(_bcm_esw_get_egr_func[0]);
                       for(attr_index=0;attr_index<total_attrs;attr_index++) {
                          if((attr_value=_bcm_esw_get_egr_func[attr_index].func(
                             &(egr_attr->cmprsd_attr_selectors.
                               pkt_attr_bits)))){
                              LOG_CLI((BSL_META_U(unit,
                                                  "-->%s:%d\n"),
                                       _bcm_esw_get_egr_func[attr_index].func_desc,
                                       attr_value));
                          }
                       }
                       break;
                  default:
                       LOG_CLI((BSL_META_U(unit,
                                           "Not Implemented yet")));
                  }
                  total_counters=flex_egress_modes[unit][offset_mode].
                                                  total_counters;
                  group_l=flex_egress_modes[unit][offset_mode].group_mode;
              }
              LOG_CLI((BSL_META_U(unit,
                                  "-->Direction:%s offset mode=%d\n" 
                                  "-->group_mode:%s total_counters=%d \n"),
                       flex_directions[direction], offset_mode, 
                       flex_group_modes[group_l], total_counters));
              for (pool_number=0;
                   pool_number<num_pools[direction];
                   pool_number++) {
                   for (base_index=0;
                        base_index<size_pool[direction];
                        base_index++) {
                        if (flex_base_index_reference_count[unit][direction]
                                            [pool_number][base_index] != 0) {
                            if(direction==bcmStatFlexDirectionIngress) {
                               _bcm_esw_stat_get_ingress_object(
                                        unit, 
                                        pool_number, 
                                        &num_objects,
                                        &ing_object_list[0]);
                              for(index=0;index<num_objects;index++) {
                                  object_l=ing_object_list[index];
                                  _bcm_esw_stat_get_counter_id(
                                                group_l,object_l,offset_mode,
                                                pool_number,base_index,
                                                &stat_counter_id);
                                  if (num_objects != 1) {
                                      LOG_CLI((BSL_META_U(unit,
                                                          "-->Probable..")));
                                  }
                                  if ((all_flag == TRUE) || 
                                      ((object_l == object) &&
                                       (group_l == group))) {
                                        LOG_CLI((BSL_META_U(unit,
                                                            "\tstat counter id %d (0x%x) "
                                                            "object=%s base index=%d (0x%x)\n"),
                                                 stat_counter_id,stat_counter_id,
                                                 flex_objects[object_l],
                                                 base_index,base_index)); 
                                  }
                              }
                        } else {
                            _bcm_esw_stat_get_egress_object(
                                     unit, pool_number, 
                                     &num_objects,&egr_object_list[0]);
                            for (index=0; index<num_objects; index++) {
                                 object_l = egr_object_list[index];
                                 _bcm_esw_stat_get_counter_id(
                                                group_l,object_l,offset_mode,
                                                pool_number,base_index,
                                                &stat_counter_id);
                                 if (num_objects != 1) {
                                     LOG_CLI((BSL_META_U(unit,
                                                         "Probable..")));
                                 }
                                 if ((all_flag == TRUE) || 
                                     ((object_l == object) &&
                                      (group_l == group))) {
                                       LOG_CLI((BSL_META_U(unit,
                                                           "\tstat counter id %d (0x%x) "
                                                           "object=%s base index=%d (0x%x)\n"),
                                                stat_counter_id,stat_counter_id,
                                                flex_objects[object_l],
                                                base_index,base_index)); 
                                 }
                            }
                        }
                   }
                }
              }
              LOG_CLI((BSL_META_U(unit,
                                  "=============================================\n")));
         }
    }
}
