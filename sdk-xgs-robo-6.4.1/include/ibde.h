/*
 * $Id: ibde.h,v 1.27 Broadcom SDK $
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

#ifndef __IBDE_H__
#define __IBDE_H__

#include <sal/types.h>

/*
 * Represents a collection of devices 
 */

typedef struct ibde_dev_s {
    uint16 device;
    uint8  rev;
    sal_vaddr_t base_address;
    sal_vaddr_t base_address1;
    sal_vaddr_t base_address2;
} ibde_dev_t;


typedef struct ibde_s {

    const char *(*name)(void);
  
    /* Returns the number of devices available */
    /* Each device is is accessed through a handle */
    /* Handles are assumed to index the array of devices */
  
    /* Support SWITCH or ETHERNET or CPU devices */
    int     (*num_devices)(int type);
#define BDE_ALL_DEVICES         0
#define BDE_SWITCH_DEVICES      1
#define BDE_ETHER_DEVICES       2
#define BDE_CPU_DEVICES         3

    const   ibde_dev_t *(*get_dev)(int d);

    /* 
     * Get types of underlaying devices.
     * A combination of bus type and functional type is returned.
     * In case of bus type, support PCI and SPI device types.
     * In case of functional type, specify if underlaying device is
     * a switching or ethernet device.
     */
    uint32  (*get_dev_type)(int d);
#define BDE_PCI_DEV_TYPE      SAL_PCI_DEV_TYPE    /* PCI device */
#define BDE_SPI_DEV_TYPE      SAL_SPI_DEV_TYPE    /* SPI device */
#define BDE_EB_DEV_TYPE       SAL_EB_DEV_TYPE     /* EB device */
#define BDE_ICS_DEV_TYPE      SAL_ICS_DEV_TYPE    /* ICS device */
#define BDE_MII_DEV_TYPE      SAL_MII_DEV_TYPE    /* MII device */
#define BDE_I2C_DEV_TYPE      SAL_I2C_DEV_TYPE    /* I2C device */
#define BDE_AXI_DEV_TYPE      SAL_AXI_DEV_TYPE    /* AXI device */
#define BDE_EMMI_DEV_TYPE     SAL_EMMI_DEV_TYPE   /* EMMI device */
#define BDE_DEV_BUS_ALT       SAL_DEV_BUS_ALT     /* Alternate Access */

#define BDE_DEV_BUS_TYPE_MASK SAL_DEV_BUS_TYPE_MASK
    
#define BDE_SWITCH_DEV_TYPE   SAL_SWITCH_DEV_TYPE /* Switch device */
#define BDE_ETHER_DEV_TYPE    SAL_ETHER_DEV_TYPE  /* Ethernet device */
#define BDE_CPU_DEV_TYPE      SAL_CPU_DEV_TYPE    /* CPU device */

#define BDE_BYTE_SWAP         0x01000000          /* SW byte swap */

#define BDE_256K_REG_SPACE    0x20000000          /* Map 256K (v 64K) */
#define BDE_128K_REG_SPACE    0x40000000          /* Map 128K (v 64K) */
#define BDE_320K_REG_SPACE    0x80000000          /* Map 256K+64K */

/* Bus supports only 16bit reads */
#define BDE_DEV_BUS_RD_16BIT  SAL_DEV_BUS_RD_16BIT 

/* Bus supports only 16bit writes */
#define BDE_DEV_BUS_WR_16BIT  SAL_DEV_BUS_WR_16BIT

/* Backward compatibility */
#define BDE_ET_DEV_TYPE       BDE_MII_DEV_TYPE

#define BDE_DEV_MEM_MAPPED(_d) \
    ((_d) & (BDE_PCI_DEV_TYPE | BDE_ICS_DEV_TYPE | BDE_EB_DEV_TYPE |\
             BDE_EMMI_DEV_TYPE | BDE_AXI_DEV_TYPE))

    /* 
     * PCI Bus Access
     */
    uint32  (*pci_conf_read)(int d, uint32 addr);
    int     (*pci_conf_write)(int d, uint32 addr, uint32 data);
    void    (*pci_bus_features)(int d, int *be_pio, int *be_packet,
                                int *be_other);

    uint32  (*read)(int d, uint32 addr);
    int     (*write)(int d, uint32 addr, uint32 data);

    uint32* (*salloc)(int d, int size, const char *name);
    void    (*sfree)(int d, void *ptr);
    int     (*sflush)(int d, void *addr, int length);
    int     (*sinval)(int d, void *addr, int length);

    int     (*interrupt_connect)(int d, void (*)(void*), void *data);  
    int     (*interrupt_disconnect)(int d);

    sal_paddr_t  (*l2p)(int d, void *laddr);
    uint32 *(*p2l)(int d, sal_paddr_t paddr);

    /* 
     * SPI Access via SMP
     */
    int  (*spi_read)(int d, uint32 addr, uint8 *buf, int len);
    int  (*spi_write)(int d, uint32 addr, uint8 *buf, int len);
    /* Special SPI access addresses */
#define BDE_DEV_OP_EMMI_INIT    SAL_DEV_OP_EMMI_INIT

    /* 
     * iProc register access
     */
    uint32  (*iproc_read)(int d, uint32 addr);
    int     (*iproc_write)(int d, uint32 addr, uint32 data);

    /* 
     * Shared memory access
     */
    uint32  (*shmem_read)(int dev, uint32 addr, uint8 *buf, uint32 len);
    void    (*shmem_write)(int dev, uint32 addr, uint8 *buf, uint32 len);
    sal_vaddr_t (*shmem_map)(int dev, uint32 addr, uint32 size);

} ibde_t;


/* System BDE */
extern ibde_t *bde;


#endif /* __IBDE_H__ */
