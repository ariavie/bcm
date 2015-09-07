/* 
 * $Id: bfd.c,v 1.22 Broadcom SDK $
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
 * File:        bfd.c
 * Purpose:     XGS5 Bidirectional Forwarding Detection common driver.
 *
 * Notes:      BFD functions will return BCM_E_UAVAIL unless these conditions
 *             are true:
 *               - BCM_CMICM_SUPPORT
 *               - soc_feature_cmicm
 */

#include <shared/bsl.h>

#include <soc/defs.h>

#if defined(INCLUDE_BFD)
#include <sal/core/libc.h>
#include <shared/pack.h>
#include <soc/drv.h>
#include <soc/hash.h>
#include <soc/l3x.h>
#include <soc/higig.h>
#include <soc/uc_msg.h>
#include <soc/shared/mos_msg_common.h>
#include <soc/shared/bfd.h>
#include <soc/shared/bfd_pkt.h>
#include <soc/shared/bfd_msg.h>
#include <soc/shared/bfd_pack.h>

#include <bcm_int/common/rx.h>
#include <bcm_int/api_xlate_port.h>
#include <bcm_int/esw/bfd.h>
#include <bcm_int/esw/xgs5.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw/triumph.h>
#include <bcm/error.h>
#include <bcm/bfd.h>

/*
 * BFD HW Definition Table
 */
static bcm_xgs5_bfd_hw_defs_t  *bcm_xgs5_bfd_hw_defs[BCM_MAX_NUM_UNITS] = {0};

/* Maximum Memory Entry */
typedef uint32 max_mem_entry_t[SOC_MAX_MEM_WORDS];

#define BFD_HW(u_)                (bcm_xgs5_bfd_hw_defs[(u_)])
#define BFD_HW_CALL(u_)           (bcm_xgs5_bfd_hw_defs[(u_)]->hw_call)
#define BFD_HW_L2(u_)             (bcm_xgs5_bfd_hw_defs[(u_)]->l2)
#define BFD_HW_L3_IPV4(u_)        (bcm_xgs5_bfd_hw_defs[(u_)]->l3_ipv4)
#define BFD_HW_L3_IPV6(u_)        (bcm_xgs5_bfd_hw_defs[(u_)]->l3_ipv6)
#define BFD_HW_L3_TUNNEL(u_)      (bcm_xgs5_bfd_hw_defs[(u_)]->l3_tunnel)
#define BFD_HW_MPLS(u_)           (bcm_xgs5_bfd_hw_defs[(u_)]->mpls)

#define BFD_HW_L2_MEM(u_)         (BFD_HW_L2((u_))->mem)
#define BFD_HW_L3_IPV4_MEM(u_)    (BFD_HW_L3_IPV4((u_))->mem)
#define BFD_HW_L3_IPV6_MEM(u_)    (BFD_HW_L3_IPV6((u_))->mem)
#define BFD_HW_L3_TUNNEL_MEM(u_)  (BFD_HW_L3_TUNNEL((u_))->mem)
#define BFD_HW_MPLS_MEM(u_)       (BFD_HW_MPLS((u_))->mem)


/*
 * BFD Information SW Data
 */

/* BFD Event Handler */
typedef struct bfd_event_handler_s {
    struct bfd_event_handler_s *next;
    bcm_bfd_event_types_t event_types;
    bcm_bfd_event_cb cb;
    void *user_data;
} bfd_event_handler_t;

/* BFD Endpoint Configuration SW Data Structure */
typedef struct bfd_endpoint_config_s {
    int endpoint_index;
    bcm_module_t  modid;    /* Destination module ID */
    bcm_port_t port;        /* Destination port */
    bcm_port_t tx_port;     /* Local physical port to TX BFD packet */
    int encap_type;         /* Raw, UDP-IPv4/IPv6, used for UDP checksum */
    int encap_length;       /* BFD Encapsulation length */
    int lkey_etype;         /* Lookup key Ether Type */
    int lkey_offset;        /* Lookup key offset */
    int lkey_length;        /* Lookup key length */
    bcm_bfd_endpoint_info_t info;
    int is_l3_vpn;          /* 1 for L3 VPN, 0 for L2 VPN */ 
} bfd_endpoint_config_t;

/* BFD Information SW Data Structure */
typedef struct bfd_info_s {
    int unit;                   /* Unit number */
    int initialized;            /* If set, BFD has been initialized */
    int endpoint_count;         /* Max number of BFD endpoints */
    int num_auth_sha1_keys;     /* Max number of sha1 auth keys */
    int num_auth_sp_keys;       /* Max number of simple pwd auth keys */
    bcm_cos_queue_t cpu_cosq;   /* CPU cos queue for RX BFD packets */
    int cosq_spath_index;       /* CPU cos queue map index for good packets */
    int cosq_ach_err_index;     /* CPU cos queue map index for ACH error */
    int rx_channel;             /* Local RX DMA channel for BFD packets */
    int uc_num;                 /* uController number running BFD appl */
    int dma_buffer_len;         /* DMA max buffer size */
    uint8* dma_buffer;          /* DMA buffer */
    SHR_BITDCL *endpoints_in_use;   /* Indicates used endpoint indexes */
    bfd_endpoint_config_t *endpoints;  /* Array to Endpoints */
    bcm_bfd_auth_sha1_t *auth_sha1; /* Array of sha1 auth keys */
    bcm_bfd_auth_simple_password_t *auth_sp; /* Array simple pwd auth keys */
    sal_thread_t event_thread_id;            /* Event handler thread id */
    int event_thread_kill;                   /* Signal Event thread exit */
    bfd_event_handler_t *event_handler_list; /* List of event
                                                         handlers */
    uint32 event_mask;                       /* Mask of all events */
} bfd_info_t;

static bfd_info_t *bcm_xgs5_bfd_info[BCM_MAX_NUM_UNITS] = {0};

#define BFD_INFO(u_)                     (bcm_xgs5_bfd_info[(u_)])
#define BFD_ENDPOINT_CONFIG(u_, index_)  \
    (&(BFD_INFO((u_))->endpoints[(index_)]))

/*
 * Utility macros
 */
/* True if BFD module has been initialized */
#define BFD_INIT(u_)                                        \
    ((BFD_INFO(u_) != NULL) && (BFD_INFO(u_)->initialized))

/* Checks for 'null' argument */
#define PARAM_NULL_CHECK(arg_)  \
    if ((arg_) == NULL) {       \
        return BCM_E_PARAM;     \
    }

/* Checks that required feature(s) is available */
#define FEATURE_CHECK(u_)                        \
    if (!soc_feature((u_), soc_feature_cmicm)) { \
        return BCM_E_UNAVAIL;                    \
    } 

/* Checks that BFD module has been initialized */
#define BFD_INIT_CHECK(u_)                                          \
    if ((BFD_INFO(u_) == NULL) || (!BFD_INFO(u_)->initialized)) {   \
        return BCM_E_INIT;                  \
    }

/* Checks that required feature(s) is available and BFD has been initialized */
#define BFD_FEATURE_INIT_CHECK(u_)              \
    do {                                        \
        FEATURE_CHECK(u_);                      \
        BFD_INIT_CHECK(u_);                     \
    } while (0)


/* Checks that endpoint index is within valid range */
#define ENDPOINT_INDEX_CHECK(u_, index_)                                \
    if ((index_) < 0 || (index_) >= BFD_INFO(u_)->endpoint_count) {     \
        return BCM_E_PARAM;                                             \
    }

/* Checks that SHA1 Authentication index is valid */
#define AUTH_SHA1_INDEX_CHECK(u_, index_)                               \
    if ((index_) >= BFD_INFO(u_)->num_auth_sha1_keys) {                 \
        return BCM_E_PARAM;                                             \
    }

/* Checks that Simple Password Authentication index is valid */
#define AUTH_SP_INDEX_CHECK(u_, index_)                                 \
    if ((index_) >= BFD_INFO(u_)->num_auth_sp_keys) {                   \
        return BCM_E_PARAM;                                             \
    }

#undef BFD_DEBUG_DUMP

#ifdef BCM_CMICM_SUPPORT

#define BFD_SDK_VERSION         0x01000001
#define BFD_UC_MIN_VERSION      0x01000200

#define BFD_COSQ_INVALID        0xffff

#define MPLS_ACTION_POP_L3_IIF   2

/*
 * Maximum number of BFD endpoints
 * Endpoint IDs valid range is  [0..2046]
 * Entry 0x7FF (2047) is reserved for unknown remote discriminator
 */
#define BFD_MAX_NUM_ENDPOINTS             2047

/* Timeout for Host <--> uC message */
#define BFD_UC_MSG_TIMEOUT_USECS             20000000 /* 20 secs */
#define BFD_UC_MSG_TIMEOUT_USECS_QUICKTURN  300000000 /* 5 mins */

static sal_usecs_t bfd_uc_msg_timeout_usecs = BFD_UC_MSG_TIMEOUT_USECS;


/*
 * BFD Encapsulation Format Header flags
 *
 * Indicates the type of headers/labels present in a BFD packet.
 */
#define BFD_ENCAP_PKT_MPLS                    (1 << 0) 
#define BFD_ENCAP_PKT_MPLS_ROUTER_ALERT       (1 << 1) 
#define BFD_ENCAP_PKT_PW                      (1 << 2) 
#define BFD_ENCAP_PKT_GAL                     (1 << 3) 
#define BFD_ENCAP_PKT_G_ACH                   (1 << 4) 
#define BFD_ENCAP_PKT_GRE                     (1 << 5) 
#define BFD_ENCAP_PKT_INNER_IP                (1 << 6)
#define BFD_ENCAP_PKT_IP                      (1 << 7) 
#define BFD_ENCAP_PKT_UDP                     (1 << 8) 
#define BFD_ENCAP_PKT_BFD                     (1 << 9)
#define BFD_ENCAP_PKT_HG                      (1 << 10)
#define BFD_ENCAP_PKT_HG2                     (1 << 11)

#define BFD_ENCAP_PKT_UDP__MULTI_HOP          (1 << 16) 
#define BFD_ENCAP_PKT_INNER_IP__V6            (1 << 17)
#define BFD_ENCAP_PKT_IP__V6                  (1 << 18) 
#define BFD_ENCAP_PKT_G_ACH__IP               (1 << 19)
#define BFD_ENCAP_PKT_G_ACH__CC               (1 << 20)
#define BFD_ENCAP_PKT_G_ACH__CC_CV            (1 << 21)


/*
 * BFD Encapsulation Definitions
 *
 * Defines for building the BFD packet encapsulation
 */

/*
 * Macros to pack uint8, uint16, uint32 in Network byte order
 */
#define BFD_ENCAP_PACK_U8(buf_, var_)   SHR_BFD_ENCAP_PACK_U8(buf_, var_)
#define BFD_ENCAP_PACK_U16(buf_, var_)  SHR_BFD_ENCAP_PACK_U16(buf_, var_)
#define BFD_ENCAP_PACK_U32(buf_, var_)  SHR_BFD_ENCAP_PACK_U32(buf_, var_)

#define BFD_MAC_ADDR_LENGTH    (sizeof(bcm_mac_t))

#define BFD_MPLS_MAX_LABELS    3   /* Max MPLS labels */

#define BFD_DEFAULT_DETECT_MULTIPLIER   10 /* BFD Default Detection Time multiplier */    

/* UDP Header */
typedef struct bfd_udp_header_s {  /* <num bits>: ... */
    uint16  src;         /* 16: Source Port Number */
    uint16  dst;         /* 16: Destination Port Number */
    uint16  length;      /* 16: Length */
    uint16  sum;         /* 16: Checksum */
} bfd_udp_header_t;

/* IPv4 Header */
typedef struct bfd_ipv4_header_s {
    uint8   version;     /*  4: Version */
    uint8   h_length;    /*  4: Header length */
    uint8   dscp;        /*  6: Differentiated Services Code Point */
    uint8   ecn;         /*  2: Explicit Congestion Notification */
    uint16  length;      /* 16: Total Length bytes (header + data) */
    uint16  id;          /* 16: Identification */
    uint8   flags;       /*  3: Flags */
    uint16  f_offset;    /* 13: Fragment Offset */
    uint8   ttl;         /*  8: Time to Live */
    uint8   protocol;    /*  8: Protocol */
    uint16  sum;         /* 16: Header Checksum */
    uint32  src;         /* 32: Source IP Address */
    uint32  dst;         /* 32: Destination IP Address */
} bfd_ipv4_header_t;

/* IPv6 Header */
typedef struct bfd_ipv6_header_s {
    uint8      version;        /*   4: Version */
    uint8      t_class;        /*   8: Traffic Class (6:dscp, 2:ecn) */
    uint32     f_label;        /*  20: Flow Label */
    uint16     p_length;       /*  16: Payload Length */
    uint8      next_header;    /*   8: Next Header */
    uint8      hop_limit;      /*   8: Hop Limit */
    bcm_ip6_t  src;            /* 128: Source IP Address */
    bcm_ip6_t  dst;            /* 128: Destination IP Address */
} bfd_ipv6_header_t;

/* GRE - Generic Routing Encapsulation */
typedef struct bfd_gre_header_s {
    uint8   checksum;           /*  1: Checksum present */
    uint8   routing;            /*  1: Routing present */
    uint8   key;                /*  1: Key present */
    uint8   sequence_num;       /*  1: Sequence number present */
    uint8   strict_src_route;   /*  1: Strict Source Routing */
    uint8   recursion_control;  /*  3: Recursion Control */
    uint8   flags;              /*  5: Flags */
    uint8   version;            /*  3: Version */
    uint16  protocol;           /* 16: Protocol Type */
} bfd_gre_header_t;

/* ACH - Associated Channel Header */
typedef struct bfd_ach_header_s {
    uint8   f_nibble;        /*  4: First nibble, must be 1 */
    uint8   version;         /*  4: Version */
    uint8   reserved;        /*  8: Reserved */
    uint16  channel_type;    /* 16: Channel Type */
} bfd_ach_header_t;

/* MPLS - Multiprotocol Label Switching Label */
typedef struct bfd_mpls_label_s {
    uint32  label;    /* 20: Label */
    uint8   exp;      /*  3: Experimental, Traffic Class, ECN */
    uint8   s;        /*  1: Bottom of Stack */
    uint8   ttl;      /*  8: Time to Live */
} bfd_mpls_label_t;

/* VLAN Tag - 802.1Q */
typedef struct bfd_vlan_tag_s {
    uint16      tpid;    /* 16: Tag Protocol Identifier */
    struct {
        uint8   prio;    /*  3: Priority Code Point */
        uint8   cfi;     /*  1: Canonical Format Indicator */
        uint16  vid;     /* 12: Vlan Identifier */
    } tci;               /* Tag Control Identifier */
} bfd_vlan_tag_t;

/* L2 Header */
typedef struct bfd_l2_header_t {
    bcm_mac_t       dst_mac;      /* 48: Destination MAC */
    bcm_mac_t       src_mac;      /* 48: Source MAC */
    bfd_vlan_tag_t  outer_vlan_tag;    /* VLAN Tag */
    bfd_vlan_tag_t  inner_vlan_tag;    /* inner VLAN Tag */
    uint16          etype;        /* 16: Ether Type */
} bfd_l2_header_t;


/*
 * UDP Definitions for BFD
 */
#define BFD_UDP_SRCPORT_MIN                49152
#define BFD_UDP_SRCPORT_MAX                65535
#define ENCAP_UDP_SRCPORT_VALIDATE(a_)                                  \
    if (((a_) < BFD_UDP_SRCPORT_MIN) || ((a_) > BFD_UDP_SRCPORT_MAX)) { \
        return BCM_E_PARAM;                                             \
    }

/*
 * IPV4
 *
 * Loopback    127.0.0.0/8         127.0.0.0   - 127.255.255.255
 *
 * Reserved
 *   Class A   10.0.0.0/8          10.0.0.0    - 10.255.255.255
 *   Class B   172.16.0.0/12       172.16.0.0  - 172.31.255.255
 *   Class C   192.168.0.0/16      192.168.0.0 - 192.168.255.255  
 *   Class E   240.0.0.0
 */
#define IPV4_LOOPBACK(a_)    (127 == (((a_) >> 24) & 0xff))

#define IPV4_RESERVED_CLASS_A(a_)  ((((a_) >> 24) & 0xff) == 10)
#define IPV4_RESERVED_CLASS_B(a_)  (((((a_) >> 24) & 0xff) == 172) &&   \
                                    ((((a_) >> 16) & 0xf0) == 16))
#define IPV4_RESERVED_CLASS_C(a_)  (((((a_) >> 24) & 0xff) == 192) &&   \
                                    ((((a_) >> 16) & 0xff) == 168))
#define IPV4_RESERVED_CLASS_E(a_)  ((((a_) >> 24) & 0xff) == 240)

#define IPV4_RESERVED(a_)                                               \
    (IPV4_RESERVED_CLASS_A(a_) || IPV4_RESERVED_CLASS_B(a_) ||          \
     IPV4_RESERVED_CLASS_C(a_) || IPV4_RESERVED_CLASS_E(a_))

#define ENCAP_IPV4_LOOPBACK_VALIDATE(a_)            \
    if (!IPV4_LOOPBACK(a_)) { return BCM_E_PARAM; }

#define ENCAP_IPV4_ROUTABLE_VALIDATE(a_)            \
    if (IPV4_RESERVED(a_)) { return BCM_E_PARAM; } 

/*
 * IPV6
 * Loopback    0:0:0:0:0:FFFF:127/104
 *
 * Reserved
 *    fc00::/7
 */
#define IPV6_LOOPBACK(a_)                                                    \
    (((a_)[0] == 0) && ((a_)[1] == 0) && ((a_)[2] == 0) && ((a_)[3] == 0) && \
     ((a_)[4] == 0) && ((a_)[5] == 0) && ((a_)[6] == 0) && ((a_)[7] == 0) && \
     ((a_)[8] == 0) && ((a_)[9] == 0) &&                                     \
     ((a_)[10] == 0xff) && ((a_)[11] == 0xff) && ((a_)[12] == 127))
     
#define IPV6_RESERVED(a_)    (((a_)[0] & 0xfe) == 0xfc)

#define ENCAP_IPV6_LOOPBACK_VALIDATE(a_)            \
    if (!IPV6_LOOPBACK(a_)) { return BCM_E_PARAM; }

#define ENCAP_IPV6_ROUTABLE_VALIDATE(a_)            \
    if (IPV6_RESERVED(a_)) { return BCM_E_PARAM; } 

#ifdef BCM_WARM_BOOT_SUPPORT
#define BCM_WB_VERSION_1_0      SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION  BCM_WB_VERSION_1_0

STATIC int
bcmi_xgs5_bfd_scache_alloc(int unit);

STATIC int
bcmi_xgs5_bfd_reinit(int unit);
#endif

/*
 * Function:
 *      bcmi_xgs5_bfd_alloc_clear
 * Purpose:
 *      Allocate memory block and set memory content to zeros.
 * Parameters:
 *      size        - (IN) Size of memory to allocate.
 *      description - (IN) Description of memory block.
 * Returns:
 *      Pointer to memory block.
 */
STATIC void *
bcmi_xgs5_bfd_alloc_clear(unsigned int size, char *description)
{
    void *block = NULL;

    if (size) {
        block = sal_alloc(size, description);
        if (block != NULL) {
            sal_memset(block, 0, size);
        }
    }

    return block;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_free_resource
 * Purpose:
 *      Free memory resources allocated for given unit.
 * Parameters:
 *      unit       - (IN) Unit number.
 * Returns:
 *      None
 */
STATIC void
bcmi_xgs5_bfd_free_resource(int unit)
{
    bfd_info_t *bfd_info = BFD_INFO(unit);;

    if (bcm_xgs5_bfd_info[unit] != NULL) {

        if (bfd_info->dma_buffer != NULL) {
            soc_cm_sfree(unit, bfd_info->dma_buffer);
        }

        if (bfd_info->auth_sp != NULL) {
            sal_free(bfd_info->auth_sp);
        }

        if (bfd_info->auth_sha1 != NULL) {
            sal_free(bfd_info->auth_sha1);
        }

        if (bfd_info->endpoints_in_use != NULL) {
            sal_free(bfd_info->endpoints_in_use);
        }

        if (bfd_info->endpoints != NULL) {
            sal_free(bfd_info->endpoints);
        }

        sal_free(bcm_xgs5_bfd_info[unit]);
        bcm_xgs5_bfd_info[unit] = NULL;
    }

    return;
}

STATIC int
bcmi_xgs5_bfd_alloc_resource(int unit, int num_sessions,
                             int num_auth_sha1_keys, int num_auth_sp_keys)
{
    bfd_info_t *bfd_info;

    if (bcm_xgs5_bfd_info[unit] != NULL) {
        bcmi_xgs5_bfd_free_resource(unit);
    }

    /* Allocate memory for BFD Information Data Structure */
    if ((bcm_xgs5_bfd_info[unit] =
         bcmi_xgs5_bfd_alloc_clear(sizeof(bfd_info_t),
                                   "BFD info")) == NULL) {
        return BCM_E_MEMORY;
    }

    bfd_info = BFD_INFO(unit);
    if ((bfd_info->endpoints =
         bcmi_xgs5_bfd_alloc_clear(num_sessions *
                                   sizeof(bfd_endpoint_config_t),
                                   "BFD endpoints")) == NULL) {
        return BCM_E_MEMORY;
    }

    if ((bfd_info->endpoints_in_use =
         bcmi_xgs5_bfd_alloc_clear(SHR_BITALLOCSIZE(num_sessions),
                                   "BFD endpoints in use")) == NULL) {
        return BCM_E_MEMORY;
    }

    /* Allocate memory for BFD Authentications */
    if (num_auth_sha1_keys > 0) {
        if ((bfd_info->auth_sha1 =
             bcmi_xgs5_bfd_alloc_clear(num_auth_sha1_keys *
                                       sizeof(bcm_bfd_auth_sha1_t),
                                       "BFD auth sha1")) == NULL) {
            return BCM_E_MEMORY;
        }
    }
    if (num_auth_sp_keys > 0) {
        if ((bfd_info->auth_sp =
             bcmi_xgs5_bfd_alloc_clear(num_auth_sp_keys *
                                       sizeof(bcm_bfd_auth_simple_password_t),
                                       "BFD auth simple password")) == NULL) {
            return BCM_E_MEMORY;
        }
    }

    /*
     * Allocate DMA buffer
     *
     * DMA buffer will be used to send and receive 'long' messages
     * between SDK Host and uController (BTE).
     */
    bfd_info->dma_buffer_len = sizeof(shr_bfd_msg_ctrl_t);
    bfd_info->dma_buffer = soc_cm_salloc(unit, bfd_info->dma_buffer_len,
                                         "BFD DMA buffer");
    if (!bfd_info->dma_buffer) {
        return BCM_E_MEMORY;
    }
    sal_memset(bfd_info->dma_buffer, 0, bfd_info->dma_buffer_len);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_event_unregister_all
 * Purpose:
 *      Free all event handlers.
 * Parameters:
 *      bfd_info - (IN/OUT) Pointer to BFD info structure.
 * Returns:
 *      None
 */
STATIC void
bcmi_xgs5_bfd_event_unregister_all(int unit)
{
    bfd_info_t *bfd_info = BFD_INFO(unit);
    bfd_event_handler_t *event_handler;
    bfd_event_handler_t *event_handler_to_delete;

    event_handler = bfd_info->event_handler_list;

    while (event_handler != NULL) {
        event_handler_to_delete = event_handler;
        event_handler = event_handler->next;

        sal_free(event_handler_to_delete);
    }

    bfd_info->event_handler_list = NULL;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_convert_uc_error
 * Purpose:
 *      Converts uController error code to corresponding BCM error code.
 * Parameters:
 *      uc_rv  - (IN) uController error code to convert.
 * Returns:
 *      BCM_E_XXX   - BCM error code.
 * Notes:
 */
STATIC int
bcmi_xgs5_bfd_convert_uc_error(uint32 uc_rv)
{
    int rv = BCM_E_NONE;

    switch(uc_rv) {
    case SHR_BFD_UC_E_NONE:
        rv = BCM_E_NONE;
        break;
    case SHR_BFD_UC_E_INTERNAL:
        rv = BCM_E_INTERNAL;
        break;
    case SHR_BFD_UC_E_MEMORY:
        rv = BCM_E_MEMORY;
        break;
    case SHR_BFD_UC_E_UNIT:
        rv = BCM_E_UNIT;
        break;
    case SHR_BFD_UC_E_PARAM:
        rv = BCM_E_PARAM;
        break;
    case SHR_BFD_UC_E_EMPTY:
        rv = BCM_E_EMPTY;
        break;
    case SHR_BFD_UC_E_FULL:
        rv = BCM_E_FULL;
        break;
    case SHR_BFD_UC_E_NOT_FOUND:
        rv = BCM_E_NOT_FOUND;
        break;
    case SHR_BFD_UC_E_EXISTS:
        rv = BCM_E_EXISTS;
        break;
    case SHR_BFD_UC_E_TIMEOUT:
        rv = BCM_E_TIMEOUT;
        break;
    case SHR_BFD_UC_E_BUSY:
        rv = BCM_E_BUSY;
        break;
    case SHR_BFD_UC_E_FAIL:
        rv = BCM_E_FAIL;
        break;
    case SHR_BFD_UC_E_DISABLED:
        rv = BCM_E_DISABLED;
        break;
    case SHR_BFD_UC_E_BADID:
        rv = BCM_E_BADID;
        break;
    case SHR_BFD_UC_E_RESOURCE:
        rv = BCM_E_RESOURCE;
        break;
    case SHR_BFD_UC_E_CONFIG:
        rv = BCM_E_CONFIG;
        break;
    case SHR_BFD_UC_E_UNAVAIL:
        rv = BCM_E_UNAVAIL;
        break;
    case SHR_BFD_UC_E_INIT:
        rv = BCM_E_INIT;
        break;
    case SHR_BFD_UC_E_PORT:
        rv = BCM_E_PORT;
        break;
    default:
        rv = BCM_E_INTERNAL;
        break;
    }

    return rv;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_msg_send_receive
 * Purpose:
 *      Sends given BFD control message to the uController.
 *      Receives and verifies expected reply.
 *      Performs DMA operation if required.
 * Parameters:
 *      unit        - (IN) Unit number.
 *      s_subclass  - (IN) BFD message subclass.
 *      s_len       - (IN) Value for 'len' field in message struct.
 *                         Length of buffer to flush if DMA send is required.
 *      s_data      - (IN) Value for 'data' field in message struct.
 *                         Ignored if message requires a DMA send/receive
 *                         operation.
 *      r_subclass  - (IN) Expected reply message subclass.
 *      r_len       - (OUT) Returns value in 'len' reply message field.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *     - The uc_msg 'len' and 'data' fields of mos_msg_data_t
 *       can take any arbitrary data.
 *
 *     BFD Long Control message:
 *     - BFD control messages that require send/receive of information
 *       that cannot fit in the uc_msg 'len' and 'data' fields need to
 *       use DMA operations to exchange information (long control message).
 *
 *     - BFD convention for long control messages for
 *        'mos_msg_data_t' fields:
 *          'len'    size of the DMA buffer to send to uController
 *          'data'   physical DMA memory address to send or receive
 *
 *      DMA Operations:
 *      - DMA read/write operation is performed when a long BFD control
 *        message is involved.
 *
 *      - Messages that require DMA operation (long control message)
 *        is indicated by MOS_MSG_DMA_MSG().
 *
 *      - Callers must 'pack' and 'unpack' corresponding information
 *        into/from DMA buffer indicated by BFD_INFO(unit)->dma_buffer.
 *
 */
STATIC int
bcmi_xgs5_bfd_msg_send_receive(int unit, uint8 s_subclass,
                               uint16 s_len, uint32 s_data,
                               uint8 r_subclass, uint16 *r_len)
{
    int rv;
    bfd_info_t *bfd_info = BFD_INFO(unit);
    mos_msg_data_t send, reply;
    uint8 *dma_buffer;
    int dma_buffer_len;
    uint32 uc_rv;

    sal_memset(&send, 0, sizeof(send));
    sal_memset(&reply, 0, sizeof(reply));
    send.s.mclass = MOS_MSG_CLASS_BFD;
    send.s.subclass = s_subclass;
    send.s.len = bcm_htons(s_len);

    /*
     * Set 'data' to DMA buffer address if a DMA operation is
     * required for send or receive.
     */
    dma_buffer = bfd_info->dma_buffer;
    dma_buffer_len = bfd_info->dma_buffer_len;
    if (MOS_MSG_DMA_MSG(s_subclass) ||
        MOS_MSG_DMA_MSG(r_subclass)) {
        send.s.data = bcm_htonl(soc_cm_l2p(unit, dma_buffer));
    } else {
        send.s.data = bcm_htonl(s_data);
    }

    /* Flush DMA memory */
    if (MOS_MSG_DMA_MSG(s_subclass)) {
        soc_cm_sflush(unit, dma_buffer, s_len);
    }

    /* Invalidate DMA memory to read */
    if (MOS_MSG_DMA_MSG(r_subclass)) {
        soc_cm_sinval(unit, dma_buffer, dma_buffer_len);
    }

    rv = soc_cmic_uc_msg_send_receive(unit, bfd_info->uc_num,
                                      &send, &reply,
                                      bfd_uc_msg_timeout_usecs);

    /* Check reply class, subclass */
    if ((rv != SOC_E_NONE) ||
        (reply.s.mclass != MOS_MSG_CLASS_BFD) ||
        (reply.s.subclass != r_subclass)) {
        return BCM_E_INTERNAL;
    }

    /* Convert BFD uController error code to BCM */
    uc_rv  = bcm_ntohl(reply.s.data);
    rv     = bcmi_xgs5_bfd_convert_uc_error(uc_rv);

    *r_len = bcm_ntohs(reply.s.len);

    return rv;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_event_mask_set
 * Purpose:
 *      Set the BFD Events mask.
 *      Events are set per BFD module.
 * Parameters:
 *      unit        - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_XXX  Operation failed
 * Notes:
 */
STATIC int
bcmi_xgs5_bfd_event_mask_set(int unit)
{
    bfd_info_t *bfd_info = BFD_INFO(unit);
    bfd_event_handler_t *event_handler;
    uint32 event_mask = 0;
    uint16 reply_len;

    /* Get event mask from all callbacks */
    for (event_handler = bfd_info->event_handler_list;
         event_handler != NULL;
         event_handler = event_handler->next) {

        if (SHR_BITGET(event_handler->event_types.w,
                       bcmBFDEventStateChange)) {
            event_mask |= BFD_BTE_EVENT_STATE;
        }
        if (SHR_BITGET(event_handler->event_types.w,
                       bcmBFDEventRemoteStateDiag)) {
            event_mask |= BFD_BTE_EVENT_REMOTE_STATE_DIAG;
        }
        if (SHR_BITGET(event_handler->event_types.w,
                       bcmBFDEventSessionDiscriminatorChange)) {
            event_mask |= BFD_BTE_EVENT_DISCRIMINATOR;
        }
        if (SHR_BITGET(event_handler->event_types.w,
                       bcmBFDEventAuthenticationChange)) {
            event_mask |= BFD_BTE_EVENT_AUTHENTICATION;
        }
        if (SHR_BITGET(event_handler->event_types.w,
                       bcmBFDEventParameterChange)) {
            event_mask |= BFD_BTE_EVENT_PARAMETER;
        }
        if (SHR_BITGET(event_handler->event_types.w,
                       bcmBFDEventSessionError)) {
            event_mask |= BFD_BTE_EVENT_ERROR;
        }
    }

    /* Update BFD event mask in uKernel */
    if (event_mask != bfd_info->event_mask) {
        /* Send BFD Event Mask message to uC */
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_msg_send_receive
             (unit,
              MOS_MSG_SUBCLASS_BFD_EVENT_MASK_SET,
              0, event_mask,
              MOS_MSG_SUBCLASS_BFD_EVENT_MASK_SET_REPLY,
              &reply_len));

        if (reply_len != 0) {
            return BCM_E_INTERNAL;
        }
    }

    bfd_info->event_mask = event_mask;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_callback_thread
 * Purpose:
 *      Thread to listen for event messages from uController.
 * Parameters:
 *      param - Pointer to BFD info structure.
 * Returns:
 *      None
 */
STATIC void
bcmi_xgs5_bfd_callback_thread(int unit)
{
    int rv;
    bfd_info_t *bfd_info = BFD_INFO(unit);
    bfd_event_handler_t *event_handler;
    bcm_bfd_event_types_t events, cb_events;
    bfd_msg_event_t event_msg;
    uint16 sess_id;
    uint32 event_mask;
    int invoke = 0;
    char thread_name[SAL_THREAD_NAME_MAX_LEN];
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "BFD callback thread starting\n")));
    thread_name[0] = 0x0;
    bfd_info->event_thread_id = sal_thread_self();
    bfd_info->event_thread_kill = 0;
    sal_thread_name(bfd_info->event_thread_id, thread_name, sizeof (thread_name));
    while (1) {
        /* Wait on notifications from uController */
        rv = soc_cmic_uc_msg_receive(bfd_info->unit, bfd_info->uc_num,
                                     MOS_MSG_CLASS_BFD_EVENT, &event_msg,
                                     sal_sem_FOREVER);

        if (BCM_FAILURE(rv) || (bfd_info->event_thread_kill)) {
            break;   /* Thread exit */
        }

        /* Get data from event message */
        sess_id    = bcm_ntohs(event_msg.s.len);
        event_mask = bcm_ntohl(event_msg.s.data);

        /* Set events */
        sal_memset(&events, 0, sizeof(events));
        if (event_mask & BFD_BTE_EVENT_STATE) {
            SHR_BITSET(events.w, bcmBFDEventStateChange);
        }
        if (event_mask & BFD_BTE_EVENT_REMOTE_STATE_DIAG) {
            SHR_BITSET(events.w, bcmBFDEventRemoteStateDiag);
        }
        if (event_mask & BFD_BTE_EVENT_DISCRIMINATOR) {
            SHR_BITSET(events.w, bcmBFDEventSessionDiscriminatorChange);
        }
        if (event_mask & BFD_BTE_EVENT_AUTHENTICATION) {
            SHR_BITSET(events.w, bcmBFDEventAuthenticationChange);
        }
        if (event_mask & BFD_BTE_EVENT_PARAMETER) {
            SHR_BITSET(events.w, bcmBFDEventParameterChange);
        }
        if (event_mask & BFD_BTE_EVENT_ERROR) {
            SHR_BITSET(events.w, bcmBFDEventSessionError);
        }


        /* Loop over registered callbacks,
         * If any match the events field, then invoke
         */
        sal_memset(&cb_events, 0, sizeof(cb_events));
        for (event_handler = bfd_info->event_handler_list;
             event_handler != NULL;
             event_handler = event_handler->next) {
            SHR_BITAND_RANGE(event_handler->event_types.w, events.w,
                             0, bcmBFDEventCount, cb_events.w);
            SHR_BITTEST_RANGE(cb_events.w, 0, bcmBFDEventCount, invoke);

            if (invoke) {
                
                event_handler->cb(bfd_info->unit, 0,
                                  cb_events, sess_id,
                                  event_handler->user_data);
            }
        }
    }
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_BFD,
                  (BSL_META_U(unit,
                              "AbnormalThreadExit:%s\n"), thread_name)); 
    }
    bfd_info->event_thread_kill = 0;
    bfd_info->event_thread_id = NULL;
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "BFD callback thread stopped\n")));
    sal_thread_exit(0);
}

/*
 * Function:
 *      bcmi_xgs5_bfd_find_free_endpoint
 * Purpose:
 *      Find an available endpoint index (endpoint id).
 * Parameters:
 *      endpoints_in_use - Current endpoint ids usage bitmap.
 *      endpoint_count   - Max number of endpoints.
 * Returns:
 *      Available endpoint id.
 *      (-1) Indicates no more available endpoint ids.
 * Notes:
 */
STATIC int
bcmi_xgs5_bfd_find_free_endpoint(SHR_BITDCL *endpoints_in_use,
                                 int endpoint_count)
{
    int endpoint_index;
    
    for (endpoint_index = 0; endpoint_index < endpoint_count;
         endpoint_index += 1) {
        if (!SHR_BITGET(endpoints_in_use, endpoint_index)) {
            break;
        }
    }

    if (endpoint_index >= endpoint_count) {
        endpoint_index = -1;
    }

    return endpoint_index;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_find_ipv4_entry
 */
STATIC int
bcmi_xgs5_bfd_find_ipv4_entry(int unit, bcm_vrf_t vrf,
                              bcm_ip_t ip_addr, int *index)
{
    max_mem_entry_t  l3_key;
    max_mem_entry_t  l3_entry;

    sal_memset(&l3_key, 0, sizeof(l3_key));

    soc_mem_field32_set(unit, BFD_HW_L3_IPV4_MEM(unit), &l3_key,
                        BFD_HW_L3_IPV4(unit)->vrf_id, vrf);

    soc_mem_field32_set(unit, BFD_HW_L3_IPV4_MEM(unit), &l3_key,
                        BFD_HW_L3_IPV4(unit)->ip_addr, ip_addr);

    soc_mem_field32_set(unit, BFD_HW_L3_IPV4_MEM(unit), &l3_key,
                        BFD_HW_L3_IPV4(unit)->key_type,
                        TR_L3_HASH_KEY_TYPE_V4UC);

    return soc_mem_search(unit, BFD_HW_L3_IPV4_MEM(unit), MEM_BLOCK_ANY,
                          index, &l3_key, &l3_entry, 0);
}

/*
 * Function:
 *      bcmi_xgs5_bfd_find_ipv6_entry
 */
STATIC int
bcmi_xgs5_bfd_find_ipv6_entry(int unit, bcm_vrf_t vrf,
                              bcm_ip6_t ip6_addr,int *index)
{
    max_mem_entry_t  l3_key;
    max_mem_entry_t  l3_entry;

    sal_memset(&l3_key, 0, sizeof(l3_key));

    soc_mem_ip6_addr_set(unit, BFD_HW_L3_IPV6_MEM(unit), &l3_key,
                         BFD_HW_L3_IPV6(unit)->ip_addr_lwr_64,
                         ip6_addr, SOC_MEM_IP6_LOWER_ONLY);

    soc_mem_ip6_addr_set(unit, BFD_HW_L3_IPV6_MEM(unit), &l3_key,
                         BFD_HW_L3_IPV6(unit)->ip_addr_upr_64,
                         ip6_addr, SOC_MEM_IP6_UPPER_ONLY);

    soc_mem_field32_set(unit, BFD_HW_L3_IPV6_MEM(unit), &l3_key,
                        BFD_HW_L3_IPV6(unit)->key_type_0,
                        TR_L3_HASH_KEY_TYPE_V6UC);

    soc_mem_field32_set(unit, BFD_HW_L3_IPV6_MEM(unit), &l3_key,
                        BFD_HW_L3_IPV6(unit)->key_type_1,
                        TR_L3_HASH_KEY_TYPE_V6UC);

    soc_mem_field32_set(unit, BFD_HW_L3_IPV6_MEM(unit), &l3_key,
                        BFD_HW_L3_IPV6(unit)->vrf_id, vrf);

    return soc_mem_search(unit, BFD_HW_L3_IPV6_MEM(unit), MEM_BLOCK_ANY,
                          index, &l3_key, &l3_entry, 0);

}


STATIC int
bcmi_xgs5_bfd_l2_entry_set(int unit,
                           int endpoint_index,
                           int mpls, int mpls_label, int your_discr,
                           bcm_module_t module_id, bcm_port_t port_id,
                           int int_pri, int cpu_qid)
{
    int local, rv;
    max_mem_entry_t  l2_entry;

    if (BFD_HW_L2_MEM(unit) == INVALIDm) {
        return BCM_E_UNAVAIL;
    }

    if (module_id == BCM_MODID_INVALID) {
        local = 1;
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_modid_is_local(unit, module_id, &local));
    }

    /* Insert your discriminator field to L2_ENTRY table. */
    sal_memset(&l2_entry, 0, sizeof(l2_entry));

    soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                        BFD_HW_L2(unit)->key_type,
                        BFD_HW_L2(unit)->bfd_key_type);
    soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                        BFD_HW_L2(unit)->valid, 1);
    soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                        BFD_HW_L2(unit)->static_bit, 1);

    if (mpls) {
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                            BFD_HW_L2(unit)->session_id_type, 1);
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                            BFD_HW_L2(unit)->label, mpls_label);
    } else {
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                            BFD_HW_L2(unit)->session_id_type, 0);
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                            BFD_HW_L2(unit)->your_discr, your_discr);
    }

    if (local) {
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                            BFD_HW_L2(unit)->session_index,
                            endpoint_index);
        if (BFD_HW_L2(unit)->cpu_queue_class != INVALIDf) {
            soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                                BFD_HW_L2(unit)->cpu_queue_class,
                                cpu_qid);
        }
    } else {
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                            BFD_HW_L2(unit)->remote, 1);
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                            BFD_HW_L2(unit)->dst_module, module_id);
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                            BFD_HW_L2(unit)->dst_port, port_id);
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_entry,
                            BFD_HW_L2(unit)->int_pri, int_pri);
    }


    rv = soc_mem_insert(unit, BFD_HW_L2_MEM(unit),MEM_BLOCK_ANY, &l2_entry);
    if (rv == BCM_E_FULL) {
        rv = bcm_bfd_l2_hash_dynamic_replace(unit, (void *)&l2_entry);
    }
   
    return rv;  
}

/*
 * Function:
 *      bcmi_xgs5_bfd_l2_entry_clear
 */
STATIC int
bcmi_xgs5_bfd_l2_entry_clear(int unit,
                             int mpls, int mpls_label, int your_discr)
{
    max_mem_entry_t  l2_key;
    max_mem_entry_t  l2_entry;
    int l2_index;

    sal_memset(&l2_key, 0, sizeof(l2_key));

    soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_key,
                        BFD_HW_L2(unit)->key_type,
                        BFD_HW_L2(unit)->bfd_key_type);

    if (mpls) {
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_key,
                            BFD_HW_L2(unit)->session_id_type, 1);
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_key,
                            BFD_HW_L2(unit)->label, mpls_label);
    } else {
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_key,
                            BFD_HW_L2(unit)->session_id_type, 0);
        soc_mem_field32_set(unit, BFD_HW_L2_MEM(unit), &l2_key,
                            BFD_HW_L2(unit)->your_discr, your_discr);
    }
    
    SOC_IF_ERROR_RETURN
        (soc_mem_search(unit, BFD_HW_L2_MEM(unit), MEM_BLOCK_ANY,
                        &l2_index, &l2_key, &l2_entry, 0));

    SOC_IF_ERROR_RETURN
        (soc_mem_delete_index(unit, BFD_HW_L2_MEM(unit), MEM_BLOCK_ANY,
                              l2_index));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_l2x_entry_set
 */
STATIC int
bcmi_xgs5_bfd_l2x_entry_set(int unit,
                            bfd_endpoint_config_t *endpoint_config,
                            int mpls)
{
    bcm_bfd_endpoint_info_t *endpoint_info;

    endpoint_info  = &endpoint_config->info;

    return(bcmi_xgs5_bfd_l2_entry_set(unit,
                                      endpoint_config->endpoint_index,
                                      mpls,
                                      endpoint_info->label,
                                      endpoint_info->local_discr,
                                      endpoint_config->modid,
                                      endpoint_config->port,
                                      endpoint_info->int_pri, 
                                      endpoint_info->cpu_qid));
}

/*
 * Function:
 *      bcmi_xgs5_bfd_l3_entry_set
 */
STATIC int
bcmi_xgs5_bfd_l3_entry_set(int unit,
                           bcm_bfd_endpoint_info_t *endpoint_info)
{
    int rv = BCM_E_NONE;
    int l3_index = 0;

    /* Check whether ipv4 or ipv6 */
    if (endpoint_info->flags & BCM_BFD_ENDPOINT_IPV6) {
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_find_ipv6_entry(unit,
                                           endpoint_info->vrf_id,
                                           endpoint_info->src_ip6_addr,
                                           &l3_index));

        /* Set BFD_ENABLE and LOCAL_ADDRESS */
        rv = soc_mem_field32_modify(unit, BFD_HW_L3_IPV6_MEM(unit), l3_index,
                                    BFD_HW_L3_IPV6(unit)->local_address, 1);
        rv |= soc_mem_field32_modify(unit, BFD_HW_L3_IPV6_MEM(unit), l3_index,
                                     BFD_HW_L3_IPV6(unit)->bfd_enable, 1);
    } else {
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_find_ipv4_entry(unit,
                                           endpoint_info->vrf_id,
                                           endpoint_info->src_ip_addr,
                                           &l3_index));
        /* Set BFD_ENABLE and LOCAL_ADDRESS */
        rv = soc_mem_field32_modify(unit, BFD_HW_L3_IPV4_MEM(unit), l3_index,
                                    BFD_HW_L3_IPV4(unit)->local_address, 1);
        rv |= soc_mem_field32_modify(unit, BFD_HW_L3_IPV4_MEM(unit), l3_index,
                                     BFD_HW_L3_IPV4(unit)->bfd_enable, 1);
    }

    return rv;
}
                         
/*
 * Function:
 *      bcmi_xgs5_bfd_l3_entry_clear
 */
STATIC int
bcmi_xgs5_bfd_l3_entry_clear(int unit,
                             bcm_bfd_endpoint_info_t *endpoint_info)
{
    int rv = BCM_E_NONE;
    int l3_index = 0;

    /* Check whether ipv4 or ipv6 */
    if (endpoint_info->flags & BCM_BFD_ENDPOINT_IPV6) {
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_find_ipv6_entry(unit,
                                           endpoint_info->vrf_id,
                                           endpoint_info->src_ip6_addr,
                                           &l3_index));
        /* Reset BFD_ENABLE and LOCAL_ADDRESS */
        rv = soc_mem_field32_modify(unit, BFD_HW_L3_IPV6_MEM(unit), l3_index,
                                    BFD_HW_L3_IPV6(unit)->local_address, 0);
        rv |= soc_mem_field32_modify(unit, BFD_HW_L3_IPV6_MEM(unit), l3_index,
                                     BFD_HW_L3_IPV6(unit)->bfd_enable, 0);
    } else {
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_find_ipv4_entry(unit,
                                           endpoint_info->vrf_id,
                                           endpoint_info->src_ip_addr,
                                           &l3_index));
        /* Reset BFD_ENABLE and LOCAL_ADDRESS */
        rv = soc_mem_field32_modify(unit, BFD_HW_L3_IPV4_MEM(unit), l3_index,
                                    BFD_HW_L3_IPV4(unit)->local_address, 0);
        rv |= soc_mem_field32_modify(unit, BFD_HW_L3_IPV4_MEM(unit), l3_index,
                                     BFD_HW_L3_IPV4(unit)->bfd_enable, 0);
    }

    return rv;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_find_l3_tunnel_entry
 */
STATIC int
bcmi_xgs5_bfd_find_l3_tunnel_entry(int unit,
                                   bcm_bfd_endpoint_info_t *endpoint_info,
                                   soc_tunnel_term_t *tunnel_entry)
{
    bcm_tunnel_type_t tunnel_type;
    bcm_tunnel_terminator_t tunnel;
    soc_tunnel_term_t key;

    /* Tunnel type */
    if (endpoint_info->type == bcmBFDTunnelTypeIp4in4) {
        tunnel_type = bcmTunnelTypeIp4In4;
    } else if (endpoint_info->type == bcmBFDTunnelTypeIp6in4) {
        tunnel_type = bcmTunnelTypeIp6In4;
    } else if (endpoint_info->type == bcmBFDTunnelTypeIp4in6) {
        tunnel_type = bcmTunnelTypeIp4In6;
    } else if (endpoint_info->type == bcmBFDTunnelTypeIp6in6) {
        tunnel_type = bcmTunnelTypeIp6In6;
    } else if (endpoint_info->type == bcmBFDTunnelTypeGre4In4) {
        tunnel_type = bcmTunnelTypeGre4In4;
    } else if (endpoint_info->type == bcmBFDTunnelTypeGre6In4) {
        tunnel_type = bcmTunnelTypeGre6In4;
    } else if (endpoint_info->type == bcmBFDTunnelTypeGre4In6) {
        tunnel_type = bcmTunnelTypeGre4In6;
    } else if (endpoint_info->type == bcmBFDTunnelTypeGre6In6) {
        tunnel_type = bcmTunnelTypeGre6In6;
    } else {
        return BCM_E_PARAM;
    }

    /* Initialize lookup key */
    bcm_tunnel_terminator_t_init(&tunnel);
    tunnel.type = tunnel_type;
    if ((tunnel_type == bcmTunnelTypeIp4In4) ||
        (tunnel_type == bcmTunnelTypeIp6In4) ||
        (tunnel_type == bcmTunnelTypeGre4In4) ||
        (tunnel_type == bcmTunnelTypeGre6In4)) {
        tunnel.sip  = endpoint_info->dst_ip_addr;
        tunnel.dip  = endpoint_info->src_ip_addr;
        tunnel.sip_mask = 0xffffffff;
        tunnel.dip_mask = 0xffffffff;
    } else {
        sal_memcpy(tunnel.sip6, endpoint_info->dst_ip6_addr, BCM_IP6_ADDRLEN);
        sal_memcpy(tunnel.dip6, endpoint_info->src_ip6_addr, BCM_IP6_ADDRLEN);
        sal_memset(tunnel.sip6_mask, 0xff, BCM_IP6_ADDRLEN);
        sal_memset(tunnel.dip6_mask, 0xff, BCM_IP6_ADDRLEN);
    }

    if ((BCM_ESW_BFD_DRV(unit) == NULL) ||
        (BFD_HW_CALL(unit)->l3_tnl_term_entry_init == NULL)) {
        return BCM_E_UNAVAIL;
    }
    BCM_IF_ERROR_RETURN
        (BFD_HW_CALL(unit)->l3_tnl_term_entry_init(unit, &tunnel, &key));

    /* Find tunnel terminator in TCAM */
    BCM_IF_ERROR_RETURN(soc_tunnel_term_match(unit, &key, tunnel_entry));

    return BCM_E_NONE;
}

STATIC int
bcmi_xgs5_bfd_l3_tunnel_entry_set(int unit,
                                  bcm_bfd_endpoint_info_t *endpoint_info)
{
    soc_tunnel_term_t tunnel_entry;
    uint32 *tunnel_entry_ptr;
    uint32 tunnel_index;

    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_find_l3_tunnel_entry(unit, endpoint_info,
                                            &tunnel_entry));

    tunnel_entry_ptr = (uint32 *)&tunnel_entry.entry_arr[0];
    soc_mem_field32_set(unit, BFD_HW_L3_TUNNEL_MEM(unit), tunnel_entry_ptr,
                        BFD_HW_L3_TUNNEL(unit)->bfd_enable, 1);

    BCM_IF_ERROR_RETURN
        (soc_tunnel_term_insert(unit, &tunnel_entry, &tunnel_index));

    return BCM_E_NONE;
}

STATIC int
bcmi_xgs5_bfd_l3_tunnel_entry_clear(int unit,
                                    bcm_bfd_endpoint_info_t *endpoint_info)
{
    soc_tunnel_term_t tunnel_entry;
    uint32 *tunnel_entry_ptr;
    uint32 tunnel_index;

    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_find_l3_tunnel_entry(unit, endpoint_info,
                                            &tunnel_entry));

    tunnel_entry_ptr = (uint32 *)&tunnel_entry.entry_arr[0];
    soc_mem_field32_set(unit, BFD_HW_L3_TUNNEL_MEM(unit), tunnel_entry_ptr,
                        BFD_HW_L3_TUNNEL(unit)->bfd_enable, 0);

    BCM_IF_ERROR_RETURN
        (soc_tunnel_term_insert(unit, &tunnel_entry, &tunnel_index));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_mpls_entry_set
 */
STATIC int
bcmi_xgs5_bfd_mpls_entry_set(int unit,
                             bfd_endpoint_config_t *endpoint_config)
{
#if defined(BCM_MPLS_SUPPORT)
    bcm_bfd_endpoint_info_t *endpoint_info;
    mpls_entry_entry_t mpls_entry;
    mpls_entry_entry_t mpls_key;
    int mpls_index = 0;
    int i, num_entries;
    bcm_l3_egress_t l3_egress;
    int ingress_if;
    int cc_type = 0;
    int cw_check = 0;

    endpoint_info  = &endpoint_config->info;
    
    sal_memset(&mpls_key, 0, sizeof(mpls_key));
    soc_mem_field32_set(unit, BFD_HW_MPLS_MEM(unit), &mpls_key,
                        BFD_HW_MPLS(unit)->key_type,
                        BFD_HW_MPLS(unit)->key_type_value);

    soc_mem_field32_set(unit, BFD_HW_MPLS_MEM(unit), &mpls_key,
                        BFD_HW_MPLS(unit)->mpls_label, endpoint_info->label);

    /* coverity[overrun-buffer-val : FALSE]    */
    SOC_IF_ERROR_RETURN(soc_mem_search(unit, BFD_HW_MPLS_MEM(unit),
                                       MEM_BLOCK_ANY, &mpls_index,
                                       &mpls_key, &mpls_entry, 0));

    if ((soc_mem_field32_get(unit, BFD_HW_MPLS_MEM(unit), &mpls_entry,
                             BFD_HW_MPLS(unit)->valid) != 0x1)) {
        return BCM_E_PARAM;
    }

    /* Set CW Check if Control Word is present */
    if ((endpoint_info->type == bcmBFDTunnelTypeMplsTpCc) ||
        (endpoint_info->type == bcmBFDTunnelTypeMplsTpCcCv) ||
        (endpoint_info->type == bcmBFDTunnelTypePweControlWord) ||
        ((endpoint_info->type == bcmBFDTunnelTypePweRouterAlert) &&
         (endpoint_info->flags & BCM_BFD_ENDPOINT_PWE_ACH)) ||
        ((endpoint_info->type == bcmBFDTunnelTypePweTtl) &&
         (endpoint_info->flags & BCM_BFD_ENDPOINT_PWE_ACH))) {
        cw_check = 1;
    }

    /* Set PW CC Type */
    if (endpoint_info->type == bcmBFDTunnelTypePweControlWord) {
        cc_type = 1;
    } else if (endpoint_info->type == bcmBFDTunnelTypePweRouterAlert) {
        cc_type = 2;
    } else if (endpoint_info->type == bcmBFDTunnelTypePweTtl) {
        cc_type = 3;
    }

    if ((endpoint_info->type == bcmBFDTunnelTypeMplsTpCc) ||
        (endpoint_info->type == bcmBFDTunnelTypeMplsTpCcCv)) {
        if (soc_mem_field32_get(unit, BFD_HW_MPLS_MEM(unit), &mpls_entry,
             BFD_HW_MPLS(unit)->mpls_action_if_bos) == MPLS_ACTION_POP_L3_IIF) {
            endpoint_config->is_l3_vpn = 1;    /* L3 VPN */
        } else {
            endpoint_config->is_l3_vpn = 0;    
            cc_type = 1;  /* PW */
        }
    }

    soc_mem_field32_set(unit, BFD_HW_MPLS_MEM(unit), &mpls_entry,
                        BFD_HW_MPLS(unit)->session_id_type, 1);
    soc_mem_field32_set(unit, BFD_HW_MPLS_MEM(unit), &mpls_entry,
                        BFD_HW_MPLS(unit)->bfd_enable, 1);
    soc_mem_field32_set(unit, BFD_HW_MPLS_MEM(unit), &mpls_entry,
                        BFD_HW_MPLS(unit)->cw_check_ctrl, cw_check);
    soc_mem_field32_set(unit, BFD_HW_MPLS_MEM(unit), &mpls_entry,
                        BFD_HW_MPLS(unit)->pw_cc_type, cc_type);

    SOC_IF_ERROR_RETURN(soc_mem_write(unit, BFD_HW_MPLS_MEM(unit),
                                      MEM_BLOCK_ANY, mpls_index, &mpls_entry));

    /*
     * PW CC-3
     *
     * Set MPLS entry DECAP_USE_TTL=0 for corresponding
     * Tunnel Terminator label.
     */
    if (endpoint_info->type == bcmBFDTunnelTypePweTtl) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_l3_egress_get(unit, endpoint_info->egress_if,
                                   &l3_egress));

        /* Look for Tunnel Terminator label */
        num_entries = soc_mem_index_count(unit, BFD_HW_MPLS_MEM(unit));
        for (i = 0; i < num_entries; i++) {
            BCM_IF_ERROR_RETURN
                (soc_mem_read(unit, BFD_HW_MPLS_MEM(unit),
                              MEM_BLOCK_ANY, i, &mpls_entry));
            if (!soc_mem_field32_get(unit, BFD_HW_MPLS_MEM(unit),
                                     &mpls_entry, BFD_HW_MPLS(unit)->valid)) {
                continue;
            }
            if (soc_mem_field32_get(unit, BFD_HW_MPLS_MEM(unit), &mpls_entry,
                                    BFD_HW_MPLS(unit)->mpls_action_if_bos)
                == 0x1) {
                continue;    /* L2_SVP */
            }

            ingress_if = soc_mem_field32_get(unit, BFD_HW_MPLS_MEM(unit),
                                             &mpls_entry,
                                             BFD_HW_MPLS(unit)->l3_iif);
            if (ingress_if == l3_egress.intf) {
                /* Label found */
                soc_mem_field32_set(unit, BFD_HW_MPLS_MEM(unit),
                                    &mpls_entry,
                                    BFD_HW_MPLS(unit)->decap_use_ttl, 0);
                SOC_IF_ERROR_RETURN(soc_mem_write(unit, BFD_HW_MPLS_MEM(unit),
                                                  MEM_BLOCK_ALL, i,
                                                  &mpls_entry));
                break;
            }
        }
    }

    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_MPLS_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_mpls_entry_clear
 */
STATIC int
bcmi_xgs5_bfd_mpls_entry_clear(int unit,
                               bcm_bfd_endpoint_info_t *endpoint_info)
{
#if defined(BCM_MPLS_SUPPORT)
    mpls_entry_entry_t mpls_entry;
    mpls_entry_entry_t mpls_key;
    int mpls_index = 0;
    
    sal_memset(&mpls_key, 0, sizeof(mpls_key));
    soc_mem_field32_set(unit, BFD_HW_MPLS_MEM(unit), &mpls_key,
                        BFD_HW_MPLS(unit)->key_type, BFD_HW_MPLS(unit)->key_type_value);
    soc_mem_field32_set(unit, BFD_HW_MPLS_MEM(unit), &mpls_key,
                        BFD_HW_MPLS(unit)->mpls_label, endpoint_info->label);

    /* coverity[overrun-buffer-val : FALSE]    */
    SOC_IF_ERROR_RETURN(soc_mem_search(unit, BFD_HW_MPLS_MEM(unit),
                                       MEM_BLOCK_ANY, &mpls_index,
                                       &mpls_key, &mpls_entry, 0));

    soc_mem_field32_set(unit, BFD_HW_MPLS_MEM(unit), &mpls_entry,
                        BFD_HW_MPLS(unit)->bfd_enable, 0);
    soc_mem_field32_set(unit, BFD_HW_MPLS_MEM(unit), &mpls_entry,
                        BFD_HW_MPLS(unit)->pw_cc_type, 0);

    SOC_IF_ERROR_RETURN(soc_mem_write(unit, BFD_HW_MPLS_MEM(unit),
                                      MEM_BLOCK_ANY,
                                      mpls_index, &mpls_entry));
    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_MPLS_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_hw_init
 * Purpose:
 *      Initialize the HW for BFD packet processing.
 *      Configure:
 *      - Copy to CPU BFD error packets
 *      - Assign RX DMA channel to BFD CPU COS Queue
 *      - Map RX BFD packets to BFD CPU COS Queue
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_bfd_hw_init(int unit)
{
    int rv = BCM_E_NONE;
    bfd_info_t *bfd_info = BFD_INFO(unit);
    uint32 val;
    int index;
    int slowpath_index, ach_err_index;
    int cosq_map_size;
    bcm_rx_reasons_t reasons, reasons_mask;
    bcm_rx_reasons_t reasons_all;
    uint8 int_prio, int_prio_mask;
    uint32 packet_type, packet_type_mask;
    bcm_cos_queue_t cosq;
    bcm_rx_chan_t chan_id;
    int num_cosq = 0;
    int min_cosq, max_cosq;
    int i;

    /*
     * Send BFD error packet to CPU
     *
     * Configure CPU_CONTROL_0 register
     */
    if (SOC_REG_IS_VALID(unit, CPU_CONTROL_0r)) {
        BCM_IF_ERROR_RETURN(READ_CPU_CONTROL_0r(unit, &val));
        soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                          BFD_UNKNOWN_VERSION_TOCPUf, 1);
        soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                          BFD_YOUR_DISCRIMINATOR_NOT_FOUND_TOCPUf, 1);
        soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                          BFD_UNKNOWN_CONTROL_PACKET_TOCPUf, 1);
        if (SOC_REG_FIELD_VALID(unit, CPU_CONTROL_0r,
                                BFD_UNKNOWN_ACH_CHANNEL_TYPE_TOCPUf)) {
            soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                              BFD_UNKNOWN_ACH_CHANNEL_TYPE_TOCPUf, 1);
        }
        if (SOC_REG_FIELD_VALID(unit, CPU_CONTROL_0r,
                                BFD_UNKNOWN_ACH_VERSION_TOCPUf)) {
            soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                              BFD_UNKNOWN_ACH_VERSION_TOCPUf, 1);
        }

        BCM_IF_ERROR_RETURN(WRITE_CPU_CONTROL_0r(unit, val));

    } else {
        return BCM_E_UNAVAIL;
    }

    /*
     * Set BFD ACH Channel Types
     */
    BCM_IF_ERROR_RETURN(READ_BFD_RX_ACH_TYPE_CONTROL0r(unit, &val));
    soc_reg_field_set(unit, BFD_RX_ACH_TYPE_CONTROL0r, &val,
                      BFD_ACH_TYPE_IPV4f, SHR_BFD_ACH_CHANNEL_TYPE_IPV4);
    soc_reg_field_set(unit, BFD_RX_ACH_TYPE_CONTROL0r, &val,
                      BFD_ACH_TYPE_IPV6f, SHR_BFD_ACH_CHANNEL_TYPE_IPV6);
    BCM_IF_ERROR_RETURN(WRITE_BFD_RX_ACH_TYPE_CONTROL0r(unit, val));

    BCM_IF_ERROR_RETURN(READ_BFD_RX_ACH_TYPE_CONTROL1r(unit, &val));
    soc_reg_field_set(unit, BFD_RX_ACH_TYPE_CONTROL1r, &val,
                      BFD_ACH_TYPE_RAWf, SHR_BFD_ACH_CHANNEL_TYPE_RAW);
    BCM_IF_ERROR_RETURN(WRITE_BFD_RX_ACH_TYPE_CONTROL1r(unit, val));

    BCM_IF_ERROR_RETURN(READ_BFD_RX_ACH_TYPE_MPLSTPr(unit, &val));
    soc_reg_field_set(unit, BFD_RX_ACH_TYPE_MPLSTPr, &val,
                      BFD_ACH_TYPE_MPLSTP_CCf,
                      SHR_BFD_ACH_CHANNEL_TYPE_MPLS_TP_CC);
    soc_reg_field_set(unit, BFD_RX_ACH_TYPE_MPLSTPr, &val,
                      BFD_ACH_TYPE_MPLSTP_CVf,
                      SHR_BFD_ACH_CHANNEL_TYPE_MPLS_TP_CV);
    BCM_IF_ERROR_RETURN(WRITE_BFD_RX_ACH_TYPE_MPLSTPr(unit, val));


    /*
     * Set special Session-ID for Unknown Remote Discriminator
     *
     * This allows initial BFD Control packets, where the remote
     * discriminator is unknown (your_discriminator=0),
     * to be received as 'good packets' and be distinct from
     * other errors (wrong BFD version) that may result
     * in "bcmRxReasonBfdLookupFailure"
     */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_l2_entry_set(unit,
                                    SHR_BFD_DISCR_UNKNOWN_SESSION_ID,
                                    0, 0, SHR_BFD_DISCR_UNKNOWN_YOUR_DISCR,
                                    BCM_MODID_INVALID, 0,
                                    0, 0));

    /*
     * BFD COS Queue
     *
     * BFD COSQ will be set to one of the following:
     * - User configured BFD COSq, if provided
     * - The default BFD COS queue.  This is set to the highest queue
     *   available within the queue range assigned to the uC where
     *   the BFD application is running.
     */

    /* Get valid range of COS queues for uC where BFD is loaded */
    min_cosq = 0;
    for (i = 0; i < SOC_CMCS_NUM(unit); i++) {
        num_cosq = NUM_CPU_ARM_COSQ(unit, i);
        if (i == bfd_info->uc_num + 1) {
            break;
        }
        min_cosq += num_cosq;
    }
    max_cosq = min_cosq + num_cosq - 1;

    /* Check that there is at least one COS queue assigned to the CMC */
    if (max_cosq < min_cosq) {
        LOG_ERROR(BSL_LS_BCM_COMMON,
                  (BSL_META_U(unit,
                              "ERROR: No BFD COS Queue available from uC%d\n"),
                   bfd_info->uc_num));
        return BCM_E_CONFIG;
    }

    /* Check user provide COSQ */
    if (bfd_info->cpu_cosq != BFD_COSQ_INVALID) {
        if ((bfd_info->cpu_cosq < min_cosq) ||
            (bfd_info->cpu_cosq > max_cosq)) {
            return BCM_E_CONFIG;
        }
        min_cosq = max_cosq = bfd_info->cpu_cosq;
    }


    /*
     * Assign RX DMA channel to BFD CPU COS Queue
     * (This is the RX channel to listen on for BFD packets).
     *
     * DMA channels (12) are assigned 4 per processor:
     * (see /src/bcm/common/rx.c)
     *   channels 0..3  --> PCI host
     *   channels 4..7  --> uController 0
     *   chnanels 8..11 --> uController 1
     *
     * The uControllers designate the 4 local DMA channels as follows:
     *   local channel  0     --> TX
     *   local channel  1..3  --> RX
     *
     * Each uController application needs to use a different
     * RX DMA channel to listen on.
     */
    chan_id = (BCM_RX_CHANNELS * (SOC_ARM_CMC(unit, bfd_info->uc_num))) +
        bfd_info->rx_channel;
    for (i = max_cosq; i >= min_cosq; i--) {
        rv = _bcm_common_rx_queue_channel_set(unit, i, chan_id);
        if (BCM_SUCCESS(rv)) {
            bfd_info->cpu_cosq = i;  /* COSQ found */
            break;
        }
    }
    if (i < min_cosq) {
        LOG_ERROR(BSL_LS_BCM_COMMON,
                  (BSL_META_U(unit,
                              "ERROR: No BFD COS Queue available from uC%d\n"),
                   bfd_info->uc_num));
        return BCM_E_RESOURCE;    /* Could not find an available COSQ */
    }


    /*
     * Direct BFD packets to designated RX CPU COS Queue
     *
     * Reasons:
     *   - bcmRxReasonBfdSlowpath
     *         BFD session lookup hit (good packet)
     *   - bcmRxReasonBfd / bcmRxReasonBfdUnknownVersion
     *         BFD error - Unknown ACH Version or Channel Type
     * NOTE:
     *     BFD RX reasons need to be mapped using different indexes
     *     due to HW restriction.
     *
     *     The user input 'cpu_qid' (bcm_bfd_endpoint_t) could be
     *     used to select different CPU COS queue. Currently,
     *     all priorities are mapped into the same CPU COS Queue.
     */

    /* Find available entries in CPU COS queue map table */
    slowpath_index   = -1;   /* COSQ map index for good packets */
    ach_err_index    = -1;   /* COSQ map index for unknown ACH error */
    BCM_IF_ERROR_RETURN
        (bcm_esw_rx_cosq_mapping_size_get(unit, &cosq_map_size));
    
    for (index = 0; index < cosq_map_size; index++) {
        rv = bcm_esw_rx_cosq_mapping_get(unit, index,
                                         &reasons, &reasons_mask,
                                         &int_prio, &int_prio_mask,
                                         &packet_type, &packet_type_mask,
                                         &cosq);
        if (rv == BCM_E_NOT_FOUND) {
            /* Find available indexes for BFD reason codes */
            rv = BCM_E_NONE;
            if (slowpath_index == -1) {
                slowpath_index = index;
            } else {
                ach_err_index = index;
                break;
            }
        }
        if (BCM_FAILURE(rv)) {
            return rv;
        }
    }
    if ((slowpath_index == -1) || (ach_err_index == -1)) {
        return BCM_E_RESOURCE;
    }

    /*
     * Map RX reason code to RX CPU COS Queue
     */
    BCM_IF_ERROR_RETURN
        (bcm_esw_rx_cosq_mapping_reasons_get(unit, &reasons_all));

    /* BFD Slowpath - Lookup hit */
    BCM_RX_REASON_CLEAR_ALL(reasons);
    BCM_RX_REASON_SET(reasons, bcmRxReasonBfdSlowpath);  /* BFD hit, good */
    BCM_IF_ERROR_RETURN
        (bcm_esw_rx_cosq_mapping_set(unit, slowpath_index,
                                     reasons, reasons,
                                     0, 0, /* Any priority */
                                     0, 0, /* Any packet type */
                                     bfd_info->cpu_cosq));
    bfd_info->cosq_spath_index = slowpath_index;

    /* BFD Error */
    BCM_RX_REASON_CLEAR_ALL(reasons);
    BCM_RX_REASON_SET(reasons, bcmRxReasonBfd);
    /*
     * ACH Unknown Version or Channel Type
     * Check if reason code is valid, if so, add to BFD error
     */
    if (BCM_RX_REASON_GET(reasons_all, bcmRxReasonBfdUnknownVersion)) {
        BCM_RX_REASON_SET(reasons, bcmRxReasonBfdUnknownVersion);
    }
    BCM_IF_ERROR_RETURN
        (bcm_esw_rx_cosq_mapping_set(unit, ach_err_index,
                                     reasons, reasons,
                                     0, 0, /* Any priority */
                                     0, 0, /* Any packet type */
                                     bfd_info->cpu_cosq));
    bfd_info->cosq_ach_err_index = ach_err_index;

    return rv;
}

/*
 * Functions:
 *      bcmi_xgs5_bfd_<header/label>_pack
 * Purpose:
 *      The following set of _pack() functions packs in
 *      network byte order a given header/label.
 * Parameters:
 *      buffer          - (OUT) Buffer where to pack header/label.
 *      <header/label>  - (IN) Header/label to pack.
 * Returns:
 *      Pointer to next position in buffer.
 * Notes:
 */
STATIC uint8 *
bcmi_xgs5_bfd_udp_header_pack(uint8 *buffer, bfd_udp_header_t *udp)
{
    BFD_ENCAP_PACK_U16(buffer, udp->src);
    BFD_ENCAP_PACK_U16(buffer, udp->dst);
    BFD_ENCAP_PACK_U16(buffer, udp->length);
    BFD_ENCAP_PACK_U16(buffer, udp->sum);

    return buffer;
}

STATIC uint8 *
bcmi_xgs5_bfd_ipv4_header_pack(uint8 *buffer, bfd_ipv4_header_t *ip)
{
    uint32  tmp;

    tmp = ((ip->version & 0xf) << 28) | ((ip->h_length & 0xf) << 24) |
        ((ip->dscp & 0x3f) << 18) | ((ip->ecn & 0x3)<< 16) | (ip->length);
    BFD_ENCAP_PACK_U32(buffer, tmp);

    tmp = (ip->id << 16) | ((ip->flags & 0x7) << 13) | (ip->f_offset & 0x1fff);
    BFD_ENCAP_PACK_U32(buffer, tmp);

    tmp = (ip->ttl << 24) | (ip->protocol << 16) | ip->sum;
    BFD_ENCAP_PACK_U32(buffer, tmp);

    BFD_ENCAP_PACK_U32(buffer, ip->src);
    BFD_ENCAP_PACK_U32(buffer, ip->dst);

    return buffer;
}

STATIC uint8 *
bcmi_xgs5_bfd_ipv6_header_pack(uint8 *buffer, bfd_ipv6_header_t *ip)
{
    uint32  tmp;
    int     i;

    tmp = ((ip->version & 0xf) << 28) | (ip->t_class << 20) |
        (ip->f_label & 0xfffff);
    BFD_ENCAP_PACK_U32(buffer, tmp);

    tmp = (ip->p_length << 16 ) | (ip->next_header << 8) | ip->hop_limit;
    BFD_ENCAP_PACK_U32(buffer, tmp);

    for (i = 0; i < BCM_IP6_ADDRLEN; i++) {
        BFD_ENCAP_PACK_U8(buffer, ip->src[i]);
    }

    for (i = 0; i < BCM_IP6_ADDRLEN; i++) {
        BFD_ENCAP_PACK_U8(buffer, ip->dst[i]);
    }

    return buffer;
}

STATIC uint8 *
bcmi_xgs5_bfd_gre_header_pack(uint8 *buffer, bfd_gre_header_t *gre)
{
    uint32  tmp;

    tmp = ((gre->checksum & 0x1) << 31) | ((gre->routing & 0x1) << 30) |
        ((gre->key & 0x1) << 29) | ((gre->sequence_num & 0x1) << 28) | 
        ((gre->strict_src_route & 0x1) << 27) |
        ((gre->recursion_control & 0x7) << 24) | ((gre->flags & 0x1f) << 19) |
        ((gre->version & 0x7) << 16) | (gre->protocol & 0xffff);

    BFD_ENCAP_PACK_U32(buffer, tmp);

    return buffer;
}

STATIC uint8 *
bcmi_xgs5_bfd_ach_header_pack(uint8 *buffer, bfd_ach_header_t *ach)
{
    uint32  tmp;

    tmp = ((ach->f_nibble & 0xf) << 28) | ((ach->version & 0xf) << 24) |
        (ach->reserved << 16) | ach->channel_type;

    BFD_ENCAP_PACK_U32(buffer, tmp);

    return buffer;
}

STATIC uint8 *
bcmi_xgs5_bfd_mpls_label_pack(uint8 *buffer, bfd_mpls_label_t *mpls)
{
    uint32  tmp;

    tmp = ((mpls->label & 0xfffff) << 12) | ((mpls->exp & 0x7) << 9) |
        ((mpls->s & 0x1) << 8) | mpls->ttl;
    BFD_ENCAP_PACK_U32(buffer, tmp);

    return buffer;
}

STATIC uint8 *
bcmi_xgs5_bfd_l2_header_pack(uint8 *buffer, bfd_l2_header_t *l2)
{
    uint32  tmp;
    int     i;

    for (i = 0; i < BFD_MAC_ADDR_LENGTH; i++) {
        BFD_ENCAP_PACK_U8(buffer, l2->dst_mac[i]);
    }

    for (i = 0; i < BFD_MAC_ADDR_LENGTH; i++) {
        BFD_ENCAP_PACK_U8(buffer, l2->src_mac[i]);
    }

    /*
     * VLAN Tag
     *
     * A TPID value of 0 indicates this is an untagged frame.
     */
    if (l2->outer_vlan_tag.tpid != 0) {
        /* Add VLAN tag */
        tmp = (l2->outer_vlan_tag.tpid << 16) |
            ((l2->outer_vlan_tag.tci.prio & 0x7) << 13) |
            ((l2->outer_vlan_tag.tci.cfi & 0x1) << 12) |
            (l2->outer_vlan_tag.tci.vid & 0xfff);
        BFD_ENCAP_PACK_U32(buffer, tmp);
    }

    /* Packet inner vlan tag */

    if (l2->inner_vlan_tag.tpid != 0) {
        /* Add VLAN tag */
        tmp = (l2->inner_vlan_tag.tpid << 16) |
            ((l2->inner_vlan_tag.tci.prio & 0x7) << 13) |
            ((l2->inner_vlan_tag.tci.cfi & 0x1) << 12) |
            (l2->inner_vlan_tag.tci.vid & 0xfff);
        BFD_ENCAP_PACK_U32(buffer, tmp);
    }

    BFD_ENCAP_PACK_U16(buffer, l2->etype);

    return buffer;
}

STATIC uint8 *
bcmi_xgs5_bfd_hg_header_pack(uint8 *buffer, int unit,
                             bfd_endpoint_config_t *endpoint_config,
                             bfd_l2_header_t *l2)
{
    soc_higig_hdr_t *hg = (soc_higig_hdr_t *)buffer;
    int hg_size;
    int src_mod;
    int src_port;
    int dst_mod;
    int dst_port;
    uint32 vlan_val;
    uint32 prio_val;
    uint32 pfm;

    hg_size = sizeof(*hg);

    if (BCM_FAILURE(bcm_esw_stk_my_modid_get(unit, &src_mod))) {
        return NULL;
    }
    src_port = endpoint_config->tx_port;
    dst_mod  = endpoint_config->modid;
    dst_port = endpoint_config->port;
    vlan_val = l2->outer_vlan_tag.tci.vid;
    pfm      = SOC_DEFAULT_DMA_PFM_GET(unit);
    prio_val = endpoint_config->info.int_pri;

    sal_memset(hg, 0, hg_size);
    soc_higig_field_set(unit, hg, HG_start, SOC_HIGIG_START);
    soc_higig_field_set(unit, hg, HG_hgi, SOC_HIGIG_HGI);
    soc_higig_field_set(unit, hg, HG_opcode, SOC_HIGIG_OP_UC);
    soc_higig_field_set(unit, hg, HG_hdr_format, SOC_HIGIG_HDR_DEFAULT);
    soc_higig_field_set(unit, hg, HG_vlan_tag, vlan_val);
    soc_higig_field_set(unit, hg, HG_dst_mod, dst_mod);
    soc_higig_field_set(unit, hg, HG_dst_port, dst_port);
    soc_higig_field_set(unit, hg, HG_src_mod, src_mod);
    soc_higig_field_set(unit, hg, HG_src_port, src_port);
    soc_higig_field_set(unit, hg, HG_pfm, pfm);
    soc_higig_field_set(unit, hg, HG_cos, prio_val);

    if (l2->inner_vlan_tag.tpid != 0) {
        soc_higig_field_set(unit, hg, HG_ingress_tagged, 1);
    }   

    buffer += hg_size;

    return buffer;
}

STATIC uint8 *
bcmi_xgs5_bfd_hg2_header_pack(uint8 *buffer, int unit,
                              bfd_endpoint_config_t *endpoint_config,
                              bfd_l2_header_t *l2)
{
    soc_higig2_hdr_t *hg = (soc_higig2_hdr_t *)buffer;
    int hg_size;
    int src_mod;
    int src_port;
    int dst_mod;
    int dst_port;
    uint32 vlan_val;
    uint32 prio_val;
    uint32 pfm;
    uint32 ppd;

    hg_size = sizeof(*hg);

    if (BCM_FAILURE(bcm_esw_stk_my_modid_get(unit, &src_mod))) {
        return NULL;
    }
    src_port = endpoint_config->tx_port;
    dst_mod  = endpoint_config->modid;
    dst_port = endpoint_config->port;
    vlan_val = l2->outer_vlan_tag.tci.vid;
    pfm      = SOC_DEFAULT_DMA_PFM_GET(unit);
    prio_val = endpoint_config->info.int_pri;
    ppd      = 0;

    sal_memset(hg, 0, hg_size);
    soc_higig2_field_set(unit, hg, HG_start, SOC_HIGIG2_START);
    soc_higig2_field_set(unit, hg, HG_ppd_type, ppd);
    soc_higig2_field_set(unit, hg, HG_opcode, SOC_HIGIG_OP_UC);
    soc_higig2_field_set(unit, hg, HG_vlan_tag, vlan_val);
    soc_higig2_field_set(unit, hg, HG_dst_mod, dst_mod);
    soc_higig2_field_set(unit, hg, HG_dst_port, dst_port);
    soc_higig2_field_set(unit, hg, HG_src_mod, src_mod);
    soc_higig2_field_set(unit, hg, HG_src_port, src_port);
    soc_higig2_field_set(unit, hg, HG_pfm, pfm);
    soc_higig2_field_set(unit, hg, HG_tc, prio_val);

    if (l2->inner_vlan_tag.tpid != 0) {
        soc_higig2_field_set(unit, hg, HG_ingress_tagged, 1);
    }   

    buffer += hg_size;

    return buffer;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_udp_checksum_set
 * Purpose:
 *      Set the UDP initial checksum (excludes checksum for the data).
 *
 *      The checksum includes the IP pseudo-header and UDP header.
 *      It does not include the checksum for the data (BFD packet).
 *      The data checksum will be added to UDP initial checksum in
 *      the uC side each time a packet is sent, since data payload may vary.
 * Parameters:
 *      v6              - (IN) Set if IP is IPv6.
 *      ipv4            - (IN) IPv4 header.
 *      ipv6            - (IN) IPv6 header.
 *      udp             - (IN/OUT) UDP header checksum to update.
 * Returns:
 *      None
 */
STATIC void
bcmi_xgs5_bfd_udp_checksum_set(int v6,
                               bfd_ipv4_header_t *ipv4,
                               bfd_ipv6_header_t *ipv6,
                               bfd_udp_header_t *udp)
{
    uint8  buffer[SHR_BFD_UDP_HEADER_LENGTH + (BCM_IP6_ADDRLEN*2) + 8];
    uint8  *cur_ptr = buffer;
    int    i, length;

    /* Build IP Pseudo-Header */
    if (v6) {
        /* IPv6 Pseudo-Header
         *     Source address
         *     Destination address
         *     UDP length  (32-bit)
         *     Next Header (lower 8 bits of 32-bit)
         */
        for (i = 0; i < BCM_IP6_ADDRLEN; i++) {
            BFD_ENCAP_PACK_U8(cur_ptr, ipv6->src[i]);
        }
        for (i = 0; i < BCM_IP6_ADDRLEN; i++) {
            BFD_ENCAP_PACK_U8(cur_ptr, ipv6->dst[i]);
        }
        BFD_ENCAP_PACK_U16(cur_ptr, udp->length);
        BFD_ENCAP_PACK_U8(cur_ptr, ipv6->next_header);
    } else {
        /* IPv4 Pseudo-Header
         *     Source address
         *     Destination address
         *     Protocol    (8 bits)
         *     UDP length  (16-bit)
         */
        BFD_ENCAP_PACK_U32(cur_ptr, ipv4->src);
        BFD_ENCAP_PACK_U32(cur_ptr, ipv4->dst);
        BFD_ENCAP_PACK_U8(cur_ptr, ipv4->protocol);
        BFD_ENCAP_PACK_U16(cur_ptr, udp->length);
    }

    /* Add UDP header */
    cur_ptr = bcmi_xgs5_bfd_udp_header_pack(cur_ptr, udp);

    /* Calculate initial UDP checksum */
    length = cur_ptr - buffer;
    udp->sum = _shr_ip_chksum(length, buffer);
}

/*
 * Functions:
 *      bcmi_xgs5_bfd_<header/label>_get
 * Purpose:
 *      The following set of _get() functions builds a given header/label.
 * Parameters:
 *      unit            - (IN) Unit number.
 *      endpoint_config - (IN) Pointer to BFD endpoint structure.
 *      packet_flags    - (IN) Flags for building packet.
 *      <... other IN args ...>
 *      <header/label>  - (OUT) Returns header/label.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_bfd_udp_header_get(int unit, bcm_bfd_endpoint_info_t *endpoint_info,
                             uint32 packet_flags, bfd_udp_header_t *udp)
{
    bfd_info_t *bfd_info = BFD_INFO(unit);
    int auth_length;
    int bfd_length;

    /* Calculate BFD packet length */
    /* Authentication (optional) */
    switch (endpoint_info->auth) {
    case _SHR_BFD_AUTH_TYPE_SIMPLE_PASSWORD:
        auth_length = SHR_BFD_AUTH_SP_HEADER_START_LENGTH +
            bfd_info->auth_sp[endpoint_info->auth_index].length;
        break;
    case _SHR_BFD_AUTH_TYPE_KEYED_SHA1:
    case _SHR_BFD_AUTH_TYPE_METICULOUS_KEYED_SHA1:
        auth_length = SHR_BFD_AUTH_SHA1_LENGTH;
        break;
    default:
        auth_length = 0;
        break;
    }
    bfd_length = SHR_BFD_CONTROL_HEADER_LENGTH + auth_length;

    /* Set UDP */
    sal_memset(udp, 0, sizeof(*udp));
    udp->src = endpoint_info->udp_src_port;
    udp->dst = (packet_flags & BFD_ENCAP_PKT_UDP__MULTI_HOP) ?
        SHR_BFD_UDP_MULTI_HOP_DEST_PORT : SHR_BFD_UDP_SINGLE_HOP_DEST_PORT;
    udp->length = SHR_BFD_UDP_HEADER_LENGTH + bfd_length;
    udp->sum = 0;    /* Calculated later */

    return BCM_E_NONE;
}

STATIC int
bcmi_xgs5_bfd_ipv4_header_get(int data_length, uint8 protocol,
                              uint8 tos, uint8 ttl,
                              bcm_ip_t src_ip_addr, bcm_ip_t dst_ip_addr,
                              bfd_ipv4_header_t *ip)
{
    uint8 buffer[SHR_BFD_IPV4_HEADER_LENGTH];

    sal_memset(ip, 0, sizeof(*ip));

    ip->version  = SHR_BFD_IPV4_VERSION;
    ip->h_length = SHR_BFD_IPV4_HEADER_LENGTH_WORDS;
    ip->dscp     = tos;
    ip->ecn      = 0;
    ip->length   = SHR_BFD_IPV4_HEADER_LENGTH + data_length;
    ip->id       = 0;
    ip->flags    = 0;
    ip->f_offset = 0;
    ip->ttl      = ttl;
    ip->protocol = protocol;
    ip->sum      = 0;
    ip->src      = src_ip_addr;
    ip->dst      = dst_ip_addr;

    /* Calculate IP header checksum */
    bcmi_xgs5_bfd_ipv4_header_pack(buffer, ip);
    ip->sum = _shr_ip_chksum(SHR_BFD_IPV4_HEADER_LENGTH, buffer);

    return BCM_E_NONE;
}

STATIC int
bcmi_xgs5_bfd_ipv6_header_get(int data_length, uint8 protocol,
                              uint8 tos, uint8 ttl,
                              bcm_ip6_t src_ip_addr, bcm_ip6_t dst_ip_addr,
                              bfd_ipv6_header_t *ip)
{
    sal_memset(ip, 0, sizeof(*ip));

    ip->version     = SHR_BFD_IPV6_VERSION;
    ip->t_class     = tos << 2;
    ip->f_label     = 0;  
    ip->p_length    = data_length;
    ip->next_header = protocol;
    ip->hop_limit   = ttl;
    sal_memcpy(ip->src, src_ip_addr, BCM_IP6_ADDRLEN);
    sal_memcpy(ip->dst, dst_ip_addr, BCM_IP6_ADDRLEN);

    return BCM_E_NONE;
}

STATIC int
bcmi_xgs5_bfd_ip_headers_get(bcm_bfd_endpoint_info_t *endpoint_info,
                             uint32 packet_flags,
                             int udp_length,
                             uint16 *etype,
                             bfd_ipv4_header_t *ipv4, bfd_ipv6_header_t *ipv6,
                             bfd_ipv4_header_t *inner_ipv4,
                             bfd_ipv6_header_t *inner_ipv6)
{
    int ip_protocol;
    int ip_length;

    if (!(packet_flags & BFD_ENCAP_PKT_IP)) {
        return BCM_E_PARAM;
    }

    /* Inner IP Header (IP-in-IP/GRE) */
    if (packet_flags & BFD_ENCAP_PKT_INNER_IP) {

        if (packet_flags & BFD_ENCAP_PKT_INNER_IP__V6) {
            ip_protocol = SHR_BFD_IP_PROTOCOL_IPV6_ENCAP;
            ip_length   = udp_length + SHR_BFD_IPV6_HEADER_LENGTH;
            BCM_IF_ERROR_RETURN
                (bcmi_xgs5_bfd_ipv6_header_get
                 (udp_length,
                  SHR_BFD_IP_PROTOCOL_UDP,
                  endpoint_info->inner_ip_tos,
                  endpoint_info->inner_ip_ttl,
                  endpoint_info->inner_src_ip6_addr,
                  endpoint_info->inner_dst_ip6_addr,
                  inner_ipv6));
        } else {
            ip_protocol = SHR_BFD_IP_PROTOCOL_IPV4_ENCAP;
            ip_length   = udp_length + SHR_BFD_IPV4_HEADER_LENGTH;
            BCM_IF_ERROR_RETURN
                (bcmi_xgs5_bfd_ipv4_header_get
                 (udp_length,
                  SHR_BFD_IP_PROTOCOL_UDP,
                  endpoint_info->inner_ip_tos,
                  endpoint_info->inner_ip_ttl,
                  endpoint_info->inner_src_ip_addr,
                  endpoint_info->inner_dst_ip_addr,
                  inner_ipv4));
        }
    } else {
        ip_protocol = SHR_BFD_IP_PROTOCOL_UDP;
        ip_length   = udp_length;
    }

    /* If GRE is present, GRE the protocol that follows the outer IP header */
    if (packet_flags & BFD_ENCAP_PKT_GRE) {
        ip_protocol = SHR_BFD_IP_PROTOCOL_GRE;
        ip_length   = udp_length + SHR_BFD_GRE_HEADER_LENGTH;
        if (packet_flags & BFD_ENCAP_PKT_INNER_IP__V6) {
            ip_length += SHR_BFD_IPV6_HEADER_LENGTH;
        } else {
            ip_length += SHR_BFD_IPV4_HEADER_LENGTH;
        }
    }

    /* Outer IP Header */
    if (packet_flags & BFD_ENCAP_PKT_IP__V6) {
        *etype = SHR_BFD_L2_ETYPE_IPV6;
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_ipv6_header_get(ip_length,
                                           ip_protocol,
                                           endpoint_info->ip_tos,
                                           endpoint_info->ip_ttl,
                                           endpoint_info->src_ip6_addr,
                                           endpoint_info->dst_ip6_addr,
                                           ipv6));
    } else {
        *etype = SHR_BFD_L2_ETYPE_IPV4;
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_ipv4_header_get(ip_length,
                                           ip_protocol,
                                           endpoint_info->ip_tos,
                                           endpoint_info->ip_ttl,
                                           endpoint_info->src_ip_addr,
                                           endpoint_info->dst_ip_addr,
                                           ipv4));
    }

    return BCM_E_NONE;
}

STATIC int
bcmi_xgs5_bfd_gre_header_get(uint32 packet_flags, bfd_gre_header_t *gre)
{
    sal_memset(gre, 0, sizeof(*gre));

    gre->checksum          = 0;
    gre->routing           = 0;
    gre->key               = 0;
    gre->sequence_num      = 0;
    gre->strict_src_route  = 0;
    gre->recursion_control = 0;
    gre->flags             = 0;
    gre->version           = 0;
    if (packet_flags & BFD_ENCAP_PKT_INNER_IP__V6) {
        gre->protocol = SHR_BFD_L2_ETYPE_IPV6;
    } else {
        gre->protocol = SHR_BFD_L2_ETYPE_IPV4;
    }

    return BCM_E_NONE;
}

STATIC int
bcmi_xgs5_bfd_ach_header_get(uint32 packet_flags, bfd_ach_header_t *ach)
{
    sal_memset(ach, 0, sizeof(*ach));

    ach->f_nibble = SHR_BFD_ACH_FIRST_NIBBLE;
    ach->version  = SHR_BFD_ACH_VERSION;
    ach->reserved = 0;

    if (packet_flags & BFD_ENCAP_PKT_G_ACH__IP) {    /* UDP/IP */
        if (packet_flags & BFD_ENCAP_PKT_IP__V6) {
            ach->channel_type = SHR_BFD_ACH_CHANNEL_TYPE_IPV6;
        } else {
            ach->channel_type = SHR_BFD_ACH_CHANNEL_TYPE_IPV4;
        }

    } else if (packet_flags & BFD_ENCAP_PKT_G_ACH__CC) { /* MPLS-TP CC only */
        ach->channel_type = SHR_BFD_ACH_CHANNEL_TYPE_MPLS_TP_CC;

    } else if (packet_flags & BFD_ENCAP_PKT_G_ACH__CC_CV) { /* MPLS-TP CC/CV */
        /*
         * CC/CV is done by interleaving CC and CV PDUs.
         * Set CC channel type as a start.
         * BFD engine (uKernel) will set appropriate channel types.
         * One CV PDU is inserted every second.
         */
        ach->channel_type = SHR_BFD_ACH_CHANNEL_TYPE_MPLS_TP_CC;

    } else {    /* Raw */
        ach->channel_type = SHR_BFD_ACH_CHANNEL_TYPE_RAW;
    }

    return BCM_E_NONE;
}

STATIC int
bcmi_xgs5_bfd_mpls_label_get(uint32 label, uint8 exp, uint8 s, uint8 ttl,
                             bfd_mpls_label_t *mpls)
{
    sal_memset(mpls, 0, sizeof(*mpls));

    mpls->label = label;
    mpls->exp   = exp;
    mpls->s     = s;
    mpls->ttl   = ttl;

    return BCM_E_NONE;
}

STATIC int
bcmi_xgs5_bfd_mpls_gal_label_get(bfd_mpls_label_t *mpls)
{
    return bcmi_xgs5_bfd_mpls_label_get(SHR_BFD_MPLS_GAL_LABEL,
                                        0, 1, 1, mpls);
}

STATIC int
bcmi_xgs5_bfd_mpls_router_alert_label_get(bfd_mpls_label_t *mpls)
{
    return bcmi_xgs5_bfd_mpls_label_get(SHR_BFD_MPLS_ROUTER_ALERT_LABEL,
                                        0, 0, 1, mpls);
}

STATIC int
bcmi_xgs5_bfd_mpls_labels_get(int unit, bcm_bfd_endpoint_info_t *endpoint_info,
                              uint32 packet_flags,
                              int max_count, bfd_mpls_label_t *mpls,
                              int *mpls_count)
{
    int count = 0;
    bcm_l3_egress_t l3_egress;
    bcm_mpls_port_t mpls_port;
    bcm_mpls_egress_label_t label_array[BFD_MPLS_MAX_LABELS];
    int label_count;
    int i;

    /* Get MPLS Tunnel Label(s) */
    bcm_l3_egress_t_init(&l3_egress);
    if (BCM_FAILURE(bcm_esw_l3_egress_get(unit, endpoint_info->egress_if,
                                          &l3_egress))) {
        return BCM_E_PARAM;
    }

    /* Look for a tunnel associated with this interface */
    if (BCM_SUCCESS
        (bcm_esw_mpls_tunnel_initiator_get(unit, l3_egress.intf,
                                           BFD_MPLS_MAX_LABELS,
                                           label_array, &label_count))) {
        for (i = 0; (i < label_count) && (count < max_count); i++) {
            bcmi_xgs5_bfd_mpls_label_get(label_array[i].label,
                                         label_array[i].exp,
                                         0,
                                         label_array[i].ttl,
                                         &mpls[count++]);
        }
    }

    /* MPLS Router Alert */
    if (packet_flags & BFD_ENCAP_PKT_MPLS_ROUTER_ALERT) {
        bcmi_xgs5_bfd_mpls_router_alert_label_get(&mpls[count++]);
    }

    /* Get L2 VPN PW VC label */
    if (BCM_GPORT_IS_MPLS_PORT(endpoint_info->gport)) {
        /* Get mpls port and label info */
        bcm_mpls_port_t_init(&mpls_port);
        mpls_port.mpls_port_id = endpoint_info->gport;
        if (BCM_FAILURE
            (bcm_esw_mpls_port_get(unit, endpoint_info->vpn,
                                   &mpls_port))) {
            return BCM_E_PARAM;
        } else {
            if (endpoint_info->type == bcmBFDTunnelTypePweTtl) {
                mpls_port.egress_label.ttl = 0x1;
            }

            bcmi_xgs5_bfd_mpls_label_get(mpls_port.egress_label.label,
                                         mpls_port.egress_label.exp,
                                         0,
                                         mpls_port.egress_label.ttl,
                                         &mpls[count++]);
        }
    }

    /* Set Bottom of Stack if there is no GAL label */
    if (!(packet_flags & BFD_ENCAP_PKT_GAL)) {
        mpls[count-1].s = 1;
    }

    *mpls_count = count;

    return BCM_E_NONE;

}

STATIC int
bcmi_xgs5_bfd_l2_header_get(int unit, bcm_bfd_endpoint_info_t *endpoint_info,
                            uint32 packet_flags, bcm_port_t port,
                            uint16 etype, bfd_l2_header_t *l2)
{
    uint16 tpid;
    bcm_l3_egress_t l3_egress;
    bcm_l3_intf_t l3_intf;
    bcm_vlan_t vid;
    int tpid_select;
    bcm_vlan_control_vlan_t vc;

    sal_memset(l2, 0, sizeof(*l2));

    /* Get L3 interfaces */
    bcm_l3_egress_t_init(&l3_egress);
    bcm_l3_intf_t_init(&l3_intf);

    if (BCM_FAILURE
        (bcm_esw_l3_egress_get(unit, endpoint_info->egress_if, &l3_egress))) {
        return BCM_E_PARAM;
    }

    l3_intf.l3a_intf_id = l3_egress.intf;
    if (BCM_FAILURE(bcm_esw_l3_intf_get(unit, &l3_intf))) {
        return BCM_E_PARAM;
    }

    /* Get TPID */
    if (BCM_FAILURE
        (bcm_esw_vlan_control_port_get(unit, port, bcmVlanPortOuterTpidSelect,
            &tpid_select))) {
        return BCM_E_INTERNAL; 
    }     
    
    if (tpid_select == BCM_PORT_OUTER_TPID) { 
    if (BCM_FAILURE(bcm_esw_port_tpid_get(unit, port, &tpid))) {
        return BCM_E_INTERNAL;
    }
    } else {
        if (BCM_FAILURE
            (bcm_esw_vlan_control_vlan_get(unit, l3_intf.l3a_vid, &vc))) {
            return BCM_E_INTERNAL;
        }
        tpid = vc.outer_tpid; 
    }

    /* Destination and Source MAC */
    sal_memcpy(l2->dst_mac, l3_egress.mac_addr, BFD_MAC_ADDR_LENGTH);
    sal_memcpy(l2->src_mac, l3_intf.l3a_mac_addr, BFD_MAC_ADDR_LENGTH);

    /*
     * VLAN Tag
     *
     * Skip VLAN tag if port's untagged VLAN matches the given VLAN,
     * or if HG header is present.
     */
    l2->outer_vlan_tag.tpid     = tpid;
    if (BCM_SUCCESS(bcm_esw_port_untagged_vlan_get(unit, port, &vid))) {
        if ((vid == l3_intf.l3a_vid) ||
            (packet_flags & (BFD_ENCAP_PKT_HG | BFD_ENCAP_PKT_HG2))) {
            l2->outer_vlan_tag.tpid = 0;  /* Set to 0 to indicate untagged */
        } else {
            l2->outer_vlan_tag.tci.prio = endpoint_info->vlan_pri;
            l2->outer_vlan_tag.tci.cfi  = 0;
            l2->outer_vlan_tag.tci.vid  = l3_intf.l3a_vid;
        }
    }

    if (l3_intf.l3a_inner_vlan != 0) {
        if (BCM_FAILURE(bcm_esw_port_inner_tpid_get(unit, port, &tpid))) {
            return BCM_E_INTERNAL;
        }
        l2->inner_vlan_tag.tpid     = tpid;
        l2->inner_vlan_tag.tci.prio = endpoint_info->inner_vlan_pri;
        l2->inner_vlan_tag.tci.cfi  = 0;
        l2->inner_vlan_tag.tci.vid  = l3_intf.l3a_inner_vlan;
    } else {
        l2->inner_vlan_tag.tpid = 0;  /* Set to 0 to indicate untagged */
    }

    /* Ether Type */
    l2->etype = etype;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_encap_build_pack
 * Purpose:
 *      Builds and packs the BFD packet encapsulation for a given
 *      BFD tunnel type.
 * Parameters:
 *      unit            - (IN) Unit number.
 *      endpoint_config - (IN/OUT) Pointer to BFD endpoint structure.
 *      packet_flags    - (IN) Flags for building packet.
 *      buffer          - (OUT) Buffer returning BFD encapsulation.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The returning BFD encapsulation includes only all the
 *      encapsulation headers/labels and does not include
 *      the BFD control packet.
 */
STATIC int
bcmi_xgs5_bfd_encap_build_pack(int unit,
                               bfd_endpoint_config_t *endpoint_config,
                               uint32 packet_flags,
                               uint8 *buffer)
{
    bcm_bfd_endpoint_info_t *endpoint_info = &endpoint_config->info;
    uint8          *cur_ptr = buffer;
    bcm_port_t     tx_port;  /* Local port to TX packet */
    uint16         etype = 0;
    bfd_udp_header_t  udp;
    bfd_ipv4_header_t ipv4, inner_ipv4;
    bfd_ipv6_header_t ipv6, inner_ipv6;
    bfd_gre_header_t  gre;
    int            v6;
    bfd_ach_header_t  ach;
    bfd_mpls_label_t  mpls_gal;
    bfd_mpls_label_t  mpls_labels[BFD_MPLS_MAX_LABELS];
    int            mpls_count = 0;
    int            i;
    bfd_l2_header_t   l2;
    int            ip_offset = 0;

    sal_memset(&udp, 0, sizeof(udp));
    sal_memset(&ipv4, 0, sizeof(ipv4));
    sal_memset(&inner_ipv4, 0, sizeof(inner_ipv4));
    sal_memset(&ipv6, 0, sizeof(ipv6));
    sal_memset(&inner_ipv6, 0, sizeof(inner_ipv6));

    tx_port = endpoint_config->tx_port;

    /*
     * Get necessary headers/labels information.
     *
     * Following order is important since some headers/labels
     * may depend on previous header/label information.
     */
    if (packet_flags & BFD_ENCAP_PKT_UDP) {
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_udp_header_get(unit, endpoint_info,
                                          packet_flags, &udp));
    }

    if (packet_flags & BFD_ENCAP_PKT_IP) {
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_ip_headers_get(endpoint_info,
                                          packet_flags, udp.length,
                                          &etype,
                                          &ipv4, &ipv6,
                                          &inner_ipv4, &inner_ipv6));
    }

    /* Set UDP checksum with corresponding IP header information */
    if (packet_flags & BFD_ENCAP_PKT_UDP) {
        v6 = 0;
        if (packet_flags & BFD_ENCAP_PKT_INNER_IP) {
            if (packet_flags & BFD_ENCAP_PKT_INNER_IP__V6) {
                v6 = 1;
            }
            bcmi_xgs5_bfd_udp_checksum_set(v6, &inner_ipv4, &inner_ipv6, &udp);
        } else {
            if (packet_flags & BFD_ENCAP_PKT_IP__V6) {
                v6 = 1;
            }
            bcmi_xgs5_bfd_udp_checksum_set(v6, &ipv4, &ipv6, &udp);
        }
    }

    if (packet_flags & BFD_ENCAP_PKT_GRE) {
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_gre_header_get(packet_flags, &gre));
    }

    if (packet_flags & BFD_ENCAP_PKT_G_ACH) {
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_ach_header_get(packet_flags, &ach));
    }

    if (packet_flags & BFD_ENCAP_PKT_GAL) {
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_mpls_gal_label_get(&mpls_gal));
    }

    if (packet_flags & BFD_ENCAP_PKT_MPLS) {
        etype = SHR_BFD_L2_ETYPE_MPLS_UCAST;
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_mpls_labels_get(unit, endpoint_info,
                                           packet_flags,
                                           BFD_MPLS_MAX_LABELS,
                                           mpls_labels, &mpls_count));

        /* User explicitly set the MPLS egress label exp and ttl values */
        if (endpoint_info->egress_label.exp != 0xFF) {
            if (endpoint_info->egress_label.exp <= 0x07) {
                mpls_labels[0].exp = endpoint_info->egress_label.exp; 
            } else { 
                return BCM_E_PARAM;
            }
        }
        if (endpoint_info->egress_label.ttl != 0) {
           mpls_labels[0].ttl = endpoint_info->egress_label.ttl; 
        }
    }

    /* Always build L2 Header */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_l2_header_get(unit, endpoint_info, packet_flags,
                                     tx_port, etype, &l2));


    /*
     * Pack header/labels into given buffer (network packet format).
     *
     * Following packing order must be followed to correctly
     * build the packet encapsulation.
     */
    cur_ptr = buffer;

    /* HG Header */
    if (packet_flags & BFD_ENCAP_PKT_HG) {
        cur_ptr = bcmi_xgs5_bfd_hg_header_pack(cur_ptr, unit,
                                               endpoint_config, &l2);
    } else if (packet_flags & BFD_ENCAP_PKT_HG2) {
        cur_ptr = bcmi_xgs5_bfd_hg2_header_pack(cur_ptr, unit,
                                                endpoint_config, &l2);
    }
    if (cur_ptr == NULL) {
        return BCM_E_INTERNAL;
    }

    /* L2 Header is always present */
    cur_ptr = bcmi_xgs5_bfd_l2_header_pack(cur_ptr, &l2);

    if (packet_flags & BFD_ENCAP_PKT_MPLS) {
        for (i = 0; i < mpls_count; i++) {
            cur_ptr = bcmi_xgs5_bfd_mpls_label_pack(cur_ptr, &mpls_labels[i]);
        }
    }

    /* Remaining packet headers */
    if (packet_flags & BFD_ENCAP_PKT_GAL) {
        cur_ptr = bcmi_xgs5_bfd_mpls_label_pack(cur_ptr, &mpls_gal);
    }
    if (packet_flags & BFD_ENCAP_PKT_G_ACH) {
        cur_ptr = bcmi_xgs5_bfd_ach_header_pack(cur_ptr, &ach);
    }
    if (packet_flags & BFD_ENCAP_PKT_GRE) {
    }
    if (packet_flags & BFD_ENCAP_PKT_IP) {
        /*
         * IP offset for Lookup Key
         * For IP-in-IP, this is the outer IP header
         */
        ip_offset = cur_ptr - buffer;
        if (packet_flags & BFD_ENCAP_PKT_IP__V6) {
            cur_ptr = bcmi_xgs5_bfd_ipv6_header_pack(cur_ptr, &ipv6);
        } else {
            cur_ptr = bcmi_xgs5_bfd_ipv4_header_pack(cur_ptr, &ipv4);
        }
    }
    if (packet_flags & BFD_ENCAP_PKT_GRE) {
        cur_ptr = bcmi_xgs5_bfd_gre_header_pack(cur_ptr, &gre);
    }
    if (packet_flags & BFD_ENCAP_PKT_INNER_IP) {
        if (packet_flags & BFD_ENCAP_PKT_INNER_IP__V6) {
            cur_ptr = bcmi_xgs5_bfd_ipv6_header_pack(cur_ptr, &inner_ipv6);
        } else {
            cur_ptr = bcmi_xgs5_bfd_ipv4_header_pack(cur_ptr, &inner_ipv4);
        }
    }
    if (packet_flags & BFD_ENCAP_PKT_UDP) {
        cur_ptr = bcmi_xgs5_bfd_udp_header_pack(cur_ptr, &udp);
    }


    /* Set BFD encapsulation length */
    endpoint_config->encap_length = cur_ptr - buffer;

    /*
     * Set Lookup Key for initial BFD packets
     * Offset is relative to the start of a given encapsulation data.
     *
     * Lookup Key:
     *   UDP/IP  : IPSA of RX packet       = IPDA on BFD Session
     *   IP-in-IP: Outer IPSA of RX packet = Outer IPDA on BFD Session
     *   MPLS    : MPLS BOS of RX packet   = 'label' on BFD endpoint struct
     *             - If GAL label (BOS) is present, then use MPLS label above.
     *             - 'mpls_label' on BFD endpoint is the incomming inner MPLS
     *               label.
     */
    endpoint_config->lkey_etype  = etype;
    endpoint_config->lkey_offset = 0;
    endpoint_config->lkey_length = 0;

    if (etype == SHR_BFD_L2_ETYPE_IPV4) {
        endpoint_config->lkey_offset = ip_offset +
            SHR_BFD_IPV4_HEADER_DA_OFFSET;
        endpoint_config->lkey_length = SHR_BFD_IPV4_HEADER_SA_LENGTH;

    } else if (etype ==  SHR_BFD_L2_ETYPE_IPV6) {
        endpoint_config->lkey_offset = ip_offset +
            SHR_BFD_IPV6_HEADER_DA_OFFSET;
        endpoint_config->lkey_length = SHR_BFD_IPV6_HEADER_SA_LENGTH;

    } else if (etype == SHR_BFD_L2_ETYPE_MPLS_UCAST) {
        /*
         * The lookup key is the incomming inner MPLS label,
         * which is specified when the BFD endpoint is create.
         *
         * The lookup key data is located in the 'mpls_label'
         * field of the message, rather than in the encap data
         * packet buffer.
         */
        endpoint_config->lkey_offset = 0;
        endpoint_config->lkey_length = SHR_BFD_MPLS_LABEL_LABEL_LENGTH;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_encap_data_dump
 * Purpose:
 *      Dumps buffer contents.
 * Parameters:
 *      buffer  - (IN) Buffer to dump data.
 *      length  - (IN) Length of buffer.
 * Returns:
 *      None
 */
void
bcmi_xgs5_bfd_encap_data_dump(uint8 *buffer, int length)
{
    int i;

    LOG_CLI((BSL_META("\nBFD encapsulation (length=%d):\n"), length));

    for (i = 0; i < length; i++) {
        if ((i % 10) == 0) {
            LOG_CLI((BSL_META("\n")));
        }
        LOG_CLI((BSL_META(" 0x%02x"), buffer[i]));
    }

    LOG_CLI((BSL_META("\n")));
    return;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_ip_validate_1
 * Purpose:
 *      Performs the following IP address check and IP TTL configuration:
 *      - IP Source Address is routable
 *      - IP Destination Addresss is routable
 *      - Sets IP TTL=255  for 1-hop
 * Parameters:
 *      endpoint_info - (IN/OUT) Pointer to BFD endpoint info structure.
 *      packet_flags  - (IN) Packet flags.
 * Returns:
 *      BCM_E_XXX
 */
int
bcmi_xgs5_bfd_ip_validate_1(bcm_bfd_endpoint_info_t *endpoint_info,
                            uint32 packet_flags)
{

    /* IP-SA and IP-DA Routable */
    if (packet_flags & BFD_ENCAP_PKT_IP__V6) {
        ENCAP_IPV6_ROUTABLE_VALIDATE(endpoint_info->src_ip6_addr);
        ENCAP_IPV6_ROUTABLE_VALIDATE(endpoint_info->dst_ip6_addr);
        if (BCM_IP6_MULTICAST(endpoint_info->src_ip6_addr)) {
            return BCM_E_PARAM;
        }
        if (IPV6_LOOPBACK(endpoint_info->dst_ip6_addr)) {
            return BCM_E_PARAM;
        }
    } else {
        if (BCM_IP4_MULTICAST(endpoint_info->src_ip_addr)) {
            return BCM_E_PARAM;
        }
        if (IPV4_LOOPBACK(endpoint_info->dst_ip_addr)) {
            return BCM_E_PARAM;
        }
    }

    if (!(packet_flags & BFD_ENCAP_PKT_UDP__MULTI_HOP)) {
        endpoint_info->ip_ttl = 255;    /* 1-hop */
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_ip_validate_2
 * Purpose:
 *      Performs the following IP address check and IP TTL configuration:
 *      - IP Source Address is routable
 *      - IP Destination Addresss is loopback or routable address
 *      - Sets IP TTL=1  for 1-hop when IP DA is loopback,
 *        or   IP TTL=255 for 1-hop when IP DA is a routable address         
 * Parameters:
 *      endpoint_info - (IN/OUT) Pointer to BFD endpoint info structure.
 *      packet_flags  - (IN) Packet flags.
 * Returns:
 *      BCM_E_XXX
 */
int
bcmi_xgs5_bfd_ip_validate_2(bcm_bfd_endpoint_info_t *endpoint_info,
                            uint32 packet_flags)
{
    int ip_ttl = 0;
    bcm_ip_t dst_ip_addr;
    bcm_ip6_t src_ip6_addr, dst_ip6_addr;
    int v6 = 0;

    if (packet_flags & BFD_ENCAP_PKT_INNER_IP) {
        if (packet_flags & BFD_ENCAP_PKT_INNER_IP__V6) {
            v6 = 1;
        }
        dst_ip_addr  = endpoint_info->inner_dst_ip_addr;
        sal_memcpy(src_ip6_addr, endpoint_info->inner_src_ip6_addr,
                   BCM_IP6_ADDRLEN);
        sal_memcpy(dst_ip6_addr, endpoint_info->inner_dst_ip6_addr,
                   BCM_IP6_ADDRLEN);
    } else {
        if (packet_flags & BFD_ENCAP_PKT_IP__V6) {
            v6 = 1;
        }
        dst_ip_addr  = endpoint_info->dst_ip_addr;
        sal_memcpy(src_ip6_addr, endpoint_info->src_ip6_addr,
                   BCM_IP6_ADDRLEN);
        sal_memcpy(dst_ip6_addr, endpoint_info->dst_ip6_addr,
                   BCM_IP6_ADDRLEN);
    }

    /* IP-SA Routable */
    if (v6) {
        ENCAP_IPV6_ROUTABLE_VALIDATE(src_ip6_addr);
    } 

    /* IP-DA:
     *     Loopback: 127/8 or 0:0:0:0:0:FFFF:127/104, then ttl=1
     *     Routable: then ttl=255
     */
    if (v6) {
        if (IPV6_LOOPBACK(dst_ip6_addr)) {
            ip_ttl = 1;
        } else {
            ENCAP_IPV6_ROUTABLE_VALIDATE(dst_ip6_addr);
            ip_ttl = 255;
        }
    } else {
        if (IPV4_LOOPBACK(dst_ip_addr)) {
            ip_ttl = 1;
        } else {
            ip_ttl = 255;
        }
    }

    if (!(packet_flags & BFD_ENCAP_PKT_UDP__MULTI_HOP)) {
        /* 1-hop */
        if (packet_flags & BFD_ENCAP_PKT_INNER_IP) {
            endpoint_info->inner_ip_ttl = ip_ttl;
        } else {
            endpoint_info->ip_ttl = ip_ttl;
        }            
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcmi_xgs5_bfd_encap_create
 * Purpose:
 *      Creates a BFD packet encapsulation.
 * Parameters:
 *      unit            - (IN) Unit number.
 *      endpoint_config - (IN/OUT) Pointer to BFD endpoint structure.
 *      encap_data      - (OUT) Buffer returning BFD encapsulation.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The returning BFD encapsulation buffer includes all the
 *      corresponding headers/labels EXCEPT for the BFD control packet.
 */
STATIC int
bcmi_xgs5_bfd_encap_create(int unit,
                           bfd_endpoint_config_t *endpoint_config,
                           uint8 *encap_data)
{
    bcm_bfd_endpoint_info_t *endpoint_info;
    uint32 packet_flags;
    uint32 ip_flag;
    bcm_port_t tx_port;  /* Local port to TX packet */
    bcm_pbmp_t st_pbmp;
    bcm_pbmp_t tx_pbmp;

    endpoint_info = &endpoint_config->info;
    tx_port       = endpoint_config->tx_port;

    /* Check for incompatible flags */
    if ((endpoint_info->flags & BCM_BFD_ENDPOINT_PWE_RAW) &&
        (endpoint_info->flags & BCM_BFD_ENDPOINT_IPV6)) {
        return BCM_E_PARAM;
    }

    packet_flags = 0;
    ip_flag      = 0;

    /* Set IPv4 or IPv6 flag */
    if (endpoint_info->flags & BCM_BFD_ENDPOINT_IPV6) {
        ip_flag |= BFD_ENCAP_PKT_IP__V6;
    }

    /* 1-hop or Multi-hop */
    if (endpoint_info->flags & BCM_BFD_ENDPOINT_MULTIHOP) {
        packet_flags |= BFD_ENCAP_PKT_UDP__MULTI_HOP;
    }

    /*
     * Get BFD encapsulation packet format flags
     *
     * Also, perform the following for each BFD tunnel type:
     * - Check for valid parameter values
     * - Set specific values required by the BFD tunnel definition 
     *   (e.g. such as ttl=1,...)
     */
    switch (endpoint_info->type) {
    case bcmBFDTunnelTypeUdp:
        packet_flags |= 
            (BFD_ENCAP_PKT_IP | ip_flag |
             BFD_ENCAP_PKT_UDP);

        ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);

        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_1(endpoint_info,
                                                        packet_flags));
        break;

    case bcmBFDTunnelTypeIp4in4:
        packet_flags |= 
            (BFD_ENCAP_PKT_IP |
             BFD_ENCAP_PKT_INNER_IP |
             BFD_ENCAP_PKT_UDP);

        ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);

        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_1(endpoint_info,
                                                        packet_flags));
        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_2(endpoint_info,
                                                        packet_flags));
        break;


    case bcmBFDTunnelTypeIp6in4:
        packet_flags |= 
            (BFD_ENCAP_PKT_IP |
             BFD_ENCAP_PKT_INNER_IP | BFD_ENCAP_PKT_INNER_IP__V6 |
             BFD_ENCAP_PKT_UDP);

        ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);

        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_1(endpoint_info,
                                                        packet_flags));
        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_2(endpoint_info,
                                                        packet_flags));
        break;

    case bcmBFDTunnelTypeIp4in6:
        packet_flags |= 
            (BFD_ENCAP_PKT_IP | BFD_ENCAP_PKT_IP__V6 |
             BFD_ENCAP_PKT_INNER_IP |
             BFD_ENCAP_PKT_UDP);

        ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);

        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_1(endpoint_info,
                                                        packet_flags));
        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_2(endpoint_info,
                                                        packet_flags));
        break;

    case bcmBFDTunnelTypeIp6in6:
        packet_flags |= 
            (BFD_ENCAP_PKT_IP | BFD_ENCAP_PKT_IP__V6 |
             BFD_ENCAP_PKT_INNER_IP | BFD_ENCAP_PKT_INNER_IP__V6 |
             BFD_ENCAP_PKT_UDP);

        ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);

        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_1(endpoint_info,
                                                        packet_flags));
        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_2(endpoint_info,
                                                        packet_flags));
        break;

    case bcmBFDTunnelTypeGre4In4:
        /*
         * bcmBFDTunnelTypeGRE is an old enum, which is now equivalent
         * to the new GRE IPv4-in-IPv4 tunnel type bcmBFDTunnelTypeGre4In4.
         */
        packet_flags |=
            (BFD_ENCAP_PKT_GRE |
             BFD_ENCAP_PKT_IP |
             BFD_ENCAP_PKT_INNER_IP |
             BFD_ENCAP_PKT_UDP);

        ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);

        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_1(endpoint_info,
                                                        packet_flags));
        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_2(endpoint_info,
                                                        packet_flags));
        break;

    case bcmBFDTunnelTypeGre6In4:
        packet_flags |=
            (BFD_ENCAP_PKT_GRE |
             BFD_ENCAP_PKT_IP |
             BFD_ENCAP_PKT_INNER_IP | BFD_ENCAP_PKT_INNER_IP__V6 |
             BFD_ENCAP_PKT_UDP);

        ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);

        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_1(endpoint_info,
                                                        packet_flags));
        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_2(endpoint_info,
                                                        packet_flags));
        break;

    case bcmBFDTunnelTypeGre4In6:
        packet_flags |=
            (BFD_ENCAP_PKT_GRE |
             BFD_ENCAP_PKT_IP | BFD_ENCAP_PKT_IP__V6 |
             BFD_ENCAP_PKT_INNER_IP |
             BFD_ENCAP_PKT_UDP);

        ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);

        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_1(endpoint_info,
                                                        packet_flags));
        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_2(endpoint_info,
                                                        packet_flags));
        break;

    case bcmBFDTunnelTypeGre6In6:

        packet_flags |=
            (BFD_ENCAP_PKT_GRE |
             BFD_ENCAP_PKT_IP | BFD_ENCAP_PKT_IP__V6 |
             BFD_ENCAP_PKT_INNER_IP | BFD_ENCAP_PKT_INNER_IP__V6 |
             BFD_ENCAP_PKT_UDP);

        ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);

        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_1(endpoint_info,
                                                        packet_flags));
        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_2(endpoint_info,
                                                        packet_flags));
        break;

    case bcmBFDTunnelTypeMpls:
        packet_flags |=
            (BFD_ENCAP_PKT_MPLS |
             BFD_ENCAP_PKT_IP | ip_flag |
             BFD_ENCAP_PKT_UDP);

        ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);

        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_2(endpoint_info,
                                                        packet_flags));

        break;

    case bcmBFDTunnelTypeMplsTpCc:
        packet_flags |=
            (BFD_ENCAP_PKT_MPLS |
             BFD_ENCAP_PKT_G_ACH |
             BFD_ENCAP_PKT_G_ACH__CC);

        if (endpoint_config->is_l3_vpn) {
            packet_flags |= BFD_ENCAP_PKT_GAL;
        }

        break;

    case bcmBFDTunnelTypeMplsTpCcCv:
        packet_flags |=
            (BFD_ENCAP_PKT_MPLS |
             BFD_ENCAP_PKT_G_ACH |
             BFD_ENCAP_PKT_G_ACH__CC_CV);

        if (endpoint_config->is_l3_vpn) {
            packet_flags |= BFD_ENCAP_PKT_GAL;
        }

        if ((endpoint_info->mep_id_length <= 0) ||
            (endpoint_info->mep_id_length >
             _SHR_BFD_ENDPOINT_MAX_MEP_ID_LENGTH)) {
            return BCM_E_PARAM;
        }
        break;

    case bcmBFDTunnelTypePweControlWord:
        packet_flags |=
            (BFD_ENCAP_PKT_MPLS |
             BFD_ENCAP_PKT_G_ACH);

        /* Check for incompatible flags */
        if ((endpoint_info->flags & BCM_BFD_ENDPOINT_DEMAND) ||
            (endpoint_info->flags & BCM_BFD_ENDPOINT_PASSIVE)) {
            return BCM_E_PARAM;
        }

        /* PW required */
        if (!BCM_GPORT_IS_MPLS_PORT(endpoint_info->gport)) {
            return BCM_E_PARAM;
        }

        /* Raw mode or UDP/IP */
        if (!(endpoint_info->flags & BCM_BFD_ENDPOINT_PWE_RAW)) {
            /* UDP/IP */
            ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);
            packet_flags |= BFD_ENCAP_PKT_G_ACH__IP;
            packet_flags |=
                (BFD_ENCAP_PKT_IP | ip_flag |
                 BFD_ENCAP_PKT_UDP);

            BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_2(endpoint_info,
                                                            packet_flags));
        }
        break;

    case bcmBFDTunnelTypePweRouterAlert:
        packet_flags |= BFD_ENCAP_PKT_MPLS_ROUTER_ALERT;
        /* Fall through */
    case bcmBFDTunnelTypePweTtl:
        packet_flags |=
            BFD_ENCAP_PKT_MPLS;

        /* Check for incompatible flags */
        if ((endpoint_info->flags & BCM_BFD_ENDPOINT_DEMAND) ||
            (endpoint_info->flags & BCM_BFD_ENDPOINT_PASSIVE)) {
            return BCM_E_PARAM;
        }

        /* PW required */
        if (!BCM_GPORT_IS_MPLS_PORT(endpoint_info->gport)) {
            return BCM_E_PARAM;
        }

        /*
         * If ACH is present, UDP/IP and RAW can be used
         * Otherwise, only UDP/IP can be set
         */
        if (endpoint_info->flags & BCM_BFD_ENDPOINT_PWE_ACH) {
            packet_flags |= BFD_ENCAP_PKT_G_ACH;
            /* Raw or UDP/IP */
            if (!(endpoint_info->flags & BCM_BFD_ENDPOINT_PWE_RAW)) {
                packet_flags |= BFD_ENCAP_PKT_G_ACH__IP;
            }
        } else {
            if (endpoint_info->flags & BCM_BFD_ENDPOINT_PWE_RAW) {
                return BCM_E_PARAM;
            }
            packet_flags |= BFD_ENCAP_PKT_G_ACH__IP;
        }
        
        if (packet_flags & BFD_ENCAP_PKT_G_ACH__IP) {
           /* UDP/IP */
            ENCAP_UDP_SRCPORT_VALIDATE(endpoint_info->udp_src_port);
            packet_flags |= BFD_ENCAP_PKT_G_ACH__IP;
            packet_flags |=
                (BFD_ENCAP_PKT_IP | ip_flag |
                 BFD_ENCAP_PKT_UDP);

            BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_ip_validate_2(endpoint_info,
                                                            packet_flags));
        }
        break;

    default:
        return BCM_E_PARAM;
    }

    /* Check if HG Header is needed (TX port is HG) */
    SOC_PBMP_PORT_SET(tx_pbmp, tx_port);
    SOC_PBMP_ASSIGN(st_pbmp, PBMP_ST_ALL(unit));
    BCM_API_XLATE_PORT_PBMP_P2A(unit, &st_pbmp);
    SOC_PBMP_AND(st_pbmp, tx_pbmp);
    if (SOC_PBMP_NOT_NULL(st_pbmp)) {
        if (IS_HG2_ENABLED_PORT(unit, tx_port)) {
            packet_flags |= BFD_ENCAP_PKT_HG2;
        } else {
            packet_flags |= BFD_ENCAP_PKT_HG;
        }
    }

    /* Build header/labels and pack in buffer */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_encap_build_pack(unit,
                                        endpoint_config,
                                        packet_flags,
                                        encap_data));

    /*
     * Set encap type.  Used to indicate uC for special
     * handling on certain encapsulation types:
     * - UDP checksum update is required
     * - packet is MPLS-TP CC only
     * - packet is MPLS-TP CC/CV
     */
    if (packet_flags & BFD_ENCAP_PKT_UDP) {
        if (packet_flags & BFD_ENCAP_PKT_IP__V6) {
            endpoint_config->encap_type = SHR_BFD_ENCAP_TYPE_V6UDP;
        } else {
            endpoint_config->encap_type = SHR_BFD_ENCAP_TYPE_V4UDP;
        }
    } else if (packet_flags & BFD_ENCAP_PKT_G_ACH__CC) {
        endpoint_config->encap_type = SHR_BFD_ENCAP_TYPE_MPLS_TP_CC;
    } else if (packet_flags & BFD_ENCAP_PKT_G_ACH__CC_CV) {
        endpoint_config->encap_type = SHR_BFD_ENCAP_TYPE_MPLS_TP_CC_CV;
    } else {
        endpoint_config->encap_type = SHR_BFD_ENCAP_TYPE_RAW;
    }                    

#ifdef BFD_DEBUG_DUMP
    bcmi_xgs5_bfd_encap_data_dump(encap_data, endpoint_config->encap_length);
#endif

    return BCM_E_NONE;
}
    
/*
 * Function:
 *      bcmi_xgs5_bfd_session_hw_set
 * Purpose:
 *      Sets BFD Session in HW device.
 * Parameters:
 *      unit            - (IN) Unit number.
 *      endpoint_config - (IN) Pointer to BFD endpoint structure.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_bfd_session_hw_set(int unit,
                             bfd_endpoint_config_t *endpoint_config)
{
    int rv = BCM_E_NONE;
    bcm_bfd_endpoint_info_t *endpoint_info;

    endpoint_info  = &endpoint_config->info;

    switch(endpoint_info->type) {
    case bcmBFDTunnelTypeUdp:
        /* Set L2_ENTRY entry, Your Discriminator lookup */
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_l2x_entry_set(unit, endpoint_config, 0));

        /* L3 entry */
        L3_LOCK(unit);
        rv = bcmi_xgs5_bfd_l3_entry_set(unit, endpoint_info);
        L3_UNLOCK(unit);

        break;

    case bcmBFDTunnelTypeIp4in4:
    case bcmBFDTunnelTypeIp6in4:
    case bcmBFDTunnelTypeIp4in6:
    case bcmBFDTunnelTypeIp6in6:
    case bcmBFDTunnelTypeGre4In4:
    case bcmBFDTunnelTypeGre6In4:
    case bcmBFDTunnelTypeGre4In6:
    case bcmBFDTunnelTypeGre6In6:
        /* Set L2_ENTRY entry, Your Discriminator lookup */
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_l2x_entry_set(unit, endpoint_config, 0));

        /* L3_TUNNEL */
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_l3_tunnel_entry_set(unit, endpoint_info));

        break;

    case bcmBFDTunnelTypeMpls:
    case bcmBFDTunnelTypeMplsTpCc:
    case bcmBFDTunnelTypeMplsTpCcCv:
    case bcmBFDTunnelTypePweControlWord:
    case bcmBFDTunnelTypePweRouterAlert:
    case bcmBFDTunnelTypePweTtl:
#if defined(BCM_MPLS_SUPPORT)
        if ((BCM_ESW_BFD_DRV(unit) == NULL) ||
            (BFD_HW_CALL(unit)->mpls_lock == NULL) ||
            (BFD_HW_CALL(unit)->mpls_unlock == NULL)) {
            return BCM_E_UNAVAIL;
        }

        /* Set L2_ENTRY entry, MPLS lookup */
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_l2x_entry_set(unit, endpoint_config, 1));

        /* MPLS entry */
        BCM_IF_ERROR_RETURN(BFD_HW_CALL(unit)->mpls_lock(unit));
        rv = bcmi_xgs5_bfd_mpls_entry_set(unit, endpoint_config);
        BFD_HW_CALL(unit)->mpls_unlock(unit);
#else
        return BCM_E_UNAVAIL;
#endif /* BCM_MPLS_SUPPORT */
        break;

    default:
        rv = BCM_E_UNAVAIL;
        break;
    }

    return rv;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_session_hw_delete
 * Purpose:
 *      Delete BFD Session in HW device.
 * Parameters:
 *      unit            - (IN) Unit number.
 *      endpoint_config - (IN) Pointer to BFD endpoint structure.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_bfd_session_hw_delete(int unit,
                                bfd_endpoint_config_t *endpoint_config)
{
    int rv = BCM_E_NONE;
    bcm_bfd_endpoint_info_t *endpoint_info;
    int index, ref_count = 0;
    bfd_info_t *bfd_info = BFD_INFO(unit);

    endpoint_info  = &endpoint_config->info;

    switch(endpoint_config->info.type) {
    case bcmBFDTunnelTypeUdp:
        for (index = 0; index < bfd_info->endpoint_count; index++) {
            if (SHR_BITGET(bfd_info->endpoints_in_use, index)) {
                endpoint_config = BFD_ENDPOINT_CONFIG(unit, index);
                /* Check for IPv4 */
                if (endpoint_config->encap_type == SHR_BFD_ENCAP_TYPE_V4UDP){
                    if (endpoint_config->info.src_ip_addr == endpoint_info->src_ip_addr) {
                        ref_count++;
                    }
                }
                /* Check for IPv6 */
                else if (endpoint_config->encap_type == SHR_BFD_ENCAP_TYPE_V6UDP) {
                    if (BCM_IP6_ADDR_EQ (endpoint_config->info.src_ip6_addr, endpoint_info->src_ip6_addr)) {
                        ref_count++;
                    }
                }
            }
        }
        /* Don't delete the l3 entry till last BFD session is destroyed */ 
        if (ref_count <= 1) {
            /* L3 entry */
            L3_LOCK(unit);
            rv = bcmi_xgs5_bfd_l3_entry_clear(unit, endpoint_info);
            L3_UNLOCK(unit);
        }

        /* L2_ENTRY */
        rv = bcmi_xgs5_bfd_l2_entry_clear(unit, 0,
                                          endpoint_info->label,
                                          endpoint_info->local_discr);
        break;

    case bcmBFDTunnelTypeIp4in4:
    case bcmBFDTunnelTypeIp6in4:
    case bcmBFDTunnelTypeIp4in6:
    case bcmBFDTunnelTypeIp6in6:
    case bcmBFDTunnelTypeGre4In4:
    case bcmBFDTunnelTypeGre6In4:
    case bcmBFDTunnelTypeGre4In6:
    case bcmBFDTunnelTypeGre6In6:
        /* L3_TUNNEL */
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_l3_tunnel_entry_clear(unit, endpoint_info));

        /* L2_ENTRY */
        rv = bcmi_xgs5_bfd_l2_entry_clear(unit, 0,
                                          endpoint_info->label,
                                          endpoint_info->local_discr);
        break;

    case bcmBFDTunnelTypeMpls:
    case bcmBFDTunnelTypeMplsTpCc:
    case bcmBFDTunnelTypeMplsTpCcCv:
    case bcmBFDTunnelTypePweControlWord:
    case bcmBFDTunnelTypePweRouterAlert:
    case bcmBFDTunnelTypePweTtl:
#if defined(BCM_MPLS_SUPPORT)
        if ((BCM_ESW_BFD_DRV(unit) == NULL) ||
            (BFD_HW_CALL(unit)->mpls_lock == NULL) ||
            (BFD_HW_CALL(unit)->mpls_unlock == NULL)) {
            return BCM_E_UNAVAIL;
        }
        /* MPLS entry */
        BCM_IF_ERROR_RETURN(BFD_HW_CALL(unit)->mpls_lock(unit));
        rv = bcmi_xgs5_bfd_mpls_entry_clear(unit, endpoint_info);
        BFD_HW_CALL(unit)->mpls_unlock(unit);
        BCM_IF_ERROR_RETURN(rv);

        /* L2_ENTRY */
        rv = bcmi_xgs5_bfd_l2_entry_clear(unit, 1,
                                          endpoint_info->label,
                                          endpoint_info->local_discr);
#else
        return BCM_E_UNAVAIL;
#endif /* BCM_MPLS_SUPPORT */
        break;

    default:
        break;
    }

    return rv;
}
#endif /* BCM_CMICM_SUPPORT */


/*
 * Function:
 *      bcmi_xgs5_bfd_init
 * Purpose:
 *      Initialize the BFD subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_MEMORY Unable to allocate memory for internal control structures
 *      BCM_E_INTERNAL Failed to initialize
 *      BCM_E_CONFIG   Configuration error 
 *      BCM_E_UNAVAIL  Not supported 
 * Notes:
 *      BFD initialization will return BCM_E_NONE for the following
 *      error conditions (i.e. feature not supported):
 *      - uKernel is not loaded
 *      - BFD application is not found in any of the uC uKernel
 */
int
bcmi_xgs5_bfd_init(int unit,
                   bcm_esw_bfd_drv_t *drv,
                   bcm_xgs5_bfd_hw_defs_t *hw_defs)
{
#ifdef BCM_CMICM_SUPPORT
    int rv = BCM_E_NONE;
    bfd_info_t *bfd_info;
    int num_sessions;
    int num_auth_sha1, num_auth_sp;
    int cpu_cosq;
    shr_bfd_msg_ctrl_init_t msg_init;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int uc;
    int priority;

    FEATURE_CHECK(unit);

    PARAM_NULL_CHECK(drv);
    PARAM_NULL_CHECK(hw_defs);

    if (SAL_BOOT_QUICKTURN) {
        bfd_uc_msg_timeout_usecs = BFD_UC_MSG_TIMEOUT_USECS_QUICKTURN;
    } else {
        bfd_uc_msg_timeout_usecs = BFD_UC_MSG_TIMEOUT_USECS;
    }

    /* Check if uKernel is loaded */
    for (uc = 0; uc < CMICM_NUM_UCS; uc++) {
        if (soc_cmic_uc_msg_active_wait(unit, uc) == SOC_E_NONE) {
            break;
        }
    }
    /* Return silently if there is no active CMICm */
    if (uc >= CMICM_NUM_UCS) {
        LOG_WARN(BSL_LS_BCM_BFD,
                 (BSL_META_U(unit,
                             "uKernel not Ready, BFD not started\n")));
        return BCM_E_NONE;
    }

    /* Detach if already initialized */
    if (BFD_INIT(unit)) {
        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_detach(unit));
    }

    /* Install Tables */
    BCM_ESW_BFD_DRV(unit) = drv;
    BFD_HW(unit)          = hw_defs;

    /* Get SOC properties */
    num_sessions = soc_property_get(unit, spn_BFD_NUM_SESSIONS, 256);
    if (num_sessions > BFD_MAX_NUM_ENDPOINTS) {
        return BCM_E_CONFIG;
    }
    num_auth_sha1 = soc_property_get(unit, spn_BFD_SHA1_KEYS, 0);
    num_auth_sp   = soc_property_get(unit, spn_BFD_SIMPLE_PASSWORD_KEYS, 0);
    cpu_cosq      = soc_property_get(unit, spn_BFD_COSQ, BFD_COSQ_INVALID);

    /*
     * Initialize HOST side
     */
    rv = bcmi_xgs5_bfd_alloc_resource(unit, num_sessions,
                                      num_auth_sha1, num_auth_sp);
    if (BCM_FAILURE(rv)) {
        bcmi_xgs5_bfd_free_resource(unit);
        return rv;
    }

    bfd_info                     = BFD_INFO(unit);
    bfd_info->unit               = unit;
    bfd_info->endpoint_count     = num_sessions;
    bfd_info->num_auth_sha1_keys = num_auth_sha1;
    bfd_info->num_auth_sp_keys   = num_auth_sp;
    bfd_info->cpu_cosq           = cpu_cosq;
    bfd_info->cosq_spath_index   = -1;
    bfd_info->cosq_ach_err_index = -1;

#ifdef BCM_WARM_BOOT_SUPPORT
    if (!SOC_WARM_BOOT(unit)) {
#endif
    /*
     * Initialize uController side
     */

    /*
     * Start BFD application in BTE (Broadcom Task Engine) uController.
     * Determine which uController is running BFD  by choosing the first
     * uC that returns successfully.
     */
    for (uc = 0; uc < CMICM_NUM_UCS; uc++) {
        rv = soc_cmic_uc_appl_init(unit, uc, MOS_MSG_CLASS_BFD,
                                   bfd_uc_msg_timeout_usecs,
                                   BFD_SDK_VERSION,
                                   BFD_UC_MIN_VERSION);
        if (SOC_E_NONE == rv) {
            /* BFD started successfully */
            bfd_info->uc_num = uc;
            break;
        }
    }

    if (uc >= CMICM_NUM_UCS) {
        /* Could not find or start BFD appl */
        bcmi_xgs5_bfd_free_resource(unit);
        LOG_WARN(BSL_LS_BCM_BFD,
                 (BSL_META_U(unit,
                             "uKernel BFD application not available\n")));
        return BCM_E_NONE;
    }
#ifdef BCM_WARM_BOOT_SUPPORT
   } 
#endif

    /* RX DMA channel (0..3) local to the uC */
    bfd_info->rx_channel = BCM_XGS5_BFD_RX_CHANNEL;

#ifdef BCM_WARM_BOOT_SUPPORT
    if (!SOC_WARM_BOOT(unit)) {
#endif

    /* Set control message data */
    sal_memset(&msg_init, 0, sizeof(msg_init));
    msg_init.num_sessions       = bfd_info->endpoint_count;
    msg_init.encap_size         = 0;    /* Not used */
    msg_init.num_auth_sha1_keys = bfd_info->num_auth_sha1_keys;
    msg_init.num_auth_sp_keys   = bfd_info->num_auth_sp_keys;
    msg_init.rx_channel         = bfd_info->rx_channel;
    
    /* Pack control message data into DMA buffer */
    buffer     = bfd_info->dma_buffer;
    buffer_ptr = shr_bfd_msg_ctrl_init_pack(buffer, &msg_init);
    buffer_len = buffer_ptr - buffer;

    /* Send BFD Init message to uC */
    rv = bcmi_xgs5_bfd_msg_send_receive(unit,
                                        MOS_MSG_SUBCLASS_BFD_INIT,
                                        buffer_len, 0,
                                        MOS_MSG_SUBCLASS_BFD_INIT_REPLY,
                                        &reply_len);

    if (BCM_FAILURE(rv) || (reply_len != 0)) {
        bcmi_xgs5_bfd_free_resource(unit);
        if (BCM_SUCCESS(rv)) {
            rv = BCM_E_INTERNAL;
        }
        return rv;
    }

    /*
     * Initialize HW
     */
    rv = bcmi_xgs5_bfd_hw_init(unit);
    if (BCM_FAILURE(rv)) {
        BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_detach(unit));
        return rv;
    }
#ifdef BCM_WARM_BOOT_SUPPORT
   } 
#endif

    /*
     * Initialize Event handler
     */
    /* Start event thread handler */
    priority = soc_property_get(unit, spn_BFD_THREAD_PRI, BCM_ESW_BFD_THREAD_PRI_DFLT);
    if (bfd_info->event_thread_id == NULL) {
        if (sal_thread_create("bcmBFD", SAL_THREAD_STKSZ, 
                              priority,
                              (void (*)(void*))bcmi_xgs5_bfd_callback_thread,
                              INT_TO_PTR(unit)) == SAL_THREAD_ERROR) {
            if (SAL_BOOT_QUICKTURN) {
                /* If emulation, then wait */
                sal_usleep(1000000);
            }

            if (bfd_info->event_thread_id == NULL) {
                BCM_IF_ERROR_RETURN(bcmi_xgs5_bfd_detach(unit));
                return BCM_E_MEMORY;
            }
        }
    }
    bfd_info->event_thread_kill = 0;

    /* Module has been initialized */
    bfd_info->initialized = 1;

#if defined(BCM_WARM_BOOT_SUPPORT)
    if(!SOC_WARM_BOOT(unit)) {
        bcmi_xgs5_bfd_scache_alloc(unit);
    }
    if (SOC_WARM_BOOT(unit)) {
        rv = bcmi_xgs5_bfd_reinit(unit);
        if (BCM_FAILURE(rv)) {
            bcmi_xgs5_bfd_free_resource(unit);
            return (rv);
        }
    }
#endif

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_detach
 * Purpose:
 *      Shut down the BFD subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 * Notes:
 *      In progress...
 */
int
bcmi_xgs5_bfd_detach(int unit)
{
#ifdef BCM_CMICM_SUPPORT
    int rv;
    bfd_info_t *bfd_info = BFD_INFO(unit);
    bfd_endpoint_config_t *endpoint_config;
    uint16 reply_len;
    int index;
    sal_usecs_t timeout = 0;

    FEATURE_CHECK(unit);

    if (!BFD_INIT(unit)) {
        return BCM_E_NONE;
    }

    /* Delete BFD Sessions in HW device */
    for (index = 0; index < bfd_info->endpoint_count; index++) {
        if (SHR_BITGET(bfd_info->endpoints_in_use, index)) {
            endpoint_config = BFD_ENDPOINT_CONFIG(unit, index);

            bcmi_xgs5_bfd_session_hw_delete(unit, endpoint_config);

            SHR_BITCLR(bfd_info->endpoints_in_use,
                       endpoint_config->endpoint_index);
            endpoint_config->endpoint_index = -1;
        }
    }

    /* Event Handler thread exit signal */
    if (bfd_info->event_thread_id) {
        bfd_info->event_thread_kill = 1;
        soc_cmic_uc_msg_receive_cancel(unit, bfd_info->uc_num,
                                   MOS_MSG_CLASS_BFD_EVENT);

        /*
         * Ensure here itself that the callback thread exists as we are going
         * to free bcm_xgs5_bfd_info below and we will loose reference
         * to the thread
         */
        timeout = sal_time_usecs() + 5000000;
        while (bfd_info->event_thread_id != NULL) {
            if (sal_time_usecs() < timeout) {
                /*give some time to already running bfd thread to schedule and exit*/
                sal_usleep(1000);
            } else {
                /*timeout*/
                LOG_ERROR(BSL_LS_BCM_BFD,
                          (BSL_META_U(unit,
                                      "BFD event thread did not exit.\n")));
                return BCM_E_INTERNAL;
            }
        }
    }

    /*
     * Send BFD Uninit message to uC
     * Ignore error since that may indicate uKernel was reloaded.
     */
    rv = bcmi_xgs5_bfd_msg_send_receive(unit,
                                        MOS_MSG_SUBCLASS_BFD_UNINIT,
                                        0, 0,
                                        MOS_MSG_SUBCLASS_BFD_UNINIT_REPLY,
                                        &reply_len);
    if (BCM_SUCCESS(rv) && (reply_len != 0)) {
        return BCM_E_INTERNAL;
    }

    /* Delete CPU COS queue mapping entries for BFD packets */
    if (bfd_info->cosq_spath_index >= 0) {
        (void) bcm_esw_rx_cosq_mapping_delete(unit,
                                              bfd_info->cosq_spath_index);
        bfd_info->cosq_spath_index = -1;
    }
    if (bfd_info->cosq_ach_err_index >= 0) {
        (void) bcm_esw_rx_cosq_mapping_delete(unit,
                                              bfd_info->cosq_ach_err_index);
        bfd_info->cosq_ach_err_index = -1;
    }

    /* Delete entry for special Session-ID for Unknown Remote Discriminator */
    (void) bcmi_xgs5_bfd_l2_entry_clear(unit,
                                        0, 0,
                                        SHR_BFD_DISCR_UNKNOWN_YOUR_DISCR);

    /* Free resources */
    bcmi_xgs5_bfd_event_unregister_all(unit);
    bcmi_xgs5_bfd_free_resource(unit);
    bcm_xgs5_bfd_info[unit] = NULL;

    /* Uninstall Tables */
    BCM_ESW_BFD_DRV(unit) = NULL;
    BFD_HW(unit)          = NULL;

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_endpoint_create
 * Purpose:
 *      Create or update an BFD endpoint object
 * Parameters:
 *      unit          - (IN) Unit number.
 *      endpoint_info - (IN/OUT) Pointer to an BFD endpoint structure.
 * Returns:
 *      BCM_E_NONE      Operation completed successfully
 *      BCM_E_PARAM     Invalid parameter specified
 *      BCM_E_NOT_FOUND Attempt to update an endpoint which does not exist
 *      BCM_E_EXISTS    Attempt to create an endpoint with a specified ID
 *                      which is already in use
 *      BCM_E_FULL      No free endpoints available.
 *
 * Notes:
 */
int
bcmi_xgs5_bfd_endpoint_create(int unit,
                              bcm_bfd_endpoint_info_t *endpoint_info)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    bfd_endpoint_config_t *endpoint_config;
    int update;
    int endpoint_index;
    bcm_module_t module_id;
    bcm_port_t port_id;
    bcm_trunk_t trunk_id;
    int local_id;
    bcm_port_t tx_port;
    int is_local;
    bcm_l3_egress_t l3_egress;
    shr_bfd_msg_ctrl_sess_set_t msg_sess;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int encap = 0;
    uint32 session_flags;
    bfd_mpls_label_t mpls_label;

    BFD_FEATURE_INIT_CHECK(unit);

    PARAM_NULL_CHECK(endpoint_info);

    /* Check Authentication key index */
    if (endpoint_info->auth == bcmBFDAuthTypeNone) {
        /* Nothing to check */
    } else if (endpoint_info->auth == bcmBFDAuthTypeSimplePassword) {
        AUTH_SP_INDEX_CHECK(unit, endpoint_info->auth_index);
    } else if ((endpoint_info->auth == bcmBFDAuthTypeKeyedSHA1) ||
               (endpoint_info->auth == bcmBFDAuthTypeMeticulousKeyedSHA1)) {
        AUTH_SHA1_INDEX_CHECK(unit, endpoint_info->auth_index);
    } else {
        /*
         * Not supported:
         * bcmBFDAuthTypeKeyedMD5 and bcmBFDAuthTypeMeticulousKeyedMD5
         */
        return BCM_E_PARAM;
    }

    /* A unique local discriminator should be nonzero */
    if (!endpoint_info->local_discr) {
        LOG_ERROR(BSL_LS_BCM_BFD,
                  (BSL_META_U(unit,
                              "BFD local discriminator cannot be zero\n")));
        return BCM_E_PARAM;
    }

    /* Local detection multiplier should be set to Default value, if configured value is 0 */
    if (!endpoint_info->local_detect_mult) {
        endpoint_info->local_detect_mult = BFD_DEFAULT_DETECT_MULTIPLIER;
        LOG_VERBOSE(BSL_LS_BCM_BFD,
                    (BSL_META_U(unit,
                                "\n BFD Detect Multiplier Set to Default Value %d\n"), 
                                endpoint_info->local_detect_mult));
    }

    /* Resolve module and port */
    sal_memset(&l3_egress, 0, sizeof(l3_egress));
    if (BCM_GPORT_INVALID != endpoint_info->gport) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_gport_resolve(unit, endpoint_info->gport,
                                    &module_id, &port_id,
                                    &trunk_id, &local_id));
        if (trunk_id != BCM_TRUNK_INVALID) {
            return BCM_E_PARAM;
        }
    } else if (BCM_SUCCESS
               (bcm_esw_l3_egress_get(unit, endpoint_info->egress_if,
                                      &l3_egress))) {
        module_id = l3_egress.module;
        port_id = l3_egress.port;
    } else {
        return BCM_E_PARAM;
    }

    /* Get local port used for TX BFD packet */
    BCM_IF_ERROR_RETURN
        (_bcm_esw_modid_is_local(unit, module_id, &is_local));
    if (is_local) {    /* Ethernet port */
        tx_port = port_id;
    } else {           /* HG port */
        BCM_IF_ERROR_RETURN
            (bcm_esw_stk_modport_get(unit, module_id, &tx_port));
    }


    /* Create or Update */
    update = (endpoint_info->flags & BCM_BFD_ENDPOINT_UPDATE) ? 1 : 0;
    session_flags = (update) ? 0 : SHR_BFD_SESS_SET_F_CREATE;

    if (endpoint_info->flags & BCM_BFD_ENDPOINT_WITH_ID) {
        endpoint_index = endpoint_info->id;

        ENDPOINT_INDEX_CHECK(unit, endpoint_index);

        /*
         * Update requires endpoint ID to exist
         * Create requires endpoint ID not to exist
         */
        if (update &&
            (!SHR_BITGET(bfd_info->endpoints_in_use, endpoint_index))) {
            return BCM_E_NOT_FOUND;
        } else if (!update &&
                   (SHR_BITGET(bfd_info->endpoints_in_use, endpoint_index))) {
            return BCM_E_EXISTS;
        }

    } else {    /* No ID */

        if (update) {    /* Update specified with no ID */
            return BCM_E_PARAM;
        }

        /* Allocate endpoint */
        endpoint_index =
            bcmi_xgs5_bfd_find_free_endpoint(bfd_info->endpoints_in_use,
                                             bfd_info->endpoint_count);
        if (endpoint_index < 0) {
            return BCM_E_FULL;
        }

        endpoint_info->id = endpoint_index;
    }

    /* Get Endpoint config entry */
    endpoint_config = BFD_ENDPOINT_CONFIG(unit, endpoint_index);

    /* Endpoint tunnel type cannot change */
    if (update && (endpoint_config->info.type != endpoint_info->type)) {
        return BCM_E_PARAM;
    }

    /*
     * Compare new and old BFD parameters for Update
     */
    if (update) {
        if (endpoint_config->info.local_discr !=
            endpoint_info->local_discr) {
            session_flags |= SHR_BFD_SESS_SET_F_LOCAL_DISC;
        }
        if (endpoint_config->info.remote_discr !=
            endpoint_info->remote_discr) {
            session_flags |= SHR_BFD_SESS_SET_F_REMOTE_DISC;
        }
        if (endpoint_config->info.local_min_tx !=
            endpoint_info->local_min_tx) {
            session_flags |= SHR_BFD_SESS_SET_F_LOCAL_MIN_TX;
        }
        if (endpoint_config->info.local_min_rx !=
            endpoint_info->local_min_rx) {
            session_flags |= SHR_BFD_SESS_SET_F_LOCAL_MIN_RX;
        }
        if (endpoint_config->info.local_min_echo !=
            endpoint_info->local_min_echo) {
            session_flags |= SHR_BFD_SESS_SET_F_LOCAL_MIN_ECHO_RX;
        }
        if (endpoint_config->info.local_diag !=
            endpoint_info->local_diag) {
            session_flags |= SHR_BFD_SESS_SET_F_LOCAL_DIAG;
        }
        if ((endpoint_config->info.flags & BCM_BFD_ENDPOINT_DEMAND)
            != (endpoint_info->flags & BCM_BFD_ENDPOINT_DEMAND)) {
            session_flags |= SHR_BFD_SESS_SET_F_LOCAL_DEMAND;
        }
        if (endpoint_config->info.local_detect_mult !=
            endpoint_info->local_detect_mult) {
            session_flags |= SHR_BFD_SESS_SET_F_LOCAL_DETECT_MULT;
        }
        if (endpoint_info->flags & BCM_BFD_ENDPOINT_ENCAP_SET) {
            session_flags |= SHR_BFD_SESS_SET_F_ENCAP;
        }
        if (endpoint_info->flags & BCM_BFD_ENDPOINT_SHA1_SEQUENCE_INCR) {
            session_flags |= SHR_BFD_SESS_SET_F_SHA1_XMT_SEQ_INCR;
        }
    }

    /* Set Endpoint config entry */
    endpoint_config->endpoint_index = endpoint_index;
    endpoint_config->modid          = module_id;
    endpoint_config->port           = port_id;
    endpoint_config->tx_port        = tx_port;
    endpoint_config->info           = *endpoint_info;

    /* Set BFD Session in HW */
    if (!update || (endpoint_info->flags & BCM_BFD_ENDPOINT_ENCAP_SET)) {
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_session_hw_set(unit, endpoint_config));
        encap = 1;
    }

    /* Set control message data */
    sal_memset(&msg_sess, 0, sizeof(msg_sess));

    /*
     * Set the BFD encapsulation data
     *
     * The function bcmi_xgs5_bfd_encap_create() is called first
     * since this sets some fields in 'endpoint_config' which are
     * used in the message.
     */
    if (encap) {
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_encap_create(unit, endpoint_config,
                                        msg_sess.encap_data));
    }
    msg_sess.sess_id = endpoint_config->endpoint_index;
    msg_sess.flags   = session_flags;
    msg_sess.passive =
        (endpoint_info->flags & BCM_BFD_ENDPOINT_PASSIVE) ? 1 : 0;
    msg_sess.local_demand =
        (endpoint_info->flags & BCM_BFD_ENDPOINT_DEMAND) ? 1 : 0;
    msg_sess.local_diag           = endpoint_info->local_diag;
    msg_sess.local_detect_mult    = endpoint_info->local_detect_mult;
    msg_sess.local_discriminator  = endpoint_info->local_discr;
    msg_sess.remote_discriminator = endpoint_info->remote_discr;
    msg_sess.local_min_tx         = endpoint_info->local_min_tx;
    msg_sess.local_min_rx         = endpoint_info->local_min_rx;
    msg_sess.local_min_echo_rx    = endpoint_info->local_min_echo;
    msg_sess.auth_type            = endpoint_info->auth;
    msg_sess.auth_key             = endpoint_info->auth_index;
    msg_sess.xmt_auth_seq         = endpoint_info->tx_auth_seq;
    if (encap) {
        msg_sess.encap_type   = endpoint_config->encap_type;
        msg_sess.encap_length = endpoint_config->encap_length;
        msg_sess.lkey_etype   = endpoint_config->lkey_etype;
        msg_sess.lkey_offset  = endpoint_config->lkey_offset;
        msg_sess.lkey_length  = endpoint_config->lkey_length;
        if (endpoint_info->mep_id_length) {
            msg_sess.mep_id_length = endpoint_info->mep_id_length;
            sal_memcpy(msg_sess.mep_id, endpoint_info->mep_id,
                       endpoint_info->mep_id_length);
        }
    }
    /* MPLS label needs to be in packet frame format (label field only) */
    bcmi_xgs5_bfd_mpls_label_get(endpoint_info->label, 0, 0, 0,
                                 &mpls_label);
    bcmi_xgs5_bfd_mpls_label_pack(msg_sess.mpls_label, &mpls_label);

    msg_sess.tx_port = endpoint_config->tx_port;
    msg_sess.tx_cos  = endpoint_info->int_pri;
    msg_sess.tx_pri  = 0;
    msg_sess.tx_qnum = SOC_INFO(unit).port_uc_cosq_base[msg_sess.tx_port] +
        endpoint_info->int_pri;

    /* Pack control message data into DMA buffer */
    buffer     = bfd_info->dma_buffer;
    buffer_ptr = shr_bfd_msg_ctrl_sess_set_pack(buffer, &msg_sess);
    buffer_len = buffer_ptr - buffer;

    /* Send BFD Session Set message to uC */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_msg_send_receive(unit,
                                        MOS_MSG_SUBCLASS_BFD_SESS_SET,
                                        buffer_len, 0,
                                        MOS_MSG_SUBCLASS_BFD_SESS_SET_REPLY,
                                        &reply_len));
    if (reply_len != 0) {
        return BCM_E_INTERNAL;
    }


    /* Mark endpoint in use */
    SHR_BITSET(bfd_info->endpoints_in_use, endpoint_config->endpoint_index);

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_endpoint_get
 * Purpose:
 *      Get an BFD endpoint object
 * Parameters:
 *      unit          - (IN) Unit number.
 *      endpoint      - (IN) The ID of the endpoint object to get.
 *      endpoint_info - (OUT) Pointer to an BFD endpoint structure
 *                            to receive the data.
 * Returns:
 *      BCM_E_NONE      Operation completed successfully
 *      BCM_E_NOT_FOUND Endpoint does not exist
 *      BCM_E_PARAM     Invalid parameter specified
 *      BCM_E_INTERNAL  Failed to read endpoint information
 * Notes:
 */
int
bcmi_xgs5_bfd_endpoint_get(int unit,
                           bcm_bfd_endpoint_t endpoint,
                           bcm_bfd_endpoint_info_t *endpoint_info)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    bfd_endpoint_config_t *endpoint_config;
    shr_bfd_msg_ctrl_sess_get_t msg_sess;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;

    BFD_FEATURE_INIT_CHECK(unit);

    PARAM_NULL_CHECK(endpoint_info);

    ENDPOINT_INDEX_CHECK(unit, endpoint);

    if (!SHR_BITGET(bfd_info->endpoints_in_use, endpoint)) {
        return BCM_E_NOT_FOUND;
    }

    endpoint_config = BFD_ENDPOINT_CONFIG(unit, endpoint);
    sal_memcpy(endpoint_info, &endpoint_config->info,
               sizeof(*endpoint_info));

    /* Send BFD Session Get message to uC */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_msg_send_receive(unit,
                                        MOS_MSG_SUBCLASS_BFD_SESS_GET,
                                        endpoint, 0,
                                        MOS_MSG_SUBCLASS_BFD_SESS_GET_REPLY,
                                        &reply_len));

    /* Unpack control message data from DMA buffer */
    sal_memset(&msg_sess, 0, sizeof(msg_sess));
    buffer     = bfd_info->dma_buffer;
    buffer_ptr = shr_bfd_msg_ctrl_sess_get_unpack(buffer, &msg_sess);
    buffer_len = buffer_ptr - buffer;

    if (reply_len != buffer_len) {
        return BCM_E_INTERNAL;
    }
    
    endpoint_info->local_state        = msg_sess.local_sess_state;
    endpoint_info->local_discr        = msg_sess.local_discriminator;
    endpoint_info->local_diag         = msg_sess.local_diag;
    endpoint_info->tx_auth_seq        = msg_sess.xmt_auth_seq;
    endpoint_info->rx_auth_seq        = msg_sess.rcv_auth_seq;
    endpoint_info->remote_state       = msg_sess.remote_sess_state;
    endpoint_info->remote_discr       = msg_sess.remote_discriminator;
    endpoint_info->remote_diag        = msg_sess.remote_diag;
    endpoint_info->remote_min_tx      = msg_sess.remote_min_tx;
    endpoint_info->remote_min_rx      = msg_sess.remote_min_rx;
    endpoint_info->remote_min_echo    = msg_sess.remote_min_echo_rx;
    endpoint_info->remote_detect_mult = msg_sess.remote_detect_mult;

    endpoint_info->remote_flags = 0;
    if (msg_sess.remote_demand) {
        endpoint_info->remote_flags |= BCM_BFD_ENDPOINT_REMOTE_DEMAND;
    }

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_endpoint_destroy
 * Purpose:
 *      Destroy an BFD endpoint object.
 * Parameters:
 *      unit     - (IN) Unit number.
 *      endpoint - (IN) The ID of the BFD endpoint object to destroy.
 * Returns:
 *      BCM_E_NONE      Operation completed successfully
 *      BCM_E_NOT_FOUND Attempt to destroy endpoint which does not exist
 *      BCM_E_INTERNAL  Unable to release resource /
 *                      Failed to read memory or read or write register
 * Notes:
 */
int
bcmi_xgs5_bfd_endpoint_destroy(int unit, bcm_bfd_endpoint_t endpoint)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    bfd_endpoint_config_t *endpoint_config;
    uint16 reply_len;

    BFD_FEATURE_INIT_CHECK(unit);

    ENDPOINT_INDEX_CHECK(unit, endpoint);

    if (!SHR_BITGET(bfd_info->endpoints_in_use, endpoint)) {
        return BCM_E_NOT_FOUND;
    }

    endpoint_config = BFD_ENDPOINT_CONFIG(unit, endpoint);

    /* Delete BFD Session in HW */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_session_hw_delete(unit, endpoint_config));

    /* Send BFD Session Delete message to uC */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_msg_send_receive(unit,
                                        MOS_MSG_SUBCLASS_BFD_SESS_DELETE,
                                        endpoint_config->endpoint_index, 0,
                                        MOS_MSG_SUBCLASS_BFD_SESS_DELETE_REPLY,
                                        &reply_len));

    if (reply_len != 0) {
        return BCM_E_INTERNAL;
    }


    SHR_BITCLR(bfd_info->endpoints_in_use, endpoint_config->endpoint_index);
    endpoint_config->endpoint_index = -1;

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_endpoint_destroy_all
 * Purpose:
 *      Destroy all BFD endpoint objects.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE     Operation completed successfully
 *      BCM_E_INTERNAL Unable to release resource /
 *                     Failed to read memory or read or write register
 * Notes:
 */
int
bcmi_xgs5_bfd_endpoint_destroy_all(int unit)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    bfd_endpoint_config_t *endpoint_config;
    uint16 reply_len;
    int index;

    BFD_FEATURE_INIT_CHECK(unit);

    for (index = 0; index < bfd_info->endpoint_count; index++) {
        if (SHR_BITGET(bfd_info->endpoints_in_use, index)) {
            endpoint_config = BFD_ENDPOINT_CONFIG(unit, index);

            bcmi_xgs5_bfd_session_hw_delete(unit, endpoint_config);

            SHR_BITCLR(bfd_info->endpoints_in_use,
                       endpoint_config->endpoint_index);
            endpoint_config->endpoint_index = -1;
        }
    }

    /* Send BFD Session Delete All message to uC */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_msg_send_receive
         (unit,
          MOS_MSG_SUBCLASS_BFD_SESS_DELETE_ALL,
          0, 0,
          MOS_MSG_SUBCLASS_BFD_SESS_DELETE_ALL_REPLY,
          &reply_len));

    if (reply_len != 0) {
        return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_endpoint_poll
 * Purpose:
 *      Poll an BFD endpoint object.  Valid only for endpoint in Demand
 *      Mode and in state bcmBFDStateUp.
 * Parameters:
 *      unit     - (IN) Unit number.
 *      endpoint - (IN) The ID of the BFD endpoint object to poll.
 * Returns:
 *      BCM_E_NONE      Operation completed successfully
 *      BCM_E_NOT_FOUND BFD Session ID not in use
 *      BCM_E_INTERNAL  Unable to read or write resource
 *      BCM_E_PARAM     Session not in Demand Mode/
 *                      Session not in UP state
 * Notes: The poll operation will be initiated.  Poll failure will be
 * signalled via asynchronous callback (aka session failure)
 */
int
bcmi_xgs5_bfd_endpoint_poll(int unit, bcm_bfd_endpoint_t endpoint)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    bfd_endpoint_config_t *endpoint_config;
    uint16 reply_len;

    BFD_FEATURE_INIT_CHECK(unit);

    ENDPOINT_INDEX_CHECK(unit, endpoint);

    if (!SHR_BITGET(bfd_info->endpoints_in_use, endpoint)) {
        return BCM_E_NOT_FOUND;
    }

    endpoint_config = BFD_ENDPOINT_CONFIG(unit, endpoint);

    /* Check for Demand mode */
    if (!(endpoint_config->info.flags & BCM_BFD_ENDPOINT_DEMAND)) {
        return BCM_E_CONFIG;
    }

    /* Issue poll request on session not in the UP state will fail */

    /* Send BFD Session Poll message to uC */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_msg_send_receive(unit,
                                        MOS_MSG_SUBCLASS_BFD_SESS_POLL,
                                        endpoint_config->endpoint_index, 0,
                                        MOS_MSG_SUBCLASS_BFD_SESS_POLL_REPLY,
                                        &reply_len));

    if (reply_len != 0) {
        return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_event_register
 * Purpose:
 *      Register a callback for handling BFD events.
 * Parameters:
 *      unit        - (IN) Unit number.
 *      event_types - (IN) The set of BFD events for which the specified
 *                    callback will be invoked.
 *      cb          - (IN) A pointer to the callback function.
 *      user_data   - (IN) Pointer to user data to pass to the callback.
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 * Notes:
 */
int
bcmi_xgs5_bfd_event_register(int unit,
                             bcm_bfd_event_types_t event_types,
                             bcm_bfd_event_cb cb, void *user_data)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    uint32 result;
    bfd_event_handler_t *event_handler;
    bfd_event_handler_t *previous = NULL;
    bcm_bfd_event_type_t event_type;

    BFD_FEATURE_INIT_CHECK(unit);

    PARAM_NULL_CHECK(cb);

    SHR_BITTEST_RANGE(event_types.w, 0, bcmBFDEventCount, result);

    if (result == 0) {
        return BCM_E_PARAM;
    }

    for (event_handler = bfd_info->event_handler_list;
         event_handler != NULL;
         event_handler = event_handler->next) {
        if (event_handler->cb == cb) {
            break;
        }

        previous = event_handler;
    }

    if (event_handler == NULL) {
        /* This handler hasn't been registered yet */

        event_handler = sal_alloc(sizeof(*event_handler), "BFD event handler");

        if (event_handler == NULL) {
            return BCM_E_MEMORY;
        }

        sal_memset(event_handler, 0, sizeof(*event_handler));

        event_handler->next = NULL;
        event_handler->cb = cb;

        SHR_BITCLR_RANGE(event_handler->event_types.w, 0, bcmBFDEventCount);

        if (previous != NULL) {
            previous->next = event_handler;
        } else {
            bfd_info->event_handler_list = event_handler;
        }
    }

    for (event_type = 0; event_type < bcmBFDEventCount; ++event_type) {
        if (SHR_BITGET(event_types.w, event_type)) {
            if (!SHR_BITGET(event_handler->event_types.w, event_type)) {
                /* This handler isn't handling this event yet */
                SHR_BITSET(event_handler->event_types.w, event_type);
            }
        }
    }

    event_handler->user_data = user_data;

    /* Update Events mask */
    return bcmi_xgs5_bfd_event_mask_set(unit);

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_event_unregister
 * Purpose:
 *      Unregister a callback for handling BFD events.
 * Parameters:
 *      unit        - (IN) Unit number.
 *      event_types - (IN) The set of BFD events for which the specified
 *                    callback should not be invoked.
 *      cb          - (IN) A pointer to the callback function.
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 * Notes:
 */
int
bcmi_xgs5_bfd_event_unregister(int unit,
                               bcm_bfd_event_types_t event_types,
                               bcm_bfd_event_cb cb)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    uint32 result;
    bfd_event_handler_t *event_handler;
    bfd_event_handler_t *previous = NULL;
    bcm_bfd_event_type_t event_type;

    BFD_FEATURE_INIT_CHECK(unit);

    PARAM_NULL_CHECK(cb);

    SHR_BITTEST_RANGE(event_types.w, 0, bcmBFDEventCount, result);

    if (result == 0) {
        return BCM_E_PARAM;
    }

    for (event_handler = bfd_info->event_handler_list;
         event_handler != NULL;
         event_handler = event_handler->next) {
        if (event_handler->cb == cb) {
            break;
        }

        previous = event_handler;
    }

    if (event_handler == NULL) {
        return BCM_E_NOT_FOUND;
    }

    for (event_type = 0; event_type < bcmBFDEventCount; ++event_type) {
        if (SHR_BITGET(event_types.w, event_type)) {
            SHR_BITCLR(event_handler->event_types.w, event_type);
        }
    }

    SHR_BITTEST_RANGE(event_handler->event_types.w, 0, bcmBFDEventCount,
        result);

    if (result == 0) {
        /* No more events for this handler to handle */
        if (previous != NULL) {
            previous->next = event_handler->next;
        } else {
            bfd_info->event_handler_list = event_handler->next;
        }

        sal_free(event_handler);
    }

    /* Update Events mask */
    return bcmi_xgs5_bfd_event_mask_set(unit);

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_endpoint_stat_get
 * Purpose:
 *      Get BFD endpoint statistics.
 * Parameters:
 *      unit     - (IN) Unit number.
 *      endpoint - (IN) The ID of the endpoint object to get stats for.
 *      ctr_info - (IN/OUT) Pointer to endpoint count structure to receive
 *                 the data.
 *      clear    - (IN) If set, clear stats after read.
 * Returns:
 *      BCM_E_NONE     Operation completed successfully
 *      BCM_E_PARAM    Null pointer to counter information structure
 *      BCM_E_MEMORY   Unable to allocate memory
 *      BCM_E_INTERNAL Unable to obtain/release resource lock /
 *                     Failed to read or write register
 * Notes:
 */
int
bcmi_xgs5_bfd_endpoint_stat_get(int unit,
                                bcm_bfd_endpoint_t endpoint,
                                bcm_bfd_endpoint_stat_t *ctr_info, uint8 clear)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    shr_bfd_msg_ctrl_stat_req_t stat_req;
    shr_bfd_msg_ctrl_stat_reply_t stat_reply;
    uint8 *buffer_req, *buffer_reply, *buffer_ptr;
    uint16 buffer_len, reply_len;

    BFD_FEATURE_INIT_CHECK(unit);

    PARAM_NULL_CHECK(ctr_info);

    ENDPOINT_INDEX_CHECK(unit, endpoint);

    if (!SHR_BITGET(bfd_info->endpoints_in_use, endpoint)) {
        return BCM_E_NOT_FOUND;
    }

    /* Set control message data */
    sal_memset(&stat_req, 0, sizeof(stat_req));
    stat_req.sess_id = endpoint;
    stat_req.clear   = clear;

    /* Pack control message data into DMA */
    buffer_req = bfd_info->dma_buffer;
    buffer_ptr = shr_bfd_msg_ctrl_stat_req_pack(buffer_req, &stat_req);
    buffer_len = buffer_ptr - buffer_req;

    /* Send BFD Stat Get message to uC */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_msg_send_receive(unit,
                                        MOS_MSG_SUBCLASS_BFD_STAT_GET,
                                        buffer_len, 0,
                                        MOS_MSG_SUBCLASS_BFD_STAT_GET_REPLY,
                                        &reply_len));

    /* Unpack control message data from DMA buffer */
    sal_memset(&stat_reply, 0, sizeof(stat_reply));
    buffer_reply = bfd_info->dma_buffer;
    buffer_ptr = shr_bfd_msg_ctrl_stat_reply_unpack(buffer_reply, &stat_reply);
    buffer_len = buffer_ptr - buffer_reply;
    if (reply_len != buffer_len) {
        return BCM_E_INTERNAL;
    }

    ctr_info->packets_in = stat_reply.packets_in;
    ctr_info->packets_out = stat_reply.packets_out;
    ctr_info->packets_drop = stat_reply.packets_drop;
    ctr_info->packets_auth_drop = stat_reply.packets_auth_drop;

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_auth_sha1_set
 * Purpose:
 *      Set SHA1 authentication entry.
 * Parameters:
 *      unit  - (IN) Unit number.
 *      index - (IN) Index of the SHA1 entry to configure.
 *      sha1  - (IN) Pointer to SHA1 info structure.
 * Returns:
 *      BCM_E_NONE     Operation completed successfully
 *      BCM_E_PARAM    Null pointer to SHA1 information structure
 *      BCM_E_MEMORY   Unable to allocate memory
 *      BCM_E_INTERNAL Unable to obtain/release resource lock /
 *                     Failed to read or write register
 * Notes:
 */
int
bcmi_xgs5_bfd_auth_sha1_set(int unit,
                            int index, bcm_bfd_auth_sha1_t *sha1)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    shr_bfd_msg_ctrl_auth_sha1_t msg_auth;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;

    BFD_FEATURE_INIT_CHECK(unit);

    PARAM_NULL_CHECK(sha1);

    AUTH_SHA1_INDEX_CHECK(unit, index);

    /* Set control message data */
    sal_memset(&msg_auth, 0, sizeof(msg_auth));
    msg_auth.index    = index;
    msg_auth.enable   = sha1->enable;
    msg_auth.sequence = sha1->sequence;
    sal_memcpy(msg_auth.key, sha1->key, _SHR_BFD_AUTH_SHA1_KEY_LENGTH);

    /* Pack control message data into DMA buffer */
    buffer     = bfd_info->dma_buffer;
    buffer_ptr = shr_bfd_msg_ctrl_auth_sha1_pack(buffer, &msg_auth);
    buffer_len = buffer_ptr - buffer;

    /* Send BFD Auth Sha1 Set message to uC */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_msg_send_receive
         (unit,
          MOS_MSG_SUBCLASS_BFD_AUTH_SHA1_SET,
          buffer_len, 0,
          MOS_MSG_SUBCLASS_BFD_AUTH_SHA1_SET_REPLY,
          &reply_len));
    if (reply_len != 0) {
        return BCM_E_INTERNAL;
    }

    bfd_info->auth_sha1[index] = *sha1;

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}


/*
 * Function:
 *      bcmi_xgs5_bfd_auth_sha1_get
 * Purpose:
 *      Get SHA1 authentication entry.
 * Parameters:
 *      unit  - (IN) Unit number.
 *      index - (IN) Index of the SHA1 entry to retrieve.
 *      sha1  - (IN/OUT) Pointer to SHA1 info structure to receive the data.
 * Returns:
 *      BCM_E_NONE     Operation completed successfully
 *      BCM_E_PARAM    Null pointer to SHA1 information structure
 *      BCM_E_MEMORY   Unable to allocate memory
 *      BCM_E_INTERNAL Unable to obtain/release resource lock /
 *                     Failed to read or write register
 * Notes:
 */
int
bcmi_xgs5_bfd_auth_sha1_get(int unit,
                            int index, bcm_bfd_auth_sha1_t *sha1)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    shr_bfd_msg_ctrl_auth_sha1_t msg_auth;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;

    BFD_FEATURE_INIT_CHECK(unit);

    PARAM_NULL_CHECK(sha1);

    AUTH_SHA1_INDEX_CHECK(unit, index);

    /* If warmboot, get authentication data from uController, else use cache */
    if (SOC_WARM_BOOT(unit)) {

        /* Send BFD Auth Sha1 Get message to uC */
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_msg_send_receive
             (unit,
              MOS_MSG_SUBCLASS_BFD_AUTH_SHA1_GET,
              index, 0,
              MOS_MSG_SUBCLASS_BFD_AUTH_SHA1_GET_REPLY,
              &reply_len));

        /* Unpack control message data from DMA buffer */
        sal_memset(&msg_auth, 0, sizeof(msg_auth));
        buffer     = bfd_info->dma_buffer;
        buffer_ptr = shr_bfd_msg_ctrl_auth_sha1_unpack(buffer, &msg_auth);
        buffer_len = buffer_ptr - buffer;

        if (reply_len != buffer_len) {
            return BCM_E_INTERNAL;
        }

        bfd_info->auth_sha1[index].enable   = msg_auth.enable;
        bfd_info->auth_sha1[index].sequence = msg_auth.sequence;
        sal_memcpy(bfd_info->auth_sha1[index].key, msg_auth.key,
                   _SHR_BFD_AUTH_SHA1_KEY_LENGTH);

    }

    *sha1 = bfd_info->auth_sha1[index];

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_auth_simple_password_set
 * Purpose:
 *      Set Simple Password authentication entry.
 * Parameters:
 *      unit  - (IN) Unit number.
 *      index - (IN) Index of the Simple Password entry to configure.
 *      sp    - (IN) Pointer to Simple Password info structure.
 * Returns:
 *      BCM_E_NONE     Operation completed successfully
 *      BCM_E_PARAM    Null pointer to SHA1 information structure
 *      BCM_E_MEMORY   Unable to allocate memory
 *      BCM_E_INTERNAL Unable to obtain/release resource lock /
 *                     Failed to read or write register
 * Notes:
 */
int
bcmi_xgs5_bfd_auth_simple_password_set(int unit,
                                       int index,
                                       bcm_bfd_auth_simple_password_t *sp)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    shr_bfd_msg_ctrl_auth_sp_t msg_auth;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;

    BFD_FEATURE_INIT_CHECK(unit);

    PARAM_NULL_CHECK(sp);

    AUTH_SP_INDEX_CHECK(unit, index);

    /* Password length MUST be from 1 to 16 bytes */
    if ((sp->length == 0) ||
        ( sp->length > _SHR_BFD_AUTH_SIMPLE_PASSWORD_KEY_LENGTH)) {
        LOG_ERROR(BSL_LS_BCM_COMMON,
                  (BSL_META_U(unit,
                              "BFD simple password length must be from 1 to 16 bytes\n")));
        return BCM_E_PARAM;
    }

    /* Set control message data */
    sal_memset(&msg_auth, 0, sizeof(msg_auth));
    msg_auth.index  = index;
    msg_auth.length = sp->length;
    sal_memcpy(msg_auth.password, sp->password,
               _SHR_BFD_AUTH_SIMPLE_PASSWORD_KEY_LENGTH);

    /* Pack control message data into DMA buffer */
    buffer     = bfd_info->dma_buffer;
    buffer_ptr = shr_bfd_msg_ctrl_auth_sp_pack(buffer, &msg_auth);
    buffer_len = buffer_ptr - buffer;

    /* Send BFD Auth SimplePassword Set message to uC */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_msg_send_receive(unit,
                                        MOS_MSG_SUBCLASS_BFD_AUTH_SP_SET,
                                        buffer_len, 0,
                                        MOS_MSG_SUBCLASS_BFD_AUTH_SP_SET_REPLY,
                                        &reply_len));

    if (reply_len != 0) {
        return BCM_E_INTERNAL;
    }

    bfd_info->auth_sp[index] = *sp;

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

/*
 * Function:
 *      bcmi_xgs5_bfd_auth_simple_password_get
 * Purpose:
 *      Get Simple Password authentication entry.
 * Parameters:
 *      unit  - (IN) Unit number.
 *      index - (IN) Index of the Simple Password entry to retrieve.
 *      sp    - (IN/OUT) Pointer to Simple Password info structure to
 *              receive the data.
 * Returns:
 *      BCM_E_NONE     Operation completed successfully
 *      BCM_E_PARAM    Null pointer to SHA1 information structure
 *      BCM_E_MEMORY   Unable to allocate memory
 *      BCM_E_INTERNAL Unable to obtain/release resource lock /
 *                     Failed to read or write register
 * Notes:
 */
int
bcmi_xgs5_bfd_auth_simple_password_get(int unit,
                                       int index,
                                       bcm_bfd_auth_simple_password_t *sp)
{
#ifdef BCM_CMICM_SUPPORT
    bfd_info_t *bfd_info = BFD_INFO(unit);
    shr_bfd_msg_ctrl_auth_sp_t msg_auth;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;

    BFD_FEATURE_INIT_CHECK(unit);

    PARAM_NULL_CHECK(sp);

    AUTH_SP_INDEX_CHECK(unit, index);

    /* If warmboot, get authentication data from uController, else use cache */
    if (SOC_WARM_BOOT(unit)) {

        /* Send BFD Auth SimplePassword Get message to uC */
        BCM_IF_ERROR_RETURN
            (bcmi_xgs5_bfd_msg_send_receive
             (unit,
              MOS_MSG_SUBCLASS_BFD_AUTH_SP_GET,
              index, 0,
              MOS_MSG_SUBCLASS_BFD_AUTH_SP_GET_REPLY,
              &reply_len));

        /* Unpack control message data from DMA buffer */
        sal_memset(&msg_auth, 0, sizeof(msg_auth));
        buffer     = bfd_info->dma_buffer;
        buffer_ptr = shr_bfd_msg_ctrl_auth_sp_unpack(buffer, &msg_auth);
        buffer_len = buffer_ptr - buffer;

        if (reply_len != buffer_len) {
            return BCM_E_INTERNAL;
        }

        bfd_info->auth_sp[index].length = msg_auth.length;
        sal_memcpy(bfd_info->auth_sp[index].password, msg_auth.password,
                   _SHR_BFD_AUTH_SIMPLE_PASSWORD_KEY_LENGTH);
    }

    *sp = bfd_info->auth_sp[index];

    return BCM_E_NONE;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}


#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *      bcmi_xgs5_bfd_sw_dump
 * Purpose:
 *      Displays BFD information maintained by software.
 * Parameters:
 *      unit  - (IN) Unit number.
 * Returns:
 *      None
 */
void
bcmi_xgs5_bfd_sw_dump(int unit)
{
    bfd_info_t *bfd_info = BFD_INFO(unit);
    bfd_endpoint_config_t *endpoint_config;
    int endpoint_idx;
    SHR_BITDCL word;

    if (!BFD_INIT(unit)) {
        return;
    }

    LOG_CLI((BSL_META_U(unit,
                        "\nSW Information BFD - Unit %d\n"), unit));

    LOG_CLI((BSL_META_U(unit,
                        "\n endpoints_in_use \n")));
    for (word = 0; word < _SHR_BITDCLSIZE
             (bfd_info->endpoint_count); word++) {
        LOG_CLI((BSL_META_U(unit,
                            " word %d value %x "), word,
                 bfd_info->endpoints_in_use[word]));
    }

    LOG_CLI((BSL_META_U(unit,
                        "\n Endpoint Information \n")));
    for (endpoint_idx = 0; endpoint_idx < bfd_info->endpoint_count;
         endpoint_idx++) {
        if (!SHR_BITGET(bfd_info->endpoints_in_use, endpoint_idx)) {
            continue;
        }
        endpoint_config = BFD_ENDPOINT_CONFIG(unit, endpoint_idx);
        LOG_CLI((BSL_META_U(unit,
                            "\n Endpoint index %d\n"), endpoint_idx));
        LOG_CLI((BSL_META_U(unit,
                            "\t Type %x\n"), endpoint_config->info.type));
        LOG_CLI((BSL_META_U(unit,
                            "\t VRF id %x\n"), endpoint_config->info.vrf_id));
    }

    return;
}
#endif  /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *       bcmi_xgs5_bfd_scache_alloc
 * Purpose:
 *     Allocate scache memory for OAM module
 * Parameters:
 *     unit - (IN) BCM device number
 * Returns:
 *     BCM_E_UNIT    - Invalid BCM unit number.
 *     BCM_E_UNAVAIL - OAM not support on this device.
 *     BCM_E_MEMORY  - Allocation failure
 *     CM_E_XXX     - Error code from bcm_XX_oam_init()
 *     BCM_E_NONE    - Success
 */
STATIC int
bcmi_xgs5_bfd_scache_alloc(int unit)
{
    bfd_info_t *bfd_info;
    soc_scache_handle_t scache_handle;
    uint8 *bfd_scache;
    int alloc_size = 0;

    bfd_info = BFD_INFO(unit);

    /* Calculate the size to store BFD num session count */
    alloc_size += sizeof(int);

    /* Calculate the size to store BFD  end point bit usage  usage */
    alloc_size += SHR_BITALLOCSIZE(bfd_info->endpoint_count);

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_OAM, 0);
    BCM_IF_ERROR_RETURN(_bcm_esw_scache_ptr_get(unit, scache_handle, 1,
                        alloc_size, &bfd_scache, BCM_WB_DEFAULT_VERSION, NULL));
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_bfd_sync
 * Purpose:
 *       
 * Parameters:
 *      unit - (IN) Unit number.
 * result =s:
 *      BCM_E_XXX
 * Notes:
 */
int
bcmi_xgs5_bfd_sync(int unit)
{
    int         rv = BCM_E_NONE;
    int         alloc_size = 0;
    int         stable_size;
    bfd_info_t  *bfd_info;
    uint8       *bfd_scache;
    int         idx = 0;
    soc_scache_handle_t scache_handle;
    BFD_INIT_CHECK(unit); 
    /* Get The BFD module storage size. */
    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    /* In case  WARM BOOT level 2 store is not configured return from here. */
    if (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit) || (stable_size == 0)) {
    /* Get The BFD module storage size. */
        return BCM_E_NONE;
    }
    do {
        /* BFD info structure for the given unit. */
        bfd_info = BFD_INFO(unit);

        /* Calculate the size to store BFD num session count */
        alloc_size += sizeof(int);

        /* Calculate the size to store BFD  end point bit usage  usage */
        alloc_size += SHR_BITALLOCSIZE(bfd_info->endpoint_count);

        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_BFD, 0);

        rv = _bcm_esw_scache_ptr_get(unit,
                                    scache_handle,
                                     0,
                                    alloc_size,
                                    &bfd_scache,
                                    BCM_WB_DEFAULT_VERSION,
                                    NULL
                                    );
        if (!SOC_WARM_BOOT(unit) && (BCM_E_NOT_FOUND == rv)) {
            rv = _bcm_esw_scache_ptr_get(unit,
                                        scache_handle,
                                        1,
                                        alloc_size,
                                         &bfd_scache,
                                        BCM_WB_DEFAULT_VERSION,
                                         NULL
                                        );
            if (BCM_FAILURE(rv) || (NULL == bfd_scache)) {
                LOG_CLI((BSL_META("\nBFD Scache alloc failed (rv= %d)\n"), rv));
                break;  
            }
        } else if (BCM_FAILURE(rv)) {
            LOG_CLI((BSL_META("\nBFD Scache ptr get failed (rv= %d)\n"), rv));
            break;
        }
        /* Store the  BFD num sessions  */
        sal_memcpy(bfd_scache, &bfd_info->endpoint_count, sizeof(int));
        bfd_scache += sizeof(int);

        /* Store the BFD  end point bit usage  usage */
        for (idx = 0; idx < _SHR_BITDCLSIZE (bfd_info->endpoint_count); idx++) {
            sal_memcpy(bfd_scache, &bfd_info->endpoints_in_use[idx], sizeof(SHR_BITDCL));
            bfd_scache += sizeof(SHR_BITDCL);
        }
    }while(0);

    return rv;
}

/*
 * Function:
 *     bcmi_xgs5_bfd_recover
 * Purpose:
 *     Recover BFD group configuration.
 * Parameters:
 *     unit        - (IN) BCM device number
 *     stable_size - (IN) BFD module Level2 storage size.
 *     oam_scache  - (IN) Pointer to BFD scache address pointer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_bfd_recover(int unit, int stable_size, uint8 **bfd_scache)
{
    int         rv = BCM_E_UNAVAIL;
    bfd_info_t  *bfd_info;
    bfd_endpoint_config_t *endpoint_config;
    int         idx = 0;

    /* BFD info structure for the given unit. */
    bfd_info = BFD_INFO(unit);

    /* Recover the  size to store BFD  end point config */
    for (idx = 0; idx < bfd_info->endpoint_count; idx++) {
        if (!SHR_BITGET(bfd_info->endpoints_in_use, idx)) {
                continue;
        }
        endpoint_config = BFD_ENDPOINT_CONFIG(unit, idx);
        if (endpoint_config) { 
            bcm_bfd_endpoint_info_t_init(&endpoint_config->info);
            endpoint_config->info.flags |= BCM_BFD_ENDPOINT_WITH_ID;
            endpoint_config->info.id = idx;
            rv = bcmi_xgs5_bfd_endpoint_get(unit, idx, &endpoint_config->info);
            if (BCM_FAILURE(rv)) {
                LOG_CLI((BSL_META("\nBFD endpoint(%d) get failed (rv= %d)\n"), 
                            idx, rv));
            }
        }
    } 

    /* Recover the simple Passwords used for endpoints  */
    for (idx = 0; idx < bfd_info->num_auth_sp_keys; idx++) {
        rv = bcmi_xgs5_bfd_auth_simple_password_get(unit, idx, 
                                                    &bfd_info->auth_sp[idx]); 
        if (BCM_FAILURE(rv)) {
            LOG_CLI((BSL_META("\n bfd sp auth(key=%d) get failed (rv=%d)\n"),
                    idx, rv));
        }
    }

    /* Recover the sha1 auth keyd for endpoints */
    for (idx = 0; idx < bfd_info->num_auth_sha1_keys; idx++) {
        rv = bcmi_xgs5_bfd_auth_sha1_get(unit, idx, 
                                         &bfd_info->auth_sha1[idx]); 
        if (BCM_FAILURE(rv)) {
            LOG_CLI((BSL_META("\n bfd sha1 (key=%d) get failed (rv=%d)\n"),
                    idx, rv));
        }
    }
    return rv;
}

/*
 * Function:
 *     bcmi_xgs5_bfd_reinit
 * Purpose:
 *     Reconstruct BFD module software state.
 * Parameters:
 *     unit - (IN) BCM device number
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_bfd_reinit(int unit)
{
    int         rv = BCM_E_NONE;
    int         stable_size;
    bfd_info_t  *bfd_info;
    uint8       *bfd_scache;
    int         idx = 0;
    soc_scache_handle_t scache_handle;

    /* BFD info structure for the given unit. */
     bfd_info = BFD_INFO(unit);

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    /* In case  WARM BOOT level 2 store is not configured return from here. */
    if (!SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit) && (stable_size > 0)) {

        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_BFD, 0);
        rv = _bcm_esw_scache_ptr_get(unit, scache_handle, 0, 0,
                                     &bfd_scache, BCM_WB_DEFAULT_VERSION,
                                     NULL);
        if (BCM_FAILURE(rv)) {
            LOG_CLI((BSL_META("\nBFD bfd reinit failed (rv= %d)\n"), rv));
            return rv;
        }

        /* Recover BFD endpoint count  */
        sal_memcpy(&bfd_info->endpoint_count, bfd_scache, sizeof(int));
        bfd_scache += sizeof(int);

        /* Recover the BFD  end point bit usage  usage */
        for (idx = 0; idx < _SHR_BITDCLSIZE (bfd_info->endpoint_count); idx++) {
            sal_memcpy(&bfd_info->endpoints_in_use[idx], bfd_scache, sizeof(SHR_BITDCL));
            bfd_scache += sizeof(SHR_BITDCL);
        }
        bcmi_xgs5_bfd_recover(unit, stable_size, &bfd_scache);
    }
    return BCM_E_NONE;
}

#endif  /* BCM_WARM_BOOT_SUPPORT */

#else /* INCLUDE_BFD */
int bcm_xgs5_bfd_not_empty;
#endif  /* INCLUDE_BFD */

