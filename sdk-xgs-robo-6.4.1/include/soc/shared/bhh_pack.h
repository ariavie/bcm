/* 
 * $Id: bhh_pack.h,v 1.9 Broadcom SDK $
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
 * File:        bhh_pack.h
 * Purpose:     Interface to pack and unpack routines common to
 *              SDK and uKernel for:
 *              - BHH Control messages
 *              - Network Packet headers
 *
 *              This is to be shared between SDK host and uKernel.
 */

#ifndef   _SOC_SHARED_BHH_PACK_H_
#define   _SOC_SHARED_BHH_PACK_H_

#ifdef BCM_UKERNEL
  /* Build for uKernel not SDK */
  #include "sdk_typedefs.h"
#else
  #include <sal/types.h>
#endif

#include <soc/shared/bhh_msg.h>
#include <soc/shared/bhh_pkt.h>

/*
 * TLV Types
 */
#define SHR_BHH_TLV_END            0x00
#define SHR_BHH_TLV_TARGET_MEP     0x21
#define SHR_BHH_TLV_REPLYING_MEP   0x22
#define SHR_BHH_TLV_REQUESTING_MEP 0x23

/*
 * TLV Subtypes
 */
#define SHR_BHH_TLV_MEP_SUB_INGRESS_DISCOVERY 0x00
#define SHR_BHH_TLV_MEP_SUB_EGRESS_DISCOVERY  0x01
#define SHR_BHH_TLV_MEP_SUB_MEP_ICC_ID        0x02
#define SHR_BHH_TLV_MEP_SUB_MIP_ICC_ID        0x03

#define SHR_BHH_ID_ZERO_LEN 24


typedef struct {
    uint8 zero[24];
} bhh_discovery_id_t;

typedef struct {
    uint16 mep_id;
    uint8 zero[22];
} bhh_reply_icc_t;

typedef struct {
    uint8 carrier_code[6];
    uint32 node_id;
    uint32 if_num;
    uint8  zero[10];
} bhh_reply_mip_icc_t;

typedef struct {
    uint16 mep_id;
    uint8  meg_id[48];
    uint16 reserved;
} bhh_requesting_mep_t;

#define SHR_BHH_TLV_MAX_VALUE_LEN sizeof(bhh_requesting_mep_t)

typedef struct {
    uint8  type;
    uint16 length;
    uint8  subtype;
    union {
	uint8 data[SHR_BHH_TLV_MAX_VALUE_LEN];
	bhh_discovery_id_t   discovery;
	bhh_reply_icc_t      reply_icc;
	bhh_reply_mip_icc_t  reply_mip_icc;
	bhh_requesting_mep_t requesting;
    } value;
} shr_bhh_tlv_t;

/* BHH Control Messages pack/unpack */
extern uint8 *
shr_bhh_msg_ctrl_init_pack(uint8 *buf, shr_bhh_msg_ctrl_init_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_init_unpack(uint8 *buf, shr_bhh_msg_ctrl_init_t *msg);

extern uint8 *
shr_bhh_msg_ctrl_sess_set_pack(uint8 *buf,
                               shr_bhh_msg_ctrl_sess_set_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_sess_set_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_sess_set_t *msg);

extern uint8 *
shr_bhh_msg_ctrl_sess_get_pack(uint8 *buf,
                               shr_bhh_msg_ctrl_sess_get_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_sess_get_unpack(uint8 *buf,
				 shr_bhh_msg_ctrl_sess_get_t *msg); 
extern uint8 *
shr_bhh_msg_ctrl_sess_enable_pack(uint8 *buf,
                               shr_bhh_msg_ctrl_sess_enable_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_sess_enable_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_sess_enable_t *msg);

extern uint8 *
shr_bhh_msg_ctrl_sess_delete_pack(uint8 *buf,
				  shr_bhh_msg_ctrl_sess_delete_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_sess_delete_unpack(uint8 *buf,
				    shr_bhh_msg_ctrl_sess_delete_t *msg);

extern uint8 *
shr_bhh_msg_ctrl_stat_req_pack(uint8 *buf,
                               shr_bhh_msg_ctrl_stat_req_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_stat_req_unpack(uint8 *buf,
                                 shr_bhh_msg_ctrl_stat_req_t *msg);

extern uint8 *
shr_bhh_msg_ctrl_stat_reply_pack(uint8 *buf,
                                 shr_bhh_msg_ctrl_stat_reply_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_stat_reply_unpack(uint8 *buf,
                                   shr_bhh_msg_ctrl_stat_reply_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loopback_add_pack(uint8 *buf,
				   shr_bhh_msg_ctrl_loopback_add_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loopback_add_unpack(uint8 *buf,
				     shr_bhh_msg_ctrl_loopback_add_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loopback_delete_pack(uint8 *buf,
				   shr_bhh_msg_ctrl_loopback_delete_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loopback_delete_unpack(uint8 *buf,
				     shr_bhh_msg_ctrl_loopback_delete_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loopback_get_pack(uint8 *buf,
				   shr_bhh_msg_ctrl_loopback_get_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loopback_get_unpack(uint8 *buf,
				     shr_bhh_msg_ctrl_loopback_get_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loss_add_pack(uint8 *buf,
				   shr_bhh_msg_ctrl_loss_add_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loss_add_unpack(uint8 *buf,
				     shr_bhh_msg_ctrl_loss_add_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loss_delete_pack(uint8 *buf,
				   shr_bhh_msg_ctrl_loss_delete_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loss_delete_unpack(uint8 *buf,
				     shr_bhh_msg_ctrl_loss_delete_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loss_get_pack(uint8 *buf,
				   shr_bhh_msg_ctrl_loss_get_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_loss_get_unpack(uint8 *buf,
				     shr_bhh_msg_ctrl_loss_get_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_delay_add_pack(uint8 *buf,
				   shr_bhh_msg_ctrl_delay_add_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_delay_add_unpack(uint8 *buf,
				     shr_bhh_msg_ctrl_delay_add_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_delay_delete_pack(uint8 *buf,
				   shr_bhh_msg_ctrl_delay_delete_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_delay_delete_unpack(uint8 *buf,
				     shr_bhh_msg_ctrl_delay_delete_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_delay_get_pack(uint8 *buf,
				   shr_bhh_msg_ctrl_delay_get_t *msg);
extern uint8 *
shr_bhh_msg_ctrl_delay_get_unpack(uint8 *buf,
				     shr_bhh_msg_ctrl_delay_get_t *msg);


/* BHH Network Packet headers pack/unpack */
extern uint8 *
shr_bhh_header_pack(uint8 *buf, shr_bhh_header_t *bhh);
extern uint8 *
shr_bhh_header_unpack(uint8 *buf, shr_bhh_header_t *bhh);

extern uint8 *
shr_bhh_tlv_unpack(uint8 *buf, shr_bhh_tlv_t *tlv);
extern uint8 *
shr_bhh_tlv_pack(uint8 *buf, shr_bhh_tlv_t *tlv);

#endif /* _SOC_SHARED_BHH_PACK_H_ */
