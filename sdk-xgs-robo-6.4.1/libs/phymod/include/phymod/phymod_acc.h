/*
 * $Id: phymod_acc.h,v 1.1.2.3 Broadcom SDK $
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

#ifndef __PHYMOD_ACC_H__
#define __PHYMOD_ACC_H__

#include <phymod/phymod_system.h>
#include <phymod/phymod.h>

/*
 * Occasionally the phymod_bus_t changes to support new features. This define
 * allows applications to write backward compatible PHY bus drivers.
 */
#define PHYMOD_BUS_VERSION              1

/*
 * Extended PHY bus operation.
 *
 * If a PHY bus driver supports extended capabilities this must be
 * indicated via the flags field.  The access driver will then provide
 * additional information embedded in the function parameters.
 *
 * Read parameter encodings:
 *
 *   addr:  bits [31:16] - address extension
 *          bits [15:0]  - register address
 *
 *   data:  bits [31:16] - unused
 *          bits [15:0]  - register value
 * 
 * Write parameter encodings:
 *
 *   addr:  bits [31:16] - address extension
 *          bits [15:0]  - register address
 *
 *   data:  bits [31:16] - write mask
 *          bits [15:0]  - register value
 *
 * The address extension is hardware-sepcific, but typically it
 * contains the clause 45 DEVAD and a XAUI lane encoding.
 *
 * A write mask of zero will write all bits, otherwise only the
 * register value bits for which the corresponsing mask bit it set
 * will be written.
 */
 
/* Raw (unadjusted) PHY address */
#define PHYMOD_ACC_CACHE_INVAL(pa_) \
    PHYMOD_ACC_CACHE(pa_) = ~0
#define PHYMOD_ACC_CACHE_SYNC(pa_,data_) \
    PHYMOD_ACC_CACHE(pa_) = (data_)
#define PHYMOD_ACC_CACHE_IS_DIRTY(pa_,data_) \
    ((PHYMOD_ACC_FLAGS(pa_) & PHYMOD_ACC_F_USE_CACHE) == 0 || \
      PHYMOD_ACC_CACHE(pa_) != (data_))
#define PHYMOD_ACC_CACHE_ENABLE(pa_) do { \
        PHYMOD_ACC_FLAGS(pa_) |= PHYMOD_ACC_F_USE_CACHE; \
        PHYMOD_ACC_CACHE_INVAL(pa_); \
    } while (0)

/* PHY bus address (MDIO PHYAD) */
#define PHYMOD_ACC_BUS_ADDR(pa_) PHYMOD_ACC_ADDR(pa_)

/* Ensures that phymod_access_t structure is properly initialized */
#define PHYMOD_ACC_CHECK(pa_) \
    do { \
        if (phymod_acc_check(pa_) < 0) return PHYMOD_E_INTERNAL; \
    } while (0)

/* Access to PHY bus with instance adjustment (addr_offset) */
#define PHYMOD_BUS_READ(pa_,r_,v_) phymod_bus_read(pa_,r_,v_)
#define PHYMOD_BUS_WRITE(pa_,r_,v_) phymod_bus_write(pa_,r_,v_)

extern int
phymod_acc_check(const phymod_access_t *pa);

extern int
phymod_bus_read(const phymod_access_t *pa, uint32_t reg, uint32_t *data);

extern int
phymod_bus_write(const phymod_access_t *pa, uint32_t reg, uint32_t data);

#endif /* __PHYMOD_ACC_H__ */
