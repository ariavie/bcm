/*
 * $Id: fcmap.h 1.11 Broadcom SDK $
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
        _SHR_BFCMAP_DIR_INGRESS
} _shr_bfcmap_dir_t;

/*
 * Identifiers for BFCMAP core type.
 */
typedef enum {
    _SHR_BFCMAP_CORE_UNKNOWN = 0,
    _SHR_BFCMAP_CORE_OCTAL_GIG, /* BCM54580, BCM54584, BCM54585 */
    _SHR_BFCMAP_CORE_BCM5458X,
    _SHR_BFCMAP_CORE_BCM5458X_B0,
    _SHR_BFCMAP_CORE_BCM8729,   /* BCM8729   */
    _SHR_BFCMAP_CORE_BCM8483X,  /* BCM8483X  */
    _SHR_BFCMAP_CORE_BCM5438X,  /* BCM5438X  */
    _SHR_BFCMAP_CORE_BCM84756,   /* BCM84756  */
    _SHR_BFCMAP_CORE_BCM84756_C0 /* BCM84756_C0  */
} _shr_bfcmap_core_t;


/* Port Mode */
typedef enum {
    _SHR_BFCMAP_FCOE_TO_FC_MODE=0,
    _SHR_BFCMAP_FCOE_TO_FCOE_MODE
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


/*
 * BFCMAP port configuration structure.
 */
typedef struct _shr_bfcmap_port_config_s {

    _shr_bfcmap_port_mode_t  port_mode;            /* FC port Mode, FCoE mode */
    _shr_bfcmap_port_speed_t speed;                /* Speed, 2/4/8/16G */
    int     tx_buffer_to_buffer_credits;           /* Transit B2B credits */
    int     rx_buffer_to_buffer_credits;           /* Receive B2B Credits (read only), computed based on max_frame_length */
    int     max_frame_length;                      /* maximum FC frame length for ingress and egress (in unit of 32-bit word) */
    int     bb_sc_n;                               /* bb credit recovery parameter */
    _shr_bfcmap_port_state_t port_state;           /* Port state, INIT, Reset, Link Up, Link down (read only) */
    int e_d_tov;                  /* Error Detect timeout, default is 2sec */
    int r_t_tov;                  /* Receive, transmit Timeout, default 100ms */
    int interrupt_enable;         /* Interrupt enable */
    _shr_bfcmap_8g_fw_on_active_t   fw_on_active_8g;     /* fillword on 8G active state */ 
    
    _shr_bmac_addr_t src_mac_addr;    /* Src MAC Addr for encapsulated frame */
    _shr_bmac_addr_t dst_mac_addr;    /* Dest MAC Addr for encapsulated frame */

    uint32 vlan_tag;                  /* vlan for the mapper */
    uint32 src_fcmap_prefix;          /* Source FCMAP prefix for the FPMA */
    uint32 dst_fcmap_prefix;          /* Destination FCMAP prefix for the FPMA */

    /* FC Mapper Configuration */
    int flags;
    int mapper_bypass;            /* Bypass Mapper */
    int mapper_len;               /* Size of VLAN-VSAN Mapper table */
    int fc_mapper_mode;           /* FCoE - FC ; or FCoe - FcoE */
    int mapper_direction;         /* VLAN entry/VSAN entry Mapping */
    int map_table_input;          /* VID  or VFID */
    int fc_crc_mode;              /* Mapper FC-CRC handling */
    int vfthdr_proc_mode;         /* VFT header processing mode */
    int vlantag_proc_mode;       /* VLAN header processing mode */
    _shr_bfcmap_encap_mac_address_t src_mac_construct;        /* Encap Source MAC construct mode */
    _shr_bfcmap_encap_mac_address_t dst_mac_construct;        /* Encap Dest MAC construct mode */

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
    _SHR_BFCMAP_EVENT_FC_LINK_INIT = 0,     /* LINK Initialized */
    _SHR_BFCMAP_EVENT_FC_LINK_RESET,        /* Link Reset received */
    _SHR_BFCMAP_EVENT_FC_LINK_DOWN,         /* Link DOWN */
    _SHR_BFCMAP_EVENT_FC_R_T_TIMEOUT,       /* R_T_Timeout expired */
    _SHR_BFCMAP_EVENT_FC_E_D_TIMEOUT,       /* E_D_Timeout expired */

    _SHR_BFCMAP_EVENT__COUNT
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
        _shr_fc_rxdebug0 = 0,
        _shr_fc_rxdebug1,
        _shr_fc_rxunicastpkts,
        _shr_fc_rxgoodframes,
        _shr_fc_rxbcastpkts,
        _shr_fc_rxbbcredit0,
        _shr_fc_rxinvalidcrc,
        _shr_fc_rxframetoolong,
        _shr_fc_rxtruncframes,
        _shr_fc_rxdelimitererr,
        _shr_fc_rxothererr,
        _shr_fc_rxruntframes,
        _shr_fc_rxlipcount,
        _shr_fc_rxnoscount,
        _shr_fc_rxerrframes,
        _shr_fc_rxdropframes,
        _shr_fc_rxlinkfail,
        _shr_fc_rxlosssync,
        _shr_fc_rxlosssig,
        _shr_fc_rxprimseqerr,
        _shr_fc_rxinvalidword,
        _shr_fc_rxinvalidset,
        _shr_fc_rxencodedisparity,
        _shr_fc_rxbyt,
        _shr_fc_txdebug0,
        _shr_fc_txdebug1,
        _shr_fc_txunicastpkts,
        _shr_fc_txbcastpkts,
        _shr_fc_txbbcredit0,
        _shr_fc_txgoodframes,
        _shr_fc_txfifounderrun,
        _shr_fc_txdropframes,
        _shr_fc_txbyt,

	_shr_bfcmap_stat__count
} _shr_bfcmap_stat_t;


#endif /* INCLUDE_FCMAP */

#endif /* _SHR_BFCMAP_H */

