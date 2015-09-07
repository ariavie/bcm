/*
 *         
 * $Id: phymod_definitions.h,v 1.2.2.12 Broadcom SDK $
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
 * Shell diagnostics of Phymod    
 *
 */

#ifndef _PHYMOD_DIAG_H_
#define _PHYMOD_DIAG_H_

#include <phymod/phymod_diagnostics.h>


/******************************************************************************
 Typedefs
******************************************************************************/
typedef int (*print_func_f)(const char *, ...);

typedef enum{
    PhymodDiagPrbsClear,
    PhymodDiagPrbsSet,
    PhymodDiagPrbsGet
}phymod_diag_prbs_operation_t;

typedef struct phymod_diag_prbs_set_args_s{
    uint32_t flags;
    phymod_prbs_t prbs_options;
    uint32_t enable;
    uint32_t loopback;
}phymod_diag_prbs_set_args_t;

typedef struct phymod_diag_prbs_get_args_s{
    uint32_t time;
}phymod_diag_prbs_get_args_t;

typedef struct phymod_diag_prbs_clear_args_s{
    uint32_t flags;
}phymod_diag_prbs_clear_args_t;


typedef union phymod_diag_prbs_command_args_u{
    phymod_diag_prbs_set_args_t set_params;
    phymod_diag_prbs_get_args_t get_params;
    phymod_diag_prbs_clear_args_t clear_params;
}phymod_prbs_command_args_t;

typedef struct phymod_diag_prbs_args_s{
    phymod_diag_prbs_operation_t prbs_cmd;
    phymod_prbs_command_args_t args;
}phymod_diag_prbs_args_t;


/******************************************************************************
Functions
******************************************************************************/

extern print_func_f phymod_diag_print_func;

int phymod_diag_firmware_mode_set(phymod_core_access_t *cores, int array_size, phymod_firmware_lane_config_t fw_config);
int phymod_diag_firmware_load(phymod_core_access_t *cores, int array_size, char *firwmware_file);
int phymod_diag_prbs(phymod_phy_access_t *phys, int array_size, phymod_diag_prbs_args_t *prbs_diag_args);
int phymod_diag_reg_write(phymod_phy_access_t *phys, int array_size, uint32_t reg_addr, uint32_t val);
int phymod_diag_reg_read(phymod_phy_access_t *phys, int array_size, uint32_t reg_addr);
int phymod_diag_dsc(phymod_phy_access_t *phys, int array_size);


#endif /*_PHYMOD_DIAG_H_*/
