/*
 * $Id: feature.h,v 1.546 Broadcom SDK $
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
 * SOC_FEATURE definitions
 *
 * File:        feature.h
 * Purpose:     Define features by chip
 */

#ifndef _SOC_FEATURE_H
#define _SOC_FEATURE_H

#include <shared/enumgen.h>

/*
 * Defines enums of the form soc_feature_xxx, and a corresponding list
 * of initializer strings "xxx".  New additions should be made in the
 * following section between the zero and count entries.
 */

#define SHR_G_ENTRIES(DONT_CARE)                                          \
SHR_G_ENTRY(zero),              /* ALWAYS FIRST PLEASE (DO NOT CHANGE) */ \
SHR_G_ENTRY(arl_hashed),                                                  \
SHR_G_ENTRY(arl_lookup_cmd),                                              \
SHR_G_ENTRY(arl_lookup_retry),  /* All-0's response requires retry */     \
SHR_G_ENTRY(arl_insert_ovr),    /* ARL ins overwrites if key exists */    \
SHR_G_ENTRY(cfap_pool),         /* Software CFAP initialization. */       \
SHR_G_ENTRY(cos_rx_dma),        /* COS based RX DMA */                    \
SHR_G_ENTRY(dcb_type3),         /* 5690 */                                \
SHR_G_ENTRY(dcb_type4),         /* 5670 */                                \
SHR_G_ENTRY(dcb_type5),         /* 5673 */                                \
SHR_G_ENTRY(dcb_type6),         /* 5665 */                                \
SHR_G_ENTRY(dcb_type7),         /* 5695 when enabled */                   \
SHR_G_ENTRY(dcb_type8),         /* not used (5345) */                     \
SHR_G_ENTRY(dcb_type9),         /* 56504 */                               \
SHR_G_ENTRY(dcb_type10),        /* 56601 */                               \
SHR_G_ENTRY(dcb_type11),        /* 56580, 56700, 56800 */                 \
SHR_G_ENTRY(dcb_type12),        /* 56218 */                               \
SHR_G_ENTRY(dcb_type13),        /* 56514 */                               \
SHR_G_ENTRY(dcb_type14),        /* 56624, 56680 */                        \
SHR_G_ENTRY(dcb_type15),        /* 56224 A0 */                            \
SHR_G_ENTRY(dcb_type16),        /* 56820 */                               \
SHR_G_ENTRY(dcb_type17),        /* 53314 */                               \
SHR_G_ENTRY(dcb_type18),        /* 56224 B0 */                            \
SHR_G_ENTRY(dcb_type19),        /* 56634, 56524, 56685 */                 \
SHR_G_ENTRY(dcb_type20),        /* 5623x */                               \
SHR_G_ENTRY(dcb_type21),        /* 56840 */                               \
SHR_G_ENTRY(dcb_type22),        /* 88732 */                               \
SHR_G_ENTRY(dcb_type23),        /* 56640 */                               \
SHR_G_ENTRY(dcb_type24),        /* 56440 */                               \
SHR_G_ENTRY(dcb_type25),        /* 88030 */                               \
SHR_G_ENTRY(dcb_type26),        /* 56850 */                               \
SHR_G_ENTRY(dcb_type27),        /* 88230 */                               \
SHR_G_ENTRY(dcb_type28),        /* 88650 */                               \
SHR_G_ENTRY(dcb_type29),        /* 56450 */                               \
SHR_G_ENTRY(dcb_type30),        /* 56150 */                               \
SHR_G_ENTRY(dcb_type31),        /* 53400 */                               \
SHR_G_ENTRY(fe_gig_macs),                                                 \
SHR_G_ENTRY(trimac),                                                      \
SHR_G_ENTRY(filter),                                                      \
SHR_G_ENTRY(filter_extended),                                             \
SHR_G_ENTRY(filter_xgs),                                                  \
SHR_G_ENTRY(filter_pktfmtext),  /* Enhanced pkt format spec for XGS */    \
SHR_G_ENTRY(filter_metering),                                             \
SHR_G_ENTRY(ingress_metering),                                            \
SHR_G_ENTRY(egress_metering),                                             \
SHR_G_ENTRY(filter_tucana),                                               \
SHR_G_ENTRY(filter_draco15),                                              \
SHR_G_ENTRY(has_gbp),                                                     \
SHR_G_ENTRY(l3),                /* Capable of Layer 3 switching.  */      \
SHR_G_ENTRY(l3_ip6),            /* IPv6 capable device. */                \
SHR_G_ENTRY(l3_lookup_cmd),                                               \
SHR_G_ENTRY(ip_mcast),                                                    \
SHR_G_ENTRY(ip_mcast_repl),     /* IPMC replication */                    \
SHR_G_ENTRY(led_proc),          /* LED microprocessor */                  \
SHR_G_ENTRY(led_data_offset_a0),/* LED RAM data offset for Link Status */ \
SHR_G_ENTRY(schmsg_alias),                                                \
SHR_G_ENTRY(stack_my_modid),                                              \
SHR_G_ENTRY(stat_dma),                                                    \
SHR_G_ENTRY(cpuport_stat_dma),                                            \
SHR_G_ENTRY(table_dma),                                                   \
SHR_G_ENTRY(tslam_dma),                                                   \
SHR_G_ENTRY(stg),               /* Generic STG */                         \
SHR_G_ENTRY(stg_xgs),           /* XGS implementation specific STG */     \
SHR_G_ENTRY(remap_ut_prio),     /* Remap prio of untagged pkts by port */ \
SHR_G_ENTRY(link_down_fd),      /* Bug in BCM5645 MAC; see mac.c */       \
SHR_G_ENTRY(phy_5690_link_war),                                           \
SHR_G_ENTRY(l3x_delete_bf_bug), /* L3 delete fails if bucket full */      \
SHR_G_ENTRY(xgxs_v1),           /* FusionCore version 1 (BCM5670/90 A0) */\
SHR_G_ENTRY(xgxs_v2),           /* FusionCore version 2 (BCM5670/90 A1) */\
SHR_G_ENTRY(xgxs_v3),           /* FusionCore version 3 (BCM5673) */      \
SHR_G_ENTRY(xgxs_v4),           /* FusionCore version 4 (BCM5674) */      \
SHR_G_ENTRY(xgxs_v5),           /* FusionCore version 5 (BCM56504/56601)*/\
SHR_G_ENTRY(xgxs_v6),           /* FusionCore version 6 (BCM56700/56800)*/\
SHR_G_ENTRY(xgxs_v7),           /* FusionCore version 7 (BCM56624/56680)*/\
SHR_G_ENTRY(lmd),               /* Lane Mux/DMux 2.5 Gbps speed support */\
SHR_G_ENTRY(bigmac_fault_stat), /* BigMac supports link fault status*/    \
SHR_G_ENTRY(phy_cl45),          /* Clause 45 phy support */               \
SHR_G_ENTRY(aging_extended),    /* extended l2 age control */             \
SHR_G_ENTRY(dmux),              /* DMUX capability */                     \
SHR_G_ENTRY(ext_gth_hd_ipg),    /* Longer half dup. IPG (BCM5690A0/A1) */ \
SHR_G_ENTRY(l2x_ins_sets_hit),  /* L2X insert hardcodes HIT bit to 1 */   \
SHR_G_ENTRY(l3x_ins_igns_hit),  /* L3X insert does not affect HIT bit */  \
SHR_G_ENTRY(fabric_debug),      /* Debug support works on 5670 types */   \
SHR_G_ENTRY(filter_krules),     /* All FFP rules usable on 5673 */        \
SHR_G_ENTRY(srcmod_filter),     /* Chip can filter on src modid */        \
SHR_G_ENTRY(dcb_st0_bug),       /* Unreliable DCB status 0 (BCM5665 A0) */\
SHR_G_ENTRY(filter_128_rules),  /* 128 FFP rules usable on 5673 */        \
SHR_G_ENTRY(modmap),            /* Module ID remap */                     \
SHR_G_ENTRY(recheck_cntrs),     /* Counter collection bug */              \
SHR_G_ENTRY(bigmac_rxcnt_bug),  /* BigMAC Rx counter bug */               \
SHR_G_ENTRY(block_ctrl_ingress),/* Must block ctrl frames in xPIC */      \
SHR_G_ENTRY(mstp_mask),         /* VLAN mask for MSTP drops */            \
SHR_G_ENTRY(mstp_lookup),       /* VLAN lookup for MSTP drops */          \
SHR_G_ENTRY(mstp_uipmc),        /* MSTP for UIPMC */                      \
SHR_G_ENTRY(ipmc_lookup),       /* IPMC echo suppression */               \
SHR_G_ENTRY(lynx_l3_expanded),  /* Lynx 1.5 with expanded L3 tables */    \
SHR_G_ENTRY(fast_rate_limit),   /* Short pulse rate limits (Lynx !A0) */  \
SHR_G_ENTRY(l3_sgv),            /* (S,G,V) hash key in L3 table */        \
SHR_G_ENTRY(l3_sgv_aisb_hash),  /* (S,G,V) variant hash */                \
SHR_G_ENTRY(l3_dynamic_ecmp_group),/* Ecmp group use base and count  */   \
SHR_G_ENTRY(l3_host_ecmp_group),   /* Ecmp group for Host Entries */   \
SHR_G_ENTRY(l2_hashed),                                                   \
SHR_G_ENTRY(l2_lookup_cmd),                                               \
SHR_G_ENTRY(l2_lookup_retry),   /* All-0's response requires retry */     \
SHR_G_ENTRY(l2_user_table),     /* Implements L2 User table */            \
SHR_G_ENTRY(rsv_mask),          /* RSV_MASK needs 10/100 mode adjust */   \
SHR_G_ENTRY(schan_hw_timeout),  /* H/W can indicate SCHAN timeout    */   \
SHR_G_ENTRY(lpm_tcam),          /* LPM table is a TCAM               */   \
SHR_G_ENTRY(mdio_enhanced),     /* MDIO address remapping + C45/C22  */   \
SHR_G_ENTRY(mem_cmd),           /* Memory commands supported  */          \
SHR_G_ENTRY(two_ingress_pipes), /* Has two ingress pipelines           */ \
SHR_G_ENTRY(mpls),              /* MPLS for XGS3 */                       \
SHR_G_ENTRY(xgxs_lcpll),        /* LCPLL Clock for XGXS */                \
SHR_G_ENTRY(dodeca_serdes),     /* Dodeca Serdes */                       \
SHR_G_ENTRY(txdma_purge),       /* TX Purge packet after DMA abort */     \
SHR_G_ENTRY(rxdma_cleanup),     /* Check RX Packet SOP after DMA abort */ \
SHR_G_ENTRY(pkt_tx_align),      /* Fix Tx alignment for 128 byte burst */ \
SHR_G_ENTRY(status_link_fail),  /* H/W linkcsan gives Link fail status */ \
SHR_G_ENTRY(fe_maxframe),       /* FE MAC MAXFR is max frame len + 1   */ \
SHR_G_ENTRY(l2x_parity),        /* Parity support on L2X table         */ \
SHR_G_ENTRY(l3x_parity),        /* Parity support on L3 tables         */ \
SHR_G_ENTRY(l3defip_parity),    /* Parity support on L3 DEFIP tables   */ \
SHR_G_ENTRY(l3_defip_map),      /* Map out unused L3_DEFIP blocks      */ \
SHR_G_ENTRY(l2_modfifo),        /* L2 Modfifo for L2 table sync        */ \
SHR_G_ENTRY(l2_multiple),       /* Multiple L2 tables implemented      */ \
SHR_G_ENTRY(l2_overflow),       /* L2 overflow mechanism implemented   */ \
SHR_G_ENTRY(vlan_mc_flood_ctrl),/* VLAN multicast flood control (PFM)  */ \
SHR_G_ENTRY(vfi_mc_flood_ctrl), /* VFI multicast flood control (PFM)   */ \
SHR_G_ENTRY(vlan_translation),  /* VLAN translation feature            */ \
SHR_G_ENTRY(egr_vlan_pfm),      /* MH PFM control per VLAN FB A0       */ \
SHR_G_ENTRY(parity_err_tocpu),  /* Packet to CPU on lookup parity error*/ \
SHR_G_ENTRY(nip_l3_err_tocpu),  /* Packet to CPU on non-IP L3 error    */ \
SHR_G_ENTRY(l3mtu_fail_tocpu),  /* Packet to CPU on L3 MTU check fail  */ \
SHR_G_ENTRY(meter_adjust),      /* Packet length adjustment for meter  */ \
SHR_G_ENTRY(prime_ctr_writes),  /* Prepare counter writes with read    */ \
SHR_G_ENTRY(xgxs_power),        /* Power up XGSS                       */ \
SHR_G_ENTRY(src_modid_blk),     /* SRC_MODID_BLOCK table support       */ \
SHR_G_ENTRY(src_modid_blk_ucast_override),/* MODID_BLOCK ucast overide */ \
SHR_G_ENTRY(src_modid_blk_opcode_override),/* MODID_BLOCK opcode overide */ \
SHR_G_ENTRY(src_modid_blk_profile), /* SRC_MODID_* block profile table */ \
SHR_G_ENTRY(egress_blk_ucast_override),  /* egr mask opcode overide    */ \
SHR_G_ENTRY(stat_jumbo_adj),    /* Adjustable stats *OVR threshold     */ \
SHR_G_ENTRY(stat_xgs3),         /* XGS3 dbg counters and IPv6 supports */ \
SHR_G_ENTRY(dbgc_higig_lkup),   /* FB B0 dbg counters w/ HiGig lookup  */ \
SHR_G_ENTRY(port_trunk_index),  /* FB B0 trunk programmable hash       */ \
SHR_G_ENTRY(port_flow_hash),    /* HB/GW enhanced hashing              */ \
SHR_G_ENTRY(seer_bcam_tune),    /* Retune SEER BCAM table              */ \
SHR_G_ENTRY(mcu_fifo_suppress), /* MCU fifo error suppression          */ \
SHR_G_ENTRY(cpuport_switched),  /* CPU port can act as switch port     */ \
SHR_G_ENTRY(cpuport_mirror),    /* CPU port can be mirrored            */ \
SHR_G_ENTRY(higig2),            /* Higig2 support                      */ \
SHR_G_ENTRY(color),             /* Select color from priority/CFI      */ \
SHR_G_ENTRY(color_inner_cfi),   /* Select color from inner tag CFI     */ \
SHR_G_ENTRY(color_prio_map),    /* Map color/int_prio from/to prio/CFI */ \
SHR_G_ENTRY(untagged_vt_miss),  /* Drop untagged VLAN xlate miss       */ \
SHR_G_ENTRY(module_loopback),   /* Allow local module ingress          */ \
SHR_G_ENTRY(dscp_map_per_port), /* Per Port DSCP mapping table support */ \
SHR_G_ENTRY(egr_dscp_map_per_port), /* Per Egress Port DSCP mapping table support */ \
SHR_G_ENTRY(dscp_map_mode_all), /* DSCP map mode ALL or NONE support   */ \
SHR_G_ENTRY(l3defip_bound_adj), /* L3 defip boundary adjustment        */ \
SHR_G_ENTRY(tunnel_dscp_trust), /* Trust incomming tunnel DSCP         */ \
SHR_G_ENTRY(tunnel_protocol_match),/* Tunnel term key includes protocol. */ \
SHR_G_ENTRY(higig_lookup),      /* FB B0 Proxy (HG lookup)             */ \
SHR_G_ENTRY(egr_l3_mtu),        /* L3 MTU check                        */ \
SHR_G_ENTRY(egr_mirror_path),   /* Alternate path for egress mirror    */ \
SHR_G_ENTRY(xgs1_mirror),       /* XGS1 mirroring compatibility support*/ \
SHR_G_ENTRY(register_hi),       /* _HI designation for > 32 ports      */ \
SHR_G_ENTRY(table_hi),          /* _HI designation for > 32 ports      */ \
SHR_G_ENTRY(trunk_extended),    /* extended designation for > 32 trunks*/ \
SHR_G_ENTRY(trunk_extended_only), /* only support extended trunk mode  */ \
SHR_G_ENTRY(hg_trunking),       /* higig trunking support              */ \
SHR_G_ENTRY(hg_trunk_override), /* higig trunk override support        */ \
SHR_G_ENTRY(trunk_group_size),  /* trunk group size support            */ \
SHR_G_ENTRY(shared_trunk_member_table), /* shared trunk member table   */ \
SHR_G_ENTRY(egr_vlan_check),    /* Support for Egress VLAN check PBM   */ \
SHR_G_ENTRY(mod1),              /* Ports spread over two module Ids    */ \
SHR_G_ENTRY(egr_ts_ctrl),       /* Egress Time Slot Control            */ \
SHR_G_ENTRY(ipmc_grp_parity),   /* Parity support on IPMC GROUP table  */ \
SHR_G_ENTRY(mpls_pop_search),   /* MPLS pop search support             */ \
SHR_G_ENTRY(cpu_proto_prio),    /* CPU protocol priority support       */ \
SHR_G_ENTRY(ignore_pkt_tag),    /* Ignore packet vlan tag support      */ \
SHR_G_ENTRY(port_lag_failover), /* Port LAG failover support           */ \
SHR_G_ENTRY(hg_trunk_failover), /* Higig trunk failover support        */ \
SHR_G_ENTRY(trunk_egress),      /* Trunk egress support                */ \
SHR_G_ENTRY(force_forward),     /* Egress port override support        */ \
SHR_G_ENTRY(port_egr_block_ctl),/* L2/L3 port egress block control     */ \
SHR_G_ENTRY(ipmc_repl_freeze),  /* IPMC repl config pauses traffic     */ \
SHR_G_ENTRY(bucket_support),    /* MAX/MIN BUCKET support              */ \
SHR_G_ENTRY(remote_learn_trust),/* DO_NOT_LEARN bit in HiGig Header    */ \
SHR_G_ENTRY(ipmc_group_mtu),    /* IPMC MTU Setting for all Groups     */ \
SHR_G_ENTRY(tx_fast_path),      /* TX Descriptor mapped send           */ \
SHR_G_ENTRY(deskew_dll),        /* Deskew DLL present                  */ \
SHR_G_ENTRY(src_mac_group),     /* Src MAC Group (MAC Block Index)     */ \
SHR_G_ENTRY(storm_control),     /* Strom Control                       */ \
SHR_G_ENTRY(hw_stats_calc),     /* Calculation of stats in HW          */ \
SHR_G_ENTRY(ext_tcam_sharing),  /* External TCAM sharing support       */ \
SHR_G_ENTRY(mac_learn_limit),   /* MAC Learn limit support             */ \
SHR_G_ENTRY(mac_learn_limit_rollover),   /* MAC Learn limit counter roll over */ \
SHR_G_ENTRY(linear_drr_weight), /* Linear DRR weight calculation       */ \
SHR_G_ENTRY(igmp_mld_support),  /* IGMP MLD snooping support           */ \
SHR_G_ENTRY(basic_dos_ctrl),    /* TCP flags, L4 port, TCP frag, ICMP  */ \
SHR_G_ENTRY(enhanced_dos_ctrl), /* Enhanced DOS control features       */ \
SHR_G_ENTRY(ctr_xaui_activity), /* XAUI counter activity support       */ \
SHR_G_ENTRY(proto_pkt_ctrl),    /* Protocol packet control             */ \
SHR_G_ENTRY(vlan_ctrl),         /* Per VLAN property control           */ \
SHR_G_ENTRY(tunnel_6to4_secure),/* Secure IPv6 to IPv4 tunnel support. */ \
SHR_G_ENTRY(tunnel_any_in_6),   /* Any in IPv6 tunnel support.         */ \
SHR_G_ENTRY(tunnel_gre),        /* GRE  tunneling support.             */ \
SHR_G_ENTRY(unknown_ipmc_tocpu),/* Send unknown IPMC packet to CPU.    */ \
SHR_G_ENTRY(src_trunk_port_bridge), /* Source trunk port bridge        */ \
SHR_G_ENTRY(stat_dma_error_ack),/* Stat DMA error acknowledge bit offset */ \
SHR_G_ENTRY(big_icmpv6_ping_check),/* ICMP V6 oversize packet disacrd  */ \
SHR_G_ENTRY(asf),               /* Alternate Store and Forward         */ \
SHR_G_ENTRY(asf_no_10_100),     /* No ASF support for 10/100Mb speeds  */ \
SHR_G_ENTRY(trunk_group_overlay),/* Direct TGID overlay of modid/port  */ \
SHR_G_ENTRY(xport_convertible), /* XPORT <-> Higig support             */ \
SHR_G_ENTRY(dual_hash),         /* Dual hash tables support            */ \
SHR_G_ENTRY(robo_sw_override),  /* Robo Port Software Override state   */ \
SHR_G_ENTRY(dscp),              /* DiffServ QoS                        */ \
SHR_G_ENTRY(auth),              /* AUTH 802.1x support                 */ \
SHR_G_ENTRY(mstp),                                                        \
SHR_G_ENTRY(arl_mode_control),  /* ARL CONTROL (DS/DD/CPU)             */ \
SHR_G_ENTRY(no_stat_mib),                                                 \
SHR_G_ENTRY(rcpu_1),            /* Remote CPU feature (Raptor)         */ \
SHR_G_ENTRY(unimac),            /* UniMAC 10/100/1000                  */ \
SHR_G_ENTRY(xmac),              /* XMAC                                */ \
SHR_G_ENTRY(xlmac),             /* XLMAC                               */ \
SHR_G_ENTRY(cmac),              /* CMAC                                */ \
SHR_G_ENTRY(hw_dos_prev),       /* hardware dos attack prevention(Robo)*/ \
SHR_G_ENTRY(hw_dos_report),     /* hardware dos event report(Robo)     */ \
SHR_G_ENTRY(generic_table_ops), /* Generic SCHAN table operations      */ \
SHR_G_ENTRY(lpm_prefix_length_max_128),/* Maximum lpm prefix len 128   */ \
SHR_G_ENTRY(tag_enforcement),   /* BRCM tag with tag_enforcement       */ \
SHR_G_ENTRY(vlan_translation_range),   /* VLAN translation range       */ \
SHR_G_ENTRY(vlan_xlate_dtag_range), /*VLAN translation double tag range*/ \
SHR_G_ENTRY(mpls_per_vlan),     /* MPLS enable per-vlan                */ \
SHR_G_ENTRY(mpls_bos_lookup),   /* Second lookup bottom of stack == 0  */ \
SHR_G_ENTRY(class_based_learning),  /* L2 class based learning         */ \
SHR_G_ENTRY(static_pfm),        /* Static MH PFM control               */ \
SHR_G_ENTRY(ipmc_repl_penultimate), /* Flag last IPMC replication      */ \
SHR_G_ENTRY(sgmii_autoneg),     /* Support SGMII autonegotiation       */ \
SHR_G_ENTRY(gmii_clkout),       /* Support GMII clock output           */ \
SHR_G_ENTRY(ip_ep_mem_parity),  /* Parity support on IP, EP memories   */ \
SHR_G_ENTRY(post),              /* Post after reset init               */ \
SHR_G_ENTRY(rcpu_priority),     /* Remote CPU packet QoS handling      */ \
SHR_G_ENTRY(rcpu_tc_mapping),   /* Remote CPU traffic class to queue   */ \
SHR_G_ENTRY(mem_push_pop),      /* SCHAN push/pop operations           */ \
SHR_G_ENTRY(dcb_reason_hi),     /* Number of RX reasons exceeds 32     */ \
SHR_G_ENTRY(multi_sbus_cmds),   /* SCHAN multiple sbus commands        */ \
SHR_G_ENTRY(flexible_dma_steps),/* Flexible sbus address increment step */ \
SHR_G_ENTRY(new_sbus_format),   /* New sbus command format             */ \
SHR_G_ENTRY(new_sbus_old_resp), /* New sbus command format, Old Response */ \
SHR_G_ENTRY(sbus_format_v4),    /* Sbus command format version 4       */ \
SHR_G_ENTRY(esm_support),       /* External TCAM support               */ \
SHR_G_ENTRY(esm_rxfifo_resync), /* ESM with DDR RX_FIFO resync logic   */ \
SHR_G_ENTRY(fifo_dma),          /* fifo DMA support                    */ \
SHR_G_ENTRY(fifo_dma_active),   /* fifo DMA active bit set if its not empty */ \
SHR_G_ENTRY(ipfix),             /* IPFIX support                       */ \
SHR_G_ENTRY(ipfix_rate),        /* IPFIX rate and rate mirror support  */ \
SHR_G_ENTRY(ipfix_flow_mirror), /* IPFIX flow mirror support           */ \
SHR_G_ENTRY(l3_entry_key_type), /* Data stored in first entry only.    */ \
SHR_G_ENTRY(robo_ge_serdes_mac_autosync), /* serdes_autosync with mac  */ \
SHR_G_ENTRY(fp_routing_mirage), /* Mirage fp based routing.            */ \
SHR_G_ENTRY(fp_routing_hk),     /* Hawkeye fp based routing.           */ \
SHR_G_ENTRY(subport),           /* Subport support                     */ \
SHR_G_ENTRY(reset_delay),       /* Delay after CMIC soft reset         */ \
SHR_G_ENTRY(fast_egr_cell_count), /* HW accelerated cell count retrieval */ \
SHR_G_ENTRY(802_3as),           /* 802.3as                             */ \
SHR_G_ENTRY(l2_pending),        /* L2 table pending support            */ \
SHR_G_ENTRY(discard_ability),   /* discard/WRED support                */ \
SHR_G_ENTRY(discard_ability_color_black), /* discard/WRED color black  */ \
SHR_G_ENTRY(distribution_ability), /* distribution/ESET support        */ \
SHR_G_ENTRY(mc_group_ability),  /* McGroup support                     */ \
SHR_G_ENTRY(cosq_gport_stat_ability), /* COS queue stats support       */ \
SHR_G_ENTRY(standalone),        /* standalone switch mode              */ \
SHR_G_ENTRY(internal_loopback), /* internal loopback port              */ \
SHR_G_ENTRY(packet_adj_len),    /* packet adjust length                */ \
SHR_G_ENTRY(vlan_action),       /* VLAN action support                 */ \
SHR_G_ENTRY(vlan_pri_cfi_action), /* VLAN pri/CFI action support       */ \
SHR_G_ENTRY(vlan_copy_action),  /* VLAN copy action support            */ \
SHR_G_ENTRY(packet_rate_limit), /* Packet-based rate limitting to CPU  */ \
SHR_G_ENTRY(system_mac_learn_limit), /* System MAC Learn limit support */ \
SHR_G_ENTRY(fp_based_routing),  /* L3 based on field processor.        */ \
SHR_G_ENTRY(field),             /* Field Processor (FP) for XGS3       */ \
SHR_G_ENTRY(field_slices2),     /* FP TCAM has 2 slices, instead of 16 */ \
SHR_G_ENTRY(field_slices4),     /* FP TCAM has 4 slices, instead of 16 */ \
SHR_G_ENTRY(field_slices8),     /* FP TCAM has 8 slices, instead of 16 */ \
SHR_G_ENTRY(field_slices10),    /* FP TCAM has 10 slices,instead of 16 */ \
SHR_G_ENTRY(field_slices12),    /* FP TCAM has 12 slices,instead of 16 */ \
SHR_G_ENTRY(field_meter_pools4),/* FP TCAM has 4 global meter pools.   */ \
SHR_G_ENTRY(field_meter_pools8),/* FP TCAM has 8 global meter pools.   */ \
SHR_G_ENTRY(field_meter_pools12),/* FP TCAM has 12 global meter pools. */ \
SHR_G_ENTRY(field_mirror_ovr),  /* FP has MirrorOverride action        */ \
SHR_G_ENTRY(field_udf_higig),   /* FP UDF window contains HiGig header */ \
SHR_G_ENTRY(field_udf_higig2),  /* FP UDF window contains HiGig2 header */ \
SHR_G_ENTRY(field_udf_hg),      /* FP UDF2.0->2.2 HiGig header         */ \
SHR_G_ENTRY(field_udf_ethertype), /* FP UDF can ues extra ethertypes   */ \
SHR_G_ENTRY(field_comb_read),   /* FP can read TCAM & Policy as one    */ \
SHR_G_ENTRY(field_wide),        /* FP has wide-mode combining of slices*/ \
SHR_G_ENTRY(field_slice_enable),/* FP enable/disable on per slice basis*/ \
SHR_G_ENTRY(field_cos),         /* FP has CoS Queue actions            */ \
SHR_G_ENTRY(field_ingress_late),/* FP has late ingress pipeline stage  */ \
SHR_G_ENTRY(field_color_indep), /* FP can set color in/dependence      */ \
SHR_G_ENTRY(field_qual_drop),   /* FP can qualify on drop state        */ \
SHR_G_ENTRY(field_qual_IpType), /* FP can qualify on IpType alone      */ \
SHR_G_ENTRY(field_qual_Ip6High), /* FP can qualify on Src/Dst Ip 6 High */ \
SHR_G_ENTRY(field_mirror_pkts_ctl), /* Enable FP for mirror packtes    */ \
SHR_G_ENTRY(field_intraslice_basic_key), /* Only IP4 or IP6 key.       */ \
SHR_G_ENTRY(field_ingress_two_slice_types),/* Ingress FP has 2 slice types*/\
SHR_G_ENTRY(field_ingress_global_meter_pools), /* Global Meter Pools   */ \
SHR_G_ENTRY(field_ingress_ipbm), /* FP IPBM qualifier is always present*/ \
SHR_G_ENTRY(field_egress_flexible_v6_key), /* Egress FP KEY2 SIP6/DIP6.*/ \
SHR_G_ENTRY(field_egress_global_counters), /* Global Counters          */ \
SHR_G_ENTRY(field_egress_metering), /* FP allows metering in egress mode */ \
SHR_G_ENTRY(field_ing_egr_separate_packet_byte_counters), /*           */ \
SHR_G_ENTRY(field_multi_stage), /* Multi staged field processor.       */ \
SHR_G_ENTRY(field_intraslice_double_wide), /* FP double wide in 1 slice*/ \
SHR_G_ENTRY(field_int_fsel_adj),/* FP internal field select adjust     */ \
SHR_G_ENTRY(field_pkt_res_adj), /* FP field packet resolution adjust   */ \
SHR_G_ENTRY(field_virtual_slice_group), /* Virtual slice/groups in FP  */ \
SHR_G_ENTRY(field_qualify_gport), /* EgrObj/Mcast Grp/Gport qualifiers */ \
SHR_G_ENTRY(field_action_redirect_ipmc), /* Redirect to ipmc index.    */ \
SHR_G_ENTRY(field_action_timestamp), /* Copy to cpu with timestamp.    */ \
SHR_G_ENTRY(field_action_l2_change), /* Modify l2 packets sa/da/vlan.  */ \
SHR_G_ENTRY(field_action_fabric_queue), /* Add HiGig2 extension header. */ \
SHR_G_ENTRY(field_virtual_queue),    /* Virtual queue support.         */ \
SHR_G_ENTRY(field_vfp_flex_counter), /* Flex cntrs support in vfp.     */ \
SHR_G_ENTRY(field_tcam_hw_move), /* TCAM move support via HW.          */ \
SHR_G_ENTRY(field_tcam_parity_check), /* TCAM parity check.            */ \
SHR_G_ENTRY(field_qual_my_station_hit), /* FP can qualify on My Station Hit*/ \
SHR_G_ENTRY(field_action_redirect_nexthop), /* Redirect packet to Next Hop index */ \
SHR_G_ENTRY(field_action_redirect_ecmp), /* Redirect packet to ECMP group */ \
SHR_G_ENTRY(field_slice_dest_entity_select), /* FP can qualify on destination type */ \
SHR_G_ENTRY(field_packet_based_metering), /* FP supports packet based metering */ \
SHR_G_ENTRY(field_stage_half_slice), /* Only half the number of total entries in FP_TCAM and FP_POLICY tables are used */ \
SHR_G_ENTRY(field_stage_quarter_slice), /* Only quarter the number of total entries in FP_TCAM and FP_POLICY tables are used */ \
SHR_G_ENTRY(field_slice_size128),/* Only 128 entries in FP_TCAM and FP_POLICY tables are used */ \
SHR_G_ENTRY(field_stage_ingress_256_half_slice),/* Only half entries/slice are used when FP_TCAM and FP_POLICY have 256 entries/slice */\
SHR_G_ENTRY(field_stage_ingress_512_half_slice),/* Only half entries/slice are used when FP_TCAM and FP_POLICY have 512 entries/slice */\
SHR_G_ENTRY(field_stage_egress_256_half_slice),/*  Only half entries/slice are used when EFP_TCAM has 256 entries/slice */\
SHR_G_ENTRY(field_stage_lookup_256_half_slice),/*  Only half entries/slice are used when VFP_TCAM has 256 entries/slice */\
SHR_G_ENTRY(field_stage_lookup_512_half_slice),/*  Only half entries/slice are used when VFP_TCAM has 512 entries/slice */\
SHR_G_ENTRY(field_oam_actions), /* FP supports OAM actions. */ \
SHR_G_ENTRY(field_ingress_cosq_override), /* Ingress FP overrides assigned CoSQ */ \
SHR_G_ENTRY(field_egress_tocpu), /* Egress copy-to-cpu */\
SHR_G_ENTRY(virtual_switching), /* Virtual lan services support        */ \
SHR_G_ENTRY(lport_tab_profile), /* Lport table is profiled per svp     */ \
SHR_G_ENTRY(xgport_one_xe_six_ge), /* XGPORT mode with 1x10G + 6x1GE   */ \
SHR_G_ENTRY(sample_offset8),  /* Sample threshold is shifted << 8 bits */ \
SHR_G_ENTRY(sample_thresh16), /* Sample threshold 16 bit wide          */ \
SHR_G_ENTRY(sample_thresh24), /* Sample threshold 24 bit wide          */ \
SHR_G_ENTRY(mim),             /* MIM for XGS3                          */ \
SHR_G_ENTRY(oam),             /* OAM for XGS3                          */ \
SHR_G_ENTRY(bfd),             /* BFD for XGS3                          */ \
SHR_G_ENTRY(bhh),             /* BHH for XGS3                          */ \
SHR_G_ENTRY(ptp),             /* PTP for XGS3                          */ \
SHR_G_ENTRY(hybrid),          /* global hybrid configuration feature   */ \
SHR_G_ENTRY(node_hybrid),     /* node hybrid configuration feature     */ \
SHR_G_ENTRY(arbiter),         /* arbiter feature                       */ \
SHR_G_ENTRY(arbiter_capable), /* arbiter capable feature               */ \
SHR_G_ENTRY(node),            /* node hybrid configuration mode        */ \
SHR_G_ENTRY(lcm),             /* lcm  feature                          */ \
SHR_G_ENTRY(xbar),            /* xbar feature                          */ \
SHR_G_ENTRY(egr_independent_fc), /* egr fifo independent flow control  */ \
SHR_G_ENTRY(egr_multicast_independent_fc), /* egr multicast fifo independent flow control  */ \
SHR_G_ENTRY(always_drive_dbus), /* drive esm dbus                      */ \
SHR_G_ENTRY(ignore_cmic_xgxs_pll_status), /* ignore CMIC XGXS PLL lock status */ \
SHR_G_ENTRY(mmu_virtual_ports), /* internal MMU virtual ports          */ \
SHR_G_ENTRY(phy_lb_needed_in_mac_lb), /* PHY loopback has to be set when in MAC loopback mode */ \
SHR_G_ENTRY(gport_service_counters), /* Counters for ports and services */ \
SHR_G_ENTRY(use_double_freq_for_ddr_pll), /* use double freq for DDR PLL */\
SHR_G_ENTRY(eav_support),     /* EAV supporting                        */ \
SHR_G_ENTRY(wlan),            /* WLAN for XGS3                         */ \
SHR_G_ENTRY(counter_parity),  /* Counter registers have parity fields  */ \
SHR_G_ENTRY(time_support),    /* Time module support                   */ \
SHR_G_ENTRY(time_v3),         /* 3rd gen time arch: 48b TS, dual BS/TS */ \
SHR_G_ENTRY(time_v3_no_bs),   /* No BroadSync for certian SKUs         */ \
SHR_G_ENTRY(timesync_support), /* Time module support                  */ \
SHR_G_ENTRY(timesync_v3),     /* 3rd gen timesync/1588 arch: 48bit TS  */ \
SHR_G_ENTRY(ip_source_bind),  /* IP to MAC address binding feature     */ \
SHR_G_ENTRY(auto_multicast),  /* Automatic Multicast Tunneling Support */ \
SHR_G_ENTRY(embedded_higig),  /* Embedded Higig Tunneling Support      */ \
SHR_G_ENTRY(hawkeye_a0_war),  /* QSGMII Workaroud                      */ \
SHR_G_ENTRY(vlan_queue_map),  /* VLAN queue mapping  support           */ \
SHR_G_ENTRY(mpls_software_failover), /* MPLS FRR support within Software */   \
SHR_G_ENTRY(mpls_failover),   /* MPLS FRR support                      */ \
SHR_G_ENTRY(extended_pci_error), /* More DMA info for PCI errors       */ \
SHR_G_ENTRY(subport_enhanced), /* Enhanced Subport Support             */ \
SHR_G_ENTRY(priority_flow_control), /* Per-priority flow control       */ \
SHR_G_ENTRY(source_port_priority_flow_control), /* Per-(source port, priority) flow control */ \
SHR_G_ENTRY(flex_port),       /* Flex-port aka hot-swap Support        */ \
SHR_G_ENTRY(qos_profile),     /* QoS profiles for ingress and egress   */ \
SHR_G_ENTRY(fe_ports),        /* Raven/Raptor with FE ports            */ \
SHR_G_ENTRY(mirror_flexible), /* Flexible ingress/egress mirroring     */ \
SHR_G_ENTRY(egr_mirror_true), /* True egress mirroring                 */ \
SHR_G_ENTRY(failover),        /* Generalized VP failover               */ \
SHR_G_ENTRY(delay_core_pll_lock), /* Tune core clock before PLL lock   */ \
SHR_G_ENTRY(extended_cmic_error), /* Block specific errors in CMIC_IRQ_STAT_1/2 */ \
SHR_G_ENTRY(short_cmic_error), /* Override Block specific errors in CMIC_IRQ_STAT_1/2 */ \
SHR_G_ENTRY(urpf),            /* Unicast reverse path check.           */ \
SHR_G_ENTRY(no_bridging),     /* No L2 and L3.                         */ \
SHR_G_ENTRY(no_higig),        /* No higig support                      */ \
SHR_G_ENTRY(no_epc_link),     /* No EPC_LINK_BMAP                      */ \
SHR_G_ENTRY(no_mirror),       /* No mirror support                     */ \
SHR_G_ENTRY(no_learning),     /* No CML support                        */ \
SHR_G_ENTRY(rx_timestamp),      /* RX packet timestamp filed should be updated  */ \
SHR_G_ENTRY(rx_timestamp_upper),/* RX packet timestamp uper 32bit filed should be updated  */ \
SHR_G_ENTRY(sysport_remap),   /* Support for remapping system port to physical port */ \
SHR_G_ENTRY(flexible_xgport), /* Allow 10G/1G speed change on XGPORT   */ \
SHR_G_ENTRY(logical_port_num),/* Physical/logical port number mapping  */ \
SHR_G_ENTRY(mmu_config_property),/* MMU config property                */ \
SHR_G_ENTRY(l2_bulk_control), /* L2 bulk control                       */ \
SHR_G_ENTRY(l2_bulk_bypass_replace), /* L2 bulk control bypass replace */ \
SHR_G_ENTRY(timestamp_counter), /* Timestamp FIFO in counter range     */ \
SHR_G_ENTRY(l3_ingress_interface), /* Layer-3 Ingress Interface Object */ \
SHR_G_ENTRY(ingress_size_templates), /* Ingress Size Tempaltes         */ \
SHR_G_ENTRY(l3_defip_ecmp_count), /* L3 DEFIP ECMP Count               */ \
SHR_G_ENTRY(ppa_bypass),      /* PPA ignoring key type field in L2X    */ \
SHR_G_ENTRY(ppa_match_vp),    /* PPA allows to match on virtual port   */ \
SHR_G_ENTRY(generic_counters), /* Common counters shared by both XE/GE */ \
SHR_G_ENTRY(mpls_enhanced),   /* MPLS Enhancements                     */ \
SHR_G_ENTRY(trill),           /* Trill support                         */ \
SHR_G_ENTRY(niv),             /* Network interface virtualization      */ \
SHR_G_ENTRY(unimac_tx_crs),   /* Unimac TX CRS */ \
SHR_G_ENTRY(hg_trunk_override_profile), /* Higig trunk override is profiled */ \
SHR_G_ENTRY(modport_map_profile), /* Modport map table is profiled     */ \
SHR_G_ENTRY(hg_dlb),          /* Higig DLB                             */ \
SHR_G_ENTRY(l3_defip_hole),   /* L3 DEFIP Hole                         */ \
SHR_G_ENTRY(eee),             /* Energy Efficient Ethernet             */ \
SHR_G_ENTRY(mdio_setup),      /* MDIO setup for FE devices             */ \
SHR_G_ENTRY(ser_parity),      /* SER parity protection available       */ \
SHR_G_ENTRY(ser_engine),      /* Standalone SER engine available       */ \
SHR_G_ENTRY(ser_fifo),        /* Hardware reports events via SER fifos */ \
SHR_G_ENTRY(ser_hw_bg_read),  /* Standalone SER engine has background read available */ \
SHR_G_ENTRY(mem_cache),       /* Memory cache for SER correction       */ \
SHR_G_ENTRY(regs_as_mem),     /* Registers implemented as memory       */ \
SHR_G_ENTRY(fp_meter_ser_verify), /* Reverify FP METER TABLE SER       */ \
SHR_G_ENTRY(int_cpu_arbiter), /* Internal CPU arbiter                  */ \
SHR_G_ENTRY(ets),             /* Enhanced transmission selection       */ \
SHR_G_ENTRY(qcn),             /* Quantized congestion notification     */ \
SHR_G_ENTRY(xy_tcam),         /* Non data/mask type TCAM               */ \
SHR_G_ENTRY(xy_tcam_direct),  /* Non data/mask type TCAM with h/w translation bypass */ \
SHR_G_ENTRY(xy_tcam_28nm),    /* Non data/mask type TCAM               */ \
SHR_G_ENTRY(bucket_update_freeze), /* Disable refresh when updating bucket */ \
SHR_G_ENTRY(hg_trunk_16_members), /* Higig trunk has fixed size of 16  */ \
SHR_G_ENTRY(vlan_vp),         /* VLAN virtual port switching           */ \
SHR_G_ENTRY(vp_group_ingress_vlan_membership), /* Ingress VLAN virtual port group membership */ \
SHR_G_ENTRY(vp_group_egress_vlan_membership), /* Egress VLAN virtual port group membership */ \
SHR_G_ENTRY(ing_vp_vlan_membership), /* Ingress VLAN virtual port membership */ \
SHR_G_ENTRY(egr_vp_vlan_membership), /* Egress VLAN virtual port membership */ \
/* Supports olicer in flow mode with committed rate */ \
SHR_G_ENTRY(policer_mode_flow_rate_committed), \
SHR_G_ENTRY(vlan_egr_it_inner_replace), \
SHR_G_ENTRY(src_modid_base_index), /* Per source module ID base index  */ \
SHR_G_ENTRY(wesp),            /* Wrapped Encapsulating Security Payload */ \
SHR_G_ENTRY(cmicm),           /* CMICm                                 */ \
SHR_G_ENTRY(cmicm_b0),        /* CMICm B0                              */ \
SHR_G_ENTRY(cmicd),           /* CMICd                                 */ \
SHR_G_ENTRY(mcs),             /* Micro Controller Subsystem            */ \
SHR_G_ENTRY(iproc),           /* iProc Internal Processor Subsystem    */ \
SHR_G_ENTRY(iproc_7),         /* iProc 7 profile */ \
SHR_G_ENTRY(cmicm_extended_interrupts), /* CMICm IRQ1, IRQ2            */ \
SHR_G_ENTRY(cmicm_multi_dma_cmc), /* CMICm dma on multiple cmc         */ \
SHR_G_ENTRY(ism_memory),      /* Support for ISM memory                */ \
SHR_G_ENTRY(shared_hash_mem), /* Support for shared hash memory        */ \
SHR_G_ENTRY(shared_hash_ins), /* Support for sw based shared hash ins  */ \
SHR_G_ENTRY(unified_port),    /* Support new unified port design       */ \
SHR_G_ENTRY(sbusdma),         /* CMICM SBUSDMA support                 */ \
SHR_G_ENTRY(regex),           /* Regex signature match                 */ \
SHR_G_ENTRY(l3_ecmp_1k_groups),  /* L3 ECMP 1K Groups                  */ \
SHR_G_ENTRY(advanced_flex_counter), /* Advance flexible counter feature */ \
SHR_G_ENTRY(ces),             /* Circuit Emulation Services            */ \
SHR_G_ENTRY(ddr3),            /* External DDR3 Buffer                  */ \
SHR_G_ENTRY(iproc_ddr),       /* iProc DDR initialization              */ \
SHR_G_ENTRY(mpls_entropy),    /* MPLS Entropy-label feature            */ \
SHR_G_ENTRY(global_meter),    /* Service meter                         */ \
SHR_G_ENTRY(modport_map_dest_is_port_or_trunk), /* Modport map destination is specified as a Higig port or a Higig trunk, instead of as a Higig port bitmap */ \
SHR_G_ENTRY(mirror_encap_profile), /* Egress mirror encap data profile */ \
SHR_G_ENTRY(directed_mirror_only), /* Only directed mirroring mode     */ \
SHR_G_ENTRY(axp),             /* Auxiliary ports                       */ \
SHR_G_ENTRY(etu_support),     /* External TCAM support                 */ \
SHR_G_ENTRY(controlled_counters),            /* L3 ECMP 1K Groups */ \
SHR_G_ENTRY(higig_misc_speed_support), /* Misc speed support - 21G, 42G */ \
SHR_G_ENTRY(e2ecc),           /* End-to-end congestion control         */ \
SHR_G_ENTRY(vpd_profile),     /* VLAN protocol data profile            */ \
SHR_G_ENTRY(color_prio_map_profile), /* Map color priority via profile */ \
SHR_G_ENTRY(hg_dlb_member_id), /* Higig DLB member ports are converted to member IDs */ \
SHR_G_ENTRY(lag_dlb),         /* LAG dynamic load balancing            */ \
SHR_G_ENTRY(ecmp_dlb),        /* ECMP dynamic load balancing           */ \
SHR_G_ENTRY(l2gre),           /* L2-VPN over GRE Tunnels               */ \
SHR_G_ENTRY(l2gre_default_tunnel),           /* Default tunnel based forwarding during L2-GRE VPN lookup failure    */ \
SHR_G_ENTRY(static_repl_head_alloc), /* Allocation of REPL_HEAD table entries is static */ \
SHR_G_ENTRY(vlan_double_tag_range_compress), /* VLAN range compression for double tagged packets */\
SHR_G_ENTRY(vlan_protocol_pkt_ctrl), /* per VLAN protocol packet control */ \
SHR_G_ENTRY(l3_extended_host_entry), /* Extended L3 host entry with embedded NHs  */ \
SHR_G_ENTRY(repl_head_ptr_replace), /* MMU supports replacement of REPL_HEAD pointers in multicast queues */ \
SHR_G_ENTRY(remote_encap),    /* Higig2 remote replication encap       */ \
SHR_G_ENTRY(rx_reason_overlay), /* RX reasons are in overlays          */ \
SHR_G_ENTRY(extended_queueing),  /* Extened queueing suport            */ \
SHR_G_ENTRY(dynamic_sched_update), /*  Enable dynamic update of scheduler mode */ \
SHR_G_ENTRY(schan_err_check), /*  Enable dynamic update of scheduler mode */\
SHR_G_ENTRY(l3_reduced_defip_table), /* Reduced L3 route table         */ \
SHR_G_ENTRY(l3_expanded_defip_table), /* Expanded L3 route table - e.g. 32 physical tcams */ \
SHR_G_ENTRY(rtag1_6_max_portcnt_less_than_rtag7), /* Max port count for RTAG 1-6 is less than RTAG 7 */ \
SHR_G_ENTRY(vlan_xlate),      /* Vlan translation on MPLS packets      */ \
SHR_G_ENTRY(split_repl_group_table), /* MMU replication group table is split into 2 halves */ \
SHR_G_ENTRY(pim_bidir),       /* Bidirectional PIM                     */ \
SHR_G_ENTRY(l3_iif_zero_invalid), /* L3 ingress interface ID 0 is invalid */ \
SHR_G_ENTRY(vector_based_spri), /* MMU vector based strict priority scheduling */ \
SHR_G_ENTRY(vxlan),           /* L2-VPN over UDP Tunnels               */ \
SHR_G_ENTRY(ep_redirect),     /* Egress pipeline redirection           */ \
SHR_G_ENTRY(repl_l3_intf_use_next_hop), /* For each L3 interface, MMU replication outputs a next hop index */ \
SHR_G_ENTRY(dynamic_shaper_update), /*  Enable dynamic update of shaper rates */ \
SHR_G_ENTRY(nat),             /* NAT                                   */ \
SHR_G_ENTRY(l3_iif_profile),  /* Profile for L3_IIF                    */ \
SHR_G_ENTRY(l3_ip4_options_profile), /* Supports special handling for IP4 options */\
SHR_G_ENTRY(linkphy_coe),     /* Supports LinkPHY subports (IEEE G.999.1)  */ \
SHR_G_ENTRY(subtag_coe),      /* Supports SubTag (Third VLAN tag) subports */ \
SHR_G_ENTRY(tr3_sp_vector_mask), /* SP <-> WRR configuration sequence  */ \
SHR_G_ENTRY(cmic_reserved_queues), /* CMIC has reserved queues     */ \
SHR_G_ENTRY(pgw_mac_rsv_mask_remap), /* PGW_MAC_RSV_MASK address remap */ \
SHR_G_ENTRY(endpoint_queuing), /* Endpoint queuing                     */ \
SHR_G_ENTRY(service_queuing), /* Service queuing                       */ \
SHR_G_ENTRY(mirror_control_mem), /* Mirror control is not register     */ \
SHR_G_ENTRY(mirror_table_trunk), /* Mirror MTPs duplicate trunk info   */ \
SHR_G_ENTRY(port_extension),  /* Port Extension (IEEE 802.1Qbh)        */ \
SHR_G_ENTRY(linkscan_pause_timeout), /* wating for Linkscan stopped signal with timeout*/\
SHR_G_ENTRY(linkscan_lock_per_unit), /* linkscsan lock per unit instead of spl*/\
SHR_G_ENTRY(easy_reload_wb_compat), /* Support Easy Reload and Warm boot within the same compilation*/\
SHR_G_ENTRY(mac_virtual_port), /* MAC-based virtual port               */ \
SHR_G_ENTRY(virtual_port_routing), /* VP based routing for VP LAG      */ \
SHR_G_ENTRY(counter_toggled_read), /* Toggled read of counter tables   */ \
SHR_G_ENTRY(vp_lag),          /* Virtual port LAG                      */ \
SHR_G_ENTRY(min_resume_limit_1), /* min resume limit for port-sp       */ \
SHR_G_ENTRY(hg_dlb_id_equal_hg_tid), /* Higig DLB ID is the same as Higig trunk ID */ \
SHR_G_ENTRY(hg_resilient_hash), /* Higig resilient hashing */ \
SHR_G_ENTRY(lag_resilient_hash), /* LAG resilient hashing */ \
SHR_G_ENTRY(ecmp_resilient_hash), /* ECMP resilient hashing */ \
SHR_G_ENTRY(min_cell_per_queue), /* reserve a min number of cells for queue */ \
SHR_G_ENTRY(gphy),            /* Built-in GPhy Support                 */ \
SHR_G_ENTRY(cpu_bp_toggle),   /* CMICm backpressure toggle             */ \
SHR_G_ENTRY(ipmc_reduced_table_size), /* IPMC with reduced table size */ \
SHR_G_ENTRY(mmu_reduced_internal_buffer), /* Recuded MMU internal packet buffer */\
SHR_G_ENTRY(ipmc_unicast), /* Support for IPMC with unicast MAC-DA */\
SHR_G_ENTRY(l3_256_defip_table), /* Route table sizing for certain device SKUs */\
SHR_G_ENTRY(mmu_packing), /* MMU buffer packing */\
SHR_G_ENTRY(l3_shared_defip_table), /* lpm table sharing between 128b and V4, 64b entries */\
SHR_G_ENTRY(fcoe),            /* fiber channel over ethernet          */ \
SHR_G_ENTRY(system_reserved_vlan), /* Supports System Reserved VLAN */ \
SHR_G_ENTRY(ipmc_remap),      /* Supports IPMC remapping */ \
SHR_G_ENTRY(proxy_port_property), /* Allow configuration of per-source port LPORT properties. */ \
SHR_G_ENTRY(multiple_split_horizon_group), /* multiple split horizon group support */ \
SHR_G_ENTRY(uc),              /* Embedded core(s) for applications */ \
SHR_G_ENTRY(overlaid_address_class), /* Overlaid address class support (With over lay PRI bits)*/ \
SHR_G_ENTRY(special_egr_ip_tunnel_ser), /* EGR_IP_TUNNEL overlay memory ser support for older devices */ \
SHR_G_ENTRY(mirror_cos),      /* Mirror UC/MC COS controls */ \
SHR_G_ENTRY(lltag),           /* The out most tag as the LLTAG */\
SHR_G_ENTRY(mem_parity_eccmask), /* memory fields of Parity and ECC masking  */ \
SHR_G_ENTRY(wred_drop_counter_per_port), /* Per port WRED dropped counter  */ \
SHR_G_ENTRY(l2_no_vfi), /* No VFI support */ \
SHR_G_ENTRY(l3mc_use_egress_next_hop), /* L3MC egress logic will use egress next hop for replication */ \
SHR_G_ENTRY(field_action_pfc_class), /* Replace the internal priority of PFC class */ \
SHR_G_ENTRY(fifo_dma_hu2), /* use the hu2 driver fucntion to handle the fifo dma */ \
SHR_G_ENTRY(eee_bb_mode), /* EEE burst and batch mode */ \
SHR_G_ENTRY(cmicd_v2),           /* CMICd v2 */ \
SHR_G_ENTRY(int_common_init),    /* internal function _bcm_common_init */ \
SHR_G_ENTRY(inner_tpid_enable), /* inner tpid enable to detect itag */\
SHR_G_ENTRY(l3_no_ecmp), /* no ECMP */ \
SHR_G_ENTRY(no_tunnel), /* no tunnel */ \
SHR_G_ENTRY(ecn_wred),    /* ECN-WRED architecture */ \
SHR_G_ENTRY(ipmc_use_configured_dest_mac),	/* IPMC configure multicast mac address  */ \
SHR_G_ENTRY(core_clock_300m),    /* system core clock is 300 Mhz */ \
SHR_G_ENTRY(l3_lpm_scaling_enable), /* Allows 64b entries to be added in the paired TCAM*/\
SHR_G_ENTRY(l3_lpm_128b_entries_reserved), /* Paired TCAM space is not reserved */\
SHR_G_ENTRY(hg_no_speed_change),    /* HiGig encap setup without speed modification */ \
SHR_G_ENTRY(mac_based_vlan),       /* MAC based VLAN support                 */ \
SHR_G_ENTRY(esm_correction),     /* Recovery for esm when esm fatal error detected */ \
SHR_G_ENTRY(l3_defip_advanced_lookup),  /* both DIP and SIP lookup using single DEFIP entry */ \
SHR_G_ENTRY(ip_subnet_based_vlan), /* IP subnet based VLAN support   */ \
SHR_G_ENTRY(portmod), /* MAC and PHY managed trough Portmod  */ \
SHR_G_ENTRY(fabric_cell_pcp), /* packet cell packing  */ \
SHR_G_ENTRY(fe_mc_id_range),  /* FE multicast id and mode */ \
SHR_G_ENTRY(fe_mc_priority_mode_enable), /* FE multicast cell priority mode */ \
SHR_G_ENTRY(count)            /* ALWAYS LAST PLEASE (DO NOT CHANGE)  */

/* Make the enums */
#undef  SHR_G_MAKE_STR
#undef  SHR_G_MAKE_STR1
#undef  SHR_G_MAKE_STR_CONCAT
#define SHR_G_MAKE_STR1(a) a
#define SHR_G_MAKE_STR_CONCAT(e,a) SHR_G_MAKE_STR1(e##a)
#define SHR_G_MAKE_STR(a)  SHR_G_MAKE_STR_CONCAT(soc_feature_,a)
SHR_G_MAKE_ENUM(soc_feature_);

/* Make the string array */
#undef  SHR_G_MAKE_STR
#define SHR_G_MAKE_STR(a)     #a
#define SOC_FEATURE_NAME_INITIALIZER                                        \
        SHR_G_NAME_BEGIN(dont_care)                                         \
        SHR_G_ENTRIES(dont_care)                                          \
        SHR_G_NAME_END(dont_care)

/* Feature-defining functions */
extern int soc_features_bcm5670_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5673_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5690_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5665_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5695_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5675_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5674_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5665_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56601_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56601_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56601_c0(int unit, soc_feature_t feature);
extern int soc_features_bcm56602_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56602_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56602_c0(int unit, soc_feature_t feature);
extern int soc_features_bcm56504_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56504_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56102_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56304_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56112_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56314_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5650_c0(int unit, soc_feature_t feature);
extern int soc_features_bcm56800_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56218_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56514_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56624_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56624_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56680_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56680_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56634_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56634_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56524_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56524_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56685_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56685_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56334_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56334_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56840_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56840_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56640_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56640_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56340_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56540_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56850_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5338_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5338_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm5324_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5324_a1(int unit, soc_feature_t feature);
extern int soc_features_bcm5380_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5388_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5396_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5389_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5398_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5325_a1(int unit, soc_feature_t feature);
extern int soc_features_bcm5348_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5397_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5347_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm5395_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm53242_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56224_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56224_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm53262_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56820_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56725_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm53314_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm53324_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm53115_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88020_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88025_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88030_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88030_a1(int unit, soc_feature_t feature);
extern int soc_features_bcm88030_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm53118_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88230_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88230_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm88230_c0(int unit, soc_feature_t feature);
extern int soc_features_bcm53280_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm53280_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56142_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56150_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm53101_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm53125_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm53128_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88640_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88650_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm2801pm_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88650_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm88660_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88670_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88202_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88732_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56440_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56440_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm56450_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm56450_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm53600_a0(int unit, soc_feature_t featrue);
extern int soc_features_bcm89500_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88750_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88750_b0(int unit, soc_feature_t feature);
extern int soc_features_bcm88754_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm88950_a0(int unit, soc_feature_t feature);


/* JERICHO-2-P3 */
#ifdef BCM_88850_P3
extern int soc_features_bcm88850_p3(int unit, soc_feature_t feature);
#endif /* BCM_88850_P3 */

extern int soc_features_bcm53010_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm53020_a0(int unit, soc_feature_t feature);
extern int soc_features_bcm53400_a0(int unit, soc_feature_t feature);


extern void soc_feature_init(int unit);
extern char *soc_feature_name[];

#define soc_feature(unit, feature)      SOC_FEATURE_GET(unit, feature)

#endif  /* !_SOC_FEATURE_H */
