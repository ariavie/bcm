/*
 * $Id: bcmenetmib.h 1.2 Broadcom SDK $
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
 * Hardware-specific MIB definition for
 * Broadcom Home Networking Division
 * BCM44XX and BCM47XX 10/100 Mbps Ethernet cores.
 */

#ifndef _bcmenetmib_h_
#define _bcmenetmib_h_

/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif	/* PAD */

/*
 * EMAC MIB Registers
 */
typedef volatile struct {
	uint32 tx_good_octets;
	uint32 tx_good_pkts;
	uint32 tx_octets;
	uint32 tx_pkts;
	uint32 tx_broadcast_pkts;
	uint32 tx_multicast_pkts;
	uint32 tx_len_64;
	uint32 tx_len_65_to_127;
	uint32 tx_len_128_to_255;
	uint32 tx_len_256_to_511;
	uint32 tx_len_512_to_1023;
	uint32 tx_len_1024_to_max;
	uint32 tx_jabber_pkts;
	uint32 tx_oversize_pkts;
	uint32 tx_fragment_pkts;
	uint32 tx_underruns;
	uint32 tx_total_cols;
	uint32 tx_single_cols;
	uint32 tx_multiple_cols;
	uint32 tx_excessive_cols;
	uint32 tx_late_cols;
	uint32 tx_defered;
	uint32 tx_carrier_lost;
	uint32 tx_pause_pkts;
	uint32 PAD[8];

	uint32 rx_good_octets;
	uint32 rx_good_pkts;
	uint32 rx_octets;
	uint32 rx_pkts;
	uint32 rx_broadcast_pkts;
	uint32 rx_multicast_pkts;
	uint32 rx_len_64;
	uint32 rx_len_65_to_127;
	uint32 rx_len_128_to_255;
	uint32 rx_len_256_to_511;
	uint32 rx_len_512_to_1023;
	uint32 rx_len_1024_to_max;
	uint32 rx_jabber_pkts;
	uint32 rx_oversize_pkts;
	uint32 rx_fragment_pkts;
	uint32 rx_missed_pkts;
	uint32 rx_crc_align_errs;
	uint32 rx_undersize;
	uint32 rx_crc_errs;
	uint32 rx_align_errs;
	uint32 rx_symbol_errs;
	uint32 rx_pause_pkts;
	uint32 rx_nonpause_pkts;
} bcmenetmib_t;

#endif	/* _bcmenetmib_h_ */
