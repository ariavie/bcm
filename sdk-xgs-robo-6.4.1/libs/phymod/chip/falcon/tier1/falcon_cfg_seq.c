/*
 *         
 * $Id: falcon.c,v 1.2.2.26 Broadcom SDK $
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
 *  $Id: 9ef217f4371321150095054233aec01d92acbfdb $
*/


#include <phymod/phymod.h>
#include "falcon_cfg_seq.h" 
#include "falcon_tsc_fields.h"
#include <phymod/chip/bcmi_tscf_xgxs_defs.h>

int falcon_tx_rx_polarity_set(const phymod_access_t *pa, uint32_t tx_pol, uint32_t rx_pol) 
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  __err = (uint32_t) wr_tx_pmd_dp_invert(tx_pol);
  if(__err) return(__err);
  __err = (uint32_t) wr_rx_pmd_dp_invert(rx_pol);
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_tx_rx_polarity_get(const phymod_access_t *pa, uint32_t *tx_pol, uint32_t *rx_pol) 
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *tx_pol = (uint32_t) rd_tx_pmd_dp_invert();
  if(__err) return(__err);
  *rx_pol = (uint32_t) rd_rx_pmd_dp_invert();
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_uc_active_set(const phymod_access_t *pa, uint32_t enable) 
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  __err=wrc_uc_active(enable);
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_uc_reset(const phymod_access_t *pa, uint32_t enable)
{
  return ERR_CODE_NONE;
}

int falcon_prbs_tx_inv_data_get(const phymod_access_t *pa, uint32_t *inv_data)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *inv_data = (uint32_t) rd_prbs_gen_inv();
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_prbs_rx_inv_data_get(const phymod_access_t *pa, uint32_t *inv_data)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *inv_data = (uint32_t) rd_prbs_gen_inv();
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_prbs_tx_poly_get(const phymod_access_t *pa, falcon_prbs_polynomial_type_t *prbs_poly)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *prbs_poly = rd_prbs_gen_mode_sel();
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_prbs_rx_poly_get(const phymod_access_t *pa, falcon_prbs_polynomial_type_t *prbs_poly)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *prbs_poly = rd_prbs_gen_mode_sel();
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_prbs_tx_enable_get(const phymod_access_t *pa, uint32_t *enable)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *enable = rd_prbs_gen_en();
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_prbs_rx_enable_get(const phymod_access_t *pa, uint32_t *enable)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *enable = rd_prbs_gen_en();
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_pmd_force_signal_detect(const phymod_access_t *pa, uint32_t enable)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  __err = wr_signal_detect_frc(enable); if(__err) return(__err);
  __err = wr_signal_detect_frc_val(enable); if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_pll_mode_set(const phymod_access_t *pa, int pll_mode)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  __err=wrc_pll_mode(pll_mode);
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_pll_mode_get(const phymod_access_t *pa, uint32_t *pll_mode)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *pll_mode=rdc_pll_mode();
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_osr_mode_set(const phymod_access_t *pa, int osr_mode)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  __err=wr_osr_mode_frc_val(osr_mode);
  if(__err) return(__err);
  __err=wr_osr_mode_frc(1);
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_osr_mode_get(const phymod_access_t *pa, int *osr_mode)
{
  int osr_forced;
  err_code_t __err;
  __err=ERR_CODE_NONE;
  osr_forced = rd_osr_mode_frc();
  if(osr_forced) {
    *osr_mode = rd_osr_mode_frc_val();
    if(__err) return(__err);
  } else {
    *osr_mode = rd_osr_mode_pin();
    if(__err) return(__err);
  }

  return ERR_CODE_NONE;
}

int falcon_tsc_dig_lpbk_get(const phymod_access_t *pa, uint32_t *lpbk)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *lpbk=rd_dig_lpbk_en();
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_tsc_rmt_lpbk_get(const phymod_access_t *pa, uint32_t *lpbk)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *lpbk=rd_rmt_lpbk_en();
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_core_soft_reset(const phymod_access_t *pa)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  __err=wrc_core_dp_s_rstb(1);
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_core_soft_reset_release(const phymod_access_t *pa, uint32_t enable)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  __err=wrc_core_dp_s_rstb(enable);
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_tsc_pwrdn_get(const phymod_access_t *pa, power_status_t *pwrdn)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  pwrdn->pll_pwrdn  = 0;
  pwrdn->tx_s_pwrdn = 0;
  pwrdn->rx_s_pwrdn = 0;
  pwrdn->pll_pwrdn  = rdc_afe_s_pll_pwrdn(); if(__err) return(__err);
  pwrdn->tx_s_pwrdn = rd_ln_tx_s_pwrdn();    if(__err) return(__err);
  pwrdn->rx_s_pwrdn = rd_ln_rx_s_pwrdn();    if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_pmd_lane_swap_tx(const phymod_access_t *pa, uint32_t tx_lane_map) {
  err_code_t __err;
  uint32_t lane_addr_0, lane_addr_1, lane_addr_2, lane_addr_3;

  __err=ERR_CODE_NONE;
  lane_addr_0 = (tx_lane_map >> 16) & 0xf;
  lane_addr_1 = (tx_lane_map >> 20) & 0xf;
  lane_addr_2 = (tx_lane_map >> 24) & 0xf;
  lane_addr_3 = (tx_lane_map >> 28) & 0xf;

  __err=wrc_lane_addr_0(lane_addr_0); if(__err) return(__err);
  __err=wrc_lane_addr_1(lane_addr_1); if(__err) return(__err);
  __err=wrc_lane_addr_2(lane_addr_2); if(__err) return(__err);
  __err=wrc_lane_addr_3(lane_addr_3); if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_pcs_lane_swap (const phymod_access_t *pa, uint32_t lane_map) {
  err_code_t __err;
  uint32_t lane_0, lane_1, lane_2, lane_3;

  __err=ERR_CODE_NONE;
  lane_0 = ((lane_map >> 0)  & 0x3);
  lane_1 = ((lane_map >> 4)  & 0x3);
  lane_2 = ((lane_map >> 8)  & 0x3);
  lane_3 = ((lane_map >> 12) & 0x3);

  __err=wrc_tx_lane_map_0(lane_0); if(__err) return(__err);
  __err=wrc_tx_lane_map_1(lane_1); if(__err) return(__err);
  __err=wrc_tx_lane_map_2(lane_2); if(__err) return(__err);
  __err=wrc_tx_lane_map_3(lane_3); if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_tsc_identify(const phymod_access_t *pa, falcon_rev_id0_t *rev_id0, falcon_rev_id1_t *rev_id1)
{
  err_code_t __err;

  rev_id0->revid_rev_letter =0;
  rev_id0->revid_rev_number =0;
  rev_id0->revid_bonding    =0;
  rev_id0->revid_process    =0;
  rev_id0->revid_model      =0;

  rev_id1->revid_multiplicity =0;
  rev_id1->revid_mdio         =0;
  rev_id1->revid_micro        =0;
  rev_id1->revid_cl72         =0;
  rev_id1->revid_pir          =0;
  rev_id1->revid_llp          =0;
  rev_id1->revid_eee          =0;

  __err=ERR_CODE_NONE;

  rev_id0->revid_rev_letter =rdc_revid_rev_letter(); if(__err) return(__err);
  rev_id0->revid_rev_number =rdc_revid_rev_number(); if(__err) return(__err);
  rev_id0->revid_bonding    =rdc_revid_bonding();    if(__err) return(__err);
  rev_id0->revid_process    =rdc_revid_process();    if(__err) return(__err);
  rev_id0->revid_model      =rdc_revid_model();      if(__err) return(__err);

  rev_id1->revid_multiplicity =rdc_revid_multiplicity(); if(__err) return(__err);
  rev_id1->revid_mdio         =rdc_revid_mdio();         if(__err) return(__err);
  rev_id1->revid_micro        =rdc_revid_micro();        if(__err) return(__err);
  rev_id1->revid_cl72         =rdc_revid_cl72();         if(__err) return(__err);
  rev_id1->revid_pir          =rdc_revid_pir();          if(__err) return(__err);
  rev_id1->revid_llp          =rdc_revid_llp();          if(__err) return(__err);
  rev_id1->revid_eee          =rdc_revid_eee();          if(__err) return(__err);

  return ERR_CODE_NONE;
}

int falcon_pmd_ln_h_rstb_pkill_override( const phymod_access_t *pa, uint16_t val) 
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

int falcon_lane_soft_reset_release(const phymod_access_t *pa, uint32_t enable)   /* release the pmd core soft reset */
{
    CKRST_LN_CLK_RST_N_PWRDWN_CTLr_t    reg_pwrdwn_ctrl;

    CKRST_LN_CLK_RST_N_PWRDWN_CTLr_CLR(reg_pwrdwn_ctrl);
    CKRST_LN_CLK_RST_N_PWRDWN_CTLr_LN_DP_S_RSTBf_SET(reg_pwrdwn_ctrl, enable);
    MODIFY_CKRST_LN_CLK_RST_N_PWRDWN_CTLr(pa,  reg_pwrdwn_ctrl);

    return PHYMOD_E_NONE;
}

int falcon_clause72_control(const phymod_access_t *pa, uint32_t cl_72_en)                /* CLAUSE_72_CONTROL */
{
  CL93N72_IT_BASE_R_PMD_CTLr_t    reg_cl72_ctrl ;

  CL93N72_IT_BASE_R_PMD_CTLr_CLR(reg_cl72_ctrl) ;

  if (cl_72_en) {
       CL93N72_IT_BASE_R_PMD_CTLr_CL93N72_IEEE_TRAINING_ENABLEf_SET(reg_cl72_ctrl, 1) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL93N72_IT_BASE_R_PMD_CTLr(pa, reg_cl72_ctrl));
       CL93N72_IT_BASE_R_PMD_CTLr_CL93N72_IEEE_RESTART_TRAININGf_SET(reg_cl72_ctrl, 1) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL93N72_IT_BASE_R_PMD_CTLr(pa, reg_cl72_ctrl));
  } else {
       CL93N72_IT_BASE_R_PMD_CTLr_CL93N72_IEEE_TRAINING_ENABLEf_SET(reg_cl72_ctrl, 0) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL93N72_IT_BASE_R_PMD_CTLr(pa, reg_cl72_ctrl));
       CL93N72_IT_BASE_R_PMD_CTLr_CL93N72_IEEE_RESTART_TRAININGf_SET(reg_cl72_ctrl, 0) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL93N72_IT_BASE_R_PMD_CTLr(pa, reg_cl72_ctrl));
  }
  return PHYMOD_E_NONE;
}

int falcon_clause72_control_get(const phymod_access_t *pa, uint32_t *cl_72_en)                /* CLAUSE_72_CONTROL */
{
  CL93N72_IT_BASE_R_PMD_CTLr_t    reg_cl72_ctrl ;

  CL93N72_IT_BASE_R_PMD_CTLr_CLR(reg_cl72_ctrl) ;

  READ_CL93N72_IT_BASE_R_PMD_CTLr(pa, &reg_cl72_ctrl);
  *cl_72_en = CL93N72_IT_BASE_R_PMD_CTLr_CL93N72_IEEE_TRAINING_ENABLEf_GET(reg_cl72_ctrl);

  return PHYMOD_E_NONE;
}




