/*
 * $Id: ea.h,v 1.9 Broadcom SDK $
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
 * File:     ea.h
 * Purpose:
 *
 */
#ifndef _BCM_DIAG_EA_H
#define _BCM_DIAG_EA_H
#include <appl/diag/diag.h>

#define CLI_CMD(_f,_u)  \
        extern cmd_result_t     _f(int, args_t *); \
        extern char             _u[];
extern int bcm_ea_cmd_cnt;
extern cmd_t bcm_ea_cmd_list[];

#if defined(BCM_TK371X_SUPPORT)
CLI_CMD(cmd_ea_clear,		cmd_ea_clear_usage)
CLI_CMD(if_ea_l2, 			if_ea_l2_usage)
CLI_CMD(if_ea_l2mode,		if_ea_l2mode_usage)
CLI_CMD(if_ea_linkscan,		if_ea_linkscan_usage)
CLI_CMD(cmd_ea_mem_list,	cmd_ea_mem_list_usage)
CLI_CMD(cmd_ea_reg_list,	cmd_ea_reg_list_usage)
CLI_CMD(if_ea_mcast,		if_ea_mcast_usage)
CLI_CMD(if_ea_port, 		if_ea_port_usage)
CLI_CMD(if_ea_port_control, if_ea_port_control_usage)
CLI_CMD(if_ea_port_cross_connect, if_ea_port_cross_connect_usage)
CLI_CMD(if_ea_port_rate, 	if_ea_port_rate_usage)
CLI_CMD(if_ea_port_samp_rate, if_ea_port_samp_rate_usage)
CLI_CMD(if_ea_port_stat,	if_ea_port_stat_usage)
CLI_CMD(if_ea_pvlan,		if_ea_pvlan_usage)
CLI_CMD(cmd_ea_cos, 		cmd_ea_cos_usage)
CLI_CMD(if_ea_field_proc,   if_ea_field_proc_usage)
CLI_CMD(cmd_ea_rx_cfg, 		cmd_ea_rx_cfg_usage)
CLI_CMD(cmd_ea_rx_init,		cmd_ea_rx_init_usage)
CLI_CMD(cmd_ea_tx,  		cmd_ea_tx_usage)
CLI_CMD(cmd_ea_tx_count, 	cmd_ea_tx_count_usage)
CLI_CMD(cmd_ea_tx_start,  	cmd_ea_tx_start_usage)
CLI_CMD(cmd_ea_tx_stop,		cmd_ea_tx_stop_usage)
CLI_CMD(cmd_ea_show, 		cmd_ea_show_usage)
CLI_CMD(test_ea_domaen, 	test_ea_domaen_usage)
CLI_CMD(cmd_ea_soc, 		cmd_ea_soc_usage)
CLI_CMD(cmd_ea_switch_control,	cmd_ea_switch_control_usage)
CLI_CMD(if_ea_stg, 			if_ea_stg_usage)
CLI_CMD(if_ea_port_phy_control, if_ea_port_phy_control_usage)
#endif
#endif /* _BCM_DIAG_EA_H */
