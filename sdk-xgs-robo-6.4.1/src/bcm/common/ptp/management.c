/*
 * $Id: management.c,v 1.1 Broadcom SDK $
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
 * File:    management.c
 *
 * Purpose: 
 *
 * Functions:
 *      _bcm_ptp_management_init
 *      _bcm_ptp_management_detach
 *      _bcm_ptp_management_stack_create
 *      _bcm_ptp_external_transport_init
 *      _bcm_ptp_internal_transport_init
 *      _bcm_ptp_management_clock_create
 *      _bcm_ptp_management_message_send
 *      _bcm_ptp_tunnel_message_to_top
 *      _bcm_ptp_tunnel_message_to_world
 *      _bcm_ptp_external_tx
 *      _bcm_ptp_external_tx_completion
 *      _bcm_ptp_internal_tx
 *      _bcm_ptp_internal_tx_completion
 *      _bcm_ptp_unicast_slave_subscribe
 *      _bcm_ptp_management_domain_get
 *      _bcm_ptp_management_domain_set
 *      _bcm_ptp_management_message_create
 *      _bcm_ptp_management_message_macaddr_set
 *      _bcm_ptp_management_message_vlantag_set
 *      _bcm_ptp_management_message_ipaddr_set
 *      _bcm_ptp_management_message_ipv4hdr_checksum_set
 *      _bcm_ptp_management_message_port_set
 *      _bcm_ptp_management_message_domain_set
 *      _bcm_ptp_management_rfc791_checksum
 *      _bcm_ptp_rfc791_checksum_add_word
 *      _bcm_ptp_construct_udp_packet
 *      _bcm_ptp_construct_signaling_msg
 *      _bcm_ptp_construct_signaling_request_tlv
 *      _bcm_ptp_construct_signaling_cancel_tlv
 *      _bcm_ptp_construct_signaling_grant_tlv
 *      _bcm_ptp_construct_signaling_ack_cancel_tlv
 *      _bcm_ptp_signaling_message_append_tlv
 */

#if defined(INCLUDE_PTP)

#ifdef BCM_HIDE_DISPATCHABLE
#undef BCM_HIDE_DISPATCHABLE
#endif

#include <shared/bsl.h>

#include <soc/defs.h>
#include <soc/drv.h>
#include <shared/bsl.h>
#include <bcm/pkt.h>
#include <bcm/tx.h>
#include <bcm/rx.h>

#include <bcm/ptp.h>
#include <bcm_int/common/ptp.h>
#include <bcm_int/ptp_common.h>
#include <bcm/error.h>

#include <bcm_int/common/rx.h>
#include <bcm_int/common/tx.h>

#if defined(BCM_PTP_INTERNAL_STACK_SUPPORT)
#include <soc/uc_msg.h>
#endif

/* Parameters and constants. */
#define PTP_MGMT_RESPONSE_TIMEOUT_US         (10000000)  /* 1 sec */
#define PTP_TUNNEL_MESSAGE_TIMEOUT_US        (100000)    /* 100 msec */

#define PTP_MGMT_RESP_ACTION_OFFSET          (46)
#define PTP_MGMT_RESP_TLV_TYPE_OFFSET        (48)
#define PTP_MGMT_RESP_TLV_LEN_OFFSET         (50)
#define PTP_MGMT_RESP_TLV_VAL_OFFSET         (52)
#define PTP_MGMT_RESP_PAYLOAD_OFFSET         (54)

#define CLOCK_DATA(unit, ptp_id, clock_num) (unit_mgmt_array[unit].stack_array[ptp_id].clock_data[clock_num])
#define STACK_DATA(unit, ptp_id)            (unit_mgmt_array[unit].stack_array[ptp_id])
#define UNIT_DATA(unit)                     (unit_mgmt_array[unit])

static const uint8 l2hdr_default[] =
{
    /* Destination MAC address (zeros placeholder). */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Source MAC address (zeros placeholder). */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8 vlanhdr_default[] =
{
    /* Tag protocol identifier (TPID). */
    0x81, 0x00,
    /*
     * Priority code point (PCP),
     * canonical form indicator (CFI),
     * VLAN identifier (VID) .
     */
    0xe0, 0x00,
};

static const uint8 ethhdr_default[] =
{
    /* Ethertype (0x0800 = IPv4, 0x86dd = IPv6). */
    0x08, 0x00,
};

static const uint8 ipv4hdr_default[] =
{
    /* Version (IPv4), header length. */
    0x45,
    /*
     * Differential service code point (DSCP),
     * Explicit congestion notification (ECN).
     */
    0x00,
    /*
     * Total length (octets).
     * NOTICE: Placeholder value.
     */
    0x00, 0x52,
    /* Identification. */
    0x00, 0x00,
    /* Flags (Do not fragment), Fragment offset. */
    0x40, 0x00,
    /* Time to live (TTL). */
    0x01,
    /* Protocol (UDP). */
    0x11,
    /* Header checksum (zeros placeholder). */
    0x00, 0x00,
    /* Source IPv4 address (zeros placeholder). */
    0x00, 0x00, 0x00, 0x00,
    /* Destination IPv4 address (zeros placeholder). */
    0x00, 0x00, 0x00, 0x00,
};

static const uint8 udphdr_default[] =
{
    /* Source port. */
    0x01, 0x40,
    /* Destination port. */
    0x01, 0x40,
    /*
     * Length (octets).
     * NOTICE: Placeholder value.
     */
    0x00, 0x00,
    /*
     * Checksum (optional).
     * NOTICE: 0x0000 indicates checksum skipped.
     */
    0x00, 0x00,
};

static const uint8 udphdr_tunnel[] =
{
    /* Source port. */
    0x01, 0x41,
    /* Destination port. */
    0x01, 0x41,
    /* Length (zeros placeholder). */
    0x00, 0x00,
    /* Checksum (optional). */
    0x00, 0x00,
};


static const uint8 ptphdr_mgmt[] =
{
    /* Message ID (management message) */
    bcmPTP_MESSAGE_TYPE_MANAGEMENT,
    /* PTP version. */
    0x02,
    /* PTP message length, TLV "V" part only */
    0x00, 0x00,
    /* PTP domain (zero default) */
    0x00,
    /* Reserved */
    0x00,
    /* PTP flags  (Unicast = true) */
    0x04, 0x00,
    /* Correction field */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Reserved */
    0x00, 0x00, 0x00, 0x00,
    /* Source Clock ID  */
    0x00, 0x10, 0x94, 0xff, 0xfe, 0x00, 0x00, 0x04,
    /* Source Port Number */
    0x00, 0x01,
    /* Sequence ID */
    0x00, 0x01,
    /* Control field (management) */
    0x04,
    /* Log inter-message interval */
    0x7f,
};


static const uint8 mgmthdr_mgmt[]=
{
    /* Target clock ID. */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* Target port ID. */
    0xff, 0xff,
    /* Starting boundary hops. */
    0x01,
    /* Boundary hops. */
    0x01,
    /* Action field. */
    0x00,
    /* Reserved. */
    0x00,
};


static const uint8 tlvhdr_mgmt[]=
{
    /* TLV type (0x0001 = Management message). */
    0x00, 0x01,
    /* TLV length (octets) (0x0002 = TLV prefix, no payload placeholder) */
    0x00, 0x02,
     /* TLV value prefix (management ID, i.e. command definition) */
    0x00, 0x00,
};

static const uint8 ptphdr_ucsig[] =
{
    /*
     * Message ID (signaling message).
     * Ref. IEEE Std. 1588-2008, Sect. 13.3.2.2.
     */
    bcmPTP_MESSAGE_TYPE_SIGNALING,
    /* PTP version. */
    0x02,
    /*
     * PTP message length, TLV "V" part only.
     * NOTICE: Placeholder based on REQUEST_UNICAST_TRANSMISSION TLV.
     */
    0x00, (PTP_UCSIG_REQUEST_TOTAL_SIZE_OCTETS - PTP_SIGHDR_START_IDX),
    /* PTP domain (zero default). */
    0x00, 0x00,
    /* PTP flags  (Unicast= true). */
    0x04, 0x00,
    /*
     * Correction field.
     * Ref. IEEE Std. 1588-2008, Chapter 13.3.2.7.
     */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Reserved. */
    0x00, 0x00, 0x00, 0x00,
    /* Source port ID. */
    0x00, 0x10, 0x94, 0xff, 0xfe, 0x00, 0x00, 0x0d,
    0x00, 0x01,
    /* Sequence ID. */
    0x00, 0x02,
    /*
     * Control field (all others).
     * Ref. IEEE Std. 1588-2008, Sect. 13.3.2.10.
     */
    0x05,
    /*
     * Log inter-message interval.
     * Ref. IEEE Std. 1588-2008, Sect. 13.3.2.11.
     */
    0x7f,
};

static const uint8 tgtport_ucsig[] =
{
    /* Signaling message target clock identity. */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* Signaling message target port. */
    0xff, 0xff,
};

static const uint8 unicast_request_tlv_ucsig[] =
{
    /*
     * TLV type. (0x0004 = REQUEST_UNICAST_TRANSMISSION placeholder).
     * Ref. IEEE Std. 1588-2008, Sect. 14.1.1.
     */
    0x00, 0x04,
    /*
     * TLV length (octets).
     * NOTICE: Placeholder based on REQUEST_UNICAST_TRANSMISSION TLV.
     */
    0x00, (PTP_UCSIG_REQUEST_TLV_SIZE_OCTETS - 4),
    /*
     * TLV value (data). Message type (0x0b = Announce message, placeholder).
     * Ref. IEEE Std. 1588-2008, Sect. 13.3.2.2.
     */
    bcmPTP_MESSAGE_TYPE_ANNOUNCE,
    /* Interval. */
    0x00,
    /* Duration. */
    0x00, 0x00, 0x00, 0x14,
    /* Address type. */
    bcmPTPUDPIPv4,
    /* Peer address. */
    0x01, 0x02, 0x03, 0x04,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    /* L2 header length (zeros placeholder). */
    0x00,
    /* L2 header (zeros placeholder). */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* Clock PTP management data. */
typedef struct _bcm_ptp_clock_mgmt_data_s {
    uint8 domain; /* PTP domain. */
} _bcm_ptp_clock_mgmt_data_t;

/* Stack PTP management data array. */
typedef struct _bcm_ptp_stack_mgmt_array_s {
    _bcm_ptp_memstate_t memstate;

    bcm_mac_t src_mac;
    bcm_mac_t dest_mac;

    bcm_ip_t src_ip;
    bcm_ip_t dest_ip;

    uint16 vlan;
    uint8 prio;

    _bcm_ptp_sem_t outgoing_available;  /* any outgoing message (management, signaling, tunneled) to TOP */

    bcm_pkt_t *mgmt_packet;
    bcm_pkt_t *tunnel_packet;
    bcm_pkt_t *ucsig_packet;

    _bcm_ptp_clock_mgmt_data_t *clock_data;
} _bcm_ptp_stack_mgmt_array_t;

/* Unit PTP management data array(s). */
typedef struct _bcm_ptp_unit_mgmt_array_s {
    _bcm_ptp_memstate_t memstate;
    _bcm_ptp_stack_mgmt_array_t *stack_array;
} _bcm_ptp_unit_mgmt_array_t;

static const _bcm_ptp_clock_mgmt_data_t mgmt_default = { 0 };

static _bcm_ptp_unit_mgmt_array_t unit_mgmt_array[BCM_MAX_NUM_UNITS];

static int _bcm_ptp_management_message_create(
    uint8 *message);

/* static int _bcm_ptp_management_message_make( */
/*     uint8 *message,  */
/*     uint8 action,  */
/*     uint16 cmd,  */
/*     uint8 *payload,  */
/*     int payload_len); */

/* static int _bcm_ptp_management_message_header_metadata_set( */
/*     uint8 *message); */

static int _bcm_ptp_management_message_macaddr_set(
    uint8 *message,
    bcm_mac_t *src_mac,
    bcm_mac_t *dest_mac);

static int _bcm_ptp_management_message_vlantag_set(
    uint8 *message,
    uint16 vlan_tag,
    uint8 prio);

static int _bcm_ptp_management_message_ipaddr_set(
    uint8 *message,
    bcm_ip_t src_ip,
    bcm_ip_t dest_ip);

static int _bcm_ptp_management_message_ipv4hdr_checksum_set(
    uint8 *message);

static int _bcm_ptp_management_message_port_set(
    uint8 *message,
    uint16 src_port,
    uint16 dest_port);

/* static int _bcm_ptp_management_message_target_set( */
/*     uint8 *message,  */
/*     const bcm_ptp_clock_identity_t clock_identity,  */
/*     uint16 port_num); */

/* static int _bcm_ptp_management_message_length_get( */
/*     uint8 *message); */

#if 0 /* Unused. */
static int _bcm_ptp_management_message_domain_get(
    uint8 *message,
    uint8 *domain);
#endif /* Unused. */

static uint16 _bcm_ptp_management_rfc791_checksum(
    uint8* packet,
    int packet_len);

static uint16 _bcm_ptp_rfc791_checksum_add_word(
    uint16 prev_checksum,
    uint16 add_word);

/*
 * Function:
 *      _bcm_ptp_management_init
 * Purpose:
 *      Initialize the PTP management framework of a unit.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_management_init(
    int unit)
{
    int rv = BCM_E_UNAVAIL;

    _bcm_ptp_stack_mgmt_array_t *stack_p;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    
    stack_p = sal_alloc(PTP_MAX_STACKS_PER_UNIT*
                        sizeof(_bcm_ptp_stack_mgmt_array_t),"Unit MGMT arrays");

    if (!stack_p) {
        unit_mgmt_array[unit].memstate = PTP_MEMSTATE_FAILURE;
        return BCM_E_MEMORY;
    }

    unit_mgmt_array[unit].stack_array = stack_p;
    unit_mgmt_array[unit].memstate = PTP_MEMSTATE_INITIALIZED;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_management_detach
 * Purpose:
 *      Shut down the PTP management framework of a unit.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_management_detach(
    int unit)
{
    if(unit_mgmt_array[unit].stack_array) {
        sal_free(unit_mgmt_array[unit].stack_array);
        unit_mgmt_array[unit].stack_array = NULL;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_management_stack_create
 * Purpose:
 *      Create the PTP management data array of a PTP stack
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      src_mac    - (IN) Source MAC address.
 *      dest_mac   - (IN) Destination (PTP stack) MAC address.
 *      src_ip     - (IN) Source IPv4 address.
 *      dest_ip    - (IN) Destination (PTP stack) IPv4 address.
 *      vlan       - (IN) VLAN.
 *      prio       - (IN) VLAN priority.
 *      top_bitmap - (IN) ToP bitmap.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_management_stack_create(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    bcm_mac_t *src_mac,
    bcm_mac_t *dest_mac,
    bcm_ip_t src_ip,
    bcm_ip_t dest_ip,
    uint16 vlan,
    uint8 prio,
    bcm_pbmp_t top_bitmap)
{
    int rv = BCM_E_UNAVAIL;
    _bcm_ptp_info_t *ptp_info_p;
    _bcm_ptp_stack_info_t *stack_p;

    _bcm_ptp_clock_mgmt_data_t *data_p;

    SET_PTP_INFO;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (unit_mgmt_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) {
        return BCM_E_UNAVAIL;
    }

    stack_p = &ptp_info_p->stack_info[ptp_id];

    
    data_p = sal_alloc(PTP_MAX_CLOCK_INSTANCES * sizeof(_bcm_ptp_clock_mgmt_data_t), "PTP stack MGMT array");

    if (!data_p) {
        STACK_DATA(unit, ptp_id).memstate = PTP_MEMSTATE_FAILURE;
        return BCM_E_MEMORY;
    }

    STACK_DATA(unit, ptp_id).clock_data = data_p;
    STACK_DATA(unit, ptp_id).memstate = PTP_MEMSTATE_INITIALIZED;

    STACK_DATA(unit, ptp_id).outgoing_available = _bcm_ptp_sem_create("BCM PTP out", sal_sem_BINARY, 0);
    rv = _bcm_ptp_sem_give(STACK_DATA(unit, ptp_id).outgoing_available);
    if (BCM_FAILURE(rv)) {
        PTP_ERROR_FUNC("_bcm_ptp_sem_give()");
        return rv;
    }

    rv = stack_p->transport_init(unit, ptp_id, src_mac, dest_mac, src_ip, dest_ip, vlan, prio, top_bitmap);

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_external_transport_init
 * Purpose:
 *      Initialize message transport to an external ToP
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      src_mac    - (IN) Source MAC address.
 *      dest_mac   - (IN) Destination (PTP stack) MAC address.
 *      src_ip     - (IN) Source IPv4 address.
 *      dest_ip    - (IN) Destination (PTP stack) IPv4 address.
 *      vlan       - (IN) VLAN.
 *      prio       - (IN) VLAN priority.
 *      top_bitmap - (IN) ToP bitmap.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int _bcm_ptp_external_transport_init(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    bcm_mac_t *src_mac,
    bcm_mac_t *dest_mac,
    bcm_ip_t src_ip,
    bcm_ip_t dest_ip,
    uint16 vlan,
    uint8 prio,
    bcm_pbmp_t top_bitmap)
{
    bcm_pkt_t *mgmt_packet;
    bcm_pkt_t *tunnel_packet;
    bcm_pkt_t *ucsig_packet;
    int i = 0;
    int rv = BCM_E_UNAVAIL;

    sal_memcpy(STACK_DATA(unit, ptp_id).src_mac, src_mac, sizeof(bcm_mac_t));

    sal_memcpy(STACK_DATA(unit, ptp_id).dest_mac, dest_mac, sizeof(bcm_mac_t));

    STACK_DATA(unit, ptp_id).src_ip = src_ip;
    STACK_DATA(unit, ptp_id).dest_ip = dest_ip;

    STACK_DATA(unit, ptp_id).vlan = vlan;
    STACK_DATA(unit, ptp_id).prio = prio;

    rv = bcm_pkt_alloc(unit, 1600, BCM_TX_CRC_REGEN, &STACK_DATA(unit, ptp_id).mgmt_packet);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    rv = bcm_pkt_alloc(unit, 1600, BCM_TX_CRC_REGEN, &STACK_DATA(unit, ptp_id).tunnel_packet);
    if (BCM_FAILURE(rv)) {
        bcm_pkt_free(unit, STACK_DATA(unit, ptp_id).mgmt_packet);
        return rv;
    }

    rv = bcm_pkt_alloc(unit, 1600, BCM_TX_CRC_REGEN, &STACK_DATA(unit, ptp_id).ucsig_packet);
    if (BCM_FAILURE(rv)) {
        bcm_pkt_free(unit, STACK_DATA(unit, ptp_id).mgmt_packet);
        bcm_pkt_free(unit, STACK_DATA(unit, ptp_id).tunnel_packet);
        return rv;
    }

    mgmt_packet = STACK_DATA(unit, ptp_id).mgmt_packet;
    tunnel_packet = STACK_DATA(unit, ptp_id).tunnel_packet;
    ucsig_packet = STACK_DATA(unit, ptp_id).ucsig_packet;

    BCM_PBMP_ASSIGN(mgmt_packet->tx_pbmp, top_bitmap);
    BCM_PBMP_ASSIGN(tunnel_packet->tx_pbmp, top_bitmap);
    BCM_PBMP_ASSIGN(ucsig_packet->tx_pbmp, top_bitmap);

    _bcm_ptp_management_message_create(BCM_PKT_IEEE(mgmt_packet));
    _bcm_ptp_management_message_macaddr_set(BCM_PKT_IEEE(mgmt_packet),
        src_mac, dest_mac);
    _bcm_ptp_management_message_ipaddr_set(BCM_PKT_IEEE(mgmt_packet),
        src_ip, dest_ip);
    _bcm_ptp_management_message_vlantag_set(BCM_PKT_IEEE(mgmt_packet),
        vlan, prio);

    /* Make template (default) tunnel message packet. */
    i = 0;
    sal_memcpy(BCM_PKT_IEEE(tunnel_packet), l2hdr_default,
               sizeof(l2hdr_default));
    i += sizeof(l2hdr_default);

    sal_memcpy(BCM_PKT_IEEE(tunnel_packet) + i, vlanhdr_default,
               sizeof(vlanhdr_default));
    i += sizeof(vlanhdr_default);

    sal_memcpy(BCM_PKT_IEEE(tunnel_packet) + i, ethhdr_default,
               sizeof(ethhdr_default));
    i += sizeof(ethhdr_default);

    sal_memcpy(BCM_PKT_IEEE(tunnel_packet) + i, ipv4hdr_default,
               sizeof(ipv4hdr_default));
    i += sizeof(ipv4hdr_default);

    sal_memcpy(BCM_PKT_IEEE(tunnel_packet) + i, udphdr_tunnel,
               sizeof(udphdr_tunnel));

    _bcm_ptp_management_message_macaddr_set(BCM_PKT_IEEE(tunnel_packet),
        src_mac, dest_mac);
    _bcm_ptp_management_message_ipaddr_set(BCM_PKT_IEEE(tunnel_packet),
        src_ip, dest_ip);
    _bcm_ptp_management_message_vlantag_set(BCM_PKT_IEEE(tunnel_packet),
        vlan, prio);

    /* Make template (default) unicast transmission signaling packet. */
    i = 0;
    sal_memcpy(BCM_PKT_IEEE(ucsig_packet), l2hdr_default,
               sizeof(l2hdr_default));
    i += sizeof(l2hdr_default);

    sal_memcpy(BCM_PKT_IEEE(ucsig_packet) + i, vlanhdr_default,
               sizeof(vlanhdr_default));
    i += PTP_VLANHDR_SIZE_OCTETS;

    sal_memcpy(BCM_PKT_IEEE(ucsig_packet) + i, ethhdr_default,
               sizeof(ethhdr_default));
    i += sizeof(ethhdr_default);

    sal_memcpy(BCM_PKT_IEEE(ucsig_packet) + i, ipv4hdr_default,
               sizeof(ipv4hdr_default));
    i += sizeof(ipv4hdr_default);

    sal_memcpy(BCM_PKT_IEEE(ucsig_packet) + i, udphdr_default,
               sizeof(udphdr_default));
    i += sizeof(udphdr_default);

    _bcm_ptp_management_message_macaddr_set(BCM_PKT_IEEE(ucsig_packet),
        src_mac, dest_mac);
    _bcm_ptp_management_message_ipaddr_set(BCM_PKT_IEEE(ucsig_packet),
        src_ip, dest_ip);
    _bcm_ptp_management_message_vlantag_set(BCM_PKT_IEEE(ucsig_packet),
        vlan, prio);

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_internal_transport_init
 * Purpose:
 *      Initialize message transport to an internal ToP core
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      src_mac    - (IN) Source MAC address.
 *      dest_mac   - (IN) Destination (PTP stack) MAC address.
 *      src_ip     - (IN) Source IPv4 address.
 *      dest_ip    - (IN) Destination (PTP stack) IPv4 address.
 *      vlan       - (IN) VLAN.
 *      prio       - (IN) VLAN priority.
 *      top_bitmap - (IN) ToP bitmap.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int _bcm_ptp_internal_transport_init(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    bcm_mac_t *src_mac,
    bcm_mac_t *dest_mac,
    bcm_ip_t src_ip,
    bcm_ip_t dest_ip,
    uint16 vlan,
    uint8 prio,
    bcm_pbmp_t top_bitmap)
{
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_management_clock_create
 * Purpose:
 *      Create the PTP management data of a PTP clock.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_management_clock_create(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num)
{
    int rv = BCM_E_NONE;

    rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    if ((unit_mgmt_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (STACK_DATA(unit, ptp_id).memstate != PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    CLOCK_DATA(unit, ptp_id, clock_num) = mgmt_default;

    return rv;
}


/*
 * Function:
 *      _bcm_ptp_management_message_send
 * Purpose:
 *      Send a PTP management message.
 * Parameters:
 *      unit            - (IN) Unit number.
 *      ptp_id          - (IN) PTP stack ID.
 *      clock_num       - (IN) PTP clock number.
 *      dest_clock_port - (IN) Destination PTP clock port.
 *      action          - (IN) PTP management action (actionField).
 *      cmd             - (IN) PTP command (managementId).
 *      payload         - (IN) PTP management message payload.
 *      payload_len     - (IN) PTP management message payload size (octets).
 *      resp            - (IN/OUT) PTP management message response.
 *      resp_len        - (OUT) PTP management message response size (octets).
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_management_message_send(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    const bcm_ptp_port_identity_t* dest_clock_port,
    uint8 action,
    uint16 cmd,
    uint8* payload,
    int payload_len,
    uint8* resp,
    int* resp_len)
{
    int rv = BCM_E_UNAVAIL;
    int rvbody = BCM_E_FAIL;

    uint8 *response_data;
    int response_len;
    int response_payload_len;

    uint8 resp_action;

    uint16 tlvtype;
    uint16 tlvlen;

    int tlv_len = 2 + payload_len;
    int message_len = 48 + 4 + tlv_len;

    uint8 message[1500];

    uint16 mgmt_id;

    int max_resp_len = (resp_len) ? *resp_len : 0;

    _bcm_ptp_info_t *ptp_info_p = &_bcm_common_ptp_info[unit];
    _bcm_ptp_stack_info_t *stack_p = &ptp_info_p->stack_info[ptp_id];

/*    ptp_printf("_bcm_ptp_management_message_send(): ENTER | %s | @ t = %u |\n", */
/*               bcm_errmsg(BCM_E_NONE), (unsigned)sal_time_usecs());             */

    if (resp_len) {
        *resp_len = 0;
    }

    /* Sanity check running system */
    rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    if ((unit_mgmt_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (STACK_DATA(unit, ptp_id).memstate != PTP_MEMSTATE_INITIALIZED)) {
        /* Verify initialization */
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(),
                     "_bcm_ptp_management_message_send(): memory state"));
        return BCM_E_UNAVAIL;
    }

    rv = _bcm_ptp_sem_take(STACK_DATA(unit, ptp_id).outgoing_available, PTP_MGMT_RESPONSE_TIMEOUT_US);
    if (BCM_FAILURE(rv)) {
        PTP_ERROR_FUNC("_bcm_ptp_sem_take()");
        return rv;
    }

/*     LOG_CLI((BSL_META_U(unit,
                           "mgmt_send(len:%d)\n"), payload_len)); */
/*     _bcm_ptp_dump_hex(payload, payload_len, 0); */

    /* Init the mgmt message with presets */
    sal_memset(message, 0, sizeof (message));

    sal_memcpy(message,
               ptphdr_mgmt, sizeof(ptphdr_mgmt));
    sal_memcpy(message + PTP_PTPHDR_SIZE_OCTETS,
               mgmthdr_mgmt, sizeof(mgmthdr_mgmt));
    sal_memcpy(message + PTP_PTPHDR_SIZE_OCTETS + PTP_MGMTHDR_SIZE_OCTETS,
               tlvhdr_mgmt, sizeof(tlvhdr_mgmt));

    /* Make the PTP Packet header */
    _bcm_ptp_uint16_write(message + PTP_PTPHDR_MSGLEN_OFFSET_OCTETS, message_len);

    message[PTP_PTPHDR_DOMAIN_OFFSET_OCTETS] = CLOCK_DATA(unit, ptp_id, clock_num).domain;

    /* Make the Management header */
    sal_memcpy(message + PTP_PTPHDR_SIZE_OCTETS,
               (uint8*)dest_clock_port->clock_identity, sizeof(bcm_ptp_clock_identity_t));

    _bcm_ptp_uint16_write(message + PTP_PTPHDR_SIZE_OCTETS + BCM_PTP_CLOCK_EUID_IEEE1588_SIZE,
                          dest_clock_port->port_number);

    message[PTP_PTPHDR_SIZE_OCTETS + sizeof(bcm_ptp_port_identity_t) + 2] = action;

    /* Make the TLV header */
    _bcm_ptp_uint16_write(message + PTP_PTPHDR_SIZE_OCTETS + PTP_MGMTHDR_SIZE_OCTETS + 2, tlv_len);

    _bcm_ptp_uint16_write(message + PTP_PTPHDR_SIZE_OCTETS + PTP_MGMTHDR_SIZE_OCTETS + 2 + 2, cmd);

    /* Make the payload */
    sal_memcpy(message + PTP_PTPHDR_SIZE_OCTETS + PTP_MGMTHDR_SIZE_OCTETS + PTP_TLVHDR_SIZE_OCTETS,
               payload, payload_len);

/*   LOG_CLI((BSL_META_U(unit,
                         "mgmt_send_tx(len:%d)\n"), message_len)); */
/*     _bcm_ptp_dump_hex(message, message_len, 0); */

    rv = stack_p->tx(unit, ptp_id, clock_num, PTP_MESSAGE_TO_TOP, 0, 0, message, message_len);

    if (rv != BCM_E_NONE) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Tx failed"));
        goto release_mgmt_lock;
    }

    /*
     * Get rx buffer, either from rx callback or from cmicm wait task
     * NOTICE: This call will return an rx response buffer that we will need to
     *         release by notifying the Rx section
     */
    rv = _bcm_ptp_rx_response_get(unit, ptp_id, clock_num, PTP_MGMT_RESPONSE_TIMEOUT_US,
                                  &response_data, &response_len);
    if (BCM_FAILURE(rv)) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "No Response"));
        goto release_mgmt_lock;
    }

    if (!response_data) {
        LOG_ERROR(BSL_LS_BCM_COMMON,
                  (BSL_META_U(unit,
                              "Unexpected NULL response\n")));
    }

    rv = BCM_E_FAIL;
    resp_action = (response_data[PTP_MGMT_RESP_ACTION_OFFSET] & 0x0f);

    if (((action == PTP_MGMTMSG_SET) || (action == PTP_MGMTMSG_GET)) && (resp_action != PTP_MGMTMSG_RESP)) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Bad response action"));
        goto dispose_of_resp;
    }

    if ((action == PTP_MGMTMSG_CMD) && (resp_action != PTP_MGMTMSG_ACK) ) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Bad cmd response action"));
        goto dispose_of_resp;
    }

    tlvtype = _bcm_ptp_uint16_read(response_data + PTP_MGMT_RESP_TLV_TYPE_OFFSET);

    if (tlvtype == bcmPTPTlvTypeManagementErrorStatus) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Error response: 0x%04x\n"),
                     _bcm_ptp_uint16_read(response_data + PTP_MGMT_RESP_TLV_VAL_OFFSET)));
        goto dispose_of_resp;
    }

    tlvlen = _bcm_ptp_uint16_read(response_data + PTP_MGMT_RESP_TLV_LEN_OFFSET);

    if (tlvlen + PTP_MGMT_RESP_TLV_VAL_OFFSET > response_len) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Bad TLV len"));
        goto dispose_of_resp;
    }

    mgmt_id = _bcm_ptp_uint16_read(response_data + PTP_MGMT_RESP_TLV_VAL_OFFSET);

    if (mgmt_id != cmd) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Bad mgmt ID: %04x vs %04x\n"), mgmt_id, cmd));
        goto dispose_of_resp;
    }

    if (resp && resp_len) {
        response_payload_len = tlvlen - 2;
        if (response_payload_len > max_resp_len) {
            response_payload_len = max_resp_len;
        }

        *resp_len = response_payload_len;
        sal_memcpy(resp, response_data + PTP_MGMT_RESP_PAYLOAD_OFFSET, response_payload_len);
    }

    rv = BCM_E_NONE;

dispose_of_resp:
    stack_p->rx_free(unit, ptp_id, response_data);

release_mgmt_lock:
    rvbody = rv;
    if (BCM_FAILURE(rv = _bcm_ptp_sem_give(STACK_DATA(unit, ptp_id).outgoing_available))) {
        PTP_ERROR_FUNC("_bcm_ptp_sem_give()");
    }

/*    ptp_printf("_bcm_ptp_management_message_send(): EXIT  | %s | @ t = %u |\n", */
/*               bcm_errmsg(rvbody), (unsigned)sal_time_usecs());                 */

    return rvbody ? rvbody : rv;
}


/*
 * Function:
 *      _bcm_ptp_tunnel_message_to_top
 * Purpose:
 *      Tunnel a message to ToP processor.
 * Parameters:
 *      unit        - (IN) Unit number.
 *      ptp_id      - (IN) PTP stack ID.
 *      clock_index - (IN) PTP clock index
 *      port_index  - (IN) PTP port index
 *      payload_len - (IN) Tunnel message payload size (octets).
 *      payload     - (IN) Tunnel message payload.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_tunnel_message_to_top(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_index,
    int port_index,
    unsigned payload_len,
    uint8 *payload)
{
    _bcm_ptp_info_t *ptp_info_p = &_bcm_common_ptp_info[unit];
    _bcm_ptp_stack_info_t *stack_p = &ptp_info_p->stack_info[ptp_id];

    int hdr_len;
    uint8 hdr[2];

    int rv = BCM_E_NONE;
    int rvbody = BCM_E_FAIL;

/*    ptp_printf("_bcm_ptp_tunnel_message_to_top(): ENTER | %s | @ t = %u |\n", */
/*               bcm_errmsg(BCM_E_NONE), (unsigned)sal_time_usecs());           */

    if ((unit_mgmt_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (STACK_DATA(unit, ptp_id).memstate != PTP_MEMSTATE_INITIALIZED)) {
        /* Verify initialization */
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(),
                     "_bcm_ptp_tunnel_message_to_top(): memory state"));
        return BCM_E_UNAVAIL;
    }

    rv = _bcm_ptp_sem_take(STACK_DATA(unit, ptp_id).outgoing_available, PTP_TUNNEL_MESSAGE_TIMEOUT_US);
    if (BCM_FAILURE(rv)) {
        PTP_ERROR_FUNC("_bcm_ptp_sem_take()");
        return rv;
    }

    _bcm_ptp_uint16_write(hdr, port_index);
    hdr_len = 2;

    rv = stack_p->tx(unit, ptp_id, clock_index, PTP_TUNNEL_TO_TOP, hdr, hdr_len, payload, payload_len);

    if (rv != BCM_E_NONE) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Tx failed"));
        goto release_tunnel_lock;
    }

    rv = stack_p->tx_completion(unit, ptp_id);

release_tunnel_lock:
    rvbody = rv;
    if (BCM_FAILURE(rv = _bcm_ptp_sem_give(STACK_DATA(unit, ptp_id).outgoing_available))) {
        PTP_ERROR_FUNC("_bcm_ptp_sem_give()");
    }

/*    ptp_printf("_bcm_ptp_tunnel_message_to_top(): EXIT  | %s | @ t = %u |\n", */
/*               bcm_errmsg(rvbody), (unsigned)sal_time_usecs());               */

    return rvbody ? rvbody : rv;
}


/*
 * Function:
 *      _bcm_ptp_tunnel_message_to_world
 * Purpose:
 *      Tunnel a message to "world".
 * Parameters:
 *      unit        - (IN) Unit number.
 *      ptp_id      - (IN) PTP stack ID.
 *      clock_index - (IN) PTP clock index.
 *      payload_len - (IN) Tunnel message payload size (octets).
 *      payload     - (IN) Tunnel message payload.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_tunnel_message_to_world(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_index,
    unsigned payload_len,
    uint8 *payload)
{
    _bcm_ptp_info_t *ptp_info_p = &_bcm_common_ptp_info[unit];
    _bcm_ptp_stack_info_t *stack_p = &ptp_info_p->stack_info[ptp_id];

    int rv = BCM_E_NONE;
    int rvbody = BCM_E_FAIL;

/*    ptp_printf("_bcm_ptp_tunnel_message_to_world(): ENTER | %s | @ t = %u |\n", */
/*               bcm_errmsg(BCM_E_NONE), (unsigned)sal_time_usecs());             */

    if ((unit_mgmt_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (STACK_DATA(unit, ptp_id).memstate != PTP_MEMSTATE_INITIALIZED)) {
        /* Verify initialization */
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(),
                     "_bcm_ptp_tunnel_message_to_world(): memory state"));
        return BCM_E_UNAVAIL;
    }

    rv = _bcm_ptp_sem_take(STACK_DATA(unit, ptp_id).outgoing_available, PTP_TUNNEL_MESSAGE_TIMEOUT_US);
    if (BCM_FAILURE(rv)) {
        PTP_ERROR_FUNC("_bcm_ptp_sem_take()");
        return rv;
    }

    rv = stack_p->tx(unit, ptp_id, clock_index, PTP_TUNNEL_FROM_TOP, 0, 0, payload, payload_len);

    if (rv != BCM_E_NONE) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Tx failed"));
        goto release_tunnel_lock;
    }

    rv = stack_p->tx_completion(unit, ptp_id);

release_tunnel_lock:
    rvbody = rv;
    if (BCM_FAILURE(rv = _bcm_ptp_sem_give(STACK_DATA(unit, ptp_id).outgoing_available))) {
        PTP_ERROR_FUNC("_bcm_ptp_sem_give()");
    }

/*    ptp_printf("_bcm_ptp_tunnel_message_to_world(): EXIT  | %s | @ t = %u |\n", */
/*               bcm_errmsg(rvbody), (unsigned)sal_time_usecs());                 */

    return rvbody ? rvbody : rv;
}



int
_bcm_ptp_external_tx(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    _bcm_ptp_transport_type_t transport,
    uint8 *hdr,
    int hdr_len,
    uint8 *message,
    int message_len)
{
    int rv = BCM_E_NONE;

/*     LOG_CLI((BSL_META_U(unit,
                           "external_tx(type:%d len:%d)\n"), (int)transport, message_len)); */
/*     _bcm_ptp_dump_hex(message, message_len, 0); */

    /* Send the right kind of message */
    switch (transport) {
    case PTP_MESSAGE_TO_TOP:
    {
        /* WFT: probably should just allocate packet here rather than having pre-allocated... */
        _bcm_ptp_stack_mgmt_array_t *mgmt = &STACK_DATA(unit, ptp_id);
        bcm_pkt_t *mgmt_packet = mgmt->mgmt_packet;
        uint8 *cursor = BCM_PKT_IEEE(mgmt_packet);
        int prio = 0;

        /* L2 Header MACs*/
        sal_memcpy(cursor, mgmt->dest_mac, sizeof(bcm_mac_t));  cursor += sizeof(bcm_mac_t);
        sal_memcpy(cursor, mgmt->src_mac, sizeof(bcm_mac_t));  cursor += sizeof(bcm_mac_t);

        /*   VLAN */
        *cursor++ = 0x81; *cursor++ = 0x00;
        *cursor++ = (prio << 5) | (0x0f & (mgmt->vlan >> 8));
        *cursor++ = (mgmt->vlan);

        /*   Ethertype: IPv4 */
        *cursor++ = 0x08;
        *cursor++ = 0x00;

        /* IPv4 Header */
        *cursor++ = 0x45;  /*type & header length */
        *cursor++ = 0x00;

        /*   IPv4 total length */
        _bcm_ptp_uint16_write(cursor, 20 + 8 + hdr_len + message_len); cursor += 2;

        *cursor++ = 0;
        *cursor++ = 0;

        *cursor++ = 0x40;  /* flags */
        *cursor++ = 0;

        *cursor++ = 1;     /* TTL */
        *cursor++ = 0x11;  /* protocol = UDP */

        *cursor++ = 0;     /* header checksum (zeros for computation) */
        *cursor++ = 0;

        _bcm_ptp_uint32_write(cursor, mgmt->src_ip);  cursor += 4;
        _bcm_ptp_uint32_write(cursor, mgmt->dest_ip);  cursor += 4;

        /* UDP Header */
        _bcm_ptp_uint16_write(cursor, 0x140);  cursor += 2;  /* source port */
        _bcm_ptp_uint16_write(cursor, 0x140);  cursor += 2;  /* dest port */

        _bcm_ptp_uint16_write(cursor, 8 + hdr_len + message_len);  cursor += 2;  /* UDP length */

        _bcm_ptp_uint16_write(cursor, 0);  cursor += 2;  /* UDP Checksum (optional) */

        /* Update IPv4 header checksum. */
        rv = _bcm_ptp_management_message_ipv4hdr_checksum_set(BCM_PKT_IEEE(mgmt_packet));
        

        sal_memcpy(cursor, hdr, hdr_len);  cursor += hdr_len;
        sal_memcpy(cursor, message, message_len);  cursor += message_len;

        /*
         * Set packet length.
         * NOTICE: Includes 4 for the CRC appended to the packet
         */
        mgmt_packet->pkt_data[0].len = (cursor - BCM_PKT_IEEE(mgmt_packet)) + 4;

/*      LOG_CLI((BSL_META_U(unit,
                            "packet(len:%d)\n"), mgmt_packet->pkt_data[0].len)); */
/*         _bcm_ptp_dump_hex(BCM_PKT_IEEE(mgmt_packet), mgmt_packet->pkt_data[0].len, 0); */

        rv = bcm_tx(unit, mgmt_packet, NULL);

        break;
    }

    case PTP_TUNNEL_TO_TOP:
    {
        int rv = BCM_E_UNAVAIL;

        bcm_pkt_t *tunnel_packet;

        uint16 udp_length = hdr_len + message_len + PTP_UDPHDR_SIZE_OCTETS;
        uint16 ip_length  = udp_length + PTP_IPV4HDR_SIZE_OCTETS;
        uint16 packet_length = ip_length + PTP_IPV4HDR_START_IDX;
        int i = 0;

        tunnel_packet = STACK_DATA(unit, ptp_id).tunnel_packet;

        i = PTP_PTPHDR_START_IDX;
        sal_memcpy(BCM_PKT_IEEE(tunnel_packet) + i, hdr, hdr_len);
        sal_memcpy(BCM_PKT_IEEE(tunnel_packet) + i + hdr_len, message, message_len);

        i = PTP_UDPHDR_START_IDX + PTP_UDPHDR_MSGLEN_OFFSET_OCTETS;
        _bcm_ptp_uint16_write(BCM_PKT_IEEE(tunnel_packet) + i, udp_length);

        i = PTP_IPV4HDR_START_IDX + PTP_IPV4HDR_MSGLEN_OFFSET_OCTETS;
        _bcm_ptp_uint16_write(BCM_PKT_IEEE(tunnel_packet) + i, ip_length);

        _bcm_ptp_management_message_port_set(BCM_PKT_IEEE(tunnel_packet), 0x0141, 0x0141);

        /* Set IPv4 header checksum. */
        if (BCM_FAILURE(rv = _bcm_ptp_management_message_ipv4hdr_checksum_set(
                            BCM_PKT_IEEE(tunnel_packet)))) {
            return rv;
        }

        /* Set packet size. */
        tunnel_packet->pkt_data[0].len = packet_length + 4;

        rv = bcm_tx(unit, tunnel_packet, NULL);

        break;
    }

    case PTP_TUNNEL_FROM_TOP:
    {
        int rv = BCM_E_UNAVAIL;

        bcm_pkt_t *tunnel_packet;

        uint16 udp_length = hdr_len + message_len + PTP_UDPHDR_SIZE_OCTETS;
        uint16 ip_length  = udp_length + PTP_IPV4HDR_SIZE_OCTETS;
        uint16 packet_length = ip_length + PTP_IPV4HDR_START_IDX;
        int i = 0;

        tunnel_packet = STACK_DATA(unit, ptp_id).tunnel_packet;

        i = PTP_PTPHDR_START_IDX;
        sal_memcpy(BCM_PKT_IEEE(tunnel_packet) + i, hdr, hdr_len);
        sal_memcpy(BCM_PKT_IEEE(tunnel_packet) + i + hdr_len, message, message_len);

        i = PTP_UDPHDR_START_IDX + PTP_UDPHDR_MSGLEN_OFFSET_OCTETS;
        _bcm_ptp_uint16_write(BCM_PKT_IEEE(tunnel_packet) + i, udp_length);

        i = PTP_IPV4HDR_START_IDX + PTP_IPV4HDR_MSGLEN_OFFSET_OCTETS;
        _bcm_ptp_uint16_write(BCM_PKT_IEEE(tunnel_packet) + i, ip_length);

        _bcm_ptp_management_message_port_set(BCM_PKT_IEEE(tunnel_packet), 0x0143, 0x0143);

        /* Set IPv4 header checksum. */
        rv = _bcm_ptp_management_message_ipv4hdr_checksum_set(BCM_PKT_IEEE(tunnel_packet));
        if (BCM_FAILURE(rv)) {
            PTP_ERROR_FUNC("_bcm_ptp_management_message_ipv4hdr_checksum_set()");
            return rv;
        }

        /* Set packet size. */
        tunnel_packet->pkt_data[0].len = packet_length + 4;

        rv = bcm_tx(unit, tunnel_packet, NULL);

        break;
    }

    default:
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Unknown transport type"));
    }

    return rv;
}


int
_bcm_ptp_external_tx_completion(
    int unit,
    bcm_ptp_stack_id_t ptp_id)
{
    /* message was already sent as a packet via switch, so it is already gone */
    return BCM_E_NONE;
}

#if defined(BCM_PTP_INTERNAL_STACK_SUPPORT)
int
_bcm_ptp_internal_tx(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    _bcm_ptp_transport_type_t transport,
    uint8 *hdr,
    int hdr_len,
    uint8 *message,
    int message_len)

{
    _bcm_ptp_info_t *ptp_info_p = &_bcm_common_ptp_info[unit];
    _bcm_ptp_stack_info_t *stack_p = &ptp_info_p->stack_info[ptp_id];

    int rv = BCM_E_NONE;

    mos_msg_data_t uc_msg;
    uint8 *msg_data;

    int wait_iter = 0;

    /* mbox 0 reserved for cmd/resp */
    msg_data = (uint8 *)stack_p->int_state.mboxes->mbox[0].data;

    if (GET_MBOX_STATUS(stack_p, 0) != MBOX_STATUS_EMPTY) {
        LOG_ERROR(BSL_LS_BCM_COMMON,
                  (BSL_META_U(unit,
                              "PTP mbox contention\n")));

        /* wait for to-TOP buffer to be free */
        while (GET_MBOX_STATUS(stack_p, 0) != MBOX_STATUS_EMPTY && wait_iter < 100000) {
            ++wait_iter;
            sal_usleep(1);
        }

        if (GET_MBOX_STATUS(stack_p, 0) != MBOX_STATUS_EMPTY) {
            LOG_CLI((BSL_META_U(unit,
                                "TOP message buffer in use on Tx, re-pinging\n")));
            rv = soc_cmic_uc_msg_send(unit, stack_p->int_state.core_num, &uc_msg, 1000000);
            return BCM_E_FAIL;
        } else if (wait_iter > 0) {
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "Wait to send outgoing to ToP: %d\n"), wait_iter));
        }
    }

    sal_memcpy(msg_data, hdr, hdr_len);
    sal_memcpy(msg_data + hdr_len, message, message_len);

    stack_p->int_state.mboxes->mbox[0].data_len = soc_htonl(hdr_len + message_len);
    stack_p->int_state.mboxes->mbox[0].clock_num = soc_htonl(clock_num);

    /* Flush entire mbox structure so firmware can DMA it (before setting status, to avoid race) */
    soc_cm_sflush(unit, (void*)&stack_p->int_state.mboxes->mbox[0],
                  sizeof(stack_p->int_state.mboxes->mbox[0]));

    /* Send the right kind of message */
    switch (transport) {
    case PTP_MESSAGE_TO_TOP:
        SET_MBOX_STATUS(stack_p, 0, MBOX_STATUS_ATTN_TOP_CMD);
        break;
    case PTP_TUNNEL_TO_TOP:
        SET_MBOX_STATUS(stack_p, 0, MBOX_STATUS_ATTN_TOP_TUNNEL_TO_TOP);
        break;
    case PTP_TUNNEL_FROM_TOP:
        SET_MBOX_STATUS(stack_p, 0, MBOX_STATUS_ATTN_TOP_TUNNEL_EXTERNAL);
        break;
    default:
        LOG_ERROR(BSL_LS_BCM_COMMON,
                  (BSL_META_U(unit,
                              "%s() failed %s\n"), FUNCTION_NAME(), "Unknown transport type"));
    }

    sal_memset(&uc_msg, 0, sizeof(uc_msg));
    uc_msg.s.mclass = MOS_MSG_CLASS_1588;
    uc_msg.s.subclass = MOS_MSG_SUBCLASS_MBOX_CMDRESP;
    uc_msg.s.len = hdr_len + message_len;
    uc_msg.s.data = 0;

    rv = soc_cmic_uc_msg_send(unit, stack_p->int_state.core_num, &uc_msg, 1000000);

    return rv;
}

int
_bcm_ptp_internal_tx_completion(
    int unit,
    bcm_ptp_stack_id_t ptp_id)
{
    _bcm_ptp_info_t *ptp_info_p = &_bcm_common_ptp_info[unit];
    _bcm_ptp_stack_info_t *stack_p = &ptp_info_p->stack_info[ptp_id];
    int iter;

    for (iter = 0; iter < 10000; ++iter) {
        if (GET_MBOX_STATUS(stack_p, 0) == MBOX_STATUS_EMPTY) {
            return BCM_E_NONE;
        }
        sal_usleep(1);
    }

    LOG_ERROR(BSL_LS_BCM_COMMON,
              (BSL_META_U(unit,
                          "Failed async Tx to ToP.  No clear\n")));
    return BCM_E_TIMEOUT;
}


#endif


/*
 * Function:
 *      _bcm_ptp_unicast_slave_subscribe
 * Purpose:
 *      Manage external slave subscriptions of a PTP clock port.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      slave_info - (IN) Unicast slave information.
 *      msg_type   - (IN) PTP message type to request/cancel.
 *      tlv_type   - (IN) TLV type (request/cancel unicast transmission).
 *      interval   - (IN) Log inter-message interval (logInterMessagePeriod).
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      PTP unicast negotiation is used to manage external slave services
 *      through REQUEST_UNICAST_TRANSMISSION and CANCEL_UNICAST_TRANSMISSION
 *      TLVs.
 *      Ref. IEEE Std. 1588-2008, Chapters 14.1 and 16.1.
 */
int
_bcm_ptp_unicast_slave_subscribe(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    _bcm_ptp_clock_peer_ext_t *slave_info,
    bcm_ptp_message_type_t msg_type,
    bcm_ptp_tlv_type_t tlv_type,
    int interval)
{
    int rv = BCM_E_UNAVAIL;
    int rvbody = BCM_E_FAIL;

    bcm_ptp_port_identity_t portid;
    uint8 message[1500];
    uint16 message_len;

    uint16 tlv_octets = 0;
    uint16 sequence_id = 0x0002;

    uint8 *response_data;
    uint16 response_tlv_type;
    int response_len;
    int i = 0;

    _bcm_ptp_info_t *ptp_info_p = &_bcm_common_ptp_info[unit];
    _bcm_ptp_stack_info_t *stack_p = &ptp_info_p->stack_info[ptp_id];

    rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    if ((unit_mgmt_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (STACK_DATA(unit, ptp_id).memstate != PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    rv = _bcm_ptp_sem_take(STACK_DATA(unit, ptp_id).outgoing_available, PTP_MGMT_RESPONSE_TIMEOUT_US);
    if (BCM_FAILURE(rv)) {
        PTP_ERROR_FUNC("_bcm_ptp_sem_take()");
        return rv;
    }

    /*
     * TLV-type dependent message constructor parameters.
     * Set unicast signaling message size information based on TLV type.
     * Set sequence ID numbering scheme offset.
     */
    switch (tlv_type) {
    case bcmPTPTlvTypeRequestUnicastTransmission:
        tlv_octets = PTP_UCSIG_REQUEST_TLV_SIZE_OCTETS;
        message_len = PTP_UCSIG_REQUEST_TOTAL_SIZE_OCTETS - PTP_PTPHDR_START_IDX;
        sequence_id += 0;
        break;

    case bcmPTPTlvTypeCancelUnicastTransmission:
        tlv_octets = PTP_UCSIG_CANCEL_TLV_SIZE_OCTETS;
        message_len = PTP_UCSIG_CANCEL_TOTAL_SIZE_OCTETS - PTP_PTPHDR_START_IDX;
        sequence_id += 4;
        break;

    default:
        rv = BCM_E_PARAM;
        goto release_return;
    }

    /* Make unicast transmission message. */
    sal_memset(message, 0, sizeof (message));

    sal_memcpy(message,
               ptphdr_ucsig, sizeof(ptphdr_ucsig));
    sal_memcpy(message + PTP_PTPHDR_SIZE_OCTETS,
               tgtport_ucsig, sizeof(tgtport_ucsig));
    sal_memcpy(message + PTP_PTPHDR_SIZE_OCTETS + PTP_MGMTHDR_SIZE_OCTETS,
               unicast_request_tlv_ucsig, sizeof(unicast_request_tlv_ucsig));

    /* Insert PTP header length. */
    _bcm_ptp_uint16_write(message + PTP_PTPHDR_MSGLEN_OFFSET_OCTETS, message_len);

    message[PTP_PTPHDR_DOMAIN_OFFSET_OCTETS] = CLOCK_DATA(unit, ptp_id, clock_num).domain;

    /*
     * Insert source port identity.
     * NOTICE: In this context, the source port is the remote port, i.e.,
     *         requester/grantee of signaling messages.
     */
    sal_memcpy(message + PTP_PTPHDR_SRCPORT_OFFSET_OCTETS,
               slave_info->peer.clock_identity, sizeof(bcm_ptp_clock_identity_t));

    _bcm_ptp_uint16_write(message + PTP_PTPHDR_SRCPORT_OFFSET_OCTETS + BCM_PTP_CLOCK_EUID_IEEE1588_SIZE,
                          slave_info->peer.remote_port_number);

    /*
     * Insert sequence ID.
     * NOTICE: Logic uses a unique sequence IDs for distinct classes only of
     *         REQUEST_UNICAST_TRANSMISSION and CANCEL_UNICAST_TRANSMISSION
     *         signaling messages.
     */
    switch (msg_type) {
    case bcmPTP_MESSAGE_TYPE_ANNOUNCE:
        break;

    case bcmPTP_MESSAGE_TYPE_SYNC:
       sequence_id += 1;
       break;

    case bcmPTP_MESSAGE_TYPE_DELAY_RESP:
       sequence_id += 2;
       break;

    case bcmPTP_MESSAGE_TYPE_PDELAY_RESP:
        sequence_id += 3;
        break;

    default:
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Unknown message type"));
        rv = BCM_E_PARAM;
        goto release_return;
    }

    _bcm_ptp_uint16_write(message + PTP_PTPHDR_SEQID_OFFSET_OCTETS, sequence_id);

    /* Insert target port identity. */
    rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id, clock_num, slave_info->peer.local_port_number, &portid);
    if (BCM_FAILURE(rv)) {
        goto release_return;
    }

    sal_memcpy(message + PTP_SIGHDR_TGTCLOCK_OFFSET_OCTETS,
               portid.clock_identity, sizeof(bcm_ptp_clock_identity_t));

    _bcm_ptp_uint16_write(message + PTP_SIGHDR_TGTPORT_OFFSET_OCTETS, slave_info->peer.local_port_number);

    /* Insert TLV type. */
    _bcm_ptp_uint16_write(message + PTP_SIGHDR_SIZE_OCTETS, tlv_type);

    /* Insert TLV length. NOTICE: PTP message length, TLV "V" part only. */
    _bcm_ptp_uint16_write(message + PTP_SIGHDR_SIZE_OCTETS + PTP_UCSIG_TLVLEN_OFFSET_OCTETS, (tlv_octets-4));

    /*
     * Insert message type.
     * NOTICE: IEEE Std. 1588-2008, Tables 73-75 specify following format for
     *         4-bit message type (messageType).
     *         |B7|B6|B5|B4|B3|B2|B1|B0| =
     *         |messageType|X |X |X |X |. X = Reserved.
     */
    message[PTP_SIGHDR_SIZE_OCTETS + PTP_UCSIG_MSGTYPE_OFFSET_OCTETS] = ((msg_type << 4) & 0xf0);

    if (tlv_type == bcmPTPTlvTypeRequestUnicastTransmission) {
        /* REQUEST_UNICAST_TRANSMISSION TLV. */

        /* Set log message interval. */
        message[PTP_SIGHDR_SIZE_OCTETS + PTP_UCSIG_INTERVAL_OFFSET_OCTETS] = (uint8)interval;

        /* Set message duration. */
        _bcm_ptp_uint32_write(message + PTP_SIGHDR_SIZE_OCTETS + PTP_UCSIG_INTERVAL_OFFSET_OCTETS + 1,
                              PTP_UNICAST_NEVER_EXPIRE);

        /* Insert signaling message TLV data. Peer address */
        i = PTP_SIGHDR_SIZE_OCTETS + PTP_UCSIG_ADDRTYPE_OFFSET_OCTETS;
        switch (slave_info->peer.peer_address.addr_type) {
        case bcmPTPUDPIPv4:
            /* Ethernet/UDP/IPv4 address type. */
            message[i++] = bcmPTPUDPIPv4;
            sal_memset(message + i, 0, PTP_MAX_NETW_ADDR_SIZE);
            _bcm_ptp_uint32_write(message + i, slave_info->peer.peer_address.ipv4_addr);

            break;

        case bcmPTPUDPIPv6:
            /* Ethernet/UDP/IPv6 address type. */
            message[i++] = bcmPTPUDPIPv6;
            sal_memcpy(message + i, slave_info->peer.peer_address.ipv6_addr, sizeof(bcm_ip6_t));

            break;

        default:
            /* Unknown or unsupported address type. */
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "%s() failed %s\n"), FUNCTION_NAME(), "Unknown or unsupported address type"));
            rv = BCM_E_PARAM;
            goto release_return;
        }

        /* Signaling message L2 header length and L2 header. */
        i += PTP_MAX_NETW_ADDR_SIZE;

        message[i++] =
            (slave_info->peer.peer_address.raw_l2_header_length > PTP_MAX_L2_HEADER_LENGTH) ?
            (PTP_MAX_L2_HEADER_LENGTH) : (slave_info->peer.peer_address.raw_l2_header_length);

        sal_memcpy(message + i,
                   slave_info->peer.peer_address.raw_l2_header,
                   slave_info->peer.peer_address.raw_l2_header_length);

        i += slave_info->peer.peer_address.raw_l2_header_length;

        /* Add Local address */
        sal_memcpy(message + i, slave_info->local_address.address, PTP_MAX_NETW_ADDR_SIZE);
        i += PTP_MAX_NETW_ADDR_SIZE;
    } else if (tlv_type == bcmPTPTlvTypeCancelUnicastTransmission) {
        /* CANCEL_UNICAST_TRANSMISSION TLV. */

        /* Reserved TLV element. */
        message[PTP_SIGHDR_SIZE_OCTETS + PTP_UCSIG_CANCEL_RESERVED_OFFSET_OCTETS] = 0x00;

        /*
         * Insert signaling message TLV data.
         *    Peer address.
         */
        i = PTP_SIGHDR_SIZE_OCTETS + PTP_UCSIG_CANCEL_ADDRTYPE_OFFSET_OCTETS;
        switch (slave_info->peer.peer_address.addr_type) {
        case bcmPTPUDPIPv4:
            /* Ethernet/UDP/IPv4 address type. */
            message[i++] = bcmPTPUDPIPv4;
            _bcm_ptp_uint32_write(message + i, slave_info->peer.peer_address.ipv4_addr);

            break;

        case bcmPTPUDPIPv6:
            /* Ethernet/UDP/IPv6 address type. */
            message[i++] = bcmPTPUDPIPv6;
            sal_memcpy(message + i, slave_info->peer.peer_address.ipv6_addr, sizeof(bcm_ip6_t));

            break;

        default:
            /* Unknown or unsupported address type. */
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "%s() failed %s\n"), FUNCTION_NAME(), "Unknown or unsupported address type"));
            rv = BCM_E_PARAM;
            goto release_return;
        }
    }

/*     LOG_CLI((BSL_META_U(unit,
                           "sig_send_tx(len:%d)\n"), message_len)); */
/*     _bcm_ptp_dump_hex(message, message_len, 0); */

    /* Transmit message. */
    rv = stack_p->tx(unit, ptp_id, clock_num, PTP_MESSAGE_TO_TOP, 0, 0, message, message_len);
    if (BCM_FAILURE(rv)) {
        PTP_ERROR_FUNC("tx()");
        goto release_return;
    }

    /*
     * NOTICE: This call will return an rx response buffer that we
     *         will own and need to free.
     */
    if (BCM_FAILURE(rv = _bcm_ptp_rx_response_get(unit, ptp_id, clock_num,
            PTP_MGMT_RESPONSE_TIMEOUT_US, &response_data, &response_len))) {
        PTP_ERROR_FUNC("bcmptp_rx_get_response()");
        goto release_return;
    }

    rv = BCM_E_FAIL;

    /* Parse response. */
    response_tlv_type = _bcm_ptp_uint16_read(response_data +
        PTP_SIGHDR_SIZE_OCTETS);
    if (tlv_type == bcmPTPTlvTypeRequestUnicastTransmission &&
            response_tlv_type != bcmPTPTlvTypeGrantUnicastTransmission) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Unicast request failed"));
        goto dispose_of_resp;
    }

    if (tlv_type == bcmPTPTlvTypeCancelUnicastTransmission &&
            response_tlv_type != bcmPTPTlvTypeAckCancelUnicastTransmission) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Unicast cancel failed"));
        goto dispose_of_resp;
    }

    rv = BCM_E_NONE;

dispose_of_resp:
    stack_p->rx_free(unit, ptp_id, response_data);

release_return:
    rvbody = rv;
    if (BCM_FAILURE(rv = _bcm_ptp_sem_give(STACK_DATA(unit, ptp_id).outgoing_available))) {
        PTP_ERROR_FUNC("_bcm_ptp_sem_give()");
    }

    return rvbody ? rvbody : rv;
}

/*
 * Function:
 *      _bcm_ptp_management_domain_get
 * Purpose:
 *      Get the current domain used for PTP management messages for a PTP clock.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      domain    - (OUT) PTP domain.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_management_domain_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 *domain)
{
    int rv = BCM_E_UNAVAIL;

    rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    if ((unit_mgmt_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (STACK_DATA(unit, ptp_id).memstate != PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    *domain = CLOCK_DATA(unit, ptp_id, clock_num).domain;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_management_domain_set
 * Purpose:
 *      Set the domain to use in PTP management messages for a PTP clock.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      domain    - (IN) PTP domain.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_management_domain_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 domain)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if ((unit_mgmt_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (STACK_DATA(unit, ptp_id).memstate != PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    CLOCK_DATA(unit, ptp_id, clock_num).domain = domain;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_management_message_create
 * Purpose:
 *      Create a PTP management message from template.
 * Parameters:
 *      message - (IN/OUT) PTP management message.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static int
_bcm_ptp_management_message_create(
    uint8 *message)
{
    int rv = BCM_E_UNAVAIL;
    int i = 0;

    /*
     * Management message header
     * NOTICE: Includes L2, VLAN, IPv4, UDP, PTP, and management headers.
     */
    sal_memcpy(message, l2hdr_default, sizeof(l2hdr_default));
    i += sizeof(l2hdr_default);

    sal_memcpy(message + i, vlanhdr_default, sizeof(vlanhdr_default));
    i += sizeof(vlanhdr_default);

    sal_memcpy(message + i, ethhdr_default, sizeof(ethhdr_default));
    i += sizeof(ethhdr_default);

    sal_memcpy(message + i, ipv4hdr_default, sizeof(ipv4hdr_default));
    i += sizeof(ipv4hdr_default);

    sal_memcpy(message + i, udphdr_default, sizeof(udphdr_default));
    i += sizeof(udphdr_default);

    sal_memcpy(message + i, ptphdr_mgmt, sizeof(ptphdr_mgmt));
    i += sizeof(ptphdr_mgmt);

    sal_memcpy(message + i, mgmthdr_mgmt, sizeof(mgmthdr_mgmt));
    i += sizeof(mgmthdr_mgmt);

    sal_memcpy(message + i, tlvhdr_mgmt, sizeof(tlvhdr_mgmt));
    i += sizeof(tlvhdr_mgmt);

    /* Set L2 header (Ethertype= IPv4). */
    i = PTP_L2HDR_SIZE_OCTETS + PTP_VLANHDR_SIZE_OCTETS;
    message[i++] = 0x08;
    message[i] = 0x00;

    /* Update IPv4 header checksum. */
    rv = _bcm_ptp_management_message_ipv4hdr_checksum_set(message);

    return rv;
}

/* /\* */
/*  * Function: */
/*  *      _bcm_ptp_management_message_make */
/*  * Purpose: */
/*  *      Make a PTP management message. */
/*  * Parameters: */
/*  *      message     - (IN/OUT) PTP management message. */
/*  *      action      - (IN) PTP management action (actionField). */
/*  *      cmd         - (IN) PTP command (managementId). */
/*  *      payload     - (IN) PTP management message payload. */
/*  *      payload_len - (IN) PTP management message payload size (octets). */
/*  * Returns: */
/*  *      BCM_E_XXX */
/*  * Notes: */
/*  *\/ */
/* static int  */
/* _bcm_ptp_management_message_make( */
/*     uint8 *message,  */
/*     uint8 action,  */
/*     uint16 cmd,  */
/*     uint8 *payload,  */
/*     int payload_len) */
/* { */
/*     int rv = BCM_E_UNAVAIL; */
/*     int i = 0; */

/*     /\* Set management message action. *\/ */
/*     i = PTP_MGMTHDR_START_IDX + PTP_MGMTHDR_ACTION_OFFSET_OCTETS; */
/*     message[i] = action; */

/*     /\*  */
/*      * Set TLV data. */
/*      * TLV length.   */
/*      *\/ */
/*     i = PTP_MGMTHDR_START_IDX + PTP_MGMTHDR_SIZE_OCTETS +  */
/*         PTP_TLVHDR_LEN_OFFSET_OCTETS; */

/*     message[i++] = (uint8)((payload_len + PTP_TLVHDR_VAL_PREFIX_SIZE_OCTETS) >> 8); */
/*     message[i] = (uint8)((payload_len + PTP_TLVHDR_VAL_PREFIX_SIZE_OCTETS) & 0x00ff); */

/*     /\*  */
/*      * Set TLV data.  */
/*      * TLV value prefix (management ID/command definition). */
/*      *\/ */
/*     i = PTP_MGMTHDR_START_IDX + PTP_MGMTHDR_SIZE_OCTETS +  */
/*         PTP_TLVHDR_VAL_PREFIX_OFFSET_OCTETS; */

/*     message[i++]= (uint8)((cmd) >> 8); */
/*     message[i]= (uint8)((cmd) & 0x00ff); */

/*     /\* */
/*      * Insert payload. */
/*      * TLV value body. */
/*      *\/ */
/*     sal_memcpy(message + PTP_MGMTMSG_TOTAL_HEADER_SIZE, payload, payload_len); */

/*     /\* Update management message header metadata. *\/ */
/*     rv = _bcm_ptp_management_message_header_metadata_set(message); */

/*     return rv; */
/* } */

/*
 * Function:
 *      _bcm_ptp_management_message_header_metadata_set
 * Purpose:
 *      Set header lengths and IPv4 checksum based on PTP message payload.
 * Parameters:
 *      message     - (IN/OUT) PTP management message.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
/* static int  */
/* _bcm_ptp_management_message_header_metadata_set( */
/*     uint8 *message) */
/* { */
/*     int rv = BCM_E_UNAVAIL; */

/*     int tlv_length_pos; */
/*     uint16 tlv_length; */
/*     uint16 ptp_length; */
/*     uint16 udp_length; */
/*     uint16 ip_length; */

/*     tlv_length_pos = PTP_MGMTHDR_START_IDX + PTP_MGMTHDR_SIZE_OCTETS +  */
/*                      PTP_TLVHDR_LEN_OFFSET_OCTETS; */

/*     tlv_length = _bcm_ptp_uint16_read(message + tlv_length_pos); */
/*     ptp_length = tlv_length + PTP_PTPHDR_SIZE_OCTETS + PTP_MGMTHDR_SIZE_OCTETS +  */
/*                  PTP_TLVHDR_VAL_PREFIX_OFFSET_OCTETS; */
/*     udp_length = ptp_length + PTP_UDPHDR_SIZE_OCTETS; */
/*     ip_length = udp_length + PTP_IPV4HDR_SIZE_OCTETS; */

/*     _bcm_ptp_uint16_write(message + PTP_PTPHDR_START_IDX +  */
/*                           PTP_PTPHDR_MSGLEN_OFFSET_OCTETS, ptp_length); */
/*     _bcm_ptp_uint16_write(message + PTP_UDPHDR_START_IDX +  */
/*                           PTP_UDPHDR_MSGLEN_OFFSET_OCTETS, udp_length); */
/*     _bcm_ptp_uint16_write(message + PTP_IPV4HDR_START_IDX +  */
/*                           PTP_IPV4HDR_MSGLEN_OFFSET_OCTETS, ip_length); */

/*     /\* Update IPv4 header checksum. *\/ */
/*     rv = _bcm_ptp_management_message_ipv4hdr_checksum_set(message); */

/*     return rv; */
/* } */

/*
 * Function:
 *      _bcm_ptp_management_macaddr_set
 * Purpose:
 *      Set MAC addresses in PTP management message.
 * Parameters:
 *      message  - (IN/OUT) PTP management message.
 *      src_mac  - (IN) Source MAC address.
 *      dest_mac - (IN) Destination MAC address.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static int
_bcm_ptp_management_message_macaddr_set(
    uint8 *message,
    bcm_mac_t *src_mac,
    bcm_mac_t *dest_mac)
{
    int i = sizeof(bcm_mac_t);

    sal_memcpy(message, dest_mac, i);
    sal_memcpy(message + i, src_mac, i);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_management_message_vlantag_set
 * Purpose:
 *      Set VLAN tag in PTP management message.
 * Parameters:
 *      message  - (IN/OUT) PTP management message.
 *      vlan_tag - (IN) VLAN identifier (VID).
 *      prio     - (IN) Priority code point (PCP).
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static int
_bcm_ptp_management_message_vlantag_set(
    uint8 *message,
    uint16 vlan_tag,
    uint8 prio)
{
    int i = PTP_L2HDR_SIZE_OCTETS + PTP_VLANHDR_VID_OFFSET_OCTETS;

    /*
     * Set VLAN tag.
     * Logically combine VLAN identifier (VID) with priority code point (PCP).
     *
     *    IEEE 802.1Q VLAN tagging 32-bit frame.
     *    |16 Bits|      16 Bits       |
     *    |       |3 Bits|1 Bit|12 Bits|
     *    |TPID   |PCP   |CFI  |VID    |
     */
    message[i++] = (prio << 5) | (0x0f & (vlan_tag >> 8));
    message[i] = (vlan_tag & 0x00ff);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_management_message_ipaddr_set
 * Purpose:
 *      Set IP addresses in PTP management message.
 * Parameters:
 *      message - (IN/OUT) PTP management message.
 *      src_ip  - (IN) Source IPv4 address.
 *      dest_ip - (IN) Destination IPv4 address.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static int
_bcm_ptp_management_message_ipaddr_set(
    uint8 *message,
    bcm_ip_t src_ip,
    bcm_ip_t dest_ip)
{
    int rv = BCM_E_UNAVAIL;
    int i = PTP_L2HDR_SIZE_OCTETS + PTP_ETHHDR_SIZE_OCTETS +
            PTP_VLANHDR_SIZE_OCTETS + PTP_IPV4HDR_SRC_IPADDR_OFFSET_OCTETS;

    /* Update IPv4 header checksum. */
    if (BCM_FAILURE(rv =
            _bcm_ptp_management_message_ipv4hdr_checksum_set(message))) {
        return rv;
    }

    /* Set management message header IP addresses. */
    _bcm_ptp_uint32_write(message + i, src_ip);
    i += sizeof(bcm_ip_t);
    _bcm_ptp_uint32_write(message + i, dest_ip);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_management_message_ipv4hdr_checksum_set
 * Purpose:
 *      Set IPv4 header checksum.
 * Parameters:
 *      message - (IN/OUT) PTP management message.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static int
_bcm_ptp_management_message_ipv4hdr_checksum_set(
    uint8 *message)
{
    uint16 chksum;
    int i = PTP_L2HDR_SIZE_OCTETS + PTP_VLANHDR_SIZE_OCTETS +
            PTP_ETHHDR_SIZE_OCTETS;

    /*
     * Zero header checksum fields in IPv4 header.
     * Ref. RFC 791, pp. 14.
     */
    message[i+PTP_IPV4HDR_HDR_CHKSUM_OFFSET_OCTETS] = 0x00;
    message[i+PTP_IPV4HDR_HDR_CHKSUM_OFFSET_OCTETS+1] = 0x00;

    /* Calculate checksum. */
    chksum = _bcm_ptp_management_rfc791_checksum(message + i,
                                                 PTP_IPV4HDR_SIZE_OCTETS);

    /* Insert checksum answer in IPv4 header. */
    message[i+PTP_IPV4HDR_HDR_CHKSUM_OFFSET_OCTETS] = (uint8)((chksum) >> 8);
    message[i+PTP_IPV4HDR_HDR_CHKSUM_OFFSET_OCTETS+1] = (uint8)((chksum) & 0x00ff);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_management_message_port_set
 * Purpose:
 *      Set source and destination port numbers in UDP header.
 * Parameters:
 *      message   - (IN/OUT) PTP management message.
 *      src_port  - (IN) Source port number.
 *      dest_port - (IN) Destination port number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static int
_bcm_ptp_management_message_port_set(
    uint8 *message,
    uint16 src_port,
    uint16 dest_port)
{
    int i = PTP_UDPHDR_START_IDX;

    _bcm_ptp_uint16_write(message + i, src_port);
    i += sizeof(uint16);
    _bcm_ptp_uint16_write(message + i, dest_port);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_management_message_target_set
 * Purpose:
 *      Set target PTP clock port identity in PTP management message.
 * Parameters:
 *      message        - (IN/OUT) PTP management message.
 *      clock_identity - (IN) Target PTP clock identity.
 *      port_num       - (IN) Target PTP clock port number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
/* static int  */
/* _bcm_ptp_management_message_target_set( */
/*     uint8 *message,  */
/*     const bcm_ptp_clock_identity_t clock_identity,  */
/*     uint16 port_num) */
/* { */
/*     int i = PTP_MGMTHDR_START_IDX;    */

/*     sal_memcpy(message + i, clock_identity, sizeof(bcm_ptp_clock_identity_t)); */
/*     i += sizeof(bcm_ptp_clock_identity_t); */
/*     _bcm_ptp_uint16_write(message + i, port_num); */

/*     return BCM_E_NONE; */
/* } */

/*
 * Function:
 *      _bcm_ptp_management_message_length_get
 * Purpose:
 *      Get the length of PTP management message.
 * Parameters:
 *      message - (IN)  PTP management message.
 * Returns:
 *      Length.
 * Notes:
 */
/* static int  */
/* _bcm_ptp_management_message_length_get( */
/*     uint8 *message) */
/* { */
/*     int len = PTP_MGMTMSG_TOTAL_HEADER_SIZE - PTP_TLVHDR_VAL_PREFIX_SIZE_OCTETS; */

/*     len += _bcm_ptp_uint16_read(message + PTP_MGMTHDR_START_IDX +  */
/*                                 PTP_MGMTHDR_SIZE_OCTETS +  */
/*                                 PTP_TLVHDR_LEN_OFFSET_OCTETS); */

/*     return len;    */
/* } */

#if 0 /* Unused. */
/*
 * Function:
 *      _bcm_ptp_management_message_domain_get
 * Purpose:
 *      Get the PTP domain in PTP management message.
 * Parameters:
 *      message - (IN)  PTP management message.
 *      domain  - (OUT) PTP domain.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static int
_bcm_ptp_management_message_domain_get(
    uint8 *message,
    uint8 *domain)
{
    *domain = message[PTP_PTPHDR_START_IDX + PTP_PTPHDR_DOMAIN_OFFSET_OCTETS];
    return BCM_E_NONE;
}
#endif /* Unused. */

/*
 * Function:
 *      _bcm_ptp_management_message_domain_set
 * Purpose:
 *      Set the PTP domain in PTP management message.
 * Parameters:
 *      message - (IN/OUT) PTP management message.
 *      domain  - (IN) PTP domain.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_management_message_domain_set(
    uint8 *message,
    uint8 domain)
{
    message[PTP_PTPHDR_START_IDX + PTP_PTPHDR_DOMAIN_OFFSET_OCTETS] = domain;
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_management_rfc791_checksum
 * Purpose:
 *      Calculate checksum of a packet per RFC 791.
 * Parameters:
 *      packet     - (IN) Packet.
 *      packet_len - (IN) Packet size (octets).
 * Returns:
 *      Checksum.
 * Notes:
 *      Caller is responsible for zeroing header checksum fields if IPv4
 *      packets are passed to the algorithm.
 */
static uint16
_bcm_ptp_management_rfc791_checksum(
    uint8 *packet,
    int packet_len)
{
    uint32 chksum = 0x00000000;

    /*
     * CHECKSUM ALGORITHM: RFC 791, pp 14.
     *
     * "The checksum algorithm is:
     *    The checksum field is the 16 bit one's complement of the one's
     *    complement sum of all 16 bit words in the header. For purposes of
     *    computing the checksum, the value of the checksum field is zero."
     *
     * NOTICE: An equivalent, efficient implementation algorithm enlists
     *         32-bit addition and handles 16-bit overflows (carries) as
     *         a penultimate step.
     */

    while (packet_len > 1)
    {
        /* Assemble words from consecutive octets and add to sum. */
        chksum += (uint32)((((uint16)packet[0]) << 8) | (packet[1]));

        /*
         * Increment pointer in packet.
         * Decrement remaining size.
         */
        packet += 2;
        packet_len -= 2;
    }

    /* Process odd octet. */
    if (packet_len == 1) {
        chksum += (uint32)(((uint16)packet[0]) << 8);
    }

    /* Carry(ies). */
    chksum = (chksum >> 16) + (chksum & 0x0000ffff);

    /*
     * Result.
     * NOTICE: 16-bit ones complement.
     */
    return (~((uint16)chksum));
}


/*
 * Function:
 *      _bcm_ptp_rfc791_checksum_add_word
 * Purpose:
 *      Calculate incremental checksum
 * Parameters:
 *      prev_checksum - (IN) Previous checksum
 *      add_word      - (IN) word added
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static uint16
_bcm_ptp_rfc791_checksum_add_word(
    uint16 prev_checksum,
    uint16 add_word)
{
    uint32 intermediate = 0xffff & ~prev_checksum;
    intermediate += add_word;
    intermediate = (intermediate >> 16) + (intermediate & 0xffff);

    return (uint16)~intermediate;
}


/*
 * Function:
 *      _bcm_ptp_construct_udp_packet
 * Purpose:
 *      Construct UDP packet for signaling messages
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_construct_udp_packet(
    bcm_ptp_clock_peer_t *peer,
    bcm_ptp_clock_port_address_t *local_port,
    uint8* udp_payload,
    int udp_payload_len,
    uint8* packet,
    int *packet_len)
{
    int ret = BCM_E_NONE;
    int cursor = 0;
    int ip_header_start, ip_checksum_pos;
    int udp_header_start, udp_checksum_pos;
    uint32 checksum;  /* 16 bit checksum, with overhead space for carry bits during computation */

    /* L2 Header */
    sal_memcpy(packet, &peer->peer_address.raw_l2_header[0], peer->peer_address.raw_l2_header_length);
    cursor += peer->peer_address.raw_l2_header_length;

    ip_header_start = cursor;

    switch (peer->peer_address.addr_type) {
    case bcmPTPUDPIPv4:
        packet[cursor++] = 0x45;  /* version */
        packet[cursor++] = 0;     /* TOS */
        _bcm_ptp_uint16_write(&packet[cursor], 20 + 8 + udp_payload_len);  /* Length: IP Hdr, UDP Hdr, payload */
        cursor += 2;
        _bcm_ptp_uint16_write(&packet[cursor], 0); /* ident */
        cursor += 2;
        packet[cursor++] = 0x40;            /* flags: don't fragment */
        packet[cursor++] = 0;               /* fragment offset 0 */
        packet[cursor++] = 64;              /* TTL = 64 */
        packet[cursor++] = 17;              /* Proto = UDP */
        ip_checksum_pos = cursor;
        _bcm_ptp_uint16_write(&packet[cursor], 0); /* checksum (write as 0, then compute) */
        cursor += 2;

        packet[cursor++] = local_port->address[0];
        packet[cursor++] = local_port->address[1];
        packet[cursor++] = local_port->address[2];
        packet[cursor++] = local_port->address[3];

        _bcm_ptp_uint32_write(&packet[cursor], peer->peer_address.ipv4_addr);
        cursor += 4;

        /* calculate and write the IP checksum */
        checksum = _bcm_ptp_management_rfc791_checksum(&packet[ip_header_start], 20);
        _bcm_ptp_uint16_write(&packet[ip_checksum_pos], checksum);

        /* UDP header */
        udp_header_start = cursor;
        _bcm_ptp_uint16_write(&packet[cursor], 320);  /* Source Port */
        cursor += 2;
        _bcm_ptp_uint16_write(&packet[cursor], 320);  /* Dest Port */
        cursor += 2;

        _bcm_ptp_uint16_write(&packet[cursor], 8 + udp_payload_len);  /* Length: UDP Hdr, payload  */
        cursor += 2;
        _bcm_ptp_uint16_write(&packet[cursor], 0);                /* Checksum: 0 -> no checksum */
        cursor += 2;

        sal_memcpy(&packet[cursor], udp_payload, udp_payload_len); /* Payload */
        cursor += udp_payload_len;
        break;

    case bcmPTPUDPIPv6:
        packet[cursor++] = 0x6e;  /* version 6, class 0xe0 (high nybble) */
        packet[cursor++] = 0;     /* class 0xe0 (low nybble), flow label 0 */
        packet[cursor++] = 0;     /* flow label 0 */
        packet[cursor++] = 0;     /* flow label 0 */
        _bcm_ptp_uint16_write(&packet[cursor], 8 + udp_payload_len);  /* Length: UDP Hdr, payload (sic, no header len like ipv4) */
        cursor += 2;
        packet[cursor++] = 17;    /* Protocol: UDP */
        packet[cursor++] = 64;    /* Hop Limit (TTL) */
        sal_memcpy(&packet[cursor], &local_port->address[0], 16);
        cursor += 16;
        sal_memcpy(&packet[cursor], &peer->peer_address.ipv6_addr[0], 16);
        cursor += 16;

        /* UDP header */
        udp_header_start = cursor;
        _bcm_ptp_uint16_write(&packet[cursor], 320);  /* Source Port */
        cursor += 2;
        _bcm_ptp_uint16_write(&packet[cursor], 320);  /* Dest Port */
        cursor += 2;

        _bcm_ptp_uint16_write(&packet[cursor], 8 + udp_payload_len);  /* Length: UDP Hdr, payload   */
        cursor += 2;
        udp_checksum_pos = cursor;
        _bcm_ptp_uint16_write(&packet[cursor], 0);                /* Checksum: 0 during calculation */
        cursor += 2;

        sal_memcpy(&packet[cursor], udp_payload, udp_payload_len); /* Payload */
        cursor += udp_payload_len;

        /* checksum over addrs, UDP, & payload */
        checksum = _bcm_ptp_management_rfc791_checksum(&packet[udp_header_start - 32], 32 + 8 + udp_payload_len);
        checksum = _bcm_ptp_rfc791_checksum_add_word(checksum, udp_payload_len + 8);  /* UDP header len + len */
        checksum = _bcm_ptp_rfc791_checksum_add_word(checksum, 17);  /* UDP protocol field */
        _bcm_ptp_uint16_write(&packet[udp_checksum_pos], checksum);

        break;

    default:
        return BCM_E_PARAM;
    }

    *packet_len = cursor;

    return ret;
}

/*
 * Function:
 *      _bcm_ptp_construct_signaling_msg
 * Purpose:
 *      Construct the UDP payload for a signaling message,
 *      given the TLV payload for the message
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_construct_signaling_msg(
    bcm_ptp_port_identity_t *from_port,
    bcm_ptp_clock_identity_t to_clock,
    uint16 to_port,
    int domain,
    uint16 sequenceId,
    uint8* tlv_payload,
    int tlv_payload_len,
    uint8* udp_payload,
    int *udp_payload_len)
{
    memset(udp_payload, 0, 34);  /* blank PTP header */
    udp_payload[0] = bcmPTP_MESSAGE_TYPE_SIGNALING;
    udp_payload[1] = 2;
    _bcm_ptp_uint16_write(&udp_payload[2], 44 + tlv_payload_len);
    udp_payload[4] = domain;
    udp_payload[6] = 0x04;    /* flags: unicast */
    sal_memcpy(&udp_payload[20], &from_port->clock_identity[0], 8);
    _bcm_ptp_uint16_write(&udp_payload[28], from_port->port_number);
    _bcm_ptp_uint16_write(&udp_payload[30], sequenceId);
    udp_payload[32] = 0x05;   /* control field: all others */
    udp_payload[33] = 0x7f;   /* logMessageInterval: 0x7f */

    sal_memcpy(&udp_payload[34], &to_clock[0], 8);
    _bcm_ptp_uint16_write(&udp_payload[42], to_port);

    if (NULL != tlv_payload) {
        sal_memcpy(&udp_payload[44], tlv_payload, tlv_payload_len);
    } else {
        tlv_payload_len = 0;
    }

    *udp_payload_len = tlv_payload_len + 44;

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_ptp_construct_signaling_request_tlv
 * Purpose:
 *      Construct the TLV for a signaling msg request unicast transmission
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_construct_signaling_request_tlv(
    int type,
    int interval,
    unsigned duration,
    uint8* tlv_payload,
    int *tlv_payload_len)
{
    tlv_payload[0] = 0;  /* TLV type: */
    tlv_payload[1] = bcmPTPTlvTypeRequestUnicastTransmission;
    tlv_payload[2] = 0;  /* Length: */
    tlv_payload[3] = 6;
    tlv_payload[4] = (type << 4);
    tlv_payload[5] = interval;
    _bcm_ptp_uint32_write(&tlv_payload[6], duration);

    *tlv_payload_len = 10;

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_ptp_construct_signaling_cancel_tlv
 * Purpose:
 *      Construct the TLV for a signaling msg cancel unicast transmission
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_construct_signaling_cancel_tlv(
    int type,
    uint8* tlv_payload,
    int *tlv_payload_len)
{
    tlv_payload[0] = 0;  /* TLV type: */
    tlv_payload[1] = bcmPTPTlvTypeCancelUnicastTransmission;
    tlv_payload[2] = 0;  /* Length: */
    tlv_payload[3] = 2;
    tlv_payload[4] = (type << 4);
    tlv_payload[5] = 0;

    *tlv_payload_len = 6;

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_ptp_construct_signaling_grant_tlv
 * Purpose:
 *      Construct the TLV for a signaling msg grant unicast transmission
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_construct_signaling_grant_tlv(
    int type,
    int interval,
    unsigned duration,
    int invite_renewal,
    uint8* tlv_payload,
    int *tlv_payload_len)
{
    tlv_payload[0] = 0;  /* TLV type: */
    tlv_payload[1] = bcmPTPTlvTypeGrantUnicastTransmission;
    tlv_payload[2] = 0;  /* Length: */
    tlv_payload[3] = 8;
    tlv_payload[4] = (type << 4);
    tlv_payload[5] = interval;
    _bcm_ptp_uint32_write(&tlv_payload[6], duration);
    tlv_payload[10] = 0;
    tlv_payload[11] = invite_renewal;

    *tlv_payload_len = 12;
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_ptp_construct_signaling_ack_cancel_tlv
 * Purpose:
 *      Construct the TLV for a signaling msg acknowledge cancel unicast transmission
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_construct_signaling_ack_cancel_tlv(
    int type,
    uint8* tlv_payload,
    int *tlv_payload_len)
{
    tlv_payload[0] = 0;  /* TLV type: */
    tlv_payload[1] = bcmPTPTlvTypeAckCancelUnicastTransmission;
    tlv_payload[2] = 0;  /* Length: */
    tlv_payload[3] = 2;
    tlv_payload[4] = (type << 4);
    tlv_payload[5] = 0;

    *tlv_payload_len = 6;

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_ptp_signaling_message_append_tlv
 * Purpose:
 *      Append a TLV to a PTP signaling message.
 * Parameters:
 *      msg       (IN/OUT) - PTP signaling message.
 *      msgLength (IN/OUT) - PTP signaling message length.
 *      tlv       (IN)     - Signaling message TLV.
 *      tlvLength (IN)     - Signaling message TLV length.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
_bcm_ptp_signaling_message_append_tlv(
    uint8 *msg,
    int *msgLength,
    uint8 *tlv,
    int tlvLength)
{
    /* Argument checking and error handling. */
    if (NULL == tlv) {
        return BCM_E_FAIL;
    }

    if ((*msgLength != (int)_bcm_ptp_uint16_read(msg + PTP_PTPHDR_MSGLEN_OFFSET_OCTETS)) ||
        (tlvLength != (int)_bcm_ptp_uint16_read(tlv + PTP_TLVHDR_LEN_OFFSET_OCTETS) + 4)) {
        return BCM_E_PARAM;
    }

    /* Append TLV. */
    sal_memcpy(msg + *msgLength, tlv, tlvLength);

    /* Update message length. */
    *msgLength += tlvLength;
    _bcm_ptp_uint16_write(msg + PTP_PTPHDR_MSGLEN_OFFSET_OCTETS, (uint16)*msgLength);

    return BCM_E_NONE;
}

#endif /* defined(INCLUDE_PTP)*/
