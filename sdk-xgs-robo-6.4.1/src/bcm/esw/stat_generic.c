/*
 * $Id: stat_generic.c,v 1.27 Broadcom SDK $
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

#include <soc/drv.h>
#include <soc/counter.h>
#include <soc/debug.h>
#include <shared/bsl.h>
#include <bcm/stat.h>
#include <bcm/error.h>
#include <bcm_int/esw/stat.h>
#include <bcm_int/esw_dispatch.h>

/*
 * Function:
 *      _bcm_stat_generic_get
 * Description:
 *      Get the specified statistic for a port with common counter shared
 *      by both XE and GE MAC.
 * Parameters:
 *      unit - PCI device unit number (driver internal)
 *      port - zero-based port number
 *      sync_mode - if 1 read hw counter else sw accumualated counter
 *      type - SNMP statistics type (see stat.h)
 *      val  - (OUT) 64 bit statistic counter value
 * Returns:
 *      BCM_E_NONE - Success.
 *      BCM_E_PARAM - Illegal parameter.
 *      BCM_E_INTERNAL - Chip access failure.
 */
int
_bcm_stat_generic_get(int unit, bcm_port_t port, int sync_mode,
                      bcm_stat_val_t type, uint64 *val)
{
    uint64 count, count1;
    REG_MATH_DECL;       /* Required for use of the REG_* macros */

    COMPILER_REFERENCE(&count);  /* Work around PPC compiler bug */
    COMPILER_64_ZERO(count);

    switch (type) {
        /* *** RFC 1213 *** */
    case snmpIfInOctets:
        REG_ADD(unit, port, sync_mode, RBYTr, count);
        REG_ADD(unit, port, sync_mode, RRBYTr, count);
        break;
    case snmpIfInUcastPkts:             /* Unicast frames */
        if (COUNT_OVR_ERRORS(unit)) {
            REG_ADD(unit, port, sync_mode, RUCAr, count); /*unicast pkts rcvd */
        } else {
            REG_ADD(unit, port, sync_mode, RUCr, count); /* unicast pkts rcvd */
        }
        break;
    case snmpIfInNUcastPkts:             /* Non-unicast frames */
        if (COUNT_OVR_ERRORS(unit)) {
            REG_ADD(unit, port, sync_mode, RMCAr, count); /* + multicast */
            REG_ADD(unit, port, sync_mode, RBCAr, count); /* + broadcast */
        } else {
            REG_ADD(unit, port, sync_mode, RUCAr, count); /* + unicast excl */
                                                          /*   oversize */
            REG_ADD(unit, port, sync_mode, RMCAr, count); /* + multicast */
            REG_ADD(unit, port, sync_mode, RBCAr, count); /* + broadcast */
            REG_ADD(unit, port, sync_mode, ROVRr, count); /* + oversize */
            REG_SUB(unit, port, sync_mode, RUCr, count); /* - unicast */
        }
        break;
    case snmpIfInBroadcastPkts:          /* Broadcast frames */
        REG_ADD(unit, port, sync_mode, RBCAr, count);
        break;
    case snmpIfInMulticastPkts:          /* Multicast frames */
        REG_ADD(unit, port, sync_mode, RMCAr, count);
        break;
    case snmpIfInDiscards:           /* Dropped packets including aborted */
        REG_ADD(unit, port, sync_mode, RDBGC0r, count); /* Ingress drop conditions */
        BCM_IF_ERROR_RETURN
            (_bcm_stat_counter_non_dma_extra_get(unit,
                                   SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING,
                                                 port, &count1));
        COMPILER_64_ADD_64(count, count1);
        break;
    case snmpIfInErrors: /* RX Errors or Receive packets - non-error frames */
        REG_ADD(unit, port, sync_mode, RFCSr, count);
        REG_ADD(unit, port, sync_mode, RJBRr, count);
        if (SOC_REG_IS_VALID(unit,RMTUEr)) {
           REG_ADD(unit, port, sync_mode, RMTUEr, count);
        } else if (COUNT_OVR_ERRORS(unit)) {
            REG_ADD(unit, port, sync_mode, ROVRr, count);
        }
        REG_ADD(unit, port, sync_mode, RRPKTr, count);
        break;
    case snmpIfInUnknownProtos:
        break;
    case snmpIfOutOctets:
        REG_ADD(unit, port, sync_mode, TBYTr, count);
        break;
    case snmpIfOutUcastPkts:             /* Unicast frames */
        REG_ADD(unit, port, sync_mode, TUCAr, count);
        break;
    case snmpIfOutNUcastPkts:            /* Non-unicast frames */
        REG_ADD(unit, port, sync_mode, TMCAr, count); /* + multicast */
        REG_ADD(unit, port, sync_mode, TBCAr, count); /* + broadcast */
        break;
    case snmpIfOutBroadcastPkts:         /* Broadcast frames */
        REG_ADD(unit, port, sync_mode, TBCAr, count);
        break;
    case snmpIfOutMulticastPkts:         /* Multicast frames */
        REG_ADD(unit, port, sync_mode, TMCAr, count);
        break;
    case snmpIfOutDiscards:              /* Aged packet counter */
        if (SOC_REG_IS_VALID(unit, HOLDr)) {
            REG_ADD(unit, port, sync_mode, HOLDr, count);  /* L2 MTU drops */
        } else if (SOC_REG_IS_VALID(unit, HOL_DROPr)) {
            REG_ADD(unit, port, sync_mode, HOL_DROPr, count);
        }
        REG_ADD(unit, port, sync_mode, TDBGC3r, count);
        BCM_IF_ERROR_RETURN(
            bcm_esw_cosq_stat_get(unit, port, BCM_COS_INVALID, bcmCosqStatDroppedPackets, &count1));
        COMPILER_64_ADD_64(count, count1);
        break;
    case snmpIfOutErrors:   /* Error packets could not be xmitted */
        REG_ADD(unit, port, sync_mode, TERRr, count);
        break;
    case snmpIfOutQLen:
        {
            uint32 qcount;
            if (bcm_esw_port_queued_count_get(unit, port, &qcount) >= 0) {
                COMPILER_64_ADD_32(count, qcount);
            }
        }
        break;
    case snmpIpInReceives:
        REG_ADD(unit, port, sync_mode, RIPC4r, count);
        break;
    case snmpIpInHdrErrors:
        REG_ADD(unit, port, sync_mode, RIPD4r, count);
        break;
    case snmpIpForwDatagrams:
        REG_ADD(unit, port, sync_mode, TDBGC4r, count);
        break;
    case snmpIpInDiscards:
        REG_ADD(unit, port, sync_mode, RIPHE4r, count);
        REG_ADD(unit, port, sync_mode, RIPD4r, count);
        break;

        /* *** RFC 1493 *** */

    case snmpDot1dBasePortDelayExceededDiscards:
        break;
    case snmpDot1dBasePortMtuExceededDiscards:
        REG_ADD(unit, port, sync_mode, RMTUEr, count);
        break;
    case snmpDot1dTpPortInFrames:
        REG_ADD(unit, port, sync_mode, RPKTr, count);
        break;
    case snmpDot1dTpPortOutFrames:
        REG_ADD(unit, port, sync_mode, TPKTr, count);
        break;
    case snmpDot1dPortInDiscards:
        REG_ADD(unit, port, sync_mode, RDISCr, count);
        REG_ADD(unit, port, sync_mode, RIPD4r, count);
        REG_ADD(unit, port, sync_mode, RIPD6r, count);
        REG_ADD(unit, port, sync_mode, RPORTDr, count);
        break;

        /* *** RFC 1757 *** */

    case snmpEtherStatsDropEvents:
        REG_ADD(unit, port, sync_mode, RDISCr, count);
        break;
    case snmpEtherStatsOctets:
        REG_ADD(unit, port, sync_mode, RBYTr, count);
        REG_ADD(unit, port, sync_mode, TBYTr, count);
        REG_ADD(unit, port, sync_mode, RRBYTr, count); /* Runts bytes */
        break;
    case snmpEtherStatsPkts:
        REG_ADD(unit, port, sync_mode, RPKTr, count);
        REG_ADD(unit, port, sync_mode, TPKTr, count);
        REG_ADD(unit, port, sync_mode, RRPKTr, count); /* Runts */
        break;
    case snmpEtherStatsBroadcastPkts:
        REG_ADD(unit, port, sync_mode, RBCAr, count);
        REG_ADD(unit, port, sync_mode, TBCAr, count);
        break;
    case snmpEtherStatsMulticastPkts:
        REG_ADD(unit, port, sync_mode, RMCAr, count);
        REG_ADD(unit, port, sync_mode, TMCAr, count);
        break;
    case snmpEtherStatsCRCAlignErrors:
        REG_ADD(unit, port, sync_mode, RFCSr, count);
        break;
    case snmpEtherStatsUndersizePkts:
        REG_ADD(unit, port, sync_mode, RUNDr, count);
        break;
    case snmpEtherStatsOversizePkts:
        REG_ADD(unit, port, sync_mode, ROVRr, count);
        REG_ADD(unit, port, sync_mode, TOVRr, count);
        break;
    case snmpEtherRxOversizePkts:
        REG_ADD(unit, port, sync_mode, ROVRr, count);
        break;
    case snmpEtherTxOversizePkts:
        REG_ADD(unit, port, sync_mode, TOVRr, count);
        break;
    case snmpEtherStatsFragments:
        REG_ADD(unit, port, sync_mode, RFRGr, count);
        break;
    case snmpEtherStatsJabbers:
        REG_ADD(unit, port, sync_mode, RJBRr, count);
        break;
    case snmpEtherStatsCollisions:
        break;

        /* *** rfc1757 definition counts receive packet only *** */

    case snmpEtherStatsPkts64Octets:
        REG_ADD(unit, port, sync_mode, R64r, count);
        REG_ADD(unit, port, sync_mode, T64r, count);
        break;
    case snmpEtherStatsPkts65to127Octets:
        REG_ADD(unit, port, sync_mode, R127r, count);
        REG_ADD(unit, port, sync_mode, T127r, count);
        break;
    case snmpEtherStatsPkts128to255Octets:
        REG_ADD(unit, port, sync_mode, R255r, count);
        REG_ADD(unit, port, sync_mode, T255r, count);
        break;
    case snmpEtherStatsPkts256to511Octets:
        REG_ADD(unit, port, sync_mode, R511r, count);
        REG_ADD(unit, port, sync_mode, T511r, count);
        break;
    case snmpEtherStatsPkts512to1023Octets:
        REG_ADD(unit, port, sync_mode, R1023r, count);
        REG_ADD(unit, port, sync_mode, T1023r, count);
        break;
    case snmpEtherStatsPkts1024to1518Octets:
        REG_ADD(unit, port, sync_mode, R1518r, count);
        REG_ADD(unit, port, sync_mode, T1518r, count);
        break;
    case snmpBcmEtherStatsPkts1519to1522Octets: /* not in rfc1757 */
        REG_ADD(unit, port, sync_mode, RMGVr, count);
        REG_ADD(unit, port, sync_mode, TMGVr, count);
        break;
    case snmpBcmEtherStatsPkts1522to2047Octets: /* not in rfc1757 */
        REG_ADD(unit, port, sync_mode, R2047r, count);
        REG_SUB(unit, port, sync_mode, RMGVr, count);
        REG_ADD(unit, port, sync_mode, T2047r, count);
        REG_SUB(unit, port, sync_mode, TMGVr, count);
        break;
    case snmpBcmEtherStatsPkts2048to4095Octets: /* not in rfc1757 */
        REG_ADD(unit, port, sync_mode, R4095r, count);
        REG_ADD(unit, port, sync_mode, T4095r, count);
        break;
    case snmpBcmEtherStatsPkts4095to9216Octets: /* not in rfc1757 */
        REG_ADD(unit, port, sync_mode, R9216r, count);
        REG_ADD(unit, port, sync_mode, T9216r, count);
        break;
    case snmpBcmEtherStatsPkts9217to16383Octets: /* not in rfc1757 */
        REG_ADD(unit, port, sync_mode, R16383r, count);
        REG_ADD(unit, port, sync_mode, T16383r, count);
        break;

    case snmpBcmReceivedPkts64Octets:
        REG_ADD(unit, port, sync_mode, R64r, count);
        break;
    case snmpBcmReceivedPkts65to127Octets:
        REG_ADD(unit, port, sync_mode, R127r, count);
        break;
    case snmpBcmReceivedPkts128to255Octets:
        REG_ADD(unit, port, sync_mode, R255r, count);
        break;
    case snmpBcmReceivedPkts256to511Octets:
        REG_ADD(unit, port, sync_mode, R511r, count);
        break;
    case snmpBcmReceivedPkts512to1023Octets:
        REG_ADD(unit, port, sync_mode, R1023r, count);
        break;
    case snmpBcmReceivedPkts1024to1518Octets:
        REG_ADD(unit, port, sync_mode, R1518r, count);
        break;
    case snmpBcmReceivedPkts1519to2047Octets:
        REG_ADD(unit, port, sync_mode, R2047r, count);
        break;
    case snmpBcmReceivedPkts2048to4095Octets:
        REG_ADD(unit, port, sync_mode, R4095r, count);
        break;
    case snmpBcmReceivedPkts4095to9216Octets:
        REG_ADD(unit, port, sync_mode, R9216r, count);
        break;
    case snmpBcmReceivedPkts9217to16383Octets:
        REG_ADD(unit, port, sync_mode, R16383r, count);
        break;

    case snmpBcmTransmittedPkts64Octets:
        REG_ADD(unit, port, sync_mode, T64r, count);
        break;
    case snmpBcmTransmittedPkts65to127Octets:
        REG_ADD(unit, port, sync_mode, T127r, count);
        break;
    case snmpBcmTransmittedPkts128to255Octets:
        REG_ADD(unit, port, sync_mode, T255r, count);
        break;
    case snmpBcmTransmittedPkts256to511Octets:
        REG_ADD(unit, port, sync_mode, T511r, count);
        break;
    case snmpBcmTransmittedPkts512to1023Octets:
        REG_ADD(unit, port, sync_mode, T1023r, count);
        break;
    case snmpBcmTransmittedPkts1024to1518Octets:
        REG_ADD(unit, port, sync_mode, T1518r, count);
        break;
    case snmpBcmTransmittedPkts1519to2047Octets:
        REG_ADD(unit, port, sync_mode, T2047r, count);
        break;
    case snmpBcmTransmittedPkts2048to4095Octets:
        REG_ADD(unit, port, sync_mode, T4095r, count);
        break;
    case snmpBcmTransmittedPkts4095to9216Octets:
        REG_ADD(unit, port, sync_mode, T9216r, count);
        break;
    case snmpBcmTransmittedPkts9217to16383Octets:
        REG_ADD(unit, port, sync_mode, T16383r, count);
        break;

    case snmpEtherStatsTXNoErrors:
        REG_ADD(unit, port, sync_mode, TPOKr, count);
        break;
    case snmpEtherStatsRXNoErrors:
        REG_ADD(unit, port, sync_mode, RPOKr, count);
        break;

        /* *** RFC 2665 *** */

    case snmpDot3StatsAlignmentErrors:
        break;
    case snmpDot3StatsFCSErrors:
        REG_ADD(unit, port, sync_mode, RFCSr, count);
        break;
    case snmpDot3StatsSingleCollisionFrames:
        break;
    case snmpDot3StatsMultipleCollisionFrames:
        break;
    case snmpDot3StatsSQETTestErrors:
        break;
    case snmpDot3StatsDeferredTransmissions:
        break;
    case snmpDot3StatsLateCollisions:
        break;
    case snmpDot3StatsExcessiveCollisions:
        break;
    case snmpDot3StatsInternalMacTransmitErrors:
        REG_ADD(unit, port, sync_mode, TUFLr, count);
        REG_ADD(unit, port, sync_mode, TERRr, count);
        break;
    case snmpDot3StatsCarrierSenseErrors:
        break;
    case snmpDot3StatsFrameTooLongs:
        REG_ADD(unit, port, sync_mode, RMTUEr, count);
        break;
    case snmpDot3StatsInternalMacReceiveErrors:
        break;
    case snmpDot3StatsSymbolErrors:
        REG_ADD(unit, port, sync_mode, RERPKTr, count);
        break;
    case snmpDot3ControlInUnknownOpcodes:
        REG_ADD(unit, port, sync_mode, RXUOr, count);
        break;
    case snmpDot3InPauseFrames:
        REG_ADD(unit, port, sync_mode, RXPFr, count);
        break;
    case snmpDot3OutPauseFrames:
        REG_ADD(unit, port, sync_mode, TXPFr, count);
        break;

        /* *** RFC 2233 high capacity versions of RFC1213 objects *** */

    case snmpIfHCInOctets:
        REG_ADD(unit, port, sync_mode, RBYTr, count);
        REG_ADD(unit, port, sync_mode, RRBYTr, count);        
        break;
    case snmpIfHCInUcastPkts:
        if (COUNT_OVR_ERRORS(unit)) {
            REG_ADD(unit, port, sync_mode, RUCAr, count);  /* unicast pkts rcvd */
        } else {
            REG_ADD(unit, port, sync_mode, RUCr, count);  /* unicast pkts rcvd */
        }
        break;
    case snmpIfHCInMulticastPkts:
        REG_ADD(unit, port, sync_mode, RMCAr, count);
        break;
    case snmpIfHCInBroadcastPkts:
        REG_ADD(unit, port, sync_mode, RBCAr, count);
        break;
    case snmpIfHCOutOctets:
        REG_ADD(unit, port, sync_mode, TBYTr, count);
        break;
    case snmpIfHCOutUcastPkts:
        REG_ADD(unit, port, sync_mode, TUCAr, count);
        break;
    case snmpIfHCOutMulticastPkts:
        REG_ADD(unit, port, sync_mode, TMCAr, count);
        break;
    case snmpIfHCOutBroadcastPckts:
        REG_ADD(unit, port, sync_mode, TBCAr, count);
        break;

        /* *** RFC 2465 *** */

    case snmpIpv6IfStatsInReceives:
        REG_ADD(unit, port, sync_mode, RIPC6r, count);
        REG_ADD(unit, port, sync_mode, IMRP6r, count);
        break;
    case snmpIpv6IfStatsInHdrErrors:
        REG_ADD(unit, port, sync_mode, RIPD6r, count);
        break;
    case snmpIpv6IfStatsInAddrErrors:
        REG_ADD(unit, port, sync_mode, RIPHE6r, count);
        break;
    case snmpIpv6IfStatsInDiscards:
        REG_ADD(unit, port, sync_mode, RIPHE6r, count);
        REG_ADD(unit, port, sync_mode, RIPD6r, count);
        break;
    case snmpIpv6IfStatsOutForwDatagrams:
        REG_ADD(unit, port, sync_mode, TDBGC0r, count);
        break;
    case snmpIpv6IfStatsOutDiscards:
        REG_ADD(unit, port, sync_mode, TDBGC1r, count);
        break;
    case snmpIpv6IfStatsInMcastPkts:
        REG_ADD(unit, port, sync_mode, IMRP6r, count);
        break;
    case snmpIpv6IfStatsOutMcastPkts:
        REG_ADD(unit, port, sync_mode, TDBGC2r, count);
        break;

        /* *** IEEE 802.1bb *** */
    case snmpIeee8021PfcRequests:
        REG_ADD(unit, port, sync_mode, TXPPr, count);
        break;
    case snmpIeee8021PfcIndications:
        REG_ADD(unit, port, sync_mode, RXPPr, count);
        break;

        /* *** RFC 1284 - unsupported in XGS *** */
    case snmpDot3StatsInRangeLengthError:
        break;

        /* *** RFC 4837 - unsupported in XGS *** */
    case snmpDot3OmpEmulationCRC8Errors:
    case snmpDot3MpcpRxGate:
    case snmpDot3MpcpRxRegister:
    case snmpDot3MpcpTxRegRequest:
    case snmpDot3MpcpTxRegAck:
    case snmpDot3MpcpTxReport:
    case snmpDot3EponFecCorrectedBlocks:
    case snmpDot3EponFecUncorrectableBlocks:
        break;

        /* IPMC counters (broadcom specific) */

    case snmpBcmIPMCBridgedPckts:
        REG_ADD(unit, port, sync_mode, RDBGC1r, count);
        break;
    case snmpBcmIPMCRoutedPckts:
        REG_ADD(unit, port, sync_mode, IMRP4r, count);
        REG_ADD(unit, port, sync_mode, IMRP6r, count);
        break;
    case snmpBcmIPMCInDroppedPckts:
        REG_ADD(unit, port, sync_mode, RDBGC2r, count);
        break;
    case snmpBcmIPMCOutDroppedPckts:
        REG_ADD(unit, port, sync_mode, TDBGC5r, count);
        break;

        /* EA (broadcom specific) - unsupported in XGS */
    case snmpBcmPonInDroppedOctets:
    case snmpBcmPonOutDroppedOctets:
    case snmpBcmPonInDelayedOctets:
    case snmpBcmPonOutDelayedOctets:
    case snmpBcmPonInDelayedHundredUs:
    case snmpBcmPonOutDelayedHundredUs:
    case snmpBcmPonInFrameErrors:
    case snmpBcmPonInOamFrames:
    case snmpBcmPonOutOamFrames:
    case snmpBcmPonOutUnusedOctets:
	break;

    /* Vlan Tag Frame Counters */
    case snmpBcmRxVlanTagFrame:
        REG_ADD(unit, port, sync_mode, RVLNr, count);
        break;
    case snmpBcmRxDoubleVlanTagFrame:
        REG_ADD(unit, port, sync_mode, RDVLNr, count);
        break;
    case snmpBcmTxVlanTagFrame:
        REG_ADD(unit, port, sync_mode, TVLNr, count);
        break;
    case snmpBcmTxDoubleVlanTagFrame:
        REG_ADD(unit, port, sync_mode, TDVLNr, count);
        break;

    /* PFC Control Frame Counters */
    case snmpBcmRxPFCControlFrame:
        REG_ADD(unit, port, sync_mode, RXPPr, count);
        break;

    case snmpBcmTxPFCControlFrame:
        REG_ADD(unit, port, sync_mode, TXPPr, count);
        break;

    /* Receive PFC Frame Priority 0 XON to XOFF */
    case snmpBcmRxPFCFrameXonPriority0:
        REG_ADD(unit, port, sync_mode, RPFCOFF0r, count);
        break;

    case snmpBcmRxPFCFrameXonPriority1:
        REG_ADD(unit, port, sync_mode, RPFCOFF1r, count);
        break;

    case snmpBcmRxPFCFrameXonPriority2:
        REG_ADD(unit, port, sync_mode, RPFCOFF2r, count);
        break;

    case snmpBcmRxPFCFrameXonPriority3:
        REG_ADD(unit, port, sync_mode, RPFCOFF3r, count);
        break;

    case snmpBcmRxPFCFrameXonPriority4:
        REG_ADD(unit, port, sync_mode, RPFCOFF4r, count);
        break;

    case snmpBcmRxPFCFrameXonPriority5:
        REG_ADD(unit, port, sync_mode, RPFCOFF5r, count);
        break;

    case snmpBcmRxPFCFrameXonPriority6:
        REG_ADD(unit, port, sync_mode, RPFCOFF6r, count);
        break;

    case snmpBcmRxPFCFrameXonPriority7:
        REG_ADD(unit, port, sync_mode, RPFCOFF7r, count);
        break;

    /* Receive PFC Frame Priority */
    case snmpBcmRxPFCFramePriority0:
        REG_ADD(unit, port, sync_mode, RPFC0r, count);
        break;

    case snmpBcmRxPFCFramePriority1:
        REG_ADD(unit, port, sync_mode, RPFC1r, count);
        break;

    case snmpBcmRxPFCFramePriority2:
        REG_ADD(unit, port, sync_mode, RPFC2r, count);
        break;

    case snmpBcmRxPFCFramePriority3:
        REG_ADD(unit, port, sync_mode, RPFC3r, count);
        break;

    case snmpBcmRxPFCFramePriority4:
        REG_ADD(unit, port, sync_mode, RPFC4r, count);
        break;

    case snmpBcmRxPFCFramePriority5:
        REG_ADD(unit, port, sync_mode, RPFC5r, count);
        break;

    case snmpBcmRxPFCFramePriority6:
        REG_ADD(unit, port, sync_mode, RPFC6r, count);
        break;

    case snmpBcmRxPFCFramePriority7:
        REG_ADD(unit, port, sync_mode, RPFC7r, count);
        break;

    /* Transmit PFC Frame Priority */
    case snmpBcmTxPFCFramePriority0:
        REG_ADD(unit, port, sync_mode, TPFC0r, count);
        break;

    case snmpBcmTxPFCFramePriority1:
        REG_ADD(unit, port, sync_mode, TPFC1r, count);
        break;

    case snmpBcmTxPFCFramePriority2:
        REG_ADD(unit, port, sync_mode, TPFC2r, count);
        break;

    case snmpBcmTxPFCFramePriority3:
        REG_ADD(unit, port, sync_mode, TPFC3r, count);
        break;

    case snmpBcmTxPFCFramePriority4:
        REG_ADD(unit, port, sync_mode, TPFC4r, count);
        break;

    case snmpBcmTxPFCFramePriority5:
        REG_ADD(unit, port, sync_mode, TPFC5r, count);
        break;

    case snmpBcmTxPFCFramePriority6:
        REG_ADD(unit, port, sync_mode, TPFC6r, count);
        break;

    case snmpBcmTxPFCFramePriority7:
        REG_ADD(unit, port, sync_mode, TPFC7r, count);
        break;

    case snmpFcmPortClass3RxFrames:
        if (soc_feature(unit, soc_feature_fcoe)) {
            REG_ADD(unit, port, sync_mode, RDBGC3r, count);
        } else {
            return BCM_E_UNAVAIL;
        }
        break;

    case snmpFcmPortClass3TxFrames:
        if (soc_feature(unit, soc_feature_fcoe)) {
            REG_ADD(unit, port, sync_mode, TDBGC6r, count);
        } else {
            return BCM_E_UNAVAIL;
        }
        break;

    case snmpFcmPortClass3Discards:
        if (soc_feature(unit, soc_feature_fcoe)) {
            REG_ADD(unit, port, sync_mode, RDBGC4r, count);
        } else {
            return BCM_E_UNAVAIL;
        }
        break;

    case snmpFcmPortClass2RxFrames:
        if (soc_feature(unit, soc_feature_fcoe)) {
            REG_ADD(unit, port, sync_mode, RDBGC5r, count);
        } else {
            return BCM_E_UNAVAIL;
        }
        break;

    case snmpFcmPortClass2TxFrames:
        if (soc_feature(unit, soc_feature_fcoe)) {
            REG_ADD(unit, port, sync_mode, TDBGC7r, count);
        } else {
            return BCM_E_UNAVAIL;
        }
        break;

    case snmpFcmPortClass2Discards:
        if (soc_feature(unit, soc_feature_fcoe)) {
            REG_ADD(unit, port, sync_mode, RDBGC6r, count);
        } else {
            return BCM_E_UNAVAIL;
        }
        break;

    case snmpFcmPortInvalidCRCs:
        if (soc_feature(unit, soc_feature_fcoe)) {
            REG_ADD(unit, port, sync_mode, EGR_FCOE_INVALID_CRC_FRAMESr, count);
        } else {
            return BCM_E_UNAVAIL;
        }
        break;

    case snmpFcmPortDelimiterErrors:
        if (soc_feature(unit, soc_feature_fcoe)) {
            REG_ADD(unit, port, sync_mode, EGR_FCOE_DELIMITER_ERROR_FRAMESr, 
                    count);
        } else {
            return BCM_E_UNAVAIL;
        }
        break;

    default:
        if (type < snmpValCount) {
            return BCM_E_UNAVAIL;
        }
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "_bcm_stat_generic_get: Statistic not supported: %d\n"), type));
        return BCM_E_PARAM;
    }

    *val = count;

    return BCM_E_NONE;
}
