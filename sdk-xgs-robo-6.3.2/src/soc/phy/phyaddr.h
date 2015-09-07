/*
 * $Id: phyaddr.h 1.4 Broadcom SDK $
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
 * File:        phyaddr.h
 * Purpose:     Basic defines for PHY configuration
 */

#ifndef _PHY_ADDR_H
#define _PHY_ADDR_H

#define _XGS12_FABRIC_PHY_ADDR_DEFAULT(_unit, _port, _phy_addr, _phy_addr_int) \
        do {                            \
            *(_phy_addr)     = (_port); \
            *(_phy_addr_int) = (_port); \
        } while (0)


#define _DRACO_PHY_ADDR_DEFAULT(_unit, _port, _phy_addr, _phy_addr_int) \
        do {                                           \
            if (IS_GE_PORT((_unit), (_port))) {        \
                *(_phy_addr) = (_port) + 1;            \
                *(_phy_addr_int) = (_port) + 0x80;     \
            } else {                                   \
                *(_phy_addr) = (_port) + 0x80;         \
                *(_phy_addr_int) = (_port) + 0x80;     \
            }                                          \
        } while (0)

#define _LYNX_PHY_ADDR_DEFAULT(_unit, _port, _phy_addr, _phy_addr_int) \
        do {                                           \
            *(_phy_addr) = (_port) + 1;                \
            *(_phy_addr_int) = (_port) + 0x80;         \
        } while (0)

#define _RAPTOR_PHY_ADDR_DEFAULT(_unit, _port, _phy_addr, _phy_addr_int)    \
        do {                                                                \
           /*                                                               \
            * MDIO address and MDIO bus select for External Phy             \
            * port 0  - 0  CPU (_port) no external MDIO address             \
            * port 1  - 2  MDIO address 2, 3 (0x2 - 0x3)                    \
            * port 3  - 3  No internal MDIO                                 \
            * port 4  - 5  MDIO address 4, 6 (0x4 - 0x6)                    \
            * port 6  - 29 MDIO address 7,30 (0x7 - 0x1e)                   \
            * port 30 - 53 MDIO address 1,24 (0x41 - 0x58 with Bus sel set) \
            */                                                              \
            *(_phy_addr) = (_port) +                                        \
                           (((_port) > 29) ? (0x40 - 29) : (1));            \
          /*                                                                \
           * MDIO address and MDIO bus select for Internal Phy (Serdes)     \
           * port 0  - 0  CPU (_port) no internal MDIO address              \
           * port 1  - 2  MDIO address 1,2 (0x81 - 0x82 with Intenal sel set) \
           * port 3  - 3  No internal MDIO                                    \
           * port 4  - 5  MDIO address 4, 5 (0x84 - 0x85 with Intenal sel set)\
           * port 6  - 29 MDIO address 6,29 (0x86 - 0x9D with Intenal sel set)\
           * port 30 - 53 MDIO address 0,23 (0xc0 - 0xd7 with Intenal sel set)\
           */                                                               \
            *(_phy_addr_int) = (_port) +                                    \
                               (((_port) > 29) ? (0xc0 - 30) : (0x80));     \
         } while (0)

#define _TUCANA_PHY_ADDR_DEFAULT(_unit, _port, _phy_addr, _phy_addr_int) \
        do {                                           \
            if (IS_HG_PORT(unit, (_port))) {           \
                *(_phy_addr) = (_port) + 0x80;         \
                *(_phy_addr_int) = (_port) + 0x80;     \
            } else {                                   \
                *(_phy_addr) = (_port) + 1;            \
                *(_phy_addr_int) = 0xff;               \
            }                                          \
         } while (0)

#define _STRATA_PHY_ADDR_DEFAULT(_unit, _port, _phy_addr, _phy_addr_int) \
        do {                                           \
            *(_phy_addr) = (_port) + 1;                \
            *(_phy_addr_int) = 0xff;                   \
        } while (0)

#define _ROBO_PHY_ADDR_DEFAULT(_unit, _port, _phy_addr, _phy_addr_int) \
        do {                                           \
            *(_phy_addr) = (_port);                \
            *(_phy_addr_int) = (_port);            \
        } while (0)

#define _SBX_PHY_ADDR_DEFAULT(_unit, _port, _phy_addr, _phy_addr_int) \
        do {                                           \
            *(_phy_addr) = (_port) ;                   \
            *(_phy_addr_int) = (_port));       \
        } while (0)


#endif /* _PHY_ADDR_H */
