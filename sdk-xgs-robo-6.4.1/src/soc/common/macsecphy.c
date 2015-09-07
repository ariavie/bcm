/*
 * $Id: macsecphy.c,v 1.65 Broadcom SDK $
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
 * StrataSwitch MACSEC PHY control
 * MACSEC PHY initialization 
 */


#include <shared/bsl.h>

#include <soc/drv.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>
#include <soc/debug.h>

#ifdef INCLUDE_MACSEC 
#include <bmacsec.h>
#ifdef BCM_SWITCHMACSEC_SUPPORT
#include <dummyphy.h>
#endif  /* BCM_SWITCHMACSEC_SUPPORT */
#if defined(INCLUDE_PHY_8729)
#include <phy8729.h>
#endif
#ifdef BCM_SBX_SUPPORT
#include <soc/sbx/sbx_drv.h>
#endif
#include <soc/macsecphy.h>


/* speed_get register */
typedef int (*bmacsec_speed_get_f) (void * ext_port_handle, int * speed);

extern void 
_bmacsec_register_speed_get(int port, bmacsec_speed_get_f f, void *pc);



int soc_macsecphy_speed_get(void * ext_port_handle, int * speed) {

    int                     rv;
    phy_ctrl_t *pc = (phy_ctrl_t *) ext_port_handle;

    if (speed == NULL) {
        return  -1;
    }

    *speed = 0;

    if (pc) {
        rv = PHYCTRL_SPEED_GET(pc, pc->unit, pc->port, speed);
        if (rv != SOC_E_NONE) {
            *speed = 0;
            return  -1;
        }
    }

    return 0;
}


/*
 * Function:
 *      soc_macsecphy_init
 * Purpose:
 *      Initializes the MACSEC phy port.
 * Parameters:
 *      unit - SOC Unit #.
 *      port - Port number.
 *      pc  - Phy control structure
 * Returns:
 *      SOC_E_XXX
 */
int
soc_macsecphy_init(int unit, soc_port_t port, phy_ctrl_t *pc, 
                   bmacsec_core_t core_type, bmacsec_dev_io_f iofn)
{
    int                     /* ii, */ p, rv;
    int                     macsec_enable = 0;

    p = SOC_MACSEC_PORTID(unit, port);
    /* Destroy the port if it were created before. */
    (void)bmacsec_port_destroy(p);

    macsec_enable = soc_property_port_get(unit, port, spn_MACSEC_ENABLE, 0);
    pc->macsec_enable = macsec_enable;

    if (macsec_enable) {
        /* Create MACSEC Port */
        rv = bmacsec_port_create(p, core_type, pc->macsec_dev_addr, 
                                 pc->macsec_dev_port, iofn);
        if (rv != BMACSEC_E_NONE) {
            LOG_WARN(BSL_LS_SOC_COMMON,
                     (BSL_META_U(unit,
                                 "soc_macsecphy_init: "
                                 "MACSEC port create failed for u=%d p=%d rv = %d\n"),
                      unit, port, rv));
            return SOC_E_FAIL;
        }

        /* register speed_get */
        _bmacsec_register_speed_get(p, soc_macsecphy_speed_get, (void *) pc);
    }
    return SOC_E_NONE;
}

int 
soc_macsecphy_miim_read(soc_macsec_dev_addr_t dev_addr, 
                        uint32 phy_reg_addr, uint16 *data)
{
    int unit, phy_id, clause45;
    int rv = SOC_E_NONE;
#ifdef BCM_SWITCHMACSEC_SUPPORT
    int nophy_macsec = 0;
#endif /* BCM_SWITCHMACSEC_SUPPORT */

    /*
     * Decode dev_addr into phyid and unit.
     */
    unit = SOC_MACSECPHY_ADDR2UNIT(dev_addr);
    phy_id = SOC_MACSECPHY_ADDR2MDIO(dev_addr);
#ifdef BCM_SWITCHMACSEC_SUPPORT
    nophy_macsec = SOC_MACSECPHY_ADDR_IS_FLAG_NOPHY(dev_addr);;
    clause45 = SOC_MACSECPHY_ADDR_IS_FLAG_CLAUSE45(dev_addr);
#else /* BCM_SWITCHMACSEC_SUPPORT */
    clause45 = SOC_MACSECPHY_ADDR_IS_CLAUSE45(dev_addr);
#endif /* BCM_SWITCHMACSEC_SUPPORT */

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
        if (clause45) {
            LOG_WARN(BSL_LS_SOC_COMMON,
                     (BSL_META_U(unit,
                                 "%s: Clause45 is currently not supported!\n"),
                      FUNCTION_NAME()));
        }
#ifdef BCM_SWITCHMACSEC_SUPPORT
        if (nophy_macsec){
            rv = soc_robo_macsec_miim_read(unit, phy_id, phy_reg_addr, data);;
        } else {
            rv = soc_robo_miim_read(unit, phy_id, phy_reg_addr, data);
        }
#else /* BCM_SWITCHMACSEC_SUPPORT */
        rv = soc_robo_miim_read(unit, phy_id, phy_reg_addr, data);
#endif /* BCM_SWITCHMACSEC_SUPPORT */
    }
#endif /* BCM_ROBO_SUPPORT */

    return (rv == SOC_E_NONE) ? 0 : -1;
}

int 
soc_macsecphy_miim_write(soc_macsec_dev_addr_t dev_addr, 
                         uint32 phy_reg_addr, uint16 data)
{
    int unit, phy_id, clause45;
    int rv = SOC_E_NONE;
#ifdef BCM_SWITCHMACSEC_SUPPORT
    int     nophy_macsec = 0;
#endif /* BCM_SWITCHMACSEC_SUPPORT */

    /*
     * Decode dev_addr into phyid and unit.
     */
    unit = SOC_MACSECPHY_ADDR2UNIT(dev_addr);
    phy_id = SOC_MACSECPHY_ADDR2MDIO(dev_addr);

#ifdef BCM_SWITCHMACSEC_SUPPORT
    nophy_macsec = SOC_MACSECPHY_ADDR_IS_FLAG_NOPHY(dev_addr);;
    clause45 = SOC_MACSECPHY_ADDR_IS_FLAG_CLAUSE45(dev_addr);
#else /* BCM_SWITCHMACSEC_SUPPORT */
    clause45 = SOC_MACSECPHY_ADDR_IS_CLAUSE45(dev_addr);
#endif /* BCM_SWITCHMACSEC_SUPPORT */

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
        if (clause45) {
            LOG_WARN(BSL_LS_SOC_COMMON,
                     (BSL_META_U(unit,
                                 "%s: Clause45 is currently not supported!\n"),
                      FUNCTION_NAME()));
        }
#ifdef BCM_SWITCHMACSEC_SUPPORT
        if (nophy_macsec){
            rv = soc_robo_macsec_miim_write(unit, phy_id, phy_reg_addr, data);;
        } else {
            rv = soc_robo_miim_write(unit, phy_id, phy_reg_addr, data);
        }
#else /* BCM_SWITCHMACSEC_SUPPORT */
        rv = soc_robo_miim_write(unit, phy_id, phy_reg_addr, data);
#endif /* BCM_SWITCHMACSEC_SUPPORT */
    }
#endif /* BCM_ROBO_SUPPORT */

    return (rv == SOC_E_NONE) ? 0 : -1;
}


#ifdef BCM_SWITCHMACSEC_SUPPORT
int 
soc_switchmacsec_init(int unit, soc_port_t port)
{
    int     rv = SOC_E_NONE;
    int     addr_flags = 0, macsec_devid = -1;    
    int     mmi_mdio_addr, port_index, macsec_enable, macsec_dev_port_cnt;
    phy_ctrl_t          *pc;
    bmacsec_core_t      macsec_core = BMACSEC_CORE_UNKNOWN;
    bmacsec_dev_io_f    macsec_mmi_io = NULL;
    bmacsec_mactype_t   macsec_mac = BMACSEC_MAC_CORE_UNKNOWN;
    bmacsec_dev_addr_t  macsec_dev_addr, macsec_dev_idx;
    
    LOG_INFO(BSL_LS_SOC_PHY,
             (BSL_META_U(unit,
                         "%s: u=%d port=%d\n"),
              FUNCTION_NAME(), unit, port));

    SOC_NOPHY_MACSEC_DEVID_GET(unit, port, macsec_devid);
    if (macsec_devid == -1) {
        return SOC_E_NONE; 
    }
    
    pc = EXT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        pc = INT_PHY_SW_STATE(unit, port);
        if (pc == NULL) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "%s: no PHY driver attached on u=%d p=%d\n"), 
                       FUNCTION_NAME(), unit, port));
            return SOC_E_CONFIG;
        }
    }

    mmi_mdio_addr = soc_property_port_get(unit, port, spn_MACSEC_DEV_ADDR, -1);
    if (mmi_mdio_addr == -1) {
        LOG_WARN(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "%s: MACSEC_DEV_ADDR property not configured for u=%d p=%d\n"), 
                  FUNCTION_NAME(), unit, port));
        return SOC_E_CONFIG;
    }
    
    port_index = soc_property_port_get(unit, port, spn_MACSEC_PORT_INDEX, -1);
    if (port_index == -1) {
        LOG_WARN(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "%s: MACSEC_PORT_INDEX property not configured for u=%d p=%d\n"), 
                  FUNCTION_NAME(), unit, port));
        return SOC_E_CONFIG;
    }

    macsec_enable = soc_property_port_get(unit, port, spn_MACSEC_ENABLE, 0);

    pc->macsec_dev_addr = mmi_mdio_addr;
    pc->macsec_dev_port = port_index;
    pc->macsec_enable = macsec_enable;
    
    addr_flags = SOC_MACSECPHY_MDIO_FLAGS_NOPHY;
    macsec_dev_idx = SOC_MACSECPHY_MDIO_ADDR(unit, macsec_devid, addr_flags); 
    macsec_dev_addr = SOC_MACSECPHY_MDIO_ADDR(unit, mmi_mdio_addr, addr_flags);

    /* core type and LMI/MMI io function assignment :
     * - The core type, MAC type, macsec_dev_port_cnt and LMI/MMI io-function in current SDK is force assigned.
     */
    SOC_NOPHY_MACSEC_CORE_GET(unit, macsec_core, macsec_dev_port_cnt);
    SOC_NOPHY_MACSEC_MAC_GET(unit, macsec_mac);
    SOC_NOPHY_MACSEC_MMI_GET(unit, macsec_mmi_io);

    /* core type and LMI/MMI io function assignment :
     * - The core type and LMI/MMI io-function in current SDK is force assigned.
     */
    /*
     * COVERITY
     *
     *  This default is unreachable in some conditional builds. 
     *  (Currently the SwitchMACSEC(MACSEC-in-Switch) is supported in ROBO 
     *  switch only)
     *  
     *  To keep this design for the supporting ability while different chip 
     *  Arch.(like ESW) in future has SwitchMACSEC chip design.
     */
    /* coverity[dead_error_begin] */
    if ((macsec_core == BMACSEC_CORE_UNKNOWN) || (macsec_dev_port_cnt <= 0) || \
            (macsec_mac == BMACSEC_MAC_CORE_UNKNOWN) || \
            (macsec_mmi_io == NULL)) {
        LOG_WARN(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "%s:MACSEC core, MAC or LMI iofu is not predefined!\n"),
                  FUNCTION_NAME()));
    }

    /* call to bmacsec_dummyphy_phy_mac_addr_set() for assigning those macsec 
     * related address and id for MDIO access.
     *  Flow :
     *  1. assigning address mapping table between PHY/MAC/port.
     *  2. attaching MAC driver. (in UNIMAC/BIGMAC/XMAC)
     */
    rv = bmacsec_dummyphy_mac_addr_set(macsec_dev_idx, macsec_dev_addr,
           port_index, macsec_dev_port_cnt, macsec_mmi_io, macsec_mac); 
    if(rv != BMACSEC_E_NONE) {
        LOG_WARN(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "%s: bmacsec_phy_mac_addr_set returned error for u=%d p=%d\n"), 
                  FUNCTION_NAME(), unit, port));
        return SOC_E_INTERNAL;
    }

    if (macsec_enable) {
        /* init MACSEC's Line/Switch MAC */
        rv = bmacsec_dummyphy_mac_init(macsec_dev_idx, macsec_enable);
        if(rv != BMACSEC_E_NONE) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "%s: failed on init MACSEC's MAC for u=%d p=%d\n"), 
                       FUNCTION_NAME(), unit, port));
            return SOC_E_INTERNAL;
        }
    }
    
    /* set MACSEC bypass control */
    rv = soc_macsecphy_dev_control_set(unit, port, 
            SOC_MACSECPHY_DEV_CONTROL_BYPASS, (macsec_enable ? FALSE : TRUE));
    if (!SOC_SUCCESS(rv)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "%s: failed on setting MACSEC Bypass for u=%d p=%d\n"), 
                   FUNCTION_NAME(), unit, port));
        return SOC_E_INTERNAL;
    }

    /* performing common macsecphy init process */
    rv = soc_macsecphy_init(unit, port, pc, macsec_core, macsec_mmi_io);
 
    if (!SOC_SUCCESS(rv)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_switchmacsec_init: MACSEC init for"
                              " u=%d p=%d FAILED\n"), unit, port));
    } else {
        LOG_INFO(BSL_LS_SOC_PHY,
                 (BSL_META_U(unit,
                             "soc_switchmacsec_init: MACSEC init for"
                             " u=%d p=%d SUCCESS\n"), unit, port));
    }
    return rv;
    
}

#endif /* BCM_SWITCHMACSEC_SUPPORT */

/* The routine for control set/get is for the setting items which are unable 
 *  to be accessed in MACSEC's internal registers. Normally, this is for the 
 *  control on whole MACSEC unit like bypass, power-down or reset etc.
 *  
 *  This request normally is for MACSEC-in-SWITCH solution
 *  (i.e. NOPHY_MACSEC). And the reasons is MACSEC-in-PHY solution for the 
 *  purpose to control whole MACSEC unit can through PHY expanded regsiter. 
 *
 *  In current existed SDK solution(MACSEC-in-PHY solution), such contorl 
 *  set/get all implemented in each independent PHY driver already.
 */
int 
soc_macsecphy_dev_control_set(int unit, soc_port_t port, int type, uint32 value)
{
    int rv = SOC_E_NONE;
    
    switch (type) {
    case SOC_MACSECPHY_DEV_CONTROL_BYPASS:
#ifdef BCM_ROBO_SUPPORT
        if (SOC_IS_ROBO(unit)) {
            rv = soc_robo_macsec_bypass_set(unit, port, value);
        } else {
            rv = SOC_E_UNAVAIL;
        }
#else   /* BCM_ROBO_SUPPORT */
        rv = SOC_E_UNAVAIL;
#endif  /* BCM_ROBO_SUPPORT */
        break;
    case SOC_MACSECPHY_DEV_CONTROL_POWERDOWN:
    case SOC_MACSECPHY_DEV_CONTROL_RESET:
        rv = SOC_E_UNAVAIL;
        break;
    default:
        rv = SOC_E_PARAM;
    }
    
    return rv;
}

int 
soc_macsecphy_dev_control_get(int unit, soc_port_t port, int type, uint32 *value)
{
    int rv = SOC_E_NONE;
    
    switch (type) {
    case SOC_MACSECPHY_DEV_CONTROL_BYPASS:
#ifdef BCM_ROBO_SUPPORT
        if (SOC_IS_ROBO(unit)) {
            rv = soc_robo_macsec_bypass_get(unit, port, value);
        } else {
            rv = SOC_E_UNAVAIL;
        }
#else   /* BCM_ROBO_SUPPORT */
        rv = SOC_E_UNAVAIL;
#endif  /* BCM_ROBO_SUPPORT */
        break;
    case SOC_MACSECPHY_DEV_CONTROL_POWERDOWN:
    case SOC_MACSECPHY_DEV_CONTROL_RESET:
        rv = SOC_E_UNAVAIL;
        break;
    default:
        rv = SOC_E_PARAM;
    }
    
    return rv;
}

/* Function : soc_macsecphy_dev_info_get() : 
 *  - for the purpose to report the MACSEC related information.
 *
 * Parameters :
 *  - port      : the switch port ID.
 *  - type      : information type
 *  - value     : (OUTPUT) value for this information type
 */
int 
soc_macsecphy_dev_info_get(int unit, soc_port_t port, int type, uint32 *value)
{
    int     rv = SOC_E_NONE;
#ifdef BCM_ROBO_SUPPORT
    uint32  temp = 0;
#endif  /* BCM_ROBO_SUPPORT */

    switch (type) {
    case SOC_MACSECPHY_DEV_INFO_SWITCHMACSEC_ATTACHED:
        *value = FALSE;
        
#ifdef BCM_ROBO_SUPPORT
        if (SOC_IS_ROBO(unit)) {
            rv = DRV_DEV_PROP_GET(unit, 
                    DRV_DEV_PROP_SWITCHMACSEC_ATTACH_PBMP, &temp);
            if (SOC_SUCCESS(rv)) {
                *value = (temp & (0x1 << port)) ? TRUE : FALSE;
            }
            
            /* any error return in this case means no MACSEC attached. */
            rv = SOC_E_NONE;
        }
#endif  /* BCM_ROBO_SUPPORT */

        break;
    default:
        rv = SOC_E_PARAM;
    }

    return rv;
}

#else /* INCLUDE_MACSEC */

int 
soc_macsecphy_init(int unit, soc_port_t port, void *pc)
{
    return SOC_E_NONE;
}
#endif /* INCLUDE_MACSEC */

