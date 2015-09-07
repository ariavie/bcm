/*
 * $Id: field.h 1.402.2.1 Broadcom SDK $
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
 * File:        field.h
 * Purpose:     Internal Field Processor data structure definitions for the
 *              BCM library.
 *
 */

#ifndef _BCM_INT_FIELD_H
#define _BCM_INT_FIELD_H

#include <bcm/field.h>
#include <bcm_int/common/field.h>

#include <soc/mem.h>
#include <soc/profile_mem.h>
#include <soc/er_tcam.h>

#include <sal/core/sync.h>

#ifdef BCM_FIELD_SUPPORT

#define _FP_PORT_BITWIDTH(_unit_)    ((SOC_IS_TD_TT(_unit_) || \
                                      SOC_IS_KATANA(_unit_)) ? 7 : \
                                      (SOC_IS_KATANA2(_unit_) ? 8 : 6))

#define _FP_INVALID_INDEX            (-1)

/* 64 bit software counter collection bucket. */
#define _FP_64_COUNTER_BUCKET  512 

/* Maximum number of paired slices.*/
#define _FP_PAIR_MAX   (3)

/* 
 * Meter mode. 
 */
#define _BCM_FIELD_METER_BOTH (BCM_FIELD_METER_PEAK | \
                               BCM_FIELD_METER_COMMITTED)

/* Value for control->tcam_ext_numb that indicates there is no external TCAM. */
#define FP_EXT_TCAM_NONE _FP_INVALID_INDEX

/*
 * Initial group IDs and entry IDs.
 */
#define _FP_ID_BASE 1
#define _FP_ID_MAX  (0x1000000)

/*
 * Action flags. 
 */
/* Action valid. */
#define _FP_ACTION_VALID              (1 << 0)
/* Reinstall is required. */
#define _FP_ACTION_DIRTY              (1 << 1)  
/* Remove action from hw. */
#define _FP_ACTION_RESOURCE_FREE      (1 << 2)
/* Action was updated free previously used hw resources. */
#define _FP_ACTION_OLD_RESOURCE_FREE  (1 << 3)

#define _FP_ACTION_MODIFY             (1 << 4)
                                      
#define _FP_ACTION_HW_FREE (_FP_ACTION_RESOURCE_FREE | \
                            _FP_ACTION_OLD_RESOURCE_FREE)

/* Field policy table Cos queue priority hw modes */ 
#define _FP_ACTION_UCAST_MCAST_QUEUE_NEW_MODE   0x1
#define _FP_ACTION_UCAST_QUEUE_NEW_MODE         0x8
#define _FP_ACTION_MCAST_QUEUE_NEW_MODE         0x9
/* Action set unicast queue value */
#define _FP_ACTION_UCAST_QUEUE_SET(q) ((q) & (0xf))
/* Action set multicast queue value */
#define _FP_ACTION_MCAST_QUEUE_SET(q) (((q) & 0x7) << 4)
/* Action get unicast queue value */
#define _FP_ACTION_UCAST_QUEUE_GET(q) ((q) & (0xf))
/* Action get multicast queue value */
#define _FP_ACTION_MCAST_QUEUE_GET(q) (((q) >> 4) & 0x7)
/* Action set both unicast and multicast queue value */
#define _FP_ACTION_UCAST_MCAST_QUEUE_SET(u,m) ((_FP_ACTION_UCAST_QUEUE_SET(u)) | \
                                                (_FP_ACTION_MCAST_QUEUE_SET(m)))

/* 
 * Maximum number of meter pools
 *     Should change when new chips are defined
 */
#define _FIELD_MAX_METER_POOLS 16

#define _FIELD_MAX_CNTR_POOLS  16

/*
 * Internal version of qset add that will allow Data[01] to be set. 
 */
#define BCM_FIELD_QSET_ADD_INTERNAL(qset, q)  SHR_BITSET(((qset).w), (q))
#define BCM_FIELD_QSET_REMOVE_INTERNAL(qset, q)  SHR_BITCLR(((qset).w), (q))
#define BCM_FIELD_QSET_TEST_INTERNAL(qset, q) SHR_BITGET(((qset).w), (q)) 

/*
 * Typedef:
 *     _field_sel_t
 * Purpose:
 */
typedef struct _field_sel_s {
    int8    fpf0;              /* FPF0 field(s) select.              */
    int8    fpf1;              /* FPF1 field(s) select.              */
    int8    fpf2;              /* FPF2 field(s) select.              */
    int8    fpf3;              /* FPF3 field(s) select.              */
    int8    fpf4;              /* FPF4 field(s) selector.            */
    int8    extn;              /* field(s) select external.          */
    int8    src_class_sel;     /* Source lookup class selection.     */        
    int8    dst_class_sel;     /* Destination lookup class selection.*/         
    int8    intf_class_sel;    /* Interface class selection.         */
    int8    ingress_entity_sel;/* Ingress entity selection.          */
    int8    src_entity_sel;    /* Src port/trunk entity selection.          */    
    int8    dst_fwd_entity_sel;/* Destination forwarding  entity.    */
    int8    fwd_field_sel;     /* Forwarding field type select.      */
    int8    loopback_type_sel; /* Loopback/Tunnel type selection.    */
    int8    ip_header_sel;     /* Inner/Outer Ip header selection.   */
    int8    ip6_addr_sel;      /* Ip6 address format selection.      */
    int8    intraslice;        /* Intraslice double wide selection.  */
    int8    secondary;
    int8    inner_vlan_overlay;
    int8    intraslice_vfp_sel;
    int8    aux_tag_1_sel;
    int8    aux_tag_2_sel;
    int8    normalize_ip_sel;
    int8    normalize_mac_sel;
    int8    egr_class_f1_sel;
    int8    egr_class_f2_sel;
    int8    egr_class_f3_sel;
    int8    egr_class_f4_sel;
    int8    src_dest_class_f1_sel;
    int8    src_dest_class_f3_sel;
    int8    src_type_sel;
    int8    egr_key4_dvp_sel;
} _field_sel_t;


/*
 * Count number of bits in a memory field.
 */
#define _FIELD_MEM_FIELD_LENGTH(_u_, _mem_, _fld_) \
             (SOC_MEM_FIELD_VALID((_u_), (_mem_), (_fld_)) ?  \
              (soc_mem_field_length((_u_), (_mem_), (_fld_))) : 0)


/*
 * Field select code macros
 *
 * These are for resolving if the select code has meaning or is really a don't
 * care.
 */

#define _FP_SELCODE_DONT_CARE (-1)
#define _FP_SELCODE_DONT_USE  (-2)

#define _FP_SELCODE_INVALIDATE(selcode) \
     ((selcode) = _FP_SELCODE_DONT_CARE);

#define _FP_SELCODE_IS_VALID(selcode) \
     (((selcode) == _FP_SELCODE_DONT_CARE || \
       (selcode) == _FP_SELCODE_DONT_USE) ? 0 : 1)

#define _FP_SELCODE_IS_INVALID(selcode) \
     (0 == _FP_SELCODE_IS_VALID(selcode))

#define _FP_MAX_FPF_BREAK(_selcode, _stage_fc, _fpf_id) \
      if (_FP_SELCODE_IS_VALID(_selcode))  {           \
         if (NULL == (_stage_fc->_field_table_fpf ## _fpf_id[_selcode])) { \
             break; \
         } \
      }

#define _FP_SELCODE_SET(_selcode_qset, _selcode) \
    sal_memset((_selcode_qset), (int8)_selcode, sizeof(_field_sel_t));


/* 
 * Macros for packing and unpacking L3 information for field actions 
 * into and out of a uint32 value.
 */

#define MAX_CNT_BITS 10

#define _FP_L3_ACTION_PACK_ECMP(ecmp_ptr, ecmp_cnt) \
    ( 0x80000000 | ((ecmp_ptr) << MAX_CNT_BITS) | (ecmp_cnt) )
#define _FP_L3_ACTION_UNPACK_ECMP_MODE(cookie) \
    ( (cookie) >> 31 )
#define _FP_L3_ACTION_UNPACK_ECMP_PTR(cookie) \
    ( ((cookie) & 0x7fffffff) >> MAX_CNT_BITS )
#define _FP_L3_ACTION_UNPACK_ECMP_CNT(cookie) \
    ((cookie) & ~(0xffffffff << MAX_CNT_BITS))

#define _FP_L3_ACTION_PACK_NEXT_HOP(next_hop) \
    ( (next_hop) & 0x7fffffff )
#define _FP_L3_ACTION_UNPACK_NEXT_HOP(cookie) \
    ( (cookie) & 0x7fffffff )


#define _FP_MAX_ENTRY_WIDTH   4 /* Maximum field entry width. */

/*  Macros for packing and unpacking Eight Bytes Payload qualifier information */
#define _FP_PACK_L2_EIGHT_BYTES_PAYLOAD(qual_in_1, qual_in_2, qual_out) do {\
        (qual_out)[1] = (uint32) (qual_in_1); \
        (qual_out)[0] = (uint32) (qual_in_2); \
    } while (0) 

#define _FP_UNPACK_L2_EIGHT_BYTES_PAYLOAD(qual_in, qual_out_1, qual_out_2) do {\
        (*qual_out_1) = (uint32) ((qual_in)[1]); \
        (*qual_out_2) = (uint32) ((qual_in)[0]); \
    } while (0)

/* 
 * Macro: _BCM_FIELD_ENTRIES_IN_SLICE
 * 
 * Purpose:
 *     Returns number of entries in a slice, depending on mode
 *
 * Parameters:
 *     fg - pointer to entry's field group struct
 *     fs - pointer to entry's field slice struct
 *     result - return value
 */
#define _BCM_FIELD_ENTRIES_IN_SLICE(_fg_, _fs_, _result_)     \
    if ((_fg_)->flags & _FP_GROUP_INTRASLICE_DOUBLEWIDE) {    \
        (_result_) = ((_fs_)->entry_count >> 1);              \
    } else {                                                  \
        (_result_) = (_fs_)->entry_count;                     \
    }

/*
 * Macro:_FP_QUAL_INFO_SET
 *
 * Purpose:
 *    Set the fields of a _qual_info_t struct.
 */
#define _FP_QUAL_INFO_SET(qual, field, off, wid, fq)  \
    (fq)->qid    = (qual);                                    \
    (fq)->fpf    = (field);                                   \
    (fq)->offset = (off);                                     \
    (fq)->width  = (wid);                                     \
    (fq)->next   = NULL;
/*
 * Macro:
 *     _FIELD_SELCODE_CLEAR (internal)
 * Purpose:
 *     Set all fields of a field select code to invalid
 * Parameters:
 *     selcode - _field_selcode_t struct
 */

#define _FIELD_SELCODE_CLEAR(_selcode_)      \
                _FP_SELCODE_SET(&(_selcode_), _FP_SELCODE_DONT_CARE)

/*
 * Macro:
 *     _FIELD_NEED_I_WRITE
 * Purpose:
 *     Test if it is necessary to write to "I" version of FP_PORT_FIELD_SEL
 *     table.
 * Parameters:
 *     _u_ - BCM device number
 *     _p_ - port
 *     _m_ - Memory (usually IFP_PORT_FIELD_SEL)
 */
#define _FIELD_NEED_I_WRITE(_u_, _p_, _m_) \
     ((SOC_MEM_IS_VALID((_u_), (_m_))) && \
      (IS_XE_PORT((_u_), (_p_)) || IS_ST_PORT((_u_),(_p_)) || \
       IS_CPU_PORT((_u_),(_p_))))


#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)

/* Egress FP slice modes. */
#define  _BCM_FIELD_EGRESS_SLICE_MODE_SINGLE_L2      (0x0)
#define  _BCM_FIELD_EGRESS_SLICE_MODE_SINGLE_L3      (0x1)
#define  _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE         (0x2)
#define  _BCM_FIELD_EGRESS_SLICE_MODE_SINGLE_L3_ANY  (0x3)
#define  _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_ANY  (0x4)
#define  _BCM_FIELD_EGRESS_SLICE_MODE_DOUBLE_L3_V6   (0x5)


/* Egress FP slice v6 key  modes. */
#define  _BCM_FIELD_EGRESS_SLICE_V6_KEY_MODE_SIP6         (0x0)
#define  _BCM_FIELD_EGRESS_SLICE_V6_KEY_MODE_DIP6         (0x1)
#define  _BCM_FIELD_EGRESS_SLICE_V6_KEY_MODE_SIP_DIP_64   (0x2)



/* Egress FP possible keys. */
#define  _BCM_FIELD_EFP_KEY1                    (0x0)  /* IPv4 key.   */
#define  _BCM_FIELD_EFP_KEY2                    (0x1)  /* IPv6 key.   */
#define  _BCM_FIELD_EFP_KEY3                    (0x2)  /* IPv6 dw key.*/
#define  _BCM_FIELD_EFP_KEY4                    (0x3)  /* L2 key.     */
#define  _BCM_FIELD_EFP_KEY5                    (0x4)  /* Ip Any key. */
/* KEY_MATCH type field for EFP_TCAM encoding. for every key. */
#define  _BCM_FIELD_EFP_KEY1_MATCH_TYPE       (0x1)
#define  _BCM_FIELD_EFP_KEY2_MATCH_TYPE       (0x2)
#define  _BCM_FIELD_EFP_KEY2_KEY3_MATCH_TYPE  (0x3)
#define  _BCM_FIELD_EFP_KEY1_KEY4_MATCH_TYPE  (0x4)
#define  _BCM_FIELD_EFP_KEY4_MATCH_TYPE       (0x5)
#define  _BCM_FIELD_EFP_KEY2_KEY4_MATCH_TYPE  (0x6)
#define  _EFP_SLICE_MODE_L2_SINGLE_WIDE       (0)
#define  _EFP_SLICE_MODE_L3_SINGLE_WIDE       (0x1)
#define  _EFP_SLICE_MODE_L3_DOUBLE_WIDE       (0x2)
#define  _EFP_SLICE_MODE_L3_ANY_SINGLE_WIDE   (0x3)
#define  _EFP_SLICE_MODE_L3_ANY_DOUBLE_WIDE   (0x4)
#define  _EFP_SLICE_MODE_L3_ALT_DOUBLE_WIDE   (0x5)


/* Key Match Type Values */
#define KEY_TYPE_INVALID        0
#define KEY_TYPE_IPv4_SINGLE        1
#define KEY_TYPE_IPv6_SINGLE        2
#define KEY_TYPE_IPv6_DOUBLE        3
#define KEY_TYPE_IPv4_L2_L3_DOUBLE  4
#define KEY_TYPE_L2_SINGLE      5
#define KEY_TYPE_IPv4_IPv6_DOUBLE   6

#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */ 

#define FPF_SZ_MAX                           (32)

#define _FP_HASH_SZ(_fc_)   \
           (((_fc_)->flags & _FP_EXTERNAL_PRESENT) ? (0x1000) : (0x100))
#define _FP_HASH_INDEX_MASK(_fc_) (_FP_HASH_SZ(_fc_) - 1)

#define _FP_HASH_INSERT(_hash_ptr_, _inserted_ptr_, _index_)    \
           do {                                                 \
              (_inserted_ptr_)->next = (_hash_ptr_)[(_index_)]; \
              (_hash_ptr_)[(_index_)] = (_inserted_ptr_);       \
           } while (0)

#define _FP_HASH_REMOVE(_hash_ptr_, _entry_type_, _removed_ptr_, _index_)  \
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

#define _FP_ACTION_IS_COLOR_BASED(action)               \
     ((bcmFieldActionRpDrop <= (action) &&              \
      (action) <= bcmFieldActionYpPrioIntCancel) ||  \
      (bcmFieldActionGpDrop <= (action)                 \
       && (action) <= bcmFieldActionGpPrioIntCancel))

/* Different types of packet types for External TCAM */
#define _FP_EXT_NUM_PKT_TYPES 3
#define _FP_EXT_L2   0
#define _FP_EXT_IP4  1
#define _FP_EXT_IP6  2

/*
 * Different types of Databases in External TCAM (Triumph)
 * Each type of database is considered as a slice (in S/W)
 * The number of entries in each type of database is configurable
 * The field_slice_t.num_ext_entries denotes the number of entries
 *
 * See include/soc/er_tcam.h for the database types:
 * slice_numb    database type (soc_tcam_partition_type_t)   meaning
 *     0         TCAM_PARTITION_ACL_L2C                      ACL_144_L2
 *     1         TCAM_PARTITION_ACL_L2                       ACL_L2
 *     2         TCAM_PARTITION_ACL_IP4C                     ACL_144_IPV4
 *     3         TCAM_PARTITION_ACL_IP4                      ACL_IPV4
 *     4         TCAM_PARTITION_ACL_L2IP4                    ACL_L2_IPV4
 *     5         TCAM_PARTITION_ACL_IP6C                     ACL_144_IPV6
 *     6         TCAM_PARTITION_ACL_IP6S                     ACL_IPV6_SHORT
 *     7         TCAM_PARTITION_ACL_IP6F                     ACL_IPV6_FULL
 *     8         TCAM_PARTITION_ACL_L2IP6                    ACL_L2_IPV6
 */
#define _FP_EXT_ACL_144_L2     0
#define _FP_EXT_ACL_L2         1
#define _FP_EXT_ACL_144_IPV4   2
#define _FP_EXT_ACL_IPV4       3
#define _FP_EXT_ACL_L2_IPV4    4
#define _FP_EXT_ACL_144_IPV6   5
#define _FP_EXT_ACL_IPV6_SHORT 6
#define _FP_EXT_ACL_IPV6_FULL  7
#define _FP_EXT_ACL_L2_IPV6    8
#define _FP_EXT_NUM_PARTITIONS  9

extern soc_tcam_partition_type_t _bcm_field_fp_tcam_partitions[];
extern soc_mem_t _bcm_field_ext_data_mask_mems[];
extern soc_mem_t _bcm_field_ext_data_mems[];
extern soc_mem_t _bcm_field_ext_mask_mems[];
extern soc_mem_t _bcm_field_ext_policy_mems[];
extern soc_mem_t _bcm_field_ext_counter_mems[];

                                   
/*
 * Structure for Group auto-expansion feature
 *     S/W copy of FP_SLICE_MAP
 */

typedef struct _field_virtual_map_s {
    int valid;           /* Map is valid flag.                           */
    int vmap_key;        /* Key = Physical slice number + stage key base */
    int virtual_group;   /* Virtual group id.                            */
    int priority;        /* Virtual group priority.                      */
    int flags;           /* Field group flags.                           */
} _field_virtual_map_t;

/* Number of entries in a single map.*/
#define _FP_VMAP_SIZE       (17)
/* Default virtual map id.           */
#define _FP_VMAP_DEFAULT    (_FP_EXT_L2)
/* Number of virtual maps per stage.*/
#define _FP_VMAP_CNT        (_FP_EXT_NUM_PKT_TYPES)

/* Encode physical slice into vmap key. */
#define _FP_VMAP_IDX_TO_KEY(_slice_, _stage_)                          \
    (((_stage_) == _BCM_FIELD_STAGE_EXTERNAL) ?                        \
     ((_FP_VMAP_SIZE_) - 1 ) :  (_slice_))

/*
 * Typedef:
 *     _bcm_field_device_sel_t
 * Purpose:
 *     Enumerate per device controls used for qualifier selection. 
 */
typedef enum _bcm_field_device_sel_e {
    _bcmFieldDevSelDisable,          /* Don't care selection.            */
    _bcmFieldDevSelInnerVlanOverlay, /* Inner vlan overlay enable.       */
    _bcmFieldDevSelIntrasliceVfpKey, /* Intraslice vfp key select.       */
    _bcmFieldDevSelCount             /* Always Last. Not a usable value. */
} _bcm_field_device_sel_t;

/*
 * Typedef:
 *     _bcm_field_fwd_entity_sel_t
 * Purpose:
 *     Enumerate Forwarding Entity Type selection. 
 */
typedef enum _bcm_field_fwd_entity_sel_e {
    _bcmFieldFwdEntityAny,           /* Don't care selection.            */
    _bcmFieldFwdEntityMplsGport,     /* MPLS gport selection.            */
    _bcmFieldFwdEntityMimGport,      /* Mim gport selection.             */
    _bcmFieldFwdEntityWlanGport,     /* Wlan gport selection.            */
    _bcmFieldFwdEntityL3Egress,      /* Next hop selection.              */
    _bcmFieldFwdEntityGlp,           /* (MOD/PORT/TRUNK) selection.      */
    _bcmFieldFwdEntityModPortGport,  /* MOD/PORT even if port is part of 
                                        the trunk.                       */
    _bcmFieldFwdEntityMulticastGroup,/* Mcast group id selection.        */
    _bcmFieldFwdEntityMultipath,     /* Multpath id selection.           */    
    _bcmFieldFwdEntityPortGroupNum,  /* Ingress port group and number */
    _bcmFieldFwdEntityCommonGport,   /* Mod/Port gport or MPLS/MiM/Wlan
                                        gport selection.                 */
    _bcmFieldFwdEntityCount          /* Always Last. Not a usable value. */
} _bcm_field_fwd_entity_sel_t;

/*
 * Typedef:
 *     _bcm_field_fwd_field_sel_t
 * Purpose:
 *     Enumerate Forwarding Field Type selection. 
 */
typedef enum _bcm_field_fwd_field_sel_e {
    _bcmFieldFwdFieldAny,           /* Don't care selection.            */
    _bcmFieldFwdFieldVrf,           /* L3 Vrf id selection.             */
    _bcmFieldFwdFieldVpn,           /* VPLS/VPWS VFI selection.         */
    _bcmFieldFwdFieldVlan,          /* Forwarding vlan selection.       */
    _bcmFieldFwdFieldCount          /* Always Last. Not a usable value. */
} _bcm_field_fwd_field_sel_t;

/*
 * Typedef:
 *     _bcm_field_aux_tag_sel_t
 * Purpose:
 *     Enumerate Auxiliary Tag Type selection. 
 */
typedef enum _bcm_field_aux_tag_sel_e {
    _bcmFieldAuxTagAny,         /* Don't care selection.            */
    _bcmFieldAuxTagVn,          /* VN Tag selection.                */
    _bcmFieldAuxTagCn,          /* CN Tag selection.                */
    _bcmFieldAuxTagFabricQueue, /* Fabric Queue Tag selection.      */
    /* <HP>
       - Warm-start recovery
    */
    _bcmFieldAuxTagMplsFwdingLabel,
    _bcmFieldAuxTagMplsCntlWord,
    _bcmFieldAuxTagRtag7A,
    _bcmFieldAuxTagRtag7B,
    /* </HP> */
    _bcmFieldAuxTagVxlanNetworkId, /* 24 bit VNI                    */
    _bcmFieldAuxTagVxlanFlags,     /* Vxlan Flags.                  */
    _bcmFieldAuxTagLogicalLinkId,  /* 16 bit Logical Link ID      */
    _bcmFieldAuxTagCount        /* Always Last. Not a usable value. */
} _bcm_field_aux_tag_sel_t;

/*
 * Typedef:
 *     _bcm_field_slice_sel_t
 * Purpose:
 *     Enumerate per slice controls used for qualifier selection. 
 */
typedef enum _bcm_field_slice_sel_e {
    _bcmFieldSliceSelDisable,        /* Don't care selection.             */
    _bcmFieldSliceSelFpf1,           /* Field selector 1.                 */
    _bcmFieldSliceSelFpf2,           /* Field selector 2.                 */
    _bcmFieldSliceSelFpf3,           /* Field selector 3.                 */
    _bcmFieldSliceSelFpf4,           /* Field selector 3.                 */
    _bcmFieldSliceSelExternal,       /* Field selector External key.      */
    _bcmFieldSliceSrcClassSelect,    /* Field source class selector.      */
    _bcmFieldSliceDstClassSelect,    /* Field destination class selector. */
    _bcmFieldSliceIntfClassSelect,   /* Field interface class selector.   */
    _bcmFieldSliceIpHeaderSelect,    /* Inner/Outer Ip header selector.   */
    _bcmFieldSliceLoopbackTypeSelect,/* Loopback vs Tunnel type selector. */
    _bcmFieldSliceIngressEntitySelect,/* Ingress entity type selector.    */
    _bcmFieldSliceSrcEntitySelect,   /* Src entity type selector.    */    
    _bcmFieldSliceDstFwdEntitySelect, /* Destination entity type selector. */
    _bcmFieldSliceIp6AddrSelect,      /* Sip/Dip/Dip64_Sip64 selector.     */
    _bcmFieldSliceFwdFieldSelect,     /* Vrf/Vfi/Forwarding vlan selector. */
    _bcmFieldSliceAuxTag1Select,      /* VN/CN/FabricQueue tag selector 1. */
    _bcmFieldSliceAuxTag2Select,      /* VN/CN/FabricQueue tag selector 2. */
    _bcmFieldSliceFpf1SrcDstClassSelect,
    _bcmFieldSliceFpf3SrcDstClassSelect,
    _bcmFieldSliceSelEgrClassF1,
    _bcmFieldSliceSelEgrClassF2,
    _bcmFieldSliceSelEgrClassF3,
    _bcmFieldSliceSelEgrClassF4,
    _bcmFieldSliceSrcTypeSelect,
    _bcmFieldSliceSelEgrDvpKey4,      /* Selector for EFP_KEY4_DVP_SELECTORr */ 
    _bcmFieldSliceCount               /* Always Last. Not a usable value.  */
} _bcm_field_slice_sel_t;

/*
 * Typedef:
 *     _bcm_field_selector_t
 * Purpose:
 *     Select codes configuration required for qualifer.
 */
typedef struct _bcm_field_selector_s {
    _bcm_field_device_sel_t   dev_sel;         /* Per device Selector.     */
    uint8                     dev_sel_val;     /* Device selector value.   */
    _bcm_field_slice_sel_t    pri_sel;         /* Primary slice selector.  */
    uint8                     pri_sel_val;     /* Primary selector value.  */
    _bcm_field_slice_sel_t    sec_sel;         /* Secondary slice selector.*/
    uint8                     sec_sel_val;     /* Secondary selector value.*/
    _bcm_field_slice_sel_t    ter_sel;         /* Tertiary slice selector. */
    uint8                     ter_sel_val;     /* Tertiary selector value. */
    uint8                     intraslice;      /* Intra-slice double wide. */
    uint8                     secondary;
    uint8                     update_count;    /* Number of selcodes updates 
                                                  required for selection.*/
} _bcm_field_selector_t;

/*
 * Typedef:
 *     _bcm_field_qual_offset_t
 * Purpose:
 *     Qualifier offsets in FP tcam.
 */
typedef struct _bcm_field_qual_offset_s {
    soc_field_t field;     /* Field name.             */
    uint16      offset;    /* First qualifier offset  */
    uint8       width;     /* First offset width.     */
    uint16      offset1;   /* Second qualifier offset */
    uint8       width1;    /* Second offset width.    */
    uint16      offset2;   /* Third qualifier offset */
    uint8       width2;    /* Third offset width.    */
    uint8       secondary;
} _bcm_field_qual_offset_t;

/*
 * Typedef:
 *     _bcm_field_qual_data_t
 * Purpose:
 *     Qualifier data/mask field buffer.
 */
#define _FP_QUAL_DATA_WORDS  (4)
typedef uint32 _bcm_field_qual_data_t[_FP_QUAL_DATA_WORDS];   /* Qualifier data/mask buffer. */

#define _FP_QUAL_DATA_CLEAR(_data_) \
        sal_memset((_data_), 0, _FP_QUAL_DATA_WORDS * sizeof(uint32))


/*
 * Typedef:
 *     _bcm_field_qual_conf_s
 * Purpose:
 *     Select codes/offsets and other qualifier attributes.
 */
typedef struct _bcm_field_qual_conf_s {
    _bcm_field_selector_t     selector;    /* Per slice/device 
                                              configuration required. */
    _bcm_field_qual_offset_t  offset;      /* Qualifier tcam offsets. */
} _bcm_field_qual_conf_t;

/*
 * Typedef:
 *     _bcm_field_entry_part_qual_s
 * Purpose:
 *     Field entry part qualifiers information. 
 */
typedef struct _bcm_field_group_qual_s {
    uint16                   *qid_arr;    /* Qualifier id.           */
    _bcm_field_qual_offset_t *offset_arr; /* Qualifier tcam offsets. */
    uint16                   size;        /* Qualifier array size.   */
} _bcm_field_group_qual_t;

/* _FP_QUAL_* macros require the following declaration 
   in any function which uses them. 
 */
#define _FP_QUAL_DECL \
    int _rv_;         \
    _bcm_field_qual_conf_t _fp_qual_conf_; \
    soc_field_t _key_fld_ = SOC_IS_TD_TT(unit) ? KEYf : DATAf;
 
/* Add qualifier to stage qualifiers set. */
#define __FP_QUAL_EXT_ADD(_unit_, _stage_fc_, _qual_id_,                \
                        _dev_sel_, _dev_sel_val_,                             \
                        _pri_sel_, _pri_sel_val_,                             \
                        _sec_sel_, _sec_sel_val_,                             \
                          _ter_sel_, _ter_sel_val_,                     \
                          _intraslice_,                                 \
                          _offset_0_, _width_0_,                        \
                          _offset_1_, _width_1_,                        \
                         _offset_2_, _width_2_,                         \
                        _secondary_)                                          \
            do {                                                              \
                _bcm_field_qual_conf_t_init(&_fp_qual_conf_);                 \
                (_fp_qual_conf_).selector.dev_sel       = (_dev_sel_);        \
                (_fp_qual_conf_).selector.dev_sel_val   = (_dev_sel_val_);    \
                (_fp_qual_conf_).selector.pri_sel       = (_pri_sel_);        \
                (_fp_qual_conf_).selector.pri_sel_val   = (_pri_sel_val_);    \
                (_fp_qual_conf_).selector.sec_sel       = (_sec_sel_);        \
                (_fp_qual_conf_).selector.sec_sel_val   = (_sec_sel_val_);    \
        (_fp_qual_conf_).selector.ter_sel       = (_ter_sel_);          \
        (_fp_qual_conf_).selector.ter_sel_val   = (_ter_sel_val_);      \
                (_fp_qual_conf_).selector.intraslice    = (_intraslice_);     \
                (_fp_qual_conf_).offset.field          = (_key_fld_);         \
                (_fp_qual_conf_).offset.offset         = (_offset_0_);        \
                (_fp_qual_conf_).offset.offset1        = (_offset_1_);        \
                (_fp_qual_conf_).offset.offset2        = (_offset_2_);        \
                (_fp_qual_conf_).offset.width          = (_width_0_);         \
                (_fp_qual_conf_).offset.width1         = (_width_1_);         \
                (_fp_qual_conf_).offset.width2         = (_width_2_);         \
                (_fp_qual_conf_).offset.secondary      = (_secondary_);       \
                (_fp_qual_conf_).selector.secondary    = (_secondary_);       \
                _rv_ =_bcm_field_qual_insert((_unit_), (_stage_fc_),          \
                                             (_qual_id_), &(_fp_qual_conf_)); \
                BCM_IF_ERROR_RETURN(_rv_);                                    \
            } while (0)

#define _FP_QUAL_EXT_ADD(_unit_, _stage_fc_, _qual_id_,         \
                         _dev_sel_, _dev_sel_val_,              \
                         _pri_sel_, _pri_sel_val_,              \
                         _sec_sel_, _sec_sel_val_,              \
                         _intraslice_,                          \
                         _offset_0_, _width_0_,                 \
                         _offset_1_, _width_1_,                 \
                         _offset_2_, _width_2_,                 \
                         _secondary_)                           \
    __FP_QUAL_EXT_ADD((_unit_), (_stage_fc_), (_qual_id_),      \
                      (_dev_sel_), (_dev_sel_val_),             \
                      (_pri_sel_), (_pri_sel_val_),             \
                      (_sec_sel_), (_sec_sel_val_),             \
                      _bcmFieldDevSelDisable, 0,                \
                      (_intraslice_),                           \
                      (_offset_0_), (_width_0_),                \
                      (_offset_1_), (_width_1_),                \
                      (_offset_2_), (_width_2_),                \
                      (_secondary_)                             \
                      )

/* Add basic (single offset & single per slice selector. */
#define _FP_QUAL_ADD(_unit_, _stage_fc_, _qual_id_,                            \
                     _pri_slice_sel_, _pri_slice_sel_val_,                     \
                     _offset_0_, _width_0_)                                    \
                     _FP_QUAL_EXT_ADD((_unit_), (_stage_fc_),(_qual_id_),      \
                                      _bcmFieldDevSelDisable, 0,               \
                                      (_pri_slice_sel_), (_pri_slice_sel_val_),\
                                      _bcmFieldSliceSelDisable, 0, 0,          \
                                      (_offset_0_), (_width_0_), 0, 0, 0, 0, 0)

/* Add single offset & two slice selectors qualifier. */
#define _FP_QUAL_TWO_SLICE_SEL_ADD(_unit_, _stage_fc_, _qual_id_,              \
                                   _pri_slice_sel_, _pri_slice_sel_val_,       \
                                   _sec_slice_sel_, _sec_slice_sel_val_,       \
                                   _offset_0_, _width_0_)                      \
                     _FP_QUAL_EXT_ADD((_unit_), (_stage_fc_),(_qual_id_),      \
                                      _bcmFieldDevSelDisable, 0,               \
                                      (_pri_slice_sel_), (_pri_slice_sel_val_),\
                                      (_sec_slice_sel_), (_sec_slice_sel_val_),\
                                      0, (_offset_0_), (_width_0_), 0, 0, 0, 0, 0)

/* Add single offset & three slice selectors qualifier. */
#define _FP_QUAL_THREE_SLICE_SEL_ADD(_unit_, _stage_fc_, _qual_id_,     \
                                     _pri_slice_sel_, _pri_slice_sel_val_, \
                                     _sec_slice_sel_, _sec_slice_sel_val_, \
                                     _ter_slice_sel_, _ter_slice_sel_val_, \
                                     _offset_0_, _width_0_)             \
    __FP_QUAL_EXT_ADD((_unit_), (_stage_fc_),(_qual_id_),               \
                      _bcmFieldDevSelDisable, 0,                        \
                      (_pri_slice_sel_), (_pri_slice_sel_val_),         \
                      (_sec_slice_sel_), (_sec_slice_sel_val_),         \
                      (_ter_slice_sel_), (_ter_slice_sel_val_),         \
                      0, (_offset_0_), (_width_0_), 0, 0, 0, 0, 0)

/* Add single offset & two slice selectors qualifier. */
#define _FP_QUAL_INTRASLICE_ADD(_unit_, _stage_fc_, _qual_id_,                 \
                                _pri_slice_sel_, _pri_slice_sel_val_,          \
                                _sec_slice_sel_, _sec_slice_sel_val_,          \
                                _offset_0_, _width_0_)                         \
                     _FP_QUAL_EXT_ADD((_unit_), (_stage_fc_),(_qual_id_),      \
                                      _bcmFieldDevSelDisable, 0,               \
                                      (_pri_slice_sel_), (_pri_slice_sel_val_),\
                                      (_sec_slice_sel_), (_sec_slice_sel_val_),\
                                      0x1, (_offset_0_), (_width_0_), 0, 0, 0, 0, 0)
/*
 * Typedef:
 *     _bcm_field_qual_t
 * Purpose:
 *          
 */
typedef struct _bcm_field_qual_info_s {
    uint16                 qid;          /* Qualifer id.               */
    _bcm_field_qual_conf_t *conf_arr;    /* Configurations array.      */
    uint8                  conf_sz;      /* Configurations array size. */                   
} _bcm_field_qual_info_t;

/*
 * Typedef:
 *     _qual_info_t
 * Purpose:
 *     Holds format info for a particular qualifier's access parameters. These
 *     nodes are stored in qualifier lists for groups and in the FPFx tables
 *     for each chip.
 */
typedef struct _qual_info_s {
    int                    qid;     /* Which Qualifier              */
    soc_field_t            fpf;     /* FPFx field choice            */
    int                    offset;  /* Offset within FPFx field     */
    int                    width;   /* Qual width within FPFx field */
    struct _qual_info_s    *next;
} _qual_info_t;

/* Typedef:
 *     _field_fpf_info_t
 * Purpose:
 *     Hardware memory details of field qualifier 
 */
typedef struct _field_fpf_info_s {
    _qual_info_t      **qual_table;
    bcm_field_qset_t  *sel_table;
    soc_field_t       field;
} _field_fpf_info_t;

/*
 * Struct:
 *     _field_counter32_collect_s
 * Purpose:
 *     Holds the accumulated count of FP Counters 
 *         Useful for wrap around and movement.
 *     This is used when h/w counter width is <= 32 bits
 */
typedef struct _field_counter32_collect_s {
    uint64 accumulated_counter;
    uint32 last_hw_value;
} _field_counter32_collect_t;

/*
 * Struct:
 *     _field_counter64_collect_s
 * Purpose:
 *     Holds the accumulated count of FP Counters 
 *         Useful for wrap around and movement.
 *     This is used when h/w counter width is > 32 bits
 *         e.g. Bradley, Triumph Byte counters
 */
typedef struct _field_counter64_collect_s {
    uint64 accumulated_counter;
    uint64 last_hw_value;
} _field_counter64_collect_t;

/*
 * Typedef:
 *     _field_meter_bmp_t
 * Purpose:
 *     Meter bit map for tracking allocation state of group's meter pairs.
 */
typedef struct _field_meter_bmp_s {
    SHR_BITDCL  *w;
} _field_meter_bmp_t;

#define _FP_METER_BMP_FREE(bmp, size)   sal_free((bmp).w)
#define _FP_METER_BMP_ADD(bmp, mtr)     SHR_BITSET(((bmp).w), (mtr))
#define _FP_METER_BMP_REMOVE(bmp, mtr)  SHR_BITCLR(((bmp).w), (mtr))
#define _FP_METER_BMP_TEST(bmp, mtr)    SHR_BITGET(((bmp).w), (mtr))


/*
 * struct:
 *     _field_meter_pool_s
 * Purpose:
 *     Global meter pool
 * Note:
 *     Note that the 2nd parameter is slice_id and not group_id. This 
 *     is because more than one group can share the same slice (pbmp based
 *     groups)
 *
 *     Due to this, it needs to be enforced that for an entry in a group
 *     which is not in the group's first slice (due to group auto-expansion/
 *     virtual groups), slice_id denotes the entry's group's first slice.
 */
typedef struct _field_global_meter_pool_s {
    uint8              level;       /* Level meter pool attached.            */
    int                slice_id;    /* First slice which uses this meter pool*/
    uint16             size;        /* Number of valid meters in pool.       */
    uint16             pool_size;   /* Total number of meters in pool.       */
    uint16             free_meters; /* Number of free meters                 */
    uint16             num_meter_pairs; /* Number of meter pairs in a pool.  */
    _field_meter_bmp_t meter_bmp;       /* Meter usage bitmap                */
} _field_meter_pool_t;


typedef struct _field_global_counter_pool_s {
    int8               slice_id;   /* First slice which uses this pool */
    uint16             size;       /* Number of counters in pool */
    uint16             free_cntrs; /* Number of free counters */
    _field_meter_bmp_t cntr_bmp;   /* Counter usage bitmap */
} _field_cntr_pool_t;


/*
 * DATA qualifiers section. 
 */

/* Instead of the old model of n sets of m 4-consecutive-byte chunks of ingress
   packet (currently: n = 2 and m = 4), consider all user-defined 4-byte chunks
   of ingress packet data presented to the FP as equivalent -- the only proviso
   is that any one data qualifier cannot use chunks that are in different "udf
   numbers" (since those are in different FP selectors).
   So, call the index of a 4-byte chunk in the set of (n*m) of them the "data
   qualifier index".  This is also the "column index" in the FP_UDF_OFFSET
   table. An API-level data qualifier that uses multiple 4-byte
   chunks will be assigned a data qualifier index (see _field_data_qualifier_t)
   based on the lowest numbered chunk index it uses.
*/
#define BCM_FIELD_USER_DQIDX(udf_num, user_num) \
    ((udf_num) * (BCM_FIELD_USER_MAX_USER_NUM + 1) + (user_num))
/* Maximum number of data qualifiers. */
#define BCM_FIELD_USER_MAX_DQS \
    BCM_FIELD_USER_DQIDX((BCM_FIELD_USER_MAX_UDF_NUM + 1), 0)
#define BCM_FIELD_USER_DQIDX_TO_UDF_NUM(dqidx)   ((dqidx) / (BCM_FIELD_USER_MAX_USER_NUM + 1))
#define BCM_FIELD_USER_DQIDX_TO_USER_NUM(dqidx)  ((dqidx) % (BCM_FIELD_USER_MAX_USER_NUM + 1))

/* Maximum packet depth data qualifier can qualify */
#define _FP_DATA_OFFSET_MAX      (128)

/* Maximum packet depth data qualifier can qualify */
#define _FP_DATA_WORDS_COUNT         (8)

/* Number of UDF_OFFSETS allocated for ethertype based qualifiers. */
#define _FP_DATA_ETHERTYPE_MAX              (8)
/* Number of UDF_OFFSETS allocated for IPv4 protocol based qualifiers. */
#define _FP_DATA_IP_PROTOCOL_MAX            (2)
/* Number of UDF_OFFSETS allocated for IPv6 next header based qualifiers. */
#define _FP_DATA_NEXT_HEADER_MAX            (2)

/* Hw specific config.*/
#define _FP_DATA_DATA0_WORD_MIN     (0)
#define _FP_DATA_DATA0_WORD_MAX     (3)
#define _FP_DATA_DATA1_WORD_MIN     (4)
#define _FP_DATA_DATA1_WORD_MAX     (7)

/* Ethertype match L2 format. */
#define  _BCM_FIELD_DATA_FORMAT_ETHERTYPE (0x3)

/* Ip protocol match . */
#define  _BCM_FIELD_DATA_FORMAT_IP4_PROTOCOL0 (BCM_FIELD_DATA_FORMAT_IP6 + 0x1)
#define  _BCM_FIELD_DATA_FORMAT_IP4_PROTOCOL1 (BCM_FIELD_DATA_FORMAT_IP6 + 0x2)
#define  _BCM_FIELD_DATA_FORMAT_IP6_PROTOCOL0 (BCM_FIELD_DATA_FORMAT_IP6 + 0x3)
#define  _BCM_FIELD_DATA_FORMAT_IP6_PROTOCOL1 (BCM_FIELD_DATA_FORMAT_IP6 + 0x4)

/* Packet content (data) qualification object flags. */
#define _BCM_FIELD_DATA_QUALIFIER_FLAGS                       \
     (BCM_FIELD_DATA_QUALIFIER_OFFSET_IP4_OPTIONS_ADJUST |    \
      BCM_FIELD_DATA_QUALIFIER_OFFSET_IP6_EXTENSIONS_ADJUST | \
      BCM_FIELD_DATA_QUALIFIER_OFFSET_GRE_OPTIONS_ADJUST |    \
      BCM_FIELD_DATA_QUALIFIER_OFFSET_FLEX_HASH |             \
      BCM_FIELD_DATA_QUALIFIER_STAGE_LOOKUP)

/* UDF spec - offset is valid flag   */
#define _BCM_FIELD_USER_OFFSET_VALID   (1 << 31)

#define _BCM_FIELD_USER_OFFSET_FLAGS           \
            (_BCM_FIELD_USER_OFFSET_VALID |    \
              BCM_FIELD_USER_OPTION_ADJUST |   \
              BCM_FIELD_USER_GRE_OPTION_ADJUST)

/* Data qualifiers tcam priorities. */
 /* Specified packet l2 format. */
#define _FP_DATA_QUALIFIER_PRIO_L2_FORMAT   (0x01)
 /* Specified number of vlan tags. */
#define _FP_DATA_QUALIFIER_PRIO_VLAN_FORMAT (0x01)
 /* Specified l3 format tunnel types/inner/outer ip type. */
#define _FP_DATA_QUALIFIER_PRIO_L3_FORMAT   (0x01)
 /* Specified MPLS format tunnel. */
#define _FP_DATA_QUALIFIER_PRIO_MPLS_FORMAT (0x01)
 /* Specified FCoE format tunnel. */
#define _FP_DATA_QUALIFIER_PRIO_FCOE_FORMAT (0x01)
 /* Specified ip protocol.  */
#define _FP_DATA_QUALIFIER_PRIO_IPPROTO     (0x01)
 /* Specified ip protocol/ethertype.  */
#define _FP_DATA_QUALIFIER_PRIO_MISC        (0x01)
 /* Maximum udf tcam entry priority.  */
#define _FP_DATA_QUALIFIER_PRIO_HIGHEST     (0xff)

/* MPLS data qualifier label priority */
#define _FP_DATA_QUALIFIER_PRIO_MPLS_ANY        (0x00)
#define _FP_DATA_QUALIFIER_PRIO_MPLS_ONE_LABEL  (0x01)
#define _FP_DATA_QUALIFIER_PRIO_MPLS_TWO_LABEL  (0x02)

/*
 * Typedef:
 *     _field_data_qualifier_s
 * Purpose:
 *     Data qualifiers description structure.
 */
typedef struct _field_data_qualifier_s {
    int    qid;                     /* Qualifier id.                     */
    bcm_field_udf_spec_t *spec;     /* Hw adjusted offsets array.        */
    bcm_field_data_offset_base_t offset_base; /* Offset base adjustment. */
    int    offset;                  /* Master word offset.               */
    uint8  byte_offset;             /* Data offset inside the word.      */
    uint32 hw_bmap;                 /* Allocated hw words.               */
    uint32 flags;                   /* Offset adjustment flags.          */
    uint8  elem_count;              /* Number of hardware elements required. */
    int    length;                  /* Matched data byte length.         */
    struct _field_data_qualifier_s *next;/* Next installed  qualifier.   */
} _field_data_qualifier_t, *_field_data_qualifier_p;

/*
 * Typedef:
 *     _field_data_ethertype_s
 * Purpose:
 *     Ethertype based data qualifiers description structure.
 */
typedef struct _field_data_ethertype_s {
    int ref_count;                  /* Reference count.        */
    uint16 l2;                      /* Packet l2 format.       */
    uint16 vlan_tag;                /* Packet vlan tag format. */
    bcm_port_ethertype_t ethertype; /* Ether type value.       */
    int relative_offset;            /* Packet byte offset      */
                                    /* relative to qualifier   */
                                    /* byte offset.            */
} _field_data_ethertype_t;

/*
 * Typedef:
 *     _field_data_protocol_s
 * Purpose:
 *     Protocol based data qualifiers description structure.
 */
typedef struct _field_data_protocol_s {
    int ip4_ref_count;    /* Ip4 Reference count.    */
    int ip6_ref_count;    /* Ip6 Reference count.    */
    uint32 flags;         /* Ip4/Ip6 flags.          */
    uint8 ip;             /* Ip protocol id.         */
    uint16 l2;            /* Packet l2 format.       */
    uint16 vlan_tag;      /* Packet vlan tag format. */
    int relative_offset;  /* Packet byte offset      */
                          /* relative to qualifier   */
                          /* byte offset.            */
} _field_data_protocol_t;

/*
 * Typedef:
 *     _field_data_tcam_entry_s
 * Purpose:
 *     Field data tcam entry structucture. Used for      
 *     tcam entry allocation and organization by relative priority.
 */
typedef struct _field_data_tcam_entry_s {
   uint8 ref_count;           /* udf tcam entry reference count.  */
   uint8 priority;            /* udf tcam entry priority.         */
} _field_data_tcam_entry_t;

/*
 * Typedef:
 *     _field_data_control_s
 * Purpose:
 *     Field data qualifiers control structucture.     
 *        
 */
typedef struct _field_data_control_s {
   uint32 usage_bmap;                 /* Offset usage bitmap.          */
   _field_data_qualifier_p data_qual; /* List of data qualifiers.      */
                                      /* Ethertype based qualifiers.   */
   _field_data_ethertype_t etype[_FP_DATA_ETHERTYPE_MAX];
                                      /* IP protocol based qualifiers. */
   _field_data_protocol_t  ip[_FP_DATA_IP_PROTOCOL_MAX];
   _field_data_tcam_entry_t *tcam_entry_arr;/* Tcam entries/keys array.*/
   int                     elem_size; /* Number of bytes per element. */
   int                     num_elems; /* Number of elements per UDF. */
} _field_data_control_t;

/*
 * Stage flags. 
 */

/* Separate packet byte counters. */
#define _FP_STAGE_SEPARATE_PACKET_BYTE_COUNTERS (1 << 0)

/* Global meter pools . */
#define _FP_STAGE_GLOBAL_METER_POOLS            (1 << 1)

/* Global counters. */
#define _FP_STAGE_GLOBAL_COUNTERS               (1 << 2)

/* Auto expansion support. */
#define _FP_STAGE_AUTO_EXPANSION                (1 << 3)

/* Slice enable/disable support. */
#define _FP_STAGE_SLICE_ENABLE                  (1 << 4)

/* Only first half of slice is valid.. */
#define _FP_STAGE_HALF_SLICE                    (1 << 5)

/* Global counter pools . */
#define _FP_STAGE_GLOBAL_CNTR_POOLS             (1 << 6)

/* Only first quarter of slice is valid.. */
#define _FP_STAGE_QUARTER_SLICE                 (1 << 7)

/*
 * Typedef:
 *     _field_stage_t
 * Purpose:
 *     Pipeline stage field processor information.
 */
typedef struct _field_stage_s {
    _field_stage_id_t      stage_id;        /* Pipeline stage id.           */
    uint8                  flags;           /* Stage flags.                 */
    int                    tcam_sz;         /* Number of entries in TCAM.   */
    int                    tcam_slices;     /* Number of internal slices.   */
    struct _field_slice_s  *slices;         /* Array of slices.*/
    struct _field_range_s  *ranges;         /* List of all ranges allocated.*/
    uint32                 range_id;        /* Seed ID for range creation.  */
                                        /* Virtual map for slice extension */
                                        /* and group priority management.  */
    _field_virtual_map_t   vmap[_FP_VMAP_CNT][_FP_VMAP_SIZE];
    
    /* FPF tables */
    _qual_info_t *_field_table_fpf0[FPF_SZ_MAX];
    _qual_info_t *_field_table_fpf1[FPF_SZ_MAX];
    _qual_info_t *_field_table_fpf2[FPF_SZ_MAX];
    _qual_info_t *_field_table_fpf3[FPF_SZ_MAX];
    _qual_info_t *_field_table_fpf4[FPF_SZ_MAX];
    _qual_info_t *_field_table_extn[FPF_SZ_MAX];
    _qual_info_t *_field_table_doublewide_fpf[FPF_SZ_MAX];
    _qual_info_t *_field_table_fixed[1];

    bcm_field_qset_t *_field_sel_f0;
    bcm_field_qset_t *_field_sel_f1;
    bcm_field_qset_t *_field_sel_f2;
    bcm_field_qset_t *_field_sel_f3;
    bcm_field_qset_t *_field_sel_f4;
    bcm_field_qset_t *_field_sel_extn;
    bcm_field_qset_t *_field_sel_doublewide;
    bcm_field_qset_t *_field_sel_fixed;
    bcm_field_qset_t _field_supported_qset;
    _bcm_field_qual_info_t  **f_qual_arr;  /* Stage qualifiers config array. */
                                            

    int counter_collect_table; /* Used for counter collection in chunks */
    int counter_collect_index; /* Used for counter collection in chunks */
    
    int num_meter_pools;
    _field_meter_pool_t *meter_pool[_FIELD_MAX_METER_POOLS];

    unsigned num_cntr_pools;
    _field_cntr_pool_t *cntr_pool[_FIELD_MAX_CNTR_POOLS];

    _field_counter32_collect_t *_field_x32_counters;
                              /* X pipeline 32 bit counter collect      */
#if defined(BCM_EASYRIDER_SUPPORT) || defined(BCM_TRIUMPH_SUPPORT)
    _field_counter32_collect_t *_field_ext_counters; 
                              /* External counter collect */
#endif /* BCM_EASYRIDER_SUPPORT || BCM_TRIUMPH_SUPPORT */

#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRX_SUPPORT)
    _field_counter64_collect_t *_field_x64_counters; 
                              /* X pipeline 64 bit counter collect */
    _field_counter64_collect_t *_field_y64_counters;
                              /* Y pipeline 64 bit counter collect  */
#endif /* BCM_BRADLEY_SUPPORT || BCM_SCORPION_SUPPORT */

#if defined(BCM_SCORPION_SUPPORT)
    _field_counter32_collect_t *_field_y32_counters;
                              /* Y pipeline packet counter collect */
#endif /* BCM_SCORPION_SUPPORT */

    soc_memacc_t               *_field_memacc_counters;
                              /* Memory access info for FP counter fields */

    soc_profile_mem_t redirect_profile;     /* Redirect action memory profile.*/
#if defined(BCM_TRIDENT2_SUPPORT)
    soc_profile_mem_t hash_select[2];    /* Hash select action memory profile */
#endif
    soc_profile_mem_t ext_act_profile;      /* Action profile for external.   */
    _field_data_control_t *data_ctrl;       /* Data qualifiers control.       */

    struct _field_stage_s *next;            /* Next pipeline FP stage.        */
} _field_stage_t;

/* Indexes into the memory access list for FP counter acceleration */
typedef enum _field_counters_memacc_type_e {
    _FIELD_COUNTER_MEMACC_BYTE,
    _FIELD_COUNTER_MEMACC_PACKET,
    _FIELD_COUNTER_MEMACC_BYTE_Y,
    _FIELD_COUNTER_MEMACC_PACKET_Y,
    _FIELD_COUNTER_MEMACC_NUM,       /* The max size of the memacc list */
    /* If the device doesn't have packet and byte counters in a single
     * counter mem entry, then these aliases are used. */
    _FIELD_COUNTER_MEMACC_COUNTER = _FIELD_COUNTER_MEMACC_BYTE,
    _FIELD_COUNTER_MEMACC_COUNTER_Y = _FIELD_COUNTER_MEMACC_PACKET
} _field_counters_memacc_type_t;

#define _FIELD_FIRST_MEMORY_COUNTERS                 (1 << 0)
#define _FIELD_SECOND_MEMORY_COUNTERS                (1 << 1)

#define FILL_FPF_TABLE(_fpf_info, _qid, _offset, _width, _code)        \
    BCM_IF_ERROR_RETURN                                                \
        (_field_qual_add((_fpf_info), (_qid), (_offset), (_width), (_code)))

/* Generic memory allocation routine. */
#define _FP_XGS3_ALLOC(_ptr_,_size_,_descr_)                 \
            do {                                             \
                if (NULL == (_ptr_)) {                       \
                   (_ptr_) = sal_alloc((_size_), (_descr_)); \
                }                                            \
                if((_ptr_) != NULL) {                        \
                    sal_memset((_ptr_), 0, (_size_));        \
                }  else {                                    \
                    FP_ERR(("FP Error: Allocation failure %s\n", (_descr_))); \
                }                                          \
            } while (0)

/*
 * Typedef:
 *     _field_udf_t
 * Purpose:
 *     Holds user-defined field (UDF) hardware metadata. 
 */
typedef struct _field_udf_s {
    uint8                  valid;     /* Indicates valid UDF             */
    int                    use_count; /* Number of groups using this UDF */
    bcm_field_qualify_t    udf_num;   /* UDFn (UDF0 or UDF1)             */
    uint8                  user_num;  /* Which user field in UDFn        */
} _field_udf_t;


/*
 * Typedef:
 *     _field_tcam_int_t
 * Purpose:
 *     These are the fields that are written into or read from FP_TCAM_xxx.
 */
typedef struct _field_tcam_s {
    uint32                 *key;
    uint32                 *key_hw;  /* Hardware replica */
    uint32                 *mask;
    uint32                 *mask_hw; /* Hardware replica */
    uint16                 key_size;
    uint32                 f4;         
    uint32                 f4_mask;    
    uint8                  higig;         /* 0/1 non-HiGig/HiGig    */
    uint8                  higig_mask;    /* 0/1 non-HiGig/HiGig    */
     uint8                 ip_type;
#if defined(BCM_FIREBOLT2_SUPPORT)
    uint8                  drop;         /* 0/1 don't Drop/Drop     */
    uint8                  drop_mask;    /* 0/1 don't Drop/Drop     */
#endif /* BCM_FIREBOLT2_SUPPORT */
} _field_tcam_t;

/*
 * Typedef:
 *     _field_tcam_mem_info_t
 * Purpose:
 *     TCAM memory name and chip specific field names. 
 */
typedef struct _field_tcam_mem_info_s{
    soc_mem_t      memory;     /* Tcam memory name.    */  
    soc_field_t    key_field;  /* Tcam key field name. */
    soc_field_t    mask_field; /* Tcam mask field name.*/
} _field_tcam_mem_info_t;

#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_TRX_SUPPORT)
typedef struct _field_pbmp_s {
    bcm_pbmp_t data;
    bcm_pbmp_t mask;
} _field_pbmp_t;
#endif /* BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */

/*
 * Typedef:
 *     _field_counter_t
 * Purpose:
 *     Holds the counter parameters to be written into FP_POLICY_TABLE
 *     (Firebolt) or FP_INTERNAL (Easyrider).
 */
typedef struct _field_counter_s {
    int                    index;
    uint16                 entries;    /* Number of entries using counter */
} _field_counter_t;

/*
 * Typedef:
 *     _field_counter_bmp_t
 * Purpose:
 *     Counter bit map for tracking allocation state of slice's counter pairs.
 */
typedef struct _field_counter_bmp_s {
    SHR_BITDCL  *w;
} _field_counter_bmp_t;

#define _FP_COUNTER_BMP_FREE(bmp, size)   sal_free((bmp).w)
#define _FP_COUNTER_BMP_ADD(bmp, ctr)     SHR_BITSET(((bmp).w), (ctr))
#define _FP_COUNTER_BMP_REMOVE(bmp, ctr)  SHR_BITCLR(((bmp).w), (ctr))
#define _FP_COUNTER_BMP_TEST(bmp, ctr)    SHR_BITGET(((bmp).w), (ctr))


/* 
 * Structure for priority management 
 * Currently used only on External TCAM
 */
typedef struct _field_prio_mgmt_s {
    int prio;
    uint32 start_index;
    uint32 end_index;
    uint32 num_free_entries;
    struct _field_prio_mgmt_s *prev;
    struct _field_prio_mgmt_s *next;
} _field_prio_mgmt_t;

/* Slice specific flags. */
#define _BCM_FIELD_SLICE_EXTERNAL                 (1 << 0)
#define _BCM_FIELD_SLICE_INTRASLICE_CAPABLE       (1 << 1)
#define _BCM_FIELD_SLICE_SIZE_SMALL               (1 << 2)
#define _BCM_FIELD_SLICE_SIZE_LARGE               (1 << 3)

#define _FP_INTRA_SLICE_PART_0        (0)
#define _FP_INTRA_SLICE_PART_1        (1)
#define _FP_INTRA_SLICE_PART_2        (2)
#define _FP_INTRA_SLICE_PART_3        (3)

#define _FP_INTER_SLICE_PART_0        (0)
#define _FP_INTER_SLICE_PART_1        (1)
#define _FP_INTER_SLICE_PART_2        (2)

/*
 * Typedef:
 *     _field_slice_t
 * Purpose:
 *     This has the fields specific to a hardware slice.
 * Notes:
 */
typedef struct _field_slice_s {
    uint8                  slice_number;  /* Hardware slice number.         */
    int                    start_tcam_idx;/* Slice first entry tcam index.  */
    int                    entry_count;   /* Number of entries in the slice.*/
    int                    free_count;    /* Number of free entries.        */
    int                    counters_count;/* Number of counters accessible. */
    int                    meters_count;  /* Number of meters accessible.   */

    _field_counter_bmp_t   counter_bmp;   /* Bitmap for counter allocation. */
    _field_meter_bmp_t     meter_bmp;     /* Bitmap for meter allocation.   */
    _field_stage_id_t      stage_id;      /* Pipeline stage slice belongs.  */
    bcm_pbmp_t             pbmp;          /* Ports in use by groups.        */
     
    struct _field_entry_s  **entries;     /* List of entries pointers.      */
    _field_prio_mgmt_t     *prio_mgmt;    /* Priority management of entries.*/

    uint8 pkt_type[_FP_EXT_NUM_PKT_TYPES];/* Packet types supported 
                                             by this slice (aka database).  */

    bcm_pbmp_t ext_pbmp[_FP_EXT_NUM_PKT_TYPES];/* Bmap for each packet type.*/

    struct _field_slice_s  *next;  /* Linked list for auto-expand of groups.*/
    struct _field_slice_s  *prev;  /* Linked list for auto-expand of groups.*/
    uint8                  slice_flags;   /* _BCM_FIELD_SLICE_XXX flags.    */
    uint8                  group_flags;   /* Intalled groups _FP_XXX_GROUP. */
    int8                   doublewide_key_select; 
                                          /* Key selection for the          */
                                          /* intraslice double wide mode.   */
    int8                   src_class_sel; /* Source lookup class selection.*/        
    int8                   dst_class_sel; /* Destination lookup class.     */         
    int8                   intf_class_sel;/* Interface class selection.    */
    int8                   loopback_type_sel;/* Loopback/Tunnel selection.  */
    int8                   ingress_entity_sel;/* Ingress entity selection.  */
    int8                   src_entity_sel;    /* Src port/trunk entity selection.          */        
    int8                   dst_fwd_entity_sel;/* Destination forwarding     */
    int8                   fwd_field_sel; /* Forwarding field vrf/vpn/vid   */
                                              /* entity selection.          */
    int8                   aux_tag_1_sel;
    int8                   aux_tag_2_sel;
    int8                   egr_class_f1_sel;
    int8                   egr_class_f2_sel;
    int8                   egr_class_f3_sel;
    int8                   egr_class_f4_sel;
    int8                   src_dest_class_f1_sel;
    int8                   src_dest_class_f3_sel;
    int8                   egr_key4_dvp_sel;
} _field_slice_t;


/* Macro: _BCM_FIELD_SLICE_SIZE
 * Purpose:
 *        Given stage, slice get number of entries in the slice.
 */
#define _BCM_FIELD_SLICE_SIZE(_stage_fc_, _slice_)     \
       (((_stage_fc_)->slices + (_slice_))->entry_count)


#define _FP_GROUP_ENTRY_ARR_BLOCK (0xff)
typedef struct _field_entry_s _field_entry_t;
/*
 * Typedef:
 *     _field_group_t
 * Purpose:
 *     This is the logical group's internal storage structure. It has 1, 2 or
 *     3 slices, each with physical entry structures.
 * Notes:
 *   'ent_qset' should always be a subset of 'qset'.
 */
typedef struct _field_group_s {
    bcm_field_group_t      gid;            /* Opaque handle.                */
    int                    priority;       /* Field group priority.         */
    bcm_field_qset_t       qset;           /* This group's Qualifier Set.   */
    uint16                 flags;          /* Group configuration flags.    */
    _field_slice_t         *slices;        /* Pointer into slice array.     */
    bcm_pbmp_t             pbmp;           /* Ports in use this group.      */
    _field_sel_t           sel_codes[_FP_MAX_ENTRY_WIDTH];
                                           /* Select codes for slice(s).    */
    _bcm_field_group_qual_t qual_arr[_FP_MAX_ENTRY_WIDTH];
                                           /* Qualifiers available in each 
                                              individual entry part.        */
    _field_stage_id_t      stage_id;       /* FP pipeline stage id.         */

    _field_entry_t         **entry_arr;    /* FP group entry array.         */
    uint16                 ent_block_count;/* FP group entry array size  .  */
    /*
     * Public data for each group: The number of used and available entries,
     * counters, and meters for a field group.
     */
    bcm_field_group_status_t group_status;
    bcm_field_aset_t         aset;
    struct _field_group_s  *next;         /* For storing in a linked-list  */
} _field_group_t;

/* Katana2 can support 170 pp_ports */
#define _FP_ACTION_PARAM_SZ             (6)
/*
 * Typedef:
 *     _field_action_t
 * Purpose:
 *     This is the real action storage structure that is hidden behind
 *     the opaque handle bcm_field_action_t.
 */
typedef struct _field_action_s {
    bcm_field_action_t     action;       /* Action type                  */
    uint32                 param[_FP_ACTION_PARAM_SZ];     
                                         /* Action specific parameters   */
    int                    hw_index;     /* Allocated hw resource index. */
    int                    old_index;    /* Hw resource to be freed, in  */ 
                                         /* case action parameters were  */ 
                                         /* modified.                    */
    uint8                  flags;        /* Field action flags.          */
    struct _field_action_s *next;
} _field_action_t;

#define _FP_RANGE_STYLE_FIREBOLT    1


#define PolicyMax(_unit_, _mem_, _field_)                                 \
    ((soc_mem_field_length((_unit_), (_mem_) , (_field_)) < 32) ?         \
        ((1 << soc_mem_field_length((_unit_), (_mem_), (_field_))) - 1) : \
        (0xFFFFFFFFUL)) 

#define PolicyGet(_unit_, _mem_, _entry_, _field_) \
    soc_mem_field32_get((_unit_), (_mem_), (_entry_), (_field_))

#define PolicySet(_unit_, _mem_, _entry_, _field_, _value_)    \
    soc_mem_field32_set((_unit_), (_mem_), (_entry_), (_field_), (_value_))

#define PolicyCheck(_unit_, _mem_, _field_, _value_)                      \
    if (0 == ((uint32)(_value_) <=                                        \
              (uint32)PolicyMax((_unit_), (_mem_), (_field_)))) {         \
        FP_ERR(("FP(unit %d) Error: Policy _value_ %d > %d (max) mem (%d) \
                field (%d).\n", _unit_, (_value_),                        \
                (uint32)PolicyMax((_unit_), (_mem_), (_field_)), (_mem_), \
                (_field_)));                                              \
        return (BCM_E_PARAM);                                             \
    }

/*
 * Typedef:
 *     _field_range_t
 * Purpose:
 *     Internal management of Range Checkers. There are two styles or range
 *     checkers, the Firebolt style that only chooses source or destination
 *     port, without any sense of TCP vs. UDP or inverting. This style writes
 *     into the FP_RANGE_CHECK table. The Easyrider style is able to specify
 *     TCP vs. UDP.
 *     The choice of styles is based on the user supplied flags.
 *     If a Firebolt style checker is sufficient, that will be used. If an
 *     Easyrider style checker is required then that will be used.
 *
 */
typedef struct _field_range_s {
    uint32                 flags;
    bcm_field_range_t      rid;
    bcm_l4_port_t          min, max;
    int                    hw_index;
    uint8                  style;        /* Simple (FB) or more complex (ER) */
    struct _field_range_s *next;
} _field_range_t;

/*
 * Entry status flags. 
 */

/* Software entry differs from one in hardware. */
#define _FP_ENTRY_DIRTY                      (1 << 0)

/* Entry is in primary slice. */
#define _FP_ENTRY_PRIMARY                    (1 << 1)     

/* Entry is in secondary slice of wide-mode group. */
#define _FP_ENTRY_SECONDARY                  (1 << 2)

/* Entry is in tertiary slice of wide-mode group. */
#define _FP_ENTRY_TERTIARY                   (1 << 3)

/* Entry has an ingress Mirror-To-Port reserved. */
#define _FP_ENTRY_MTP_ING0                   (1 << 4)

/* Entry has an ingress 1 Mirror-To-Port reserved. */
#define _FP_ENTRY_MTP_ING1                   (1 << 5)

/* Entry has an egress Mirror-To-Port reserved. */
#define _FP_ENTRY_MTP_EGR0                   (1 << 6)

/* Entry has an egress 1 Mirror-To-Port reserved. */
#define _FP_ENTRY_MTP_EGR1                   (1 << 7)

/* Second part of double wide intraslice entry. */  
#define _FP_ENTRY_SECOND_HALF                (1 << 8)

/* Field entry installed in hw. */
#define _FP_ENTRY_INSTALLED                  (1 << 9)

/* Treat all packets as green. */
#define _FP_ENTRY_COLOR_INDEPENDENT          (1 << 10)

/* Meter installed in secondary slice . */
#define _FP_ENTRY_POLICER_IN_SECONDARY_SLICE   (1 << 11)

/* Counter installed in secondary slice . */
#define _FP_ENTRY_STAT_IN_SECONDARY_SLICE    (1 << 12)

/* Allocate meters/counters from secondary slice. */
#define _FP_ENTRY_ALLOC_FROM_SECONDARY_SLICE (1 << 13)

/* Entry uses secondary overlay */
#define _FP_ENTRY_USES_IPBM_OVERLAY (1 << 14)

/* Entry action dirty */
#define _FP_ENTRY_ACTION_ONLY_DIRTY (1 << 15)

/* Field entry enabled in hw. */
#define _FP_ENTRY_ENABLED                    (1 << 16)


/* Entry slice identification flags. */
#define _FP_ENTRY_SLICE_FLAGS (_FP_ENTRY_PRIMARY |  _FP_ENTRY_SECONDARY | \
                               _FP_ENTRY_TERTIARY)

/* Entry slice identification flags. */
#define _FP_ENTRY_MIRROR_ON  (_FP_ENTRY_MTP_ING0 |  _FP_ENTRY_MTP_ING1 | \
                               _FP_ENTRY_MTP_EGR0 | _FP_ENTRY_MTP_EGR1)


/*
 * Group status flags. 
 */

/* Group resides in a single slice. */
#define _FP_GROUP_SPAN_SINGLE_SLICE          (1 << 0)     

/* Group resides in a two paired slices. */
#define _FP_GROUP_SPAN_DOUBLE_SLICE          (1 << 1)

/* Group resides in three paired slices. */
#define _FP_GROUP_SPAN_TRIPLE_SLICE          (1 << 2)

/* Group entries are double wide in each slice. */  
#define _FP_GROUP_INTRASLICE_DOUBLEWIDE      (1 << 3)

/* 
 * Group has slice lookup enabled 
 *     This is default, unless it is disabled by call to
 *     bcm_field_group_enable_set with enable=0
 */
#define _FP_GROUP_LOOKUP_ENABLED             (1 << 4)

/* Group for WLAN tunnel terminated packets. */  
#define _FP_GROUP_WLAN                       (1 << 5)

/* Group resides on the smaller slice */
#define _FP_GROUP_SELECT_SMALL_SLICE         (1 << 6)

/* Group resides on the larger slice */
#define _FP_GROUP_SELECT_LARGE_SLICE         (1 << 7)

/* Group supports auto expansion */
#define _FP_GROUP_SELECT_AUTO_EXPANSION      (1 << 8)

#define _FP_GROUP_STATUS_MASK        (_FP_GROUP_SPAN_SINGLE_SLICE | \
                                      _FP_GROUP_SPAN_DOUBLE_SLICE | \
                                      _FP_GROUP_SPAN_TRIPLE_SLICE | \
                                      _FP_GROUP_INTRASLICE_DOUBLEWIDE | \
                                      _FP_GROUP_WLAN)   

/* Internal DATA qualifiers. */
typedef enum _bcm_field_internal_qualify_e {
    _bcmFieldQualifyData0 = bcmFieldQualifyCount,/* [0x00] Data qualifier 0.  */
    _bcmFieldQualifyData1,                    /* [0x01] Data qualifier 1.     */
    _bcmFieldQualifyData2,                    /* [0x02] Data qualifier 2.     */
    _bcmFieldQualifyData3,                    /* [0x03] Data qualifier 3.     */
    _bcmFieldQualifySvpValid,                 /* [0x04] SVP valid             */
    _bcmFieldQualifyCount                     /* [0x05] Always last not used. */
} _bcm_field_internal_qualify_t;


/* Committed portion in sw doesn't match hw. */
#define _FP_POLICER_COMMITTED_DIRTY     (0x80000000) 

/* Peak portion in sw doesn't match hw. */
#define _FP_POLICER_PEAK_DIRTY          (0x40000000) 

/* Policer created through meter APIs.  */
#define _FP_POLICER_INTERNAL            (0x20000000) 

/* Policer using excess meter. */
#define _FP_POLICER_EXCESS_METER        (0x10000000) 

#define _FP_POLICER_DIRTY             (_FP_POLICER_COMMITTED_DIRTY | \
                                       _FP_POLICER_PEAK_DIRTY)

#define _FP_POLICER_LEVEL_COUNT       (2)

/* Flow mode policer using committed meter in hardware. */
#define _FP_POLICER_COMMITTED_HW_METER(f_pl)                \
        (bcmPolicerModeCommitted == (f_pl)->cfg.mode        \
         && !((f_pl)->hw_flags & _FP_POLICER_EXCESS_METER))

/* Flow mode policer using excess meter in hardware. */
#define _FP_POLICER_EXCESS_HW_METER(f_pl)                   \
        (bcmPolicerModeCommitted == (f_pl)->cfg.mode        \
         && ((f_pl)->hw_flags & _FP_POLICER_EXCESS_METER))

/* Set excess meter bit. */
#define _FP_POLICER_EXCESS_HW_METER_SET(f_pl)               \
        ((f_pl)->hw_flags |= _FP_POLICER_EXCESS_METER)

/* Clear excess meter bit. */
#define _FP_POLICER_EXCESS_HW_METER_CLEAR(f_pl)             \
        ((f_pl)->hw_flags &= ~_FP_POLICER_EXCESS_METER)

/* Check for Flow mode policer. */
#define _FP_POLICER_IS_FLOW_MODE(f_pl)                      \
        (bcmPolicerModeCommitted == (f_pl)->cfg.mode)

/*
 * Typedef:
 *     _field_policer_t
 * Purpose:
 *     This is the policer description structure. 
 *     Indexed by bcm_policer_t handle.
 */
typedef struct _field_policer_s {
    bcm_policer_t        pid;         /* Unique policer identifier.       */
    bcm_policer_config_t cfg;         /* API level policer configuration. */
    uint16               sw_ref_count;/* SW object use reference count.   */
    uint16               hw_ref_count;/* HW object use reference count.   */
    uint8                level;       /* Policer attachment level.        */
    int8                 pool_index;  /* Meter pool/slice policer resides.*/
    int                  hw_index;    /* HW index policer resides.        */
    uint32               hw_flags;    /* HW installation status flags.    */
    _field_stage_id_t    stage_id;    /* Attached entry stage id.         */
    struct _field_policer_s *next;    /* Policer lookup linked list.      */
}_field_policer_t;


#define _FP_POLICER_VALID                (1 << 0)
#define _FP_POLICER_INSTALLED            (1 << 1)

/*
 * Typedef:
 *     _field_entry_policer_t
 * Purpose:
 *     This is field entry policers description structure.
 *     Used to form an array for currently attached policers. 
 */
typedef struct _field_entry_policer_s {
    bcm_policer_t  pid;         /* Unique policer identifier. */
    uint8          flags;       /* Policer/entry flags.       */
}_field_entry_policer_t;

/* _bcm_field_stat_e  - Internal counter types. */
typedef enum _bcm_field_stat_e {
    _bcmFieldStatCount = bcmFieldStatAcceptedBytes /* Internal STAT count
                                                      for XGS devices. Not
                                                      a usable value. */
} _bcm_field_stat_t;

#define _BCM_FIELD_STAT \
{ \
    "BytesEven", \
    "BytesOdd", \
    "PacketsEven", \
    "PacketsOdd" \
}

#define _FP_STAT_HW_MODE_MAX  (0xf)
#define _FP_TRIDENT_STAT_HW_MODE_MAX  (0x3F)

/* Statistics entity was  created through counter APIs.  */
#define _FP_STAT_INTERNAL          (1 << 0) 
/* UpdateCounter action was used with NO_YES/YES_NO. */
#define _FP_STAT_COUNTER_PAIR      (1 << 1) 
/* Arithmetic operations. */
#define _FP_STAT_ADD               (1 << 2)
#define _FP_STAT_SUBSTRACT         (1 << 3)
/* Packet/bytes selector. */
#define _FP_STAT_BYTES             (1 << 4)
/* Sw entry doesn't match hw. */
#define _FP_STAT_DIRTY             (1 << 5)
/* Stat Create with ID */
#define _FP_STAT_CREATE_ID         (1 << 6)
/* Stat uses Flex Stat resources. */
#define _FP_STAT_FLEX_CNTR         (1 << 7)
/* Stat Create with Internal Advanced Flex Counter feature  */
#define _FP_STAT_INTERNAL_FLEX_COUNTER (1 << 8)




/* Action conflict check macro. */
#define _FP_ACTIONS_CONFLICT(_val_)    \
    if (action == _val_) {             \
        return (BCM_E_CONFIG);         \
    }


/*
 * Typedef:
 *     _field_stat_t
 * Purpose:
 *     This is the statistics collection entity description structure.
 *     Indexed by int sid (statistics id) handle.
 *     
 */
typedef struct _field_stat_s {
    uint32               sid;           /* Unique stat entity identifier.  */

    /* Reference counters  information.*/
    uint16               sw_ref_count;  /* SW object use reference count.   */
    uint16               hw_ref_count;  /* HW object use reference count.   */
    /* Allocated hw resource information.*/
    int8                 pool_index;    /* Counter pool/slice stat resides. */
    int                  hw_index;      /* HW index stat resides.           */
    /* Reinstall flags. */
    uint32               hw_flags;      /* HW installation status flags.    */
    /* Application requested statistics. */
    uint8                nstat;         /* User requested stats array size. */
    bcm_field_stat_t     *stat_arr;     /* User requested stats array.      */ 
    /* HW supported statistics. */
    uint32               hw_stat;       /* Statistics supported by HW.      */
    uint8                hw_mode;       /* Hw configuration mode.           */
    uint8                hw_entry_count;/* Number of counters needed.       */
    bcm_field_group_t    gid;           /* Group statistics entity attached.*/
    _field_stage_id_t    stage_id;      /* Attached entry stage id.         */
    struct _field_stat_s *next;         /* Stat lookup linked list.         */
    /* Values after last detach. */
    uint64               *stat_values;  /* Stat value after it was detached */
                                        /* from a last entry.               */
    uint32              flex_mode;      /* Flex stat entity identifier.     */
} _field_stat_t;


/*
 * Typedef:
 *     _field_entry_stat_t
 * Purpose:
 *     This is field entry statistics collector descriptor structure.
 */
typedef struct _field_entry_stat_s {
    int            sid;         /* Unique statistics entity id.  */
    uint8          flags;       /* Statistics entity/entry flags.*/
}_field_entry_stat_t;
/* Statistics entity attached to fp entry flag. */
#define _FP_ENTRY_STAT_VALID                (1 << 0)
/* Statistics entity installed in HW. */
#define _FP_ENTRY_STAT_INSTALLED            (1 << 1)
/* Statistics entity doesn't have any counters attached. */
#define _FP_ENTRY_STAT_EMPTY                (1 << 2)
/* Statistics entity use even counter. */
#define _FP_ENTRY_STAT_USE_EVEN             (1 << 3)
/* Statistics entity use odd counter. */
#define _FP_ENTRY_STAT_USE_ODD              (1 << 4)

/*
 * Typedef:
 *     _field_entry_t
 * Purpose:
 *     This is the physical entry structure, hidden behind the logical
 *     bcm_field_entry_t handle.
 * Notes:
 *     Entries are stored in linked list in the under a slice's _field_slice_t
 *     structure.
 *
 *     Each entry can use 0 or 1 counters. Multiple entries may use the
 *     same counter. The only requirement is that the counter be within
 *     the same slice as the entry.
 *
 *     Similarly each entry can use 0 or 1 meter. Multiple entries may use
 *     the same meter. The only requirement is that the meter be within
 *     the same slice as the entry.
 */
struct _field_entry_s {
    bcm_field_entry_t      eid;        /* BCM unit unique entry identifier   */
    int                    prio;       /* Entry priority                     */
    uint32                 slice_idx;  /* Field entry tcam index.            */
    uint32                 ext_act_profile_idx; /* External field entry action profile index. */
    uint32                 flags;      /* _FP_ENTRY_xxx flags                */
    _field_tcam_t          tcam;       /* Fields to be written into FP_TCAM  */
    _field_tcam_t          extra_tcam;
#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_TRX_SUPPORT)
    _field_pbmp_t          pbmp;       /* Port bitmap                        */
#endif /* BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */    
    _field_action_t        *actions;   /* linked list of actions for entry   */
    _field_slice_t         *fs;        /* Slice where entry lives            */
    _field_group_t         *group;     /* Group where entry lives            */
    _field_entry_stat_t    statistic;  /* Statistics collection entity.      */           
                                       /* Policers attached to the entry.    */
    _field_entry_policer_t policer[_FP_POLICER_LEVEL_COUNT];
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    _field_entry_policer_t global_meter_policer;
#endif
    struct _field_entry_s  *ent_copy;
    struct _field_entry_s  *next;      /* Entry lookup linked list.          */
};

typedef struct _field_control_s _field_control_t;

/*
 * Typedef:
 *     _field_funct_t
 * Purpose:
 *     Function pointers to device specific Field functions
 */
typedef struct _field_funct_s {
    int(*fp_detach)(int, _field_control_t *fc);  /* destructor function */
    int(*fp_data_qualifier_ethertype_add)(int, int, 
                                          bcm_field_data_ethertype_t *);
    int(*fp_data_qualifier_ethertype_delete)(int, int, 
                                          bcm_field_data_ethertype_t *);
    int(*fp_data_qualifier_ip_protocol_add)(int, int, 
                                          bcm_field_data_ip_protocol_t *);
    int(*fp_data_qualifier_ip_protocol_delete)(int, int, 
                                          bcm_field_data_ip_protocol_t *);
    int(*fp_data_qualifier_packet_format_add)(int, int, 
                                          bcm_field_data_packet_format_t *);
    int(*fp_data_qualifier_packet_format_delete)(int, int, 
                                          bcm_field_data_packet_format_t *);
    int(*fp_group_install)(int, _field_group_t *fg);
    int(*fp_selcodes_install)(int unit, _field_group_t *fg,
                              uint8 slice_numb, bcm_pbmp_t pbmp,
                              int selcode_index);
    int(*fp_slice_clear)(int unit, _field_group_t *fg, _field_slice_t *fs);
    int(*fp_entry_remove)(int unit, _field_entry_t *f_ent, int tcam_idx);
    int(*fp_entry_move) (int unit, _field_entry_t *f_ent, int parts_count, 
                         int *tcam_idx_old, int *tcam_idx_new);
    int(*fp_selcode_get)(int unit, _field_stage_t*, bcm_field_qset_t*, 
                         _field_group_t*);
    int(*fp_selcode_to_qset)(int unit, _field_stage_t *stage_fc, 
                             _field_group_t *fg,
                             int code_id,
                             bcm_field_qset_t *qset);
    int(*fp_qual_list_get)(int unit, _field_stage_t *, _field_group_t*);
    int(*fp_tcam_policy_clear)(int unit, _field_stage_id_t stage_id, int idx);
    int(*fp_tcam_policy_install)(int unit, _field_entry_t *f_ent, int idx);
    int(*fp_tcam_policy_reinstall)(int unit, _field_entry_t *f_ent, int idx);
    int(*fp_policer_install)(int unit, _field_entry_t *f_ent,
                             _field_policer_t *f_pl);
    int(*fp_write_slice_map)(int unit, _field_stage_t *stage_fc);
    int(*fp_qualify_ip_type)(int unit, _field_entry_t *f_ent,
                             bcm_field_IpType_t type); 
    int(*fp_qualify_ip_type_get)(int unit, _field_entry_t *f_ent,
                                 bcm_field_IpType_t *type); 
    int(*fp_action_support_check)(int unit, _field_entry_t *f_ent,
                                  bcm_field_action_t action, int *result); 
    int(*fp_action_conflict_check)(int unit, _field_entry_t *f_ent,
                                   bcm_field_action_t action, 
                                   bcm_field_action_t action1); 
    int(*fp_action_params_check)(int unit, _field_entry_t *f_ent,
                                 _field_action_t *fa);
    int(*fp_action_depends_check)(int unit, _field_entry_t *f_ent,
                                  _field_action_t *fa);
    int (*fp_egress_key_match_type_set)(int unit, _field_entry_t *f_ent);
    int (*fp_external_entry_install)(int unit, _field_entry_t *f_ent);
    int (*fp_external_entry_reinstall)(int unit, _field_entry_t *f_ent);
    int (*fp_external_entry_remove)(int unit, _field_entry_t *f_ent);
    int (*fp_external_entry_prio_set)(int unit, _field_entry_t *f_ent, 
                                      int prio);
    int (*fp_counter_get)(int unit, _field_stage_t *stage_fc,
                          soc_mem_t counter_x_mem, uint32 *mem_x_buf,
                          soc_mem_t counter_y_mem, uint32 *mem_y_buf,
                          int idx, uint64 *packet_count,
                          uint64 *byte_count);
    int (*fp_counter_set)(int unit, _field_stage_t *stage_fc,
                          soc_mem_t counter_x_mem, uint32 *mem_x_buf,
                          soc_mem_t counter_y_mem, uint32 *mem_y_buf,
                          int idx, uint64 *packet_count,
                          uint64 *byte_count);
    int (*fp_stat_index_get)(int unit, _field_stat_t *f_st, 
                             bcm_field_stat_t stat, int *idx1, 
                             int *idx2, uint32 *out_flags);
} _field_funct_t;

#ifdef BCM_WARM_BOOT_SUPPORT
typedef struct _field_slice_group_info_s {
    bcm_field_group_t                  gid; /* Temp recovered GID       */
    int                           priority; /* Temp recovered group priority */
    bcm_field_qset_t                  qset; /* Temp QSET retrieved      */
    bcm_pbmp_t                        pbmp; /* Temp recovered rep. port */
    int                              found; /* Group has been found     */
    struct _field_slice_group_info_s *next;
} _field_slice_group_info_t;

typedef struct _field_hw_qual_info_s {
    _field_sel_t pri_slice[2]; /* Potentially intraslice */
    _field_sel_t sec_slice[2]; /* Potentially intraslice */
} _field_hw_qual_info_t;

typedef struct _meter_config_s {
    uint8 meter_mode;
    uint8 meter_mode_modifier;
    uint16 meter_idx;
    uint8 meter_update_odd;
    uint8 meter_test_odd;
    uint8 meter_update_even;
    uint8 meter_test_even;
} _meter_config_t;
typedef struct _field_table_pointers_s {
    char *fp_global_mask_tcam_buf;
    char *fp_gm_tcam_x_buf;
    char *fp_gm_tcam_y_buf;
    uint32 *fp_tcam_buf;
    uint32 *fp_tcam_x_buf;
    uint32 *fp_tcam_y_buf;
} _field_table_pointers_t;
#endif

/* This reflects the 64,000bps granularity */
#define _FP_POLICER_REFRESH_RATE 64

/*
 * Typedef:
 *     _field_control_t
 * Purpose:
 *     One structure for each StrataSwitch Device that holds the global
 *     field processor metadata for one device.
 */
struct _field_control_s {
    int                    init;          /* TRUE if field module has been */
                                          /* initialized                   */
    sal_mutex_t            fc_lock;       /* Protection mutex.             */
    uint8                  flags;         /* Module specific flags, as follows */
#define _FP_COLOR_INDEPENDENT          (1 << 0)
#define _FP_INTRASLICE_ENABLE          (1 << 1)
#define _FP_EXTERNAL_PRESENT           (1 << 2)
#ifdef BCM_WARM_BOOT_SUPPORT
#define _FP_STABLE_SAVE_LONG_IDS       (1 << 3)
#endif
#define _FP_STAT_SYNC_ENABLE           (1 << 4)
#define _FP_POLICER_GROUP_SHARE_ENABLE (1 << 5)

    bcm_field_stage_t      stage;         /* Default FP pipeline stage.    */
    int                    max_stage_id;  /* Number of fp stages.          */
    int                    tcam_ext_numb; /* Slice number for external.    */
                                          /* TCAM (-1 if not present).     */
    _field_udf_t           *udf;          /* field_status->group_total     */
                                          /* elements indexed by priority. */
    struct _field_group_s  *groups;       /* List of groups in unit.       */
    struct _field_stage_s  *stages;       /* Pipeline stage FP info.       */
    _field_funct_t         functions;     /* Device specific functions.    */
    _field_entry_t         **entry_hash;  /* Entry lookup hash.            */
    _field_policer_t       **policer_hash;/* Policer lookup hash.          */
    uint32                 policer_count; /* Number of active policers.    */
    _field_stat_t          **stat_hash;   /* Counter lookup hash.          */
    uint32                 stat_count;    /* Number of active counters.    */
#ifdef BCM_WARM_BOOT_SUPPORT
#define _FIELD_SCACHE_PART_COUNT 0x2
    /* Size of Level2 warm boot cache */
    uint32                 scache_size[_FIELD_SCACHE_PART_COUNT];
    uint32                 scache_usage;  /* Actual scache usage            */
    uint32                 scache_pos;   /* Current position of scache     
                                            pointer in _FIELD_ACACHE_PART_0 */
    uint32                 scache_pos1;  /* Current position of scache  
                                            pointer in _FIELD_ACACHE_PART_1 */
    /* Pointer to scache section      */
    uint8                  *scache_ptr[_FIELD_SCACHE_PART_COUNT];  
    uint8                  l2warm;        /* Use the stored scache data     */
    _field_slice_group_info_t *group_arr; /* Temp recovered group info      */
#endif
};


/*
 *  _field_group_add_fsm struct. 
 *  Field group creation state machine.  
 *  Handles sanity checks, resources selection, allocation. 
 */
typedef struct _field_group_add_fsm_s {
    /* State machine data. */
    uint8                 fsm_state;     /* FSM state.                     */
    uint8                 fsm_state_prev;/* Previous FSM state.            */
    uint32                flags;         /* State specific flags.          */
    int                   rv;            /* Error code.                    */

    /* Field control info. */
    _field_control_t      *fc;           /* Field control structure.       */
    _field_stage_t        *stage_fc;     /* Field stage control structure. */
    

    /* Group information. */
    int                    priority;     /* New group priority.            */
    bcm_field_group_t      group_id;     /* SW Group id.                   */
    bcm_pbmp_t             pbmp;         /* Input port bitmap.             */
    bcm_field_qset_t       qset;         /* Qualifiers set.                */      
    bcm_field_group_mode_t mode;         /* Group mode.                    */

    /* Allocated data structures. */
    _field_group_t          *fg;         /* Allocated group structure.     */   
} _field_group_add_fsm_t;

#define _BCM_FP_GROUP_ADD_STATE_START                  (0x1)
#define _BCM_FP_GROUP_ADD_STATE_ALLOC                  (0x2)
#define _BCM_FP_GROUP_ADD_STATE_QSET_UPDATE            (0x3)
#define _BCM_FP_GROUP_ADD_STATE_SEL_CODES_GET          (0x4)
#define _BCM_FP_GROUP_ADD_STATE_QSET_ALTERNATE         (0x5)
#define _BCM_FP_GROUP_ADD_STATE_SLICE_ALLOCATE         (0x6)
#define _BCM_FP_GROUP_ADD_STATE_HW_QUAL_LIST_GET       (0x7)
#define _BCM_FP_GROUP_ADD_STATE_UDF_UPDATE             (0x8)
#define _BCM_FP_GROUP_ADD_STATE_ADJUST_VIRTUAL_MAP     (0x9)
#define _BCM_FP_GROUP_ADD_STATE_EXTERNAL_GROUP_CREATE  (0xa)
#define _BCM_FP_GROUP_ADD_STATE_CAM_COMPRESS           (0xb)
#define _BCM_FP_GROUP_ADD_STATE_END                    (0xc)

#define _BCM_FP_GROUP_ADD_STATE_STRINGS \
{                                  \
    "None", \
    "GroupAddStart",               \
    "GroupAddAlloc",               \
    "GroupAddStateQsetUpdate",     \
    "GroupAddSelCodesGet",         \
    "GroupAddQsetAlternate",       \
    "GroupAddSliceAllocate",       \
    "GroupAddHwQualListGet",       \
    "GroupAddUdfUpdate",           \
    "GroupAddAdjustVirtualMap",    \
    "GroupAddExternalGroupCreate", \
    "GroupAddCamCompress",         \
    "GroupAddEnd"                  \
}

#define _BCM_FP_GROUP_ADD_INTRA_SLICE          (1 << 0)
#define _BCM_FP_GROUP_ADD_INTRA_SLICE_ONLY     (1 << 1)
#define _BCM_FP_GROUP_ADD_WLAN                 (1 << 2)
#define _BCM_FP_GROUP_ADD_SMALL_SLICE          (1 << 3)
#define _BCM_FP_GROUP_ADD_LARGE_SLICE          (1 << 4)

/* _bcm_field_DestType_e  - Destination types. */
typedef enum _bcm_field_DestType_e {
    _bcmFieldDestTypeNhi,   /* Next Hop Interface */
    _bcmFieldDestTypeL3mc,  /* IPMC group         */
    _bcmFieldDestTypeL2mc,  /* L2 Multicast group */
    _bcmFieldDestTypeDvp,   /* Virtual Port       */
    _bcmFieldDestTypeDglp,  /* Mod/Port/Trunk     */
    _bcmFieldDestTypeNone,  /* None               */
    _bcmFieldDestTypeEcmp,  /* ECMP group         */
    _bcmFieldDestTypeCount  /* Always Last. Not a usable value. */
} _bcm_field_DestType_t;

#define _FIELD_D_TYPE_MASK  (0x7)
#define _FIELD_D_TYPE_POS   \
       ((SOC_IS_TRIDENT2(unit) || SOC_IS_TRIUMPH3(unit)) ? (18) : \
       ((SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) ? (16) : (14)))
#define _FIELD_D_TYPE_INSERT(_d_, _dtype_)     \
       ((_d_) |= ((uint32) (_dtype_) << _FIELD_D_TYPE_POS))
#define _FIELD_D_TYPE_RESET(_d_)                \
       ((_d_) &= ~(_FIELD_D_TYPE_MASK << _FIELD_D_TYPE_POS))
#define _FIELD_D_TYPE_MASK_INSERT(_d_)          \
       ((_d_) |= (_FIELD_D_TYPE_MASK << _FIELD_D_TYPE_POS))

/* Insert destination type value at specific bit offset */
#define _FIELD_D_TYPE_INSERT_BITPOS(_d_, _dtype_, _bitpos_)   \
       ((_d_) |= ((uint32) (_dtype_) << (_bitpos_)))
#define _FIELD_D_TYPE_RESET_BITPOS(_d_, _bitpos_)             \
       ((_d_) &= ~(_FIELD_D_TYPE_MASK << (_bitpos_)))
#define _FIELD_D_TYPE_MASK_INSERT_BITPOS(_d_, _bitpos_)              \
       ((_d_) |= (_FIELD_D_TYPE_MASK << (_bitpos_)))

extern soc_field_t _trx_src_class_id_sel[], _trx_dst_class_id_sel[],
    _trx_interface_class_id_sel[];

#define COPY_TO_CPU_CANCEL                 0x2
#define SWITCH_TO_CPU_CANCEL               0x3
#define COPY_AND_SWITCH_TO_CPU_CANCEL      0x6

/*
 * Prototypes of Field Processor utility funtions
 */
extern int _field_control_get(int unit, _field_control_t **fc);
extern int _field_stage_control_get(int unit, _field_stage_id_t stage,
                                    _field_stage_t **stage_fc);
extern int _field_group_get(int unit, bcm_field_group_t gid, 
                            _field_group_t **group_p);
extern int _field_group_id_generate(int unit, bcm_field_group_t *group);
extern int _bcm_field_group_default_aset_set(int unit, _field_group_t *fg);
extern int _field_entry_get(int unit, bcm_field_entry_t eid, uint32 flags, 
                            _field_entry_t **enty_p);
extern int _field_group_entry_add (int unit, _field_group_t *fg, 
                                   _field_entry_t *f_ent);
extern int _bcm_field_entry_tcam_parts_count (int unit, uint32 group_flags, 
                                              int *part_count);

extern int _bcm_field_entry_flags_to_tcam_part (uint32 entry_flags, 
                                                uint32 group_flags, 
                                                uint8 *entry_part);
extern int _bcm_field_tcam_part_to_entry_flags(int entry_part, 
                                               uint32 group_flags, 
                                               uint32 *entry_flags);
extern int _field_port_filter_enable_set(int unit, _field_control_t *fc, 
                                         uint32 state);
extern int _field_meter_refresh_enable_set(int unit, _field_control_t *fc,
                                           uint32 state);
extern int _field_qual_info_create(bcm_field_qualify_t qid, soc_mem_t mem,
                                   soc_field_t fpf, int offset, int width,
                                    _qual_info_t **fq_p);
extern int _field_qualify32(int unit, bcm_field_entry_t entry, 
                            int qual, uint32 data, uint32 mask);
extern int _field_qset_union(const bcm_field_qset_t *qset1,
                             const bcm_field_qset_t *qset2,
                             bcm_field_qset_t *qset_union);
extern int _field_qset_is_subset(const bcm_field_qset_t *qset1,
                                 const bcm_field_qset_t *qset2);
extern bcm_field_qset_t _field_qset_diff(const bcm_field_qset_t qset_1,
                                         const bcm_field_qset_t qset_2);
extern int _field_qset_is_empty(const bcm_field_qset_t qset);
extern int
_field_selector_diff(int unit, _field_sel_t *sel_arr, int part_idx, 
                     _bcm_field_selector_t *selector, 
                     uint8 *diff_count
                     );

extern int _field_trans_flags_to_index(int unit, int flags, uint8 *tbl_idx);
extern int _field_qual_add (_field_fpf_info_t *fpf_info, int qid, int offset,
                            int width, int code);
extern int _bcm_field_tpid_hw_encode(int unit, uint16 tpid, uint32 *hw_code);
extern int _field_tpid_hw_decode(int unit, uint32 hw_code, uint16 *tpid);
extern int _field_entry_prio_cmp(int prio_first, int prio_second);
extern int _bcm_field_prio_mgmt_init(int unit, _field_stage_t *stage_fc);
extern int _bcm_field_prio_mgmt_deinit(int unit, _field_stage_t *stage_fc);
extern int _bcm_field_entry_prio_mgmt_update(int unit, _field_entry_t *f_ent, 
                                              int flag, uint32 old_location);
extern int
_bcm_field_prio_mgmt_slice_reinit(int            unit,
                                  _field_stage_t *stage_fc,
                                  _field_slice_t *fs
                                  );
extern int _bcm_field_entry_target_location(int unit, _field_stage_t *stage_fc, 
                   _field_entry_t *f_ent, int new_prio, uint32 *new_location);
extern int _bcm_field_entry_counters_move(int unit, _field_stage_t *stage_fc, 
                                          int old_index, int new_index);
extern int _bcm_field_sw_counter_update(int unit, _field_stage_t *stage_fc, 
                                        soc_mem_t mem, int idx_min, int idx_max,
                                        char *buf, int flags);
extern int _bcm_field_counter_mem_get(int            unit,
                                      _field_stage_t *stage_fc, 
                                      soc_mem_t      *counter_x_mem,
                                      soc_mem_t      *counter_y_mem
                                      );
extern int _bcm_field_counter_hw_alloc(int            unit,
                                       _field_entry_t *f_ent,
                                       _field_slice_t *fs
                                       );
extern int _bcm_field_stat_hw_free(int            unit,
                                   _field_entry_t *f_ent
                                   );
extern int _bcm_field_policer_get(int unit, bcm_policer_t pid, 
                                  _field_policer_t **policer_p);
extern int _bcm_field_stat_get(int unit, int sid, _field_stat_t **stat_p);
extern int _field_stat_id_alloc(int unit, int *sid);
extern int _field_stat_array_init(int unit, _field_stat_t *f_st, 
                                  int nstat, bcm_field_stat_t *stat_arr);
extern int _bcm_field_meter_pair_mode_get(int unit, _field_policer_t *f_pl,
                                          uint32 *mode);
extern int _field_policer_id_alloc(int unit, bcm_policer_t *pid);
extern int _bcm_field_29bit_counter_update(int unit, uint32 *new_val, 
                                    _field_counter32_collect_t *result);
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
extern int _bcm_field_30bit_counter_update(int unit, uint32 *new_val, 
                                    _field_counter32_collect_t *result);
#endif /* BCM_TRIDENT_SUPPORT || BCM_TRIUMPH3_SUPPORT */

extern int _bcm_field_32bit_counter_update(int unit, uint32 *new_val, 
                                           _field_counter32_collect_t *result);
#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
extern int _bcm_field_36bit_counter_update(int unit, uint32 *new_val, 
                                           _field_counter64_collect_t *result);
#endif /* BCM_BRADLEY_SUPPORT||BCM_TRIUMPH3_SUPPORT*/
#ifdef BCM_TRIDENT_SUPPORT
extern int _bcm_field_37bit_counter_update(int unit, uint32 *new_val, 
                                           _field_counter64_collect_t *result);
#endif
extern int _bcm_field_entry_counter_move(int unit, _field_stage_t *stage_fc,
                                         uint8 old_slice, int old_hw_index,
                                         _field_stat_t *f_st_old,
                                         _field_stat_t *f_st);
extern int _bcm_field_qset_test(bcm_field_qualify_t qid, 
                                bcm_field_qset_t *qset, uint8 *result);
extern int _bcm_field_entry_get_by_id(int unit, bcm_field_entry_t eid, 
                                      _field_entry_t **f_ent);
extern int _field_qual_tcam_key_mask_get(int unit, _field_entry_t *f_ent,
                                         _field_tcam_t *tcam, int ipbm_overlay);
extern int _bcm_field_qual_tcam_key_mask_get(int unit, _field_entry_t *f_ent);
extern int _bcm_field_qual_list_append(_field_group_t *fg, uint8 entry_part,
                                     _qual_info_t *fq_src, int offset);
extern int _bcm_field_qual_value_set(int unit, _bcm_field_qual_offset_t *q_offset, 
                                     _field_entry_t *f_ent, 
                                     uint32 *p_data, uint32 *p_mask);
extern int _bcm_field_group_qualifiers_free(_field_group_t *fg, int idx);
extern int _bcm_field_qual_insert (int unit, _field_stage_t *stage_fc, 
                                   int qual_id, _bcm_field_qual_conf_t *ptr);
extern int _bcm_field_tcam_part_to_slice_number(int entry_part, 
                                                uint32 group_flags, 
                                                uint8 *slice_number);
extern int _bcm_field_group_status_init(int unit,
                                        bcm_field_group_status_t *entry);
extern int _bcm_field_group_status_calc(int unit, _field_group_t *fg);
extern int _bcm_field_stage_entries_free(int unit, 
                                         _field_stage_t *stage_fc);
extern int _bcm_field_selcode_get(int unit, _field_stage_t *stage_fc, 
                                      bcm_field_qset_t *qset_req,
                                      _field_group_t *fg);
extern void _bcm_field_qual_conf_t_init(_bcm_field_qual_conf_t *ptr);
extern void _bcm_field_qual_info_t_init(_bcm_field_qual_info_t *ptr);
extern int _bcm_field_selcode_to_qset(int unit, _field_stage_t *stage_fc, 
                                          _field_group_t *fg, int code_id,
                                          bcm_field_qset_t *qset);
extern int _bcm_field_qual_lists_get(int unit, _field_stage_t *stage_fc,
                                         _field_group_t *fg);
extern int _bcm_field_entry_part_tcam_idx_get(int unit, _field_entry_t *f_ent,
                                              uint32 idx_pri, uint8 ent_part, 
                                              int *idx_out);
extern int _bcm_field_data_qualifier_get(int unit, _field_stage_t *stage_fc,  
                                         int qid, 
                                         _field_data_qualifier_t **data_qual);
extern int _bcm_field_data_qualifier_alloc(int unit,
                                           _field_data_qualifier_t **qual);
extern int _bcm_field_data_qualifier_free(int unit, 
                                          _field_data_qualifier_t *qual);
extern int _bcm_field_entry_qualifier_uint32_get(int unit, 
                                                 bcm_field_entry_t entry, 
                                                 int qual_id, uint32 *data, 
                                                 uint32 *mask);
extern int _bcm_field_entry_qualifier_uint16_get(int unit, 
                                                 bcm_field_entry_t entry, 
                                                 int qual_id, uint16 *data, 
                                                 uint16 *mask);
extern int _bcm_field_entry_qualifier_uint8_get(int unit, 
                                                bcm_field_entry_t entry, 
                                                int qual_id, uint8 *data, 
                                                uint8 *mask);
extern int _bcm_field_qual_tcam_key_mask_free(int unit, _field_entry_t *f_ent);
extern int _bcm_field_action_dest_check(int unit, _field_action_t *fa);
extern int _bcm_field_policy_set_l3_nh_resolve(int unit,  int value,
                                               uint32 *flags, int *nh_ecmp_id);
extern int _bcm_field_virtual_map_size_get(int unit, _field_stage_t *stage_fc,
                                           int *size);
extern int _bcm_field_entry_tcam_idx_get(int unit,_field_entry_t *f_ent,
                                         int *tcam_idx);
extern int _bcm_field_tcam_idx_to_slice_offset(int unit,
                                             _field_stage_t *stage_fc,
                                             int tcam_idx, int *slice,
                                             int *offset);
extern int _bcm_field_slice_offset_to_tcam_idx(int unit, 
                                               _field_stage_t *stage_fc,
                                               int slice, int offset, 
                                               int *tcam_idx);
extern int _field_action_alloc(int unit, bcm_field_action_t action, 
                               uint32 param0, uint32 param1,
                               uint32 param2, uint32 param3,
                               uint32 param4, uint32 param5,
                               _field_action_t **fa);
extern int
_field_qualify_VlanFormat_get(int                 unit,
                              bcm_field_entry_t   entry,
                              bcm_field_qualify_t qual_id,
                              uint8               *data,
                              uint8               *mask
                              );
extern int
_field_data_qualifier_id_alloc(int unit, _field_stage_t *stage_fc,  
                               bcm_field_data_qualifier_t *data_qualifier);
extern int
_field_data_qualifier_init(int                        unit,
                           _field_stage_t             *stage_fc,
                           _field_data_qualifier_t    *f_dq,
                           bcm_field_data_qualifier_t *data_qualifier
                           );
extern int
_field_data_qualifier_init2(int                        unit,
                            _field_stage_t             *stage_fc,
                            _field_data_qualifier_t    *f_dq
                            );
extern int
_field_data_qual_recover(int              unit,
                         _field_control_t *fc, 
                         _field_stage_t   *stage_fc
                         );
extern void
_field_qset_udf_bmap_reinit(int              unit,
                            _field_stage_t   *stage_fc,
                            bcm_field_qset_t *qset,
                            int              qual_id
                            );
extern int
_field_trx2_udf_tcam_entry_l3_parse(int unit, uint32 *hw_buf,
                                    bcm_field_data_packet_format_t *pkt_fmt);
extern int
_field_trx2_udf_tcam_entry_vlanformat_parse(int unit, uint32 *hw_buf,
                                    uint16 *vlanformat);
extern int
_field_trx2_udf_tcam_entry_l2format_parse(int unit, uint32 *hw_buf,
                                    uint16 *l2format);

extern void
_field_scache_sync_data_quals_write(_field_control_t *fc, _field_data_control_t *data_ctrl);

extern int
_field_counter_mem_get(int unit, _field_stage_t *stage_fc,
                       soc_mem_t *counter_x_mem, soc_mem_t *counter_y_mem);
extern int _bcm_field_policer_mode_support(int              unit,
                                           _field_entry_t   *f_ent,
                                           int              level,
                                           _field_policer_t *f_pl
                                           );
extern int _field_stat_value_set(int unit,
                                 _field_stat_t *f_st,
                                 bcm_field_stat_t stat,
                                 uint64 value);

extern int _field_stat_value_get(int unit,
                                 _field_stat_t *f_st,
                                 bcm_field_stat_t stat,
                                 uint64 *value);
extern int _field_group_default_aset_set(int unit,
                                         _field_group_t *fg);
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
extern int _bcm_field_egress_key_attempt(int unit, _field_stage_t *stage_fc, 
                              bcm_field_qset_t *qset_req,
                              uint8 key_pri, uint8 key_sec,
                              _field_group_t *fg);
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
extern int _bcm_field_35bit_counter_update(int unit, uint32 *new_val, 
                                    _field_counter64_collect_t *result);
extern int _field_trx_action_copy_to_cpu(int unit, soc_mem_t mem,
                                         _field_entry_t *f_ent,
                                         _field_action_t *fa, uint32 *buf);
#endif /* BCM_TRX_SUPPORT */


#ifdef BCM_WARM_BOOT_SUPPORT
#define _FIELD_IFP_DATA_START 0xDEADBEE0
#define _FIELD_IFP_DATA_END   0XDEADBEEF
#define _FIELD_EFP_DATA_START 0xDEADBEE1
#define _FIELD_EFP_DATA_END   0XDEADBEEE
#define _FIELD_VFP_DATA_START 0xDEADBEE2
#define _FIELD_VFP_DATA_END   0XDEADBEED
#define _FIELD_EXTFP_DATA_START 0xDEADBEE3
#define _FIELD_EXTFP_DATA_END   0XDEADBEEC
#define _FIELD_SCACHE_PART_0    0x0
#define _FIELD_SCACHE_PART_1    0x1

#define _FIELD_QSET_ITER(qset, q) \
        for ((q) = 0; (q) < (int)bcmFieldQualifyCount; (q)++) \
            if (BCM_FIELD_QSET_TEST((qset), (q)))
#define _FIELD_QSET_INCL_INTERNAL_ITER(qset, q) \
        for ((q) = 0; (q) < (int)_bcmFieldQualifyCount; (q)++) \
            if (BCM_FIELD_QSET_TEST((qset), (q)))
extern int _field_table_read(int unit, soc_mem_t mem, char **buffer_p,
    const char *buffer_name_p);
extern void _bcm_field_last_alloc_eid_incr(void);
extern int _bcm_field_last_alloc_eid_get(void);
extern int _field_slice_expanded_status_get(int unit, _field_stage_t *stage_fc, 
                                 int *expanded);
extern int _mask_is_set(int unit, _bcm_field_qual_offset_t *offset, uint32 *buf, 
             _field_stage_id_t stage_id);
extern int _field_physical_to_virtual(int unit, int slice_idx, _field_stage_t *stage_fc);
extern int _bcm_esw_field_scache_sync(int unit);
extern int _bcm_esw_field_tr2_ext_scache_size(int       unit,
                                              unsigned  part_idx
                                              );
extern unsigned _field_trx_ext_scache_size(int       unit,
                                                  unsigned  part_idx,
                                                  soc_mem_t *mems
                                                  );
extern int _field_group_info_retrieve(int               unit,
                                      bcm_port_t        port,
                                      bcm_field_group_t *gid,
                                      int               *priority,
                                      bcm_field_qset_t  *qset,
                                      _field_control_t  *fc
                                      );
extern int _field_scache_slice_group_recover(int unit, _field_control_t *fc, 
    int slice_num, int *multigroup);
extern int _field_entry_info_retrieve(int unit, bcm_field_entry_t *eid,
    int *prio, _field_control_t *fc, int multigroup, int *prev_prio,
    bcm_field_stat_t *sid, bcm_policer_t *pid);
extern void _field_scache_slice_group_free(int unit, _field_control_t *fc, 
    int slice_num);
extern void _field_scache_stage_hdr_save(_field_control_t *fc,
                                         uint32           header
                                         );
extern int _field_scache_stage_hdr_chk(_field_control_t *fc,
                                       uint32           header
                                       );
extern int _field_range_check_reinit(int unit, _field_stage_t *stage_fc,
                                     _field_control_t *fc);

extern int _field_table_pointers_init(int unit,
                                      _field_table_pointers_t *field_tables);
#ifdef BCM_FIREBOLT_SUPPORT
extern int
_field_fb_meter_recover(int             unit,
                        _field_entry_t  *f_ent, 
                        _meter_config_t *meter_conf,
                        int             part, 
                        bcm_policer_t   pid
                        );
extern int _field_fb_stage_reinit(int unit, _field_control_t *fc,
    _field_stage_t *stage_fc);
extern int _field_scache_sync(int unit, _field_control_t *fc,
        _field_stage_t *stage_fc);
extern int _field_fb_slice_is_primary(int unit, int slice_index,
    int *is_primary_p, int *slice_mode_p);
#endif /* BCM_FIREBOLT_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT)
extern int _field_fb2_stage_ingress_reinit(int unit, _field_control_t *fc, 
                                           _field_stage_t *stage_fc);
extern int _field_fb2_stage_egress_reinit(int unit, _field_control_t *fc, 
                                          _field_stage_t *stage_fc);
extern int _field_fb2_stage_lookup_reinit(int unit, _field_control_t *fc, 
                                          _field_stage_t *stage_fc);
#endif
#if defined(BCM_RAVEN_SUPPORT)
extern int _field_raven_stage_reinit(int unit, _field_control_t *fc, 
                                     _field_stage_t *stage_fc);
#endif


#if defined(BCM_TRX_SUPPORT)
extern int _field_tr2_stage_ingress_reinit(int unit, _field_control_t *fc, 
                                           _field_stage_t *stage_fc);
extern int _field_tr2_stage_egress_reinit(int unit, _field_control_t *fc, 
                                          _field_stage_t *stage_fc);
extern int _field_tr2_stage_lookup_reinit(int unit, _field_control_t *fc, 
                                          _field_stage_t *stage_fc);
extern int _field_tr2_stage_external_reinit(int unit, _field_control_t *fc, 
                                            _field_stage_t *stage_fc);
extern int _field_tr2_scache_sync(int unit, _field_control_t *fc,
                                  _field_stage_t *stage_fc);
extern int _field_tr2_ifp_slice_expanded_status_get(int unit,
                                                    _field_stage_t *stage_fc,
                                                    int *expanded);
extern int _field_trx_scache_slice_group_recover(int unit,
                                                 _field_control_t *fc,
                                                 int slice_num,
                                                 int *multigroup,
                                                 _field_stage_t *stage_fc);
extern int _field_tr2_slice_key_control_entry_recover(int unit,
                                                      unsigned slice_num,
                                                      _field_sel_t *sel);
extern int _field_tr2_group_construct_alloc(int unit, _field_group_t **pfg);
extern void _field_tr2_ingress_entity_get(int unit,
                                          int slice_idx,
                                          uint32 *fp_tcam_buf,
                                          int slice_ent_cnt,
                                          _field_stage_t *stage_fc,
                                          int8 *ingress_entity_sel);
extern int _field_tr2_group_construct(int unit,
                                      _field_hw_qual_info_t *hw_sel,
                                      int intraslice,
                                      int paired,
                                      _field_control_t *fc,
                                      bcm_port_t port,
                                     _field_stage_id_t stage_id,
                                     int slice_idx);
extern int _field_trx_entry_info_retrieve(int unit,
                               bcm_field_entry_t *eid,
                               int               *prio,
                               _field_control_t  *fc,
                               int               multigroup,
                               int               *prev_prio,
                               bcm_field_stat_t  *sid,
                               bcm_policer_t     *pid,
                               _field_stage_t   *stage_fc);
extern int _field_tr2_actions_recover(int unit,
                                      soc_mem_t policy_mem,
                                      uint32 *policy_entry, 
                                      _field_entry_t *f_ent,
                                      int part,
                                      bcm_field_stat_t sid,
                                      bcm_policer_t pid);
extern int
_field_trx_actions_recover_action_add(int                unit,
                                      _field_entry_t     *f_ent,
                                      bcm_field_action_t action,
                                      uint32 param0, uint32 param1,
                                      uint32 param2, uint32 param3,
                                      uint32 param4, uint32 param5,
                                      uint32 hw_index);
extern int _field_tr2_stage_reinit_all_groups_cleanup(int unit,
                                                      _field_control_t *fc,
                                                      unsigned stage_id,
                                _field_table_pointers_t *fp_table_pointers);
extern int _field_tr2_group_entry_write(int unit,
                                        int slice_idx,
                                        _field_slice_t *fs,
                                        _field_control_t *fc,
                                        _field_stage_t *stage_fc);

extern int _bcm_field_trx_meter_rate_burst_recover(uint32 unit,
                                    uint32 meter_table,
                                    uint32 mem_idx,
                                    uint32   *prate,
                                    uint32   *pburst
                                    );
extern int _field_adv_flex_stat_info_retrieve(int unit, int stat_id);
#endif
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
extern void _field_extract(uint32 *buf, int offset, int width, uint32 *value);
#endif

#if defined(BCM_TRIUMPH3_SUPPORT)
extern int _bcm_field_tr3_stage_lookup_reinit(int unit,
                                              _field_control_t *fc,
                                              _field_stage_t *stage_fc);
extern int _bcm_field_tr3_stage_ingress_reinit(int unit,
                                               _field_control_t *fc,
                                               _field_stage_t   *stage_fc);
extern int _bcm_field_tr3_stage_egress_reinit(int unit,
                                              _field_control_t *fc,
                                              _field_stage_t *stage_fc);
extern int _bcm_field_tr3_stage_external_reinit(int unit,
                                                _field_control_t *fc,
                                                _field_stage_t *stage_fc);
extern int _bcm_field_tr3_counter_recover(int unit,
                                          _field_entry_t *f_ent,
                                          uint32 *policy_entry,
                                          int part,
                                          bcm_field_stat_t sid);
extern int _bcm_field_tr3_scache_sync(int unit,
                                      _field_control_t *fc,
                                      _field_stage_t *stage_fc);
extern int _bcm_field_tr3_meter_recover(int unit,
                                        _field_entry_t *f_ent,
                                        int part,
                                        bcm_policer_t pid,
                                        uint32 level,
                                        soc_mem_t policy_mem,
                                        uint32 *policy_buf
                                        );
#endif

#if defined(BCM_TRIDENT2_SUPPORT)
extern int _bcm_field_td2_scache_sync(int unit,
                                      _field_control_t *fc,
                                      _field_stage_t *stage_fc);
extern int _bcm_field_td2_stage_lookup_reinit(int unit,
                                              _field_control_t *fc,
                                              _field_stage_t *stage_fc);
#endif /* BCM_TRIDENT2_SUPPORT */
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
extern int _bcm_field_fb_slice_enable_set(int            unit,
                                          _field_group_t *fg,
                                          uint8          slice,
                                          int enable
                                          );
#endif

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_field_cleanup(int unit);
#else
#define _bcm_field_cleanup(u)        (BCM_E_NONE)
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BROADCOM_DEBUG
extern void _field_qset_debug(bcm_field_qset_t qset);
extern void _field_qset_dump(char *prefix, bcm_field_qset_t qset, char* suffix);
extern void _field_aset_dump(char *prefix, bcm_field_aset_t aset, char* suffix);
#endif /* BROADCOM_DEBUG */

#endif  /* BCM_FIELD_SUPPORT */
#endif  /* !_BCM_INT_FIELD_H */
