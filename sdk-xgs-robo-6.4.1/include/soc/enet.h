/*
 * $Id: enet.h,v 1.12 Broadcom SDK $
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
 * This header file defines packet formats for Ethernet and Broadcom stacking.
 */

#ifndef _SOC_ENET_H
#define _SOC_ENET_H

#include <sal/core/libc.h>
#include <soc/util.h>
#include <soc/types.h>
#include <soc/macipadr.h>

/*
 * Least-common denominator rxbuf start-of-data offset:
 * Must be >= size of largest rxhdr
 * Must be 2-mod-4 aligned so IP is 0-mod-4
 */
#define	MAX_HWRXOFF		32
#define	ETC47XX_HWRXOFF		30
#if defined(IPROC_CMICD)
#define	ETCGMAC_HWRXOFF		30
#else
/* Keystone */
#define	ETCGMAC_HWRXOFF		32
#endif

typedef struct enet_hdr_s {
    sal_mac_addr_t		en_dhost;	/* Destination */
    sal_mac_addr_t		en_shost;	/* Source */
    union {
	uint16	_tpid;			/* Tag Prot ID */
	uint16	_len;			/* Length */
    }			en_f3;		/* Field 3 */
#   define	en_untagged_len	en_f3._len
#   define	en_tag_tpid	en_f3._tpid

    /* Untagged Format ends here */

    uint16		en_tag_ctrl;	/* Tag control */
    uint16		en_tag_len;	/* Tagged length */

    /* SNAP Fields */

    uint8		en_snap_dsap;
    uint8		en_snap_ssap;
    uint8		en_snap_ctrl;
    uint8		en_snap_org[3];
    uint16		en_snap_etype;
} enet_hdr_t;

#define ENET_MAC_SIZE				6
#define ENET_FCS_SIZE                           4
#define ENET_MIN_PKT_SIZE                       64

#define ENET_DEFAULT_BRCMID			0x8874
#define ENET_BRCM_TAG_SIZE			6
#define ENET_BRCM_SHORT_TAG_SIZE    4

/* Define the default tagged protocol id */

#define ENET_DEFAULT_TPID			0x8100
#define ENET_TAG_SIZE				4

/*
 * Defines:	ENET_XXX_HDR_LEN
 * Purpose:	Defines the length of the various enet headers.
 */
#define		ENET_UNTAGGED_HDR_LEN		14
#define		ENET_TAGGED_HDR_LEN		18
#define		ENET_SNAP_HDR_LEN		26
#define          ENET_CHECKSUM_LEN              4

/*
 * Defines:	ENET_[TAGGED|SNAP|II]
 * Purpose:	Returned TRUE if the ethernet header is TAGGED or SNAP,
 *		or ethernet II.
 */
#define		ENET_TAGGED(e) \
    (soc_ntohs((e)->en_untagged_len) == (0x8100))

#define		ENET_SNAP(e) \
    (ENET_TAGGED(e) && ((e)->en_snap_dsap == 0xaa) && \
    ((e)->en_snap_ssap == 0xaa) && ((e)->en_snap_ctrl == 0x03))

#define		ENET_II(e) \
    (0x800 == (ENET_TAGGED(e) ? \
	       soc_ntohs((e)->en_tag_len) : soc_ntohs((e)->en_untagged_len)))

/*
 * Macro: 	ENET_HDR_LEN
 * Purpose: 	Returns the length of the Ethernet header.
 */
#define		ENET_HDR_LEN(e) \
   (ENET_SNAP(e) ? ENET_SNAP_HDR_LEN : \
		    (ENET_TAGGED(e) ? ENET_TAGGED_HDR_LEN : \
				   ENET_UNTAGGED_HDR_LEN))

/*
 * Macro:	ENET_LEN
 * Purpose:	Check if the ethernet length field is a length
 *		or an ID of some sort.
 */
#define	ENET_LEN(e)						\
    (0x600 > soc_ntohs(ENET_TAGGED(e) ?				\
		   (e)->en_tag_len : (e)->en_untagged_len))

/*
 * Macro:	ENET_SET/GET_MACADDR
 * Purpose:	Set or get the mac address from an ethernet header into
 *		a sal_mac_addr_t.
 */
#define		ENET_SET_MACADDR(e, s) \
    sal_memcpy((e), (s), sizeof(sal_mac_addr_t))
#define		ENET_GET_MACADDR(e, s) \
    sal_memcpy((s), (e), sizeof(sal_mac_addr_t))
#define		ENET_COPY_MACADDR(s, d)	\
    sal_memcpy((d), (s), sizeof(sal_mac_addr_t))
#define		ENET_CMP_MACADDR(a, b)	\
    sal_memcmp((a), (b), sizeof(sal_mac_addr_t))

/*
 * Typedef:	ENET_MAX_SIZE
 * Purpose:	Define the maximum size of an ethernet packet (2K)
 */
#define	ENET_MAX_SIZE	(2 * 1024)


/*
 * Macro:	ENET_MACADDR_MULTICAST/BROADCAST/UNICAST
 * Purpose:	Determine if a mac address is unicast, multicast,
 *		or broadcast.
 */
#define	ENET_MACADDR_BROADCAST(e) \
    MAC_ADDR_EQUAL((e), _soc_mac_all_ones)

#define	ENET_MACADDR_MULTICAST(e) \
    (((e)[0] & 0x01) && !ENET_MACADDR_BROADCAST(e))

#define	ENET_MACADDR_UNICAST(e)	(!((e)[0] & 0x01))

/** Following macros extract source & dest mac addresses */
/* sal_mac_addr_t m; */
#define ENET_SRCMAC_GET(pkt, m)			\
    do { /* Look like single 'C' stmt */	\
	(m)[5] = (pkt)[11];			\
	(m)[4] = (pkt)[10];			\
	(m)[3] = (pkt)[9];			\
	(m)[2] = (pkt)[8];			\
	(m)[1] = (pkt)[7];			\
	(m)[0] = (pkt)[6];			\
    } while (0)

/* sal_mac_addr_t m; */
#define ENET_DSTMAC_GET(pkt, m)			\
    do { /* Look like single 'C' stmt */	\
	(m)[5] = (pkt)[5];			\
	(m)[4] = (pkt)[4];			\
	(m)[3] = (pkt)[3];			\
	(m)[2] = (pkt)[2];			\
	(m)[1] = (pkt)[1];			\
	(m)[0] = (pkt)[0];			\
    } while (0)

#endif	/* !_SOC_ENET_H */
