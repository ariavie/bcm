/*
 * $Id: defs.h,v 1.271 Broadcom SDK $
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
 * File:        defs.h
 * Purpose:     Basic defines for system and chips
 *              This should be included before all other SOC include
 *              files to make sure BCM_xxxx is defined for them.
 */

#ifndef _SOC_DEFS_H
#define _SOC_DEFS_H

#include <sdk_config.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#include <soc/mcm/allenum.h>
#endif
#ifdef BCM_ROBO_SUPPORT
#include <soc/robo/mcm/allenum.h>
#endif
#ifdef BCM_SBX_SUPPORT
#include <soc/sbx/allenum.h>
#endif
#ifdef BCM_EA_SUPPORT
#include <soc/ea/allenum.h>
#endif
#include <soc/maxdef.h>

/* max number of switch devices */
#ifndef SOC_MAX_NUM_SWITCH_DEVICES
#define SOC_MAX_NUM_SWITCH_DEVICES  16
#endif

/* max number of ethernet devices */
#ifndef SOC_MAX_NUM_ETHER_DEVICES
#define SOC_MAX_NUM_ETHER_DEVICES   2
#endif

/* max number of all kinds of devices */
#define SOC_MAX_NUM_DEVICES         (SOC_MAX_NUM_SWITCH_DEVICES +\
                     SOC_MAX_NUM_ETHER_DEVICES)

/*
 * BCM Chip Support
 *
 * There are a few classes of defines used throughout the code:
 * - chip number defines with revsion numbers (from soc/allenum.h,
 *   but it can be modified from $SDK/make/Make.local)
 * - chip number defines without revision numbers
 * - chip type or group defines (such as BCM_*_SUPPORT)
 * - component support defines (GBP or FILTER)
 */
#if defined(BCM_5675_A0)
#define BCM_5675
#define BCM_5670
#define BCM_HERCULES15_SUPPORT
#define BCM_HERCULES_SUPPORT
#endif

#if defined(BCM_56504_A0) || defined(BCM_56504_B0)
#define BCM_56504
#define BCM_FIREBOLT_SUPPORT
#endif

#if defined(BCM_56102_A0)
#define BCM_56102
#define BCM_FIREBOLT_SUPPORT
#define BCM_FELIX_SUPPORT
#define BCM_FELIX1_SUPPORT
#endif

#if defined(BCM_56304_B0)
#define BCM_56304
#define BCM_FIREBOLT_SUPPORT
#define BCM_HELIX_SUPPORT
#define BCM_HELIX1_SUPPORT
#define BCM_MIRAGE_SUPPORT
#endif

#if defined(BCM_56112_A0)
#define BCM_56112
#define BCM_FIREBOLT_SUPPORT
#define BCM_FELIX_SUPPORT
#define BCM_FELIX15_SUPPORT
#endif

#if defined(BCM_56314_A0)
#define BCM_56314
#define BCM_FIREBOLT_SUPPORT
#define BCM_HELIX_SUPPORT
#define BCM_HELIX15_SUPPORT
#define BCM_MIRAGE_SUPPORT
#endif

#if defined(BCM_56580_A0)
#define BCM_56580
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_GOLDWING_SUPPORT
#endif

#if defined(BCM_56700_A0)
#define BCM_56700
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_HUMV_SUPPORT
#endif

#if defined(BCM_56800_A0)
#define BCM_56800
#define BCM_56304
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#endif

#if defined(BCM_56218_A0)
#define BCM_56218
#define BCM_FIREBOLT_SUPPORT
#define BCM_HELIX_SUPPORT
#define BCM_RAPTOR_SUPPORT
#endif

#if defined(BCM_56514_A0)
#define BCM_56514
#define BCM_FIREBOLT_SUPPORT
#define BCM_FIREBOLT2_SUPPORT
#endif

#if defined(BCM_56624_A0) || defined(BCM_56624_B0)
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_MPLS_SUPPORT
#define BCM_IPFIX_SUPPORT
#endif

#if defined(BCM_56680_A0) || defined(BCM_56680_B0)
#define BCM_56680
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_VALKYRIE_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_MPLS_SUPPORT
#define BCM_IPFIX_SUPPORT
#endif

#if defined(BCM_56224_A0) || defined(BCM_56224_B0)
#define BCM_56224
#define BCM_FIREBOLT_SUPPORT
#define BCM_HELIX_SUPPORT
#define BCM_RAPTOR_SUPPORT
#define BCM_RAVEN_SUPPORT
#endif

#if defined(BCM_QE2000_A0) || defined(BCM_QE2000_A1) \
    || defined(BCM_QE2000_A2) || defined(BCM_QE2000_A3)
#define BCM_QE2000
#define BCM_QE2000_SUPPORT
#endif

#if defined(BCM_BME3200_B0) || defined(BCM_BME3200_A0)
#define BCM_BME3200
#define BCM_BME3200_SUPPORT
#endif

#if defined(BCM_BM9600_A1) || defined(BCM_BM9600_A0)
#define BCM_BM9600
#define BCM_BM9600_SUPPORT
#define BCM_88130
#define BCM_88130_SUPPORT
#endif

#if defined(BCM_FE2000_A0)
#define BCM_FE2000
#if !defined(BCM_FE2000_SUPPORT)
#define BCM_FE2000_SUPPORT
#endif
#endif

#if defined(BCM_53314_A0) || defined(BCM_53324_A0)
#define BCM_53314
#define BCM_FIREBOLT_SUPPORT
#define BCM_HELIX_SUPPORT
#define BCM_RAPTOR_SUPPORT
#define BCM_RAVEN_SUPPORT
#define BCM_HAWKEYE_SUPPORT
#endif

#if defined(BCM_56820_A0)
#define BCM_56820
#define BCM_56800
#define BCM_56304
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_SCORPION_SUPPORT
#define BCM_TRX_SUPPORT
#endif

#if defined(BCM_56725_A0)
#define BCM_56725
#define BCM_56820
#define BCM_56800
#define BCM_56304
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_HUMV_SUPPORT
#define BCM_SCORPION_SUPPORT
#define BCM_CONQUEROR_SUPPORT
#define BCM_TRX_SUPPORT
#endif

#if defined(BCM_56634_A0) || defined(BCM_56634_B0)
#define BCM_56634
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_MPLS_SUPPORT
#define BCM_IPFIX_SUPPORT
#endif

#if defined(BCM_56524_A0) || defined(BCM_56524_B0)
#define BCM_56524
#define BCM_56634
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_APOLLO_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_MPLS_SUPPORT
#define BCM_IPFIX_SUPPORT
#endif

#if defined(BCM_56685_A0) || defined(BCM_56685_B0)
#define BCM_56685
#define BCM_56634
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_VALKYRIE2_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_MPLS_SUPPORT
#define BCM_IPFIX_SUPPORT
#endif

#if defined(BCM_56334_A0) || defined(BCM_56334_B0)
#define BCM_56334
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_ENDURO_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_MPLS_SUPPORT
#endif

#if defined(BCM_56142_A0)
#define BCM_56142
#define BCM_56334
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_ENDURO_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_HURRICANE_SUPPORT
#endif

#if defined(BCM_88230_A0) || defined(BCM_88230_B0) || defined(BCM_88230_C0)
#define BCM_88230
#define BCM_88235
#define BCM_88239
#define BCM_56613
#define BCM_56931
#define BCM_56936
#if !defined(BCM_SIRIUS_SUPPORT)
#define BCM_SIRIUS_SUPPORT
#endif
#if !defined(BCM_SBX_CMIC_SUPPORT)
#define BCM_SBX_CMIC_SUPPORT
#endif
#endif

#if defined(BCM_88030_A0) || defined(BCM_88030_A1) || defined(BCM_88030_B0)
#define BCM_88030
#if !defined(BCM_CALADAN3_SUPPORT)
#define BCM_CALADAN3_SUPPORT
#endif
#if !defined(BCM_SBX_CMIC_SUPPORT)
#define BCM_SBX_CMIC_SUPPORT
#endif
#define BCM_CMICM_SUPPORT
#define BCM_EXTND_SBUS_SUPPORT
#define BCM_TIMESYNC_SUPPORT
#define BCM_SBUSDMA_SUPPORT
#define BCM_DDR3_SUPPORT
#endif

#if defined(BCM_56840_A0) || defined(BCM_56840_B0)
#define BCM_56840
#define BCM_56634
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_HUMV_SUPPORT
#define BCM_SCORPION_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_TRIDENT_SUPPORT
#define BCM_MPLS_SUPPORT
#define BCM_XGS3_FABRIC_SUPPORT
#define BCM_EXTND_SBUS_SUPPORT
#endif

#if defined(BCM_88732_A0) || defined(BCM88732_B0)
#define BCM_88732
#define BCM_56820
#define BCM_56800
#define BCM_56304
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_HUMV_SUPPORT
#define BCM_SCORPION_SUPPORT
#define BCM_CONQUEROR_SUPPORT
#define BCM_TRX_SUPPORT
#ifndef BCM_SHADOW_SUPPORT
#define BCM_SHADOW_SUPPORT
#endif
#endif

#if defined(BCM_56640_A0) || defined(BCM_56640_B0)
#define BCM_56640
#define BCM_56840
#define BCM_56634
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_SCORPION_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_TRIDENT_SUPPORT
#define BCM_TRIUMPH3_SUPPORT
#define BCM_MPLS_SUPPORT
#define BCM_CMICM_SUPPORT
#define BCM_EXTND_SBUS_SUPPORT
#define BCM_TIMESYNC_SUPPORT
#define BCM_SBUSDMA_SUPPORT
#define BCM_IPFIX_SUPPORT
#endif

#if defined(BCM_56340_A0)
#define BCM_56340
#define BCM_56840
#define BCM_56634
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_56640
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_SCORPION_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_TRIDENT_SUPPORT
#define BCM_TRIUMPH3_SUPPORT
#define BCM_HELIX4_SUPPORT
#define BCM_MPLS_SUPPORT
#define BCM_CMICM_SUPPORT
#define BCM_IPROC_SUPPORT
#define BCM_EXTND_SBUS_SUPPORT
#define BCM_TIMESYNC_SUPPORT
#define BCM_SBUSDMA_SUPPORT
#define BCM_HUMV_SUPPORT
#define BCM_XGS3_FABRIC_SUPPORT
#endif

#if defined(BCM_56440_A0) || defined(BCM_56440_B0)
#define BCM_56440
#define BCM_56840
#define BCM_56634
#define BCM_56334
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_HUMV_SUPPORT
#define BCM_SCORPION_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_ENDURO_SUPPORT
#define BCM_TRIDENT_SUPPORT
#define BCM_KATANA_SUPPORT
#define BCM_MPLS_SUPPORT
#define BCM_CMICM_SUPPORT
#define BCM_DDR3_SUPPORT
#define BCM_TIMESYNC_SUPPORT
#endif

#if defined(BCM_56450_A0) || defined(BCM_56450_B0)
#define BCM_56450
#define BCM_56440
#define BCM_56840
#define BCM_56634
#define BCM_56640
#define BCM_56334
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_SCORPION_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_TRIUMPH3_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_ENDURO_SUPPORT
#define BCM_TRIDENT_SUPPORT
#define BCM_KATANA2_SUPPORT
#define BCM_KATANA_SUPPORT
#define BCM_MPLS_SUPPORT
#define BCM_IPROC_SUPPORT
#define BCM_DDR3_SUPPORT
#define BCM_TIMESYNC_SUPPORT
#define BCM_EXTND_SBUS_SUPPORT
#define BCM_SBUSDMA_SUPPORT
#endif

#if defined(BCM_56850_A0)
#define BCM_56850
#define BCM_56840
#define BCM_56634
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_HUMV_SUPPORT
#define BCM_SCORPION_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_TRIDENT_SUPPORT
#define BCM_TRIUMPH3_SUPPORT
#define BCM_TRIDENT2_SUPPORT
#define BCM_MPLS_SUPPORT
#define BCM_XGS3_FABRIC_SUPPORT
#define BCM_EXTND_SBUS_SUPPORT
#define BCM_SBUSDMA_SUPPORT
#define BCM_TIMESYNC_SUPPORT
#endif

#if defined(BCM_88640_A0)
#define BCM_88640
#define BCM_DPP_SUPPORT
#define BCM_PETRAB_SUPPORT
/*Compilation features*/
#define BCM_LINKSCAN_LOCK_PER_UNIT
#endif

/* if ARAD_A0, ARAD_B0, ARAD_PLUS is defined, define also ARAD_A0, ARAD_B0, ARAD_PLUS */
#if defined(BCM_88650_A0) || defined(BCM_88650_B0) || defined(BCM_88660_A0) || defined(BCM_88202_A0) || defined(BCM_88670_A0)
#define BCM_88650
#define BCM_88650_A0
#define BCM_88650_B0
#define BCM_88660
#define BCM_88660_A0
#define BCM_88202
#define BCM_88202_A0
#define BCM_ARAD_SUPPORT
#define BCM_CMICM_SUPPORT
#define BCM_SBUSDMA_SUPPORT
#ifndef BCM_DPP_SUPPORT
#define BCM_DPP_SUPPORT
#endif
/* #define BCM_EXTND_SBUS_SUPPORT */
#define BCM_DDR3_SUPPORT
/*Compilation features*/
#define BCM_LINKSCAN_LOCK_PER_UNIT
#endif

#if defined(BCM_88670_A0) /* Defined via mcmrelease: genallchips.pl... */
#define BCM_88670
#define BCM_IPROC_SUPPORT
/* Compilation features */
#define BCM_LINKSCAN_LOCK_PER_UNIT
#define PORTMOD_PM4X10_SUPPORT
#define PORTMOD_PM4x10Q_SUPPORT
#define PORTMOD_PM_OS_ILKN_SUPPORT
#define PORTMOD_PM4X25_SUPPORT
#define PORTMOD_DNX_FABRIC_SUPPORT
#define PHYMOD_EAGLE_SUPPORT
#define PHYMOD_FALCON_SUPPORT
#endif

/* JERICHO-2-P3 */
#if defined(BCM_88850_P3)
#ifndef BCM_DPP_SUPPORT
#define BCM_DPP_SUPPORT
#endif
#define BCM_JERICHO_P3_SUPPORT
#define BCM_DNX_P3_SUPPORT
#endif


#if defined(BCM_88754_A0)
#define  BCM_88750_A0
#define  BCM_88750_B0
#endif

#if defined(BCM_88750_A0) || defined(BCM_88750_B0)
#define BCM_88750
#define BCM_88750_A0
#define BCM_88750_B0
#define BCM_88750_SUPPORT
/*Compilation features*/
#define BCM_LINKSCAN_LOCK_PER_UNIT
#endif
#if defined(BCM_88950_A0)
#define BCM_88950
#define BCM_88950_A0
#define BCM_88950_SUPPORT
#define BCM_88750
#define BCM_88750_A0
#define BCM_88750_B0
#define BCM_88750_SUPPORT
#define BCM_LINKSCAN_LOCK_PER_UNIT
#define BCM_CMICM_SUPPORT
#define BCM_SBUSDMA_SUPPORT
#define BCM_IPROC_SUPPORT
#define PORTMOD_DNX_FABRIC_SUPPORT
#define PHYMOD_FALCON_SUPPORT
#endif

#if defined(BCM_56150_A0)
#define BCM_56150
#define BCM_56142
#define BCM_56334
#define BCM_56624
#define BCM_56304
#define BCM_56800
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_ENDURO_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_HURRICANE_SUPPORT
#define BCM_HURRICANE2_SUPPORT
#define BCM_CMICM_SUPPORT
#define BCM_IPROC_SUPPORT
#define BCM_EXTND_SBUS_SUPPORT
#define BCM_SBUSDMA_SUPPORT
#define BCM_TIMESYNC_SUPPORT
#define BCM_TIME_V3_SUPPORT
#define BCM_DDR3_SUPPORT
#define BCM_IPROC_DDR_SUPPORT
#endif

#if defined(BCM_88750_B0)
#define BCM_88750
#define BCM_88750_SUPPORT
#endif

#if defined(BCM_53400_A0)
#define BCM_53400
#define BCM_56304
#define BCM_FIREBOLT_SUPPORT
#define BCM_BRADLEY_SUPPORT
#define BCM_TRIUMPH_SUPPORT
#define BCM_TRIUMPH2_SUPPORT
#define BCM_ENDURO_SUPPORT
#define BCM_TRX_SUPPORT
#define BCM_HURRICANE_SUPPORT
#define BCM_HURRICANE2_SUPPORT
#define BCM_CMICM_SUPPORT
#define BCM_IPROC_SUPPORT
#define BCM_EXTND_SBUS_SUPPORT
#define BCM_SBUSDMA_SUPPORT
#define BCM_TIMESYNC_SUPPORT
#define BCM_TIMESYNC_V3_SUPPORT
#define BCM_TIME_V3_SUPPORT
#define BCM_GREYHOUND_SUPPORT
#endif

/*
 * Strata XGS
 */
#if defined(BCM_5673) || defined(BCM_5690)
#define BCM_XGS1_SWITCH_SUPPORT
#endif

#if defined(BCM_5665) || defined(BCM_5695) || defined(BCM_5674)
#define BCM_XGS2_SWITCH_SUPPORT
#endif

#if defined(BCM_56504) || \
    defined(BCM_56102) || defined(BCM_56304) || defined(BCM_56218) || \
    defined(BCM_56112) || defined(BCM_56314) || defined(BCM_56514) || \
    defined(BCM_56580) || defined(BCM_56700) || defined(BCM_56800) || \
    defined(BCM_56624) || defined(BCM_56224) || defined(BCM_56820) || \
    defined(BCM_53314) || defined(BCM_56680) || defined(BCM_56725) || \
    defined(BCM_56634) || defined(BCM_56524) || defined(BCM_56685) || \
    defined(BCM_56334) || defined(BCM_56840) || defined(BCM_56142) || \
    defined(BCM_88732) || defined(BCM_56640) || defined(BCM_56440) || \
    defined(BCM_56850) || defined(BCM_56450) || defined(BCM_56340) || \
    defined(BCM_56150) || defined(BCM_53400)
#define BCM_XGS3_SWITCH_SUPPORT
#endif

#if defined(BCM_5675)
#define BCM_XGS2_FABRIC_SUPPORT
#endif

#if defined(BCM_56700) || defined(BCM_56701) || defined(BCM_56725)
#define BCM_XGS3_FABRIC_SUPPORT
#endif

#if defined(BCM_XGS3_SWITCH_SUPPORT)
#define BCM_XGS_SWITCH_SUPPORT
#endif

#if defined(BCM_XGS2_FABRIC_SUPPORT)
#define BCM_XGS12_FABRIC_SUPPORT
#endif

#if defined(BCM_XGS12_FABRIC_SUPPORT)
#define BCM_XGS12_SUPPORT
#endif

#if defined(BCM_XGS12_FABRIC_SUPPORT) || defined(BCM_XGS3_FABRIC_SUPPORT)
#define BCM_XGS_FABRIC_SUPPORT
#endif

#if defined(BCM_XGS_SWITCH_SUPPORT) || defined(BCM_XGS_FABRIC_SUPPORT)
#define BCM_XGS_SUPPORT
#endif

#if defined(BCM_XGS_SWITCH_SUPPORT)
#define BCM_SWITCH_SUPPORT
#endif

/*
 * SBX
 */
#if defined(BCM_QE2000) || defined(BCM_BME3200) || defined(BCM_BM9600) || defined(BCM_88130) || defined(BCM_88230) || defined(BCM_FE2000)
#ifndef BCM_SBX_SUPPORT
#define BCM_SBX_SUPPORT
#endif
#endif

/* 
 * EA
 */
#if defined(BCM_TK371X_A0)
#ifndef BCM_TK371X_SUPPORT
#define BCM_TK371X_SUPPORT
#endif
#endif 

#if defined(BCM_XGS3_SWITCH_SUPPORT)
#define BCM_FIELD_SUPPORT
#endif

/*
 * If any supported chip includes a Fast Filter Processor (FFP)
 * or Field Processor (FP)
 */
#if defined(BCM_XGS1_SWITCH_SUPPORT) || defined(BCM_XGS2_SWITCH_SUPPORT)
#define BCM_FILTER_SUPPORT 
#endif

#if defined(INCLUDE_ACL)
#define BCM_ACL_SUPPORT
#endif

/*
 * If any supported chip supports Higig2
 */
#if defined(BCM_56580) || defined(BCM_56700) || defined(BCM_56800) || \
    defined(BCM_56218) || defined(BCM_56624) || defined(BCM_56224) || \
    defined(BCM_56820) || defined(BCM_56680) || defined(BCM_56725) || \
    defined(BCM_53314) || defined(BCM_56634) || defined(BCM_56524) || \
    defined(BCM_56685) || defined(BCM_56334) || defined(BCM_88230) || \
    defined(BCM_56840) || defined(BCM_56142) || defined(BCM_88732) || \
    defined(BCM_56640) || defined(BCM_56440) || defined(BCM_56850) || \
    defined(BCM_56450) || defined(BCM_56340) || defined(BCM_56150) || \
    defined(BCM_88030) || defined(BCM_53400)
#define BCM_HIGIG2_SUPPORT
#endif

/*
 * If any supported chip has GXPORTS
 */
#if defined(BCM_56580) || defined(BCM_56700) || defined(BCM_56800) || \
    defined(BCM_56624) || defined(BCM_56820) || defined(BCM_56680) || \
    defined(BCM_56725) || defined(BCM_56634) || defined(BCM_56524) || \
    defined(BCM_56685) || defined(BCM_56334) || defined(BCM_88230) || \
    defined(BCM_56840) || defined(BCM_56142) || defined(BCM_88732) || \
    defined(BCM_56640) || defined(BCM_56440) || defined(BCM_56850) || \
    defined(BCM_56450) || defined(BCM_56340) || defined(BCM_56150) || \
    defined(BCM_53400)
#define BCM_GXPORT_SUPPORT
#endif

/*
 * If any supported chip includes storm control (bandwidth rate) capability
 */
#if defined(BCM_56218) || defined(BCM_56224) || defined(BCM_53314) || defined(BCM_88732)
#define BCM_BANDWIDTH_RATE_METER
#endif

/*
 * If any supported chip has BigMAC ports
 */
#if defined(BCM_XGS12_SUPPORT) || \
    defined(BCM_56504) || \
    defined(BCM_56102) || defined(BCM_56304) || defined(BCM_56112) || \
    defined(BCM_56314) || defined(BCM_56580) || defined(BCM_56700) || \
    defined(BCM_56800) || defined(BCM_56514) || defined(BCM_56624) || \
    defined(BCM_56680) || defined(BCM_56820) || defined(BCM_56725) || \
    defined(BCM_56334) || defined(BCM_88230) || defined(BCM_88732)
#define BCM_BIGMAC_SUPPORT
#endif

/*
 * If any supported chip has XMAC ports
 */
#if defined(BCM_56840) || defined(BCM_88732) || defined(BCM_56640) || \
    defined(BCM_56440) || defined(BCM_56450) || defined(BCM_56340) || \
    defined(BCM_88030)
#define BCM_XMAC_SUPPORT
#endif

/*
 * If any supported chip has XLMAC ports
 */
#if defined(BCM_56850) || defined(BCM_56150)|| defined(BCM_53400)
#define BCM_XLMAC_SUPPORT
#endif

/*
 * If any supported chip has UniMAC ports
 */
#if defined(BCM_56224) || defined(BCM_56624) || defined(BCM_56820) || \
    defined(BCM_53314) || defined(BCM_56680) || defined(BCM_56725) || \
    defined(BCM_56634) || defined(BCM_FE2000) || defined(BCM_56524) || \
    defined(BCM_56685) || defined(BCM_56334) || defined(BCM_88230) || \
    defined(BCM_56840) || defined(BCM_56142) || defined(BCM_88732) || \
    defined(BCM_56440) || defined(BCM_56450) || defined(BCM_56150) || \
    defined(BCM_53400)
#define BCM_UNIMAC_SUPPORT
#endif

/*
 * If any supported chip has CMAC ports
 */
#if defined(BCM_56640) || defined(BCM_56850) || defined(BCM_88030)
#define BCM_CMAC_SUPPORT
#endif

/*
 * If any supported chip has Unified ports
 */
#if defined(BCM_56640) || defined(BCM_56850) || defined(BCM_56340) || defined(BCM_88030)
#define BCM_UNIPORT_SUPPORT
#endif

/*
 * If any supported chip has ISM support
 */
#if defined(BCM_56640) || defined(BCM_56340) || defined(BCM_56850)
#define BCM_ISM_SUPPORT
#endif

/*
 * If any supported chip has CMICM support
 */
#ifdef BCM_SBUSDMA_SUPPORT
#define BCM_CMICM_SUPPORT
#endif

/*
 * If any supported chip includes an LED Processor
 */

#if defined(BCM_5675)  || defined(BCM_56504) || \
    defined(BCM_56102) || defined(BCM_56304) || \
    defined(BCM_56112) || defined(BCM_56314) || \
    defined(BCM_56580) || defined(BCM_56700) || \
    defined(BCM_56800) || defined(BCM_56218) || \
    defined(BCM_56514) || defined(BCM_56624) || \
    defined(BCM_56224) || defined(BCM_56820) || \
    defined(BCM_53314) || defined(BCM_56680) || \
    defined(BCM_56725) || defined(BCM_56634) || \
    defined(BCM_56524) || defined(BCM_56334) || \
    defined(BCM_56685) || defined(BCM_88230) || \
    defined(BCM_56840) || defined(BCM_56142) || \
    defined(BCM_88732) || defined(BCM_56640) || \
    defined(BCM_56440) || defined(BCM_56850) || \
    defined(BCM_56450) || defined(BCM_56340) || \
    defined(BCM_56150) || defined(BCM_88650) || \
    defined(BCM_53400)
#define BCM_LEDPROC_SUPPORT
#endif
#if defined(BCM_5324_A0)
#define BCM_5324
#endif

#if defined(BCM_5324_A1)
#define BCM_5324
#endif


#if defined(BCM_5396_A0)
#define BCM_5396
#define BCM_GEX_SUPPORT
#define BCM_DINO16_SUPPORT
#endif

#if defined(BCM_5389_A0)
#define BCM_5389
#define BCM_GEX_SUPPORT
#define BCM_DINO8_SUPPORT
#endif

#if defined(BCM_5398_A0)
#define BCM_5398
#endif

#if defined(BCM_5348_A0)
#define BCM_5348
#define BCM_FIELD_SUPPORT
#endif

#if defined(BCM_5397_A0)
#define BCM_5397
#endif

#if defined(BCM_5347_A0)
#define BCM_5347
#define BCM_FIELD_SUPPORT
#endif
#if defined(BCM_5395_A0)
#define BCM_5395
#define BCM_FIELD_SUPPORT
#define BCM_MAC2V_SUPPORT
#endif

#if defined(BCM_53242_A0)
#define BCM_53242
#define BCM_FIELD_SUPPORT
/* Below used in ROBO chip only. No exist in ESW */
#define BCM_V2V_SUPPORT
#define BCM_MAC2V_SUPPORT
#define BCM_PROTOCOL2V_SUPPORT
#define BCM_HARRIER_SUPPORT
/* not supported yet. 
#define BCM_EGRV2V_SUPPORT 
*/
#endif

#if defined(BCM_53262_A0)
#define BCM_53262
#define BCM_FIELD_SUPPORT
/* Below used in ROBO chip only. No exist in ESW */
#define BCM_V2V_SUPPORT
#define BCM_MAC2V_SUPPORT
#define BCM_PROTOCOL2V_SUPPORT
#define BCM_HARRIER_SUPPORT
/* not supported yet. 
#define BCM_EGRV2V_SUPPORT 
*/
#endif

#if defined(BCM_53115_A0)
#define BCM_53115
#define BCM_FIELD_SUPPORT
#define BCM_V2V_SUPPORT
#define BCM_GEX_SUPPORT
#define BCM_VULCAN_SUPPORT
#endif

#if defined(BCM_53118_A0)
#define BCM_53118
#define BCM_GEX_SUPPORT
#define BCM_BLACKBIRD_SUPPORT
#endif

#if defined(BCM_53280_A0) || defined(BCM_53280_B0) \
 || defined(BCM_53600_A0)
#define BCM_53280
#define BCM_FIELD_SUPPORT
#define BCM_V2V_SUPPORT
#define BCM_PROTOCOL2V_SUPPORT
#define BCM_TBX_SUPPORT
#endif

#if defined(BCM_53101_A0)
#define BCM_53101
#define BCM_GEX_SUPPORT
#define BCM_LOTUS_SUPPORT
#endif

#if defined(BCM_53125_A0)
#define BCM_53125
#define BCM_FIELD_SUPPORT
#define BCM_V2V_SUPPORT
#define BCM_GEX_SUPPORT
#define BCM_STARFIGHTER_SUPPORT
#endif
 
#if defined(BCM_53128_A0)
#define BCM_53128
#define BCM_FIELD_SUPPORT /* For Auto VoIP feature */ 
#define BCM_GEX_SUPPORT
#define BCM_BLACKBIRD2_SUPPORT
#endif

#if defined(BCM_53600_A0)
#define BCM_53600
#endif

#if defined(BCM_89500_A0)
#define BCM_89500
#define BCM_FIELD_SUPPORT
#define BCM_V2V_SUPPORT
#define BCM_GEX_SUPPORT
#endif

#if defined(BCM_53010_A0)
#define BCM_53010
#define BCM_FIELD_SUPPORT
#define BCM_V2V_SUPPORT
#define BCM_GEX_SUPPORT
#endif

#if defined(BCM_53020_A0)
#define BCM_53020
#define BCM_FIELD_SUPPORT
#define BCM_V2V_SUPPORT
#define BCM_GEX_SUPPORT
#endif


#if defined(INCLUDE_ACL)
#define BCM_ACL_SUPPORT
#endif

#if defined(BCM_XGS_SUPPORT)
#ifndef BCM_ESW_SUPPORT
#define BCM_ESW_SUPPORT
#endif
#endif

#if defined(INCLUDE_RCPU)
#define BCM_RCPU_SUPPORT
#endif

#if defined(INCLUDE_OOB_RCPU)
#define BCM_OOB_RCPU_SUPPORT
#endif

    
/*
 * ROBO chips
 */
#if defined(BCM_5324) || defined(BCM_5396) || defined(BCM_5389) || defined(BCM_5398) || \
     defined(BCM_5348) || defined(BCM_5397) || defined(BCM5347) ||  defined(BCM_5395) ||\
     defined(BCM_53242) || defined (BCM_53262) || defined (BCM_53115) ||\
     defined (BCM_53118) || defined(BCM_53280) || defined(BCM_53101) ||\
     defined (BCM_53600)
#ifndef BCM_ROBO_SUPPORT
#define BCM_ROBO_SUPPORT
#endif
#endif

#if defined(BCM_53280) || defined(BCM_53600)
#define BCM_TB_SUPPORT
#endif

#if defined(BCM_53600)
#define BCM_VO_SUPPORT
#endif

#if defined(BCM_89500)
#define BCM_POLAR_SUPPORT
#endif

#if defined(BCM_53010)
#define BCM_NORTHSTAR_SUPPORT
#endif

#if defined(BCM_53020)
#define BCM_NORTHSTARPLUS_SUPPORT

#endif

/* MACSEC special definition for those switch device within bounded MACSEC supports. */
#ifdef INCLUDE_MACSEC
/*  Current MACSEC solution in SDK can be classified to two solutions.
 *      1. PHY-MACSEC : MACSEC is bounded in a stand-along PHY chip.
 *      2. Switch-MACSEC : MACSEC is bounded with switch device.
 *          - BCM_SWITCHMACSEC_SUPPORT is specified for this solution.
 */
#if defined(BCM_NORTHSTARPLUS_SUPPORT)
#define BCM_SWITCHMACSEC_SUPPORT
#endif  /* NORTHSTARPLUS */
#endif  /* INCLUDE_MACSEC */

/****************************************************************
 *
 * This is a list of all known chips which may or may not be supported
 * by a given image.
 *
 * Use soc_chip_supported() for which are supported
 * CHANGE soc_chip_type_map if you change this
 *
 * See also socchip.h
 *
 * Enumerated types include:
 *   Unit numbers
 *   Chip type numbers (specific chip references w/ rev id)
 *   Chip group numbers (groups of chips, usually dropping revision
 *        number)
 *
 * All of the above are 0-based.
 *
 ****************************************************************/

typedef enum soc_chip_types_e {
    SOC_CHIP_BCM5670_A0,
    SOC_CHIP_BCM5673_A0,
    SOC_CHIP_BCM5690_A0,
    SOC_CHIP_BCM5665_A0,
    SOC_CHIP_BCM5695_A0,
    SOC_CHIP_BCM5675_A0,
    SOC_CHIP_BCM5674_A0,
    SOC_CHIP_BCM5665_B0,
    SOC_CHIP_BCM56601_A0,
    SOC_CHIP_BCM56601_B0,
    SOC_CHIP_BCM56601_C0,
    SOC_CHIP_BCM56602_A0,
    SOC_CHIP_BCM56602_B0,
    SOC_CHIP_BCM56602_C0,
    SOC_CHIP_BCM56504_A0,
    SOC_CHIP_BCM56504_B0,
    SOC_CHIP_BCM56102_A0,
    SOC_CHIP_BCM56304_B0,
    SOC_CHIP_BCM56112_A0,
    SOC_CHIP_BCM56314_A0,
    SOC_CHIP_BCM5650_C0,
    SOC_CHIP_BCM56800_A0,
    SOC_CHIP_BCM56218_A0,
    SOC_CHIP_BCM56514_A0,
    SOC_CHIP_BCM56624_A0,
    SOC_CHIP_BCM56624_B0,
    SOC_CHIP_BCM56680_A0,
    SOC_CHIP_BCM56680_B0,
    SOC_CHIP_BCM56224_A0,
    SOC_CHIP_BCM56224_B0,
    SOC_CHIP_BCM53314_A0,
    SOC_CHIP_BCM53324_A0,
    SOC_CHIP_BCM56725_A0,
    SOC_CHIP_BCM56820_A0,
    SOC_CHIP_BCM56634_A0,
    SOC_CHIP_BCM56634_B0,
    SOC_CHIP_BCM56524_A0,
    SOC_CHIP_BCM56524_B0,
    SOC_CHIP_BCM56685_A0,
    SOC_CHIP_BCM56685_B0,
    SOC_CHIP_BCM56334_A0,
    SOC_CHIP_BCM56334_B0,
    SOC_CHIP_BCM56840_A0,
    SOC_CHIP_BCM56840_B0,
    SOC_CHIP_BCM56850_A0,
    SOC_CHIP_BCM5324_A0,
    SOC_CHIP_BCM5324_A1,
    SOC_CHIP_BCM5324_A2,
    SOC_CHIP_BCM5396_A0,
    SOC_CHIP_BCM5389_A0,
    SOC_CHIP_BCM5398_A0,
    SOC_CHIP_BCM5348_A0,
    SOC_CHIP_BCM5397_A0,
    SOC_CHIP_BCM5347_A0,
    SOC_CHIP_BCM5395_A0,
    SOC_CHIP_BCM53242_A0,
    SOC_CHIP_BCM53262_A0,
    SOC_CHIP_BCM53115_A0,
    SOC_CHIP_BCM53118_A0,
    SOC_CHIP_BCM53280_A0,
    SOC_CHIP_BCM53280_B0,
    SOC_CHIP_BCM53101_A0,
    SOC_CHIP_BCM53125_A0,
    SOC_CHIP_BCM53128_A0,
    SOC_CHIP_BCM53600_A0,
    SOC_CHIP_BCM89500_A0,
    SOC_CHIP_BCM53010_A0,
    SOC_CHIP_BCM53020_A0,
    SOC_CHIP_BCM4713_A0,
    SOC_CHIP_QE2000_A4,
    SOC_CHIP_BME3200_A0,
    SOC_CHIP_BME3200_B0,
    SOC_CHIP_BM9600_A0,
    SOC_CHIP_BM9600_B0,
    SOC_CHIP_BCM88020_A0, /* FE2000_A0 */
    SOC_CHIP_BCM88020_A1, /* FE2000_A1 */
    SOC_CHIP_BCM88025_A0, /* FE2000XT_A0 */
    SOC_CHIP_BCM88230_A0,
    SOC_CHIP_BCM88230_B0,
    SOC_CHIP_BCM88230_C0,
    SOC_CHIP_BCM56142_A0,
    SOC_CHIP_BCM56150_A0,
    SOC_CHIP_BCM88640_A0,
    SOC_CHIP_BCM88650_A0,
    SOC_CHIP_BCM88650_B0,
    SOC_CHIP_BCM88650_B1,
    SOC_CHIP_BCM88660_A0,
    SOC_CHIP_BCM88670_A0,
    SOC_CHIP_BCM88202_A0,
    SOC_CHIP_BCM2801PM_A0,
    /* JERICHO-2-P3 */
    SOC_CHIP_BCM88850_P3,
    SOC_CHIP_BCM88732_A0,
    SOC_CHIP_BCM56640_A0,
    SOC_CHIP_BCM56640_B0,
    SOC_CHIP_BCM56340_A0, /* Helix4 */
    SOC_CHIP_BCM56440_A0,
    SOC_CHIP_BCM56440_B0,
    SOC_CHIP_BCM56450_A0,
    SOC_CHIP_BCM56450_B0,
    SOC_CHIP_BCM88030_A0, /* caladan 3 */
    SOC_CHIP_BCM88030_A1, /* caladan 3 */
    SOC_CHIP_BCM88030_B0, /* caladan 3 */
    SOC_CHIP_TK371X_A0,
    SOC_CHIP_BCM88750_A0,
    SOC_CHIP_BCM88750_B0,
    SOC_CHIP_BCM88754_A0,
    SOC_CHIP_BCM88755_B0,
    SOC_CHIP_BCM88950_A0,
    SOC_CHIP_ACP,
    SOC_CHIP_BCM53400_A0,
    SOC_CHIP_TYPES_COUNT
} soc_chip_types;

/****************************************************************
 * 
 * NB: Order of this array must match soc_chip_types_e above.
 *
 ****************************************************************/

#define SOC_CHIP_TYPE_MAP_INIT \
    /* SOC_CHIP_BCM5670_A0,  */ SOC_CHIP_BCM5670,  \
    /* SOC_CHIP_BCM5673_A0,  */ SOC_CHIP_BCM5673,  \
    /* SOC_CHIP_BCM5690_A0,  */ SOC_CHIP_BCM5690,  \
    /* SOC_CHIP_BCM5665_A0,  */ SOC_CHIP_BCM5665,  \
    /* SOC_CHIP_BCM5695_A0,  */ SOC_CHIP_BCM5695,  \
    /* SOC_CHIP_BCM5675_A0,  */ SOC_CHIP_BCM5675,  \
    /* SOC_CHIP_BCM5674_A0,  */ SOC_CHIP_BCM5674,  \
    /* SOC_CHIP_BCM5665_B0,  */ SOC_CHIP_BCM5665,  \
    /* SOC_CHIP_BCM56601_A0, */ SOC_CHIP_BCM56601, \
    /* SOC_CHIP_BCM56601_B0, */ SOC_CHIP_BCM56601, \
    /* SOC_CHIP_BCM56601_C0, */ SOC_CHIP_BCM56601, \
    /* SOC_CHIP_BCM56602_A0, */ SOC_CHIP_BCM56602, \
    /* SOC_CHIP_BCM56602_B0, */ SOC_CHIP_BCM56602, \
    /* SOC_CHIP_BCM56602_C0, */ SOC_CHIP_BCM56602, \
    /* SOC_CHIP_BCM56504_A0, */ SOC_CHIP_BCM56504, \
    /* SOC_CHIP_BCM56504_B0, */ SOC_CHIP_BCM56504, \
    /* SOC_CHIP_BCM56102_A0, */ SOC_CHIP_BCM56102, \
    /* SOC_CHIP_BCM56304_B0, */ SOC_CHIP_BCM56304, \
    /* SOC_CHIP_BCM56112_A0, */ SOC_CHIP_BCM56112, \
    /* SOC_CHIP_BCM56314_A0, */ SOC_CHIP_BCM56314, \
    /* SOC_CHIP_BCM5650_C0,  */ SOC_CHIP_BCM5650,  \
    /* SOC_CHIP_BCM56800_A0, */ SOC_CHIP_BCM56800, \
    /* SOC_CHIP_BCM56218_A0, */ SOC_CHIP_BCM56218, \
    /* SOC_CHIP_BCM56514_A0, */ SOC_CHIP_BCM56514, \
    /* SOC_CHIP_BCM56624_A0, */ SOC_CHIP_BCM56624, \
    /* SOC_CHIP_BCM56624_B0, */ SOC_CHIP_BCM56624, \
    /* SOC_CHIP_BCM56680_A0, */ SOC_CHIP_BCM56680, \
    /* SOC_CHIP_BCM56680_B0, */ SOC_CHIP_BCM56680, \
    /* SOC_CHIP_BCM56224_A0, */ SOC_CHIP_BCM56224, \
    /* SOC_CHIP_BCM56224_B0, */ SOC_CHIP_BCM56224, \
    /* SOC_CHIP_BCM53314_A0, */ SOC_CHIP_BCM53314, \
    /* SOC_CHIP_BCM53324_A0, */ SOC_CHIP_BCM53314, \
    /* SOC_CHIP_BCM56725_A0, */ SOC_CHIP_BCM56725, \
    /* SOC_CHIP_BCM56820_A0, */ SOC_CHIP_BCM56820, \
    /* SOC_CHIP_BCM56634_A0, */ SOC_CHIP_BCM56634, \
    /* SOC_CHIP_BCM56634_B0, */ SOC_CHIP_BCM56634, \
    /* SOC_CHIP_BCM56524_A0, */ SOC_CHIP_BCM56524, \
    /* SOC_CHIP_BCM56524_B0, */ SOC_CHIP_BCM56524, \
    /* SOC_CHIP_BCM56685_A0, */ SOC_CHIP_BCM56685, \
    /* SOC_CHIP_BCM56685_B0, */ SOC_CHIP_BCM56685, \
    /* SOC_CHIP_BCM56334_A0, */ SOC_CHIP_BCM56334, \
    /* SOC_CHIP_BCM56334_B0, */ SOC_CHIP_BCM56334, \
    /* SOC_CHIP_BCM56840_A0, */ SOC_CHIP_BCM56840, \
    /* SOC_CHIP_BCM56840_B0, */ SOC_CHIP_BCM56840, \
    /* SOC_CHIP_BCM56850_A0, */ SOC_CHIP_BCM56850, \
    /* SOC_CHIP_BCM5324_A0,  */ SOC_CHIP_BCM5324,  \
    /* SOC_CHIP_BCM5324_A1,  */ SOC_CHIP_BCM5324,  \
    /* SOC_CHIP_BCM5324_A2,  */ SOC_CHIP_BCM5324,  \
    /* SOC_CHIP_BCM5396_A0,  */ SOC_CHIP_BCM5396,  \
    /* SOC_CHIP_BCM5389_A0,  */ SOC_CHIP_BCM5389,  \
    /* SOC_CHIP_BCM5398_A0,  */ SOC_CHIP_BCM5398,  \
    /* SOC_CHIP_BCM5348_A0,  */ SOC_CHIP_BCM5348,  \
    /* SOC_CHIP_BCM5397_A0,  */ SOC_CHIP_BCM5397,  \
    /* SOC_CHIP_BCM5347_A0,  */ SOC_CHIP_BCM5347,  \
    /* SOC_CHIP_BCM5395_A0,  */ SOC_CHIP_BCM5395,  \
    /* SOC_CHIP_BCM53242_A0, */ SOC_CHIP_BCM53242, \
    /* SOC_CHIP_BCM53262_A0, */ SOC_CHIP_BCM53262, \
    /* SOC_CHIP_BCM53115_A0, */ SOC_CHIP_BCM53115, \
    /* SOC_CHIP_BCM53118_A0, */ SOC_CHIP_BCM53118, \
    /* SOC_CHIP_BCM53280_A0, */ SOC_CHIP_BCM53280, \
    /* SOC_CHIP_BCM53280_B0, */ SOC_CHIP_BCM53280, \
    /* SOC_CHIP_BCM53101_A0, */ SOC_CHIP_BCM53101, \
    /* SOC_CHIP_BCM53125_A0, */ SOC_CHIP_BCM53125, \
    /* SOC_CHIP_BCM53128_A0, */ SOC_CHIP_BCM53128, \
    /* SOC_CHIP_BCM53600_A0, */ SOC_CHIP_BCM53600, \
    /* SOC_CHIP_BCM89500_A0, */ SOC_CHIP_BCM89500, \
    /* SOC_CHIP_BCM53010_A0, */ SOC_CHIP_BCM53010, \
    /* SOC_CHIP_BCM53020_A0, */ SOC_CHIP_BCM53020, \
    /* SOC_CHIP_BCM4713_A0,  */ SOC_CHIP_BCM4713,  \
    /* SOC_CHIP_QE2000_A4,   */ SOC_CHIP_QE2000,   \
    /* SOC_CHIP_BME3200_A0,  */ SOC_CHIP_BME3200,  \
    /* SOC_CHIP_BME3200_B0,  */ SOC_CHIP_BME3200,  \
    /* SOC_CHIP_BM9600_A0,   */ SOC_CHIP_BM9600,   \
    /* SOC_CHIP_BM9600_B0,   */ SOC_CHIP_BM9600,   \
    /* SOC_CHIP_FE2000_A0,   */ SOC_CHIP_BCM88020, \
    /* SOC_CHIP_FE2000_A1,   */ SOC_CHIP_BCM88020, \
    /* SOC_CHIP_FE2000XT_A0, */ SOC_CHIP_BCM88020, \
    /* SOC_CHIP_BCM88230_A0, */ SOC_CHIP_BCM88230, \
    /* SOC_CHIP_BCM88230_B0, */ SOC_CHIP_BCM88230, \
    /* SOC_CHIP_BCM88230_C0, */ SOC_CHIP_BCM88230, \
    /* SOC_CHIP_BCM56142_A0, */ SOC_CHIP_BCM56142, \
    /* SOC_CHIP_BCM56150_A0, */ SOC_CHIP_BCM56150, \
    /* SOC_CHIP_BCM88640_A0  */ SOC_CHIP_BCM88640, \
    /* SOC_CHIP_BCM88650_A0  */ SOC_CHIP_BCM88650, \
    /* SOC_CHIP_BCM88650_B0  */ SOC_CHIP_BCM88650, \
    /* SOC_CHIP_BCM88650_B1  */ SOC_CHIP_BCM88650, \
    /* SOC_CHIP_BCM88660_A0  */ SOC_CHIP_BCM88660, \
    /* SOC_CHIP_BCM88670_A0  */ SOC_CHIP_BCM88670, \
    /* SOC_CHIP_BCM88202_A0, */ SOC_CHIP_BCM88202, \
    /* SOC_CHIP_BCM2801PM_A0 */ SOC_CHIP_BCM2801PM, \
    /* JERICHO-2-P3 */ /* SOC_CHIP_BCM88850_P3  */ SOC_CHIP_BCM88850, \
    /* SOC_CHIP_BCM88732_A0, */ SOC_CHIP_BCM88732, \
    /* SOC_CHIP_BCM56640_A0, */ SOC_CHIP_BCM56640, \
    /* SOC_CHIP_BCM56640_B0, */ SOC_CHIP_BCM56640, \
    /* SOC_CHIP_BCM56340_A0, */ SOC_CHIP_BCM56340, \
    /* SOC_CHIP_BCM56440_A0  */ SOC_CHIP_BCM56440, \
    /* SOC_CHIP_BCM56440_B0  */ SOC_CHIP_BCM56440, \
    /* SOC_CHIP_BCM56450_A0  */ SOC_CHIP_BCM56450, \
    /* SOC_CHIP_BCM56450_B0  */ SOC_CHIP_BCM56450, \
    /* SOC_CHIP_BCM88030_A0  */ SOC_CHIP_BCM88030, \
    /* SOC_CHIP_BCM88030_A1  */ SOC_CHIP_BCM88030, \
    /* SOC_CHIP_BCM88030_B0  */ SOC_CHIP_BCM88030, \
    /* SOC_CHIP_TK371X_A0    */ SOC_CHIP_TK371X,   \
    /* SOC_CHIP_BCM88750_A0  */ SOC_CHIP_BCM88750,  \
    /* SOC_CHIP_BCM88750_B0  */ SOC_CHIP_BCM88750,  \
    /* SOC_CHIP_BCM88754_A0  */ SOC_CHIP_BCM88750,  \
    /* SOC_CHIP_BCM88755_B0  */ SOC_CHIP_BCM88750,  \
    /* SOC_CHIP_BCM88950_A0  */ SOC_CHIP_BCM88950,  \
    /* SOC_CHIP_ACP  */         SOC_CHIP_BCM88650ACP,\
    /* SOC_CHIP_BCM53400_A0 */  SOC_CHIP_BCM53400, 



#define SOC_CHIP_TYPE_NAMES_INIT \
    "BCM5670_A0",  \
    "BCM5673_A0",  \
    "BCM5690_A0",  \
    "BCM5665_A0",  \
    "BCM5695_A0",  \
    "BCM5675_A0",  \
    "BCM5674_A0",  \
    "BCM5665_B0",  \
    "BCM56601_A0", \
    "BCM56601_B0", \
    "BCM56601_C0", \
    "BCM56602_A0", \
    "BCM56602_B0", \
    "BCM56602_C0", \
    "BCM56504_A0", \
    "BCM56504_B0", \
    "BCM56102_A0", \
    "BCM56304_B0", \
    "BCM56112_A0", \
    "BCM56314_A0", \
    "BCM5650_C0",  \
    "BCM56800_A0", \
    "BCM56218_A0", \
    "BCM56514_A0", \
    "BCM56624_A0", \
    "BCM56624_B0", \
    "BCM56680_A0", \
    "BCM56680_B0", \
    "BCM56224_A0", \
    "BCM56224_B0", \
    "BCM53314_A0", \
    "BCM53324_A0", \
    "BCM56725_A0", \
    "BCM56820_A0", \
    "BCM56634_A0", \
    "BCM56634_B0", \
    "BCM56524_A0", \
    "BCM56524_B0", \
    "BCM56685_A0", \
    "BCM56685_B0", \
    "BCM56334_A0", \
    "BCM56334_B0", \
    "BCM56840_A0", \
    "BCM56840_B0", \
    "BCM56850_A0", \
    "BCM5324_A0",  \
    "BCM5324_A1",  \
    "BCM5324_A2",  \
    "BCM5396_A0",  \
    "BCM5389_A0",  \
    "BCM5398_A0",  \
    "BCM5348_A0",  \
    "BCM5397_A0",  \
    "BCM5347_A0",  \
    "BCM5395_A0",  \
    "BCM53242_A0", \
    "BCM53262_A0", \
    "BCM53115_A0", \
    "BCM53118_A0", \
    "BCM53280_A0", \
    "BCM53280_B0", \
    "BCM53101_A0", \
    "BCM53125_A0", \
    "BCM53128_A0", \
    "BCM53600_A0", \
    "BCM89500_A0", \
    "BCM53010_A0", \
    "BCM53020_A0", \
    "BCM4713_A0",  \
    "QE2000_A0",   \
    "BME3200_A0",  \
    "BME3200_B0",  \
    "BCM88130_A0", \
    "BCM88130_B0", \
    "BCM88020_A0", \
    "BCM88020_A1", \
    "BCM88025_A0", \
    "BCM88230_A0", \
    "BCM88230_B0", \
    "BCM88230_C0", \
    "BCM56142_A0", \
    "BCM56150_A0", \
    "BCM88640_A0", \
    "BCM88650_A0", \
    "BCM88650_B0", \
    "BCM88650_B1", \
    "BCM88660_A0", \
    "BCM88670_A0", \
    "BCM88202_A0", \
    "BCM2801PM_A0", \
    /* JERICHO-2-P3 */ "BCM88850_P3", \
    "BCM88732_A0", \
    "BCM56640_A0", \
    "BCM56640_B0", \
    "BCM56340_A0", \
    "BCM56440_A0", \
    "BCM56440_B0", \
    "BCM56450_A0", \
    "BCM56450_B0", \
    "BCM88030_A0", \
    "BCM88030_A1", \
    "BCM88030_B0", \
    "TK371X_A0",   \
    "BCM88750_A0", \
    "BCM88750_B0", \
    "BCM88754_A0", \
    "BCM88755_B0", \
    "BCM88950_A0", \
    "ACP",         \
    "BCM53400_A0",    




typedef enum soc_chip_groups_e {
    /* Chip names w/o revision */
    SOC_CHIP_BCM5670,
    SOC_CHIP_BCM5673,
    SOC_CHIP_BCM5690,
    SOC_CHIP_BCM5665,
    SOC_CHIP_BCM5695,
    SOC_CHIP_BCM5675,
    SOC_CHIP_BCM5674,
    SOC_CHIP_BCM56601,
    SOC_CHIP_BCM56602,
    SOC_CHIP_BCM56504,
    SOC_CHIP_BCM56102,
    SOC_CHIP_BCM56304,
    SOC_CHIP_BCM5650,
    SOC_CHIP_BCM56800,
    SOC_CHIP_BCM56218,
    SOC_CHIP_BCM56112,
    SOC_CHIP_BCM56314,
    SOC_CHIP_BCM56514,
    SOC_CHIP_BCM56624,
    SOC_CHIP_BCM56680,
    SOC_CHIP_BCM56224,
    SOC_CHIP_BCM53314,
    SOC_CHIP_BCM56725,
    SOC_CHIP_BCM56820,
    SOC_CHIP_BCM56634,
    SOC_CHIP_BCM56524,
    SOC_CHIP_BCM56685,
    SOC_CHIP_BCM56334,
    SOC_CHIP_BCM56840,
    SOC_CHIP_BCM56850,
    SOC_CHIP_BCM5324,
    SOC_CHIP_BCM5396,
    SOC_CHIP_BCM5389,
    SOC_CHIP_BCM5398,
    SOC_CHIP_BCM5348,
    SOC_CHIP_BCM5397,
    SOC_CHIP_BCM5347,
    SOC_CHIP_BCM5395,
    SOC_CHIP_BCM53242,
    SOC_CHIP_BCM53262,
    SOC_CHIP_BCM53115,
    SOC_CHIP_BCM53118,
    SOC_CHIP_BCM53280,
    SOC_CHIP_BCM53101,
    SOC_CHIP_BCM53125,
    SOC_CHIP_BCM53128,
    SOC_CHIP_BCM53600,
    SOC_CHIP_BCM89500,
    SOC_CHIP_BCM53010,
    SOC_CHIP_BCM53020,
    SOC_CHIP_BCM4713,
    SOC_CHIP_QE2000,
    SOC_CHIP_BME3200,
    SOC_CHIP_BM9600,
    SOC_CHIP_BCM88020,
    SOC_CHIP_BCM88230,
    SOC_CHIP_BCM56142,
    SOC_CHIP_BCM56150,
    SOC_CHIP_BCM88640,
    SOC_CHIP_BCM88650,
    SOC_CHIP_BCM88660,
    SOC_CHIP_BCM88670,
    SOC_CHIP_BCM88202,
    SOC_CHIP_BCM2801PM,
    /* JERICHO-2-P3 */
    SOC_CHIP_BCM88850,
    SOC_CHIP_BCM88732,
    SOC_CHIP_BCM56640,
    SOC_CHIP_BCM56340,
    SOC_CHIP_BCM56440,
    SOC_CHIP_BCM56450,
    SOC_CHIP_BCM88030,
    SOC_CHIP_TK371X,
    SOC_CHIP_BCM88750,
    SOC_CHIP_BCM88950,
    SOC_CHIP_BCM88650ACP,
    SOC_CHIP_BCM53400,    
    SOC_CHIP_GROUPS_COUNT
} soc_chip_groups_t;


#define SOC_CHIP_GROUP_NAMES_INIT \
    "BCM5670",  \
    "BCM5673",  \
    "BCM5690",  \
    "BCM5665",  \
    "BCM5695",  \
    "BCM5675",  \
    "BCM5674",  \
    "BCM56601", \
    "BCM56602", \
    "BCM56504", \
    "BCM56102", \
    "BCM56304", \
    "BCM5650",  \
    "BCM56800", \
    "BCM56218", \
    "BCM56112", \
    "BCM56314", \
    "BCM56514", \
    "BCM56624", \
    "BCM56680", \
    "BCM56224", \
    "BCM53314", \
    "BCM56725", \
    "BCM56820", \
    "BCM56634", \
    "BCM56524", \
    "BCM56685", \
    "BCM56334", \
    "BCM56840", \
    "BCM56850", \
    "BCM5324",  \
    "BCM5396",  \
    "BCM5389",  \
    "BCM5398",  \
    "BCM5348",  \
    "BCM5397",\
    "BCM5347",\
    "BCM5395",\
    "BCM53242",\
    "BCM53262",\
    "BCM53115", \
    "BCM53118", \
    "BCM53280", \
    "BCM53101", \
    "BCM53125", \
    "BCM53128", \
    "BCM53600", \
    "BCM89500", \
    "BCM53010", \
    "BCM53020", \
    "BCM4713",  \
    "QE2000",   \
    "BME3200",  \
    "BCM88130", \
    "BCM88020", \
    "BCM88230", \
    "BCM56142", \
    "BCM56150", \
    "BCM88640", \
    "BCM88650", \
    "BCM88660", \
    "BCM88670", \
    "BCM88202", \
    "BCM2801PM", \
    "BCM88850", \
    "BCM88732", \
    "BCM56640", \
    "BCM56340", \
    "BCM56440", \
    "BCM56450", \
    "BCM88030", \
    "TK371X",   \
    "BCM88750",  \
    "BCM88950",  \
    "SOC_CHIP_BCM88650ACP", \
    "BCM53400",


#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
#define SER_TR_TEST_SUPPORT
#endif

/*
 * If you add anything here, check soc/common.c for arrays
 * indexed by soc_regtype_t.
 */
typedef enum soc_regtype_t {
    soc_schan_reg,      /* Generic register read thru SCHAN */
    soc_genreg,         /* General soc registers */
    soc_portreg,        /* Port soc registers */
    soc_ppportreg,      /* Packet-Processing-Port soc registers */
    soc_cosreg,         /* COS soc registers */
    soc_cpureg,         /* AKA PCI memory */
    soc_pci_cfg_reg,    /* PCI configuration space register */
    soc_phy_reg,        /* PHY register, access thru mii */
    soc_spi_reg,        /* SPI relevant Registers*/
    soc_mcsreg,         /* Microcontroller Subsystem - Indirect Access */
    soc_iprocreg,       /* iProc Reg in AXI Address Space */
    soc_hostmem_w,      /* word */
    soc_hostmem_h,      /* half word */
    soc_hostmem_b,      /* byte */
    soc_invalidreg
} soc_regtype_t;

#endif  /* !_SOC_DEFS_H */

