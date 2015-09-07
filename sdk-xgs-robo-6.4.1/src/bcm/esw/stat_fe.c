/*
 * $Id: stat_fe.c,v 1.48 Broadcom SDK $
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

#define FE_CNTR(_fe)	(_fe)

/*
 * Function:
 *      _bcm_stat_fe_get
 * Description:
 *      Get the specified statistic for a FE port or GE port in 10/100 Mb
 *      mode on the StrataSwitch family of devices.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
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
_bcm_stat_fe_get(int unit, bcm_port_t port, int sync_mode, 
                  bcm_stat_val_t type, uint64 *val)
{
    uint64      count, count1;
    REG_MATH_DECL;       /* Required for use of the REG_* macros */

    COMPILER_REFERENCE(&count);  /* Work around PPC compiler bug */

    COMPILER_64_ZERO(count);

    switch (type) {
        /* *** RFC 1213 *** */

    case snmpIfInOctets:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RBYTr), count);
        break;
    case snmpIfInUcastPkts:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RPKTr), count); /* all pkts rcvd */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RMCAr), count); /* - multicast */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RBCAr), count); /* - broadcast */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RALNr), count); /* - bad FCS, dribble bit  */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RFCSr), count); /* - bad FCS, no dribble bit */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RFLRr), count); /* - good FCS, bad length */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RJBRr), count); /* - oversize, bad FCS */
        if (COUNT_OVR_ERRORS(unit)) {
            REG_SUB(unit, port, sync_mode, FE_CNTR(ROVRr), count); /* - oversize, good FCS */
        }
        break;
    case snmpIfInNUcastPkts:  /* Multicast frames plus broadcast frames */
        REG_ADD(unit, port, sync_mode, FE_CNTR(RMCAr), count);
        REG_ADD(unit, port, sync_mode, FE_CNTR(RBCAr), count);
        break;
    case snmpIfInBroadcastPkts:  /* broadcast frames */
        REG_ADD(unit, port, sync_mode, FE_CNTR(RBCAr), count);
        break;
    case snmpIfInMulticastPkts:  /* Multicast frames */
        REG_ADD(unit, port, sync_mode, FE_CNTR(RMCAr), count);
        break;
    case snmpIfInDiscards:  /* Dropped packets including aborted */
        REG_ADD(unit, port, sync_mode, FE_CNTR(RDISCr), count);
        break;
    case snmpIfInErrors:        /* received packts containing errors */
        REG_ADD(unit, port, sync_mode, FE_CNTR(RUNDr), count);  /* undersize, ok FCS */
        REG_ADD(unit, port, sync_mode, FE_CNTR(RFRGr), count);  /* ..., bad FCS */
        REG_ADD(unit, port, sync_mode, FE_CNTR(RFCSr), count);  /* FCS errors */
        if (COUNT_OVR_ERRORS(unit)) {
            REG_ADD(unit, port, sync_mode, FE_CNTR(ROVRr), count);  /* oversize, ok FCS */
        }
        REG_ADD(unit, port, sync_mode, FE_CNTR(RJBRr), count);  /* ..., bad FCS */
        break;
    case snmpIfInUnknownProtos:
        /* always 0 */
        break;
    case snmpIfOutOctets:        /* TX bytes */
        REG_ADD(unit, port, sync_mode, FE_CNTR(TBYTr), count);
        break;
    case snmpIfOutUcastPkts:        /* ALL - mcast - bcast */
        REG_ADD(unit, port, sync_mode, FE_CNTR(TPKTr), count);
        REG_SUB(unit, port, sync_mode, FE_CNTR(TMCAr), count);
        REG_SUB(unit, port, sync_mode, FE_CNTR(TBCAr), count);
        REG_SUB(unit, port, sync_mode, FE_CNTR(TFCSr), count); /* bad FCS */
        REG_SUB(unit, port, sync_mode, FE_CNTR(TJBRr), count); /* oversize, bad FCS */
        if (COUNT_OVR_ERRORS(unit)) {
            REG_SUB(unit, port, sync_mode, FE_CNTR(TOVRr), count);  /* oversize, good FCS */
        }
        break;
    case snmpIfOutNUcastPkts:  /* broadcast frames plus multicast frames */
        REG_ADD(unit, port, sync_mode, FE_CNTR(TMCAr), count);
        REG_ADD(unit, port, sync_mode, FE_CNTR(TBCAr), count);
        break;
    case snmpIfOutBroadcastPkts:        /* broadcast frames */
        REG_ADD(unit, port, sync_mode, FE_CNTR(TBCAr), count);
        break;
    case snmpIfOutMulticastPkts:        /* multicast frames */
        REG_ADD(unit, port, sync_mode, FE_CNTR(TMCAr), count);
        break;
    case snmpIfOutDiscards:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TEDFr), count);  /* multiple deferral */
        _bcm_stat_counter_extra_get(unit, EGRDROPPKTCOUNTr, port, &count1);
        COMPILER_64_ADD_64(count, count1);
        /* other causes of discards? */
        break;
    case snmpIfOutErrors:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TXCLr), count);  /* excessive coll */
        break;
    case snmpIfOutQLen: {
        uint32  qcount;
        if (bcm_esw_port_queued_count_get(unit, port, &qcount) >= 0) {
            COMPILER_64_ADD_32(count, qcount);
        }
    }
    break;
    case snmpIpInReceives:
        /* XGS2 specific */
        break;
    case snmpIpInHdrErrors:
        break;
    case snmpIpForwDatagrams:
        /* XGS2 specific */
        break;
    case snmpIpInDiscards:
        /* XGS2 specific */
        break;

        /* *** RFC 1493 *** */

    case snmpDot1dBasePortDelayExceededDiscards:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TEDFr), count);  /* multiple deferral */
        break;
    case snmpDot1dBasePortMtuExceededDiscards:
        if (COUNT_OVR_ERRORS(unit)) {
            REG_ADD(unit, port, sync_mode, FE_CNTR(ROVRr), count);
            REG_ADD(unit, port, sync_mode, FE_CNTR(TOVRr), count);
        }
        break;
    case snmpDot1dTpPortInFrames:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RPKTr), count);
        break;
    case snmpDot1dTpPortOutFrames:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TPKTr), count);
        break;
    case snmpDot1dPortInDiscards:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RDISCr), count);
        break;

        /* *** RFC 1757 *** */

    case snmpEtherStatsDropEvents:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RDISCr), count);
        break;
    case snmpEtherStatsOctets:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RBYTr), count);
        REG_ADD(unit, port, sync_mode, FE_CNTR(TBYTr), count);
        break;
    case snmpEtherStatsPkts:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RPKTr), count);
        REG_ADD(unit, port, sync_mode, FE_CNTR(TPKTr), count);
        REG_ADD(unit, port, sync_mode, FE_CNTR(RUNDr), count); /* Runt packets */
        break;
    case snmpEtherStatsBroadcastPkts:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RBCAr), count);
        REG_ADD(unit, port, sync_mode, FE_CNTR(TBCAr), count);
        break;
    case snmpEtherStatsMulticastPkts:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RMCAr), count);
        REG_ADD(unit, port, sync_mode, FE_CNTR(TMCAr), count);
        break;
    case snmpEtherStatsCRCAlignErrors:  /* CRC errors + alignment errors */
        REG_ADD(unit, port, sync_mode, FE_CNTR(RFCSr), count);
        break;
    case snmpEtherStatsUndersizePkts:  /* Undersize frames */
        REG_ADD(unit, port, sync_mode, FE_CNTR(RUNDr), count);
        break;
    case snmpEtherStatsOversizePkts:
        if (soc_feature(unit, soc_feature_stat_jumbo_adj)) {
            REG_ADD(unit, port, sync_mode, FE_CNTR(ROVRr), count);
            REG_ADD(unit, port, sync_mode, FE_CNTR(TOVRr), count);
        } else {
            REG_ADD(unit, port, sync_mode, FE_CNTR(RJBRr), count);
        }
        break;
    case snmpEtherRxOversizePkts:
        if (soc_feature(unit, soc_feature_stat_jumbo_adj)) {
                REG_ADD(unit, port, sync_mode, FE_CNTR(ROVRr), count);
        }
        break;
    case snmpEtherTxOversizePkts:
        if (soc_feature(unit, soc_feature_stat_jumbo_adj)) {
                REG_ADD(unit, port, sync_mode, FE_CNTR(TOVRr), count);
            }
        break;
     case snmpEtherStatsFragments:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RFRGr), count);
        if (SOC_REG_IS_VALID(unit,TFRGr)) {
           REG_ADD(unit, port, sync_mode, FE_CNTR(TFRGr), count);
        }
        break;
    case snmpEtherStatsJabbers:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RJBRr), count);
        break;
    case snmpEtherStatsCollisions:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TNCLr), count);
        break;
    case snmpEtherStatsPkts64Octets:
    case snmpEtherStatsPkts65to127Octets:
    case snmpEtherStatsPkts128to255Octets:
    case snmpEtherStatsPkts256to511Octets:
    case snmpEtherStatsPkts512to1023Octets:
        /* XGS2 specific */
        break;
    case snmpEtherStatsPkts1024to1518Octets:
        break;

        /* *** not actually in rfc1757 *** */

    case snmpBcmEtherStatsPkts1519to1522Octets:
    case snmpBcmEtherStatsPkts1522to2047Octets:
    case snmpBcmEtherStatsPkts2048to4095Octets:
    case snmpBcmEtherStatsPkts4095to9216Octets:
    case snmpBcmEtherStatsPkts9217to16383Octets:
    case snmpBcmReceivedPkts64Octets:
    case snmpBcmReceivedPkts65to127Octets:
    case snmpBcmReceivedPkts128to255Octets:
    case snmpBcmReceivedPkts256to511Octets:
    case snmpBcmReceivedPkts512to1023Octets:
    case snmpBcmReceivedPkts1024to1518Octets:
    case snmpBcmReceivedPkts1519to2047Octets:
    case snmpBcmReceivedPkts2048to4095Octets:
    case snmpBcmReceivedPkts4095to9216Octets:
    case snmpBcmReceivedPkts9217to16383Octets:
    case snmpBcmTransmittedPkts64Octets:
    case snmpBcmTransmittedPkts65to127Octets:
    case snmpBcmTransmittedPkts128to255Octets:
    case snmpBcmTransmittedPkts256to511Octets:
    case snmpBcmTransmittedPkts512to1023Octets:
    case snmpBcmTransmittedPkts1024to1518Octets:
    case snmpBcmTransmittedPkts1519to2047Octets:
    case snmpBcmTransmittedPkts2048to4095Octets:
    case snmpBcmTransmittedPkts4095to9216Octets:
    case snmpBcmTransmittedPkts9217to16383Octets:
        break;

    case snmpEtherStatsTXNoErrors:
        /*  TPKT - (TNCL + TOVR + TFRG + TUND) */
        REG_ADD(unit, port, sync_mode, FE_CNTR(TPKTr), count);  /* All Packets */
        REG_SUB(unit, port, sync_mode, FE_CNTR(TNCLr), count);  /* collision */
        if (COUNT_OVR_ERRORS(unit)) {
            REG_SUB(unit, port, sync_mode, FE_CNTR(TOVRr), count);  /* Oversize */
        }
        REG_SUB(unit, port, sync_mode, FE_CNTR(TFRGr), count);  /* Fragments */
        break;
    case snmpEtherStatsRXNoErrors:
        /* RPKT - ( RFCS + RXUO + ROVR + RFLR) */
        REG_ADD(unit, port, sync_mode, FE_CNTR(RPKTr), count);
        REG_SUB(unit, port, sync_mode, FE_CNTR(RFCSr), count);
        REG_SUB(unit, port, sync_mode, FE_CNTR(RXUOr), count);
        if (COUNT_OVR_ERRORS(unit)) {
            REG_SUB(unit, port, sync_mode, FE_CNTR(ROVRr), count);
        }
        REG_SUB(unit, port, sync_mode, FE_CNTR(RFLRr), count);
        break;

        /* *** RFC 2665 *** */

    case snmpDot3StatsAlignmentErrors:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RALNr), count);
        break;
    case snmpDot3StatsFCSErrors:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RFCSr), count);
        break;
    case snmpDot3StatsSingleCollisionFrames:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TSCLr), count);
        break;
    case snmpDot3StatsMultipleCollisionFrames:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TMCLr), count);
        break;
    case snmpDot3StatsSQETTestErrors:
        /* always 0 */
        break;
    case snmpDot3StatsDeferredTransmissions:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TDFRr), count);
        break;
    case snmpDot3StatsLateCollisions:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TLCLr), count);
        break;
    case snmpDot3StatsExcessiveCollisions:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TXCLr), count);
        break;
    case snmpDot3StatsInternalMacTransmitErrors:
        /* always 0 */
        break;
    case snmpDot3StatsCarrierSenseErrors:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RFCRr), count);
        break;
    case snmpDot3StatsFrameTooLongs:
        if (soc_feature(unit, soc_feature_stat_jumbo_adj)) {
            REG_ADD(unit, port, sync_mode, FE_CNTR(ROVRr), count);
        } else {
            REG_ADD(unit, port, sync_mode, FE_CNTR(RJBRr), count);
        }
        break;
    case snmpDot3StatsInternalMacReceiveErrors:
        /* always 0 */
        break;
    case snmpDot3StatsSymbolErrors:
        /* XGS2 specific */
        break;
    case snmpDot3ControlInUnknownOpcodes:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RXUOr), count);
        break;
    case snmpDot3InPauseFrames:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RXPFr), count);
        break;
    case snmpDot3OutPauseFrames:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TXPFr), count);
        break;

        /* *** RFC 2233 high capacity versions of RFC1213 objects *** */

    case snmpIfHCInOctets:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RBYTr), count);
        break;
    case snmpIfHCInUcastPkts:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RPKTr), count); /* all pkts rcvd */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RMCAr), count); /* - multicast */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RBCAr), count); /* - broadcast */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RALNr), count); /* - bad FCS, dribble bit  */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RFCSr), count); /* - bad FCS, no dribble bit */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RFLRr), count); /* - good FCS, bad length */
        REG_SUB(unit, port, sync_mode, FE_CNTR(RJBRr), count); /* - oversize, bad FCS */
        if (COUNT_OVR_ERRORS(unit)) {
            REG_SUB(unit, port, sync_mode, FE_CNTR(ROVRr), count); /* - oversize, good FCS */
        }
        break;
    case snmpIfHCInMulticastPkts:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RMCAr), count);
        break;
    case snmpIfHCInBroadcastPkts:
        REG_ADD(unit, port, sync_mode, FE_CNTR(RBCAr), count);
        break;
    case snmpIfHCOutOctets:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TBYTr), count);
        break;
    case snmpIfHCOutUcastPkts:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TPKTr), count);
        REG_SUB(unit, port, sync_mode, FE_CNTR(TMCAr), count);
        REG_SUB(unit, port, sync_mode, FE_CNTR(TBCAr), count);
        REG_SUB(unit, port, sync_mode, FE_CNTR(TFCSr), count); /* bad FCS */
        REG_SUB(unit, port, sync_mode, FE_CNTR(TJBRr), count); /* oversize, bad FCS */
        if (COUNT_OVR_ERRORS(unit)) {
            REG_SUB(unit, port, sync_mode, FE_CNTR(TOVRr), count);  /* oversize, good FCS */
        }
        break;
    case snmpIfHCOutMulticastPkts:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TMCAr), count);
        break;
    case snmpIfHCOutBroadcastPckts:
        REG_ADD(unit, port, sync_mode, FE_CNTR(TBCAr), count);
        break;

        /* *** RFC 2465 *** */

    case snmpIpv6IfStatsInReceives:
    case snmpIpv6IfStatsInHdrErrors:
    case snmpIpv6IfStatsInAddrErrors:
    case snmpIpv6IfStatsInDiscards:
    case snmpIpv6IfStatsOutForwDatagrams:
    case snmpIpv6IfStatsOutDiscards:
    case snmpIpv6IfStatsInMcastPkts:
    case snmpIpv6IfStatsOutMcastPkts:
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

        /* IPMC counters (broadcom XGS2 specific) */
    case snmpBcmIPMCBridgedPckts:
    case snmpBcmIPMCRoutedPckts:
    case snmpBcmIPMCInDroppedPckts:
    case snmpBcmIPMCOutDroppedPckts:
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
                                "bcm_stat_get: Statistic not supported: %d\n"), type));
        return BCM_E_PARAM;
    }

    *val = count;

    return BCM_E_NONE;
}
