/*
 * $Id: drv.h,v 1.663 Broadcom SDK $
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
 * This file contains structure and routine declarations for the
 * Switch-on-a-Chip Driver.
 *
 * This file also includes the more common include files so the
 * individual driver files don't have to include as much.
 */

#ifndef _SOC_DRV_H
#define _SOC_DRV_H

#include <sal/core/time.h>
#include <sal/core/boot.h>

#include <assert.h>

#include <shared/avl.h>
#include <shared/bitop.h>
#include <shared/fifo.h>
#include <shared/warmboot.h>
#include <shared/switch.h>
#include <shared/gport.h>

#include <soc/util.h>
#include <soc/error.h>
#include <soc/cm.h>
#include <soc/feature.h>
#include <soc/property.h>
#include <soc/dport.h>

#include <shared/alloc.h>
#include <sal/core/spl.h>
#include <sal/core/sync.h>
#include <sal/core/thread.h>

#include <soc/chip.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/error.h>
#include <soc/dma.h>
#include <soc/enet.h>
#include <soc/counter.h>
#include <soc/mmuerr.h>
#include <soc/drv_if.h>
#include <soc/types.h>
#include <soc/macipadr.h>
#include <soc/ll.h>
#include <soc/axp.h>
#include <soc/intr.h>
#include <soc/mem.h>
#ifdef BCM_WARM_BOOT_SUPPORT
#include <soc/scache.h>
#endif /* BCM_WARM_BOOT_SUPPORT */

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT)\
    || defined(BCM_DFE_SUPPORT)
#ifndef SWIG_C3_BUILD
#include <soc/mcm/memregs.h>
#endif
#endif

#ifdef BCM_ROBO_SUPPORT
#ifndef SWIG_C3_BUILD
#include <soc/robo/mcm/memregs.h>
#endif
#endif

#ifdef BCM_ISM_SUPPORT
#include <soc/ism.h>
#include <soc/ism_hash.h>
#endif
#ifdef BCM_CMICM_SUPPORT
#include <soc/shared/mos_intr_common.h>
#include <soc/shared/mos_msg_common.h>
#ifdef BCM_RCPU_SUPPORT
#include <soc/rcpu.h>
#endif
#endif
#ifdef BCM_SBUSDMA_SUPPORT
#include <soc/sbusdma.h>
#endif
#ifdef BCM_ROBO_SUPPORT
#include <soc/robo/robo_drv.h>
#endif
#ifdef BCM_EA_SUPPORT
#include <soc/ea/allenum.h>
#ifdef BCM_TK371X_SUPPORT
#include <soc/ctrl_if.h>
#endif
#endif


#ifdef  BCM_CALADAN3_SUPPORT
#include <soc/sbx/caladan3/port.h>
#endif

/****************************************************************
 * UNIT DRIVER ACCESS MACROS
 *
 *         MACRO                             EVALUATES TO
 *  ________________________________________________________________
 *      SOC_DRIVER(unit)                Chip driver structure
 *      SOC_INFO(unit)                  SOC Info structure
 *      SOC_MEM_INFO(unit,mem)          Memory info structure
 *      SOC_REG_INFO(unit,reg)          Register info structure
 *      SOC_BLOCK_INFO(unit,blk)        Block info structure
 *      SOC_PORT_INFO(unit,port)        Port info structure
 *      SOC_BLOCK2SCH(unit,blk)         Integer schan num for block
 *      SOC_BLOCK2OFFSET(unit,blk)      Block to idx for cmic cmds
 *      SOC_HAS_CTR_TYPE(unit, ctype)   Does the device have a given
 *                                      counter map defined?
 *      SOC_CTR_DMA_MAP(unit, ctype)    Return pointer to the counter
 *                                      map of the indicated type.
 *      SOC_CTR_TO_REG(unit, ctype, ind) Return the register index for
 *                                       a given counter index.
 *      SOC_CTR_MAP_SIZE(unit, ctype)   How many entries in a given
 *                                      counter map.
 ****************************************************************/

#define SOC_CONTROL(unit)               (soc_control[unit])
#define SOC_DRIVER(unit)                (SOC_CONTROL(unit)->chip_driver)
#define SOC_FUNCTIONS(unit)             (SOC_CONTROL(unit)->soc_functions)
#define SOC_INFO(unit)                  (SOC_CONTROL(unit)->info)
#define SOC_STAT(unit)                  (&(SOC_CONTROL(unit)->stat))
#define SOC_REG_MASK_SUBSET(unit)       (SOC_CONTROL(unit)->reg_subset_mask)

#define DRV_SERVICES(unit)              (SOC_DRIVER(unit)->services)
#define SOC_MEM_INFO(unit, mem)         (*SOC_DRIVER(unit)->mem_info[mem])
#define SOC_MEM_AGGR(unit, index)       (SOC_DRIVER(unit)->mem_aggr[index])
#define SOC_MEM_PTR(unit, mem)          (SOC_DRIVER(unit)->mem_info[mem])
#define SOC_REG_INFO(unit, reg)         (*SOC_DRIVER(unit)->reg_info[reg])
#define SOC_REG_STAGE(unit, reg)        ((SOC_REG_INFO(unit, reg).offset >> 26) & 0x3F)
#define SOC_REG_ABOVE_64_INFO(unit, reg) (*SOC_DRIVER(unit)->reg_above_64_info[reg]) 
#define SOC_REG_ARRAY_INFO(unit, reg)   (*SOC_DRIVER(unit)->reg_array_info[reg]) 
#define SOC_REG_ARRAY_INFOP(unit, reg)  (SOC_DRIVER(unit)->reg_array_info[reg]) 
#define SOC_MEM_ARRAY_INFO(unit, mem)   (*SOC_DRIVER(unit)->mem_array_info[mem]) 
#define SOC_MEM_ARRAY_INFOP(unit, mem)  (SOC_DRIVER(unit)->mem_array_info[mem]) 
#define SOC_REG_PTR(unit, reg)          (SOC_DRIVER(unit)->reg_info[reg])
#define SOC_BLOCK_INFO(unit, blk)       (SOC_DRIVER(unit)->block_info[blk])
#define SOC_PORT_IDX_INFO(unit, port, idx) \
    (SOC_DRIVER(unit)->port_info[SOC_DRIVER(unit)->port_num_blktype > 1 ? \
                                 port * SOC_DRIVER(unit)->port_num_blktype + \
                                 idx : port])
#define SOC_PORT_INFO(unit, port)       SOC_PORT_IDX_INFO(unit, port, 0)
                                                                     
#define SOC_CHIP_STRING(unit)           (SOC_DRIVER(unit)->chip_string)
#define SOC_ORIGIN(unit)                (SOC_DRIVER(unit)->origin)
#define SOC_PCI_VENDOR(unit)            (SOC_DRIVER(unit)->pci_vendor)
#define SOC_PCI_DEVICE(unit)            (SOC_DRIVER(unit)->pci_device)
#define SOC_PCI_REVISION(unit)          (SOC_DRIVER(unit)->pci_revision)

#define SOC_REG_ADDR_STAGE(addr)        ((addr >> 26) & 0x3F)
#define    SOC_MEM_ADDR_OFFSET(_addr)      ((_addr) & 0x000fffff)
#define    SOC_MEM_ADDR_STAGE(_addr)       (((_addr) >> 24) & 0x3f)
#ifdef BCM_EXTND_SBUS_SUPPORT
#define    SOC_MEM_ADDR_NUM_EXTENDED(_addr) (((_addr) >> 18) & 0xff)
#define    SOC_MEM_ADDR_OFFSET_EXTENDED_IPIPE(_addr) ((_addr) & 0x0003ffff)
#define    SOC_MEM_ADDR_OFFSET_EXTENDED(_addr) ((_addr) & 0x03ffffff)
#define    SOC_MEM_ADDR_STAGE_EXTENDED(_addr) (((_addr) >> 26) & 0x3f)
#endif

#define SOC_BLOCK2SCH(unit, blk)        (SOC_BLOCK_INFO(unit,blk).schan)
#define SOC_BLOCK2OFFSET(unit, blk)     (SOC_BLOCK_INFO(unit,blk).cmic)

#define SOC_PERSIST(unit)               (soc_persist[unit])

#define SOC_IRQ_MASK(unit)              (SOC_CONTROL(unit)->irq_mask)
#define SOC_IRQ1_MASK(unit)             (SOC_CONTROL(unit)->irq1_mask)
#define SOC_IRQ2_MASK(unit)             (SOC_CONTROL(unit)->irq2_mask)

#define SOC_SWITCH_EVENT_NOMINAL_STORM(unit)    (SOC_CONTROL(unit)->switch_event_nominal_storm)

#ifdef BCM_CMICM_SUPPORT
#define SOC_CMCS_NUM_MAX                (5)
#define SOC_CMCx_NUM_SBUSDMA            (3)
#define SOC_CMCS_NUM(unit)              (SOC_CONTROL(unit)->cmc_num)
#define SOC_PCI_CMC(unit)               (SOC_CONTROL(unit)->pci_cmc)
#define SOC_ARM_CMC(unit, arm)          (SOC_CONTROL(unit)->arm_cmc[(arm)])
#define SOC_CMCx_IRQ0_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq0_mask[cmc])
#define SOC_CMCx_IRQ1_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq1_mask[cmc])
#define SOC_CMCx_IRQ2_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq2_mask[cmc])
#define SOC_CMCx_IRQ3_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq3_mask[cmc])
#define SOC_CMCx_IRQ4_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq4_mask[cmc])
#define CPU_ARM_QUEUE_BITMAP(unit,cmc)  (SOC_CONTROL(unit)->cpu_arm_queues[cmc])
#define CPU_ARM_RSVD_QUEUE_BITMAP(unit,cmc)  (SOC_CONTROL(unit)->cpu_arm_reserved_queues[cmc])
#define NUM_CPU_ARM_COSQ(unit, cmc)     (SOC_CONTROL(unit)->num_cpu_arm_cosq[cmc])

#ifdef BCM_RCPU_SUPPORT
#define SOC_RCPU_IRQ0_MASK(unit)        (SOC_CONTROL(unit)->rpe_irq0_mask)
#define SOC_RCPU_IRQ1_MASK(unit)        (SOC_CONTROL(unit)->rpe_irq1_mask)
#define SOC_RCPU_IRQ2_MASK(unit)        (SOC_CONTROL(unit)->rpe_irq2_mask)
#define SOC_RCPU_IRQ3_MASK(unit)        (SOC_CONTROL(unit)->rpe_irq3_mask)
#define SOC_RCPU_IRQ4_MASK(unit)        (SOC_CONTROL(unit)->rpe_irq4_mask)
#define SOC_RCPU_CMC0_IRQ0_MASK(unit)   (SOC_CONTROL(unit)->rpe_pcie_irq0_mask)
#define SOC_RCPU_CMC1_IRQ0_MASK(unit)   (SOC_CONTROL(unit)->rpe_uc0_irq0_mask)
#define SOC_RCPU_CMC2_IRQ0_MASK(unit)   (SOC_CONTROL(unit)->rpe_uc1_irq0_mask)
#endif /* BCM_RCPU_SUPPORT */

#ifdef BCM_SBUSDMA_SUPPORT
#define SOC_SBUSDMA_INFO(unit)          (SOC_CONTROL(unit)->sbd_dm_inf)
#endif
#endif /* BCM_CMICM_SUPPORT */
#ifdef BCM_ISM_SUPPORT
#define SOC_ISM_INFO(unit)              (SOC_CONTROL(unit)->ism)
#define SOC_ISM_INFO_MAX_BANKS(unit)    (SOC_ISM_INFO(unit)->max_banks)
#define SOC_ISM_HASH_INFO(unit)         (SOC_CONTROL(unit)->ism_hash)
#endif /* BCM_ISM_SUPPORT */
#ifdef BCM_DDR3_SUPPORT
#define SOC_DDR3_NUM_COLUMNS(unit)           (SOC_CONTROL(unit)->ddr3_num_columns)
#define SOC_DDR3_NUM_ROWS(unit)              (SOC_CONTROL(unit)->ddr3_num_rows)
#define SOC_DDR3_NUM_BANKS(unit)             (SOC_CONTROL(unit)->ddr3_num_banks)
#define SOC_DDR3_NUM_MEMORIES(unit)          (SOC_CONTROL(unit)->ddr3_num_memories)
#define SOC_DDR3_CLOCK_MHZ(unit)             (SOC_CONTROL(unit)->ddr3_clock_mhz)
#define SOC_DDR3_MEM_GRADE(unit)             (SOC_CONTROL(unit)->ddr3_mem_grade)
#define SOC_DDR3_OFFSET_WR_DQ_CI02_WL0(unit) (SOC_CONTROL(unit)->ddr3_offset_wr_dq_ci02_wl0)
#define SOC_DDR3_OFFSET_WR_DQ_CI00_WL1(unit) (SOC_CONTROL(unit)->ddr3_offset_wr_dq_ci00_wl1)
#endif /* BCM_DDR3_SUPPORT */
#define SOC_IS_RCPU_UNIT(unit)          (SOC_CONTROL(unit)->remote_cpu)
#define SOC_IS_RCPU_SCHAN(unit)         (SOC_CONTROL(unit)->soc_flags & SOC_F_RCPU_SCHAN)
#define SOC_IS_RCPU_ONLY(unit)          (SOC_CONTROL(unit)->soc_flags & SOC_F_RCPU_ONLY)
#define SOC_DPORT_MAP(unit)             (SOC_CONTROL(unit)->dport_map)
#define SOC_DPORT_RMAP(unit)            (SOC_CONTROL(unit)->dport_rmap)
#define SOC_DPORT_MAP_FLAGS(unit)       (SOC_CONTROL(unit)->dport_map_flags)

/*Linkscan*/
#ifdef BCM_LINKSCAN_LOCK_PER_UNIT
#define SOC_LINKSCAN_LOCK(unit, s)                                                  \
        if (soc_feature(unit, soc_feature_linkscan_lock_per_unit)) {                \
            sal_mutex_take(SOC_CONTROL(unit)->linkscan_mutex, sal_mutex_FOREVER);   \
            s = 0;                                                                  \
        } else {                                                                    \
            s = sal_splhi();                                                        \
        }

#define SOC_LINKSCAN_UNLOCK(unit, s)                                                \
        if (soc_feature(unit, soc_feature_linkscan_lock_per_unit)) {                \
            sal_mutex_give(SOC_CONTROL(unit)->linkscan_mutex);                      \
            s = 0;                                                                  \
        } else {                                                                    \
            sal_spl(s);                                                             \
        }
#else
#define SOC_LINKSCAN_LOCK(unit, s)                                                  \
            s = sal_splhi(); 
#define SOC_LINKSCAN_UNLOCK(unit, s)                                                \
            sal_spl(s);
#endif /*BCM_LINKSCAN_LOCK_PER_UNIT*/


#ifdef BCM_TIMESYNC_SUPPORT
#define SOC_TIMESYNC_PLL_CLOCK_NS(unit) \
        (SOC_CONTROL(unit)->timesync_pll_clock_ns)
#endif /* BCM_TIMESYNC_SUPPORT */
/* rval must be 64-bit value */
#define SOC_REG_RST_VAL_GET(unit, reg, rval) \
    if(!SOC_REG_IS_ABOVE_64(unit, reg)) {\
        COMPILER_64_SET(rval, SOC_REG_INFO(unit, reg).rst_val_hi, SOC_REG_INFO(unit, reg).rst_val_lo); \
    } else { \
        COMPILER_64_SET(rval, 0, 0); \
    } 

#define SOC_REG_RST_MSK_GET(unit, reg, rmsk) \
    if(!SOC_REG_IS_ABOVE_64(unit, reg)) {\
        COMPILER_64_SET(rmsk, SOC_REG_INFO(unit, reg).rst_mask_hi, SOC_REG_INFO(unit, reg).rst_mask_lo); \
    } else { \
        COMPILER_64_SET(rmsk, 0, 0); \
    } 

/* rval for above 64-bit value */
#define SOC_REG_ABOVE_64_RST_VAL_GET(unit, reg, rval) \
    if(SOC_REG_IS_ABOVE_64(unit, reg)) {\
        SOC_REG_ABOVE_64_CLEAR(rval); \
        SHR_BITCOPY_RANGE(rval, 0, SOC_REG_ABOVE_64_INFO(unit, reg).reset, 0, SOC_REG_ABOVE_64_INFO(unit, reg).size*32); \
    } else { \
        SOC_REG_ABOVE_64_CLEAR(rval); \
        rval[0] = SOC_REG_INFO(unit, reg).rst_val_lo; \
        rval[1] = SOC_REG_INFO(unit, reg).rst_val_hi; \
    } 
    

#define SOC_REG_ABOVE_64_RST_MSK_GET(unit, reg, rmsk) \
    if(SOC_REG_IS_ABOVE_64(unit, reg)) {\
        SOC_REG_ABOVE_64_CLEAR(rmsk); \
        SHR_BITCOPY_RANGE(rmsk, 0, SOC_REG_ABOVE_64_INFO(unit, reg).mask, 0, SOC_REG_ABOVE_64_INFO(unit, reg).size*32); \
    } else { \
        SOC_REG_ABOVE_64_CLEAR(rmsk); \
        rmsk[0] = SOC_REG_INFO(unit, reg).rst_mask_lo; \
        rmsk[1] = SOC_REG_INFO(unit, reg).rst_mask_hi; \
    }
/*
 * Counter map macros
 * Assumes "ctype" is of SOC_CTR_TYPE_xxx
 */
#define SOC_HAS_CTR_TYPE(unit, ctype) \
    (SOC_DRIVER(unit)->counter_maps[ctype].cmap_base != NULL)
/* Reference to array of counters */
#define SOC_CTR_DMA_MAP(unit, ctype)  \
    (SOC_DRIVER(unit)->counter_maps[ctype])
/* Map a counter index to a register index */
#define SOC_CTR_TO_REG(unit, ctype, ind)  \
    (SOC_DRIVER(unit)->counter_maps[ctype].cmap_base[ind].reg)
#define SOC_CTR_TO_REG_IDX(unit, ctype, ind)  \
    (SOC_DRIVER(unit)->counter_maps[ctype].cmap_base[ind].index)
/* Right now, this is really per chip, not per counter type. */
#define SOC_CTR_MAP_SIZE(unit, ctype)   \
    (SOC_DRIVER(unit)->counter_maps[ctype].cmap_size)

#define SOC_REG_CTR_IDX(unit, reg) \
    (SOC_REG_INFO(unit, reg).ctr_idx)

#define SOC_REG_NUMELPORTLIST_IDX(unit, reg) \
    (SOC_REG_INFO(unit, reg).numelportlist_idx)

#ifdef BCM_ROBO_SUPPORT
#define SOC_MEM_IS_VALID(unit, mem) \
    ((mem >= 0 && mem < (SOC_IS_ROBO(unit) ? NUM_SOC_ROBO_MEM : NUM_SOC_MEM)) && \
     (SOC_CONTROL(unit) != NULL) && \
     (SOC_DRIVER(unit) != NULL) && \
     (SOC_MEM_PTR(unit, mem) != NULL) && \
     (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_VALID))
#else /* !BCM_ROBO_SUPPORT */
#define SOC_MEM_IS_VALID(unit, mem) \
    ((mem >= 0 && mem < NUM_SOC_MEM) && \
     (SOC_CONTROL(unit) != NULL) && \
     (SOC_DRIVER(unit) != NULL) && \
     (SOC_MEM_PTR(unit, mem) != NULL) && \
     (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_VALID))
#endif /* BCM_ROBO_SUPPORT */

#define SOC_MEM_IS_ENABLED(unit, mem) \
    (SOC_MEM_IS_VALID(unit, mem) && \
     !(SOC_MEM_INFO(unit, mem).flags & \
       SOC_CONTROL(unit)->disabled_mem_flags))

#define SOC_MEM_IS_ARRAY(unit, mem)     \
    (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_IS_ARRAY)
#define SOC_MEM_IS_ARRAY_SAFE(unit, mem)     \
    ( SOC_MEM_IS_ARRAY(unit, mem) && SOC_MEM_ARRAY_INFOP(unit, mem) )
#define SOC_MEM_ELEM_SKIP(unit, mem)     \
    (SOC_MEM_ARRAY_INFO(unit, mem).element_skip)
#define SOC_MEM_NUMELS(unit, mem)     \
    (SOC_MEM_ARRAY_INFO(unit, mem).numels)
#define SOC_MEM_ELEM_SKIP_SAFE(unit, mem)     \
    ( SOC_MEM_ARRAY_INFOP(unit, mem) ? SOC_MEM_ARRAY_INFOP(unit, mem)->element_skip : 0 )
#define SOC_MEM_NUMELS_SAFE(unit, mem)     \
    ( SOC_MEM_ARRAY_INFOP(unit, mem) ? SOC_MEM_ARRAY_INFOP(unit, mem)->numels : 1 )


#ifdef BCM_ROBO_SUPPORT
#define SOC_REG_IS_VALID(unit, reg) \
    ((reg >= 0 && reg < (SOC_IS_ROBO(unit) ? NUM_SOC_ROBO_REG : NUM_SOC_REG)) && \
     (SOC_REG_PTR(unit, reg) != NULL))
#else /* !BCM_ROBO_SUPPORT */
#define SOC_REG_IS_VALID(unit, reg) \
    ((reg >= 0 && reg < NUM_SOC_REG) && \
     (SOC_REG_PTR(unit, reg) != NULL))
#endif /* BCM_ROBO_SUPPORT */

#define SOC_MAX_COUNTER_NUM(unit)  \
        ((IS_FE_PORT(unit,0)) ?  SOC_CTR_MAP_SIZE(unit,SOC_CTR_TYPE_FE) : \
        SOC_CTR_MAP_SIZE(unit, SOC_CTR_TYPE_GE))


#define SOC_REG_IS_ENABLED(unit, reg) \
    (SOC_REG_IS_VALID(unit, reg) && \
     !(SOC_REG_INFO(unit, reg).flags & \
       SOC_CONTROL(unit)->disabled_reg_flags))

#define SOC_MEM_BYTES(unit, mem)        (SOC_MEM_INFO(unit, mem).bytes)
#define SOC_MEM_WORDS(unit, mem)        (BYTES2WORDS(SOC_MEM_BYTES(unit, mem)))

#define SOC_MEM_BASE(unit, mem)     (SOC_MEM_INFO(unit, mem).base)
#define SOC_MEM_SIZE(unit, mem) \
    (SOC_MEM_INFO(unit, mem).index_max - \
     SOC_MEM_INFO(unit, mem).index_min + 1)
#define SOC_MEM_TABLE_BYTES(unit, mem) \
    (4 * SOC_MEM_WORDS(unit, mem) * \
     (SOC_MEM_INFO(unit, mem).index_max - \
      SOC_MEM_INFO(unit, mem).index_min + 1))

#define SOC_REG_TYPE(unit, reg)      \
    (SOC_REG_INFO(unit, reg).regtype)

#define SOC_REG_IS_WRITE_ONLY(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_WO)

#define SOC_REG_IS_ABOVE_64(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_ABOVE_64_BITS)

#define SOC_REG_IS_64(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_64_BITS)

#define SOC_REG_IS_ABOVE_32(unit, reg) \
    (SOC_REG_INFO(unit, reg).flags & (SOC_REG_FLAG_64_BITS | SOC_REG_FLAG_ABOVE_64_BITS))

#define SOC_REG_IS_32(unit, reg) \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_32_BITS)

#define SOC_REG_IS_16(unit, reg) \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_16_BITS)

#define SOC_REG_IS_8(unit, reg) \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_8_BITS)

/* Register requires special processing */
#define SOC_REG_IS_SPECIAL(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_SPECIAL)

#define SOC_REG_ARRAY(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_ARRAY)
#define SOC_REG_ARRAY2(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_ARRAY2)
#define SOC_REG_ARRAY4(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_ARRAY4)
#define SOC_REG_IS_ARRAY(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_REG_ARRAY)
#define SOC_REG_BASE(unit, reg)     \
    (SOC_REG_INFO(unit, reg).offset)
#define SOC_REG_NUMELS(unit, reg)     \
    (SOC_REG_INFO(unit, reg).numels)
#define SOC_REG_GRAN(unit, reg)     \
    (soc_regtype_gran[SOC_REG_INFO(unit, reg).regtype])
#define SOC_REG_PAGE(unit, reg) \
    (SOC_REG_INFO(unit, reg).page)
#define SOC_REG_ELEM_SKIP(unit, reg)     \
    (SOC_REG_ARRAY_INFO(unit, reg).element_skip)

#define SOC_CHIP_TYPE(unit)     (SOC_INFO(unit).driver_type)
#define SOC_CHIP_GROUP(unit)    (SOC_INFO(unit).driver_group)

#define SOC_IS_DIRECT_PORT(unit, port) \
    (port>=SOC_INFO(unit).physical_port_offset && \
           SOC_PBMP_MEMBER(SOC_INFO(unit).physical_pbm, port-SOC_INFO(unit).physical_port_offset+1))    

/*
 * SOC_IS_* Macros.  If support for the chip is defined out of the
 * build, they are defined as zero to let the optimizer remove
 * code.
 */

/* Not supported in this version of SDK */
#define SOC_IS_DRACO1(unit)     (0)
#define SOC_IS_DRACO15(unit)    (0)
#define SOC_IS_DRACO(unit)      (0)
#define SOC_IS_HERCULES1(unit)  (0)
#define SOC_IS_LYNX(unit)       (0)
#define SOC_IS_TUCANA(unit)     (0)
#define SOC_IS_XGS12_SWITCH(unit) (0)

#ifdef  BCM_HERCULES_SUPPORT
#ifdef  BCM_HERCULES15_SUPPORT
#define SOC_IS_HERCULES15(unit) ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HERCULES15))
#else
#define SOC_IS_HERCULES15(unit) (0)
#endif
#define SOC_IS_HERCULES(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HERCULES))
#else
#define SOC_IS_HERCULES15(unit) (0)
#define SOC_IS_HERCULES(unit)   (0)
#endif

#ifdef  BCM_FIREBOLT_SUPPORT
#define SOC_IS_FIREBOLT(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FIREBOLT))
#define SOC_IS_FB(unit)         ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FB))
#define SOC_IS_FB_FX_HX(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FB_FX_HX))
#define SOC_IS_HB_GW(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HB_GW))
#define SOC_IS_HBX(unit)        ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HBX))
#define SOC_IS_FBX(unit)        ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip & SOC_INFO_CHIP_FBX) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2) ||\
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND)))

#else
#define SOC_IS_FIREBOLT(unit)   (0)
#define SOC_IS_FB(unit)         (0)
#define SOC_IS_FB_FX_HX(unit)   (0)
#define SOC_IS_HB_GW(unit)      (0)
#define SOC_IS_HBX(unit)        (0)
#define SOC_IS_FBX(unit)        (0)
#endif

#ifdef  BCM_HURRICANE2_SUPPORT
#define SOC_IS_HURRICANE2(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2))
#else
#define SOC_IS_HURRICANE2(unit)    (0)
#endif

#ifdef  BCM_GREYHOUND_SUPPORT
#define SOC_IS_GREYHOUND(unit)  ((!SOC_INFO(unit).spi_device) && \
        (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND))
#else
#define SOC_IS_GREYHOUND(unit)    (0)
#endif

#ifdef  BCM_HELIX_SUPPORT
#define SOC_IS_HELIX1(unit)     ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FELIX))
#define SOC_IS_HELIX15(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HELIX15))
#define SOC_IS_HELIX(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                ((SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HELIX) || \
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HELIX15)))
#else
#define SOC_IS_HELIX(unit)      (0)
#define SOC_IS_HELIX1(unit)     (0)
#define SOC_IS_HELIX15(unit)    (0)
#endif
#ifdef  BCM_FELIX_SUPPORT
#define SOC_IS_FELIX1(unit)     ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FELIX))
#define SOC_IS_FELIX15(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FELIX15))
#define SOC_IS_FELIX(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                ((SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FELIX) || \
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FELIX15)))
#else
#define SOC_IS_FELIX(unit)      (0)
#define SOC_IS_FELIX1(unit)     (0)
#define SOC_IS_FELIX15(unit)    (0)
#endif
#ifdef  BCM_RAPTOR_SUPPORT
#define SOC_IS_RAPTOR(unit)     ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_RAPTOR))
#define SOC_IS_RAVEN(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_RAVEN))
#define SOC_IS_HAWKEYE(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HAWKEYE))
#else
#define SOC_IS_RAPTOR(unit)     (0)
#define SOC_IS_RAVEN(unit)      (0)
#define SOC_IS_HAWKEYE(unit)    (0)
#endif
#if defined(BCM_HELIX_SUPPORT) || defined(BCM_FELIX_SUPPORT)
#define SOC_IS_FX_HX(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FX_HX))
#else
#define SOC_IS_FX_HX(unit)      (0)
#endif

#ifdef  BCM_GOLDWING_SUPPORT
#define SOC_IS_GOLDWING(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_GOLDWING))
#else
#define SOC_IS_GOLDWING(unit)   (0)
#endif

#ifdef  BCM_HUMV_SUPPORT
#define SOC_IS_HUMV(unit)       ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HUMV))
#else
#define SOC_IS_HUMV(unit)       (0)
#endif

#ifdef  BCM_BRADLEY_SUPPORT
#define SOC_IS_BRADLEY(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_BRADLEY))
#else
#define SOC_IS_BRADLEY(unit)    (0)
#endif

#ifdef  BCM_FIREBOLT2_SUPPORT
#define SOC_IS_FIREBOLT2(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FIREBOLT2))
#else
#define SOC_IS_FIREBOLT2(unit)  (0)
#endif

#ifdef  BCM_TRIUMPH_SUPPORT
#define SOC_IS_TRIUMPH(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIUMPH))
#define SOC_IS_ENDURO(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_ENDURO))
#define SOC_IS_HURRICANE(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE))
#define SOC_IS_HURRICANEX(unit)   ((!SOC_INFO(unit).spi_device) && \
                    ((SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE) || \
                     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2)))
#define SOC_IS_TR_VL(unit)      ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TR_VL) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2) || \
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND)))

#else
#define SOC_IS_TRIUMPH(unit)    (0)
#define SOC_IS_ENDURO(unit)     (0)
#define SOC_IS_HURRICANE(unit)  (0)
#define SOC_IS_HURRICANEX(unit) (0)
#define SOC_IS_TR_VL(unit)      (0)
#endif

#ifdef  BCM_VALKYRIE_SUPPORT
#define SOC_IS_VALKYRIE(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_VALKYRIE))
#else
#define SOC_IS_VALKYRIE(unit)   (0)
#endif

#ifdef  BCM_SCORPION_SUPPORT
#define SOC_IS_SCORPION(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_SCORPION))
#define SOC_IS_SC_CQ(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_SC_CQ))
#else
#define SOC_IS_SCORPION(unit)   (0)
#define SOC_IS_SC_CQ(unit)      (0)
#endif

#ifdef  BCM_CONQUEROR_SUPPORT
#define SOC_IS_CONQUEROR(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_CONQUEROR))
#else
#define SOC_IS_CONQUEROR(unit)  (0)
#endif

#ifdef  BCM_TRIUMPH2_SUPPORT
#define SOC_IS_TRIUMPH2(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIUMPH2))
#else
#define SOC_IS_TRIUMPH2(unit)   (0)
#endif

#ifdef  BCM_APOLLO_SUPPORT
#define SOC_IS_APOLLO(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_APOLLO))
#else
#define SOC_IS_APOLLO(unit)     (0)
#endif

#ifdef  BCM_VALKYRIE2_SUPPORT
#define SOC_IS_VALKYRIE2(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_VALKYRIE2))
#else
#define SOC_IS_VALKYRIE2(unit)  (0)
#endif

#ifdef  BCM_SIRIUS_SUPPORT
#define SOC_IS_SIRIUS(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_SIRIUS))
#else
#define SOC_IS_SIRIUS(unit)     (0)
#endif

#ifdef  BCM_CALADAN3_SUPPORT
#define SOC_IS_CALADAN3(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_CALADAN3))
#else
#define SOC_IS_CALADAN3(unit)   (0)
#endif

#ifdef  BCM_TRIDENT_SUPPORT
#define SOC_IS_TRIDENT(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIDENT))
#define SOC_IS_TITAN(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TITAN))
#define SOC_IS_TD_TT(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TD_TT))
#else
#define SOC_IS_TRIDENT(unit)    (0)
#define SOC_IS_TITAN(unit)      (0)
#define SOC_IS_TD_TT(unit)      (0)
#endif

#ifdef  BCM_SHADOW_SUPPORT
#define SOC_IS_SHADOW(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_SHADOW))
#else
#define SOC_IS_SHADOW(unit)     (0)
#endif

#ifdef  BCM_TRIUMPH3_SUPPORT
#define SOC_IS_TRIUMPH3(unit)  ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIUMPH3) || \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4)))
#else
#define SOC_IS_TRIUMPH3(unit)   (0)
#endif

#ifdef  BCM_HELIX4_SUPPORT
#define SOC_IS_HELIX4(unit) (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4)
#else
#define SOC_IS_HELIX4(unit)     (0)
#endif

#ifdef  BCM_KATANA_SUPPORT
#define SOC_IS_KATANA(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA))
#define SOC_IS_KATANAX(unit)   ((!SOC_INFO(unit).spi_device) && \
                    ((SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA) || \
                     (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2)))
#else
#define SOC_IS_KATANA(unit)     (0)
#define SOC_IS_KATANAX(unit)    (0)
#endif

#ifdef  BCM_KATANA2_SUPPORT
#define SOC_IS_KATANA2(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2))
#else
#define SOC_IS_KATANA2(unit)    (0)
#endif

#ifdef  BCM_TRIDENT2_SUPPORT
#define SOC_IS_TRIDENT2(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIDENT2))
#define SOC_IS_TITAN2(unit)     ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TITAN2))
#define SOC_IS_TD2_TT2(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TD2_TT2))
#else
#define SOC_IS_TRIDENT2(unit)   (0)
#define SOC_IS_TITAN2(unit)     (0)
#define SOC_IS_TD2_TT2(unit)    (0)
#endif

#ifdef BCM_TRX_SUPPORT
#define SOC_IS_TRX(unit)          ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TRX) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2)||\
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND)))

#else
#define SOC_IS_TRX(unit)        (0)
#endif

#ifdef  BCM_XGS_SUPPORT
#define SOC_IS_XGS(unit)        ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2) ||\
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND)))

#else
#define SOC_IS_XGS(unit)        (0)
#endif
#ifdef  BCM_XGS_FABRIC_SUPPORT
#define SOC_IS_XGS_FABRIC(unit) ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS_FABRIC))
#else
#define SOC_IS_XGS_FABRIC(unit) (0)
#endif
#ifdef  BCM_XGS_SWITCH_SUPPORT
#define SOC_IS_XGS_SWITCH(unit) ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS_SWITCH) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2)||\
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND)))

#else
#define SOC_IS_XGS_SWITCH(unit) (0)
#endif
#ifdef  BCM_XGS12_FABRIC_SUPPORT
#define SOC_IS_XGS12_FABRIC(unit) ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS12_FABRIC))
#else
#define SOC_IS_XGS12_FABRIC(unit) (0)
#endif
#ifdef  BCM_XGS3_SWITCH_SUPPORT
#define SOC_IS_XGS3_SWITCH(unit)  ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS3_SWITCH) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2)||\
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND)))
#else
#define SOC_IS_XGS3_SWITCH(unit) (0)
#endif
#ifdef  BCM_XGS3_FABRIC_SUPPORT
#define SOC_IS_XGS3_FABRIC(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS3_FABRIC))
#else
#define SOC_IS_XGS3_FABRIC(unit) (0)
#endif
#ifdef  BCM_ROBO_SUPPORT
#define SOC_IS_ROBO(unit)        ((SOC_INFO(unit).spi_device) && \
                SOC_IS_ROBO_ARCH(unit))

#define SOC_IS_ROBO5325(unit) (0)
                                                            
#else
#define SOC_IS_ROBO(unit)        (0)
#define SOC_IS_ROBO_GE_SWITCH(unit) (0)
#endif

#ifdef  BCM_EA_SUPPORT
#ifdef  BCM_TK371X_SUPPORT
#define SOC_IS_TK371X(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TK371X))
#else
#define SOC_IS_TK371X(unit) (0)
#endif /* BCM_TK371X_SUPPORT */
#define SOC_IS_EA(unit) (SOC_IS_TK371X(unit))
#else
#define SOC_IS_EA(unit)      (0)
#define SOC_IS_TK371X(unit)  (0)
#endif  /* BCM_EA_SUPPORT */

#ifdef  BCM_PETRAB_SUPPORT
#define SOC_IS_PETRAB(unit) (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_PETRAB)
#else
#define SOC_IS_PETRAB(unit) (0)
#endif

#ifdef BCM_ARAD_SUPPORT
#define SOC_IS_ARDON(unit)        (SOC_INFO(unit).chip_type   == SOC_INFO_CHIP_TYPE_ARDON)
#define SOC_IS_ARADPLUS(unit)     ((SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_ARADPLUS) || (SOC_IS_ARDON(unit)) || (SOC_IS_JERICHO(unit)))
#define SOC_IS_ARAD(unit)        ((SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_ARAD) || (SOC_IS_ARADPLUS(unit))) /* SOC_IS_ARADPLUS already 
                                                                                                                includes all chips above it. */
#if 1
#define SOC_IS_ARAD_A0(unit)           (SOC_IS_ARAD(unit)     &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88650_A0))
#define SOC_IS_ARAD_B0(unit)           (SOC_IS_ARAD(unit)     &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88650_B0))
#define SOC_IS_ARAD_B1(unit)           (SOC_IS_ARAD(unit)     &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88650_B1))
#define SOC_IS_ARADPLUS_A0(unit)       (SOC_IS_ARADPLUS(unit) &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88660_A0))
#define SOC_IS_ARDON_A0(unit)          (SOC_IS_ARDON(unit)    &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88202_A0))
#endif
#define SOC_IS_ARAD_B0_AND_ABOVE(unit)  (SOC_IS_ARAD(unit)     && (!SOC_IS_ARAD_A0(unit))) 
#define SOC_IS_ARAD_B1_AND_BELOW(unit)  (SOC_IS_ARAD(unit)     && (!SOC_IS_ARADPLUS(unit)))
#define SOC_IS_ARADPLUS_AND_BELOW(unit) (SOC_IS_ARAD(unit)     && (!SOC_IS_JERICHO(unit)))
#define SOC_IS_ACP(unit)        (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_ACP)
#else
#define SOC_IS_ARAD(unit)               (0)
#define SOC_IS_ARADPLUS(unit)           (0) 
#define SOC_IS_ARDON(unit)              (0)
#define SOC_IS_ARAD_A0(unit)            (0)
#define SOC_IS_ARAD_B0(unit)            (0)
#define SOC_IS_ARAD_B1(unit)            (0)
#define SOC_IS_ARADPLUS_A0(unit)        (0)
#define SOC_IS_ARAD_B0_AND_ABOVE(unit)  (0)
#define SOC_IS_ARAD_B1_AND_BELOW(unit)  (0)
#define SOC_IS_ARADPLUS_AND_BELOW(unit) (0)
#define SOC_IS_ACP(unit)                (0)
#endif

#ifdef BCM_JERICHO_SUPPORT
#define SOC_IS_JERICHO(unit)      (SOC_INFO(unit).chip_type   == SOC_INFO_CHIP_TYPE_JERICHO)
#define SOC_IS_JERICHO_A0(unit)   (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88670_A0)
#define SOC_IS_QMX(unit) (0)
#else
#define SOC_IS_JERICHO(unit)      (0)
#define SOC_IS_JERICHO_A0(unit)   (0)
#define SOC_IS_QMX(unit) (0)
#endif

/* JERICHO-2-P3 */
#ifdef BCM_JERICHO_P3_SUPPORT
#define SOC_IS_JERICHO_P3(unit)     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_JERICHO_P3)
#else
#define SOC_IS_JERICHO_P3(unit)              (0)
#endif


#ifdef BCM_DFE_SUPPORT
#define SOC_IS_FE1600(unit) (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FE1600)
#define SOC_IS_FE1600_REDUCED(unit) (CMDEV(unit).dev.dev_id == BCM88755_DEVICE_ID)
#define SOC_IS_BCM88754_A0(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88754_A0) 
#define SOC_IS_FE3200(unit) (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FE3200)
#if 1
#define SOC_IS_FE1600_A0(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88750_A0)
#define SOC_IS_FE1600_B0(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88750_B0 || SOC_INFO(unit).driver_type == SOC_CHIP_BCM88754_A0)
#define SOC_IS_FE3200_A0(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88950_A0)
#define SOC_IS_DFE_TYPE(dev_type) ((dev_type) == BCM88750_DEVICE_ID || (dev_type) == BCM88754_DEVICE_ID  || (dev_type) == BCM88755_DEVICE_ID || (dev_type) == BCM88950_DEVICE_ID ) 
#endif

#define SOC_IS_FE1600_B0_AND_ABOVE(unit) (SOC_IS_FE1600(unit) && !SOC_IS_FE1600_A0(unit))
#else /*!BCM_DFE_SUPPORT*/
#define SOC_IS_FE1600_A0(unit)          (0)
#define SOC_IS_FE1600_B0(unit)          (0)
#define SOC_IS_FE1600_B0_AND_ABOVE(unit)(0)
#define SOC_IS_FE1600(unit)             (0)
#define SOC_IS_FE3200(unit)             (0)
#define SOC_IS_FE3200_A0(unit)          (0)
#define SOC_IS_DFE_TYPE(dev_type)       (0)
#define SOC_IS_FE1600_REDUCED(unit)     (0)
#define SOC_IS_BCM88754_A0(unit) 		(0)
#endif /*BCM_DFE_SUPPORT*/

#define SOC_IS_DPP(unit) (SOC_IS_PETRAB(unit) || SOC_IS_ARAD(unit) || SOC_IS_ACP(unit) || SOC_IS_JERICHO_P3(unit) || SOC_IS_ARDON(unit))
#define SOC_IS_DFE(unit) (SOC_IS_FE1600(unit) || SOC_IS_FE3200(unit))
#define SOC_IS_SAND(unit) (SOC_IS_DPP(unit) || SOC_IS_DFE(unit))

#define SOC_IS_DPP_DRC_COMBO28(unit) (SOC_IS_ARDON(unit) || SOC_IS_JERICHO(unit))
            
/*
 * Note: ESW and ROBO devices need similar SOC_IS definitions
 *
 * SOC_IS_* macros for SBX devices
 */
typedef enum {
    SOC_INFO_CHIP_TYPE_HERCULES15 = 1, /* only use non-zero values to be able to skip further compares */ 
    SOC_INFO_CHIP_TYPE_FIREBOLT,
    SOC_INFO_CHIP_TYPE_FELIX,
    SOC_INFO_CHIP_TYPE_HELIX,       /* 5630x */
    SOC_INFO_CHIP_TYPE_GOLDWING,
    SOC_INFO_CHIP_TYPE_HUMV,
    SOC_INFO_CHIP_TYPE_BRADLEY,
    SOC_INFO_CHIP_TYPE_RAPTOR,
    SOC_INFO_CHIP_TYPE_FELIX15,     /* 5611x */
    SOC_INFO_CHIP_TYPE_HELIX15,     /* 5631x */
    SOC_INFO_CHIP_TYPE_FIREBOLT2,
    SOC_INFO_CHIP_TYPE_TRIUMPH,
    SOC_INFO_CHIP_TYPE_SHADOW,
    SOC_INFO_CHIP_TYPE_TRIUMPH3,
    SOC_INFO_CHIP_TYPE_HELIX4,
    SOC_INFO_CHIP_TYPE_RAVEN,
    SOC_INFO_CHIP_TYPE_GREYHOUND,
    SOC_INFO_CHIP_TYPE_QE2000,
    SOC_INFO_CHIP_TYPE_BME3200,
    SOC_INFO_CHIP_TYPE_FE2000,
    SOC_INFO_CHIP_TYPE_FE2000XT,
    SOC_INFO_CHIP_TYPE_BM9600,
    SOC_INFO_CHIP_TYPE_CALADAN3,
    SOC_INFO_CHIP_TYPE_PETRAB,
    SOC_INFO_CHIP_TYPE_ARAD,
    SOC_INFO_CHIP_TYPE_ARADPLUS,
    SOC_INFO_CHIP_TYPE_JERICHO,
    SOC_INFO_CHIP_TYPE_ARDON,
    /* JERICHO-2-P3 */
    SOC_INFO_CHIP_TYPE_JERICHO_P3,
    SOC_INFO_CHIP_TYPE_FE1600,
    SOC_INFO_CHIP_TYPE_FE3200,
    SOC_INFO_CHIP_TYPE_TK371X,
    SOC_INFO_CHIP_TYPE_TKDUMMY,
    SOC_INFO_CHIP_TYPE_SIRIUS,
    SOC_INFO_CHIP_TYPE_SCORPION,
    SOC_INFO_CHIP_TYPE_ACP
} soc_chip_e;

#ifdef BCM_SBX_SUPPORT
#define SBX_CHIPS(unit) \
        ((SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FE2000) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FE2000XT) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_QE2000) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_BME3200) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_BM9600) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_CALADAN3)) 
#define SOC_IS_ESW(unit) (!SOC_IS_ROBO(unit) && !SBX_CHIPS(unit) && \
                          !SOC_IS_SIRIUS(unit) && !SOC_IS_SAND(unit))
#else
#define SOC_IS_ESW(unit) (!SOC_IS_ROBO(unit) && !SOC_IS_SIRIUS(unit) &&\
                          !SOC_IS_CALADAN3(unit) && !SOC_IS_SAND(unit))
#endif

/* Some devices need a register updated before starting a DMA TX operation */
#if     defined(BCM_DRACO1_SUPPORT) || \
        defined(BCM_LYNX_SUPPORT) || \
        defined(BCM_TUCANA_SUPPORT)
#define SOC_TX_PKT_PROPERTIES
#endif


#define SOC_UNIT_NUM_VALID(unit)        (((unit) >= 0) &&                  \
                                         ((unit) < SOC_MAX_NUM_DEVICES))

#define SOC_UNIT_VALID(unit)            (SOC_UNIT_NUM_VALID(unit) && \
                                         (SOC_CONTROL(unit) != NULL))

/* Indicates port lies in module 1.  Currently applies only to Tucana */
#ifdef  BCM_TUCANA_SUPPORT
#define SOC_PORT_MOD1(unit, port)       ((((port) & 0x20) != 0) && \
                                        (soc_feature(unit, soc_feature_mod1)))

/*
 * This macro returns the memory table (soc_mem_t) associated with
 * the given unit/port for Tucana.  It returns a memory valid for
 * other chips as well.
 */
#define SOC_PORT_MEM_TAB(unit, port)                            \
    (SOC_PORT_MOD1(unit, port) ? PORT_TAB1m : PORT_TABm)


/*
 * Macro:
 *      SOC_PORT_MOD_OFFSET
 * Purpose:
 *      Maps a port to its offset in its module on Tucana.
 *      (Lower 5 bits: port % 32)
 *      Example:   7 -> 7,   32 -> 0,  35 -> 3.
 */
#define SOC_PORT_MOD_OFFSET(unit, port) ((soc_feature(unit, soc_feature_mod1) ? \
                                         (port & 0x1f) : port))

/*
 * Read/write macros for accessing proper port table w/ proper index
 */
#define READ_PORT_TAB_MOD_CHK(unit, block, port, val_p)          \
    (SOC_PORT_MOD1(unit, port) ?                                 \
        READ_PORT_TAB1m(unit, block, (port) & 0x1f, val_p) :     \
        READ_PORT_TABm(unit, block, port, val_p))

#define WRITE_PORT_TAB_MOD_CHK(unit, block, port, val_p)         \
    (SOC_PORT_MOD1(unit, port) ?                                 \
        WRITE_PORT_TAB1m(unit, block, (port) & 0x1f, val_p) :    \
        WRITE_PORT_TABm(unit, block, port, val_p))

/* Forces port to lie in module 1.  Currently applies only to Tucana */
#define SOC_PORT_IN_MOD1(unit, port)    ((port) | 0x20)

#else /* BCM_TUCANA_SUPPORT */
#define SOC_PORT_MOD1(unit, port)       (0)
#define SOC_PORT_MEM_TAB(unit, port)    (PORT_TABm)
#define SOC_PORT_MOD_OFFSET(unit, port) (port)
#define READ_PORT_TAB_MOD_CHK(unit, block, port, val_p)          \
            (READ_PORT_TABm(unit, block, port, val_p))

#define WRITE_PORT_TAB_MOD_CHK(unit, block, port, val_p)         \
            (WRITE_PORT_TABm(unit, block, port, val_p))
#define SOC_PORT_IN_MOD1(unit, port)    (port)

#endif /* BCM_TUCANA_SUPPORT */

#ifdef BCM_ROBO_SUPPORT
#if defined(BCM_EA_SUPPORT) && defined(BCM_TK371X_SUPPORT)
#define SOC_PORT_VALID_RANGE(unit,port)      ((port) >= 0 && \
                                         (port) < (SOC_IS_ROBO(unit) ? \
                                             SOC_ROBO_MAX_NUM_PORTS : \
                            SOC_IS_EA(unit)?SOC_EA_MAX_NUM_PORTS:SOC_MAX_NUM_PORTS) )

#else
#define SOC_PORT_VALID_RANGE(unit,port) \
                            ((port) >= 0 && \
                            (port) < (SOC_IS_ROBO(unit) ? \
                                SOC_ROBO_MAX_NUM_PORTS : \
                                ((SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS) ? \
                                    SOC_MAX_NUM_PP_PORTS : SOC_MAX_NUM_PORTS)))
#endif                    
#else /* !BCM_ROBO_SUPPORT */
#define SOC_PORT_VALID_RANGE(unit,port) \
             ((port) >= 0 && \
             ((port) < ((SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS) ? \
                 SOC_MAX_NUM_PP_PORTS : SOC_MAX_NUM_PORTS)))
#endif /* BCM_ROBO_SUPPORT */

/* Holes in the port list shall be detected by port type */

#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
#define SOC_PP_PORT_VALID(unit, port) \
    ((soc_feature(unit, soc_feature_linkphy_coe) && \
        SOC_PBMP_MEMBER(SOC_INFO(unit).linkphy_pp_port_pbm, port)) || \
        (soc_feature(unit, soc_feature_subtag_coe) && \
        SOC_PBMP_MEMBER(SOC_INFO(unit).subtag_pp_port_pbm, port)))

#define SOC_PORT_VALID(unit,port)   \
    (SOC_PORT_VALID_RANGE(unit, port) && \
        ((SOC_PORT_TYPE(unit, port) != 0) || \
        SOC_PP_PORT_VALID(unit, port)))
#else
#define SOC_PP_PORT_VALID(unit, port)    (0)
#define SOC_PORT_VALID(unit,port)   ((SOC_PORT_VALID_RANGE(unit, port)) && \
                                     (SOC_PORT_TYPE(unit, port) != 0))

#endif
#define SOC_PORT_CLASS_VALID(unit, port_class) ((port_class) >= 0 && \
                                 (port_class) < SOC_INFO(unit).port_class_max)


#define SOC_PORT_NAME(unit,port)        (SOC_INFO(unit).port_name[port])
#define SOC_PORT_OFFSET(unit,port)      (SOC_INFO(unit).port_offset[port])
#define SOC_PORT_TYPE(unit,port)        (SOC_INFO(unit).port_type[port])
#define SOC_PORT_SPEED_MAX_SET(unit,port,speed)      \
    if (SOC_PORT_VALID_RANGE(unit,port)) {           \
        SOC_INFO(unit).port_speed_max[port] = speed; \
    }
#define SOC_PORT_GROUP_SET(unit,port,group)      \
    if (SOC_PORT_VALID_RANGE(unit,port)) {       \
        SOC_INFO(unit).port_group[port] = group; \
    }
#define SOC_PORT_SERDES_SET(unit,port,serdes)      \
    if (SOC_PORT_VALID_RANGE(unit,port)) {       \
        SOC_INFO(unit).port_serdes[port] = serdes; \
    }
#define SOC_PORT_NUM_LANES_SET(unit,port,num)      \
    if (SOC_PORT_VALID_RANGE(unit,port)) {         \
        SOC_INFO(unit).port_num_lanes[port] = num; \
    }
#define SOC_PORT_P2L_MAPPING_SET(unit,phy_port,port)      \
    if (SOC_PORT_VALID_RANGE(unit,phy_port)) {            \
        SOC_INFO(unit).port_p2l_mapping[phy_port] = port; \
    }
#define SOC_PORT_L2P_MAPPING_SET(unit,port,phy_port)      \
    if (SOC_PORT_VALID_RANGE(unit,port)) {                \
        SOC_INFO(unit).port_l2p_mapping[port] = phy_port; \
    }
#define SOC_PORT_P2M_MAPPING_SET(unit,phy_port,mmu_port)      \
    if (SOC_PORT_VALID_RANGE(unit,phy_port)) {                \
        SOC_INFO(unit).port_p2m_mapping[phy_port] = mmu_port; \
    }
#define SOC_PORT_M2P_MAPPING_SET(unit,mmu_port,phy_port)      \
    if (SOC_PORT_VALID_RANGE(unit,mmu_port)) {                \
        SOC_INFO(unit).port_m2p_mapping[mmu_port] = phy_port; \
    }
/* NOTE: The param 'port' is physical port in the following */
#define SOC_PORT_IDX_BLOCK(unit, port, idx) \
    (SOC_PORT_IDX_INFO(unit, port, idx).blk)
#define SOC_PORT_IDX_BINDEX(unit, port, idx) \
    (SOC_PORT_IDX_INFO(unit, port, idx).bindex)
#define SOC_PORT_BLOCK(unit, port)      SOC_PORT_IDX_BLOCK(unit, port, 0)
#define SOC_PORT_BINDEX(unit, port)     SOC_PORT_IDX_BINDEX(unit, port, 0)
#define SOC_PIPE_ANY                    -1
/*
 * In blocks with an instance per core, used to select all instance for writes, and any instance for reads.
 * In register access used as the port parameter.
 * In memory access used either as the block parameter or as <block>_BLOCK(unit, SOC_CORE_ALL).
 */
#define SOC_CORE_ALL                    _SHR_CORE_ALL
#define MAX_NUM_OF_CORES                2

/* used as the block parameter in memory access */
#define SOC_BLOCK_ANY                   -1      /* for reading */
#define SOC_BLOCK_ALL                   -1      /* for writing */
#define SOC_BLOCK_NAME(unit,blk)        ((int)blk == SOC_BLOCK_ALL ? \
                                        "*" : \
                                        SOC_INFO(unit).block_name[blk])

#define SOC_BLOCK_PORT(unit, blk)       (SOC_INFO(unit).block_port[blk])
#define SOC_BLOCK_BITMAP(unit, blk)     (SOC_INFO(unit).block_bitmap[blk])
#define SOC_BLOCK_TYPE(unit, blk)       (SOC_BLOCK_INFO(unit, blk).type)
#define SOC_BLOCK_NUMBER(unit, blk)     (SOC_BLOCK_INFO(unit, blk).number)
/* macros for broadcast blocks */
#define SOC_BLOCK_IS_BROADCAST(unit, blk)            (SOC_INFO(unit).broadcast_blocks[blk])
#define SOC_BLOCK_BROADCAST_SIZE(unit, blk)          (SOC_INFO(unit).broadcast_blocks_size[blk])
#define SOC_BLOCK_BROADCAST_MEMBER(unit, blk, index) (SOC_INFO(unit).broadcast_blocks[blk][index])

#define SOC_BLOCK_IS(blk, type) (blk[0] == type)
/* Used for regs in a single block only */ 
#define SOC_REG_BLOCK_IS(unit, reg, type) \
    SOC_BLOCK_IS(SOC_REG_INFO(unit, reg).block, type)

extern int SOC_BLOCK_IN_LIST(int *blk, int type);
/* Used for regs in multiple blocks */ 
#define SOC_REG_BLOCK_IN_LIST(unit, reg, type) \
    SOC_BLOCK_IN_LIST(SOC_REG_INFO(unit, reg).block, type)

/*
 * Macros to get the device block type, number (instance) and index
 * within block for a given port
 */
#define SOC_PORT_BLOCK_TYPE(unit, port)    \
    (SOC_BLOCK_TYPE(unit, SOC_PORT_BLOCK(unit, port)))

#define SOC_PORT_BLOCK_NUMBER(unit, port)    \
    (SOC_BLOCK_NUMBER(unit, SOC_PORT_BLOCK(unit, port)))

#define SOC_PORT_BLOCK_INDEX(unit, port)   (SOC_PORT_BINDEX(unit, port))

     
#define NUM_COS(unit)                   (SOC_DRIVER(unit)->num_cos)
#define NUM_CPU_COSQ(unit)              (SOC_INFO(unit).num_cpu_cosq)
#define NUM_CPU_COSQ_DEF                (16)
#define NUM_CPU_COSQ_MAX                (64)

                                        
#define NUM_TIME_INTERFACE(unit)        (SOC_INFO(unit).num_time_interface)

#define NUM_MODID(unit)                 (SOC_INFO(unit).modid_count)
#define SOC_MODPORT_MAX(unit)           (SOC_INFO(unit).modport_max)
#define PORT_DUALMODID_VALID(_u_, _p_) \
    if ((NUM_MODID(_u_) > 1) && ((_p_ > SOC_INFO(_u_).modport_max) || \
                                 (_p_ < -1))) \
        {return SOC_E_PORT;}       

#define SOC_BASE_MODID(unit)  SOC_CONTROL(unit)->base_modid
#define SOC_BASE_MODID_SET(unit, value) \
            SOC_CONTROL(unit)->base_modid = value

#define SOC_VLAN_XLATE_GLP_WILDCARD(unit) (SOC_INFO(unit).vlan_xlate_glp_wildcard)
#define SOC_MODID_MAX(unit)             (SOC_INFO(unit).modid_max)
#define SOC_TRUNK_BIT_POS(unit)         (SOC_INFO(unit).trunk_bit_pos)
#define SOC_PORT_ADDR_MAX(unit)         (SOC_INFO(unit).port_addr_max)
#define SOC_HG_OFFSET(unit)             (SOC_INFO(unit).hg_offset)
#define SOC_PORT_ADDRESSABLE(unit,port) (port >= 0 && port <= \
                                            SOC_INFO(unit).port_addr_max)
#define SOC_MODID_ADDRESSABLE(unit,mod) (mod >= 0 && mod <= \
                                            SOC_INFO(unit).modid_max)

#define SOC_PORT(unit,ptype,pno)        (SOC_INFO(unit).ptype.port[pno])
#define SOC_PORT_NUM(unit,ptype)        (SOC_INFO(unit).ptype.num)
#define SOC_PORT_MIN(unit,ptype)        (SOC_INFO(unit).ptype.min)
#define SOC_PORT_MAX(unit,ptype)        (SOC_INFO(unit).ptype.max)
#define SOC_PORT_BITMAP(unit,ptype)     (SOC_INFO(unit).ptype.bitmap)
#define SOC_PORT_DISABLED_BITMAP(unit,ptype) \
    (SOC_INFO(unit).ptype.disabled_bitmap)

#define NUM_OLP_PORT(unit)              (SOC_INFO(unit).olp_port[0] > 0 ? 1 : 0) 
#define NUM_ERP_PORT(unit)              (SOC_INFO(unit).erp_port[0] > 0 ? 1 : 0)
#define NUM_OAMP_PORT(unit)             (SOC_INFO(unit).oamp_port[0] > 0 ? 1 : 0)
#define NUM_RCY_PORT(unit)              SOC_PORT_NUM(unit,rcy)
#define NUM_FE_PORT(unit)               SOC_PORT_NUM(unit,fe)
#define NUM_GE_PORT(unit)               SOC_PORT_NUM(unit,ge)
#define NUM_LLID_PORT(unit)             SOC_PORT_NUM(unit,llid)
#define NUM_PON_PORT(unit)              SOC_PORT_NUM(unit,pon)
#define NUM_XE_PORT(unit)               SOC_PORT_NUM(unit,xe)
#define NUM_HG_PORT(unit)               SOC_PORT_NUM(unit,hg)
#define NUM_HG_SUBPORT_PORT(unit)       SOC_PORT_NUM(unit,hg_subport)
#define NUM_HL_PORT(unit)               SOC_PORT_NUM(unit,hl)
#define NUM_IL_PORT(unit)               SOC_PORT_NUM(unit,il)
#define NUM_SCH_PORT(unit)              SOC_PORT_NUM(unit,sch)
#define NUM_ST_PORT(unit)               SOC_PORT_NUM(unit,st)
#define NUM_GX_PORT(unit)               SOC_PORT_NUM(unit,gx)
#define NUM_XL_PORT(unit)               SOC_PORT_NUM(unit,xl)
#define NUM_MXQ_PORT(unit)              SOC_PORT_NUM(unit,mxq)
#define NUM_XG_PORT(unit)               SOC_PORT_NUM(unit,xg)
#define NUM_XQ_PORT(unit)               SOC_PORT_NUM(unit,xq)
#define NUM_HYPLITE_PORT(unit)          SOC_PORT_NUM(unit,hyplite)
#define NUM_SPI_PORT(unit)              SOC_PORT_NUM(unit,spi)
#define NUM_SPI_SUBPORT_PORT(unit)      SOC_PORT_NUM(unit,spi_subport)
#define NUM_SCI_PORT(unit)              SOC_PORT_NUM(unit,sci)
#define NUM_SFI_PORT(unit)              SOC_PORT_NUM(unit,sfi)
#define NUM_REQ_PORT(unit)              SOC_PORT_NUM(unit,req)
#define NUM_FAB_PORT(unit)              SOC_PORT_NUM(unit,fab)
#define NUM_E_PORT(unit)                SOC_PORT_NUM(unit,ether)
#define NUM_LP_PORT(unit)               SOC_PORT_NUM(unit,lp)
#define NUM_SUBTAG_PORT(unit)           SOC_PORT_NUM(unit,subtag)
#define NUM_PORT(unit)                  SOC_PORT_NUM(unit,port)
#define NUM_ALL_PORT(unit)              SOC_PORT_NUM(unit,all)
#define MAX_PORT(unit)                  (SOC_INFO(unit).port_num)
#define LB_PORT(unit)                   (SOC_INFO(unit).lb_port)
#define AXP_PORT(unit, type)            (SOC_INFO(unit).axp_port[type])
#define IPIC_PORT(unit)                 (SOC_INFO(unit).ipic_port)
#define IPIC_BLOCK(unit)                (SOC_INFO(unit).ipic_block)
#define CMIC_PORT(unit)                 (SOC_INFO(unit).cmic_port)
#define CMIC_BLOCK(unit)                (SOC_INFO(unit).cmic_block)
#define RCPU_PORT(unit)                 (SOC_INFO(unit).rcpu_port)
#define ARL_BLOCK(unit)                 (SOC_INFO(unit).arl_block)
#define MMU_BLOCK(unit)                 (SOC_INFO(unit).mmu_block)
#define MCU_BLOCK(unit)                 (SOC_INFO(unit).mcu_block)
#define IPIPE_BLOCK(unit)               (SOC_INFO(unit).ipipe_block)
#define IPIPE_HI_BLOCK(unit)            (SOC_INFO(unit).ipipe_hi_block)
#define EPIPE_BLOCK(unit)               (SOC_INFO(unit).epipe_block)
#define EPIPE_HI_BLOCK(unit)            (SOC_INFO(unit).epipe_hi_block)
#define IGR_BLOCK(unit)                 (SOC_INFO(unit).igr_block)
#define EGR_BLOCK(unit)                 (SOC_INFO(unit).egr_block)
#define BSE_BLOCK(unit)                 (SOC_INFO(unit).bse_block)
#define CSE_BLOCK(unit)                 (SOC_INFO(unit).cse_block)
#define HSE_BLOCK(unit)                 (SOC_INFO(unit).hse_block)
#define BSAFE_BLOCK(unit)               (SOC_INFO(unit).bsafe_block)
#define OTPC_BLOCK(unit)                (SOC_INFO(unit).otpc_block)
#define ESM_BLOCK(unit)                 (SOC_INFO(unit).esm_block)
#define PGW_CL_BLOCK(unit, instance)    ((((instance) >= 0) && ((instance) < 8)) ? SOC_INFO(unit).pgw_cl_block[(instance)] : (-1))
#define PG4_BLOCK(unit, instance)       ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).pg4_block[(instance)] : (-1))
#define PG5_BLOCK(unit, instance)       ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).pg5_block[(instance)] : (-1))
#define EXTERN_BLOCK(unit)              (SOC_INFO(unit).exter_block)
#define TOP_BLOCK(unit)                 (SOC_INFO(unit).top_block)
#define SER_BLOCK(unit)                 (SOC_INFO(unit).ser_block)
#define AXP_BLOCK(unit)                 (SOC_INFO(unit).axp_block)
#define ISM_BLOCK(unit)                 (SOC_INFO(unit).ism_block)
#define ETU_BLOCK(unit)                 (SOC_INFO(unit).etu_block)
#define ETU_WRAP_BLOCK(unit)            (SOC_INFO(unit).etu_wrap_block)
#define IBOD_BLOCK(unit)                (SOC_INFO(unit).ibod_block)
#define LLS_BLOCK(unit)                 (SOC_INFO(unit).lls_block)
#define CES_BLOCK(unit)                 (SOC_INFO(unit).ces_block)
#define IPROC_BLOCK(unit)               (SOC_INFO(unit).iproc_block)
#define SOC_MAX_NUM_PMQ_BLKS            6
#define PMQ_BLOCK(unit, instance)       ((((instance) >= 0) && ((instance) < SOC_INFO(unit).nof_pmqs)) ? SOC_INFO(unit).pmq_block[(instance)] : (-1))
#define AVS_BLOCK(unit)                 (SOC_INFO(unit).avs_block)

#define MS_ISEC0_BLOCK(unit)            (SOC_INFO(unit).ms_isec_block[0])
#define MS_ISEC1_BLOCK(unit)            (SOC_INFO(unit).ms_isec_block[1])
#define MS_ISEC_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).ms_isec_block[(instance)] : (-1))
#define MS_ESEC0_BLOCK(unit)            (SOC_INFO(unit).ms_esec_block[0])
#define MS_ESEC1_BLOCK(unit)            (SOC_INFO(unit).ms_esec_block[1])
#define MS_ESEC_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).ms_esec_block[(instance)] : (-1))
#define IL0_BLOCK(unit)                 (SOC_INFO(unit).il_block[0])
#define IL1_BLOCK(unit)                 (SOC_INFO(unit).il_block[1])
#define IL_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).il_block[(instance)] : (-1))
#define CW_BLOCK(unit)                  (SOC_INFO(unit).cw)

#define MS_ISEC0_BLOCK(unit)            (SOC_INFO(unit).ms_isec_block[0])
#define MS_ISEC1_BLOCK(unit)            (SOC_INFO(unit).ms_isec_block[1])
#define MS_ISEC_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).ms_isec_block[(instance)] : (-1))
#define MS_ESEC0_BLOCK(unit)            (SOC_INFO(unit).ms_esec_block[0])
#define MS_ESEC1_BLOCK(unit)            (SOC_INFO(unit).ms_esec_block[1])
#define MS_ESEC_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).ms_esec_block[(instance)] : (-1))
#define IL0_BLOCK(unit)                 (SOC_INFO(unit).il_block[0])
#define IL1_BLOCK(unit)                 (SOC_INFO(unit).il_block[1])
#define IL_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).il_block[(instance)] : (-1))
#define CW_BLOCK(unit)                  (SOC_INFO(unit).cw)

#define BP_BLOCK(unit)                  (SOC_INFO(unit).bp_block)
#define CS_BLOCK(unit)                  (SOC_INFO(unit).cs_block)
#define EB_BLOCK(unit)                  (SOC_INFO(unit).eb_block)
#define EP_BLOCK(unit)                  (SOC_INFO(unit).ep_block)
#define ES_BLOCK(unit)                  (SOC_INFO(unit).es_block)
#define FD_BLOCK(unit)                  (SOC_INFO(unit).fd_block)
#define FF_BLOCK(unit)                  (SOC_INFO(unit).ff_block)
#define FR_BLOCK(unit)                  (SOC_INFO(unit).fr_block)
#define TX_BLOCK(unit)                  (SOC_INFO(unit).tx_block)
#define QMA_BLOCK(unit)                 (SOC_INFO(unit).qma_block)
#define QMB_BLOCK(unit)                 (SOC_INFO(unit).qmb_block)
#define QMC_BLOCK(unit)                 (SOC_INFO(unit).qmc_block)
#define QSA_BLOCK(unit)                 (SOC_INFO(unit).qsa_block)
#define QSB_BLOCK(unit)                 (SOC_INFO(unit).qsb_block)
#define RB_BLOCK(unit)                  (SOC_INFO(unit).rb_block)
#define SC_TOP_BLOCK(unit)              (SOC_INFO(unit).sc_top_block)
#define SF_TOP_BLOCK(unit)              (SOC_INFO(unit).sf_top_block)
#define TS_BLOCK(unit)                  (SOC_INFO(unit).ts_block)

#ifdef  BCM_CALADAN3_SUPPORT
#define SOC_MAX_NUM_CI_BLKS             (16)
#else
#define SOC_MAX_NUM_CI_BLKS             (15)
#endif

#define CI0_BLOCK(unit)                 (SOC_INFO(unit).ci_block[0])
#define CI1_BLOCK(unit)                 (SOC_INFO(unit).ci_block[1])
#define CI2_BLOCK(unit)                 (SOC_INFO(unit).ci_block[2])
#define CI3_BLOCK(unit)                 (SOC_INFO(unit).ci_block[3])
#define CI4_BLOCK(unit)                 (SOC_INFO(unit).ci_block[4])
#define CI5_BLOCK(unit)                 (SOC_INFO(unit).ci_block[5])
#define CI6_BLOCK(unit)                 (SOC_INFO(unit).ci_block[6])
#define CI7_BLOCK(unit)                 (SOC_INFO(unit).ci_block[7])
#define CI8_BLOCK(unit)                 (SOC_INFO(unit).ci_block[8])
#define CI9_BLOCK(unit)                 (SOC_INFO(unit).ci_block[9])
#define CI9_BLOCK(unit)                 (SOC_INFO(unit).ci_block[9])
#define CI10_BLOCK(unit)                (SOC_INFO(unit).ci_block[10])
#define CI11_BLOCK(unit)                (SOC_INFO(unit).ci_block[11])
#define CI12_BLOCK(unit)                (SOC_INFO(unit).ci_block[12])
#define CI13_BLOCK(unit)                (SOC_INFO(unit).ci_block[13])
#define CI14_BLOCK(unit)                (SOC_INFO(unit).ci_block[14])
#ifdef  BCM_CALADAN3_SUPPORT
#define CI15_BLOCK(unit)                (SOC_INFO(unit).ci_block[15])
#endif
#define CI_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_CI_BLKS)) ? \
                                               SOC_INFO(unit).ci_block[(instance)] : (-1))
#define SOC_MAX_NUM_XPORT_BLKS          (8)
#define XPORT0_BLOCK(unit)              (SOC_INFO(unit).xport_block[0])
#define XPORT1_BLOCK(unit)              (SOC_INFO(unit).xport_block[1])
#define XPORT2_BLOCK(unit)              (SOC_INFO(unit).xport_block[2])
#define XPORT3_BLOCK(unit)              (SOC_INFO(unit).xport_block[3])
#define XPORT4_BLOCK(unit)              (SOC_INFO(unit).xport_block[4])
#define XPORT5_BLOCK(unit)              (SOC_INFO(unit).xport_block[5])
#define XPORT6_BLOCK(unit)              (SOC_INFO(unit).xport_block[6])
#define XPORT7_BLOCK(unit)              (SOC_INFO(unit).xport_block[7])
#define XPORT_BLOCK(unit, instance)     ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_XPORT_BLKS)) ? SOC_INFO(unit).xport_block[(instance)] : (-1))

#define SOC_MAX_NUM_CLPORT_BLKS         (2)
#define CLPORT0_BLOCK(unit)              (SOC_INFO(unit).clport_block[0])
#define CLPORT1_BLOCK(unit)              (SOC_INFO(unit).clport_block[1])
#define CLPORT_BLOCK(unit, instance)     ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_XPORT_BLKS)) ? \
                                                   SOC_INFO(unit).clport_block[(instance)] : (-1))

#define SOC_MAX_NUM_XTPORT_BLKS          (3)
#define SOC_MAX_NUM_ILPORT_BLKS          (2)

#define CM_BLOCK(unit)                  (SOC_INFO(unit).cm_block)

#define SOC_MAX_NUM_COP_BLKS            (2)
#define CO0_BLOCK(unit)                 (SOC_INFO(unit).co_block[0])
#define CO1_BLOCK(unit)                 (SOC_INFO(unit).co_block[1])
#define CO_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_COP_BLKS)) ? \
                                                   SOC_INFO(unit).co_block[(instance)] : (-1))

#define CX_BLOCK(unit)                  (SOC_INFO(unit).cx_block)
#define PD_BLOCK(unit)                  (SOC_INFO(unit).pd_block)

#define SOC_MAX_NUM_PR_BLKS             (2)
#define PR0_BLOCK(unit)                 (SOC_INFO(unit).pr_block[0])
#define PR1_BLOCK(unit)                 (SOC_INFO(unit).pr_block[1])
#define PR_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_PR_BLKS)) ? \
                                                   SOC_INFO(unit).pr_block[(instance)] : (-1))

#define SOC_MAX_NUM_PT_BLKS             (2)
#define PT0_BLOCK(unit)                 (SOC_INFO(unit).pt_block[0])
#define PT1_BLOCK(unit)                 (SOC_INFO(unit).pt_block[1])
#define PT_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_PT_BLKS)) ? \
                                                   SOC_INFO(unit).pt_block[(instance)] : (-1))

#define QM_BLOCK(unit)                  (SOC_INFO(unit).qm_block)

#define SOC_MAX_NUM_TMU_BLKS            (2)
#define TMA_BLOCK(unit)                 (SOC_INFO(unit).tma_block)
#define TMB_BLOCK(unit)                 (SOC_INFO(unit).tmb_block)

#define CM_BLOCK(unit)                  (SOC_INFO(unit).cm_block)
#define PD_BLOCK(unit)                  (SOC_INFO(unit).pd_block)
#define OC_BLOCK(unit)                  (SOC_INFO(unit).oc_block)
#define RC_BLOCK(unit)                  (SOC_INFO(unit).rc_block)
#define LRA_BLOCK(unit)                 (SOC_INFO(unit).lra_block)
#define LRB_BLOCK(unit)                 (SOC_INFO(unit).lrb_block)
#define PP_BLOCK(unit)                  (SOC_INFO(unit).pp_block)
#define PB_BLOCK(unit)                  (SOC_INFO(unit).pb_block)

#define SOC_MAX_NUM_TM_QE_BLKS             (16)
#define TM_QE0_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[0])
#define TM_QE1_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[1])
#define TM_QE2_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[2])
#define TM_QE3_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[3])
#define TM_QE4_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[4])
#define TM_QE5_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[5])
#define TM_QE6_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[6])
#define TM_QE7_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[7])
#define TM_QE8_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[8])
#define TM_QE9_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[9])
#define TM_QE9_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[9])
#define TM_QE10_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[10])
#define TM_QE11_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[11])
#define TM_QE12_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[12])
#define TM_QE13_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[13])
#define TM_QE14_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[14])
#define TM_QE15_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[15])
#define TM_QE_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_TM_QE_BLKS)) ? \
                                               SOC_INFO(unit).tm_qe_block[(instance)] : (-1))
#define SOC_MAX_NUM_TP_BLKS             (2)
#define TP0_BLOCK(unit)                 (SOC_INFO(unit).tp_block[0])
#define TP1_BLOCK(unit)                 (SOC_INFO(unit).tp_block[1])
#define TP_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_TP_BLKS)) ? \
                                                   SOC_INFO(unit).tp_block[(instance)] : (-1))
#define SOC_MAX_NUM_TXLP_BLKS             (2)
#define SOC_MAX_NUM_RXLP_BLKS             (2)
#define TXLP0_BLOCK(unit)                 (SOC_INFO(unit).txlp_block[0])
#define TXLP1_BLOCK(unit)                 (SOC_INFO(unit).txlp_block[1])
#define RXLP0_BLOCK(unit)                 (SOC_INFO(unit).rxlp_block[0])
#define RXLP1_BLOCK(unit)                 (SOC_INFO(unit).rxlp_block[1])
#define TXLP_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_TXLP_BLKS)) ? \
                                               SOC_INFO(unit).txlp_block[(instance)] : (-1))
#define RXLP_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_RXLP_BLKS)) ? \
                                                   SOC_INFO(unit).rxlp_block[(instance)] : (-1))

/* DPP blocks */
#define SOC_MAX_NUM_DPP_PIPES 2
#define SOC_MAX_NUM_CGM_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_EGQ_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_EPNI_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_IHB_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_IHP_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_IPS_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_IQM_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_SCH_BLKS SOC_MAX_NUM_DPP_PIPES
#define CFC_BLOCK(unit) (SOC_INFO(unit).cfc_block)      
#define OCB_BLOCK(unit) (SOC_INFO(unit).ocb_block)         
#define CRPS_BLOCK(unit) (SOC_INFO(unit).crps_block)            
#define ECI_BLOCK(unit) (SOC_INFO(unit).eci_block)
#define EDB_BLOCK(unit) (SOC_INFO(unit).edb_block)
#define ILKN_PMH_BLOCK(unit) (SOC_INFO(unit).ilkn_pmh_block)
#define IPST_BLOCK(unit) (SOC_INFO(unit).ipst_block)
#define ILKN_PML_BLOCK(unit) (SOC_INFO(unit).ilkn_pml_block)
#define IQMT_BLOCK(unit) (SOC_INFO(unit).iqmt_block)
#define PPDB_A_BLOCK(unit) (SOC_INFO(unit).ppdb_a_block)
#define PPDB_B_BLOCK(unit) (SOC_INFO(unit).ppdb_b_block)
#define NBIL_BLOCK(unit) (SOC_INFO(unit).nbil_block)
#define NBIH_BLOCK(unit) (SOC_INFO(unit).nbih_block)
#define FCR_BLOCK(unit) (SOC_INFO(unit).fcr_block) 
#define FCT_BLOCK(unit) (SOC_INFO(unit).fct_block)
#define FDR_BLOCK(unit) (SOC_INFO(unit).fdr_block)      
#define FDA_BLOCK(unit) (SOC_INFO(unit).fda_block)      
#define FDT_BLOCK(unit) (SOC_INFO(unit).fdt_block)    
#define MESH_TOPOLOGY_BLOCK(unit) (SOC_INFO(unit).mesh_topology_block)      
#define IDR_BLOCK(unit) (SOC_INFO(unit).idr_block)            
#define IPT_BLOCK(unit) (SOC_INFO(unit).ipt_block)               
#define IRE_BLOCK(unit) (SOC_INFO(unit).ire_block)           
#define IRR_BLOCK(unit) (SOC_INFO(unit).irr_block)               
#define SOC_MAX_NUM_FMAC_BLKS 36
#define FMAC_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_FMAC_BLKS)) ? SOC_INFO(unit).fmac_block[instance] : (-1))    
#define SOC_MAX_NUM_XLP_BLKS 12        
#define XLP_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_XLP_BLKS)) ? SOC_INFO(unit).xlp_block[instance] : (-1))  
#define SOC_MAX_NUM_CLP_BLKS 6               
#define CLP_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_CLP_BLKS)) ? SOC_INFO(unit).clp_block[instance] : (-1))        
#define NBI_BLOCK(unit) (SOC_INFO(unit).nbi_block)           
#define OAMP_BLOCK(unit) (SOC_INFO(unit).oamp_block)               
#define OLP_BLOCK(unit) (SOC_INFO(unit).olp_block)      
#define SOC_MAX_NUM_FSRD_BLKS 12     
#define FSRD_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_FSRD_BLKS)) ? SOC_INFO(unit).fsrd_block[instance] : (-1))         
#define RTP_BLOCK(unit) (SOC_INFO(unit).rtp_block)               
#define DRCA_BLOCK(unit) (SOC_INFO(unit).drca_block)       
#define DRCB_BLOCK(unit) (SOC_INFO(unit).drcb_block)           
#define DRCC_BLOCK(unit) (SOC_INFO(unit).drcc_block)                 
#define DRCD_BLOCK(unit) (SOC_INFO(unit).drcd_block)            
#define DRCE_BLOCK(unit) (SOC_INFO(unit).drce_block)               
#define DRCF_BLOCK(unit) (SOC_INFO(unit).drcf_block)       
#define DRCG_BLOCK(unit) (SOC_INFO(unit).drcg_block)           
#define DRCH_BLOCK(unit) (SOC_INFO(unit).drch_block)  
#define DRCBROADCAST_BLOCK(unit) (SOC_INFO(unit).drcbroadcast_block)  
#define OCCG_BLOCK(unit) (SOC_INFO(unit).occg_block)
#define SOC_MAX_NUM_MRPS_BLKS 4
#define SOC_MAX_NUM_DCH_BLKS 4     
#define DCH_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DCH_BLKS)) ? SOC_INFO(unit).dch_block[instance] : (-1)) 
#define SOC_MAX_NUM_DCL_BLKS 4      
#define DCL_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DCL_BLKS)) ? SOC_INFO(unit).dcl_block[instance] : (-1))   
#define SOC_MAX_NUM_DCMA_BLKS 2    
#define DCMA_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DCMA_BLKS)) ? SOC_INFO(unit).dcma_block[instance] : (-1))  
#define SOC_MAX_NUM_DCMB_BLKS 2   
#define DCMB_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DCMB_BLKS)) ? SOC_INFO(unit).dcmb_block[instance] : (-1))   
#define DCMC_BLOCK(unit) (SOC_INFO(unit).dcmc_block)
#define SOC_MAX_NUM_CCS_BLKS 4
#define CCS_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_CCS_BLKS)) ? SOC_INFO(unit).ccs_block[instance] : (-1)) 
#define SOC_MAX_NUM_DCM_BLKS 4
#define DCM_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DCM_BLKS)) ? SOC_INFO(unit).dcm_block[instance] : (-1))
 
#define SOC_MAX_NUM_GPORT_BLKS 12
#define GPORT_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_GPORT_BLKS)) ? SOC_INFO(unit).gport_block[instance] : (-1))  

#define MRPS_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_MRPS_BLKS)) ? \
   SOC_INFO(unit).mrps_blocks[instance] : (-1))

#define CGM_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_CGM_BLKS)) ? \
   SOC_INFO(unit).cgm_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_cgm_block : -1))

#define IPS_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_IPS_BLKS)) ? \
   SOC_INFO(unit).ips_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_ips_block : -1))

#define IHP_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_IHP_BLKS)) ? \
   SOC_INFO(unit).ihp_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_ihp_block : -1))

#define IQM_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_IQM_BLKS)) ? \
   SOC_INFO(unit).iqm_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_iqm_block : -1))

#define SCH_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_SCH_BLKS)) ? \
   SOC_INFO(unit).sch_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_sch_block : -1))

#define EPNI_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_EPNI_BLKS)) ? \
   SOC_INFO(unit).epni_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_epni_block : -1))

#define IHB_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_IHB_BLKS)) ? \
   SOC_INFO(unit).ihb_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_ihb_block : -1))

#define EGQ_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_EGQ_BLKS)) ? \
   SOC_INFO(unit).egq_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_egq_block : -1))

#define BRDC_FMACH_BLOCK(unit)  (SOC_INFO(unit).brdc_fmach_block)
#define BRDC_FMACL_BLOCK(unit)  (SOC_INFO(unit).brdc_fmacl_block)
#define BRDC_FSRD_BLOCK(unit)   (SOC_INFO(unit).brdc_fsrd_block)
#define BRDC_FMAC_BLOCK(unit)   (SOC_INFO(unit).brdc_fmac_block)
#define BRDC_FMAC_AC_BLOCK(unit)  (SOC_INFO(unit).brdc_fmac_ac_block)
#define BRDC_FMAC_BD_BLOCK(unit)  (SOC_INFO(unit).brdc_fmac_bd_block)
#define BRDC_DCH_BLOCK(unit)  (SOC_INFO(unit).brdc_dch_block)
#define BRDC_DCL_BLOCK(unit)  (SOC_INFO(unit).brdc_dcl_block)
#define BRDC_DCM_BLOCK(unit)  (SOC_INFO(unit).brdc_dcm_block)
#define BRDC_CCS_BLOCK(unit)  (SOC_INFO(unit).brdc_ccs_block)
#define BRDC_CGM_BLOCK(unit)    (SOC_INFO(unit).brdc_cgm_block)
#define BRDC_EGQ_BLOCK(unit)    (SOC_INFO(unit).brdc_egq_block)
#define BRDC_EPNI_BLOCK(unit)   (SOC_INFO(unit).brdc_epni_block)
#define BRDC_IHB_BLOCK(unit)    (SOC_INFO(unit).brdc_ihb_block)
#define BRDC_IHP_BLOCK(unit)    (SOC_INFO(unit).brdc_ihp_block)
#define BRDC_IPS_BLOCK(unit)    (SOC_INFO(unit).brdc_ips_block)
#define BRDC_IQM_BLOCK(unit)    (SOC_INFO(unit).brdc_iqm_block)
#define BRDC_SCH_BLOCK(unit)    (SOC_INFO(unit).brdc_sch_block)

#if defined(BCM_DNX_P3_SUPPORT)
#define P3_CFG_BLOCK(unit) (SOC_INFO(unit).p3_cfg_block)
#define P3_EM_BLOCK(unit) (SOC_INFO(unit).p3_em_block)
#define P3_TABLE_BLOCK(unit) (SOC_INFO(unit).p3_table_block)
#define P3_TCAM_BLOCK(unit) (SOC_INFO(unit).p3_tcam_block)
#endif /* BCM_DNX_P3_SUPPORT*/

/*
 * E is ethernet port (FE|GE|XE)
 * PORT is all net ports (E|HG|IL)
 * ALL is all ports (PORT|CMIC)
 */
#define PBMP_FE_ALL(unit)               SOC_PORT_BITMAP(unit,fe)
#define PBMP_GE_ALL(unit)               SOC_PORT_BITMAP(unit,ge)
#define PBMP_LLID_ALL(unit)             SOC_PORT_BITMAP(unit,llid)
#define PBMP_PON_ALL(unit)              SOC_PORT_BITMAP(unit,pon)
#define PBMP_CE_ALL(unit)               SOC_PORT_BITMAP(unit,ce)
#define PBMP_XE_ALL(unit)               SOC_PORT_BITMAP(unit,xe)
#define PBMP_HG_ALL(unit)               SOC_PORT_BITMAP(unit,hg)
#define PBMP_IL_ALL(unit)               SOC_PORT_BITMAP(unit,il)
#define PBMP_SCH_ALL(unit)              SOC_PORT_BITMAP(unit,sch)
#define PBMP_HG_SUBPORT_ALL(unit)       SOC_PORT_BITMAP(unit,hg_subport)
#define PBMP_HL_ALL(unit)               SOC_PORT_BITMAP(unit,hl)
#define PBMP_ST_ALL(unit)               SOC_PORT_BITMAP(unit,st)
#define PBMP_GX_ALL(unit)               SOC_PORT_BITMAP(unit,gx)
#define PBMP_XL_ALL(unit)               SOC_PORT_BITMAP(unit,xl)
#define PBMP_XT_ALL(unit)               SOC_PORT_BITMAP(unit,xt)
#define PBMP_XW_ALL(unit)               SOC_PORT_BITMAP(unit,xw)
#define PBMP_CL_ALL(unit)               SOC_PORT_BITMAP(unit,cl)
#define PBMP_C_ALL(unit)                SOC_PORT_BITMAP(unit,c)
#define PBMP_AXP_ALL(unit)              SOC_PORT_BITMAP(unit,axp)
#define PBMP_MXQ_ALL(unit)              SOC_PORT_BITMAP(unit,mxq)
#define PBMP_XG_ALL(unit)               SOC_PORT_BITMAP(unit,xg)
#define PBMP_XQ_ALL(unit)               SOC_PORT_BITMAP(unit,xq)
#define PBMP_HYPLITE_ALL(unit)          SOC_PORT_BITMAP(unit,hyplite)
#define PBMP_SPI_ALL(unit)              SOC_PORT_BITMAP(unit,spi)
#define PBMP_SPI_SUBPORT_ALL(unit)      SOC_PORT_BITMAP(unit,spi_subport)
#define PBMP_SCI_ALL(unit)              SOC_PORT_BITMAP(unit,sci)
#define PBMP_SFI_ALL(unit)              SOC_PORT_BITMAP(unit,sfi)
#define PBMP_REQ_ALL(unit)              SOC_PORT_BITMAP(unit,req)
#define PBMP_FAB_ALL(unit)              SOC_PORT_BITMAP(unit,fab)
#define PBMP_E_ALL(unit)                SOC_PORT_BITMAP(unit,ether)
#define PBMP_LP_ALL(unit)               SOC_PORT_BITMAP(unit,lp)
#define PBMP_SUBTAG_ALL(unit)           SOC_PORT_BITMAP(unit,subtag)
#define PBMP_TDM_ALL(unit)              SOC_PORT_BITMAP(unit,tdm)
#define PBMP_RCY_ALL(unit)              SOC_PORT_BITMAP(unit,rcy)
#define PBMP_PORT_ALL(unit)             SOC_PORT_BITMAP(unit,port)
#define PBMP_CMIC(unit)                 SOC_INFO(unit).cmic_bitmap
#define PBMP_LB(unit)                   SOC_INFO(unit).lb_pbm
#define PBMP_MMU(unit)                  SOC_INFO(unit).mmu_pbm
#define PBMP_EQ(unit)                   SOC_INFO(unit).eq_pbm
#define PBMP_MANAGEMENT(unit)           SOC_INFO(unit).management_pbm
#define PBMP_OVERSUB(unit)              SOC_INFO(unit).oversub_pbm
#define PBMP_XPIPE(unit)                SOC_INFO(unit).xpipe_pbm
#define PBMP_YPIPE(unit)                SOC_INFO(unit).ypipe_pbm
#define PBMP_TDM(unit)                  SOC_INFO(unit).tdm_pbm
#define PBMP_RCY(unit)                  SOC_INFO(unit).rcy_pbm
#define PBMP_SPI(unit)                  SOC_INFO(unit).spi_bitmap
#define PBMP_PP_ALL(unit)               SOC_PORT_BITMAP(unit,pp)
#define PBMP_ALL(unit)                  SOC_PORT_BITMAP(unit,all)
#define PBMP_EXT_MEM(unit)              SOC_INFO(unit).pbm_ext_mem

/* use PORT_MIN/_MAX to be more efficient than PBMP_ITER */
#define _SOC_PBMP_ITER(_u,_pt,_p)       \
        for ((_p) = SOC_PORT_MIN(_u,_pt); \
             (_p) >= 0 && (_p) <= SOC_PORT_MAX(_u,_pt); \
             (_p)++) \
                if (_SHR_PBMP_MEMBER(SOC_PORT_BITMAP(_u,_pt), (_p)))
#define PBMP_FE_ITER(_u, _p)            _SOC_PBMP_ITER(_u,fe,_p)
#define PBMP_GE_ITER(_u, _p)            _SOC_PBMP_ITER(_u,ge,_p)
#define PBMP_LLID_ITER(_u,_p)           _SOC_PBMP_ITER(_u,llid,_p)
#define PBMP_PON_ITER(_u,_p)            _SOC_PBMP_ITER(_u,pon,_p)
#define PBMP_XE_ITER(_u, _p)            _SOC_PBMP_ITER(_u,xe,_p)
#define PBMP_CE_ITER(_u, _p)            _SOC_PBMP_ITER(_u,ce,_p)
#define PBMP_HG_ITER(_u, _p)            _SOC_PBMP_ITER(_u,hg,_p)
#define PBMP_IL_ITER(_u, _p)            _SOC_PBMP_ITER(_u,il,_p)
#define PBMP_SCH_ITER(_u, _p)           _SOC_PBMP_ITER(_u,sch,_p)
#define PBMP_HG_SUBPORT_ITER(_u, _p)    _SOC_PBMP_ITER(_u,hg_subport,_p)
#define PBMP_HL_ITER(_u, _p)            _SOC_PBMP_ITER(_u,hl,_p)
#define PBMP_ST_ITER(_u, _p)            _SOC_PBMP_ITER(_u,st,_p)
#define PBMP_GX_ITER(_u, _p)            _SOC_PBMP_ITER(_u,gx,_p)
#define PBMP_XL_ITER(_u, _p)            _SOC_PBMP_ITER(_u,xl,_p)
#define PBMP_XT_ITER(_u, _p)            _SOC_PBMP_ITER(_u,xt,_p)
#define PBMP_CL_ITER(_u, _p)            _SOC_PBMP_ITER(_u,cl,_p)
#define PBMP_C_ITER(_u, _p)             _SOC_PBMP_ITER(_u,c,_p)
#define PBMP_AXP_ITER(_u, _p)           _SOC_PBMP_ITER(_u,axp,_p)
#define PBMP_MXQ_ITER(_u, _p)           _SOC_PBMP_ITER(_u,mxq,_p)
#define PBMP_XG_ITER(_u, _p)            _SOC_PBMP_ITER(_u,xg,_p)
#define PBMP_XQ_ITER(_u, _p)            _SOC_PBMP_ITER(_u,xq,_p)
#define PBMP_HYPLITE_ITER(_u, _p)       _SOC_PBMP_ITER(_u,hyplite,_p)
#define PBMP_SPI_ITER(_u, _p)           _SOC_PBMP_ITER(_u,spi,_p)
#define PBMP_SPI_SUBPORT_ITER(_u, _p)   _SOC_PBMP_ITER(_u,spi_subport,_p)
#define PBMP_SCI_ITER(_u, _p)           _SOC_PBMP_ITER(_u,sci,_p)
#define PBMP_SFI_ITER(_u, _p)           _SOC_PBMP_ITER(_u,sfi,_p)
#define PBMP_REQ_ITER(_u, _p)           _SOC_PBMP_ITER(_u,req,_p)
#define PBMP_E_ITER(_u, _p)             _SOC_PBMP_ITER(_u,ether,_p)
#define PBMP_LP_ITER(_u, _p)            _SOC_PBMP_ITER(_u,lp,_p)
#define PBMP_SUBTAG_ITER(_u, _p)        _SOC_PBMP_ITER(_u,subtag,_p)
#define PBMP_TDM_ITER(_u, _p)           _SOC_PBMP_ITER(_u,tdm,_p)
#define PBMP_PORT_ITER(_u, _p)          _SOC_PBMP_ITER(_u,port,_p)
#define PBMP_PP_ALL_ITER(_u, _p)        _SOC_PBMP_ITER(_u,pp,_p)
#define PBMP_ALL_ITER(_u, _p)           _SOC_PBMP_ITER(_u,all,_p)

#define IS_OLP_PORT(unit, port)         \
        (port == OLP_PORT(unit))
#define IS_ERP_PORT(unit, port)         \
        (port == ERP_PORT(unit))
#define IS_RCPU_PORT(unit, port)         \
        (port == RCPU_PORT(unit))
#define IS_S_PORT(unit, port)           \
        (SOC_PBMP_MEMBER(SOC_INFO(unit).s_pbm, port))
#define IS_GMII_PORT(unit, port)        \
        (SOC_PBMP_MEMBER(SOC_INFO(unit).gmii_pbm, port))
#define IS_FE_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_FE_ALL(unit), port))
#define IS_GE_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_GE_ALL(unit), port))
#define IS_LLID_PORT(unit,port)         \
        (SOC_PBMP_MEMBER(PBMP_LLID_ALL(unit), port))
#define IS_PON_PORT(unit,port)          \
        (SOC_PBMP_MEMBER(PBMP_PON_ALL(unit), port))
#define IS_XE_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XE_ALL(unit), port))
#define IS_CE_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_CE_ALL(unit), port))
#define IS_HG_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_HG_ALL(unit), port))
#define IS_IL_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_IL_ALL(unit), port))
#define IS_SCH_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_SCH_ALL(unit), port))
#define IS_HG_SUBPORT_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_HG_SUBPORT_ALL(unit), port))
#define IS_HL_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_HL_ALL(unit), port))
#define IS_ST_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_ST_ALL(unit), port))
#define IS_GX_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_GX_ALL(unit), port))
#define IS_XL_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XL_ALL(unit), port))
#define IS_CL_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_CL_ALL(unit), port))
#define IS_C_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_C_ALL(unit), port))
#define IS_AXP_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_AXP_ALL(unit), port))
#define IS_XT_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XT_ALL(unit), port))
#define IS_XW_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XW_ALL(unit), port))
#define IS_MXQ_PORT(unit,port)           \
            (SOC_PBMP_MEMBER(PBMP_MXQ_ALL(unit), port))
#define IS_XG_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XG_ALL(unit), port))
#define IS_XQ_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XQ_ALL(unit), port))
#define IS_HYPLITE_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_HYPLITE_ALL(unit), port))
#define IS_SPI_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_SPI_ALL(unit), port))
#define IS_SPI_SUBPORT_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_SPI_SUBPORT_ALL(unit), port))
#define IS_SCI_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_SCI_ALL(unit), port))
#define IS_SFI_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_SFI_ALL(unit), port))
#define IS_REQ_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_REQ_ALL(unit), port))
#define IS_E_PORT(unit,port)            \
        (SOC_PBMP_MEMBER(PBMP_E_ALL(unit), port))
#define IS_LP_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_LP_ALL(unit), port))
#define IS_SUBTAG_PORT(unit, port)      \
        (SOC_PBMP_MEMBER(PBMP_SUBTAG_ALL(unit), port))
#define IS_RCY_PORT(unit,port)            \
        (SOC_PBMP_MEMBER(PBMP_RCY_ALL(unit), port))
#define IS_TDM_PORT(unit,port)            \
        (SOC_PBMP_MEMBER(PBMP_TDM_ALL(unit), port))
#define IS_PORT(unit,port)              \
        (SOC_BLOCK_IN_LIST(&(SOC_PORT_TYPE(unit, port)), SOC_BLK_NET))
#define IS_LB_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_LB(unit), port))
#define IS_MMU_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_MMU(unit), port))
#define IS_EXT_MEM_PORT(unit,port)           \
            (SOC_PBMP_MEMBER(PBMP_EXT_MEM(unit), port))
#define IS_CPU_PORT(unit,port)          \
        (SOC_BLOCK_IN_LIST(&(SOC_PORT_TYPE(unit, port)), SOC_BLK_CPU))
#define IS_SPI_BLK(unit,port)      \
    (SOC_PORT_TYPE(unit, port) == SOC_BLK_SPI)
#define IS_EXP_PORT(unit,port)      \
    (SOC_PORT_TYPE(unit, port) == SOC_BLK_EXP)
#define IS_ALL_PORT(unit,port)          \
        (SOC_PORT_TYPE(unit, port) != 0)

/* Stack related macros */

#define SOC_PBMP_STACK_CURRENT(unit)    \
    (SOC_PERSIST(unit)->stack_ports_current)
#define SOC_PBMP_STACK_INACTIVE(unit)   \
    (SOC_PERSIST(unit)->stack_ports_inactive)
#define SOC_PBMP_STACK_PREVIOUS(unit)   \
    (SOC_PERSIST(unit)->stack_ports_previous)

#define SOC_PBMP_STACK_ACTIVE_GET(unit, active) do { \
        SOC_PBMP_ASSIGN(active, SOC_PBMP_STACK_CURRENT(unit)); \
        SOC_PBMP_REMOVE(active, SOC_PBMP_STACK_INACTIVE(unit)); \
    } while (0)

#define SOC_IS_STACK_PORT(unit, port) \
    SOC_PBMP_MEMBER(SOC_PBMP_STACK_CURRENT(unit), port)

#define SOC_IS_INACTIVE_STACK_PORT(unit, port) \
    SOC_PBMP_MEMBER(SOC_PBMP_STACK_INACTIVE(unit), port)

/* SL Mode set/get macros */
#define SOC_SL_MODE(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_SL_MODE) != 0)

#define SOC_SL_MODE_SET(unit, sl_mode) \
    if (sl_mode) SOC_CONTROL(unit)->soc_flags |= SOC_F_SL_MODE; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_SL_MODE

/* KNET Mode set/get macros */
#define SOC_KNET_MODE(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_KNET_MODE) != 0)

#define SOC_KNET_MODE_SET(unit, knet_mode) \
    if (knet_mode) SOC_CONTROL(unit)->soc_flags |= SOC_F_KNET_MODE; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_KNET_MODE

/* URPF on/off set/get macros */
#define SOC_URPF_STATUS_GET(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_URPF_ENABLED) != 0)

#define SOC_URPF_STATUS_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_URPF_ENABLED; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_URPF_ENABLED

#define SOC_L3_DEFIP_INDEX_REMAP_GET(unit) \
    SOC_CONTROL(unit)->l3_defip_index_remap

#define SOC_L3_DEFIP_MAX_TCAMS_GET(unit) \
    SOC_CONTROL(unit)->l3_defip_max_tcams

#define SOC_L3_DEFIP_TCAM_DEPTH_GET(unit) \
    SOC_CONTROL(unit)->l3_defip_tcam_size

#define SOC_L3_DEFIP_LOG_TO_PHY_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_phy_index

#define SOC_L3_DEFIP_PHY_TO_LOG_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_log_index

#define SOC_L3_DEFIP_URPF_LOG_TO_PHY_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_urpf_phy_index

#define SOC_L3_DEFIP_URPF_PHY_TO_LOG_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_urpf_log_index

#define SOC_L3_DEFIP_LOG_TO_PHY_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_phy_index[index]

#define SOC_L3_DEFIP_PHY_TO_LOG_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_log_index[index]

#define SOC_L3_DEFIP_URPF_LOG_TO_PHY_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_urpf_phy_index[index]

#define SOC_L3_DEFIP_URPF_PHY_TO_LOG_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_urpf_log_index[index]

#define SOC_L3_DEFIP_PAIR_LOG_TO_PHY_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_phy_index

#define SOC_L3_DEFIP_PAIR_PHY_TO_LOG_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_log_index

#define SOC_L3_DEFIP_PAIR_URPF_LOG_TO_PHY_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_urpf_phy_index

#define SOC_L3_DEFIP_PAIR_URPF_PHY_TO_LOG_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_urpf_log_index

#define SOC_L3_DEFIP_PAIR_LOG_TO_PHY_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_phy_index[index]

#define SOC_L3_DEFIP_PAIR_PHY_TO_LOG_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_log_index[index]

#define SOC_L3_DEFIP_PAIR_URPF_LOG_TO_PHY_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_urpf_phy_index[index]

#define SOC_L3_DEFIP_PAIR_URPF_PHY_TO_LOG_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_urpf_log_index[index]

#define SOC_L3_DEFIP_INDEX_INIT(unit) \
    SOC_CONTROL(unit)->defip_index_table

#define SOC_L3_DEFIP_MAX_128B_ENTRIES(unit) \
    SOC_CONTROL(unit)->l3_defip_max_128b_entries

#define SOC_L3_DEFIP_SCACHE_HANDLE(unit) \
    SOC_CONTROL(unit)->l3_defip_scache_ptr

/* URPF on/off set/get macros */
#define SOC_L2X_GROUP_ENABLE_GET(_unit_) (SOC_CONTROL(_unit_)->l2x_group_enable)
#define SOC_L2X_GROUP_ENABLE_SET(_unit_, _status_) \
           (SOC_CONTROL(_unit_)->l2x_group_enable) = (_status_) ?  TRUE : FALSE


/* Device should use memory dma for memory clear ops */
#define SOC_MEM_CLEAR_USE_DMA(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_MEM_CLEAR_USE_DMA) != 0)

#define SOC_MEM_CLEAR_USE_DMA_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_MEM_CLEAR_USE_DMA; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_MEM_CLEAR_USE_DMA


/* Device should use hw pipe clear for memory clear ops */
#define SOC_MEM_CLEAR_HW_ACC(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_MEM_CLEAR_HW_ACC) != 0)

#define SOC_MEM_CLEAR_HW_ACC_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_MEM_CLEAR_HW_ACC; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_MEM_CLEAR_HW_ACC

/* Device should not use mem cache in mem test mode */
#define SOC_MEM_TEST_SKIP_CACHE(unit) \
    (SOC_CONTROL(unit)->skip_cache_use)

#define SOC_MEM_TEST_SKIP_CACHE_SET(unit, enable) \
    (SOC_CONTROL(unit)->skip_cache_use = enable)

#define SOC_MEM_FORCE_READ_THROUGH(unit) \
    (SOC_CONTROL(unit)->force_read_through)

#define SOC_MEM_FORCE_READ_THROUGH_SET(unit, enable) \
    (SOC_CONTROL(unit)->force_read_through = enable)

#define SOC_MEM_CACHE_COHERENCY_CHECK(unit) \
    (SOC_CONTROL(unit)->cache_coherency_chk)

#define SOC_MEM_CACHE_COHERENCY_CHECK_SET(unit, enable) \
    (SOC_CONTROL(unit)->cache_coherency_chk = enable)

#define SOC_SER_SUPPORT(unit) \
    (SOC_CONTROL(unit)->parity_enable)

#define SOC_SER_CORRECTION_SUPPORT(unit) \
    (SOC_CONTROL(unit)->parity_correction)

#define SOC_SER_COUNTER_CORRECTION(unit) \
    (SOC_CONTROL(unit)->parity_counter_clear)

#define SOC_REG_RETURN_SER_ERROR(unit) \
    (SOC_CONTROL(unit)->return_reg_error)

#define SOC_REG_RETURN_SER_ERROR_SET(unit, enable) \
    (SOC_CONTROL(unit)->return_reg_error = enable)

/* Device should use gport for source/destination notation */
#define SOC_USE_GPORT(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_GPORT) != 0)

#define SOC_USE_GPORT_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_GPORT; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_GPORT

/* Maximum vrf id for the device set/get macro */
#define SOC_VRF_MAX(unit)  SOC_CONTROL(unit)->max_vrf_id
#define SOC_VRF_MAX_SET(unit, value) \
            SOC_CONTROL(unit)->max_vrf_id = value

/* Maximum address class for the device set/get macro */
#define SOC_ADDR_CLASS_MAX(unit)  SOC_CONTROL(unit)->max_address_class
#define SOC_ADDR_CLASS_MAX_SET(unit, value) \
            SOC_CONTROL(unit)->max_address_class = value

/* Maximum overlaid address class for the device set/get macro */
#define SOC_OVERLAID_ADDR_CLASS_MAX(unit) \
            SOC_CONTROL(unit)->max_overlaid_address_class
#define SOC_OVERLAID_ADDR_CLASS_MAX_SET(unit, value) \
            SOC_CONTROL(unit)->max_overlaid_address_class = value

/* Maximum extended address class for the device set/get macro */
#define SOC_EXT_ADDR_CLASS_MAX(unit) \
            SOC_CONTROL(unit)->max_extended_address_class
#define SOC_EXT_ADDR_CLASS_MAX_SET(unit, value) \
            SOC_CONTROL(unit)->max_extended_address_class = value

/* Maximum interface class for the device set/get macro */
#define SOC_INTF_CLASS_MAX(unit)  SOC_CONTROL(unit)->max_interface_class
#define SOC_INTF_CLASS_MAX_SET(unit, value) \
            SOC_CONTROL(unit)->max_interface_class = value


/* Device memory clear chunk size set/get macros */
#define SOC_MEM_CLEAR_CHUNK_SIZE_GET(unit)  SOC_CONTROL(unit)->mem_clear_chunk_size
#define SOC_MEM_CLEAR_CHUNK_SIZE_SET(unit, value) \
            SOC_CONTROL(unit)->mem_clear_chunk_size = value

/* IPMC replication sharing set/get macros */
#define SOC_IPMCREPLSHR_GET(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_IPMCREPLSHR) != 0)

#define SOC_IPMCREPLSHR_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_IPMCREPLSHR; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_IPMCREPLSHR

/* Device should use gport for source/destination notation */
#define SOC_REMOTE_ENCAP(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_REMOTE_ENCAP) != 0)

#define SOC_REMOTE_ENCAP_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_REMOTE_ENCAP; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_REMOTE_ENCAP

/* Dual hash global maximum recursion level */
#define SOC_DUAL_HASH_MOVE_MAX(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth
/* L2X dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_L2X(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_l2x
/* MPLS dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_MPLS(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_mpls
/* VLAN dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_VLAN(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_vlan
/* EGRESS VLAN dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_EGRESS_VLAN(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_egress_vlan
#if defined(INCLUDE_L3)
/* L3X dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_L3X(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_l3x
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
/* L3X dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_WLAN_PORT(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_wlan_port
#define SOC_DUAL_HASH_MOVE_MAX_WLAN_CLIENT(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_wlan_client
/* FT_SESSION*m dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MAX_FT_SESSION_IPV4(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_ft_session_ipv4
#define SOC_DUAL_HASH_MAX_FT_SESSION_IPV6(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_ft_session_ipv6
#endif
/* ING_VP_VLAN_MEMBERSHIP dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_ING_VP_VLAN_MEMBER(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_ing_vp_vlan_member
/* EGR_VP_VLAN_MEMBERSHIP dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_EGR_VP_VLAN_MEMBER(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_egr_vp_vlan_member
/* ING_DNAT_ADDRESS_TYPE dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_ING_DNAT_ADDRESS_TYPE(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_ing_dnat_address_type
/* L2_ENDPOINT_ID dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_L2_ENDPOINT_ID(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_l2_endpoint_id
/* ENDPOINT_QUEUE_MAP dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_ENDPOINT_QUEUE_MAP(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_endpoint_queue_map
/* Multi hash global maximum recursion level */
#define SOC_MULTI_HASH_MOVE_MAX(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth
/* L2 multi hash tables specific recursion level */
#define SOC_MULTI_HASH_MOVE_MAX_L2(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth_l2
/* L3 multi hash tables specific recursion level */
#define SOC_MULTI_HASH_MOVE_MAX_L3(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth_l3
/* MPLS multi hash tables specific recursion level */
#define SOC_MULTI_HASH_MOVE_MAX_MPLS(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth_mpls
/* VLAN multi hash tables specific recursion level */
#define SOC_MULTI_HASH_MOVE_MAX_VLAN(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth_vlan
/* EGRESS VLAN multi hash tables specific recursion level */
#define SOC_MULTI_HASH_MOVE_MAX_EGRESS_VLAN(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth_egress_vlan

/* Multi hash tables specific moving recursion algorithm */
#define SOC_MULTI_HASH_MOVE_ALGORITHM(unit) \
    SOC_CONTROL(unit)->multi_hash_move_algorithm

#define SOC_MCAST_ADD_ALL_ROUTER_PORTS(unit) \
    SOC_CONTROL(unit)->mcast_add_all_router_ports

#ifdef BCM_CB_ABORT_ON_ERR
#define SOC_CB_ABORT_ON_ERR(unit) \
    SOC_CONTROL(unit)->cb_abort_on_err
#endif

/* Switching logic bypass mode */
#define SOC_SWITCH_BYPASS_MODE_NONE             0
#define SOC_SWITCH_BYPASS_MODE_L3_ONLY          1
#define SOC_SWITCH_BYPASS_MODE_L3_AND_FP        2

#define SOC_SWITCH_BYPASS_MODE(unit) \
    SOC_CONTROL(unit)->switch_bypass_mode

/* Double tag mode when it is a device-wide property */
#define SOC_DT_MODE(unit) \
    SOC_CONTROL(unit)->dt_mode

#define SOC_MAC_LOW_POWER_ENABLED(unit) \
    SOC_CONTROL(unit)->mac_low_power_enabled

#define SOC_AUTO_MAC_LOW_POWER(unit) \
    SOC_CONTROL(unit)->auto_mac_low_power
    
#define MAC_LOW_POWER_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->mac_low_power_mutex, sal_mutex_FOREVER)
#define MAC_LOW_POWER_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->mac_low_power_mutex)

#if defined(BCM_53125) || defined(BCM_53128) || defined(BCM_POLAR_SUPPORT)
/* Lock the internal micro control unit to prevent the PHY register access */
#define INT_MCU_LOCK(unit) \
        if (soc_feature(unit, soc_feature_int_cpu_arbiter) && \
            (SOC_CONTROL(unit)->int_cpu_enabled))  \
        DRV_DEV_PROP_SET(unit, DRV_DEV_PROP_RESOURCE_ARBITER_REQ, TRUE)
#define INT_MCU_UNLOCK(unit) \
        if (soc_feature(unit, soc_feature_int_cpu_arbiter) && \
            (SOC_CONTROL(unit)->int_cpu_enabled))  \
        DRV_DEV_PROP_SET(unit, DRV_DEV_PROP_RESOURCE_ARBITER_REQ, FALSE)
#else
#define INT_MCU_LOCK(unit) 
#define INT_MCU_UNLOCK(unit) 
#endif /* BCM_53125 */

/*
 * LMD enabled ports. 2.5 Gbps stacking support
 */
#define SOC_LMD_PBM(unit) SOC_INFO(unit).lmd_pbm
#define SOC_LMD_ENABLED_PORT_SET(unit, port)     \
    SOC_PBMP_PORT_SET(SOC_INFO(unit).lmd_pbm, port)
#define IS_LMD_ENABLED_PORT(unit, port)          \
    SOC_PBMP_MEMBER(SOC_INFO(unit).lmd_pbm, port)

/*
 * Ports using HiGig2 encapsulation.
 * This is a cached value used for bcm_tx performance reasons.
 */
#define SOC_HG2_PBM(unit) SOC_INFO(unit).hg2_pbm
#define SOC_HG2_ENABLED_PORT_ADD(unit, port)     \
    SOC_PBMP_PORT_ADD(SOC_INFO(unit).hg2_pbm, port)
#define SOC_HG2_ENABLED_PORT_REMOVE(unit, port)     \
    SOC_PBMP_PORT_REMOVE(SOC_INFO(unit).hg2_pbm, port)
#define IS_HG2_ENABLED_PORT(unit, port)          \
    SOC_PBMP_MEMBER(SOC_INFO(unit).hg2_pbm, port)

extern int SOC_BLOCK_IS_TYPE(int unit, int blk_idx, int *list);
/* iterate over enabled blocks of type */
#define SOC_BLOCKS_ITER(unit, var, list) \
        for ((var) = 0; SOC_BLOCK_INFO(unit, var).type >= 0; (var)++) \
                if (SOC_INFO(unit).block_valid[var] && \
                    SOC_BLOCK_IS_TYPE(unit, var, list))

#define SOC_BLOCK_IS_CMP(unit, blk, val)\
    (SOC_BLOCK_TYPE(unit, blk) == val)
extern int SOC_BLOCK_IS_COMPOSITE(int unit, int blk_idx, int type);
/* iterate over enabled block of type(specific or composite) */
#define SOC_BLOCK_ITER(unit, var, val) \
        for ((var) = 0; SOC_BLOCK_INFO(unit, var).type >= 0; (var)++) \
                if (SOC_INFO(unit).block_valid[var] && \
                    (SOC_BLOCK_IS_CMP(unit, var, val) || \
                     SOC_BLOCK_IS_COMPOSITE(unit, var, val)))

#define SOC_MEM_BLOCK_MIN(unit, mem)    SOC_MEM_INFO(unit, mem).minblock
#define SOC_MEM_BLOCK_MAX(unit, mem)    (soc_mem_is_unified(unit, mem) ? \
                        SOC_MEM_INFO(unit, mem).minblock : \
                        SOC_MEM_INFO(unit, mem).maxblock)
#define SOC_MEM_BLOCK_ANY(unit, mem)    (SOC_INFO(unit).mem_block_any[mem])

#ifdef BCM_ROBO_SUPPORT
#define SOC_MEM_BLOCK_VALID(unit, mem, blk)     \
                        ((blk) >= 0 && \
                         (blk) < (SOC_IS_ROBO(unit) ? SOC_ROBO_MAX_NUM_BLKS \
                         : SOC_MAX_NUM_BLKS) && \
                         (SOC_IS_ROBO(unit) || (((blk)>=32)?(SOC_MEM_INFO(unit, mem).blocks_hi & (1<<((blk)&0x1F))):(SOC_MEM_INFO(unit, mem).blocks & (1<<(blk)))))&& \
                         (SOC_INFO(unit).block_valid[blk]))
#else /* !BCM_ROBO_SUPPORT */
#define SOC_MEM_BLOCK_VALID(unit, mem, blk)     \
                        ((blk) >= 0 && \
                         (blk) < SOC_MAX_NUM_BLKS && \
                         (((blk)>=32)?(SOC_MEM_INFO(unit, mem).blocks_hi & (1<<((blk)&0x1F))):(SOC_MEM_INFO(unit, mem).blocks & (1<<(blk))))&& \
                         (SOC_INFO(unit).block_valid[blk]))

#endif /* BCM_ROBO_SUPPORT */

#ifdef BCM_ROBO_SUPPORT
#define SOC_MEM_BLOCK_ITER(unit, mem, var) \
        for ((var) = SOC_MEM_BLOCK_MIN(unit, mem); \
                (var) <= SOC_MEM_BLOCK_MAX(unit, mem); \
                (var)++) \
            if ((SOC_IS_ROBO(unit) || (((var)>=32)?(SOC_MEM_INFO(unit, mem).blocks_hi & (1 << ((var)&0x1F))):(SOC_MEM_INFO(unit, mem).blocks & (1 << (var))))) && \
                           SOC_INFO(unit).block_valid[var])
#else /* !BCM_ROBO_SUPPORT */
#define SOC_MEM_BLOCK_ITER(unit, mem, var) \
          for ((var) = SOC_MEM_BLOCK_MIN(unit, mem); \
                  (var) <= SOC_MEM_BLOCK_MAX(unit, mem); \
                  (var)++) \
          if ((((var)>=32)?(SOC_MEM_INFO(unit, mem).blocks_hi & (1 << ((var)&0x1F))):(SOC_MEM_INFO(unit, mem).blocks & (1 << (var)))) && \
                         SOC_INFO(unit).block_valid[var])
#endif /* BCM_ROBO_SUPPORT */


/* Default dcb/dma values */
#define SOC_DEFAULT_DMA_SRCMOD_GET(_u)      SOC_PERSIST(_u)->dcb_srcmod
#define SOC_DEFAULT_DMA_SRCPORT_GET(_u)     SOC_PERSIST(_u)->dcb_srcport
#define SOC_DEFAULT_DMA_PFM_GET(_u)         SOC_PERSIST(_u)->dcb_pfm
#define SOC_DEFAULT_DMA_SRCMOD_SET(_u, _v)  SOC_PERSIST(_u)->dcb_srcmod = (_v)
#define SOC_DEFAULT_DMA_SRCPORT_SET(_u, _v) SOC_PERSIST(_u)->dcb_srcport = (_v)
#define SOC_DEFAULT_DMA_PFM_SET(_u, _v)     SOC_PERSIST(_u)->dcb_pfm = (_v)

/* Features cache */
#define SOC_FEATURE_GET(unit, feat)     \
        SHR_BITGET(SOC_CONTROL(unit)->features, feat)
#define SOC_FEATURE_SET(unit, feat)     \
        SHR_BITSET(SOC_CONTROL(unit)->features, feat)
#define SOC_FEATURE_CLEAR(unit, feat)     \
        SHR_BITCLR(SOC_CONTROL(unit)->features, feat)

/*
 * Various mutex controls
 */
#define SCHAN_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->schanMutex, sal_mutex_FOREVER)
#define SCHAN_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->schanMutex)

#ifdef BCM_CMICM_SUPPORT
#define FSCHAN_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->fschanMutex, sal_mutex_FOREVER)
#define FSCHAN_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->fschanMutex)
#endif

#define MIIM_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->miimMutex, sal_mutex_FOREVER)
#define MIIM_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->miimMutex)

#define TABLE_DMA_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->tableDmaMutex, sal_mutex_FOREVER)
#define TABLE_DMA_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->tableDmaMutex)

#define TSLAM_DMA_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->tslamDmaMutex, sal_mutex_FOREVER)
#define TSLAM_DMA_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->tslamDmaMutex)

#ifdef BCM_CMICM_SUPPORT
#define CCM_DMA_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->ccmDmaMutex, sal_mutex_FOREVER)
#define CCM_DMA_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->ccmDmaMutex)
#endif

#ifdef BCM_SBUSDMA_SUPPORT
#define SOC_SBUSDMA_DM_LOCK(unit) \
    sal_mutex_take(SOC_SBUSDMA_DM_MUTEX(unit), sal_mutex_FOREVER)
#define SOC_SBUSDMA_DM_UNLOCK(unit) \
    sal_mutex_give(SOC_SBUSDMA_DM_MUTEX(unit))

#define SBUS_DMA_LOCK(unit, cmc, ch) \
        sal_mutex_take(SOC_CONTROL(unit)->sbusDmaMutexs[(cmc)][(ch)], sal_mutex_FOREVER)
#define SBUS_DMA_UNLOCK(unit, cmc, ch)                    \
        sal_mutex_give(SOC_CONTROL(unit)->sbusDmaMutexs[(cmc)][(ch)])
#endif

#define SOC_CONTROL_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->socControlMutex, sal_mutex_FOREVER)
#define SOC_CONTROL_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->socControlMutex)

#define COUNTER_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->counterMutex, sal_mutex_FOREVER)
#define COUNTER_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->counterMutex)
#define SOC_COUNTER_IF_ERROR_RETURN(op)         \
    do {                                        \
        int __rv__;                             \
        if (((__rv__ = (op)) < 0)) {            \
            COUNTER_UNLOCK(unit);               \
            _SHR_ERROR_TRACE(__rv__);           \
            return(__rv__);                     \
        }                                       \
    } while(0)


#define OVERRIDE_LOCK(unit) \
       sal_mutex_take(SOC_CONTROL(unit)->overrideMutex, sal_mutex_FOREVER)
#define OVERRIDE_UNLOCK(unit) \
       sal_mutex_give(SOC_CONTROL(unit)->overrideMutex)

#define SPI_LOCK \
    sal_mutex_take(spiMutex, sal_mutex_FOREVER)
#define SPI_UNLOCK \
    sal_mutex_give(spiMutex)

#define SOC_EGRESS_METERING_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->egressMeteringMutex, \
                       sal_mutex_FOREVER)
#define SOC_EGRESS_METERING_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->egressMeteringMutex)

#define ARLDMA_SIZE_DEFAULT     1024    /* Size in 16-byte entries */

#define IP_ARBITER_LOCK(unit) \
    sal_mutex_take(SOC_CONTROL(unit)->ipArbiterMutex, sal_mutex_FOREVER)
#define IP_ARBITER_UNLOCK(unit) \
    sal_mutex_give(SOC_CONTROL(unit)->ipArbiterMutex)

#if defined(BCM_TRIUMPH3_SUPPORT)
#define SOC_ESM_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->esm_lock, sal_mutex_FOREVER)
#define SOC_ESM_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->esm_lock)
#endif
/*
 * Define the default number of milliseconds before a BIST operation
 * times out and fails.  Different values apply if the SAL boot flag
 * BOOT_F_QUICKTURN or BOOT_F_PLISIM are set.
 */

#define BIST_TIMEOUT            (1  * SECOND_MSEC)
#define BIST_TIMEOUT_QT         (1  * MINUTE_MSEC)
#define BIST_TIMEOUT_PLI        (20 * MINUTE_MSEC)

typedef struct _soc_l3_defip_index_table_s {
    int         *defip_tcam_phy_index;  /* Array holding defip phyiscal indexes
                                         * for a given logical index */
    int         *defip_tcam_log_index;    /* Array holding defip logical indexes
                                           * for a  given physical index */
    int         *defip_tcam_urpf_phy_index; /* Array holding defip phyiscal
                                             * indexes for a given logical
                                             * index when urpf is enabled */
    int         *defip_tcam_urpf_log_index; /* Array holding defip logical
                                             * indexes for a  given physical
                                             * index when urpf is enabled */
    int         *defip_pair_tcam_phy_index;  /* Array holding defip_pair
                                              * phyiscal indexes for a given
                                              * logical index */
    int         *defip_pair_tcam_log_index;  /* Array holding defip_pair logical
                                              * indexes for a  given physical
                                              * index */
    int         *defip_pair_tcam_urpf_phy_index; /* Array holding defip_pair
                                                  * phyiscal indexes for a
                                                  * given logical index when
                                                  * urpf is enabled */
    int         *defip_pair_tcam_urpf_log_index; /* Array holding defip logical
                                                  * indexes for a  given
                                                  * physical index when urpf is
                                                  * enabled */
} _soc_l3_defip_index_table_t;

/*
 * Typedef: soc_stat_t
 * Purpose: Driver statistics counts (interrupts, errors, etc).
 */
typedef struct soc_stat_s {
    uint32      intr;           /* Total interrupt count */
    uint32      intr_sce;       /* S-Channel error interrupt count */
    uint32      intr_sc;        /* S-Channel interrupt count */
    uint32      intr_ls;        /* Link status interrupt count */
    uint32      intr_gbp;       /* GBP Full interrupt count */
    uint32      intr_pci_fe;    /* PCI Fatal Error interrupt count */
    uint32      intr_pci_pe;    /* PCI Parity Error interrupt count */
    uint32      intr_arl_d;     /* ARL message dropped interrupt count */
    uint32      intr_arl_m;     /* ARL message ready interrupt count */
    uint32      intr_arl_x;     /* ARL DMA xfer done interrupt count */
    uint32      intr_arl_0;     /* ARL DMA count=0 interrupt count */
    uint32      intr_i2c;       /* I2C controller interrupt count */
    uint32      intr_mii;       /* MII interrupt count */
    uint32      intr_stats;     /* Stats DMA interrupt count */
    uint32      intr_desc;      /* DMA desc done interrupt count */
    uint32      intr_chain;     /* DMA chain done interrupt count */
    uint32      intr_mmu;       /* MMU status interrupt count */
    uint32      intr_tdma;      /* Table DMA complete interrupt count */
    uint32      intr_tslam;     /* Table SLAM DMA complete interrupt count */
    uint32      intr_ccmdma;    /* CCM DMA complete interrupt count */
    uint32      intr_sw;        /* Cross CPU S/W interrupts */
    uint32      intr_mem_cmd[3]; /* Memory command complete interrupt count */
    uint32      intr_chip_func[5]; /* Chip functional interrupt count */
    uint32      intr_fifo_dma[4];  /* Fifo-dma interrupt count */
    uint32      intr_block;     /* Block interrupt count */

    uint32      schan_op;       /* Number of S-Channel operations */
    uint32      mem_cmd_op;     /* Number of memory command operations */

    uint32      err_sdram;      /* SDRAM parity error count */
    uint32      err_cfap;       /* CFAP oversubscribed count */
    uint32      err_fcell;      /* Unexpected First cell count */
    uint32      err_sr;         /* MMU Soft Reset count */
    uint32      err_cellcrc;    /* CBP cell CRC count */
    uint32      err_cbphp;      /* CBP header parity count */
    uint32      err_npcell;     /* MMU cells not in packet count */
    uint32      err_mp;         /* Memory parity error count */
    uint32      err_pdlock;     /* PLL/DLL Lock loss count */
    uint32      err_cpcrc;      /* Cell pointer CRC error count */
    uint32      err_cdcrc;      /* Cell data CRC error count */
    uint32      err_fdcrc;      /* Frame data CRC error count */
    uint32      err_cpbcrc;     /* Cell pointer block CRC error count */
    uint32      err_multi;      /* Multiple error count */
    uint32      err_invalid;    /* Invalid schan error count */
    uint32      err_sc_tmo;     /* S-Channel operation timeout count */
    uint32      err_mii_tmo;    /* MII operation timeout count */
    uint32      err_mc_tmo;     /* Memory command operation timeout count */

    uint32      arl_msg_ins;    /* Count of ARL insert messages processed */
    uint32      arl_msg_del;    /* Count of ARL delete messages processed */
    uint32      arl_msg_bad;    /* Count of bad messages processed */
    uint32      arl_msg_tot;    /* Count of all ARL messages */

    uint32      dma_rpkt;       /* Packets received by CPU */
    uint32      dma_rbyt;       /* Bytes received by CPU */
    uint32      dma_tpkt;       /* Packets transmitted by CPU */
    uint32      dma_tbyt;       /* Bytes transmitted by CPU */

    uint32      dv_alloc;       /* Number of DV alloc's */
    uint32      dv_free;        /* Number of DV free's */
    uint32      dv_alloc_q;     /* Free list satisfied DV allocs */

    uint32      mem_cache_count;
    uint32      mem_cache_size;
    uint32      mem_cache_vmap_size;
    uint32      tcam_corrupt_map_size;
    uint32      ser_err_int;    /* Number of ser interrupt events */
    uint32      ser_err_fifo;   /* Number of ser fifo events */
    uint32      ser_err_tcam;   /* Number of ser tcam events */
    uint32      ser_err_nak;    /* Number of ser NAK events */
    uint32      ser_err_stat;   /* Number of ser stat events */
    uint32      ser_err_ecc;    /* Number of ser ecc events */
    uint32      ser_err_corr;   /* Number of ser error corrections */
    uint32      ser_err_clear;  /* Number of ser error reg/mem cleared */
    uint32      ser_err_restor; /* Number of ser error cache restored */
    uint32      ser_err_spe;    /* Number of special ser errors */
    uint32      ser_err_reg;    /* Number of reg based ser errors */
    uint32      ser_err_mem;    /* Number of mem based ser errors */
    uint32      ser_err_sw;     /* Number of ser s/w errors */
} soc_stat_t;

/*
 * Typedef: soc_memstate_t
 * Purpose: Maintain per-memory information
 * Notes: To avoid deadlock, do not use MEM_LOCK while holding SCHAN_LOCK.
 *      MEM_LOCK must be held while manipulating memory caches in any way.
 */

#define SOC_MEM_STATE(unit, mem)        SOC_CONTROL(unit)->memState[mem]

#define MEM_LOCK(unit, mem) \
        sal_mutex_take(SOC_MEM_STATE(unit, mem).lock, sal_mutex_FOREVER)
#define MEM_UNLOCK(unit, mem) \
        sal_mutex_give(SOC_MEM_STATE(unit, mem).lock)

#define CACHE_VMAP_OP(vmap, index, BOOL_OP) \
        ((vmap)[(index) / 8] BOOL_OP (1 << ((index) % 8)))

#define CACHE_VMAP_SET(vmap, index)     CACHE_VMAP_OP(vmap, index, |=)
#define CACHE_VMAP_CLR(vmap, index)     CACHE_VMAP_OP(vmap, index, &= ~)
#define CACHE_VMAP_TST(vmap, index)     CACHE_VMAP_OP(vmap, index, &)

#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_HELIX4_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) || \
    defined(BCM_GREYHOUND_SUPPORT)
#define TCAM_CORRUPT_MAP_OP(corrupt, index, BOOL_OP) \
        ((corrupt)[(index) / 8] BOOL_OP (1 << ((index) % 8)))

#define TCAM_CORRUPT_MAP_SET(corrupt, index)     TCAM_CORRUPT_MAP_OP(corrupt, index, |=)
#define TCAM_CORRUPT_MAP_CLR(corrupt, index)     TCAM_CORRUPT_MAP_OP(corrupt, index, &= ~)
#define TCAM_CORRUPT_MAP_TST(corrupt, index)     TCAM_CORRUPT_MAP_OP(corrupt, index, &)
#endif

typedef struct soc_memstate_s {
    sal_mutex_t lock;                   /* Table update lock */
                                        /*   (sorted tables only) */
    uint32     *cache[SOC_MAX_NUM_BLKS];/* Write-through cache when non-NULL */
    uint8      *vmap[SOC_MAX_NUM_BLKS]; /* Cache entry valid bitmap */
#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_HELIX4_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) || \
    defined(BCM_GREYHOUND_SUPPORT)
    uint8      *corrupt[SOC_MAX_NUM_BLKS]; /* entry corrupted bitmap */
                                           /* tcam tables only */
#endif
} soc_memstate_t;

/*
 * Typedef: soc_driver_t
 * Purpose: Chip driver.  All info about a particular device type.
 * Notes: These structures are automatically generated by mcm.
 *        Used in soc/mcm/bcm*.c files.
 */
typedef struct soc_driver_s {
    soc_chip_types          type;                /* the chip type id */
    char                    *chip_string;        /* chip string for var defs */
    char                    *origin;             /* regsfile origin */
    uint16                  pci_vendor;          /* nominal PCI vendor */
    uint16                  pci_device;          /* nominal PCI device */
    uint8                   pci_revision;        /* nominal PCI revision */
    int                     num_cos;             /* classes of service */
    soc_reg_info_t          **reg_info;          /* register array */
    soc_reg_above_64_info_t **reg_above_64_info; /* large register array */
    soc_reg_array_info_t    **reg_array_info;    /* register array array */
    soc_mem_info_t          **mem_info;          /* memory array */
    soc_mem_t               **mem_aggr;          /* memory aggregate */
    soc_mem_array_info_t    **mem_array_info;    /* memory array array */
    soc_block_info_t        *block_info;         /* block array */
    soc_port_info_t         *port_info;          /* port array */
    soc_cmap_t              *counter_maps;       /* counter map */
    soc_feature_fun_t       feature;             /* boolean feature function */
    soc_init_chip_fun_t     init;                /* chip init function */
    drv_if_t                *services;
    int                     port_num_blktype;    /* block types per port */
    uint32                  cmicd_base;          /* Base address for CMICd Registers */ 
} soc_driver_t;

typedef int (*soc_misc_init_f)(int);
typedef int (*soc_mmu_init_f)(int);
typedef int (*soc_age_timer_get_f)(int, int *, int *);
typedef int (*soc_age_timer_max_get_f)(int, int *);
typedef int (*soc_age_timer_set_f)(int, int, int);
typedef int (*soc_phy_firmware_load_f)(int, int, uint8 *, int);
typedef int (*soc_sbus_mdio_read_f)(int, uint32, uint32, uint32 *);
typedef int (*soc_sbus_mdio_write_f)(int, uint32, uint32, uint32);

/*
 * Typedef: soc_functions_t
 * Purpose: Chip driver functions that are not automatically generated.
 */
typedef struct soc_functions_s {
    /* Get/set age timer value and enable in device, if supported */
    soc_misc_init_f                    soc_misc_init;
    soc_mmu_init_f                     soc_mmu_init;
    soc_age_timer_get_f                soc_age_timer_get;
    soc_age_timer_max_get_f            soc_age_timer_max_get;
    soc_age_timer_set_f                soc_age_timer_set;
    soc_phy_firmware_load_f            soc_phy_firmware_load;
    soc_sbus_mdio_read_f               soc_sbus_mdio_read;
    soc_sbus_mdio_write_f              soc_sbus_mdio_write;
} soc_functions_t;

/* Below defines and soc_port_detals_t are applicable for katana2 device only */
#define GE_PORT    0x0001
#define XE_PORT    0x0002
#define HG_PORT    0x0004
#define HGL_PORT   0x0008
#define STK_PORT   0x0010
#define CES_PORT   0x0020
#define OLP_PORT   0x0040
#define LPHY_PORT  0x0080

typedef struct soc_port_details_s {
    /* Range of port numbers */
    uint8 start_port_no;
    uint8 end_port_no;
    uint8 port_incr;

    /* Port Properties */
    uint8 port_type;/*Combination of hash defines(GE,FE,XE,HG,HGL,STK,CE,LPH*/ 

    /* Port Speed */
    uint32 port_speed;
}soc_port_details_t;

/*
 * This define tells whether a driver is active (not null).
 * The null driver is allocated, but is all zeros.  We assume that
 * the block list (block_info) is non-null for non-null drivers.
 */
#define SOC_DRIVER_ACTIVE(i) (soc_base_driver_table[i]->block_info)
#define SOC_ROBO_DRIVER_ACTIVE(i) (soc_robo_base_driver_table[i]->block_info)

/* Find an active chip to get reg info from base driver table. */
#define SOC_FIRST_ACTIVE_DRIVER(chip) \
    do { \
        chip = 0; \
        while (!SOC_DRIVER_ACTIVE(chip) && chip < SOC_NUM_SUPPORTED_CHIPS) { \
            chip++; \
        } \
        assert(chip < SOC_NUM_SUPPORTED_CHIPS); \
    } while (0)

#ifdef BCM_ROBO_SUPPORT
#define SOC_ROBO_DRIVER_ACTIVE(i) (soc_robo_base_driver_table[i]->block_info)

/* Find an active chip to get reg info from base driver table. */
#define SOC_ROBO_FIRST_ACTIVE_DRIVER(chip) \
    do { \
    chip = 0; \
    while (!SOC_ROBO_DRIVER_ACTIVE(chip) && chip < SOC_ROBO_NUM_SUPPORTED_CHIPS) { \
        chip++; \
    } \
    assert(chip < SOC_ROBO_NUM_SUPPORTED_CHIPS); \
    } while (0)
#endif /* BCM_ROBO_SUPPORT */    

extern soc_driver_t *soc_chip_driver_find(uint16 dev_id, uint8 rev_id);
extern void soc_chip_dump(int unit, soc_driver_t *d);
extern void soc_robo_chip_dump(int unit, soc_driver_t *d);

#define SOC_UNIT_NAME(unit) \
    (SOC_CHIP_NAME(SOC_DRIVER(unit)->type))

#define SOC_UNIT_GROUP(unit) \
    (soc_chip_group_names[soc_chip_type_map[SOC_DRIVER(unit)->type]])


/*
 * Typedef: soc_fm_t
 * Purpose: For each filter mask entry, cache the index of the first rule
 *          using that mask, and the number of rules using that mask.
 *          (All rules that use a given mask are necessarily contiguous.)
 *          These values speed up searches in the FFP, and on 5690 they
 *          are required by the hardware.
 */

#define SOC_FFP_MASK_MAX        24      /* Max # masks of any chip */

typedef struct soc_fm_s {
    uint16      start, count;
} soc_fm_t;

/*
 * Typedef: soc_ipg_t
 * Purpose: IFG register setting for all speed/duplex combinations
 */

typedef struct soc_ipg_s {
    uint32      hd_10;
    uint32      hd_100;
    uint32      hd_1000;
    uint32      hd_2500;
    uint32      fd_10;
    uint32      fd_100;
    uint32      fd_1000;
    uint32      fd_2500;
    uint32      fd_10000;
    uint32      fd_xe;
    uint32      fd_hg;
    uint32      fe_hd_10;
    uint32      fe_hd_100;
    uint32      fe_fd_10;
    uint32      fe_fd_100;
    uint32      gth_hd_10;
    uint32      gth_hd_100;
    uint32      gth_fd_10;
    uint32      gth_fd_100;
    uint32      ge_hd_1000;
    uint32      ge_fd_1000;
    uint32      ge_hd_2500;
    uint32      ge_fd_2500;
    uint32      bm_fd_10000;
} soc_ipg_t;

/*
 * soc_info_t is part of the per-unit soc_control_t.
 * It gets filled in at run time based on the port_info and block_info
 * from the driver structure.
 */
typedef struct {
#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
    int     port[SOC_MAX_NUM_PP_PORTS];
#else
    int     port[SOC_MAX_NUM_PORTS];
#endif
    int     num;                    /* number of entries used in port[] */
    int     min;                    /* minimum bit on */
    int     max;                    /* maximum bit on */
    pbmp_t  bitmap;
    pbmp_t  disabled_bitmap;        /* ports that are forced to be disabled */
} soc_ptype_t;

typedef struct {
    soc_chip_types    driver_type;          /* chip type (with rev) */
    soc_chip_groups_t driver_group;         /* chip type (no rev) */

    soc_chip_e        chip_type;            /* chip type enum - used instead of 'chip' 
                                               for newer devices as the bitmap was exhausted */
    uint32            spi_device;

    uint32           chip;                  /* chip id bits */

#define SOC_INFO_CHIP_ENDURO         0x00000001    /* 56334 */
#define SOC_INFO_CHIP_APOLLO         0x00000002    /* 56524 */
#define SOC_INFO_CHIP_TRIDENT2       0x00000004    /* 5685x */
#define SOC_INFO_CHIP_TITAN2         0x00000008    /* 5675x */
#define SOC_INFO_CHIP_KATANA2        0x00000010    /* 5645x */
#define SOC_INFO_CHIP_HURRICANE2     0x00000020    /* 5615x */
#define SOC_INFO_CHIP_HELIX4         0x00000040    /* 5634x */
#define SOC_INFO_CHIP_HERCULES15     0x00000080    /* 5675 */
#define SOC_INFO_CHIP_TOMAHAWK       0x00000100    /* 5696x */
#define SOC_INFO_CHIP_FIREBOLT       0x00000200    /* 56504 */
#define SOC_INFO_CHIP_VALKYRIE2      0x00000400    /* 56685 */
#define SOC_INFO_CHIP_TRIDENT2PLUS   0x00000800    /* 5686x */
#define SOC_INFO_CHIP_FHX            0x00001000    /* 5610x, 5630x */
#define SOC_INFO_CHIP_SHADOW         0x00002000    /* 88732 */
#define SOC_INFO_CHIP_GOLDWING       0x00004000    /* 56580 */
#define SOC_INFO_CHIP_HUMV           0x00008000    /* 56700 */
#define SOC_INFO_CHIP_BRADLEY        0x00010000    /* 56800 */
#define SOC_INFO_CHIP_RAPTOR         0x00020000    /* 56218 */
#define SOC_INFO_CHIP_KATANA         0x00040000    /* 56440 */
#define SOC_INFO_CHIP_TRIUMPH3       0x00080000    /* 56640 */
#define SOC_INFO_CHIP_FIREBOLT2      0x00100000    /* 56514 */
#define SOC_INFO_CHIP_SCORPION       0x00200000    /* 56820 */
#define SOC_INFO_CHIP_TRIUMPH        0x00400000    /* 56624 */
#define SOC_INFO_CHIP_RAVEN          0x00800000    /* 56224 */
#define SOC_INFO_CHIP_HAWKEYE        0x01000000    /* 53314 */
#define SOC_INFO_CHIP_VALKYRIE       0x02000000    /* 56680 */
#define SOC_INFO_CHIP_CONQUEROR      0x04000000    /* 56725 */
#define SOC_INFO_CHIP_TRIUMPH2       0x08000000    /* 56634 */
#define SOC_INFO_CHIP_TRIDENT        0x10000000    /* 5684x */
#define SOC_INFO_CHIP_HURRICANE      0x20000000    /* 56142 */
#define SOC_INFO_CHIP_TITAN          0x40000000    /* 5674x */
#define SOC_INFO_CHIP_SIRIUS         0x80000000    /* 88230 */
#define SOC_INFO_CHIP_HERCULES       (SOC_INFO_CHIP_HERCULES15)
#define SOC_INFO_CHIP_FB             (SOC_INFO_CHIP_FIREBOLT | SOC_INFO_CHIP_FIREBOLT2)
#define SOC_INFO_CHIP_FX_HX          (SOC_INFO_CHIP_FHX | \
                                      SOC_INFO_CHIP_RAPTOR | \
                                      SOC_INFO_CHIP_RAVEN | \
                                      SOC_INFO_CHIP_HAWKEYE)
#define SOC_INFO_CHIP_FB_FX_HX       (SOC_INFO_CHIP_FB | SOC_INFO_CHIP_FX_HX)
#define SOC_INFO_CHIP_HB_GW          (SOC_INFO_CHIP_HUMV | \
                                      SOC_INFO_CHIP_BRADLEY | \
                                      SOC_INFO_CHIP_GOLDWING)
#define SOC_INFO_CHIP_TD2_TT2        (SOC_INFO_CHIP_TRIDENT2 | \
                                      SOC_INFO_CHIP_TITAN2)
#define SOC_INFO_CHIP_TD_TT          (SOC_INFO_CHIP_TRIDENT | \
                                      SOC_INFO_CHIP_TITAN | \
                                      SOC_INFO_CHIP_TD2_TT2)
#define SOC_INFO_CHIP_TR_VL          (SOC_INFO_CHIP_TRIUMPH | \
                                      SOC_INFO_CHIP_VALKYRIE | \
                                      SOC_INFO_CHIP_TRIUMPH2 | \
                                      SOC_INFO_CHIP_APOLLO | \
                                      SOC_INFO_CHIP_VALKYRIE2 | \
                                      SOC_INFO_CHIP_ENDURO | \
                                      SOC_INFO_CHIP_TD_TT | \
                                      SOC_INFO_CHIP_HURRICANE | \
                                      SOC_INFO_CHIP_KATANA | \
                                      SOC_INFO_CHIP_TRIUMPH3 | \
                                      SOC_INFO_CHIP_HELIX4)
#define SOC_INFO_CHIP_SC_CQ          (SOC_INFO_CHIP_SCORPION | \
                                      SOC_INFO_CHIP_CONQUEROR | \
                                      SOC_INFO_CHIP_SHADOW)
#define SOC_INFO_CHIP_HBX            (SOC_INFO_CHIP_HB_GW | SOC_INFO_CHIP_SC_CQ)
#define SOC_INFO_CHIP_TRX            (SOC_INFO_CHIP_TR_VL | \
                                      SOC_INFO_CHIP_SC_CQ | \
                                      SOC_INFO_CHIP_KATANA2 | \
                                      SOC_INFO_CHIP_HURRICANE2)
#define SOC_INFO_CHIP_FBX            (SOC_INFO_CHIP_FB_FX_HX | \
                                      SOC_INFO_CHIP_HB_GW | \
                                      SOC_INFO_CHIP_TRX)
#define SOC_INFO_CHIP_XGS3_SWITCH    (SOC_INFO_CHIP_FBX)
#define SOC_INFO_CHIP_XGS3_FABRIC    (SOC_INFO_CHIP_HUMV | \
                                      SOC_INFO_CHIP_CONQUEROR | \
                                      SOC_INFO_CHIP_TITAN | \
                                      SOC_INFO_CHIP_TITAN2)
#define SOC_INFO_CHIP_XGS_SWITCH     (SOC_INFO_CHIP_XGS3_SWITCH)
#define SOC_INFO_CHIP_XGS12_FABRIC   (SOC_INFO_CHIP_HERCULES)
#define SOC_INFO_CHIP_XGS_FABRIC     (SOC_INFO_CHIP_XGS12_FABRIC | \
                                      SOC_INFO_CHIP_XGS3_FABRIC)
#define SOC_INFO_CHIP_XGS            (SOC_INFO_CHIP_XGS_FABRIC | \
                                      SOC_INFO_CHIP_XGS_SWITCH)
    int             port_num;               /* count of entries in port_info */
    int             block_num;              /* count of entries in block_info */
    soc_ptype_t     fe;
    soc_ptype_t     ge;
    soc_ptype_t     ce;                     /* 100g port */
    soc_ptype_t     tpon;                   /* 10g PON port*/
    soc_ptype_t     pon;                    /* PON port*/
    soc_ptype_t     llid;                   /* LLID of epon*/ 
    soc_ptype_t     xe;
    soc_ptype_t     hg;
    soc_ptype_t     il;                     /* Interlaken */
    soc_ptype_t     sch;                    /* Stub Control Header */
    soc_ptype_t     hg_subport;             /* Higig subport */
    soc_ptype_t     hl;                     /* Higig Lite */
    soc_ptype_t     st;                     /* hg | ge (Higig Lite) */
    soc_ptype_t     gx;                     /* gx (Unicore) */
    soc_ptype_t     xl;                     /* xl (Warpcore) */
    soc_ptype_t     qsgmii;                 /* QSGMII ports */
    soc_ptype_t     mxq;                    /* MXQport (KA) */
    soc_ptype_t     xg;                     /* xg (Hyperlite + Octal Serdes) */
    soc_ptype_t     xq;                     /* xg (Unicore + Octal Serdes) */
    soc_ptype_t     xt;                     /* xport cores based port */ 
    soc_ptype_t     xw;                     /* clp-lite cores based port */
    soc_ptype_t     cl;                     /* cport and xport cores based port */
    soc_ptype_t     c;                      /* cport cores based port */
    soc_ptype_t     axp;                    /* Auxilliary port */
    soc_ptype_t     hyplite;                /* Hyperlite ports */
    soc_ptype_t     mmu;                    /* Internal to the MMU only */
    soc_ptype_t     spi;                    /* spi interface port */
    soc_ptype_t     spi_subport;            /* spi subport */
    soc_ptype_t     sci;                    /* fabric control */
    soc_ptype_t     sfi;                    /* fabric data */
    soc_ptype_t     req;                    /* requeue port */
    soc_ptype_t     tdm;                    /* TDM port */
    soc_ptype_t     rcy;                    /* RCY port */

    soc_ptype_t     port;                   /* fe|ge|xe|hg|spi|fl */
    soc_ptype_t     ether;                  /* fe|ge|xe */
    soc_ptype_t     all;                    /* fe|ge|xe|hg|cmic|fl */
    soc_ptype_t     pp;                     /* Packet Processing Port */
    soc_ptype_t     lp;                     /* Link PHY Port */
    soc_ptype_t     subtag;                 /* SubTag Port */
    pbmp_t          cmic_bitmap;            /* cmic bitmap */
    pbmp_t          lmd_pbm;                /* LMD port bitmap */
    pbmp_t          spi_bitmap;             /* spi bitmap */
    pbmp_t          s_pbm;                  /* 2.5 Gpbs Comboserdes ports */
    pbmp_t          gmii_pbm;               /* GMII port bitmap */
    pbmp_t          hg2_pbm;                /* HiGig2 encap port bitmap */
    pbmp_t          lb_pbm;                 /* Internal loopback ports */
    pbmp_t          mmu_pbm;                /* Ports internal to the MMU */
    pbmp_t          eq_pbm;                 /* Extended queuing ports */
    pbmp_t          management_pbm;         /* Management ports */
    pbmp_t          oversub_pbm;            /* Oversubscription ports */
    pbmp_t          pbm_ext_mem;            /* Pors with buffer in exteral DRAM*/
    pbmp_t          xpipe_pbm;              /* Ports in X-pipe */
    pbmp_t          ypipe_pbm;              /* Ports in Y-pipe */
    pbmp_t          tdm_pbm;                /* TDM port bitmap */
    pbmp_t          rcy_pbm;                /* recycling port bitmap */
    pbmp_t          physical_pbm;           /* physical port bitmap */
    pbmp_t          linkphy_pbm;            /* LinkPHY enabled port bitmap */
    pbmp_t          subtag_pbm;             /* 3rd Vlan SubTag enabled port bitmap */
    pbmp_t          subtag_allowed_pbm;     /* port bitmap on which 3rd Vlan SubTag is allowed at runtime */
    pbmp_t          linkphy_pp_port_pbm;    /* pp_port bitmap reserved for LinkPHY*/
    pbmp_t          subtag_pp_port_pbm;     /* pp_port bitmap reserved for SubTag*/
    int             linkphy_enabled;        /* LinkPHY enabled status */
    int             subtag_enabled;         /* SubTag enabled status */
    int             physical_port_offset;   /* first physical port of direct mapping*/
    int             lb_port;                /* loopback port */
    uint16          pcie_phy_addr;          /* MDIO Address of the PCIE Phy*/
    int             management_port;        /* 1G managment port */
    int             axp_port[SOC_AXP_NLF_NUM]; /* AXP NLF ports */
    int             ipic_port;              /* ipic port or -1 */
    int             ipic_block;             /* ipic block or -1 */
    int             olp_port[MAX_NUM_OF_CORES];     /* offload processor port */
    int             erp_port[MAX_NUM_OF_CORES];     /* egress replocation port */
    int             oamp_port[MAX_NUM_OF_CORES];    /* OAM processor port */
    int             rcpu_port;               /* RCPU communication port */
    int             rcy_port_start;         /* recycling port start */
    int             rcy_port_count;         /* number of recycling ports */
    int             cmic_port;              /* cmic port */
    int             cmic_block;             /* cmic block */
    int             arl_block;              /* arl block */
    int             mmu_block;              /* mmu block */
    int             mcu_block;              /* mcu block */
    int             ipipe_block;            /* ipipe block (FB) */
    int             ipipe_hi_block;         /* ipipe_hi block (FB) */
    int             epipe_block;            /* epipe block (FB) */
    int             epipe_hi_block;         /* epipe_hi block (FB) */
    int             igr_block;              /* Ingress block (ER) */
    int             egr_block;              /* Egress block (ER) */
    int             bse_block;              /* bse block (ER) */
    int             cse_block;              /* cse block (ER) */
    int             hse_block;              /* hse block (ER) */
    int             bsafe_block;            /* BSAFE block (FB/ER) */
    int             otpc_block;             /* OTPC block (Raven) */
    int             esm_block;              /* External Search Machine block (TR) */
    int             pgw_cl_block[8];        /* PGW_CL blocks */
    int             pg4_block[2];           /* PORT_GROUP4 blocks (TD) */
    int             pg5_block[2];           /* PORT_GROUP5 blocks (TD) */
    int             exp_port;               /* expansion port or -1 */
    int             exp_block;              /* expansion block or -1 */
    int             spi_port;               /* spi port */ /* Add for Robo */
    int             spi_block;              /* spi block */
    int             inter_block;            /* internal mii block*/
    int             exter_block;            /* external mii block*/
    int             ms_isec_block[2];       /* MACSEC ISEC */
    int             ms_esec_block[2];       /* MACSEC ESEC */
    int             il_block[SOC_MAX_NUM_ILPORT_BLKS]; /* Interlaken blocks */
    int             top_block;  
    int             ser_block;
    int             axp_block;
    int             ism_block;
    int             etu_block;
    int             etu_wrap_block;
    int             ibod_block;
    int             lls_block;              /* link list scheduler block */
    int             ces_block;              /* Circuit Emulation Service Interface */
    int             cw;                     /* CW Block */
    int             iproc_block;
    /* SBX only blocks */
    int             bp_block;               /* bypass block */
    int             cs_block;               /* cs block */
    int             eb_block;               /* eb block */
    int             ep_block;               /* ep block */
    int             es_block;               /* es block */
    int             fd_block;               /* fd block */
    int             ff_block;               /* ff block */
    int             fr_block;               /* fr block */
    int             tx_block;               /* tx block */
    int             qma_block;              /* qm block a */
    int             qmb_block;              /* qm block b */
    int             qmc_block;              /* qm block c */
    int             qsa_block;              /* qs block a */
    int             qsb_block;              /* qs block b */
    int             rb_block;               /* rb block */
    int             sc_top_block;           /* sc top block */
    int             sf_top_block;           /* sf top block */
    int             ts_block;               /* ts block */
    int             ci_block[SOC_MAX_NUM_CI_BLKS];         /* ci blocks    */
    int             xport_block[SOC_MAX_NUM_XPORT_BLKS];   /* xport block  */
    int             xtport_block[SOC_MAX_NUM_XTPORT_BLKS]; /* xtport block  */
    int             clport_block[SOC_MAX_NUM_CLPORT_BLKS]; /* clport block */
    int             xlport_block;           /* xlport block */
    int             cm_block;
    int             co_block[SOC_MAX_NUM_COP_BLKS];   
    int             cx_block; 
    int             pd_block; 
    int             pr_block[SOC_MAX_NUM_PR_BLKS]; 
    int             pt_block[SOC_MAX_NUM_PT_BLKS]; 
    int             qm_block; 
    int             tma_block;  
    int             tmb_block;
    int             tp_block[SOC_MAX_NUM_TP_BLKS];
    int             tm_qe_block[SOC_MAX_NUM_TM_QE_BLKS];   
    int             pb_block; 
    int             oc_block; 
    int             rc_block; 
    int             lra_block; 
    int             lrb_block; 
    int             pp_block; 
    int             txlp_block[SOC_MAX_NUM_TXLP_BLKS];
    int             rxlp_block[SOC_MAX_NUM_RXLP_BLKS];
    int             pmq_block[SOC_MAX_NUM_PMQ_BLKS];
    int             nof_pmqs;
    int             avs_block;
    
        /* DPP & DFE blocks */
        int             cfc_block;      
        int             ocb_block;         
        int             crps_block;            
        int             eci_block;  
        int             egq_blocks[SOC_MAX_NUM_EGQ_BLKS];
        int             fcr_block; 
        int             fct_block;
        int             fdr_block;      
        int             fda_block;
        int             fdt_block;    
        int             mesh_topology_block;      
        int             idr_block;            
        int             ihb_blocks[SOC_MAX_NUM_IHB_BLKS];
        int             ihp_blocks[SOC_MAX_NUM_IHP_BLKS];
        int             ips_blocks[SOC_MAX_NUM_IPS_BLKS];
        int             ipt_block;               
        int             iqm_blocks[SOC_MAX_NUM_IQM_BLKS];
        int             ire_block;           
        int             irr_block;               
        int             fmac_block[SOC_MAX_NUM_FMAC_BLKS];            
        int             xlp_block[SOC_MAX_NUM_XLP_BLKS];               
        int             clp_block[SOC_MAX_NUM_CLP_BLKS];       
        int             nbi_block;           
        int             cgm_blocks[SOC_MAX_NUM_CGM_BLKS];
        int             oamp_block;               
        int             olp_block;       
        int             fsrd_block[SOC_MAX_NUM_FSRD_BLKS];           
        int             rtp_block;               
        int             sch_blocks[SOC_MAX_NUM_SCH_BLKS];
        int             epni_blocks[SOC_MAX_NUM_EPNI_BLKS];
        int             mrps_blocks[SOC_MAX_NUM_MRPS_BLKS];
        int             drca_block;       
        int             drcb_block;           
        int             drcc_block;                 
        int             drcd_block;            
        int             drce_block;               
        int             drcf_block;       
        int             drcg_block;           
        int             drch_block;  
        int             drcbroadcast_block; 
        int             occg_block;
        int             dch_block[SOC_MAX_NUM_DCH_BLKS]; 
        int             dcl_block[SOC_MAX_NUM_DCL_BLKS]; 
        int             dcma_block[SOC_MAX_NUM_DCMA_BLKS]; 
        int             dcmb_block[SOC_MAX_NUM_DCMB_BLKS]; 
        int             dcmc_block;
        int             ccs_block[SOC_MAX_NUM_CCS_BLKS]; 
        int             edb_block;
        int             ilkn_pmh_block;
        int             ipst_block;
        int             ilkn_pml_block;
        int             iqmt_block;
        int             ppdb_a_block;
        int             ppdb_b_block;
        int             nbil_block;
        int             nbih_block;
        int             brdc_fmach_block;
        int             brdc_fmacl_block;
        int             brdc_fsrd_block;
        int             gport_block[SOC_MAX_NUM_GPORT_BLKS];
        /* fe 3200 blocks*/
        int             dcm_block[SOC_MAX_NUM_DCM_BLKS];
        int             brdc_dch_block;
        int             brdc_dcm_block;
        int             brdc_dcl_block;
        int             brdc_ccs_block;
        int             brdc_fmac_ac_block;
        int             brdc_fmac_bd_block;
        int             brdc_fmac_block;
        int             brdc_cgm_block;
        int             brdc_egq_block;
        int             brdc_epni_block;
        int             brdc_ihb_block;
        int             brdc_ihp_block;
        int             brdc_ips_block;
        int             brdc_iqm_block;
        int             brdc_sch_block;
        int             p3_cfg_block;
        int             p3_em_block;
        int             p3_table_block;
        int             p3_tcam_block;

    soc_block_t     has_block[SOC_MAX_NUM_BLKS];          /* list of all blocks */
    /*
     * The broadcast_blocks pointer will be null for blocks that are not broadcast blocks.
     * For broadcast block it points to an array of the block indexes of the blocks written to when writing to the broadcast block.
     * The size of the array (number of broadcast members) can be found using broadcast_blocks_size.
     */
    int             *broadcast_blocks[SOC_MAX_NUM_BLKS];
    uint8           broadcast_blocks_size[SOC_MAX_NUM_BLKS];
    int             port_offset[SOC_MAX_NUM_PORTS];       /* 3 for ge3 */
    int             port_speed_max[SOC_MAX_NUM_PORTS];    /* max port speed */
    int             port_group[SOC_MAX_NUM_PORTS];        /* group number */
    int             port_serdes[SOC_MAX_NUM_PORTS];       /* serdes number */
    int             port_num_subport[SOC_MAX_NUM_PORTS]; /* number of subport */
    int             port_subport_base[SOC_MAX_NUM_PORTS];/* subport base */
    #if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
    char            port_name[SOC_MAX_NUM_PP_PORTS][11];
    int             port_type[SOC_MAX_NUM_PP_PORTS];
    int             port_num_cosq[SOC_MAX_NUM_PP_PORTS];     /* cosq/mcast cosq */
    int             port_cosq_base[SOC_MAX_NUM_PP_PORTS];     /* cosq/mcast cosq base */
    int             port_num_uc_cosq[SOC_MAX_NUM_PP_PORTS];  /* ucast cosq */
    int             port_uc_cosq_base[SOC_MAX_NUM_PP_PORTS];  /* ucast cosq base */
    int             port_num_ext_cosq[SOC_MAX_NUM_PP_PORTS]; /* ext ucast cosq */
    int             port_ext_cosq_base[SOC_MAX_NUM_PP_PORTS]; /* ext ucast cosq base */
    #else
    char            port_name[SOC_MAX_NUM_PORTS][11];
    int             port_type[SOC_MAX_NUM_PORTS];
    int             port_num_cosq[SOC_MAX_NUM_PORTS];     /* cosq/mcast cosq */
    int             port_cosq_base[SOC_MAX_NUM_PORTS];     /* cosq/mcast cosq base */
    int             port_num_uc_cosq[SOC_MAX_NUM_PORTS];  /* ucast cosq */
    int             port_uc_cosq_base[SOC_MAX_NUM_PORTS];  /* ucast cosq base */
    int             port_num_ext_cosq[SOC_MAX_NUM_PORTS]; /* ext ucast cosq */
    int             port_ext_cosq_base[SOC_MAX_NUM_PORTS]; /* ext ucast cosq base */    
    #endif
    int             port_p2l_mapping[SOC_MAX_NUM_PORTS];  /* phy to logic */
    int             flex_port_p2l_mapping[SOC_MAX_NUM_PORTS];  /* flex(current) phy to logic */
    int             port_l2p_mapping[SOC_MAX_NUM_PORTS];  /* logic to phy */
    int             port_p2m_mapping[SOC_MAX_NUM_PORTS];  /* phy to mmu */
    int             max_port_p2m_mapping[SOC_MAX_NUM_PORTS];  /* max(flex) phy to mmu */
    int             port_m2p_mapping[SOC_MAX_NUM_PORTS];  /* mmu to phy */
    int             port_num_lanes[SOC_MAX_NUM_PORTS];    /* number of lanes */
    int             port_refclk_int[SOC_MAX_NUM_PORTS]; /*internal phy refclk*/
    int             phy_port_base[2];       /* per pipe physical port base */
    int             mmu_port_base[2];       /* per pipe mmu port base */
    char            block_valid[SOC_MAX_NUM_BLKS];        /* is block used? */
    pbmp_t          block_bitmap[SOC_MAX_NUM_BLKS];
    int             block_port[SOC_MAX_NUM_BLKS];
    char            block_name[SOC_MAX_NUM_BLKS][16];
    int             mem_block_any[NUM_SOC_MEM];
    int             *blk_fpp;               /* blk to first physical port mapping */
    int             bandwidth;              /* max core bandwidth */
    int             io_bandwidth;           /* max io bandwidth */
    int             frequency;              /* core clock frequency */
    int             modid_count;            /* needed modids */
    int             modid_max;              /* maximum modid supported */
    int             modport_max;            /* max port id in each module */
    int             port_class_max;         /* Number of supported port classes */
    int             trunk_bit_pos;          /* Bit-0 is 0 */
    uint32          vlan_xlate_glp_wildcard; /* Value of GLP_WILDCARD per chip */
    int             port_addr_max;          /* max addressable ports */
    int             hg_offset;              /* offset of hg ports in pbmp */
    int             cpu_hg_index;           /* table index for cpu port
                                             * higig packet where table indexed
                                             * by physical port*/
    int             cpu_hg_pp_port_index;   /* table index for cpu port higig
                                             * packet where table indexed by
                                             * pp_port */
    int             num_cpu_cosq;           /* number of cpu cosq */
    int             num_mmu_base_cosq;      /* number of MMU base cosq */
    int             num_time_interface;     /* number of time sync interfaces */
    int             internal_loopback;      /* internal loopback non-zero BW */ 
    char            lock_info[128];         /* lock debug info */
    int             num_flex_ingress_pools; /* number of ingress pools */
    int             size_flex_ingress_pool; /* size of ingress pools */
    int             num_flex_egress_pools;  /* number of egress pools */
    int             size_flex_egress_pool;  /* size of egress pools */
    int             global_meter_pools;     /* number of global meter pools*/
    int             global_meter_size_of_pool; /* Size of each pool */
    int             global_meter_max_size_of_pool; /* Maximum size of each pool */
    int             global_meter_action_size; /* Size of global meter action */
#ifdef BCM_EA_SUPPORT   
#ifdef BCM_TK371X_SUPPORT           
    /* Tk EPON only*/
    void            *private;               /* point the EPON soc_ea_chip_info_t */             
#endif
#endif
    soc_reg_access_t reg_access;
} soc_info_t;


typedef int (*soc_rcpu_schan_op_cb)(
    int unit, 
    schan_msg_t *msg, 
    int dwc_write,
    int dwc_read);

extern int soc_rcpu_schan_op_register(int unit, soc_rcpu_schan_op_cb f);
extern int soc_rcpu_schan_op_unregister(int unit);

 
typedef void (*soc_event_cb_t)(
    int                 unit,
    soc_switch_event_t  event,
    uint32              arg1,
    uint32              arg2,
    uint32              arg3,
    void                *userdata);

typedef struct list_event_cb_s {
   soc_event_cb_t           cb;
   void                     *userdata;
   struct list_event_cb_s   *next;
}soc_event_cb_list_t;

extern int soc_event_register(int unit, soc_event_cb_t cb, void *userdata);
extern int soc_event_unregister(int unit, soc_event_cb_t cb, void *userdata);
extern int soc_event_generate(int unit,  soc_switch_event_t event, 
                              uint32 arg1, uint32 arg2, uint32 arg3);
extern void soc_event_assert(const char *expr, const char *file, int line);



#ifdef BCM_WARM_BOOT_SUPPORT
extern int soc_scache_esw_nh_mem_write(int unit, uint8 *buf, 
                                       int offset, int nbytes);
extern int soc_scache_esw_nh_mem_read(int unit, uint8 *buf, 
                                      int offset, int nbytes);

#ifdef BROADCOM_DEBUG
extern int soc_scache_file_close(void);
extern int soc_scache_file_open(int unit, int warm_boot);
#endif /* BROADCOM_DEBUG */

extern int soc_switch_control_scache_init(int unit);
extern int soc_switch_control_scache_sync(int unit);

#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_CMICM_SUPPORT
/*Structures for UC message receive LL*/
/*LL Node*/
typedef struct ll_element_s {
    struct ll_element_s *p_next;
    struct ll_element_s *p_prev;
}ll_element_t;

/* Doubly linked list ctrl structure*/
typedef struct ll_ctrl_s {
    ll_element_t    *p_head;    /* pointer to the first element */
    ll_element_t    *p_tail;    /* pointer to the last element */
    uint32          ll_count;   /* the number of elements  the list */
}ll_ctrl_t;

typedef struct mos_msg_ll_node_s {
    ll_element_t  msg_q;        /*LL linkage*/
    mos_msg_data_t  msg_data;   /*Msg Data*/
}mos_msg_ll_node_t;
#endif

/*
 * Typedef: soc_control_t
 * Purpose: SOC Control Structure.  All info about a device instance.
 */

typedef struct soc_control_s {
    uint32      soc_flags; /* Flags for this device */
#define   SOC_F_ATTACHED           0x01    /* Device attached */
#define   SOC_F_INITED             0x02    /* Device inited */
#define   SOC_F_LSE                0x04    /* Link SCAN enabled in SW */
#define   SOC_F_SL_MODE            0x08    /* Device is in SL stack mode */
#define   SOC_F_POLLED             0x10    /* Polled IRQ mode */
#define   SOC_F_URPF_ENABLED       0x20    /* Unicast rpf enabled on device */
#define   SOC_F_IPMCREPLSHR        0x40    /* IPMC replication with lists */
#define   SOC_F_BUSY               0x80    /* Device is not fully out of Reset */
#define   SOC_F_MEM_CLEAR_USE_DMA  0x100   /* Device should use table dma
                                              for memory clear operations */
#define   SOC_F_GPORT              0x200   /* Device should use GPORT for
                                              source/destination notation */
#define   SOC_F_RCPU_SCHAN         0x400   /* Indicates that SCHAN operations will   
                                              be sent over RCPU mechanism */ 
#define   SOC_F_RCPU_ONLY          0x800   /* Indicates that all access to the device
                                              will be done through RCPU */
#define   SOC_F_RE_INITED          0x1000  /* Device re-inited while running */
#define   SOC_F_PORT_CLASS_BLOCKED 0x2000  /* Device is not allowed to change port classes */
#define   SOC_F_XLATE_EGR_BLOCKED  0x4000  /* Device is not allowed to setup old
                                              style vlan egress translate */
#define   SOC_F_KNET_MODE          0x8000  /* Device is in kernel networking mode */
#define   SOC_F_REMOTE_ENCAP       0x10000 /* Device is uses remote multicast encapsulation */
#define   SOC_F_MEM_CLEAR_HW_ACC   0x20000 /* Device should use h/w pipe clear
                                              for memory clear operations */

    /* Mask of flags that are persistent across init */
#define   SOC_F_RESET              (SOC_F_ATTACHED|SOC_F_INITED|SOC_F_POLLED|SOC_F_KNET_MODE)

    /* Port and block information filled in when attached or inited */
    soc_info_t  info;

#ifdef BCM_CALADAN3_SUPPORT
    /* Port information for reconfiguration */
    int             reconfig_flag;          /* True when doing reconfig or hotswap */
#define SOC_RECONFIG_TDM (SOC_CONTROL(unit)->reconfig_flag)
    int             hotswap_flag;          /* True when doing hotswap */
#define SOC_HOTSWAP_TDM (SOC_CONTROL(unit)->hotswap_flag)
    soc_sbx_caladan3_port_config_t  *interface_config;  /* For hotswap */
    soc_sbx_caladan3_port_queue_t   *port_queue_config; /* For hotswap */
    pbmp_t          all_skip_pbm;           /* All ports to skip during reconfiguration */
    pbmp_t          mac_phy_skip_pbm;       /* Skip phy init during reconfiguration */
    int             port_l2p_mapping[SOC_MAX_NUM_PORTS];
    int             intftype[SOC_MAX_NUM_PORTS];
    int             cl0_reset;              /* Indicates whether or not CL0 and PT0 need to be
                                               reset as part of reconfiguration */
#endif

    /* currently used by sbx devices only */
    void        *drv;

    /* Disable subsets of registers and memories */
    uint32      disabled_reg_flags;
    uint32      disabled_mem_flags;

    /* Miscellaneous chip state */
    uint32      gbpMemSize;     /* GBP size in bytes */
    int         gbpBanks;       /* Number of banks, 2 or 4 */
    int         gbpDDR;         /* Boolean for DDR vs non-DDR */
    int         gbpWidth;       /* 128 or 64 bits wide */
    int         gbpFullEnable;  /* GBP Full interrupts (see soc/intr.c) */
    int         gbpFullTime;

    int         pciParityDPC;   /* True if PCI Parity Error DPC queued */
    int         pciFatalDPC;    /* True if PCI Fatal Error DPC queued */

    uint32      irq_mask;       /* Cached copy of CMIC_IRQ_MASK */
    uint32      irq1_mask;      /* Cached copy of CMIC_IRQ_MASK_1 */
    uint32      irq2_mask;      /* Cached copy of CMIC_IRQ_MASK_2 */

    uint32        switch_event_nominal_storm;

#ifdef BCM_DDR3_SUPPORT
    int         ddr3_num_columns;
    int         ddr3_num_rows;
    int         ddr3_num_banks;
    int         ddr3_num_memories;
    int         ddr3_clock_mhz;
    int         ddr3_mem_grade;
    int         ddr3_offset_wr_dq_ci02_wl0;
    int         ddr3_offset_wr_dq_ci00_wl1;
#endif /* BCM_DDR3_SUPPORT */

    uint32      board_type;     /* From I2C */
    uint32      remote_cpu;
    soc_rcpu_schan_op_cb    soc_rcpu_schan_op; /* RCPU SCHAN function */

    /* Operation Management */

    sal_mutex_t schanMutex;     /* S-Channel mutual exclusion */
#ifdef BCM_CMICM_SUPPORT
    sal_mutex_t fschanMutex;     /* Fast S-Channel mutual exclusion */
    sal_mutex_t ccmDmaMutex;  /* CCM DMA register mutual exclusion */
    int         ccmDmaTimeout;/* Operation timeout in microseconds */
    sal_sem_t   ccmDmaIntr;   /* CCM DMA interrupt */
    int         ccmDmaIntrEnb;/* Use interrupts instead of polling */

    int         pci_cmc;
    int         arm_cmc[SOC_CMCS_NUM_MAX - 1];
    int         cmc_num;        /* Number of CMCs on this unit. */
    uint32      cmc_irq0_mask[SOC_CMCS_NUM_MAX]; /* Cached copy of CMIC_CMCx_PCIE_IRQ_MASK0 */
    uint32      cmc_irq1_mask[SOC_CMCS_NUM_MAX]; /* Cached copy of CMIC_CMCx_PCIE_IRQ_MASK1 */
    uint32      cmc_irq2_mask[SOC_CMCS_NUM_MAX]; /* Cached copy of CMIC_CMCx_PCIE_IRQ_MASK2 */
    uint32      cmc_irq3_mask[SOC_CMCS_NUM_MAX]; /* Cached copy of CMIC_CMCx_PCIE_IRQ_MASK3 */
    uint32      cmc_irq4_mask[SOC_CMCS_NUM_MAX]; /* Cached copy of CMIC_CMCx_PCIE_IRQ_MASK4 */
#ifdef BCM_RCPU_SUPPORT
    uint32      rpe_irq0_mask; /* Cached copy of CMIC_RPE_RCPU_IRQ_MASK0 */
    uint32      rpe_irq1_mask; /* Cached copy of CMIC_RPE_RCPU_IRQ_MASK1 */
    uint32      rpe_irq2_mask; /* Cached copy of CMIC_RPE_RCPU_IRQ_MASK2 */
    uint32      rpe_irq3_mask; /* Cached copy of CMIC_RPE_RCPU_IRQ_MASK3 */
    uint32      rpe_irq4_mask; /* Cached copy of CMIC_RPE_RCPU_IRQ_MASK4 */
    uint32      rpe_pcie_irq0_mask; /* Cached copy of CMIC_RPE_PCIE_IRQ_MASK0 */
    uint32      rpe_uc0_irq0_mask; /* Cached copy of CMIC_RPE_UC0_IRQ_MASK0 */
    uint32      rpe_uc1_irq0_mask; /* Cached copy of CMIC_RPE_RCPU_IRQ_MASK2 */
#endif /* BCM_RCPU_SUPPORT */
    SHR_BITDCLNAME(cpu_arm_queues[SOC_CMCS_NUM_MAX], NUM_CPU_COSQ_MAX);
                            /* Bitmap of CMIC queues allocated for ARM use */
    SHR_BITDCLNAME(cpu_arm_reserved_queues[SOC_CMCS_NUM_MAX], NUM_CPU_COSQ_MAX);
                            /* Bitmap of CMIC queues reserved for special purpose*/
    int         num_cpu_arm_cosq[SOC_CMCS_NUM_MAX];
                            /* number of CPU/ARM cosq */
#ifdef  BCM_KATANA_SUPPORT
    /* CES Service Control */
    void        *ces_ctrl;      /* CES control, cast to bcm_ces_service_global_config_t */
    void        *tdm_ctrl;      /* CES TDM control, cast to bcm_tdm_port_record_t */
    uint32      tdm_count;      /* Number of CES TDM ports */
#endif /* BCM_KATANA_SUPPORT */

    int         swIntrActive;
    sal_sem_t   swIntr[CMICM_SW_INTR_NUM_HOSTS]; /* For uc/host
                                                     communications */

    /* uC message structures */
    uint32      uc_msg_area;
    sal_mutex_t uc_msg_control;
    sal_sem_t   uc_msg_active[CMICM_NUM_UCS];

    sal_sem_t   uc_msg_rcv_sems[CMICM_NUM_UCS][MAX_MOS_MSG_CLASS + 1];
    sal_sem_t   uc_msg_ack_sems[CMICM_NUM_UCS][NUM_MOS_MSG_SLOTS];

    /* LL for Storing messages received per class */
    ll_ctrl_t   uc_msg_rcvd_ll[CMICM_NUM_UCS][MAX_MOS_MSG_CLASS + 1];
    
    uint8       *uc_msg_ack_data[CMICM_NUM_UCS][NUM_MOS_MSG_SLOTS];

    /* Semaphores for senders waiting for a free slot */
    sal_sem_t   uc_msg_send_queue_sems[CMICM_NUM_UCS];

    mos_msg_host_status_t uc_msg_prev_status_in[CMICM_NUM_UCS],
        uc_msg_prev_status_out[CMICM_NUM_UCS];

    /* Timeout properties */
    sal_usecs_t uc_msg_queue_timeout;           /* Free slot timeout */
    sal_usecs_t uc_msg_ctl_timeout;             /* Control seamphore timeout */
    sal_usecs_t uc_msg_send_timeout;            /* Ack/NAK wait timeout */
    sal_usecs_t uc_msg_send_retry_delay;        /* Delay btween send retries */

    /* System-type msg control */
    sal_mutex_t uc_msg_system_control;          
    uint8       uc_msg_system_count;            /* Incremnted by thread */
    
    /* tx_beacon info */
    int         txbeacon_init;                  /* Set on init */
    int         txbeacon_uC;                    /* ARM responsible for txb */
    sal_usecs_t txbeacon_msg_timeout;           /* Timeout for msgs */

#ifdef BCM_SBUSDMA_SUPPORT
    uint8       max_sbusdma_channels;
    int8        tdma_ch;
    int8        tslam_ch;
    int8        desc_ch;
    soc_sbusdma_desc_info_t *sbd_dm_inf;
                                                /* per CMC/CH sbusDMA register mutual exclusion */
    sal_mutex_t sbusDmaMutexs[SOC_CMCS_NUM_MAX][SOC_SBUSDMA_CH_PER_CMC];
                                                /* per CMC/CH sbusDMA DONE interrupt */
    sal_sem_t   sbusDmaIntrs[SOC_CMCS_NUM_MAX][SOC_SBUSDMA_CH_PER_CMC];
#endif /* BCM_SBUSDMA_SUPPORT */
#endif /* BCM_CMICM_SUPPORT */

    int         schanTimeout;   /* Operation timeout in microseconds */
    sal_sem_t   schanIntr;      /* S-Channel DONE interrupt notification */
    int         schanIntrEnb;   /* Allow interrupts to be used */
    sal_usecs_t schanIntrBlk;   /* Length of time to block the S-channel
                                   error interrupt after one occurs */
    uint32      schan_result;   /* CMIC_SCHAN_CTRL register after
                                   last S-channel operation */
    uint32      schan_override; /* ER cmd memory write override during warm reboot */

    sal_mutex_t miimMutex;      /* MII registers mutual exclusion */
    int         miimTimeout;    /* Operation timeout in microseconds */
    sal_sem_t   miimIntr;       /* MII DONE interrupt notification */
    int         miimIntrEnb;    /* Use interrupts instead of polling */

    int         bistTimeout;    /* Operation timeout in milliseconds */

    sal_mutex_t tableDmaMutex;  /* Table DMA register mutual exclusion */
    int         tableDmaTimeout;/* Operation timeout in microseconds */
    sal_sem_t   tableDmaIntr;   /* Table DMA DONE interrupt FB/ER */
    int         tableDmaIntrEnb;/* Use interrupts instead of polling */

    sal_mutex_t tslamDmaMutex;  /* Table SLAM DMA register mutual exclusion */
    int         tslamDmaTimeout;/* Operation timeout in microseconds */
    sal_sem_t   tslamDmaIntr;   /* Table SLAM DMA DONE interrupt FB/ER */
    int         tslamDmaIntrEnb;/* Use interrupts instead of polling */

    sal_mutex_t socControlMutex;/* SOC control structure mutual exclusion */
    sal_mutex_t counterMutex;   /* Counter activity mutual exclusion */
    sal_thread_t HwDisableThreadId; /*Thread ID use to evoide deadlock state while allowing warmboot write*/
    sal_mutex_t HwDisableMutex; /* mutex for toggling hw disable flag */

    int         memCmdTimeout;  /* Operation timeout in microseconds */
    sal_sem_t   memCmdIntr[3];  /* Memory command interrupt ER */
    int         memCmdIntrEnb;  /* Use interrupts instead of polling */

    sal_sem_t   ipfixIntr;      /* IPFIX FIFO DMA interrupt */
    int         ipfixIntrEnb;   /* Use interrupts instead of polling */
 
    sal_sem_t   ftreportIntr;      /* IPFIX FIFO DMA interrupt */
    int         ftreportIntrEnb;   /* Use interrupts instead of polling */

    int         l2modDmaIntrEnb; /* Use interrupts instead of polling */

    int         sbusCmdSpacing; /* Multiple outstanding sbus command spacing */
 
    sal_mutex_t overrideMutex;  /* OVERRIDE registers mutual exclusion */

    /* MAC IPG Configuration */

    soc_ipg_t   ipg;
    /* Statistics counts */

    soc_stat_t  stat;

    /* On-chip memory management */

    soc_memstate_t
                memState[NUM_SOC_MEM];
    uint8       tcam_protect_write;     /* Duplicate existing entry before
                                         * overwrite */
    uint8       *mem_scache_ptr;        /* mem scache pointer */
    uint8       skip_cache_use;         /* Mem test mode to avoid cache use */
    uint8       force_read_through;     /* Read from h/w even if cached */
    uint8       cache_coherency_chk;    /* Check cache coherency */
    uint8       dma_from_mem_cache;     /* DMA from mem cache */
    uint8       parity_enable;          /* SER enabled */
    uint8       parity_correction;      /* SER correction enabled */
    uint8       parity_counter_clear;   /* 0:Clear or 1:Restore counter on SER event */
    uint8       return_reg_error;       /* Return error on SER error */
    uint32      alpm_mem_ops;           /* Bitmap for alpm mem ops executed */
#define _SOC_ALPM_LOOKUP                0
#define _SOC_ALPM_INSERT                1
#define _SOC_ALPM_DELETE                2
    sal_sem_t   alpm_bulk_retry;        /* ALPM bulk op retry handshake semaphore */
    sal_sem_t   alpm_lookup_retry;      /* ALPM lookup op retry handshake semaphore */
    sal_sem_t   alpm_insert_retry;      /* ALPM insert op retry handshake semaphore */
    sal_sem_t   alpm_delete_retry;      /* ALPM delete op retry handshake semaphore */
    sal_mutex_t l3x_lock;               /* L3 module lock */
    uint8       l3_defip_urpf;          /* urpf_route_enabled */
    int         l3_defip_index_remap;   /* l3_defip index remap offset */
    int         l3_defip_max_tcams;     /* Number of defip tcams */
    int         l3_defip_tcam_size;     /* size of each defip tcam */  
    _soc_l3_defip_index_table_t *defip_index_table;
    int         l3_defip_max_128b_entries; /* Number of 128B V6 entries */ 
    uint8       *l3_defip_scache_ptr;  /* handle to save/resote l3_defip_max_128b_entries */

#ifdef INCLUDE_MEM_SCAN
    VOL sal_thread_t
                mem_scan_pid;
    char        mem_scan_name[16];      /* Memory scan task name */
    VOL int     mem_scan_rate;          /* Max entries to scan per pass */
    VOL sal_usecs_t
                mem_scan_interval;      /* Task wake up interval */
    sal_sem_t   mem_scan_notify;
#endif

#if defined(BCM_TRIUMPH3_SUPPORT)
    VOL sal_thread_t
                esm_recovery_pid;
    sal_sem_t   esm_recovery_notify;    /* ESM recovery handshake semaphore */
    sal_mutex_t esm_lock;
    int         esm_recovery_enable;
#endif

#ifdef BCM_ISM_SUPPORT
    _soc_ism_t *ism;            /* ISM info structure */
    _soc_ism_hash_t *ism_hash;  /* ISM hash info structure */
#endif /* BCM_ISM_SUPPORT */

    /* Hardware ARL Table */

    VOL sal_thread_t
                arl_pid;        /* Software ARL message task */
    char        arl_name[16];   /* ARL message task name */
    int         arl_exit;       /* Request for ARL thread to exit */
    sal_sem_t   arl_notify;
    VOL sal_usecs_t
        arl_interval;   /* Interval between L2 table sync passes */
    int         arl_notified;   /* TRUE if notify semaphore already given */
    int         arl_dma_cnt0;   /* can only be set in intr; only cleared
                                   when done processing in arl thread. */
    int         arl_dma_xfer;   /* can only be set in intr; only cleared
                                   when done processing in arl thread. */
    int         arl_mbuf_done;  /* can only be set in intr; only cleared
                                   when done processing in arl thread. */
    int         arl_msg_drop;   /* can only be set in intr; only cleared
                                   when processed in arl_mbuf_process. */
    sal_time_t  arlDropWarn;    /* Timestamp of last warning msg printed */
    uint32      arlResyncDelay; /* Time after drop before resync (0=never) */
    int         arlBufSize;     /* Number of entries in arlBuf */
    schan_msg_arlins_t
                *arlBuf;        /* ARL DMA Message Buffer */
    schan_msg_arlins_t
                *arlPtr;        /* Consumer pointer */
    shr_avl_t   *arlShadow;     /* Shadow copy of ARL or L2X table */
    shr_avl_t   *arlShadow_ext1;/* Shadow copy of EXT_L2_1 table */
    shr_avl_t   *arlShadow_ext2;/* Shadow copy of EXT_L2_2 table */
    sal_mutex_t arlShadowMutex;
    int         arlMode;        /* Current ARL mode */
    int         arlNextBuf;     /* Poll mode: next ARL Message Buffer (0-3) */
    int         arlRateLimit;   /* Limit in msgs/sec */

#ifdef BCM_ROBO_SUPPORT
    /* robo */
    l2_arl_sw_entry_t *arl_table;
#endif /* BCM_ROBO_SUPPORT */
    sal_mutex_t arl_table_Mutex;
    sal_mutex_t arl_mem_search_Mutex;
    sal_mutex_t mem_rwctrl_reg_Mutex;


    /* Hardware L2X Table */

    VOL sal_thread_t
                l2x_pid;        /* Software L2 shadow task */
    char        l2x_name[16];   /* L2X shadow task name */
    int         l2x_mode;       /* L2MODE_POLL or L2MODE_FIFO */
    VOL sal_usecs_t
                l2x_interval;   /* Interval between L2 table sync passes */
    int         l2x_age_timeout; /* Timeout for h/w l2x age to complete */

#ifdef BCM_TRIUMPH3_SUPPORT
    VOL sal_usecs_t
                l2x_agetime_adjust_usec; /* usec waited on sem before
                                            age thread stops */
    VOL sal_usecs_t
                l2x_agetime_curr_timeblk; /* Current timeout block index
                                            , each blk is 2147sec. */
    VOL sal_usecs_t
                l2x_agetime_curr_timeblk_usec; /* amount of usec spent on sem
                                                of current 2147sec time blk */
    VOL sal_usecs_t
                l2x_prev_age_timeout; /* stores prev age timeout between
                                        thread start and stop */
#endif
    VOL sal_thread_t
                l2x_age_pid;    /* Software L2 age task */
    char        l2x_age_name[16]; /* L2X age task name */
#ifdef BCM_TRIUMPH3_SUPPORT
    int         l2x_age_hitsa_only;
#endif
    sal_sem_t   l2x_age_notify;
#ifdef BCM_TRIDENT2_SUPPORT
    sal_sem_t   l2x_age_sync;
    int         l2x_age_enable;
    int         l2x_sw_aging;
#endif
    VOL sal_usecs_t
                l2x_age_interval; /* Interval between L2 table age passes */
    VOL uint32  l2x_flags;
    int         l2x_ppa_in_progress;
    int         l2x_ppa_bypass;
    sal_sem_t   l2x_notify;
    sal_mutex_t l2x_del_sync;
#define SOC_L2_DEL_SYNC_LOCK(s) \
    sal_mutex_take((s)->l2x_del_sync, sal_mutex_FOREVER)

#define SOC_L2_DEL_SYNC_UNLOCK(s)  sal_mutex_give((s)->l2x_del_sync)

    sal_sem_t   l2x_lock;
    uint8       l2x_shadow_hit_bits;
#       define  L2X_SHADOW_HIT_BITS 0x1 /* global hit bits control */
#       define  L2X_SHADOW_HIT_SRC  0x2 /* source hit bit control */
#       define  L2X_SHADOW_HIT_DST  0x4 /* destination hit bit control */

    int         l2x_l2mc_in_l2entry;
    int         l2x_external;
    int         l2x_group_enable;
    int         l2_overflow_active;      /* L2 overflow mechanism active */
    int         l2_overflow_enable;      /* L2 overflow mechanism enabled */
    /* Hash keys configuration */
    uint32      hash_key_config;
#       define  L3X_IPMC_ENABLE  0x0001  /* ipmc is enabled */
#       define  L3X_IPMC_SIP     0x0002  /* lookup includes src ip */
#       define  L3X_IPMC_SIP0    0x0004  /* lookup with src ip set to zero */
#       define  L3X_IPMC_VLAN    0x0008  /* lookup includes vlan */

    /* Hash mask cache */

    uint32      hash_mask_l2x;
    int         hash_bits_l2x;
    uint32      hash_mask_l3x;
    int         hash_bits_l3x;
    uint32      hash_mask_vlan_mac;
    int         hash_bits_vlan_mac;
    uint32      hash_mask_egr_vlan_xlate;
    int         hash_bits_egr_vlan_xlate;
    uint32      hash_mask_mpls;
    int         hash_bits_mpls;
    uint32      hash_mask_l2x_ext;
    int         hash_bits_l2x_ext;
    uint32      hash_mask_l3v6;
    int         hash_bits_l3v6;
    uint32      hash_mask_my_station;
    int         hash_bits_my_station;
    uint32      hash_mask_wlan_client;
    int         hash_bits_wlan_client;
    uint32      hash_mask_wlan_port;
    int         hash_bits_wlan_port;
    uint32      hash_mask_ing_vp_vlan_member;
    int         hash_bits_ing_vp_vlan_member;
    uint32      hash_mask_egr_vp_vlan_member;
    int         hash_bits_egr_vp_vlan_member;
    uint32      hash_mask_ing_dnat_address_type;
    int         hash_bits_ing_dnat_address_type;
    uint32      hash_mask_l2_endpoint_id;
    int         hash_bits_l2_endpoint_id;
    uint32      hash_mask_endpoint_queue_map;
    int         hash_bits_endpoint_queue_map;
    uint32      hash_mask_ft_session_ipv4;
    int         hash_bits_ft_session_ipv4;
    uint32      hash_mask_ft_session_ipv6;
    int         hash_bits_ft_session_ipv6;

    /* Hash recurse depths */
    uint32      dual_hash_recurse_depth;
    uint32      dual_hash_recurse_depth_l2x;
    uint32      dual_hash_recurse_depth_mpls;
    uint32      dual_hash_recurse_depth_vlan;
    uint32      dual_hash_recurse_depth_egress_vlan;
#if defined(INCLUDE_L3)
    uint32      dual_hash_recurse_depth_l3x;
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
    uint32      dual_hash_recurse_depth_wlan_port;
    uint32      dual_hash_recurse_depth_wlan_client;
    uint32      dual_hash_recurse_depth_ft_session_ipv4;
    uint32      dual_hash_recurse_depth_ft_session_ipv6;
#endif
    uint32      dual_hash_recurse_depth_ing_vp_vlan_member;
    uint32      dual_hash_recurse_depth_egr_vp_vlan_member;
    uint32      dual_hash_recurse_depth_ing_dnat_address_type;
    uint32      dual_hash_recurse_depth_l2_endpoint_id;
    uint32      dual_hash_recurse_depth_endpoint_queue_map;
    uint32      multi_hash_recurse_depth;
    uint32      multi_hash_recurse_depth_l2;
    uint32      multi_hash_recurse_depth_l3;
    uint32      multi_hash_recurse_depth_mpls;
    uint32      multi_hash_recurse_depth_vlan;
    uint32      multi_hash_recurse_depth_egress_vlan;

    /* Multi-Hash move recurse algorithms */
#define         MULTI_HASH_MOVE_ALGO_DEPTH        0x0    /* Depth first recurse */
#define         MULTI_HASH_MOVE_ALGO_BREADTH      0x1    /* Breadth first recurse */
    uint32      multi_hash_move_algorithm;

    /* Multicast ranges */

    int mcast_size;
    int ipmc_size;
    int higig2_bcast_size;
    int higig2_mcast_size;
    int higig2_ipmc_size;

    int mcast_add_all_router_ports;

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    pbmp_t repl_eligible_pbmp; /* Ports eligible for packet replication */
#endif

    /* Switching logic bypass mode */
    int switch_bypass_mode;

    /* Double tag mode when it is a device-wide property */
    int dt_mode;

    /* Link status */

    pbmp_t      link_mask2;     /* Override for LCC drain */

    int         soc_link_pause; /* Link scan pause count */
    pbmp_t      hw_linkscan_pbmp;    /* Cached HW linkscan port bitmap */
    void        (*soc_link_callout)(int);

    pbmp_t      link_fwd;    /* Forwarding ports current value */

    void        (*soc_time_callout)(int);   
    int         time_call_ref_count;

#ifdef BCM_LINKSCAN_LOCK_PER_UNIT
    sal_mutex_t linkscan_mutex; 
#endif

    /* DMA Management */
    uint32  soc_robo_dma_lock;  /* Lock for updating DMA operations etc. */
#   define SOC_DMA_LOCK(l)      (l = sal_splhi())
#   define SOC_DMA_UNLOCK(l)    (void)sal_spl(l)
    sdc_t       soc_channels[N_DMA_CHAN];
    int         soc_max_channels;       /* maximum channel count */
    sdc_t       *soc_dma_default_rx;    /* Default RX channel */
    sdc_t       *soc_dma_default_tx;    /* Default TX channel */
    int         dma_droptx;             /* Any channels in drop tx mode */
    dv_t        *soc_dv_rx_free;        /* Available RX DVs */
    int         soc_dv_rx_free_cnt;     /* # on RX free list */
    dv_t        *soc_dv_tx_free;        /* Available TX DVs */
    int         soc_dv_tx_free_cnt;     /* # on TX free list */
    int         soc_dv_cnt;             /* # allowed on free list */
    int         soc_dv_size;            /* Number DCBs in free list entries */
    uint32      *tx_purge_pkt;          /* DMA able buffer for TX Purge */

    dcb_op_t    *dcb_op;                /* DCB operations */

    /*
     * Packet Counter Management
     *
     * The hardware counter buffer is allocated using I/O-safe
     * memory because counter values may be transferred to the buffer
     * via DMA.  The software counter buffer uses regular memory.
     */

    int         counter_portsize;       /* Bytes per port for counters */
    int         counter_bufsize;        /* Total memory allocated for dma */
    int         counter_n32;            /* Number of 32-bit counters */
    int         counter_n64;            /* Number of 64-bit counters */
    int         counter_n64_non_dma;    /* Number of non DMA 64-bit counters */
    int         counter_ports32;        /* Number of 32-bit ports */
    int         counter_ports64;        /* Number of 64-bit ports */
    int         counter_perport;        /* Number of counter per port */
    uint32      *counter_buf32;         /* Hardware counter DMA buffer */
    uint64      *counter_buf64;         /* Hardware counter DMA buffer */
    uint64      *counter_hw_val;        /* Hardware counters */
    uint64      *counter_sw_val;        /* Software accumulated counters */
    uint64      *counter_delta;         /* Deltas over last two collections */
    soc_cmap_t  *counter_map[SOC_MAX_NUM_PORTS];        /* per port cmap */

    VOL sal_thread_t
                counter_pid;            /* Counter DMA emulation task */
    char        counter_name[16];       /* Counter task name */
    VOL int     counter_interval;       /* Update interval in usec */
                                        /*   (Zero when thread not running) */
    sal_usecs_t counter_coll_prev;      /* Timestamp of previous collection */
    sal_usecs_t counter_coll_cur;       /* Timestamp of current collection */
    VOL pbmp_t  counter_pbmp;           /* Ports being scanned */
    VOL uint32  counter_flags;          /* SOC_COUNTER_F_XXX */
    VOL sal_sem_t
                counter_trigger;        /* Trigger counter thread attn */
    VOL sal_sem_t
                counter_intr;           /* Counter H/W interrupt notifier */
    VOL int     counter_sync_req;       /* soc_counter_sync() support */
    VOL int     counter_suspend;        /* Skip update for another interval */
    VOL int     counter_updating;       /* Processing update cycle */
    VOL sal_sem_t
        counter_notify;     /* Scan complete notification */
    int     counter_swap64;     /* DMA buf 64-bit values must
                       be word-swapped */
    soc_counter_non_dma_t *counter_non_dma; /* non counter DMA support
                                             * entries */
    soc_controlled_counter_t *controlled_counters; /* controlled counters support */
    uint32 soc_controlled_counter_flags; /* controlled counters support */
    uint32 soc_controlled_counter_num; /*number of collected counters*/
    uint32 soc_controlled_counter_all_num; /*all controlled counters*/
    shr_fifo_t *counter_timestamp_fifo[SOC_MAX_NUM_PORTS];
                                        /* Timestamp FIFO data */
    soc_blk_ctr_reg_desc_t *blk_ctr_desc;
    uint32      blk_ctr_desc_count;
    uint64      *blk_ctr_buf;
    uint64      *blk_counter_hw_val;
    uint64      *blk_counter_sw_val;
    uint32      blk_ctr_count;

    /* Memory correction status */

    soc_mmu_error_t *mmu_errors;        /* Statistics tracking for MMU */
    uint32      *lla_cells_good;        /* LLA/PP entries known good */
    SHR_BITDCL  **lla_map;              /* LLA/PP analysis cache */
    int         *sbe_disable;           /* PP has entries w/ SBE errors */

    SHR_BITDCL  *port_lls_l0_bmap[SOC_MAX_NUM_PORTS]; /* LLS L0 usage map */
    SHR_BITDCL  *port_lls_l1_bmap[SOC_MAX_NUM_PORTS]; /* LLS l1 usage map */
    SHR_BITDCL  *port_lls_l2_bmap[SOC_MAX_NUM_PORTS]; /* LLS L2 usage map */
#if defined(BCM_KATANA2_SUPPORT)
    SHR_BITDCL  *port_lls_s0_bmap[SOC_MAX_NUM_PORTS]; /* LLS S0 usage map */
    SHR_BITDCL  *port_lls_s1_bmap[SOC_MAX_NUM_PORTS]; /* LLS S1 usage map */
#endif /*BCM_KATANA2_SUPPORT*/

    int         mmu_error_block;        /* Disable MMU errors */
    /* Chip driver pointers */

    soc_driver_t
                *chip_driver;
    soc_functions_t
                *soc_functions;

    pbmp_t      **reg_subset_mask;      /* Reg mask subset arrays */

    void        *i2c_bus;               /* Cast to soc_i2c_bus_t */
    void        *bsc_bus;               /* Cast to soc_bsc_bus_t */
    uint16      max_vrf_id;             /* Maximum vrf id on the device. */
    uint16      max_address_class;      /* Maximum src/dst class         */ 
                                        /* on the device.                */
    uint16      max_overlaid_address_class;  /* Maximum overlaid src/dst class */ 
                                             /* on the device. */
    uint16      max_extended_address_class;  /* Maximum extended src/dst class */ 
                                             /* on the device. */
    uint16      max_interface_class;    /* Maximum interface class       */
                                        /* on the device.                */
    uint16      mem_clear_chunk_size;   /* Memory clear chunk size.      */
    sal_mutex_t egressMeteringMutex;    /* Egress metering access mutex. */
    void        *tcam_info;             /* Ext memory control structure  */
    void        *ext_l2_ppa_info;
    void        *ext_l2_ppa_vlan;
    void        *ext_l2_ppa_info_2;
    void        *ext_l2_ppa_vlan_2;
    int         ext_l2_ppa_threshold;   /* Transition threshold */
    int         l2e_ppa;

    int16       dport_map[SOC_DPORT_MAX];/* User to physical port map    */
    int16       dport_rmap[SOC_PBMP_PORT_MAX];/* Reverse user port map   */
    int         dport_map_flags;        /* See dport.h                   */
    soc_event_cb_list_t *ev_cb_head;    /* critical events cb handler    */

    soc_module_t base_modid;            /* Device base module id.        */

#ifdef BCM_WARM_BOOT_SUPPORT
    uint32      autosync;               /* Autosync mode for warm boot   */
    uint32      scache_dirty;           /* Warm boot cache is dirty      */
#endif

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) /* || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) */
    iarb_tdm_table_entry_t iarb_tdm;    /* Cache of CPU slot (TR2/AP/EN) */
    int         iarb_tdm_idx;           /* Cache of CPU slot (TR2/AP/EN) */
#endif    
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_DFE_SUPPORT)|| defined(BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)|| defined(PORTMOD_SUPPORT)
    soc_interrupt_t         *interrupts_info;    /* interrupts array */
#endif
    /* WC firmware load configuration */
    void        *soc_wc_ucode_dma_buf;  /* DMA buffer for WC firmware load */
    int         soc_wc_ucode_alloc_size; /* DMA buffer for WC firmware size */

    /* MAC low power mode for Robo chips */
    uint32      mac_low_power_enabled; /* currently low power mode */
    uint32      auto_mac_low_power; /* Automatic enable low power mode */
    sal_mutex_t mac_low_power_mutex; 
    sal_time_t  all_link_down_detected;    /* Timestamp of event that all ports link down*/   
#ifdef BCM_POLAR_SUPPORT
    sal_time_t  ultra_low_power_detected;    /* Timestamp of event for ultra low power detected */   
#endif /* BCM_POLAR_SUPPORT */

    /* Mutex for internal CPU arbiter */
    sal_mutex_t arbiter_mutex;
    uint8    int_cpu_enabled;
    uint32  arbiter_lock_count;
    /* Feature cache */

    SHR_BITDCLNAME(features, soc_feature_count);

#ifdef BCM_TIMESYNC_SUPPORT
    /* Timesync clock period */
    uint32  timesync_pll_clock_ns;
#endif

#ifdef BCM_CB_ABORT_ON_ERR
    /* Callback return code handling */
    uint8 cb_abort_on_err;
#endif

#ifdef BCM_EA_SUPPORT
#ifdef BCM_TK371X_SUPPORT
    soc_ea_oam_ctrl_if_t *oam_ctrlops; /* pointer to control interface opsvec */
    int attached_unit;
    int attached_port;
#endif    
#endif
    sal_mutex_t ipArbiterMutex;
} soc_control_t;

/*
 * Typedef: sop_memstate_t
 * Purpose: Maintain per-memory persistent information
 */

#define SOP_MEM_STATE(unit, mem)        SOC_PERSIST(unit)->memState[mem]

typedef struct sop_memstate_s {
    uint32      count[SOC_MAX_NUM_BLKS];/* Number of valid table entries */
                                        /*   (sorted tables only) */
    int         index_max;              /* May be updated per unit */
} sop_memstate_t;

typedef struct sop_memcfg_er_s {
    uint32      ext_table_cfg;
    uint32      host_hash_table_cfg;
    uint32      l3v4_search_offset;
    uint32      l3v6_search_offset;
    uint32      mvl_hash_table_cfg;
    uint32      mystation_search_offset;
    int         tcam_select;
} sop_memcfg_er_t;

/*
 * Typedef: soc_persist_t
 * Purpose: SOC Persistent Structure.
 *      All info about a device instance that must be saved across a reload.
 * Notes:
 *      Modifications to this structure will prevent reload.  A system running
 *      a given version would not be able to be upgraded to a modified version.
 */

typedef struct soc_persist_s {
    /* Miscellaneous chip state */

    int         version;        /* Persistent state version */

    int         debugMode;      /* True if MMU is in debug mode */

    /* DCB static configuration */

    uint32      dcb_srcmod;             /* Source module and port for dcbs */
    uint32      dcb_srcport;
    uint32      dcb_pfm;                /* Port Forwarding Mode */

    /* IPG Configuration per port */

    soc_ipg_t   ipg[SOC_MAX_NUM_PORTS];

    /* On-chip memory management */

    sop_memstate_t
                memState[NUM_SOC_MEM];
#ifdef SOC_MEM_L3_DEFIP_WAR
    uint32      l3_defip_map;   /* Unused L3_DEFIP blocks */
#endif

    sop_memcfg_er_t
                er_memcfg;

    /* Link status */

    pbmp_t      link_fwd;       /* Forwarding ports current value */

    /* Linkscan status (private; may only accessed by link.c) */

    pbmp_t      lc_pbm_link;            /* Ports currently up */
    pbmp_t      lc_pbm_link_change;     /* Ports needed to recognize down */
    pbmp_t      lc_pbm_override_ports;  /* Force up/Down ports */
    pbmp_t      lc_pbm_override_link;   /* Force up/Down status */
    pbmp_t      lc_pbm_linkdown_tx;     /* Enable tx without link */
    pbmp_t      lc_pbm_remote_fault;    /* Ports receiving remote fault */
    pbmp_t      lc_pbm_failover;        /* Ports set for LAG failover */
    pbmp_t      lc_pbm_failed;          /* Failed LAG failover ports */
    pbmp_t      lc_pbm_failed_clear;    /* Failed ports ready to reset */
    pbmp_t      lc_pbm_fc;              /* FC ports */

    /*
     * Stacking related:
     *     stack_ports_current:   Ports currently engaged in stacking
     *     stack_ports_inactive:  Ports stacked, but explicitly blocked
     *     stack_ports_simplex:   Ports stacked using simplex mode
     *     stack_ports_previous:  Last stack port; to detect changes
     */
    soc_pbmp_t stack_ports_current;
    soc_pbmp_t stack_ports_inactive;
    soc_pbmp_t stack_ports_previous;
} soc_persist_t;

#define SOC_FLAGS_CLR(s, f) {       \
    int _s = sal_splhi();       \
    (s)->soc_flags &= ~(f);     \
    sal_spl(_s);            \
}

/*
 * soc_control: Per-device non-persistent global driver state
 * soc_persist: Per-device persistent global driver state
 * soc_ndev: Number of devices found during probe
 */

extern soc_control_t    *soc_control[SOC_MAX_NUM_DEVICES];
extern soc_persist_t    *soc_persist[SOC_MAX_NUM_DEVICES];
extern int              soc_ndev_attached;
extern int              soc_ndev;
extern int      soc_eth_ndev;
extern int      soc_all_ndev;
extern int      soc_eth_unit;
extern int      soc_mii_unit;
extern char     soc_reg_noinfo[];

extern soc_block_name_t soc_block_port_names[];
extern soc_block_name_t soc_block_names[];
extern soc_block_name_t soc_sbx_block_port_names[];
extern soc_block_name_t soc_sbx_block_names[];
#if defined(BCM_DPP_SUPPORT) || defined (BCM_DFE_SUPPORT)
extern soc_block_name_t soc_dpp_block_port_names[];
extern soc_block_name_t soc_dpp_block_names[];
#endif /* defined(BCM_DPP_SUPPORT) || defined (BCM_DFE_SUPPORT) */
extern sal_mutex_t spiMutex;

extern uint32    soc_state[SOC_MAX_NUM_DEVICES];

#if defined(BCM_WARM_BOOT_SUPPORT)
/*
 * BCM Warm Boot Support
 *
 * Purpose:  Indicates whether the device is currently in reload
 *           state (performing Warm boot) or not.
 *
 *           If finer granularity is needed in the future, additional
 *           flags can be defined to control specific hardware accesses.
 */

#define SOC_ST_F_RELOADING         0x1    /* Warm boot in progress, device is reloading */
#define SOC_WARM_BOOT(unit)       (soc_state[unit] == SOC_ST_F_RELOADING)
#define SOC_WARM_BOOT_START(unit) (soc_state[unit] = SOC_ST_F_RELOADING)
#define SOC_WARM_BOOT_DONE(unit)  (soc_state[unit] = 0x0)

extern int soc_wb_mim_state[SOC_MAX_NUM_DEVICES];
#define SOC_WARM_BOOT_MIM(unit) (soc_wb_mim_state[unit] = 1)
#define SOC_WARM_BOOT_IS_MIM(unit) (soc_wb_mim_state[unit] == 1)
extern int soc_shutdown(int unit);
extern int soc_system_scrub(int unit);
#else
#define SOC_WARM_BOOT(unit)       (0)
#define soc_shutdown(unit)        (SOC_E_UNAVAIL)
#define soc_system_scrub(unit)    (SOC_E_UNAVAIL)
#define SOC_WARM_BOOT_START(unit) \
             do { \
             } while(0)

#define SOC_WARM_BOOT_DONE(unit)  \
             do { \
             } while(0)
#define SOC_WARM_BOOT_MIM(unit)   (0)
#define SOC_WARM_BOOT_IS_MIM(unit)   (0)
#endif /* BCM_WARM_BOOT_SUPPORT */

#if defined(BCM_EASY_RELOAD_SUPPORT)
/*
 * BCM Easy Reload Support
 *
 * Purpose:  Indicates whether the device is currently in reload
 *           state (performing Easy Reload) or not.
 *
 *           If finer granularity is needed in the future, additional
 *           flags can be defined to control specific hardware accesses.
 */

#define SOC_ST_F_EASY_RELOAD       0x2    /* Easy Reload is in progress */
/* Reload mode set/get macros */
#define SOC_IS_RELOADING(unit)    (soc_state[unit] == SOC_ST_F_EASY_RELOAD)
#define SOC_RELOAD_MODE_SET(unit, reload_mode) \
    if (reload_mode) {              \
    soc_state[unit] = SOC_ST_F_EASY_RELOAD; \
    } else {                    \
        soc_state[unit] = 0;            \
    }
#elif defined(BCM_EASY_RELOAD_WB_COMPAT_SUPPORT)
/*Easy reload wb compat*/
#define SOC_ST_F_EASY_RELOAD       0x2    /* Easy Reload is in progress */
#define SOC_IS_RELOADING(unit)    (soc_feature(unit, soc_feature_easy_reload_wb_compat) &&  \
                                    soc_state[unit] == SOC_ST_F_EASY_RELOAD)
#define SOC_RELOAD_MODE_SET(unit, reload_mode)                                              \
    if (soc_feature(unit, soc_feature_easy_reload_wb_compat)) {                             \
        if (reload_mode) {                                                                  \
            soc_state[unit] = SOC_ST_F_EASY_RELOAD;                                         \
        } else {                                                                            \
            soc_state[unit] = 0;                                                            \
        }                                                                                   \
    }

#else
/*No Easy reload support*/
#define SOC_IS_RELOADING(unit) (0)
#define SOC_RELOAD_MODE_SET(unit, reload_mode)  \
             do { \
             } while(0)
#endif /* BCM_EASY_RELOAD_SUPPORT */
/*
 * BCM detach 
 *
 * Purpose:  Indicates whether the device is currently being detached from the
 *           system state (performing detach) or not.
 */

#define SOC_ST_F_DETACH       0x4    /* Detach is in progress */
/* Reload mode set/get macros */
#define SOC_IS_DETACHING(_unit_)    (soc_state[_unit_] == SOC_ST_F_DETACH)
#define SOC_DETACH(_unit_, _detach_) \
    if (_detach_) {              \
        soc_state[_unit_] = SOC_ST_F_DETACH; \
    } else {                    \
        soc_state[_unit_] = 0;            \
    }

#define SOC_HW_ACCESS_DISABLE(_unit_)                  \
    ((SOC_WARM_BOOT(_unit_) &&                         \
      (0 == (SOC_CONTROL(_unit_)->schan_override))) || \
     SOC_IS_RELOADING(_unit_) || SOC_IS_DETACHING(_unit_))

/*
 * Driver calls.
 */
extern int soc_attach(int unit);
extern int soc_attached(int unit);
extern int soc_reset(int unit);
extern int soc_init(int unit);
extern int soc_reset_init(int unit);
extern int soc_device_reset(int unit, int mdoe, int action);
extern int soc_detach(int unit);
extern int soc_deinit(int unit);
extern int soc_bist(int unit, soc_mem_t *mems, int num_mems, int timeout_msec);
extern int soc_bist_all(int unit);

extern int soc_bpdu_addr_set(int unit, int index, sal_mac_addr_t addr);
extern int soc_bpdu_addr_get(int unit, int index, sal_mac_addr_t *addr);

extern int soc_rcpu_schan_enable_set(int unit, int enable);

extern int soc_mmu_init(int unit);
extern int soc_misc_init(int unit);
extern const char *soc_dev_name(int unit);
extern int soc_info_config(int unit, soc_control_t *soc);
extern void soc_xgxs_lcpll_lock_check(int unit);
extern void soc_xport_type_update(int unit, soc_port_t port, int to_hg_port);
extern int  soc_port_type_verify(int unit);
extern int  soc_robo_attach(int unit);
extern int  soc_robo_reset(int unit);
extern int  soc_robo_init(int unit);
extern int  soc_robo_reset_init(int unit);
extern int  soc_robo_detach(int unit);
extern int  soc_robo_mgt_init(int unit);
extern int _soc_robo_device_created(int unit);

#ifdef BCM_LEDPROC_SUPPORT
extern int soc_ledproc_config(int unit, const uint8 *program, int bytes);
#endif

extern int soc_warpcore_firmware_set(int unit, int port, uint8 *array,
                                     int datalen, int wc_instance,
                                     soc_mem_t wc_ucmem_data,
                                     soc_reg_t wc_ucmem_ctrl);

extern int
soc_phy_firmware_load(int unit, int port, uint8 *fw_data, int fw_size);

extern int
soc_sbus_mdio_reg_read(int unit, int port, int blk, int wc_instance,
                       uint32 phy_addr, uint32 phy_reg, uint32 *phy_data,
                       soc_mem_t wc_ucmem_data, soc_reg_t wc_ucmem_ctrl);
extern int
soc_sbus_mdio_reg_write(int unit, int port, int blk, int wc_instance,
                        uint32 phy_addr, uint32 phy_reg, uint32 phy_data,
                        soc_mem_t wc_ucmem_data, soc_reg_t wc_ucmem_ctrl);

#define SOC_PROPERTY_NAME_MAX   64
#define SOC_PROPERTY_VALUE_MAX  64

extern char *soc_property_get_str(int unit, const char *name);
extern uint32 soc_property_get(int unit, const char *name, uint32 defl);
extern uint32 soc_property_obj_attr_get(int unit, const char *prefix,
                                        const char *obj, int index,
                                        const char *attr, int scale,
                                        char *suffix, uint32 defl);
extern pbmp_t soc_property_get_pbmp(int unit, const char *name,
                                    int defneg);
extern pbmp_t soc_property_get_pbmp_default(int unit, const char *name,
                                            pbmp_t def_pbmp);

extern pbmp_t soc_property_suffix_num_pbmp_get(int unit, int num, const char *name,
                            const char *suffix, soc_pbmp_t pbmp_def);

extern void soc_property_get_bitmap_default(int unit, const char *name,
                              uint32 *bitmap, int max_words, uint32 *def_bitmap);

extern int soc_property_get_csv(int unit, const char *name,
                                     int val_max, int *val_array);
extern char *soc_property_port_get_str(int unit, soc_port_t port,
                                       const char *name);
extern uint32 soc_property_port_get(int unit, soc_port_t port,
                                    const char *name, uint32 defl);
extern uint32 soc_property_port_obj_attr_get(int unit, soc_port_t port,
                                             const char *prefix,
                                             const char *obj, int index,
                                             const char *attr, int scale,
                                             char *suffix, uint32 defl);
extern int soc_property_port_get_csv(int unit, soc_port_t port,
                                     const char *name, int val_max,
                                     int *val_array);
extern uint32 soc_property_suffix_num_get(int unit, int num,
                                          const char *name,
                                          const char *suffix, uint32 defl);

extern uint32 soc_property_suffix_num_get_only_suffix(int unit, int num, const char *name,
                            const char *suffix, uint32 defl);

extern char* soc_property_suffix_num_str_get(int unit, int num, const char *name,
                            const char *suffix);
extern uint32 soc_property_port_suffix_num_get(int unit, soc_port_t port,
                                               int num, const char *name,
                                          const char *suffix, uint32 defl);
extern char *soc_property_port_suffix_num_get_str(int unit, soc_port_t port,
                                               int num, const char *name,
                                          const char *suffix);
extern uint32 soc_property_cos_get(int unit, soc_cos_t cos,
                                    const char *name, uint32 defl);
extern uint32 soc_property_uc_get(int unit, int uc,
                                   const char *name, uint32 defl);
extern uint32 soc_property_ci_get(int unit, int ci,
                                   const char *name, uint32 defl);
extern int soc_property_ci_get_csv(int unit, int ci,
                                     const char *name, int val_max,
                                     int *val_array);

extern char *soc_block_port_name_lookup_ext(soc_block_t, int);
extern char *soc_block_name_lookup_ext(soc_block_t, int);
extern soc_block_t soc_block_port_name_match(char *);
extern soc_block_t soc_block_name_match(char *);


#define SOC_PCI_DEV_TYPE      SAL_PCI_DEV_TYPE    /* PCI device */
#define SOC_SPI_DEV_TYPE      SAL_SPI_DEV_TYPE    /* SPI device */
#define SOC_EB_DEV_TYPE       SAL_EB_DEV_TYPE     /* EB device */
#define SOC_ICS_DEV_TYPE      SAL_ICS_DEV_TYPE    /* ICS device */
#define SOC_MII_DEV_TYPE      SAL_MII_DEV_TYPE    /* MII device */
#define SOC_RCPU_DEV_TYPE     SAL_RCPU_DEV_TYPE   /* RCPU device */
#define SOC_I2C_DEV_TYPE      SAL_I2C_DEV_TYPE    /* I2C device */
#define SOC_AXI_DEV_TYPE      SAL_AXI_DEV_TYPE    /* AXI device */
#define SOC_EMMI_DEV_TYPE     SAL_EMMI_DEV_TYPE   /* EMMI device*/   
#define SOC_DEV_BUS_ALT       SAL_DEV_BUS_ALT     /* Alternate Access */

#define SOC_SWITCH_DEV_TYPE   SAL_SWITCH_DEV_TYPE /* Switch device */
#define SOC_ETHER_DEV_TYPE    SAL_ETHER_DEV_TYPE  /* Ethernet device */
#define SOC_CPU_DEV_TYPE      SAL_CPU_DEV_TYPE    /* CPU device */

/* Backward compatibility */
#define SOC_ET_DEV_TYPE       SOC_MII_DEV_TYPE

#define SOC_TIMEOUT_VAL 100 /* Times for retrying */
/* soc/intr.c exported routines */

/* Interrupt function type */
typedef void (*soc_interrupt_fn_t)(void *_unit);


extern void soc_intr(void *unit);
extern uint32  soc_intr_enable(int unit, uint32 mask);
extern uint32  soc_intr_disable(int unit, uint32 mask);
extern uint32  soc_intr_block_lo_enable(int unit, uint32 mask);
extern uint32  soc_intr_block_lo_disable(int unit, uint32 mask);
extern uint32  soc_intr_block_hi_enable(int unit, uint32 mask);
extern uint32  soc_intr_block_hi_disable(int unit, uint32 mask);
#ifdef BCM_CMICM_SUPPORT
extern void soc_cmicm_intr(void *unit);
extern uint32  soc_cmicm_intr0_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr0_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr1_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr1_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr2_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr2_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr3_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr3_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr4_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr4_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr0_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr0_disable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr1_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr1_disable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr2_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr2_disable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr3_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr3_disable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr4_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr4_disable(int unit, int cmc, uint32 mask);
#ifdef BCM_RCPU_SUPPORT
extern void soc_cmicm_rcpu_intr(int unit, soc_rcpu_intr_packet_t *intr_pkt);
extern uint32  soc_cmicm_rcpu_intr0_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr0_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr1_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr1_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr2_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr2_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr3_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr3_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr4_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr4_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc0_intr0_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc0_intr0_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc1_intr0_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc1_intr0_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc2_intr0_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc2_intr0_disable(int unit, uint32 mask);
#endif /* BCM_RCPU_SUPPORT */
extern void soc_cmicm_ihost_irq_offset_set(int unit);
extern void soc_cmicm_ihost_irq_offset_reset(int unit);
#endif /* CMICM Support */

extern void soc_endian_config(int unit);
extern void soc_pci_ep_config(int unit, int pcie);
extern void soc_pci_burst_enable(int unit);

extern int soc_max_vrf_set(int unit, int value);

#if defined(BROADCOM_DEBUG)                      /* Debug Support */
extern int      soc_dump(int unit, const char *pfx);
extern int  soc_robo_dump(int unit, const char *pfx);
#endif /* defined(BROADCOM_DEBUG) */

/* MAC Core initialization */
extern int soc_xgxs_reset(int unit, soc_port_t port);
extern int soc_wc_xgxs_reset(int unit, soc_port_t port, int reg_idx);
extern int soc_wc_xgxs_pll_check(int unit, soc_port_t port, int reg_idx);
extern int soc_wc_xgxs_power_down(int unit, soc_port_t port, int reg_idx);
extern int soc_tsc_xgxs_reset(int unit, soc_port_t port, int reg_idx);
extern int soc_tsc_xgxs_pll_check(int unit, soc_port_t port, int reg_idx);
extern int soc_fusioncore_reset(int unit, soc_port_t port);
#if defined(BCM_GXPORT_SUPPORT)
extern int soc_unicore_reset(int unit, soc_port_t port);
#endif /* BCM_GXPORT_SUPPORT */

/* Cosq init */
#ifdef BCM_COSQ_HIGIG_MAP_DISABLE
int soc_cosq_stack_port_map_disable(int unit);
#endif

#ifdef BCM_XGS3_SWITCH_SUPPORT
int soc_cpu_priority_mapping_init(int unit);
#endif

#if defined(BCM_FIREBOLT_SUPPORT)
#if defined(INCLUDE_L3)
extern int soc_tunnel_term_block_size_set (int unit, int size);
extern int soc_tunnel_term_block_size_get (int unit, int *size);
#endif /* BCM_FIREBOLT_SUPPORT */
#endif /* INCLUDE_L3 */

/* SER HW engine configuration and control structures */

/* SER per-table control flags */
#define _SOC_SER_FLAG_ACC_TYPE_MASK  0x001f  /* Access type for multi-pipe */
#define _SOC_SER_FLAG_SW_COMPARE     0x0100  /* SW implemented SER check */
#define _SOC_SER_FLAG_DISCARD_READ   0x0200  /* Read DMA with HW discard */
#define _SOC_SER_FLAG_MULTI_PIPE     0x0400  /* HW multi-pipe table */
#define _SOC_SER_FLAG_XY_READ        0x0800  /* Read SER optimization */
#define _SOC_SER_FLAG_NO_DMA         0x1000  /* Use PIO iterations */
#define _SOC_SER_FLAG_REMAP_READ     0x2000  /* Read remapped indexes */
#define _SOC_SER_FLAG_OVERLAY        0x4000  /* Memories shared in HW */
#define _SOC_SER_FLAG_OVERLAY_CASE   0x8000  /* Polarity of overlay */
#define _SOC_SER_FLAG_SIZE_VERIFY    0x10000  /* Check table size */

/*
 * Multipipe entries for tables that should have the SER HW engine
 * configured to protect more than one pipeline must be
 * consecutive in the (generic) SER info structure for the same table.
 *
 * Overlay tables must also be consecutive by overlay type.
 * See trident[2].c for examples of the tables.
 */

typedef enum {
    _SOC_SER_PARITY_MODE_1BIT,
    _SOC_SER_PARITY_MODE_2BITS,
    _SOC_SER_PARITY_MODE_4BITS,
    _SOC_SER_PARITY_MODE_8BITS,
    _SOC_SER_PARITY_MODE_NUM
} _soc_ser_parity_mode_t;

typedef struct _soc_ser_parity_info_s {
    soc_mem_t               mem;
    _soc_ser_parity_mode_t  parity_mode; /* 0/1/2/3 => 1/2/4/8 bit parity */
    soc_reg_t               start_addr_reg;
    soc_reg_t               end_addr_reg;
    soc_reg_t               cmic_mem_addr_reg;
    soc_reg_t               parity_mode_reg;
    soc_field_t             parity_mode_field;
    int                     bit_offset; /* Ignored if -1 */
    soc_reg_t               entry_len_reg; /* Ignored if bit_offset is ignored */
    uint8                   cmic_ser_id; 
    int                     ser_section_start;
    int                     ser_section_end;
    uint32                  ser_flags;
} _soc_ser_parity_info_t;

#define SOC_NUM_GENERIC_PROT_SECTIONS  4
typedef enum {
    _SOC_SER_PARITY_1BIT,
    _SOC_SER_ECC_1FLD = _SOC_SER_PARITY_1BIT,
    _SOC_SER_PARITY_2BITS,
    _SOC_SER_ECC_2FLD = _SOC_SER_PARITY_2BITS,
    _SOC_SER_PARITY_4BITS,
    _SOC_SER_ECC_4FLD = _SOC_SER_PARITY_4BITS,
    _SOC_SER_PARITY_8BITS
} _soc_ser_protection_mode_t;

typedef enum {
    _SOC_SER_TYPE_PARITY,
    _SOC_SER_TYPE_ECC
} _soc_ser_protection_type_t;

typedef enum {
    _SOC_SER_INTERLEAVE_NONE,
    _SOC_SER_INTERLEAVE_MOD2,
    _SOC_SER_INTERLEAVE_MOD4
} _soc_ser_interleave_type_t;

typedef struct __soc_ser_start_end_bits_s {
    int start_bit;
    int end_bit;
} __soc_ser_start_end_bits_t;

typedef struct _soc_generic_ser_info_s {
    soc_mem_t                  mem; /* last if INVALIDm */
    soc_mem_t                  alias_mem; /* share parity data with alias mem */
    _soc_ser_protection_type_t prot_type;
    _soc_ser_protection_mode_t prot_mode;
    _soc_ser_interleave_type_t intrlv_mode;
    __soc_ser_start_end_bits_t start_end_bits[SOC_NUM_GENERIC_PROT_SECTIONS];
    uint32                     addr_mask;
    int                        ser_section_start;
    int                        ser_section_end;
    int                        ser_hw_index;
    uint32                     ser_flags;
    uint32                     addr_start_bit;
    uint32                     disable_mask;
} _soc_generic_ser_info_t;

extern int
soc_ser_init(int unit, _soc_ser_parity_info_t *_soc_ser_parity_info,
              int max_mem);
extern int
soc_process_ser_parity_error(int unit, 
                             _soc_ser_parity_info_t *_soc_ser_parity_info,
                             int parity_err_type);
extern int
soc_process_cmicm_ser_parity_error(int unit, 
                             _soc_ser_parity_info_t *_soc_ser_parity_info,
                                   int parity_err_type);

extern int
soc_ser_mem_clear(int unit,
                  _soc_ser_parity_info_t *_soc_ser_parity_info,
                  soc_mem_t mem);
extern int
soc_cmicm_ser_mem_clear(int unit,
                        _soc_ser_parity_info_t *_soc_ser_parity_info,
                        soc_mem_t mem);

extern int
soc_generic_ser_init(int unit, _soc_generic_ser_info_t *_ser_info);
extern int
soc_generic_ser_process_error(int unit, _soc_generic_ser_info_t *_ser_info,
                              int err_type);

extern int
soc_ser_mem_nack(void *unit_vp, void *addr_vp, void *d2,
             void *d3, void *d4);

extern void
soc_ser_fail(void *unit_vp, void *addr_vp, void *d2,
             void *d3, void *d4);
extern int
soc_ser_parity_error_intr(int unit);

extern int
soc_ser_parity_error_cmicm_intr(void *unit_vp, void *d1, void *d2,
             void *d3, void *d4);

typedef struct soc_ser_functions_s {
    void (*_soc_ser_stat_nack_f)(int, int*);
    void (*_soc_ser_fail_f)(int);
    void (*_soc_ser_mem_nack_f)(void*, void*, void*, void*, void*);
    void (*_soc_ser_parity_error_intr_f)(void*, void*, void*, void*, void*);
    void (*_soc_ser_parity_error_cmicm_intr_f)(void*, void*, void*, void*, void*);
    int  (*_soc_ser_populate_tcam_log_f)(int, soc_mem_t, soc_acc_type_t, int);
} soc_ser_functions_t;

extern void
soc_ser_function_register(int unit, soc_ser_functions_t *functions);
extern int
soc_ser_populate_tcam_log(int unit, soc_mem_t mem, int target_pipe, int index);
extern int
soc_ser_stat_error(int unit, int port);
extern int
soc_ser_reg_cache_init(int unit);
extern int
soc_ser_reg_cache_clear(int unit, soc_reg_t reg, int port);
extern int
soc_ser_reg_cache_set(int unit, soc_reg_t reg, int port, int index, uint64 data);
extern int
soc_ser_reg32_cache_set(int unit, soc_reg_t reg, int port, int index, uint32 data);
extern int 
soc_ser_reg_cache_get(int unit, soc_reg_t reg, int port, int index, uint64 *data);
extern int 
soc_ser_reg32_cache_get(int unit, soc_reg_t reg, int port, int index, uint32 *data);
extern void
soc_ser_reg_cache_info(int unit, int *count, int *size);
extern int
soc_ser_reg_load_scrub(int unit, int scrub_load);

typedef struct _soc_ser_correct_info_s {
    uint32 flags;
#define SOC_SER_SRC             0x1 /* Reg: 0, Mem: 1 */
#define SOC_SER_SRC_REG         0x0
#define SOC_SER_SRC_MEM         0x1
#define SOC_SER_REG_MEM_KNOWN   0x2 /* No decoding required */
#define SOC_SER_REG_MEM_UNKNOWN 0x0 /* Decoding required */
#define SOC_SER_ERR_HW          0x0 /* Error reported for hw lookup */
#define SOC_SER_ERR_CPU         0x4 /* Error reported for cpu access */
#define SOC_SER_ERR_MULTI       0x8 
    soc_reg_t reg;
    soc_mem_t mem;
    soc_block_t blk_type; /* s/w block type */
    uint32 sblk; /* schan blk */
    int pipe_num; /* for multi pipe devices */
    int acc_type;
    uint32 stage;
    uint32 addr;
    int port;
    int index;
    uint8 double_bit;
    uint32 log_id;
    sal_usecs_t detect_time;
    int parity_type;
} _soc_ser_correct_info_t;

typedef struct _soc_ser_sram_info_s {
#define _SOC_SER_MAX_SRAMS              8
#define _SOC_SER_MAX_ENTRIES_PER_BKT    6
#define _SOC_SER_SRAM_CORRECTION_MODE_0 0 /* So called "original" XOR scheme */
#define _SOC_SER_SRAM_CORRECTION_MODE_1 1 /* So called "modified" XOR scheme */
    int ram_count;
    soc_mem_t view[_SOC_SER_MAX_SRAMS];
    int index_count[_SOC_SER_MAX_SRAMS];
    int mem_indexes[_SOC_SER_MAX_SRAMS][_SOC_SER_MAX_ENTRIES_PER_BKT];
    soc_reg_t force_reg;
    soc_field_t force_field;   /* Force XOR generation */
    soc_reg_t disable_reg;
    soc_field_t disable_field; /* Disable error generation on write */
} _soc_ser_sram_info_t;

extern int
soc_ser_correction(int unit, _soc_ser_correct_info_t *si);
extern int
soc_ser_sram_correction(int unit, int pipe, int sblk, int addr, soc_mem_t mem, 
                        int copyno, int index, int log_id);
extern int
soc_generic_ser_mem_scan_start(int unit);
extern int
soc_generic_ser_mem_scan_stop(int unit);

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/* FCOE Header Types */
#define SOC_FCOE_HDR_TYPE_STD     0x1 /* FC standard header     */
#define SOC_FCOE_HDR_TYPE_VFT     0x2 /* Virtual Fabric Tag     */
#define SOC_FCOE_HDR_TYPE_ENCAP   0x3 /* FC encapsulation hdr   */
#define SOC_FCOE_HDR_TYPE_IFR     0x4 /* Inter fabric routing   */

/* FCOE header type configuration */
typedef struct _soc_fcoe_header_info_s {
    int fc_hdr_type;
    int r_ctl_min;
    int r_ctl_max;
} _soc_fcoe_header_info_t;
#endif /* (BCM_TRIDENT_SUPPORT) || (BCM_TRIUMPH3_SUPPORT) */

typedef _shr_switch_temperature_monitor_t soc_switch_temperature_monitor_t;

extern int soc_do_init(int unit, int reset);
#ifdef BCM_ROBO_SUPPORT
extern int soc_robo_chip_reset(int unit);
#endif /* BCM_ROBO_SUPPORT */
extern int soc_robo_do_init(int unit, int reset);

extern int soc_esw_div64(uint64 x, uint32 y, uint32 *result);
extern int soc_esw_hw_qnum_get(int unit, int port, int cos, int *qnum);

extern int soc_ea_reset_init(int unit);
extern int soc_ea_pre_attach(int unit);
extern int soc_ea_detach(int unit);
extern int soc_ea_do_init(int unit_count);
extern int soc_is_valid_block_instance(int unit, soc_block_types_t block_types, int block_instance, int *is_valid);

#ifdef BCM_NORTHSTARPLUS_SUPPORT 
/* report the attached MACSEC devic ID (-1 means invalid) */ 
#define SOC_ROBO_NSP_MACSEC_ID_GET(unit, port)  \
        ((SOC_IS_NORTHSTARPLUS(unit) == 0) ? -1 : (((port) == 4) ? 1 : \
        (((port) == 5) ? 0 : -1)))

#define NSP_CORE_APB_ADDR               (0x3F000) /* offset from 0x18000000 */
#define NSP_MACSEC_CONFIG               (0x40C)
#define NSP_MACSEC0_BYPASS_EN_OFFSET    (0) /* bit 0 */
#define NSP_MACSEC1_BYPASS_EN_OFFSET    (1) /* bit 1 */
#define NSP_MACSEC_BYPASS_EN_LEN        (1) /* 1 bit */
#define NSP_MACSEC_BYPASS_EN_MASK       (0x1)
#define NSP_MACSEC1_BYPASS_EN_MASK      NSP_MACSEC0_BYPASS_EN_MASK

#define NSP_MACSEC_BYPASS_EN_SET(val, id, en)   \
        if ((en))   (val) |= (NSP_MACSEC_BYPASS_EN_MASK <<  \
                    (NSP_MACSEC0_BYPASS_EN_OFFSET + (id * NSP_MACSEC_BYPASS_EN_LEN)));\
        else    (val) &= ~(NSP_MACSEC_BYPASS_EN_MASK <<     \
                    (NSP_MACSEC0_BYPASS_EN_OFFSET + (id * NSP_MACSEC_BYPASS_EN_LEN)))
#define NSP_MACSEC_BYPASS_EN_GET(val, id)   \
        (((val) & (NSP_MACSEC_BYPASS_EN_MASK << \
        (NSP_MACSEC0_BYPASS_EN_OFFSET + (id * NSP_MACSEC_BYPASS_EN_LEN)))) ? \
        TRUE : FALSE)
#endif     /* BCM_NORTHSTARPLUS_SUPPORT */

#ifdef INCLUDE_MACSEC 
#ifdef BCM_ROBO_SUPPORT
/* macsec bypass set/get are supported for NOPHY_MACSEC only. */
extern int soc_robo_macsec_bypass_set(int unit, soc_port_t port, uint32 value);
extern int soc_robo_macsec_bypass_get(int unit, soc_port_t port, uint32 *value);
#endif  /* BCM_ROBO_SUPPORT */
#endif     /* INCLUDE_MACSEC */

/* Generic parameters to configure a PLL */
typedef struct soc_pll_param_s {
    unsigned ref_freq;
    unsigned ndiv_int;
    unsigned ndiv_frac;
    unsigned pdiv;
    unsigned mdiv;
    unsigned ka;
    unsigned ki;
    unsigned kp;
    unsigned vco_div2;
} soc_pll_param_t;

#endif  /* !_SOC_DRV_H */
