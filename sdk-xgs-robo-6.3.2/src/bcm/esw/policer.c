/*
 * $Id: policer.c 1.38 Broadcom SDK $
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
/*
 * All Rights Reserved.$
 *
 * Module: Service Meter(global meter) 
 *
 * Purpose:
 *     These routines manage the handling of service meter(global meter)
 *      functionality 
 *
 *
 */
#include <shared/idxres_afl.h>

#include <sal/core/libc.h>

#include <soc/defs.h>

#include <bcm/policer.h>
#include <bcm/module.h>
#include <bcm/debug.h>
#include <bcm_int/esw/policer.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/vlan.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw_dispatch.h>

#ifdef BCM_WARM_BOOT_SUPPORT
#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_0
#endif

#define _BCM_XGS_METER_FLAG_GLOBAL_METER_POLICER _BCM_XGS_METER_FLAG_FP_POLICER


#define POLICER_OUT(flags, stuff)  BCM_DEBUG(flags | BCM_DBG_POLICER, stuff)
#define POLICER_WARN(stuff)        BCM_DEBUG(BCM_DBG_WARN, stuff)
#define POLICER_ERR(stuff)         BCM_DEBUG(BCM_DBG_ERR, stuff)
#define POLICER_VERB(stuff)        POLICER_OUT(BCM_DBG_VERBOSE, stuff)
#define POLICER_VVERB(stuff)       POLICER_OUT(BCM_DBG_VVERBOSE, stuff)

#define UNKNOWN_PKT                 0x00       
#define CONTROL_PKT                 0x01     
#define OAM_PKT                     0x02       
#define BFD_PKT                     0x03       
#define BPDU_PKT                    0x04     
#define ICNM_PKT                    0x05     
#define PKT_IS_1588                 0x06       
#define KNOWN_L2UC_PKT              0x07       

#define UNKNOWN_L2UC_PKT            0x08    
#define KNOWN_L2MC_PKT              0x09       
#define UNKNOWN_L2MC_PKT            0x0a     
#define L2BC_PKT                    0x0b     
#define KNOWN_L3UC_PKT              0x0c      
#define UNKNOWN_L3UC_PKT            0x0d    
#define KNOWN_IPMC_PKT              0x0e  
#define UNKNOWN_IPMC_PKT            0x0f      

#define KNOWN_MPLS_L2_PKT           0x10   
#define UNKNOWN_MPLS_PKT            0x11   
#define KNOWN_MPLS_L3_PKT           0x12      
#define KNOWN_MPLS_PKT              0x13      
#define KNOWN_MPLS_MULTICAST_PKT    0x14      
#define KNOWN_MIM_PKT               0x15      
#define UNKNOWN_MIM_PKT             0x16     
#define KNOWN_TRILL_PKT             0x17     

#define UNKNOWN_TRILL_PKT           0x18    
#define KNOWN_NIV_PKT               0x19       
#define UNKNOWN_NIV_PKT             0x1a    
#define	KNOWN_L2GRE_PKT             0x1b
#define KNOWN_VDL2_PKT              0x1c
#define KNOWN_FCOE_PKT              0x1d
#define UNKNOWN_FCOE_PKT            0x1e
#define FP_RESOLUTION_MAX           0x1f  /* Max value */

#define METER_REFRESH_INTERVAL      8     /* 7.8125usec */
#define MIN_BURST_MULTIPLE          2     

#define CONVERT_SECOND_TO_MICROSECOND 1000000

static soc_reg_t _pkt_attr_sel_key_reg[4] = {
                ING_SVM_PKT_ATTR_SELECTOR_KEY_0r,
                ING_SVM_PKT_ATTR_SELECTOR_KEY_1r,
                ING_SVM_PKT_ATTR_SELECTOR_KEY_2r,
                ING_SVM_PKT_ATTR_SELECTOR_KEY_3r
            };

STATIC sal_mutex_t global_meter_mutex[BCM_MAX_NUM_UNITS] = {NULL};

static uint8 kt_pkt_res[FP_RESOLUTION_MAX] = 
                              { 
                                   0x0, 0x1, 0x0, 0x0, 0x2, 0x0, 0x0, 0x4,
                                   0x5, 0x8, 0x9, 0x3, 0xa, 0xb, 0x7, 0x6,
                                   0xe, 0xf, 0xd, 0xc, 0x12, 0x10, 0x11, 0,
                                   0, 0, 0, 0, 0, 0, 0     
                               };
static uint8 tr3_pkt_res[FP_RESOLUTION_MAX] = 
                          { 
                               0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x8,
                               0x9, 0xa, 0xb, 0xc, 0x10, 0x11, 0x12, 0x13,
                               0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x20, 0x21, 0x28,
                               0x29, 0x30, 0x31, 0, 0, 0, 0     
                          };
static uint8 kt2_pkt_res[FP_RESOLUTION_MAX] =
                          {
                               0x0,  0x1,  0x2,  0x3,  0x4,  0x5,  0x6,   0x8,
                               0x9,  0xa,  0xb,  0xc,  0x10, 0x11, 0x12,  0x13,
                               0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x20, 0x21,  0x28,
                               0x29, 0x30, 0x31, 0,    0,    0,    0
                          };
/*
 * Global meter policer control data, one per device.
*/
static _global_meter_policer_control_t 
                    **global_meter_policer_bookkeep[BCM_MAX_NUM_UNITS];
/*
 * Global meter horizontal alloc management data, one per device.
 */
static _global_meter_horizontal_alloc_t 
              *global_meter_hz_alloc_bookkeep[BCM_MAX_NUM_UNITS];

static bcm_policer_svc_meter_bookkeep_mode_t
      global_meter_offset_mode[BCM_MAX_NUM_UNITS]\
                              [BCM_POLICER_SVC_METER_MAX_MODE];

static bcm_policer_global_meter_action_bookkeep_t
          *global_meter_action_bookkeep[BCM_MAX_NUM_UNITS];

/* Global meter module status - initialised or not initilised */
static bcm_policer_global_meter_init_status_t
                        global_meter_status[BCM_MAX_NUM_UNITS] = {{0}};
/*
 * Global meter resource lock
 */
#define GLOBAL_METER_LOCK(unit) \
        sal_mutex_take(global_meter_mutex[unit], sal_mutex_FOREVER);

#define GLOBAL_METER_UNLOCK(unit) \
        sal_mutex_give(global_meter_mutex[unit]);

/* Handles for indexed allocation - one for meter action id allocation
   and the other for policer id alloaction  */
shr_aidxres_list_handle_t  
          meter_action_list_handle[BCM_UNITS_MAX] = {NULL};
shr_aidxres_list_handle_t  
        meter_alloc_list_handle[BCM_UNITS_MAX]\
                           [BCM_POLICER_GLOBAL_METER_MAX_POOL] = {{NULL}}; 

#define CHECK_GLOBAL_METER_INIT(unit)                           \
        if (!global_meter_status[unit].initialised)             \
            return BCM_E_INIT

/*
 * Function:
 *      check_global_meter_init
 * Purpose:
 *     To check whether the global meter module is initialized or not 
 * Parameters:
 *     Unit                  - (IN) unit number 
 * Returns:
 *     BCM_E_INIT 
 */
int _check_global_meter_init(int unit)
{
    if (!global_meter_status[unit].initialised) {             
            return BCM_E_INIT;
    } else {
        return BCM_E_NONE;
    }
}

/*
 * Function:
 *      convert_to_bitmask
 * Purpose:
 *      
 * Parameters:
 *     num - number to be converted to bistmask
 * Returns:
 *     bit mask for the number 
 */
static int convert_to_bitmask(int num) 
{
    int bit_mask = 0;
    num = num & 0xf;
    while (num > 0) {
        bit_mask = bit_mask | (1 << (num-1));
        num--;
    }
    return bit_mask;
}

/*
 * Function:
 *      convert_to_mask
 * Purpose:
 *     Creates a mask with all the bit positon other then the positon 
 *     represented by num 
 * Parameters:
 *     num -  number 
 * Returns:
 *     BCM_E_XXX 
 */
static int convert_to_mask(int num) 
{
    int bit_mask = 0;
    num = num & 0xf;
    bit_mask =  ~(1 << num);
    return bit_mask;
}

/*
 * Function:
 *      _bcm_policer_svc_meter_update_udf_selector_keys
 * Purpose:
 *     Internal function for updating udf selector keys 
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     pkt_attr_selector_key - (IN) packet attribute selector key 
 *     udf_pkt_attr          - (IN) UDF packet attributes 
 *     total_udf_bits        - (OUT) Total number of udf bits used  
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_policer_svc_meter_update_udf_selector_keys(
    int                        unit,
    soc_reg_t                  pkt_attr_selector_key,
    udf_pkt_attr_selectors_t   udf_pkt_attr,
    uint32                     *total_udf_bits)
{
    uint64    pkt_attr_selector_key_value;
    uint32    udf_valid_bits = 0;
    uint8     udf_value_bit_position = 0;
    uint8     key_bit_selector_position = 0;
    uint16    udf_value = 0;

    COMPILER_64_ZERO(pkt_attr_selector_key_value); 
    (*total_udf_bits) = 0;
    if (!(pkt_attr_selector_key >= ING_SVM_PKT_ATTR_SELECTOR_KEY_0r) && 
         (pkt_attr_selector_key <= ING_SVM_PKT_ATTR_SELECTOR_KEY_3r)) {
      /* Valid mem value go ahead for setting 
                       ING_FLEX_CTR_PKT_ATTR_SELECTOR_KEY_?r: */
        POLICER_VVERB(("Invalid memory for packet attribute selector\n"));
        return BCM_E_PARAM;
    }

    /* Sanity Check: Get total number of udf bits */
    udf_value = udf_pkt_attr.udf_pkt_attr_bits_v.udf0;

    for (udf_value_bit_position = 0; udf_value_bit_position < 
                   SVC_METER_UDF_MAX_BIT_POSITION; udf_value_bit_position++) {
        if (udf_value & 0x1) {
            (*total_udf_bits)++;
        }
        udf_value = udf_value >> 1;
    }

    if (*total_udf_bits > BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) {
        POLICER_VVERB(("Invalid selection of UDF bits - exceeds max allowed\n"));
        return BCM_E_PARAM;
    }

    udf_value = udf_pkt_attr.udf_pkt_attr_bits_v.udf1;
    for (udf_value_bit_position = 0; udf_value_bit_position < 
                    SVC_METER_UDF_MAX_BIT_POSITION; udf_value_bit_position++) {
        if (udf_value & 0x1) {
            (*total_udf_bits)++;
        }
        udf_value = udf_value >> 1;
    }
    if ((*total_udf_bits) > BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) {
        POLICER_VVERB(("Invalid selection of UDF bits - exceeds max allowed\n"));
        return BCM_E_PARAM;
    }

    /* First Get complete value of ING_SVM_PKT_ATTR_SELECTOR_KEY_?r value */
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, 
            pkt_attr_selector_key, 
            REG_PORT_ANY, 
            0, 
            &pkt_attr_selector_key_value));

    /* Next set field value for 
                    ING_SVM_PKT_ATTR_SELECTOR_KEY_?r:SELECTOR_KEY field*/
    soc_reg64_field32_set( 
            unit, 
            pkt_attr_selector_key,
            &pkt_attr_selector_key_value,
            USE_UDF_KEYf,
            1);
    if (udf_pkt_attr.udf_pkt_attr_bits_v.udf0 != 0) {
        udf_valid_bits |= SVC_METER_UDF1_VALID;
    }
    if (udf_pkt_attr.udf_pkt_attr_bits_v.udf1 != 0) {
        udf_valid_bits |= SVC_METER_UDF2_VALID;
    }
    soc_reg64_field32_set( 
            unit, 
            pkt_attr_selector_key,
            &pkt_attr_selector_key_value,
            USER_SPECIFIED_UDF_VALIDf,
            udf_valid_bits);

    /* Now update selector keys */

    udf_value = udf_pkt_attr.udf_pkt_attr_bits_v.udf0;
    for (udf_value_bit_position = 0; udf_value_bit_position < 
                     SVC_METER_UDF_MAX_BIT_POSITION; udf_value_bit_position++) {
        if (udf_value & 0x1) {
            BCM_IF_ERROR_RETURN(
                  _bcm_policer_svc_meter_update_selector_keys_enable_fields(
                        unit, 
                        pkt_attr_selector_key, 
                        &pkt_attr_selector_key_value,
                        udf_value_bit_position + 1, 
                        1,
                        &key_bit_selector_position));
        }
        udf_value = udf_value >> 1;
    }
    udf_value = udf_pkt_attr.udf_pkt_attr_bits_v.udf1;
    for (; udf_value_bit_position < SVC_METER_UDF1_MAX_BIT_POSITION; 
                                                   udf_value_bit_position++) {
        if (udf_value & 0x1) {
            BCM_IF_ERROR_RETURN(
                   _bcm_policer_svc_meter_update_selector_keys_enable_fields(
                        unit, 
                        pkt_attr_selector_key, 
                        &pkt_attr_selector_key_value,
                        udf_value_bit_position + 1,
                        1,
                        &key_bit_selector_position));
        }
        udf_value = udf_value >> 1;
    }
    /* Finally set value for ING_SVM_PKT_ATTR_SELECTOR_KEY_?r */
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, 
            pkt_attr_selector_key, 
            REG_PORT_ANY, 
            0, 
            pkt_attr_selector_key_value));
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_policer_get_offset_table_policer_count
 * Purpose:
 *    set group mode and number of policers in offset table at
 *              offsets 254 and 255 - for use during warmboot 
 *      
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     offset_mode           - (IN) Offset mode 
 *     mode                  - (OUT) Policer group mode 
 *     npolicers             - (OUT) Total number of policer created  
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_esw_policer_get_offset_table_policer_count(
    int                                unit, 
    bcm_policer_svc_meter_mode_t       offset_mode,
    bcm_policer_group_mode_t           *mode, 
    int                                *npolicers)
{
    uint32            offset_table_entry = 0;
    int               max_index = 0;
    max_index = BCM_POLICER_SVC_METER_MAX_OFFSET;
    /* First Get complete value of ING_SVM_METER_OFFSET_TABLE_?m value */
    SOC_IF_ERROR_RETURN(soc_mem_read(unit,
         SVM_OFFSET_TABLEm, MEM_BLOCK_ANY,
         ((offset_mode << BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) | 
                                                        (max_index  - 1)),
         &offset_table_entry));
    soc_mem_field_get( unit, SVM_OFFSET_TABLEm, &offset_table_entry,
                                            OFFSETf, (uint32 *)npolicers);
    SOC_IF_ERROR_RETURN(soc_mem_read(unit,
         SVM_OFFSET_TABLEm, MEM_BLOCK_ANY,
         ((offset_mode << BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) | 
                                                        (max_index  - 2)),
         &offset_table_entry));
    soc_mem_field_get( unit, SVM_OFFSET_TABLEm, &offset_table_entry,
                                                 OFFSETf, (uint32 *)mode);
   return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_policer_update_offset_table_policer_count
 * Purpose:
 *    set group mode and number of policers in offset table at
 *              offsets 254 and 255 - for use during warmboot 
 *      
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     offset_mode           - (IN) Offset mode 
 *     mode                  - (IN) Policer group mode 
 *     npolicers             - (IN) Nmber of policers  
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_esw_policer_update_offset_table_policer_count(
    int                                unit, 
    bcm_policer_svc_meter_mode_t       offset_mode,
    bcm_policer_group_mode_t           mode, 
    int                                npolicers)
{
    uint32            offset_table_entry = 0;
    uint32            zero = 0;
    int               max_index = 0;
    bcm_policer_group_mode_t  *grp_mode = &mode; 
    max_index = BCM_POLICER_SVC_METER_MAX_OFFSET;
    /* First Get complete value of ING_SVM_METER_OFFSET_TABLE_?m value */
    SOC_IF_ERROR_RETURN(soc_mem_read(unit,
         SVM_OFFSET_TABLEm, MEM_BLOCK_ANY,
         ((offset_mode << BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) | 
                                                        (max_index  - 1)),
         &offset_table_entry));
    /* Set OFFSETf=number of policers in this mode */
    soc_mem_field_set( unit, SVM_OFFSET_TABLEm, &offset_table_entry,
                                            OFFSETf, (uint32 *)&npolicers);
    /* Set METER_ENABLEf=zero */
    soc_mem_field_set(unit, SVM_OFFSET_TABLEm, &offset_table_entry, 
                                                     METER_ENABLEf, &zero);
    /* Finally set value for ING_SVM_METER_OFFSET_TABLE_?m?r */
    soc_mem_write(unit, SVM_OFFSET_TABLEm, MEM_BLOCK_ALL, 
         ((offset_mode << BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) |
                                                         (max_index - 1)),
         &offset_table_entry);
    /* Set OFFSETf=group mode */
    soc_mem_field_set(unit, SVM_OFFSET_TABLEm, &offset_table_entry, 
                                            OFFSETf, (uint32 *)grp_mode);
    soc_mem_write(unit, SVM_OFFSET_TABLEm, MEM_BLOCK_ALL,
         ((offset_mode << BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) |  
                                                       (max_index - 2)), 
         &offset_table_entry);
   return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_policer_svc_meter_update_offset_table
 * Purpose:
 *     Internal function to update the SVM offset table    
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     offset_table_mem      - (IN) Offset table memory 
 *     svc_meter_mode        - (IN) Meter offset mode 
 *     offset_map            - (IN) Offset table entry map  
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_policer_svc_meter_update_offset_table(
    int                                unit,
    soc_mem_t                          offset_table_mem,
    bcm_policer_svc_meter_mode_t       svc_meter_mode,
    offset_table_entry_t  *offset_map)
{
    uint32            offset_table_entry = 0;
    uint32            index = 0;
    uint32            zero = 0;
    uint32            meter_enable = 0; 
    uint32            offset_value = 0, pool = 0;       
    if (offset_table_mem != SVM_OFFSET_TABLEm) {  
        /* Valid mem value go ahead for setting SVM_OFFSET_TABLEm */
        POLICER_VVERB(("%s : Invalid table specified \n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    if(NULL == offset_map) {
        /* Clear all the entries */
        for (; index < BCM_POLICER_SVC_METER_MAX_OFFSET; index++) {
            /* First Get complete value of ING_SVM_METER_OFFSET_TABLE_?m value */
                SOC_IF_ERROR_RETURN(soc_mem_read(unit,
                    offset_table_mem,
                    MEM_BLOCK_ANY,
                    ((svc_meter_mode << 
                        BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) | index), 
                    &offset_table_entry));
            /* Set POOL_OFFSETf */
            soc_mem_field_set(
                    unit,
                    offset_table_mem,
                    &offset_table_entry,
                    POOL_OFFSETf,
                    &zero);
            /* Set OFFSETf=zero */
            soc_mem_field_set(
                    unit,
                    offset_table_mem,
                    &offset_table_entry,
                    OFFSETf,
                    &zero);
            /* Set METER_ENABLEf=zero */
            soc_mem_field_set(
                    unit,
                    offset_table_mem,
                    &offset_table_entry,
                    METER_ENABLEf,
                    &zero);
            /* Finally set value for ING_SVM_METER_OFFSET_TABLE_?m?r */
            soc_mem_write(unit,
                    offset_table_mem,
                    MEM_BLOCK_ALL,
                    ((svc_meter_mode << 
                     BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) | index),
                    &offset_table_entry);

        }
    } else {
        for (index = 0; index < 256; index++) {
            offset_value = offset_map[index].offset;
            meter_enable = offset_map[index].meter_enable;
            pool = offset_map[index].pool;
            /* First Get complete value of SVM_OFFSET_TABLE_?m value */
            SOC_IF_ERROR_RETURN(soc_mem_read(unit,
                offset_table_mem,
                MEM_BLOCK_ANY,
                ((svc_meter_mode << 
                 BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) | index), 
                &offset_table_entry));
            /* Set POOL_OFFSETf */
            soc_mem_field_set(
                unit,
                offset_table_mem,
                &offset_table_entry,
                POOL_OFFSETf,
                &pool);
            /* Set OFFSETf */
            soc_mem_field_set(
                unit,
                offset_table_mem,
                &offset_table_entry,
                OFFSETf,
                &offset_value);
           /* Set METER_ENABLEf */
            soc_mem_field_set(
                unit,
                offset_table_mem,
                &offset_table_entry,
                METER_ENABLEf,
                &meter_enable);
            /* Finally set value for ING_SVM_OFFSET_TABLE_?m?r */
            soc_mem_write(unit,
                offset_table_mem,
                MEM_BLOCK_ALL,
                ((svc_meter_mode << 
                   BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) | index),
                &offset_table_entry);
        }
    }
    if(svc_meter_mode == 0) {
        index = 0;
        meter_enable  = 1;
        /*  SVM_OFFSET_TABLE_?m value */
        SOC_IF_ERROR_RETURN(soc_mem_read(unit,
            offset_table_mem,
            MEM_BLOCK_ANY,
            ((svc_meter_mode << BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE)), 
            &offset_table_entry));
        /* Set OFFSETf */
        soc_mem_field_set(
            unit,
            offset_table_mem,
            &offset_table_entry,
            OFFSETf,
            &zero);
       /* Set METER_ENABLEf */
        soc_mem_field_set(
            unit,
            offset_table_mem,
            &offset_table_entry,
            METER_ENABLEf,
            &meter_enable);
        /* Finally set value for ING_SVM_OFFSET_TABLE_?m?r */
        soc_mem_write(unit,
            offset_table_mem,
            MEM_BLOCK_ALL,
            ((svc_meter_mode << BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) | index),
            &offset_table_entry);
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_policer_svc_meter_update_selector_keys_enable_fields
 * Purpose:
 *     Internal function to set selector key enabled fields      
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     pkt_attr_selector_key - (IN) Pkt attribute selector key 
 *     pkt_attr_selector_key_value - (IN) Value of selector key 
 *     pkt_attr_bit_position - (IN) Packet attribute bit position  
 *     pkt_attr_total_bits   - (IN) total number of packet attribute bits  
 *     current_bit_selector_position - (IN) Current bit position  
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_policer_svc_meter_update_selector_keys_enable_fields(
    int         unit, 
    soc_reg_t   pkt_attr_selector_key, 
    uint64      *pkt_attr_selector_key_value,
    uint32      pkt_attr_bit_position,
    uint32      pkt_attr_total_bits,
    uint8       *current_bit_selector_position)
{
    uint32    index = 0;
    uint32    selector_x_en_field_name[8]= {
        SELECTOR_0_ENf,
        SELECTOR_1_ENf,
        SELECTOR_2_ENf,
        SELECTOR_3_ENf,
        SELECTOR_4_ENf,
        SELECTOR_5_ENf,
        SELECTOR_6_ENf,
        SELECTOR_7_ENf,
    };
    uint32    selector_for_bit_x_field_name[8]={
        SELECTOR_FOR_BIT_0f,
        SELECTOR_FOR_BIT_1f,
        SELECTOR_FOR_BIT_2f,
        SELECTOR_FOR_BIT_3f,
        SELECTOR_FOR_BIT_4f,
        SELECTOR_FOR_BIT_5f,
        SELECTOR_FOR_BIT_6f,
        SELECTOR_FOR_BIT_7f,
    };

    if ((*current_bit_selector_position) + 
        pkt_attr_total_bits > BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) {
        POLICER_VVERB(("%s : Number of slector bits exceeds max allowed \n",
                                                          FUNCTION_NAME()));
        return BCM_E_INTERNAL;
    }

    for (index = 0; index < pkt_attr_total_bits; index++) {
        soc_reg64_field32_set( 
            unit, 
            pkt_attr_selector_key,
            pkt_attr_selector_key_value,
            selector_x_en_field_name[*current_bit_selector_position],
            1);
        soc_reg64_field32_set( 
            unit, 
            pkt_attr_selector_key,
            pkt_attr_selector_key_value,
            selector_for_bit_x_field_name[*current_bit_selector_position],
            pkt_attr_bit_position + index);
        (*current_bit_selector_position) += 1;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_policer_svc_meter_dec_mode_reference_count
 * Purpose:
 *     Internal function to decrement meter offset mode usage count      
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     svc_meter_mode        - (IN) Meter offset mode 
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t bcm_policer_svc_meter_dec_mode_reference_count( 
    uint32 unit,
    bcm_policer_svc_meter_mode_t svc_meter_mode)
{
    int rv = BCM_E_NONE;
    if (!((svc_meter_mode >= 1) && (svc_meter_mode <= 3))) {
        POLICER_VVERB(("%s: Wrong offset mode specified \n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    if ( global_meter_offset_mode[unit][svc_meter_mode].reference_count > 0) {
       global_meter_offset_mode[unit][svc_meter_mode].reference_count--;
    }
    if (global_meter_offset_mode[unit][svc_meter_mode].reference_count == 0) {
        rv = _bcm_esw_policer_svc_meter_delete_mode(unit, svc_meter_mode);
    }
    return rv;
}

/*
 * Function:
 *      bcm_policer_svc_meter_inc_mode_reference_count
 * Purpose:
 *     Internal function to increment meter offset mode usage count      
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     svc_meter_mode        - (IN) Meter offset mode 
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t bcm_policer_svc_meter_inc_mode_reference_count( 
    uint32 unit,
    bcm_policer_svc_meter_mode_t svc_meter_mode)
{
    if (!((svc_meter_mode >= 1) && (svc_meter_mode <= 3))) {
        POLICER_VVERB(("%s: Wrong offset mode specified \n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    global_meter_offset_mode[unit][svc_meter_mode].reference_count++;
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_policer_svc_meter_reserve_mode
 * Purpose:
 *     Internal function to allocate/reserve meter offset mode      
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     svc_meter_mode        - (IN) Meter offset mode 
 *     total_bits            - (IN) Total number of bits used 
 *     group_mode            - (IN) Policer group mode 
 *     meter_attr            - (IN) Meter attributes 
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_policer_svc_meter_reserve_mode( 
    uint32                               unit,
    bcm_policer_svc_meter_mode_t         svc_meter_mode,
    bcm_policer_group_mode_t             group_mode,            
    bcm_policer_svc_meter_attr_t         *meter_attr)
{
    if (!((svc_meter_mode >= 1) && (svc_meter_mode <= 3))) {
        POLICER_VVERB(("%s: Wrong offset mode specified \n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    global_meter_offset_mode[unit][svc_meter_mode].used = 1;
    global_meter_offset_mode[unit][svc_meter_mode].group_mode = group_mode;
    global_meter_offset_mode[unit][svc_meter_mode].meter_attr = *meter_attr;    
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_policer_svc_meter_unreserve_mode
 * Purpose:
 *     Internal function to unreserve meter offset mode      
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     svc_meter_mode        - (IN) Meter offset mode 
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_policer_svc_meter_unreserve_mode(
    uint32                                  unit,
    bcm_policer_svc_meter_mode_t            svc_meter_mode)
{
    uint64    pkt_attr_selector_key_value;
    bcm_policer_svc_meter_mode_type_t       mode_type_v;

    COMPILER_64_ZERO(pkt_attr_selector_key_value); 
    if (!((svc_meter_mode >= 1) && (svc_meter_mode <= 3))) {
        POLICER_VVERB(("%s: Wrong offset mode specified \n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    if ( global_meter_offset_mode[unit][svc_meter_mode].used == 0) {
        POLICER_VVERB(("%s: Wrong offset mode: Mode is not in use\n", 
                                                          FUNCTION_NAME()));
        return BCM_E_NOT_FOUND;
    }
    if ( global_meter_offset_mode[unit][svc_meter_mode].reference_count != 0) {
        POLICER_VVERB(("%s: Mode is still in use\n", FUNCTION_NAME()));
        return BCM_E_INTERNAL;
    }

    /* Step1:  Reset selector keys */
    SOC_IF_ERROR_RETURN(soc_reg_set(unit,
        _pkt_attr_sel_key_reg[svc_meter_mode],
        REG_PORT_ANY,
        0,
        pkt_attr_selector_key_value));

     /* Step2: Reset Offset table filling */
    BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_update_offset_table(unit,
        SVM_OFFSET_TABLEm,
        svc_meter_mode,
        NULL));
    /* Step3: Cleanup based on mode */
    mode_type_v = global_meter_offset_mode[unit][svc_meter_mode].\
                                                    meter_attr.mode_type_v;
    switch (mode_type_v) {
       case uncompressed_mode:
           break;
       case udf_mode:
           break;
       case cascade_mode:
           break;
       case compressed_mode:
       {
            compressed_attr_selectors_t  compressed_attr_selectors_v = 
                           global_meter_offset_mode[unit][svc_meter_mode].\
                                  meter_attr.compressed_attr_selectors_v;
            pkt_attr_bits_t  pkt_attr_bits_v = compressed_attr_selectors_v.\
                                                            pkt_attr_bits_v;

           /* Step3: Cleanup map array */
           if ((pkt_attr_bits_v.cng != 0) || (pkt_attr_bits_v.int_pri)) {
               SOC_IF_ERROR_RETURN(
                   soc_mem_clear(unit, ING_SVM_PRI_CNG_MAPm, MEM_BLOCK_ALL, TRUE));
           }
           if ((pkt_attr_bits_v.vlan_format != 0) || 
               (pkt_attr_bits_v.outer_dot1p) ||
               (pkt_attr_bits_v.inner_dot1p)) {
               SOC_IF_ERROR_RETURN(
                   soc_mem_clear(unit, ING_SVM_PKT_PRI_MAPm, MEM_BLOCK_ALL, TRUE));
           }
           if (pkt_attr_bits_v.ing_port != 0){
               SOC_IF_ERROR_RETURN(
                   soc_mem_clear(unit, ING_SVM_PORT_MAPm, MEM_BLOCK_ALL, TRUE));
           }
           if (pkt_attr_bits_v.tos != 0) {
               SOC_IF_ERROR_RETURN(
                   soc_mem_clear(unit, ING_SVM_TOS_MAPm, MEM_BLOCK_ALL, TRUE));
           }
           if ((pkt_attr_bits_v.pkt_resolution != 0) || 
                (pkt_attr_bits_v.svp_type) ||
                (pkt_attr_bits_v.drop)) {
               SOC_IF_ERROR_RETURN(
                  soc_mem_clear(unit, ING_SVM_PKT_RES_MAPm, MEM_BLOCK_ALL, TRUE));
           }
           break;
       }
       default:
           POLICER_ERR(("%s :Invalid offset mode\n", FUNCTION_NAME()));
           return BCM_E_PARAM;
    }
    global_meter_offset_mode[unit][svc_meter_mode].used = 0;
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_policer_svc_meter_get_available_mode
 * Purpose:
 *      Internal function to get a free meter offset table entry
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     svc_meter_mode        - (IN) Meter offset mode 
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_policer_svc_meter_get_available_mode(uint32 unit,
    bcm_policer_svc_meter_mode_t        *svc_meter_mode)
{
    uint32    mode_index = 1;
    for (; mode_index < BCM_POLICER_SVC_METER_MAX_MODE; mode_index++) {
        if (global_meter_offset_mode[unit][mode_index].used == 0) {
            *svc_meter_mode = mode_index;      
            return BCM_E_NONE;
        }
    }
    return BCM_E_FULL;
}

typedef enum pkt_attrs_e {
    pkt_attr_ip_pkt,
    pkt_attr_drop,
    pkt_attr_svp_group,
    pkt_attr_pkt_resolution,
    pkt_attr_tos,
    pkt_attr_ing_port,
    pkt_attr_inner_dot1p,
    pkt_attr_outer_dot1p,
    pkt_attr_vlan_format,
    pkt_attr_int_pri,
    pkt_attr_cng,
    pkt_attr_count
} pkt_attrs_t;

typedef struct pkt_attr_bit_pos_s {
    int start_bit;
    int end_bit;
} pkt_attr_bit_pos_t;

static pkt_attr_bit_pos_t  kt_pkt_attr_bit_pos[pkt_attr_count] =  {
                                       {0, 0}, {1, 1}, {2, 2}, {3, 8},
                                       {11,16}, {17,22}, {23,25}, {26,28},
                                       {29,30}, {31,34}, {35,36}
                                  };
static pkt_attr_bit_pos_t kt2_pkt_attr_bit_pos[pkt_attr_count] =  { 
                                       {0, 0}, {1, 1}, {2, 4}, {5, 10},
                                       {13,18}, {19, 26}, {27, 29}, {30,32},
                                       {33,34}, {35,38}, {39, 40}
                                  };

/*
 * Function:
 *      _bcm_policer_svc_meter_update_selector_keys
 * Purpose:
 *      Update the selector keys
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     mode_type_v           - (IN) Meter offset mode 
 *     pkt_attr_selector_key - (IN) Selector key
 *     pkt_attr_bits_v       - (IN) Packet attribute bits 
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_policer_svc_meter_update_selector_keys(
    int           unit,
    bcm_policer_svc_meter_mode_type_t    mode_type_v,
    soc_reg_t          pkt_attr_selector_key,
    pkt_attr_bits_t    pkt_attr_bits_v)
{
    uint64   pkt_attr_selector_key_value;
    uint8    current_bit_position=0;
    pkt_attr_bit_pos_t      *pkt_attr_bit_pos = NULL;

    if (SOC_IS_KATANA2(unit)) {
        pkt_attr_bit_pos = &kt2_pkt_attr_bit_pos[0];
    } else {
        pkt_attr_bit_pos = &kt_pkt_attr_bit_pos[0];
    }
 
    COMPILER_64_ZERO(pkt_attr_selector_key_value); 
    if (!((pkt_attr_selector_key >= ING_SVM_PKT_ATTR_SELECTOR_KEY_0r) && 
          (pkt_attr_selector_key <= ING_SVM_PKT_ATTR_SELECTOR_KEY_3r))) {
            POLICER_VVERB(("%s : Invalid Key for packet attribute selector\
                                                  \n", FUNCTION_NAME()));
            return BCM_E_PARAM;
    }
    /* BCM_POLICER_SVC_METER_UDF_MODE not supported here */
    if (!((mode_type_v == uncompressed_mode) ||
          (mode_type_v == compressed_mode))) {
            POLICER_VVERB(("%s : UDF mode not supported \n", FUNCTION_NAME()));
            return BCM_E_PARAM;
    }
    /* First Get complete value of ING_SVM_PKT_ATTR_SELECTOR_KEY_?r value */
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, 
            pkt_attr_selector_key, 
            REG_PORT_ANY, 
            0, 
            &pkt_attr_selector_key_value));

    /* Next set field value for 
                 ING_SVM_PKT_ATTR_SELECTOR_KEY_?r:SELECTOR_KEY field*/
    soc_reg64_field32_set( 
            unit, 
            pkt_attr_selector_key,
            &pkt_attr_selector_key_value,
            USER_SPECIFIED_UDF_VALIDf,
            0);
    soc_reg64_field32_set( 
            unit, 
            pkt_attr_selector_key,
            &pkt_attr_selector_key_value,
            USE_UDF_KEYf,
            0);
    if (mode_type_v == compressed_mode) {
        soc_reg64_field32_set( 
            unit, 
            pkt_attr_selector_key,
            &pkt_attr_selector_key_value,
            USE_COMPRESSED_PKT_KEYf,
            1);
    } else {
        soc_reg64_field32_set( 
            unit, 
            pkt_attr_selector_key,
            &pkt_attr_selector_key_value,
            USE_COMPRESSED_PKT_KEYf,
            0);
    }
    if (pkt_attr_bits_v.ip_pkt != 0) {
        BCM_IF_ERROR_RETURN(
          _bcm_policer_svc_meter_update_selector_keys_enable_fields(
            unit, 
            pkt_attr_selector_key, 
            &pkt_attr_selector_key_value,
            pkt_attr_bit_pos[pkt_attr_ip_pkt].start_bit, /*IP_PKT bit position*/
            pkt_attr_bits_v.ip_pkt,/*1:IP_PKT total bit*/
            &current_bit_position));
    }
    if (pkt_attr_bits_v.drop != 0) {
        BCM_IF_ERROR_RETURN(
          _bcm_policer_svc_meter_update_selector_keys_enable_fields(
            unit, 
            pkt_attr_selector_key, 
            &pkt_attr_selector_key_value,
            pkt_attr_bit_pos[pkt_attr_drop].start_bit, /*DROP bit position */
            pkt_attr_bits_v.drop, /*1:DROP total bits */
            &current_bit_position));
    }
    if (pkt_attr_bits_v.svp_type != 0) {
        BCM_IF_ERROR_RETURN(
          _bcm_policer_svc_meter_update_selector_keys_enable_fields(
            unit, 
            pkt_attr_selector_key, 
            &pkt_attr_selector_key_value,
            pkt_attr_bit_pos[pkt_attr_svp_group].start_bit, /* SVP_TYPE bit position */
            pkt_attr_bits_v.svp_type, 
            &current_bit_position));
    }
    if (pkt_attr_bits_v.pkt_resolution != 0) {
        BCM_IF_ERROR_RETURN(
          _bcm_policer_svc_meter_update_selector_keys_enable_fields(
            unit, 
            pkt_attr_selector_key, 
            &pkt_attr_selector_key_value,
            pkt_attr_bit_pos[pkt_attr_pkt_resolution].start_bit, /* PKT_RESOLUTION bit position */
            pkt_attr_bits_v.pkt_resolution,
            &current_bit_position));
    }
    if (pkt_attr_bits_v.tos != 0) {
        BCM_IF_ERROR_RETURN(
           _bcm_policer_svc_meter_update_selector_keys_enable_fields(
            unit, 
            pkt_attr_selector_key, 
            &pkt_attr_selector_key_value,
            pkt_attr_bit_pos[pkt_attr_tos].start_bit, /* TOS bit position */
            pkt_attr_bits_v.tos,
            &current_bit_position));
    }
    if (pkt_attr_bits_v.ing_port != 0) {
        BCM_IF_ERROR_RETURN(
          _bcm_policer_svc_meter_update_selector_keys_enable_fields(
            unit, 
            pkt_attr_selector_key, 
            &pkt_attr_selector_key_value,
            pkt_attr_bit_pos[pkt_attr_ing_port].start_bit, /* ING_PORT bit position */
            pkt_attr_bits_v.ing_port,
            &current_bit_position));
    }
    if (pkt_attr_bits_v.inner_dot1p != 0) {
        BCM_IF_ERROR_RETURN(
          _bcm_policer_svc_meter_update_selector_keys_enable_fields(
            unit,
            pkt_attr_selector_key, 
            &pkt_attr_selector_key_value,
            pkt_attr_bit_pos[pkt_attr_inner_dot1p].start_bit, /* INNER_DOT1P bit position */
            pkt_attr_bits_v.inner_dot1p,
            &current_bit_position));
    }
    if (pkt_attr_bits_v.outer_dot1p != 0) {
        BCM_IF_ERROR_RETURN(
          _bcm_policer_svc_meter_update_selector_keys_enable_fields(
            unit, 
            pkt_attr_selector_key, 
            &pkt_attr_selector_key_value,
            pkt_attr_bit_pos[pkt_attr_outer_dot1p].start_bit, /* OUTER_DOT1P bit position */
            pkt_attr_bits_v.outer_dot1p,
            &current_bit_position));
    }
    if (pkt_attr_bits_v.vlan_format != 0) {
        BCM_IF_ERROR_RETURN(
          _bcm_policer_svc_meter_update_selector_keys_enable_fields(
            unit, 
            pkt_attr_selector_key, 
            &pkt_attr_selector_key_value,
            pkt_attr_bit_pos[pkt_attr_vlan_format].start_bit, /* VLAN_FORMAT bit position*/
            pkt_attr_bits_v.vlan_format, 
            &current_bit_position));
    }
    if (pkt_attr_bits_v.int_pri != 0) {
        int start_bit = 0; 
        if ((mode_type_v == compressed_mode) && (SOC_IS_KATANA2(unit))) { 
            start_bit = 33;
        } else {
            start_bit = pkt_attr_bit_pos[pkt_attr_int_pri].start_bit; /*INT_PRI bit position*/
        }
        BCM_IF_ERROR_RETURN(
          _bcm_policer_svc_meter_update_selector_keys_enable_fields(
            unit, 
            pkt_attr_selector_key, 
            &pkt_attr_selector_key_value,
            start_bit, 
            pkt_attr_bits_v.int_pri,
            &current_bit_position));
    }
    if (pkt_attr_bits_v.cng != 0) {
        BCM_IF_ERROR_RETURN(
          _bcm_policer_svc_meter_update_selector_keys_enable_fields(
            unit, 
            pkt_attr_selector_key, 
            &pkt_attr_selector_key_value,
            pkt_attr_bit_pos[pkt_attr_cng].start_bit, /* CNG bit position */
            pkt_attr_bits_v.cng,
            &current_bit_position));
    }
    /* Finally set value for ING_SVM_PKT_ATTR_SELECTOR_KEY_?r */
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, 
            pkt_attr_selector_key, 
            REG_PORT_ANY, 
            0, 
            pkt_attr_selector_key_value));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_policer_svc_meter_create_cascade_mode
 * Purpose:
 *     Function to create an offset mode with uncompressed packet attributes
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     uncompressed_attr_selectors_v - (IN) Packet attribute selectors 
 *     group_mode            - (IN) Policer group mode
 *     num_pol               - (IN) Number of policers
 *     svc_meter_mode        - (OUT) Allocated meter offset mode 
 *     total_bits            - (OUT) Total number of packet attribute bits 
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_policer_svc_meter_create_cascade_mode(
    int unit,
    uncompressed_attr_selectors_t *uncompressed_attr_selectors_v,
    bcm_policer_group_mode_t     group_mode,
    int                          num_pol,
    bcm_policer_svc_meter_mode_t *svc_meter_mode)
{
    bcm_error_t                         bcm_error_v = BCM_E_NONE;
    bcm_policer_svc_meter_mode_t        bcm_policer_svc_meter_mode = 0;
    pkt_attr_bits_t     pkt_attr_bits_v = {0};
    uint32              total_bits_used = 0;    
    uint32              index = 0;
    bcm_policer_svc_meter_bookkeep_mode_t   mode_info;
    uncompressed_attr_selectors_t *un_attr;
    /*check if a mode is already configured with these attributes */
    for (index = 1; index < BCM_POLICER_SVC_METER_MAX_MODE; index++) {
        if (_bcm_policer_svc_meter_get_mode_info(unit,index,&mode_info) 
                                                         == BCM_E_NONE) {
            if ((mode_info.meter_attr.mode_type_v != cascade_mode) ||
                                 (group_mode != mode_info.group_mode )) {
                continue;
            }
            if (mode_info.no_of_policers != num_pol) {
                continue;
            }
            un_attr = &mode_info.meter_attr.uncompressed_attr_selectors_v;
            if (un_attr->uncompressed_attr_bits_selector ==
                uncompressed_attr_selectors_v->uncompressed_attr_bits_selector) {   
                *svc_meter_mode = index;
                return BCM_E_EXISTS;
            }
        }
    }

    bcm_error_v = _bcm_policer_svc_meter_get_available_mode(unit,
                                                &bcm_policer_svc_meter_mode);
    if (bcm_error_v != BCM_E_NONE) {
        POLICER_WARN((" Offset Table is full\n"));
        return bcm_error_v;
    }

    /* Step1: Packet Attribute selection */
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_CNG_ATTR_BITS) {
        pkt_attr_bits_v.cng = BCM_POLICER_SVC_METER_CNG_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_CNG_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS) {
        pkt_attr_bits_v.int_pri = BCM_POLICER_SVC_METER_INT_PRI_ATTR_SIZE ;
        total_bits_used += BCM_POLICER_SVC_METER_INT_PRI_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
         BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_VLAN_FORMAT_ATTR_BITS) {
        pkt_attr_bits_v.vlan_format = 
                            BCM_POLICER_SVC_METER_VLAN_FORMAT_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_VLAN_FORMAT_ATTR_SIZE; 
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS) {
        pkt_attr_bits_v.outer_dot1p = 
                               BCM_POLICER_SVC_METER_OUTER_DOT1P_ATTR_SIZE ;
        total_bits_used += BCM_POLICER_SVC_METER_OUTER_DOT1P_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS) {
            pkt_attr_bits_v.inner_dot1p = 
                               BCM_POLICER_SVC_METER_INNER_DOT1P_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_INNER_DOT1P_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INGRESS_PORT_ATTR_BITS) {
            pkt_attr_bits_v.ing_port = 
                                 BCM_POLICER_SVC_METER_ING_PORT_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_ING_PORT_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_TOS_ATTR_BITS) {
        pkt_attr_bits_v.tos = BCM_POLICER_SVC_METER_TOS_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_TOS_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS) {
        pkt_attr_bits_v.pkt_resolution = 
                             BCM_POLICER_SVC_METER_PKT_REOLUTION_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_PKT_REOLUTION_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS) {
        if (SOC_IS_KATANA2(unit)) {
            total_bits_used += BCM_POLICER_SVC_METER_SVP_NETWORK_GROUP_ATTR_SIZE;
            pkt_attr_bits_v.svp_type = 
                           BCM_POLICER_SVC_METER_SVP_NETWORK_GROUP_ATTR_SIZE;
        } else {
            total_bits_used += BCM_POLICER_SVC_METER_SVP_TYPE_ATTR_SIZE;  
            pkt_attr_bits_v.svp_type = BCM_POLICER_SVC_METER_SVP_TYPE_ATTR_SIZE;
        }  
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_DROP_ATTR_BITS) {
        pkt_attr_bits_v.drop = BCM_POLICER_SVC_METER_DROP_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_DROP_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS) {
        pkt_attr_bits_v.ip_pkt = BCM_POLICER_SVC_METER_IP_PKT_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_IP_PKT_ATTR_SIZE;    
    }
    if (total_bits_used > BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) {
        POLICER_ERR(("%s : Key size exceeds max allowed size \n", 
                                                      FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    /* Step2: Packet Attribute filling */
    BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_update_selector_keys(unit,
               uncompressed_mode,
               _pkt_attr_sel_key_reg[bcm_policer_svc_meter_mode],
               pkt_attr_bits_v));

    /* Step3: Offset table filling */
    BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_update_offset_table(unit,
                           SVM_OFFSET_TABLEm,
                           bcm_policer_svc_meter_mode,
                           &uncompressed_attr_selectors_v->offset_map[0]));

    /* Step4: Final: reserve mode and return */
    *svc_meter_mode = bcm_policer_svc_meter_mode;
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_policer_svc_meter_create_uncompress_mode
 * Purpose:
 *     Function to create an offset mode with uncompressed packet attributes
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     uncompressed_attr_selectors_v - (IN) Packet attribute selectors 
 *     group_mode            - (IN) Policer group mode
 *     svc_meter_mode        - (OUT) Allocated meter offset mode 
 *     total_bits            - (OUT) Total number of packet attribute bits 
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_policer_svc_meter_create_uncompress_mode(
    int unit,
    uncompressed_attr_selectors_t *uncompressed_attr_selectors_v,
    bcm_policer_group_mode_t     group_mode,
    bcm_policer_svc_meter_mode_t *svc_meter_mode)
{
    bcm_error_t                         bcm_error_v = BCM_E_NONE;
    bcm_policer_svc_meter_mode_t        bcm_policer_svc_meter_mode = 0;
    pkt_attr_bits_t     pkt_attr_bits_v = {0};
    uint32              total_bits_used = 0;    
    uint32              index = 0;
    bcm_policer_svc_meter_bookkeep_mode_t   mode_info;
    uncompressed_attr_selectors_t *un_attr;
    /*check if a mode is already configured with these attributes */
    for (index = 1; index < BCM_POLICER_SVC_METER_MAX_MODE; index++) {
        if (_bcm_policer_svc_meter_get_mode_info(unit,index,&mode_info) 
                                                         == BCM_E_NONE) {
            if ((mode_info.meter_attr.mode_type_v != uncompressed_mode) ||
                                 (group_mode != mode_info.group_mode )) {
                continue;
            }
            un_attr = &mode_info.meter_attr.uncompressed_attr_selectors_v;
            if (un_attr->uncompressed_attr_bits_selector ==
                uncompressed_attr_selectors_v->uncompressed_attr_bits_selector) {   
                *svc_meter_mode = index;
                return BCM_E_EXISTS;
            }
        }
    }

    bcm_error_v = _bcm_policer_svc_meter_get_available_mode(unit,
                                                &bcm_policer_svc_meter_mode);
    if (bcm_error_v != BCM_E_NONE) {
        POLICER_WARN((" Offset Table is full\n"));
        return bcm_error_v;
    }

    /* Step1: Packet Attribute selection */
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_CNG_ATTR_BITS) {
        pkt_attr_bits_v.cng = BCM_POLICER_SVC_METER_CNG_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_CNG_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS) {
        pkt_attr_bits_v.int_pri = BCM_POLICER_SVC_METER_INT_PRI_ATTR_SIZE ;
        total_bits_used += BCM_POLICER_SVC_METER_INT_PRI_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
         BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_VLAN_FORMAT_ATTR_BITS) {
        pkt_attr_bits_v.vlan_format = 
                            BCM_POLICER_SVC_METER_VLAN_FORMAT_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_VLAN_FORMAT_ATTR_SIZE; 
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS) {
        pkt_attr_bits_v.outer_dot1p = 
                               BCM_POLICER_SVC_METER_OUTER_DOT1P_ATTR_SIZE ;
        total_bits_used += BCM_POLICER_SVC_METER_OUTER_DOT1P_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS) {
            pkt_attr_bits_v.inner_dot1p = 
                               BCM_POLICER_SVC_METER_INNER_DOT1P_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_INNER_DOT1P_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INGRESS_PORT_ATTR_BITS) {
            pkt_attr_bits_v.ing_port = 
                                 BCM_POLICER_SVC_METER_ING_PORT_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_ING_PORT_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_TOS_ATTR_BITS) {
        pkt_attr_bits_v.tos = BCM_POLICER_SVC_METER_TOS_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_TOS_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS) {
        pkt_attr_bits_v.pkt_resolution = 
                             BCM_POLICER_SVC_METER_PKT_REOLUTION_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_PKT_REOLUTION_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS) {
        if (SOC_IS_KATANA2(unit)) {
            total_bits_used += BCM_POLICER_SVC_METER_SVP_NETWORK_GROUP_ATTR_SIZE;
            pkt_attr_bits_v.svp_type = 
                       BCM_POLICER_SVC_METER_SVP_NETWORK_GROUP_ATTR_SIZE;
        } else {
            total_bits_used += BCM_POLICER_SVC_METER_SVP_TYPE_ATTR_SIZE;    
            pkt_attr_bits_v.svp_type = BCM_POLICER_SVC_METER_SVP_TYPE_ATTR_SIZE;
        }
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_DROP_ATTR_BITS) {
        pkt_attr_bits_v.drop = BCM_POLICER_SVC_METER_DROP_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_DROP_ATTR_SIZE;    
    }
    if (uncompressed_attr_selectors_v->uncompressed_attr_bits_selector &
        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS) {
        pkt_attr_bits_v.ip_pkt = BCM_POLICER_SVC_METER_IP_PKT_ATTR_SIZE;
        total_bits_used += BCM_POLICER_SVC_METER_IP_PKT_ATTR_SIZE;    
    }
    if (total_bits_used > BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) {
        POLICER_ERR(("%s : Key size exceeds max allowed size \n", 
                                                      FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    /* Step2: Packet Attribute filling */
    BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_update_selector_keys(unit,
               uncompressed_mode,
               _pkt_attr_sel_key_reg[bcm_policer_svc_meter_mode],
               pkt_attr_bits_v));

    /* Step3: Offset table filling */
    BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_update_offset_table(unit,
                           SVM_OFFSET_TABLEm,
                           bcm_policer_svc_meter_mode,
                           &uncompressed_attr_selectors_v->offset_map[0]));

    /* Step4: Final: reserve mode and return */
    *svc_meter_mode = bcm_policer_svc_meter_mode;
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_policer_svc_meter_create_compress_mode
 * Purpose:
 *     Function to create an offset mode with compressed packet attributes
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     compressed_attr_selectors_v - (IN) Packet attribute selectors 
 *     group_mode            - (IN) Policer group mode
 *     svc_meter_mode        - (OUT) Allocated meter offset mode 
 *     total_bits            - (OUT) Total number of packet attribute bits 
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_policer_svc_meter_create_compress_mode(
    int unit,
    compressed_attr_selectors_t *compressed_attr_selectors_v,
    bcm_policer_group_mode_t group_mode,
    bcm_policer_svc_meter_mode_t *svc_meter_mode)
{
    bcm_error_t                     bcm_error_v=BCM_E_NONE;
    bcm_policer_svc_meter_mode_t    bcm_policer_svc_meter_mode = 0;
    pkt_attr_bits_t                 *pkt_attr_bits_v = NULL;
    pkt_attr_bits_t                 *config_pkt_attr = NULL;
    uint32                          total_bits_used = 0;    
    uint32                          index = 0;    
    bcm_policer_svc_meter_bookkeep_mode_t   *mode_info = NULL;
    pkt_attr_bits_t                 *pkt_attr_bits = NULL;
    uint32                           map_entry = 0; 
    int                             index_max = 0;
  
    mode_info = sal_alloc(sizeof(bcm_policer_svc_meter_bookkeep_mode_t), 
                                                   "mode bookkeep info");
    if ( mode_info == NULL) {
        POLICER_VERB(("%s : Failed to allocate memory for mode bookkeep \
                                  info\n", FUNCTION_NAME()));
        return BCM_E_MEMORY;
    }
    sal_memset(mode_info, 0, sizeof(bcm_policer_svc_meter_bookkeep_mode_t));

    config_pkt_attr = &compressed_attr_selectors_v->pkt_attr_bits_v;
    /*check if a mode is already configured with these attributes */
    for (index = 1; index < BCM_POLICER_SVC_METER_MAX_MODE; index++) {
        if (_bcm_policer_svc_meter_get_mode_info(
                             unit, index, mode_info) == BCM_E_NONE) {
            if ((mode_info->meter_attr.mode_type_v != compressed_mode) ||
               (group_mode != mode_info->group_mode )) {
                if (mode_info->meter_attr.mode_type_v == compressed_mode) {
                    POLICER_ERR(("%s : Some other policer group is already \
                                 using this resource\n", FUNCTION_NAME()));
                    sal_free(mode_info);
                    return BCM_E_PARAM;
                } 
                continue;
            }
            pkt_attr_bits = &mode_info->meter_attr.compressed_attr_selectors_v.\
                                                                pkt_attr_bits_v;
            if ((((pkt_attr_bits->cng) || (pkt_attr_bits->int_pri)) &&
                 ((config_pkt_attr->cng) || (config_pkt_attr->int_pri))) || \
                 (((pkt_attr_bits->vlan_format) || (pkt_attr_bits->outer_dot1p) ||
                   (pkt_attr_bits->inner_dot1p)) && 
                 ((config_pkt_attr->vlan_format) || (config_pkt_attr->outer_dot1p) ||
                 (config_pkt_attr->inner_dot1p))) || ((pkt_attr_bits->ing_port) && 
                 (config_pkt_attr->ing_port)) || 
                 ((pkt_attr_bits->tos) && (config_pkt_attr->tos)) || 
                 (((pkt_attr_bits->pkt_resolution) || \
                 (pkt_attr_bits->svp_type) || (pkt_attr_bits->drop)) && 
                 ((config_pkt_attr->pkt_resolution) || (config_pkt_attr->svp_type) || \
                 (config_pkt_attr->drop)))) {

                *svc_meter_mode = index;
                sal_free(mode_info);
                return BCM_E_EXISTS;

            }
        }
    } /* end of for */
    sal_free(mode_info);
    bcm_error_v = _bcm_policer_svc_meter_get_available_mode(unit,
                                                &bcm_policer_svc_meter_mode);
    if (bcm_error_v != BCM_E_NONE) {
        return bcm_error_v;
    }
    /* Step1: Packet Attribute selection */
    if (config_pkt_attr->cng != 0) {
        if (config_pkt_attr->cng > BCM_POLICER_SVC_METER_CNG_ATTR_SIZE) { 
            POLICER_VVERB(("%s :CNG attribute size exceeds max allowed size\n",\
                                                      FUNCTION_NAME()));
            return BCM_E_PARAM;
        }
        total_bits_used += config_pkt_attr->cng; 
    }
    if (config_pkt_attr->int_pri != 0) {
        if (config_pkt_attr->int_pri > BCM_POLICER_SVC_METER_INT_PRI_ATTR_SIZE) {
            POLICER_VVERB(("%s : int_pri attribute size exceeds max allowed \
                                               size\n", FUNCTION_NAME()));
            return BCM_E_PARAM;
        }
        total_bits_used += config_pkt_attr->int_pri; 
    }
    if (config_pkt_attr->vlan_format != 0) {
        if (config_pkt_attr->vlan_format > 
                                BCM_POLICER_SVC_METER_VLAN_FORMAT_ATTR_SIZE) { 
            POLICER_VVERB(("%s : Vlan attribute size exceeds max allowed \
                                               size\n", FUNCTION_NAME()));
            return BCM_E_PARAM;
        }
        total_bits_used += config_pkt_attr->vlan_format;    
    }
    if (config_pkt_attr->outer_dot1p != 0) {
        if (config_pkt_attr->outer_dot1p > 
                                BCM_POLICER_SVC_METER_OUTER_DOT1P_ATTR_SIZE) { 
            POLICER_VVERB(("%s : Outer DOT1P attribute size exceeds max allowed \
                                               size\n", FUNCTION_NAME()));
            return BCM_E_PARAM;
        }
        total_bits_used += config_pkt_attr->outer_dot1p;    
    }
    if (config_pkt_attr->inner_dot1p != 0) {
        if (config_pkt_attr->inner_dot1p >
                                BCM_POLICER_SVC_METER_INNER_DOT1P_ATTR_SIZE) { 
            POLICER_VVERB(("%s : Inner DOT1P attribute size exceeds max allowed \
                                               size\n", FUNCTION_NAME()));
            return BCM_E_PARAM;
        }
        total_bits_used += config_pkt_attr->inner_dot1p;    
    }
    if (config_pkt_attr->ing_port != 0) {
        if (config_pkt_attr->ing_port > 
                                 BCM_POLICER_SVC_METER_ING_PORT_ATTR_SIZE) {
            POLICER_VVERB(("%s : port attribute size exceeds max allowed \
                                               size\n", FUNCTION_NAME()));
            return BCM_E_PARAM;
        }
        total_bits_used += config_pkt_attr->ing_port;
    }
    if (config_pkt_attr->tos != 0) {
        if (config_pkt_attr->tos > BCM_POLICER_SVC_METER_TOS_ATTR_SIZE) { 
            POLICER_VVERB(("%s : TOS attribute size exceeds max allowed \
                                               size\n", FUNCTION_NAME()));
            return BCM_E_PARAM;
        }
        total_bits_used += config_pkt_attr->tos;
    }
    if (config_pkt_attr->pkt_resolution != 0) {
        if (config_pkt_attr->pkt_resolution > 
                              BCM_POLICER_SVC_METER_PKT_REOLUTION_ATTR_SIZE) { 
            POLICER_VVERB(("%s : Pkt resolution attribute size exceeds max \
                                allowed size\n", FUNCTION_NAME()));
            return BCM_E_PARAM;
        }
        total_bits_used += config_pkt_attr->pkt_resolution;
    }
    if (config_pkt_attr->svp_type != 0) {
        if (SOC_IS_KATANA2(unit)) {
            if (config_pkt_attr->svp_type > 
                           BCM_POLICER_SVC_METER_SVP_NETWORK_GROUP_ATTR_SIZE) { 
                POLICER_VVERB(("%s : SVP type attribute size exceeds max allowed \
                                                   size\n", FUNCTION_NAME()));
                return BCM_E_PARAM;
            }
        } else { 
            if (config_pkt_attr->svp_type > 
                                 BCM_POLICER_SVC_METER_SVP_TYPE_ATTR_SIZE) { 
                POLICER_VVERB(("%s : SVP type attribute size exceeds max allowed \
                                                   size\n", FUNCTION_NAME()));
                return BCM_E_PARAM;
            }
        }
        total_bits_used += config_pkt_attr->svp_type;
    }
    if (config_pkt_attr->drop != 0) {
        if (config_pkt_attr->drop > BCM_POLICER_SVC_METER_DROP_ATTR_SIZE) {
            POLICER_VVERB(("%s : DROP attribute size exceeds max allowed  \
                                               size\n", FUNCTION_NAME()));
            return BCM_E_PARAM;
        }
        total_bits_used += config_pkt_attr->drop;
    }
    if (config_pkt_attr->ip_pkt != 0) {
        if (config_pkt_attr->ip_pkt > BCM_POLICER_SVC_METER_IP_PKT_ATTR_SIZE) {
            POLICER_VVERB(("%s : IP pkt attribute size exceeds max allowed \
                                               size\n", FUNCTION_NAME()));
            return BCM_E_PARAM;
        }
        total_bits_used += config_pkt_attr->ip_pkt;
    }
    if (total_bits_used > BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE) {
        POLICER_VVERB(("%s : Key size exceeds max allowed  \
                                               size\n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    pkt_attr_bits_v = &compressed_attr_selectors_v->pkt_attr_bits_v;

    /* Step2: Fill up map array */
    if ((pkt_attr_bits_v->cng != 0) || (pkt_attr_bits_v->int_pri)) {
        index_max = soc_mem_index_max(unit, ING_SVM_PRI_CNG_MAPm);
        for (index = 0; index <= index_max; index++) {
            map_entry = 
               compressed_attr_selectors_v->compressed_pri_cnf_attr_map_v[index];
            soc_mem_write(
             unit, 
             ING_SVM_PRI_CNG_MAPm, 
             MEM_BLOCK_ALL, 
             index, 
             &map_entry);
        }
    }
    if ((pkt_attr_bits_v->vlan_format != 0) || (pkt_attr_bits_v->outer_dot1p) ||
                                              (pkt_attr_bits_v->inner_dot1p)) {
        index_max = soc_mem_index_max(unit, ING_SVM_PKT_PRI_MAPm);
         for (index = 0; index < BCM_SVC_METER_MAP_SIZE_256; index++) {
            map_entry = 
               compressed_attr_selectors_v->compressed_pkt_pri_attr_map_v[index];
            soc_mem_write(
             unit, 
             ING_SVM_PKT_PRI_MAPm, 
             MEM_BLOCK_ALL, 
             index, 
             &map_entry);
        }
        
    }
    if (pkt_attr_bits_v->ing_port != 0) {
        index_max = soc_mem_index_max(unit, ING_SVM_PORT_MAPm);
        for (index = 0; index < BCM_SVC_METER_PORT_MAP_SIZE; index++) {
            map_entry = 
               compressed_attr_selectors_v->compressed_port_attr_map_v[index];
            soc_mem_write(
             unit, 
             ING_SVM_PORT_MAPm, 
             MEM_BLOCK_ALL, 
             index, 
             &map_entry);
        }
    }
    if (pkt_attr_bits_v->tos != 0){
        index_max = soc_mem_index_max(unit, ING_SVM_TOS_MAPm);
        for (index = 0; index < BCM_SVC_METER_MAP_SIZE_256; index++) {
            map_entry = 
               compressed_attr_selectors_v->compressed_tos_attr_map_v[index];
            soc_mem_write(
                unit, 
                ING_SVM_TOS_MAPm, 
                MEM_BLOCK_ALL, 
                index, 
                &map_entry);
        }
    }
    if ((pkt_attr_bits_v->pkt_resolution != 0) || (pkt_attr_bits_v->svp_type) ||
                                                 (pkt_attr_bits_v->drop))   {
        index_max = soc_mem_index_max(unit, ING_SVM_PKT_RES_MAPm);
        for (index = 0; index < BCM_SVC_METER_MAP_SIZE_256; index++) {
            map_entry = 
               compressed_attr_selectors_v->compressed_pkt_res_attr_map_v[index];
            soc_mem_write(
             unit, 
             ING_SVM_PKT_RES_MAPm,
             MEM_BLOCK_ALL, 
             index, 
             &map_entry);
        }
    }

    /* Step3: Packet Attribute filling */
    BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_update_selector_keys(unit,
                 compressed_mode,
                 _pkt_attr_sel_key_reg[bcm_policer_svc_meter_mode],
                 *pkt_attr_bits_v));

    /* Step4: Offset table filling */
    BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_update_offset_table(unit,
                            SVM_OFFSET_TABLEm,
                            bcm_policer_svc_meter_mode,
                            &compressed_attr_selectors_v->offset_map[0]));

    /* Step5: Final: reserve mode and return */
    *svc_meter_mode = bcm_policer_svc_meter_mode;
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_policer_svc_meter_create_udf_mode
 * Purpose:
 *     Function to create an offset mode with udf packet attributes
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     udf_pkt_attr          - (IN) Packet attribute selectors 
 *     group_mode            - (IN) Policer group mode
 *     svc_meter_mode        - (OUT) Allocated meter offset mode 
 *     total_bits            - (OUT) Total number of packet attribute bits 
 * Returns:
 *     BCM_E_XXX 
 */
bcm_error_t _bcm_policer_svc_meter_create_udf_mode(
    int unit,
    udf_pkt_attr_selectors_t     *udf_pkt_attr,
    bcm_policer_group_mode_t     group_mode,
    bcm_policer_svc_meter_mode_t *svc_meter_mode,
    uint32                       *total_bits)
{
    bcm_error_t                  bcm_error_v = BCM_E_NONE;
    bcm_policer_svc_meter_mode_t bcm_policer_svc_meter_mode = 0;
    uint32                       total_udf_bits = 0;
    bcm_policer_svc_meter_bookkeep_mode_t   mode_info;
    udf_pkt_attr_selectors_t     *udf_attr = NULL;
    uint32                       index = 0;    
    /*check if a mode is already configured with these attributes */
    for (index = 1;index < BCM_POLICER_SVC_METER_MAX_MODE; index++) {
        if (_bcm_policer_svc_meter_get_mode_info(unit, index, &mode_info) == 
                                                                 BCM_E_NONE) {
            if ((mode_info.meter_attr.mode_type_v != udf_mode) ||
               (group_mode != mode_info.group_mode )) {
                continue;
            }
            udf_attr = &mode_info.meter_attr.udf_pkt_attr_selectors_v;
            if ((udf_attr->udf_pkt_attr_bits_v.udf0 == 
                         udf_pkt_attr->udf_pkt_attr_bits_v.udf0) && 
               (udf_attr->udf_pkt_attr_bits_v.udf1 ==
                                udf_pkt_attr->udf_pkt_attr_bits_v.udf1))
            {                                
                *svc_meter_mode = index;
                return BCM_E_EXISTS;
            }
        }
    }

    bcm_error_v = _bcm_policer_svc_meter_get_available_mode(unit, 
                                           &bcm_policer_svc_meter_mode);
    if (bcm_error_v != BCM_E_NONE) {
        return bcm_error_v;
    }
    /* Step1: Packet Attribute filling */
    BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_update_udf_selector_keys(
                unit,
                _pkt_attr_sel_key_reg[bcm_policer_svc_meter_mode],
                *udf_pkt_attr,
                &total_udf_bits));

    /* Step2: Offset table filling */
    BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_update_offset_table(unit,
                        SVM_OFFSET_TABLEm,
                        bcm_policer_svc_meter_mode,
                        NULL));

    /* Step3: Final: reserve mode and return */
    *total_bits = total_udf_bits;
    *svc_meter_mode = bcm_policer_svc_meter_mode;
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_policer_svc_meter_create_mode
 * Purpose:
 *     Function to create an Meter offset mode
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     meter_attr            - (IN) Packet attribute selectors 
 *     group_mode            - (IN) Policer group mode
 *     num_pol               - (IN) Number of policers
 *     svc_meter_mode        - (OUT) Allocated meter offset mode 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_policer_svc_meter_create_mode (
    int unit,
    bcm_policer_svc_meter_attr_t *meter_attr,
    bcm_policer_group_mode_t group_mode,
    int num_pol,
    bcm_policer_svc_meter_mode_t *svc_meter_mode)
{
    uint32    total_bits;
    int rv = BCM_E_NONE;
     
    switch (meter_attr->mode_type_v)
    {
        case uncompressed_mode:
            rv = _bcm_policer_svc_meter_create_uncompress_mode(
                    unit,
                    &meter_attr->uncompressed_attr_selectors_v,
                    group_mode, svc_meter_mode);
            if (BCM_FAILURE(rv) && (rv == BCM_E_EXISTS)) {
                return rv;
            }
            break;
        case compressed_mode:
            BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_create_compress_mode(
                    unit,
                    &meter_attr->compressed_attr_selectors_v,
                    group_mode, svc_meter_mode));
            break;
        case udf_mode:
            BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_create_udf_mode(
                    unit,
                    &meter_attr->udf_pkt_attr_selectors_v,
                    group_mode, svc_meter_mode, &total_bits));
            break;
        case cascade_mode:
            BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_create_cascade_mode(
                    unit,
                    &meter_attr->uncompressed_attr_selectors_v,
                    group_mode, num_pol, svc_meter_mode));
            break;

        default:
            POLICER_VVERB(("%s :  Invalid offset mode\n", FUNCTION_NAME()));
            return BCM_E_PARAM;
    }
    if (BCM_FAILURE(rv) && ((rv == BCM_E_FULL) || BCM_E_PARAM)) {
        return rv;
    }
    BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_reserve_mode(unit,
                                    *svc_meter_mode,
                                    group_mode,
                                    meter_attr));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_policer_svc_meter_delete_mode
 * Purpose:
 *      Delete a meter offset mode
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     svc_meter_mode        - (IN) offset mode 
 * Returns:
 *     BCM_E_XXX 
 */
int  _bcm_esw_policer_svc_meter_delete_mode(
        int                                  unit,
        bcm_policer_svc_meter_mode_t         svc_meter_mode)
{
    if (svc_meter_mode == 0) {
        POLICER_VVERB(("%s : Invalid mode passed: %d \n", \
                                      FUNCTION_NAME(), svc_meter_mode));
        return BCM_E_PARAM; /*0 is Reserved */
    }
    if (svc_meter_mode > (BCM_POLICER_SVC_METER_MAX_MODE - 1)) {
        POLICER_VVERB(("%s : Invalid mode passed: %d \n", \
                                      FUNCTION_NAME(), svc_meter_mode));
        return BCM_E_PARAM; /* Exceeding max allowed value */
    }
    return _bcm_policer_svc_meter_unreserve_mode(unit, svc_meter_mode);
} 

/*
 * Function:
 *      _bcm_esw_get_policer_id_from_index_offset
 * Purpose:
 *      Get policer id, given the HW policer index and offset_mode
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     index                 - (IN) HW policer index 
 *     offset_mode           - (IN) offset mode 
 *     pid                   - (OUT) policer Id 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_get_policer_id_from_index_offset(int unit, int index, 
                                              int offset_mode, 
                                              bcm_policer_t *pid)
{
    int pool=0, size_of_pool=0;
    size_of_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
    if (index > 0) {
       pool = index / size_of_pool;
       index = index % size_of_pool;
       *pid = (((offset_mode + 1) << BCM_POLICER_GLOBAL_METER_MODE_SHIFT) +
                    (pool << _shr_popcount(size_of_pool - 1)) + index);
    } else {
       *pid = 0;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_get_policer_table_index
 * Purpose:
 *     Get the HW policer index, given a policer id.
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     policer               - (IN) policer Id 
 *     index                 - (OUT) HW policer index 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_get_policer_table_index(int unit, bcm_policer_t policer,
                                                         int *index) 
{
    int rv = BCM_E_NONE;
    int size_pool = 0, num_pools = 0;
    int pool=0;
    int offset_mask = 0; 
    int pool_mask = 0, pool_offset = 0;
    size_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
    num_pools =  SOC_INFO(unit).global_meter_pools;
    offset_mask = size_pool - 1;
    pool_offset = _shr_popcount(offset_mask);
    pool_mask = (num_pools - 1) << pool_offset; 
    pool = ((policer & pool_mask) >> pool_offset); 
    *index = ((policer & offset_mask) + (pool * size_pool));
    return rv;
}

/*
 * Function:
 *      _bcm_esw_global_meter_cleanup
 * Purpose:
 *       Cleanup the global meter(service meter) internal data structure on
 *       init failure.
 * Parameters:
 *     Unit - unit number 
 * Returns:
 *     BCM_E_XXX 
 */
int
_bcm_esw_global_meter_cleanup(int unit)
{
    int rv = BCM_E_NONE;
    int pool_id = 0, num_pools = 0;
    
    if (0 == global_meter_status[unit].initialised) {
        POLICER_VERB(("%s : Global meter feature not initialized for \
                              unit %d\n",FUNCTION_NAME(), unit));
        return rv;
    }
    /* mutex destroy */
    if (NULL != global_meter_mutex[unit]) {
        sal_mutex_destroy(global_meter_mutex[unit]);
        global_meter_mutex[unit] = NULL;
    }
    /* Free handles of index management for meter action */
    if (NULL != meter_action_list_handle[unit]) {
        shr_aidxres_list_destroy(meter_action_list_handle[unit]);
        meter_action_list_handle[unit] = NULL;
    }
    /* Free handles of index management for meter action */
    num_pools = SOC_INFO(unit).global_meter_pools;
    for (pool_id = 0; pool_id < num_pools; pool_id++) {
        if (NULL != meter_alloc_list_handle[unit][pool_id]) {
            shr_aidxres_list_destroy(meter_alloc_list_handle[unit][pool_id]);
            meter_alloc_list_handle[unit][pool_id] = NULL;
        }
    /* Free handles of index management for meter action */
    }
    if (NULL != global_meter_policer_bookkeep[unit]) {
        sal_free(global_meter_policer_bookkeep[unit]);
        global_meter_policer_bookkeep[unit] = NULL;
    }
    if (NULL != global_meter_hz_alloc_bookkeep[unit]) {
        sal_free(global_meter_hz_alloc_bookkeep[unit]);
        global_meter_hz_alloc_bookkeep[unit] = NULL;
    }
    if (NULL != global_meter_action_bookkeep[unit]) {
        sal_free(global_meter_action_bookkeep[unit]);
        global_meter_action_bookkeep[unit] = NULL;
    }
    global_meter_status[unit].initialised = 0;
    POLICER_VERB(("%s : Clening up global meter config\n", FUNCTION_NAME()));
    return rv;
}

/*
 * Function:
 *      bcm_esw_global_meter_init
 * Purpose:
 *     Initialise service meter data-structure 
 * Parameters:
 *     Unit - unit number 
 * Returns:
 *     BCM_E_XXX 
 */
int 
bcm_esw_global_meter_init(int unit)
{
    int rv = BCM_E_NONE;
    uint32 service_meter_control = 0;
    uint32 ing_svm_control = 0;
    uint32 aux_arb_control = 0;
    int mode = 0;
    uint32 num_pools = 0, size_pool = 0, total_size = 0, action_size = 0;
    uint32 max_size_pool = 0;
    uint32 pool_id = 0, index = 0;
#ifdef BCM_WARM_BOOT_SUPPORT
    int size = 0;
    soc_scache_handle_t scache_handle;
    uint8  *policer_state;
#endif
    svm_policy_table_entry_t meter_action;
    svm_macroflow_index_table_entry_t macro_flow_entry;
    if (1 == global_meter_status[unit].initialised) {
        POLICER_VERB(("%s : GLOBAL METER FEATURE already \
                         initialised for unit %d \n", FUNCTION_NAME(), unit));
        return rv;
    }

    if (!soc_feature(unit, soc_feature_global_meter)) {
        return BCM_E_UNAVAIL;
    }

    if (!soc_property_get(unit, spn_GLOBAL_METER_CONTROL, 1)) {
        service_meter_control = BCM_POLICER_GLOBAL_METER_DISABLE;
    }
    SOC_IF_ERROR_RETURN(soc_reg32_get( unit,
        ING_SVM_CONTROLr,
        REG_PORT_ANY,
        0,
        &ing_svm_control));

    soc_reg_field_set(unit,
        ING_SVM_CONTROLr,
        &ing_svm_control,
        BLOCK_DISABLEf,
        service_meter_control);

    SOC_IF_ERROR_RETURN(soc_reg32_set(unit,
        ING_SVM_CONTROLr,
        REG_PORT_ANY,
        0,
        ing_svm_control));

    if ( BCM_POLICER_GLOBAL_METER_DISABLE == service_meter_control) {
        global_meter_status[unit].initialised = 0;
        POLICER_WARN(("%s : GLOBAL METER FEATURE disabled \
                         at HW for unit %d\n", FUNCTION_NAME(), unit));
        return rv;
    }
    num_pools =  SOC_INFO(unit).global_meter_pools;
    size_pool =  SOC_INFO(unit).global_meter_size_of_pool;
    max_size_pool = SOC_INFO(unit).global_meter_max_size_of_pool;
    total_size = num_pools * max_size_pool;   
    if ((total_size) > (soc_mem_index_max(unit, SVM_METER_TABLEm) + 1))
    {
        POLICER_ERR(("%s : Number of global meters exceeding \
                       its max value for unit %d \n", FUNCTION_NAME(), unit));
        return BCM_E_INTERNAL;
    } 

    action_size = SOC_INFO(unit).global_meter_action_size;
    if ((action_size) > (soc_mem_index_max(unit, SVM_POLICY_TABLEm) + 1))
    {
        POLICER_ERR(("%s : Global meter actions exceeding \
                       its max value for unit %d \n", FUNCTION_NAME(), unit));
        return BCM_E_INTERNAL;
    } 
 
    /* Init the offset table 0 such that it can be used for non-offset
      based metering */
    BCM_IF_ERROR_RETURN(_bcm_policer_svc_meter_update_offset_table(unit,
                        SVM_OFFSET_TABLEm, 0, NULL));
    
    /* Initialize bookkeeping data structure */
    for (mode=1; mode < BCM_POLICER_SVC_METER_MAX_MODE; mode++) {
        sal_memset(&global_meter_offset_mode[unit][mode], 
                     0, sizeof(bcm_policer_svc_meter_bookkeep_mode_t));
    } 

     /* Allocate policer lookup hash. */
    _GLOBAL_METER_XGS3_ALLOC(global_meter_policer_bookkeep[unit], 
               _GLOBAL_METER_HASH_SIZE * \
               sizeof(_global_meter_policer_control_t *), "Global meter hash");
    if (NULL == global_meter_policer_bookkeep[unit]) {
        _bcm_esw_global_meter_cleanup(unit);
        POLICER_ERR(("%s : Unable to allocate memory for policer bookkeep \
                        data structure for unit %d \n", FUNCTION_NAME(), unit));
        return (BCM_E_MEMORY);
    }
     /* Allocate horizontal allocation management data structure */
    _GLOBAL_METER_XGS3_ALLOC(global_meter_hz_alloc_bookkeep[unit], 
                      size_pool * sizeof(_global_meter_horizontal_alloc_t), 
                                            "Global meter horizontal alloc");
    if (NULL == global_meter_hz_alloc_bookkeep[unit]) {
        _bcm_esw_global_meter_cleanup(unit);
        POLICER_ERR(("%s : Unable to allocate memory for hz alloc bookkeep \
                        data structure for unit %d \n", FUNCTION_NAME(), unit));
        return (BCM_E_MEMORY);
    }

    for (index = 0; index < size_pool; index++) {
        global_meter_hz_alloc_bookkeep[unit][index].alloc_bit_map = 0xff;
    }

     /* Allocate meter action management data structure */
    _GLOBAL_METER_XGS3_ALLOC(global_meter_action_bookkeep[unit], 
                   action_size * sizeof(bcm_policer_global_meter_action_bookkeep_t), 
                                            "Global meter action alloc");
    if (NULL == global_meter_action_bookkeep[unit]) {
        _bcm_esw_global_meter_cleanup(unit);
        POLICER_ERR(("%s : Unable to allocate memory for meter action bookkeep \
                        data structure for unit %d \n", FUNCTION_NAME(), unit));
        return (BCM_E_MEMORY);
    }
    if (global_meter_mutex[unit] == NULL) {
        global_meter_mutex[unit] = sal_mutex_create("Global meter mutex");
        if (global_meter_mutex[unit] == NULL) {
            _bcm_esw_global_meter_cleanup(unit);
            POLICER_ERR(("%s : Unable to create mutex for unit %d \n", \
                                                 FUNCTION_NAME(), unit));
            return BCM_E_MEMORY;
        }
    }

    /* Index management for meter action */
    if ( NULL != meter_action_list_handle[unit]) {
        shr_aidxres_list_destroy(meter_action_list_handle[unit]);
        meter_action_list_handle[unit] = NULL;
    }
    /* Create it */
    if (shr_aidxres_list_create(
                      &meter_action_list_handle[unit],
                      0,(action_size - 1),
                      0,(action_size - 1),
                      8,                                          
                      "global_meter_action") != BCM_E_NONE) {
        _bcm_esw_global_meter_cleanup(unit);
        POLICER_ERR(("%s : Couldn't create alligned list \
                for global meter action for unit %d\n", FUNCTION_NAME(), unit));
        return BCM_E_INTERNAL;                         
    }
    /* Reserve the first entry in action table */
    rv = shr_aidxres_list_reserve_block(
                          meter_action_list_handle[unit],0,1); 
    if (BCM_FAILURE(rv)) {
        _bcm_esw_global_meter_cleanup(unit);
        POLICER_ERR(("%s : Not able to reserve first entry \
                in action table for unit %d\n", FUNCTION_NAME(), unit));
        return (rv);
    }

    /* Index management for policer */
    for (pool_id = 0; pool_id < num_pools; pool_id++) {
        if ( NULL != meter_alloc_list_handle[unit][pool_id]) {
            shr_aidxres_list_destroy(
                           meter_alloc_list_handle[unit][pool_id]);
            meter_alloc_list_handle[unit][pool_id] = NULL;
        }
        /* Create it */
        if (shr_aidxres_list_create(
                      &meter_alloc_list_handle[unit][pool_id],
                      0, (size_pool - 1), 0, (size_pool - 1),
                      8, "global_meter_policer") != BCM_E_NONE) 
        {
            _bcm_esw_global_meter_cleanup(unit);
            POLICER_ERR(("%s : Couldn't create alligned list for \
                 global meter policer for unit %d\n", FUNCTION_NAME(), unit));
            return BCM_E_INTERNAL;                         
        }
        /* Reserve the first entry in policer table */
        rv = shr_aidxres_list_reserve_block(
                         meter_alloc_list_handle[unit][pool_id], 0, 1); 
        if (BCM_FAILURE(rv)) {
            _bcm_esw_global_meter_cleanup(unit);
            POLICER_ERR(("%s : Not able to reserve first entry \
                    in policer table for unit %d\n", FUNCTION_NAME(), unit));
            return (rv);
        }
        rv = _bcm_global_meter_reserve_bloc_horizontally(unit,pool_id, 0);
        if (!BCM_SUCCESS(rv)) {
            rv = shr_aidxres_list_free(meter_alloc_list_handle[unit]\
                                                              [pool_id], 0);
            if (!BCM_SUCCESS(rv)) {
                _bcm_esw_global_meter_cleanup(unit);
                POLICER_ERR(("%s : Not able to reserve first entry \
                    in policer table for unit %d\n", FUNCTION_NAME(), unit));
                return BCM_E_INTERNAL;
            }
        }
    } 
#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        rv = _bcm_esw_global_meter_reinit(unit);
        if (!BCM_SUCCESS(rv)) {
            _bcm_esw_global_meter_cleanup(unit);
            POLICER_ERR(("%s : Warm boot init failed for unit %d \n",\
                                          FUNCTION_NAME(), unit));
            return BCM_E_INTERNAL;
        }
    } else { /* cold boot */
        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_POLICER, 0);
        size = sizeof(int32) * BCM_GLOBAL_METER_MAX_SCACHE_ENTRIES;
        rv = _bcm_esw_scache_ptr_get(unit, scache_handle, TRUE,
                                     size, &policer_state,
                                     BCM_WB_DEFAULT_VERSION, NULL);
        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
            POLICER_VVERB(("%s : Scache Memory not available\n", \
                                                  FUNCTION_NAME()));
            return rv;
        }
        rv = BCM_E_NONE;
    }
#endif
    global_meter_status[unit].initialised = 1;
    POLICER_VERB(("%s :  GLOBAL METER FEATURE initialised \
                        for unit %d \n", FUNCTION_NAME(), unit));

    if (SOC_IS_KATANAX(unit)) {
        /* SW WAR for avoiding continuous parity errors coming from
           SVM_POLICY_TABLE and SVM_MACROFLOW_INDEX_TABLE entry 0 */
        rv = READ_SVM_POLICY_TABLEm(unit, MEM_BLOCK_ANY, 0, &meter_action);
        rv = READ_SVM_MACROFLOW_INDEX_TABLEm(unit, MEM_BLOCK_ANY,
                                     0, &macro_flow_entry); 
        rv = WRITE_SVM_POLICY_TABLEm(unit,MEM_BLOCK_ANY,0, &meter_action);
        rv = WRITE_SVM_MACROFLOW_INDEX_TABLEm(unit, MEM_BLOCK_ANY,
                                     0, &macro_flow_entry); 
    }
    /* Set MASTER_REFRESH_RATE and MASTER_REFRESH_ENABLE fields in
         AUX_ARB_CONTROL_2 register */
    SOC_IF_ERROR_RETURN(soc_reg32_get(unit, AUX_ARB_CONTROL_2r,
                                      REG_PORT_ANY, 0, &aux_arb_control));
    if (SOC_REG_FIELD_VALID(unit, AUX_ARB_CONTROL_2r, 
                            SVC_MTR_REFRESH_ENABLEf)) {
        soc_reg_field_set(unit, AUX_ARB_CONTROL_2r, &aux_arb_control,
                          SVC_MTR_REFRESH_ENABLEf, 1);
    }
    if (SOC_REG_FIELD_VALID(unit, AUX_ARB_CONTROL_2r, 
                            SVM_MASTER_REFRESH_RATEf)) {
        soc_reg_field_set(unit, AUX_ARB_CONTROL_2r, &aux_arb_control,
                          SVM_MASTER_REFRESH_RATEf, 1);
    }
    if (SOC_REG_FIELD_VALID(unit, AUX_ARB_CONTROL_2r, 
                            SVM_MASTER_REFRESH_ENABLEf)) {
        soc_reg_field_set(unit, AUX_ARB_CONTROL_2r, &aux_arb_control,
                          SVM_MASTER_REFRESH_ENABLEf, 1);
    }
    SOC_IF_ERROR_RETURN(soc_reg32_set(unit, AUX_ARB_CONTROL_2r,
                                      REG_PORT_ANY, 0, aux_arb_control));
    return rv;
} 

/*
 * Function:
 *      _bcm_esw_policer_validate
 * Purpose:
 *      Validate the given policer id
 * Parameters:
 *     Unit                  - (IN) unit number 
 *     policer               - (IN) policer Id 
 * Returns:
 *     BCM_E_XXX 
 */
int   _bcm_esw_policer_validate(int unit, bcm_policer_t *policer) 
{
    int rv = BCM_E_NONE;
    int index = 0;
    uint32 offset_mode = 0;
    if( *policer == 0) {
        POLICER_VERB(("%s : Policer id passed is 0 n", FUNCTION_NAME()));
        return rv;
    }
    _bcm_esw_get_policer_table_index(unit, *policer, &index); 
    if (index > (soc_mem_index_max(unit, SVM_METER_TABLEm)))
    {
        POLICER_VVERB(("%s : Invalid table index for SVM_POLICY_TABLE\n", \
                                                        FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    offset_mode = ((*policer & BCM_POLICER_GLOBAL_METER_MODE_MASK) >>
                                  BCM_POLICER_GLOBAL_METER_MODE_SHIFT);
    if (offset_mode >= 1) {
        offset_mode = offset_mode - 1;
    }
    if (offset_mode > BCM_POLICER_SVC_METER_MAX_MODE)
    {
        POLICER_VERB(("%s : Invalid Offset mode \n", FUNCTION_NAME()));
        return BCM_E_PARAM;

    }  
    return rv;
}

/*
 * Function:
 *      _bcm_esw_policer_increment_ref_count
 * Purpose:
 *      Increment the policer usage reference count
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer               - (IN) policer Id 
 * Returns:
 *     BCM_E_XXX 
 */
int  _bcm_esw_policer_increment_ref_count(int unit, bcm_policer_t policer) 
{
    int rv = BCM_E_NONE;
    _global_meter_policer_control_t *policer_control = NULL;
    /* validate policer id */
    rv = _bcm_esw_policer_validate(unit, &policer);
    if (BCM_FAILURE(rv)) {
        POLICER_VERB(("%s : Invalid policer id passed: %x \n", \
                                      FUNCTION_NAME(), policer));
        return (rv);
    }
    GLOBAL_METER_LOCK(unit);
    rv = _bcm_global_meter_base_policer_get(unit, policer, &policer_control);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_VERB(("%s : Unable to get policer control for policer \
                                     id %d \n", FUNCTION_NAME(), policer));
        return (rv);
    }
    policer_control->ref_count++;
    GLOBAL_METER_UNLOCK(unit);
    return rv;
} 

/*
 * Function:
 *      _bcm_esw_policer_decrement_ref_count
 * Purpose:
 *     Decrement the policer usage refernce count
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer               - (IN) policer Id 
 * Returns:
 *     BCM_E_XXX 
 */
int  _bcm_esw_policer_decrement_ref_count(int unit, bcm_policer_t policer) 
{
    int rv = BCM_E_NONE;
    _global_meter_policer_control_t *policer_control = NULL;
    /* validate policer id */
    rv = _bcm_esw_policer_validate(unit, &policer);
    if (BCM_FAILURE(rv)) {
        POLICER_VERB(("%s : Invalid policer id passed: %x \n", \
                                      FUNCTION_NAME(), policer));
        return (rv);
    }
    GLOBAL_METER_LOCK(unit);
    rv = _bcm_global_meter_base_policer_get(unit, policer, &policer_control);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_VERB(("%s : Unable to get policer control for policer \
                                     id %d \n", FUNCTION_NAME(), policer));
        return (rv);
    }
    if (policer_control->ref_count > 0) {
        policer_control->ref_count--;
    }
    GLOBAL_METER_UNLOCK(unit);
    return rv;
} 

/*
 * Function:
 *      _bcm_esw_add_policer_to_table
 * Purpose:
 *      Add policer id to the table entry specified 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer               - (IN) policer Id 
 *     table                 - (IN) Table to which the entry needs to be added 
 *     index                 - (IN) index in the table
 *     data                  - (IN) Data to be written
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_add_policer_to_table(int unit,bcm_policer_t policer,
                               soc_mem_t table, int index, void *data) 
{
    int offset_mode = 0;
    int rv = BCM_E_NONE;
    bcm_policer_t current_policer = 0;
    if (soc_feature(unit, soc_feature_global_meter)) {
         /* validate policer id */
        rv = _bcm_esw_policer_validate(unit, &policer);
        if (BCM_FAILURE(rv)) {
            POLICER_VERB(("%s : Invalid policer id passed: %x \n", \
                                      FUNCTION_NAME(), policer));
            return (rv);
        }
        switch (table) {
            case VFIm:
            case SOURCE_VPm:
            case VLAN_XLATEm:
            case VLAN_TABm:
            case PORT_TABm:
#ifdef BCM_TRIUMPH3_SUPPORT
            case VLAN_XLATE_EXTDm:
#endif
                rv = _bcm_esw_get_policer_from_table(unit, table, index, data, 
                                                    &current_policer, 1); 
                if (BCM_FAILURE(rv)) {
                    POLICER_VERB(("%s : Unable to read the table entry %d at\
                             index %d \n", FUNCTION_NAME(), table, index));
                    return (rv);
                }
                /* Set policer id */
                if ((policer & BCM_POLICER_GLOBAL_METER_INDEX_MASK) > 0) {
                    offset_mode = 
                          ((policer & BCM_POLICER_GLOBAL_METER_MODE_MASK) >>
                                    BCM_POLICER_GLOBAL_METER_MODE_SHIFT);
                    if (offset_mode >= 1) {
                        offset_mode = offset_mode - 1;
                    } 
                    if (offset_mode >= 
                               (soc_mem_index_count(unit,SVM_OFFSET_TABLEm) >>
                               BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE))  {
                        POLICER_VERB(("%s : Offset mode for the policer \
                              exceeds max allowed value \n", FUNCTION_NAME()));
                        return BCM_E_PARAM;
                    }
                    _bcm_esw_get_policer_table_index(unit, policer, &index); 
                } else {
                    index = 0;
                    offset_mode = 0;
                }
                if (index >= soc_mem_index_count(unit, SVM_METER_TABLEm))  {
                    POLICER_VERB(("%s : Invalid table index passed for \
                             SVM_METER_TABLE\n", FUNCTION_NAME()));
                    return BCM_E_PARAM;
                }
                if (SOC_IS_TRIUMPH3(unit) && (table == VLAN_XLATE_EXTDm)) {
                    if (SOC_MEM_FIELD_VALID(unit, table, 
                                           XLATE__SVC_METER_OFFSET_MODEf)) {
                        soc_mem_field32_set(unit,table, data, 
                                   XLATE__SVC_METER_OFFSET_MODEf,offset_mode);
                    }
                    if (SOC_MEM_FIELD_VALID(unit, table, 
                                                 XLATE__SVC_METER_INDEXf)) {
                        soc_mem_field32_set(unit,table, data, 
                                           XLATE__SVC_METER_INDEXf, index);
                    }

                } else {
                    if (SOC_MEM_FIELD_VALID(unit, table, SVC_METER_OFFSET_MODEf)) {
                        soc_mem_field32_set(unit,table, data, 
                                           SVC_METER_OFFSET_MODEf,offset_mode);
                    }
                    if (SOC_MEM_FIELD_VALID(unit, table, SVC_METER_INDEXf)) {
                        soc_mem_field32_set(unit,table, data, 
                                             SVC_METER_INDEXf, index);
                    }
                }
                break; 

            case VFP_POLICY_TABLEm:
                break; 
            default:
                  break; 
        }
        /* decrement current policer if any */
        if ((current_policer & BCM_POLICER_GLOBAL_METER_INDEX_MASK) > 0) {
            rv = _bcm_esw_policer_decrement_ref_count(unit, current_policer);
            BCM_IF_ERROR_RETURN(rv);
        }
        /* increment new policer  */
        if ((policer & BCM_POLICER_GLOBAL_METER_INDEX_MASK) > 0) {
            rv = _bcm_esw_policer_increment_ref_count(unit, policer);
            BCM_IF_ERROR_RETURN(rv);
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_esw_get_policer_from_table
 * Purpose:
 *      Read the policer id configured in the given table
 * Parameters:
 *     unit                  - (IN) unit number 
 *     table                 - (IN) Table from which the entry needs to be got 
 *     index                 - (IN) index in the table
 *     data                  - (IN) Data to be written
 *     policer               - (OUT) policer Id 
 *     skip_read             - (IN) Skip reading the policer entry 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_get_policer_from_table(int unit, soc_mem_t table, int index, 
                                    void *data, bcm_policer_t *policer, 
                                                         int skip_read) 
{
    int rv = BCM_E_NONE;
    int offset = 0;
    if (soc_feature(unit, soc_feature_global_meter)) {
        if (!skip_read) {
            rv = soc_mem_read(unit, table, MEM_BLOCK_ANY, index, data);
            BCM_IF_ERROR_RETURN(rv);
        }
        switch (table) {
            case VFIm:
            case SOURCE_VPm:
            case VLAN_XLATEm:
            case VLAN_TABm:
            case PORT_TABm:
            case VFP_POLICY_TABLEm:
#ifdef BCM_TRIUMPH3_SUPPORT
            case VLAN_XLATE_EXTDm:
#endif
                if (!skip_read) {
                    rv = soc_mem_read(unit, table, MEM_BLOCK_ANY, index, data);
                    BCM_IF_ERROR_RETURN(rv);
                }
                index = 0;
                if (SOC_IS_TRIUMPH3(unit) && (table == VLAN_XLATE_EXTDm)) {
                    if (SOC_MEM_FIELD_VALID(unit, table, 
                                              XLATE__SVC_METER_OFFSET_MODEf)) {
                        offset = soc_mem_field32_get(unit,table, data, 
                                                XLATE__SVC_METER_OFFSET_MODEf);
                    }
                    if (SOC_MEM_FIELD_VALID(unit, table, 
                                                     XLATE__SVC_METER_INDEXf)) {
                        index = soc_mem_field32_get(unit,table, data, 
                                                       XLATE__SVC_METER_INDEXf);
                    }

                } else {
                    if (SOC_MEM_FIELD_VALID(unit, table, 
                                                     SVC_METER_OFFSET_MODEf)) {
                        offset = soc_mem_field32_get(unit,table, data, 
                                                       SVC_METER_OFFSET_MODEf);
                    }
                    if (SOC_MEM_FIELD_VALID(unit, table, SVC_METER_INDEXf)) {
                        index = soc_mem_field32_get(unit,table, data, 
                                                             SVC_METER_INDEXf);
                    }
                }
                break; 
            case SVM_MACROFLOW_INDEX_TABLEm:
                if (SOC_MEM_FIELD_VALID(unit, table, MACROFLOW_INDEXf)) {
                    index = soc_mem_field32_get(unit, table, data, 
                                                             MACROFLOW_INDEXf);
                }
                break; 

            default:
               break; 
        }
        _bcm_esw_get_policer_id_from_index_offset(unit, index, 
                                                          offset, policer);
    } else {
        return BCM_E_UNAVAIL;
    }
    return rv;
}

/*
 * Function:
 *      _bcm_esw_reset_policer_from_table
 * Purpose:
 *      Remove the policer id configured from a table and re-adjust
 *      policer usage reference count. 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer               - (IN) policer Id 
 *     table                 - (IN) Table in which an entry needs to be reset 
 *     index                 - (IN) index in the table
 *     data                  - (IN) Data to be written
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_reset_policer_from_table(int unit, bcm_policer_t policer, 
                                      int table, int index, void *data) 
{

    int rv = BCM_E_NONE;
    rv = _bcm_esw_delete_policer_from_table(unit, policer, table, index, data);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to delete policer from table %d at \
                           index %d \n",FUNCTION_NAME(), table, index));
        return (rv);
    }
    soc_mem_write(unit, table, MEM_BLOCK_ANY, index, data);
    return rv;
}


/*
 * Function:
 *      _bcm_esw_delete_policer_from_table
 * Purpose:
 *      Remove the policer id configured from a table and re-adjust
 *      policer usage reference count
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer               - (IN) policer Id 
 *     table                 - (IN) Table in which an entry needs to be deleted 
 *     index                 - (IN) index in the table
 *     data                  - (IN) Data to be written
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_delete_policer_from_table(int unit, bcm_policer_t policer, 
                                      int table, int index, void *data) 
{
    int rv = BCM_E_NONE;
    bcm_policer_t current_policer = 0;
    int offset_mode = 0;
    if (soc_feature(unit, soc_feature_global_meter)) {
         /* validate policer id */
        rv = _bcm_esw_policer_validate(unit, &policer);
        if (BCM_FAILURE(rv)) {
            POLICER_VVERB(("%s : Invalid policer id passed: %x \n", \
                                      FUNCTION_NAME(), policer));
            return (rv);
        }
        switch (table) {
            case VFIm:
            case SOURCE_VPm:
            case VLAN_XLATEm:
            case VLAN_TABm:
            case PORT_TABm:
#ifdef BCM_TRIUMPH3_SUPPORT
            case VLAN_XLATE_EXTDm:
#endif
                rv =  _bcm_esw_get_policer_from_table(unit, table, index, data,
                                                          &current_policer, 0); 
                if (BCM_FAILURE(rv)) {
                    POLICER_VVERB(("%s : Unable to read the policer from table \
                             %d at index %d\n", FUNCTION_NAME(), table, index));
                    return (rv);
                }
                if (current_policer != policer) {
                    POLICER_VVERB(("%s : Policer Id passed is different from the \
                       one that is configured in the table. configured value is \
                       %d \n", FUNCTION_NAME(), current_policer));
                    return BCM_E_INTERNAL;
                } 
                if (SOC_MEM_FIELD_VALID(unit, table, SVC_METER_OFFSET_MODEf)) {
                    soc_mem_field32_set(unit, table, data, 
                                         SVC_METER_OFFSET_MODEf, offset_mode);
                }
                if (SOC_MEM_FIELD_VALID(unit, table, SVC_METER_INDEXf)) {
                    index = 0;
                    soc_mem_field32_set(unit,table, data,
                                                      SVC_METER_INDEXf, index);
                }
                break; 
            case VFP_POLICY_TABLEm:
                break; 
            default:
                  break; 
        }
        /* decrement current policer if any */
        if ((policer & BCM_POLICER_GLOBAL_METER_INDEX_MASK) > 0) {
            rv = _bcm_esw_policer_decrement_ref_count(unit, policer);
            BCM_IF_ERROR_RETURN(rv);
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_policer_svc_meter_get_mode_info
 * Purpose:
 *      Get the info related to a configured offset mode
 * Parameters:
 *     unit                  - (IN) unit number 
 *     meter_mode_v          - (IN) Meter offset mode 
 *     mode_info             - (OUT) Info of the configured mode 
 * Returns:
 *     BCM_E_XXX 
 */

bcm_error_t _bcm_policer_svc_meter_get_mode_info(
                int                                     unit,
                bcm_policer_svc_meter_mode_t            meter_mode_v,
                bcm_policer_svc_meter_bookkeep_mode_t   *mode_info
                )
{
    if (!((meter_mode_v >= 1) && 
                    (meter_mode_v < BCM_POLICER_SVC_METER_MAX_MODE))) {
        POLICER_VVERB(("%s : Invalid offset mode %d  \n", \
                                        FUNCTION_NAME(), meter_mode_v));
        return BCM_E_PARAM;
    }
    if (global_meter_offset_mode[unit][meter_mode_v].used == 0) {
        POLICER_VVERB(("%s : Passed mode is not used \n", FUNCTION_NAME()));
        return BCM_E_NOT_FOUND;
    }         
    *mode_info = global_meter_offset_mode[unit][meter_mode_v];
    return BCM_E_NONE; 
}


/* ACTION API's */

/*
 * Function:
 *      bcm_esw_policer_action_create
 * Purpose:
 *       Create a new action entry in meter action table 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     action_id             - (OUT) Action Id created 
 * Returns:
 *     BCM_E_XXX 
 */
int bcm_esw_policer_action_create(
    int unit,
    uint32 *action_id)
{
    int rv = BCM_E_NONE;
    if (!soc_feature(unit, soc_feature_global_meter)) {
        return BCM_E_UNAVAIL;
    }
    CHECK_GLOBAL_METER_INIT(unit);
    rv = shr_aidxres_list_alloc_block(
                 meter_action_list_handle[unit], 1, (uint32 *)action_id);
    if (BCM_FAILURE(rv)) {
        POLICER_ERR(("%s : Unable to allocate a handle for new \
                action\n", FUNCTION_NAME()));
        return (rv);
    }
    global_meter_action_bookkeep[unit][*action_id].used = 1;
    return rv; 
}

/*
 * Function:
 *     bcm_esw_policer_action_add
 * Purpose:
 *     Add an entry to meter action table 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     action_id             - (IN) Action Id created 
 *     action                - (IN) Action to be added 
 *     action_id             - (IN) Parameter associated with action 
 * Returns:
 *     BCM_E_XXX 
 */
int bcm_esw_policer_action_add(
    int unit, 
    uint32 action_id, 
    bcm_policer_action_t action, 
    uint32 param0)
{
    int rv = BCM_E_NONE;
    svm_policy_table_entry_t meter_action;
    uint32 green_action[1] = {0}, yellow_action[1] = {0}, red_action[1] = {0};

    if (global_meter_action_bookkeep[unit][action_id].used != 1)
    {
        POLICER_ERR(("%s : Action id is not created \n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    }

    GLOBAL_METER_LOCK(unit);
    /* read action table entry */
    rv = READ_SVM_POLICY_TABLEm(unit, MEM_BLOCK_ANY, action_id, &meter_action);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to read SVM_POLICY_TABLE for given \
                                            action id \n", FUNCTION_NAME()));
        return (rv);
    }
    soc_SVM_POLICY_TABLEm_field_get(unit, &meter_action, G_ACTIONSf,
                                                      &green_action[0]);
    soc_SVM_POLICY_TABLEm_field_get(unit, &meter_action, Y_ACTIONSf,
                                                     &yellow_action[0]);
    soc_SVM_POLICY_TABLEm_field_get(unit, &meter_action, R_ACTIONSf,
                                                        &red_action[0]);


    switch (action) { 
       case bcmPolicerActionGpDrop :
           SHR_BITSET(green_action, 
                              BCM_POLICER_GLOBAL_METER_ACTION_DROP_BIT_POS);
           break;       

       case bcmPolicerActionYpDrop :
           SHR_BITSET(yellow_action, 
                             BCM_POLICER_GLOBAL_METER_ACTION_DROP_BIT_POS);
           break;       

       case bcmPolicerActionRpDrop :
           SHR_BITSET(red_action, 
                            BCM_POLICER_GLOBAL_METER_ACTION_DROP_BIT_POS);
           break;       

       case bcmPolicerActionGpEcnNew :
           if (param0 > 0x3) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for ECN  \n", FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(green_action, 
                            BCM_POLICER_GLOBAL_METER_ACTION_ECN_BIT_POS, 2);
           green_action[0] = green_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_ECN_BIT_POS);
           SHR_BITSET(green_action, 
                          BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_ECN_BIT_POS);
           break;       

       case bcmPolicerActionYpEcnNew :
           if (param0 > 0x3) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for ECN  \n", FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(yellow_action, 
                            BCM_POLICER_GLOBAL_METER_ACTION_ECN_BIT_POS, 2);
           yellow_action[0] = yellow_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_ECN_BIT_POS);
           SHR_BITSET(yellow_action, 
                          BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_ECN_BIT_POS);
           break;       

       case bcmPolicerActionRpEcnNew :
           if (param0 > 0x3) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for ECN  \n", FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(red_action, 
                            BCM_POLICER_GLOBAL_METER_ACTION_ECN_BIT_POS, 2);
           red_action[0] = red_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_ECN_BIT_POS);
           SHR_BITSET(red_action, 
                         BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_ECN_BIT_POS);
           break;       

       case bcmPolicerActionGpCngNew :
           if (param0 > 0x3) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for CNG  \n", FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(green_action, 
                        BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_CNG_BIT_POS, 2);
           green_action[0] = green_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_CNG_BIT_POS);
           break;       

       case bcmPolicerActionYpCngNew :
           if (param0 > 0x3) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for CNG  \n", FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(yellow_action, 
                        BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_CNG_BIT_POS, 2);
           yellow_action[0] = yellow_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_CNG_BIT_POS);
           break;       

       case bcmPolicerActionRpCngNew :
           if (param0 > 0x3) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for CNG  \n", FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(red_action, 
                        BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_CNG_BIT_POS, 2);
           red_action[0] = red_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_CNG_BIT_POS);
           break;       

       case bcmPolicerActionGpDscpNew:
           if (param0 > 0x3f) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for DSCP  \n", FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(green_action,
                      BCM_POLICER_GLOBAL_METER_ACTION_DSCP_BIT_POS, 6);
           green_action[0] = green_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_DSCP_BIT_POS);
           SHR_BITSET(green_action, 
                         BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_DSCP_BIT_POS);
           break;       

       case bcmPolicerActionYpDscpNew:
           if (param0 > 0x3f) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for DSCP  \n", FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(yellow_action,
                      BCM_POLICER_GLOBAL_METER_ACTION_DSCP_BIT_POS, 6);
           yellow_action[0] = yellow_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_DSCP_BIT_POS);
           SHR_BITSET(yellow_action, 
                          BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_DSCP_BIT_POS);
           break;

       case bcmPolicerActionRpDscpNew:
           if (param0 > 0x3f) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for DSCP  \n", FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(red_action,
                      BCM_POLICER_GLOBAL_METER_ACTION_DSCP_BIT_POS, 6);
           red_action[0] = red_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_DSCP_BIT_POS);
           SHR_BITSET(red_action, 
                        BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_DSCP_BIT_POS);
           break;

       case bcmPolicerActionGpPrioIntNew :
           if (param0 > 0xf) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for int pri\n", \
                                                        FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(green_action,
                      BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_BIT_POS, 4);
           green_action[0] = green_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_BIT_POS);
           SHR_BITSET(green_action, 
                       BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_INT_PRI_BIT_POS);
           break;

       case bcmPolicerActionYpPrioIntNew :
           if (param0 > 0xf) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for int pri\n", \
                                                        FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(yellow_action,
                      BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_BIT_POS, 4);
           yellow_action[0] = yellow_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_BIT_POS);
           SHR_BITSET(yellow_action, 
                      BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_INT_PRI_BIT_POS);
           break;

       case bcmPolicerActionRpPrioIntNew :
           if (param0 > 0xf) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for int pri\n", \
                                                        FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(red_action,
                      BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_BIT_POS, 4);
           red_action[0] = red_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_BIT_POS);
           SHR_BITSET(red_action, 
                      BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_INT_PRI_BIT_POS);
           break;


       case bcmPolicerActionGpVlanPrioNew :
           if (param0 > 0x7) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for vlan pri\n", \
                                                        FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(green_action,
                      BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_BIT_POS, 3);
           green_action[0] = green_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_BIT_POS);
           SHR_BITSET(green_action, 
                        BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_DOT1P_BIT_POS);
           break;

       case bcmPolicerActionYpVlanPrioNew :
           if (param0 > 0x7) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for vlan pri\n", \
                                                        FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(yellow_action,
                      BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_BIT_POS, 3);
           yellow_action[0] = yellow_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_BIT_POS);
           SHR_BITSET(yellow_action, 
                        BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_DOT1P_BIT_POS);
           break;

       case bcmPolicerActionRpVlanPrioNew :
           if (param0 > 0x7) {
               GLOBAL_METER_UNLOCK(unit);
               POLICER_ERR(("%s : Invalid value for vlan pri\n", \
                                                        FUNCTION_NAME()));
               return BCM_E_PARAM;
           }
           SHR_BITCLR_RANGE(red_action,
                      BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_BIT_POS, 3);
           red_action[0] = red_action[0] | 
                   (param0 << BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_BIT_POS);
           SHR_BITSET(red_action, 
                        BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_DOT1P_BIT_POS);
           break;

       default:
          GLOBAL_METER_UNLOCK(unit);
          POLICER_ERR(("%s : Unsupported Action specified\n", FUNCTION_NAME()));
          return BCM_E_PARAM;
    }

    soc_SVM_POLICY_TABLEm_field_set(unit, &meter_action, G_ACTIONSf,
                                                    &green_action[0]);
    soc_SVM_POLICY_TABLEm_field_set(unit, &meter_action, Y_ACTIONSf,
                                                   &yellow_action[0]);
    soc_SVM_POLICY_TABLEm_field_set(unit, &meter_action, R_ACTIONSf,
                                                      &red_action[0]);

    rv = WRITE_SVM_POLICY_TABLEm(unit,MEM_BLOCK_ANY,action_id, &meter_action);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to write to SVM_POLICY_TABLE at location \
                        specified by action_id \n", FUNCTION_NAME()));
        return (rv);
    }
    GLOBAL_METER_UNLOCK(unit);
    return rv; 
}

/*
 * Function:
 *     bcm_esw_policer_action_destroy
 * Purpose:
 *     Delete an entry from meter action table 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     action_id             - (IN) Action Id to be destroyed 
 * Returns:
 *     BCM_E_XXX 
 */
int bcm_esw_policer_action_destroy(
    int unit, 
    uint32 action_id)
{
    int rv = BCM_E_NONE;
    uint32 reset=0;
    svm_policy_table_entry_t meter_action;
    if (global_meter_action_bookkeep[unit][action_id].used != 1)
    {
        POLICER_ERR(("%s : Action Id specified doesn't exist\n", \
                                                  FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    if (global_meter_action_bookkeep[unit]\
                                  [action_id].reference_count != 0)
    {
        POLICER_ERR(("%s : Action Id specified still being used \n", \
                                                  FUNCTION_NAME()));
        return BCM_E_BUSY;
    }
    GLOBAL_METER_LOCK(unit);
    /* read action table entry */
    rv = READ_SVM_POLICY_TABLEm(unit, MEM_BLOCK_ANY, action_id, &meter_action);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to read SVM_POLICY_TABLE at location \
                        specified by action_id \n", FUNCTION_NAME()));
        return (rv);
    }

    soc_SVM_POLICY_TABLEm_field_set(unit, &meter_action, G_ACTIONSf, &reset);
    soc_SVM_POLICY_TABLEm_field_set(unit, &meter_action, Y_ACTIONSf, &reset);
    soc_SVM_POLICY_TABLEm_field_set(unit, &meter_action, R_ACTIONSf, &reset);

    rv = WRITE_SVM_POLICY_TABLEm(unit, MEM_BLOCK_ANY, action_id, &meter_action);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to write to SVM_POLICY_TABLE at location \
                        specified by action_id \n", FUNCTION_NAME()));
        return (rv);
    }

    rv = shr_aidxres_list_free(meter_action_list_handle[unit], action_id); 
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_VERB(("%s : Unable to free action handle \n", FUNCTION_NAME()));
        return (rv);
    }
    GLOBAL_METER_UNLOCK(unit);
    global_meter_action_bookkeep[unit][action_id].used = 0;
    return rv; 

}

/*
 * Function:
 *      bcm_esw_policer_action_get
 * Purpose:
 *     get action parameter for a given action 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     action_id             - (IN) Action Id
 *     action                - (IN) Action for which param is needed 
 *     param0                - (OUT) Parameter associated with action id
 * Returns:
 *     BCM_E_XXX 
 */
int bcm_esw_policer_action_get(
    int unit, 
    uint32 action_id, 
    bcm_policer_action_t action, 
    uint32   *param0) 
{
    int rv = BCM_E_NONE;
    svm_policy_table_entry_t meter_action;
    uint32 green_action = 0, yellow_action = 0, red_action = 0;
    if (global_meter_action_bookkeep[unit][action_id].used != 1)
    {
        POLICER_ERR(("%s : Action Id specified doesn't exist\n", \
                                                  FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    GLOBAL_METER_LOCK(unit);
    /* read action table entry */
    rv = READ_SVM_POLICY_TABLEm(unit, MEM_BLOCK_ANY, action_id, &meter_action);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to read SVM_POLICY_TABLE at location \
                        specified by action_id \n", FUNCTION_NAME()));
        return (rv);
    }
    soc_SVM_POLICY_TABLEm_field_get(unit, &meter_action, G_ACTIONSf, 
                                                         &green_action);
    soc_SVM_POLICY_TABLEm_field_get(unit, &meter_action, Y_ACTIONSf,
                                                         &yellow_action);
    soc_SVM_POLICY_TABLEm_field_get(unit, &meter_action, R_ACTIONSf,
                                                           &red_action);
    
    switch (action) { 
        case bcmPolicerActionGpEcnNew :
            *param0 = (green_action & BCM_POLICER_GLOBAL_METER_ACTION_ECN_MASK);
            break;

        case bcmPolicerActionGpDscpNew:
            *param0 = (green_action & 
                            BCM_POLICER_GLOBAL_METER_ACTION_DSCP_MASK) >> 2;
            break;

        case bcmPolicerActionGpVlanPrioNew:
            *param0 = (green_action & 
                              BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_MASK) >> 
                              BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_BIT_POS;
            break;

        case bcmPolicerActionGpPrioIntNew:
            *param0 = (green_action & 
                             BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_MASK) >>
                             BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_BIT_POS ;
            break;

       case bcmPolicerActionGpCngNew :
            *param0 = (green_action & 
                             BCM_POLICER_GLOBAL_METER_ACTION_CNG_MASK) >>
                             BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_CNG_BIT_POS ;
            break;

        case bcmPolicerActionYpEcnNew :
            *param0 = (yellow_action & 
                                  BCM_POLICER_GLOBAL_METER_ACTION_ECN_MASK);
            break;

        case bcmPolicerActionYpDscpNew:
            *param0 = (yellow_action & 
                            BCM_POLICER_GLOBAL_METER_ACTION_DSCP_MASK) >> 2;
            break;

        case bcmPolicerActionYpVlanPrioNew:
            *param0 = (yellow_action & 
                               BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_MASK) >> 
                                 BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_BIT_POS;
            break;

        case bcmPolicerActionYpPrioIntNew:
            *param0 = (yellow_action & 
                              BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_MASK) >>
                              BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_BIT_POS ;
            break;

       case bcmPolicerActionYpCngNew :
            *param0 = (yellow_action & 
                             BCM_POLICER_GLOBAL_METER_ACTION_CNG_MASK) >>
                             BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_CNG_BIT_POS ;
            break;

        case bcmPolicerActionRpEcnNew :
            *param0 = (red_action & BCM_POLICER_GLOBAL_METER_ACTION_ECN_MASK);
            break;

        case bcmPolicerActionRpDscpNew:
            *param0 = (red_action & 
                             BCM_POLICER_GLOBAL_METER_ACTION_DSCP_MASK) >> 2;
            break;

        case bcmPolicerActionRpVlanPrioNew:
            *param0 = (red_action & 
                               BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_MASK) >> 
                               BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_BIT_POS;
            break;

        case bcmPolicerActionRpPrioIntNew:
            *param0 = (red_action & 
                             BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_MASK) >>
                             BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_BIT_POS ;
            break;

       case bcmPolicerActionRpCngNew :
            *param0 = (red_action & 
                             BCM_POLICER_GLOBAL_METER_ACTION_CNG_MASK) >>
                             BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_CNG_BIT_POS ;
            break;

        default:
            POLICER_ERR(("%s : Unsupported Action specified\n", \
                                                 FUNCTION_NAME()));
            rv = BCM_E_PARAM;
            break;
    }
    GLOBAL_METER_UNLOCK(unit);
    return rv; 
}

/* end Action API's */


/* POLICER API's */
/*
 * Function:
 *      _bcm_global_meter_free_allocated_policer_on_error
 * Purpose:
 *      Clean up the internal data structure in case of error.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     numbers               - (IN) Number of policer to be freed 
 *     offset                - (IN) Offset w.r.t base policer 
 *     policer_index         - (IN) Base policer id
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_global_meter_free_allocated_policer_on_error(int unit,  
                                           uint32 numbers, uint8 *offset, 
                                                  uint32 policer_index) 
{

    int rv = BCM_E_NONE;
    int index    =0;
    int pool  =  0;

    for (index = 0;index < numbers; index++) {
        if (index) {
            pool = offset[0] + offset[index];
        } else {
            pool = offset[0];
        }
        rv = shr_aidxres_list_free(
                         meter_alloc_list_handle[unit][pool], policer_index);
        if (!BCM_SUCCESS(rv)) {
            POLICER_VVERB(("%s : Unable to free policer handle \n", \
                                                       FUNCTION_NAME()));
            return BCM_E_INTERNAL;
        }

        rv = _bcm_gloabl_meter_unreserve_bloc_horizontally(unit, pool, policer_index); 
        if (!BCM_SUCCESS(rv)) {
            POLICER_VVERB(("%s : Unable to free policer handle \n", \
                                                       FUNCTION_NAME()));
            return BCM_E_INTERNAL;
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_gloabl_meter_alloc_horizontally
 * Purpose:
 *      Allocate a set of policers such that each of the policers
 *      created have the same index but differnt pool number.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     numbers               - (IN) Number of policer to be allocated 
 *     pid                   - (OUT) Base policer id
 *     offset                - (OUT) Offset w.r.t base policer 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_gloabl_meter_alloc_horizontally(int unit, int *numbers, 
                                         bcm_policer_t *pid,
                                         uint8 *offset)
{
    int rv = BCM_E_NONE;
     _global_meter_horizontal_alloc_t *alloc_control; 
    uint32 allocated_policer = 0;
    int max_index = 0, max_pool = 0, index = 0, map = 0, pool = 0;
    int policers_available = 0;
    int skip_index = 0;
    max_index =  SOC_INFO(unit).global_meter_size_of_pool;
    max_pool = SOC_INFO(unit).global_meter_pools;
    alloc_control = global_meter_hz_alloc_bookkeep[unit];
    for (index = 1; index < max_index; index++) {
        skip_index = 0;
        allocated_policer = 0;
        if (alloc_control[index].no_of_groups_allocated == 2) {
            continue;            
        } else if (alloc_control[index].no_of_groups_allocated == 1) {
        /* check if we have enough free policers to allocate */
            map = alloc_control[index].alloc_bit_map;
            if (alloc_control[index].first_bit_to_use > 0) {
                map = map & 
                (convert_to_bitmask(alloc_control[index].first_bit_to_use - 1));
                policers_available = _shr_popcount(map);
            } else {
                policers_available = 0;
            }
            if (policers_available >= *numbers) {
                offset[0] = 0;
            /* Use shared aligned index management to allocate policers */
                for (pool = 0; pool <= alloc_control[index].first_bit_to_use; 
                                                                    pool++) {
                    rv = shr_aidxres_list_reserve_block(
                      meter_alloc_list_handle[unit][pool], index, 1);
                    if (!BCM_SUCCESS(rv)) {
                        if (allocated_policer) {
                       /* free all the allocated policers */
                           rv = _bcm_global_meter_free_allocated_policer_on_error(unit,
                                              allocated_policer, offset, index);

                           skip_index = 1;
                           allocated_policer = 0;
                        }
                        continue;
                    }
                    rv = _bcm_global_meter_reserve_bloc_horizontally(unit, 
                                                              pool, index);
                    if (!BCM_SUCCESS(rv)) {
                        rv = shr_aidxres_list_free(
                            meter_alloc_list_handle[unit][pool], index);
                        if (!BCM_SUCCESS(rv)) {
                            POLICER_VVERB(("%s : Unable to free policer \
                                            handle \n", FUNCTION_NAME()));
                            return BCM_E_INTERNAL;
                        }
                        continue;
                    }
                    if (allocated_policer) {
                        offset[allocated_policer] = pool - offset[0];
                    } else {
                        offset[allocated_policer] = pool;
                    }
                    allocated_policer++;
                    if (allocated_policer == *numbers) {
                       alloc_control[index].last_bit_to_use =  
                                         alloc_control[index].first_bit_to_use;
                       alloc_control[index].first_bit_to_use = pool;
                       break;
                    }
        
                }
                if (allocated_policer != *numbers) {
                     /* free all the allocated policers */
                    rv = _bcm_global_meter_free_allocated_policer_on_error(
                                     unit, allocated_policer, offset, index);
                    if (skip_index == 0 ) {
                        POLICER_VVERB(("%s : Unable to free policer \
                                            handle \n", FUNCTION_NAME()));
                        return BCM_E_INTERNAL;
                    }
                } else if( allocated_policer == *numbers) {
                    *pid = index; 
                    alloc_control[index].no_of_groups_allocated++;
                    POLICER_VVERB(("%s : Allocated base policer with \
                                      index %x \n", FUNCTION_NAME(), index));
                    return BCM_E_NONE;
                }
            } 
            map = alloc_control[index].alloc_bit_map;
            map = map & 
                   ~(convert_to_bitmask(alloc_control[index].last_bit_to_use));
            policers_available = _shr_popcount(map);
            if (policers_available >= *numbers) {
            /* Use shared aligned index management to allocate policers */
                for (pool = alloc_control[index].last_bit_to_use + 1; 
                                              pool < max_pool; pool++) {
                    rv = shr_aidxres_list_reserve_block(
                     meter_alloc_list_handle[unit][pool], index, 1);
                    if (!BCM_SUCCESS(rv)) {
                        if (allocated_policer) {
                       /* free all the allocated policers */
                           rv = _bcm_global_meter_free_allocated_policer_on_error(unit,
                                              allocated_policer, offset, index);

                           skip_index = 1;
                           allocated_policer = 0;
                        }
                        continue;
                    }
                    rv = _bcm_global_meter_reserve_bloc_horizontally(unit, 
                                                               pool, index);
                    if (!BCM_SUCCESS(rv)) {
                        rv = shr_aidxres_list_free(
                            meter_alloc_list_handle[unit][pool], index);
                        if (!BCM_SUCCESS(rv)) {
                            POLICER_VVERB(("%s : Unable to free policer \
                                            handle \n", FUNCTION_NAME()));
                            return BCM_E_INTERNAL;
                        }
                        continue;
                    }
                    if (allocated_policer) {
                        offset[allocated_policer] = pool - offset[0];
                    } else {
                        offset[allocated_policer] = pool;
                    }
                    allocated_policer++;
                    if (allocated_policer == *numbers) {
                       alloc_control[index].first_bit_to_use =  
                                         alloc_control[index].last_bit_to_use;
                       alloc_control[index].last_bit_to_use = offset[0];
                       break;
                    }
                }
                if (allocated_policer != *numbers) {
                     /* free all the allocated policers */
                    rv = _bcm_global_meter_free_allocated_policer_on_error(
                                     unit, allocated_policer, offset, index);
                    if (skip_index == 0 ) {
                        POLICER_VVERB(("%s : Unable to free policer \
                                            handle \n", FUNCTION_NAME()));
                        return BCM_E_INTERNAL;
                    }
                } else if( allocated_policer == *numbers) {
                    *pid = index; 
                    alloc_control[index].no_of_groups_allocated++;
                    POLICER_VVERB(("%s : Allocated base policer with \
                                      index %x \n", FUNCTION_NAME(), index));
                    return BCM_E_NONE;
                }
            }
        } else {   /* No Cascade group assigned */
            map = alloc_control[index].alloc_bit_map;
            policers_available = _shr_popcount(map);
            if (policers_available < *numbers) {
                continue;
            }
            offset[0] = 0;
            /* Use shared aligned index management to allocate policers */
            for (pool = 0; pool < max_pool; pool++) {
                rv = shr_aidxres_list_reserve_block(
                     meter_alloc_list_handle[unit][pool], index, 1);
                if (!BCM_SUCCESS(rv)) {
                    if (allocated_policer) {
                       /* free all the allocated policers */
                       rv = _bcm_global_meter_free_allocated_policer_on_error(unit,
                                              allocated_policer, offset, index);

                       skip_index = 1;
                       allocated_policer = 0;
                    }
                    continue;
                }
                rv = _bcm_global_meter_reserve_bloc_horizontally(unit, pool,
                                                                      index);
                if (!BCM_SUCCESS(rv)) {
                    rv = shr_aidxres_list_free(
                            meter_alloc_list_handle[unit][pool], *pid);
                    if (!BCM_SUCCESS(rv)) {
                        POLICER_VVERB(("%s : Unable to free policer \
                                            handle \n", FUNCTION_NAME()));
                        return BCM_E_INTERNAL;
                    }
                    continue;
                }
                if (allocated_policer == 0) {
                    alloc_control[index].first_bit_to_use = pool;
                    offset[allocated_policer]=pool;
                } else {
                    offset[allocated_policer] = pool - offset[0];
                }
                allocated_policer++;
                if (allocated_policer == *numbers) {
                   alloc_control[index].last_bit_to_use = pool;
                   break;
                }
            }
            if (allocated_policer != *numbers) {
                 /* free all the allocated policers */
                rv = _bcm_global_meter_free_allocated_policer_on_error(unit,
                                              allocated_policer, offset, index);
                if (skip_index == 0) {
                    POLICER_VVERB(("%s : Unable to free policer \
                                            handle \n", FUNCTION_NAME()));
                    return BCM_E_INTERNAL;
                }
            } else if( allocated_policer == *numbers) {
                *pid = index; 
                alloc_control[index].no_of_groups_allocated++;
                POLICER_VVERB(("%s : Allocated base policer with index %x \n", \
                                                     FUNCTION_NAME(), index));
                return BCM_E_NONE;
            }
        }
    } 
    POLICER_VVERB(("%s : Unable to allocate policer as table is full  \n", \
                                                          FUNCTION_NAME()));
    return BCM_E_FULL;
}
 
/*
 * Function:
 *      _bcm_global_meter_reserve_bloc_horizontally
 * Purpose:
 *      reset the bit in the inernal data structure corresponding
 *      to the policer that is being reserved.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     pool_id               - (IN) polier pool number 
 *     pid                   - (IN) policer id
 * Returns:
 *     BCM_E_XXX 
 */

int _bcm_global_meter_reserve_bloc_horizontally(int unit, int pool_id,  
                                           bcm_policer_t pid) 
{
    int rv = BCM_E_NONE;
    global_meter_hz_alloc_bookkeep[unit][pid].alloc_bit_map \
                       &= convert_to_mask(pool_id);
    return rv;
}

/*
 * Function:
 *      _bcm_esw_get_policer_control
 * Purpose:
 *     Get the policer control data sructure for a given pool, offset mode
 *     and index in the pool.   
 * Parameters:
 *     unit                  - (IN) unit number 
 *     pid                   - (IN) policer id
 *     pool                  - (IN) polier pool number 
 *     offset_mode           - (IN) meter offset mode 
 *     policer_control       - (OUT) polier control info 
 * Returns:
 *     BCM_E_XXX 
 */

int _bcm_esw_get_policer_control(int unit, bcm_policer_t pid, 
                                 int pool, int offset_mode,
              _global_meter_policer_control_t **policer_control) 
{
    int rv = BCM_E_NONE;
    bcm_policer_t policer_id = 0;
    int size_pool = 0;
    size_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
    policer_id = (((offset_mode + 1) << BCM_POLICER_GLOBAL_METER_MODE_SHIFT) +
                    (pool << _shr_popcount(size_pool - 1)) + pid);
    rv = _bcm_global_meter_policer_get(unit, policer_id, policer_control);
    return rv;
}

/*
 * Function:
 *      _bcm_gloabl_meter_free_horizontally
 * Purpose:
 *      Free the bits in the inernal data structure corresponding
 *      to set of policer that is being destroyed.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     pool_id               - (IN) polier pool number 
 *     pid                   - (IN) policer id
 *     numbers               - (IN) Number of policers to be freed 
 *     offset                - (IN) offset w.r.t pid 
 * Returns:
 *     BCM_E_XXX 
 */

int _bcm_gloabl_meter_free_horizontally(int unit, int pool_id,  
                                         bcm_policer_t pid, 
                                         int numbers, uint8 offset[8]) 
{
    int rv = BCM_E_NONE;
    int index = 0;
     _global_meter_horizontal_alloc_t *alloc_control; 
    int num_pol = 0;
    int actual_pool = 0;
    _global_meter_policer_control_t *policer_control = NULL;
    alloc_control = global_meter_hz_alloc_bookkeep[unit];
    actual_pool = pool_id;
    for (index = 0; index < numbers; index++)
    {
        if (index > 0) {
           actual_pool = pool_id + offset[index];
        }
        alloc_control[pid].alloc_bit_map |= 
                             ((~convert_to_mask(actual_pool)) & 0xFF);
    } 
    if ((numbers > 1) && (alloc_control[pid].no_of_groups_allocated)) {
        alloc_control[pid].no_of_groups_allocated--;
        if (alloc_control[pid].no_of_groups_allocated == 1) {
            if (pool_id + offset[index - 1] == 
                                 alloc_control[pid].first_bit_to_use) {
                rv = _bcm_esw_get_policer_control(unit, pid, 
                                     alloc_control[pid].last_bit_to_use,
                                         0, &policer_control);
                if (!BCM_SUCCESS(rv)) {
                    POLICER_VVERB(("%s : Unable to get policer control for pid \
                                             %x\n", FUNCTION_NAME(), pid));
                    return rv;
                }
                alloc_control[pid].first_bit_to_use = 
                                        alloc_control[pid].last_bit_to_use;
                num_pol = policer_control->no_of_policers;
                if (num_pol > 0 ) {
                    pool_id = policer_control->offset[0] + 
                          policer_control->offset[num_pol-1];
                    alloc_control[pid].last_bit_to_use = pool_id;
                } else {
                    POLICER_VVERB(("%s : Number of policers in policer control \
                          structure is 0\n", FUNCTION_NAME()));
                    return BCM_E_INTERNAL;
                } 
            } else if (pool_id == alloc_control[pid].last_bit_to_use) { 
                alloc_control[pid].last_bit_to_use = 
                                       alloc_control[pid].first_bit_to_use;
                rv = _bcm_esw_get_policer_control(unit, pid, 
                                     alloc_control[pid].first_bit_to_use,
                                         0, &policer_control);
                if (!BCM_SUCCESS(rv)) {
                    POLICER_VVERB(("%s : Unable to get policer control for pid \
                                             %x\n", FUNCTION_NAME(), pid));
                    return rv;
                }
                num_pol = policer_control->no_of_policers;
                if (num_pol > 0 ) {
                    pool_id = policer_control->offset[0];  
                    alloc_control[pid].first_bit_to_use = pool_id;
                } else {
                    POLICER_VVERB(("%s : Number of policers in policer control \
                          structure is 0\n", FUNCTION_NAME()));
                    return BCM_E_INTERNAL;
                } 
            }
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_gloabl_meter_unreserve_bloc_horizontally
 * Purpose:
 *      Free the bit in the inernal data structure corresponding
 *      to the policer that is being destroyed.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     pool_id               - (IN) polier pool number 
 *     pid                   - (IN) policer id
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_gloabl_meter_unreserve_bloc_horizontally(int unit, int pool_id,  
                                           bcm_policer_t pid) 
{
    int rv = BCM_E_NONE;
   
    global_meter_hz_alloc_bookkeep[unit][pid].alloc_bit_map \
        |= (~convert_to_mask(pool_id) & 0xFF);
    return rv;
}

/*
 * Function:
 *      _global_meter_reserve_policer_id
 * Purpose:
 *      Reserve a given policer id. 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     direction             - (IN) Horizontal/Vertical 
 *     numbers               - (IN) Number of policers to be reserved
 *     pid                   - (IN) base policer id
 *     pid_offset            - (IN) Offset w.r.t base policer 
 * Returns:
 * Returns:
 * Returns:
 *     BCM_E_XXX 
 */
int _global_meter_reserve_policer_id(int unit, int direction, int numbers, 
                                             bcm_policer_t pid, 
                                             uint8 *pid_offset) 
{
    int rv = BCM_E_NONE;
    uint32 index = 0, free_index = 0;
    int pool = 0, pol_index = 0, pool_id = 0;
    int offset_mask = 0;
    int size_pool = 0, num_pools = 0;
    int pool_mask = 0, pool_offset = 0;
    size_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
    num_pools =  SOC_INFO(unit).global_meter_pools;
    offset_mask = size_pool - 1;
    pool_offset = _shr_popcount(offset_mask);
    pool_mask = (num_pools - 1) << pool_offset; 

    pool = (pid & pool_mask) >> pool_offset;
    pol_index = pid & offset_mask; 

    if (direction == GLOBAL_METER_ALLOC_VERTICAL) {
        rv = shr_aidxres_list_reserve_block(
                    meter_alloc_list_handle[unit][pool], pol_index, numbers); 
        if (!BCM_SUCCESS(rv)) {
            POLICER_VVERB(("%s : Unable to reserve policer in shared index \
                                             management\n", FUNCTION_NAME()));
            return BCM_E_INTERNAL;
        }
        for (index = 0; index < numbers; index++) {
            rv = _bcm_global_meter_reserve_bloc_horizontally(unit, pool,
                                                            pol_index + index);
            if (!BCM_SUCCESS(rv)) {
                rv = shr_aidxres_list_free(
                     meter_alloc_list_handle[unit][pool], pol_index);
                if (!BCM_SUCCESS(rv)) {
                    POLICER_VVERB(("%s : Unable to free policer in shared \
                               index management\n", FUNCTION_NAME()));
                    return BCM_E_INTERNAL;
                }
                for (free_index = 0; free_index < index; free_index++) {
                    rv = _bcm_gloabl_meter_unreserve_bloc_horizontally(unit,
                                                      pool, pol_index+index);
                    if (!BCM_SUCCESS(rv)) {
                        POLICER_VVERB(("%s : Unable to free policer in hz \
                               index management\n", FUNCTION_NAME()));
                        return BCM_E_INTERNAL;
                    }
                }
            }
        }

    } else if (direction == GLOBAL_METER_ALLOC_HORIZONTAL) {
        for (index = 0; index < numbers; index++) {
            if (index > 0) {
                pool_id = pool + pid_offset[index];
            }
            rv = shr_aidxres_list_reserve_block(
                   meter_alloc_list_handle[unit][pool_id], pol_index, 1);
            if (!BCM_SUCCESS(rv)) {
                POLICER_VVERB(("%s : Unable to reserve policer in shared \
                               index management\n", FUNCTION_NAME()));
                return BCM_E_INTERNAL;
            }
            rv = _bcm_global_meter_reserve_bloc_horizontally(unit,
                             pool_id, pol_index);
            if (!BCM_SUCCESS(rv)) {
                rv = shr_aidxres_list_free(
                        meter_alloc_list_handle[unit][pool], pol_index);
                if (!BCM_SUCCESS(rv)) {
                    POLICER_VVERB(("%s : Unable to free policer in \
                               index management\n", FUNCTION_NAME()));
                    return BCM_E_INTERNAL;
                }
                for (free_index=0; free_index < index; free_index++) {
                    rv = _bcm_gloabl_meter_unreserve_bloc_horizontally(unit,
                                                     pool_id, pol_index);
                    if (!BCM_SUCCESS(rv)) {
                        POLICER_VVERB(("%s : Unable to free policer in hz \
                               index management\n", FUNCTION_NAME()));
                        return BCM_E_INTERNAL;
                    }
                }
            }
        }
    } else {
        POLICER_VVERB(("%s : Invalid direction for policer allocation \n", \
                                                         FUNCTION_NAME()));
        return BCM_E_INTERNAL;
    }
    return rv;
}

/*
 * Function:
 *      _global_meter_policer_id_alloc
 * Purpose:
 *     Allocate a new policer.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     direction             - (IN) Horizontal/Vertical 
 *     numbers               - (IN/OUT) Number of policers
 *     pid                   - (OUT) base policer id
 *     pid_offset            - (OUT) Offset w.r.t base policer 
 *     skip_pool             - (IN) Skip a pool 
 * Returns:
 *     BCM_E_XXX 
 */
int _global_meter_policer_id_alloc(int unit, int direction,
                                   int *numbers, bcm_policer_t *pid, 
                                   int skip_pool, uint8 *pid_offset)

{
    int rv = BCM_E_NONE;
    uint32 index = 0, no_of_pools_checked = 0;
    static uint32 pool = 0;
    int size_pool = 0, num_pools = 0;
    size_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
    num_pools =  SOC_INFO(unit).global_meter_pools;

    if (pool >= (num_pools - 1)) {
        pool = 0;
    } else {
        pool++;
    }
    if (direction == GLOBAL_METER_ALLOC_VERTICAL) {
        for ( ;pool < num_pools; pool++) {
            /* in case of envelop meters, micro and macro flow meters can't be
               on same pool */
            if (pool == skip_pool) {
                no_of_pools_checked++;
                continue;
            }
            rv = shr_aidxres_list_alloc_block(
                  (meter_alloc_list_handle[unit][pool]), 
                                                   *numbers, (uint32 *)pid);
         
            if (BCM_SUCCESS(rv)) {
               /* add code to set bits in horizontal allocation datastruture */
                for (index = 0; index < *numbers; index++) {
                    rv = _bcm_global_meter_reserve_bloc_horizontally(unit, pool,
                                                          ((*pid)+index));
                    if (!BCM_SUCCESS(rv)) {
                        for (index = 0; index < *numbers; index++) {
                            rv = shr_aidxres_list_free(
                              meter_alloc_list_handle[unit][pool], 
                                                             *pid + index);
                        }
                        POLICER_VVERB(("%s : Unable to free policer in \
                               index management\n", FUNCTION_NAME()));
                        return BCM_E_INTERNAL;
                    }
                }
                /* Add pool id to make complete policer id */
                *pid = *pid | (pool << _shr_popcount(size_pool - 1));
                POLICER_VVERB(("%s : allocated policer with pid %x \n", \
                                                   FUNCTION_NAME(), *pid));
                return rv;
            }
            no_of_pools_checked++;
            if (no_of_pools_checked == num_pools) {
               return BCM_E_FULL;
            }
        } 
        return BCM_E_FULL;
    } else if (direction == GLOBAL_METER_ALLOC_HORIZONTAL) {
    
        /* alloc horizontally */
        rv = _bcm_gloabl_meter_alloc_horizontally(unit, numbers, pid, 
                                                             pid_offset);
        if (!BCM_SUCCESS(rv)) {
           return BCM_E_FULL;
        }
        *pid |= (*pid_offset << _shr_popcount(size_pool - 1));
        POLICER_VVERB(("%s : allocated policer with pid %x \n", \
                                                   FUNCTION_NAME(), *pid));
    }
    return rv;
}

/*
 * Function:
 *      _bcm_global_meter_read_config_from_hw
 * Purpose:
 *       Read policer configuration form the HW table entry.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer_id            - (IN) policer id
 *     pol_cfg               - (OUT) policer configuration 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_global_meter_read_config_from_hw(int unit, 
                                          bcm_policer_t policer_id,
                                          bcm_policer_config_t *pol_cfg) 
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    svm_meter_table_entry_t data;
    int rv = BCM_E_NONE;
    uint32    refresh_rate = 0;    /* Policer refresh rate.    */
    uint32    granularity = 0;     /* Policer granularity.     */
    uint32  mode = 0, mode_modifier = 0, coupling = 0;
    uint32 bucket_count = 0, bucket_size = 0;
    int index = 0;
    /* read HW register */
    _bcm_esw_get_policer_table_index(unit, policer_id, &index); 
    rv = soc_mem_read(unit, SVM_METER_TABLEm, MEM_BLOCK_ANY,index, &data);
    if (!BCM_SUCCESS(rv)) {
        POLICER_VVERB(("%s : Unable to read SVM METER TABLE at index %d \
                                           \n", FUNCTION_NAME(), index));
        return rv;
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COMMITTED_BUCKETCOUNTf)) {
        soc_SVM_METER_TABLEm_field_get(unit, &data, COMMITTED_BUCKETCOUNTf,
                                                           &bucket_count);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COMMITTED_REFRESHCOUNTf)) {
        soc_SVM_METER_TABLEm_field_get(unit, &data, COMMITTED_REFRESHCOUNTf,
                                                           &refresh_rate);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COMMITTED_BUCKETSIZEf)) {
        soc_SVM_METER_TABLEm_field_get(unit, &data, COMMITTED_BUCKETSIZEf,
                                                             &bucket_size);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_GRANf)) {
       soc_SVM_METER_TABLEm_field_get(unit, &data, METER_GRANf,&granularity);
    }
    rv = _bcm_xgs_bucket_encoding_to_kbits
            (refresh_rate, bucket_size, granularity,
             _BCM_XGS_METER_FLAG_GRANULARITY | 
                    _BCM_XGS_METER_FLAG_GLOBAL_METER_POLICER,
             &pol_cfg->ckbits_sec, &pol_cfg->ckbits_burst);
    if (!BCM_SUCCESS(rv)) {
        POLICER_VVERB(("%s : Unable to translate rate in kbps to bucket size \
                       and granularity \n", FUNCTION_NAME()));
        return rv;
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, EXCESS_BUCKETCOUNTf)) {
        soc_SVM_METER_TABLEm_field_get(unit, &data, EXCESS_BUCKETCOUNTf,
                                                          &bucket_count);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, EXCESS_REFRESHCOUNTf)) {
        soc_SVM_METER_TABLEm_field_get(unit, &data, EXCESS_REFRESHCOUNTf,
                                                           &refresh_rate);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, EXCESS_BUCKETSIZEf)) {
       soc_SVM_METER_TABLEm_field_get(unit, &data, EXCESS_BUCKETSIZEf,
                                                               &bucket_size);
    }
    rv = _bcm_xgs_bucket_encoding_to_kbits
            (refresh_rate, bucket_size, granularity,
             _BCM_XGS_METER_FLAG_GRANULARITY | 
                  _BCM_XGS_METER_FLAG_GLOBAL_METER_POLICER,
             &pol_cfg->pkbits_sec, &pol_cfg->pkbits_burst);
    if (!BCM_SUCCESS(rv)) {
        POLICER_VVERB(("%s : Unable to translate rate in kbps to bucket size \
                       and granularity \n", FUNCTION_NAME()));
        return rv;
    }

    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_SHARING_MODEf)) {
        soc_SVM_METER_TABLEm_field_get(unit, &data, 
                         METER_SHARING_MODEf, &pol_cfg->sharing_mode);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, POLICY_TABLE_INDEXf)) {
        soc_SVM_METER_TABLEm_field_get(unit,
              &data, POLICY_TABLE_INDEXf, &pol_cfg->action_id);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COUPLING_FLAGf)) {
        soc_SVM_METER_TABLEm_field_get(unit, &data, COUPLING_FLAGf, &coupling);
    }

    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_MODIFIERf)) {
        soc_SVM_METER_TABLEm_field_get(unit, &data, METER_MODIFIERf, 
                                                     &mode_modifier);
        if (mode_modifier == 0) {
            pol_cfg->flags = BCM_POLICER_COLOR_BLIND;
        } else { 
            pol_cfg->flags = 0; /* color aware */
        }
    }

    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_MODEf)) {
        soc_SVM_METER_TABLEm_field_get(unit, &data, METER_MODEf, &mode);
        switch (mode) { 
 
            case GLOBAL_METER_MODE_DEFAULT:
                if (mode_modifier) {
                    pol_cfg->mode = bcmPolicerModePassThrough;
                } else {
                    pol_cfg->mode = bcmPolicerModeGreen;
                }
                break;

            case GLOBAL_METER_MODE_SR_TCM:
                pol_cfg->mode = bcmPolicerModeSrTcm;
                break;
         
            case GLOBAL_METER_MODE_SR_TCM_MODIFIED:
                pol_cfg->mode = bcmPolicerModeSrTcmModified;
                break;

            case GLOBAL_METER_MODE_TR_TCM:
                pol_cfg->mode = bcmPolicerModeTrTcm;
                break;
         
            case GLOBAL_METER_MODE_TR_TCM_MODIFIED:
                pol_cfg->mode = bcmPolicerModeTrTcmDs;
                if (coupling) {
                   pol_cfg->mode = bcmPolicerModeCoupledTrTcmDs;
                }            
                break;

            case GLOBAL_METER_MODE_CASCADE:
                pol_cfg->mode = bcmPolicerModeCascade;
                if (coupling) {
                   pol_cfg->mode = bcmPolicerModeCoupledCascade;
                }            
                break;

            default:
                break;
        }
    }
    return rv;
#else
    return BCM_E_UNAVAIL;
#endif
}

/*
 * Function:
 *      _bcm_esw_policer_action_detach
 * Purpose:
 *     Internal function to Disassociate a policer action from a given 
 *     policer id and readjust reference count.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer_id            - (IN) policer id
 *     action_id             - (IN) Action ID 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_policer_action_detach(int unit, bcm_policer_t policer_id,
                                uint32 action_id)
{
    int rv = BCM_E_NONE;
    svm_meter_table_entry_t meter_entry;
    _global_meter_policer_control_t *policer_control = NULL;
    int policer_index = 0;
    uint32 mode = 0, coupling = 0, policy_index = 0;
    int index = 0;
    if (!soc_feature(unit, soc_feature_global_meter)) {
        return BCM_E_UNAVAIL;
    }
    CHECK_GLOBAL_METER_INIT(unit);
    /* validate policer id */
    rv = _bcm_esw_policer_validate(unit, &policer_id);
    if (BCM_FAILURE(rv)) {
        POLICER_ERR(("%s : Invalid Policer Id \n", FUNCTION_NAME()));
        return (rv);
    }
    /* validate action id */
    if (action_id > soc_mem_index_max(unit, SVM_POLICY_TABLEm)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Invalid Action Id \n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    } 
    GLOBAL_METER_LOCK(unit);

    rv = _bcm_global_meter_policer_get(unit, policer_id, &policer_control);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to get policer control for the policer Id \
                                              passed  \n", FUNCTION_NAME()));
        return (rv);
    }
    /*  read meter table and remove action_id*/
    _bcm_esw_get_policer_table_index(unit, policer_id, &policer_index); 
    rv = READ_SVM_METER_TABLEm(unit, MEM_BLOCK_ANY, policer_index, 
                                                          &meter_entry);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to read SVM_METER_TABLE entry \n", \
                                                        FUNCTION_NAME()));
        return (rv);
    }
    /* set action to 0 */
    soc_SVM_METER_TABLEm_field_set(unit, &meter_entry, POLICY_TABLE_INDEXf,
                                                           &policy_index);

    /* Write to HW*/
    rv = WRITE_SVM_METER_TABLEm(unit,MEM_BLOCK_ANY, policer_index, 
                                                            &meter_entry);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to write SVM_METER_TABLE entry \n", \
                                                        FUNCTION_NAME()));
        return (rv);
    }
    /* in case of cascade with coupling we need to configure the second set
       as well */
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_MODEf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &meter_entry, METER_MODEf,&mode);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COUPLING_FLAGf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &meter_entry, 
                                                   COUPLING_FLAGf,&coupling);
    }
    if ((coupling == 1) && (mode == GLOBAL_METER_MODE_CASCADE)) {
        rv =_bcm_global_meter_get_coupled_cascade_policer_index(unit, 
                                      policer_id, policer_control, &index); 
        /* write to hW */
        rv = soc_mem_write(unit, SVM_METER_TABLEm, MEM_BLOCK_ANY, index, 
                                                             &meter_entry);
        if (BCM_FAILURE(rv)) {
            POLICER_ERR(("%s : Unable to write SVM_METER_TABLE entry \n", \
                                                        FUNCTION_NAME()));
            return (rv);
        }
    } 

   /* decrement action usage reference count */
    if (global_meter_action_bookkeep[unit][action_id].reference_count > 0) {
        global_meter_action_bookkeep[unit][action_id].reference_count--; 
    }
    /* reset action id in policer control */
    policer_control->action_id = 0;
    GLOBAL_METER_UNLOCK(unit);
    return rv; 
}

/*
 * Function:
 *      _bcm_global_meter_write_config_to_hw
 * Purpose:
 *      Write policer configuraton to meter table.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     pol_cfg               - (IN) policer configuration 
 *     policer_id            - (IN) policer id
 *     policer_control       - (IN) Policer control info 
 * Returns:
 *     BCM_E_XXX 
 */
int  _bcm_global_meter_write_config_to_hw(int unit, 
                                          bcm_policer_config_t *pol_cfg, 
                                          bcm_policer_t policer_id,
                  _global_meter_policer_control_t *policer_control)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    int rv = BCM_E_NONE;
    svm_meter_table_entry_t data;
    uint32 flags=0;
    uint32    bucketsize = 0;      /* Bucket size.             */
    uint32    refresh_rate = 0;    /* Policer refresh rate.    */
    uint32    granularity = 0;     /* Policer granularity.     */
    uint32    refresh_bitsize;     /* Number of bits for the
                                      refresh rate field.      */
    uint32    bucket_max_bitsize;  /* Number of bits for the
                                      bucket max field.        */
    uint32 bucketcount = 0, bucket_cnt_bitsize = 0;
    uint32 mode = 0, mode_modifier = 0; 
    uint32 coupling = 0, policy_index = 0;
    uint32  refreshmax = 0; 
    int index = 0;
    uint32 current_action_id = 0;
    /* read HW register */
    _bcm_esw_get_policer_table_index(unit, policer_id, &index); 
    rv = soc_mem_read(unit, SVM_METER_TABLEm, MEM_BLOCK_ANY, index, &data);
    BCM_IF_ERROR_RETURN(rv);
    policy_index = pol_cfg->action_id;
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, POLICY_TABLE_INDEXf)) {
        soc_SVM_METER_TABLEm_field_get(unit,
              &data, POLICY_TABLE_INDEXf, &current_action_id);
        /* detach the existing action and decrement reference count */ 
        if (current_action_id) {
            _bcm_esw_policer_action_detach(unit, policer_id, current_action_id); 
        }
        soc_SVM_METER_TABLEm_field_set(unit, &data, POLICY_TABLE_INDEXf,
                                                          &policy_index);
    }

    flags = _BCM_XGS_METER_FLAG_GRANULARITY | 
                                 _BCM_XGS_METER_FLAG_GLOBAL_METER_POLICER;

    refresh_bitsize = soc_mem_field_length(unit, SVM_METER_TABLEm, 
                                                     COMMITTED_REFRESHCOUNTf);
    bucket_max_bitsize = soc_mem_field_length(unit, SVM_METER_TABLEm, 
                                                     COMMITTED_BUCKETSIZEf);

    /* Calculate policer bucket size/refresh_rate/granularity. */
     rv = _bcm_xgs_kbits_to_bucket_encoding(pol_cfg->ckbits_sec,
                                            pol_cfg->ckbits_burst,
                                            flags, refresh_bitsize,
                                            bucket_max_bitsize,
                                            &refresh_rate, &bucketsize,
                                            &granularity);
     /* Calculate initial backet count.
     * bucket size << (bit count diff - 1(sign) -1 (overflow))  - 1
     */
    if (bucketsize) {
        bucket_cnt_bitsize = soc_mem_field_length(unit, SVM_METER_TABLEm,
                                                  COMMITTED_BUCKETCOUNTf);
        bucketcount =
            (bucketsize << (bucket_cnt_bitsize - bucket_max_bitsize - 2))  - 1;
        bucketcount &= ((1 << bucket_cnt_bitsize) - 1);
    } else {
        bucketcount = 0;
    }
    refreshmax = GLOBAL_METER_REFRESH_MAX_DISABLE_LIMIT;
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COMMITTED_REFRESHMAXf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, COMMITTED_REFRESHMAXf,
                                                             &refreshmax);
    }
 
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COMMITTED_BUCKETCOUNTf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, COMMITTED_BUCKETCOUNTf,
                                                             &bucketcount);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COMMITTED_REFRESHCOUNTf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, COMMITTED_REFRESHCOUNTf,
                                                             &refresh_rate);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COMMITTED_BUCKETSIZEf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, COMMITTED_BUCKETSIZEf,
                                                               &bucketsize);
    }

    refresh_bitsize = soc_mem_field_length(unit, SVM_METER_TABLEm, 
                                                         EXCESS_REFRESHCOUNTf);
    bucket_max_bitsize = soc_mem_field_length(unit, SVM_METER_TABLEm, 
                                                           EXCESS_BUCKETSIZEf);

    if ((pol_cfg->ckbits_sec == 0) && (pol_cfg->ckbits_burst == 0)) {
        rv = _bcm_xgs_kbits_to_bucket_encoding(pol_cfg->pkbits_sec,
                                            pol_cfg->pkbits_burst,
                                            flags, refresh_bitsize,
                                            bucket_max_bitsize,
                                            &refresh_rate, &bucketsize,
                                            &granularity);

    } else {
        rv = _bcm_xgs_kbits_to_bucket_encoding_with_granularity(
                                            pol_cfg->pkbits_sec,
                                            pol_cfg->pkbits_burst,
                                            flags, refresh_bitsize,
                                            bucket_max_bitsize,
                                            &refresh_rate, &bucketsize,
                                            &granularity);
    }
     /* Calculate initial backet count.
     * bucket size << (bit count diff - 1(sign) -1 (overflow))  - 1
     */
    if (bucketsize) {
        bucket_cnt_bitsize = soc_mem_field_length(unit, SVM_METER_TABLEm,
                                                  EXCESS_BUCKETCOUNTf);
        bucketcount =
            (bucketsize << (bucket_cnt_bitsize - bucket_max_bitsize - 2))  - 1;
        bucketcount &= ((1 << bucket_cnt_bitsize) - 1);
    } else {
        bucketcount = 0;
    }
    /* Set HW register */

    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_GRANf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, METER_GRANf, &granularity);
    }
     
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, EXCESS_REFRESHMAXf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, EXCESS_REFRESHMAXf,
                                                             &refreshmax);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, EXCESS_BUCKETCOUNTf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, EXCESS_BUCKETCOUNTf,
                                                          &bucketcount);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, EXCESS_REFRESHCOUNTf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, EXCESS_REFRESHCOUNTf,
                                                             &refresh_rate);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, EXCESS_BUCKETSIZEf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, EXCESS_BUCKETSIZEf,
                                                                  &bucketsize);
    }
#if 0
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, PKT_BYTESf)) {
        soc_SVM_METER_TABLEm_field_set(unit,&data, PKT_BYTESf,);
    }
#endif
    /* MIN/MAX/MIN_MAX */
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_SHARING_MODEf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, METER_SHARING_MODEf, 
                                                 &pol_cfg->sharing_mode);
    }
    if (pol_cfg->flags & BCM_POLICER_COLOR_BLIND) {
        mode_modifier = 0; /* color blind */
    } else {
        mode_modifier = 1; /* color aware */ 
    } 

    switch (pol_cfg->mode) { 
        case bcmPolicerModeGreen:
            mode = GLOBAL_METER_MODE_DEFAULT; 
            mode_modifier = 0;
            break;

        case bcmPolicerModePassThrough:
            mode = GLOBAL_METER_MODE_DEFAULT; 
            mode_modifier = 1;
            break;

        case bcmPolicerModeSrTcm:
            mode = GLOBAL_METER_MODE_SR_TCM; 
            break;
         
        case bcmPolicerModeSrTcmModified:
            mode = GLOBAL_METER_MODE_SR_TCM_MODIFIED;
            break;

        case bcmPolicerModeTrTcm:
            mode = GLOBAL_METER_MODE_TR_TCM; 
            break;
         
        case bcmPolicerModeTrTcmDs:
            mode = GLOBAL_METER_MODE_TR_TCM_MODIFIED;
            break;

        case bcmPolicerModeCoupledTrTcmDs:
            mode = GLOBAL_METER_MODE_TR_TCM_MODIFIED;
            coupling = 1;            
            break;

        case bcmPolicerModeCascade:
            mode = GLOBAL_METER_MODE_CASCADE;
            coupling = 0;            
            break;

        case bcmPolicerModeCoupledCascade:
            mode = GLOBAL_METER_MODE_CASCADE;
            coupling = 1;            
            break;

        default:
           POLICER_VVERB(("%s : Invalid policer mode \n", FUNCTION_NAME()));
           return BCM_E_PARAM;
           break;
     }
    /* SRTCM TrTcm Modified SrTcm, Modified TrTcm, cascade, default */
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_MODEf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, METER_MODEf, &mode);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_MODIFIERf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, METER_MODIFIERf,
                                                        &mode_modifier);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COUPLING_FLAGf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &data, COUPLING_FLAGf, &coupling);
    }

    /* write to hW */
    rv = soc_mem_write(unit, SVM_METER_TABLEm, MEM_BLOCK_ANY, index, &data);
    BCM_IF_ERROR_RETURN(rv);
    /* in case of cascade with coupling we need to configure the second set
       as well */
    if ((coupling == 1) && (mode == GLOBAL_METER_MODE_CASCADE)) {
        rv =_bcm_global_meter_get_coupled_cascade_policer_index(unit, 
                                      policer_id, policer_control, &index); 
        /* write to hW */
        rv = soc_mem_write(unit, SVM_METER_TABLEm, MEM_BLOCK_ANY, 
                                                             index, &data);
        if (!BCM_SUCCESS(rv)) {
            POLICER_VVERB(("%s : Unable to configure the cascaded pair of \
                            policers \n", FUNCTION_NAME()));
            return rv;
        }
    } 
    return rv;
#else 
    return BCM_E_UNAVAIL;
#endif
}

/*
 * Function:
 *      _bcm_global_meter_get_coupled_cascade_policer_index
 * Purpose:
 *      In case of cascade with coupling get the index of the coupled
 *      pair 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer_id            - (IN) policer id
 *     policer_control       - (IN) Policer control info 
 *     new_index             - (OUT) Policer index 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_global_meter_get_coupled_cascade_policer_index(int unit, 
                                          bcm_policer_t policer_id,
                  _global_meter_policer_control_t *policer_control,
                   int *new_index) 
{

    int rv = BCM_E_NONE;
    int offset1 = 0, pool1 = 0, pool2 = 0; 
    int new_offset = 0; 
    int size_pool = 0, num_pools = 0;
    int index = 0, index_max = 0;
    int offset_mask = 0;
    int pool_mask = 0, pool_offset = 0;
    size_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
    num_pools =  SOC_INFO(unit).global_meter_pools;
    offset_mask = size_pool - 1;
    pool_offset = _shr_popcount(offset_mask);
    pool_mask = (num_pools - 1) << pool_offset; 

    offset1 = policer_control->pid & offset_mask;

    pool1 = (policer_control->pid & pool_mask) >> pool_offset; 
    pool2 = (policer_id & pool_mask) >> pool_offset;

    index_max = policer_control->no_of_policers/2;
    if (pool1 == pool2) {
        new_offset = policer_control->offset[index_max];
        *new_index = offset1 + (new_offset * size_pool);  
        return rv;
    }
    for (index = 1; index < index_max; index++) {
        if ((pool1 + policer_control->offset[index]) == pool2) {
            new_offset = policer_control->offset[index_max + index];
            *new_index = offset1 + (new_offset * size_pool);  
        }
    } 
    return rv;
}

/*
 * Function:
 *      _bcm_global_meter_base_policer_get
 * Purpose:
 *      Get the internal data sructure for a given polier id if 
 *      it is base policer. 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     pid                   - (IN) policer id
 *     policer_p             - (OUT) Policer control info 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_global_meter_base_policer_get(int unit, bcm_policer_t pid, 
                         _global_meter_policer_control_t **policer_p)
{
    _global_meter_policer_control_t *global_meter_pl = NULL;
    uint32 hash_index;      /* Entry hash.  */
    /* Input parameters check. */
    if (NULL == policer_p) {
        POLICER_VVERB(("%s : Policer control is null \n", FUNCTION_NAME()));
        return (BCM_E_PARAM);
    }
    hash_index = pid & _GLOBAL_METER_HASH_MASK;
    global_meter_pl  =  global_meter_policer_bookkeep[unit][hash_index];
    while (NULL != global_meter_pl) {
        /* Match entry id. */
        if (global_meter_pl->pid == pid) {
            *policer_p = global_meter_pl;
            return (BCM_E_NONE);
        }
        global_meter_pl = global_meter_pl->next;
    }
    /* Policer with pid == pid was not found. */
    return (BCM_E_NOT_FOUND); 
}

/*
 * Function:
 *      _bcm_global_meter_policer_get
 * Purpose:
 *      Get the internal control data structure for a given policer id.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     pid                   - (IN) policer id
 *     policer_p             - (OUT) Policer control info 
 * Returns:
 *     BCM_E_XXX 
 */

int _bcm_global_meter_policer_get(int unit, bcm_policer_t pid, 
                         _global_meter_policer_control_t **policer_p)
{
    _global_meter_policer_control_t *global_meter_pl = NULL;
    uint32 hash_index;      /* Entry hash */
    int offset1 = 0, offset2 = 0;
    int pool1 = 0, pool2 = 0;
    uint32 index = 0, index_max = 0;
    int rv = BCM_E_NONE;
    int hash_max = 0; 
    int offset_mask = 0;
    int size_pool = 0, num_pools = 0;
    int pool_mask = 0, pool_offset = 0;
    /* Input parameters check. */
    if (NULL == policer_p) {
        POLICER_VVERB(("%s : Policer control is null \n", FUNCTION_NAME()));
        return (BCM_E_PARAM);
    }
    size_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
    num_pools =  SOC_INFO(unit).global_meter_pools;
    offset_mask = size_pool - 1;
    pool_offset = _shr_popcount(offset_mask);
    pool_mask = (num_pools - 1) << pool_offset; 

    rv = _bcm_global_meter_base_policer_get(unit, pid, policer_p); 
    if (BCM_E_NOT_FOUND == rv) {
        hash_max = pid & _GLOBAL_METER_HASH_MASK;
        for (hash_index=0; hash_index <= hash_max; hash_index++) {
            global_meter_pl  = 
                      global_meter_policer_bookkeep[unit][hash_index];
            while (NULL != global_meter_pl) {
                offset1 = global_meter_pl->pid & offset_mask;
                offset2 = pid & offset_mask;

                pool1 = (global_meter_pl->pid & pool_mask) >> pool_offset; 
                pool2 = (pid & pool_mask) >> pool_offset; 

                /* in case of cascade policers, only pool number is diferent */
                if (global_meter_pl->direction == 
                                               GLOBAL_METER_ALLOC_HORIZONTAL) {
                    if (offset1 == offset2) {
                        index_max = global_meter_pl->no_of_policers;
                        for (index = 1; index < index_max; index++) {
                            if ((pool1 + global_meter_pl->offset[index]) == 
                                                                     pool2) {
                                *policer_p = global_meter_pl;
                                 return (BCM_E_NONE);
                            }
                        }     
                    }
                } else {
                    if (pool1 == pool2) {
                        if (offset2 > offset1 && offset2 <= 
                                (offset1 + global_meter_pl->no_of_policers)) {
                            *policer_p = global_meter_pl;
                             return (BCM_E_NONE);
                        }
                    }

                } 
                global_meter_pl = global_meter_pl->next;
            }
        }
        /* Policer with pid == pid was not found. */
        return (BCM_E_NOT_FOUND); 
    } else {
        return rv;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_global_meter_policer_get
 * Purpose:
 *      Get the configuration info for a given policer id
 * Parameters:
 *     unit                  - (IN) unit number 
 *     pid                   - (IN) policer id
 *     policer_p             - (OUT) Policer control info 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_global_meter_policer_get(int unit, bcm_policer_t policer_id,
                   bcm_policer_config_t *pol_cfg)
{
    int rv = BCM_E_NONE;
    _global_meter_policer_control_t *policer_control = NULL;
    /* validate policer id */
    rv = _bcm_esw_policer_validate(unit, &policer_id);
    if (BCM_FAILURE(rv)) {
        POLICER_ERR(("%s : Invalid policer id %x  \n", \
                                    FUNCTION_NAME(), policer_id));
        return (rv);
    }
    GLOBAL_METER_LOCK(unit);
   /* we come here from bcm_esw_policer_get,
      only when policer id indicates : global meter */
    rv = _bcm_global_meter_policer_get(unit, policer_id, &policer_control);
    if (BCM_SUCCESS(rv)) {
        rv = _bcm_global_meter_read_config_from_hw(unit, policer_id, pol_cfg);
        if (BCM_FAILURE(rv)) {
            POLICER_ERR(("%s : Unable to read policer config from hw %x\n", \
                                    FUNCTION_NAME(), policer_id));
            return (rv);
        }
    }
    GLOBAL_METER_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_global_meter_min_burst_size_set
 * Purpose:
 *     If the passed burst size is less than the minimum required, set it to 
 *     minimum required value. 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer_id            - (IN) policer id
 *     pol_cfg               - (IN) Policer control info 
 * Returns:
 *     BCM_E_XXX 
 */

int _bcm_global_meter_min_burst_size_set(bcm_policer_config_t *pol_cfg)
{
    uint32 min_burst = 0;
    /* min burst should be 
       max (MTU size + metering rate * refresh interval,
                                    2 * metering rate * refresh interval) */

    min_burst = (MIN_BURST_MULTIPLE * 
                 ((pol_cfg->ckbits_sec * METER_REFRESH_INTERVAL) /
                                    CONVERT_SECOND_TO_MICROSECOND));
    if (min_burst > pol_cfg->ckbits_burst) {
        POLICER_ERR(("%s : Commited burst is less than the minimum required \
                            value.  \n", FUNCTION_NAME()));
        return (BCM_E_PARAM); 
    } 
    min_burst = (MIN_BURST_MULTIPLE * 
                 ((pol_cfg->pkbits_sec * METER_REFRESH_INTERVAL) /
                                    CONVERT_SECOND_TO_MICROSECOND));
    if (min_burst > pol_cfg->pkbits_burst) {
        POLICER_ERR(("%s : Peak burst is less than the minimum required \
                            value.  \n", FUNCTION_NAME()));
        return (BCM_E_PARAM); 
    } 
    return (BCM_E_NONE); 
}
/*
 * Function:
 *      _bcm_esw_global_meter_policer_set
 * Purpose:
 *     Configure the parmeters for a policer already created.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer_id            - (IN) policer id
 *     pol_cfg               - (IN) Policer control info 
 * Returns:
 *     BCM_E_XXX 
 */

int _bcm_esw_global_meter_policer_set(int unit, bcm_policer_t policer_id,
                   bcm_policer_config_t *pol_cfg)
{
    int rv = BCM_E_NONE;
    _global_meter_policer_control_t *policer_control = NULL;
    /* validate policer id */
    rv = _bcm_esw_policer_validate(unit, &policer_id);
    if (BCM_FAILURE(rv)) {
        POLICER_ERR(("%s : Invalid policer id %x  \n", \
                                    FUNCTION_NAME(), policer_id));
        return (rv);
    }
    GLOBAL_METER_LOCK(unit);

   /* we come here from bcm_esw_policer_get,
      only when policer id indicates : global meter */
    rv = _bcm_global_meter_policer_get(unit, policer_id, &policer_control);
    if (BCM_SUCCESS(rv)) {
       /* Check whether the burst size is >= min required. If not, set it 
          to minimum required value */
        rv = _bcm_global_meter_min_burst_size_set(pol_cfg);

       /* Add code to write this to HW */
        rv =  _bcm_global_meter_write_config_to_hw(unit, pol_cfg, 
                                            policer_id, policer_control);
        if (BCM_FAILURE(rv)) {
            POLICER_ERR(("%s : Unable to write policer config to hw %x\n", \
                                    FUNCTION_NAME(), policer_id));
            return (rv);
        }
        
    } 
    GLOBAL_METER_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_esw_global_meter_policer_destroy_all
 * Purpose:
 *      Destroy all the policers.
 * Parameters:
 *     unit                  - (IN) unit number 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_global_meter_policer_destroy_all(int unit)
{
    
    int rv = BCM_E_NONE;
    int idx = 0;
    _global_meter_policer_control_t *policer_control = NULL;
    if (SOC_WARM_BOOT(unit)) {
        if (global_meter_status[unit].initialised == 0) {
            return rv;
        }
    }
    GLOBAL_METER_LOCK(unit);
    /* Iterate over all hash buckets. */
    for (idx = 0; idx < _GLOBAL_METER_HASH_SIZE; idx++) {
        policer_control = global_meter_policer_bookkeep[unit][idx];
        /* Destroy entries in each bucket. */
        while (NULL != policer_control) {
            rv = _bcm_esw_global_meter_policer_destroy2(unit, policer_control);
            if (BCM_FAILURE(rv)) {
                break;
            }
            policer_control = global_meter_policer_bookkeep[unit][idx];
        }
        if (BCM_FAILURE(rv)) {
            break;
        }
    }
    GLOBAL_METER_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_esw_global_meter_policer_traverse
 * Purpose:
 *     Traverse through all the policers that are configured and
 *     call the given call back function with policer configuration. 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     cb                    - (IN) Call back function
 *     user_data             - (OUT) Data for callback function 
 * Returns:
 *     BCM_E_XXX 
 */
int  _bcm_esw_global_meter_policer_traverse(int unit,
               bcm_policer_traverse_cb cb,  void *user_data)
{
    int rv = BCM_E_NONE;
    int idx = 0;
    bcm_policer_config_t    cfg;      /* Policer configuration.         */
    _global_meter_policer_control_t *policer_control = NULL;
    /* Iterate over all hash buckets. */
    for (idx = 0; idx < _GLOBAL_METER_HASH_SIZE; idx++) {
        policer_control = global_meter_policer_bookkeep[unit][idx];
        /* Destroy entries in each bucket. */
        while (NULL != policer_control) {
            rv = _bcm_esw_global_meter_policer_get(unit, policer_control->pid, 
                                                                         &cfg);
            if (BCM_FAILURE(rv)) {
                break;
            }
            rv = (*cb)(unit, policer_control->pid, &cfg, user_data);
            if (BCM_FAILURE(rv)) {
                break;
            }
            policer_control = policer_control->next;
        }
        if (BCM_FAILURE(rv)) {
            break;
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_esw_global_meter_policer_destroy
 * Purpose:
 *     Destroys a policer and readjusts action
 *     usage reference count and policer allocation management data struture.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer_id            - (IN) policer id 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_global_meter_policer_destroy(int unit, bcm_policer_t policer_id)
{
    int rv = BCM_E_NONE;
    _global_meter_policer_control_t *policer_control = NULL;

    if (!soc_feature(unit, soc_feature_global_meter)) {
        return BCM_E_UNAVAIL;
    }
    /* validate policer id */
    rv = _bcm_esw_policer_validate(unit, &policer_id);
    if (BCM_FAILURE(rv)) {
        POLICER_VERB(("%s : Invalid policer id %x  \n", \
                                             FUNCTION_NAME(), policer_id));
        return (rv);
    }
    GLOBAL_METER_LOCK(unit);
    rv = _bcm_global_meter_base_policer_get(unit, 
                                   policer_id, &policer_control);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_VERB(("%s : Unable to get policer control for policer id %x\n", \
                                             FUNCTION_NAME(), policer_id));
        return (rv);
    }
    rv = _bcm_esw_global_meter_policer_destroy2(unit, policer_control);
    GLOBAL_METER_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_esw_global_meter_policer_destroy2
 * Purpose:
 *     Internal function that destroys a policer and readjusts action
 *     usage reference count and policer allocation management data struture.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer_id            - (IN) policer id 
 *     policer_control       - (IN) policer control info 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_global_meter_policer_destroy2(int unit, 
               _global_meter_policer_control_t *policer_control) 
{

    int index = 0;
    uint32 pool = 0, pid = 0, numbers = 0;
    int rv=BCM_E_NONE;
    int offset_mode = 0;
    int pool_id = 0;
    int offset_mask = 0;
    int size_pool = 0, num_pools = 0;
    int pool_mask = 0, pool_offset = 0;
    bcm_policer_t  macro_meter_policer = 0;
    int macro_flow_meter_index = 0;
    svm_macroflow_index_table_entry_t   *buf;        
    svm_macroflow_index_table_entry_t   *entry;        
    int entry_mem_size;    /* Size of table entry. */
    int entry_index = 0; 
    int entry_index_max = 0; 
    int entry_modified = 0;

    /* Make sure policer is not attached to any entry. */
    if (0 != policer_control->ref_count) {
        POLICER_VVERB(("%s : Policer is still in use  \n", FUNCTION_NAME()));
        return (BCM_E_BUSY);
    }
    if (global_meter_action_bookkeep[unit]\
                             [policer_control->action_id].reference_count > 0)
    {
        global_meter_action_bookkeep[unit]\
                               [policer_control->action_id].reference_count--; 
    }
    size_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
    num_pools =  SOC_INFO(unit).global_meter_pools;
    offset_mask = size_pool - 1;
    pool_offset = _shr_popcount(offset_mask);
    pool_mask = (num_pools - 1) << pool_offset; 

    pid = policer_control->pid & offset_mask;
    pool = (policer_control->pid & pool_mask) >> pool_offset;
    numbers = policer_control->no_of_policers; 

    offset_mode = 
             ((policer_control->pid & BCM_POLICER_GLOBAL_METER_MODE_MASK) >> 
                                    BCM_POLICER_GLOBAL_METER_MODE_SHIFT);
    if (offset_mode >= 1) {
        offset_mode = offset_mode - 1;
    } 

    /* check horizontal or vertical allocation and then free the policer */
    if (policer_control->direction == GLOBAL_METER_ALLOC_VERTICAL) {
        rv = shr_aidxres_list_free(meter_alloc_list_handle[unit][pool], pid);
        if (BCM_FAILURE(rv)) {
            POLICER_VVERB(("%s : Unable to free policer handle\n",
                                                       FUNCTION_NAME()));
            return (rv);
        }
        for (index = 0; index < numbers; index++) {
            rv = _bcm_gloabl_meter_unreserve_bloc_horizontally(unit,
                                                    pool, pid+index);
            if (!BCM_SUCCESS(rv)) {
                POLICER_VVERB(("%s : Unable to free policer handle in hz \
                                     index management\n", FUNCTION_NAME()));
                return BCM_E_INTERNAL;
            }
        }
    } else if (policer_control->direction == GLOBAL_METER_ALLOC_HORIZONTAL) {
        pool_id = pool;
        for (index = 0;index < numbers; index++) {
            if (index > 0) {
                pool_id = pool + policer_control->offset[index];
            }
            rv = shr_aidxres_list_free(
                   meter_alloc_list_handle[unit][pool_id], pid);
            if (BCM_FAILURE(rv)) {
                POLICER_VVERB(("%s : Unable to free policer handle\n", \
                                                       FUNCTION_NAME()));
                return (rv);
            }
        }
        rv = _bcm_gloabl_meter_free_horizontally(unit, pool, pid, numbers,
                                                      policer_control->offset);
        if (BCM_FAILURE(rv)) {
            POLICER_VVERB(("%s : Unable to free policer handle in hz \
                                     index management\n", FUNCTION_NAME()));
            return (rv);
        }
    }

   /* Delete policer from policers hash. */
    _GLOBAL_METER_HASH_REMOVE(global_meter_policer_bookkeep[unit],
                   _global_meter_policer_control_t,
                   policer_control, (pid & _GLOBAL_METER_HASH_MASK));
    /* decrement mode reference count  */
    if (offset_mode) {
        bcm_policer_svc_meter_dec_mode_reference_count(unit, offset_mode);
    }  

    /*  get the HW index for the policer to be deleted */
    rv = _bcm_esw_get_policer_table_index(unit, policer_control->pid,
                                                          &index);
    if (BCM_FAILURE(rv)) {
        POLICER_ERR(("%s : Unable to Get policer index for \
                            policer \n", FUNCTION_NAME()));
        return rv;
    }

    /* Write the index of the macro-meter at "micro_flow_index" location 
           of SVM_MACROFLOW_INDEX_TABLE */  
    if (policer_control->direction == GLOBAL_METER_ALLOC_VERTICAL) {

        entry_mem_size = sizeof(svm_macroflow_index_table_entry_t);
        entry_index_max = index + numbers - 1;

        /* Allocate buffer to store the DMAed table entries. */
        buf = soc_cm_salloc(unit, entry_mem_size * numbers,
                              "svm macro flow index table entry buffer");
        if (NULL == buf) {
            return (BCM_E_MEMORY);
        }
        /* Initialize the entry buffer. */
        sal_memset(buf, 0, sizeof(entry_mem_size) * numbers);

        /* Read the table entries into the buffer. */
        rv = soc_mem_read_range(unit, SVM_MACROFLOW_INDEX_TABLEm,
                                MEM_BLOCK_ALL, index,
                                entry_index_max, buf);
        if (BCM_FAILURE(rv)) {
            if (buf) {
                soc_cm_sfree(unit, buf);
            }
            return rv;
        }

        /* Iterate over the table entries. */
        for (entry_index = 0; entry_index < numbers; entry_index++) {
            entry = soc_mem_table_idx_to_pointer(unit, 
                              SVM_MACROFLOW_INDEX_TABLEm,
                              svm_macroflow_index_table_entry_t *,
                              buf, entry_index);
            soc_mem_field_get(unit, SVM_MACROFLOW_INDEX_TABLEm, (void *)entry,
                           MACROFLOW_INDEXf, (uint32 *)&macro_flow_meter_index);
            if (macro_flow_meter_index > 0) {
                _bcm_esw_get_policer_id_from_index_offset(unit, 
                               macro_flow_meter_index, 0, &macro_meter_policer);
                rv = _bcm_esw_policer_decrement_ref_count(unit, 
                                                      macro_meter_policer);
                if (BCM_FAILURE(rv)) {
                    POLICER_VERB(("%s : Unable to decrement ref count \
                             for macro meter provided\n", FUNCTION_NAME()));
                    if (buf) {
                        soc_cm_sfree(unit, buf);
                    } 
                    return rv;
                }
                macro_flow_meter_index = 0; /* clear macro flow index entry */
                soc_mem_field_set(unit, SVM_MACROFLOW_INDEX_TABLEm, 
                                     (void *)entry, MACROFLOW_INDEXf, 
                                     (uint32 *)&macro_flow_meter_index);
                entry_modified = 1;
            }
        }
        if (entry_modified) { 
            if ((rv = soc_mem_write_range(unit, SVM_MACROFLOW_INDEX_TABLEm,
                                  MEM_BLOCK_ALL, index,
                                  entry_index_max, buf)) < 0) {
                if (BCM_FAILURE(rv)) {
                         POLICER_VERB(("%s : Unable to write to macro flow \
                                       index table at index provided\n", 
                                       FUNCTION_NAME()));
                    if (buf) {
                        soc_cm_sfree(unit, buf);
                    } 
                    return rv;
                } 
            }
        }
        if (buf) {
            soc_cm_sfree(unit, buf);
        }
        return rv;
    }  
    /* De-allocate policer descriptor. */
    sal_free(policer_control);
    return rv;
}

/*
 * Function:
 *      _bcm_esw_global_meter_set_cascade_info_to_hw
 * Purpose:
 *      In case of cascade meter, write configuration info that is
 *      specific to cascade meters to HW
 * Parameters:
 *     unit                  - (IN) unit number 
 *     numbers               - (IN) Number of policers 
 *     policer_id            - (IN) base policer id 
 *     mode                  - (IN) policer group mode 
 *     pid_offset            - (IN) Offset w.r.t base policer 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_global_meter_set_cascade_info_to_hw(int unit,int  numbers,
                          bcm_policer_t policer_id,
                          bcm_policer_group_mode_t mode, uint8 *pid_offset) 
{
    int rv = BCM_E_NONE;
    svm_meter_table_entry_t data;
    int index = 0;
    uint32 start_of_chain = 0, end_of_chain = 0, meter_mode = 0; 
    uint32 coupling_flag = 0; 
    int size_pool;
    int policer_index = 0;
    int table_offset = 0;
    size_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
   
    _bcm_esw_get_policer_table_index(unit, policer_id, &policer_index); 
   
    for (index=0; index < numbers; index++) {
        if (index > 0) {
            table_offset = policer_index + (pid_offset[index] * size_pool);
        } else {
            table_offset = policer_index;
        }
        /* read HW register */
        rv = soc_mem_read(unit, SVM_METER_TABLEm, MEM_BLOCK_ANY, 
                                                  table_offset,  &data);
        if (BCM_FAILURE(rv)) {
            POLICER_VVERB(("%s : Unable to read SVC METER TABLE at \
                                offset %d\n", FUNCTION_NAME(), table_offset));
            return (rv);
        }
        switch (mode) {
            case bcmPolicerGroupModeCascade:
                if (index == 0) {
                    start_of_chain = 1;
                } else {
                    start_of_chain = 0;
                }
                if (index == (numbers - 1)) {
                    end_of_chain = 1;
                } else { 
                    end_of_chain = 0;
                }
                meter_mode = 1;
                coupling_flag = 0;
                break;


            case bcmPolicerGroupModeCascadeWithCoupling:
                if ((index == 0) || (index == (numbers / 2))) {
                    start_of_chain = 1;
                } else {
                    start_of_chain = 0;
                }
                if ((index == (numbers - 1)) || (index == ((numbers/2) - 1))) {
                    end_of_chain = 1;
                } else { 
                    end_of_chain = 0;
                }
                meter_mode = 1;
                coupling_flag = 1;
                break;

            default:
                POLICER_VVERB(("%s : Invalid mode passed \n", FUNCTION_NAME()));
                return BCM_E_NONE;
                break;
        }
        if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, START_OF_CHAINf)) {
            soc_SVM_METER_TABLEm_field_set(unit, &data, 
                                            START_OF_CHAINf, &start_of_chain);
        }
        if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, END_OF_CHAINf)) {
            soc_SVM_METER_TABLEm_field_set(unit, &data, 
                                               END_OF_CHAINf, &end_of_chain);
        }
        if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_MODEf)) {
            soc_SVM_METER_TABLEm_field_set(unit, &data, 
                                                    METER_MODEf, &meter_mode);
        }
        if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COUPLING_FLAGf)) {
            soc_SVM_METER_TABLEm_field_set(unit, &data, 
                                             COUPLING_FLAGf, &coupling_flag);
        }
        /* write to hW */
        rv = soc_mem_write(unit, SVM_METER_TABLEm, MEM_BLOCK_ANY, 
                                                         table_offset, &data);
        if (BCM_FAILURE(rv)) {
            POLICER_VVERB(("%s : Unable to write SVC METER TABLE at \
                                offset %d\n", FUNCTION_NAME(), table_offset));
            return (rv);
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_esw_policer_set_offset_table_map_to_increasing_value
 * Purpose:
 *     Set first n entires of the offset table with a incremental value. 
 *     starting from 0
 * Parameters:
 *     num_offsets           - (IN) Number of offsets 
 *     offset_map            - (IN/OUT) Offset Map
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_policer_set_offset_table_map_to_increasing_value(uint32 num_offsets, 
                               offset_table_entry_t *offset_map)
{
    int rv = BCM_E_NONE;
    int i = 0;
    if (num_offsets >= BCM_SVC_METER_MAP_SIZE_256) {  
        POLICER_VVERB(("%s : Number of offsets passed is more than map table \
                             size %d\n", FUNCTION_NAME(), num_offsets));
        return BCM_E_INTERNAL;
    } 
    for(i = 0; i < num_offsets; i++) {
        offset_map[i].offset= i; 
        offset_map[i].meter_enable = 1;
    } 
    return rv;
}

/*
 * Function:
 *      _bcm_esw_policer_set_offset_table_map_to_a_value_with_pool
 * Purpose:
 *     Set first n entires of the offset table with a specified value. 
 * Parameters:
 *     num_offsets           - (IN) Number of offsets 
 *     value                 - (IN) Value to be set 
 *     offset_map            - (IN/OUT) Offset Map
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_policer_set_offset_table_map_to_a_value_with_pool(uint32 num_offsets, 
                               uint32 value,  
                               offset_table_entry_t *offset_map)
{
    int rv = BCM_E_NONE;
    int i = 0;
    if (num_offsets >= BCM_SVC_METER_MAP_SIZE_256) {  
        POLICER_VVERB(("%s : Number of offsets passed is more than map table \
                             size %d\n", FUNCTION_NAME(), num_offsets));
        return BCM_E_INTERNAL;
    } 
    for(i = 0; i < num_offsets; i++) {
        offset_map[i].offset= value; 
        offset_map[i].pool= i; 
        offset_map[i].meter_enable = 1;
    } 
    return rv;
}
/*
 * Function:
 *      _bcm_esw_policer_set_offset_table_map_to_a_value
 * Purpose:
 *     Set first n entires of the offset table with a specified value. 
 * Parameters:
 *     num_offsets           - (IN) Number of offsets 
 *     value                 - (IN) Value to be set 
 *     offset_map            - (IN/OUT) Offset Map
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_policer_set_offset_table_map_to_a_value(uint32 num_offsets, 
                               uint32 value,  
                               offset_table_entry_t *offset_map)
{
    int rv = BCM_E_NONE;
    int i = 0;
    if (num_offsets >= BCM_SVC_METER_MAP_SIZE_256) {  
        POLICER_VVERB(("%s : Number of offsets passed is more than map table \
                             size %d\n", FUNCTION_NAME(), num_offsets));
        return BCM_E_INTERNAL;
    } 
    for(i = 0; i < num_offsets; i++) {
        offset_map[i].offset= value; 
        offset_map[i].meter_enable = 1;
    } 
    return rv;
}
/*
 * Function:
 *      _bcm_esw_policer_set_offset_table_map
 * Purpose:
 *     Create the map of policer offsets that needs to be written to offset
 *     table
 * Parameters:
 *     num_offsets           - (IN) Number of offsets 
 *     table_offset          - (IN) Offset positons 
 *     value                 - (IN) Values to be set 
 *     offset_map            - (IN/OUT) Offset Map
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_policer_set_offset_table_map(uint32 num_offsets, 
                               uint32 *table_offset, 
                               uint32 *value,  
                               offset_table_entry_t *offset_map) 
{
    int rv = BCM_E_NONE;
    int i = 0;
    for(i = 0; i < num_offsets; i++) {
        if ((table_offset[i] >= BCM_SVC_METER_MAP_SIZE_256) || 
               (value[i] >= BCM_SVC_METER_MAP_SIZE_256)) {
            POLICER_VVERB(("%s : Offset/value passed is more than map table \
                size %d %d\n", FUNCTION_NAME(), table_offset[i], value[i]));
            return BCM_E_INTERNAL;
        } 
        offset_map[table_offset[i]].offset= value[i]; 
        offset_map[table_offset[i]].meter_enable = 1; 
    }
    return rv;
}


/*
 * Function:
 *      _bcm_esw_policer_group_set_mode_and_map
 * Purpose:
 *
 * Parameters:
 *     unit                  - (IN) unit number 
 *     mode                  - (IN) Policer group mode 
 *     npolicers             - (IN) Number of policers 
 *     direction             - (IN) Horizontal/Vertical 
 *     mode_attr             - (IN) Offset mode attributes
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_policer_group_set_mode_and_map( int unit, 
                               bcm_policer_group_mode_t mode, 
                               int *npolicers, uint32 *direction,  
                               bcm_policer_svc_meter_attr_t *mode_attr) 
{
    int rv = BCM_E_NONE;
    int num_pools = 0;
    uint32 table_offset[10] = { 0 };
    uint32 value[10] = { 0 };
    pkt_attr_bits_t  *pkt_attr = NULL;
    uint32                   map_index = 0;
    compressed_pkt_res_attr_map_t *pkt_res_map = NULL;
    compressed_pri_cnf_attr_map_t  *pri_cnf_map = NULL;
    offset_table_entry_t   *offset_map = NULL;
    uint32                  type = 0;
    uint32                  offset = 0;
    uint8                   *pkt_res = NULL;
    int                     no_entries_to_populate = 0;
    int                     shift_bits = 0;
    int                     index_max = 0;
    mode_attr->mode_type_v = uncompressed_mode; 
    num_pools =  SOC_INFO(unit).global_meter_pools;

    if (SOC_IS_KATANA(unit)) {
        pkt_res = &kt_pkt_res[0];
    } else if (SOC_IS_TRIUMPH3(unit)) {
        pkt_res = &tr3_pkt_res[0];
    } else if (SOC_IS_KATANA2(unit)) {
        pkt_res = &kt2_pkt_res[0];
    } else {
        /*
         * Other devices do not support service meters, return error.
         */
        return (BCM_E_UNAVAIL);
    }

    switch (mode) {
        /* A single policer used for all traffic types */
        case bcmPolicerGroupModeSingle:
        {
            *npolicers = 1;
            rv =  _bcm_esw_policer_set_offset_table_map_to_a_value(
                      FP_RESOLUTION_MAX, 0, 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }    
        /* **************************************************************** */
         /* A dedicated policer per traffic type Unicast,multicast,broadcast */
         /* 1) L2UC_PKT | KNOWN_L3UC_PKT | UNKNOWN_L3UC_PKT                  */
         /* 2) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT|                              */
         /* 3) L2BC_PKT|                                                     */
         /* **************************************************************** */
        case bcmPolicerGroupModeTrafficType:
        {
            *npolicers = 3;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
               BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
            table_offset[0] = pkt_res[KNOWN_L2UC_PKT];
            table_offset[1] = pkt_res[KNOWN_L3UC_PKT];
            table_offset[2] = pkt_res[UNKNOWN_L3UC_PKT];
            table_offset[3] = pkt_res[KNOWN_L2MC_PKT];
            table_offset[4] = pkt_res[UNKNOWN_L2MC_PKT];
            table_offset[5] = pkt_res[L2BC_PKT];
            value[3] = 1;
            value[4] = 1;
            value[5] = 2;
            rv =  _bcm_esw_policer_set_offset_table_map(6, &table_offset[0], 
                      &value[0], 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }    
         /************************************************************** */
         /* A pair of policers where the base policer is used for dlf and */
         /* the other is used for all traffic types                       */
         /* 1) L2DLF_PKT(UNKNOWN_L2UC_PKT)                                */
         /* 2) UNKNOWN_PKT | CONTROL_PKT|BPDU_PKT|L2BC_PKT|L2UC_PKT|      */
         /*    UNKNOWN_IPMC_PKT|KNOWN_IPMC_PKT|KNOWN_L2MC_PKT|            */
         /*    UNKNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT|KNOWN_L3UC_PKT|          */
         /*    UNKNOWN_L3UC_PKT|KNOWN_MPLS_PKT|KNOWN_MPLS_L3_PKT|         */
         /*    KNOWN_MPLS_L2_PKT|UNKNOWN_MPLS_PKT|KNOWN_MIM_PKT|          */
         /*    UNKNOWN_MIM_PKT|KNOWN_MPLS_MULTICAST_PKT                   */
         /* ************************************************************* */
 
        case bcmPolicerGroupModeDlfAll:
        {
            *npolicers = 2;
            mode_attr->uncompressed_attr_selectors_v.\
                     uncompressed_attr_bits_selector =
               BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_a_value(
                      FP_RESOLUTION_MAX, 1, 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            table_offset[0] = pkt_res[UNKNOWN_L2UC_PKT];
            value[0] = 0;
            rv =  _bcm_esw_policer_set_offset_table_map(1, &table_offset[0], 
                      &value[0], 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }    
         /* ******************************************************* */
         /* A dedicated policer for unknown unicast, known unicast, */
         /* multicast, broadcast                                    */
         /* 1) UNKNOWN_L3UC_PKT                                     */
         /* 2) L2UC_PKT | KNOWN_L3UC_PKT                            */
         /* 3) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT                      */
         /* 4) L2BC_PKT                                             */
         /* ******************************************************* */
        case bcmPolicerGroupModeTyped:
        {
            *npolicers = 4;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
               BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
            table_offset[0] = pkt_res[UNKNOWN_L3UC_PKT];
            table_offset[1] = pkt_res[KNOWN_L3UC_PKT];
            table_offset[2] = pkt_res[KNOWN_L2UC_PKT];
            table_offset[3] = pkt_res[KNOWN_L2MC_PKT];
            table_offset[4] = pkt_res[UNKNOWN_L2MC_PKT];
            table_offset[5] = pkt_res[L2BC_PKT];
            value[1] = 1;
            value[2] = 1;
            value[3] = 2;
            value[4] = 2;
            value[5] = 3;
            rv =  _bcm_esw_policer_set_offset_table_map(6, &table_offset[0], 
                      &value[0], 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }    
         /* ******************************************************* */
         /* A dedicated policer for unknown unicast, known unicast, */
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
        case bcmPolicerGroupModeTypedAll:
        {
            *npolicers = 5;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
               BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_a_value(
                      FP_RESOLUTION_MAX, 4, 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            table_offset[0] = pkt_res[UNKNOWN_L3UC_PKT];
            table_offset[1] = pkt_res[KNOWN_L3UC_PKT];
            table_offset[2] = pkt_res[KNOWN_L2UC_PKT];
            table_offset[3] = pkt_res[KNOWN_L2MC_PKT];
            table_offset[4] = pkt_res[UNKNOWN_L2MC_PKT];
            table_offset[5] = pkt_res[L2BC_PKT];
            value[1] = 1;
            value[2] = 1;
            value[3] = 2;
            value[4] = 2;
            value[5] = 3;
            rv =  _bcm_esw_policer_set_offset_table_map(6, &table_offset[0], 
                      &value[0], 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }    
        /* **************************************************************   */
         /* A single policer used for all traffic types with an additional   */
         /* policer for control traffic                                      */
         /* 1) UNKNOWN_PKT|L2BC_PKT|L2UC_PKT|L2DLF_PKT|                      */
         /*    UNKNOWN_IPMC_PKT|KNOWN_IPMC_PKT|KNOWN_L2MC_PKT|               */
         /*    UNKNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT|KNOWN_L3UC_PKT|             */
         /*    UNKNOWN_L3UC_PKT|KNOWN_MPLS_PKT|KNOWN_MPLS_L3_PKT|            */
         /*    KNOWN_MPLS_L2_PKT|UNKNOWN_MPLS_PKT|KNOWN_MIM_PKT|             */
         /*    UNKNOWN_MIM_PKT|KNOWN_MPLS_MULTICAST_PKT                      */
         /* 2) CONTROL_PKT|BPDU_PKT                                          */
         /* **************************************************************   */
        case bcmPolicerGroupModeSingleWithControl:
        {
            *npolicers = 2;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
               BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_a_value(
                      FP_RESOLUTION_MAX, 0, 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            table_offset[0] = pkt_res[CONTROL_PKT];
            table_offset[1] = pkt_res[BPDU_PKT];
            value[0] = 1;
            value[1] = 1;
            rv =  _bcm_esw_policer_set_offset_table_map(2, &table_offset[0], 
                      &value[0], 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }    
         /* ********************************************************  */
         /* A dedicated policer per traffic type unicast, multicast,  */
         /* broadcast with an additional counter for control traffic  */
         /* 1) L2UC_PKT | KNOWN_L3UC_PKT | UNKNOWN_L3UC_PKT           */
         /* 2) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT|                       */
         /* 3) L2BC_PKT|                                              */
         /* 4) CONTROL_PKT|BPDU_PKT                                   */
         /* ********************************************************  */
        case bcmPolicerGroupModeTrafficTypeWithControl:
        {
            *npolicers = 4;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
               BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
            table_offset[0] = pkt_res[UNKNOWN_L3UC_PKT];
            table_offset[1] = pkt_res[KNOWN_L3UC_PKT];
            table_offset[2] = pkt_res[KNOWN_L2UC_PKT];
            table_offset[3] = pkt_res[KNOWN_L2MC_PKT];
            table_offset[4] = pkt_res[UNKNOWN_L2MC_PKT];
            table_offset[5] = pkt_res[L2BC_PKT];
            table_offset[6] = pkt_res[CONTROL_PKT];
            table_offset[7] = pkt_res[BPDU_PKT];
            value[3] = 1;
            value[4] = 1;
            value[5] = 2;
            value[6] = 3;
            value[7] = 3;
            rv =  _bcm_esw_policer_set_offset_table_map(8, &table_offset[0], 
                      &value[0], 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }    
         /* ************************************************************** */
         /* A set of 3 policers where the base policer is used for control, */
         /* the next one for dlf and the other counter is used for all     */
         /* traffic types                                                  */
         /* 1) CONTROL_PKT|BPDU_PKT                                        */
         /* 2) L2DLF_PKT(UNKNOWN_L2UC_PKT)                                 */
         /* 3)UNKNOWN_PKT | CONTROL_PKT|BPDU_PKT|L2BC_PKT|L2UC_PKT|        */
         /*   UNKNOWN_IPMC_PKT|KNOWN_IPMC_PKT|KNOWN_L2MC_PKT|              */
         /*   UNKNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT|KNOWN_L3UC_PKT|            */
         /*   UNKNOWN_L3UC_PKT|KNOWN_MPLS_PKT|KNOWN_MPLS_L3_PKT|           */
         /*   KNOWN_MPLS_L2_PKT|UNKNOWN_MPLS_PKT|KNOWN_MIM_PKT|            */
         /*   UNKNOWN_MIM_PKT|KNOWN_MPLS_MULTICAST_PKT                     */
         /* ************************************************************** */
        case bcmPolicerGroupModeDlfAllWithControl:
        {
            *npolicers = 3;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
               BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_a_value(
                      FP_RESOLUTION_MAX, 2, 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            table_offset[0] = pkt_res[CONTROL_PKT];
            table_offset[1] = pkt_res[BPDU_PKT];
            table_offset[2] = pkt_res[UNKNOWN_L2UC_PKT];
            value[2] = 1;
            rv =  _bcm_esw_policer_set_offset_table_map(3, &table_offset[0], 
                      &value[0], 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }    
         /* **************************************************************** */
         /* A dedicated policer for control, unknown unicast, known unicast, */
         /* multicast, broadcast                                             */
         /* 1) CONTROL_PKT|BPDU_PKT                                          */
         /* 2) UNKNOWN_L3UC_PKT                                              */
         /* 3) KNOWN_L2UC_PKT | KNOWN_L3UC_PKT                               */
         /* 4) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT                               */
         /* 5) L2BC_PKT                                                      */
         /* **************************************************************** */
        case bcmPolicerGroupModeTypedWithControl:  
        {
            *npolicers = 5;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
               BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
            table_offset[0] = pkt_res[CONTROL_PKT];
            table_offset[1] = pkt_res[BPDU_PKT];
            table_offset[2] = pkt_res[UNKNOWN_L3UC_PKT];
            table_offset[3] = pkt_res[KNOWN_L2UC_PKT];
            table_offset[4] = pkt_res[KNOWN_L3UC_PKT];
            table_offset[5] = pkt_res[KNOWN_L2MC_PKT];
            table_offset[6] = pkt_res[UNKNOWN_L2MC_PKT];
            table_offset[7] = pkt_res[L2BC_PKT];
            value[2] = 1;
            value[3] = 2;
            value[4] = 2;
            value[5] = 3;
            value[6] = 3;
            value[7] = 4;
            rv =  _bcm_esw_policer_set_offset_table_map(8, &table_offset[0], 
                      &value[0], 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }    
        /* ***************************************************************** */
         /* A dedicated policer for control, unknown unicast, known unicast,  */
         /* multicast, broadcast and one for all traffic (not already policed)*/
         /* 1) CONTROL_PKT|BPDU_PKT                                           */
         /* 2) UNKNOWN_L3UC_PKT                                               */
         /* 3) KNOWN_L2UC_PKT | KNOWN_L3UC_PKT                                */
         /* 4) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT                                */
         /* 5) L2BC_PKT                                                       */
         /* 6) UNKNOWN_PKT|L2DLF_PKT|UNKNOWN_IPMC_PKT|KNOWN_IPMC_PKT          */
         /*    KNOWN_MPLS_PKT | KNOWN_MPLS_L3_PKT|KNOWN_MPLS_L2_PKT|          */
         /*    UNKNOWN_MPLS_PKT|KNOWN_MIM_PKT|UNKNOWN_MIM_PKT|                */
         /*    KNOWN_MPLS_MULTICAST_PKT                                       */
         /* ***************************************************************** */
        case bcmPolicerGroupModeTypedAllWithControl:
        {
            *npolicers = 6;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
               BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_a_value(
                      FP_RESOLUTION_MAX, 5, 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            table_offset[0] = pkt_res[CONTROL_PKT];
            table_offset[1] = pkt_res[BPDU_PKT];
            table_offset[2] = pkt_res[UNKNOWN_L3UC_PKT];
            table_offset[3] = pkt_res[KNOWN_L2UC_PKT];
            table_offset[4] = pkt_res[KNOWN_L3UC_PKT];
            table_offset[5] = pkt_res[KNOWN_L2MC_PKT];
            table_offset[6] = pkt_res[UNKNOWN_L2MC_PKT];
            table_offset[7] = pkt_res[L2BC_PKT];
            value[2] = 1;
            value[3] = 2;
            value[4] = 2;
            value[5] = 3;
            value[6] = 3;
            value[7] = 4;
            rv =  _bcm_esw_policer_set_offset_table_map(8, &table_offset[0], 
                      &value[0], 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }    
        /* ******************************************************** */
        /* A set of 8(2^3) policers selected based on outer Vlan pri*/
        /* outer_dot1p; 3 bits 1..8                                 */
        /* ******************************************************** */

        case bcmPolicerGroupModeDot1P:
        {
            *npolicers = 8;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
                  BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_increasing_value(8, 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }       
        /* ******************************************************** */
        /* A set of 8(2^3) policers selected based on inner Vlan pri*/
        /* inner_dot1p; 3 bits 1..8                                 */
        /* ******************************************************** */

        case bcmPolicerGroupModeInnerDot1P:
        {
            *npolicers = 8;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
                  BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_increasing_value(8, 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }       
        /* **************************************************** */
        /* A set of 16(2^4) policers based on internal priority */
        /* 1..16 INT_PRI bits: 4bits                            */
        /* **************************************************** */
        case bcmPolicerGroupModeIntPri:
        {
            *npolicers = 16;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
                  BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_increasing_value(16,
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }     
        /* ********************************************************** */
        /* set of 64 policers(2^(4+2)) based on Internal priority+CNG */
        /* 1..64 (INT_PRI bits: 4bits + CNG 2 bits                    */
        /* 1..64 (INT_PRI bits: 4bits + CNG 2 bits                    */
        /* ********************************************************** */
        case bcmPolicerGroupModeIntPriCng:
        {
            *npolicers = 64;
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
                  BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_CNG_ATTR_BITS | 
                  BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_increasing_value(64,
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
                 
            break;
        }       
         /* ****************************************** */
         /* A set of 2 policers(2^1) based on SVP type */
         /* 1..2 (SVP 1 bit). In case of KT2, SVP is 3 */
         /* 3 bits and hence no of policers is 8       */
         /* ********************************************/
        case bcmPolicerGroupModeSvpType:
        {
            *npolicers = 2;
            if (SOC_IS_KATANA2(unit)) {
                *npolicers = 8;
            }
            mode_attr->uncompressed_attr_selectors_v.\
                   uncompressed_attr_bits_selector =
                     BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_increasing_value(*npolicers,
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        } 

         /* ******************************************** */
         /* A set of 64 policers(2^6) based on DSCP bits */
         /* 1..64 (6 bits from TOS 8 bits)               */
         /* ******************************************** */      
        case bcmPolicerGroupModeDscp:
        
        {
            *npolicers = 64;
            mode_attr->uncompressed_attr_selectors_v.\
                        uncompressed_attr_bits_selector =
                      BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_TOS_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_increasing_value(64,
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }       
         /* ******************************************** */
         /* A set of n policers based on user input */
         /* ******************************************** */      
        case bcmPolicerGroupModeCascade:
            if ((*npolicers > num_pools) || (*npolicers == 0 )) {
                POLICER_VERB(("%s : Invalid number of Policers \n", \
                                                          FUNCTION_NAME()));
                return BCM_E_PARAM;
            }
            *direction = GLOBAL_METER_ALLOC_HORIZONTAL;
            mode_attr->mode_type_v = cascade_mode; 
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
                  BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_a_value_with_pool(
                      *npolicers, 0, 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;

         /* ******************************************** */
         /* A set of n policers based on user input */
         /* ******************************************** */      
        case bcmPolicerGroupModeCascadeWithCoupling:
        {
            if ((*npolicers > 0 ) && 
                 ((*npolicers * 2) <= num_pools)) {
                *direction = GLOBAL_METER_ALLOC_HORIZONTAL;
                *npolicers *= 2;
            } else {
                POLICER_VERB(("%s : Invalid number of Policers \n", \
                                                          FUNCTION_NAME()));
                return BCM_E_PARAM;
            }
            mode_attr->mode_type_v = cascade_mode; 
            mode_attr->uncompressed_attr_selectors_v.\
                      uncompressed_attr_bits_selector =
                  BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS;
            rv =  _bcm_esw_policer_set_offset_table_map_to_a_value_with_pool(
                      *npolicers/2, 0, 
                      &mode_attr->uncompressed_attr_selectors_v.offset_map[0]); 
            BCM_IF_ERROR_RETURN(rv);
            break;
        }
         /* ************************************************************** */
         /* N+1 policers where in the base counter is used for dlf and next N */
         /* are used as per Cos                                            */
         /* 1) L2_DLF(UNKNOWN_L2UC_PKT)                                    */
         /* 2..17) INT_PRI bits: 4bits                                     */
         /* ************************************************************** */
        case bcmPolicerGroupModeDlfIntPri:  
        {
            *npolicers = 17;
            mode_attr->mode_type_v = compressed_mode; 
            pkt_attr = &mode_attr->compressed_attr_selectors_v.pkt_attr_bits_v;
            pkt_attr->pkt_resolution = 1;
            pkt_attr->int_pri = 4;

            /* Reset pkt_resolution map */
            pkt_res_map =  &mode_attr->compressed_attr_selectors_v.\
                                             compressed_pkt_res_attr_map_v[0];
            index_max = soc_mem_index_max(unit, ING_SVM_PKT_RES_MAPm);
            for (map_index = 0; map_index <= index_max; map_index++) {
                pkt_res_map[map_index] = 0;
            }
            /* set pkt_resolution map for DLF to 1. In the pkt_res_map first
               2 bits correspond to SVP  and Drop and hence are don't care
               bits. So we need to set our meter value at 4 locations*/
            no_entries_to_populate = 4;
            shift_bits = 2;
            if (SOC_IS_KATANA2(unit)) {
                /* In kt2 4 bits correspond to SVP(3)+drop(1). So we need to set
                   our meter value at 16 locations */
                no_entries_to_populate = 16;
                shift_bits = 4;
            }
            for (offset = 0; offset < no_entries_to_populate; offset++) { 
                pkt_res_map[(pkt_res[UNKNOWN_L2UC_PKT] << shift_bits) + \
                                                      offset] = 1 << shift_bits;
            }
            /* Reset pri_cnf map */
            pri_cnf_map = &mode_attr->compressed_attr_selectors_v.\
                                             compressed_pri_cnf_attr_map_v[0];
            index_max = soc_mem_index_max(unit, ING_SVM_PRI_CNG_MAPm);
            for (map_index = 0; map_index <= index_max; map_index++) {
               pri_cnf_map[map_index] = 0;
            }
            /* set pri_cnf map for  16 policers */
            for (offset = 0; offset < 16; offset++) {
                for (map_index = 0; map_index < 16; map_index++) {
                    pri_cnf_map[offset << 4 | map_index] = map_index;
                }
            }
         


            /* Reset all Offset table fields */
            offset_map = &mode_attr->compressed_attr_selectors_v.offset_map[0];
            for (map_index = 0; map_index < BCM_SVC_METER_MAP_SIZE_256; 
                                                                map_index++) {
                offset_map[map_index].offset = 0;
                offset_map[map_index].meter_enable = 0;
            }
             /* Set DLF policer indexes considering INT_PRI bits don't care */
            for(map_index = 0; map_index < 16; map_index++) {
                offset_map[(map_index << 1) | 1].offset = 0;
                offset_map[(map_index << 1) | 1].meter_enable = 1;
            }
            /* Set Int pri policer indexes considering DLF=0 */
            for(map_index = 0; map_index < 16; map_index++) {
                offset_map[map_index << 1].offset = map_index + 1;
                offset_map[map_index << 1].meter_enable = 1;
            }
            break;
        }
         /* *************************************************************** */
         /* A dedicated policer for unknown unicast, known unicast,         */
         /* multicast,broadcast and N internal priority policers for traffic*/
         /* (not already policed)                                           */
         /* 1) UNKNOWN_L3UC_PKT                                             */
         /* 2) KNOWN_L2UC_PKT | KNOWN_L3UC_PKT                              */
         /* 3) KNOWN_L2MC_PKT|UNKNOWN_L2MC_PKT                              */
         /* 4) L2BC_PKT                                                     */
         /* 5..20) INT_PRI bits: 4bits                                      */
         /* *************************************************************** */
        case bcmPolicerGroupModeTypedIntPri:
        {
            *npolicers = 20;
            mode_attr->mode_type_v = compressed_mode; 
            pkt_attr = &mode_attr->compressed_attr_selectors_v.pkt_attr_bits_v;
            pkt_attr->pkt_resolution = 3;
            pkt_attr->int_pri = 4;
            /* Reset pkt_resolution map */
            pkt_res_map =  &mode_attr->compressed_attr_selectors_v.\
                                             compressed_pkt_res_attr_map_v[0];
            index_max = soc_mem_index_max(unit, ING_SVM_PKT_RES_MAPm);
            for (map_index = 0; map_index <= index_max; map_index++) {
                pkt_res_map[map_index] = 0;
            }
            /* set pkt_resolution map for DLF to 1. In the pkt_res_map first
               2 bits correspond to SVP  and Drop and hence are don't care
               bits. So we need to set our meter value at 4 locations*/
            no_entries_to_populate = 4;
            shift_bits = 2;
            if (SOC_IS_KATANA2(unit)) {
                /* In kt2 4 bits correspond to SVP(3)+drop(1). So we need to set
                   our meter value at 16 locations */
                no_entries_to_populate = 16;
                shift_bits = 4;
            }
            for (offset = 0; offset < no_entries_to_populate; offset++) { 
                pkt_res_map[(pkt_res[UNKNOWN_L3UC_PKT] << shift_bits) + offset]
                                                              = 1 << shift_bits;
                pkt_res_map[(pkt_res[KNOWN_L2UC_PKT] << shift_bits) + offset]
                                                              = 2 << shift_bits;
                pkt_res_map[(pkt_res[KNOWN_L3UC_PKT] << shift_bits) + offset] 
                                                              = 2 << shift_bits;
                pkt_res_map[(pkt_res[KNOWN_L2MC_PKT] << shift_bits) + offset] 
                                                              = 3 << shift_bits;
                pkt_res_map[(pkt_res[UNKNOWN_L2MC_PKT] << shift_bits) + offset]
                                                              = 3 << shift_bits;
                pkt_res_map[(pkt_res[L2BC_PKT] << shift_bits) + offset] 
                                                              = 4 << shift_bits;
            }
            /* Reset pri_cng map */
            pri_cnf_map = &mode_attr->compressed_attr_selectors_v.\
                                             compressed_pri_cnf_attr_map_v[0];
            index_max = soc_mem_index_max(unit, ING_SVM_PRI_CNG_MAPm);
            for (map_index = 0; map_index <= index_max; map_index++) {
                pri_cnf_map[map_index] = 0;
            }
            /* set pri_cnf map for  16 policers */
            for (map_index = 0; map_index < 16; map_index++) {
                pri_cnf_map[map_index] = map_index;
            }

            /* Reset all Offset table fields */
            offset_map = &mode_attr->compressed_attr_selectors_v.offset_map[0];
            for (map_index = 0; map_index < BCM_SVC_METER_MAP_SIZE_256; 
                                                                map_index++) {
                offset_map[map_index].offset = 0;
                offset_map[map_index].meter_enable = 0;
            }
            /* Set Int pri policer indexes considering Type fields = 0 */
            for(map_index = 0; map_index < 16; map_index++) {
                for(type = 0; type < 8; type++) {
                   offset_map[type + (8 * map_index)].offset = map_index + 4;
                   offset_map[type + (8 * map_index)].meter_enable = 1;
               }
            }
             /* Set Type based indexes considering INT_PRI bits don't care */
            for(type = 0; type < 4; type++) {
                for(map_index = 0; map_index < 16; map_index++) {
                    offset_map[(map_index << 3) | (type + 1)].offset = type;
                    offset_map[(map_index << 3) | (type + 1)].meter_enable = 1;
                }
            }
            break;
        }
         /* ************************************************************* */
         /* N+2 policers where the base policers is used for control, the  */
         /* next one for dlf and next N are used per Cos                  */
         /* 1) CONTROL_PKT|BPDU_PKT                                       */
         /* 2) L2_DLF                                                     */
         /* 3..18) INT_PRI bits: 4bits                                    */
         /* ************************************************************* */
        case bcmPolicerGroupModeDlfIntPriWithControl:
        {
            *npolicers = 18;
            mode_attr->mode_type_v = compressed_mode; 
            pkt_attr = &mode_attr->compressed_attr_selectors_v.pkt_attr_bits_v;
            pkt_attr->pkt_resolution = 2;
            pkt_attr->int_pri = 4;
            /* Reset pkt_resolution map */
            pkt_res_map =  &mode_attr->compressed_attr_selectors_v.\
                                             compressed_pkt_res_attr_map_v[0];
            index_max = soc_mem_index_max(unit, ING_SVM_PKT_RES_MAPm);
            for (map_index = 0; map_index <= index_max; map_index++) {
                pkt_res_map[map_index] = 0;
            }
            /* set pkt_resolution map for DLF to 1. In the pkt_res_map first
               2 bits correspond to SVP  and Drop and hence are don't care
               bits. So we need to set our meter value at 4 locations*/
            no_entries_to_populate = 4;
            shift_bits = 2;
            if (SOC_IS_KATANA2(unit)) {
                /* In kt2 4 bits correspond to SVP(3)+drop(1). So we need to set
                   our meter value at 16 locations */
                no_entries_to_populate = 16;
                shift_bits = 4;
            }
            /* set pkt_resolution map */
            for (offset = 0; offset < no_entries_to_populate; offset++) { 
                pkt_res_map[(pkt_res[CONTROL_PKT] << shift_bits) + offset] 
                                                           = 1 << shift_bits;
                pkt_res_map[(pkt_res[BPDU_PKT] << shift_bits) + offset] 
                                                           = 1 << shift_bits;
                pkt_res_map[(pkt_res[UNKNOWN_L2UC_PKT] << shift_bits) + offset] 
                                                           = 2 << shift_bits;
            }
            /* Reset pri_cng map */
            pri_cnf_map = &mode_attr->compressed_attr_selectors_v.\
                                             compressed_pri_cnf_attr_map_v[0];
            index_max = soc_mem_index_max(unit, ING_SVM_PRI_CNG_MAPm);
            for (map_index = 0; map_index <= index_max; map_index++) {
                pri_cnf_map[map_index] = 0;
            }
            /* set pri_cnf map for 16 policers */
            for (map_index = 0; map_index < 16; map_index++) {
                pri_cnf_map[map_index] = map_index;
            }

            /* Reset all Offset table fields */
            offset_map = &mode_attr->compressed_attr_selectors_v.offset_map[0];
            for (map_index = 0; map_index < BCM_SVC_METER_MAP_SIZE_256; 
                                                                map_index++) {
                offset_map[map_index].offset = 0;
                offset_map[map_index].meter_enable = 0;
            }
            /* Set Int pri policer indexes considering Type fields = 0 */
            for(map_index = 0; map_index < 16; map_index++) {
               for(type = 0; type < 4; type++) {
                   offset_map[type + (4 * map_index)].offset = map_index + 2;
                   offset_map[type + (4 * map_index)].meter_enable = 1;
               }
            }
             /* Set Type based indexes considering INT_PRI bits don't care */
            for(type = 0; type < 2; type++) {
                for(map_index = 0; map_index < 16; map_index++) {
                    offset_map[(map_index << 2) | (type + 1)].offset = type;
                    offset_map[(map_index << 2) | (type + 1)].meter_enable = 1;
                }
            }
            break;
        }
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
        case bcmPolicerGroupModeTypedIntPriWithControl:
        {
            *npolicers = 21;
            mode_attr->mode_type_v = compressed_mode; 
            pkt_attr = &mode_attr->compressed_attr_selectors_v.pkt_attr_bits_v;
            pkt_attr->pkt_resolution = 3;
            pkt_attr->int_pri = 4;
            /* Reset pkt_resolution map */
            pkt_res_map =  &mode_attr->compressed_attr_selectors_v.\
                                             compressed_pkt_res_attr_map_v[0];
            index_max = soc_mem_index_max(unit, ING_SVM_PKT_RES_MAPm);
            for (map_index = 0; map_index <= index_max; map_index++) {
                pkt_res_map[map_index] = 0;
            }
            /* set pkt_resolution map for DLF to 1. In the pkt_res_map first
               2 bits correspond to SVP  and Drop and hence are don't care
               bits. So we need to set our meter value at 4 locations*/
            no_entries_to_populate = 4;
            shift_bits = 2;
            if (SOC_IS_KATANA2(unit)) {
                /* In kt2 4 bits correspond to SVP(3)+drop(1). So we need to set
                   our meter value at 16 locations */
                no_entries_to_populate = 16;
                shift_bits = 4;
            }
            /* set pkt_resolution map */
            for (offset = 0; offset < no_entries_to_populate; offset++) { 
                pkt_res_map[(pkt_res[CONTROL_PKT] << shift_bits) + offset] 
                                                             = 1 << shift_bits;
                pkt_res_map[(pkt_res[BPDU_PKT] << shift_bits) + offset] 
                                                             = 1 << shift_bits;
                pkt_res_map[(pkt_res[UNKNOWN_L3UC_PKT] << shift_bits)+ offset] 
                                                             = 2 << shift_bits;
                pkt_res_map[(pkt_res[KNOWN_L2UC_PKT] << shift_bits) + offset] 
                                                             = 3 << shift_bits;
                pkt_res_map[(pkt_res[KNOWN_L3UC_PKT] << shift_bits) + offset] 
                                                             = 3 << shift_bits;
                pkt_res_map[(pkt_res[KNOWN_L2MC_PKT] << shift_bits) + offset] 
                                                             = 4 << shift_bits;
                pkt_res_map[(pkt_res[UNKNOWN_L2MC_PKT] << shift_bits) + offset] 
                                                             = 4 << shift_bits;
                pkt_res_map[(pkt_res[L2BC_PKT] << shift_bits) + offset] 
                                                             = 5 << shift_bits;
            }
            /* Reset pri_cnf map */
            pri_cnf_map = &mode_attr->compressed_attr_selectors_v.\
                                             compressed_pri_cnf_attr_map_v[0];
            index_max = soc_mem_index_max(unit, ING_SVM_PRI_CNG_MAPm);
            for (map_index = 0; map_index <= index_max; map_index++) {
                pri_cnf_map[map_index] = 0;
            }
            /* set pri_cnf map for  16 policers */
            for (map_index = 0; map_index < 16; map_index++) {
                pri_cnf_map[map_index] = map_index;
            }

            /* Reset all Offset table fields */
            offset_map = &mode_attr->compressed_attr_selectors_v.offset_map[0];
            for (map_index = 0; map_index < BCM_SVC_METER_MAP_SIZE_256; 
                                                                map_index++) {
                offset_map[map_index].offset = 0;
                offset_map[map_index].meter_enable = 0;
            }
            /* Set Int pri policer indexes considering Type fields = 0 */
            for(map_index = 0; map_index < 16; map_index++) {
                for(type = 0; type < 8; type++) {
                    offset_map[type + (8 * map_index)].offset = map_index + 5;
                    offset_map[type + (8 * map_index)].meter_enable = 1;
                }
            }
             /* Set Type based indexes considering INT_PRI bits don't care */
            for(type = 0; type < 5; type++) {
                for(map_index = 0; map_index < 16; map_index++) {
                    offset_map[(map_index << 3) | (type + 1)].offset = type;
                    offset_map[(map_index << 3) | (type + 1)].meter_enable = 1;
                }
            }
            break;
        }

        default:
            POLICER_VERB(("%s : Invalid policer group mode\n", FUNCTION_NAME()));
            return BCM_E_PARAM;

    }
    return rv;
}

/*
 * Function:
 *      _bcm_esw_policer_group_create
 * Purpose:
 *      Create a group of policers based on the mode.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     mode                  - (IN) Policer group mode 
 *     policer_id            - (OUT) Base policer id
 *     skip_pool             - (IN) Pool to be skipped 
 *     npolicers             - (OUT) Number of policers 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_policer_group_create(int unit, bcm_policer_group_mode_t mode,
                                   int skip_pool,
                                   bcm_policer_t *policer_id,
                                   int *npolicers)
{
    int rv = BCM_E_NONE;
    uint32 direction = 0;
    int index = 0;
    bcm_policer_svc_meter_mode_t offset_mode = 0;
    uint8 pid_offset[BCM_POLICER_GLOBAL_METER_MAX_POOL] = {0};
    _global_meter_policer_control_t *policer_control = NULL;
    bcm_policer_svc_meter_attr_t *mode_attr = NULL;
    int offset_mask = 0;

    if (!soc_feature(unit, soc_feature_global_meter)) {
        return BCM_E_UNAVAIL;
    }

    mode_attr = sal_alloc(sizeof(bcm_policer_svc_meter_attr_t),"meter mode attr");
    if ( mode_attr == NULL) {
        POLICER_VERB(("%s : Failed to allocate memory for svc meter attr \n",\
                                                            FUNCTION_NAME()));
        return BCM_E_MEMORY;
    }
    sal_memset(mode_attr, 0, sizeof(bcm_policer_svc_meter_attr_t));

    direction = GLOBAL_METER_ALLOC_VERTICAL;

    rv = _bcm_esw_policer_group_set_mode_and_map(unit, mode, npolicers, 
                                                  &direction, mode_attr);
    if (!(BCM_SUCCESS(rv))) {
        sal_free(mode_attr);
        POLICER_VERB(("%s : Failed to set mode and map\n", FUNCTION_NAME()));
        return rv;
    } 

    GLOBAL_METER_LOCK(unit);

    if (((direction == GLOBAL_METER_ALLOC_VERTICAL) && (*npolicers > 1)) ||
        ((direction == GLOBAL_METER_ALLOC_HORIZONTAL))) {
        rv = _bcm_esw_policer_svc_meter_create_mode(unit, mode_attr, mode, 
                                         *npolicers, &offset_mode);
        if (BCM_FAILURE(rv) && (rv != BCM_E_EXISTS)) {
            GLOBAL_METER_UNLOCK(unit);
            sal_free(mode_attr);
            return rv;
        } else { 
             global_meter_offset_mode[unit][offset_mode].no_of_policers = 
                                                               *npolicers;
            /* set group mode and number of policers in offset table at
               offsets 254 and 255 - for use during warmboot */
             rv = _bcm_esw_policer_update_offset_table_policer_count(unit,
                                              offset_mode, mode, *npolicers);
             if (BCM_FAILURE(rv)) {
                 GLOBAL_METER_UNLOCK(unit);
                 sal_free(mode_attr);
                 POLICER_VERB(("%s : Unable to write to offset table  \n", \
                                                         FUNCTION_NAME()));
                 return rv;
             } 
        }
    }
    sal_free(mode_attr);
    rv = _global_meter_policer_id_alloc(unit, direction,
                            npolicers, policer_id, skip_pool,
                            &pid_offset[0]);
    if (!(BCM_SUCCESS(rv))) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_VERB(("%s : Failed to allocate policer\n", FUNCTION_NAME()));
        return rv;
    } 
    offset_mask = SOC_INFO(unit).global_meter_max_size_of_pool - 1;

    /* Allocate policer descriptor. */
    _GLOBAL_METER_XGS3_ALLOC(policer_control, 
             sizeof (_global_meter_policer_control_t), "Global meter policer");
    if (NULL == policer_control) {
        _bcm_global_meter_free_allocated_policer_on_error(unit, *npolicers,
                             &pid_offset[0], (*policer_id & offset_mask));
        GLOBAL_METER_UNLOCK(unit);
        POLICER_VERB(("%s : Unable to allocate memeory for policer control \n",\
                                                             FUNCTION_NAME()));
        return (BCM_E_MEMORY);
    }

    /* Add mode info to the policer index */
    *policer_id |= (offset_mode + 1) << BCM_POLICER_GLOBAL_METER_MODE_SHIFT;

    /* Set policer configuration */
    policer_control->direction = direction;   
    policer_control->pid = *policer_id;
    policer_control->no_of_policers =  *npolicers;

    if (direction == GLOBAL_METER_ALLOC_HORIZONTAL) {
        index = 0; 
        do {
            policer_control->offset[index] = pid_offset[index]; 
            index++;
        } while (index < (*npolicers));
        rv = _bcm_esw_global_meter_set_cascade_info_to_hw(unit, *npolicers,
                                             *policer_id, mode, &pid_offset[0]);
        if (!(BCM_SUCCESS(rv))) {
            /* free all the allocated policers */
            _bcm_global_meter_free_allocated_policer_on_error(unit, *npolicers,
                             &pid_offset[0], (*policer_id & offset_mask));
            /* De-allocate policer descriptor. */
            sal_free(policer_control);
            GLOBAL_METER_UNLOCK(unit);
            return rv;
        } 
    }
    if (mode == bcmPolicerGroupModeCascadeWithCoupling) {
        *npolicers = *npolicers / 2;
    }
    /* increment mode reference count for new policer  */
    if (offset_mode) {
        rv = bcm_policer_svc_meter_inc_mode_reference_count(unit, offset_mode);
        if (!(BCM_SUCCESS(rv))) {
            /* free all the allocated policers */
            _bcm_global_meter_free_allocated_policer_on_error(unit,*npolicers,
                             &pid_offset[0], (*policer_id & offset_mask));
            /* De-allocate policer descriptor. */
            sal_free(policer_control);
            GLOBAL_METER_UNLOCK(unit);
            return rv;
        }
    }
   /* Insert policer into policers hash. */
    _GLOBAL_METER_HASH_INSERT(global_meter_policer_bookkeep[unit],
                   policer_control, (*policer_id & _GLOBAL_METER_HASH_MASK));

    GLOBAL_METER_UNLOCK(unit);
    POLICER_VVERB(("%s : create policer with id %x\n", FUNCTION_NAME(),\
                                                           *policer_id));
    return rv; 
}

/*
 * Function:
 *      bcm_esw_policer_envelop_create
 * Purpose:
 *      Create a single policer in Envelop mode. The policer created is
 *      micro or a macro meter depending on the configuration.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     flags                 - (IN) Indicates micro/macro flow policer  
 *     macro_flow_policer_id - (IN) macro flow policer id
 *     policer_id            - (OUT) policer Id
 * Returns:
 *     BCM_E_XXX 
 */
int bcm_esw_policer_envelop_create(int unit, uint32 flags,
                                   bcm_policer_t macro_flow_policer_id, 
                                   bcm_policer_t *policer_id)
{
    int rv = BCM_E_NONE;
    int index = 0;
    int micro_flow_index = 0;
    int pool = 0;
    int numbers = 1;
    svm_macroflow_index_table_entry_t macro_flow_entry;
    int size_pool = 0, num_pools = 0;
    int pool_mask = 0, pool_offset = 0;
    size_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
    num_pools =  SOC_INFO(unit).global_meter_pools;
    pool_offset = _shr_popcount(size_pool - 1);
    pool_mask = (num_pools - 1) << pool_offset; 

    if (flags == BCM_POLICER_GLOBAL_METER_ENVELOP_MACRO_FLOW) {
        rv = _bcm_esw_policer_group_create(unit, bcmPolicerGroupModeSingle,
                    num_pools, policer_id, &numbers); 
        if (!(BCM_SUCCESS(rv))) {
            POLICER_ERR(("%s : Unable to create macro flow policer\n", \
                                                        FUNCTION_NAME()));
            return rv;
        } 
    } else if (flags == BCM_POLICER_GLOBAL_METER_ENVELOP_MICRO_FLOW) {
        /* validate macroflow policer id */
        rv = _bcm_esw_policer_validate(unit, &macro_flow_policer_id);
        if (!(BCM_SUCCESS(rv))) {
            POLICER_ERR(("%s : Invalid policer Id passed\n", \
                                                        FUNCTION_NAME()));
            return rv;
        } 
        rv = _bcm_esw_get_policer_table_index(unit, macro_flow_policer_id, 
                                                                   &index); 
        if (!(BCM_SUCCESS(rv))) {
            POLICER_ERR(("%s : Unable to Get policer index for macro flow \
                                policer \n", FUNCTION_NAME()));
            return rv;
        } 
        pool = (macro_flow_policer_id & pool_mask) >> pool_offset;
        rv = _bcm_esw_policer_group_create(unit, bcmPolicerGroupModeSingle,
                                                pool, policer_id, &numbers); 
        if (!(BCM_SUCCESS(rv))) {
            POLICER_ERR(("%s : Unable to create micro flow policer\n", \
                                                        FUNCTION_NAME()));
            return rv;
        } 
        rv = _bcm_esw_policer_increment_ref_count(unit, macro_flow_policer_id);
        if (!(BCM_SUCCESS(rv))) {
            POLICER_ERR(("%s : Unable to increment ref count for  micro flow \
                               policer\n", FUNCTION_NAME()));
            return rv;
        } 
        /*  get the HW index for the micro flow policer created */
        rv = _bcm_esw_get_policer_table_index(unit, *policer_id, 
                                                           &micro_flow_index); 
        if (!(BCM_SUCCESS(rv))) {
            POLICER_ERR(("%s : Unable to Get policer index for micro flow \
                                policer \n", FUNCTION_NAME()));
            return rv;
        } 
        /* Write the index of the macro-meter at "micro_flow_index" location 
           of SVM_MACROFLOW_INDEX_TABLE */  
        rv = READ_SVM_MACROFLOW_INDEX_TABLEm(unit, MEM_BLOCK_ANY, 
                                         micro_flow_index, &macro_flow_entry);
        if (!(BCM_SUCCESS(rv))) {
            POLICER_VERB(("%s : Unable to access macro flow table at the index \
                                provided\n", FUNCTION_NAME()));
            return rv;
        } 
        if (SOC_MEM_FIELD_VALID(unit, SVM_MACROFLOW_INDEX_TABLEm, 
                                                           MACROFLOW_INDEXf)) {
            soc_SVM_MACROFLOW_INDEX_TABLEm_field_set(unit, &macro_flow_entry, 
                                           MACROFLOW_INDEXf, (uint32 *)&index);
        }
        rv = WRITE_SVM_MACROFLOW_INDEX_TABLEm(unit, MEM_BLOCK_ANY, 
                                          micro_flow_index, &macro_flow_entry);
        if (!(BCM_SUCCESS(rv))) {
            POLICER_VERB(("%s : Unable to write to macro flow table at index \
                                provided\n", FUNCTION_NAME()));
            return rv;
        } 
    } else {
        POLICER_ERR(("%s : Invalid flag passed \n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    return rv;
}

/*
 * Function:
 *      bcm_esw_policer_group_create
 * Purpose:
 *      Create a group of policers based on the mode.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     mode                  - (IN) policer group mode
 *     policer_id            - (OUT)Base policer Id
 *     npolicers             - (OUT) Number of policers created 
 * Returns:
 *     BCM_E_XXX 
 */
int bcm_esw_policer_group_create(int unit, bcm_policer_group_mode_t mode,
                                   bcm_policer_t *policer_id, int *npolicers)
{
    int rv = BCM_E_NONE;
    int num_pools = 0;
    num_pools =  SOC_INFO(unit).global_meter_pools;
    rv = _bcm_esw_policer_group_create(unit, mode, num_pools, 
                                               policer_id, npolicers);
    if (BCM_FAILURE(rv)) {
        POLICER_ERR(("%s : Unable to create policer for mode \
                                          passed  \n", FUNCTION_NAME()));
        return (rv);
    }
    POLICER_VERB(("%s : Created policer group of %d with base policer %x \n",\
                       FUNCTION_NAME(), *npolicers, *policer_id));
    return rv;
}


/*
 * Function:
 *      bcm_esw_policer_envelop_group_create
 * Purpose:
 *      Create a group of policers based on the mode.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     flags                 - (IN) Indicates micro/macro flow policer  
 *     mode                  - (IN) policer group mode
 *     macro_flow_policer_id - (IN) macro flow policer id
 *     policer_id            - (OUT)Base policer Id
 *     npolicers             - (OUT) Number of policers created 
 * Returns:
 *     BCM_E_XXX 
 */
int bcm_esw_policer_envelop_group_create(int unit, uint32 flag, 
                                     bcm_policer_group_mode_t mode, 
                                     bcm_policer_t macro_flow_policer_id, 
                                     bcm_policer_t *policer_id, 
                                     int *npolicers)
{
    int rv = BCM_E_NONE;
    int index = 0;
    int micro_flow_index = 0;
    int pool = 0;
    int numbers = 1;
    int size_pool = 0, num_pools = 0;
    int pool_mask = 0, pool_offset = 0;
    svm_macroflow_index_table_entry_t   *buf;        
    svm_macroflow_index_table_entry_t   *entry;        
    int entry_mem_size;    /* Size of table entry. */
    int entry_index = 0; 
    int entry_index_max = 0; 

    size_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
    num_pools =  SOC_INFO(unit).global_meter_pools;
    pool_offset = _shr_popcount(size_pool - 1);
    pool_mask = (num_pools - 1) << pool_offset; 

    if (flag == BCM_POLICER_GLOBAL_METER_ENVELOP_MACRO_FLOW) {
        rv = _bcm_esw_policer_group_create(unit, bcmPolicerGroupModeSingle,
                    num_pools, policer_id, &numbers); 
        if (!(BCM_SUCCESS(rv))) {
            POLICER_ERR(("%s : Unable to create macro flow policer\n", \
                                                        FUNCTION_NAME()));
            return rv;
        } 
    } else if (flag == BCM_POLICER_GLOBAL_METER_ENVELOP_MICRO_FLOW) {

        if ((mode == bcmPolicerGroupModeCascade) ||
            (mode == bcmPolicerGroupModeCascadeWithCoupling)) {
            POLICER_ERR(("%s : Unable to create micro flow policers \
                           due to unsupported mode\n", FUNCTION_NAME()));
            return (BCM_E_PARAM);
        } 
        /* validate macroflow policer id */
        rv = _bcm_esw_policer_validate(unit, &macro_flow_policer_id);
        if (!(BCM_SUCCESS(rv))) {
            POLICER_ERR(("%s : Invalid policer Id passed\n", \
                                                        FUNCTION_NAME()));
            return rv;
        } 
        rv = _bcm_esw_get_policer_table_index(unit, macro_flow_policer_id, 
                                                                   &index); 
        if (BCM_FAILURE(rv)) {
            POLICER_ERR(("%s : Unable to Get policer index for macro flow \
                                policer \n", FUNCTION_NAME()));
            return rv;
        } 

        pool = (macro_flow_policer_id & pool_mask) >> pool_offset;

        rv = _bcm_esw_policer_group_create(unit, mode, pool, 
                                           policer_id, &numbers); 
        if (BCM_FAILURE(rv)) {
            POLICER_ERR(("%s : Unable to create micro flow policers\n", \
                                                        FUNCTION_NAME()));
            return rv;
        } 
        /*  get the HW index for the micro flow policer created */
        rv = _bcm_esw_get_policer_table_index(unit, *policer_id, 
                                                           &micro_flow_index); 
        if (BCM_FAILURE(rv)) {
            POLICER_ERR(("%s : Unable to Get policer index for micro flow \
                                policer \n", FUNCTION_NAME()));
            return rv;
        } 
        /* Write the index of the macro-meter at "micro_flow_index" location 
           of SVM_MACROFLOW_INDEX_TABLE */  

        entry_mem_size = sizeof(svm_macroflow_index_table_entry_t);
        entry_index_max = micro_flow_index + numbers - 1;
        /* Allocate buffer to store the DMAed table entries. */
        buf = soc_cm_salloc(unit, entry_mem_size * numbers,
                              "svm macro flow index table entry buffer");
        if (NULL == buf) {
            return (BCM_E_MEMORY);
        }
        /* Initialize the entry buffer. */
        sal_memset(buf, 0, sizeof(entry_mem_size) * numbers);

        /* Read the table entries into the buffer. */
        rv = soc_mem_read_range(unit, SVM_MACROFLOW_INDEX_TABLEm,
                                MEM_BLOCK_ALL, micro_flow_index,
                                entry_index_max, buf);
        if (BCM_FAILURE(rv)) {
            if (buf) {
                soc_cm_sfree(unit, buf);
            }
            return rv;
        }

        /* Iterate over the table entries. */
        for (entry_index = 0; entry_index < numbers; entry_index++) {
            entry = soc_mem_table_idx_to_pointer(unit, 
                                  SVM_MACROFLOW_INDEX_TABLEm,
                                  svm_macroflow_index_table_entry_t *,
                                  buf, entry_index);
            soc_mem_field_set(unit, SVM_MACROFLOW_INDEX_TABLEm, 
                           (void *)entry, MACROFLOW_INDEXf, (uint32 *)&index);
            rv = _bcm_esw_policer_increment_ref_count(unit, 
                                                      macro_flow_policer_id);
            if (BCM_FAILURE(rv)) {
                POLICER_ERR(("%s : Unable to increment ref count for micro flow \
                               policer\n", FUNCTION_NAME()));
                if (buf) {
                    soc_cm_sfree(unit, buf);
                }
                return rv;
            } 
        }
        if ((rv = soc_mem_write_range(unit, SVM_MACROFLOW_INDEX_TABLEm,
                                      MEM_BLOCK_ALL, micro_flow_index,
                                      entry_index_max, buf)) < 0) {
            if (BCM_FAILURE(rv)) {
                POLICER_VERB(("%s: Unable to write to macro flow table at index \
                                provided\n", FUNCTION_NAME()));
                if (buf) {
                    soc_cm_sfree(unit, buf);
                }
                return rv;
            } 
        }
        if (buf) {
            soc_cm_sfree(unit, buf);
        }
    } else {
        POLICER_ERR(("%s : Invalid flag passed \n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    *npolicers = numbers;
    return rv;
}

/*
 * Function:
 *      bcm_esw_policer_action_attach_get
 * Purpose:
 *     Get the action id associated with a given policer.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer_id            - (IN) Base policer Id
 *     action_id             - (OUT) action id 
 * Returns:
 *     BCM_E_XXX 
 */
int bcm_esw_policer_action_attach_get(int unit, bcm_policer_t policer_id,
                                uint32 *action_id)
{
    int rv = BCM_E_NONE;
    svm_meter_table_entry_t meter_entry;
    _global_meter_policer_control_t *policer_control = NULL;
    int policer_index = 0;
    if (!soc_feature(unit, soc_feature_global_meter)) {
        return BCM_E_UNAVAIL;
    }
    CHECK_GLOBAL_METER_INIT(unit);
    /* validate policer id */
    rv = _bcm_esw_policer_validate(unit, &policer_id);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }
    GLOBAL_METER_LOCK(unit);
    rv = _bcm_global_meter_policer_get(unit, policer_id, &policer_control);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to get policer control for the policer Id \
                                              passed  \n", FUNCTION_NAME()));
        return (rv);
    }
    /*  read meter table action_id*/
    _bcm_esw_get_policer_table_index(unit, policer_id, &policer_index); 
    rv = READ_SVM_METER_TABLEm(unit, MEM_BLOCK_ANY, policer_index, 
                                                            &meter_entry);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to read SVM_METER_TABLE entry \n", \
                                                        FUNCTION_NAME()));
        return (rv);
    }
    /* get action id */
    soc_SVM_METER_TABLEm_field_get(unit, &meter_entry, POLICY_TABLE_INDEXf,
                                                                 action_id);
    GLOBAL_METER_UNLOCK(unit);
    return rv; 
}

/*
 * Function:
 *      bcm_esw_policer_action_detach
 * Purpose:
 *     Disassociate a policer action from a given policer id and
 *     readjust reference count.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer_id            - (IN) Base policer Id
 *     action_id             - (IN) action id 
 * Returns:
 *     BCM_E_XXX 
 */
int bcm_esw_policer_action_detach(int unit, bcm_policer_t policer_id,
                                uint32 action_id)
{
    return _bcm_esw_policer_action_detach(unit, policer_id, action_id);
}

/*
 * Function:
 *      bcm_esw_policer_action_attach
 * Purpose:
 *     Associate a policer action with a given policer id and
 *     increment action usage reference count.
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer_id            - (IN) Base policer Id
 *     action_id             - (IN) action id 
 * Returns:
 *     BCM_E_XXX 
 */

int bcm_esw_policer_action_attach(int unit, bcm_policer_t policer_id,
                                uint32 action_id)
{
    int rv = BCM_E_NONE;
    svm_meter_table_entry_t meter_entry;
    _global_meter_policer_control_t *policer_control = NULL;
    int policer_index = 0;
    uint32 mode = 0, coupling = 0; 
    int index = 0;
    if (!soc_feature(unit, soc_feature_global_meter)) {
        return BCM_E_UNAVAIL;
    }
    CHECK_GLOBAL_METER_INIT(unit);
    /* validate policer id */
    rv = _bcm_esw_policer_validate(unit, &policer_id);
    if (BCM_FAILURE(rv)) {
        POLICER_ERR(("%s : Invalid Policer Id \n", FUNCTION_NAME()));
        return (rv);
    }
    /* validate action id */
    if (action_id > soc_mem_index_max(unit, SVM_POLICY_TABLEm)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Invalid Action Id \n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    } 

    GLOBAL_METER_LOCK(unit);
    rv = _bcm_global_meter_policer_get(unit, policer_id, &policer_control);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to get policer control for the policer Id \
                                              passed  \n", FUNCTION_NAME()));
        return (rv);
    }

    if (policer_control->action_id == action_id) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_VERB(("%s : Action Id passed is different from the one in \
         policer control-%x\n", FUNCTION_NAME(), policer_control->action_id));
        return (BCM_E_NONE);
    }
    /* check if action id is allocated */
    if (global_meter_action_bookkeep[unit][action_id].used !=1)
    {
        POLICER_ERR(("%s : Action Id is not created \n", FUNCTION_NAME()));
        return BCM_E_PARAM;
    }
    /*  read meter table and add action_id*/
    _bcm_esw_get_policer_table_index(unit, policer_id, &policer_index); 
    rv = READ_SVM_METER_TABLEm(unit, MEM_BLOCK_ANY, policer_index, 
                                                             &meter_entry);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to read SVM_METER_TABLE entry \n", \
                                                        FUNCTION_NAME()));
        return (rv);
    }
    soc_SVM_METER_TABLEm_field_set(unit, &meter_entry, POLICY_TABLE_INDEXf,
                                                                 &action_id);
    /* detach the existing action and decrement reference count */ 
    if (policer_control->action_id) {
        _bcm_esw_policer_action_detach(unit, policer_id, 
                                                policer_control->action_id); 
    }
    /* Write to HW*/
    rv = WRITE_SVM_METER_TABLEm(unit, MEM_BLOCK_ANY, policer_index, 
                                                                 &meter_entry);
    if (BCM_FAILURE(rv)) {
        GLOBAL_METER_UNLOCK(unit);
        POLICER_ERR(("%s : Unable to write SVM_METER_TABLE entry \n", \
                                                        FUNCTION_NAME()));
        return (rv);
    }
    /* in case of cascade with coupling we need to configure the second set
       as well */
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_MODEf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &meter_entry, METER_MODEf, &mode);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COUPLING_FLAGf)) {
        soc_SVM_METER_TABLEm_field_set(unit, &meter_entry, COUPLING_FLAGf,
                                                                &coupling);
    }
    if ((coupling == 1) && (mode == GLOBAL_METER_MODE_CASCADE)) {
        rv =_bcm_global_meter_get_coupled_cascade_policer_index(unit, 
                                     policer_id, policer_control, &index); 
        /* write to hW */
        rv = soc_mem_write(unit, SVM_METER_TABLEm, MEM_BLOCK_ANY, index, 
                                                            &meter_entry);
        if (BCM_FAILURE(rv)) {
            POLICER_ERR(("%s : Unable to write SVM_METER_TABLE entry \n", \
                                                        FUNCTION_NAME()));
        return (rv);
       }
    } 
   /* increment action usage reference count */
    global_meter_action_bookkeep[unit][action_id].reference_count++; 
    policer_control->action_id = action_id;
    GLOBAL_METER_UNLOCK(unit);
    return rv; 
}                      
/* END POLICER API's */

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      _bcm_esw_policer_config_from_meter_table
 * Purpose:
 *     For a given policer, read the configuraton info from meter table
 * Parameters:
 *     unit                  - (IN) unit number 
 *     policer               - (IN) Base policer Id
 *     policer_control       - (OUT) Policer control info 
 * Returns:
 *     BCM_E_XXX 
 */
int  _bcm_esw_policer_config_from_meter_table(int unit, bcm_policer_t policer, 
                           _global_meter_policer_control_t *policer_control) 
{
    svm_meter_table_entry_t data;
    int index = 0, numbers = 0;
    uint32 mode = 0, coupling_flag = 0, pool = 0;
    bcm_policer_svc_meter_mode_t meter_mode;
    uint32 pol_index = 0, offset_mode = 0; 
    uint32 end_of_chain = 0, first_end_of_chain = 0;
    int rv = BCM_E_NONE;
    int policer_index = 0;
    int size_pool = 0, num_pools = 0;
    int pool_mask = 0, pool_offset = 0;
    int offset_mask = 0; 
    size_pool =  SOC_INFO(unit).global_meter_max_size_of_pool;
    num_pools =  SOC_INFO(unit).global_meter_pools;
    offset_mask = size_pool - 1;
    pool_offset = _shr_popcount(offset_mask);
    pool_mask = (num_pools - 1) << pool_offset; 

   /* read meter table to get policer configuration */
    _bcm_esw_get_policer_table_index(unit, policer, &policer_index); 
    rv = soc_mem_read(unit, SVM_METER_TABLEm, MEM_BLOCK_ANY, 
                                                    policer_index, &data);
    BCM_IF_ERROR_RETURN(rv);
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, METER_MODEf)) {
        soc_SVM_METER_TABLEm_field_get(unit, &data, METER_MODEf, &mode);
    }
    if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, COUPLING_FLAGf)) {
       soc_SVM_METER_TABLEm_field_get(unit, &data, COUPLING_FLAGf,
                                                         &coupling_flag);
   }
   pool = (policer & pool_mask) >> pool_offset; 

   offset_mode = ((policer & BCM_POLICER_GLOBAL_METER_MODE_MASK) >> 
                             BCM_POLICER_GLOBAL_METER_MODE_SHIFT);
   if (offset_mode >= 1) {
       offset_mode = offset_mode - 1;
       if ((offset_mode) &&  
          (global_meter_offset_mode[unit][offset_mode].no_of_policers == 0)) {
           return rv;
       }
   } 

   /* Allocate policer descriptor. */
    _GLOBAL_METER_XGS3_ALLOC(policer_control, 
       sizeof (_global_meter_policer_control_t), "Global meter policer");
   if (NULL == policer_control) {
       GLOBAL_METER_UNLOCK(unit);
        POLICER_VVERB(("%s : Unable to allocate memory for Policer control data \
                                               structure\n", FUNCTION_NAME()));
       return (BCM_E_MEMORY);
   }

   if (SOC_MEM_FIELD_VALID(unit, SVM_METER_TABLEm, POLICY_TABLE_INDEXf)) {
       soc_SVM_METER_TABLEm_field_get(unit,
                      &data, POLICY_TABLE_INDEXf, &policer_control->action_id);
        /* increment action usage reference count */
       if (policer_control->action_id) {
              global_meter_action_bookkeep[unit]\
                       [policer_control->action_id].reference_count++; 
       }
   }

   _bcm_esw_get_policer_table_index(unit, policer, &policer_index); 
    if(mode == GLOBAL_METER_MODE_CASCADE) {
       policer_control->direction = GLOBAL_METER_ALLOC_HORIZONTAL;   
       policer_control->pid = policer;
       policer_control->offset[numbers] = 0;
       numbers++;
       pol_index = policer_index;
       for (index = pool + 1; index < num_pools; index++) {
           policer_index = pol_index + (index << _shr_popcount(size_pool - 1)); 
           rv = soc_mem_read(unit, SVM_METER_TABLEm, MEM_BLOCK_ANY, 
                                                   policer_index, &data);
           if (!BCM_SUCCESS(rv)) {
               sal_free(policer_control);
               POLICER_VVERB(("%s : Unable to read SVM meter table at \
                         index %x\n", FUNCTION_NAME(), policer_index));
               return rv;
           }
           soc_SVM_METER_TABLEm_field_get(unit, &data, 
                                        END_OF_CHAINf, &end_of_chain);
           soc_SVM_METER_TABLEm_field_get(unit, &data, 
                                              METER_MODEf, &meter_mode);
           if ((meter_mode == GLOBAL_METER_MODE_CASCADE) && 
                                                   (end_of_chain != 1)) {
               policer_control->offset[numbers] = index - pool; 
           }
           if (!(coupling_flag) && 
                  (meter_mode == GLOBAL_METER_MODE_CASCADE) &&
                  (end_of_chain == 1)) {
               policer_control->offset[numbers] = index - pool; 
               index = num_pools;
           }
           if ((coupling_flag) && 
                           (meter_mode == GLOBAL_METER_MODE_CASCADE) &&
                           (end_of_chain) && (first_end_of_chain)) {
               policer_control->offset[numbers] = index - pool; 
               index = num_pools;
           }
           if ((coupling_flag) && 
                       (meter_mode == GLOBAL_METER_MODE_CASCADE) &&
                       (end_of_chain) && (first_end_of_chain == 0)) {
               policer_control->offset[numbers] = index - pool; 
               first_end_of_chain = 1;
           }
           numbers++;
       } /* end of for */ 
       policer_control->no_of_policers = numbers;
    } else if (offset_mode >= 1) {
       policer_control->direction = GLOBAL_METER_ALLOC_VERTICAL;   
       policer_control->pid = policer;
       /* add code to get the number of policers for this mode */
       policer_control->no_of_policers = 
       global_meter_offset_mode[unit][offset_mode].no_of_policers;
    } else {
       policer_control->direction = GLOBAL_METER_ALLOC_VERTICAL;   
       policer_control->pid = policer;
       policer_control->no_of_policers = 1;
    }
    rv =  _global_meter_reserve_policer_id(unit, policer_control->direction, 
                                            policer_control->no_of_policers, 
                                        policer, &policer_control->offset[0]); 
    if (!BCM_SUCCESS(rv)) {
         /* De-allocate policer descriptor. */
        sal_free(policer_control);
        POLICER_VVERB(("%s : Unable to allocate policers with base id %x\n", \
                                                 FUNCTION_NAME(), policer));
        return BCM_E_INTERNAL;
    }
    /* Insert policer into policers hash. */
    _GLOBAL_METER_HASH_INSERT(global_meter_policer_bookkeep[unit],
                 policer_control,(policer & _GLOBAL_METER_HASH_MASK));
    /* increment mode reference count for new policer  */
    if (offset_mode) {
        rv = bcm_policer_svc_meter_inc_mode_reference_count(unit, offset_mode);
    }
    return rv;
}
    
/*
 * Function:
 *      _bcm_policer_config_reinit_from_table
 * Purpose:
 *     Get policer configuration for those policers that are
 *     configured in given HW table on warm boot and rebuild SDK 
 *     internal data strutcure 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     table                 - (IN) table id
 *     data                  - (IN) table entry 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_policer_config_reinit_from_table(int unit, soc_mem_t table,void *data)
{
    bcm_policer_t policer = 0;
    int index = 0, max_index = 0;
    _global_meter_policer_control_t *policer_control = NULL;
    int rv = BCM_E_NONE;
    max_index = soc_mem_index_max(unit, table);

    for ( index = 0; index <= max_index; index++) {
        policer = 0;
        rv =_bcm_esw_get_policer_from_table(unit, table, index, 
                                                         data, &policer, 0); 
        if (BCM_FAILURE(rv)) {
            POLICER_VVERB(("%s : Unable to read policer entry from table %d at\
                             %d \n", FUNCTION_NAME(), table, index));
            return (rv);
        }
        /* get policer config */
        if ((policer & BCM_POLICER_GLOBAL_METER_INDEX_MASK) > 0) {
            rv = _bcm_global_meter_base_policer_get(unit, policer, 
                                                           &policer_control);
            if (BCM_FAILURE(rv)) {
                rv = _bcm_esw_policer_config_from_meter_table(unit, policer, 
                                                            policer_control);
            }
            if (table == SVM_MACROFLOW_INDEX_TABLEm) {
                rv = _bcm_esw_policer_increment_ref_count(unit, policer);    
                if (BCM_FAILURE(rv)) {
                    POLICER_VVERB(("%s : Unable to increment the policer usage \
                       ref counter for policer %x\n", FUNCTION_NAME(), policer));
                    return (rv);
                }
            }
        }
    } /* end of for*/
    return (rv);
}

/*
 * Function:
 *      _bcm_policer_config_reinit
 * Purpose:
 *     go through all the table entries that support service meter 
 *     and get policer configuration for those policers that are
 *     configured in HW to rebuild SDK internal data strutcure on
 *     warm boot. 
 * Parameters:
 *     unit                  - (IN) unit number 
 * Returns:
 *     BCM_E_XXX 
 */

int _bcm_policer_config_reinit(int unit) 
{
    int rv = BCM_E_NONE;
    vlan_tab_entry_t vlan_entry;
    vfi_entry_t vfi_entry;
    source_vp_entry_t  svp_entry;
    vfp_policy_table_entry_t vfp_entry;
    vlan_xlate_entry_t vlan_xlate_entry;
#ifdef BCM_TRIUMPH3_SUPPORT
    vlan_xlate_extd_entry_t vlan_xlate_xentry;
#endif
    port_tab_entry_t port_entry;
    svm_macroflow_index_table_entry_t macro_flow_entry;

    rv = _bcm_policer_config_reinit_from_table(unit, PORT_TABm, &port_entry);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to re-init policer config from Port \
                                               table  \n", FUNCTION_NAME()));
        return (rv);
    }
    rv = _bcm_policer_config_reinit_from_table(unit, VLAN_TABm, &vlan_entry);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to re-init policer config from vlan \
                                               table  \n", FUNCTION_NAME()));
        return (rv);
    }
    if (SOC_IS_KATANAX(unit)) {
        rv = _bcm_policer_config_reinit_from_table(unit, VLAN_XLATEm,
                                               &vlan_xlate_entry);
#ifdef BCM_TRIUMPH3_SUPPORT
    } else if (SOC_IS_TRIUMPH3(unit)) {
        rv = _bcm_policer_config_reinit_from_table(unit, VLAN_XLATE_EXTDm,
                                               &vlan_xlate_xentry);
#endif
    }
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to re-init policer config from vlan \
                                          xlate table  \n", FUNCTION_NAME()));
        return (rv);
    }
    rv = _bcm_policer_config_reinit_from_table(unit, SOURCE_VPm, &svp_entry);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to re-init policer config from svp \
                                               table  \n", FUNCTION_NAME()));
        return (rv);
    }
    rv = _bcm_policer_config_reinit_from_table(unit, VFIm, &vfi_entry);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to re-init policer config from vfi \
                                               table  \n", FUNCTION_NAME()));
        return (rv);
    }
    rv = _bcm_policer_config_reinit_from_table(unit, VFP_POLICY_TABLEm,
                                                            &vfp_entry);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to re-init policer config from VFP \
                                          policy table  \n", FUNCTION_NAME()));
        return (rv);
    }
    rv = _bcm_policer_config_reinit_from_table(unit, SVM_MACROFLOW_INDEX_TABLEm,
                                                          &macro_flow_entry);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to re-init policer config from Macro flow \
                                          index table  \n", FUNCTION_NAME()));
        return (rv);
    }
    return rv;
}
   
/*
 * Function:
 *      _bcm_policer_ref_count_reinit_from_table
 * Purpose:
 *      Increment the policer usage reference count by reading 
 *      all the entires of given HW table 
 * Parameters:
 *     unit                  - (IN) unit number 
 *     table                 - (IN) table id
 *     data                  - (IN) table entry 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_policer_ref_count_reinit_from_table(int unit, soc_mem_t table, 
                                                              void *data)
{
    bcm_policer_t policer;
    uint32 index = 0, max_index = 0;
    int rv = BCM_E_NONE;

    max_index = soc_mem_index_max(unit, table);
    
    for (index = 0; index <= max_index; index++) {
        policer = 0;
        rv =_bcm_esw_get_policer_from_table(unit, table, index, 
                                                        data, &policer, 0); 
        if (BCM_FAILURE(rv)) {
            POLICER_VVERB(("%s : Unable to get policer usage info from table \
                          %d at index %d \n", FUNCTION_NAME(), table, index));
            return (rv);
        }
        if (policer > 0) {
            rv = _bcm_esw_policer_increment_ref_count(unit, policer);    
            if (BCM_FAILURE(rv)) {
                POLICER_VVERB(("%s : Unable to increment the policer usage \
                    ref counter for policer %x\n", FUNCTION_NAME(), policer));
                return (rv);
            }
        }
    }
    return (rv);
}

/*
 * Function:
 *      _bcm_policer_ref_count_reinit
 * Purpose:
 *      Increment the policer usage reference count by reading all
 *      the HW tables that support service meter policer configuration 
 *      on warm boot
 * Parameters:
 *     unit                  - (IN) unit number 
 * Returns:
 *     BCM_E_XXX 
 */

int _bcm_policer_ref_count_reinit(int unit) 
{
    int rv = BCM_E_NONE;
    vlan_tab_entry_t vlan_entry;
    vfi_entry_t vfi_entry;
    source_vp_entry_t  svp_entry;
    vfp_policy_table_entry_t vfp_entry;
    vlan_xlate_entry_t vlan_xlate_entry;
#ifdef BCM_TRIUMPH3_SUPPORT
    vlan_xlate_extd_entry_t vlan_xlate_xentry;
#endif
    port_tab_entry_t port_entry;

    rv = _bcm_policer_ref_count_reinit_from_table(unit, PORT_TABm, &port_entry);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to get policer usage info from port \
                                           table \n", FUNCTION_NAME()));
        return (rv);
    }
    rv = _bcm_policer_ref_count_reinit_from_table(unit, VLAN_TABm, &vlan_entry);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to get policer usage info from Vlan \
                                           table \n", FUNCTION_NAME()));
        return (rv);
    }
    if (SOC_IS_KATANAX(unit)) {
        rv = _bcm_policer_ref_count_reinit_from_table(unit, VLAN_XLATEm, 
                                                           &vlan_xlate_entry);
#ifdef BCM_TRIUMPH3_SUPPORT
    } else if(SOC_IS_TRIUMPH3(unit)) {
        rv = _bcm_policer_ref_count_reinit_from_table(unit, VLAN_XLATE_EXTDm, 
                                                          &vlan_xlate_xentry);
#endif
    }
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to get policer usage info from Vlan \
                                     xlate table \n", FUNCTION_NAME()));
        return (rv);
    }
    rv = _bcm_policer_ref_count_reinit_from_table(unit, SOURCE_VPm, &svp_entry);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to get policer usage info from SVP \
                                           table \n", FUNCTION_NAME()));
        return (rv);
    }
    rv = _bcm_policer_ref_count_reinit_from_table(unit, VFIm, &vfi_entry);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to get policer usage info from VFI \
                                           table \n", FUNCTION_NAME()));
        return (rv);
    }
    rv = _bcm_policer_ref_count_reinit_from_table(unit, VFP_POLICY_TABLEm,
                                                                &vfp_entry);
    if (BCM_FAILURE(rv)) {
        POLICER_VVERB(("%s : Unable to get policer usage info from VFP \
                                     policy table \n", FUNCTION_NAME()));
        return (rv);
    }
    return rv;
}

/*
 * Function:
 *      _bcm_esw_global_meter_uncompressed_offset_mode_reinit
 * Purpose:
 *      Recover un-coompessed offset mode information from HW and re-configure 
 *      SDK data structures on warm boot
 * Parameters:
 *     unit                  - (IN) unit number 
 *     mode                  - (IN) meter offset mode
 *     selector_count        - (IN) Number of selectors 
 *     selector_for_bit_x_en_field_value - (IN) selector field enable 
 *     selector_for_bit_x_field_value    - (IN) selector field
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_global_meter_uncompressed_offset_mode_reinit(
                                 int unit, 
                                 int mode,
                                 uint32 selector_count, 
                                 uint32  selector_for_bit_x_en_field_value[8],
                                 uint32  selector_for_bit_x_field_value[8])
{
    uint32  uncompressed_attr_bits_selector = 0;
    uint32  index = 0;
    int rv = BCM_E_NONE;
    pkt_attr_bit_pos_t      *pkt_attr_bit_pos = NULL;

    if (SOC_IS_KATANA2(unit)) {
        pkt_attr_bit_pos = &kt2_pkt_attr_bit_pos[0];
    } else {
        pkt_attr_bit_pos = &kt_pkt_attr_bit_pos[0];
    }
    global_meter_offset_mode[unit][mode].used = 1;
    global_meter_offset_mode[unit][mode].reference_count = 0;
    global_meter_offset_mode[unit][mode].meter_attr.mode_type_v = 
                                                  uncompressed_mode;
    for (uncompressed_attr_bits_selector = 0, index = 0; index < 8; index++)
    {
        if (selector_for_bit_x_en_field_value[index] != 0)
        {
            if ( selector_for_bit_x_field_value[index] == 
                               pkt_attr_bit_pos[pkt_attr_ip_pkt].start_bit) 
            {
                 uncompressed_attr_bits_selector |= 
                       BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS;
                 continue;
            }
            if ( selector_for_bit_x_field_value[index] ==  
                               pkt_attr_bit_pos[pkt_attr_drop].start_bit) 
            {
                 uncompressed_attr_bits_selector |= 
                        BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_DROP_ATTR_BITS;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] ==  
                        pkt_attr_bit_pos[pkt_attr_svp_group].start_bit)  &&
               ( selector_for_bit_x_field_value[index] <= 
                        pkt_attr_bit_pos[pkt_attr_svp_group].end_bit))  
            {
                 uncompressed_attr_bits_selector |= 
                    BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >= 
                        pkt_attr_bit_pos[pkt_attr_pkt_resolution].start_bit) && 
               ( selector_for_bit_x_field_value[index] <= 
                        pkt_attr_bit_pos[pkt_attr_pkt_resolution].end_bit))  
                                
            {
                 uncompressed_attr_bits_selector |= 
                               BCM_POLICER_SVC_METER_PKT_REOLUTION_ATTR_SIZE;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=
                               pkt_attr_bit_pos[pkt_attr_tos].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                               pkt_attr_bit_pos[pkt_attr_tos].end_bit))  
            {
                 uncompressed_attr_bits_selector |= 
                                        BCM_POLICER_SVC_METER_TOS_ATTR_SIZE;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=  
                             pkt_attr_bit_pos[pkt_attr_ing_port].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                             pkt_attr_bit_pos[pkt_attr_ing_port].end_bit))  
            {
                 uncompressed_attr_bits_selector |= 
                                   BCM_POLICER_SVC_METER_ING_PORT_ATTR_SIZE;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >= 
                          pkt_attr_bit_pos[pkt_attr_inner_dot1p].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                          pkt_attr_bit_pos[pkt_attr_inner_dot1p].end_bit))  
            {
                 uncompressed_attr_bits_selector |= 
                                BCM_POLICER_SVC_METER_INNER_DOT1P_ATTR_SIZE;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=   
                           pkt_attr_bit_pos[pkt_attr_outer_dot1p].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                           pkt_attr_bit_pos[pkt_attr_outer_dot1p].end_bit)) 
            {
                 uncompressed_attr_bits_selector |= 
                                BCM_POLICER_SVC_METER_OUTER_DOT1P_ATTR_SIZE;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=  
                           pkt_attr_bit_pos[pkt_attr_vlan_format].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                           pkt_attr_bit_pos[pkt_attr_vlan_format].end_bit))  
            {
                 uncompressed_attr_bits_selector |= 
                                BCM_POLICER_SVC_METER_VLAN_FORMAT_ATTR_SIZE;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=  
                               pkt_attr_bit_pos[pkt_attr_int_pri].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                               pkt_attr_bit_pos[pkt_attr_int_pri].end_bit))  
            {
                 uncompressed_attr_bits_selector |= 
                                   BCM_POLICER_SVC_METER_INT_PRI_ATTR_SIZE;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=  
                               pkt_attr_bit_pos[pkt_attr_cng].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                               pkt_attr_bit_pos[pkt_attr_cng].end_bit))  
            {
                 uncompressed_attr_bits_selector |= 
                                          BCM_POLICER_SVC_METER_CNG_ATTR_SIZE;
                 continue;
            }
        }
    }   
    global_meter_offset_mode[unit][mode].meter_attr.\
          uncompressed_attr_selectors_v.uncompressed_attr_bits_selector = 
                                              uncompressed_attr_bits_selector;
    return rv; 
}

/*
 * Function:
 *      _bcm_esw_global_meter_compressed_offset_mode_reinit
 * Purpose:
 *      Recover Compessed offset mode information from HW and re-configure SDK
 *      data structures on warm boot
 *
 * Parameters:
 *     unit                  - (IN) unit number 
 *     mode                  - (IN) meter offset mode
 *     selector_count        - (IN) Number of selectors 
 *     selector_for_bit_x_en_field_value - (IN) selector field enable 
 *     selector_for_bit_x_field_value    - (IN) selector field
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_global_meter_compressed_offset_mode_reinit(
                                 int unit, 
                                 int mode,
                                 uint32 selector_count, 
                                 uint32  selector_for_bit_x_en_field_value[8],
                                 uint32  selector_for_bit_x_field_value[8])
{
    uint32  index = 0;
    int rv = BCM_E_NONE;
    compressed_attr_selectors_t *comp_pkt_attr_sel;
    pkt_attr_bits_t *pkt_attr_bits;
    pkt_attr_bit_pos_t      *pkt_attr_bit_pos = NULL;
    int                     index_max = 0;
    if (SOC_IS_KATANA2(unit)) {
        pkt_attr_bit_pos = &kt2_pkt_attr_bit_pos[0];
    } else {
        pkt_attr_bit_pos = &kt_pkt_attr_bit_pos[0];
    }
    global_meter_offset_mode[unit][mode].used = 1;
    global_meter_offset_mode[unit][mode].reference_count = 0;
    global_meter_offset_mode[unit][mode].meter_attr.mode_type_v = 
                                                     compressed_mode;
    pkt_attr_bits = &(global_meter_offset_mode[unit][mode].meter_attr.\
                       compressed_attr_selectors_v.pkt_attr_bits_v);
    comp_pkt_attr_sel = &(global_meter_offset_mode[unit][mode].meter_attr.\
                                               compressed_attr_selectors_v);
    for (index = 0; index < 8; index++)
    {
        if (selector_for_bit_x_en_field_value[index] != 0)
        {
            if ( selector_for_bit_x_field_value[index] == 
                               pkt_attr_bit_pos[pkt_attr_ip_pkt].start_bit) 
            {
                 pkt_attr_bits->ip_pkt = 1;
                 continue;
            }
            if ( selector_for_bit_x_field_value[index] ==  
                               pkt_attr_bit_pos[pkt_attr_drop].start_bit) 
            {
                 pkt_attr_bits->drop = 1;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=  
                        pkt_attr_bit_pos[pkt_attr_svp_group].start_bit) && 
               ( selector_for_bit_x_field_value[index] <= 
                        pkt_attr_bit_pos[pkt_attr_svp_group].end_bit))  
            {
                 pkt_attr_bits->svp_type++ ;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >= 
                        pkt_attr_bit_pos[pkt_attr_pkt_resolution].start_bit) && 
               ( selector_for_bit_x_field_value[index] <= 
                        pkt_attr_bit_pos[pkt_attr_pkt_resolution].end_bit))  
                                
            {
                 pkt_attr_bits->pkt_resolution++;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=
                               pkt_attr_bit_pos[pkt_attr_tos].start_bit) && 
               ( selector_for_bit_x_field_value[index] <= 
                               pkt_attr_bit_pos[pkt_attr_tos].end_bit))  
            {
                 pkt_attr_bits->tos++;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=  
                             pkt_attr_bit_pos[pkt_attr_ing_port].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                             pkt_attr_bit_pos[pkt_attr_ing_port].end_bit))  
            {
                 pkt_attr_bits->ing_port++;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >= 
                          pkt_attr_bit_pos[pkt_attr_inner_dot1p].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                          pkt_attr_bit_pos[pkt_attr_inner_dot1p].end_bit))  
            {
                 pkt_attr_bits->inner_dot1p++;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=   
                           pkt_attr_bit_pos[pkt_attr_outer_dot1p].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                           pkt_attr_bit_pos[pkt_attr_outer_dot1p].end_bit)) 
            {
                 pkt_attr_bits->outer_dot1p++;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=  
                           pkt_attr_bit_pos[pkt_attr_vlan_format].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                           pkt_attr_bit_pos[pkt_attr_vlan_format].end_bit))  
            {
                 pkt_attr_bits->vlan_format++;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=  
                               pkt_attr_bit_pos[pkt_attr_int_pri].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                               pkt_attr_bit_pos[pkt_attr_int_pri].end_bit))  
            {
                 pkt_attr_bits->int_pri++;
                 continue;
            }
            if (( selector_for_bit_x_field_value[index] >=  
                               pkt_attr_bit_pos[pkt_attr_cng].start_bit) && 
               ( selector_for_bit_x_field_value[index] <=  
                               pkt_attr_bit_pos[pkt_attr_cng].end_bit))  
            {
                 pkt_attr_bits->cng++;
                 continue;
            }
        }
    }

    if ((pkt_attr_bits->cng) || (pkt_attr_bits->int_pri))            
    {
        index_max = soc_mem_index_max(unit, ING_SVM_PRI_CNG_MAPm);
        for (index = 0; index <= index_max; index++) {
            SOC_IF_ERROR_RETURN(soc_mem_read(
                unit,
                ING_SVM_PRI_CNG_MAPm, 
                MEM_BLOCK_ANY,
                index,
                &comp_pkt_attr_sel->compressed_pri_cnf_attr_map_v[index]));
        }
    }
    if ((pkt_attr_bits->vlan_format) || (pkt_attr_bits->outer_dot1p) || 
                                               (pkt_attr_bits->inner_dot1p))  
    {
        index_max = soc_mem_index_max(unit, ING_SVM_PKT_PRI_MAPm);
        for (index = 0; index < BCM_SVC_METER_MAP_SIZE_256; index++) {
            SOC_IF_ERROR_RETURN(soc_mem_read(
                unit,
                ING_SVM_PKT_PRI_MAPm, 
                MEM_BLOCK_ANY,
                index,
                &comp_pkt_attr_sel->compressed_pkt_pri_attr_map_v[index]));
        }
    }

    if ((pkt_attr_bits->ing_port))            
    {
        index_max = soc_mem_index_max(unit, ING_SVM_PORT_MAPm);
        for (index = 0; index < BCM_SVC_METER_PORT_MAP_SIZE; index++) {
            SOC_IF_ERROR_RETURN(soc_mem_read(
                unit,
                ING_SVM_PORT_MAPm, 
                MEM_BLOCK_ANY,
                index,
                &comp_pkt_attr_sel->compressed_port_attr_map_v[index]));
        }
    }

    if (pkt_attr_bits->tos)            
    {
        index_max = soc_mem_index_max(unit, ING_SVM_TOS_MAPm);
        for (index = 0; index < BCM_SVC_METER_MAP_SIZE_256; index++) {
            SOC_IF_ERROR_RETURN(soc_mem_read(
                unit,
                ING_SVM_TOS_MAPm, 
                MEM_BLOCK_ANY,
                index,
                &comp_pkt_attr_sel->compressed_tos_attr_map_v[index]));
        }
    }

    if ((pkt_attr_bits->pkt_resolution) || (pkt_attr_bits->svp_type) || 
                                                (pkt_attr_bits->drop))      
    {
        index_max = soc_mem_index_max(unit, ING_SVM_PKT_RES_MAPm);
        for (index = 0; index < BCM_SVC_METER_MAP_SIZE_256; index++) {
            SOC_IF_ERROR_RETURN(soc_mem_read(
                unit,
                ING_SVM_PKT_RES_MAPm, 
                MEM_BLOCK_ANY,
                index,
                &comp_pkt_attr_sel->compressed_pkt_res_attr_map_v[index]));
        }
    }
    return rv; 
}

/*
 * Function:
 *      _bcm_esw_global_meter_udf_offset_mode_reinit
 * Purpose:
 *      Recover UDF offset mode information from HW and re-configure SDK
 *      data structures on warm boot
 * Parameters:
 *     unit                  - (IN) unit number 
 *     mode                  - (IN) meter offset mode
 *     selector_count        - (IN) Number of selectors 
 *     selector_for_bit_x_en_field_value - (IN) selector field enable 
 *     selector_for_bit_x_field_value    - (IN) selector field
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_global_meter_udf_offset_mode_reinit(
                                 int unit, 
                                 int mode,
                                 uint32 selector_count, 
                                 uint32  selector_for_bit_x_en_field_value[8],
                                 uint32  selector_for_bit_x_field_value[8])
{
    int rv = BCM_E_NONE;
    uint16  udf0 = 0;
    uint16  udf1 = 0;
    uint32  index = 0;

    for (udf0 = 0, udf1 = 0, index = 0; index < 8; index++) {   
        if (selector_for_bit_x_field_value[index] != 0) {
            if ( selector_for_bit_x_field_value[index] <= 16) {
                 udf0 |= (1 << (selector_for_bit_x_field_value[index] - 1));
            } else {  
                 udf1 |= (1 << (selector_for_bit_x_field_value[index]- 16 - 1));
            }
        } 
    }

    global_meter_offset_mode[unit][mode].used = 1;
    global_meter_offset_mode[unit][mode].reference_count = 0;
    global_meter_offset_mode[unit][mode].meter_attr.mode_type_v = udf_mode;
    global_meter_offset_mode[unit][mode].meter_attr.udf_pkt_attr_selectors_v.\
                                               udf_pkt_attr_bits_v.udf0 = udf0;
    global_meter_offset_mode[unit][mode].meter_attr.udf_pkt_attr_selectors_v.\
                                               udf_pkt_attr_bits_v.udf1 = udf1;
    return rv;
}

/*
 * Function:
 *      _bcm_eswg_lobal_meter_offset_mode_reinit
 * Purpose:
 *      Recover the service meter offset mode configuration information
 *      from HW on warm boot
 * Parameters:
 *     unit                  - (IN) unit number 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_global_meter_offset_mode_reinit(int unit)
{
    int rv = BCM_E_NONE;
    int mode = 0;
    bcm_policer_group_mode_t          group_mode = 0; 
    int                               npolicers = 0;
    uint64 selector_key_value;
    uint32 selector_count = 0, index = 0;
    uint32 selector_for_bit_x_en_field_value[8] = {0}; 
    uint32 selector_for_bit_x_en_field_name[8] = {
                SELECTOR_0_ENf,
                SELECTOR_1_ENf,
                SELECTOR_2_ENf,
                SELECTOR_3_ENf,
                SELECTOR_4_ENf,
                SELECTOR_5_ENf,
                SELECTOR_6_ENf,
                SELECTOR_7_ENf,
    };
    uint32  selector_for_bit_x_field_value[8] = {0};
    uint32  selector_for_bit_x_field_name[8] = {
                SELECTOR_FOR_BIT_0f,
                SELECTOR_FOR_BIT_1f,
                SELECTOR_FOR_BIT_2f,
                SELECTOR_FOR_BIT_3f,
                SELECTOR_FOR_BIT_4f,
                SELECTOR_FOR_BIT_5f,
                SELECTOR_FOR_BIT_6f,
                SELECTOR_FOR_BIT_7f,
    };
    COMPILER_64_ZERO(selector_key_value); 
    for (mode = 1; mode < BCM_POLICER_SVC_METER_MAX_MODE; mode++) { 
        BCM_IF_ERROR_RETURN(soc_reg64_get(unit,
                                          _pkt_attr_sel_key_reg[mode],
                                          REG_PORT_ANY,
                                          0, 
                                          &selector_key_value)
                            );
        if (!COMPILER_64_IS_ZERO(selector_key_value))
        {
            for (selector_count = 0, index = 0; index < 8; index++)
            {
                selector_for_bit_x_en_field_value[index] = soc_reg64_field32_get(
                        unit,
                        _pkt_attr_sel_key_reg[mode],
                        selector_key_value,
                        selector_for_bit_x_en_field_name[index]);
                selector_count += selector_for_bit_x_en_field_value[index];
                selector_for_bit_x_field_value[index] = 0;
                if (selector_for_bit_x_en_field_value[index] != 0) 
                {
                          selector_for_bit_x_field_value[index] = 
                                  soc_reg64_field32_get(
                                     unit,
                                     _pkt_attr_sel_key_reg[mode],
                                     selector_key_value,
                                     selector_for_bit_x_field_name[index]);
                }
            }

            if (soc_reg64_field32_get(unit,
                                     _pkt_attr_sel_key_reg[mode],
                                      selector_key_value, USE_UDF_KEYf)) 
            {
                rv =  _bcm_esw_global_meter_udf_offset_mode_reinit(unit, mode,
                             selector_count, selector_for_bit_x_en_field_value,
                                              selector_for_bit_x_field_value);
                if (BCM_FAILURE(rv)) {
                   POLICER_VVERB(("%s : Unable to re-init UDF offset mode\n", \
                                                            FUNCTION_NAME()));
                   return (rv);
                }
            } else if (soc_reg64_field32_get(unit,
                                   _pkt_attr_sel_key_reg[mode],
                                   selector_key_value, 
                                   USE_COMPRESSED_PKT_KEYf))
            {            
                rv =  _bcm_esw_global_meter_compressed_offset_mode_reinit(unit,
                                          mode, selector_count,
                                          selector_for_bit_x_en_field_value, 
                                          selector_for_bit_x_field_value);
                if (BCM_FAILURE(rv)) {
                   POLICER_VVERB(("%s : Unable to re-init compressed offset \
                                       mode\n", FUNCTION_NAME()));
                   return (rv);
                }
            } else {
                rv = _bcm_esw_global_meter_uncompressed_offset_mode_reinit(
                         unit, mode, selector_count, 
                         selector_for_bit_x_en_field_value,
                         selector_for_bit_x_field_value);
                if (BCM_FAILURE(rv)) {
                   POLICER_VERB(("%s : Unable to re-init uncompressed offset \
                                       mode\n", FUNCTION_NAME()));
                   return (rv);
                }
            }
            /* get the number of policers allocated for this mode */
            rv = _bcm_esw_policer_get_offset_table_policer_count(unit, 
                                              mode, &group_mode, &npolicers);
            if (BCM_FAILURE(rv)) {
               POLICER_VVERB(("%s : Unable to re-init number of policers \
                              to be allcated in this mode\n", FUNCTION_NAME()));
               return (rv);
            }
            global_meter_offset_mode[unit][mode].no_of_policers = npolicers;
            global_meter_offset_mode[unit][mode].group_mode = group_mode;
            if ((group_mode == bcmPolicerGroupModeCascade) ||
               (group_mode == bcmPolicerGroupModeCascadeWithCoupling)) {
                global_meter_offset_mode[unit][mode].meter_attr.mode_type_v = 
                                                               cascade_mode;
            }
        }
    } 
    return rv; 
}

/*
 * Function:
 *      _bcm_esw_global_meter_action_reinit
 * Purpose:
 *      Recover the policer action table entries on warm boot
 * Parameters:
 *     unit                  - (IN) unit number 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_global_meter_action_reinit(unit)
{
    int rv = BCM_E_NONE;
    svm_policy_table_entry_t meter_action;
    uint32 green_action[1] = {0}, yellow_action[1] = {0}, red_action[1] = {0};
    uint32 index = 0;
    int total_size = soc_mem_index_max(unit, SVM_POLICY_TABLEm); 

    GLOBAL_METER_LOCK(unit);
    for (index = 0; index <= total_size; index++) {
        /* read action table entry */
        rv = READ_SVM_POLICY_TABLEm(unit, MEM_BLOCK_ANY, index, &meter_action);
        if (BCM_FAILURE(rv)) {
            GLOBAL_METER_UNLOCK(unit);
            POLICER_VVERB(("%s : Unable to read SVM_POLICY_TABLE at \
                                  index %d \n", FUNCTION_NAME(), index));
            return (rv);
        }
        soc_SVM_POLICY_TABLEm_field_get(unit, &meter_action,
                                                 G_ACTIONSf, &green_action[0]);
        soc_SVM_POLICY_TABLEm_field_get(unit, &meter_action,
                                                 Y_ACTIONSf, &yellow_action[0]);
        soc_SVM_POLICY_TABLEm_field_get(unit, &meter_action,
                                                 R_ACTIONSf, &red_action[0]);

        if ((green_action[0] > 0) || (yellow_action[0] > 0) || 
                                                      (red_action[0] > 0)) 
        {
            global_meter_action_bookkeep[unit][index].used = 1;
            /* Reserve the the action table entry */
            rv = shr_aidxres_list_reserve_block(
                         meter_action_list_handle[unit], index, 1); 
            if (BCM_FAILURE(rv)) {
                GLOBAL_METER_UNLOCK(unit);
                POLICER_VVERB(("%s : Unable to reserve action id %d in \
                           index management list\n", FUNCTION_NAME(), index));
                return (rv);
            }
        }
    }
    GLOBAL_METER_UNLOCK(unit);
    return rv; 
}

/*
 * Function:
 *      _bcm_esw_global_meter_policer_sync
 * Purpose:
 *      Write poliers which are created but not yet used in any
 *      table to scache so that we can recover the configuration
 *      on warm boot
 * Parameters:
 *     unit                  - (IN) unit number 
 * Returns:
 *     BCM_E_XXX 
 */
int
_bcm_esw_global_meter_policer_sync(int unit)
{
    int    rv = BCM_E_NONE;
    soc_scache_handle_t scache_handle;
    _global_meter_policer_control_t *global_meter_pl = NULL;
    uint32 hash_index;      /* Entry hash  */
    int policer_id = 0;
    uint8  *policer_state;
    int size = 0, count = 0;
    if (!soc_feature(unit, soc_feature_global_meter)) {
        return BCM_E_UNAVAIL;
    }
    CHECK_GLOBAL_METER_INIT(unit);
    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_POLICER, 0);
    size = sizeof(int) * BCM_GLOBAL_METER_MAX_SCACHE_ENTRIES;
    SOC_IF_ERROR_RETURN(_bcm_esw_scache_ptr_get(unit, scache_handle,
                          FALSE, size, &policer_state,
                          BCM_WB_DEFAULT_VERSION, NULL));

    if (NULL == policer_state) {
        POLICER_VERB(("%s : SCACHE Memory not available \n", FUNCTION_NAME()));
        return BCM_E_MEMORY;
    }
    /* go through policer control bookkeep structure and get policer
       entries with ref_count=0 */
    for (hash_index = 0; hash_index < _GLOBAL_METER_HASH_SIZE; hash_index++) {
        global_meter_pl  =  global_meter_policer_bookkeep[unit][hash_index];
        while ((NULL != global_meter_pl) && 
                      (count < BCM_GLOBAL_METER_MAX_SCACHE_ENTRIES)) {
            /* Match entry id. */
            if (global_meter_pl->ref_count == 0) {
                policer_id  = global_meter_pl->pid;
                sal_memcpy(policer_state, &policer_id, sizeof(bcm_policer_t));
                policer_state += sizeof(bcm_policer_t);
                count++;
            }
            global_meter_pl = global_meter_pl->next;
        }
        if (count == BCM_GLOBAL_METER_MAX_SCACHE_ENTRIES) {
            return rv;
        }
    }     
    return rv;
}
/*
 * Function:
 *      _bcm_esw_global_meter_policer_recover_from_scache
 * Purpose:
 *     Recover policers which are not configured in HW from
 *     Scache as part of warm boot
 * Parameters:
 *     unit                  - (IN) unit number 
 * Returns:
 *     BCM_E_XXX 
 */

int _bcm_esw_global_meter_policer_recover_from_scache(int unit)
{
    int    rv = BCM_E_NONE;
    int    stable_size;
    int policer_id = 0;
    uint8  *policer_state;
    int size = 0, scache_index = 0;
    soc_scache_handle_t scache_handle;
    _global_meter_policer_control_t *policer_control = NULL;

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_POLICER, 0);
    size = sizeof(int32) * BCM_GLOBAL_METER_MAX_SCACHE_ENTRIES;
    if (SOC_WARM_BOOT(unit)) {
    /* Warm Boot */
        SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
        if (stable_size > size) {
            SOC_IF_ERROR_RETURN(_bcm_esw_scache_ptr_get(unit, scache_handle,
                                    FALSE, size, &policer_state,
                                    BCM_WB_DEFAULT_VERSION, NULL));

            if (NULL == policer_state) {
                POLICER_VVERB(("%s : SCACHE Memory not available \n",\
                                                    FUNCTION_NAME()));
                return BCM_E_MEMORY;
            }
        /*  read from scache and create policer bookkeep data structure */
            for (scache_index = 0; scache_index < 
                        BCM_GLOBAL_METER_MAX_SCACHE_ENTRIES; scache_index++) {
                           sal_memcpy(&policer_id, 
                                  &policer_state[scache_index * sizeof(int)],
                                                                sizeof(int));
                if ((policer_id & BCM_POLICER_GLOBAL_METER_INDEX_MASK) > 0) {
                    rv = _bcm_esw_policer_config_from_meter_table(unit, 
                                    policer_id, policer_control);
                }
            }
            sal_memset(policer_state, 0, size);
        }
    } else {
    /* Cold Boot */
        rv = _bcm_esw_scache_ptr_get(unit, scache_handle, TRUE,
                                     size, &policer_state,
                                     BCM_WB_DEFAULT_VERSION, NULL);
        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
            POLICER_VVERB(("%s : Scache Memory not available\n", \
                                                  FUNCTION_NAME()));
            return rv;
        }
    }
    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_esw_global_meter_reinit
 * Purpose:
 *      Reinit all the global meter internal data structures on warm boot
 * Parameters:
 *     unit                  - (IN) unit number 
 * Returns:
 *     BCM_E_XXX 
 */
int _bcm_esw_global_meter_reinit(int unit)
{
    int rv = BCM_E_NONE;
    rv =  _bcm_esw_global_meter_offset_mode_reinit(unit);
    if (BCM_FAILURE(rv)) {
        POLICER_VERB(("%s : Unable to reinit offset modes for  \
                            unit %d\n", FUNCTION_NAME(), unit));
        return (rv);
    }
    /* go through all policer entries to increment action reference count
       and set policer configuration parameter */ 
    rv = _bcm_policer_config_reinit(unit); 
    if (BCM_FAILURE(rv)) {
        POLICER_VERB(("%s : Unable to reinit policer configuration for  \
                            unit %d\n", FUNCTION_NAME(), unit));
        return (rv);
    }
    /* go through scache and recover policer configuration */
    rv = _bcm_esw_global_meter_policer_recover_from_scache(unit);
    if (BCM_FAILURE(rv)) {
        POLICER_VERB(("%s : Unable to reinit policer configuration for  \
                            unit %d from scache\n", FUNCTION_NAME(), unit));
        return (rv);
    }
    /* go through all the table entries to increment policer and 
       mode reference count */
    rv = _bcm_policer_ref_count_reinit(unit); 
    if (BCM_FAILURE(rv)) {
        POLICER_VERB(("%s : Unable to reinit policer and offset mode ref count \
                            for unit %d\n", FUNCTION_NAME(), unit));
        return (rv);
    }
    /* reinit action table bookkeep data structure */
    rv =  _bcm_esw_global_meter_action_reinit(unit);
    if (BCM_FAILURE(rv)) {
        POLICER_VERB(("%s : Unable to reinit meter action configuration for  \
                            unit %d\n", FUNCTION_NAME(), unit));
        return (rv);
    }
    return BCM_E_NONE; 
}
#endif /* BCM_WARM_BOOT_SUPPORT */
