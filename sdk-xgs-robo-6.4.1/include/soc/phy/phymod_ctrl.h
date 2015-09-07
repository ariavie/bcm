/*
 * $Id: phymod_ctrl.h,v 1.1.2.7 Broadcom SDK $
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
 */

#ifndef _PHY_PHYMOD_CTRL_H_
#define _PHY_PHYMOD_CTRL_H_

#include <soc/types.h>
#include <phymod/phymod.h>
#include <phymod/phymod_symbols.h>

/*
 * Generic object for managing cores and lanes.
 */
typedef struct soc_phy_obj_s {
    struct soc_phy_obj_s *next;
    int obj_id;
    void *owner;
} soc_phy_obj_t;

/*
 * Core object
 *
 * The core object is a physical entity which is independent of other
 * physical entities.
 */
typedef struct soc_phymod_core_s {
    soc_phy_obj_t obj;
    phymod_core_access_t pm_core;
    phymod_bus_t pm_bus;
    int ref_cnt;
    int init;
    int unit;
    int port;
    int (*read)(int, uint32, uint32, uint16*);
    int (*write)(int, uint32, uint32, uint16);
    int (*wrmask)(int, uint32, uint32, uint16, uint16);
    void *device_aux_modes ;
    phymod_core_init_config_t init_config;
} soc_phymod_core_t;

/*
 * PHY object
 *
 * A PHY is one or more lanes belonging to the same core object. Most
 * PHY hardware is capable of configuring multiple lanes
 * simultaneously, so we do not create an object for each lane of a
 * logical port.
 */
typedef struct soc_phymod_phy_s {
    soc_phy_obj_t obj;
    phymod_phy_access_t pm_phy;
    soc_phymod_core_t *core;
    phymod_phy_init_config_t  init_config;
} soc_phymod_phy_t;

/*
 * Logical port object
 *
 * This object represents a logical port as seen from the MAC (and
 * usually also the application). It consists of one or more lane
 * groups, depending on how many core objects the logical port spans.
 */
typedef struct soc_phymod_ctrl_s {
    int unit;
    int num_phys;
    soc_phymod_phy_t *phy[PHYMOD_CONFIG_MAX_CORES_PER_PORT];
    phymod_symbols_t *symbols;
    void (*cleanup)(struct soc_phymod_ctrl_s*);
} soc_phymod_ctrl_t;


extern int
soc_phymod_miim_bus_read(int unit, uint32_t addr, uint32_t reg, uint32_t *data);

extern int
soc_phymod_miim_bus_write(int unit, uint32_t addr, uint32_t reg, uint32_t data);

extern int
soc_phymod_core_create(int unit, int core_id, soc_phymod_core_t **core);

extern int
soc_phymod_core_destroy(int unit, soc_phymod_core_t *core);

extern int
soc_phymod_core_find_by_id(int unit, int core_id, soc_phymod_core_t **core);

extern int
soc_phymod_phy_create(int unit, int phy_id, soc_phymod_phy_t **phy);

extern int
soc_phymod_phy_destroy(int unit, soc_phymod_phy_t *phy);

extern int
soc_phymod_phy_find_by_id(int unit, int phy_id, soc_phymod_phy_t **phy);

#endif /* _PHY_PHYMOD_CTRL_H_ */
