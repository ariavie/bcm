/*
 * $Id: 0fb6da1211faa06dbbc5e255c84bd371e3df81c9 $
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
 *
 * This software simulator can emulate basic register access for the
 * TSC/Eagle SerDes PHY.
 *
 * The simulator suppor both IEEE clause 22/45 access and Broadcom
 * proprietary SBUS access.
 *
 * Clause 22 address format:
 *   Bits [4:0]   : Clause 22 register address
 *   Bits [31:5]  : Unused
 *
 * Clause 45 address format:
 *   Bits [15:0]  : Clause 45 register address
 *   Bits [20:16] : Clause 45 DEVAD
 *   Bits [23:21] : Clause 45 indicator (001b)
 *   Bits [31:24] : Unused
 *
 * SBUS address format:
 *   Bits [15:0]  : Clause 45 register address
 *   Bits [18:16] : Lane control
 *   Bits [26:19] : Lane multicast (old format)
 *   Bits [31:27] : Clause 45 DEVAD
 *
 * The upper 16 bits if the SBUS address format is identical to the
 * Broadcom Address Extension Register (AER) format.
 *
 * The clause 45 indicator serves two purposes which is to ensure that
 * the upper 16 bits are never zero for a clause 45 address, but it
 * also makes it possible for the PHY bus driver to distinguish
 * between a clause 45 DEVAD and the old AER multicast format.
 *
 */

#include <phymod/phymod_sim.h>
#include <phymod/chip/bcmi_tsce_xgxs_resetval.h>

/* Convenience macro */
#define DBG_VERB PHYMOD_DEBUG_VERBOSE

/* Bit field get/set macros */
#define TSCE_BF_SET(_val, _mask, _shift) _val |= ((_mask) << (_shift))
#define TSCE_BF_GET(_val, _mask, _shift) (((_val) >> (_shift)) & (_mask))

/*
 * Raw 32-bit address consists of AER value in upper 16 bits and
 * clause 45 address in lower 16 bits.
 */
#define TSCE_DEVAD_SHIFT       27
#define TSCE_DEVAD_MASK        0x1f
#define TSCE_DEVAD_GET(_addr) \
    TSCE_BF_GET(_addr, TSCE_DEVAD_MASK, TSCE_DEVAD_SHIFT)
#define TSCE_LANE_SHIFT        16
#define TSCE_LANE_MASK         0x7
#define TSCE_LANE_GET(_addr) \
    TSCE_BF_GET(_addr, TSCE_LANE_MASK, TSCE_LANE_SHIFT)
#define TSCE_REG_SHIFT         0
#define TSCE_REG_MASK          0xffff
#define TSCE_REG_GET(_addr) \
    TSCE_BF_GET(_addr, TSCE_REG_MASK, TSCE_REG_SHIFT)

#define TSCE_ADDR(_devad, _lane, _reg) \
    (((_devad) << TSCE_DEVAD_SHIFT) +  \
     ((_lane) << TSCE_LANE_SHIFT) +    \
     ((_reg) << TSCE_REG_SHIFT))

#define TSCE_AER               TSCE_ADDR(0, 0, 0xffde)
#define TSCE_BLK               TSCE_ADDR(0, 0, 0x001f)

/*
 * The CL45 indicator is used to determine whether the upper 16 bits
 * of the address is an AER value or a clause 45 DEVAD.
 */
#define TSCE_CL45              (0x20 << 16)
#define TSCE_CL45_MASK         (0xe0 << 16)

/* Forward declarations */
STATIC int
tsce_sim_write(phymod_sim_data_t *pms_data, uint32_t addr, uint32_t data);


STATIC uint32_t
tsce_sim_default_data_get(uint32_t addr)
{
    uint32_t devad, reg;

    devad = TSCE_DEVAD_GET(addr);
    reg = TSCE_REG_GET(addr);

    return bcmi_tsce_xgxs_resetval_get(devad, reg);
}

STATIC uint32_t
tsce_sim_reg_copies_get(uint32_t addr)
{
    uint32_t devad, reg;

    devad = TSCE_DEVAD_GET(addr);
    reg = TSCE_REG_GET(addr);

    if (reg == TSCE_AER || reg == TSCE_BLK) {
        return 1;
    }

    if (devad == 0) {
        if ((reg & 0xf000) == 0x9000) {
            return 1;
        }
        if ((reg & 0xf000) == 0xa000) {
            return 2;
        }
        return 4;
    } else if (devad == 1) {
        return 4;
    }
    return 0;
}

STATIC uint32_t
tsce_sim_write_adjust(phymod_sim_data_t *pms_data, uint32_t addr, uint32_t data)
{
    uint32_t devad, reg;

    devad = TSCE_DEVAD_GET(addr);
    reg = TSCE_REG_GET(addr);

    if (devad == 0) {
        switch (reg) {
        case 0xc050:
            /* Set SW_SPEED_CHANGE_DONE and SW_SPEED_CONFIG_VLD in status reg */
            tsce_sim_write(pms_data, addr + 1, 0x3);
            break;
        default:
            break;
        }
    } else if (devad == 1) {
        switch (reg) {
        default:
            break;
        }
    }

    return data;
}

STATIC int
tsce_sim_init(phymod_sim_data_t *pms_data,
               int num_entries, phymod_sim_entry_t *entries)
{
    if (pms_data != NULL) {
        PHYMOD_MEMSET(pms_data, 0, sizeof(*pms_data));
        pms_data->num_entries = num_entries;
        pms_data->entries = entries;
    }
    return PHYMOD_E_NONE;
}

STATIC int
tsce_sim_reset(phymod_sim_data_t *pms_data)
{
    uint32_t sim_size;

    if (pms_data == NULL || pms_data->entries == NULL) {
        return PHYMOD_E_INIT;
    }

    pms_data->entries_used = 0;
    sim_size = pms_data->num_entries * sizeof(phymod_sim_entry_t);
    PHYMOD_MEMSET(pms_data->entries, 0, sim_size);

    return PHYMOD_E_NONE;
}

STATIC int
tsce_sim_read(phymod_sim_data_t *pms_data, uint32_t addr, uint32_t *data)
{
    int idx;
    uint32_t aer, blk, devad, reg, copies;
    uint32_t lane = 0;
    phymod_sim_entry_t *pse;

    if (pms_data == NULL || pms_data->entries == NULL) {
        return PHYMOD_E_INIT;
    }

    devad = 0;

    if (addr < TSCE_BLK) {
        /* Assume clause 22 access */
        (void)tsce_sim_read(pms_data, TSCE_BLK, &blk);
        /* IEEE bit */
        if (addr & 0x10) {
            blk |= 0x8000;
        } else {
            blk &= ~0x8000;
        }
        addr = (addr & 0xf) | (blk & 0xfff0);
        if (addr != TSCE_AER && addr != TSCE_BLK) {
            (void)tsce_sim_read(pms_data, TSCE_AER, &aer);
            addr |= (aer << 16);
        }
    } else {
        /* Extract devad if clause 45 address format */
        if ((addr & TSCE_CL45_MASK) == TSCE_CL45) {
            devad = (addr >> 16) & 0x1f;
            addr &= 0xffff;
        }
    }

    if (addr != TSCE_AER && addr != TSCE_BLK) {
        /* Assume AER is in upper 16 bits */
        aer = (addr >> 16);
        if (aer == 0) {
            /* Try reading real AER instead */
            (void)tsce_sim_read(pms_data, TSCE_AER, &aer);
        }
        /* Add clause 45 devad (if used) */
        if (devad) {
            aer |= (devad << 11);
            addr = (addr & 0xffff) | (aer << 16);
        }
        lane = (aer & 0x7);
        if (lane > 3) {
            /* Force lane 0 if lane is invalid */
            addr = TSCE_ADDR(TSCE_DEVAD_GET(addr), 0, TSCE_REG_GET(addr));
        }
    }

    /* Adjust lane according to number of copies */
    devad = TSCE_DEVAD_GET(addr);
    reg = TSCE_REG_GET(addr);
    copies = tsce_sim_reg_copies_get(addr);
    if (copies == 1) {
        lane = 0;
    } else if (copies == 2) {
        lane &= ~0x1;
    }
    addr = TSCE_ADDR(devad, lane, reg);

    /* Check if this register has been written already */
    for (idx = 0; idx < pms_data->entries_used; idx++) {
        pse = &pms_data->entries[idx];
        if (pse->addr == addr) {
            *data = pse->data;
            DBG_VERB(("tsce_sim_read 0x%08"PRIx32" = 0x%04"PRIx32"\n",
                      addr, *data));
            return PHYMOD_E_NONE;
        }
    }

    /* Return default value if register was never written */
    *data = tsce_sim_default_data_get(addr);

    DBG_VERB(("tsce_sim_read 0x%08"PRIx32" = [0x%04"PRIx32"]\n",
              addr, *data));

    return PHYMOD_E_NONE;
}

STATIC int
tsce_sim_write(phymod_sim_data_t *pms_data, uint32_t addr, uint32_t data)
{
    int idx;
    uint32_t aer, blk, devad, reg, copies, mask;
    uint32_t lane = 0;
    phymod_sim_entry_t *pse;

    if (pms_data == NULL || pms_data->entries == NULL) {
        return PHYMOD_E_INIT;
    }

    devad = 0;

    if (addr < TSCE_BLK) {
        /* Assume clause 22 access */
        (void)tsce_sim_read(pms_data, TSCE_BLK, &blk);
        /* IEEE bit */
        if (addr & 0x10) {
            blk |= 0x8000;
        } else {
            blk &= ~0x8000;
        }
        addr = (addr & 0xf) | (blk & 0xfff0);
        if (addr != TSCE_AER && addr != TSCE_BLK) {
            (void)tsce_sim_read(pms_data, TSCE_AER, &aer);
            addr |= (aer << 16);
        }
    } else {
        /* Extract devad if clause 45 address format */
        if ((addr & TSCE_CL45_MASK) == TSCE_CL45) {
            devad = (addr >> 16) & 0x1f;
            addr &= 0xffff;
        }
    }

    if (addr != TSCE_AER && addr != TSCE_BLK) {
        /* Assume AER is in upper 16 bits */
        aer = (addr >> 16);
        if (aer == 0) {
            /* Try reading real AER instead */
            (void)tsce_sim_read(pms_data, TSCE_AER, &aer);
        }
        /* Add clause 45 devad (if used) */
        if (devad) {
            aer |= (devad << 11);
            addr = (addr & 0xffff) | (aer << 16);
        }
        lane = (aer & 0x7);
        if (lane > 6) {
            return PHYMOD_E_PARAM;
        }
        if (lane > 3) {
            /*
             * Handle lane broadcast
             *
             * Note that we use lane 8 instead of lane 0 to prevent a
             * broadcast loop. The value 8 will become 0 when masked
             * with 0x7, but it prevents the AER in the upper 16 bits
             * from being zero, which will cause the code above to
             * obtain the AER value from register 0xffde.
             */
            reg = TSCE_REG_GET(addr);
            devad = TSCE_DEVAD_GET(addr);
            if (lane == 4 || lane == 6) {
                /* Write lanes 0 and 1 */
                addr = TSCE_ADDR(devad, 8, reg);
                (void)tsce_sim_write(pms_data, addr, data);
                addr = TSCE_ADDR(devad, 1, reg);
                (void)tsce_sim_write(pms_data, addr, data);
            }
            if (lane == 5 || lane == 6) {
                /* Write lanes 2 and 3 */
                addr = TSCE_ADDR(devad, 2, reg);
                (void)tsce_sim_write(pms_data, addr, data);
                addr = TSCE_ADDR(devad, 3, reg);
                (void)tsce_sim_write(pms_data, addr, data);
            }
            return PHYMOD_E_NONE;
        }
    }

    /* Adjust data and/or related registers */
    data = tsce_sim_write_adjust(pms_data, addr, data);

    /* Adjust lane according to number of copies */
    devad = TSCE_DEVAD_GET(addr);
    reg = TSCE_REG_GET(addr);
    copies = tsce_sim_reg_copies_get(addr);
    if (copies == 1) {
        lane = 0;
    } else if (copies == 2) {
        lane &= ~0x1;
    }
    addr = TSCE_ADDR(devad, lane, reg);

    /* Support optional write mask in upper 16 bits */
    mask = (data >> 16);
    if (mask == 0) {
        mask = 0xffff;
    }
    data &= mask;

    /* Check if this register has been written already */
    for (idx = 0; idx < pms_data->entries_used; idx++) {
        pse = &pms_data->entries[idx];
        if (pse->addr == addr) {
            pse->data &= ~mask;
            pse->data |= data;
            DBG_VERB(("tsce_sim_write 0x%08"PRIx32" = 0x%04"PRIx32"\n",
                      addr, pse->data));
            return PHYMOD_E_NONE;
        }
    }

    /* Check if database is full */
    if (pms_data->entries_used >= pms_data->num_entries) {
        return PHYMOD_E_RESOURCE;
    }

    /* Check if new data matches default value */
    if (data == tsce_sim_default_data_get(addr)) {
        return PHYMOD_E_NONE;
    }

    /* Add new register value */
    pse = &pms_data->entries[pms_data->entries_used++];
    pse->addr = addr;
    pse->data = data;

    DBG_VERB(("tsce_sim_write 0x%08"PRIx32" = 0x%04"PRIx32" (new)\n",
              addr, pse->data));

    return PHYMOD_E_NONE;
}

STATIC int
tsce_sim_event(phymod_sim_data_t *pms_data, phymod_sim_event_t event)
{
    if (pms_data == NULL || pms_data->entries == NULL) {
        return PHYMOD_E_INIT;
    }

    return PHYMOD_E_NONE;
}

phymod_sim_drv_t tsce_sim_drv = {
    tsce_sim_init,
    tsce_sim_reset,
    tsce_sim_read,
    tsce_sim_write,
    tsce_sim_event
};

