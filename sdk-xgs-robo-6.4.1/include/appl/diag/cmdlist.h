/*
 * $Id: cmdlist.h,v 1.178 Broadcom SDK $
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
 * File:    cmdlist.h
 * Purpose: Extern declarations for command functions and
 *      their associated usage strings.
 */

#ifndef _DIAG_CMDLIST_H
#define _DIAG_CMDLIST_H

#include <appl/diag/parse.h>
#include <appl/diag/shell.h>

#if defined(MOUSSE) || defined(BMW) || defined(IDTRP334) || defined(GTO) || \
    defined(MBZ) || defined(IDT438) || defined(NSX) || defined(ROBO_4704) || \
    defined(METROCORE) || defined(KEYSTONE)
#ifdef VXWORKS
#include <config.h>      /* For INCLUDE_XXX */
#endif
#endif

#define DCL_CMD(_f,_u)  \
    extern cmd_result_t _f(int, args_t *); \
    extern char     _u[];

extern  void cmdlist_init(void);
extern  cmd_result_t subcommand_execute(int unit, args_t *args,
                                        cmd_t *cmd_list, int cmd_cnt);
extern cmd_result_t _bcm_diag_trunk_show(int unit, args_t *a);


/* Dynamic commands */
#ifndef BCM_SHELL_MAX_DYNAMIC_CMDS
#define BCM_SHELL_MAX_DYNAMIC_CMDS 10
#endif

extern int cmdlist_add(cmd_t* cmd);
extern int cmdlist_remove(cmd_t* cmd);


#if defined(INCLUDE_LIB_CINT)
DCL_CMD(cmd_cint, cmd_cint_usage)
#endif
#if defined(INCLUDE_APIMODE)
DCL_CMD(cmd_api, cmd_api_usage)
#endif

/* test.c */

DCL_CMD(test_print_list,test_list_usage)
DCL_CMD(test_select,    test_select_usage)
DCL_CMD(test_mode,  test_mode_usage)
DCL_CMD(test_parameters,test_parameters_usage)
DCL_CMD(test_run,   test_run_usage)
DCL_CMD(test_clear, test_clear_usage)

/* shell.c */

DCL_CMD(sh_rccache, sh_rccache_usage)
DCL_CMD(sh_help,    sh_help_usage)
DCL_CMD(sh_help_short,  sh_help_short_usage)
DCL_CMD(sh_exit,    sh_exit_usage)
DCL_CMD(sh_rcload,  sh_rcload_usage)
DCL_CMD(sh_set,     sh_set_usage)
DCL_CMD(sh_debug,   sh_debug_usage)
DCL_CMD(sh_log,     sh_log_usage)
DCL_CMD(sh_console, sh_console_usage)
DCL_CMD(sh_reboot,  sh_reboot_usage)
DCL_CMD(sh_version, sh_version_usage)
DCL_CMD(sh_attach,  sh_attach_usage)
DCL_CMD(sh_detach,  sh_detach_usage)
DCL_CMD(sh_echo,    sh_echo_usage)
DCL_CMD(sh_noecho,  sh_noecho_usage)
DCL_CMD(sh_pause,   sh_pause_usage)
DCL_CMD(sh_sleep,   sh_sleep_usage)
DCL_CMD(sh_delay,   sh_delay_usage)
DCL_CMD(sh_shell,   sh_shell_usage)
DCL_CMD(sh_ls,      sh_ls_usage)
DCL_CMD(sh_pwd,     sh_pwd_usage)
DCL_CMD(sh_probe,   sh_probe_usage)
DCL_CMD(sh_history, sh_history_usage)
DCL_CMD(sh_loop,    sh_loop_usage)
DCL_CMD(sh_for,     sh_for_usage)
DCL_CMD(sh_init,    sh_init_usage)
DCL_CMD(sh_deinit,    sh_deinit_usage)
DCL_CMD(sh_device_reset,    sh_device_reset_usage)
DCL_CMD(sh_reinit,  sh_reinit_usage)
DCL_CMD(sh_expr,    sh_expr_usage)
DCL_CMD(sh_if,      sh_if_usage)
DCL_CMD(sh_break,   sh_break_usage)
DCL_CMD(sh_case,    sh_case_usage)
DCL_CMD(sh_cd,      sh_cd_usage)
DCL_CMD(sh_copy,    sh_copy_usage)
DCL_CMD(sh_flashinit,   sh_flashinit_usage)
DCL_CMD(sh_flashsync,   sh_flashsync_usage)
DCL_CMD(sh_config,  sh_config_usage)
DCL_CMD(sh_more,    sh_more_usage)
DCL_CMD(sh_write,   sh_write_usage)
DCL_CMD(sh_remove,  sh_remove_usage)
DCL_CMD(sh_rename,  sh_rename_usage)
DCL_CMD(sh_mkdir,   sh_mkdir_usage)
DCL_CMD(sh_rmdir,   sh_rmdir_usage)
DCL_CMD(sh_date,    sh_date_usage)
DCL_CMD(sh_time,    sh_time_usage)
DCL_CMD(sh_bcmx,    sh_bcmx_usage)
DCL_CMD(sh_bg,      sh_bg_usage)
DCL_CMD(sh_jobs,    sh_jobs_usage)
DCL_CMD(sh_kill,    sh_kill_usage)
DCL_CMD(sh_warmboot,    sh_warmboot_usage)
DCL_CMD(cmd_sh_restest, cmd_sh_restest_usage)
/* edline.c */

DCL_CMD(edline,     edline_usage)

/* pcktwatch.c */

DCL_CMD(pw_command, pw_usage)

/* reg.c */

DCL_CMD(cmd_esw_reg_get,    cmd_esw_reg_get_usage)
DCL_CMD(cmd_esw_reg_set,    cmd_esw_reg_set_usage)
DCL_CMD(cmd_esw_reg_mod,    cmd_esw_reg_mod_usage)
DCL_CMD(cmd_esw_reg_list,   cmd_esw_reg_list_usage)
DCL_CMD(cmd_esw_reg_edit,   cmd_esw_reg_edit_usage)

DCL_CMD(cmd_robo_reg_get,   cmd_robo_reg_get_usage)
DCL_CMD(cmd_robo_reg_set,   cmd_robo_reg_set_usage)
DCL_CMD(cmd_robo_reg_mod, cmd_robo_reg_mod_usage)
DCL_CMD(cmd_robo_reg_list,  cmd_robo_reg_list_usage)
DCL_CMD(cmd_robo_reg_edit,  cmd_robo_reg_edit_usage)

DCL_CMD(reg_cmp,    regcmp_usage)

/* ledproc.c */

DCL_CMD(ledproc_cmd,    ledproc_usage)

#ifdef BCM_CMICM_SUPPORT

/* mcs.c */
DCL_CMD(mcsdump_cmd,      mcsdump_usage)
DCL_CMD(mcsload_cmd,      mcsload_usage)
DCL_CMD(mcsmsg_cmd,       mcsmsg_usage)
DCL_CMD(mcsstatus_cmd,    mcsstatus_usage)
DCL_CMD(mcstimestamp_cmd, mcstimestamp_usage)
/* cmicm.c */
DCL_CMD(dpll_cmd,       dpll_usage)
DCL_CMD(mspi_cmd,       mspi_usage)

/* pscan.c */
DCL_CMD(pscan_cmd,      pscan_usage)
#endif

/* iproc.c */
#ifdef BCM_IPROC_SUPPORT
DCL_CMD(iprocread_cmd,   iprocread_usage)
DCL_CMD(iprocwrite_cmd,  iprocwrite_usage)
#endif

/* ptp.c */
#ifdef INCLUDE_PTP
DCL_CMD(cmd_ptp,        cmd_ptp_usage)
DCL_CMD(cmd_tdpll,      cmd_tdpll_usage)
#ifdef PTP_KEYSTONE_STACK
DCL_CMD(cmd_topload,    cmd_topload_usage)
#endif
#endif

/* bs.c */
#ifdef BCM_CMICM_SUPPORT
DCL_CMD(cmd_broadsync, cmd_broadsync_usage)
#endif

/* mem.c */
DCL_CMD(mem_mmudebug,   mmudebug_usage)
DCL_CMD(mem_cfapinit,   cfapinit_usage)
DCL_CMD(cmd_esw_ser,    cmd_esw_ser_usage)
#ifdef BCM_HERCULES_SUPPORT
DCL_CMD(mem_llainit,    llainit_usage)
#endif

DCL_CMD(mem_insert, insert_usage)
DCL_CMD(mem_delete, delete_usage)
DCL_CMD(mem_lookup, lookup_usage)
DCL_CMD(mem_remove, remove_usage)
DCL_CMD(mem_search, search_usage)
DCL_CMD(mem_pop,    pop_usage)
DCL_CMD(mem_push,   push_usage)

DCL_CMD(cmd_esw_mem_write,  cmd_esw_mem_write_usage)
DCL_CMD(cmd_esw_mem_modify, cmd_esw_mem_modify_usage)
DCL_CMD(cmd_esw_mem_list,   cmd_esw_mem_list_usage)
DCL_CMD(mem_esw_cache,  mem_esw_cache_usage)

DCL_CMD(cmd_robo_mem_write, cmd_robo_mem_write_usage)
DCL_CMD(cmd_robo_mem_modify,    cmd_robo_mem_modify_usage)
DCL_CMD(cmd_robo_mem_list,  cmd_robo_mem_list_usage)
DCL_CMD(mem_robo_cache, mem_robo_cache_usage)
#ifdef INCLUDE_MEM_SCAN
DCL_CMD(mem_scan,   memscan_usage)
#endif

/* debug.c */

#ifdef BROADCOM_DEBUG
DCL_CMD(dbg_cellhdr,    cellhdr_usage)
#endif  /* BROADCOM_DEBUG */

DCL_CMD(dbg_bkpmon, bkpmon_usage)

/* dump.c */

DCL_CMD(cmd_esw_dump,   cmd_esw_dump_usage)
DCL_CMD(cmd_robo_dump,  cmd_robo_dump_usage)
/* txrx.c */

DCL_CMD(cmd_esw_tx,     cmd_esw_tx_usage)
DCL_CMD(cmd_esw_tx_start,   cmd_esw_tx_start_usage)
DCL_CMD(cmd_esw_tx_stop,    cmd_esw_tx_stop_usage)
DCL_CMD(cmd_esw_tx_count,   cmd_esw_tx_count_usage)
DCL_CMD(cmd_esw_rx_cfg, cmd_esw_rx_cfg_usage)
DCL_CMD(cmd_esw_rx_init,    cmd_esw_rx_init_usage)
DCL_CMD(cmd_esw_rx_mon, cmd_esw_rx_mon_usage)

DCL_CMD(cmd_robo_tx,        cmd_robo_tx_usage)
DCL_CMD(cmd_robo_tx_start,  cmd_robo_tx_start_usage)
DCL_CMD(cmd_robo_tx_stop,   cmd_robo_tx_stop_usage)
DCL_CMD(cmd_robo_tx_count,  cmd_robo_tx_count_usage)
DCL_CMD(cmd_robo_rx_cfg,    cmd_robo_rx_cfg_usage)
DCL_CMD(cmd_robo_rx_init,   cmd_robo_rx_init_usage)
DCL_CMD(cmd_robo_rx_mon,    cmd_robo_rx_mon_usage)


DCL_CMD(cmd_rx,     cmd_rx_usage)
DCL_CMD(cmd_hdr_mode,   cmd_hdr_mode_usage)
DCL_CMD(cmd_txrx_reload,cmd_txrx_reload_usage)
DCL_CMD(cmd_drt,    cmd_drt_usage)
/* if.c */

DCL_CMD(cmd_if_config,  cmd_if_config_usage)
#ifdef INCLUDE_FCMAP
DCL_CMD(if_esw_fcoe,     if_esw_fcoe_usage)
#endif
#ifdef BCM_FIELD_SUPPORT
DCL_CMD(if_esw_field_proc,  if_esw_field_proc_usage)
DCL_CMD(if_robo_field_proc, if_robo_field_proc_usage)
#endif /* BCM_FIELD_SUPPORT */
#ifdef BCM_ACL_SUPPORT
DCL_CMD(cmd_acl,    cmd_acl_usage)
#endif /* BCM_ACL_SUPPORT */
DCL_CMD(if_esw_stg,     if_esw_stg_usage)
DCL_CMD(if_esw_vlan,    if_esw_vlan_usage)
DCL_CMD(if_esw_pvlan,   if_esw_pvlan_usage)
DCL_CMD(if_robo_stg,        if_robo_stg_usage)
DCL_CMD(if_robo_vlan,   if_robo_vlan_usage)
DCL_CMD(if_robo_pvlan,  if_robo_pvlan_usage)
DCL_CMD(if_stkd,    if_stkd_usage)
#if defined(BCM_ROBO_SUPPORT)
DCL_CMD(if_protovlan,   if_protovlan_usage)
#endif /* BCM_ROBO_SUPPORT */
DCL_CMD(if_esw_trunk,   if_esw_trunk_usage)
DCL_CMD(if_esw_mirror,  if_esw_mirror_usage)
DCL_CMD(mem_watch,  memwatch_usage)
DCL_CMD(reg_watch,  regwatch_usage)
DCL_CMD(if_esw_l2,      if_esw_l2_usage)
DCL_CMD(if_esw_bpdu,    if_esw_bpdu_usage)
DCL_CMD(if_robo_trunk,  if_robo_trunk_usage)
DCL_CMD(if_robo_mirror, if_robo_mirror_usage)
DCL_CMD(if_robo_l2,     if_robo_l2_usage)
DCL_CMD(if_robo_bpdu,   if_robo_bpdu_usage)

DCL_CMD(if_dmirror, if_dmirror_usage)
DCL_CMD(if_robo_dmirror,    if_robo_dmirror_usage)
#ifdef INCLUDE_L3
DCL_CMD(if_ipmc,    if_ipmc_usage)
DCL_CMD(if_l3,      if_l3_usage)
#ifdef BCM_EASYRIDER_SUPPORT
DCL_CMD(if_er_mpls,    if_er_mpls_usage)
#endif
#ifdef BCM_TRIUMPH_SUPPORT
DCL_CMD(if_tr_mpls,    if_tr_mpls_usage)
#endif
#ifdef BCM_TRIUMPH2_SUPPORT
DCL_CMD(if_tr2_mim,    if_tr2_mim_usage)
#endif
#ifdef BCM_TRIUMPH2_SUPPORT
DCL_CMD(if_tr2_wlan,    if_tr2_wlan_usage)
#endif
#endif
DCL_CMD(if_esw_mcast,   if_esw_mcast_usage)
DCL_CMD(if_robo_mcast,  if_robo_mcast_usage)
#ifdef VXWORKS
DCL_CMD(if_host,    if_host_usage)
#endif
#ifdef BCM_HERCULES_SUPPORT
DCL_CMD(if_xqdump,  if_xqdump_usage)
DCL_CMD(if_xqerr,   if_xqerr_usage)
DCL_CMD(if_ibdump,  if_ibdump_usage)
DCL_CMD(if_mmu_cfg, if_mmu_cfg_usage)
#endif
#ifdef  BCM_XGS_SUPPORT
DCL_CMD(if_h2higig, if_h2higig_usage)
#ifdef BCM_HIGIG2_SUPPORT
DCL_CMD(if_h2higig2, if_h2higig2_usage)
#endif
#endif
#ifdef BCM_DMUX_SUPPORT
DCL_CMD(if_dmux,    if_dmux_usage)
#endif

DCL_CMD(if_dos_attack,  if_dos_attack_usage)

#if defined(MBZ)
DCL_CMD(if_cfm_failover,if_cfm_failover_usage)
#endif

/* ipfix.c */
DCL_CMD(cmd_ipfix,  cmd_ipfix_usage)

#if defined(VXWORKS)
#if defined(BMW) || defined(NSX) || defined(METROCORE)
DCL_CMD(lm_console, lm_console_usage)
#endif
#endif

/* multicast.c */
DCL_CMD(cmd_multicast,  cmd_multicast_usage)

/* oam.c */

DCL_CMD(cmd_esw_oam, cmd_esw_oam_usage)

#if defined(INCLUDE_BFD)
/* bfd.c */

DCL_CMD(cmd_esw_bfd, cmd_esw_bfd_usage)
#endif

DCL_CMD(cmd_esw_txbeacon, cmd_esw_txbeacon_usage)

#if defined(BCM_ROBO_SUPPORT)
/* subpoort.c */
DCL_CMD(if_robo_subport, if_robo_subport_usage)
/* mcastrep.c */
DCL_CMD(if_robo_mcastrep, if_robo_mcastrep_usage)
/* port control */
DCL_CMD(if_robo_port_control, if_robo_port_control_usage)
/* arl thread */
DCL_CMD(if_robo_l2mode, if_robo_l2mode_usage)
/* port cross connect */
DCL_CMD(if_robo_port_cross_connect, if_robo_port_cross_connect_usage)

#endif /* BCM_ROBO_SUPPORT */

#if defined(INCLUDE_SIM8051)
DCL_CMD(sim8051_ent,    sim8051_usage)
#endif

#ifdef BCM_EASYRIDER_SUPPORT
DCL_CMD(cmd_qdr,    cmd_qdr_usage)
DCL_CMD(cmd_cdt,    cmd_cdt_usage)
#endif /* BCM_EASYRIDER_SUPPORT */

#if defined(BCM_EASYRIDER_SUPPORT) || defined(BCM_TRIUMPH_SUPPORT)
DCL_CMD(cmd_tcam,   cmd_tcam_usage)
DCL_CMD(cmd_sram,   cmd_sram_usage)
DCL_CMD(cmd_extt,   cmd_extt_usage)
DCL_CMD(cmd_extts,  cmd_extts_usage)
DCL_CMD(cmd_extt2,   cmd_extt2_usage)
#endif /* BCM_EASYRIDER_SUPPORT || BCM_TRIUMPH_SUPPORT */

/* port.c */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
DCL_CMD(if_esw_port_policer,   if_esw_port_policer_usage)
#endif
DCL_CMD(if_esw_port_stat,   if_esw_port_stat_usage)
DCL_CMD(if_esw_port_rate,   if_esw_port_rate_usage)
DCL_CMD(if_esw_port,    if_esw_port_usage)
DCL_CMD(if_esw_gport,    if_esw_gport_usage)
DCL_CMD(if_esw_dscp,    if_esw_dscp_usage)
DCL_CMD(if_esw_ipg,     if_esw_ipg_usage)
DCL_CMD(if_esw_dtag,    if_esw_dtag_usage)
DCL_CMD(if_esw_linkscan,    if_esw_linkscan_usage)
DCL_CMD(if_esw_phy,     if_esw_phy_usage)
DCL_CMD(if_esw_combo,   if_esw_combo_usage)
DCL_CMD(cmd_esw_cablediag,  cmd_esw_cablediag_usage)
DCL_CMD(cmd_ibod_sync, ibod_sync_usage)

DCL_CMD(if_robo_port_stat,  if_robo_port_stat_usage)
DCL_CMD(if_robo_port_rate,  if_robo_port_rate_usage)
DCL_CMD(if_robo_port,   if_robo_port_usage)
DCL_CMD(if_robo_dscp,   if_robo_dscp_usage)
DCL_CMD(if_robo_ipg,        if_robo_ipg_usage)
DCL_CMD(if_robo_dtag,   if_robo_dtag_usage)
DCL_CMD(if_robo_linkscan,   if_robo_linkscan_usage)
DCL_CMD(if_robo_phy,        if_robo_phy_usage)
DCL_CMD(if_robo_combo,  if_robo_combo_usage)
DCL_CMD(cmd_robo_cablediag, cmd_robo_cablediag_usage)


DCL_CMD(cmd_xaui,       cmd_xaui_usage)
DCL_CMD(if_port_samp_rate,  if_port_samp_rate_usage)
DCL_CMD(if_robo_port_samp_rate, if_robo_port_samp_rate_usage)
DCL_CMD(if_egress,  if_egress_usage)
DCL_CMD(if_robo_egress, if_robo_egress_usage)
DCL_CMD(cmd_color,  cmd_color_usage)
DCL_CMD(cmd_robo_color,  cmd_robo_color_usage)
/* system.c */

DCL_CMD(cmd_esw_age,    cmd_age_esw_usage)
DCL_CMD(cmd_robo_age,   age_robo_usage)
#ifdef  BCM_XGS_SWITCH_SUPPORT
DCL_CMD(cmd_l2mode, l2mode_usage)
DCL_CMD(cmd_hash,   hash_usage)
#endif
DCL_CMD(cmd_stimeout,   stimeout_usage)
DCL_CMD(cmd_mtimeout,   mtimeout_usage)
DCL_CMD(cmd_btimeout,   btimeout_usage)
DCL_CMD(cmd_schan,  schan_usage)
DCL_CMD(cmd_filter, cmd_filter_usage)
DCL_CMD(cmd_stkmode,    cmd_stkmode_usage)
#ifdef INCLUDE_TCL
DCL_CMD(cmd_tcl,    tcl_usage)
#endif
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
DCL_CMD(cmd_esw_policer_global_meter, cmd_esw_policer_global_meter_usage)
#endif
DCL_CMD(cmd_esw_show,   cmd_esw_show_usage)
DCL_CMD(cmd_esw_clear,  cmd_esw_clear_usage)
DCL_CMD(cmd_robo_show,  cmd_robo_show_usage)
DCL_CMD(cmd_robo_clear, cmd_robo_clear_usage)

DCL_CMD(cmd_check,  check_usage)
DCL_CMD(cmd_socres, socres_usage)
#ifdef PLISIM
DCL_CMD(cmd_txen,   cmd_txen_usage)
DCL_CMD(cmd_shutd,  cmd_shutd_usage)
DCL_CMD(cmd_simstart,   cmd_simstart_usage)
#endif
DCL_CMD(cmd_esw_counter,    cmd_esw_counter_usage)
DCL_CMD(cmd_esw_custom_stat,    cmd_esw_custom_stat_usage)
DCL_CMD(cmd_esw_soc,    cmd_esw_soc_usage)
DCL_CMD(cmd_esw_pbmp,   cmd_esw_pbmp_usage)
DCL_CMD(cmd_esw_cos,    cmd_esw_cos_usage)
DCL_CMD(cmd_esw_rate,   cmd_esw_rate_usage)

DCL_CMD(cmd_robo_counter,   cmd_robo_counter_usage)
DCL_CMD(cmd_robo_soc,   cmd_robo_soc_usage)
DCL_CMD(cmd_robo_pbmp,  cmd_robo_pbmp_usage)
DCL_CMD(cmd_robo_cos,   cmd_robo_cos_usage)
DCL_CMD(cmd_robo_rate,  cmd_robo_rate_usage)


DCL_CMD(cmd_panic,  panic_usage)
DCL_CMD(cmd_bist,   bist_usage)
DCL_CMD(cmd_bw_rate,    cmd_bw_rate_usage)
#ifdef INCLUDE_NFS
DCL_CMD(cmd_nfs,    cmd_nfs_usage)
#endif
DCL_CMD(cmd_intr,   cmd_intr_usage)
DCL_CMD(cmd_ss,     cmd_ss_usage)

/* switchctl.c */

DCL_CMD(cmd_switch_control, cmd_switch_control_usage)

DCL_CMD(cmd_robo_switch_control, cmd_robo_switch_control_usage)

/* variable.c */

DCL_CMD(var_export, var_export_usage)
DCL_CMD(var_local,  var_local_usage)
DCL_CMD(var_display,    var_display_usage)

#if defined(VXWORKS)
DCL_CMD(cmd_ping,   ping_usage)
#endif

#if defined(INCLUDE_BCMX_DIAG)
DCL_CMD(cmd_mode_bcmx,  shell_bcmx_usage)
DCL_CMD(cmd_mode_bcm,   shell_bcm_usage)
DCL_CMD(cmd_mode,       shell_mode_usage)
#endif

DCL_CMD(cmd_esw_auth,   auth_esw_cmd_usage)
DCL_CMD(cmd_robo_auth,  cmd_robo_auth_usage)
#ifdef BCM_XGS_SUPPORT
DCL_CMD(cmd_modmap, modmap_cmd_usage)
#endif

#ifdef BCM_HERCULES15_SUPPORT
DCL_CMD(cmd_trunkpool,  trunkpool_cmd_usage)
#endif

#if defined(INCLUDE_LIB_CPUDB)
/* tksdiag.c */
DCL_CMD(ct_cpudb,   ct_cpudb_usage)
DCL_CMD(ct_setup,   ct_setup_usage)
DCL_CMD(ct_echo,    ct_echo_usage)
DCL_CMD(tks_stk_task,   tks_stk_task_usage)
DCL_CMD(ct_install, ct_install_usage)
DCL_CMD(cmd_st_db_dump, cmd_st_db_dump_usage)
DCL_CMD(cmd_cpudb_parse,cmd_cpudb_parse_usage)
#endif
DCL_CMD(cmd_stk_port_set,cmd_stk_port_set_usage)
DCL_CMD(cmd_stk_port_get,cmd_stk_port_get_usage)
DCL_CMD(cmd_stk_mode,cmd_stk_mode_usage)

#if defined(INCLUDE_LIB_AEDEV)
DCL_CMD(cmd_aedev,  cmd_aedev_usage)
DCL_CMD(cmd_swdev,  cmd_swdev_usage)
#endif

/* Assert */
DCL_CMD(cmd_assert,  cmd_assert_usage)

/* dispatch.c */
#ifdef  BCM_ASYNC_SUPPORT
DCL_CMD(cmd_async,  cmd_async_usage)
#endif
#ifdef  BCM_RPC_SUPPORT
DCL_CMD(cmd_rpc,    cmd_rpc_usage)
#endif

DCL_CMD(cmd_dispatch,   cmd_dispatch_usage)
#if defined(BCM_XGS_SUPPORT)||defined(BCM_ROBO_SUPPORT)||defined(BCM_SBX_SUPPORT)
DCL_CMD(cmd_device,     cmd_device_usage)
#endif  /* BCM_[XGS ROBO SBX]_SUPPORT */
/* i2c_cmds.c */

#ifdef INCLUDE_I2C
#ifdef BCM_FE2000_SUPPORT
DCL_CMD(cmd_bsc,    cmd_bsc_usage)
DCL_CMD(cmd_sensor, cmd_sensor_usage)
DCL_CMD(cmd_setfanspeed,cmd_setfanspeed_usage)
DCL_CMD(cmd_dpm,    cmd_dpm_usage)
#endif /*BCM_FE2000_SUPPORT*/
DCL_CMD(cmd_i2c,    cmd_i2c_usage)
DCL_CMD(cmd_cpu_i2c,    cmd_cpu_i2c_usage)
DCL_CMD(cmd_temperature,cmd_temperature_usage)
DCL_CMD(cmd_nvram,  cmd_nvram_usage)
DCL_CMD(cmd_adc,    cmd_adc_usage)
DCL_CMD(cmd_clk,    cmd_clk_usage)
DCL_CMD(cmd_xclk,   cmd_xclk_usage)
DCL_CMD(cmd_poe,    cmd_poe_usage)
DCL_CMD(cmd_xpoe,   cmd_xpoe_usage)
DCL_CMD(cmd_muxsel, cmd_muxsel_usage)
DCL_CMD(cmd_hclksel,    cmd_hclksel_usage)
#if defined(SHADOW_SVK) || defined(BCM_CALADAN3_SVK)
DCL_CMD(cmd_pio,    cmd_pio_usage)
#endif
DCL_CMD(cmd_poesel, cmd_poesel_usage)
DCL_CMD(cmd_dac,    cmd_dac_usage)
DCL_CMD(cmd_bb,     cmd_bb_usage)
DCL_CMD(cmd_lcdmsg, cmd_lcdmsg_usage)
#ifdef BCM_ESW_SUPPORT
DCL_CMD(cmd_synth,  cmd_synth_usage)
DCL_CMD(cmd_ppdclk, cmd_ppdclk_usage)
#endif
DCL_CMD(cmd_pcie,  cmd_pcie_usage)
#endif

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
DCL_CMD(cmd_sal_profile,    cmd_sal_profile_usage)
#endif
#endif

DCL_CMD(cmd_stkcb,  cmd_stkcb_usage)

#ifdef BCM_EASY_RELOAD_SUPPORT
DCL_CMD(cmd_xxreload,   cmd_xxreload_usage)
#endif

#ifdef INCLUDE_EXAMPLES
DCL_CMD(cmd_example_exec, cmd_example_exec_usage)
#endif

/* eav.c */
DCL_CMD(cmd_esw_eav,    cmd_esw_eav_usage)

#ifdef INCLUDE_CES
DCL_CMD(cmd_esw_ces,    cmd_esw_ces_usage)
#endif

#ifdef BCM_ROBO_SUPPORT
DCL_CMD(cmd_robo_eav,   cmd_robo_eav_usage)

#ifdef INCLUDE_EAV_APPL
DCL_CMD(cmd_robo_timesync,	cmd_robo_timesync_usage)
DCL_CMD(cmd_robo_discovery,	        cmd_robo_discovery_usage)
DCL_CMD(cmd_robo_bandwidthreserve,	cmd_robo_bandwidthreserve_usage)
#endif

#ifdef IMP_SW_PROTECT
DCL_CMD(if_robo_imp_protect, if_robo_imp_protect_usage)
#endif /* IMP_SW_PROTECT */

#ifdef INCLUDE_APS_DIAG_LIBS
DCL_CMD(cmd_robo_aps, cmd_robo_aps_usage)
#endif /* INCLUDE_APS_DIAG_LIBS */

#endif

/* dma.c */
DCL_CMD(cmd_esw_dma, cmd_esw_dma_usage)

#ifdef INCLUDE_MACSEC
DCL_CMD(sh_macsec,	sh_macsec_usage)
#endif /* INCLUDE_MACSEC */

#ifdef INCLUDE_KNET
DCL_CMD(cmd_knet_ctrl,	cmd_knet_ctrl_usage)
#endif /* INCLUDE_UKMSG_SUPPORT */

/* board.c */
#ifdef INCLUDE_BOARD
DCL_CMD(board_cmd, board_cmd_usage)
#endif

#ifdef INCLUDE_REGEX
DCL_CMD(cmd_regex, cmd_regex_usage)
#endif

/* buffer.c */
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
DCL_CMD(cmd_buffer, cmd_buffer_usage)
#endif /* BCM_TRIDENT_SUPPORT || BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT)
DCL_CMD(cmd_lls, cmd_lls_usage)
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIDENT2_SUPPORT)
DCL_CMD(cmd_hsp, cmd_hsp_usage)
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_BPROF_STATS
DCL_CMD(sh_prof, sh_prof_usage)
#endif

#ifdef BCM_DDR3_SUPPORT
DCL_CMD(cmd_ddr_mem_read, cmd_ddr_mem_read_usage)
DCL_CMD(cmd_ddr_mem_write, cmd_ddr_mem_write_usage)
DCL_CMD(cmd_ddr_phy_read, cmd_ddr_phy_read_usage)
DCL_CMD(cmd_ddr_phy_write, cmd_ddr_phy_write_usage)
DCL_CMD(cmd_ddr_phy_tune, cmd_ddr_phy_tune_usage)
#endif
#if defined(BCM_TRIDENT_SUPPORT)
DCL_CMD(cmd_hash_destination, hash_destination_usage)
#endif /* BCM_TRIDENT_SUPPORT */

#undef  DCL_CMD

#endif  /* !_DIAG_CMDLIST_H */
