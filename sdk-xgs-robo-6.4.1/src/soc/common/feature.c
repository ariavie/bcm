/*
 * $Id: feature.c,v 1.937 Broadcom SDK $
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
 * Functions returning TRUE/FALSE providing chip/feature matrix.
 * TRUE means chip supports feature.
 *
 * File:        feature.c
 * Purpose:     Define features by chip, functionally.
 */

#include <shared/bsl.h>
#include <sal/types.h>
#include <soc/drv.h>
#ifdef BCM_SBX_SUPPORT
#include <soc/sbx/sbx_drv.h>
#endif
#include <soc/cm.h>
#include <soc/debug.h>
#ifdef BCM_GREYHOUND_SUPPORT
#include <soc/greyhound.h>
#endif

#if defined(SOC_FEATURE_DEBUG)
#define BSL_LAYER_SOURCE (BSL_L_SOC | BSL_S_COMMON)
#define SOC_FEATURE_DEBUG_PRINT(_x) LOG_VERBOSE(_x)
#else
#define SOC_FEATURE_DEBUG_PRINT(_x)
#endif

char    *soc_feature_name[] = SOC_FEATURE_NAME_INITIALIZER;

#ifdef  BCM_88732
/*
 * BCM88732 A0
 */
int
soc_features_bcm88732_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 88732_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);
    switch (feature) {
    case soc_feature_no_bridging:
    case soc_feature_no_higig:
    case soc_feature_no_mirror:
    case soc_feature_no_learning:
    case soc_feature_xmac:
    case soc_feature_dcb_type22:
    case soc_feature_counter_parity:
    case soc_feature_eee:
    case soc_feature_priority_flow_control:
    case soc_feature_storm_control:
    case soc_feature_flex_port:
    case soc_feature_xy_tcam:
    case soc_feature_dscp_map_per_port:
        return TRUE;
    case soc_feature_dcb_type16:
    case soc_feature_arl_hashed:
    case soc_feature_class_based_learning:
    case soc_feature_lpm_prefix_length_max_128:
    case soc_feature_tunnel_gre:
    case soc_feature_tunnel_any_in_6:
    case soc_feature_fifo_dma:
    case soc_feature_subport:
    case soc_feature_lpm_tcam:
    case soc_feature_ip_mcast:
    case soc_feature_ip_mcast_repl:
    case soc_feature_l3:
    case soc_feature_l3_ip6:
    case soc_feature_l3_lookup_cmd:
    case soc_feature_l3_sgv:
    case soc_feature_higig2:
    case soc_feature_bigmac_rxcnt_bug:
    case soc_feature_vlan_action:
    case soc_feature_mac_based_vlan:
    case soc_feature_ip_subnet_based_vlan:
    case soc_feature_vlan_translation:
    case soc_feature_vlan_ctrl:
    case soc_feature_color_prio_map:
    case soc_feature_virtual_switching:
    case soc_feature_gport_service_counters:
    case soc_feature_l2_multiple:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_l2_user_table:
    case soc_feature_filter:
    case soc_feature_field:
    case soc_feature_port_lag_failover:
    case soc_feature_e2ecc:
    case soc_feature_mem_cache:
    case soc_feature_ser_parity:
        return FALSE;
    default:
        return soc_features_bcm56820_a0(unit, feature);
    }
}
#endif

#ifdef  BCM_5675
/*
 * BCM5675 A0
 */
int
soc_features_bcm5675_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 5675_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
    case soc_feature_stat_dma:
    case soc_feature_dcb_type4:
    case soc_feature_led_proc:
    case soc_feature_xgxs_v2:
    case soc_feature_bigmac_fault_stat:
    case soc_feature_fabric_debug:
    case soc_feature_srcmod_filter:
    case soc_feature_modmap:
    case soc_feature_ip_mcast:
    case soc_feature_tx_fast_path:
    case soc_feature_policer_mode_flow_rate_committed:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_5675 */

#if defined(BCM_56504) || defined(BCM_56102) || defined(BCM_56304) || \
    defined(BCM_56514) || defined(BCM_56112) || defined(BCM_56314)
/*
 * BCM56504 A0
 */
int
soc_features_bcm56504_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         l3disable;
    int         a0;
    int         helix;

    SOC_FEATURE_DEBUG_PRINT((" 56504_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    a0    = (rev_id == BCM56504_A0_REV_ID);
    helix = (dev_id == BCM56404_DEVICE_ID);

    l3disable = (dev_id == BCM56305_DEVICE_ID || 
                 dev_id == BCM56306_DEVICE_ID ||
                 dev_id == BCM56307_DEVICE_ID || 
                 dev_id == BCM56308_DEVICE_ID ||
                 dev_id == BCM56309_DEVICE_ID ||
                 dev_id == BCM56315_DEVICE_ID || 
                 dev_id == BCM56316_DEVICE_ID ||
                 dev_id == BCM56317_DEVICE_ID || 
                 dev_id == BCM56318_DEVICE_ID ||
                 dev_id == BCM56319_DEVICE_ID ||
                 dev_id == BCM56516_DEVICE_ID ||
                 dev_id == BCM56517_DEVICE_ID || 
                 dev_id == BCM56518_DEVICE_ID ||
                 dev_id == BCM56519_DEVICE_ID || 
                 dev_id == BCM56700_DEVICE_ID ||
                 dev_id == BCM56701_DEVICE_ID);

    l3disable = (l3disable ||
                 dev_id == BCM53301_DEVICE_ID ||
                 dev_id == BCM56218_DEVICE_ID);

    l3disable = (l3disable ||
                 (SOC_SWITCH_BYPASS_MODE(unit) != SOC_SWITCH_BYPASS_MODE_NONE));

    switch (feature) {
        case soc_feature_table_dma:
        case soc_feature_tslam_dma:
        case soc_feature_dcb_type9:
        case soc_feature_schmsg_alias:
        case soc_feature_l2_lookup_cmd:
        case soc_feature_l2_user_table:
        case soc_feature_aging_extended:
        case soc_feature_arl_hashed:
        case soc_feature_l2_modfifo:
        case soc_feature_phy_cl45:
        case soc_feature_mdio_enhanced:
        case soc_feature_schan_hw_timeout:
        case soc_feature_stat_dma:
        case soc_feature_cpuport_stat_dma:
        case soc_feature_cpuport_switched:
        case soc_feature_cpuport_mirror:
        case soc_feature_fe_gig_macs:
        case soc_feature_trimac:
        case soc_feature_cos_rx_dma:
        case soc_feature_xgxs_lcpll:
        case soc_feature_dodeca_serdes:
        case soc_feature_xgxs_v5:
        case soc_feature_txdma_purge:
        case soc_feature_rxdma_cleanup:
        case soc_feature_fe_maxframe:   /* fe_maxfr = MAXFR + 1 */
        case soc_feature_vlan_mc_flood_ctrl: /* Per VLAN PFM support */

        case soc_feature_l2_hashed:
        case soc_feature_l2_lookup_retry:
        case soc_feature_arl_insert_ovr:
        case soc_feature_cfap_pool:
        case soc_feature_stg_xgs:
        case soc_feature_stg:
        case soc_feature_stack_my_modid:
        case soc_feature_remap_ut_prio:
        case soc_feature_led_proc:
        case soc_feature_field:
        case soc_feature_bigmac_fault_stat:
        case soc_feature_ingress_metering:
        case soc_feature_egress_metering:
        case soc_feature_stat_jumbo_adj:
        case soc_feature_stat_xgs3:
        case soc_feature_trunk_egress:
        case soc_feature_color:
        case soc_feature_dscp:
        case soc_feature_dscp_map_mode_all:
            /* Only 2 mapping modes. All or None */
        case soc_feature_egr_vlan_check:
        case soc_feature_xgs1_mirror:
        case soc_feature_vlan_translation:
        case soc_feature_trunk_extended:
        case soc_feature_bucket_support:
        case soc_feature_hg_trunk_override: 
        case soc_feature_hg_trunking:
        case soc_feature_field_mirror_pkts_ctl:
        case soc_feature_unknown_ipmc_tocpu:
        case soc_feature_basic_dos_ctrl:
        case soc_feature_proto_pkt_ctrl:
        case soc_feature_stat_dma_error_ack:
        case soc_feature_asf:
        case soc_feature_xport_convertible: 
        case soc_feature_sgmii_autoneg:
        case soc_feature_sample_offset8:
        case soc_feature_extended_pci_error:
        case soc_feature_l3_defip_ecmp_count:
        case soc_feature_field_action_l2_change:
        case soc_feature_special_egr_ip_tunnel_ser:
            return TRUE;
        case soc_feature_l3_defip_map:  /* Map out unused L3_DEFIP blocks */
            return a0;
        case soc_feature_status_link_fail:
        case soc_feature_egr_vlan_pfm:  /* MH PFM control per VLAN FB A0 */
        case soc_feature_asf_no_10_100:
            return (rev_id < BCM56504_B0_REV_ID);
        case soc_feature_ipmc_repl_freeze:
            return (rev_id < BCM56504_B2_REV_ID);
        case soc_feature_policer_mode_flow_rate_committed:
            return (dev_id == BCM56504_DEVICE_ID); /* Only for Firebolt proper */
        case soc_feature_lpm_tcam:
        case soc_feature_ip_mcast:
        case soc_feature_ip_mcast_repl:
        case soc_feature_l3:
        case soc_feature_l3_ip6:
        case soc_feature_l3_lookup_cmd:
        case soc_feature_l3_sgv:
            return !l3disable;
        case soc_feature_field_slices8:
            return helix;
        case soc_feature_field_slices4:
        case soc_feature_fp_based_routing: 
        case soc_feature_fp_routing_mirage:
            return (dev_id == BCM53300_DEVICE_ID);
        default:
            return FALSE;
    }
}

/*
 * BCM56504 B0
 */
int
soc_features_bcm56504_b0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56504_b0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
    case soc_feature_l2x_parity:
    case soc_feature_l3defip_parity:
            if (SAL_BOOT_BCMSIM) {
                return FALSE;
            }
            /* Fall through */
    case soc_feature_parity_err_tocpu:
    case soc_feature_nip_l3_err_tocpu:
    case soc_feature_l3mtu_fail_tocpu:
    case soc_feature_meter_adjust:      /* Adjust for IPG */
    case soc_feature_field_mirror_ovr:
    case soc_feature_field_udf_higig:
    case soc_feature_field_udf_ethertype:
    case soc_feature_field_comb_read:
    case soc_feature_field_wide:
    case soc_feature_field_slice_enable:
    case soc_feature_field_cos:
    case soc_feature_field_color_indep:
    case soc_feature_field_qual_drop:
    case soc_feature_field_qual_IpType:
    case soc_feature_field_qual_Ip6High:
    case soc_feature_src_modid_blk:
    case soc_feature_dbgc_higig_lkup:
    case soc_feature_port_trunk_index:
    case soc_feature_color_inner_cfi:
    case soc_feature_untagged_vt_miss:
    case soc_feature_module_loopback:
    case soc_feature_tunnel_dscp_trust:
    case soc_feature_higig_lookup:
    case soc_feature_egr_l3_mtu:
    case soc_feature_egr_mirror_path:
    case soc_feature_port_egr_block_ctl:
    case soc_feature_ipmc_group_mtu:
    case soc_feature_tunnel_6to4_secure:
    case soc_feature_stat_dma_error_ack:
    case soc_feature_big_icmpv6_ping_check:
    case soc_feature_src_modid_blk_ucast_override:
    case soc_feature_egress_blk_ucast_override:
    case soc_feature_static_pfm:
    case soc_feature_dcb_type13:
    case soc_feature_sample_thresh16:
        return TRUE;
    case soc_feature_src_mac_group:
    case soc_feature_remote_learn_trust:
        return (rev_id >= BCM56504_B2_REV_ID);
    case soc_feature_dcb_type9:
    case soc_feature_field_mirror_pkts_ctl:
    case soc_feature_l3x_parity:
    case soc_feature_sample_offset8:
        return FALSE;
    default:
        return soc_features_bcm56504_a0(unit, feature);
    }
}
#endif  /* BCM_565[01]4 BCM_561[01]2 BCM_563[01]4 */

#ifdef  BCM_56102
/*
 * BCM56102 A0
 */
int
soc_features_bcm56102_a0(int unit, soc_feature_t feature)
{
    SOC_FEATURE_DEBUG_PRINT((" 56102_a0"));
    switch (feature) {
    case soc_feature_l2x_parity:
    case soc_feature_parity_err_tocpu:
    case soc_feature_meter_adjust:      /* Adjust for IPG */
    case soc_feature_field_slices8:
    case soc_feature_xgxs_power:
    case soc_feature_field_mirror_ovr:
    case soc_feature_src_modid_blk:
    case soc_feature_field_udf_higig:
    case soc_feature_field_comb_read:
    case soc_feature_egr_mirror_path:
    case soc_feature_egr_vlan_check:
    case soc_feature_vlan_translation:
    case soc_feature_ipmc_repl_freeze:
    case soc_feature_stat_dma_error_ack:
    case soc_feature_big_icmpv6_ping_check:
    case soc_feature_asf_no_10_100:
    case soc_feature_static_pfm:
    case soc_feature_reset_delay:
        return TRUE;
    case soc_feature_egr_vlan_pfm:  /* MH PFM control per VLAN FB A0 only */
    case soc_feature_status_link_fail:
    case soc_feature_field_mirror_pkts_ctl:
    case soc_feature_l3x_parity:
    case soc_feature_l3_defip_ecmp_count:
        return FALSE;
    default:
        return soc_features_bcm56504_a0(unit, feature);
    }
}
#endif  /* BCM_56102 */

#ifdef  BCM_56112
/*
 * BCM56112 A0
 */
int
soc_features_bcm56112_a0(int unit, soc_feature_t feature)
{
    SOC_FEATURE_DEBUG_PRINT((" 56112_a0"));
    switch (feature) {
    case soc_feature_l2x_parity:
    case soc_feature_parity_err_tocpu:
    case soc_feature_nip_l3_err_tocpu:
    case soc_feature_meter_adjust:      /* Adjust for IPG */
    case soc_feature_field_slices8:
    case soc_feature_field_qual_Ip6High:
    case soc_feature_xgxs_power:
    case soc_feature_lmd:
    case soc_feature_field_mirror_ovr:
    case soc_feature_src_modid_blk:
    case soc_feature_field_udf_higig:
    case soc_feature_field_comb_read:
    case soc_feature_egr_mirror_path:
    case soc_feature_egr_vlan_check:
    case soc_feature_remote_learn_trust:
    case soc_feature_ipmc_group_mtu:
    case soc_feature_src_mac_group:
    case soc_feature_stat_dma_error_ack:
    case soc_feature_reset_delay:
        return TRUE;
    case soc_feature_ipmc_repl_freeze:
    case soc_feature_egr_vlan_pfm:  /* MH PFM control per VLAN FB A0 only */
    case soc_feature_status_link_fail:
    case soc_feature_asf_no_10_100:
    case soc_feature_l3x_parity:
    case soc_feature_l3_defip_ecmp_count:
        return FALSE;
    default:
        return soc_features_bcm56504_b0(unit, feature);
    }
}
#endif  /* BCM_56112 */

#ifdef  BCM_56304
/*
 * BCM56304 B0
 */
int
soc_features_bcm56304_b0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56304_b0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
    case soc_feature_l2x_parity:
    case soc_feature_parity_err_tocpu:
    case soc_feature_meter_adjust:      /* Adjust for IPG */
    case soc_feature_field_slices8:
    case soc_feature_field_mirror_ovr:
    case soc_feature_src_modid_blk:
    case soc_feature_field_udf_higig:
    case soc_feature_field_comb_read:
    case soc_feature_egr_mirror_path:
    case soc_feature_egr_vlan_check:
    case soc_feature_xgxs_power:
    case soc_feature_vlan_translation:
    case soc_feature_ipmc_repl_freeze:
    case soc_feature_stat_dma_error_ack:
    case soc_feature_big_icmpv6_ping_check:
    case soc_feature_asf_no_10_100:
    case soc_feature_static_pfm:
    case soc_feature_reset_delay:
    case soc_feature_sample_thresh16:
        return TRUE;
    case soc_feature_egr_vlan_pfm:  /* MH PFM control per VLAN FB A0 only */
    case soc_feature_status_link_fail:
    case soc_feature_field_mirror_pkts_ctl:
    case soc_feature_l3x_parity:
    case soc_feature_sample_offset8:
    case soc_feature_l3_defip_ecmp_count:
        return FALSE;
    case soc_feature_fp_based_routing: 
    case soc_feature_fp_routing_mirage:
        return (dev_id == BCM53300_DEVICE_ID);
    default:
        return soc_features_bcm56504_a0(unit, feature);
    }
}
#endif  /* BCM_56304 */

#ifdef  BCM_56314
/*
 * BCM56314 A0
 */
int
soc_features_bcm56314_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56314_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
    case soc_feature_l2x_parity:
    case soc_feature_parity_err_tocpu:
    case soc_feature_nip_l3_err_tocpu:
    case soc_feature_meter_adjust:      /* Adjust for IPG */
    case soc_feature_field_slices8:
    case soc_feature_field_qual_Ip6High:
    case soc_feature_field_mirror_ovr:
    case soc_feature_src_modid_blk:
    case soc_feature_field_udf_higig:
    case soc_feature_field_comb_read:
    case soc_feature_egr_mirror_path:
    case soc_feature_egr_vlan_check:
    case soc_feature_xgxs_power:
    case soc_feature_lmd:
    case soc_feature_remote_learn_trust:
    case soc_feature_ipmc_group_mtu:
    case soc_feature_src_mac_group:
    case soc_feature_stat_dma_error_ack:
    case soc_feature_reset_delay:
        return TRUE;
    case soc_feature_ipmc_repl_freeze:
    case soc_feature_egr_vlan_pfm:  /* MH PFM control per VLAN FB A0 only */
    case soc_feature_status_link_fail:
    case soc_feature_asf_no_10_100:
    case soc_feature_l3x_parity:
    case soc_feature_l3_defip_ecmp_count:
        return FALSE;
    default:
        return soc_features_bcm56504_b0(unit, feature);
    }
}
#endif  /* BCM_56314 */

#ifdef  BCM_56800
/*
 * BCM56800 A0
 */
int
soc_features_bcm56800_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         fabric;

    SOC_FEATURE_DEBUG_PRINT((" 56800_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    fabric = (dev_id == BCM56700_DEVICE_ID ||
              dev_id == BCM56701_DEVICE_ID ||
              dev_id == BCM56720_DEVICE_ID ||
              dev_id == BCM56721_DEVICE_ID ||
              dev_id == BCM56725_DEVICE_ID);

    switch (feature) {
    case soc_feature_dcb_type11:
    case soc_feature_higig2:
    case soc_feature_color:
    case soc_feature_color_inner_cfi:
    case soc_feature_untagged_vt_miss:
    case soc_feature_module_loopback:
    case soc_feature_xgxs_v6:
    case soc_feature_egr_l3_mtu:
    case soc_feature_egr_vlan_check:
    case soc_feature_hg_trunk_failover:
    case soc_feature_hg_trunking:
    case soc_feature_hg_trunk_override:
    case soc_feature_vlan_translation:
    case soc_feature_modmap:
    case soc_feature_two_ingress_pipes:
    case soc_feature_force_forward:
    case soc_feature_port_egr_block_ctl:
    case soc_feature_remote_learn_trust:
    case soc_feature_ipmc_group_mtu:
    case soc_feature_bigmac_rxcnt_bug:
    case soc_feature_cpu_proto_prio:
    case soc_feature_nip_l3_err_tocpu:
    case soc_feature_tunnel_6to4_secure:
    case soc_feature_field_wide:
    case soc_feature_field_qual_Ip6High:
    case soc_feature_field_mirror_ovr:
    case soc_feature_field_udf_ethertype:
    case soc_feature_field_comb_read:
    case soc_feature_field_slice_enable:
    case soc_feature_field_cos:
    case soc_feature_field_qual_drop:
    case soc_feature_field_qual_IpType:
    case soc_feature_field_udf_higig: 
    case soc_feature_field_color_indep: 
    case soc_feature_port_flow_hash:
    case soc_feature_trunk_group_size:
    case soc_feature_stat_dma_error_ack:
    case soc_feature_l3mtu_fail_tocpu:
    case soc_feature_src_modid_blk_opcode_override:
    case soc_feature_src_modid_blk_ucast_override:
    case soc_feature_egress_blk_ucast_override:
    case soc_feature_ipmc_repl_penultimate:
    case soc_feature_sgmii_autoneg: 
    case soc_feature_l3x_parity:
    case soc_feature_tunnel_dscp_trust:
        return TRUE;
    case soc_feature_dcb_type9:
    case soc_feature_egr_vlan_pfm:  /* MH PFM control per VLAN FB A0 only */
    case soc_feature_txdma_purge:
    case soc_feature_xgxs_lcpll:
    case soc_feature_xgxs_v5:
    case soc_feature_asf_no_10_100:
    case soc_feature_ipmc_repl_freeze:
        return FALSE;
    case soc_feature_ctr_xaui_activity:
        return (rev_id == BCM56800_A0_REV_ID);
    case soc_feature_urpf:
        return !fabric;
    case soc_feature_arl_hashed:
    case soc_feature_l2_user_table:
    case soc_feature_trunk_egress:
    case soc_feature_proto_pkt_ctrl:
    case soc_feature_xport_convertible: 
        if (fabric) {
            return FALSE;
        }
        /* Fall through */
    default:
        return soc_features_bcm56304_b0(unit, feature);
    }
}
#endif  /* BCM_56800 */

#ifdef  BCM_56624
/*
 * BCM56624 A0
 */
int
soc_features_bcm56624_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56624_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
    case soc_feature_dcb_type14:
    case soc_feature_table_hi:      /* > 32 bits in PBM */
    case soc_feature_vlan_ctrl:          /* per VLAN property control */  
    case soc_feature_igmp_mld_support:
    case soc_feature_trunk_group_overlay:
    case soc_feature_dscp:
    case soc_feature_dscp_map_per_port:
    case soc_feature_mac_learn_limit:
    case soc_feature_system_mac_learn_limit:
    case soc_feature_class_based_learning:
    case soc_feature_storm_control:
    case soc_feature_aging_extended:
    case soc_feature_unimac:
    case soc_feature_dual_hash:
    case soc_feature_l3_entry_key_type:
    case soc_feature_generic_table_ops:
    case soc_feature_mem_push_pop:
    case soc_feature_lpm_prefix_length_max_128:
    case soc_feature_color_prio_map:
    case soc_feature_tunnel_gre:
    case soc_feature_tunnel_any_in_6:
    case soc_feature_field_virtual_slice_group:
    case soc_feature_field_intraslice_double_wide:
    case soc_feature_field_egress_flexible_v6_key:
    case soc_feature_field_multi_stage:
    case soc_feature_field_ingress_global_meter_pools:
    case soc_feature_field_egress_global_counters:
    case soc_feature_field_ing_egr_separate_packet_byte_counters:
    case soc_feature_field_ingress_ipbm:
    case soc_feature_field_egress_metering:
    case soc_feature_dcb_reason_hi:
    case soc_feature_trunk_group_size:
    case soc_feature_multi_sbus_cmds:
    case soc_feature_esm_support:
    case soc_feature_fifo_dma:
    case soc_feature_ipfix:
    case soc_feature_src_mac_group:
    case soc_feature_higig_lookup:
    case soc_feature_xgxs_v7:
    case soc_feature_mpls:
    case soc_feature_mpls_software_failover:
    case soc_feature_subport:
    case soc_feature_l2_pending:
    case soc_feature_vlan_translation_range:
    case soc_feature_vlan_action:
    case soc_feature_mac_based_vlan:
    case soc_feature_ip_subnet_based_vlan:        
    case soc_feature_packet_rate_limit:
    case soc_feature_hw_stats_calc:
    case soc_feature_sample_thresh16:
    case soc_feature_virtual_switching:
    case soc_feature_enhanced_dos_ctrl:
    case soc_feature_use_double_freq_for_ddr_pll:
    case soc_feature_ignore_cmic_xgxs_pll_status:
    case soc_feature_rcpu_1:
    case soc_feature_l3_ingress_interface:
    case soc_feature_ppa_bypass:
    case soc_feature_egr_dscp_map_per_port:
    case soc_feature_l3_dynamic_ecmp_group:
    case soc_feature_modport_map_profile:
    case soc_feature_qos_profile:
    case soc_feature_field_action_l2_change:
    case soc_feature_vfi_mc_flood_ctrl:
    case soc_feature_mem_cache:
    case soc_feature_ser_parity:
#ifdef INCLUDE_RCPU
    case soc_feature_rcpu_priority:
    case soc_feature_rcpu_tc_mapping:
#endif /* INCLUDE_RCPU */
    case soc_feature_system_reserved_vlan:

        return TRUE;
    case soc_feature_xgxs_v6:
    case soc_feature_dcb_type11:
    case soc_feature_fe_gig_macs:
    case soc_feature_ctr_xaui_activity:
    case soc_feature_stat_dma_error_ack:
    case soc_feature_two_ingress_pipes:
    case soc_feature_field_slices8:
    case soc_feature_ipmc_group_mtu:
    case soc_feature_egr_l3_mtu:
    case soc_feature_trimac:
    case soc_feature_unknown_ipmc_tocpu:
    case soc_feature_sample_offset8:
    case soc_feature_reset_delay:
    case soc_feature_l3_defip_ecmp_count:
    case soc_feature_tunnel_dscp_trust:
        return FALSE;
    case soc_feature_always_drive_dbus:
        return (dev_id == BCM56624_DEVICE_ID || dev_id == BCM56626_DEVICE_ID ||
                dev_id == BCM56628_DEVICE_ID || dev_id == BCM56629_DEVICE_ID)
                && rev_id == BCM56624_A1_REV_ID;
    default:
        return soc_features_bcm56800_a0(unit, feature);
    }
}

/*
 * BCM56624 B0
 */
int
soc_features_bcm56624_b0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         flexible_xgport;

    SOC_FEATURE_DEBUG_PRINT((" 56624_b0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    flexible_xgport = FALSE;
    if ((dev_id == BCM56626_DEVICE_ID && rev_id != BCM56626_B0_REV_ID) ||
        (dev_id == BCM56628_DEVICE_ID && rev_id != BCM56628_B0_REV_ID)) {
        flexible_xgport =
            soc_property_get(unit, spn_FLEX_XGPORT, flexible_xgport);
    }

    switch (feature) {
    case soc_feature_xgport_one_xe_six_ge:
    case soc_feature_sample_thresh24:
    case soc_feature_flexible_dma_steps:
        return TRUE;
    case soc_feature_sample_thresh16:
    case soc_feature_use_double_freq_for_ddr_pll:
    case soc_feature_ignore_cmic_xgxs_pll_status:
        return FALSE;
    case soc_feature_flexible_xgport:
        return flexible_xgport;
    default:
        return soc_features_bcm56624_a0(unit, feature);
    }
}
#endif  /* BCM_56624 */

#ifdef  BCM_56680
/*
 * BCM56680 A0
 */
int
soc_features_bcm56680_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56680_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
    case soc_feature_esm_support:
        return FALSE;
    default:
        return soc_features_bcm56624_a0(unit, feature);
    }
}

/*
 * BCM56680 B0
 */
int
soc_features_bcm56680_b0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56680_b0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
    case soc_feature_esm_support:
    case soc_feature_flexible_xgport:
        return FALSE;
    default:
        return soc_features_bcm56624_b0(unit, feature);
    }
}
#endif  /* BCM_56680 */

#ifdef  BCM_56634
/*
 * BCM56634 A0
 */
int
soc_features_bcm56634_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         esm = TRUE;
    int         flex_port;
    soc_info_t  *si = &SOC_INFO(unit);

    SOC_FEATURE_DEBUG_PRINT((" 56634_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    if (dev_id == 0xb639 || dev_id == 0xb526 || dev_id == 0xb538) {
        esm = FALSE;
    }

    switch (dev_id) {
    case BCM56636_DEVICE_ID:
    case BCM56638_DEVICE_ID:
    case BCM56639_DEVICE_ID:
        flex_port = TRUE;
        break;
    default:
        flex_port = FALSE;
    } 

    switch (feature) {
    case soc_feature_oam:
    case soc_feature_time_support:
    case soc_feature_timesync_support:
    case soc_feature_dcb_type19:
    case soc_feature_gport_service_counters:
    case soc_feature_counter_parity:
    case soc_feature_ip_source_bind:
    case soc_feature_auto_multicast:
    case soc_feature_mpls_failover:
    case soc_feature_embedded_higig:
    case soc_feature_field_qualify_gport:
    case soc_feature_field_action_timestamp:
    case soc_feature_field_action_l2_change:
    case soc_feature_field_action_redirect_ipmc:
    case soc_feature_field_action_redirect_nexthop:
    case soc_feature_field_slice_dest_entity_select:
    case soc_feature_field_virtual_queue:
    case soc_feature_field_vfp_flex_counter:
    case soc_feature_field_packet_based_metering:
    case soc_feature_tunnel_protocol_match:
    case soc_feature_ipfix_rate:
    case soc_feature_ipfix_flow_mirror:
    case soc_feature_vlan_queue_map:
    case soc_feature_subport_enhanced:
    case soc_feature_mirror_flexible:
    case soc_feature_qos_profile:
    case soc_feature_fast_egr_cell_count:
    case soc_feature_led_data_offset_a0:
    case soc_feature_lport_tab_profile:
    case soc_feature_failover:
    case soc_feature_rx_timestamp:
    case soc_feature_sysport_remap:
    case soc_feature_timestamp_counter:
    case soc_feature_ser_parity:
    case soc_feature_pkt_tx_align:
    case soc_feature_mpls_enhanced:
    case soc_feature_e2ecc:
    case soc_feature_src_modid_blk_profile:
    case soc_feature_vpd_profile:
    case soc_feature_color_prio_map_profile:
    case soc_feature_vlan_vp:
    case soc_feature_mem_cache:
    case soc_feature_ipmc_remap:
    case soc_feature_proxy_port_property:
        return TRUE;
    case soc_feature_wlan:
    case soc_feature_egr_mirror_true:
    case soc_feature_mim:
    case soc_feature_internal_loopback:
    case soc_feature_mmu_virtual_ports:
        return si->internal_loopback;
    case soc_feature_flex_port:
        return flex_port;
    case soc_feature_esm_support:
        return esm;
    case soc_feature_dcb_type14:
    case soc_feature_use_double_freq_for_ddr_pll:
    case soc_feature_xgport_one_xe_six_ge:
    case soc_feature_flexible_xgport:
    case soc_feature_vfi_mc_flood_ctrl:
    case soc_feature_ppa_bypass:
        return FALSE;
    case soc_feature_priority_flow_control:
        return (rev_id != BCM56634_A0_REV_ID);
    case soc_feature_field_action_fabric_queue:
        return TRUE;
    case soc_feature_field_stage_half_slice:
         return (dev_id == BCM56538_DEVICE_ID);
    default:
        return soc_features_bcm56624_b0(unit, feature);
    }
}

/*
 * BCM56634 B0
 */
int
soc_features_bcm56634_b0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         mim, defip_hole = FALSE, mpls = TRUE, oam = TRUE;
    soc_info_t  *si = &SOC_INFO(unit);

    SOC_FEATURE_DEBUG_PRINT((" 56634_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);
    mim = si->internal_loopback;    
    if (dev_id == 0xb538) {
        mpls = mim = oam = FALSE;
        defip_hole = TRUE;
    }

    switch (feature) {
    case soc_feature_gmii_clkout:
        return TRUE;
    case soc_feature_l3_defip_hole:  /* Hole in L3_DEFIP block */
        return defip_hole;
    case soc_feature_unimac_tx_crs:
    case soc_feature_esm_rxfifo_resync:
    case soc_feature_ppa_match_vp:
        return TRUE;
    case soc_feature_mpls:
    case soc_feature_mpls_failover:
    case soc_feature_mpls_software_failover:
    case soc_feature_mpls_enhanced:
        return mpls;
    case soc_feature_mim:
        return mim;
    case soc_feature_oam:
        return oam;
    default:
        return soc_features_bcm56634_a0(unit, feature);
    }
}
#endif  /* BCM_56634 */

#ifdef  BCM_56524
/*
 * BCM56524 A0
 */
int
soc_features_bcm56524_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         fp_stage_half_slice;

    SOC_FEATURE_DEBUG_PRINT((" 56524_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    fp_stage_half_slice = (dev_id == BCM56520_DEVICE_ID ||
                           dev_id == BCM56521_DEVICE_ID ||
                           dev_id == BCM56522_DEVICE_ID ||
                           dev_id == BCM56524_DEVICE_ID ||
                           dev_id == BCM56526_DEVICE_ID ||
                           dev_id == BCM56534_DEVICE_ID);

    switch (feature) {
    case soc_feature_esm_support:
        return FALSE;
    case soc_feature_field_stage_half_slice:
        return fp_stage_half_slice;
    case soc_feature_flex_port:
        return (dev_id == BCM56526_DEVICE_ID);
    default:
        return soc_features_bcm56634_a0(unit, feature);
    }
}

/*
 * BCM56524 B0
 */
int
soc_features_bcm56524_b0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         mpls = TRUE, mim = TRUE, oam = TRUE, defip_hole = TRUE;

    SOC_FEATURE_DEBUG_PRINT((" 56524_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);
    if (dev_id == 0xb534) {
        mpls = mim = oam = FALSE;
    }
    if (dev_id == 0xb630) {
        defip_hole = FALSE;
    }
    switch (feature) {
    case soc_feature_gmii_clkout:
        return TRUE;
    case soc_feature_l3_defip_hole:  /* Hole in L3_DEFIP block */
        return defip_hole;
    case soc_feature_unimac_tx_crs:
        return TRUE;
    case soc_feature_mpls:
    case soc_feature_mpls_failover:
    case soc_feature_mpls_enhanced:
        return mpls;
    case soc_feature_mim:
        return mim;
    case soc_feature_oam:
        return oam;
    default:
        return soc_features_bcm56524_a0(unit, feature);
    }
}
#endif  /* BCM_56524 */

#ifdef  BCM_56685
/*
 * BCM56685 A0
 */
int
soc_features_bcm56685_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56685_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
    case soc_feature_esm_support:
    case soc_feature_wlan:
        return FALSE;
    case soc_feature_field_stage_half_slice:
        return TRUE;
    default:
        return soc_features_bcm56634_a0(unit, feature);
    }
}

/*
 * BCM56685 B0
 */
int
soc_features_bcm56685_b0(int unit, soc_feature_t feature)
{
    switch (feature) {
    case soc_feature_gmii_clkout:
        return TRUE;
    case soc_feature_unimac_tx_crs:
        return TRUE;
    default:
        return soc_features_bcm56685_a0(unit, feature);
    }
}
#endif  /* BCM_56685 */

#ifdef  BCM_56334
/*
 * BCM56334 A0
 */
int
soc_features_bcm56334_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int             helix3 = FALSE;

    SOC_FEATURE_DEBUG_PRINT((" 56334_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    helix3 = (dev_id == BCM56320_DEVICE_ID || dev_id == BCM56321_DEVICE_ID);

    switch (feature) {
    case soc_feature_esm_support:
    case soc_feature_ipfix:
    case soc_feature_dcb_type14:
    case soc_feature_register_hi:   /* > 32 bits in PBM */
    case soc_feature_table_hi:      /* > 32 bits in PBM */
    case soc_feature_ppa_bypass:
    case soc_feature_vfi_mc_flood_ctrl:
    case soc_feature_pkt_tx_align: 
    case soc_feature_sample_thresh16:
        return FALSE;
    case soc_feature_urpf:
    case soc_feature_oam:
    case soc_feature_mim:
    case soc_feature_mpls:
    case soc_feature_mpls_enhanced:
        return !helix3;
    case soc_feature_lpm_tcam:
    case soc_feature_ip_mcast:
    case soc_feature_ip_mcast_repl:
    case soc_feature_l3:
    case soc_feature_l3_ip6:
    case soc_feature_l3_lookup_cmd:
    case soc_feature_l3_sgv:
    case soc_feature_subport:
    case soc_feature_vlan_egr_it_inner_replace:
    case soc_feature_vlan_vp:
    case soc_feature_vpd_profile:
    case soc_feature_ppa_match_vp:
    case soc_feature_fifo_dma_active:
    case soc_feature_ser_parity:
        return TRUE;
    case soc_feature_delay_core_pll_lock:
        return (dev_id == BCM56333_DEVICE_ID);
    case soc_feature_dcb_type20:
    case soc_feature_field_slices8:
    case soc_feature_field_meter_pools8:
    case soc_feature_field_action_timestamp:
    case soc_feature_internal_loopback:
    case soc_feature_vlan_queue_map:
    case soc_feature_subport_enhanced:
    case soc_feature_qos_profile:
    case soc_feature_field_action_l2_change:
    case soc_feature_field_action_redirect_nexthop:
    case soc_feature_rx_timestamp:
    case soc_feature_rx_timestamp_upper:
    case soc_feature_led_data_offset_a0:
    case soc_feature_timesync_support:
    case soc_feature_timestamp_counter:
    case soc_feature_time_support:
    case soc_feature_flex_port:
    case soc_feature_lport_tab_profile:
    case soc_feature_field_virtual_queue:
    case soc_feature_gmii_clkout:
    case soc_feature_priority_flow_control:
    case soc_feature_field_slice_dest_entity_select:
    case soc_feature_ptp:
    case soc_feature_field_packet_based_metering:
    case soc_feature_color_prio_map_profile:
    case soc_feature_field_oam_actions:
    case soc_feature_regs_as_mem:
        return TRUE;
    default:
        return soc_features_bcm56624_a0(unit, feature);
    }
}

/*
 * BCM56334 B0
 */
int
soc_features_bcm56334_b0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         defip_map = FALSE;

    SOC_FEATURE_DEBUG_PRINT((" 56334_b0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    if ((dev_id == 0xb230) || (dev_id == 0xb231)) {
        defip_map = TRUE;
    }

    switch (feature) {
    case soc_feature_l3_defip_map:  /* Map out unused L3_DEFIP blocks */
        return defip_map;
    default:
        return soc_features_bcm56334_a0(unit, feature);
    }
}
#endif /* BCM_56334 */

#ifdef BCM_56142

/*
 * BCM56142 A0
 */
int
soc_features_bcm56142_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56142_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
        case soc_feature_vlan_queue_map:
        case soc_feature_field_multi_stage:
        case soc_feature_mpls:
        case soc_feature_mpls_enhanced:
        case soc_feature_mim:
        case soc_feature_oam:
        case soc_feature_field_slices8:
        case soc_feature_subport:
        case soc_feature_subport_enhanced:
        case soc_feature_lpm_prefix_length_max_128:
        case soc_feature_class_based_learning:
        case soc_feature_urpf:
        case soc_feature_l3_ingress_interface:
        case soc_feature_ipmc_repl_penultimate:
        case soc_feature_trunk_group_size:
        case soc_feature_xgs1_mirror:
        case soc_feature_failover:
        case soc_feature_vlan_vp:
        case soc_feature_ptp:
        case soc_feature_ppa_match_vp:
        case soc_feature_field_oam_actions:
        case soc_feature_ser_parity:
        case soc_feature_mem_cache:
        case soc_feature_virtual_switching:
            return FALSE;
        case soc_feature_field_slices4:
        case soc_feature_linear_drr_weight:
        case soc_feature_egr_dscp_map_per_port:
        case soc_feature_port_trunk_index:
        case soc_feature_eee:
        case soc_feature_l2_no_vfi:
        case soc_feature_int_common_init:
        case soc_feature_no_tunnel:
            return TRUE;
        default:
            return soc_features_bcm56334_a0(unit, feature);
    }
}
#endif /* BCM_56142 */

#ifdef BCM_56150

/*
 * BCM56150 A0
 */
int
soc_features_bcm56150_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         gphy, field_slice_size_128;
    int         fp_stage_multi_stage;  

    SOC_FEATURE_DEBUG_PRINT((" 56150_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    gphy = ((dev_id == BCM56150_DEVICE_ID) || 
                    (dev_id == BCM56152_DEVICE_ID) ||
                    (dev_id == BCM53342_DEVICE_ID) ||
                    (dev_id == BCM53343_DEVICE_ID) ||
                    (dev_id == BCM53344_DEVICE_ID) ||
                    (dev_id == BCM53346_DEVICE_ID) ||
                    (dev_id == BCM53333_DEVICE_ID) ||
                    (dev_id == BCM53334_DEVICE_ID)); 
    field_slice_size_128 = ((dev_id == BCM53342_DEVICE_ID) ||
                            (dev_id == BCM53343_DEVICE_ID) ||
                            (dev_id == BCM53344_DEVICE_ID) ||
                            (dev_id == BCM53346_DEVICE_ID) ||
                            (dev_id == BCM53347_DEVICE_ID) ||
                            (dev_id == BCM53393_DEVICE_ID) ||
                            (dev_id == BCM53394_DEVICE_ID));
    fp_stage_multi_stage = ((dev_id != BCM53342_DEVICE_ID) &&
                            (dev_id != BCM53343_DEVICE_ID) &&
                            (dev_id != BCM53344_DEVICE_ID) &&
                            (dev_id != BCM53346_DEVICE_ID) &&
                            (dev_id != BCM53347_DEVICE_ID) &&
                            (dev_id != BCM53393_DEVICE_ID) &&
                            (dev_id != BCM53394_DEVICE_ID));

    switch (feature) {
        case soc_feature_dcb_type20:
        case soc_feature_gmii_clkout:
        case soc_feature_field_slices4:
        case soc_feature_special_egr_ip_tunnel_ser:
        case soc_feature_virtual_switching:
            return FALSE;
        case soc_feature_iproc:
        case soc_feature_new_sbus_format:
        case soc_feature_new_sbus_old_resp:
        case soc_feature_sbusdma:
        case soc_feature_cmicm:
        case soc_feature_xlmac:
        case soc_feature_logical_port_num:
        case soc_feature_xy_tcam:
        case soc_feature_xy_tcam_direct:
        case soc_feature_unified_port:
        case soc_feature_dcb_type30:
        case soc_feature_field_slices8:
        case soc_feature_field_virtual_slice_group:
        case soc_feature_field_intraslice_double_wide:
        case soc_feature_oam:
        case soc_feature_generic_counters:
        case soc_feature_higig_misc_speed_support:
        case soc_feature_led_data_offset_a0:
        case soc_feature_time_support:
        case soc_feature_time_v3:
        case soc_feature_fifo_dma_hu2:
        case soc_feature_eee_bb_mode:
        case soc_feature_ser_parity:
        case soc_feature_mem_cache:
        case soc_feature_ser_engine:
        case soc_feature_regs_as_mem:
        case soc_feature_mem_parity_eccmask:
        case soc_feature_int_common_init:
        case soc_feature_inner_tpid_enable:
        case soc_feature_uc:
        case soc_feature_iproc_ddr:
            return TRUE;
        case soc_feature_gphy:
            return gphy;
        case soc_feature_field_slice_size128:
            return field_slice_size_128;
        case soc_feature_field_multi_stage: 
            return fp_stage_multi_stage;
        default:
            return soc_features_bcm56142_a0(unit, feature);
    }
}
#endif /* BCM_56150 */

#ifdef BCM_53400

/*
 * BCM53400 A0
 */
int
soc_features_bcm53400_a0(int unit, soc_feature_t feature)
{
    SOC_FEATURE_DEBUG_PRINT((" 53400_a0"));

    return _soc_greyhound_features(unit,feature);
}
#endif /* BCM_53400 */

#if defined(BCM_56218) || defined(BCM_56224) || defined(BCM_53314)
/*
 * BCM56218 A0
 */
int
soc_features_bcm56218_a0(int unit, soc_feature_t feature)
{
    uint32  rcpu = SOC_IS_RCPU_UNIT(unit);
    int     l3_disable, field_slices4, field_slices2, fe_ports;
    uint16  dev_id;
    uint8   rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56218_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);
   
    l3_disable = (dev_id == BCM53716_DEVICE_ID);

    field_slices4 = (dev_id == BCM53718_DEVICE_ID ||
                     dev_id == BCM53714_DEVICE_ID);

    field_slices2 = (dev_id == BCM53716_DEVICE_ID);

    fe_ports = (dev_id == BCM56014_DEVICE_ID ||
                dev_id == BCM56018_DEVICE_ID ||
                dev_id == BCM56024_DEVICE_ID ||
                dev_id == BCM56025_DEVICE_ID);
    
    switch (feature) {
        case soc_feature_l2x_parity:
            if (SAL_BOOT_BCMSIM) {
                return FALSE;
            }
            /* Fall through */
        case soc_feature_parity_err_tocpu:
        case soc_feature_nip_l3_err_tocpu:
        case soc_feature_meter_adjust:      /* Adjust for IPG */
        case soc_feature_src_modid_blk:
        case soc_feature_dcb_type12:
        case soc_feature_schmsg_alias:
        case soc_feature_l2_lookup_cmd:
        case soc_feature_l2_user_table:
        case soc_feature_aging_extended:
        case soc_feature_arl_hashed:
        case soc_feature_l2_modfifo:
        case soc_feature_mdio_enhanced:
        case soc_feature_schan_hw_timeout:
        case soc_feature_cpuport_switched:
        case soc_feature_cpuport_mirror:
        case soc_feature_fe_gig_macs:
        case soc_feature_trimac:
        case soc_feature_cos_rx_dma:
        case soc_feature_xgxs_lcpll:
        case soc_feature_dodeca_serdes:
        case soc_feature_rxdma_cleanup:
        case soc_feature_fe_maxframe:   /* fe_maxfr = MAXFR + 1 */
        case soc_feature_vlan_mc_flood_ctrl: /* Per VLAN PFM support */

        case soc_feature_trunk_extended:
        case soc_feature_l2_hashed:
        case soc_feature_l2_lookup_retry:
        case soc_feature_arl_insert_ovr:
        case soc_feature_cfap_pool:
        case soc_feature_stg_xgs:
        case soc_feature_stg:
        case soc_feature_stack_my_modid:
        case soc_feature_remap_ut_prio:
        case soc_feature_led_proc:
        case soc_feature_ingress_metering:
        case soc_feature_egress_metering:
        case soc_feature_stat_jumbo_adj:
        case soc_feature_stat_xgs3:
        case soc_feature_color:
        case soc_feature_dscp:
        case soc_feature_dscp_map_mode_all:
        case soc_feature_register_hi:   /* > 32 bits in PBM */
        case soc_feature_table_hi:      /* > 32 bits in PBM */
        case soc_feature_port_egr_block_ctl:
        case soc_feature_higig2:
        case soc_feature_storm_control:
        case soc_feature_hw_stats_calc:
        case soc_feature_cpu_proto_prio:
        case soc_feature_linear_drr_weight:     /* Linear DRR Weight calc */
        case soc_feature_igmp_mld_support:      /* IGMP/MLD snooping support */
        case soc_feature_enhanced_dos_ctrl:
        case soc_feature_src_mac_group:
        case soc_feature_tunnel_6to4_secure:
        /* Raptor supports DSCP map per port */
        case soc_feature_dscp_map_per_port: 
        case soc_feature_src_trunk_port_bridge:
        case soc_feature_big_icmpv6_ping_check:
        case soc_feature_src_modid_blk_ucast_override:
        case soc_feature_src_modid_blk_opcode_override:
        case soc_feature_egress_blk_ucast_override:
        case soc_feature_rcpu_1:
        case soc_feature_phy_cl45:
        case soc_feature_static_pfm:
        case soc_feature_sgmii_autoneg:
        case soc_feature_module_loopback:
        case soc_feature_sample_thresh16:
        case soc_feature_extended_pci_error:
        case soc_feature_directed_mirror_only:
        case soc_feature_tunnel_dscp_trust:
            return TRUE;
        case soc_feature_post:
            return (rev_id <= BCM56018_A1_REV_ID);

        case soc_feature_field:         /* Field Processor */
        case soc_feature_field_mirror_ovr:
        case soc_feature_field_udf_higig2:
        case soc_feature_field_udf_ethertype:
        case soc_feature_field_comb_read:
        case soc_feature_field_wide:
        case soc_feature_field_slice_enable:
        case soc_feature_field_cos:
        case soc_feature_field_color_indep:
        case soc_feature_field_qual_drop:
        case soc_feature_field_qual_IpType:
        case soc_feature_field_qual_Ip6High:
        case soc_feature_field_ingress_ipbm:
        case soc_feature_field_virtual_slice_group:
        case soc_feature_system_mac_learn_limit:
            return TRUE;
        case soc_feature_field_slices8:
            return !field_slices2 && !field_slices4; /* If not 2, 4, it is 8 */

        case soc_feature_field_slices4:
            return field_slices4;
            
        case soc_feature_field_slices2:
            return field_slices2;
            
        case soc_feature_l3:
        case soc_feature_l3_ip6:
        case soc_feature_fp_based_routing: 
            return !l3_disable;

        case soc_feature_table_dma:
        case soc_feature_tslam_dma:
        case soc_feature_stat_dma:
        case soc_feature_cpuport_stat_dma:
            return (rcpu) ? FALSE : TRUE;

        case soc_feature_fe_ports:
            return fe_ports;
       
        case soc_feature_stat_dma_error_ack:
        case soc_feature_sample_offset8:
        default:
            return FALSE;

    }
}
#endif  /* BCM_56218 */

#ifdef  BCM_56224
/*
 * BCM56224 A0
 */
int
soc_features_bcm56224_a0(int unit, soc_feature_t feature)
{
    int     l3_disable, field_slices4;
    uint16  dev_id;
    uint8   rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56224_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);
   
    l3_disable = (dev_id == BCM56225_DEVICE_ID ||
                  dev_id == BCM56227_DEVICE_ID ||
                  dev_id == BCM56229_DEVICE_ID ||
                  dev_id == BCM56025_DEVICE_ID ||
                  dev_id == BCM53724_DEVICE_ID ||
                  dev_id == BCM53726_DEVICE_ID); 

    field_slices4 = (dev_id == BCM53724_DEVICE_ID ||
                     dev_id == BCM53726_DEVICE_ID);

    switch (feature) {
        case soc_feature_l2x_parity:
            if (SAL_BOOT_BCMSIM) {
                return FALSE;
            } else {
                return TRUE;
            }
        case soc_feature_l3x_parity:
        case soc_feature_l3defip_parity:
            if (SAL_BOOT_BCMSIM) {
                return FALSE;
            }
            /* Fall through */
        case soc_feature_lpm_tcam:
        case soc_feature_ip_mcast:
        case soc_feature_ip_mcast_repl:
        case soc_feature_l3:
        case soc_feature_l3_ip6:
        case soc_feature_l3_lookup_cmd:
        case soc_feature_l3_sgv:
        case soc_feature_ipmc_lookup:
            return !l3_disable;
        case soc_feature_l3mtu_fail_tocpu:
            return FALSE;
        case soc_feature_unimac:
        case soc_feature_dual_hash:
        case soc_feature_dcb_type15: 
        case soc_feature_vlan_ctrl:
        case soc_feature_color:
        case soc_feature_force_forward:
        case soc_feature_color_prio_map:
        case soc_feature_hg_trunk_override:
        case soc_feature_hg_trunking:
        case soc_feature_port_trunk_index:
        case soc_feature_vlan_translation:  
        case soc_feature_egr_vlan_check:
        case soc_feature_xgxs_lcpll:
        case soc_feature_bucket_support:
        case soc_feature_ip_ep_mem_parity:
        case soc_feature_mac_learn_limit:
#ifdef INCLUDE_RCPU
        case soc_feature_rcpu_priority:
#endif /* INCLUDE_RCPU */
            return TRUE;
        case soc_feature_field_slices4:
            return field_slices4;
        case soc_feature_register_hi:   /* > 32 bits in PBM */
        case soc_feature_table_hi:      /* > 32 bits in PBM */
        case soc_feature_trimac:
        case soc_feature_dcb_type12:
        case soc_feature_field_slices8:
        case soc_feature_fp_based_routing:
            return FALSE;
        case soc_feature_post:
            return ((dev_id == BCM56024_DEVICE_ID) || 
                    (dev_id == BCM56025_DEVICE_ID));
        case soc_feature_mdio_setup:
            return (((dev_id == BCM56024_DEVICE_ID) || 
                    (dev_id == BCM56025_DEVICE_ID)) && 
                    (rev_id >= BCM56024_B0_REV_ID));
        default:
            return soc_features_bcm56218_a0(unit, feature);
    }
}

int
soc_features_bcm56224_b0(int unit, soc_feature_t feature)
{
    SOC_FEATURE_DEBUG_PRINT((" 56224_b0"));
    switch (feature) {
        case soc_feature_dcb_type18: 
        case soc_feature_fast_egr_cell_count:
            return TRUE;
        case soc_feature_dcb_type15:
        case soc_feature_post:
            return FALSE;
        default:
            return soc_features_bcm56224_a0(unit, feature);
    }
}
#endif /* BCM_56224 */

#ifdef  BCM_53314
/*
 * BCM53314 A0
 */
int
soc_features_bcm53314_a0(int unit, soc_feature_t feature)
{
    int     field_slices4, hawkeye_a0_war;
    uint16  dev_id;
    uint8   rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 53314_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);
   
    field_slices4 = (dev_id == BCM53312_DEVICE_ID ||
                     dev_id == BCM53313_DEVICE_ID ||
                     dev_id == BCM53314_DEVICE_ID ||
                     dev_id == BCM53322_DEVICE_ID ||
                     dev_id == BCM53323_DEVICE_ID ||
                     dev_id == BCM53324_DEVICE_ID );

    hawkeye_a0_war = (rev_id == BCM53314_A0_REV_ID);

    switch (feature) {
        case soc_feature_l2x_parity:
            if (SAL_BOOT_BCMSIM) {
                return FALSE;
            } else {
                return TRUE;
            }
        case soc_feature_l3x_parity:
        case soc_feature_l3defip_parity:
        case soc_feature_hg_trunk_override:
        case soc_feature_hg_trunking:
            return FALSE;
        case soc_feature_l3:
        case soc_feature_l3_ip6:
        case soc_feature_fp_based_routing: 
        case soc_feature_fp_routing_hk:
            return TRUE;            
        case soc_feature_lpm_tcam:
        case soc_feature_ip_mcast:
        case soc_feature_ip_mcast_repl:
        case soc_feature_l3_lookup_cmd:
        case soc_feature_l3_sgv:
        case soc_feature_l3mtu_fail_tocpu:
        case soc_feature_ipmc_lookup:
            return FALSE;
        case soc_feature_unimac:
        case soc_feature_dual_hash:
        case soc_feature_dcb_type17: 
        case soc_feature_vlan_ctrl:
        case soc_feature_color:
        case soc_feature_force_forward:
        case soc_feature_color_prio_map:
        case soc_feature_port_trunk_index:
        case soc_feature_vlan_translation:  
        case soc_feature_egr_vlan_check:
        case soc_feature_bucket_support:
        case soc_feature_ip_ep_mem_parity:
        case soc_feature_fast_egr_cell_count:
#ifdef INCLUDE_RCPU
        case soc_feature_rcpu_priority:
#endif /* INCLUDE_RCPU */
        case soc_feature_phy_lb_needed_in_mac_lb:
        case soc_feature_rx_timestamp:
        case soc_feature_mac_learn_limit:
        case soc_feature_system_mac_learn_limit:
        case soc_feature_timesync_support:
            return TRUE;
        case soc_feature_field_slices4:
            return field_slices4;
        case soc_feature_xgxs_lcpll:
        case soc_feature_register_hi:   /* > 32 bits in PBM */
        case soc_feature_table_hi:      /* > 32 bits in PBM */
        case soc_feature_trimac:
        case soc_feature_dcb_type12:
        case soc_feature_dcb_type15:
        case soc_feature_field_slices8:
        case soc_feature_static_pfm:
            return FALSE;
        case soc_feature_hawkeye_a0_war:
            return hawkeye_a0_war;
        default:
            return soc_features_bcm56218_a0(unit, feature);
    }
}

/*
 * BCM53324 A0
 */
int
soc_features_bcm53324_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 53324_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
        case soc_feature_field_slices4:
        case soc_feature_eee:
            return TRUE;
        case soc_feature_hawkeye_a0_war:
            return FALSE;
        default:
            return soc_features_bcm53314_a0(unit, feature);
    }
}
#endif /* BCM_53314 */

#ifdef  BCM_56514
/*
 * BCM56514 A0
 */
int
soc_features_bcm56514_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         l3disable;
#ifndef EXCLUDE_BCM56324
    int         helix2;
#endif /* EXCLUDE_BCM56324 */

    SOC_FEATURE_DEBUG_PRINT((" 56514_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

#ifndef EXCLUDE_BCM56324
    helix2 = (dev_id == BCM56324_DEVICE_ID ||
              dev_id == BCM56322_DEVICE_ID);
#endif /* EXCLUDE_BCM56324 */

    l3disable = (dev_id == BCM56516_DEVICE_ID ||
                 dev_id == BCM56517_DEVICE_ID || 
                 dev_id == BCM56518_DEVICE_ID ||
                 dev_id == BCM56519_DEVICE_ID);

    switch (feature) {
    case soc_feature_l3_defip_map:  /* Map out unused L3_DEFIP blocks */
    case soc_feature_status_link_fail:
    case soc_feature_egr_vlan_pfm:  /* MH PFM control per VLAN FB A0 */
    case soc_feature_ipmc_repl_freeze:
    case soc_feature_basic_dos_ctrl:
    case soc_feature_proto_pkt_ctrl:
    case soc_feature_dcb_type9:
    case soc_feature_unknown_ipmc_tocpu:
    case soc_feature_stat_dma_error_ack:
    case soc_feature_asf_no_10_100:
    case soc_feature_txdma_purge:
    case soc_feature_l3_defip_ecmp_count:
        return FALSE;  /* Override the 56504 B0 values*/
    case soc_feature_cpu_proto_prio:
    case soc_feature_enhanced_dos_ctrl:
    case soc_feature_igmp_mld_support:   /* IGMP/MLD snooping support */
    case soc_feature_vlan_ctrl:          /* per VLAN property control */  
    case soc_feature_trunk_group_size:   /* trunk group size support  */
    case soc_feature_dcb_type13:
    case soc_feature_hw_stats_calc:
    case soc_feature_dscp_map_per_port:
    case soc_feature_src_modid_blk_opcode_override: 
    case soc_feature_dual_hash: 
    case soc_feature_field_multi_stage:
    case soc_feature_field_virtual_slice_group:
    case soc_feature_field_qual_Ip6High:
    case soc_feature_field_intraslice_double_wide:
    case soc_feature_linear_drr_weight:     /* Linear DRR Weight calc */
    case soc_feature_src_trunk_port_bridge:
    case soc_feature_lmd:
    case soc_feature_field_intraslice_basic_key: 
    case soc_feature_force_forward:
    case soc_feature_color_prio_map:
    case soc_feature_field_egress_global_counters:
    case soc_feature_src_mac_group:
    case soc_feature_packet_rate_limit:
    case soc_feature_field_action_l2_change:
    case soc_feature_field_egress_metering:
    case soc_feature_mem_cache:
    case soc_feature_l3x_parity:
        return TRUE;
#ifndef EXCLUDE_BCM56324
    case soc_feature_field_slices8:
        return helix2;
    case soc_feature_urpf:
        return !helix2 && !l3disable;
    case soc_feature_l3defip_parity:
        if (helix2) {
            return FALSE;
        }
        /* Fall through */
#else
    case soc_feature_urpf:
        return !l3disable;
#endif /* EXCLUDE_BCM56324 */
    default:
        return soc_features_bcm56504_b0(unit, feature);
    }
}
#endif  /* BCM_56514 */

#ifdef  BCM_56820
/*
 * BCM56820 A0
 */
int
soc_features_bcm56820_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         bypass_mode;

    SOC_FEATURE_DEBUG_PRINT((" 56820_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    bypass_mode = 
        soc_property_get(unit, spn_SWITCH_BYPASS_MODE,
                         SOC_SWITCH_BYPASS_MODE_NONE);

    switch (feature) {
    case soc_feature_dcb_type16:
    case soc_feature_unimac:
    case soc_feature_xgxs_v7:
    case soc_feature_trunk_group_overlay:
    case soc_feature_trunk_group_size:
    case soc_feature_vlan_ctrl:          /* per VLAN property control */  
    case soc_feature_force_forward:
    case soc_feature_hw_stats_calc:
    case soc_feature_dual_hash:
    case soc_feature_generic_table_ops:
    case soc_feature_igmp_mld_support:
    case soc_feature_mem_push_pop:
    case soc_feature_src_mac_group:
    case soc_feature_higig_lookup:
    case soc_feature_lpm_prefix_length_max_128:
    case soc_feature_fifo_dma:
    case soc_feature_l2_pending:
    case soc_feature_packet_rate_limit:
    case soc_feature_sample_thresh16:
    case soc_feature_enhanced_dos_ctrl:
    case soc_feature_status_link_fail:
    case soc_feature_src_trunk_port_bridge:
    case soc_feature_egr_dscp_map_per_port:
    case soc_feature_port_lag_failover:
    case soc_feature_l3_dynamic_ecmp_group:
    case soc_feature_e2ecc:
    case soc_feature_mem_cache:
    case soc_feature_ser_parity:
    case soc_feature_field_egress_metering:
#ifdef INCLUDE_RCPU
    case soc_feature_rcpu_1:
    case soc_feature_rcpu_priority:
    case soc_feature_rcpu_tc_mapping:
#endif /* INCLUDE_RCPU */
        return TRUE;
    case soc_feature_lpm_tcam:
    case soc_feature_ip_mcast:
    case soc_feature_ip_mcast_repl:
    case soc_feature_l3:
    case soc_feature_l3_ip6:
    case soc_feature_l3_lookup_cmd:
    case soc_feature_l3_sgv:
    case soc_feature_field_egress_flexible_v6_key:
    case soc_feature_field_multi_stage:
    case soc_feature_field_egress_global_counters:
    case soc_feature_vlan_translation:  
    case soc_feature_untagged_vt_miss:
    case soc_feature_egr_l3_mtu:
    case soc_feature_ipmc_group_mtu:
    case soc_feature_tunnel_gre:
    case soc_feature_tunnel_any_in_6:
    case soc_feature_tunnel_6to4_secure:
    case soc_feature_unknown_ipmc_tocpu:
    case soc_feature_class_based_learning:
    case soc_feature_storm_control:
    case soc_feature_vlan_translation_range:
    case soc_feature_vlan_action:
    case soc_feature_mac_based_vlan:
    case soc_feature_ip_subnet_based_vlan:        
    case soc_feature_color:
    case soc_feature_color_prio_map:
    case soc_feature_subport:
    case soc_feature_dscp:
    case soc_feature_dscp_map_per_port:
        /* In bypass mode, all of these features are unusable */
        return bypass_mode == SOC_SWITCH_BYPASS_MODE_NONE;
   case soc_feature_field:
    case soc_feature_field_virtual_slice_group:
    case soc_feature_field_intraslice_double_wide:
    case soc_feature_field_ing_egr_separate_packet_byte_counters:
    case soc_feature_field_ingress_global_meter_pools:
    case soc_feature_field_ingress_two_slice_types:
    case soc_feature_field_slices12:
    case soc_feature_field_meter_pools4:
    case soc_feature_field_ingress_ipbm:
        /* In L3 + FP bypass mode, IFP is unavailable */
        return bypass_mode != SOC_SWITCH_BYPASS_MODE_L3_AND_FP;
    case soc_feature_xgxs_v6:
    case soc_feature_dcb_type11:
    case soc_feature_fe_gig_macs:
    case soc_feature_stat_dma_error_ack:
    case soc_feature_ctr_xaui_activity:
    case soc_feature_field_slices8:
    case soc_feature_trimac:
    case soc_feature_sample_offset8:
        return FALSE;
    case soc_feature_ignore_cmic_xgxs_pll_status:
    case soc_feature_delay_core_pll_lock:
        return (rev_id == BCM56820_A0_REV_ID);
    case soc_feature_priority_flow_control:
        return (rev_id != BCM56820_A0_REV_ID);
    case soc_feature_pkt_tx_align:
        return (rev_id >= BCM56820_B0_REV_ID);
    default:
        return soc_features_bcm56800_a0(unit, feature);
    }
}
#endif /* BCM_56820 */

#ifdef  BCM_56840
/*
 * BCM56840 A0
 */
int
soc_features_bcm56840_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56840_a0"));

    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
    case soc_feature_dcb_type19:
    case soc_feature_multi_sbus_cmds:
    case soc_feature_esm_support:
    case soc_feature_ipfix:
    case soc_feature_ipfix_rate:
    case soc_feature_ipfix_flow_mirror:
    case soc_feature_mac_learn_limit:
    case soc_feature_system_mac_learn_limit:
    case soc_feature_oam:
    case soc_feature_internal_loopback:
    case soc_feature_embedded_higig:
    case soc_feature_trunk_egress:
    case soc_feature_bigmac_rxcnt_bug:
    case soc_feature_xgs1_mirror:
    case soc_feature_sysport_remap:
    case soc_feature_timestamp_counter:
    case soc_feature_l3_defip_map:
    case soc_feature_pkt_tx_align:
        return FALSE;
    case soc_feature_field_intraslice_double_wide:
    case soc_feature_dcb_type21:
    case soc_feature_xmac:
    case soc_feature_logical_port_num:
    case soc_feature_mmu_config_property:
    case soc_feature_l2_bulk_control:
    case soc_feature_l2_bulk_bypass_replace:
    case soc_feature_two_ingress_pipes:
    case soc_feature_generic_counters:
    case soc_feature_niv:
    case soc_feature_trunk_extended_only:
    case soc_feature_hg_trunk_override_profile:
    case soc_feature_hg_dlb:
    case soc_feature_shared_trunk_member_table:
    case soc_feature_ets:
    case soc_feature_qcn:
    case soc_feature_priority_flow_control:
    case soc_feature_vp_group_ingress_vlan_membership:
    case soc_feature_eee:
    case soc_feature_ctr_xaui_activity:
    case soc_feature_vlan_egr_it_inner_replace:
    case soc_feature_flex_port:
    case soc_feature_ser_parity:
    case soc_feature_xy_tcam:
    case soc_feature_vlan_pri_cfi_action:
    case soc_feature_vlan_copy_action:
    case soc_feature_modport_map_dest_is_port_or_trunk:
    case soc_feature_mirror_encap_profile:
    case soc_feature_directed_mirror_only:
    case soc_feature_rtag1_6_max_portcnt_less_than_rtag7:
    case soc_feature_mem_cache:
    case soc_feature_mirror_control_mem:
    case soc_feature_mirror_table_trunk:
    case soc_feature_gmii_clkout:
    case soc_feature_fifo_dma_active:
    case soc_feature_regs_as_mem:
    case soc_feature_fp_meter_ser_verify:
    case soc_feature_vlan_protocol_pkt_ctrl:
    case soc_feature_field_egress_metering:
        return TRUE;
    case soc_feature_bucket_update_freeze:
        return rev_id < BCM56840_B0_REV_ID;
    case soc_feature_hg_trunk_16_members:
        return rev_id >= BCM56840_B0_REV_ID;
    case soc_feature_field_egress_flexible_v6_key:
    case soc_feature_field_multi_stage:
    case soc_feature_field_egress_global_counters:
    case soc_feature_vlan_translation:  
    case soc_feature_untagged_vt_miss:
    case soc_feature_tunnel_gre:
    case soc_feature_tunnel_any_in_6:
    case soc_feature_tunnel_6to4_secure:
    case soc_feature_class_based_learning:
    case soc_feature_storm_control:
    case soc_feature_vlan_translation_range:
    case soc_feature_vlan_action:
    case soc_feature_mac_based_vlan:
    case soc_feature_ip_subnet_based_vlan:        
    case soc_feature_color:
    case soc_feature_color_prio_map:
    case soc_feature_dscp:
    case soc_feature_dscp_map_per_port:
    case soc_feature_field:
    case soc_feature_field_virtual_slice_group:
    case soc_feature_field_ing_egr_separate_packet_byte_counters:
    case soc_feature_field_ingress_global_meter_pools:
    case soc_feature_field_ingress_two_slice_types:
    case soc_feature_field_slices10:
    case soc_feature_field_meter_pools4:
    case soc_feature_field_ingress_ipbm:
    case soc_feature_src_modid_base_index:
    case soc_feature_field_action_redirect_ecmp:
    case soc_feature_field_slice_dest_entity_select:
        if (SOC_IS_XGS_FABRIC(unit)) {
            return SOC_SWITCH_BYPASS_MODE(unit) == SOC_SWITCH_BYPASS_MODE_NONE;
        } else {
            return TRUE;
        }
        break;
    case soc_feature_urpf:
    case soc_feature_lpm_tcam:
    case soc_feature_ip_mcast:
    case soc_feature_ip_mcast_repl:
    case soc_feature_l3:
    case soc_feature_l3_ingress_interface:
    case soc_feature_trill:
    case soc_feature_l3_ip6:
    case soc_feature_l3_lookup_cmd:
    case soc_feature_l3_sgv:
    case soc_feature_wesp:
    case soc_feature_mim:
    case soc_feature_mpls:
    case soc_feature_subport:
        if (SOC_IS_XGS_FABRIC(unit)) {
            return FALSE;
        } else {
            return TRUE;
        }
    default:
        return soc_features_bcm56634_a0(unit, feature);
    }
}

/*
 * BCM56840 B0
 */
int
soc_features_bcm56840_b0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    SOC_FEATURE_DEBUG_PRINT((" 56840_b0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    if (dev_id == BCM56838_DEVICE_ID) {
        if ((feature == soc_feature_vlan_translation) ||
            (feature == soc_feature_vlan_translation_range) ||
            (feature == soc_feature_vlan_vp) ||
            (feature == soc_feature_mim) ||            
            (feature == soc_feature_trill) ||
            (feature == soc_feature_fcoe) ||   
            (feature == soc_feature_pim_bidir) || 
            (feature == soc_feature_mpls) ||            
            (feature == soc_feature_mpls_enhanced) ||
            (feature == soc_feature_mpls_failover) ||
            (feature == soc_feature_mpls_software_failover) ||
            (feature == soc_feature_priority_flow_control) ||            
            (feature == soc_feature_ets) ||            
            (feature == soc_feature_qcn) ||  
            (feature == soc_feature_asf) ||       
            (feature == soc_feature_tunnel_6to4_secure) ||
            (feature == soc_feature_tunnel_any_in_6) ||
            (feature == soc_feature_tunnel_gre) ||
            (feature == soc_feature_mac_based_vlan)) { 
            return FALSE;
        }
    }
    
    if (dev_id == BCM56831_DEVICE_ID) {
        if ((feature == soc_feature_vlan_vp) ||
            (feature == soc_feature_mim) ||
            (feature == soc_feature_trill) ||
            (feature == soc_feature_pim_bidir) || 
            (feature == soc_feature_mpls) ||
            (feature == soc_feature_mpls_enhanced) ||
            (feature == soc_feature_mpls_failover) ||
            (feature == soc_feature_mpls_software_failover) ||
            (feature == soc_feature_ets) ||
            (feature == soc_feature_tunnel_6to4_secure) ||
            (feature == soc_feature_tunnel_any_in_6) ||
            (feature == soc_feature_tunnel_gre) ||
            (feature == soc_feature_vxlan) ||             
            (feature == soc_feature_advanced_flex_counter)||
            (feature == soc_feature_urpf) ||
            (feature == soc_feature_lpm_tcam) ||            
            (feature == soc_feature_ip_mcast) ||
            (feature == soc_feature_ip_mcast_repl) ||   
            (feature == soc_feature_l3) || 
            (feature == soc_feature_l3_ingress_interface) ||            
            (feature == soc_feature_l3_ip6) ||
            (feature == soc_feature_l3_lookup_cmd) ||
            (feature == soc_feature_l3_sgv) ||
            (feature == soc_feature_l3_iif_profile) ||            
            (feature == soc_feature_l3_iif_zero_invalid) ||            
            (feature == soc_feature_l3_ecmp_1k_groups) ||  
            (feature == soc_feature_l3_extended_host_entry) ||       
            (feature == soc_feature_l3_host_ecmp_group) ||
            (feature == soc_feature_repl_l3_intf_use_next_hop) ||
            (feature == soc_feature_virtual_port_routing) ||
            (feature == soc_feature_nat) ||
            (feature == soc_feature_wesp) ||
            (feature == soc_feature_l3_defip_map) ||            
            (feature == soc_feature_l3_ip4_options_profile) ||
            (feature == soc_feature_l3_shared_defip_table)||
            (feature == soc_feature_subport) ||
            (feature == soc_feature_ip_subnet_based_vlan)){ 
            return FALSE;
        }
    }    

    if (dev_id == BCM56835_DEVICE_ID) {
        if ((feature == soc_feature_mim) ||
            (feature == soc_feature_mpls) ||
            (feature == soc_feature_mpls_enhanced) ||
            (feature == soc_feature_mpls_failover) ||
            (feature == soc_feature_mpls_software_failover)) { 
            return FALSE;
        }
    }

    switch (feature) {
    case soc_feature_vp_group_egress_vlan_membership:
    case soc_feature_xy_tcam_direct:
    case soc_feature_higig_misc_speed_support:
    case soc_feature_field_qual_my_station_hit:
    case soc_feature_vlan_protocol_pkt_ctrl: 
    case soc_feature_bucket_update_freeze:
        return TRUE;
    case soc_feature_l3_ecmp_1k_groups:
        return (dev_id == BCM56842_DEVICE_ID ||
                dev_id == BCM56844_DEVICE_ID ||
                dev_id == BCM56846_DEVICE_ID ||
                dev_id == BCM56549_DEVICE_ID ||
                dev_id == BCM56053_DEVICE_ID || 
                dev_id == BCM56831_DEVICE_ID ||
                dev_id == BCM56835_DEVICE_ID || 
                dev_id == BCM56838_DEVICE_ID ||
                dev_id == BCM56847_DEVICE_ID || 
                dev_id == BCM56849_DEVICE_ID);
    case soc_feature_l2_bulk_bypass_replace:
    case soc_feature_rtag1_6_max_portcnt_less_than_rtag7:
    case soc_feature_hg_trunk_16_members:
        return FALSE;
    default:
        return soc_features_bcm56840_a0(unit, feature);
    }
}
#endif  /* BCM_56840 */

#ifdef  BCM_56725
/*
 * BCM56725 A0
 */
int
soc_features_bcm56725_a0(int unit, soc_feature_t feature)
{
    SOC_FEATURE_DEBUG_PRINT((" 56725_a0"));
    switch (feature) {
    case soc_feature_unimac:
    case soc_feature_class_based_learning:
    case soc_feature_lpm_prefix_length_max_128:
    case soc_feature_tunnel_gre:
    case soc_feature_tunnel_any_in_6:
    case soc_feature_fifo_dma:
    case soc_feature_subport:
    case soc_feature_lpm_tcam:
    case soc_feature_ip_mcast:
    case soc_feature_ip_mcast_repl:
    case soc_feature_l3:
    case soc_feature_l3_ip6:
    case soc_feature_l3_lookup_cmd:
    case soc_feature_l3_sgv:
        return FALSE;
    default:
        return soc_features_bcm56820_a0(unit, feature);
    }
}
#endif /* BCM_56725 */

#ifdef  BCM_56640
/*
 * BCM56640 A0
 */
int
soc_features_bcm56640_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         metro_dis, l3_reddefip, l3_256_defip;
    int         fp_mp8;
    int         fp_stage_half_slice;
    int         fp_stage_quarter_slice;
    int         shared_defip_table = 0;

    SOC_FEATURE_DEBUG_PRINT((" 56640_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);
    metro_dis = (dev_id == BCM56545_DEVICE_ID ||
                 dev_id == BCM56546_DEVICE_ID ||
                 dev_id == BCM56044_DEVICE_ID ||
                 dev_id == BCM56045_DEVICE_ID ||
                 dev_id == BCM56046_DEVICE_ID);
    l3_reddefip = (dev_id == BCM56540_DEVICE_ID ||
                   dev_id == BCM56541_DEVICE_ID ||
                   dev_id == BCM56542_DEVICE_ID ||
                   dev_id == BCM56543_DEVICE_ID ||
                   dev_id == BCM56544_DEVICE_ID ||
                   dev_id == BCM56545_DEVICE_ID ||
                   dev_id == BCM56546_DEVICE_ID);

    fp_mp8 = l3_reddefip;

    fp_stage_half_slice = (dev_id == BCM56540_DEVICE_ID ||
                           dev_id == BCM56541_DEVICE_ID ||
                           dev_id == BCM56542_DEVICE_ID ||
                           dev_id == BCM56543_DEVICE_ID ||
                           dev_id == BCM56544_DEVICE_ID ||
                           dev_id == BCM56545_DEVICE_ID ||
                           dev_id == BCM56546_DEVICE_ID ||
                           dev_id == BCM56044_DEVICE_ID ||
                           dev_id == BCM56045_DEVICE_ID ||
                           dev_id == BCM56046_DEVICE_ID);

    fp_stage_quarter_slice = (dev_id == BCM56044_DEVICE_ID ||
                              dev_id == BCM56045_DEVICE_ID ||
                              dev_id == BCM56046_DEVICE_ID);

    l3_256_defip = (dev_id == BCM56044_DEVICE_ID ||
                    dev_id == BCM56045_DEVICE_ID ||
                    dev_id == BCM56046_DEVICE_ID);

    shared_defip_table = !(l3_256_defip); 

    switch (feature) {
    case soc_feature_mpls:
    case soc_feature_oam:
        return !metro_dis;
    case soc_feature_l3_reduced_defip_table:
        return l3_reddefip;
    case soc_feature_l3_256_defip_table:
        return l3_256_defip;
    case soc_feature_field_meter_pools8:
        return fp_mp8;
    case soc_feature_field_stage_half_slice:
        return fp_stage_half_slice;
    case soc_feature_etu_support:
    case soc_feature_esm_support:
    case soc_feature_esm_correction:
        return ((dev_id == BCM56640_DEVICE_ID) ||
                (dev_id == BCM56643_DEVICE_ID));
    case soc_feature_cmac:
        return (dev_id == BCM56640_DEVICE_ID);
    case soc_feature_static_repl_head_alloc:
    case soc_feature_cpu_bp_toggle:
        return (rev_id < BCM56640_B0_REV_ID);
    case soc_feature_repl_head_ptr_replace:
    case soc_feature_l2_overflow:
    case soc_feature_tr3_sp_vector_mask:
    case soc_feature_cmic_reserved_queues:
    case soc_feature_l3_extended_host_entry:
        return (rev_id >= BCM56640_B0_REV_ID);
    case soc_feature_wlan:
    case soc_feature_regex:
    case soc_feature_urpf:
    case soc_feature_ecmp_dlb:
    case soc_feature_mim:
    case soc_feature_failover:
    case soc_feature_global_meter:
    case soc_feature_mpls_software_failover:
    case soc_feature_mpls_failover:
    case soc_feature_mpls_enhanced:
    case soc_feature_mpls_entropy:
         return !((dev_id == BCM56044_DEVICE_ID) ||
                  (dev_id == BCM56045_DEVICE_ID) ||
                  (dev_id == BCM56046_DEVICE_ID));
    case soc_feature_system_mac_learn_limit:
    case soc_feature_internal_loopback:
    case soc_feature_trunk_egress:
    case soc_feature_bigmac_rxcnt_bug:
    case soc_feature_two_ingress_pipes:
    case soc_feature_unimac:
    case soc_feature_dcb_type21:
    case soc_feature_gport_service_counters:
    case soc_feature_field_vfp_flex_counter:
    case soc_feature_flexible_dma_steps:
    case soc_feature_field_ingress_two_slice_types:
    case soc_feature_field_slices10:
    case soc_feature_field_meter_pools4:
    case soc_feature_asf: 
    case soc_feature_port_lag_failover:
    case soc_feature_fp_meter_ser_verify:
    case soc_feature_timesync_support:
    case soc_feature_pkt_tx_align: 
    case soc_feature_special_egr_ip_tunnel_ser:
        return FALSE;
    case soc_feature_new_sbus_format:
    case soc_feature_cmicm:
    case soc_feature_mcs:
    case soc_feature_uc:
    case soc_feature_cmicm_extended_interrupts:
    case soc_feature_ism_memory:
    case soc_feature_sbusdma:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    case soc_feature_fifo_dma:
    case soc_feature_stat_dma:
    case soc_feature_multi_sbus_cmds:
    case soc_feature_dcb_type23:
    case soc_feature_logical_port_num:
    case soc_feature_unified_port:
    case soc_feature_generic_counters:
    case soc_feature_stat_xgs3:
    case soc_feature_ipfix:
    case soc_feature_ipfix_rate:
    case soc_feature_ipfix_flow_mirror:
    case soc_feature_xy_tcam:
    case soc_feature_mac_learn_limit:
    case soc_feature_axp:
    case soc_feature_l3_ecmp_1k_groups:
    case soc_feature_time_support:
    case soc_feature_hg_dlb_member_id:
    case soc_feature_lag_dlb:
    case soc_feature_advanced_flex_counter:
    case soc_feature_ets:
    case soc_feature_qcn:
    case soc_feature_l2gre:
    case soc_feature_vlan_double_tag_range_compress:
    case soc_feature_vlan_protocol_pkt_ctrl:
    case soc_feature_embedded_higig:
    case soc_feature_remote_encap:
    case soc_feature_rx_reason_overlay:
    case soc_feature_ptp:
    case soc_feature_field_oam_actions:
    case soc_feature_egr_mirror_true:
    case soc_feature_flex_port: 
    case soc_feature_ep_redirect: 
    case soc_feature_repl_l3_intf_use_next_hop:
    case soc_feature_bfd:
    case soc_feature_port_extension:
    case soc_feature_gmii_clkout:
    case soc_feature_service_queuing:
    case soc_feature_l3_defip_map:
    case soc_feature_vlan_xlate_dtag_range:
    case soc_feature_field_ingress_cosq_override:
    case soc_feature_bhh:
    case soc_feature_post:
    case soc_feature_field_egress_tocpu:
    case soc_feature_ser_fifo:
    case soc_feature_rx_timestamp_upper:
    case soc_feature_mirror_cos:
        return TRUE;
    case soc_feature_field_stage_quarter_slice:
        return fp_stage_quarter_slice; 
    case soc_feature_l3_shared_defip_table: 
        return shared_defip_table;
    case soc_feature_field_egress_metering:
        if (dev_id == BCM56640_DEVICE_ID) {
            if ((rev_id == BCM56640_A0_REV_ID) ||
                (rev_id == BCM56640_A1_REV_ID)) {
                return FALSE;
            }
        }
        return TRUE;
    default:
        return soc_features_bcm56840_b0(unit, feature);
    }
}
#endif  /* BCM_56640 */

#ifdef  BCM_56340
/*
 * BCM56340 A0
 */
int
soc_features_bcm56340_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         ehost_mode;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    SOC_FEATURE_DEBUG_PRINT((" 56340_a0"));
    ehost_mode   = !(soc_cm_get_bus_type(unit) & SOC_AXI_DEV_TYPE);

    switch (feature) {
    case soc_feature_esm_support:
    case soc_feature_ipfix:
    case soc_feature_ipfix_rate:
    case soc_feature_ipfix_flow_mirror:
    case soc_feature_ecmp_dlb:
    case soc_feature_mcs:
    case soc_feature_static_repl_head_alloc:
    case soc_feature_l3_shared_defip_table:
        return FALSE;
    case soc_feature_uc:
        return ehost_mode;
    case soc_feature_oam:
    case soc_feature_wlan:
    case soc_feature_regex:
    case soc_feature_urpf:
    case soc_feature_mim:
    case soc_feature_failover:
    case soc_feature_global_meter:
    case soc_feature_mpls:
    case soc_feature_mpls_software_failover:
    case soc_feature_mpls_failover:
    case soc_feature_mpls_enhanced:
    case soc_feature_mpls_entropy:
        return !((dev_id == BCM56040_DEVICE_ID) ||
                 (dev_id == BCM56041_DEVICE_ID) || 
                 (dev_id == BCM56042_DEVICE_ID) ||
                 (dev_id == BCM56047_DEVICE_ID) ||
                 (dev_id == BCM56048_DEVICE_ID) ||                  
                 (dev_id == BCM56049_DEVICE_ID));

    case soc_feature_iproc:
    case soc_feature_l2_overflow:
    case soc_feature_tr3_sp_vector_mask:
    case soc_feature_repl_head_ptr_replace:
    case soc_feature_ser_hw_bg_read:
    case soc_feature_cmic_reserved_queues:
    case soc_feature_l3_expanded_defip_table:
    case soc_feature_timesync_support:
    case soc_feature_timestamp_counter:  
    case soc_feature_time_support:       
    case soc_feature_ptp:               
        return TRUE;
    case soc_feature_l3_256_defip_table:
    case soc_feature_field_stage_half_slice:
    case soc_feature_field_stage_ingress_256_half_slice:
    case soc_feature_field_stage_egress_256_half_slice:
    case soc_feature_field_stage_lookup_512_half_slice:
        return ((dev_id == BCM56040_DEVICE_ID) ||
                (dev_id == BCM56041_DEVICE_ID) ||
                (dev_id == BCM56042_DEVICE_ID));

    case soc_feature_l3:
    case soc_feature_vlan_translation:
    case soc_feature_lpm_tcam:
    case soc_feature_ip_mcast:
    case soc_feature_advanced_flex_counter:
    case soc_feature_niv:
        return !((BCM56047_DEVICE_ID == dev_id) ||
                 (BCM56048_DEVICE_ID == dev_id) ||                  
                 (BCM56049_DEVICE_ID == dev_id));      

    default:
        return soc_features_bcm56640_a0(unit, feature);
        
    }
    
}
#endif  /* BCM_56340 */

#ifdef  BCM_56850
/*
 * BCM56850 A0
 */
int
soc_features_bcm56850_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    int         feature_enable;    
    SOC_FEATURE_DEBUG_PRINT((" 56850_a0"));

    soc_cm_get_id(unit, &dev_id, &rev_id);

    feature_enable = (dev_id != BCM56830_DEVICE_ID);

    if (dev_id == BCM56830_DEVICE_ID) {
        if ((feature == soc_feature_qcn) ||
            (feature == soc_feature_tunnel_6to4_secure) ||
            (feature == soc_feature_tunnel_any_in_6) ||
            (feature == soc_feature_tunnel_gre) ||
            (feature == soc_feature_hg_dlb) ||
            (feature == soc_feature_xport_convertible) ||
            (feature == soc_feature_ets) ||
            (feature == soc_feature_mpls_enhanced) ||
            (feature == soc_feature_mpls_failover) ||
            (feature == soc_feature_virtual_switching) ||
            (feature == soc_feature_subport_enhanced) ||
            (feature == soc_feature_mpls_software_failover) ||
            (feature == soc_feature_field_action_l2_change)) { 
            return FALSE;
        }

        if ((feature == soc_feature_int_common_init) ||
            (feature == soc_feature_field_stage_lookup_256_half_slice) ||
            (feature == soc_feature_field_stage_egress_256_half_slice) ||
            (feature == soc_feature_field_stage_ingress_512_half_slice) ||
            (feature == soc_feature_field_slices8)) {
            return TRUE;
        }
    }

    if (dev_id == BCM56834_DEVICE_ID) {
        if ((feature == soc_feature_ets) ||
            (feature == soc_feature_subport)||            
            (feature == soc_feature_l2gre) ||            
            (feature == soc_feature_l2gre_default_tunnel) ||
            (feature == soc_feature_vxlan) ||            
            (feature == soc_feature_mpls) ||            
            (feature == soc_feature_mpls_enhanced) ||
            (feature == soc_feature_mpls_failover) ||
            (feature == soc_feature_mpls_software_failover) ||
            (feature == soc_feature_tunnel_6to4_secure) ||
            (feature == soc_feature_tunnel_any_in_6) ||
            (feature == soc_feature_tunnel_gre) ||         
            (feature == soc_feature_vlan_vp) ||         
            (feature == soc_feature_vp_lag) ||
            (feature == soc_feature_mim) ||
            (feature == soc_feature_trill) ||
            (feature == soc_feature_fcoe) ||
            (feature == soc_feature_vxlan) ||
            (feature == soc_feature_urpf) ||
            (feature == soc_feature_pim_bidir) ||
            (feature == soc_feature_lpm_tcam) ||            
            (feature == soc_feature_ip_mcast) ||
            (feature == soc_feature_ip_mcast_repl) ||   
            (feature == soc_feature_l3) || 
            (feature == soc_feature_l3_ingress_interface) ||            
            (feature == soc_feature_l3_ip6) ||
            (feature == soc_feature_l3_lookup_cmd) ||
            (feature == soc_feature_l3_sgv) ||
            (feature == soc_feature_l3_iif_profile) ||            
            (feature == soc_feature_l3_iif_zero_invalid) ||            
            (feature == soc_feature_l3_ecmp_1k_groups) ||  
            (feature == soc_feature_l3_extended_host_entry) ||       
            (feature == soc_feature_l3_host_ecmp_group) ||
            (feature == soc_feature_repl_l3_intf_use_next_hop) ||
            (feature == soc_feature_virtual_port_routing) ||
            (feature == soc_feature_nat) ||
            (feature == soc_feature_wesp) ||
            (feature == soc_feature_l3_defip_map) ||            
            (feature == soc_feature_l3_ip4_options_profile) ||
            (feature == soc_feature_l3_shared_defip_table)) { 
            return FALSE;
        }
    }    

    switch (feature) {
    case soc_feature_dcb_type21:
    case soc_feature_xmac:
    case soc_feature_unimac:
    case soc_feature_gport_service_counters:
    case soc_feature_field_vfp_flex_counter:
    case soc_feature_fp_meter_ser_verify:
    case soc_feature_field_meter_pools4:
    case soc_feature_special_egr_ip_tunnel_ser:
    case soc_feature_ctr_xaui_activity: 
        return FALSE;
    case soc_feature_dcb_type26:
    case soc_feature_cmicm:
    case soc_feature_mcs:
    case soc_feature_uc:
    case soc_feature_bfd:
    case soc_feature_sbusdma:
    case soc_feature_new_sbus_format:
    case soc_feature_unified_port:
    case soc_feature_xlmac:
    case soc_feature_vlan_double_tag_range_compress:
    case soc_feature_split_repl_group_table:
    case soc_feature_shared_hash_mem:
    case soc_feature_endpoint_queuing:
    case soc_feature_service_queuing:
    case soc_feature_mac_virtual_port:
    case soc_feature_ing_vp_vlan_membership:
    case soc_feature_egr_vp_vlan_membership:
    case soc_feature_vp_lag:
    case soc_feature_vlan_xlate_dtag_range:
    case soc_feature_min_resume_limit_1:
    case soc_feature_hg_dlb_member_id:
    case soc_feature_hg_dlb_id_equal_hg_tid:
    case soc_feature_min_cell_per_queue:
    case soc_feature_post:
    case soc_feature_time_support:
    case soc_feature_ptp:
    case soc_feature_l3_ip4_options_profile: 
    case soc_feature_ser_fifo:
    case soc_feature_l2_overflow:        
    case soc_feature_field_meter_pools8:    
    case soc_feature_l3_shared_defip_table:
    case soc_feature_overlaid_address_class:
    case soc_feature_mirror_cos:
    case soc_feature_field_ingress_cosq_override:
        return TRUE;
    case soc_feature_advanced_flex_counter:
    case soc_feature_port_extension:
    case soc_feature_hg_resilient_hash:
    case soc_feature_lag_resilient_hash:
    case soc_feature_ecmp_resilient_hash:
    case soc_feature_fcoe:
        return feature_enable; 
    case soc_feature_cmac:
        return (dev_id == BCM56850_DEVICE_ID || dev_id == BCM56852_DEVICE_ID ||
                dev_id == BCM56853_DEVICE_ID || dev_id == BCM56750_DEVICE_ID);
    case soc_feature_urpf:
    case soc_feature_pim_bidir:
    case soc_feature_lpm_tcam:
    case soc_feature_ip_mcast:
    case soc_feature_ip_mcast_repl:
    case soc_feature_l3:
    case soc_feature_l3_ingress_interface:
    case soc_feature_l3_ip6:
    case soc_feature_l3_lookup_cmd:
    case soc_feature_l3_sgv:
    case soc_feature_l3_iif_profile:
    case soc_feature_l3_iif_zero_invalid:
    case soc_feature_l3_ecmp_1k_groups:
    case soc_feature_l3_extended_host_entry:
    case soc_feature_l3_host_ecmp_group:
    case soc_feature_repl_l3_intf_use_next_hop:
    case soc_feature_virtual_port_routing:
    case soc_feature_nat:
    case soc_feature_wesp:
    case soc_feature_mpls:
    case soc_feature_mim:
    case soc_feature_trill:
    case soc_feature_l2gre:
    case soc_feature_l2gre_default_tunnel:
    case soc_feature_vxlan:
    case soc_feature_subport:
    case soc_feature_l3_defip_map:
        if (SOC_IS_XGS_FABRIC(unit)) {
            return FALSE;
        } else {
            return feature_enable;
        }
    case soc_feature_cmic_reserved_queues:
        return (rev_id >= BCM56850_A1_REV_ID) ? TRUE : FALSE;
    case soc_feature_pgw_mac_rsv_mask_remap:
        return rev_id <= BCM56850_A1_REV_ID ? TRUE : FALSE;
    case soc_feature_shared_hash_ins:
        return (rev_id == BCM56850_A0_REV_ID ||
                rev_id == BCM56850_A1_REV_ID) ? TRUE : FALSE;
    case soc_feature_port_lag_failover:
    case soc_feature_ipmc_use_configured_dest_mac:
        return TRUE;
    case soc_feature_l3_lpm_scaling_enable:
        if (soc_property_get(unit, spn_LPM_SCALING_ENABLE, 0)) {
            return TRUE;
        }
        return FALSE;
    case soc_feature_l3_lpm_128b_entries_reserved:
        if (soc_property_get(unit, spn_LPM_SCALING_ENABLE, 0)) {
            if(soc_property_get(unit, spn_LPM_IPV6_128B_RESERVED, 1)) {
                return TRUE;
            }
            return FALSE;
        }
        return TRUE;

    default:
        return soc_features_bcm56840_b0(unit, feature);
    }
}
#endif  /* BCM_56850 */

#ifdef  BCM_5324
/*
 * BCM5324 XX
 */
int
soc_features_bcm5324_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 5324_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    case soc_feature_mac_learn_limit:
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_arl_mode_control:
        return TRUE;
    default:
        return FALSE;
    }
}

int
soc_features_bcm5324_a1(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 5324_a1"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    case soc_feature_mac_learn_limit:
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_arl_mode_control:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_5324 */


#ifdef  BCM_5396

int
soc_features_bcm5396_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 5396_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    /* soc_feature_dodeca_serdes: for fiber with SerDes + PHY_PassThrough */
    case soc_feature_dodeca_serdes: 
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_no_stat_mib:
    case soc_feature_sgmii_autoneg:        
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_5396 */

#ifdef  BCM_5389

int
soc_features_bcm5389_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 5389_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:   
    /* soc_feature_dodeca_serdes: for fiber with SerDes + PHY_PassThrough */
    case soc_feature_dodeca_serdes: 
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
    case soc_feature_sgmii_autoneg:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_5389 */

#ifdef  BCM_5398

int
soc_features_bcm5398_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 5398_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_arl_mode_control:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_5398 */

#ifdef  BCM_5348
/*
 * BCM5324 XX
 */
int
soc_features_bcm5348_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 5348_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    case soc_feature_mac_learn_limit:
    /* soc_feature_dodeca_serdes: for fiber with SerDes + PHY_PassThrough */
    case soc_feature_dodeca_serdes:         
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_field:
    case soc_feature_field_slice_enable:
    case soc_feature_arl_mode_control:
    case soc_feature_robo_ge_serdes_mac_autosync:             
    case soc_feature_sgmii_autoneg:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_5348 */

#ifdef  BCM_5397

int
soc_features_bcm5397_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 5397_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_arl_mode_control:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_5397 */

#ifdef  BCM_5347
int
soc_features_bcm5347_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 5347_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    case soc_feature_mac_learn_limit:
    /* soc_feature_dodeca_serdes: for fiber with SerDes + PHY_PassThrough */
    case soc_feature_dodeca_serdes: 
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_field:
    case soc_feature_field_slice_enable:
    case soc_feature_arl_mode_control:
    case soc_feature_robo_ge_serdes_mac_autosync:             
    case soc_feature_sgmii_autoneg:        
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_5347 */
#ifdef  BCM_5395
int
soc_features_bcm5395_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 5395_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_field:
    case soc_feature_field_slice_enable:
    case soc_feature_arl_mode_control:
    case soc_feature_hw_dos_prev:
    case soc_feature_tag_enforcement:
    case soc_feature_igmp_mld_support:
    case soc_feature_eav_support:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_5395 */

#ifdef  BCM_53242
int
soc_features_bcm53242_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 53242_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    case soc_feature_mac_learn_limit:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_field:
    case soc_feature_arl_mode_control:
    case soc_feature_tag_enforcement:
    case soc_feature_vlan_translation:
    case soc_feature_igmp_mld_support:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_53242 */

#ifdef  BCM_FE2000_A0
/*
 * BCM88020 A0
 */
int
soc_features_bcm88020_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
    case soc_feature_dodeca_serdes: 
    case soc_feature_xgxs_v5:
    case soc_feature_lmd:
    case soc_feature_xgxs_lcpll:
    case soc_feature_xport_convertible: 
    case soc_feature_phy_cl45:
    case soc_feature_higig2:
    case soc_feature_field:
        return TRUE;
    default:
        return FALSE;
    }
}
/*
 * BCM88025 A0
 */
int
soc_features_bcm88025_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
    case soc_feature_dodeca_serdes: 
    case soc_feature_xgxs_v5:
    case soc_feature_lmd:
    case soc_feature_xgxs_lcpll:
    case soc_feature_xport_convertible: 
    case soc_feature_phy_cl45:
    case soc_feature_higig2:
    case soc_feature_field:
        return TRUE;
    default:
        return FALSE;
    }
}

#endif  /* BCM_FE2000_A0 */

#ifdef  BCM_88030
/*
 * BCM88030 A0
 */
int
soc_features_bcm88030_a0(int unit, soc_feature_t feature)
{
    
    uint16      dev_id;
    uint8       rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
        case soc_feature_cmicm:
        case soc_feature_time_support:
        case soc_feature_mcs:
        case soc_feature_uc:
        case soc_feature_dcb_type25: 
        case soc_feature_table_dma:
        case soc_feature_tslam_dma:
        case soc_feature_fifo_dma:
        case soc_feature_multi_sbus_cmds:
        case soc_feature_extended_cmic_error:
        case soc_feature_cmicm_extended_interrupts:
        case soc_feature_cmicm_multi_dma_cmc:
        case soc_feature_higig2:
        case soc_feature_new_sbus_format:
        case soc_feature_sbusdma:
        case soc_feature_cos_rx_dma:
        case soc_feature_phy_cl45:
        case soc_feature_mdio_enhanced:
        case soc_feature_sgmii_autoneg:
        case soc_feature_logical_port_num:
        case soc_feature_unified_port:
        case soc_feature_controlled_counters:
        case soc_feature_cmac:
        case soc_feature_ddr3:
        case soc_feature_led_proc:
        case soc_feature_led_data_offset_a0:			
        return TRUE;

    default:
        return FALSE;
    }
}
/*
 * BCM88030 A1
 */
int
soc_features_bcm88030_a1(int unit, soc_feature_t feature)
{
    
    uint16      dev_id;
    uint8       rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
        case soc_feature_cmicm:
        case soc_feature_time_support:
        case soc_feature_mcs:
        case soc_feature_uc:
        case soc_feature_dcb_type25: 
        case soc_feature_table_dma:
        case soc_feature_tslam_dma:
        case soc_feature_fifo_dma:
        case soc_feature_multi_sbus_cmds:
        case soc_feature_extended_cmic_error:
        case soc_feature_cmicm_extended_interrupts:
        case soc_feature_cmicm_multi_dma_cmc:
        case soc_feature_higig2:
        case soc_feature_new_sbus_format:
        case soc_feature_sbusdma:
        case soc_feature_cos_rx_dma:
        case soc_feature_phy_cl45:
        case soc_feature_mdio_enhanced:
        case soc_feature_sgmii_autoneg:
        case soc_feature_logical_port_num:
        case soc_feature_unified_port:
        case soc_feature_controlled_counters:
        case soc_feature_cmac:
        case soc_feature_ddr3:
        case soc_feature_led_proc:
        case soc_feature_led_data_offset_a0:
        return TRUE;

    default:
        return FALSE;
    }
}
/*
 * BCM88030 B0
 */
int
soc_features_bcm88030_b0(int unit, soc_feature_t feature)
{
    
    uint16      dev_id;
    uint8       rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (feature) {
        case soc_feature_cmicm:
        case soc_feature_time_support:
        case soc_feature_mcs:
        case soc_feature_uc:
        case soc_feature_dcb_type25: 
        case soc_feature_table_dma:
        case soc_feature_tslam_dma:
        case soc_feature_fifo_dma:
        case soc_feature_multi_sbus_cmds:
        case soc_feature_extended_cmic_error:
        case soc_feature_cmicm_extended_interrupts:
        case soc_feature_cmicm_multi_dma_cmc:
        case soc_feature_higig2:
        case soc_feature_new_sbus_format:
        case soc_feature_sbusdma:
        case soc_feature_cos_rx_dma:
        case soc_feature_phy_cl45:
        case soc_feature_mdio_enhanced:
        case soc_feature_sgmii_autoneg:
        case soc_feature_logical_port_num:
        case soc_feature_unified_port:
        case soc_feature_controlled_counters:
        case soc_feature_cmac:
        case soc_feature_ddr3:
        case soc_feature_led_proc:
        case soc_feature_led_data_offset_a0:
        return TRUE;

    default:
        return FALSE;
    }
}
#endif  /* BCM_88030 */

#ifdef  BCM_88230
/*
 * BCM88230 A0
 */
int
soc_features_bcm88230_a0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    uint32      tme_mode, hybrid_mode;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    /* hardcoded soc features */
    switch (feature) {
        /* case soc_feature_dcb_type27:*/
        case soc_feature_dcb_type19:
        case soc_feature_schmsg_alias:
        case soc_feature_table_dma:
        case soc_feature_tslam_dma:
	case soc_feature_stat_dma:
        case soc_feature_higig2:
        case soc_feature_mem_push_pop:
        case soc_feature_fifo_dma:
        case soc_feature_mc_group_ability:
        case soc_feature_packet_adj_len:
        case soc_feature_multi_sbus_cmds:
        case soc_feature_extended_cmic_error:
        case soc_feature_unimac: /* for lab bringup only */
        case soc_feature_phy_cl45:
        case soc_feature_mdio_enhanced:
        case soc_feature_sgmii_autoneg:
        case soc_feature_ingress_size_templates:
            return TRUE;
            break;
        default:
            break;
    }

    /* soc features depends on configuration */
    if (SOC_CONTROL(unit) == NULL) {
        return(FALSE);
    }
    if (SOC_SBX_CONTROL(unit) == NULL) {
        return(FALSE);
    }
    if (SOC_SBX_CFG(unit) == NULL) {
        return(FALSE);
    }

    tme_mode = soc_property_get(unit, spn_QE_TME_MODE, SOC_SBX_CFG(unit)->bTmeMode);
    hybrid_mode = soc_property_get(unit, spn_HYBRID_MODE, SOC_SBX_CFG(unit)->bHybridMode);

    switch (feature) {
        case soc_feature_discard_ability:
            return( ((SOC_SBX_CFG(unit)->bTmeMode == SOC_SBX_QE_MODE_TME) ||
                     (SOC_SBX_CFG(unit)->bTmeMode == SOC_SBX_QE_MODE_TME_BYPASS) ||
                     (SOC_SBX_CFG(unit)->bTmeMode ==  SOC_SBX_QE_MODE_HYBRID)) ?
                    TRUE : FALSE );
            break;
        case soc_feature_egress_metering:
            /* Support is available for egress shaping */
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "rate egress_metering feature available\n")));
            return( TRUE );
            break;
        case soc_feature_cosq_gport_stat_ability:
            /* Support is available for gport statistics */
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "gport statistics feature available\n")));
            return( TRUE );
            break;
        case soc_feature_standalone:
            if ((tme_mode == SOC_SBX_QE_MODE_TME) ||
                (tme_mode == SOC_SBX_QE_MODE_TME_BYPASS)) {
                return TRUE;
            } else {
                return FALSE;
            } 
            break;
        case soc_feature_hybrid:
            return( ( (hybrid_mode == TRUE) &&
                      (tme_mode != TRUE /* SOC_SBX_QE_MODE_TME */) ) ? TRUE : FALSE);
            break;
        case soc_feature_node:
            return(TRUE);
            break;
        case soc_feature_node_hybrid:
            return( ((tme_mode == SOC_SBX_QE_MODE_HYBRID) ? TRUE : FALSE) );
            break;
        case soc_feature_egr_independent_fc:
            switch (SOC_SBX_CFG(unit)->uFabricConfig) {
                case SOC_SBX_SYSTEM_CFG_VPORT:
                case SOC_SBX_SYSTEM_CFG_VPORT_LEGACY:
                case SOC_SBX_SYSTEM_CFG_VPORT_MIX:
                    return((SOC_SBX_CFG(unit)->bEgressFifoIndependentFlowControl == FALSE) ? FALSE : TRUE);
                    break;

                case SOC_SBX_SYSTEM_CFG_DMODE:
                default:
                    return(FALSE);
                    break;
            }
            break;
        case soc_feature_egr_multicast_independent_fc:
            return((SOC_SBX_CFG(unit)->bEgressMulticastFifoIndependentFlowControl == FALSE) ? FALSE : TRUE);
            break;
        case soc_feature_discard_ability_color_black:
            return (TRUE);
            break;
        case soc_feature_distribution_ability:
            /* ESET need to be awared for FIC traffic for node type */
            return ( ((tme_mode == SOC_SBX_QE_MODE_TME) ||
                      (tme_mode == SOC_SBX_QE_MODE_TME_BYPASS))
                      ? FALSE : TRUE );
            break;
        default:
            return FALSE;
    }
}

/*
 * BCM88230 B0
 */
int
soc_features_bcm88230_b0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    uint32      tme_mode, hybrid_mode;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    /* hardcoded soc features */
    switch (feature) {
        /* case soc_feature_dcb_type27:*/
        case soc_feature_dcb_type19:
        case soc_feature_schmsg_alias:
        case soc_feature_table_dma:
        case soc_feature_tslam_dma:
	case soc_feature_stat_dma:
        case soc_feature_higig2:
        case soc_feature_mem_push_pop:
        case soc_feature_fifo_dma:
        case soc_feature_mc_group_ability:
        case soc_feature_packet_adj_len:
        case soc_feature_multi_sbus_cmds:
        case soc_feature_extended_cmic_error:
        case soc_feature_unimac: /* for lab bringup only */
        case soc_feature_phy_cl45:
        case soc_feature_mdio_enhanced:
        case soc_feature_sgmii_autoneg:
        case soc_feature_ingress_size_templates:
        case soc_feature_priority_flow_control:
            return TRUE;
            break;
        default:
            break;
    }

    /* soc features depends on configuration */
    if (SOC_CONTROL(unit) == NULL) {
        return(FALSE);
    }
    if (SOC_SBX_CONTROL(unit) == NULL) {
        return(FALSE);
    }
    if (SOC_SBX_CFG(unit) == NULL) {
        return(FALSE);
    }

    tme_mode = soc_property_get(unit, spn_QE_TME_MODE, SOC_SBX_CFG(unit)->bTmeMode);
    hybrid_mode = soc_property_get(unit, spn_HYBRID_MODE, SOC_SBX_CFG(unit)->bHybridMode);

    switch (feature) {
        case soc_feature_discard_ability:
            return( ((SOC_SBX_CFG(unit)->bTmeMode == SOC_SBX_QE_MODE_TME) ||
                     (SOC_SBX_CFG(unit)->bTmeMode == SOC_SBX_QE_MODE_TME_BYPASS) ||
                     (SOC_SBX_CFG(unit)->bTmeMode ==  SOC_SBX_QE_MODE_HYBRID)) ?
                    TRUE : FALSE );
            break;
        case soc_feature_egress_metering:
            /* Support is available for egress shaping */
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "rate egress_metering feature available\n")));
            return( TRUE );
            break;
        case soc_feature_cosq_gport_stat_ability:
            /* Support is available for gport statistics */
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "gport statistics feature available\n")));
            return( TRUE );
            break;
        case soc_feature_standalone:
            if ((tme_mode == SOC_SBX_QE_MODE_TME) ||
                (tme_mode == SOC_SBX_QE_MODE_TME_BYPASS)) {
                return TRUE;
            } else {
                return FALSE;
            } 
            break;
        case soc_feature_hybrid:
            return( ( (hybrid_mode == TRUE) &&
                      (tme_mode != TRUE /* SOC_SBX_QE_MODE_TME */) ) ? TRUE : FALSE);
            break;
        case soc_feature_node:
            return(TRUE);
            break;
        case soc_feature_node_hybrid:
            return( ((tme_mode == SOC_SBX_QE_MODE_HYBRID) ? TRUE : FALSE) );
            break;
        case soc_feature_egr_independent_fc:
            switch (SOC_SBX_CFG(unit)->uFabricConfig) {
                case SOC_SBX_SYSTEM_CFG_VPORT:
                case SOC_SBX_SYSTEM_CFG_VPORT_LEGACY:
                case SOC_SBX_SYSTEM_CFG_VPORT_MIX:
                    return((SOC_SBX_CFG(unit)->bEgressFifoIndependentFlowControl == FALSE) ? FALSE : TRUE);
                    break;

                case SOC_SBX_SYSTEM_CFG_DMODE:
                default:
                    return(FALSE);
                    break;
            }
            break;
        case soc_feature_egr_multicast_independent_fc:
            return((SOC_SBX_CFG(unit)->bEgressMulticastFifoIndependentFlowControl == FALSE) ? FALSE : TRUE);
            break;
        case soc_feature_discard_ability_color_black:
            return (TRUE);
            break;
        case soc_feature_distribution_ability:
            /* ESET need to be awared for FIC traffic for node type */
            return ( ((tme_mode == SOC_SBX_QE_MODE_TME) ||
                      (tme_mode == SOC_SBX_QE_MODE_TME_BYPASS))
                      ? FALSE : TRUE );
            break;
        default:
            return FALSE;
    }
}

/*
 * BCM88230 C0
 */
int
soc_features_bcm88230_c0(int unit, soc_feature_t feature)
{
    uint16      dev_id;
    uint8       rev_id;
    uint32      tme_mode, hybrid_mode;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    /* hardcoded soc features */
    switch (feature) {
        /* case soc_feature_dcb_type27:*/
        case soc_feature_dcb_type19:
        case soc_feature_schmsg_alias:
        case soc_feature_table_dma:
        case soc_feature_tslam_dma:
	case soc_feature_stat_dma:
        case soc_feature_higig2:
        case soc_feature_mem_push_pop:
        case soc_feature_fifo_dma:
        case soc_feature_mc_group_ability:
        case soc_feature_packet_adj_len:
        case soc_feature_multi_sbus_cmds:
        case soc_feature_extended_cmic_error:
        case soc_feature_unimac: /* for lab bringup only */
        case soc_feature_phy_cl45:
        case soc_feature_mdio_enhanced:
        case soc_feature_sgmii_autoneg:
        case soc_feature_ingress_size_templates:
        case soc_feature_priority_flow_control:
        case soc_feature_source_port_priority_flow_control:
            return TRUE;
            break;
        default:
            break;
    }

    /* soc features depends on configuration */
    if (SOC_CONTROL(unit) == NULL) {
        return(FALSE);
    }
    if (SOC_SBX_CONTROL(unit) == NULL) {
        return(FALSE);
    }
    if (SOC_SBX_CFG(unit) == NULL) {
        return(FALSE);
    }

    tme_mode = soc_property_get(unit, spn_QE_TME_MODE, SOC_SBX_CFG(unit)->bTmeMode);
    hybrid_mode = soc_property_get(unit, spn_HYBRID_MODE, SOC_SBX_CFG(unit)->bHybridMode);

    switch (feature) {
        case soc_feature_discard_ability:
            return( ((SOC_SBX_CFG(unit)->bTmeMode == SOC_SBX_QE_MODE_TME) ||
                     (SOC_SBX_CFG(unit)->bTmeMode == SOC_SBX_QE_MODE_TME_BYPASS) ||
                     (SOC_SBX_CFG(unit)->bTmeMode ==  SOC_SBX_QE_MODE_HYBRID)) ?
                    TRUE : FALSE );
            break;
        case soc_feature_egress_metering:
            /* Support is available for egress shaping */
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "rate egress_metering feature available\n")));
            return( TRUE );
            break;
        case soc_feature_cosq_gport_stat_ability:
            /* Support is available for gport statistics */
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "gport statistics feature available\n")));
            return( TRUE );
            break;
        case soc_feature_standalone:
            if ((tme_mode == SOC_SBX_QE_MODE_TME) ||
                (tme_mode == SOC_SBX_QE_MODE_TME_BYPASS)) {
                return TRUE;
            } else {
                return FALSE;
            } 
            break;
        case soc_feature_hybrid:
            return( ( (hybrid_mode == TRUE) &&
                      (tme_mode != TRUE /* SOC_SBX_QE_MODE_TME */) ) ? TRUE : FALSE);
            break;
        case soc_feature_node:
            return(TRUE);
            break;
        case soc_feature_node_hybrid:
            return( ((tme_mode == SOC_SBX_QE_MODE_HYBRID) ? TRUE : FALSE) );
            break;
        case soc_feature_egr_independent_fc:
            switch (SOC_SBX_CFG(unit)->uFabricConfig) {
                case SOC_SBX_SYSTEM_CFG_VPORT:
                case SOC_SBX_SYSTEM_CFG_VPORT_LEGACY:
                case SOC_SBX_SYSTEM_CFG_VPORT_MIX:
                    return((SOC_SBX_CFG(unit)->bEgressFifoIndependentFlowControl == TRUE) ? TRUE : FALSE);
                    break;

                case SOC_SBX_SYSTEM_CFG_DMODE:
                default:
                    return(FALSE);
                    break;
            }
            break;
        case soc_feature_egr_multicast_independent_fc:
            return((SOC_SBX_CFG(unit)->bEgressMulticastFifoIndependentFlowControl == FALSE) ? FALSE : TRUE);
            break;
        case soc_feature_discard_ability_color_black:
            return (TRUE);
            break;
        case soc_feature_distribution_ability:
            /* ESET need to be awared for FIC traffic for node type */
            return ( ((tme_mode == SOC_SBX_QE_MODE_TME) ||
                      (tme_mode == SOC_SBX_QE_MODE_TME_BYPASS))
                      ? FALSE : TRUE );
            break;
        default:
            return FALSE;
    }
}
#endif  /* BCM_88230 */

#ifdef  BCM_53262
int
soc_features_bcm53262_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 53262_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    case soc_feature_mac_learn_limit:
    /* soc_feature_dodeca_serdes: for fiber with SerDes + PHY_PassThrough */
    case soc_feature_dodeca_serdes: 
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_field:
    case soc_feature_arl_mode_control:
    case soc_feature_tag_enforcement:
    case soc_feature_vlan_translation:
    case soc_feature_igmp_mld_support:
    case soc_feature_robo_ge_serdes_mac_autosync:
    case soc_feature_sgmii_autoneg:        
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_53262 */

#ifdef  BCM_53115
int
soc_features_bcm53115_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 53115_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_field:
    case soc_feature_field_tcam_parity_check:
    case soc_feature_arl_mode_control:
    case soc_feature_hw_dos_prev:
    case soc_feature_tag_enforcement:
    case soc_feature_igmp_mld_support:
    case soc_feature_vlan_translation:
    case soc_feature_802_3as:
    case soc_feature_color_prio_map:
    case soc_feature_eav_support:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_53115 */

#ifdef  BCM_53118
int
soc_features_bcm53118_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 53118_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_arl_mode_control:
    case soc_feature_hw_dos_prev:
    case soc_feature_tag_enforcement:
    case soc_feature_igmp_mld_support:
    case soc_feature_802_3as:
    case soc_feature_eav_support:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_53118 */

#ifdef  BCM_53280
int
soc_features_bcm53280_a0(int unit, soc_feature_t feature)
{
   switch (feature) {
    case soc_feature_l2_hashed:
    case soc_feature_stg:
    case soc_feature_vlan_translation:
    case soc_feature_vlan_action:
    case soc_feature_dscp:
    case soc_feature_auth:
    case soc_feature_subport:
    case soc_feature_l2_pending:
    case soc_feature_mac_learn_limit:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_igmp_mld_support:
    case soc_feature_field:        
    case soc_feature_field_multi_stage:
    case soc_feature_field_tcam_hw_move:
    case soc_feature_hw_dos_prev:
    case soc_feature_hw_dos_report:
    case soc_feature_sgmii_autoneg:
    case soc_feature_field_tcam_parity_check:        
    case soc_feature_vlan_vp:
        return TRUE;
    default:
        return FALSE;
    }
}
int
soc_features_bcm53280_b0(int unit, soc_feature_t feature)
{
    char *s;
   switch (feature) {
    case soc_feature_l2_hashed:    
    case soc_feature_stg:
    case soc_feature_vlan_translation:
    case soc_feature_vlan_action:
    case soc_feature_dscp:
    case soc_feature_auth:
    case soc_feature_subport:
    case soc_feature_l2_pending:
    case soc_feature_mac_learn_limit:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_igmp_mld_support:
    case soc_feature_field:        
    case soc_feature_field_multi_stage:
    case soc_feature_hw_dos_prev:
    case soc_feature_hw_dos_report:
    case soc_feature_sgmii_autoneg:
    case soc_feature_vlan_vp:
        return TRUE;
    case soc_feature_field_tcam_hw_move:
    case soc_feature_field_tcam_parity_check:        
        s = soc_property_get_str(unit, "board_name");
        if( (s != NULL) && (sal_strcmp(s, "bcm53280_fpga") == 0))
            return FALSE;
        else
            return TRUE;
    default:
        return FALSE;
    }

}
#endif  /* BCM_53280 */

#ifdef  BCM_53101
int
soc_features_bcm53101_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 53101_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_arl_mode_control:
    case soc_feature_hw_dos_prev:
    case soc_feature_tag_enforcement:
    case soc_feature_igmp_mld_support:
    case soc_feature_802_3as:
    case soc_feature_eav_support:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_53101 */

#if defined(BCM_53125) || defined(BCM_POLAR_SUPPORT)
int
soc_features_bcm53125_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 53125_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_field:
    case soc_feature_field_tcam_parity_check:      
    case soc_feature_arl_mode_control:
    case soc_feature_hw_dos_prev:
    case soc_feature_tag_enforcement:
    case soc_feature_igmp_mld_support:
    case soc_feature_vlan_translation:
    case soc_feature_802_3as:
    case soc_feature_color_prio_map:
    case soc_feature_eav_support:
    case soc_feature_eee:
    case soc_feature_int_cpu_arbiter:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_53125 || BCM_POLAR_SUPPORT */

#ifdef  BCM_POLAR_SUPPORT
int
soc_features_bcm89500_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 53125_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_eee:
        return FALSE;
    case soc_feature_field_tcam_parity_check:        
        return TRUE;
    default:
        return soc_features_bcm53125_a0(unit, feature);        
    }
}
#endif  /* BCM_POLAR_SUPPORT */


#ifdef  BCM_53128
int
soc_features_bcm53128_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 53128_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_field: /* For auto voip feature */
    case soc_feature_arl_mode_control:
    case soc_feature_hw_dos_prev:
    case soc_feature_tag_enforcement:
    case soc_feature_igmp_mld_support:
    case soc_feature_802_3as:
    case soc_feature_eav_support:
    case soc_feature_eee:
    case soc_feature_int_cpu_arbiter:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_53128 */

#ifdef BCM_NORTHSTAR_SUPPORT
int
soc_features_bcm53010_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 53010_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_field:
    case soc_feature_field_tcam_parity_check:
    case soc_feature_arl_mode_control:
    case soc_feature_hw_dos_prev:
    case soc_feature_tag_enforcement:
    case soc_feature_igmp_mld_support:
    case soc_feature_vlan_translation:
    case soc_feature_802_3as:
    case soc_feature_color_prio_map:
    case soc_feature_eav_support:
    case soc_feature_eee:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_NORTHSTAR_SUPPORT */

#ifdef BCM_NORTHSTARPLUS_SUPPORT
int
soc_features_bcm53020_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 53020_a0"));

    /* Wiil be added later */
    switch (feature) {
    case soc_feature_stg:
    case soc_feature_l2_hashed:
    case soc_feature_l2_lookup_cmd:
    case soc_feature_l2_lookup_retry:
    case soc_feature_dscp:
    case soc_feature_auth:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_field:
    case soc_feature_field_tcam_parity_check:
    case soc_feature_arl_mode_control:
    case soc_feature_hw_dos_prev:
    case soc_feature_tag_enforcement:
    case soc_feature_igmp_mld_support:
    case soc_feature_vlan_translation:
    case soc_feature_802_3as:
    case soc_feature_color_prio_map:
    case soc_feature_eav_support:
    case soc_feature_eee:
    case soc_feature_mac_learn_limit:
    case soc_feature_system_mac_learn_limit:
        return TRUE;
    default:
        return FALSE;
    }
}
#endif  /* BCM_NORTHSTARPLUS_SUPPORT */


#ifdef BCM_88640
int
soc_features_bcm88640_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88640_a0"));

    /* Wiil be added later */
    switch (feature) {
    default:
        return FALSE;
    }

    return TRUE;
}
#endif

#ifdef BCM_53600
int
soc_features_bcm53600_a0(int unit, soc_feature_t feature)
{
  char *s;
   switch (feature) {
    case soc_feature_l2_hashed:
    case soc_feature_stg:
    case soc_feature_vlan_translation:
    case soc_feature_vlan_action:
    case soc_feature_dscp:
    case soc_feature_auth:
    case soc_feature_subport:
    case soc_feature_l2_pending:
    case soc_feature_mac_learn_limit:
    /* soc_feature_robo_sw_override: Robo port MAC state maintained by CPU  */
    case soc_feature_robo_sw_override: 
#ifdef INCLUDE_MSTP
    case soc_feature_mstp:
#endif
    case soc_feature_igmp_mld_support:
    case soc_feature_field:        
    case soc_feature_field_multi_stage:
    case soc_feature_hw_dos_prev:
    case soc_feature_hw_dos_report:
    case soc_feature_sgmii_autoneg:
    case soc_feature_arl_mode_control:
    case soc_feature_vlan_vp:
        return TRUE;
    case soc_feature_field_tcam_hw_move:
    case soc_feature_field_tcam_parity_check:        
        s = soc_property_get_str(unit, "board_name");
        if( (s != NULL) && (sal_strcmp(s, "bcm53280_fpga") == 0))
            return FALSE;
        else
            return TRUE;
    default:
        return FALSE;
    }
}

#endif /* BCM_53600 */

#ifdef  BCM_56440
/*
 * BCM56440 A0
 */
int
soc_features_bcm56440_a0(int unit, soc_feature_t feature)
{
    int     ddr3_disable, l3;
    uint16  dev_id;
    uint8   rev_id;
    int     dynamic_update;
    uint8   vlan_xlate;
    int     mpls, mim;
    int     spri_vector;
    int     shaper_update;
    int     toggled_read;
    int     ipmc_mode;
    int     ipmc_reduced_mode = 0;
    int     mmu_reduced_int_buffer;

    SOC_FEATURE_DEBUG_PRINT((" 56440_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);

    ddr3_disable = ((dev_id == BCM56445_DEVICE_ID) || 
                    (dev_id == BCM56446_DEVICE_ID) ||
                    (dev_id == BCM56447_DEVICE_ID) ||
                    (dev_id == BCM56448_DEVICE_ID) ||
                    (dev_id == BCM56241_DEVICE_ID) ||
                    (dev_id == BCM55441_DEVICE_ID)); 

    dynamic_update = soc_property_get(unit, spn_MMU_DYNAMIC_SCHED_UPDATE, 0);
    dynamic_update = ((dev_id == BCM56445_DEVICE_ID) &&
                      (rev_id == BCM56445_A0_REV_ID) &&
                      dynamic_update);

    vlan_xlate     = ((dev_id == BCM56440_DEVICE_ID) && 
                      (rev_id == BCM56440_B0_REV_ID));
    mpls = ((dev_id != BCM55441_DEVICE_ID) &&
            (dev_id != BCM55440_DEVICE_ID));
    mim = (dev_id != BCM55441_DEVICE_ID); 
    l3 = (dev_id == BCM55440_DEVICE_ID);

    spri_vector = soc_property_get(unit, spn_MMU_STRICT_PRI_VECTOR_MODE, 1);
    spri_vector = ((rev_id != BCM56440_A0_REV_ID) && spri_vector);
    shaper_update = (rev_id == BCM56440_A0_REV_ID);
    toggled_read = (rev_id == BCM56440_A0_REV_ID);

    if (rev_id == BCM56440_B0_REV_ID) {
        ipmc_mode = soc_property_get(unit, spn_IPMC_INDEPENDENT_MODE, 0);
        if (ipmc_mode == 1 &&
            soc_property_get(unit, spn_IPMC_REDUCED_TABLE_SIZE, 0)) {
            ipmc_reduced_mode = 1;
        }
    }

    mmu_reduced_int_buffer = ((dev_id == BCM56240_DEVICE_ID) ||
                              (dev_id == BCM56241_DEVICE_ID) ||
                              (dev_id == BCM56242_DEVICE_ID) ||
                              (dev_id == BCM56243_DEVICE_ID) ||
                              (dev_id == BCM56245_DEVICE_ID) ||
                              (dev_id == BCM56246_DEVICE_ID));

    switch (feature) {
    case soc_feature_dcb_type21:
    case soc_feature_logical_port_num:
    case soc_feature_wlan:
    case soc_feature_gport_service_counters:
    case soc_feature_priority_flow_control:
#ifndef _KATANA_DEBUG
    /* Temporarily Disabled until implementation complete */
    case soc_feature_field_vfp_flex_counter:
    case soc_feature_ctr_xaui_activity:
#endif
    case soc_feature_asf:
    case soc_feature_qcn:
    case soc_feature_egr_mirror_true:
    case soc_feature_trill:
    case soc_feature_two_ingress_pipes:
    case soc_feature_niv:
    case soc_feature_ip_source_bind:
    case soc_feature_vp_group_ingress_vlan_membership:
    case soc_feature_vp_group_egress_vlan_membership:
    case soc_feature_ets:
    case soc_feature_field_ingress_two_slice_types:
    case soc_feature_mmu_config_property:
    case soc_feature_rtag1_6_max_portcnt_less_than_rtag7:
    case soc_feature_hg_dlb:
    case soc_feature_bucket_update_freeze:
    case soc_feature_hg_trunk_16_members:
    case soc_feature_field_meter_pools4:
    case soc_feature_fp_meter_ser_verify:
    case soc_feature_special_egr_ip_tunnel_ser:
        return FALSE;
    case soc_feature_dcb_type24:
    case soc_feature_multi_sbus_cmds:
    case soc_feature_cmicm:
    case soc_feature_mcs:
    case soc_feature_uc:
    case soc_feature_bfd:
    case soc_feature_oam:
    case soc_feature_ptp:
    case soc_feature_mpls_entropy:
    case soc_feature_bhh:
    case soc_feature_mem_cache:
    case soc_feature_xy_tcam:
        return TRUE;
    case soc_feature_field_slices12:
    case soc_feature_field_meter_pools12:
        if ((dev_id == BCM55441_DEVICE_ID) ||
            (dev_id == BCM56240_DEVICE_ID) ||
            (dev_id == BCM56241_DEVICE_ID) ||
            (dev_id == BCM56242_DEVICE_ID) ||
            (dev_id == BCM56243_DEVICE_ID) ||
            (dev_id == BCM56245_DEVICE_ID) ||
            (dev_id == BCM56246_DEVICE_ID)) {
            return FALSE;
        } else {
            return TRUE;
        } 
    case soc_feature_field_virtual_slice_group:
    case soc_feature_field_intraslice_double_wide:
    case soc_feature_field_egress_flexible_v6_key:
    case soc_feature_field_multi_stage:
    case soc_feature_field_ingress_global_meter_pools:
    case soc_feature_field_egress_global_counters:
    case soc_feature_field_ing_egr_separate_packet_byte_counters:
    case soc_feature_field_ingress_ipbm:
    case soc_feature_field_egress_metering:
    case soc_feature_global_meter:
    case soc_feature_mac_learn_limit:    
    case soc_feature_advanced_flex_counter:
    case soc_feature_time_support:
    case soc_feature_vlan_double_tag_range_compress:
    case soc_feature_field_packet_based_metering:
    case soc_feature_vlan_protocol_pkt_ctrl:
    case soc_feature_extended_queueing:
    case soc_feature_rx_timestamp_upper:
    case soc_feature_field_oam_actions:
    case soc_feature_gmii_clkout:
    case soc_feature_vlan_xlate_dtag_range:
    case soc_feature_post:
    case soc_feature_timesync_support:
        return TRUE;
    case soc_feature_ces:
        /* For now, set this feature to False. During SOC reset, this may be
         * set to TRUE as per 1588 / CES  Enable Bond Option..
        */
        return FALSE;
    case soc_feature_field_slices8:
    case soc_feature_field_meter_pools8:
        if ((dev_id == BCM55441_DEVICE_ID) ||
            (dev_id == BCM56240_DEVICE_ID) ||
            (dev_id == BCM56241_DEVICE_ID) ||
            (dev_id == BCM56242_DEVICE_ID) ||
            (dev_id == BCM56243_DEVICE_ID) ||
            (dev_id == BCM56245_DEVICE_ID) ||
            (dev_id == BCM56246_DEVICE_ID)) {
            return TRUE;
        } else {
            return FALSE;
        } 
    case soc_feature_ddr3:
        return !ddr3_disable;
    case soc_feature_dynamic_sched_update:
        return dynamic_update;
    case soc_feature_vlan_xlate:
        return vlan_xlate;
    case soc_feature_mpls:
        return mpls;
    case soc_feature_mim:
        return mim;
    case soc_feature_vector_based_spri:
        return spri_vector;
    case soc_feature_cosq_gport_stat_ability:
        /* variants that support <= 1k queues */
        return ddr3_disable;
    case soc_feature_dynamic_shaper_update:
        return shaper_update;
    case soc_feature_counter_toggled_read:
        return toggled_read;
    case soc_feature_ipmc_reduced_table_size:
        return ipmc_reduced_mode;
    case soc_feature_mmu_reduced_internal_buffer:
        return mmu_reduced_int_buffer;
    case soc_feature_urpf:
    case soc_feature_lpm_tcam:
    case soc_feature_ip_mcast:
    case soc_feature_ip_mcast_repl:
    case soc_feature_l3:
    case soc_feature_l3_ingress_interface:
    case soc_feature_l3_ip6:
    case soc_feature_l3_lookup_cmd:
    case soc_feature_l3_sgv:
    case soc_feature_subport:
        return !l3;   
    default:
        return soc_features_bcm56840_a0(unit, feature);
    }
}

/*
 * BCM56440 B0
 */
int
soc_features_bcm56440_b0(int unit, soc_feature_t feature)
{
    return soc_features_bcm56440_a0(unit, feature);
}
#endif  /* BCM_56440 */

#ifdef BCM_88650_A0
int
soc_features_bcm88650_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88650_a0"));

    switch (feature) {
    case soc_feature_sbusdma:
    case soc_feature_cmicm:
    case soc_feature_mcs:
    case soc_feature_uc:
    case soc_feature_new_sbus_format:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    case soc_feature_logical_port_num:
    case soc_feature_short_cmic_error:
    case soc_feature_controlled_counters:
    case soc_feature_dcb_type28: 
    case soc_feature_cos_rx_dma: 
    case soc_feature_schan_hw_timeout:
    case soc_feature_ddr3:
    case soc_feature_tunnel_gre:
    case soc_feature_generic_counters:
    case soc_feature_led_proc:
    case soc_feature_led_data_offset_a0:
    case soc_feature_time_support:
    case soc_feature_ptp:
    case soc_feature_linkscan_lock_per_unit:
#ifdef INCLUDE_RCPU
    case soc_feature_rcpu_1:
#endif /* INCLUDE_RCPU */
        return TRUE;
    default:
        return FALSE;
    }

   return TRUE;
}


int
soc_features_bcm88650_b0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88650_b0"));

    switch (feature) {
    case soc_feature_sbusdma:
    case soc_feature_cmicm:
    case soc_feature_mcs:
    case soc_feature_uc:
    case soc_feature_new_sbus_format:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    case soc_feature_logical_port_num:
    case soc_feature_short_cmic_error:
    case soc_feature_controlled_counters:
    case soc_feature_dcb_type28: 
    case soc_feature_cos_rx_dma: 
    case soc_feature_schan_hw_timeout:
    case soc_feature_ddr3:
    case soc_feature_tunnel_gre:
    case soc_feature_generic_counters:
    case soc_feature_led_proc:
    case soc_feature_led_data_offset_a0:
    case soc_feature_time_support:
    case soc_feature_ptp:
    case soc_feature_timesync_support:
    case soc_feature_linkscan_lock_per_unit:
#ifdef INCLUDE_RCPU
    case soc_feature_rcpu_1:
#endif /* INCLUDE_RCPU */
       return TRUE;
    default:
        return FALSE;
    }

   return TRUE;
}


#endif /* BCM_88650_A0 */

#ifdef BCM_88660_A0
int
soc_features_bcm88660_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88660_a0"));

    switch (feature) {
    case soc_feature_sbusdma:
    case soc_feature_cmicm:
    case soc_feature_mcs:
    case soc_feature_uc:
    case soc_feature_new_sbus_format:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    case soc_feature_logical_port_num:
    case soc_feature_short_cmic_error:
    case soc_feature_controlled_counters:
    case soc_feature_dcb_type28: 
    case soc_feature_cos_rx_dma: 
    case soc_feature_schan_hw_timeout:
    case soc_feature_ddr3:
    case soc_feature_tunnel_gre:
    case soc_feature_generic_counters:
    case soc_feature_led_proc:
    case soc_feature_led_data_offset_a0:
    case soc_feature_time_support:
    case soc_feature_ptp:
    case soc_feature_timesync_support:
    case soc_feature_linkscan_lock_per_unit:
#ifdef INCLUDE_RCPU
    case soc_feature_rcpu_1:
#endif /* INCLUDE_RCPU */
        return TRUE;
    default:
        return FALSE;
    }

   return TRUE;
}

#endif /* BCM_88660_A0 */

#ifdef BCM_88202_A0
int
soc_features_bcm88202_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88202_a0"));

    switch (feature) {
    case soc_feature_sbusdma:
    case soc_feature_cmicm:
    case soc_feature_mcs:
    case soc_feature_uc:
    case soc_feature_new_sbus_format:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    case soc_feature_logical_port_num:
    case soc_feature_short_cmic_error:
    case soc_feature_controlled_counters:
    case soc_feature_dcb_type28: 
    case soc_feature_cos_rx_dma: 
    case soc_feature_schan_hw_timeout:
    case soc_feature_tunnel_gre:
    case soc_feature_generic_counters:
    case soc_feature_time_support:
    case soc_feature_linkscan_lock_per_unit:
#ifdef INCLUDE_RCPU
    case soc_feature_rcpu_1:
#endif /* INCLUDE_RCPU */
        return TRUE;
    default:
        return FALSE;
    }

   return TRUE;
}
#endif /* BCM_88202_A0 */

#ifdef BCM_2801PM_A0
int
soc_features_bcm2801pm_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88650_a0"));

    switch (feature) {
    case soc_feature_sbusdma:
    case soc_feature_cmicm:
    case soc_feature_new_sbus_format:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    case soc_feature_logical_port_num:
    case soc_feature_short_cmic_error:
    case soc_feature_schan_hw_timeout:
    case soc_feature_time_support:
    case soc_feature_linkscan_lock_per_unit:
        return TRUE;
    default:
        return FALSE;
    }

   return TRUE;
}
#endif /* BCM_2801PM_A0 */

#ifdef BCM_88670
int
soc_features_bcm88670_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88670_a0"));

    switch (feature) {
    /* General */
    case soc_feature_iproc:
    case soc_feature_cmicm:
    /* DMA */
    case soc_feature_sbusdma:
    case soc_feature_dcb_type28: 
    case soc_feature_cos_rx_dma:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    /* SCHAN */
    case soc_feature_new_sbus_format:
    case soc_feature_sbus_format_v4:
    case soc_feature_schan_hw_timeout:
    /* Interrupts */
    case soc_feature_short_cmic_error:
    case soc_feature_portmod:
    /* Fabric */
    case soc_feature_fabric_cell_pcp:
    /* Ports */
    case soc_feature_logical_port_num:
     
        return TRUE;
    default:
        return FALSE;
    }

    return TRUE;
}
#endif



#ifdef BCM_88750
int
soc_features_bcm88750_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88750_a0"));

    switch (feature) {
    case soc_feature_controlled_counters:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    case soc_feature_short_cmic_error:
    case soc_feature_linkscan_pause_timeout:
    case soc_feature_linkscan_lock_per_unit:
    case soc_feature_easy_reload_wb_compat:
        return TRUE;
    default:
        return FALSE;
    }

    return TRUE;
}



int
soc_features_bcm88750_b0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88750_b0"));

    switch (feature) {
    case soc_feature_controlled_counters:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    case soc_feature_short_cmic_error:
    case soc_feature_linkscan_pause_timeout:
    case soc_feature_linkscan_lock_per_unit:
    case soc_feature_easy_reload_wb_compat:
        return TRUE;
    default:
        return FALSE;
    }

    return TRUE;
}

#ifdef BCM_88754_A0
int
soc_features_bcm88754_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88750_b0"));

    switch (feature) {
    case soc_feature_controlled_counters:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    case soc_feature_short_cmic_error:
    case soc_feature_schan_err_check:
    case soc_feature_linkscan_pause_timeout:
    case soc_feature_linkscan_lock_per_unit:
    case soc_feature_easy_reload_wb_compat:
        return TRUE;
    default:
        return FALSE;
    }

    return TRUE;
}
#endif /*BCM_88754_A0*/

#endif /* BCM_88750 */

#ifdef BCM_88950
int
soc_features_bcm88950_a0(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88950_a0"));

    /* Will be added later */
    switch (feature) {
    case soc_feature_controlled_counters:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    case soc_feature_short_cmic_error:
    case soc_feature_linkscan_pause_timeout:
	case soc_feature_linkscan_lock_per_unit:
	case soc_feature_new_sbus_format:
    case soc_feature_sbus_format_v4:
	case soc_feature_schan_hw_timeout:
	case soc_feature_iproc:
	case soc_feature_cmicm:
    case soc_feature_sbusdma:
    case soc_feature_easy_reload_wb_compat:
    case soc_feature_portmod:
	case soc_feature_fabric_cell_pcp:
	case soc_feature_fe_mc_id_range:
	case soc_feature_fe_mc_priority_mode_enable:
        return TRUE;
    default:
        return FALSE;
    }

    return TRUE;
}

#endif

#ifdef BCM_88850_P3
int
soc_features_bcm88850_p3(int unit, soc_feature_t feature)
{
    COMPILER_REFERENCE(unit);
    SOC_FEATURE_DEBUG_PRINT((" 88850_p3"));

    switch (feature) {
    case soc_feature_sbusdma:
    case soc_feature_cmicm:
    case soc_feature_mcs:
    case soc_feature_uc:
    case soc_feature_new_sbus_format:
    case soc_feature_tslam_dma:
    case soc_feature_table_dma:
    case soc_feature_logical_port_num:
    case soc_feature_short_cmic_error:
    case soc_feature_controlled_counters:
    case soc_feature_dcb_type28: 
    case soc_feature_cos_rx_dma: 
    case soc_feature_schan_err_check:
    case soc_feature_schan_hw_timeout:
    case soc_feature_ddr3:
    case soc_feature_tunnel_gre:
    case soc_feature_generic_counters:
    case soc_feature_led_proc:
    case soc_feature_led_data_offset_a0:
    case soc_feature_time_support:
    case soc_feature_ptp:
    case soc_feature_timesync_support:
    case soc_feature_linkscan_lock_per_unit:
#ifdef INCLUDE_RCPU
    case soc_feature_rcpu_1:
#endif /* INCLUDE_RCPU */
        return TRUE;
    default:
        return FALSE;
    }

   return TRUE;
}
#endif /* BCM_88850_P3 */


#ifdef  BCM_56450
/*
 * BCM56450 A0
 */
int
soc_features_bcm56450_a0(int unit, soc_feature_t feature)
{
    uint16  dev_id;
    uint8   rev_id;
    uint8   vlan_xlate = 0;
    int     ddr3_disable = 0;
    int     pack_mode = 0;
    int     ehost_mode = 0;
    int     ext_mem_port_count = 0;


    SOC_FEATURE_DEBUG_PRINT((" 56450_a0"));
    soc_cm_get_id(unit, &dev_id, &rev_id);
    ddr3_disable = (dev_id == BCM56456_DEVICE_ID) || 
                   (dev_id == BCM56457_DEVICE_ID) ||
                   (dev_id == BCM56458_DEVICE_ID);
    /* If  external memory available for give KT2 variant */
    if (!ddr3_disable) {
        /* Get number of external memorory port count defined by 
           by config variable pbmp_ext_mem */
        SOC_PBMP_COUNT(PBMP_EXT_MEM (unit), ext_mem_port_count);
    }
    vlan_xlate   = (dev_id == BCM56450_DEVICE_ID);

    
    /* if external memory ports are available */
    if (ext_mem_port_count) {
        pack_mode  = soc_property_get(unit, spn_MMU_MULTI_PACKETS_PER_CELL, 0);
    } 
    ehost_mode   = !(soc_cm_get_bus_type(unit) & SOC_AXI_DEV_TYPE);

    switch (feature) {
    case soc_feature_ces:
    case soc_feature_unimac:
    case soc_feature_dcb_type24:
    case soc_feature_gmii_clkout:
    case soc_feature_field_slices10:
    case soc_feature_field_slices12:
    case soc_feature_field_meter_pools12:
    case soc_feature_mcs:
    case soc_feature_counter_toggled_read:
    case soc_feature_vpd_profile:
        return FALSE;
    case soc_feature_uc:
        return ehost_mode;
    case soc_feature_oam:
    case soc_feature_iproc:
    case soc_feature_new_sbus_format:
    case soc_feature_sbusdma:
    case soc_feature_dcb_type29:
    case soc_feature_ep_redirect:
    case soc_feature_linkphy_coe:
    case soc_feature_subtag_coe:
    case soc_feature_priority_flow_control:/*Should be set true for katana too*/
    case soc_feature_vector_based_spri:
    case soc_feature_new_sbus_old_resp:
    case soc_feature_field_egress_tocpu:
    case soc_feature_global_meter:
    case soc_feature_failover:
    case soc_feature_mpls_software_failover:
    case soc_feature_mpls_failover:
    case soc_feature_mpls_enhanced:
    case soc_feature_mpls_entropy:
    case soc_feature_field_ingress_cosq_override:
    case soc_feature_service_queuing:
    case soc_feature_multiple_split_horizon_group:
    case soc_feature_cmic_reserved_queues:
    case soc_feature_mem_cache:
    case soc_feature_egr_mirror_true:
    case soc_feature_lltag:
    case soc_feature_mem_parity_eccmask:
    case soc_feature_timestamp_counter:  
    case soc_feature_time_support:       
    case soc_feature_ptp:
    case soc_feature_bhh:
    case soc_feature_xy_tcam:
    case soc_feature_inner_tpid_enable:
    case soc_feature_l3_defip_advanced_lookup:
    case soc_feature_l3_defip_map:
        return TRUE;
    case soc_feature_ddr3:
        return !ddr3_disable;
    case soc_feature_vlan_xlate:
        return vlan_xlate;
    case soc_feature_mmu_packing:
        return pack_mode;
    default:
        return soc_features_bcm56440_a0(unit, feature);
    }
}

/*
 * BCM56450 B0
 */
int
soc_features_bcm56450_b0(int unit, soc_feature_t feature)
{
    return soc_features_bcm56450_a0(unit, feature);
}

#endif  /* BCM_56450 */

/*
 * Function:    soc_feature_init
 * Purpose:     initialize features into the SOC_CONTROL cache
 * Parameters:  unit    - the device
 * Returns:     void
 */
void
soc_feature_init(int unit)
{
    soc_feature_t       f;

    assert(COUNTOF(soc_feature_name) == (soc_feature_count + 1));

    sal_memset(SOC_CONTROL(unit)->features, 0,
               sizeof(SOC_CONTROL(unit)->features));
    for (f = 0; f < soc_feature_count; f++) {
        SOC_FEATURE_DEBUG_PRINT(("%s : ", soc_feature_name[f]));
        if (SOC_DRIVER(unit)->feature(unit, f)) {
            SOC_FEATURE_DEBUG_PRINT((" TRUE\n"));
            SOC_FEATURE_SET(unit, f);
        } else {
            SOC_FEATURE_DEBUG_PRINT((" FALSE\n"));
        }
    }
}
