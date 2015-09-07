/*
 * $Id: robo_drv.h 1.13 Broadcom SDK $
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
 * 
 */
   

#ifndef _SOC_ROBO_DRV_H
#define _SOC_ROBO_DRV_H

typedef enum soc_robo_rev_id_e {
	SocRoboRevA,
	SocRoboRevB,
	SocRoboRevC
}soc_robo_rev_id_t;


typedef enum soc_robo_arch_type_e {
	SOC_ROBO_ARCH_FEX,
	SOC_ROBO_ARCH_TBX,
	SOC_ROBO_ARCH_VULCAN,
	SOC_ROBO_ARCH_DINO8
}soc_robo_arch_type_t;

typedef enum soc_robo_chip_type_e {
    SOC_ROBO_CHIP_53242,
    SOC_ROBO_CHIP_53262,
    SOC_ROBO_CHIP_53115,
    SOC_ROBO_CHIP_53118,
    SOC_ROBO_CHIP_53280,
    SOC_ROBO_CHIP_53101,
    SOC_ROBO_CHIP_53125,
    SOC_ROBO_CHIP_53128,
    SOC_ROBO_CHIP_53600,
    SOC_ROBO_CHIP_89500,
    SOC_ROBO_CHIP_53010,
    SOC_ROBO_CHIP_5389,
    SOC_ROBO_CHIP_53020	
}soc_robo_chip_type_t;


typedef struct soc_robo_control_s{
soc_robo_chip_type_t	chip_type;
soc_robo_rev_id_t	rev_id;
int			rev_num;
uint32			chip_bonding;
soc_robo_arch_type_t	arch_type;
}soc_robo_control_t;


/* Bonding option */
#define SOC_ROBO_BOND_M                     0x0001
#define SOC_ROBO_BOND_S                     0x0002
#define SOC_ROBO_BOND_V                     0x0004
#define SOC_ROBO_BOND_PHY_S3MII             0x0008
#define SOC_ROBO_BOND_PHY_COMBOSERDES       0x0010


/*
 * SOC_ROBO_* control driver macros
 */
#define SOC_ROBO_CONTROL(unit) \
        ((soc_robo_control_t *)(SOC_CONTROL(unit)->drv))

#define SOC_IS_ROBO_ARCH_FEX(unit) \
    (SOC_ROBO_CONTROL(unit) == NULL ? 0: \
    (SOC_ROBO_CONTROL(unit)->arch_type == SOC_ROBO_ARCH_FEX))

#define SOC_IS_ROBO_ARCH_GEX(unit) \
    (SOC_ROBO_CONTROL(unit) == NULL ? 0: \
    (SOC_ROBO_CONTROL(unit)->arch_type == SOC_ROBO_ARCH_GEX))

#ifdef BCM_TB_SUPPORT
#define SOC_IS_ROBO_ARCH_TBX(unit) \
    (SOC_ROBO_CONTROL(unit) == NULL ? 0: \
    (SOC_ROBO_CONTROL(unit)->arch_type == SOC_ROBO_ARCH_TBX))
#else
#define SOC_IS_ROBO_ARCH_TBX(unit) (0)
#endif

#define SOC_IS_ROBO_ARCH_VULCAN(unit) \
    (SOC_ROBO_CONTROL(unit) == NULL ? 0: \
    (SOC_ROBO_CONTROL(unit)->arch_type == SOC_ROBO_ARCH_VULCAN))


#ifdef BCM_TB_SUPPORT
#define SOC_IS_TB(unit)   \
    ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
     (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53280))

#define SOC_IS_TB_BX(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
        ((SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53280) && \
        (SOC_ROBO_CONTROL(unit)->rev_id == SocRoboRevB) ))


#define SOC_IS_TB_B0(unit) SOC_IS_TB_BX(unit)

#define SOC_IS_TB_AX(unit) \
    (SOC_ROBO_CONTROL(unit) == NULL ? 0: \
        ((SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53280) && \
        (SOC_ROBO_CONTROL(unit)->rev_id == SocRoboRevA) ))

#define SOC_IS_TBX(unit)   SOC_IS_ROBO_ARCH_TBX(unit)

#define SOC_IS_VO(unit) \
    (SOC_ROBO_CONTROL(unit) == NULL ? 0: \
    (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53600))

#else
#define SOC_IS_TB(unit)    (0)
#define SOC_IS_TB_BX(unit)   (0)
#define SOC_IS_TB_B0(unit)  (0)
#define SOC_IS_TB_AX(unit)  (0)
#define SOC_IS_TBX(unit)    (0)
#define SOC_IS_VO(unit) (0)

#endif

#define SOC_IS_ROBO53242(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
         (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53242))
         
#define SOC_IS_ROBO53262(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
         (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53262))

#define SOC_IS_HARRIER(unit)    \
        (SOC_IS_ROBO53242(unit) || SOC_IS_ROBO53262(unit))

#define SOC_IS_VULCAN(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
         (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53115))

#define SOC_IS_BLACKBIRD(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
         (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53118))
         
#define SOC_IS_LOTUS(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
         (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53101))

#define SOC_IS_STARFIGHTER(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
         (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53125))

#define SOC_IS_BLACKBIRD2(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
         (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53128))

#define SOC_IS_BLACKBIRD2_BOND_V(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
        ((SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53128) && \
        (SOC_ROBO_CONTROL(unit)->chip_bonding == SOC_ROBO_BOND_V) ))


#define SOC_IS_POLAR(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
         (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_89500))

#define SOC_IS_NORTHSTAR(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
         (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53010))

#define SOC_IS_DINO8(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
         (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_5389))

#define SOC_IS_NORTHSTARPLUS(unit)   \
        ((SOC_ROBO_CONTROL(unit) == NULL) ? 0: \
         (SOC_ROBO_CONTROL(unit)->chip_type == SOC_ROBO_CHIP_53020))

#define SOC_IS_GEX(unit)    \
    (SOC_IS_LOTUS(unit) || SOC_IS_VULCAN(unit) || \
    SOC_IS_BLACKBIRD(unit) || SOC_IS_STARFIGHTER(unit) || \
    SOC_IS_BLACKBIRD2(unit) || SOC_IS_POLAR(unit) || \
    SOC_IS_NORTHSTAR(unit) || SOC_IS_DINO8(unit) || \
    SOC_IS_NORTHSTARPLUS(unit))

#ifdef  BCM_ROBO_SUPPORT
#define SOC_IS_ROBO_ARCH(unit) \
    (SOC_IS_ROBO_ARCH_TBX(unit) ||\
    SOC_IS_ROBO_ARCH_FEX(unit) ||\
    SOC_IS_ROBO_ARCH_VULCAN(unit) ||\
    SOC_IS_DINO8(unit))
#else
#define SOC_IS_ROBO_ARCH(unit)  (0)
#endif

#define SOC_IS_ROBO_GE_SWITCH(unit) (SOC_IS_VULCAN(unit) || \
    SOC_IS_BLACKBIRD(unit) || SOC_IS_BLACKBIRD2(unit) || \
    SOC_IS_STARFIGHTER(unit) || SOC_IS_POLAR(unit) || \
    SOC_IS_NORTHSTAR(unit) || SOC_IS_DINO8(unit) || \
    SOC_IS_NORTHSTARPLUS(unit))


#endif
