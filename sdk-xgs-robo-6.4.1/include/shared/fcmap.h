/*
 * $Id: fcmap.h,v 1.11 Broadcom SDK $
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

#ifndef _SHR_BFCMAP_H
#define _SHR_BFCMAP_H

#ifdef   INCLUDE_FCMAP
#include <sal/types.h>
#include <bfcmap_types.h>
#include <bfcmap_io.h>
#include <bfcmap_err.h>
 

typedef uint32  _shr_bfcmap_dev_addr_t;

/*
 * Attach console/debug functions to BFCMAP module.
 */
typedef bprint_fn _shr_bfcmap_print_fn;

/*
 * Enum for traffic direction. 
 * Ingress traffic is defined as the one coming from the Line side.
 * Egress traffic is defined as coming from the switch/ASIC side.
 */
typedef enum {
        _SHR_BFCMAP_DIR_EGRESS   = 0,
        _SHR_BFCMAP_DIR_INGRESS  = 1,
} _shr_bfcmap_dir_t;

/*
 * Identifiers for BFCMAP core type.
 */
typedef enum {
    _SHR_BFCMAP_CORE_UNKNOWN        = 0,
    _SHR_BFCMAP_CORE_OCTAL_GIG      = 1, /* BCM54580, BCM54584, BCM54585 */
    _SHR_BFCMAP_CORE_BCM5458X       = 2,
    _SHR_BFCMAP_CORE_BCM5458X_B0    = 3,
    _SHR_BFCMAP_CORE_BCM8729        = 4, /* BCM8729   */
    _SHR_BFCMAP_CORE_BCM8483X       = 5, /* BCM8483X  */
    _SHR_BFCMAP_CORE_BCM5438X       = 6, /* BCM5438X  */
    _SHR_BFCMAP_CORE_BCM84756       = 7, /* BCM84756  */
    _SHR_BFCMAP_CORE_BCM84756_C0    = 8, /* BCM84756_C0  */
} _shr_bfcmap_core_t;


/* Port Mode */
typedef enum {
    _SHR_BFCMAP_FCOE_TO_FC_MODE    = 0,
    _SHR_BFCMAP_FCOE_TO_FCOE_MODE  = 1,
} _shr_bfcmap_port_mode_t;

/* Port Speed */
typedef enum {
    _SHR_BFCMAP_PORT_SPEED_AN     = 0,
    _SHR_BFCMAP_PORT_SPEED_2GBPS  = 1,
    _SHR_BFCMAP_PORT_SPEED_4GBPS  = 2,
    _SHR_BFCMAP_PORT_SPEED_8GBPS  = 3,
    _SHR_BFCMAP_PORT_SPEED_16GBPS = 4,
    _SHR_BFCMAP_PORT_SPEED_AN_2GBPS = 5,
    _SHR_BFCMAP_PORT_SPEED_AN_4GBPS = 6,
    _SHR_BFCMAP_PORT_SPEED_AN_8GBPS = 7,
    _SHR_BFCMAP_PORT_SPEED_AN_16GBPS = 8,
    _SHR_BFCMAP_PORT_SPEED_MAX_COUNT
} _shr_bfcmap_port_speed_t;

/* Port State */
typedef enum {
    _SHR_BFCMAP_PORT_STATE_INIT     = 0,         /* INITIALIZATION STATE */
    _SHR_BFCMAP_PORT_STATE_RESET    = 1,         /* PORT IN RESETING STATE */
    _SHR_BFCMAP_PORT_STATE_ACTIVE   = 2,         /* PORT IN ACTIVE(LINK UP) STATE */
    _SHR_BFCMAP_PORT_STATE_LINKDOWN = 3,         /* PORT IN LINK DOWN STATE */
    _SHR_BFCMAP_PORT_STATE_DISABLE  = 4,         /* PORT IN DISABLED STATE */
    _SHR_BFCMAP_PORT_STATE_MAX_COUNT
} _shr_bfcmap_port_state_t;


typedef uint8 _shr_bmac_addr_t[6];


typedef enum {
    _SHR_BFCMAP_ENCAP_FCOE_FPMA             = 0,/* FPMA, use prefix and Node
                                                 ID to construct FCoE MAC Address */
    _SHR_BFCMAP_ENCAP_FCOE_ETH_ADDRESS_NULL = 1,/* Null address for FCOE MAC */
    _SHR_BFCMAP_ENCAP_FCOE_ETH_ADDRESS_USER = 2/* User Provided Address */
} _shr_bfcmap_encap_mac_address_t;


typedef enum {
    _SHR_BFCMAP_8G_FW_ON_ACTIVE_ARBFF  = 0,        /* Use ARBFF as fillword on 8G Active state */
    _SHR_BFCMAP_8G_FW_ON_ACTIVE_IDLE   = 1         /* Use IDLE as fillword on 8G Active state */
}  _shr_bfcmap_8g_fw_on_active_t;

typedef enum {
    _SHR_BFCMAP_MAP_TABLE_INPUT_VID   = 0,         /* Use VID as input for FCMAPPER */
    _SHR_BFCMAP_MAP_TABLE_INPUT_VFID  = 1          /* Use VFID as input for FCMAPPER */
}  _shr_bfcmap_map_table_input_t;

typedef enum {
    _SHR_BFCMAP_FC_CRC_MODE_NORMAL        = 0,     /* normal mode */
    _SHR_BFCMAP_FC_CRC_MODE_NO_CRC_CHECK  = 1      /* no CRC check mode */
}  _shr_bfcmap_fc_crc_mode_t;

typedef enum {
    _SHR_BFCMAP_VFTHDR_PRESERVE           = 0,     /* preserve vfthdr */
    _SHR_BFCMAP_VFTHDR_INSERT             = 1,     /* insert vfthdr */
    _SHR_BFCMAP_VFTHDR_DELETE             = 2      /* delete vfthdr */
}  _shr_bfcmap_vfthdr_proc_mode_t;

typedef enum {
    _SHR_BFCMAP_VLANTAG_PRESERVE           = 0,     /* preserve vlantag */
    _SHR_BFCMAP_VLANTAG_INSERT             = 1,     /* insert vlantag */
    _SHR_BFCMAP_VLANTAG_DELETE             = 2      /* delete vlantag */
}  _shr_bfcmap_vlantag_proc_mode_t;

typedef enum {
    _SHR_BFCMAP_VFID_MACSRC_PASSTHRU      = 0,     /* passthru the vfid */
    _SHR_BFCMAP_VFID_MACSRC_PORT_DEFAULT  = 1,     /* use port default vfid */
    _SHR_BFCMAP_VFID_MACSRC_VID           = 2,     /* use vid if exists*/
    _SHR_BFCMAP_VFID_MACSRC_MAPPER        = 3      /* use FCMAPER ouput if exists */
}  _shr_bfcmap_vfid_mapsrc_t;

typedef enum {
    _SHR_BFCMAP_VID_MACSRC_VFID           = 0,     /* use vfid if exists*/
    _SHR_BFCMAP_VID_MACSRC_PASSTHRU       = 1,     /* passthru the vid */
    _SHR_BFCMAP_VID_MACSRC_PORT_DEFAULT   = 2,     /* use port default vid */
    _SHR_BFCMAP_VID_MACSRC_MAPPER         = 3      /* use FCMAPER ouput if exists */
}  _shr_bfcmap_vid_mapsrc_t;


typedef enum {
    _SHR_BFCMAP_VLAN_PRI_MAP_MODE_PORT_DEFAULT  = 0,     /* use port default priority */
    _SHR_BFCMAP_VLAN_PRI_MAP_MODE_PASSTHRU      = 1,     /* passthru if exists, else use port default priority */
}  _shr_bfcmap_vlan_pri_map_mode_t;

typedef enum {
    _SHR_BFCMAP_HOPCNT_CHECK_MODE_NO_CHK        = 0,      /* no check on hop count at ingress */
    _SHR_BFCMAP_HOPCNT_CHECK_MODE_FAIL_DROP     = 1,      /* discard on hop count == 1 */
    _SHR_BFCMAP_HOPCNT_CHECK_MODE_FAIL_EOFNI    = 2,      /* EOFni on hop count == 1 */
}  _shr_bfcmap_hopcnt_check_mode_t;


#define _SHR_FCMAP_ATTR_PORT_MODE_MASK                        (0x1 << 0) 
#define _SHR_FCMAP_ATTR_SPEED_MASK                            (0x1 << 1)
#define _SHR_FCMAP_ATTR_TX_BB_CREDITS_MASK                    (0x1 << 2)
#define _SHR_FCMAP_ATTR_RX_BB_CREDITS_MASK                    (0x1 << 3)
#define _SHR_FCMAP_ATTR_MAX_FRAME_LENGTH_MASK                 (0x1 << 4)
#define _SHR_FCMAP_ATTR_BB_SC_N_MASK                          (0x1 << 5)
#define _SHR_FCMAP_ATTR_PORT_STATE_MASK                       (0x1 << 6)
#define _SHR_FCMAP_ATTR_R_T_TOV_MASK                          (0x1 << 7)
#define _SHR_FCMAP_ATTR_INTERRUPT_ENABLE_MASK                 (0x1 << 8)
#define _SHR_FCMAP_ATTR_FW_ON_ACTIVE_8G_MASK                  (0x1 << 9)
#define _SHR_FCMAP_ATTR_SRC_MAC_CONSTRUCT_MASK                (0x1 << 10)
#define _SHR_FCMAP_ATTR_DST_MAC_CONSTRUCT_MASK                (0x1 << 11)
#define _SHR_FCMAP_ATTR_VLAN_TAG_MASK                         (0x1 << 12)
#define _SHR_FCMAP_ATTR_VFT_TAG_MASK                          (0x1 << 13)
#define _SHR_FCMAP_ATTR_MAPPER_LEN_MASK                       (0x1 << 14)
#define _SHR_FCMAP_ATTR_INGRESS_MAPPER_BYPASS_MASK            (0x1 << 15)
#define _SHR_FCMAP_ATTR_EGRESS_MAPPER_BYPASS_MASK             (0x1 << 16)
#define _SHR_FCMAP_ATTR_INGRESS_MAP_TABLE_INPUT_MASK          (0x1 << 17)
#define _SHR_FCMAP_ATTR_EGRESS_MAP_TABLE_INPUT_MASK           (0x1 << 18)
#define _SHR_FCMAP_ATTR_INGRESS_FC_CRC_MODE_MASK              (0x1 << 19)
#define _SHR_FCMAP_ATTR_EGRESS_FC_CRC_MODE_MASK               (0x1 << 20)
#define _SHR_FCMAP_ATTR_INGRESS_VFTHDR_PROC_MODE_MASK         (0x1 << 21)
#define _SHR_FCMAP_ATTR_EGRESS_VFTHDR_PROC_MODE_MASK          (0x1 << 22)
#define _SHR_FCMAP_ATTR_INGRESS_VLANTAG_PROC_MODE_MASK        (0x1 << 23)
#define _SHR_FCMAP_ATTR_EGRESS_VLANTAG_PROC_MODE_MASK         (0x1 << 24)
#define _SHR_FCMAP_ATTR_INGRESS_VFID_MAPSRC_MASK              (0x1 << 25)
#define _SHR_FCMAP_ATTR_EGRESS_VFID_MAPSRC_MASK               (0x1 << 26)
#define _SHR_FCMAP_ATTR_INGRESS_VID_MAPSRC_MASK               (0x1 << 27)
#define _SHR_FCMAP_ATTR_EGRESS_VID_MAPSRC_MASK                (0x1 << 28)
#define _SHR_FCMAP_ATTR_INGRESS_VLAN_PRI_MAP_MODE_MASK        (0x1 << 29)
#define _SHR_FCMAP_ATTR_EGRESS_VLAN_PRI_MAP_MODE_MASK         (0x1 << 30)
#define _SHR_FCMAP_ATTR_INGRESS_HOPCNT_CHECK_MODE_MASK        (0x1 << 31)
#define _SHR_FCMAP_ATTR2_EGRESS_HOPCNT_DEC_ENABLE_MASK        (0x1 << 0)


#define _SHR_FCMAP_ATTR_ALL_MASK                        0xFFFFFFFF
#define _SHR_FCMAP_ATTR2_ALL_MASK                       0xFFFFFFFF

/*
 * BFCMAP port configuration structure.
 */
typedef struct _shr_bfcmap_port_config_s {
    uint32 action_mask;                            /* _SHR_FCMAP_ATTR_xxx. */
    uint32 action_mask2;                           /* _SHR_FCMAP_ATTR2_xxx. */
    _shr_bfcmap_port_mode_t  port_mode;            /* FC port Mode, FCoE mode */
    _shr_bfcmap_port_speed_t speed;                /* Speed, 2/4/8/16G */
    int     tx_buffer_to_buffer_credits;           /* Transit B2B credits */
    int     rx_buffer_to_buffer_credits;           /* Receive B2B Credits (read only), computed based on max_frame_length */
    int     max_frame_length;                      /* maximum FC frame length for ingress and egress (in unit of 32-bit word) */
    int     bb_sc_n;                               /* bb credit recovery parameter */
    _shr_bfcmap_port_state_t port_state;           /* Port state, INIT, Reset, Link Up, Link down (read only) */
    int r_t_tov;                      /* Receive, transmit Timeout, default 100ms */
    int interrupt_enable;             /* Interrupt enable */
    _shr_bfcmap_8g_fw_on_active_t   fw_on_active_8g;     /* fillword on 8G active state */ 

    /* The following three fields should be valid when  _SHR_FCMAP_ATTR_SRC_MAC_CONSTRUCT_MASK is set */
    _shr_bfcmap_encap_mac_address_t src_mac_construct;        /* Encap Source MAC construct mode */
    _shr_bmac_addr_t src_mac_addr;    /* Src MAC Addr for encapsulated frame */
    uint32 src_fcmap_prefix;          /* Source FCMAP prefix for the FPMA */

    /* The following three fields should be valid when  _SHR_FCMAP_ATTR_DST_MAC_CONSTRUCT_MASK is set */
    _shr_bfcmap_encap_mac_address_t dst_mac_construct;        /* Encap Dest MAC construct mode */
    _shr_bmac_addr_t dst_mac_addr;    /* Dest MAC Addr for encapsulated frame */
    uint32 dst_fcmap_prefix;          /* Destination FCMAP prefix for the FPMA */

    /* FC Mapper Configuration */
    uint32 vlan_tag;                  /* default vlan for the mapper */
    uint32 vft_tag;                   /* default VFT tag for the mapper */
    int mapper_len;                   /* Size of VLAN-VSAN Mapper table (read only) */
    int ingress_mapper_bypass;        /* Bypass Mapper */
    int egress_mapper_bypass;         /* Bypass Mapper */
    _shr_bfcmap_map_table_input_t     ingress_map_table_input;      /* VID or VFID */
    _shr_bfcmap_map_table_input_t     egress_map_table_input;       /* VID or VFID */
    _shr_bfcmap_fc_crc_mode_t         ingress_fc_crc_mode;          /* Mapper FC-CRC handling */
    _shr_bfcmap_fc_crc_mode_t         egress_fc_crc_mode;           /* Mapper FC-CRC handling */
    _shr_bfcmap_vfthdr_proc_mode_t    ingress_vfthdr_proc_mode;     /* VFT header processing mode */
    _shr_bfcmap_vfthdr_proc_mode_t    egress_vfthdr_proc_mode;      /* VFT header processing mode */
    _shr_bfcmap_vlantag_proc_mode_t   ingress_vlantag_proc_mode;    /* VLAN header processing mode */
    _shr_bfcmap_vlantag_proc_mode_t   egress_vlantag_proc_mode;     /* VLAN header processing mode */
    _shr_bfcmap_vfid_mapsrc_t         ingress_vfid_mapsrc;          /* source of VFID */
    _shr_bfcmap_vfid_mapsrc_t         egress_vfid_mapsrc;           /* source of VFID */
    _shr_bfcmap_vid_mapsrc_t          ingress_vid_mapsrc;           /* source of VID */
    _shr_bfcmap_vid_mapsrc_t          egress_vid_mapsrc;            /* source of VID */
    _shr_bfcmap_vlan_pri_map_mode_t   ingress_vlan_pri_map_mode;    /* processing mode for priority field of vlan tag */
    _shr_bfcmap_vlan_pri_map_mode_t   egress_vlan_pri_map_mode;     /* processing mode for priority field of vlan tag */
    _shr_bfcmap_hopcnt_check_mode_t   ingress_hopCnt_check_mode;    /* mode of VFT hopCnt check on ingress */
    int egress_hopCnt_dec_enable;     /* enable VFT hopCnt decrement on egress */
} _shr_bfcmap_port_config_t;


/*
 * Prototype for Port interation callback function.
 */
typedef int (*_shr_bfcmap_port_traverse_cb)(bfcmap_port_t p, 
                                        _shr_bfcmap_core_t dev_core, 
                                        bfcmap_dev_addr_t dev_addr, 
                                        int dev_port, 
                                        bfcmap_dev_io_f devio_f, 
                                        void *user_data);


/*
 * BFCMAP VLAN - VSAN Mapping
 */
typedef struct _shr_bfcmap_vlan_vsan_map_s {
	uint16   vlan_vid;
	uint16   vsan_vfid;
} _shr_bfcmap_vlan_vsan_map_t;


/*
 * BFCMAP Events that are triggered by the BFCMAP driver.
 */
typedef enum {
    _SHR_BFCMAP_EVENT_FC_LINK_INIT      = 0, /* LINK Initialized */
    _SHR_BFCMAP_EVENT_FC_LINK_RESET     = 1, /* Link Reset received */
    _SHR_BFCMAP_EVENT_FC_LINK_DOWN      = 2, /* Link DOWN */
    _SHR_BFCMAP_EVENT_FC_R_T_TIMEOUT    = 3, /* R_T_Timeout expired */
    _SHR_BFCMAP_EVENT_FC_E_D_TIMEOUT    = 4, /* E_D_Timeout expired */
    _SHR_BFCMAP_EVENT__COUNT = 5
} _shr_bfcmap_event_t;


/*
 * Prototype for the event callback function.
 */
typedef int (*_shr_bfcmap_event_cb_fn)(bfcmap_port_t p,      /* Port ID          */
                                  _shr_bfcmap_event_t event, /* Event            */
                                  void *user_data);         /* secure assoc ID  */


/*
 * The following enums define the stats/couter types.
 */
typedef enum {
        _shr_fc_rxdebug0        = 0,
        _shr_fc_rxdebug1        = 1,
        _shr_fc_rxunicastpkts   = 2,
        _shr_fc_rxgoodframes    = 3,
        _shr_fc_rxbcastpkts     = 4,
        _shr_fc_rxbbcredit0     = 5,
        _shr_fc_rxinvalidcrc    = 6,
        _shr_fc_rxframetoolong  = 7,
        _shr_fc_rxtruncframes   = 8,
        _shr_fc_rxdelimitererr  = 9,
        _shr_fc_rxothererr      = 10,
        _shr_fc_rxruntframes    = 11,
        _shr_fc_rxlipcount      = 12,
        _shr_fc_rxnoscount      = 13,
        _shr_fc_rxerrframes     = 14,
        _shr_fc_rxdropframes    = 15,
        _shr_fc_rxlinkfail      = 16,
        _shr_fc_rxlosssync      = 17,
        _shr_fc_rxlosssig       = 18,
        _shr_fc_rxprimseqerr    = 19,
        _shr_fc_rxinvalidword   = 20,
        _shr_fc_rxinvalidset    = 21,
        _shr_fc_rxencodedisparity = 22,
        _shr_fc_rxbyt           = 23,
        _shr_fc_txdebug0        = 24,
        _shr_fc_txdebug1        = 25,
        _shr_fc_txunicastpkts   = 26,
        _shr_fc_txbcastpkts     = 27,
        _shr_fc_txbbcredit0     = 28,
        _shr_fc_txgoodframes    = 29,
        _shr_fc_txfifounderrun  = 30,
        _shr_fc_txdropframes    = 31,
        _shr_fc_txbyt           = 32,
        _shr_bfcmap_stat__count = 33,
} _shr_bfcmap_stat_t;


/* FC link failure trigger type */
typedef enum {
    _SHR_BFCMAP_LF_TR_NONE = 0x0,
    _SHR_BFCMAP_LF_TR_PORT_INIT = 0x1,
    _SHR_BFCMAP_LF_TR_OPEN_LINK = 0x2,
    _SHR_BFCMAP_LF_TR_LINK_FAILURE = 0x3,
    _SHR_BFCMAP_LF_TR_OLS_RCVD  = 0x4,
    _SHR_BFCMAP_LF_TR_NOS_RCVD = 0x5,
    _SHR_BFCMAP_LF_TR_SYNC_LOSS = 0x6,
    _SHR_BFCMAP_LF_TR_BOUCELINK_FROM_ADMIN = 0x7,
    _SHR_BFCMAP_LF_TR_CHGSPEED_FROM_ADMIN = 0x8,
    _SHR_BFCMAP_LF_TR_DISABLE_FROM_ADMIN = 0x9,
    _SHR_BFCMAP_LF_TR_RESET_FROM_ADMIN = 0xA,
    _SHR_BFCMAP_LF_TR_LR_RCVD = 0xB,
    _SHR_BFCMAP_LF_TR_LRR_RCVD = 0xC,
    _SHR_BFCMAP_LF_TR_ED_TOV = 0xD
}  _shr_bfcmap_lf_tr_t;

/* FC link failure reason code */
typedef enum {
    _SHR_BFCMAP_LF_RC_NONE = 0x0,
    _SHR_BFCMAP_LF_RC_PORT_INIT = 0x1,
    _SHR_BFCMAP_LF_RC_OPEN_LINK = 0x2,
    _SHR_BFCMAP_LF_RC_LINK_FAILURE = 0x3,
    _SHR_BFCMAP_LF_RC_OLS_RCVD  = 0x4,
    _SHR_BFCMAP_LF_RC_NOS_RCVD = 0x5,
    _SHR_BFCMAP_LF_RC_SYNC_LOSS = 0x6,
    _SHR_BFCMAP_LF_RC_BOUCELINK_FROM_ADMIN = 0x7,
    _SHR_BFCMAP_LF_RC_CHGSPEED_FROM_ADMIN = 0x8,
    _SHR_BFCMAP_LF_RC_DISABLE_FROM_ADMIN = 0x9,
    _SHR_BFCMAP_LF_RC_RESET_FAILURE = 0xA
}  _shr_bfcmap_lf_rc_t;


/* FC link failure diagnostic code */
typedef enum {
    _SHR_BFCMAP_DIAG_OK = 0x0,
    _SHR_BFCMAP_DIAG_PORT_INIT = 0x1,
    _SHR_BFCMAP_DIAG_OPEN_LINK = 0x2,
    _SHR_BFCMAP_DIAG_LINK_FAILURE = 0x3,
    _SHR_BFCMAP_DIAG_OLS_RCVD  = 0x4,
    _SHR_BFCMAP_DIAG_NOS_RCVD = 0x5,
    _SHR_BFCMAP_DIAG_SYNC_LOSS = 0x6,
    _SHR_BFCMAP_DIAG_BOUCELINK_FROM_ADMIN = 0x7,
    _SHR_BFCMAP_DIAG_CHGSPEED_FROM_ADMIN = 0x8,
    _SHR_BFCMAP_DIAG_DISABLE_FROM_ADMIN = 0x9,
    _SHR_BFCMAP_DIAG_AN_NO_SIGNAL = 0xA,
    _SHR_BFCMAP_DIAG_AN_TIMEOUT = 0xB,
    _SHR_BFCMAP_DIAG_PROTO_TIMEOUT = 0xC
}  _shr_bfcmap_diag_code_t;



#endif /* INCLUDE_FCMAP */

#endif /* _SHR_BFCMAP_H */

