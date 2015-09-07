/*
 *         
 * $Id: viper.c,v 1.2.2.26 Broadcom SDK $
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
#include <phymod/chip/bcmi_viper_xgxs_defs.h>

#include "../../viper/tier1/viper_inc.h"

#define VQMOD_ID0       0x600d
#define VQMOD_ID1       0x8770
#define VQMOD_MODEL     0x01

#define VXMOD_ID0       0x600d
#define VXMOD_ID1       0x8770
#define VXMOD_MODEL     0x01

typedef enum viper_core_version_e {
    vxmodCoreVersionA0,
    vqmodCoreVersionA0,
} viper_core_version_t;

#define VIPER_NOF_DFES (5)
#define VIPER_NOF_LANES_IN_CORE (4)
#define VIPER_LANE_SWAP_LANE_MASK (0x3)
#define VIPER_PHY_ALL_LANES (0xf)


int viper_core_identify(const phymod_core_access_t* core, uint32_t core_id, uint32_t* is_identified)
{
/*    int ioerr;    
    return ioerr ? PHYMOD_E_IO : PHYMOD_E_NONE;    */
    return 0;
}

int viper_core_info_get(const phymod_core_access_t* phy, phymod_core_info_t* info)
{
    uint32_t serdes_id;

    PHYMOD_IF_ERR_RETURN(viper_revid_read(&phy->access, &serdes_id));

    info->serdes_id = serdes_id;
    if ((serdes_id & 0x3f) == VXMOD_MODEL) {
        info->core_version = vxmodCoreVersionA0;
    } else {
        info->core_version = vqmodCoreVersionA0;
    }

    return PHYMOD_E_NONE;
}

/*
 * set lane swapping for core
 * The tx swap is composed of PCS swap and after that the PMD swap.
 * The rx swap is composed just by PCS swap
 *
 * lane_map_tx and lane_map_rx[lane=logic_lane] are logic-lane base.
 * pcs_swap and register is logic_lane base.
 * but pmd_tx_map and addr_index_swap (and registers) are physical lane base
 */

int viper_core_lane_map_set(const phymod_core_access_t* core, const phymod_lane_map_t* lane_map)
{
    /*
     * 0x8169, 0x816B Do Analog to Digital Lanemap.
    viper_tx_lane_swap(&core->access, lane_map->num_of_lanes, lane_map->lane_map_tx);
    viper_tx_lane_swap(&core->access, lane_map->num_of_lanes, lane_map->lane_map_rx);
     */

    return PHYMOD_E_NONE;
}

int viper_core_lane_map_get(const phymod_core_access_t* core, phymod_lane_map_t* lane_map)
{
    /*
     * 0x8169, 0x816B Do Analog to Digital Lanemap.
     */
    return PHYMOD_E_NONE;
}

/* reset pll sequencer
 * flags - unused parameter
 */
int viper_core_pll_sequencer_restart (const phymod_core_access_t  *core, 
                                      uint32_t                     flags, 
                                      phymod_sequencer_operation_t operation)
{
    /*
     * 0x8000:  Start Sequencer - use this bit
     */
    switch (operation) {
        case phymodSeqOpStop:
             break;

        case phymodSeqOpStart:
             break;

        case phymodSeqOpRestart:
             break;

        default:
            return PHYMOD_E_UNAVAIL;
        break;
    }
    return PHYMOD_E_NONE;
}

int viper_core_wait_event (const phymod_core_access_t* core, 
                           phymod_core_event_t         event, 
                           uint32_t                    timeout)
{
    switch(event)
    {
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("illegal wait event %u"), event));
    }
    return PHYMOD_E_NONE;
}


int viper_phy_rx_restart(const phymod_phy_access_t* phy)
{
    /*
     * HELP: 0xd019:  rx_restart_pmd - no such register in Viper
     */
    return PHYMOD_E_NONE;
}

int viper_phy_polarity_set(const phymod_phy_access_t* phy, const phymod_polarity_t* polarity)
{
    int rv;

    rv = viper_tx_pol_set(&phy->access, polarity->tx_polarity);
    if (rv != PHYMOD_E_NONE) return (rv);

    rv = viper_rx_pol_set(&phy->access, polarity->rx_polarity);
    return rv;
}

int viper_phy_polarity_get(const phymod_phy_access_t* phy, phymod_polarity_t* polarity)
{
    int rv;

    rv = viper_tx_pol_get(&phy->access, &polarity->tx_polarity);
    if (rv != PHYMOD_E_NONE) return (rv);

    rv = viper_rx_pol_get(&phy->access, &polarity->rx_polarity);
    return rv;
}

int viper_phy_tx_set(const phymod_phy_access_t* phy, const phymod_tx_t* tx)
{
    /*
     * 0xD110: txfir_post_override, txfir_pre_override
     * Ask John if they have post, pre and main taps
     */
    return PHYMOD_E_NONE;
}

int viper_phy_tx_get(const phymod_phy_access_t* phy, phymod_tx_t* tx)
{
    /*
     * HELP:  No equivalent register in Viper.
     * 0xD110: txfir_post_override, txfir_pre_override
     */
    return PHYMOD_E_NONE;
}

int viper_phy_tx_override_set(const phymod_phy_access_t  *phy, 
                              const phymod_tx_override_t *tx_override)
{
    PHYMOD_IF_ERR_RETURN
        (viper_tsc_tx_pi_freq_override(&phy->access,
                                    tx_override->phase_interpolator.enable,
                                    tx_override->phase_interpolator.value));
    return PHYMOD_E_NONE;
}

int viper_phy_rx_set(const phymod_phy_access_t* phy, const phymod_rx_t* rx)
{
    /* Ask John if we have DFE */ 
    return PHYMOD_E_NONE;
}

int viper_phy_rx_get(const phymod_phy_access_t* phy, phymod_rx_t* rx)
{
    /* Ask John if we have DFE */ 
    return PHYMOD_E_NONE;
}


int viper_phy_power_set(const phymod_phy_access_t* phy, const phymod_phy_power_t* power)
{
    /*
     * Program 0x8018 here.
     * Port Enable Set
     * Tx Disable Set
     * Rx Squelch set
     */
    return PHYMOD_E_NONE;
} 

int viper_phy_power_get(const phymod_phy_access_t* phy, phymod_phy_power_t* power)
{
    /*
     * Program 0x8018 here.
     * Port Enable Set
     * Tx Disable Set
     * Rx Squelch set
     */
    return PHYMOD_E_NONE;
}

int viper_phy_tx_lane_control_set(const phymod_phy_access_t* phy, uint32_t enable)
{
    /*
     * Need to check what to do here 
     */
    return PHYMOD_E_NONE;
}

int viper_phy_fec_enable_set(const phymod_phy_access_t* phy, uint32_t enable)
{
    /*
     * check with john if there ifs FEC
     */
    return PHYMOD_E_NONE;
}

int viper_phy_fec_enable_get(const phymod_phy_access_t* phy, uint32_t* enable)
{
    /*
     * check with john if there ifs FEC
     */
    return PHYMOD_E_NONE;
}

int viper_phy_interface_config_set (const phymod_phy_access_t*     phy, 
                                    uint32_t                       flags, 
                                    const phymod_phy_inf_config_t* config)
{
    return PHYMOD_E_NONE;
}

int viper_phy_interface_config_get (const phymod_phy_access_t *phy, 
                                    uint32_t                   flags, 
                                    phymod_phy_inf_config_t   *config)
{
    return PHYMOD_E_NONE;
}

int viper_phy_cl72_set(const phymod_phy_access_t* phy, uint32_t cl72_en)
{
    /* HELP: NO EQUIVALENT REGISTER */
    return PHYMOD_E_NONE;
}

int viper_phy_cl72_get (const phymod_phy_access_t* phy, uint32_t* cl72_en)
{
    /* HELP: NO EQUIVALENT REGISTER */
    return PHYMOD_E_NONE;
}

int viper_phy_cl72_status_get (const phymod_phy_access_t *phy, 
                               phymod_cl72_status_t      *status)
{
    /* HELP: NO EQUIVALENT REGISTER */
    return PHYMOD_E_NONE;
}

int viper_phy_autoneg_ability_set (const phymod_phy_access_t      *phy, 
                                   const phymod_autoneg_ability_t *an_ability)
{
    /* Fiber nothing.
     * SGMII Master or SLAVE, and what is the speed
     */ 
    return PHYMOD_E_NONE;
}

int viper_phy_autoneg_ability_get (const phymod_phy_access_t *phy, 
                                   phymod_autoneg_ability_t  *an_ability_get_type)
{

    return PHYMOD_E_NONE;
}

int viper_phy_autoneg_remote_ability_get (const phymod_phy_access_t* phy, 
                                          phymod_autoneg_ability_t* an_ability_get_type) 
{
    return PHYMOD_E_NONE;
}

int viper_phy_autoneg_set (const phymod_phy_access_t      *phy, 
                           const phymod_autoneg_control_t *an)
{
    /* depending on the media = iuse one of the autoneg routines*/
    return PHYMOD_E_NONE;
}

int viper_phy_autoneg_get(const phymod_phy_access_t *phy, 
                          phymod_autoneg_control_t  *an, 
                          uint32_t                  *an_done)
{
    return (viper_autoneg_get(&phy->access, an, an_done));
}

int viper_phy_autoneg_status_get(const phymod_phy_access_t *phy,
                                 phymod_autoneg_status_t   *status)
{
    return (viper_autoneg_status_get(&phy->access, status));
}   

int viper_core_init (const phymod_core_access_t      *core,
                     const phymod_core_init_config_t *init_config,
                     const phymod_core_status_t      *core_status)
{
    /* HELP */
    return PHYMOD_E_NONE;
}   

int viper_phy_init (const phymod_phy_access_t      * phy,
                    const phymod_phy_init_config_t *init_config)
{
    /* HELP */
    return PHYMOD_E_NONE;
}


int viper_phy_loopback_set (const phymod_phy_access_t *phy, 
                            phymod_loopback_mode_t loopback, 
                            uint32_t enable)
{
    int start_lane, num_lane, i;
    phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    for (i = 0; i < num_lane; i++) {
        phy_copy.access.lane = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN(viper_global_loopback_set(&phy_copy.access, (uint8_t) enable));
    }

    return (viper_global_loopback_set(&phy->access, enable));
}

int viper_phy_loopback_get (const phymod_phy_access_t *phy, 
                            phymod_loopback_mode_t loopback, 
                            uint32_t *enable)
{
    return (viper_global_loopback_get(&phy->access, enable));
}

int viper_phy_rx_pmd_locked_get (const phymod_phy_access_t* phy, 
                                 uint32_t* rx_seq_done)
{
    return(viper_pmd_lock_get(&phy->access, rx_seq_done));
}

int viper_phy_link_status_get (const phymod_phy_access_t* phy, 
                               uint32_t *link_status)
{
    return(viper_get_link_status(&phy->access, link_status));
}

int viper_phy_reg_read (const phymod_phy_access_t* phy, 
                        uint32_t reg_addr, uint32_t *val)
{
    return(phymod_tsc_iblk_read(&phy->access, reg_addr, val));
}


int viper_phy_reg_write (const phymod_phy_access_t* phy, 
                         uint32_t reg_addr, uint32_t val)
{
    return (phymod_tsc_iblk_write(&phy->access, reg_addr, val));
}


#ifdef _MICROCODE_RELATED_FUNCTIONS_
int viper_core_firmware_info_get (const phymod_core_access_t* core, 
                                  phymod_core_firmware_info_t* fw_info)
{
    /*
     * this is for micro code., There is no micro code for viper.
     */
    return PHYMOD_E_NONE;
}

int viper_phy_firmware_core_config_set (const phymod_phy_access_t* phy, 
                                        phymod_firmware_core_config_t fw_config)
{
    /*
     * this is for micro code., There is no micro code for viper.
     */
    return PHYMOD_E_NONE;
}


int viper_phy_firmware_core_config_get (const phymod_phy_access_t* phy, 
                                        phymod_firmware_core_config_t* fw_config)
{
    /*
     * this is for micro code., There is no micro code for viper.
     */
    return PHYMOD_E_NONE;
}


int viper_phy_firmware_lane_config_set (const phymod_phy_access_t* phy, 
                                        phymod_firmware_lane_config_t fw_config)
{
    /*
     * this is for micro code., There is no micro code for viper.
     */
    return PHYMOD_E_NONE;
}

int viper_phy_firmware_lane_config_get (const phymod_phy_access_t* phy, 
                                        phymod_firmware_lane_config_t* fw_config)
{
    /*
     * this is for micro code., There is no micro code for viper.
     */
    return PHYMOD_E_NONE;
}
#endif   /* _MICROCODE_RELATED_FUNCTIONS_ */


#ifdef _NO_TSCE_FUNCTIONS_
int viper_core_reset_set (const phymod_core_access_t *core, 
                          phymod_reset_mode_t         reset_mode, 
                          phymod_reset_direction_t    direction)
{
    /* Place your code here : TSCE Does not have any code either. */
    return PHYMOD_E_NONE;
}

int viper_core_reset_get (const phymod_core_access_t  *core, 
                          phymod_reset_mode_t          reset_mode, 
                          phymod_reset_direction_t    *direction)
{   
    return PHYMOD_E_UNAVAIL;
}

int viper_phy_media_type_tx_get(const phymod_phy_access_t* phy, 
				phymod_media_typed_t media, phymod_tx_t* tx)
{
    return PHYMOD_E_NONE;
}

int viper_phy_tx_override_get(const phymod_phy_access_t* phy, phymod_tx_override_t* tx_override)
{
    return PHYMOD_E_NONE;
}

int viper_phy_reset_set(const phymod_phy_access_t* phy, const phymod_phy_reset_t* reset)
{
    return PHYMOD_E_NONE;
}

int viper_phy_reset_get(const phymod_phy_access_t* phy, phymod_phy_reset_t* reset)
{
    return PHYMOD_E_NONE;
}

int viper_phy_tx_lane_control_get(const phymod_phy_access_t* phy, uint32_t* enable)
{  
    return PHYMOD_E_NONE;
}

int viper_phy_rx_lane_control_set(const phymod_phy_access_t* phy, uint32_t enable)
{   
    return PHYMOD_E_NONE;
}

int viper_phy_rx_lane_control_get(const phymod_phy_access_t* phy, uint32_t* enable)
{  
    return PHYMOD_E_NONE;
}
#endif /* _NO_TSCE_FUNCTIONS */
