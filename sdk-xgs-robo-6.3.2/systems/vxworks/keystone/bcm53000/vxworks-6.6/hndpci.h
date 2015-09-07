/*
 * HND SiliconBackplane PCI core software interface.
 *
 * $Id: hndpci.h 1.3 Broadcom SDK $
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
 */

#ifndef _hndpci_h_
#define _hndpci_h_

/* Starting bus for secondary PCIE port */
#define PCIE_PORT1_BUS_START    (17)

/* Host bridge */
#define PCIE_PORT0_HB_BUS       (1)
#define PCIE_PORT1_HB_BUS       (17)

/* PCIE mapped addresses per port */
#define SI_PCI_MEM(p)           ((p)? SI_PCI1_MEM : SI_PCI0_MEM)
#define SI_PCI_CFG(p)           ((p)? SI_PCI1_CFG : SI_PCI0_CFG)
#define SI_PCIE_DMA_HIGH(p)     ((p)? SI_PCIE1_DMA_H32 : SI_PCIE_DMA_H32)

/* Determine actual port by bus number */
#define PCIE_GET_PORT_BY_BUS(bus)       \
    ((bus) >= PCIE_PORT1_BUS_START ? 1 : 0)

/* Check if the given bus has a host bridge */
#define PCIE_IS_BUS_HOST_BRIDGE(bus)    \
    ((bus == PCIE_PORT0_HB_BUS) || (bus == PCIE_PORT1_HB_BUS))

/* Get bus number that has a host bridge by given port */    
#define PCIE_GET_HOST_BRIDGE_BUS(port)   \
    ((port)? PCIE_PORT1_HB_BUS : PCIE_PORT0_HB_BUS)


extern int hndpci_read_config(si_t *sih, uint bus, uint dev, uint func, uint off, void *buf,
                              int len);
extern int extpci_read_config(si_t *sih, uint bus, uint dev, uint func, uint off, void *buf,
                              int len);
extern int hndpci_write_config(si_t *sih, uint bus, uint dev, uint func, uint off, void *buf,
                               int len);
extern int extpci_write_config(si_t *sih, uint bus, uint dev, uint func, uint off, void *buf,
                               int len);
extern void hndpci_ban(uint16 core, uint8 unit);
extern int hndpci_init(si_t *sih);
extern int hndpci_init_pci(si_t *sih, uint8 port);
extern void hndpci_init_cores(si_t *sih);
extern void hndpci_arb_park(si_t *sih, uint parkid);

#define PCI_PARK_NVRAM    0xff
#define PCIE_ROM_ADDRESS_ENABLE     0x1

#endif /* _hndpci_h_ */
