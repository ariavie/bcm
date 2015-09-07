/*
 * $Id: linux-user-bde.c 1.81.6.1 Broadcom SDK $
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
 * Linux User BDE Helper Module
 */
#include <gmodule.h>
#include <mpool.h>
#include <linux-bde.h>

#include <sal/core/thread.h>
#include <sal/core/sync.h>
#include <soc/devids.h>

#include "linux-user-bde.h"

#ifdef KEYSTONE
#include <shared/et/bcmdevs.h>
#endif


MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("User BDE Helper Module");
MODULE_LICENSE("Proprietary");

/* CMIC/CMICe defines */
#define CMIC_IRQ_STAT                   0x00000144
#define CMIC_IRQ_MASK                   0x00000148
#define CMIC_IRQ_MASK_1                 0x0000006C
#define CMIC_IRQ_MASK_2                 0x00000070

/* CMICm defines */
#define CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(x)           	(0x31414 + (0x1000 * x))
#define CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(x)           	(0x31418 + (0x1000 * x))
#define CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(x)           	(0x3141c + (0x1000 * x))
#define CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(x)           	(0x31420 + (0x1000 * x))
#define CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(x)           	(0x31424 + (0x1000 * x))

#define CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(x)           	(0x31428 + (0x1000 * x))
#define CMIC_CMCx_UC0_IRQ_MASK1_OFFSET(x)           	(0x3142c + (0x1000 * x))
#define CMIC_CMCx_UC0_IRQ_MASK2_OFFSET(x)           	(0x31430 + (0x1000 * x))
#define CMIC_CMCx_UC0_IRQ_MASK3_OFFSET(x)           	(0x31434 + (0x1000 * x))
#define CMIC_CMCx_UC0_IRQ_MASK4_OFFSET(x)           	(0x31438 + (0x1000 * x))

/* Allow override of default CMICm CMC */
#ifndef BDE_CMICM_PCIE_CMC
#define BDE_CMICM_PCIE_CMC              0
#endif

/* Defines used to distinguish CMICe from CMICm */
#define CMICE_DEV_REV_ID                (0x178 / sizeof(uint32))

static ibde_t *user_bde = NULL;

typedef void (*isr_f)(void *);

typedef struct bde_ctrl_s {
    uint32 dev_type;
    int irq;
    int enabled;
    int devid;
    isr_f isr;
    uint32 *ba;
} bde_ctrl_t;

static bde_ctrl_t _devices[LINUX_BDE_MAX_DEVICES];

static wait_queue_head_t _interrupt_wq;
static atomic_t _interrupt_has_taken_place = ATOMIC_INIT(0);

static wait_queue_head_t _ether_interrupt_wq;
static atomic_t _ether_interrupt_has_taken_place = ATOMIC_INIT(0);


#ifdef KEYSTONE
/*
 * Enforce PCIE transaction ordering. Commit the write transaction.
 */

#define SSOC_WRITEL(val, addr)                  \
            do {                                \
                writel((val), (addr));          \
                __asm__ __volatile__("sync");   \
            } while(0)

#else

#define SSOC_WRITEL(val, addr) \
            writel((val), (addr))

#endif
/*
 * Function: _interrupt
 *
 * Purpose:
 *    Interrupt Handler.
 *    Mask all interrupts on device and wake up interrupt
 *    thread. It is assumed that the interrupt thread unmasks
 *    interrupts again when interrupt handling is complete.
 * Parameters:
 *    ctrl - BDE control structure for this device.
 * Returns:
 *    Nothing
 */
static void 
_cmic_interrupt(bde_ctrl_t *ctrl)
{
    int d;
    uint32_t imask, stat;

    d = (((uint8 *)ctrl - (uint8 *)_devices) / sizeof (bde_ctrl_t));

    /* Check for secondary interrupt handler */
    if (lkbde_irq_imask_get(d, &imask) < 0) {
        imask = 0;
    }

    if (imask != 0) {
        /* Check for pending user mode interrupts */
        stat = user_bde->read(d, CMIC_IRQ_STAT);
        if ((stat & imask) == 0) {
            /* All handled in kernel mode */
            lkbde_irq_mask_set(d, CMIC_IRQ_MASK, imask, 0);
            return;
        }
    }

    lkbde_irq_mask_set(d, CMIC_IRQ_MASK, 0, 0);

    atomic_set(&_interrupt_has_taken_place, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&_interrupt_wq);
#else
    wake_up_interruptible(&_interrupt_wq);
#endif
}

static void 
_cmicm_interrupt(bde_ctrl_t *ctrl)
{
    int d;
    int cmc = BDE_CMICM_PCIE_CMC;

    d = (((uint8 *)ctrl - (uint8 *)_devices) / sizeof (bde_ctrl_t));

    if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
        lkbde_irq_mask_set(d, CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(cmc), 0, 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK1_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK2_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK3_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK4_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(1), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(2), 0);
    }
    else {
        lkbde_irq_mask_set(d, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(cmc), 0, 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(1), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(2), 0);
    }
    atomic_set(&_interrupt_has_taken_place, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&_interrupt_wq);
#else
    wake_up_interruptible(&_interrupt_wq);
#endif
}

static void 
_bcm88750_interrupt(bde_ctrl_t *ctrl)
{
    int d;

    d = (((uint8 *)ctrl - (uint8 *)_devices) / sizeof (bde_ctrl_t));

    lkbde_irq_mask_set(d, CMIC_IRQ_MASK, 0, 0);

    printk("%s(): Mask CMIC_IRQ_MASK_1, CMIC_IRQ_MASK_2\n", __FUNCTION__);
    lkbde_irq_mask_set(d, CMIC_IRQ_MASK_1, 0, 0); 
    lkbde_irq_mask_set(d, CMIC_IRQ_MASK_2, 0, 0);
    atomic_set(&_interrupt_has_taken_place, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&_interrupt_wq);
#else
    wake_up_interruptible(&_interrupt_wq);
#endif
}

static void
_qe2k_interrupt(bde_ctrl_t *ctrl)
{
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x20/sizeof(uint32));

    atomic_set(&_interrupt_has_taken_place, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&_interrupt_wq);
#else
    wake_up_interruptible(&_interrupt_wq);
#endif
}

static void
_fe2k_interrupt(bde_ctrl_t *ctrl)
{
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x18/sizeof(uint32)); /* PC_INTERRUPT_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x24/sizeof(uint32)); /* PC_ERROR0_MASK    */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x2c/sizeof(uint32)); /* PC_ERROR1_MASK    */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x34/sizeof(uint32)); /* PC_UNIT_MASK      */

    atomic_set(&_interrupt_has_taken_place, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&_interrupt_wq);
#else
    wake_up_interruptible(&_interrupt_wq);
#endif
}

static void
_fe2kxt_interrupt(bde_ctrl_t *ctrl)
{
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x2c/sizeof(uint32)); /* PC_INTERRUPT_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x38/sizeof(uint32)); /* PC_ERROR0_MASK    */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x40/sizeof(uint32)); /* PC_ERROR1_MASK    */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x50/sizeof(uint32)); /* PC_UNIT_MASK      */

    atomic_set(&_interrupt_has_taken_place, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&_interrupt_wq);
#else
    wake_up_interruptible(&_interrupt_wq);
#endif
}

static void
_bme3200_interrupt(bde_ctrl_t *ctrl)
{
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x54/sizeof(uint32)); /* PI_PT_ERROR0 */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x5c/sizeof(uint32)); /* PI_PT_ERROR1 */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x64/sizeof(uint32)); /* PI_PT_ERROR2 */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x6c/sizeof(uint32)); /* PI_PT_ERROR3 */

    atomic_set(&_interrupt_has_taken_place, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&_interrupt_wq);
#else
    wake_up_interruptible(&_interrupt_wq);
#endif
}


static void
_bm9600_interrupt(bde_ctrl_t *ctrl)
{
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x5c/sizeof(uint32));  /* PI_INTERRUPT_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0xc/sizeof(uint32));   /* PI_UNIT_INTERRUPT0_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x14/sizeof(uint32));  /* PI_UNIT_INTERRUPT1_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x1c/sizeof(uint32));  /* PI_UNIT_INTERRUPT2_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x24/sizeof(uint32));  /* PI_UNIT_INTERRUPT3_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x2c/sizeof(uint32));  /* PI_UNIT_INTERRUPT4_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x34/sizeof(uint32));  /* PI_UNIT_INTERRUPT5_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x3c/sizeof(uint32));  /* PI_UNIT_INTERRUPT6_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x44/sizeof(uint32));  /* PI_UNIT_INTERRUPT7_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x4c/sizeof(uint32));  /* PI_UNIT_INTERRUPT8_MASK */
    SSOC_WRITEL(0xffffffff, ctrl->ba + 0x54/sizeof(uint32));  /* PI_UNIT_INTERRUPT9_MASK */

    atomic_set(&_interrupt_has_taken_place, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&_interrupt_wq);
#else
    wake_up_interruptible(&_interrupt_wq);
#endif
}



/* The actual interrupt handler of ethernet devices */
static void 
_ether_interrupt(bde_ctrl_t *ctrl)
{
#ifdef KEYSTONE
    /* 
        * Since the two GMAC cores are sharing the same IRQ.
        * Add the checking to handle the interrupt events.
        */
    if ((ctrl->devid == BCM53000_GMAC_ID)) {
        if ((readl(ctrl->ba + 0x020/4) & readl(ctrl->ba + 0x024/4)) == 0) {
            return;
        }
    }
#endif    
    SSOC_WRITEL(0, ctrl->ba + 0x024/4);

    atomic_set(&_ether_interrupt_has_taken_place, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&_ether_interrupt_wq);
#else
    wake_up_interruptible(&_ether_interrupt_wq);
#endif
}


static struct _intr_mode_s {
    isr_f isr;
    const char *name;
} _intr_mode[] = {
    { (isr_f)_cmic_interrupt,       "CMIC/CMICe" },
    { (isr_f)_cmicm_interrupt,      "CMICm" },
    { (isr_f)_qe2k_interrupt,       "QE2K" },
    { (isr_f)_fe2k_interrupt,       "FE2K" },
    { (isr_f)_fe2kxt_interrupt,     "FE2KXT" },
    { (isr_f)_bme3200_interrupt,    "BME3200" },
    { (isr_f)_bm9600_interrupt,     "BM9600" },
    { (isr_f)_bcm88750_interrupt,   "BCM88750" },
    { NULL, NULL }
};

static const char *
_intr_mode_str(void *isr)
{
    int imx;

    imx = 0;
    while (_intr_mode[imx].isr != NULL) {
        if (isr == _intr_mode[imx].isr) {
            return _intr_mode[imx].name;
        }
        imx++;
    }
    return NULL;
}

/*
 * Function: _init
 *
 * Purpose:
 *    Module initialization.
 *    Attaches to kernel BDE.
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
static int
_init(void)
{
    int i;

    /* Connect to the kernel bde */
    if ((linux_bde_create(NULL, &user_bde) < 0) || user_bde == NULL) {
        return -ENODEV;
    }

    init_waitqueue_head(&_interrupt_wq);
    init_waitqueue_head(&_ether_interrupt_wq);

    memset(_devices, 0, sizeof(_devices));

    for (i = 0; i < user_bde->num_devices(BDE_ALL_DEVICES); i++) {
        /* Initialize our control info */
        _devices[i].dev_type = user_bde->get_dev_type(i);
        _devices[i].devid = user_bde->get_dev(i)->device;
        if (BDE_DEV_MEM_MAPPED(_devices[i].dev_type)) {
            _devices[i].enabled = 0;
            _devices[i].ba = lkbde_get_dev_virt(i);
        }
        if (_devices[i].dev_type & BDE_SWITCH_DEV_TYPE) {
            switch (user_bde->get_dev(i)->device) {
            case QE2000_DEVICE_ID:
                _devices[i].isr = (isr_f)_qe2k_interrupt;
                break;
            case BCM88020_DEVICE_ID:
                _devices[i].isr = (isr_f)_fe2k_interrupt;
                break;
            case BCM88025_DEVICE_ID:
                _devices[i].isr = (isr_f)_fe2kxt_interrupt;
                break;
            case BME3200_DEVICE_ID:
                _devices[i].isr = (isr_f)_bme3200_interrupt;
                break;
            case BM9600_DEVICE_ID:
                _devices[i].isr = (isr_f)_bm9600_interrupt;
                break;

            case BCM88750_DEVICE_ID:
            case BCM88754_DEVICE_ID:
            case BCM88755_DEVICE_ID:

                _devices[i].isr = (isr_f)_bcm88750_interrupt;
                break;
            default:
                _devices[i].isr = (isr_f)_cmic_interrupt;
                if ((_devices[i].dev_type & BDE_256K_REG_SPACE) &&
                    readl(_devices[i].ba + CMICE_DEV_REV_ID) == 0) {
                    _devices[i].isr = (isr_f)_cmicm_interrupt;
                }
                break;
            }
            if (_intr_mode_str(_devices[i].isr) == NULL) {
                gprintk("Warning: Unknown interrupt mode\n");
            }
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

    if (user_bde) {
        for (i = 0; i < user_bde->num_devices(BDE_ALL_DEVICES); i++) {
            if (_devices[i].enabled &&
                BDE_DEV_MEM_MAPPED(_devices[i].dev_type)) {
                user_bde->interrupt_disconnect(i);
            }
        }
        linux_bde_destroy(user_bde);
        user_bde = NULL;
    }
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
    int idx;
    const char *name;

    pprintf("Broadcom Device Enumerator (%s)\n", LINUX_USER_BDE_NAME);
    for (idx = 0; idx < user_bde->num_devices(BDE_ALL_DEVICES); idx++) {
        name = _intr_mode_str(_devices[idx].isr);
        if (name == NULL) {
            name = "unknown";
        }
        pprintf("\t%d: Interrupt mode %s\n", idx, name);
    }
    return 0;
}

/*
 * Function: _ioctl
 *
 * Purpose:
 *    Handle IOCTL commands from user mode.
 * Parameters:
 *    cmd - IOCTL cmd
 *    arg - IOCTL parameters
 * Returns:
 *    0 on success, <0 on error
 */
static int 
_ioctl(unsigned int cmd, unsigned long arg)
{
    lubde_ioctl_t io;
    uint32 pbase, size;
    const ibde_dev_t *bde_dev;

    if (copy_from_user(&io, (void *)arg, sizeof(io))) {
        return -EFAULT;
    }
  
    io.rc = LUBDE_SUCCESS;
  
    switch(cmd) {
    case LUBDE_VERSION:
        io.d0 = 0;
        break;
    case LUBDE_GET_NUM_DEVICES:
        io.d0 = user_bde->num_devices(io.dev);
        break;
    case LUBDE_GET_DEVICE:
        bde_dev = user_bde->get_dev(io.dev);
        if (bde_dev) {
            io.d0 = bde_dev->device;
            io.d1 = bde_dev->rev;
            if (BDE_DEV_MEM_MAPPED(_devices[io.dev].dev_type)) {
                /* Get physical address to map */
                io.d2 = lkbde_get_dev_phys(io.dev);
                io.d3 = lkbde_get_dev_phys_hi(io.dev);
            }
        } else {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_GET_DEVICE_TYPE:
        io.d0 = _devices[io.dev].dev_type;
        break;
    case LUBDE_GET_BUS_FEATURES:
        user_bde->pci_bus_features(io.dev, (int *) &io.d0, (int *) &io.d1,
                                   (int *) &io.d2);
        break;
    case LUBDE_PCI_CONFIG_PUT32:
        if (_devices[io.dev].dev_type & BDE_PCI_DEV_TYPE) {
            user_bde->pci_conf_write(io.dev, io.d0, io.d1);
        } else {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_PCI_CONFIG_GET32:
        if (_devices[io.dev].dev_type & BDE_PCI_DEV_TYPE) {
            io.d0 = user_bde->pci_conf_read(io.dev, io.d0);
        } else {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_GET_DMA_INFO:
        lkbde_get_dma_info(&pbase, &size);
        io.d0 = pbase;
        io.d1 = size; 
        io.d2 = 0;
        break;
    case LUBDE_ENABLE_INTERRUPTS:
        if (_devices[io.dev].dev_type & BDE_SWITCH_DEV_TYPE) {
            if (_devices[io.dev].isr && !_devices[io.dev].enabled) {
                user_bde->interrupt_connect(io.dev,
                                            _devices[io.dev].isr,
                                            _devices+io.dev);
                _devices[io.dev].enabled = 1;
            }
        } else {
            /* Process ethernet device interrupt */
            
            if (!_devices[io.dev].enabled) {
                user_bde->interrupt_connect(io.dev,
                                            (void(*)(void *))_ether_interrupt, 
                                            _devices+io.dev);
                _devices[io.dev].enabled = 1;
            }
        }
        break;
    case LUBDE_DISABLE_INTERRUPTS:
        if (_devices[io.dev].enabled) {
            user_bde->interrupt_disconnect(io.dev);
            _devices[io.dev].enabled = 0;
        }
        break;
    case LUBDE_WAIT_FOR_INTERRUPT:
        if (_devices[io.dev].dev_type & BDE_SWITCH_DEV_TYPE) {
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
            wait_event_timeout(_interrupt_wq, 
                               atomic_read(&_interrupt_has_taken_place) != 0, 100);
#else
            wait_event_interruptible(_interrupt_wq, 
                                     atomic_read(&_interrupt_has_taken_place) != 0);
#endif
            /* 
             * Even if we get multiple interrupts, we 
             * only run the interrupt handler once.
             */
            atomic_set(&_interrupt_has_taken_place, 0);
        } else {
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
            wait_event_timeout(_ether_interrupt_wq,     
                               atomic_read(&_ether_interrupt_has_taken_place) != 0, 100);
#else
            wait_event_interruptible(_ether_interrupt_wq,     
                                     atomic_read(&_ether_interrupt_has_taken_place) != 0);
#endif
            /* 
             * Even if we get multiple interrupts, we 
             * only run the interrupt handler once.
             */
            atomic_set(&_ether_interrupt_has_taken_place, 0);
        }
        break;
    case LUBDE_USLEEP:
        sal_usleep(io.d0);
        break;
    case LUBDE_UDELAY:
        sal_udelay(io.d0);
        break;
    case LUBDE_SEM_OP:
        switch (io.d0) {
        case LUBDE_SEM_OP_CREATE:
            io.p0 = (bde_kernel_addr_t)sal_sem_create("", io.d1, io.d2);
            break;
        case LUBDE_SEM_OP_DESTROY:
            sal_sem_destroy((sal_sem_t)io.p0);
            break;
        case LUBDE_SEM_OP_TAKE:
            io.rc = sal_sem_take((sal_sem_t)io.p0, io.d2);
            break;
        case LUBDE_SEM_OP_GIVE:
            io.rc = sal_sem_give((sal_sem_t)io.p0);
            break;
        default:
            io.rc = LUBDE_FAIL;
            break;
        }
        break;
    case LUBDE_WRITE_IRQ_MASK:
        io.rc = lkbde_irq_mask_set(io.dev, io.d0, io.d1, 0);
        break;
    case LUBDE_SPI_READ_REG:
        if (user_bde->spi_read(io.dev, io.d0, io.dx.buf, io.d1) == -1) {
            io.rc = LUBDE_FAIL;
        } 
        break;
    case LUBDE_SPI_WRITE_REG:
        if (user_bde->spi_write(io.dev, io.d0, io.dx.buf, io.d1) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_READ_REG_16BIT_BUS:
        io.d1 = user_bde->read(io.dev, io.d0);
        break;
    case LUBDE_WRITE_REG_16BIT_BUS:
        io.rc = user_bde->write(io.dev, io.d0, io.d1);
        break;
#if (defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT))
    case LUBDE_CPU_WRITE_REG:
    {
        if (lkbde_cpu_write(io.dev, io.d0, (uint32*)io.dx.buf) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
    }
    case LUBDE_CPU_READ_REG:
    {
        if (lkbde_cpu_read(io.dev, io.d0, (uint32*)io.dx.buf) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
    }
    case LUBDE_CPU_PCI_REGISTER:
    {
        if (lkbde_cpu_pci_register(io.dev) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
    }
#endif
    case LUBDE_DEV_RESOURCE:
        bde_dev = user_bde->get_dev(io.dev);
        if (bde_dev) {
            if (BDE_DEV_MEM_MAPPED(_devices[io.dev].dev_type)) {
                /* Get physical address to map */
                io.rc = lkbde_get_dev_resource(io.dev, io.d0,
                                               &io.d1, &io.d2, &io.d3);
            }
        } else {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_IPROC_READ_REG:
        io.d1 = user_bde->iproc_read(io.dev, io.d0);
        if (io.d1 == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_IPROC_WRITE_REG:
        if (user_bde->iproc_write(io.dev, io.d0, io.d1) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;

    default:
        gprintk("Error: Invalid ioctl (%08x)\n", cmd);
        io.rc = LUBDE_FAIL;
        break;
    }
  
    if (copy_to_user((void *)arg, &io, sizeof(io))) {
        return -EFAULT;
    }

    return 0;
}

/* Workaround for broken Busybox/PPC insmod */
static char _modname[] = LINUX_USER_BDE_NAME;

static gmodule_t _gmodule = 
{
    name: LINUX_USER_BDE_NAME, 
    major: LINUX_USER_BDE_MAJOR, 
    init: _init, 
    cleanup: _cleanup, 
    pprint: _pprint, 
    ioctl: _ioctl,
}; 

gmodule_t*
gmodule_get(void)
{
    _gmodule.name = _modname;
    return &_gmodule;
}
