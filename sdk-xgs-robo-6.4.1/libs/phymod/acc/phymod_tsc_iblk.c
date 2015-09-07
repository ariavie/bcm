/*
 * $Id: phymod_tsc_iblk.c,v 1.1.2.5 Broadcom SDK $
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
 * XGS internal PHY read function with AER support.
 *
 * This function accepts a 32-bit PHY register address and will
 * properly configure clause 45 DEVAD and XAUI lane access.
 * Please see phymod_reg.h for additional information.
 *
 */

#include <phymod/acc/phymod_tsc_iblk.h>
#include <phymod/phymod_debug.h>

/*
 * Some bus drivers can handle both clause 22 and clause 45 access, so
 * we use this bit to request clause 45 access.
 */
#define FORCE_CL45      0x20

int
phymod_tsc_iblk_read(const phymod_access_t *pa, uint32_t addr, uint32_t *data)
{
    int ioerr = 0;
    uint32_t devad = (addr >> 16) & 0xf;
    uint32_t blkaddr, regaddr;
    uint32_t lane_map, lane;
    uint32_t aer, add;
    phymod_bus_t* bus;

    add = addr ;

    if (pa == NULL) {
        PHYMOD_VDBG(DBG_ACC,pa,("iblk_rd add=%x pa=null\n", add));
        return -1;
    }

    bus = PHYMOD_ACC_BUS(pa);
    
    /* Do not attempt to read write-only registers */
    if (addr & PHYMOD_REG_ACC_TSC_IBLK_WR_ONLY) {
        *data = 0;
        PHYMOD_VDBG(DBG_ACC,pa,("iblk_rd add=%x WO=1\n", add));
        return 0;
    }

    /* Determine which lane to read from */
    lane = 0;
    if (addr & PHYMOD_REG_ACC_AER_IBLK_FORCE_LANE) {
        /* Forcing lane overrides default behavior */
        lane = (addr >> PHYMOD_REG_ACCESS_FLAGS_SHIFT) & 0x7;
    } else {
        /* Use first lane in lane map by default */
        lane_map = PHYMOD_ACC_LANE(pa);
        if (lane_map & 0x1) {
            lane = 0;
        } else if (lane_map & 0x2) {
            lane = 1;
        } else if (lane_map & 0x4) {
            lane = 2;
        } else if (lane_map & 0x8) {
            lane = 3;
        } else if (lane_map & 0xfff0) {
            lane = -1;
            while (lane_map) {
                lane++;
                lane_map >>= 1;
            }
        }
    }    

    /* Use default DEVAD if none specified */
    if (devad == 0) {
        devad = PHYMOD_ACC_DEVAD(pa);
    }

    /* Encode address extension */
    aer = lane | (devad << 11);

    /* Mask raw register value */
    addr &= 0xffff;

    /* If bus driver supports lane control, then we are done */
    if (PHYMOD_BUS_CA_IS_LANE_CTRL(bus)) {
        ioerr += PHYMOD_BUS_READ(pa, addr | (aer << 16), data);
        PHYMOD_VDBG(DBG_ACC,pa,("iblk_rd sbus add=%x aer=%x adr=%x rtn=%0d d=%x\n", add, aer, addr, ioerr, *data));
        return ioerr;
    }

    /* Use clause 45 access if supported */
    if (PHYMOD_PHYMOD_ACC_F_IS_CLAUSE45(pa)) {
        devad |= FORCE_CL45;
        ioerr += PHYMOD_BUS_WRITE(pa, 0xffde | (devad << 16), aer);
        ioerr += PHYMOD_BUS_READ(pa, addr | (devad << 16), data);
        PHYMOD_VDBG(DBG_ACC,pa,("iblk_rd cl45 add=%x dev=%x aer=%x adr=%x rtn=%0d d=%x\n", add, devad, aer, addr, ioerr, *data));
        return ioerr;
    }

    /* Write address extension register */
    ioerr += PHYMOD_BUS_WRITE(pa, 0x1f, 0xffd0);
    ioerr += PHYMOD_BUS_WRITE(pa, 0x1e, aer);

    /* Select block */
    blkaddr = addr & 0xfff0;
    ioerr += PHYMOD_BUS_WRITE(pa, 0x1f, blkaddr);

    /* Read register value */
    regaddr = addr & 0xf;
    if (addr & 0x8000) {
        regaddr |= 0x10;
    }
    ioerr += PHYMOD_BUS_READ(pa, regaddr, data);
    PHYMOD_VDBG(DBG_ACC,pa,("iblk_rd cl22 add=%x aer=%x blk=%x adr=%x reg=%x rtn=%0d d=%x\n", add, aer, blkaddr, addr, regaddr, ioerr, *data));
    return ioerr;
}

int
phymod_tsc_iblk_write(const phymod_access_t *pa, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    uint32_t devad = (addr >> 16) & 0xf;
    uint32_t blkaddr, regaddr;
    uint32_t lane_map, lane;
    uint32_t aer, add;
    uint32_t wr_mask, rdata;
    phymod_bus_t* bus;
    
    add = addr ;

    if (pa == NULL) {
        PHYMOD_VDBG(DBG_ACC,pa,("iblk_wr add=%x pa=null\n", addr));
        return -1;
    }

    bus = PHYMOD_ACC_BUS(pa);

    lane = 0;
    if (addr & PHYMOD_REG_ACC_AER_IBLK_FORCE_LANE) {
        /* Forcing lane overrides default behavior */
        lane = (addr >> PHYMOD_REG_ACCESS_FLAGS_SHIFT) & 0x7;
    } else {
        /* Write to all lanes by default */
        lane_map = PHYMOD_ACC_LANE(pa);
        if (lane_map == 0xf) {
            lane = PHYMOD_TSC_IBLK_BCAST;
        } else if (lane_map == 0x3) {
            lane = PHYMOD_TSC_IBLK_MCAST01;
        } else if (lane_map == 0xc) {
            lane = PHYMOD_TSC_IBLK_MCAST23;
        } else if (lane_map & 0xffff) {
            lane = -1;
            while (lane_map) {
                lane++;
                lane_map >>= 1;
            }
        }
    }

    /* Use default DEVAD if none specified */
    if (devad == 0) {
        devad = PHYMOD_ACC_DEVAD(pa);
    }

    /* Check if write mask is specified */
    wr_mask = (data >> 16);
    if (wr_mask) {
        /* Read register if bus driver does not support write mask */
        if (PHYMOD_BUS_CA_IS_WR_MODIFY(bus) == 0) {
            ioerr += phymod_tsc_iblk_read(pa, addr, &rdata);
            data = (rdata & ~wr_mask) | (data & wr_mask);
            data &= 0xffff;
        }
    }

    /* Encode address extension */
    aer = lane | (devad << 11);

    /* Mask raw register value */
    addr &= 0xffff;

    /* If bus driver supports lane control, then we are done */
    if (PHYMOD_BUS_CA_IS_LANE_CTRL(bus)) {
        ioerr += PHYMOD_BUS_WRITE(pa, addr | (aer << 16), data);
        PHYMOD_VDBG(DBG_ACC,pa,("iblk_wr sbus add=%x aer=%x adr=%x rtn=%0d d=%x\n", add, aer, addr, ioerr, data));
        return ioerr;
    }

    /* Use clause 45 access if supported */
    if (PHYMOD_PHYMOD_ACC_F_IS_CLAUSE45(pa)) {
        addr &= 0xffff;
        devad |= FORCE_CL45;
        ioerr += PHYMOD_BUS_WRITE(pa, 0xffde | (devad << 16), aer);
        ioerr += PHYMOD_BUS_WRITE(pa, addr | (devad << 16), data);
        PHYMOD_VDBG(DBG_ACC,pa,("iblk_wr cl45 add=%x dev=%x aer=%x adr=%x rtn=%0d d=%x\n", add, devad, aer, addr, ioerr, data));
        return ioerr;
    }

    /* Write address extension register */
    ioerr += PHYMOD_BUS_WRITE(pa, 0x1f, 0xffd0);
    ioerr += PHYMOD_BUS_WRITE(pa, 0x1e, aer);

    /* Select block */
    blkaddr = addr & 0xfff0;
    ioerr += PHYMOD_BUS_WRITE(pa, 0x1f, blkaddr);

    /* Write register value */
    regaddr = addr & 0xf;
    if (addr & 0x8000) {
        regaddr |= 0x10;
    }
    ioerr += PHYMOD_BUS_WRITE(pa, regaddr, data);
    PHYMOD_VDBG(DBG_ACC,pa,("iblk_wr cl22 add=%x aer=%x blk=%x reg=%x adr=%x rtn=%0d d=%x\n", addr, aer, blkaddr, regaddr, addr, ioerr, data));
    return ioerr;
}
