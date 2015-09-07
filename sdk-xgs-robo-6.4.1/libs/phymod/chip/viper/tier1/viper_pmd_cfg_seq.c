/*
 *         
 * $Id: viper_pmd_cfg_seq.c,v 1.2.2.26 Broadcom SDK $
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
 *     
 */
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
 *  $Id: 7d13559611366fa3b0a1f109c3bc54af13c1f60a $
*/


#include <phymod/phymod.h>
#include "viper_common.h" 
#include "viper_pmd_cfg_seq.h" 
#include <phymod/chip/bcmi_viper_xgxs_defs.h>

#ifdef _HDR_FIX_
#else
#define LANEPRBSr_PRBS_INV3f_GET LANEPRBSr_PRBS_INV0f_GET
#define LANEPRBSr_PRBS_INV2f_GET LANEPRBSr_PRBS_INV0f_GET
#define LANEPRBSr_PRBS_INV1f_GET LANEPRBSr_PRBS_INV0f_GET

#define LANEPRBSr_PRBS_ORDER3f_GET LANEPRBSr_PRBS_ORDER0f_GET
#define LANEPRBSr_PRBS_ORDER2f_GET LANEPRBSr_PRBS_ORDER0f_GET
#define LANEPRBSr_PRBS_ORDER1f_GET LANEPRBSr_PRBS_ORDER0f_GET

#define LANEPRBSr_PRBS_INV3f_SET LANEPRBSr_PRBS_INV0f_SET
#define LANEPRBSr_PRBS_INV2f_SET LANEPRBSr_PRBS_INV0f_SET
#define LANEPRBSr_PRBS_INV1f_SET LANEPRBSr_PRBS_INV0f_SET

#define LANEPRBSr_PRBS_ORDER3f_SET LANEPRBSr_PRBS_ORDER0f_SET
#define LANEPRBSr_PRBS_ORDER2f_SET LANEPRBSr_PRBS_ORDER0f_SET
#define LANEPRBSr_PRBS_ORDER1f_SET LANEPRBSr_PRBS_ORDER0f_SET


#endif

int viper_prbs_lane_inv_data_get (PHYMOD_ST *pa, 
                                  uint8_t    lane_num, 
                                  uint32_t  *inv_data)
{
    LANEPRBSr_t tmp_reg;

    LANEPRBSr_CLR(tmp_reg);
    READ_LANEPRBSr(pa, &tmp_reg);

    switch(lane_num) {
        case 3:
             *inv_data = LANEPRBSr_PRBS_INV3f_GET(tmp_reg);
             break;
        case 2:
             *inv_data = LANEPRBSr_PRBS_INV2f_GET(tmp_reg);
             break;
        case 1:
             *inv_data = LANEPRBSr_PRBS_INV1f_GET(tmp_reg);
             break;
        case 0:
        default:
             *inv_data = LANEPRBSr_PRBS_INV0f_GET(tmp_reg);
             break;
    }

    return PHYMOD_E_NONE;
}

int viper_prbs_lane_poly_get (PHYMOD_ST *pa, 
                              uint8_t    lane_num, 
                              viper_prbs_poly_t *prbs_poly)
{
    LANEPRBSr_t tmp_reg;

    LANEPRBSr_CLR(tmp_reg);
    READ_LANEPRBSr(pa, &tmp_reg);

    switch(lane_num) {
        case 3:
             *prbs_poly = LANEPRBSr_PRBS_ORDER3f_GET(tmp_reg);
             break;
        case 2:
             *prbs_poly = LANEPRBSr_PRBS_ORDER2f_GET(tmp_reg);
             break;
        case 1:
             *prbs_poly = LANEPRBSr_PRBS_ORDER1f_GET(tmp_reg);
             break;
        case 0:
        default:
             *prbs_poly = LANEPRBSr_PRBS_ORDER0f_GET(tmp_reg);
             break;
    }

    return PHYMOD_E_NONE;
}

int viper_prbs_enable_get (PHYMOD_ST *pa, 
                           uint8_t    lane_num, 
                           uint32_t  *enable)
{
    LANEPRBSr_t tmp_reg;

    LANEPRBSr_CLR(tmp_reg);
    READ_LANEPRBSr(pa, &tmp_reg);

    switch(lane_num) {
        case 3:
             *enable = LANEPRBSr_PRBS_ORDER3f_GET(tmp_reg);
             break;
        case 2:
             *enable = LANEPRBSr_PRBS_ORDER2f_GET(tmp_reg);
             break;
        case 1:
             *enable = LANEPRBSr_PRBS_ORDER1f_GET(tmp_reg);
             break;
        case 0:
        default:
             *enable = LANEPRBSr_PRBS_ORDER0f_GET(tmp_reg);
             break;
    }

    return PHYMOD_E_NONE;
}

int viper_prbs_lane_inv_data_set (PHYMOD_ST *pa, 
                                  uint8_t    lane_num, 
                                  uint32_t   inv_data)
{
    LANEPRBSr_t tmp_reg;

    LANEPRBSr_CLR(tmp_reg);
    READ_LANEPRBSr(pa, &tmp_reg);

    switch(lane_num) {
        case 3:
             LANEPRBSr_PRBS_INV3f_SET(tmp_reg, inv_data);
             break;
        case 2:
             LANEPRBSr_PRBS_INV2f_SET(tmp_reg, inv_data);
             break;
        case 1:
             LANEPRBSr_PRBS_INV1f_SET(tmp_reg, inv_data);
             break;
        case 0:
        default:
             LANEPRBSr_PRBS_INV0f_SET(tmp_reg, inv_data);
             break;
    }

    MODIFY_LANEPRBSr(pa, tmp_reg);
    return PHYMOD_E_NONE;
}

int viper_prbs_lane_poly_set (PHYMOD_ST *pa, 
                              uint8_t    lane_num, 
                              viper_prbs_poly_t prbs_poly)
{
    LANEPRBSr_t tmp_reg;

    LANEPRBSr_CLR(tmp_reg);
    READ_LANEPRBSr(pa, &tmp_reg);

    switch(lane_num) {
        case 3:
             LANEPRBSr_PRBS_ORDER3f_SET(tmp_reg, prbs_poly);
             break;
        case 2:
             LANEPRBSr_PRBS_ORDER2f_SET(tmp_reg, prbs_poly);
             break;
        case 1:
             LANEPRBSr_PRBS_ORDER1f_SET(tmp_reg, prbs_poly);
             break;
        case 0:
        default:
             LANEPRBSr_PRBS_ORDER0f_SET(tmp_reg, prbs_poly);
             break;
    }

    MODIFY_LANEPRBSr(pa, tmp_reg);
    return PHYMOD_E_NONE;
}

int viper_prbs_enable_set (PHYMOD_ST *pa, 
                           uint8_t    lane_num, 
                           uint32_t   enable)
{
    LANEPRBSr_t tmp_reg;

    LANEPRBSr_CLR(tmp_reg);
    READ_LANEPRBSr(pa, &tmp_reg);

    switch(lane_num) {
        case 3:
             LANEPRBSr_PRBS_ORDER3f_SET(tmp_reg, enable);
             break;
        case 2:
             LANEPRBSr_PRBS_ORDER2f_SET(tmp_reg, enable);
             break;
        case 1:
             LANEPRBSr_PRBS_ORDER1f_SET(tmp_reg, enable);
             break;
        case 0:
        default:
             LANEPRBSr_PRBS_ORDER0f_SET(tmp_reg, enable);
             break;
    }

    MODIFY_LANEPRBSr(pa, tmp_reg);
    return PHYMOD_E_NONE;
}


/* tx lane reset */
int viper_tx_lane_reset (PHYMOD_ST *pa, 
                         uint32_t  enable)   
{
    LANERESETr_t reg_lane_reset;

    LANERESETr_CLR           (reg_lane_reset);
    LANERESETr_RESET_TXf_SET (reg_lane_reset, enable);
    MODIFY_LANERESETr        (pa,  reg_lane_reset);

    return PHYMOD_E_NONE;
}

/* rx lane reset */
int viper_rx_lane_reset (PHYMOD_ST *pa, 
                         uint32_t  enable)   
{
    LANERESETr_t reg_lane_reset;

    LANERESETr_CLR           (reg_lane_reset);
    LANERESETr_RESET_RXf_SET (reg_lane_reset, enable);
    MODIFY_LANERESETr        (pa,  reg_lane_reset);

    return PHYMOD_E_NONE;
}

/* pll reset */
int viper_pll_reset (PHYMOD_ST *pa, 
                     uint32_t  enable)   
{
    LANERESETr_t reg_lane_reset;

    LANERESETr_CLR           (reg_lane_reset);
    LANERESETr_RESET_PLLf_SET(reg_lane_reset, enable);
    MODIFY_LANERESETr        (pa,  reg_lane_reset);

    return PHYMOD_E_NONE;
}

/* mdio reset */
int viper_mdio_reset (PHYMOD_ST *pa, 
                      uint32_t  enable)   
{
    LANERESETr_t reg_lane_reset;

    LANERESETr_CLR            (reg_lane_reset);
    LANERESETr_RESET_MDIOf_SET(reg_lane_reset, enable);
    MODIFY_LANERESETr         (pa,  reg_lane_reset);

    return PHYMOD_E_NONE;
}


int viper_pmd_force_ana_signal_detect (PHYMOD_ST *pa, int enable)   
{
    RX_AFE_ANARXSIGDETr_t reg;

    RX_AFE_ANARXSIGDETr_CLR(reg);
    RX_AFE_ANARXSIGDETr_RX_SIGDET_Rf_SET(reg, enable);
    RX_AFE_ANARXSIGDETr_RX_SIGDET_FORCE_Rf_SET(reg, enable);

    MODIFY_RX_AFE_ANARXSIGDETr(pa, reg);

    return PHYMOD_E_NONE;
}

int viper_mii_gloop_get(PHYMOD_ST *pa, uint32_t *enable)   
{
    LANECTL2r_t reg;

    READ_LANECTL2r(pa, &reg);

    *enable = LANECTL2r_GLOOP1Gf_GET(reg);
    return PHYMOD_E_NONE;
}

int viper_mii_gloop_set(PHYMOD_ST *pa, uint32_t enable)   
{
    LANECTL2r_t reg;

    READ_LANECTL2r(pa, &reg);
    LANECTL2r_GLOOP1Gf_SET(reg, enable);
    MODIFY_LANECTL2r(pa, reg);

    return PHYMOD_E_NONE;
}

int viper_pll_disable(PHYMOD_ST *pa)
{
    XGXSCTLr_t reg;

    XGXSCTLr_CLR(reg);

    /* Disable PLL 0x8000 : 0x052F */
    XGXSCTLr_EDENf_SET      (reg, 1);
    XGXSCTLr_CDET_ENf_SET   (reg, 1);
    XGXSCTLr_AFRST_ENf_SET  (reg, 1);
    XGXSCTLr_TXCKO_DIVf_SET (reg, 1);
    XGXSCTLr_RESERVED_5f_SET(reg, 1);
    XGXSCTLr_MODEf_SET      (reg, VIPER_XGXS_MODE_INDLANE_OS5);

    MODIFY_XGXSCTLr(pa, reg);
    return PHYMOD_E_NONE;
}

int viper_pll_disable_forced_10G(PHYMOD_ST *pa)
{
    XGXSCTLr_t reg;

    XGXSCTLr_CLR(reg);

    /* Disable PLL 0x8000 : 0x0C2F */
    XGXSCTLr_EDENf_SET      (reg, 1);
    XGXSCTLr_CDET_ENf_SET   (reg, 1);
    XGXSCTLr_AFRST_ENf_SET  (reg, 1);
    XGXSCTLr_TXCKO_DIVf_SET (reg, 1);
    XGXSCTLr_RESERVED_5f_SET(reg, 1);
    XGXSCTLr_MODEf_SET      (reg, VIPER_XGXS_MODE_COMBO_CORE);

    MODIFY_XGXSCTLr(pa, reg);
    return PHYMOD_E_NONE;
}

int viper_pll_enable(PHYMOD_ST *pa)
{
    XGXSCTLr_t reg;

    XGXSCTLr_CLR(reg);

    /* Disable PLL 0x8000 : 0x252F */
    XGXSCTLr_START_SEQUENCERf_SET (reg, 1);
    XGXSCTLr_MODEf_SET            (reg, VIPER_XGXS_MODE_INDLANE_OS5);
    XGXSCTLr_EDENf_SET            (reg, 1);
    XGXSCTLr_CDET_ENf_SET         (reg, 1);
    XGXSCTLr_AFRST_ENf_SET        (reg, 1);
    XGXSCTLr_TXCKO_DIVf_SET       (reg, 1);
    XGXSCTLr_RESERVED_5f_SET      (reg, 1);

    MODIFY_XGXSCTLr(pa, reg);
    return PHYMOD_E_NONE;
}

int viper_pll_enable_forced_10G(PHYMOD_ST *pa)
{
    XGXSCTLr_t reg;

    XGXSCTLr_CLR(reg);

    /* Disable PLL 0x8000 : 0x2C2F */
    XGXSCTLr_START_SEQUENCERf_SET (reg, 1);
    XGXSCTLr_MODEf_SET            (reg, VIPER_XGXS_MODE_COMBO_CORE);
    XGXSCTLr_EDENf_SET            (reg, 1);
    XGXSCTLr_CDET_ENf_SET         (reg, 1);
    XGXSCTLr_AFRST_ENf_SET        (reg, 1);
    XGXSCTLr_TXCKO_DIVf_SET       (reg, 1);
    XGXSCTLr_RESERVED_5f_SET      (reg, 1);

    MODIFY_XGXSCTLr(pa, reg);
    return PHYMOD_E_NONE;
}


/* 
 * viper_sgmii_force_speed 
 *
 * Set SGMII Mode   0x8300: 0x0100
 * Set 10M & Dis AN 0x0000: 0x0100
 * Set OS 5 Mode    0x834A: 0x0003
 *
 */
static int viper_sgmii_force_speed (PHYMOD_ST *pa, uint8_t speed)
{
    DIG_CTL1000X1r_t  x1reg;
    COMBO_MIICTLr_t   miictrl;
    DIG_MISC8r_t          misc8;

    /* Clear Registers */
    DIG_CTL1000X1r_CLR(x1reg);
    COMBO_MIICTLr_CLR(miictrl);
    DIG_MISC8r_CLR(misc8);

    /* Bit 0 is SGMII Mode */
    DIG_CTL1000X1r_COMMA_DET_ENf_SET(x1reg, 1);
    MODIFY_DIG_CTL1000X1r(pa, x1reg);

    /* Set 10M, Dis AN */
    COMBO_MIICTLr_AUTONEG_ENABLEf_SET(miictrl, 0);
    COMBO_MIICTLr_MANUAL_SPEED0f_SET (miictrl, speed&0x1);
    COMBO_MIICTLr_MANUAL_SPEED1f_SET (miictrl, speed&0x2?1:0);
    COMBO_MIICTLr_FULL_DUPLEXf_SET   (miictrl, 1);
    MODIFY_COMBO_MIICTLr(pa, miictrl);

    /* Set os5 mode */
    DIG_MISC8r_FORCE_OSCDR_MODEf_SET(misc8, VIPER_MISC8_OSDR_MODE_OSX5);
    MODIFY_DIG_MISC8r(pa, misc8);

    return PHYMOD_E_NONE;
}


/* 
 * viper_sgmii_force_10m 
 *
 * Set SGMII Mode   0x8300: 0x0100
 * Set 10M & Dis AN 0x0000: 0x0100
 * Set OS 5 Mode    0x834A: 0x0003
 *
 */
int viper_sgmii_force_10m (PHYMOD_ST *pa)
{
    return (viper_sgmii_force_speed (pa, VIPER_SGMII_SPEED_10M));
}


/* 
 * viper_sgmii_force_100m 
 *
 * Set SGMII Mode   0x8300: 0x0100
 * Set 10M & Dis AN 0x0000: 0x2100
 * Set OS 5 Mode    0x834A: 0x0003
 *
 */
int viper_sgmii_force_100m (PHYMOD_ST *pa)
{
    return (viper_sgmii_force_speed (pa, VIPER_SGMII_SPEED_100M));
}


/* 
 * viper_sgmii_force_1G 
 *
 * Set SGMII Mode   0x8300: 0x0100
 * Set 10M & Dis AN 0x0000: 0x0140
 * Set OS 5 Mode    0x834A: 0x0003
 *
 */
int viper_sgmii_force_1g (PHYMOD_ST *pa)
{
    return (viper_sgmii_force_speed (pa, VIPER_SGMII_SPEED_1000M));
}


/* 
 * viper_fiber_force_100FX 
 *
 * Set Fiber Mode   0x8300: 0x0005
 * Set OS 5 Mode    0x834A: 0x0003
 * Set FX Mode      0x8400: 0x014B
 * Dis Idle Correla 0x8402: 0x0880
 *
 */
int viper_fiber_force_100FX (PHYMOD_ST *pa)
{
    DIG_CTL1000X1r_t  x1reg;
    DIG_MISC8r_t      misc8;
    FX100_CTL1r_t     fxctrl;
    FX100_CTL3r_t     ctrl3;

    /* Clear Registers */
    DIG_CTL1000X1r_CLR(x1reg);
    FX100_CTL1r_CLR(fxctrl);
    FX100_CTL3r_CLR(ctrl3);
    DIG_MISC8r_CLR(misc8);

    /* Bit 0 is Fiber Mode */
    DIG_CTL1000X1r_FIBER_MODE_1000Xf_SET(x1reg, 1);
    DIG_CTL1000X1r_SIGNAL_DETECT_ENf_SET(x1reg, 1);
    MODIFY_DIG_CTL1000X1r(pa, x1reg);

    /* Set os5 mode */
    DIG_MISC8r_FORCE_OSCDR_MODEf_SET(misc8, VIPER_MISC8_OSDR_MODE_OSX5);
    MODIFY_DIG_MISC8r(pa, misc8);

    /* Set FX Mode, Disable Idle Correlator */ 
    FX100_CTL1r_RXDATA_SELf_SET(fxctrl, 5);
    FX100_CTL1r_FAR_END_FAULT_ENf_SET(fxctrl, 1);
    FX100_CTL1r_FULL_DUPLEXf_SET(fxctrl, 1);
    FX100_CTL1r_ENABLEf_SET(fxctrl, 1);
    MODIFY_FX100_CTL1r(pa, fxctrl);

    FX100_CTL3r_NUMBER_OF_IDLEf_SET(ctrl3, 8);
    FX100_CTL3r_CORRELATOR_DISABLEf_SET(ctrl3, 1);
    MODIFY_FX100_CTL3r(pa, ctrl3);

    return PHYMOD_E_NONE;
}

/* 
 * viper_fiber_force_1G 
 *
 * Set Fiber Mode   0x8300: 0x0105
 * Set 1G, Dis AN   0x0000: 0x0140
 * Set OS 5 Mode    0x834A: 0x0003
 *
 */
int viper_fiber_force_1G (PHYMOD_ST *pa)
{
    DIG_CTL1000X1r_t  x1reg;
    COMBO_MIICTLr_t        miictrl;
    DIG_MISC8r_t          misc8;

    /* Clear Registers */
    DIG_CTL1000X1r_CLR(x1reg);
    COMBO_MIICTLr_CLR(miictrl);
    DIG_MISC8r_CLR(misc8);

    /* Bit 0 is Fiber Mode */
    DIG_CTL1000X1r_FIBER_MODE_1000Xf_SET(x1reg, 1);
    DIG_CTL1000X1r_SIGNAL_DETECT_ENf_SET(x1reg, 1);
    DIG_CTL1000X1r_COMMA_DET_ENf_SET(x1reg, 1);
    MODIFY_DIG_CTL1000X1r(pa, x1reg);

    /* Set 1G, Dis AN */
    COMBO_MIICTLr_AUTONEG_ENABLEf_SET(miictrl, 0);
    COMBO_MIICTLr_MANUAL_SPEED1f_SET (miictrl, VIPER_SGMII_SPEED_1000M>>1);
    COMBO_MIICTLr_MANUAL_SPEED0f_SET (miictrl, VIPER_SGMII_SPEED_1000M&1);
    COMBO_MIICTLr_FULL_DUPLEXf_SET   (miictrl, 1);
    MODIFY_COMBO_MIICTLr(pa, miictrl);

    /* Set os5 mode */
    DIG_MISC8r_FORCE_OSCDR_MODEf_SET(misc8, VIPER_MISC8_OSDR_MODE_OSX5);
    MODIFY_DIG_MISC8r(pa, misc8);

    return PHYMOD_E_NONE;
}

/* 
 * viper_fiber_force_2p5G 
 *
 * Set Fiber Mode   0x8300: 0x0105
 * Set 2.5G         0x8308: 0xC010
 * Set OS 5 Mode    0x834A: 0x0001
 *
 */
int viper_fiber_force_2p5G (PHYMOD_ST *pa)
{
    DIG_CTL1000X1r_t  x1reg;
    DIG_MISC1r_t          misc1;
    DIG_MISC8r_t          misc8;

    /* Clear Registers */
    DIG_CTL1000X1r_CLR(x1reg);
    DIG_MISC1r_CLR(misc1);
    DIG_MISC8r_CLR(misc8);

    /* Bit 0 is Fiber Mode */
    DIG_CTL1000X1r_FIBER_MODE_1000Xf_SET(x1reg, 1);
    DIG_CTL1000X1r_SIGNAL_DETECT_ENf_SET(x1reg, 1);
    DIG_CTL1000X1r_COMMA_DET_ENf_SET(x1reg, 1);
    MODIFY_DIG_CTL1000X1r(pa, x1reg);

    DIG_MISC1r_REFCLK_SELf_SET (misc1, VIPER_MISC1_CLK_50M); /* clk_50Mhz */
    DIG_MISC1r_FORCE_SPEEDf_SET(misc1, VIPER_MISC1_10G_X2);  /* dr_10G_X2 */ 

    MODIFY_DIG_MISC1r(pa, misc1);

    /* Set os5 mode */
    DIG_MISC8r_FORCE_OSCDR_MODEf_SET(misc8, VIPER_MISC8_OSDR_MODE_OSX2);
    MODIFY_DIG_MISC8r(pa, misc8);

    return PHYMOD_E_NONE;
}


/* 
 * viper_fiber_force_speed 
 *
 * Set Lane Broadcast 0xFFD0: 0x001F
 * Set Fiber Mode     0x8300: 0x0105
 * Set SPEED          0x8308: 0x601X
 * Set OS 2 Mode      0x834A: 0x0001
 *
 */
static int viper_fiber_force_speed (PHYMOD_ST *pa, uint8_t speed)
{
    DIG_CTL1000X1r_t  x1reg;
    DIG_MISC1r_t          misc1;
    DIG_MISC8r_t          misc8;

    /* Clear Registers */
    DIG_CTL1000X1r_CLR(x1reg);
    DIG_MISC1r_CLR(misc1);
    DIG_MISC8r_CLR(misc8);

    /* rv = pc->write(unit, phy_addr, 0x1f, 0xffd0); */
    /* Set Broadcast */
#ifdef _FIX_
    rv = tscmod_cl22_write(pa, 0x1f, 0xffd0);
#endif
    /* Bit 0 is Fiber Mode */
    DIG_CTL1000X1r_FIBER_MODE_1000Xf_SET(x1reg, 1);
    DIG_CTL1000X1r_SIGNAL_DETECT_ENf_SET(x1reg, 1);
    DIG_CTL1000X1r_COMMA_DET_ENf_SET(x1reg, 1);
    MODIFY_DIG_CTL1000X1r(pa, x1reg);

    /* Set Speed */
    DIG_MISC1r_REFCLK_SELf_SET(misc1, VIPER_MISC1_CLK_156p25M); /* clk_156.25Mhz */
    DIG_MISC1r_FORCE_SPEEDf_SET(misc1, speed);
    MODIFY_DIG_MISC1r(pa, misc1);

    /* Set os2 mode */
    DIG_MISC8r_FORCE_OSCDR_MODEf_SET(misc8, VIPER_MISC8_OSDR_MODE_OSX2);
    MODIFY_DIG_MISC8r(pa, misc8);

    return PHYMOD_E_NONE;
}

/* 
 * viper_fiber_force_10G 
 *
 * Set Lane Broadcast 0xFFD0: 0x001F
 * Set Fiber Mode     0x8300: 0x0105
 * Set 10G            0x8318: 0x6013
 * Set OS 2 Mode      0x834A: 0x0001
 *
 */
int viper_fiber_force_10G (PHYMOD_ST *pa)
{
    return (viper_fiber_force_speed (pa, VIPER_MISC1_10GHiGig_X4));
}

/* 
 * viper_fiber_force_10G_CX4 
 *
 * Set Lane Broadcast 0xFFD0: 0x001F
 * Set Fiber Mode     0x8300: 0x0105
 * Set 10G            0x8318: 0x6014
 * Set OS 2 Mode      0x834A: 0x0001
 *
 */
int viper_fiber_force_10G_CX4 (PHYMOD_ST *pa)
{
    return (viper_fiber_force_speed (pa, VIPER_MISC1_10GBASE_CX4));
}

/* 
 * viper_fiber_AN_1G 
 *
 * Set Fiber Mode     0x8300: 0x0145
 * Set OS 5 Mode      0x834A: 0x0003
 * Set 10G            0x8000: 0x1140
 *
 */
int viper_fiber_AN_1G (PHYMOD_ST *pa)
{
    DIG_CTL1000X1r_t  x1reg;
    DIG_MISC8r_t          misc8;
    COMBO_MIICTLr_t        miictrl;

    /* Clear Registers */
    DIG_CTL1000X1r_CLR(x1reg);
    DIG_MISC8r_CLR(misc8);
    COMBO_MIICTLr_CLR(miictrl);

    /* Fiber Mode, sig det, dis pll pwrdn, comma det */
    DIG_CTL1000X1r_FIBER_MODE_1000Xf_SET(x1reg, 1);
    DIG_CTL1000X1r_SIGNAL_DETECT_ENf_SET(x1reg, 1);
    DIG_CTL1000X1r_DISABLE_PLL_PWRDWNf_SET(x1reg, 1);
    DIG_CTL1000X1r_COMMA_DET_ENf_SET(x1reg, 1);
    MODIFY_DIG_CTL1000X1r(pa, x1reg);

    /* Set os5 mode */
    DIG_MISC8r_FORCE_OSCDR_MODEf_SET(misc8, VIPER_MISC8_OSDR_MODE_OSX5);
    MODIFY_DIG_MISC8r(pa, misc8);

    /* AN, FD, 1G */
    COMBO_MIICTLr_AUTONEG_ENABLEf_SET(miictrl, 1);
    COMBO_MIICTLr_MANUAL_SPEED0f_SET (miictrl, VIPER_SGMII_SPEED_1000M&1);
    COMBO_MIICTLr_FULL_DUPLEXf_SET   (miictrl, 1);
    COMBO_MIICTLr_MANUAL_SPEED1f_SET (miictrl, VIPER_SGMII_SPEED_1000M>>1);
    MODIFY_COMBO_MIICTLr(pa, miictrl);

    return PHYMOD_E_NONE;
}


/* 
 * viper_sgmii_aneg_speed 
 *
 * Set Fiber Mode     0x8300: 0x0120
 * Set OS 5 Mode      0x834A: 0x0003
 * Set SPEED          0x0000: 0xXXXX
 *
 */
static int viper_sgmii_aneg_speed (PHYMOD_ST *pa, uint8_t master, uint8_t speed)
{
    DIG_CTL1000X1r_t  x1reg;
    DIG_MISC8r_t      misc8;
    COMBO_MIICTLr_t   miictrl;

    /* Clear Registers */
    DIG_CTL1000X1r_CLR(x1reg);
    DIG_MISC8r_CLR(misc8);
    COMBO_MIICTLr_CLR(miictrl);

    /* SGMII Master Mode, comma det */
    DIG_CTL1000X1r_SGMII_MASTER_MODEf_SET(x1reg, master);
    DIG_CTL1000X1r_FIBER_MODE_1000Xf_SET(x1reg, 0);
    DIG_CTL1000X1r_COMMA_DET_ENf_SET(x1reg, 1);

    MODIFY_DIG_CTL1000X1r(pa, x1reg);

    /* Set os5 mode */
    DIG_MISC8r_FORCE_OSCDR_MODEf_SET(misc8, VIPER_MISC8_OSDR_MODE_OSX5);
    MODIFY_DIG_MISC8r(pa, misc8);

    /* AN, FD, 100M */
    COMBO_MIICTLr_AUTONEG_ENABLEf_SET(miictrl, 1);
    COMBO_MIICTLr_MANUAL_SPEED0f_SET (miictrl, speed&1);
    COMBO_MIICTLr_FULL_DUPLEXf_SET   (miictrl, 1);
    COMBO_MIICTLr_MANUAL_SPEED1f_SET (miictrl, speed>>1);
    MODIFY_COMBO_MIICTLr(pa, miictrl);

    return PHYMOD_E_NONE;
}

/* 
 * viper_sgmii_master_aneg_100M 
 *
 * Set Fiber Mode     0x8300: 0x0120
 * Set OS 5 Mode      0x834A: 0x0003
 * Set SPEED          0x0000: 0x3100
 *
 */
int viper_sgmii_master_aneg_100M (PHYMOD_ST *pa, uint8_t speed)
{
    return(viper_sgmii_aneg_speed (pa, 1, VIPER_SGMII_SPEED_100M));
}

/* 
 * viper_sgmii_master_aneg_10M 
 *
 * Set Fiber Mode     0x8300: 0x0120
 * Set OS 5 Mode      0x834A: 0x0003
 * Set SPEED          0x0000: 0x1100
 *
 */
int viper_sgmii_aneg_10M (PHYMOD_ST *pa)
{
    return(viper_sgmii_aneg_speed (pa, 1, VIPER_SGMII_SPEED_10M));
}

/* 
 * viper_sgmii_slave_aneg_speed 
 *
 * Set Fiber Mode     0x8300: 0x0100
 * Set OS 5 Mode      0x834A: 0x0003
 * Set SPEED          0x0000: 0x1140
 *
 */
int viper_sgmii_slave_aneg_speed (PHYMOD_ST *pa)
{
    return(viper_sgmii_aneg_speed (pa, 0, VIPER_SGMII_SPEED_1000M));
}

/* 
 * viper_lane_reset 
 *
 * Reset lane 0x810A: 0x00FF
 *
 */
int viper_lane_reset (PHYMOD_ST *pa)
{
    LANERESETr_t reg;

    LANERESETr_CLR(reg); 
    LANERESETr_RESET_TXf_SET(reg, 0xF);
    LANERESETr_RESET_RXf_SET(reg, 0xF);
    MODIFY_LANERESETr(pa, reg);

    return PHYMOD_E_NONE;
}

/* 
 * viper_lpi_disable 
 *
 * Disable LPI CL48  0x8150: 0x0000
 * Disable LPI CL36  0x833E: 0x0000
 *
 */
int viper_lpi_disable (PHYMOD_ST *pa)
{
    EEECTLr_t reg;
    DIG_MISC5r_t      misc5;

    /* Clear registers */
    EEECTLr_CLR(reg); 
    DIG_MISC5r_CLR(misc5);

    EEECTLr_LPI_EN_RXf_SET(reg, 0);
    EEECTLr_LPI_EN_TXf_SET(reg, 0);
    MODIFY_EEECTLr(pa, reg);

    DIG_MISC5r_LPI_EN_RXf_SET(misc5, 0);
    DIG_MISC5r_LPI_EN_TXf_SET(misc5, 0);
    MODIFY_DIG_MISC5r(pa, misc5);

    return PHYMOD_E_NONE;
}

/* 
 * viper_global_loopback_ena 
 *
 * Enable MDIO 0x0000: 0x0C3F
 * Enable MDIO 0x8017: 0xFF0F
 * Enable MDIO 0x0000: 0x2C3F
 *
 */
int viper_global_loopback_set (const PHYMOD_ST *pa, uint8_t enable)
{
    XGXSCTLr_t    reg;
    LANECTL2r_t   ctrl;
    uint16_t      gloop, cdet, eden;
    XGXSCTLr_CLR(reg);
    READ_LANECTL2r(pa, &ctrl);

    /* Enable MDIO */ 
    XGXSCTLr_MODEf_SET        (reg, VIPER_XGXS_MODE_COMBO_CORE);
    XGXSCTLr_EDENf_SET        (reg, 1);
    XGXSCTLr_CDET_ENf_SET     (reg, 1);
    XGXSCTLr_AFRST_ENf_SET    (reg, 1);
    XGXSCTLr_TXCKO_DIVf_SET   (reg, 1);
    XGXSCTLr_RESERVED_5f_SET  (reg, 1);
    XGXSCTLr_MDIO_CONT_ENf_SET(reg, 1);
    MODIFY_XGXSCTLr(pa, reg);

    gloop = LANECTL2r_GLOOP1Gf_GET(ctrl);
    cdet  = LANECTL2r_CDET_EN1Gf_GET(ctrl);
    eden  = LANECTL2r_EDEN1Gf_GET(ctrl);

    if (enable) {
        gloop |= 1<<pa->lane;
        cdet  |= 1<<pa->lane;
        eden  |= 1<<pa->lane;
    } else {
        gloop &= ~(1<<pa->lane);
        cdet  &= ~(1<<pa->lane);
        eden  &= ~(1<<pa->lane);
    }

    LANECTL2r_GLOOP1Gf_SET(ctrl, gloop);
    LANECTL2r_CDET_EN1Gf_SET(ctrl, cdet);
    LANECTL2r_EDEN1Gf_SET(ctrl, eden);

    MODIFY_LANECTL2r(pa, ctrl);

    XGXSCTLr_START_SEQUENCERf_SET(reg, 1);
    MODIFY_XGXSCTLr(pa, reg);

    return PHYMOD_E_NONE;
}

/* 
 * viper_global_loopback_get 
 *
 */
int viper_global_loopback_get (const PHYMOD_ST *pa, uint32_t *lpbk) 
{
    LANECTL2r_t   ctrl;

    LANECTL2r_CLR (ctrl);
    READ_LANECTL2r(pa, &ctrl);

    *lpbk = LANECTL2r_GLOOP1Gf_GET(ctrl);

    return PHYMOD_E_NONE;
}

/* 
 * viper_remote_loopback_ena 
 *
 * Set remote loop 0x8300: 0x040X
 *
 */
int viper_remote_loopback_ena (PHYMOD_ST *pa)
{
    DIG_CTL1000X1r_t  x1reg;

    /* Clear Registers */
    READ_DIG_CTL1000X1r(pa, &x1reg);

    DIG_CTL1000X1r_DISABLE_SIGNAL_DETECT_FILTERf_SET(x1reg, 0);

    DIG_CTL1000X1r_SEL_RX_PKTS_FOR_CNTRf_SET(x1reg, 0);
    DIG_CTL1000X1r_REMOTE_LOOPBACKf_SET(x1reg, 1);
    DIG_CTL1000X1r_ZERO_COMMA_DETECTOR_PHASEf_SET(x1reg, 0);
    DIG_CTL1000X1r_COMMA_DET_ENf_SET(x1reg, 0);

    DIG_CTL1000X1r_CRC_CHECKER_DISABLEf_SET(x1reg, 0);
    DIG_CTL1000X1r_DISABLE_PLL_PWRDWNf_SET(x1reg, 0);
    DIG_CTL1000X1r_SGMII_MASTER_MODEf_SET(x1reg, 0);

    MODIFY_DIG_CTL1000X1r(pa, x1reg);

    return PHYMOD_E_NONE;
}


/* 
 * viper_tx_lane_swap 
 *
 * Lane Swap 0x8169: Value
 *
 */

int viper_tx_lane_swap (PHYMOD_ST *pa, 
                        uint8_t   tx_ln_0, 
                        uint8_t   tx_ln_1, 
                        uint8_t   tx_ln_2, 
                        uint8_t   tx_ln_3)
{
    TXLNSWP1r_t  ln_swap;

    /* Clear Registers */
    TXLNSWP1r_CLR(ln_swap);

    TXLNSWP1r_TX0_LNSWAP_SELf_SET(ln_swap, tx_ln_0);
    TXLNSWP1r_TX1_LNSWAP_SELf_SET(ln_swap, tx_ln_1);
    TXLNSWP1r_TX2_LNSWAP_SELf_SET(ln_swap, tx_ln_2);
    TXLNSWP1r_TX3_LNSWAP_SELf_SET(ln_swap, tx_ln_3);
/*
#define TXLN_SWAP(ln_swap, tx_ln, num) TXLN_SWP1r_TXnum##_LNSWAP_SELf_SET(ln_swap, tx_ln)
    TXLN_SWAP(ln_swap, tx_ln_0, 0);
    TXLN_SWAP(ln_swap, tx_ln_1, 1);
    TXLN_SWAP(ln_swap, tx_ln_2, 2);
    TXLN_SWAP(ln_swap, tx_ln_3, 3);
*/

    MODIFY_TXLNSWP1r(pa, ln_swap);
    return PHYMOD_E_NONE;
}

/* 
 * viper_rx_lane_swap 
 *
 * Lane Swap 0x816b: Value
 *
 */
int viper_rx_lane_swap (PHYMOD_ST *pa, 
                        uint8_t   rx_ln_0, 
                        uint8_t   rx_ln_1, 
                        uint8_t   rx_ln_2, 
                        uint8_t   rx_ln_3)
{
    RXLNSWP1r_t  ln_swap;

    /* Clear Registers */
    RXLNSWP1r_CLR(ln_swap);

    RXLNSWP1r_RX0_LNSWAP_SELf_SET(ln_swap, rx_ln_0);
    RXLNSWP1r_RX1_LNSWAP_SELf_SET(ln_swap, rx_ln_1);
    RXLNSWP1r_RX2_LNSWAP_SELf_SET(ln_swap, rx_ln_2);
    RXLNSWP1r_RX3_LNSWAP_SELf_SET(ln_swap, rx_ln_3);

    MODIFY_RXLNSWP1r(pa, ln_swap);
    return PHYMOD_E_NONE;
}


/* 
 * viper_tx_pol_set 
 *
 * Polarity Flip 0x8061: 0x0020
 *
 */
int viper_tx_pol_set (const PHYMOD_ST *pa, uint8_t val)
{
    TX_AFE_ANATXACTL0r_t  reg;

    READ_TX_AFE_ANATXACTL0r(pa, &reg);
    TX_AFE_ANATXACTL0r_TXPOL_FLIPf_SET(reg, val);
    MODIFY_TX_AFE_ANATXACTL0r(pa, reg);

    return PHYMOD_E_NONE;
}


/* 
 * viper_rx_pol_set 
 *
 * Polarity Flip 0x80b1: 0x0020
 *
 */
int viper_rx_pol_set (const PHYMOD_ST *pa, uint8_t val)
{
    RX_AFE_ANARXCTLPCIr_t  reg;

    READ_RX_AFE_ANARXCTLPCIr(pa, &reg);

    RX_AFE_ANARXCTLPCIr_LINK_EN_Rf_SET(reg, val);
    RX_AFE_ANARXCTLPCIr_RX_POLARITY_FORCE_SMf_SET(reg, val);
    RX_AFE_ANARXCTLPCIr_RX_POLARITY_Rf_SET(reg, val);

    MODIFY_RX_AFE_ANARXCTLPCIr(pa, reg);

    return PHYMOD_E_NONE;
}

/*
 * viper_pol_get
 */
int viper_tx_pol_get (const PHYMOD_ST *pa, uint32_t *val)
{
    TX_AFE_ANATXACTL0r_t  reg;

    READ_TX_AFE_ANATXACTL0r(pa, &reg);
    *val = TX_AFE_ANATXACTL0r_TXPOL_FLIPf_GET(reg);

    return PHYMOD_E_NONE;
}


/*
 * viper_rx_pol_get 
 */
int viper_rx_pol_get (const PHYMOD_ST *pa, uint32_t *val)
{
    RX_AFE_ANARXCTLPCIr_t  reg;

    READ_RX_AFE_ANARXCTLPCIr(pa, &reg);
    *val = RX_AFE_ANARXCTLPCIr_RX_POLARITY_Rf_GET(reg);

    return PHYMOD_E_NONE;
}


/* 
 * viper_pll_lock_speed_up 
 *
 * Calib Charge Time 0x8183 : 0x002A 
 * Calib Delay  Time 0x8184 : 0x021C 
 * Calib Step   Time 0x8185 : 0x0055 
 *
 */
int viper_pll_lock_speed_up (PHYMOD_ST *pa)
{
    PLL2_CTL3r_t   ctrl3;
    PLL2_CTL4r_t   ctrl4;
    PLL2_CTL5r_t   ctrl5;

    PLL2_CTL3r_CALIB_CAP_CHARGE_TIMEf_SET (ctrl3, 0x002A); 
    PLL2_CTL4r_CALIB_DELAY_TIMEf_SET      (ctrl4, 0x021C); 
    PLL2_CTL5r_CALIB_STEP_TIMEf_SET       (ctrl5, 0x0055); 

    MODIFY_PLL2_CTL3r(pa, ctrl3);
    MODIFY_PLL2_CTL4r(pa, ctrl4);
    MODIFY_PLL2_CTL5r(pa, ctrl5);

    return PHYMOD_E_NONE;
}


/* 
 * viper_an_speed_up 
 *
 * enable fast timers 0x8301 : 0x0040 
 *
 */
int viper_an_speed_up (PHYMOD_ST *pa)
{
    DIG_CTL1000X2r_t   ctrl;

    READ_DIG_CTL1000X2r (pa, &ctrl);
    DIG_CTL1000X2r_AUTONEG_FAST_TIMERSf_SET(ctrl, 1);
    MODIFY_DIG_CTL1000X2r (pa, ctrl);

    return PHYMOD_E_NONE;
}


/* 
 * viper_forced_speed_up 
 *
 * clock speed up   0x8309 : 0x2790 
 * fx100 fast timer 0x8402 : 0x0801 
 *
 */
int viper_forced_speed_up (PHYMOD_ST *pa)
{
    DIG_MISC2r_t      misc2;
    FX100_CTL3r_t     ctrl3;

    DIG_MISC2r_CLR(misc2);
    DIG_MISC2r_RESERVED_14_13f_SET(misc2, 1);
    DIG_MISC2r_CLKSIGDET_BYPASSf_SET(misc2, 1);
    DIG_MISC2r_CLK41_BYPASSf_SET(misc2, 1);
    DIG_MISC2r_MIIGMIIDLY_ENf_SET(misc2, 1);
    DIG_MISC2r_MIIGMIIMUX_ENf_SET(misc2, 1);
    DIG_MISC2r_FIFO_ERR_CYAf_SET(misc2, 1);
    MODIFY_DIG_MISC2r(pa, misc2);

    FX100_CTL3r_CLR(ctrl3);
    FX100_CTL3r_NUMBER_OF_IDLEf_SET(ctrl3, 8);
    FX100_CTL3r_FAST_TIMERSf_SET(ctrl3, 1);
    MODIFY_FX100_CTL3r(pa, ctrl3);

    return PHYMOD_E_NONE;
}

#if 0
/* 
 * viper_prbs 
 *
 * Disable PLL       0x8000 : 0x052F 
 * Disable CL36      0x8015 : 0x0000 
 * Set 1G Mode       0x8016 : 0x0000 
 * Disable cden/eden 0x8017 : 0x0000 
 * set prbs order    0x8019 : 0x3333 
 * enable prbs       0x8019 : 0xBBBB 
 * choose tx datai   0x8150 : 0x00F0 
 * Broadcast         0xFFDE : 0x001F 
 * OS2               0x834A : 0x0001 
 * Eable PLL SEQ     0x8340 : 0x252F 
 *
 */
int viper_prbs (PHYMOD_ST *pa, uint8_t prbs_mode)
{
    XGXSCTLr_t   xgxs_ctrl;
    LANECTL0r_t  lane_ctrl0;
    LANECTL1r_t  lane_ctrl1;
    LANECTL2r_t  lane_ctrl2;
    LANEPRBSr_t  lane_prbs;

    return PHYMOD_E_NONE;
}
#endif

/**
@brief   Read Link status
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   *link Reference for Status of PCS Link
@returns The value PHYMOD_E_NONE upon successful completion
@details Return the status of the PCS link. The link up implies the PCS is able
to decode the digital bits coming in on the serdes. It automatically implies
that the PLL is stable and that the PMD sublayer has successfully recovered the
clock on the receive line.
*/
int viper_get_link_status(const PHYMOD_ST* pc, uint32_t *link)
{
    STS1000X1r_t sts;
    STS1000X1r_CLR(sts);

    PHYMOD_IF_ERR_RETURN(READ_STS1000X1r(pc, &sts));
    *link = STS1000X1r_LINK_STATUSf_GET(sts);
    /* FIX THE DISPLAY */
    /* VIPER_DBG_IN_FUNC_VOUT_INFO(pc,("serdes link: %d", *link)); */

    return PHYMOD_E_NONE;
}

/**
@brief   This function reads TX-PMD PMD_LOCK bit.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   lockStatus reference which is updated with pmd_lock status
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Read PMD lock status Returns 1 or 0 (locked/not)
*/

int viper_pmd_lock_get(const PHYMOD_ST* pc, uint32_t* lockStatus)
{
    /* Place Holder */
    return PHYMOD_E_NONE;
}

int viper_autoneg_status_get(const PHYMOD_ST *pc, phymod_autoneg_status_t *status)
{

    /* Place Holder */
    /*
     * status->enabled   = 
     * status->locked    = 
     * status->data_rate = 
     * status->interface = phymodInterfaceXFI;
     */ 
    return PHYMOD_E_NONE;
}

int viper_autoneg_get(const PHYMOD_ST *pc, phymod_autoneg_control_t  *an, 
                      uint32_t                  *an_done)
{
    COMBO_MIICTLr_t   miictrl;
    COMBO_MIISTATr_t  miistat;

    READ_COMBO_MIICTLr (pc, &miictrl);
    READ_COMBO_MIISTATr(pc, &miistat);

    /*
     * an->flags        = 
     * an->an_mode      =
     * an->num_lane_adv = 
     */

    an->enable = COMBO_MIICTLr_AUTONEG_ENABLEf_GET(miictrl);
    *an_done   = COMBO_MIISTATr_AUTONEG_COMPLETEf_GET(miistat);

    return PHYMOD_E_NONE;
}

static int _viper_getRevDetails(const PHYMOD_ST* pc)
{
    SERDESID0r_t reg_serdesid;

    SERDESID0r_CLR(reg_serdesid);
    PHYMOD_IF_ERR_RETURN(READ_SERDESID0r(pc,&reg_serdesid));
    return (SERDESID0r_GET(reg_serdesid));
}

/**
@brief   Read the 16 bit rev. id value etc.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   *revid int ref. to return revision id to calling function.
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This  fucntion reads the revid register that contains core number, revision
number and returns the 16-bit value in the revid
*/
int viper_revid_read(const PHYMOD_ST* pc, uint32_t *revid)              /* REVID_READ */
{
    *revid=_viper_getRevDetails(pc);
    /* VIPER_DBG_IN_FUNC_VOUT_INFO(pc,("revid: %x", *revid)); */
    return PHYMOD_E_NONE;
}

int viper_tsc_tx_pi_freq_override (const PHYMOD_ST *pa, 
                                   uint8_t                enable, 
                                   int16_t                freq_override_val) 
{
#if 0
/* HEADER FILE SHOULD BE FIXED 806A - 806E */
   if (enable) {
     wrc_tx_pi_en(0x1);                                 /* TX_PI enable :            0 = disabled, 1 = enabled */
     wrc_tx_pi_freq_override_en(0x1);                   /* Fixed freq mode enable:   0 = disabled, 1 = enabled */
     wrc_tx_pi_freq_override_val(freq_override_val);    /* Fixed Freq Override Value (+/-8192) */
   }
   else {
     wrc_tx_pi_freq_override_val(0);                    /* Fixed Freq Override Value to 0 */
     wrc_tx_pi_freq_override_en(0x0);                   /* Fixed freq mode enable:   0 = disabled, 1 = enabled */
     wrc_tx_pi_en(0x0);                                 /* TX_PI enable :            0 = disabled, 1 = enabled */
   }
#endif
  return (PHYMOD_E_NONE);
}

