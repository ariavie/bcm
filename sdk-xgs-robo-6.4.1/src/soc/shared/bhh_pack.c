/* 
 * $Id: bhh_pack.c,v 1.13 Broadcom SDK $
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
 * File:        bhh_pack.c
 * Purpose:     BHH pack and unpack routines for:
 *              - BHH Control messages
 *              - Network Packet headers (PDUs)
 *
 *
 * BHH control messages
 *
 * BHH messages between the Host CPU and uController are sent
 * using the uc_message module which allows short messages
 * to be passed (see include/soc/shared/mos_msg_common.h)
 *
 * Additional information for a given message (a long message) is passed
 * using DMA.  The BHH control message types defines the format
 * for these long messages.
 *
 * This file is shared between SDK and uKernel.
 */

#include <shared/pack.h>
#include <soc/shared/bhh_msg.h>
#include <soc/shared/bhh_pkt.h>
#include <soc/shared/bhh_pack.h>


/***********************************************************
 * BHH Control Message Pack/Unpack
 *
 * Functions:
 *      shr_bhh_msg_ctrl_<type>_pack/unpack
 * Purpose:
 *      The following set of routines pack/unpack specified
 *      BHH control message into/from a given buffer
 * Parameters:
 *   _pack()
 *      buffer  - (OUT) Buffer where to pack message.
 *      msg     - (IN)  BHH control message to pack.
 *   _unpack()
 *      buffer  - (IN)  Buffer with message to unpack.
 *      msg     - (OUT) Returns BHH control message.
 * Returns:
 *      Pointer to next position in buffer.
 * Notes:
 *      Assumes pointers are valid and contain enough memory space.
 */
uint8 *
shr_bhh_msg_ctrl_init_pack(uint8 *buf, shr_bhh_msg_ctrl_init_t *msg)
{
    int i;

    _SHR_PACK_U32(buf, msg->num_sessions);
    _SHR_PACK_U32(buf, msg->rx_channel);
    _SHR_PACK_U32(buf, msg->node_id);
    for (i = 0;i < SHR_BHH_CARRIER_CODE_LEN;i++)
	_SHR_PACK_U8(buf, msg->carrier_code[i]);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_init_unpack(uint8 *buf, shr_bhh_msg_ctrl_init_t *msg)
{
    int i;

    _SHR_UNPACK_U32(buf, msg->num_sessions);
    _SHR_UNPACK_U32(buf, msg->rx_channel);
    _SHR_UNPACK_U32(buf, msg->node_id);
    for (i = 0;i < SHR_BHH_CARRIER_CODE_LEN;i++)
	_SHR_UNPACK_U8(buf, msg->carrier_code[i]);

    return buf;
}


uint8 *
shr_bhh_msg_ctrl_sess_delete_pack(uint8 *buf, shr_bhh_msg_ctrl_sess_delete_t *msg)
{
    _SHR_PACK_U32(buf, msg->sess_id);
    return buf;
}

uint8 *
shr_bhh_msg_ctrl_sess_delete_unpack(uint8 *buf, shr_bhh_msg_ctrl_sess_delete_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->sess_id);
    return buf;
}

uint8 *
shr_bhh_msg_ctrl_sess_enable_pack(uint8 *buf, shr_bhh_msg_ctrl_sess_enable_t *msg)
{
    _SHR_PACK_U32(buf, msg->sess_id);
    _SHR_PACK_U32(buf, msg->flags);
    _SHR_PACK_U16(buf, msg->remote_mep_id);
    _SHR_PACK_U8(buf, msg->enable);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_sess_enable_unpack(uint8 *buf, shr_bhh_msg_ctrl_sess_enable_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->sess_id);
    _SHR_UNPACK_U32(buf, msg->flags);
    _SHR_UNPACK_U16(buf, msg->remote_mep_id);
    _SHR_UNPACK_U8(buf, msg->enable);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_sess_set_pack(uint8 *buf,
                               shr_bhh_msg_ctrl_sess_set_t *msg)
{
    uint16 i;

    _SHR_PACK_U32(buf, msg->sess_id);             
    _SHR_PACK_U32(buf, msg->flags);
    _SHR_PACK_U8(buf, msg->passive);
    _SHR_PACK_U8(buf, msg->mel);
    _SHR_PACK_U16(buf, msg->mep_id);
    for (i = 0; i < SHR_BHH_MEG_ID_LENGTH; i++) {
        _SHR_PACK_U8(buf, msg->meg_id[i]);
    }
    _SHR_PACK_U32(buf, msg->period);
    _SHR_PACK_U8(buf, msg->encap_type);
    _SHR_PACK_U32(buf, msg->encap_length);
    for (i = 0; i < SHR_BHH_MAX_ENCAP_LENGTH; i++) {
        _SHR_PACK_U8(buf, msg->encap_data[i]);
    }
    _SHR_PACK_U32(buf, msg->tx_port);
    _SHR_PACK_U32(buf, msg->tx_cos);
    _SHR_PACK_U32(buf, msg->tx_pri);
    _SHR_PACK_U32(buf, msg->tx_qnum);
    _SHR_PACK_U32(buf, msg->mpls_label);
    _SHR_PACK_U32(buf, msg->if_num);
    _SHR_PACK_U32(buf, msg->lm_counter_index);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_sess_set_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_sess_set_t *msg)
{
    uint16 i;

    _SHR_UNPACK_U32(buf, msg->sess_id);             
    _SHR_UNPACK_U32(buf, msg->flags);
    _SHR_UNPACK_U8(buf, msg->passive);
    _SHR_UNPACK_U8(buf, msg->mel);
    _SHR_UNPACK_U16(buf, msg->mep_id);
    for (i = 0; i < SHR_BHH_MEG_ID_LENGTH; i++) {
        _SHR_UNPACK_U8(buf, msg->meg_id[i]);
    }
    _SHR_UNPACK_U32(buf, msg->period);
    _SHR_UNPACK_U8(buf, msg->encap_type);
    _SHR_UNPACK_U32(buf, msg->encap_length);
    for (i = 0; i < SHR_BHH_MAX_ENCAP_LENGTH; i++) {
        _SHR_UNPACK_U8(buf, msg->encap_data[i]);
    }
    _SHR_UNPACK_U32(buf, msg->tx_port);
    _SHR_UNPACK_U32(buf, msg->tx_cos);
    _SHR_UNPACK_U32(buf, msg->tx_pri);
    _SHR_UNPACK_U32(buf, msg->tx_qnum);
    _SHR_UNPACK_U32(buf, msg->mpls_label);
    _SHR_UNPACK_U32(buf, msg->if_num);
    _SHR_UNPACK_U32(buf, msg->lm_counter_index);

    return buf;
}



uint8 *
shr_bhh_msg_ctrl_sess_get_pack(uint8 *buf,
                               shr_bhh_msg_ctrl_sess_get_t *msg)
{
    int i;

    _SHR_PACK_U32(buf, msg->sess_id);
    _SHR_PACK_U8(buf, msg->enable);
    _SHR_PACK_U8(buf, msg->passive);
    _SHR_PACK_U8(buf, msg->local_demand);
    _SHR_PACK_U8(buf, msg->remote_demand);
    _SHR_PACK_U8(buf, msg->local_sess_state);
    _SHR_PACK_U8(buf, msg->remote_sess_state);
    _SHR_PACK_U8(buf, msg->mel);
    _SHR_PACK_U16(buf, msg->mep_id);
    for (i = 0; i < SHR_BHH_MEG_ID_LENGTH; i++) {
        _SHR_PACK_U8(buf, msg->meg_id[i]);
    }
    _SHR_PACK_U32(buf, msg->period);
    _SHR_PACK_U8(buf, msg->encap_type);
    _SHR_PACK_U32(buf, msg->encap_length);
    for (i = 0; i < SHR_BHH_MAX_ENCAP_LENGTH; i++) {
        _SHR_PACK_U8(buf, msg->encap_data[i]);
    }
    _SHR_PACK_U32(buf, msg->tx_port);
    _SHR_PACK_U32(buf, msg->tx_cos);
    _SHR_PACK_U32(buf, msg->tx_pri);
    _SHR_PACK_U32(buf, msg->tx_qnum);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_sess_get_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_sess_get_t *msg)
{
    int i;

    _SHR_UNPACK_U32(buf, msg->sess_id);
    _SHR_UNPACK_U8(buf, msg->enable);
    _SHR_UNPACK_U8(buf, msg->passive);
    _SHR_UNPACK_U8(buf, msg->local_demand);
    _SHR_UNPACK_U8(buf, msg->remote_demand);
    _SHR_UNPACK_U8(buf, msg->local_sess_state);
    _SHR_UNPACK_U8(buf, msg->remote_sess_state);
    _SHR_UNPACK_U8(buf, msg->mel);
    _SHR_UNPACK_U16(buf, msg->mep_id);
    for (i = 0; i < SHR_BHH_MEG_ID_LENGTH; i++) {
        _SHR_UNPACK_U8(buf, msg->meg_id[i]);
    }
    _SHR_UNPACK_U32(buf, msg->period);
    _SHR_UNPACK_U8(buf, msg->encap_type);
    _SHR_UNPACK_U32(buf, msg->encap_length);
    for (i = 0; i < SHR_BHH_MAX_ENCAP_LENGTH; i++) {
        _SHR_UNPACK_U8(buf, msg->encap_data[i]);
    }
    _SHR_UNPACK_U32(buf, msg->tx_port);
    _SHR_UNPACK_U32(buf, msg->tx_cos);
    _SHR_UNPACK_U32(buf, msg->tx_pri);
    _SHR_UNPACK_U32(buf, msg->tx_qnum);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_stat_req_pack(uint8 *buf,
                               shr_bhh_msg_ctrl_stat_req_t *msg)
{
    _SHR_PACK_U32(buf, msg->sess_id);
    _SHR_PACK_U32(buf, msg->clear);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_stat_req_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_stat_req_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->sess_id);
    _SHR_UNPACK_U32(buf, msg->clear);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_stat_reply_pack(uint8 *buf,
                                 shr_bhh_msg_ctrl_stat_reply_t *msg)
{
    _SHR_PACK_U32(buf, msg->sess_id);
    _SHR_PACK_U32(buf, msg->packets_in);
    _SHR_PACK_U32(buf, msg->packets_out);
    _SHR_PACK_U32(buf, msg->packets_drop);
    _SHR_PACK_U32(buf, msg->packets_auth_drop);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_stat_reply_unpack(uint8 *buf,
                                   shr_bhh_msg_ctrl_stat_reply_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->sess_id);
    _SHR_UNPACK_U32(buf, msg->packets_in);
    _SHR_UNPACK_U32(buf, msg->packets_out);
    _SHR_UNPACK_U32(buf, msg->packets_drop);
    _SHR_UNPACK_U32(buf, msg->packets_auth_drop);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loopback_add_pack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loopback_add_t *msg)
{
    _SHR_PACK_U32(buf, msg->flags);
    _SHR_PACK_U32(buf, msg->sess_id);
    _SHR_PACK_U32(buf, msg->period);
    _SHR_PACK_U32(buf, msg->ttl);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loopback_add_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loopback_add_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->flags);
    _SHR_UNPACK_U32(buf, msg->sess_id);
    _SHR_UNPACK_U32(buf, msg->period);
    _SHR_UNPACK_U32(buf, msg->ttl);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loopback_delete_pack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loopback_delete_t *msg)
{
    _SHR_PACK_U32(buf, msg->sess_id);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loopback_delete_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loopback_delete_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->sess_id);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loopback_get_pack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loopback_get_t *msg)
{
    _SHR_PACK_U32(buf, msg->flags);
    _SHR_PACK_U32(buf, msg->sess_id);
    _SHR_PACK_U32(buf, msg->period);
    _SHR_PACK_U32(buf, msg->ttl);
    _SHR_PACK_U32(buf, msg->discovery_flags);
    _SHR_PACK_U32(buf, msg->discovery_id);
    _SHR_PACK_U32(buf, msg->discovery_ttl);
    _SHR_PACK_U32(buf, msg->rx_count);
    _SHR_PACK_U32(buf, msg->tx_count);
    _SHR_PACK_U32(buf, msg->drop_count);
    _SHR_PACK_U32(buf, msg->unexpected_response);
    _SHR_PACK_U32(buf, msg->out_of_sequence);
    _SHR_PACK_U32(buf, msg->local_mipid_missmatch);
    _SHR_PACK_U32(buf, msg->remote_mipid_missmatch);
    _SHR_PACK_U32(buf, msg->invalid_target_mep_tlv);
    _SHR_PACK_U32(buf, msg->invalid_mep_tlv_subtype);
    _SHR_PACK_U32(buf, msg->invalid_tlv_offset);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loopback_get_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loopback_get_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->flags);
    _SHR_UNPACK_U32(buf, msg->sess_id);
    _SHR_UNPACK_U32(buf, msg->period);
    _SHR_UNPACK_U32(buf, msg->ttl);
    _SHR_UNPACK_U32(buf, msg->discovery_flags);
    _SHR_UNPACK_U32(buf, msg->discovery_id);
    _SHR_UNPACK_U32(buf, msg->discovery_ttl);
    _SHR_UNPACK_U32(buf, msg->rx_count);
    _SHR_UNPACK_U32(buf, msg->tx_count);
    _SHR_UNPACK_U32(buf, msg->drop_count);
    _SHR_UNPACK_U32(buf, msg->unexpected_response);
    _SHR_UNPACK_U32(buf, msg->out_of_sequence);
    _SHR_UNPACK_U32(buf, msg->local_mipid_missmatch);
    _SHR_UNPACK_U32(buf, msg->remote_mipid_missmatch);
    _SHR_UNPACK_U32(buf, msg->invalid_target_mep_tlv);
    _SHR_UNPACK_U32(buf, msg->invalid_mep_tlv_subtype);
    _SHR_UNPACK_U32(buf, msg->invalid_tlv_offset);
    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loss_add_pack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loss_add_t *msg)
{
    _SHR_PACK_U32(buf, msg->flags);
    _SHR_PACK_U32(buf, msg->sess_id);
    _SHR_PACK_U32(buf, msg->period);
    _SHR_PACK_U32(buf, msg->int_pri);
    _SHR_PACK_U8 (buf, msg->pkt_pri);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loss_add_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loss_add_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->flags);
    _SHR_UNPACK_U32(buf, msg->sess_id);
    _SHR_UNPACK_U32(buf, msg->period);
    _SHR_UNPACK_U32(buf, msg->int_pri);
    _SHR_UNPACK_U8 (buf, msg->pkt_pri);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loss_delete_pack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loss_delete_t *msg)
{
    _SHR_PACK_U32(buf, msg->sess_id);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loss_delete_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loss_delete_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->sess_id);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loss_get_pack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loss_get_t *msg)
{
    _SHR_PACK_U32(buf, msg->flags);
    _SHR_PACK_U32(buf, msg->sess_id);
    _SHR_PACK_U32(buf, msg->period);
    _SHR_PACK_U32(buf, msg->loss_threshold);
    _SHR_PACK_U32(buf, msg->loss_nearend);
    _SHR_PACK_U32(buf, msg->tx_nearend);
    _SHR_PACK_U32(buf, msg->rx_nearend);
    _SHR_PACK_U32(buf, msg->tx_farend);
    _SHR_PACK_U32(buf, msg->rx_farend);
    _SHR_PACK_U32(buf, msg->rx_oam_packets);
    _SHR_PACK_U32(buf, msg->tx_oam_packets);
    _SHR_PACK_U32(buf, msg->int_pri);
    _SHR_PACK_U8 (buf, msg->pkt_pri);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_loss_get_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_loss_get_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->flags);
    _SHR_UNPACK_U32(buf, msg->sess_id);
    _SHR_UNPACK_U32(buf, msg->period);
    _SHR_UNPACK_U32(buf, msg->loss_threshold);
    _SHR_UNPACK_U32(buf, msg->loss_nearend);
    _SHR_UNPACK_U32(buf, msg->tx_nearend);
    _SHR_UNPACK_U32(buf, msg->rx_nearend);
    _SHR_UNPACK_U32(buf, msg->tx_farend);
    _SHR_UNPACK_U32(buf, msg->rx_farend);
    _SHR_UNPACK_U32(buf, msg->rx_oam_packets);
    _SHR_UNPACK_U32(buf, msg->tx_oam_packets);
    _SHR_UNPACK_U32(buf, msg->int_pri);
    _SHR_UNPACK_U8 (buf, msg->pkt_pri);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_delay_add_pack(uint8 *buf,
                                 shr_bhh_msg_ctrl_delay_add_t *msg)
{
    _SHR_PACK_U32(buf, msg->flags);
    _SHR_PACK_U32(buf, msg->sess_id);
    _SHR_PACK_U32(buf, msg->period);
    _SHR_PACK_U32(buf, msg->int_pri);
    _SHR_PACK_U8 (buf, msg->pkt_pri);
    _SHR_PACK_U8 (buf, msg->dm_format);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_delay_add_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_delay_add_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->flags);
    _SHR_UNPACK_U32(buf, msg->sess_id);
    _SHR_UNPACK_U32(buf, msg->period);
    _SHR_UNPACK_U32(buf, msg->int_pri);
    _SHR_UNPACK_U8 (buf, msg->pkt_pri);
    _SHR_UNPACK_U8 (buf, msg->dm_format);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_delay_delete_pack(uint8 *buf,
                                 shr_bhh_msg_ctrl_delay_delete_t *msg)
{
    _SHR_PACK_U32(buf, msg->sess_id);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_delay_delete_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_delay_delete_t *msg)
{
    _SHR_UNPACK_U32(buf, msg->sess_id);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_delay_get_pack(uint8 *buf,
                                 shr_bhh_msg_ctrl_delay_get_t *msg)
{

    _SHR_PACK_U32(buf, msg->flags);
    _SHR_PACK_U32(buf, msg->sess_id);
    _SHR_PACK_U32(buf, msg->period);
    _SHR_PACK_U32(buf, msg->delay_seconds);
    _SHR_PACK_U32(buf, msg->delay_nanoseconds);
    _SHR_PACK_U32(buf, msg->txf_seconds);
    _SHR_PACK_U32(buf, msg->txf_nanoseconds);
    _SHR_PACK_U32(buf, msg->rxf_seconds);
    _SHR_PACK_U32(buf, msg->rxf_nanoseconds);
    _SHR_PACK_U32(buf, msg->txb_seconds);
    _SHR_PACK_U32(buf, msg->txb_nanoseconds);
    _SHR_PACK_U32(buf, msg->rxb_seconds);
    _SHR_PACK_U32(buf, msg->rxb_nanoseconds);
    _SHR_PACK_U32(buf, msg->rx_oam_packets);
    _SHR_PACK_U32(buf, msg->tx_oam_packets);
    _SHR_PACK_U32(buf, msg->int_pri);
    _SHR_PACK_U8 (buf, msg->pkt_pri);
    _SHR_PACK_U8 (buf, msg->dm_format);

    return buf;
}

uint8 *
shr_bhh_msg_ctrl_delay_get_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_delay_get_t *msg)
{

    _SHR_UNPACK_U32(buf, msg->flags);
    _SHR_UNPACK_U32(buf, msg->sess_id);
    _SHR_UNPACK_U32(buf, msg->period);
    _SHR_UNPACK_U32(buf, msg->delay_seconds);
    _SHR_UNPACK_U32(buf, msg->delay_nanoseconds);
    _SHR_UNPACK_U32(buf, msg->txf_seconds);
    _SHR_UNPACK_U32(buf, msg->txf_nanoseconds);
    _SHR_UNPACK_U32(buf, msg->rxf_seconds);
    _SHR_UNPACK_U32(buf, msg->rxf_nanoseconds);
    _SHR_UNPACK_U32(buf, msg->txb_seconds);
    _SHR_UNPACK_U32(buf, msg->txb_nanoseconds);
    _SHR_UNPACK_U32(buf, msg->rxb_seconds);
    _SHR_UNPACK_U32(buf, msg->rxb_nanoseconds);
    _SHR_UNPACK_U32(buf, msg->rx_oam_packets);
    _SHR_UNPACK_U32(buf, msg->tx_oam_packets);
    _SHR_UNPACK_U32(buf, msg->int_pri);
    _SHR_UNPACK_U8 (buf, msg->pkt_pri);
    _SHR_UNPACK_U8 (buf, msg->dm_format);

    return buf;
}

/*********************************************************
 * BHH Network Packet Header Pack/Unpack
 *
 * Functions:
 *      shr_bhh_<header>_pack/unpack
 * Purpose:
 *      The following set of routines pack/unpack specified
 *      network packet header to/from given buffer.
 * Parameters:
 *   _pack()
 *      buffer   - (OUT) Buffer where to pack header.
 *      <header> - (IN)  Header to pack.
 *   _unpack()
 *      buffer   - (IN)  Buffer with data to unpack.
 *      <header> - (OUT) Returns unpack header.
 * Returns:
 *      Pointer to next position in buffer.
 * Notes:
 *      Assumes pointers are valid and contain enough memory space.
 */
uint8 *
shr_bhh_header_pack(uint8 *buf, shr_bhh_header_t *bhh)
{
    uint32 *ptr;

    ptr = (uint32 *)bhh;

    _SHR_PACK_U32(buf, ptr[0]);
#if 0
    _SHR_PACK_U8(buf, bhh->opcode);
    _SHR_PACK_U8(buf, bhh->flags);
    _SHR_PACK_U8(buf, bhh->tlv_offset);
#endif
    return buf;
}

uint8 *
shr_bhh_header_unpack(uint8 *buf, shr_bhh_header_t *bhh)
{
    uint32 *ptr;

    ptr = (uint32 *)bhh;

    _SHR_UNPACK_U32(buf, ptr[0]);
#if 0
    uint8 *byte;

    byte = (uint8 *)bhh;

    _SHR_UNPACK_U8(buf, byte[0]);
    _SHR_UNPACK_U8(buf, bhh->opcode);
    _SHR_UNPACK_U8(buf, bhh->flags);
    _SHR_UNPACK_U8(buf, bhh->tlv_offset);
#endif
    return buf;
}

/*********************************************************
 * BHH TLV Pack/Unpack
 *
 * Functions:
 *      
 * Purpose:
 *      The following set of routines pack/unpack specified
 *      
 * Parameters:
 *   _pack()
 *      buffer   - (OUT) Buffer where to pack header.
 *   
 *   _unpack()
 *      buffer   - (IN)  Buffer with data to unpack.
 *   
 * Returns:
 *   
 * Notes:
 *   
 */
uint8 *
shr_bhh_tlv_pack(uint8 *buf, shr_bhh_tlv_t *tlv)
{
    int i;

    _SHR_PACK_U8(buf, tlv->type);

    if (tlv->type == SHR_BHH_TLV_END) {
        tlv->length = 0;
        tlv->subtype = 0;
    } else {
        _SHR_PACK_U16(buf, tlv->length);
        _SHR_PACK_U8(buf, tlv->subtype);

        if (tlv->type == SHR_BHH_TLV_REQUESTING_MEP) {
            _SHR_PACK_U16(buf, tlv->value.requesting.mep_id);
            for (i = 0;i < sizeof(tlv->value.requesting.meg_id);i++) {
                _SHR_PACK_U8(buf, tlv->value.requesting.meg_id[i]);
            }
            _SHR_PACK_U16(buf, 0x0000);
        } else if (tlv->type == SHR_BHH_TLV_TARGET_MEP ||
                   tlv->type == SHR_BHH_TLV_REPLYING_MEP) { 
            switch (tlv->subtype) {
            case SHR_BHH_TLV_MEP_SUB_INGRESS_DISCOVERY:
            case SHR_BHH_TLV_MEP_SUB_EGRESS_DISCOVERY:
                for (i = 0;i < SHR_BHH_ID_ZERO_LEN;i++) {
                    _SHR_PACK_U8(buf, 0);
                }
                break;

            case SHR_BHH_TLV_MEP_SUB_MEP_ICC_ID:
            {
                _SHR_PACK_U16(buf, tlv->value.reply_icc.mep_id);
                for (i = 0;i < sizeof(tlv->value.reply_icc.zero);i++) {
                    _SHR_PACK_U8(buf, 0);
                }       
            }
            break;

            case SHR_BHH_TLV_MEP_SUB_MIP_ICC_ID:
            {
                for (i = 0;i < sizeof(tlv->value.reply_mip_icc.carrier_code);i++) {
                    _SHR_PACK_U8(buf, tlv->value.reply_mip_icc.carrier_code[i]);
                }
                _SHR_PACK_U32(buf, tlv->value.reply_mip_icc.node_id);
                _SHR_PACK_U32(buf, tlv->value.reply_mip_icc.if_num);
                for (i = 0;i < sizeof(tlv->value.reply_mip_icc.zero);i++) {
                    _SHR_PACK_U8(buf, 0);
                }
            }
            break;
            }
        } else {
            ; 
        }
    }

    return buf;
}



uint8 *
shr_bhh_tlv_unpack(uint8 *buf, shr_bhh_tlv_t *tlv)
{
    int i;

    _SHR_UNPACK_U8(buf, tlv->type);
    _SHR_UNPACK_U16(buf, tlv->length);
    _SHR_UNPACK_U8(buf, tlv->subtype);

    if (tlv->type == SHR_BHH_TLV_REQUESTING_MEP) {
        _SHR_UNPACK_U16(buf, tlv->value.requesting.mep_id);
        for (i = 0;i < sizeof(tlv->value.requesting.meg_id);i++) {
            _SHR_UNPACK_U8(buf, tlv->value.requesting.meg_id[i]);
        }
        _SHR_UNPACK_U16(buf, tlv->value.requesting.reserved);
    } else if (tlv->type == SHR_BHH_TLV_TARGET_MEP ||
               tlv->type == SHR_BHH_TLV_REPLYING_MEP) { 
        switch (tlv->subtype) {
        case SHR_BHH_TLV_MEP_SUB_INGRESS_DISCOVERY:
        case SHR_BHH_TLV_MEP_SUB_EGRESS_DISCOVERY:
        {
            for (i = 0;i < SHR_BHH_ID_ZERO_LEN;i++) {
                _SHR_UNPACK_U8(buf, tlv->value.discovery.zero[i]);
            }
        }
        break;

        case SHR_BHH_TLV_MEP_SUB_MEP_ICC_ID:
        {
            _SHR_UNPACK_U16(buf, tlv->value.reply_icc.mep_id);
            for (i = 0;i < sizeof(tlv->value.reply_icc.zero);i++) {
                _SHR_UNPACK_U8(buf, tlv->value.reply_icc.zero[i]);
            }   
        }
        break;

        case SHR_BHH_TLV_MEP_SUB_MIP_ICC_ID:
        {
            for (i = 0;i < sizeof(tlv->value.reply_mip_icc.carrier_code);i++) {
                _SHR_UNPACK_U8(buf, tlv->value.reply_mip_icc.carrier_code[i]);
            }
            _SHR_UNPACK_U32(buf, tlv->value.reply_mip_icc.node_id);
            _SHR_UNPACK_U32(buf, tlv->value.reply_mip_icc.if_num);
            for (i = 0;i < sizeof(tlv->value.reply_mip_icc.zero);i++) {
                _SHR_UNPACK_U8(buf, tlv->value.reply_mip_icc.zero[i]);
            }
        }
        break;
        }
    } else {
        ; 
    }

    return buf;
}

