/*
 * $Id: rx.h 1.36 Broadcom SDK $
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
 * File:        rx.h
 * Purpose:     Packet Receive Reason Types shared between BCM and SOC layers
 */

#ifndef   _SHR_RX_H_
#define   _SHR_RX_H_

#include <shared/types.h>
#include <shared/bitop.h>

/*
 * PKT RX Packet Reasons; reason CPU received the packet.
 * Notes:
 *      It is possible no reasons are set (directed to CPU from ARL
 *      for example), or multiple reasons may be set.
 */

typedef enum _shr_rx_reason_e {
    _SHR_RX_INVALID,
    _SHR_RX_ARP,                   /* ARP Packet                        */
    _SHR_RX_BPDU,                  /* BPDU Packet                       */
    _SHR_RX_BROADCAST,             /* Broadcast packet                  */
    _SHR_RX_CLASS_BASED_MOVE,      /* Class-based move prevented        */
    _SHR_RX_CLASS_TAG_PACKETS,     /* Higig Header with PPD=1           */
    _SHR_RX_CONTROL,               /* Control frame or reserved addr    */
    _SHR_RX_CPU_LEARN,             /* CPU Learned (or VLAN not found    */
                                   /* on Strata devices)                */
    _SHR_RX_DEST_LOOKUP_FAIL,      /* Destination lookup fail (or L3    */
                                   /* station move on Strata devices)   */
    _SHR_RX_DHCP,                  /* DHCP packets                      */
    _SHR_RX_DOS_ATTACK,            /* DOS Attack Packet                 */
    _SHR_RX_E2E_HOL_IBP,           /* E2E HOL or IBP Packet             */
    _SHR_RX_ENCAP_HIGIG_ERROR,     /* Non-HG packets received on an E-HG port */
    _SHR_RX_FILTER_MATCH,          /* Filter Match                      */
    _SHR_RX_GRE_CHECKSUM,          /* GRE checksum                      */
    _SHR_RX_GRE_SOURCE_ROUTE,      /* GRE source routing                */
    _SHR_RX_HIGIG_CONTROL,         /* Higig Packet with Control Opcode  */
    _SHR_RX_HIGIG_HDR_ERROR,       /* Higig+ header errors              */
    _SHR_RX_ICMP_REDIRECT,         /* ICMP Recirect                     */
    _SHR_RX_IGMP,                  /* IGMP packet                       */
    _SHR_RX_INGRESS_FILTER,        /* Ingress Filter (VLAN membership)  */
    _SHR_RX_IP,                    /* IP packet                         */
    _SHR_RX_IPFIX_RATE_VIOLATION,  /* IPFIX flows exceed metering       */
    _SHR_RX_IP_MCAST_MISS,         /* IPMC miss                         */
    _SHR_RX_IP_OPTION_VERSION,     /* IP options present or IP ver != 4 */
    _SHR_RX_IPMC,                  /* Class D IP multicast packet       */
    _SHR_RX_IPMC_RSVD,             /* IPMC Reserved Packet              */
    _SHR_RX_L2_CPU,                /* L2_TABLE - copy to CPU (on 5690)  */
    _SHR_RX_L2_DEST_MISS,          /* L2 destination lookup failure     */
    _SHR_RX_L2_LEARN_LIMIT,        /* L2 Learn Limit                    */
    _SHR_RX_L2_MOVE,               /* L2 Station Movement               */
    _SHR_RX_L2_MTU_FAIL,           /* L2 MTU check fail                 */
    _SHR_RX_L2_NON_UNICAST_MISS,   /* L2 Non-Unicast lookup miss        */
    _SHR_RX_L2_SOURCE_MISS,        /* L2 source lookup failure          */
    _SHR_RX_L3_ADDR_BIND_FAIL,     /* MAC to IP bind check failure      */
    _SHR_RX_L3_DEST_MISS,          /* L3 DIP Miss                       */
    _SHR_RX_L3_HEADER_ERROR,       /* L3 header - IP options,           */
    _SHR_RX_L3_MTU_FAIL,           /* L3 MTU check fail                 */
    _SHR_RX_L3_SLOW_PATH,          /* L3 slow path processed pkt.       */
    _SHR_RX_L3_SOURCE_MISS,        /* L3 SIP Miss                       */
    _SHR_RX_L3_SOUCE_MOVE,        /* L3 Station Movement               */
    _SHR_RX_MARTIAN_ADDR,          /* Pkt. with Martian address         */
    _SHR_RX_MCAST_IDX_ERROR,       /* Multicast index error             */
    _SHR_RX_MCAST_MISS,            /* MC miss                           */
    _SHR_RX_MIM_SERVICE_ERROR,     /* MAC-in-MAC terminated unicast packets */
                                   /* that do not have a valid I-SID    */
    _SHR_RX_MPLS_CTRL_WORD_ERROR,  /* MPLS Control Word type is not zero */
    _SHR_RX_MPLS_ERROR,            /* MPLS error                        */
    _SHR_RX_MPLS_INVALID_ACTION,   /* MPLS Invalid Action               */
    _SHR_RX_MPLS_INVALID_PAYLOAD,  /* MPLS Invalid Payload              */
    _SHR_RX_MPLS_LABEL_MISS,       /* MPLS table miss                   */
    _SHR_RX_MPLS_SEQUENCE_NUMBER,  /* MPLS Sequence number              */
    _SHR_RX_MPLS_TTL,              /* MPLS TTL                          */
    _SHR_RX_MULTICAST,             /* Multicast packet                  */
    _SHR_RX_NHOP,                  /* Copy to CPU from Next Hop Idx Tbl */
    _SHR_RX_OAM_ERROR,             /* OAM packets to CPU for error cases */
    _SHR_RX_OAM_SLOW_PATH,         /* OAM packets to CPU - slowpath process */
    _SHR_RX_OAM_LMDM,              /* OAM LMM/LMR, DMM/DMR packets to CPU */
    _SHR_RX_PARITY_ERROR,          /* Parity error on IP tables         */
    _SHR_RX_PROTOCOL,              /* Protocol Packet                   */
    _SHR_RX_SAMPLE_DEST,           /* Egress  sFlow sampled             */
    _SHR_RX_SAMPLE_SOURCE,         /* Ingress sFlow sampled             */
    _SHR_RX_SHARED_VLAN_MISMATCH,  /* Private VLAN Mismatch             */
    _SHR_RX_SOURCE_ROUTE,          /* Source routing bit set            */
    _SHR_RX_TIME_STAMP,            /* Network time sync packet          */
    _SHR_RX_TTL,                   /* TTL <= 0 or TTL < IPMC threshold  */
    _SHR_RX_TTL1,                  /* L3UC or IPMC packet with TTL      */
                                   /* equal to 1                        */
    _SHR_RX_TUNNEL_ERROR,          /* Tunnel error trap                 */
    _SHR_RX_UDP_CHECKSUM,          /* UDP checksum                      */
    _SHR_RX_UNKNOWN_VLAN,          /* unknown VLAN; VID = 0xfff;        */
                                   /* CPU Learn bit (on 5690 devices)   */
    _SHR_RX_URPF_FAIL,             /* URPF Check Failed                 */
    _SHR_RX_VC_LABEL_MISS,         /* VPLS table miss                   */
    _SHR_RX_VLAN_FILTER_MATCH,     /* VLAN Filter Match                 */
    _SHR_RX_WLAN_CLIENT_ERROR,     /* ROC error packets to the CPU      */
    _SHR_RX_WLAN_SLOW_PATH,        /* WLAN packets slowpath to the CPU  */
    _SHR_RX_WLAN_DOT1X_DROP,       /* WLAN client is unauthenticated    */
    _SHR_RX_EXCEPTION_FLOOD,       /* Exception processing or flooding  (Robo chips) */
    _SHR_RX_TIMESYNC,              /* Time Sync protocol packet */
    _SHR_RX_EAV_DATA,              /* Ethernet AV data packet */
    _SHR_RX_SAME_PORT_BRIDGE,      /* Hairpin or Same port switching/bridging */
    _SHR_RX_SPLIT_HORIZON,         /* Basic bridging or VPLS Split horizon */
    _SHR_RX_L4_ERROR,              /* TCP/UDP header or port number errors */
    _SHR_RX_STP,                   /* STP Ingress or Egress checks      */
    _SHR_RX_EGRESS_FILTER_REDIRECT, /* Vlan egress filter redirect */
    _SHR_RX_FILTER_REDIRECT,       /* Field processor redirect */
    _SHR_RX_LOOPBACK,              /* Loopbacked  */
    _SHR_RX_VLAN_TRANSLATE,        /* VLAN translation table missed when it is expected to hit */
    _SHR_RX_MMRP,                  /* Packet of type MMRP               */
    _SHR_RX_SRP,                   /* Packet of type SRP                */
    _SHR_RX_TUNNEL_CONTROL,        /* Tunnel control packet             */
    _SHR_RX_L2_MARKED,             /* L2 table marked                   */
    _SHR_RX_WLAN_SLOWPATH_KEEPALIVE,  /* WLAN slowpath to the CPU, otherwise dropped */
    _SHR_RX_STATION,               /* MPLS sent to CPU                  */
    _SHR_RX_NIV,                   /* NIV packet                        */
    _SHR_RX_NIV_PRIO_DROP,         /* NIV packet, priority drop         */
    _SHR_RX_NIV_INTERFACE_MISS,    /* NIV packet, interface miss        */
    _SHR_RX_NIV_RPF_FAIL,          /* NIV packet, RPF failed            */
    _SHR_RX_NIV_TAG_INVALID,       /* NIV packet, invalid tag           */
    _SHR_RX_NIV_TAG_DROP,          /* NIV packet, tag drop              */
    _SHR_RX_NIV_UNTAG_DROP,        /* NIV packet, untagged drop         */
    _SHR_RX_TRILL,                 /* TRILL packet                      */
    _SHR_RX_TRILL_INVALID,         /* TRILL packet, header error        */
    _SHR_RX_TRILL_MISS,            /* TRILL packet, lookup miss         */
    _SHR_RX_TRILL_RPF_FAIL,        /* TRILL packet, RPF check failed    */
    _SHR_RX_TRILL_SLOWPATH,        /* TRILL packet, slowpath to CPU     */
    _SHR_RX_TRILL_CORE_IS_IS,      /* TRILL packet, Core IS-IS          */
    _SHR_RX_TRILL_TTL,             /* TRILL packet, TTL check failed    */
    _SHR_RX_BFD_SLOWPATH,          /* The BFD packet is being fwd to the local uC for processing */
    _SHR_RX_BFD,                   /* BFD Error                         */
    _SHR_RX_MIRROR,                /* Mirror packet                     */
    _SHR_RX_REGEX_ACTION,          /* Flow tracker                      */
    _SHR_RX_REGEX_MATCH,           /* Signature Match                   */
    _SHR_RX_FAILOVER_DROP,         /* Protection drop data              */
    _SHR_RX_WLAN_TUNNEL_ERROR,     /* WLAN shim header error to CPU     */
    _SHR_RX_CONGESTION_CNM_PROXY,  /* Congestion CNM Proxy              */
    _SHR_RX_CONGESTION_CNM_PROXY_ERROR, /* Congestion CNM Proxy Error   */
    _SHR_RX_CONGESTION_CNM,        /* Congestion CNM Internal Packet    */
    _SHR_RX_MPLS_UNKNOWN_ACH,      /* MPLS Unknown ACH                  */
    _SHR_RX_MPLS_LOOKUPS_EXCEEDED, /* MPLS out of lookups               */
    _SHR_RX_MPLS_RESERVED_ENTROPY_LABEL, /* MPLS Entropy label in unallowed range */
    _SHR_RX_MPLS_ILLEGAL_RESERVED_LABEL, /* MPLS illegal reserved label */
    _SHR_RX_MPLS_ROUTER_ALERT_LABEL, /* MPLS alert label                */
    _SHR_RX_NIV_PRUNE,             /* NIV access port pruning (dst = src) */
    _SHR_RX_VIRTUAL_PORT_PRUNE,    /* SVP == DVP                        */
    _SHR_RX_NON_UNICAST_DROP,      /* Explicit multicast packet drop    */
    _SHR_RX_TRILL_PACKET_PORT_MISMATCH, /* TRILL packet vs Rbridge port conflict */
    _SHR_RX_WLAN_CLIENT_MOVE,      /* WLAN client moved                 */
    _SHR_RX_WLAN_SOURCE_PORT_MISS, /* WLAN SVP miss                     */
    _SHR_RX_WLAN_CLIENT_SOURCE_MISS, /* WLAN client database SA miss    */
    _SHR_RX_WLAN_CLIENT_DEST_MISS,   /* WLAN client database DA miss    */
    _SHR_RX_WLAN_MTU,              /* WLAN MTU error                    */
    _SHR_RX_TRILL_NAME,            /* TRILL packet, Name check failed   */
    _SHR_RX_L2GRE_SIP_MISS,        /* L2 GRE SIP miss                   */
    _SHR_RX_L2GRE_VPN_ID_MISS,     /* L2 GRE VPN id miss                */
    _SHR_RX_TIMESYNC_UNKNOWN_VERSION, /* Unknown version of IEEE1588    */
    _SHR_RX_BFD_ERROR,             /* BFD ERROR */
    _SHR_RX_BFD_UNKNOWN_VERSION,   /* BFD UNKNOWN VERSION */
    _SHR_RX_BFD_INVALID_VERSION,   /* BFD INVALID VERSION */
    _SHR_RX_BFD_LOOKUP_FAILURE,    /* BFD LOOKUP FAILURE */
    _SHR_RX_BFD_INVALID_PACKET,    /* BFD INVALID PACKET */
    _SHR_RX_VXLAN_SIP_MISS,        /* Vxlan SIP miss                    */
    _SHR_RX_VXLAN_VPN_ID_MISS,     /* Vxlan VPN id miss                 */
    _SHR_RX_FCOE_ZONE_CHECK_FAIL,  /* Fcoe zone check failed            */
    _SHR_RX_IPMC_INTERFACE_MISMATCH, /* IPMC input interface check failed */
    _SHR_RX_NAT,                   /* NAT                               */
    _SHR_RX_TCP_UDP_NAT_MISS,      /* TCP/UDP packet NAT lookup miss    */
    _SHR_RX_ICMP_NAT_MISS,         /* ICMP packet NAT lookup miss       */
    _SHR_RX_NAT_FRAGMENT,          /* NAT lookup on fragmented packet   */
    _SHR_RX_NAT_MISS,              /* non TCP/UDP/ICMP packet NAT lookup miss */
    _SHR_RX_UNKNOWN_SUBTENTING_PORT,  /* UNKNOWN_SUBTENTING_PORT */
    _SHR_RX_LLTAG_ABSENT_DROP,  /* LLTAG_ABSENT */
    _SHR_RX_LLTAG_PRESENT_DROP,  /* LLTAG_PRESENT */
    _SHR_RX_OAM_CCM_SLOWPATH, /* OAM CCM packet copied to CPU */    
    _SHR_RX_OAM_INCOMPLETE_OPCODE, /* OAM INCOMPLETE_OPCODE */    
    _SHR_RX_REASON_COUNT           /* MUST BE LAST                      */
} _shr_rx_reason_t;

#define _SHR_RX_REASON_NAMES_INITIALIZER { \
    "Invalid",                  \
    "Arp",                      \
    "Bpdu",                     \
    "Broadcast",                \
    "ClassBasedMove",           \
    "ClassTagPackets",          \
    "Control",                  \
    "CpuLearn",                 \
    "DestLookupFail",           \
    "Dhcp",                     \
    "DosAttack",                \
    "E2eHolIbp",                \
    "EncapHiGigError",          \
    "FilterMatch",              \
    "GreChecksum",              \
    "GreSourceRoute",           \
    "HigigControl",             \
    "HigigHdrError",            \
    "IcmpRedirect",             \
    "Igmp",                     \
    "IngressFilter",            \
    "Ip",                       \
    "IpfixRateViolation",       \
    "IpMcastMiss",              \
    "IpOptionVersion",          \
    "Ipmc",                     \
    "IpmcRsvd",                 \
    "L2Cpu",                    \
    "L2DestMiss",               \
    "L2LearnLimit",             \
    "L2Move",                   \
    "L2MtuFail",                \
    "L2NonUnicastMiss",         \
    "L2SourceMiss",             \
    "L3AddrBindFail",           \
    "L3DestMiss",               \
    "L3HeaderError",            \
    "L3MtuFail",                \
    "L3Slowpath",               \
    "L3SourceMiss",             \
    "L3SourceMove",             \
    "MartianAddr",              \
    "McastIdxError",            \
    "McastMiss",                \
    "MimServiceError",          \
    "MplsCtrlWordError",        \
    "MplsError",                \
    "MplsInvalidAction",        \
    "MplsInvalidPayload",       \
    "MplsLabelMiss",            \
    "MplsSequenceNumber",       \
    "MplsTtl",                  \
    "Multicast",                \
    "Nhop",                     \
    "OamError",                 \
    "OamSlowPath",              \
    "OamLMDM",                  \
    "ParityError",              \
    "Protocol",                 \
    "SampleDest",               \
    "SampleSource",             \
    "SharedVlanMismatch",       \
    "SourceRoute",              \
    "TimeStamp",                \
    "Ttl",                      \
    "Ttl1",                     \
    "TunnelError",              \
    "UdpChecksum",              \
    "UnknownVlan",              \
    "UrpfFail",                 \
    "VcLabelMiss",              \
    "VlanFilterMatch",          \
    "WlanClientError",          \
    "WlanSlowPath",             \
    "WlanDot1xDrop",            \
    "ExceptionFlood",           \
    "Timesync",                 \
    "EavData",                  \
    "SamePortBridge",           \
    "SplitHorizon",             \
    "L4Error",                  \
    "Stp",                      \
    "EgressFilterRedirect",     \
    "FilterRedirect",           \
    "Loopback",                 \
    "VlanTranslate",            \
    "Mmrp",                     \
    "Srp",                      \
    "TunnelControl",            \
    "L2Marked",                 \
    "WlanSlowpathKeepalive",    \
    "Station",                  \
    "Niv",                      \
    "NivPrioDrop",              \
    "NivInterfaceMiss",         \
    "NivRpfFail",               \
    "NivTagInvalid",            \
    "NivTagDrop",               \
    "NivUntagDrop",             \
    "Trill",                    \
    "TrillInvalid",             \
    "TrillMiss",                \
    "TrillRpfFail",             \
    "TrillSlowpath",            \
    "TrillCoreIsIs",            \
    "TrillTtl",                 \
    "BfdSlowpath",              \
    "Bfd",                      \
    "Mirror",                   \
    "RegexAction",              \
    "RegexMatch",               \
    "FailoverDrop",             \
    "WlanTunnelError",          \
    "CongestionCnmProxy",       \
    "CongestionCnmProxyError",  \
    "CongestionCnm",            \
    "MplsUnknownAch",           \
    "MplsLookupsExceeded",      \
    "MplsReservedEntropyLabel", \
    "MplsIllegalReservedLabel", \
    "MplsRouterAlertLabel",     \
    "NivPrune",                 \
    "VirtualPortPrune",         \
    "NonUnicastDrop",           \
    "TrillPacketPortMismatch",  \
    "WlanClientMove",           \
    "WlanSourcePortMiss",       \
    "WlanClientSourceMiss",     \
    "WlanClientDestMiss",       \
    "WlanMtu",                  \
    "TrillName",                \
    "L2GreSipMiss",             \
    "L2GreVpnIdMiss",           \
    "TimesyncUnknownVersion",   \
    "BfdError",                 \
    "BfdUnknownVersion",        \
    "BfdInvalidVersion",        \
    "BfdLookupFailure",         \
    "BfdInvalidPacket",         \
    "VxlanSipMiss",             \
    "VxlanVpnIdMiss",           \
    "FcoeZoneCheckFail",        \
    "IpmcInterfaceMismatch",    \
    "Nat",                      \
    "TcpUdpNatMiss",            \
    "IcmpNatMiss",              \
    "NatFragment",              \
    "NatMiss",                  \
    "UnknownSubtentingPort",     \
    "LLTagAbsentDrop",              \
    "LLTagpresenDrop",             \
    "OAMCCMpacket",             \
    "OAMIncompleteOpcode",      \
}

/*
 * Set of "reasons" (see _SHR_RX_*) why a packet came to the CPU.
 */
typedef struct _shr_rx_reasons_s {
    SHR_BITDCL  pbits[_SHR_BITDCLSIZE(_SHR_RX_REASON_COUNT)];
} _shr_rx_reasons_t;

/*
 * Macro to check if a reason (_SHR_RX_*) is included in a
 * set of reasons (_shr_rx_reasons_t). Returns:
 *   zero     => reason is not included in the set
 *   non-zero => reason is included in the set
 */
#define _SHR_RX_REASON_GET(_reasons, _reason) \
        SHR_BITGET(((_reasons).pbits), (_reason))

/*
 * Macro to add a reason (_SHR_RX_*) to a set of
 * reasons (_shr_rx_reasons_t)
 */
#define _SHR_RX_REASON_SET(_reasons, _reason) \
        SHR_BITSET(((_reasons).pbits), (_reason))

/*
 * Macro to add all reasons (_SHR_RX_*) to a set of
 * reasons (_shr_rx_reasons_t)
 */
#define _SHR_RX_REASON_SET_ALL(_reasons) \
        SHR_BITSET_RANGE(((_reasons).pbits), 0, _SHR_RX_REASON_COUNT)

/*
 * Macro to clear a reason (_SHR_RX_*) from a set of
 * reasons (_shr_rx_reasons_t)
 */
#define _SHR_RX_REASON_CLEAR(_reasons, _reason) \
        SHR_BITCLR(((_reasons).pbits), (_reason))

/*
 * Macro to clear a set of reasons (_shr_rx_reasons_t).
 */
#define _SHR_RX_REASON_CLEAR_ALL(_reasons) \
        SHR_BITCLR_RANGE(((_reasons).pbits), 0, _SHR_RX_REASON_COUNT)

#define _SHR_RX_REASON_IS_NULL(_reasons) \
        SHR_BITNULL_RANGE(((_reasons).pbits), \
                          0, _SHR_RX_REASON_COUNT)
#define _SHR_RX_REASON_ITER(_reasons, reason) \
    for(reason = _SHR_RX_INVALID; reason < (int)_SHR_RX_REASON_COUNT; reason++) \
        if(_SHR_RX_REASON_GET(_reasons, reason))

#define _SHR_RX_REASON_COUNT(_reasons, _count) \
        SHR_BITCOUNT_RANGE(((_reasons).pbits), _count, \
                           0, _SHR_RX_REASON_COUNT)
#define _SHR_RX_REASON_EQ(_reasons1, _reasons2) \
        SHR_BITEQ_RANGE(((_reasons1).pbits), ((_reasons2).pbits), \
                        0, _SHR_RX_REASON_COUNT)
#define _SHR_RX_REASON_NEQ(_reasons1, _reasons2) \
        (!SHR_BITEQ_RANGE(((_reasons1).pbits), ((_reasons2).pbits), \
                          0, _SHR_RX_REASON_COUNT))
#define _SHR_RX_REASON_AND(_reasons1, _reasons2) \
        SHR_BITAND_RANGE(((_reasons1).pbits), ((_reasons2).pbits), 0, \
                          _SHR_RX_REASON_COUNT, ((_reasons1).pbits))
#define _SHR_RX_REASON_OR(_reasons1, _reasons2) \
        SHR_BITOR_RANGE(((_reasons1).pbits), ((_reasons2).pbits), 0, \
                         _SHR_RX_REASON_COUNT, ((_reasons1).pbits))
#define _SHR_RX_REASON_XOR(_reasons1, _reasons2) \
        SHR_BITXOR_RANGE(((_reasons1).pbits), ((_reasons2).pbits), 0, \
                         _SHR_RX_REASON_COUNT, ((_reasons1).pbits))
#define _SHR_RX_REASON_REMOVE(_reasons1, _reasons2) \
        SHR_BITREMOVE_RANGE(((_reasons1).pbits), ((_reasons2).pbits), 0, \
                             _SHR_RX_REASON_COUNT, ((_reasons1).pbits));
#define _SHR_RX_REASON_NEGATE(_reasons1, _reasons2) \
        SHR_BITNEGATE_RANGE(((_reasons2).pbits), 0, \
                             _SHR_RX_REASON_COUNT, ((_reasons1).pbits));

typedef enum _shr_rx_redirect_e { 
    _SHR_RX_REDIRECT_NORMAL, 
    _SHR_RX_REDIRECT_HIGIG, 
    _SHR_RX_REDIRECT_TRUNCATED,
    _SHR_RX_REDIRECT_MAX = _SHR_RX_REDIRECT_TRUNCATED
} _shr_rx_redirect_e; 

#endif /* _SHR_RX_H_ */
