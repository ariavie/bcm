/*
 * $Id: aiutils_priv.h,v 1.2 Broadcom SDK $
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
 * Include file private to the SOC Interconnect support files.
 */

#ifndef	_aiutils_priv_h_
#define	_aiutils_priv_h_


/* Define SI_VMSG to printf for verbose debugging, but don't check it in */
#define	SI_VMSG(args)

typedef uint32 (*si_intrsoff_t)(void *intr_arg);
typedef void (*si_intrsrestore_t)(void *intr_arg, uint32 arg);
typedef bool (*si_intrsenabled_t)(void *intr_arg);

typedef struct gpioh_item {
	void			*arg;
	bool			level;
	gpio_handler_t		handler;
	uint32			event;
	struct gpioh_item	*next;
} gpioh_item_t;

/* misc si info needed by some of the routines */
typedef struct si_info {
	struct si_pub pub;		/* back plane public state (must be first field) */

	void	*osh;			/* osl os handle */
	void	*sdh;			/* bcmsdh handle */

	uint	dev_coreid;		/* the core provides driver functions */
	void	*intr_arg;		/* interrupt callback function arg */
	si_intrsoff_t intrsoff_fn;	/* turns chip interrupts off */
	si_intrsrestore_t intrsrestore_fn; /* restore chip interrupts */
	si_intrsenabled_t intrsenabled_fn; /* check if interrupts are enabled */

	void *pch;			/* PCI/E core handle */

	gpioh_item_t *gpioh_head; 	/* GPIO event handlers list */

	bool	memseg;			/* flag to toggle MEM_SEG register */

	char *vars;
	uint varsz;

	void	*curmap;		/* current regs va */
	void	*regs[SI_MAXCORES];	/* other regs va */

	uint	curidx;			/* current core index */
	uint	numcores;		/* # discovered cores */
	uint	coreid[SI_MAXCORES];	/* id of each core */
	uint32	coresba[SI_MAXCORES];	/* backplane address of each core */
	void	*regs2[SI_MAXCORES];	/* va of each core second register set (usbh20) */
	uint32	coresba2[SI_MAXCORES];	/* address of each core second register set (usbh20) */
	uint32	coresba_size[SI_MAXCORES]; /* backplane address space size */
	uint32	coresba2_size[SI_MAXCORES]; /* second address space size */

	void	*curwrap;		/* current wrapper va */
	void	*wrappers[SI_MAXCORES];	/* other cores wrapper va */
	uint32	wrapba[SI_MAXCORES];	/* address of controlling wrapper */

	uint32	cia[SI_MAXCORES];	/* erom cia entry for each core */
	uint32	cib[SI_MAXCORES];	/* erom cia entry for each core */
} si_info_t;

#define	SI_INFO(sih)	(si_info_t *)(uint*)sih

#define	GOODCOREADDR(x, b) (((x) >= (b)) && ((x) < ((b) + SI_MAXCORES * SI_CORE_SIZE)) && \
		ISALIGNED((x), SI_CORE_SIZE))
#define	GOODREGS(regs)	((regs) != NULL && ISALIGNED((uint*)(regs), SI_CORE_SIZE))
#define BADCOREADDR	0
#define	GOODIDX(idx)	(((uint)idx) < SI_MAXCORES)
#define	BADIDX		(SI_MAXCORES + 1)
#define	NOREV		-1		/* Invalid rev */

#define PCI(si)		((BUSTYPE((si)->pub.bustype) == PCI_BUS) &&	\
			 ((si)->pub.buscoretype == PCI_CORE_ID))
#define PCIE(si)	((BUSTYPE((si)->pub.bustype) == PCI_BUS) &&	\
			 ((si)->pub.buscoretype == PCIE_CORE_ID))
#define PCMCIA(si)	((BUSTYPE((si)->pub.bustype) == PCMCIA_BUS) && ((si)->memseg == TRUE))			 

/* Newer chips can access PCI/PCIE and CC core without requiring to change
 * PCI BAR0 WIN
 */
#define SI_FAST(si) (((si)->pub.buscoretype == PCIE_CORE_ID) ||	\
		     (((si)->pub.buscoretype == PCI_CORE_ID) && (si)->pub.buscorerev >= 13))

#define PCIEREGS(si) (((char *)((si)->curmap) + PCI_16KB0_PCIREGS_OFFSET))
#define CCREGS_FAST(si) (((char *)((si)->curmap) + PCI_16KB0_CCREGS_OFFSET))

/*
 * Macros to disable/restore function core(D11, ENET, ILINE20, etc) interrupts before/
 * after core switching to avoid invalid register accesss inside ISR.
 */
#define INTR_OFF(si, intr_val) \
	if ((si)->intrsoff_fn && (si)->coreid[(si)->curidx] == (si)->dev_coreid) {	\
		intr_val = (*(si)->intrsoff_fn)((si)->intr_arg); }
#define INTR_RESTORE(si, intr_val) \
	if ((si)->intrsrestore_fn && (si)->coreid[(si)->curidx] == (si)->dev_coreid) {	\
		(*(si)->intrsrestore_fn)((si)->intr_arg, intr_val); }

/* GPIO Based LED powersave defines */
#define DEFAULT_GPIO_ONTIME	10		/* Default: 10% on */
#define DEFAULT_GPIO_OFFTIME	90		/* Default: 10% on */

#ifndef DEFAULT_GPIOTIMERVAL
#define DEFAULT_GPIOTIMERVAL  ((DEFAULT_GPIO_ONTIME << GPIO_ONTIME_SHIFT) | DEFAULT_GPIO_OFFTIME)
#endif

#endif	/* _aiutils_priv_h_ */

