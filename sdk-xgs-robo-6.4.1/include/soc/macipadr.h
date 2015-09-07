/*
 * $Id: macipadr.h,v 1.7 Broadcom SDK $
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
 * File:        macipadr.h
 * Purpose:     Typedefs for MAC and IP addresses
 */

#ifndef _SYS_MACIPADR_H
#define _SYS_MACIPADR_H

#include <sal/core/libc.h>
#include <soc/types.h>
#include <shared/l3.h>

typedef _shr_ip_addr_t  ip_addr_t;      /* IP Address */
typedef _shr_ip6_addr_t  ip6_addr_t;    /* IPv6 Address */

extern const sal_mac_addr_t _soc_mac_spanning_tree;
extern const sal_mac_addr_t _soc_mac_all_routers;
extern const sal_mac_addr_t _soc_mac_all_zeroes;
extern const sal_mac_addr_t _soc_mac_all_ones;

/* sal_mac_addr_t mac;  Just generate a list of the macs for display */
#define MAC_ADDR_LIST(mac) \
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]

/* sal_mac_addr_t m1, m2;  True if equal */
#define MAC_ADDR_EQUAL(m1, m2) (!sal_memcmp(m1, m2, sizeof(sal_mac_addr_t)))

/* sal_mac_addr_t m1, m2;  like memcmp, returns -1, 0, or 1 */
#define MAC_ADDR_CMP(m1, m2) sal_memcmp(m1, m2, sizeof(sal_mac_addr_t))

/* sal_mac_addr_t mac; True if multicast mac address */
#define MAC_IS_MCAST(mac) (((mac)[0] & 0x1) ? TRUE : FALSE)

#define MACADDR_STR_LEN 18              /* Formatted MAC address */
#define IPADDR_STR_LEN  16              /* Formatted IP address */
#define IP6ADDR_STR_LEN 64              /* Formatted IP address */


/* Adjust justification for uint32 writes to fields */
/* dst is an array name of type uint32 [] */
#define MAC_ADDR_TO_UINT32(mac, dst) do {\
        dst[0] = (((uint32)mac[2]) << 24 | \
                  ((uint32)mac[3]) << 16 | \
                  ((uint32)mac[4]) << 8 | \
                  ((uint32)mac[5])); \
        dst[1] = (((uint32)mac[0]) << 8 | \
                  ((uint32)mac[1])); \
    } while (0)

/* Adjust justification for uint32 writes to fields */
/* dst is an array name of type uint32 [] */
#define MAC_ADDR_FROM_UINT32(mac, src) do {\
        mac[0] = (uint8) (src[1] >> 8 & 0xff); \
        mac[1] = (uint8) (src[1] & 0xff); \
        mac[2] = (uint8) (src[0] >> 24); \
        mac[3] = (uint8) (src[0] >> 16 & 0xff); \
        mac[4] = (uint8) (src[0] >> 8 & 0xff); \
        mac[5] = (uint8) (src[0] & 0xff); \
    } while (0)

#endif  /* _SYS_MACIPADR_H */
