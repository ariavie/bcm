/*----------------------------------------------------------------------
 * $Id: e052711a0407786d589e10ba3fd535eea85603f1 $
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
 * Broadcom Corporation
 * Proprietary and Confidential information
 * All rights reserved
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without the
 * prior written consent of Broadcom Corporation.
 *---------------------------------------------------------------------
 ############### THIS FILE IS AUTOMATICALLY GENERATED.  ###############
 ############### DO !! NOT !! MANUALLY EDIT THIS FILE.  ###############
 *---------------------------------------------------------------------
 * Description: This file contains enums, elems and doxyten comments
 * needed for SerDes Configuration programs.
 *---------------------------------------------------------------------
 * CVS INFORMATION:
 * Please see inc/enum_desc.txt for CVS information.
 *----------------------------------------------------------------------
 */

/* This file is automatically generated. Do not modify it. Modify the
 * inc/enum_desc.txt to change enums, elems, or comments. For issues about
 * the process that creates this file contact the tefmod development team.
 */

#include "tefmod_enum_defines.h"

char* e2s_tefmod_lane_select_t [CNT_tefmod_lane_select_t] = {
  "TEFMOD_LANE_0_0_0_0" ,
  "TEFMOD_LANE_0_0_0_1" ,
  "TEFMOD_LANE_0_0_1_0" ,
  "TEFMOD_LANE_0_0_1_1" ,
  "TEFMOD_LANE_0_1_0_0" ,
  "TEFMOD_LANE_0_1_0_1" ,
  "TEFMOD_LANE_0_1_1_0" ,
  "TEFMOD_LANE_0_1_1_1" ,
  "TEFMOD_LANE_1_0_0_0" ,
  "TEFMOD_LANE_1_0_0_1" ,
  "TEFMOD_LANE_1_0_1_0" ,
  "TEFMOD_LANE_1_0_1_1" ,
  "TEFMOD_LANE_1_1_0_0" ,
  "TEFMOD_LANE_1_1_0_1" ,
  "TEFMOD_LANE_1_1_1_0" ,
  "TEFMOD_LANE_1_1_1_1" ,
  "TEFMOD_LANE_BCST" ,
  "TEFMOD_LANE_ILLEGAL" 
}; /* e2s_tefmod_lane_select_t */

int e2n_tefmod_lane_select_t [CNT_tefmod_lane_select_t] = {
  0 ,
  1 ,
  2 ,
  3 ,
  4 ,
  5 ,
  6 ,
  7 ,
  8 ,
  9 ,
  10 ,
  11 ,
  12 ,
  13 ,
  14 ,
  15 ,
  15 ,
  0 
}; /* e2n_tefmod_lane_select_t */

char* e2s_tefmod_spd_intfc_type_t [CNT_tefmod_spd_intfc_type_t] = {
  "TEFMOD_SPD_ZERO" ,
  "TEFMOD_SPD_10_SGMII" ,
  "TEFMOD_SPD_100_SGMII" ,
  "TEFMOD_SPD_1000_SGMII" ,
  "TEFMOD_SPD_2500" ,
  "TEFMOD_SPD_10000_XFI" ,
  "TEFMOD_SPD_10600_XFI_HG" ,
  "TEFMOD_SPD_20000_XFI" ,
  "TEFMOD_SPD_21200_XFI_HG" ,
  "TEFMOD_SPD_25000_XFI" ,
  "TEFMOD_SPD_26500_XFI_HG" ,
  "TEFMOD_SPD_20G_MLD_X2" ,
  "TEFMOD_SPD_21G_MLD_HG_X2" ,
  "TEFMOD_SPD_40G_MLD_X2" ,
  "TEFMOD_SPD_42G_MLD_HG_X2" ,
  "TEFMOD_SPD_40G_MLD_X4" ,
  "TEFMOD_SPD_42G_MLD_HG_X4" ,
  "TEFMOD_SPD_50G_MLD_X2" ,
  "TEFMOD_SPD_53G_MLD_HG_X2" ,
  "TEFMOD_SPD_100G_MLD_X4" ,
  "TEFMOD_SPD_106G_MLD_HG_X4" ,
  "TEFMOD_SPD_10000_HI" ,
  "TEFMOD_SPD_10000" ,
  "TEFMOD_SPD_12000_HI" ,
  "TEFMOD_SPD_13000" ,
  "TEFMOD_SPD_15000" ,
  "TEFMOD_SPD_16000" ,
  "TEFMOD_SPD_20000" ,
  "TEFMOD_SPD_20000_SCR" ,
  "TEFMOD_SPD_21000" ,
  "TEFMOD_SPD_25455" ,
  "TEFMOD_SPD_31500" ,
  "TEFMOD_SPD_40G_X4" ,
  "TEFMOD_SPD_42G_X4" ,
  "TEFMOD_SPD_40G_XLAUI" ,
  "TEFMOD_SPD_42G_XLAUI" ,
  "TEFMOD_SPD_10000_X2" ,
  "TEFMOD_SPD_10000_HI_DXGXS" ,
  "TEFMOD_SPD_10000_DXGXS" ,
  "TEFMOD_SPD_10000_HI_DXGXS_SCR" ,
  "TEFMOD_SPD_10000_DXGXS_SCR" ,
  "TEFMOD_SPD_10500_HI_DXGXS" ,
  "TEFMOD_SPD_12773_HI_DXGXS" ,
  "TEFMOD_SPD_12773_DXGXS" ,
  "TEFMOD_SPD_15750_HI_DXGXS" ,
  "TEFMOD_SPD_20G_MLD_DXGXS" ,
  "TEFMOD_SPD_21G_HI_MLD_DXGXS" ,
  "TEFMOD_SPD_20G_DXGXS" ,
  "TEFMOD_SPD_21G_HI_DXGXS" ,
  "TEFMOD_SPD_100G_CR10" ,
  "TEFMOD_SPD_120G_CR12" ,
  "TEFMOD_SPD_10000_XFI_HG2" ,
  "TEFMOD_SPD_50G_MLD_X4" ,
  "TEFMOD_SPD_53G_MLD_HG_X4" ,
  "TEFMOD_SPD_10000_XFI_CR1" ,
  "TEFMOD_SPD_10600_XFI_HG_CR1" ,
  "TEFMOD_SPD_CL73_20G" ,
  "TEFMOD_SPD_CL73_25G" ,
  "TEFMOD_SPD_1G_20G" ,
  "TEFMOD_SPD_1G_25G" ,
  "TEFMOD_SPD_ILLEGAL" 
}; /* e2s_tefmod_spd_intfc_type_t */

int e2n_tefmod_spd_intfc_type_t [CNT_tefmod_spd_intfc_type_t] = {
  0 ,
  10 ,
  100 ,
  1000 ,
  2500 ,
  10000 ,
  10560 ,
  20000 ,
  21200 ,
  25000 ,
  26500 ,
  20000 ,
  21000 ,
  40000 ,
  42400 ,
  40000 ,
  42400 ,
  50000 ,
  53000 ,
  100000 ,
  106000 ,
  10000 ,
  10000 ,
  12000 ,
  13000 ,
  15000 ,
  16000 ,
  20000 ,
  20000 ,
  21000 ,
  25000 ,
  31500 ,
  40000 ,
  40000 ,
  40000 ,
  42000 ,
  10000 ,
  10000 ,
  10000 ,
  10000 ,
  10000 ,
  10500 ,
  12773 ,
  12773 ,
  15750 ,
  20000 ,
  20000 ,
  20000 ,
  21000 ,
  100000 ,
  120000 ,
  10000 ,
  50000 ,
  53000 ,
  54000 ,
  55000 ,
  56000 ,
  57000 ,
  58000 ,
  59000 ,
  0 
}; /* e2n_tefmod_spd_intfc_type_t */

char* e2s_tefmod_regacc_type_t [CNT_tefmod_regacc_type_t] = {
  "TEFMOD_REGACC_CL22" ,
  "TEFMOD_REGACC_CL45" ,
  "TEFMOD_REGACC_TOTSC" ,
  "TEFMOD_REGACC_SBUS_FD" ,
  "TEFMOD_REGACC_SBUS_BD" ,
  "TEFMOD_REGACC_ILLEGAL" 
}; /* e2s_tefmod_regacc_type_t */

char* e2s_tefmod_port_type_t [CNT_tefmod_port_type_t] = {
  "TEFMOD_MULTI_PORT" ,
  "TEFMOD_TRI1_PORT" ,
  "TEFMOD_TRI2_PORT" ,
  "TEFMOD_DXGXS" ,
  "TEFMOD_SINGLE_PORT" ,
  "TEFMOD_PORT_MODE_ILLEGAL" 
}; /* e2s_tefmod_port_type_t */

char* e2s_tefmod_sc_mode_type_t [CNT_tefmod_sc_mode_type_t] = {
  "TEFMOD_SC_MODE_HT_WITH_BASIC_OVERRIDE" ,
  "TEFMOD_SC_MODE_HT_OVERRIDE" ,
  "TEFMOD_SC_MODE_ST" ,
  "TEFMOD_SC_MODE_ST_OVERRIDE" ,
  "TEFMOD_SC_MODE_AN_CL37" ,
  "TEFMOD_SC_MODE_AN_CL73" ,
  "TEFMOD_SC_MODE_BYPASS" ,
  "TEFMOD_SC_MODE_ILLEGAL" 
}; /* e2s_tefmod_sc_mode_type_t */

char* e2s_tefmod_diag_type_t [CNT_tefmod_diag_type_t] = {
  "TEFMOD_DIAG_GENERAL" ,
  "TEFMOD_DIAG_TOPOLOGY" ,
  "TEFMOD_DIAG_LINK" ,
  "TEFMOD_DIAG_SPEED" ,
  "TEFMOD_DIAG_ANEG" ,
  "TEFMOD_DIAG_TFC" ,
  "TEFMOD_DIAG_AN_TIMERS" ,
  "TEFMOD_DIAG_STATE" ,
  "TEFMOD_DIAG_DEBUG" ,
  "TEFMOD_DIAG_IEEE" ,
  "TEFMOD_DIAG_EEE" ,
  "TEFMOD_SERDES_DIAG" ,
  "TEFMOD_DIAG_ALL" ,
  "TEFMOD_DIAG_ILLEGAL" 
}; /* e2s_tefmod_diag_type_t */

char* e2s_tefmod_model_type_t [CNT_tefmod_model_type_t] = {
  "TEFMOD_WC" ,
  "TEFMOD_WC_A0" ,
  "TEFMOD_WC_A1" ,
  "TEFMOD_WC_A2" ,
  "TEFMOD_WC_B0" ,
  "TEFMOD_WC_B1" ,
  "TEFMOD_WC_B2" ,
  "TEFMOD_WC_C0" ,
  "TEFMOD_WC_C1" ,
  "TEFMOD_WC_C2" ,
  "TEFMOD_WC_D0" ,
  "TEFMOD_WC_D1" ,
  "TEFMOD_WC_D2" ,
  "TEFMOD_XN" ,
  "TEFMOD_WL" ,
  "TEFMOD_WL_A0" ,
  "TEFMOD_QS" ,
  "TEFMOD_QS_A0" ,
  "TEFMOD_QS_B0" ,
  "TEFMOD_MODEL_TYPE_ILLEGAL" 
}; /* e2s_tefmod_model_type_t */

char* e2s_tefmod_an_type_t [CNT_tefmod_an_type_t] = {
  "TEFMOD_CL73" ,
  "TEFMOD_CL37" ,
  "TEFMOD_CL73_BAM" ,
  "TEFMOD_CL37_BAM" ,
  "TEFMOD_CL37_SGMII" ,
  "TEFMOD_HPAM" ,
  "TEFMOD_AN_TYPE_ILLEGAL" 
}; /* e2s_tefmod_an_type_t */

char* e2s_tefmod_eye_direction_t [CNT_tefmod_eye_direction_t] = {
  "TEFMOD_EYE_VU" ,
  "TEFMOD_EYE_VD" ,
  "TEFMOD_EYE_HL" ,
  "TEFMOD_EYE_HR" ,
  "TEFMOD_EYE_ILLEGAL" 
}; /* e2s_tefmod_eye_direction_t */

char* e2s_tefmod_aspd_type_t [CNT_tefmod_aspd_type_t] = {
  "TEFMOD_ASPD_10M" ,
  "TEFMOD_ASPD_100M" ,
  "TEFMOD_ASPD_1000M" ,
  "TEFMOD_ASPD_2p5G_X1" ,
  "TEFMOD_ASPD_5G_X4" ,
  "TEFMOD_ASPD_6G_X4" ,
  "TEFMOD_ASPD_10G_X4" ,
  "TEFMOD_ASPD_10G_CX4" ,
  "TEFMOD_ASPD_12G_X4" ,
  "TEFMOD_ASPD_12p5G_X4" ,
  "TEFMOD_ASPD_13G_X4" ,
  "TEFMOD_ASPD_15G_X4" ,
  "TEFMOD_ASPD_16G_X4" ,
  "TEFMOD_ASPD_1G_KX1" ,
  "TEFMOD_ASPD_10G_KX4" ,
  "TEFMOD_ASPD_10G_KR1" ,
  "TEFMOD_ASPD_5G_X1" ,
  "TEFMOD_ASPD_6p36G_X1" ,
  "TEFMOD_ASPD_20G_CX4" ,
  "TEFMOD_ASPD_21G_X4" ,
  "TEFMOD_ASPD_25p45G_X4" ,
  "TEFMOD_ASPD_10G_X2_NOSCRAMBLE" ,
  "TEFMOD_ASPD_10G_CX2_NOSCRAMBLE" ,
  "TEFMOD_ASPD_10p5G_X2" ,
  "TEFMOD_ASPD_10p5G_CX2_NOSCRAMBLE" ,
  "TEFMOD_ASPD_12p7G_X2" ,
  "TEFMOD_ASPD_12p7G_CX2" ,
  "TEFMOD_ASPD_10G_X1" ,
  "TEFMOD_ASPD_40G_X4" ,
  "TEFMOD_ASPD_20G_X2" ,
  "TEFMOD_ASPD_20G_CX2" ,
  "TEFMOD_ASPD_10G_SFI" ,
  "TEFMOD_ASPD_31p5G_X4" ,
  "TEFMOD_ASPD_32p7G_X4" ,
  "TEFMOD_ASPD_20G_X4" ,
  "TEFMOD_ASPD_10G_X2" ,
  "TEFMOD_ASPD_10G_CX2" ,
  "TEFMOD_ASPD_12G_SCO_R2" ,
  "TEFMOD_ASPD_10G_SCO_X2" ,
  "TEFMOD_ASPD_40G_KR4" ,
  "TEFMOD_ASPD_40G_CR4" ,
  "TEFMOD_ASPD_100G_CR10" ,
  "TEFMOD_ASPD_5G_X2" ,
  "TEFMOD_ASPD_15p75G_X2" ,
  "TEFMOD_ASPD_2G_FC" ,
  "TEFMOD_ASPD_4G_FC" ,
  "TEFMOD_ASPD_8G_FC" ,
  "TEFMOD_ASPD_10G_CX1" ,
  "TEFMOD_ASPD_1G_CX1" ,
  "TEFMOD_ASPD_20G_KR2" ,
  "TEFMOD_ASPD_20G_CR2" ,
  "TEFMOD_ASPD_TYPE_ILLEGAL" 
}; /* e2s_tefmod_aspd_type_t */

int e2n_tefmod_aspd_type_t [CNT_tefmod_aspd_type_t] = {
  0x0 ,
  0x1 ,
  0x2 ,
  0x3 ,
  0x4 ,
  0x5 ,
  0x6 ,
  0x7 ,
  0x8 ,
  0x9 ,
  0xa ,
  0xb ,
  0xc ,
  0xd ,
  0xe ,
  0xf ,
  0x10 ,
  0x11 ,
  0x12 ,
  0x13 ,
  0x14 ,
  0x15 ,
  0x16 ,
  0x17 ,
  0x18 ,
  0x19 ,
  0x1a ,
  0x1b ,
  0x1c ,
  0x1d ,
  0x1e ,
  0x1f ,
  0x20 ,
  0x21 ,
  0x22 ,
  0x23 ,
  0x24 ,
  0x25 ,
  0x26 ,
  0x27 ,
  0x28 ,
  0x29 ,
  0x2a ,
  0x2c ,
  0x2e ,
  0x2f ,
  0x30 ,
  0x33 ,
  0x34 ,
  0x39 ,
  0x3a ,
  0xff 
}; /* e2n_tefmod_aspd_type_t */

char* e2s_tefmod_tech_ability_t [CNT_tefmod_tech_ability_t] = {
  "TEFMOD_ABILITY_1G" ,
  "TEFMOD_ABILITY_10G_KR" ,
  "TEFMOD_ABILITY_40G_KR4" ,
  "TEFMOD_ABILITY_40G_CR4" ,
  "TEFMOD_ABILITY_100G_CR10" ,
  "TEFMOD_ABILITY_10G_HIGIG" ,
  "TEFMOD_ABILITY_10G_CX4" ,
  "TEFMOD_ABILITY_20G_X4" ,
  "TEFMOD_ABILITY_40G" ,
  "TEFMOD_ABILITY_25P455G" ,
  "TEFMOD_ABILITY_21G_X4" ,
  "TEFMOD_ABILITY_20G_X4S" ,
  "TEFMOD_ABILITY_10G_DXGXS_HIGIG" ,
  "TEFMOD_ABILITY_10G_DXGXS" ,
  "TEFMOD_ABILITY_10P5G_DXGXS" ,
  "TEFMOD_ABILITY_12P5_DXGXS" ,
  "TEFMOD_ABILITY_20G_KR2_HIGIG" ,
  "TEFMOD_ABILITY_20G_KR2" ,
  "TEFMOD_ABILITY_20G_CR2" ,
  "TEFMOD_ABILITY_15P75G_R2" ,
  "TEFMOD_ABILITY_100G_KR4" ,
  "TEFMOD_ABILITY_100G_CR4" ,
  "TEFMOD_ABILITY_40G_KR2" ,
  "TEFMOD_ABILITY_40G_CR2" ,
  "TEFMOD_ABILITY_50G_KR2" ,
  "TEFMOD_ABILITY_50G_CR2" ,
  "TEFMOD_ABILITY_50G_KR4" ,
  "TEFMOD_ABILITY_50G_CR4" ,
  "TEFMOD_ABILITY_20G_KR1" ,
  "TEFMOD_ABILITY_20G_CR1" ,
  "TEFMOD_ABILITY_25G_KR1" ,
  "TEFMOD_ABILITY_25G_CR1" ,
  "TEFMOD_ABILITY_ILLEGAL" 
}; /* e2s_tefmod_tech_ability_t */

int e2n_tefmod_tech_ability_t [CNT_tefmod_tech_ability_t] = {
  0x6 ,
  0x7 ,
  0x8 ,
  0x9 ,
  0xa ,
  0x103 ,
  0x104 ,
  0x10a ,
  0x305 ,
  0x308 ,
  0x309 ,
  0x400 ,
  0x401 ,
  0x402 ,
  0x403 ,
  0x404 ,
  0x405 ,
  0x406 ,
  0x406 ,
  0x407 ,
  0x408 ,
  0x409 ,
  0x40a ,
  0x500 ,
  0x501 ,
  0x502 ,
  0x503 ,
  0x504 ,
  0x505 ,
  0x506 ,
  0x507 ,
  0x508 ,
  0x999 
}; /* e2n_tefmod_tech_ability_t */

char* e2s_tefmod_cl37bam_ability_t [CNT_tefmod_cl37bam_ability_t] = {
  "TEFMOD_BAM37ABL_2P5G" ,
  "TEFMOD_BAM37ABL_5G_X4" ,
  "TEFMOD_BAM37ABL_6G_X4" ,
  "TEFMOD_BAM37ABL_10G_HIGIG" ,
  "TEFMOD_BAM37ABL_10G_CX4" ,
  "TEFMOD_BAM37ABL_12G_X4" ,
  "TEFMOD_BAM37ABL_12P5_X4" ,
  "TEFMOD_BAM37ABL_13G_X4" ,
  "TEFMOD_BAM37ABL_15G_X4" ,
  "TEFMOD_BAM37ABL_16G_X4" ,
  "TEFMOD_BAM37ABL_20G_X4_CX4" ,
  "TEFMOD_BAM37ABL_20G_X4" ,
  "TEFMOD_BAM37ABL_21G_X4" ,
  "TEFMOD_BAM37ABL_25P455G" ,
  "TEFMOD_BAM37ABL_31P5G" ,
  "TEFMOD_BAM37ABL_32P7G" ,
  "TEFMOD_BAM37ABL_40G" ,
  "TEFMOD_BAM37ABL_10G_X2_CX4" ,
  "TEFMOD_BAM37ABL_10G_DXGXS" ,
  "TEFMOD_BAM37ABL_10P5G_DXGXS" ,
  "TEFMOD_BAM37ABL_12P7_DXGXS" ,
  "TEFMOD_BAM37ABL_15P75G_R2" ,
  "TEFMOD_BAM37ABL_20G_X2_CX4" ,
  "TEFMOD_BAM37ABL_20G_X2" ,
  "TEFMOD_BAM37ABL_ILLEGAL" 
}; /* e2s_tefmod_cl37bam_ability_t */

char* e2s_tefmod_diag_an_type_t [CNT_tefmod_diag_an_type_t] = {
  "TEFMOD_DIAG_AN_DONE" ,
  "TEFMOD_DIAG_AN_HCD" ,
  "TEFMOD_DIAG_AN_TYPE_ILLEGAL" 
}; /* e2s_tefmod_diag_an_type_t */

char* e2s_tefmod_tier1_function_type_t [CNT_tefmod_tier1_function_type_t] = {
  "PCS_BYPASS_CTL" ,
  "CREDIT_SET" ,
  "ENCODE_SET" ,
  "DECODE_SET" ,
  "CREDIT_CONTROL" ,
  "AFE_SPEED_UP_DSC_VGA" ,
  "TX_LANE_CONTROL" ,
  "RX_LANE_CONTROL" ,
  "TX_LANE_DISABLE" ,
  "POWER_CONTROL" ,
  "AUTONEG_SET" ,
  "AUTONEG_GET" ,
  "AUTONEG_CONTROL" ,
  "AUTONEG_PAGE_SET" ,
  "REG_READ" ,
  "REG_WRITE" ,
  "PRBS_CHECK" ,
  "CJPAT_CRPAT_CONTROL" ,
  "CJPAT_CRPAT_CHECK" ,
  "TEFMOD_DIAG" ,
  "LANE_SWAP" ,
  "PARALLEL_DETECT_CONTROL" ,
  "CLAUSE_72_CONTROL" ,
  "PLL_SEQUENCER_CONTROL" ,
  "PLL_LOCK_WAIT" ,
  "DUPLEX_CONTROL" ,
  "REVID_READ" ,
  "BYPASS_SC" ,
  "SET_POLARITY" ,
  "SET_PORT_MODE" ,
  "SET_AN_PORT_MODE" ,
  "PRBS_CONTROL" ,
  "PRBS_MODE" ,
  "SOFT_RESET" ,
  "SET_SPD_INTF" ,
  "TX_BERT_CONTROL" ,
  "RX_LOOPBACK_CONTROL" ,
  "RX_PMD_LOOPBACK_CONTROL" ,
  "TX_LOOPBACK_CONTROL" ,
  "TX_PMD_LOOPBACK_CONTROL" ,
  "CORE_RESET" ,
  "REFCLK_SET" ,
  "WAIT_PMD_LOCK" ,
  "FIRMWARE_SET" ,
  "INIT_PCS_FALCON" ,
  "DISABLE_PCS_FALCON" ,
  "INIT_PMD_FALCON" ,
  "PMD_LANE_SWAP_TX" ,
  "PMD_LANE_SWAP" ,
  "PCS_LANE_SWAP" ,
  "SET_SC_SPEED" ,
  "CHECK_SC_STATS" ,
  "SET_OVERRIDE_0" ,
  "SET_OVERRIDE_1" ,
  "PMD_RESET_REMOVE" ,
  "PMD_RESET_BYPASS" ,
  "INIT_PCS_ILKN" ,
  "TOGGLE_SW_SPEED_CHANGE" ,
  "TIER1_FUNCTION_ILLEGAL" 
}; /* e2s_tefmod_tier1_function_type_t */

char* e2s_tefmod_fec_en_parm_t [CNT_tefmod_fec_en_parm_t] = {
  "TEFMOD_CL91_TX_EN_DIS" ,
  "TEFMOD_CL91_RX_EN_DIS" ,
  "TEFMOD_CL91_IND_ONLY_EN_DIS" ,
  "TEFMOD_CL91_COR_ONLY_EN_DIS" ,
  "TEFMOD_CL74_TX_EN_DIS" ,
  "TEFMOD_CL74_RX_EN_DIS" ,
  "TEFMOD_CL74_CL91_EN_DIS" 
}; /* e2s_tefmod_fec_en_parm_t */

char* e2s_tefmod_tx_disable_enum_t [CNT_tefmod_tx_disable_enum_t] = {
  "TEFMOD_TX_LANE_TRAFFIC" ,
  "TEFMOD_TX_LANE_RESET" 
}; /* e2s_tefmod_tx_disable_enum_t */

char* e2s_tefmod_os_mode_type [CNT_tefmod_os_mode_type] = {
  "TEFMOD_PMA_OS_MODE_1" ,
  "TEFMOD_PMA_OS_MODE_2" ,
  "TEFMOD_PMA_OS_MODE_4" ,
  "TEFMOD_PMA_OS_MODE_16_25" ,
  "TEFMOD_PMA_OS_MODE_20_625" ,
  "TEFMOD_PMA_OS_MODE_ILLEGAL" 
}; /* e2s_tefmod_os_mode_type */

int e2n_tefmod_os_mode_type [CNT_tefmod_os_mode_type] = {
  1 ,
  2 ,
  4 ,
  16 ,
  10 ,
  0 
}; /* e2n_tefmod_os_mode_type */

char* e2s_tefmod_scr_mode [CNT_tefmod_scr_mode] = {
  "TEFMOD_SCR_MODE_BYPASS" ,
  "TEFMOD_SCR_MODE_CL49" ,
  "TEFMOD_SCR_MODE_40G_2_LANE" ,
  "TEFMOD_SCR_MODE_100G" ,
  "TEFMOD_SCR_MODE_20G" ,
  "TEFMOD_SCR_MODE_40G_4_LANE" ,
  "TEFMOD_SCR_MODE_ILLEGAL" 
}; /* e2s_tefmod_scr_mode */

int e2n_tefmod_scr_mode [CNT_tefmod_scr_mode] = {
  0 ,
  1 ,
  2 ,
  3 ,
  4 ,
  5 ,
  0 
}; /* e2n_tefmod_scr_mode */

char* e2s_tefmod_encode_mode [CNT_tefmod_encode_mode] = {
  "TEFMOD_ENCODE_MODE_NONE" ,
  "TEFMOD_ENCODE_MODE_CL49" ,
  "TEFMOD_ENCODE_MODE_CL82" ,
  "TEFMOD_ENCODE_MODE_ILLEGAL" 
}; /* e2s_tefmod_encode_mode */

int e2n_tefmod_encode_mode [CNT_tefmod_encode_mode] = {
  0 ,
  1 ,
  2 ,
  15 
}; /* e2n_tefmod_encode_mode */

char* e2s_tefmod_descrambler_mode [CNT_tefmod_descrambler_mode] = {
  "TEFMOD_R_DESCR_MODE_BYPASS" ,
  "TEFMOD_R_DESCR_MODE_CL49" ,
  "TEFMOD_R_DESCR_MODE_CL82" ,
  "TEFMOD_R_DESCR_MODE_ILLEGAL" 
}; /* e2s_tefmod_descrambler_mode */

char* e2s_tefmod_dec_tl_mode [CNT_tefmod_dec_tl_mode] = {
  "TEFMOD_DEC_TL_MODE_NONE" ,
  "TEFMOD_DEC_TL_MODE_CL49" ,
  "TEFMOD_DEC_TL_MODE_CL82" ,
  "TEFMOD_DEC_TL_MODE_ILLEGAL" 
}; /* e2s_tefmod_dec_tl_mode */

char* e2s_tefmod_dec_fsm_mode [CNT_tefmod_dec_fsm_mode] = {
  "TEFMOD_DEC_FSM_MODE_NONE" ,
  "TEFMOD_DEC_FSM_MODE_CL49" ,
  "TEFMOD_DEC_FSM_MODE_CL82" ,
  "TEFMOD_DEC_FSM_MODE_ILLEGAL" 
}; /* e2s_tefmod_dec_fsm_mode */

char* e2s_tefmod_deskew_mode [CNT_tefmod_deskew_mode] = {
  "TEFMOD_R_DESKEW_MODE_BYPASSi" ,
  "TEFMOD_R_DESKEW_MODE_10G" ,
  "TEFMOD_R_DESKEW_MODE_40G_4_LANE" ,
  "TEFMOD_R_DESKEW_MODE_40G_2_LANE" ,
  "TEFMOD_R_DESKEW_MODE_100G" ,
  "TEFMOD_R_DESKEW_MODE_CL49" ,
  "TEFMOD_R_DESKEW_MODE_CL91" ,
  "TEFMOD_R_DESKEW_MODE_ILLEGAL" 
}; /* e2s_tefmod_deskew_mode */

char* e2s_tefmod_t_fifo_mode [CNT_tefmod_t_fifo_mode] = {
  "TEFMOD_T_FIFO_MODE_NONE" ,
  "TEFMOD_T_FIFO_MODE_40G" ,
  "TEFMOD_T_FIFO_MODE_100G" ,
  "TEFMOD_T_FIFO_MODE_20G" ,
  "TEFMOD_T_FIFO_MODE_ILLEGAL" 
}; /* e2s_tefmod_t_fifo_mode */

char* e2s_tefmod_bs_btmx_mode [CNT_tefmod_bs_btmx_mode] = {
  "TEFMOD_BS_BTMX_MODE_NONE" ,
  "TEFMOD_BS_BTMX_MODE_1to1" ,
  "TEFMOD_BS_BTMX_MODE_2to1" ,
  "TEFMOD_BS_BTMX_MODE_5to1" ,
  "TEFMOD_BS_BTMX_MODE_ILLEGAL" 
}; /* e2s_tefmod_bs_btmx_mode */

char* e2s_tefmod_bs_dist_mode [CNT_tefmod_bs_dist_mode] = {
  "TEFMOD_BS_DIST_MODE_5_LANE_TDM" ,
  "TEFMOD_BS_DIST_MODE_2_LANE_TDM_2_VLANE" ,
  "TEFMOD_BS_DIST_MODE_2_LANE_TDM_1_VLANE" ,
  "TEFMOD_BS_DIST_MODE_NO_TDM" ,
  "TEFMOD_BS_DIST_MODE_ILLEGAL" 
}; /* e2s_tefmod_bs_dist_mode */

