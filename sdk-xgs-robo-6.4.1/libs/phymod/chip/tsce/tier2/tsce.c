/*
 *         
 * $Id: tsce.c,v 1.2.2.26 Broadcom SDK $
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
#include <phymod/chip/bcmi_tsce_xgxs_defs.h>

#include "../../tsce/tier1/temod_enum_defines.h"
#include "../../tsce/tier1/temod.h"
#include "../../tsce/tier1/temod_device.h"
#include "../../tsce/tier1/temod_sc_lkup_table.h"
#include "../../tsce/tier1/tePCSRegEnums.h"
#include "../../eagle/tier1/eagle_cfg_seq.h"  
#include "../../eagle/tier1/eagle_tsc_common.h" 
#include "../../eagle/tier1/eagle_tsc_interface.h" 
#include "../../eagle/tier1/eagle_tsc_dependencies.h" 


#ifdef PHYMOD_TSCE_SUPPORT

#define TSCE_ID0        0x600d
#define TSCE_ID1        0x8770
#define TSCE_REV_MASK   0x0

#define TSCE4_MODEL     0x12
#define TSCE12_MODEL    0x13
#define EAGLE_MODEL     0x1a

#define TSCE_NOF_DFES (5)
#define TSCE_NOF_LANES_IN_CORE (4)
#define TSCE_LANE_SWAP_LANE_MASK (0x3)
#define TSCE_PHY_ALL_LANES (0xf)
#define TSCE_CORE_TO_PHY_ACCESS(_phy_access, _core_access) \
    do{\
        PHYMOD_MEMCPY(&(_phy_access)->access, &(_core_access)->access, sizeof((_phy_access)->access));\
        (_phy_access)->type = (_core_access)->type; \
        (_phy_access)->access.lane = TSCE_PHY_ALL_LANES; \
    }while(0)

#define TSCE_MAX_FIRMWARES (5)

#define TSCE_USERSPEED_SELECT(_phy, _config, _type)  \
    ((_config)->mode==phymodPcsUserSpeedModeST) ?                       \
      temod_st_control_field_set(&(_phy)->access, (_config)->current_entry, _type, (_config)->value) : \
        temod_override_set(&(_phy)->access, _type, (_config)->value) 

#define TSCE_USERSPEED_CREDIT_SELECT(_phy, _config, _type)  \
    ((_config)->mode==phymodPcsUserSpeedModeST) ?                       \
      temod_st_credit_field_set(&(_phy)->access, (_config)->current_entry, _type, (_config)->value) : \
        temod_credit_override_set(&(_phy)->access, _type, (_config)->value) 

/* uController's firmware */
extern unsigned char tsce_ucode[];
extern unsigned short tsce_ucode_ver;
extern unsigned short tsce_ucode_crc;
extern unsigned short tsce_ucode_len;

typedef int (*sequncer_control_f)(const phymod_access_t* core, uint32_t enable);
typedef int (*rx_DFE_tap_control_set_f)(const phymod_access_t* phy, uint32_t val);


int tsce_core_identify(const phymod_core_access_t* core, uint32_t core_id, uint32_t* is_identified)
{
    int ioerr = 0;
    const phymod_access_t *pm_acc = &core->access;
    PHYID2r_t id2;
    PHYID3r_t id3;
    MAIN0_SERDESIDr_t serdesid;
    /* DIG_REVID0r_t revid; */
    uint32_t model;

    *is_identified = 0;

    if(core_id == 0){
        ioerr += READ_PHYID2r(pm_acc, &id2);
        ioerr += READ_PHYID3r(pm_acc, &id3);
    }
    else{
        PHYID2r_SET(id2, ((core_id >> 16) & 0xffff));
        PHYID3r_SET(id3, core_id & 0xffff);
    }

    if (PHYID2r_GET(id2) == TSCE_ID0 &&
        (PHYID3r_GET(id3) &= ~TSCE_REV_MASK) == TSCE_ID1) {
        /* PHY IDs match - now check PCS model */
        ioerr += READ_MAIN0_SERDESIDr(pm_acc, &serdesid);
        model = MAIN0_SERDESIDr_MODEL_NUMBERf_GET(serdesid);
        if (model == TSCE4_MODEL || model == TSCE12_MODEL)  {
            /* PCS model matches - now check PMD model */
            /* for now bypass the pmd register rev check */
            /* ioerr += READ_DIG_REVID0r(pm_acc, &revid);
            if (DIG_REVID0r_REVID_MODELf_GET(revid) == EAGLE_MODEL) */ {
                *is_identified = 1;
            }
        }
    }
        
    return ioerr ? PHYMOD_E_IO : PHYMOD_E_NONE;    
}


int tsce_core_info_get(const phymod_core_access_t* phy, phymod_core_info_t* info)
{
    uint32_t serdes_id;
    PHYMOD_IF_ERR_RETURN
        (temod_revid_read(&phy->access, &serdes_id));
    info->serdes_id = serdes_id;
    if ((serdes_id & 0x3f) == TSCE4_MODEL) {
        info->core_version = phymodCoreVersionTsce4A0;
    } else {
        info->core_version = phymodCoreVersionTsce12A0;
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
 
int tsce_core_lane_map_set(const phymod_core_access_t* core, const phymod_lane_map_t* lane_map)
{
    uint32_t pcs_swap = 0 , pmd_swap = 0, lane;
    uint32_t addr_index_swap = 0, pmd_tx_map =0;

    if(lane_map->num_of_lanes != TSCE_NOF_LANES_IN_CORE){
        return PHYMOD_E_CONFIG;
    }
    for( lane = 0 ; lane < TSCE_NOF_LANES_IN_CORE ; lane++){
        if(lane_map->lane_map_rx[lane] >= TSCE_NOF_LANES_IN_CORE){
            return PHYMOD_E_CONFIG;
        }
        /* encode each lane as four bits */
        /* pcs_map[lane] = rx_map[lane] */
        pcs_swap += lane_map->lane_map_rx[lane]<<(lane*4);
    }

    for( lane = 0 ; lane < TSCE_NOF_LANES_IN_CORE ; lane++){
        if(lane_map->lane_map_tx[lane] >= TSCE_NOF_LANES_IN_CORE){
            return PHYMOD_E_CONFIG;
        }
        /* encode each lane as four bits. pmd_swap is logic base */
        /* considering the pcs lane swap: pmd_map[pcs_map[lane]] = tx_map[lane] */
        pmd_swap += lane_map->lane_map_tx[lane]<<(lane_map->lane_map_rx[lane]*4);
    }

    for( lane = 0 ; lane < TSCE_NOF_LANES_IN_CORE ; lane++){
        addr_index_swap |= (lane << 4*((pcs_swap >> lane*4) & 0xf)) ;
        pmd_tx_map      |= (lane << 4*((pmd_swap >> lane*4) & 0xf)) ;
    }

    PHYMOD_IF_ERR_RETURN(temod_pcs_lane_swap(&core->access, pcs_swap));

    PHYMOD_IF_ERR_RETURN
        (temod_pmd_addr_lane_swap(&core->access, addr_index_swap));
    PHYMOD_IF_ERR_RETURN
        (temod_pmd_lane_swap_tx(&core->access, pmd_tx_map));
    return PHYMOD_E_NONE;
}


int tsce_core_lane_map_get(const phymod_core_access_t* core, phymod_lane_map_t* lane_map)
{
    uint32_t pmd_swap = 0 , pcs_swap = 0, lane; 
    PHYMOD_IF_ERR_RETURN(temod_pcs_lane_swap_get(&core->access, &pcs_swap));
    PHYMOD_IF_ERR_RETURN(temod_pmd_lane_swap_tx_get(&core->access, &pmd_swap));
    for( lane = 0 ; lane < TSCE_NOF_LANES_IN_CORE ; lane++){
        /* deccode each lane from four bits */
        lane_map->lane_map_rx[lane] = (pcs_swap>>(lane*4)) & TSCE_LANE_SWAP_LANE_MASK;
        /* considering the pcs lane swap: tx_map[lane] = pmd_map[pcs_map[lane]] */
        lane_map->lane_map_tx[lane] = (pmd_swap>>(lane_map->lane_map_rx[lane]*4)) & TSCE_LANE_SWAP_LANE_MASK;
    }
    lane_map->num_of_lanes = TSCE_NOF_LANES_IN_CORE;   
    return PHYMOD_E_NONE;
}


int tsce_core_reset_set(const phymod_core_access_t* core, phymod_reset_mode_t reset_mode, phymod_reset_direction_t direction)
{
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
}


int tsce_core_reset_get(const phymod_core_access_t* core, phymod_reset_mode_t reset_mode, phymod_reset_direction_t* direction)
{       
    return PHYMOD_E_UNAVAIL;
}


int tsce_core_firmware_info_get(const phymod_core_access_t* core, phymod_core_firmware_info_t* fw_info)
{
    
	/* 
	 * It's O.K to use this code as is since the firmware CRC is already checked at 
	 * at the time we load it.
	 */
    fw_info->fw_crc = tsce_ucode_crc;
    fw_info->fw_version = tsce_ucode_ver;
    return PHYMOD_E_NONE;
}

/* load tsce fw. the fw_loader parameter is valid just for external fw load */
STATIC
int _tsce_core_firmware_load(const phymod_core_access_t* core, phymod_firmware_load_method_t load_method, phymod_firmware_loader_f fw_loader)
{
#ifndef NEW_PMD_UCODE
    phymod_core_firmware_info_t actual_fw;
#endif

    switch(load_method){
    case phymodFirmwareLoadMethodInternal:
        PHYMOD_IF_ERR_RETURN(eagle_tsc_ucode_mdio_load(&core->access, tsce_ucode, tsce_ucode_len));
        break;
    case phymodFirmwareLoadMethodExternal:
        PHYMOD_NULL_CHECK(fw_loader); 

        eagle_pram_flop_set(&core->access, 0x0);

        PHYMOD_IF_ERR_RETURN(eagle_tsc_ucode_init(&core->access));

        PHYMOD_IF_ERR_RETURN
            (temod_pram_abl_enable_set(&core->access,1));
        PHYMOD_IF_ERR_RETURN
            (eagle_pram_firmware_enable(&core->access, 1));

        PHYMOD_IF_ERR_RETURN(fw_loader(core, tsce_ucode_len, tsce_ucode)); 

        PHYMOD_IF_ERR_RETURN
            (eagle_pram_firmware_enable(&core->access, 0));
        PHYMOD_IF_ERR_RETURN
            (temod_pram_abl_enable_set(&core->access,0));
        break;
    case phymodFirmwareLoadMethodNone:
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("illegal fw load method %u"), load_method));
    }
#ifndef NEW_PMD_UCODE 
    if(load_method != phymodFirmwareLoadMethodNone){
        PHYMOD_IF_ERR_RETURN(tsce_core_firmware_info_get(core, &actual_fw));
        if((tsce_ucode_crc != actual_fw.fw_crc) || (tsce_ucode_ver != actual_fw.fw_version)){
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("fw load validation was failed")));
        }
    }
#endif
    return PHYMOD_E_NONE;
}

int tsce_phy_firmware_core_config_set(const phymod_phy_access_t* phy, phymod_firmware_core_config_t fw_config)
{
    struct eagle_tsc_uc_core_config_st serdes_firmware_core_config;
    PHYMOD_MEMSET(&serdes_firmware_core_config, 0, sizeof(serdes_firmware_core_config));
    serdes_firmware_core_config.field.core_cfg_from_pcs = fw_config.CoreConfigFromPCS;
	serdes_firmware_core_config.field.vco_rate = fw_config.VcoRate; 
    PHYMOD_IF_ERR_RETURN(eagle_tsc_set_uc_core_config(&phy->access, serdes_firmware_core_config));
    return PHYMOD_E_NONE;
}


int tsce_phy_firmware_core_config_get(const phymod_phy_access_t* phy, phymod_firmware_core_config_t* fw_config)
{
    struct eagle_tsc_uc_core_config_st serdes_firmware_core_config;
    PHYMOD_IF_ERR_RETURN(eagle_tsc_get_uc_core_config(&phy->access, &serdes_firmware_core_config));
    PHYMOD_MEMSET(fw_config, 0, sizeof(*fw_config));
    fw_config->CoreConfigFromPCS = serdes_firmware_core_config.field.core_cfg_from_pcs;
    fw_config->VcoRate = serdes_firmware_core_config.field.vco_rate;
    return PHYMOD_E_NONE; 
}


int tsce_phy_firmware_lane_config_set(const phymod_phy_access_t* phy, phymod_firmware_lane_config_t fw_config)
{
    struct eagle_tsc_uc_lane_config_st serdes_firmware_config;
    serdes_firmware_config.field.lane_cfg_from_pcs = fw_config.LaneConfigFromPCS;
    serdes_firmware_config.field.an_enabled        = fw_config.AnEnabled;
    serdes_firmware_config.field.dfe_on            = fw_config.DfeOn; 
    serdes_firmware_config.field.force_brdfe_on    = fw_config.ForceBrDfe;
    serdes_firmware_config.field.cl72_emulation_en = fw_config.Cl72Enable;
    serdes_firmware_config.field.scrambling_dis    = fw_config.ScramblingDisable;
    serdes_firmware_config.field.unreliable_los    = fw_config.UnreliableLos;
    serdes_firmware_config.field.media_type        = fw_config.MediaType;
    PHYMOD_IF_ERR_RETURN(eagle_tsc_set_uc_lane_cfg(&phy->access, serdes_firmware_config));
    return PHYMOD_E_NONE;
}


int tsce_phy_firmware_lane_config_get(const phymod_phy_access_t* phy, phymod_firmware_lane_config_t* fw_config)
{
    struct eagle_tsc_uc_lane_config_st serdes_firmware_config;
    PHYMOD_IF_ERR_RETURN(eagle_tsc_get_uc_lane_cfg(&phy->access, &serdes_firmware_config));
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

/* reset pll sequencer
 * flags - unused parameter
 */
int tsce_core_pll_sequencer_restart(const phymod_core_access_t* core, uint32_t flags, phymod_sequencer_operation_t operation)
{
    switch (operation) {
        case phymodSeqOpStop:
            PHYMOD_IF_ERR_RETURN
                (temod_pll_sequencer_control(&core->access, 0));
        break;
        case phymodSeqOpStart:
            PHYMOD_IF_ERR_RETURN
                (temod_pll_sequencer_control(&core->access, 1));
            
            /* PHYMOD_IF_ERR_RETURN
                (temod_pll_lock_wait(&core->access, 250000)); */
        break;
        case phymodSeqOpRestart:
            PHYMOD_IF_ERR_RETURN
                (temod_pll_sequencer_control(&core->access, 0));
            
            PHYMOD_IF_ERR_RETURN
                (temod_pll_sequencer_control(&core->access, 1));
            
            /* PHYMOD_IF_ERR_RETURN
                (temod_pll_lock_wait(&core->access, 250000)); */
        break;
        default:
            return PHYMOD_E_UNAVAIL;
        break;
    }     
    return PHYMOD_E_NONE;
}

int tsce_core_wait_event(const phymod_core_access_t* core, phymod_core_event_t event, uint32_t timeout)
{
    switch(event){
    case phymodCoreEventPllLock: 
        /* PHYMOD_IF_ERR_RETURN(temod_pll_lock_wait(&core->access, timeout)); */
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("illegal wait event %u"), event));
    }
    return PHYMOD_E_NONE;
}

/* reset rx sequencer 
 * flags - unused parameter
 */
int tsce_phy_rx_restart(const phymod_phy_access_t* phy)
{
    PHYMOD_IF_ERR_RETURN(eagle_tsc_rx_restart(&phy->access, 1)); 
    return PHYMOD_E_NONE;
}


int tsce_phy_polarity_set(const phymod_phy_access_t* phy, const phymod_polarity_t* polarity)
{
    PHYMOD_IF_ERR_RETURN
        (temod_tx_rx_polarity_set(&phy->access, polarity->tx_polarity, polarity->rx_polarity));
    return PHYMOD_E_NONE;
}


int tsce_phy_polarity_get(const phymod_phy_access_t* phy, phymod_polarity_t* polarity)
{
   PHYMOD_IF_ERR_RETURN
        (temod_tx_rx_polarity_get(&phy->access, &polarity->tx_polarity, &polarity->rx_polarity));
    return PHYMOD_E_NONE;
}


int tsce_phy_tx_set(const phymod_phy_access_t* phy, const phymod_tx_t* tx)
{
    
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_write_tx_afe(&phy->access, TX_AFE_PRE, tx->pre));
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_write_tx_afe(&phy->access, TX_AFE_MAIN, tx->main));
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_write_tx_afe(&phy->access, TX_AFE_POST1, tx->post));
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_write_tx_afe(&phy->access, TX_AFE_POST2, tx->post2));
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_write_tx_afe(&phy->access, TX_AFE_POST3, tx->post3));
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_write_tx_afe(&phy->access, TX_AFE_AMP,  tx->amp));
    
    return PHYMOD_E_NONE;
}

int tsce_phy_tx_get(const phymod_phy_access_t* phy, phymod_tx_t* tx)
{
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_read_tx_afe(&phy->access, TX_AFE_PRE, &tx->pre));
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_read_tx_afe(&phy->access, TX_AFE_MAIN, &tx->main));
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_read_tx_afe(&phy->access, TX_AFE_POST1, &tx->post));
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_read_tx_afe(&phy->access, TX_AFE_POST2, &tx->post2));
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_read_tx_afe(&phy->access, TX_AFE_POST3, &tx->post3));
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_read_tx_afe(&phy->access, TX_AFE_AMP, &tx->amp));
    return PHYMOD_E_NONE;
}


int tsce_phy_media_type_tx_get(const phymod_phy_access_t* phy, phymod_media_typed_t media, phymod_tx_t* tx)
{
    /* Place your code here */

    PHYMOD_MEMSET(tx, 0, sizeof(*tx));
        
    return PHYMOD_E_NONE;
        
    
}


int tsce_phy_tx_override_set(const phymod_phy_access_t* phy, const phymod_tx_override_t* tx_override)
{
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_tx_pi_freq_override(&phy->access,
                                    tx_override->phase_interpolator.enable,
                                    tx_override->phase_interpolator.value));    
    return PHYMOD_E_NONE;
}

int tsce_phy_tx_override_get(const phymod_phy_access_t* phy, phymod_tx_override_t* tx_override)
{
/*
    PHYMOD_IF_ERR_RETURN
        (temod_tx_pi_control_get(&phy->access, &tx_override->phase_interpolator.value));
*/
    return PHYMOD_E_NONE;
}


int tsce_phy_rx_set(const phymod_phy_access_t* phy, const phymod_rx_t* rx)
{
    uint32_t i;

    /* params check */
    if((rx->num_of_dfe_taps == 0) || (rx->num_of_dfe_taps < TSCE_NOF_DFES)){
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("illegal number of DFEs to set %u"), rx->num_of_dfe_taps));
    }

    /*vga set*/
    if (rx->vga.enable) {
        /* first stop the rx adaption */
        PHYMOD_IF_ERR_RETURN(eagle_tsc_stop_rx_adaptation(&phy->access, 1));
        PHYMOD_IF_ERR_RETURN(eagle_tsc_write_rx_afe(&phy->access, RX_AFE_VGA, rx->vga.value));
    } else {
        PHYMOD_IF_ERR_RETURN(eagle_tsc_stop_rx_adaptation(&phy->access, 0));
    }
    
    /* dfe set */
    for (i = 0 ; i < rx->num_of_dfe_taps ; i++){
        if(rx->dfe[i].enable){
            PHYMOD_IF_ERR_RETURN(eagle_tsc_stop_rx_adaptation(&phy->access, 1));
            switch (i) {
                case 0:
                    PHYMOD_IF_ERR_RETURN(eagle_tsc_write_rx_afe(&phy->access, RX_AFE_DFE1, rx->dfe[i].value));
                    break;
                case 1:
                    PHYMOD_IF_ERR_RETURN(eagle_tsc_write_rx_afe(&phy->access, RX_AFE_DFE2, rx->dfe[i].value));
                    break;
                case 2:
                    PHYMOD_IF_ERR_RETURN(eagle_tsc_write_rx_afe(&phy->access, RX_AFE_DFE3, rx->dfe[i].value));
                    break;
                case 3:
                    PHYMOD_IF_ERR_RETURN(eagle_tsc_write_rx_afe(&phy->access, RX_AFE_DFE4, rx->dfe[i].value));
                    break;
                case 4:
                    PHYMOD_IF_ERR_RETURN(eagle_tsc_write_rx_afe(&phy->access, RX_AFE_DFE5, rx->dfe[i].value));
                    break;
                default:
                    return PHYMOD_E_PARAM;
            }
        } else {
            PHYMOD_IF_ERR_RETURN(eagle_tsc_stop_rx_adaptation(&phy->access, 0));
        }
    }
     
    /* peaking filter set */
    if(rx->peaking_filter.enable){
        /* first stop the rx adaption */
        PHYMOD_IF_ERR_RETURN(eagle_tsc_stop_rx_adaptation(&phy->access, 1));
        PHYMOD_IF_ERR_RETURN(eagle_tsc_write_rx_afe(&phy->access, RX_AFE_PF, rx->peaking_filter.value));
    } else {
        PHYMOD_IF_ERR_RETURN(eagle_tsc_stop_rx_adaptation(&phy->access, 0));
    }
    
    if(rx->low_freq_peaking_filter.enable){
        /* first stop the rx adaption */
        PHYMOD_IF_ERR_RETURN(eagle_tsc_stop_rx_adaptation(&phy->access, 1));
        PHYMOD_IF_ERR_RETURN(eagle_tsc_write_rx_afe(&phy->access, RX_AFE_PF2, rx->low_freq_peaking_filter.value));
    } else {
        PHYMOD_IF_ERR_RETURN(eagle_tsc_stop_rx_adaptation(&phy->access, 0));
    }
    return PHYMOD_E_NONE;
}


int tsce_phy_rx_get(const phymod_phy_access_t* phy, phymod_rx_t* rx)
{
    
    return PHYMOD_E_NONE;
}


int tsce_phy_reset_set(const phymod_phy_access_t* phy, const phymod_phy_reset_t* reset)
{
        
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
        
    
}


int tsce_phy_reset_get(const phymod_phy_access_t* phy, phymod_phy_reset_t* reset)
{
        
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
        
    
}


int tsce_phy_power_set(const phymod_phy_access_t* phy, const phymod_phy_power_t* power)
{
    phymod_phy_access_t pm_phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    /* next program the tx fir taps and driver current based on the input */
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    if ((power->tx == phymodPowerOff) && (power->rx == phymodPowerOff)) {
        for (i = 0; i < num_lane; i++) {
            pm_phy_copy.access.lane = 1 << (start_lane + i);
            PHYMOD_IF_ERR_RETURN(temod_port_enable_set(&pm_phy_copy.access, 0));
        }
    }
    if ((power->tx == phymodPowerOn) && (power->rx == phymodPowerOn)) {
        for (i = 0; i < num_lane; i++) {
            pm_phy_copy.access.lane = 1 << (start_lane + i);
            PHYMOD_IF_ERR_RETURN(temod_port_enable_set(&pm_phy_copy.access, 1));
        }
    }
    if ((power->tx == phymodPowerOff) && (power->rx == phymodPowerNoChange)) {
            /* disable tx on the PMD side */
            PHYMOD_IF_ERR_RETURN(eagle_tsc_tx_disable(&phy->access, 1));
    }
    if ((power->tx == phymodPowerOn) && (power->rx == phymodPowerNoChange)) {
            /* enable tx on the PMD side */
            PHYMOD_IF_ERR_RETURN(eagle_tsc_tx_disable(&phy->access, 0));
    }
    if ((power->tx == phymodPowerNoChange) && (power->rx == phymodPowerOff)) {
            /* disable rx on the PMD side */
            PHYMOD_IF_ERR_RETURN(temod_rx_squelch_set(&phy->access, 1));
    }
    if ((power->tx == phymodPowerNoChange) && (power->rx == phymodPowerOn)) {
            /* enable rx on the PMD side */
            PHYMOD_IF_ERR_RETURN(temod_rx_squelch_set(&phy->access, 0));
    }
    return PHYMOD_E_NONE; 
}

int tsce_phy_power_get(const phymod_phy_access_t* phy, phymod_phy_power_t* power)
{
    int enable;
    uint32_t lb_enable;
    phymod_phy_access_t pm_phy_copy;
    int start_lane, num_lane;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    /* next program the tx fir taps and driver current based on the input */
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    pm_phy_copy.access.lane = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN(temod_rx_squelch_get(&pm_phy_copy.access, &enable));

    /* next check if PMD loopback is on */ 
    if (enable) {                           
        PHYMOD_IF_ERR_RETURN(eagle_pmd_loopback_get(&pm_phy_copy.access, &lb_enable));
        if (lb_enable) enable = 0;
    }

    power->rx = (enable == 1)? phymodPowerOff: phymodPowerOn;
    /* Commented the following line. Because if in PMD loopback mode, we squelch the
           xmit, and we should still see the correct port status */
    /* PHYMOD_IF_ERR_RETURN(temod_tx_squelch_get(&pm_phy_copy.access, &enable)); */
    power->tx = (enable == 1)? phymodPowerOff: phymodPowerOn;
    return PHYMOD_E_NONE;
}

int tsce_phy_tx_lane_control_set(const phymod_phy_access_t* phy, uint32_t enable)
{   
    PHYMOD_IF_ERR_RETURN(temod_tx_lane_control(&phy->access, enable, TEMOD_TX_LANE_RESET_TRAFFIC));
    return PHYMOD_E_NONE;
}

int tsce_phy_tx_lane_control_get(const phymod_phy_access_t* phy, uint32_t* enable)
{   
    return PHYMOD_E_NONE;
}


int tsce_phy_rx_lane_control_set(const phymod_phy_access_t* phy, uint32_t enable)
{       
    PHYMOD_IF_ERR_RETURN(temod_rx_lane_control_set(&phy->access, enable));
    return PHYMOD_E_NONE;
}

int tsce_phy_rx_lane_control_get(const phymod_phy_access_t* phy, uint32_t* enable)
{   
    return PHYMOD_E_NONE;
}

int tsce_phy_tx_disable_set(const phymod_phy_access_t* phy, uint32_t tx_disable)
{       
    return PHYMOD_E_NONE;
}


int tsce_phy_tx_disable_get(const phymod_phy_access_t* phy, uint32_t* tx_disable)
{
    return PHYMOD_E_NONE;
}

int tsce_phy_fec_enable_set(const phymod_phy_access_t* phy, uint32_t enable)
{
    PHYMOD_IF_ERR_RETURN(temod_fecmode_set(&phy->access, enable));
    return PHYMOD_E_NONE;

}

int tsce_phy_fec_enable_get(const phymod_phy_access_t* phy, uint32_t* enable)
{
    PHYMOD_IF_ERR_RETURN(temod_fecmode_get(&phy->access, enable));
    return PHYMOD_E_NONE;

}

int _tsce_pll_multiplier_get(uint32_t pll_div, uint32_t *pll_multiplier)
{
    switch (pll_div) {
    case 0x0:
        *pll_multiplier = 46;
        break;
    case 0x1:
        *pll_multiplier = 72;
        break;
    case 0x2:
        *pll_multiplier = 40;
        break;
    case 0x3:
        *pll_multiplier = 42;
        break;
    case 0x4:
        *pll_multiplier = 48;
        break;
    case 0x5:
        *pll_multiplier = 50;
        break;
    case 0x6:
        *pll_multiplier = 52;
        break;
    case 0x7:
        *pll_multiplier = 54;
        break;
    case 0x8:
        *pll_multiplier = 60;
        break;
    case 0x9:
        *pll_multiplier = 64;
        break;
    case 0xa:
        *pll_multiplier = 66;
        break;
    case 0xb:
        *pll_multiplier = 68;
        break;
    case 0xc:
        *pll_multiplier = 70;
        break;
    case 0xd:
        *pll_multiplier = 80;
        break;
    case 0xe:
        *pll_multiplier = 92;
        break;
    case 0xf:
        *pll_multiplier = 100;
        break;
    default:
        *pll_multiplier = 66;
        break;
    }
    return PHYMOD_E_NONE;
}

STATIC
int _tsce_st_hto_interface_config_set(const temod_device_aux_modes_t *device, int lane_num, int speed_vec, uint32_t *new_pll_div, int16_t *os_mode)
{
    int i, max_lane=4 ;
    *os_mode = -1 ;
    /* ST */
    for(i=0;i<max_lane; i++) {
        if(device->st_hcd[i]==speed_vec){
            *new_pll_div = device->st_pll_div[i];
            *os_mode     = device->st_os[i] ;
            break ;
        }
    }
    /* HTO */
    if(device->hto_enable[lane_num]){
        *new_pll_div = device->hto_pll_div[lane_num];
        *os_mode     = device->hto_os[lane_num] ;
    }
    return PHYMOD_E_NONE;
} 

int tsce_phy_interface_config_set(const phymod_phy_access_t* phy, uint32_t flags, const phymod_phy_inf_config_t* config)
{
    phymod_tx_t tx_params;  
    uint32_t current_pll_div=0;
    uint32_t new_pll_div=0;
    uint16_t new_speed_vec=0;
    int16_t  new_os_mode =-1;
    temod_spd_intfc_type spd_intf = TEMOD_SPD_ILLEGAL;
    phymod_phy_access_t pm_phy_copy;
    int start_lane, num_lane, i, pll_switch = 0;
    uint32_t os_dfeon=0;
    
    /* sc_table_entry exp_entry; RAVI */
    phymod_firmware_lane_config_t firmware_lane_config;  
    phymod_firmware_core_config_t firmware_core_config;

	firmware_lane_config.MediaType = 0;

    /* next program the tx fir taps and driver current based on the input */
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    /* Hold the per lne soft reset bit */
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
            (eagle_lane_soft_reset_release(&pm_phy_copy.access, 0));
    }

    pm_phy_copy.access.lane = 0x1 << start_lane;
     PHYMOD_IF_ERR_RETURN
        (tsce_phy_firmware_lane_config_get(&pm_phy_copy, &firmware_lane_config)); 

    /* make sure that an and config from pcs is off */
    firmware_core_config.CoreConfigFromPCS = 0;
    firmware_lane_config.AnEnabled = 0;
    firmware_lane_config.LaneConfigFromPCS = 0;

    PHYMOD_IF_ERR_RETURN
        (temod_update_port_mode(&phy->access, (int *) &pll_switch));
    
    if  (config->interface_type == phymodInterfaceSGMII) {
        firmware_lane_config.MediaType = phymodFirmwareMediaTypePcbTraceBackPlane;
        switch (config->data_rate) {
            case 10:  
                spd_intf = TEMOD_SPD_10_X1_SGMII;
                break;
            case 100:
                spd_intf = TEMOD_SPD_100_X1_SGMII;
                break;
            case 1000:
                if(config->pll_divider_req==40) {
                    spd_intf = TEMOD_SPD_1000_SGMII;
                } else {
                    spd_intf = TEMOD_SPD_1000_X1_SGMII;
                }
                break;
            default:
                spd_intf = TEMOD_SPD_1000_X1_SGMII;
                break;
        }
    } else if (config->interface_type == phymodInterface1000X) {
            firmware_lane_config.MediaType = phymodFirmwareMediaTypeOptics;
            if (config->data_rate == 1000) {
                if(config->pll_divider_req==40) {
                    spd_intf = TEMOD_SPD_1000_SGMII;
                } else {
                    spd_intf = TEMOD_SPD_1000_X1_SGMII;
                }
            } else if (config->data_rate == 2500) { 
                if(config->pll_divider_req==40) {
                spd_intf = TEMOD_SPD_2500;
                } else {
                spd_intf = TEMOD_SPD_2500_X1;
                }
            } else if (config->data_rate == 100) { 
                spd_intf = TEMOD_SPD_100_X1_SGMII;
            } else if (config->data_rate == 10) { 
                spd_intf = TEMOD_SPD_10_X1_SGMII;
            } else {
                spd_intf = TEMOD_SPD_1000_X1_SGMII; 
            }
     } else if (config->interface_type == phymodInterfaceSFI) {
            spd_intf = TEMOD_SPD_10000_XFI;
            /* firmware_lane_config.DfeOn = 1; */
            firmware_lane_config.MediaType = phymodFirmwareMediaTypeOptics;
     } else if (config->interface_type == phymodInterfaceXFI) {
            firmware_lane_config.MediaType = phymodFirmwareMediaTypePcbTraceBackPlane;
            /* firmware_lane_config.DfeOn = 1; */
            if (PHYMOD_INTERFACE_MODES_IS_HIGIG(config)) {
                if (config->data_rate == 10000) {
                    spd_intf = TEMOD_SPD_10000_XFI;
                } else {
                    spd_intf = TEMOD_SPD_10600_XFI_HG;
                }
            } else {
               if (config->data_rate == 5000) { 
                if(config->pll_divider_req==40) {
                  spd_intf = TEMOD_SPD_5000;
                 } else {
                  spd_intf = TEMOD_SPD_5000_XFI;
                 } 
				           } else {
                    spd_intf = TEMOD_SPD_10000_XFI;
			            }
            } 
     } else if (config->interface_type == phymodInterfaceRXAUI) {
            if (PHYMOD_INTERFACE_MODES_IS_HIGIG(config)) {
                switch (config->data_rate) {
                    case 10000:
                        if (PHYMOD_INTERFACE_MODES_IS_SCR(config)) {
                            spd_intf = TEMOD_SPD_10000_HI_DXGXS_SCR;
                        } else {
                            spd_intf = TEMOD_SPD_10000_HI_DXGXS;
                        }
                        break;               
                    case 10500:
                        spd_intf = TEMOD_SPD_10500_HI;
                        break;               
                    case 12773:
                        spd_intf = TEMOD_SPD_12773_HI_DXGXS;
                        break;
                    case 15750:
                        spd_intf = TEMOD_SPD_15750_HI_DXGXS;
                        break;
                    case 21000:
                        spd_intf = TEMOD_SPD_21G_HI_DXGXS;
                        break;
                    default:
                        spd_intf = TEMOD_SPD_1000_X1_SGMII;
                        break;
                }
            } else {
                switch (config->data_rate) {
                    case 10000:
                        if (PHYMOD_INTERFACE_MODES_IS_SCR(config)) {
                            spd_intf = TEMOD_SPD_10000_DXGXS_SCR;
                        } else {
                            spd_intf = TEMOD_SPD_10000_DXGXS;
                        }
                        break; 
                    case 12773:
                        spd_intf = TEMOD_SPD_12773_DXGXS;
                        break;
                    case 20000:
                        spd_intf = TEMOD_SPD_20G_DXGXS;
                        break;
                    default:
                        spd_intf = TEMOD_SPD_1000_X1_SGMII;
                        break;
                }
            }
    } else if (config->interface_type == phymodInterfaceX2) {
        spd_intf = TEMOD_SPD_10000_X2;
    } else if (config->interface_type == phymodInterfaceXLAUI) {
        firmware_lane_config.MediaType = phymodFirmwareMediaTypePcbTraceBackPlane;
        spd_intf = TEMOD_SPD_40G_XLAUI;
    } else if (config->interface_type == phymodInterfaceKR4) {
        spd_intf = TEMOD_SPD_40G_XLAUI;
    } else if (config->interface_type == phymodInterfaceXGMII) {
        switch (config->data_rate) {
            case 10000:
                spd_intf = TEMOD_SPD_10000;
                break;
            case 13000:
                spd_intf = TEMOD_SPD_13000;
                break;
            case 15000:
                spd_intf = TEMOD_SPD_15000;
                break;
            case 16000:
                spd_intf = TEMOD_SPD_16000;
                break;
            case 20000:
                spd_intf = TEMOD_SPD_20000;
                break;
            case 21000:
                spd_intf = TEMOD_SPD_21000;
                break;
            case 25000:
            case 25455:
                spd_intf = TEMOD_SPD_25455;
                break;
            case 30000:
            case 31500:
                spd_intf = TEMOD_SPD_31500;
                break;
            case 40000:
                spd_intf = TEMOD_SPD_40G_X4;
                break;
            case 42000:
                spd_intf = TEMOD_SPD_42G_X4;
                break;
            default:
                spd_intf = TEMOD_SPD_1000_X1_SGMII;
                break;
        }
    }

    PHYMOD_IF_ERR_RETURN
        (temod_get_plldiv(&phy->access, &current_pll_div));

    PHYMOD_IF_ERR_RETURN
        (temod_plldiv_lkup_get(&phy->access, spd_intf, &new_pll_div, &new_speed_vec));
    
    if(config->device_aux_modes !=NULL){    
        PHYMOD_IF_ERR_RETURN
            (_tsce_st_hto_interface_config_set(config->device_aux_modes, start_lane, new_speed_vec, &new_pll_div, &new_os_mode)) ;
    }

    if(new_os_mode>=0) {
        new_os_mode |= 0x80000000 ;   
    } else {
        new_os_mode = 0 ;
    }

    
    PHYMOD_IF_ERR_RETURN
        (temod_pmd_osmode_set(&phy->access, spd_intf, new_os_mode));
    PHYMOD_IF_ERR_RETURN(temod_osdfe_on_lkup_get(&phy->access, spd_intf, &os_dfeon));
    firmware_lane_config.DfeOn = 0;
    if(os_dfeon == 0x1)
      firmware_lane_config.DfeOn = 1;
      
    /* if pll change is enabled. new_pll_div is the reg vector value */
    if((current_pll_div != new_pll_div) && (PHYMOD_IF_FLAGS_DONT_TURN_OFF_PLL & flags)){
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, 
                               (_PHYMOD_MSG("pll has to change for speed_set from %u to %u but DONT_TURN_OFF_PLL flag is enabled"),
                                 current_pll_div, new_pll_div));
    }
    /* pll switch is required and expected */
    if((current_pll_div != new_pll_div) && !(PHYMOD_IF_FLAGS_DONT_TURN_OFF_PLL & flags)) {
        /* phymod_access_t tmp_phy_access; */
        uint32_t pll_multiplier, vco_rate;
        
        /* tmp_phy_access = phy->access; */

        PHYMOD_IF_ERR_RETURN
            (eagle_core_soft_reset_release(&pm_phy_copy.access, 0));

        /* release the uc reset */
        PHYMOD_IF_ERR_RETURN
            (eagle_uc_reset(&pm_phy_copy.access ,1)); 

        /* set the PLL divider */
        PHYMOD_IF_ERR_RETURN
            (eagle_pll_mode_set(&pm_phy_copy.access, new_pll_div));
        PHYMOD_IF_ERR_RETURN
            (_tsce_pll_multiplier_get(new_pll_div, &pll_multiplier));
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
        firmware_core_config.VcoRate = (vco_rate - 5750) / 250 + 1;                 

        /* change the  master port num to the current caller port */
        PHYMOD_IF_ERR_RETURN
            (temod_master_port_num_set(&pm_phy_copy.access, start_lane));

        PHYMOD_IF_ERR_RETURN
            (temod_pll_reset_enable_set(&pm_phy_copy.access, 1));

        /* update the firmware config properly */
        PHYMOD_IF_ERR_RETURN
            (tsce_phy_firmware_core_config_set(&pm_phy_copy, firmware_core_config));
        PHYMOD_IF_ERR_RETURN
            (eagle_core_soft_reset_release(&pm_phy_copy.access, 1));
    }

    PHYMOD_IF_ERR_RETURN
        (temod_set_spd_intf(&phy->access, spd_intf));

    /* change TX parameters if enabled */
    /* if((PHYMOD_IF_FLAGS_DONT_OVERIDE_TX_PARAMS & flags) == 0) */ {
        PHYMOD_IF_ERR_RETURN
            (tsce_phy_media_type_tx_get(phy, phymodMediaTypeMid, &tx_params));

#ifdef FIXME 
/*
 * We are calling this function with all params=0
*/
        PHYMOD_IF_ERR_RETURN
            (tsce_phy_tx_set(phy, &tx_params));
#endif
    }
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane = 0x1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
             (tsce_phy_firmware_lane_config_set(&pm_phy_copy, firmware_lane_config));
    }
    
    /* release the per lne soft reset bit */
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
            (eagle_lane_soft_reset_release(&pm_phy_copy.access, 1));
    }
              
    return PHYMOD_E_NONE;
}

int _tsce_speed_id_interface_config_get(const phymod_phy_access_t* phy, int speed_id, phymod_phy_inf_config_t* config)
{
    switch (speed_id) {
    case 0x1:
        config->data_rate = 10;
        config->interface_type = phymodInterfaceSGMII;
        break;
    case 0x2:
        config->data_rate = 100;
        config->interface_type = phymodInterfaceSGMII;
        break;
    case 0x3:
        {
            phymod_firmware_lane_config_t lane_config;
            PHYMOD_IF_ERR_RETURN
                (tsce_phy_firmware_lane_config_get(phy, &lane_config)); 
            if (lane_config.MediaType == phymodFirmwareMediaTypeOptics) {
                config->interface_type = phymodInterface1000X;
            } else {
                config->interface_type = phymodInterfaceSGMII;
            }     
            config->data_rate = 1000;
            break;
        }
    case 0x4:
        config->data_rate = 1000;
        config->interface_type = phymodInterfaceCX;
        break;
    case 0x5:
        config->data_rate = 1000;
        config->interface_type = phymodInterfaceKX;
        break;
    case 0x6:
        config->data_rate = 2500;
        config->interface_type = phymodInterfaceSR;
        break;
    case 0x7:
        config->data_rate = 5000;
        config->interface_type = phymodInterfaceSR;
        break;
    case 0x8:
        config->data_rate = 10000;
        config->interface_type = phymodInterfaceCX4;
        break;
    case 0x9:
        config->data_rate = 10000;
        config->interface_type = phymodInterfaceKX4;
        break;
    case 0xa:
        config->data_rate = 10000;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0xb:
        config->data_rate = 13000;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0xc:
        config->data_rate = 15000;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0xd:
        config->data_rate = 16000;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0xe:
        config->data_rate = 20000;
        config->interface_type = phymodInterfaceCX4;
        break;
    case 0xf:
        config->data_rate = 10000;
        config->interface_type = phymodInterfaceCR2;
        break;
    case 0x10:
        config->data_rate = 10000;
        config->interface_type = phymodInterfaceX2;
        break;
    case 0x11:
        config->data_rate = 20000;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0x12:
        config->data_rate = 10500;
        config->interface_type = phymodInterfaceX2;
        break;
    case 0x13:
        config->data_rate = 21000;
        config->interface_type = phymodInterfaceCX4;
        break;
    case 0x14:
        config->data_rate = 12700;
        config->interface_type = phymodInterfaceX2;
        break;
    case 0x15:
        config->data_rate = 25450;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0x16:
        config->data_rate = 15750;
        config->interface_type = phymodInterfaceX2;
        break;
    case 0x17:
        config->data_rate = 31500;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0x18:
        config->data_rate = 31500;
        config->interface_type = phymodInterfaceKR4;
        break;
    case 0x19:
        config->data_rate = 20000;
        config->interface_type = phymodInterfaceCX2;
        break;
    case 0x1a:
        config->data_rate = 20000;
        config->interface_type = phymodInterfaceX2;
        break;
    case 0x1b:
        config->data_rate = 40000;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0x1c:
        config->data_rate = 10000;
        config->interface_type = phymodInterfaceKR;
        break;
    case 0x1d:
        config->data_rate = 10600;
        config->interface_type = phymodInterfaceXFI;
        break;
    case 0x1e:
        config->data_rate = 20000;
        config->interface_type = phymodInterfaceKR2;
        break;
    case 0x1f:
        config->data_rate = 20000;
        config->interface_type = phymodInterfaceCR2;
        break;
    case 0x20:
        config->data_rate = 21000;
        config->interface_type = phymodInterfaceX2;
        break;
    case 0x21:
        config->data_rate = 40000;
        config->interface_type = phymodInterfaceKR4;
        break;
    case 0x22:
        config->data_rate = 40000;
        config->interface_type = phymodInterfaceCR4;
        break;
    case 0x23:
        config->data_rate = 42000;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0x24:
        config->data_rate = 100000;
        config->interface_type = phymodInterfaceCR10;
        break;
    case 0x25:
        config->data_rate = 107000;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0x26:
        config->data_rate = 120000;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0x27:
        config->data_rate = 127000;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0x31:
        config->data_rate = 5000;
        config->interface_type = phymodInterfaceKR;
        break;
    case 0x32:
        config->data_rate = 10500;
        config->interface_type = phymodInterfaceXGMII;
        break;
    case 0x35:
        config->data_rate = 10;
        config->interface_type = phymodInterfaceSGMII;
        break;
    case 0x36:
        config->data_rate = 100;
        config->interface_type = phymodInterfaceSGMII;
        break;
    case 0x37:
        config->data_rate = 1000;
        config->interface_type = phymodInterfaceSGMII;
        break;
    case 0x38:
        config->data_rate = 2500;
        config->interface_type = phymodInterfaceSR;
        break;
    default:
        config->data_rate = 0;
        config->interface_type = phymodInterfaceSGMII;
        break;
    }
    return PHYMOD_E_NONE;
}


/* flags- unused parameter */
int tsce_phy_interface_config_get(const phymod_phy_access_t* phy, uint32_t flags, phymod_phy_inf_config_t* config)
{
    int speed_id;

    PHYMOD_IF_ERR_RETURN
        (temod_speed_id_get(&phy->access, &speed_id));
    PHYMOD_IF_ERR_RETURN
        (_tsce_speed_id_interface_config_get(phy, speed_id, config)); 
    return PHYMOD_E_NONE;
}


int tsce_phy_cl72_set(const phymod_phy_access_t* phy, uint32_t cl72_en)
{
    PHYMOD_IF_ERR_RETURN
        (temod_clause72_control(&phy->access, cl72_en));
    return PHYMOD_E_NONE;
}

int tsce_phy_cl72_get(const phymod_phy_access_t* phy, uint32_t* cl72_en)
{
        
        
    
    /* Place your code here */

        
    return PHYMOD_E_NONE;
        
    
}


int tsce_phy_cl72_status_get(const phymod_phy_access_t* phy, phymod_cl72_status_t* status)
{
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_display_cl72_status(&phy->access));
    /* once the PMD function updated, will update the value properly */
    status->locked = 0;
    return PHYMOD_E_NONE;
}

int tsce_phy_autoneg_ability_set(const phymod_phy_access_t* phy, const phymod_autoneg_ability_t* an_ability)
{
    temod_an_ability_t value;
    int start_lane, num_lane;
    phymod_phy_access_t phy_copy;

    /* next program the tx fir taps and driver current based on the input */
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane = 0x1 << start_lane;

    PHYMOD_MEMSET(&value, 0x0, sizeof(value));

    value.cl37_adv.an_cl72 = an_ability->an_cl72;
    value.cl73_adv.an_cl72 = an_ability->an_cl72;
    value.cl37_adv.an_hg2 = an_ability->an_hg2;
    value.cl37_adv.an_fec = an_ability->an_fec;
    value.cl73_adv.an_fec = an_ability->an_fec;

    /* check if sgmii  or not */
    if (PHYMOD_AN_CAPABILITIES_IS_SGMII(an_ability)) {
        switch (an_ability->sgmii_speed) {
        case phymod_CL37_SGMII_10M:
            value.cl37_adv.cl37_sgmii_speed = TEMOD_CL37_SGMII_10M;
            break;
        case phymod_CL37_SGMII_100M:
            value.cl37_adv.cl37_sgmii_speed = TEMOD_CL37_SGMII_100M;
            break;
        case phymod_CL37_SGMII_1000M:
            value.cl37_adv.cl37_sgmii_speed = TEMOD_CL37_SGMII_1000M;
            break;
        default:
            value.cl37_adv.cl37_sgmii_speed = TEMOD_CL37_SGMII_1000M;
            break;
        }
    }
    /* next check pause */
    if (PHYMOD_AN_CAPABILITIES_IS_SYMM_PAUSE(an_ability)) {
        value.cl37_adv.an_pause = TEMOD_SYMM_PAUSE;
        value.cl73_adv.an_pause = TEMOD_SYMM_PAUSE;
    } else if (PHYMOD_AN_CAPABILITIES_IS_ASYM_PAUSE(an_ability)) {
        value.cl37_adv.an_pause = TEMOD_ASYM_PAUSE;
        value.cl73_adv.an_pause = TEMOD_ASYM_PAUSE;
    }

    /* check cl73 and cl73 bam ability */
    if (PHYMOD_AN_TECH_ABILITY_IS_1G_KX(an_ability->an_tech_ability)) 
        value.cl73_adv.an_base_speed |= 1 << TEMOD_CL73_1000BASE_KX;
    if (PHYMOD_AN_TECH_ABILITY_IS_10G_KX4(an_ability->an_tech_ability)) 
        value.cl73_adv.an_base_speed |= 1 << TEMOD_CL73_10GBASE_KX4;
    if (PHYMOD_AN_TECH_ABILITY_IS_10G_KR(an_ability->an_tech_ability)) 
        value.cl73_adv.an_base_speed |= 1 << TEMOD_CL73_10GBASE_KR;
    if (PHYMOD_AN_TECH_ABILITY_IS_40G_KR4(an_ability->an_tech_ability)) 
        value.cl73_adv.an_base_speed |= 1 << TEMOD_CL73_40GBASE_KR4;
    if (PHYMOD_AN_TECH_ABILITY_IS_40G_CR4(an_ability->an_tech_ability)) 
        value.cl73_adv.an_base_speed |= 1 << TEMOD_CL73_40GBASE_CR4;
    if (PHYMOD_AN_TECH_ABILITY_IS_100G_CR10(an_ability->an_tech_ability)) 
        value.cl73_adv.an_base_speed |= 1 << TEMOD_CL73_100GBASE_CR10;
    if (PHYMOD_BAM_CL73_ABL_IS_20G_KR2(an_ability->an_tech_ability)) 
        value.cl73_adv.an_bam_speed |= 1 << TEMOD_CL73_BAM_20GBASE_KR2;
    if (PHYMOD_BAM_CL73_ABL_IS_20G_CR2(an_ability->an_tech_ability)) 
        value.cl73_adv.an_bam_speed |= 1 << TEMOD_CL73_BAM_20GBASE_CR2;

    /* check cl37 and cl37 bam ability */
    if (PHYMOD_BAM_CL37_ABL_IS_2P5G(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed |= 1 << TEMOD_CL37_BAM_2p5GBASE_X;
    if (PHYMOD_BAM_CL37_ABL_IS_5G_X4(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed |= 1 << TEMOD_CL37_BAM_5GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_6G_X4(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed |= 1 << TEMOD_CL37_BAM_6GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_10G_HIGIG(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed |= 1 << TEMOD_CL37_BAM_10GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_10G_CX4(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed |= 1 << TEMOD_CL37_BAM_10GBASE_X4_CX4;
    if (PHYMOD_BAM_CL37_ABL_IS_12G_X4(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed |= 1 << TEMOD_CL37_BAM_12GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_12P5_X4(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed |= 1 << TEMOD_CL37_BAM_12p5GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_10G_X2_CX4(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed |= 1 << TEMOD_CL37_BAM_10GBASE_X2_CX4;
    if (PHYMOD_BAM_CL37_ABL_IS_10G_DXGXS(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed |= 1 << TEMOD_CL37_BAM_10GBASE_X2;
    if (PHYMOD_BAM_CL37_ABL_IS_10P5G_DXGXS(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed |= 1 << TEMOD_CL37_BAM_BAM_10p5GBASE_X2;
    if (PHYMOD_BAM_CL37_ABL_IS_12P7_DXGXS(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed |= 1 << TEMOD_CL37_BAM_12p7GBASE_X2;
    if (PHYMOD_BAM_CL37_ABL_IS_20G_X2_CX4(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_20GBASE_X2_CX4;
    if (PHYMOD_BAM_CL37_ABL_IS_20G_X2(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_20GBASE_X2;
    if (PHYMOD_BAM_CL37_ABL_IS_13G_X4(an_ability->cl37bam_ability))
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_13GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_15G_X4(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_15GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_16G_X4(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_16GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_20G_X4_CX4(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_20GBASE_X4_CX4;
    if (PHYMOD_BAM_CL37_ABL_IS_20G_X4(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_20GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_21G_X4(an_ability->cl37bam_ability))
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_21GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_25P455G(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_25p455GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_31P5G(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_31p5GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_32P7G(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_32p7GBASE_X4;
    if (PHYMOD_BAM_CL37_ABL_IS_40G(an_ability->cl37bam_ability)) 
        value.cl37_adv.an_bam_speed1 |= 1 << TEMOD_CL37_BAM_40GBASE_X4;

    PHYMOD_IF_ERR_RETURN
        (temod_autoneg_set(&phy_copy.access, &value));
    return PHYMOD_E_NONE;
    
}

int tsce_phy_autoneg_ability_get(const phymod_phy_access_t* phy, phymod_autoneg_ability_t* an_ability_get_type){
    temod_an_ability_t value;
    phymod_phy_access_t phy_copy;
    int start_lane, num_lane;

    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane = 0x1 << start_lane;
    PHYMOD_MEMSET(&value, 0x0, sizeof(value));
     
    PHYMOD_IF_ERR_RETURN
        (temod_autoneg_local_ability_get(&phy_copy.access, &value));
    an_ability_get_type->an_cl72 = value.cl37_adv.an_cl72 | value.cl73_adv.an_cl72;
    an_ability_get_type->an_hg2 = value.cl37_adv.an_hg2;
    an_ability_get_type->an_fec = value.cl37_adv.an_fec | value.cl73_adv.an_fec;
    switch (value.cl37_adv.an_pause) {
    case TEMOD_ASYM_PAUSE:
        PHYMOD_AN_CAPABILITIES_ASYM_PAUSE_SET(an_ability_get_type);
        break;
    case TEMOD_SYMM_PAUSE:
        PHYMOD_AN_CAPABILITIES_SYMM_PAUSE_SET(an_ability_get_type);
        break;
    default:
        break;
    }
    /* get the cl37 sgmii speed */
    switch (value.cl37_adv.cl37_sgmii_speed) {
    case TEMOD_CL37_SGMII_10M:
        an_ability_get_type->sgmii_speed = phymod_CL37_SGMII_10M;
        break;
    case TEMOD_CL37_SGMII_100M:
        an_ability_get_type->sgmii_speed = phymod_CL37_SGMII_100M;
        break;
    case TEMOD_CL37_SGMII_1000M:
        an_ability_get_type->sgmii_speed = phymod_CL37_SGMII_1000M;
        break;
    default:
        break;
    }            
    /* first check cl73 ability */
    if (value.cl73_adv.an_base_speed &  1 << TEMOD_CL73_100GBASE_CR10) 
        PHYMOD_AN_TECH_ABILITY_100G_CR10_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_base_speed & 1 << TEMOD_CL73_40GBASE_CR4) 
        PHYMOD_AN_TECH_ABILITY_40G_CR4_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_base_speed & 1 << TEMOD_CL73_40GBASE_KR4) 
        PHYMOD_AN_TECH_ABILITY_40G_KR4_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_base_speed & 1 << TEMOD_CL73_10GBASE_KR) 
        PHYMOD_AN_TECH_ABILITY_10G_KR_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_base_speed & 1 << TEMOD_CL73_10GBASE_KX4) 
        PHYMOD_AN_TECH_ABILITY_10G_KX4_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_base_speed & 1 << TEMOD_CL73_1000BASE_KX) 
        PHYMOD_AN_TECH_ABILITY_1G_KX_SET(an_ability_get_type->an_tech_ability);

    /* next check cl73 bam ability */
    if (value.cl73_adv.an_bam_speed & 1 << TEMOD_CL73_BAM_20GBASE_KR2) 
        PHYMOD_BAM_CL73_ABL_20G_KR2_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_bam_speed & 1 << TEMOD_CL73_BAM_20GBASE_CR2) 
        PHYMOD_BAM_CL73_ABL_20G_CR2_SET(an_ability_get_type->an_tech_ability);

    /* check cl37 bam ability */
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_2p5GBASE_X) 
        PHYMOD_BAM_CL37_ABL_2P5G_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_5GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_5G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_6GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_6G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_10GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_10G_HIGIG_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_10GBASE_X4_CX4) 
        PHYMOD_BAM_CL37_ABL_10G_CX4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_10GBASE_X2) 
        PHYMOD_BAM_CL37_ABL_10G_DXGXS_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_10GBASE_X2_CX4) 
        PHYMOD_BAM_CL37_ABL_10G_X2_CX4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_BAM_10p5GBASE_X2) 
        PHYMOD_BAM_CL37_ABL_10P5G_DXGXS_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_12GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_12G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_12p5GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_12P5_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_12p7GBASE_X2) 
        PHYMOD_BAM_CL37_ABL_12P7_DXGXS_SET(an_ability_get_type->cl37bam_ability);

    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_13GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_13G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_15GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_15G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_15p75GBASE_X2) 
        PHYMOD_BAM_CL37_ABL_12P7_DXGXS_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_16GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_16G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_20GBASE_X4_CX4) 
        PHYMOD_BAM_CL37_ABL_20G_X4_CX4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_20GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_20G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_20GBASE_X2) 
        PHYMOD_BAM_CL37_ABL_20G_X2_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_20GBASE_X2_CX4) 
        PHYMOD_BAM_CL37_ABL_20G_X2_CX4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_21GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_21G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_25p455GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_25P455G_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_31p5GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_31P5G_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_32p7GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_32P7G_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_40GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_40G_SET(an_ability_get_type->cl37bam_ability);


    return PHYMOD_E_NONE;
}

int tsce_phy_autoneg_remote_ability_get(const phymod_phy_access_t* phy, phymod_autoneg_ability_t* an_ability_get_type)
{

    temod_an_ability_t value;
    phymod_phy_access_t phy_copy;
    int start_lane, num_lane;

    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane = 0x1 << start_lane;
    PHYMOD_MEMSET(&value, 0x0, sizeof(value));
     
    PHYMOD_IF_ERR_RETURN
        (temod_autoneg_remote_ability_get(&phy_copy.access, &value));
    an_ability_get_type->an_cl72 = value.cl37_adv.an_cl72 | value.cl73_adv.an_cl72;
    an_ability_get_type->an_hg2 = value.cl37_adv.an_hg2;
    an_ability_get_type->an_fec = value.cl37_adv.an_fec | value.cl73_adv.an_fec;
    switch (value.cl37_adv.an_pause) {
    case TEMOD_ASYM_PAUSE:
        PHYMOD_AN_CAPABILITIES_ASYM_PAUSE_SET(an_ability_get_type);
        break;
    case TEMOD_SYMM_PAUSE:
        PHYMOD_AN_CAPABILITIES_SYMM_PAUSE_SET(an_ability_get_type);
        break;
    default:
        break;
    }
    /* get the cl37 sgmii speed */
    switch (value.cl37_adv.cl37_sgmii_speed) {
    case TEMOD_CL37_SGMII_10M:
        an_ability_get_type->sgmii_speed = phymod_CL37_SGMII_10M;
        break;
    case TEMOD_CL37_SGMII_100M:
        an_ability_get_type->sgmii_speed = phymod_CL37_SGMII_100M;
        break;
    case TEMOD_CL37_SGMII_1000M:
        an_ability_get_type->sgmii_speed = phymod_CL37_SGMII_1000M;
        break;
    default:
        break;
    }            

    /* first check cl73 ability */
    if (value.cl73_adv.an_base_speed &  1 << TEMOD_CL73_100GBASE_CR10) 
        PHYMOD_AN_TECH_ABILITY_100G_CR10_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_base_speed & 1 << TEMOD_CL73_40GBASE_CR4) 
        PHYMOD_AN_TECH_ABILITY_40G_CR4_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_base_speed & 1 << TEMOD_CL73_40GBASE_KR4) 
        PHYMOD_AN_TECH_ABILITY_40G_KR4_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_base_speed & 1 << TEMOD_CL73_10GBASE_KR) 
        PHYMOD_AN_TECH_ABILITY_10G_KR_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_base_speed & 1 << TEMOD_CL73_10GBASE_KX4) 
        PHYMOD_AN_TECH_ABILITY_10G_KX4_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_base_speed & 1 << TEMOD_CL73_1000BASE_KX) 
        PHYMOD_AN_TECH_ABILITY_1G_KX_SET(an_ability_get_type->an_tech_ability);

    /* next check cl73 bam ability */
    if (value.cl73_adv.an_bam_speed & 1 << TEMOD_CL73_BAM_20GBASE_KR2) 
        PHYMOD_BAM_CL73_ABL_20G_KR2_SET(an_ability_get_type->an_tech_ability);
    if (value.cl73_adv.an_bam_speed & 1 << TEMOD_CL73_BAM_20GBASE_CR2) 
        PHYMOD_BAM_CL73_ABL_20G_CR2_SET(an_ability_get_type->an_tech_ability);

    /* check cl37 bam ability */
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_2p5GBASE_X) 
        PHYMOD_BAM_CL37_ABL_2P5G_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_5GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_5G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_6GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_6G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_10GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_10G_HIGIG_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_10GBASE_X4_CX4) 
        PHYMOD_BAM_CL37_ABL_10G_CX4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_10GBASE_X2) 
        PHYMOD_BAM_CL37_ABL_10G_DXGXS_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_10GBASE_X2_CX4) 
        PHYMOD_BAM_CL37_ABL_10G_X2_CX4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_BAM_10p5GBASE_X2) 
        PHYMOD_BAM_CL37_ABL_10P5G_DXGXS_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_12GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_12G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_12p5GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_12P5_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed & 1 << TEMOD_CL37_BAM_12p7GBASE_X2) 
        PHYMOD_BAM_CL37_ABL_12P7_DXGXS_SET(an_ability_get_type->cl37bam_ability);

    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_13GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_13G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_15GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_15G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_15p75GBASE_X2) 
        PHYMOD_BAM_CL37_ABL_12P7_DXGXS_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_16GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_16G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_20GBASE_X4_CX4) 
        PHYMOD_BAM_CL37_ABL_20G_X4_CX4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_20GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_20G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_20GBASE_X2) 
        PHYMOD_BAM_CL37_ABL_20G_X2_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_20GBASE_X2_CX4) 
        PHYMOD_BAM_CL37_ABL_20G_X2_CX4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_21GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_21G_X4_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_25p455GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_25P455G_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_31p5GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_31P5G_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_32p7GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_32P7G_SET(an_ability_get_type->cl37bam_ability);
    if (value.cl37_adv.an_bam_speed1 & 1 << TEMOD_CL37_BAM_40GBASE_X4) 
        PHYMOD_BAM_CL37_ABL_40G_SET(an_ability_get_type->cl37bam_ability);

    return PHYMOD_E_NONE;
}




int tsce_phy_autoneg_set(const phymod_phy_access_t* phy, const phymod_autoneg_control_t* an)
{
    int num_lane_adv_encoded;
    phymod_firmware_lane_config_t firmware_lane_config;
    int start_lane, num_lane, i;
    phymod_phy_access_t phy_copy;
    temod_an_control_t an_control;

    /* next program the tx fir taps and driver current based on the input */
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane = 0x1 << start_lane;

    switch (an->num_lane_adv) {
        case 1:
            num_lane_adv_encoded = 0;
            break;
        case 2:
            num_lane_adv_encoded = 1;
            break;
        case 4:
            num_lane_adv_encoded = 2;
            break;
        case 10:
            num_lane_adv_encoded = 3;
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    an_control.pd_kx4_en = 0;  /* for now  disable */
    an_control.pd_kx_en = 0;   /* for now disable */
    an_control.num_lane_adv = num_lane_adv_encoded;
    an_control.enable       = an->enable;
    an_control.an_property_type = 0x0;   /* for now disable */  
    switch (an->an_mode) {
    case phymod_AN_MODE_CL73:
        an_control.an_type = TEMOD_AN_MODE_CL73;
        break;
    case phymod_AN_MODE_CL37:
        an_control.an_type = TEMOD_AN_MODE_CL37;
        break;
    case phymod_AN_MODE_CL73BAM:
        an_control.an_type = TEMOD_AN_MODE_CL73BAM;
        break;
    case phymod_AN_MODE_CL37BAM:
        an_control.an_type = TEMOD_AN_MODE_CL37BAM;
        break;
    case phymod_AN_MODE_HPAM:
        an_control.an_type = TEMOD_AN_MODE_HPAM;
        break;
    case phymod_AN_MODE_SGMII:
        an_control.an_type = TEMOD_AN_MODE_SGMII;
        break;
    default:
        return PHYMOD_E_PARAM;
    }

    /* make sure the firmware config is set to an eenabled */
    PHYMOD_IF_ERR_RETURN
        (tsce_phy_firmware_lane_config_get(&phy_copy, &firmware_lane_config));
    if (an->enable) {
        firmware_lane_config.AnEnabled = 1;
        firmware_lane_config.LaneConfigFromPCS = 1;
    } else {
        firmware_lane_config.AnEnabled = 0;
        firmware_lane_config.LaneConfigFromPCS = 0;
    }
    for (i = 0; i < num_lane; i++) {
        phy_copy.access.lane = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
            (tsce_phy_firmware_lane_config_set(&phy_copy, firmware_lane_config));
    }

    phy_copy.access.lane = 0x1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (temod_set_an_port_mode(&phy->access, num_lane_adv_encoded, start_lane, 0));
    PHYMOD_IF_ERR_RETURN
        (temod_autoneg_control(&phy_copy.access, &an_control));       
    return PHYMOD_E_NONE;
    
}

int tsce_phy_autoneg_get(const phymod_phy_access_t* phy, phymod_autoneg_control_t* an, uint32_t* an_done)
{
    temod_an_control_t an_control;
    phymod_phy_access_t phy_copy;
    int start_lane, num_lane; 
    int an_complete = 0;

    /* next program the tx fir taps and driver current based on the input */
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane = 0x1 << start_lane;

    PHYMOD_MEMSET(&an_control, 0x0,  sizeof(temod_an_control_t));
    PHYMOD_IF_ERR_RETURN
        (temod_autoneg_control_get(&phy_copy.access, &an_control, &an_complete));
    
    if (an_control.enable) {
        an->enable = 1;
        *an_done = an_complete; 
    } else {
        an->enable = 0;
    }                
    return PHYMOD_E_NONE;
}


int tsce_phy_autoneg_status_get(const phymod_phy_access_t* phy, phymod_autoneg_status_t* status)
{
    /* Place your code here */

        
    return PHYMOD_E_NONE;
}


int tsce_core_init(const phymod_core_access_t* core, const phymod_core_init_config_t* init_config, const phymod_core_status_t* core_status)
{
    phymod_phy_access_t phy_access, phy_access_copy;
    phymod_core_access_t  core_copy;
    phymod_firmware_core_config_t  firmware_core_config_tmp;
#if 0
    temod_an_init_t an; 
#endif

    TSCE_CORE_TO_PHY_ACCESS(&phy_access, core);
    phy_access_copy = phy_access;
    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));
    core_copy.access.lane = 0x1;
	    phy_access_copy = phy_access;
	    phy_access_copy.access = core->access;
	    phy_access_copy.access.lane = 0x1;
	    phy_access_copy.type = core->type;

    PHYMOD_IF_ERR_RETURN
        (temod_pmd_reset_seq(&core_copy.access, core_status->pmd_active));

    /* need to set the heart beat default is for 156.25M */
    if (init_config->interface.ref_clock == phymodRefClk125Mhz) {
        /* we need to set the ref clock config differently */
    }
    if (_tsce_core_firmware_load(&core_copy, init_config->firmware_load_method, init_config->firmware_loader)) {
        PHYMOD_DEBUG_ERROR(("devad 0x%x lane 0x%x: UC firmware-load failed\n", core->access.addr, core->access.lane));  
        PHYMOD_IF_ERR_RETURN ( 1 );
    }
#ifndef NEW_PMD_UCODE
    /* next we need to check if the load is correct or not */
    if (eagle_tsc_ucode_load_verify(&core_copy.access, (uint8_t *) &tsce_ucode, tsce_ucode_len)) {
        PHYMOD_DEBUG_ERROR(("devad 0x%x lane 0x%x: UC load-verify failed\n", core->access.addr, core->access.lane));  
        PHYMOD_IF_ERR_RETURN ( 1 );
    }
#endif /* NEW_PMD_UCODE */

	PHYMOD_IF_ERR_RETURN
		(eagle_pmd_ln_h_rstb_pkill_override( &phy_access_copy.access, 0x1));

    /* next we need to set the uc active and release uc */
    PHYMOD_IF_ERR_RETURN
        (eagle_uc_active_set(&core_copy.access ,1)); 
    /* release the uc reset */
    PHYMOD_IF_ERR_RETURN
        (eagle_uc_reset(&core_copy.access ,1)); 
    /* we need to wait at least 10ms for the uc to settle */
    PHYMOD_USLEEP(10000);

#ifndef NEW_PMD_UCODE
    /* poll the ready bit in 10 ms */
            eagle_tsc_poll_uc_dsc_ready_for_cmd_equals_1(&phy_access_copy.access, 1);
#else
	PHYMOD_IF_ERR_RETURN(
			eagle_tsc_ucode_crc_verify( &core_copy.access, tsce_ucode_len,tsce_ucode_crc));
#endif /* NEW_PMD_UCODE */

	PHYMOD_IF_ERR_RETURN(
		eagle_pmd_ln_h_rstb_pkill_override( &phy_access_copy.access, 0x0));

    /* plldiv CONFIG */
    PHYMOD_IF_ERR_RETURN
        (eagle_pll_mode_set(&core->access, 0xa));

    /* now config the lane mapping and polarity */
    PHYMOD_IF_ERR_RETURN
        (tsce_core_lane_map_set(core, &init_config->lane_map));
#if 0
  /* NEED TO REVISIT */
    PHYMOD_IF_ERR_RETURN
        (temod_autoneg_set_init(&core->access, &an));
#endif
    PHYMOD_IF_ERR_RETURN
        (temod_autoneg_timer_init(&core->access));
    PHYMOD_IF_ERR_RETURN
        (temod_master_port_num_set(&core->access, 0));
    /* don't overide the fw that set in config set if not specified */
    firmware_core_config_tmp = init_config->firmware_core_config;
    firmware_core_config_tmp.CoreConfigFromPCS = 0;
    /* set the vco rate to be default at 10.3125G */
    firmware_core_config_tmp.VcoRate = 0x13;

    PHYMOD_IF_ERR_RETURN
        (tsce_phy_firmware_core_config_set(&phy_access_copy, firmware_core_config_tmp)); 

    /* release core soft reset */
    PHYMOD_IF_ERR_RETURN
        (eagle_core_soft_reset_release(&core_copy.access, 1));
        
    return PHYMOD_E_NONE;
}


int tsce_phy_init(const phymod_phy_access_t* phy, const phymod_phy_init_config_t* init_config)
{
    int pll_restart = 0;
    const phymod_access_t *pm_acc = &phy->access;
    phymod_phy_access_t pm_phy_copy;
    int start_lane, num_lane, i;
    int lane_bkup;
    phymod_polarity_t tmp_pol;

    PHYMOD_MEMSET(&tmp_pol, 0x0, sizeof(tmp_pol));
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    /* next program the tx fir taps and driver current based on the input */
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(pm_acc, &start_lane, &num_lane));
    /* per lane based reset release */
    PHYMOD_IF_ERR_RETURN
        (temod_pmd_x4_reset(pm_acc));

    /* poll for per lane uc_dsc_ready */
    lane_bkup = pm_phy_copy.access.lane;
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
            (eagle_lane_soft_reset_release(&pm_phy_copy.access, 1));
    }
    pm_phy_copy.access.lane = lane_bkup;

    /* program the rx/tx polarity */
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane = 0x1 << (i + start_lane);
        tmp_pol.tx_polarity = (init_config->polarity.tx_polarity) >> i & 0x1;
        tmp_pol.rx_polarity = (init_config->polarity.rx_polarity) >> i & 0x1;
        PHYMOD_IF_ERR_RETURN
            (tsce_phy_polarity_set(&pm_phy_copy, &tmp_pol));
    }

    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
            (tsce_phy_tx_set(&pm_phy_copy, &init_config->tx[i]));
    }

    pm_phy_copy.access.lane = 0x1;

    PHYMOD_IF_ERR_RETURN
        (temod_update_port_mode(pm_acc, &pll_restart));

    PHYMOD_IF_ERR_RETURN
        (temod_rx_lane_control_set(pm_acc, 1));
    PHYMOD_IF_ERR_RETURN
        (temod_tx_lane_control(pm_acc, 1, TEMOD_TX_LANE_RESET_TRAFFIC));         /* TX_LANE_CONTROL */

    return PHYMOD_E_NONE;
}


int tsce_phy_loopback_set(const phymod_phy_access_t* phy, phymod_loopback_mode_t loopback, uint32_t enable)
{
    int start_lane, num_lane;
    int rv = PHYMOD_E_NONE;
    int i = 0;
    phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    switch (loopback) {
    case phymodLoopbackGlobal :
        PHYMOD_IF_ERR_RETURN(temod_tx_loopback_control(&phy->access, enable, start_lane, num_lane));
        break;
    case phymodLoopbackGlobalPMD :
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN(temod_tx_squelch_set(&phy_copy.access, enable));
            PHYMOD_IF_ERR_RETURN(eagle_tsc_dig_lpbk(&phy_copy.access, (uint8_t) enable));
            PHYMOD_IF_ERR_RETURN(eagle_pmd_force_signal_detect(&phy_copy.access, (int) enable));
        }
        break;
    case phymodLoopbackRemotePMD :
        PHYMOD_IF_ERR_RETURN(eagle_tsc_rmt_lpbk(&phy->access, (uint8_t)enable));
        break;
    case phymodLoopbackRemotePCS :
        PHYMOD_IF_ERR_RETURN(temod_rx_loopback_control(&phy->access, enable, enable, enable)); /* RAVI */
        break;
    default :
        break;
    }             
    return rv;
}

int tsce_phy_loopback_get(const phymod_phy_access_t* phy, phymod_loopback_mode_t loopback, uint32_t* enable)
{
    uint32_t enable_core;
    int start_lane, num_lane;

    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    switch (loopback) {
    case phymodLoopbackGlobal :
        PHYMOD_IF_ERR_RETURN(temod_tx_loopback_get(&phy->access, &enable_core));
        *enable = (enable_core >> start_lane) & 0x1; 
        break;
    case phymodLoopbackGlobalPMD :
        PHYMOD_IF_ERR_RETURN(eagle_pmd_loopback_get(&phy->access, enable));
        break;
    case phymodLoopbackRemotePMD :
        /* PHYMOD_IF_ERR_RETURN(eagle_tsc_rmt_lpbk(&phy->access, (uint8_t)enable)); */
        break;
    case phymodLoopbackRemotePCS :
        /* PHYMOD_IF_ERR_RETURN(temod_rx_loopback_control(&phy->access, enable, enable, enable)); */ /* RAVI */
        break;
    default :
        break;
    }             
    return PHYMOD_E_NONE;
}


int tsce_phy_rx_pmd_locked_get(const phymod_phy_access_t* phy, uint32_t* rx_seq_done){
    PHYMOD_IF_ERR_RETURN(temod_pmd_lock_get(&phy->access, rx_seq_done));
    return PHYMOD_E_NONE;
}


int tsce_phy_link_status_get(const phymod_phy_access_t* phy, uint32_t* link_status)
{
    PHYMOD_IF_ERR_RETURN(temod_get_pcs_link_status(&phy->access, link_status));
    return PHYMOD_E_NONE;
}


int tsce_phy_pcs_userspeed_set(const phymod_phy_access_t* phy, const phymod_pcs_userspeed_config_t* config)
{
    int rv ;  
    rv = PHYMOD_E_UNAVAIL ;
   
    switch(config->param){
    case phymodPcsUserSpeedParamEntry:
        /* no need */
        break ;
    case phymodPcsUserSpeedParamHCD:
        /* missing */
        break ;
    case phymodPcsUserSpeedParamClear:
        if(config->mode==phymodPcsUserSpeedModeST) {
            /* set HCD to 0xff */
        }
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_RESET) ;
        break ;
    case phymodPcsUserSpeedParamPllDiv:
        /* not required */
        break ;
    case phymodPcsUserSpeedParamPmaOS:
        /* required ? */
        break ;
    case phymodPcsUserSpeedParamScramble:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_SCR_MODE) ;
        break ;
    case phymodPcsUserSpeedParamEncode:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_ENCODE_MODE) ;
        break ;
    case phymodPcsUserSpeedParamCl48CheckEnd:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_CHKEND_EN) ;
        break ;
    case phymodPcsUserSpeedParamBlkSync:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_BLKSYNC_MODE) ;
        break ;
    case phymodPcsUserSpeedParamReorder:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_REORDER_EN) ;
        break ;
    case phymodPcsUserSpeedParamCl36Enable:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_CL36_EN) ;
        break ;
    case phymodPcsUserSpeedParamDescr1:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_DESCR_MODE) ;
        break ;
    case phymodPcsUserSpeedParamDecode1:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_DECODE_MODE) ;
        break ;
    case phymodPcsUserSpeedParamDeskew:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_DESKEW_MODE) ;
        break ;
    case phymodPcsUserSpeedParamDescr2:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_DESC2_MODE) ;
        break ;
    case phymodPcsUserSpeedParamDescr2ByteDel:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_CL36BYTEDEL_MODE) ;
        break ;
    case phymodPcsUserSpeedParamBrcm64B66:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_BRCM64B66_DESCR_MODE) ;
        break ;
    case phymodPcsUserSpeedParamSgmii:
        rv = TSCE_USERSPEED_CREDIT_SELECT(phy, config, TEMOD_CREDIT_SGMII_SPD) ;
        break ;
    case phymodPcsUserSpeedParamClkcnt0:
        rv = TSCE_USERSPEED_CREDIT_SELECT(phy, config, TEMOD_CREDIT_CLOCK_COUNT_0);
        break ;
    case phymodPcsUserSpeedParamClkcnt1:
        rv = TSCE_USERSPEED_CREDIT_SELECT(phy, config, TEMOD_CREDIT_CLOCK_COUNT_1);
        break ;
    case phymodPcsUserSpeedParamLpcnt0:
        rv = TSCE_USERSPEED_CREDIT_SELECT(phy, config, TEMOD_CREDIT_LOOP_COUNT_0);
        break ;
    case phymodPcsUserSpeedParamLpcnt1:
        rv = TSCE_USERSPEED_CREDIT_SELECT(phy, config, TEMOD_CREDIT_LOOP_COUNT_1);
        break ;
    case phymodPcsUserSpeedParamMacCGC:
        rv = TSCE_USERSPEED_CREDIT_SELECT(phy, config, TEMOD_CREDIT_MAC);
        break ;
    case phymodPcsUserSpeedParamRepcnt:
        rv = TSCE_USERSPEED_CREDIT_SELECT(phy, config, TEMOD_CREDIT_PCS_REPCNT);
        break ;
    case phymodPcsUserSpeedParamCrdtEn:
        rv = TSCE_USERSPEED_CREDIT_SELECT(phy, config, TEMOD_CREDIT_EN) ;
        break ;
    case phymodPcsUserSpeedParamPcsClkcnt:
        rv = TSCE_USERSPEED_CREDIT_SELECT(phy, config, TEMOD_CREDIT_PCS_CLOCK_COUNT_0);
        break ;
    case phymodPcsUserSpeedParamPcsCGC:
        rv = TSCE_USERSPEED_CREDIT_SELECT(phy, config, TEMOD_CREDIT_PCS_GEN_COUNT);
        break ;
    case phymodPcsUserSpeedParamCl72En:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_CL72) ;
        break ;
    case phymodPcsUserSpeedParamNumOfLanes:
        rv = TSCE_USERSPEED_SELECT(phy, config, TEMOD_OVERRIDE_NUM_LANES) ;
        break ;
    default:
        break ;
    }
    return rv;
}

int tsce_phy_pcs_userspeed_get(const phymod_phy_access_t* phy, const phymod_pcs_userspeed_config_t* config)
{
    return PHYMOD_E_UNAVAIL;
}


int tsce_phy_status_dump(const phymod_phy_access_t* phy)
{
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_display_core_state(&phy->access));
    PHYMOD_IF_ERR_RETURN
        (eagle_tsc_display_lane_debug_status(&phy->access));

    return PHYMOD_E_NONE;
}

int tsce_phy_reg_read(const phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t *val)
{
    PHYMOD_IF_ERR_RETURN(phymod_tsc_iblk_read(&phy->access, reg_addr, val));
    return PHYMOD_E_NONE;
}


int tsce_phy_reg_write(const phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t val)
{
    PHYMOD_IF_ERR_RETURN(phymod_tsc_iblk_write(&phy->access, reg_addr, val));
    return PHYMOD_E_NONE;  
}


#endif /* PHYMOD_TSCE_SUPPORT */
