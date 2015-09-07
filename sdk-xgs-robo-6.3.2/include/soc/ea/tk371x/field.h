/*
 * $Id: field.h 1.1 Broadcom SDK $
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
 * File:     field.h
 * Purpose:
 *
 */
#ifndef _SOC_EA_FIELD_H
#define _SOC_EA_FIELD_H

#include <soc/ea/tk371x/TkRuleApi.h>

typedef TkLinkConfigInfo 	_soc_ea_tk_link_config_info_t;
typedef TkPortConfigInfo 	_soc_ea_tk_port_config_info_t;
typedef TkQueueConfigInfo 	_soc_ea_tk_queue_config_info_t;
typedef TkRuleCondition		_soc_ea_tk_rule_condition_t;
typedef TkRuleConditionList	_soc_ea_tk_rule_condition_list_t;
typedef TkRuleNameQueue		_soc_ea_tk_rule_name_queue_t;
typedef TkRulePara			_soc_ea_tk_rule_para_t;

#define _soc_ea_filter_rules_by_port_get	\
									TkExtOamGetFilterRulesByPort
#define _soc_ea_all_filter_rules_by_port_clear \
									TkExtOamClearAllFilterRulesByPort
#define _soc_ea_all_user_rules_by_port_clear	\
									TkExtOamClearAllUserRulesByPort
#define _soc_ea_one_rule_by_port_add	\
									TkExtOamAddOneRuleByPort
#define _soc_ea_one_rule_by_port_delete	\
									TkExtOamDelOneRuleByPort
#define _soc_ea_all_classify_rules_by_port_clear	\
									TkExtOamClearAllClassifyRulesByPort
#define _soc_ea_rule_act_discard	TkRuleActDiscard
#define _soc_ea_rule_ds_arp_with_specified_sender_ip_addr_discard \
							TkRuleDiscardDsArpWithSpecifiedSenderIpAddr

int
_soc_ea_queue_configuration_get(int unit, TkQueueConfigInfo * q_cfg);

int
_soc_ea_queue_configuration_set(int unit, TkQueueConfigInfo *q_cfg);

#endif /* _SOC_EA_FIELD_H */
