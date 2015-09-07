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
#include <phymod/phymod_dispatch.h>

#ifdef PHYMOD_EAGLE_SUPPORT

int eagle_core_identify(const phymod_core_access_t* core, uint32_t core_id, uint32_t* is_identified)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_core_info_get(const phymod_core_access_t* core, phymod_core_info_t* info)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_core_lane_map_set(const phymod_core_access_t* core, const phymod_lane_map_t* lane_map)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_core_lane_map_get(const phymod_core_access_t* core, phymod_lane_map_t* lane_map)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_core_reset_set(const phymod_core_access_t* core, phymod_reset_mode_t reset_mode, phymod_reset_direction_t direction)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_core_reset_get(const phymod_core_access_t* core, phymod_reset_mode_t reset_mode, phymod_reset_direction_t* direction)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_core_firmware_info_get(const phymod_core_access_t* core, phymod_core_firmware_info_t* fw_info)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_firmware_config_set(const phymod_phy_access_t* phy, phymod_firmware_lane_config_t fw_mode)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_firmware_config_get(const phymod_phy_access_t* phy, phymod_firmware_lane_config_t* fw_mode)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_core_pll_sequencer_restart(const phymod_core_access_t* core, uint32_t flags, phymod_sequencer_operation_t operation)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_core_wait_event(const phymod_core_access_t* core, phymod_core_event_t event, uint32_t timeout)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_rx_restart(const phymod_phy_access_t* phy)
{
    return PHYMOD_E_NONE;
}


int eagle_phy_polarity_set(const phymod_phy_access_t* phy, const phymod_polarity_t* polarity)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_polarity_get(const phymod_phy_access_t* phy, phymod_polarity_t* polarity)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_tx_set(const phymod_phy_access_t* phy, const phymod_tx_t* tx)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_tx_get(const phymod_phy_access_t* phy, phymod_tx_t* tx)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_media_type_tx_get(const phymod_phy_access_t* phy, phymod_media_typed_t media, phymod_tx_t* tx)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_tx_override_set(const phymod_phy_access_t* phy, const phymod_tx_override_t* tx_override)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_tx_override_get(const phymod_phy_access_t* phy, phymod_tx_override_t* tx_override)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_rx_set(const phymod_phy_access_t* phy, const phymod_rx_t* rx)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_rx_get(const phymod_phy_access_t* phy, phymod_rx_t* rx)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_reset_set(const phymod_phy_access_t* phy, const phymod_phy_reset_t* reset)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_reset_get(const phymod_phy_access_t* phy, phymod_phy_reset_t* reset)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_power_set(const phymod_phy_access_t* phy, const phymod_phy_power_t* power)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_power_get(const phymod_phy_access_t* phy, phymod_phy_power_t* power)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_tx_disable_set(const phymod_phy_access_t* phy, uint32_t tx_disable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_tx_disable_get(const phymod_phy_access_t* phy, uint32_t* tx_disable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_tx_lane_control_set(const phymod_phy_access_t* phy, uint32_t enable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_tx_lane_control_get(const phymod_phy_access_t* phy, uint32_t* enable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

/*Rx control*/
int eagle_phy_rx_lane_control_set(const phymod_phy_access_t* phy, uint32_t enable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_rx_lane_control_get(const phymod_phy_access_t* phy, uint32_t* enable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_interface_config_set(const phymod_phy_access_t* phy, uint32_t flags, const phymod_phy_inf_config_t* config)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_interface_config_get(const phymod_phy_access_t* phy, uint32_t flags, phymod_phy_inf_config_t* config)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_cl72_set(const phymod_phy_access_t* phy, uint32_t cl72_en)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_cl72_get(const phymod_phy_access_t* phy, uint32_t* cl72_en)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_cl72_status_get(const phymod_phy_access_t* phy, phymod_cl72_status_t* status)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_autoneg_ability_set(const phymod_phy_access_t* phy, const phymod_autoneg_ability_t* an_ability_set_type)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_autoneg_ability_get(const phymod_phy_access_t* phy, phymod_autoneg_ability_t* an_ability_get_type)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_autoneg_set(const phymod_phy_access_t* phy, const phymod_autoneg_control_t* an)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_autoneg_get(const phymod_phy_access_t* phy, phymod_autoneg_control_t* an)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_autoneg_status_get(const phymod_phy_access_t* phy, phymod_autoneg_status_t* status)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_core_init(const phymod_core_access_t* core, const phymod_core_init_config_t* init_config, const phymod_core_status_t* core_status)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_init(const phymod_phy_access_t* phy, const phymod_phy_init_config_t* init_config)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_loopback_set(const phymod_phy_access_t* phy, phymod_loopback_mode_t loopback, uint32_t enable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}

int eagle_phy_loopback_get(const phymod_phy_access_t* phy, phymod_loopback_mode_t loopback, uint32_t* enable)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_rx_pmd_locked_get(const phymod_phy_access_t* phy, uint32_t* rx_seq_done)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_link_status_get(const phymod_phy_access_t* phy, uint32_t* link_status)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_reg_read(const phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t* val)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


int eagle_phy_reg_write(const phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t val)
{
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
    
}


#endif /* PHYMOD_EAGLE_SUPPORT */
