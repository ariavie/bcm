/*
 * $Id: phydefs.h,v 1.38 Broadcom SDK $
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
 * File:     phydefs.h
 * Purpose:  Basic defines for PHYs.
 *           This should be included after all other SOC include files and
 *           before all other PHY include files.
 */
#ifndef _PHY_CONFIG_H
#define _PHY_CONFIG_H 

#include <soc/defs.h>

/* PHY/Switch dependicies */
#if defined(BCM_DRACO_SUPPORT)
#define INCLUDE_PHY_5690
#endif /* BCM_DRACO_SUPPORT */

#if defined(BCM_XGS_SUPPORT)
#define INCLUDE_PHY_XGXS1
#endif /* BCM_XGS_SUPPORT */

#if defined(BCM_EASYRIDER_SUPPORT)
#define INCLUDE_PHY_56XXX
#define INCLUDE_PHY_XGXS5
#endif /* BCM_EASYRIDER_SUPPORT */

#if defined(BCM_FIREBOLT_SUPPORT)
#define INCLUDE_PHY_56XXX
#define INCLUDE_PHY_XGXS5
#endif /* BCM_FIREBOLT_SUPPORT */

#if defined(BCM_HUMV_SUPPORT) || defined(BCM_BRADLEY_SUPPORT) 
#define INCLUDE_PHY_XGXS6
#endif /* BCM_HUMV_SUPPORT || BCM_BRADLEY_SUPPORT */

#if defined(BCM_GOLDWING_SUPPORT)
#define INCLUDE_PHY_56XXX
#define INCLUDE_PHY_XGXS6
#endif /* BCM_GOLDWING_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT)
#define INCLUDE_SERDES_100FX   
#define INCLUDE_PHY_XGXS5
#endif /* BCM_FIREBOLT2_SUPPORT */

#if defined(BCM_RAPTOR_SUPPORT)
#define INCLUDE_PHY_56XXX
#define INCLUDE_SERDES_COMBO
#endif /* BCM_RAPTOR_SUPPORT */

#if defined(BCM_RAVEN_SUPPORT)
#define INCLUDE_SERDES_65LP
#define INCLUDE_SERDES_COMBO65
#endif /* BCM_RAVEN_SUPPORT */

#if defined(BCM_HAWKEYE_SUPPORT)
#define INCLUDE_XGXS_QSGMII65
#endif /* BCM_HAWKEYE_SUPPORT */

#if defined(BCM_HURRICANE_SUPPORT)
#define INCLUDE_XGXS_QSGMII65
#define INCLUDE_XGXS_WCMOD
#define INCLUDE_XGXS_TSCMOD
#endif /* BCM_HURRICANE_SUPPORT */

#if defined(BCM_BM9600_SUPPORT)
#define INCLUDE_XGXS_HL65
#endif

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT)
/* #define INCLUDE_XGXS_WCMOD  :debug WL driver with trident WC B0 board -ravick*/
#define INCLUDE_XGXS_WC40
#endif

#if defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
#define INCLUDE_XGXS_WCMOD
#endif

#if defined(BCM_TRIUMPH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
#define INCLUDE_SERDES_65LP
#define INCLUDE_XGXS_HL65
#define INCLUDE_XGXS_16G
#endif /* BCM_TRIUMPH_SUPPORT */

#if defined(BCM_SCORPION_SUPPORT)
#define INCLUDE_SERDES_65LP
#define INCLUDE_XGXS_HL65
#define INCLUDE_XGXS_16G
#endif /* BCM_SCORPION_SUPPORT */

#if defined(BCM_KATANA_SUPPORT)
#define INCLUDE_XGXS_16G
#endif /* BCM_KATANA_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT)
#define INCLUDE_XGXS_16G
#define INCLUDE_XGXS_WCMOD
#define INCLUDE_XGXS_WC40    /*added for Helix 4 warpCore B0 */
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIDENT2_SUPPORT)
#define INCLUDE_XGXS_TSCMOD
#endif

#if defined(BCM_GREYHOUND_SUPPORT)
#if defined(PHYMOD_SUPPORT)
#define INCLUDE_XGXS_TSCE
#define INCLUDE_SERDES_QSGMIIE
#endif
#endif

#if defined(BCM_FE2000_SUPPORT)
#define INCLUDE_SERDES_100FX
#define INCLUDE_PHY_XGXS5
#define INCLUDE_XGXS_16G
#define INCLUDE_SERDES_65LP
#endif /* BCM_FE2000_SUPPORT */

/* PHY/PHY dependencies */
#if defined(INCLUDE_PHY_5464) && defined(BCM_ROBO_SUPPORT)
#define INCLUDE_PHY_5464_ROBO
#define INCLUDE_PHY_53XXX
#endif /* BCM_ROBO_SUPPORT */

#if defined(INCLUDE_PHY_5482) && defined(BCM_ROBO_SUPPORT)
#define INCLUDE_PHY_5482_ROBO
#define INCLUDE_PHY_53XXX
#endif /* BCM_ROBO_SUPPORT */

#if defined(INCLUDE_PHY_5464) && \
    (defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT))
#define INCLUDE_PHY_5464_ESW
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT */

#if defined(INCLUDE_PHY_5482) && \
    (defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT))
#define INCLUDE_PHY_5482_ESW
#endif /* BCM_ESW_SUPPORT || BCM_SBX_SUPPORT */

/* definition for the SW cablediag supporting on phy54684 */
#if defined(INCLUDE_PHY_54684)
#define INCLUDE_PHY_54684_ESW
#endif /* INCLUDE_PHY_54684 */

#if defined(INCLUDE_PHY_SERDES) && defined(BCM_ROBO_SUPPORT)
#define INCLUDE_SERDES_ROBO
#define INCLUDE_XGXS_16G
#define INCLUDE_SERDES_65LP
#define INCLUDE_SERDES_COMBO65
#endif /* INCLUDE_PHY_SERDES || BCM_ROBO_SUPPORT */

#if defined(INCLUDE_PHY_XGXS1) || defined(INCLUDE_PHY_XGXS5) || \
    defined(INCLUDE_PHY_XGXS6)
#define INCLUDE_PHY_XGXS
#endif 

#if defined(INCLUDE_SERDES_100FX)  || defined(INCLUDE_SERDES_65LP) || \
    defined (INCLUDE_SERDES_COMBO) || defined(INCLUDE_PHY_56XXX) || \
    defined(INCLUDE_PHY_53XXX)
#define INCLUDE_SERDES
#endif

#if defined(INCLUDE_PHY_8703) || defined(INCLUDE_PHY_8705) || \
    defined(INCLUDE_PHY_8706) || defined(INCLUDE_PHY_8750) || \
    defined(INCLUDE_PHY_84334)|| defined(INCLUDE_PHY_8072) || \
    defined(INCLUDE_PHY_8481)
#define INCLUDE_PHY_XEHG
#endif
#endif /* _PHY_DEFS_H */
