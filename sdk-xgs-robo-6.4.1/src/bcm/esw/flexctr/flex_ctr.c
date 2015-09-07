/*
 * $Id: flex_ctr.c,v 1.31 Broadcom SDK $
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
#include <shared/bsl.h>
#include <bcm_int/esw/flex_ctr.h>
#include <bcm_int/control.h>

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
#define BCM_STAT_FLEX_OBJECT_MSB_START_BIT     14
#define BCM_STAT_FLEX_OBJECT_MSB_END_BIT     14
#define BCM_STAT_FLEX_BASE_IDX_START_BIT  0
#define BCM_STAT_FLEX_BASE_IDX_END_BIT   13

#define BCM_STAT_FLEX_MODE_MASK \
    ((1<<(BCM_STAT_FLEX_MODE_END_BIT - BCM_STAT_FLEX_MODE_START_BIT+1))-1)
#define BCM_STAT_FLEX_GROUP_MASK \
    ((1<<(BCM_STAT_FLEX_GROUP_END_BIT - BCM_STAT_FLEX_GROUP_START_BIT+1))-1)
#define BCM_STAT_FLEX_POOL_MASK \
    ((1<<(BCM_STAT_FLEX_POOL_END_BIT - BCM_STAT_FLEX_POOL_START_BIT+1))-1)
#define BCM_STAT_FLEX_OBJECT_MASK \
    ((1<<(BCM_STAT_FLEX_OBJECT_END_BIT - BCM_STAT_FLEX_OBJECT_START_BIT+1))-1)
#define BCM_STAT_FLEX_OBJECT_MSB_MASK \
    ((1<<(BCM_STAT_FLEX_OBJECT_MSB_END_BIT - \
        BCM_STAT_FLEX_OBJECT_MSB_START_BIT+1))-1)
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

#define TR3_PKT_RESOLUTIONS _UNKNOWN_PKT_TR3, \
                            _CONTROL_PKT_TR3, \
                            _BPDU_PKT_TR3, \
                            _L2BC_PKT_TR3, \
                            _L2UC_PKT_TR3, \
                            _L2DLF_PKT_TR3, \
                            _UNKNOWN_IPMC_PKT_TR3, \
                            _KNOWN_IPMC_PKT_TR3,  \
                            _KNOWN_L2MC_PKT_TR3, \
                            _UNKNOWN_L2MC_PKT_TR3, \
                            _KNOWN_L3UC_PKT_TR3, \
                            _UNKNOWN_L3UC_PKT_TR3, \
                            _KNOWN_MPLS_PKT_TR3, \
                            _KNOWN_MPLS_L3_PKT_TR3, \
                            _KNOWN_MPLS_L2_PKT_TR3, \
                            _UNKNOWN_MPLS_PKT_TR3, \
                            _KNOWN_MIM_PKT_TR3, \
                            _UNKNOWN_MIM_PKT_TR3, \
                            _KNOWN_MPLS_MULTICAST_PKT_TR3, \
                            _OAM_PKT_TR3, \
                            _BFD_PKT_TR3, \
                            _ICNM_PKT_TR3, \
                            _1588_PKT_TR3, \
                            _KNOWN_TRILL_PKT_TR3, \
                            _UNKNOWN_TRILL_PKT_TR3, \
                            _KNOWN_NIV_PKT_TR3, \
                            _UNKNOWN_NIV_PKT_TR3
    
static uint32 _flex_pkt_res_values_tr3[]={TR3_PKT_RESOLUTIONS};


/* added in TD2 */
#define TD2_PKT_RESOLUTIONS     _KNOWN_L2GRE_PKT_TD2, \
                                _KNOWN_VXLAN_PKT_TD2, \
                                _KNOWN_FCOE_PKT_TD2, \
                                _UNKNOWN_FCOE_PKT_TD2

static uint32 _flex_pkt_res_values_td2[] = {TR3_PKT_RESOLUTIONS, 
                                            TD2_PKT_RESOLUTIONS};


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
#if defined(BCM_TRIDENT2_SUPPORT)
                 ,
                 {_KNOWN_L2GRE_PKT,0},
                 {_KNOWN_VXLAN_PKT,0},
                 {_KNOWN_FCOE_PKT,0},
                 {_UNKNOWN_FCOE_PKT,0},
#endif /* BCM_TRIDENT2_SUPPORT */
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
                 {_UNKNOWN_L3UC_PKT,0},
                 {_L2DLF_PKT,0}
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
bcm_stat_flex_ing_pkt_attr_bits_t ing_pkt_attr_uncmprsd_bits_g={0};
bcm_stat_flex_egr_pkt_attr_bits_t egr_pkt_attr_uncmprsd_bits_g={0};
bcm_stat_flex_ing_pkt_attr_bits_t ing_pkt_attr_cmprsd_bits_g={0};
bcm_stat_flex_egr_pkt_attr_bits_t egr_pkt_attr_cmprsd_bits_g={0};

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
   if (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit)) {
       flex_pkt_res_values = _flex_pkt_res_values_tr3;
       flex_pkt_res_values_count = sizeof(_flex_pkt_res_values_tr3)/
                                   sizeof(_flex_pkt_res_values_tr3[0]);
   }
   if (SOC_IS_TD2_TT2(unit)) {
       flex_pkt_res_values = _flex_pkt_res_values_td2;
       flex_pkt_res_values_count = sizeof(_flex_pkt_res_values_td2)/
                                   sizeof(_flex_pkt_res_values_td2[0]);
   }
   if ( flex_pkt_res_values_count == 0) {
        LOG_WARN(BSL_LS_BCM_FLEXCTR,
                 (BSL_META_U(unit,
                             "CONFIG ERROR: flex_pkt_res_values_count=0\n")));
        return ;
   }
  
   for(index=0;index<num_entries;index++) {
       /* Check Whether Index is exceeding chip specific count */
       if (flex_pkt_res_data[index].pkt_res_field >= 
           flex_pkt_res_values_count) {
           LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                     (BSL_META_U(unit,
                                 "Flex Pkt Resolution Value Initialization failed"
                                  "pkt_res_field=%d > flex_pkt_res_values_count=%d=0\n"),
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
                        (((object >> (BCM_STAT_FLEX_OBJECT_END_BIT 
                                - BCM_STAT_FLEX_OBJECT_START_BIT+1)) 
                                & BCM_STAT_FLEX_OBJECT_MSB_MASK)     << 
                                 BCM_STAT_FLEX_OBJECT_MSB_START_BIT) |                                 
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
     *object      = (bcm_stat_object_t) (((stat_counter_id >> 
                                          BCM_STAT_FLEX_OBJECT_START_BIT) &
                                         (BCM_STAT_FLEX_OBJECT_MASK))
                                         | (((stat_counter_id >> 
                                          BCM_STAT_FLEX_OBJECT_MSB_START_BIT) &
                                         BCM_STAT_FLEX_OBJECT_MSB_MASK)
                                         <<(BCM_STAT_FLEX_OBJECT_END_BIT - 
                                           BCM_STAT_FLEX_OBJECT_START_BIT+1)));
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
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Failed to allocate memory for bcm_stat_flex_attr_t ")));
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
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Overiding group_mode->bcmStatGroupModeSingle\n")));
             group_mode = bcmStatGroupModeSingle;
             break;
        case bcmStatGroupModeTyped:
        case bcmStatGroupModeTypedAll:
        case bcmStatGroupModeTrafficTypeWithControl:
        case bcmStatGroupModeDlfAllWithControl:
        case bcmStatGroupModeTypedWithControl:
        case bcmStatGroupModeTypedAllWithControl: 
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Overiding group_mode to "
                                    "bcmStatGroupModeTrafficType \n")));
             group_mode = bcmStatGroupModeTrafficType;
             break;
        case bcmStatGroupModeDlfIntPri: 
        case bcmStatGroupModeDlfIntPriWithControl: 
        case bcmStatGroupModeTypedIntPriWithControl:
             LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "Overiding group_mode to "
                                    "bcmStatGroupModeTypedIntPri \n")));
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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "bcmStatGroupModeDlfAll is not supported"
                                    "in egress side\n")));
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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "bcmStatGroupModeDlfIntPri IsNotAvailable"
                                    "in EgressSide\n")));
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
         
         ing_cmprsd_pkt_attr_bits->pkt_resolution_pos = ing_pkt_attr_cmprsd_bits_g.
                                                        pkt_resolution_pos;
#if 0
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->pkt_resolution_pos -= 2;
         }
#endif
         ing_cmprsd_pkt_attr_bits->pkt_resolution_mask = 1;

         ing_cmprsd_pkt_attr_bits->int_pri = ing_pkt_attr_cmprsd_bits_g.int_pri;
         ing_cmprsd_pkt_attr_bits->int_pri_pos = ing_pkt_attr_cmprsd_bits_g.
                                                 int_pri_pos;
#if 0
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->int_pri_pos -= 2;
         }
#endif
         ing_cmprsd_pkt_attr_bits->int_pri_mask = ing_pkt_attr_cmprsd_bits_g.
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
         shift_by_bits= ing_pkt_attr_cmprsd_bits_g.svp_type + ing_pkt_attr_cmprsd_bits_g.drop;
         shift_by_bits_for_value = shift_by_bits;
#if 0
         if (SOC_IS_KATANA2(unit)) {
             shift_by_bits_for_value -= 2;
         }
#endif

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
         /* 1) UNKNOWN_L3UC_PKT|_L2DLF_PKT                                     */
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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "bcmStatGroupModeTyped: is not supported"
                                    " in egress side\n")));
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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "bcmStatGroupModeTypedAll is NotSupported"
                                    " in EgressSide\n")));
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
             ing_cmprsd_pkt_attr_bits->pkt_resolution_pos = ing_pkt_attr_cmprsd_bits_g.
                                                            pkt_resolution_pos;
#if 0
             if (SOC_IS_KATANA2(unit)) {
                 ing_cmprsd_pkt_attr_bits->pkt_resolution_pos -= 2;
             }
#endif
             ing_cmprsd_pkt_attr_bits->pkt_resolution_mask = (1<<3)-1;

             ing_cmprsd_pkt_attr_bits->int_pri = ing_pkt_attr_cmprsd_bits_g.int_pri;
             ing_cmprsd_pkt_attr_bits->int_pri_pos = ing_pkt_attr_cmprsd_bits_g.
                                                     int_pri_pos;
#if 0
             if (SOC_IS_KATANA2(unit)) {
                 ing_cmprsd_pkt_attr_bits->int_pri_pos -= 2;
             }
#endif
             ing_cmprsd_pkt_attr_bits->int_pri_mask= ing_pkt_attr_cmprsd_bits_g.
                                                     int_pri_mask;                                                    

             ing_cmprsd_attr_selectors->total_counters = 20;
             /* Reset pkt_resolution map */
             for (map_index=0; map_index < 256 ;map_index++) {
                  ing_cmprsd_attr_selectors->pkt_res_attr_map[map_index]=0;
             }       
             /* set pkt_resolution map for  1 counters */
             /* Map[Encoded Packet value << 2=>[SVP-1bit + DROP-1bit]=
                CounterIndex << 2=>[SVP-1bit + DROP-1bit]=4(Start),8,12*/
             shift_by_bits= ing_pkt_attr_cmprsd_bits_g.svp_type + 
                            ing_pkt_attr_cmprsd_bits_g.drop;
             shift_by_bits_for_value = shift_by_bits;
#if 0
             if (SOC_IS_KATANA2(unit)) {
                 shift_by_bits_for_value -= 2;
             }
#endif

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

            egr_cmprsd_pkt_attr_bits->pkt_resolution = egr_pkt_attr_cmprsd_bits_g.
                                                       pkt_resolution;
            egr_cmprsd_pkt_attr_bits->pkt_resolution_pos = egr_pkt_attr_cmprsd_bits_g.
                                                           pkt_resolution_pos;
#if 0
            if (SOC_IS_KATANA2(unit)) {
                 egr_cmprsd_pkt_attr_bits->pkt_resolution_pos -= 4;
            }
#endif
            egr_cmprsd_pkt_attr_bits->pkt_resolution_mask = egr_pkt_attr_cmprsd_bits_g.
                                                            pkt_resolution_mask;

            egr_cmprsd_pkt_attr_bits->int_pri = egr_pkt_attr_cmprsd_bits_g.int_pri;
            egr_cmprsd_pkt_attr_bits->int_pri_pos = egr_pkt_attr_cmprsd_bits_g.
                                                    int_pri_pos;
#if 0
            if (SOC_IS_KATANA2(unit)) {
                 egr_cmprsd_pkt_attr_bits->int_pri_pos -= 4;
            }
#endif
            egr_cmprsd_pkt_attr_bits->int_pri_mask=egr_pkt_attr_cmprsd_bits_g.int_pri_mask;

            egr_cmprsd_attr_selectors->total_counters = 18;

            shift_by_bits= egr_pkt_attr_cmprsd_bits_g.svp_type + 
                           egr_pkt_attr_cmprsd_bits_g.dvp_type + 
                           egr_pkt_attr_cmprsd_bits_g.drop;
             shift_by_bits_for_value = shift_by_bits;
#if 0
             if (SOC_IS_KATANA2(unit)) {
                 shift_by_bits_for_value -= 4;
             }
#endif
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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "bcmStatGroupModeSingleWithControl"
                                    " is not supported in egress side\n")));
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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "bcmStatGroupModeTrafficTypeWithControl"
                                    "is not supported in egress side\n")));
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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "bcmStatGroupModeDlfAllWithControl is not supported "
                                    "in egress side\n")));
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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "GroupModeDlfIntPriWithControl is not available in "
                                    "egress side\n")));
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
         ing_cmprsd_pkt_attr_bits->pkt_resolution_pos = ing_pkt_attr_cmprsd_bits_g.
                                                        pkt_resolution_pos;
#if 0
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->pkt_resolution_pos -= 2;
         }
#endif
         ing_cmprsd_pkt_attr_bits->pkt_resolution_mask = (1<<2)-1;

         ing_cmprsd_pkt_attr_bits->int_pri = ing_pkt_attr_cmprsd_bits_g.int_pri;
         ing_cmprsd_pkt_attr_bits->int_pri_pos = ing_pkt_attr_cmprsd_bits_g.
                                                 int_pri_pos;
#if 0
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->int_pri_pos -= 2;
         }
#endif
         ing_cmprsd_pkt_attr_bits->int_pri_mask = ing_pkt_attr_cmprsd_bits_g.
                                                  int_pri_mask;
         ing_cmprsd_attr_selectors->total_counters = 18;

         /* Reset pkt_resolution map */
         for (map_index=0; map_index < 256 ;map_index++) {
              ing_cmprsd_attr_selectors->pkt_res_attr_map[map_index]=0;
         }
         /* set pkt_resolution map for  1 counters */
         /* Map[Encoded Packet value << 2=>[SVP-1bit + DROP-1bit]=
            CounterIndex << 2=>[SVP-1bit + DROP-1bit]=4(Start),8,12*/

         shift_by_bits= ing_pkt_attr_cmprsd_bits_g.svp_type + ing_pkt_attr_cmprsd_bits_g.drop;
         shift_by_bits_for_value = shift_by_bits;
#if 0
         if (SOC_IS_KATANA2(unit)) {
             shift_by_bits_for_value -= 2;
         }
#endif

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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "bcmStatGroupModeTypedWithControl" 
                                    "is not supported in egress side\n")));
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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "bcmStatGroupModeTypedAllWithControl"
                                    " is not supported in egress side\n")));
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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "GroupModeTypedIntPriWithControl is not available in "
                                    "egress side\n")));
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
         ing_cmprsd_pkt_attr_bits->pkt_resolution_pos = ing_pkt_attr_cmprsd_bits_g.
                                                        pkt_resolution_pos;
#if 0
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->pkt_resolution_pos -= 2;
         }
#endif
         ing_cmprsd_pkt_attr_bits->pkt_resolution_mask = (1<<3)-1;
         ing_cmprsd_pkt_attr_bits->int_pri = ing_pkt_attr_cmprsd_bits_g.int_pri;
         ing_cmprsd_pkt_attr_bits->int_pri_pos = ing_pkt_attr_cmprsd_bits_g.
                                                 int_pri_pos;
#if 0
         if (SOC_IS_KATANA2(unit)) {
             ing_cmprsd_pkt_attr_bits->int_pri_pos -= 2;
         }
#endif
         ing_cmprsd_pkt_attr_bits->int_pri_mask = ing_pkt_attr_cmprsd_bits_g.int_pri_mask;
         ing_cmprsd_attr_selectors->total_counters = 21;
            
         /* Reset pkt_resolution map */
         for (map_index=0; map_index < 256 ;map_index++) {
              ing_cmprsd_attr_selectors->pkt_res_attr_map[map_index]=0;
         }       
         /* set pkt_resolution map for  1 counters */
         /* set pkt_resolution map for  1 counters */
         /* Map[Encoded Packet value << 2=>[SVP-1bit + DROP-1bit]=
            CounterIndex << 2=>[SVP-1bit + DROP-1bit]=4(Start),8,12*/

         shift_by_bits= ing_pkt_attr_cmprsd_bits_g.svp_type + ing_pkt_attr_cmprsd_bits_g.drop;
         shift_by_bits_for_value = shift_by_bits;
#if 0
         if (SOC_IS_KATANA2(unit)) {
             shift_by_bits_for_value -= 2;
         }
#endif

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
                        offset_table_map[counter_index].offset=counter_index;
                  ing_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index].count_enable=1;
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
                 BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS,
                 64);
             /* Set Offset table fields */
             for (counter_index=0;counter_index<64;counter_index++) {
                  ing_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index].offset=counter_index;
                  ing_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index].count_enable=1;
             }
         } else {
             total_counters=64;
             _bcm_esw_fillup_egr_uncmp_attr(
                 egr_attr,
                 BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS,
                 64);
             /* Set Offset table fields */
             for (counter_index=0;counter_index<2;counter_index++) {
                  egr_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index].offset=counter_index;
                  egr_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index].count_enable=1;
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
             LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                       (BSL_META_U(unit,
                                   "bcm_stat_group_mode_t %d is NotSupported"
                                    " in IngressSide\n"), group_mode));
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
                        offset_table_map[counter_index].offset=counter_index;
                  egr_attr->uncmprsd_attr_selectors.
                        offset_table_map[counter_index].count_enable=1;
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
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, EXT_FP_POLICYm, object, mode,
                        &base_index, &pool_number);
              break;
         case bcmStatObjectIngVxlan:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, SOURCE_VPm,object,mode,
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
         case bcmStatObjectIngIpmc:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, L3_ENTRY_IPV4_MULTICASTm,object,mode,
                        &base_index,&pool_number);
              break; 
         case bcmStatObjectIngVxlanDip:
              rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                        unit, VLAN_XLATEm, object,mode,
                        &base_index, &pool_number);
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
         case bcmStatObjectMaxValue:
              rv = BCM_E_PARAM;
              break;
         }
    }
    /* Cleanup activity ... */
    sal_free(attr);
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "creation of counters passed  failed..\n")));
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
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Create: mode:%d group_mode:%d pool:%d object:%d"
                               " base:%d\n stat_counter_id:%d\n"),
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
    LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
              (BSL_META_U(unit,
                          "Deleting : mode:%d group_mode:%d pool:%d object:%d"
                           "base:%d\n stat_counter_id:%d\n"),offset_mode,group_mode,
               pool_number,object,base_index,stat_counter_id));

    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_object(unit,object,&direction));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_group(unit,group_mode));

    if (offset_mode > (BCM_STAT_FLEX_COUNTER_MAX_MODE-1)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Invalid flex counter mode value %d \n"),
                   offset_mode));
        return BCM_E_PARAM;
    }
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
                        unit, EXT_FP_POLICYm, object, offset_mode,
                        base_index, pool_number);
              break;
         case bcmStatObjectIngVxlan:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, SOURCE_VPm, object, offset_mode,
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
         case bcmStatObjectIngIpmc:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, L3_ENTRY_IPV4_MULTICASTm,object,offset_mode,
                        base_index,pool_number);
              break;
         case bcmStatObjectIngVxlanDip:
              rv = _bcm_esw_stat_flex_destroy_ingress_table_counters(
                        unit, VLAN_XLATEm, object, offset_mode,
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
         case bcmStatObjectMaxValue:
              rv = BCM_E_PARAM;
              break;

    }
    if (BCM_SUCCESS(rv)) {
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Destroyed egress table counters.."
                               "Trying to delete group mode itself \n")));
        /* No decision on return values as actual call is successful */
        if (direction == bcmStatFlexDirectionIngress) {
            if (_bcm_esw_stat_flex_delete_ingress_mode(
                unit,offset_mode) == BCM_E_NONE) {
                LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "Destroyed Ingress Mode also \n")));
                _bcm_esw_stat_flex_reset_group_mode(
                                    unit,bcmStatFlexDirectionIngress,
                                    offset_mode,group_mode);
            }
       } else {
           if (_bcm_esw_stat_flex_delete_egress_mode(
               unit,offset_mode) == BCM_E_NONE) {
                LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "Destroyed Egress Mode also \n")));
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
       egr_pkt_attr_uncmprsd_bits_g.cng = 2;
       egr_pkt_attr_uncmprsd_bits_g.cng_pos = 32;
       egr_pkt_attr_uncmprsd_bits_g.cng_mask = (1 << egr_pkt_attr_uncmprsd_bits_g.cng) - 1;

       egr_pkt_attr_uncmprsd_bits_g.int_pri = 4;
       egr_pkt_attr_uncmprsd_bits_g.int_pri_pos = 28;
       egr_pkt_attr_uncmprsd_bits_g.int_pri_mask = 
                           (1 << egr_pkt_attr_uncmprsd_bits_g.int_pri) - 1;

       egr_pkt_attr_uncmprsd_bits_g.vlan_format = 2;
       egr_pkt_attr_uncmprsd_bits_g.vlan_format_pos = 26;
       egr_pkt_attr_uncmprsd_bits_g.vlan_format_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.vlan_format) - 1;

       egr_pkt_attr_uncmprsd_bits_g.outer_dot1p = 3;
       egr_pkt_attr_uncmprsd_bits_g.outer_dot1p_pos = 23;
       egr_pkt_attr_uncmprsd_bits_g.outer_dot1p_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.outer_dot1p) - 1;

       egr_pkt_attr_uncmprsd_bits_g.inner_dot1p = 3;
       egr_pkt_attr_uncmprsd_bits_g.inner_dot1p_pos = 20;
       egr_pkt_attr_uncmprsd_bits_g.inner_dot1p_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.inner_dot1p) - 1;

       egr_pkt_attr_uncmprsd_bits_g.egr_port = 7; /* For KT2,TR3 6 bits */
       egr_pkt_attr_uncmprsd_bits_g.egr_port_pos = 13;
       egr_pkt_attr_uncmprsd_bits_g.egr_port_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.egr_port) - 1;

       egr_pkt_attr_uncmprsd_bits_g.tos_dscp = 6;
       egr_pkt_attr_uncmprsd_bits_g.tos_dscp_pos = 7;
       egr_pkt_attr_uncmprsd_bits_g.tos_dscp_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.tos_dscp) - 1;
       egr_pkt_attr_uncmprsd_bits_g.tos_ecn = 2;
       egr_pkt_attr_uncmprsd_bits_g.tos_ecn_pos = 5;
       egr_pkt_attr_uncmprsd_bits_g.tos_ecn_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.tos_ecn) - 1;

       egr_pkt_attr_uncmprsd_bits_g.pkt_resolution = 1;
       egr_pkt_attr_uncmprsd_bits_g.pkt_resolution_pos = 4;
       egr_pkt_attr_uncmprsd_bits_g.pkt_resolution_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.pkt_resolution) - 1;

       egr_pkt_attr_uncmprsd_bits_g.svp_type = 1;
       egr_pkt_attr_uncmprsd_bits_g.svp_type_pos = 3;
       egr_pkt_attr_uncmprsd_bits_g.svp_type_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.svp_type) - 1;

       egr_pkt_attr_uncmprsd_bits_g.dvp_type = 1;
       egr_pkt_attr_uncmprsd_bits_g.dvp_type_pos = 2;
       egr_pkt_attr_uncmprsd_bits_g.dvp_type_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.dvp_type) - 1;

       egr_pkt_attr_uncmprsd_bits_g.drop = 1;
       egr_pkt_attr_uncmprsd_bits_g.drop_pos = 1;
       egr_pkt_attr_uncmprsd_bits_g.drop_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.drop) - 1;

       egr_pkt_attr_uncmprsd_bits_g.ip_pkt = 1;
       egr_pkt_attr_uncmprsd_bits_g.ip_pkt_pos = 0;
       egr_pkt_attr_uncmprsd_bits_g.ip_pkt_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.ip_pkt) - 1;

       /* Ingress bits initialization */
       ing_pkt_attr_uncmprsd_bits_g.cng = 2;
       ing_pkt_attr_uncmprsd_bits_g.cng_pos = 38;
       ing_pkt_attr_uncmprsd_bits_g.cng_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.cng) - 1;

       ing_pkt_attr_uncmprsd_bits_g.ifp_cng = 2;
       ing_pkt_attr_uncmprsd_bits_g.ifp_cng_pos = 36;
       ing_pkt_attr_uncmprsd_bits_g.ifp_cng_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.ifp_cng) - 1;

       ing_pkt_attr_uncmprsd_bits_g.int_pri = 4;
       ing_pkt_attr_uncmprsd_bits_g.int_pri_pos = 32;
       ing_pkt_attr_uncmprsd_bits_g.int_pri_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.int_pri) - 1;

       ing_pkt_attr_uncmprsd_bits_g.vlan_format = 2;
       ing_pkt_attr_uncmprsd_bits_g.vlan_format_pos = 30;
       ing_pkt_attr_uncmprsd_bits_g.vlan_format_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.vlan_format) - 1;

       ing_pkt_attr_uncmprsd_bits_g.outer_dot1p = 3;
       ing_pkt_attr_uncmprsd_bits_g.outer_dot1p_pos = 27;
       ing_pkt_attr_uncmprsd_bits_g.outer_dot1p_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.outer_dot1p) - 1;

       ing_pkt_attr_uncmprsd_bits_g.inner_dot1p = 3;
       ing_pkt_attr_uncmprsd_bits_g.inner_dot1p_pos = 24;
       ing_pkt_attr_uncmprsd_bits_g.inner_dot1p_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.inner_dot1p) - 1;

       ing_pkt_attr_uncmprsd_bits_g.ing_port = 7; /* FOR TD2, 7 bits */
       ing_pkt_attr_uncmprsd_bits_g.ing_port_pos = 17;
       ing_pkt_attr_uncmprsd_bits_g.ing_port_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.ing_port) - 1;

       ing_pkt_attr_uncmprsd_bits_g.tos_dscp = 6;
       ing_pkt_attr_uncmprsd_bits_g.tos_dscp_pos = 11;
       ing_pkt_attr_uncmprsd_bits_g.tos_dscp_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.tos_dscp) - 1;

       ing_pkt_attr_uncmprsd_bits_g.tos_ecn = 2;
       ing_pkt_attr_uncmprsd_bits_g.tos_ecn_pos = 9;
       ing_pkt_attr_uncmprsd_bits_g.tos_ecn_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.tos_ecn) - 1;

       ing_pkt_attr_uncmprsd_bits_g.pkt_resolution = 6;
       ing_pkt_attr_uncmprsd_bits_g.pkt_resolution_pos = 3;
       ing_pkt_attr_uncmprsd_bits_g.pkt_resolution_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.pkt_resolution) - 1;

       ing_pkt_attr_uncmprsd_bits_g.svp_type = 1;
       ing_pkt_attr_uncmprsd_bits_g.svp_type_pos = 2;
       ing_pkt_attr_uncmprsd_bits_g.svp_type_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.svp_type) - 1;

       ing_pkt_attr_uncmprsd_bits_g.drop = 1;
       ing_pkt_attr_uncmprsd_bits_g.drop_pos = 1;
       ing_pkt_attr_uncmprsd_bits_g.drop_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.drop_mask) - 1;

       ing_pkt_attr_uncmprsd_bits_g.ip_pkt = 1;
       ing_pkt_attr_uncmprsd_bits_g.ip_pkt_pos = 0;
       ing_pkt_attr_uncmprsd_bits_g.ip_pkt_mask = 
                           (1<<ing_pkt_attr_uncmprsd_bits_g.ip_pkt) - 1;
   } else { 
       /* Egress bits initialization */
       if (SOC_IS_KATANA2(unit)) {
           egr_pkt_attr_uncmprsd_bits_g.cng = 2;
           egr_pkt_attr_uncmprsd_bits_g.cng_pos = 37;
           egr_pkt_attr_uncmprsd_bits_g.cng_mask = (1 << egr_pkt_attr_uncmprsd_bits_g.cng) - 1;

           egr_pkt_attr_uncmprsd_bits_g.int_pri = 4;
           egr_pkt_attr_uncmprsd_bits_g.int_pri_pos = 33;
           egr_pkt_attr_uncmprsd_bits_g.int_pri_mask = 
                               (1 << egr_pkt_attr_uncmprsd_bits_g.int_pri) - 1;

           egr_pkt_attr_uncmprsd_bits_g.vlan_format = 2;
           egr_pkt_attr_uncmprsd_bits_g.vlan_format_pos = 31;
           egr_pkt_attr_uncmprsd_bits_g.vlan_format_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.vlan_format) - 1;

           egr_pkt_attr_uncmprsd_bits_g.outer_dot1p = 3;
           egr_pkt_attr_uncmprsd_bits_g.outer_dot1p_pos = 28;
           egr_pkt_attr_uncmprsd_bits_g.outer_dot1p_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.outer_dot1p) - 1;

           egr_pkt_attr_uncmprsd_bits_g.inner_dot1p = 3;
           egr_pkt_attr_uncmprsd_bits_g.inner_dot1p_pos = 25;
           egr_pkt_attr_uncmprsd_bits_g.inner_dot1p_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.inner_dot1p) - 1;

           egr_pkt_attr_uncmprsd_bits_g.egr_port = 8; /* For KT2 : 8 bits */
           egr_pkt_attr_uncmprsd_bits_g.egr_port_pos = 17;
           egr_pkt_attr_uncmprsd_bits_g.egr_port_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.egr_port) - 1;

           egr_pkt_attr_uncmprsd_bits_g.tos_dscp = 6;
           egr_pkt_attr_uncmprsd_bits_g.tos_dscp_pos = 11;
           egr_pkt_attr_uncmprsd_bits_g.tos_dscp_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.tos_dscp) - 1;

           egr_pkt_attr_uncmprsd_bits_g.tos_ecn = 2;
           egr_pkt_attr_uncmprsd_bits_g.tos_ecn_pos = 9;
           egr_pkt_attr_uncmprsd_bits_g.tos_ecn_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.tos_ecn) - 1;

           egr_pkt_attr_uncmprsd_bits_g.pkt_resolution = 1;
           egr_pkt_attr_uncmprsd_bits_g.pkt_resolution_pos = 8;
           egr_pkt_attr_uncmprsd_bits_g.pkt_resolution_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.pkt_resolution) - 1;

           egr_pkt_attr_uncmprsd_bits_g.svp_type = 3;
           egr_pkt_attr_uncmprsd_bits_g.svp_type_pos = 5;
           egr_pkt_attr_uncmprsd_bits_g.svp_type_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.svp_type) - 1;

           egr_pkt_attr_uncmprsd_bits_g.dvp_type = 3;
           egr_pkt_attr_uncmprsd_bits_g.dvp_type_pos = 2;
           egr_pkt_attr_uncmprsd_bits_g.dvp_type_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.dvp_type) - 1;
       } else {
           egr_pkt_attr_uncmprsd_bits_g.cng = 2;
           egr_pkt_attr_uncmprsd_bits_g.cng_pos = 31;
           egr_pkt_attr_uncmprsd_bits_g.cng_mask = (1 << egr_pkt_attr_uncmprsd_bits_g.cng) - 1;

           egr_pkt_attr_uncmprsd_bits_g.int_pri = 4;
           egr_pkt_attr_uncmprsd_bits_g.int_pri_pos = 27;
           egr_pkt_attr_uncmprsd_bits_g.int_pri_mask = 
                               (1 << egr_pkt_attr_uncmprsd_bits_g.int_pri) - 1;

           egr_pkt_attr_uncmprsd_bits_g.vlan_format = 2;
           egr_pkt_attr_uncmprsd_bits_g.vlan_format_pos = 25;
           egr_pkt_attr_uncmprsd_bits_g.vlan_format_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.vlan_format) - 1;

           egr_pkt_attr_uncmprsd_bits_g.outer_dot1p = 3;
           egr_pkt_attr_uncmprsd_bits_g.outer_dot1p_pos = 22;
           egr_pkt_attr_uncmprsd_bits_g.outer_dot1p_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.outer_dot1p) - 1;

           egr_pkt_attr_uncmprsd_bits_g.inner_dot1p = 3;
           egr_pkt_attr_uncmprsd_bits_g.inner_dot1p_pos = 19;
           egr_pkt_attr_uncmprsd_bits_g.inner_dot1p_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.inner_dot1p) - 1;

           egr_pkt_attr_uncmprsd_bits_g.egr_port = 6; /* For KT2,TR3 6 bits */
           egr_pkt_attr_uncmprsd_bits_g.egr_port_pos = 13;
           egr_pkt_attr_uncmprsd_bits_g.egr_port_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.egr_port) - 1;

           egr_pkt_attr_uncmprsd_bits_g.tos_dscp = 6;
           egr_pkt_attr_uncmprsd_bits_g.tos_dscp_pos = 7;
           egr_pkt_attr_uncmprsd_bits_g.tos_dscp_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.tos_dscp) - 1;

           egr_pkt_attr_uncmprsd_bits_g.tos_ecn = 2;
           egr_pkt_attr_uncmprsd_bits_g.tos_ecn_pos = 5;
           egr_pkt_attr_uncmprsd_bits_g.tos_ecn_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.tos_ecn) - 1;

           egr_pkt_attr_uncmprsd_bits_g.pkt_resolution = 1;
           egr_pkt_attr_uncmprsd_bits_g.pkt_resolution_pos = 4;
           egr_pkt_attr_uncmprsd_bits_g.pkt_resolution_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.pkt_resolution) - 1;
    
           egr_pkt_attr_uncmprsd_bits_g.svp_type = 1;
           egr_pkt_attr_uncmprsd_bits_g.svp_type_pos = 3;
           egr_pkt_attr_uncmprsd_bits_g.svp_type_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.svp_type) - 1;

           egr_pkt_attr_uncmprsd_bits_g.dvp_type = 1;
           egr_pkt_attr_uncmprsd_bits_g.dvp_type_pos = 2;
           egr_pkt_attr_uncmprsd_bits_g.dvp_type_mask = 
                               (1<<egr_pkt_attr_uncmprsd_bits_g.dvp_type) - 1;
       }
       egr_pkt_attr_uncmprsd_bits_g.drop = 1;
       egr_pkt_attr_uncmprsd_bits_g.drop_pos = 1;
       egr_pkt_attr_uncmprsd_bits_g.drop_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.drop) - 1;

       egr_pkt_attr_uncmprsd_bits_g.ip_pkt = 1;
       egr_pkt_attr_uncmprsd_bits_g.ip_pkt_pos = 0;
       egr_pkt_attr_uncmprsd_bits_g.ip_pkt_mask = 
                           (1<<egr_pkt_attr_uncmprsd_bits_g.ip_pkt) - 1;

       /* Ingress bits initialization */
       if (SOC_IS_KATANA2(unit)) {
           ing_pkt_attr_uncmprsd_bits_g.cng = 2;
           ing_pkt_attr_uncmprsd_bits_g.cng_pos = 41;
           ing_pkt_attr_uncmprsd_bits_g.cng_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.cng) - 1;

           ing_pkt_attr_uncmprsd_bits_g.ifp_cng = 2;
           ing_pkt_attr_uncmprsd_bits_g.ifp_cng_pos = 39;
           ing_pkt_attr_uncmprsd_bits_g.ifp_cng_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.ifp_cng) - 1;

           ing_pkt_attr_uncmprsd_bits_g.int_pri = 4;
           ing_pkt_attr_uncmprsd_bits_g.int_pri_pos = 35;
           ing_pkt_attr_uncmprsd_bits_g.int_pri_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.int_pri) - 1;

           ing_pkt_attr_uncmprsd_bits_g.vlan_format = 2;
           ing_pkt_attr_uncmprsd_bits_g.vlan_format_pos = 33;
           ing_pkt_attr_uncmprsd_bits_g.vlan_format_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.vlan_format) - 1;

           ing_pkt_attr_uncmprsd_bits_g.outer_dot1p = 3;
           ing_pkt_attr_uncmprsd_bits_g.outer_dot1p_pos = 30;
           ing_pkt_attr_uncmprsd_bits_g.outer_dot1p_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.outer_dot1p) - 1;

           ing_pkt_attr_uncmprsd_bits_g.inner_dot1p = 3;
           ing_pkt_attr_uncmprsd_bits_g.inner_dot1p_pos = 27;
           ing_pkt_attr_uncmprsd_bits_g.inner_dot1p_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.inner_dot1p) - 1;

           ing_pkt_attr_uncmprsd_bits_g.ing_port = 8; /* For KT2,TR3 6 bits */
           ing_pkt_attr_uncmprsd_bits_g.ing_port_pos = 19;
           ing_pkt_attr_uncmprsd_bits_g.ing_port_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.ing_port) - 1;

           ing_pkt_attr_uncmprsd_bits_g.tos_dscp = 6;
           ing_pkt_attr_uncmprsd_bits_g.tos_dscp_pos = 13;
           ing_pkt_attr_uncmprsd_bits_g.tos_dscp_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.tos_dscp) - 1;

           ing_pkt_attr_uncmprsd_bits_g.tos_ecn = 2;
           ing_pkt_attr_uncmprsd_bits_g.tos_ecn_pos = 11;
           ing_pkt_attr_uncmprsd_bits_g.tos_ecn_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.tos_ecn) - 1;

           ing_pkt_attr_uncmprsd_bits_g.pkt_resolution = 6;
           ing_pkt_attr_uncmprsd_bits_g.pkt_resolution_pos = 5;
           ing_pkt_attr_uncmprsd_bits_g.pkt_resolution_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.pkt_resolution) - 1;

           ing_pkt_attr_uncmprsd_bits_g.svp_type = 3;
           ing_pkt_attr_uncmprsd_bits_g.svp_type_pos = 2;
           ing_pkt_attr_uncmprsd_bits_g.svp_type_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.svp_type) - 1;
       } else {
           ing_pkt_attr_uncmprsd_bits_g.cng = 2;
           ing_pkt_attr_uncmprsd_bits_g.cng_pos = 37;
           ing_pkt_attr_uncmprsd_bits_g.cng_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.cng) - 1;

           ing_pkt_attr_uncmprsd_bits_g.ifp_cng = 2;
           ing_pkt_attr_uncmprsd_bits_g.ifp_cng_pos = 35;
           ing_pkt_attr_uncmprsd_bits_g.ifp_cng_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.ifp_cng) - 1;

           ing_pkt_attr_uncmprsd_bits_g.int_pri = 4;
           ing_pkt_attr_uncmprsd_bits_g.int_pri_pos = 31;
           ing_pkt_attr_uncmprsd_bits_g.int_pri_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.int_pri) - 1;

           ing_pkt_attr_uncmprsd_bits_g.vlan_format = 2;
           ing_pkt_attr_uncmprsd_bits_g.vlan_format_pos = 29;
           ing_pkt_attr_uncmprsd_bits_g.vlan_format_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.vlan_format) - 1;

           ing_pkt_attr_uncmprsd_bits_g.outer_dot1p = 3;
           ing_pkt_attr_uncmprsd_bits_g.outer_dot1p_pos = 26;
           ing_pkt_attr_uncmprsd_bits_g.outer_dot1p_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.outer_dot1p) - 1;

           ing_pkt_attr_uncmprsd_bits_g.inner_dot1p = 3;
           ing_pkt_attr_uncmprsd_bits_g.inner_dot1p_pos = 23;
           ing_pkt_attr_uncmprsd_bits_g.inner_dot1p_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.inner_dot1p) - 1;

           ing_pkt_attr_uncmprsd_bits_g.ing_port = 6; /* For KT2,TR3 6 bits */
           ing_pkt_attr_uncmprsd_bits_g.ing_port_pos = 17;
           ing_pkt_attr_uncmprsd_bits_g.ing_port_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.ing_port) - 1;

           ing_pkt_attr_uncmprsd_bits_g.tos_dscp = 6;
           ing_pkt_attr_uncmprsd_bits_g.tos_dscp_pos = 11;
           ing_pkt_attr_uncmprsd_bits_g.tos_dscp_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.tos_dscp) - 1;
           ing_pkt_attr_uncmprsd_bits_g.tos_ecn = 2;
           ing_pkt_attr_uncmprsd_bits_g.tos_ecn_pos = 9;
           ing_pkt_attr_uncmprsd_bits_g.tos_ecn_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.tos_ecn) - 1;

           ing_pkt_attr_uncmprsd_bits_g.pkt_resolution = 6;
           ing_pkt_attr_uncmprsd_bits_g.pkt_resolution_pos = 3;
           ing_pkt_attr_uncmprsd_bits_g.pkt_resolution_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.pkt_resolution) - 1;

           ing_pkt_attr_uncmprsd_bits_g.svp_type = 1;
           ing_pkt_attr_uncmprsd_bits_g.svp_type_pos = 2;
           ing_pkt_attr_uncmprsd_bits_g.svp_type_mask =
                               (1<<ing_pkt_attr_uncmprsd_bits_g.svp_type) - 1;
       }
       ing_pkt_attr_uncmprsd_bits_g.drop = 1;
       ing_pkt_attr_uncmprsd_bits_g.drop_pos = 1;
       ing_pkt_attr_uncmprsd_bits_g.drop_mask =
                           (1<<ing_pkt_attr_uncmprsd_bits_g.drop) - 1;

       ing_pkt_attr_uncmprsd_bits_g.ip_pkt = 1;
       ing_pkt_attr_uncmprsd_bits_g.ip_pkt_pos = 0;
       ing_pkt_attr_uncmprsd_bits_g.ip_pkt_mask =
                           (1<<ing_pkt_attr_uncmprsd_bits_g.ip_pkt) - 1;
   }
   if (SOC_IS_KATANA2(unit)) {
       ing_pkt_attr_cmprsd_bits_g.cng = 2;
       ing_pkt_attr_cmprsd_bits_g.cng_pos = 39;
       ing_pkt_attr_cmprsd_bits_g.cng_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.cng) - 1;

       ing_pkt_attr_cmprsd_bits_g.ifp_cng = 2;
       ing_pkt_attr_cmprsd_bits_g.ifp_cng_pos = 37;
       ing_pkt_attr_cmprsd_bits_g.ifp_cng_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.ifp_cng) - 1;

       ing_pkt_attr_cmprsd_bits_g.int_pri = 4;
       ing_pkt_attr_cmprsd_bits_g.int_pri_pos = 33;
       ing_pkt_attr_cmprsd_bits_g.int_pri_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.int_pri) - 1;

       ing_pkt_attr_cmprsd_bits_g.vlan_format = 2;
       ing_pkt_attr_cmprsd_bits_g.vlan_format_pos = 31;
       ing_pkt_attr_cmprsd_bits_g.vlan_format_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.vlan_format) - 1;

       ing_pkt_attr_cmprsd_bits_g.outer_dot1p = 3;
       ing_pkt_attr_cmprsd_bits_g.outer_dot1p_pos = 28;
       ing_pkt_attr_cmprsd_bits_g.outer_dot1p_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.outer_dot1p) - 1;

       ing_pkt_attr_cmprsd_bits_g.inner_dot1p = 3;
       ing_pkt_attr_cmprsd_bits_g.inner_dot1p_pos = 25;
       ing_pkt_attr_cmprsd_bits_g.inner_dot1p_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.inner_dot1p) - 1;

       ing_pkt_attr_cmprsd_bits_g.ing_port = 8; /* For KT2,TR3 6 bits */
       ing_pkt_attr_cmprsd_bits_g.ing_port_pos = 17;
       ing_pkt_attr_cmprsd_bits_g.ing_port_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.ing_port) - 1;

       ing_pkt_attr_cmprsd_bits_g.tos_dscp = 6;
       ing_pkt_attr_cmprsd_bits_g.tos_dscp_pos = 11;
       ing_pkt_attr_cmprsd_bits_g.tos_dscp_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.tos_dscp) - 1;

       ing_pkt_attr_cmprsd_bits_g.tos_ecn = 2;
       ing_pkt_attr_cmprsd_bits_g.tos_ecn_pos = 9;
       ing_pkt_attr_cmprsd_bits_g.tos_ecn_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.tos_ecn) - 1;

       ing_pkt_attr_cmprsd_bits_g.pkt_resolution = 4;
       ing_pkt_attr_cmprsd_bits_g.pkt_resolution_pos = 5;
       ing_pkt_attr_cmprsd_bits_g.pkt_resolution_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.pkt_resolution) - 1;

       ing_pkt_attr_cmprsd_bits_g.svp_type = 3;
       ing_pkt_attr_cmprsd_bits_g.svp_type_pos = 2;
       ing_pkt_attr_cmprsd_bits_g.svp_type_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.svp_type) - 1;
       ing_pkt_attr_cmprsd_bits_g.drop = 1;
       ing_pkt_attr_cmprsd_bits_g.drop_pos = 1;
       ing_pkt_attr_cmprsd_bits_g.drop_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.drop) - 1;

       ing_pkt_attr_cmprsd_bits_g.ip_pkt = 1;
       ing_pkt_attr_cmprsd_bits_g.ip_pkt_pos = 0;
       ing_pkt_attr_cmprsd_bits_g.ip_pkt_mask =
                           (1<<ing_pkt_attr_cmprsd_bits_g.ip_pkt) - 1;

       egr_pkt_attr_cmprsd_bits_g.cng = 2;
       egr_pkt_attr_cmprsd_bits_g.cng_pos = 33;
       egr_pkt_attr_cmprsd_bits_g.cng_mask = (1 << egr_pkt_attr_cmprsd_bits_g.cng) - 1;

       egr_pkt_attr_cmprsd_bits_g.int_pri = 4;
       egr_pkt_attr_cmprsd_bits_g.int_pri_pos = 29;
       egr_pkt_attr_cmprsd_bits_g.int_pri_mask = 
                           (1 << egr_pkt_attr_cmprsd_bits_g.int_pri) - 1;

       egr_pkt_attr_cmprsd_bits_g.vlan_format = 2;
       egr_pkt_attr_cmprsd_bits_g.vlan_format_pos = 27;
       egr_pkt_attr_cmprsd_bits_g.vlan_format_mask = 
                           (1<<egr_pkt_attr_cmprsd_bits_g.vlan_format) - 1;

       egr_pkt_attr_cmprsd_bits_g.outer_dot1p = 3;
       egr_pkt_attr_cmprsd_bits_g.outer_dot1p_pos = 24;
       egr_pkt_attr_cmprsd_bits_g.outer_dot1p_mask = 
                           (1<<egr_pkt_attr_cmprsd_bits_g.outer_dot1p) - 1;

       egr_pkt_attr_cmprsd_bits_g.inner_dot1p = 3;
       egr_pkt_attr_cmprsd_bits_g.inner_dot1p_pos = 21;
       egr_pkt_attr_cmprsd_bits_g.inner_dot1p_mask = 
                           (1<<egr_pkt_attr_cmprsd_bits_g.inner_dot1p) - 1;

       egr_pkt_attr_cmprsd_bits_g.egr_port = 8; /* For KT2 : 8 bits */
       egr_pkt_attr_cmprsd_bits_g.egr_port_pos = 13;
       egr_pkt_attr_cmprsd_bits_g.egr_port_mask = 
                           (1<<egr_pkt_attr_cmprsd_bits_g.egr_port) - 1;

       egr_pkt_attr_cmprsd_bits_g.tos_dscp = 6;
       egr_pkt_attr_cmprsd_bits_g.tos_dscp_pos = 7;
       egr_pkt_attr_cmprsd_bits_g.tos_dscp_mask = 
                           (1<<egr_pkt_attr_cmprsd_bits_g.tos_dscp) - 1;

       egr_pkt_attr_cmprsd_bits_g.tos_ecn = 2;
       egr_pkt_attr_cmprsd_bits_g.tos_ecn_pos = 5;
       egr_pkt_attr_cmprsd_bits_g.tos_ecn_mask = 
                           (1<<egr_pkt_attr_cmprsd_bits_g.tos_ecn) - 1;

       egr_pkt_attr_cmprsd_bits_g.pkt_resolution = 1;
       egr_pkt_attr_cmprsd_bits_g.pkt_resolution_pos = 4;
       egr_pkt_attr_cmprsd_bits_g.pkt_resolution_mask = 
                       (1<<egr_pkt_attr_cmprsd_bits_g.pkt_resolution) - 1;

       egr_pkt_attr_cmprsd_bits_g.svp_type = 1;
       egr_pkt_attr_cmprsd_bits_g.svp_type_pos = 3;
       egr_pkt_attr_cmprsd_bits_g.svp_type_mask = 
                           (1<<egr_pkt_attr_cmprsd_bits_g.svp_type) - 1;

       egr_pkt_attr_cmprsd_bits_g.dvp_type = 1;
       egr_pkt_attr_cmprsd_bits_g.dvp_type_pos = 2;
       egr_pkt_attr_cmprsd_bits_g.dvp_type_mask = 
                           (1<<egr_pkt_attr_cmprsd_bits_g.dvp_type) - 1;
       egr_pkt_attr_cmprsd_bits_g.drop = 1;
       egr_pkt_attr_cmprsd_bits_g.drop_pos = 1;
       egr_pkt_attr_cmprsd_bits_g.drop_mask = 
                           (1<<egr_pkt_attr_cmprsd_bits_g.drop) - 1;

       egr_pkt_attr_cmprsd_bits_g.ip_pkt = 1;
       egr_pkt_attr_cmprsd_bits_g.ip_pkt_pos = 0;
       egr_pkt_attr_cmprsd_bits_g.ip_pkt_mask = 
                           (1<<egr_pkt_attr_cmprsd_bits_g.ip_pkt) - 1;
   } else {
       egr_pkt_attr_cmprsd_bits_g = egr_pkt_attr_uncmprsd_bits_g;
       ing_pkt_attr_cmprsd_bits_g = ing_pkt_attr_uncmprsd_bits_g;
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
        LOG_WARN(BSL_LS_BCM_FLEXCTR,
                 (BSL_META_U(unit,
                             "CONFIG ERROR: flex_pkt_res_values_count=0\n")));
        return 0;
   }
   if (pkt_res_field >= flex_pkt_res_values_count) {
       LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                 (BSL_META_U(unit,
                             "Flex Pkt Resolution Value Initialization failed"
                              "pkt_res_field=%d > flex_pkt_res_values_count=%d=0\n"),
                  pkt_res_field, flex_pkt_res_values_count));
           return 0;
   }
   return flex_pkt_res_values[pkt_res_field];
}

/* Create Customized Stat Group mode for given Counter Attributes:UpdateMap  */
static 
void _bcm_stat_flex_update_map(
     bcm_stat_flex_direction_t                 direction,
     bcm_stat_flex_attr_t                      *attr,
     uint8                                     offset ,
     uint8                                     counter)
{
    if (direction == bcmStatFlexDirectionIngress) {
        if (attr->ing_attr.packet_attr_type == 
            bcmStatFlexPacketAttrTypeCompressed) {
            attr->ing_attr.cmprsd_attr_selectors.
              offset_table_map[offset].offset=counter;
            attr->ing_attr.cmprsd_attr_selectors.
              offset_table_map[offset].count_enable=1;
        } else {
        attr->ing_attr.uncmprsd_attr_selectors.
              offset_table_map[offset].offset=counter;
        attr->ing_attr.uncmprsd_attr_selectors.
              offset_table_map[offset].count_enable=1;
        }
     } else {
        if (attr->egr_attr.packet_attr_type == 
            bcmStatFlexPacketAttrTypeCompressed) {
            attr->egr_attr.cmprsd_attr_selectors.
                  offset_table_map[offset].offset=counter;
            attr->egr_attr.cmprsd_attr_selectors.
                  offset_table_map[offset].count_enable=1;

     } else {
        attr->egr_attr.uncmprsd_attr_selectors.
              offset_table_map[offset].offset=counter;
        attr->egr_attr.uncmprsd_attr_selectors.
              offset_table_map[offset].count_enable=1;
    }
    }
}
static 
uint8 _bcm_stat_flex_get_num_bits(uint8 value) 
{
      if (value <= 1) {
          return 1;
      }
      if (value <= 3) {
          return 2;
      }
      if (value <= 7) {
          return 3;
      }
      if (value <= 15) {
          return 4;
      }
      if (value <= 31) {
          return 5;
      }
      if (value <= 63) {
          return 6;
      }
      if (value <= 127) {
          return 7;
      } else {
          return 8;
      }
}

/* Create Customized Stat Group mode for given Counter Attributes:CreateMode  */
static 
bcm_error_t _bcm_esw_stat_group_mode_associate_id(
            int unit,
            uint32 flags,
            bcm_stat_flex_attribute_t  *flex_attribute,
            uint32 *mode_id)
{
    bcm_stat_flex_mode_t     mode=0;
    bcm_error_t              rv=BCM_E_NONE;
    bcm_stat_flex_attr_t     *attr=NULL;
    bcm_stat_flex_ing_attr_t *ing_attr=NULL;
    bcm_stat_flex_egr_attr_t *egr_attr=NULL;
    bcm_stat_flex_ing_cmprsd_attr_selectors_t *ing_cmprsd_attr_selectors=NULL;
    bcm_stat_flex_ing_pkt_attr_bits_t         *ing_cmprsd_pkt_attr_bits=NULL;

    bcm_stat_flex_egr_cmprsd_attr_selectors_t *egr_cmprsd_attr_selectors=NULL;
    bcm_stat_flex_egr_pkt_attr_bits_t         *egr_cmprsd_pkt_attr_bits=NULL;
    bcm_stat_flex_direction_t                 direction =
                                              bcmStatFlexDirectionIngress;
    uint8                                     total_bits=0;
    uint8                                     counter=0;
    uint8                                     loop=0;
    uint8                 *offset_array[8]={NULL};
    uint8                 *map_array[bcmStatGroupModeAttrPacketTypeIp+1]={NULL};
    uint8                                     shift_by_bits=0;
#if 0
    uint8                                     shift_by_bits_for_values=0;
#endif
    uint8                                     temp_count=0;
    uint8                                     max_count=0;
    uint8                                     cng_bits=0;
    uint8                                     ifp_cng_bits=0;
    uint8                                     int_pri_bits=0;
    uint8                                     vlan_format_bits=0;
    uint8                                     outer_dot1p_bits=0;
    uint8                                     inner_dot1p_bits=0;
    uint8                                     port_bits=0;
    uint8                                     tos_dscp_bits=0;
    uint8                                     tos_ecn_bits=0;
    uint8                                     pkt_resolution_bits=0;
    uint8                                     svp_bits=0;
    uint8                                     dvp_bits=0;
    uint8                                     drop_bits=0;
    uint8                                     ip_pkt_bits=0;
    uint8                                     unknown_pkt=0;
    uint8                                     control_pkt=0;
    uint8                                     oam_pkt=0;
    uint8                                     bfd_pkt=0;
    uint8                                     bpdu_pkt=0;
    uint8                                     icnm_pkt=0;
    uint8                                     _1588_pkt=0;
    uint8                                     known_l2uc_pkt=0;
    uint8                                     unknown_l2uc_pkt=0;
    uint8                                     l2bc_pkt=0;
    uint8                                     known_l2mc_pkt=0;
    uint8                                     unknown_l2mc_pkt=0;
    uint8                                     known_l3uc_pkt=0;
    uint8                                     unknown_l3uc_pkt=0;
    uint8                                     known_ipmc_pkt=0;
    uint8                                     unknown_ipmc_pkt=0;
    uint8                                     known_mpls_l2_pkt=0;
    uint8                                     known_mpls_l3_pkt=0;
    uint8                                     known_mpls_pkt=0;
    uint8                                     unknown_mpls_pkt=0;
    uint8                                     known_mpls_multicast_pkt=0;
    uint8                                     known_mim_pkt=0;
    uint8                                     unknown_mim_pkt=0;
    uint8                                     known_trill_pkt=0;
    uint8                                     unknown_trill_pkt=0;
    uint8                                     known_niv_pkt=0;
    uint8                                     unknown_niv_pkt=0;
    uint16                                     loop_index=0;
    uint16                                     final_index=0;
    uint16                                     index0=0;
    uint16                                     index1=0;
    uint16                                     index2=0;
    uint16                                     index3=0;
    uint16                                     index4=0;
    uint16                                     index5=0;
    uint16                                     index6=0;
    uint16                                     index7=0;
    uint8                                      offset=0;
    uint8                                      final_mapped_value;
    uint8                                      mapped_value0;
    uint8                                      mapped_value1;
    uint8                                      mapped_value2;
    uint8                                      mapped_value3;
    uint8                                      max_bits=0;

    uint8                                      pre_ifp_cng_cmprsd_max_bits=0;
    uint8                                      ifp_cng_cmprsd_max_bits=0;
    uint8                                      int_pri_cmprsd_max_bits=0;

    uint8                                      vlan_format_cmprsd_max_bits=0;
    uint8                                      outer_dot1p_cmprsd_max_bits=0;
    uint8                                      inner_dot1p_cmprsd_max_bits=0;

    uint8                                      port_cmprsd_max_bits=0;

    uint8                                      tos_dscp_cmprsd_max_bits=0;
    uint8                                      tos_ecn_cmprsd_max_bits=0;

    uint8                                      pkt_resolution_cmprsd_max_bits=0;
    uint8                                      svp_cmprsd_max_bits=0;
    uint8                                      dvp_cmprsd_max_bits=0;
    uint8                                      drop_cmprsd_max_bits=0;

    if (flags & BCM_STAT_GROUP_MODE_INGRESS) {
        direction = bcmStatFlexDirectionIngress;
    } else {
        direction = bcmStatFlexDirectionEgress;
    } 
    attr = (bcm_stat_flex_attr_t *) sal_alloc(sizeof(bcm_stat_flex_attr_t),
                                              "attr");
    if (attr == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(attr,0,sizeof(bcm_stat_flex_attr_t)); 

    if (direction == bcmStatFlexDirectionIngress) {
        /* INGRESS SIDE */
        attr->direction=bcmStatFlexDirectionIngress;
        ing_attr = &(attr->ing_attr);
        ing_cmprsd_attr_selectors=&(ing_attr->cmprsd_attr_selectors);
        ing_cmprsd_pkt_attr_bits= &(ing_attr->cmprsd_attr_selectors.
                                    pkt_attr_bits);
        unknown_pkt      = _bcm_esw_stat_flex_get_pkt_res_value(unit ,
                                                                _UNKNOWN_PKT);
        control_pkt      = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                                                                _CONTROL_PKT);
        oam_pkt          = _bcm_esw_stat_flex_get_pkt_res_value(unit,_OAM_PKT);
        bfd_pkt          = _bcm_esw_stat_flex_get_pkt_res_value(unit,_BFD_PKT);
        bpdu_pkt         = _bcm_esw_stat_flex_get_pkt_res_value(unit,_BPDU_PKT);
        icnm_pkt         = _bcm_esw_stat_flex_get_pkt_res_value(unit,_ICNM_PKT);
        _1588_pkt        = _bcm_esw_stat_flex_get_pkt_res_value(unit,_1588_PKT);
        known_l2uc_pkt   = _bcm_esw_stat_flex_get_pkt_res_value(unit,_L2UC_PKT);
        unknown_l2uc_pkt = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                                                                _L2DLF_PKT);
        l2bc_pkt         = _bcm_esw_stat_flex_get_pkt_res_value(unit,_L2BC_PKT);
        known_l2mc_pkt   = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                           _KNOWN_L2MC_PKT);
        unknown_l2mc_pkt = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                           _UNKNOWN_L2MC_PKT);
        known_l3uc_pkt   = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                           _KNOWN_L3UC_PKT);
        unknown_l3uc_pkt = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                           _UNKNOWN_L3UC_PKT);
        known_ipmc_pkt   = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                           _KNOWN_IPMC_PKT);
        unknown_ipmc_pkt = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                           _UNKNOWN_IPMC_PKT);
        known_mpls_l2_pkt = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                           _KNOWN_MPLS_L2_PKT);
        known_mpls_l3_pkt = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                            _KNOWN_MPLS_L3_PKT);
        known_mpls_pkt    = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                            _KNOWN_MPLS_PKT);
        unknown_mpls_pkt  = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                            _UNKNOWN_MPLS_PKT);
        known_mpls_multicast_pkt = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                                   _KNOWN_MPLS_MULTICAST_PKT);
        known_mim_pkt     = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                            _KNOWN_MIM_PKT);
        unknown_mim_pkt   = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                            _UNKNOWN_MIM_PKT);
        known_trill_pkt     = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                            _KNOWN_TRILL_PKT);
        unknown_trill_pkt   = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                            _UNKNOWN_TRILL_PKT);
        known_niv_pkt     = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                            _KNOWN_NIV_PKT);
        unknown_niv_pkt   = _bcm_esw_stat_flex_get_pkt_res_value(unit,
                            _UNKNOWN_NIV_PKT);
    } else {
        /* EGRESS SIDE */
        attr->direction=bcmStatFlexDirectionEgress;
        egr_attr = &(attr->egr_attr);
        egr_cmprsd_attr_selectors=&(egr_attr->cmprsd_attr_selectors);
        egr_cmprsd_pkt_attr_bits= &(egr_attr->cmprsd_attr_selectors.
                                    pkt_attr_bits);
        unknown_pkt      = 0;
        control_pkt      = 0;
        oam_pkt          = 0;
        bfd_pkt          = 0;
        bpdu_pkt         = 0;
        icnm_pkt         = 0;
        _1588_pkt        = 0;
        known_l2uc_pkt   = 0;
        unknown_l2uc_pkt = 0;
        l2bc_pkt         = 1;
        known_l2mc_pkt   = 1;
        unknown_l2mc_pkt = 1;
        known_l3uc_pkt   = 0;
        unknown_l3uc_pkt = 0;
        known_ipmc_pkt   = 0;
        unknown_ipmc_pkt = 0;
        known_mpls_l2_pkt = 0;
        known_mpls_l3_pkt = 0;
        known_mpls_pkt    = 0;
        unknown_mpls_pkt  = 0;
        known_mpls_multicast_pkt = 1;
        known_mim_pkt     = 0;
        unknown_mim_pkt   = 0;
        known_trill_pkt     = 0;
        unknown_trill_pkt   = 0;
        known_niv_pkt     = 0;
        unknown_niv_pkt   = 0;
    }
    if (flex_attribute->pre_ifp_color != 0) {
        total_bits += cng_bits =
                      (direction == bcmStatFlexDirectionIngress) ?
                       ing_pkt_attr_uncmprsd_bits_g.cng: egr_pkt_attr_uncmprsd_bits_g.cng;
    }
    if (flex_attribute->ifp_color != 0) {
        if (direction == bcmStatFlexDirectionEgress) {
            sal_free(attr);
            return BCM_E_PARAM;
        }
        ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                  BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_IFP_CNG_ATTR_BITS;
        total_bits += ifp_cng_bits = ing_pkt_attr_uncmprsd_bits_g.ifp_cng;
    }
    if (flex_attribute->int_pri != 0) {
        total_bits += int_pri_bits =
                       (direction == bcmStatFlexDirectionIngress) ?
                       ing_pkt_attr_uncmprsd_bits_g.int_pri:egr_pkt_attr_uncmprsd_bits_g.int_pri;
    }
    if (flex_attribute->vlan_format != 0) {
        total_bits += vlan_format_bits =
                      (direction == bcmStatFlexDirectionIngress) ?
                       ing_pkt_attr_uncmprsd_bits_g.vlan_format:
                       egr_pkt_attr_uncmprsd_bits_g.vlan_format;
    }
    if (flex_attribute->outer_dot1p != 0) {
        total_bits += outer_dot1p_bits =
                      (direction == bcmStatFlexDirectionIngress) ?
                       ing_pkt_attr_uncmprsd_bits_g.outer_dot1p:
                       egr_pkt_attr_uncmprsd_bits_g.outer_dot1p;
    }
    if (flex_attribute->inner_dot1p != 0) {
        total_bits += inner_dot1p_bits =
                      (direction == bcmStatFlexDirectionIngress) ?
                       ing_pkt_attr_uncmprsd_bits_g.inner_dot1p:
                       egr_pkt_attr_uncmprsd_bits_g.inner_dot1p;
    }
    if (flex_attribute->port != 0) {
        total_bits += port_bits =
                      (direction == bcmStatFlexDirectionIngress) ?
                       ing_pkt_attr_uncmprsd_bits_g.ing_port:
                       egr_pkt_attr_uncmprsd_bits_g.egr_port;
    }
    if (flex_attribute->tos_dscp != 0) {
        total_bits += tos_dscp_bits =
                      (direction == bcmStatFlexDirectionIngress) ?
                       ing_pkt_attr_uncmprsd_bits_g.tos_dscp:
                       egr_pkt_attr_uncmprsd_bits_g.tos_dscp;
    }
    if (flex_attribute->tos_ecn != 0) {
        total_bits += tos_ecn_bits =
                      (direction == bcmStatFlexDirectionIngress) ?
                       ing_pkt_attr_uncmprsd_bits_g.tos_ecn:
                       egr_pkt_attr_uncmprsd_bits_g.tos_ecn;
    }
    if (flex_attribute->pkt_resolution != 0) {
        total_bits += pkt_resolution_bits =
                      (direction == bcmStatFlexDirectionIngress) ?
                       ing_pkt_attr_uncmprsd_bits_g.pkt_resolution:
                       egr_pkt_attr_uncmprsd_bits_g.pkt_resolution;
    }
    if (flex_attribute->svp != 0) {
        total_bits += svp_bits =
                      (direction == bcmStatFlexDirectionIngress) ?
                       ing_pkt_attr_uncmprsd_bits_g.svp_type:
                       egr_pkt_attr_uncmprsd_bits_g.svp_type;
    }
    if (flex_attribute->dvp != 0) {
        if (direction == bcmStatFlexDirectionIngress) {
            sal_free(attr);
            return BCM_E_PARAM;
        }
        total_bits += dvp_bits = egr_pkt_attr_uncmprsd_bits_g.dvp_type;
    }
    if (flex_attribute->drop != 0) {
        total_bits += drop_bits =
                      (direction == bcmStatFlexDirectionIngress) ?
                       ing_pkt_attr_uncmprsd_bits_g.drop:
                       egr_pkt_attr_uncmprsd_bits_g.drop;
    }

    if (total_bits <= BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS ) {
        LOG_CLI((BSL_META_U(unit,
                            "INFO: UnCompressedMode Will be used \n")));
        if (direction == bcmStatFlexDirectionIngress) {
            ing_attr->packet_attr_type=bcmStatFlexPacketAttrTypeUncompressed;
            if (flex_attribute->pre_ifp_color != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                          BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_CNG_ATTR_BITS;
            }
            if (flex_attribute->ifp_color != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                          BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_IFP_CNG_ATTR_BITS;
            }
            if (flex_attribute->int_pri != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                          BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS;
            }
            if (flex_attribute->vlan_format != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_VLAN_FORMAT_ATTR_BITS;
            }
            if (flex_attribute->outer_dot1p != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS;
            }
            if (flex_attribute->inner_dot1p != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS;
            }
            if (flex_attribute->port != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_INGRESS_PORT_ATTR_BITS;
            }
            if (flex_attribute->tos_dscp != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS;
            }
            if (flex_attribute->tos_ecn != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_TOS_ECN_ATTR_BITS;
            }
            if (flex_attribute->pkt_resolution != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                   BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
            }
            if (flex_attribute->svp != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                   BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS;
            }
            if (flex_attribute->drop != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                   BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_DROP_ATTR_BITS;
            }
            if (flex_attribute->ip_pkt != 0) {
                ing_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                   BCM_STAT_FLEX_ING_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS;
            }
            ing_attr->uncmprsd_attr_selectors.total_counters = 
                      flex_attribute->total_counters;
            /* Reset all Offset table fields */
            for (index0=0;index0<256;index0++) {
                 ing_attr->uncmprsd_attr_selectors.offset_table_map[index0].
                           offset=0;
                 ing_attr->uncmprsd_attr_selectors.offset_table_map[index0].
                           count_enable=0;
           }
        } else {
            egr_attr->packet_attr_type=bcmStatFlexPacketAttrTypeUncompressed;
            if (flex_attribute->pre_ifp_color != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                          BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_CNG_ATTR_BITS;
            }
            if (flex_attribute->int_pri != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                          BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS;
            }
            if (flex_attribute->vlan_format != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_VLAN_FORMAT_ATTR_BITS;
            }
            if (flex_attribute->outer_dot1p != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS;
            }
            if (flex_attribute->inner_dot1p != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS;
            }
            if (flex_attribute->port != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_EGRESS_PORT_ATTR_BITS;
            }
            if (flex_attribute->tos_dscp != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_TOS_DSCP_ATTR_BITS;
            }
            if (flex_attribute->tos_ecn != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                      BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_TOS_ECN_ATTR_BITS;
            }
            if (flex_attribute->pkt_resolution != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                   BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
            }
            if (flex_attribute->svp != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                   BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS;
            }
            if (flex_attribute->dvp != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                   BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_DVP_TYPE_ATTR_BITS;
            }
            if (flex_attribute->drop != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                   BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_DROP_ATTR_BITS;
            }
            if (flex_attribute->ip_pkt != 0) {
                egr_attr->uncmprsd_attr_selectors.uncmprsd_attr_bits_selector |=
                   BCM_STAT_FLEX_EGR_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS;
            }
            egr_attr->uncmprsd_attr_selectors.total_counters = 
                      flex_attribute->total_counters;
            /* Reset all Offset table fields */
            for (index0=0;index0<256;index0++) {
                 egr_attr->uncmprsd_attr_selectors.offset_table_map[index0].
                           offset=0;
                 egr_attr->uncmprsd_attr_selectors.offset_table_map[index0].
                           count_enable=0;
           }
        }

       for (loop=0;loop<8;loop++) {
            offset_array[loop] = (uint8 *) sal_alloc(256+1,"offset_array");
       }
       /* Second Level Analysis: Update offset_array */
       for (counter=0;counter < flex_attribute->total_counters ; counter ++ ) {
            for (loop=0;loop<8;loop++) {
                 sal_memset(offset_array[loop],0,256+1);
            }
            /* Logical Group0 */
            loop = 0;
            if (flex_attribute->pre_ifp_color != 0) {
                shift_by_bits = ifp_cng_bits +
                                int_pri_bits + 
                                vlan_format_bits + 
                                outer_dot1p_bits + 
                                inner_dot1p_bits + 
                                port_bits + 
                                tos_dscp_bits + 
                                tos_ecn_bits + 
                                pkt_resolution_bits + 
                                svp_bits + 
                                dvp_bits + 
                                drop_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    pre_ifp_color_flags & BCM_STAT_FLEX_COLOR_GREEN) {
                    temp_count = offset_array[loop][0];
                                                       /* GreenIndex=0*/
                    offset_array[loop][temp_count+1] = (0 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pre_ifp_color_flags & BCM_STAT_FLEX_COLOR_RED) {
                    temp_count = offset_array[loop][0];
                                                      /* RedIndex=1 */
                    offset_array[loop][temp_count+1]= (1 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pre_ifp_color_flags & BCM_STAT_FLEX_COLOR_YELLOW) {
                    temp_count = offset_array[loop][0];
                                                      /* YellowIndex=3 */
                    offset_array[loop][temp_count+1]= (3 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            if (flex_attribute->ifp_color != 0) {
                shift_by_bits = int_pri_bits + 
                                vlan_format_bits + 
                                outer_dot1p_bits + 
                                inner_dot1p_bits + 
                                port_bits + 
                                tos_dscp_bits + 
                                tos_ecn_bits + 
                                pkt_resolution_bits + 
                                svp_bits + 
                                dvp_bits + 
                                drop_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    ifp_color_flags & BCM_STAT_FLEX_COLOR_GREEN) {
                    temp_count = offset_array[loop][0];
                                                       /* GreenIndex=0*/
                    offset_array[loop][temp_count+1] = (0 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    ifp_color_flags & BCM_STAT_FLEX_COLOR_RED) {
                    temp_count = offset_array[loop][0];
                                                      /* RedIndex=1 */
                    offset_array[loop][temp_count+1]= (1 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    ifp_color_flags & BCM_STAT_FLEX_COLOR_YELLOW) {
                    temp_count = offset_array[loop][0];
                                                      /* YellowIndex=3 */
                    offset_array[loop][temp_count+1]= (3 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            if (flex_attribute->int_pri != 0) {
                shift_by_bits = vlan_format_bits + 
                                outer_dot1p_bits + 
                                inner_dot1p_bits + 
                                port_bits + 
                                tos_dscp_bits + 
                                tos_ecn_bits + 
                                pkt_resolution_bits + 
                                svp_bits + 
                                dvp_bits + 
                                drop_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI0) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (0 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI1) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (1 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI2) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (2 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI3) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (3 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI4) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (4 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI5) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (5 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI6) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (6 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI7) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (7 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI8) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (8 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI9) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (9 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI10) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (10 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI11) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (11 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI12) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (12 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI13) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (13 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI14) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (14 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI15) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (15 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            /* LogicalGroup1 */
            if (flex_attribute->vlan_format != 0) {
                shift_by_bits = outer_dot1p_bits + 
                                inner_dot1p_bits + 
                                port_bits + 
                                tos_dscp_bits + 
                                tos_ecn_bits + 
                                pkt_resolution_bits + 
                                svp_bits + 
                                dvp_bits + 
                                drop_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_UNTAGGED) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (0 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_INNER) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (1 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_OUTER) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (2 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_BOTH) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (3 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            if (flex_attribute->outer_dot1p != 0) {
                shift_by_bits = inner_dot1p_bits + 
                                port_bits + 
                                tos_dscp_bits + 
                                tos_ecn_bits + 
                                pkt_resolution_bits + 
                                svp_bits + 
                                dvp_bits + 
                                drop_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI0) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (0 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI1) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (1 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI2) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (2 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI3) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (3 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI4) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (4 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI5) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (5 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI6) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (6 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI7) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (7 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            if (flex_attribute->inner_dot1p != 0) {
                shift_by_bits = port_bits + 
                                tos_dscp_bits + 
                                tos_ecn_bits + 
                                pkt_resolution_bits + 
                                svp_bits + 
                                dvp_bits + 
                                drop_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI0) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (0 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI1) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (1 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI2) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (2 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI3) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (3 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI4) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (4 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI5) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (5 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI6) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (6 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI7) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (7 << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            /* LogicalGroup2 */
            if (flex_attribute->port != 0) {
                shift_by_bits = tos_dscp_bits + 
                                tos_ecn_bits + 
                                pkt_resolution_bits + 
                                svp_bits + 
                                dvp_bits + 
                                drop_bits + 
                                ip_pkt_bits ;
                if (direction == bcmStatFlexDirectionIngress) {
                    max_count= (1 << ing_pkt_attr_uncmprsd_bits_g.ing_port) - 1;
                } else {
                    max_count= (1 << egr_pkt_attr_uncmprsd_bits_g.egr_port) - 1;
                }
                for (loop_index=0; loop_index <= max_count ; loop_index++) { 
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrPort],loop_index)) {
                        temp_count = offset_array[loop][0];
                        offset_array[loop][temp_count+1]= (loop_index /*value*/
                                                           << shift_by_bits); 
                        offset_array[loop][0] = temp_count+1;
                    } 
                }
                loop++;
            }
            /* LogicalGroup3 */
            if ((flex_attribute->tos_dscp != 0) && 
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = tos_ecn_bits + 
                                pkt_resolution_bits + 
                                svp_bits + 
                                dvp_bits + 
                                drop_bits + 
                                ip_pkt_bits ;
                if (direction == bcmStatFlexDirectionIngress) {
                   max_count= (1 << ing_pkt_attr_uncmprsd_bits_g.tos_dscp) - 1;
                } else {
                   max_count= (1 << egr_pkt_attr_uncmprsd_bits_g.tos_dscp) - 1;
                }
                for (loop_index=0; loop_index <= max_count ; loop_index++) { 
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrTosDscp],loop_index)) {
                        temp_count = offset_array[loop][0];
                        offset_array[loop][temp_count+1]= (loop_index /*value*/
                                                           << (shift_by_bits)); 
                        offset_array[loop][0] = temp_count+1;
                    } 
                }
                loop++;
            }
            if ((flex_attribute->tos_ecn != 0) && 
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = pkt_resolution_bits + 
                                svp_bits + 
                                dvp_bits + 
                                drop_bits + 
                                ip_pkt_bits ;
                if (direction == bcmStatFlexDirectionIngress) {
                   max_count= (1 << ing_pkt_attr_uncmprsd_bits_g.tos_ecn) - 1;
                } else {
                   max_count= (1 << egr_pkt_attr_uncmprsd_bits_g.tos_ecn) - 1;
                }
                for (loop_index=0; loop_index <= max_count ; loop_index++) { 
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrTosEcn],loop_index)) {
                        temp_count = offset_array[loop][0];
                        offset_array[loop][temp_count+1]= (loop_index /*value*/
                                                           << (shift_by_bits)); 
                        offset_array[loop][0] = temp_count+1;
                    } 
                }
                loop++;
            }
            /* LogicalGroup4 */
            if ((flex_attribute->pkt_resolution != 0) && 
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = svp_bits + 
                                dvp_bits + 
                                drop_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (unknown_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_CONTROL_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (control_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_OAM_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (oam_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_BFD_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (bfd_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_BPDU_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (bpdu_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_ICNM_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (icnm_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_PKT_IS_1588) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (_1588_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_BFD_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (bfd_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2UC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (known_l2uc_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2UC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (unknown_l2uc_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_L2BC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (l2bc_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2MC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (known_l2mc_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2MC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (unknown_l2mc_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_L3UC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (known_l3uc_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L3UC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (unknown_l3uc_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_IPMC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (known_ipmc_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_IPMC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (unknown_ipmc_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L2_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (known_mpls_l2_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L3_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (known_mpls_l3_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (known_mpls_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MPLS_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (unknown_mpls_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_MULTICAST_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (known_mpls_multicast_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_MIM_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (known_mim_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MIM_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (unknown_mim_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_TRILL_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (known_trill_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_TRILL_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (unknown_trill_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_NIV_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (known_niv_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_NIV_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (unknown_niv_pkt << 
                                                       shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            /* Logical Group 5 */
            if ((flex_attribute->svp != 0) && 
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = dvp_bits + 
                                drop_bits + 
                                ip_pkt_bits ;
                if (direction == bcmStatFlexDirectionIngress) {
                    max_count= (1 << ing_pkt_attr_uncmprsd_bits_g.svp_type)-1;
                } else {
                    max_count= (1 << egr_pkt_attr_uncmprsd_bits_g.svp_type)-1;
                }
                for (loop_index=0; loop_index <= max_count ; loop_index++) { 
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrSvp],loop_index)) {
                        temp_count = offset_array[loop][0];
                        offset_array[loop][temp_count+1]= (loop_index /*value*/
                                                           << shift_by_bits); 
                        offset_array[loop][0] = temp_count+1;
                    } 
                }
                loop++;
            }
            if ((flex_attribute->dvp != 0) &&
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = drop_bits + 
                                ip_pkt_bits ;
                max_count= (1 << egr_pkt_attr_uncmprsd_bits_g.dvp_type)-1;
                for (loop_index=0; loop_index <= max_count ; loop_index++) { 
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrDvp],loop_index)) {
                        temp_count = offset_array[loop][0];
                        offset_array[loop][temp_count+1]= (loop_index /*value*/
                                                           << shift_by_bits); 
                        offset_array[loop][0] = temp_count+1;
                    } 
                }
                loop++;
            }
            if ((flex_attribute->drop != 0) &&
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    drop_flags & BCM_STAT_FLEX_DROP_DISABLE) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (0 << shift_by_bits);
                    offset_array[loop][0] = temp_count;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    drop_flags & BCM_STAT_FLEX_DROP_ENABLE) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (1 << shift_by_bits);
                    offset_array[loop][0] = temp_count;
                }
                loop++;
            }
            if ((flex_attribute->ip_pkt != 0) &&
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = 0;
                if (flex_attribute->combine_attr_counter[counter].
                    ip_pkt_flags & BCM_STAT_FLEX_IP_PKT_DISABLE) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (0 << shift_by_bits);
                    offset_array[loop][0] = temp_count;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    ip_pkt_flags & BCM_STAT_FLEX_IP_PKT_ENABLE) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (1 << shift_by_bits);
                    offset_array[loop][0] = temp_count;
                }
                loop++;
            }
            /* Third and Final Level Update: Make Absolute Offset */
            for (index0=0;
                 index0 < offset_array[0][0] ; 
                 index0++) {
                 offset = offset_array[0][index0+1];
                 if (offset_array[1][0] != 0) {
                     for (index1=0;
                          index1 < offset_array[1][0] ; 
                          index1++) {
                          offset = offset_array[0][index0+1] | offset_array[1][index1+1];
                          if (offset_array[2][0] != 0) {
                              for (index2=0;
                                   index2 < offset_array[2][0] ; 
                                   index2++) {
                                   offset = offset_array[0][index0+1] | 
                                            offset_array[1][index1+1] |
                                            offset_array[2][index2+1];
                                   if (offset_array[3][0] != 0) {
                                       for (index3=0;
                                            index3 < offset_array[3][0] ; 
                                            index3++) {
                                            offset =  offset_array[0][index0+1] | 
                                                      offset_array[1][index1+1] |
                                                      offset_array[2][index2+1] |
                                                      offset_array[3][index3+1];
                                            if (offset_array[4][0] != 0) {
                                                for (index4=0;
                                                     index4 < offset_array[4][0] ; 
                                                     index4++) {
                                                     offset =  offset_array[0][index0+1] | 
                                                               offset_array[1][index1+1] |
                                                               offset_array[2][index2+1] |
                                                               offset_array[3][index3+1] |
                                                               offset_array[4][index4+1];
                                                     if (offset_array[5][0] != 0) {
                                                         for (index5=0;
                                                              index5 < offset_array[5][0] ; 
                                                              index5++) {
                                                              offset =  offset_array[0][index0+1] | 
                                                                        offset_array[1][index1+1] |
                                                                        offset_array[2][index2+1] |
                                                                        offset_array[3][index3+1] |
                                                                        offset_array[4][index4+1] |
                                                                        offset_array[5][index5+1];
                                                              if (offset_array[6][0] != 0) {
                                                                  for (index6=0;
                                                                       index6 < offset_array[6][0] ; 
                                                                       index6++) {
                                                                       offset =  offset_array[0][index0+1] | 
                                                                                 offset_array[1][index1+1] |
                                                                                 offset_array[2][index2+1] |
                                                                                 offset_array[3][index3+1] |
                                                                                 offset_array[4][index4+1] |
                                                                                 offset_array[5][index5+1] |
                                                                                 offset_array[6][index6+1];
                                                                       if (offset_array[7][0] != 0) {
                                                                           for (index7=0;
                                                                                index7 < offset_array[7][0] ; 
                                                                                index7++) {
                                                                                offset =  offset_array[0][index0+1] | 
                                                                                          offset_array[1][index1+1] |
                                                                                          offset_array[2][index2+1] |
                                                                                          offset_array[3][index3+1] |
                                                                                          offset_array[4][index4+1] |
                                                                                          offset_array[5][index5+1] |
                                                                                          offset_array[6][index6+1] |
                                                                                          offset_array[7][index6+1];
                                                                                _bcm_stat_flex_update_map(direction,attr,
                                                                                                          offset,counter);
                                                                           }
                                                                       } else {
                                                                           _bcm_stat_flex_update_map(direction,attr,
                                                                                                     offset,counter);
                                                                       }
                                                                  }
                                                              } else {
                                                                  _bcm_stat_flex_update_map(direction,attr,offset,counter);
                                                              }
                                                         }
                                                     } else {
                                                         _bcm_stat_flex_update_map(direction,attr,offset,counter);
                                                     }
                                                }
                                            } else {
                                                _bcm_stat_flex_update_map(direction,attr,offset,counter);
                                            }
                                       }
                                   } else {
                                       _bcm_stat_flex_update_map(direction,attr,offset,counter);
                                   }
                              }
                          } else {
                              _bcm_stat_flex_update_map(direction,attr,offset,counter);
                          }
                     }
                 } else {
                     _bcm_stat_flex_update_map(direction,attr,offset,counter);
                 }
            }
       }
 } else {
        LOG_CLI((BSL_META_U(unit,
                            "INFO: TotalBits=%d Trying out CompressedMode\n"),
                 total_bits));
        if (direction == bcmStatFlexDirectionIngress) {
            ing_attr->packet_attr_type=bcmStatFlexPacketAttrTypeCompressed;
        } else {
            egr_attr->packet_attr_type=bcmStatFlexPacketAttrTypeCompressed;
        }
        ing_attr->cmprsd_attr_selectors.total_counters = 
                      flex_attribute->total_counters;
        for (counter=0;counter < flex_attribute->total_counters ; counter ++ ) {
             /* BEGIN: PRI_CNG_MAP */ 
             if (flex_attribute->pre_ifp_color != 0) {
                 if (map_array[bcmStatGroupModeAttrColor] == NULL) {
                     map_array[bcmStatGroupModeAttrColor] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                     sal_memset(map_array[bcmStatGroupModeAttrColor],
                                0xFF,256);
                     map_array[bcmStatGroupModeAttrColor][256]=0;
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pre_ifp_color_flags & BCM_STAT_FLEX_COLOR_GREEN) {
                     if (map_array[bcmStatGroupModeAttrColor][0] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrColor][256];
                         map_array[bcmStatGroupModeAttrColor][0] = temp_count;
                         map_array[bcmStatGroupModeAttrColor][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pre_ifp_color_flags & BCM_STAT_FLEX_COLOR_RED) {
                     if (map_array[bcmStatGroupModeAttrColor][1] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrColor][256];
                         map_array[bcmStatGroupModeAttrColor][1] = temp_count;
                         map_array[bcmStatGroupModeAttrColor][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pre_ifp_color_flags & BCM_STAT_FLEX_COLOR_YELLOW) {
                     if (map_array[bcmStatGroupModeAttrColor][3] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrColor][256];
                         map_array[bcmStatGroupModeAttrColor][3] = temp_count;
                         map_array[bcmStatGroupModeAttrColor][256]=temp_count+1;
                     }
                 }
             }
             if (flex_attribute->ifp_color != 0) {
                 if (map_array[bcmStatGroupModeAttrFieldIngressColor] == NULL) {
                     map_array[bcmStatGroupModeAttrFieldIngressColor] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrFieldIngressColor],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrFieldIngressColor][256]=0;
                 }
                 /* Egress Not Possbile */
                 /* 
                 shift_by_bits = ing_pkt_attr_cmprsd_bits_g.int_pri;
                  */
                 if (flex_attribute->combine_attr_counter[counter].
                     ifp_color_flags & BCM_STAT_FLEX_COLOR_GREEN) {
                     if (map_array[bcmStatGroupModeAttrFieldIngressColor][0] 
                         == 0xFF) {
                         temp_count = map_array
                                      [bcmStatGroupModeAttrFieldIngressColor]
                                      [256];
                         map_array[bcmStatGroupModeAttrFieldIngressColor][0] = 
                                      temp_count;
                         map_array[bcmStatGroupModeAttrFieldIngressColor][256] =
                                      temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     ifp_color_flags & BCM_STAT_FLEX_COLOR_RED) {
                     if (map_array[bcmStatGroupModeAttrFieldIngressColor][1] 
                         == 0xFF) {
                         temp_count = map_array
                                      [bcmStatGroupModeAttrFieldIngressColor]
                                      [256];
                         map_array[bcmStatGroupModeAttrFieldIngressColor][1] = 
                                      temp_count;
                         map_array[bcmStatGroupModeAttrFieldIngressColor][256] =
                                      temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     ifp_color_flags & BCM_STAT_FLEX_COLOR_YELLOW) {
                     if (map_array[bcmStatGroupModeAttrFieldIngressColor][3] 
                         == 0xFF) {
                         temp_count = map_array
                                      [bcmStatGroupModeAttrFieldIngressColor]
                                      [256];
                         map_array[bcmStatGroupModeAttrFieldIngressColor][3] = 
                                      temp_count;
                         map_array[bcmStatGroupModeAttrFieldIngressColor][256] =
                                      temp_count+1;
                     }
                 }
             }
             if (flex_attribute->int_pri != 0) {
                 /*
                 shift_by_bits = 0;
                  */
                 if (map_array[bcmStatGroupModeAttrIntPri] == NULL) {
                     map_array[bcmStatGroupModeAttrIntPri] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrIntPri],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrIntPri][256]=0;
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI0) {
                     if (map_array[bcmStatGroupModeAttrIntPri][0] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][0] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI1) {
                     if (map_array[bcmStatGroupModeAttrIntPri][1] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][1] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI2) {
                     if (map_array[bcmStatGroupModeAttrIntPri][2] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][2] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI3) {
                     if (map_array[bcmStatGroupModeAttrIntPri][3] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][3] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI4) {
                     if (map_array[bcmStatGroupModeAttrIntPri][4] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][4] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI5) {
                     if (map_array[bcmStatGroupModeAttrIntPri][5] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][5] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI6) {
                     if (map_array[bcmStatGroupModeAttrIntPri][6] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][6] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI7) {
                     if (map_array[bcmStatGroupModeAttrIntPri][7] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][7] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI8) {
                     if (map_array[bcmStatGroupModeAttrIntPri][8] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][8] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI9) {
                     if (map_array[bcmStatGroupModeAttrIntPri][9] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][9] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI10) {
                     if (map_array[bcmStatGroupModeAttrIntPri][10] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][10] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI11) {
                     if (map_array[bcmStatGroupModeAttrIntPri][11] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][11] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI12) {
                     if (map_array[bcmStatGroupModeAttrIntPri][12] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][12] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI13) {
                     if (map_array[bcmStatGroupModeAttrIntPri][13] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][13] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI14) {
                     if (map_array[bcmStatGroupModeAttrIntPri][14] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][14] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     int_pri_flags & BCM_STAT_FLEX_PRI15) {
                     if (map_array[bcmStatGroupModeAttrIntPri][15] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrIntPri][256];
                        map_array[bcmStatGroupModeAttrIntPri][15] = temp_count;
                        map_array[bcmStatGroupModeAttrIntPri][256]=temp_count+1;
                     }
                 }
             }
             /* END: PRI_CNG_MAP */ 
             /* BEGIN: PKT_PRI_MAP*/ 
             if (flex_attribute->vlan_format != 0) {
                 if (map_array[bcmStatGroupModeAttrVlan] == NULL) {
                     map_array[bcmStatGroupModeAttrVlan] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrVlan],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrVlan][256]=0;
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_UNTAGGED) {
                     if (map_array[bcmStatGroupModeAttrVlan][0] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrVlan][256];
                        map_array[bcmStatGroupModeAttrVlan][0] = temp_count;
                        map_array[bcmStatGroupModeAttrVlan][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_INNER) {
                     if (map_array[bcmStatGroupModeAttrVlan][1] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrVlan][256];
                        map_array[bcmStatGroupModeAttrVlan][1] = temp_count;
                        map_array[bcmStatGroupModeAttrVlan][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_OUTER) {
                     if (map_array[bcmStatGroupModeAttrVlan][2] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrVlan][256];
                        map_array[bcmStatGroupModeAttrVlan][2] = temp_count;
                        map_array[bcmStatGroupModeAttrVlan][256]=temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_BOTH) {
                     if (map_array[bcmStatGroupModeAttrVlan][3] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrVlan][256];
                        map_array[bcmStatGroupModeAttrVlan][3] = temp_count;
                        map_array[bcmStatGroupModeAttrVlan][256]=temp_count+1;
                     }
                 }
             }
             if (flex_attribute->outer_dot1p != 0) {
                 if (map_array[bcmStatGroupModeAttrOuterPri] == NULL) {
                     map_array[bcmStatGroupModeAttrOuterPri] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrOuterPri],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrOuterPri][256]=0;
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     outer_dot1p_flags & BCM_STAT_FLEX_PRI0) {
                     if (map_array[bcmStatGroupModeAttrOuterPri][0] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrOuterPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrOuterPri][0] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrOuterPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     outer_dot1p_flags & BCM_STAT_FLEX_PRI1) {
                     if (map_array[bcmStatGroupModeAttrOuterPri][1] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrOuterPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrOuterPri][1] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrOuterPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     outer_dot1p_flags & BCM_STAT_FLEX_PRI2) {
                     if (map_array[bcmStatGroupModeAttrOuterPri][2] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrOuterPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrOuterPri][2] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrOuterPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     outer_dot1p_flags & BCM_STAT_FLEX_PRI3) {
                     if (map_array[bcmStatGroupModeAttrOuterPri][3] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrOuterPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrOuterPri][3] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrOuterPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     outer_dot1p_flags & BCM_STAT_FLEX_PRI4) {
                     if (map_array[bcmStatGroupModeAttrOuterPri][4] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrOuterPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrOuterPri][4] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrOuterPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     outer_dot1p_flags & BCM_STAT_FLEX_PRI5) {
                     if (map_array[bcmStatGroupModeAttrOuterPri][5] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrOuterPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrOuterPri][5] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrOuterPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     outer_dot1p_flags & BCM_STAT_FLEX_PRI6) {
                     if (map_array[bcmStatGroupModeAttrOuterPri][6] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrOuterPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrOuterPri][6] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrOuterPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     outer_dot1p_flags & BCM_STAT_FLEX_PRI7) {
                     if (map_array[bcmStatGroupModeAttrOuterPri][7] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrOuterPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrOuterPri][7] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrOuterPri][256] =
                                               temp_count+1;
                     }
                 }
             }
             if (flex_attribute->inner_dot1p != 0) {
                 if (map_array[bcmStatGroupModeAttrInnerPri] == NULL) {
                     map_array[bcmStatGroupModeAttrInnerPri] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrInnerPri],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrInnerPri][256]=0;
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     inner_dot1p_flags & BCM_STAT_FLEX_PRI0) {
                     if (map_array[bcmStatGroupModeAttrInnerPri][0] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrInnerPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrInnerPri][0] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrInnerPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     inner_dot1p_flags & BCM_STAT_FLEX_PRI1) {
                     if (map_array[bcmStatGroupModeAttrInnerPri][1] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrInnerPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrInnerPri][1] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrInnerPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     inner_dot1p_flags & BCM_STAT_FLEX_PRI2) {
                     if (map_array[bcmStatGroupModeAttrInnerPri][2] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrInnerPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrInnerPri][2] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrInnerPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     inner_dot1p_flags & BCM_STAT_FLEX_PRI3) {
                     if (map_array[bcmStatGroupModeAttrInnerPri][3] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrInnerPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrInnerPri][3] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrInnerPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     inner_dot1p_flags & BCM_STAT_FLEX_PRI4) {
                     if (map_array[bcmStatGroupModeAttrInnerPri][4] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrInnerPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrInnerPri][4] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrInnerPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     inner_dot1p_flags & BCM_STAT_FLEX_PRI5) {
                     if (map_array[bcmStatGroupModeAttrInnerPri][5] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrInnerPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrInnerPri][5] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrInnerPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     inner_dot1p_flags & BCM_STAT_FLEX_PRI6) {
                     if (map_array[bcmStatGroupModeAttrInnerPri][6] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrInnerPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrInnerPri][6] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrInnerPri][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     inner_dot1p_flags & BCM_STAT_FLEX_PRI7) {
                     if (map_array[bcmStatGroupModeAttrInnerPri][7] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrInnerPri]
                                               [256];
                         map_array[bcmStatGroupModeAttrInnerPri][7] = 
                                               temp_count;
                         map_array[bcmStatGroupModeAttrInnerPri][256] =
                                               temp_count+1;
                     }
                 }
             }
             /* END: PKT_PRI_MAP*/ 
             /* BEGIN: PORT_MAP*/ 
             if (flex_attribute->port != 0) {
                 if (map_array[bcmStatGroupModeAttrPort] == NULL) {
                     map_array[bcmStatGroupModeAttrPort] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrPort],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrPort][256]=0;
                 }
                if (direction == bcmStatFlexDirectionIngress) {
                    max_count= (1 << ing_pkt_attr_cmprsd_bits_g.ing_port) - 1;
                } else {
                    max_count= (1 << egr_pkt_attr_cmprsd_bits_g.egr_port) - 1;
                }
                for (loop_index=0; loop_index <= max_count ; loop_index++) {
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrPort],loop_index)) {
                        if (map_array[bcmStatGroupModeAttrPort][loop_index] 
                            == 0xFF) {
                            temp_count = map_array[bcmStatGroupModeAttrPort]
                                                  [256];
                            map_array[bcmStatGroupModeAttrPort][loop_index] = 
                                               temp_count;
                            map_array[bcmStatGroupModeAttrPort][256] =
                                               temp_count+1;
                        }
                    }
                } 
             }
             /* END: PORT_MAP*/ 
             /* BEGIN: TOS_MAP*/ 
             if (flex_attribute->tos_dscp != 0) {
                 if (map_array[bcmStatGroupModeAttrTosDscp] == NULL) {
                     map_array[bcmStatGroupModeAttrTosDscp] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrTosDscp],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrTosDscp][256]=0;
                 }
                if (direction == bcmStatFlexDirectionIngress) {
                   max_count= (1 << ing_pkt_attr_cmprsd_bits_g.tos_dscp) - 1;
                } else {
                   max_count= (1 << egr_pkt_attr_cmprsd_bits_g.tos_dscp) - 1;
                }
                for (loop_index=0; loop_index <= max_count ; loop_index++) {
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrTosDscp],loop_index)) {
                        if (map_array[bcmStatGroupModeAttrTosDscp][loop_index] 
                            == 0xFF) {
                            temp_count = map_array[bcmStatGroupModeAttrTosDscp]
                                                  [256];
                            map_array[bcmStatGroupModeAttrTosDscp][loop_index] = 
                                               temp_count;
                            map_array[bcmStatGroupModeAttrTosDscp][256] =
                                               temp_count+1;
                        }
                    }
                }
             }
             if (flex_attribute->tos_ecn != 0) {
                 if (map_array[bcmStatGroupModeAttrTosEcn] == NULL) {
                     map_array[bcmStatGroupModeAttrTosEcn] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrTosEcn],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrTosEcn][256]=0;
                 }
                if (direction == bcmStatFlexDirectionIngress) {
                   max_count= (1 << ing_pkt_attr_cmprsd_bits_g.tos_ecn) - 1;
                } else {
                   max_count= (1 << egr_pkt_attr_cmprsd_bits_g.tos_ecn) - 1;
                }
                for (loop_index=0; loop_index <= max_count ; loop_index++) {
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrTosEcn],loop_index)) {
                        if (map_array[bcmStatGroupModeAttrTosEcn][loop_index] 
                            == 0xFF) {
                            temp_count = map_array[bcmStatGroupModeAttrTosEcn]
                                                  [256];
                            map_array[bcmStatGroupModeAttrTosEcn][loop_index] = 
                                               temp_count;
                            map_array[bcmStatGroupModeAttrTosEcn][256] =
                                               temp_count+1;
                        }
                    }
                }
             }
             /* END: TOS_MAP*/ 
             /* BEGIN: PKT_RES_MAP*/ 
             if (flex_attribute->pkt_resolution != 0) {
                 if (map_array[bcmStatGroupModeAttrPktType] == NULL) {
                     map_array[bcmStatGroupModeAttrPktType] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrPktType],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrPktType][256]=0;
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_CONTROL_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [control_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [control_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_OAM_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [oam_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [oam_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_BFD_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [bfd_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [bfd_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_BPDU_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [bpdu_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [bpdu_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_ICNM_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [icnm_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [icnm_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_PKT_IS_1588) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [_1588_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [_1588_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_BFD_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [bfd_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [bfd_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2UC_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [known_l2uc_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [known_l2uc_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2UC_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_l2uc_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_l2uc_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_L2BC_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [l2bc_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [l2bc_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2MC_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [known_l2mc_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [known_l2mc_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2MC_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_l2mc_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_l2mc_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_KNOWN_L3UC_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [known_l3uc_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [known_l3uc_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L3UC_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_l3uc_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_l3uc_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_KNOWN_IPMC_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [known_ipmc_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [known_ipmc_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_IPMC_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_ipmc_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_ipmc_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L2_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [known_mpls_l2_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [known_mpls_l2_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L3_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [known_mpls_l3_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [known_mpls_l3_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [known_mpls_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [known_mpls_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MPLS_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_mpls_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_mpls_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_MULTICAST_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [known_mpls_multicast_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [known_mpls_multicast_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_KNOWN_MIM_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [known_mim_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [known_mim_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MIM_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_mim_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_mim_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_KNOWN_TRILL_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [known_trill_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [known_trill_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_TRILL_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_trill_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_trill_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_KNOWN_NIV_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [known_niv_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [known_niv_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                     pkt_resolution_flags & 
                     BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_NIV_PKT) {
                     if (map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_niv_pkt] == 0xFF) {
                         temp_count = map_array[bcmStatGroupModeAttrPktType]
                                      [256];
                         map_array[bcmStatGroupModeAttrPktType]
                                  [unknown_niv_pkt] = temp_count;
                         map_array[bcmStatGroupModeAttrPktType][256] =
                                               temp_count+1;
                     }
                 }
             } 
             if (flex_attribute->svp != 0) {
                 if (map_array[bcmStatGroupModeAttrIngNetworkGroup] == NULL) {
                     map_array[bcmStatGroupModeAttrIngNetworkGroup] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrIngNetworkGroup],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrIngNetworkGroup][256]=0;
                 }
                 if (direction == bcmStatFlexDirectionIngress) {
                     max_count= (1 << ing_pkt_attr_cmprsd_bits_g.svp_type)-1;
                 } else {
                     max_count= (1 << egr_pkt_attr_cmprsd_bits_g.svp_type)-1;
                 }
                 for (loop_index=0; loop_index <= max_count ; loop_index++) {
                      if (BCM_STAT_FLEX_VALUE_GET(
                          flex_attribute->combine_attr_counter[counter].
                          value_array[bcmStatFlexAttrSvp],
                          loop_index)) {
                          if (map_array[bcmStatGroupModeAttrIngNetworkGroup]
                                       [loop_index] == 0xFF) {
                              temp_count = map_array
                                           [bcmStatGroupModeAttrIngNetworkGroup]
                                           [256];
                              map_array[bcmStatGroupModeAttrIngNetworkGroup]
                                       [loop_index] = temp_count;
                              map_array[bcmStatGroupModeAttrIngNetworkGroup]
                                       [256] = temp_count + 1;
                          }
                      }
                  }
             }
             if (flex_attribute->dvp != 0) {
                 if (map_array[bcmStatGroupModeAttrEgrNetworkGroup] == NULL) {
                     map_array[bcmStatGroupModeAttrEgrNetworkGroup] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrEgrNetworkGroup],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrEgrNetworkGroup][256]=0;
                 }
                 max_count= (1 << egr_pkt_attr_cmprsd_bits_g.dvp_type)-1;
                 for (loop_index=0; loop_index <= max_count ; loop_index++) {
                      if (BCM_STAT_FLEX_VALUE_GET(
                          flex_attribute->combine_attr_counter[counter].
                          value_array[bcmStatFlexAttrDvp],
                          loop_index)) {
                          if (map_array[bcmStatGroupModeAttrEgrNetworkGroup]
                                       [loop_index] == 0xFF) {
                              temp_count = map_array
                                           [bcmStatGroupModeAttrEgrNetworkGroup]
                                           [256];
                              map_array[bcmStatGroupModeAttrEgrNetworkGroup]
                                       [loop_index] = temp_count;
                              map_array[bcmStatGroupModeAttrEgrNetworkGroup]
                                       [256] = temp_count + 1;
                          }
                      }
                  }
             }
             if (flex_attribute->drop != 0) {
                 if (map_array[bcmStatGroupModeAttrDrop] == NULL) {
                     map_array[bcmStatGroupModeAttrDrop] = 
                               (uint8 *) sal_alloc(256+1,"map_array");
                    sal_memset(map_array[bcmStatGroupModeAttrDrop],
                               0xFF,256);
                    map_array[bcmStatGroupModeAttrDrop][256]=0;
                 }
                 if (flex_attribute->combine_attr_counter[counter].
                    drop_flags & BCM_STAT_FLEX_DROP_DISABLE) {
                    if (map_array[bcmStatGroupModeAttrDrop][0] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrDrop][256];
                        map_array[bcmStatGroupModeAttrDrop][0] = temp_count;
                        map_array[bcmStatGroupModeAttrDrop][256]=temp_count + 1;
                    }
                 } 
                 if (flex_attribute->combine_attr_counter[counter].
                     drop_flags & BCM_STAT_FLEX_DROP_ENABLE) {
                    if (map_array[bcmStatGroupModeAttrDrop][1] == 0xFF) {
                        temp_count = map_array[bcmStatGroupModeAttrDrop][256];
                        map_array[bcmStatGroupModeAttrDrop][1] = temp_count;
                        map_array[bcmStatGroupModeAttrDrop][256]=temp_count + 1;
                    }
                 } 
             }
             /* END: PKT_RES_MAP*/ 
        }
        if (map_array[bcmStatGroupModeAttrColor] != NULL) {
            pre_ifp_cng_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                        map_array[bcmStatGroupModeAttrColor][256]);
            if (pre_ifp_cng_cmprsd_max_bits > cng_bits) {
                pre_ifp_cng_cmprsd_max_bits = cng_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrFieldIngressColor] != NULL) {
            ifp_cng_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                    map_array[bcmStatGroupModeAttrFieldIngressColor][256]);
            if (ifp_cng_cmprsd_max_bits > ifp_cng_bits) {
                ifp_cng_cmprsd_max_bits = ifp_cng_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrIntPri] != NULL) {
            int_pri_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                    map_array[bcmStatGroupModeAttrIntPri][256]);
            if (int_pri_cmprsd_max_bits > int_pri_bits) {
                int_pri_cmprsd_max_bits = int_pri_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrVlan] != NULL) {
            vlan_format_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                        map_array[bcmStatGroupModeAttrVlan][256]);
            if (vlan_format_cmprsd_max_bits > vlan_format_bits) {
                vlan_format_cmprsd_max_bits = vlan_format_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrOuterPri] != NULL) {
            outer_dot1p_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                        map_array[bcmStatGroupModeAttrOuterPri][256]);
            if (outer_dot1p_cmprsd_max_bits > outer_dot1p_bits) {
                outer_dot1p_cmprsd_max_bits = outer_dot1p_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrInnerPri] != NULL) {
            inner_dot1p_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                        map_array[bcmStatGroupModeAttrInnerPri][256]);
            if (inner_dot1p_cmprsd_max_bits > inner_dot1p_bits) {
                inner_dot1p_cmprsd_max_bits = inner_dot1p_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrPort] != NULL) {
            port_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                 map_array[bcmStatGroupModeAttrPort][256]);
            if (port_cmprsd_max_bits > port_bits) {
                port_cmprsd_max_bits = port_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrTosDscp] != NULL) {
            tos_dscp_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                map_array[bcmStatGroupModeAttrTosDscp][256]);
            if (tos_dscp_cmprsd_max_bits > tos_dscp_bits) {
                tos_dscp_cmprsd_max_bits = tos_dscp_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrTosEcn] != NULL) {
            tos_ecn_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                map_array[bcmStatGroupModeAttrTosEcn][256]);
            if (tos_ecn_cmprsd_max_bits > tos_ecn_bits) {
                tos_ecn_cmprsd_max_bits = tos_ecn_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrPktType] != NULL) {
            pkt_resolution_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                           map_array[bcmStatGroupModeAttrPktType][256]);
            if (pkt_resolution_cmprsd_max_bits > pkt_resolution_bits) {
                pkt_resolution_cmprsd_max_bits = pkt_resolution_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrIngNetworkGroup] != NULL) {
            svp_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                map_array[bcmStatGroupModeAttrIngNetworkGroup][256]);
            if (svp_cmprsd_max_bits > svp_bits) {
                svp_cmprsd_max_bits = svp_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrEgrNetworkGroup] != NULL) {
            dvp_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                map_array[bcmStatGroupModeAttrEgrNetworkGroup][256]);
            if (dvp_cmprsd_max_bits > dvp_bits) {
                dvp_cmprsd_max_bits = dvp_bits;
            }
        }
        if (map_array[bcmStatGroupModeAttrDrop] != NULL) {
            drop_cmprsd_max_bits = _bcm_stat_flex_get_num_bits(
                 map_array[bcmStatGroupModeAttrDrop][256]);
            if (drop_cmprsd_max_bits > drop_bits) {
                drop_cmprsd_max_bits = drop_bits;
            }
        }
        if ((total_bits = 
             pre_ifp_cng_cmprsd_max_bits + ifp_cng_cmprsd_max_bits + 
             int_pri_cmprsd_max_bits + vlan_format_cmprsd_max_bits + 
             outer_dot1p_cmprsd_max_bits + inner_dot1p_cmprsd_max_bits +
             port_cmprsd_max_bits + 
             tos_dscp_cmprsd_max_bits + tos_ecn_cmprsd_max_bits + 
             pkt_resolution_cmprsd_max_bits + svp_cmprsd_max_bits + 
             dvp_cmprsd_max_bits + drop_cmprsd_max_bits) > BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS)  {
            LOG_CLI((BSL_META_U(unit,
                                "TotalBits=%d Even Compressed mode cannot be used\n"),
                     total_bits));
             for (loop=0;loop<(bcmStatGroupModeAttrPacketTypeIp+1);loop++) {
                  if (map_array[loop] != NULL) {
                      sal_free(map_array[loop]);
                  }
             }
             sal_free(attr);
             return BCM_E_PARAM;
        }
        /* Creating MAP..there will be some unnecessary entries but no harm */
        switch(direction) {
        case bcmStatFlexDirectionIngress:
            if ((flex_attribute->pre_ifp_color != 0) ||
                (flex_attribute->ifp_color != 0) || 
                (flex_attribute->int_pri != 0)) {
                 mapped_value0 = 0;
                 mapped_value1 = 0;
                 mapped_value2 = 0;
                 max_bits=0;
                 LOG_CLI((BSL_META_U(unit,
                                     "Ing: PRI_CNG_MAP Creation \n")));
                 if (map_array[bcmStatGroupModeAttrColor] != NULL) {
                     max_bits =  pre_ifp_cng_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->cng = max_bits;
                     ing_cmprsd_pkt_attr_bits->cng_mask = (1 << max_bits) - 1;
                     ing_cmprsd_pkt_attr_bits->cng_pos = ing_pkt_attr_cmprsd_bits_g.
                                                         cng_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrFieldIngressColor] != NULL) {
                     max_bits =  ifp_cng_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->ifp_cng = max_bits;
                     ing_cmprsd_pkt_attr_bits->ifp_cng_mask = (1 << max_bits)-1;
                     ing_cmprsd_pkt_attr_bits->ifp_cng_pos =  
                                               ing_pkt_attr_cmprsd_bits_g.ifp_cng_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrIntPri] != NULL) {
                     max_bits = int_pri_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->int_pri = max_bits;
                     ing_cmprsd_pkt_attr_bits->int_pri_mask = (1 << max_bits)-1;
                     ing_cmprsd_pkt_attr_bits->int_pri_pos= ing_pkt_attr_cmprsd_bits_g.
                                                            int_pri_pos;
                 }
                 for (index0 =0;
                      index0 < (1 << ing_pkt_attr_cmprsd_bits_g.cng);
                      index0++ ) {
                      if (map_array[bcmStatGroupModeAttrColor] != NULL) {
                          if (map_array[bcmStatGroupModeAttrColor][index0] ==
                              0xFF ) {
                              mapped_value0 = map_array
                                              [bcmStatGroupModeAttrColor][256];
                          } else {
                              mapped_value0 = map_array
                                            [bcmStatGroupModeAttrColor][index0];
                          }
                      }
                      for (index1 =0;
                           index1 < (1 << ing_pkt_attr_cmprsd_bits_g.ifp_cng);
                           index1++ ) {
                           if (map_array[bcmStatGroupModeAttrFieldIngressColor] 
                               != NULL) {
                               if (map_array
                                   [bcmStatGroupModeAttrFieldIngressColor]
                                   [index1] == 0xFF){
                                   mapped_value1 = map_array
                                         [bcmStatGroupModeAttrFieldIngressColor]
                                         [256];
                               } else {
                                   mapped_value1 = map_array
                                         [bcmStatGroupModeAttrFieldIngressColor]
                                         [index1];
                               }
                           }
                           for (index2 =0;
                                index2 < (1 << ing_pkt_attr_cmprsd_bits_g.int_pri);
                                index2++ ) {
                                if (map_array[bcmStatGroupModeAttrIntPri] 
                                    != NULL) {
                                    if (map_array
                                        [bcmStatGroupModeAttrIntPri][index2]
                                        == 0xFF){
                                        mapped_value2 = map_array
                                                   [bcmStatGroupModeAttrIntPri]
                                                   [256];
                                    } else {
                                        mapped_value2 = map_array
                                                   [bcmStatGroupModeAttrIntPri]
                                                   [index2];
                                    }
                                 }
                                    final_index =
                                     (index0 << 
                                      (ing_pkt_attr_cmprsd_bits_g.ifp_cng +
                                       ing_pkt_attr_cmprsd_bits_g.int_pri)) +
                                     (index1 << 
                                      (ing_pkt_attr_cmprsd_bits_g.int_pri)) +
                                     (index2) ;
                                    final_mapped_value =
                                     (mapped_value0 << 
                                      (ing_pkt_attr_cmprsd_bits_g.ifp_cng +
                                       ing_pkt_attr_cmprsd_bits_g.int_pri)) +
                                     (mapped_value1 << 
                                      (ing_pkt_attr_cmprsd_bits_g.int_pri)) +
                                     (mapped_value2) ;
                                    ing_cmprsd_attr_selectors->pri_cnf_attr_map
                                        [final_index] = final_mapped_value;
                                }
                           }
                      }
                 }
            if ((flex_attribute->vlan_format != 0) ||
                (flex_attribute->outer_dot1p != 0) || 
                (flex_attribute->inner_dot1p != 0)) {
                 mapped_value0 = 0;
                 mapped_value1 = 0;
                 mapped_value2 = 0;
                 max_bits=0;
                 LOG_CLI((BSL_META_U(unit,
                                     "Ing: PKT_PRI_MAP Creation \n")));
                 if (map_array[bcmStatGroupModeAttrVlan] != NULL) {
                     max_bits =  vlan_format_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->vlan_format = max_bits;
                     ing_cmprsd_pkt_attr_bits->vlan_format_mask = 
                                               (1 << max_bits) - 1;
                     ing_cmprsd_pkt_attr_bits->vlan_format_pos = 
                                               ing_pkt_attr_cmprsd_bits_g.
                                               vlan_format_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrOuterPri] != NULL) {
                     max_bits =  outer_dot1p_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->outer_dot1p = max_bits;
                     ing_cmprsd_pkt_attr_bits->outer_dot1p_mask=(1<<max_bits)-1;
                     ing_cmprsd_pkt_attr_bits->outer_dot1p_pos =  
                                        ing_pkt_attr_cmprsd_bits_g.outer_dot1p_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrInnerPri] != NULL) {
                     max_bits = inner_dot1p_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->inner_dot1p = max_bits;
                     ing_cmprsd_pkt_attr_bits->inner_dot1p_mask=(1<<max_bits)-1;
                     ing_cmprsd_pkt_attr_bits->inner_dot1p_pos= 
                                         ing_pkt_attr_cmprsd_bits_g.inner_dot1p_pos;
                 }
                 for (index0 =0;
                      index0 < (1 << ing_pkt_attr_cmprsd_bits_g.vlan_format);
                      index0++ ) {
                      if (map_array[bcmStatGroupModeAttrVlan] != NULL) {
                          if (map_array[bcmStatGroupModeAttrVlan][index0] ==
                              0xFF ) {
                              mapped_value0 = map_array
                                              [bcmStatGroupModeAttrVlan][256];
                          } else {
                              mapped_value0 = map_array
                                            [bcmStatGroupModeAttrVlan][index0];
                          }
                      }
                      for (index1 =0;
                           index1 < (1 << ing_pkt_attr_cmprsd_bits_g.outer_dot1p);
                           index1++ ) {
                           if (map_array[bcmStatGroupModeAttrOuterPri] 
                               != NULL) {
                               if (map_array
                                   [bcmStatGroupModeAttrOuterPri]
                                   [index1] == 0xFF){
                                   mapped_value1 = map_array
                                         [bcmStatGroupModeAttrOuterPri]
                                         [256];
                               } else {
                                   mapped_value1 = map_array
                                         [bcmStatGroupModeAttrOuterPri]
                                         [index1];
                               }
                           }
                           for (index2 =0;
                                index2 < (1 << ing_pkt_attr_cmprsd_bits_g.inner_dot1p);
                                index2++ ) {
                                if (map_array[bcmStatGroupModeAttrInnerPri] 
                                    != NULL) {
                                    if (map_array
                                        [bcmStatGroupModeAttrInnerPri][index2]
                                        == 0xFF){
                                        mapped_value2 = map_array
                                                [bcmStatGroupModeAttrInnerPri]
                                                [256];
                                    } else {
                                        mapped_value2 = map_array
                                               [bcmStatGroupModeAttrInnerPri]
                                               [index2];
                                    }
                                }
                                    final_index =
                                     (index0 << 
                                      (ing_pkt_attr_cmprsd_bits_g.outer_dot1p +
                                       ing_pkt_attr_cmprsd_bits_g.inner_dot1p)) +
                                     (index1 << 
                                      (ing_pkt_attr_cmprsd_bits_g.inner_dot1p)) +
                                     (index2) ;
                                    final_mapped_value =
                                     (mapped_value0 << 
                                      (ing_pkt_attr_cmprsd_bits_g.outer_dot1p +
                                       ing_pkt_attr_cmprsd_bits_g.inner_dot1p)) +
                                     (mapped_value1 << 
                                      (ing_pkt_attr_cmprsd_bits_g.inner_dot1p)) +
                                     (mapped_value2) ;
                                    ing_cmprsd_attr_selectors->pkt_pri_attr_map
                                        [final_index] = final_mapped_value;
                                }
                           }
                      }
                 }
             if (flex_attribute->port != 0) {
                 mapped_value0 = 0;
                 max_bits=0;
                 LOG_CLI((BSL_META_U(unit,
                                     "Ing: PORT_MAP Creation \n")));
                 if (map_array[bcmStatGroupModeAttrPort] != NULL) {
                     max_bits =  port_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->ing_port = max_bits;
                     ing_cmprsd_pkt_attr_bits->ing_port_mask = 
                                               (1 << max_bits) - 1;
                     ing_cmprsd_pkt_attr_bits->ing_port_pos = 
                                               ing_pkt_attr_cmprsd_bits_g.ing_port_pos;
                 }
                 for (index0 =0;
                      index0 < (1 << ing_pkt_attr_cmprsd_bits_g.ing_port);
                      index0++ ) {
                      if (map_array[bcmStatGroupModeAttrPort] != NULL) {
                          if (map_array[bcmStatGroupModeAttrPort][index0] ==
                              0xFF ) {
                              mapped_value0 = map_array
                                              [bcmStatGroupModeAttrPort][256];
                          } else {
                              mapped_value0 = map_array
                                            [bcmStatGroupModeAttrPort][index0];
                          }
                      }
                      final_index = index0 ;
                      final_mapped_value = mapped_value0 ;
                      ing_cmprsd_attr_selectors->port_attr_map[final_index] = 
                                                 final_mapped_value;
                 }
             }
             if ((flex_attribute->tos_dscp != 0) ||
                 (flex_attribute->tos_ecn != 0)) {
                 mapped_value0 = 0;
                 mapped_value1 = 0;
                 max_bits=0;
                 LOG_CLI((BSL_META_U(unit,
                                     "Ing: TOS_MAP Creation \n")));
                 if (map_array[bcmStatGroupModeAttrTosDscp] != NULL) {
                     max_bits =  tos_dscp_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->tos_dscp = max_bits;
                     ing_cmprsd_pkt_attr_bits->tos_dscp_mask = 
                                               (1 << max_bits) - 1;
                     ing_cmprsd_pkt_attr_bits->tos_dscp_pos = 
                                               ing_pkt_attr_cmprsd_bits_g.tos_dscp_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrTosEcn] != NULL) {
                     max_bits =  tos_ecn_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->tos_ecn = max_bits;
                     ing_cmprsd_pkt_attr_bits->tos_ecn_mask = 
                                               (1 << max_bits) - 1;
                     ing_cmprsd_pkt_attr_bits->tos_ecn_pos = 
                                               ing_pkt_attr_cmprsd_bits_g.tos_ecn_pos;
                 }
                 for (index0 =0;
                      index0 < (1 << ing_pkt_attr_cmprsd_bits_g.tos_dscp);
                      index0++ ) {
                      if (map_array[bcmStatGroupModeAttrTosDscp] != NULL) {
                          if (map_array[bcmStatGroupModeAttrTosDscp][index0] ==
                              0xFF ) {
                              mapped_value0 = map_array
                                              [bcmStatGroupModeAttrTosDscp][256];
                          } else {
                              mapped_value0 = map_array
                                            [bcmStatGroupModeAttrTosDscp][index0];
                          }
                          for (index1 =0;
                               index1 < (1 << ing_pkt_attr_cmprsd_bits_g.tos_ecn);
                               index1++ ) {
                               if (map_array[bcmStatGroupModeAttrTosEcn] != NULL) {
                                   if (map_array[bcmStatGroupModeAttrTosEcn]
                                                [index1] == 0xFF ) {
                                       mapped_value1 = map_array
                                              [bcmStatGroupModeAttrTosEcn][256];
                                   } else {
                                       mapped_value1 = map_array
                                              [bcmStatGroupModeAttrTosEcn][index1];
                                   }
                               }
                               final_index = (index0 << 
                                              ing_pkt_attr_cmprsd_bits_g.tos_ecn) + 
                                              index1;
                               final_mapped_value = (mapped_value0 <<
                                                     ing_pkt_attr_cmprsd_bits_g.tos_ecn) + 
                                                     mapped_value1;
                               ing_cmprsd_attr_selectors->tos_attr_map[final_index] = 
                                                 final_mapped_value;
                          } 
                      } 
                 }
             }
            if ((flex_attribute->pkt_resolution != 0) ||
                (flex_attribute->svp != 0) || 
                (flex_attribute->drop != 0)) {
                 mapped_value0 = 0;
                 mapped_value1 = 0;
                 mapped_value2 = 0;
                 max_bits=0;
                 LOG_CLI((BSL_META_U(unit,
                                     "Ing: PKT_RES_MAP Creation \n")));
                 if (map_array[bcmStatGroupModeAttrPktType] != NULL) {
                     max_bits =  pkt_resolution_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->pkt_resolution = max_bits;
                     ing_cmprsd_pkt_attr_bits->pkt_resolution_mask = 
                                               (1 << max_bits) - 1;
                     ing_cmprsd_pkt_attr_bits->pkt_resolution_pos = 
                                               ing_pkt_attr_cmprsd_bits_g.
                                               pkt_resolution_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrIngNetworkGroup] != NULL) {
                     max_bits =  svp_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->svp_type = max_bits;
                     ing_cmprsd_pkt_attr_bits->svp_type_mask=(1<<max_bits)-1;
                     ing_cmprsd_pkt_attr_bits->svp_type_pos =  
                                        ing_pkt_attr_cmprsd_bits_g.svp_type_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrDrop] != NULL) {
                     max_bits = drop_cmprsd_max_bits;
                     ing_cmprsd_pkt_attr_bits->drop = max_bits;
                     ing_cmprsd_pkt_attr_bits->drop_mask=(1<<max_bits)-1;
                     ing_cmprsd_pkt_attr_bits->drop_pos= 
                                         ing_pkt_attr_cmprsd_bits_g.drop_pos;
                 }
                 for (index0 =0;
                      index0 < (1 << ing_pkt_attr_cmprsd_bits_g.pkt_resolution);
                      index0++ ) {
                      if (map_array[bcmStatGroupModeAttrPktType] != NULL) {
                          if (map_array[bcmStatGroupModeAttrPktType][index0] ==
                              0xFF ) {
                              mapped_value0 = map_array
                                            [bcmStatGroupModeAttrPktType][256];
                          } else {
                              mapped_value0 = map_array
                                         [bcmStatGroupModeAttrPktType][index0];
                          }
                      }
                      for (index1 =0;
                           index1 < (1 << ing_pkt_attr_cmprsd_bits_g.svp_type);
                           index1++ ) {
                           if (map_array[bcmStatGroupModeAttrIngNetworkGroup] 
                               != NULL) {
                               if (map_array
                                   [bcmStatGroupModeAttrIngNetworkGroup]
                                   [index1] == 0xFF){
                                   mapped_value1 = map_array
                                         [bcmStatGroupModeAttrIngNetworkGroup]
                                         [256];
                               } else {
                                   mapped_value1 = map_array
                                         [bcmStatGroupModeAttrIngNetworkGroup]
                                         [index1];
                               }
                           }
                           for (index2 =0;
                                index2 < (1 << ing_pkt_attr_cmprsd_bits_g.drop);
                                index2++ ) {
                                if (map_array[bcmStatGroupModeAttrDrop] 
                                    != NULL) {
                                    if (map_array
                                        [bcmStatGroupModeAttrDrop][index2]
                                        == 0xFF){
                                        mapped_value2 = map_array
                                                [bcmStatGroupModeAttrDrop]
                                                [256];
                                    } else {
                                        mapped_value2 = map_array
                                               [bcmStatGroupModeAttrDrop]
                                               [index2];
                                    }
                                 }
                                    final_index =
                                     (index0 << 
                                      (ing_pkt_attr_cmprsd_bits_g.svp_type +
                                       ing_pkt_attr_cmprsd_bits_g.drop)) +
                                     (index1 << 
                                      (ing_pkt_attr_cmprsd_bits_g.drop)) +
                                     (index2) ;
                                    final_mapped_value =
                                     (mapped_value0 << 
                                      (ing_pkt_attr_cmprsd_bits_g.svp_type +
                                       ing_pkt_attr_cmprsd_bits_g.drop)) +
                                     (mapped_value1 << 
                                      (ing_pkt_attr_cmprsd_bits_g.drop)) +
                                     (mapped_value2) ;
                                    ing_cmprsd_attr_selectors->pkt_res_attr_map
                                        [final_index] = final_mapped_value;
                                }
                           }
                      }
                 }
             break; 
        case bcmStatFlexDirectionEgress:
            mapped_value0 = 0;
            mapped_value1 = 0;
            mapped_value2 = 0;
            max_bits=0;
            if ((flex_attribute->pre_ifp_color != 0) ||
                (flex_attribute->int_pri != 0)) {
                 LOG_CLI((BSL_META_U(unit,
                                     "Egr: PRI_CNG_MAP Creation \n")));
                 if (map_array[bcmStatGroupModeAttrColor] != NULL) {
                     max_bits =  pre_ifp_cng_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->cng = max_bits;
                     egr_cmprsd_pkt_attr_bits->cng_mask = (1 << max_bits) - 1;
                     egr_cmprsd_pkt_attr_bits->cng_pos = egr_pkt_attr_cmprsd_bits_g.
                                                         cng_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrIntPri] != NULL) {
                     max_bits = int_pri_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->int_pri = max_bits;
                     egr_cmprsd_pkt_attr_bits->int_pri_mask=(1 << max_bits) - 1;
                     egr_cmprsd_pkt_attr_bits->int_pri_pos= egr_pkt_attr_cmprsd_bits_g.
                                                            int_pri_pos;
                 }
                 for (index0 =0;
                      index0 < (1 << egr_pkt_attr_cmprsd_bits_g.cng);
                      index0++ ) {
                      if (map_array[bcmStatGroupModeAttrColor] != NULL) {
                          if (map_array[bcmStatGroupModeAttrColor][index0] ==
                              0xFF) {
                              mapped_value0 = map_array
                                              [bcmStatGroupModeAttrColor][256];
                          } else {
                              mapped_value0 = map_array
                                              [bcmStatGroupModeAttrColor]
                                              [index0];
                          }
                      }
                      for (index1 =0;
                           index1 < (1 << egr_pkt_attr_cmprsd_bits_g.int_pri);
                           index1++ ) {
                           if (map_array[bcmStatGroupModeAttrIntPri] != NULL) {
                               if (map_array[bcmStatGroupModeAttrIntPri]
                                   [index1] == 0xFF){
                                   mapped_value1 = map_array
                                              [bcmStatGroupModeAttrIntPri]
                                              [256];
                               } else {
                                   mapped_value1 = map_array
                                              [bcmStatGroupModeAttrIntPri]
                                              [index1];
                               }
                            }
                               final_index =
                                          (index0 << 
                                           (egr_pkt_attr_cmprsd_bits_g.int_pri)) +
                                          (index1) ;
                               final_mapped_value =
                                          (mapped_value0 << 
                                           (egr_pkt_attr_cmprsd_bits_g.int_pri)) +
                                          (mapped_value1) ;
                               egr_cmprsd_attr_selectors->pri_cnf_attr_map
                                        [final_index] = final_mapped_value;
                           }
                      }
                 }
             if ((flex_attribute->vlan_format != 0) ||
                 (flex_attribute->outer_dot1p != 0) || 
                 (flex_attribute->inner_dot1p != 0)) {
                 mapped_value0 = 0;
                 mapped_value1 = 0;
                 mapped_value2 = 0;
                 max_bits=0;
                 LOG_CLI((BSL_META_U(unit,
                                     "Egr: PKT_PRI_MAP Creation \n")));
                 if (map_array[bcmStatGroupModeAttrVlan] != NULL) {
                     max_bits =  vlan_format_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->vlan_format = max_bits;
                     egr_cmprsd_pkt_attr_bits->vlan_format_mask = 
                                               (1 << max_bits) - 1;
                     egr_cmprsd_pkt_attr_bits->vlan_format_pos = 
                                               egr_pkt_attr_cmprsd_bits_g.
                                               vlan_format_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrOuterPri] != NULL) {
                     max_bits =  outer_dot1p_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->outer_dot1p = max_bits;
                     egr_cmprsd_pkt_attr_bits->outer_dot1p_mask=(1<<max_bits)-1;
                     egr_cmprsd_pkt_attr_bits->outer_dot1p_pos =  
                                        egr_pkt_attr_cmprsd_bits_g.outer_dot1p_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrInnerPri] != NULL) {
                     max_bits = inner_dot1p_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->inner_dot1p = max_bits;
                     egr_cmprsd_pkt_attr_bits->inner_dot1p_mask=(1<<max_bits)-1;
                     egr_cmprsd_pkt_attr_bits->inner_dot1p_pos= 
                                         egr_pkt_attr_cmprsd_bits_g.inner_dot1p_pos;
                 }
                 for (index0 =0;
                      index0 < (1 << egr_pkt_attr_cmprsd_bits_g.vlan_format);
                      index0++ ) {
                      if (map_array[bcmStatGroupModeAttrVlan] != NULL) {
                          if (map_array[bcmStatGroupModeAttrVlan][index0] ==
                              0xFF ) {
                              mapped_value0 = map_array
                                              [bcmStatGroupModeAttrVlan][256];
                          } else {
                              mapped_value0 = map_array
                                            [bcmStatGroupModeAttrVlan][index0];
                          }
                      }
                      for (index1 =0;
                           index1 < (1 << egr_pkt_attr_cmprsd_bits_g.outer_dot1p);
                           index1++ ) {
                           if (map_array[bcmStatGroupModeAttrOuterPri] 
                               != NULL) {
                               if (map_array
                                   [bcmStatGroupModeAttrOuterPri]
                                   [index1] == 0xFF){
                                   mapped_value1 = map_array
                                         [bcmStatGroupModeAttrOuterPri]
                                         [256];
                               } else {
                                   mapped_value1 = map_array
                                         [bcmStatGroupModeAttrOuterPri]
                                         [index1];
                               }
                           }
                           for (index2 =0;
                                index2 < (1 << egr_pkt_attr_cmprsd_bits_g.inner_dot1p);
                                index2++ ) {
                                if (map_array[bcmStatGroupModeAttrInnerPri] 
                                    != NULL) {
                                    if (map_array
                                        [bcmStatGroupModeAttrInnerPri][index2]
                                        == 0xFF){
                                        mapped_value2 = map_array
                                                [bcmStatGroupModeAttrInnerPri]
                                                [256];
                                    } else {
                                        mapped_value2 = map_array
                                               [bcmStatGroupModeAttrInnerPri]
                                               [index2];
                                    }
                                }   
                                    final_index =
                                     (index0 << 
                                      (egr_pkt_attr_cmprsd_bits_g.outer_dot1p +
                                       egr_pkt_attr_cmprsd_bits_g.inner_dot1p)) +
                                     (index1 << 
                                      (egr_pkt_attr_cmprsd_bits_g.inner_dot1p)) +
                                     (index2) ;
                                    final_mapped_value =
                                     (mapped_value0 << 
                                      (egr_pkt_attr_cmprsd_bits_g.outer_dot1p +
                                       egr_pkt_attr_cmprsd_bits_g.inner_dot1p)) +
                                     (mapped_value1 << 
                                      (egr_pkt_attr_cmprsd_bits_g.inner_dot1p)) +
                                     (mapped_value2) ;
                                    egr_cmprsd_attr_selectors->pkt_pri_attr_map
                                        [final_index] = final_mapped_value;
                                }
                           }
                      }
                 }
             if (flex_attribute->port != 0) {
                 mapped_value0 = 0;
                 max_bits=0;
                 LOG_CLI((BSL_META_U(unit,
                                     "Egr: PORT_MAP Creation \n")));
                 if (map_array[bcmStatGroupModeAttrPort] != NULL) {
                     max_bits =  port_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->egr_port = max_bits;
                     egr_cmprsd_pkt_attr_bits->egr_port_mask = 
                                               (1 << max_bits) - 1;
                     egr_cmprsd_pkt_attr_bits->egr_port_pos = 
                                               egr_pkt_attr_cmprsd_bits_g.egr_port_pos;
                 }
                 for (index0 =0;
                      index0 < (1 << egr_pkt_attr_cmprsd_bits_g.egr_port);
                      index0++ ) {
                      if (map_array[bcmStatGroupModeAttrPort] != NULL) {
                          if (map_array[bcmStatGroupModeAttrPort][index0] ==
                              0xFF ) {
                              mapped_value0 = map_array
                                              [bcmStatGroupModeAttrPort][256];
                          } else {
                              mapped_value0 = map_array
                                            [bcmStatGroupModeAttrPort][index0];
                          }
                      }
                      final_index = index0 ;
                      final_mapped_value = mapped_value0 ;
                      egr_cmprsd_attr_selectors->port_attr_map[final_index] = 
                                                 final_mapped_value;
                 }
             }
             if ((flex_attribute->tos_dscp != 0) ||
                 (flex_attribute->tos_ecn != 0)) {
                 mapped_value0 = 0;
                 mapped_value1 = 0;
                 max_bits=0;
                 LOG_CLI((BSL_META_U(unit,
                                     "Egr: TOS_MAP Creation \n")));
                 if (map_array[bcmStatGroupModeAttrTosDscp] != NULL) {
                     max_bits =  tos_dscp_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->tos_dscp = max_bits;
                     egr_cmprsd_pkt_attr_bits->tos_dscp_mask = 
                                               (1 << max_bits) - 1;
                     egr_cmprsd_pkt_attr_bits->tos_dscp_pos = 
                                               egr_pkt_attr_cmprsd_bits_g.tos_dscp_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrTosEcn] != NULL) {
                     max_bits =  tos_ecn_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->tos_ecn = max_bits;
                     egr_cmprsd_pkt_attr_bits->tos_ecn_mask = 
                                               (1 << max_bits) - 1;
                     egr_cmprsd_pkt_attr_bits->tos_ecn_pos = 
                                               egr_pkt_attr_cmprsd_bits_g.tos_ecn_pos;
                 }
                 for (index0 =0;
                      index0 < (1 << egr_pkt_attr_cmprsd_bits_g.tos_dscp);
                      index0++ ) {
                      if (map_array[bcmStatGroupModeAttrTosDscp] != NULL) {
                          if (map_array[bcmStatGroupModeAttrTosDscp][index0] ==
                              0xFF ) {
                              mapped_value0 = map_array
                                              [bcmStatGroupModeAttrTosDscp][256];
                          } else {
                              mapped_value0 = map_array
                                            [bcmStatGroupModeAttrTosDscp][index0];
                          }
                          for (index1 =0;
                               index1 < (1 << egr_pkt_attr_cmprsd_bits_g.tos_ecn);
                               index1++ ) {
                               if (map_array[bcmStatGroupModeAttrTosEcn] != NULL) {
                                   if (map_array[bcmStatGroupModeAttrTosEcn]
                                                [index1] == 0xFF ) {
                                       mapped_value1 = map_array
                                              [bcmStatGroupModeAttrTosEcn][256];
                                   } else {
                                       mapped_value1 = map_array
                                              [bcmStatGroupModeAttrTosEcn][index1];
                                   }
                               }
                               final_index = (index0 << 
                                              egr_pkt_attr_cmprsd_bits_g.tos_ecn) + 
                                              index1;
                               final_mapped_value = (mapped_value0 <<
                                                     egr_pkt_attr_cmprsd_bits_g.tos_ecn) + 
                                                     mapped_value1;
                               egr_cmprsd_attr_selectors->tos_attr_map[final_index] = 
                                                 final_mapped_value;
                          } 
                      } 
                 }
             }
            if ((flex_attribute->pkt_resolution != 0) ||
                (flex_attribute->svp != 0) || 
                (flex_attribute->dvp != 0) || 
                (flex_attribute->drop != 0)) {
                 mapped_value0 = 0;
                 mapped_value1 = 0;
                 mapped_value2 = 0;
                 mapped_value3 = 0;
                 max_bits=0;
                 LOG_CLI((BSL_META_U(unit,
                                     "Egr: PKT_RES_MAP Creation \n")));
                 if (map_array[bcmStatGroupModeAttrPktType] != NULL) {
                     max_bits =  pkt_resolution_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->pkt_resolution = max_bits;
                     egr_cmprsd_pkt_attr_bits->pkt_resolution_mask = 
                                               (1 << max_bits) - 1;
                     egr_cmprsd_pkt_attr_bits->pkt_resolution_pos = 
                                               egr_pkt_attr_cmprsd_bits_g.
                                               pkt_resolution_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrIngNetworkGroup] != NULL) {
                     max_bits =  svp_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->svp_type = max_bits;
                     egr_cmprsd_pkt_attr_bits->svp_type_mask=(1<<max_bits)-1;
                     egr_cmprsd_pkt_attr_bits->svp_type_pos =  
                                        egr_pkt_attr_cmprsd_bits_g.svp_type_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrEgrNetworkGroup] != NULL) {
                     max_bits =  dvp_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->dvp_type = max_bits;
                     egr_cmprsd_pkt_attr_bits->dvp_type_mask=(1<<max_bits)-1;
                     egr_cmprsd_pkt_attr_bits->dvp_type_pos =  
                                        egr_pkt_attr_cmprsd_bits_g.dvp_type_pos;
                 }
                 if (map_array[bcmStatGroupModeAttrDrop] != NULL) {
                     max_bits = drop_cmprsd_max_bits;
                     egr_cmprsd_pkt_attr_bits->drop = max_bits;
                     egr_cmprsd_pkt_attr_bits->drop_mask=(1<<max_bits)-1;
                     egr_cmprsd_pkt_attr_bits->drop_pos= 
                                         egr_pkt_attr_cmprsd_bits_g.drop_pos;
                 }
                 for (index0 =0;
                      index0 < (1 << egr_pkt_attr_cmprsd_bits_g.pkt_resolution);
                      index0++ ) {
                      if (map_array[bcmStatGroupModeAttrPktType] != NULL) {
                          if (map_array[bcmStatGroupModeAttrPktType][index0] ==
                              0xFF ) {
                              mapped_value0 = map_array
                                            [bcmStatGroupModeAttrPktType][256];
                          } else {
                              mapped_value0 = map_array
                                         [bcmStatGroupModeAttrPktType][index0];
                          }
                      }
                      for (index1 =0;
                           index1 < (1 << egr_pkt_attr_cmprsd_bits_g.svp_type);
                           index1++ ) {
                           if (map_array[bcmStatGroupModeAttrIngNetworkGroup] 
                               != NULL) {
                               if (map_array
                                   [bcmStatGroupModeAttrIngNetworkGroup]
                                   [index1] == 0xFF){
                                   mapped_value1 = map_array
                                         [bcmStatGroupModeAttrIngNetworkGroup]
                                         [256];
                               } else {
                                   mapped_value1 = map_array
                                         [bcmStatGroupModeAttrIngNetworkGroup]
                                         [index1];
                               }
                           }
                           for (index2 =0;
                                index2 < (1 << egr_pkt_attr_cmprsd_bits_g.dvp_type);
                                index2++ ) {
                                if (map_array
                                    [bcmStatGroupModeAttrEgrNetworkGroup] 
                                    != NULL) {
                                    if (map_array
                                        [bcmStatGroupModeAttrEgrNetworkGroup]
                                        [index2] == 0xFF){
                                        mapped_value2 = map_array
                                         [bcmStatGroupModeAttrEgrNetworkGroup]
                                         [256];
                                    } else {
                                        mapped_value2 = map_array
                                         [bcmStatGroupModeAttrEgrNetworkGroup]
                                         [index2];
                                    }
                                }
                                for (index3 =0;
                                     index3 < (1 << egr_pkt_attr_cmprsd_bits_g.drop);
                                     index3++ ) {
                                     if (map_array[bcmStatGroupModeAttrDrop] 
                                         != NULL) {
                                         if (map_array
                                             [bcmStatGroupModeAttrDrop][index3]
                                             == 0xFF){
                                             mapped_value3 = map_array
                                                [bcmStatGroupModeAttrDrop]
                                                [256];
                                         } else {
                                              mapped_value3 = map_array
                                               [bcmStatGroupModeAttrDrop]
                                               [index3];
                                         }
                                     }
                                         final_index =
                                          (index0 << 
                                           (egr_pkt_attr_cmprsd_bits_g.svp_type +
                                            egr_pkt_attr_cmprsd_bits_g.dvp_type +
                                            egr_pkt_attr_cmprsd_bits_g.drop)) +
                                          (index1 << 
                                           (egr_pkt_attr_cmprsd_bits_g.dvp_type +
                                            egr_pkt_attr_cmprsd_bits_g.drop)) +
                                          (index2 << 
                                            egr_pkt_attr_cmprsd_bits_g.drop) +
                                          (index3) ;
                                         final_mapped_value =
                                          (mapped_value0 << 
                                           (egr_pkt_attr_cmprsd_bits_g.svp_type +
                                            egr_pkt_attr_cmprsd_bits_g.dvp_type +
                                            egr_pkt_attr_cmprsd_bits_g.drop)) +
                                          (mapped_value1 << 
                                           (egr_pkt_attr_cmprsd_bits_g.dvp_type +
                                            egr_pkt_attr_cmprsd_bits_g.drop)) +
                                          (mapped_value2 <<
                                            egr_pkt_attr_cmprsd_bits_g.drop) +
                                          (mapped_value3) ;
                                    egr_cmprsd_attr_selectors->pkt_res_attr_map
                                        [final_index] = final_mapped_value;
                                     }
                                }
                           }
                      }
                 }
             break;
        default:
             break;
        }
       for (loop=0;loop<8;loop++) {
            offset_array[loop] = (uint8 *) sal_alloc(256+1,"offset_array");
       }
       /* Second Level Analysis: Update offset_array */
       for (counter=0;counter < flex_attribute->total_counters ; counter ++ ) {
            for (loop=0;loop<8;loop++) {
                 sal_memset(offset_array[loop],0,256+1);
            }
            /* Logical Group0 */
            loop = 0;
            if (flex_attribute->pre_ifp_color != 0) {
                shift_by_bits = ifp_cng_cmprsd_max_bits +
                                int_pri_cmprsd_max_bits + 
                                vlan_format_cmprsd_max_bits + 
                                outer_dot1p_cmprsd_max_bits + 
                                inner_dot1p_cmprsd_max_bits + 
                                port_cmprsd_max_bits + 
                                tos_dscp_cmprsd_max_bits + 
                                tos_ecn_cmprsd_max_bits + 
                                pkt_resolution_cmprsd_max_bits + 
                                svp_cmprsd_max_bits + 
                                dvp_cmprsd_max_bits + 
                                drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    pre_ifp_color_flags & BCM_STAT_FLEX_COLOR_GREEN) {
                    temp_count = offset_array[loop][0];
                                                       /* GreenIndex=0*/
                    offset_array[loop][temp_count+1] = (
                     map_array[bcmStatGroupModeAttrColor][0] << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pre_ifp_color_flags & BCM_STAT_FLEX_COLOR_RED) {
                    temp_count = offset_array[loop][0];
                                                      /* RedIndex=1 */
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrColor][1] << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pre_ifp_color_flags & BCM_STAT_FLEX_COLOR_YELLOW) {
                    temp_count = offset_array[loop][0];
                                                      /* YellowIndex=3 */
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrColor][3] << shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            if (flex_attribute->ifp_color != 0) {
                shift_by_bits = int_pri_cmprsd_max_bits + 
                                vlan_format_cmprsd_max_bits + 
                                outer_dot1p_cmprsd_max_bits + 
                                inner_dot1p_cmprsd_max_bits + 
                                port_cmprsd_max_bits + 
                                tos_dscp_cmprsd_max_bits + 
                                tos_ecn_cmprsd_max_bits + 
                                pkt_resolution_cmprsd_max_bits + 
                                svp_cmprsd_max_bits + 
                                dvp_cmprsd_max_bits + 
                                drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    ifp_color_flags & BCM_STAT_FLEX_COLOR_GREEN) {
                    temp_count = offset_array[loop][0];
                                                       /* GreenIndex=0*/
                    offset_array[loop][temp_count+1] = (
                     map_array[bcmStatGroupModeAttrFieldIngressColor][0] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    ifp_color_flags & BCM_STAT_FLEX_COLOR_RED) {
                    temp_count = offset_array[loop][0];
                                                      /* RedIndex=1 */
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrFieldIngressColor][1] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    ifp_color_flags & BCM_STAT_FLEX_COLOR_YELLOW) {
                    temp_count = offset_array[loop][0];
                                                      /* YellowIndex=3 */
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrFieldIngressColor][3] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            if (flex_attribute->int_pri != 0) {
                shift_by_bits = vlan_format_cmprsd_max_bits + 
                                outer_dot1p_cmprsd_max_bits + 
                                inner_dot1p_cmprsd_max_bits + 
                                port_cmprsd_max_bits + 
                                tos_dscp_cmprsd_max_bits + 
                                tos_ecn_cmprsd_max_bits + 
                                pkt_resolution_cmprsd_max_bits + 
                                svp_cmprsd_max_bits + 
                                dvp_cmprsd_max_bits + 
                                drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI0) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][0] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI1) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][1] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI2) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][2] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI3) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][3] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI4) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][4] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI5) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][5] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI6) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][6] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI7) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][7] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI8) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][8] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI9) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][9] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI10) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][10] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI11) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][11] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI12) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][12] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI13) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][13] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI14) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][14] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    int_pri_flags & BCM_STAT_FLEX_PRI15) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrIntPri][15] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            /* LogicalGroup1 */
            if (flex_attribute->vlan_format != 0) {
                shift_by_bits = outer_dot1p_cmprsd_max_bits + 
                                inner_dot1p_cmprsd_max_bits + 
                                port_cmprsd_max_bits + 
                                tos_dscp_cmprsd_max_bits + 
                                tos_ecn_cmprsd_max_bits + 
                                pkt_resolution_cmprsd_max_bits + 
                                svp_cmprsd_max_bits + 
                                dvp_cmprsd_max_bits + 
                                drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_UNTAGGED) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrVlan][0] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_INNER) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrVlan][1] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_OUTER) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrVlan][2] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    vlan_format_flags & BCM_STAT_FLEX_VLAN_FORMAT_BOTH) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrVlan][3] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            if (flex_attribute->outer_dot1p != 0) {
                shift_by_bits = inner_dot1p_cmprsd_max_bits + 
                                port_cmprsd_max_bits + 
                                tos_dscp_cmprsd_max_bits + 
                                tos_ecn_cmprsd_max_bits + 
                                pkt_resolution_cmprsd_max_bits + 
                                svp_cmprsd_max_bits + 
                                dvp_cmprsd_max_bits + 
                                drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI0) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrOuterPri][0] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI1) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrOuterPri][1] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI2) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrOuterPri][2] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI3) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrOuterPri][3] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI4) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrOuterPri][4] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI5) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrOuterPri][5] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI6) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrOuterPri][6] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    outer_dot1p_flags & BCM_STAT_FLEX_PRI7) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrOuterPri][7] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            if (flex_attribute->inner_dot1p != 0) {
                shift_by_bits = port_cmprsd_max_bits + 
                                tos_dscp_cmprsd_max_bits + 
                                tos_ecn_cmprsd_max_bits + 
                                pkt_resolution_cmprsd_max_bits + 
                                svp_cmprsd_max_bits + 
                                dvp_cmprsd_max_bits + 
                                drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI0) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrInnerPri][0] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI1) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrInnerPri][1] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI2) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrInnerPri][2] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI3) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrInnerPri][3] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI4) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrInnerPri][4] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI5) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrInnerPri][5] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI6) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrInnerPri][6] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    inner_dot1p_flags & BCM_STAT_FLEX_PRI7) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                     map_array[bcmStatGroupModeAttrInnerPri][7] << 
                     shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            /* LogicalGroup2 */
            if (flex_attribute->port != 0) {
                shift_by_bits = tos_dscp_cmprsd_max_bits + 
                                tos_ecn_cmprsd_max_bits + 
                                pkt_resolution_cmprsd_max_bits + 
                                svp_cmprsd_max_bits + 
                                dvp_cmprsd_max_bits + 
                                drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                if (direction == bcmStatFlexDirectionIngress) {
                    max_count= (1 << ing_pkt_attr_cmprsd_bits_g.ing_port) - 1;
                } else {
                    max_count= (1 << egr_pkt_attr_cmprsd_bits_g.egr_port) - 1;
                }
                for (loop_index=0; loop_index <= max_count ; loop_index++) { 
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrPort],loop_index)) {
                        temp_count = offset_array[loop][0];
                        offset_array[loop][temp_count+1]= (
                             map_array[bcmStatGroupModeAttrPort][loop_index] << 
                                                          shift_by_bits); 
                        offset_array[loop][0] = temp_count+1;
                    } 
                }
                loop++;
            }
            /* LogicalGroup3 */
            if ((flex_attribute->tos_dscp != 0) &&
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = tos_ecn_cmprsd_max_bits +
                                pkt_resolution_cmprsd_max_bits + 
                                svp_cmprsd_max_bits + 
                                dvp_cmprsd_max_bits + 
                                drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                if (direction == bcmStatFlexDirectionIngress) {
                   max_count= (1 << ing_pkt_attr_cmprsd_bits_g.tos_dscp) - 1;
                } else {
                   max_count= (1 << egr_pkt_attr_cmprsd_bits_g.tos_dscp) - 1;
                }
                for (loop_index=0; loop_index <= max_count ; loop_index++) { 
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrTosDscp],loop_index)) {
                        temp_count = offset_array[loop][0];
                        offset_array[loop][temp_count+1]= (
                             map_array[bcmStatGroupModeAttrTosDscp][loop_index] << 
                                                          shift_by_bits); 
                        offset_array[loop][0] = temp_count+1;
                    } 
                }
                loop++;
            }
            if ((flex_attribute->tos_ecn != 0) &&
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = pkt_resolution_cmprsd_max_bits + 
                                svp_cmprsd_max_bits + 
                                dvp_cmprsd_max_bits + 
                                drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                if (direction == bcmStatFlexDirectionIngress) {
                   max_count= (1 << ing_pkt_attr_cmprsd_bits_g.tos_ecn) - 1;
                } else {
                   max_count= (1 << egr_pkt_attr_cmprsd_bits_g.tos_ecn) - 1;
                }
                for (loop_index=0; loop_index <= max_count ; loop_index++) { 
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrTosEcn],loop_index)) {
                        temp_count = offset_array[loop][0];
                        offset_array[loop][temp_count+1]= (
                             map_array[bcmStatGroupModeAttrTosEcn][loop_index] << 
                                                          shift_by_bits); 
                        offset_array[loop][0] = temp_count+1;
                    } 
                }
                loop++;
            }
            /* LogicalGroup4 */
            if ((flex_attribute->pkt_resolution != 0) &&
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = svp_cmprsd_max_bits + 
                                dvp_cmprsd_max_bits + 
                                drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                             map_array[bcmStatGroupModeAttrPktType][unknown_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_CONTROL_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                             map_array[bcmStatGroupModeAttrPktType][control_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_OAM_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                             map_array[bcmStatGroupModeAttrPktType][oam_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_BFD_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                             map_array[bcmStatGroupModeAttrPktType][bfd_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_BPDU_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                             map_array[bcmStatGroupModeAttrPktType][bpdu_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_ICNM_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                             map_array[bcmStatGroupModeAttrPktType][icnm_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_PKT_IS_1588) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                             map_array[bcmStatGroupModeAttrPktType][_1588_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_BFD_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                             map_array[bcmStatGroupModeAttrPktType][bfd_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2UC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                          map_array[bcmStatGroupModeAttrPktType][known_l2uc_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2UC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                        map_array[bcmStatGroupModeAttrPktType][unknown_l2uc_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & BCM_STAT_FLEX_PKT_TYPE_L2BC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                        map_array[bcmStatGroupModeAttrPktType][l2bc_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2MC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                        map_array[bcmStatGroupModeAttrPktType][known_l2mc_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2MC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                        map_array[bcmStatGroupModeAttrPktType][unknown_l2mc_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_L3UC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                        map_array[bcmStatGroupModeAttrPktType][known_l3uc_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L3UC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                        map_array[bcmStatGroupModeAttrPktType][unknown_l3uc_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_IPMC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                        map_array[bcmStatGroupModeAttrPktType][known_ipmc_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_IPMC_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                        map_array[bcmStatGroupModeAttrPktType][unknown_ipmc_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L2_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                      map_array[bcmStatGroupModeAttrPktType][known_mpls_l2_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L3_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                      map_array[bcmStatGroupModeAttrPktType][known_mpls_l3_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                      map_array[bcmStatGroupModeAttrPktType][known_mpls_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MPLS_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                      map_array[bcmStatGroupModeAttrPktType][unknown_mpls_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_MULTICAST_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
               map_array[bcmStatGroupModeAttrPktType][known_mpls_multicast_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_MIM_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                 map_array[bcmStatGroupModeAttrPktType][known_mim_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MIM_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                 map_array[bcmStatGroupModeAttrPktType][unknown_mim_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_TRILL_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                 map_array[bcmStatGroupModeAttrPktType][known_trill_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_TRILL_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                 map_array[bcmStatGroupModeAttrPktType][unknown_trill_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_KNOWN_NIV_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                 map_array[bcmStatGroupModeAttrPktType][known_niv_pkt]<< 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    pkt_resolution_flags & 
                    BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_NIV_PKT) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                 map_array[bcmStatGroupModeAttrPktType][unknown_niv_pkt] << 
                                                          shift_by_bits); 
                    offset_array[loop][0] = temp_count+1;
                }
                loop++;
            }
            /* Logical Group 5 */
            if ((flex_attribute->svp != 0) &&
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = dvp_cmprsd_max_bits + 
                                drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                if (direction == bcmStatFlexDirectionIngress) {
                    max_count= (1 << ing_pkt_attr_cmprsd_bits_g.svp_type)-1;
                } else {
                    max_count= (1 << egr_pkt_attr_cmprsd_bits_g.svp_type)-1;
                }
                for (loop_index=0; loop_index <= max_count ; loop_index++) { 
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrSvp],loop_index)) {
                        temp_count = offset_array[loop][0];
                        offset_array[loop][temp_count+1]= (
                 map_array[bcmStatGroupModeAttrIngNetworkGroup][loop_index] << 
                                                          shift_by_bits); 
                        offset_array[loop][0] = temp_count+1;
                    } 
                }
                loop++;
            }
            if ((flex_attribute->dvp != 0) &&
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = drop_cmprsd_max_bits + 
                                ip_pkt_bits ;
                max_count= (1 << egr_pkt_attr_cmprsd_bits_g.dvp_type)-1;
                for (loop_index=0; loop_index <= max_count ; loop_index++) { 
                    if (BCM_STAT_FLEX_VALUE_GET(
                            flex_attribute->combine_attr_counter[counter].
                            value_array[bcmStatFlexAttrDvp],loop_index)) {
                        temp_count = offset_array[loop][0];
                        offset_array[loop][temp_count+1]= (
                 map_array[bcmStatGroupModeAttrEgrNetworkGroup][loop_index] << 
                                                          shift_by_bits); 
                        offset_array[loop][0] = temp_count+1;
                    } 
                }
                loop++;
            }
            if ((flex_attribute->drop != 0) &&
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = ip_pkt_bits ;
                if (flex_attribute->combine_attr_counter[counter].
                    drop_flags & BCM_STAT_FLEX_DROP_DISABLE) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                 map_array[bcmStatGroupModeAttrDrop][0] << shift_by_bits); 
                    offset_array[loop][0] = temp_count;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    drop_flags & BCM_STAT_FLEX_DROP_ENABLE) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (
                 map_array[bcmStatGroupModeAttrDrop][1] << shift_by_bits); 
                    offset_array[loop][0] = temp_count;
                }
                loop++;
            }
            if ((flex_attribute->ip_pkt != 0) &&
                (loop <= (BCM_STAT_FLEX_COUNTER_MAX_TOTAL_BITS - 2))) {
                shift_by_bits = 0;
                if (flex_attribute->combine_attr_counter[counter].
                    ip_pkt_flags & BCM_STAT_FLEX_IP_PKT_DISABLE) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (0 << shift_by_bits);
                    offset_array[loop][0] = temp_count;
                }
                if (flex_attribute->combine_attr_counter[counter].
                    ip_pkt_flags & BCM_STAT_FLEX_IP_PKT_ENABLE) {
                    temp_count = offset_array[loop][0];
                    offset_array[loop][temp_count+1]= (1 << shift_by_bits);
                    offset_array[loop][0] = temp_count;
                }
                loop++;
            }
            /* Third and Final Level Update: Make Absolute Offset */
            for (index0=0;
                 index0 < offset_array[0][0] ; 
                 index0++) {
                 offset = offset_array[0][index0+1];
                 if (offset_array[1][0] != 0) {
                     for (index1=0;
                          index1 < offset_array[1][0] ; 
                          index1++) {
                          offset = offset_array[0][index0+1] | offset_array[1][index1+1];
                          if (offset_array[2][0] != 0) {
                              for (index2=0;
                                   index2 < offset_array[2][0] ; 
                                   index2++) {
                                   offset = offset_array[0][index0+1] | 
                                            offset_array[1][index1+1] |
                                            offset_array[2][index2+1];
                                   if (offset_array[3][0] != 0) {
                                       for (index3=0;
                                            index3 < offset_array[3][0] ; 
                                            index3++) {
                                            offset =  offset_array[0][index0+1] | 
                                                      offset_array[1][index1+1] |
                                                      offset_array[2][index2+1] |
                                                      offset_array[3][index3+1];
                                            if (offset_array[4][0] != 0) {
                                                for (index4=0;
                                                     index4 < offset_array[4][0] ; 
                                                     index4++) {
                                                     offset =  offset_array[0][index0+1] | 
                                                               offset_array[1][index1+1] |
                                                               offset_array[2][index2+1] |
                                                               offset_array[3][index3+1] |
                                                               offset_array[4][index4+1];
                                                     if (offset_array[5][0] != 0) {
                                                         for (index5=0;
                                                              index5 < offset_array[5][0] ; 
                                                              index5++) {
                                                              offset =  offset_array[0][index0+1] | 
                                                                        offset_array[1][index1+1] |
                                                                        offset_array[2][index2+1] |
                                                                        offset_array[3][index3+1] |
                                                                        offset_array[4][index4+1] |
                                                                        offset_array[5][index5+1];
                                                              if (offset_array[6][0] != 0) {
                                                                  for (index6=0;
                                                                       index6 < offset_array[6][0] ; 
                                                                       index6++) {
                                                                       offset =  offset_array[0][index0+1] | 
                                                                                 offset_array[1][index1+1] |
                                                                                 offset_array[2][index2+1] |
                                                                                 offset_array[3][index3+1] |
                                                                                 offset_array[4][index4+1] |
                                                                                 offset_array[5][index5+1] |
                                                                                 offset_array[6][index6+1];
                                                                       if (offset_array[7][0] != 0) {
                                                                           for (index7=0;
                                                                                index7 < offset_array[7][0] ; 
                                                                                index7++) {
                                                                                offset =  offset_array[0][index0+1] | 
                                                                                          offset_array[1][index1+1] |
                                                                                          offset_array[2][index2+1] |
                                                                                          offset_array[3][index3+1] |
                                                                                          offset_array[4][index4+1] |
                                                                                          offset_array[5][index5+1] |
                                                                                          offset_array[6][index6+1] |
                                                                                          offset_array[7][index6+1];
                                                                                _bcm_stat_flex_update_map(direction,attr,
                                                                                                          offset,counter);
                                                                           }
                                                                       } else {
                                                                           _bcm_stat_flex_update_map(direction,attr,
                                                                                                     offset,counter);
                                                                       }
                                                                  }
                                                              } else {
                                                                  _bcm_stat_flex_update_map(direction,attr,offset,counter);
                                                              }
                                                         }
                                                     } else {
                                                         _bcm_stat_flex_update_map(direction,attr,offset,counter);
                                                     }
                                                }
                                            } else {
                                                _bcm_stat_flex_update_map(direction,attr,offset,counter);
                                            }
                                       }
                                   } else {
                                       _bcm_stat_flex_update_map(direction,attr,offset,counter);
                                   }
                              }
                          } else {
                              _bcm_stat_flex_update_map(direction,attr,offset,counter);
                          }
                     }
                 } else {
                     _bcm_stat_flex_update_map(direction,attr,offset,counter);
                 }
            }
       }
 }
 rv =  _bcm_esw_stat_flex_create_mode(unit,attr,&mode);
 for (loop=0;loop<8;loop++) {
      if (offset_array[loop] != NULL) {
      sal_free(offset_array[loop]);
 }
 }
 for (loop=0;loop<(bcmStatGroupModeAttrPacketTypeIp+1);loop++) {
      if (map_array[loop] != NULL) {
          sal_free(map_array[loop]);
      }
 }
 sal_free(attr);
 return rv;
}

/* Create Customized Stat Group mode for given Counter Attributes:FillUpValues*/
static 
bcm_error_t _bcm_esw_stat_group_mode_fillup_values(
            int unit,
            uint32 flags,
            uint32 total_counters,
            uint32 num_selectors,
            bcm_stat_group_mode_attr_selector_t *attr_selectors,
            bcm_stat_flex_attribute_t  *flex_attribute)
{
    uint32 sel=0;
    uint32 counter_offset=0;              /* Counter Offset */
    bcm_stat_group_mode_attr_t attr=0;    /* (Invalid)Attribute Selector */
    uint32 attr_value=0;                  /* Attribute Values */
    uint32 value=0;
    uint32 max_value=0;
    uint32 array_index=0;

    for (sel = 0; sel < num_selectors ; sel++) {
         counter_offset = attr_selectors[sel].counter_offset;
         attr = attr_selectors[sel].attr;
         attr_value = attr_selectors[sel].attr_value;
         if (counter_offset >=  total_counters) {
             return BCM_E_PARAM;
         }

         switch(attr) {
         case bcmStatGroupModeAttrColor:
              flex_attribute->pre_ifp_color=1;
              switch(attr_value) {
              case bcmColorGreen:
                   flex_attribute->combine_attr_counter[counter_offset].
                        pre_ifp_color_flags |= BCM_STAT_FLEX_COLOR_GREEN;
                   break;
              case bcmColorYellow:
                   flex_attribute->combine_attr_counter[counter_offset].
                        pre_ifp_color_flags |= BCM_STAT_FLEX_COLOR_YELLOW;
                   break;
              case bcmColorRed:
                   flex_attribute->combine_attr_counter[counter_offset].
                        pre_ifp_color_flags |= BCM_STAT_FLEX_COLOR_RED;
                   break;
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                   flex_attribute->combine_attr_counter[counter_offset].
                        pre_ifp_color_flags |= (BCM_STAT_FLEX_COLOR_GREEN |
                                                BCM_STAT_FLEX_COLOR_YELLOW |
                                                BCM_STAT_FLEX_COLOR_RED); 
                   break;
              default: 
                  LOG_CLI((BSL_META_U(unit,
                                      "BAD PARAM(Color)"
                           ":sel=%d offset=%d attr=%d value=%d\n"),
                           sel, counter_offset , attr, attr_value ));
                   return BCM_E_PARAM;
              }
              break;
         case bcmStatGroupModeAttrFieldIngressColor:
              flex_attribute->ifp_color=1;
              switch(attr_value) {
              case bcmColorGreen:
                   flex_attribute->combine_attr_counter[counter_offset].
                        ifp_color_flags |= BCM_STAT_FLEX_COLOR_GREEN;
                   break;
              case bcmColorYellow:
                   flex_attribute->combine_attr_counter[counter_offset].
                        ifp_color_flags |= BCM_STAT_FLEX_COLOR_YELLOW;
                   break;
              case bcmColorRed:
                   flex_attribute->combine_attr_counter[counter_offset].
                        ifp_color_flags |= BCM_STAT_FLEX_COLOR_RED;
                   break;
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                   flex_attribute->combine_attr_counter[counter_offset].
                        ifp_color_flags |= (BCM_STAT_FLEX_COLOR_GREEN |
                                            BCM_STAT_FLEX_COLOR_YELLOW |
                                            BCM_STAT_FLEX_COLOR_RED); 
                   break;
              default: 
                   LOG_CLI((BSL_META_U(unit,
                                       "BAD PARAM(FieldIngressColor)"
                                       ":sel=%d offset=%d attr=%d value=%d\n"),
                            sel, counter_offset , attr, attr_value ));
                   return BCM_E_PARAM;
              }
              break;
         case bcmStatGroupModeAttrIntPri:
              flex_attribute->int_pri=1;
              switch(attr_value) {
              case 0:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI0;
                     break;
              case 1:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI1;
                     break;
              case 2:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI2;
                     break;
              case 3:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI3;
                     break;
              case 4:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI4;
                     break;
              case 5:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI5;
                     break;
              case 6:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI6;
                     break;
              case 7:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI7;
                     break;
              case 8:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI8;
                     break;
              case 9:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI9;
                     break;
              case 10:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI10;
                     break;
              case 11:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI11;
                     break;
              case 12:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI12;
                     break;
              case 13:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI13;
                     break;
              case 14:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI14;
                     break;
              case 15:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= BCM_STAT_FLEX_PRI15;
                     break;
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                     flex_attribute->combine_attr_counter[counter_offset].
                         int_pri_flags |= (BCM_STAT_FLEX_PRI0 |
                                           BCM_STAT_FLEX_PRI1 |
                                           BCM_STAT_FLEX_PRI2 |
                                           BCM_STAT_FLEX_PRI3 |
                                           BCM_STAT_FLEX_PRI4 |
                                           BCM_STAT_FLEX_PRI5 |
                                           BCM_STAT_FLEX_PRI6 |
                                           BCM_STAT_FLEX_PRI7 |
                                           BCM_STAT_FLEX_PRI8 |
                                           BCM_STAT_FLEX_PRI9 |
                                           BCM_STAT_FLEX_PRI10 |
                                           BCM_STAT_FLEX_PRI11 |
                                           BCM_STAT_FLEX_PRI12 |
                                           BCM_STAT_FLEX_PRI13 |
                                           BCM_STAT_FLEX_PRI14 |
                                           BCM_STAT_FLEX_PRI15);
                     break;
              default:
                   LOG_CLI((BSL_META_U(unit,
                                       "BAD PARAM(IntPri)"
                            ":sel=%d offset=%d attr=%d value=%d\n"),
                            sel, counter_offset , attr, attr_value ));
                   return BCM_E_PARAM;
              }
              break;
         case bcmStatGroupModeAttrVlan:
              flex_attribute->vlan_format = 1;
              switch(attr_value) {
              case bcmStatGroupModeAttrVlanUnTagged:
                   flex_attribute->combine_attr_counter[counter_offset].
                        vlan_format_flags |= BCM_STAT_FLEX_VLAN_FORMAT_UNTAGGED;
                   break;
              case bcmStatGroupModeAttrVlanInnerTag:
                   flex_attribute->combine_attr_counter[counter_offset].
                        vlan_format_flags |= BCM_STAT_FLEX_VLAN_FORMAT_INNER;
                   break;
              case bcmStatGroupModeAttrVlanOuterTag:
                   flex_attribute->combine_attr_counter[counter_offset].
                        vlan_format_flags |= BCM_STAT_FLEX_VLAN_FORMAT_OUTER;
                   break;
              case bcmStatGroupModeAttrVlanStackedTag:
                   flex_attribute->combine_attr_counter[counter_offset].
                        vlan_format_flags |= BCM_STAT_FLEX_VLAN_FORMAT_BOTH;
                   break;
              case bcmStatGroupModeAttrVlanAll:
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                   flex_attribute->combine_attr_counter[counter_offset].
                        vlan_format_flags |= (BCM_STAT_FLEX_VLAN_FORMAT_BOTH |
                                            BCM_STAT_FLEX_VLAN_FORMAT_UNTAGGED);
                   break;
              default:
                   LOG_CLI((BSL_META_U(unit,
                                       "BAD PARAM(VlanFormat)"
                            ":sel=%d offset=%d attr=%d value=%d\n"),
                            sel, counter_offset , attr, attr_value ));
                   return BCM_E_PARAM;
              }
              break;
         case bcmStatGroupModeAttrOuterPri:
              flex_attribute->outer_dot1p = 1;
              switch(attr_value) {
              case 0:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         outer_dot1p_flags |= BCM_STAT_FLEX_PRI0;
                     break;
              case 1:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         outer_dot1p_flags |= BCM_STAT_FLEX_PRI1;
                     break;
              case 2:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         outer_dot1p_flags |= BCM_STAT_FLEX_PRI2;
                     break;
              case 3:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         outer_dot1p_flags |= BCM_STAT_FLEX_PRI3;
                     break;
              case 4:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         outer_dot1p_flags |= BCM_STAT_FLEX_PRI4;
                     break;
              case 5:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         outer_dot1p_flags |= BCM_STAT_FLEX_PRI5;
                     break;
              case 6:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         outer_dot1p_flags |= BCM_STAT_FLEX_PRI6;
                     break;
              case 7:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         outer_dot1p_flags |= BCM_STAT_FLEX_PRI7;
                     break;
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                     flex_attribute->combine_attr_counter[counter_offset].
                         outer_dot1p_flags |= (BCM_STAT_FLEX_PRI0 |
                                               BCM_STAT_FLEX_PRI1 |
                                               BCM_STAT_FLEX_PRI2 |
                                               BCM_STAT_FLEX_PRI3 |
                                               BCM_STAT_FLEX_PRI4 |
                                               BCM_STAT_FLEX_PRI5 |
                                               BCM_STAT_FLEX_PRI6 |
                                               BCM_STAT_FLEX_PRI7);
                     break;
              default:
                  LOG_CLI((BSL_META_U(unit,
                                      "BAD PARAM(OuterPri)"
                           ":sel=%d offset=%d attr=%d value=%d\n"),
                           sel, counter_offset , attr, attr_value ));
                   return BCM_E_PARAM;
              }
              break;
         case bcmStatGroupModeAttrInnerPri:
              flex_attribute->inner_dot1p = 1;
              switch(attr_value) {
              case 0:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         inner_dot1p_flags |= BCM_STAT_FLEX_PRI0;
                     break;
              case 1:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         inner_dot1p_flags |= BCM_STAT_FLEX_PRI1;
                     break;
              case 2:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         inner_dot1p_flags |= BCM_STAT_FLEX_PRI2;
                     break;
              case 3:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         inner_dot1p_flags |= BCM_STAT_FLEX_PRI3;
                     break;
              case 4:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         inner_dot1p_flags |= BCM_STAT_FLEX_PRI4;
                     break;
              case 5:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         inner_dot1p_flags |= BCM_STAT_FLEX_PRI5;
                     break;
              case 6:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         inner_dot1p_flags |= BCM_STAT_FLEX_PRI6;
                     break;
              case 7:   
                     flex_attribute->combine_attr_counter[counter_offset].
                         inner_dot1p_flags |= BCM_STAT_FLEX_PRI7;
                     break;
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                     flex_attribute->combine_attr_counter[counter_offset].
                         inner_dot1p_flags |= (BCM_STAT_FLEX_PRI0 |
                                               BCM_STAT_FLEX_PRI1 |
                                               BCM_STAT_FLEX_PRI2 |
                                               BCM_STAT_FLEX_PRI3 |
                                               BCM_STAT_FLEX_PRI4 |
                                               BCM_STAT_FLEX_PRI5 |
                                               BCM_STAT_FLEX_PRI6 |
                                               BCM_STAT_FLEX_PRI7);
                     break;
              default:
                   LOG_CLI((BSL_META_U(unit,
                                       "BAD PARAM(InnerPri)"
                            ":sel=%d offset=%d attr=%d value=%d\n"),
                            sel, counter_offset , attr, attr_value ));
                   return BCM_E_PARAM;
              }
              break;
         case bcmStatGroupModeAttrPort:
              flex_attribute->port = 1;
              if (flags & BCM_STAT_GROUP_MODE_INGRESS) {
                  max_value = (1 << ing_pkt_attr_uncmprsd_bits_g.ing_port) - 1;
              } else {
                  max_value = (1 << egr_pkt_attr_uncmprsd_bits_g.egr_port) - 1;
              }   
              switch(attr_value) {
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                   for(value=0;value <= max_value ; value++) {
                       BCM_STAT_FLEX_VALUE_SET(
                           flex_attribute->combine_attr_counter[counter_offset].
                           value_array[bcmStatFlexAttrPort], value);
                   }
                   break;
              default:
                   if (attr_value > max_value) {
                       LOG_CLI((BSL_META_U(unit,
                                           "BAD PARAM(Port)"
                                ":sel=%d offset=%d attr=%d value=%d\n"),
                                sel, counter_offset , attr, attr_value ));
                       return BCM_E_PARAM;
                   }    
                   BCM_STAT_FLEX_VALUE_SET(
                       flex_attribute->combine_attr_counter[counter_offset].
                       value_array[bcmStatFlexAttrPort], attr_value);
                   break;
              } 
              break;
         case bcmStatGroupModeAttrTosDscp:
              flex_attribute->tos_dscp = 1;
              if (flags & BCM_STAT_GROUP_MODE_INGRESS) {
                  max_value = (1 << ing_pkt_attr_uncmprsd_bits_g.tos_dscp) - 1;
              } else {
                  max_value = (1 << egr_pkt_attr_uncmprsd_bits_g.tos_dscp) - 1;
              }   
              switch(attr_value) {
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                   for(value=0;value <= max_value ; value++) {
                       BCM_STAT_FLEX_VALUE_SET(
                           flex_attribute->combine_attr_counter[counter_offset].
                           value_array[bcmStatFlexAttrTosDscp], value);
                   }
                   break;
              default:
                   if (attr_value > max_value) {
                       LOG_CLI((BSL_META_U(unit,
                                           "BAD PARAM(Tos)"
                                ":sel=%d offset=%d attr=%d value=%d\n"),
                                sel, counter_offset , attr, attr_value ));
                       return BCM_E_PARAM;
                   }    
                   BCM_STAT_FLEX_VALUE_SET(
                       flex_attribute->combine_attr_counter[counter_offset].
                       value_array[bcmStatFlexAttrTosDscp], attr_value);
                   break;
              } 
              break;
         case bcmStatGroupModeAttrTosEcn:
              flex_attribute->tos_ecn = 1;
              if (flags & BCM_STAT_GROUP_MODE_INGRESS) {
                  max_value = (1 << ing_pkt_attr_uncmprsd_bits_g.tos_ecn) - 1;
              } else {
                  max_value = (1 << egr_pkt_attr_uncmprsd_bits_g.tos_ecn) - 1;
              }   
              switch(attr_value) {
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                   for(value=0;value <= max_value ; value++) {
                       BCM_STAT_FLEX_VALUE_SET(
                           flex_attribute->combine_attr_counter[counter_offset].
                           value_array[bcmStatFlexAttrTosEcn], value);
                   }
                   break;
              default:
                   if (attr_value > max_value) {
                       LOG_CLI((BSL_META_U(unit,
                                           "BAD PARAM(Tos)"
                                ":sel=%d offset=%d attr=%d value=%d\n"),
                                sel, counter_offset , attr, attr_value ));
                       return BCM_E_PARAM;
                   }    
                   BCM_STAT_FLEX_VALUE_SET(
                       flex_attribute->combine_attr_counter[counter_offset].
                       value_array[bcmStatFlexAttrTosEcn], attr_value);
                   break;
              } 
              break;
         case bcmStatGroupModeAttrPktType:
              flex_attribute->pkt_resolution = 1;
              switch(attr_value) {
              case bcmStatGroupModeAttrPktTypeUnknown:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_PKT;
                   break; 
              case bcmStatGroupModeAttrPktTypeControl:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_CONTROL_PKT                ;
                   break; 
              case bcmStatGroupModeAttrPktTypeOAM:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_OAM_PKT                    ;
                   break;
              case bcmStatGroupModeAttrPktTypeBFD:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_BFD_PKT                    ;
                   break;
              case bcmStatGroupModeAttrPktTypeBPDU:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_BPDU_PKT                   ;
                   break;
              case bcmStatGroupModeAttrPktTypeICNM:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_ICNM_PKT                   ;
                   break;
              case bcmStatGroupModeAttrPktType1588:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_PKT_IS_1588                ;
                   break;
              case bcmStatGroupModeAttrPktTypeKnownL2UC:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2UC_PKT             ;
                   break; 
              case bcmStatGroupModeAttrPktTypeUnknownL2UC:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2UC_PKT           ;
                   break;
              case bcmStatGroupModeAttrPktTypeL2BC:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_L2BC_PKT                   ;
                   break;
              case bcmStatGroupModeAttrPktTypeKnownL2MC:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2MC_PKT             ;
                   break;
              case bcmStatGroupModeAttrPktTypeUnknownL2MC:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2MC_PKT           ;
                   break;
              case bcmStatGroupModeAttrPktTypeKnownL3UC:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_L3UC_PKT             ;
                   break;
              case bcmStatGroupModeAttrPktTypeUnknownL3UC:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L3UC_PKT           ;
                   break;
              case bcmStatGroupModeAttrPktTypeKnownIPMC :
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_IPMC_PKT             ;
                   break; 
              case bcmStatGroupModeAttrPktTypeUnknownIPMC :
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_IPMC_PKT           ;
                   break;
              case bcmStatGroupModeAttrPktTypeKnownMplsL2 :
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L2_PKT          ;
                   break;
              case bcmStatGroupModeAttrPktTypeKnownMplsL3 :
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L3_PKT          ;
                   break;
              case bcmStatGroupModeAttrPktTypeKnownMpls :
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_PKT             ;
                   break;
              case bcmStatGroupModeAttrPktTypeUnknownMpls :
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MPLS_PKT           ;
                   break;
              case bcmStatGroupModeAttrPktTypeKnownMplsMulticast:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_MULTICAST_PKT   ;
                   break;
              case bcmStatGroupModeAttrPktTypeKnownMim:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_MIM_PKT              ;
                   break;
              case bcmStatGroupModeAttrPktTypeUnknownMim:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MIM_PKT            ;
                   break;
              case bcmStatGroupModeAttrPktTypeKnownTrill:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_TRILL_PKT            ;
                   break;
              case bcmStatGroupModeAttrPktTypeUnknownTrill:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_TRILL_PKT          ;
                   break;
              case bcmStatGroupModeAttrPktTypeKnownNiv:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_NIV_PKT              ;
                   break;
              case bcmStatGroupModeAttrPktTypeUnknownNiv:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= 
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_NIV_PKT            ;
                   break;
              case bcmStatGroupModeAttrPktTypeAll:  
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                   flex_attribute->combine_attr_counter[counter_offset].
                         pkt_resolution_flags |= (
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_PKT                |
                         BCM_STAT_FLEX_PKT_TYPE_CONTROL_PKT                |
                         BCM_STAT_FLEX_PKT_TYPE_OAM_PKT                    |
                         BCM_STAT_FLEX_PKT_TYPE_BFD_PKT                    | 
                         BCM_STAT_FLEX_PKT_TYPE_BPDU_PKT                   |
                         BCM_STAT_FLEX_PKT_TYPE_ICNM_PKT                   |
                         BCM_STAT_FLEX_PKT_TYPE_PKT_IS_1588                |
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2UC_PKT             |
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2UC_PKT           |
                         BCM_STAT_FLEX_PKT_TYPE_L2BC_PKT                   |
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_L2MC_PKT             |
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L2MC_PKT           |
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_L3UC_PKT             |
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_L3UC_PKT           |
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_IPMC_PKT             |
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_IPMC_PKT           |
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L2_PKT          |
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_L3_PKT          |
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_PKT             |
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MPLS_PKT           |
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_MPLS_MULTICAST_PKT   |
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_MIM_PKT              |
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_MIM_PKT            |
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_TRILL_PKT            |
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_TRILL_PKT          |
                         BCM_STAT_FLEX_PKT_TYPE_KNOWN_NIV_PKT              |
                         BCM_STAT_FLEX_PKT_TYPE_UNKNOWN_NIV_PKT            ); 
                   break;
              default:
                   LOG_CLI((BSL_META_U(unit,
                                       "BAD PARAM(PktType)"
                            ":sel=%d offset=%d attr=%d value=%d\n"),
                            sel, counter_offset , attr, attr_value ));
                   return BCM_E_PARAM;
              }
              break;
         case bcmStatGroupModeAttrIngNetworkGroup:
              flex_attribute->svp = 1;
              if (flags & BCM_STAT_GROUP_MODE_INGRESS) {
                  max_value = (1 << ing_pkt_attr_uncmprsd_bits_g.svp_type) - 1;
              } else {
                  max_value = (1 << egr_pkt_attr_uncmprsd_bits_g.svp_type) - 1;
              }   
              switch(attr_value) {
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                   for(value=0;value <= max_value ; value++) {
                       BCM_STAT_FLEX_VALUE_SET(
                           flex_attribute->combine_attr_counter[counter_offset].
                           value_array[bcmStatFlexAttrSvp], value);
                   }
                   break;
              default:
                   if (attr_value > max_value) {
                       LOG_CLI((BSL_META_U(unit,
                                           "BAD PARAM(Svp)"
                                ":sel=%d offset=%d attr=%d value=%d\n"),
                                sel, counter_offset , attr, attr_value ));
                       return BCM_E_PARAM;
                   }    
                   BCM_STAT_FLEX_VALUE_SET(
                       flex_attribute->combine_attr_counter[counter_offset].
                       value_array[bcmStatFlexAttrSvp], attr_value);
                   break;
              } 
              break;
         case bcmStatGroupModeAttrEgrNetworkGroup:
              flex_attribute->dvp = 1;
              if (flags & BCM_STAT_GROUP_MODE_INGRESS) {
                  LOG_CLI((BSL_META_U(unit,
                                      "BAD PARAM(DVP) for Ingress Direction\n")));
                  return BCM_E_PARAM;
              } else {
                  max_value = (1 << egr_pkt_attr_uncmprsd_bits_g.dvp_type) - 1;
              }   
              switch(attr_value) {
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                   for(value=0;value <= max_value ; value++) {
                       BCM_STAT_FLEX_VALUE_SET(
                           flex_attribute->combine_attr_counter[counter_offset].
                           value_array[bcmStatFlexAttrDvp], value);
                   }
                   break;
              default:
                   if (attr_value > max_value) {
                       LOG_CLI((BSL_META_U(unit,
                                           "BAD PARAM(Dvp)"
                                ":sel=%d offset=%d attr=%d value=%d\n"),
                                sel, counter_offset , attr, attr_value ));
                       return BCM_E_PARAM;
                   }    
                   BCM_STAT_FLEX_VALUE_SET(
                       flex_attribute->combine_attr_counter[counter_offset].
                       value_array[bcmStatFlexAttrDvp], attr_value);
                   break;
              } 
              break;
         case bcmStatGroupModeAttrDrop:
              flex_attribute->drop = 1;
              switch(attr_value) {
              case 0:
                     flex_attribute->combine_attr_counter[counter_offset].
                         drop_flags |= BCM_STAT_FLEX_DROP_DISABLE;
                     break;     
              case 1:
                     flex_attribute->combine_attr_counter[counter_offset].
                         drop_flags |= BCM_STAT_FLEX_DROP_ENABLE;
                     break;     
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                     flex_attribute->combine_attr_counter[counter_offset].
                         drop_flags |= (BCM_STAT_FLEX_DROP_ENABLE |
                                        BCM_STAT_FLEX_DROP_DISABLE);
                     break;     
              default:
                   LOG_CLI((BSL_META_U(unit,
                                       "BAD PARAM(DropType)"
                            ":sel=%d offset=%d attr=%d value=%d\n"),
                            sel, counter_offset , attr, attr_value ));
                   return BCM_E_PARAM;
              }
              break;
         case bcmStatGroupModeAttrPacketTypeIp:
              flex_attribute->ip_pkt = 1;
              switch(attr_value) {
              case 0:
                     flex_attribute->combine_attr_counter[counter_offset].
                         ip_pkt_flags |= BCM_STAT_FLEX_IP_PKT_DISABLE;
                     break; 
              case 1:
                     flex_attribute->combine_attr_counter[counter_offset].
                         ip_pkt_flags |= BCM_STAT_FLEX_IP_PKT_ENABLE;
                     break;
              case BCM_STAT_GROUP_MODE_ATTR_ALL_VALUES:
                     flex_attribute->combine_attr_counter[counter_offset].
                         ip_pkt_flags |= (BCM_STAT_FLEX_IP_PKT_ENABLE |
                                          BCM_STAT_FLEX_IP_PKT_DISABLE);
                     break;
              default:
                   LOG_CLI((BSL_META_U(unit,
                                       "BAD PARAM(Ip)"
                            ":sel=%d offset=%d attr=%d value=%d\n"),
                            sel, counter_offset , attr, attr_value ));
                   return BCM_E_PARAM;
              }
              break;
         default: 
              return BCM_E_PARAM;
         } 
    }
    for (counter_offset = 0; 
         counter_offset < total_counters ; 
         counter_offset++) {
         if (flex_attribute->pre_ifp_color) {
             if (flex_attribute->combine_attr_counter[counter_offset].
                 pre_ifp_color_flags == 0) {
                 LOG_CLI((BSL_META_U(unit,
                                     "PreIfpColor:Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             } 
         } 
         if (flex_attribute->ifp_color) {
             if (flex_attribute->combine_attr_counter[counter_offset].
                 ifp_color_flags == 0) {
                 LOG_CLI((BSL_META_U(unit,
                                     "IfpColor:Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
         if (flex_attribute->int_pri) {
             if (flex_attribute->combine_attr_counter[counter_offset].
                 int_pri_flags == 0) {
                 LOG_CLI((BSL_META_U(unit,
                                     "IntPri:Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
         if (flex_attribute->vlan_format) {
             if (flex_attribute->combine_attr_counter[counter_offset].
                 vlan_format_flags == 0) {
                 LOG_CLI((BSL_META_U(unit,
                                     "Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
         if (flex_attribute->outer_dot1p) {
             if (flex_attribute->combine_attr_counter[counter_offset].
                 outer_dot1p_flags == 0) {
                 LOG_CLI((BSL_META_U(unit,
                                     "OuterPri: Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
         if (flex_attribute->inner_dot1p) {
             if (flex_attribute->combine_attr_counter[counter_offset].
                 inner_dot1p_flags == 0) {
                 LOG_CLI((BSL_META_U(unit,
                                     "InnerPri: Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
         if (flex_attribute->port) {
             for (array_index=0; 
                  array_index < BCM_STAT_FLEX_BIT_ARRAY_SIZE; 
                  array_index++) {
                  if (flex_attribute->combine_attr_counter[counter_offset].
                      value_array[bcmStatFlexAttrPort][array_index]) {
                      break;
                  }
             }
             if (array_index == BCM_STAT_FLEX_BIT_ARRAY_SIZE ) {
                 LOG_CLI((BSL_META_U(unit,
                                     "Port: Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
         if (flex_attribute->tos_dscp) {
             for (array_index=0; 
                  array_index < BCM_STAT_FLEX_BIT_ARRAY_SIZE; 
                  array_index++) {
                  if (flex_attribute->combine_attr_counter[counter_offset].
                      value_array[bcmStatFlexAttrTosDscp][array_index]) {
                      break;
                  }
             }
             if (array_index == BCM_STAT_FLEX_BIT_ARRAY_SIZE ) {
                 LOG_CLI((BSL_META_U(unit,
                                     "Tos: Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
         if (flex_attribute->tos_ecn) {
             for (array_index=0; 
                  array_index < BCM_STAT_FLEX_BIT_ARRAY_SIZE; 
                  array_index++) {
                  if (flex_attribute->combine_attr_counter[counter_offset].
                      value_array[bcmStatFlexAttrTosEcn][array_index]) {
                      break;
                  }
             }
             if (array_index == BCM_STAT_FLEX_BIT_ARRAY_SIZE ) {
                 LOG_CLI((BSL_META_U(unit,
                                     "Tos: Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
         if (flex_attribute->pkt_resolution) {
             if (flex_attribute->combine_attr_counter[counter_offset].
                 pkt_resolution_flags == 0) {
                 LOG_CLI((BSL_META_U(unit,
                                     "Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             } 
         }
         if (flex_attribute->svp) {
             for (array_index=0; 
                  array_index < BCM_STAT_FLEX_BIT_ARRAY_SIZE; 
                  array_index++) {
                  if (flex_attribute->combine_attr_counter[counter_offset].
                      value_array[bcmStatFlexAttrSvp][array_index]) {
                      break;
                  }
             }
             if (array_index == BCM_STAT_FLEX_BIT_ARRAY_SIZE ) {
                 LOG_CLI((BSL_META_U(unit,
                                     "SVP: Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
         if (flex_attribute->dvp) {
             for (array_index=0; 
                  array_index < BCM_STAT_FLEX_BIT_ARRAY_SIZE; 
                  array_index++) {
                  if (flex_attribute->combine_attr_counter[counter_offset].
                      value_array[bcmStatFlexAttrDvp][array_index]) {
                      break;
                  }
             }
             if (array_index == BCM_STAT_FLEX_BIT_ARRAY_SIZE ) {
                 LOG_CLI((BSL_META_U(unit,
                                     "DVP: Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
         if (flex_attribute->drop) {
             if (flex_attribute->combine_attr_counter[counter_offset].
                 drop_flags == 0) {
                 LOG_CLI((BSL_META_U(unit,
                                     "Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
         if (flex_attribute->ip_pkt) {
             if (flex_attribute->combine_attr_counter[counter_offset].
                 ip_pkt_flags == 0) {
                 LOG_CLI((BSL_META_U(unit,
                                     "DROP: Combination issue..Check parameters"
                                     "And Assign dummy attributes\n"))); 
                 return BCM_E_PARAM; 
             }
         }
    }
    flex_attribute->total_counters = total_counters;
    return BCM_E_NONE;
}
/* Create Customized Stat Group mode for given Counter Attributes */
int _bcm_esw_stat_group_mode_id_create(
    int unit,
    uint32 flags,
    uint32 total_counters,
    uint32 num_selectors,
    bcm_stat_group_mode_attr_selector_t *attr_selectors,
    uint32 *mode_id)
{
    bcm_stat_flex_attribute_t  flex_attribute={0};
    bcm_error_t rv=BCM_E_NONE;
    bcm_stat_flex_ingress_mode_t *flex_ingress_mode=NULL;
    bcm_stat_flex_egress_mode_t  *flex_egress_mode=NULL;
    uint32      mode=0;
    *mode_id = 0; 
    /* Perform Sanity Checks */
    /* Unit will be surely valid */
    if (!((flags & BCM_STAT_GROUP_MODE_INGRESS) || 
          (flags & BCM_STAT_GROUP_MODE_EGRESS))) { 
         return BCM_E_PARAM;
    }
    if ((total_counters == 0) ||
        (total_counters >= BCM_STAT_FLEX_MAX_COUNTER)) {
         return BCM_E_PARAM;
    }
    if (attr_selectors == NULL) {
         return BCM_E_PARAM;
    } 
    /* Checck for existing mode */
    if (flags & BCM_STAT_GROUP_MODE_INGRESS) {
        flex_ingress_mode = sal_alloc(sizeof(bcm_stat_flex_ingress_mode_t),
                                      "flex_ingress_mod");
        if (flex_ingress_mode == NULL) {
            return BCM_E_MEMORY;
        }
        for (mode=0;mode <  BCM_STAT_FLEX_COUNTER_MAX_MODE ; mode++) {
             if (_bcm_esw_stat_flex_get_ingress_mode_info(
                            unit,mode,flex_ingress_mode) == BCM_E_NONE) {
                 if (flex_ingress_mode->num_selectors == num_selectors) {
                     if (sal_memcmp(flex_ingress_mode->attr_selectors,
                         attr_selectors,
                         sizeof(bcm_stat_group_mode_attr_selector_t) &
                         num_selectors) == 0 ) {
                         LOG_CLI((BSL_META_U(unit,
                                             "Mode exist \n")));
                         sal_free(flex_ingress_mode); 
                         *mode_id = mode;
                         return BCM_E_EXISTS;
                     }
                 }
             }
        }
        sal_free(flex_ingress_mode); 
    } else {
        flex_egress_mode = sal_alloc(sizeof(bcm_stat_flex_egress_mode_t),
                                      "flex_egress_mod");
        if (flex_egress_mode == NULL) {
            return BCM_E_MEMORY;
        }
        for (mode=0;mode <  BCM_STAT_FLEX_COUNTER_MAX_MODE ; mode++) {
             if (_bcm_esw_stat_flex_get_egress_mode_info(
                            unit,mode,flex_egress_mode) == BCM_E_NONE) {
                 if (flex_egress_mode->num_selectors == num_selectors) {
                     if (sal_memcmp(flex_egress_mode->attr_selectors,
                         attr_selectors,
                         sizeof(bcm_stat_group_mode_attr_selector_t) &
                         num_selectors) == 0 ) {
                         LOG_CLI((BSL_META_U(unit,
                                             "Mode exist \n")));
                         sal_free(flex_egress_mode); 
                         *mode_id = mode + BCM_STAT_FLEX_COUNTER_MAX_MODE;
                         return BCM_E_EXISTS;
                     }
                 }
             }
        }
        sal_free(flex_egress_mode); 
    }

    flex_attribute.combine_attr_counter = 
                    sal_alloc(sizeof(bcm_stat_flex_combine_attr_counter_t) * 
                               total_counters, "flex_attribute");
    if (flex_attribute.combine_attr_counter == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(flex_attribute.combine_attr_counter,
              0,sizeof(bcm_stat_flex_combine_attr_counter_t) * total_counters); 
    rv = _bcm_esw_stat_group_mode_fillup_values(unit,
         flags, total_counters, num_selectors, attr_selectors, &flex_attribute);
    if (BCM_FAILURE(rv)) { 
        sal_free(flex_attribute.combine_attr_counter);
        return rv;
    }
    rv = _bcm_esw_stat_group_mode_associate_id(unit, 
         flags, &flex_attribute, mode_id);
    if ((rv == BCM_E_NONE) || (rv==BCM_E_EXISTS)) {
         if (flags & BCM_STAT_GROUP_MODE_INGRESS) {
             BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_update_ingress_flex_info(
                                 unit, *mode_id, flags, num_selectors,
                                 attr_selectors));
         } else {    
             BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_update_egress_flex_info(
                                 unit, *mode_id, flags, num_selectors,
                                 attr_selectors));
         }     
         if (flags & BCM_STAT_GROUP_MODE_EGRESS) { 
             *mode_id += BCM_STAT_FLEX_COUNTER_MAX_MODE;
         } 
         rv = BCM_E_NONE;
    }
    sal_free(flex_attribute.combine_attr_counter);
    return rv;
}

/* Retrieves Customized Stat Group mode Attributes for given mode_id */
int _bcm_esw_stat_group_mode_id_get(
    int unit,
    uint32 mode_id,
    uint32 *flags,
    uint32 *total_counters,
    uint32 num_selectors,
    bcm_stat_group_mode_attr_selector_t *attr_selectors,
    uint32 *actual_num_selectors)
{
    bcm_stat_flex_ingress_mode_t *flex_ingress_mode=NULL;
    bcm_stat_flex_egress_mode_t  *flex_egress_mode=NULL;
    uint32                       selector=0;
    bcm_error_t                  rv=BCM_E_NONE;
    if ((flags == NULL) ||
        (total_counters == NULL) ||
        /* (attr_selectors == NULL) || */ /*Could be used to get num_selectors*/
        (actual_num_selectors == NULL)) {
         return BCM_E_PARAM;
    }

    if (mode_id < BCM_STAT_FLEX_COUNTER_MAX_MODE) {
        flex_ingress_mode = sal_alloc(sizeof(bcm_stat_flex_ingress_mode_t),
                                      "flex_ingress_mod");
        if (flex_ingress_mode == NULL) {
            return BCM_E_MEMORY;
        } 
        rv = _bcm_esw_stat_flex_get_ingress_mode_info(unit,
                  mode_id,flex_ingress_mode);
        if (BCM_SUCCESS(rv)) {
            *flags = flex_ingress_mode->flags;     
            *total_counters = flex_ingress_mode->total_counters;     
            *actual_num_selectors = flex_ingress_mode->num_selectors;     
            if (num_selectors <= *actual_num_selectors) {
                for (selector = 0 ; selector <  num_selectors; selector++) {
                     sal_memcpy(&attr_selectors[selector], 
                                &flex_ingress_mode->attr_selectors[selector], 
                                sizeof(bcm_stat_group_mode_attr_selector_t));
                }
            }
        }
        sal_free(flex_ingress_mode);
    } else {
        flex_egress_mode = sal_alloc(sizeof(bcm_stat_flex_egress_mode_t),
                                      "flex_egress_mod");
        if (flex_egress_mode == NULL) {
            return BCM_E_MEMORY;
        } 
        mode_id -= BCM_STAT_FLEX_COUNTER_MAX_MODE;
        rv = _bcm_esw_stat_flex_get_egress_mode_info(unit,
                  mode_id,flex_egress_mode);
        if (BCM_SUCCESS(rv)) {
            *flags = flex_egress_mode->flags;     
            *total_counters = flex_egress_mode->total_counters;     
            *actual_num_selectors = flex_egress_mode->num_selectors;     
            if (num_selectors <= *actual_num_selectors) {
                for (selector = 0 ; selector <  num_selectors; selector++) {
                     sal_memcpy(&attr_selectors[selector], 
                                &flex_egress_mode->attr_selectors[selector], 
                                sizeof(bcm_stat_group_mode_attr_selector_t));
                }
            }
        }
        sal_free(flex_egress_mode);
    }
    return rv;
}

/* Destroys Customized Group mode */
int _bcm_esw_stat_group_mode_id_destroy(
    int unit,
    uint32 mode_id)
{
    uint32 offset_mode=0;
    bcm_error_t rv=BCM_E_NONE;
    bcm_stat_group_mode_t     group_mode= bcmStatGroupModeSingle;
    uint32 flags = BCM_STAT_GROUP_MODE_INGRESS; 

    if (mode_id >= BCM_STAT_FLEX_COUNTER_MAX_MODE) {
        flags = BCM_STAT_GROUP_MODE_EGRESS; 
        mode_id -= BCM_STAT_FLEX_COUNTER_MAX_MODE;
    }
    offset_mode = mode_id;
    if (flags & BCM_STAT_GROUP_MODE_INGRESS) {
        if ((rv=_bcm_esw_stat_flex_delete_ingress_mode(
                unit,offset_mode)) == BCM_E_NONE) {
                group_mode = bcmStatGroupModeFlex1 + offset_mode;
                LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "Destroyed Ingress Mode also \n")));
                _bcm_esw_stat_flex_reset_group_mode(
                                    unit,bcmStatFlexDirectionIngress,
                                    offset_mode,group_mode);
        }
    } else {
        if ((rv=_bcm_esw_stat_flex_delete_egress_mode(
                unit,offset_mode)) == BCM_E_NONE) {
                group_mode = bcmStatGroupModeFlex1 + offset_mode;
                LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                          (BSL_META_U(unit,
                                      "Destroyed Egress Mode also \n")));
                _bcm_esw_stat_flex_reset_group_mode(
                                    unit,bcmStatFlexDirectionEgress,
                                    offset_mode,group_mode);
        }
    }
    return  rv;
}
/* Associate an accounting object to customized group mode */
static 
int _bcm_esw_stat_custom_group_associate_object(
    int               unit,
    uint32            mode_id,
    bcm_stat_object_t object,
    uint32            *stat_counter_id,
    uint32            *num_entries)
{
    bcm_stat_flex_direction_t    direction = bcmStatFlexDirectionIngress;
    uint32                       mode=0;
    uint32                       base_index=0;
    uint32                       pool_number=0;
    bcm_error_t                  rv=BCM_E_NONE;
    bcm_stat_group_mode_t        group_mode= bcmStatGroupModeSingle;
    bcm_stat_flex_ingress_mode_t *flex_ingress_mode=NULL;
    bcm_stat_flex_egress_mode_t  *flex_egress_mode=NULL;
    uint32                       total_counters=0;

    group_mode = bcmStatGroupModeFlex1 + mode;

    /* 1. Deciding direction */
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_object(unit,object,&direction));
    if (direction == bcmStatFlexDirectionIngress) {
        mode = mode_id;
        flex_ingress_mode = sal_alloc(sizeof(bcm_stat_flex_ingress_mode_t),
                                      "flex_ingress_mod");
        if (flex_ingress_mode == NULL) {
            return BCM_E_MEMORY;
        }
        rv= _bcm_esw_stat_flex_get_ingress_mode_info(
                 unit,mode,flex_ingress_mode);
        if (BCM_SUCCESS(rv)) {
            total_counters = flex_ingress_mode->total_counters;     
        }
        sal_free(flex_ingress_mode);
        BCM_IF_ERROR_RETURN(rv);
     } else {
        mode = mode_id - BCM_STAT_FLEX_COUNTER_MAX_MODE;
        flex_egress_mode = sal_alloc(sizeof(bcm_stat_flex_egress_mode_t),
                                      "flex_egress_mod");
        if (flex_egress_mode == NULL) {
            return BCM_E_MEMORY;
        }
        rv= _bcm_esw_stat_flex_get_egress_mode_info(
                 unit,mode,flex_egress_mode);
        if (BCM_SUCCESS(rv)) {
            total_counters = flex_egress_mode->total_counters;     
        }
        sal_free(flex_egress_mode);
        BCM_IF_ERROR_RETURN(rv);
     }   

    BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_set_group_mode(
                        unit,direction, mode,group_mode));
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
                   unit, SOURCE_VPm,object,mode,
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
    case bcmStatObjectIngIpmc:
         rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                   unit, L3_ENTRY_IPV4_MULTICASTm,object,mode,
                   &base_index,&pool_number);
         break; 
    case bcmStatObjectIngVxlanDip:
         rv = _bcm_esw_stat_flex_create_ingress_table_counters(
                   unit, VLAN_XLATEm, object, mode,
                   &base_index, &pool_number);
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
    case bcmStatObjectMaxValue:
         rv = BCM_E_PARAM;
         break;
    }
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "creation of counters passed  failed..\n")));
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
        LOG_DEBUG(BSL_LS_BCM_FLEXCTR,
                  (BSL_META_U(unit,
                              "Create: mode:%d group_mode:%d pool:%d object:%d"
                               " base:%d\n stat_counter_id:%d\n"),
                   mode,group_mode,pool_number,object,base_index,
                   *stat_counter_id));
        *num_entries= total_counters ; 
    }
    return rv;
}

/* Associate an accounting object to customized group mode */
int _bcm_esw_stat_custom_group_create(
    int               unit,
    uint32            mode_id,
    bcm_stat_object_t object,
    uint32            *stat_counter_id,
    uint32            *num_entries)
{
    return  _bcm_esw_stat_custom_group_associate_object(
            unit, mode_id, object, stat_counter_id, num_entries);
}


