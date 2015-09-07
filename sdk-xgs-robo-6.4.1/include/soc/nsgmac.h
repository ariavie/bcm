/*
 * $Id: nsgmac.h,v 1.1.2.1 Broadcom SDK $
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
 * Broadcom Gigabit Ethernet MAC defines.
 */
#ifndef _nsgmac_h_
#define _nsgmac_h_

/* chip interrupt bit error summary */
#define	I_ERRORS		(GMAC2_INTMASK_DESCRERREN_MASK | \
     GMAC2_INTMASK_DATAERREN_MASK | \
     GMAC2_INTMASK_DESCPROTOERREN_MASK  | GMAC2_INTMASK_XMTUFEN_MASK)
#define	DEF_INTMASK		(GMAC2_INTMASK_XMTINTEN_0_MASK | \
    GMAC2_INTMASK_XMTINTEN_1_MASK | GMAC2_INTMASK_XMTINTEN_2_MASK | \
    GMAC2_INTMASK_XMTINTEN_3_MASK | GMAC2_INTMASK_RCVINTEN_MASK | \
    I_ERRORS)

#define GMAC_RESET_DELAY 	2

/* CHECKME */
#define GMAC_MIN_FRAMESIZE	17	/* gmac can only send frames of
	                                 * size above 17 octetes.
	                                 */

#define LOOPBACK_MODE_DMA	0	/* loopback the packet at the DMA engine */
#define LOOPBACK_MODE_MAC	1	/* loopback the packet at MAC */
#define LOOPBACK_MODE_NONE	2	/* no Loopback */

#define DMAREG(ch, dir, qnum)	((dir == DMA_TX) ? \
	                         (void *)(uint*)(&(ch->regs->gmac2_xmtcontrol_0) + (0x10 * qnum)) : \
	                         (void *)(uint*)(&(ch->regs->gmac2_rcvcontrol) + (0x10 * qnum)))

/*
 * Add multicast address to the list. Multicast address are maintained as
 * hash table with chaining.
 */
typedef struct mclist {
	struct ether_addr mc_addr;	/* multicast address to allow */
	struct mclist *next;		/* next entry */
} mflist_t;

#define GMAC_HASHT_SIZE		16	/* hash table size */
#define GMAC_MCADDR_HASH(m)	((((uint8 *)(m))[3] + ((uint8 *)(m))[4] + \
	                         ((uint8 *)(m))[5]) & (GMAC_HASHT_SIZE - 1))

#define ETHER_MCADDR_CMP(x, y) ((((uint16 *)(x))[0] ^ ((uint16 *)(y))[0]) | \
				(((uint16 *)(x))[1] ^ ((uint16 *)(y))[1]) | \
				(((uint16 *)(x))[2] ^ ((uint16 *)(y))[2]))

#define SUCCESS			0
#define FAILURE			-1
#define TIMEOUT                 -2

typedef struct mcfilter {
					/* hash table for multicast filtering */
	mflist_t *bucket[GMAC_HASHT_SIZE];
} mcfilter_t;

#endif /* _nsgmac_h_ */

