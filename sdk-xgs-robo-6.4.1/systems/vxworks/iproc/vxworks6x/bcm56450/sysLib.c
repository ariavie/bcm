/* sysLib.c - Katana2 system-dependent routines */

/*
 * Copyright (c) 2010-2011,2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */
 
/*
modification history
--------------------
01a,28oct13,dnb  created from A9x4 version 1c
*/

/*
DESCRIPTION
This library provides board-specific routines for the 
Katana2 BSP
*/

/* includes */

#include <vxWorks.h>
#include "config.h"

#if !defined(INCLUDE_MMU) && \
    (defined(INCLUDE_CACHE_SUPPORT) || defined(INCLUDE_MMU_BASIC) || \
     defined(INCLUDE_MMU_FULL) || defined(INCLUDE_MMU_MPU) || \
     defined(INCLUDE_MMU_GLOBAL_MAP))
#   define INCLUDE_MMU
#endif

#include <sysLib.h>
#include <string.h>
#include <intLib.h>
#include <taskLib.h>
#include <vxLib.h>
#include <muxLib.h>
#include <cacheLib.h>

#ifdef INCLUDE_MMU
#   include <arch/arm/mmuArmLib.h>
#   include <private/vmLibP.h>
#endif /* INCLUDE_MMU */
#include <dllLib.h>

#include <private/windLibP.h>

#ifdef INCLUDE_VXIPI
#   include <vxIpiLib.h>
#endif /* INCLUDE_VXIPI */

#ifdef _WRS_CONFIG_SMP
#   include <arch/arm/vxAtomicArchLib.h>
#endif /* _WRS_CONFIG_SMP */

#include <hwif/intCtlr/vxbArmGenIntCtlr.h>

#include <hwif/vxbus/vxBus.h>
#include "hwconf.c"

/* imports */

IMPORT void hardWareInterFaceInit(void);

#ifdef INCLUDE_SIO_UTILS
IMPORT void sysSerialConnectAll(void);
#endif /* INCLUDE_SIO_UTILS */

#ifdef INCLUDE_CACHE_SUPPORT
IMPORT void cacheCortexA9LibInstall(VIRT_ADDR(physToVirt)(PHYS_ADDR), PHYS_ADDR(virtToPhys)(VIRT_ADDR));
#endif /* INCLUDE_CACHE_SUPPORT */

#ifdef INCLUDE_MMU
IMPORT void mmuCortexA8LibInstall(VIRT_ADDR(physToVirt)(PHYS_ADDR), PHYS_ADDR(virtToPhys)(VIRT_ADDR));
#endif /* INCLUDE_MMU */

#ifndef _ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK
IMPORT VOIDFUNCPTR _func_armIntStackSplit;  /* ptr to fn to split stack */
#endif /* !_ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK */

/* globals */

#ifdef INCLUDE_MMU

/*
 * The following structure describes the various different parts of the
 * memory map to be used only during initialization by
 * vm(Base)GlobalMapInit() when INCLUDE_MMU_BASIC/FULL/GLOBAL_MAP are
 * defined.
 *
 * Clearly, this structure is only needed if the CPU has an MMU!
 *
 * The following are not the smallest areas that could be allocated for a
 * working system. If the amount of memory used by the page tables is
 * critical, they could be reduced.
 */


PHYS_MEM_DESC sysPhysMemDesc [] =
    {
    /* DRAM - Always the first entry */
    {
    KATANA2_DRAM_BASE_VIRT,    /* virtual address */
    KATANA2_DRAM_BASE_PHYS,    /* physical address */
    ROUND_UP (KATANA2_DRAM_SIZE, PAGE_SIZE),
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE
    },
    {
    0x00000000,    /* virtual address */
    0x00000000,    /* physical address */
    0x08000000,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
    {
    IPROC_CCA_REG_BASE,
    IPROC_CCA_REG_BASE,
    0x00200000,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE	 | VM_STATE_CACHEABLE_NOT
    },
    {
    0x19000000,    /* ARMCore and PCIe MSI region */
    0x19000000,
    0x00040000,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE	 | VM_STATE_CACHEABLE_NOT
    },
    {
    0x48000000,    /* CMICD region */
    0x48000000,
    ROUND_UP(0x00040000, PAGE_SIZE),
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE	 | VM_STATE_CACHEABLE_NOT
    },
    {
    0x1c000000,    /* nand flash */
    0x1c000000,
    0x02000000,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE	 | VM_STATE_CACHEABLE_NOT
    },
    {
    0x1e000000,    /* spi flash */
    0x1e000000,
    0x02000000,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE	 | VM_STATE_CACHEABLE_NOT
    },

#ifdef DRV_IPROC_PCIE
    {
    PCIEX0_MEMIO_ADRS,    /* PCIe 0 memory region */
    PCIEX0_MEMIO_ADRS,
    ROUND_UP(PCIEX0_MEMIO_SIZE, PAGE_SIZE),
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE	 | VM_STATE_CACHEABLE_NOT
    },
#endif
    };

int sysPhysMemDescNumEnt = NELEMENTS (sysPhysMemDesc);

#endif /* INCLUDE_MMU */

int    sysCpu      = CPU;            /* system CPU type (e.g. ARMARCH7) */
char * sysBootLine = BOOT_LINE_ADRS; /* address of boot line */
char * sysExcMsg   = EXC_MSG_ADRS;   /* catastrophic message area */
int    sysProcNum;                   /* processor number of this CPU */

/* locals */

#ifdef _WRS_CONFIG_SMP

/* Non-Boot CPU Start info. Managed by sysCpuEnable */

struct sysMPCoreStartup
    {
    UINT32      newPC;          /* Address of 'C' based startup code */
    UINT32      newSP;          /* Stack pointer for startup */
    UINT32      newArg;         /* vxWorks kernel entry point */
    UINT32      newSync;        /* Translation Table Base and sync */
    };

extern struct sysMPCoreStartup sysMPCoreStartup[VX_SMP_NUM_CPUS];

#endif /* _WRS_CONFIG_SMP */

/* externals */
void (*_vxb_msDelayRtn)(int);
void (*_vxb_usDelayRtn)(int);

#ifndef _ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK
IMPORT void sysIntStackSplit(char *, long);
#endif /* !_ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK */

/* globals */

/* forward declarations */
void sysMsDelay(INT32);
void sysUsDelay(INT32);


#ifdef _WRS_CONFIG_SMP
IMPORT void   mmuCortexA8TtbrSetAll(MMU_LEVEL_1_DESC *);
IMPORT void   mmuCortexA8DacrSet(UINT32 dacrVal);
IMPORT void   mmuCortexA8AcrSet(UINT32 acrVal);
IMPORT STATUS sysArmGicDevInit(void);
IMPORT void   mmuCortexA8AEnable(UINT32 cacheState);
IMPORT void   mmuCortexA8ADisable(void);
IMPORT void   armInitExceptionModes(void);
IMPORT void   sysCpuInit(void);
IMPORT void   sysMPCoreApResetLoop(void);
IMPORT void   cacheCortexA9MPCoreSMPInit(void);
IMPORT void   mmuCortexA8TLBIDFlushAll(void);
IMPORT MMU_LEVEL_1_DESC * mmuCortexA8TtbrGet(void);

UINT32        sysCpuAvailableGet(void);
STATUS        sysCpuEnable(unsigned int, void (* startFunc) (void), char *);
#endif /* _WRS_CONFIG_SMP */

/* included source files */

#ifdef INCLUDE_FLASH
#   include "nvRamToFlash.c"
#else /* INCLUDE_FLASH */
#   include <mem/nullNvRam.c>
#endif /* INCLUDE_FLASH */


#ifdef INCLUDE_L2_CACHE
#   include "sysL2Cache.c"
#endif /* INCLUDE_L2_CACHE */

#ifdef INCLUDE_END
#   include "sysNet.c"
#endif /* INCLUDE_END */

#ifdef INCLUDE_VFP
IMPORT STATUS vfpEnable(void);
IMPORT STATUS vfpDisable(void);
#endif

/*******************************************************************************
*
* sysModel - return the model name of the CPU board
*
* This routine returns the model name of the CPU board.
*
* RETURNS: A pointer to a string identifying the board and CPU.
*
* ERRNO: N/A
*/

char *sysModel (void)
    {
    return SYS_MODEL;
    }

/*******************************************************************************
*
* sysBspRev - return the BSP version with the revision eg 2.0/<x>
*
* This function returns a pointer to a BSP version with the revision.
* e.g. 2.0/<x>. BSP_REV is concatenated to BSP_VERSION to form the
* BSP identification string.
*
* RETURNS: A pointer to the BSP version/revision string.
*
* ERRNO: N/A
*/

char * sysBspRev (void)
    {
    return (BSP_VERSION BSP_REV);
    }

/*******************************************************************************
*
* sysHwInit0 - perform early BSP-specific initialization
*
* This routine performs such BSP-specific initialization as is necessary before
* the architecture-independent cacheLibInit can be called. It is called
* from usrInit() before cacheLibInit(), before sysHwInit() and before BSS
* has been cleared.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysHwInit0 (void)
    {
    
#ifdef INCLUDE_CACHE_SUPPORT

    /*
     * Install the appropriate cache library, no address translation
     * routines are required for this BSP, as the default memory map has
     * virtual and physical addresses the same.
     */

    cacheCortexA9LibInstall(mmuPhysToVirt, mmuVirtToPhys);

#ifdef INCLUDE_L2_CACHE
    sysL2CacheInit();
#endif /* INCLUDE_L2_CACHE */

#endif /* INCLUDE_CACHE_SUPPORT */

#ifdef INCLUDE_MMU

    /* Install the appropriate MMU library and translation routines */

    mmuCortexA8LibInstall(mmuPhysToVirt, mmuVirtToPhys);

#endif /* defined(INCLUDE_MMU) */

    return;
    }


/*******************************************************************************
*
* sysHwInit - initialize the CPU board hardware
*
* This routine initializes various features of the hardware.
* Normally, it is called from usrInit() in usrConfig.c.
*
* NOTE: This routine should not be called directly by the user.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysHwInit (void)
    {

    /* set up our ownd vxbus delay routines */
    _vxb_msDelayRtn = sysMsDelay;
    _vxb_usDelayRtn = sysUsDelay;
    

    /* install the IRQ/SVC interrupt stack splitting routine */
    
#ifndef _ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK
    _func_armIntStackSplit = sysIntStackSplit;
#endif    /* !_ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK */

    hardWareInterFaceInit();

#ifdef  FORCE_DEFAULT_BOOT_LINE
    strncpy(sysBootLine, DEFAULT_BOOT_LINE, strlen(DEFAULT_BOOT_LINE)+1);
#endif /* FORCE_DEFAULT_BOOT_LINE */
    }

/*******************************************************************************
*
* sysHwInit2 - additional system configuration and initialization
*
* This routine connects system interrupts and does any additional
* configuration necessary. Note that this is called from
* sysClkConnect() in the timer driver.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysHwInit2 (void)
    {
    static BOOL initialized = FALSE;

    if (initialized)
        {
        return;
        }

    vxbDevInit ();

#ifdef INCLUDE_SIO_UTILS
    sysSerialConnectAll ();
#endif

    taskSpawn("tDevConn", 11, 0, 10000, vxbDevConnect, 
	      0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

    initialized = TRUE;
    }

/*******************************************************************************
*
* sysPhysMemTop - get the address of the top of physical memory
*
* This routine returns the address of the first missing byte of memory,
* which indicates the top of memory.
*
* Normally, the user specifies the amount of physical memory with the
* macro LOCAL_MEM_SIZE in config.h.  BSPs that support run-time
* memory sizing do so only if the macro LOCAL_MEM_AUTOSIZE is defined.
* If not defined, then LOCAL_MEM_SIZE is assumed to be, and must be, the
* true size of physical memory.
*
* NOTE: Do no adjust LOCAL_MEM_SIZE to reserve memory for application
* use. See sysMemTop() for more information on reserving memory.
*
* RETURNS: The address of the top of physical memory.
*
* ERRNO: N/A
*
* SEE ALSO: sysMemTop()
*/

char * sysPhysMemTop (void)
    {
      return (char *)(LOCAL_MEM_END_ADRS);
    }

/*******************************************************************************
*
* sysMemTop - get the address of the top of VxWorks memory
*
* This routine returns a pointer to the first byte of memory not
* controlled or used by VxWorks.
*
* The user can reserve memory space by defining the macro USER_RESERVED_MEM
* in config.h.  This routine returns the address of the reserved memory
* area.  The value of USER_RESERVED_MEM is in bytes.
*
* RETURNS: The address of the top of VxWorks memory.
*
* ERRNO: N/A
*/

char * sysMemTop (void)
    {
    static char * memTop = NULL;

    if (memTop == NULL)
        {
#ifdef INCLUDE_EDR_PM
        memTop = (char *)sysPhysMemTop () - USER_RESERVED_MEM - PM_RESERVED_MEM;
#else
        memTop = (char *)sysPhysMemTop () - USER_RESERVED_MEM;
#endif /* INCLUDE_EDR_PM */
        }

    return (memTop);
    }

/*******************************************************************************
*
* sysToMonitor - transfer control to the ROM monitor
*
* This routine transfers control to the ROM monitor. It is usually called
* only by reboot() -- which services ^X -- and bus errors at interrupt
* level.  However, in some circumstances, the user may wish to introduce a
* new <startType> to enable special boot ROM facilities.
*
* RETURNS: Does not return.
*
* ERRNO: N/A
*/

STATUS sysToMonitor
    (
    int startType    /* passed to ROM to tell it how to boot */
    )
    {
    FUNCPTR    pRom;
    int i;
    int intID;
    int j;
    volatile UINT32 intrAck;
    volatile UINT32 intActiveBits;

#if 0
#ifdef _WRS_CONFIG_SMP
    cpuset_t cpuList;    
    volatile int idx;
    int srcCpuId;

    /* if the current core is not core0, we won't run sysToMonitor directly */
        
    if (vxCpuIndexGet() != 0)
        {

        /* 
         * 0xFEFEFEFE means that all other cores are indicated to shutdown 
         * by core 0 
         */

        if ((startType & 0xFFFFFFF0) != 0xFEFEFEF0)
            {

            /* If not directed here then tell CPU 0 to enter sysToMonitor */

            vxIpiConnect(INT_LVL_MPCORE_RESET, (IPI_HANDLER_FUNC)(sysToMonitor), 
                (void *)(0xFEF00000 | startType | (vxCpuIndexGet() << 16)));

            vxIpiEnable(INT_LVL_MPCORE_RESET);

            vxIpiConnect(INT_LVL_MPCORE_IPI08, (IPI_HANDLER_FUNC)(sysToMonitor), 
                (void *)(0xFEFEFEF0));

            vxIpiEnable(INT_LVL_MPCORE_IPI08);

            vxIpiEmit(INT_LVL_MPCORE_RESET, 1);

            /* wait for a reset from core 0 */

            for (;;);

            }

         /* Disable Core intrs */
		
         ARMA9CTX_REGISTER_WRITE(PBXA9_GIC_CPU_CONTROL, 0x0);
	 		 	
         /*
          * If this is not core0, it must be informed by core0 to reboot 
          * through IPI. Therefore, we must acknowkedge the IPI interrupt.
          * The IPI(SGI) interrupt information can not be got from Interrupt 
          * Acknowledge Register, because ISR in interrupt controller driver 
          * has read(to clear) this register.
          */

         srcCpuId = (startType & 0xF) << 10;

         ARMA9CTX_REGISTER_WRITE(PBXA9_GIC_CPU_END_INTR, INT_LVL_MPCORE_RESET | srcCpuId);

         ARMA9CTX_REGISTER_WRITE(PBXA9_GIC_CPU_END_INTR, INT_LVL_MPCORE_IPI08 | srcCpuId);
	 

	 /* empty interrupt ack fifo */

         intrAck = ARMA9CTX_REGISTER_READ(PBXA9_GIC_CPU_ACK) & GIC_INT_SPURIOUS;

         while ((intrAck != GIC_INT_SPURIOUS) )
             {
             ARMA9CTX_REGISTER_WRITE(PBXA9_GIC_CPU_END_INTR, intrAck);
        
             intrAck = ARMA9CTX_REGISTER_READ(PBXA9_GIC_CPU_ACK) & GIC_INT_SPURIOUS;
             }
         
        /* flush data cache */

        cachePipeFlush();
        
        /* disable the MMU, cache(s) and write-buffer */

        mmuCortexA8ADisable();


        /* Make sure we go back to the bootMonitor loop */

        pRom = (FUNCPTR)(ROM_TEXT_ADRS + 4);

        /* jump to boot ROM */
         
        (* pRom)(0);

        }

#endif /* _WRS_CONFIG_SMP */

    sysClkDisable();

    intIFLock();

#ifdef _WRS_CONFIG_SMP

    /* here is the core 0 */

    if ((startType & 0xFFF00000) == 0xFEF00000)
        {

        /* 
         * core0 is informed by other core through IPI to reboot, so we must
         * acknowledge the IPI(SGI) interrupt.
         */

        srcCpuId = (startType & 0x000F0000) >> 16;

        ARMA9CTX_REGISTER_WRITE(PBXA9_GIC_CPU_END_INTR,
            INT_LVL_MPCORE_RESET + (srcCpuId << 10));

        startType = (startType & 0xFFFF);
        }

    /* clear entry address flags */

    ARMA9CTX_REGISTER_WRITE(PBXA9_SR_FLAGSCLR, 0xffffffff);

    /* direct all interrupts to go to CPU 0 */
        
   for (i = SPI_START_INT_NUM; i < SYS_INT_LEVELS_MAX; i += PRIOS_PER_WORD)
       {
       ARMA9CTX_REGISTER_WRITE(PBXA9_GIC1_BASE + PBXA9_GIC_DIST_TARG \
           + (0x4 * (i / TARGETS_PER_WORD)), GIC_CPU_DIR_DEFAULT);
       }        

    /* reset all other cores */

    if ((cpuList = vxCpuEnabledGet()) && (cpuList > 1))
        {

        if (_WRS_KERNEL_CPU_GLOBAL_GET(vxCpuIndexGet(), intCnt) != 0)
            {
            for (idx = 1; idx < VX_SMP_NUM_CPUS; idx++)
                {
                if (cpuList & (1 << idx))
                    {
                    vxIpiEmit(INT_LVL_MPCORE_IPI08, 1 << idx);
                    }
                }
            }
        else
            {
            vxIpiConnect(INT_LVL_MPCORE_RESET, (IPI_HANDLER_FUNC)(sysToMonitor), 
                (void *)(0xFEFEFEF0 | vxCpuIndexGet()));

            vxIpiEnable(INT_LVL_MPCORE_RESET);

            for (idx = 1; idx < VX_SMP_NUM_CPUS ; idx++)
                {
                if (cpuList & (1 << idx))
                    {
                    vxIpiEmit(INT_LVL_MPCORE_RESET, 1 << idx);
                    }
                }
            }
        }

    /* delay for non-boot core to shutdown */

    for (idx = 0; idx < 0x1ffff; idx++);

#endif /* _WRS_CONFIG_SMP */

    /* disable GIC cpu interface */
    
    ARMA9CTX_REGISTER_WRITE(PBXA9_GIC_CPU_CONTROL, 0x0);

    /* disable GIC distributor */
    
    ARMA9CTX_REGISTER_WRITE(PBXA9_GIC1_BASE + PBXA9_GIC_DIST_CONTROL, 0x0);

    /* clear any active interrupts */
    
    for (i = SPI_START_INT_NUM; i < SYS_INT_LEVELS_MAX; i += BITS_PER_WORD)
        {

        intActiveBits = ARMA9CTX_REGISTER_READ(PBXA9_GIC1_BASE + 
                                               PBXA9_GIC_DIST_ACTIVE1 + 
                                               0x4 * NWORD(i));

        if (intActiveBits != 0)
            {
            for (j = 0; j < BITS_PER_WORD; j++)
                {
                if (intActiveBits & BIT(j))
                    {
                    intID = i + j;
                    ARMA9CTX_REGISTER_WRITE(PBXA9_GIC_CPU_END_INTR, intID);
                    }
                }
            }
        }

    /* disable all SPI interrupts */
    
    for (i = SPI_START_INT_NUM; i < SYS_INT_LEVELS_MAX; i += BITS_PER_WORD)
        {
        ARMA9CTX_REGISTER_WRITE(PBXA9_GIC1_BASE + 
            PBXA9_GIC_DIST_ENABLE_CLR1 + (0x4 * NWORD(i)), 0xffffffff);
        }

    /* empty interrupt ack fifo */
    
    intrAck = ARMA9CTX_REGISTER_READ(PBXA9_GIC_CPU_ACK) & GIC_INT_SPURIOUS;

    while ((intrAck != GIC_INT_SPURIOUS) && (intrAck >= SGI_INT_MAX) )
        {
        ARMA9CTX_REGISTER_WRITE(PBXA9_GIC_CPU_END_INTR, intrAck);
        
        intrAck = ARMA9CTX_REGISTER_READ(PBXA9_GIC_CPU_ACK) & GIC_INT_SPURIOUS;
        }
       

#ifdef INCLUDE_MMU

    /* flush data cache */

    cacheFlush((CACHE_TYPE)DATA_CACHE, (void *)0, (size_t)ENTIRE_CACHE);

    cachePipeFlush();

#ifdef _WRS_CONFIG_SMP
    mmuCortexA8AcrSet(0);
#ifdef INCLUDE_VFP
    vfpDisable();
#endif /* INCLUDE_VFP */
#endif /* _WRS_CONFIG_SMP */

    /* disable the MMU, cache(s) and write-buffer */

    mmuCortexA8ADisable();

#endif /* INCLUDE_MMU */

    pRom = (FUNCPTR)(ROM_TEXT_ADRS + 4);

    /* jump to boot ROM */
    
    (* pRom)(startType);

#endif
    return OK;
    }

/*******************************************************************************
*
* sysProcNumGet - get the processor number
*
* This routine returns the processor number for the CPU board, which is
* set with sysProcNumSet().
*
* RETURNS: The processor number for the CPU board.
*
* ERRNO: N/A
*
* SEE ALSO: sysProcNumSet()
*/

int sysProcNumGet (void)
    {
    return sysProcNum;
    }

/*******************************************************************************
*
* sysProcNumSet - set the processor number
*
* Set the processor number for the CPU board. Processor numbers should be
* unique on a single backplane.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* SEE ALSO: sysProcNumGet()
*/

void sysProcNumSet
    (
    int procNum        /* processor number */
    )
    {
    sysProcNum = procNum;
    }

#ifdef INCLUDE_SIO_UTILS
/******************************************************************************
*
* bspSerialChanGet - get the SIO_CHAN device associated with a serial channel
*
* The sysSerialChanGet() routine returns a pointer to the SIO_CHAN
* device associated with a specified serial channel. It is called
* by usrRoot() to obtain pointers when creating the system serial
* devices, `/tyCo/x'. It is also used by the WDB agent to locate its
* serial channel.  The VxBus function requires that the BSP provide a
* function named bspSerialChanGet() to provide the information about
* any non-VxBus serial channels, provided by the BSP.  As this BSP
* does not support non-VxBus serial channels, this routine always
* returns ERROR.
*
* RETURNS: ERROR, always
*
* ERRNO: N/A
*
* \NOMANUAL
*/

SIO_CHAN * bspSerialChanGet
    (
    int channel     /* serial channel */
    )
    {
    return ((SIO_CHAN *) ERROR);
    }
#endif

#if defined(_WRS_CONFIG_SMP)
/*******************************************************************************
*
* sysCpuStart - vxWorks startup
*
* This routine establishes a CPUs vxWorks envirnonment.
*
* This is NOT callable from C
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void sysCpuStart
    (
    void (*startFunc) (void),
    int cpuNum,
    UINT32 tTbr
    )
    {

    volatile UINT32 intrAck;
    int intId;

    intId = ARMA9CTX_REGISTER_READ(PBXA9_GIC_CPU_ACK) & GIC_INT_SPURIOUS;

    /* 
     * this core is waked up by core0, so we must acknowledge the IPI interrupt
     * emitted by core0.
     */
     
    if(intId < SGI_INT_MAX)
        {
        ARMA9CTX_REGISTER_WRITE(PBXA9_GIC_CPU_END_INTR, intId);
        }

    /* flush all pending local interrupts */

    intrAck = ARMA9CTX_REGISTER_READ(PBXA9_GIC_CPU_ACK) & GIC_INT_SPURIOUS;
    
    while ((intrAck != GIC_INT_SPURIOUS) && (intrAck >= SGI_INT_MAX))
        {
        ARMA9CTX_REGISTER_WRITE(PBXA9_GIC_CPU_END_INTR, intrAck);
        
        intrAck = ARMA9CTX_REGISTER_READ(PBXA9_GIC_CPU_ACK) & GIC_INT_SPURIOUS;
        }

    ARMA9CTX_REGISTER_WRITE(PBXA9_GIC1_BASE + PBXA9_GIC_DIST_PEND_CLR1, 0xffff);

    /* Initialise ARM exception mode registers */

    armInitExceptionModes();

    /* Set Translation Table Base Register */

    mmuCortexA8TtbrSetAll((MMU_LEVEL_1_DESC *)tTbr);

    /* Set Domain Access Control */
    
    mmuCortexA8DacrSet(MMU_DACR_VAL_NORMAL);

    /* setup AUX register */

    mmuCortexA8AcrSet(AUX_CTL_REG_FW | AUX_CTL_REG_L1_PRE_EN);

    /* announce we are part of SMP */

    cacheCortexA9MPCoreSMPInit();

#ifdef INCLUDE_VFP
    vfpEnable();
#endif

    /* Enable MMU */

    mmuCortexA8AEnable(MMU_INIT_VALUE | MMUCR_I_ENABLE | MMUCR_C_ENABLE);

    /* Enable Local S/W interrupts */

    sysArmGicDevInit();

    /* Tell the boot CPU we are here */    

    sysMPCoreStartup[cpuNum].newSync = 0;

    intIFUnlock(0);

    /* Enter the Kernel */

    (*startFunc)();

    }

/*******************************************************************************
*
* sysCpuEnable - enable a multi core CPU
*
* This routine brings a CPU out of reset
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sysCpuEnable
    (
    unsigned int cpuNum,
    void (*startFunc) (void),
    char *stackPtr
    )
    {

    /* Validate cpuNum */

    if (cpuNum < 1 || cpuNum >= VX_MAX_SMP_CPUS)
        return (ERROR);

    /* Setup init values */

    sysMPCoreStartup[cpuNum].newPC   = (UINT32)sysCpuStart;
    sysMPCoreStartup[cpuNum].newSP   = (UINT32)stackPtr;
    sysMPCoreStartup[cpuNum].newArg  = (UINT32)startFunc;
    sysMPCoreStartup[cpuNum].newSync = (UINT32)mmuCortexA8TtbrGet();

    /* Make sure data hits memory */

    cacheFlush((CACHE_TYPE)DATA_CACHE, (void *)sysMPCoreStartup, (size_t)(sizeof(sysMPCoreStartup)));
    
    cachePipeFlush();

   /* Data Synchronization Barrier */
    
    VX_SYNC_BARRIER();

    /* Setup that CPU for interrupt */

    ARMA9CTX_REGISTER_WRITE(PBXA9_SR_FLAGSCLR, 0xffffffff);

    ARMA9CTX_REGISTER_WRITE(PBXA9_SR_FLAGSSET, (UINT32)sysCpuInit);

    /* Bump target CPU out of boot monitor idle loop */

    vxIpiEmit(INT_LVL_MPCORE_START, 1 << cpuNum);

    return OK;

    }

#endif /* if defined(_WRS_CONFIG_SMP) */

/*******************************************************************************
*
* sysCpuAvailableGet - return the number of CPUs available
*
* This routine gets the number of CPUs available.
*
* RETURNS:  number of CPU cores available
*
* ERRNO: N/A
*/

UINT32 sysCpuAvailableGet (void)
    {
    return 1;
    }

/*******************************************************************************
*
* sysUsDelay - delay counter with microsecond resolution
*
*
* RETURNS: N/A
*/

void sysUsDelay(INT32 count)
    {
    UINT32 prescale;

    /* disable and clear the global timer */
    volatile UINT32 *p =(volatile UINT32 *)(IPROC_GLB_TIM_CTRL_ADDR);

    /* set prescalar for us resolution */
    prescale = IPROC_ARM_CLK / ( 2 * 1000000);

    /* clear counter */
    p = (volatile UINT32 *)(IPROC_GLB_TIM_COUNT_HI);
    *p = 0;

    p = (volatile UINT32 *)(IPROC_GLB_TIM_COUNT_LO);
    *p = 0;

    /* enable counter */
    p =(volatile UINT32 *)(IPROC_GLB_TIM_CTRL_ADDR);
    *p = (prescale << 8) | 0x00000001;
 
    p = (volatile UINT32 *)(IPROC_GLB_TIM_COUNT_LO);
 
    while(*p < count) ;

    return;
    }

/*******************************************************************************
*
* sysMsDelay - time delay in the unit of ms
*
* This routine delays for approximately one ms. When system timer 
* count register add 1 tick,
* 1 tick = 1/system timer clk = 1/SYS_TIMER_CLK = 1/1000000 (s) = 1000ns = 1us
* timer grows up.
*
* RETURNS: N/A
*/

void sysMsDelay
    (
    INT32        delay          /* length of time in MS to delay */
    )
    {
    sysUsDelay(1000 * delay);
    }

/*******************************************************************************
*
* sysDelay - delay for approximately one millisecond
*
* This routine delays for approximately one milli-second.
*
* RETURNS: N/A
*/

void sysDelay (void)
    {
    sysMsDelay(1);
    }

/*******************************************************************************
*
* sysUartFreqGet - get uart freq
*
* RETURNS: N/A
*/
extern uint32_t iproc_get_uart_clk(uint32_t uart);

UINT32 sysUartFreqGet(void)
    {
    return iproc_get_uart_clk(0);
    }


#if defined(IPROC_UART0_PA)
/*******************************************************************************
*
* putc - direct serial output
*
*
* RETURNS: N/A
*/

#define UART0_LSR_OFFSET	0x05
#define UART0_LSR_THRE_MASK	0x60

void myPutc(int c)
    {
    volatile char *p = 
	(volatile char *)IPROC_UART0_PA;

    while((p[UART0_LSR_OFFSET] & UART0_LSR_THRE_MASK) == 0);

    *p = (char)c;
    } 
    
STATUS myPutstr( char *p, size_t len)
    {
    while(len > 0)
	{
	myPutc(*p);
	p += 1;
	len -= 1;
	}
   return 0;
   }
  
#endif

/* temp dummy functions */
/* SIO_CHAN *sysSerialChanGet(int a){return 0;} */

/* Line module specific */
int sysSlotIdGet();
int sysIsLM();

int
sysSlotIdGet()
{
    return 0;
}

int
sysIsLM()
{
    return 0;
}

void sysBacktraceMips(char *pfx, char *sfx, int direct)
{
}

void
sysReboot(void)
{
}

int sysToMonitorExcMessage = 0;
int sysToMonitorBacktrace = 1;
int sysToMonitorReboot = 1;
void (*sysToMonitorHook)(void);

int   sysVectorIRQ0 = 0;

/* need to be moved to sysSerial.c */
void sysSerialPrintString(char *s)
{
}
