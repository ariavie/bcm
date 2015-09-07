/* config.h - Katana2 configuration header */

/*
 * Copyright (c) 2010-2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01a,25,sep13,dnb  created from arm_a9_ctx version 0x1d

*/

/*
DESCRIPTION
This file contains the configuration parameters for the 
Broadcom Katana2 board.
*/

#ifndef __INCconfigh
#define __INCconfigh

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * BSP version/revision identification, should be placed
 * before #include "configAll.h"
 */

#define BSP_VERSION     "2.0"
#define BSP_REV         "/0" /* 0 for first revision */

#include "configSdkPre.h"   /* Configs for Broadcom SDK */
#include <configAll.h>
#include "configSdkPost.h"  /* Configs for Broadcom SDK */

#include "katana2.h"

#define BOOT_FROM_UBOOT

#ifdef _WRS_CONFIG_SMP
#   define SYS_MODEL "BRCM Katana 2 MPCore"
#else
#   define SYS_MODEL "BRCM Katana 2"
#endif /* _WRS_CONFIG_SMP */

#define USER_RESERVED_MEM       0

#define LOCAL_MEM_LOCAL_ADRS    KATANA2_DRAM_BASE_VIRT
#define LOCAL_MEM_BUS_ADRS      KATANA2_DRAM_BASE_PHYS
#define LOCAL_MEM_SIZE          KATANA2_DRAM_SIZE
#define LOCAL_MEM_END_ADRS      (LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE)


/* fix this */
#define ROM_BASE_ADRS       0x1c000000     /* base of NOR Flash/EPROM */
#define ROM_TEXT_ADRS       0x1c040000     /* code start addr in ROM */
#define ROM_SIZE            0x01000000     /* size of ROM holding VxWorks*/

#define DEFAULT_BOOT_LINE \
    "et(0,0)host:vxWorks h=192.168.1.50 e=192.168.1.12 u=target"


#define HWMEM_POOL_SIZE 50000
#define INCLUDE_VXB_CMDLINE


/*
 * Cache/MMU configuration
 *
 * Note that when MMU is enabled, cache modes are controlled by
 * the MMU table entries in sysPhysMemDesc[], not the cache mode
 * macros defined here.
 */


/*
 * These processors can be either write-through or copyback (defines 
 * whether write-buffer is enabled); cache itself is write-through.
 *
 * Specifying CACHE_SNOOP_ENABLE initialized the processor for 
 * cache coherency, i.e, enables SMP mode(AMP is not supported).
 * CACHE_SNOOP_ENABLE must be defined if the system is running 
 * with SMP mode.
 */
 
#undef  USER_I_CACHE_MODE
#define USER_I_CACHE_MODE       (CACHE_COPYBACK)

#undef  USER_D_CACHE_MODE
#ifdef _WRS_CONFIG_SMP
#   define USER_D_CACHE_MODE    (CACHE_COPYBACK | CACHE_SNOOP_ENABLE)
#else
#   define USER_D_CACHE_MODE    (CACHE_COPYBACK)
#endif /* _WRS_CONFIG_SMP */

/*
 * We use the generic architecture libraries, with caches/MMUs present. A
 * call to sysHwInit0() is needed from within usrInit before
 * cacheLibInit() is called.
 */

#ifndef _ASMLANGUAGE
IMPORT void sysHwInit0 (void);
#endif /* _ASMLANGUAGE */

#define INCLUDE_SYS_HW_INIT_0
#define SYS_HW_INIT_0()         sysHwInit0()

#define INCLUDE_VXBUS
#define INCLUDE_HWMEM_ALLOC
#define INCLUDE_PLB_BUS

#define INCLUDE_TIMER_SYS
#define DRV_ARM_GIC
#define DRV_ARM_MPCORE_TIMER

#undef  CONSOLE_BAUD_RATE
#define CONSOLE_BAUD_RATE 115200
#define INCLUDE_SIO_UTILS
#define DRV_SIO_NS16550

#undef NUM_TTY
#define NUM_TTY 1

#define  INCLUDE_NETWORK

#ifdef  INCLUDE_NETWORK
#define INCLUDE_END
#define DRV_VXBEND_IPROC
#define INCLUDE_MII_BUS
#undef  INCLUDE_BCM54XXPHY
#define INCLUDE_GENERICPHY
#endif

#define STANDALONE_NET
#define INCLUDE_NET_INIT
#define INCLUDE_IFCONFIG
#define INCLUDE_PING

#define INCLUDE_PCI_BUS
#define INCLUDE_PCI_BUS_AUTOCONF
#define DRV_IPROC_PCIE

#define INCLUDE_VXBUS_SHOW
#define INCLUDE_PCI_BUS_SHOW

#define DRV_TIMER_IPROC_CCB

/* see katana2.h for default memory map */
#define PCIEX0_MEMIO_ADRS  0x08000000
#define PCIEX0_MEMIO_SIZE  0x08000000

#ifdef BOOTAPP
#   undef INCLUDE_MMU_BASIC
#   undef INCLUDE_MMU_FULL 
#   undef INCLUDE_MMU_GLOBAL_MAP
#   undef INCLUDE_EDR_SYSDBG_FLAG
#else /*BOOTAPP*/
#   define INCLUDE_MMU_BASIC
#   define INCLUDE_MMU_FULL 
#   define INCLUDE_CACHE_SUPPORT
#endif /*BOOTAPP*/

/* L2 cache support */
#undef INCLUDE_L2_CACHE

/* I2C */
#define CONFIG_I2C_SPEED    0    	/* Default on 100KHz */
#define CONFIG_I2C_SLAVE    0xff	/* No slave address */

/* SPI flash configurations */
#define CONFIG_IPROC_QSPI
#define CONFIG_IPROC_QSPI_BUS                   0
#define CONFIG_IPROC_QSPI_CS                    0

/* SPI flash configuration - flash specific */
#define CONFIG_IPROC_BSPI_DATA_LANES            1
#define CONFIG_IPROC_BSPI_ADDR_LANES            1
#define CONFIG_IPROC_BSPI_READ_CMD              0x0b
#define CONFIG_IPROC_BSPI_READ_DUMMY_CYCLES     8
#define CONFIG_SF_DEFAULT_SPEED                 50000000
#define CONFIG_SF_DEFAULT_MODE                  SPI_MODE_3

#define INCLUDE_FLASH
/* parameters for SPI flash */
#define SPI_FLASH_NAME                          "N25Q256"
#define SPI_FLASH_ID                            0xba19
#define SPI_FLASH_PAGE_SIZE			256
#define SPI_FLASH_PAGES_PER_SECTOR              256
#define SPI_FLASH_N_SECTORS                     512
#define SPI_FLASH_SECTOR_SIZE                   (256 * SPI_FLASH_PAGES_PER_SECTOR)
#define SPI_FLASH_SIZE                          (SPI_FLASH_SECTOR_SIZE * SPI_FLASH_N_SECTORS)  

#define SPI_FLASH_UBOOT_ENV_OFFSET              0x000C0000
#define SPI_FLASH_UBOOT_SCHMOO_PARAMS           0x000E0000
#define SPI_FLASH_VXWORKS_PARAMS                0x00100000

/* NVRAM configuration needed by sysNet.c/h */
/*
 * FLASH_OVERLAY means saving the sector firstly and then write the
 * speficied offset.
 */

#define FLASH_OVERLAY

#ifdef CONFIG_IPROC_QSPI
#undef  NV_BOOT_OFFSET
#define NV_BOOT_OFFSET       0
#define NV_BOOT_LINE_SIZE    0x200
#define NV_RAM_SIZE          (SPI_FLASH_SECTOR_SIZE * 2)
#define NV_MAC_ADRS_OFFSET   SPI_FLASH_SECTOR_SIZE
#define NV_RAM_ADRS          SPI_FLASH_VXWORKS_PARAMS
#else
#   define NV_RAM_SIZE       NONE
#endif /* CONFIG_IPROC_QSPI */

/* MAC Address configuration */

#define MAC_ADRS_LEN     6
#define MAX_MAC_ADRS     1
#define MAX_MAC_DEVS     1 

#define IPROC_ENET0      0x00   /* Epigram specific portion of MAC (MSB->LSB) */
#define IPROC_ENET1      0x90
#define IPROC_ENET2      0x4C

#define CUST_ENET3       0x06   /* Customer specific portion of MAC address */
#define CUST_ENET4       0xA5
#define CUST_ENET5       0x72

/* default mac address */

#define ENET_DEFAULT0 IPROC_ENET0
#define ENET_DEFAULT1 IPROC_ENET1
#define ENET_DEFAULT2 IPROC_ENET2

#define  INCLUDE_DEBUG_KWRITE_USER
#ifdef  INCLUDE_DEBUG_KWRITE_USER
    #define IPROC_UART0_PA IPROC_CCA_UART0_REG_BASE
    #ifndef _ASMLANGUAGE
    extern STATUS myPutstr( char *p, size_t len);
    #endif
    #undef  DEBUG_KWRITE_USR_RTN
#define DEBUG_KWRITE_USR_RTN myPutstr
#endif

/*
 * miscellaneous definitions
 * Note: ISR_STACK_SIZE is defined here rather than in ../all/configAll.h
 * (as is more usual) because the stack size depends on the interrupt
 * structure of the BSP.
 */

/*
 * interrupt mode - interrupts can be in either preemptive or non-preemptive
 * mode. For preemptive mode, change INT_MODE to INT_PREEMPT_MODEL
 */

#define INT_MODE                 INT_NON_PREEMPT_MODEL

#define ISR_STACK_SIZE  0x2000  /* size of ISR stack, in bytes */

#ifdef __cplusplus
}
#endif
#endif  /* __INCconfigh */

#if defined(PRJ_BUILD)
#include "prjParams.h"
#endif


#ifndef BOOTAPP
#ifdef BROADCOM_BSP
#define INCLUDE_USER_APPL
#define USER_APPL_INIT { extern void vxSpawn(void); vxSpawn(); }
#define INCLUDE_SHELL
#define INCLUDE_SHELL_INTERP_C  /* C interpreter */
#define INCLUDE_SHELL_INTERP_CMD /* shell command interpreter */

#else
#define INCLUDE_SHELL
#define INCLUDE_SHELL_INTERP_C  /* C interpreter */
#define INCLUDE_SHELL_INTERP_CMD /* shell command interpreter */
#endif
#endif

#define INCLUDE_DOSFS           /* dosFs file system */
#define INCLUDE_FLASH
