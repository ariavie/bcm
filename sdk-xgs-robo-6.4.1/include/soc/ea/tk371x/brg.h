/*
 * $Id: brg.h,v 1.7 Broadcom SDK $
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
 * File:     brg.h
 * Purpose:
 *
 */

#ifndef _SOC_EA_BRG_H
#define _SOC_EA_BRG_H

#include <soc/ea/tk371x/TkBrgApi.h>
#include <soc/ea/tk371x/TkExtSwitchApi.h>
#include <soc/ea/tk371x/Oam.h>

typedef LoopbackState soc_port_loopback_state_t;

typedef TagPortMirrorCfg 	_soc_ea_tag_port_mirror_cfg_t;
typedef FlushMacType		_soc_ea_flush_mac_type_t;
typedef ArlLearnStatus		_soc_ea_arl_learn_status_t;
typedef ForwardingMode		_soc_ea_fowarding_mode_t;
typedef OamAutoNegAdminState _soc_ea_oam_auto_neg_admin_state_t;
typedef OamMacDuplexStatus	_soc_ea_oam_mac_duplex_status_t;


#define _soc_ea_dyna_mac_tab_size_get	TkExtOamGetDynaMacTabSizeNew
#define _soc_ea_dyna_mac_tab_size_set	TkExtOamSetDynaMacTabSizeNew
#define _soc_ea_dyna_mac_tab_age_get	TkExtOamGetDynaMacTabAge
#define _soc_ea_dyna_mac_tab_age_set	TkExtOamSetDynaMacTabAge
#define _soc_ea_dyna_mac_entries_get	TkExtOamGetDynaMacEntries
#define _soc_ea_dyna_mac_table_clr_set	TkExtOamSetClrDynaMacTable
#define _soc_ea_static_mac_entries_get	TkExtOamGetStaticMacEntries
#define _soc_ea_static_mac_entry_add	TkExtOamAddStaticMacEntry
#define _soc_ea_static_mac_entry_del	TkExtOamDelStaticMacEntry
#define	_soc_ea_mac_table_flush			TkExtOamFlushMacTableNew
#define _soc_ea_mac_learning_set		TkExtOamSetMacLearning
#define _soc_ea_forward_mode_set		TkExtOamSetForwardMode
#define _soc_ea_auto_neg_get			TkExtOamGetAutoNeg
#define _soc_ea_auto_neg_set			TkExtOamSetAutoNeg
#define _soc_ea_mtu_set					TkExtOamSetMtu
#define _soc_ea_mtu_get					TkExtOamGetMtu
#define _soc_ea_flood_unknown_set		TkExtOamSetFloodUnknown
#define _soc_ea_flood_unknown_get		TkExtOamGetFloodUnknown
#define _soc_ea_mac_learn_switch_set	TkExtOamSetMacLearnSwitch
#define _soc_ea_mac_learn_switch_get	TkExtOamGetMacLearnSwitch
#define _soc_ea_eth_link_state_get		TkExtOamGetEthLinkState
#define _soc_ea_phy_admin_state_set		TkExtOamSetPhyAdminState
#define _soc_ea_phy_admin_state_get		TkExtOamGetPhyAdminState
#define _soc_ea_all_filter_table_clr	TkExtOamClrAllFilterTbl

#endif /* _SOC_EA_HAL_BRG_H */
