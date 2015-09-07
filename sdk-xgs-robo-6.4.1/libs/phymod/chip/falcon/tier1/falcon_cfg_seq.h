/*----------------------------------------------------------------------
 * $Id: falcon_cfg_seq.h,v 1.1.2.2 Broadcom SDK $ 
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
 * File       : falcon_cfg_seq.h
 * Description: c functions implementing Tier1s for TEMod Serdes Driver
m
m
M
k
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
 *  $Id: b7d69c8996fbbbf0d1d0c2515afcf22a90789a8c $
*/


#ifndef FALCON_CFG_SEQ_H 
#define FALCON_CFG_SEQ_H

#include "falcon_tsc_err_code.h"
#include "falcon_tsc_enum.h"

typedef struct {
  int8_t pll_pwrdn;
  int8_t tx_s_pwrdn;
  int8_t rx_s_pwrdn;
} power_status_t;

typedef struct {
  int8_t revid_model;
  int8_t revid_process;
  int8_t revid_bonding;
  int8_t revid_rev_number;
  int8_t revid_rev_letter;
} falcon_rev_id0_t;

typedef struct {
  int8_t revid_eee;
  int8_t revid_llp; 
  int8_t revid_pir; 
  int8_t revid_cl72; 
  int8_t revid_micro; 
  int8_t revid_mdio; 
  int8_t revid_multiplicity;
} falcon_rev_id1_t;

typedef enum {
  TX = 0,
  Rx
} tx_rx_t;

typedef enum {
    FALCON_PRBS_POLYNOMIAL_7 = 0,
    FALCON_PRBS_POLYNOMIAL_9,
    FALCON_PRBS_POLYNOMIAL_11,
    FALCON_PRBS_POLYNOMIAL_15,
    FALCON_PRBS_POLYNOMIAL_23,
    FALCON_PRBS_POLYNOMIAL_31,
    FALCON_PRBS_POLYNOMIAL_58,
    FALCON_PRBS_POLYNOMIAL_TYPE_COUNT 
}falcon_prbs_polynomial_type_t;

extern err_code_t _falcon_tsc_pmd_mwr_reg_byte( const phymod_access_t *pa, uint16_t addr, uint16_t mask, uint8_t lsb, uint8_t val);
extern uint8_t _falcon_tsc_pmd_rde_field_byte( const phymod_access_t *pa, uint16_t addr, uint8_t shift_left, uint8_t shift_right, err_code_t *err_code_p);

int falcon_tx_rx_polarity_set(const phymod_access_t *pa, uint32_t tx_pol, uint32_t rx_pol);
int falcon_tx_rx_polarity_get(const phymod_access_t *pa, uint32_t *tx_pol, uint32_t *rx_pol);
int falcon_uc_active_set(const phymod_access_t *pa, uint32_t enable);
int falcon_uc_reset(const phymod_access_t *pa, uint32_t enable);
int falcon_prbs_tx_inv_data_get(const phymod_access_t *pa, uint32_t *inv_data);
int falcon_prbs_rx_inv_data_get(const phymod_access_t *pa, uint32_t *inv_data);
int falcon_prbs_tx_poly_get(const phymod_access_t *pa, falcon_prbs_polynomial_type_t *prbs_poly);
int falcon_prbs_rx_poly_get(const phymod_access_t *pa, falcon_prbs_polynomial_type_t *prbs_poly);
int falcon_prbs_tx_enable_get(const phymod_access_t *pa, uint32_t *enable);
int falcon_prbs_rx_enable_get(const phymod_access_t *pa, uint32_t *enable);
int falcon_pmd_force_signal_detect(const phymod_access_t *pa, uint32_t enable);
int falcon_pll_mode_set(const phymod_access_t *pa, int pll_mode);
int falcon_pll_mode_get(const phymod_access_t *pa, uint32_t *pll_mode);
int falcon_osr_mode_set(const phymod_access_t *pa, int osr_mode);
int falcon_osr_mode_get(const phymod_access_t *pa, int *osr_mode);
int falcon_tsc_dig_lpbk_get(const phymod_access_t *pa, uint32_t *lpbk);
int falcon_tsc_rmt_lpbk_get(const phymod_access_t *pa, uint32_t *lpbk);
int falcon_core_soft_reset(const phymod_access_t *pa);
int falcon_core_soft_reset_release(const phymod_access_t *pa, uint32_t enable);
int falcon_tsc_pwrdn_get(const phymod_access_t *pa, power_status_t *pwrdn);
int falcon_pmd_lane_swap_tx(const phymod_access_t *pa, uint32_t tx_lane_map);
int falcon_pcs_lane_swap(const phymod_access_t *pa, uint32_t lane_map);
int falcon_tsc_identify(const phymod_access_t *pa, falcon_rev_id0_t *rev_id0, falcon_rev_id1_t *rev_id1);
int falcon_pmd_ln_h_rstb_pkill_override( const phymod_access_t *pa, uint16_t val); 
int falcon_lane_soft_reset_release(const phymod_access_t *pa, uint32_t enable);   /* release the pmd core soft reset */
int falcon_clause72_control(const phymod_access_t *pc, uint32_t cl_72_en);                /* CLAUSE_72_CONTROL */
int falcon_clause72_control_get(const phymod_access_t *pc, uint32_t *cl_72_en);                /* CLAUSE_72_CONTROL */

#endif /* PHY_TSC_IBLK_H */
