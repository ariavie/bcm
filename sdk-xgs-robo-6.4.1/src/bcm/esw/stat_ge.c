/*
 * $Id: stat_ge.c,v 1.78 Broadcom SDK $
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
 *      _bcm_stat_ge_get
 * Description:
 *      Get the specified statistic for a GE port in 1000 Mb mode on the
 *      StrataSwitch family of devices.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      port - zero-based port number
 *      sync_mode - if 1 read hw counter else sw accumualated counter
 *      type - SNMP statistics type (see bcm/stat.h)
 *      val  - (OUT) 64 bit statistic counter value
 * Returns:
 *      BCM_E_NONE - Success.
 *      BCM_E_PARAM - Illegal parameter.
 *      BCM_E_INTERNAL - Chip access failure.
 */
int
_bcm_stat_ge_get(int unit, bcm_port_t port, int sync_mode,
                 bcm_stat_val_t type, uint64 *val, int incl_non_ge_stat)
{
    uint64 count, count1;
    REG_MATH_DECL;       /* Required for use of the REG_* macros */

    COMPILER_REFERENCE(&count);  /* Work around PPC compiler bug */
    COMPILER_64_ZERO(count);

    switch (type) {
        /* *** RFC 1213 *** */

    case snmpIfInOctets:
        REG_ADD(unit, port, sync_mode, GRBYTr, count);  /* bytes rcvd */
        REG_ADD(unit, port, sync_mode, RRBYTr, count); /* Runt bytes */
        break;
    case snmpIfInUcastPkts:    /* Unicast packets received */
        if (soc_feature(unit, soc_feature_hw_stats_calc)) {
            if (SOC_REG_IS_VALID(unit, GRUCr)
                && !((SOC_IS_RAPTOR(unit) || SOC_IS_RAVEN(unit)) 
                      && SOC_IS_STACK_PORT(unit, port))) {
                REG_ADD(unit, port, sync_mode, GRUCr, count);
            } else {
                if (incl_non_ge_stat) {
                    /* RUC switch register has the count of all received packets, 
                     * add the count when incl_non_ge_stat flag is set.
                     */
                    REG_ADD(unit, port, sync_mode, RUCr, count);  /* unicast pkts rcvd */
                }
            }
        } else {
            REG_ADD(unit, port, sync_mode, GRPKTr, count); /* all pkts rcvd */
            REG_SUB(unit, port, sync_mode, GRMCAr, count); /* - multicast */
            REG_SUB(unit, port, sync_mode, GRBCAr, count); /* - broadcast */
            REG_SUB(unit, port, sync_mode, GRALNr, count); /* - bad FCS, dribble bit  */
            REG_SUB(unit, port, sync_mode, GRFCSr, count); /* - bad FCS, no dribble bit */
            REG_SUB(unit, port, sync_mode, GRJBRr, count); /* - oversize, bad FCS */
            if (SOC_REG_IS_VALID(unit, GRMTUEr)) {
                REG_SUB(unit, port, sync_mode, GRMTUEr, count); /* - mtu exceeded, good FCS */
                if (SOC_REG_IS_VALID(unit, GROVRr)) {
                   REG_SUB(unit, port, sync_mode, GROVRr, count);
                }

            } else if (COUNT_OVR_ERRORS(unit)) {
                REG_SUB(unit, port, sync_mode, GROVRr, count); /* - oversize, good FCS */
            }
        }
        break;
    case snmpIfInNUcastPkts:    /* Non Unicast packets received */
        REG_ADD(unit, port, sync_mode, GRMCAr, count); /* multicast pkts rcvd */
        REG_ADD(unit, port, sync_mode, GRBCAr, count); /* broadcast pkts rcvd */
        break;
    case snmpIfInBroadcastPkts: /* Broadcast packets received */
        REG_ADD(unit, port, sync_mode, GRBCAr, count);  /* broadcast pkts rcvd */
        break;
    case snmpIfInMulticastPkts:  /* Multicast packets received */
        REG_ADD(unit, port, sync_mode, GRMCAr, count);  /* multicast pkts rcvd */
        break;
    case snmpIfInDiscards:    /* Dropped packets including aborted */
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, RDBGC0r, count); /* Ingress drop conditions */
                BCM_IF_ERROR_RETURN
                    (_bcm_stat_counter_non_dma_extra_get(unit,
                                   SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING,
                                                         port, &count1));
                COMPILER_64_ADD_64(count, count1);
            }
        }
        break;
    case snmpIfInErrors:    /* Receive packets containing errors */
        REG_ADD(unit, port, sync_mode, GRUNDr, count);  /* undersize pkts, good FCS */
        REG_ADD(unit, port, sync_mode, GRFRGr, count);  /* undersize pkts, bad FCS */
        REG_ADD(unit, port, sync_mode, GRFCSr, count);  /* FCS errors */
        if (SOC_REG_IS_VALID(unit, GRMTUEr)) {
            REG_ADD(unit, port, sync_mode, GRMTUEr, count); /* mtu exceeded pkts, good FCS */
        } else if (COUNT_OVR_ERRORS(unit)) {
            REG_ADD(unit, port, sync_mode, GROVRr, count);    /* oversize pkts, good FCS */
        }
        REG_ADD(unit, port, sync_mode, GRJBRr, count);  /* oversize pkts, bad FCS */
        break;
    case snmpIfInUnknownProtos:
        break;
    case snmpIfOutOctets:
        REG_ADD(unit, port, sync_mode, GTBYTr, count);  /* transmit bytes */
        break;
    case snmpIfOutUcastPkts:
#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || \
    defined(BCM_TRX_SUPPORT)
        if (soc_feature(unit, soc_feature_hw_stats_calc)) {
            REG_ADD(unit, port, sync_mode, GTUCr, count);  /* unicast pkts sent */
        } else 
#endif /* BCM_RAPTOR_SUPPORT || BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */
        {
            REG_ADD(unit, port, sync_mode, GTPKTr, count); /* all pkts xmited */
            REG_SUB(unit, port, sync_mode, GTMCAr, count); /* - multicast */
            REG_SUB(unit, port, sync_mode, GTBCAr, count); /* - broadcast */
            REG_SUB(unit, port, sync_mode, GTFCSr, count); /* - bad FCS */
            REG_SUB(unit, port, sync_mode, GTJBRr, count); /* - oversize, bad FCS */
            if (COUNT_OVR_ERRORS(unit)) {
                REG_SUB(unit, port, sync_mode, GTOVRr, count); /* - oversize, good FCS */
            }
        }
        break;
    case snmpIfOutNUcastPkts:
        REG_ADD(unit, port, sync_mode, GTMCAr, count);  /* multicast pkts */
        REG_ADD(unit, port, sync_mode, GTBCAr, count);  /* broadcast pkts */
        break;
    case snmpIfOutBroadcastPkts:
        REG_ADD(unit, port, sync_mode, GTBCAr, count);  /* broadcast pkts */
        break;
    case snmpIfOutMulticastPkts:
        REG_ADD(unit, port, sync_mode, GTMCAr, count);  /* multicast pkts */
        break;
    case snmpIfOutDiscards:
        REG_ADD(unit, port, sync_mode, GTEDFr, count);  /* multiple deferral */
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                if (SOC_REG_IS_VALID(unit, HOLDr)) {  /* L2 MTU drops */
                    REG_ADD(unit, port, sync_mode, HOLDr, count);
                } else if (SOC_REG_IS_VALID(unit, HOL_DROPr)) {
                    REG_ADD(unit, port, sync_mode, HOL_DROPr, count);
                }
                REG_ADD(unit, port, sync_mode, TDBGC3r, count);
                BCM_IF_ERROR_RETURN
                    (_bcm_stat_counter_extra_get(unit, EGRDROPPKTCOUNTr,
                                                 port, &count1));
                COMPILER_64_ADD_64(count, count1);
                BCM_IF_ERROR_RETURN
                    (bcm_esw_cosq_stat_get(unit, port, BCM_COS_INVALID, 
                                       bcmCosqStatDroppedPackets, &count1));
                COMPILER_64_ADD_64(count, count1);
            }
        }
        /* other causes of discards? */
        break;
    case snmpIfOutErrors:
        REG_ADD(unit, port, sync_mode, GTXCLr, count);  /* excessive collisions */
        break;
    case snmpIfOutQLen: {
        uint32  qcount;
        if (incl_non_ge_stat) {
            if (bcm_esw_port_queued_count_get(unit, port, &qcount) >= 0) { 
                COMPILER_64_ADD_32(count, qcount);
            }
        }
    }
    break;
    case snmpIpInReceives:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, RIPC4r, count);
            }
        }
        break;
    case snmpIpInHdrErrors:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, RIPD4r, count);
            }
        }
        break;
/* ipInAddrErrors */
    case snmpIpForwDatagrams:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, TDBGC4r, count);
            }
        }
        break;
/* ipInUnknownProtos */
    case snmpIpInDiscards:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, RIPHE4r, count);
                REG_ADD(unit, port, sync_mode, RIPD4r, count);
            }
        }
        break;
/* ipInDelivers */
/* ipOutRequests */
/* ipOutDiscards */
/* ipOutNoRoutes */

        /* *** RFC 1493 *** */

    case snmpDot1dBasePortDelayExceededDiscards:
        REG_ADD(unit, port, sync_mode, GTEDFr, count);  /* multiple deferral */
        break;
    case snmpDot1dBasePortMtuExceededDiscards:
        if (SOC_REG_IS_VALID(unit, GRMTUEr)) {
            REG_ADD(unit, port, sync_mode, GRMTUEr, count); /* mtu exceeded pkts */
        } else if (COUNT_OVR_ERRORS(unit)) {
            REG_ADD(unit, port, sync_mode, GROVRr, count);  /* oversize pkts */
        }
        break;
    case snmpDot1dTpPortInFrames:  /* should be only bridge mgmt */
        REG_ADD(unit, port, sync_mode, GRPKTr, count);
        break;
    case snmpDot1dTpPortOutFrames:  /* should be only bridge mgmt */
        REG_ADD(unit, port, sync_mode, GTPKTr, count);
        break;
    case snmpDot1dPortInDiscards:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, RDISCr, count); 
                REG_ADD(unit, port, sync_mode, RIPD4r, count); 
                REG_ADD(unit, port, sync_mode, RIPD6r, count); 
                REG_ADD(unit, port, sync_mode, RPORTDr, count);
            }
        }
        break;

        /* *** RFC 1757 *** */

    case snmpEtherStatsDropEvents:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, RDISCr, count); 
            }
        }
        break;
    case snmpEtherStatsOctets:
        REG_ADD(unit, port, sync_mode, GRBYTr, count);
        REG_ADD(unit, port, sync_mode, GTBYTr, count);
        if (SOC_REG_IS_VALID(unit, GRRBYTr)) {
           REG_ADD(unit,port,sync_mode,GRRBYTr,count); /* Runt bytes */
        } else { 
           REG_ADD(unit, port, sync_mode, RRBYTr, count); /* Runt bytes */
        }
        break;
    case snmpEtherStatsPkts:
        REG_ADD(unit, port, sync_mode, GRPKTr, count);
        REG_ADD(unit, port, sync_mode, GTPKTr, count);
        if (SOC_REG_IS_VALID(unit, GRRPKTr)) {
           REG_ADD(unit, port, sync_mode, GRRPKTr, count); /* Runt packets */
        } else {
           REG_ADD(unit, port, sync_mode, RRPKTr, count); /* Runt packets */
        }
        break;
    case snmpEtherStatsBroadcastPkts:
        REG_ADD(unit, port, sync_mode, GRBCAr, count);
        REG_ADD(unit, port, sync_mode, GTBCAr, count);
        break;
    case snmpEtherStatsMulticastPkts:
        REG_ADD(unit, port, sync_mode, GRMCAr, count);
        REG_ADD(unit, port, sync_mode, GTMCAr, count);
        break;
    case snmpEtherStatsCRCAlignErrors:  /* CRC errors + alignment errors */
        REG_ADD(unit, port, sync_mode, GRFCSr, count);
        break;
    case snmpEtherStatsUndersizePkts:  /* Undersize frames */
        REG_ADD(unit, port, sync_mode, GRUNDr, count);
        break;
    case snmpEtherStatsOversizePkts:
        if (soc_feature(unit, soc_feature_stat_jumbo_adj)) {
            REG_ADD(unit, port, sync_mode, GROVRr, count);
            REG_ADD(unit, port, sync_mode, GTOVRr, count);
        } else {
            REG_ADD(unit, port, sync_mode, GRJBRr, count);
            REG_ADD(unit, port, sync_mode, GTJBRr, count);
        }
        break;
    case snmpEtherRxOversizePkts:
        if (soc_feature(unit, soc_feature_stat_jumbo_adj)) {
            REG_ADD(unit, port, sync_mode, GROVRr, count);
        }
        break;
    case snmpEtherTxOversizePkts:
        if (soc_feature(unit, soc_feature_stat_jumbo_adj)) {
            REG_ADD(unit, port, sync_mode, GTOVRr, count);
        }
        break;
    case snmpEtherStatsFragments:
        REG_ADD(unit, port, sync_mode, GRFRGr, count);
        REG_ADD(unit, port, sync_mode, GTFRGr, count);
        break;
    case snmpEtherStatsJabbers:
        REG_ADD(unit, port, sync_mode, GRJBRr, count);
        REG_ADD(unit, port, sync_mode, GTJBRr, count);
        break;
    case snmpEtherStatsCollisions:
        REG_ADD(unit, port, sync_mode, GTNCLr, count);
        break;
    case snmpEtherStatsPkts64Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {  
            REG_ADD(unit, port, sync_mode, GR64r, count);
            REG_ADD(unit, port, sync_mode, GT64r, count);
        }
        break;
    case snmpEtherStatsPkts65to127Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, GR127r, count);
            REG_ADD(unit, port, sync_mode, GT127r, count);
        }
        break;
    case snmpEtherStatsPkts128to255Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, GR255r, count);
            REG_ADD(unit, port, sync_mode, GT255r, count);
        }
        break;
    case snmpEtherStatsPkts256to511Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, GR511r, count);
            REG_ADD(unit, port, sync_mode, GT511r, count);
        }
        break;
    case snmpEtherStatsPkts512to1023Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, GR1023r, count);
            REG_ADD(unit, port, sync_mode, GT1023r, count);
        }
        break;
    case snmpEtherStatsPkts1024to1518Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, GR1518r, count);
            REG_ADD(unit, port, sync_mode, GT1518r, count);
        }
        break;

        /* *** not actually in rfc1757 *** */

    case snmpBcmEtherStatsPkts1519to1522Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, GRMGVr, count);
            REG_ADD(unit, port, sync_mode, GTMGVr, count);
        }
        break;
    case snmpBcmEtherStatsPkts1522to2047Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, GR2047r, count);
            REG_SUB(unit, port, sync_mode, GRMGVr, count);

            REG_ADD(unit, port, sync_mode, GT2047r, count);
            REG_SUB(unit, port, sync_mode, GTMGVr, count);
        }
        break;
    case snmpBcmEtherStatsPkts2048to4095Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, GR4095r, count);
            REG_ADD(unit, port, sync_mode, GT4095r, count);
        }
        break;
    case snmpBcmEtherStatsPkts4095to9216Octets:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            REG_ADD(unit, port, sync_mode, GR9216r, count);
            REG_ADD(unit, port, sync_mode, GT9216r, count);
        }
        break;
#ifdef BCM_KATANA_SUPPORT
    case snmpBcmEtherStatsPkts9217to16383Octets: /* not in rfc1757 */
        if(SOC_IS_KATANA(unit))
        {
            if (soc_feature(unit, soc_feature_stat_xgs3)) {
               REG_ADD(unit, port, sync_mode, GR16383r, count);
               REG_ADD(unit, port, sync_mode, GT16383r, count);
            }
        }
        break;
#endif
    case snmpBcmReceivedPkts64Octets:
        REG_ADD(unit, port, sync_mode, GR64r, count);
        break;
    case snmpBcmReceivedPkts65to127Octets:
        REG_ADD(unit, port, sync_mode, GR127r, count);
        break;
    case snmpBcmReceivedPkts128to255Octets:
        REG_ADD(unit, port, sync_mode, GR255r, count);
        break;
    case snmpBcmReceivedPkts256to511Octets:
        REG_ADD(unit, port, sync_mode, GR511r, count);
        break;
    case snmpBcmReceivedPkts512to1023Octets:
        REG_ADD(unit, port, sync_mode, GR1023r, count);
        break;
    case snmpBcmReceivedPkts1024to1518Octets:
        REG_ADD(unit, port, sync_mode, GR1518r, count);
        break;
    case snmpBcmReceivedPkts1519to2047Octets:
        REG_ADD(unit, port, sync_mode, GR2047r, count);
        break;
    case snmpBcmReceivedPkts2048to4095Octets:
        REG_ADD(unit, port, sync_mode, GR4095r, count);
        break;
    case snmpBcmReceivedPkts4095to9216Octets:
        REG_ADD(unit, port, sync_mode, GR9216r, count);
        break;
#ifdef BCM_KATANA_SUPPORT
    case snmpBcmReceivedPkts9217to16383Octets:
        if(SOC_IS_KATANA(unit))
        {
            REG_ADD(unit, port, sync_mode, GR16383r, count);
        }
        break;
#endif

    case snmpBcmTransmittedPkts64Octets:
        REG_ADD(unit, port, sync_mode, GT64r, count);
        break;
    case snmpBcmTransmittedPkts65to127Octets:
        REG_ADD(unit, port, sync_mode, GT127r, count);
        break;
    case snmpBcmTransmittedPkts128to255Octets:
        REG_ADD(unit, port, sync_mode, GT255r, count);
        break;
    case snmpBcmTransmittedPkts256to511Octets:
        REG_ADD(unit, port, sync_mode, GT511r, count);
        break;
    case snmpBcmTransmittedPkts512to1023Octets:
        REG_ADD(unit, port, sync_mode, GT1023r, count);
        break;
    case snmpBcmTransmittedPkts1024to1518Octets:
        REG_ADD(unit, port, sync_mode, GT1518r, count);
        break;
    case snmpBcmTransmittedPkts1519to2047Octets:
        REG_ADD(unit, port, sync_mode, GT2047r, count);
        break;
    case snmpBcmTransmittedPkts2048to4095Octets:
        REG_ADD(unit, port, sync_mode, GT4095r, count);
        break;
    case snmpBcmTransmittedPkts4095to9216Octets:
        REG_ADD(unit, port, sync_mode, GT9216r, count);
        break;
#ifdef BCM_KATANA_SUPPORT
    case snmpBcmTransmittedPkts9217to16383Octets:
        if(SOC_IS_KATANA(unit))
        {
            REG_ADD(unit, port, sync_mode, GT16383r, count);
        }
        break;
#endif
    case snmpEtherStatsTXNoErrors:
        /* FE = TPKT - (TNCL + TOVR + TFRG + TUND) */
        /* GE = GTPKT - (GTOVR + GTIPD + GTABRT) */
#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || \
    defined(BCM_TRX_SUPPORT)
        if (soc_feature(unit, soc_feature_hw_stats_calc)) {
            REG_ADD(unit, port, sync_mode, GTPOKr, count); /* All good packets */
        } else 
#endif /* BCM_RAPTOR_SUPPORT || BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */
        {
            REG_ADD(unit, port, sync_mode, GTPKTr, count); /* All Packets */
            if (COUNT_OVR_ERRORS(unit)) {
                REG_SUB(unit, port, sync_mode, GTOVRr, count); /* Oversize */
            }
        }
        break;
    case snmpEtherStatsRXNoErrors:
#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || \
    defined(BCM_TRX_SUPPORT)
        /* Some chips have a dedicated register for this stat */
        if (soc_feature(unit, soc_feature_hw_stats_calc)) {
            REG_ADD(unit, port, sync_mode, GRPOKr, count);
        } else 
#endif /* BCM_RAPTOR_SUPPORT || BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */
        {
            /*
             * GE = GRPKT - (GRFCS + GRXUO + GRFLR + GRCDE + GRFCR +
             *                GRUND + GROVR + GRJBR + GRIPCHK/GRIPHE)
             */
    
            REG_ADD(unit, port, sync_mode, GRPKTr, count);
            REG_SUB(unit, port, sync_mode, GRFCSr, count);
            REG_SUB(unit, port, sync_mode, GRXUOr, count);
    
            REG_SUB(unit, port, sync_mode, GRCDEr, count);
            REG_SUB(unit, port, sync_mode, GRFCRr, count);
            REG_SUB(unit, port, sync_mode, GRUNDr, count);
            if (SOC_REG_IS_VALID(unit, GRMTUEr)) {
                REG_SUB(unit, port, sync_mode, GRMTUEr, count); /* mtu exceeded pkts */
            } else if (COUNT_OVR_ERRORS(unit)) {
                REG_SUB(unit, port, sync_mode, GROVRr, count);  /* oversize pkts */
            }
            REG_SUB(unit, port, sync_mode, GRJBRr, count);
        }
        break;
        /* *** RFC 2665 *** */

    case snmpDot3StatsAlignmentErrors:
        REG_ADD(unit, port, sync_mode, GRALNr, count);
        break;
    case snmpDot3StatsFCSErrors:
        REG_ADD(unit, port, sync_mode, GRFCSr, count);
        break;
    case snmpDot3StatsSingleCollisionFrames:
        REG_ADD(unit, port, sync_mode, GTSCLr, count);
        break;
    case snmpDot3StatsMultipleCollisionFrames:
        REG_ADD(unit, port, sync_mode, GTMCLr, count);
        break;
    case snmpDot3StatsSQETTestErrors:
        /* always 0 */
        break;
    case snmpDot3StatsDeferredTransmissions:
        REG_ADD(unit, port, sync_mode, GTDFRr, count);
        break;
    case snmpDot3StatsLateCollisions:
        REG_ADD(unit, port, sync_mode, GTLCLr, count);
        break;
    case snmpDot3StatsExcessiveCollisions:
        REG_ADD(unit, port, sync_mode, GTXCLr, count);
        break;
    case snmpDot3StatsInternalMacTransmitErrors:
        /* always 0 */
        break;
    case snmpDot3StatsCarrierSenseErrors:
        REG_ADD(unit, port, sync_mode, GRFCRr, count);
        break;
    case snmpDot3StatsFrameTooLongs:
        if (soc_feature(unit, soc_feature_stat_jumbo_adj)) {
            if (SOC_REG_IS_VALID(unit, GRMTUEr)) {
                REG_ADD(unit, port, sync_mode, GRMTUEr, count); /* mtu exceeded pkts */
            } else {
                REG_ADD(unit, port, sync_mode, GROVRr, count); /* oversized pkts */
            }
        } else {
            REG_ADD(unit, port, sync_mode, GRJBRr, count);
        }
        break;
    case snmpDot3StatsInternalMacReceiveErrors:
        /* always 0 */
        break;
    case snmpDot3StatsSymbolErrors:
        REG_ADD(unit, port, sync_mode, GRCDEr, count);
        break;
    case snmpDot3ControlInUnknownOpcodes:
        REG_ADD(unit, port, sync_mode, GRXUOr, count);
        break;
    case snmpDot3InPauseFrames:
        REG_ADD(unit, port, sync_mode, GRXPFr, count);
        break;
    case snmpDot3OutPauseFrames:
        if (incl_non_ge_stat) {
            REG_ADD(unit, port, sync_mode, GTXPFr, count);
        } /* Else avoid double-counting of pause frames on GXMACs */
        break;

        /* *** RFC 2233 high capacity versions of RFC1213 objects *** */

    case snmpIfHCInOctets:
        REG_ADD(unit, port, sync_mode, GRBYTr, count);  /* bytes rcvd */
        REG_ADD(unit, port, sync_mode, RRBYTr, count); /* Runt bytes */
        break;
    case snmpIfHCInUcastPkts:
        if (soc_feature(unit, soc_feature_hw_stats_calc)) {
            if (SOC_REG_IS_VALID(unit, GRUCr)
                && !((SOC_IS_RAPTOR(unit) || SOC_IS_RAVEN(unit)) 
                      && SOC_IS_STACK_PORT(unit, port))) {
                    REG_ADD(unit, port, sync_mode, GRUCr, count);
            } else {
                if (incl_non_ge_stat) {
                    /* RUC switch register has the count of all received packets, 
                     * add the count when incl_non_ge_stat flag is set.
                     */
                    REG_ADD(unit, port, sync_mode, RUCr, count);  /* unicast pkts rcvd */
                }
            }
        } else {
            REG_ADD(unit, port, sync_mode, GRPKTr, count); /* all pkts rcvd */
            REG_SUB(unit, port, sync_mode, GRMCAr, count); /* - multicast */
            REG_SUB(unit, port, sync_mode, GRBCAr, count); /* - broadcast */
            REG_SUB(unit, port, sync_mode, GRALNr, count); /* - bad FCS, dribble bit  */
            REG_SUB(unit, port, sync_mode, GRFCSr, count); /* - bad FCS, no dribble bit */
            REG_SUB(unit, port, sync_mode, GRJBRr, count); /* - oversize, bad FCS */
            if (SOC_REG_IS_VALID(unit, GRMTUEr)) {
                REG_SUB(unit, port, sync_mode, GRMTUEr, count); /* mtu exceeded pkts */
            } else if (COUNT_OVR_ERRORS(unit)) {
                REG_SUB(unit, port, sync_mode, GROVRr, count);  /* oversize pkts */
            }
        }
        break;
    case snmpIfHCInMulticastPkts:
        REG_ADD(unit, port, sync_mode, GRMCAr, count);  /* multicast */
        break;
    case snmpIfHCInBroadcastPkts:
        REG_ADD(unit, port, sync_mode, GRBCAr, count);  /* broadcast */
        break;
    case snmpIfHCOutOctets:
        REG_ADD(unit, port, sync_mode, GTBYTr, count);  /* transmit bytes */
        break;
    case snmpIfHCOutUcastPkts:
#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || \
    defined(BCM_TRX_SUPPORT)
        if (soc_feature(unit, soc_feature_hw_stats_calc)) {
            REG_ADD(unit, port, sync_mode, GTUCr, count); /* All good packets */
        } else 
#endif /* BCM_RAPTOR_SUPPORT || BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */
        { 
            REG_ADD(unit, port, sync_mode, GTPKTr, count); /* all pkts xmited */
            REG_SUB(unit, port, sync_mode, GTMCAr, count); /* - multicast */
            REG_SUB(unit, port, sync_mode, GTBCAr, count); /* - broadcast */
            REG_SUB(unit, port, sync_mode, GTFCSr, count); /* - bad FCS */
            REG_SUB(unit, port, sync_mode, GTJBRr, count); /* - oversize, bad FCS */
            if (COUNT_OVR_ERRORS(unit)) {
                REG_SUB(unit, port, sync_mode, GTOVRr, count); /* - oversize, good FCS */
            }
        }
        break;
    case snmpIfHCOutMulticastPkts:
        REG_ADD(unit, port, sync_mode, GTMCAr, count);  /* multicast xmited */
        break;
    case snmpIfHCOutBroadcastPckts:
        REG_ADD(unit, port, sync_mode, GTBCAr, count);  /* broadcast xmited */
        break;

        /* *** RFC 2465 *** */

    case snmpIpv6IfStatsInReceives:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, RIPC6r, count);
                REG_ADD(unit, port, sync_mode, IMRP6r, count);
            }
        }
        break;
    case snmpIpv6IfStatsInHdrErrors:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, RIPD6r, count);
            }
        }
        break;
    case snmpIpv6IfStatsInAddrErrors:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, RIPHE6r, count);
            }
        }
        break;
    case snmpIpv6IfStatsInDiscards:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, RIPHE6r, count);
                REG_ADD(unit, port, sync_mode, RIPD6r, count);
            }
        }
        break;
    case snmpIpv6IfStatsOutForwDatagrams:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, TDBGC0r, count);
            }
        }
        break;
    case snmpIpv6IfStatsOutDiscards:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, TDBGC1r, count);
            }
        }
        break;
    case snmpIpv6IfStatsInMcastPkts:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, IMRP6r, count);
            }
        }
        break;
    case snmpIpv6IfStatsOutMcastPkts:
        if (soc_feature(unit, soc_feature_stat_xgs3)) {
            if (incl_non_ge_stat) {
                REG_ADD(unit, port, sync_mode, TDBGC2r, count);
            }
        }
        break;

        /* *** IEEE 802.1bb *** */
    case snmpIeee8021PfcRequests:
        REG_ADD(unit, port, sync_mode, GTXPPr, count);
        break;
    case snmpIeee8021PfcIndications:
        REG_ADD(unit, port, sync_mode, GRXPPr, count);
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
        if (soc_feature(unit, soc_feature_ip_mcast)) {
            if (soc_feature(unit, soc_feature_stat_xgs3)) {
                if (incl_non_ge_stat) {
                    REG_ADD(unit, port, sync_mode, RDBGC1r, count);
                }
            }
        }
        break;
    case snmpBcmIPMCRoutedPckts:
        if (soc_feature(unit, soc_feature_ip_mcast)) {
            if (soc_feature(unit, soc_feature_stat_xgs3)) {
                if (incl_non_ge_stat) {
                    REG_ADD(unit, port, sync_mode, IMRP4r, count);
                    REG_ADD(unit, port, sync_mode, IMRP6r, count);
                }
            }
        }
        break;
    case snmpBcmIPMCInDroppedPckts:
        if (soc_feature(unit, soc_feature_ip_mcast)) {
            if (soc_feature(unit, soc_feature_stat_xgs3)) {
                if (incl_non_ge_stat) {
                    REG_ADD(unit, port, sync_mode, RDBGC2r, count);
                }
            }
        }
        break;
    case snmpBcmIPMCOutDroppedPckts:
        if (soc_feature(unit, soc_feature_ip_mcast)) {
            if (soc_feature(unit, soc_feature_stat_xgs3)) {
                if (incl_non_ge_stat) {
                    REG_ADD(unit, port, sync_mode, TDBGC5r, count);
                }
            }
        }
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
