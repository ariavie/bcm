/* 
 * $Id: llm_pack.h,v 1.6 Broadcom SDK $
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
 * File:        llm_pack.h
 * Purpose:     Interface to pack and unpack LLM messages.
 *              This is to be shared between SDK host and uKernel.
 */

#ifndef   _SOC_SHARED_LLM_PACK_H_
#define   _SOC_SHARED_LLM_PACK_H_

#ifdef BCM_UKERNEL
  /* Build for uKernel not SDK */
  #include "sdk_typedefs.h"
#else
  #include <sal/types.h>
#endif

#include <soc/shared/mos_msg_common.h>
extern uint32 shr_max_buffer_get(void);
extern uint8 *shr_llm_msg_ctrl_init_pack    (uint8 *buf, shr_llm_msg_ctrl_init_t *msg);
extern uint8 *shr_llm_msg_ctrl_init_unpack  (uint8 *buf, shr_llm_msg_ctrl_init_t *msg);
extern uint8 *shr_llm_msg_pon_att_pack      (uint8 *buf, shr_llm_msg_pon_att_t *msg);
extern uint8 *shr_llm_msg_pon_att_unpack    (uint8 *buf, shr_llm_msg_pon_att_t *msg);
extern uint8 *shr_llm_msg_app_info_get_pack (uint8 *buf, shr_llm_msg_app_info_get_t *msg);
extern uint8 *shr_llm_msg_app_info_get_unpack(uint8 *buf, shr_llm_msg_app_info_get_t *msg);
extern uint8 *shr_llm_msg_ctrl_config_pack  (uint8 *buf, shr_llm_msg_ctrl_limit_value_t *msg);
extern uint8 *shr_llm_msg_ctrl_config_unpack(uint8 *buf, shr_llm_msg_ctrl_limit_value_t *msg);
extern uint8 *shr_llm_msg_pon_db_pack       (uint8 *buf, shr_llm_PON_ID_attributes_t *msg);
extern uint8 *shr_llm_msg_pon_db_unpack     (uint8 *buf, shr_llm_PON_ID_attributes_t *msg);
extern uint8 *shr_llm_msg_pointer_pool_pack (uint8 *buf, shr_llm_pointer_pool_t *msg);
extern uint8 *shr_llm_msg_pointer_pool_unpack(uint8 *buf, shr_llm_pointer_pool_t *msg);
extern uint8 *shr_llm_msg_pon_db_pack_port  (uint8 *buf, uint32 *port);
extern uint8 *shr_llm_msg_pon_db_unpack_port(uint8 *buf, uint32 *port);
extern uint8 *shr_llm_msg_pon_att_enable_pack(uint8 *buf, shr_llm_msg_pon_att_enable_t *msg);
extern uint8 *shr_llm_msg_pon_att_enable_unpack(uint8 *buf, shr_llm_msg_pon_att_enable_t *msg);
extern uint8 *shr_llm_msg_pon_att_update_pack(uint8 *buf, shr_llm_msg_pon_att_update_t *msg);
extern uint8 *shr_llm_msg_pon_att_update_unpack(uint8 *buf, shr_llm_msg_pon_att_update_t *msg);
extern uint8 *shr_llm_msg_pon_att_rx_mode_pack(uint8 *buf, shr_llm_msg_pon_att_rx_mode_t *msg);
extern uint8 *shr_llm_msg_pon_att_rx_mode_unpack(uint8 *buf, shr_llm_msg_pon_att_rx_mode_t *msg);
extern uint8 *shr_llm_msg_pon_att_mac_move_pack(uint8 *buf, shr_llm_msg_pon_att_mac_move_t *msg);
extern uint8 *shr_llm_msg_pon_att_mac_move_unpack(uint8 *buf, shr_llm_msg_pon_att_mac_move_t *msg);
extern uint8 *shr_llm_msg_pon_att_mac_move_event_pack(uint8 *buf, shr_llm_msg_pon_att_mac_move_event_t *msg);
extern uint8 *shr_llm_msg_pon_att_mac_move_event_unpack(uint8 *buf, shr_llm_msg_pon_att_mac_move_event_t *msg);
#endif /* _SOC_SHARED_LLM_PACK_H_ */
