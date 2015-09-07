/*
 * $Id: xgxs.c 1.58 Broadcom SDK $
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
 * File:        xgxs.c
 * Purpose:     Defines common PHY driver routines for Broadcom XGXS core.
 */

#include <sal/types.h>
#include <sal/core/spl.h>

#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/phyreg.h>

#include <soc/phy.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>

#include "phydefs.h"      /* Must include before other phy related includes */

#if defined(INCLUDE_PHY_XGXS)
#include "phyconfig.h"    /* Must be the first phy include after phydefs.h */
#include "phyreg.h"
#include "xgxs.h"

/*
 *      phy_xgxs_reg_read
 * Purpose:
 *      Routine to read PHY register
 * Parameters:
 *      uint         - BCM unit number
 *      pc           - PHY state
 *      flags        - Flags which specify the register type
 *      phy_reg_addr - Encoded register address
 *      phy_data     - (OUT) Value read from PHY register
 * Note:
 *      This register read function is not thread safe. Higher level
 * function that calls this function must obtain a per port lock
 * to avoid overriding register page mapping between threads.
 */
int
phy_xgxs_reg_read(int unit, soc_port_t port, uint32 flags,
                  uint32 phy_reg_addr, uint32 *phy_data)
{
    uint16               reg_bank;
    uint8                reg_addr;
    uint16               data;     /* Temporary holder for phy_data */
    phy_ctrl_t           *pc;      /* PHY software state */

    COMPILER_REFERENCE(flags);

    pc   = INT_PHY_SW_STATE(unit, port);

    reg_bank  = SOC_PHY_REG_BANK(phy_reg_addr);
    reg_addr  = SOC_PHY_REG_ADDR(phy_reg_addr);

    SOC_IF_ERROR_RETURN
        (phy_reg_xgxs_read(unit, pc, reg_bank, reg_addr, &data));

   *phy_data = data;

    return SOC_E_NONE;
}
/*
 * Function:
 *      phy_xgxs_reg_write
 * Purpose:
 *      Routine to write PHY register
 * Parameters:
 *      uint         - BCM unit number
 *      pc           - PHY state
 *      flags        - Flags which specify the register type
 *      phy_reg_addr - Encoded register address
 *      phy_data     - Value write to PHY register
 * Note:
 *      This register read function is not thread safe. Higher level
 * function that calls this function must obtain a per port lock
 * to avoid overriding register page mapping between threads.
 */
int
phy_xgxs_reg_write(int unit, soc_port_t port, uint32 flags,
                   uint32 phy_reg_addr, uint32 phy_data)
{
    uint16               reg_bank;
    uint8                reg_addr;
    uint16               data;     /* Temporary holder for phy_data */
    phy_ctrl_t           *pc;      /* PHY software state */

    COMPILER_REFERENCE(flags);

    pc   = INT_PHY_SW_STATE(unit, port);
    data = (uint16) (phy_data & 0x0000FFFF);

    reg_bank  = SOC_PHY_REG_BANK(phy_reg_addr);
    reg_addr  = SOC_PHY_REG_ADDR(phy_reg_addr);

    return phy_reg_xgxs_write(unit, pc, reg_bank, reg_addr, data);
} 
/*
 * Function:
 *      phy_xgxs_reg_modify
 * Purpose:
 *      Routine to write PHY register
 * Parameters:
 *      uint         - BCM unit number
 *      pc           - PHY state
 *      flags        - Flags which specify the register type
 *      phy_reg_addr - Encoded register address
 *      phy_mo_data  - New value for the bits specified in phy_mo_mask
 *      phy_mo_mask  - Bit mask to modify
 * Note:
 *      This register read function is not thread safe. Higher level
 * function that calls this function must obtain a per port lock
 * to avoid overriding register page mapping between threads.
 */
int
phy_xgxs_reg_modify(int unit, soc_port_t port, uint32 flags,
                    uint32 phy_reg_addr, uint32 phy_data,
                    uint32 phy_data_mask)
{
    uint16               reg_bank;
    uint8                reg_addr;
    uint16               data;     /* Temporary holder for phy_data */
    uint16               mask;     /* Temporary holder for phy_data_mask */
    phy_ctrl_t           *pc;      /* PHY software state */

    COMPILER_REFERENCE(flags);

    pc   = INT_PHY_SW_STATE(unit, port);
    data = (uint16) (phy_data & 0x0000FFFF);
    mask = (uint16) (phy_data_mask & 0x0000FFFF);

    reg_bank  = SOC_PHY_REG_BANK(phy_reg_addr);
    reg_addr  = SOC_PHY_REG_ADDR(phy_reg_addr);

    return phy_reg_xgxs_modify(unit, pc, reg_bank, reg_addr, data, mask);
}

#else /* INCLUDE_PHY_XGXS */
int _phy_xgxs_not_empty;
#endif /* INCLUDE_PHY_XGXS */

