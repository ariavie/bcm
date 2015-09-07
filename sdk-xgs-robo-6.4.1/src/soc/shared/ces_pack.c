/* 
 * $Id: ces_pack.c 1.1.2.1 Broadcom SDK $
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
 * File:        ces_pack.c
 * Purpose:     CES pack and unpack routines for CES control messages.
 *
 * CES messages between the Host CPU and uController are sent
 * using the uc_message module which allows short messages
 * to be passed (see include/soc/shared/mos_msg_common.h)
 *
 * Additional information for a given message (a long message) is passed
 * using DMA.  The CES control message types defines the format
 * for these long messages.
 *
 * This file is shared between SDK and uKernel.
 */
#ifdef BCM_UKERNEL
  /* Build for uKernel not SDK */
  #include "sdk_typedefs.h"
#else
  #include <sal/types.h>
#endif
#include <shared/pack.h>
#include <soc/shared/mos_msg_ces.h>
#include <soc/shared/ces_pack.h>

uint8 *
bcm_ces_crm_init_msg_pack(uint8 *buf, bcm_ces_crm_init_msg_t *msg)
{
    _SHR_PACK_U8(buf, msg->flags);

    return buf;
}

uint8 *
bcm_ces_crm_init_msg_unpack(uint8 *buf, bcm_ces_crm_init_msg_t *msg)
{
    _SHR_UNPACK_U8(buf, msg->flags);

    return buf;
}

uint8 *
bcm_ces_crm_cclk_config_msg_pack(uint8 *buf, bcm_ces_crm_cclk_config_msg_t *msg)
{
    _SHR_PACK_U8(buf, msg->flags);
    _SHR_PACK_U8(buf, msg->e_cclk_select);
    _SHR_PACK_U8(buf, msg->e_ref_clk_1_select);
    _SHR_PACK_U8(buf, msg->e_ref_clk_2_select);
    _SHR_PACK_U32(buf, msg->n_ref_clk_1_port);
    _SHR_PACK_U32(buf, msg->n_ref_clk_2_port);
    _SHR_PACK_U8(buf, msg->e_protocol);

    return buf;
}

uint8 *
bcm_ces_crm_cclk_config_msg_unpack(uint8 *buf, bcm_ces_crm_cclk_config_msg_t *msg)
{
    _SHR_UNPACK_U8(buf, msg->flags);
    _SHR_UNPACK_U8(buf, msg->e_cclk_select);
    _SHR_UNPACK_U8(buf, msg->e_ref_clk_1_select);
    _SHR_UNPACK_U8(buf, msg->e_ref_clk_2_select);
    _SHR_UNPACK_U32(buf, msg->n_ref_clk_1_port);
    _SHR_UNPACK_U32(buf, msg->n_ref_clk_2_port);
    _SHR_UNPACK_U8(buf, msg->e_protocol);

    return buf;
}

uint8 *
bcm_ces_crm_rclock_config_msg_pack(uint8 *buf, bcm_ces_crm_rclock_config_msg_t *msg)
{
    _SHR_PACK_U8(buf, msg->flags);
    _SHR_PACK_U8(buf, msg->b_enable);
    _SHR_PACK_U8(buf, msg->e_protocol);
    _SHR_PACK_U8(buf, msg->b_structured);
    _SHR_PACK_U8(buf, msg->output_brg);
    _SHR_PACK_U8(buf, msg->rclock);
    _SHR_PACK_U8(buf, msg->port);
    _SHR_PACK_U8(buf, msg->recovery_type);
    _SHR_PACK_U16(buf, msg->tdm_clocks_per_packet);
    _SHR_PACK_U8(buf, msg->service);
    /*Add new fields*/
    _SHR_PACK_U8(buf, msg->rcr_flags);
    _SHR_PACK_U32(buf, msg->ho_q_count);
    _SHR_PACK_U32(buf, msg->ho_a_threshold);
    _SHR_PACK_U32(buf, msg->ho_a_threshold);
    _SHR_PACK_U32(buf, msg->ho_a_threshold);
    return buf;
}

uint8 *
bcm_ces_crm_rclock_config_msg_unpack(uint8 *buf, bcm_ces_crm_rclock_config_msg_t *msg)
{
    _SHR_UNPACK_U8(buf, msg->flags);
    _SHR_UNPACK_U8(buf, msg->b_enable);
    _SHR_UNPACK_U8(buf, msg->e_protocol);
    _SHR_UNPACK_U8(buf, msg->b_structured);
    _SHR_UNPACK_U8(buf, msg->output_brg);
    _SHR_UNPACK_U8(buf, msg->rclock);
    _SHR_UNPACK_U8(buf, msg->port);
    _SHR_UNPACK_U8(buf, msg->recovery_type);
    _SHR_UNPACK_U16(buf, msg->tdm_clocks_per_packet);
    _SHR_UNPACK_U8(buf, msg->service);
    /*Add new fields*/
    _SHR_UNPACK_U8(buf, msg->rcr_flags);
    _SHR_UNPACK_U32(buf, msg->ho_q_count);
    _SHR_UNPACK_U32(buf, msg->ho_a_threshold);
    _SHR_UNPACK_U32(buf, msg->ho_a_threshold);
    _SHR_UNPACK_U32(buf, msg->ho_a_threshold);

    return buf;
}

uint8 *
bcm_ces_crm_status_msg_pack(uint8 *buf, bcm_ces_crm_status_msg_t *msg)
{
    _SHR_PACK_U8(buf, msg->rclock);
    _SHR_PACK_U8(buf, msg->clock_state);
    _SHR_PACK_U32(buf, msg->seconds_locked);
    _SHR_PACK_U32(buf, msg->seconds_active);
    _SHR_PACK_U32(buf, msg->calculated_frequency_w);
    _SHR_PACK_U32(buf, msg->calculated_frequency_f);

    return buf;
}

uint8 *
bcm_ces_crm_status_msg_unpack(uint8 *buf, bcm_ces_crm_status_msg_t *msg)
{
    _SHR_UNPACK_U8(buf, msg->rclock);
    _SHR_UNPACK_U8(buf, msg->clock_state);
    _SHR_UNPACK_U32(buf, msg->seconds_locked);
    _SHR_UNPACK_U32(buf, msg->seconds_active);
    _SHR_UNPACK_U32(buf, msg->calculated_frequency_w);
    _SHR_UNPACK_U32(buf, msg->calculated_frequency_f);

    return buf;
}
