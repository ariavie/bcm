/*
 * $Id: intr.c 1.87.2.1 Broadcom SDK $
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
 * SOC Interrupt Handlers
 *
 * NOTE: These handlers are called from an interrupt context, so their
 *       actions are restricted accordingly.
 */
#ifdef _ERR_MSG_MODULE_NAME
  #error "_ERR_MSG_MODULE_NAME redefined"
#endif
#define _ERR_MSG_MODULE_NAME SOC_DBG_INTR

#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>
#include <sal/core/sync.h>
#include <sal/core/dpc.h>
#include <sal/core/time.h>
#include <sal/types.h>

#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/dma.h>
#include <soc/i2c.h>
#include <soc/intr.h>

#ifdef BCM_FIREBOLT_SUPPORT
#include <soc/firebolt.h>
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_BRADLEY_SUPPORT
#include <soc/bradley.h>
#endif /* BCM_BRADLEY_SUPPORT */

#ifdef BCM_TRIUMPH_SUPPORT
#include <soc/triumph.h>
#endif /* BCM_TRIUMPH_SUPPORT */

#ifdef BCM_TRIUMPH2_SUPPORT
#include <soc/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
#include <soc/trident.h>
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_ENDURO_SUPPORT
#include <soc/enduro.h>
#endif /* BCM_ENDURO_SUPPORT */

#ifdef BCM_HURRICANE_SUPPORT
#include <soc/hurricane.h>
#endif /* BCM_HURRICANE_SUPPORT */

#ifdef BCM_SIRIUS_SUPPORT
#include <soc/sbx/sbx_drv.h>
#include <soc/sbx/sirius.h>
#endif

#ifdef BCM_DFE_SUPPORT
#include <soc/dfe/cmn/dfe_drv.h>
#include <soc/dfe/cmn/dfe_interrupt.h>
#include <soc/dfe/fe1600/fe1600_interrupts.h>
#endif
#ifdef BCM_ARAD_SUPPORT
#include <soc/dpp/ARAD/arad_interrupts.h>
#include <soc/dpp/ARAD/arad_sw_db.h>
#endif
#ifdef INCLUDE_KNET
#include <soc/knet.h>
#define IRQ_MASK_SET(_u,_a,_m) soc_knet_irq_mask_set(_u,_a,_m)
#else
#define IRQ_MASK_SET(_u,_a,_m) soc_pci_write(_u,_a,_m)
#endif

#define INTR_CMN_ERROR_MAX_INTERRUPTS_SIZE    50
#define SOC_INTERRUPT_DB_PRIORITY_MECHANISM_MAX_LEVELS 300
#define FIRST_PORT_BLOCK_INTER 727

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_DFE_SUPPORT)|| defined(BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)

/* Declare static functions for interrupt handler array */
STATIC void soc_intr_schan_done(int unit, uint32 ignored);
STATIC void soc_intr_pci_parity(int unit, uint32 ignored);
STATIC void soc_intr_pci_fatal(int unit, uint32 ignored);
STATIC void soc_intr_link_stat(int unit, uint32 ignored);
STATIC void soc_intr_gbp_full(int unit, uint32 ignored);
STATIC void soc_intr_arl_xfer(int unit, uint32 ignored);
STATIC void soc_intr_arl_cnt0(int unit, uint32 ignored);
STATIC void soc_intr_arl_drop(int unit, uint32 ignored);
STATIC void soc_intr_arl_mbuf(int unit, uint32 ignored);
STATIC void soc_intr_schan_error(int unit, uint32 ignored);
STATIC void soc_intr_i2c(int unit, uint32 ignored);
STATIC void soc_intr_miim_op(int unit, uint32 ignored);
STATIC void soc_intr_stat_dma(int unit, uint32 ignored);
STATIC void soc_intr_bit21(int unit, uint32 ignored);
STATIC void soc_intr_bit22(int unit, uint32 ignored);
STATIC void soc_intr_bit23(int unit, uint32 ignored);
#ifdef BCM_HERCULES_SUPPORT
STATIC void soc_intr_mmu_stat(int unit, uint32 ignored);
#endif
#if defined(BCM_XGS12_SWITCH_SUPPORT)
STATIC void soc_intr_arl_error(int unit, uint32 ignored);
#endif
STATIC void soc_intr_lpm_lo_parity(int unit, uint32 ignored);
STATIC void soc_intr_bit25(int unit, uint32 ignored);
STATIC void soc_intr_bit26(int unit, uint32 ignored);
STATIC void soc_intr_bit27(int unit, uint32 ignored);
STATIC void soc_intr_bit28(int unit, uint32 ignored);
STATIC void soc_intr_bit31(int unit, uint32 ignored);
STATIC void soc_intr_tdma_done(int unit, uint32 ignored);
STATIC void soc_intr_tslam_done(int unit, uint32 ignored);
STATIC void soc_intr_block(int unit, uint32 block);

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

STATIC intr_handler_t soc_intr_handlers[] = {

 /* Errors (Highest priority) [0..3] */

 { IRQ_PCI_PARITY_ERR,    soc_intr_pci_parity,    0, "PCI_PARITY_ERR"    },
 { IRQ_PCI_FATAL_ERR,    soc_intr_pci_fatal,    0, "PCI_FATAL_ERR"    },
 { IRQ_SCHAN_ERR,    soc_intr_schan_error,    0, "SCHAN_ERR"        },
 { IRQ_GBP_FULL,    soc_intr_gbp_full,    0, "GBP_FULL"        },

 /* S-Channel [4] */

 { IRQ_SCH_MSG_DONE,    soc_intr_schan_done,    0, "SCH_MSG_DONE"    },

 /* MII [5-6] */

 { IRQ_MIIM_OP_DONE,    soc_intr_miim_op,    0, "MIIM_OP_DONE"    },
 { IRQ_LINK_STAT_MOD,    soc_intr_link_stat,    0, "LINK_STAT_MOD"    },

 /* ARL messages [7-10] */

 { IRQ_ARL_MBUF,    soc_intr_arl_mbuf,    0, "ARL_MBUF"        },
 { IRQ_ARL_MBUF_DROP,    soc_intr_arl_drop,    0, "ARL_MBUF_DROP"    },
 { IRQ_ARL_DMA_CNT0,    soc_intr_arl_cnt0,    0, "ARL_DMA_CNT0"    },
 { IRQ_ARL_DMA_XFER,    soc_intr_arl_xfer,    0, "ARL_DMA_XFER"    },

 /* TDMA/TSLAM [11-12] */
 { IRQ_TDMA_DONE,    soc_intr_tdma_done,    0, "TDMA_DONE"            },
 { IRQ_TSLAM_DONE,    soc_intr_tslam_done,    0, "TSLAM_DONE"            },

 /* Packet DMA [13-20] */

 { IRQ_CHAIN_DONE(0),    soc_dma_done_chain,    0, "CH0_CHAIN_DONE"    },
 { IRQ_CHAIN_DONE(1),    soc_dma_done_chain,    1, "CH1_CHAIN_DONE"    },
 { IRQ_CHAIN_DONE(2),    soc_dma_done_chain,    2, "CH2_CHAIN_DONE"    },
 { IRQ_CHAIN_DONE(3),    soc_dma_done_chain,    3, "CH3_CHAIN_DONE"    },

 { IRQ_DESC_DONE(0),    soc_dma_done_desc,    0, "CH0_DESC_DONE"    },
 { IRQ_DESC_DONE(1),    soc_dma_done_desc,    1, "CH1_DESC_DONE"    },
 { IRQ_DESC_DONE(2),    soc_dma_done_desc,    2, "CH2_DESC_DONE"    },
 { IRQ_DESC_DONE(3),    soc_dma_done_desc,    3, "CH3_DESC_DONE"    },

 /* Other (lowest priority) [21-28] */

 { IRQ_BIT21,        soc_intr_bit21,        0, "MMU_IRQ_STAT"    },
 { IRQ_BIT22,        soc_intr_bit22,        0, "IRQ_FIFO_CH1_DMA"    },
 { IRQ_BIT23,        soc_intr_bit23,        0, "IRQ_FIFO_CH2_DMA"    },
 { IRQ_STAT_ITER_DONE,    soc_intr_stat_dma,    0, "STAT_ITER_DONE"    },
 { IRQ_I2C_INTR,    soc_intr_i2c,        0, "I2C_INTR"        },
 { IRQ_ARL_LPM_LO_PAR,    soc_intr_lpm_lo_parity,    0, "LPM_LO_PARITY"    },
 { IRQ_BIT25,           soc_intr_bit25,            0, "LPM_HI_PARITY/BSE"    },
 { IRQ_BIT26,            soc_intr_bit26,            0, "L3_PARITY/CSE"      },
 { IRQ_BIT27,            soc_intr_bit27,            0, "L2_PARITY/HSE"      },
 { IRQ_BIT28,           soc_intr_bit28,         0, "VLAN_PARITY/MEMFAIL"},
 { IRQ_BROADSYNC_INTR,  soc_intr_bit31,         0, "BSAFE_OP_DONE/BROADSYNC_INTR"},

};

#define INTR_HANDLERS_COUNT    COUNTOF(soc_intr_handlers)

/*
 * define some short cuts to start processing interrupts quickly
 * start2: skip to packet processing
 * start1: skip low probability errors
 * else start at 0
 */
#define    INTR_START1_MASK    (IRQ_PCI_PARITY_ERR | \
                IRQ_PCI_FATAL_ERR | \
                IRQ_SCHAN_ERR | \
                IRQ_GBP_FULL)
#define    INTR_START1_POS        4
#define    INTR_START2_MASK    (INTR_START1_MASK | \
                IRQ_SCH_MSG_DONE | \
                IRQ_MIIM_OP_DONE | \
                IRQ_LINK_STAT_MOD | \
                IRQ_ARL_MBUF | \
                IRQ_ARL_MBUF_DROP | \
                IRQ_ARL_DMA_CNT0 | \
                IRQ_ARL_DMA_XFER)
#define    INTR_START2_POS        11

STATIC intr_handler_t soc_intr_block_lo_handlers[] = {
 { IRQ_BLOCK(0),    soc_intr_block,    0,  "BLOCK_0_ERR"    },
 { IRQ_BLOCK(1),    soc_intr_block,    1,  "BLOCK_1_ERR"    },
 { IRQ_BLOCK(2),    soc_intr_block,    2,  "BLOCK_2_ERR"    },
 { IRQ_BLOCK(3),    soc_intr_block,    3,  "BLOCK_3_ERR"    },
 { IRQ_BLOCK(4),    soc_intr_block,    4,  "BLOCK_4_ERR"    },
 { IRQ_BLOCK(5),    soc_intr_block,    5,  "BLOCK_5_ERR"    },
 { IRQ_BLOCK(6),    soc_intr_block,    6,  "BLOCK_6_ERR"    },
 { IRQ_BLOCK(7),    soc_intr_block,    7,  "BLOCK_7_ERR"    },
 { IRQ_BLOCK(8),    soc_intr_block,    8,  "BLOCK_8_ERR"    },
 { IRQ_BLOCK(9),    soc_intr_block,    9,  "BLOCK_9_ERR"    },
 { IRQ_BLOCK(10),    soc_intr_block,    10, "BLOCK_10_ERR"    },
 { IRQ_BLOCK(11),    soc_intr_block,    11, "BLOCK_11_ERR"    },
 { IRQ_BLOCK(12),    soc_intr_block,    12, "BLOCK_12_ERR"    },
 { IRQ_BLOCK(13),    soc_intr_block,    13, "BLOCK_13_ERR"    },
 { IRQ_BLOCK(14),    soc_intr_block,    14, "BLOCK_14_ERR"    },
 { IRQ_BLOCK(15),    soc_intr_block,    15, "BLOCK_15_ERR"    },
 { IRQ_BLOCK(16),    soc_intr_block,    16, "BLOCK_16_ERR"    },
 { IRQ_BLOCK(17),    soc_intr_block,    17, "BLOCK_17_ERR"    },
 { IRQ_BLOCK(18),    soc_intr_block,    18, "BLOCK_18_ERR"    },
 { IRQ_BLOCK(19),    soc_intr_block,    19, "BLOCK_19_ERR"    },
 { IRQ_BLOCK(20),    soc_intr_block,    20, "BLOCK_20_ERR"    },
 { IRQ_BLOCK(21),    soc_intr_block,    21, "BLOCK_21_ERR"    },
 { IRQ_BLOCK(22),    soc_intr_block,    22, "BLOCK_22_ERR"    },
 { IRQ_BLOCK(23),    soc_intr_block,    23, "BLOCK_23_ERR"    },
 { IRQ_BLOCK(24),    soc_intr_block,    24, "BLOCK_24_ERR"    },
 { IRQ_BLOCK(25),    soc_intr_block,    25, "BLOCK_25_ERR"    },
 { IRQ_BLOCK(26),    soc_intr_block,    26, "BLOCK_26_ERR"    },
 { IRQ_BLOCK(27),    soc_intr_block,    27, "BLOCK_27_ERR"    },
 { IRQ_BLOCK(28),    soc_intr_block,    28, "BLOCK_28_ERR"    },
 { IRQ_BLOCK(29),    soc_intr_block,    29, "BLOCK_29_ERR"    },
 { IRQ_BLOCK(30),    soc_intr_block,    30, "BLOCK_30_ERR"    },
 { IRQ_BLOCK(31),    soc_intr_block,    31, "BLOCK_31_ERR"    },
};
STATIC intr_handler_t soc_intr_block_hi_handlers[] = {
 { IRQ_BLOCK(0),    soc_intr_block,    32, "BLOCK_32_ERR"    },
 { IRQ_BLOCK(1),    soc_intr_block,    33, "BLOCK_33_ERR"    },
 { IRQ_BLOCK(2),    soc_intr_block,    34, "BLOCK_34_ERR"    },
 { IRQ_BLOCK(3),    soc_intr_block,    35, "BLOCK_35_ERR"    },
 { IRQ_BLOCK(4),    soc_intr_block,    36, "BLOCK_36_ERR"    },
 { IRQ_BLOCK(5),    soc_intr_block,    37, "BLOCK_37_ERR"    },
 { IRQ_BLOCK(6),    soc_intr_block,    38, "BLOCK_38_ERR"    },
 { IRQ_BLOCK(7),    soc_intr_block,    39, "BLOCK_39_ERR"    },
 { IRQ_BLOCK(8),    soc_intr_block,    40, "BLOCK_40_ERR"    },
 { IRQ_BLOCK(9),    soc_intr_block,    41, "BLOCK_41_ERR"    },
 { IRQ_BLOCK(10),    soc_intr_block,    42, "BLOCK_42_ERR"    },
 { IRQ_BLOCK(11),    soc_intr_block,    43, "BLOCK_43_ERR"    },
 { IRQ_BLOCK(12),    soc_intr_block,    44, "BLOCK_44_ERR"    },
 { IRQ_BLOCK(13),    soc_intr_block,    45, "BLOCK_45_ERR"    },
 { IRQ_BLOCK(14),    soc_intr_block,    46, "BLOCK_46_ERR"    },
 { IRQ_BLOCK(15),    soc_intr_block,    47, "BLOCK_47_ERR"    },
 { IRQ_BLOCK(16),    soc_intr_block,    48, "BLOCK_48_ERR"    },
 { IRQ_BLOCK(17),    soc_intr_block,    49, "BLOCK_49_ERR"    },
 { IRQ_BLOCK(18),    soc_intr_block,    50, "BLOCK_50_ERR"    },
 { IRQ_BLOCK(19),    soc_intr_block,    51, "BLOCK_51_ERR"    },
 { IRQ_BLOCK(20),    soc_intr_block,    52, "BLOCK_52_ERR"    },
 { IRQ_BLOCK(21),    soc_intr_block,    53, "BLOCK_53_ERR"    },
 { IRQ_BLOCK(22),    soc_intr_block,    54, "BLOCK_54_ERR"    },
 { IRQ_BLOCK(23),    soc_intr_block,    55, "BLOCK_55_ERR"    },
 { IRQ_BLOCK(24),    soc_intr_block,    56, "BLOCK_56_ERR"    },
 { IRQ_BLOCK(25),    soc_intr_block,    57, "BLOCK_57_ERR"    },
 { IRQ_BLOCK(26),    soc_intr_block,    58, "BLOCK_58_ERR"    },
 { IRQ_BLOCK(27),    soc_intr_block,    59, "BLOCK_59_ERR"    },
 { IRQ_BLOCK(28),    soc_intr_block,    60, "BLOCK_60_ERR"    },
 { IRQ_BLOCK(29),    soc_intr_block,    61, "BLOCK_61_ERR"    },
 { IRQ_BLOCK(30),    soc_intr_block,    62, "BLOCK_62_ERR"    },
 { IRQ_BLOCK(31),    soc_intr_block,    63, "BLOCK_63_ERR"    },
};
#define INTR_BLOCK_LO_HANDLERS_COUNT    COUNTOF(soc_intr_block_lo_handlers)
#define INTR_BLOCK_HI_HANDLERS_COUNT    COUNTOF(soc_intr_block_hi_handlers)

#define SOC_CMIC_BLK_CLP_0_INDX 24
#define SOC_CMIC_BLK_CLP_1_INDX 25
#define SOC_CMIC_BLK_XLP_0_INDX 27
#define SOC_CMIC_BLK_XLP_1_INDX 28

#define _PORT_BLOCK_FROM_IRQ_STATE2(unit, cmic_irq_state_2, stat2_field, block_bit)   \
        ( (soc_reg_field_get(unit, CMIC_CMC0_IRQ_STAT2r, (cmic_irq_state_2), stat2_field) != 0 ) << block_bit ) 

/*
 * Interrupt handler functions
 */

STATIC void
soc_intr_schan_done(int unit, uint32 ignored)
{
    soc_control_t    *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    /* Record the schan control regsiter */
    soc->schan_result = soc_pci_read(unit, CMIC_SCHAN_CTRL);

    soc->stat.intr_sc++;

    soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MSG_DONE_CLR);

    if (soc->schanIntr) {
    sal_sem_give(soc->schanIntr);
    }
}

STATIC soc_schan_err_t
soc_schan_error_type(int unit, int err_code)
{
    int            bitcount = 0;
    soc_schan_err_t    err = SOC_SCERR_INVALID;

    switch (SOC_CHIP_GROUP(unit)) {
    case SOC_CHIP_BCM5673:
    case SOC_CHIP_BCM5674:
        if (err_code & 0x10) {
            err = SOC_SCERR_MMU_NPKT_CELLS;
            ++bitcount;
        }
    if (err_code & 0x20) {
            err = SOC_SCERR_MEMORY_PARITY;
            ++bitcount;
        }
        /* Fall through */
    case SOC_CHIP_BCM5690:
    case SOC_CHIP_BCM5695:
        if (err_code & 0x1) {
            err = SOC_SCERR_CFAP_OVER_UNDER;
            ++bitcount;
        }
    if (err_code & 0x2) {
            err = SOC_SCERR_MMU_SOFT_RST;
            ++bitcount;
        }
    if (err_code & 0x4) {
            err = SOC_SCERR_CBP_CELL_CRC;
            ++bitcount;
        }
    if (err_code & 0x8) {
            err = SOC_SCERR_CBP_HEADER_PARITY;
            ++bitcount;
        }
        break;
    case SOC_CHIP_BCM5665:
    case SOC_CHIP_BCM5650:
        if (err_code & 0x1) {
            err = SOC_SCERR_CELL_PTR_CRC;
            ++bitcount;
        }
    if (err_code & 0x2) {
            err = SOC_SCERR_CELL_DATA_CRC;
            ++bitcount;
        }
    if (err_code & 0x4) {
            err = SOC_SCERR_FRAME_DATA_CRC;
            ++bitcount;
        }
    if (err_code & 0x8) {
            err = SOC_SCERR_CELL_PTR_BLOCK_CRC;
            ++bitcount;
        }
    if (err_code & 0x10) {
            err = SOC_SCERR_MEMORY_PARITY;
            ++bitcount;
        }
    if (err_code & 0x20) {
            err = SOC_SCERR_PLL_DLL_LOCK_LOSS;
            ++bitcount;
        }
        break;
    default:
        break;
    }

    if (bitcount > 1) {
        err = SOC_SCERR_MULTIPLE_ERR;
    }

    return err;
}

STATIC void
_soc_sch_error_unblock(void *p_unit, void *p2, void *p3, void *p4, void *p5)
{
    COMPILER_REFERENCE(p2);
    COMPILER_REFERENCE(p3);
    COMPILER_REFERENCE(p4);
    COMPILER_REFERENCE(p5);

    soc_intr_enable(PTR_TO_INT(p_unit), IRQ_SCHAN_ERR);
}

STATIC void
soc_intr_schan_error(int unit, uint32 ignored)
{
    soc_control_t    *soc = SOC_CONTROL(unit);
    uint32        scerr, slot;
    int            vld, src, dst, opc, err;

    COMPILER_REFERENCE(ignored);

    /*
     * Read the beginning of the S-chan message so its contents are
     * visible when a PCI bus analyzer is connected.
     */

    soc_pci_analyzer_trigger(unit);

    if (soc_cm_debug_check(DK_INTR)) {
        slot = soc_pci_read(unit, 0);
        slot = soc_pci_read(unit, 4);
        slot = soc_pci_read(unit, 8);
        slot = soc_pci_read(unit, 0xC);
    }

    scerr = soc_pci_read(unit, CMIC_SCHAN_ERR);    /* Clears intr */
    soc_pci_write(unit, CMIC_SCHAN_ERR, 0); /* Clears intr in some devs */

    soc->stat.intr_sce++;

    /*
     * If the valid bit is not set, it's probably because the error
     * occurred at the same time the software was starting an unrelated
     * S-channel operation.  There is no way to prevent this conflict.
     * We'll indicate that that the valid bit was not set and continue,
     * since the error is probably still latched.
     */

    vld = soc_reg_field_get(unit, CMIC_SCHAN_ERRr, scerr,
           (SOC_IS_XGS3_SWITCH(unit) || SOC_IS_SIRIUS(unit)) ? ERRBITf : VALIDf);
    src = soc_reg_field_get(unit, CMIC_SCHAN_ERRr, scerr, SRC_PORTf);
    dst = soc_reg_field_get(unit, CMIC_SCHAN_ERRr, scerr, DST_PORTf);
    opc = soc_reg_field_get(unit, CMIC_SCHAN_ERRr, scerr, OP_CODEf);
    err = soc_reg_field_get(unit, CMIC_SCHAN_ERRr, scerr, ERR_CODEf);

    /*
     * Output message in two pieces because on vxWorks, it goes through
     * logMsg which only supports up to 6 arguments (sigh).
     */

    if ((!soc->mmu_error_block) || (opc != MEMORY_FAIL_NOTIFY)) {
        soc_cm_debug(DK_ERR,
                     "UNIT %d SCHAN ERROR: V/E=%d SRC=%d DST=%d ",
                     unit, vld, src, dst);

        soc_cm_debug(DK_ERR,
                     "OPCODE=%d(%s) ERRCODE=0x%x\n",
                     opc, soc_schan_op_name(opc), err);
    }

#ifdef BCM_XGS3_SWITCH_SUPPORT
        if (SOC_IS_XGS3_SWITCH(unit)) {
            soc_cm_debug(DK_ERR,
                    "UNIT %d SCHAN ERROR: Unknown reason\n",
                    unit);
    } else
#endif
#ifdef BCM_SIRIUS_SUPPORT
        if (SOC_IS_SIRIUS(unit)) {
            soc_cm_debug(DK_ERR,
                    "UNIT %d SCHAN ERROR: Unknown reason\n",
                    unit);
    } else
#endif
    if (opc == MEMORY_FAIL_NOTIFY) {
    switch (soc_schan_error_type(unit, err)) {
    case SOC_SCERR_CFAP_OVER_UNDER:
        /*
         * The CFAP is empty but a request for a cell pointer came
         * in, or the CFAP is full but a request to return a cell
         * pointer came in.
         */
        soc->stat.err_cfap++;
        soc_cm_debug(DK_ERR, 
             "UNIT %d SCHAN ERROR: CFAP oversubscribed\n",
             unit);
        break;
    case SOC_SCERR_SDRAM_CHKSUM:
        /*
         * Checksum error occurred when fetching a slot from SDRAM.
         */
        soc->stat.err_sdram++;
        slot = soc_pci_read(unit, CMIC_MEM_FAIL);
        soc_cm_debug(DK_ERR, 
             "UNIT %d SCHAN ERROR: SDRAM checksum error, "
             "slot=0x%x (GBP index 0x%x)\n",
             unit, slot, slot * 0x40);
        break;
    case SOC_SCERR_UNEXP_FIRST_CELL:
        /*
         * Unexpected first cell
         */
        soc->stat.err_fcell++;
        soc_cm_debug(DK_ERR, 
             "UNIT %d SCHAN ERROR: Unexpected first cell\n",
             unit);
        break;
    case SOC_SCERR_MMU_SOFT_RST:
        /*
         * MMU soft reset: received a second start cell without
         * receiving and end cell for the previous packet.
         */
        soc->stat.err_sr++;
        soc_cm_debug(DK_ERR,
             "UNIT %d SCHAN ERROR: MMU soft reset\n",
             unit);
        break;
    case SOC_SCERR_CBP_CELL_CRC:
        soc->stat.err_cellcrc++;
        soc_cm_debug(DK_ERR,
             "UNIT %d SCHAN ERROR: CBP Cell CRC error\n",
             unit);
        break;
    case SOC_SCERR_CBP_HEADER_PARITY:
        soc->stat.err_cbphp++;
        soc_cm_debug(DK_ERR,
             "UNIT %d SCHAN ERROR: CBP Header parity error\n",
             unit);
        break;
    case SOC_SCERR_MMU_NPKT_CELLS:
        soc->stat.err_npcell++;
        soc_cm_debug(DK_ERR,
             "UNIT %d SCHAN ERROR: "
             "MMU sent cells not in packet\n",
             unit);
        break;
    case SOC_SCERR_MEMORY_PARITY:
        soc->stat.err_mp++;
        break;
    case SOC_SCERR_CELL_PTR_CRC:
        soc->stat.err_cpcrc++;
            soc_cm_debug(DK_ERR,
                         "UNIT %d SCHAN ERROR: Cell data CRC error\n",
                         unit);
        break;
    case SOC_SCERR_CELL_DATA_CRC:
        soc->stat.err_cdcrc++;
            soc_cm_debug(DK_ERR,
                         "UNIT %d SCHAN ERROR: Cell data CRC error\n",
                         unit);
        break;
    case SOC_SCERR_FRAME_DATA_CRC:
        soc->stat.err_fdcrc++;
            soc_cm_debug(DK_ERR,
                         "UNIT %d SCHAN ERROR: Frame data CRC error\n",
                         unit);
        break;
    case SOC_SCERR_CELL_PTR_BLOCK_CRC:
        soc->stat.err_cpbcrc++;
            soc_cm_debug(DK_ERR,
                         "UNIT %d SCHAN ERROR: "
                         "Cell pointer block CRC error\n",
                         unit);
        break;
    case SOC_SCERR_PLL_DLL_LOCK_LOSS:
        soc->stat.err_pdlock++;
        break;
    case SOC_SCERR_MULTIPLE_ERR:
        soc->stat.err_multi++;
        soc_cm_debug(DK_ERR,
             "UNIT %d SCHAN ERROR: Multiple errors: 0x%x\n",
             unit, err);
        break;
    case SOC_SCERR_INVALID:
        soc->stat.err_invalid++;
        soc_cm_debug(DK_ERR,
             "UNIT %d SCHAN ERROR: Unknown memory error\n",
             unit);
        break;
        default:
            assert(0);
            break;
    }
    }

    if (soc->schanIntrBlk != 0) {
    soc_intr_disable(unit, IRQ_SCHAN_ERR);

    sal_dpc_time(soc->schanIntrBlk, _soc_sch_error_unblock,
             INT_TO_PTR(unit), 0, 0, 0, 0);
    }
}

STATIC void
soc_intr_arl_mbuf(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

#if defined(BCM_TRX_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (SOC_IS_TRX(unit) || SOC_IS_SIRIUS(unit)) {
        /* IRQ_CHIP_FUNC_0 */
        soc_intr_disable(unit, IRQ_CHIP_FUNC_0);
        soc->stat.intr_chip_func[0]++;
        return;
    }
#endif

    /*
     * Disable the interrupt; it is re-enabled by the ARL thread after
     * it processes the messages.
     */

    soc_intr_disable(unit, IRQ_ARL_MBUF);

    soc->stat.intr_arl_m++;

    if (soc->arl_notify) {
        soc->arl_mbuf_done = 1;
    if (!soc->arl_notified) {
        soc->arl_notified = 1;
        sal_sem_give(soc->arl_notify);
    }
    }
}

STATIC void
soc_intr_arl_drop(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

#if defined(BCM_TRX_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (SOC_IS_TRX(unit) || SOC_IS_SIRIUS(unit)) {
        /* IRQ_CHIP_FUNC_1 */
        soc_intr_disable(unit, IRQ_CHIP_FUNC_1);
        soc->stat.intr_chip_func[1]++;
        return;
    }
#endif

    soc_pci_analyzer_trigger(unit);

    soc_intr_disable(unit, IRQ_ARL_MBUF_DROP);

    soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_ARL_MSG_DROPPED_CLR);

    soc->stat.intr_arl_d++;

    if (soc->arl_notify) {
        soc->arl_msg_drop = 1;
    if (!soc->arl_notified) {
        soc->arl_notified = 1;
        sal_sem_give(soc->arl_notify);
    }
    }
}

STATIC void
soc_intr_arl_cnt0(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

#if defined(BCM_TRX_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (SOC_IS_TRX(unit) || SOC_IS_SIRIUS(unit)) {
        /* IRQ_CHIP_FUNC_4 */
        soc_intr_disable(unit, IRQ_CHIP_FUNC_4);
#if defined(BCM_TRIUMPH_SUPPORT)
        if (SOC_IS_TRIUMPH(unit)) {
            sal_dpc(soc_triumph_esm_intr_status, INT_TO_PTR(unit),
                    0, 0, 0, 0);
        }
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT)
        if (SOC_IS_TRIUMPH2(unit)) {
            sal_dpc(soc_triumph2_esm_intr_status, INT_TO_PTR(unit),
                    0, 0, 0, 0);
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
        soc->stat.intr_chip_func[4]++;
        return;
    }
#endif

    soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_ARL_DMA_EN_CLR);
    soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_ARL_DMA_DONE_CLR);

    soc->stat.intr_arl_0++;

    if (soc->arl_notify) {
    soc->arl_dma_cnt0 = 1;
    if (!soc->arl_notified) {
        soc->arl_notified = 1;
        sal_sem_give(soc->arl_notify);
    }
    }
}

STATIC void
soc_intr_arl_xfer(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

#if defined(BCM_TRX_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (SOC_IS_TRX(unit) || SOC_IS_SIRIUS(unit)) {
        /* IRQ_CHIP_FUNC_3 */
        soc_intr_disable(unit, IRQ_CHIP_FUNC_3);
#if defined(BCM_TRIUMPH_SUPPORT)
        if (SOC_IS_TRIUMPH(unit)) {
            sal_dpc(soc_triumph_esm_intr_status, INT_TO_PTR(unit),
                    0, 0, 0, 0);
        }
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT)
        if (SOC_IS_TRIUMPH2(unit)) {
            sal_dpc(soc_triumph2_esm_intr_status, INT_TO_PTR(unit),
                    0, 0, 0, 0);
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
        soc->stat.intr_chip_func[3]++;
        return;
    }
#endif

    soc_intr_disable(unit, IRQ_ARL_DMA_XFER);

    soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_ARL_DMA_XFER_DONE_CLR);

    soc->stat.intr_arl_x++;

    if (soc->arl_notify) {
    soc->arl_dma_xfer = 1;
    if (!soc->arl_notified) {
        soc->arl_notified = 1;
        sal_sem_give(soc->arl_notify);
    }
    }
}

STATIC void
soc_intr_tdma_done(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    soc_intr_disable(unit, IRQ_TDMA_DONE);

    soc->stat.intr_tdma++;

    if (soc->tableDmaIntr) {
        sal_sem_give(soc->tableDmaIntr);
    }
}

STATIC void
soc_intr_tslam_done(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    soc_intr_disable(unit, IRQ_TSLAM_DONE);

    soc->stat.intr_tslam++;

    if (soc->tslamDmaIntr) {
        sal_sem_give(soc->tslamDmaIntr);
    }
}

STATIC void
soc_intr_gbp_full(int unit, uint32 ignored)
{
    soc_control_t    *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

#if defined(BCM_TRX_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (SOC_IS_TRX(unit) || SOC_IS_SIRIUS(unit)) {
        /* IRQ_CHIP_FUNC_2 */
        soc_intr_disable(unit, IRQ_CHIP_FUNC_2);
        soc->stat.intr_chip_func[2]++;
        return;
    }
#endif

    soc->stat.intr_gbp++;

    soc_pci_analyzer_trigger(unit);

    /*
     * It doesn't make sense to "clear" this interrupt, so we disable
     * the interrupt in the mask register and re-enable it some time
     * later using a deferred procedure call.
     */

    soc_intr_disable(unit, IRQ_GBP_FULL);

#ifdef BCM_GBP_SUPPORT
    sal_dpc(_soc_gbp_full_block, INT_TO_PTR(unit), 0, 0, 0, 0);
#endif
}

STATIC void
soc_intr_link_stat(int unit, uint32 ignored)
{
    soc_control_t    *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    soc_pci_analyzer_trigger(unit);

    soc->stat.intr_ls++;

    /* Clear interrupt */

    soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_LINK_STAT_MSG_CLR);

    /* Perform user callout, if one is registered */

    if (soc->soc_link_callout != NULL) {
    (*soc->soc_link_callout)(unit);
    }
}

/*
 * PCI Parity and Fatal Error Reporting
 *
 *    If the interrupt routine prints a message on each error,
 *    the console can be frozen or VxWorks workq overflow can occur.
 *
 *    For this reason errors are counted for a period of time and
 *    reported together at a maximum rate.
 */

#define PCI_REPORT_TYPE_PARITY        1
#define PCI_REPORT_TYPE_FATAL        2
#define PCI_REPORT_PERIOD        (SECOND_USEC / 4)

STATIC char *_soc_pci_dma_types[] = {
    "DMA CH0",
    "DMA CH1",
    "DMA CH2",
    "DMA CH3"
};

STATIC char *_soc_pci_extended_dma_types[] = {
    "Status write for TX and RX DMA CH0",     /* 0 */
    "Table DMA",                              /* 1 */
    "Memory write for RX DMA CH0",            /* 2 */
    "Stats DMA",                              /* 3 */
    "Status write for TX and RX DMA CH1",     /* 4 */
    "Unknown",                                /* 5 */
    "Memory write for RX DMA CH1",            /* 6 */
    "Unknown",                                /* 7 */
    "Status write for TX and RX DMA CH2",     /* 8 */
    "Unknown",                                /* 9 */
    "Memory write for RX DMA CH2",            /* 10 */
    "Unknown",                                /* 11 */
    "Status write for TX and RX DMA CH3",     /* 12 */
    "Unknown",                                /* 13 */
    "Memory write for RX DMA CH3",            /* 14 */
    "Unknown",                                /* 15 */
    "Descriptor read for TX and RX DMA CH0",  /* 16 */
    "SLAM DMA",                               /* 17 */
    "Memory read for TX DMA CH0",             /* 18 */
    "Unknown",                                /* 19 */
    "Descriptor read for TX and RX DMA CH1",  /* 20 */
    "Unknown",                                /* 21 */
    "Memory read for TX DMA CH1",             /* 22 */
    "Unknown",                                /* 23 */
    "Descriptor read for TX and RX DMA CH2",  /* 24 */
    "Unknown",                                /* 25 */
    "Memory read for TX DMA CH2",             /* 26 */
    "Unknown",                                /* 27 */
    "Descriptor read for TX and RX DMA CH3",  /* 28 */
    "Unknown",                                /* 29 */
    "Memory read for TX DMA CH3",             /* 30 */
    "Unknown"                                 /* 31 */
};

STATIC char *_soc_pci_extended_trx_dma_types[] = {
    "Table DMA",                                /* 0 */
    "Stats DMA",                                /* 1 */
    "Memory write for RX DMA CH0",              /* 2 */
    "Memory write for RX DMA CH1",              /* 3 */
    "Memory write for RX DMA CH2",              /* 4 */
    "Memory write for RX DMA CH3",              /* 5 */
    "Status write for TX and RX DMA CH0",       /* 6 */
    "Status write for TX and RX DMA CH1",       /* 7 */
    "Status write for TX and RX DMA CH2",       /* 8 */
    "Status write for TX and RX DMA CH3",       /* 9 */
    "SLAM DMA",                                 /* 10 */
    "Memory read for TX DMA CH0",               /* 11 */
    "Memory read for TX DMA CH1",               /* 12 */
    "Memory read for TX DMA CH2",               /* 13 */
    "Memory read for TX DMA CH3",               /* 14 */
    "Descriptor read for TX and RX DMA CH0",    /* 15 */
    "Descriptor read for TX and RX DMA CH1",    /* 16 */
    "Descriptor read for TX and RX DMA CH2",    /* 17 */
    "Descriptor read for TX and RX DMA CH3",    /* 18 */
    "FIFO DMA CH0",                             /* 19 */
    "FIFO DMA CH1",                             /* 20 */
    "FIFO DMA CH2",                             /* 21 */
    "FIFO DMA CH3",                             /* 22 */
    "Unknown",                                  /* 23 */
    "Unknown",                                  /* 24 */
    "Unknown",                                  /* 25 */
    "Unknown",                                  /* 26 */
    "Unknown",                                  /* 27 */
    "Unknown",                                  /* 28 */
    "Unknown",                                  /* 29 */
    "Unknown",                                  /* 30 */
    "Unknown"                                   /* 31 */
};

STATIC void
_soc_pci_report_error(void *p_unit, void *stat, void *type,
              void *errcnt_dpc, void *p5)
{
    int             unit = PTR_TO_INT(p_unit);
    soc_control_t    *soc = SOC_CONTROL(unit);
    uint32        errcnt_cur = 0, dmatype_code = 0;
    char        *errtype = NULL, *dmatype = NULL;

    COMPILER_REFERENCE(p5);

    switch (PTR_TO_INT(type)) {
    case PCI_REPORT_TYPE_PARITY:
    soc->pciParityDPC = 0;
    errcnt_cur = soc->stat.intr_pci_pe;
    errtype = "Parity";
        if (soc_feature(unit, soc_feature_extended_pci_error)) {
            dmatype_code = DS_EXT_PCI_PARITY_ERR(PTR_TO_INT(stat));
        } else {
            dmatype_code = DS_PCI_PARITY_ERR(PTR_TO_INT(stat));
        }
    break;
    case PCI_REPORT_TYPE_FATAL:
    soc->pciFatalDPC = 0;
    errcnt_cur = soc->stat.intr_pci_fe;
    errtype = "Fatal";
        if (soc_feature(unit, soc_feature_extended_pci_error)) {
            dmatype_code = DS_EXT_PCI_FATAL_ERR(PTR_TO_INT(stat));
        } else {
            dmatype_code = DS_PCI_FATAL_ERR(PTR_TO_INT(stat));
        }
    break;
    }

    if (soc_feature(unit, soc_feature_extended_pci_error)) {
        if (SOC_IS_TRX(unit)) {
            dmatype =
                _soc_pci_extended_trx_dma_types[dmatype_code];
        } else {
            dmatype = _soc_pci_extended_dma_types[dmatype_code];
        }
    } else {
        dmatype = _soc_pci_dma_types[dmatype_code];
    }

    if (errcnt_cur == PTR_TO_INT(errcnt_dpc) + 1) {
    soc_cm_debug(DK_ERR,
             "UNIT %d ERROR interrupt: "
                     "CMIC_DMA_STAT = 0x%08x "
             "PCI %s Error on %s\n",
             unit,
                     PTR_TO_INT(stat),
             errtype, dmatype);
    } else {
    soc_cm_debug(DK_ERR,
             "UNIT %d ERROR interrupt: "
             "%d PCI %s Errors on %s\n",
             unit, errcnt_cur - PTR_TO_INT(errcnt_dpc),
             errtype, dmatype);
    }
}

STATIC void
soc_intr_pci_parity(int unit, uint32 ignored)
{
    soc_control_t    *soc = SOC_CONTROL(unit);
    uint32        stat;
    int            errcnt;

    COMPILER_REFERENCE(ignored);

    soc_pci_analyzer_trigger(unit);

    stat = soc_pci_read(unit, CMIC_DMA_STAT);

    soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_PCI_PARITY_ERR_CLR);

    errcnt = soc->stat.intr_pci_pe++;

    if (!soc->pciParityDPC) {
    soc->pciParityDPC = 1;
    sal_dpc_time(PCI_REPORT_PERIOD, _soc_pci_report_error,
             INT_TO_PTR(unit), INT_TO_PTR(stat), 
             INT_TO_PTR(PCI_REPORT_TYPE_PARITY), 
             INT_TO_PTR(errcnt), 0);
    }
}

STATIC void
soc_intr_pci_fatal(int unit, uint32 ignored)
{
    soc_control_t    *soc = SOC_CONTROL(unit);
    uint32        stat;
    int            errcnt;

    COMPILER_REFERENCE(ignored);

    soc_pci_analyzer_trigger(unit);

    stat = soc_pci_read(unit, CMIC_DMA_STAT);

    soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_PCI_FATAL_ERR_CLR);

    errcnt = soc->stat.intr_pci_fe++;

    if (!soc->pciFatalDPC) {
    soc->pciFatalDPC = 1;
    sal_dpc_time(PCI_REPORT_PERIOD, _soc_pci_report_error,
             INT_TO_PTR(unit), INT_TO_PTR(stat), 
             INT_TO_PTR(PCI_REPORT_TYPE_FATAL), 
             INT_TO_PTR(errcnt), 0);
    }
}

STATIC void
soc_intr_i2c(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);
    
    soc->stat.intr_i2c++;

#if defined (INCLUDE_I2C) && !defined (BCM_PETRA_SUPPORT) && !defined (BCM_DFE_SUPPORT)
    soc_i2c_intr(unit);
#else
    soc_intr_disable(unit, IRQ_I2C_INTR);
#endif
}

STATIC void
soc_intr_miim_op(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_OP_DONE_CLR);

    soc->stat.intr_mii++;

    if (soc->miimIntr) {
    sal_sem_give(soc->miimIntr);
    }
}
   
STATIC void
soc_intr_stat_dma(int unit, uint32 ignored)
{
    soc_control_t *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    soc_pci_write(unit, CMIC_DMA_STAT, DS_STAT_DMA_ITER_DONE_CLR);

    soc->stat.intr_stats++;

    if (soc->counter_intr) {
    sal_sem_give(soc->counter_intr);
    }
}

#ifdef BCM_HERCULES_SUPPORT

STATIC void
_soc_intr_mmu_analyze(void *p_unit, void *p2, void *p3, void *p4, void *p5)
{
    int unit = PTR_TO_INT(p_unit);
 
    COMPILER_REFERENCE(p2);
    COMPILER_REFERENCE(p3);
    COMPILER_REFERENCE(p4);
    COMPILER_REFERENCE(p5);
 
    if (soc_mmu_error_all(unit) < 0) {
        soc_cm_debug(DK_ERR, 
                     "MMU error analysis failed, MMU interrupt disabled\n");
    } else {
        soc_intr_enable(unit, IRQ_MMU_IRQ_STAT);
    }
}
 
STATIC void
soc_intr_mmu_stat(int unit, uint32 ignored)
{
    uint32         src, mask;
    soc_control_t     *soc = SOC_CONTROL(unit);

    COMPILER_REFERENCE(ignored);

    src = soc_pci_read(unit, CMIC_MMUIRQ_STAT);
    mask = soc_pci_read(unit, CMIC_MMUIRQ_MASK);

    mask &= ~src;

    /* We know about the port(s), don't interrupt again until serviced */
    soc_pci_write(unit, CMIC_MMUIRQ_MASK, mask);    

    soc->stat.intr_mmu++;

    /* We'll turn this back on if we succeed in the analysis */
    soc_intr_disable(unit, IRQ_MMU_IRQ_STAT);
    sal_dpc(_soc_intr_mmu_analyze, INT_TO_PTR(unit), 0, 0, 0, 0);
}

#endif /* BCM_HERCULES_SUPPORT */


#if defined(BCM_XGS12_SWITCH_SUPPORT)
STATIC void
soc_intr_arl_error(int unit, uint32 ignored)
{
    soc_control_t    *soc;

    COMPILER_REFERENCE(ignored);

    soc = SOC_CONTROL(unit);
    soc->stat.intr_mmu++;    /* should use separate counter */
    soc_intr_disable(unit, IRQ_ARL_ERROR);
    
    
    soc_cm_debug(DK_ERR,
         "UNIT %d ARL ERROR (bucket overflow or parity error\n",
         unit);
}

#endif /* BCM_XGS12_SWITCH_SUPPORT */

STATIC void
soc_intr_bit21(int unit, uint32 ignored)
{
#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        soc_control_t *soc;
        soc = SOC_CONTROL(unit);

        /* IRQ_FIFO_CH0_DMA */
        soc_intr_disable(unit, IRQ_FIFO_CH0_DMA);
        soc->stat.intr_fifo_dma[0]++;

    /* Clear FIFO_CH0_DMA_HOSTMEM_TIMEOUT bit */
    WRITE_CMIC_FIFO_RD_DMA_DEBUGr(unit, 1);

        if (soc->ipfixIntr) {
            /* Ingress IPFIX */
        sal_sem_give(soc->ipfixIntr);
        }
        return;
    }
#endif
#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
    soc_control_t *soc;
    soc = SOC_CONTROL(unit);
    
    soc_intr_disable(unit, IRQ_FIFO_CH0_DMA);

    soc->stat.intr_fifo_dma[0]++;
    soc_cm_debug(DK_INTR,"irq_fifo_ch0_dma unit %d\n", unit);

        /* IRQ_FIFO_CH0_DMA */
    if (soc_sbx_sirius_process_cs_dma_fifo(unit) != SOC_E_NONE) {
        soc_cm_debug(DK_COUNTER, "irq_fifo_ch0_dma: unit=%d CS FIFO busy\n", unit);
    }

    /* Clear interrupt by clearing FIFO_CH0_DMA_HOSTMEM_OVERFLOW bit */
    WRITE_CMIC_FIFO_RD_DMA_DEBUGr(unit, 0);

    /* Clear interrupt by clearing FIFO_CH0_DMA_HOSTMEM_TIMEOUT bit */
    WRITE_CMIC_FIFO_RD_DMA_DEBUGr(unit, 1);

    soc_intr_enable(unit, IRQ_FIFO_CH0_DMA);
        return;
    }
#endif
#ifdef BCM_HERCULES_SUPPORT
    if (SOC_IS_HERCULES(unit)) {
    soc_intr_mmu_stat(unit, ignored);
    }
#endif /* BCM_HERCULES_SUPPORT */

#if defined(BCM_XGS12_SWITCH_SUPPORT)
    if (SOC_IS_XGS12_SWITCH(unit)) {
    soc_intr_arl_error(unit, ignored);
    }
#endif /* BCM_XGS_SWITCH_SUPPORT */
}

STATIC void
soc_intr_bit22(int unit, uint32 ignored)
{
#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        soc_control_t *soc;
        soc = SOC_CONTROL(unit);

        /* IRQ_FIFO_CH1_DMA */
        soc_intr_disable(unit, IRQ_FIFO_CH1_DMA);
        soc->stat.intr_fifo_dma[1]++;

    /* Clear FIFO_CH1_DMA_HOSTMEM_TIMEOUT bit */
    WRITE_CMIC_FIFO_RD_DMA_DEBUGr(unit, 3);

        if (soc->arl_notify) {
            /* Internal L2_MOD_FIFO */
        sal_sem_give(soc->arl_notify);
        }
        return;
    }
#endif
}

STATIC void
soc_intr_bit23(int unit, uint32 ignored)
{
#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        soc_control_t *soc;
        soc = SOC_CONTROL(unit);

        /* IRQ_FIFO_CH2_DMA */
        soc_intr_disable(unit, IRQ_FIFO_CH2_DMA);
        soc->stat.intr_fifo_dma[2]++;

    /* Clear FIFO_CH2_DMA_HOSTMEM_TIMEOUT bit */
    WRITE_CMIC_FIFO_RD_DMA_DEBUGr(unit, 5);

        if (soc->arl_notify) {
            /* External EXT_L2_MOD_FIFO */
        sal_sem_give(soc->arl_notify);
        }
        return;
    }
#endif
}

STATIC void
soc_intr_lpm_lo_parity(int unit, uint32 ignored)
{
#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        soc_control_t *soc;
        soc = SOC_CONTROL(unit);

        /* IRQ_FIFO_CH3_DMA */
        soc_intr_disable(unit, IRQ_FIFO_CH3_DMA);
        soc->stat.intr_fifo_dma[3]++;

    /* Clear FIFO_CH3_DMA_HOSTMEM_TIMEOUT bit */
    WRITE_CMIC_FIFO_RD_DMA_DEBUGr(unit, 7);

        if (soc->ipfixIntr) {
            /* Egress IPFIX */
        sal_sem_give(soc->ipfixIntr);
        }
        return;
    }
#endif
}

STATIC void
soc_intr_bit25(int unit, uint32 ignored)
{
}

STATIC void
soc_intr_bit26(int unit, uint32 ignored)
{
}

STATIC void
soc_intr_bit27(int unit, uint32 ignored)
{
}

STATIC void
soc_intr_bit28(int unit, uint32 ignored)
{
    soc_pci_analyzer_trigger(unit);

#if defined(BCM_XGS_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) 
    (void)soc_ser_parity_error_intr(unit);
#endif
}

STATIC void
soc_intr_bit31(int unit, uint32 ignored)
{
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_time_support)) {
        soc_control_t    *soc = SOC_CONTROL(unit);
        soc_intr_disable(unit, IRQ_BROADSYNC_INTR);

        /* Perform user callout, if one is registered */
        if (soc->soc_time_callout != NULL) {
            (*soc->soc_time_callout)(unit);
        }
        soc_intr_enable(unit, IRQ_BROADSYNC_INTR);
    }
#endif /* BCM_TRIUMPH2_SUPPORT */
}


STATIC void
soc_intr_block(int unit, uint32 block)
{
#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
    if (block < 32) {
        soc_intr_block_lo_disable(unit, (1<<block));
    } else {
        soc_intr_block_hi_disable(unit, (1<<(block-32)));
    }
        sal_dpc(soc_sirius_block_error, INT_TO_PTR(unit),
        INT_TO_PTR(block), 0, 0, 0);
    }
#endif /* BCM_SIRIUS_SUPPORT */
    if (block < 32) {
        soc_intr_block_lo_disable(unit, (1<<block));
    } else {
        soc_intr_block_hi_disable(unit, (1<<(block-32)));
    }
    sal_dpc(soc_cmn_block_error, INT_TO_PTR(unit), INT_TO_PTR(block), 0, 0, 0);
}

/*
 * Enable (unmask) or disable (mask) a set of CMIC interrupts.  These
 * routines should be used instead of manipulating CMIC_IRQ_MASK
 * directly, since a read-modify-write is required.  The return value is
 * the previous mask (can pass mask of 0 to just get the current mask).
 */

uint32
soc_intr_enable(int unit, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_IRQ_MASK(unit);
    SOC_IRQ_MASK(unit) |= mask;
    newMask = SOC_IRQ_MASK(unit);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED && !SOC_IS_SAND(unit)) {
            newMask = 0;
    }
    IRQ_MASK_SET(unit, CMIC_IRQ_MASK, newMask);
    sal_spl(s);

    return oldMask;
}

uint32
soc_intr_disable(int unit, uint32 mask)
{
    uint32 oldMask;
    uint32 newMask;
    int s;

    s = sal_splhi();
    oldMask = SOC_IRQ_MASK(unit);
    SOC_IRQ_MASK(unit) &= ~mask;
    newMask = SOC_IRQ_MASK(unit);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    IRQ_MASK_SET(unit, CMIC_IRQ_MASK, newMask);
    sal_spl(s);

    return oldMask;
}


/*
 * Enable (unmask) or disable (mask) a set of CMIC block specific interrupts. 
 * soc_intr_block_lo_enable/disable handle block 0-31, while
 * soc_intr_block_hi_enable/disable handle block 32-63. 
 * These routines should be used instead of manipulating CMIC_IRQ_MASK_1/CMIC_IRQ_MASK_2
 * directly, since a read-modify-write is required.  The return value is
 * the previous mask (can pass mask of 0 to just get the current mask).
 * For now, these routines apply to SIRIUS only, leave in the common directory
 * in case later devices start to use the new set of IRQ registers
 */

uint32
soc_intr_block_lo_enable(int unit, uint32 mask)
{
    uint32 oldMask = 0;

#ifdef BCM_SIRIUS_SUPPORT
{
    uint32 newMask;
    int s;

    if (SOC_IS_SIRIUS(unit)) {
    s = sal_splhi();
    oldMask = SOC_IRQ1_MASK(unit);
    SOC_IRQ1_MASK(unit) |= mask;
    newMask = SOC_IRQ1_MASK(unit);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    soc_pci_write(unit, CMIC_IRQ_MASK_1, newMask);
    sal_spl(s);
    }
}
#endif /* BCM_SIRIUS_SUPPORT */
#ifdef BCM_DFE_SUPPORT
{
    uint32 newMask;
    int s;

    if (SOC_IS_DFE(unit)) {
    s = sal_splhi();
    oldMask = SOC_IRQ1_MASK(unit);
    SOC_IRQ1_MASK(unit) |= mask;
    newMask = SOC_IRQ1_MASK(unit);
    _SOC_VERB((("%s(): oldMask=0x%x, mask=0x%x, newMask=0x%x\n"), FUNCTION_NAME(), oldMask, mask, newMask));
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    soc_pci_write(unit, CMIC_IRQ_MASK_1, newMask);
    sal_spl(s);
    }
}
#endif /* BCM_DFE_SUPPORT */
    return oldMask;
}

uint32
soc_intr_block_lo_disable(int unit, uint32 mask)
{
    uint32 oldMask = 0;

#ifdef BCM_SIRIUS_SUPPORT
{
    uint32 newMask;
    int s;

    if (SOC_IS_SIRIUS(unit)) {
    s = sal_splhi();
    oldMask = SOC_IRQ1_MASK(unit);
    SOC_IRQ1_MASK(unit) &= ~mask;
    newMask = SOC_IRQ1_MASK(unit);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    soc_pci_write(unit, CMIC_IRQ_MASK_1, newMask);
    sal_spl(s);
    }
}
#endif /* BCM_SIRIUS_SUPPORT */
#ifdef BCM_DFE_SUPPORT
{
    uint32 newMask;
    int s;

    if (SOC_IS_DFE(unit)) {
    s = sal_splhi();
    oldMask = SOC_IRQ1_MASK(unit);
    SOC_IRQ1_MASK(unit) &= ~mask;
    newMask = SOC_IRQ1_MASK(unit);
    _SOC_VERB((("%s(): oldMask=0x%x, mask=0x%x, newMask=0x%x\n"), FUNCTION_NAME(), oldMask, mask, newMask));
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    soc_pci_write(unit, CMIC_IRQ_MASK_1, newMask);
    sal_spl(s);
    }
}
#endif /* BCM_DFE_SUPPORT */
    return oldMask;
}

uint32
soc_intr_block_hi_enable(int unit, uint32 mask)
{
    uint32 oldMask = 0;

#ifdef BCM_SIRIUS_SUPPORT
{
    uint32 newMask;
    int s;

    if (SOC_IS_SIRIUS(unit)) {
    s = sal_splhi();
    oldMask = SOC_IRQ2_MASK(unit);
    SOC_IRQ2_MASK(unit) |= mask;
    newMask = SOC_IRQ2_MASK(unit);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    soc_pci_write(unit, CMIC_IRQ_MASK_2, newMask);
    sal_spl(s);
    }
}
#endif /* BCM_SIRIUS_SUPPORT */
#ifdef BCM_DFE_SUPPORT
{
    uint32 newMask;
    int s;

    if (SOC_IS_DFE(unit)) {
    s = sal_splhi();
    oldMask = SOC_IRQ2_MASK(unit);
    SOC_IRQ2_MASK(unit) |= mask;
    newMask = SOC_IRQ2_MASK(unit);
    _SOC_VERB((("%s(): oldMask=0x%x, mask=0x%x, newMask=0x%x\n"), FUNCTION_NAME(), oldMask, mask, newMask));
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    soc_pci_write(unit, CMIC_IRQ_MASK_2, newMask);
    sal_spl(s);
    }
}
#endif /* BCM_DFE_SUPPORT */

    return oldMask;
}

uint32
soc_intr_block_hi_disable(int unit, uint32 mask)
{
    uint32 oldMask = 0;

#ifdef BCM_SIRIUS_SUPPORT
{
    uint32 newMask;
    int s;

    if (SOC_IS_SIRIUS(unit)) {
    s = sal_splhi();
    oldMask = SOC_IRQ2_MASK(unit);
    SOC_IRQ2_MASK(unit) &= ~mask;
    newMask = SOC_IRQ2_MASK(unit);
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    soc_pci_write(unit, CMIC_IRQ_MASK_2, newMask);
    sal_spl(s);
    }
}
#endif /* BCM_SIRIUS_SUPPORT */
#ifdef BCM_DFE_SUPPORT
{
    uint32 newMask;
    int s;

    if (SOC_IS_DFE(unit)) {
    s = sal_splhi();
    oldMask = SOC_IRQ2_MASK(unit);
    SOC_IRQ2_MASK(unit) &= ~mask;
    newMask = SOC_IRQ2_MASK(unit);
    _SOC_VERB((("%s(): oldMask=0x%x, mask=0x%x, newMask=0x%x\n"), FUNCTION_NAME(), oldMask, mask, newMask));
    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
        newMask = 0;
    }
    soc_pci_write(unit, CMIC_IRQ_MASK_2, newMask);
    sal_spl(s);
    }
}
#endif /* BCM_DFE_SUPPORT */
    return oldMask;
}

/*
 * SOC Interrupt Service Routine
 *
 *   In PLI simulation, the intr thread can call this routine at any
 *   time.  The connection is protected at the level of pli_{set/get}reg.
 */

#define POLL_LIMIT 100000


void
soc_intr(void *_unit)
{
    uint32         irqStat, irqMask;
    soc_control_t    *soc;
    int         i = 0;
    int         poll_limit = POLL_LIMIT;
    int                 unit = PTR_TO_INT(_unit);
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

    soc->stat.intr++;        /* Update count */

    /*
     * Read IRQ Status and IRQ Mask and AND to determine active ints.
     * These are re-read each time since either can be changed by ISRs.
     *
     * Since interrupts are edge-driven, it's necessary to continue
     * processing them until the IRQ_STAT register reads zero.  If we
     * return without doing that, we may never see another interrupt!
     */
    for (;;) {
    irqStat = soc_pci_read(unit, CMIC_IRQ_STAT);
    if (irqStat == 0) {
        break;
    }
    irqMask = SOC_IRQ_MASK(unit);
    irqStat &= irqMask;
    if (irqStat == 0) {
        break;
    }

    /*
     * find starting point for handler search
     * skip over blocks of high-priority but unlikely entries
     */
    if ((irqStat & INTR_START2_MASK) == 0) {
        i = INTR_START2_POS;
    } else if ((irqStat & INTR_START1_MASK) == 0) {
        i = INTR_START1_POS;
    } else {
        i = 0;
    }

        /*
         * We may have received an interrupt before all data has been
         * posted from the device or intermediate bridge. 
         * The PCI specification requires that we read a device register
         * to make sure pending data is flushed. 
         * Some bridges (we have determined through testing) require more
         * than one read.
         */
        soc_pci_read(unit, CMIC_SCHAN_CTRL); 
        soc_pci_read(unit, CMIC_IRQ_MASK); 

    for (; i < INTR_HANDLERS_COUNT; i++) {
        if (irqStat & soc_intr_handlers[i].mask) {

        /*
         * Bit found, dispatch interrupt
         */

        soc_cm_debug(DK_INTR,
                 "soc_intr unit %d: dispatch %s\n",
                 unit, soc_intr_handlers[i].intr_name);

        (*soc_intr_handlers[i].intr_fn)
            (unit, soc_intr_handlers[i].intr_data);

        /*
         * Prevent infinite loop in interrupt handler by
         * disabling the offending interrupt(s).
         */

        if (--poll_limit == 0) {
            soc_cm_debug(DK_ERR,
                 "soc_intr unit %d: "
                 "ERROR can't clear interrupt(s): "
                 "IRQ=0x%x (disabling 0x%x)\n",
                 unit, irqStat, soc_intr_handlers[i].mask);
            soc_intr_disable(unit, soc_intr_handlers[i].mask);
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

    if (soc_feature(unit, soc_feature_extended_cmic_error)) {
    /* process block specific interrupts for block 0 - 31 */
    for (;;) {
        irqStat = soc_pci_read(unit, CMIC_IRQ_STAT_1);
        if (irqStat == 0) {
        break;
        }
        irqMask = SOC_IRQ1_MASK(unit);
        irqStat &= irqMask;
        if (irqStat == 0) {
        break;
        }
        
        /*
         * We may have received an interrupt before all data has been
         * posted from the device or intermediate bridge. 
         * The PCI specification requires that we read a device register
         * to make sure pending data is flushed. 
         * Some bridges (we have determined through testing) require more
         * than one read.
         */
        soc_pci_read(unit, CMIC_SCHAN_CTRL); 
        soc_pci_read(unit, CMIC_IRQ_MASK_1); 
        
        for (i=0 ; i < INTR_BLOCK_LO_HANDLERS_COUNT; i++) {
        if (irqStat & soc_intr_block_lo_handlers[i].mask) {
            
            /*
             * Bit found, dispatch interrupt
             */
            
            soc_cm_debug(DK_INTR,
                 "soc_intr unit %d: dispatch %s\n",
                 unit, soc_intr_block_lo_handlers[i].intr_name);
            
            (*soc_intr_block_lo_handlers[i].intr_fn)
            (unit, soc_intr_block_lo_handlers[i].intr_data);
            
            /*
             * Prevent infinite loop in interrupt handler by
             * disabling the offending interrupt(s).
             */
            
            if (--poll_limit == 0) {
            soc_cm_debug(DK_ERR,
                     "soc_intr unit %d: "
                     "ERROR can't clear interrupt(s): "
                     "IRQ=0x%x (disabling 0x%x)\n",
                     unit, irqStat, soc_intr_block_lo_handlers[i].mask);
            soc_intr_block_lo_disable(unit, soc_intr_block_lo_handlers[i].mask);
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
    
    /* process block specific interrupts for block 32 - 63 */
    for (;;) {
        irqStat = soc_pci_read(unit, CMIC_IRQ_STAT_2);
        if (irqStat == 0) {
        break;
        }
        irqMask = SOC_IRQ2_MASK(unit);
        irqStat &= irqMask;
        if (irqStat == 0) {
        break;
        }
        
        /*
         * We may have received an interrupt before all data has been
         * posted from the device or intermediate bridge. 
         * The PCI specification requires that we read a device register
         * to make sure pending data is flushed. 
         * Some bridges (we have determined through testing) require more
         * than one read.
         */
        soc_pci_read(unit, CMIC_SCHAN_CTRL); 
        soc_pci_read(unit, CMIC_IRQ_MASK_2); 
        
        for (i=0; i < INTR_BLOCK_HI_HANDLERS_COUNT; i++) {
        if (irqStat & soc_intr_block_hi_handlers[i].mask) {
            
            /*
             * Bit found, dispatch interrupt
             */
            
            soc_cm_debug(DK_INTR,
                 "soc_intr unit %d: dispatch %s\n",
                 unit, soc_intr_block_hi_handlers[i].intr_name);
            
            (*soc_intr_block_hi_handlers[i].intr_fn)
            (unit, soc_intr_block_hi_handlers[i].intr_data);
            
            /*
             * Prevent infinite loop in interrupt handler by
             * disabling the offending interrupt(s).
             */
            
            if (--poll_limit == 0) {
            soc_cm_debug(DK_ERR,
                     "soc_intr unit %d: "
                     "ERROR can't clear interrupt(s): "
                     "IRQ=0x%x (disabling 0x%x)\n",
                     unit, irqStat, soc_intr_block_hi_handlers[i].mask);
            soc_intr_block_hi_disable(unit, soc_intr_block_hi_handlers[i].mask);
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
    }

    if (soc_feature(unit, soc_feature_short_cmic_error)) {
        /* Using sal_dpc since there are schan reads in this function
         * and schan read cant be done from interrupt context.
         * the function soc_cmn_error will be excecuted only after this 
         * function will end.
         */
        sal_dpc(soc_cmn_error, INT_TO_PTR(unit), 0, 0, 0, 0);
    }

    /* In polled mode, the hardware IRQ mask is always zero */
    if (SOC_CONTROL(unit)->soc_flags & SOC_F_POLLED) {
#ifdef SAL_SPL_LOCK_ON_IRQ
        sal_spl(s);
#endif
        return;
    }
    /*
     * If the interrupt handler is not run in interrupt context, but 
     * rather as a thread or a signal handler, the interrupt handler 
     * must reenable interrupts on the switch controller. Currently
     * we don't distinguish between the two modes of operation, so 
     * we always reenable interrupts here.
     */
    IRQ_MASK_SET(unit, CMIC_IRQ_MASK, SOC_IRQ_MASK(unit));
    if (soc_feature(unit, soc_feature_extended_cmic_error)) {
        soc_pci_write(unit, CMIC_IRQ_MASK_1, SOC_IRQ1_MASK(unit));
        soc_pci_write(unit, CMIC_IRQ_MASK_2, SOC_IRQ2_MASK(unit));
    }


#ifdef SAL_SPL_LOCK_ON_IRQ
    sal_spl(s);
#endif
}


void soc_cmn_block_error(void *unit_vp, void *d1, void *d2, void *d3, void *d4)
{
    int blk, rc, is_valid, idx, i, nof_interrupts, is_enabled;
    soc_block_info_t* bi = NULL;
    soc_interrupt_db_t* interrupt, *prev_interrupt = NULL;
    int unit = PTR_TO_INT(unit_vp); /*unit should be set before SOC_INIT_FUNC_DEFS*/
    soc_reg_above_64_val_t data, field;
    int interrupt_action;
  
    SOC_INIT_FUNC_DEFS;

    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    blk = PTR_TO_INT(d1);

    _SOC_VERB((_SOC_MSG("enter \n")));

    for (i = 0; SOC_BLOCK_INFO(unit, i).type >= 0; i++) {
        if (SOC_INFO(unit).block_valid[i] && SOC_BLOCK2SCH(unit,i) == blk) {
            bi = &(SOC_BLOCK_INFO(unit, i));
            break;
        }
    }

    if (NULL == bi) {
        _SOC_ERR((_SOC_MSG("Interrupt for unknown block %d"),blk));
        SOC_EXIT;
    }

    soc_nof_interrupts(unit, &nof_interrupts);

    for (idx = 0; idx < nof_interrupts; idx++) {
        
        /* Advance to next interrupt */
        interrupt = &SOC_CONTROL(unit)->interrupts_info->interrupt_db_info[idx];
        if(NULL == interrupt) {
            _SOC_ERR((_SOC_MSG("No interrupts for device")));
            SOC_EXIT;
        }
        
        rc = soc_interrupt_is_valid(unit, bi, interrupt, &is_valid);
        _SOC_IF_ERR_EXIT(rc);
        if(!is_valid) {
            continue;
        }

        rc = soc_interrupt_is_enabled(unit, bi->number, interrupt, &is_enabled);
        _SOC_IF_ERR_EXIT(rc);
        if (!is_enabled) {
            continue;
        }

        interrupt_action = 0;
        if ((NULL != prev_interrupt) && (prev_interrupt->reg == interrupt->reg) && (prev_interrupt->reg_index == interrupt->reg_index)) {
            soc_reg_above_64_field_get(unit, interrupt->reg, data, interrupt->field, field);
        } else {
            rc = soc_reg_above_64_get(unit, interrupt->reg, bi->number, interrupt->reg_index, data);
            if(SOC_E_NONE != rc) { 
                prev_interrupt = NULL;
               _SOC_IF_ERR_EXIT(rc);
           }

           prev_interrupt = interrupt;
           soc_reg_above_64_field_get(unit, interrupt->reg, data, interrupt->field, field);
        }
        
        if (!SOC_REG_ABOVE_64_IS_ZERO(field)) {
            if(interrupt->bit_in_field!= SOC_INTERRUPT_BIT_FIELD_DONT_CARE ) {
                interrupt_action  = SHR_BITGET(field, interrupt->bit_in_field);
            } else {
              interrupt_action = 1;
            }
         }

          /* CallBack */
          if(interrupt_action) {
              soc_event_generate(unit, SOC_SWITCH_EVENT_DEVICE_INTERRUPT, idx, bi->number, 0);
          }
    }    

    if (blk < 32) {
        soc_intr_block_lo_enable(unit, (1<<blk));
    } else {
        soc_intr_block_hi_enable(unit, (1<<(blk-32)));
    }
  
exit:
    if (SOC_FAILURE(_rv)) {
        _SOC_ERR((_SOC_MSG("Internal error in soc_cmn_block_error")));
    }
    _SOC_VERB((_SOC_MSG("exit \n")));

}

void soc_cmn_error(void *unit_vp, void *d1, void *d2, void *d3, void *d4)
{
    int rc, i;
    int unit = PTR_TO_INT(unit_vp);
    int flags = 0;
    int max_interrupts_size = INTR_CMN_ERROR_MAX_INTERRUPTS_SIZE;
    soc_interrupt_cause_t interrupts[INTR_CMN_ERROR_MAX_INTERRUPTS_SIZE];
    int total_interrupts = 0;
    int interrupt_num = INTR_CMN_ERROR_MAX_INTERRUPTS_SIZE;
    
    SOC_INIT_FUNC_DEFS;

    sal_memset(interrupts, 0x0, INTR_CMN_ERROR_MAX_INTERRUPTS_SIZE * sizeof(soc_interrupt_cause_t));

    /* Get all current Active interrupts */
    flags = SOC_ACTIVE_INTERRUPTS_GET_UNMASKED_ONLY; 
    rc = soc_active_interrupts_get(unit, flags, max_interrupts_size, interrupts, &total_interrupts);
    _SOC_IF_ERR_EXIT(rc);

    _SOC_VERB((("%s(): interrupt_num=%d, max_interrupts_size=%d, total_interrupts=%d\n"), FUNCTION_NAME(), interrupt_num, max_interrupts_size, total_interrupts));

    if (interrupt_num > total_interrupts) {
        interrupt_num = total_interrupts;
    }

    /* sort interrupts according to priority */
    if(interrupt_num > 1) {
        rc = soc_sort_interrupts_according_to_priority(unit, interrupts, interrupt_num);
        _SOC_IF_ERR_EXIT(rc);
    }
       
    /* Call CB for every Active interrupt */
    for (i = 0; i < interrupt_num; i++) {
        soc_event_generate(unit, SOC_SWITCH_EVENT_DEVICE_INTERRUPT, interrupts[i].id, interrupts[i].index, 0);
    }

    /* Enable interrups */
    if (soc_feature(unit, soc_feature_cmicm)) {
        if (SOC_IS_ARAD(unit)) {
#ifdef BCM_CMICM_SUPPORT
        int cmc = SOC_PCI_CMC(unit);
        soc_pci_write(unit, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc), SOC_CMCx_IRQ2_MASK(unit, cmc));
        soc_pci_write(unit, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc), SOC_CMCx_IRQ3_MASK(unit, cmc));
        soc_pci_write(unit, CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(cmc), SOC_CMCx_IRQ4_MASK(unit, cmc));
#endif
        }
    } else {
        soc_pci_write(unit, CMIC_IRQ_MASK_1, SOC_IRQ1_MASK(unit));
        soc_pci_write(unit, CMIC_IRQ_MASK_2, SOC_IRQ2_MASK(unit));
    }

#ifdef PLISIM
    /* Turn off Interrupts in PCID - to avoid endless loop */
    if (soc_feature(unit, soc_feature_cmicm)) {
        if (SOC_IS_ARAD(unit)) {
#ifdef BCM_CMICM_SUPPORT
            int cmc = SOC_PCI_CMC(unit);
            soc_pci_write(unit, CMIC_CMCx_IRQ_STAT2_OFFSET(cmc), 0x0);
            soc_pci_write(unit, CMIC_CMCx_IRQ_STAT4_OFFSET(cmc), 0x0); 
            soc_pci_write(unit, CMIC_CMCx_IRQ_STAT3_OFFSET(cmc), 0x0);
#endif
        }
    } else {
        soc_pci_write(unit, CMIC_IRQ_STAT_1, 0x0); 
        soc_pci_write(unit, CMIC_IRQ_STAT_2, 0x0);
    }
#endif
exit:
    if (SOC_FAILURE(_rv)) {
        _SOC_ERR((_SOC_MSG("Internal error in soc_cmn_error")));
    }
    _SOC_VERB((_SOC_MSG("exit \n")));

}

int soc_interrupt_is_valid(int unit, const soc_block_info_t* bi, const soc_interrupt_db_t* inter, int* is_valid /*out*/)
{
    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(bi);
    SOC_NULL_CHECK(inter);
    SOC_NULL_CHECK(is_valid);

    if(!SOC_REG_IS_VALID(unit, inter->reg)){
        *is_valid = 0;
    } else {
        if(SOC_BLOCK_IN_LIST(SOC_REG_INFO(unit,inter->reg).block, bi->type)) {
            *is_valid = 1;
        } else {
            *is_valid = 0;
        }
    }

exit:
    SOC_FUNC_RETURN;
}

/*
 *    
 */

int soc_interrupt_get(int unit, int block_instance , const soc_interrupt_db_t* inter, int* inter_val /*out*/)
{
    soc_reg_above_64_val_t data, field, field_mask;
    int rc;
    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(inter);
    SOC_NULL_CHECK(inter_val);

    if(!SOC_REG_IS_VALID(unit, inter->reg)){
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid register for the device")));
    }

    rc = soc_reg_above_64_get(unit, inter->reg, block_instance, inter->reg_index, data);
    _SOC_IF_ERR_EXIT(rc);

    soc_reg_above_64_field_get(unit, inter->reg, data, inter->field, field);

    if (inter->bit_in_field != SOC_INTERRUPT_BIT_FIELD_DONT_CARE) {
        SOC_REG_ABOVE_64_CREATE_MASK(field_mask, 0x1, inter->bit_in_field);
        SOC_REG_ABOVE_64_AND(field, field_mask);
    }

    *inter_val = (SOC_REG_ABOVE_64_IS_ZERO(field) ? 0x0 : 0x1);

exit:
    SOC_FUNC_RETURN;
}

/*
 * Function:
 *    soc_interrupt_force_get
 * Description:
 *    Set/Clear interrupt test registers bits & appropriate mask register
 * Parameters:
 *  unit        - Device unit number
 *  block_instance   - block_instance
 *  inter            - interrupt info
 *  *inter_val        - return value
 * Returns:
 *      BCM_E_xxx
 */  
int soc_interrupt_force_get (int unit, int block_instance, const soc_interrupt_db_t* inter, int* inter_val)
{
 #if    defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)  
    soc_reg_above_64_val_t data , field_mask, field; 
    soc_field_info_t *finfop;
    int rc;
    int curr_bit;
    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(inter);
    SOC_REG_ABOVE_64_CLEAR(field);

    if (inter->reg_test != INVALIDr) {
        if(!SOC_REG_IS_VALID(unit, inter->mask_reg)){
            _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid mask register for the device")));
        }

        rc = soc_reg_above_64_get(unit, inter->reg_test, block_instance, inter->mask_reg_index, data);
        _SOC_IF_ERR_EXIT(rc);
        

        SOC_FIND_FIELD(inter->field,
                       SOC_REG_INFO(unit, inter->reg).fields,
                       SOC_REG_INFO(unit, inter->reg).nFields,
                       finfop);
        if (finfop == NULL) {
            _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid Field Name for the event")));
        }

        curr_bit = finfop->bp;
        if (inter->bit_in_field != SOC_INTERRUPT_BIT_FIELD_DONT_CARE) {
            curr_bit += inter->bit_in_field;
        }

        SOC_REG_ABOVE_64_CREATE_MASK(field_mask, 0x1, curr_bit);
        SOC_REG_ABOVE_64_AND(field, field_mask);
        *inter_val = (SOC_REG_ABOVE_64_IS_ZERO(field) ? 0x0 : 0x1);
    }

exit:
    SOC_FUNC_RETURN;
#else
  return 0;
#endif

}


int soc_interrupt_enable(int unit, int block_instance, const soc_interrupt_db_t* inter)
{
    soc_reg_above_64_val_t data, field;
    int rc;
    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(inter);

    if(!SOC_REG_IS_VALID(unit, inter->mask_reg)){
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid mask register for the device")));
    }

    rc = soc_reg_above_64_get(unit, inter->mask_reg, block_instance, inter->mask_reg_index, data);
    _SOC_IF_ERR_EXIT(rc);

    if (inter->bit_in_field == SOC_INTERRUPT_BIT_FIELD_DONT_CARE) {
        SOC_REG_ABOVE_64_CLEAR(field);
        SHR_BITSET(field, 0x0);
    } else {
        soc_reg_above_64_field_get(unit, inter->mask_reg, data, inter->mask_field, field);
        SHR_BITSET(field, inter->bit_in_field);
    }
    
    soc_reg_above_64_field_set(unit, inter->mask_reg, data, inter->mask_field, field);

    rc = soc_reg_above_64_set(unit, inter->mask_reg, block_instance,  inter->mask_reg_index, data);
    _SOC_IF_ERR_EXIT(rc);

exit:
    SOC_FUNC_RETURN;

}

/*
 * Function:
 *    soc_interrupt_force
 * Description:
 *    Set/Clear interrupt test registers bits & appropriate mask register
 * Parameters:
 *  unit        - Device unit number
 *  block_instance   - block_instance
 *  inter            - interrupt info
 *  action           - action to do - 0 enable force interrupts, 1 - disable it
 * Returns:
 *      BCM_E_xxx
 */ 
int soc_interrupt_force (int unit, int block_instance, const soc_interrupt_db_t* inter, int action)
{
 #if   defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)  
    soc_reg_above_64_val_t data; /* , field; */
    soc_field_info_t *finfop;
    int rc;
    int curr_bit;
    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(inter);

    if (inter->reg_test != INVALIDr) {
        if(!SOC_REG_IS_VALID(unit, inter->mask_reg)){
            _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid mask register for the device")));
        }

        rc = soc_reg_above_64_get(unit, inter->reg_test, block_instance, inter->reg_index, data);
        _SOC_IF_ERR_EXIT(rc);
        

        SOC_FIND_FIELD(inter->field,
                       SOC_REG_INFO(unit, inter->reg).fields,
                       SOC_REG_INFO(unit, inter->reg).nFields,
                       finfop);
        if (finfop == NULL) {
            _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid Field Name for the event")));
        }

        curr_bit = finfop->bp;
        if (inter->bit_in_field != SOC_INTERRUPT_BIT_FIELD_DONT_CARE) {
            curr_bit += inter->bit_in_field;
        }


        if (action == 0) { /* enable force*/
           /* SOC_REG_ABOVE_64_CLEAR(data); */
            SHR_BITSET(data, curr_bit); 
        } else if (action == 1){ /* disable force*/
            SHR_BITCLR(data, curr_bit);
        } else {
            _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid action")));
        }

        rc = soc_reg_above_64_set(unit, inter->reg_test, block_instance,  inter->reg_index, data);
        _SOC_IF_ERR_EXIT(rc);
    }

exit:
    SOC_FUNC_RETURN;

#else
  return 0;
#endif
}


int soc_interrupt_disable(int unit, int block_instance, const soc_interrupt_db_t* inter)
{
    soc_reg_above_64_val_t data, field;
    int rc;
     
    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(inter);

    if(!SOC_REG_IS_VALID(unit, inter->mask_reg)){
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid mask register for the device")));
    }

    rc = soc_reg_above_64_get(unit, inter->mask_reg, block_instance, inter->mask_reg_index, data);
    _SOC_IF_ERR_EXIT(rc);

    if (inter->bit_in_field == SOC_INTERRUPT_BIT_FIELD_DONT_CARE) {
        SOC_REG_ABOVE_64_CLEAR(field);
    } else {
        soc_reg_above_64_field_get(unit, inter->mask_reg, data, inter->mask_field, field);
        SHR_BITCLR(field, inter->bit_in_field);
    }
    
    soc_reg_above_64_field_set(unit, inter->mask_reg, data, inter->mask_field, field);

    rc = soc_reg_above_64_set(unit, inter->mask_reg, block_instance,  inter->mask_reg_index, data);
    _SOC_IF_ERR_EXIT(rc);

exit:

    SOC_FUNC_RETURN;
}

int soc_interrupt_is_enabled(int unit, int block_instance, const soc_interrupt_db_t* inter, int* is_enabled /*out*/)
{
    soc_reg_above_64_val_t data, field, field_mask;
    int rc;
    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(inter);

    if(!SOC_REG_IS_VALID(unit, inter->mask_reg)){
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid mask register for the device")));
    }

    rc = soc_reg_above_64_get(unit, inter->mask_reg, block_instance, inter->mask_reg_index, data);
    _SOC_IF_ERR_EXIT(rc);

    soc_reg_above_64_field_get(unit, inter->mask_reg, data, inter->mask_field, field);

    if (inter->bit_in_field != SOC_INTERRUPT_BIT_FIELD_DONT_CARE) {
        SOC_REG_ABOVE_64_CREATE_MASK(field_mask, 0x1, inter->bit_in_field);
        SOC_REG_ABOVE_64_AND(field, field_mask);
    }

    *is_enabled = (SOC_REG_ABOVE_64_IS_ZERO(field) ? 0x0 : 0x1);

exit:
    SOC_FUNC_RETURN;
}

/* 
 * Interrupt Clear Functions
 */

int soc_interrupt_clear_on_write(int unit, int block_instance, int interrupt_id)
{
    soc_reg_above_64_val_t data, field;
    int rc;
    int nof_interrupts;
    soc_interrupt_db_t *inter, *interrupts;
    SOC_INIT_FUNC_DEFS;

    if (!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    interrupts = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;
    SOC_NULL_CHECK(interrupts);

    /*verify interrupt_id*/
    soc_nof_interrupts(unit, &nof_interrupts);
    if ((interrupt_id > nof_interrupts) || interrupt_id < 0) {
          _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("event_id is unavail")));
    }

    inter = &(interrupts[interrupt_id]);
    if (!SOC_REG_IS_VALID(unit, inter->reg)) {
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid register for the device")));
    }

    if (!SOC_REG_IS_VALID(unit, inter->mask_reg)) {
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid mask register for the device")));
    }

    SOC_REG_ABOVE_64_CLEAR(data);
    SOC_REG_ABOVE_64_CLEAR(field);
    
    if (inter->bit_in_field == SOC_INTERRUPT_BIT_FIELD_DONT_CARE) {
        SHR_BITSET(field, 0x0);
    } else {
        SHR_BITSET(field, inter->bit_in_field);
    }

    soc_reg_above_64_field_set(unit, inter->reg, data, inter->field, field);

    rc = soc_reg_above_64_set(unit, inter->reg, block_instance,  inter->reg_index, data);
    _SOC_IF_ERR_EXIT(rc);

exit:
    SOC_FUNC_RETURN;
}

int soc_interrupt_clear_on_reg_write(int unit, int block_instance, int interrupt_id)
{
    int rc;
    int nof_interrupts;
    soc_interrupt_db_t *inter, *interrupts;
    SOC_INIT_FUNC_DEFS;

    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    interrupts = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;
    SOC_NULL_CHECK(interrupts);

    /*verify interrupt_id*/
    soc_nof_interrupts(unit, &nof_interrupts);
    if ((interrupt_id > nof_interrupts) || interrupt_id < 0) {
          _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("event_id is unavail")));
    }

    inter = &(interrupts[interrupt_id]);
    if(!SOC_REG_IS_VALID(unit, ((soc_interrupt_clear_reg_write_t*)inter->interrupt_clear_param1)->status_reg)){
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid register for the device")));
    }
    
    rc = soc_reg_above_64_set(unit, 
                              ((soc_interrupt_clear_reg_write_t*)inter->interrupt_clear_param1)->status_reg, 
                              block_instance,  
                              inter->reg_index, 
                              ((soc_interrupt_clear_reg_write_t*)inter->interrupt_clear_param1)->data);
    _SOC_IF_ERR_EXIT(rc);

exit:
    SOC_FUNC_RETURN;

}

int soc_interrupt_clear_on_clear(int unit, int block_instance, int interrupt_id)
{
    soc_reg_above_64_val_t data, field;
    int rc;
    int nof_interrupts;
    soc_interrupt_db_t *inter, *interrupts;
    SOC_INIT_FUNC_DEFS;

    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    interrupts = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;

    SOC_NULL_CHECK(interrupts);

    /*verify interrupt_id*/
    soc_nof_interrupts(unit, &nof_interrupts);
    if ((interrupt_id > nof_interrupts) || interrupt_id < 0) {
          _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("event_id is unavail")));
    }

    inter = &(interrupts[interrupt_id]);
    if(!SOC_REG_IS_VALID(unit, inter->reg)){
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid register for the device")));
    }
    if(!SOC_REG_IS_VALID(unit, inter->mask_reg)){
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid mask register for the device")));
    }

    SOC_REG_ABOVE_64_CLEAR(data);
    SOC_REG_ABOVE_64_CLEAR(field);

    rc = soc_reg_above_64_get(unit, inter->reg, block_instance, inter->reg_index, data);
    _SOC_IF_ERR_EXIT(rc);

    if (inter->bit_in_field != SOC_INTERRUPT_BIT_FIELD_DONT_CARE) {
        soc_reg_above_64_field_get(unit, inter->reg, data, inter->field, field);
        SHR_BITCLR(field, inter->bit_in_field);
    }

    soc_reg_above_64_field_set(unit, inter->reg, data, inter->field, field);
 
    rc = soc_reg_above_64_set(unit, inter->reg, block_instance,  inter->reg_index, data);
    _SOC_IF_ERR_EXIT(rc);
 
exit:
    SOC_FUNC_RETURN;
}

/* 
 *    
 */
int soc_interrupt_clear_on_read_fifo(int unit, int block_instance, int interrupt_id)
{
    soc_interrupt_db_t *inter, *interrupts;
    int nof_interrupts;
    int rc, read_count;
    soc_reg_above_64_val_t data ;
    int inter_get;
    
    SOC_INIT_FUNC_DEFS;
    
    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    interrupts = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;
    SOC_NULL_CHECK(interrupts);
    
    /*verify interrupt_id*/
    soc_nof_interrupts(unit, &nof_interrupts);
    if ((interrupt_id > nof_interrupts) || interrupt_id < 0) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("event_id is unavail")));
    }
    inter = &(interrupts[interrupt_id]);
    if(!SOC_REG_IS_VALID(unit, inter->reg)){
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid register for the device")));
    }
    if(!SOC_REG_IS_VALID(unit, inter->mask_reg)){
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid mask register for the device")));
    }

    for(read_count = 0; read_count < ((soc_interrupt_clear_read_fifo_t*)(inter->interrupt_clear_param1))->read_count; read_count++){
        rc = soc_reg_above_64_get(unit, ((soc_interrupt_clear_read_fifo_t*)(inter->interrupt_clear_param1))->fifo_reg, block_instance, inter->reg_index, data);
        _SOC_IF_ERR_EXIT(rc);

        rc = soc_interrupt_get(unit, block_instance , inter, &inter_get);
        _SOC_IF_ERR_EXIT(rc);

        if (!inter_get) {
            break;
        }
    }
 
exit:
    SOC_FUNC_RETURN;
}


/* 
 *    
 */
int soc_interrupt_clear_on_read_array_index(int unit, int block_instance, int interrupt_id)
{
    soc_interrupt_db_t *inter, *interrupts;
    int nof_interrupts;
    int rc, read_count;
    soc_reg_above_64_val_t data ;
    int inter_get;
    
    SOC_INIT_FUNC_DEFS;
    
    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    interrupts = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;
    SOC_NULL_CHECK(interrupts);
    
    /*verify interrupt_id*/
    soc_nof_interrupts(unit, &nof_interrupts);
    if ((interrupt_id > nof_interrupts) || interrupt_id < 0) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("event_id is unavail")));
    }
    inter = &(interrupts[interrupt_id]);
    if(!SOC_REG_IS_VALID(unit, inter->reg)){
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid register for the device")));
    }
    if(!SOC_REG_IS_VALID(unit, inter->mask_reg)){
        _SOC_EXIT_WITH_ERR(SOC_E_INTERNAL, (_SOC_MSG("Invalid mask register for the device")));
    }

    for(read_count = 0; read_count < ((soc_interrupt_clear_array_index_t*)(inter->interrupt_clear_param1))->read_count; read_count++){
        rc = soc_reg_above_64_get(unit, 
                                  ((soc_interrupt_clear_array_index_t*)(inter->interrupt_clear_param1))->fifo_reg, 
                                  block_instance, 
                                  ((soc_interrupt_clear_array_index_t*)(inter->interrupt_clear_param1))->reg_index, 
                                  data);
        _SOC_IF_ERR_EXIT(rc);

        rc = soc_interrupt_get(unit, block_instance , inter, &inter_get);
        _SOC_IF_ERR_EXIT(rc);

        if (!inter_get) {
            break;
        }
    }
 
exit:
    SOC_FUNC_RETURN;
}


/* 
 *    
 */

int
soc_active_interrupts_get(int unit, int flags, int max_interrupts_size, soc_interrupt_cause_t *interrupts, int *total_interrupts)
{
    int rc;
    int i = 0, blk = 0, int_bit_idx = 0, vector_int_bit_idx = 0;
    int index = 0;
    int int_id, int_port=0; 
    int vector_id, vector_int_id;
    int is_unmasked_flag = 0, is_cont_prev_flag = 0;
    static int cont_prev_i_blk = 0, cont_prev_bit_idx = 0, cont_prev_vector_bit_idx = 0;
    int cont_prev_start_i_blk = 0;
    int first_blk_loop = 1;

    uint32 cmic_irq_stat[2];
    uint64 cmic_irq_stat_comb;

    soc_block_info_t *bi;
    soc_interrupt_db_t *interrupts_arr;
    soc_interrupt_tree_t *interrupt_tree;

    soc_reg_above_64_val_t block_int_data, block_int_mask_data, block_int_bitmap;
    soc_reg_above_64_val_t vector_int_data, vector_int_mask_data, vector_int_bitmap;

    soc_field_info_t *finfop;

#ifdef BCM_ARAD_SUPPORT
    uint32 cmic_irq_state_2;
#endif /* BCM_ARAD_SUPPORT */

    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(interrupts);
    SOC_NULL_CHECK(total_interrupts);

    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    interrupts_arr = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;
    SOC_NULL_CHECK(interrupts_arr);

    interrupt_tree = SOC_CONTROL(unit)->interrupts_info->interrupt_tree_info;
    SOC_NULL_CHECK(interrupt_tree);

    sal_memset(cmic_irq_stat, 0x0, 2 * sizeof(uint32));

    if (flags & SOC_ACTIVE_INTERRUPTS_GET_UNMASKED_ONLY) {
        is_unmasked_flag = 1;
    }

    if (flags & SOC_ACTIVE_INTERRUPTS_GET_CONT_PREV) {
        is_cont_prev_flag = 1;
    }

    if (is_cont_prev_flag) {
        cont_prev_start_i_blk = i = cont_prev_i_blk;
        int_bit_idx = 0 /*cont_prev_bit_idx*/;
        vector_int_bit_idx = 0 /*cont_prev_vector_bit_idx*/;
    } else {
        i = 0;
        int_bit_idx = 0;
        vector_int_bit_idx = 0;
    }

    cmic_irq_stat[0] = 0;
    cmic_irq_stat[1] = 0;
    if (soc_feature(unit, soc_feature_cmicm)) {
        if (SOC_IS_ARAD(unit)) {
#ifdef BCM_CMICM_SUPPORT
            int cmc = SOC_PCI_CMC(unit);
            cmic_irq_stat[0] = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT3_OFFSET(cmc)); 
            cmic_irq_stat[1] = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT4_OFFSET(cmc));
#ifdef BCM_ARAD_SUPPORT 
            /* add to cimic_irq_stat[0] the port interrupts bits from irq_state2 */
            if(SOC_IS_ARAD(unit)) {
                cmic_irq_state_2 = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT2_OFFSET(cmc));
                cmic_irq_stat[0] |= _PORT_BLOCK_FROM_IRQ_STATE2(unit, cmic_irq_state_2, PARITY_INTERRUPT_4f, SOC_CMIC_BLK_CLP_0_INDX)
                                 | _PORT_BLOCK_FROM_IRQ_STATE2(unit, cmic_irq_state_2, PARITY_INTERRUPT_3f, SOC_CMIC_BLK_CLP_1_INDX) 
                                 | _PORT_BLOCK_FROM_IRQ_STATE2(unit, cmic_irq_state_2, PARITY_INTERRUPT_2f, SOC_CMIC_BLK_XLP_0_INDX)    
                                 | _PORT_BLOCK_FROM_IRQ_STATE2(unit, cmic_irq_state_2, PARITY_INTERRUPT_1f, SOC_CMIC_BLK_XLP_1_INDX);
            }
#endif /* BCM_ARAD_SUPPORT */
      
#endif
        }

    } else {
        cmic_irq_stat[0] = soc_pci_read(unit, CMIC_IRQ_STAT_1); 
        cmic_irq_stat[1] = soc_pci_read(unit, CMIC_IRQ_STAT_2);
    }
    _SOC_VERB((("%s(): cmic_irq_stat[0]=0x%x, cmic_irq_stat[1]=0x%x,\n"),
               FUNCTION_NAME(), cmic_irq_stat[0], cmic_irq_stat[1]));
    COMPILER_64_SET(cmic_irq_stat_comb, cmic_irq_stat[1], cmic_irq_stat[0]);

    while (1) {

        if (SOC_BLOCK_INFO(unit, i).type < 0) {
            i = 0;
        }

        if ((i == cont_prev_start_i_blk) && (first_blk_loop == 0)) {
            break;
        }

        first_blk_loop = 0;

        if (!(SOC_INFO(unit).block_valid[i])) {
            i++;
            continue;
        }

        bi = &(SOC_BLOCK_INFO(unit, i));
        blk=bi->cmic;

        if (((!(COMPILER_64_BITTEST(cmic_irq_stat_comb,blk))) && (is_unmasked_flag == 1)) || (interrupt_tree[blk].int_reg == INVALIDr)){
            _SOC_VVERB((("%s(): no interrupt for blk=%d\n"), FUNCTION_NAME(), blk));
            i++;
            continue;
        }

        _SOC_VERB((("%s(): blk=%d, bi->number=%d,\n"), FUNCTION_NAME(), blk, bi->number));

        /* Read block interrupt register */
#ifdef BCM_ARAD_SUPPORT
       if(SOC_IS_ARAD(unit) &&  (bi->type == SOC_BLK_CLP || bi->type == SOC_BLK_XLP )) {
            int_port = SOC_BLOCK_PORT(unit, i);
        } else 
#endif /* BCM_ARAD_SUPPORT */
        {
        int_port =  bi->number;
        }
            
        rc = soc_reg_above_64_get(unit, interrupt_tree[blk].int_reg, int_port, interrupt_tree[blk].index, block_int_data);
        _SOC_IF_ERR_EXIT(rc);
        rc = soc_reg_above_64_get(unit, interrupt_tree[blk].int_mask_reg, int_port, interrupt_tree[blk].index, block_int_mask_data);
        _SOC_IF_ERR_EXIT(rc);

        /* Calc interrupt bit map according to  'flags' */
        SOC_REG_ABOVE_64_COPY(block_int_bitmap, block_int_data);
        if (is_unmasked_flag) {
            SOC_REG_ABOVE_64_AND(block_int_bitmap, block_int_mask_data);
        }
        _SOC_VERB((("%s(): block_int_data=0x%x, block_int_mask_data=0x%x, block_int_bitmap=0x%x,\n"), FUNCTION_NAME(), block_int_data[0], block_int_mask_data[0], block_int_bitmap[0]));

        for (; int_bit_idx < SOC_INTERRUPT_INTERRUPT_PER_REG_NUM_MAX; int_bit_idx++) {
            int_id = interrupt_tree[blk].int_id[int_bit_idx];
            if (int_id == -1) {
                _SOC_VERB((("%s(): Reached hidden interrupt. int_bit_idx=%d,\n"), FUNCTION_NAME(), int_bit_idx));
                continue;
            }

            vector_id = interrupts_arr[int_id].vector_id;
            if ((!(block_int_bitmap[0] & (1<<int_bit_idx))) && (!(vector_id == 1 && is_unmasked_flag == 0))) {
                _SOC_VVERB((("%s(): no interrupt for int_bit_idx=%d\n"), FUNCTION_NAME(), int_bit_idx));
                continue;
            }

#if !defined(SOC_NO_NAMES)
            _SOC_VERB((("%s(): \tgettind int_id: blk=%d, int_bit_idx=%d, int_id=%d, name=%s, vector_id=%d,\n"), FUNCTION_NAME(), blk, int_bit_idx, int_id, interrupts_arr[int_id].name, vector_id));
#else
            _SOC_VERB((("%s(): \tgettind int_id: blk=%d, int_bit_idx=%d, int_id=%d, vector_id=%d,\n"), FUNCTION_NAME(), blk, int_bit_idx, int_id, vector_id));
#endif

            /* Senity check between the interrupt field and the register bit */
            SOC_FIND_FIELD(interrupts_arr[int_id].field, SOC_REG_INFO(unit, interrupts_arr[int_id].reg).fields, SOC_REG_INFO(unit, interrupts_arr[int_id].reg).nFields, finfop);
            if ((finfop->len == 0x1) && (finfop->bp != int_bit_idx)) {
                _SOC_ERR((("%s(): Error: Where finfop->len=%d. finfop->bp=%d != int_bit_idx=%d.\n"), FUNCTION_NAME(), finfop->len, finfop->bp, int_bit_idx));
                _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No match between interrupt bit and interrupt field\n")));
            }

            /* handle vecor */
            if (vector_id == 0) {
                /* no vecotr - real int*/
                if (index < max_interrupts_size ) {

                    interrupts[index].index = int_port;
                    interrupts[index].id = int_id;
                    cont_prev_i_blk = i;
                    cont_prev_bit_idx = int_bit_idx;
                    cont_prev_vector_bit_idx = vector_int_bit_idx;
                    _SOC_VERB((("%s(): \tinterrupts[%d].index=%d, interrupts[%d].id=%d,\n"), FUNCTION_NAME(), index, interrupts[index].index, index, interrupts[index].id));
                }
                index++;
            } else {
                /* vecotr int  */
                rc = soc_reg_above_64_get(unit, interrupts_arr[int_id].vector_info->int_reg, int_port, interrupts_arr[int_id].vector_info->index, vector_int_data);
                _SOC_IF_ERR_EXIT(rc);
                rc = soc_reg_above_64_get(unit, interrupts_arr[int_id].vector_info->int_mask_reg, int_port, interrupts_arr[int_id].vector_info->index, vector_int_mask_data);
                _SOC_IF_ERR_EXIT(rc);

                /* Calc interrupt bit map according to  'flags' */
                SOC_REG_ABOVE_64_COPY(vector_int_bitmap, vector_int_data);
                if (is_unmasked_flag) {
                    SOC_REG_ABOVE_64_AND(vector_int_bitmap, vector_int_mask_data);
                }
                _SOC_VERB((("%s(): \t\tvector_int_data=0x%x, vector_int_mask_data=0x%x, vector_int_bitmap=0x%x,\n"), FUNCTION_NAME(), vector_int_data[0], vector_int_mask_data[0], vector_int_bitmap[0]));

                for (; vector_int_bit_idx < SOC_INTERRUPT_INTERRUPT_PER_REG_NUM_MAX; vector_int_bit_idx++) {
                    if (!(vector_int_bitmap[0] & (1<<vector_int_bit_idx))) {
                        _SOC_VVERB((("%s(): no interrupt for vector_int_bit_idx=%d\n"), FUNCTION_NAME(), vector_int_bit_idx));
                        continue;
                    }

                    vector_int_id = interrupts_arr[int_id].vector_info->int_id[vector_int_bit_idx];
                    if (vector_int_id == -1) {
                        _SOC_VERB((("%s(): Reached hidden interrupt. vector_int_bit_idx=%d,\n"), FUNCTION_NAME(), vector_int_bit_idx));
                        continue;
                    }

#if !defined(SOC_NO_NAMES)
                    _SOC_VERB((("%s(): \t\tblk=%d, int_id=%d, vector_int_bit_idx=%d, vector_int_id=%d, name=%s,\n"), FUNCTION_NAME(), blk, int_id, vector_int_bit_idx, vector_int_id, interrupts_arr[vector_int_id].name));
#else
                    _SOC_VERB((("%s(): \t\tblk=%d, int_id=%d, vector_int_bit_idx=%d, vector_int_id=%d,\n"), FUNCTION_NAME(), blk, int_id, vector_int_bit_idx, vector_int_id));
#endif

                    /* Senity check between the interrupt field and the register bit */
                    SOC_FIND_FIELD(interrupts_arr[vector_int_id].field, SOC_REG_INFO(unit, interrupts_arr[vector_int_id].reg).fields, SOC_REG_INFO(unit, interrupts_arr[vector_int_id].reg).nFields, finfop);
                    if ((finfop->len == 0x1) && (finfop->bp != vector_int_bit_idx)) {
                        _SOC_ERR((("%s(): Error: Where finfop->len=%d. finfop->bp=%d != vector_int_bit_idx=%d.\n"), FUNCTION_NAME(), finfop->len, finfop->bp, vector_int_bit_idx));
                        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No match between interrupt bit and interrupt field\n")));
                    }

                    if (index < max_interrupts_size ) {
                        interrupts[index].index = int_port;
                        interrupts[index].id = vector_int_id;
                        cont_prev_i_blk = i;
                        cont_prev_bit_idx = int_bit_idx;
                        cont_prev_vector_bit_idx = vector_int_bit_idx;
                        _SOC_VERB((("%s(): \t\tinterrupts[%d].index=%d, interrupts[%d].id=%d,\n"), FUNCTION_NAME(), index, interrupts[index].index, index, interrupts[index].id));
                    }
                    index++;
                }
                vector_int_bit_idx = 0;
            }
        }
        int_bit_idx = 0;
        i++;
    }

    *total_interrupts = index;
    _SOC_VERB((("%s(): index=%d, *total_interrupts=%d, cont_prev_i_blk=%d, cont_prev_bit_idx=%d, cont_prev_vector_bit_idx=%d.\n"), FUNCTION_NAME(), index, *total_interrupts, cont_prev_i_blk, cont_prev_bit_idx, cont_prev_vector_bit_idx));

exit:
    SOC_FUNC_RETURN;
}

int
soc_interrupt_info_get(int unit, int interrupt_id, soc_interrupt_db_t *inter)
{
    soc_interrupt_db_t *interrupts_arr;
    int nof_interrupts;

    SOC_INIT_FUNC_DEFS;
    
    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    interrupts_arr = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;

    _SOC_IF_ERR_EXIT(soc_nof_interrupts(unit, &nof_interrupts));
     if(interrupt_id > nof_interrupts) {
          _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("interrupt_id is unavail")));
    }

     SOC_NULL_CHECK(inter);
#if !defined(SOC_NO_NAMES)
    inter->name = interrupts_arr[interrupt_id].name;
#endif
    inter->reg = interrupts_arr[interrupt_id].reg;
    inter->reg_index = interrupts_arr[interrupt_id].reg_index;
    inter->field = interrupts_arr[interrupt_id].field;   
    inter->mask_reg = interrupts_arr[interrupt_id].mask_reg;
    inter->mask_reg_index = interrupts_arr[interrupt_id].mask_reg_index;
    inter->mask_field = interrupts_arr[interrupt_id].mask_field;
    inter->bit_in_field = interrupts_arr[interrupt_id].bit_in_field;
    inter->reg_test = interrupts_arr[interrupt_id].reg_test;

exit:
    SOC_FUNC_RETURN
}

int 
soc_get_interrupt_id(int unit, soc_reg_t reg, int reg_index, soc_field_t field, int bit_in_field, int* interrupt_id)
{
    soc_interrupt_db_t *interrupts_arr;
    int nof_interrupts;
    int i;

    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(interrupt_id);
    *interrupt_id = -1;

    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    interrupts_arr = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;

    /* look for interrupt id*/
    _SOC_IF_ERR_EXIT(soc_nof_interrupts(unit, &nof_interrupts));
    for(i=0; i < nof_interrupts; i++) {
        if(reg == interrupts_arr[i].reg && field == interrupts_arr[i].field && reg_index == interrupts_arr[i].reg_index) {

            if (interrupts_arr[i].bit_in_field == SOC_INTERRUPT_BIT_FIELD_DONT_CARE || 
                interrupts_arr[i].bit_in_field == bit_in_field) {
                *interrupt_id = i;
                break;
            }
        }
    }

    if(*interrupt_id == -1) {
        _SOC_EXIT_WITH_ERR(SOC_E_NOT_FOUND, (_SOC_MSG("interrupt ID was not found.\n")));
    }

exit:
    SOC_FUNC_RETURN
}


int
soc_get_interrupt_id_specific(int unit, int reg_adress, int reg_block, int field_bit, int* interrupt_id)
{
    soc_interrupt_db_t *interrupts_arr;
    int nof_interrupts;
    soc_field_info_t *finfop;
    int i, blk_indx;   
      
    SOC_INIT_FUNC_DEFS;

    *interrupt_id = -1;

    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    interrupts_arr = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;

    /* find the block index */    
    for(blk_indx = 0 ; SOC_BLOCK_INFO(unit, blk_indx).type >= 0; ++blk_indx ) { 
        if (SOC_INFO(unit).block_valid[blk_indx]) {
            if (reg_block == SOC_BLOCK_INFO(unit, blk_indx).cmic) {
                break;
            }
        }
    } 

    if(SOC_BLOCK_INFO(unit, blk_indx).type < 0) {
        _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Block number invalid"))); 
    }     

    _SOC_IF_ERR_EXIT(soc_nof_interrupts(unit, &nof_interrupts));

     /* look for interrupt id*/
    for(i=0; i < nof_interrupts; i++) {
        /* check block */
        if(interrupts_arr[i].reg == INVALIDr) {
            continue; 
        }

        if(SOC_BLOCK_INFO(unit, blk_indx).type != SOC_REG_INFO(unit, interrupts_arr[i].reg).block[0]) {
            continue;    
        }

        /* check address */
        if(reg_adress == (SOC_REG_INFO(unit, interrupts_arr[i].reg ).offset + interrupts_arr[i].reg_index)){


            SOC_FIND_FIELD( interrupts_arr[i].field,
                            SOC_REG_INFO(unit, interrupts_arr[i].reg).fields,
                            SOC_REG_INFO(unit, interrupts_arr[i].reg).nFields,
                            finfop); 
            
            if(!finfop) {
                continue;
            }
            /* check interrupt bit */
            if(interrupts_arr[i].bit_in_field == SOC_INTERRUPT_BIT_FIELD_DONT_CARE ) {
                if(field_bit != finfop->bp ) {
                    continue;
                }
            }else {
                if(field_bit != finfop->bp + interrupts_arr[i].bit_in_field ) {
                    continue;
                }
            }
                
            *interrupt_id = i;

            break; 
   
        }
    }

    if(*interrupt_id == -1) {
        _SOC_EXIT_WITH_ERR(SOC_E_NOT_FOUND, (_SOC_MSG("interrupt ID was not found.\n")));
    }

exit:
    SOC_FUNC_RETURN
}

/*number of interrupts per block instance*/
int
soc_nof_interrupts  (int unit, int* nof_interrupts) {
#if defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)
    int rc;
#endif
    SOC_INIT_FUNC_DEFS;
    /*default*/
    SOC_NULL_CHECK(nof_interrupts);

   *nof_interrupts = 0;
#ifdef BCM_DFE_SUPPORT
 if(SOC_IS_FE1600(unit)) {
    rc = soc_fe1600_nof_interrupts(unit, nof_interrupts);
    _SOC_IF_ERR_EXIT(rc);
 }
#endif
#ifdef BCM_ARAD_SUPPORT
 if(SOC_IS_ARAD(unit)) {
    rc = soc_arad_nof_interrupts(unit, nof_interrupts);
    _SOC_IF_ERR_EXIT(rc);
 }
#endif

exit:
   SOC_FUNC_RETURN;
}

int soc_interrupt_get_block_index_from_port(int unit, int interrupt_id, int port)
{
#ifdef BCM_ARAD_SUPPORT
    int index;
    soc_interrupt_db_t *interrupts_arr;
    soc_block_types_t block_types;

    
    interrupts_arr = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;

    block_types = SOC_REG_INFO(unit, interrupts_arr[interrupt_id].reg).block;

    if(SOC_IS_ARAD(unit)){
        if(SOC_BLOCK_IN_LIST(block_types, SOC_BLK_CLP)) {
            index = SOC_PORT_BLOCK_NUMBER(unit, port);
        } else if(SOC_BLOCK_IN_LIST(block_types, SOC_BLK_XLP)){
            index = SOC_PORT_BLOCK_NUMBER(unit, port) + SOC_MAX_NUM_CLP_BLKS;
        } else {
            index = port;
        }

        return index;
    }
#endif /* BCM_ARAD_SUPPORT */   

    return port;
}

int soc_interrupt_get_intr_port_from_index(int unit, int interrupt_id, int block_instance)
{
    int port=block_instance;
#ifdef BCM_ARAD_SUPPORT
    int bi_index;
    soc_interrupt_db_t *interrupts_arr;
    soc_block_types_t block_types;
    soc_block_info_t *bi;

    interrupts_arr = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;
    block_types = SOC_REG_INFO(unit, interrupts_arr[interrupt_id].reg).block;

     if(SOC_IS_ARAD(unit) && 
        (SOC_BLOCK_IN_LIST(block_types, SOC_BLK_CLP) || SOC_BLOCK_IN_LIST(block_types, SOC_BLK_XLP))) {

        /* find the block index */
        for (bi_index = 0;  SOC_BLOCK_INFO(unit, bi_index).type >= 0; bi_index++ ) {
                    
            bi = &(SOC_BLOCK_INFO(unit, bi_index));
            if(bi->type == block_types[0] && bi->number == block_instance) {
                break;
            }

        }
        
        port = SOC_BLOCK_PORT(unit, bi_index);

    }
    
#endif /* BCM_ARAD_SUPPORT */

    return port;

}

/* 
 * Interrupt aplication Functions - uses Soc DB 
 */ 
 
int soc_interrupt_flags_set(int unit, int interrupt_id, uint32 flags)
{

    SOC_INIT_FUNC_DEFS;
#ifdef BCM_ARAD_SUPPORT  
    if(SOC_IS_ARAD(unit)) { 
           
        _SOC_IF_ERR_EXIT(arad_sw_db_interrupts_flags_set(unit, interrupt_id, flags));

    } else 
#endif /* BCM_ARAD_SUPPORT */     
#ifdef BCM_DFE_SUPPORT     
    if(SOC_IS_DFE(unit)) {
    
        int nof_interrupts;
        
        _SOC_IF_ERR_EXIT(soc_dfe_nof_interrupts(unit, &nof_interrupts)); 
        
        if(interrupt_id < 0 || interrupt_id >= nof_interrupts) {
            _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Interrupt_id is out of range")));  
        }
        
        SOC_DFE_CONTROL(unit)->interrupts.interrupt_data[interrupt_id].flags = flags;

    } else
#endif /* BCM_DFE_SUPPORT */
    {
        _SOC_EXIT_WITH_ERR(SOC_E_UNIT, (_SOC_MSG("Unsupported in this unit type")));
    }
        
exit:
   SOC_FUNC_RETURN;
}

int soc_interrupt_flags_get(int unit, int interrupt_id, uint32* flags)
{
#if  defined ( BCM_ARAD_SUPPORT) || defined (BCM_DFE_SUPPORT) 
    int rc;
#endif    

    SOC_INIT_FUNC_DEFS;
 
    SOC_NULL_CHECK(flags);   
#ifdef BCM_ARAD_SUPPORT  
    if(SOC_IS_ARAD(unit)) { 
           
        rc =  arad_sw_db_interrupts_flags_get(unit, interrupt_id, flags);
        _SOC_IF_ERR_EXIT(rc);

    } else 
#endif /* BCM_ARAD_SUPPORT */
#ifdef BCM_DFE_SUPPORT
    if(SOC_IS_DFE(unit)) {
    
        int nof_interrupts;
        
        rc = soc_dfe_nof_interrupts(unit, &nof_interrupts); 
        _SOC_IF_ERR_EXIT(rc); 
        
        if(interrupt_id < 0 || interrupt_id >= nof_interrupts) {
            _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Interrupt_id is out of range")));  
        }
        
        *flags = SOC_DFE_CONTROL(unit)->interrupts.interrupt_data[interrupt_id].flags;

    } else 
#endif /* BCM_DFE_SUPPORT */
    {
        _SOC_EXIT_WITH_ERR(SOC_E_UNIT, (_SOC_MSG("Unsupported in this unit type")));
    }
        
exit:
   SOC_FUNC_RETURN;

}

int soc_interrupt_storm_timed_period_set(int unit, int interrupt_id, uint32 storm_timed_period)
{

    SOC_INIT_FUNC_DEFS;
#ifdef BCM_ARAD_SUPPORT    
    if(SOC_IS_ARAD(unit)) { 
           
        _SOC_IF_ERR_EXIT(arad_sw_db_interrupts_storm_timed_period_set(unit, interrupt_id, storm_timed_period));

    } else 
#endif /* BCM_ARAD_SUPPORT */
#ifdef BCM_DFE_SUPPORT
    if(SOC_IS_DFE(unit)) {
    
        int nof_interrupts;
        /* get num of interrupts */
        _SOC_IF_ERR_EXIT(soc_dfe_nof_interrupts(unit, &nof_interrupts)); 
        
        if(interrupt_id < 0 || interrupt_id >= nof_interrupts) {
            _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Interrupt_id is out of range")));  
        }
        
        SOC_DFE_CONTROL(unit)->interrupts.interrupt_data[interrupt_id].storm_timed_period = storm_timed_period;

    } else
#endif /* BCM_DFE_SUPPORT */
    {
        _SOC_EXIT_WITH_ERR(SOC_E_UNIT, (_SOC_MSG("Unsupported in this unit type")));
    }
        
exit:
   SOC_FUNC_RETURN;
}

int soc_interrupt_storm_timed_period_get(int unit, int interrupt_id, uint32* storm_timed_period)
{

    SOC_INIT_FUNC_DEFS;
 
    SOC_NULL_CHECK(storm_timed_period);   
#ifdef BCM_ARAD_SUPPORT
    if(SOC_IS_ARAD(unit)) { 
    
        _SOC_IF_ERR_EXIT(arad_sw_db_interrupts_storm_timed_period_get(unit, interrupt_id, storm_timed_period));

    } else 
#endif /* BCM_ARAD_SUPPORT */
#ifdef BCM_DFE_SUPPORT
    if(SOC_IS_DFE(unit)) {
    
        int nof_interrupts;
        
        _SOC_IF_ERR_EXIT(soc_dfe_nof_interrupts(unit, &nof_interrupts)); 
        
        if(interrupt_id < 0 || interrupt_id >= nof_interrupts) {
            _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Interrupt_id is out of range")));  
        }
        
        *storm_timed_period = SOC_DFE_CONTROL(unit)->interrupts.interrupt_data[interrupt_id].storm_timed_period;

    } else 
#endif /* BCM_DFE_SUPPORT */
    {
        _SOC_EXIT_WITH_ERR(SOC_E_UNIT, (_SOC_MSG("Unsupported in this unit type")));
    }
        
exit:
   SOC_FUNC_RETURN;
}

int soc_interrupt_storm_timed_count_set(int unit, int interrupt_id, uint32 storm_timed_count)
{

    SOC_INIT_FUNC_DEFS;
#ifdef  BCM_ARAD_SUPPORT     
    if(SOC_IS_ARAD(unit)) { 
           
        _SOC_IF_ERR_EXIT( arad_sw_db_interrupts_storm_timed_count_set(unit, interrupt_id, storm_timed_count));

    } else 
#endif /* BCM_ARAD_SUPPORT */
#ifdef BCM_DFE_SUPPORT
    if(SOC_IS_DFE(unit)) {
    
        int nof_interrupts;
        
        /* get num of interrupts */
        _SOC_IF_ERR_EXIT(soc_dfe_nof_interrupts(unit, &nof_interrupts)); 
        
        if(interrupt_id < 0 || interrupt_id >= nof_interrupts) {
            _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Interrupt_id is out of range")));  
        }
        
        SOC_DFE_CONTROL(unit)->interrupts.interrupt_data[interrupt_id].storm_timed_count = storm_timed_count;

    } else 
#endif /* BCM_DFE_SUPPORT */
    {
        _SOC_EXIT_WITH_ERR(SOC_E_UNIT, (_SOC_MSG("Unsupported in this unit type")));
    }
        
exit:
   SOC_FUNC_RETURN;
}

int soc_interrupt_storm_timed_count_get(int unit, int interrupt_id, uint32* storm_timed_count)
{

    SOC_INIT_FUNC_DEFS;
 
    SOC_NULL_CHECK(storm_timed_count);   
#ifdef  BCM_ARAD_SUPPORT 
    if(SOC_IS_ARAD(unit)) { 
           
        _SOC_IF_ERR_EXIT(arad_sw_db_interrupts_storm_timed_count_get(unit, interrupt_id, storm_timed_count));

    } else 
#endif /* BCM_ARAD_SUPPORT */
#ifdef BCM_DFE_SUPPORT
    if(SOC_IS_DFE(unit)) {
    
        int nof_interrupts;
        
        _SOC_IF_ERR_EXIT(soc_dfe_nof_interrupts(unit, &nof_interrupts)); 
        
        if(interrupt_id < 0 || interrupt_id >= nof_interrupts) {
            _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Interrupt_id is out of range")));  
        }
        
        *storm_timed_count =  SOC_DFE_CONTROL(unit)->interrupts.interrupt_data[interrupt_id].storm_timed_count;

    } else
#endif /* BCM_DFE_SUPPORT */
    {
        _SOC_EXIT_WITH_ERR(SOC_E_UNIT, (_SOC_MSG("Unsupported in this unit type")));
    }
        
exit:
   SOC_FUNC_RETURN;
}

int soc_interrupt_update_storm_detection(int unit, int block_instance, soc_interrupt_db_t *inter) 
{
    uint32 current_time, storm_timed_period, storm_timed_count;
    int inf_index ;

    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(inter);
    current_time = sal_time();

    inf_index = soc_interrupt_get_block_index_from_port(unit, inter->id, block_instance);  
    if(inf_index < 0) {
        _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Invalid parameters")));
    }
    
    _SOC_IF_ERR_EXIT(soc_interrupt_storm_timed_period_get(unit, inter->id, &storm_timed_period));
    
    _SOC_IF_ERR_EXIT(soc_interrupt_storm_timed_count_get(unit, inter->id, &storm_timed_count));

    if (storm_timed_count > 0 && storm_timed_period > 0) {

        if ((current_time - inter->storm_detection_start_time[inf_index]) > storm_timed_period) {
            inter->storm_detection_start_time[inf_index] = current_time;
            inter->storm_detection_occurrences[inf_index] = 0;
        }
        (inter->storm_detection_occurrences[inf_index])++;
    }

    if (SOC_SWITCH_EVENT_NOMINAL_STORM(unit) > 0) {
    
        if (inter->storm_nominal_count[inf_index] >= SOC_SWITCH_EVENT_NOMINAL_STORM(unit)) {
            inter->storm_nominal_count[inf_index] = 0;
        } else {
            (inter->storm_nominal_count[inf_index])++;

        }
    }

exit:
   SOC_FUNC_RETURN;
}

int soc_interrupt_is_storm(int unit, int block_instance, soc_interrupt_db_t *inter, int *is_storm_count_period,int *is_storm_nominal) {

    int inf_index = block_instance;
    uint32 storm_timed_count;

    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(inter);
    SOC_NULL_CHECK(is_storm_count_period);
    SOC_NULL_CHECK(is_storm_nominal);

    *is_storm_count_period = 0x0;
    *is_storm_nominal  = 0x0;
        
    inf_index = soc_interrupt_get_block_index_from_port(unit, inter->id, block_instance);
    if(inf_index < 0) {
        _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Invalid parameters")));
    }

    _SOC_IF_ERR_EXIT(soc_interrupt_storm_timed_count_get(unit, inter->id, &storm_timed_count));

    if ((storm_timed_count != 0x0) && (inter->storm_detection_occurrences[inf_index] >= storm_timed_count)) {
        inter->storm_detection_occurrences[inf_index] = 0x0;
        *is_storm_count_period = 0x1;
    }

    if ((SOC_SWITCH_EVENT_NOMINAL_STORM(unit) != 0x0) && (inter->storm_nominal_count[inf_index] >= SOC_SWITCH_EVENT_NOMINAL_STORM(unit))){
        inter->storm_nominal_count[inf_index] = 0x0;
        *is_storm_nominal = 0x1;
    }

exit:
   SOC_FUNC_RETURN;
}

int soc_interrupt_clear_all(int unit){
    int is_valid;
    int nof_interrupts;
    int inter;
    int bi_index, int_port;
    int rc;
    soc_block_info_t *bi;
    int is_on;
    soc_interrupt_db_t *interrupts_arr;

    SOC_INIT_FUNC_DEFS;
    
    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    interrupts_arr = SOC_CONTROL(unit)->interrupts_info->interrupt_db_info;

    soc_nof_interrupts(unit, &nof_interrupts);
    for (bi_index = 0;  SOC_BLOCK_INFO(unit, bi_index).type >= 0; bi_index++ ) {
        for (inter = 0; inter < nof_interrupts ; inter++) {

            if (!SOC_INFO(unit).block_valid[bi_index]) {
                continue;  
            }

            bi = &(SOC_BLOCK_INFO(unit, bi_index));
            if(NULL == bi) {
                _SOC_EXIT_WITH_ERR(SOC_E_FAIL,  (_SOC_MSG("Unknown block %d"),bi_index)); 
            }

            if (!SOC_INFO(unit).block_valid[bi_index]) {
                continue;
            }

            rc = soc_interrupt_is_valid(unit, bi, &(interrupts_arr[inter]), &is_valid);
            _SOC_IF_ERR_EXIT(rc);
            if (!is_valid) {
                continue;
            }

#ifdef BCM_ARAD_SUPPORT
       if(SOC_IS_ARAD(unit) &&  (bi->type == SOC_BLK_CLP || bi->type == SOC_BLK_XLP )) {
            int_port = SOC_BLOCK_PORT(unit, bi_index);
        } else 
#endif /* BCM_ARAD_SUPPORT */
            int_port = bi->number;

            rc = soc_interrupt_get(unit, int_port, &(interrupts_arr[inter]), &is_on );

            if (is_on) {
                if(NULL != interrupts_arr[inter].interrupt_clear) {
                    rc = interrupts_arr[inter].interrupt_clear(unit, int_port , inter);
                    _SOC_IF_ERR_EXIT(rc);
                }
            }
        }
    }

exit:    
    SOC_FUNC_RETURN;
}

int soc_interrupt_is_all_clear(int unit, int *is_all_clear){
    soc_interrupt_cause_t interrupt;
    int total = 0;
    int rc;

    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(is_all_clear);

    rc  = soc_active_interrupts_get(unit, 0x0 ,1, &interrupt, &total);
    _SOC_IF_ERR_EXIT(rc);

    *is_all_clear = (total == 0);
exit:
    SOC_FUNC_RETURN;
}

int soc_interrupt_is_all_mask(int unit, int *is_all_mask){
    uint32 high_mask = 0;
    uint32 low_mask  = 0;

    SOC_INIT_FUNC_DEFS;

    SOC_NULL_CHECK(is_all_mask);

    if (SOC_IS_FE1600(unit)) {
        low_mask = SOC_IRQ1_MASK(unit);
        high_mask = SOC_IRQ2_MASK(unit);
    }
    else if (SOC_IS_ARAD(unit)) {
#ifdef BCM_CMICM_SUPPORT
        low_mask = SOC_CMCx_IRQ3_MASK(unit, SOC_PCI_CMC(unit));
        high_mask =  SOC_CMCx_IRQ4_MASK(unit, SOC_PCI_CMC(unit));
#endif
    }

    *is_all_mask = ((low_mask == 0x0) && (high_mask == 0x0));

exit:
    SOC_FUNC_RETURN;
}

/* 
 * Statistics functions
 */
int soc_interrupt_stat_cnt_increase(int unit, int bi,  int interrupt_id)
{
    int nof_interrupts;
    soc_interrupt_db_t *intr_id_db;

    SOC_INIT_FUNC_DEFS;

    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    /*verify interrupt_id*/
    soc_nof_interrupts(unit, &nof_interrupts);
    if ((interrupt_id > nof_interrupts) || interrupt_id < 0) {
          _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("event_id is unavail")));
    }

    /* Get specific interrupt soc db */
    intr_id_db = &(SOC_CONTROL(unit)->interrupts_info->interrupt_db_info[interrupt_id]);

    /* Increase statistics count */
    (intr_id_db->statistics_count[bi]) ++;
  
exit:
    SOC_FUNC_RETURN;
}

/*
 * Sort interrupts according to priority 
 */
int soc_sort_interrupts_according_to_priority(int unit, soc_interrupt_cause_t* interrupts, uint32 interrupts_size)
{

    int i,j;
    int left_interrupt_priority,right_interrupt_priority;
    int stop_check_flag;
    uint32 left_intr_flags, right_intr_flags;
    soc_interrupt_cause_t tmp;

    SOC_INIT_FUNC_DEFS;

    if(!SOC_INTR_IS_SUPPORTED(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("No interrupts for device"))); 
    }

    SOC_NULL_CHECK(interrupts);
    
    for(i=interrupts_size-2 ;i>=0;--i) {
     
        stop_check_flag=1;
          
        for(j=0;j<=i;j++) {  

            _SOC_IF_ERR_EXIT(soc_interrupt_flags_get(unit, interrupts[j].id, &left_intr_flags));            
            _SOC_IF_ERR_EXIT(soc_interrupt_flags_get(unit, interrupts[j+1].id, &right_intr_flags));
            left_interrupt_priority = ((left_intr_flags & SOC_INTERRUPT_DB_FLAGS_PRIORITY_MASK) >> SOC_INTERRUPT_DB_FLAGS_PRIORITY_BITS_LSB);
            right_interrupt_priority = ((right_intr_flags & SOC_INTERRUPT_DB_FLAGS_PRIORITY_MASK) >> SOC_INTERRUPT_DB_FLAGS_PRIORITY_BITS_LSB);
             
            if(left_interrupt_priority > right_interrupt_priority) {   
                tmp = interrupts[j];  
                interrupts[j] = interrupts[j+1];
                interrupts[j+1] = tmp;
                stop_check_flag = 0;  
            }   
        }     

        if(stop_check_flag == 1) {
            break;
        }
    }   

exit:
    SOC_FUNC_RETURN;
}

#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_DFE_SUPPORT)|| defined(BCM_PETRA_SUPPORT) */


