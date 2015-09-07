/*----------------------------------------------------------------------
 * $Id: sc_field_defines.h  $
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
 * Broadcom Corporation
 * Proprietary and Confidential information
 * All rights reserved
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without the
 * prior written consent of Broadcom Corporation.
 *---------------------------------------------------------------------
 * File       : sc_field_defines.h
 * Description: 
 *---------------------------------------------------------------------*/

/*##############################################################################
############################################################################# */
#define TEMOD_PLL_MODE_DIV_40       2
#define TEMOD_PLL_MODE_DIV_42       3
#define TEMOD_PLL_MODE_DIV_48       4
#define TEMOD_PLL_MODE_DIV_52       6
#define TEMOD_PLL_MODE_DIV_54       7
#define TEMOD_PLL_MODE_DIV_60       8
#define TEMOD_PLL_MODE_DIV_64       9
#define TEMOD_PLL_MODE_DIV_66       10
#define TEMOD_PLL_MODE_DIV_70       12
#define TEMOD_PLL_MODE_DIV_80       13
#define TEMOD_PLL_MODE_DIV_92       14
#define TEMOD_PLL_MODE_ILLEGAL      0 

/*############################################################################# */
#define TEMOD_PMA_OS_MODE_1         0 
#define TEMOD_PMA_OS_MODE_2         1 
#define TEMOD_PMA_OS_MODE_3         2 
#define TEMOD_PMA_OS_MODE_3_3       3 
#define TEMOD_PMA_OS_MODE_4         4 
#define TEMOD_PMA_OS_MODE_5         5 
#define TEMOD_PMA_OS_MODE_8         6 
#define TEMOD_PMA_OS_MODE_8_25      7 
#define TEMOD_PMA_OS_MODE_10        8 
#define TEMOD_PMA_OS_MODE_ILLEGAL   15

/*############################################################################# */
#define TEMOD_SCR_MODE_BYPASS       0
#define TEMOD_SCR_MODE_66B          1
#define TEMOD_SCR_MODE_80B          2
#define TEMOD_SCR_MODE_64B          3

/*############################################################################# */
#define TEMOD_ENCODE_MODE_NONE        0
#define TEMOD_ENCODE_MODE_CL48        1
#define TEMOD_ENCODE_MODE_CL48_2_LANE 2
#define TEMOD_ENCODE_MODE_CL36        3
#define TEMOD_ENCODE_MODE_CL82        4
#define TEMOD_ENCODE_MODE_CL49        5
#define TEMOD_ENCODE_MODE_BRCM        6
#define TEMOD_ENCODE_MODE_ILLEGAL     7


/*############################################################################# */
#define TEMOD_CL48_CHECK_END_OFF       0
#define TEMOD_CL48_CHECK_END_ON        1

/*############################################################################# */
#define TEMOD_SIGDET_FILTER_NONCX4     0
#define TEMOD_SIGDET_FILTER_CX4        1

/*############################################################################# */
#define TEMOD_BLOCKSYNC_MODE_NONE      0
#define TEMOD_BLOCKSYNC_MODE_CL49      1
#define TEMOD_BLOCKSYNC_MODE_CL82      2
#define TEMOD_BLOCKSYNC_MODE_8B10B     3
#define TEMOD_BLOCKSYNC_MODE_FEC       4
#define TEMOD_BLOCKSYNC_MODE_BRCM      5
#define TEMOD_BLOCKSYNC_MODE_ILLEGAL   7

/*############################################################################# */
#define TEMOD_R_REORDER_MODE_NONE      0
#define TEMOD_R_REORDER_MODE_CL48      1
#define TEMOD_R_REORDER_MODE_CL36      2
#define TEMOD_R_REORDER_MODE_CL36_CL48 3

/*############################################################################# */
#define TEMOD_CL36_DISABLE             0
#define TEMOD_CL36_ENABLE              1

/*############################################################################# */
#define TEMOD_R_DESCR1_MODE_BYPASS     0
#define TEMOD_R_DESCR1_MODE_66B        1
#define TEMOD_R_DESCR1_MODE_10B        2
#define TEMOD_R_DESCR1_MODE_ILLEGAL    3


/*############################################################################# */
#define TEMOD_DECODER_MODE_NONE       0
#define TEMOD_DECODER_MODE_CL49       1
#define TEMOD_DECODER_MODE_BRCM       2     
#define TEMOD_DECODER_MODE_ALU        3   
#define TEMOD_DECODER_MODE_CL48       4   
#define TEMOD_DECODER_MODE_CL36       5 
#define TEMOD_DECODER_MODE_CL82       6
#define TEMOD_DECODER_MODE_ILLEGAL    7

/*############################################################################# */
#define TEMOD_R_DESKEW_MODE_BYPASS        0
#define TEMOD_R_DESKEW_MODE_8B_10B        1
#define TEMOD_R_DESKEW_MODE_BRCM_66B      2
#define TEMOD_R_DESKEW_MODE_CL82_66B      3
#define TEMOD_R_DESKEW_MODE_CL36_10B      4
#define TEMOD_R_DESKEW_MODE_ILLEGAL       7

/*#############################################################################*/
#define TEMOD_DESC2_MODE_NONE             0
#define TEMOD_DESC2_MODE_CL49             1
#define TEMOD_DESC2_MODE_BRCM             2
#define TEMOD_DESC2_MODE_ALU              3
#define TEMOD_DESC2_MODE_CL48             4
#define TEMOD_DESC2_MODE_CL36             5
#define TEMOD_DESC2_MODE_CL82             6
#define TEMOD_DESC2_MODE_ILLEGAL          7


/*#############################################################################*/

#define TEMOD_R_DESC2_BYTE_DELETION_100M     0
#define TEMOD_R_DESC2_BYTE_DELETION_10M      1
#define TEMOD_R_DESC2_BYTE_DELETION_NONE     2

/*############################################################################*/

#define TEMOD_R_DEC1_DESCR_MODE_NONE         0
#define TEMOD_R_DEC1_DESCR_MODE_BRCM64B66B   1

/*#############################################################################*/

#define RTL_SPEED_100G_CR10                         0x29
#define RTL_SPEED_40G_CR4                           0x28
#define RTL_SPEED_40G_KR4                           0x27
#define RTL_SPEED_40G_X4                            0x1C
#define RTL_SPEED_32p7G_X4                          0x21
#define RTL_SPEED_31p5G_X4                          0x20
#define RTL_SPEED_25p45G_X4                         0x14
#define RTL_SPEED_21G_X4                            0x13
#define RTL_SPEED_20G_CR2                           0x3A
#define RTL_SPEED_20G_KR2                           0x39
#define RTL_SPEED_20G_X2                            0x1D
#define RTL_SPEED_20G_X4                            0x22
#define RTL_SPEED_20G_CX2                           0x1E
#define RTL_SPEED_20G_CX4                           0x12
#define RTL_SPEED_16G_X4                            0x0C
#define RTL_SPEED_15p75G_X2                         0x2C
#define RTL_SPEED_15G_X4                            0x0B
#define RTL_SPEED_13G_X4                            0x0A
#define RTL_SPEED_12p7G_X2                          0x19
#define RTL_SPEED_12p5G_X4                          0x09
#define RTL_SPEED_12G_X4                            0x08
#define RTL_SPEED_10p5G_X2                          0x17
#define RTL_SPEED_10G_KR1                           0x0F
#define RTL_SPEED_10G_CX2                           0x24
#define RTL_SPEED_10G_X2                            0x23
#define RTL_SPEED_10G_KX4                           0x0E
#define RTL_SPEED_10G_CX4                           0x07
#define RTL_SPEED_10G_X4                            0x06
#define RTL_SPEED_10G_CX1                           0x33
#define RTL_SPEED_6G_X4                             0x05
#define RTL_SPEED_5G_X4                             0x04
#define RTL_SPEED_5G_X2                             0x2A
#define RTL_SPEED_2p5G_X1                           0x03
#define RTL_SPEED_1G_KX1                            0x0D
#define RTL_SPEED_1000M                             0x02
#define RTL_SPEED_1G_CX1                            0x34
#define RTL_SPEED_100M                              0x01
#define RTL_SPEED_10M                               0x00
#define RTL_SPEED_5G_X1                             0x10
#define RTL_SPEED_6p36G_X1                          0x11
#define RTL_SPEED_10G_X2_NOSCRAMBLE                 0x15
#define RTL_SPEED_10G_CX2_NOSCRAMBLE                0x16
#define RTL_SPEED_10p5G_CX2_NOSCRAMBLE              0x18
#define RTL_SPEED_12p7G_CX2                         0x1A
#define RTL_SPEED_10G_X1                            0x1B
#define RTL_SPEED_10G_SFI                           0x1F
#define RTL_SPEED_3p125G_IL                         0x3C
#define RTL_SPEED_6p25_IL                           0x3D
#define RTL_SPEED_10p31G_IL                         0x3E
#define RTL_SPEED_10p96G_IL                         0x3F
#define RTL_SPEED_12p5G_IL                          0x40
#define RTL_SPEED_11p5G_IL                          0x41
#define RTL_SPEED_1p25G_IL                          0x42
#define RTL_SPEED_2G_FC                             0x2E
#define RTL_SPEED_4G_FC                             0x2F
#define RTL_SPEED_8G_FC                             0x30
#define RTL_SPEED_1G_CL73                           0x3B
#define RTL_SPEED_1000M_SGMII                       0x2D
#define RTL_SPEED_NONE                              0x2B


/*#############################################################################*/
