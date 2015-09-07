/*
 * $Id: phy8040.h,v 1.1 Broadcom SDK $
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
 * File:        phy8040.h
 *
 */

#ifndef   _phy8040_H_
#define   _phy8040_H_

#include <soc/phy.h>

/*
 * By default assume ports 0 <--> 1 are connected.
 */
#define PHY8040_DFLT_SWITCH_PORT    1
#define PHY8040_DFLT_MUX_PORT0      0
#define PHY8040_DFLT_MUX_PORT1      3

/*
 * Pick the same phy address as top level but different dev address 
 * for XGXS blocks.
 */
#define PHY8040_C45_DEV_TOPLVL     0x1
#define PHY8040_C45_DEV_XGXS0      0x3
#define PHY8040_C45_DEV_XGXS1      0x4
#define PHY8040_C45_DEV_XGXS2      0x6
#define PHY8040_C45_DEV_XGXS3      0x7

#define PHY8040_REG_READ(_unit, _phy_ctrl, _addr, _val) \
            READ_PHY_REG((_unit), (_phy_ctrl), (_addr), (_val))
#define PHY8040_REG_WRITE(_unit, _phy_ctrl, _addr, _val) \
            WRITE_PHY_REG((_unit), (_phy_ctrl), (_addr), (_val))
#define PHY8040_REG_MODIFY(_unit, _phy_ctrl, _addr, _val, _mask) \
            MODIFY_PHY_REG((_unit), (_phy_ctrl), (_addr), (_val), (_mask))

/* Power down XGXS blocks (Dev Addr 1) */
#define PHY8040_PWRCTRL_RSVD      0x7830 /* only last nibble is writeable */
#define READ_PHY8040_PWR_CTLr(_unit, _phy_ctrl, _val) \
            PHY8040_REG_READ((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8040_C45_DEV_TOPLVL, 0x8001), (_val))
#define WRITE_PHY8040_PWR_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8040_REG_WRITE((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8040_C45_DEV_TOPLVL, 0x8001), (_val))

/* XGXS {_xgxs=0,1,2,3} Control Register (Addr 0000h) */
#define PHY8040_XGXS_CTRL_SRC(_mux)    (((_mux) & 0xf)<<12)
#define PHY8040_XGXS_CTRL_MDMX         (1<<11)
#define PHY8040_XGXS_CTRL_C45          (1<<10)
#define PHY8040_XGXS_CTRL_DEVID(_id)   (((_id) & 0x1f)<<5)
#define PHY8040_XGXS_CTRL_PRTAD(_ad)   ((_ad) & 0x1f)

#define READ_PHY8040_XGXSn_CTRLr(_unit, _phy_ctrl, _xgxs,_val) \
            PHY8040_REG_READ((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8040_C45_DEV_TOPLVL, (0x8010|(_xgxs))))
#define WRITE_PHY8040_XGXSn_CTRLr(_unit, _phy_ctrl, _xgxs, _val) \
            PHY8040_REG_WRITE((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8040_C45_DEV_TOPLVL, (0x8010|(_xgxs))), (_val))
#define MODIFY_PHY8040_XGXSn_CTRLr(_unit, _phy_ctrl, _xgxs, _val, _mask) \
            PHY8040_REG_MODIFY((_unit), (_phy_ctrl), \
             SOC_PHY_CLAUSE45_ADDR(PHY8040_C45_DEV_TOPLVL, (0x8010|(_xgxs))), (_val), (_mask))

/* Generic XGXSn port/phy register reg access */
#define PHY8040_PRE_EMPHASIS_REG    0x80A7
#define PHY8040_PRE_EMPHASIS(_P)    (((_P) & 0xf) << 12)
#define PHY8040_DRIVER_STRENGTH(_C) (((_C) & 0xf) << 8)

#define PHY8040_LANE_STATUS_REG    0x0018

#define READ_PHY8040_XGXSn_REG(_unit, _phy_ctrl, _devid, _reg, _val)          \
            PHY8040_REG_READ((_unit), (_phy_ctrl),                            \
             SOC_PHY_CLAUSE45_ADDR((_devid), (_reg)), (_val))
#define WRITE_PHY8040_XGXSn_REG(_unit, _phy_ctrl, _devid, _reg, _val)         \
            PHY8040_REG_WRITE((_unit), (_phy_ctrl), \
             SOC_PHY_CLAUSE45_ADDR((_devid), (_reg)), (_val))
#define MODIFY_PHY8040_XGXSn_REG(_unit, _phy_ctrl, _devid, _reg, _val, _mask) \
            PHY8040_REG_MODIFY((_unit), (_phy_ctrl), \
             SOC_PHY_CLAUSE45_ADDR((_devid), (_reg)), (_val), (_mask))

/* user registers */
#define PHY8040_XGXS_CTRL_REG       0x8000
#define PHY8040_LANE_CTRL1_REG      0x8016
#define PHY8040_LANE_CTRL2_REG      0x8017
#define PHY8040_TX0A_CTRL1_REG      0x80A7
#define PHY8040_RX_CTRL_REG         0x80F1
#define PHY8040_RX_STATUS_REG       0x80b0

/* RX status register */
#define PHY8040_1G_LINKUP_MASK      ((1 << 15) | (1 << 12))

#endif  /* _phy8705_H_ */
