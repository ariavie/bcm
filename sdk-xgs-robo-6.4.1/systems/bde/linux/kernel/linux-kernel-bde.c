/*
 * $Id: linux-kernel-bde.c,v 1.414 Broadcom SDK $
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
 * Linux Kernel BDE
 *
 *
 * DMA memory allocation modes
 * ===========================
 *
 * 1. Using private pool in kernel memory
 * --------------------------------------
 * In this mode the BDE module will try to assemble a physically contiguous
 * of memory using the kernel page allocator. This memory block is then
 * administered by the mpool allocation functions. Note that once a system
 * has been running for a while, the memory fragmentation may prevent the
 * allocator from assembling a contiguous memory block, however, if the
 * module is loaded shortly after system startup, it is very unlikely to
 * fail.
 *
 * This allocation method is used by default.
 *
 * 2. Using private pool in high memory
 * ------------------------------------
 * In this mode the BDE module will assume that unused physical memory is
 * present at the high_memory address, i.e. memory not managed by the Linux
 * memory manager. This memory block is mapped into kernel space and
 * administered by the mpool allocation functions. High memory must be
 * reserved using either the mem=xxx kernel parameter (recommended), or by
 * hardcoding the memory limit in the kernel image.
 *
 * The module parameter himem=1 enables this allocation mode.
 *
 * 3. Using kernel allocators (kmalloc, __get_free_pages)
 * ------------------------------------------------------
 * In this mode all DMA memory is allocated from the kernel on the fly, i.e.
 * no private DMA memory pool will be created. If large memory blocks are
 * only allocated at system startup (or not at all), this allocation method
 * is the most flexible and memory-efficient, however, it is not recommended
 * for non-coherent memory platforms due to an overall system performance
 * degradation arising from the use of cache flush/invalidate instructions.
 *
 * The module parameter dmasize=0M enables this allocation mode, however if
 * DMA memory is requested from a user mode application, a private memory
 * pool will be created and used irrespectively.
 */

#include <gmodule.h>
#include <linux-bde.h>
#include <mpool.h>
#include <linux/delay.h>
#include <sdk_config.h>
#include <soc/devids.h>
#include <soc/cmic.h>

#ifdef BCM_PLX9656_LOCAL_BUS
#include <asm/cacheflush.h>
#endif

#ifdef BCM_ROBO_SUPPORT
/* robo/et related header files */
#include <shared/et/typedefs.h>

#include <shared/et/sbconfig.h>

#if defined(KEYSTONE)
#include <shared/et/aiutils.h>
#include <sbchipc.h>
#include <etc_robo_spi.h>
#include <soc/gmac0_core.h>
#elif defined(IPROC_CMICD)
#include <shared/et/aiutils.h>
#include <sbchipc.h>
#include <robo_srab.h>
#include <soc/nsgmac2reg.h>
#else /* BCM4704 */
#include <shared/et/sbutils.h>
#include <etc_robo.h>
#endif 
#endif /* BCM_ROBO_SUPPORT */

#if _SIMPLE_MEMORY_ALLOCATION_
#include <linux/dma-mapping.h>
#if defined(IPROC_CMICD) && defined(CONFIG_CMA) && defined(CONFIG_CMA_SIZE_MBYTES)
#define DMA_MAX_ALLOC_SIZE (CONFIG_CMA_SIZE_MBYTES * 1024 * 1024)
#else
#define DMA_MAX_ALLOC_SIZE (1 << (MAX_ORDER - 1 + PAGE_SHIFT)) /* Maximum size the kernel can allocate in one allocation */
#endif /* _SIMPLE_MEMORY_ALLOCATION_ */
#endif

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("Kernel BDE");
MODULE_LICENSE("Proprietary");

/* DMA memory pool size */
static char *dmasize;
LKM_MOD_PARAM(dmasize, "s", charp, 0);
MODULE_PARM_DESC(dmasize,
"Specify DMA memory size (default 4MB)");

/* Use high memory for DMA */
static char *himem;
LKM_MOD_PARAM(himem, "s", charp, 0);
MODULE_PARM_DESC(himem,
"Use high memory for DMA (default no)");

/* PCIe max payload */
int maxpayload = 128;
LKM_MOD_PARAM(maxpayload, "i", int, 0);
MODULE_PARM_DESC(maxpayload,
"Limit maximum payload size and request size on PCIe devices");

/* Use MSI interrupts */
int usemsi = -1;
LKM_MOD_PARAM(usemsi, "i", int, 0);
MODULE_PARM_DESC(usemsi,
"Use MSI interrupts if supported by kernel");

/* Ignore all recognized devices (for debug purposes) */
int nodevices;
LKM_MOD_PARAM(nodevices, "i", int, 0);
MODULE_PARM_DESC(nodevices,
"Ignore all recognized devices (default no)");

/*
 * This usually is defined at /usr/include/linux/pci_ids.h
 * But this ID is newer.
 */
#ifndef PCI_DEVICE_ID_PLX_9656
#define PCI_DEVICE_ID_PLX_9656 0x9656
#endif

#ifndef PCI_DEVICE_ID_PLX_9056
#define PCI_DEVICE_ID_PLX_9056 0x9056
#endif

/* local defined device IDs, refer to bcmdevs.h */
#ifndef BCM53000_GMAC_ID
#define BCM53000_GMAC_ID      0x4715      /* 53003 gmac id */
#endif
#ifndef BCM53010_GMAC_ID
#define BCM53010_GMAC_ID      0x4715      /* 5301x gmac id */
#endif
#ifndef BCM47XX_ENET_ID
#define BCM47XX_ENET_ID       0x4713      /* 4710 enet */
#endif
#ifndef BCM53010_CHIP_ID
#define BCM53010_CHIP_ID      0xcf12      /* 53010 chipcommon chipid */
#endif
#ifndef BCM53018_CHIP_ID
#define BCM53018_CHIP_ID      0xcf1a      /* 53018 chipcommon chipid */
#endif
#ifndef BCM53020_CHIP_ID
#define BCM53020_CHIP_ID      0xcf1e      /* 53020 chipcommon chipid */
#endif

/* For 2.4.x kernel support */
#ifndef IRQF_SHARED
#define IRQF_SHARED     SA_SHIRQ
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
typedef unsigned long resource_size_t;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18) */

#ifdef BCM_ICS
#define BCM_ICS_CMIC_BASE       0x08000000
#else

/* Force interrupt line */
static int forceirq = -1;
static uint32_t forceirqubm = 0xffffffff;
LKM_MOD_PARAM(forceirq, "i", int, 0);
LKM_MOD_PARAM(forceirqubm, "i", uint, 0);
MODULE_PARM_DESC(forceirq,
"Override IRQ line assigned by boot loader");
MODULE_PARM_DESC(forceirqubm,
"Bitmap for overriding the IRQ line assigned by boot loader for given units");

/* Create SPI slave device (cannot be probed) */
static uint32_t spi_devid = 0;
LKM_MOD_PARAM(spi_devid, "i", uint, 0);
MODULE_PARM_DESC(spi_devid,
"Create SPI slave device using this device ID");

/* Select SPI device revision (cannot be probed) */
static uint32_t spi_revid = 1;
LKM_MOD_PARAM(spi_revid, "i", uint, 1);
MODULE_PARM_DESC(spi_revid,
"Select device revision for SPI slave device");

#endif /* BCM_ICS */

/* Debug output */
static int debug;
LKM_MOD_PARAM(debug, "i", int, 0);
MODULE_PARM_DESC(debug,
"Set debug level (default 0");
/* Use high memory for DMA */

/* module param for probing EB devices. */
static char *eb_bus;
LKM_MOD_PARAM(eb_bus, "s", charp, 0);
MODULE_PARM_DESC(eb_bus,
"List of EB devices on platform. Input format (BA=%x IRQ=%d RD16=%d WR16=%d");

#ifdef KEYSTONE
/* Force SPI Frequency */
static int spifreq = 0;
LKM_MOD_PARAM(spifreq, "i", int, 0);
MODULE_PARM_DESC(spifreq,
"Force SPI Frequency for Keystone CPU (0 for default frequency)");
#endif

#if defined(BCM_EA_SUPPORT) 
#if defined(BCM_TK371X_SUPPORT)
static int eadevices;
LKM_MOD_PARAM(eadevices, "i", int, 0);
MODULE_PARM_DESC(eadevices,
"Number of TK371X devices");
#endif /* */
#endif /* BCM_EA_SUPPORT */

/* Compatibility */
#ifdef LKM_2_4
#define _ISR_RET void
#define _ISR_PARAMS(_i,_d,_r) int _i, void *_d, struct pt_regs *_r
#define IRQ_NONE
#define IRQ_HANDLED
#define MEM_MAP_RESERVE mem_map_reserve
#define MEM_MAP_UNRESERVE mem_map_unreserve
#define SYNC_IRQ(_i) synchronize_irq()
#else /* LKM_2_6 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
#define _ISR_RET irqreturn_t
#else
#define _ISR_RET int
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
#define _ISR_PARAMS(_i,_d,_r) int _i, void *_d
#else
#define _ISR_PARAMS(_i,_d,_r) int _i, void *_d, struct pt_regs *_r
#endif
#define MEM_MAP_RESERVE SetPageReserved
#define MEM_MAP_UNRESERVE ClearPageReserved
#define SYNC_IRQ(_i) synchronize_irq(_i)
char * ___strtok;
char * strtok(char * s,const char * ct)
{
    char *sbegin, *send;
    sbegin  = s ? s : ___strtok;
    if (!sbegin) {
        return NULL;
    }
    sbegin += strspn(sbegin,ct);
    if (*sbegin == '\0') {
        ___strtok = NULL;
        return( NULL );
    }
    send = strpbrk( sbegin, ct);
    if (send && *send != '\0')
        *send++ = '\0';
    ___strtok = send;
    return (sbegin);
}
LKM_EXPORT_SYM(___strtok);
LKM_EXPORT_SYM(strtok);
#endif /* LKM_2_x */

#ifndef DMA_BIT_MASK
#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#endif

#ifndef GFP_DMA32
#define GFP_DMA32 0
#endif

/* PCIe capabilities */
#ifndef PCI_CAP_ID_EXP
#define PCI_CAP_ID_EXP          0x10
#endif
#ifndef PCI_EXP_DEVCAP
#define PCI_EXP_DEVCAP          4
#endif
#ifndef PCI_EXP_DEVCTL
#define PCI_EXP_DEVCTL          8
#endif
#ifndef PCI_EXT_CAP_START
#define PCI_EXT_CAP_START       0x100
#endif
#ifndef PCI_EXT_CAP_ID
#define PCI_EXT_CAP_ID(_hdr)    (_hdr & 0x0000ffff)
#endif
#ifndef PCI_EXT_CAP_VER
#define PCI_EXT_CAP_VER(_hdr)   ((_hdr >> 16) & 0xf)
#endif
#ifndef PCI_EXT_CAP_NEXT
#define PCI_EXT_CAP_NEXT(_hdr)  ((_hdr >> 20) & 0xffc)
#endif
#ifndef PCI_EXT_CAP_ID_VNDR
#define PCI_EXT_CAP_ID_VNDR     0x0b
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
#define PCI_FIND_DEV(_d, _v, _fr)       pci_find_device(_d, _v, _fr)
#else
#define PCI_FIND_DEV(_d, _v, _fr)       pci_get_device(_d, _v, _fr)
#endif

#if defined(CONFIG_RESOURCES_64BIT) || defined(CONFIG_PHYS_ADDR_T_64BIT)
#define PHYS_ADDR_IS_64BIT
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21))
#define VIRT_TO_PAGE(p)     virt_to_page((void*)(p))
#else
#define VIRT_TO_PAGE(p)     virt_to_page((p))
#endif

#if defined(CONFIG_IDT_79EB334) || defined(CONFIG_BCM4702)
/* ioremap is broken in kernel */
#define IOREMAP(addr, size) ((void *)KSEG1ADDR(addr))
#else
#define IOREMAP(addr, size) ioremap_nocache(addr, size)
#endif

#if defined (__mips__)
#if defined(CONFIG_NONCOHERENT_IO) || defined(CONFIG_DMA_NONCOHERENT)
/* Use flush/invalidate for cached memory */
#define NONCOHERENT_DMA_MEMORY
/* Remap virtual DMA addresses to non-cached segment */
#define REMAP_DMA_NONCACHED
#endif /* CONFIG_NONCOHERENT_IO || CONFIG_DMA_NONCOHERENT */
#endif /* __mips__ */

#if defined(BCM958525)
#define REMAP_DMA_NONCACHED
#endif

/* Structure of private SPI device */
struct spi_dev {
    uint8          cid;         /* Chip ID */
    uint32          part;        /* Part number of the chip */
    uint8          rev;         /* Revision of the chip */
    void           *robo;       /* ptr to robo info required to access SPI */
    unsigned short phyid_high;  /* PHYID HIGH in MII regs of detected chip */
    unsigned short phyid_low;   /* PHYID LOW in MII regs of detected chip */
};

struct bde_spi_device_id {
    unsigned short phyid_high;  /* PHYID HIGH in MII regs of detected chip */
    unsigned short phyid_low;   /* PHYID LOW in MII regs of detected chip */
    uint32  model_info;
    uint32  rev_info;
    uint32  spifreq;
};

/* Control Data */
typedef struct bde_ctrl_s {
    struct list_head list;

    /* Specify the type of device, pci, spi, switch, ether ... */
    uint32 dev_type;

    int be_pio;

    union {
        /* Linux PCI device pointer */
        struct pci_dev* _pci_dev;

        /* SPI device pointer */
        struct spi_dev* _spi_dev;
    } dev;
#define pci_device  dev._pci_dev
#define spi_device  dev._spi_dev

#ifdef LINUX_BDE_DMA_DEVICE_SUPPORT
    struct device *dma_dev;
#endif

    /* Physical addresses */
    resource_size_t phys_address;
    resource_size_t phys_address1;
    resource_size_t phys_address2;

#if defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    void *cpu_address;
#endif

    /* Secondary mapped base address */
    sal_vaddr_t alt_base_addr;
    
    /* BDE device description */
    ibde_dev_t bde_dev;

    /* Interrupt Handling */
    int iLine; /* Interrupt line */
    void (*isr)(void *);
    void *isr_data;

    /*
     * Controls to allow two drivers to share a single set of
     * hardware registers. Typically a kernel driver will handle
     * a subset of hardware interrupts and a user mode driver
     * will handle the remaining interrupts.
     */
    void (*isr2)(void *);
    void *isr2_data;
    uint32_t fmask;  /* Interrupts controlled by secondary handler */
    uint32_t imask;  /* Enabled interrupts for primary handler */
    uint32_t imask2; /* Enabled interrupts for secondary handler */
    spinlock_t lock; /* Lock for IRQ mask synchronization */

} bde_ctrl_t;

static bde_ctrl_t _devices[LINUX_BDE_MAX_DEVICES];
static int _ndevices = 0;
static int _switch_ndevices = 0;
static int _ether_ndevices = 0;
static int _cpu_ndevices = 0;
static int  robo_switch = 0;
static int  use_msi = 0;

#define VALID_DEVICE(_n) ((_n >= 0) && (_n < _ndevices))

#ifdef BCM_ROBO_SUPPORT

/* for SPI access via bcm4710 core */
static void *robo = NULL;
static void *sbh = NULL;

#ifdef ALTA_ROBO_SPI

extern void *alta_eth_spi_ctrl;

extern int
robo_spi_read(void *cookie, uint16_t reg, uint8_t *buf, int len);

extern int
robo_spi_write(void *cookie, uint16_t reg, uint8_t *buf, int len);

#define ROBO_RREG(_robo, _dev, _page, _reg, _buf, _len) \
        robo_spi_read(_dev ? NULL : alta_eth_spi_ctrl, \
                      (_page << 8) | (_reg), _buf, _len)
#define ROBO_WREG(_robo, _dev, _page, _reg, _buf, _len) \
        robo_spi_write(_dev ? NULL : alta_eth_spi_ctrl, \
                       (_page << 8) | (_reg), _buf, _len)

#else /* !ALTA_ROBO_SPI */

#if defined(KEYSTONE) || defined(IPROC_CMICD)
#define ROBO_RREG(_robo, _dev, _page, _reg, _buf, _len) \
        robo_rreg(_robo, _dev, _page, _reg, _buf, _len)
#define ROBO_WREG(_robo, _dev, _page, _reg, _buf, _len) \
        robo_wreg(_robo, _dev, _page, _reg, _buf, _len)
#else
#define ROBO_RREG(_robo, _dev, _page, _reg, _buf, _len)
#define ROBO_WREG(_robo, _dev, _page, _reg, _buf, _len)
#endif

#endif /* ALTA_ROBO_SPI */

#endif /* BCM_ROBO_SUPPORT */

/* Broadcom BCM4704 */
#define BCM4704_VENDOR_ID 0x14E4
#define BCM4704_DEVICE_ID 0x4704

/* SiByte PCI Host */
#define SIBYTE_PCI_VENDOR_ID 0x166D
#define SIBYTE_PCI_DEVICE_ID 0x0001

/* Intel 21150 PCI-PCI Bridge */
#define DC21150_VENDOR_ID 0x1011
#define DC21150_DEVICE_ID 0x0022

/* HiNT HB4 PCI-PCI Bridge (21150 clone) */
#define HINT_HB4_VENDOR_ID 0x3388
#define HINT_HB4_DEVICE_ID 0x0022

/* Pericom PI7C8150 PCI-PCI Bridge (21150 clone) */
#define PI7C8150_VENDOR_ID 0x12D8
#define PI7C8150_DEVICE_ID 0x8150

/* Pericom PI7C9X130 PCI-PCIE Bridge */
#define PCI_VNDID_PERICOM     0x12D8
#define PCI_DEVID_PI7C9X130   0xE130
#define DEV_CTRL_REG           0xb8

#define MAX_PAYLOAD_256B       (1 << 5)
#define MAX_PAYLOAD_512B       (2 << 5)
#define MAX_READ_REQ_256B      (1 << 12)


/* Freescale 8548 PCI-E  host Bridge */
#define FSL_VENDOR_ID                   0x1957
#define FSL8548PCIE_DEVICE_ID           0x0013
#define FSL2020EPCIE_DEVICE_ID          0x0070
#define FSL8548PCIE_DEV_CTRL_REG        0x54

/* 4716 PCI-E  host Bridge */
#define BCM4716_VENDOR_ID               0x14e4
#define BCM4716PCIE_DEVICE_ID           0x4716
#define BCM4716PCIE_DEV_CAP_REG         0xd4
#define BCM4716PCIE_DEV_CTRL_REG        0xd8
#define BCM53000_VENDOR_ID              0x14e4
#define BCM53000PCIE_DEVICE_ID          0x5300

#define BCM53000PCIE_DEV(port) ((port == 0) ? pcie0 : pcie1)
#define BCM53000PCIE_BASE(port) ((port == 0) ? 0xb8005000 : 0xb800e000)
#define BCM53000PCIE_FUNC0_COFIG_SPACE 0x400
#define BCM53000PCIE_SROM_SPACE 0x800
#define BCM53000PCIE_DEV_CAP_REG  0xd4
#define BCM53000PCIE_DEV_CTRL_REG 0xd8
#define BCM53000PCIE_MAX_PAYLOAD_MASK  0x7
#define BCM53000PCIE_CAP_MAX_PAYLOAD_256B  (1 << 0)
#define BCM53000PCIE_DEFAULT_STATUS 0x00100146

/* 16bit wide register. offset 14, 14*2 = 0x1c */
#define BCM53000PCIE_SPROM_OFFSET 0x1c  
/* bit 15:13 spromData.MaxPayloadSize. 1: 256 bytes */
#define BCM53000PCIE_SPROM_MAX_PAYLOAD_MASK 0xe000
#define BCM53000PCIE_SPROM_MAX_PAYLOAD_256B (1 << 13)


/* Intel 21150, HiNT HB4 and other 21150-compatible */
#define PCI_CFG_DEC21150_SEC_CLK 0x68

#define BCM4704_ENUM_BASE     0x18000000
#define BCM4704_MEMC_BASE     (BCM4704_ENUM_BASE+0x8000)
#define BCM4704_MEMC_PRIORINV 0x18

/* PLX PCI-E Switch */
#define PLX_PEX8608_DEV_ID         0x8608
#define PLX_PEX8617_DEV_ID         0x8617
#define PLX_PEX86XX_DEV_CTRL_REG   0x70 

/* Broadcom BCM58525 */
#define BCM58525_PCI_VENDOR_ID     0x14E4
#define BCM58525_PCI_DEVICE_ID     0x8025
#define BCM58522_PCI_DEVICE_ID     0x8022

#define IPROC_SUBWIN_MAX        8
#define IPROC_DEFAULT_SUBWIN    7

/* PAXB_ENDIANESS register offset with PCI BAR0 window */
#define BAR0_PAXB_ENDIANESS             0x2030
#define BAR0_PAXB_IMAP0_0               0x2c00
#define BAR0_PAXB_OARR_FUNC0_MSI_PAGE   0x2d34
#define BAR0_PAXB_OARR_2                0x2d60
#define BAR0_PAXB_OARR_2_UPPER          0x2d64

typedef struct _iproc_subwin_s {
    uint32_t addr_min;
    uint32_t addr_max;
} _iproc_subwin_t;

typedef struct _iproc_map_s {
    _iproc_subwin_t subwin[IPROC_SUBWIN_MAX];
} _iproc_map_t;

static _iproc_map_t iproc_map_default = {
    {
        { 0x18000000, 0x18000fff }, /* CCA */
        { 0x18030000, 0x18030fff }, /* CCB */
        { 0x18012000, 0x18012fff }, /* PAXB-0 */
        { 0, 0 }
    }
};

_iproc_map_t *iproc_map[] = {
    &iproc_map_default
};

static uint32_t _read(int d, uint32_t addr);

#ifdef BCM_ICS
#else
/* Used to determine overall memory limits across all devices */
static uint32_t _pci_mem_start = 0xFFFFFFFF;
static uint32_t _pci_mem_end = 0;
#endif

#ifdef BCM_PLX9656_LOCAL_BUS

#define CPLD_OFFSET             0x00800000
#define CPLD_REVISION_REG       0x0000
#define CPLD_REVISION_MASK      0xffff
#define CPLD_RESET_REG          0x0004
#define CPLD_RESET_NONE         0x0000

#define PL0_OFFSET              0x00800000
#define PL0_SIZE                0x00040000
#define PL0_REVISION_REG        0x0000

/* Assume there's only one PLX PCI-to-Local bus bridge if any */
static bde_ctrl_t plx_ctrl;
static int num_plx = 0;

#endif /* BCM_PLX9656_LOCAL_BUS */

/* Flags for memory allocations */
#ifdef SAL_BDE_XLP
#define KMALLOC(size, flags)    __kmalloc(size, flags)
static int mem_flags = GFP_ATOMIC | GFP_KERNEL | GFP_DMA;
#else
#define KMALLOC(size, flags)    kmalloc(size, flags)
static int mem_flags = GFP_ATOMIC | GFP_DMA32;
#endif

static spinlock_t bus_lock;

static int
_parse_eb_args(char *str, char * format, ...)
    __attribute__ ((format (scanf, 2, 3)));

static int
_parse_eb_args(char *str, char * format, ...)
{
    va_list args;

    va_start(args, format);
    vsscanf(str, format, args);
    va_end(args);

    return 0;
}

static int
_eb_device_create(resource_size_t paddr, int irq, int rd_hw, int wr_hw)
{
    bde_ctrl_t *ctrl;
    uint32  dev_rev_id = 0x0, dev_id;

    dev_id = _ndevices;

    ctrl = _devices + _ndevices++;
    _switch_ndevices++;

    ctrl->dev_type |= BDE_EB_DEV_TYPE | BDE_SWITCH_DEV_TYPE;
    ctrl->pci_device = NULL; /* No PCI bus */

    if(rd_hw) {
        ctrl->dev_type |= BDE_DEV_BUS_RD_16BIT;
    }

    if (wr_hw) {
        ctrl->dev_type |= BDE_DEV_BUS_WR_16BIT;
    }

    /* Map in the device */
    ctrl->bde_dev.base_address = (sal_vaddr_t)IOREMAP(paddr, 0x10000);
    ctrl->phys_address = paddr;

    dev_rev_id = _read(dev_id, 0x178);  /* CMIC_DEV_REV_ID */

    ctrl->bde_dev.device = dev_rev_id & 0xFFFF;
    ctrl->bde_dev.rev = (dev_rev_id >> 16) & 0xFF;

    ctrl->iLine = irq;
    ctrl->isr = NULL;
    ctrl->isr_data = NULL;

    gprintk("Created EB device at BA=%x IRQ=%d RD16=%d WR16=%d device=0x%x\n",
            (unsigned int)paddr, irq, rd_hw, wr_hw, ctrl->bde_dev.device);

    return 0;
}

#if defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)

#include <soc/devids.h>

static int
petra_device_create(void)
{
    bde_ctrl_t* ctrl;

    ctrl = _devices; 

#ifndef __DUNE_LINUX_BCM_CPU_PCIE__
    _switch_ndevices++;
    _ndevices++;

    ctrl->dev_type |= BDE_PCI_DEV_TYPE | BDE_SWITCH_DEV_TYPE;
    ctrl->pci_device = NULL; /* No PCI bus */

    /* Map in the device */ 
    ctrl->bde_dev.base_address = (sal_vaddr_t)IOREMAP(0x40000000, 0x100000);
    ctrl->phys_address = 0x40000000;

    ctrl->iLine = 0;
    ctrl->isr = NULL;
    ctrl->isr_data = NULL;

    ctrl->bde_dev.device = BCM88750_DEVICE_ID; /*PETRAB_DEVICE_ID; */
    ctrl->bde_dev.rev = BCM88750_A0_REV_ID; /*PETRAB_A0_REV_ID;*/
#endif

    /* Map CPU regs */
    ctrl->cpu_address = IOREMAP(0xe0000000, 0x100000);


    if ((ctrl->bde_dev.device == PCP_PCI_DEVICE_ID)) {
        ctrl->bde_dev.device = PETRAB_DEVICE_ID;
        ctrl->bde_dev.rev = PETRAB_A0_REV_ID;
    }

    if ((ctrl->bde_dev.device == ACP_PCI_DEVICE_ID)) {
        ctrl->dev_type |= BDE_PCI_DEV_TYPE | BDE_SWITCH_DEV_TYPE;
    }

    return 0;
}
#endif

#if defined(IPROC_CMICD) || !defined(BCM_ICS)
typedef struct _iproc_profile_info_s{
    uint16_t    device;
    int         iproc_profile_version;
}_iproc_profile_info_t;

static _iproc_profile_info_t _iproc_profile_map[]={
{BCM53401_DEVICE_ID, 7},
{BCM53402_DEVICE_ID, 7},
{BCM53403_DEVICE_ID, 7},
{BCM53404_DEVICE_ID, 7},
{BCM53405_DEVICE_ID, 7},
{BCM53406_DEVICE_ID, 7},
{BCM53408_DEVICE_ID, 7},
{BCM53411_DEVICE_ID, 7},
{BCM53412_DEVICE_ID, 7},
{BCM53413_DEVICE_ID, 7},
{BCM53414_DEVICE_ID, 7},
{BCM53415_DEVICE_ID, 7},
{BCM53416_DEVICE_ID, 7},
{BCM53418_DEVICE_ID, 7},
{BCM56060_DEVICE_ID, 7},
{BCM56062_DEVICE_ID, 7},
{BCM56063_DEVICE_ID, 7},
{BCM56064_DEVICE_ID, 7},
{ 0, 0}
};

static void
iproc_ver_get(ibde_dev_t bde_dev, int *iproc_rev)
{
    uint16_t device;
    int i;

    device = bde_dev.device;
    *iproc_rev = -1;

    i = 0;
    while (_iproc_profile_map[i].device != 0) {
        if (_iproc_profile_map[i].device == device) {
            *iproc_rev = _iproc_profile_map[i].iproc_profile_version;
            break;
        }
        i++;
    }
}
#endif /* defined(IPROC_CMICD) || !defined(BCM_ICS) */

#ifdef IPROC_CMICD
static void
iproc_cmicd_get_irqres(ibde_dev_t bde_dev, struct resource *res_irq);

#include <linux/platform_device.h>

extern void iproc_platform_driver_register(struct platform_driver *drv);
extern void iproc_platform_driver_unregister(struct platform_driver *drv);
extern void iproc_platform_device_register(struct platform_device *drv);
extern void iproc_platform_device_unregister(struct platform_device *drv);

extern struct resource *
iproc_platform_get_resource(struct platform_device *dev, unsigned int type,
                            unsigned int num);

#define IPROC_CHIPCOMMONA_BASE  0x18000000
#define IPROC_CMICD_BASE        0x48000000
#define IPROC_CMICD_SIZE        0x40000
#define IPROC_CMICD_INT         194

static int
iproc_cmicd_probe(struct platform_device *pldev)
{
    bde_ctrl_t *ctrl;
    uint32 size, dev_rev_id;
    struct resource *memres, *irqres;    

    memres = iproc_platform_get_resource(pldev, IORESOURCE_MEM, 0);
    if (memres == NULL) {
        gprintk("Unable to retrieve iProc CMIC resources"); 
        return -1;
    }
    size = memres->end - memres->start + 1;

    ctrl = _devices + _ndevices++;
    _switch_ndevices++;

    ctrl->dev_type = BDE_AXI_DEV_TYPE | BDE_SWITCH_DEV_TYPE | BDE_256K_REG_SPACE;
    ctrl->pci_device = NULL; /* No PCI bus */

    /* Map CMIC block in the AXI memory space into CPU address space */
    ctrl->bde_dev.base_address = (sal_vaddr_t)IOREMAP(memres->start, size);
    if (!ctrl->bde_dev.base_address) {
        gprintk("Error mapping iProc CMIC registers"); 
        return -1;
    }
    ctrl->phys_address = memres->start;

    /* Read switch device ID from CMIC */
    dev_rev_id = *((uint32_t*)(ctrl->bde_dev.base_address + 0x10224));
#if defined(BCM_CMICM_SUPPORT) && defined(BE_HOST)
    ctrl->bde_dev.device = (  (((dev_rev_id >> 16) & 0xff) << 8) |
                               ((dev_rev_id >> 24) & 0xff));
    ctrl->bde_dev.rev    =      (dev_rev_id >>  8) & 0xff ;
#else
    ctrl->bde_dev.device = dev_rev_id & 0xffff;
    ctrl->bde_dev.rev = (dev_rev_id >> 16) & 0xff;
#endif

    iproc_cmicd_get_irqres(ctrl->bde_dev, &pldev->resource[0]);

    irqres = iproc_platform_get_resource(pldev, IORESOURCE_IRQ, 0);

    ctrl->iLine = irqres->start;

    ctrl->isr = NULL;
    ctrl ->isr_data = NULL;

#ifdef LINUX_BDE_DMA_DEVICE_SUPPORT
    ctrl->dma_dev = &pldev->dev;
#endif

    /* Let's boogie */
    return 0;
}

static int
iproc_cmicd_remove(struct platform_device *pldev)
{
    return 0;
}

static char iproc_cmicd_string[] = "bcmiproc-cmicd";

static struct platform_driver iproc_cmicd_driver = 
{
    .probe = iproc_cmicd_probe,
    .remove = iproc_cmicd_remove,
    .driver =
    {
        .name = iproc_cmicd_string,
        .owner = THIS_MODULE,
    },
};

typedef enum {
    IPROC_CMICD_RES_INTR = 0,
    IPROC_CMICD_RES_MEM
} IPROC_CMICD_RES_E;

static struct resource iproc_cmicd_resources[] = {
    [IPROC_CMICD_RES_INTR] = {
        .flags  = IORESOURCE_IRQ,
        .start  = IPROC_CMICD_INT,
    },
    [IPROC_CMICD_RES_MEM] = {
        .flags  = IORESOURCE_MEM,
        .start  = IPROC_CMICD_BASE,
        .end    = IPROC_CMICD_BASE+IPROC_CMICD_SIZE-1,
    },
};

static void
iproc_cmicd_release(struct device *dev)
{
}

static u64 iproc_cmicd_dmamask = DMA_BIT_MASK(32);

static struct platform_device iproc_cmicd_pdev = {
    .name = iproc_cmicd_string,
    .id = 0,
    .dev =  {
        .release = iproc_cmicd_release,
        .init_name = iproc_cmicd_string,
        .dma_mask = &iproc_cmicd_dmamask,
        .coherent_dma_mask = DMA_BIT_MASK(32),
    },
    .resource = iproc_cmicd_resources,
    .num_resources = ARRAY_SIZE(iproc_cmicd_resources),
};

static int
iproc_has_cmicd(void)
{
    void *iproc_cca_base;
    uint32 cca_cid;

    /* Read ChipcommonA chip id register to identify current SOC */
    iproc_cca_base = IOREMAP(IPROC_CHIPCOMMONA_BASE, 0x3000);
    if (iproc_cca_base == NULL) {
        gprintk("iproc_has_cmicd: ioremap of ChipcommonA registers failed"); 
        return 0;
    }
    cca_cid = readl((uint32 *)iproc_cca_base);
    cca_cid &= 0xffff;
    iounmap(iproc_cca_base);

    /* Only allowed accessing CMICD module if the SOC has it */
    switch (cca_cid) {
        case BCM53010_CHIP_ID:
        case BCM53018_CHIP_ID:
        case BCM53020_CHIP_ID:
            return 0;
        default:
            break;
    }

    /* Device has CMIC */
    return 1;
}

#define IPROC_CHIPCOMMONA_EROM_PTR_OFFSET   (0x180000fc)
#define EROM_MAX_SIZE   (0x1000)
#define EROM_PARTNUM_CMICD   (0x14)

#define EROM_DESC_COMPIDENT  (0x1)
#define EROM_DESC_MASTER     (0x3)
#define EROM_DESC_ADDR       (0x5)
#define EROM_DESC_END        (0xF)
#define EROM_DESC_EMPTY      (0)

#define EROM_IS_DESC_COMPIDENT(x) ((x & 0x7) == EROM_DESC_COMPIDENT)
#define EROM_IS_DESC_MASTER(x)    ((x & 0x7) == EROM_DESC_MASTER)
#define EROM_IS_DESC_ADDR(x)      ((x & 0x7) == EROM_DESC_ADDR)
#define EROM_IS_DESC_END(x)       ((x & 0xF) == EROM_DESC_END)

#define EROM_GET_PARTNUM(x)       ((x >> 8) & (0xFFF))       /* Bit 8~19 */
#define EROM_GET_ADDRESS(x)       ((x >> 12) & (0xFFFFF))    /* Bit 12~31 */
#define EROM_GET_SIZETYPE(x)      ((x >> 4) & (0x3))         /* Bit 4~5 */
#define EROM_GET_AG32(x)          ((x >> 3) & (0x1))         /* Bit 3 */
#define EROM_GET_SIZE(x)          ((x >> 12) & (0xFFFFF))    /* Bit 12~31 */
#define EROM_GET_SG32(x)          ((x >> 3) & (0x1))         /* Bit 3 */

#define EROM_ADDR_SIZETYPE_4K   (0)
#define EROM_ADDR_SIZETYPE_8K   (1)
#define EROM_ADDR_SIZETYPE_16K  (2)
#define EROM_ADDR_SIZETYPE_MORE (3)

#define EROM_ADDR_FLAG_AG32        (1) /* Address space greater than 32 bit */
#define EROM_ADDR_FLAG_SIZE        (2) /* Addition size descriptor */
#define EROM_ADDR_FLAG_SG32        (4) /* Size descriptor greater than 32 bit */

static void
iproc_cmicd_get_memregion(struct resource *res_mem)
{
    void *erom_ptr_oft;
    uint32_t erom_phy_addr;
    uint32_t *erom_base;
    uint32_t i = 0;
    uint32_t word = 0;
    uint8_t more_addr_word = 0; /* bit 0: AG32; bit 1: SIZE; bit 2: SG32 */
    uint8_t found_cmicd_dev = 0;
    uint8_t size_type = 0;
    bool is_compident_a = 1; /* 1: CompidentA; o/w: CompidentB */

    erom_ptr_oft = IOREMAP(IPROC_CHIPCOMMONA_EROM_PTR_OFFSET, 0x100);

    erom_phy_addr = readl((uint32 *)(erom_ptr_oft));
    iounmap(erom_ptr_oft);

    erom_base = IOREMAP(erom_phy_addr, EROM_MAX_SIZE);

    while (1) {
        word = readl((uint32 *)(erom_base + i));

        if (EROM_IS_DESC_ADDR(word) || more_addr_word) {
            if (more_addr_word == 0) { /* Normal Addr Desc */
                if (EROM_GET_AG32(word) == 1) {
                    more_addr_word |= EROM_ADDR_FLAG_AG32;
                }

                size_type = EROM_GET_SIZETYPE(word);
                if (size_type == EROM_ADDR_SIZETYPE_MORE) {
                    more_addr_word |= EROM_ADDR_FLAG_SIZE;
                }

                if (found_cmicd_dev == 1) {
                    res_mem->start = EROM_GET_ADDRESS(word) << 12;
                    if (size_type < EROM_ADDR_SIZETYPE_MORE) {
                        res_mem->end = res_mem->start + 4096 * (1 << size_type) - 1;
                    }
                }
            }
            else if (more_addr_word & EROM_ADDR_FLAG_AG32) { /* UpperAddr Desc */
                more_addr_word &= ~EROM_ADDR_FLAG_AG32;

                if (found_cmicd_dev == 1) {
                    /* res_mem->start |= word << 32; */
                    gprintk("Expect cmicd address to be 32-bit\n");
                }
            }
            else if (more_addr_word & EROM_ADDR_FLAG_SIZE) { /* Size Desc */
                if (EROM_GET_SG32(word) == 1) {
                    more_addr_word |= EROM_ADDR_FLAG_SG32;
                }

                more_addr_word &= ~EROM_ADDR_FLAG_SIZE;

                if (found_cmicd_dev == 1) {
                    res_mem->end = res_mem->start + (EROM_GET_SIZE(word) << 12) - 1;
                }
            }
            else if (more_addr_word & EROM_ADDR_FLAG_SG32) { /* UpperSize Desc */
                more_addr_word &= ~EROM_ADDR_FLAG_SG32;

                if (found_cmicd_dev == 1) {
                    /* res_mem->end += (word) << 32; */
                    gprintk("Expect cmicd size to be 32-bit\n");
                }
            }

            if (found_cmicd_dev == 1 && more_addr_word == 0) {
                break;  /* We have gotten all necessary information, exit the loop */
            }
        }
        else if (EROM_IS_DESC_COMPIDENT(word)) {
            if (is_compident_a == 1) {
                if (EROM_GET_PARTNUM(word) == EROM_PARTNUM_CMICD) {
                    found_cmicd_dev = 1;
                }
            }

            is_compident_a = 1 - is_compident_a;
        }
        else if (EROM_IS_DESC_END(word)) {
            break;
        }

        i++;
    }
    iounmap(erom_base);

    if (debug >= 1) {
        gprintk("CMICD info by %s: cmicd_mem.start=%x, cmicd_mem.end=%x\n",
                found_cmicd_dev ? "EROM" : "Default",
                iproc_cmicd_resources[IPROC_CMICD_RES_MEM].start,
                iproc_cmicd_resources[IPROC_CMICD_RES_MEM].end);
    }
}

static void
iproc_cmicd_get_irqres(ibde_dev_t bde_dev, struct resource *res_irq)
{
    int iproc_rev;
    iproc_ver_get(bde_dev, &iproc_rev);
    if ((iproc_rev == -1) && (debug >= 1)) {
        gprintk("device is not defined in the _iproc_profile_map \n");
    }
    if (iproc_rev == 7) {
        res_irq->start = 221;
    }

}

#endif /* IPROC_CMICD */

#ifdef BCM_ICS
static int
_ics_bde_create(void)
{
    bde_ctrl_t *ctrl;
    uint32 dev_rev_id = 0x0;
    resource_size_t paddr;

    if (_ndevices == 0) {
        ctrl = _devices + _ndevices++;
        _switch_ndevices++;

        ctrl->dev_type |= BDE_ICS_DEV_TYPE | BDE_SWITCH_DEV_TYPE;
        ctrl->pci_device = NULL; /* No PCI bus */

        /* Map in the device */
        paddr = BCM_ICS_CMIC_BASE;
        ctrl->bde_dev.base_address = (sal_vaddr_t)IOREMAP(paddr, 0x10000);
        ctrl->phys_address = paddr;

        dev_rev_id = *((unsigned int *)(KSEG1ADDR(paddr + 0x178)));

        ctrl->bde_dev.device = dev_rev_id & 0xFFFF;
        ctrl->bde_dev.rev = (dev_rev_id >> 16) & 0xFF;

        ctrl->iLine = 5; /* From raptor linux BSP */

        ctrl->isr = NULL;
        ctrl->isr_data = NULL;
        printk("Created ICS device ..%x\n", ctrl->bde_dev.base_address);
    }

    return 0;
}

#else /* !BCM_ICS */

/*
 * Function: p2p_bridge
 *
 * Purpose:
 *    Finalize initialization secondary PCI-PCI bridge.
 * Parameters:
 *    membase - start of memory address space for secondary PCI bus
 * Returns:
 *    0
 * Notes:
 *    The membase depends on the processor architecture, and is
 *    derived from the memory space addresses set up by the kernel.
 */
static int
p2p_bridge(void)
{
    struct pci_dev *dev;
    uint16 cmd;
    uint16 mem_base;
    uint16 mem_limit;
    uint8 bridge_ctrl;

    if ((dev = PCI_FIND_DEV(DC21150_VENDOR_ID, DC21150_DEVICE_ID, NULL)) != NULL ||
        (dev = PCI_FIND_DEV(HINT_HB4_VENDOR_ID, HINT_HB4_DEVICE_ID, NULL)) != NULL ||
        (dev = PCI_FIND_DEV(PI7C8150_VENDOR_ID, PI7C8150_DEVICE_ID, NULL)) != NULL) {

        if (debug >= 1) gprintk("fixing up PCI-to-PCI bridge\n");
        /* Adjust command register */
        pci_read_config_word(dev, PCI_COMMAND, &cmd);
        cmd |= PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
        /* Disable device */
        pci_write_config_word(dev, PCI_COMMAND, 0);
        /* Initialize non-prefetchable memory window if needed */
        pci_read_config_word(dev, PCI_MEMORY_BASE, &mem_base);
        if (mem_base == 0) {
            mem_base = (uint16)((_pci_mem_start & 0xFFF00000) >> 16);
            mem_limit = (uint16)((_pci_mem_end & 0xFFF00000) >> 16);
            pci_write_config_word(dev, PCI_MEMORY_BASE, mem_base);
            pci_write_config_word(dev, PCI_MEMORY_LIMIT, mem_limit);
        }
        /* Enable PCI clocks on remote end */
        pci_write_config_word(dev, PCI_CFG_DEC21150_SEC_CLK, 0);
        /* Re-enable config space */
        pci_write_config_word(dev, PCI_COMMAND, cmd);

        /* Avoid DMA data corruption */
        if (dev->vendor == HINT_HB4_VENDOR_ID) {
            /* Fix for HiNT bridge and BCM4704 DMA problem */
            if ((dev = PCI_FIND_DEV(BCM4704_VENDOR_ID, BCM4704_DEVICE_ID, NULL)) != NULL) {
                /* Reset PrefetchEn (PE) */
                pci_write_config_dword(dev, 0x8c, 1);
                if (debug >= 1) {
                    gprintk("reset PrefetchEn on BCM4704 when HiNT bridge is present\n");
                }
            }
        }
    }
    /* Enable fast back-to-back read/write */
    if ((dev = PCI_FIND_DEV(PI7C8150_VENDOR_ID, PI7C8150_DEVICE_ID, NULL)) != NULL) {
        pci_read_config_word(dev, PCI_COMMAND, &cmd);
        cmd |= PCI_COMMAND_FAST_BACK;
        pci_read_config_byte(dev, PCI_BRIDGE_CONTROL, &bridge_ctrl);
        bridge_ctrl |= PCI_BRIDGE_CTL_FAST_BACK;
        pci_write_config_word(dev, PCI_COMMAND, 0);
        pci_write_config_byte(dev, PCI_BRIDGE_CONTROL, bridge_ctrl);
        pci_write_config_word(dev, PCI_COMMAND, cmd);
    }

    /* Netlogic XLP CPU */
    if ((dev = PCI_FIND_DEV(0x184e, 0x1004, NULL)) != NULL) {
        /* configure the PCIE cap: Max payload size: 256, Max Read
         * Request size: 256, disabling relax ordering.
         * Writes to the PCIE capability device control register
         */

        pci_write_config_dword(dev,0x78,MAX_PAYLOAD_256B |
                                        MAX_READ_REQ_256B);
    }
    if ((dev = PCI_FIND_DEV(0x14e4, 0xb634, NULL)) != NULL) {
        pci_write_config_dword(dev,0x78,MAX_PAYLOAD_256B |
                                        MAX_READ_REQ_256B);
    }

    if ((dev = PCI_FIND_DEV(PCI_VNDID_PERICOM, PCI_DEVID_PI7C9X130, NULL)) != NULL) {
        /*
         * Configure the PCIE cap: Max payload size: 256, Max Read
         * Request size: 256, disabling relax ordering.
         * Writes to the PCIE capability device control register
         */
        pci_write_config_dword(dev, DEV_CTRL_REG,
                               MAX_PAYLOAD_256B | MAX_READ_REQ_256B);
    }

    if ((dev = PCI_FIND_DEV(FSL_VENDOR_ID, FSL8548PCIE_DEVICE_ID, NULL)) != NULL ||
        (dev = PCI_FIND_DEV(FSL_VENDOR_ID, FSL2020EPCIE_DEVICE_ID, NULL)) != NULL) {
        /*
         * Configure the PCIE cap: Max payload size: 256, Max Read
         * Request size: 256, disabling relax ordering.
         * Writes to the PCIE capability device control register
         */
        pci_write_config_dword(dev, FSL8548PCIE_DEV_CTRL_REG,
                               MAX_PAYLOAD_256B | MAX_READ_REQ_256B);
    }
    if ((dev = PCI_FIND_DEV(BCM4716_VENDOR_ID, BCM4716PCIE_DEVICE_ID, NULL)) != NULL ||
        (dev = PCI_FIND_DEV(BCM53000_VENDOR_ID, BCM53000PCIE_DEVICE_ID, NULL)) != NULL) {
        uint32 tmp, maxpayld, device_bmp=0, mask;
        unsigned long addr;
        uint16 tmp16, tmp161;
        int i, bus0 = -1, bus1 = -1, port;        
        struct pci_dev *pcie0, *pcie1;
        
        pcie0 = dev;
        bus0 = dev->bus->number;
        if ((pcie1 = PCI_FIND_DEV(BCM53000_VENDOR_ID, BCM53000PCIE_DEVICE_ID, pcie0)) != NULL) {
            bus1 = pcie1->bus->number;
        }

        for(i = 0; i < _ndevices; i++) {
            bde_ctrl_t *ctrl = _devices + i;

            mask = BDE_SWITCH_DEV_TYPE | BDE_PCI_DEV_TYPE;
            if ((ctrl->dev_type & mask) == mask) {
                if (ctrl->pci_device->bus->number == bus0) {
                    device_bmp |= 1 << 0;
                }
                if (ctrl->pci_device->bus->number == bus1) {
                    device_bmp |= 1 << 1;
                }
            }
        }        

        /* configure the PCIE cap: Max payload size: 256, Max Read
         * Request size: 256, disabling relax ordering.
         * Writes to the PCIE capability device control register
         */

        i = 0;
        while(device_bmp) {
            if (device_bmp & (1 << i)){
                port = i ;                                  
                pci_read_config_dword(BCM53000PCIE_DEV(port), 
                                      BCM53000PCIE_DEV_CAP_REG, &tmp);
                maxpayld = (tmp & BCM53000PCIE_MAX_PAYLOAD_MASK);
                if (debug >= 1) {
                    gprintk("port %d\n",port);
                    gprintk("DevCap (@%x): 0x%x%c\n", BCM53000PCIE_DEV_CAP_REG, tmp,
                            (maxpayld != BCM53000PCIE_CAP_MAX_PAYLOAD_256B) ? ' ':'\n');
                }
                if (maxpayld != BCM53000PCIE_CAP_MAX_PAYLOAD_256B) {
                    addr = BCM53000PCIE_BASE(port);
                    addr |= (BCM53000PCIE_SROM_SPACE | BCM53000PCIE_SPROM_OFFSET);
                    tmp16 = *((uint16 *)addr);                                                       
                    if (debug >= 1){
                        gprintk("addr %lx spromData.MaxPayloadSize: 0x%x\n", addr, tmp16);
                    }
                    mask = BCM53000PCIE_SPROM_MAX_PAYLOAD_MASK;
                    if ((tmp16 & mask) != BCM53000PCIE_SPROM_MAX_PAYLOAD_256B) {
                        tmp161 = (tmp16 & ~mask) | BCM53000PCIE_SPROM_MAX_PAYLOAD_256B;
                        *((uint16 *)addr) = tmp161;                                                  
                        if (debug >= 1) {
                            tmp16 = 0;                                                                         
                            tmp16 = *((uint16 *)addr);                                                   
                            gprintk("Enable spromData.MaxPayloadSize to 1 (256 bytes): "
                                    "0x%x (%s w/ 0x%x)\n", tmp161,
                                    ((tmp16 & mask) == BCM53000PCIE_SPROM_MAX_PAYLOAD_256B) ? 
                                    "Success":"Fail",
                                    tmp16);
                        }
                    }                                                                                      
                    pci_read_config_dword(BCM53000PCIE_DEV(port), 
                                          BCM53000PCIE_DEV_CAP_REG, &tmp);                            
                    if (debug >= 1){
                        gprintk("DevCap (@%x): now is 0x%x\n\n", 
                                BCM53000PCIE_DEV_CAP_REG, tmp);        
                    }
                }                                                                                          

                addr = BCM53000PCIE_BASE(port);
                addr |= (BCM53000PCIE_FUNC0_COFIG_SPACE | BCM53000PCIE_DEV_CTRL_REG);
                tmp16 = *((uint16 *)addr);                                                           
                if (debug >= 1) {
                    gprintk("DevControl (@%x): 0x%x\n", BCM53000PCIE_DEV_CTRL_REG, tmp16);                      
                }
                if (!(tmp16 & MAX_PAYLOAD_256B) || !(tmp16 & MAX_READ_REQ_256B)) {                         
                    tmp161 = tmp16 | MAX_PAYLOAD_256B | MAX_READ_REQ_256B;                                 
                    *((uint16 *)addr) = tmp161;                                                                                                          
                    if (debug >= 1) {
                        tmp16 = 0;                                                                             
                        tmp16 = *((uint16 *)addr);   
                        gprintk("addr %lx Enable DevControl MaxPayloadSize to 1 (256 bytes): "
                                "0x%x (%s w/ 0x%x)\n", addr, tmp161,
                                (tmp16 & MAX_PAYLOAD_256B) ? "Success":"Fail",
                                tmp16);
                        gprintk("Enable DevControl MaxReadRequestSize to 1 (256 bytes): "
                                "0x%x (%s w/ 0x%x)\n\n", tmp161,
                                (tmp16 & MAX_READ_REQ_256B) ? "Success":"Fail",
                                tmp16);
                    }                    
                }             
                device_bmp &= ~(1 << i);
            }
            i++;
        }
    }

    /* 
     * Configure max payload to 512 on all ports in the PLX8608/PLX8617.
     * The device supports 128, 512, and 1024 max payload sizes. 
     */
    dev = NULL;
    while ((dev = PCI_FIND_DEV(PCI_VENDOR_ID_PLX, PCI_ANY_ID, dev)) != NULL) {
        if ((dev->device == PLX_PEX8608_DEV_ID) ||
            (dev->device == PLX_PEX8617_DEV_ID)) { 
            uint16 ctrl_reg;
            pci_read_config_word(dev, PLX_PEX86XX_DEV_CTRL_REG, &ctrl_reg);
            ctrl_reg = (ctrl_reg & ~(7<<5)) | MAX_PAYLOAD_512B;
            pci_write_config_word(dev, PLX_PEX86XX_DEV_CTRL_REG, ctrl_reg);
        }
    }
    return 0;
}

#ifdef BCM_PLX9656_LOCAL_BUS

#define PLX_LAS0_BA         0x00000004  /* LAS0 Local Base Address Remap   */
#define PLX_LAS1_BA         0x000000f4  /* LAS1 Local Base Address Remap   */
#define PLX_LAS_EN          0x00000001  /* Space Enable bit                */

#define PLX_MMAP_PCIBAR0    0           /* Memory-Mapped Config (PCIBAR0)  */
#define PLX_LAS0_PCIBAR2    2           /* Local Address Space 0 (PCIBAR2) */
#define PLX_LAS1_PCIBAR3    3           /* Local Address Space 1 (PCIBAR3) */

STATIC int 
_plx_las_bar_get(struct pci_dev *dev)
{
    void           *local_config_addr;
    int             bar = -1;

    local_config_addr = IOREMAP(pci_resource_start(dev, PLX_MMAP_PCIBAR0),
                                pci_resource_len(dev, PLX_MMAP_PCIBAR0));
    if (local_config_addr) {
        uint32          las_remap_reg;        
        /* 
         * Make sure LAS0BA or LAS1BA is enabled before returning
         * BAR that will be used to access the Local Bus
         */
        las_remap_reg = ioread32(local_config_addr + PLX_LAS0_BA);
        if (las_remap_reg & PLX_LAS_EN) {
            bar = PLX_LAS0_PCIBAR2;
        } else {
            las_remap_reg = ioread32(local_config_addr + PLX_LAS1_BA);
            if (las_remap_reg & PLX_LAS_EN) {
                bar = PLX_LAS1_PCIBAR3;
            }
        } 
    }
    iounmap(local_config_addr);
    return bar;
}
#endif /* BCM_PLX9656_LOCAL_BUS */


/*
 * Function: _pci_probe
 *
 * Purpose:
 *    Device initialization callback used by the Linux PCI
 *    subsystem. Called as a result of pci_register_driver().
 * Parameters:
 *    dev - Linux PCI device structure
 * Returns:
 *    0
 */
static int
_pci_probe(struct pci_dev *dev, const struct pci_device_id *ent)
{
    bde_ctrl_t *ctrl;
    resource_size_t paddr;
    uint16  cmd = 0;
    uint32 bar_len;
    int baroff = 0;
    int pcie = 0;
    int iproc = 0;
    int iproc_rev = 0;
    uint16 cap_base = 0;
    uint16 rval = 0;
    uint32 rval32 = 0;
    uint32 gmac_base = 0;
    uint32 cap_val = 0;
    int plx_dev = 0;
    int eth_dev = 0;
    int cpu_dev = 0;
    int update_devid = 0;
    int i;

    if (nodevices == 1) {
        return 0; 
    }

    /*
     * If an AXI-based switch device has been found already, then it means
     * that the AXI bus has been probed, and that we should ignore devices
     * found on PCI bus zero.
     */

#if defined(IPROC_CMICD)
    if ((dev->bus) && (dev->bus->number == 0)) {
        int i;
        uint32 mask = BDE_SWITCH_DEV_TYPE | BDE_AXI_DEV_TYPE;

        for (i = 0; i < _ndevices; i++) {
            ctrl = _devices + i;

            if ((ctrl->dev_type & mask) == mask) {
                return 0;
            }
        }        
    }
#endif /* IPROC_CMICD */

    /*
     * Note that a few supported devices have a non-Broadcom PCI vendor ID,
     * but since none of their associated PCI device IDs collide with the
     * Broadcom device IDs, the probe code below only checks the device ID.
     */

    switch (dev->device) {
    case PCI_DEVICE_ID_PLX_9656:
    case PCI_DEVICE_ID_PLX_9056:
        plx_dev = 1;
        break;
#if defined (BCM_ROBO_SUPPORT) && defined(KEYSTONE)
    case BCM53000_GMAC_ID:
        eth_dev = 1;
        gmac_base = SB_ENUM_BASE;
        break;
#endif
#if defined (BCM_ROBO_SUPPORT)
    case BCM47XX_ENET_ID:
        eth_dev = 1;
        break;
#endif
    case BCM53000PCIE_DEVICE_ID:
        cpu_dev = 1;
        break;
        /*
         * The device ID for 56500, 56100 and 56300 family of devices may
         * be incorrect just after a PCI HW reset. It needs to be read
         * again after stabilization.
         */
    case BCM56102_DEVICE_ID:
    case BCM56504_DEVICE_ID:
    case BCM56304_DEVICE_ID:
    case BCM56314_DEVICE_ID:
    case BCM56112_DEVICE_ID:
        update_devid = 1;
        break;
    default:
        break;
    }

    ctrl = NULL;
    if (plx_dev) {
#if defined(BCM_PLX9656_LOCAL_BUS)
        /* PLX chip itself won't be part of _devices[]. */
        baroff = _plx_las_bar_get(dev);
        if (baroff == -1) {
            gprintk("No Local Address Space enabled in PLX\n");
            return 0;
        }
        ctrl = &plx_ctrl;
        num_plx++;
#endif
    } else if (eth_dev) {
        if (_ether_ndevices >= LINUX_BDE_MAX_ETHER_DEVICES) {
            return 0;;
        }
        ctrl = _devices + _ndevices++;
        _ether_ndevices++;
        ctrl->dev_type |= BDE_ETHER_DEV_TYPE;
        ctrl->iLine = dev->irq;
        if (debug >= 1)
            gprintk("Found PCI device %04x:%04x as Ethernet device\n",
                    dev->vendor, dev->device);
    } else if (cpu_dev) {
        if (_cpu_ndevices >= LINUX_BDE_MAX_CPU_DEVICES) {
            return 0;;
        }
        ctrl = _devices + _ndevices++;
        _cpu_ndevices++;
        ctrl->dev_type |= BDE_CPU_DEV_TYPE;
        if (debug >= 1)
            gprintk("Found PCI device %04x:%04x as CPU device\n",
                    dev->vendor, dev->device);
    } else {
        if (PCI_FUNC(dev->devfn) > 0) {
            return 0;
        }
        if (_switch_ndevices >= LINUX_BDE_MAX_SWITCH_DEVICES) {
            return 0;;
        }
        ctrl = _devices + _ndevices++;
        _switch_ndevices++;
        ctrl->dev_type |= BDE_SWITCH_DEV_TYPE;

        if (update_devid) {
            /* Re-read the device ID */
            pci_read_config_word(dev,
                                 PCI_DEVICE_ID,
                                 &ctrl->bde_dev.device);
            dev->device = ctrl->bde_dev.device;
        }

        if (pci_enable_device(dev)) {
            gprintk("Cannot enable PCI device : vendor_id = %x, device_id = %x\n",
                    dev->vendor, dev->device);
        }

        
        /*
         * These are workarounds to get around some existing
         * kernel problems :(
         */

        /*
         * While probing we determine the overall limits for the PCI
         * memory windows across all devices. These limits are used
         * later on by the PCI-PCI bridge  code.
         */
        if (pci_resource_start(dev, baroff) < _pci_mem_start) {
            _pci_mem_start = pci_resource_start(dev, baroff);
        }
        if (pci_resource_end(dev, baroff) > _pci_mem_end) {
            _pci_mem_end = pci_resource_end(dev, baroff);
        }

#ifdef CONFIG_SANDPOINT
        /*
         * Something wrong with the PCI subsystem in the mousse kernel.
         * The device is programmed correctly, but the irq in the pci
         * structure is hosed. This checks for the hosed-ness and fixes it.
         */
        if (dev->irq > 100) {
            dev->irq = 2;
            gprintk("irq problem: setting irq = %d\n", dev->irq);
        }
#endif

#ifdef CONFIG_BMW
        /*
         * PCI subsystem does not always program the system correctly.
         */
        if (dev->irq < 16 && dev->irq != 2) {
            dev->irq = 2;
            gprintk("irq problem: setting irq = %d\n", dev->irq);
        }
#endif

#ifdef CONFIG_IDT_79EB334
        /*
         * IDT kernel is not currently mapping interrupts correctly
         * Hardwired to core mips interrupt, irq 3 for kernel 2.4.18
         */
        if (dev->irq != 3) {
            dev->irq = 3;
            gprintk("irq problem: setting irq = %d\n", dev->irq);
        }
#endif

#ifdef CONFIG_BCM94702_CPCI
        /*
         * Hardwired to core mips interrupt irq 6 on MBZ
         */
        if (dev->irq != 6) {
            dev->irq = 6;
            gprintk("irq problem: setting irq = %d\n", dev->irq);
        }
#endif

        if ((PCI_FIND_DEV(BCM4704_VENDOR_ID, BCM4704_DEVICE_ID, NULL)) != NULL) {
            /*
             * Decrease the PCI bus priority for the CPU for better overall
             * system performance. This change significantly reduces the
             * number of PCI retries from other devices on the PCI bus.
             */
            void * _mc_vbase = IOREMAP(BCM4704_MEMC_BASE, 0x1000);
            int priorinv = 0x80;
            static int done = 0;
            if (!done) {
                done = 1;
                writel(priorinv, _mc_vbase + BCM4704_MEMC_PRIORINV);
                if (debug >= 1)
                    gprintk("set BCM4704 PriorInvTim register to 0x%x\n", priorinv);
                iounmap(_mc_vbase);
            }
        }

        if ((PCI_FIND_DEV(SIBYTE_PCI_VENDOR_ID, SIBYTE_PCI_DEVICE_ID, NULL)) != NULL) {
            /*
             * The BCM91125CPCI CPU boards with a PCI-PCI bridge use the same
             * interrupt line for all switch ships behind the bridge.
             */
            if (PCI_FIND_DEV(DC21150_VENDOR_ID, DC21150_DEVICE_ID, NULL) ||
                PCI_FIND_DEV(HINT_HB4_VENDOR_ID, HINT_HB4_DEVICE_ID, NULL) ||
                PCI_FIND_DEV(PI7C8150_VENDOR_ID, PI7C8150_DEVICE_ID, NULL)) {
                /*
                 * By default we try to guess the correct IRQ based on the design.
                 * For now we only look at the bridge vendor, but it may be necessary
                 * to look at the switch chip configuration as well.
                 */
                if (forceirq == -1) {
                    if ((PCI_FIND_DEV(HINT_HB4_VENDOR_ID, HINT_HB4_DEVICE_ID, NULL)) ||
                        ((dev->device == BCM5674_DEVICE_ID) &&
                         (PCI_FIND_DEV(PI7C8150_VENDOR_ID, PI7C8150_DEVICE_ID, NULL)))) {
                        forceirq = 58;
                    } else {
                        forceirq = 56;
                    }
                }
            }
        }

        if (((PCI_FIND_DEV(BCM58525_PCI_VENDOR_ID, BCM58525_PCI_DEVICE_ID, NULL)) != NULL) ||
              ((PCI_FIND_DEV(BCM58525_PCI_VENDOR_ID, BCM58522_PCI_DEVICE_ID, NULL)) != NULL)) {
            /* BCM58525 CPU boards support 128 Max payload size */
            if (maxpayload) {
                maxpayload = 128;
                if (debug >= 1) gprintk("force max payload size to 128\n");
            }
        }

        if (forceirq > 0 && dev->irq != (uint32) forceirq) {
            if (forceirqubm & (1U << (_ndevices - 1))) {
                dev->irq = forceirq;
                if (debug >= 1) gprintk("force irq to %d\n", forceirq);
            }
        } else if (debug >= 1) gprintk("found irq %d\n", dev->irq);

        ctrl->iLine = dev->irq;

        /* Search config space for PCIe capabilities */
        pci_read_config_word(dev, PCI_CAPABILITY_LIST, &cap_base);
        while (cap_base) {
            pci_read_config_word(dev, cap_base, &rval);
            if ((rval & 0xff) == PCI_CAP_ID_EXP) {
                pcie = 1;
                break;
            }
            cap_base = (rval >> 8) & 0xff;
        }

        if (cap_base) {
            int max_val, max_cap;
            uint16 devcap, devctl;

            /* Get current device control settings */
            pci_read_config_word(dev, cap_base + PCI_EXP_DEVCTL, &devctl);

            /* Get current max payload setting */
            max_val = (devctl >> 5) & 0x7;

            if (maxpayload) {
                /* Get encoding from byte value */
                max_val = 0;
                while ((1 << (max_val + 7)) < maxpayload) {
                    max_val++;
                }
                if (debug >= 1) {
                    gprintk("Set max payload size = %d (%d)\n",
                            maxpayload, max_val);
                }

                /* Get max supported payload size */
                pci_read_config_word(dev, cap_base + PCI_EXP_DEVCAP, &devcap);
                max_cap = (devcap & 0x7);

                /* Do not exceed device capabilities */
                if (max_val > max_cap) {
                    max_val = max_cap;
                    gprintk("Payload size of %d exceeds device capability\n",
                            maxpayload);
                }

                /* Update max payload size */
                devctl &= ~(0x7 << 5);
                devctl |= (max_val) << 5;

                /* Update max request size */
                devctl &= ~(0x7 << 12);
                devctl |= (max_val << 12);
            }

            /* Always disable relaxed ordering */
            devctl &= ~(1 << 4);

            /* Update device control settings */
            pci_write_config_word(dev, cap_base + PCI_EXP_DEVCTL, devctl);

            /* Warn if non-default setting is used */
            if (max_val > 0) {
                gprintk("Payload size of %d bytes may not be supported by all "
                        "PCIe bridges by default.\n",
                        (1 << (max_val + 7)));
            }
        } else {
            /* Set PCI retry to infinite on non-PCIe switch device */
            pci_write_config_word(dev, 0x40, 0x0080);
            if (debug >= 1) gprintk("set DMA retry to infinite on switch device\n");
        }
    }

#if defined(BCM_DFE_SUPPORT)
    switch (dev->device) {
    case BCM88750_DEVICE_ID:
    case BCM88755_DEVICE_ID:
    case BCM88950_DEVICE_ID:
    
        
        /*
         * For DMA transactions - set Max_Payload_Size and
         * Max_Read_Request_Size to 128 bytes.
         */
        pci_write_config_byte(dev, 0xb5, 0x0c);
        pci_write_config_byte(dev, 0xb4, 0x0);
        break;
    default:
        break;
    }
#endif /* BCM_DFE_SUPPORT */

    /* Prevent compiler warning */
    if (ctrl == NULL) {
        return 0;
    }

    ctrl->be_pio = 0;
    ctrl->dev_type |= BDE_PCI_DEV_TYPE;
    ctrl->pci_device = dev;
    pci_set_drvdata(dev, ctrl);


    /* Look for PCIe vendor-specific extended capability (VSEC) */
    if (pcie) {
        cap_base = PCI_EXT_CAP_START;
        while (cap_base) {
            pci_read_config_dword(dev, cap_base, &cap_val);
            if (PCI_EXT_CAP_ID(cap_val) == PCI_EXT_CAP_ID_VNDR) {
                break;
            }
            cap_base = PCI_EXT_CAP_NEXT(cap_val);
        }
        if (cap_base) {
            /*
             * VSEC layout:
             *
             * 0x00: PCI Express Extended Capability Header
             * 0x04: Vendor-Specific Header
             * 0x08: Vendor-Specific Register 1
             * 0x0c: Vendor-Specific Register 2
             *     ...
             * 0x24: Vendor-Specific Register 8
             */
            pci_read_config_dword(dev, cap_base + 8, &cap_val);
            if (debug >= 1) gprintk("Found VSEC, reg1 = 0x%x\n", cap_val);
            if (cap_val == 0x100) {
                iproc = 1;
                baroff = 2;
            }
        }
    }

    /* Get the device revision */
    pci_read_config_byte(dev, PCI_REVISION_ID, &ctrl->bde_dev.rev);

    /* Map in the device */
    ctrl->bde_dev.device = dev->device;
    paddr = pci_resource_start(dev, baroff);

    switch (dev->device) {
#if defined(BCM_PETRA_SUPPORT) && defined(__DUNE_LINUX_BCM_CPU_PCIE__) 
    case PETRAB_DEVICE_ID:
    case PCP_PCI_DEVICE_ID:
        bar_len = 0x1000000;
        break;
#endif
    default:
        bar_len = pci_resource_len(dev, baroff);
        break;
    }

    ctrl->bde_dev.base_address = (sal_vaddr_t)IOREMAP(paddr, bar_len);
    ctrl->phys_address = paddr;

    /* Map secondary address spaces */
    if (iproc) {
        paddr = pci_resource_start(dev, 0);
        bar_len = pci_resource_len(dev, 0);
        ctrl->bde_dev.base_address1 = (sal_vaddr_t)IOREMAP(paddr, bar_len);
        ctrl->phys_address1 = paddr;
    }

    /* Configure iProc PCI endianess */
    if (iproc && ctrl->bde_dev.base_address1) {
        volatile uint32_t *bar0, *pv;

        /*
         * The following code attempts to auto-detect the correct
         * iProc PCI endianess configuration by reading a well-known
         * register (the endianess configuration register itself).
         * Note that the PCI endianess may be different for different
         * big endian host processors.
         */
        bar0 = (volatile uint32_t *)ctrl->bde_dev.base_address1;
        pv = &bar0[BAR0_PAXB_ENDIANESS/4];
        /* Select big endian */
        *pv = 0x01010101;
        /* Check if endianess register itself is correct endian */
        if (*pv != 1) {
            /* If not, then assume little endian */
            *pv = 0;
        }

        /* Cache IMAP0 registers from the current PAXB */
        for (i = 0; i < IPROC_SUBWIN_MAX; i++) {
            pv = &bar0[BAR0_PAXB_IMAP0_0 / 4];
            rval32 = pv[i];
            if (rval32 & 1) { /* Valid Bit */
                iproc_map[0]->subwin[i].addr_min = rval & ~(0xfff);
                iproc_map[0]->subwin[i].addr_max = rval | 0xfff;
            } else {
                iproc_map[0]->subwin[i].addr_min = 0;
                iproc_map[0]->subwin[i].addr_max = 0;
            }
        }

        if (iproc_map[0]->subwin[2].addr_min  & 0x1000) {
            /* PAXB_1 */
            ctrl->dev_type |= BDE_DEV_BUS_ALT;
        }

        /* Enable iProc DMA to external host memory */
        pv = &bar0[BAR0_PAXB_OARR_2 / 4];
        *pv = 1;
        pv = &bar0[BAR0_PAXB_OARR_2_UPPER / 4];
        iproc_ver_get(ctrl->bde_dev, &iproc_rev);
        if (iproc_rev == 7) {
            *pv = 2;
        } else {
            *pv = 1;
        }
#if defined(CONFIG_PCI_MSI)
        if (use_msi) {
            pv = &bar0[BAR0_PAXB_OARR_FUNC0_MSI_PAGE / 4];
            *pv |= 1;
        }
#endif
    }


    /*
     * Since the GMAC driver of Robo chips needs access to the
     * ChipCommon and Wrapper registers, we set the base address
     * as the enumeration base address and its size as 3MB to
     * cover all Wrapper register regions. 
     */
    if (gmac_base) {
        uint32_t offset;

        ctrl->bde_dev.base_address = (sal_vaddr_t)IOREMAP(gmac_base, 0x300000);

        /* Record the base address of GMAC core */
        offset = ctrl->phys_address - gmac_base;
        ctrl->alt_base_addr = ctrl->bde_dev.base_address + offset;
        ctrl->phys_address = gmac_base;
    }

    /*
     * Workaround bug in FE2K A1 part; shows as A0 part in PCI config space,
     * read the FE's regs directly to get the true revision
     */
    if (ctrl->bde_dev.device == BCM88020_DEVICE_ID && ctrl->bde_dev.rev == 0) {
#define FE2000_REVISION_OFFSET      (0x0)
        uint32_t fe_rev;

        fe_rev = *((uint32_t*)(ctrl->bde_dev.base_address + FE2000_REVISION_OFFSET));
        if ((fe_rev >> 16) == BCM88020_DEVICE_ID) {
            fe_rev &= 0xff;
        } else {
            fe_rev = (fe_rev >> 24) & 0xff;
        }
        ctrl->bde_dev.rev = fe_rev;
    }

    ctrl->isr = NULL;
    ctrl->isr_data = NULL;

    pci_read_config_word(dev, PCI_COMMAND, &cmd);
    if (!(cmd & PCI_COMMAND_MEMORY) || !(cmd & PCI_COMMAND_MASTER)) {
        cmd |= PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
        pci_write_config_word(dev, PCI_COMMAND, cmd);
        if (debug >= 1) gprintk("enable PCI resources 0x%x (PCI_COMMAND)\n", cmd);
    }

    /* Check if we need 256 KB memory window (default is 64 KB) */
    bar_len = pci_resource_len(dev, baroff);
    if ((bar_len == 0x40000) || (bar_len == 0x800000)) {
        ctrl->dev_type |= BDE_256K_REG_SPACE;
        if (debug >= 1) gprintk("PCI resource len 256K\n");
    }

#if defined (BCM_ROBO_SUPPORT) && !defined(ALTA_ROBO_SPI)
    /* MDC/MDIO path for pseudo PHY access to ROBO register on BCM5836/4704 */
    if (dev->device == BCM47XX_ENET_ID) {
        ((uint32 *)ctrl->bde_dev.base_address)[0x410 / 4] = 0;
    }
#endif

#ifdef LINUX_BDE_DMA_DEVICE_SUPPORT
    ctrl->dma_dev = &dev->dev;
#endif

    /* Let's boogie */
    return 0;
}

/*
 * Function: _pci_remove
 *
 * Purpose:
 *    Detach driver from device. Called from pci_unregister_driver().
 * Parameters:
 *    dev - Linux PCI device structure
 * Returns:
 *    0
 */
static void
_pci_remove(struct pci_dev* dev)
{
    bde_ctrl_t *ctrl;

    if (nodevices == 1) {
        return; 
    }

#if defined(BCM_DFE_SUPPORT)
    if (dev->device == PCI_DEVICE_ID_PLX_9056) {
        return;
    }
#endif

    ctrl = (bde_ctrl_t *) pci_get_drvdata(dev);

    if (ctrl == NULL) {
        /* Unused device */
        return;
    }

    if (ctrl->bde_dev.base_address1) {
        iounmap((void *)ctrl->bde_dev.base_address1);
    }
    if (ctrl->bde_dev.base_address) {
        iounmap((void *)ctrl->bde_dev.base_address);
    }

    /* Free our interrupt handler, if we have one */
    if (ctrl->isr || ctrl->isr2) {
        free_irq(ctrl->iLine, ctrl);
#if defined(CONFIG_PCI_MSI)
        if (use_msi) {
            pci_disable_msi(ctrl->pci_device);
        }
#endif
        ctrl->isr = NULL;
        ctrl->isr_data = NULL;
        ctrl->isr2 = NULL;
        ctrl->isr2_data = NULL;
    }
}


/*
 * PCI device table.
 * Populated from the include/soc/devids.h file.
 */

static struct pci_device_id _id_table[] = {
    { BROADCOM_VENDOR_ID, BCM5675_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM5676_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56218X_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56218_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56219_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56218R_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56219R_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56214_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56215_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56214R_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56215R_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56216_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56217_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56212_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56213_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56230_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56231_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53718_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53714_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53716_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56018_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56014_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56224_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56225_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56226_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56227_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56228_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56229_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56024_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56025_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53724_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53726_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56100_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56101_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56102_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56105_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56106_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56107_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56110_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56111_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56112_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56115_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56116_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56117_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56300_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56301_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56302_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56303_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56304_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56404_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56305_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56306_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56307_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56308_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56309_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56310_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56311_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56312_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56313_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56314_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56315_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56316_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56317_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56318_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56319_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#ifndef EXCLUDE_BCM56324
    { BROADCOM_VENDOR_ID, BCM56322_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56324_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#endif /* EXCLUDE_BCM56324 */
    { BROADCOM_VENDOR_ID, BCM53312_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53313_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53314_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53324_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53333_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53334_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53342_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53343_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53344_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53346_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53347_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53393_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53394_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53300_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53301_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53302_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56500_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56501_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56502_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56503_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56504_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56505_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56506_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56507_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56508_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56509_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56510_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56511_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56512_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56513_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56514_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56516_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56517_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56518_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56519_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56580_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56620_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56624_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56626_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56628_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56629_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56680_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56684_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56700_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56701_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56720_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56721_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56725_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56800_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56801_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56802_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56803_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56820_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56821_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56822_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56823_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56825_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56630_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56634_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56636_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56638_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56639_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56538_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56520_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56521_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56522_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56524_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56526_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56534_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56685_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56689_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56331_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56333_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56334_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56338_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56320_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56321_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56132_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56134_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88732_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56140_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56142_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56143_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56144_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56146_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56147_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56149_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56150_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56151_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56152_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56613_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56930_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56931_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56935_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56936_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56939_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56840_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56841_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56842_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56843_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56844_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56845_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56846_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56847_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },    
    { BROADCOM_VENDOR_ID, BCM56549_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56053_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56838_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56831_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56835_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },    
    { BROADCOM_VENDOR_ID, BCM56849_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },    
    { BROADCOM_VENDOR_ID, BCM56743_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56744_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56745_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56746_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56640_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56548_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56547_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56347_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56346_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56344_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56342_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56340_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56049_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56048_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56047_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56042_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56041_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56040_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56643_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56644_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56648_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56649_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56540_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56541_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56542_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56543_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56544_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56545_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56546_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56044_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56045_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56046_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88230_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88030_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88034_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88039_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88231_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88235_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88236_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88239_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM55440_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56440_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56441_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56442_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56443_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56445_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56446_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56447_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56448_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56449_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56240_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56241_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56242_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56243_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56245_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56246_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM55450_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM55455_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56248_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56450_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56452_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56454_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56455_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56456_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56457_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56458_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56850_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56851_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56852_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56853_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56854_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56855_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56834_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },    
    { BROADCOM_VENDOR_ID, BCM56750_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56830_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM55440_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM55441_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56060_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56062_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56063_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56064_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53401_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53411_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53402_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53412_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53403_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53413_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53404_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53414_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53405_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53415_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53406_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53416_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53408_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53418_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },

#ifdef BCM_ROBO_SUPPORT
    { BROADCOM_VENDOR_ID, BCM47XX_ENET_ID, PCI_ANY_ID, PCI_ANY_ID },
#ifdef KEYSTONE 
    { BROADCOM_VENDOR_ID, BCM53000_GMAC_ID, PCI_ANY_ID, PCI_ANY_ID },
#endif
#endif
    { SANDBURST_VENDOR_ID, QE2000_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { SANDBURST_VENDOR_ID, BCM88020_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { SANDBURST_VENDOR_ID, BCM88025_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { PCI_VENDOR_ID_PLX, PCI_DEVICE_ID_PLX_9656, PCI_ANY_ID, PCI_ANY_ID },
    { PCI_VENDOR_ID_PLX, PCI_DEVICE_ID_PLX_9056, PCI_ANY_ID, PCI_ANY_ID },
    { BCM53000_VENDOR_ID, BCM53000PCIE_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#ifdef BCM_PETRA_SUPPORT 
    { BROADCOM_VENDOR_ID, BCM88650_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88350_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88351_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88450_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88451_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88550_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88551_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88552_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88651_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88654_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { PCP_PCI_VENDOR_ID, PCP_PCI_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { ACP_PCI_VENDOR_ID, ACP_PCI_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88660_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88670_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88202_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88360_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88361_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88363_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88460_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88461_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88560_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88561_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88562_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88661_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88664_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#endif
#ifdef BCM_DFE_SUPPORT
    { BROADCOM_VENDOR_ID, BCM88750_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88755_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88950_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#endif
    { 0, 0, 0, 0 }
};

static struct pci_driver _device_driver = {
    probe: _pci_probe,
    remove: _pci_remove,
    id_table: _id_table,
    /* The rest are dynamic */
};

static void
_spi_device_setup(void) {
    bde_ctrl_t *ctrl;
    ctrl = _devices + _ndevices++;
    _switch_ndevices++;
    ctrl->dev_type |= BDE_SWITCH_DEV_TYPE;
    ctrl->iLine = 0xa3;
    ctrl->be_pio = 0;
    ctrl->dev_type |= BDE_SPI_DEV_TYPE;
    ctrl->bde_dev.device = spi_devid;
    ctrl->bde_dev.rev = spi_revid;
    if (debug >= 1) {
        gprintk("SPI Slave Mode: force ctrl->bde_dev.device=0x%x\n",ctrl->bde_dev.device);
        gprintk("SPI Slave Mode: force ctrl->bde_dev.rev=0x%x\n",ctrl->bde_dev.rev);
        gprintk("SPI Slave Mode: force ctrl->dev_type=0x%x\n",ctrl->dev_type);
    }
}
#endif /* BCM_ICS */


#ifdef BCM_ROBO_SUPPORT

#ifdef KEYSTONE
#define DEFAULT_FREQ    (SPI_FREQ_DEFAULT)
#define FREQ_20MHZ      (SPI_FREQ_20MHZ)
#else /* IPROC_CMICD */   
#define DEFAULT_FREQ    (0)
#define FREQ_20MHZ      (0)
#endif


/*
* The model_info /rev_info for Robo devices is defined like this:
*
* 31 28 27     24 23  20 19 16 15      8 7      0
* +----+---------+------+-----+---------+--------+
* | op | reserved| mask |len  | page    |offset  |
* +----+---------+------+-----+---------+--------+
*
* op:          1:OR phyidl, 2: use PCIE device ID
* mlen:      mask len (in bytes) 1:means 0xf,2 means 0xff
* len:         Size of model/rev ID register (in bytes)
* page:      Page containing model ID and revision registers
* offset:     Model/rev ID register offset
*/
static struct bde_spi_device_id _spi_id_table[] = {
    { BCM53242_PHYID_HIGH, BCM53242_PHYID_LOW , 0, 0, DEFAULT_FREQ},
    { BCM53262_PHYID_HIGH, BCM53262_PHYID_LOW , 0, 0, DEFAULT_FREQ},
    { BCM53115_PHYID_HIGH, BCM53115_PHYID_LOW , 0, 0, DEFAULT_FREQ},
    { BCM53118_PHYID_HIGH, BCM53118_PHYID_LOW , 0, 0, DEFAULT_FREQ},
    { BCM53280_PHYID_HIGH, BCM53280_PHYID_LOW , 0x101800e8, 0, FREQ_20MHZ},
    { BCM53101_PHYID_HIGH, BCM53101_PHYID_LOW , 0, 0x110240, FREQ_20MHZ},
    { BCM53125_PHYID_HIGH, BCM53125_PHYID_LOW , 0, 0x110240, FREQ_20MHZ},
    { BCM53128_PHYID_HIGH, BCM53128_PHYID_LOW , 0, 0x110240, FREQ_20MHZ},
    { BCM53600_PHYID_HIGH, BCM53600_PHYID_LOW , 0x101800e8, 0, FREQ_20MHZ},
    { BCM89500_PHYID_HIGH, BCM89500_PHYID_LOW ,0x240230, 0x110240, FREQ_20MHZ},
    { BCM53010_PHYID_HIGH, BCM53010_PHYID_LOW ,0x240230, 0x110240, 0},
    { BCM53018_PHYID_HIGH, BCM53018_PHYID_LOW ,0x240230, 0x110240, 0},
    { BCM5389_PHYID_HIGH, BCM5389_PHYID_LOW, 0x110230, 0x110240, DEFAULT_FREQ},
    { BCM53020_PHYID_HIGH, BCM53020_PHYID_LOW ,0x20240230, 0x110240, 0},
    { BCM5396_PHYID_HIGH, BCM5396_PHYID_LOW, 0x110230, 0x110240, DEFAULT_FREQ},
    { 0, 0, 0, 0, 0 },
};
#endif


/* DMA memory allocation */

#define ONE_KB 1024
#define ONE_MB (1024*1024)

/* Default DMA memory size */
#ifdef SAL_BDE_DMA_MEM_DEFAULT
#define DMA_MEM_DEFAULT (SAL_BDE_DMA_MEM_DEFAULT * ONE_MB)
#else
#define DMA_MEM_DEFAULT (8 * ONE_MB)
#endif
#define DMA_MEM_DEFAULT_ROBO (4 * ONE_MB)


/* We try to assemble a contiguous segment from chunks of this size */
#define DMA_BLOCK_SIZE (512 * ONE_KB)

typedef struct _dma_segment {
    struct list_head list;
    unsigned long req_size;     /* Requested DMA segment size */
    unsigned long blk_size;     /* DMA block size */
    unsigned long blk_order;    /* DMA block size in alternate format */
    unsigned long seg_size;     /* Current DMA segment size */
    unsigned long seg_begin;    /* Logical address of segment */
    unsigned long seg_end;      /* Logical end address of segment */
    unsigned long *blk_ptr;     /* Array of logical DMA block addresses */
    int blk_cnt_max;            /* Maximum number of block to allocate */
    int blk_cnt;                /* Current number of blocks allocated */
} dma_segment_t;

static unsigned int _dma_mem_size = DMA_MEM_DEFAULT;
static mpool_handle_t _dma_pool = NULL;
static void *_dma_vbase = NULL;
static uint32_t _dma_pbase = 0;
static int _use_himem = 0;
static LIST_HEAD(_dma_seg);

/*
 * Function: _find_largest_segment
 *
 * Purpose:
 *    Find largest contiguous segment from a pool of DMA blocks.
 * Parameters:
 *    dseg - DMA segment descriptor
 * Returns:
 *    0 on success, < 0 on error.
 * Notes:
 *    Assembly stops if a segment of the requested segment size
 *    has been obtained.
 *
 *    Lower address bits of the DMA blocks are used as follows:
 *       0: Untagged
 *       1: Discarded block
 *       2: Part of largest contiguous segment
 *       3: Part of current contiguous segment
 */
static int
_find_largest_segment(dma_segment_t *dseg)
{
    int i, j, blks, found;
    unsigned long b, e, a;

    blks = dseg->blk_cnt;
    /* Clear all block tags */
    for (i = 0; i < blks; i++) {
        dseg->blk_ptr[i] &= ~3;
    }
    for (i = 0; i < blks && dseg->seg_size < dseg->req_size; i++) {
        /* First block must be an untagged block */
        if ((dseg->blk_ptr[i] & 3) == 0) {
            /* Initial segment size is the block size */
            b = dseg->blk_ptr[i];
            e = b + dseg->blk_size;
            dseg->blk_ptr[i] |= 3;
            /* Loop looking for adjacent blocks */
            do {
                found = 0;
                for (j = i + 1; j < blks && (e - b) < dseg->req_size; j++) {
                    a = dseg->blk_ptr[j];
                    /* Check untagged blocks only */
                    if ((a & 3) == 0) {
                        if (a == (b - dseg->blk_size)) {
                            /* Found adjacent block below current segment */
                            dseg->blk_ptr[j] |= 3;
                            b = a;
                            found = 1;
                        } else if (a == e) {
                            /* Found adjacent block above current segment */
                            dseg->blk_ptr[j] |= 3;
                            e += dseg->blk_size;
                            found = 1;
                        }
                    }
                }
            } while (found);
            if ((e - b) > dseg->seg_size) {
                /* The current block is largest so far */
                dseg->seg_begin = b;
                dseg->seg_end = e;
                dseg->seg_size = e - b;
                /* Re-tag current and previous largest segment */
                for (j = 0; j < blks; j++) {
                    if ((dseg->blk_ptr[j] & 3) == 3) {
                        /* Tag current segment as the largest */
                        dseg->blk_ptr[j] &= ~1;
                    } else if ((dseg->blk_ptr[j] & 3) == 2) {
                        /* Discard previous largest segment */
                        dseg->blk_ptr[j] ^= 3;
                    }
                }
            } else {
                /* Discard all blocks in current segment */
                for (j = 0; j < blks; j++) {
                    if ((dseg->blk_ptr[j] & 3) == 3) {
                        dseg->blk_ptr[j] &= ~2;
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * Function: _alloc_dma_blocks
 *
 * Purpose:
 *    Allocate DMA blocks and add them to the pool.
 * Parameters:
 *    dseg - DMA segment descriptor
 *    blks - number of DMA blocks to allocate
 * Returns:
 *    0 on success, < 0 on error.
 * Notes:
 *    DMA blocks are allocated using the page allocator.
 */
static int
_alloc_dma_blocks(dma_segment_t *dseg, int blks)
{
    int i, start;
    unsigned long addr;

    if (dseg->blk_cnt + blks > dseg->blk_cnt_max) {
        gprintk("No more DMA blocks\n");
        return -1;
    }
    start = dseg->blk_cnt;
    dseg->blk_cnt += blks;
    for (i = start; i < dseg->blk_cnt; i++) {
        /*
         * Note that we cannot use pci_alloc_consistent when we
         * want to be able to map DMA memory to user space.
         *
         * The GFP_DMA flag is omitted as this imposes the ISA
         * addressing limitations on x86 platforms. As long as
         * we have less than 1GB of memory, we can do PCI DMA
         * to all physical RAM locations.
         */
        addr = __get_free_pages(mem_flags, dseg->blk_order);
        if (addr) {
            dseg->blk_ptr[i] = addr;
        } else {
            gprintk("DMA allocation failed\n");
            return -1;
        }
    }
    return 0;
}

/*
 * Function: _dma_segment_alloc
 *
 * Purpose:
 *    Allocate large physically contiguous DMA segment.
 * Parameters:
 *    size - requested DMA segment size
 *    blk_size - assemble segment from blocks of this size
 * Returns:
 *    DMA segment descriptor.
 * Notes:
 *    Since we cannot allocate large blocks of contiguous
 *    memory from the kernel, we simply keep allocating
 *    smaller chunks until we can assemble a contiguous
 *    block of the desired size.
 *
 *    When system allowed maximum bytes of memory has been allocated
 *    without a successful assembly of a contiguous DMA
 *    segment, the allocation function will return the
 *    largest contiguous segment found so far. It is up
 *    to the calling function to decide whether this
 *    amount is sufficient to proceed.
 */
static dma_segment_t *
_dma_segment_alloc(size_t size, size_t blk_size)
{
    dma_segment_t *dseg;
    int i, blk_ptr_size;
    unsigned long page_addr;
    struct sysinfo si;

    /* Sanity check */
    if (size == 0 || blk_size == 0) {
        return NULL;
    }
    /* Allocate an initialize DMA segment descriptor */
    if ((dseg = kmalloc(sizeof(dma_segment_t), GFP_KERNEL)) == NULL) {
        return NULL;
    }
    memset(dseg, 0, sizeof(dma_segment_t));
    dseg->req_size = size;
    dseg->blk_size = PAGE_ALIGN(blk_size);
    while ((PAGE_SIZE << dseg->blk_order) < dseg->blk_size) {
        dseg->blk_order++;
    }

    si_meminfo(&si);
    dseg->blk_cnt_max = (si.totalram << PAGE_SHIFT) / dseg->blk_size;
    blk_ptr_size = dseg->blk_cnt_max * sizeof(unsigned long);
    /* Allocate an initialize DMA block pool */
    dseg->blk_ptr = KMALLOC(blk_ptr_size, GFP_KERNEL);
    if (dseg->blk_ptr == NULL) {
        kfree(dseg);
        return NULL;
    }
    memset(dseg->blk_ptr, 0, blk_ptr_size);
    /* Allocate minimum number of blocks */
    _alloc_dma_blocks(dseg, dseg->req_size / dseg->blk_size);
    /* Allocate more blocks until we have a complete segment */
    do {
        _find_largest_segment(dseg);
        if (dseg->seg_size >= dseg->req_size) {
            break;
        }
    } while (_alloc_dma_blocks(dseg, 8) == 0);
    /* Reserve all pages in the DMA segment and free unused blocks */
    for (i = 0; i < dseg->blk_cnt; i++) {
        if ((dseg->blk_ptr[i] & 3) == 2) {
            dseg->blk_ptr[i] &= ~3;
            for (page_addr = dseg->blk_ptr[i];
                 page_addr < dseg->blk_ptr[i] + dseg->blk_size;
                 page_addr += PAGE_SIZE) {
                MEM_MAP_RESERVE(VIRT_TO_PAGE(page_addr));
            }
        } else if (dseg->blk_ptr[i]) {
            dseg->blk_ptr[i] &= ~3;
            free_pages(dseg->blk_ptr[i], dseg->blk_order);
            dseg->blk_ptr[i] = 0;
        }
    }
    return dseg;
}

/*
 * Function: _dma_segment_free
 *
 * Purpose:
 *    Release resources used by DMA segment.
 * Parameters:
 *    dseg - DMA segment descriptor
 * Returns:
 *    Nothing.
 */
static void
_dma_segment_free(dma_segment_t *dseg)
{
    int i;
    unsigned long page_addr;

    if (dseg->blk_ptr) {
        for (i = 0; i < dseg->blk_cnt; i++) {
            if (dseg->blk_ptr[i]) {
                for (page_addr = dseg->blk_ptr[i];
                     page_addr < dseg->blk_ptr[i] + dseg->blk_size;
                     page_addr += PAGE_SIZE) {
                    MEM_MAP_UNRESERVE(VIRT_TO_PAGE(page_addr));
                }
                free_pages(dseg->blk_ptr[i], dseg->blk_order);
            }
        }
        kfree(dseg->blk_ptr);
        kfree(dseg);
    }
}

/*
 * Function: _pgalloc
 *
 * Purpose:
 *    Allocate DMA memory using page allocator
 * Parameters:
 *    size - number of bytes to allocate
 * Returns:
 *    Pointer to allocated DMA memory or NULL if failure.
 * Notes:
 *    For any sizes less than DMA_BLOCK_SIZE, we ask the page
 *    allocator for the entire memory block, otherwise we try
 *    to assemble a contiguous segment ourselves.
 */
static void *
_pgalloc(size_t size)
{
    dma_segment_t *dseg;
    size_t blk_size;

    blk_size = (size < DMA_BLOCK_SIZE) ? size : DMA_BLOCK_SIZE;
    if ((dseg = _dma_segment_alloc(size, blk_size)) == NULL) {
        return NULL;
    }
    if (dseg->seg_size < size) {
        /* If we didn't get the full size then forget it */
        _dma_segment_free(dseg);
        return NULL;
    }
    list_add(&dseg->list, &_dma_seg);
    return (void *)dseg->seg_begin;
}

/*
 * Function: _pgfree
 *
 * Purpose:
 *    Free memory allocated by _pgalloc
 * Parameters:
 *    ptr - pointer returned by _pgalloc
 * Returns:
 *    0 if succesfully freed, otherwise -1.
 */
static int
_pgfree(void *ptr)
{
    struct list_head *pos;
    list_for_each(pos, &_dma_seg) {
        dma_segment_t *dseg = list_entry(pos, dma_segment_t, list);
        if (ptr == (void *)dseg->seg_begin) {
            list_del(&dseg->list);
            _dma_segment_free(dseg);
            return 0;
        }
    }
    return -1;
}

/*
 * Function: _pgcleanup
 *
 * Purpose:
 *    Free all memory allocated by _pgalloc
 * Parameters:
 *    None
 * Returns:
 *    Nothing.
 */
static void
_pgcleanup(void)
{
#if _SIMPLE_MEMORY_ALLOCATION_
    if (_dma_vbase) {
        if (debug >= 1) gprintk("freeing v=%p p=0x%lx size=0x%lx\n", _dma_vbase,(unsigned long) _dma_pbase, (unsigned long)_dma_mem_size);
        dma_free_coherent(0, _dma_mem_size, _dma_vbase, _dma_pbase);
    }

#else /* _SIMPLE_MEMORY_ALLOCATION_ */

    struct list_head *pos, *tmp;
    list_for_each_safe(pos, tmp, &_dma_seg) {
        dma_segment_t *dseg = list_entry(pos, dma_segment_t, list);
        list_del(&dseg->list);
        _dma_segment_free(dseg);
    }
#endif /* _SIMPLE_MEMORY_ALLOCATION_ */
}

/*
 * Function: _alloc_mpool
 *
 * Purpose:
 *    Allocate DMA memory pool
 * Parameters:
 *    size - size of DMA memory pool
 * Returns:
 *    Nothing.
 * Notes:
 *    If set up to use high memory, we simply map the memory into
 *    kernel space.
 *    It is assumed there is only one pool.
 */
static void
_alloc_mpool(size_t size)
{
    unsigned long pbase = 0; 

    if (_use_himem) {
        /* Use high memory for DMA */
        pbase = virt_to_bus(high_memory);
        if (((pbase + size) >> 16) > DMA_BIT_MASK(16)) {
            gprintk("DMA in high memory at 0x%lx size 0x%lx is beyond the 4GB limit and not supported.\n", pbase, (unsigned long)size);
            return;
        }
        _dma_pbase = pbase;
        _dma_vbase = IOREMAP(_dma_pbase, size);
    } else {
        /* Get DMA memory from kernel */
#if _SIMPLE_MEMORY_ALLOCATION_
        size_t alloc_size = size; /* size of memory allocated in current iteration */
        if (alloc_size > DMA_MAX_ALLOC_SIZE) {
            alloc_size = DMA_MAX_ALLOC_SIZE;
        }
        /* get a memory allocation from the kernel */
        {
            dma_addr_t dma_handle;
            if (!(_dma_vbase = dma_alloc_coherent(0, alloc_size, &dma_handle, GFP_KERNEL)) || !dma_handle) {
                gprintk("_alloc_mpool: Kernel failed to allocate the memory pool of size 0x%lx\n", (unsigned long)alloc_size);
                return;
            }
            pbase = dma_handle;
        }

        if (alloc_size != size) {
            gprintk("_alloc_mpool: allocated 0x%lx bytes instead of 0x%lx bytes.\n", (unsigned long)alloc_size, (unsigned long)size);
        }
        size = _dma_mem_size = alloc_size;

#else /* _SIMPLE_MEMORY_ALLOCATION_ */
        _dma_vbase = _pgalloc(size);
        pbase = virt_to_bus(_dma_vbase);
#endif /* _SIMPLE_MEMORY_ALLOCATION_ */
        if (debug >= 1) gprintk("_alloc_mpool: _dma_vbase:%p pbase:%lx  allocated:%lx\n", _dma_vbase, pbase, (unsigned long)size);
        if (((pbase + size) >> 16) > DMA_BIT_MASK(16)) {
            _dma_vbase = NULL;
            gprintk("DMA memory allocated at 0x%lx size 0x%lx is beyond the 4GB limit and not supported.\n", pbase, (unsigned long)size);
            _pgcleanup();
            return;
        }
        _dma_pbase = pbase;
#ifdef REMAP_DMA_NONCACHED
        _dma_vbase = IOREMAP(_dma_pbase, size);
#endif
    }
}
#ifdef BCM_ROBO_SUPPORT

static int
_spi_device_valid_check(unsigned short phyidh,unsigned short phyidl, uint8 check_flag)
{
    struct bde_spi_device_id *_ids;
    int idx, match_idx;

    match_idx = -1;
    idx = 0;

    if (check_flag == 0){
    /* check_flag == 0 check phyidh only*/
        for (_ids = _spi_id_table;
            _ids->phyid_high && _ids->phyid_low; _ids++){
            if (_ids->phyid_high == phyidh) {
                return 0;
            }
        }
        /* No valid SPI devices found */
        return 1;
    } else {        
        while(_spi_id_table[idx].phyid_high){
            if (phyidh == _spi_id_table[idx].phyid_high &&
                phyidl == _spi_id_table[idx].phyid_low) {
                /* Found a match */
                match_idx = idx;
                break;
            }
            idx++;
        }
        return match_idx;
    }
}

#if defined(IPROC_CMICD) || defined(KEYSTONE)
#define ROBO_ATTACH_AVAIL
#endif

#ifdef ROBO_ATTACH_AVAIL

#define SOC_ATTACH(_sc)\
  ai_soc_kattach(_sc)
  
#if defined(IPROC_CMICD)
#define ROBO_ATTACH(_sih, _ss)\
    robo_attach(_sih)
#define MAX_BUSTYPE 1
#define ROBO_SWITCH_BUS(_robo, _bustype)
#define ROBO_SELECT_DEVICE(_robo, _phyidh, _phyidl)
#else  /* KEYSTONE */
#define ROBO_ATTACH(_sih, _ss)\
    robo_attach(_sih, _ss)
/* bustype 2: ROBO_MDCMDIO_BUS, 1: ROBO_SPI_BUS */
#define MAX_BUSTYPE 2
#define ROBO_SWITCH_BUS(_robo, _bustype)\
    robo_switch_bus(_robo, _bustype)

#define ROBO_SELECT_DEVICE(_robo, _phyidh, _phyidl) \
    robo_select_device(_robo, _phyidh, _phyidl)
#endif

#else

#define SOC_ATTACH(_sc)        (NULL)
#define ROBO_ATTACH(_sih, _ss)    (NULL)
#define MAX_BUSTYPE        (0)
#define ROBO_SWITCH_BUS(_robo, _bustype) 
#define ROBO_SELECT_DEVICE(_robo, _phyidh, _phyidl)
#endif


static int
probe_robo_switch(void)
{
    int dev;
    int max_devices, max_bustype;
    uint8   buf[8];
    unsigned short phyidh = 0, phyidl = 0;
#if defined(KEYSTONE) 
    uint32 spi_freq = 0;
#endif
#if defined(IPROC_CMICD)
    sal_vaddr_t addr_base;
    uint32  data_reg;
#endif /* IPROC_CMICD */ 


    spin_lock_init(&bus_lock);

    if (_switch_ndevices) {
    /*
     * Currently skip probe robo if esw chips were found
     * FIX this while combined plateform support.
     */
        return robo_switch;
    }

    /* Get Robo device handle */
    if (robo == NULL) {
        sbh = (void *)SOC_ATTACH(NULL);
        if (sbh == NULL) {
            return -ENODEV;
        }
        robo = (void *)ROBO_ATTACH(sbh, 0);
    }
    if (robo == NULL) {
        return -ENODEV;
    }

    max_bustype = MAX_BUSTYPE + 1;

    while(_spi_device_valid_check(phyidh, 0, 0)) {
        max_bustype --;
        if(!max_bustype)
            return -ENODEV;
        ROBO_SWITCH_BUS(robo, max_bustype);
        buf[0] = buf[1] = 0;
        ROBO_RREG(robo, 0, 0x10, 0x04, buf, (uint)2);
        phyidh = buf[0] | (buf[1] << 8);
        /* re-try */
        if ((phyidh == 0x0) || (phyidh == 0xffff)) {
            ROBO_RREG(robo, 0, 0x10, 0x04, buf, (uint)2);
            phyidh = buf[0] | (buf[1] << 8);
        }
    }

    /* For psedo_phy access, only support one robo switch*/
    /* For Northstar, only one switch on SRAB interface */    
    max_devices = (max_bustype == MAX_BUSTYPE) ? 1 : LINUX_BDE_MAX_SWITCH_DEVICES;

    for (dev = 0; dev < max_devices; dev++) {
        bde_ctrl_t *ctrl;
        int match_idx, i;
        unsigned short phyidl_nr; /* phyidl with revision stripped */        
        uint16 model_id;
        uint8 rev_id;
#if defined(KEYSTONE) || defined(IPROC_CMICD)
        uint32 addr, len;
#endif
        uint32 mlen, op;

        if (_switch_ndevices >= LINUX_BDE_MAX_SWITCH_DEVICES) {
            break;
        }
        buf[0] = buf[1] = 0;
        ROBO_RREG(robo, dev, 0x10, 0x04, buf, (uint)2);
        phyidh = buf[0] | (buf[1] << 8);

        buf[0] = buf[1] = 0;
        ROBO_RREG(robo, dev, 0x10, 0x06, buf, (uint)2);
        phyidl = buf[0] | (buf[1] << 8);

        /* Strip revision */
        phyidl_nr = phyidl & 0xfff0;

        match_idx = _spi_device_valid_check(phyidh, phyidl_nr, 1);
        if (match_idx == -1) {
            if (debug >= 1) gprintk("found %d robo device(s).\n", robo_switch);
            break;
        }

        if(_spi_id_table[match_idx].model_info){
#if defined(KEYSTONE) || defined(IPROC_CMICD)
            addr = _spi_id_table[match_idx].model_info & 0xffff;
            len = (_spi_id_table[match_idx].model_info >> 16) & 0xf;
#endif
            ROBO_RREG(robo, dev, (addr >> 8), (addr & 0xff), buf, (uint)len);
            mlen = (_spi_id_table[match_idx].model_info >> 20) & 0xf;
            model_id = 0;
            for (i = 0; i < mlen; i++)
                model_id |= buf[i] << (i << 3);
            op = (_spi_id_table[match_idx].model_info >> 28) & 0xf;
            if(op == 1) {
                model_id |= phyidl_nr;
#if defined(IPROC_CMICD)                
            } else if (op == 2) {
                /* The package id of NS+ is determined by :  
                 * Write 0 to 0x18012120 (PAXB_0_CONFIG_IND_ADDR)
                 * Read 0x18012124 (PAX_B_CONFIG_IND_DATA), 
                 * bits 31:16 will be the device id from OTP space 
                 */
#define     PAXB_ENUM_BASE                (0x18012000)
#define     PAXB_CONFIG_IND_ADDR_OFFSET   (0x120)
#define     PAXB_CONFIG_IND_DATA_OFFSET   (0x124)

                addr_base = (sal_vaddr_t)IOREMAP(PAXB_ENUM_BASE, 0x1000);
                if (!addr_base) {
                    gprintk("ioremap of PAXB registers failed\n"); 
                } else {
                    writel(0x0, (uint32 *)(addr_base + 
                                        PAXB_CONFIG_IND_ADDR_OFFSET)); 
                    data_reg = readl((uint32 *)(addr_base + 
                                        PAXB_CONFIG_IND_DATA_OFFSET));
                    model_id = (data_reg >> 16);
                    iounmap((void *)addr_base);

                    /* 
                     * Some model ID can't be determined by PCIE device ID
                     * It needs to refer some OTP values.
                     */
                    robo_model_id_adjust_from_otp(robo, &model_id);                 
                }
                 
#undef      PAXB_ENUM_BASE                
#undef      PAXB_CONFIG_IND_ADDR_OFFSET
#undef      PAXB_CONFIG_IND_DATA_OFFSET
#endif /* IPROC_CMICD */
            }
        } else {
            model_id = phyidl_nr;
        }
        if(_spi_id_table[match_idx].rev_info){
#if defined(KEYSTONE) || defined(IPROC_CMICD)
            addr = _spi_id_table[match_idx].rev_info & 0xffff;
            len = (_spi_id_table[match_idx].rev_info >> 16) & 0xf;
#endif
            ROBO_RREG(robo, dev, (addr >> 8), (addr & 0xff), buf, (uint)len);
            mlen = (_spi_id_table[match_idx].rev_info >> 20) & 0xf;
            rev_id = 0;
            for (i = 0; i < mlen; i++)
                rev_id |= buf[i] << (i << 3);
        } else {
            rev_id = phyidl & 0xf;
        }

        gprintk("found robo device with %d:%04x:%04x:%04x:%02x\n",
                dev, phyidh, phyidl, model_id, rev_id);

        ROBO_SELECT_DEVICE(robo, phyidh, phyidl);

        /* Match supported chips */
        ctrl = _devices + _ndevices++;
        _switch_ndevices++;

        if (NULL == (ctrl->spi_device = (struct spi_dev *)
                     KMALLOC(sizeof(struct spi_dev), GFP_KERNEL))) {
            gprintk("no memory available");
            return -ENOMEM;
        }        
        ctrl->dev_type = (BDE_SPI_DEV_TYPE | BDE_SWITCH_DEV_TYPE);
        ctrl->spi_device->cid = dev;
        ctrl->spi_device->part = model_id;
        ctrl->spi_device->rev = rev_id;
        ctrl->spi_device->robo = robo;
        ctrl->spi_device->phyid_high = phyidh;
        ctrl->spi_device->phyid_low = phyidl;
        ctrl->bde_dev.device = model_id;
        ctrl->bde_dev.rev = rev_id;
        ctrl->bde_dev.base_address = (sal_vaddr_t)NULL;
        ctrl->isr = NULL;
        ctrl->isr_data = NULL;
        robo_switch++;

#if defined(KEYSTONE) 
        spi_freq = _spi_id_table[match_idx].spifreq;
#endif
    }

#if defined(KEYSTONE) 
    /* Override the SPI frequency from user configuration */ 
    if (spifreq != 0) {
        spi_freq = spifreq;
    }
    if (spi_freq != 0) {
        /*
         * The underlying chip can support the SPI frequency
         * higher than default (2MHz).
         */
        if (spi_freq != SPI_FREQ_DEFAULT) {
            chipc_spi_set_freq(robo, 0, spi_freq); 
        }
    }
#endif
    return robo_switch;

}

#endif

#if defined(BCM_METROCORE_LOCAL_BUS)
static bde_ctrl_t*
map_local_bus(uint64_t addr, uint32_t size)
{
    bde_ctrl_t *ctrl;

    ctrl = _devices + _ndevices++;
    _switch_ndevices++;

    /*
     * For now: use EB type as `local bus'
     * (memory mapped, no DMA, no interrupts)
     * metrocore local bus supports interrupts, but we don't use them.
     */
    ctrl->dev_type |= BDE_EB_DEV_TYPE | BDE_SWITCH_DEV_TYPE
        | (size > 64 * 1024 ? BDE_128K_REG_SPACE : 0);
    ctrl->pci_device = NULL; /* No PCI bus */

    /* Map in the device */
    ctrl->bde_dev.base_address = (sal_vaddr_t)IOREMAP(addr, size);
    ctrl->phys_address = addr;

    return(ctrl);
}

#define BME_REVISION_OFFSET     (0x0)

#endif


#ifdef BCM_METROCORE_LOCAL_BUS
    /*
     * SBX platform has both PCI- and local bus-attached devices
     * The local bus devices have fixed address ranges (and don't
     * support or require DMA), but are otherwise the same as PCI devices
     */
#define FPGA_IRQ                37
#define FPGA_PHYS               0x100E0000
#define BME_PHYS                0x100C0000
#define SE_PHYS                 0x100D0000
#define FPGA_SIZE               0x00004000
#define BME_SIZE                0x00004000
#define MAC0_PHYS               0x100B0000
#define MAC1_PHYS               0x100B8000
#define MAC_SIZE                0x800


/*
 * Please refer to "Supervisor Fabric Module (SFM) Specification"
 * page 23 for the following registers.
 */
#define FPGA_LC_POWER_DISABLE_OFFSET             0x4
#define FPGA_LC_POWER_DISABLE_ENABLE_ALL_MASK    0x1e

#define FPGA_LC_POWER_RESET_OFFSET               0x5
#define FPGA_LC_POWER_RESET_ENABLE_ALL_MASK      0x1e

#define FPGA_SW_SFM_MASTER_MODE_OFFSET           0x14
#define FPGA_SW_SFM_MASTER_MODE_ENABLE_MASK      0x10


static int
probe_metrocore_local_bus(void) {
    bde_ctrl_t *ctrl;
    uint32_t dev_rev_id;
    VOL uint8_t *fpga;

    /*
     * Write the FPGA on the fabric card, to let metrocore
     * line cards out of reset.  We actually don't bother to determine whether
     * the card is a line card or a fabric card because when we do
     * this on the line cards, it has no effect.
     */
    fpga = (uint8_t *) IOREMAP(FPGA_PHYS, FPGA_SIZE);
    fpga[FPGA_SW_SFM_MASTER_MODE_OFFSET]
        |= FPGA_SW_SFM_MASTER_MODE_ENABLE_MASK;
    fpga[FPGA_LC_POWER_DISABLE_OFFSET]
        |= FPGA_LC_POWER_DISABLE_ENABLE_ALL_MASK;
    fpga[FPGA_LC_POWER_RESET_OFFSET]
        |= FPGA_LC_POWER_RESET_ENABLE_ALL_MASK;

    ctrl = map_local_bus(BME_PHYS, BME_SIZE);

    dev_rev_id =
        *((uint32_t *)
          (((uint8_t *) ctrl->bde_dev.base_address) + BME_REVISION_OFFSET));
    ctrl->bde_dev.device = dev_rev_id >> 16;
    ctrl->bde_dev.rev = (dev_rev_id & 0xFF);

    if ((ctrl->bde_dev.device != BME3200_DEVICE_ID) &&
    (ctrl->bde_dev.device != BCM88130_DEVICE_ID)) {
        gprintk("probe_metrocore_local_bus: wrong BME type: "
                "0x%x (vs 0x%x or 0x%x)\n",
                ctrl->bde_dev.device, BME3200_DEVICE_ID, BCM88130_DEVICE_ID);
        return -1;
    }

    ctrl->iLine = FPGA_IRQ;
    ctrl->isr = NULL;
    ctrl->isr_data = NULL;

    /*
     * <BME-- 64k --><SE -- 64k --><FPGA -- 64k -->
     * We start from SE & include the FPGA, which is 128k
     */
    ctrl = map_local_bus(SE_PHYS, 128 * 1024);

    dev_rev_id =
        *((uint32_t *)
          (((uint8_t *) ctrl->bde_dev.base_address) + BME_REVISION_OFFSET));
    ctrl->bde_dev.device = dev_rev_id >> 16;
    ctrl->bde_dev.rev = (dev_rev_id & 0xFF);

    if ((ctrl->bde_dev.device != BME3200_DEVICE_ID) &&
    (ctrl->bde_dev.device != BCM88130_DEVICE_ID)) {
        gprintk("probe_metrocore_local_bus: wrong SE (BME) type: "
                "0x%x (vs 0x%x)\n",
                ctrl->bde_dev.device, BME3200_DEVICE_ID);
        return -1;
    }

    ctrl->iLine = FPGA_IRQ;
    ctrl->isr = NULL;
    ctrl->isr_data = NULL;

    return 0;
}
#endif

#ifdef BCM_PLX9656_LOCAL_BUS

#define DEV_REG_BASE_OFFSET     PL0_OFFSET /* Polaris register base */
#define DEV_REG_DEVID           0          /* Device ID is first register */

/*
 * The difference at map_local_bus2:
 *
 * The PLX9656 PCI-to-LOCAL bridge chip already has been iomapped the
 * whole address space.  So the devices off local bus don't need to be
 * mapped again.  They only need to claim their own sub-space.
 */
static bde_ctrl_t *
map_local_bus2(bde_ctrl_t *plx_ctrl, uint32_t dev_base, uint32_t size)
{
    uint32_t dev_rev_id;
    uint8_t *addr;
    bde_ctrl_t *ctrl;

    ctrl = _devices + _ndevices++;
    _switch_ndevices++;

    /*
     * For now: use EB type as `local bus'
     * (memory mapped, no DMA, no interrupts)
     * metrocore local bus supports interrupts, but we don't use them.
     */
    ctrl->dev_type |= BDE_EB_DEV_TYPE |  BDE_SWITCH_DEV_TYPE;
    ctrl->dev_type |= BDE_320K_REG_SPACE; /* Polaris 18 bits address + FPGA */
    ctrl->pci_device = NULL; /* No PCI bus */

    /* Map in the device */
    ctrl->bde_dev.base_address = plx_ctrl->bde_dev.base_address + dev_base;
    ctrl->phys_address = plx_ctrl->phys_address + (resource_size_t)dev_base;

    addr = (uint8_t *)ctrl->bde_dev.base_address + PL0_REVISION_REG;
    dev_rev_id = readl(addr);
    ctrl->bde_dev.device = dev_rev_id >> 16;
    ctrl->bde_dev.rev = (dev_rev_id & 0xFF);

    switch (ctrl->bde_dev.device) {
    case BCM88130_DEVICE_ID:
    case BME3200_DEVICE_ID:
        break;
    default:
        gprintk("wrong BME type: 0x%x (vs 0x%x or 0x%x)\n",
                    ctrl->bde_dev.device, BME3200_DEVICE_ID, BCM88130_DEVICE_ID);
        return 0;
    }
    return(ctrl);
}

static int
probe_plx_local_bus(void)
{
    bde_ctrl_t *ctrl;
    uint32_t val;
    uint8_t *addr;
    char addr_hi_str[16];

    if (num_plx > 1) {
            printk(KERN_ERR "There's more than one PLX 9656/9056 chip\n");
        return -1;
    }
    addr_hi_str[0] = 0;
#ifdef PHYS_ADDR_IS_64BIT
    sprintf(addr_hi_str, "%08x", (uint32_t)(plx_ctrl.phys_address >> 32));
#endif
    printk(KERN_ERR "Found PLX %04x:%04x vir: 0x%08x phy: 0x%s%08x\n",
           plx_ctrl.bde_dev.device, plx_ctrl.bde_dev.rev,
           plx_ctrl.bde_dev.base_address, addr_hi_str,
           (uint32_t)(plx_ctrl.phys_address)); 

    addr = (uint8_t *)plx_ctrl.bde_dev.base_address + CPLD_OFFSET + CPLD_REVISION_REG;
    val = readl(addr);
    printk(KERN_ERR "plx: CPLD revision %d\n", val & CPLD_REVISION_MASK);
#if 000
    addr = (uint8_t *)plx_ctrl.bde_dev.base_address + CPLD_OFFSET + CPLD_RESET_REG;
    writel(CPLD_RESET_NONE, addr);
#endif
    ctrl = map_local_bus2(&plx_ctrl, PL0_OFFSET, PL0_SIZE);
    if (ctrl == 0)
        return -1;

    /* Uses PLX IRQ for Polaris LC */
    ctrl->iLine = 48;
    ctrl->isr = NULL;
    ctrl->isr_data = NULL;

    return 0;
}

#endif /* BCM_PLX9656_LOCAL_BUS */

#if defined(BCM_EA_SUPPORT)
#if defined(BCM_TK371X_SUPPORT)
static void
probe_tk371x_dev(void)
{ 
    bde_ctrl_t *ctrl;
    int ea_uid=0;

    /* eadevices is from the argument of insmod  */
    for (ea_uid = 0; ea_uid < eadevices; ea_uid++) {
        ctrl = _devices + _ndevices++;
        _switch_ndevices++;
        ctrl->dev_type = (BDE_MII_DEV_TYPE | BDE_SWITCH_DEV_TYPE);
        ctrl->bde_dev.device = TK371X_DEVICE_ID;
        ctrl->bde_dev.rev = 0x0;
        ctrl->bde_dev.base_address = (sal_vaddr_t)NULL;
        ctrl->iLine = 0;
    }
}
#endif /* BCM_TK371X_SUPPORT*/
#endif /* BCM_EA_SUPPORT */


#if defined(BCM_ROBO_SUPPORT) 
#if defined(IPROC_CMICD)
struct chip_device_info {
    uint32 cc_base;         /* Chip-common register base */
    uint32 cc_size;         /* Chip-common register limit */
    uint32 cid_reg_off;     /* Chip id register offset */
};

static struct chip_device_info _chip_table = {
#if defined(IPROC_CMICD)
    0x18000000, 0x00000300, 0x00000000
#else
    0,0,0
#endif  
};      

struct gmac_device_info {
    uint32 cid;             /* chip id */
    uint32 rid;             /* revision id */
    uint32 pid;             /* package id */
        
    uint32 gmac_dev_id;     /* gmac core device id */
    uint32 gmac_base_addr;  /* gmac core base address */
    int gmac_irq;           /* gmac irq number */
};

static struct gmac_device_info _gmac_table[] = {
#if defined(IPROC_CMICD)
    {BCM53010_CHIP_ID, 0, 0, BCM53010_GMAC_ID, 0x18026000, 181}, /* BCM53012 */
    {BCM53010_CHIP_ID, 0, 2, BCM53010_GMAC_ID, 0x18026000, 181}, /* BCM53011 */
    {BCM53010_CHIP_ID, 0, 1, BCM53010_GMAC_ID, 0x18026000, 181}, /* BCM53010 */
    {BCM53018_CHIP_ID, 0, 0, BCM53010_GMAC_ID, 0x18026000, 181}, /* BCM53018 */
    {BCM53018_CHIP_ID, 0, 2, BCM53010_GMAC_ID, 0x18026000, 181}, /* BCM53017 */
    {BCM53018_CHIP_ID, 0, 1, BCM53010_GMAC_ID, 0x18026000, 181}, /* BCM53019 */
    {BCM53020_CHIP_ID, 0, 0, BCM53010_GMAC_ID, 0x18024000, 181}, /* BCM53022 */
#endif
    {0,0,0,0,0,0}
};      

static sal_vaddr_t _cca_base = 0;

static int
_gmac_dev_create(void) 
{
    bde_ctrl_t *ctrl;
    uint32 gmac_base = 0;
    uint32 offset = 0;
    uint32 cca_cid;
    uint32 cid, rid, pid;
    int i = 0, found;

    if (_chip_table.cc_base == 0) {
        gprintk("Create GMAC device failed. Unable to identify CPU.\n");
        return -1;
    }

    /* 1. Determine which CPU/GMAC configuration is now */
    _cca_base = (sal_vaddr_t)IOREMAP(_chip_table.cc_base, _chip_table.cc_size);
    cca_cid = readl((uint32 *)(_cca_base + _chip_table.cid_reg_off));

    cid = cca_cid & CID_ID_MASK;
    rid = (cca_cid & CID_REV_MASK) >> CID_REV_SHIFT;
    pid = (cca_cid & CID_PKG_MASK) >> CID_PKG_SHIFT;

    found = 0;
    for (i = 0; ; i++) {
        if (_gmac_table[i].cid == 0) {
            /* End of table */
            break;
        }
        if ((_gmac_table[i].cid == cid) && 
            (_gmac_table[i].rid == rid) &&
            (_gmac_table[i].pid == pid)) {
            /* found */
            found = 1;
            break;
        }
    }
    if (!found) {
        gprintk("Create GMAC device failed. Unable to identify GMAC device.\n");
    }

    /* 2. Create GMAC device */
    /* fill-in necessary information depends on the CPU/GMAC configuration */
    if ((cid == BCM53010_CHIP_ID) || (cid == BCM53018_CHIP_ID) ||
        (cid == BCM53020_CHIP_ID)) {
        ctrl = _devices + _ndevices++;
        _ether_ndevices++;

        ctrl->dev_type |= BDE_ETHER_DEV_TYPE;
        ctrl->dev_type |= BDE_PCI_DEV_TYPE;

        ctrl->iLine = _gmac_table[i].gmac_irq;
    
        ctrl->be_pio = 0;
    
        ctrl->bde_dev.rev = _gmac_table[i].rid;
        ctrl->bde_dev.device = _gmac_table[i].gmac_dev_id;
    
        gmac_base = 0x18000000;
        offset = _gmac_table[i].gmac_base_addr - gmac_base;
        ctrl->bde_dev.base_address = (sal_vaddr_t)IOREMAP(gmac_base, 0x300000);
        ctrl->alt_base_addr = ctrl->bde_dev.base_address + offset;
        ctrl->phys_address = gmac_base;
    
        ctrl->isr = NULL;
        ctrl->isr_data = NULL;
    }

    return 0;
}
#endif
#endif

/*
 * Generic module functions
 */

/*
 * Function: _init
 *
 * Purpose:
 *    Module initialization.
 *    Attaches to kernel BDE.
 * Parameters:
 *    None
 * Returns:
 *    0 on success, < 0 on error.
 */
static int
_init(void)
{

#ifdef IPROC_CMICD
    if (iproc_has_cmicd()) {
        iproc_cmicd_get_memregion(&iproc_cmicd_resources[IPROC_CMICD_RES_MEM]);
        iproc_platform_driver_register(&iproc_cmicd_driver);
        iproc_platform_device_register(&iproc_cmicd_pdev);
    }
#endif /* IPROC_CMICD */

#ifdef BCM_ICS
    _ics_bde_create();
#else /* PCI */
    /* Register our goodies */
    _device_driver.name = LINUX_KERNEL_BDE_NAME;

#if defined(BCM_ROBO_SUPPORT) 
#if defined(IPROC_CMICD) 
    if (_gmac_dev_create()) {
        return -ENODEV;
    }
#endif
#endif /* defined (BCM_ROBO_SUPPORT)  */

    /* Configure MSI interrupt support */
    use_msi = usemsi;
#if defined(CONFIG_PCI_MSI)
    if (use_msi < 0) {
        /* Compilation flag determines default value */
#if defined(BDE_LINUX_USE_MSI_INTERRUPT)
        use_msi = 1;
#else
        use_msi = 0;
#endif
    }
#else /* !defined(CONFIG_PCI_MSI) */
    if (use_msi != 0) {
        if (use_msi > 0) {
            /* Warn if invalid configuration */
            gprintk("MSI interrupts not supported by kernel\n");
        }
        use_msi = 0;
    }
#endif /* defined(CONFIG_PCI_MSI) */

    if (spi_devid) {
        _spi_device_setup();
    } else {
        if (pci_register_driver(&_device_driver) < 0) {
            return -ENODEV;
        }
    }

    /* Note: PCI-PCI bridge  uses results from pci_register_driver */
    p2p_bridge();

#ifdef BCM_METROCORE_LOCAL_BUS
    if (probe_metrocore_local_bus()) {
        return -1;
    }
#endif
#ifdef BCM_PLX9656_LOCAL_BUS
    if (num_plx > 0) {
        probe_plx_local_bus();
    }
#endif
#endif /* BCM_ICS */

#ifdef BCM_ROBO_SUPPORT
    probe_robo_switch();
#endif

#if defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    petra_device_create();
#endif

#if defined(BCM_TK371X_SUPPORT)
    probe_tk371x_dev();
#endif
    /*
     * Probe for EB Bus devices.
     */
    if (eb_bus) {
        char  *tok;
        uint   irq = -1, eb_rd16bit=0, eb_wr16bit =0;
        unsigned int eb_ba = 0x0;

        gprintk("EB bus info: %s\n", eb_bus);
        tok = strtok(eb_bus,",");
        while (tok) {
            _parse_eb_args(tok, "BA=%x IRQ=%d RD16=%d WR16=%d",
                    &eb_ba, &irq, &eb_rd16bit, &eb_wr16bit);
            _eb_device_create(eb_ba, irq, eb_rd16bit, eb_wr16bit);
            tok = strtok(NULL,",");
        }
    }
    
    /* DMA Setup */
    if (dmasize) {
        if ((dmasize[strlen(dmasize)-1] & ~0x20) == 'M') {
            _dma_mem_size = simple_strtoul(dmasize, NULL, 0);
            _dma_mem_size *= ONE_MB;
        } else {
            gprintk("DMA memory size must be specified as e.g. dmasize=8M\n");
        }
        if (_dma_mem_size & (_dma_mem_size-1)) {
            gprintk("dmasize must be a power of 2 (1M, 2M, 4M, 8M etc.)\n");
            _dma_mem_size = 0;
        }
    } else {
        if(robo_switch){
            _dma_mem_size =  DMA_MEM_DEFAULT_ROBO;
        }
    }

    if (himem) {
        if ((himem[0] & ~0x20) == 'Y' || himem[0] == '1') {
            _use_himem = 1;
        } else if ((himem[0] & ~0x20) == 'N' || himem[0] == '0') {
            _use_himem = 0;
        }
    }

    if (_dma_mem_size) {
        _alloc_mpool(_dma_mem_size);
        if (_dma_vbase == NULL) {
            gprintk("no DMA memory available\n");
        }
        else {
            mpool_init();
            _dma_pool = mpool_create(_dma_vbase, _dma_mem_size);
        }
    }

    /*
     * In order to be backward compatible with the user mode BDE
     * (specifically the interrupt IOCTLs) and the CM, switch devices
     * *must* come first. If this is not the case (due to the probing
     * order), we let the non-switch device(s) drop down to the end of
     * the device array.
     */
    if (_switch_ndevices > 0) {
        bde_ctrl_t tmp_dev;
        int i, s = 0;

        while (s < _switch_ndevices) {
            if (_devices[s].dev_type & BDE_SWITCH_DEV_TYPE) {
                s++;
                continue;
            }
            tmp_dev = _devices[s];
            for (i = s; i < _ndevices - 1; i++) {
                _devices[i] = _devices[i+1];
            }
            _devices[i] = tmp_dev;
        }
    }

    /* Initialize device locks */
    if (_ndevices > 0) {
        int i;

        for (i = 0; i < _ndevices; i++) {
            spin_lock_init(&_devices[i].lock);
        }
    }

    return 0;
}

/*
 * Function: _cleanup
 *
 * Purpose:
 *    Module cleanup function.
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
static int
_cleanup(void)
{
    int i;

    if (_dma_vbase) {
        mpool_destroy(_dma_pool);
        if (_use_himem) {
            iounmap(_dma_vbase);
        } else {
#ifdef REMAP_DMA_NONCACHED
            iounmap(_dma_vbase);
#endif
            _pgcleanup();
        }
        _dma_vbase = NULL;
        _dma_pbase = 0;

    }
#ifdef BCM_ROBO_SUPPORT

#if defined(IPROC_CMICD)
    if (_cca_base) {
        iounmap((void *)_cca_base);
    }
    if (iproc_has_cmicd()) {
        iproc_platform_device_unregister(&iproc_cmicd_pdev);
        iproc_platform_driver_unregister(&iproc_cmicd_driver);
    }
#endif

    if (robo) {
#if defined(KEYSTONE) || defined(IPROC_CMICD)
        robo_detach(robo);
#endif /* KEYSTONE || IPROC_CMICD */
    }
    if (sbh) {
#if defined(KEYSTONE) || defined(IPROC_CMICD)
    ai_soc_detach(sbh);
#endif /* KEYSTONE || IPROC_CMICD */        
    }
#endif
    for (i = 0; i < _ndevices; i++) {
        bde_ctrl_t *ctrl = _devices + i;

        /* free allocated kernel space memory */
        if (ctrl->dev_type & BDE_SPI_DEV_TYPE) {
            if (ctrl->spi_device) {
                kfree(ctrl->spi_device);
            }
        }
    }
#ifdef BCM_ICS
#else
    pci_unregister_driver(&_device_driver);
#endif /* BCM_ICS */
    return 0;
}

/*
 * Function: _pprint
 *
 * Purpose:
 *    Print proc filesystem information.
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
static int
_pprint(void)
{
    int i = 0;

    pprintf("Broadcom Device Enumerator (%s)\n", LINUX_KERNEL_BDE_NAME);
    pprintf("DMA Memory (%s): %d bytes, %d used, %d free%s\n",
            (_use_himem) ? "high" : "kernel",
            (_dma_vbase) ? _dma_mem_size : 0,
            (_dma_vbase) ? mpool_usage(_dma_pool) : 0,
            (_dma_vbase) ? _dma_mem_size - mpool_usage(_dma_pool) : 0,
            USE_LINUX_BDE_MMAP ? ", local mmap" : "");

    if (_ndevices == 0) {
        pprintf("No devices found\n");
    } else {
        pprintf("Devices:\n");
    }
    for (i = 0; i < _ndevices; i++) {
        bde_ctrl_t *ctrl = _devices + i;

        if (ctrl->dev_type & BDE_SWITCH_DEV_TYPE) {
            pprintf("\t%d (swi) : ", i);
        } else if (ctrl->dev_type & BDE_ETHER_DEV_TYPE) {
            pprintf("\t%d (eth) : ", i);
        } else if (ctrl->dev_type & BDE_CPU_DEV_TYPE) {
            pprintf("\t%d (cpu) : ", i);
        } else {
            pprintf("\t%d (?)   : ", i);
        }

        if (ctrl->dev_type & BDE_PCI_DEV_TYPE) {
            pprintf("PCI device 0x%x:0x%x:%d:0x%.8lx:0x%.8lx:%d%s\n",
                    ctrl->pci_device->vendor,
                    ctrl->pci_device->device,
                    ctrl->bde_dev.rev,
                    (unsigned long)pci_resource_start(ctrl->pci_device, 0),
                    (unsigned long)pci_resource_start(ctrl->pci_device, 2),
                    ctrl->pci_device->irq,
                    use_msi ? " (MSI)" : "");
        } else if (ctrl->dev_type & BDE_SPI_DEV_TYPE) {
            pprintf("SPI Device %d:%x:%x:0x%x:0x%x:%d\n",
                    ctrl->spi_device->cid,
                    ctrl->spi_device->part,
                    ctrl->spi_device->rev,
                    ctrl->spi_device->phyid_high,
                    ctrl->spi_device->phyid_low,
                    ctrl->bde_dev.rev);
        } else if (ctrl->dev_type & BDE_ICS_DEV_TYPE) {
            pprintf("ICS Device 0x%x:0x%x\n",
                    ctrl->bde_dev.device,
                    ctrl->bde_dev.rev);
        } else if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            pprintf("AXI Device 0x%x:0x%x:0x%.8lx:%d\n",
                    ctrl->bde_dev.device,
                    ctrl->bde_dev.rev,
                    (unsigned long)ctrl->phys_address,
                    ctrl->iLine);
        } else if (ctrl->dev_type & BDE_EB_DEV_TYPE) {
            pprintf("EB Bus Device 0x%x:0x%x\n",
                    ctrl->bde_dev.device,
                    ctrl->bde_dev.rev);
        }
        if (debug >= 1) {
            pprintf("\t\timask:imask2:fmask 0x%x:0x%x:0x%x\n",
                    ctrl->imask,
                    ctrl->imask2,
                    ctrl->fmask);
        }
    }
    return 0;
}

#if USE_LINUX_BDE_MMAP
/*
 * Some kernels (mainly x86) prevent mapping of kernel RAM memory to
 * user space via the /dev/mem device. The function below provides a
 * backdoor to mapping the DMA pool to user space via the
 * /dev/linux-kernel-bde device.
 */
static int _mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long phys_addr = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;
    unsigned long pool_start = _dma_pbase;
    unsigned long pool_end = pool_start + _dma_mem_size;

    if (phys_addr < pool_start || (phys_addr + size) > pool_end) {
        gprintk("mmap range 0x%lx-0x%lx outside DMA pool 0x%lx-0x%lx\n",
                phys_addr, phys_addr + size, pool_start, pool_end);
        return -EINVAL;
    }

#ifdef REMAP_DMA_NONCACHED
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#endif

    if (remap_pfn_range(vma,
                        vma->vm_start,
                        vma->vm_pgoff,
                        size,
                        vma->vm_page_prot)) {
        gprintk("Failed to mmap phys range 0x%lx-0x%lx to 0x%lx-0x%lx\n",
                phys_addr, phys_addr + size, vma->vm_start,vma->vm_end);
        return -EAGAIN;
    }

    return 0;
}
#endif /* USE_LINUX_BDE_MMAP */

/* Workaround for broken Busybox/PPC insmod */
static char _modname[] = LINUX_KERNEL_BDE_NAME;

static gmodule_t _gmodule = {
    name: LINUX_KERNEL_BDE_NAME,
    major: LINUX_KERNEL_BDE_MAJOR,
    init: _init,
    cleanup: _cleanup,
    pprint: _pprint,
#if USE_LINUX_BDE_MMAP
    mmap: _mmap,
#endif
};

gmodule_t *
gmodule_get(void)
{
    _gmodule.name = _modname;
    return &_gmodule;
}


/*
 * BDE Interface
 */

static const char *
_name(void)
{
    return LINUX_KERNEL_BDE_NAME;
}

static int
_num_devices(int type)
{
    switch (type) {
     case BDE_ALL_DEVICES:
        return _ndevices;
     case BDE_SWITCH_DEVICES:
        return _switch_ndevices;
     case BDE_ETHER_DEVICES:
        return _ether_ndevices;
     case BDE_CPU_DEVICES:
        return _cpu_ndevices;
    }

    return 0;
}

static const ibde_dev_t *
_get_dev(int d)
{
    if (!VALID_DEVICE(d)) {
        gprintk("_get_dev: Invalid device index %d\n", d);
        return NULL;
    }

    return &_devices[d].bde_dev;
}

static uint32
_get_dev_type(int d)
{
    if (!VALID_DEVICE(d)) {
        gprintk("_get_dev: Invalid device index %d\n", d);
        return 0;
    }

    return _devices[d].dev_type;
}

static uint32
_pci_conf_read(int d, uint32 addr)
{
#ifdef BCM_ICS
    return 0xFFFFFFFF;
#else
    uint32 rc = 0;

    if (!VALID_DEVICE(d)) {
        gprintk("_pci_conf_read: Invalid device index %d\n", d);
        return 0xFFFFFFFF;
    }

    if (!(_devices[d].dev_type & BDE_PCI_DEV_TYPE)) {
        gprintk("_pci_conf_read: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return 0xFFFFFFFF;
    }

    pci_read_config_dword(_devices[d].pci_device, addr, &rc);
    return rc;
#endif /* BCM_ICS */
}

static int
_pci_conf_write(int d, uint32 addr, uint32 data)
{
#ifdef BCM_ICS
    return -1;
#else
    if (!VALID_DEVICE(d)) {
        gprintk("_pci_conf_write: Invalid device index %d\n", d);
        return -1;
    }

    if (!(_devices[d].dev_type & BDE_PCI_DEV_TYPE)) {
        gprintk("_pci_conf_write: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return -1;
    }

    pci_write_config_dword(_devices[d].pci_device, addr, data);
    return 0;
#endif /* BCM_ICS */
}

/* Initialized when the bde is created */
static linux_bde_bus_t _bus;

static void
_pci_bus_features(int unit, int *be_pio, int *be_packet, int *be_other)
{
    if ((_devices[unit].bde_dev.device & 0xFF00) != 0x5600 &&
        (_devices[unit].bde_dev.device & 0xF000) != 0xc000 &&
        (_devices[unit].bde_dev.device & 0xF000) != 0xb000 &&
        (_devices[unit].bde_dev.device & 0xFFF0) != 0x0230 &&
        (_devices[unit].bde_dev.device & 0xFFF0) != 0xa440) {
        if (be_pio) *be_pio = 0;
        if (be_packet) *be_packet = 0;
        if (be_other) *be_other = 0;
    } else {
        if (be_pio) *be_pio = _bus.be_pio;
        if (be_packet) *be_packet = _bus.be_packet;
        if (be_other) *be_other = _bus.be_other;
    }
#if defined(BCM_METROCORE_LOCAL_BUS)
    if (_devices[unit].dev_type & BDE_EB_DEV_TYPE && be_pio) {
        *be_pio = 1;
    }
#endif

}

static uint32_t
_read(int d, uint32_t addr)
{
    unsigned long flags;
    volatile uint16  msb, lsb;
    uint32  sl_addr;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        return -1;
    }

    if (_devices[d].dev_type & BDE_DEV_BUS_RD_16BIT) {
        /*
         * Adjust the address presented to Eb slave. Move A15:A0 to A16:A1.
         */
        sl_addr = (addr & 0xffff0000) | ((addr & 0xffff) << 1);
        /* Disable interrupts */
        spin_lock_irqsave(&bus_lock, flags);

        lsb = *((uint16 *)(_devices[d].bde_dev.base_address + sl_addr));
        msb = *((uint16 *)(_devices[d].bde_dev.base_address + sl_addr));
        spin_unlock_irqrestore(&bus_lock, flags);

        return (msb << 16) | lsb;
    } else {
        return ((VOL uint32_t *)_devices[d].bde_dev.base_address)[addr / 4];
    }
}

static int
_write(int d, uint32_t addr, uint32_t data)
{
    unsigned long flags;
    uint32  sl_addr;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        return -1;
    }

    if (_devices[d].dev_type & BDE_DEV_BUS_WR_16BIT) {
        /*
         * Adjust the address presented to Eb slave. Move A15:A0 to A16:A1.
         */
        sl_addr = (addr & 0xffff0000) | ((addr & 0xffff) << 1);

        /* Disable interrupts */
        spin_lock_irqsave(&bus_lock, flags);

        *((VOL uint16 *)(_devices[d].bde_dev.base_address + sl_addr)) =
                                                             data & 0xffff;
        *((VOL uint16 *)(_devices[d].bde_dev.base_address + sl_addr)) =
                                                     (data >> 16) & 0xffff;
        spin_unlock_irqrestore(&bus_lock, flags);
    } else {
        ((VOL uint32_t *)_devices[d].bde_dev.base_address)[addr / 4] = data;
#ifdef KEYSTONE
        /* Enforce PCIe transaction ordering. Commit the write transaction */
        __asm__ __volatile__("sync");
#endif
    }
    return 0;

}

static uint32_t *
_salloc(int d, int size, const char *name)
{
    void *ptr;

    if (_dma_mem_size) {
        return mpool_alloc(_dma_pool, size);
    }
    if ((ptr = kmalloc(size, mem_flags)) == NULL) {
        ptr = _pgalloc(size);
    }
    return ptr;
}

static void
_sfree(int d, void *ptr)
{
    if (_dma_mem_size) {
        return mpool_free(_dma_pool, ptr);
    }
    if (_pgfree(ptr) < 0) {
        kfree(ptr);
    }
}

static int
_sinval(int d, void *ptr, int length)
{
#if defined(dma_cache_wback_inv)
     dma_cache_wback_inv((unsigned long)ptr, length);
#else
#if defined(IPROC_CMICD) || defined(BCM958525)
    
    dma_sync_single_for_cpu(NULL, (unsigned long)ptr, length, DMA_BIDIRECTIONAL);
#else
    dma_cache_sync(NULL, ptr, length, DMA_BIDIRECTIONAL);
#endif
#endif
    return 0;
}

static int
_sflush(int d, void *ptr, int length)
{
#if defined(dma_cache_wback_inv)
    dma_cache_wback_inv((unsigned long)ptr, length);
#else
#if defined(IPROC_CMICD) || defined(BCM958525)
    
    dma_sync_single_for_cpu(NULL, (unsigned long)ptr, length, DMA_BIDIRECTIONAL);
#else
    dma_cache_sync(NULL, ptr, length, DMA_BIDIRECTIONAL);
#endif
#endif

    return 0;
            

}

static _ISR_RET
_isr(_ISR_PARAMS(irq, dev_id, iregs))
{
    bde_ctrl_t *ctrl = (bde_ctrl_t *) dev_id;

    if (ctrl && ctrl->isr) {
        ctrl->isr(ctrl->isr_data);
    }
    if (ctrl && ctrl->isr2) {
        ctrl->isr2(ctrl->isr2_data);
    }
    return IRQ_HANDLED;
}

static int
_interrupt_connect(int d,
                   void (*isr)(void *),
                   void *isr_data)
{
    bde_ctrl_t *ctrl;
    unsigned long irq_flags;
    int isr2_dev;
    int isr_active;

    isr2_dev = d & LKBDE_ISR2_DEV;
    d &= ~LKBDE_ISR2_DEV;

    if (!VALID_DEVICE(d)) {
        gprintk("_interrupt_connect: Invalid device index %d\n", d);
        return -1;
    }
    if (debug >= 1) {
        gprintk("_interrupt_connect d %d\n", d);
    }
    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("_interrupt_connect: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return -1;
    }

    ctrl = _devices + d;

    isr_active = (ctrl->isr || ctrl->isr2) ? 1 : 0;

    if (isr2_dev) {
        if (debug >= 1) {
            gprintk("connect secondary isr\n");
        }
        ctrl->isr2_data = isr_data;
        ctrl->isr2 = isr;
        if (isr_active) {
            /* Main handler (_isr) already installed */
            return 0;
        }
    } else {
        if (debug >= 1) {
            gprintk("connect primary isr\n");
        }
        ctrl->isr = isr;
        ctrl->isr_data = isr_data;
        if (isr_active) {
            /* Main handler (_isr) already installed */
            return 0;
        }
    }

    if (ctrl->iLine != -1) {
        irq_flags = IRQF_SHARED;
#if defined(CONFIG_PCI_MSI)
        if (use_msi && pci_enable_msi(ctrl->pci_device) == 0) {
            irq_flags = 0;
            ctrl->iLine = ctrl->pci_device->irq;
        }
#endif
        if (request_irq(ctrl->iLine,
                       _isr,
                       irq_flags,
                       LINUX_KERNEL_BDE_NAME,
                       ctrl) < 0) {
            gprintk("could not request irq %d for device %d\n",
                    ctrl->pci_device->irq, d);

            ctrl->isr = NULL;
            ctrl->isr_data = NULL;
            ctrl->isr2 = NULL;
            ctrl->isr2_data = NULL;
#if defined(CONFIG_PCI_MSI)
            if (use_msi && irq_flags == 0) {
                pci_disable_msi(ctrl->pci_device);
            }
#endif
            return -1;
        }
    }

    return 0;
}

static int
_interrupt_disconnect(int d)
{
    bde_ctrl_t *ctrl;
    int isr2_dev;
    int isr_active;

    isr2_dev = d & LKBDE_ISR2_DEV;
    d &= ~LKBDE_ISR2_DEV;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (debug >= 1) {
        gprintk("_interrupt_disconnect d %d\n", d);
    }
    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("_interrupt_disconnect: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return -1;
    }

    ctrl = _devices + d;

    isr_active = (ctrl->isr || ctrl->isr2) ? 1 : 0;

    if (isr2_dev) {
        if (debug >= 1) {
            gprintk("disconnect secondary isr\n");
        }
        ctrl->isr2 = NULL;
        ctrl->isr2_data = NULL;
        ctrl->fmask = 0;
        if (ctrl->isr) {
            /* Primary handler still active */
            SYNC_IRQ(ctrl->iLine); 
            return 0;
        }
    } else {
        if (debug >= 1) {
            gprintk("disconnect primary isr\n");
        }
        ctrl->isr = NULL;
        ctrl->isr_data = NULL;
        if (ctrl->isr2) {
            /* Secondary handler still active */
            SYNC_IRQ(ctrl->iLine); 
            return 0;
        }
    }

    if (isr_active) {
        free_irq(ctrl->iLine, ctrl);
#if defined(CONFIG_PCI_MSI)
        if (use_msi) {
            pci_disable_msi(ctrl->pci_device);
        }
#endif
    }

    return 0;
}

static sal_paddr_t
_l2p(int d, void *vaddr)
{
    if (_dma_mem_size) {
        /* dma memory is a contiguous block */
        if (vaddr) {
            return _dma_pbase + (PTR_TO_UINTPTR(vaddr) - PTR_TO_UINTPTR(_dma_vbase));
        }
        return 0;
    }
    return virt_to_bus(vaddr);
}

static uint32_t *
_p2l(int d, sal_paddr_t paddr)
{
    if (_dma_mem_size) {
        /* dma memory is a contiguous block */
        return paddr ? (void *)_dma_vbase + (paddr - _dma_pbase) : NULL;
    }
    return bus_to_virt(paddr);
}

static uint32_t
_iproc_offset(uint32_t map_no, int d, uint32_t addr)
{
    _iproc_map_t *map;
    _iproc_subwin_t *subwin;
    int idx;
    volatile uint32_t *bar0, *pv;

    if (map_no > (sizeof(iproc_map)/sizeof(iproc_map[0]))) {
        return 0;
    }
    map = iproc_map[map_no];
    for (idx = 0; idx < IPROC_SUBWIN_MAX; idx++) {
        subwin = &map->subwin[idx];
        if (addr >= subwin->addr_min && addr <= subwin->addr_max) {
            return subwin->addr_min - (idx * 0x1000);
        }
    }

    /* Not found. (Re)use the default Subwindow */
    subwin = &map->subwin[IPROC_DEFAULT_SUBWIN];
    if (addr < subwin->addr_min || addr > subwin->addr_max) {
        subwin->addr_min = addr & ~(0xfff);
        subwin->addr_max = addr | 0xfff;

        bar0 = (volatile uint32_t *)_devices[d].bde_dev.base_address1;
        pv = &bar0[BAR0_PAXB_IMAP0_0/4];
        pv[IPROC_DEFAULT_SUBWIN] = subwin->addr_min | 0x1;

        /* Read back IMAP register to ensure the write completes before proceeding */
        if (pv[IPROC_DEFAULT_SUBWIN] != (subwin->addr_min | 0x1)) {
            return 0;
        }
    }
    return subwin->addr_min - (IPROC_DEFAULT_SUBWIN * 0x1000);
}

static uint32_t
_iproc_ihost_read(int d, uint32_t addr)
{
    uint32_t *mapaddr;
    uint32_t reg_val;

    if (!request_mem_region(addr, sizeof(uint32_t), "ihost-sdk")) {
        return -1;
    } else {
        mapaddr = IOREMAP(addr, sizeof(uint32_t));
        if (mapaddr == NULL) {
            return -1;
        }
        reg_val = readl(mapaddr);
        iounmap(mapaddr);
        release_mem_region(addr, sizeof(uint32_t));
        return reg_val;
    }
}

static int
_iproc_ihost_write(int d, uint32_t addr, uint32_t data)
{
    uint32_t *mapaddr;

    if (!request_mem_region(addr, sizeof(uint32_t), "ihost-sdk")) {
        return -1;
    } else {
        mapaddr = IOREMAP(addr, sizeof(uint32_t));
        if (mapaddr == NULL) {
            return -1;
        }
        writel(data, mapaddr);
        iounmap(mapaddr);
        release_mem_region(addr, sizeof(uint32_t));
        return 0;
    }
}

static uint32_t
_iproc_read(int d, uint32_t addr)
{
    uint32_t offset;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        return -1;
    }

    if (_devices[d].dev_type & BDE_AXI_DEV_TYPE) {
        return _iproc_ihost_read(d, addr);
    }

    if ((offset = _iproc_offset(0, d, addr)) == 0) {
        return -1;
    }
    addr -= offset;

    return ((VOL uint32_t *)_devices[d].bde_dev.base_address1)[addr / 4];
}

static int
_iproc_write(int d, uint32_t addr, uint32_t data)
{
    uint32_t offset;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        return -1;
    }

    if (_devices[d].dev_type & BDE_AXI_DEV_TYPE) {
        return _iproc_ihost_write(d, addr, data);
    }

    if ((offset = _iproc_offset(0, d, addr)) == 0) {
        return -1;
    }
    addr -= offset;

    ((VOL uint32_t *)_devices[d].bde_dev.base_address1)[addr / 4] = data;

    return 0;

}

#ifdef BCM_ROBO_SUPPORT
#define SOC_ROBO_PAGE_BP        8    /* for Robo Chip only */

#if defined(IPROC_CMICD)
extern int ccb_mii_read(int dev_type, int phy_addr, int reg_off, uint16 *data);
extern int ccb_mii_write(int dev_type, int phy_addr, int reg_off, uint16 data);

/* device type */
#define MII_DEV_LOCAL 0
#define MII_DEV_EXT   1
#endif

static int
_spi_read(int d, uint32 addr, uint8 *buf, int len)
{
#if defined(KEYSTONE) || defined(IPROC_CMICD)
    bde_ctrl_t *ctrl;
    uint8 page, offset;
#endif
#if defined(IPROC_CMICD)
    int rv = 0;
    uint16 value = 0;
#endif

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(_devices[d].dev_type & BDE_SPI_DEV_TYPE)) {
        gprintk("_spi_read: Not SPI device %d, type %x\n",
                d, _devices[d].dev_type);
        return -1;
    }

#if defined(KEYSTONE) || defined(IPROC_CMICD)
    ctrl = _devices + d;
#endif

#if defined(IPROC_CMICD)
    if (addr & SOC_EXTERNAL_PHY_BUS_CPUMDIO) {
        rv = ccb_mii_read(MII_DEV_EXT, (addr >> 8) & 0xff, addr & 0xff, &value);
        memcpy(buf, &value, 2);
        return rv;
    }
#endif

#if defined(KEYSTONE) || defined(IPROC_CMICD)
    page = (addr >> SOC_ROBO_PAGE_BP) & 0xFF;
    offset = addr & 0xFF;
#endif

    ROBO_RREG(ctrl->spi_device->robo, ctrl->spi_device->cid,
              page, offset, buf, (uint)len);

    return 0;
}

static int
_spi_write(int d, uint32 addr, uint8 *buf, int len)
{
#if defined(KEYSTONE) || defined(IPROC_CMICD)
    bde_ctrl_t *ctrl;
    uint8 page, offset;
#endif
#if defined(IPROC_CMICD)
    int rv = 0;
    uint16 value = 0;
#endif
    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(_devices[d].dev_type & BDE_SPI_DEV_TYPE)) {
        gprintk("_spi_write: Not SPI device %d, type %x\n",
                d, _devices[d].dev_type);
        return -1;
    }

#if defined(KEYSTONE) || defined(IPROC_CMICD)
    ctrl = _devices + d;
#endif

#if defined(IPROC_CMICD)
    if (addr & SOC_EXTERNAL_PHY_BUS_CPUMDIO) {
        memcpy(&value, buf, 2);
        rv = ccb_mii_write(MII_DEV_EXT, (addr >> 8) & 0xff, addr & 0xff, value);
        return rv;
    }
#endif

#if defined(KEYSTONE) || defined(IPROC_CMICD)
    page = (addr >> SOC_ROBO_PAGE_BP) & 0xFF;
    offset = addr & 0xFF;
#endif

    ROBO_WREG(ctrl->spi_device->robo, ctrl->spi_device->cid,
              page, offset, buf, (uint)len);

    return 0;
}

#endif

#if (defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT))
int
lkbde_cpu_write(int d, uint32 addr, uint32 *buf)
{
    bde_ctrl_t* ctrl;
    void *full_addr;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    ctrl = &_devices[d];

    full_addr   = ctrl->cpu_address + addr;

    *((uint32_t*)full_addr) = *buf;

    return 0;
}

int
lkbde_cpu_read(int d, uint32 addr, uint32 *buf)
{
     bde_ctrl_t* ctrl;
    void *full_addr;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    ctrl = &_devices[d];

    full_addr   = ctrl->cpu_address + addr;

    *buf = *((uint32_t*)full_addr);
    return 0;
}

int
lkbde_cpu_pci_register(int d)
{
    bde_ctrl_t* ctrl;
    uint16  cmd = 0;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    ctrl = &_devices[d];

    /* enable device */
    if (pci_enable_device(ctrl->pci_device)) {
        gprintk("Cannot enable pci device : vendor_id = %x, device_id = %x\n",
                ctrl->pci_device->vendor, ctrl->pci_device->device);
    }
    
    /* Add PCI_COMMAND_MEMORY and PCI_COMMAND_MASTER */
    pci_read_config_word(ctrl->pci_device, PCI_COMMAND, &cmd);
    if (!(cmd & PCI_COMMAND_MEMORY) || !(cmd & PCI_COMMAND_MASTER)) {
        cmd |= PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
        pci_write_config_word(ctrl->pci_device, PCI_COMMAND, cmd);
    }

    switch (ctrl->bde_dev.device) {
    case PETRAB_DEVICE_ID:
        /* Fix bar 0 address */ 
        pci_write_config_byte(ctrl->pci_device, 0x13, 0x60);

        /* Fix Max payload size */
        pci_write_config_byte(ctrl->pci_device, 0x88, 0x2f);
        pci_write_config_byte(ctrl->pci_device, 0x89, 0x10);
        break;
    case BCM88750_DEVICE_ID:
    case BCM88755_DEVICE_ID:
    case BCM88950_DEVICE_ID:
    case ACP_PCI_DEVICE_ID:
    case BCM88650_DEVICE_ID:
    case BCM88670_DEVICE_ID:
    case BCM88350_DEVICE_ID:
    case BCM88351_DEVICE_ID:
    case BCM88450_DEVICE_ID:      
    case BCM88451_DEVICE_ID:
    case BCM88550_DEVICE_ID:
    case BCM88551_DEVICE_ID:
    case BCM88552_DEVICE_ID:
    case BCM88651_DEVICE_ID:
    case BCM88654_DEVICE_ID: 
    case BCM88660_DEVICE_ID:      
    case BCM88360_DEVICE_ID:
    case BCM88361_DEVICE_ID:
    case BCM88363_DEVICE_ID:
    case BCM88460_DEVICE_ID:
    case BCM88461_DEVICE_ID:
    case BCM88560_DEVICE_ID:
    case BCM88561_DEVICE_ID:
    case BCM88562_DEVICE_ID:
    case BCM88661_DEVICE_ID:
    case BCM88664_DEVICE_ID: 
        /* Fix bar 0 address */ 
        pci_write_config_byte(ctrl->pci_device, 0x12, 0x10);
        pci_write_config_byte(ctrl->pci_device, 0x13, 0x60);

        /*
         * For DMA transactions - set Max_Payload_Size and
         * Max_Read_Request_Size to 128 bytes.
         */
        pci_write_config_byte(ctrl->pci_device, 0xb5, 0x0c);
        pci_write_config_byte(ctrl->pci_device, 0xb4, 0x0);
        break;
    default:
        break;
    }

    /* Redo ioremap */
    if (ctrl->bde_dev.base_address) {
        iounmap((void *)ctrl->bde_dev.base_address);
    }
    ctrl->bde_dev.base_address = (sal_vaddr_t)IOREMAP(ctrl->phys_address, 0x1000000);

    if (debug >= 1) {
        gprintk("%s, %s(): info:\n", __FILE__, __FUNCTION__);
        gprintk("_ndevices=%d, _switch_ndevices=%d\n",
                _ndevices, _switch_ndevices);
        gprintk("ctrl->dev_type=0x%x, ctrl->phys_address=0x%lx, "
                "ctrl->cpu_address=%p\n",
                ctrl->dev_type, (unsigned long)ctrl->phys_address,
                ctrl->cpu_address);
        gprintk("ctrl->bde_dev.device=0x%x, ctrl->bde_dev.rev=0x%x, "
                "ctrl->bde_dev.base_address=0x%lx\n",
                ctrl->bde_dev.device, ctrl->bde_dev.rev,
                (unsigned long)ctrl->bde_dev.base_address);
    }

    return 0;
}

/* 
 * Export Low level access function - currently for PCP DMA Kernel module. 
 */
int
lkbde_mem_write(int d, uint32 addr, uint32 *buf)
{
    bde_ctrl_t* ctrl;
    void *full_addr;

    if (!VALID_DEVICE(d)) return -1;
    ctrl = &_devices[d];
    
    full_addr   = (void *)ctrl->bde_dev.base_address + addr;
   *((uint32_t*)full_addr) = *buf;
    return 0;
}
LKM_EXPORT_SYM(lkbde_mem_write);

int
lkbde_mem_read(int d, uint32 addr, uint32 *buf)
{
    bde_ctrl_t* ctrl;
    void *full_addr;

    if (!VALID_DEVICE(d)) return -1;
    ctrl = &_devices[d];

    full_addr   = (void *)ctrl->bde_dev.base_address + addr;
    *buf = *((uint32_t*)full_addr);
    return 0;
}
LKM_EXPORT_SYM(lkbde_mem_read);
#endif /* defined(BCM_PETRA_SUPPORT) */

static ibde_t _ibde = {
    name: _name,
    num_devices: _num_devices,
    get_dev: _get_dev,
    get_dev_type: _get_dev_type,
    pci_conf_read: _pci_conf_read,
    pci_conf_write: _pci_conf_write,
    pci_bus_features: _pci_bus_features,
    read: _read,
    write: _write,
    salloc: _salloc,
    sfree: _sfree,
    sinval: _sinval,
    sflush: _sflush,
    interrupt_connect: _interrupt_connect,
    interrupt_disconnect: _interrupt_disconnect,
    l2p: _l2p,
    p2l: _p2l,
#if defined(BCM_ROBO_SUPPORT)
    spi_read: _spi_read,
    spi_write: _spi_write,
#else
    NULL,
    NULL,
#endif /* defined(BCM_ROBO_SUPPORT) */
    iproc_read: _iproc_read,
    iproc_write: _iproc_write,
};

/*
 * Function: linux_bde_create
 *
 * Purpose:
 *    Creator function for this BDE interface.
 * Parameters:
 *    bus - pointer to the bus features structure you want this
 *          bde to export. Depends on the system.
 *    ibde - pointer to a location to recieve the bde interface pointer.
 * Returns:
 *    0 on success
 *    -1 on failure.
 * Notes:
 *    This is the main BDE create function for this interface.
 *    Used by the external system initialization code.
 */
int
linux_bde_create(linux_bde_bus_t *bus, ibde_t **ibde)
{

    memset(&_bus, 0, sizeof(_bus));

    if (bus) {
        _bus = *bus;
    }
#ifdef NONCOHERENT_DMA_MEMORY
#ifdef REMAP_DMA_NONCACHED
    /*
     * If we have a non-cached DMA memory pool
     * there is no need to flush and invalidate.
     */
    if (_dma_vbase != NULL) {
        _ibde.sinval = NULL;
        _ibde.sflush = NULL;
    }
#endif
#else
    _ibde.sinval = NULL;
    _ibde.sflush = NULL;
#endif
    *ibde = &_ibde;
    return 0;
}

/*
 * Function: linux_bde_destroy
 *
 * Purpose:
 *    destroy this bde
 * Parameters:
 *    BDE interface pointer
 * Returns:
 *    0 on success, < 0 on error.
 */
int
linux_bde_destroy(ibde_t *ibde)
{
    /* nothing */
    return 0;
}

/*
 *  Backdoors provided by the kernel bde
 *
 */


/*
 * Some of the driver malloc's are too large for
 * kmalloc(), so 'sal_alloc' and 'sal_free' in the
 * linux kernel sal cannot be implemented with kmalloc().
 *
 * Instead, they expect someone to provide an allocator
 * that can handle the gimongous size of some of the
 * allocations, and we provide it here, by allocating
 * this memory out of the boot-time dma pool.
 *
 * These are the functions in question:
 */

void* kmalloc_giant(int sz)
{
    return mpool_alloc(_dma_pool, sz);
}

void kfree_giant(void* ptr)
{
    return mpool_free(_dma_pool, ptr);
}

/*
 *  Backdoors provided by the kernel bde
 */

uint32_t
lkbde_get_dev_phys(int d)
{
    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("lkbde_get_dev_phys: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return 0;
    }
    return _devices[d].phys_address;
}

uint32_t
lkbde_get_dev_phys_hi(int d)
{
    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("lkbde_get_dev_phys: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return 0;
    }
#ifdef PHYS_ADDR_IS_64BIT
    return (uint32_t)(_devices[d].phys_address >> 32);
#else
    return 0;
#endif
}

void *
lkbde_get_dev_virt(int d)
{
    if (!VALID_DEVICE(d)) {
        return NULL;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("lkbde_get_dev_virt: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return 0;
    }

    if (_devices[d].alt_base_addr) {
        return (void *)_devices[d].alt_base_addr;
    }

    return (void *)_devices[d].bde_dev.base_address;
}

int
lkbde_get_dev_resource(int d, int rsrc, uint32_t *flags,
                       uint32_t *phys_lo, uint32_t *phys_hi)
{
    if (!VALID_DEVICE(d)) {
        return -1;
    }

    *flags = 0;
    *phys_lo = 0;
    *phys_hi = 0;

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("lkbde_get_dev_phys: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return 0;
    }

    switch (rsrc) {
    case 0:
        *phys_lo = (uint32_t)(_devices[d].phys_address);
#ifdef PHYS_ADDR_IS_64BIT
        *phys_hi = (uint32_t)(_devices[d].phys_address >> 32);
#endif
        break;
    case 1:
        *phys_lo = (uint32_t)(_devices[d].phys_address1);
#ifdef PHYS_ADDR_IS_64BIT
        *phys_hi = (uint32_t)(_devices[d].phys_address1 >> 32);
#endif
        break;
    default:
        break;
    }

    return 0;
}

int
lkbde_get_dma_info(uint32_t *pbase, uint32_t *size)
{
    if (_dma_pbase == 0) {
        if (_dma_mem_size == 0) {
            _dma_mem_size = DMA_MEM_DEFAULT;
        }
        _alloc_mpool(_dma_mem_size);
    }
    *pbase = _dma_pbase;
    *size = _dma_mem_size;
    return 0;
}

void *
lkbde_get_dma_dev(int d)
{
    if (!VALID_DEVICE(d)) {
        return NULL;
    }

#ifdef LINUX_BDE_DMA_DEVICE_SUPPORT
    return (void *)_devices[d].dma_dev;
#else
    return (void *)_devices[d].pci_device;
#endif
}

void *
lkbde_get_hw_dev(int d)
{
    if (!VALID_DEVICE(d)) {
        return NULL;
    }

    return (void *)_devices[d].pci_device;
}

/*
 * When a secondary interrupt handler is installed this function
 * is used for synchronizing hardware access to the IRQ mask
 * register. The secondary driver will supply a non-zero fmask
 * (filter mask) to indicate which interrupt bits it controls.
 * The fmask is ignored for the primary driver.
 */
int
lkbde_irq_mask_set(int d, uint32_t addr, uint32_t mask, uint32_t fmask)
{
    bde_ctrl_t *ctrl;
    int isr2_dev;
    unsigned long flags;

    isr2_dev = d & LKBDE_ISR2_DEV;
    d &= ~LKBDE_ISR2_DEV;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    ctrl = _devices + d;

    /* Lock is required to synchronize access from user space */
    spin_lock_irqsave(&ctrl->lock, flags);

    if (isr2_dev) {
        /* This is the secondary interrupt handler */
        ctrl->fmask = fmask;
        ctrl->imask2 = mask & ctrl->fmask;
    } else {
        /* This is the primary interrupt handler */
        ctrl->imask = mask & ~ctrl->fmask;
    }
    _write(d, addr, ctrl->imask | ctrl->imask2);

    spin_unlock_irqrestore(&ctrl->lock, flags);

    return 0;
}

/*
 * When a secondary interrupt handler is installed, this function
 * is used to avoid activating the user mode interrupt handler 
 * thread if all pending interrupts are handled in kernel space.
 *
 * The mask returned is the mask of interrupts that should be 
 * handled by the primary handler, i.e. doing a logical AND of
 * the current interrupt status and this mask will tell you if
 * the user mode interrupt handler needs to be invoked.
 *
 * Note that if no secondary handler is installed, the mask will
 * be set to zero, and hence there will be no need to read the
 * current interrupt status from hardware (from kernel space).
 */
int
lkbde_irq_imask_get(int d, uint32_t *mask)
{
    bde_ctrl_t *ctrl;

    d &= ~LKBDE_ISR2_DEV;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (mask == NULL) {
        return -1;
    }

    ctrl = _devices + d;

    *mask = 0;
    if (ctrl->fmask) {
        *mask = ctrl->imask;
    }
    return 0;
}


/*
 * Export functions
 */
LKM_EXPORT_SYM(linux_bde_create);
LKM_EXPORT_SYM(linux_bde_destroy);
LKM_EXPORT_SYM(kmalloc_giant);
LKM_EXPORT_SYM(kfree_giant);
LKM_EXPORT_SYM(lkbde_get_dev_phys);
LKM_EXPORT_SYM(lkbde_get_dev_virt);
LKM_EXPORT_SYM(lkbde_get_dev_resource);
LKM_EXPORT_SYM(lkbde_get_dma_info);
LKM_EXPORT_SYM(lkbde_get_hw_dev);
LKM_EXPORT_SYM(lkbde_get_dma_dev);
LKM_EXPORT_SYM(lkbde_irq_mask_set);
LKM_EXPORT_SYM(lkbde_irq_imask_get);
LKM_EXPORT_SYM(lkbde_get_dev_phys_hi);
#if (defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT))
LKM_EXPORT_SYM(lkbde_cpu_write);
LKM_EXPORT_SYM(lkbde_cpu_read);
LKM_EXPORT_SYM(lkbde_cpu_pci_register);
#endif
