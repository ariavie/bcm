/*
 *         
 * $Id: phymod.xml,v 1.1.2.5 Broadcom SDK $
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

#include <phymod/phymod.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_dispatch.h>

#include "../../falcon/tier1/falcon_cfg_seq.h"
#include "../../falcon/tier1/falcon_tsc_enum.h"
#include "../../falcon/tier1/falcon_tsc_common.h"
#include "../../falcon/tier1/falcon_tsc_interface.h"

#define TSCF_PHY_ALL_LANES 0xf
#define TSCE_CORE_TO_PHY_ACCESS(_phy_access, _core_access) \
    do{\
        PHYMOD_MEMCPY(&(_phy_access)->access, &(_core_access)->access, sizeof((_phy_access)->access));\
        (_phy_access)->type = (_core_access)->type; \
        (_phy_access)->access.lane = TSCF_PHY_ALL_LANES; \
    }while(0)

#define TSCF_NOF_DFES 9
#define TSCF_NOF_LANES_IN_CORE 4
extern unsigned char  tscf_ucode[];
extern unsigned short tscf_ucode_len;


#ifdef PHYMOD_FALCON_SUPPORT

int falcon_core_identify(const phymod_core_access_t* core, uint32_t core_id, uint32_t* is_identified)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int falcon_core_info_get(const phymod_core_access_t* core, phymod_core_info_t* info)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int falcon_core_pll_sequencer_restart(const phymod_core_access_t* core, uint32_t flags, phymod_sequencer_operation_t operation)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int falcon_core_lane_map_get(const phymod_core_access_t* core, phymod_lane_map_t* lane_map)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int falcon_core_reset_set(const phymod_core_access_t* core, phymod_reset_mode_t reset_mode, phymod_reset_direction_t direction)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int falcon_core_reset_get(const phymod_core_access_t* core, phymod_reset_mode_t reset_mode, phymod_reset_direction_t* direction)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int falcon_core_firmware_info_get(const phymod_core_access_t* core, phymod_core_firmware_info_t* fw_info)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int falcon_phy_tx_lane_control_set(const phymod_phy_access_t* phy, uint32_t enable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int falcon_phy_tx_lane_control_get(const phymod_phy_access_t* phy, uint32_t* enable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

/*Rx control*/
int falcon_phy_rx_lane_control_set(const phymod_phy_access_t* phy, uint32_t enable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int falcon_phy_rx_lane_control_get(const phymod_phy_access_t* phy, uint32_t* enable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int falcon_phy_autoneg_ability_set(const phymod_phy_access_t* phy, const phymod_autoneg_ability_t* an_ability_set_type)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int falcon_phy_autoneg_ability_get(const phymod_phy_access_t* phy, phymod_autoneg_ability_t* an_ability_get_type)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int falcon_phy_autoneg_set(const phymod_phy_access_t* phy, const phymod_autoneg_control_t* an)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int falcon_phy_autoneg_get(const phymod_phy_access_t* phy, phymod_autoneg_control_t* an)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int falcon_phy_autoneg_status_get(const phymod_phy_access_t* phy, phymod_autoneg_status_t* status)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int falcon_phy_firmware_core_config_set(const phymod_phy_access_t* phy, phymod_firmware_core_config_t fw_config)
{
    struct falcon_tsc_uc_core_config_st serdes_firmware_core_config;
    PHYMOD_MEMSET(&serdes_firmware_core_config, 0, sizeof(serdes_firmware_core_config));
    serdes_firmware_core_config.field.core_cfg_from_pcs = fw_config.CoreConfigFromPCS;
    serdes_firmware_core_config.field.vco_rate = fw_config.VcoRate; 
    PHYMOD_IF_ERR_RETURN(falcon_tsc_set_uc_core_config(&phy->access, serdes_firmware_core_config));
    return PHYMOD_E_NONE;
}


int falcon_phy_firmware_core_config_get(const phymod_phy_access_t* phy, phymod_firmware_core_config_t* fw_config)
{
    struct falcon_tsc_uc_core_config_st serdes_firmware_core_config;
    PHYMOD_IF_ERR_RETURN(falcon_tsc_get_uc_core_config(&phy->access, &serdes_firmware_core_config));
    PHYMOD_MEMSET(fw_config, 0, sizeof(*fw_config));
    fw_config->CoreConfigFromPCS = serdes_firmware_core_config.field.core_cfg_from_pcs;
    fw_config->VcoRate = serdes_firmware_core_config.field.vco_rate;
    return PHYMOD_E_NONE; 
}


int falcon_phy_firmware_lane_config_get(const phymod_phy_access_t* phy, phymod_firmware_lane_config_t* fw_config)
{
    struct falcon_tsc_uc_lane_config_st serdes_firmware_config;
    PHYMOD_IF_ERR_RETURN(falcon_tsc_get_uc_lane_cfg(&phy->access, &serdes_firmware_config));
    PHYMOD_MEMSET(fw_config, 0, sizeof(*fw_config));
    fw_config->LaneConfigFromPCS = serdes_firmware_config.field.lane_cfg_from_pcs;
    fw_config->AnEnabled         = serdes_firmware_config.field.an_enabled;
    fw_config->DfeOn             = serdes_firmware_config.field.dfe_on;
    fw_config->ForceBrDfe        = serdes_firmware_config.field.force_brdfe_on;
    fw_config->Cl72Enable        = serdes_firmware_config.field.cl72_emulation_en;
    fw_config->ScramblingDisable = serdes_firmware_config.field.scrambling_dis;
    fw_config->UnreliableLos     = serdes_firmware_config.field.unreliable_los;
    fw_config->MediaType         = serdes_firmware_config.field.media_type;
     
    return PHYMOD_E_NONE; 
}

int falcon_phy_tx_set(const phymod_phy_access_t* phy, const phymod_tx_t* tx)
{
    
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_write_tx_afe(&phy->access, TX_AFE_PRE, tx->pre));
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_write_tx_afe(&phy->access, TX_AFE_MAIN, tx->main));
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_write_tx_afe(&phy->access, TX_AFE_POST1, tx->post));
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_write_tx_afe(&phy->access, TX_AFE_POST2, tx->post2));
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_write_tx_afe(&phy->access, TX_AFE_POST3, tx->post3));
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_write_tx_afe(&phy->access, TX_AFE_AMP,  tx->amp));
    
    return PHYMOD_E_NONE;
}

int falcon_phy_media_type_tx_get(const phymod_phy_access_t* phy, phymod_media_typed_t media, phymod_tx_t* tx)
{
    /* Place your code here */

    PHYMOD_MEMSET(tx, 0, sizeof(*tx));
        
    return PHYMOD_E_NONE;
        
    
}

/* 
 * set lane swapping for core 
 * The tx swap is composed of PCS swap and after that the PMD swap. 
 * The rx swap is composed just by PCS swap 
 */

int falcon_core_lane_map_set(const phymod_core_access_t* core, const phymod_lane_map_t* lane_map)
{
    uint32_t pcs_swap = 0 , pmd_swap = 0, lane;

    if(lane_map->num_of_lanes != TSCF_NOF_LANES_IN_CORE){
        return PHYMOD_E_CONFIG;
    }
    for( lane = 0 ; lane < TSCF_NOF_LANES_IN_CORE ; lane++){
        if(lane_map->lane_map_rx[lane] >= TSCF_NOF_LANES_IN_CORE){
            return PHYMOD_E_CONFIG;
        }
        /*encode each lane as four bits*/
        /*pcs_map[lane] = rx_map[lane]*/
        pcs_swap += lane_map->lane_map_rx[lane]<<(lane*4);
    }

    for( lane = 0 ; lane < TSCF_NOF_LANES_IN_CORE ; lane++){
        if(lane_map->lane_map_tx[lane] >= TSCF_NOF_LANES_IN_CORE){
            return PHYMOD_E_CONFIG;
        }
        /*encode each lane as four bits*/
        /*considering the pcs lane swap: pmd_map[pcs_map[lane]] = tx_map[lane]*/
        pmd_swap += lane_map->lane_map_tx[lane]<<(lane_map->lane_map_rx[lane]*4);
    }

    PHYMOD_IF_ERR_RETURN(falcon_pcs_lane_swap(&core->access, pcs_swap));
    PHYMOD_IF_ERR_RETURN
        (falcon_pmd_lane_swap_tx(&core->access,
                                pmd_swap));
    return PHYMOD_E_NONE;
}

int _phymod_pll_multiplier_get(uint32_t pll_div, uint32_t *pll_multiplier)
{
    switch (pll_div) {
    case 0x0:
        *pll_multiplier = 64;
        break;
    case 0x1:
        *pll_multiplier = 66;
        break;
    case 0x2:
        *pll_multiplier = 80;
        break;
    case 0x3:
        *pll_multiplier = 128;
        break;
    case 0x4:
        *pll_multiplier = 132;
        break;
    case 0x5:
        *pll_multiplier = 140;
        break;
    case 0x6:
        *pll_multiplier = 160;
        break;
    case 0x7:
        *pll_multiplier = 165;
        break;
    case 0x8:
        *pll_multiplier = 168;
        break;
    case 0x9:
        *pll_multiplier = 170;
        break;
    case 0xa:
        *pll_multiplier = 175;
        break;
    case 0xb:
        *pll_multiplier = 180;
        break;
    case 0xc:
        *pll_multiplier = 184;
        break;
    case 0xd:
        *pll_multiplier = 200;
        break;
    case 0xe:
        *pll_multiplier = 224;
        break;
    case 0xf:
        *pll_multiplier = 264;
        break;
    default:
        *pll_multiplier = 165;
        break;
    }
    return PHYMOD_E_NONE;
}


int _phymod_speed_id_interface_config_get(const phymod_phy_access_t* phy, int vco_rate, int osr_mode, phymod_phy_inf_config_t* config)
{
    config->data_rate = vco_rate/osr_mode;
    return PHYMOD_E_NONE;
}


int falcon_phy_firmware_lane_config_set(const phymod_phy_access_t* phy, phymod_firmware_lane_config_t fw_config)
{
    struct falcon_tsc_uc_lane_config_st serdes_firmware_config;
    serdes_firmware_config.field.lane_cfg_from_pcs = fw_config.LaneConfigFromPCS;
    serdes_firmware_config.field.an_enabled        = fw_config.AnEnabled;
    serdes_firmware_config.field.dfe_on            = fw_config.DfeOn; 
    serdes_firmware_config.field.force_brdfe_on    = fw_config.ForceBrDfe;
    serdes_firmware_config.field.cl72_emulation_en = fw_config.Cl72Enable;
    serdes_firmware_config.field.scrambling_dis    = fw_config.ScramblingDisable;
    serdes_firmware_config.field.unreliable_los    = fw_config.UnreliableLos;
    serdes_firmware_config.field.media_type        = fw_config.MediaType;
    PHYMOD_IF_ERR_RETURN(falcon_tsc_set_uc_lane_cfg(&phy->access, serdes_firmware_config));
    return ERR_CODE_NONE;
}


/* reset rx sequencer 
 * flags - unused parameter
 */
int falcon_phy_rx_restart(const phymod_phy_access_t* phy)
{
    PHYMOD_IF_ERR_RETURN(falcon_tsc_rx_restart(&phy->access, 1)); 
    return ERR_CODE_NONE;
}


int falcon_phy_polarity_set(const phymod_phy_access_t* phy, const phymod_polarity_t* polarity)
{
    PHYMOD_IF_ERR_RETURN
        (falcon_tx_rx_polarity_set(&phy->access, polarity->tx_polarity, polarity->rx_polarity));
    return ERR_CODE_NONE;
}


int falcon_phy_polarity_get(const phymod_phy_access_t* phy, phymod_polarity_t* polarity)
{
   PHYMOD_IF_ERR_RETURN
        (falcon_tx_rx_polarity_get(&phy->access, &polarity->tx_polarity, &polarity->rx_polarity));
    return ERR_CODE_NONE;
}

int falcon_phy_tx_get(const phymod_phy_access_t* phy, phymod_tx_t* tx)
{
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_read_tx_afe(&phy->access, TX_AFE_PRE, &tx->pre));
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_read_tx_afe(&phy->access, TX_AFE_MAIN, &tx->main));
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_read_tx_afe(&phy->access, TX_AFE_POST1, &tx->post));
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_read_tx_afe(&phy->access, TX_AFE_POST2, &tx->post2));
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_read_tx_afe(&phy->access, TX_AFE_POST3, &tx->post3));
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_read_tx_afe(&phy->access, TX_AFE_AMP, &tx->amp));
    return ERR_CODE_NONE;
}



int falcon_phy_tx_override_set(const phymod_phy_access_t* phy, const phymod_tx_override_t* tx_override)
{
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_tx_pi_freq_override(&phy->access,
                                    tx_override->phase_interpolator.enable,
                                    tx_override->phase_interpolator.value));    
    return ERR_CODE_NONE;
}

int falcon_phy_tx_override_get(const phymod_phy_access_t* phy, phymod_tx_override_t* tx_override)
{
/*
    PHYMOD_IF_ERR_RETURN
        (temod_tx_pi_control_get(&phy->access, &tx_override->phase_interpolator.value));
*/
    return ERR_CODE_NONE;
}


int falcon_phy_rx_set(const phymod_phy_access_t* phy, const phymod_rx_t* rx)
{
    uint32_t i;

    /*params check*/
    if((rx->num_of_dfe_taps == 0) || (rx->num_of_dfe_taps < TSCF_NOF_DFES)){
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("illegal number of DFEs to set %u"), rx->num_of_dfe_taps));
    }

    /*vga set*/
    if (rx->vga.enable) {
        /* first stop the rx adaption */
        PHYMOD_IF_ERR_RETURN(falcon_tsc_stop_rx_adaptation(&phy->access, 1));
        PHYMOD_IF_ERR_RETURN(falcon_tsc_write_rx_afe(&phy->access, RX_AFE_VGA, rx->vga.value));
    } else {
        PHYMOD_IF_ERR_RETURN(falcon_tsc_stop_rx_adaptation(&phy->access, 0));
    }
    
    /*dfe set*/
    for (i = 0 ; i < rx->num_of_dfe_taps ; i++){
        if(rx->dfe[i].enable){
            PHYMOD_IF_ERR_RETURN(falcon_tsc_stop_rx_adaptation(&phy->access, 1));
            switch (i) {
                case 0:
                    PHYMOD_IF_ERR_RETURN(falcon_tsc_write_rx_afe(&phy->access, RX_AFE_DFE1, rx->dfe[i].value));
                    break;
                case 1:
                    PHYMOD_IF_ERR_RETURN(falcon_tsc_write_rx_afe(&phy->access, RX_AFE_DFE2, rx->dfe[i].value));
                    break;
                case 2:
                    PHYMOD_IF_ERR_RETURN(falcon_tsc_write_rx_afe(&phy->access, RX_AFE_DFE3, rx->dfe[i].value));
                    break;
                case 3:
                    PHYMOD_IF_ERR_RETURN(falcon_tsc_write_rx_afe(&phy->access, RX_AFE_DFE4, rx->dfe[i].value));
                    break;
                case 4:
                    PHYMOD_IF_ERR_RETURN(falcon_tsc_write_rx_afe(&phy->access, RX_AFE_DFE5, rx->dfe[i].value));
                    break;
                default:
                    return PHYMOD_E_PARAM;
            }
        } else {
            PHYMOD_IF_ERR_RETURN(falcon_tsc_stop_rx_adaptation(&phy->access, 0));
        }
    }
     
    /*peaking filter set*/
    if(rx->peaking_filter.enable){
        /* first stop the rx adaption */
        PHYMOD_IF_ERR_RETURN(falcon_tsc_stop_rx_adaptation(&phy->access, 1));
        PHYMOD_IF_ERR_RETURN(falcon_tsc_write_rx_afe(&phy->access, RX_AFE_PF, rx->peaking_filter.value));
    } else {
        PHYMOD_IF_ERR_RETURN(falcon_tsc_stop_rx_adaptation(&phy->access, 0));
    }
    
    if(rx->low_freq_peaking_filter.enable){
        /* first stop the rx adaption */
        PHYMOD_IF_ERR_RETURN(falcon_tsc_stop_rx_adaptation(&phy->access, 1));
        PHYMOD_IF_ERR_RETURN(falcon_tsc_write_rx_afe(&phy->access, RX_AFE_PF2, rx->low_freq_peaking_filter.value));
    } else {
        PHYMOD_IF_ERR_RETURN(falcon_tsc_stop_rx_adaptation(&phy->access, 0));
    }
    return ERR_CODE_NONE;
}


int falcon_phy_rx_get(const phymod_phy_access_t* phy, phymod_rx_t* rx)
{
    
    return ERR_CODE_NONE;
}


int falcon_phy_reset_set(const phymod_phy_access_t* phy, const phymod_phy_reset_t* reset)
{
        
        
    
    /* Place your code here */

        
    return ERR_CODE_NONE;
        
    
}


int falcon_phy_reset_get(const phymod_phy_access_t* phy, phymod_phy_reset_t* reset)
{
        
        
    
    /* Place your code here */

        
    return ERR_CODE_NONE;
        
    
}


int falcon_phy_power_set(const phymod_phy_access_t* phy, const phymod_phy_power_t* power)
{
    if ((power->tx == phymodPowerOff) && (power->rx == phymodPowerNoChange)) {
            /*disable tx on the PMD side */
            PHYMOD_IF_ERR_RETURN(falcon_tsc_tx_disable(&phy->access, 1));
    }
    if ((power->tx == phymodPowerOn) && (power->rx == phymodPowerNoChange)) {
            /*enable tx on the PMD side */
            PHYMOD_IF_ERR_RETURN(falcon_tsc_tx_disable(&phy->access, 0));
    }
    if ((power->tx == phymodPowerNoChange) && (power->rx == phymodPowerOff)) {
            /*disable rx on the PMD side */
    }
    if ((power->tx == phymodPowerNoChange) && (power->rx == phymodPowerOn)) {
            /*enable rx on the PMD side */
    }
    return ERR_CODE_NONE; 
}

int falcon_phy_power_get(const phymod_phy_access_t* phy, phymod_phy_power_t* power)
{
    power_status_t pwrdn;
    PHYMOD_IF_ERR_RETURN(falcon_tsc_pwrdn_get(&phy->access, &pwrdn));
    power->rx = (pwrdn.rx_s_pwrdn == 0)? phymodPowerOn: phymodPowerOff;
    power->tx = (pwrdn.tx_s_pwrdn == 0)? phymodPowerOn: phymodPowerOff;
    return ERR_CODE_NONE;
}


int falcon_phy_interface_config_set(const phymod_phy_access_t* phy, uint32_t flags, const phymod_phy_inf_config_t* config)
{
#ifdef FIXME 
    phymod_tx_t tx_params;  
#endif
    uint32_t current_pll_div=0;
    /* uint32_t new_pll_div=0; */
    uint32_t pll_mode = 0;
    /* uint32_t osr_mode = 0; */

    phymod_phy_access_t pm_phy_copy;
    phymod_firmware_core_config_t firmware_core_config;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    /* sc_table_entry exp_entry; RAVI */
    /* phymod_firmware_lane_config_t firmware_lane_config; */

    pm_phy_copy.access.lane = 0x1;
    PHYMOD_IF_ERR_RETURN
        (falcon_phy_firmware_core_config_get(&pm_phy_copy, &firmware_core_config));
    /* PHYMOD_IF_ERR_RETURN
        (tsce_phy_firmware_lane_config_get(phy, &firmware_lane_config)); */

    /*make sure that an and config from pcs is off*/
    firmware_core_config.CoreConfigFromPCS = 0;
    /* firmware_lane_config.AnEnabled = 0;
    firmware_lane_config.LaneConfigFromPCS = 0; */

    /* PHYMOD_IF_ERR_RETURN
        (temod_update_port_mode(&phy->access, (int *) &flags)); */

     switch (config->data_rate) {
         case 1000:
             pll_mode = 4;
             /* osr_mode = 8; */
             break;
             /* For 1 gig with 25G vco use
             pll_mode = 7;
             osr_mode = 12;
             break; */
         case 1060:
             pll_mode = 5;
             /* osr_mode = 8; */
             break;
             /* For 1 gig with 25G vco use
             pll_mode = 10;
             osr_mode = 12;
             break; */
         case 10000:
             pll_mode = 4;
             /* osr_mode = 2; */
             break;
         case 10600:
             pll_mode = 5;
             /* osr_mode = 2; */
             break;
         case 12500:
             pll_mode = 7;
             /* osr_mode = 2; */
             break;
         case 13000:
             pll_mode = 10;
             /* osr_mode = 2; */
             break;
         case 20000:
             pll_mode = 4;
             /* osr_mode = 1; */
             break;
         case 25000:
             pll_mode = 7;
             /* osr_mode = 1; */
             break;
         case 26000:
             pll_mode = 10;
             /* osr_mode = 1; */
             break;
     }

    PHYMOD_IF_ERR_RETURN
        (falcon_pll_mode_get(&phy->access, &current_pll_div));

    /* PHYMOD_IF_ERR_RETURN
        (temod_plldiv_lkup_get(&phy->access, spd_intf, &new_pll_div)); */

    
    /* PHYMOD_IF_ERR_RETURN
        (temod_pmd_osmode_set(&phy->access, spd_intf, 0)); */

    /*if pll change is enabled*/
    if((current_pll_div != pll_mode) && (PHYMOD_IF_FLAGS_DONT_TURN_OFF_PLL & flags)){
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, 
                               (_PHYMOD_MSG("pll has to change for speed_set from %u to %u but DONT_TURN_OFF_PLL flag is enabled"),
                                 current_pll_div, pll_mode));
    }
    /*pll switch is required and expected */
    if((current_pll_div != pll_mode) && !(PHYMOD_IF_FLAGS_DONT_TURN_OFF_PLL & flags)) {
        uint32_t pll_multiplier;
        /*
        phymod_access_t tmp_phy_access;
        int lane_num = 0;
        tmp_phy_access = phy->access;
        switch (tmp_phy_access.lane) {
            case 0xf:
                lane_num = 0;
                break;
            case 0x1:
            case 0x3:
                lane_num = 0;
                break;
            case 0x2:
                lane_num = 1;
                break;
            case 0x4:
            case 0xc:
                lane_num = 2;
                break;
            case 0x8:
                lane_num = 3;
                break;
            default:
                break;
        }
        */
        PHYMOD_IF_ERR_RETURN
            (falcon_core_soft_reset_release(&pm_phy_copy.access, 0));

        /*release the uc reset */
        PHYMOD_IF_ERR_RETURN
            (falcon_tsc_uc_reset(&pm_phy_copy.access ,1)); 

        /*set the PLL divider */
        PHYMOD_IF_ERR_RETURN
            (falcon_pll_mode_set(&pm_phy_copy.access, pll_mode));
        PHYMOD_IF_ERR_RETURN
            (_phymod_pll_multiplier_get(pll_mode, &pll_multiplier));
        /* update the VCO rate properly */
        /*
        switch (config->ref_clock) {
            case phymodRefClk156Mhz:
                vco_rate = pll_multiplier * 156 + pll_multiplier * 25 / 100;
                break;
            case phymodRefClk125Mhz:
                vco_rate = pll_multiplier * 125;
                break;
            default:
                vco_rate = pll_multiplier * 156 + pll_multiplier * 25 / 100;
                break;
        }
        */
        firmware_core_config.VcoRate = pll_mode;

        /*change the  master port num to the current caller port */
        
        /* PHYMOD_IF_ERR_RETURN
            (temod_master_port_num_set(&pm_phy_copy.access, lane_num)); */

        /* PHYMOD_IF_ERR_RETURN
            (temod_pll_reset_enable_set(&pm_phy_copy.access, 1)); */

        PHYMOD_IF_ERR_RETURN
            (falcon_core_soft_reset_release(&pm_phy_copy.access, 1));
    }

    /* PHYMOD_IF_ERR_RETURN
        (temod_set_spd_intf(&phy->access, spd_intf)); */

    /*change TX parameters if enabled*/
    /*if((PHYMOD_IF_FLAGS_DONT_OVERIDE_TX_PARAMS & flags) == 0) */ {
    /*    PHYMOD_IF_ERR_RETURN
            (tsce_phy_media_type_tx_get(phy, phymodMediaTypeMid, &tx_params)); */

#ifdef FIXME 
/*
 * We are calling this function with all params=0
*/
        PHYMOD_IF_ERR_RETURN
            (phymod_phy_tx_set(phy, &tx_params));
#endif
    }
    /*update the firmware config properly*/
    PHYMOD_IF_ERR_RETURN
        (falcon_phy_firmware_core_config_set(&pm_phy_copy, firmware_core_config));
    /* PHYMOD_IF_ERR_RETURN
         (tsce_phy_firmware_lane_config_set(phy, firmware_lane_config)); */
    return ERR_CODE_NONE;
}

/*flags- unused parameter*/
int falcon_phy_interface_config_get(const phymod_phy_access_t* phy, uint32_t flags, phymod_phy_inf_config_t* config)
{
    int vco_rate;
    int osr_mode;
    uint32_t pll_div;
    uint32_t pll_multiplier;

    PHYMOD_IF_ERR_RETURN
        (falcon_osr_mode_get(&phy->access, &osr_mode));
    PHYMOD_IF_ERR_RETURN
        (falcon_pll_mode_get(&phy->access, &pll_div));
    PHYMOD_IF_ERR_RETURN
        (_phymod_pll_multiplier_get(pll_div, &pll_multiplier));
        /* update the VCO rate properly */
        switch (config->ref_clock) {
            case phymodRefClk156Mhz:
                vco_rate = pll_multiplier * 156 + pll_multiplier * 25 / 100;
                break;
            case phymodRefClk125Mhz:
                vco_rate = pll_multiplier * 125;
                break;
            default:
                vco_rate = pll_multiplier * 156 + pll_multiplier * 25 / 100;
                break;
        }
    PHYMOD_IF_ERR_RETURN
        (_phymod_speed_id_interface_config_get(phy, vco_rate, osr_mode, config)); 
    return ERR_CODE_NONE;
}


int falcon_phy_cl72_set(const phymod_phy_access_t* phy, uint32_t cl72_en)
{
        
        
    
    /* Place your code here */

        
    return ERR_CODE_NONE;
        
    
}

int falcon_phy_cl72_get(const phymod_phy_access_t* phy, uint32_t* cl72_en)
{
        
        
    
    /* Place your code here */

        
    return ERR_CODE_NONE;
        
    
}


int falcon_phy_cl72_status_get(const phymod_phy_access_t* phy, phymod_cl72_status_t* status)
{
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_display_cl93n72_status(&phy->access));
    /*once the PMD function updated, will update the value properly*/
    status->locked = 0;
    return ERR_CODE_NONE;
}

int falcon_phy_loopback_set(const phymod_phy_access_t* phy, phymod_loopback_mode_t loopback, uint32_t enable)
{
    int start_lane, num_lane;
    int rv = ERR_CODE_NONE;

    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    switch (loopback) {
    case phymodLoopbackGlobal :
        /* PHYMOD_IF_ERR_RETURN(temod_tx_loopback_control(&phy->access, enable, start_lane, num_lane)); */
        break;
    case phymodLoopbackGlobalPMD :
        PHYMOD_IF_ERR_RETURN(falcon_tsc_dig_lpbk(&phy->access, (uint8_t) enable));
        PHYMOD_IF_ERR_RETURN(falcon_pmd_force_signal_detect(&phy->access, (int) enable));
        break;
    case phymodLoopbackRemotePMD :
        PHYMOD_IF_ERR_RETURN(falcon_tsc_rmt_lpbk(&phy->access, (uint8_t)enable));
        break;
    case phymodLoopbackRemotePCS :
        /* PHYMOD_IF_ERR_RETURN(temod_rx_loopback_control(&phy->access, enable, enable, enable)); */
        break;
    default :
        break;
    }             
    return rv;
}

int falcon_phy_loopback_get(const phymod_phy_access_t* phy, phymod_loopback_mode_t loopback, uint32_t* enable)
{
    int start_lane, num_lane;

    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    switch (loopback) {
    case phymodLoopbackGlobal :
        /* PHYMOD_IF_ERR_RETURN(temod_tx_loopback_get(&phy->access, &enable_core)); */
        /* *enable = (enable_core >> start_lane) & 0x1; */
        break;
    case phymodLoopbackGlobalPMD :
        PHYMOD_IF_ERR_RETURN(falcon_tsc_dig_lpbk_get(&phy->access, enable));
        break;
    case phymodLoopbackRemotePMD :
        PHYMOD_IF_ERR_RETURN(falcon_tsc_rmt_lpbk_get(&phy->access, enable));
        break;
    case phymodLoopbackRemotePCS :
        /* PHYMOD_IF_ERR_RETURN(temod_rx_loopback_control(&phy->access, enable, enable, enable)); */ /* RAVI */
        break;
    default :
        break;
    }             
    return ERR_CODE_NONE;
}


#if 0  
int falcon_phy_prbs_config_set(const phymod_access_t *pa, tx_rx_t dir, enum falcon_tsc_prbs_polynomial_enum prbs_poly_mode,
                               enum falcon_tsc_prbs_checker_mode_enum prbs_checker_mode, uint8_t prbs_inv)
{
  if(dir == TX) {
    PHYMOD_IF_ERR_RETURN
      (falcon_tsc_config_tx_prbs( pa, prbs_poly_mode, prbs_inv));
  } else {
    PHYMOD_IF_ERR_RETURN
      (falcon_tsc_config_rx_prbs( pa, prbs_poly_mode, prbs_checker_mode, prbs_inv));
  }
  return ERR_CODE_NONE;
}


int falcon_phy_prbs_config_get(const phymod_access_t *pa, tx_rx_t dir, enum falcon_tsc_prbs_polynomial_enum *prbs_poly_mode,
                               enum falcon_tsc_prbs_checker_mode_enum *prbs_checker_mode, uint8_t *prbs_inv)
{
  if(dir == TX) {
    PHYMOD_IF_ERR_RETURN
      (falcon_tsc_get_tx_prbs_config( pa, prbs_poly_mode, prbs_inv));
  } else {
    PHYMOD_IF_ERR_RETURN
      (falcon_tsc_get_rx_prbs_config( pa, prbs_poly_mode, prbs_checker_mode, prbs_inv));
  }
  return ERR_CODE_NONE;
}

int falcon_phy_prbs_enable_set(const phymod_access_t *pa, tx_rx_t dir, uint8_t enable)
{
  if(dir == TX) {
    PHYMOD_IF_ERR_RETURN
      (falcon_tsc_tx_prbs_en( pa, enable));
  } else {
    PHYMOD_IF_ERR_RETURN
      (falcon_tsc_rx_prbs_en( pa, enable));
  }
  return ERR_CODE_NONE;
}

int falcon_phy_prbs_enable_get(const phymod_access_t *pa, tx_rx_t dir, uint8_t *enable)
{
  if(dir == TX) {
    PHYMOD_IF_ERR_RETURN
      (falcon_tsc_get_tx_prbs_en( pa, enable));
  } else {
    PHYMOD_IF_ERR_RETURN
      (falcon_tsc_get_rx_prbs_en( pa, enable));
  }
  return ERR_CODE_NONE;
}

int falcon_phy_prbs_status_get(const phymod_access_t *pa, uint8_t *chk_lock, uint32_t *prbs_err_cnt, uint8_t *lock_lost)
{
  PHYMOD_IF_ERR_RETURN
    (falcon_tsc_prbs_chk_lock_state( pa, chk_lock));
  PHYMOD_IF_ERR_RETURN
    (falcon_tsc_prbs_err_count_state( pa, prbs_err_cnt, lock_lost));
  return ERR_CODE_NONE;
}
#endif


int falcon_core_init(const phymod_core_access_t* core, const phymod_core_init_config_t* init_config, const phymod_core_status_t* core_status)
{
    phymod_phy_access_t phy_access, phy_access_copy;
    phymod_core_access_t  core_copy;
    phymod_firmware_core_config_t  firmware_core_config_tmp;

    TSCE_CORE_TO_PHY_ACCESS(&phy_access, core);
    phy_access_copy = phy_access;
    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));
    core_copy.access.lane = 0x1;
    phy_access_copy = phy_access;
    phy_access_copy.access = core->access;
    phy_access_copy.access.lane = 0x1;
    phy_access_copy.type = core->type;

    /* PHYMOD_IF_ERR_RETURN
        (temod_pmd_reset_seq(&core_copy.access, core_status->pmd_active)); */
    /*need to set the heart beat default is for 156.25M*/
    if (init_config->interface.ref_clock == phymodRefClk125Mhz) {
        /*we need to set the ref clock config differently */
    }
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_ucode_mdio_load( &core->access, tscf_ucode, tscf_ucode_len));
    /*next we need to check if the load is correct or not */
    
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_ucode_load_verify(&core_copy.access, (uint8_t *) &tscf_ucode, tscf_ucode_len));
    /*next we need to set the uc active and release uc */
    PHYMOD_IF_ERR_RETURN
        (falcon_uc_active_set(&core_copy.access ,1));
    /*release the uc reset */
    PHYMOD_IF_ERR_RETURN
        (falcon_tsc_uc_reset(&core_copy.access ,1));
    /* we need to wait at least 10ms for the uc to settle */
    PHYMOD_USLEEP(10000);

    /* plldiv CONFIG */
    PHYMOD_IF_ERR_RETURN
        (falcon_pll_mode_set(&core->access, 0xa));

    /*now config the lane mapping and polarity */
    PHYMOD_IF_ERR_RETURN
        (falcon_core_lane_map_set(core, &init_config->lane_map));
    /* PHYMOD_IF_ERR_RETURN
        (temod_autoneg_set_init(&core->access, &an));
    PHYMOD_IF_ERR_RETURN
        (temod_autoneg_timer_init(&core->access));
    PHYMOD_IF_ERR_RETURN
        (temod_master_port_num_set(&core->access, 0)); */
    /*don't overide the fw that set in config set if not specified*/
    firmware_core_config_tmp = init_config->firmware_core_config;
    firmware_core_config_tmp.CoreConfigFromPCS = 0;
    /*set the vco rate to be default at 10.3125G */
    firmware_core_config_tmp.VcoRate = 0x13;

    PHYMOD_IF_ERR_RETURN
        (falcon_phy_firmware_core_config_set(&phy_access_copy, firmware_core_config_tmp));

    /* release core soft reset */
    PHYMOD_IF_ERR_RETURN
        (falcon_core_soft_reset_release(&core_copy.access, 1));

    return PHYMOD_E_NONE;
}

int falcon_phy_init(const phymod_phy_access_t* phy, const phymod_phy_init_config_t* init_config)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int falcon_phy_rx_pmd_locked_get(const phymod_phy_access_t* phy, uint32_t* rx_seq_done)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int falcon_phy_link_status_get(const phymod_phy_access_t* phy, uint32_t* link_status)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int falcon_phy_reg_read(const phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t* val)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int falcon_phy_reg_write(const phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t val)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int falcon_core_wait_event(const phymod_core_access_t* core, phymod_core_event_t event, uint32_t timeout)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


#endif /* PHYMOD_FALCON_SUPPORT */
