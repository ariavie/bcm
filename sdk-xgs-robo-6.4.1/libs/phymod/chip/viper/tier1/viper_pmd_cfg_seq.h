/*----------------------------------------------------------------------
 * $Id: viper_pmd_cfg_seq.h,v 1.1.2.2 Broadcom SDK $ 
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
 * File       : viper_pmd_cfg_seq.h
 * Description: c functions implementing Tier1s for TEMod Serdes Driver
 *---------------------------------------------------------------------*/
/*
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
 *  $Id: a58b77b369dcd3e114c5db4ac703b7cf8df6f14c $
*/


#ifndef VIPER_PMD_CFG_SEQ_H 
#define VIPER_PMD_CFG_SEQ_H

typedef enum {
    VIPER_PRBS_POLYNOMIAL_7 = 0,
    VIPER_PRBS_POLYNOMIAL_15,
    VIPER_PRBS_POLYNOMIAL_23,
    VIPER_PRBS_POLYNOMIAL_31,
    VIPER_PRBS_POLYNOMIAL_TYPE_COUNT
} viper_prbs_poly_t;

/* SGMII SPEEDS */
#define VIPER_SGMII_SPEED_10M	0
#define VIPER_SGMII_SPEED_100M	1
#define VIPER_SGMII_SPEED_1000M	2

/* XGXS Control Mode defines */
#define VIPER_XGXS_MODE_XGXS		0x00
#define VIPER_XGXS_MODE_XGXS_nCC	0x01
#define VIPER_XGXS_MODE_INDLANE_OS8	0x04
#define VIPER_XGXS_MODE_INDLANE_OS5	0x05
#define VIPER_XGXS_MODE_INDLANE_OS4	0x06
#define VIPER_XGXS_MODE_PCI		0x07
#define VIPER_XGXS_MODE_XGXS_nLQ	0x08
#define VIPER_XGXS_MODE_XGXS_nLQnCC	0x09
#define VIPER_XGXS_MODE_PBYPASS		0x0A
#define VIPER_XGXS_MODE_PBYPASS_nDSK	0x0B
#define VIPER_XGXS_MODE_COMBO_CORE	0x0C
#define VIPER_XGXS_MODE_CLOCKS_OFF	0x0F

/* DIGITAL5 MISC8 Mode defines */
#define VIPER_MISC8_OSDR_MODE_OSX2      0x01
#define VIPER_MISC8_OSDR_MODE_OSX4      0x02
#define VIPER_MISC8_OSDR_MODE_OSX5      0x03

/* DIGITAL MISC1 Register CLOCK / Speed Defines */
#define VIPER_MISC1_CLK_25M		0
#define VIPER_MISC1_CLK_100M		1
#define VIPER_MISC1_CLK_125M		2
#define VIPER_MISC1_CLK_156p25M		3
#define VIPER_MISC1_CLK_187p5M		4
#define VIPER_MISC1_CLK_161p25M		5
#define VIPER_MISC1_CLK_50M		6
#define VIPER_MISC1_CLK_106p25M		7

/* speed definitiosn if force_speed_b5 = 1 */
#define VIPER_MISC1_10G_DXGXS		0x00
#define VIPER_MISC1_10p5G_HiG_DXGXS	0x01
#define VIPER_MISC1_10p5G_DXGXS		0x02
#define VIPER_MISC1_12p773G_HiG_DXGXS	0x03
#define VIPER_MISC1_12p773G_DXGXS	0x04
#define VIPER_MISC1_10G_XFI		0x05
#define VIPER_MISC1_40G_X4		0x06
#define VIPER_MISC1_20G_HiG_DXGXS	0x07
#define VIPER_MISC1_20G_DXGXS		0x08
#define VIPER_MISC1_10G_SFI		0x09
#define VIPER_MISC1_31p5G		0x0a
#define VIPER_MISC1_32p7G		0x0b
#define VIPER_MISC1_20G_SCR		0x0c
#define VIPER_MISC1_10G_HiG_DXGXS_SCR	0x0d
#define VIPER_MISC1_10G_DXGXS_SCR	0x0e
#define VIPER_MISC1_12G_R2		0x0f
#define VIPER_MISC1_10G_X2		0x10
#define VIPER_MISC1_40G_KR4		0x11
#define VIPER_MISC1_40G_CR4		0x12
#define VIPER_MISC1_100G_CR10		0x13
#define VIPER_MISC1_5G_HiG_DXGXS	0x14
#define VIPER_MISC1_5G_DXGXS		0x15
#define VIPER_MISC1_15p75_HiG_DXGXS	0x16

/* speed definitiosn if force_speed_b5 = 0 */
#define VIPER_MISC1_2500BRCM_X1		0x10
#define VIPER_MISC1_5000BRCM_X4		0x11
#define VIPER_MISC1_6000BRCM_X4		0x12
#define VIPER_MISC1_10GHiGig_X4		0x13
#define VIPER_MISC1_10GBASE_CX4		0x14
#define VIPER_MISC1_12GHiGig_X4		0x15
#define VIPER_MISC1_12p5GhiGig_X4	0x16
#define VIPER_MISC1_13GHiGig_X4		0x17
#define VIPER_MISC1_15GHiGig_X4		0x18
#define VIPER_MISC1_16GHiGig_X4		0x19
#define VIPER_MISC1_5000BRCM_X1		0x1a
#define VIPER_MISC1_6363BRCM_X1		0x1b
#define VIPER_MISC1_20GHiGig_X4		0x1c
#define VIPER_MISC1_21GHiGig_X4		0x1d
#define VIPER_MISC1_25p45GHiGig_X4	0x1e
#define VIPER_MISC1_10G_HiG_DXGXS	0x1f

extern int viper_pmd_force_ana_signal_detect (PHYMOD_ST *pa, int enable);   

extern int viper_prbs_lane_poly_get     (PHYMOD_ST *pa, uint8_t ln, viper_prbs_poly_t *prbs_poly);
extern int viper_prbs_lane_poly_set     (PHYMOD_ST *pa, uint8_t ln, viper_prbs_poly_t  prbs_poly);

extern int viper_prbs_lane_inv_data_get (PHYMOD_ST *pa, uint8_t ln, uint32_t  *inv_data);
extern int viper_prbs_lane_inv_data_set (PHYMOD_ST *pa, uint8_t ln, uint32_t   inv_data);
extern int viper_prbs_enable_get        (PHYMOD_ST *pa, uint8_t ln, uint32_t  *enable);
extern int viper_prbs_enable_set        (PHYMOD_ST *pa, uint8_t ln, uint32_t   enable);

extern int viper_tx_lane_reset          (PHYMOD_ST *pa, uint32_t  enable);   
extern int viper_rx_lane_reset          (PHYMOD_ST *pa, uint32_t  enable);   
extern int viper_mii_gloop_get          (PHYMOD_ST *pa, uint32_t *enable);   
extern int viper_mii_gloop_set          (PHYMOD_ST *pa, uint32_t  enable);   
extern int viper_mdio_reset             (PHYMOD_ST *pa, uint32_t  enable);   
extern int viper_pll_reset              (PHYMOD_ST *pa, uint32_t  enable);   

extern int viper_sgmii_master_aneg_100M (PHYMOD_ST *pa, uint8_t speed);
extern int viper_sgmii_slave_aneg_speed (PHYMOD_ST *pa);
extern int viper_pll_disable_forced_10G (PHYMOD_ST *pa);
extern int viper_pll_enable_forced_10G  (PHYMOD_ST *pa);
extern int viper_fiber_force_10G_CX4    (PHYMOD_ST *pa);
extern int viper_fiber_force_100FX      (PHYMOD_ST *pa);
extern int viper_fiber_force_2p5G       (PHYMOD_ST *pa);
extern int viper_sgmii_force_100m       (PHYMOD_ST *pa);
extern int viper_sgmii_force_10m        (PHYMOD_ST *pa);
extern int viper_fiber_force_10G        (PHYMOD_ST *pa);
extern int viper_sgmii_aneg_10M         (PHYMOD_ST *pa);
extern int viper_sgmii_force_1g         (PHYMOD_ST *pa);
extern int viper_fiber_force_1G         (PHYMOD_ST *pa);
extern int viper_fiber_AN_1G            (PHYMOD_ST *pa);
extern int viper_lpi_disable            (PHYMOD_ST *pa);
extern int viper_pll_disable            (PHYMOD_ST *pa);
extern int viper_pll_enable             (PHYMOD_ST *pa);
extern int viper_lane_reset             (PHYMOD_ST *pa);

extern int viper_tx_pol_set             (const PHYMOD_ST *pa, uint8_t val);
extern int viper_rx_pol_set             (const PHYMOD_ST *pa, uint8_t val);

extern int viper_tx_pol_get             (const PHYMOD_ST *pa, uint32_t *val);
extern int viper_rx_pol_get             (const PHYMOD_ST *pa, uint32_t *val);

extern int viper_global_loopback_set    (const PHYMOD_ST *pa, uint8_t enable);
extern int viper_global_loopback_get    (const PHYMOD_ST *pa, uint32_t *lpbk); 
extern int viper_get_link_status        (const PHYMOD_ST *pc, uint32_t *link);
extern int viper_pmd_lock_get           (const PHYMOD_ST *pc, uint32_t* lockStatus);
extern int viper_prbs                   (PHYMOD_ST *pa, uint8_t prbs_mode);

extern int viper_remote_loopback_ena    (PHYMOD_ST *pa);
extern int viper_pll_lock_speed_up      (PHYMOD_ST *pa);
extern int viper_forced_speed_up        (PHYMOD_ST *pa);
extern int viper_an_speed_up            (PHYMOD_ST *pa);

extern int viper_revid_read   (const PHYMOD_ST *pc, uint32_t *revid); 
extern int viper_autoneg_get  (const PHYMOD_ST *pc, phymod_autoneg_control_t  *an, uint32_t *an_done);
extern int viper_tx_lane_swap (PHYMOD_ST *pa, uint8_t ln0, uint8_t ln1, uint8_t ln2, uint8_t ln3);
extern int viper_rx_lane_swap (PHYMOD_ST *pa, uint8_t ln0, uint8_t ln1, uint8_t ln2, uint8_t ln3);

extern int viper_autoneg_status_get      (const PHYMOD_ST *pc, phymod_autoneg_status_t *status);
extern int viper_tsc_tx_pi_freq_override (const PHYMOD_ST *pa, uint8_t enable, int16_t freq_override_val); 


#endif /* VIPER_PMD_CFG_SEQ_H */
