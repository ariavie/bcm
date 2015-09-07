/*----------------------------------------------------------------------
 * $Id: temod_cfg_seq.h,v 1.1.2.2 Broadcom SDK $ 
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
 * File       : tqmod_cfg_seq.h
 * Description: c functions implementing Tier1s for TEMod Serdes Driver
 *---------------------------------------------------------------------*/

#ifndef TQMOD_CFG_SEQ_H 
#define TQMOD_CFG_SEQ_H
#include "../../tsce/tier1/temod_enum_defines.h"
#include "../../tsce/tier1/temod.h"
typedef enum {
    TQMOD_TX_LANE_TRAFFIC = 0,  
    TQMOD_TX_LANE_RESET = 2,
    TQMOD_TX_LANE_RESET_TRAFFIC = 4,
    TQMOD_TX_LANE_TYPE_COUNT
}txq_lane_disable_type_t;

typedef struct tqmod_an_control_s {
  temod_an_mode_type_t an_type; 
  uint16_t enable;
  an_property_enable  an_property_type;
} tqmod_an_control_t;


int tqmod_set_spd_intf(const phymod_access_t *pa, int speed);                    /* SET_SPD_INTF */
int tqmod_init_pmd_qsgmii(const phymod_access_t *pa, int pmd_active, int uc_active); /* INIT_PMD */
int tqmod_init_pcs_ilkn(const phymod_access_t *pa);                                  /* INIT_PCS */

typedef struct tqmod_an_adv_ability_s{
  temod_an_pause_t an_pause; 
  temod_cl37_sgmii_speed_t cl37_sgmii_speed;
}tqmod_an_adv_ability_t;


typedef struct tqmod_an_ability_s {
  tqmod_an_adv_ability_t cl37_adv; /*includes cl37 and cl37-bam related (fec, cl72) */
} tqmod_an_ability_t;


/*link status*/
int tqmod_get_pcs_link_status(const phymod_access_t* pa, uint32_t *link);

/* get_speed*/
int tqmod_speed_id_get(const phymod_access_t *pa, int *speed_id);

/* lane_enable*/
int tqmod_lane_enable_set(const phymod_access_t *pa, int enable);

/* set osmode*/
int tqmod_pmd_osmode_set(const phymod_access_t *pa, int os_mode);

/* set autoneg*/
int tqmod_autoneg_set(const phymod_access_t *pa, int *an_en);

/* pmd_per_lane reset*/
int tqmod_pmd_x4_reset(const phymod_access_t *pa);

int tqmod_rx_lane_control_set(const phymod_access_t *pa, int enable);

int tqmod_tx_lane_control_set(const phymod_access_t *pa, int enable);

int tqmod_tx_rx_polarity_get (const phymod_access_t *pa, uint32_t* txp, uint32_t* rxp);

int tqmod_tx_rx_polarity_set (const phymod_access_t *pa, uint32_t txp, uint32_t rxp);

int tqmod_tx_lane_control(const phymod_access_t *pa, int enable, txq_lane_disable_type_t tx_dis_type);

int tqmod_txfir_tx_disable_set(const phymod_access_t *pa);

#endif /* TQMOD_CFG_SEQ_H */ 
