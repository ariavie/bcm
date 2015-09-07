/*
 * $Id: 4399c587a6b4bb358faffcbdad5832fec65c590f $ 
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
 */

#ifndef _temod_sc_lkup_table_DEFINES_H_
#define _temod_sc_lkup_table_DEFINES_H_

/**
\struct sc_table_entry_t

This embodies all fields in the PHY hard table (HT). For details
please refer to the micro-arch document.
*/
typedef struct sc_table_entry_t
{
  int num_lanes;
  int t_pma_os_mode;
  int t_scr_mode;
  int t_encode_mode;
  int cl48_check_end;
  int blk_sync_mode;
  int r_reorder_mode;
  int cl36_en;

  int r_descr1_mode;
  int r_dec1_mode;
  int r_deskew_mode;
  int r_desc2_mode;
  int r_desc2_byte_deletion;
  int r_dec1_brcm64b66_descr;
/*
  Notes:
    1. When the loop_cnt1		== 0, the clk_cnt1 is a don't care
    2. the pcs crd/cnt are valid only when byte replication is valid, say for 10M, 100M cases
*/
  int pll_mode;
  int sgmii;
  int clkcnt0;
  int clkcnt1;
  int lpcnt0;
  int lpcnt1;
  int mac_cgc;
  int pcs_repcnt;
  int pcs_crdten;
  int pcs_clkcnt;
  int pcs_cgc;
  int cl72_en;

} sc_table_entry;

#ifdef _DV_TB_
/* theoretical max is 256. There are around 55 now.  */
static sc_table_entry sc_ht_entry[] = {

/*digital_operationSpeeds_SPEED_10M :                     0*/  { 0, 5, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 1, 0, TEMOD_PLL_MODE_DIV_40, 1, 2500, 0, 1, 0, 2499, 1, 1, 25, 24 ,0}, 
/*digital_operationSpeeds_SPEED_10M :                     1*/  { 0, 5, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 1, 0, TEMOD_PLL_MODE_DIV_40, 1, 2500, 0, 1, 0, 2499, 1, 1, 25, 24,0 }, 
/*digital_operationSpeeds_SPEED_100M :                    2*/  { 0, 5, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 0, 0, TEMOD_PLL_MODE_DIV_40, 1, 250, 0, 1, 0, 249, 0, 1, 25, 24 ,0}, 
/*digital_operationSpeeds_SPEED_1000M :                   3*/  { 0, 5, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 25, 0, 1, 0, 24, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_1G_CX1 :TBD               4*/  { 0, 5, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 165, 0, 1, 0, 41, 0, 0, 0, 0,0 },
/*digital_operationSpeeds_SPEED_1G_KX1 :                  5*/  { 0, 8, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 165, 0, 1, 0, 41, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_2p5G :                    6*/  { 0, 1, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 10, 0, 1, 0, 6, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_5G_X1 :                   7*/  { 0, 0, 2, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_10B, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 5, 0, 1, 0, 3, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_10G_CX4 :                 8*/  { 2, 1, 0, TEMOD_ENCODE_MODE_CL48, 1, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 5, 0, 1, 0, 2, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_10G_KX4:                  9*/  { 2, 1, 0, TEMOD_ENCODE_MODE_CL48, 1, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 5, 0, 1, 0, 2, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_10G_X4:              10/0xa*/  { 2, 1, 0, TEMOD_ENCODE_MODE_CL48, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 5, 0, 1, 0, 2, 0, 0, 0, 0,0 },
/*digital_operationSpeeds_SPEED_13G_X4 :             11/0xb*/  { 2, 1, 0, TEMOD_ENCODE_MODE_CL48, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_52, 0, 5, 0, 1, 0, 2, 0, 0, 0, 0,0 },
/*digital_operationSpeeds_SPEED_15G_X4 :             12/0xc*/  { 2, 1, 0, TEMOD_ENCODE_MODE_CL48, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_60, 0, 5, 0, 1, 0, 2, 0, 0, 0, 0,0 },
/*digital_operationSpeeds_SPEED_16G_X4 :             13/0xd*/  { 2, 1, 0, TEMOD_ENCODE_MODE_CL48, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_64, 0, 5, 0, 1, 0, 2, 0, 0, 0, 0,0 },
/*digital_operationSpeeds_SPEED_20G_CX4:             14/0xe*/  { 2, 0, 0, TEMOD_ENCODE_MODE_CL48, 1, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 5, 0, 1, 0, 1, 0, 0, 0, 0,0 },
/*digital_operationSpeeds_SPEED_10G_CX2 :            15/0xf*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL48_2_LANE, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_10B, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 5, 0, 1, 0, 2, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_10G_X2 :            16/0x10*/  { 1, 0, 2, TEMOD_ENCODE_MODE_CL48_2_LANE, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_10B, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 5, 0, 1, 0, 2, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_20G_X4:             17/0x11*/  { 2, 0, 2, TEMOD_ENCODE_MODE_CL48, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_10B, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 5, 0, 1, 0, 1, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_10p5G_X2:           18/0x12*/  { 1, 0, 2, TEMOD_ENCODE_MODE_CL48_2_LANE, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_10B, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_42, 0, 5, 0, 1, 0, 2, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_21G_X4:             19/0x13*/  { 2, 0, 2, TEMOD_ENCODE_MODE_CL48, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_10B,    TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_42, 0, 5, 0, 1, 0, 1, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_12p7G_X2 :          20/0x14*/  { 1, 0, 1, TEMOD_ENCODE_MODE_BRCM, 0, TEMOD_BLOCKSYNC_MODE_BRCM, 1, 0, TEMOD_R_DESCR1_MODE_66B, TEMOD_DECODER_MODE_BRCM, TEMOD_R_DESKEW_MODE_BRCM_66B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_42, 0, 33, 0, 1, 0, 2, 0, 0, 0, 0,0 },
/*digital_operationSpeeds_SPEED_25p45G_X4:          21/0x15*/  { 2, 0, 1, TEMOD_ENCODE_MODE_BRCM, 0, TEMOD_BLOCKSYNC_MODE_BRCM, 1, 0, TEMOD_R_DESCR1_MODE_66B, TEMOD_DECODER_MODE_BRCM, TEMOD_R_DESKEW_MODE_BRCM_66B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_42, 0, 33, 0, 1, 0, 1, 0, 0, 0, 0,0 },
/*digital_operationSpeeds_SPEED_15p75G_X2           22/0x16*/  { 1, 0, 1, TEMOD_ENCODE_MODE_BRCM, 0, TEMOD_BLOCKSYNC_MODE_BRCM, 1, 0, TEMOD_R_DESCR1_MODE_66B, TEMOD_DECODER_MODE_BRCM, TEMOD_R_DESKEW_MODE_BRCM_66B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_52, 0, 33, 0, 1, 0, 2, 0, 0, 0, 0,0 },
/*digital_operationSpeeds_SPEED_31p5G_X4:           23/0x17*/  { 2, 0, 1, TEMOD_ENCODE_MODE_BRCM, 0, TEMOD_BLOCKSYNC_MODE_BRCM, 1, 0, TEMOD_R_DESCR1_MODE_66B, TEMOD_DECODER_MODE_BRCM, TEMOD_R_DESKEW_MODE_BRCM_66B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_52, 0, 33, 0, 1, 0, 1, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_31p5G_KR4:          24/0x18*/  { 2, 0, 3, TEMOD_ENCODE_MODE_CL82, 0, TEMOD_BLOCKSYNC_MODE_CL82, 0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL82, TEMOD_R_DESKEW_MODE_CL82_66B, TEMOD_DESC2_MODE_CL82, 2, 0, TEMOD_PLL_MODE_DIV_52, 0, 33, 0, 1, 0, 1, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_20G_CX2:            25/0x19*/  { 1, 0, 1, TEMOD_ENCODE_MODE_BRCM, 0, TEMOD_BLOCKSYNC_MODE_BRCM, 1, 0, TEMOD_R_DESCR1_MODE_66B, TEMOD_DECODER_MODE_BRCM, TEMOD_R_DESKEW_MODE_BRCM_66B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 33, 0, 1, 0, 2, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_20G_X2:             26/0x1a*/  { 1, 0, 1, TEMOD_ENCODE_MODE_BRCM, 0, TEMOD_BLOCKSYNC_MODE_BRCM, 1, 0, TEMOD_R_DESCR1_MODE_66B, TEMOD_DECODER_MODE_BRCM, TEMOD_R_DESKEW_MODE_BRCM_66B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 33, 0, 1, 0, 2, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_40G_X4:             27/0x1b*/  { 2, 0, 1, TEMOD_ENCODE_MODE_BRCM, 0, TEMOD_BLOCKSYNC_MODE_BRCM, 1, 0, TEMOD_R_DESCR1_MODE_66B, TEMOD_DECODER_MODE_BRCM, TEMOD_R_DESKEW_MODE_BRCM_66B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 33, 0, 1, 0, 1, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_10G_KR1:            28/0x1c*/  { 0, 0, 3, TEMOD_ENCODE_MODE_CL49, 0, TEMOD_BLOCKSYNC_MODE_CL49,  0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL49, TEMOD_R_DESKEW_MODE_BYPASS, TEMOD_DESC2_MODE_CL49, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 33, 0, 1, 0, 4, 0, 0, 0, 0 ,1},
/*digital_operationSpeeds_SPEED_10p6_X1:            29/0x1d*/  { 0, 0, 3, TEMOD_ENCODE_MODE_CL49, 0, TEMOD_BLOCKSYNC_MODE_CL49,  0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL49, TEMOD_R_DESKEW_MODE_BYPASS, TEMOD_DESC2_MODE_CL49, 2, 0, TEMOD_PLL_MODE_DIV_70, 0, 33, 0, 1, 0, 4, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_20G_KR2             30/0x1e*/  { 1, 0, 3, TEMOD_ENCODE_MODE_CL82, 0, TEMOD_BLOCKSYNC_MODE_CL82, 0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL82, TEMOD_R_DESKEW_MODE_CL82_66B, TEMOD_DESC2_MODE_CL82, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 33, 0, 1, 0, 2, 0, 0, 0, 0 ,1},
/*digital_operationSpeeds_SPEED_20G_CR2             31/0x1f*/  { 1, 0, 3, TEMOD_ENCODE_MODE_CL82, 0, TEMOD_BLOCKSYNC_MODE_CL82, 0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL82, TEMOD_R_DESKEW_MODE_CL82_66B, TEMOD_DESC2_MODE_CL82, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 33, 0, 1, 0, 2, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_21G_X2              32/0x20*/  { 1, 0, 3, TEMOD_ENCODE_MODE_CL82, 0, TEMOD_BLOCKSYNC_MODE_CL82, 0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL82, TEMOD_R_DESKEW_MODE_CL82_66B, TEMOD_DESC2_MODE_CL82, 2, 0, TEMOD_PLL_MODE_DIV_70, 0, 33, 0, 1, 0, 2, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_40G_KR4             33/0x21*/  { 2, 0, 3, TEMOD_ENCODE_MODE_CL82, 0, TEMOD_BLOCKSYNC_MODE_CL82, 0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL82, TEMOD_R_DESKEW_MODE_CL82_66B, TEMOD_DESC2_MODE_CL82, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 33, 0, 1, 0, 1, 0, 0, 0, 0 ,1},
/*digital_operationSpeeds_SPEED_40G_CR4             34/0x22*/  { 2, 0, 3, TEMOD_ENCODE_MODE_CL82, 0, TEMOD_BLOCKSYNC_MODE_CL82, 0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL82, TEMOD_R_DESKEW_MODE_CL82_66B, TEMOD_DESC2_MODE_CL82, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 33, 0, 1, 0, 1, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_42G_X4              35/0x23*/  { 2, 0, 3, TEMOD_ENCODE_MODE_CL82, 0, TEMOD_BLOCKSYNC_MODE_CL82, 0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL82, TEMOD_R_DESKEW_MODE_CL82_66B, TEMOD_DESC2_MODE_CL82, 2, 0, TEMOD_PLL_MODE_DIV_70, 0, 33, 0, 1, 0, 1, 0, 0, 0, 0 ,1},
/*digital_operationSpeeds_SPEED_100G_CR10           36/0x24*/  { 3, 0, 3, TEMOD_ENCODE_MODE_CL82, 0, TEMOD_BLOCKSYNC_MODE_CL82, 0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL82, TEMOD_R_DESKEW_MODE_CL82_66B, TEMOD_DESC2_MODE_CL82, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 9, 5, 1, 18,   1, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_107G_CR10 TBD       37/0x25*/  { 3, 0, 3, TEMOD_ENCODE_MODE_CL82, 0, TEMOD_BLOCKSYNC_MODE_CL82, 0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL82, TEMOD_R_DESKEW_MODE_CL82_66B, TEMOD_DESC2_MODE_CL82, 2, 0, TEMOD_PLL_MODE_DIV_70, 0, 9, 5, 1, 18, 1, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_120G_X12 TBD        38/0x26*/  { 4, 0, 3, TEMOD_ENCODE_MODE_CL82, 0, TEMOD_BLOCKSYNC_MODE_CL82, 0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL82, TEMOD_R_DESKEW_MODE_CL82_66B, TEMOD_DESC2_MODE_CL82, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 33, 0, 1, 0, 1, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_127G_X12 TBD        39/0x27*/  { 4, 0, 3, TEMOD_ENCODE_MODE_CL82, 0, TEMOD_BLOCKSYNC_MODE_CL82, 0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL82, TEMOD_R_DESKEW_MODE_CL82_66B, TEMOD_DESC2_MODE_CL82, 2, 0, TEMOD_PLL_MODE_DIV_70, 0, 33, 0, 1, 0, 1, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             40/0x28*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             41/0x29*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             42/0x2a*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             43/0x2b*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             44/0x2c*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             45/0x2d*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             46/0x2e*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             47/0x2f*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             48/0x30*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_5G_KR1:             49/0x31*/  { 0, 1, 3, TEMOD_ENCODE_MODE_CL49, 0, TEMOD_BLOCKSYNC_MODE_CL49,  0, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL49, TEMOD_R_DESKEW_MODE_BYPASS, TEMOD_DESC2_MODE_CL49, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 33, 0, 1, 0, 8, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_10p5G_X4:           50/0x32*/  { 2, 1, 0, TEMOD_ENCODE_MODE_CL48, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 1, 0, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL48, TEMOD_R_DESKEW_MODE_8B_10B, TEMOD_DESC2_MODE_CL48, 2, 0, TEMOD_PLL_MODE_DIV_42, 0, 5, 0, 1, 0, 2, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             51/0x33*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             52/0x34*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_10M_10p3125:        53/0x35*/  { 0, 8, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 1, 0, TEMOD_PLL_MODE_DIV_66, 1, 4125, 0, 1, 0, 4124, 1, 1, 165, 41 ,0},
/*digital_operationSpeeds_SPEED_100M_10p3125:       54/0x36*/  { 0, 8, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 0, 0, TEMOD_PLL_MODE_DIV_66, 1, 825, 0, 1, 0, 412, 0, 1, 165, 41 ,0},
/*digital_operationSpeeds_SPEED_1000M_10p3125:      55/0x37*/  { 0, 8, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 165, 0, 1, 0, 41, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_2p5G_X1_10p3125:    56/0x38*/  { 0, 3, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_66, 0, 33, 0, 1, 0, 16, 0, 0, 0, 0 ,0},
/*digital_operationSpeeds_SPEED_ILLEGAL             57/0x39*/  { 1, 0, 0, TEMOD_ENCODE_MODE_CL36, 0, TEMOD_BLOCKSYNC_MODE_8B10B, 0, 1, TEMOD_R_DESCR1_MODE_BYPASS, TEMOD_DECODER_MODE_CL36, TEMOD_R_DESKEW_MODE_CL36_10B, TEMOD_DESC2_MODE_CL36, 2, 0, TEMOD_PLL_MODE_DIV_40, 0, 2500, 0, 1, 0, 2499, 0, 0, 0, 0 ,0}
};

#endif
/**
\struct eagle_sc_pmd_entry_t

This embodies most parameters passed from PCS to PMD. A table of these values
for all supported speeds is documented below. Two parameters (vco_rate, and
media_type) cannot be hardcoded as a function of speed and are dynamically
calculated by TMod.  For more details please refer to the micro-arch document
and the Eagle programmer's guide.

<table cellspacing=5>
<tr><td colspan=3><B>Per Speed PMD Parameters</B></td></tr>
<tr><td><B>speed</B></td>
<td><B>num_lanes</B></td>
<td><B>pll_mode</B></td>
<td><B>os_type</B></td>
<td><B>brdfe_on</B></td>
<td><B>osdfe_on</B></td>
<td><B>cl72_emulation_en</B></td>
<td><B>scrambling_dis</B></td></tr>
<tr><td>SPD_10M           ( 0)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_10M           ( 1)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_100M          ( 2)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_1000M         ( 3)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_1G_CX1        ( 4)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_1G_KX1        ( 5)</td><td>0</td><td>TEMOD_PMA_OS_MODE_10 </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_2p5G          ( 6)</td><td>0</td><td>TEMOD_PMA_OS_MODE_2  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_5G_X1         ( 7)</td><td>0</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_10G_CX4       ( 8)</td><td>4</td><td>TEMOD_PMA_OS_MODE_2  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_10G_KX4       ( 9)</td><td>4</td><td>TEMOD_PMA_OS_MODE_2  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_10G_X4        (10)</td><td>4</td><td>TEMOD_PMA_OS_MODE_2  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_13G_X4        (11)</td><td>4</td><td>TEMOD_PMA_OS_MODE_2  </td><td>TEMOD_PLL_MODE_DIV_52</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_15G_X4        (12)</td><td>4</td><td>TEMOD_PMA_OS_MODE_2  </td><td>TEMOD_PLL_MODE_DIV_60</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_16G_X4        (13)</td><td>4</td><td>TEMOD_PMA_OS_MODE_2  </td><td>TEMOD_PLL_MODE_DIV_64</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_20G_CX4       (14)</td><td>4</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_10G_CX2       (15)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_10G_X2        (16)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_20G_X4        (17)</td><td>4</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>1</tr>
<tr><td>SPD_10p5G_X2      (18)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_42</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_21G_X4        (19)</td><td>4</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_42</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_12p7G_X2      (20)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_42</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_25p45G_X4     (21)</td><td>4</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_42</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_15p75G_X2     (22)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_52</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_31p5G_X4      (23)</td><td>4</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_52</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_31p5G_KR4     (24)</td><td>4</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_52</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_20G_CX2       (25)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_20G_X2        (26)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_40G_X4        (27)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_10G_KR1       (28)</td><td>0</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_10p6_X1       (29)</td><td>0</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_70</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_20G_KR2       (30)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_20G_CR2       (31)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_21G_X2        (32)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_70</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_40G_KR4       (33)</td><td>4</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_40G_CR4       (34)</td><td>4</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_42G_X4        (35)</td><td>4</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_70</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_100G_CR10     (36)</td><td>0</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_107G_CR10     (37)</td><td>0</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_70</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_120G_X12      (38)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_127G_X12      (39)</td><td>2</td><td>TEMOD_PMA_OS_MODE_1  </td><td>TEMOD_PLL_MODE_DIV_70</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (40)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (41)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (42)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (43)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (44)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (45)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (46)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (47)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (48)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_5G_KR1        (49)</td><td>0</td><td>TEMOD_PMA_OS_MODE_2  </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_10p5G_X4      (50)</td><td>2</td><td>TEMOD_PMA_OS_MODE_2  </td><td>TEMOD_PLL_MODE_DIV_42</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (51)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (52)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_10M_10p3125   (53)</td><td>0</td><td>TEMOD_PMA_OS_MODE_10 </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_100M_10p3125  (54)</td><td>0</td><td>TEMOD_PMA_OS_MODE_10 </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_1000M_10p3125 (55)</td><td>0</td><td>TEMOD_PMA_OS_MODE_10 </td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_2p5G_X1_10p312(56)</td><td>0</td><td>TEMOD_PMA_OS_MODE_3_3</td><td>TEMOD_PLL_MODE_DIV_66</td><td>0</td><td>0</td><td>0</td><td>0</tr>
<tr><td>SPD_ILLEGAL       (57)</td><td>0</td><td>TEMOD_PMA_OS_MODE_5  </td><td>TEMOD_PLL_MODE_DIV_40</td><td>0</td><td>0</td><td>0</td><td>0</tr>
</table>
*/

typedef struct eagle_sc_pmd_entry_t
{
  int num_lanes;
  int t_pma_os_mode;
  int pll_mode;
  int osdfe_on;
  int brdfe_on;
  int scrambling_dis;
  /* add additional entries here */
} eagle_sc_pmd_entry_st;

#endif                             
