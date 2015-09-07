/*
 * $Id: fcmapphy.h 1.1 Broadcom SDK $
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
 * File:        fcmapphy.h
 *
 * Header file for FCMAP PHYs
 */
#ifndef SOC_FCMAPPHY_H
#define SOC_FCMAPPHY_H


#include <soc/phy/phyctrl.h>
#ifdef INCLUDE_FCMAP

#include <bfcmap.h>

typedef bfcmap_dev_addr_t soc_fcmap_dev_addr_t;

extern int 
soc_fcmapphy_miim_write(soc_fcmap_dev_addr_t dev_addr, 
                         uint32 phy_reg_addr, uint16 data);

extern int 
soc_fcmapphy_miim_read(soc_fcmap_dev_addr_t dev_addr, 
                        uint32 phy_reg_addr, uint16 *data);

/*
 * Create a unique FCMAP portId for specified fcmap port
 * connected to unit/port.
 */
#define SOC_FCMAP_PORTID(u, p) (((u) << 16) | ((p) & 0xff))

/*
 * Return port number within BCM unit from fcmap portId.
 */
#define SOC_FCMAP_PORTID2UNIT(p)   ((p) >> 16)

/*
 * Return BCM unit number from fcmap portId.
 */
#define SOC_FCMAP_PORTID2PORT(p)   ((p) & 0xff)

/*
 * Create FCMAP MDIO address.
 */
#define SOC_FCMAPPHY_MDIO_ADDR(unit, mdio, clause45) \
                        ((((unit) & 0xff) << 24)    |       \
                         (((mdio) & 0xff) << 0)     |       \
                         (((clause45) & 0x1) << 8))

#define SOC_FCMAPPHY_ADDR2UNIT(a)  (((a) >> 24) & 0xff)

#define SOC_FCMAPPHY_ADDR2MDIO(a)  ((a) & 0xff)

#define SOC_FCMAPPHY_ADDR_IS_CLAUSE45(a)  (((a) >> 8) & 1)

extern int
soc_fcmapphy_init(int unit, soc_port_t port, phy_ctrl_t *pc, 
                   bfcmap_core_t core_type, bfcmap_dev_io_f iofn);



#endif /* INCLUDE_FCMAP */
#endif /* SOC_FCMAPPHY_H */

