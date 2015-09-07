/* 
 * $Id: brd_ident.h,v 1.10 Broadcom SDK $
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
 * File:        brd_ident.h
 * Purpose:     CPUDB base board identifiers
 */

#ifndef   _BRD_IDENT_H_
#define   _BRD_IDENT_H_

typedef enum {
    cpudb_board_id_unknown,
    cpudb_board_id_cfm_xgs1,
    cpudb_board_id_cfm_xgs2,
    cpudb_board_id_draco_b2b,
    cpudb_board_id_galahad,
    cpudb_board_id_guenevere,
    cpudb_board_id_lancelot,
    cpudb_board_id_lm24g,
    cpudb_board_id_lm2x,
    cpudb_board_id_lm6x,
    cpudb_board_id_lm_xgs1_6x,
    cpudb_board_id_lm_xgs2_48g,
    cpudb_board_id_lm_xgs2_6x,
    cpudb_board_id_lm_xgs3_12x,
    cpudb_board_id_lm_xgs3_48g,
    cpudb_board_id_lynxalot,
    cpudb_board_id_magnum,
    cpudb_board_id_merlin,
    cpudb_board_id_sdk_xgs2_12g,
    cpudb_board_id_sdk_xgs3_12g,
    cpudb_board_id_sdk_xgs3_24g,
    cpudb_board_id_sl24f2g,
    cpudb_board_id_tucana,
    cpudb_board_id_white_knight,
    cpudb_board_id_cfm_xgs3,
    cpudb_board_id_lm_56800_12x,
    cpudb_board_id_sdk_xgs3_48f,
    cpudb_board_id_sdk_xgs3_48g,
    cpudb_board_id_sdk_xgs3_20x,
    cpudb_board_id_sdk_xgs3_16h,
    cpudb_board_id_sdk_xgs3_48g5g,
    cpudb_board_id_sdk_xgs3_48g2,
    cpudb_board_id_count /* last */
} CPUDB_BOARD_ID;

#endif /* _BRD_IDENT_H_ */
