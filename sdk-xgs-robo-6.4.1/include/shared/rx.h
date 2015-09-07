/*
 * $Id: rx.h,v 1.35 Broadcom SDK $
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
    _SHR_RX_INVALID         = 0,
    _SHR_RX_ARP             = 1,                 /* ARP Packet                        */                         
    _SHR_RX_BPDU            = 2,                 /* BPDU Packet                       */                         
    _SHR_RX_BROADCAST       = 3,                 /* Broadcast packet                  */                         
    _SHR_RX_CLASS_BASED_MOVE    = 4,             /* Class-based move prevented        */                         
    _SHR_RX_CLASS_TAG_PACKETS   = 5,             /* Higig Header with PPD=1           */                         
    _SHR_RX_CONTROL         = 6,                 /* Control frame or reserved addr    */                         
    _SHR_RX_CPU_LEARN       = 7,                 /* CPU Learned (or VLAN not found    */                         
                                                 /* on Strata devices)                */                         
    _SHR_RX_DEST_LOOKUP_FAIL    = 8,             /* Destination lookup fail (or L3    */                         
                                                 /* station move on Strata devices)   */                         
    _SHR_RX_DHCP            = 9,                 /* DHCP packets                      */                         
    _SHR_RX_DOS_ATTACK      = 10,                /* DOS Attack Packet                 */                         
    _SHR_RX_E2E_HOL_IBP     = 11,                /* E2E HOL or IBP Packet             */                         
    _SHR_RX_ENCAP_HIGIG_ERROR   = 12,            /* Non-HG packets received on an E-HG port */                   
    _SHR_RX_FILTER_MATCH    = 13,                /* Filter Match                      */                         
    _SHR_RX_GRE_CHECKSUM    = 14,                /* GRE checksum                      */                         
    _SHR_RX_GRE_SOURCE_ROUTE    = 15,            /* GRE source routing                */                         
    _SHR_RX_HIGIG_CONTROL   = 16,                /* Higig Packet with Control Opcode  */                         
    _SHR_RX_HIGIG_HDR_ERROR = 17,                /* Higig+ header errors              */                         
    _SHR_RX_ICMP_REDIRECT   = 18,                /* ICMP Recirect                     */                         
    _SHR_RX_IGMP            = 19,                /* IGMP packet                       */                         
    _SHR_RX_INGRESS_FILTER  = 20,                /* Ingress Filter (VLAN membership)  */                         
    _SHR_RX_IP              = 21,                /* IP packet                         */                         
    _SHR_RX_IPFIX_RATE_VIOLATION = 22,           /* IPFIX flows exceed metering       */                         
    _SHR_RX_IP_MCAST_MISS   = 23,                /* IPMC miss                         */                         
    _SHR_RX_IP_OPTION_VERSION   = 24,            /* IP options present or IP ver != 4 */                         
    _SHR_RX_IPMC                = 25,            /* Class D IP multicast packet       */                         
    _SHR_RX_IPMC_RSVD           = 26,            /* IPMC Reserved Packet              */                         
    _SHR_RX_L2_CPU              = 27,            /* L2_TABLE - copy to CPU (on 5690)  */                         
    _SHR_RX_L2_DEST_MISS        = 28,            /* L2 destination lookup failure     */                         
    _SHR_RX_L2_LEARN_LIMIT      = 29,            /* L2 Learn Limit                    */                         
    _SHR_RX_L2_MOVE             = 30,            /* L2 Station Movement               */                         
    _SHR_RX_L2_MTU_FAIL         = 31,            /* L2 MTU check fail                 */                         
    _SHR_RX_L2_NON_UNICAST_MISS = 32,            /* L2 Non-Unicast lookup miss        */                         
    _SHR_RX_L2_SOURCE_MISS      = 33,            /* L2 source lookup failure          */                         
    _SHR_RX_L3_ADDR_BIND_FAIL   = 34,            /* MAC to IP bind check failure      */                         
    _SHR_RX_L3_DEST_MISS        = 35,            /* L3 DIP Miss                       */                         
    _SHR_RX_L3_HEADER_ERROR     = 36,            /* L3 header - IP options,           */                         
    _SHR_RX_L3_MTU_FAIL         = 37,            /* L3 MTU check fail                 */                         
    _SHR_RX_L3_SLOW_PATH        = 38,            /* L3 slow path processed pkt.       */                         
    _SHR_RX_L3_SOURCE_MISS      = 39,            /* L3 SIP Miss                       */                         
    _SHR_RX_L3_SOUCE_MOVE       = 40,            /* L3 Station Movement               */                         
    _SHR_RX_MARTIAN_ADDR        = 41,            /* Pkt. with Martian address         */                         
    _SHR_RX_MCAST_IDX_ERROR     = 42,            /* Multicast index error             */                         
    _SHR_RX_MCAST_MISS          = 43,            /* MC miss                           */                         
    _SHR_RX_MIM_SERVICE_ERROR   = 44,            /* MAC-in-MAC terminated unicast packets */                     
                                                 /* that do not have a valid I-SID    */                         
    _SHR_RX_MPLS_CTRL_WORD_ERROR= 45,            /* MPLS Control Word type is not zero */                        
    _SHR_RX_MPLS_ERROR          = 46,            /* MPLS error                        */                         
    _SHR_RX_MPLS_INVALID_ACTION = 47,            /* MPLS Invalid Action               */                         
    _SHR_RX_MPLS_INVALID_PAYLOAD= 48,            /* MPLS Invalid Payload              */                         
    _SHR_RX_MPLS_LABEL_MISS     = 49,            /* MPLS table miss                   */                         
    _SHR_RX_MPLS_SEQUENCE_NUMBER= 50,            /* MPLS Sequence number              */                         
    _SHR_RX_MPLS_TTL            = 51,            /* MPLS TTL                          */                         
    _SHR_RX_MULTICAST           = 52,            /* Multicast packet                  */                         
    _SHR_RX_NHOP                = 53,            /* Copy to CPU from Next Hop Idx Tbl */                         
    _SHR_RX_OAM_ERROR           = 54,            /* OAM packets to CPU for error cases */                        
    _SHR_RX_OAM_SLOW_PATH       = 55,            /* OAM packets to CPU - slowpath process */                     
    _SHR_RX_OAM_LMDM            = 56,            /* OAM LMM/LMR, DMM/DMR packets to CPU */                       
    _SHR_RX_PARITY_ERROR        = 57,            /* Parity error on IP tables         */                         
    _SHR_RX_PROTOCOL            = 58,            /* Protocol Packet                   */                         
    _SHR_RX_SAMPLE_DEST         = 59,            /* Egress  sFlow sampled             */                         
    _SHR_RX_SAMPLE_SOURCE       = 60,            /* Ingress sFlow sampled             */                         
    _SHR_RX_SHARED_VLAN_MISMATCH= 61,            /* Private VLAN Mismatch             */                         
    _SHR_RX_SOURCE_ROUTE        = 62,            /* Source routing bit set            */                         
    _SHR_RX_TIME_STAMP          = 63,            /* Network time sync packet          */                         
    _SHR_RX_TTL                 = 64,            /* TTL <= 0 or TTL < IPMC threshold  */                         
    _SHR_RX_TTL1                = 65,            /* L3UC or IPMC packet with TTL      */                         
                                                 /* equal to 1                        */                         
    _SHR_RX_TUNNEL_ERROR        = 66,            /* Tunnel error trap                 */                         
    _SHR_RX_UDP_CHECKSUM        = 67,            /* UDP checksum                      */                         
    _SHR_RX_UNKNOWN_VLAN        = 68,            /* unknown VLAN; VID = 0xfff;        */                         
                                                 /* CPU Learn bit (on 5690 devices)   */                         
    _SHR_RX_URPF_FAIL           = 69,            /* URPF Check Failed                 */                         
    _SHR_RX_VC_LABEL_MISS       = 70,            /* VPLS table miss                   */                         
    _SHR_RX_VLAN_FILTER_MATCH   = 71,            /* VLAN Filter Match                 */                         
    _SHR_RX_WLAN_CLIENT_ERROR   = 72,            /* ROC error packets to the CPU      */                         
    _SHR_RX_WLAN_SLOW_PATH      = 73,            /* WLAN packets slowpath to the CPU  */                         
    _SHR_RX_WLAN_DOT1X_DROP     = 74,            /* WLAN client is unauthenticated    */                         
    _SHR_RX_EXCEPTION_FLOOD     = 75,            /* Exception processing or flooding  (Robo chips) */            
    _SHR_RX_TIMESYNC            = 76,            /* Time Sync protocol packet */                                 
    _SHR_RX_EAV_DATA            = 77,            /* Ethernet AV data packet */                                   
    _SHR_RX_SAME_PORT_BRIDGE    = 78,            /* Hairpin or Same port switching/bridging */                   
    _SHR_RX_SPLIT_HORIZON       = 79,            /* Basic bridging or VPLS Split horizon */                      
    _SHR_RX_L4_ERROR            = 80,            /* TCP/UDP header or port number errors */                      
    _SHR_RX_STP                 = 81,            /* STP Ingress or Egress checks      */                         
    _SHR_RX_EGRESS_FILTER_REDIRECT= 82,          /* Vlan egress filter redirect */                               
    _SHR_RX_FILTER_REDIRECT     = 83,            /* Field processor redirect */                                  
    _SHR_RX_LOOPBACK            = 84,            /* Loopbacked  */                                               
    _SHR_RX_VLAN_TRANSLATE      = 85,            /* VLAN translation table missed when it is expected to hit */  
    _SHR_RX_MMRP                = 86,            /* Packet of type MMRP               */                         
    _SHR_RX_SRP                 = 87,            /* Packet of type SRP                */                         
    _SHR_RX_TUNNEL_CONTROL      = 88,            /* Tunnel control packet             */                         
    _SHR_RX_L2_MARKED           = 89,            /* L2 table marked                   */                         
    _SHR_RX_WLAN_SLOWPATH_KEEPALIVE = 90,        /* WLAN slowpath to the CPU, otherwise dropped */               
    _SHR_RX_STATION             = 91,            /* MPLS sent to CPU                  */                         
    _SHR_RX_NIV                 = 92,            /* NIV packet                        */                         
    _SHR_RX_NIV_PRIO_DROP       = 93,            /* NIV packet, priority drop         */                         
    _SHR_RX_NIV_INTERFACE_MISS  = 94,            /* NIV packet, interface miss        */                         
    _SHR_RX_NIV_RPF_FAIL        = 95,            /* NIV packet, RPF failed            */                         
    _SHR_RX_NIV_TAG_INVALID     = 96,            /* NIV packet, invalid tag           */                         
    _SHR_RX_NIV_TAG_DROP        = 97,            /* NIV packet, tag drop              */                         
    _SHR_RX_NIV_UNTAG_DROP      = 98,            /* NIV packet, untagged drop         */                         
    _SHR_RX_TRILL               = 99,            /* TRILL packet                      */                         
    _SHR_RX_TRILL_INVALID       = 100,           /* TRILL packet, header error        */                         
    _SHR_RX_TRILL_MISS          = 101,           /* TRILL packet, lookup miss         */                         
    _SHR_RX_TRILL_RPF_FAIL      = 102,           /* TRILL packet, RPF check failed    */                         
    _SHR_RX_TRILL_SLOWPATH      = 103,           /* TRILL packet, slowpath to CPU     */                         
    _SHR_RX_TRILL_CORE_IS_IS    = 104,           /* TRILL packet, Core IS-IS          */                         
    _SHR_RX_TRILL_TTL           = 105,           /* TRILL packet, TTL check failed    */                         
    _SHR_RX_BFD_SLOWPATH        = 106,           /* The BFD packet is being fwd to the local uC for processing  */ 
    _SHR_RX_BFD                 = 107,           /* BFD Error                         */                         
    _SHR_RX_MIRROR              = 108,           /* Mirror packet                     */                         
    _SHR_RX_REGEX_ACTION        = 109,           /* Flow tracker                      */                         
    _SHR_RX_REGEX_MATCH         = 110,           /* Signature Match                   */                         
    _SHR_RX_FAILOVER_DROP       = 111,           /* Protection drop data              */                         
    _SHR_RX_WLAN_TUNNEL_ERROR   = 112,           /* WLAN shim header error to CPU     */                         
    _SHR_RX_CONGESTION_CNM_PROXY= 113,           /* Congestion CNM Proxy              */                         
    _SHR_RX_CONGESTION_CNM_PROXY_ERROR = 114,    /* Congestion CNM Proxy Error   */                              
    _SHR_RX_CONGESTION_CNM      = 115,           /* Congestion CNM Internal Packet    */                         
    _SHR_RX_MPLS_UNKNOWN_ACH    = 116,           /* MPLS Unknown ACH                  */                         
    _SHR_RX_MPLS_LOOKUPS_EXCEEDED = 117,         /* MPLS out of lookups               */                         
    _SHR_RX_MPLS_RESERVED_ENTROPY_LABEL = 118,   /* MPLS Entropy label in unallowed range */                     
    _SHR_RX_MPLS_ILLEGAL_RESERVED_LABEL = 119,   /* MPLS illegal reserved label */                               
    _SHR_RX_MPLS_ROUTER_ALERT_LABEL = 120,       /* MPLS alert label                */                           
    _SHR_RX_NIV_PRUNE           = 121,           /* NIV access port pruning (dst = src) */                       
    _SHR_RX_VIRTUAL_PORT_PRUNE  = 122,           /* SVP == DVP                        */                         
    _SHR_RX_NON_UNICAST_DROP    = 123,           /* Explicit multicast packet drop    */                         
    _SHR_RX_TRILL_PACKET_PORT_MISMATCH = 124,    /* TRILL packet vs Rbridge port conflict */                     
    _SHR_RX_WLAN_CLIENT_MOVE    = 125,           /* WLAN client moved                 */                         
    _SHR_RX_WLAN_SOURCE_PORT_MISS = 126,         /* WLAN SVP miss                     */                         
    _SHR_RX_WLAN_CLIENT_SOURCE_MISS = 127,       /* WLAN client database SA miss    */                           
    _SHR_RX_WLAN_CLIENT_DEST_MISS = 128,         /* WLAN client database DA miss    */                           
    _SHR_RX_WLAN_MTU            = 129,           /* WLAN MTU error                    */                         
    _SHR_RX_TRILL_NAME          = 130,           /* TRILL packet, Name check failed   */                         
    _SHR_RX_L2GRE_SIP_MISS      = 131,           /* L2 GRE SIP miss                   */                         
    _SHR_RX_L2GRE_VPN_ID_MISS   = 132,           /* L2 GRE VPN id miss                */                         
    _SHR_RX_TIMESYNC_UNKNOWN_VERSION = 133,      /* Unknown version of IEEE1588    */                            
    _SHR_RX_BFD_ERROR           = 134,           /* BFD ERROR */                                                 
    _SHR_RX_BFD_UNKNOWN_VERSION = 135,           /* BFD UNKNOWN VERSION */                                       
    _SHR_RX_BFD_INVALID_VERSION = 136,           /* BFD INVALID VERSION */                                       
    _SHR_RX_BFD_LOOKUP_FAILURE  = 137,           /* BFD LOOKUP FAILURE */                                        
    _SHR_RX_BFD_INVALID_PACKET  = 138,           /* BFD INVALID PACKET */                                        
    _SHR_RX_VXLAN_SIP_MISS      = 139,           /* Vxlan SIP miss                    */                         
    _SHR_RX_VXLAN_VPN_ID_MISS   = 140,           /* Vxlan VPN id miss                 */                         
    _SHR_RX_FCOE_ZONE_CHECK_FAIL= 141,           /* Fcoe zone check failed            */                         
    _SHR_RX_IPMC_INTERFACE_MISMATCH = 142,       /* IPMC input interface check failed */                         
    _SHR_RX_NAT                 = 143,           /* NAT                               */                         
    _SHR_RX_TCP_UDP_NAT_MISS    = 144,           /* TCP/UDP packet NAT lookup miss    */                         
    _SHR_RX_ICMP_NAT_MISS       = 145,           /* ICMP packet NAT lookup miss       */                         
    _SHR_RX_NAT_FRAGMENT        = 146,           /* NAT lookup on fragmented packet   */                         
    _SHR_RX_NAT_MISS            = 147,           /* non TCP/UDP/ICMP packet NAT lookup miss */                   
    _SHR_RX_UNKNOWN_SUBTENTING_PORT = 148,       /* UNKNOWN_SUBTENTING_PORT */                                   
    _SHR_RX_LLTAG_ABSENT_DROP   = 149,           /* LLTAG_ABSENT */                                              
    _SHR_RX_LLTAG_PRESENT_DROP  = 150,           /* LLTAG_PRESENT */                                             
    _SHR_RX_OAM_CCM_SLOWPATH    = 151,           /* OAM CCM packet copied to CPU */                              
    _SHR_RX_OAM_INCOMPLETE_OPCODE = 152,         /* OAM INCOMPLETE_OPCODE */   
    _SHR_RX_BHH_OAM_PACKET = 153,                /* BHH OAM Packet                    */                                  
    _SHR_RX_REASON_COUNT = 154                   /* MUST BE LAST                      */                         
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
    "OAMCCMslowpath",             \
    "OAMIncompleteOpcode",      \
    "OAMCCMpacket",             \
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
    _SHR_RX_REDIRECT_NORMAL = 0, 
    _SHR_RX_REDIRECT_HIGIG  = 1, 
    _SHR_RX_REDIRECT_TRUNCATED = 2,
    _SHR_RX_REDIRECT_MAX = _SHR_RX_REDIRECT_TRUNCATED
} _shr_rx_redirect_e; 


typedef enum _shr_rx_decap_tunnel_e {
    _SHR_RX_DECAP_NONE = 0,             /* No tunnel Decap */
    _SHR_RX_DECAP_ACCESS_SVP = 1,       /* Packet ingress on Access SVP (No decap) */
    _SHR_RX_DECAP_MIM = 2,              /* Decap Mac-in-Mac tunnel */
    _SHR_RX_DECAP_L2GRE = 3,            /* Decap L2GRE tunnel */
    _SHR_RX_DECAP_VXLAN = 4,            /* Decap VXLAN tunnel */
    _SHR_RX_DECAP_AMT = 5,              /* Decap AMT tunnel */
    _SHR_RX_DECAP_IP = 6,               /* Decap IP tunnel */
    _SHR_RX_DECAP_TRILL = 7,            /* Decap TRILL tunnel */
    _SHR_RX_DECAP_L2MPLS_1LABEL = 8,    /* Decap MPLS 1 Label, L2 payload, no
                                           Control Word */
    _SHR_RX_DECAP_L2MPLS_2LABEL = 9,    /* Decap MPLS 2 Label, L2 payload, no
                                           Control Word */
    _SHR_RX_DECAP_L2MPLS_1LABELCW = 10, /* Decap MPLS 1 Label, L2 payload, Control
                                           Word present */
    _SHR_RX_DECAP_L2MPLS_2LABELCW = 11, /* Decap MPLS 2 Label, L2 payload, Control
                                            Word present */
    _SHR_RX_DECAP_L3MPLS_1LABEL = 12,   /* Decap MPLS 1 Label, L3 payload, no
                                           Control Word present */
    _SHR_RX_DECAP_L3MPLS_2LABEL = 13,   /* Decap MPLS 2 Label, L3 payload, no
                                           Control Word present */
    _SHR_RX_DECAP_L3MPLS_1LABELCW = 14, /* Decap MPLS 1 Label, L3 payload, Control
                                           Word present */
    _SHR_RX_DECAP_L3MPLS_2LABELCW = 15, /* Decap MPLS 2 Label, L3 payload, Control
                                           Word present */
    _SHR_RX_DECAP_WTP2AC = 16,          /* Decap WTP2AC Tunnel */
    _SHR_RX_DECAP_AC2AC = 17,           /* Decap AC2AC Tunnel */
    _SHR_RX_DECAP_COUNT = 18            /* Decap Tunnel Max */
} _shr_rx_decap_tunnel_t;

#endif /* _SHR_RX_H_ */
