/*
 * $Id: fcmapphy.c 1.6 Broadcom SDK $
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
 * StrataSwitch FCMAP PHY control
 * FCMAP PHY initialization 
 */


#include <soc/drv.h>
#include <soc/phy/phyctrl.h>
#include <soc/debug.h>

#ifdef INCLUDE_FCMAP
#include <bfcmap.h>
#ifdef BCM_SBX_SUPPORT
#include <soc/sbx/sbx_drv.h>
#endif
#include <soc/fcmapphy.h>


/*
 * Function:
 *      soc_fcmapphy_init
 * Purpose:
 *      Initializes the FCMAP phy port.
 * Parameters:
 *      unit - SOC Unit #.
 *      port - Port number.
 *      pc  - Phy control structure
 * Returns:
 *      SOC_E_XXX
 */
int
soc_fcmapphy_init(int unit, soc_port_t port, phy_ctrl_t *pc, 
                   bfcmap_core_t core_type, bfcmap_dev_io_f iofn)
{
    int                     /* ii, */ p, rv;
    int                     fcmap_enable = 0;

    p = SOC_FCMAP_PORTID(unit, port);
    /* Destroy the port if it were created before. */
    (void)bfcmap_port_destroy(p);

    fcmap_enable = soc_property_port_get(unit, port, spn_FCMAP_ENABLE, 0);
    pc->fcmap_enable = fcmap_enable;

    if (fcmap_enable) {
        /* Create FCMAP Port */
        rv = bfcmap_port_create(p, core_type, pc->fcmap_dev_addr, 
                                 pc->fcmap_uc_dev_addr, 
                                 pc->fcmap_dev_port, iofn);
        if (rv != BFCMAP_E_NONE) {
            soc_cm_debug(DK_WARN, "soc_fcmapphy_init: "
                        "FCMAP port create failed for u=%d p=%d rv = %d\n",
                        unit, port, rv);
            return SOC_E_FAIL;
        }
    }
    return SOC_E_NONE;
}

int 
soc_fcmapphy_miim_read(soc_fcmap_dev_addr_t dev_addr, 
                        uint32 phy_reg_addr, uint16 *data)
{
    int unit, phy_id, clause45;
    int rv = SOC_E_NONE;

    /*
     * Decode dev_addr into phyid and unit.
     */
    unit = SOC_FCMAPPHY_ADDR2UNIT(dev_addr);
    phy_id = SOC_FCMAPPHY_ADDR2MDIO(dev_addr);
    clause45 = SOC_FCMAPPHY_ADDR_IS_CLAUSE45(dev_addr);

#ifdef BCM_ESW_SUPPORT
    if (SOC_IS_ESW(unit)) {
        if (clause45) {
            rv = soc_esw_miimc45_read(unit, phy_id, 
                                  phy_reg_addr, data);
        } else {
            rv = soc_miim_read(unit, phy_id, phy_reg_addr, data);
        }
    }
#endif /* BCM_ESW_SUPPORT */

#ifdef BCM_SBX_SUPPORT
    if (SOC_IS_SBX(unit)) {
        if (clause45) {
            rv = soc_sbx_miimc45_read(unit, phy_id, 
                                      phy_reg_addr, data);
        } else {
            rv = soc_sbx_miim_read(unit, phy_id, phy_reg_addr, data);
        }
    }
#endif /* BCM_SBX_SUPPORT */

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(unit)) {
        rv = soc_robo_miim_read(unit, phy_id, phy_reg_addr, data);
    }
#endif /* BCM_ROBO_SUPPORT */

    return (rv == SOC_E_NONE) ? 0 : -1;
}

int 
soc_fcmapphy_miim_write(soc_fcmap_dev_addr_t dev_addr, 
                         uint32 phy_reg_addr, uint16 data)
{
    int unit, phy_id, clause45;
    int rv = SOC_E_NONE;

    /*
     * Decode dev_addr into phyid and unit.
     */
    unit = SOC_FCMAPPHY_ADDR2UNIT(dev_addr);
    phy_id = SOC_FCMAPPHY_ADDR2MDIO(dev_addr);

    clause45 = SOC_FCMAPPHY_ADDR_IS_CLAUSE45(dev_addr);

#ifdef BCM_ESW_SUPPORT
    if (SOC_IS_ESW(unit)) {
        if (clause45) {
            rv = soc_esw_miimc45_write(unit, phy_id, 
                                   phy_reg_addr, data);
        } else {
            rv = soc_miim_write(unit, phy_id, phy_reg_addr, data);
        }
    }
#endif /* BCM_ESW_SUPPORT */

#ifdef BCM_SBX_SUPPORT
    if (SOC_IS_SBX(unit)) {
        if (clause45) {
            rv = soc_sbx_miimc45_write(unit, phy_id, 
                                      phy_reg_addr, data);
        } else {
            rv = soc_sbx_miim_write(unit, phy_id, phy_reg_addr, data);
        }
    }
#endif /* BCM_SBX_SUPPORT */

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(unit)) {
        rv = soc_robo_miim_write(unit, phy_id, phy_reg_addr, data);
    }
#endif /* BCM_ROBO_SUPPORT */

    return (rv == SOC_E_NONE) ? 0 : -1;
}

#else

int 
soc_fcmapphy_init(int unit, soc_port_t port, void *pc)
{
    return SOC_E_NONE;
}
#endif /* INCLUDE_FCMAP */

