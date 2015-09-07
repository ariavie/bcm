/*
 * $Id: pae.h,v 1.0 Broadcom SDK $
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
 * This file contains pae definitions internal to the BCM library.
 */
#ifndef _SOC_ROBO_PAE_H
#define _SOC_ROBO_PAE_H

#ifdef BCM_NORTHSTARPLUS_SUPPORT
#ifdef LINUX
#if !defined(__KERNEL__)

#define PAE_RES_WAIT_MS   50
/*the database configuration, use only one database*/
#define PAE_DB_ENABLE_FLAG 1
#define PAE_DB_NUMBER      0
#define PAE_RULE_SET       32
#define PAE_RULE_BASE      0
#define PAE_META_BASE      0
#define PAE_PAYLOAD_SIZE   8*8
#define PAE_KEY_TYPE       5  /*58 bits*/
#define PAE_KEY_SIZE       (PAE_KEY_TYPE*12-2)

#define PAE_PACKET_BUFFER_BASE 0x40050000
#define PAE_SCRATCH_PAD_START_BYTE  (0xD00*8)
#define PAE_SCRATCH_PAD_END_BYTE    (0xFFF*8)
#define PAE_SCRATCH_PAD_START_ADDR  (PAE_PACKET_BUFFER_BASE+PAE_SCRATCH_PAD_START_BYTE)
#define PAE_SCRATCH_PAD_END_ADDR  (PAE_PACKET_BUFFER_BASE+PAE_SCRATCH_PAD_END_BYTE)

typedef enum {
    PAE_RES_INVALID = 0xFFFFFFFF,
    PAE_RES_OK= 0,
    PAE_RES_FAILURE
} pae_response_code;

typedef enum {
    PAE_NO_ACTION = 0,
    PAE_READ_32,
    PAE_READ_64,
    PAE_WRITE_32,
    PAE_WRITE_64,
    PAE_WRITE_BUF,
    PAE_READ_BUF
} pae_msg_type;

int
soc_pae_lue_db_config(
            int    unit,
            uint32 db_num,
            uint32 enabled,
            uint32 rule_sets,
            uint32 rule_base,
            uint32 meta_base,
            uint32 key_size_encoded,
            uint32 payload_size);

int
soc_pae_lue_idx_alloc(
            int    unit,
            uint32 db_num, 
            uint32 *idx);

int
soc_pae_lue_idx_free(
            int    unit,
            uint32 db_num, 
            uint32 idx);

int
soc_pae_lue_rule_add(
            int    unit,
            uint32 db_num,
            uint8  *pat_p,
            uint32 pat_nbits,
            uint8  *pay_p,
            uint32 rule_idx);


int
soc_pae_lue_rule_del( 
            int    unit,
            uint32 db_num, 
            uint32 rule_idx);

uint32 
soc_pae_get_sratch_pad_entry_addr(
            int unit, 
            uint32 idx);

pae_response_code 
soc_pae_get_cmd_response(int unit);

int 
soc_pae_set_cmd_response(int unit, pae_response_code response_code);

int 
soc_pae_send_msg(int unit, pae_msg_type msg_type, uint32 addr, uint32 len, void * buf);

#endif
#endif
#endif

#endif


