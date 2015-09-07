/*
 * $Id: stat_hg.c,v 1.64 Broadcom SDK $
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
 *      _bcm_stat_hg_get
 * Description:
 *      Get the specified statistic for a HG port on the StrataSwitch family
 *      of devices.
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
_bcm_stat_hg_get(int unit, bcm_port_t port, int sync_mode,
                 bcm_stat_val_t type, uint64 *val)
{
    uint64 count, count1;
    REG_MATH_DECL;       /* Required for use of the REG_* macros */

    COMPILER_REFERENCE(&count);  /* Work around PPC compiler bug */
    COMPILER_64_ZERO(count);

    switch (type) {
    case snmpIfInOctets:
        REG_ADD(unit, port, sync_mode, IRBYTr, count);
        break;
    case snmpIfInUcastPkts:
       if (soc_feature(unit, soc_feature_hw_stats_calc)) {
            if (SOC_REG_IS_VALID(unit, IRUCAr)) {
                REG_ADD(unit, port, sync_mode, IRUCAr, count);
            } else if (SOC_REG_IS_VALID(unit, IRUCr)) {
                REG_ADD(unit, port, sync_mode, IRUCr, count);
            } else {
                REG_ADD(unit, port, sync_mode, RUCr, count);  /* unicast pkts rcvd */
            }
        } else {
            REG_ADD(unit, port, sync_mode, IRPKTr, count);
            REG_SUB(unit, port, sync_mode, IRMCAr, count); /* - multicast */
            REG_SUB(unit, port, sync_mode, IRBCAr, count); /* - broadcast */
            REG_SUB(unit, port, sync_mode, IRFCSr, count); /* - bad FCS */
            REG_SUB(unit, port, sync_mode, IRXCFr, count); /* - good FCS, all MAC ctrl */
            REG_SUB(unit, port, sync_mode, IRJBRr, count); /* - oversize, bad FCS */
            if (COUNT_OVR_ERRORS(unit)) {
                REG_SUB(unit, port, sync_mode, IROVRr, count); /* - oversize, good FCS */
            }
        }
            break;
    case snmpIfInNUcastPkts:   /* Multicast frames plus broadcast frames */
        REG_ADD(unit, port, sync_mode, IRMCAr, count); /* + multicast */
        REG_ADD(unit, port, sync_mode, IRBCAr, count); /* + broadcast */
        break;
    case snmpIfInBroadcastPkts: /* broadcast frames */
        REG_ADD(unit, port, sync_mode, IRBCAr, count); /* + broadcast */
        break;
    case snmpIfInMulticastPkts: /* Multicast frames */
        REG_ADD(unit, port, sync_mode, IRMCAr, count); /* + multicast */
        break;
    case snmpIfInDiscards: /* Dropped packets including aborted */
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, RDBGC0r, count); /* Ingress drop conditions */
            BCM_IF_ERROR_RETURN
                (_bcm_stat_counter_non_dma_extra_get(unit,
                                   SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING,
                                                     port, &count1));
            COMPILER_64_ADD_64(count, count1);
        } else {
            REG_ADD(unit, port, sync_mode, IRDISCr, count);
        }
        break;
    case snmpIfInErrors: /* RX Errors or Receive packets - non-error frames */
        REG_ADD(unit, port, sync_mode, IRFCSr, count);
        REG_ADD(unit, port, sync_mode, IRJBRr, count);
        if (COUNT_OVR_ERRORS(unit)) {
            REG_ADD(unit, port, sync_mode, IROVRr, count);
        }
        REG_ADD(unit, port, sync_mode, IRUNDr, count);
        REG_ADD(unit, port, sync_mode, IRFRGr, count);
        break;
    case snmpIfInUnknownProtos: /* Not supported */
        break;
    case snmpIfOutOctets: /* TX bytes */
        REG_ADD(unit, port, sync_mode, ITBYTr, count);
        break;
    case snmpIfOutUcastPkts: /* ALL - mcast - bcast */
        REG_ADD(unit, port, sync_mode, ITPKTr, count);
        REG_SUB(unit, port, sync_mode, ITMCAr, count); /* - multicast */
        REG_SUB(unit, port, sync_mode, ITBCAr, count); /* - broadcast */
        break;
    case snmpIfOutNUcastPkts: /* broadcast frames plus multicast frames */
        REG_ADD(unit, port, sync_mode, ITMCAr, count); /* + multicast */
        REG_ADD(unit, port, sync_mode, ITBCAr, count); /* + broadcast */
        break;
    case snmpIfOutBroadcastPkts: /* broadcast frames */
        REG_ADD(unit, port, sync_mode, ITBCAr, count); /* + broadcast */
        break;
    case snmpIfOutMulticastPkts: /* multicast frames */
        REG_ADD(unit, port, sync_mode, ITMCAr, count); /* + multicast */
        break;
    case snmpIfOutDiscards: /* Aged packet counter */
        if (SOC_REG_IS_VALID(unit, HOLDr)) {
            REG_ADD(unit, port, sync_mode, HOLDr, count); /* L2 MTU drops */
        } else if (SOC_REG_IS_VALID(unit, HOL_DROPr)) {
            REG_ADD(unit, port, sync_mode, HOL_DROPr, count);
        }
        REG_ADD(unit, port, sync_mode, ITAGEr, count);
        BCM_IF_ERROR_RETURN
            (_bcm_stat_counter_extra_get(unit, EGRDROPPKTCOUNTr,
                                         port, &count1));
        COMPILER_64_ADD_64(count, count1);
        BCM_IF_ERROR_RETURN
            (bcm_esw_cosq_stat_get(unit, port, BCM_COS_INVALID, 
                               bcmCosqStatDroppedPackets, &count1));
        COMPILER_64_ADD_64(count, count1);
        break;
    case snmpIfOutErrors:   /* Error packets could not be xmitted */
        /* XGS2 specific */
        break;
    case snmpIfOutQLen: {
        uint32  qcount;
        if (bcm_esw_port_queued_count_get(unit, port, &qcount) >= 0) {
            COMPILER_64_ADD_32(count, qcount);
        }
    }
    break;
    case snmpIpInReceives:
        break;
    case snmpIpInHdrErrors:
        break;
    case snmpIpForwDatagrams:
        break;
    case snmpIpInDiscards:
        break;

        /* *** RFC 1493 *** */

    case snmpDot1dBasePortDelayExceededDiscards:
        break;
    case snmpDot1dBasePortMtuExceededDiscards:
        if (COUNT_OVR_ERRORS(unit)) {
            REG_ADD(unit, port, sync_mode, IROVRr, count);     /* oversize pkts */
            REG_ADD(unit, port, sync_mode, ITOVRr, count);     /* oversize pkts */
        }
        break;
    case snmpDot1dTpPortInFrames:
        REG_ADD(unit, port, sync_mode, IRPKTr, count);
        break;
    case snmpDot1dTpPortOutFrames:
        REG_ADD(unit, port, sync_mode, ITPKTr, count);
        break;
    case snmpDot1dPortInDiscards:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, RDISCr, count);
        } else {
            REG_ADD(unit, port, sync_mode, IRDISCr, count);
        }
        break;

        /* *** RFC 1757 *** */

    case snmpEtherStatsDropEvents:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, RDISCr, count);
        } else {
            REG_ADD(unit, port, sync_mode, IRDISCr, count);
            REG_ADD(unit, port, sync_mode, ITAGEr, count);
        }
        break;
    case snmpEtherStatsOctets:
        REG_ADD(unit, port, sync_mode, IRBYTr, count);
        REG_ADD(unit, port, sync_mode, ITBYTr, count);
        break;
    case snmpEtherStatsPkts:
        REG_ADD(unit, port, sync_mode, IRPKTr, count);
        REG_ADD(unit, port, sync_mode, ITPKTr, count);
        REG_ADD(unit, port, sync_mode, IRUNDr, count); /* Runts */
        REG_ADD(unit, port, sync_mode, IRFRGr, count); /* Fragments */
        break;
    case snmpEtherStatsBroadcastPkts:  /* Broadcast packets (RX/TX) */
        REG_ADD(unit, port, sync_mode, IRBCAr, count); /* + rx broadcast  */
        REG_ADD(unit, port, sync_mode, ITBCAr, count); /* + tx broadcast */
        break;
    case snmpEtherStatsMulticastPkts:  /* Multicast packets (TX+RX) */
        REG_ADD(unit, port, sync_mode, IRMCAr, count); /* + rx multicast */
        REG_ADD(unit, port, sync_mode, ITMCAr, count); /* + tx multicast */
        break;
    case snmpEtherStatsCRCAlignErrors:
        REG_ADD(unit, port, sync_mode, IRFCSr, count);
        break;
    case snmpEtherStatsUndersizePkts:  /* Undersize frames */
        REG_ADD(unit, port, sync_mode, IRUNDr, count);
        break;
    case snmpEtherStatsOversizePkts:
        REG_ADD(unit, port, sync_mode, IROVRr, count);
        REG_ADD(unit, port, sync_mode, ITOVRr, count);
        break;
    case snmpEtherRxOversizePkts:
        REG_ADD(unit, port, sync_mode, IROVRr, count);
        break;
    case snmpEtherTxOversizePkts:
        REG_ADD(unit, port, sync_mode, ITOVRr, count);
        break;
    case snmpEtherStatsFragments:
        REG_ADD(unit, port, sync_mode, IRFRGr, count);
        break;
    case snmpEtherStatsJabbers:
        REG_ADD(unit, port, sync_mode, IRJBRr, count);
        break;
    case snmpEtherStatsCollisions:
        /* always 0 */
        break;
    case snmpEtherStatsPkts64Octets:
        REG_ADD(unit, port, sync_mode, IR64r, count);
        REG_ADD(unit, port, sync_mode, IT64r, count);
        break;
    case snmpEtherStatsPkts65to127Octets:
        REG_ADD(unit, port, sync_mode, IR127r, count);
        REG_ADD(unit, port, sync_mode, IT127r, count);
        break;
    case snmpEtherStatsPkts128to255Octets:
        REG_ADD(unit, port, sync_mode, IR255r, count);
        REG_ADD(unit, port, sync_mode, IT255r, count);
        break;
    case snmpEtherStatsPkts256to511Octets:
        REG_ADD(unit, port, sync_mode, IR511r, count);
        REG_ADD(unit, port, sync_mode, IT511r, count);
        break;
    case snmpEtherStatsPkts512to1023Octets:
        REG_ADD(unit, port, sync_mode, IR1023r, count);
        REG_ADD(unit, port, sync_mode, IT1023r, count);
        break;
    case snmpEtherStatsPkts1024to1518Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, IR1518r, count);
            REG_ADD(unit, port, sync_mode, IT1518r, count);
        } else {
            REG_ADD(unit, port, sync_mode, IR2047r, count);
            REG_ADD(unit, port, sync_mode, IT2047r, count);
        }
        break;

        /* *** not actually in rfc1757 *** */

    case snmpBcmEtherStatsPkts1519to1522Octets:
        break;
    case snmpBcmEtherStatsPkts1522to2047Octets:
        REG_ADD(unit, port, sync_mode, IR2047r, count);
        REG_ADD(unit, port, sync_mode, IT2047r, count);
        break;
    case snmpBcmEtherStatsPkts2048to4095Octets:
        REG_ADD(unit, port, sync_mode, IR4095r, count);
        REG_ADD(unit, port, sync_mode, IT4095r, count);
        break;
    case snmpBcmEtherStatsPkts4095to9216Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, IR9216r, count);
            REG_ADD(unit, port, sync_mode, IT9216r, count);
        } else {
            REG_ADD(unit, port, sync_mode, IR8191r, count);
            REG_ADD(unit, port, sync_mode, IT8191r, count);
            REG_ADD(unit, port, sync_mode, IR16383r, count);
            REG_ADD(unit, port, sync_mode, IT16383r, count);
        }
        break;
    case snmpBcmEtherStatsPkts9217to16383Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, IR16383r, count);
            REG_ADD(unit, port, sync_mode, IT16383r, count);
        }
        break;

    case snmpBcmReceivedPkts64Octets:
        REG_ADD(unit, port, sync_mode, IR64r, count);
        break;
    case snmpBcmReceivedPkts65to127Octets:
        REG_ADD(unit, port, sync_mode, IR127r, count);
        break;
    case snmpBcmReceivedPkts128to255Octets:
        REG_ADD(unit, port, sync_mode, IR255r, count);
        break;
    case snmpBcmReceivedPkts256to511Octets:
        REG_ADD(unit, port, sync_mode, IR511r, count);
        break;
    case snmpBcmReceivedPkts512to1023Octets:
        REG_ADD(unit, port, sync_mode, IR1023r, count);
        break;
    case snmpBcmReceivedPkts1024to1518Octets:
        REG_ADD(unit, port, sync_mode, IR1518r, count);
        break;
    case snmpBcmReceivedPkts1519to2047Octets:
        REG_ADD(unit, port, sync_mode, IR2047r, count);
        break;
    case snmpBcmReceivedPkts2048to4095Octets:
        REG_ADD(unit, port, sync_mode, IR4095r, count);
        break;
    case snmpBcmReceivedPkts4095to9216Octets:
        REG_ADD(unit, port, sync_mode, IR9216r, count);
        break;
    case snmpBcmReceivedPkts9217to16383Octets:
        REG_ADD(unit, port, sync_mode, IR16383r, count);
        break;

    case snmpBcmTransmittedPkts64Octets:
        REG_ADD(unit, port, sync_mode, IT64r, count);
        break;
    case snmpBcmTransmittedPkts65to127Octets:
        REG_ADD(unit, port, sync_mode, IT127r, count);
        break;
    case snmpBcmTransmittedPkts128to255Octets:
        REG_ADD(unit, port, sync_mode, IT255r, count);
        break;
    case snmpBcmTransmittedPkts256to511Octets:
        REG_ADD(unit, port, sync_mode, IT511r, count);
        break;
    case snmpBcmTransmittedPkts512to1023Octets:
        REG_ADD(unit, port, sync_mode, IT1023r, count);
        break;
    case snmpBcmTransmittedPkts1024to1518Octets:
        REG_ADD(unit, port, sync_mode, IT1518r, count);
        break;
    case snmpBcmTransmittedPkts1519to2047Octets:
        REG_ADD(unit, port, sync_mode, IT2047r, count);
        break;
    case snmpBcmTransmittedPkts2048to4095Octets:
        REG_ADD(unit, port, sync_mode, IT4095r, count);
        break;
    case snmpBcmTransmittedPkts4095to9216Octets:
        REG_ADD(unit, port, sync_mode, IT9216r, count);
        break;
    case snmpBcmTransmittedPkts9217to16383Octets:
        REG_ADD(unit, port, sync_mode, IT16383r, count);
        break;

    case snmpEtherStatsTXNoErrors:
        REG_ADD(unit, port, sync_mode, ITPKTr, count);
        REG_SUB(unit, port, sync_mode, ITFRGr, count);
        if (COUNT_OVR_ERRORS(unit)) {
            REG_SUB(unit, port, sync_mode, ITOVRr, count);
        }
        REG_SUB(unit, port, sync_mode, ITUFLr, count);
        REG_SUB(unit, port, sync_mode, ITERRr, count);
        break;
    case snmpEtherStatsRXNoErrors: /* RPKT - ( RFCS + RXUO + RFLR) */
        REG_ADD(unit, port, sync_mode, IRPKTr, count);
        REG_SUB(unit, port, sync_mode, IRFCSr, count);
        REG_SUB(unit, port, sync_mode, IRJBRr, count);
        if (COUNT_OVR_ERRORS(unit)) {
            REG_SUB(unit, port, sync_mode, IROVRr, count);
        }
        REG_SUB(unit, port, sync_mode, IRUNDr, count);
        REG_SUB(unit, port, sync_mode, IRFRGr, count);
        REG_SUB(unit, port, sync_mode, IRERPKTr, count);
        break;

        /* *** RFC 2665 *** */

    case snmpDot3StatsAlignmentErrors:
        break;
    case snmpDot3StatsFCSErrors:
        REG_ADD(unit, port, sync_mode, IRFCSr, count);
        break;
    case snmpDot3StatsSingleCollisionFrames:
        break;
    case snmpDot3StatsMultipleCollisionFrames:
        break;
    case snmpDot3StatsSQETTestErrors:
        /* always 0 */
        break;
    case snmpDot3StatsDeferredTransmissions:
        break;
    case snmpDot3StatsLateCollisions:
        break;
    case snmpDot3StatsExcessiveCollisions:
        break;
    case snmpDot3StatsInternalMacTransmitErrors:
        REG_ADD(unit, port, sync_mode, ITUFLr, count);
        REG_ADD(unit, port, sync_mode, ITERRr, count);
        break;
    case snmpDot3StatsCarrierSenseErrors:
        break;
    case snmpDot3StatsFrameTooLongs:
        if (SOC_REG_IS_VALID(unit, IRMEGr) && SOC_REG_IS_VALID(unit, IRMEBr)) {
             REG_ADD(unit, port, sync_mode, IRMEGr, count);
             REG_ADD(unit, port, sync_mode, IRMEBr, count);
        } else {
             REG_ADD(unit, port, sync_mode, IRJBRr, count);
        }
    break;
    case snmpDot3StatsInternalMacReceiveErrors:
        break;
    case snmpDot3StatsSymbolErrors:
        REG_ADD(unit, port, sync_mode, IRERBYTr, count);
        break;
    case snmpDot3ControlInUnknownOpcodes:
        REG_ADD(unit, port, sync_mode, IRXUOr, count);
        break;
    case snmpDot3InPauseFrames:
        REG_ADD(unit, port, sync_mode, IRXPFr, count);
        break;
    case snmpDot3OutPauseFrames:
        REG_ADD(unit, port, sync_mode, ITXPFr, count);
        break;

        /* *** RFC 2233 high capacity versions of RFC1213 objects *** */

    case snmpIfHCInOctets:
        REG_ADD(unit, port, sync_mode, IRBYTr, count);
        break;
    case snmpIfHCInUcastPkts:
        if (soc_feature(unit, soc_feature_hw_stats_calc)) {
            if (SOC_REG_IS_VALID(unit, IRUCAr)) {
                REG_ADD(unit, port, sync_mode, IRUCAr, count);
            } else if (SOC_REG_IS_VALID(unit, IRUCr)) {
                REG_ADD(unit, port, sync_mode, IRUCr, count);
            } else {
                REG_ADD(unit, port, sync_mode, RUCr, count);  /* unicast pkts rcvd */
            }
        } else {
            REG_ADD(unit, port, sync_mode, IRPKTr, count);
            REG_SUB(unit, port, sync_mode, IRMCAr, count);
            REG_SUB(unit, port, sync_mode, IRBCAr, count);
            REG_SUB(unit, port, sync_mode, IRFCSr, count); /* - bad FCS */
            REG_SUB(unit, port, sync_mode, IRXCFr, count); /* - good FCS, all MAC ctrl */
            REG_SUB(unit, port, sync_mode, IRJBRr, count); /* - oversize, bad FCS */
            if (COUNT_OVR_ERRORS(unit)) {
                REG_SUB(unit, port, sync_mode, IROVRr, count); /* - oversize, good FCS */
            }
        }
            break;
    case snmpIfHCInMulticastPkts:
        REG_ADD(unit, port, sync_mode, IRMCAr, count);
        break;
    case snmpIfHCInBroadcastPkts:
        REG_ADD(unit, port, sync_mode, IRBCAr, count);
        break;
    case snmpIfHCOutOctets:
        REG_ADD(unit, port, sync_mode, ITBYTr, count);
        break;
    case snmpIfHCOutUcastPkts:
        REG_ADD(unit, port, sync_mode, ITPKTr, count);
        REG_SUB(unit, port, sync_mode, ITMCAr, count);
        REG_SUB(unit, port, sync_mode, ITBCAr, count);
        break;
    case snmpIfHCOutMulticastPkts:
        REG_ADD(unit, port, sync_mode, ITMCAr, count);
        break;
    case snmpIfHCOutBroadcastPckts:
        REG_ADD(unit, port, sync_mode, ITBCAr, count);
        break;

        /* *** RFC 2465 *** */

    case snmpIpv6IfStatsInReceives:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, RIPC6r, count);
            REG_ADD(unit, port, sync_mode, IMRP6r, count);
        }
        break;
    case snmpIpv6IfStatsInHdrErrors:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, RIPD6r, count);
        }
        break;
    case snmpIpv6IfStatsInAddrErrors:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, RIPHE6r, count);
        }
        break;
    case snmpIpv6IfStatsInDiscards:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
             REG_ADD(unit, port, sync_mode, RIPD6r, count);
        }
        break;
    case snmpIpv6IfStatsOutForwDatagrams:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, TDBGC0r, count);
        }
        break;
    case snmpIpv6IfStatsOutDiscards:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, TDBGC1r, count);
        }
        break;
    case snmpIpv6IfStatsInMcastPkts:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
             REG_ADD(unit, port, sync_mode, IMRP6r, count);
        }
        break;
    case snmpIpv6IfStatsOutMcastPkts:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, TDBGC2r, count);
        }
        break;

        /* *** IEEE 802.1bb *** */
    case snmpIeee8021PfcRequests:
        REG_ADD(unit, port, sync_mode, ITXPPr, count);
        break;
    case snmpIeee8021PfcIndications:
        REG_ADD(unit, port, sync_mode, IRXPPr, count);
        break;

        /* IPMC counters (broadcom specific) */

    case snmpBcmIPMCBridgedPckts:
        break;
    case snmpBcmIPMCRoutedPckts:
        break;
    case snmpBcmIPMCInDroppedPckts:
        break;
    case snmpBcmIPMCOutDroppedPckts:
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
                                "_bcm_stat_hg_get: Statistic not supported: %d\n"), type));
        return BCM_E_PARAM;
    }

    *val = count;

    return BCM_E_NONE;
}
