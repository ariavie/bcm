/*
 *         
 * $Id: tscf_diagnostics.c,v 1.1.2.8 Broadcom SDK $
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

#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_util.h>
#include "../../tscf/tier1/tefmod_enum_defines.h" 
#include "../../tscf/tier1/tefmod.h" 
#include "../../falcon/tier1/falcon_tsc_common.h" 
#include "../../falcon/tier1/falcon_tsc_interface.h" 
#include "../../falcon/tier1/falcon_cfg_seq.h"
#include "../../falcon/tier1/falcon_tsc_debug_functions.h" 
  

#ifdef PHYMOD_TSCE_SUPPORT

/*phymod, internal enum mappings*/
STATIC
int _tscf_prbs_poly_phymod_to_falcon(phymod_prbs_poly_t phymod_poly, enum falcon_tsc_prbs_polynomial_enum *falcon_poly)
{
    switch(phymod_poly){
    case phymodPrbsPoly7:
        *falcon_poly = PRBS_7;
        break;
    case phymodPrbsPoly9:
        *falcon_poly = PRBS_9;
        break;
    case phymodPrbsPoly11:
        *falcon_poly = PRBS_11;
        break;
    case phymodPrbsPoly15:
        *falcon_poly = PRBS_15;
        break;
    case phymodPrbsPoly23:
        *falcon_poly = PRBS_23;
        break;
    case phymodPrbsPoly31:
        *falcon_poly = PRBS_31;
        break;
    case phymodPrbsPoly58:
        *falcon_poly = PRBS_58;
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported poly for tscf %u"), phymod_poly));
    }
    return PHYMOD_E_NONE;
}

STATIC
int _tscf_prbs_poly_tscf_to_phymod(falcon_prbs_polynomial_type_t tscf_poly, phymod_prbs_poly_t *phymod_poly)
{
    switch(tscf_poly){
    case FALCON_PRBS_POLYNOMIAL_7:
        *phymod_poly = phymodPrbsPoly7;
        break;
    case FALCON_PRBS_POLYNOMIAL_9:
        *phymod_poly = phymodPrbsPoly9;
        break;
    case FALCON_PRBS_POLYNOMIAL_11:
        *phymod_poly = phymodPrbsPoly11;
        break;
    case FALCON_PRBS_POLYNOMIAL_15:
        *phymod_poly = phymodPrbsPoly15;
        break;
    case FALCON_PRBS_POLYNOMIAL_23:
        *phymod_poly = phymodPrbsPoly23;
        break;
    case FALCON_PRBS_POLYNOMIAL_31:
        *phymod_poly = phymodPrbsPoly31;
        break;
    case FALCON_PRBS_POLYNOMIAL_58:
        *phymod_poly = phymodPrbsPoly58;
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("uknown poly %u"), tscf_poly));
    }
    return PHYMOD_E_NONE;
}

#ifdef PHYMOD_TO_TSCE_OS_MODE_TRANS
STATIC
int _tscf_os_mode_phymod_to_tscf(phymod_osr_mode_t phymod_os_mode, tefmod_os_mode_type *tscf_os_mode)
{
    switch(phymod_os_mode){
    case phymodOversampleMode1:
        *tscf_os_mode = TEFMOD_PMA_OS_MODE_1;
        break;
    case phymodOversampleMode2:
        *tscf_os_mode = TEFMOD_PMA_OS_MODE_2;
        break;
    case phymodOversampleMode3:
        *tscf_os_mode = TEFMOD_PMA_OS_MODE_3;
        break;
    case phymodOversampleMode3P3:
        *tscf_os_mode = TEFMOD_PMA_OS_MODE_3_3;
        break;
    case phymodOversampleMode4:
        *tscf_os_mode = TEFMOD_PMA_OS_MODE_4;
        break;
    case phymodOversampleMode5:
        *tscf_os_mode = TEFMOD_PMA_OS_MODE_5;
        break;
    case phymodOversampleMode8:
        *tscf_os_mode = TEFMOD_PMA_OS_MODE_8;
        break;
    case phymodOversampleMode8P25:
        *tscf_os_mode = TEFMOD_PMA_OS_MODE_8_25;
        break;
    case phymodOversampleMode10:
        *tscf_os_mode = TEFMOD_PMA_OS_MODE_10;
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported over sample mode for tscf %u"), phymod_os_mode));
    }
    return PHYMOD_E_NONE;
}

STATIC
int _tscf_os_mode_tscf_to_phymod(tefmod_os_mode_type tscf_os_mode, phymod_osr_mode_t *phymod_os_mode)
{
    switch(tscf_os_mode){
    case TEFMOD_PMA_OS_MODE_1:
        *phymod_os_mode = phymodOversampleMode1;
        break;
    case TEFMOD_PMA_OS_MODE_2:
        *phymod_os_mode = phymodOversampleMode2;
        break;
    case TEFMOD_PMA_OS_MODE_3:
        *phymod_os_mode = phymodOversampleMode3;
        break;
    case TEFMOD_PMA_OS_MODE_3_3:
        *phymod_os_mode = phymodOversampleMode3P3;
        break;
    case TEFMOD_PMA_OS_MODE_4:
        *phymod_os_mode = phymodOversampleMode4;
        break;
    case TEFMOD_PMA_OS_MODE_5:
        *phymod_os_mode = phymodOversampleMode5;
        break;
    case TEFMOD_PMA_OS_MODE_8:
        *phymod_os_mode = phymodOversampleMode8;
        break;
    case TEFMOD_PMA_OS_MODE_8_25:
        *phymod_os_mode = phymodOversampleMode8P25;
        break;
    case TEFMOD_PMA_OS_MODE_10:
        *phymod_os_mode = phymodOversampleMode10;
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("uknown os mode %u"), tscf_os_mode));
    }
    return PHYMOD_E_NONE;
}
#endif

/*diagnotics functions*/

int tscf_phy_rx_slicer_position_set(const phymod_phy_access_t* phy, uint32_t flags, const phymod_slicer_position_t* position)
{
        
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
        
    
}

int tscf_phy_rx_slicer_position_get(const phymod_phy_access_t* phy, uint32_t flags, phymod_slicer_position_t* position)
{
        
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
        
    
}


int tscf_phy_rx_slicer_position_max_get(const phymod_phy_access_t* phy, uint32_t flags, const phymod_slicer_position_t* position_min, const phymod_slicer_position_t* position_max)
{
        
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
        
    
}

int tscf_phy_prbs_config_set(const phymod_phy_access_t* phy, uint32_t flags , const phymod_prbs_t* prbs)
{
    enum falcon_tsc_prbs_polynomial_enum falcon_poly;
    PHYMOD_IF_ERR_RETURN(_tscf_prbs_poly_phymod_to_falcon(prbs->poly, &falcon_poly));
    /*first check which direction */
    if (PHYMOD_PRBS_DIRECTION_IS_RX(flags)) {
        PHYMOD_IF_ERR_RETURN
            (falcon_tsc_config_rx_prbs(&phy->access, falcon_poly, PRBS_INITIAL_SEED_HYSTERESIS,  prbs->invert));
    } else if (PHYMOD_PRBS_DIRECTION_IS_TX(flags)) {
        PHYMOD_IF_ERR_RETURN
            (falcon_tsc_config_tx_prbs(&phy->access, falcon_poly, prbs->invert));
    } else {
        PHYMOD_IF_ERR_RETURN
            (falcon_tsc_config_rx_prbs(&phy->access, falcon_poly, PRBS_INITIAL_SEED_HYSTERESIS,  prbs->invert));
        PHYMOD_IF_ERR_RETURN
            (falcon_tsc_config_tx_prbs(&phy->access, falcon_poly, prbs->invert));
    }         
    return PHYMOD_E_NONE;
}

int tscf_phy_prbs_config_get(const phymod_phy_access_t* phy, uint32_t flags , phymod_prbs_t* prbs)
{
    phymod_prbs_t config_tmp;
    falcon_prbs_polynomial_type_t tscf_poly;

    if (PHYMOD_PRBS_DIRECTION_IS_TX(flags)) {
        PHYMOD_IF_ERR_RETURN(falcon_prbs_tx_inv_data_get(&phy->access, &config_tmp.invert));
        PHYMOD_IF_ERR_RETURN(falcon_prbs_tx_poly_get(&phy->access, &tscf_poly));
        PHYMOD_IF_ERR_RETURN(_tscf_prbs_poly_tscf_to_phymod(tscf_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    } else if (PHYMOD_PRBS_DIRECTION_IS_RX(flags)) {
        PHYMOD_IF_ERR_RETURN(falcon_prbs_rx_inv_data_get(&phy->access, &config_tmp.invert));
        PHYMOD_IF_ERR_RETURN(falcon_prbs_rx_poly_get(&phy->access, &tscf_poly));
        PHYMOD_IF_ERR_RETURN(_tscf_prbs_poly_tscf_to_phymod(tscf_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    } else {
        PHYMOD_IF_ERR_RETURN(falcon_prbs_tx_inv_data_get(&phy->access, &config_tmp.invert));
        PHYMOD_IF_ERR_RETURN(falcon_prbs_tx_poly_get(&phy->access, &tscf_poly));
        PHYMOD_IF_ERR_RETURN(_tscf_prbs_poly_tscf_to_phymod(tscf_poly, &config_tmp.poly));
        prbs->invert = config_tmp.invert;
        prbs->poly = config_tmp.poly;
    }
    return PHYMOD_E_NONE;
}



int tscf_phy_prbs_enable_set(const phymod_phy_access_t* phy, uint32_t flags , uint32_t enable)
{
    if (PHYMOD_PRBS_DIRECTION_IS_TX(flags)) {
        PHYMOD_IF_ERR_RETURN(falcon_tsc_tx_prbs_en(&phy->access, enable));
    } else if (PHYMOD_PRBS_DIRECTION_IS_RX(flags)) {
        PHYMOD_IF_ERR_RETURN(falcon_tsc_rx_prbs_en(&phy->access, enable));
    } else {
        PHYMOD_IF_ERR_RETURN(falcon_tsc_tx_prbs_en(&phy->access, enable));
        PHYMOD_IF_ERR_RETURN(falcon_tsc_rx_prbs_en(&phy->access, enable));
    }
    return PHYMOD_E_NONE;   
}

int tscf_phy_prbs_enable_get(const phymod_phy_access_t* phy, uint32_t flags , uint32_t *enable)
{
    uint32_t enable_tmp;
    if (PHYMOD_PRBS_DIRECTION_IS_TX(flags)) {
        PHYMOD_IF_ERR_RETURN(falcon_prbs_tx_enable_get(&phy->access, &enable_tmp));
        *enable = enable_tmp;
    } else if (PHYMOD_PRBS_DIRECTION_IS_RX(flags)) {
        PHYMOD_IF_ERR_RETURN(falcon_prbs_rx_enable_get(&phy->access, &enable_tmp));
        *enable = enable_tmp;
    } else {
        PHYMOD_IF_ERR_RETURN(falcon_prbs_tx_enable_get(&phy->access, &enable_tmp));
        *enable = enable_tmp;
        PHYMOD_IF_ERR_RETURN(falcon_prbs_rx_enable_get(&phy->access, &enable_tmp));
        *enable &= enable_tmp;
    }

    return PHYMOD_E_NONE;
}


int tscf_phy_prbs_status_get(const phymod_phy_access_t* phy, uint32_t flags, phymod_prbs_status_t* prbs_status)
{
    uint8_t status = 0;
    uint32_t prbs_err_count = 0;

    PHYMOD_IF_ERR_RETURN(falcon_tsc_prbs_chk_lock_state(&phy->access, &status));
    if (status) {
        prbs_status->prbs_lock = 1;
        /*next check the lost of lock and error count */
        status = 0;
        PHYMOD_IF_ERR_RETURN
            (falcon_tsc_prbs_err_count_state(&phy->access, &prbs_err_count, &status));
        if (status) {
        /*temp lost of lock */
            prbs_status->prbs_lock_loss = 1;
        } else {
            prbs_status->prbs_lock_loss = 0;
            prbs_status->error_count = prbs_err_count;
        }
    } else {
        prbs_status->prbs_lock = 0;
    }
    return PHYMOD_E_NONE;
}


int tscf_phy_pattern_config_set(const phymod_phy_access_t* phy, const phymod_pattern_t* pattern, uint32_t* mode_sel)
{
#ifdef HOANG
    PHYMOD_IF_ERR_RETURN
        (serdes_config_shared_tx_pattern(&phy->access, (uint8_t *)mode_sel,  
                                        pattern->pattern_len, 
                                        (char *) pattern->pattern));
#endif
    return PHYMOD_E_NONE;
}

int tscf_phy_pattern_config_get(const phymod_phy_access_t* phy, phymod_pattern_t* pattern)
{
    /* Place your code here */

        
    return PHYMOD_E_NONE;
}

int tscf_phy_pattern_enable_set(const phymod_phy_access_t* phy, uint32_t enable, uint32_t mode_sel)
{
#ifdef HOANG
    PHYMOD_IF_ERR_RETURN
        (serdes_tx_shared_patt_gen_en(&phy->access, enable,(uint8_t)  mode_sel)); 
#endif
    return PHYMOD_E_NONE;
}

int tscf_phy_pattern_enable_get(const phymod_phy_access_t* phy, uint32_t* enable)
{
        
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
}

int tscf_core_diagnostics_get(const phymod_core_access_t* core, phymod_core_diagnostics_t* diag)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int tscf_phy_diagnostics_get(const phymod_phy_access_t* phy, phymod_phy_diagnostics_t* diag)
{
    
    /* tefmod_os_mode_type os_mode = TEFMOD_PMA_OS_MODE_1; */
    phymod_diag_eyescan_t_init(&diag->eyescan);
    phymod_diag_slicer_offset_t_init(&diag->slicer_offset);

    PHYMOD_IF_ERR_RETURN(tefmod_pmd_lock_get(&phy->access, &diag->rx_lock));
    
    /* PHYMOD_IF_ERR_RETURN(_tscf_os_mode_tscf_to_phymod(os_mode, &diag->osr_mode)); */
        
    return PHYMOD_E_NONE;
    
}

int tscf_phy_pmd_info_dump(const phymod_phy_access_t* phy, uint32_t mode)
{
    int start_lane, num_lane;
    phymod_phy_access_t phy_copy;
    int i = 0;


    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
 
    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    for (i = 0; i < num_lane; i++) {
        phy_copy.access.lane = 0x1 << (i + start_lane);

        switch(mode) {
            default:
               PHYMOD_IF_ERR_RETURN
                  (falcon_tsc_display_lane_state_hdr(&phy_copy.access));
               PHYMOD_IF_ERR_RETURN
                  (falcon_tsc_display_lane_state(&phy_copy.access));
               break;
        }
    }
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_display_lane_state_legend(&phy_copy.access));

    return PHYMOD_E_NONE;
}

int tscf_phy_pcs_info_dump(const phymod_phy_access_t* phy, uint32_t mode)
{
    int start_lane, num_lane;
    phymod_phy_access_t phy_copy;
    int i = 0;


    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
 
    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    for (i = 0; i < num_lane; i++) {
        phy_copy.access.lane = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
            (tefmod_diag(&phy_copy.access, mode));
    }

    return PHYMOD_E_NONE;
}

err_code_t tscf_phy_meas_lowber_eye (const phymod_access_t *pa,
                                     phymod_phy_eyescan_options_t eyescan_options,
                                     uint32_t *buffer)

{
    struct falcon_tsc_eyescan_options_st e_options;

    e_options.linerate_in_khz = eyescan_options.linerate_in_khz;
    e_options.timeout_in_milliseconds = eyescan_options.timeout_in_milliseconds;
    e_options.horz_max = eyescan_options.horz_max;
    e_options.horz_min = eyescan_options.horz_min;
    e_options.hstep = eyescan_options.hstep;
    e_options.vert_max = eyescan_options.vert_max;
    e_options.vert_min = eyescan_options.vert_min;
    e_options.vstep = eyescan_options.vstep;
    e_options.mode = eyescan_options.mode;

    return (falcon_tsc_meas_lowber_eye(pa, e_options, buffer));
}

err_code_t tscf_phy_display_lowber_eye (const phymod_access_t *pa,
                                        phymod_phy_eyescan_options_t eyescan_options,
                                        uint32_t *buffer)
{
    struct falcon_tsc_eyescan_options_st e_options;

    e_options.linerate_in_khz = eyescan_options.linerate_in_khz;
    e_options.timeout_in_milliseconds = eyescan_options.timeout_in_milliseconds;
    e_options.horz_max = eyescan_options.horz_max;
    e_options.horz_min = eyescan_options.horz_min;
    e_options.hstep = eyescan_options.hstep;
    e_options.vert_max = eyescan_options.vert_max;
    e_options.vert_min = eyescan_options.vert_min;
    e_options.vstep = eyescan_options.vstep;
    e_options.mode = eyescan_options.mode;

    return(falcon_tsc_display_lowber_eye (pa, e_options, buffer));
}


err_code_t tscf_phy_pmd_ber_end_cmd (const phymod_access_t *pa,
                                uint8_t supp_info, uint32_t timeout_ms)
{
    return (falcon_tsc_pmd_uc_cmd(pa, CMD_CAPTURE_BER_END, supp_info, timeout_ms));
}

err_code_t tscf_phy_meas_eye_scan_start (const phymod_access_t *pa, uint8_t direction)
{
    return(falcon_tsc_meas_eye_scan_start(pa, direction));
}

err_code_t tscf_phy_read_eye_scan_stripe (const phymod_access_t *pa, uint32_t *buffer,uint16_t *status)
{
    return (falcon_tsc_read_eye_scan_stripe(pa, buffer, status));
}

err_code_t tscf_phy_display_eye_scan_header (const phymod_access_t *pa, int8_t i)
{
    return (falcon_tsc_display_eye_scan_header(pa, i));
}

err_code_t tscf_phy_display_eye_scan_footer (const phymod_access_t *pa, int8_t i)
{
    return (falcon_tsc_display_eye_scan_footer(pa, i));
}

err_code_t tscf_phy_display_eye_scan_stripe (const phymod_access_t *pa, int8_t y,uint32_t *buffer)
{
    return (falcon_tsc_display_eye_scan_stripe(pa, y, buffer));
}

err_code_t tscf_phy_meas_eye_scan_done( const phymod_access_t *pa )
{
    return (falcon_tsc_meas_eye_scan_done(pa));
}

err_code_t tscf_phy_eye_scan_debug_info_dump( const phymod_access_t *pa )
{
    return PHYMOD_E_NONE;
}

#endif /* PHYMOD_TSCE_SUPPORT */
