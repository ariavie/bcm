/***********************************************************************
 *
 * $Id: linux-bde.h 1.24 Broadcom SDK $
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
 * Linux Broadcom Device Enumerators
 *
 *
 * There are two Linux BDEs:
 *
 * 1. Linux Kernel BDE
 *
 *	This is a kernel module implementing a BDE
 *	for the driver running as part of the kernel. 
 * 	
 *	It manages the devices through the linux PCI interfaces, 
 *	and manages a chunk of contiguous, boot-time allocated
 *	DMA memory. This is all that is needed if the BCM driver
 *	is run as part of the kernel (in another module). 
 *
 * 2. Linux User BDE
 *
 *	This is a kernel module and userland library which implement
 * 	a complete BDE for applications running in userland. 
 *	
 *	The kernel module relies upon the real kernel bde, 
 *	and allows a user space application (through the user library)
 *	to talk directly to the devices. It also virtualized the device 
 *	interrupts, so the entire driver can be run as a userspace 
 *	application. 
 *	
 *	While this causes a significant degradation in performance, 
 *	because the system runs as a user application, the development
 *	and debugging process is about a gillion times easier. 
 *	After the core logic is debugged, it can be retargeted using
 *	only the kernel bde and run in the kernel. 
 *	
 *
 **********************************************************************/

#ifndef __LINUX_BDE_H__
#define __LINUX_BDE_H__

#include <sal/types.h>

#include <ibde.h>


/* 
 * Device Major Numbers
 * 
 * The kernel and user bdes need unique major numbers
 * on systems that do not use devfs. 
 * 
 * They are defined here, along with the module names, 
 * to document them if you need to mknod them (or open) them, 
 * and to keep them unique. 
 *
 */

#define LINUX_KERNEL_BDE_NAME 	"linux-kernel-bde"
#define LINUX_KERNEL_BDE_MAJOR 	127

#define LINUX_USER_BDE_NAME 	"linux-user-bde"
#define LINUX_USER_BDE_MAJOR   	126


/* Max devices */
/* 16 switch chips + 2 out-of-band Ethernet + 2 CPUs */
#define LINUX_BDE_MAX_SWITCH_DEVICES    16
#define LINUX_BDE_MAX_ETHER_DEVICES     2
#define LINUX_BDE_MAX_CPU_DEVICES       2
#define LINUX_BDE_MAX_DEVICES           (LINUX_BDE_MAX_SWITCH_DEVICES + \
                                         LINUX_BDE_MAX_ETHER_DEVICES + \
                                         LINUX_BDE_MAX_CPU_DEVICES) 

/* 
 * PCI devices will be initialized by the Linux Kernel, 
 * regardless of architecture.
 *
 * You need only provide bus endian settings. 
 */

typedef struct linux_bde_bus_s {
    int be_pio;
    int be_packet;
    int be_other;
} linux_bde_bus_t;

extern int linux_bde_create(linux_bde_bus_t* bus, ibde_t** bde);
extern int linux_bde_destroy(ibde_t* bde);


#ifdef __KERNEL__

/*
 *  Backdoors provided by the kernel bde
 *
 */

/*
 * The user bde needs to get some physical addresses for 
 * the userland code to mmap. 
 */
extern int lkbde_get_dma_info(uint32 *pbase, uint32 *size); 
extern uint32 lkbde_get_dev_phys(int d); 
extern uint32 lkbde_get_dev_phys_hi(int d); 

/*
 * Virtual device address needed by kernel space 
 * interrupt handler.
 */
extern void *lkbde_get_dev_virt(int d);

/*
 * The user bde needs to get some physical addresses for 
 * the userland code to mmap. The following functions
 * supports multiple resources for a single device.
 */
extern int lkbde_get_dev_resource(int d, int rsrc, uint32 *flags,
                                  uint32 *phys_lo, uint32 *phys_hi); 

/*
 * Backdoor to retrieve original hardware/OS device
 * structure, e.g. struct pci_dev.
 *
 */
extern void *lkbde_get_hw_dev(int d);

/*
 * Functions that allow an interrupt handler in user mode to
 * coexist with interrupt handler in kernel module.
 */
extern int lkbde_irq_mask_set(int d, uint32 addr, uint32 mask, uint32 fmask);
extern int lkbde_irq_imask_get(int d, uint32 *mask);

#if (defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT))
extern int lkbde_cpu_write(int d, uint32 addr, uint32 *buf);
extern int lkbde_cpu_read(int d, uint32 addr, uint32 *buf);
extern int lkbde_cpu_pci_register(int d);
#endif

/*
 * This flag must be OR'ed onto the device number when calling
 * interrupt_connect/disconnect and irq_mask_set functions from
 * a secondary device driver.
 */
#define LKBDE_ISR2_DEV  0x8000

#if defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#include <linux/version.h>
#if defined(__DUNE_LINUX_BCM_CPU_PCIE__) && ! defined(_SIMPLE_MEMORY_ALLOCATION_) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#define _SIMPLE_MEMORY_ALLOCATION_ 1
#endif
#endif

#ifndef _SIMPLE_MEMORY_ALLOCATION_
#define _SIMPLE_MEMORY_ALLOCATION_ 0
#endif

#endif /* __KERNEL__ */

#endif /* __LINUX_BDE_H__ */
