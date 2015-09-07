/*
 * $Id: phyreg.h 1.8 Broadcom SDK $
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
 * This file defines common PHY register definition.
 *
 * Its contents are not used directly by applications; it is used only
 * by header files of parent APIs which need to define PHY register definition.
 */

#ifndef _SHR_PHYREG_H
#define _SHR_PHYREG_H

#define _SHR_PORT_PHY_INTERNAL        0x00000001
#define _SHR_PORT_PHY_NOMAP           0x00000002
#define _SHR_PORT_PHY_CLAUSE45        0x00000004
#define _SHR_PORT_PHY_I2C_DATA8       0x00000008
#define _SHR_PORT_PHY_I2C_DATA16      0x00000010

#define _SHR_PORT_PHY_CLAUSE45_ADDR(_devad, _regad)     \
            ((((_devad) & 0x3F) << 16) |                \
             ((_regad) & 0xFFFF))
#define _SHR_PORT_PHY_CLAUSE45_DEVAD(_reg_addr)         \
            (((_reg_addr) >> 16) & 0x3F)
#define _SHR_PORT_PHY_CLAUSE45_REGAD(_reg_addr)         \
            ((_reg_addr) & 0xFFFF)
#define _SHR_PORT_PHY_I2C_ADDR(_devad, _regad)          \
            ((((_devad) & 0xFF) << 16) |                \
             ((_regad) & 0xFFFF))
#define _SHR_PORT_PHY_I2C_DEVAD(_reg_addr)              \
            (((_reg_addr) >> 16) & 0xFF)
#define _SHR_PORT_PHY_I2C_REGAD(_reg_addr)              \
            ((_reg_addr) & 0xFFFF)

#define _SHR_PORT_PHY_REG_RESERVE_ACCESS    0x20000000
#define _SHR_PORT_PHY_REG_1000X             0x40000000
#define _SHR_PORT_PHY_REG_INDIRECT          0x80000000

#define _SHR_PORT_PHY_REG_FLAGS_MASK  0xFF000000
#define _SHR_PORT_PHY_REG_BANK_MASK   0x0000FFFF
#define _SHR_PORT_PHY_REG_ADDR_MASK   0x000000FF

#define _SHR_PORT_PHY_REG_INDIRECT_ADDR(_flags, _reg_bank, _reg_addr) \
            ((((_flags) | _SHR_PORT_PHY_REG_INDIRECT) & \
              _SHR_PORT_PHY_REG_FLAGS_MASK) | \
             (((_reg_bank) & _SHR_PORT_PHY_REG_BANK_MASK) << 8) | \
             ((_reg_addr) & _SHR_PORT_PHY_REG_ADDR_MASK))
#define _SHR_PORT_PHY_REG_FLAGS(_reg_addr)                 \
            ((_reg_addr) & _SHR_PORT_PHY_REG_FLAGS_MASK)
#define _SHR_PORT_PHY_REG_BANK(_reg_addr)                 \
            (((_reg_addr) >> 8) & _SHR_PORT_PHY_REG_BANK_MASK) 
#define _SHR_PORT_PHY_REG_ADDR(_reg_addr)                 \
            ((_reg_addr) & _SHR_PORT_PHY_REG_ADDR_MASK)

#endif  /* !_SHR_PHYREG_H */
