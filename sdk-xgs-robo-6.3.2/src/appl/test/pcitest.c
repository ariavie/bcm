/*
 * $Id: pcitest.c 1.16 Broadcom SDK $
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
 * PCI Tests
 */

#include <sal/types.h>

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <soc/debug.h>
#include <sal/appl/pci.h>
#include <ibde.h>
#include "testlist.h"

#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT)

/*
 * These numbers need to be chosen not to conflict with other read data
 * in pca table.  Chip specific values are substituted for them in
 * setup_pca_table.
 *
 */
#define	DEVID	0x12
#define	VENID	0x13
#define	REVID	0x14
#define	ID	0x15	/* 0xff00ffff to accommodate bondouts */

#define	ILINE	0
#define	IPIN	(PCI_INTERRUPT_A << 8)

typedef struct pci_config_addr_s {
    uint32	pca_flags;		/* Flags - see below */
#   define	RD	0x01		/* Do read */
#   define	WR	0x02		/* Do write */
    uint32	pca_addr;		/* Address to muck with */
    uint32	pca_write;		/* Write Value */
    uint32	pca_read;		/* Expected read value */
    uint32	pca_read_mask;		/* Mask actual read with this */
} pca_t;

typedef struct pt_s {
#   define	PT_REGS ((int)((PCI_CONF_MAX_LATENCY + 3) / sizeof(uint32)))
    uint32	pt_config[PT_REGS];
} pt_t;

pt_t	pt_config[SOC_MAX_NUM_DEVICES];

static const pca_t	pca_table_fixed[] = {
/* read/write	Config Address      w-data     r-data 	 	Mask	   */
/* -----------+------------------+------------+--------------+------------ */

    /* Test Device/Vendor ID for Read/Write */

    {RD, 	PCI_CONF_VENDOR_ID,	0,		VENID,		0x0000ffff},
    /* High byte of DEVID only, to accommodate different bond-outs */
    {RD, 	PCI_CONF_VENDOR_ID,	0,		DEVID,		0xff000000},
    {RD+WR,	PCI_CONF_VENDOR_ID,	0xaaaaaaaa,	ID,		0xff00ffff},
    {RD+WR,	PCI_CONF_VENDOR_ID,	0x55555555,	ID,		0xff00ffff},
    {RD+WR,	PCI_CONF_VENDOR_ID,	0xffffffff,	ID,		0xff00ffff},
    {RD+WR,	PCI_CONF_VENDOR_ID,	0x00000000,	ID,		0xff00ffff},

    /* Test MBAR0 - should be 64K + anywhere in 64-bit space */

    /* {RD+WR,	PCI_CONF_BASE0,	   0xffffffff,	0xffff0004,	0xffffffff},*/

    /* Class Code and revision for Read Only */
    /* Excludes metal-spin portion of Rev ID (lower 4 bits) */

    {RD, 	PCI_CONF_REVISION_ID,	   0,		REVID,		0xff7fffff},
    {RD+WR, 	PCI_CONF_REVISION_ID,	   0,		REVID,		0xff7fffff},
    {RD+WR, 	PCI_CONF_REVISION_ID,	   0x55555555,	REVID,		0xff7fffff},
    {RD+WR, 	PCI_CONF_REVISION_ID,	   0xaaaaaaaa,	REVID,		0xff7fffff},
    {RD+WR, 	PCI_CONF_REVISION_ID,	   0xffffffff,	REVID,		0xff7fffff},

    /* Int pin indication */

    {RD, 	PCI_CONF_INTERRUPT_LINE,	   0,		IPIN,		0xff00},
    {RD+WR, 	PCI_CONF_INTERRUPT_LINE,	   0,		IPIN,		0xff00},
    {RD+WR, 	PCI_CONF_INTERRUPT_LINE,	   0x55555555,	IPIN,		0xff00},
    {RD+WR, 	PCI_CONF_INTERRUPT_LINE,	   0xaaaaaaaa,	IPIN,		0xff00},
    {RD+WR, 	PCI_CONF_INTERRUPT_LINE,	   0xffffffff,	IPIN,		0xff00},

    /* Interrupt line assignment */

    {RD+WR, 	PCI_CONF_INTERRUPT_LINE,	   0,		0,		0xff},
    {RD+WR, 	PCI_CONF_INTERRUPT_LINE,	   0,		0,		0xff},
    {RD+WR, 	PCI_CONF_INTERRUPT_LINE,	   0x55555555,	0x55,		0xff},
    {RD+WR, 	PCI_CONF_INTERRUPT_LINE,	   0xaaaaaaaa,	0xaa,		0xff},
    {RD+WR, 	PCI_CONF_INTERRUPT_LINE,	   0xffffffff,	0xff,		0xff}
};

static int pca_cnt = COUNTOF(pca_table_fixed);
static pca_t pca_table[COUNTOF(pca_table_fixed)];

/*
 * Function to set up a chips expected PCI table for testing
 */
static void
setup_pca_table(int unit)
{
    int i;
    uint16 dev_id;
    uint8 rev_id;

    /* Get the real chip ID, as opposed to driver chip ID */
    soc_cm_get_id(unit, &dev_id, &rev_id);

    memcpy(pca_table, pca_table_fixed, sizeof(pca_table_fixed));
    for (i=0; i<pca_cnt; i++) {
        switch (pca_table_fixed[i].pca_read) {
        case VENID:
            pca_table[i].pca_read = SOC_PCI_VENDOR(unit);
            break;
        case DEVID:
	    /* High byte only (see comment above) */
            pca_table[i].pca_read = (dev_id << 16) & 0xff000000;
            if (soc_property_get_str(unit, spn_PCI_OVERRIDE_DEV) != NULL) {
                /* Ignore if PCI DEV ID override configured */
                pca_table[i].pca_read &= ~0xFFFF0000;
                pca_table[i].pca_read_mask &= ~0xFFFF0000;
            }
            break;
        case REVID:
            pca_table[i].pca_read = (rev_id | PCI_NETWORK_CLASS_ID << 8);
            if (soc_property_get_str(unit, spn_PCI_OVERRIDE_REV) != NULL) {
                /* Ignore if PCI REV ID override configured */
                pca_table[i].pca_read &= ~0x000000FF;
                pca_table[i].pca_read_mask &= ~0x000000FF;
            }
            pca_table[i].pca_read &= pca_table[i].pca_read_mask;
            break;
        case ID:
            pca_table[i].pca_read =
		((dev_id << 16) |
		 (SOC_PCI_VENDOR(unit))) & 0xff00ffff;
            if (soc_property_get_str(unit, spn_PCI_OVERRIDE_DEV) != NULL) {
                /* Ignore if PCI DEV ID override configured */
                pca_table[i].pca_read &= ~0xFFFF0000;
                pca_table[i].pca_read_mask &= ~0xFFFF0000;
            }
            break;
        }
    }
}


int
pci_test(int u, args_t *a, void *pa)
/*
 * Function: 	pci_test
 * Purpose:	Test basic PCI stuff on StrataSwitch.
 * Parameters:	u - unit #.
 *		a - pointer to arguments.
 *		pa - ignored cookie.
 * Returns:	0
 */
{
    int		i;

    COMPILER_REFERENCE(a);
    COMPILER_REFERENCE(pa);

    setup_pca_table(u);


    for (i = 0; i < pca_cnt; i++) {
	pca_t *p = &pca_table[i];
	uint32	read_val;

	/* If write - do write first */

	if (p->pca_flags & WR) {
	    soc_cm_debug(DK_TESTS+DK_VERBOSE, "Writing PCI Config 0x%x <--- 0x%x\n",
                         p->pca_addr, p->pca_write);
	    if (bde->pci_conf_write(u, p->pca_addr, p->pca_write)){
		test_error(u, "PCI config write failed to address: 0x%x\n",
			   p->pca_addr);
		continue;		/* If test error returns */
	    }
	}
	read_val = bde->pci_conf_read(u, p->pca_addr);
	read_val &= p->pca_read_mask;
	soc_cm_debug(DK_TESTS+DK_VERBOSE, "Reading PCI Config (Masked) 0x%x --> 0x%x\n", 
                     p->pca_addr, read_val);
	if (read_val != p->pca_read) {
	    test_error(u, "PCI Config @0x%x Read 0x%x expected 0x%x\n",
		       p->pca_addr, read_val, p->pca_read);
	}
    }

    return(0);
}

/*ARGSUSED*/
int
pci_test_done(int u, void *pa)
/*
 * Function: 	pci_test_done
 * Purpose:	Restore all values to CMIC from soc structure.
 * Parameters:	u - unit #
 *		pa - cookie (Ignored)
 * Returns:	0 - OK
 *		-1 - failed
 */
{
    pt_t	*pt;
    int		i;
    int		rv = 0;

    if (!pa) {		/* Never got far enough --- just return success */
	return(0);
    }
    pt = (pt_t *)pa;

    for (i = 0; i < PT_REGS; i++) {      
      rv |= bde->pci_conf_write(u, i * 4, pt->pt_config[i]);
    }
    return(rv);
}

int
pci_test_init(int u, args_t *a, void **pa)
/*
 * Function: 	pci_test_init
 * Purpose:	Save all the current PCI Config registers to write
 *		on completion.
 * Parameters:	u - unit #
 *		a - pointer to args
 *		pa - Pointer to cookie
 * Returns:	0 - success, -1 - failed.
 */
{
    pt_t	*pt = &pt_config[u];
    int		i;

    if (ARG_CNT(a)) {
	test_error(u, "Arguments not supported\n");
	return -1;
    }

    for (i = 0; i < PT_REGS; i++) {
	pt->pt_config[i] = bde->pci_conf_read(u, i * 4);
    }
    *pa = pt;
    return(0);
}

int
pci_sch_test(int u, args_t *a, void *pa)
/*
 * Function: 	pci_sch_test
 * Purpose:	Run test patterns through CMIC_SCHAN_MESSAGE buffer
 * Parameters:	u - unit #.
 *		a - pointer to arguments.
 *		pa - ignored cookie.
 * Returns:	0
 */
{
    COMPILER_REFERENCE(a);
    COMPILER_REFERENCE(pa);

    if (soc_pci_test(u) < 0) {
	test_error(u, "CMIC PCI S-Channel Buffer test failed\n");
    }

    return 0;	/* If test_error() returns */
}

#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT */
