/*----------------------------------------------------------------------
 * $Id: 5509d2d352c03b7980d7916c76729908c47d1fa6 $
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
 *  Broadcom Corporation
 *  Proprietary and Confidential information
 *  All rights reserved
 *  This source file is the property of Broadcom Corporation, and
 *  may not be copied or distributed in any isomorphic form without the
 *  prior written consent of Broadcom Corporation.
 *----------------------------------------------------------------------
 *  Description: define enumerators  
 *----------------------------------------------------------------------*/
#ifndef tefmod_sc_defines_H_
#define tefmod_sc_defines_H_


#define T_PMA_BITMUX_MODE_1to1   0
#define T_PMA_BITMUX_MODE_2to1   1
#define T_PMA_BITMUX_MODE_5to1   2

#define T_SCR_MODE_BYPASS        0
#define T_SCR_MODE_CL49          1
#define T_SCR_MODE_40G_2_LANE    2
#define T_SCR_MODE_100G          3
#define T_SCR_MODE_20G           4
#define T_SCR_MODE_40G_4_LANE    5

#define R_DESCR_MODE_BYPASS      0
#define R_DESCR_MODE_CL49        1
#define R_DESCR_MODE_CL82        2

#define R_DEC_TL_MODE_BYPASS     0
#define R_DEC_TL_MODE_CL49       1
#define R_DEC_TL_MODE_CL82       2

#define R_DEC_FSM_MODE_BYPASS    0
#define R_DEC_FSM_MODE_CL49      1
#define R_DEC_FSM_MODE_CL82      2

#define BS_DIST_MODE_5_LANE_TDM          0
#define BS_DIST_MODE_2_LANE_TDM_2_VLANE  1
#define BS_DIST_MODE_2_LANE_TDM_1_VLANE  2
#define BS_DIST_MODE_NO_TDM              3

#define BS_BTMX_MODE_1to1        0
#define BS_BTMX_MODE_2to1        1
#define BS_BTMX_MODE_5to1        2

#define IDLE_DELETION_MODE_BYPASS       0 
#define IDLE_DELETION_MODE_40G          1 
#define IDLE_DELETION_MODE_100G         2
#define IDLE_DELETION_MODE_20G          3

#define T_FIFO_MODE_BYPASS       0 
#define T_FIFO_MODE_40G          1 
#define T_FIFO_MODE_100G         2
#define T_FIFO_MODE_20G          3

#define T_ENC_MODE_BYPASS        0
#define T_ENC_MODE_CL49          1
#define T_ENC_MODE_CL82          2

#define R_DESKEW_MODE_BYPASS     0
#define R_DESKEW_MODE_20G        1
#define R_DESKEW_MODE_40G_4_LANE 2
#define R_DESKEW_MODE_40G_2_LANE 3
#define R_DESKEW_MODE_100G       4
#define R_DESKEW_MODE_CL49       5
#define R_DESKEW_MODE_CL91       6

#define AM_MODE_20G              1
#define AM_MODE_40G              2
#define AM_MODE_100G             3

#define NUM_LANES_01                            0
#define NUM_LANES_02                            1
#define NUM_LANES_04                            2


#define PORT_MODE_FOUR                          0
#define PORT_MODE_THREE_3_2_COMBO               1
#define PORT_MODE_THREE_1_0_COMBO               2
#define PORT_MODE_TWO                           3
#define PORT_MODE_ONE                           4

#define OS_MODE_1                               0
#define OS_MODE_2                               1
#define OS_MODE_4                               2
#define OS_MODE_16p5                            8
#define OS_MODE_20p625                          12

#define REG_ACCESS_TYPE_MDIO                    0
#define REG_ACCESS_TYPE_UC                      1
#define REG_ACCESS_TYPE_EXT                     2


#define BS_CL82_SYNC_MODE 0
#define BS_CL49_SYNC_MODE 1
#define BLKSIZE           66
#define DSC_DATA_WID      40

#endif

