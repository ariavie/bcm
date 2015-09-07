/*
 * $Id: flex_ctr.c 1.21 Broadcom SDK $
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
 * File:        flex_ctr.c
 * Purpose:     Manage flex counter group creation and deletion
 */

#include <bcm_int/esw/flex_ctr.h>
#include <bcm_int/control.h>
#include <bcm/debug.h>

/* ******************************************************************  */
/*              COMPOSITION OF STAT COUNTER ID                         */
/* ******************************************************************  */
/*              mode_id =  Max 3 bits (Total Eight modes)              */
/*              group_mode_id = Max 5 bits (Total 32)                  */
/*              pool_id = 4 (Max Pool:16)                              */
/*              a/c object_id=4 (Max Object:32)                        */
/*              15 bits for base index                                 */
/*              000    0-0000    0000   -00000     000-0000 0000-0000 */
/*              Mode3b Group5b   Pool4b -A/cObj4b  base-index          */
/* ******************************************************************  */
#define BCM_STAT_FLEX_MODE_START_BIT     29
#define BCM_STAT_FLEX_MODE_END_BIT       31
#define BCM_STAT_FLEX_GROUP_START_BIT    24
#define BCM_STAT_FLEX_GROUP_END_BIT      28
#define BCM_STAT_FLEX_POOL_START_BIT     20
#define BCM_STAT_FLEX_POOL_END_BIT       23
#define BCM_STAT_FLEX_OBJECT_START_BIT   15
#define BCM_STAT_FLEX_OBJECT_END_BIT     19
#define BCM_STAT_FLEX_BASE_IDX_START_BIT  0
#define BCM_STAT_FLEX_BASE_IDX_END_BIT   14

#define BCM_STAT_FLEX_MODE_MASK \
    ((1<<(BCM_STAT_FLEX_MODE_END_BIT - BCM_STAT_FLEX_MODE_START_BIT+1))-1)
#define BCM_STAT_FLEX_GROUP_MASK \
    ((1<<(BCM_STAT_FLEX_GROUP_END_BIT - BCM_STAT_FLEX_GROUP_START_BIT+1))-1)
#define BCM_STAT_FLEX_POOL_MASK \
    ((1<<(BCM_STAT_FLEX_POOL_END_BIT - BCM_STAT_FLEX_POOL_START_BIT+1))-1)
#define BCM_STAT_FLEX_OBJECT_MASK \
    ((1<<(BCM_STAT_FLEX_OBJECT_END_BIT - BCM_STAT_FLEX_OBJECT_START_BIT+1))-1)
#define BCM_STAT_FLEX_BASE_IDX_MASK \
    ((1<<(BCM_STAT_FLEX_BASE_IDX_END_BIT-BCM_STAT_FLEX_BASE_IDX_START_BIT+1))-1)

typedef struct _flex_pkt_res_data_s {
    uint32 pkt_res_field;
    uint32 counter_index; 
} _flex_pkt_res_data_t;


static uint32 _flex_pkt_res_values_katana[]={
_UNKNOWN_PKT_KATANA,
_CONTROL_PKT_KATANA,
_BPDU_PKT_KATANA,
_L2BC_PKT_KATANA,    
_L2UC_PKT_KATANA,
_L2DLF_PKT_KATANA,
_UNKNOWN_IPMC_PKT_KATANA,
_KNOWN_IPMC_PKT_KATANA, 
_KNOWN_L2MC_PKT_KATANA,
_UNKNOWN_L2MC_PKT_KATANA,
_KNOWN_L3UC_PKT_KATANA,
_UNKNOWN_L3UC_PKT_KATANA,
_KNOWN_MPLS_PKT_KATANA,
_KNOWN_MPLS_L3_PKT_KATANA,
_KNOWN_MPLS_L2_PKT_KATANA, 
_UNKNOWN_MPLS_PKT_KATANA,
_KNOWN_MIM_PKT_KATANA,
_UNKNOWN_MIM_PKT_KATANA,   
_KNOWN_MPLS_MULTICAST_PKT_KATANA    
};
static uint32 _flex_pkt_res_values_tr3[]={
_UNKNOWN_PKT_TR3,
_CONTROL_PKT_TR3,
_BPDU_PKT_TR3,
_L2BC_PKT_TR3,    
_L2UC_PKT_TR3,
_L2DLF_PKT_TR3,
_UNKNOWN_IPMC_PKT_TR3,
_KNOWN_IPMC_PKT_TR3, 
_KNOWN_L2MC_PKT_TR3,
_UNKNOWN_L2MC_PKT_TR3,
_KNOWN_L3UC_PKT_TR3,
_UNKNOWN_L3UC_PKT_TR3,
_KNOWN_MPLS_PKT_TR3,
_KNOWN_MPLS_L3_PKT_TR3,
_KNOWN_MPLS_L2_PKT_TR3, 
_UNKNOWN_MPLS_PKT_TR3,
_KNOWN_MIM_PKT_TR3,
_UNKNOWN_MIM_PKT_TR3,   
_KNOWN_MPLS_MULTICAST_PKT_TR3,
_OAM_PKT_TR3,
_BFD_PKT_TR3,
_ICNM_PKT_TR3,
_1588_PKT_TR3,
_KNOWN_TRILL_PKT_TR3,
_UNKNOWN_TRILL_PKT_TR3,
_KNOWN_NIV_PKT_TR3,
_UNKNOWN_NIV_PKT_TR3
};

static _flex_pkt_res_data_t ing_Single_res[]={ /* 19 */
                 {_UNKNOWN_PKT,0},
                 {_CONTROL_PKT,0},
                 {_BPDU_PKT,0},
                 {_L2BC_PKT,0},
                 {_L2UC_PKT,0},
                 {_L2DLF_PKT,0},
                 {_UNKNOWN_IPMC_PKT,0},
                 {_KNOWN_IPMC_PKT,0},
                 {_KNOWN_L2MC_PKT,0},
                 {_UNKNOWN_L2MC_PKT,0},
                 {_KNOWN_L3UC_PKT,0},
                 {_UNKNOWN_L3UC_PKT,0},
                 {_KNOWN_MPLS_PKT,0},
                 {_KNOWN_MPLS_L3_PKT,0},
                 {_KNOWN_MPLS_L2_PKT,0},
                 {_UNKNOWN_MPLS_PKT,0},
                 {_KNOWN_MIM_PKT,0},
                 {_UNKNOWN_MIM_PKT,0},
                 {_KNOWN_MPLS_MULTICAST_PKT,0}
#if defined(BCM_TRIUMPH3_SUPPORT)
                 ,
                 {_OAM_PKT,0},
                 {_BFD_PKT,0},
                 {_ICNM_PKT,0},
                 {_1588_PKT,0},
                 {_KNOWN_TRILL_PKT,0},
                 {_UNKNOWN_TRILL_PKT,0},
                 {_KNOWN_NIV_PKT,0},
                 {_UNKNOWN_NIV_PKT,0}
#endif
                 };
static _flex_pkt_res_data_t ing_TrafficType_res[]={ /* 6 */
                 {_L2BC_PKT,2},
                 {_L2UC_PKT,0},
                 {_KNOWN_L2MC_PKT,1},
                 {_UNKNOWN_L2MC_PKT,1},
                 {_KNOWN_L3UC_PKT,0},
                 {_UNKNOWN_L3UC_PKT,0}
                 };
static _flex_pkt_res_data_t ing_DlfAll_res[]={ /* 19 */
                 {_UNKNOWN_PKT,1},
                 {_CONTROL_PKT,1},
                 {_BPDU_PKT,1},
                 {_L2BC_PKT,1},
                 {_L2UC_PKT,1},
                 {_L2DLF_PKT,0},
                 {_UNKNOWN_IPMC_PKT,1},
                 {_KNOWN_IPMC_PKT,1},
                 {_KNOWN_L2MC_PKT,1},
                 {_UNKNOWN_L2MC_PKT,1},
                 {_KNOWN_L3UC_PKT,1},
                 {_UNKNOWN_L3UC_PKT,1},
                 {_KNOWN_MPLS_PKT,1},
                 {_KNOWN_MPLS_L3_PKT,1},
                 {_KNOWN_MPLS_L2_PKT,1},
                 {_UNKNOWN_MPLS_PKT,1},
                 {_KNOWN_MIM_PKT,1},
                 {_UNKNOWN_MIM_PKT,1},
                 {_KNOWN_MPLS_MULTICAST_PKT,1}
                 };
static _flex_pkt_res_data_t ing_Typed_res[]={ /* 6 */
                 {_L2BC_PKT,3},
                 {_L2UC_PKT,1},
                 {_KNOWN_L2MC_PKT,2},
                 {_UNKNOWN_L2MC_PKT,2},
                 {_KNOWN_L3UC_PKT,1},
                 {_UNKNOWN_L3UC_PKT,0}
                 };
static _flex_pkt_res_data_t ing_TypedAll_res[]={ /* 19 */
                 {_UNKNOWN_PKT,4},
                 {_CONTROL_PKT,4},
                 {_BPDU_PKT,4},
                 {_L2BC_PKT,3},
                 {_L2UC_PKT,1},
                 {_L2DLF_PKT,4},
                 {_UNKNOWN_IPMC_PKT,4},
                 {_KNOWN_IPMC_PKT,4},
                 {_KNOWN_L2MC_PKT,2},
                 {_UNKNOWN_L2MC_PKT,2},
                 {_KNOWN_L3UC_PKT,1},
                 {_UNKNOWN_L3UC_PKT,0},
                 {_KNOWN_MPLS_PKT,4},
                 {_KNOWN_MPLS_L3_PKT,4},
                 {_KNOWN_MPLS_L2_PKT,4},
                 {_UNKNOWN_MPLS_PKT,4},
                 {_KNOWN_MIM_PKT,4},
                 {_UNKNOWN_MIM_PKT,4},
                 {_KNOWN_MPLS_MULTICAST_PKT,4}
                 };
static _flex_pkt_res_data_t ing_SingleWithControl_res[]={ /* 19 */
                 {_UNKNOWN_PKT,0},
                 {_CONTROL_PKT,1},
                 {_BPDU_PKT,1},
                 {_L2BC_PKT,0},
                 {_L2UC_PKT,0},
                 {_L2DLF_PKT,0},
                 {_UNKNOWN_IPMC_PKT,0},
                 {_KNOWN_IPMC_PKT,0},
                 {_KNOWN_L2MC_PKT,0},
                 {_UNKNOWN_L2MC_PKT,0},
                 {_KNOWN_L3UC_PKT,0},
                 {_UNKNOWN_L3UC_PKT,0},
                 {_KNOWN_MPLS_PKT,0},
                 {_KNOWN_MPLS_L3_PKT,0},
                 {_KNOWN_MPLS_L2_PKT,0},
                 {_UNKNOWN_MPLS_PKT,0},
                 {_KNOWN_MIM_PKT,0},
                 {_UNKNOWN_MIM_PKT,0},
                 {_KNOWN_MPLS_MULTICAST_PKT,0}
                 };
static _flex_pkt_res_data_t ing_TrafficTypeWithControl_res[]={ /* 8 */
                 {_CONTROL_PKT,3},
                 {_BPDU_PKT,3},
                 {_L2BC_PKT,2},
                 {_L2UC_PKT,0},
                 {_KNOWN_L2MC_PKT,1},
                 {_UNKNOWN_L2MC_PKT,1},
                 {_KNOWN_L3UC_PKT,0},
                 {_UNKNOWN_L3UC_PKT,0}
                 };
static _flex_pkt_res_data_t ing_DlfAllWithControl_res[]={ /* 19 */
                 {_UNKNOWN_PKT,2},
                 {_CONTROL_PKT,0},
                 {_BPDU_PKT,0},
                 {_L2BC_PKT,2},
                 {_L2UC_PKT,2},
                 {_L2DLF_PKT,1},
                 {_UNKNOWN_IPMC_PKT,2},
                 {_KNOWN_IPMC_PKT,2},
                 {_KNOWN_L2MC_PKT,2},
                 {_UNKNOWN_L2MC_PKT,2},
                 {_KNOWN_L3UC_PKT,2},
                 {_UNKNOWN_L3UC_PKT,2},
                 {_KNOWN_MPLS_PKT,2},
                 {_KNOWN_MPLS_L3_PKT,2},
                 {_KNOWN_MPLS_L2_PKT,2},
                 {_UNKNOWN_MPLS_PKT,2},
                 {_KNOWN_MIM_PKT,2},
                 {_UNKNOWN_MIM_PKT,2},
                 {_KNOWN_MPLS_MULTICAST_PKT,2}
                 };
static _flex_pkt_res_data_t ing_TypedWithControl_res[]={ /* 8 */
                 {_CONTROL_PKT,0},
                 {_BPDU_PKT,0},
                 {_L2BC_PKT,4},
                 {_L2UC_PKT,2},
                 {_KNOWN_L2MC_PKT,3},
                 {_UNKNOWN_L2MC_PKT,3},
                 {_KNOWN_L3UC_PKT,2},
                 {_UNKNOWN_L3UC_PKT,1}
                 };
static _flex_pkt_res_data_t ing_TypedAllWithControl_res[]={ /* 19 */
                 {_UNKNOWN_PKT,5},
                 {_CONTROL_PKT,0},
                 {_BPDU_PKT,0},
                 {_L2BC_PKT,4},
                 {_L2UC_PKT,2},
                 {_L2DLF_PKT,5},
                 {_UNKNOWN_IPMC_PKT,5},
                 {_KNOWN_IPMC_PKT,5},
                 {_KNOWN_L2MC_PKT,3},
                 {_UNKNOWN_L2MC_PKT,3},
                 {_KNOWN_L3UC_PKT,2},
                 {_UNKNOWN_L3UC_PKT,1},
                 {_KNOWN_MPLS_PKT,5},
                 {_KNOWN_MPLS_L3_PKT,5},
                 {_KNOWN_MPLS_L2_PKT,5},
                 {_UNKNOWN_MPLS_PKT,5},
                 {_KNOWN_MIM_PKT,5},
                 {_UNKNOWN_MIM_PKT,5},
                 {_KNOWN_MPLS_MULTICAST_PKT,5}
                 };
static _flex_pkt_res_data_t egr_Single_res[]={ /* 2 */
                 {0,0}, /*Unicast */
                 {1,0}  /*Multicast */
                 };
static _flex_pkt_res_data_t egr_TrafficType_res[]={ /* 2 */
                 {0,0}, /*Unicast */
                 {1,1}  /*Multicast */
                 };
bcm_stat_flex_ing_pkt_attr_bits_t ing_pkt_attr_bits_g={0};
bcm_stat_flex_egr_pkt_attr_bits_t egr_pkt_attr_bits_g={0};

/*
 * Function:
 *      _bcm_esw_stat_flex_init_pkt_res_values
 * Description:
 *      Initialize Packet Resolution related (static) structures as
 *      per pkt_res_field indexes.
 *      
 * Parameters:
 *      unit               - (IN)     unit number
 *      flex_pkt_res_data  - (IN/OUT) Packet Resolution data Pointer
 *      num_entries        - (IN)     Entries in Packet Resolution data array
 *
 * Return Value:
 *      BCM_E_XXX
 *
 * Notes: Please note. Same field is used as index and gets initialized!
 *        TBD:Optimize this procedure using a function pointers array.
 *      
 */
static void _bcm_esw_stat_flex_init_pkt_res_values(
            int                  unit,
            _flex_pkt_res_data_t *flex_pkt_res_data,
            uint32               num_entries)
{
   uint32 index=0;
   uint32 *flex_pkt_res_values=NULL;
   uint32 flex_pkt_res_values_count=0;
  
   if (SOC_IS_KATANA(unit)) {
       flex_pkt_res_values = _flex_pkt_res_values_katana;
       flex_pkt_res_values_count = sizeof(_flex_pkt_res_values_katana)/
                                   sizeof(_flex_pkt_res_values_katana[0]);
   }
   if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit) || SOC_IS_KATANA2(unit)) {
       flex_pkt_res_values = _flex_pkt_res_values_tr3;
       flex_pkt_res_values_count = sizeof(_flex_pkt_res_values_tr3)/
                                   sizeof(_flex_pkt_res_values_tr3[0]);
   }
   if ( flex_pkt_res_values_count == 0) {
        FLEXCTR_WARN(("CONFIG ERROR: flex_pkt_res_values_count=0\n"));
        return ;
   }
  
   for(index=0;index<num_entries;index++) {
       /* Check Whether Index is exceeding chip specific count */
       if (flex_pkt_res_data[index].pkt_res_field >= 
           flex_pkt_res_values_count) {
           FLEXCTR_VVERB(("Flex Pkt Resolution Value Initialization failed"
                         "pkt_res_field=%d > flex_pkt_res_values_count=%d=0\n",
                         flex_pkt_res_data[index].pkt_res_field,
                         flex_pkt_res_values_count));
           /* Set it to invalid value */
           flex_pkt_res_data[index].pkt_res_field = 0xFFFFFFFF;
           continue ;
       }
       flex_pkt_res_data[index].pkt_res_field = 
            flex_pkt_res_values[flex_pkt_res_data[index].pkt_res_field];
   }
   return ;
}
 
/*
 * Function:
 *      _bcm_esw_stat_flex_create_mode
 * Description:
 *      Checks attributes direction and calls ingress/egress mode creation 
 *      function
 * Parameters:
 *      unit  - (IN) unit number
 *      attr  - (IN) Flex attributes
 *      mode  - (OUT) Flex mode
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      
 */
static bcm_error_t _bcm_esw_stat_flex_create_mode (
                   int                  unit,
                   bcm_stat_flex_attr_t *attr,
                   bcm_stat_flex_mode_t *mode)
{
    if (attr == NULL) {
        return BCM_E_PARAM;
    }
    if (attr->direction == bcmStatFlexDirectionIngress) {
        return _bcm_esw_stat_flex_create_ingress_mode(
                unit,
                &(attr->ing_attr),
                mode);
    }
    if (attr->direction == bcmStatFlexDirectionEgress) {
        return _bcm_esw_stat_flex_create_egress_mode(
                unit,
                &(attr->egr_attr),
                mode);
    }
    return BCM_E_PARAM;
}
/*
 * Function:
 *      _bcm_esw_fillup_ing_uncmp_attr
 * Description:
 *      Fill up ingress uncompressed flex attributes with required parameters
 *      Inialize offset table map also.
 * Parameters:
 *      ing_attr                     - (IN) Flex Ingress attributes
 *      uncmprsd_attr_bits_selector  - (IN) Uncompressed Bits Selector 
 *      total_counters               - (IN) Total Counters
 * Return Value:
 *      None
 * Notes:
 *      
 */
static void _bcm_esw_fillup_ing_uncmp_attr(
            bcm_stat_flex_ing_attr_t            *ing_attr,
            uint32                              uncmprsd_attr_bits_selector,
            uint8                               total_counters)
{
    uint32 index=0;

    ing_attr->packet_attr_type=bcmStatFlexPacketAttrTypeUncompressed;

    ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector = 
                uncmprsd_attr_bits_selector;
    ing_attr->uncmprsd_attr_selectors.total_counters = total_counters;
    /* Reset all Offset table fields */
    for (index=0;index<256;index++) {
         ing_attr->uncmprsd_attr_selectors.offset_table_map[index].offset=0;
         ing_attr->uncmprsd_attr_selectors.offset_table_map[index].
                  count_enable=0;
    }
}
/*
 * Function:
 *      _bcm_esw_fillup_ing_pkt_res_offset_table
 * Description:
        Fills up Ingress Offset table for Packet Resolution fields
 * Parameters:
 *      ing_attr     - (IN) Flex ingress attributes
 *      num_pairs    - (IN) Number of Packet Resolution Data pairs
 *      pkt_res_data - (IN) Packet Resolution Data Pointer
 * Return Value:
 *      None
 * Notes:
 *      
 */
static void _bcm_esw_fillup_ing_pkt_res_offset_table(
            bcm_stat_flex_ing_attr_t *ing_attr,
            uint32                    num_pairs,
            _flex_pkt_res_data_t     *pkt_res_data)
{
    uint32  count=0;
    int     pkt_res_field=0;
    int     counter_index=0;

    /* DROP:1bits(0th) SVP:1bits(1st) PKT_RES:6bits(2nd) bit position */

    for (count=0;count<num_pairs;count++) {
         /* Check for Invalid Value */
         if ((pkt_res_field=pkt_res_data[count].pkt_res_field) == 0xFFFFFFFF) {
             continue;
         }
         counter_index=pkt_res_data[count].counter_index;
         ing_attr->uncmprsd_attr_selectors.offset_table_map[pkt_res_field].
                                           offset= counter_index;
         ing_attr->uncmprsd_attr_selectors.offset_table_map[pkt_res_field].
                                           count_enable=1;
    }
}
/*
 * Function:
 *      _bcm_esw_fillup_egr_uncmp_attr
 * Description:
 *      Fill up egress uncompressed flex attributes with required parameters
 *      Inialize offset table map also.
 * Parameters:
 *      egr_attr                     - (IN) Flex Egress attributes
 *      uncmprsd_attr_bits_selector  - (IN) Uncompressed Bits Selector 
 *      total_counters               - (IN) Total Counters
 * Return Value:
 *      None
 * Notes:
 *      
 */
static void _bcm_esw_fillup_egr_uncmp_attr(
            bcm_stat_flex_egr_attr_t            *egr_attr,
            uint32                              uncmprsd_attr_bits_selector,
            uint8                               total_counters)
{
    uint32 index=0;

    egr_attr->packet_attr_type= bcmStatFlexPacketAttrTypeUncompressed;

    egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector = 
                uncmprsd_attr_bits_selector;
    egr_attr->uncmprsd_attr_selectors.total_counters = total_counters;
    /* Reset all Offset table fields */
    for (index=0;index<256;index++) {
         egr_attr->uncmprsd_attr_selectors.offset_table_map[index].offset=0;
         egr_attr->uncmprsd_attr_selectors.offset_table_map[index].
                   count_enable=0;
    }
}
/*
 * Function:
 *      _bcm_esw_fillup_egr_pkt_res_offset_table
 * Description:
        Fills up Egress Offset table for Packet Resolution fields
 * Parameters:
 *      egr_attr     - (IN) Flex egress attributes
 *      num_pairs    - (IN) Number of Packet Resolution Data pairs
 *      pkt_res_data - (IN) Packet Resolution Data Pointer
 * Return Value:
 *      None
 * Notes:
 *      
 */
static void _bcm_esw_fillup_egr_pkt_res_offset_table(
            bcm_stat_flex_egr_attr_t *egr_attr,
            uint32                    num_pairs,
            _flex_pkt_res_data_t     *pkt_res_data)
{
    uint32  count=0;
    int     pkt_res_field=0;
    int     counter_index=0;

    /* DROP:1bits(0th) SVP:1bits(1st) DVP:1bits(2nd) PKT_RES:1bit(3rd) bit */

    for (count=0;count<num_pairs;count++) {
         if ((pkt_res_field=pkt_res_data[count].pkt_res_field) == 0xFFFFFFFF) {
             continue;
         }
         counter_index=pkt_res_data[count].counter_index;
         egr_attr->uncmprsd_attr_selectors.offset_table_map[pkt_res_field].
                                           offset= counter_index;
         egr_attr->uncmprsd_attr_selectors.offset_table_map[pkt_res_field].
                                           count_enable=1;
    }
}
/*
 * Function:
 *      _bcm_esw_stat_get_counter_id
 * Description:
 *      Get Stat Counter Id based on offset mode,group mode,pool number,object
 *      and base index. 
 *      
 * Parameters:
 *      group             (IN)  Flex Group Mode
 *      object            (IN)  Flex Accounting Object
 *      mode              (IN)  Flex Offset Mode
 *      pool_number       (IN)  Allocated Pool Number for Flex Accounting Object
 *      base_idx          (IN)  Allocated Base Index for Flex Accounting Object
 *      stat_counter_id   (OUT) Stat Counter Id
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      
 */
void _bcm_esw_stat_get_counter_id(
     bcm_stat_group_mode_t group,
     bcm_stat_object_t     object,
     uint32                mode,
     uint32                pool_number,
     uint32                base_idx,
     uint32                *stat_counter_id)
{
     *stat_counter_id = ((mode & BCM_STAT_FLEX_MODE_MASK)            << 
                                 BCM_STAT_FLEX_MODE_START_BIT)       |
                        ((group & BCM_STAT_FLEX_GROUP_MASK)          << 
                                 BCM_STAT_FLEX_GROUP_START_BIT)      |
                        ((pool_number & BCM_STAT_FLEX_POOL_MASK)     <<
                                 BCM_STAT_FLEX_POOL_START_BIT)       |
                        ((object & BCM_STAT_FLEX_OBJECT_MASK)        << 
                                 BCM_STAT_FLEX_OBJECT_START_BIT)     |
                         (base_idx & BCM_STAT_FLEX_BASE_IDX_MASK);
}
/*
 * Function:
 *      _bcm_esw_stat_get_counter_id_info
 * Description:
 *      Get Stat Counter Id based on offset mode,group mode,pool number,object
 *      and base index. 
 *      
 * Parameters:
 *      stat_counter_id  (IN) Stat Counter Id
 *      group            (OUT)  Flex Group Mode
 *      mode             (OUT)  Flex Accounting Object
 *      offset           (OUT)  Flex Offset Mode
 *      pool_number      (OUT)  Allocated Pool Number for Flex Accounting Object
 *      base_idx         (OUT)  Allocated Base Index for Flex Accounting Object
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      
 */
void _bcm_esw_stat_get_counter_id_info(
     uint32                stat_counter_id,
     bcm_stat_group_mode_t *group,
     bcm_stat_object_t     *object,
     uint32                *mode,
     uint32                *pool_number,
     uint32                *base_idx)
{
     *mode        = (bcm_stat_flex_mode_t ) ((stat_counter_id >> 
                                              BCM_STAT_FLEX_MODE_START_BIT) &
                                             (BCM_STAT_FLEX_MODE_MASK));
     *group       = (bcm_stat_group_mode_t) ((stat_counter_id >> 
                                              BCM_STAT_FLEX_GROUP_START_BIT) &
                                             (BCM_STAT_FLEX_GROUP_MASK));
     *pool_number = ((stat_counter_id >> BCM_STAT_FLEX_POOL_START_BIT) &
                                       (BCM_STAT_FLEX_POOL_MASK));
     *object      = (bcm_stat_object_t) ((stat_counter_id >> 
                                          BCM_STAT_FLEX_OBJECT_START_BIT) &
                                         (BCM_STAT_FLEX_OBJECT_MASK));
     *base_idx    = (stat_counter_id & BCM_STAT_FLEX_BASE_IDX_MASK);
}
/*
 * Function:
 *      _bcm_esw_stat_group_create
 * Description:
 *      Reserve HW counter resources as per given group mode and acounting 
 *      object and make system ready for further stat collection action 
 *      
 * Parameters:
 *    Unit            (IN)  Unit number
 *    object          (IN)  Accounting Object
 *    Group_mode      (IN)  Group Mode
 *    Stat_counter_id (OUT) Stat Counter Id
 *    num_entries     (OUT) Number of Counter entries created 
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      
 */
bcm_error_t _bcm_esw_stat_group_create (
            int	                  unit,
            bcm_stat_object_t     object,
            bcm_stat_group_mode_t group_mode,
            uint32                *stat_counter_id,
            uint32                *num_entries)
{
    bcm_stat_flex_attr_t     *attr=NULL;
    bcm_stat_flex_mode_t     mode=0;
    bcm_error_t              rv=BCM_E_NONE;
    bcm_stat_flex_ing_attr_t *ing_attr=NULL;
    bcm_stat_flex_egr_attr_t *egr_attr=NULL;
    uint32                   map_index=0;
    uint32                   ignore_index=0;
    uint32                   counter_index=0;
    uint32                   outer_index=0;
    uint32                   inner_index=0;
    uint32                   base_index=0;
    uint32                   pool_number=0;
    uint32                   total_counters=0;
    uint32                   l2dlf_pkt=0;
    uint32                   unknown_l3uc_pkt=0;
    uint32                   unknown_l2mc_pkt=0;
    uint32                   known_l3uc_pkt=0;
    uint32                   known_l2mc_pkt=0;
    uint32                   l2uc_pkt=0;
    uint32                   l2bc_pkt=0;
    uint32                   control_pkt=0;
    uint32                   bpdu_pkt=0;
    uint32                   shift_by_bits=0;
    uint32                   shift_by_bits_for_value=0;

    bcm_stat_flex_ing_cmprsd_attr_selectors_t *ing_cmprsd_attr_selectors=NULL;
    bcm_stat_flex_ing_pkt_attr_bits_t         *ing_cmprsd_pkt_attr_bits=NULL;

    bcm_stat_flex_egr_cmprsd_attr_selectors_t *egr_cmprsd_attr_selectors=NULL;
    bcm_stat_flex_egr_pkt_attr_bits_t         *egr_cmprsd_pkt_attr_bits=NULL;
    bcm_stat_flex_direction_t                 direction;


    if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
         return BCM_E_UNAVAIL;
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_object(unit,object,&direction));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_group(unit,group_mode));

    /* Parameters look OK. ... */
  
    l2dlf_pkt        = _bcm_esw_stat_flex_get_pkt_res_value(unit,_L2DLF_PKT);
    unknown_l3uc_pkt = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                                                            _UNKNOWN_L3UC_PKT);
    unknown_l2mc_pkt =  _bcm_esw_stat_flex_get_pkt_res_value(unit,
                                                             _UNKNOWN_L2MC_PKT);
    known_l3uc_pkt   = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                                                            _KNOWN_L3UC_PKT);
    known_l2mc_pkt   = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                                                            _KNOWN_L2MC_PKT);
    l2uc_pkt         = _bcm_esw_stat_flex_get_pkt_res_value(unit,_L2UC_PKT);
    l2bc_pkt         = _bcm_esw_stat_flex_get_pkt_res_value(unit,_L2BC_PKT);
    control_pkt      = _bcm_esw_stat_flex_get_pkt_res_value(unit,_CONTROL_PKT);
    bpdu_pkt         = _bcm_esw_stat_flex_get_pkt_res_value(unit,_BPDU_PKT);

    /* 1. Allocating attribute Memory .... */

    attr = sal_alloc(sizeof(bcm_stat_flex_attr_t),"attr");
    if (attr == NULL) {
        FLEXCTR_ERR(("Failed to allocate memory for bcm_stat_flex_attr_t "));
        return BCM_E_MEMORY;
    }
    sal_memset(attr,0,sizeof(bcm_stat_flex_attr_t));

    /* 2. Deciding direction */
    if (direction == bcmStatFlexDirectionIngress) {
        /* INGRESS SIDE */
        attr->direction=bcmStatFlexDirectionIngress;
        ing_attr = &(attr->ing_attr);
        ing_cmprsd_attr_selectors=&(ing_attr->cmprsd_attr_selectors);
        ing_cmprsd_pkt_attr_bits= &(ing_attr->cmprsd_attr_selectors.
                                    pkt_attr_bits);
    } else {
        /* EGRESS SIDE */
        attr->direction=bcmStatFlexDirectionEgress;
        egr_attr = &(attr->egr_attr);
        egr_cmprsd_attr_selectors=&(egr_attr->cmprsd_attr_selectors);
        egr_cmprsd_pkt_attr_bits= &(egr_attr->cmprsd_attr_selectors.
                                    pkt_attr_bits);
    } 

    if (attr->direction == bcmStatFlexDirectionEgress) {
        switch(group_mode) {
        case bcmStatGroupModeDlfAll:
        case bcmStatGroupModeSingleWithControl:
             FLEXCTR_VVERB(("Overiding group_mode->bcmStatGroupModeSingle\n"));
             group_mode = bcmStatGroupModeSingle;
             break;
        case bcmStatGroupModeTyped:
        case bcmStatGroupModeTypedAll:
        case bcmStatGroupModeTrafficTypeWithControl:
        case bcmStatGroupModeDlfAllWithControl:
        case bcmStatGroupModeTypedWithControl:
        case bcmStatGroupModeTypedAllWithControl: 
             FLEXCTR_VVERB(("Overiding group_mode to "
                            "bcmStatGroupModeTrafficType \n"));
             group_mode = bcmStatGroupModeTrafficType;
             break;
        case bcmStatGroupModeDlfIntPri: 
        case bcmStatGroupModeDlfIntPriWithControl: 
        case bcmStatGroupModeTypedIntPriWithControl:
             FLEXCTR_VVERB(("Overiding group_mode to "
                            "bcmStatGroupModeTypedIntPri \n"));
             group_mode = bcmStatGroupModeTypedIntPri;
             break;
        default:
            break;
        }
    }
    /* ######################################################### */
    /* Ingress Packet Attributes(KATANA)  */
    /*
    CNG            IFP_CNG INT_PRI VlanFmt OuterDot1P InnerDot1P IngressPort   TOS       PacketRes SVPType DROP  IP
    38-37:2        36-35:2 34-31:4 30-29:2 28-26:3    25-23:3    22-17:6       16-9:8    8-3:6     2-2:1   1-1:1 0-0:1
    PRI_CNG_FN(8)                  PKT_PRI_FN(8)                 PORT_FN_FN(6) TOS_FN(8) PKT_RES_FN(8)           NOT_USED
     */
    /* ######################################################### */

    /* ######################################################### */
    /* Egress Packet Attributes(KATANA)  */
    /*
    CNG            INT_PRI         VlanFmt OuterDot1P InnerDot1P EgressPort    TOS       PacketRes SVPType DVPType DROP  IP
    32-31:2        30-27:4         26-25:2 24-22:3    21-19:3    18-13:6       12-5:8    4-4:1     3-3:1   2-2:1   1-1:1 0-0:1
    PRI_CNG_FN(6)                  PKT_PRI_FN(8)                 PORT_FN_FN(6) TOS_FN(8) PKT_RES_FN(4)                   NOT_USED
     */
    /* ######################################################### */


    /* 3. Filling up attributes */
    switch(group_mode) {
    case bcmStatGroupModeSingle:
         /* *********************************************/
         /* A single counter used for all traffic types */
         /* 1) UNKNOWN_PKT|CONTROL_PKT|BPDU_PKT|L2BC_PKT|L2UC_PKT|L2DLF_PKT| */
         /*    UNKNOWN_IPMC_PKT|KNOWN_IPMC_PKT|KNOWN_L2MC_PKT|               */
         /*    UNKNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT|KNOWN_L3UC_PKT|             */
         /*    UNKNOWN_L3UC_PKT|KNOWN_MPLS_PKT|KNOWN_MPLS_L3_PKT|            */
         /*    KNOWN_MPLS_L2_PKT|UNKNOWN_MPLS_PKT|KNOWN_MIM_PKT|             */
         /*    UNKNOWN_MIM_PKT|KNOWN_MPLS_MULTICAST_PKT                      */

         /* ******************************************* */
         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=1;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 1);
             _bcm_esw_fillup_ing_pkt_res_offset_table(
                 ing_attr,sizeof(ing_Single_res)/sizeof(ing_Single_res[0]),
                 &ing_Single_res[0]);
         } else {
             total_counters=1;
             _bcm_esw_fillup_egr_uncmp_attr(
                 egr_attr,
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 1);
             _bcm_esw_fillup_egr_pkt_res_offset_table(
                 egr_attr,sizeof(egr_Single_res)/sizeof(egr_Single_res[0]),
                 &egr_Single_res[0]);
         }
         break;
    case bcmStatGroupModeTrafficType:
         /* **************************************************************** */
         /* A dedicated counter per traffic type Unicast,multicast,broadcast */
         /* 1) L2UC_PKT | KNOWN_L3UC_PKT | UNKNOWN_L3UC_PKT                  */
         /* 2) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT|                              */
         /* 3) L2BC_PKT|                                                     */
         /* **************************************************************** */

         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=3;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 3);
             _bcm_esw_fillup_ing_pkt_res_offset_table(
                 ing_attr,
                 sizeof(ing_TrafficType_res)/sizeof(ing_TrafficType_res[0]),
                 &ing_TrafficType_res[0]);
         } else {
             total_counters=2;
             _bcm_esw_fillup_egr_uncmp_attr(
                 egr_attr,
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 2);
             _bcm_esw_fillup_egr_pkt_res_offset_table(
                 egr_attr,
                 sizeof(egr_TrafficType_res)/sizeof(egr_TrafficType_res[0]),
                 &egr_TrafficType_res[0]);
         }
         break;
    case bcmStatGroupModeDlfAll:
         /* ************************************************************* */
         /* A pair of counters where the base counter is used for dlf and */ 
         /* the other counter is used for all traffic types               */
         /* 1) L2DLF_PKT                                                  */
         /* 2) UNKNOWN_PKT | CONTROL_PKT|BPDU_PKT|L2BC_PKT|L2UC_PKT|      */
         /*    L2DLF_PKT|UNKNOWN_IPMC_PKT|KNOWN_IPMC_PKT|KNOWN_L2MC_PKT|  */
         /*    UNKNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT|KNOWN_L3UC_PKT|          */
         /*    UNKNOWN_L3UC_PKT|KNOWN_MPLS_PKT|KNOWN_MPLS_L3_PKT|         */
         /*    KNOWN_MPLS_L2_PKT|UNKNOWN_MPLS_PKT|KNOWN_MIM_PKT|          */
         /*    UNKNOWN_MIM_PKT|KNOWN_MPLS_MULTICAST_PKT                   */
         /* ************************************************************* */

         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=2;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 2);
             _bcm_esw_fillup_ing_pkt_res_offset_table(
                 ing_attr,sizeof(ing_DlfAll_res)/sizeof(ing_DlfAll_res[0]),
                 &ing_DlfAll_res[0]);
         } else {
             /* Group mode is overrided so  control shouldn't hit this part */
             FLEXCTR_ERR(("bcmStatGroupModeDlfAll is not supported"
                          "in egress side\n"));
             sal_free(attr);
             return BCM_E_INTERNAL;
         }
         break;
    case bcmStatGroupModeDlfIntPri:
         /* ************************************************************** */
         /* N+1 counters where the base counter is used for dlf and next N */
         /* are used per Cos                                               */
         /* 1) L2_DLF                                                      */
         /* 2..17) INT_PRI bits: 4bits                                     */
         /* ************************************************************** */

         if (attr->direction==bcmStatFlexDirectionEgress) {
             /* Must not hit */
             FLEXCTR_ERR(("bcmStatGroupModeDlfIntPri IsNotAvailable"
                            "in EgressSide\n"));
             sal_free(attr);
             return BCM_E_INTERNAL;
         }

         /* Although 17 counters but pkt_res(6) + int_pri(4)=10 bits
            so cannot use UNCOMPRESSED MODE */

         /* INT_PRI */ /* .... */ /* PacketRes */ 
         /* 34-31:4 */ /* .... */ /* 8-3:6     */
         /* PRI_CNG_FN=Cng(2x):IFP(2x):IntPri( all 4bits are used). 
            RightMost so no shifting required */
         /* PKT_RES_FN=Pkt(6bits  but 1 is used):SVP(1x):Drop(1x).
            LeftMost so 2 Left shifting required */

         total_counters=17;
         ing_attr->packet_attr_type=bcmStatFlexPacketAttrTypeCompressed;

         ing_cmprsd_pkt_attr_bits->pkt_resolution = 1;
         
         ing_cmprsd_pkt_attr_bits->pkt_resolution_pos = ing_pkt_attr_bits_g.
                                                        pkt_resolution_pos;
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->pkt_resolution_pos -= 2;
         }
         ing_cmprsd_pkt_attr_bits->pkt_resolution_mask = 1;

         ing_cmprsd_pkt_attr_bits->int_pri = ing_pkt_attr_bits_g.int_pri;
         ing_cmprsd_pkt_attr_bits->int_pri_pos = ing_pkt_attr_bits_g.
                                                 int_pri_pos;
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->int_pri_pos -= 2;
         }
         ing_cmprsd_pkt_attr_bits->int_pri_mask = ing_pkt_attr_bits_g.
                                                 int_pri_mask;

         ing_cmprsd_attr_selectors->total_counters = 17;

         /* set pkt_resolution map for  1 counters */
         /* Map[Encoded Packet value << 2=>[SVP-1bit + DROP-1bit]=
                CounterIndex << 2=>[SVP-1bit + DROP-1bit]=4(Start),8,12*/

         /* Reset pkt_resolution map */
         for (map_index=0; 
              map_index < sizeof(bcm_stat_flex_ing_cmprsd_pkt_res_attr_map_t) ;
              map_index++) {
              ing_cmprsd_attr_selectors->pkt_res_attr_map[map_index]=0;
         }
         shift_by_bits= ing_pkt_attr_bits_g.svp_type + ing_pkt_attr_bits_g.drop;
         shift_by_bits_for_value = shift_by_bits;
         if (SOC_IS_KATANA2(unit)) {
             shift_by_bits_for_value -= 2;
         }

         for (ignore_index=0; 
              ignore_index < (1<<shift_by_bits); ignore_index++) {
              ing_cmprsd_attr_selectors->
                  pkt_res_attr_map[(l2dlf_pkt<<shift_by_bits) | ignore_index]=
                  (1<<(shift_by_bits_for_value));
         }

         /* Reset pri_cng map */
         for (map_index=0; map_index < 256 ;map_index++) {
              ing_cmprsd_attr_selectors->pri_cnf_attr_map[map_index]=0;
         }
         /* set pri_cng map for  16 counters */
         for (ignore_index=0; ignore_index < (1<<4) ; ignore_index++) {
              for (map_index=0; map_index < 16 ;map_index++) {
                   ing_cmprsd_attr_selectors->
                   pri_cnf_attr_map[(ignore_index<<4) | map_index]=map_index;
              }
         }

         /* Reset all Offset table fields */
         for (counter_index=0;counter_index<256;counter_index++) {
              ing_cmprsd_attr_selectors->
                        offset_table_map[counter_index].offset=0;
              ing_cmprsd_attr_selectors->
                        offset_table_map[counter_index].count_enable=0;
         }

         /* Set DLF counter indexes(ODD 1,3,5) considering INT_PRI bits 
            don't care */
         for (counter_index=0;counter_index<16;counter_index++) {
              ing_cmprsd_attr_selectors->
                        offset_table_map[(counter_index<<1)|1].offset=0;
              ing_cmprsd_attr_selectors->
                        offset_table_map[(counter_index<<1)|1].count_enable=1;
         }

         /* Set Int pri counter indexes(Even 2,4,6) considering DLF=0 */
         for (counter_index=0;counter_index<16;counter_index++) {
              ing_cmprsd_attr_selectors->
                        offset_table_map[(counter_index<<1)].
                        offset=(counter_index+1);
              ing_cmprsd_attr_selectors->
                        offset_table_map[(counter_index<<1)].count_enable=1;
         }
         break;
    case bcmStatGroupModeTyped:
         /* ******************************************************* */
         /* A dedicated counter for unknown unicast, known unicast, */
         /* multicast, broadcast                                    */
         /* 1) UNKNOWN_L3UC_PKT                                     */
         /* 2) L2UC_PKT | KNOWN_L3UC_PKT                            */
         /* 3) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT                      */
         /* 4) L2BC_PKT                                             */
         /* ******************************************************* */
         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=4;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 4);
             _bcm_esw_fillup_ing_pkt_res_offset_table(
                 ing_attr,sizeof(ing_Typed_res)/sizeof(ing_Typed_res[0]),
                 &ing_Typed_res[0]);
         } else {
             /* Group mode is overrided so  control shouldn't hit this part */
             FLEXCTR_ERR(("bcmStatGroupModeTyped: is not supported"
                          " in egress side\n"));
             sal_free(attr);
             return BCM_E_INTERNAL;
         }
         break;
    case bcmStatGroupModeTypedAll:
         /* ******************************************************* */
         /* A dedicated counter for unknown unicast, known unicast, */
         /* multicast, broadcast and one for all traffic(not already*/
         /* counted)                                                */
         /* 1) UNKNOWN_L3UC_PKT                                     */
         /* 2) L2UC_PKT | KNOWN_L3UC_PKT                            */
         /* 3) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT                      */
         /* 4) L2BC_PKT                                             */
         /* 5) UNKNOWN_PKT|CONTROL_PKT|BPDU_PKT|L2DLF_PKT|          */
         /*    UNKNOWN_IPMC_PKT|KNOWN_IPMC_PKT|KNOWN_MPLS_PKT |     */
         /*    KNOWN_MPLS_L3_PKT|KNOWN_MPLS_L2_PKT|UNKNOWN_MPLS_PKT */
         /*    KNOWN_MIM_PKT|UNKNOWN_MIM_PKT|                       */
         /*    KNOWN_MPLS_MULTICAST_PKT                             */
         /* ******************************************************* */
         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=5;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 5);
             _bcm_esw_fillup_ing_pkt_res_offset_table(
                 ing_attr,sizeof(ing_TypedAll_res)/sizeof(ing_TypedAll_res[0]),
                 &ing_TypedAll_res[0]);
         } else {
             /* Group mode is overrided so  control shouldn't hit this part */
             FLEXCTR_ERR(("bcmStatGroupModeTypedAll is NotSupported"
                           " in EgressSide\n"));
             sal_free(attr);
             return BCM_E_INTERNAL;
         }
         break;
    case bcmStatGroupModeTypedIntPri:
         /* *************************************************************** */
         /* A dedicated counter for unknown unicast, known unicast,         */
         /* multicast,broadcast and N internal priority counters for traffic*/
         /* (not already counted)                                           */
         /* 1) UNKNOWN_L3UC_PKT                                             */
         /* 2) L2UC_PKT | KNOWN_L3UC_PKT                                    */ 
         /* 3) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT                              */
         /* 4) L2BC_PKT                                                     */
         /* 5..20) INT_PRI bits: 4bits                                      */
         /* *************************************************************** */
         if (attr->direction==bcmStatFlexDirectionIngress) {

             /* Although 20 counters but pkt_res(6) + int_pri(4)=10 bits
                so cannot use UNCOMPRESSED MODE */

             /* INT_PRI */ /* .... */ /* PacketRes */ 
             /* 34-31:4 */ /* .... */ /* 8-3:6     */
             /* PRI_CNG_FN=Cng(2x):IFP(2x):IntPri( all 4bits are used). 
                RightMost so no shifting required */
             /* PKT_RES_FN=Pkt(6bits  but 3 is used):SVP(1x):Drop(1x).
                LeftMost so 2 Left shifting required */

             total_counters=20;

             ing_attr->packet_attr_type=bcmStatFlexPacketAttrTypeCompressed;
             /* Cannot consider 0 value so taking 3 i.s.o. 2 */
             ing_cmprsd_pkt_attr_bits->pkt_resolution = 3;
             ing_cmprsd_pkt_attr_bits->pkt_resolution_pos = ing_pkt_attr_bits_g.
                                                            pkt_resolution_pos;
             if (SOC_IS_KATANA2(unit)) {
                 ing_cmprsd_pkt_attr_bits->pkt_resolution_pos -= 2;
             }
             ing_cmprsd_pkt_attr_bits->pkt_resolution_mask = (1<<3)-1;

             ing_cmprsd_pkt_attr_bits->int_pri = ing_pkt_attr_bits_g.int_pri;
             ing_cmprsd_pkt_attr_bits->int_pri_pos = ing_pkt_attr_bits_g.
                                                     int_pri_pos;
             if (SOC_IS_KATANA2(unit)) {
                 ing_cmprsd_pkt_attr_bits->int_pri_pos -= 2;
             }
             ing_cmprsd_pkt_attr_bits->int_pri_mask= ing_pkt_attr_bits_g.
                                                     int_pri_mask;                                                    

             ing_cmprsd_attr_selectors->total_counters = 20;
             /* Reset pkt_resolution map */
             for (map_index=0; map_index < 256 ;map_index++) {
                  ing_cmprsd_attr_selectors->pkt_res_attr_map[map_index]=0;
             }       
             /* set pkt_resolution map for  1 counters */
             /* Map[Encoded Packet value << 2=>[SVP-1bit + DROP-1bit]=
                CounterIndex << 2=>[SVP-1bit + DROP-1bit]=4(Start),8,12*/
             shift_by_bits= ing_pkt_attr_bits_g.svp_type + 
                            ing_pkt_attr_bits_g.drop;
             shift_by_bits_for_value = shift_by_bits;
             if (SOC_IS_KATANA2(unit)) {
                 shift_by_bits_for_value -= 2;
             }

             /* set pkt_resolution map for  1 counters.Ignore SVP,DROP bits */
             for (ignore_index=0; 
                  ignore_index < (1<<shift_by_bits) ; ignore_index++) {
                  ing_cmprsd_attr_selectors->pkt_res_attr_map[
                      (unknown_l3uc_pkt<<shift_by_bits)|ignore_index]=
                      (1<<shift_by_bits_for_value);
                  ing_cmprsd_attr_selectors->pkt_res_attr_map[
                      (l2uc_pkt<<shift_by_bits)|ignore_index]=
                      (2<<shift_by_bits_for_value);
                  ing_cmprsd_attr_selectors->pkt_res_attr_map[
                      (known_l3uc_pkt<<shift_by_bits)|ignore_index]=
                      (2<<shift_by_bits_for_value);
                  ing_cmprsd_attr_selectors->pkt_res_attr_map[
                      (known_l2mc_pkt<<shift_by_bits)|ignore_index]=
                      (3<<shift_by_bits_for_value);
                  ing_cmprsd_attr_selectors->pkt_res_attr_map[
                      (unknown_l2mc_pkt<<shift_by_bits)|ignore_index]=
                      (3<<shift_by_bits_for_value);
                  ing_cmprsd_attr_selectors->pkt_res_attr_map[
                      (l2bc_pkt<<shift_by_bits)|ignore_index]=
                      (4<<shift_by_bits_for_value);
             }
             /* Reset pri_cng map */
             for (map_index=0; map_index < 256 ;map_index++) {
                  ing_cmprsd_attr_selectors->pri_cnf_attr_map[map_index]=0;
             }       
             /* set pri_cng map for  16 counters */
             for (ignore_index=0; ignore_index < (1<<4) ; ignore_index++) {
                  for (map_index=0; map_index < 16 ;map_index++) {
                       ing_cmprsd_attr_selectors->
                        pri_cnf_attr_map[(ignore_index<<4)|map_index]=map_index;
                  }       
             }
             /* Reset all Offset table fields */
             for (counter_index=0;counter_index<256;counter_index++) {
                  ing_cmprsd_attr_selectors->
                           offset_table_map[counter_index].offset=0;
                  ing_cmprsd_attr_selectors->
                           offset_table_map[counter_index].count_enable=0;
             }
             /* ************************************************************* */
             /* Set unicast, known unicast, multicast, broadcast counter      */
             /* indexes considering INT_PRI as 0                              */
             /* ************************************************************* */
             ing_cmprsd_attr_selectors->offset_table_map[1].offset=0;
             ing_cmprsd_attr_selectors->offset_table_map[1].count_enable=1;
             ing_cmprsd_attr_selectors->offset_table_map[2].offset=1;
             ing_cmprsd_attr_selectors->offset_table_map[2].count_enable=1;
             ing_cmprsd_attr_selectors->offset_table_map[3].offset=2;
             ing_cmprsd_attr_selectors->offset_table_map[3].count_enable=1;
             ing_cmprsd_attr_selectors->offset_table_map[4].offset=3;
             ing_cmprsd_attr_selectors->offset_table_map[4].count_enable=1;


             /* Set Int pri counter indexes NotConsidering pkt resolution bits*/
             /* Priority 0 Counter not satisfying any condition */
             ing_cmprsd_attr_selectors->offset_table_map[0].offset=4;
             ing_cmprsd_attr_selectors->offset_table_map[0].count_enable=1;
             ing_cmprsd_attr_selectors->offset_table_map[5].offset=4;
             ing_cmprsd_attr_selectors->offset_table_map[5].count_enable=1;
             ing_cmprsd_attr_selectors->offset_table_map[6].offset=4;
             ing_cmprsd_attr_selectors->offset_table_map[6].count_enable=1;
             ing_cmprsd_attr_selectors->offset_table_map[7].offset=4;
             ing_cmprsd_attr_selectors->offset_table_map[7].count_enable=1;
             /*INT_PRI*/
             for (counter_index=1;counter_index<(1<<4);counter_index++) {
                  /*PktRes*/
                  for(ignore_index=0;ignore_index<(1<<3);ignore_index++) {
                      ing_cmprsd_attr_selectors->
                       offset_table_map[(counter_index<<3)|ignore_index].
                       offset=(counter_index+4);
                      ing_cmprsd_attr_selectors->
                       offset_table_map[(counter_index<<3)|ignore_index].
                       count_enable=1;
                  }
             }
         } else {
             /* ************************************************************* */
             /* A dedicated counter for unknown unicast, known unicast,       */
             /* multicast,broadcast and N internal priority counters for      */ 
             /* traffic (not already counted)                                 */
             /* 1) Unicast                                                    */
             /* 2) Multicast                                                  */
             /* 3..18) INT_PRI bits: 4bits                                    */
             /* ************************************************************* */
            total_counters=18;
            egr_attr->packet_attr_type=bcmStatFlexPacketAttrTypeCompressed;

            egr_cmprsd_pkt_attr_bits->pkt_resolution = egr_pkt_attr_bits_g.
                                                       pkt_resolution;
            egr_cmprsd_pkt_attr_bits->pkt_resolution_pos = egr_pkt_attr_bits_g.
                                                           pkt_resolution_pos;
            if (SOC_IS_KATANA2(unit)) {
                 egr_cmprsd_pkt_attr_bits->pkt_resolution_pos -= 4;
            }
            egr_cmprsd_pkt_attr_bits->pkt_resolution_mask = egr_pkt_attr_bits_g.
                                                            pkt_resolution_mask;

            egr_cmprsd_pkt_attr_bits->int_pri = egr_pkt_attr_bits_g.int_pri;
            egr_cmprsd_pkt_attr_bits->int_pri_pos = egr_pkt_attr_bits_g.
                                                    int_pri_pos;
            if (SOC_IS_KATANA2(unit)) {
                 egr_cmprsd_pkt_attr_bits->int_pri_pos -= 4;
            }
            egr_cmprsd_pkt_attr_bits->int_pri_mask=egr_pkt_attr_bits_g.int_pri_mask;

            egr_cmprsd_attr_selectors->total_counters = 18;

            shift_by_bits= egr_pkt_attr_bits_g.svp_type + 
                           egr_pkt_attr_bits_g.dvp_type + 
                           egr_pkt_attr_bits_g.drop;
             shift_by_bits_for_value = shift_by_bits;
             if (SOC_IS_KATANA2(unit)) {
                 shift_by_bits_for_value -= 4;
             }
            /* Set pkt_resolution map */
            /* Unicast */
            for (map_index=0; map_index < 1<< shift_by_bits ;map_index++) {
                 egr_cmprsd_attr_selectors->pkt_res_attr_map[map_index]=0;
            }       
            /* Multicast */
            for (; map_index < 255 ;map_index++) {
                 egr_cmprsd_attr_selectors->
                       pkt_res_attr_map[map_index]=(1<<shift_by_bits_for_value);
            }       

            /* Reset pri_cng map */
            for (map_index=0; map_index < 64 ;map_index++) {
                 egr_cmprsd_attr_selectors->pri_cnf_attr_map[map_index]=0;
            }        
            /* set pri_cng map for  16 counters */
            for (ignore_index=0; ignore_index < (1<<2) ; ignore_index++) {
                 for (map_index=0; map_index < 16 ;map_index++) {
                      egr_cmprsd_attr_selectors->
                        pri_cnf_attr_map[(ignore_index<<4)|map_index]=map_index;
                 }       
            }
            /* Reset all Offset table fields */
            for(counter_index=0;counter_index<256;counter_index++) {
                egr_cmprsd_attr_selectors->
                          offset_table_map[counter_index].offset=0;
                egr_cmprsd_attr_selectors->
                          offset_table_map[counter_index].count_enable=0;
            }
            /* ************************************************************** */
            /* Set unicast, multicast, counter indexes                        */
            /* considering INT_PRI bits zero                                  */
            /* ************************************************************** */
            egr_cmprsd_attr_selectors->
                      offset_table_map[0].offset=0;
            egr_cmprsd_attr_selectors->
                      offset_table_map[0].count_enable=1;
            egr_cmprsd_attr_selectors->
                      offset_table_map[1].offset=1;
            egr_cmprsd_attr_selectors->
                      offset_table_map[1].count_enable=1;
            /* PRI-0 counters will be addition of unicast & multicast packets!*/
            /* Set Int pri counter indexes ignoring pkt_res bits           */
            /*INT_PRI*/
            for (counter_index=1;counter_index<(1<<4);counter_index++) {
                 /*PktRes*/
                 for (ignore_index=0;ignore_index<(1<<1);ignore_index++) {
                      egr_cmprsd_attr_selectors->
                       offset_table_map[(counter_index<<1)|ignore_index].
                       offset=(counter_index+2);
                      egr_cmprsd_attr_selectors->
                       offset_table_map[(counter_index<<1)|ignore_index].
                       count_enable=1;
                 }
            }
         }
         break;
    case bcmStatGroupModeSingleWithControl:
         /* **************************************************************   */
         /* A single counter used for all traffic types with an additional   */ 
         /* counter for control traffic                                      */
         /* 1) UNKNOWN_PKT|                    |L2BC_PKT|L2UC_PKT|L2DLF_PKT| */
         /*    UNKNOWN_IPMC_PKT|KNOWN_IPMC_PKT|KNOWN_L2MC_PKT|               */
         /*    UNKNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT|KNOWN_L3UC_PKT|             */
         /*    UNKNOWN_L3UC_PKT|KNOWN_MPLS_PKT|KNOWN_MPLS_L3_PKT|            */
         /*    KNOWN_MPLS_L2_PKT|UNKNOWN_MPLS_PKT|KNOWN_MIM_PKT|             */
         /*    UNKNOWN_MIM_PKT|KNOWN_MPLS_MULTICAST_PKT                      */
         /* 2) CONTROL_PKT|BPDU_PKT                                          */
         /* **************************************************************   */
         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=2;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 2);
             _bcm_esw_fillup_ing_pkt_res_offset_table(
                 ing_attr,
                 sizeof(ing_SingleWithControl_res)/
                 sizeof(ing_SingleWithControl_res[0]),
                 &ing_SingleWithControl_res[0]);
         } else {
             /* Group mode is overrided so  control shouldn't hit this part */
             FLEXCTR_ERR(("bcmStatGroupModeSingleWithControl"
                          " is not supported in egress side\n"));
             sal_free(attr);
             return BCM_E_INTERNAL;
         }
         break;
    case bcmStatGroupModeTrafficTypeWithControl:
         /* ********************************************************  */
         /* A dedicated counter per traffic type unicast, multicast,  */
         /* broadcast with an additional counter for control traffic  */
         /* 1) L2UC_PKT | KNOWN_L3UC_PKT | UNKNOWN_L3UC_PKT           */
         /* 2) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT|                       */
         /* 3) L2BC_PKT|                                              */
         /* 4) CONTROL_PKT|BPDU_PKT                                   */
         /* ********************************************************  */
         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=4;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 4);
             _bcm_esw_fillup_ing_pkt_res_offset_table(
                 ing_attr,
                 sizeof(ing_TrafficTypeWithControl_res)/
                 sizeof(ing_TrafficTypeWithControl_res[0]),
                 &ing_TrafficTypeWithControl_res[0]);
         } else {
             /* Group mode is overrided so  control shouldn't hit this part */
             FLEXCTR_ERR(("bcmStatGroupModeTrafficTypeWithControl"
                          "is not supported in egress side\n"));
             sal_free(attr);
             return BCM_E_INTERNAL;
         }
         break;
    case bcmStatGroupModeDlfAllWithControl:
         /* ************************************************************** */
         /* A pair of counters where the base counter is used for control, */
         /* the next one for dlf and the other counter is used for all     */
         /* traffic types                                                  */
         /* 1) CONTROL_PKT|BPDU_PKT                                        */
         /* 2) L2DLF_PKT                                                   */
         /* 3)UNKNOWN_PKT | CONTROL_PKT|BPDU_PKT|L2BC_PKT|L2UC_PKT|        */
         /*   L2DLF_PKT|UNKNOWN_IPMC_PKT|KNOWN_IPMC_PKT|KNOWN_L2MC_PKT|    */
         /*   UNKNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT|KNOWN_L3UC_PKT|            */
         /*   UNKNOWN_L3UC_PKT|KNOWN_MPLS_PKT|KNOWN_MPLS_L3_PKT|           */
         /*   KNOWN_MPLS_L2_PKT|UNKNOWN_MPLS_PKT|KNOWN_MIM_PKT|            */
         /*   UNKNOWN_MIM_PKT|KNOWN_MPLS_MULTICAST_PKT                     */
         /* ************************************************************** */
         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=3;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 3);
             _bcm_esw_fillup_ing_pkt_res_offset_table(
                 ing_attr,
                 sizeof(ing_DlfAllWithControl_res)/
                 sizeof(ing_DlfAllWithControl_res[0]),
                 &ing_DlfAllWithControl_res[0]);
         } else {
             /* Group mode is overrided so  control shouldn't hit this part */
             FLEXCTR_ERR(("bcmStatGroupModeDlfAllWithControl is not supported "
                          "in egress side\n"));
             sal_free(attr);
             return BCM_E_INTERNAL;
         }
         break;
    case bcmStatGroupModeDlfIntPriWithControl:
         /* ************************************************************* */
         /* N+2 counters where the base counter is used for control, the  */
         /* next one for dlf and next N are used per Cos                  */
         /* 1) CONTROL_PKT|BPDU_PKT                                       */
         /* 2) L2_DLF                                                     */
         /* 3..18) INT_PRI bits: 4bits                                    */
         /* ************************************************************* */
         if (attr->direction==bcmStatFlexDirectionEgress) {
             FLEXCTR_ERR(("GroupModeDlfIntPriWithControl is not available in "
                          "egress side\n"));
             sal_free(attr);
             return BCM_E_INTERNAL;
         }
         /* Although 18 counters but pkt_res(6) + int_pri(4)=10 bits
            so cannot use UNCOMPRESSED MODE */

         /* INT_PRI */ /* .... */ /* PacketRes */ 
         /* 34-31:4 */ /* .... */ /* 8-3:6     */
         /* PRI_CNG_FN=Cng(2x):IFP(2x):IntPri( all 4bits are used). 
             RightMost so no shifting required */
         /* PKT_RES_FN=Pkt(6bits  but 2 is used):SVP(1x):Drop(1x).
            LeftMost so 2 Left shifting required */

         total_counters=18;
         ing_attr->packet_attr_type=bcmStatFlexPacketAttrTypeCompressed;

         ing_cmprsd_pkt_attr_bits->pkt_resolution = 2;
         ing_cmprsd_pkt_attr_bits->pkt_resolution_pos = ing_pkt_attr_bits_g.
                                                        pkt_resolution_pos;
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->pkt_resolution_pos -= 2;
         }
         ing_cmprsd_pkt_attr_bits->pkt_resolution_mask = (1<<2)-1;

         ing_cmprsd_pkt_attr_bits->int_pri = ing_pkt_attr_bits_g.int_pri;
         ing_cmprsd_pkt_attr_bits->int_pri_pos = ing_pkt_attr_bits_g.
                                                 int_pri_pos;
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->int_pri_pos -= 2;
         }
         ing_cmprsd_pkt_attr_bits->int_pri_mask = ing_pkt_attr_bits_g.
                                                  int_pri_mask;
         ing_cmprsd_attr_selectors->total_counters = 18;

         /* Reset pkt_resolution map */
         for (map_index=0; map_index < 256 ;map_index++) {
              ing_cmprsd_attr_selectors->pkt_res_attr_map[map_index]=0;
         }
         /* set pkt_resolution map for  1 counters */
         /* Map[Encoded Packet value << 2=>[SVP-1bit + DROP-1bit]=
            CounterIndex << 2=>[SVP-1bit + DROP-1bit]=4(Start),8,12*/

         shift_by_bits= ing_pkt_attr_bits_g.svp_type + ing_pkt_attr_bits_g.drop;
         shift_by_bits_for_value = shift_by_bits;
         if (SOC_IS_KATANA2(unit)) {
             shift_by_bits_for_value -= 2;
         }

         for (ignore_index=0; 
              ignore_index < (1<<shift_by_bits) ; ignore_index++) {
              ing_cmprsd_attr_selectors->
                   pkt_res_attr_map[
                   (control_pkt<<shift_by_bits) | ignore_index]=
                   (1<<shift_by_bits_for_value);
              ing_cmprsd_attr_selectors->
                   pkt_res_attr_map[(bpdu_pkt<<shift_by_bits) | ignore_index]=
                   (1<<shift_by_bits_for_value);
              ing_cmprsd_attr_selectors->
                   pkt_res_attr_map[
                   (l2dlf_pkt<<shift_by_bits) | ignore_index]=
                   (2<<shift_by_bits_for_value);
         }

         /* Reset pri_cng map */
         for (map_index=0; map_index < 256 ;map_index++) {
              ing_cmprsd_attr_selectors->pri_cnf_attr_map[map_index]=0;
         }
         /* set pri_cng map for  16 counters */
         for (ignore_index=0; ignore_index < (1<<4) ; ignore_index++) {
              for (map_index=0; map_index < 16 ;map_index++) {
                   ing_cmprsd_attr_selectors->
                     pri_cnf_attr_map[(ignore_index<<4) | map_index]=map_index;
              }
         }

         /* Reset all Offset table fields */
         for(counter_index=0;counter_index<256;counter_index++) {
                 ing_cmprsd_attr_selectors->
                           offset_table_map[counter_index].offset=0;
                 ing_cmprsd_attr_selectors->
                           offset_table_map[counter_index].count_enable=0;
         }
         /* ************************************************************ */
         /* Set CONTROL_PKT|BPDU_PKT , L2DLF counter indexes considering */
         /* INT_PRI bits 0                                               */
         /* ************************************************************ */

         ing_cmprsd_attr_selectors->offset_table_map[1].offset=0;
         ing_cmprsd_attr_selectors->offset_table_map[1].count_enable=1;
         ing_cmprsd_attr_selectors->offset_table_map[2].offset=1;
         ing_cmprsd_attr_selectors->offset_table_map[2].count_enable=1;

         /* Set IntPri counter indexes not-considering pkt resolution bits*/
         /* Priority 0 Counter not satisfying any condition  */
         ing_cmprsd_attr_selectors->offset_table_map[0].offset=2;
         ing_cmprsd_attr_selectors->offset_table_map[0].count_enable=1;
         ing_cmprsd_attr_selectors->offset_table_map[3].offset=2;
         ing_cmprsd_attr_selectors->offset_table_map[3].count_enable=1;
         /*INT_PRI*/
         for (counter_index=1;counter_index<(1<<4);counter_index++) {
              /*PktRes*/
              for (ignore_index=0;ignore_index<(1<<2);ignore_index++) {
                   ing_cmprsd_attr_selectors->
                       offset_table_map[(counter_index<<2)|ignore_index].
                       offset=(counter_index+2);
                   ing_cmprsd_attr_selectors->
                       offset_table_map[(counter_index<<2)|ignore_index].
                       count_enable=1;
              }
         }
         break;
    case bcmStatGroupModeTypedWithControl:
         /* **************************************************************** */
         /* A dedicated counter for control, unknown unicast, known unicast, */
         /* multicast, broadcast                                             */
         /* 1) CONTROL_PKT|BPDU_PKT                                          */
         /* 2) UNKNOWN_L3UC_PKT                                              */
         /* 3) L2UC_PKT | KNOWN_L3UC_PKT                                     */
         /* 4) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT                               */
         /* 5) L2BC_PKT                                                      */
         /* **************************************************************** */
         if (attr->direction == bcmStatFlexDirectionIngress) {
             total_counters=5;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 5);
             _bcm_esw_fillup_ing_pkt_res_offset_table(
                 ing_attr,
                 sizeof(ing_TypedWithControl_res)/
                 sizeof(ing_TypedWithControl_res[0]),
                 &ing_TypedWithControl_res[0]);
         } else {
             /* Group mode is overrided so  control shouldn't hit this part */
             FLEXCTR_ERR(("bcmStatGroupModeTypedWithControl" 
                          "is not supported in egress side\n"));
             sal_free(attr);
             return BCM_E_INTERNAL;
         }
         break;
    case bcmStatGroupModeTypedAllWithControl:
         /* ***************************************************************** */
         /* A dedicated counter for control, unknown unicast, known unicast,  */
         /* multicast, broadcast and one for all traffic (not already counted)*/
         /* 1) CONTROL_PKT|BPDU_PKT                                           */
         /* 2) UNKNOWN_L3UC_PKT                                               */
         /* 3) L2UC_PKT | KNOWN_L3UC_PKT                                      */
         /* 4) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT                                */
         /* 5) L2BC_PKT                                                       */
         /* 6) UNKNOWN_PKT|L2DLF_PKT|UNKNOWN_IPMC_PKT|KNOWN_IPMC_PKT          */
         /*    KNOWN_MPLS_PKT | KNOWN_MPLS_L3_PKT|KNOWN_MPLS_L2_PKT|          */
         /*    UNKNOWN_MPLS_PKT|KNOWN_MIM_PKT|UNKNOWN_MIM_PKT|                */
         /*    KNOWN_MPLS_MULTICAST_PKT                                       */
         /* ***************************************************************** */
         if (attr->direction == bcmStatFlexDirectionIngress) {
             total_counters=6;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS,
                 6);
             _bcm_esw_fillup_ing_pkt_res_offset_table(
                 ing_attr,
                 sizeof(ing_TypedAllWithControl_res)/
                 sizeof(ing_TypedAllWithControl_res[0]),
                 &ing_TypedAllWithControl_res[0]);
         } else {
             /* Group mode is overrided so  control shouldn't hit this part */
             FLEXCTR_ERR(("bcmStatGroupModeTypedAllWithControl"
                          " is not supported in egress side\n"));
             sal_free(attr);
             return BCM_E_INTERNAL;
         }
         break;
    case bcmStatGroupModeTypedIntPriWithControl:
         /* *************************************************************** */
         /* A dedicated counter for control, unknown unicast, known unicast */
         /* , multicast, broadcast and N internal priority counters for     */
         /* traffic (not already counted)                                   */
         /* 1) CONTROL_PKT|BPDU_PKT                                         */
         /* 2) UNKNOWN_L3UC_PKT                                             */
         /* 3) L2UC_PKT | KNOWN_L3UC_PKT                                    */
         /* 4) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT                              */
         /* 5) L2BC_PKT                                                     */
         /* 6..21) INT_PRI bits: 4bits                                      */
         /* *************************************************************** */
         if (attr->direction == bcmStatFlexDirectionEgress) {
             FLEXCTR_ERR(("GroupModeTypedIntPriWithControl is not available in "
                          "egress side\n"));
             sal_free(attr);
             return BCM_E_INTERNAL;
         }
         /* Although 21 counters but pkt_res(6) + int_pri(4)=10 bits
            so cannot use UNCOMPRESSED MODE */

         /* INT_PRI */ /* .... */ /* PacketRes */ 
         /* 34-31:4 */ /* .... */ /* 8-3:6     */
         /* PRI_CNG_FN=Cng(2x):IFP(2x):IntPri( all 4bits are used). 
             RightMost so no shifting required */
         /* PKT_RES_FN=Pkt(6bits  but 2 is used):SVP(1x):Drop(1x).
            LeftMost so 2 Left shifting required */

         total_counters=21;
         ing_attr->packet_attr_type=bcmStatFlexPacketAttrTypeCompressed;

         ing_cmprsd_pkt_attr_bits->pkt_resolution = 3;
         ing_cmprsd_pkt_attr_bits->pkt_resolution_pos = ing_pkt_attr_bits_g.
                                                        pkt_resolution_pos;
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->pkt_resolution_pos -= 2;
         }
         ing_cmprsd_pkt_attr_bits->pkt_resolution_mask = (1<<3)-1;
         ing_cmprsd_pkt_attr_bits->int_pri = ing_pkt_attr_bits_g.int_pri;
         ing_cmprsd_pkt_attr_bits->int_pri_pos = ing_pkt_attr_bits_g.
                                                 int_pri_pos;
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->int_pri_pos -= 2;
         }
         ing_cmprsd_pkt_attr_bits->int_pri_mask = ing_pkt_attr_bits_g.int_pri_mask;
         ing_cmprsd_attr_selectors->total_counters = 21;
            
         /* Reset pkt_resolution map */
         for (map_index=0; map_index < 256 ;map_index++) {
              ing_cmprsd_attr_selectors->pkt_res_attr_map[map_index]=0;
         }       
         /* set pkt_resolution map for  1 counters */
         /* set pkt_resolution map for  1 counters */
         /* Map[Encoded Packet value << 2=>[SVP-1bit + DROP-1bit]=
            CounterIndex << 2=>[SVP-1bit + DROP-1bit]=4(Start),8,12*/

         shift_by_bits= ing_pkt_attr_bits_g.svp_type + ing_pkt_attr_bits_g.drop;
         shift_by_bits_for_value = shift_by_bits;
         if (SOC_IS_KATANA2(unit)) {
             shift_by_bits_for_value -= 2;
         }

         for (ignore_index=0; 
              ignore_index < (1<<shift_by_bits) ; ignore_index++) {
              ing_cmprsd_attr_selectors->
                 pkt_res_attr_map[
                 (control_pkt<<shift_by_bits) | ignore_index]=
                 (1<<shift_by_bits_for_value);
              ing_cmprsd_attr_selectors->
                 pkt_res_attr_map[
                 (bpdu_pkt<<shift_by_bits) | ignore_index]=
                 (1<<shift_by_bits_for_value);
              ing_cmprsd_attr_selectors->
                 pkt_res_attr_map[
                 (unknown_l3uc_pkt<<shift_by_bits) | ignore_index]=
                 (2<<shift_by_bits_for_value);
              ing_cmprsd_attr_selectors->
                 pkt_res_attr_map[
                 (l2uc_pkt<<shift_by_bits) | ignore_index]=
                 (3<<shift_by_bits_for_value);
              ing_cmprsd_attr_selectors->
                 pkt_res_attr_map[
                 (known_l3uc_pkt<<shift_by_bits) | ignore_index]=
                 (3<<shift_by_bits_for_value);
              ing_cmprsd_attr_selectors->
                 pkt_res_attr_map[
                 (known_l2mc_pkt<<shift_by_bits) | ignore_index]=
                 (4<<shift_by_bits_for_value);
              ing_cmprsd_attr_selectors->
                 pkt_res_attr_map[
                 (unknown_l2mc_pkt<<shift_by_bits) | ignore_index]=
                 (4<<shift_by_bits_for_value);
              ing_cmprsd_attr_selectors->
                 pkt_res_attr_map[
                 (l2bc_pkt<<shift_by_bits) | ignore_index]=
                 (5<<shift_by_bits_for_value);
         }
         /* Reset pri_cng map */
         for (map_index=0; map_index < 256 ;map_index++) {
              ing_cmprsd_attr_selectors->pri_cnf_attr_map[map_index]=0;
         }       
         /* set pri_cng map for  16 counters */
         for (ignore_index=0; ignore_index < (1<<4) ; ignore_index++) {
              for (map_index=0; map_index < 16 ;map_index++) {
                   ing_cmprsd_attr_selectors->
                      pri_cnf_attr_map[(ignore_index<<4) | map_index]=map_index;
              }       
         }
                 
         /* Reset all Offset table fields */
         for (counter_index=0;counter_index<256;counter_index++) {
              ing_cmprsd_attr_selectors->
                        offset_table_map[counter_index].offset=0;
              ing_cmprsd_attr_selectors->
                        offset_table_map[counter_index].count_enable=0;
         }
         /* **************************************************************** */
         /* Set unicast, known unicast, multicast, broadcast counter indexes */
         /* considering INT_PRI bits 0                                       */
         /* **************************************************************** */
         ing_cmprsd_attr_selectors->offset_table_map[1].offset=0;
         ing_cmprsd_attr_selectors->offset_table_map[1].count_enable=1;
         ing_cmprsd_attr_selectors->offset_table_map[2].offset=1;
         ing_cmprsd_attr_selectors->offset_table_map[2].count_enable=1;
         ing_cmprsd_attr_selectors->offset_table_map[3].offset=2;
         ing_cmprsd_attr_selectors->offset_table_map[3].count_enable=1;
         ing_cmprsd_attr_selectors->offset_table_map[4].offset=3;
         ing_cmprsd_attr_selectors->offset_table_map[4].count_enable=1;
         ing_cmprsd_attr_selectors->offset_table_map[5].offset=4;
         ing_cmprsd_attr_selectors->offset_table_map[5].count_enable=1;

         /* Set IntPri counter indexes not-considering pkt resolution bits*/
         /* Priority 0 Counter not satisfying any condition  */
         ing_cmprsd_attr_selectors->offset_table_map[0].offset=5;
         ing_cmprsd_attr_selectors->offset_table_map[0].count_enable=1;
         ing_cmprsd_attr_selectors->offset_table_map[6].offset=5;
         ing_cmprsd_attr_selectors->offset_table_map[6].count_enable=1;
         ing_cmprsd_attr_selectors->offset_table_map[7].offset=5;
         ing_cmprsd_attr_selectors->offset_table_map[7].count_enable=1;

         /*INT_PRI*/
         for (counter_index=1;counter_index<(1<<4);counter_index++) {
              /*PktRes*/
              for (ignore_index=0;ignore_index<(1<<3);ignore_index++) {
                   ing_cmprsd_attr_selectors->
                       offset_table_map[(counter_index<<3)|ignore_index].
                       offset=(counter_index+5);
                   ing_cmprsd_attr_selectors->
                       offset_table_map[(counter_index<<3)|ignore_index].
                       count_enable=1;
              }
         }
         break;
    case bcmStatGroupModeDot1P:
         /* ******************************************************** */
         /* A set of 8(2^3) counters selected based on Vlan priority */
         /* outer_dot1p; 3 bits 1..8                                 */
         /* ******************************************************** */

         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=8;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS,
                 8);
             /* Set Offset table fields */
             for (counter_index=0;counter_index<8;counter_index++) {
                  ing_attr->uncmprsd_attr_selectors.
                   offset_table_map[counter_index].offset=counter_index;
                  ing_attr->uncmprsd_attr_selectors.
                   offset_table_map[counter_index].count_enable=1;
             }
             /* Set Offset table fields */
             for(counter_index=0;counter_index<256;counter_index++) {
                 ing_attr->uncmprsd_attr_selectors.
                   offset_table_map[counter_index].offset=counter_index;
                 ing_attr->uncmprsd_attr_selectors.
                   offset_table_map[counter_index].count_enable=1;
             }
         } else {
             total_counters=8;
             _bcm_esw_fillup_egr_uncmp_attr(
                 egr_attr,
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS,
                 8);
             /* Set Offset table fields */
             for (counter_index=0;counter_index<8;counter_index++) {
                  egr_attr->uncmprsd_attr_selectors.
                   offset_table_map[counter_index].offset=counter_index;
                  egr_attr->uncmprsd_attr_selectors.
                   offset_table_map[counter_index].count_enable=1;
             }
         }
         break;
    case bcmStatGroupModeIntPri:
         /* **************************************************** */
         /* A set of 16(2^4) counters based on internal priority */
         /* 1..16 INT_PRI bits: 4bits                            */
         /* **************************************************** */

         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=16;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS,
                 16);
             /* Set Offset table fields */
             for (counter_index=0;counter_index<16;counter_index++) {
                  ing_attr->uncmprsd_attr_selectors.
                          offset_table_map[counter_index].offset=counter_index;
                  ing_attr->uncmprsd_attr_selectors.
                          offset_table_map[counter_index].count_enable=1;
             }
         } else {
             total_counters=16;
             _bcm_esw_fillup_egr_uncmp_attr(
                 egr_attr,
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS,
                 16);
             /* Set Offset table fields */
             for (counter_index=0;counter_index<16;counter_index++) {
                  egr_attr->uncmprsd_attr_selectors.
                          offset_table_map[counter_index].offset=counter_index;
                  egr_attr->uncmprsd_attr_selectors.
                          offset_table_map[counter_index].count_enable=1;
             }
         }
         break;
    case bcmStatGroupModeIntPriCng:
         /* ********************************************************** */
         /* set of 64 counters(2^(4+2)) based on Internal priority+CNG */
         /* 1..64 (INT_PRI bits: 4bits + CNG 2 bits                    */
         /* 1..64 (INT_PRI bits: 4bits + CNG 2 bits                    */
         /* ********************************************************** */
         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=64;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_CNG_ATTR_BITS|
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS,
                 64);
             /* Set Offset table fields */
             counter_index=0;
             for (outer_index=0;outer_index<4;outer_index++) {/*CNG*/
                  for (inner_index=0;inner_index<16;inner_index++) {/*IntPri*/
                       ing_attr->uncmprsd_attr_selectors.
                          offset_table_map[counter_index].count_enable=1;
                       ing_attr->uncmprsd_attr_selectors.
                       offset_table_map[(outer_index<<4)|inner_index].
                       offset=counter_index++;
                  }
             }
         } else {
             total_counters=64;
             _bcm_esw_fillup_egr_uncmp_attr(
                 egr_attr,
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_CNG_ATTR_BITS|
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS,
                 64);
             /* Set Offset table fields */
             counter_index=0;
             for (outer_index=0;outer_index<4;outer_index++) {/*CNG*/
                  for(inner_index=0;inner_index<16;inner_index++) {/*IntPri*/
                      egr_attr->uncmprsd_attr_selectors.
                         offset_table_map[counter_index].count_enable=1;
                      egr_attr->uncmprsd_attr_selectors.
                         offset_table_map[(outer_index<<4)|inner_index].
                         offset=counter_index++;
                  }
             }
         }
         break;
    case bcmStatGroupModeSvpType:
         /* ****************************************** */
         /* A set of 2 counters(2^1) based on SVP type */
         /* 1..2 (SVP 1 bit)                           */
         /* ****************************************** */

         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=2;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS,
                 2);
             /* Set Offset table fields */
             /* DropBitShifting=1 is required */
             for (counter_index=0;counter_index<2;counter_index++) {
                  ing_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index<<1].offset=counter_index;
                  ing_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index<<1].count_enable=1;
             }
         } else {
             total_counters=2;
             _bcm_esw_fillup_egr_uncmp_attr(
                 egr_attr,
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS,
                 2);
             /* Set Offset table fields */
             /* DropBitShifting=1 + DvpBitShifting=1 ==> 2 is required */
             for (counter_index=0;counter_index<2;counter_index++) {
                  egr_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index<<2].offset=counter_index;
                  egr_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index<<2].count_enable=1;
             }
         }
         break;
    case bcmStatGroupModeDscp:
         /* ******************************************** */
         /* A set of 64 counters(2^6) based on DSCP bits */
         /* 1..64 (6 bits from TOS 8 bits)               */
         /* ******************************************** */

         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=64;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_TOS_ATTR_BITS,
                 64);
             /* Set Offset table fields */
             for (counter_index=0;counter_index<64;counter_index++) {
                  ing_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index<<2].offset=counter_index;
                  ing_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index<<2].count_enable=1;
             }
         } else {
             total_counters=64;
             _bcm_esw_fillup_egr_uncmp_attr(
                 egr_attr,
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_TOS_ATTR_BITS,
                 64);
             /* Set Offset table fields */
             for (counter_index=0;counter_index<2;counter_index++) {
                  egr_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index<<2].offset=counter_index;
                  egr_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index<<2].count_enable=1;
             }
         }
         break;
    case bcmStatGroupModeDvpType:
         /* ******************************************** */
         /* EGRESS SIDE ONLY:                            */
         /* A set of 2 counters(2^1) based on DVP type   */
         /* 1..2 (DVP 1 bits)                            */
         /* ******************************************** */

         if (attr->direction==bcmStatFlexDirectionIngress) {
             FLEXCTR_ERR(("bcm_stat_group_mode_t %d is NotSupported"
                           " in IngressSide\n", group_mode));
             sal_free(attr);
             return BCM_E_PARAM;
         } else {
             total_counters=2;
             _bcm_esw_fillup_egr_uncmp_attr(
                 egr_attr,
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_DVP_TYPE_ATTR_BITS,
                 2);
             /* Set Offset table fields */
             for (counter_index=0;counter_index<2;counter_index++) {
                  egr_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index<<1].offset=counter_index;
                  egr_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index<<1].count_enable=1;
             }
         }
         break;
    case bcmStatGroupModeCng:
         if (attr->direction==bcmStatFlexDirectionIngress) {
             total_counters=4;
             _bcm_esw_fillup_ing_uncmp_attr(
                 ing_attr,
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_CNG_ATTR_BITS,
                 4);
             /* Set Offset table fields */
             for (counter_index=0;counter_index<4;counter_index++) {
                  ing_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index].offset=counter_index;
                  ing_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index].count_enable=1;
             }
         } else {
             total_counters=4;
             _bcm_esw_fillup_egr_uncmp_attr(
                 egr_attr,
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_CNG_ATTR_BITS,
                 4);
             /* Set Offset table fields */
             for (counter_index=0;counter_index<4;counter_index++) {
                  egr_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index].offset=counter_index;
                  egr_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index].count_enable=1;
             }
         }
         break;
    };
    rv =  _bcm_esw_stat_flex_create_mode(unit,attr,&mode);
    if ((rv == BCM_E_NONE) || (rv==BCM_E_EXISTS)) {
         BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_set_group_mode(
                             unit,attr->direction, mode,group_mode));
         rv = BCM_E_NONE;
         switch(object) {
         /* Ingress Side */
         case bcmStatObjectIngPort:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, PORT_TABm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngVlan:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, VLAN_TABm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngVlanXlate:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, VLAN_XLATEm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngVfi:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, VFIm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngL3Intf:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, L3_IIFm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngVrf:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, VRFm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngPolicy:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, VFP_POLICY_TABLEm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngNiv:
         case bcmStatObjectIngMplsVcLabel:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, SOURCE_VPm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngMplsSwitchLabel:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, MPLS_ENTRYm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngMplsFrrLabel:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, L3_TUNNELm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngL3Host:
              /* To Be Completed..L3_ENTRY_2/4,EXT_IPV4/6_128_UCAST_WIDE */
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, EXT_IPV4_UCAST_WIDEm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngTrill:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, MPLS_ENTRY_EXTDm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngMimLookupId:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, MPLS_ENTRY_EXTDm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngL2Gre:
              /* To Be Completed..SOURCE_VPm, VLAN_XLATE_1m L2GRE_DIP view */
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, VLAN_XLATEm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngEXTPolicy:
              /* To Be Completed..*/
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, VFP_POLICY_TABLEm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngVxlan:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, VLAN_XLATEm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngVsan:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, ING_VSANm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngFcoe:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, L3_ENTRY_IPV4_MULTICASTm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectIngL3Route:
              /* To include L3_DEFIP_AUX_SCRATCH L3_DEFIP_ALPM_IPV4 */
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, L3_DEFIPm,object,mode,
                        &base_index,&pool_number);
              break;
         /* Egress Side */
         case bcmStatObjectEgrPort:
              rv = _bcm_esw_stat_flex_create_egress_table_counters(
                        unit, EGR_PORTm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectEgrVlan:
              rv = _bcm_esw_stat_flex_create_egress_table_counters(
                        unit, EGR_VLANm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectEgrVlanXlate:
              rv = _bcm_esw_stat_flex_create_egress_table_counters(
                        unit, EGR_VLAN_XLATEm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectEgrVfi:
              rv = _bcm_esw_stat_flex_create_egress_table_counters(
                        unit, EGR_VFIm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectEgrNiv:
         case bcmStatObjectEgrL3Intf:
              rv = _bcm_esw_stat_flex_create_egress_table_counters(
                        unit, EGR_L3_NEXT_HOPm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectEgrWlan:
              rv = _bcm_esw_stat_flex_create_egress_table_counters(
                        unit, EGR_L3_NEXT_HOPm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectEgrMim:
              rv = _bcm_esw_stat_flex_create_egress_table_counters(
                        unit, EGR_L3_NEXT_HOPm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectEgrMimLookupId:
              rv = _bcm_esw_stat_flex_create_egress_table_counters(
                        unit, EGR_VLAN_XLATEm,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectEgrL2Gre:
              rv = _bcm_esw_stat_flex_create_egress_table_counters(
                        unit, EGR_DVP_ATTRIBUTE_1m,object,mode,
                        &base_index,&pool_number);
              break;
         case bcmStatObjectEgrVxlan:
              rv = _bcm_esw_stat_flex_create_egress_table_counters(
                        unit, EGR_DVP_ATTRIBUTE_1m, object,mode,
                        &base_index, &pool_number);
              break;
         case bcmStatObjectEgrL3Nat:
              rv = _bcm_esw_stat_flex_create_egress_table_counters(
                        unit, EGR_NAT_PACKET_EDIT_INFOm, object,mode,
                        &base_index, &pool_number);
              break;
         }
    }
    /* Cleanup activity ... */
    sal_free(attr);
    if (BCM_FAILURE(rv)) {
        FLEXCTR_ERR(("creation of counters passed  failed..\n"));
    } else {
        /* mode_id =  Max 3 bits (Total Eight modes) */
        /* group_mode_id = Max 5 bits (Total 32)     */
        /* pool_id = 4 (Max Pool:16)                 */
        /* a/c object_id=4 (Max Object:16)           */
        /* 2 bytes for base index                    */
        /* 000    0-0000    0000   -0000      0000-0000 0000-0000   */
        /* Mode3b Group5b   Pool4b -A/cObj4b  base-index            */
    
        _bcm_esw_stat_get_counter_id(group_mode,object, mode,
                                     pool_number,base_index,stat_counter_id);
        FLEXCTR_VVERB(("Create: mode:%d group_mode:%d pool:%d object:%d"
                       " base:%d\n stat_counter_id:%d\n",
                       mode,group_mode,pool_number,object,base_index,
                       *stat_counter_id));
        *num_entries=total_counters;
    }
    return rv;
}
/*
 * Function:
 *      _bcm_esw_stat_group_destroy
 * Description:
 *      Release HW counter resources as per given counter id and makes system 
 *      unavailable for any further stat collection action 
 * Parameters:
 *      unit            - (IN) unit number
 *      Stat_counter_id - (IN) Stat Counter Id
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      
 */
bcm_error_t _bcm_esw_stat_group_destroy(
            int	   unit,
            uint32 stat_counter_id)
{
    bcm_error_t               rv=BCM_E_NONE;
    uint32                    pool_number=0;
    uint32                    base_index=0;
    bcm_stat_flex_mode_t      offset_mode=0;
    bcm_stat_object_t         object=bcmStatObjectIngPort;
    bcm_stat_group_mode_t     group_mode= bcmStatGroupModeSingle;
    bcm_stat_flex_direction_t direction=bcmStatFlexDirectionIngress;

    if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
         return BCM_E_UNAVAIL;
    }

    _bcm_esw_stat_get_counter_id_info(stat_counter_id,&group_mode,&object, 
                                      &offset_mode,&pool_number,&base_index);
    FLEXCTR_VVERB(("Deleting : mode:%d group_mode:%d pool:%d object:%d"
                   "base:%d\n stat_counter_id:%d\n",offset_mode,group_mode,
                   pool_number,object,base_index,stat_counter_id));

    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_object(unit,object,&direction));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_group(unit,group_mode));
    switch(object) {
         /* Ingress Side */
         case bcmStatObjectIngPort:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, PORT_TABm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngVlan:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, VLAN_TABm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngVlanXlate:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, VLAN_XLATEm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngVfi:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, VFIm, object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngL3Intf:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, L3_IIFm, object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngVrf:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, VRFm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngPolicy:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, VFP_POLICY_TABLEm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngNiv:
         case bcmStatObjectIngMplsVcLabel:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, SOURCE_VPm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngMplsSwitchLabel:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, MPLS_ENTRYm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngMplsFrrLabel:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, L3_TUNNELm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngL3Host:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, EXT_IPV4_UCAST_WIDEm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngTrill:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, MPLS_ENTRY_EXTDm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngMimLookupId:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, MPLS_ENTRY_EXTDm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngL2Gre:
              /* To Be Completed..SOURCE_VPm, VLAN_XLATE_1m L2GRE_DIP view */
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, VLAN_XLATEm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngEXTPolicy:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, VFP_POLICY_TABLEm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngVxlan:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, VLAN_XLATEm, object, offset_mode,
                        base_index, pool_number);
              break;
         case bcmStatObjectIngVsan:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, ING_VSANm,object,offset_mode,
                        base_index, pool_number);
              break;
         case bcmStatObjectIngFcoe:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, L3_ENTRY_IPV4_MULTICASTm, object, offset_mode,
                        base_index, pool_number);
              break;
         case bcmStatObjectIngL3Route:
              /* To include L3_DEFIP_AUX_SCRATCH L3_DEFIP_ALPM_IPV4 */
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, L3_DEFIPm, object, offset_mode,
                        base_index, pool_number);
              break;
         /* Egress Side */
         case bcmStatObjectEgrPort:
              rv = _bcm_esw_stat_flex_destroy_egress_table_counters(
                        unit, EGR_PORTm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectEgrVlan:
              rv = _bcm_esw_stat_flex_destroy_egress_table_counters(
                        unit, EGR_VLANm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectEgrVlanXlate:
              rv = _bcm_esw_stat_flex_destroy_egress_table_counters(
                        unit, EGR_VLAN_XLATEm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectEgrVfi:
              rv = _bcm_esw_stat_flex_destroy_egress_table_counters(
                        unit, EGR_VFIm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectEgrNiv:
         case bcmStatObjectEgrL3Intf:
              rv = _bcm_esw_stat_flex_destroy_egress_table_counters(
                        unit, EGR_L3_NEXT_HOPm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectEgrWlan:
              rv = _bcm_esw_stat_flex_destroy_egress_table_counters(
                        unit, EGR_L3_NEXT_HOPm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectEgrMim:
              rv = _bcm_esw_stat_flex_destroy_egress_table_counters(
                        unit, EGR_L3_NEXT_HOPm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectEgrMimLookupId:
              rv = _bcm_esw_stat_flex_destroy_egress_table_counters(
                        unit, EGR_VLAN_XLATEm ,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectEgrL2Gre:
              rv = _bcm_esw_stat_flex_destroy_egress_table_counters(
                        unit, EGR_DVP_ATTRIBUTE_1m ,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectEgrVxlan:
              rv = _bcm_esw_stat_flex_destroy_egress_table_counters(
                        unit, EGR_DVP_ATTRIBUTE_1m, object, offset_mode,
                        base_index, pool_number);
              break;
         case bcmStatObjectEgrL3Nat:
              rv = _bcm_esw_stat_flex_destroy_egress_table_counters(
                        unit, EGR_NAT_PACKET_EDIT_INFOm, object, offset_mode,
                        base_index, pool_number);
              break;

    }
    if (BCM_SUCCESS(rv)) {
        FLEXCTR_VVERB(("Destroyed egress table counters.."
                       "Trying to delete group mode itself \n"));
        /* No decision on return values as actual call is successful */
        if (direction == bcmStatFlexDirectionIngress) {
            if (_bcm_esw_stat_flex_delete_ingress_mode(
                unit,offset_mode) == BCM_E_NONE) {
                FLEXCTR_VVERB(("Destroyed Ingress Mode also \n"));
                _bcm_esw_stat_flex_reset_group_mode(
                                    unit,bcmStatFlexDirectionIngress,
                                    offset_mode,group_mode);
            }
       } else {
           if (_bcm_esw_stat_flex_delete_egress_mode(
               unit,offset_mode) == BCM_E_NONE) {
                FLEXCTR_VVERB(("Destroyed Egress Mode also \n"));
                _bcm_esw_stat_flex_reset_group_mode(
                                    unit,bcmStatFlexDirectionEgress,
                                    offset_mode,group_mode);
           }
       }
    }
    return rv;
}
/*
 * Function:
 *      _bcm_esw_stat_flex_init_pkt_attr_bits
 * Description:
 *      Initialize All Packet attr bits.
 *      
 * Parameters:
 *      unit            - (IN) unit number
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      
 */
void _bcm_esw_stat_flex_init_pkt_attr_bits(int unit)
{
   if (SOC_IS_TD2_TT2(unit)) {
       /* Egress bits initialization */
       egr_pkt_attr_bits_g.cng = 2;
       egr_pkt_attr_bits_g.cng_pos = 32;
       egr_pkt_attr_bits_g.cng_mask = (1 << egr_pkt_attr_bits_g.cng) - 1;

       egr_pkt_attr_bits_g.int_pri = 4;
       egr_pkt_attr_bits_g.int_pri_pos = 28;
       egr_pkt_attr_bits_g.int_pri_mask = 
                           (1 << egr_pkt_attr_bits_g.int_pri) - 1;

       egr_pkt_attr_bits_g.vlan_format = 2;
       egr_pkt_attr_bits_g.vlan_format_pos = 26;
       egr_pkt_attr_bits_g.vlan_format_mask = 
                           (1<<egr_pkt_attr_bits_g.vlan_format) - 1;

       egr_pkt_attr_bits_g.outer_dot1p = 3;
       egr_pkt_attr_bits_g.outer_dot1p_pos = 23;
       egr_pkt_attr_bits_g.outer_dot1p_mask = 
                           (1<<egr_pkt_attr_bits_g.outer_dot1p) - 1;

       egr_pkt_attr_bits_g.inner_dot1p = 3;
       egr_pkt_attr_bits_g.inner_dot1p_pos = 20;
       egr_pkt_attr_bits_g.inner_dot1p_mask = 
                           (1<<egr_pkt_attr_bits_g.inner_dot1p) - 1;

       egr_pkt_attr_bits_g.egr_port = 7; /* For KT2,TR3 6 bits */
       egr_pkt_attr_bits_g.egr_port_pos = 13;
       egr_pkt_attr_bits_g.egr_port_mask = 
                           (1<<egr_pkt_attr_bits_g.egr_port) - 1;

       egr_pkt_attr_bits_g.tos = 8;
       egr_pkt_attr_bits_g.tos_pos = 5;
       egr_pkt_attr_bits_g.tos_mask = 
                           (1<<egr_pkt_attr_bits_g.tos) - 1;

       egr_pkt_attr_bits_g.pkt_resolution = 1;
       egr_pkt_attr_bits_g.pkt_resolution_pos = 4;
       egr_pkt_attr_bits_g.pkt_resolution_mask = 
                           (1<<egr_pkt_attr_bits_g.pkt_resolution) - 1;

       egr_pkt_attr_bits_g.svp_type = 1;
       egr_pkt_attr_bits_g.svp_type_pos = 3;
       egr_pkt_attr_bits_g.svp_type_mask = 
                           (1<<egr_pkt_attr_bits_g.svp_type) - 1;

       egr_pkt_attr_bits_g.dvp_type = 1;
       egr_pkt_attr_bits_g.dvp_type_pos = 2;
       egr_pkt_attr_bits_g.dvp_type_mask = 
                           (1<<egr_pkt_attr_bits_g.dvp_type) - 1;

       egr_pkt_attr_bits_g.drop = 1;
       egr_pkt_attr_bits_g.drop_pos = 1;
       egr_pkt_attr_bits_g.drop_mask = 
                           (1<<egr_pkt_attr_bits_g.drop) - 1;

       egr_pkt_attr_bits_g.ip_pkt = 1;
       egr_pkt_attr_bits_g.ip_pkt_pos = 0;
       egr_pkt_attr_bits_g.ip_pkt_mask = 
                           (1<<egr_pkt_attr_bits_g.ip_pkt) - 1;

       /* Ingress bits initialization */
       ing_pkt_attr_bits_g.cng = 2;
       ing_pkt_attr_bits_g.cng_pos = 38;
       ing_pkt_attr_bits_g.cng_mask = 
                           (1<<ing_pkt_attr_bits_g.cng) - 1;

       ing_pkt_attr_bits_g.ifp_cng = 2;
       ing_pkt_attr_bits_g.ifp_cng_pos = 36;
       ing_pkt_attr_bits_g.ifp_cng_mask = 
                           (1<<ing_pkt_attr_bits_g.ifp_cng) - 1;

       ing_pkt_attr_bits_g.int_pri = 4;
       ing_pkt_attr_bits_g.int_pri_pos = 32;
       ing_pkt_attr_bits_g.int_pri_mask = 
                           (1<<ing_pkt_attr_bits_g.int_pri) - 1;

       ing_pkt_attr_bits_g.vlan_format = 2;
       ing_pkt_attr_bits_g.vlan_format_pos = 30;
       ing_pkt_attr_bits_g.vlan_format_mask = 
                           (1<<ing_pkt_attr_bits_g.vlan_format) - 1;

       ing_pkt_attr_bits_g.outer_dot1p = 3;
       ing_pkt_attr_bits_g.outer_dot1p_pos = 27;
       ing_pkt_attr_bits_g.outer_dot1p_mask = 
                           (1<<ing_pkt_attr_bits_g.outer_dot1p) - 1;

       ing_pkt_attr_bits_g.inner_dot1p = 3;
       ing_pkt_attr_bits_g.inner_dot1p_pos = 24;
       ing_pkt_attr_bits_g.inner_dot1p_mask = 
                           (1<<ing_pkt_attr_bits_g.inner_dot1p) - 1;

       ing_pkt_attr_bits_g.ing_port = 7; /* FOR TD2, 7 bits */
       ing_pkt_attr_bits_g.ing_port_pos = 17;
       ing_pkt_attr_bits_g.ing_port_mask = 
                           (1<<ing_pkt_attr_bits_g.ing_port) - 1;

       ing_pkt_attr_bits_g.tos = 8;
       ing_pkt_attr_bits_g.tos_pos = 9;
       ing_pkt_attr_bits_g.tos_mask = 
                           (1<<ing_pkt_attr_bits_g.tos) - 1;

       ing_pkt_attr_bits_g.pkt_resolution = 6;
       ing_pkt_attr_bits_g.pkt_resolution_pos = 3;
       ing_pkt_attr_bits_g.pkt_resolution_mask = 
                           (1<<ing_pkt_attr_bits_g.pkt_resolution) - 1;

       ing_pkt_attr_bits_g.svp_type = 1;
       ing_pkt_attr_bits_g.svp_type_pos = 2;
       ing_pkt_attr_bits_g.svp_type_mask = 
                           (1<<ing_pkt_attr_bits_g.svp_type) - 1;

       ing_pkt_attr_bits_g.drop = 1;
       ing_pkt_attr_bits_g.drop_pos = 1;
       ing_pkt_attr_bits_g.drop_mask = 
                           (1<<ing_pkt_attr_bits_g.drop_mask) - 1;

       ing_pkt_attr_bits_g.ip_pkt = 1;
       ing_pkt_attr_bits_g.ip_pkt_pos = 0;
       ing_pkt_attr_bits_g.ip_pkt_mask = 
                           (1<<ing_pkt_attr_bits_g.ip_pkt) - 1;
   } else { 
       /* Egress bits initialization */
       if (SOC_IS_KATANA2(unit)) {
           egr_pkt_attr_bits_g.cng = 2;
           egr_pkt_attr_bits_g.cng_pos = 37;
           egr_pkt_attr_bits_g.cng_mask = (1 << egr_pkt_attr_bits_g.cng) - 1;

           egr_pkt_attr_bits_g.int_pri = 4;
           egr_pkt_attr_bits_g.int_pri_pos = 33;
           egr_pkt_attr_bits_g.int_pri_mask = 
                               (1 << egr_pkt_attr_bits_g.int_pri) - 1;

           egr_pkt_attr_bits_g.vlan_format = 2;
           egr_pkt_attr_bits_g.vlan_format_pos = 31;
           egr_pkt_attr_bits_g.vlan_format_mask = 
                               (1<<egr_pkt_attr_bits_g.vlan_format) - 1;

           egr_pkt_attr_bits_g.outer_dot1p = 3;
           egr_pkt_attr_bits_g.outer_dot1p_pos = 28;
           egr_pkt_attr_bits_g.outer_dot1p_mask = 
                               (1<<egr_pkt_attr_bits_g.outer_dot1p) - 1;

           egr_pkt_attr_bits_g.inner_dot1p = 3;
           egr_pkt_attr_bits_g.inner_dot1p_pos = 25;
           egr_pkt_attr_bits_g.inner_dot1p_mask = 
                               (1<<egr_pkt_attr_bits_g.inner_dot1p) - 1;

           egr_pkt_attr_bits_g.egr_port = 8; /* For KT2 : 8 bits */
           egr_pkt_attr_bits_g.egr_port_pos = 17;
           egr_pkt_attr_bits_g.egr_port_mask = 
                               (1<<egr_pkt_attr_bits_g.egr_port) - 1;

           egr_pkt_attr_bits_g.tos = 8;
           egr_pkt_attr_bits_g.tos_pos = 9;
           egr_pkt_attr_bits_g.tos_mask = 
                               (1<<egr_pkt_attr_bits_g.tos) - 1;

           egr_pkt_attr_bits_g.pkt_resolution = 1;
           egr_pkt_attr_bits_g.pkt_resolution_pos = 8;
           egr_pkt_attr_bits_g.pkt_resolution_mask = 
                               (1<<egr_pkt_attr_bits_g.pkt_resolution) - 1;

           egr_pkt_attr_bits_g.svp_type = 3;
           egr_pkt_attr_bits_g.svp_type_pos = 5;
           egr_pkt_attr_bits_g.svp_type_mask = 
                               (1<<egr_pkt_attr_bits_g.svp_type) - 1;

           egr_pkt_attr_bits_g.dvp_type = 3;
           egr_pkt_attr_bits_g.dvp_type_pos = 2;
           egr_pkt_attr_bits_g.dvp_type_mask = 
                               (1<<egr_pkt_attr_bits_g.dvp_type) - 1;
       } else {
           egr_pkt_attr_bits_g.cng = 2;
           egr_pkt_attr_bits_g.cng_pos = 31;
           egr_pkt_attr_bits_g.cng_mask = (1 << egr_pkt_attr_bits_g.cng) - 1;

           egr_pkt_attr_bits_g.int_pri = 4;
           egr_pkt_attr_bits_g.int_pri_pos = 27;
           egr_pkt_attr_bits_g.int_pri_mask = 
                               (1 << egr_pkt_attr_bits_g.int_pri) - 1;

           egr_pkt_attr_bits_g.vlan_format = 2;
           egr_pkt_attr_bits_g.vlan_format_pos = 25;
           egr_pkt_attr_bits_g.vlan_format_mask = 
                               (1<<egr_pkt_attr_bits_g.vlan_format) - 1;

           egr_pkt_attr_bits_g.outer_dot1p = 3;
           egr_pkt_attr_bits_g.outer_dot1p_pos = 22;
           egr_pkt_attr_bits_g.outer_dot1p_mask = 
                               (1<<egr_pkt_attr_bits_g.outer_dot1p) - 1;

           egr_pkt_attr_bits_g.inner_dot1p = 3;
           egr_pkt_attr_bits_g.inner_dot1p_pos = 19;
           egr_pkt_attr_bits_g.inner_dot1p_mask = 
                               (1<<egr_pkt_attr_bits_g.inner_dot1p) - 1;

           egr_pkt_attr_bits_g.egr_port = 6; /* For KT2,TR3 6 bits */
           egr_pkt_attr_bits_g.egr_port_pos = 13;
           egr_pkt_attr_bits_g.egr_port_mask = 
                               (1<<egr_pkt_attr_bits_g.egr_port) - 1;

           egr_pkt_attr_bits_g.tos = 8;
           egr_pkt_attr_bits_g.tos_pos = 5;
           egr_pkt_attr_bits_g.tos_mask = 
                               (1<<egr_pkt_attr_bits_g.tos) - 1;

           egr_pkt_attr_bits_g.pkt_resolution = 1;
           egr_pkt_attr_bits_g.pkt_resolution_pos = 4;
           egr_pkt_attr_bits_g.pkt_resolution_mask = 
                               (1<<egr_pkt_attr_bits_g.pkt_resolution) - 1;
    
           egr_pkt_attr_bits_g.svp_type = 1;
           egr_pkt_attr_bits_g.svp_type_pos = 3;
           egr_pkt_attr_bits_g.svp_type_mask = 
                               (1<<egr_pkt_attr_bits_g.svp_type) - 1;

           egr_pkt_attr_bits_g.dvp_type = 1;
           egr_pkt_attr_bits_g.dvp_type_pos = 2;
           egr_pkt_attr_bits_g.dvp_type_mask = 
                               (1<<egr_pkt_attr_bits_g.dvp_type) - 1;
       }
       egr_pkt_attr_bits_g.drop = 1;
       egr_pkt_attr_bits_g.drop_pos = 1;
       egr_pkt_attr_bits_g.drop_mask = 
                           (1<<egr_pkt_attr_bits_g.drop) - 1;

       egr_pkt_attr_bits_g.ip_pkt = 1;
       egr_pkt_attr_bits_g.ip_pkt_pos = 0;
       egr_pkt_attr_bits_g.ip_pkt_mask = 
                           (1<<egr_pkt_attr_bits_g.ip_pkt) - 1;

       /* Ingress bits initialization */
       if (SOC_IS_KATANA2(unit)) {
           ing_pkt_attr_bits_g.cng = 2;
           ing_pkt_attr_bits_g.cng_pos = 41;
           ing_pkt_attr_bits_g.cng_mask =
                               (1<<ing_pkt_attr_bits_g.cng) - 1;

           ing_pkt_attr_bits_g.ifp_cng = 2;
           ing_pkt_attr_bits_g.ifp_cng_pos = 39;
           ing_pkt_attr_bits_g.ifp_cng_mask =
                               (1<<ing_pkt_attr_bits_g.ifp_cng) - 1;

           ing_pkt_attr_bits_g.int_pri = 4;
           ing_pkt_attr_bits_g.int_pri_pos = 35;
           ing_pkt_attr_bits_g.int_pri_mask =
                               (1<<ing_pkt_attr_bits_g.int_pri) - 1;

           ing_pkt_attr_bits_g.vlan_format = 2;
           ing_pkt_attr_bits_g.vlan_format_pos = 33;
           ing_pkt_attr_bits_g.vlan_format_mask =
                               (1<<ing_pkt_attr_bits_g.vlan_format) - 1;

           ing_pkt_attr_bits_g.outer_dot1p = 3;
           ing_pkt_attr_bits_g.outer_dot1p_pos = 30;
           ing_pkt_attr_bits_g.outer_dot1p_mask =
                               (1<<ing_pkt_attr_bits_g.outer_dot1p) - 1;

           ing_pkt_attr_bits_g.inner_dot1p = 3;
           ing_pkt_attr_bits_g.inner_dot1p_pos = 27;
           ing_pkt_attr_bits_g.inner_dot1p_mask =
                               (1<<ing_pkt_attr_bits_g.inner_dot1p) - 1;

           ing_pkt_attr_bits_g.ing_port = 8; /* For KT2,TR3 6 bits */
           ing_pkt_attr_bits_g.ing_port_pos = 19;
           ing_pkt_attr_bits_g.ing_port_mask =
                               (1<<ing_pkt_attr_bits_g.ing_port) - 1;

           ing_pkt_attr_bits_g.tos = 8;
           ing_pkt_attr_bits_g.tos_pos = 11;
           ing_pkt_attr_bits_g.tos_mask =
                               (1<<ing_pkt_attr_bits_g.tos) - 1;

           ing_pkt_attr_bits_g.pkt_resolution = 6;
           ing_pkt_attr_bits_g.pkt_resolution_pos = 5;
           ing_pkt_attr_bits_g.pkt_resolution_mask =
                               (1<<ing_pkt_attr_bits_g.pkt_resolution) - 1;

           ing_pkt_attr_bits_g.svp_type = 3;
           ing_pkt_attr_bits_g.svp_type_pos = 2;
           ing_pkt_attr_bits_g.svp_type_mask =
                               (1<<ing_pkt_attr_bits_g.svp_type) - 1;
       } else {
           ing_pkt_attr_bits_g.cng = 2;
           ing_pkt_attr_bits_g.cng_pos = 37;
           ing_pkt_attr_bits_g.cng_mask =
                               (1<<ing_pkt_attr_bits_g.cng) - 1;

           ing_pkt_attr_bits_g.ifp_cng = 2;
           ing_pkt_attr_bits_g.ifp_cng_pos = 35;
           ing_pkt_attr_bits_g.ifp_cng_mask =
                               (1<<ing_pkt_attr_bits_g.ifp_cng) - 1;

           ing_pkt_attr_bits_g.int_pri = 4;
           ing_pkt_attr_bits_g.int_pri_pos = 31;
           ing_pkt_attr_bits_g.int_pri_mask =
                               (1<<ing_pkt_attr_bits_g.int_pri) - 1;

           ing_pkt_attr_bits_g.vlan_format = 2;
           ing_pkt_attr_bits_g.vlan_format_pos = 29;
           ing_pkt_attr_bits_g.vlan_format_mask =
                               (1<<ing_pkt_attr_bits_g.vlan_format) - 1;

           ing_pkt_attr_bits_g.outer_dot1p = 3;
           ing_pkt_attr_bits_g.outer_dot1p_pos = 26;
           ing_pkt_attr_bits_g.outer_dot1p_mask =
                               (1<<ing_pkt_attr_bits_g.outer_dot1p) - 1;

           ing_pkt_attr_bits_g.inner_dot1p = 3;
           ing_pkt_attr_bits_g.inner_dot1p_pos = 23;
           ing_pkt_attr_bits_g.inner_dot1p_mask =
                               (1<<ing_pkt_attr_bits_g.inner_dot1p) - 1;

           ing_pkt_attr_bits_g.ing_port = 6; /* For KT2,TR3 6 bits */
           ing_pkt_attr_bits_g.ing_port_pos = 17;
           ing_pkt_attr_bits_g.ing_port_mask =
                               (1<<ing_pkt_attr_bits_g.ing_port) - 1;

           ing_pkt_attr_bits_g.tos = 8;
           ing_pkt_attr_bits_g.tos_pos = 9;
           ing_pkt_attr_bits_g.tos_mask =
                               (1<<ing_pkt_attr_bits_g.tos) - 1;

           ing_pkt_attr_bits_g.pkt_resolution = 6;
           ing_pkt_attr_bits_g.pkt_resolution_pos = 3;
           ing_pkt_attr_bits_g.pkt_resolution_mask =
                               (1<<ing_pkt_attr_bits_g.pkt_resolution) - 1;

           ing_pkt_attr_bits_g.svp_type = 1;
           ing_pkt_attr_bits_g.svp_type_pos = 2;
           ing_pkt_attr_bits_g.svp_type_mask =
                               (1<<ing_pkt_attr_bits_g.svp_type) - 1;
       }
       ing_pkt_attr_bits_g.drop = 1;
       ing_pkt_attr_bits_g.drop_pos = 1;
       ing_pkt_attr_bits_g.drop_mask =
                           (1<<ing_pkt_attr_bits_g.drop) - 1;

       ing_pkt_attr_bits_g.ip_pkt = 1;
       ing_pkt_attr_bits_g.ip_pkt_pos = 0;
       ing_pkt_attr_bits_g.ip_pkt_mask =
                           (1<<ing_pkt_attr_bits_g.ip_pkt) - 1;
   }
}
/*
 * Function:
 *      _bcm_esw_stat_flex_init_pkt_res_fields
 * Description:
 *      Initialize All Packet Resolution related (static) structures.
 *      Happens only one time.
 *      
 * Parameters:
 *      unit            - (IN) unit number
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      
 */
void _bcm_esw_stat_flex_init_pkt_res_fields(int unit)
{
  static uint32 init_flag=0;
  if (init_flag==0) {
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit, ing_Single_res,
          sizeof(ing_Single_res)/sizeof(ing_Single_res[0]));
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit,ing_TrafficType_res,
          sizeof(ing_TrafficType_res)/sizeof(ing_TrafficType_res[0]));
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit,ing_DlfAll_res,
          sizeof(ing_DlfAll_res)/sizeof(ing_DlfAll_res[0]));
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit,ing_Typed_res,
          sizeof(ing_Typed_res)/sizeof(ing_Typed_res[0]));
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit,ing_TypedAll_res,
          sizeof(ing_TypedAll_res)/sizeof(ing_TypedAll_res[0]));
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit,ing_SingleWithControl_res,
          sizeof(ing_SingleWithControl_res)/
          sizeof(ing_SingleWithControl_res[0]));
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit,ing_TrafficTypeWithControl_res,
          sizeof(ing_TrafficTypeWithControl_res)/
          sizeof(ing_TrafficTypeWithControl_res[0]));
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit,ing_DlfAllWithControl_res,
          sizeof(ing_DlfAllWithControl_res)/
          sizeof(ing_DlfAllWithControl_res[0]));
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit,ing_TypedWithControl_res,
          sizeof(ing_TypedWithControl_res)/
          sizeof(ing_TypedWithControl_res[0]));
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit,ing_TypedAllWithControl_res,
          sizeof(ing_TypedAllWithControl_res)/
          sizeof(ing_TypedAllWithControl_res[0]));
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit,egr_Single_res,
          sizeof(egr_Single_res)/sizeof(egr_Single_res[0]));
      _bcm_esw_stat_flex_init_pkt_res_values(
          unit,egr_TrafficType_res,
          sizeof(egr_TrafficType_res)/sizeof(egr_TrafficType_res[0]));
      init_flag=1;
  }
}
uint32 _bcm_esw_stat_flex_get_pkt_res_value(int unit,uint32 pkt_res_field)
{
   uint32 *flex_pkt_res_values=NULL;
   uint32 flex_pkt_res_values_count=0;
  
   if (SOC_IS_KATANAX(unit)) {
       flex_pkt_res_values = _flex_pkt_res_values_katana;
       flex_pkt_res_values_count = sizeof(_flex_pkt_res_values_katana)/
                                   sizeof(_flex_pkt_res_values_katana[0]);
   }
   if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit) || SOC_IS_KATANA2(unit)) {
       flex_pkt_res_values = _flex_pkt_res_values_tr3;
       flex_pkt_res_values_count = sizeof(_flex_pkt_res_values_tr3)/
                                   sizeof(_flex_pkt_res_values_tr3[0]);
   }
   if ( flex_pkt_res_values_count == 0) {
        FLEXCTR_WARN(("CONFIG ERROR: flex_pkt_res_values_count=0\n"));
        return 0;
   }
   if (pkt_res_field >= flex_pkt_res_values_count) {
       FLEXCTR_VVERB(("Flex Pkt Resolution Value Initialization failed"
                     "pkt_res_field=%d > flex_pkt_res_values_count=%d=0\n",
                      pkt_res_field, flex_pkt_res_values_count));
           return 0;
   }
   return flex_pkt_res_values[pkt_res_field];
}
