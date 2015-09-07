/*
 *         
 * $Id: eagle.c,v 1.2.2.26 Broadcom SDK $
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
 *  $Id: c89e03334066f68dbc4892964e8f32ad981d1f1c $
*/


#include <phymod/phymod.h>
#include "eagle_cfg_seq.h" 
#include <phymod/chip/bcmi_tsce_xgxs_defs.h>
#include "eagle_tsc_fields.h"
#include "eagle_tsc_dependencies.h"
#include "eagle_tsc_field_access.h"


int eagle_uc_active_set(const phymod_access_t *pa, uint32_t enable)           /* set microcontroller active or not  */
{
    DIG_TOP_USER_CTL0r_t uc_active_reg;

    DIG_TOP_USER_CTL0r_CLR(uc_active_reg);
    DIG_TOP_USER_CTL0r_UC_ACTIVEf_SET(uc_active_reg, enable);
    MODIFY_DIG_TOP_USER_CTL0r(pa, uc_active_reg);
      
    return PHYMOD_E_NONE;
}

int eagle_uc_reset(const phymod_access_t *pa, uint32_t enable)           /* set dw8501 reset  */
{
    UC_COMMANDr_t command_reg;

    UC_COMMANDr_CLR(command_reg);
    UC_COMMANDr_MICRO_MDIO_DW8051_RESET_Nf_SET(command_reg, enable);
    MODIFY_UC_COMMANDr(pa, command_reg);
 
    return PHYMOD_E_NONE;
}

int eagle_prbs_tx_inv_data_get(const phymod_access_t *pa, uint32_t *inv_data)
{
    TLB_TX_PRBS_GEN_CFGr_t tmp_reg;
    TLB_TX_PRBS_GEN_CFGr_CLR(tmp_reg);
    READ_TLB_TX_PRBS_GEN_CFGr(pa, &tmp_reg);
    *inv_data = TLB_TX_PRBS_GEN_CFGr_PRBS_GEN_INVf_GET(tmp_reg);

    return PHYMOD_E_NONE;
}

int eagle_prbs_rx_inv_data_get(const phymod_access_t *pa, uint32_t *inv_data)
{
    TLB_RX_PRBS_CHK_CFGr_t tmp_reg;
    TLB_RX_PRBS_CHK_CFGr_CLR(tmp_reg);
    READ_TLB_RX_PRBS_CHK_CFGr(pa, &tmp_reg);
    *inv_data = TLB_RX_PRBS_CHK_CFGr_PRBS_CHK_INVf_GET(tmp_reg);
    return PHYMOD_E_NONE;
}


int eagle_prbs_tx_poly_get(const phymod_access_t *pa, eagle_prbs_polynomial_type_t *prbs_poly)
{
    TLB_TX_PRBS_GEN_CFGr_t tmp_reg;
    TLB_TX_PRBS_GEN_CFGr_CLR(tmp_reg);
    READ_TLB_TX_PRBS_GEN_CFGr(pa, &tmp_reg);
    *prbs_poly = TLB_TX_PRBS_GEN_CFGr_PRBS_GEN_MODE_SELf_GET(tmp_reg);

    return PHYMOD_E_NONE;
}

int eagle_prbs_rx_poly_get(const phymod_access_t *pa, eagle_prbs_polynomial_type_t *prbs_poly)
{
    TLB_RX_PRBS_CHK_CFGr_t tmp_reg;
    TLB_RX_PRBS_CHK_CFGr_CLR(tmp_reg);
    READ_TLB_RX_PRBS_CHK_CFGr(pa, &tmp_reg);
    *prbs_poly = TLB_RX_PRBS_CHK_CFGr_PRBS_CHK_MODE_SELf_GET(tmp_reg);
    return PHYMOD_E_NONE;
}

int eagle_prbs_tx_enable_get(const phymod_access_t *pa, uint32_t *enable)
{
    TLB_TX_PRBS_GEN_CFGr_t tmp_reg;
    TLB_TX_PRBS_GEN_CFGr_CLR(tmp_reg);
    READ_TLB_TX_PRBS_GEN_CFGr(pa, &tmp_reg);
    *enable = TLB_TX_PRBS_GEN_CFGr_PRBS_GEN_ENf_GET(tmp_reg);

    return PHYMOD_E_NONE;
}

int eagle_prbs_rx_enable_get(const phymod_access_t *pa, uint32_t *enable)
{
    TLB_RX_PRBS_CHK_CFGr_t tmp_reg;
    TLB_RX_PRBS_CHK_CFGr_CLR(tmp_reg);
    READ_TLB_RX_PRBS_CHK_CFGr(pa, &tmp_reg);
    *enable = TLB_RX_PRBS_CHK_CFGr_PRBS_CHK_ENf_GET(tmp_reg);
    return PHYMOD_E_NONE;
}

int eagle_pll_mode_set(const phymod_access_t *pa, int pll_mode)   /* PLL divider set */
{
    PLL_CAL_CTL7r_t eagle_xgxs_ctl_7_reg;

    PLL_CAL_CTL7r_CLR(eagle_xgxs_ctl_7_reg);
    PLL_CAL_CTL7r_PLL_MODEf_SET(eagle_xgxs_ctl_7_reg, pll_mode);
    MODIFY_PLL_CAL_CTL7r(pa, eagle_xgxs_ctl_7_reg); 
    return PHYMOD_E_NONE;
}


int eagle_core_soft_reset_release(const phymod_access_t *pa, uint32_t enable)   /* release the pmd core soft reset */
{
    DIG_TOP_USER_CTL0r_t reg;

    DIG_TOP_USER_CTL0r_CLR(reg);
    DIG_TOP_USER_CTL0r_CORE_DP_S_RSTBf_SET(reg, enable);
    MODIFY_DIG_TOP_USER_CTL0r(pa, reg);
    return PHYMOD_E_NONE;
}

int eagle_lane_soft_reset_release(const phymod_access_t *pa, uint32_t enable)   /* release the pmd core soft reset */
{
    CKRST_LN_CLK_RST_N_PWRDWN_CTLr_t    reg_pwrdwn_ctrl;

    CKRST_LN_CLK_RST_N_PWRDWN_CTLr_CLR(reg_pwrdwn_ctrl);
    CKRST_LN_CLK_RST_N_PWRDWN_CTLr_LN_DP_S_RSTBf_SET(reg_pwrdwn_ctrl, enable);
    MODIFY_CKRST_LN_CLK_RST_N_PWRDWN_CTLr(pa,  reg_pwrdwn_ctrl);

    return PHYMOD_E_NONE;
}

int eagle_pram_flop_set(const phymod_access_t *pa, int value)   /* release the pmd core soft reset */
{
    UC_COMMAND3r_t uc_reg;
    UC_COMMAND3r_CLR(uc_reg);
    UC_COMMAND3r_MICRO_PRAM_IF_FLOP_BYPASSf_SET(uc_reg, value);
    MODIFY_UC_COMMAND3r(pa, uc_reg);
    return PHYMOD_E_NONE;
}


int eagle_pram_firmware_enable(const phymod_access_t *pa, int enable)   /* release the pmd core soft reset */
{
    UC_COMMAND3r_t uc_reg;
    /* UC_COMMAND4r_t reg; */
    UC_COMMAND3r_CLR(uc_reg);
    /* UC_COMMAND4r_CLR(reg); */
    if(enable == 1) {
    UC_COMMAND3r_MICRO_PRAM_IF_RSTBf_SET(uc_reg, 1);
    UC_COMMAND3r_MICRO_PRAM_IF_ENf_SET(uc_reg, 1);
    UC_COMMAND3r_MICRO_PRAM_IF_FLOP_BYPASSf_SET(uc_reg, 0);
    /*
    UC_COMMAND4r_MICRO_SYSTEM_CLK_ENf_SET(reg, 1);
    UC_COMMAND4r_MICRO_SYSTEM_RESET_Nf_SET(reg, 1);
    MODIFY_UC_COMMAND4r(pa, reg);
    */
    } else {
    UC_COMMAND3r_MICRO_PRAM_IF_RSTBf_SET(uc_reg, 0);
    UC_COMMAND3r_MICRO_PRAM_IF_ENf_SET(uc_reg, 0);
    UC_COMMAND3r_MICRO_PRAM_IF_FLOP_BYPASSf_SET(uc_reg, 1);
    }
    MODIFY_UC_COMMAND3r(pa, uc_reg);
    return PHYMOD_E_NONE;
}

int eagle_pmd_force_signal_detect(const phymod_access_t *pa, int enable)   
{
    SIGDET_CTL1r_t reg;
    SIGDET_CTL1r_CLR(reg);
    SIGDET_CTL1r_SIGNAL_DETECT_FRCf_SET(reg, enable);
    SIGDET_CTL1r_SIGNAL_DETECT_FRC_VALf_SET(reg, enable);
    MODIFY_SIGDET_CTL1r(pa, reg);
    return PHYMOD_E_NONE;
}

int eagle_pmd_loopback_get(const phymod_access_t *pa, uint32_t *enable)   
{
    TLB_RX_DIG_LPBK_CFGr_t reg;
    READ_TLB_RX_DIG_LPBK_CFGr(pa, &reg);
    *enable = TLB_RX_DIG_LPBK_CFGr_DIG_LPBK_ENf_GET(reg);
    return PHYMOD_E_NONE;
}


int eagle_uc_init(const phymod_access_t *pa)
{
  uint16_t __err = 0;
  int result;

  wrc_micro_system_clk_en(0x1);                         /* Enable clock to micro  */
  wrc_micro_system_reset_n(0x1);                        /* De-assert reset to micro */
  wrc_micro_system_reset_n(0x0);                        /* Assert reset to micro - Toggling micro reset*/
  wrc_micro_system_reset_n(0x1);                        /* De-assert reset to micro */

  eagle_pram_flop_set(pa, 0x0);
  wrc_micro_mdio_ram_access_mode(0x0);                  /* Select Program Memory access mode - Program RAM load when Serdes DW8051 in reset */

  wrc_micro_ram_address(0x0);                           /* RAM start address */
  wrc_micro_init_cmd(0x0);                              /* Clear initialization command */
  wrc_micro_init_cmd(0x1);                              /* Set initialization command */

  eagle_tsc_delay_us(4000);

  result = !rdc_micro_init_done();
	if (result) {                                        /* Check if initialization done within 500us time interval */
		PHYMOD_DEBUG_ERROR(("ERR_CODE_MICRO_INIT_NOT_DONE\n"));
		return (ERR_CODE_MICRO_INIT_NOT_DONE);    /* Else issue error code */
	}
  wrc_micro_init_cmd(0x0);                              /* Clear initialization command */
  return ( PHYMOD_E_NONE );

}


/***********************************************/
/*  Microcode Init into Program RAM Functions  */
/***********************************************/

  /* uCode Load through Register (MDIO) Interface [Return Val = Error_Code (0 = PASS)] */
uint16_t eagle_tsc_ucode_init( const phymod_access_t *pa ) {

    err_code_t __err=0;
    uint8_t result;

	wrc_micro_system_clk_en(0x1);                         /* Enable clock to micro  */
	wrc_micro_system_reset_n(0x1);                        /* De-assert reset to micro */
	wrc_micro_system_reset_n(0x0);                        /* Assert reset to micro - Toggling micro reset*/
	wrc_micro_system_reset_n(0x1);                        /* De-assert reset to micro */

	wrc_micro_mdio_ram_access_mode(0x0);                  /* Select Program Memory access mode - Program RAM load when Serdes DW8051 in reset */

	wrc_micro_ram_address(0x0);                           /* RAM start address */
	wrc_micro_init_cmd(0x0);                              /* Clear initialization command */
	wrc_micro_init_cmd(0x1);                              /* Set initialization command */

	eagle_tsc_delay_us(500);
	wrc_micro_init_cmd(0x0);                              /* Set initialization command */

    result = !rdc_micro_init_done();

	if (result) {                                        /* Check if initialization done within 500us time interval */
		PHYMOD_DEBUG_ERROR(("ERR_CODE_MICRO_INIT_NOT_DONE\n"));
		return (ERR_CODE_MICRO_INIT_NOT_DONE);    /* Else issue error code */
	}

	return (ERR_CODE_NONE);
}

int eagle_pmd_ln_h_rstb_pkill_override( const phymod_access_t *pa, uint16_t val) 
{
	CKRST_LN_RST_N_PWRDN_PIN_KILL_CTLr_t pin_ctl;

	/* 
	* Work around per Magesh/Justin
	* override input from PCS to allow uc_dsc_ready_for_cmd 
	* reg get written by UC
	*/ 
	CKRST_LN_RST_N_PWRDN_PIN_KILL_CTLr_CLR(pin_ctl);
	CKRST_LN_RST_N_PWRDN_PIN_KILL_CTLr_PMD_LN_H_RSTB_PKILLf_SET(pin_ctl, val);
	WRITE_CKRST_LN_RST_N_PWRDN_PIN_KILL_CTLr(pa, pin_ctl);
	return (ERR_CODE_NONE);
}

