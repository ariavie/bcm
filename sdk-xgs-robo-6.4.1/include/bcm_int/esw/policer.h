/*
 * $Id: policer.h,v 1.11 Broadcom SDK $
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
 * This file contains Service meters(global meters) definitions internal 
 * to the BCM library.
 */
#ifndef _BCM_INT_SVC_METER_H
#define _BCM_INT_SVC_METER_H


#include <shared/bsl.h>

#include <bcm/types.h>
#include <sal/core/libc.h>
#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/l2x.h>
#include <bcm/error.h>
#include <bcm/policer.h>
#include <soc/mcm/memregs.h>

#define BCM_POLICER_TYPE_REGEX       0x1
#define BCM_POLICER_TYPE_SHIFT  24
#define BCM_POLICER_TYPE_MASK   0x0f000000
#define BCM_POLICER_IS_REGEX_METER(policer_id) \
  ((((policer_id) & BCM_POLICER_TYPE_MASK) >> BCM_POLICER_TYPE_SHIFT)== BCM_POLICER_TYPE_REGEX)

#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_CNG_ATTR_BITS 0x00000001 
#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INT_PRI_ATTR_BITS 0x00000002 
#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_VLAN_FORMAT_ATTR_BITS 0x00000004
#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_OUTER_DOT1P_ATTR_BITS 0x00000008
#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INNER_DOT1P_ATTR_BITS 0x00000010
#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_INGRESS_PORT_ATTR_BITS 0x00000020
#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_TOS_ATTR_BITS 0x00000040 
#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_PKT_RESOLUTION_ATTR_BITS 0x00080
#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_SVP_TYPE_ATTR_BITS 0x00000100 
#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_DROP_ATTR_BITS 0x00000200 
#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_IP_PKT_ATTR_BITS 0x00000400 
#define BCM_POLICER_SVC_METER_UNCOMPRESSED_USE_SHORT_INT_PRI_ATTR_BITS 0x00000800

#define BCM_SVC_METER_PORT_MAP_SIZE         172         
#define BCM_SVC_METER_MAP_SIZE_64           64         
#define BCM_SVC_METER_MAP_SIZE_256          256        
#define BCM_SVC_METER_MAP_SIZE_1024         1024        

#define    BCM_POLICER_SVC_METER_MAX_MODE                4
#define    BCM_POLICER_SVC_METER_OFFSET_TABLE_KEY_SIZE   8
#define    BCM_POLICER_SVC_METER_OFFSET_TABLE_MAX_INDEX  4
#define    BCM_POLICER_SVC_METER_MAX_OFFSET              256
#define    SVC_METER_UDF_MAX_BIT_POSITION                16
#define    SVC_METER_UDF1_MAX_BIT_POSITION               32 
#define    SVC_METER_UDF1_VALID                          0x1
#define    SVC_METER_UDF2_VALID                          0x2

#define BCM_POLICER_SVC_METER_CNG_ATTR_SIZE                             2
#define BCM_POLICER_SVC_METER_IFP_ATTR_SIZE                             2
#define BCM_POLICER_SVC_METER_INT_PRI_ATTR_SIZE                         4
#define BCM_POLICER_SVC_METER_SHORT_INT_PRI_ATTR_SIZE                   3
#define BCM_POLICER_SVC_METER_VLAN_FORMAT_ATTR_SIZE                     2
#define BCM_POLICER_SVC_METER_OUTER_DOT1P_ATTR_SIZE                     3
#define BCM_POLICER_SVC_METER_INNER_DOT1P_ATTR_SIZE                     3
#define BCM_POLICER_SVC_METER_ING_PORT_ATTR_SIZE                        6
#define BCM_POLICER_SVC_METER_TOS_ATTR_SIZE                             6
#define BCM_POLICER_SVC_METER_PKT_REOLUTION_ATTR_SIZE                   6 
#define BCM_POLICER_SVC_METER_SVP_TYPE_ATTR_SIZE                        1
#define BCM_POLICER_SVC_METER_SVP_NETWORK_GROUP_ATTR_SIZE               3
#define BCM_POLICER_SVC_METER_DROP_ATTR_SIZE                            1
#define BCM_POLICER_SVC_METER_IP_PKT_ATTR_SIZE                          1

#define BCM_POLICER_GLOBAL_METER_INDEX_MASK  0x1FFFFFFF
#define BCM_POLICER_GLOBAL_METER_MODE_MASK   0xE0000000
#define BCM_POLICER_GLOBAL_METER_MODE_SHIFT  29 
#define BCM_POLICER_GLOBAL_METER_NO_OFFSET_MODE 0x20000000
#define BCM_POLICER_GLOBAL_METER_DISABLE  0x1
#define BCM_POLICER_GLOBAL_METER_MAX_POOL 8

#define BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_INT_PRI_BIT_POS 16
#define BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_ECN_BIT_POS 17
#define BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_DOT1P_BIT_POS 18
#define BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_CNG_BIT_POS 19
#define BCM_POLICER_GLOBAL_METER_ACTION_CHANGE_DSCP_BIT_POS 21

#define BCM_POLICER_GLOBAL_METER_ACTION_ECN_BIT_POS 0
#define BCM_POLICER_GLOBAL_METER_ACTION_DSCP_BIT_POS 2
#define BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_BIT_POS 8
#define BCM_POLICER_GLOBAL_METER_ACTION_DROP_BIT_POS 11
#define BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_BIT_POS 12


#define BCM_POLICER_GLOBAL_METER_ACTION_DSCP_MASK 0xfc
#define BCM_POLICER_GLOBAL_METER_ACTION_DOT1P_MASK 0x700 
#define BCM_POLICER_GLOBAL_METER_ACTION_INT_PRI_MASK 0xf000 
#define BCM_POLICER_GLOBAL_METER_ACTION_SHORT_INT_PRI_MASK 0x7000
#define BCM_POLICER_GLOBAL_METER_ACTION_ECN_MASK 0x3
#define BCM_POLICER_GLOBAL_METER_ACTION_CNG_MASK 0x180000

#define BCM_POLICER_GLOBAL_METER_MAX_ACTIONS  8192

#define GLOBAL_METER_ALLOC_VERTICAL  0x0
#define GLOBAL_METER_ALLOC_HORIZONTAL  0x1


#define GLOBAL_METER_REFRESH_MAX_DISABLE_LIMIT 0x1f
#define GLOBAL_METER_MODE_DEFAULT 0x0
#define GLOBAL_METER_MODE_CASCADE 0x1
#define GLOBAL_METER_MODE_TR_TCM 0x2
#define GLOBAL_METER_MODE_TR_TCM_MODIFIED 0x3
#define GLOBAL_METER_MODE_SR_TCM 0x4
#define GLOBAL_METER_MODE_SR_TCM_MODIFIED 0x5

#define BCM_GLOBAL_METER_MAX_SCACHE_ENTRIES 16

/* Generic memory allocation routine. */
#define _GLOBAL_METER_XGS3_ALLOC(_ptr_,_size_,_descr_)                 \
    do {                                             \
         if (NULL == (_ptr_)) {                       \
             (_ptr_) = sal_alloc((_size_), (_descr_)); \
         }                                            \
         if((_ptr_) != NULL) {                        \
              sal_memset((_ptr_), 0, (_size_));        \
         }  else {                                    \
             LOG_ERROR(BSL_LS_BCM_POLICER,                              \
                       (BSL_META("Error:Alloc failure %s\n"), (_descr_))); \
         }                                                              \
    } while (0)

#define _GLOBAL_METER_HASH_SIZE 0x100 
#define _GLOBAL_METER_HASH_MASK 0xff 



#define _GLOBAL_METER_HASH_INSERT(_hash_ptr_, _inserted_ptr_, _index_)    \
    do {                                                 \
        (_inserted_ptr_)->next = (_hash_ptr_)[(_index_)]; \
        (_hash_ptr_)[(_index_)] = (_inserted_ptr_);       \
    } while (0)

#define _GLOBAL_METER_HASH_REMOVE(_hash_ptr_, _entry_type_, _removed_ptr_, _index_)  \
    do {                                                    \
         _entry_type_ *_prev_ent_ = NULL;                    \
         _entry_type_ *_lkup_ent_ = (_hash_ptr_)[(_index_)]; \
         while (NULL != _lkup_ent_) {                        \
             if (_lkup_ent_ != (_removed_ptr_))  {           \
                 _prev_ent_ = _lkup_ent_;                    \
                 _lkup_ent_ = _lkup_ent_->next;              \
                 continue;                                   \
             }                                               \
             if (_prev_ent_!= NULL) {                        \
                 _prev_ent_->next = (_removed_ptr_)->next;   \
             } else {                                        \
                (_hash_ptr_)[(_index_)] = (_removed_ptr_)->next; \
             }                                               \
             break;                                          \
        }                                                   \
    } while (0)

/* Policer service meter Modes. */
typedef enum bcm_policer_svc_meter_mode_type_e {
    uncompressed_mode,/*service meter uncompressed mode*/
    compressed_mode, /* service meter compressed mode */
    udf_mode,      /* service meter in udf mode */
    cascade_mode,
    cascade_with_coupling_mode
} bcm_policer_svc_meter_mode_type_t;

/* bcm_policer_svc_meter_mode_t */
typedef uint32 bcm_policer_svc_meter_mode_t;

/* compressed_pri_cnf_attr_map_t */
typedef uint8 compressed_pri_cnf_attr_map_t;

/* compressed_pkt_pri_attr_map_t */
typedef uint8 compressed_pkt_pri_attr_map_t;

/* compressed_port_attr_map_t */
typedef uint8 compressed_port_attr_map_t;

/* compressed_tos_attr_map_t */
typedef uint8 compressed_tos_attr_map_t;

/* compressed_pkt_res_attr_map_t */
typedef uint8 compressed_pkt_res_attr_map_t;

/* offset_table_map_t */
typedef struct offset_table_entry_s {
    uint8 offset;
    uint8 meter_enable;
    uint8 pool;
} offset_table_entry_t;

typedef struct bcm_policer_svc_meter_uncompressed_attr_selectors_s {
    uint32 uncompressed_attr_bits_selector;
    offset_table_entry_t offset_map[BCM_SVC_METER_MAP_SIZE_256];
} uncompressed_attr_selectors_t;

typedef struct bcm_policer_svc_meter_pkt_attr_bits_s {
    uint8 cng;              /* CNG bits - corresponds to 2 bits in uncompressed
                               mode */
    uint8 int_pri;          /* internal priority bits -corresponds to 4 bits in
                               uncompressed mode */
    uint8 short_int_pri;    /* internal priority bits -corresponds to 3 bits in
                               uncompressed mode */
    uint8 vlan_format;      /* vlan format - corresponds to 2 bits in
                               uncompressed mode */
    uint8 outer_dot1p;      /* outer 802.1p - corresponds to 3 bits in
                               uncompressed mode */
    uint8 inner_dot1p;      /* inner 802.1p - corresponds to 3 bits in
                               uncompressed mode */
    uint8 ing_port;         /* ingress port - corresponds to 6 bits in
                               uncompressed mode */
    uint8 tos;              /* tos bits - corresponds to 8 bits in uncompressed
                               mode */
    uint8 pkt_resolution;   /* packet resolution - corresponds to 6 bits in
                               uncompressed mode */
    uint8 svp_type;         /* svp type - corresponds to 1 bit in uncompressed
                               mode */
    uint8 drop;             /* use drop bit -corresponds to 1 bit in
                               uncompressed mode */
    uint8 ip_pkt;           /* corresponds to 1 bit in uncompressed mode */
} pkt_attr_bits_t;

typedef struct bcm_policer_svc_meter_udf_pkt_attr_bits_s {
    uint8 udf0; /* Represents bits to be used from UDF0 in udf mode */
    uint8 udf1; /* Represents bits to be used from UDF1 in udf mode */
} udf_pkt_attr_bits_t;

typedef struct bcm_policer_svc_meter_compressed_attr_selectors_s {
    pkt_attr_bits_t pkt_attr_bits_v; 
    compressed_pri_cnf_attr_map_t 
         compressed_pri_cnf_attr_map_v[BCM_SVC_METER_MAP_SIZE_64]; 
    compressed_pkt_pri_attr_map_t 
         compressed_pkt_pri_attr_map_v[BCM_SVC_METER_MAP_SIZE_256]; 
    compressed_port_attr_map_t 
         compressed_port_attr_map_v[BCM_SVC_METER_PORT_MAP_SIZE]; 
    compressed_tos_attr_map_t 
         compressed_tos_attr_map_v[BCM_SVC_METER_MAP_SIZE_256]; 
    compressed_pkt_res_attr_map_t 
         compressed_pkt_res_attr_map_v[BCM_SVC_METER_MAP_SIZE_1024]; 
    offset_table_entry_t offset_map[BCM_SVC_METER_MAP_SIZE_256];
} compressed_attr_selectors_t;

typedef struct bcm_policer_svc_meter_udf_pkt_attr_selectors_s {
    udf_pkt_attr_bits_t udf_pkt_attr_bits_v; 
} udf_pkt_attr_selectors_t;

typedef struct bcm_policer_svc_meter_attr_s {
    bcm_policer_svc_meter_mode_type_t mode_type_v; 
    /* Valid only for uncompressed */
    uncompressed_attr_selectors_t uncompressed_attr_selectors_v;
    /* valid only for compressed */
    compressed_attr_selectors_t compressed_attr_selectors_v;
    /* valid only for udf */
    udf_pkt_attr_selectors_t udf_pkt_attr_selectors_v; 
} bcm_policer_svc_meter_attr_t;


/* Create a svc meter mode entry. */
extern int _bcm_policer_svc_meter_create_mode(
    int unit, 
    bcm_policer_svc_meter_attr_t meter_attr, 
    bcm_policer_svc_meter_mode_t *bcm_policer_svc_meter_mode_v);

/* Delete a svc meter mode entry. */
extern int _bcm_policer_svc_meter_delete_mode(
    int unit, 
    bcm_policer_svc_meter_mode_t bcm_policer_svc_meter_mode_v);


typedef struct  bcm_policer_svc_meter_bookkeep_mode_s {
    uint32                          used;
    uint32                          reference_count;
    bcm_policer_svc_meter_attr_t    meter_attr;
    uint32                          no_of_policers;
    bcm_policer_group_mode_t        group_mode; 
} bcm_policer_svc_meter_bookkeep_mode_t;

bcm_error_t _bcm_policer_svc_meter_get_available_mode( uint32 unit,
    bcm_policer_svc_meter_mode_t        *svc_meter_mode 
);

bcm_error_t _bcm_policer_svc_meter_update_selector_keys_enable_fields(
    int         unit, 
    soc_reg_t   bcm_policer_svc_meter_pkt_attr_selector_key_reg, 
    uint64      *bcm_policer_svc_meter_pkt_attr_selector_key_reg_value,
    uint32      bcm_policer_svc_meter_pkt_attr_bit_position,
    uint32      bcm_policer_svc_meter_pkt_attr_total_bits,
    uint8       *bcm_policer_svc_meter_current_bit_selector_position
);

bcm_error_t _bcm_policer_svc_meter_update_offset_table(
    int         unit,
    soc_mem_t   bcm_policer_svc_meter_offset_table_mem,
    bcm_policer_svc_meter_mode_t     bcm_policer_svc_meter_mode_v,
    offset_table_entry_t *offset_map 
);

bcm_error_t _bcm_policer_svc_meter_reserve_mode( 
    uint32                         unit,
    bcm_policer_svc_meter_mode_t   bcm_policer_svc_meter_mode_v,
    bcm_policer_group_mode_t       group_mode,
    bcm_policer_svc_meter_attr_t   *meter_attr
);

bcm_error_t _bcm_policer_svc_meter_unreserve_mode(
    uint32                            unit,
    bcm_policer_svc_meter_mode_t   bcm_policer_svc_meter_mode_v
);

bcm_error_t bcm_policer_svc_meter_inc_mode_reference_count(
    uint32                       unit,
    bcm_policer_svc_meter_mode_t bcm_policer_svc_meter_mode_v
);

bcm_error_t bcm_policer_svc_meter_dec_mode_reference_count(
    uint32                         unit,
    bcm_policer_svc_meter_mode_t   bcm_policer_svc_meter_mode_v
);

bcm_error_t _bcm_policer_svc_meter_update_udf_selector_keys(
    int                       unit,
    soc_reg_t                 bcm_policer_svc_meter_pkt_attr_selector_key_reg,
    udf_pkt_attr_selectors_t    
                              udf_pkt_attr_selectors_v,
    uint32                    *total_udf_bits
);


bcm_error_t _bcm_policer_svc_meter_get_mode_info(
                int                                     unit,
                bcm_policer_svc_meter_mode_t            meter_mode_v,
                bcm_policer_svc_meter_bookkeep_mode_t   *mode_info
                );

/* policer API's */

typedef struct  bcm_policer_global_meter_action_bookkeep_s {
    uint32                          used;
    uint32                          reference_count;
} bcm_policer_global_meter_action_bookkeep_t;

typedef struct  bcm_policer_global_meter_init_staus_s {
    uint32                          initialised;
} bcm_policer_global_meter_init_status_t;

int bcm_esw_global_meter_init(int unit);
int _bcm_esw_global_meter_reinit(int unit);

int _bcm_esw_global_meter_offset_mode_reinit(int unit);
int _bcm_esw_global_meter_compressed_offset_mode_reinit(
                                 int unit, 
                                 int mode,
                                 uint32 selector_count, 
                                 uint32 selector_for_bit_x_en_field_value[8],
                                 uint32 selector_for_bit_x_field_value[8]);
int _bcm_esw_global_meter_uncompressed_offset_mode_reinit(
                                 int unit, 
                                 int mode,
                                 uint32 selector_count, 
                                 uint32 selector_for_bit_x_en_field_value[8],
                                 uint32 selector_for_bit_x_field_value[8]);
int _bcm_esw_global_meter_udf_offset_mode_reinit(
                                 int unit, 
                                 int mode,
                                 uint32 selector_count, 
                                 uint32 selector_for_bit_x_en_field_value[8],
                                 uint32 selector_for_bit_x_field_value[8]);

int _bcm_esw_global_meter_action_reinit(int unit);
int _bcm_esw_policer_validate(int unit, bcm_policer_t *policer);
int _bcm_esw_policer_decrement_ref_count(int unit, bcm_policer_t policer);
int _bcm_esw_policer_increment_ref_count(int unit, bcm_policer_t policer);
int _bcm_esw_global_meter_cleanup(int unit);
int _bcm_esw_add_policer_to_table(int unit,bcm_policer_t policer,
                                       int table, int index, void *data);
int _bcm_esw_delete_policer_from_table(int unit,bcm_policer_t policer, 
                                 soc_mem_t table, int index, void *data);
int _bcm_esw_get_policer_from_table(int unit,soc_mem_t table,int index, 
                                      void *data, 
                                  bcm_policer_t *policer, int skip_read);


/*
 * Typedef:
 *     _global_meter_policer_t
 * Purpose:
 *     This is the policer description structure.
 *     Indexed by bcm_policer_t handle.
 */
typedef struct _global_meter_policer_s {
    bcm_policer_t        pid;         /* Unique policer identifier.       */
    uint16               ref_count;/* SW object use reference count.   */
    uint32               action_id; /* Index to action table */ 
    uint32               direction;
    uint32               no_of_policers;
    uint8                offset[8];
    struct _global_meter_policer_s *next;    /* Policer lookup linked list */
}_global_meter_policer_control_t;

/*
 * Typedef:
 *     _global_meter_horizontal_alloc_t
 * Purpose:
 *     This is the policer description structure.
 *     Indexed by bcm_policer_t handle.
 */
typedef struct _global_meter_horizontal_alloc_s {
    uint8  alloc_bit_map;/*Meter allocation bit map across pools for an index */
    uint8  no_of_groups_allocated;/* SW object use reference count.   */
    uint8  first_bit_to_use;  /* first bit position from where new 
                                             block can be allocated */ 
    uint8  last_bit_to_use;  /* last pool till which 
                                             block can be allocated */ 
}_global_meter_horizontal_alloc_t;

int _bcm_global_meter_policer_get(int unit, bcm_policer_t pid, 
                                 _global_meter_policer_control_t **policer_p);


int _bcm_gloabl_meter_reserve_bloc_horizontally(int unit, int pool_id, 
                                                          bcm_policer_t pid); 
int _bcm_gloabl_meter_unreserve_bloc_horizontally(int unit, int pool_id,  
                                                          bcm_policer_t pid); 
int _bcm_global_meter_reserve_bloc_horizontally(int unit, int pool_id,  
                                                          bcm_policer_t pid); 

int _bcm_esw_global_meter_policer_sync(int unit);
int _bcm_esw_global_meter_policer_get(int unit, bcm_policer_t policer_id, 
                                                bcm_policer_config_t *pol_cfg);
int _bcm_esw_global_meter_policer_destroy(int unit, bcm_policer_t policer_id);
int _bcm_esw_global_meter_policer_destroy_all(int unit);
int _bcm_esw_global_meter_policer_destroy2(int unit, 
               _global_meter_policer_control_t *policer_control); 
int _bcm_esw_global_meter_policer_set(int unit, bcm_policer_t policer_id,
                   bcm_policer_config_t *pol_cfg);
int  _bcm_esw_global_meter_policer_traverse(int unit, 
                                bcm_policer_traverse_cb cb,  void *user_data);

int  _bcm_esw_policer_svc_meter_delete_mode( int unit,
                    bcm_policer_svc_meter_mode_t bcm_policer_svc_meter_mode_v);
int _bcm_global_meter_get_coupled_cascade_policer_index(int unit, 
                                          bcm_policer_t policer_id,
                  _global_meter_policer_control_t *policer_control,
                   int *new_index);
int _bcm_global_meter_base_policer_get(int unit, bcm_policer_t pid, 
                         _global_meter_policer_control_t **policer_p);
int _bcm_esw_get_policer_id_from_index_offset(int unit, int index, 
                                              int offset_mode, 
                                              bcm_policer_t *policer_id);
int _bcm_esw_reset_policer_from_table(int unit, bcm_policer_t policer, 
                                      int table, int index, void *data); 
int _bcm_esw_policer_group_create(int unit, bcm_policer_group_mode_t mode,
                                   int skip_pool,
                                   bcm_policer_t *policer_id, int *npolicers);
int _bcm_esw_get_policer_table_index(int unit, bcm_policer_t policer,
                                                         int *index); 
int _check_global_meter_init(int unit);
#endif	/* !_BCM_INT_SVC_METER_H */
