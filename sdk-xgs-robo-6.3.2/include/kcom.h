/*
 * $Id: kcom.h 1.10 Broadcom SDK $
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
 * File: 	kcom.h
 * Purpose: 	User/Kernel message definitions
 */

#ifndef _KCOM_H
#define _KCOM_H

#include <sal/types.h>

#define KCOM_CHAN_KNET          "KCOM_KNET"

/*
 * Message types
 */
#define KCOM_MSG_TYPE_CMD       1  /* Command */
#define KCOM_MSG_TYPE_RSP       2  /* Command response */
#define KCOM_MSG_TYPE_EVT       3  /* Unsolicited event */


/*
 * Message opcodes
 */
#define KCOM_M_NONE             0  /* Should not be used */
#define KCOM_M_VERSION          1  /* Protocol version */
#define KCOM_M_STRING           2  /* For debug messages */
#define KCOM_M_HW_RESET         3  /* H/W not ready */
#define KCOM_M_HW_INIT          4  /* H/W initialized */
#define KCOM_M_ETH_HW_CONFIG    5  /* ETH HW config*/
#define KCOM_M_NETIF_CREATE     11 /* Create network interface */
#define KCOM_M_NETIF_DESTROY    12 /* Destroy network interface */
#define KCOM_M_NETIF_LIST       13 /* Get list of network interface IDs */
#define KCOM_M_NETIF_GET        14 /* Get network interface info */
#define KCOM_M_FILTER_CREATE    21 /* Create Rx filter */
#define KCOM_M_FILTER_DESTROY   22 /* Destroy Rx filter */
#define KCOM_M_FILTER_LIST      23 /* Get list of Rx filter IDs */
#define KCOM_M_FILTER_GET       24 /* Get Rx filter info */
#define KCOM_M_DMA_INFO         31 /* Tx/Rx DMA info */

#define KCOM_VERSION            4  /* Protocol version */

/*
 * Message status codes
 */
#define KCOM_E_NONE             0  /* No errors */
#define KCOM_E_PARAM            1  /* Invalid/unsupported parameter */
#define KCOM_E_RESOURCE         2  /* Out of memory or other resource */
#define KCOM_E_NOT_FOUND        3  /* Requested object not found */

typedef struct kcom_msg_hdr_s {
    uint8 type;
    uint8 opcode;
    uint8 seqno;
    uint8 status;
    uint8 unit;
    uint8 id;
    uint16 reserved;
} kcom_msg_hdr_t;


/*
 * Object types
 */

/*
 * System network interface
 *
 * Network interface types:
 *
 *  KCOM_NETIF_T_VLAN
 *  Transmits to this interface will go to ingress PIPE of switch
 *  CPU port using specified VLAN ID. Packet will be switched.
 *
 *  KCOM_NETIF_T_PORT
 *  Transmits to this interface will go to unmodified to specified
 *  physical switch port. All switching logic is bypassed.
 *
 *  KCOM_NETIF_T_META
 *  Transmits to this interface will be done using raw meta data
 *  as DMA descriptors. Currently used for RCPU mode only.
 *
 * Network interface flags:
 *
 *  KCOM_NETIF_F_ADD_TAG
 *  Add VLAN tag to packets sent directly to physical port.
 *
 *  KCOM_NETIF_F_RCPU_ENCAP
 *  Use RCPU encapsulation for packets that enter and exit this
 *  interface.
 */
#define KCOM_NETIF_T_VLAN       0
#define KCOM_NETIF_T_PORT       1
#define KCOM_NETIF_T_META       2

#define KCOM_NETIF_F_ADD_TAG    (1U << 0)
#define KCOM_NETIF_F_RCPU_ENCAP (1U << 1)

#define KCOM_NETIF_NAME_MAX     16

typedef struct kcom_netif_s {
    uint8 id;
    uint8 type;
    uint8 flags;
    uint8 port;
    uint16 vlan;
    uint16 qnum;
    uint8 macaddr[6];
    char name[KCOM_NETIF_NAME_MAX];
} kcom_netif_t;

/*
 * Packet filters
 *
 * Filters work like software TCAMs where a mask is applied to the
 * source data, and the result is then compared to the filter data.
 *
 * Filters are checked in priority order with the lowest priority
 * values being checked first (i.e. 0 is the highest priority).
 *
 * Filter types:
 *
 *  KCOM_FILTER_T_RX_PKT
 *  Filter data and mask are applied to the Rx DMA control block
 *  as well as to the Rx packet contents.
 *
 * Destination types:
 *
 *  KCOM_DEST_T_NULL
 *  Packet is dropped.
 *
 *  KCOM_DEST_T_NETIF
 *  Packet is sent to network interface with ID <dest_id>.
 *
 *  KCOM_DEST_T_API
 *  Packet is sent to Rx API through queue <dest_id>.
 *
 * Filter flags:
 *
 *  KCOM_FILTER_F_ANY_DATA
 *  When this flags is set the filter will match any packet on
 *  the associated unit.
 *
 *  KCOM_FILTER_F_STRIP_TAG
 *  Strip VLAN tag before packet is sent to destination.
 *  This flag only applies to KCOM_DEST_T_NETIF.
 *
 */
#define KCOM_FILTER_BYTES_MAX   256
#define KCOM_FILTER_WORDS_MAX   BYTES2WORDS(KCOM_FILTER_BYTES_MAX)

#define KCOM_FILTER_T_RX_PKT    1

#define KCOM_DEST_T_NULL        0
#define KCOM_DEST_T_NETIF       1
#define KCOM_DEST_T_API         2

#define KCOM_FILTER_F_ANY_DATA  (1U << 0)
#define KCOM_FILTER_F_STRIP_TAG (1U << 1)

#define KCOM_FILTER_DESC_MAX    32

typedef struct kcom_filter_s {
    uint8 id;
    uint8 type;
    uint8 priority;
    uint8 reserved;
    char desc[KCOM_FILTER_DESC_MAX];
    uint32 flags;
    uint16 dest_type;
    uint16 dest_id;
    uint16 mirror_type;
    uint16 mirror_id;
    uint16 oob_data_offset;
    uint16 oob_data_size;
    uint16 pkt_data_offset;
    uint16 pkt_data_size;
    union {
        uint8 b[KCOM_FILTER_BYTES_MAX];
        uint32 w[KCOM_FILTER_WORDS_MAX];
    } data;
    union {
        uint8 b[KCOM_FILTER_BYTES_MAX];
        uint32 w[KCOM_FILTER_WORDS_MAX];
    } mask;
} kcom_filter_t;

/*
 * DMA buffer information
 *
 * Cookie field is reserved use by application (32/64-bit pointer).
 *
 * For Tx operation the application will submit the start address of
 * the Tx DCB chain which is queued for transfer by the kernel module.
 * Once DMA is done a DMA event is returned to the application with an
 * optional sequence number.
 *
 * For Rx operation the application will submit the start address of
 * the Rx DCB chain which should be use for packet reception by the
 * kernel module. Once DMA is done a DMA event is returned to the
 * application with an optional sequence number.
 *
 * Cookie field is reserved use by application (32/64-bit pointer).
 *
 * Packet info types:
 *
 *  KCOM_DMA_INFO_T_TX_DCB
 *  Data is physical start address of Tx DCB chain.
 *
 *  KCOM_DMA_INFO_T_RX_DCB
 *  Data is physical start address of Rx DCB chain.
 *
 * Packet info flags:
 *
 *  KCOM_DMA_INFO_F_TX_DONE
 *  This flag is set by the kernel module and means that one or more
 *  packets have been sent.
 *
 *  KCOM_DMA_INFO_F_RX_DONE
 *  This flag is set by the kernel module and means that one or more
 *  Rx buffers contain valid packet data.
 */
#define KCOM_DMA_INFO_T_TX_DCB  1
#define KCOM_DMA_INFO_T_RX_DCB  2

#define KCOM_DMA_INFO_F_TX_DONE (1U << 0)
#define KCOM_DMA_INFO_F_RX_DONE (1U << 1)

typedef struct kcom_dma_info_s {
    uint8 type;
    uint8 cnt;
    uint16 size;
    uint16 chan;
    uint16 flags;
    union {
        void *p;
        uint8 b[8];
    } cookie;
    union {
        uint32 dcb_start;
        struct {
            uint32 tx;
            uint32 rx;
        } seqno;
    } data;
  } kcom_dma_info_t;



#define KCOM_ETH_HW_T_RESET     1
#define KCOM_ETH_HW_T_INIT      2
#define KCOM_ETH_HW_T_OTHER     3

#define KCOM_ETH_HW_C_ALL   0xff

#define KCOM_ETH_HW_RESET_F_TX     (1U << 0)
#define KCOM_ETH_HW_RESET_F_RX     (1U << 1)
#define KCOM_ETH_HW_RESET_F_TX_RECLAIM     (1U << 2)
#define KCOM_ETH_HW_RESET_F_RX_RECLAIM     (1U << 3)

#define KCOM_ETH_HW_INIT_F_TX      (1U << 0)
#define KCOM_ETH_HW_INIT_F_RX      (1U << 1)
#define KCOM_ETH_HW_INIT_F_RX_FILL  (1U << 2)


#define KCOM_ETH_HW_OTHER_F_FIFO_LOOPBACK     (1U << 0)
#define KCOM_ETH_HW_OTHER_F_INTERRUPT    (1U << 1)




typedef struct kcom_eth_hw_config_s {
    uint8 type;
    uint8 chan;
    uint32 flags;
    uint32 value;
  } kcom_eth_hw_config_t;

/*
 * Message types
 */

/*
 * Request KCOM interface version of kernel module.
 */
typedef struct kcom_msg_version_s {
    kcom_msg_hdr_t hdr;
    uint32 version;
} kcom_msg_version_t;

/*
 * Send literal string to/from kernel module.
 * Mainly for debugging purposes.
 */
#define KCOM_MSG_STRING_MAX     128

typedef struct kcom_msg_string_s {
    kcom_msg_hdr_t hdr;
    uint32 len;
    char val[KCOM_MSG_STRING_MAX];
} kcom_msg_string_t;


/*
 * Indicate that eth hardware is about to be reset. Active
 * DMA operations should be aborted and DMA and interrupts
 * should be disabled.
 */
/*
 * Indicate that eth hardware has been properly initialized
 * for DMA operation to commence.
 */ 
typedef struct kcom_msg_eth_hw_config_s {
    kcom_msg_hdr_t hdr;
    kcom_eth_hw_config_t config;
} kcom_msg_eth_hw_config_t;


/*
 * Indicate that switch hardware is about to be reset. Active
 * DMA operations should be aborted and DMA and interrupts
 * should be disabled.
 */
typedef struct kcom_msg_hw_reset_s {
    kcom_msg_hdr_t hdr;
    uint32 channels;
} kcom_msg_hw_reset_t;

/*
 * Indicate that switch hardware has been properly initialized
 * for DMA operation to commence.
 */
typedef struct kcom_msg_hw_init_s {
    kcom_msg_hdr_t hdr;
    uint16 dcb_size;
    uint16 dcb_type;
} kcom_msg_hw_init_t;

/*
 * Create new system network interface. The network interface will
 * be associated with the specified switch unit number.
 * The interface id and name will be assigned by the kernel module.
 */
typedef struct kcom_msg_netif_create_s {
    kcom_msg_hdr_t hdr;
    kcom_netif_t netif;
} kcom_msg_netif_create_t;

/*
 * Destroy system network interface.
 */
typedef struct kcom_msg_netif_destroy_s {
    kcom_msg_hdr_t hdr;
} kcom_msg_netif_destroy_t;

/*
 * Get list of currently defined system network interfaces.
 */
#define KCOM_NETIF_MAX          128

typedef struct kcom_msg_netif_list_s {
    kcom_msg_hdr_t hdr;
    uint32 ifcnt;
    uint8 id[KCOM_NETIF_MAX];
} kcom_msg_netif_list_t;

/*
 * Get detailed network interface information.
 */
typedef struct kcom_msg_netif_get_s {
    kcom_msg_hdr_t hdr;
    kcom_netif_t netif;
} kcom_msg_netif_get_t;

/*
 * Create new packet filter.
 * The filter id will be assigned by the kernel module.
 */
typedef struct kcom_msg_filter_create_s {
    kcom_msg_hdr_t hdr;
    kcom_filter_t filter;
} kcom_msg_filter_create_t;

/*
 * Destroy packet filter.
 */
typedef struct kcom_msg_filter_destroy_s {
    kcom_msg_hdr_t hdr;
} kcom_msg_filter_destroy_t;

/*
 * Get list of currently defined packet filters.
 */
#define KCOM_FILTER_MAX  128

typedef struct kcom_msg_filter_list_s {
    kcom_msg_hdr_t hdr;
    uint32 fcnt;
    uint8 id[KCOM_FILTER_MAX];
} kcom_msg_filter_list_t;

/*
 * Get detailed packet filter information.
 */
typedef struct kcom_msg_filter_get_s {
    kcom_msg_hdr_t hdr;
    kcom_filter_t filter;
} kcom_msg_filter_get_t;

/*
 * DMA info
 */
typedef struct kcom_msg_dma_info_s {
    kcom_msg_hdr_t hdr;
    kcom_dma_info_t dma_info;
} kcom_msg_dma_info_t;


/*
 * All messages (e.g. for generic receive)
 */

typedef union kcom_msg_s {
    kcom_msg_hdr_t hdr;
    kcom_msg_version_t version;
    kcom_msg_string_t string;
    kcom_msg_hw_reset_t hw_reset;
    kcom_msg_hw_init_t hw_init;
    kcom_msg_eth_hw_config_t eth_hw_config;
    kcom_msg_netif_create_t netif_create;
    kcom_msg_netif_destroy_t netif_destroy;
    kcom_msg_netif_list_t netif_list;
    kcom_msg_netif_get_t netif_get;
    kcom_msg_filter_create_t filter_create;
    kcom_msg_filter_destroy_t filter_destroy;
    kcom_msg_filter_list_t filter_list;
    kcom_msg_filter_get_t filter_get;
    kcom_msg_dma_info_t dma_info;
} kcom_msg_t;


/*
 * KCOM communication channel vectors
 *
 * open
 * Open KCOM channel.
 *
 * close
 * Close KCOM channel.
 *
 * send
 * Send KCOM message. If bufsz is non-zero, a synchronous send will be
 * performed (if supported) and the function will return the number of
 * bytes in the response.
 *
 * recv
 * Receive KCOM message. This function is used t oreceive unsolicited
 * messages from the kernel. If synchronous send is not supported, this
 * function is also used to retrieve responses to command messages.
 */

typedef struct kcom_chan_s {
    void *(*open)(char *name);
    int (*close)(void *handle);
    int (*send)(void *handle, void *msg, unsigned int len, unsigned int bufsz);
    int (*recv)(void *handle, void *msg, unsigned int bufsz);
} kcom_chan_t;

#endif	/* _KCOM_H */
