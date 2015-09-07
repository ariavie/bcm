/*
 * $Id: phy_mac_ctrl.h,v 1.4 Broadcom SDK $
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
 */

#ifndef _MAC_CTRL_H
#define _MAC_CTRL_H

#include <sal/types.h>
#include <sal/core/spl.h>
#include <sal/core/libc.h>
#include <blmi_io.h>


struct phy_mac_ctrl_s;
/*
 * MAC Driver Function prototypes 
 */
typedef int (*mac_init)(struct phy_mac_ctrl_s*, int);
typedef int (*mac_reset)(struct phy_mac_ctrl_s*, int, int);
typedef int (*mac_auto_cfg_set)(struct phy_mac_ctrl_s*, int, int);
typedef int (*mac_enable_set)(struct phy_mac_ctrl_s*, int, int);
typedef int (*mac_enable_get)(struct phy_mac_ctrl_s*, int, int *);
typedef int (*mac_speed_set)(struct phy_mac_ctrl_s*, int, int);
typedef int (*mac_speed_get)(struct phy_mac_ctrl_s*, int, int *);
typedef int (*mac_duplex_set)(struct phy_mac_ctrl_s*, int, int);
typedef int (*mac_duplex_get)(struct phy_mac_ctrl_s*, int, int *);
typedef int (*mac_ipg_set)(struct phy_mac_ctrl_s*, int, int);
typedef int (*mac_max_frame_set)(struct phy_mac_ctrl_s*, int, buint32_t);
typedef int (*mac_max_frame_get)(struct phy_mac_ctrl_s*, int, buint32_t*);
typedef int (*mac_pause_set)(struct phy_mac_ctrl_s*, int, int);
typedef int (*mac_pause_get)(struct phy_mac_ctrl_s*, int, int*);
typedef int (*mac_pause_frame_fwd_set)(struct phy_mac_ctrl_s*, int, int);

typedef struct phy_mac_drv_s {
    mac_init                f_mac_init;
    mac_reset               f_mac_reset;
    mac_auto_cfg_set        f_mac_auto_config_set;
    mac_enable_set          f_mac_enable_set;
    mac_enable_get          f_mac_enable_get;
    mac_speed_set           f_mac_speed_set;
    mac_speed_get           f_mac_speed_get;
    mac_duplex_set          f_mac_duplex_set;
    mac_duplex_get          f_mac_duplex_get;
    mac_ipg_set             f_mac_ipg_set;
    mac_max_frame_set       f_mac_max_frame_set;
    mac_max_frame_get       f_mac_max_frame_get;
    mac_pause_set           f_mac_pause_set;
    mac_pause_get           f_mac_pause_get;
    mac_pause_frame_fwd_set f_mac_pause_frame_fwd_set;
} phy_mac_drv_t;

typedef struct phy_mac_ctrl_s {
    /* Device address for the MMI */
    blmi_dev_addr_t  dev_addr;

    /* MAC driver */
    phy_mac_drv_t   mac_drv;

    /* MAC IO handler for MMI */
    blmi_dev_io_f    devio_f;

    /* Config flag */
    uint32 flag;
#define PHY_MAC_CTRL_FLAG_PHY_FIX_LATENCY_EN  (1<<0) 
#define PHY_MAC_CTRL_FLAG_XMAC_V2                (1<<1) 

} phy_mac_ctrl_t;

#define BMACSEC_MAC_DRV_FN(mmc,_mf)   (mmc)->mac_drv._mf

#define _MAC_DRV_CALL(mmc, _mf, _ma)                             \
        (mmc == NULL ? BMACSEC_E_PARAM :                         \
         (BMACSEC_MAC_DRV_FN(mmc,_mf) == NULL ?                  \
         BMACSEC_E_UNAVAIL : BMACSEC_MAC_DRV_FN(mmc,_mf) _ma))

#define BMACSEC_MAC_INIT(mmc, _p) \
        _MAC_DRV_CALL((mmc), f_mac_init, ((mmc), (_p)))

#define BMACSEC_MAC_RESET(mmc, _p) \
        _MAC_DRV_CALL((mmc), f_mac_reset, ((mmc), (_p)))

#define BMACSEC_MAC_AUTO_CONFIG_SET(mmc, _p, _v) \
        _MAC_DRV_CALL((mmc), f_mac_auto_config_set, ((mmc), (_p), (_v)))

#define BMACSEC_MAC_ENABLE_SET(mmc, _p) \
        _MAC_DRV_CALL((mmc), f_mac_enable_set, ((mmc), (_p)))

#define BMACSEC_MAC_SPEED_SET(mmc, _p, _s) \
        _MAC_DRV_CALL((mmc), f_mac_speed_set, ((mmc), (_p), (_s)))

#define BMACSEC_MAC_SPEED_GET(mmc, _p) \
        _MAC_DRV_CALL((mmc), f_mac_speed_get, ((mmc), (_p)))

#define BMACSEC_MAC_DUPLEX_SET(mmc, _p, _d) \
        _MAC_DRV_CALL((mmc), f_mac_duplex_set, ((mmc), (_p), (_d)))

#define BMACSEC_MAC_DUPLEX_GET(mmc, _p) \
        _MAC_DRV_CALL((mmc), f_mac_duplex_get, ((mmc), (_p)))

#define BMACSEC_MAC_IPG_SET(mmc, _p, _ipg) \
        _MAC_DRV_CALL((mmc), f_mac_ipg_set, ((mmc), (_p), (_ipg)))

#define BMACSEC_MAC_IPG_GET(mmc, _p) \
        _MAC_DRV_CALL((mmc), f_mac_ipg_get, ((mmc), (_p)))

#define BMACSEC_MAC_MAX_FRAME_SET(mmc, _p, _max) \
        _MAC_DRV_CALL((mmc), f_mac_max_frame_set, ((mmc), (_p), (_max)))

#define BMACSEC_MAC_MAX_FRAME_GET(mmc, _p) \
        _MAC_DRV_CALL((mmc), f_mac_max_frame_get, ((mmc), (_p)))

#define BMACSEC_MAC_PAUSE_SET(mmc, _p, _v) \
        _MAC_DRV_CALL((mmc), f_mac_pause_set, ((mmc), (_p), (_v)))

#define BMACSEC_MAC_PAUSE_GET(mmc, _p) \
        _MAC_DRV_CALL((mmc), f_mac_pause_get, ((mmc), (_p)))

#define BMACSEC_MAC_PAUSE_FRAME_FWD_SET(mmc, _p, _v) \
        _MAC_DRV_CALL((mmc), f_mac_pause_frame_fwd_set, ((mmc), (_p), (_v)))

/*
 * MAC driver types.
 */
typedef enum {
    PHY_MAC_CORE_UNKNOWN    = 0,
    PHY_MAC_CORE_UNIMAC,
    PHY_MAC_CORE_BIGMAC,
    PHY_MAC_CORE_XMAC
} phy_mactype_t;

/*
 * Allocate mac control block and attach the MAC driver.
 */
phy_mac_ctrl_t* 
phy_mac_driver_attach(blmi_dev_addr_t dev_addr,
                         phy_mactype_t mac_type,
                         blmi_dev_io_f mmi_cbf);

int phy_mac_driver_detach(phy_mac_ctrl_t *mmc);

void 
phy_mac_driver_config(phy_mac_ctrl_t *mmc,
                         phy_mactype_t  mac_type,
                         uint32 flag);

 
#endif /* _MAC_CTRL_H */

