/* 
 * $Id: bfd_pack.h,v 1.3 Broadcom SDK $
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
 * File:        bfd_pack.h
 * Purpose:     Interface to pack and unpack routines common to
 *              SDK and uKernel for:
 *              - BFD Control messages
 *              - Network Packet headers
 *
 *              This is to be shared between SDK host and uKernel.
 */

#ifndef   _SOC_SHARED_BFD_PACK_H_
#define   _SOC_SHARED_BFD_PACK_H_

#ifdef BCM_UKERNEL
  /* Build for uKernel not SDK */
  #include "sdk_typedefs.h"
#else
  #include <sal/types.h>
#endif

#include <soc/shared/bfd_msg.h>
#include <soc/shared/bfd_pkt.h>

/* BFD Control Messages pack/unpack */
extern uint8 *
shr_bfd_msg_ctrl_init_pack(uint8 *buf, shr_bfd_msg_ctrl_init_t *msg);
extern uint8 *
shr_bfd_msg_ctrl_init_unpack(uint8 *buf, shr_bfd_msg_ctrl_init_t *msg);

extern uint8 *
shr_bfd_msg_ctrl_sess_set_pack(uint8 *buf,
                               shr_bfd_msg_ctrl_sess_set_t *msg);
extern uint8 *
shr_bfd_msg_ctrl_sess_set_unpack(uint8 *buf,
                                 shr_bfd_msg_ctrl_sess_set_t *msg);

extern uint8 *
shr_bfd_msg_ctrl_sess_get_pack(uint8 *buf,
                               shr_bfd_msg_ctrl_sess_get_t *msg);
extern uint8 *
shr_bfd_msg_ctrl_sess_get_unpack(uint8 *buf,
                                 shr_bfd_msg_ctrl_sess_get_t *msg);

extern uint8 *
shr_bfd_msg_ctrl_auth_sha1_pack(uint8 *buf,
                                shr_bfd_msg_ctrl_auth_sha1_t *msg);
extern uint8 *
shr_bfd_msg_ctrl_auth_sha1_unpack(uint8 *buf,
                                  shr_bfd_msg_ctrl_auth_sha1_t *msg);

extern uint8 *
shr_bfd_msg_ctrl_auth_sp_pack(uint8 *buf, shr_bfd_msg_ctrl_auth_sp_t *msg);
extern uint8 *
shr_bfd_msg_ctrl_auth_sp_unpack(uint8 *buf, shr_bfd_msg_ctrl_auth_sp_t *msg);

extern uint8 *
shr_bfd_msg_ctrl_stat_req_pack(uint8 *buf,
                               shr_bfd_msg_ctrl_stat_req_t *msg);
extern uint8 *
shr_bfd_msg_ctrl_stat_req_unpack(uint8 *buf,
                                 shr_bfd_msg_ctrl_stat_req_t *msg);

extern uint8 *
shr_bfd_msg_ctrl_stat_reply_pack(uint8 *buf,
                                 shr_bfd_msg_ctrl_stat_reply_t *msg);
extern uint8 *
shr_bfd_msg_ctrl_stat_reply_unpack(uint8 *buf,
                                   shr_bfd_msg_ctrl_stat_reply_t *msg);

/* BFD Network Packet headers pack/unpack */
extern uint8 *
shr_bfd_header_pack(uint8 *buf, shr_bfd_header_t *bfd);
extern uint8 *
shr_bfd_header_unpack(uint8 *buf, shr_bfd_header_t *bfd);

extern uint8 *
shr_bfd_auth_header_pack(uint8 *buf, shr_bfd_auth_header_t *auth);
extern uint8 *
shr_bfd_auth_header_unpack(uint8 *buf, shr_bfd_auth_header_t *auth);

#endif /* _SOC_SHARED_BFD_PACK_H_ */
