/*
 * $Id: rx.h,v 1.16 Broadcom SDK $
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

#ifndef _SOC_RX_H
#define _SOC_RX_H

#include <sal/types.h>

#include <shared/rx.h>

/*
 * PKT RX Packet Reasons; reason CPU received the packet.
 * Notes:
 *      It is possible no reasons are set (directed to CPU from ARL
 *      for example), or multiple reasons may be set.
 *
 * See "include/shared/rx.h" for full description.
 */

typedef enum soc_rx_reason_e {
    socRxReasonInvalid = _SHR_RX_INVALID, 
    socRxReasonArp = _SHR_RX_ARP, 
    socRxReasonBpdu = _SHR_RX_BPDU, 
    socRxReasonBroadcast = _SHR_RX_BROADCAST, 
    socRxReasonClassBasedMove = _SHR_RX_CLASS_BASED_MOVE, 
    socRxReasonClassTagPackets = _SHR_RX_CLASS_TAG_PACKETS, 
    socRxReasonControl = _SHR_RX_CONTROL, 
    socRxReasonCpuLearn = _SHR_RX_CPU_LEARN, 
    socRxReasonDestLookupFail = _SHR_RX_DEST_LOOKUP_FAIL, 
    socRxReasonDhcp = _SHR_RX_DHCP, 
    socRxReasonDosAttack = _SHR_RX_DOS_ATTACK, 
    socRxReasonE2eHolIbp = _SHR_RX_E2E_HOL_IBP, 
    socRxReasonEncapHigigError = _SHR_RX_ENCAP_HIGIG_ERROR, 
    socRxReasonFilterMatch = _SHR_RX_FILTER_MATCH, 
    socRxReasonGreChecksum = _SHR_RX_GRE_CHECKSUM, 
    socRxReasonGreSourceRoute = _SHR_RX_GRE_SOURCE_ROUTE, 
    socRxReasonHigigControl = _SHR_RX_HIGIG_CONTROL, 
    socRxReasonHigigHdrError = _SHR_RX_HIGIG_HDR_ERROR, 
    socRxReasonIcmpRedirect = _SHR_RX_ICMP_REDIRECT, 
    socRxReasonIgmp = _SHR_RX_IGMP, 
    socRxReasonIngressFilter = _SHR_RX_INGRESS_FILTER, 
    socRxReasonIp = _SHR_RX_IP, 
    socRxReasonIpfixRateViolation = _SHR_RX_IPFIX_RATE_VIOLATION, 
    socRxReasonIpMcastMiss = _SHR_RX_IP_MCAST_MISS, 
    socRxReasonIpmcReserved = _SHR_RX_IPMC_RSVD, 
    socRxReasonIpOptionVersion = _SHR_RX_IP_OPTION_VERSION, 
    socRxReasonIpmc = _SHR_RX_IPMC, 
    socRxReasonL2Cpu = _SHR_RX_L2_CPU, 
    socRxReasonL2DestMiss = _SHR_RX_L2_DEST_MISS, 
    socRxReasonL2LearnLimit = _SHR_RX_L2_LEARN_LIMIT, 
    socRxReasonL2Move = _SHR_RX_L2_MOVE, 
    socRxReasonL2MtuFail = _SHR_RX_L2_MTU_FAIL, 
    socRxReasonL2NonUnicastMiss = _SHR_RX_L2_NON_UNICAST_MISS, 
    socRxReasonL2SourceMiss = _SHR_RX_L2_SOURCE_MISS, 
    socRxReasonL3AddrBindFail = _SHR_RX_L3_ADDR_BIND_FAIL, 
    socRxReasonL3DestMiss = _SHR_RX_L3_DEST_MISS, 
    socRxReasonL3HeaderError = _SHR_RX_L3_HEADER_ERROR, 
    socRxReasonL3MtuFail = _SHR_RX_L3_MTU_FAIL, 
    socRxReasonL3Slowpath = _SHR_RX_L3_SLOW_PATH, 
    socRxReasonL3SourceMiss = _SHR_RX_L3_SOURCE_MISS, 
    socRxReasonL3SourceMove = _SHR_RX_L3_SOUCE_MOVE, 
    socRxReasonMartianAddr = _SHR_RX_MARTIAN_ADDR, 
    socRxReasonMcastIdxError = _SHR_RX_MCAST_IDX_ERROR, 
    socRxReasonMcastMiss = _SHR_RX_MCAST_MISS, 
    socRxReasonMimServiceError = _SHR_RX_MIM_SERVICE_ERROR, 
    socRxReasonMplsCtrlWordError = _SHR_RX_MPLS_CTRL_WORD_ERROR, 
    socRxReasonMplsError = _SHR_RX_MPLS_ERROR, 
    socRxReasonMplsInvalidAction = _SHR_RX_MPLS_INVALID_ACTION, 
    socRxReasonMplsInvalidPayload = _SHR_RX_MPLS_INVALID_PAYLOAD, 
    socRxReasonMplsLabelMiss = _SHR_RX_MPLS_LABEL_MISS, 
    socRxReasonMplsSequenceNumber = _SHR_RX_MPLS_SEQUENCE_NUMBER, 
    socRxReasonMplsTtl = _SHR_RX_MPLS_TTL, 
    socRxReasonMulticast = _SHR_RX_MULTICAST, 
    socRxReasonNhop = _SHR_RX_NHOP, 
    socRxReasonOAMError = _SHR_RX_OAM_ERROR, 
    socRxReasonOAMSlowpath = _SHR_RX_OAM_SLOW_PATH, 
    socRxReasonOAMLMDM = _SHR_RX_OAM_LMDM, 
    socRxReasonParityError = _SHR_RX_PARITY_ERROR, 
    socRxReasonProtocol = _SHR_RX_PROTOCOL, 
    socRxReasonSampleDest = _SHR_RX_SAMPLE_DEST, 
    socRxReasonSampleSource = _SHR_RX_SAMPLE_SOURCE, 
    socRxReasonSharedVlanMismatch = _SHR_RX_SHARED_VLAN_MISMATCH, 
    socRxReasonSourceRoute = _SHR_RX_SOURCE_ROUTE, 
    socRxReasonTimeStamp = _SHR_RX_TIME_STAMP, 
    socRxReasonTtl = _SHR_RX_TTL, 
    socRxReasonTtl1 = _SHR_RX_TTL1, 
    socRxReasonTunnelError = _SHR_RX_TUNNEL_ERROR, 
    socRxReasonUdpChecksum = _SHR_RX_UDP_CHECKSUM, 
    socRxReasonUnknownVlan = _SHR_RX_UNKNOWN_VLAN, 
    socRxReasonUrpfFail = _SHR_RX_URPF_FAIL, 
    socRxReasonVcLabelMiss = _SHR_RX_VC_LABEL_MISS, 
    socRxReasonVlanFilterMatch = _SHR_RX_VLAN_FILTER_MATCH, 
    socRxReasonWlanClientError = _SHR_RX_WLAN_CLIENT_ERROR, 
    socRxReasonWlanSlowpath = _SHR_RX_WLAN_SLOW_PATH, 
    socRxReasonWlanDot1xDrop = _SHR_RX_WLAN_DOT1X_DROP, 
    socRxReasonExceptionFlood = _SHR_RX_EXCEPTION_FLOOD, 
    socRxReasonTimeSync = _SHR_RX_TIMESYNC, 
    socRxReasonEAVData = _SHR_RX_EAV_DATA, 
    socRxReasonSamePortBridge = _SHR_RX_SAME_PORT_BRIDGE, 
    socRxReasonSplitHorizon = _SHR_RX_SPLIT_HORIZON, 
    socRxReasonL4Error = _SHR_RX_L4_ERROR, 
    socRxReasonStp = _SHR_RX_STP, 
    socRxReasonEgressFilterRedirect = _SHR_RX_EGRESS_FILTER_REDIRECT, 
    socRxReasonFilterRedirect = _SHR_RX_FILTER_REDIRECT, 
    socRxReasonLoopback = _SHR_RX_LOOPBACK,
    socRxReasonVlanTranslate = _SHR_RX_VLAN_TRANSLATE, 
    socRxReasonMmrp = _SHR_RX_MMRP, 
    socRxReasonSrp = _SHR_RX_SRP, 
    socRxReasonTunnelControl = _SHR_RX_TUNNEL_CONTROL, 
    socRxReasonL2Marked = _SHR_RX_L2_MARKED, 
    socRxReasonWlanSlowpathKeepalive = _SHR_RX_WLAN_SLOWPATH_KEEPALIVE, 
    socRxReasonStation = _SHR_RX_STATION, 
    socRxReasonNiv = _SHR_RX_NIV, 
    socRxReasonNivPrioDrop = _SHR_RX_NIV_PRIO_DROP, 
    socRxReasonNivInterfaceMiss = _SHR_RX_NIV_INTERFACE_MISS, 
    socRxReasonNivRpfFail = _SHR_RX_NIV_RPF_FAIL, 
    socRxReasonNivTagInvalid = _SHR_RX_NIV_TAG_INVALID, 
    socRxReasonNivTagDrop = _SHR_RX_NIV_TAG_DROP, 
    socRxReasonNivUntagDrop = _SHR_RX_NIV_UNTAG_DROP, 
    socRxReasonTrill = _SHR_RX_TRILL, 
    socRxReasonTrillInvalid = _SHR_RX_TRILL_INVALID, 
    socRxReasonTrillMiss = _SHR_RX_TRILL_MISS, 
    socRxReasonTrillRpfFail = _SHR_RX_TRILL_RPF_FAIL, 
    socRxReasonTrillSlowpath = _SHR_RX_TRILL_SLOWPATH, 
    socRxReasonTrillCoreIsIs = _SHR_RX_TRILL_CORE_IS_IS, 
    socRxReasonTrillTtl = _SHR_RX_TRILL_TTL, 
    socRxReasonTrillName = _SHR_RX_TRILL_NAME, 
    socRxReasonBfdSlowpath = _SHR_RX_BFD_SLOWPATH, 
    socRxReasonBfd = _SHR_RX_BFD, 
    socRxReasonMirror = _SHR_RX_MIRROR, 
    socRxReasonRegexAction = _SHR_RX_REGEX_ACTION, 
    socRxReasonFailoverDrop = _SHR_RX_FAILOVER_DROP, 
    socRxReasonWlanTunnelError = _SHR_RX_WLAN_TUNNEL_ERROR, 
    socRxReasonMplsReservedEntropyLabel = \
                                      _SHR_RX_MPLS_RESERVED_ENTROPY_LABEL, 
    socRxReasonCongestionCnmProxy = _SHR_RX_CONGESTION_CNM_PROXY,
    socRxReasonCongestionCnmProxyError = _SHR_RX_CONGESTION_CNM_PROXY_ERROR,
    socRxReasonCongestionCnm = _SHR_RX_CONGESTION_CNM,
    socRxReasonMplsUnknownAch = _SHR_RX_MPLS_UNKNOWN_ACH, 
    socRxReasonMplsLookupsExceeded = _SHR_RX_MPLS_LOOKUPS_EXCEEDED, 
    socRxReasonMplsIllegalReservedLabel = \
                                      _SHR_RX_MPLS_ILLEGAL_RESERVED_LABEL, 
    socRxReasonMplsRouterAlertLabel = _SHR_RX_MPLS_ROUTER_ALERT_LABEL, 
    socRxReasonNivPrune = _SHR_RX_NIV_PRUNE, 
    socRxReasonVirtualPortPrune = _SHR_RX_VIRTUAL_PORT_PRUNE, 
    socRxReasonNonUnicastDrop = _SHR_RX_NON_UNICAST_DROP, 
    socRxReasonTrillPacketPortMismatch = _SHR_RX_TRILL_PACKET_PORT_MISMATCH, 
    socRxReasonRegexMatch = _SHR_RX_REGEX_MATCH, 
    socRxReasonWlanClientMove = _SHR_RX_WLAN_CLIENT_MOVE, 
    socRxReasonWlanSourcePortMiss = _SHR_RX_WLAN_SOURCE_PORT_MISS, 
    socRxReasonWlanClientSourceMiss = _SHR_RX_WLAN_CLIENT_SOURCE_MISS, 
    socRxReasonWlanClientDestMiss = _SHR_RX_WLAN_CLIENT_DEST_MISS, 
    socRxReasonWlanMtu = _SHR_RX_WLAN_MTU, 
    socRxReasonL2GreSipMiss = _SHR_RX_L2GRE_SIP_MISS, 
    socRxReasonL2GreVpnIdMiss = _SHR_RX_L2GRE_VPN_ID_MISS, 
    socRxReasonTimesyncUnknownVersion = _SHR_RX_TIMESYNC_UNKNOWN_VERSION,
    socRxReasonVxlanSipMiss = _SHR_RX_VXLAN_SIP_MISS, 
    socRxReasonVxlanVpnIdMiss = _SHR_RX_VXLAN_VPN_ID_MISS, 
    socRxReasonFcoeZoneCheckFail = _SHR_RX_FCOE_ZONE_CHECK_FAIL, 
    socRxReasonIpmcInterfaceMismatch = _SHR_RX_IPMC_INTERFACE_MISMATCH, 
    socRxReasonNat = _SHR_RX_NAT, 
    socRxReasonTcpUdpNatMiss = _SHR_RX_TCP_UDP_NAT_MISS, 
    socRxReasonIcmpNatMiss = _SHR_RX_ICMP_NAT_MISS, 
    socRxReasonNatFragment = _SHR_RX_NAT_FRAGMENT, 
    socRxReasonNatMiss = _SHR_RX_NAT_MISS,
    socRxReasonUnknownSubtendingPort = _SHR_RX_UNKNOWN_SUBTENTING_PORT,
    socRxReasonLLTagAbsentDrop = _SHR_RX_LLTAG_ABSENT_DROP,
    socRxReasonLLTagPresentDrop = _SHR_RX_LLTAG_PRESENT_DROP,  
    socRxReasonOAMCCMSlowPath = _SHR_RX_OAM_CCM_SLOWPATH,  
    socRxReasonOAMIncompleteOpcode = _SHR_RX_OAM_INCOMPLETE_OPCODE,
    socRxReasonCount = _SHR_RX_REASON_COUNT /* MUST BE LAST */
} soc_rx_reason_t;

/*
 * Set of "reasons" (see socRxReason*) why a packet came to the CPU.
 */
typedef _shr_rx_reasons_t soc_rx_reasons_t;

#define SOC_RX_REASON_NAMES_INITIALIZER _SHR_RX_REASON_NAMES_INITIALIZER

/*
 * Macro to check if a reason (socRxReason*) is included in a
 * set of reasons (soc_rx_reasons_t). Returns:
 *   zero     => reason is not included in the set
 *   non-zero => reason is included in the set
 */
#define SOC_RX_REASON_GET(_reasons, _reason) \
       _SHR_RX_REASON_GET(_reasons, _reason)

/*
 * Macro to add a reason (socRxReason*) to a set of
 * reasons (soc_rx_reasons_t)
 */
#define SOC_RX_REASON_SET(_reasons, _reason) \
       _SHR_RX_REASON_SET(_reasons, _reason)

/*
 * Macro to add all reasons (socRxReason*) to a set of
 * reasons (soc_rx_reasons_t)
 */
#define SOC_RX_REASON_SET_ALL(_reasons) \
       _SHR_RX_REASON_SET_ALL(_reasons)

/*
 * Macro to clear a reason (socRxReason*) from a set of
 * reasons (soc_rx_reasons_t)
 */
#define SOC_RX_REASON_CLEAR(_reasons, _reason) \
       _SHR_RX_REASON_CLEAR(_reasons, _reason)

/*
 * Macro to clear a set of reasons (soc_rx_reasons_t).
 */
#define SOC_RX_REASON_CLEAR_ALL(_reasons) \
       _SHR_RX_REASON_CLEAR_ALL(_reasons)

#endif  /* !_SOC_RX_H */
