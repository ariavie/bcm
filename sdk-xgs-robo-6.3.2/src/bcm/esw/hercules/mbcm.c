/*
 * $Id: mbcm.c 1.39 Broadcom SDK $
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
 * File:        mbcm.c
 */

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/hercules.h>

mbcm_functions_t mbcm_hercules_driver = {
    /* L2 functions */
    bcm_hercules_l2_init,
    bcm_hercules_l2_term,
    bcm_hercules_l2_addr_get,
    bcm_hercules_l2_addr_add,
    bcm_hercules_l2_addr_delete,
    bcm_hercules_l2_conflict_get, 

    /* Port table related functions */
    bcm_hercules_port_cfg_init, 
    bcm_hercules_port_cfg_get,
    bcm_hercules_port_cfg_set,

    /* VLAN functions */
    bcm_hercules_vlan_init,
    bcm_hercules_vlan_reload,
    bcm_hercules_vlan_create,
    bcm_hercules_vlan_destroy,
    bcm_hercules_vlan_port_add,
    bcm_hercules_vlan_port_remove,
    bcm_hercules_vlan_port_get,
    bcm_hercules_vlan_stg_get,
    bcm_hercules_vlan_stg_set,

    /* Hercules trunking functions */
    bcm_hercules_trunk_modify,
    bcm_hercules_trunk_get,
    bcm_hercules_trunk_destroy,
    bcm_hercules_trunk_mcast_join,

    /* Spanning Tree Group functions */
    bcm_hercules_stg_stp_init,
    bcm_hercules_stg_stp_get,
    bcm_hercules_stg_stp_set,

    /* Multicasting functions */
    bcm_hercules_mcast_addr_add,
    bcm_hercules_mcast_addr_remove,
    bcm_hercules_mcast_port_get,
    bcm_hercules_mcast_init,
    _bcm_hercules_mcast_detach,
    bcm_hercules_mcast_addr_add_w_l2mcindex,
    bcm_hercules_mcast_addr_remove_w_l2mcindex,
    bcm_hercules_mcast_port_add,
    bcm_hercules_mcast_port_remove,

    /* COSQ functions */
    bcm_hercules_cosq_init,
    bcm_hercules_cosq_detach,
    bcm_hercules_cosq_config_set,
    bcm_hercules_cosq_config_get,
    bcm_hercules_cosq_mapping_set,
    bcm_hercules_cosq_mapping_get,
    bcm_hercules_cosq_port_sched_set,
    bcm_hercules_cosq_port_sched_get,
    bcm_hercules_cosq_sched_weight_max_get,
    bcm_hercules_cosq_port_bandwidth_set,
    bcm_hercules_cosq_port_bandwidth_get,
    bcm_hercules_cosq_discard_set,
    bcm_hercules_cosq_discard_get,
    bcm_hercules_cosq_discard_port_set,
    bcm_hercules_cosq_discard_port_get,
#ifdef BCM_WARM_BOOT_SUPPORT
    bcm_hercules_cosq_sync,
#endif /* BCM_WARM_BOOT_SUPPORT */
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
    bcm_hercules_cosq_sw_dump,
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

    /*  Port meter functions */
    bcm_hercules_port_rate_egress_set,
    bcm_hercules_port_rate_egress_get,

#ifdef INCLUDE_L3
    /* L3 functions */
    bcm_hercules_l3_tables_init,
    bcm_hercules_l3_tables_cleanup,
    bcm_hercules_l3_enable,
    bcm_hercules_l3_intf_get,
    bcm_hercules_l3_intf_get_by_vid,
    bcm_hercules_l3_intf_create,
    bcm_hercules_l3_intf_id_create,
    bcm_hercules_l3_intf_lookup,
    bcm_hercules_l3_intf_del,
    bcm_hercules_l3_intf_del_all,

    bcm_hercules_l3_get,
    bcm_hercules_l3_add,
    bcm_hercules_l3_del,
    bcm_hercules_l3_del_prefix,
    bcm_hercules_l3_del_intf,
    bcm_hercules_l3_del_all,
    bcm_hercules_l3_replace,
    bcm_hercules_l3_age,
    bcm_hercules_l3_traverse,

    bcm_hercules_l3_ip6_get,
    bcm_hercules_l3_ip6_add,
    bcm_hercules_l3_ip6_del,
    bcm_hercules_l3_ip6_del_prefix,
    bcm_hercules_l3_ip6_replace,
    bcm_hercules_l3_ip6_traverse,

    bcm_hercules_defip_cfg_get,
    bcm_hercules_defip_ecmp_get_all,
    bcm_hercules_defip_add,
    bcm_hercules_defip_del,
    bcm_hercules_defip_del_intf,
    bcm_hercules_defip_del_all,
    bcm_hercules_defip_age,
    bcm_hercules_defip_traverse,

    bcm_hercules_ip6_defip_cfg_get,
    bcm_hercules_ip6_defip_ecmp_get_all,
    bcm_hercules_ip6_defip_add,
    bcm_hercules_ip6_defip_del,
    bcm_hercules_ip6_defip_traverse,

    bcm_hercules_l3_conflict_get,
    bcm_hercules_l3_info,

    /* IPMC functions */
    bcm_hercules_ipmc_init,
    bcm_hercules_ipmc_detach,
    bcm_hercules_ipmc_enable,
    bcm_hercules_ipmc_src_port_check,
    bcm_hercules_ipmc_src_ip_search,
    bcm_hercules_ipmc_add,
    bcm_hercules_ipmc_delete,
    bcm_hercules_ipmc_delete_all,
    bcm_hercules_ipmc_lookup,
    bcm_hercules_ipmc_get,
    bcm_hercules_ipmc_put,
    bcm_hercules_ipmc_egress_port_get,
    bcm_hercules_ipmc_egress_port_set,    
    bcm_hercules_ipmc_repl_init,
    bcm_hercules_ipmc_repl_detach,
    bcm_hercules_ipmc_repl_get,
    bcm_hercules_ipmc_repl_add,
    bcm_hercules_ipmc_repl_delete,
    bcm_hercules_ipmc_repl_delete_all,
    bcm_hercules_ipmc_egress_intf_add,
    bcm_hercules_ipmc_egress_intf_delete,
    bcm_hercules_ipmc_age,
    bcm_hercules_ipmc_traverse,
#endif /* INCLUDE_L3 */
};
