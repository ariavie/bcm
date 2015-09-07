/*
 * $Id: pci.c,v 1.9 Broadcom SDK $
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
 * PCI memory and configuration space routines.
 *
 * Under Solaris, these routines call the verinet model PLI simulation.
 * All PCI operations are performed talking over a socket to the model.
 *
 * NOTE: the base address used in this file is opaque. This means that
 * the PCI device must first have its base-address programmed by
 * writing the start offset to PCI configuration offset 0x10.
 * Writes to the memory space afterwards requires the address to be
 * based on the base-address plus the offset to the desire register
 * to be accessed.
 */

#include <assert.h>

#include <sal/appl/io.h>
#include <sal/appl/pci.h>

#include "verinet.h"

/*
 * NOTE:  Assume dev->devNo is the verinet unit (for now always zero).
 */

/* In the DV model for the RTL, bit 12 selects Orion */
#define IDSEL 0x1000

#define VALID_DEV(dev) \
	((dev)->busNo == 0 && \
         (dev)->devNo < pli_client_count() && \
         (dev)->funcNo == 0)

/*
 * Initialize the package if not already done
 */

int pci_init_check(void)
{
    static int	 	pli_connected;
    int			client;

    if (! pli_connected) {
	/*
	 * Initialize each PLI client.
	 */

	for (client = 0; client < pli_client_count(); client++) {
	    if (pli_client_attach(client) < 0) {
		printk("pci_init_check: error attaching PLI client %d\n",
		       client);
		return -1;
	    }
	}

	pli_connected = 1;
    }

    return 0;
}

/*
 * Write a DWORD (32 bits) of data to PCI configuration space
 * at the specified offset.
 */

int pci_config_putw(pci_dev_t *dev, uint32 addr, uint32 data)
{
    if(pci_init_check() < 0)
        return -1;

    assert(! (addr & 3));

    if (VALID_DEV(dev))
	return pli_setreg(dev->devNo, PCI_CONFIG, addr | IDSEL, data);
    else
	return -1;
}

/*
 * Read a DWORD (32 bits) of data from PCI configuration space
 * at the specified offset.
 */

uint32 pci_config_getw(pci_dev_t *dev, uint32 addr)
{
    if (pci_init_check() < 0)
	return 0xffffffff;

    assert(! (addr & 3));

    if (VALID_DEV(dev))
	return pli_getreg(dev->devNo, PCI_CONFIG, addr | IDSEL);
    else
	return 0xffffffff;
}

/*
 * Write a DWORD (32 bits) of data to PCI memory space
 * at the specified offset. Return last value at address.
 */

int pci_memory_putw(pci_dev_t *dev, uint32 addr, uint32 data)
{
    if (pci_init_check() < 0)
	return -1;

    return pli_setreg(dev->devNo, PCI_MEMORY, addr, data);
}

/*
 * Read a DWORD (32 bits) of data from PCI memory space.
 */

uint32 pci_memory_getw(pci_dev_t *dev, uint32 addr)
{
    if (pci_init_check() < 0)
	return 0xffffffff;

    return pli_getreg(dev->devNo, PCI_MEMORY, addr);
}

/*
 * pci_int_connect
 *
 *    For PLI simulation, the ISR has already been attached.
 */

int pci_int_connect(int intLine,
		    pci_isr_t isr,
		    void *isr_data)
{
    if (pci_init_check() < 0)
	return -1;

    return pli_register_isr(intLine, isr, isr_data);
}
