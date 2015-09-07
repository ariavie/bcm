/*----------------------------------------------------------------------
 * $Id: tscmod_functions.h,v 1.10 Broadcom SDK $
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
 * Description: string defines  for tier1 and tier2 selectors
 *-----------------------------------------------------------------------*/

#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

/* These are the Tier1 functions */

/* special handle for MDK */
#define TSCMOD_INIT              "TSCMOD_INIT"

#define MASTER_SLAVE_CONTROL     "MASTER_SLAVE_CONTROL"
#define SLAVE_WAKEUP_CONTROL     "SLAVE_WAKEUP_CONTROL"
#define CREDIT_SET               "CREDIT_SET"
#define ENCODE_SET               "ENCODE_SET"
#define DECODE_SET               "DECODE_SET"
#define CREDIT_CONTROL           "CREDIT_CONTROL"
#define AFE_RESET_ANALOG         "AFE_RESET_ANALOG"
#define AFE_RXSEQ_START_CONTROL  "AFE_RXSEQ_START_CONTROL"
#define AFE_SPEED_UP_DSC_VGA     "AFE_SPEED_UP_DSC_VGA"
#define TX_LANE_CONTROL          "TX_LANE_CONTROL"
#define RX_LANE_CONTROL          "RX_LANE_CONTROL"
#define RX_LOOPBACK_PCSBYP       "RX_LOOPBACK_PCS_BYPASS"
#define PCS_BYPASS_CTL           "PCS_BYPASS_CTL"
#define TX_TAP_CONTROL           "TX_TAP_CONTROL"
#define TX_PI_CONTROL            "TX_PI_CONTROL"
#define TX_AMP_CONTROL           "TX_AMP_CONTROL"
#define RX_P1_SLICER_CONTROL     "RX_P1_SLICER_CONTROL"
#define RX_M1_SLICER_CONTROL     "RX_M1_SLICER_CONTROL"
#define RX_D_SLICER_CONTROL      "RX_D_SLICER_CONTROL"
#define REFCLK_SET               "REFCLK_SET"
#define ASYMMETRIC_MODE_SET      "ASYMMETRIC_MODE_SET"
#define SPD_INTF_CONTROL         "SPD_INTF_CONTROL"
/* put an _ since this begins with a number */
#define _100FX_CONTROL           "100FX_CONTROL"
#define TX_LANE_DISABLE          "TX_LANE_DISABLE"
#define DUPLEX_CONTROL           "DUPLEX_CONTROL"
#define POWER_CONTROL            "POWER_CONTROL"
#define AUTONEG_SET              "AUTONEG_SET"
#define AUTONEG_GET              "AUTONEG_GET"
#define AUTONEG_CONTROL          "AUTONEG_CONTROL"
#define AUTONEG_PAGE_SET         "AUTONEG_PAGE_SET"
#define ANEG_CONTROL             "ANEG_CONTROL"
#define CARRIER_EXTENSION        "CARRIER_EXTENSION"
#define REG_READ                 "REG_READ"
#define REG_WRITE                "REG_WRITE"
#define PRBS_CHECK               "PRBS_CHECK"
#define CJPAT_CRPAT_CONTROL      "CJPAT_CRPAT_CONTROL"
#define CJPAT_CRPAT_CHECK        "CJPAT_CRPAT_CHECK"
#define TSCMOD_DIAG               "TSCMOD_DIAG"
#define LANE_SWAP                "LANE_SWAP"
#define PARALLEL_DETECT_CONTROL  "PARALLEL_DETECT_CONTROL"
#define CLAUSE_72_CONTROL        "CLAUSE_72_CONTROL"
#define PLL_SEQUENCER_CONTROL    "PLL_SEQUENCER_CONTROL"
#define PLL_LOCK_WAIT            "PLL_LOCK_WAIT"
#define PROG_DATA                "PROG_DATA"
#define REVID_READ               "REVID_READ"
#define RX_SEQ_CONTROL           "RX_SEQ_CONTROL"
#define SET_POLARITY             "SET_POLARITY"
#define SET_AER                  "SET_AER"
#define SET_PORT_MODE            "SET_PORT_MODE"
#define SET_AN_PORT_MODE         "SET_AN_PORT_MODE"
#define PRBS_CONTROL             "PRBS_CONTROL"
#define PRBS_MODE                "PRBS_MODE"
#define PRBS_SEED_A1A0           "PRBS_SEED_A1A0"
#define PRBS_SEED_A3A2           "PRBS_SEED_A3A2"
#define PRBS_SEED_LOAD_CONTROL   "PRBS_SEED_LOAD_CONTROL"
#define PRBS_DECOUPLE_CONTROL    "PRBS_DECOUPLE_CONTROL"
#define SCRAMBLER_CONTROL        "SCRAMBLER_CONTROL"
#define RX_PF_CONTROL            "RX_PF_CONTROL"
#define RX_DFE_TAP1_CONTROL      "RX_DFE_TAP1_CONTROL"
#define RX_DFE_TAP2_CONTROL      "RX_DFE_TAP2_CONTROL"
#define RX_DFE_TAP3_CONTROL      "RX_DFE_TAP3_CONTROL"
#define RX_DFE_TAP4_CONTROL      "RX_DFE_TAP4_CONTROL"
#define RX_DFE_TAP5_CONTROL      "RX_DFE_TAP5_CONTROL"
#define RX_VGA_CONTROL           "RX_VGA_CONTROL"
#define SOFT_RESET               "SOFT_RESET"
#define AER_LANE_SELECT          "AER_LANE_SELECT"
#define SET_SPD_INTF             "SET_SPD_INTF"
#define TX_BERT_CONTROL          "TX_BERT_CONTROL"
#define RX_LOOPBACK_CONTROL      "RX_LOOPBACK_CONTROL"
#define TX_LOOPBACK_CONTROL      "TX_LOOPBACK_CONTROL"
#define PORT_STAT_DISPLAY        "PORT_STAT_DISPLAY"
#define CORE_RESET               "CORE_RESET"
#define WAIT_PMD_LOCK            "WAIT_PMD_LOCK"
#define HGSOLO                   "HGSOLO"
#define MISC_CONTROL             "MISC_CONTROL"
#define FIRMWARE_SET             "FIRMWARE_SET"
#define UC_SYNC_CMD              "UC_SYNC_CMD"
#define FWMODE_CONTROL           "FWMODE_CONTROL"
#define FEC_CONTROL              "FEC_CONTROL"
#define HIGIG2_CONTROL           "HIGIG2_CONTROL"

/* These are the Tier2+ functions */
#define SPD_INTF_SET                    "SPD_INTF_SET"
#define PRBS_ENABLE           "PRBS7_ENABLE"
#define PRBS_DISABLE           "PRBS_DISABLE"
/*PRBS_CHECK is also a Tier2 function */

#endif
