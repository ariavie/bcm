/*
 * BCM47XX Sonics SiliconBackplane embedded ram core
 *
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
 * $Id: sbsocram.h,v 1.2 Broadcom SDK $
 */

#ifndef	_SBSOCRAM_H
#define	_SBSOCRAM_H

#ifndef _LANGUAGE_ASSEMBLY

/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif	/* PAD */

/* Memcsocram core registers */
typedef volatile struct sbsocramregs {
	uint32	coreinfo;
	uint32	bwalloc;
	uint32	PAD;
	uint32	biststat;
	uint32	bankidx;
	uint32	standbyctrl;
	uint32	PAD[116];
	uint32	pwrctl;		/* corerev >= 2 */
} sbsocramregs_t;

#endif	/* _LANGUAGE_ASSEMBLY */

/* Register offsets */
#define	SR_COREINFO		0x00
#define	SR_BWALLOC		0x04
#define	SR_BISTSTAT		0x0c
#define	SR_BANKINDEX		0x10
#define	SR_BANKSTBYCTL		0x14
#define SR_PWRCTL		0x1e8

/* Coreinfo register */
#define	SRCI_PT_MASK		0x00030000
#define	SRCI_PT_SHIFT		16
/* corerev >= 3 */
#define SRCI_LSS_MASK		0x00f00000
#define SRCI_LSS_SHIFT		20
#define SRCI_LRS_MASK		0x0f000000
#define SRCI_LRS_SHIFT		24

/* In corerev 0, the memory size is 2 to the power of the
 * base plus 16 plus to the contents of the memsize field plus 1.
 */
#define	SRCI_MS0_MASK		0xf
#define SR_MS0_BASE		16

/*
 * In corerev 1 the bank size is 2 ^ the bank size field plus 14,
 * the memory size is number of banks times bank size.
 * The same applies to rom size.
 */
#define	SRCI_ROMNB_MASK		0xf000
#define	SRCI_ROMNB_SHIFT	12
#define	SRCI_ROMBSZ_MASK	0xf00
#define	SRCI_ROMBSZ_SHIFT	8
#define	SRCI_SRNB_MASK		0xf0
#define	SRCI_SRNB_SHIFT		4
#define	SRCI_SRBSZ_MASK		0xf
#define	SRCI_SRBSZ_SHIFT	0

#define SR_BSZ_BASE		14

/* Standby control register */
#define	SRSC_SBYOVR_MASK	0x80000000
#define	SRSC_SBYOVR_SHIFT	31
#define	SRSC_SBYOVRVAL_MASK	0x60000000
#define	SRSC_SBYOVRVAL_SHIFT	29
#define	SRSC_SBYEN_MASK		0x01000000	/* rev >= 3 */
#define	SRSC_SBYEN_SHIFT	24

/* Power control register */
#define SRPC_PMU_STBYDIS_MASK	0x00000010	/* rev >= 3 */
#define SRPC_PMU_STBYDIS_SHIFT	4
#define SRPC_STBYOVRVAL_MASK	0x00000008
#define SRPC_STBYOVRVAL_SHIFT	3
#define SRPC_STBYOVR_MASK	0x00000007
#define SRPC_STBYOVR_SHIFT	0

#endif	/* _SBSOCRAM_H */
