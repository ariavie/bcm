/*
 * $Id: aidmp.h,v 1.2 Broadcom SDK $
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
 * Hardware-specific Receive Data Header for the
 * Broadcom Home Networking Division
 * BCM44XX and BCM47XX 10/100 Mbps Ethernet cores.
 */

#ifndef _bcmenetrxh_h_
#define	_bcmenetrxh_h_

/*
 * The Ethernet MAC core returns an 8-byte Receive Frame Data Header
 * with every frame consisting of
 * 16bits of frame length, followed by
 * 16bits of EMAC rx descriptor info, followed by 32bits of undefined.
 */
typedef volatile struct {
	uint16	len;
	uint16	flags;
	uint16	pad[12];
} bcmenetrxh_t;

#define	RXHDR_LEN	28

#define	RXF_L		((uint16)1 << 11)	/* last buffer in a frame */
#define	RXF_MISS	((uint16)1 << 7)	/* received due to promisc mode */
#define	RXF_BRDCAST	((uint16)1 << 6)	/* dest is broadcast address */
#define	RXF_MULT	((uint16)1 << 5)	/* dest is multicast address */
#define	RXF_LG		((uint16)1 << 4)	/* frame length > rxmaxlength */
#define	RXF_NO		((uint16)1 << 3)	/* odd number of nibbles */
#define	RXF_RXER	((uint16)1 << 2)	/* receive symbol error */
#define	RXF_CRC		((uint16)1 << 1)	/* crc error */
#define	RXF_OV		((uint16)1 << 0)	/* fifo overflow */

#endif	/* _bcmenetrxh_h_ */
