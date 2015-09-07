/*
 * $Id: intr_cmicm.c,v 1.107 Broadcom SDK $
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
 * SOC CMICm Interrupt Handlers
 *
 * NOTE: These handlers are called from an interrupt context, so their
 *       actions are restricted accordingly.
 */

#include <shared/bsl.h>

#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>
#include <sal/core/sync.h>
#include <sal/core/dpc.h>

#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/dma.h>
#include <soc/i2c.h>
#include <soc/cmicm.h>
#include <soc/feature.h>
#include <soc/intr.h>
#include <soc/mem.h>
#ifdef BCM_DPP_SUPPORT
#include <soc/dpp/ARAD/ARAD_PP/arad_pp_api_oam.h>
#endif 
#ifdef BCM_KATANA_SUPPORT
#include <soc/katana.h>
#endif /* BCM_KATANA_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
#include <soc/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
#include <soc/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_HURRICANE2_SUPPORT
#include <soc/hurricane2.h>
#endif /* BCM_HURRICANE2_SUPPORT */
#ifdef BCM_GREYHOUND_SUPPORT
#include <soc/greyhound.h>
#endif /* BCM_GREYHOUND_SUPPORT */
#ifdef BCM_CALADAN3_SUPPORT
#include <soc/sbx/caladan3.h>
#endif /* BCM_CALADAN3_SUPPORT */

#ifdef BCM_CMICM_SUPPORT

#define HOST_IRQ_MASK_OFFSET_DIFF (CMIC_CMC0_UC0_IRQ_MASK0_OFFSET              \
                - CMIC_CMC0_PCIE_IRQ_MASK0_OFFSET)

static uint32 soc_cmicm_host_irq_offset[SOC_MAX_NUM_DEVICES] = {0};

#define INTR_MASK_OFFSET(_u,_a) (_a + soc_cmicm_host_irq_offset[_u])

#ifdef INCLUDE_KNET
#include <soc/knet.h>
#define IRQ_MASK0_SET_FUNC soc_knet_irq_mask_set
#else
#define IRQ_MASK0_SET_FUNC soc_pci_write
#endif

#define IRQ_MASK0_SET(_u,_a,_m) IRQ_MASK0_SET_FUNC(_u,INTR_MASK_OFFSET(_u,_a),_m)
#define IRQ_MASKx_SET(_u,_a,_m) soc_pci_write(_u,INTR_MASK_OFFSET(_u,_a),_m)


#include <soc/shared/mos_intr_common.h>
#include <soc/uc_msg.h>

/* Declare static functions for interrupt handler array */
STATIC void soc_cmicm_intr_schan_done(int unit, uint32 ignored);
STATIC void soc_cmicm_intr_miim_op(int unit, uint32 ignored);
STATIC void soc_cmicm_intr_tdma_done(int unit, uint32 ignored);
STATIC void soc_cmicm_intr_tslam_done(int unit, uint32 ignored);
STATIC void soc_cmicm_intr_stat_dma(int unit, uint32 ignored);
STATIC void soc_cmicm_intr_ccmdma_done(int unit, uint32 ignored);
STATIC void soc_cmicm_fifo_dma_done(int unit, uint32 ch);
STATIC void soc_cmicm_intr_sbusdma_done(int unit, uint32 ch);
#if defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DPP_SUPPORT)
STATIC void soc_cmicm_chip_func_intr(int unit, uint32 val);
STATIC void soc_cmicm_intr_link_stat(int unit, uint32 ignored);
STATIC void soc_cmicm_parity_intr(int unit, uint32 val);
STATIC void soc_cmicm_block_lo_intr(int unit, uint32 val);
STATIC void soc_cmicm_link_stat(int unit, uint32 ignored);
STATIC void soc_ser_engine_intr(int unit, uint32 val);
STATIC void soc_cmicdv2_parity_intr(int unit, uint32 val);
#endif
#ifdef BCM_CALADAN3_SUPPORT
STATIC void soc_cmicm_block_hi_intr(int unit, uint32 val);
#endif
#ifdef INCLUDE_RCPU
STATIC void soc_cmicm_rcpu_intr_miim_op(int unit, uint32 ignored);
#endif


/*
 * SOC Interrupt Table
 *
 * The table is stored in priority order:  Interrupts that are listed
 * first have their handlers called first.
 *
 * A handler can clear more than one interrupt bit to prevent a
 * subsequent handler from being called.  E.g., if the DMA CHAIN_DONE
 * handler clears both CHAIN_DONE and DESC_DONE, the DESC_DONE handler
 * will not be called.
 */

typedef void (*ifn_t)(int unit, uint32 data);

typedef struct {
    uint32    mask;
    ifn_t    intr_fn;
    uint32    intr_data;
    char    *intr_name;
} intr_handler_t;

static intr_handler_t soc_cmicm_intr_handlers[] = {
    { IRQ_CMCx_SCH_OP_DONE,    soc_cmicm_intr_schan_done,    0, "SCH_OP_DONE"   },
    { IRQ_CMCx_MIIM_OP_DONE,   soc_cmicm_intr_miim_op,       0, "MIIM_OP_DONE"   },
    { IRQ_CMCx_TDMA_DONE,      soc_cmicm_intr_tdma_done,     0, "TDMA_DONE"   },
    { IRQ_CMCx_TSLAM_DONE,     soc_cmicm_intr_tslam_done,    0, "TSLAM_DONE"   },
    { IRQ_CMCx_CCMDMA_DONE,    soc_cmicm_intr_ccmdma_done,   0, "CCNDMA_DONE"   },
    
    { IRQ_CMCx_CHAIN_DONE(0),  soc_dma_done_chain, 0, "CH0_CHAIN_DONE" },
    { IRQ_CMCx_CHAIN_DONE(1),  soc_dma_done_chain, 1, "CH1_CHAIN_DONE" },
    { IRQ_CMCx_CHAIN_DONE(2),  soc_dma_done_chain, 2, "CH2_CHAIN_DONE" },
    { IRQ_CMCx_CHAIN_DONE(3),  soc_dma_done_chain, 3, "CH3_CHAIN_DONE" },
    
    { IRQ_CMCx_DESC_DONE(0),   soc_dma_done_desc,  0, "CH0_DESC_DONE"  },
    { IRQ_CMCx_DESC_DONE(1),   soc_dma_done_desc,  1, "CH1_DESC_DONE"  },
    { IRQ_CMCx_DESC_DONE(2),   soc_dma_done_desc,  2, "CH2_DESC_DONE"  },
    { IRQ_CMCx_DESC_DONE(3),   soc_dma_done_desc,  3, "CH3_DESC_DONE"  },
    
    { IRQ_CMCx_STAT_ITER_DONE, soc_cmicm_intr_stat_dma,  0, "STAT_ITER_DONE" },
    
    { IRQ_CMCx_SW_INTR(CMICM_SW_INTR_UC0), soc_cmic_sw_intr, CMICM_SW_INTR_UC0, "UC0_SW_INTR" },
    { IRQ_CMCx_SW_INTR(CMICM_SW_INTR_UC1), soc_cmic_sw_intr, CMICM_SW_INTR_UC1, "UC1_SW_INTR" },

    { 0, NULL, 0, "" } /* Termination */
};

static intr_handler_t soc_cmicm_intr_handlers0_fifo_dma[] = {
    { IRQ_CMCx_SCH_OP_DONE,    soc_cmicm_intr_schan_done,    0, "SCH_OP_DONE"   },
    { IRQ_CMCx_MIIM_OP_DONE,   soc_cmicm_intr_miim_op,       0, "MIIM_OP_DONE"   },
    { IRQ_CMCx_TDMA_DONE,      soc_cmicm_intr_tdma_done,     0, "TDMA_DONE"   },
    { IRQ_CMCx_TSLAM_DONE,     soc_cmicm_intr_tslam_done,    0, "TSLAM_DONE"   },
    { IRQ_CMCx_CCMDMA_DONE,    soc_cmicm_intr_ccmdma_done,   0, "CCNDMA_DONE"   },
    
    { IRQ_CMCx_CHAIN_DONE(0),  soc_dma_done_chain, 0, "CH0_CHAIN_DONE" },
    { IRQ_CMCx_CHAIN_DONE(1),  soc_dma_done_chain, 1, "CH1_CHAIN_DONE" },
    { IRQ_CMCx_CHAIN_DONE(2),  soc_dma_done_chain, 2, "CH2_CHAIN_DONE" },
    { IRQ_CMCx_CHAIN_DONE(3),  soc_dma_done_chain, 3, "CH3_CHAIN_DONE" },
    
    { IRQ_CMCx_DESC_DONE(0),   soc_dma_done_desc,  0, "CH0_DESC_DONE"  },
    { IRQ_CMCx_DESC_DONE(1),   soc_dma_done_desc,  1, "CH1_DESC_DONE"  },
    { IRQ_CMCx_DESC_DONE(2),   soc_dma_done_desc,  2, "CH2_DESC_DONE"  },
    { IRQ_CMCx_DESC_DONE(3),   soc_dma_done_desc,  3, "CH3_DESC_DONE"  },
    
    { IRQ_CMCx_STAT_ITER_DONE, soc_cmicm_intr_stat_dma,  0, "STAT_ITER_DONE" },
    
    { IRQ_CMCx_SW_INTR(CMICM_SW_INTR_UC0), soc_cmic_sw_intr, CMICM_SW_INTR_UC0, "UC0_SW_INTR" },
    { IRQ_CMCx_SW_INTR(CMICM_SW_INTR_UC1), soc_cmic_sw_intr, CMICM_SW_INTR_UC1, "UC1_SW_INTR" },
    { IRQ_CMCx_FIFO_CH_DMA(0), soc_cmicm_fifo_dma_done, 0, "CH0_FIFO_DMA_DONE" },
    { IRQ_CMCx_FIFO_CH_DMA(1), soc_cmicm_fifo_dma_done, 1, "CH1_FIFO_DMA_DONE" },
    { IRQ_CMCx_FIFO_CH_DMA(2), soc_cmicm_fifo_dma_done, 2, "CH2_FIFO_DMA_DONE" },
    { IRQ_CMCx_FIFO_CH_DMA(3), soc_cmicm_fifo_dma_done, 3, "CH3_FIFO_DMA_DONE" },

    { 0, NULL, 0, "" } /* Termination */
};


STATIC intr_handler_t soc_cmicm_intr_handlers0[] = {
    { IRQ_CMCx_SCH_OP_DONE,    soc_cmicm_intr_schan_done,    0, "SCH_OP_DONE"   },
    { IRQ_CMCx_MIIM_OP_DONE,   soc_cmicm_intr_miim_op,       0, "MIIM_OP_DONE"   },

    { IRQ_SBUSDMA_CH0_DONE,    soc_cmicm_intr_sbusdma_done,  0, "SBUS_DMA0_DONE"   },
    { IRQ_SBUSDMA_CH1_DONE,    soc_cmicm_intr_sbusdma_done,  1, "SBUS_DMA1_DONE"   },
    { IRQ_SBUSDMA_CH2_DONE,    soc_cmicm_intr_sbusdma_done,  2, "SBUS_DMA2_DONE"   },

    { IRQ_CMCx_CCMDMA_DONE,    soc_cmicm_intr_ccmdma_done,   0, "CCNDMA_DONE"   },
    
    { IRQ_CMCx_CHAIN_DONE(0),  soc_dma_done_chain, 0, "CH0_CHAIN_DONE" },
    { IRQ_CMCx_CHAIN_DONE(1),  soc_dma_done_chain, 1, "CH1_CHAIN_DONE" },
    { IRQ_CMCx_CHAIN_DONE(2),  soc_dma_done_chain, 2, "CH2_CHAIN_DONE" },
    { IRQ_CMCx_CHAIN_DONE(3),  soc_dma_done_chain, 3, "CH3_CHAIN_DONE" },
    
    { IRQ_CMCx_DESC_DONE(0),   soc_dma_done_desc,  0, "CH0_DESC_DONE"  },
    { IRQ_CMCx_DESC_DONE(1),   soc_dma_done_desc,  1, "CH1_DESC_DONE"  },
    { IRQ_CMCx_DESC_DONE(2),   soc_dma_done_desc,  2, "CH2_DESC_DONE"  },
    { IRQ_CMCx_DESC_DONE(3),   soc_dma_done_desc,  3, "CH3_DESC_DONE"  },
    
    { IRQ_CMCx_STAT_ITER_DONE, soc_cmicm_intr_stat_dma,  0, "STAT_ITER_DONE" },
    
    { IRQ_CMCx_SW_INTR(CMICM_SW_INTR_UC0), soc_cmic_sw_intr, CMICM_SW_INTR_UC0, "UC0_SW_INTR" },
    { IRQ_CMCx_SW_INTR(CMICM_SW_INTR_UC1), soc_cmic_sw_intr, CMICM_SW_INTR_UC1, "UC1_SW_INTR" },

    { IRQ_CMCx_FIFO_CH_DMA(0), soc_cmicm_fifo_dma_done, 0, "CH0_FIFO_DMA_DONE" },
    { IRQ_CMCx_FIFO_CH_DMA(1), soc_cmicm_fifo_dma_done, 1, "CH1_FIFO_DMA_DONE" },
    { IRQ_CMCx_FIFO_CH_DMA(2), soc_cmicm_fifo_dma_done, 2, "CH2_FIFO_DMA_DONE" },
    { IRQ_CMCx_FIFO_CH_DMA(3), soc_cmicm_fifo_dma_done, 3, "CH3_FIFO_DMA_DONE" },
    
    { 0, NULL, 0, "" } /* Termination */
};

#if defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DPP_SUPPORT)
STATIC intr_handler_t soc_cmicm_intr_handlers1[] = {
    { IRQ_CMCx_CHIP_FUNC_INTR, soc_cmicm_chip_func_intr, 0, "CHIP FUNC INTR" },
    { IRQ_CMCx_LINK_STAT_MOD, soc_cmicm_link_stat, 0, "PHY_LINKSCAN_LINKSTATUS_CHD"},
    { IRQ_CMCx_LINK_STAT_MOD, soc_cmicm_intr_link_stat, 0, "LINK_STAT_MOD" },
    { IRQ_CMCx_SER_INTR, soc_ser_engine_intr, 0, "SER ENGINE INTR" },
    { 0, NULL, 0, "" } /* Termination */
};

STATIC intr_handler_t soc_cmicm_intr_handlers2[] = {
    { IRQ_CMCx_PARITY, soc_cmicm_parity_intr, 0, "PARITY INTR" },
    { 0, NULL, 0, "" } /* Termination */
};

STATIC intr_handler_t soc_cmicm_intr_handlers3[] = {
    { IRQ_CMCx_BLOCK(6),  soc_cmicm_block_lo_intr,  6, "BLOCK 6 INTR" },
    { IRQ_CMCx_BLOCK(25), soc_cmicm_block_lo_intr, 25, "BLOCK 25 INTR" },
    { IRQ_CMCx_BLOCK(26), soc_cmicm_block_lo_intr, 26, "BLOCK 26 INTR" },
    { IRQ_CMCx_BLOCK(27), soc_cmicm_block_lo_intr, 27, "BLOCK 27 INTR" },
#ifdef BCM_CALADAN3_SUPPORT
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_OC_INTR_POS), soc_cmicm_block_lo_intr,
      SOC_SBX_CALADAN3_OC_INTR_POS, "BLOCK OCM INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_LRA_INTR_POS), soc_cmicm_block_lo_intr,
      SOC_SBX_CALADAN3_LRA_INTR_POS, "BLOCK LRA INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_LRB_INTR_POS), soc_cmicm_block_lo_intr,
      SOC_SBX_CALADAN3_LRB_INTR_POS, "BLOCK LRB INTR" },
#endif
    { 0, NULL, 0, "" } /* Termination */
};

STATIC intr_handler_t soc_cmicm_intr_handlers4[] = {
#ifdef BCM_CALADAN3_SUPPORT
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE0_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE0_INTR_POS, "BLOCK QE0 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE1_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE1_INTR_POS, "BLOCK QE1 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE2_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE2_INTR_POS, "BLOCK QE2 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE3_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE3_INTR_POS, "BLOCK QE3 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE4_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE4_INTR_POS, "BLOCK QE4 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE5_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE5_INTR_POS, "BLOCK QE5 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE6_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE6_INTR_POS, "BLOCK QE6 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE7_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE7_INTR_POS, "BLOCK QE7 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE8_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE8_INTR_POS, "BLOCK QE8 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE9_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE9_INTR_POS, "BLOCK QE9 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE10_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE10_INTR_POS, "BLOCK QE10 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE11_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE11_INTR_POS, "BLOCK QE11 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE12_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE12_INTR_POS, "BLOCK QE12 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE13_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE13_INTR_POS, "BLOCK QE13 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE14_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE14_INTR_POS, "BLOCK QE14 INTR" },
    { IRQ_CMCx_BLOCK(SOC_SBX_CALADAN3_QE15_INTR_POS%32), soc_cmicm_block_hi_intr,
    SOC_SBX_CALADAN3_QE15_INTR_POS, "BLOCK QE15 INTR" },
#endif
    { 0, NULL, 0, "" } /* Termination */
};

STATIC intr_handler_t soc_cmicdv2_intr_handlers2[] = {
    { 0, NULL, 0, "" } /* Termination */
};

STATIC intr_handler_t soc_cmicdv2_intr_handlers3[] = {
    { IRQ_CMCx_PARITY, soc_cmicdv2_parity_intr, 0, "PARITY INTR" },
    { 0, NULL, 0, "" } /* Termination */
};

STATIC intr_handler_t soc_cmicdv2_intr_handlers4[] = {
    { 0, NULL, 0, "" } /* Termination */
};

/* IRQ0 handler for CMC of SOC_ARM_CMC(unit, 0) 
 *  For now only support TSLAM (for Caladan3 TMU)
 *  and FIFO DMA 0/1 (for Caladan3 COP0/1)
 */
STATIC intr_handler_t soc_cmicm_intr_handlers0_arm_cmc[] = {
    { IRQ_SBUSDMA_CH0_DONE,    soc_cmicm_intr_sbusdma_done,  0, "SBUS_DMA0_DONE"   },
    { IRQ_SBUSDMA_CH1_DONE,    soc_cmicm_intr_sbusdma_done,  1, "SBUS_DMA1_DONE"   },
    { IRQ_SBUSDMA_CH2_DONE,    soc_cmicm_intr_sbusdma_done,  2, "SBUS_DMA2_DONE"   },

    { IRQ_CMCx_FIFO_CH_DMA(0), soc_cmicm_fifo_dma_done, 0, "CH0_FIFO_DMA_DONE" },
    { IRQ_CMCx_FIFO_CH_DMA(1), soc_cmicm_fifo_dma_done, 1, "CH1_FIFO_DMA_DONE" },
    { IRQ_CMCx_FIFO_CH_DMA(2), soc_cmicm_fifo_dma_done, 2, "CH2_FIFO_DMA_DONE" },
    { IRQ_CMCx_FIFO_CH_DMA(3), soc_cmicm_fifo_dma_done, 3, "CH3_FIFO_DMA_DONE" },
    
    { 0, NULL, 0, "" } /* Termination */
};
#endif

#ifdef INCLUDE_RCPU
STATIC intr_handler_t soc_cmicm_rcpu_intr_handlers0[] = {
    { IRQ_RCPU_MIIM_OP_DONE, soc_cmicm_rcpu_intr_miim_op, 0, "RCPU_MIIM_OP_DONE" },
    { 0, NULL, 0, "" } /* Termination */
};
#endif /* INCLUDE_RCPU */
/*
 * Interrupt handler functions
 */

STATIC void
soc_cmicm_intr_schan_done(int unit, uint32 ignored)
{
    soc_control_t    *soc = SOC_CONTROL(unit);
    int cmc = SOC_PCI_CMC(unit);

    COMPILER_REFERENCE(ignored);

    /* Record the schan control regsiter */
    soc->schan_result = soc_pci_read(unit, CMIC_CMCx_SCHAN_CTRL_OFFSET(cmc));
    soc_pci_write(unit, CMIC_CMCx_SCHAN_CTRL_OFFSET(cmc),
        soc_pci_read(unit, CMIC_CMCx_SCHAN_CTRL_OFFSET(cmc)) & ~SC_CMCx_MSG_DONE);

    soc->stat.intr_sc++;

    if (soc->schanIntr) {
        sal_sem_give(soc->schanIntr);
    }
}

STATIC void
soc_cmicm_intr_miim_op(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);
    int cmc = SOC_PCI_CMC(unit);

    COMPILER_REFERENCE(ignored);

    soc_pci_write(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc), 0); /* Clr Read & Write Stat */

    soc->stat.intr_mii++;

    if (soc->miimIntr) {
        sal_sem_give(soc->miimIntr);
    }
}


STATIC void
soc_cmicm_intr_tdma_done(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    (void)soc_cmicm_intr0_disable(unit, IRQ_CMCx_TDMA_DONE);
    
    soc->stat.intr_tdma++;
    
    if (soc->tableDmaIntr) {
    sal_sem_give(soc->tableDmaIntr);
    }
}

STATIC void
soc_cmicm_intr_tslam_done(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    (void)soc_cmicm_intr0_disable(unit, IRQ_CMCx_TSLAM_DONE);
    
    soc->stat.intr_tslam++;
    
    if (soc->tslamDmaIntr) {
    sal_sem_give(soc->tslamDmaIntr);
    }
}

STATIC void
soc_cmicm_intr_stat_dma(int unit, uint32 ignored)
{
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_DPP_SUPPORT) 

    soc_control_t *soc = SOC_CONTROL(unit);
    int cmc = SOC_PCI_CMC(unit);

    COMPILER_REFERENCE(ignored);
    if (SOC_IS_SAND(unit)) {
        return;
    }

    soc_pci_write(unit, CMIC_CMCx_STAT_DMA_CFG_OFFSET(cmc),
        soc_pci_read(unit, CMIC_CMCx_STAT_DMA_CFG_OFFSET(cmc)) | STDMA_ITER_DONE_CLR);

    soc->stat.intr_stats++;

    if (soc->counter_intr) {
        sal_sem_give(soc->counter_intr);
    }
#endif    
}

STATIC void
soc_cmicm_intr_ccmdma_done(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    (void)soc_cmicm_intr0_disable(unit, IRQ_CMCx_CCMDMA_DONE);

    soc->stat.intr_ccmdma++;

    if (soc->ccmDmaIntr) {
        sal_sem_give(soc->ccmDmaIntr);
    }
}

#ifdef BCM_SBUSDMA_SUPPORT
STATIC uint32 _soc_irq_cmic_sbusdma_ch[] = {
    IRQ_SBUSDMA_CH0_DONE, 
    IRQ_SBUSDMA_CH1_DONE,
    IRQ_SBUSDMA_CH2_DONE
};
#endif

STATIC void
soc_cmicm_intr_sbusdma_done(int unit, uint32 ch)
{
#ifdef BCM_SBUSDMA_SUPPORT
    soc_control_t *soc = SOC_CONTROL(unit);
    int cmc;

    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
	cmc = ch / 4;
	ch = ch % 4;
	
	(void)soc_cmicm_cmcx_intr0_disable(unit, cmc, _soc_irq_cmic_sbusdma_ch[ch]);
	
	if (SOC_IS_CALADAN3(unit)) {
#ifdef BCM_CALADAN3_SUPPORT
            uint32 op;
	    
	    /* check what types of dma is used on the channel */
	    if (SOC_FAILURE(soc_sbx_caladan3_sbusdma_cmc_ch_type_get(unit, cmc, ch, &op))) {
		LOG_INFO(BSL_LS_SOC_INTR,
                         (BSL_META_U(unit,
                                     "Received unallocated sbusdma interrupt cmc %d ch %d !!\n"),
                          cmc, ch));
	    } else {
		switch (op) {
		    case SOC_SBUSDMA_TYPE_TDMA:
			soc->stat.intr_tdma++;
			if (soc->tableDmaIntrEnb) {
			    sal_sem_give(soc->sbusDmaIntrs[cmc][ch]);
			}
			break;
		    case SOC_SBUSDMA_TYPE_SLAM:
			soc->stat.intr_tslam++;
			if (soc->tslamDmaIntrEnb) {
			    sal_sem_give(soc->sbusDmaIntrs[cmc][ch]);
			}
			break;
		    case SOC_SBUSDMA_TYPE_DESC:
			soc->stat.intr_desc++;
			if (SOC_SBUSDMA_INFO(unit) && SOC_SBUSDMA_DM_INTRENB(unit)) {
			    sal_sem_give(SOC_SBUSDMA_DM_INTR(unit));
			}
			break;
		    default:
			LOG_INFO(BSL_LS_SOC_INTR,
                                 (BSL_META_U(unit,
                                             "Received unallocated sbusdma interrupt !!\n")));
			break;
		}
	    }
#endif /* BCM_CALADAN3_SUPPORT */
        } else {
	    LOG_INFO(BSL_LS_SOC_INTR,
                     (BSL_META_U(unit,
                                 "Received unallocated sbusdma interrupt !!\n")));
	}
    } else {
        (void)soc_cmicm_intr0_disable(unit, _soc_irq_cmic_sbusdma_ch[ch]);
        if (ch == soc->tdma_ch) {
            soc->stat.intr_tdma++;
            if (soc->tableDmaIntrEnb) {
                sal_sem_give(soc->tableDmaIntr);
            }
        } else if (ch == soc->tslam_ch) {
            soc->stat.intr_tslam++;
            if (soc->tslamDmaIntrEnb) {
                sal_sem_give(soc->tslamDmaIntr);
            }
        } else if (ch == soc->desc_ch) {
            soc->stat.intr_desc++;
            if (SOC_SBUSDMA_INFO(unit) && SOC_SBUSDMA_DM_INTRENB(unit)) {
                sal_sem_give(SOC_SBUSDMA_DM_INTR(unit));
            }
        } else {
            LOG_INFO(BSL_LS_SOC_INTR,
                     (BSL_META_U(unit,
                                 "Received unallocated sbusdma interrupt !!\n")));
        }
    }
#else
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(ch);
#endif
}

STATIC void
soc_cmicm_fifo_dma_done(int unit, uint32 ch)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    if (SOC_IS_CALADAN3(unit)) {
#ifdef BCM_CALADAN3_SUPPORT
        int cmc;

        cmc = ch / 4;
        ch = ch % 4;
        
        (void)soc_cmicm_cmcx_intr0_disable(unit, cmc, IRQ_CMCx_FIFO_CH_DMA(ch));

	/* account here even if it's not handled */
	SOC_CONTROL(unit)->stat.intr_fifo_dma[ch]++;
       
        switch (cmc) {
            case CMC0:        
            switch (ch) {
                case SOC_MEM_FIFO_DMA_CHANNEL_0:
                /* CMU timeout/overflow interrupt */
                sal_dpc(soc_sbx_caladan3_cmu_ring_processing_wakeup, INT_TO_PTR(unit),
                    0, 0, 0, 0);
                break; 
                default:
                LOG_INFO(BSL_LS_SOC_INTR,
                         (BSL_META_U(unit,
                                     "Received unallocated fifo dma interrupt !!\n")));
            }
            break;
            case CMC1:
            switch (ch) {
                case SOC_MEM_FIFO_DMA_CHANNEL_0:
                case SOC_MEM_FIFO_DMA_CHANNEL_1:
                /* COP0/1 timeout/overflow interrupt */
                sal_dpc(soc_sbx_caladan3_cop_ring_processing_wakeup, INT_TO_PTR(unit),
                    INT_TO_PTR((ch==SOC_MEM_FIFO_DMA_CHANNEL_0)?0:1), 0, 0, 0);
                break;
                default:
                LOG_INFO(BSL_LS_SOC_INTR,
                         (BSL_META_U(unit,
                                     "Received unallocated fifo dma interrupt !!\n")));
            }
            break;
            default:
            LOG_INFO(BSL_LS_SOC_INTR,
                     (BSL_META_U(unit,
                                 "Received unallocated fifo dma interrupt !!\n")));
        }
#endif
    }
    else if ((SOC_IS_ARADPLUS(unit)) && (ch == SOC_MEM_FIFO_DMA_CHANNEL_3))  {
#ifdef BCM_DPP_SUPPORT
        (void)soc_cmicm_intr0_disable(unit, IRQ_CMCx_FIFO_CH_DMA(ch));
        sal_dpc(arad_pp_oam_dma_event_handler, INT_TO_PTR(unit), 0,0,0,0 );
#endif
    }

    else {
        (void)soc_cmicm_intr0_disable(unit, IRQ_CMCx_FIFO_CH_DMA(ch));

        switch (ch) {
            case SOC_MEM_FIFO_DMA_CHANNEL_0:
                if (SOC_CONTROL(unit)->ftreportIntrEnb) {
                    SOC_CONTROL(unit)->stat.intr_fifo_dma[ch]++;
                    sal_sem_give(SOC_CONTROL(unit)->ftreportIntr);
                } else if (SOC_IS_TD2_TT2(unit) && soc->l2modDmaIntrEnb) {
                    /* L2 fifo dma ch = SOC_MEM_FIFO_DMA_CHANNEL_0; */
                    SOC_CONTROL(unit)->stat.intr_fifo_dma[ch]++;
                    sal_sem_give(soc->arl_notify);
                }
                break;
            case SOC_MEM_FIFO_DMA_CHANNEL_1:
                if (soc->l2modDmaIntrEnb) {
                    SOC_CONTROL(unit)->stat.intr_fifo_dma[ch]++;
                    sal_sem_give(soc->arl_notify);
                }
                break;
            case SOC_MEM_FIFO_DMA_CHANNEL_2:
                if (SOC_CONTROL(unit)->ipfixIntrEnb) {
                    SOC_CONTROL(unit)->stat.intr_fifo_dma[ch]++;
                    sal_sem_give(SOC_CONTROL(unit)->ipfixIntr);
                }
                break;
            case SOC_MEM_FIFO_DMA_CHANNEL_3:
                if (SOC_CONTROL(unit)->ipfixIntrEnb) {
                    SOC_CONTROL(unit)->stat.intr_fifo_dma[ch]++;
                    sal_sem_give(SOC_CONTROL(unit)->ipfixIntr);
                }
                break;
            default:
                LOG_INFO(BSL_LS_SOC_INTR,
                         (BSL_META_U(unit,
                                     "Received unallocated fifo dma interrupt !!\n")));
            }
        }
}

#if defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DPP_SUPPORT)
STATIC void
soc_cmicm_chip_func_intr(int unit, uint32 val)
{
    int cmc = SOC_PCI_CMC(unit);
    uint32 irqStat;
#if defined (BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    uint32 irqMask, oldmask;

    oldmask = 0;

    irqMask = SOC_CMCx_IRQ1_MASK(unit,cmc);
#endif

    irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT1_OFFSET(cmc));

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)) {
        if (irqStat & ~_SOC_TD2_FUNC_INTR_MASK) {
            (void)soc_cmicm_intr1_disable(unit, irqStat &
                                          ~_SOC_TD2_FUNC_INTR_MASK);
        }

        if (irqStat & _SOC_TD2_FUNC_INTR_MASK) {
            oldmask = soc_cmicm_intr1_disable(unit, irqMask);

            /* dispatch interrupt */
            LOG_INFO(BSL_LS_SOC_INTR,
                     (BSL_META_U(unit,
                                 "soc_cmicm_intr type 1 unit %d: dispatch\n"),
                      unit));

            sal_dpc(soc_td2_process_func_intr, INT_TO_PTR(unit),
                    INT_TO_PTR(oldmask), 0, 0, 0);
        }
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        if (soc_feature(unit, soc_feature_esm_correction)) {
            if (irqStat & ~(_SOC_TR3_FUNC_PARITY_INTR_MASK |
                            _SOC_TR3_ESM_INTR_MASK)) {
                (void)soc_cmicm_intr1_disable(unit, irqStat &
                                              ~(_SOC_TR3_FUNC_PARITY_INTR_MASK |
                                                _SOC_TR3_ESM_INTR_MASK));
            }

            if (irqStat & (_SOC_TR3_FUNC_PARITY_INTR_MASK |
                            _SOC_TR3_ESM_INTR_MASK)) {
                oldmask = soc_cmicm_intr1_disable(unit, irqMask);
                /* dispatch interrupt */
                LOG_INFO(BSL_LS_SOC_INTR,
                         (BSL_META_U(unit,
                                     "soc_cmicm_intr type 1 unit %d: dispatch\n"),
                          unit));

                if (irqStat & _SOC_TR3_ESM_INTR_MASK) {
                    sal_sem_give(SOC_CONTROL(unit)->esm_recovery_notify);
                }
                sal_dpc(soc_tr3_process_func_intr, INT_TO_PTR(unit),
                        INT_TO_PTR(oldmask), 0, 0, 0);
            }
        } else {
            if (irqStat & ~_SOC_TR3_FUNC_PARITY_INTR_MASK) {
                (void)soc_cmicm_intr1_disable(unit, irqStat &
                                              ~_SOC_TR3_FUNC_PARITY_INTR_MASK);
            }

            if (irqStat & _SOC_TR3_FUNC_PARITY_INTR_MASK) {
                oldmask = soc_cmicm_intr1_disable(unit, irqMask);

                /* dispatch interrupt */
                LOG_INFO(BSL_LS_SOC_INTR,
                         (BSL_META_U(unit,
                                     "soc_cmicm_intr type 1 unit %d: dispatch\n"),
                          unit));

                sal_dpc(soc_tr3_process_func_intr, INT_TO_PTR(unit),
                    INT_TO_PTR(oldmask), 0, 0, 0);
            }

        }
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_cmicm_intr unit %d: "
                              "Disabling unhandled interrupt(s): %d\n"), unit, irqStat));
        (void)soc_cmicm_intr1_disable(unit, irqStat);
    }
}
#endif /* (BCM_ESW_SUPPORT) || (BCM_CALADAN3_SUPPORT)  || defined(BCM_DPP_SUPPORT) */
#if defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DPP_SUPPORT)
STATIC void
soc_cmicm_intr_link_stat(int unit, uint32 ignored)
{
    soc_control_t    *soc = SOC_CONTROL(unit);
    uint32 rval = 0;

    COMPILER_REFERENCE(ignored);

    soc_pci_analyzer_trigger(unit);

    soc->stat.intr_ls++;

    /* Clear interrupt */
    READ_CMIC_MIIM_SCAN_STATUSr(unit, &rval);
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "Status: 0x%08x\n"), rval));
    WRITE_CMIC_MIIM_CLR_SCAN_STATUSr(unit, rval);

    /* Perform user callout, if one is registered */

    if (soc->soc_link_callout != NULL) {
        (*soc->soc_link_callout)(unit);
    }
}

STATIC void
soc_ser_engine_intr(int unit, uint32 val)
{
    int cmc;
    uint32 irqMask, irqStat;

    cmc = SOC_PCI_CMC(unit);
    irqMask = SOC_CMCx_IRQ1_MASK(unit,cmc);
    irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT1_OFFSET(cmc));

    (void)soc_cmicm_intr1_disable(unit, irqMask);

    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "soc_cmicm_intr unit %d: "
                          "Disabling unhandled interrupt(s): %d\n"),
               unit, irqStat));
    (void)soc_cmicm_intr1_disable(unit, irqStat);
}

STATIC void
soc_cmicm_parity_intr(int unit, uint32 val)
{
    int cmc;
    uint32 irqStat;
#if defined(BCM_XGS_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) 
    uint32 irqMask, oldmask;
#endif

    cmc = SOC_PCI_CMC(unit);
    irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT2_OFFSET(cmc));

#if defined(BCM_XGS_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) 
    irqMask = SOC_CMCx_IRQ2_MASK(unit,cmc);

    oldmask = soc_cmicm_intr2_disable(unit, irqMask);

    /* dispatch interrupt if we have handler */
    if (soc_ser_parity_error_cmicm_intr(INT_TO_PTR(unit), 0,
        INT_TO_PTR(oldmask), 0, 0)) {
        LOG_INFO(BSL_LS_SOC_INTR,
                 (BSL_META_U(unit,
                             "soc_cmicm_intr type 2 unit %d: dispatch\n"),
                  unit));
    } else 
#endif
    {
/* Decoupling OAM interrupt handler from parity interrupt handler */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
             sal_dpc((sal_dpc_fn_t)soc_tr3_process_func_intr, INT_TO_PTR(unit),
                       0, INT_TO_PTR(oldmask), 0, 0);

    } else
#endif
    {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_cmicm_intr unit %d: "
                              "Disabling unhandled interrupt(s): %d\n"), 
                   unit, irqStat));
        (void)soc_cmicm_intr2_disable(unit, irqStat);
    }
    }
}

STATIC void
soc_cmicdv2_parity_intr(int unit, uint32 val)
{
    int cmc;
    uint32 irqStat;
#if defined(BCM_XGS_SUPPORT)
    uint32 irqMask, oldmask;
#endif

    cmc = SOC_PCI_CMC(unit);
    /* The parity error interrupts are moved to STAT3 for CMICD v2 */
    irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT3_OFFSET(cmc));

#if defined(BCM_XGS_SUPPORT)
    irqMask = SOC_CMCx_IRQ3_MASK(unit,cmc);

    oldmask = soc_cmicm_intr3_disable(unit, irqMask);

    /* dispatch interrupt if we have handler */
    if (soc_ser_parity_error_cmicm_intr(INT_TO_PTR(unit), 0,
        INT_TO_PTR(oldmask), 0, 0)) {
        LOG_INFO(BSL_LS_SOC_INTR,
                 (BSL_META_U(unit,
                             "soc_cmicdv2_intr type 3 unit %d: dispatch\n"),
                  unit));
        
    } else 
#endif
    {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_cmicdv2_intr unit %d: "
                              "Disabling unhandled interrupt(s): %d\n"), 
                   unit, irqStat));
        
        (void)soc_cmicm_intr3_disable(unit, irqStat);
    }
}


STATIC void
soc_cmicm_block_lo_intr(int unit, uint32 val)
{
    uint32  irqStat = 0;
    int cmc;

    cmc = SOC_PCI_CMC(unit);
    irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT3_OFFSET(cmc));

#ifdef BCM_CALADAN3_SUPPORT
    if (SOC_IS_CALADAN3(unit)) {
        uint32 irqMask = SOC_CMCx_IRQ3_MASK(unit,cmc);

        soc_cmicm_intr3_disable(unit, irqMask & irqStat);

        /* dispatch interrupt */
        LOG_INFO(BSL_LS_SOC_INTR,
                 (BSL_META_U(unit,
                             "soc_cmicm_intr type 3 unit %d: dispatch\n"),
                  unit));
	
	SOC_CONTROL(unit)->stat.intr_block++;
	switch (val) {
	    case SOC_SBX_CALADAN3_TMB_INTR_POS: /* 6 - Caladan3 TMB - interrupt */
		sal_dpc(soc_sbx_caladan3_tmu_isr, INT_TO_PTR(unit),
			0, 0, 0, 0);                
		break;
             case SOC_SBX_CALADAN3_OC_INTR_POS:
                 LOG_INFO(BSL_LS_SOC_INTR,
                          (BSL_META_U(unit,
                                      "%d: Caladan3 OCM ISR\n"),
                           unit));
		sal_dpc(soc_sbx_caladan3_ocm_isr, INT_TO_PTR(unit),
			0, 0, 0, 0);  
                 break;
	    case SOC_SBX_CALADAN3_CO1_INTR_POS:
	    case SOC_SBX_CALADAN3_CO0_INTR_POS:
		/* 25,26 interrupts from COP block */
		break;
	    case SOC_SBX_CALADAN3_CM_INTR_POS:
		/* 27 interrupts from CMU block, we handle manual ejection done here 
		 * and mask out every thing else
		 */
		sal_dpc(soc_sbx_caladan3_cmu_ring_processing_wakeup, INT_TO_PTR(unit),
			0, 0, 0, 0);
		break;
	    case SOC_SBX_CALADAN3_LRA_INTR_POS:
		sal_dpc(soc_sbx_caladan3_lr_isr, INT_TO_PTR(unit),
			0, 0, 0, 0);
		break;
	    case SOC_SBX_CALADAN3_LRB_INTR_POS:
		sal_dpc(soc_sbx_caladan3_lrb_isr, INT_TO_PTR(unit),
			0, 0, 0, 0);
		break;
	    default:
		LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "soc_cmicm_intr unit %d: "
                                      "Disabling unhandled interrupt(s): %d\n"), 
                           unit, irqStat));
		(void)soc_cmicm_intr3_disable(unit, irqStat);
	}        
    } else 
#endif /* BCM_CALADAN3_SUPPORT */
    {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_cmicm_intr unit %d: "
                              "Disabling unhandled interrupt(s): %d\n"), 
                   unit, irqStat));
        (void)soc_cmicm_intr3_disable(unit, irqStat);
    }
}

#if defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DPP_SUPPORT)
STATIC void
soc_cmicm_link_stat(int unit, uint32 ignored)
{
    soc_control_t    *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    soc_pci_analyzer_trigger(unit);

    soc->stat.intr_ls++;

    /* Clear interrupt */

    soc_pci_write(unit, CMIC_MIIM_CLR_SCAN_STATUS_OFFSET, CLR_LINK_STATUS_CHANGE_MASK);

    /* Perform user callout, if one is registered */

    if (soc->soc_link_callout != NULL) {
    (*soc->soc_link_callout)(unit);
    }
}
#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DPP_SUPPORT) */

#ifdef BCM_CALADAN3_SUPPORT
STATIC void
soc_cmicm_block_hi_intr(int unit, uint32 val)
{
    uint32  irqStat = 0;
    int cmc;

    cmc = SOC_PCI_CMC(unit);
    irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT4_OFFSET(cmc));

    if (SOC_IS_CALADAN3(unit)) {
        uint32 irqMask = SOC_CMCx_IRQ4_MASK(unit,cmc);

        soc_cmicm_intr4_disable(unit, irqMask & irqStat);

        /* dispatch interrupt */
        LOG_INFO(BSL_LS_SOC_INTR,
                 (BSL_META_U(unit,
                             "soc_cmicm_intr type 4 unit %d: dispatch\n"),
                  unit));
	
    	SOC_CONTROL(unit)->stat.intr_block++;
    	switch (val) {
            case SOC_SBX_CALADAN3_QE0_INTR_POS:
            case SOC_SBX_CALADAN3_QE1_INTR_POS:
            case SOC_SBX_CALADAN3_QE2_INTR_POS:
            case SOC_SBX_CALADAN3_QE3_INTR_POS:
            case SOC_SBX_CALADAN3_QE4_INTR_POS:
            case SOC_SBX_CALADAN3_QE5_INTR_POS:
            case SOC_SBX_CALADAN3_QE6_INTR_POS:
            case SOC_SBX_CALADAN3_QE7_INTR_POS:
            case SOC_SBX_CALADAN3_QE8_INTR_POS:
            case SOC_SBX_CALADAN3_QE9_INTR_POS:
            case SOC_SBX_CALADAN3_QE10_INTR_POS:
            case SOC_SBX_CALADAN3_QE11_INTR_POS:
    	    case SOC_SBX_CALADAN3_QE12_INTR_POS:
            case SOC_SBX_CALADAN3_QE13_INTR_POS:
            case SOC_SBX_CALADAN3_QE14_INTR_POS:
            case SOC_SBX_CALADAN3_QE15_INTR_POS:
        		sal_dpc(soc_sbx_caladan3_tmu_qe_isr, INT_TO_PTR(unit),
        			INT_TO_PTR(val), 0, 0, 0);                
        		break;
    	    default:
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "soc_cmicm_intr unit %d: "
                                      "Disabling unhandled interrupt(s): %d\n"), 
                           unit, irqStat));
        		(void)soc_cmicm_intr4_disable(unit, irqStat);
    	}        
    } else 
    {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_cmicm_intr unit %d: "
                              "Disabling unhandled interrupt(s): %d\n"), 
                   unit, irqStat));
        (void)soc_cmicm_intr4_disable(unit, irqStat);
    }
}
#endif /* BCM_CALADAN3_SUPPORT */
#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DPP_SUPPORT) */

/*
 * Enable (unmask) or disable (mask) a set of CMIC interrupts.  These
 * routines should be used instead of manipulating CMIC_IRQ_MASK
 * directly, since a read-modify-write is required.  The return value is
 * the previous mask (can pass mask of 0 to just get the current mask).
 * for CMICm use CMIC_CMCx_PCIE_IRQ_MASK0.
 */

uint32
soc_cmicm_cmcx_intr0_enable(int unit, int cmc, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_CMCx_IRQ0_MASK(unit,cmc);
    SOC_CMCx_IRQ0_MASK(unit,cmc) |= mask;
    newMask = SOC_CMCx_IRQ0_MASK(unit,cmc);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    LOG_INFO(BSL_LS_SOC_INTR,
             (BSL_META_U(unit,
                         "soc_cmicm_intr0_enable cmc %d unit %d: mask 0x%8x\n"),
              cmc, unit, mask));

    IRQ_MASK0_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(cmc), newMask);

    sal_spl(s);

    return oldMask;
}

uint32
soc_cmicm_cmcx_intr0_disable(int unit, int cmc, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_CMCx_IRQ0_MASK(unit,cmc);
    SOC_CMCx_IRQ0_MASK(unit,cmc) &= ~mask;
    newMask = SOC_CMCx_IRQ0_MASK(unit,cmc);

    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    LOG_INFO(BSL_LS_SOC_INTR,
             (BSL_META_U(unit,
                         "soc_cmicm_intr0_disable cmc %d unit %d: mask 0x%8x\n"),
              cmc, unit, mask));

    IRQ_MASK0_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(cmc), newMask);

    sal_spl(s);

    return oldMask;
}

/*
 * Enable (unmask) or disable (mask) a set of CMICM Common / Switch-Specific 
 * interrupts.  These routines should be used instead of manipulating 
 * CMIC_CMCx_PCIE_IRQ_MASK1 directly, since a read-modify-write is required.
 * The return value is the previous mask (can pass mask of 0 to just
 * get the current mask) 
 */

uint32
soc_cmicm_cmcx_intr1_enable(int unit, int cmc, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_CMCx_IRQ1_MASK(unit,cmc);
    SOC_CMCx_IRQ1_MASK(unit,cmc) |= mask;
    newMask = SOC_CMCx_IRQ1_MASK(unit,cmc);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(cmc), newMask);
    sal_spl(s);

    return oldMask;
}


uint32
soc_cmicm_cmcx_intr1_disable(int unit, int cmc, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_CMCx_IRQ1_MASK(unit,cmc);
    SOC_CMCx_IRQ1_MASK(unit,cmc) &= ~mask;
    newMask = SOC_CMCx_IRQ1_MASK(unit,cmc);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(cmc), newMask);
    sal_spl(s);

    return oldMask;
}

uint32
soc_cmicm_cmcx_intr2_enable(int unit, int cmc, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_CMCx_IRQ2_MASK(unit,cmc);
    SOC_CMCx_IRQ2_MASK(unit,cmc) |= mask;
    newMask = SOC_CMCx_IRQ2_MASK(unit,cmc);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc), newMask);
    sal_spl(s);

    return oldMask;
}


uint32
soc_cmicm_cmcx_intr2_disable(int unit, int cmc, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_CMCx_IRQ2_MASK(unit,cmc);
    SOC_CMCx_IRQ2_MASK(unit,cmc) &= ~mask;
    newMask = SOC_CMCx_IRQ2_MASK(unit,cmc);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc), newMask);
    sal_spl(s);

    return oldMask;
}

uint32
soc_cmicm_cmcx_intr3_enable(int unit, int cmc, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_CMCx_IRQ3_MASK(unit,cmc);
    SOC_CMCx_IRQ3_MASK(unit,cmc) |= mask;
    newMask = SOC_CMCx_IRQ3_MASK(unit,cmc);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }

    IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc), newMask);
    sal_spl(s);

    return oldMask;
}


uint32
soc_cmicm_cmcx_intr3_disable(int unit, int cmc, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_CMCx_IRQ3_MASK(unit,cmc);
    SOC_CMCx_IRQ3_MASK(unit,cmc) &= ~mask;
    newMask = SOC_CMCx_IRQ3_MASK(unit,cmc);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc), newMask);
    sal_spl(s);

    return oldMask;
}

uint32
soc_cmicm_cmcx_intr4_enable(int unit, int cmc, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_CMCx_IRQ4_MASK(unit,cmc);
    SOC_CMCx_IRQ4_MASK(unit,cmc) |= mask;
    newMask = SOC_CMCx_IRQ4_MASK(unit,cmc);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(cmc), newMask);
    sal_spl(s);

    return oldMask;
}


uint32
soc_cmicm_cmcx_intr4_disable(int unit, int cmc, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_CMCx_IRQ4_MASK(unit,cmc);
    SOC_CMCx_IRQ4_MASK(unit,cmc) &= ~mask;
    newMask = SOC_CMCx_IRQ4_MASK(unit,cmc);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(cmc), newMask);
    sal_spl(s);

    return oldMask;
}

uint32
soc_cmicm_intr0_enable(int unit, uint32 mask)
{
    return soc_cmicm_cmcx_intr0_enable(unit, SOC_PCI_CMC(unit), mask);
}

uint32
soc_cmicm_intr0_disable(int unit, uint32 mask)
{
    return soc_cmicm_cmcx_intr0_disable(unit, SOC_PCI_CMC(unit), mask);
}

uint32
soc_cmicm_intr1_enable(int unit, uint32 mask)
{
    return soc_cmicm_cmcx_intr1_enable(unit, SOC_PCI_CMC(unit), mask);
}

uint32
soc_cmicm_intr1_disable(int unit, uint32 mask)
{
    return soc_cmicm_cmcx_intr1_disable(unit, SOC_PCI_CMC(unit), mask);
}

uint32
soc_cmicm_intr2_enable(int unit, uint32 mask)
{
    return soc_cmicm_cmcx_intr2_enable(unit, SOC_PCI_CMC(unit), mask);
}

uint32
soc_cmicm_intr2_disable(int unit, uint32 mask)
{
    return soc_cmicm_cmcx_intr2_disable(unit, SOC_PCI_CMC(unit), mask);
}

uint32
soc_cmicm_intr3_enable(int unit, uint32 mask)
{
    return soc_cmicm_cmcx_intr3_enable(unit, SOC_PCI_CMC(unit), mask);
}

uint32
soc_cmicm_intr3_disable(int unit, uint32 mask)
{
    return soc_cmicm_cmcx_intr3_disable(unit, SOC_PCI_CMC(unit), mask);
}

uint32
soc_cmicm_intr4_enable(int unit, uint32 mask)
{
    return soc_cmicm_cmcx_intr4_enable(unit, SOC_PCI_CMC(unit), mask);
}

uint32
soc_cmicm_intr4_disable(int unit, uint32 mask)
{
    return soc_cmicm_cmcx_intr4_disable(unit, SOC_PCI_CMC(unit), mask);
}

/*
 * SOC CMICm Interrupt Service Routine
 */

#define POLL_LIMIT 100000

void
soc_cmicm_intr(void *_unit)
{
    uint32 irqStat, irqMask;
    int cmc, i = 0;
    int poll_limit = POLL_LIMIT;
    int unit = PTR_TO_INT(_unit);
    intr_handler_t *intr_handler = soc_cmicm_intr_handlers;
    soc_control_t *soc;
    int arm;

#ifdef SAL_SPL_LOCK_ON_IRQ
    int s;

    s = sal_splhi();
#endif

    soc = SOC_CONTROL(unit);
    /*
     * Our handler is permanently registered in soc_probe().  If our
     * unit is not attached yet, it could not have generated this
     * interrupt.  The interrupt line must be shared by multiple PCI
     * cards.  Simply ignore the interrupt and let another handler
     * process it.
     */
    if (soc == NULL || (soc->soc_flags & SOC_F_BUSY) ||
        !(soc->soc_flags & SOC_F_ATTACHED)) {
#ifdef SAL_SPL_LOCK_ON_IRQ
        sal_spl(s);
#endif
        return;
    }
    cmc = SOC_PCI_CMC(unit);

    soc->stat.intr++; /* Update count */

    if (SOC_IS_KATANA(unit)) {
        intr_handler = soc_cmicm_intr_handlers0_fifo_dma;
    }

    if (soc_feature(unit, soc_feature_sbusdma)) {
        intr_handler = soc_cmicm_intr_handlers0;
    }
    /*
     * Read IRQ Status and IRQ Mask and AND to determine active ints.
     * These are re-read each time since either can be changed by ISRs.
     */
    for (;;) {
        irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT0_OFFSET(cmc));
        if (irqStat == 0) {
            goto check_type1;  /* No pending Interrupts */
        }
        irqMask = SOC_CMCx_IRQ0_MASK(unit,cmc);
        irqStat &= irqMask;
        if (irqStat == 0) {
            goto check_type1;
        }
    
        i = 0;
        /*
        * We may have received an interrupt before all data has been
        * posted from the device or intermediate bridge. 
        * The PCI specification requires that we read a device register
        * to make sure pending data is flushed. 
        */
        soc_pci_read(unit, CMIC_CMCx_SCHAN_CTRL_OFFSET(cmc)); 
        soc_pci_read(unit, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(cmc));
    
        for (; intr_handler[i].mask; i++) {
            if (irqStat & intr_handler[i].mask) {

            /* dispatch interrupt */
            LOG_INFO(BSL_LS_SOC_INTR,
                     (BSL_META_U(unit,
                                 "soc_cmicm_intr type 0 unit %d: dispatch %s\n"),
                      unit, intr_handler[i].intr_name));
    
            (*intr_handler[i].intr_fn)
                (unit, intr_handler[i].intr_data);
    
            /*
             * Prevent infinite loop in interrupt handler by
             * disabling the offending interrupt(s).
             */
            if (--poll_limit == 0) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "soc_cmicm_intr unit %d: "
                                      "ERROR can't clear type 0 interrupt(s): "
                                      "IRQ=0x%x (disabling 0x%x)\n"),
                           unit, irqStat, intr_handler[i].mask));
                (void)soc_cmicm_intr0_disable(unit, intr_handler[i].mask);
                poll_limit = POLL_LIMIT;
            }
    
            /*
             * Go back and re-read IRQ status.  Start processing
             * from scratch since handler may clear more than one
             * bit. We don't leave the ISR until all of the bits
             * have been cleared and their handlers called.
             */
            break;
            }
        }
    }
check_type1:

#if defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DPP_SUPPORT)
    /* process irq1 (chip_func) */
    for (;;) {
        irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT1_OFFSET(cmc));
        if (irqStat == 0) {
            goto check_type2;  /* No pending Interrupts */
        }
        irqMask = SOC_CMCx_IRQ1_MASK(unit,cmc);
        irqStat &= irqMask;
        if (irqStat == 0) {
            goto check_type2;
        }
        
        intr_handler = soc_cmicm_intr_handlers1;
        poll_limit = POLL_LIMIT;
        i = 0;
        
        for (; intr_handler[i].mask; i++) {
            if (irqStat & intr_handler[i].mask) {
                /* dispatch interrupt */
                LOG_INFO(BSL_LS_SOC_INTR,
                         (BSL_META_U(unit,
                                     "soc_cmicm_intr type 1 unit %d: dispatch %s\n"),
                          unit, intr_handler[i].intr_name));
                
                (*intr_handler[i].intr_fn)
                    (unit, intr_handler[i].intr_data);
                
                if (--poll_limit == 0) {
                    LOG_ERROR(BSL_LS_SOC_COMMON,
                              (BSL_META_U(unit,
                                          "soc_cmicm_intr unit %d: "
                                          "ERROR can't clear type 1 interrupt(s): "
                                          "IRQ=0x%x (disabling 0x%x)\n"),
                               unit, irqStat, intr_handler[i].mask));
                    (void)soc_cmicm_intr1_disable(unit, intr_handler[i].mask);
                    poll_limit = POLL_LIMIT;
                }
                break;
            }
        }
        
        /* optimization: don't go back to re-read (PCI transaction) when there is 
         * only 1 interrupt handled in IRQ1 (for now, this is true)
         */
        if (sizeof(soc_cmicm_intr_handlers1)/sizeof(intr_handler_t) == 2) {
            break;
        }
    }
check_type2:

    /* process irq2 (parity error) */
    for (;;) {
        irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT2_OFFSET(cmc));
        if (irqStat == 0) {
            goto check_type3;  /* No pending Interrupts */
        }
        irqMask = SOC_CMCx_IRQ2_MASK(unit,cmc);
        irqStat &= irqMask;
        if (irqStat == 0) {
            goto check_type3;
        }

        if (soc_feature(unit, soc_feature_cmicd_v2)) {
            intr_handler = soc_cmicdv2_intr_handlers2;
        } else {
            intr_handler = soc_cmicm_intr_handlers2;
        }
        poll_limit = POLL_LIMIT;
        i = 0;
        
        for (; intr_handler[i].mask; i++) {
            if (irqStat & intr_handler[i].mask) {
                /* dispatch interrupt */
                LOG_INFO(BSL_LS_SOC_INTR,
                         (BSL_META_U(unit,
                                     "soc_cmicm_intr type 2 unit %d: dispatch %s\n"),
                          unit, intr_handler[i].intr_name));
                
                (*intr_handler[i].intr_fn)
                    (unit, intr_handler[i].intr_data);
                
                if (--poll_limit == 0) {
                    /* coverity [dead_error_begin] */
                    LOG_ERROR(BSL_LS_SOC_COMMON,
                              (BSL_META_U(unit,
                                          "soc_cmicm_intr unit %d: "
                                          "ERROR can't clear type 2 interrupt(s): "
                                          "IRQ=0x%x (disabling 0x%x)\n"),
                               unit, irqStat, intr_handler[i].mask));
                    (void)soc_cmicm_intr2_disable(unit, intr_handler[i].mask);
                    poll_limit = POLL_LIMIT;
                }
                break;
            }
        }
        
        /* optimization: don't go back to re-read (PCI transaction) when there is 
         * only 1 interrupt handled in IRQ1 (for now, this is true)
         */
        if (sizeof(soc_cmicm_intr_handlers1)/sizeof(intr_handler_t) == 2) {
            break;
        }
    }
check_type3:

    if (soc_feature(unit, soc_feature_cmicm_extended_interrupts)) {
        /* this enable processing of IRQ3/4, the sbus slave interrupts */
        for (;;) {
            irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT3_OFFSET(cmc));
            if (irqStat == 0) {
                goto check_type4;
            }
            irqMask = SOC_CMCx_IRQ3_MASK(unit,cmc);
            irqStat &= irqMask;
            if (irqStat == 0) {
                goto check_type4;
            }

            if (soc_feature(unit, soc_feature_cmicd_v2)) {
                intr_handler = soc_cmicdv2_intr_handlers3;
            } else {
                intr_handler = soc_cmicm_intr_handlers3;
            }
            poll_limit = POLL_LIMIT;
            i = 0;
            
            for (; intr_handler[i].mask; i++) {
                if (irqStat & intr_handler[i].mask) {
                    
                    /* dispatch interrupt */
                    LOG_INFO(BSL_LS_SOC_INTR,
                             (BSL_META_U(unit,
                                         "soc_cmicm_intr type 3 unit %d: dispatch %s\n"),
                              unit, intr_handler[i].intr_name));
                    
                    (*intr_handler[i].intr_fn)
                    (unit, intr_handler[i].intr_data);
                    
                    if (--poll_limit == 0) {
                        LOG_ERROR(BSL_LS_SOC_COMMON,
                                  (BSL_META_U(unit,
                                              "soc_cmicm_intr unit %d: "
                                              "ERROR can't clear type 3 interrupt(s): "
                                              "IRQ=0x%x (disabling 0x%x)\n"),
                                   unit, irqStat, intr_handler[i].mask));
                        (void)soc_cmicm_intr3_disable(unit, intr_handler[i].mask);
                        poll_limit = POLL_LIMIT;
                    }
                    
                    /* sbus slave interrupt is per block, assuming that
                     * handler will only clear interrupt for its own block
                     * reduce PCI transaction with this assumption
                     */
                    /* break; */
                }
            }
        }

check_type4:
        for (;;) {
            irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT4_OFFSET(cmc));
            if (irqStat == 0) {
                goto check_arm_type0;
            }
            irqMask = SOC_CMCx_IRQ4_MASK(unit,cmc);
            irqStat &= irqMask;
            if (irqStat == 0) {
                goto check_arm_type0;
            }

            if (soc_feature(unit, soc_feature_cmicd_v2)) {
                intr_handler = soc_cmicdv2_intr_handlers4;
            } else {
                intr_handler = soc_cmicm_intr_handlers4;
            }
            poll_limit = POLL_LIMIT;
            i = 0;
            
            for (; intr_handler[i].mask; i++) {
                if (irqStat & intr_handler[i].mask) {
                    
                    /* dispatch interrupt */
                    LOG_INFO(BSL_LS_SOC_INTR,
                             (BSL_META_U(unit,
                                         "soc_cmicm_intr type 4 unit %d: dispatch %s\n"),
                              unit, intr_handler[i].intr_name));
                    
                    (*intr_handler[i].intr_fn)
                    (unit, intr_handler[i].intr_data);
                    
                    if (--poll_limit == 0) {
                        LOG_ERROR(BSL_LS_SOC_COMMON,
                                  (BSL_META_U(unit,
                                              "soc_cmicm_intr unit %d: "
                                              "ERROR can't clear type 4 interrupt(s): "
                                              "IRQ=0x%x (disabling 0x%x)\n"),
                                   unit, irqStat, intr_handler[i].mask));
                        (void)soc_cmicm_intr4_disable(unit, intr_handler[i].mask);
                        poll_limit = POLL_LIMIT;
                    }
                    
                    /* sbus slave interrupt is per block, assuming that
                     * handler will only clear interrupt for its own block
                     * reduce PCI transaction with this assumption
                     */
                    /* break; */
                }
            }
        }
    }

check_arm_type0:
    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        intr_handler = soc_cmicm_intr_handlers0_arm_cmc;
        poll_limit = POLL_LIMIT;
        
        for (arm = 0; arm < (SOC_CMCS_NUM(unit)-1); arm++) {
            /* this enable processing of ARM_CMC0/ARM_CMC1 IRQ0 */
            cmc = SOC_ARM_CMC(unit, arm);
        
            for (;;) {
                irqStat = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT0_OFFSET(cmc));
                if (irqStat == 0) {
                    break; /* move to next arm cmc */
                }
                irqMask = SOC_CMCx_IRQ0_MASK(unit,cmc);
                irqStat &= irqMask;
                if (irqStat == 0) {
                    break; /* move to next arm cmc */
                }
                
                i = 0;
                for (; intr_handler[i].mask; i++) {
                    if (irqStat & intr_handler[i].mask) {
                        /* dispatch interrupt */
                        LOG_INFO(BSL_LS_SOC_INTR,
                                 (BSL_META_U(unit,
                                             "soc_cmicm_intr CMC %d type 0 unit %d: dispatch %s\n"),
                                  unit, cmc, intr_handler[i].intr_name));
                        
                        (*intr_handler[i].intr_fn)
                            (unit, ((cmc<<2) + intr_handler[i].intr_data));
                        
                        if (--poll_limit == 0) {
                            LOG_ERROR(BSL_LS_SOC_COMMON,
                                      (BSL_META_U(unit,
                                                  "soc_cmicm_intr unit %d: "
                                                  "ERROR can't clear type 3 interrupt(s): "
                                                  "IRQ=0x%x (disabling 0x%x)\n"),
                                       unit, irqStat, intr_handler[i].mask));
                            (void)soc_cmicm_cmcx_intr0_disable(unit, cmc, 
                                                               intr_handler[i].mask);
                            poll_limit = POLL_LIMIT;
                        }
                        break;
                    }
                }
            }
        }
    }
#else
    COMPILER_REFERENCE(arm);
#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DPP_SUPPORT) */

    if (soc_feature(unit, soc_feature_short_cmic_error)) {
        /* Using sal_dpc since there are schan reads in this function
         * and schan read cant be done from interrupt context.
         * the function soc_cmn_error will be excecuted only after this 
         * function will end.
         */
        sal_dpc(soc_cmn_error, INT_TO_PTR(unit), 0, 0, 0, 0);
    }

    cmc = SOC_PCI_CMC(unit);
    IRQ_MASK0_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(cmc), SOC_CMCx_IRQ0_MASK(unit, cmc));
#if defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DPP_SUPPORT)

    IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(cmc), SOC_CMCx_IRQ1_MASK(unit, cmc));
    IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc), SOC_CMCx_IRQ2_MASK(unit, cmc));
    if (soc_feature(unit, soc_feature_extended_cmic_error)) {
        IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc), SOC_CMCx_IRQ3_MASK(unit, cmc));
        IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(cmc), SOC_CMCx_IRQ4_MASK(unit, cmc));
    }
    if (soc_feature(unit, soc_feature_cmicd_v2)) {
        IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc), SOC_CMCx_IRQ3_MASK(unit, cmc));
    }
    if (soc_feature(unit, soc_feature_short_cmic_error)) {
        /* When working with this feature other interrupts handling in this reg are done from soc_cmn_error()  
         * only cmic parity error (bit 0) is handled by this function.
         */
        IRQ_MASKx_SET(unit, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc), SOC_CMCx_IRQ2_MASK(unit, cmc) & 0x1);
    }
 
    /* May need to restore the masks in ARM's CMCs as well */
    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        int arm;

        for (arm = 0; arm < (SOC_CMCS_NUM(unit)-1); arm++) {
            int arm_cmc = SOC_ARM_CMC(unit, arm);

            soc_pci_write(unit, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(arm_cmc), SOC_CMCx_IRQ0_MASK(unit, arm_cmc));
        }
    }
#endif

#ifdef SAL_SPL_LOCK_ON_IRQ
    sal_spl(s);
#endif
}


#ifdef INCLUDE_RCPU
STATIC void
soc_cmicm_rcpu_intr_miim_op(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    /* Clr Read & Write Stat */
    soc_pci_write(unit, CMIC_RPE_MIIM_CTRL_OFFSET, 0);

    soc->stat.intr_mii++;

    if (soc->miimIntr) {
        sal_sem_give(soc->miimIntr);
    }
}
/*
 * Enable (unmask) or disable (mask) a set of CMIC RPE interrupts.  These
 * routines should be used instead of manipulating CMIC_RPE_RCPU_IRQ_MASK
 * directly, since a read-modify-write is required.  The return value is
 * the previous mask (can pass mask of 0 to just get the current mask).
 */

STATIC uint32
soc_cmicm_rcpu_intrx_enable(int unit, uint32 offset, uint32 mask, uint32 *mask_reg)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = *mask_reg;
    *mask_reg |= mask;
    newMask = *mask_reg;
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    LOG_INFO(BSL_LS_SOC_INTR,
             (BSL_META_U(unit,
                         "soc_cmicm_rcpu_intrx_enable unit %d: mask 0x%8x\n"), unit, mask));

    soc_pci_write(unit, offset, newMask);

    sal_spl(s);

    return oldMask;
}

STATIC uint32
soc_cmicm_rcpu_intrx_disable(int unit, uint32 offset, uint32 mask, uint32 *mask_reg)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = *mask_reg;
    *mask_reg &= ~mask;
    newMask = *mask_reg;

    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    LOG_INFO(BSL_LS_SOC_INTR,
             (BSL_META_U(unit,
                         "soc_cmicm_rcpu_intrx_disable unit %d: mask 0x%8x\n"), unit, mask));
    soc_pci_write(unit, offset, newMask);

    sal_spl(s);

    return oldMask;
}

uint32
soc_cmicm_rcpu_intr0_enable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_enable(unit, CMIC_RPE_RCPU_IRQ_MASK0_OFFSET,
                                       mask, &SOC_RCPU_IRQ0_MASK(unit));
}

uint32
soc_cmicm_rcpu_intr0_disable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_disable(unit, CMIC_RPE_RCPU_IRQ_MASK0_OFFSET,
                                       mask, &SOC_RCPU_IRQ0_MASK(unit));
}

uint32
soc_cmicm_rcpu_intr1_enable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_enable(unit, CMIC_RPE_RCPU_IRQ_MASK1_OFFSET,
                                       mask, &SOC_RCPU_IRQ1_MASK(unit));
}

uint32
soc_cmicm_rcpu_intr1_disable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_disable(unit, CMIC_RPE_RCPU_IRQ_MASK1_OFFSET,
                                       mask, &SOC_RCPU_IRQ1_MASK(unit));
}

uint32
soc_cmicm_rcpu_intr2_enable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_enable(unit, CMIC_RPE_RCPU_IRQ_MASK2_OFFSET,
                                       mask, &SOC_RCPU_IRQ2_MASK(unit));
}

uint32
soc_cmicm_rcpu_intr2_disable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_disable(unit, CMIC_RPE_RCPU_IRQ_MASK2_OFFSET,
                                       mask, &SOC_RCPU_IRQ2_MASK(unit));
}

uint32
soc_cmicm_rcpu_intr3_enable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_enable(unit, CMIC_RPE_RCPU_IRQ_MASK3_OFFSET,
                                       mask, &SOC_RCPU_IRQ3_MASK(unit));
}

uint32
soc_cmicm_rcpu_intr3_disable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_disable(unit, CMIC_RPE_RCPU_IRQ_MASK3_OFFSET,
                                       mask, &SOC_RCPU_IRQ3_MASK(unit));
}

uint32
soc_cmicm_rcpu_intr4_enable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_enable(unit, CMIC_RPE_RCPU_IRQ_MASK4_OFFSET,
                                       mask, &SOC_RCPU_IRQ4_MASK(unit));
}

uint32
soc_cmicm_rcpu_intr4_disable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_disable(unit, CMIC_RPE_RCPU_IRQ_MASK4_OFFSET,
                                       mask, &SOC_RCPU_IRQ4_MASK(unit));
}

uint32
soc_cmicm_rcpu_cmc0_intr0_enable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_enable(unit, CMIC_CMC0_RCPU_IRQ_MASK0_OFFSET,
                                       mask, &SOC_RCPU_CMC0_IRQ0_MASK(unit));
}

uint32
soc_cmicm_rcpu_cmc0_intr0_disable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_disable(unit, CMIC_CMC0_RCPU_IRQ_MASK0_OFFSET,
                                       mask, &SOC_RCPU_CMC0_IRQ0_MASK(unit));
}

uint32
soc_cmicm_rcpu_cmc1_intr0_enable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_enable(unit, CMIC_CMC1_RCPU_IRQ_MASK0_OFFSET,
                                       mask, &SOC_RCPU_CMC1_IRQ0_MASK(unit));
}

uint32
soc_cmicm_rcpu_cmc1_intr0_disable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_disable(unit, CMIC_CMC1_RCPU_IRQ_MASK0_OFFSET,
                                       mask, &SOC_RCPU_CMC1_IRQ0_MASK(unit));
}

uint32
soc_cmicm_rcpu_cmc2_intr0_enable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_enable(unit, CMIC_CMC2_RCPU_IRQ_MASK0_OFFSET,
                                       mask, &SOC_RCPU_CMC2_IRQ0_MASK(unit));
}

uint32
soc_cmicm_rcpu_cmc2_intr0_disable(int unit, uint32 mask)
{
    return soc_cmicm_rcpu_intrx_disable(unit, CMIC_CMC2_RCPU_IRQ_MASK0_OFFSET,
                                       mask, &SOC_RCPU_CMC2_IRQ0_MASK(unit));
}

void
soc_cmicm_rcpu_intr(int unit, soc_rcpu_intr_packet_t *intr_pkt)
{
    uint32 irqStat, irqMask;
    int i = 0;
    intr_handler_t *intr_handler = soc_cmicm_rcpu_intr_handlers0;
    soc_control_t *soc;

#ifdef SAL_SPL_LOCK_ON_IRQ
    int s;

    s = sal_splhi();
#endif

    soc = SOC_CONTROL(unit);

    /*
     * Our handler is permanently registered in soc_probe().  If our
     * unit is not attached yet, it could not have generated this
     * interrupt.  The interrupt line must be shared by multiple PCI
     * cards.  Simply ignore the interrupt and let another handler
     * process it.
     */
    if (soc == NULL || (soc->soc_flags & SOC_F_BUSY) ||
        !(soc->soc_flags & SOC_F_ATTACHED)) {
#ifdef SAL_SPL_LOCK_ON_IRQ
        sal_spl(s);
#endif
        return;
    }

    soc->stat.intr++; /* Update count */

    /*
     * Read IRQ Status and IRQ Mask and AND to determine active ints.
     * These are re-read each time since either can be changed by ISRs.
     */
    
    irqStat = intr_pkt->rcpu_irq0_stat;
    if (irqStat == 0) {
        goto check_rcpu_type1;  /* No pending Interrupts */
    }
    irqMask = intr_pkt->rcpu_irq0_mask;
    irqStat &= irqMask;
    if (irqStat == 0) {
        goto check_rcpu_type1;
    }

    i = 0;

    for (; intr_handler[i].mask; i++) {
        if (irqStat & intr_handler[i].mask) {

            /* dispatch interrupt */
            if (LOG_CHECK(BSL_LS_SOC_RCPU | BSL_INFO) &&
                LOG_CHECK(BSL_LS_SOC_INTR | BSL_INFO)) {
                LOG_CLI((BSL_META_U(unit,
                                    "soc_cmicm_rcpu_intr type 0 unit %d: dispatch %s\n"),
                         unit, intr_handler[i].intr_name));
            }

            (*intr_handler[i].intr_fn)(unit, intr_handler[i].intr_data);
        }
    }

check_rcpu_type1:

#if defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
    irqStat = intr_pkt->rcpu_irq1_stat;
    if (irqStat == 0) {
        goto check_rcpu_type2;  /* No pending Interrupts */
    }
    irqMask = intr_pkt->rcpu_irq1_mask;
    irqStat &= irqMask;
    if (irqStat == 0) {
        goto check_rcpu_type2;
    }

    intr_handler = soc_cmicm_intr_handlers1;
    i = 0;

    for (; intr_handler[i].mask; i++) {
        if (irqStat & intr_handler[i].mask) {
    
            /* dispatch interrupt, verbose only */
            if (LOG_CHECK(BSL_LS_SOC_RCPU | BSL_INFO) &&
                LOG_CHECK(BSL_LS_SOC_INTR | BSL_INFO)) {
                LOG_CLI((BSL_META_U(unit,
                                    "soc_cmicm_rcpu_intr type 1 unit %d: dispatch %s\n"),
                         unit, intr_handler[i].intr_name));
            }
        }
    }

check_rcpu_type2:

    irqStat = intr_pkt->rcpu_irq2_stat;
    if (irqStat == 0) {
        goto check_rcpu_type3;  /* No pending Interrupts */
    }
    irqMask = intr_pkt->rcpu_irq2_mask;
    irqStat &= irqMask;
    if (irqStat == 0) {
        goto check_rcpu_type2;
    }

    intr_handler = soc_cmicm_intr_handlers2;
    i = 0;

    for (; intr_handler[i].mask; i++) {
        if (irqStat & intr_handler[i].mask) {
    
            /* dispatch interrupt, verbose only */
            if (LOG_CHECK(BSL_LS_SOC_RCPU | BSL_INFO) &&
                LOG_CHECK(BSL_LS_SOC_INTR | BSL_INFO)) {
                LOG_CLI((BSL_META_U(unit,
                                    "soc_cmicm_rcpu_intr type 2 unit %d: dispatch %s\n"),
                         unit, intr_handler[i].intr_name));
            }
        }
    }

check_rcpu_type3:

    irqStat = intr_pkt->rcpu_irq3_stat;
    if (irqStat == 0) {
        goto check_rcpu_type4;  /* No pending Interrupts */
    }
    irqMask = intr_pkt->rcpu_irq3_mask;
    irqStat &= irqMask;
    if (irqStat == 0) {
        goto check_rcpu_type4;
    }

    intr_handler = soc_cmicm_intr_handlers3;
    i = 0;

    for (; intr_handler[i].mask; i++) {
        if (irqStat & intr_handler[i].mask) {
    
            /* dispatch interrupt, verbose only */
            if (LOG_CHECK(BSL_LS_SOC_RCPU | BSL_INFO) &&
                LOG_CHECK(BSL_LS_SOC_INTR | BSL_INFO)) {
                LOG_CLI((BSL_META_U(unit,
                                    "soc_cmicm_rcpu_intr type 3 unit %d: dispatch %s\n"),
                         unit, intr_handler[i].intr_name));
            }
        }
    }

check_rcpu_type4:
    irqStat = intr_pkt->rcpu_irq4_stat;
    if (irqStat == 0) {
        goto check_rcpu_cmc0;  /* No pending Interrupts */
    }
    irqMask = intr_pkt->rcpu_irq4_mask;
    irqStat &= irqMask;
    if (irqStat == 0) {
        goto check_rcpu_cmc0;
    }

    intr_handler = soc_cmicm_intr_handlers4;
    i = 0;

    for (; intr_handler[i].mask; i++) {
        if (irqStat & intr_handler[i].mask) {
    
            /* dispatch interrupt, verbose only */
            if (LOG_CHECK(BSL_LS_SOC_RCPU | BSL_INFO) &&
                LOG_CHECK(BSL_LS_SOC_INTR | BSL_INFO)) {
                LOG_CLI((BSL_META_U(unit,
                                    "soc_cmicm_rcpu_intr type 4 unit %d: dispatch %s\n"),
                         unit, intr_handler[i].intr_name));
            }
        }
    }

check_rcpu_cmc0:

#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) */

    irqStat = intr_pkt->cmc0_irq_stat;
    if (irqStat == 0) {
        goto rcpu_intr_exit;  /* No pending Interrupts */
    }
    irqMask = intr_pkt->cmc0_rcpu_irq_mask;
    irqStat &= irqMask;
    if (irqStat == 0) {
        goto rcpu_intr_exit;
    }

    if (soc_feature(unit, soc_feature_sbusdma)) {
        intr_handler = soc_cmicm_intr_handlers0;
    } else {
        intr_handler = soc_cmicm_intr_handlers;
    }

    i = 0;

    for (; intr_handler[i].mask; i++) {
        if (irqStat & intr_handler[i].mask) {

            if (LOG_CHECK(BSL_LS_SOC_RCPU | BSL_INFO) &&
                LOG_CHECK(BSL_LS_SOC_INTR | BSL_INFO)) {
                /* dispatch interrupt, verbose only */
                LOG_CLI((BSL_META_U(unit,
                                    "soc_cmicm_rcpu_cmc0_intr type 0 unit %d: dispatch %s\n"),
                         unit, intr_handler[i].intr_name));
            }
        }
    }

rcpu_intr_exit:

#ifdef SAL_SPL_LOCK_ON_IRQ
    sal_spl(s);
#endif
    return;
}

#endif /* INCLUDE_RCPU */

/*
 * Initialize iProc based iHost irq offset
 */
void
soc_cmicm_ihost_irq_offset_set(int unit)
{
    soc_cmicm_host_irq_offset[unit] = HOST_IRQ_MASK_OFFSET_DIFF;
}

/*
 * Initialize iProc based iHost irq offset
 */
void
soc_cmicm_ihost_irq_offset_reset(int unit)
{
    soc_cmicm_host_irq_offset[unit] = 0;
}

#endif /* CMICM Support */
