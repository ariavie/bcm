/*
 * $Id: oam.h,v 1.60 Broadcom SDK $
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
 */

#ifndef __BCM_INT_OAM_H__
#define __BCM_INT_OAM_H__

#include <bcm/oam.h>

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT)
#include <shared/idxres_fl.h>
#include <shared/idxres_afl.h>
#include <shared/hash_tbl.h>
#include <soc/mem.h>
#include <soc/profile_mem.h>
#endif

#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || \
    defined(BCM_HURRICANE2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
#include <bcm/field.h>
#include <bcm_int/esw/trunk.h>
#endif

#include <soc/shared/bhh_msg.h>


extern int bcm_esw_oam_lock(int unit);
extern int bcm_esw_oam_unlock(int unit);
#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_oam_sync(int unit);
#endif
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_oam_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */
#define _BCM_OAM_ENDPOINT_CCM_PERIOD_UNDEFINED 0xFFFFFFFF

#define _BCM_OAM_MAC_DA_UPPER_32 0x0180C200
#define _BCM_OAM_MAC_DA_LOWER_13 0x0030 /* To be >> 3 before use */

#define _BCM_OAM_INVALID_INDEX (-1) /* Invalid Index */

#if defined(INCLUDE_BHH)
#define _BCM_OAM_BHH_MEL_MAX 0x07
/*TR3 needs 26624 endpoints. So leave 4K endpoints and start from 30720 for BHH*/
#define _BCM_OAM_BHH_ENDPOINT_OFFSET  30720
#ifdef BCM_KATANA2_SUPPORT
#define _BCM_OAM_BHH_KT2_ENDPOINT_OFFSET     22528 
#endif
#endif /* BHH */

#ifdef BCM_KATANA2_SUPPORT
#define _BCM_OAM_DOMAIN_PORT                 0x0
#define _BCM_OAM_DOMAIN_CVLAN                0x01
#define _BCM_OAM_DOMAIN_SVLAN                0x02
#define _BCM_OAM_DOMAIN_S_PLUS_CVLAN         0x03
#define _BCM_OAM_DOMAIN_VP                   0x04
#define _BCM_OAM_DOMAIN_BHH                  0x05
#define _BCM_OAM_KATANA2_ENDPOINT_MAX        32768
#endif

#define _BCM_OAM_TRIUMPH3_ENDPOINT_MAX     32768
#define _BCM_OAM_HURRICANE2_ENDPOINT_MAX   8192
#define SOC_MEM_KEY_L3_ENTRY_LMEP            4
#define SOC_MEM_KEY_L3_ENTRY_RMEP            5

/*
 * Typedef:
 *      _bcm_oam_group_t
 * Purpose:
 *      Group information.
 */
struct _bcm_oam_group_s
{
    int     in_use;                          /* Group status.   */
    uint8   name[BCM_OAM_GROUP_NAME_LENGTH]; /* Group name.     */
};
typedef struct _bcm_oam_group_s _bcm_oam_group_t;

/*
 * Typedef:
 *      _bcm_oam_endpoint_t
 * Purpose:
 *      Endpoint information.
 *      One structure for each endpoint created on device.
 */
struct _bcm_oam_endpoint_s
{
    bcm_oam_endpoint_type_t type;           /* Type */
    int                 in_use;             /* Endpoint status.             */
    int                 is_remote;          /* Local or Remote MEP.         */
    bcm_oam_group_t     group_index;        /* Group ID.                    */
    uint16              name;               /* Endpoint name.               */
    int                 level;              /* Maintenance domain level.    */
    bcm_vlan_t          vlan;               /* Vlan membership.             */
    uint32              glp;                /* Generic local port number.   */
    int                 local_tx_enabled;   /* CCM Tx enabled.              */
    int                 local_rx_enabled;   /* CCM Rx enabled.              */
    int                 remote_index;       /* RMEP entry hardware index.   */
    int                 local_tx_index;     /* LMEP Tx endpoint Hw index.   */
    int                 local_rx_index;     /* LMEP Rx endpoint Hw index.   */
#if defined(BCM_ENDURO_SUPPORT)
    uint32              vp;                 /* Virtual Port.                */
    uint32              flags;              /* Endpoint flags.              */
    int                 lm_counter_index;   /* LM counter index.            */
    int                 pri_map_index;      /* LM Priority Map Index.       */
    bcm_field_entry_t   vfp_entry;          /* Field Lookup stage entry ID. */
    bcm_field_entry_t   fp_entry_tx;        /* FP LM Tx entry ID.           */
    bcm_field_entry_t   fp_entry_rx;        /* FP LM Rx entry ID.           */
    bcm_field_entry_t   fp_entry_trunk[BCM_SWITCH_TRUNK_MAX_PORTCNT];
                                            /* FP trunk entry ID            */
#endif
#if defined(BCM_KATANA_SUPPORT)
    int                 opcode_profile_index; /* Opcode profile table index.*/
    uint32              opcode_flags;       /* Endpoint opcode flags.       */
#if defined(INCLUDE_BHH)
    bcm_gport_t         gport;              /* gport identifier */
    bcm_if_t            egress_if;          /* egress interface */
    uint8               vlan_pri;           /* VLAN tag priority */
    bcm_mpls_label_t    label;              /* incoming inner label */
    uint8               cpu_qid;            /* CPU queue for BHH */
    bcm_oam_vccv_type_t vccv_type;          /* VCCV pseudowire type */
    int                 bhh_endpoint_index;
    bcm_vpn_t           vpn;
#endif /* INCLUDE_BHH */
#endif /* BCM_KATANA_SUPPORT */
};
typedef struct _bcm_oam_endpoint_s _bcm_oam_endpoint_t;


/*
 * Typedef:
 *      _bcm_oam_event_handler_t
 * Purpose:
 *      Event handler information.
 */
struct _bcm_oam_event_handler_s
{
    bcm_oam_event_types_t           event_types; /* Type of event detected. */
    bcm_oam_event_cb                cb;          /* Registered callback
                                                    routine. */
    void                            *user_data;  /* Application supplied
                                                    data. */
    struct _bcm_oam_event_handler_s *next_p;     /* Pointer to next event. */
};

typedef struct _bcm_oam_event_handler_s _bcm_oam_event_handler_t;


/*
 * Typedef:
 *      _bcm_oam_info_t
 * Purpose:
 *      One structure for each StrataSwitch Device that holds the global
 *      OAM metadata for one device.
 */
struct _bcm_oam_info_s
{
    int                 initialized;            /* TRUE if oam module has 
                                                   been been initialized.   */
    int                 group_count;            /* No. of oam groups supported
                                                   on this device.          */
    _bcm_oam_group_t    *groups;                /* Pointer to oam group name 
                                                   and group status.        */
    int                 local_rx_endpoint_count; /* Total RX LMEP indices
                                                    supported on device.    */
    int                 local_tx_endpoint_count; /* Total TX LMEP indices
                                                    supported on device.    */
    int                 remote_endpoint_count;   /* Total RMEP indices
                                                    supported on device.    */
    int                 endpoint_count;          /* Total no. of endpoints
                                                    supported on device.
                                                    (local_rx + local_tx
                                                    remote).                */
#if defined(BCM_ENDURO_SUPPORT)
    bcm_field_qset_t    vfp_qs;                  /* Vlan and Mac match qset.*/
    bcm_field_qset_t    fp_vp_qs;                /* Virtual port qset.      */
    bcm_field_qset_t    fp_glp_qs;               /* Physical port qset.     */
    bcm_field_group_t   vfp_group;               /* VFP Group ID            */
    bcm_field_group_t   fp_vp_group;             /* IFP Group ID for Logical
                                                    ports                   */
    bcm_field_group_t   fp_glp_group;            /* IFP Group ID for Physical
                                                    Ports*/
    int                 vfp_entry_count;         /* VFP group entries used. */
    int                 fp_vp_entry_count;       /* IFP group entries used. */
    int                 fp_glp_entry_count;      /* IFP group entries used. */
    int                 lm_counter_count;        /* Total Loss Measurement
                                                    counters supported on
                                                    device                  */
    SHR_BITDCL          *lm_counter_in_use;      /* Loss Measurement counters
                                                    used.                   */
#endif
    SHR_BITDCL          *local_tx_endpoints_in_use; /* TX LMEP indices that
                                                       are in use.          */
    SHR_BITDCL          *local_rx_endpoints_in_use; /* RX LMEP indices that
                                                       are in use.          */
    SHR_BITDCL          *remote_endpoints_in_use;   /* RMEP indices that are
                                                       in use.              */
    bcm_oam_endpoint_t  *remote_endpoints;          /* Endpoint ID/Index.   */
    _bcm_oam_endpoint_t *endpoints;                 /* Endpoint information.*/
    _bcm_oam_event_handler_t *event_handler_list_p; /* Event handlers
                                                       callbacks.           */
    int                 event_handler_count[bcmOAMEventCount]; /* Events
                                                                  occurrance
                                                                  count.    */
#if defined(INCLUDE_BHH)
    int                 unit;                   /* Used for callback thread */
    bcm_cos_queue_t cpu_cosq;   /* CPU cos queue for RX BHH packets */
    int cpu_cosq_ach_error_index;   /* CPU cos queue map index for error packets */
    int cpu_cosq_invalid_error_index;   /* CPU cos queue map index for error packets */
    int rx_channel;             /* Local RX DMA channel for BHH packets */
    int uc_num;                 /* uController number running BHH appl */
    int dma_buffer_len;         /* DMA max buffer size */
    uint8* dma_buffer;          /* DMA buffer */
    uint8* dmabuf_reply;        /* DMA reply buffer */
    int    bhh_endpoint_count;  
    sal_thread_t event_thread_id;            /* Event handler thread id */
    int event_thread_kill;                   /* Signal Event thread exit */
    uint32 event_mask;                       /* Events registered with uKernel */
#endif
};

typedef struct _bcm_oam_info_s _bcm_oam_info_t;


/*
 * Typedef:
 *      _bcm_oam_fault_t
 * Purpose:
 *      OAM group defects information structure.
 */
struct _bcm_oam_fault_s
{
    int     current_field;      /* Defects field.            */
    int     sticky_field;       /* Defects sticky field.     */
    uint32  mask;               /* Defects mask bits.        */
    uint32  clear_sticky_mask;  /* Sticky defects mask bits. */
};

typedef struct _bcm_oam_fault_s _bcm_oam_fault_t;


/*
 * Typedef:
 *      _bcm_oam_interrupt_t
 * Purpose:
 *      OAM group interrupt information structure.
 */
struct _bcm_oam_interrupt_s
{
    soc_reg_t            status_register;      /* Interrupt status register.*/
    soc_field_t          endpoint_index_field; /* Remote endpoint index.    */
    soc_field_t          group_index_field;    /* Group index.              */
    soc_field_t          status_field;         /* Interrupt status field.   */
    bcm_oam_event_type_t event_type;           /* Event type.               */
};

typedef  struct _bcm_oam_interrupt_s _bcm_oam_interrupt_t;


/*
 * Typedef:
 *      oam_hdr_t
 * Purpose:
 *      OAM PDU header information structure.
 *      Used in OAM application diagnostics code.
 */
struct oam_hdr_s {
    uint8 mdl_ver;          /* Maintenance domain level + Version.  */
    uint8 opcode;           /* Identifies OAM packet type.          */
    uint8 flags;            /* Infomation depends on OAM PDU type.  */
    uint8 first_tlvoffset;  /* Offset to first TLV.                 */
};

typedef struct oam_hdr_s oam_hdr_t;


/*
 * Typedef:
 *      oam_lm_pkt_t
 * Purpose:
 *      OAM Loss Measurement counter information.
 */
struct oam_lm_pkt_s {
    uint32 txfcf; /* Value of local counter TxFCl. */
    uint32 rxfcf; /* Value of local counter RxFCl. */
    uint32 txfcb; /* Value of TxFCf in the last received CCM frame from peer
                     MEP. */
};

typedef struct oam_lm_pkt_s oam_lm_pkt_t;


/*
 * Typedef:
 *      oam_dm_pkt_t
 * Purpose:
 *      OAM Delay Measurement time stamp information.
 */
struct oam_dm_pkt_s {
    uint32 txtsf_upper; /* Timestamp at the transmission time of DMM frame.  */
    uint32 txtsf;
    uint32 rxtsf_upper; /* Timestamp at the time of receiving the DMM frame. */
    uint32 rxtsf;
    uint32 txtsb_upper; /* Timestamp at the transmission time of DMR frame.  */
    uint32 txtsb;
    uint32 rxtsb_upper; /* Timestamp at the time of receiving the DMR frame. */
    uint32 rxtsb;
};

typedef struct oam_dm_pkt_s oam_dm_pkt_t;

#if defined(BCM_TRIUMPH3_SUPPORT)  ||  \
    defined(BCM_HURRICANE2_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT)
      /* BCM_TRIUMPH3_SUPPORT /BCM_HURRICANE2_SUPPORT/BCM_KATANA2_SUPPORT */

#define _BCM_OAM_ESW_EP_INVALID_FLAGS_MASK              \
            ((BCM_OAM_ENDPOINT_PORT_STATE_TX)           \
             | (BCM_OAM_ENDPOINT_INTERFACE_STATE_TX))

#define _BCM_OAM_REMOTE_EP_INVALID_FLAGS_MASK           \
            ((BCM_OAM_ENDPOINT_CCM_RX)                  \
             | (BCM_OAM_ENDPOINT_LOOPBACK)              \
             | (BCM_OAM_ENDPOINT_DELAY_MEASUREMENT)     \
             | (BCM_OAM_ENDPOINT_LINKTRACE)             \
             | (BCM_OAM_ENDPOINT_LOSS_MEASUREMENT)      \
             | (_BCM_OAM_ESW_EP_INVALID_FLAGS_MASK))

#define _BCM_OAM_EP_RX_ENABLE                      \
            ((BCM_OAM_ENDPOINT_CCM_RX)             \
            | (BCM_OAM_ENDPOINT_LOOPBACK)          \
            | (BCM_OAM_ENDPOINT_DELAY_MEASUREMENT) \
            | (BCM_OAM_ENDPOINT_LOSS_MEASUREMENT)  \
            | (BCM_OAM_ENDPOINT_LINKTRACE))

#define _BCM_OAM_EP_LEVEL_BIT_COUNT (3)

/* Endpoint port TLV mask. */
#define _BCM_OAM_PORT_TLV_MASK      (0x3)

/* Endpoint interface TLV mask. */
#define _BCM_OAM_INTERFACE_TLV_MASK (0x7)

/* Endpoint opcode values mask. */
#define _BCM_TR3_OAM_OPCODE_MASK (0x1ffff)

#ifdef BCM_KATANA2_SUPPORT
/* Endpoint opcode values mask. */
#define _BCM_KT2_OAM_OPCODE_MASK       (0x3ffffff)
#define _BCM_KT2_OAM_OTHER_OPCODE_MASK (0x3ff)
#define _BCM_KT2_MAX_PP_PORT           170
#endif /* BCM_KATANA2_SUPPORT */

#define _BCM_OAM_EP_GLP_VALID           (1 << 0)
#define _BCM_OAM_EP_REPLACE             (1 << 1)
#define _BCM_OAM_EP_RX_IDX_MDL_UPDATE   (1 << 2)

#define _BCM_OAM_LM_COUNTER_IDX_INVALID  (-1)
#if defined(BCM_TRIUMPH3_SUPPORT)
#define _BCM_OAM_LM_COUNTER_TX_OFFSET   (1 << 13)
#endif

#define _BCM_OAM_MAX_MDL 0x07
#define _BCM_OAM_ETHER_TYPE 0x8902

/*
 * Macro: _OAM_HASH_KEY_SIZE
 *
 * Purpose:
 *      OAM endpoint hash table key size.
 */

/* Group ID + Endpoint Name + Level + Direction + Gport + 
                                                  VID/Label(inner and outer) */
#define _OAM_HASH_KEY_SIZE (4 + 4 + 4 + 4 + 4 + 4 + 4)
typedef uint8 _bcm_oam_hash_key_t[_OAM_HASH_KEY_SIZE];

/*
 * Typedef:
 *     _bcm_oam_hash_data_t
 * Purpose:
 *     Endpoint hash table data structure.
 */
struct _bcm_oam_hash_data_s {
    bcm_oam_endpoint_type_t type;           /* Type */
    int                 in_use;             /* Endpoint status.              */
    bcm_oam_endpoint_t  ep_id;              /* Endpoint ID                   */
    uint8               is_remote;          /* Local or Remote MEP.          */
    uint16              name;               /* Endpoint name.                */
    uint8               level;              /* Maintenance domain level.     */
    bcm_vlan_t          vlan;               /* Vlan membership.              */
    bcm_vlan_t          inner_vlan;         /* C Vlan membership -when double
                                                                       tagged*/
    bcm_gport_t         gport;              /* Sw Generic Port.              */
    uint32              sglp;               /* Hw Src GLP CCM Rx port/trunk. */
    uint32              dglp;               /* Hw Dest GLP for CCM Tx.       */
    uint8               local_tx_enabled;   /* CCM Tx enabled.               */
    uint8               local_rx_enabled;   /* CCM Rx enabled.               */
    bcm_oam_group_t     group_index;        /* Group ID.                     */
    int                 remote_index;       /* RMEP entry hardware index.    */
    int                 lm_counter_index;   /* LM counter index.             */
    int                 pri_map_index;      /* LM Priority Map Index.        */
    int                 profile_index;      /* Opcode profile table index.   */
    int                 local_tx_index;     /* LMEP Tx endpoint Hw index.    */
    int                 local_rx_index;     /* LMEP Rx endpoint Hw index.    */
    uint32              vp;                 /* Virtual Port.                 */
    uint32              flags;              /* Endpoint flags.               */
    bcm_field_entry_t   vfp_entry;          /* Field Lookup stage entry ID.  */
    bcm_field_entry_t   fp_entry_tx;        /* FP LM Tx entry ID.            */
    bcm_field_entry_t   fp_entry_rx;        /* FP LM Rx entry ID.            */
    bcm_field_entry_t   fp_entry_trunk[BCM_SWITCH_TRUNK_MAX_PORTCNT];
                                            /* FP trunk entry ID             */
    uint32              period;             /* CCM interval encoding.        */
    uint32              opcode_flags;       /* Endpoint opcode flags.        */
    bcm_oam_timestamp_format_t ts_format;   /* Timestamp format: ntp/ptp     */
#if defined(INCLUDE_BHH)
    bcm_if_t            egress_if;          /* egress interface              */
    uint8               vlan_pri;           /* VLAN tag priority             */
    bcm_mpls_label_t    label;              /* incoming inner label          */
    uint8               cpu_qid;            /* CPU queue for BHH             */
    bcm_oam_vccv_type_t vccv_type;          /* VCCV pseudowire type          */
    int                 bhh_endpoint_index;
    bcm_vpn_t           vpn;
    bcm_field_entry_t   bhh_dm_entry_rx;    /* FP entry for DM msg.          */
    bcm_field_entry_t   bhh_entry_pkt_rx;   /* pkt. count entry in RX direction*/
    bcm_field_entry_t   bhh_entry_pkt_tx;   /* pkt. count entry in TX direction*/
    bcm_field_entry_t   bhh_lm_entry_rx;    /* FP entry for LM msg.          */
#endif /* INCLUDE_BHH */
#if defined(BCM_KATANA2_SUPPORT)
    uint8               oam_domain;         /* Endpoint OAM domain           */
    int                 egr_pri_map_index;  /* LM Priority Map Index.     */
    int                 rx_ctr;             /* Base counter index            */
    int                 tx_ctr;             /* Base counter index            */
    uint32              src_pp_port;        /* Hw PP port                    */
    uint32              dst_pp_port;        /* Hw PP port                    */
    uint32              dglp1;              /* DGLP to redirect              */
    uint32              dglp2;              /* DGLP to redirect              */
    int                 dglp1_profile_index;/* DGLP profile table index.     */
    int                 dglp2_profile_index;/* DGLP profile table index.     */
    int                 outer_tpid;         /* Outer TPID value              */
    int                 inner_tpid;         /* Inner TPID value              */
    int                 subport_tpid;       /* subport TPID value            */
    int                 outer_tpid_profile_index;/* Outer TPID profile index. */
    int                 inner_tpid_profile_index;/* Inner TPID profile index. */
    int                 subport_tpid_profile_index;/* Subport TPID profile index.*/
    int                 ma_base_index;       /* Base Index of the MA entry for 
                                               this maintenance domain       */
    bcm_trunk_t         trunk_id;            /* Trunk ID of the gport        */
    uint8               active_mdl_bitmap;   /* Active MDL bitmap for MA     */ 
#endif /* BCM_KATANA2_SUPPORT */
};
typedef struct _bcm_oam_hash_data_s _bcm_oam_hash_data_t;

/*
 * Typedef:
 *     _bcm_oam_ep_list_t
 * Purpose:
 *     Doubly Link List of endpoint data. This list is created and maintained
 *     for each group.
 */
struct _bcm_oam_ep_list_s {
    _bcm_oam_hash_data_t *ep_data_p;
    struct _bcm_oam_ep_list_s *next;
    struct _bcm_oam_ep_list_s *prev;
};
typedef struct _bcm_oam_ep_list_s _bcm_oam_ep_list_t;

/*
 * Typedef:
 *     _bcm_oam_group_data_t
 * Purpose:
 *     Group information.
 */
struct _bcm_oam_group_data_s
{
    int                in_use;                          /* Group status.   */
    uint8              name[BCM_OAM_GROUP_NAME_LENGTH]; /* Group name.     */
    bcm_oam_group_fault_alarm_defect_priority_t lowest_alarm_priority; /*Alarms
                    are generated for priority greater than or equal to this */
    
    _bcm_oam_ep_list_t **ep_list;                       /* Group endpoints */
                                                        /* list.           */
    
};
typedef struct _bcm_oam_group_data_s _bcm_oam_group_data_t;

/*
 * Typedef:
 *     _bcm_oam_control_t
 * Purpose:
 *     OAM module control structure. One structure for each XGS device.
 */
struct _bcm_oam_control_s {
    int                         init;           /* TRUE if OAM module has    */
                                                /* been initialized.         */
    sal_mutex_t                 oc_lock;        /* Protection mutex          */
    uint32                      ma_idx_count;   /* Max number of Rx          */
                                                /* endpoints supported.      */
    uint32                      egr_ma_idx_count;   /* Max number of Rx      */
                                                /* endpoints supported.      */
    uint32                      rmep_count;     /* Max number of remote      */
                                                /* endpoints supported.      */
    uint32                      lmep_count;     /* Max number of local       */
                                                /* endpoints supported.      */
    uint32                      ep_count;       /* Total number of           */
                                                /* endpoints supported.      */
    uint32                      group_count;    /* Total groups count.       */
    _bcm_oam_group_data_t       *group_info;    /* MA Group information.     */
    shr_idxres_list_handle_t    lmep_pool;      /* Local endpoint indices    */
                                                /* pool.                     */
    shr_idxres_list_handle_t    rmep_pool;      /* Remote endpoint indices   */
                                                /* pool.                     */
    shr_idxres_list_handle_t    ma_idx_pool;    /* Rx endpoint indices pool. */
    /* KT2 OAM */
#ifdef BCM_KATANA2_SUPPORT
    shr_idxres_list_handle_t    egr_ma_idx_pool;/* Egress Rx endpoint indices
                                                   pool - used for UpMEP */
#endif
    shr_idxres_list_handle_t    mep_pool;       /* Endpoint indices pool.    */
    shr_idxres_list_handle_t    group_pool;     /* Group indices pool.       */
    shr_htb_hash_table_t        ma_mep_htbl;    /* Endpoint hash table.      */
    _bcm_oam_hash_data_t        *oam_hash_data; /* Pointer to endpoint hash  */
                                                /* data memory.              */
    soc_profile_mem_t           ing_service_pri_map; /* Ingress service      */
                                                /* priority map profile TAB  */
    soc_profile_mem_t           oam_opcode_control_profile; /* OAM Opcode    */
                                                /* control profile table.    */
    _bcm_oam_event_handler_t    *event_handler_list_p; /* Event handlers     */
                                                /* linked list.              */
    int                         event_handler_cnt[bcmOAMEventCount];
                                                /* No. of registered         */
                                                /* handlers per event.       */
    bcm_field_qset_t            vfp_qs;         /* Vlan and Mac match qset.  */
    bcm_field_qset_t            fp_vp_qs;       /* Virtual port qset.        */
    bcm_field_qset_t            fp_glp_qs;      /* Physical port qset.       */
    bcm_field_group_t           vfp_group;      /* VFP Group ID.             */
    bcm_field_group_t           fp_vp_group;    /* IFP Group ID for Logical  */
                                                /* ports.                    */
    bcm_field_group_t           fp_glp_group;   /* IFP Group ID for physical */
                                                /* Ports. */
    int                         vfp_entry_cnt;  /* VFP group entries used.   */
    int                         fp_vp_entry_cnt; /* IFP group entries used.  */
    int                         fp_glp_entry_cnt; /* IFP group entries used. */
    int                         lm_counter_cnt;  /* Total Loss Measurement.  */
    shr_idxres_list_handle_t    lm_counter_pool;  /* LM counter pool.        */
    bcm_oam_endpoint_t          *remote_endpoints; /* Mapping for remote EP Hw
                                                     index and logical index */
#if defined(INCLUDE_BHH)
    int                         unit;            /* Used for callback thread */
    shr_idxres_list_handle_t    bhh_pool;        /* BHH endpoint indices pool. */
    bcm_cos_queue_t             cpu_cosq;        /* CPU cos queue for RX     */
                                                 /*              BHH packets */
    int                         cpu_cosq_ach_error_index; /* CPU cos queue map*/ 
                                                 /* index for error packets   */
    int                         cpu_cosq_invalid_error_index; /* CPU cos queue*/
                                                /*map index for error packets */
    int                         rx_channel;     /* Local RX DMA channel for   */
                                                /*                BHH packets */
    int                         uc_num;         /* uController number running */
                                                /*                   BHH appl */
    int                         dma_buffer_len; /* DMA max buffer size        */
    uint8*                      dma_buffer;     /* DMA buffer                 */
    uint8*                      dmabuf_reply;   /* DMA reply buffer           */
    int                         bhh_endpoint_count;  
    sal_thread_t                event_thread_id;/* Event handler thread id    */
    int                         event_thread_kill;/* Signal Event thread exit */
    uint32                      event_mask;     /*Events registered with uKernel*/
    bcm_field_data_qualifier_t  bhh_qual_label;  /* mpls label FP qualifier    */
    bcm_field_data_qualifier_t  bhh_qual_ach;    /* ach FP qualifier           */
    bcm_field_data_qualifier_t  bhh_qual_opcode; /* FP opcode qualifier        */
    bcm_field_data_packet_format_t  bhh_pkt_fmt; /* FP pkt format              */
    bcm_field_qset_t            bhh_lmdm_qset;   /* FP qset                    */
    int                         bhh_lmdm_entry_count;/* FP group lmdm entries used*/
    bcm_field_group_t           bhh_lmdm_group;   /* FP Group ID for bhh       */
#endif
    /* KT2 OAM */
#ifdef BCM_KATANA2_SUPPORT
    soc_profile_mem_t           egr_service_pri_map; /* Egress service      */
                                                /* priority map profile TAB  */
    soc_profile_mem_t           egr_oam_opcode_control_profile; /*egr OAM    */
                                                /* Opcode control profile tbl */
    soc_profile_mem_t           ing_oam_dglp_profile; /* Ingress oam dglp    */
                                                      /*profile  */
    soc_profile_mem_t           egr_oam_dglp_profile; /* Egress oam dglp      */
                                                      /* profile */
    shr_aidxres_list_handle_t   ing_lm_ctr_pool[2];/* Ingress LM counter indices
                                                                       pool */
    uint32                      opcode_grp_bmp[8];/* default Bitmap of
                                                         non CFM opcodes  */
    uint32                      opcode_grp_1_bmp[8];/* Bitmap of non CFM opcodes
                                                       in group 1 */
    uint32                      opcode_grp_2_bmp[8]; /* Bitmap of non CFM opcodes
                                                        in group 2 */
   /* No of endpoints using OAM key1 */
    uint16                      oam_key1_ref_count[_BCM_KT2_MAX_PP_PORT];  
 /* No of endpoints using OAM key2 */
    uint16                      oam_key2_ref_count[_BCM_KT2_MAX_PP_PORT];
   /* No of endpoints using OAM key1 */
    uint16                      egr_oam_key1_ref_count[_BCM_KT2_MAX_PP_PORT];  
 /* No of endpoints using OAM key2 */
    uint16                      egr_oam_key2_ref_count[_BCM_KT2_MAX_PP_PORT];
#endif
};
typedef struct _bcm_oam_control_s _bcm_oam_control_t;
#endif /* !BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_KATANA2_SUPPORT
/* MEP LM counter related fields */
typedef struct mep_ctr_info_s {
    soc_field_t  ctr_valid;
    soc_field_t  ctr_base_ptr;
    soc_field_t  ctr_mep_type;
    soc_field_t  ctr_mep_mdl;
    soc_field_t  ctr_profile;
} mep_ctr_info_t;
#endif /* BCM_KATANA2_SUPPORT */


#if defined(INCLUDE_BHH)
#include <soc/shared/bhh_msg.h>
typedef shr_bhh_msg_ctrl_stat_reply_t bcm_bhh_stats_t ;
extern int bcm_en_oam_endpoint_stat_get(
    int unit, 
    bcm_oam_endpoint_t endpoint, 
    bcm_bhh_stats_t *stats);
#endif
#endif /* !__BCM_INT_OAM_H__ */
