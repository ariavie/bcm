/*
 * $Id: mbcm.c 1.46 Broadcom SDK $
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
 * File:        mbcm.c
 * Purpose:     Implementation of bcm multiplexing
 */


#include <soc/defs.h>

#include <bcm/error.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw_dispatch.h>

#include <bcm_int/common/family.h>

mbcm_functions_t	*mbcm_driver[BCM_MAX_NUM_UNITS];

/****************************************************************
 *
 * Function:        mbcm_init
 * Parameters:      unit   --   unit to setup
 * 
 * Initialize the mbcm driver for the indicated unit.
 *
 ****************************************************************/
int
mbcm_init(int unit)
{
#ifdef	BCM_HERCULES15_SUPPORT
    if (SOC_IS_HERCULES15(unit)) {
        mbcm_driver[unit] = &mbcm_hercules_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_HERCULES); /* Should become H15 */
	return BCM_E_NONE;
    }
#endif	/* BCM_HERCULES15_SUPPORT */
#ifdef	BCM_ALLLAYER_SUPPORT
    if (SOC_IS_ALLLAYER(unit)) {
        mbcm_driver[unit] = &mbcm_alllayer_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_ALLLAYER);         
        return BCM_E_NONE;
    }
#endif	/* BCM_ALLLAYER_SUPPORT */
#ifdef	BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FB_FX_HX(unit)) {
        mbcm_driver[unit] = &mbcm_firebolt_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_FIREBOLT); 
	return BCM_E_NONE;
    }
#endif	/* BCM_FIREBOLT_SUPPORT */
#ifdef	BCM_BRADLEY_SUPPORT
    if (SOC_IS_BRADLEY(unit)) {
        mbcm_driver[unit] = &mbcm_bradley_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_BRADLEY); 
	return BCM_E_NONE;
    }
#endif	/* BCM_BRADLEY_SUPPORT */
#ifdef	BCM_GOLDWING_SUPPORT
    if (SOC_IS_GOLDWING(unit)) {
        mbcm_driver[unit] = &mbcm_bradley_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_BRADLEY); 
        return BCM_E_NONE;
    }
#endif	/* BCM_GOLDWING_SUPPORT */
#ifdef	BCM_HUMV_SUPPORT
    if (SOC_IS_HUMV(unit)) {
        mbcm_driver[unit] = &mbcm_humv_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_HUMV); 
        return BCM_E_NONE;
    }
#endif	/* BCM_HUMV_SUPPORT */
#ifdef	BCM_TRIUMPH_SUPPORT
    if (SOC_IS_TRIUMPH(unit)) {
        mbcm_driver[unit] = &mbcm_triumph_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIUMPH); 
        return BCM_E_NONE;
    }
#endif	/* BCM_TRIUMPH_SUPPORT */
#ifdef	BCM_VALKYRIE_SUPPORT
    if (SOC_IS_VALKYRIE(unit)) {
        mbcm_driver[unit] = &mbcm_triumph_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIUMPH); 
        return BCM_E_NONE;
    }
#endif	/* BCM_VALKYRIE_SUPPORT */
#ifdef	BCM_SCORPION_SUPPORT
    if (SOC_IS_SCORPION(unit)) {
        mbcm_driver[unit] = &mbcm_scorpion_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_SCORPION); 
	return BCM_E_NONE;
    }
#endif	/* BCM_SCORPION_SUPPORT */
#ifdef	BCM_CONQUEROR_SUPPORT
    if (SOC_IS_CONQUEROR(unit)) {
        mbcm_driver[unit] = &mbcm_conqueror_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_CONQUEROR); 
	return BCM_E_NONE;
    }
#endif	/* BCM_CONQUEROR_SUPPORT */
#ifdef	BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit)) {
        mbcm_driver[unit] = &mbcm_triumph2_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIUMPH2); 
        return BCM_E_NONE;
    }
#endif	/* BCM_TRIUMPH2_SUPPORT */
#ifdef	BCM_APOLLO_SUPPORT
    if (SOC_IS_APOLLO(unit)) {
        mbcm_driver[unit] = &mbcm_triumph2_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIUMPH2); 
        return BCM_E_NONE;
    }
#endif	/* BCM_APOLLO_SUPPORT */
#ifdef	BCM_VALKYRIE2_SUPPORT
    if (SOC_IS_VALKYRIE2(unit)) {
        mbcm_driver[unit] = &mbcm_triumph2_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIUMPH2); 
        return BCM_E_NONE;
    }
#endif	/* BCM_VALKYRIE2_SUPPORT */
#ifdef	BCM_ENDURO_SUPPORT
    if (SOC_IS_ENDURO(unit)) {
        mbcm_driver[unit] = &mbcm_enduro_driver;
        
        bcm_chip_family_set(unit, BCM_FAMILY_TRIUMPH); 
        return BCM_E_NONE;
    }
#endif	/* BCM_ENDURO_SUPPORT */
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        mbcm_driver[unit] = &mbcm_conqueror_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_CONQUEROR);
       return BCM_E_NONE;
    }
#endif /* BCM_SHADOW_SUPPORT */
#ifdef	BCM_TRIDENT_SUPPORT
    if (SOC_IS_TRIDENT(unit)) {
        mbcm_driver[unit] = &mbcm_trident_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIDENT);
        return BCM_E_NONE;
    }
    if (SOC_IS_TITAN(unit)) {
        mbcm_driver[unit] = &mbcm_titan_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIDENT);
        return BCM_E_NONE;
    }
#endif	/* BCM_TRIDENT_SUPPORT */
#ifdef	BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)) {
        mbcm_driver[unit] = &mbcm_trident2_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIDENT);
        return BCM_E_NONE;
    }
    if (SOC_IS_TITAN2(unit)) {
        mbcm_driver[unit] = &mbcm_titan2_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIDENT);
        return BCM_E_NONE;
    }
#endif	/* BCM_TRIDENT2_SUPPORT */
#ifdef	BCM_HURRICANE_SUPPORT
    if (SOC_IS_HURRICANE(unit)) {
        mbcm_driver[unit] = &mbcm_hurricane_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIUMPH); 
        return BCM_E_NONE;
    }
#endif	/* BCM_HURRICANE_SUPPORT */
#ifdef	BCM_HURRICANE_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        mbcm_driver[unit] = &mbcm_hurricane2_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIUMPH);
        return BCM_E_NONE;
    }
#endif	/* BCM_HURRICANE_SUPPORT */
#ifdef  BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        mbcm_driver[unit] = &mbcm_triumph3_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIDENT);
        return BCM_E_NONE;
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef	BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        mbcm_driver[unit] = &mbcm_katana_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIDENT);
        return BCM_E_NONE;
    }
#endif	/* BCM_KATANA_SUPPORT */
#ifdef	BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        mbcm_driver[unit] = &mbcm_katana2_driver;
        bcm_chip_family_set(unit, BCM_FAMILY_TRIDENT);
        return BCM_E_NONE;
    }
#endif	/* BCM_KATANA2_SUPPORT */

    soc_cm_print("ERROR: mbcm_init unit %d: unsupported chip type\n", unit);
    return BCM_E_INTERNAL;
}

int mbcm_deinit(int unit)
{
    mbcm_driver[unit] = NULL;

    return BCM_E_NONE;
}
