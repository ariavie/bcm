/*
 * $Id: plibde.c 1.34.6.1 Broadcom SDK $
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

#include "plibde.h"

#include <sal/types.h>
#include <sal/appl/pci.h>
#include <sal/appl/sal.h>
#include <soc/devids.h>
#include <soc/defs.h>
#include <sal/appl/io.h>
#include <sal/appl/config.h>
#include <sal/core/libc.h>
#include <sal/core/boot.h>
#include <sal/core/alloc.h>
#include <shared/util.h>

/* 
 * PLI BDE
 *
 * Currently, the implementation is 
 * based directly on the old SAL architecture. 
 * As soon as the BDE migration is complete and tested, 
 * the old SAL architecture will be remove, 
 * and it will be implemented properly. 
 *
 */
#define PLI_PCI_MBAR0        0x00000000
#define PLI_PCI_ILINE(dev)    (0 + (dev))

typedef struct vxbde_dev_s {
    ibde_dev_t bde_dev;
    pci_dev_t pci_dev;
    uint32 cmic_base;
} plibde_dev_t;

#if !defined(PLI_MAX_DEVICES)
#define PLI_MAX_DEVICES 16
#endif

static plibde_dev_t _devices[PLI_MAX_DEVICES];
static int _n_devices = 0;

static const char*
_name(void)
{
    return "pli-pci-bde";
}

static int 
_num_devices(int type)
{
    switch (type) {
    case BDE_ALL_DEVICES:
    case BDE_SWITCH_DEVICES:
        return _n_devices; 
    case BDE_ETHER_DEVICES:
        return 0; 
    }

    return 0;
}

static const ibde_dev_t*
_get_dev(int d)
{
    if(d < 0 || d > _n_devices) {
    return NULL;
    }
    return &_devices[d].bde_dev;
}

static uint32
_get_dev_type(int d)
{
    return BDE_PCI_DEV_TYPE;
}


static uint32 
_pci_read(int d, uint32 addr)
{
    return  pci_config_getw(&_devices[d].pci_dev, addr);
}

static int
_pci_write(int d, uint32 addr, uint32 data)
{
    return pci_config_putw(&_devices[d].pci_dev, addr, data);
}

static void
_pci_bus_features(int unit, int* be_pio, int* be_packet, int* be_other)
{
#ifdef SYS_BE_PIO
    *be_pio = SYS_BE_PIO;
#else
    *be_pio = 0;
#endif
    
#ifdef SYS_BE_PACKET
    *be_packet = SYS_BE_PACKET;
#else
    *be_packet = (sal_boot_flags_get() & BOOT_F_RTLSIM) ? 1 : 0;
#endif

#ifdef SYS_BE_OTHER
    *be_other = SYS_BE_OTHER;
#else
    *be_other = 0;
#endif
}
  

static uint32  
_read(int d, uint32 addr)
{
    return pci_memory_getw(&_devices[d].pci_dev, 
               PLI_PCI_MBAR0 + _devices[d].cmic_base + addr); 
}

static int
_write(int d, uint32 addr, uint32 data)
{
    pci_memory_putw(&_devices[d].pci_dev, 
            PLI_PCI_MBAR0 + _devices[d].cmic_base + addr, data);
    return 0;
}

static uint32* 
_salloc(int d, int size, const char *name)
{
    return sal_dma_alloc(size, (char *)name);
}

static void
_sfree(int d, void* ptr)
{
    sal_dma_free(ptr);
}

static int 
_sflush(int d, void* addr, int length)
{
    sal_dma_flush(addr, length);
    return 0;
}

static int
_sinval(int d, void* addr, int length)
{
    sal_dma_inval(addr, length);
    return 0;
}

static int 
_interrupt_connect(int d, void (*isr)(void*), void* data)
{
    return pci_int_connect(PLI_PCI_ILINE(d), isr, data);
}
             
static int
_interrupt_disconnect(int d)
{
    return 0;
}
   
static uint32 
_l2p(int d, void* laddr)
{
    return PTR_TO_INT(laddr);
}

static uint32*
_p2l(int d, uint32 paddr)
{
    return INT_TO_PTR(paddr);
}

static uint32  
_iproc_read(int d, uint32 addr)
{
    return pci_memory_getw(&_devices[d].pci_dev, 
               PLI_PCI_MBAR0 + addr); 
}

static int
_iproc_write(int d, uint32 addr, uint32 data)
{
    pci_memory_putw(&_devices[d].pci_dev, 
            PLI_PCI_MBAR0 + addr, data);
    return 0;
}


static ibde_t _ibde = {
    _name, 
    _num_devices, 
    _get_dev, 
    _get_dev_type, 
    _pci_read,
    _pci_write,
    _pci_bus_features,
    _read,
    _write,
    _salloc,
    _sfree,
    _sflush,
    _sinval,
    _interrupt_connect,
    _interrupt_disconnect,
    _l2p,
    _p2l,
    NULL, /* spi_read */
    NULL, /* spi_write */
    _iproc_read,
    _iproc_write,
};

static int
_setup(pci_dev_t* dev, 
       uint16 pciVenID, 
       uint16 pciDevID,
       uint8 pciRevID)
{
    plibde_dev_t*       vxd;
    uint32              tmp;
    int                 devno;

    if((pciVenID != BROADCOM_VENDOR_ID) && (pciVenID != SANDBURST_VENDOR_ID)) {
    return 0;
    }

    /* don't want to expose non 56XX devices */
    if (pciVenID == BROADCOM_VENDOR_ID) {
        if(((pciDevID & 0xFF00) != 0x5600) &&
           ((pciDevID & 0xF000) != 0xc000) &&
       ((pciDevID & 0xFFF0) != 0x0230) &&
           ((pciDevID & 0xF000) != 0xb000) &&
           (pciDevID != BCM88030_DEVICE_ID) &&
           ((pciDevID & 0xFFF0) != BCM88650_DEVICE_ID) &&
           ((pciDevID & 0xFFF0) != BCM88750_DEVICE_ID) &&
           ((pciDevID & 0xFFF0) != BCM88660_DEVICE_ID) &&
           ((pciDevID & 0xFFF0) != BCM88850_DEVICE_ID))
           {
            return 0;
        }
    }


    /* Pay dirt */
    devno = _n_devices++;
    vxd = _devices + devno;
  
    vxd->bde_dev.device = pciDevID;
    vxd->bde_dev.rev = pciRevID;
    vxd->bde_dev.base_address = 0; /* Device not memory accessible */
    vxd->pci_dev = *dev;

    /* Configure PCI
     * On PLI, to speed things up, we set these PCI config location to...
     * 
     * latency timer : 32'h0000_100c = 48<<8
     * enable fast back-to-back: 32'h0000_1004 = 32'h0000_0356    
     *
     */

    
    /* Write control word (turns on parity detect) */
    pci_config_putw(dev, PCI_CONF_COMMAND, (PCI_CONF_COMMAND_BM |
                                            PCI_CONF_COMMAND_MS |
                                            PCI_CONF_COMMAND_PERR |
                                            PCI_CONF_COMMAND_FBBE));
    /* Write base address */
    pci_config_putw(dev, PCI_CONF_BAR0, PLI_PCI_MBAR0);
  
    /* read back */
    if((tmp = pci_config_getw(dev, PCI_CONF_BAR0) & PCI_CONF_BAR_MASK) !=
       PLI_PCI_MBAR0) {
    printk("pli-bde: warning: read back of MBAR0 failed.\n");
    printk("pli-bde: warning: expected 0x%.8x, read 0x%.8x\n", 
           PLI_PCI_MBAR0, tmp);
    }

    /* Write something to int line */
    tmp = pci_config_getw(dev, PCI_CONF_INTERRUPT_LINE);
    tmp = (tmp & ~0xff) | PLI_PCI_ILINE(devno);
    pci_config_putw(dev, PCI_CONF_INTERRUPT_LINE, tmp);

    /* Set latency timer to 48 */
    tmp = pci_config_getw(dev, PCI_CONF_CACHE_LINE_SIZE);
    tmp |= (48 << 8);
    pci_config_putw(dev, PCI_CONF_CACHE_LINE_SIZE, tmp);

    switch (pciDevID) {
    case BCM56340_DEVICE_ID:
    case BCM56150_DEVICE_ID:
    case BCM56450_DEVICE_ID:
        /* Ensure we work with BCMSIM */
        vxd->cmic_base = 0x48000000;
        break;
    default:
        vxd->cmic_base = pci_config_getw(dev, PCI_CONF_BAR1) & PCI_CONF_BAR_MASK;
        break;
    }
    printk("pli-bde: cmic_base = 0x%08x\n", vxd->cmic_base);

    /* Have at it */
    return 0;
}
    

int
plibde_create(ibde_t** bde)
{
    if(_n_devices == 0) {
    pci_device_iter(_setup);
    }
    *bde = &_ibde;
    return 0;  
}


/*
 * Perform a complete  SCHANNEL operation. 
 * This is a backdoor into the simulator to 
 * improve performance. 
 */

int 
plibde_schan_op(int unit,
                uint32* msg,
                int dwc_write, int dwc_read)
{
    /* in pli.c */
    extern int pli_schan(int devNo, uint32* words, int dw_write, int dw_read); 
    return pli_schan(unit, msg, dwc_write, dwc_read); 
}
    
