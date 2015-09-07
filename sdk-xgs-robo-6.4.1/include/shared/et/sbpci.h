/*
 * $Id: sbpci.h,v 1.1 Broadcom SDK $
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
 * BCM47XX Sonics SiliconBackplane PCI core hardware definitions.
 */

#ifndef	_SBPCI_H
#define	_SBPCI_H

/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif

/* Sonics side: PCI core and host control registers */
typedef struct sbpciregs {
	uint32 control;		/* PCI control */
	uint32 PAD[3];
	uint32 arbcontrol;	/* PCI arbiter control */
	uint32 PAD[3];
	uint32 intstatus;	/* Interrupt status */
	uint32 intmask;		/* Interrupt mask */
	uint32 sbtopcimailbox;	/* Sonics to PCI mailbox */
	uint32 PAD[9];
	uint32 bcastaddr;	/* Sonics broadcast address */
	uint32 bcastdata;	/* Sonics broadcast data */
	uint32 PAD[2];
	uint32 gpioin;		/* ro: gpio input (>=rev2) */
	uint32 gpioout;		/* rw: gpio output (>=rev2) */
	uint32 gpioouten;	/* rw: gpio output enable (>= rev2) */
	uint32 gpiocontrol;	/* rw: gpio control (>= rev2) */
	uint32 PAD[36];
	uint32 sbtopci0;	/* Sonics to PCI translation 0 */
	uint32 sbtopci1;	/* Sonics to PCI translation 1 */
	uint32 sbtopci2;	/* Sonics to PCI translation 2 */
	uint32 PAD[445];
	uint16 sprom[36];	/* SPROM shadow Area */
	uint32 PAD[46];
} sbpciregs_t;

/* PCI control */
#define PCI_RST_OE	0x01	/* When set, drives PCI_RESET out to pin */
#define PCI_RST		0x02	/* Value driven out to pin */
#define PCI_CLK_OE	0x04	/* When set, drives clock as gated by PCI_CLK out to pin */
#define PCI_CLK		0x08	/* Gate for clock driven out to pin */	

/* PCI arbiter control */
#define PCI_INT_ARB	0x01	/* When set, use an internal arbiter */
#define PCI_EXT_ARB	0x02	/* When set, use an external arbiter */
#define PCI_PARKID_MASK	0x06	/* Selects which agent is parked on an idle bus */
#define PCI_PARKID_SHIFT   1
#define PCI_PARKID_LAST	   0	/* Last requestor */
#define PCI_PARKID_4710	   1	/* 4710 */
#define PCI_PARKID_EXTREQ0 2	/* External requestor 0 */
#define PCI_PARKID_EXTREQ1 3	/* External requestor 1 */

/* Interrupt status/mask */
#define PCI_INTA	0x01	/* PCI INTA# is asserted */
#define PCI_INTB	0x02	/* PCI INTB# is asserted */
#define PCI_SERR	0x04	/* PCI SERR# has been asserted (write one to clear) */
#define PCI_PERR	0x08	/* PCI PERR# has been asserted (write one to clear) */
#define PCI_PME		0x10	/* PCI PME# is asserted */

/* (General) PCI/SB mailbox interrupts, two bits per pci function */
#define	MAILBOX_F0_0	0x100	/* function 0, int 0 */
#define	MAILBOX_F0_1	0x200	/* function 0, int 1 */
#define	MAILBOX_F1_0	0x400	/* function 1, int 0 */
#define	MAILBOX_F1_1	0x800	/* function 1, int 1 */
#define	MAILBOX_F2_0	0x1000	/* function 2, int 0 */
#define	MAILBOX_F2_1	0x2000	/* function 2, int 1 */
#define	MAILBOX_F3_0	0x4000	/* function 3, int 0 */
#define	MAILBOX_F3_1	0x8000	/* function 3, int 1 */

/* Sonics broadcast address */
#define BCAST_ADDR_MASK	0xff	/* Broadcast register address */

/* Sonics to PCI translation types */
#define SBTOPCI0_MASK	0xfc000000
#define SBTOPCI1_MASK	0xfc000000
#define SBTOPCI2_MASK	0xc0000000
#define SBTOPCI_MEM	0
#define SBTOPCI_IO	1
#define SBTOPCI_CFG0	2
#define SBTOPCI_CFG1	3
#define	SBTOPCI_PREF	0x4	/* prefetch enable */
#define	SBTOPCI_BURST	0x8	/* burst enable */

/* PCI side: Reserved PCI configuration registers (see pcicfg.h) */
#define cap_list	rsvd_a[0]
#define bar0_window	dev_dep[0x80 - 0x40]
#define bar1_window	dev_dep[0x84 - 0x40]
#define sprom_control	dev_dep[0x88 - 0x40]

#ifndef _LANGUAGE_ASSEMBLY

extern int sbpci_read_config(void *sbh, uint bus, uint dev, uint func, uint off, void *buf, int len);
extern int sbpci_write_config(void *sbh, uint bus, uint dev, uint func, uint off, void *buf, int len);
extern void sbpci_ban(uint16 core);
extern int sbpci_init(void *sbh);
#ifdef BCMINTERNAL
extern void sbpci_check(void *sbh);
#endif

#endif /* !_LANGUAGE_ASSEMBLY */

#endif	/* _SBPCI_H */
