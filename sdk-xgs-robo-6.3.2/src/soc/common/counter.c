/*

 * $Id: counter.c 1.227.2.1 Broadcom SDK $

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
 * Packet Statistics Counter Management
 *
 * Firebolt     1024 bytes per port (128 * uint64)
 * Helix        1024 bytes per port (128 * uint64)
 * Felix        1024 bytes per port (128 * uint64)
 * Raptor       1024 bytes per port (128 * uint64)
 * Hercules:    1024 bytes per port (128 * uint64)
 * HUMV:        2048 bytes per port (256 * uint64)
 * Bradley:     2048 bytes per port (256 * uint64)
 * Goldwing:    2048 bytes per port (256 * uint64)
 * Triumph:     4096 bytes per port (512 * uint64)
 *
 * BCM5670 Endian: on Hercules, the well-intentioned ES_BIG_ENDIAN_OTHER
 * causes counter DMA to write the most significant word first in the
 * DMA buffer.  This is wrong because not all big-endian hosts have the
 * same setting for ES_BIG_ENDIAN_OTHER (e.g. mousse and idtrp334).
 * This problem makes it necessary to check the endian select to
 * determine whether to swap words.
 */

#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>
#include <sal/core/sync.h>
#include <sal/core/time.h>

#include <soc/drv.h>
#include <soc/counter.h>
#include <soc/ll.h>
#include <soc/debug.h>
#include <soc/mem.h>
#include <soc/register.h>
#ifdef BCM_TRIDENT_SUPPORT
#include <soc/trident.h>
#endif
#if defined(BCM_PETRAB_SUPPORT)
#include <soc/dpp/Petra/PB_TM/pb_api_nif.h>
#endif
#if defined(BCM_PETRA_SUPPORT)
#include <soc/dpp/Petra/petra_api_ports.h>
#include <soc/dpp/drv.h>
#include <bcm_int/dpp/error.h>
#include <bcm_int/dpp/utils.h>
#endif
#if defined(BCM_DFE_SUPPORT)
#include <soc/dfe/cmn/dfe_drv.h>
#endif
#if defined(BCM_ARAD_SUPPORT)
#include <soc/dpp/ARAD/arad_stat.h>
#endif
#if defined(BCM_88750_SUPPORT)
#include <soc/dfe/fe1600/fe1600_stat.h>
#endif
#if defined(BCM_ARAD_SUPPORT) || defined(BCM_PETRAB_SUPPORT)
#include <soc/dpp/mbcm.h>
#endif
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/trident2.h>
#endif
#if defined(BCM_KATANA2_SUPPORT)
#include <soc/katana2.h>
#endif

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)

#define COUNTER_IDX_PORTBASE(unit, port) \
        ((port) * SOC_CONTROL(unit)->counter_perport)

#define COUNTER_IDX_GET(unit, ctr_ref, port) \
        (COUNTER_IDX_PORTBASE(unit, port) + \
         SOC_REG_CTR_IDX(unit, (ctr_ref)->reg) + (ctr_ref)->index)

#define COUNTER_IDX_OFFSET(unit)        0

#define COUNTER_MIN_IDX_GET(unit, port) \
        (COUNTER_IDX_PORTBASE(unit, port) + COUNTER_IDX_OFFSET(unit))

static soc_counter_extra_f soc_counter_extra[SOC_MAX_NUM_DEVICES]
                                            [SOC_COUNTER_EXTRA_CB_MAX];
/* Number of cosq per unit per port */
static int num_cosq[SOC_MAX_NUM_DEVICES][SOC_MAX_NUM_PORTS] = {{ 0 }};

#ifdef BCM_SBUSDMA_SUPPORT
volatile uint32 _soc_counter_pending[SOC_MAX_NUM_DEVICES] = { 0 };
static uint32 soc_counter_mib_offset[SOC_MAX_NUM_DEVICES] = { 0 };
#endif

STATIC int
soc_counter_collect64(int unit, int discard, soc_port_t tmp_port, soc_reg_t hw_ctr_reg);

/*
 * Turn on COUNTER_BENCH and "debug +verbose" to benchmark the CPU time
 * spent on counter activity.  NOTE: most useful on platforms where
 * sal_time_usecs() has usec precision instead of clock tick precision.
 */

#undef COUNTER_BENCH

/* Per port mapping to counter map structures */
#define PORT_CTR_REG(unit, port, idx) \
        (&SOC_CONTROL(unit)->counter_map[port]->cmap_base[idx])
#define PORT_CTR_NUM(unit, port) \
        (SOC_CONTROL(unit)->counter_map[port]->cmap_size)

#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRX_SUPPORT)

static uint64 *soc_counter_tbuf[SOC_MAX_NUM_DEVICES];
#define SOC_COUNTER_TBUF_SIZE(unit) \
    (SOC_CONTROL(unit)->counter_perport * sizeof(uint64))

#ifdef BCM_TRIUMPH2_SUPPORT
#define COUNTER_TIMESTAMP_FIFO_SIZE     4
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
typedef struct _kt_cosq_counter_mem_map_s {
    soc_mem_t mem;
    soc_counter_non_dma_id_t non_dma_id;
}_kt_cosq_counter_mem_map_t;

/* 
 * CTR_FLEX_COUNT_0/1/2/3 = Total ENQ discarded 
 * CTR_FLEX_COUNT_4/5/6/7  = Red packet ENQ discarded 
 * CTR_FLEX_COUNT_8/9/10/11  = Total DEQ
 */ 
_kt_cosq_counter_mem_map_t _kt_cosq_mem_map[] = 
{
    { CTR_FLEX_COUNT_0m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT },
    { CTR_FLEX_COUNT_1m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT },    
    { CTR_FLEX_COUNT_2m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT },
    { CTR_FLEX_COUNT_3m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT },    
    { CTR_FLEX_COUNT_4m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_RED },
    { CTR_FLEX_COUNT_5m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_RED },    
    { CTR_FLEX_COUNT_6m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_RED },
    { CTR_FLEX_COUNT_7m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_RED },
    { CTR_FLEX_COUNT_8m, SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT },
    { CTR_FLEX_COUNT_9m, SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT },
    { CTR_FLEX_COUNT_10m, SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT },
    { CTR_FLEX_COUNT_11m, SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT }
};

/* 
 * CTR_FLEX_COUNT_0 = Total ENQ discarded 
 * CTR_FLEX_COUNT_1 = Total ENQ Accepted 
 * CTR_FLEX_COUNT_2 = Green ENQ discarded 
 * CTR_FLEX_COUNT_3 = Yellow ENQ discarded
 * CTR_FLEX_COUNT_4    = Red packet ENQ discarded 
 * CTR_FLEX_COUNT_5 = Green ENQ Accepted
 * CTR_FLEX_COUNT_6 = Yellow ENQ Accepted
 * CTR_FLEX_COUNT_7 = Red ENQ Accepted
 * CTR_FLEX_COUNT_8    = Total DEQ
*/
_kt_cosq_counter_mem_map_t _kt_cosq_gport_mem_map[] = 
{
    { CTR_FLEX_COUNT_0m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT },
    { CTR_FLEX_COUNT_1m, SOC_COUNTER_NON_DMA_COSQ_ACCEPT_PKT },    
    { CTR_FLEX_COUNT_2m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_GREEN },
    { CTR_FLEX_COUNT_3m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_YELLOW },    
    { CTR_FLEX_COUNT_4m, SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_RED },
    { CTR_FLEX_COUNT_5m, SOC_COUNTER_NON_DMA_COSQ_ACCEPT_PKT_GREEN },    
    { CTR_FLEX_COUNT_6m, SOC_COUNTER_NON_DMA_COSQ_ACCEPT_PKT_YELLOW },
    { CTR_FLEX_COUNT_7m, SOC_COUNTER_NON_DMA_COSQ_ACCEPT_PKT_RED },
    { CTR_FLEX_COUNT_8m, SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT }
};
#endif
int
is_xaui_rx_counter(soc_reg_t ctr_reg)
{
    switch (ctr_reg) {
    case IR64r:
    case IR127r:
    case IR255r:
    case IR511r:
    case IR1023r:
    case IR1518r:
    case IR2047r:
    case IR4095r:
    case IR9216r:
    case IR16383r:
    case IRBCAr:
    case IRBYTr:
    case IRERBYTr:
    case IRERPKTr:
    case IRFCSr:
    case IRFLRr:
    case IRFRGr:
    case IRJBRr:
    case IRJUNKr:
    case IRMAXr:
    case IRMCAr:
    case IRMEBr:
    case IRMEGr:
    case IROVRr:
    case IRPKTr:
    case IRUNDr:
    case IRXCFr:
    case IRXPFr:
    case IRXUOr:
#ifdef BCM_TRX_SUPPORT
    case IRUCr:
#ifdef BCM_ENDURO_SUPPORT
    case IRUCAr:
#endif /* BCM_ENDURO_SUPPORT */
    case IRPOKr:
    case MAC_RXLLFCMSGCNTr:
#endif
#if defined(BCM_SCORPION_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT)
    case IRXPPr:
#endif
        return 1;
    default:
        break;
    }
    return 0;
}
#endif /* BCM_BRADLEY_SUPPORT || BCM_TRX_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
STATIC int
_soc_counter_trident_get_info(int unit, soc_port_t port, soc_reg_t id,
                              int *base_index, int *num_entries)
{
    soc_control_t *soc;
    soc_info_t *si;
    soc_counter_non_dma_t *non_dma;
    soc_port_t phy_port, mmu_port;
    soc_port_t mmu_cmic_port, mmu_lb_port;

    soc = SOC_CONTROL(unit);
    non_dma = &soc->counter_non_dma[id - SOC_COUNTER_NON_DMA_START];

    if (!(non_dma->flags & _SOC_COUNTER_NON_DMA_VALID)) {
        return SOC_E_UNAVAIL;
    }

    if (port < 0) {
        return SOC_E_PARAM;
    }

    si = &SOC_INFO(unit);
    if (si->port_l2p_mapping[port] == -1) {
        *base_index = 0;
        *num_entries = 0;
        return SOC_E_NONE;
    }

    mmu_cmic_port = si->port_p2m_mapping[si->port_l2p_mapping[si->cmic_port]];
    mmu_lb_port = si->port_p2m_mapping[si->port_l2p_mapping[si->lb_port]];

    phy_port = si->port_l2p_mapping[port];
    mmu_port = si->port_p2m_mapping[phy_port];

    switch (id) {
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT:
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE:
        if (SOC_PBMP_MEMBER(si->xpipe_pbm, port)) { /* in X pipe */
            *base_index = si->port_cosq_base[port];
        } else { /* in Y pipe */
            *base_index = si->port_cosq_base[port] +
                non_dma->dma_index_max[0] + 1;
        }
        *num_entries = si->port_num_cosq[port];
        break;
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_UC:
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_UC:
        if (SOC_PBMP_MEMBER(si->xpipe_pbm, port)) { /* in X pipe */
            *base_index = si->port_uc_cosq_base[port];
        } else { /* in Y pipe */
            *base_index = si->port_uc_cosq_base[port] +
                non_dma->dma_index_max[0] + 1;
        }
        *num_entries = si->port_num_uc_cosq[port];
        break;
    case SOC_COUNTER_NON_DMA_MMU_QCN_CNM:
        if((mmu_port == mmu_lb_port) || (mmu_port == mmu_cmic_port)){
            *num_entries = 0;
            *base_index = 0;  
        }else{
            *num_entries = 2;/* 2 QCN queue per port */
            *base_index = port * (*num_entries);
        }     
        break;
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_EXT:
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_EXT:
        if (SOC_PBMP_MEMBER(si->xpipe_pbm, port)) { /* in X pipe */
            *base_index = si->port_ext_cosq_base[port];
        } else { /* in Y pipe */
            *base_index = si->port_ext_cosq_base[port] +
                non_dma->dma_index_max[0] + 1;
        }
        *num_entries = si->port_num_ext_cosq[port];
        break;
    case SOC_COUNTER_NON_DMA_COSQ_DROP_PKT:
    case SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE:
        if (mmu_port == mmu_cmic_port) {
            *base_index = 0;
            *num_entries = 48;
        } else {
            *base_index = 48 + (mmu_port - mmu_cmic_port - 1) * 5;
            *num_entries = 5;
        }
        break;
    case SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC:
    case SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE_UC:
        if (mmu_port < mmu_lb_port) { /* in X pipe */
            if (mmu_port == mmu_cmic_port) {
                *base_index = 0;
                *num_entries = 0;
            } else if (mmu_port <= mmu_cmic_port + 4) {
                *base_index = (mmu_port - mmu_cmic_port - 1) * 74;
                *num_entries = 74; /* 10 ucast + 64 ext ucast */
            } else {
                *base_index = 74 * 4 + (mmu_port - mmu_cmic_port - 5) * 10;
                *num_entries = 10;
            }
        } else { /* in Y pipe */
            if (mmu_port == mmu_lb_port) {
                *base_index = 0;
                *num_entries = 0;
            } else if (mmu_port <= mmu_lb_port + 4) {
                *base_index = 576 + (mmu_port - mmu_lb_port - 1) * 74;
                *num_entries = 74; /* 10 ucast + 64 ext ucast */
            } else {
                *base_index = 576 + 74 * 4 + (mmu_port - mmu_lb_port - 5) * 10;
                *num_entries = 10;
            }
        }
        break;
    case SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING:
    case SOC_COUNTER_NON_DMA_PORT_DROP_BYTE_ING:
        *num_entries = 1;
        *base_index = port;
        break;
    case SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW:
    case SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED:
    case SOC_COUNTER_NON_DMA_PORT_WRED_PKT_GREEN:
    case SOC_COUNTER_NON_DMA_PORT_WRED_PKT_YELLOW:
    case SOC_COUNTER_NON_DMA_PORT_WRED_PKT_RED:
        *num_entries = 1;
        *base_index = mmu_port;
        break;
    case SOC_COUNTER_NON_DMA_PG_MIN_PEAK:
    case SOC_COUNTER_NON_DMA_PG_MIN_CURRENT:
    case SOC_COUNTER_NON_DMA_PG_SHARED_PEAK:
    case SOC_COUNTER_NON_DMA_PG_SHARED_CURRENT:
    case SOC_COUNTER_NON_DMA_PG_HDRM_PEAK:
    case SOC_COUNTER_NON_DMA_PG_HDRM_CURRENT:
        *num_entries = 8;
        *base_index = port * (*num_entries);
        break;
    case SOC_COUNTER_NON_DMA_QUEUE_PEAK:
    case SOC_COUNTER_NON_DMA_QUEUE_CURRENT:
        *num_entries = 5;
        *base_index = port * (*num_entries);
        break;
    case SOC_COUNTER_NON_DMA_UC_QUEUE_PEAK:
    case SOC_COUNTER_NON_DMA_UC_QUEUE_CURRENT:
        *num_entries = 10;
        *base_index = port * (*num_entries);
        break;
    case SOC_COUNTER_NON_DMA_EXT_QUEUE_PEAK:
    case SOC_COUNTER_NON_DMA_EXT_QUEUE_CURRENT:
        *num_entries = 64;
        *base_index = port * (*num_entries);
        break;
    default:
        return SOC_E_INTERNAL;
    }
    *base_index += non_dma->base_index;

    return SOC_E_NONE;
}
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
STATIC int
_soc_counter_trident2_get_info(int unit, soc_port_t port, soc_reg_t id,
                              int *base_index, int *num_entries)
{
    soc_info_t *si;
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma;
    soc_port_t phy_port, mmu_port;
    int pipe;

    soc = SOC_CONTROL(unit);
    non_dma = &soc->counter_non_dma[id - SOC_COUNTER_NON_DMA_START];

    if (!(non_dma->flags & _SOC_COUNTER_NON_DMA_VALID)) {
        return SOC_E_UNAVAIL;
    }

    si = &SOC_INFO(unit);
    if (port >= 0) {
        if (si->port_l2p_mapping[port] == -1) {
            *base_index = 0;
            *num_entries = 0;
            return SOC_E_NONE;
        }

        phy_port = si->port_l2p_mapping[port];
        mmu_port = si->port_p2m_mapping[phy_port];
        pipe = (mmu_port >= 64) ? 1 : 0;
    } else {
        phy_port = mmu_port = -1;
        pipe = -1;
    }
    COMPILER_REFERENCE(pipe);

    switch (id) {
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT:
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE:
        if (mmu_port >= 0) {
            *base_index =
                soc_td2_logical_qnum_hw_qnum(unit, port,
                                             si->port_cosq_base[port], 0);
            *num_entries = si->port_num_cosq[port];
        } else {
            *base_index = 0;
            *num_entries = non_dma->num_entries;
        }
        break;
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_UC:
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_UC:
        if (mmu_port >= 0) {
            *base_index =
                soc_td2_logical_qnum_hw_qnum(unit, port,
                                             si->port_uc_cosq_base[port], 1);
            *num_entries = si->port_num_uc_cosq[port];
        } else {
            *base_index = 0;
            *num_entries = non_dma->num_entries;
        }
        break;
    case SOC_COUNTER_NON_DMA_COSQ_DROP_PKT:
    case SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE:
    case SOC_COUNTER_NON_DMA_QUEUE_PEAK:
    case SOC_COUNTER_NON_DMA_QUEUE_CURRENT:
        if (mmu_port >= 0) {
            *base_index = si->port_cosq_base[port];
            *num_entries = si->port_num_cosq[port];
        } else {
            *base_index = 0;
            *num_entries = non_dma->num_entries;
        }
        break;
    case SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC:
    case SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE_UC:
    case SOC_COUNTER_NON_DMA_UC_QUEUE_PEAK:
    case SOC_COUNTER_NON_DMA_UC_QUEUE_CURRENT:
        if (mmu_port >= 0) {
            *base_index = si->port_uc_cosq_base[port];
            *num_entries = si->port_num_uc_cosq[port];
        } else {
            *base_index = 0;
            *num_entries = non_dma->num_entries;
        }
        break;
    case SOC_COUNTER_NON_DMA_MMU_QCN_CNM:        
        *base_index = 0;     
        *num_entries = non_dma->num_entries;
        break;
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_EXT:
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_EXT:
        if (mmu_port >= 0) {
            *base_index = si->port_ext_cosq_base[port];
            *num_entries = si->port_num_ext_cosq[port];
        } else {
            *base_index = 0;
            *num_entries = non_dma->num_entries;
        }
        break;
    case SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING:
    case SOC_COUNTER_NON_DMA_PORT_DROP_BYTE_ING:
    case SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING_METER:
    case SOC_COUNTER_NON_DMA_PORT_DROP_BYTE_ING_METER:
        if (mmu_port >= 0) {
            *num_entries = non_dma->entries_per_port;
            *base_index = mmu_port;
        } else {
            return SOC_E_INTERNAL;
        }
        break;
    case SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW:
    case SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED:
    case SOC_COUNTER_NON_DMA_PORT_WRED_PKT_GREEN:
    case SOC_COUNTER_NON_DMA_PORT_WRED_PKT_YELLOW:
    case SOC_COUNTER_NON_DMA_PORT_WRED_PKT_RED:
        if (mmu_port >= 0) {
            if (SOC_PBMP_MEMBER(si->xpipe_pbm, port)) { /* in X pipe */
                *base_index = (mmu_port & 0x3f) * non_dma->entries_per_port;
            } else { /* in Y pipe */
                *base_index = (mmu_port & 0x3f) * non_dma->entries_per_port +
                    (non_dma->num_entries / 2);
            }
            *num_entries = non_dma->entries_per_port;
        } else {
            return SOC_E_INTERNAL;
        }
        break;
    case SOC_COUNTER_NON_DMA_PG_MIN_PEAK:
    case SOC_COUNTER_NON_DMA_PG_MIN_CURRENT:
    case SOC_COUNTER_NON_DMA_PG_SHARED_PEAK:
    case SOC_COUNTER_NON_DMA_PG_SHARED_CURRENT:
    case SOC_COUNTER_NON_DMA_PG_HDRM_PEAK:
    case SOC_COUNTER_NON_DMA_PG_HDRM_CURRENT:
        if (mmu_port >= 0) {
            if (SOC_PBMP_MEMBER(si->xpipe_pbm, port)) { /* in X pipe */
                *base_index = (mmu_port & 0x3f) * non_dma->entries_per_port;
            } else { /* in Y pipe */
                *base_index = (mmu_port & 0x3f) * non_dma->entries_per_port +
                    non_dma->dma_index_max[0] + 1;
            }
            *num_entries = non_dma->entries_per_port;
        } else {
            return SOC_E_INTERNAL;
        }
        break;
    case SOC_COUNTER_NON_DMA_DROP_RQ_PKT:
    case SOC_COUNTER_NON_DMA_DROP_RQ_BYTE:
    case SOC_COUNTER_NON_DMA_DROP_RQ_PKT_YELLOW:
    case SOC_COUNTER_NON_DMA_DROP_RQ_PKT_RED:
    case SOC_COUNTER_NON_DMA_POOL_PEAK:
    case SOC_COUNTER_NON_DMA_POOL_CURRENT:
        *base_index = 0;
        *num_entries = non_dma->num_entries;
        break;
    default:
        return SOC_E_INTERNAL;
    }
    *base_index += non_dma->base_index;

    return SOC_E_NONE;
}
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
STATIC int
_soc_counter_triumph3_get_info(int unit, soc_port_t port, soc_reg_t id,
                              int *base_index, int *num_entries)
{
    soc_info_t *si;
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma;
    soc_port_t phy_port, mmu_port;

    soc = SOC_CONTROL(unit);
    non_dma = &soc->counter_non_dma[id - SOC_COUNTER_NON_DMA_START];

    if (!(non_dma->flags & _SOC_COUNTER_NON_DMA_VALID)) {
        return SOC_E_UNAVAIL;
    }

    si = &SOC_INFO(unit);
    if (port >= 0) {
        phy_port = si->port_l2p_mapping[port];
        mmu_port = si->port_p2m_mapping[phy_port];
    } else {
        phy_port = mmu_port = -1;
    }

    switch (id) {
    case SOC_COUNTER_NON_DMA_COSQ_DROP_PKT:
    case SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE:
        if (mmu_port >= 0) {
            if (mmu_port <= 39) {
                *base_index = mmu_port * 8;
                *num_entries = 8;
            } else if (mmu_port <= 55) {
                *base_index = 320 + (mmu_port - 40) * 10;
                *num_entries = 10;
            } else if (mmu_port == 56) {
                *base_index = 0x1e0;
                *num_entries = 8;
            } else if (mmu_port == 57) {
                *num_entries = 0;
            } else if (mmu_port == 58) {
                *base_index = 0x1e8;
                *num_entries = 8;
            } else if (mmu_port == 59) {
                *base_index = 0x200;
                *num_entries = 48;
            } else if (mmu_port == 60) {
                *base_index = 0x1f0;
                *num_entries = 8;
            } else if (mmu_port == 61) {
                *base_index = 0x1f8;
                *num_entries = 1;
            } else {
                return SOC_E_PARAM;
            }
        } else {
            *base_index = 0;
            *num_entries = 560;
        }
        break;
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT:
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE:
        if (mmu_port >= 0) {
            *base_index = si->port_cosq_base[port];
            *num_entries = si->port_num_cosq[port];
        } else {
            *base_index = 0;
            *num_entries = 1592;
        }
        break;
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_UC:
    case SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_UC:
        if (mmu_port >= 0) {
            *base_index = si->port_uc_cosq_base[port];
            *num_entries = si->port_num_uc_cosq[port];
        } else {
            *base_index = 0;
            *num_entries = 1592;
        }
        break;
    case SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC:
    case SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE_UC:
    case SOC_COUNTER_NON_DMA_UC_QUEUE_PEAK:
    case SOC_COUNTER_NON_DMA_UC_QUEUE_CURRENT:
        if (mmu_port >= 0) {
            *base_index = si->port_uc_cosq_base[port];
            *num_entries = si->port_num_uc_cosq[port];
        } else {
            *base_index = 0;
            *num_entries = 1024;
        }
        break;
    case SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING:
    case SOC_COUNTER_NON_DMA_PORT_DROP_BYTE_ING:
        *num_entries = 1;
        *base_index = port;
        break;
    case SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW:
    case SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED:
    case SOC_COUNTER_NON_DMA_PORT_WRED_PKT_GREEN:
    case SOC_COUNTER_NON_DMA_PORT_WRED_PKT_YELLOW:
    case SOC_COUNTER_NON_DMA_PORT_WRED_PKT_RED:
        if (mmu_port >= 0) {
            *num_entries = 1;
            *base_index = mmu_port;
        } else {
            return SOC_E_INTERNAL;
        }
        break;
    case SOC_COUNTER_NON_DMA_PG_MIN_PEAK:
    case SOC_COUNTER_NON_DMA_PG_MIN_CURRENT:
    case SOC_COUNTER_NON_DMA_PG_SHARED_PEAK:
    case SOC_COUNTER_NON_DMA_PG_SHARED_CURRENT:
    case SOC_COUNTER_NON_DMA_PG_HDRM_PEAK:
    case SOC_COUNTER_NON_DMA_PG_HDRM_CURRENT:
        if (mmu_port >= 0) {
            *num_entries = 8;
            *base_index = mmu_port * (*num_entries);
        } else {
            *base_index = 0;
            *num_entries = 504;
        }
        break;
    case SOC_COUNTER_NON_DMA_QUEUE_PEAK:
    case SOC_COUNTER_NON_DMA_QUEUE_CURRENT:
        *num_entries = 48;
        *base_index = 0;
        break;
    default:
        return SOC_E_INTERNAL;
    }
    *base_index += non_dma->base_index;

    return SOC_E_NONE;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
STATIC int
_soc_counter_katana_get_info(int unit, soc_port_t port, soc_reg_t id,
                              int *base_index, int *num_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma;
    
    soc = SOC_CONTROL(unit);
    non_dma = &soc->counter_non_dma[id - SOC_COUNTER_NON_DMA_START];

    if (!(non_dma->flags & _SOC_COUNTER_NON_DMA_VALID)) {
        return SOC_E_UNAVAIL;
    }

    if (port >= 0 ) {
        if (port < SOC_MAX_NUM_PORTS) {
            *base_index = SOC_INFO(unit).port_cosq_base[port];
            *num_entries = num_cosq[unit][port];
            *base_index += non_dma->base_index;
        } else {
            /* extended queues */
            *base_index = port;
            *num_entries = non_dma->num_entries;
            *base_index += non_dma->base_index;
        }
    }

    return SOC_E_NONE;
}
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
STATIC int
_soc_counter_fb_get_info(int unit, soc_port_t port, soc_reg_t id,
                         int *base_index, int *num_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma;
    int i;

    soc = SOC_CONTROL(unit);
    non_dma = &soc->counter_non_dma[id - SOC_COUNTER_NON_DMA_START];

    if (!(non_dma->flags & _SOC_COUNTER_NON_DMA_VALID)) {
        return SOC_E_UNAVAIL;
    }

    if (non_dma->entries_per_port == 1) {
        /* This non-dma counter is per port */
        *base_index = non_dma->base_index + port;
        *num_entries = 1;
    } else {
        if (SOC_IS_SC_CQ(unit)) {
            /* This non-dma counter is per cosq */
            if (non_dma->flags & _SOC_COUNTER_NON_DMA_PERQ_REG) {
                *base_index = non_dma->base_index;
                for (i = 0; i < SOC_MAX_NUM_PORTS; i++) {
                    if (i == port) {
                        break;
                    }
                    *base_index += num_cosq[unit][i];
                }
                *num_entries = num_cosq[unit][port];
            } else {
                /* This non-dma counter is per uc/mc q */
                *base_index = non_dma->base_index +
                              (non_dma->entries_per_port * port);
                *num_entries = non_dma->entries_per_port;
            }
        } else {
            *base_index = non_dma->base_index;
            for (i = 0; i < SOC_MAX_NUM_PORTS; i++) {
                if (i == port) {
                    break;
                }
                *base_index += num_cosq[unit][i];
            }
            *num_entries = num_cosq[unit][port];
        }
    }

    return SOC_E_NONE;
}
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
STATIC int
_soc_counter_shadow_get_info(int unit, soc_port_t port, soc_reg_t id,
                         int *base_index, int *num_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma;

    soc = SOC_CONTROL(unit);
    non_dma = &soc->counter_non_dma[id - SOC_COUNTER_NON_DMA_START];

    if (!(non_dma->flags & _SOC_COUNTER_NON_DMA_VALID)) {
        return SOC_E_UNAVAIL;
    }

    *base_index = non_dma->base_index;
    *num_entries = 1;

    return SOC_E_NONE;
}
#endif /* BCM_SHADOW_SUPPORT */

int
_soc_controlled_counter_get_info(int unit, soc_port_t port, soc_reg_t ctr_reg,
                            int *base_index, int *num_entries, char **cname)
{
    soc_control_t       *soc = SOC_CONTROL(unit);

    if (ctr_reg < 0) {
        return SOC_E_PARAM;
    }

    if (soc->controlled_counters == NULL) {
        return SOC_E_UNAVAIL;
    }    

    if(!COUNTER_IS_COLLECTED(soc->controlled_counters[ctr_reg])) {
        return SOC_E_PARAM;
    }

    *base_index = COUNTER_IDX_PORTBASE(unit, port) + soc->controlled_counters[ctr_reg].counter_idx;
    *num_entries = 1; /* MAC counters are non-array */
    if (NULL != cname) {
        *cname = soc->controlled_counters[ctr_reg].cname;
    }

    return SOC_E_NONE;
}

#ifdef BCM_ARAD_SUPPORT
STATIC int
_soc_counter_arad_get_info(int unit, soc_port_t port, soc_reg_t ctr_reg,
                            int *base_index, int *num_entries, char **cname)
{
    int rv = BCM_E_NONE;

    if (_SOC_CONTROLLED_COUNTER_USE(unit, port)) {
        rv = _soc_controlled_counter_get_info(unit, port, ctr_reg, base_index,
                                             num_entries, cname);
        
    }  else if (ctr_reg >= NUM_SOC_REG) {
        if (ctr_reg >= SOC_COUNTER_NON_DMA_END) {
            return SOC_E_PARAM;
        } else {
            /*non dma counters*/
            return SOC_E_UNAVAIL;
        }
    } else  {
        *base_index = COUNTER_IDX_PORTBASE(unit, port) +
            SOC_REG_CTR_IDX(unit, ctr_reg) + _SOC_CONTROLLED_COUNTER_NOF(unit)/*added shift for tbe controlled registers*/;
       
        *num_entries = SOC_REG_NUMELS(unit, ctr_reg);
        if (cname) {
            *cname = SOC_REG_NAME(unit, ctr_reg);
        }
    }
    return rv;

}
#endif /* BCM_ARAD_SUPPORT */

#ifdef BCM_PETRAB_SUPPORT
STATIC int
_soc_counter_petra_get_info(int unit, soc_port_t port, soc_reg_t ctr_reg,
                            int *base_index, int *num_entries, char **cname)
{
    if ((ctr_reg < 0) || (ctr_reg >= SOC_DPP_PETRA_NUM_MAC_COUNTERS)) {
        return SOC_E_PARAM;
    }

    *base_index = COUNTER_IDX_PORTBASE(unit, port) + ctr_reg;
    *num_entries = 1; /* MAC counters are non-array */
    if (cname != NULL) {
        *cname = soc_dpp_petra_nif_ctr_names[ctr_reg];
    }

    return SOC_E_NONE;
}
#endif /* BCM_PETRAB_SUPPORT */

STATIC int
_soc_counter_get_info(int unit, soc_port_t port, soc_reg_t ctr_reg,
                      int *base_index, int *num_entries, char **cname)
{
    int rv;

    if ((!(SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit))) && (port < 0)) {
        return SOC_E_PARAM;
    }

#ifdef BCM_ARAD_SUPPORT
    if (SOC_IS_ARAD(unit)) {
        rv = _soc_counter_arad_get_info(unit, port, ctr_reg, base_index,
                                         num_entries, cname);
        return rv;
    }  
#endif /* BCM_PETRAB_SUPPORT */

    if (soc_feature(unit, soc_feature_controlled_counters)) {
            rv = _soc_controlled_counter_get_info(unit, port, ctr_reg, base_index,
                                             num_entries, cname);
            return rv;
    }  

#ifdef BCM_PETRAB_SUPPORT
    if (SOC_IS_PETRAB(unit)) {
        rv = _soc_counter_petra_get_info(unit, port, ctr_reg, base_index,
                                         num_entries, cname);
        return rv;
    }  
#endif /* BCM_PETRAB_SUPPORT */

    if (ctr_reg >= NUM_SOC_REG) {
        if (ctr_reg >= SOC_COUNTER_NON_DMA_END) {
            return SOC_E_PARAM;
        }

#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TD2_TT2(unit)) {
            rv = _soc_counter_trident2_get_info(unit, port, ctr_reg, base_index,
                                               num_entries);
        } else
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_TRIDENT_SUPPORT
        if (SOC_IS_TD_TT(unit)) {
            rv = _soc_counter_trident_get_info(unit, port, ctr_reg, base_index,
                                               num_entries);
        } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            rv = _soc_counter_triumph3_get_info(unit, port, ctr_reg, base_index,
                                               num_entries);
        } else
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
        if (SOC_IS_KATANAX(unit)) {
            rv = _soc_counter_katana_get_info(unit, port, ctr_reg, base_index,
                                               num_entries);
        } else
#endif /* BCM_KATANA_SUPPORT */
#ifdef BCM_FIREBOLT_SUPPORT
        if (SOC_IS_FBX(unit) && !(SOC_IS_SHADOW(unit))) {
            rv = _soc_counter_fb_get_info(unit, port, ctr_reg, base_index,
                                          num_entries);
        } else 
#endif /* BCM_FIREBOLT_SUPPORT */
#ifdef BCM_SHADOW_SUPPORT
        if (SOC_IS_SHADOW(unit)) {
            rv = _soc_counter_shadow_get_info(unit, port, ctr_reg, base_index,
                                          num_entries);
        } else 
#endif /* BCM_SHADOW_SUPPORT */
        {
            rv = SOC_E_UNAVAIL;
        }

        if (rv < 0) {
            return rv;
        }
        if (cname) {
            *cname = SOC_CONTROL(unit)->
                counter_non_dma[ctr_reg - SOC_COUNTER_NON_DMA_START].cname;
        }
     
    } else {
        if (!SOC_REG_IS_ENABLED(unit, ctr_reg)) {
            return SOC_E_PARAM;
        }
#ifdef BCM_SHADOW_SUPPORT
        if (SOC_IS_SHADOW(unit) && IS_IL_PORT(unit, port)) {
            return SOC_E_PORT;
        }
#endif /* BCM_SHADOW_SUPPORT */

        *base_index = COUNTER_IDX_PORTBASE(unit, port) +
            SOC_REG_CTR_IDX(unit, ctr_reg);
        if (SOC_IS_TRIUMPH3(unit)) {
            *base_index -= 0x20;
        }        
#ifdef BCM_HURRICANE2_SUPPORT
        if (SOC_IS_HURRICANE2(unit)) {
            *base_index -= 0x37;
        }
#endif /* BCM_HURRICANE2_SUPPORT */
        *num_entries = SOC_REG_NUMELS(unit, ctr_reg);
        if (cname) {
            *cname = SOC_REG_NAME(unit, ctr_reg);
        }
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_counter_idx_get
 * Purpose:
 *      Get the index of a counter given the counter and port
 * Parameters:
 *      unit - The SOC unit number
 *      reg - The register number
 *      port - The port for which index is being calculated
 * Returns:
 *      SOC_E_XXX, no it is not
 * Notes:
 */

int
soc_counter_idx_get(int unit, soc_reg_t reg, int ar_idx, int port)
{
    int base_index, num_entries;
    if (_soc_counter_get_info(unit, port, reg, &base_index, &num_entries,
                              NULL) < 0) {
        return -1;
    }

    if (ar_idx < 0) {
        return base_index;
    } else if (ar_idx < num_entries) {
        return base_index + ar_idx;
    } else {
        return -1;
    }
}

/*
 * Function:
 *      _soc_counter_num_cosq_init
 * Purpose:
 *      Initialize the number of COSQ per port.
 * Parameters:
 *      unit - The SOC unit number
 *      nports - Number of ports the unit has.
 * Returns:
 *      None.
 */
STATIC void
_soc_counter_num_cosq_init(int unit, int nports)
{
    /* Some compiler may generate false warning in partial build dead code */
    if (nports > SOC_MAX_NUM_PORTS) {
        return;
    }

#ifdef BCM_TRIUMPH_SUPPORT
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        return;
    }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        return;
    }
#endif /* BCM_SHADOW_SUPPORT */
#ifdef BCM_HURRICANE_SUPPORT
    if (SOC_IS_HURRICANEX(unit)) {
        int port;
    
        /* all ports 8Q, including G, HG, CMIC and the reserved port 1 */
        for (port = 0; port < nports; port++) {
            num_cosq[unit][port] = 8;
        }

    } else
#endif /* BCM_HURRICANE_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || SOC_IS_VALKYRIE2(unit)){
        const int port_24q[] = {26, 27, 28, 29, 30, 31, 34, 38, 39, 42, 43, 46, 50, 51};
        const int port_16q = 54;
        int port, i;

        for (port = 0; port < nports; port++) {
            if (IS_CPU_PORT(unit, port)) {
                num_cosq[unit][port] = NUM_CPU_COSQ(unit);
            } else { 
                num_cosq[unit][port] = 8;
                for (i = 0; i < sizeof(port_24q) / sizeof(port_24q[1]); i++) {
                    if (port == port_24q[i]) {
                        num_cosq[unit][port] = 24;
                        break;
                    }
                }
                if (port == port_16q) {
                    num_cosq[unit][port] = 16;
                }
            }
        }

    } else
#endif /* BCM_TRIUMPH2_SUPPORT */
#ifdef BCM_ENDURO_SUPPORT
    if (SOC_IS_ENDURO(unit)){
        const int port_24q[] = {26, 27, 28, 29};
        int port, i;

        for (port = 0; port < nports; port++) {
            if (IS_CPU_PORT(unit, port)) {
                num_cosq[unit][port] = NUM_CPU_COSQ(unit);
            } else { 
                num_cosq[unit][port] = 8;
                for (i = 0; i < sizeof(port_24q) / sizeof(port_24q[1]); i++) {
                    if (port == port_24q[i]) {
                        num_cosq[unit][port] = 24;
                        break;
                    }
                }
            }
        }

    } else
#endif /* BCM_ENDURO_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        int port;

        for (port = 0; port < nports; port++) {
            if (IS_CPU_PORT(unit, port)) {
                num_cosq[unit][port] = NUM_CPU_COSQ(unit);
            } else if (IS_HG_PORT(unit, port)) {
                num_cosq[unit][port] = 24;
            } else {
                num_cosq[unit][port] = 8;
            }
        }
    } else
#endif /* BCM_KATANA_SUPPORT */
    if (SOC_IS_TR_VL(unit)) { 
        const int port_24q[] = {2, 3, 14, 15, 26, 27, 28, 29, 30, 31, 32, 43};
        int port, i;

        for (port = 0; port < nports; port++) {
            if (IS_CPU_PORT(unit, port)) {
                num_cosq[unit][port] = NUM_CPU_COSQ(unit);
            } else { 
                num_cosq[unit][port] = 8;
                for (i = 0; i < sizeof(port_24q) / sizeof(port_24q[1]); i++) {
                    if (port == port_24q[i]) {
                        num_cosq[unit][port] = 24;
                        break;
                    }
                }
            }
        }
    } else 
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_SC_CQ(unit) || SOC_IS_HB_GW(unit) || SOC_IS_FB_FX_HX(unit)) {
        int port;

        for (port = 0; port < nports; port++) {
            if (IS_CPU_PORT(unit, port)) {
                num_cosq[unit][port] = NUM_CPU_COSQ(unit);
            } else { 
                num_cosq[unit][port] = 8;
            }
        }
    } else
#endif /* BCM_FIREBOLT_SUPPORT */
    {
        return;
    }

    return;
}

#ifdef BCM_TRIUMPH3_SUPPORT
STATIC int
_soc_counter_triumph3_non_dma_init(int unit, int nports, 
                                   int non_dma_start_index, 
                                   int *non_dma_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma0, *non_dma1, *non_dma2;
    int num_entries, alloc_size;
    uint32 *buf;

    soc = SOC_CONTROL(unit);
    *non_dma_entries = 0;

    /* EGR_PERQ_XMT_COUNTERS size depends on user's portmap config */
    num_entries = soc_mem_index_count(unit, EGR_PERQ_XMT_COUNTERSm);
    alloc_size = num_entries * sizeof(uint32) *
                 soc_mem_entry_words(unit, EGR_PERQ_XMT_COUNTERSm);

    buf = soc_cm_salloc(unit, alloc_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, alloc_size);

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_ALLOC;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 48;
    non_dma0->num_entries = num_entries;
    non_dma0->mem = EGR_PERQ_XMT_COUNTERSm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PACKET_COUNTERf;
    non_dma0->cname = "PERQ_PKT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_max[0] = num_entries - 1;
    non_dma0->dma_mem[0] = EGR_PERQ_XMT_COUNTERSm;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE -
                                    SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTE_COUNTERf;
    non_dma1->cname = "PERQ_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 =  &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_UC -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma2->pbmp = PBMP_ALL(unit);
    non_dma2->entries_per_port = 10;
    non_dma2->num_entries = 0;
    non_dma2->cname = "UC_PERQ_PKT";

    non_dma2 =  &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_UC -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma1, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma2->pbmp = PBMP_ALL(unit);
    non_dma2->entries_per_port = 10;
    non_dma2->num_entries = 0;
    non_dma2->cname = "UC_PERQ_BYTE";

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
                      _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 48; /* 48 for cpu port */
    non_dma0->num_entries = soc_mem_index_count(unit, MMU_CTR_MC_DROP_MEMm);
    non_dma0->mem = MMU_CTR_MC_DROP_MEMm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PKT_CNTf;
    non_dma0->cname = "PERQ_DROP_PKT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_max[0] = non_dma0->num_entries - 1;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTE_COUNTf;
    non_dma1->cname = "PERQ_DROP_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID | _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 8;
    non_dma0->num_entries = soc_mem_index_count(unit, MMU_CTR_UC_DROP_MEMm);
    non_dma0->mem = MMU_CTR_UC_DROP_MEMm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PKT_CNTf;
    non_dma0->cname = "PERQ_DROP_PKT_UC";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_max[0] = non_dma0->num_entries - 1;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE_UC -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTE_COUNTf;
    non_dma1->cname = "PERQ_DROP_BYTE_UC";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = nports;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = DROP_PKT_CNT_INGr;
    non_dma0->field = COUNTf;
    non_dma0->cname = "DROP_PKT_ING";
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_BYTE_ING -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = DROP_BYTE_CNT_ING_64r;
    non_dma1->cname = "DROP_BYTE_ING";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = 63; /* one per port */
    non_dma0->mem = MMU_CTR_COLOR_DROP_MEMm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PKT_CNTf;
    non_dma0->cname = "DROP_PKT_YEL";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_min[0] = 63 * 4;
    non_dma0->dma_index_max[0] = 63 * 4 + 62;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "DROP_PKT_RED";
    non_dma1->dma_index_min[0] = 63 * 3;
    non_dma1->dma_index_max[0] = 63 * 3 + 62;
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_WRED_PKT_GREEN -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "WRED_PKT_GRE";
    non_dma1->dma_index_min[0] = 63 * 2;
    non_dma1->dma_index_max[0] = 63 * 2 + 62;
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_WRED_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "WRED_PKT_YEL";
    non_dma1->dma_index_min[0] = 63 * 1;
    non_dma1->dma_index_max[0] = 66 * 1 + 62;
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_WRED_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "WRED_PKT_RED";
    non_dma1->dma_index_min[0] = 0;
    non_dma1->dma_index_max[0] = 62;
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_POOL_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_PEAK;
    SOC_PBMP_CLEAR(non_dma0->pbmp);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 0;
    non_dma0->num_entries = 4;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = TOTAL_BUFFER_COUNT_CELL_SPr;
    non_dma0->field = TOTAL_BUFFER_COUNTf;
    non_dma0->cname = "POOL_PEAK";
    non_dma0->dma_buf[0] = buf;
    *non_dma_entries += non_dma0->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_POOL_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "POOL_CUR";

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_MIN_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_PEAK;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 8;
    non_dma0->num_entries = nports * non_dma0->entries_per_port;
    non_dma0->dma_index_max[0] = non_dma0->num_entries - 1;
    non_dma0->mem = THDI_PORT_PG_CNTRSm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PG_MIN_COUNTf;
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_mem[0] = non_dma0->mem;
    non_dma0->dma_index_max[0] = non_dma0->num_entries - 1;
    non_dma0->cname = "PG_MIN_PEAK";
    *non_dma_entries += non_dma0->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_MIN_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID | 
            _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "PG_MIN_CUR";

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_SHARED_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = PG_SHARED_COUNTf;
    non_dma1->cname = "PG_SHARED_PEAK";
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_SHARED_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma1, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID | _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "PG_SHARED_CUR";

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_HDRM_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = PG_HDRM_COUNTf;
    non_dma1->cname = "PG_HDRM_PEAK";
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_HDRM_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma1, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID | _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "PG_HDRM_CUR";

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_QUEUE_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->pbmp = PBMP_CMIC(unit);
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->entries_per_port = 48;
    non_dma1->num_entries = nports * non_dma0->entries_per_port;
    non_dma1->reg = OP_QUEUE_TOTAL_COUNT_CELLr;
    non_dma1->mem = INVALIDm;
    non_dma1->field = Q_TOTAL_COUNT_CELLf;
    non_dma1->cname = "MC_QUEUE_PEAK";
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_QUEUE_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma1, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID | _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "MC_QUEUE_CUR";

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_UC_QUEUE_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->pbmp = PBMP_PORT_ALL(unit);
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->entries_per_port = 48;
    non_dma1->num_entries = soc_mem_index_count(unit, MMU_THDO_COUNTER_QUEUEm);
    non_dma1->reg = INVALIDr;
    non_dma1->mem = MMU_THDO_COUNTER_QUEUEm;
    non_dma1->field =  MIN_COUNTf;
    non_dma1->cname = "UC_QUEUE_PEAK";
    non_dma1->dma_mem[0] = non_dma1->mem;
    non_dma1->dma_index_max[0] = non_dma1->num_entries - 1;
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_UC_QUEUE_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma1, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "UC_QUEUE_CUR";
    return SOC_E_NONE;
}
#endif

#ifdef BCM_TRIDENT_SUPPORT
/*
 * Function:
 *      _soc_counter_trident_non_dma_init
 * Purpose:
 *      Initialize Trident's non-DMA counters.
 * Parameters:
 *      unit - The SOC unit number
 *      nports - Number of ports.
 *      non_dma_start_index - The starting index of non-DMA counter entries.
 *      non_dma_entries - (OUT) The number of non-DMA counter entries.
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_MEMORY
 */
STATIC int
_soc_counter_trident_non_dma_init(int unit, int nports, int non_dma_start_index, 
        int *non_dma_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma0, *non_dma1, *non_dma2;
    int entry_words, num_entries[2], alloc_size, table_size;
    uint32 *buf;

    soc = SOC_CONTROL(unit);
    *non_dma_entries = 0;

    soc_trident_get_egr_perq_xmt_counters_size(unit, &num_entries[0],
                                               &num_entries[1]);

    /* EGR_PERQ_XMT_COUNTERS size depends on user's portmap config */
    alloc_size = (num_entries[0] + num_entries[1]) *
        soc_mem_entry_words(unit, EGR_PERQ_XMT_COUNTERSm) * sizeof(uint32);

    /* MMU_CTR_UC_DROP_MEM is the largest among MMU_CTR_*_DROP_MEM tables */
    table_size = soc_mem_index_count(unit, MMU_CTR_UC_DROP_MEMm) *
        soc_mem_entry_words(unit, MMU_CTR_UC_DROP_MEMm) * sizeof(uint32);
    if (alloc_size < table_size) {
        alloc_size = table_size;
    }

    buf = soc_cm_salloc(unit, alloc_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, alloc_size);

    entry_words = soc_mem_entry_words(unit, EGR_PERQ_XMT_COUNTERSm);
    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_ALLOC;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 48; /* cpu port has max number of queues */
    non_dma0->num_entries = num_entries[0] + num_entries[1];
    non_dma0->mem = EGR_PERQ_XMT_COUNTERSm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PACKET_COUNTERf;
    non_dma0->cname = "PERQ_PKT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_buf[1] = &buf[num_entries[0] * entry_words];
    non_dma0->dma_index_max[0] = num_entries[0] - 1;
    non_dma0->dma_index_max[1] = num_entries[1] - 1;
    non_dma0->dma_mem[0] = EGR_PERQ_XMT_COUNTERS_Xm;
    non_dma0->dma_mem[1] = EGR_PERQ_XMT_COUNTERS_Ym;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTE_COUNTERf;
    non_dma1->cname = "PERQ_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_UC -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma0;
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma2->pbmp = PBMP_PORT_ALL(unit);
    non_dma2->entries_per_port = 10;
    non_dma2->num_entries = 0;
    non_dma2->cname = "UC_PERQ_PKT";

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_UC -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma1;
    non_dma2->pbmp = PBMP_PORT_ALL(unit);
    non_dma2->entries_per_port = 10;
    non_dma2->num_entries = 0;
    non_dma2->cname = "UC_PERQ_BYTE";

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_EXT -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma0;
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma2->pbmp = PBMP_PORT_ALL(unit);
    non_dma2->entries_per_port = 64;
    non_dma2->num_entries = 0;
    non_dma2->cname = "EXT_PERQ_PKT";

    non_dma2 =
        &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_EXT -
                              SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma1;
    non_dma2->pbmp = PBMP_PORT_ALL(unit);
    non_dma2->entries_per_port = 64;
    non_dma2->num_entries = 0;
    non_dma2->cname = "EXT_PERQ_BYTE";

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 48; /* 48 for cpu port */
    non_dma0->num_entries = soc_mem_index_count(unit, MMU_CTR_MC_DROP_MEMm);
    non_dma0->mem = MMU_CTR_MC_DROP_MEMm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PKT_CNTf;
    non_dma0->cname = "MCQ_DROP_PKT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_max[0] = non_dma0->num_entries - 1;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTE_COUNTf;
    non_dma1->cname = "MCQ_DROP_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 74; /*  10 ucast + 64 ext ucast */
    non_dma0->num_entries = soc_mem_index_count(unit, MMU_CTR_UC_DROP_MEMm);
    non_dma0->mem = MMU_CTR_UC_DROP_MEMm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PKT_CNTf;
    non_dma0->cname = "UCQ_DROP_PKT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_max[0] = non_dma0->num_entries - 1;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE_UC -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTE_COUNTf;
    non_dma1->cname = "UCQ_DROP_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = nports;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = DROP_PKT_CNT_INGr;
    non_dma0->field = COUNTf;
    non_dma0->cname = "DROP_PKT_ING";
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_BYTE_ING -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = DROP_BYTE_CNT_ING_64r;
    non_dma1->cname = "DROP_BYTE_ING";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = 66; /* one per port */
    non_dma0->mem = MMU_CTR_COLOR_DROP_MEMm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PKT_CNTf;
    non_dma0->cname = "DROP_PKT_YEL";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_min[0] = 66 * 4;
    non_dma0->dma_index_max[0] = 66 * 4 + 65;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "DROP_PKT_RED";
    non_dma1->dma_index_min[0] = 66 * 3;
    non_dma1->dma_index_max[0] = 66 * 3 + 65;
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_WRED_PKT_GREEN -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "WRED_PKT_GRE";
    non_dma1->dma_index_min[0] = 66 * 2;
    non_dma1->dma_index_max[0] = 66 * 2 + 65;
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_WRED_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "WRED_PKT_YEL";
    non_dma1->dma_index_min[0] = 66 * 1;
    non_dma1->dma_index_max[0] = 66 * 1 + 65;
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_WRED_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "WRED_PKT_RED";
    non_dma1->dma_index_min[0] = 0;
    non_dma1->dma_index_max[0] = 65;
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_POOL_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_PEAK;
    SOC_PBMP_CLEAR(non_dma0->pbmp);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 0;
    non_dma0->num_entries = 4;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = TOTAL_BUFFER_COUNT_CELL_SPr;
    non_dma0->field = TOTAL_BUFFER_COUNTf;
    non_dma0->cname = "POOL_PEAK";
    *non_dma_entries += non_dma0->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_POOL_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "POOL_CUR";

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_MIN_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_PEAK;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 8;
    non_dma0->num_entries = nports * non_dma0->entries_per_port;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = PG_MIN_COUNT_CELLr;
    non_dma0->field = PG_MIN_COUNTf;
    non_dma0->cname = "PG_MIN_PEAK";
    *non_dma_entries += non_dma0->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_MIN_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "PG_MIN_CUR";

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_SHARED_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = PG_SHARED_COUNT_CELLr;
    non_dma1->field = PG_SHARED_COUNTf;
    non_dma1->cname = "PG_SHARED_PEAK";
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_SHARED_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma1, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "PG_SHARED_CUR";

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_HDRM_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = PG_HDRM_COUNT_CELLr;
    non_dma1->field = PG_HDRM_COUNTf;
    non_dma1->cname = "PG_HDRM_PEAK";
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_HDRM_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma1, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "PG_HDRM_CUR";

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_QUEUE_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->pbmp = PBMP_ALL(unit);
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->entries_per_port = 5;
    non_dma1->num_entries = nports * non_dma0->entries_per_port;
    non_dma1->reg = OP_QUEUE_TOTAL_COUNT_CELLr;
    non_dma1->field = Q_TOTAL_COUNT_CELLf;
    non_dma1->cname = "MC_QUEUE_PEAK";
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_QUEUE_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma1, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "MC_QUEUE_CUR";

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_UC_QUEUE_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->pbmp = PBMP_PORT_ALL(unit);
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->entries_per_port = 10;
    non_dma1->num_entries = nports * non_dma1->entries_per_port;
    non_dma1->reg = OP_UC_QUEUE_TOTAL_COUNT_CELLr;
    non_dma1->field =  Q_TOTAL_COUNT_CELLf;
    non_dma1->cname = "UC_QUEUE_PEAK";
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_UC_QUEUE_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma1, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "UC_QUEUE_CUR";

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EXT_QUEUE_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->pbmp = PBMP_EQ(unit);
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->entries_per_port = 64;
    non_dma1->num_entries = nports * non_dma1->entries_per_port;
    non_dma1->reg = OP_EX_QUEUE_TOTAL_COUNT_CELLr;
    non_dma1->cname = "EXT_QUEUE_PEAK";
    non_dma1->field =  Q_TOTAL_COUNT_CELLf;
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EXT_QUEUE_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma2, non_dma1, sizeof(soc_counter_non_dma_t));
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "EXT_QUEUE_CUR";

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_MMU_QCN_CNM -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 2;
    non_dma0->num_entries = soc_mem_index_count(unit, MMU_QCN_CNM_COUNTERm);
    non_dma0->mem = MMU_QCN_CNM_COUNTERm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = CNM_CNTf;
    non_dma0->cname = "QCN_CNM_COUNTER";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_mem[0] = non_dma0->mem;
    non_dma0->dma_index_min[0] = 0;
    non_dma0->dma_index_max[0] = soc_mem_index_max(unit, non_dma0->dma_mem[0]);
    *non_dma_entries += non_dma0->num_entries;

    return SOC_E_NONE;
}
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
STATIC int
_soc_counter_trident2_non_dma_init(int unit, int nports, 
                                   int non_dma_start_index, 
                                   int *non_dma_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma0, *non_dma1, *non_dma2;
    int  alloc_size;
    uint32 *buf;

    soc = SOC_CONTROL(unit);
    *non_dma_entries = 0;

    /* MMU_CTR_UC_DROP_MEM is the largest table to be DMA'ed */

    alloc_size = 2 * soc_mem_index_count(unit, MMU_CTR_UC_DROP_MEMm) *
                  sizeof(uint32) *  soc_mem_entry_words(unit, MMU_CTR_UC_DROP_MEMm);


    buf = soc_cm_salloc(unit, alloc_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, alloc_size);

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_ALLOC;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 48; /* cpu port has max number of queues */
    non_dma0->num_entries = 2 * soc_mem_index_count(unit, EGR_PERQ_XMT_COUNTERS_Xm);
    non_dma0->mem = EGR_PERQ_XMT_COUNTERSm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PACKET_COUNTERf;
    non_dma0->cname = "PERQ_PKT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_buf[1] =
              &buf[soc_mem_index_count(unit, EGR_PERQ_XMT_COUNTERS_Xm) * 
                   soc_mem_entry_words(unit, EGR_PERQ_XMT_COUNTERS_Xm)];
    non_dma0->dma_index_max[0] = soc_mem_index_count(unit, EGR_PERQ_XMT_COUNTERS_Xm) - 1;
    non_dma0->dma_index_max[1] = soc_mem_index_count(unit, EGR_PERQ_XMT_COUNTERS_Xm) - 1;
    non_dma0->dma_mem[0] = EGR_PERQ_XMT_COUNTERS_Xm;
    non_dma0->dma_mem[1] = EGR_PERQ_XMT_COUNTERS_Ym;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTE_COUNTERf;
    non_dma1->cname = "PERQ_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT_UC -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma0;
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma2->pbmp = PBMP_PORT_ALL(unit);
    non_dma2->entries_per_port = 10;
    non_dma2->num_entries = 0; 
    non_dma2->cname = "UC_PERQ_PKT";

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE_UC -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma1;
    non_dma2->pbmp = PBMP_PORT_ALL(unit);
    non_dma2->entries_per_port = 10;
    non_dma2->num_entries = 0; 
    non_dma2->cname = "UC_PERQ_BYTE";

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
                      _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 48; /* 48 for cpu port */
    non_dma0->num_entries = soc_mem_index_count(unit, MMU_CTR_MC_DROP_MEM0m) +
        soc_mem_index_count(unit, MMU_CTR_MC_DROP_MEM1m);
    non_dma0->mem = MMU_CTR_MC_DROP_MEM0m;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PKTCNTf;
    non_dma0->cname = "MCQ_DROP_PKT";
    non_dma0->dma_mem[0] = MMU_CTR_MC_DROP_MEM0m;
    non_dma0->dma_mem[1] = MMU_CTR_MC_DROP_MEM1m;
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_buf[1] =
        &buf[soc_mem_index_count(unit, non_dma0->dma_mem[0]) *
             soc_mem_entry_words(unit, non_dma0->dma_mem[0])];
    non_dma0->dma_index_max[0] = soc_mem_index_max(unit, non_dma0->dma_mem[0]);
    non_dma0->dma_index_max[1] = soc_mem_index_max(unit, non_dma0->dma_mem[1]);
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTECNTf;
    non_dma1->cname = "MCQ_DROP_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID | _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 10;
    non_dma0->num_entries = soc_mem_index_count(unit, MMU_CTR_UC_DROP_MEMm);
    non_dma0->mem = MMU_CTR_UC_DROP_MEMm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PKTCNTf;
    non_dma0->cname = "UCQ_DROP_PKT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_buf[1] =
        &buf[1480 * soc_mem_entry_words(unit, non_dma0->mem)];
    non_dma0->dma_index_max[0] = 1479;
    non_dma0->dma_index_min[1] = 1536;
    non_dma0->dma_index_max[1] = 3015;
    non_dma0->dma_mem[0] = non_dma0->mem;
    non_dma0->dma_mem[1] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE_UC -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTECNTf;
    non_dma1->cname = "UCQ_DROP_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID | _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = soc_mem_index_count(unit, MMU_CTR_ING_DROP_MEMm);
    non_dma0->mem = MMU_CTR_ING_DROP_MEMm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PKTCNTf;
    non_dma0->cname = "DROP_PKT_ING";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_max[0] = soc_mem_index_max(unit, non_dma0->mem);
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_BYTE_ING -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTECNTf;
    non_dma1->cname = "DROP_BYTE_ING";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 =
        &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING_METER -
                              SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID | _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = soc_mem_index_count(unit, MMU_CTR_ING_DROP_MEMm);
    non_dma0->mem = MMU_CTR_MTRI_DROP_MEMm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PKTCNTf;
    non_dma0->cname = "DROP_PKT_IMTR";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_max[0] = soc_mem_index_max(unit, non_dma0->mem);
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 =
        &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_BYTE_ING_METER -
                              SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTECNTf;
    non_dma1->cname = "DROP_BYTE_IMTR";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = 106; /* one per port */
    non_dma0->mem = MMU_CTR_COLOR_DROP_MEMm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PKTCNTf;
    non_dma0->cname = "DROP_PKT_YEL";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_min[0] = 106 * 4;
    non_dma0->dma_index_max[0] = (106 * 4) + 105;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "DROP_PKT_RED";
    non_dma1->dma_index_min[0] = 106 * 3;
    non_dma1->dma_index_max[0] = (106 * 3) + 105;
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_WRED_PKT_GREEN -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "WRED_PKT_GRE";
    non_dma1->dma_index_min[0] = 106 * 2;
    non_dma1->dma_index_max[0] = (106 * 2) + 105;
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_WRED_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "WRED_PKT_YEL";
    non_dma1->dma_index_min[0] = 106 * 1;
    non_dma1->dma_index_max[0] = (106 * 1) + 105;
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_WRED_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->cname = "WRED_PKT_RED";
    non_dma1->dma_index_min[0] = 0;
    non_dma1->dma_index_max[0] = 105;
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_DROP_RQ_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA;
    SOC_PBMP_CLEAR(non_dma0->pbmp);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 0;
    non_dma0->num_entries = 11;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = DROP_PKT_CNT_RQE_PKTr;
    non_dma0->field = PKTCNTf;
    non_dma0->cname = "RQ_DROP_PKT";
    non_dma0->dma_buf[0] = buf;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_DROP_RQ_BYTE -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = DROP_PKT_CNT_RQE_BYTE_64r;
    non_dma1->field = BYTECNTf;
    non_dma1->cname = "RQ_DROP_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_DROP_RQ_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = DROP_PKT_CNT_RQE_YELr;
    non_dma1->field = PKTCNTf;
    non_dma1->cname = "RQ_DROP_PKT_YEL";
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_DROP_RQ_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = DROP_PKT_CNT_RQE_REDr;
    non_dma1->field = PKTCNTf;
    non_dma1->cname = "RQ_DROP_PKT_RED";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_POOL_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_PEAK;
    SOC_PBMP_CLEAR(non_dma0->pbmp);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 0;
    non_dma0->num_entries = 4;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = THDI_POOL_SHARED_COUNT_SPr;
    non_dma0->field = TOTAL_BUFFER_COUNTf;
    non_dma0->cname = "POOL_PEAK";
    non_dma0->dma_buf[0] = buf;
    *non_dma_entries += non_dma0->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_POOL_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma0;
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "POOL_CUR";

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_MIN_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_PEAK;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 8;
    non_dma0->num_entries = nports * non_dma0->entries_per_port;
    non_dma0->mem = THDI_PORT_PG_CNTRS_RT1_Xm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PG_MIN_COUNTf;
    non_dma0->cname = "PG_MIN_PEAK";
    non_dma0->dma_mem[0] = THDI_PORT_PG_CNTRS_RT1_Xm;
    non_dma0->dma_mem[1] = THDI_PORT_PG_CNTRS_RT1_Ym;
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_buf[1] =
        &buf[soc_mem_index_count(unit, non_dma0->dma_mem[0]) *
             soc_mem_entry_words(unit, non_dma0->dma_mem[0])];
    non_dma0->dma_index_max[0] = soc_mem_index_max(unit, non_dma0->dma_mem[0]);
    non_dma0->dma_index_max[1] = soc_mem_index_max(unit, non_dma0->dma_mem[1]);
    *non_dma_entries += non_dma0->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_MIN_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma0;
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
            _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "PG_MIN_CUR";

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_SHARED_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma1 = *non_dma0;
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_PEAK;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = PG_SHARED_COUNTf;
    non_dma1->cname = "PG_SHARED_PEAK";
    *non_dma_entries += non_dma1->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_SHARED_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma1;
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "PG_SHARED_CUR";

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_HDRM_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_PEAK;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 8;
    non_dma0->num_entries = nports * non_dma0->entries_per_port;
    non_dma0->mem = THDI_PORT_PG_CNTRS_RT2_Xm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PG_HDRM_COUNTf;
    non_dma0->cname = "PG_HDRM_PEAK";
    non_dma0->dma_mem[0] = THDI_PORT_PG_CNTRS_RT2_Xm;
    non_dma0->dma_mem[1] = THDI_PORT_PG_CNTRS_RT2_Ym;
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_buf[1] =
        &buf[soc_mem_index_count(unit, non_dma0->dma_mem[0]) *
             soc_mem_entry_words(unit, non_dma0->dma_mem[0])];
    non_dma0->dma_index_max[0] = soc_mem_index_max(unit, non_dma0->dma_mem[0]);
    non_dma0->dma_index_max[1] = soc_mem_index_max(unit, non_dma0->dma_mem[1]);
    *non_dma_entries += non_dma0->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PG_HDRM_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma1;
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "PG_HDRM_CUR";

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_QUEUE_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_PEAK;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 48; /* 48 for cpu port */
    non_dma0->num_entries =
        soc_mem_index_count(unit, MMU_THDM_DB_QUEUE_COUNT_0m) +
        soc_mem_index_count(unit, MMU_THDM_DB_QUEUE_COUNT_1m);
    non_dma0->mem = MMU_THDM_DB_QUEUE_COUNT_0m;
    non_dma0->reg = INVALIDr;
    non_dma0->field = SHARED_COUNTf;
    non_dma0->cname = "QUEUE_PEAK";
    non_dma0->dma_mem[0] = MMU_THDM_DB_QUEUE_COUNT_0m;
    non_dma0->dma_mem[1] = MMU_THDM_DB_QUEUE_COUNT_1m;
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_buf[1] =
        &buf[soc_mem_index_count(unit, non_dma0->dma_mem[0]) *
             soc_mem_entry_words(unit, non_dma0->dma_mem[0])];
    non_dma0->dma_index_max[0] = soc_mem_index_max(unit, non_dma0->dma_mem[0]);
    non_dma0->dma_index_max[1] = soc_mem_index_max(unit, non_dma0->dma_mem[1]);
    *non_dma_entries += non_dma0->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_QUEUE_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma0;
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "QUEUE_CUR";

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_UC_QUEUE_PEAK -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_PEAK;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 0;
    non_dma0->num_entries =
        soc_mem_index_count(unit, MMU_THDU_XPIPE_COUNTER_QUEUEm) +
        soc_mem_index_count(unit, MMU_THDU_YPIPE_COUNTER_QUEUEm);
    non_dma0->mem = MMU_THDU_XPIPE_COUNTER_QUEUEm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = SHARED_COUNTf;
    non_dma0->cname = "UC_QUEUE_PEAK";
    non_dma0->dma_mem[0] = MMU_THDU_XPIPE_COUNTER_QUEUEm;
    non_dma0->dma_mem[1] = MMU_THDU_YPIPE_COUNTER_QUEUEm;
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_buf[1] =
        &buf[soc_mem_index_count(unit, non_dma0->dma_mem[0]) *
             soc_mem_entry_words(unit, non_dma0->dma_mem[0])];
    non_dma0->dma_index_max[0] = soc_mem_index_max(unit, non_dma0->dma_mem[0]);
    non_dma0->dma_index_max[1] = soc_mem_index_max(unit, non_dma0->dma_mem[1]);
    *non_dma_entries += non_dma0->num_entries;

    non_dma2 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_UC_QUEUE_CURRENT -
                                     SOC_COUNTER_NON_DMA_START];
    *non_dma2 = *non_dma0;
    non_dma2->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_CURRENT;
    non_dma2->cname = "UC_QUEUE_CUR";

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_MMU_QCN_CNM -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 8;
    non_dma0->num_entries = soc_mem_index_count(unit, MMU_QCN_CNM_COUNTERm);
    non_dma0->mem = MMU_QCN_CNM_COUNTERm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = CNM_CNTf;
    non_dma0->cname = "QCN_CNM_COUNTER";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_mem[0] = non_dma0->mem;
    non_dma0->dma_index_max[0] = soc_mem_index_max(unit, non_dma0->dma_mem[0]);
    *non_dma_entries += non_dma0->num_entries;
    return SOC_E_NONE;

}
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_HURRICANE_SUPPORT
/*
 * Function:
 *      _soc_counter_hu_non_dma_init
 * Purpose:
 *      Initialize Hurricane's non-DMA counters.
 * Parameters:
 *      unit - The SOC unit number
 *      nports - Number of ports.
 *      non_dma_start_index - The starting index of non-DMA counter entries.
 *      non_dma_entries - (OUT) The number of non-DMA counter entries.
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_MEMORY
 */
STATIC int
_soc_counter_hu_non_dma_init(int unit, int nports, int non_dma_start_index, 
        int *non_dma_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma0, *non_dma1;
    int max_cosq_per_port, total_num_cosq, port;
    int num_entries, alloc_size;
    uint32 *buf;

    soc = SOC_CONTROL(unit);
    *non_dma_entries = 0;

    max_cosq_per_port = 0;
    total_num_cosq = 0;
    for (port = 0; port < nports; port++) {
        if (num_cosq[unit][port] > max_cosq_per_port) {
            max_cosq_per_port = num_cosq[unit][port];
        }
        total_num_cosq += num_cosq[unit][port];
    }

    num_entries = soc_mem_index_count(unit, EGR_PERQ_XMT_COUNTERSm);
    alloc_size = num_entries *
        soc_mem_entry_words(unit, EGR_PERQ_XMT_COUNTERSm) * sizeof(uint32);

    buf = soc_cm_salloc(unit, alloc_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, alloc_size);

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_ALLOC;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = max_cosq_per_port;
    non_dma0->num_entries = num_entries;
    non_dma0->mem = EGR_PERQ_XMT_COUNTERSm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PACKET_COUNTERf;
    non_dma0->cname = "PERQ_PKT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_max[0] = num_entries - 1;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTE_COUNTERf;
    non_dma1->cname = "PERQ_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = nports;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = CNGDROPCOUNT1r;
    non_dma0->field = DROPPKTCOUNTf;
    non_dma0->cname = "DROP_PKT_YEL";
    *non_dma_entries += non_dma0->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = nports;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = CNGDROPCOUNT0r;
    non_dma0->field = DROPPKTCOUNTf;
    non_dma0->cname = "DROP_PKT_RED";
    *non_dma_entries += non_dma0->num_entries;

    return SOC_E_NONE;
}
#endif /* BCM_HURRICANE_SUPPORT */

#ifdef BCM_KATANA_SUPPORT

STATIC int
_soc_counter_kt_cosq_gport_non_dma_init(int unit, int nports, 
                                        int non_dma_start_index, 
                                        int *non_dma_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma0, *non_dma1;
    _kt_cosq_counter_mem_map_t *mem_map;
    int table_size, id;
    uint32 *buf;
    char *cname_pkt[] = { "DROP_PKT", "ACCEPT_PKT",  
                          "DROP_PKT_G", "DROP_PKT_Y",
                          "DROP_PKT_R", "ACCEPT_PKT_G",
                          "ACCEPT_PKT_Y", "ACCEPT_PKT_R",
                          "PERQ_PKT" };
    char *cname_byte[] = { "DROP_BYTE", "ACCEPT_BYTE",  
                          "DROP_BYTE_G", "DROP_BYTE_Y",
                          "DROP_BYTE_R", "ACCEPT_BYTE_G",
                          "ACCEPT_BYTE_Y", "ACCEPT_BYTE_R",
                          "PERQ_BYTE" };  

    soc = SOC_CONTROL(unit);
    *non_dma_entries = 0;

    table_size = soc_mem_index_count(unit, CTR_FLEX_COUNT_0m) *
        soc_mem_entry_words(unit, CTR_FLEX_COUNT_0m) * sizeof(uint32);

    buf = soc_cm_salloc(unit, table_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, table_size);

    mem_map = _kt_cosq_gport_mem_map;
        
    /* 
     * CTR_FLEX_COUNT_0 = Total ENQ discarded 
     * CTR_FLEX_COUNT_1 = Total ENQ Accepted 
     * CTR_FLEX_COUNT_2 = Green ENQ discarded 
     * CTR_FLEX_COUNT_3 = Yellow ENQ discarded
     * CTR_FLEX_COUNT_4 = Red packet ENQ discarded 
     * CTR_FLEX_COUNT_5 = Green ENQ Accepted
     * CTR_FLEX_COUNT_6 = Yellow ENQ Accepted
     * CTR_FLEX_COUNT_7 = Red ENQ Accepted
     * CTR_FLEX_COUNT_8 = Total DEQ
     */
    for (id = 0; id < 9; id++) {
        if (soc_feature(unit, soc_feature_counter_toggled_read) && 
            (mem_map[id].non_dma_id == SOC_COUNTER_NON_DMA_COSQ_DROP_PKT ||
             mem_map[id].non_dma_id == SOC_COUNTER_NON_DMA_COSQ_ACCEPT_PKT)) {
            continue;
        }
        non_dma0 = &soc->counter_non_dma[mem_map[id].non_dma_id -
                                         SOC_COUNTER_NON_DMA_START];
        non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
                          _SOC_COUNTER_NON_DMA_DO_DMA |
                          _SOC_COUNTER_NON_DMA_CLEAR_ON_READ;
        if (id == 0) {
            non_dma0->flags |= _SOC_COUNTER_NON_DMA_ALLOC;
        }
        if (!soc_feature(unit, soc_feature_counter_toggled_read)) {
            /* adjust the extra count 1 in s/w counters */
            if (!SOC_IS_KATANA2(unit)) { 
                non_dma0->flags |= _SOC_COUNTER_NON_DMA_EXTRA_COUNT;
            }
        }
        non_dma0->pbmp = PBMP_ALL(unit);
        non_dma0->base_index = non_dma_start_index + *non_dma_entries;
        non_dma0->entries_per_port = 1;
        non_dma0->num_entries = soc_mem_index_count(unit, CTR_FLEX_COUNT_0m);
        non_dma0->mem = mem_map[id].mem;
        non_dma0->reg = INVALIDr;
        non_dma0->field = COUNTf;
        non_dma0->cname = cname_pkt[id];
        non_dma0->dma_buf[0] = buf;
        non_dma0->dma_index_max[0] = non_dma0->num_entries - 1;
        non_dma0->dma_mem[0] = non_dma0->mem;
        *non_dma_entries += non_dma0->num_entries;

        non_dma1 = &soc->counter_non_dma[mem_map[id].non_dma_id -
                                        SOC_COUNTER_NON_DMA_START + 1];
        sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
        non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID |
                          _SOC_COUNTER_NON_DMA_CLEAR_ON_READ;
        non_dma1->base_index = non_dma_start_index + *non_dma_entries;
        non_dma1->field = BYTE_COUNTf;
        non_dma1->cname = cname_byte[id];
        *non_dma_entries += non_dma1->num_entries;    
    }    
    
    return SOC_E_NONE;
}

/*
 * Function:
 *      _soc_counter_katana_non_dma_init
 * Purpose:
 *      Initialize Katana's non-DMA counters.
 * Parameters:
 *      unit - The SOC unit number
 *      nports - Number of ports.
 *      non_dma_start_index - The starting index of non-DMA counter entries.
 *      non_dma_entries - (OUT) The number of non-DMA counter entries.
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_MEMORY
 */
STATIC int
_soc_counter_katana_non_dma_init(int unit, int nports, int non_dma_start_index, 
                                 int *non_dma_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma0, *non_dma1;
    _kt_cosq_counter_mem_map_t *mem_map;
    int table_size;
    uint32 *buf;
    int i, id;
    int entry_words, buf_base;
    char *cname_pkt[] = { "DROP_PKT", "DROP_PKT_R",
                          "PERQ_PKT" };
    char *cname_byte[] = { "DROP_BYTE", "DROP_BYTE_R",
                           "PERQ_BYTE" };
   
    if (soc_feature(unit, soc_feature_cosq_gport_stat_ability)) {
        SOC_IF_ERROR_RETURN
            (_soc_counter_kt_cosq_gport_non_dma_init(unit, nports,
                non_dma_start_index, non_dma_entries));
        return SOC_E_NONE;
    }
    mem_map = _kt_cosq_mem_map;
    soc = SOC_CONTROL(unit);
    *non_dma_entries = 0;

    entry_words = soc_mem_entry_words(unit, CTR_FLEX_COUNT_0m);
    table_size = soc_mem_index_count(unit, CTR_FLEX_COUNT_0m) * 4 *
                 entry_words * sizeof(uint32);
    buf = soc_cm_salloc(unit, table_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, table_size);

    /*
     * CTR_FLEX_COUNT_0/1/2/3 = Total ENQ discarded
     * CTR_FLEX_COUNT_4/5/6/7  = Red packet ENQ discarded
     * CTR_FLEX_COUNT_8/9/10/11  = Total DEQ
     */
    for (id = 0; id < 12; id = id + 4) {
        if (soc_feature(unit, soc_feature_counter_toggled_read) &&
            mem_map[id].non_dma_id == SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_RED) {
            continue;
        }
        non_dma0 = &soc->counter_non_dma[mem_map[id].non_dma_id -
                                     SOC_COUNTER_NON_DMA_START];
        non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
                          _SOC_COUNTER_NON_DMA_DO_DMA |
                          _SOC_COUNTER_NON_DMA_CLEAR_ON_READ;
        if (id == 0) {
            non_dma0->flags |= _SOC_COUNTER_NON_DMA_ALLOC;
        }
        if (!soc_feature(unit, soc_feature_counter_toggled_read)) {
            /* adjust the extra count 1 in s/w counters */
            if (!SOC_IS_KATANA2(unit)) { 
                non_dma0->flags |= _SOC_COUNTER_NON_DMA_EXTRA_COUNT;
            }
        }
        non_dma0->pbmp = PBMP_ALL(unit);
        non_dma0->base_index = non_dma_start_index + *non_dma_entries;
        non_dma0->entries_per_port = 1;
        non_dma0->mem = mem_map[id].mem;
        if (soc_feature(unit, soc_feature_counter_toggled_read) && 
            (id == 8)) {
            non_dma0->mem = mem_map[id].mem - 4; 
        }
        non_dma0->num_entries = 4 * soc_mem_index_count(unit, non_dma0->mem);
        non_dma0->reg = INVALIDr;
        non_dma0->field = COUNTf;
        non_dma0->cname = cname_pkt[id / 4];

        entry_words = soc_mem_entry_words(unit, non_dma0->mem);
        for (i = 0; i < 4; i++) {
            buf_base = i * entry_words * soc_mem_index_count(unit, non_dma0->mem);
            non_dma0->dma_buf[i] = &buf[buf_base];
            non_dma0->dma_index_min[i] = 0;
            non_dma0->dma_index_max[i] = soc_mem_index_count(unit, non_dma0->mem) - 1;
            non_dma0->dma_mem[i] = mem_map[id + i].mem;
            if (soc_feature(unit, soc_feature_counter_toggled_read) && 
                (id == 8)) {
                non_dma0->dma_mem[i] = mem_map[id + i].mem - 4;
            }
        }
        *non_dma_entries += non_dma0->num_entries;

        non_dma1 = &soc->counter_non_dma[mem_map[id].non_dma_id -
                                    SOC_COUNTER_NON_DMA_START + 1];
        sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
        non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID |
                          _SOC_COUNTER_NON_DMA_CLEAR_ON_READ;
        non_dma1->base_index = non_dma_start_index + *non_dma_entries;
        non_dma1->field = BYTE_COUNTf;
        non_dma1->cname = cname_byte[id / 4];
        *non_dma_entries += non_dma1->num_entries;
    }

    return SOC_E_NONE;
}
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_TRIUMPH_SUPPORT
/*
 * Function:
 *      _soc_counter_tr_non_dma_init
 * Purpose:
 *      Initialize Triumph's non-DMA counters.
 * Parameters:
 *      unit - The SOC unit number
 *      nports - Number of ports.
 *      non_dma_start_index - The starting index of non-DMA counter entries.
 *      non_dma_entries - (OUT) The number of non-DMA counter entries.
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_MEMORY
 */
STATIC int
_soc_counter_tr_non_dma_init(int unit, int nports, int non_dma_start_index, 
        int *non_dma_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma0, *non_dma1;
    int max_cosq_per_port, total_num_cosq, port;
    int num_entries, alloc_size;
    uint32 *buf;

    soc = SOC_CONTROL(unit);
    *non_dma_entries = 0;

    max_cosq_per_port = 0;
    total_num_cosq = 0;
    for (port = 0; port < nports; port++) {
        if (num_cosq[unit][port] > max_cosq_per_port) {
            max_cosq_per_port = num_cosq[unit][port];
        }
        total_num_cosq += num_cosq[unit][port];
    }

    num_entries = soc_mem_index_count(unit, EGR_PERQ_XMT_COUNTERSm);
    alloc_size = num_entries *
        soc_mem_entry_words(unit, EGR_PERQ_XMT_COUNTERSm) * sizeof(uint32);

    buf = soc_cm_salloc(unit, alloc_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, alloc_size);

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_ALLOC;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = max_cosq_per_port;
    non_dma0->num_entries = num_entries;
    non_dma0->mem = EGR_PERQ_XMT_COUNTERSm;
    non_dma0->reg = INVALIDr;
    non_dma0->field = PACKET_COUNTERf;
    non_dma0->cname = "PERQ_PKT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_max[0] = num_entries - 1;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = BYTE_COUNTERf;
    non_dma1->cname = "PERQ_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_PERQ_REG;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = max_cosq_per_port;
    non_dma0->num_entries = total_num_cosq;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = DROP_PKT_CNTr;
    non_dma0->field = COUNTf;
    non_dma0->cname = "PERQ_DROP_PKT";
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = DROP_BYTE_CNTr;
    non_dma1->cname = "PERQ_DROP_BYTE";
    *non_dma_entries += non_dma1->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = nports;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = DROP_PKT_CNT_INGr;
    non_dma0->field = COUNTf;
    non_dma0->cname = "DROP_PKT_ING";
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_BYTE_ING -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = DROP_BYTE_CNT_INGr;
    non_dma1->cname = "DROP_BYTE_ING";
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = DROP_PKT_CNT_YELr;
    non_dma1->cname = "DROP_PKT_YEL";
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = DROP_PKT_CNT_REDr;
    non_dma1->cname = "DROP_PKT_RED";
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_TX_LLFC_MSG_CNT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->pbmp = PBMP_ALL(unit);
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->mem = INVALIDm;
    non_dma1->reg = TXLLFCMSGCNTr;
    non_dma1->field = TXLLFCMSGCNT_STATSf;
    non_dma1->cname = "MAC_TXLLFCMSG";
    non_dma1->entries_per_port = 1;
    non_dma1->num_entries = nports;
    *non_dma_entries += non_dma1->num_entries;
    return SOC_E_NONE;
}
#endif /* BCM_TRIUMPH_SUPPORT */

#ifdef BCM_SCORPION_SUPPORT
/*
 * Function:
 *      _soc_counter_sc_non_dma_init
 * Purpose:
 *      Initialize Scorpion's non-DMA counters.
 * Parameters:
 *      unit - The SOC unit number
 *      nports - Number of ports
 *      non_dma_start_index - The starting index of non-DMA counter entries.
 *      non_dma_entries - (OUT) The number of non-DMA counter entries.
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
_soc_counter_sc_non_dma_init(int unit, int nports, int non_dma_start_index, 
        int *non_dma_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma0, *non_dma1;
    int max_cosq_per_port, total_num_cosq, port;

    soc = SOC_CONTROL(unit);
    *non_dma_entries = 0;

    max_cosq_per_port = 0;
    total_num_cosq = 0;
    for (port = 0; port < nports; port++) {
        if (num_cosq[unit][port] > max_cosq_per_port) {
            max_cosq_per_port = num_cosq[unit][port];
        }
        total_num_cosq += num_cosq[unit][port];
    }

    /* EGR_PERQ_XMT_COUNTERS register support in scorpion */
    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma0->pbmp = PBMP_PORT_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = SOC_REG_NUMELS(unit, EGR_PERQ_XMT_COUNTERSr);
    non_dma0->num_entries = nports * non_dma0->entries_per_port;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = EGR_PERQ_XMT_COUNTERSr;
    non_dma0->field = PACKET_COUNTERf;
    non_dma0->cname = "PERQ_PKT";
    *non_dma_entries += non_dma0->num_entries;


    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_PERQ_REG;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = max_cosq_per_port;
    non_dma0->num_entries = total_num_cosq;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = HOLDROP_PKT_CNTr;
    non_dma0->field = COUNTf;
    non_dma0->cname = "PERQ_DROP_PKT";
    *non_dma_entries += non_dma0->num_entries;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_IBP -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = nports;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = IBP_DROP_PKT_CNTr;
    non_dma0->field = COUNTf;
    non_dma0->cname = "DROP_PKT_IBP";
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_CFAP -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = CFAP_DROP_PKT_CNTr;
    non_dma1->cname = "DROP_PKT_CFAP";
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = YELLOW_CNG_DROP_CNTr;
    non_dma1->cname = "DROP_PKT_YEL";
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = RED_CNG_DROP_CNTr;
    non_dma1->cname = "DROP_PKT_RED";
    *non_dma_entries += non_dma1->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_TX_LLFC_MSG_CNT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->pbmp = PBMP_ALL(unit);
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->mem = INVALIDm;
    non_dma1->reg = TXLLFCMSGCNTr;
    non_dma1->field = TXLLFCMSGCNT_STATSf;
    non_dma1->cname = "MAC_TXLLFCMSG";
    non_dma1->entries_per_port = 1;
    non_dma1->num_entries = nports;
    *non_dma_entries += non_dma1->num_entries;

    return SOC_E_NONE;
}
#endif /* BCM_SCORPION_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
/*
 * Function:
 *      _soc_counter_shadow_non_dma_init
 * Purpose:
 *      Initialize Shadow's non-DMA counters.
 * Parameters:
 *      unit - The SOC unit number
 *      nports - Number of ports.
 *      non_dma_start_index - The starting index of non-DMA counter entries.
 *      non_dma_entries - (OUT) The number of non-DMA counter entries.
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_MEMORY
 */
STATIC int
_soc_counter_shadow_non_dma_init(int unit, int nports, int non_dma_start_index, 
        int *non_dma_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma0,*non_dma1;
    int num_entries,alloc_size;
    uint32 *buf;

    soc = SOC_CONTROL(unit);
    *non_dma_entries = 0;
    
    /* Reusing non_dma id for interlaken counters */

    /* IL_STAT_MEM_0 Entries */
    num_entries = soc_mem_index_count(unit, IL_STAT_MEM_0m);
    alloc_size = num_entries *
        soc_mem_entry_words(unit, IL_STAT_MEM_0m) * sizeof(uint32);

    buf = soc_cm_salloc(unit, alloc_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, alloc_size);


    /* IL_STAT_MEM_0: RX_STAT_PKT_COUNTf */
    /* Overload SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT */ 
    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_ALLOC;
    non_dma0->pbmp = PBMP_IL_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = 1;
    non_dma0->mem = IL_STAT_MEM_0m;
    non_dma0->reg = INVALIDr;
    non_dma0->field = RX_STAT_PKT_COUNTf;
    non_dma0->cname = "IL_RX_PKTCNT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_min[0] = 0;
    non_dma0->dma_index_max[0] = num_entries - 1;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    /* IL_STAT_MEM_0: RX_STAT_BYTE_COUNTf */
    /* Overload SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE */ 
    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_BYTE -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = RX_STAT_BYTE_COUNTf;
    non_dma1->cname = "IL_RX_BYTECNT";
    *non_dma_entries += non_dma1->num_entries;

    /* IL_STAT_MEM_0: RX_STAT_BAD_PKT_ILERR_COUNTf */
    /* Overload SOC_COUNTER_NON_DMA_COSQ_DROP_PKT */ 
    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = RX_STAT_BAD_PKT_ILERR_COUNTf;
    non_dma1->cname = "IL_RX_ERRCNT";
    *non_dma_entries += non_dma1->num_entries;


    /* IL_STAT_MEM_1 Entries */
    num_entries = soc_mem_index_count(unit, IL_STAT_MEM_1m);
    alloc_size = num_entries *
        soc_mem_entry_words(unit, IL_STAT_MEM_1m) * sizeof(uint32);

    buf = soc_cm_salloc(unit, alloc_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, alloc_size);


    /* IL_STAT_MEM_1:  RX_STAT_GTMTU_PKT_COUNTf Greater that MTU size*/
    /* Overload SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE */ 
    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_ALLOC;
    non_dma0->pbmp = PBMP_IL_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = 1;
    non_dma0->mem = IL_STAT_MEM_1m;
    non_dma0->reg = INVALIDr;
    non_dma0->field =  RX_STAT_GTMTU_PKT_COUNTf;
    non_dma0->cname = "IL_RX_GTMTUCNT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_min[0] = 0;
    non_dma0->dma_index_max[0] = num_entries - 1;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    /* IL_STAT_MEM_1: RX_STAT_EQMTU_PKT_COUNTf - equal to MTU Size*/
    /* Overload SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC */ 
    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = RX_STAT_EQMTU_PKT_COUNTf;
    non_dma1->cname = "IL_RX_EQMTUCNT";
    *non_dma_entries += non_dma1->num_entries;


    /* IL_STAT_MEM_1: RX_STAT_IEEE_CRCERR_PKT_COUNTf - IEEE CRC err count*/
    /* Overload SOC_COUNTER_NON_DMA_COSQ_DROP_PKT_UC */ 
    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_COSQ_DROP_BYTE_UC -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = RX_STAT_IEEE_CRCERR_PKT_COUNTf;
    non_dma1->cname = "IL_RX_CRCERRCNT";
    *non_dma_entries += non_dma1->num_entries;

    /* IL_STAT_MEM_2 Entries */
    num_entries = soc_mem_index_count(unit, IL_STAT_MEM_2m);
    alloc_size = num_entries *
        soc_mem_entry_words(unit, IL_STAT_MEM_2m) * sizeof(uint32);

    buf = soc_cm_salloc(unit, alloc_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, alloc_size);

    /* IL_STAT_MEM_2:  RX_STAT_PKT_COUNTf Rx stat packet count*/
    /* Overload SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING */ 
    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_ALLOC;
    non_dma0->pbmp = PBMP_IL_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = 1;
    non_dma0->mem = IL_STAT_MEM_2m;
    non_dma0->reg = INVALIDr;
    non_dma0->field =  RX_STAT_PKT_COUNTf;
    non_dma0->cname = "IL_RX_STATCNT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_min[0] = 0;
    non_dma0->dma_index_max[0] = num_entries - 1;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    /* IL_STAT_MEM_3 Entries */
    num_entries = soc_mem_index_count(unit, IL_STAT_MEM_3m);
    alloc_size = num_entries *
        soc_mem_entry_words(unit, IL_STAT_MEM_3m) * sizeof(uint32);

    buf = soc_cm_salloc(unit, alloc_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, alloc_size);

    /* IL_STAT_MEM_3:  TX_STAT_PKT_COUNT */
    /* Overload SOC_COUNTER_NON_DMA_PORT_DROP_BYTE_ING */ 
    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_BYTE_ING -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_ALLOC;
    non_dma0->pbmp = PBMP_IL_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = 1;
    non_dma0->mem = IL_STAT_MEM_3m;
    non_dma0->reg = INVALIDr;
    non_dma0->field =  TX_STAT_PKT_COUNTf;
    non_dma0->cname = "IL_TX_PKTCNT";
    non_dma0->dma_buf[0] = buf;
    non_dma0->dma_index_min[0] = 0;
    non_dma0->dma_index_max[0] = 1;
    non_dma0->dma_mem[0] = non_dma0->mem;
    *non_dma_entries += non_dma0->num_entries;

    /* IL_STAT_MEM_3: TX_STAT_BYTE_COUNTf */
    /* Overload SOC_COUNTER_NON_DMA_PORT_DROP_PKT_IBP */ 
    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_IBP -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = TX_STAT_BYTE_COUNTf;
    non_dma1->cname = "IL_TX_BYTECNT";
    *non_dma_entries += non_dma1->num_entries;

    /* IL_STAT_MEM_3: TX_STAT_BAD_PKT_PERR_COUNTf */
    /* Overload SOC_COUNTER_NON_DMA_PORT_DROP_PKT_CFAP */ 
    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_CFAP -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = TX_STAT_BAD_PKT_PERR_COUNTf;
    non_dma1->cname = "IL_TX_ERRCNT";
    *non_dma_entries += non_dma1->num_entries;

    /* IL_STAT_MEM_3: TX_STAT_GTMTU_PKT_COUNTf */
    /* Overload SOC_COUNTER_NON_DMA_PORT_DROP_PKT */ 
    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = TX_STAT_GTMTU_PKT_COUNTf;
    non_dma1->cname = "IL_TX_GTMTUCNT";
    *non_dma_entries += non_dma1->num_entries;

    /* IL_STAT_MEM_3: TX_STAT_EQMTU_PKT_COUNT - Greater than mtu size */
    /* Overload SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW */ 
    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->field = TX_STAT_EQMTU_PKT_COUNTf;
    non_dma1->cname = "IL_TX_EQMTUCNT";
    *non_dma_entries += non_dma1->num_entries;

    /* IL_STAT_MEM_4 Entries */
    /* IL_STAT_MEM_4 overloaded on SOC_COUNTER_NON_DMA_EGR_PERQ_XMT_PKT */ 
    num_entries = soc_mem_index_count(unit, IL_STAT_MEM_4m);
    alloc_size = num_entries *
        soc_mem_entry_words(unit, IL_STAT_MEM_4m) * sizeof(uint32);

    buf = soc_cm_salloc(unit, alloc_size, "non_dma_counter");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(buf, 0, alloc_size);

    /* TX_STAT_PKT_COUNT */ 
    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma1->flags = _SOC_COUNTER_NON_DMA_VALID |
        _SOC_COUNTER_NON_DMA_DO_DMA | _SOC_COUNTER_NON_DMA_ALLOC;
    non_dma1->pbmp = PBMP_IL_ALL(unit);
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->entries_per_port = 1;
    non_dma1->num_entries = 1;
    non_dma1->mem = IL_STAT_MEM_4m;
    non_dma1->reg = INVALIDr;
    non_dma1->field = TX_STAT_PKT_COUNTf;
    non_dma1->cname = "IL_TX_STATCNT";
    non_dma1->dma_buf[0] = buf;
    non_dma1->dma_index_min[0] = 0;
    non_dma1->dma_index_max[0] = num_entries - 1;
    non_dma1->dma_mem[0] = non_dma1->mem;
    *non_dma_entries += non_dma1->num_entries;

    return SOC_E_NONE;
}
#endif /* BCM_SHADOW_SUPPORT */

#ifdef BCM_BRADLEY_SUPPORT
/*
 * Function:
 *      _soc_counter_hb_non_dma_init
 * Purpose:
 *      Initialize Bradley's non-DMA counters.
 * Parameters:
 *      unit - The SOC unit number
 *      nports - Number of ports
 *      non_dma_start_index - The starting index of non-DMA counter entries.
 *      non_dma_entries - (OUT) The number of non-DMA counter entries.
 * Returns:
 *      SOC_E_NONE
 */
STATIC int 
_soc_counter_hb_non_dma_init(int unit, int nports, int non_dma_start_index, 
        int *non_dma_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma;

    soc = SOC_CONTROL(unit);
    *non_dma_entries = 0;

    non_dma = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT -
                                    SOC_COUNTER_NON_DMA_START];
    non_dma->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma->pbmp = PBMP_ALL(unit);
    non_dma->base_index = non_dma_start_index + *non_dma_entries;
    non_dma->entries_per_port = 1;
    non_dma->num_entries = nports;
    non_dma->mem = INVALIDm;
    non_dma->reg = DROP_PKT_CNTr;
    non_dma->field = COUNTf;
    non_dma->cname = "DROP_PKT_MMU";
    *non_dma_entries += non_dma->num_entries;

    return SOC_E_NONE;
}
#endif /* BCM_BRADLEY_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
/*
 * Function:
 *      _soc_counter_fb_non_dma_init
 * Purpose:
 *      Initialize Firebolt's non-DMA counters.
 * Parameters:
 *      unit - The SOC unit number
 *      nports - Number of ports
 *      non_dma_start_index - The starting index of non-DMA counter entries.
 *      non_dma_entries - (OUT) The number of non-DMA counter entries.
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
_soc_counter_fb_non_dma_init(int unit, int nports, int non_dma_start_index,
        int *non_dma_entries)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma0, *non_dma1;

    soc = SOC_CONTROL(unit);
    *non_dma_entries = 0;

    non_dma0 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_YELLOW -
                                     SOC_COUNTER_NON_DMA_START];
    non_dma0->flags = _SOC_COUNTER_NON_DMA_VALID;
    non_dma0->pbmp = PBMP_ALL(unit);
    non_dma0->base_index = non_dma_start_index + *non_dma_entries;
    non_dma0->entries_per_port = 1;
    non_dma0->num_entries = nports;
    non_dma0->mem = INVALIDm;
    non_dma0->reg = CNGDROPCOUNT1r;
    non_dma0->field = DROPPKTCOUNTf;
    non_dma0->cname = "DROP_PKT_YEL";
    *non_dma_entries += non_dma0->num_entries;

    non_dma1 = &soc->counter_non_dma[SOC_COUNTER_NON_DMA_PORT_DROP_PKT_RED -
                                     SOC_COUNTER_NON_DMA_START];
    sal_memcpy(non_dma1, non_dma0, sizeof(soc_counter_non_dma_t));
    non_dma1->base_index = non_dma_start_index + *non_dma_entries;
    non_dma1->reg = CNGDROPCOUNT0r;
    non_dma1->cname = "DROP_PKT_RED";
    *non_dma_entries += non_dma1->num_entries;

    return SOC_E_NONE;
}
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_SBUSDMA_SUPPORT
static sbusdma_desc_handle_t *_soc_blk_counter_handles[SOC_MAX_NUM_DEVICES] = {NULL};
static soc_blk_ctr_process_t **_blk_ctr_process[SOC_MAX_NUM_DEVICES] = {NULL};
void _soc_sbusdma_blk_ctr_cb(int unit, int status, sbusdma_desc_handle_t handle, 
                             void *data)
{
    uint16 i, j, arr, bufidx, hwidx;
    uint64 ctr_new, ctr_prev, ctr_diff;
    int wide, width, f;
    volatile uint64 *vptr;
    uint32 *hwptr;
    soc_control_t *soc = SOC_CONTROL(unit);
    soc_reg_t ctr_reg;
    soc_blk_ctr_process_t *ctr_process = _blk_ctr_process[unit][PTR_TO_INT(data)];
    soc_cm_debug(DK_COUNTER+DK_VERBOSE, "In blk counter cb [%d]\n", handle);
    if (status == SOC_E_NONE) {
        soc_cm_debug(DK_COUNTER+DK_VERBOSE, "Complete: blk:%d, index: %d, entries: %d.\n", 
                     ctr_process->blk, ctr_process->bindex, ctr_process->entries);
        /* Process data (need bindex, count) */
        hwptr = (uint32*)ctr_process->buff;
        for (i=0, bufidx = 0, hwidx = 0; i<ctr_process->entries; i++) {
            arr = soc->blk_ctr_desc[ctr_process->bindex].desc[i].entries;
            ctr_reg = soc->blk_ctr_desc[ctr_process->bindex].desc[i].reg;
            wide = (soc->blk_ctr_desc[ctr_process->bindex].desc[i].width > 1) ? 1 : 0;
            for (j=0; j<arr; j++, bufidx++, hwidx++) {
                uint32 *ptr = &hwptr[hwidx];
                if (wide) {
                    if (soc->counter_flags & SOC_COUNTER_F_SWAP64) {
                        COMPILER_64_SET(ctr_new, ptr[0], ptr[1]);
                    } else {
                        COMPILER_64_SET(ctr_new, ptr[1], ptr[0]);
                    }
                    hwidx++;
                } else {
                    COMPILER_64_SET(ctr_new, 0, ptr[0]);
                }
                if (!COMPILER_64_IS_ZERO(ctr_new)) {
                    soc_cm_debug(DK_COUNTER+DK_VERBOSE, "idx: %d, bufidx: %d, :val: %x_%x\n", 
                                 i, bufidx, COMPILER_64_HI(ctr_new),
                                 COMPILER_64_LO(ctr_new));
                }
                ctr_prev = ctr_process->hwval[bufidx];
                vptr = &ctr_process->swval[bufidx];                    
                if (COMPILER_64_EQ(ctr_new, ctr_prev)) {
                    /* clear delta. (not maintaining delta for now) */
                } else {
                    ctr_diff = ctr_new;
                    if (COMPILER_64_LT(ctr_diff, ctr_prev)) { /* handle rollover */
                        uint64 wrap_amt;
                        f = 0;
                        width = SOC_REG_INFO(unit, ctr_reg).fields[f].len;
                        while((SOC_REG_INFO(unit, ctr_reg).fields + f) != NULL) { 
                            if (SOC_REG_INFO(unit, ctr_reg).fields[f].field == 
                                COUNTf) {
                                width = SOC_REG_INFO(unit, ctr_reg).fields[f].len;
                                break;
                            }
                            f++;
                        }
                        if (width < 32) {
                            COMPILER_64_SET(wrap_amt, 0, 1UL << width);
                            COMPILER_64_ADD_64(ctr_diff, wrap_amt);
                        } else if (width < 64) {
                            COMPILER_64_SET(wrap_amt, 1UL << (width - 32), 0);
                            COMPILER_64_ADD_64(ctr_diff, wrap_amt);
                        }
                    }
                    COMPILER_64_SUB_64(ctr_diff, ctr_prev);
                    ctr_process->hwval[bufidx] = ctr_new;
                    COMPILER_64_ADD_64(*vptr, ctr_diff);
                }
            }
        }
    } else {
        soc_cm_debug(DK_ERR, "Counter SBUSDMA failed: blk:%d, index: %d, entries: %d.\n", 
                     ctr_process->blk, ctr_process->bindex, ctr_process->entries);
        if (status == SOC_E_TIMEOUT) {
            /* delete this handles desc */
            (void)soc_sbusdma_desc_delete(unit, handle);
            _soc_blk_counter_handles[unit][ctr_process->bindex] = 0;
        }
    }
}
#endif

/*
 * Function:
 *      soc_counter_attach
 * Purpose:
 *      Initialize counter module.
 * Notes:
 *      Allocates counter collection buffers.
 *      We need to work with the true data for the chip, not the current
 *      pbmp_valid settings.  They may be changed after attach, but the
 *      memory won't be reallocated at that time.
 */

int
soc_counter_attach(int unit)
{
    soc_control_t       *soc;
    int                 n_entries, n_bytes;
    int                 non_dma_entries = 0;
    int                 portsize, nports, ports64;
    int                 blk, bindex, port, phy_port;
    int                 blktype, ctype, idx;
    uint32 array_idx;
    char *counter_level;

    assert(SOC_UNIT_VALID(unit));

    soc = SOC_CONTROL(unit);

    soc->counter_pid = SAL_THREAD_ERROR;
    soc->counter_interval = 0;
    SOC_PBMP_CLEAR(soc->counter_pbmp);
    soc->counter_trigger = NULL;
    soc->counter_intr = NULL;
    /* Note that flags will be reinitialized in soc_counter_start */
    if (soc_feature(unit, soc_feature_stat_dma) && !SOC_IS_RCPU_ONLY(unit)) {
        soc->counter_flags = SOC_COUNTER_F_DMA;
    } else {
        soc->counter_flags = 0;
    }
    soc->counter_coll_prev = soc->counter_coll_cur = sal_time_usecs();

    /*For soc_feature_controlled_counters feture (use only the relevant)*/
    if (soc_feature(unit, soc_feature_controlled_counters)) {
        counter_level = soc_property_get_str(unit, spn_SOC_COUNTER_CONTROL_LEVEL);
        if(NULL == counter_level) {
            soc->soc_controlled_counter_flags |= _SOC_CONTROLLED_COUNTER_FLAG_HIGH;
        } else if (sal_strcmp(counter_level, "HIGH_LEVEL")) {
            soc->soc_controlled_counter_flags |= _SOC_CONTROLLED_COUNTER_FLAG_HIGH;
        } else if (sal_strcmp(counter_level, "MEDIUM_LEVEL")) {
            soc->soc_controlled_counter_flags |= (_SOC_CONTROLLED_COUNTER_FLAG_HIGH | _SOC_CONTROLLED_COUNTER_FLAG_MEDIUM);
        } else if (sal_strcmp(counter_level, "LOW_LEVEL")) {
            soc->soc_controlled_counter_flags |= (_SOC_CONTROLLED_COUNTER_FLAG_HIGH | _SOC_CONTROLLED_COUNTER_FLAG_MEDIUM | _SOC_CONTROLLED_COUNTER_FLAG_LOW);
        } else { /*default*/
            soc_cm_debug(DK_ERR,
                     "soc_counter_attach: unit %d:illegal counter level \n", unit);
            return SOC_E_FAIL;
        }

        /*count the relevant counters (according to level)*/
        soc->soc_controlled_counter_num = 0;
        soc->soc_controlled_counter_all_num = 0;
        for (array_idx = 0; soc->controlled_counters[array_idx].controlled_counter_f != NULL; array_idx++) {

#ifdef BCM_ARAD_SUPPORT
            if (SOC_IS_ARAD(unit)) {
                /*do not collect NIF counters - the register are not cleared on read*/
                if (soc->controlled_counters[array_idx].flags & _SOC_CONTROLLED_COUNTER_FLAG_NIF) {
                    continue;
                }
            }
#endif 
            (soc->soc_controlled_counter_all_num)++;
            if (soc->controlled_counters[array_idx].flags & soc->soc_controlled_counter_flags) {
                (soc->soc_controlled_counter_num)++;
                soc->controlled_counters[array_idx].counter_idx = array_idx;
            }
        }
    }

    portsize = 0;
    port = 0;
    /* We can't use pbmp_valid calculations, so we must do this manually. */
    for (phy_port = 0; ; phy_port++) {
        blk = SOC_PORT_BLOCK(unit, phy_port);
        bindex = SOC_PORT_BINDEX(unit, phy_port);
        if (blk < 0 && bindex < 0) {                    /* end of list */
            break;
        }
        if (soc_feature(unit, soc_feature_logical_port_num)) {
            if (port < SOC_INFO(unit).port_p2l_mapping[phy_port]) {
                port = SOC_INFO(unit).port_p2l_mapping[phy_port];
            }
        } else {
            port = phy_port;
        }
    }
    port++;

    /* Don't count last port if CMIC */
    if (CMIC_PORT(unit) == (port - 1)) {
        /*
         * On XGS3 switch CMIC port stats are DMAable. Allocate space
         * in case these stats need to be DMAed as well.
         */
        if (!soc_feature(unit, soc_feature_cpuport_stat_dma)) {
            port--;      
        }
    }
#if defined(BCM_PETRAB_SUPPORT)
    if (SOC_IS_PETRAB(unit)) {
        SOC_PBMP_COUNT(PBMP_ALL(unit), port);
    }
#endif

    nports = port;      /* 0..max_port_num */
    ports64 = 0;

    if (SOC_IS_HB_GW(unit)) {
        portsize = 2048;
        ports64 = nports;
    } else if (SOC_IS_TRX(unit)) {
        portsize = 4096;
        ports64 = nports;
    } else if (SOC_IS_HERCULES(unit) || SOC_IS_XGS3_SWITCH(unit)) {
        portsize = 1024;
        ports64 = nports;
#if defined(BCM_SIRIUS_SUPPORT)
    } else if (SOC_IS_SIRIUS(unit)) {
        portsize = 2048;
        ports64 = nports;
#endif
#if defined(BCM_PETRAB_SUPPORT)
    } else if (SOC_IS_PETRAB(unit)) {
        ports64 = nports;
        portsize = sizeof(uint64) * SOC_PB_NIF_NOF_COUNTERS;
#endif
#if defined(BCM_ARAD_SUPPORT)
    } else if (SOC_IS_ARAD(unit)) {
        ports64 = nports;
        portsize = sizeof(uint64) * (soc->soc_controlled_counter_all_num + 160 /*nof non controlled counters*/);
#endif
#if defined(BCM_DFE_SUPPORT)
    } else if (SOC_IS_FE1600(unit)) {
        ports64 = nports;
        portsize = sizeof(uint64)*(soc->soc_controlled_counter_all_num);
#endif
#if defined(BCM_CALADAN3_SUPPORT)
        
    } else if (SOC_IS_CALADAN3(unit)) {
        portsize = 2048;
        ports64 = nports;
#endif
    } else {
        soc_cm_debug(DK_ERR,
                     "soc_counter_attach: unit %d: unexpected chip type\n",
                     unit);
        return SOC_E_FAIL;
    }

    soc->counter_perport = portsize / sizeof(uint64);
    soc->counter_n32 = 0;
    soc->counter_n64 = (portsize / sizeof(uint64)) * ports64;
    soc->counter_ports32 = 0;
    if(soc->counter_flags & SOC_COUNTER_F_DMA) {
        soc->counter_ports64 = ports64;
    }
    n_bytes = portsize * nports;

    n_entries = soc->counter_n32 + soc->counter_n64;
    soc->counter_portsize = portsize;
    soc->counter_bufsize = n_bytes;

    soc_cm_debug(DK_VERBOSE,
                 "soc_counter_attach: %d bytes/port, %d ports, %d ctrs/port, "
                 "%d ports with %d ctrs\n",
                 portsize, nports, soc->counter_perport,
                 ports64, soc->counter_n64);

    /* Initialize Non-DMA counters */

    if (soc->counter_non_dma == NULL) {
        soc->counter_non_dma = sal_alloc((SOC_COUNTER_NON_DMA_END -
                                         SOC_COUNTER_NON_DMA_START) *
                                         sizeof(soc_counter_non_dma_t),
                                         "cntr_non_dma");
        if (soc->counter_non_dma == NULL) {
            goto error;
        }
    } else {
        for (idx = 0; idx < SOC_COUNTER_NON_DMA_END - SOC_COUNTER_NON_DMA_START;
             idx++) {
            if (soc->counter_non_dma[idx].flags & _SOC_COUNTER_NON_DMA_ALLOC) {
                soc_cm_sfree(unit, soc->counter_non_dma[idx].dma_buf[0]);
            }
        }
    }
    sal_memset(soc->counter_non_dma, 0,
               (SOC_COUNTER_NON_DMA_END - SOC_COUNTER_NON_DMA_START) *
               sizeof(soc_counter_non_dma_t));

    /* Controlled counters */
    if (!soc_feature(unit, soc_feature_controlled_counters)) {
        soc->controlled_counters = NULL;
    }

    /* Initialize the number of cosq counters per port */
    _soc_counter_num_cosq_init(unit, nports);

#ifdef BCM_CALADAN3_SUPPORT
    if (SOC_IS_CALADAN3(unit)) {
        non_dma_entries = 0;
    } else 
#endif

#ifdef BCM_TRIUMPH_SUPPORT
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        if (SOC_FAILURE(_soc_counter_triumph3_non_dma_init(unit, 
                            nports, n_entries, &non_dma_entries))) {
            goto error;
        }
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        if (SOC_FAILURE(_soc_counter_trident2_non_dma_init(unit, 
                            nports, n_entries, &non_dma_entries))) {
            goto error;
        }
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        if (SOC_FAILURE(_soc_counter_trident_non_dma_init(unit, nports, n_entries, 
                        &non_dma_entries))) {
            goto error;
        }
    } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_HURRICANE_SUPPORT
    if (SOC_IS_HURRICANEX(unit)) {
        if (SOC_FAILURE(_soc_counter_hu_non_dma_init(unit, nports, n_entries, 
                        &non_dma_entries))) {
            goto error;
        }
    } else
#endif /* BCM_HURRICANE_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANAX(unit)) {
        if (SOC_FAILURE(_soc_counter_katana_non_dma_init(unit, nports, n_entries, 
                        &non_dma_entries))) {
            goto error;
        }
    } else
#endif /* BCM_KATANA_SUPPORT */
    if (SOC_IS_TR_VL(unit)) {
        if (SOC_FAILURE(_soc_counter_tr_non_dma_init(unit, nports, n_entries, 
                        &non_dma_entries))) {
            goto error;
        }
    } else
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_SCORPION_SUPPORT
    if (SOC_IS_SC_CQ(unit) && (!(SOC_IS_SHADOW(unit)))) {
        if (SOC_FAILURE(_soc_counter_sc_non_dma_init(unit, nports, n_entries, 
                        &non_dma_entries))) {
            goto error;
        }
    } else
#endif /* BCM_SCORPION_SUPPORT */
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        if (!SOC_PBMP_IS_NULL(PBMP_IL_ALL(unit))) {
            if (SOC_FAILURE(_soc_counter_shadow_non_dma_init(unit, nports, 
                            n_entries, &non_dma_entries))) {
                goto error;
            }
        }
    } else
#endif /* BCM_SHADOW_SUPPORT */
#ifdef BCM_BRADLEY_SUPPORT
    if (SOC_IS_HB_GW(unit)) {
        if (SOC_FAILURE(_soc_counter_hb_non_dma_init(unit, nports, n_entries, 
                        &non_dma_entries))) {
            goto error;
        }
    } else
#endif /* BCM_BRADLEY_SUPPORT */
#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FB_FX_HX(unit)) {
        if (SOC_FAILURE(_soc_counter_fb_non_dma_init(unit, nports, n_entries, 
                        &non_dma_entries))) {
            goto error;
        }
    } else
#endif /* BCM_FIREBOLT_SUPPORT */
    {
        non_dma_entries = 0;
    }

    soc->counter_n64_non_dma = non_dma_entries;

    /*
     * Counter DMA buffer allocation (32 and 64 bit allocated together)
     */

    assert(n_bytes > 0);

    /* Hardware DMA buf */
    if(soc->counter_flags & SOC_COUNTER_F_DMA) {
        if (soc->counter_buf32 == NULL) {
            soc->counter_buf32 = soc_cm_salloc(unit, n_bytes, "cntr_dma_buf");
            if (soc->counter_buf32 == NULL) {
                goto error;
            }
            soc->counter_buf64 = (uint64 *)&soc->counter_buf32[soc->counter_n32];
        }
        sal_memset(soc->counter_buf32, 0, n_bytes);
    }

    n_entries += non_dma_entries;

    /* Hardware value buf */
    if (soc->counter_hw_val != NULL) {
        sal_free(soc->counter_hw_val);
        soc->counter_hw_val = NULL;
    }
    if (soc->counter_hw_val == NULL) {
        soc->counter_hw_val = sal_alloc(n_entries * sizeof(uint64),
                                        "cntr_hw_val");
        if (soc->counter_hw_val == NULL) {
            goto error;
        }
    }
    sal_memset(soc->counter_hw_val, 0, n_entries * sizeof(uint64));

    /* Software value buf */
    if (soc->counter_sw_val != NULL) {
        sal_free(soc->counter_sw_val);
        soc->counter_sw_val = NULL;
    }
    if (soc->counter_sw_val == NULL) {
        soc->counter_sw_val = sal_alloc(n_entries * sizeof(uint64),
                                        "cntr_sw_val");
        if (soc->counter_sw_val == NULL) {
            goto error;
        }
    }
    sal_memset(soc->counter_sw_val, 0, n_entries * sizeof(uint64));

    /* Delta buf */
    if (soc->counter_delta != NULL) {
        sal_free(soc->counter_delta);
        soc->counter_delta = NULL;
    }
    if (soc->counter_delta == NULL) {
        soc->counter_delta = sal_alloc(n_entries * sizeof(uint64),
                                       "cntr_delta");
        if (soc->counter_delta == NULL) {
            goto error;
        }
    }
    sal_memset(soc->counter_delta, 0, n_entries * sizeof(uint64));

#ifdef BCM_SBUSDMA_SUPPORT
    _soc_counter_pending[unit] = 0;
    soc_counter_mib_offset[unit] = 0;
#endif

    /* Set up and install counter maps */

    /* We can't use pbmp_valid calculations, so we must do this manually. */
    for (phy_port = 0; ; phy_port++) {
        blk = SOC_PORT_BLOCK(unit, phy_port);
        bindex = SOC_PORT_BINDEX(unit, phy_port);
        if (blk < 0 && bindex < 0) {                    /* end of list */
            break;
        }
        if (blk < 0) {                                  /* empty slot */
            continue;       
        }
        if (soc_feature(unit, soc_feature_logical_port_num)) {
            port = SOC_INFO(unit).port_p2l_mapping[phy_port];
            if (port < 0) {
                continue;
            }
        } else {
            port = phy_port;
            if (port >= nports) {
                continue;
            }
        }
        blktype = SOC_BLOCK_INFO(unit, blk).type;
        switch (blktype) {
        case SOC_BLK_EPIC:
            ctype = SOC_CTR_TYPE_FE;
            break;
        case SOC_BLK_GPIC:
        case SOC_BLK_GPORT:
        case SOC_BLK_QGPORT:
        case SOC_BLK_SPORT:
            ctype = SOC_CTR_TYPE_GE;
            break;
        case SOC_BLK_XQPORT:
        case SOC_BLK_XGPORT:
            if (IS_XE_PORT(unit, port) || IS_HG_PORT(unit, port)) {
                ctype = SOC_CTR_TYPE_XE;
            } else {
                ctype = SOC_CTR_TYPE_GE;
            }
            break;
        case SOC_BLK_IPIC:
        case SOC_BLK_HPIC:
            ctype = SOC_CTR_TYPE_HG;
            break;
        case SOC_BLK_XPIC:
        case SOC_BLK_XPORT:
        case SOC_BLK_GXPORT:
        case SOC_BLK_XLPORT:
        case SOC_BLK_XTPORT:
        case SOC_BLK_CLPORT:
        case SOC_BLK_MXQPORT:
        case SOC_BLK_XWPORT:
            ctype = SOC_CTR_TYPE_XE;
            break;
        case SOC_BLK_CLP:
        case SOC_BLK_XLP:
            if (IS_IL_PORT(unit, port)) {
                continue;
            } else if (IS_CE_PORT(unit, port)) {
                ctype = SOC_CTR_TYPE_CE;
            } else {
                ctype = SOC_CTR_TYPE_XE;
            }
            break;
        case SOC_BLK_CMIC:
            if (soc_feature(unit, soc_feature_cpuport_stat_dma)) {
                ctype = SOC_CTR_TYPE_CPU;
                break;
            }
        case SOC_BLK_CPIC:
        default:
            continue;
        }

        if (!SOC_CONTROL(unit)->counter_map[port]) {
            SOC_CONTROL(unit)->counter_map[port] =
                &SOC_CTR_DMA_MAP(unit, ctype);
            assert(SOC_CONTROL(unit)->counter_map[port]);
        }
        assert(SOC_CONTROL(unit)->counter_map[port]->cmap_base);

#ifdef BCM_TRIUMPH2_SUPPORT
        /* We need this done in a port loop like this one, so we're
         * piggybacking on it. */

        if (soc_feature(unit, soc_feature_timestamp_counter)) {
            /* A HW register implemented as a FIFO-pop on read is in the
             * counter collection space.  We must copy the DMA'd values 
             * into a SW FIFO and make them available to the application
             * on request.
             */
            if (soc->counter_timestamp_fifo[port] == NULL) {
                soc->counter_timestamp_fifo[port] =
                    sal_alloc(sizeof(shr_fifo_t), "Timestamp counter FIFO");
                if (soc->counter_timestamp_fifo[port] == NULL) {
                    goto error;
                }
            } else {
                SHR_FIFO_FREE(soc->counter_timestamp_fifo[port]);
            }
            sal_memset(soc->counter_timestamp_fifo[port], 0,
                       sizeof(shr_fifo_t));
            SHR_FIFO_ALLOC(soc->counter_timestamp_fifo[port],
                           COUNTER_TIMESTAMP_FIFO_SIZE, sizeof(uint32),
                           SHR_FIFO_FLAG_DO_NOT_PUSH_DUPLICATE);
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
    }
#ifdef BCM_SBUSDMA_SUPPORT
    /* Init blk counters */
    if (soc->blk_ctr_desc_count) {
        uint16 ctr;
        /* Calc size */
        n_entries = 0;
        for (idx = 0; idx < soc->blk_ctr_desc_count; idx++) {
            ctr = 0;
            while (soc->blk_ctr_desc[idx].desc[ctr].reg != INVALIDr) {
                n_entries += soc->blk_ctr_desc[idx].desc[ctr].entries;
                ctr++;
            } 
        }
        soc->blk_ctr_count = n_entries;
        soc_cm_debug(DK_COUNTER, "Total ctr blks: %d, Total entries: %d\n", 
                     soc->blk_ctr_desc_count, n_entries);
        if (_soc_blk_counter_handles[unit] == NULL) {
            _soc_blk_counter_handles[unit] = sal_alloc(n_entries * sizeof(sbusdma_desc_handle_t), 
                                                 "blk_ctr_desc_handles");
            if (_soc_blk_counter_handles[unit] == NULL) {
                goto error;
            }
        }
        sal_memset(_soc_blk_counter_handles[unit], 0, n_entries * sizeof(sbusdma_desc_handle_t));
        
        if (soc->blk_ctr_buf == NULL) {
            soc->blk_ctr_buf = soc_cm_salloc(unit, n_entries * sizeof(uint64), 
                                             "blk_ctr_hw_buff");
            if (soc->blk_ctr_buf == NULL) {
                goto error;
            }
        }
        sal_memset(soc->blk_ctr_buf, 0, n_entries * sizeof(uint64));
        
        if (soc->blk_counter_hw_val == NULL) {
            soc->blk_counter_hw_val = sal_alloc(n_entries * sizeof(uint64),
                                                "blk_ctr_hw_vals");
            if (soc->blk_counter_hw_val == NULL) {
                goto error;
            }
        }
        sal_memset(soc->blk_counter_hw_val, 0, n_entries * sizeof(uint64));
        
        if (soc->blk_counter_sw_val == NULL) {
            soc->blk_counter_sw_val = sal_alloc(n_entries * sizeof(uint64),
                                                "blk_ctr_sw_vals");
            if (soc->blk_counter_sw_val == NULL) {
                goto error;
            }
        }
        sal_memset(soc->blk_counter_sw_val, 0, n_entries * sizeof(uint64));
    }
#endif
    return SOC_E_NONE;

 error:

#ifdef BCM_SBUSDMA_SUPPORT
    if (soc->blk_counter_sw_val != NULL) {
        sal_free(soc->blk_counter_sw_val);
        soc->blk_counter_sw_val = NULL;
    }
    
    if (soc->blk_counter_hw_val != NULL) {
        sal_free(soc->blk_counter_hw_val);
        soc->blk_counter_hw_val = NULL;
    }
    
    if (soc->blk_ctr_buf != NULL) {
        soc_cm_sfree(unit, soc->blk_ctr_buf);
        soc->blk_ctr_buf = NULL;
    }
    
    if (_soc_blk_counter_handles[unit] != NULL) {
        sal_free(_soc_blk_counter_handles[unit]);
        _soc_blk_counter_handles[unit] = NULL;
    }    
#endif

#ifdef BCM_TRIUMPH2_SUPPORT
    for (port = 0; port < SOC_MAX_NUM_PORTS; port++) {
        if (soc->counter_timestamp_fifo[port] != NULL) {
            SHR_FIFO_FREE(soc->counter_timestamp_fifo[port]);
            sal_free(soc->counter_timestamp_fifo[port]);
            soc->counter_timestamp_fifo[port] = NULL;
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    if (soc->counter_non_dma != NULL) {
        for (idx = 0;
             idx < SOC_COUNTER_NON_DMA_END - SOC_COUNTER_NON_DMA_START;
             idx++) {
            if (soc->counter_non_dma[idx].flags & _SOC_COUNTER_NON_DMA_ALLOC) {
                soc_cm_sfree(unit, soc->counter_non_dma[idx].dma_buf[0]);
            }
        }
        sal_free(soc->counter_non_dma);
        soc->counter_non_dma = NULL;
    }

    if (soc->counter_buf32 != NULL) {
        soc_cm_sfree(unit, soc->counter_buf32);
        soc->counter_buf32 = NULL;
        soc->counter_buf64 = NULL;
    }

    if (soc->counter_hw_val != NULL) {
        sal_free(soc->counter_hw_val);
        soc->counter_hw_val = NULL;
    }

    if (soc->counter_sw_val != NULL) {
        sal_free(soc->counter_sw_val);
        soc->counter_sw_val = NULL;
    }

    if (soc->counter_delta != NULL) {
        sal_free(soc->counter_delta);
        soc->counter_delta = NULL;
    }

    return SOC_E_MEMORY;
}

/*
 * Function:
 *      soc_port_cmap_get/set
 * Purpose:
 *      Access the counter map structure per port
 */

soc_cmap_t *
soc_port_cmap_get(int unit, soc_port_t port)
{
    if (!SOC_UNIT_VALID(unit)) {
        return NULL;
    }
    if (!SOC_PORT_VALID(unit, port) || 
        (!IS_PORT(unit, port) &&
         (!soc_feature(unit, soc_feature_cpuport_stat_dma)))) {
        return NULL;
    }
    return SOC_CONTROL(unit)->counter_map[port];
}

int
soc_port_cmap_set(int unit, soc_port_t port, soc_ctr_type_t ctype)
{
    if (!SOC_UNIT_VALID(unit)) {
        return SOC_E_UNIT;
    }
    if (!SOC_PORT_VALID(unit, port) ||
        (!IS_PORT(unit, port) &&
         (!soc_feature(unit, soc_feature_cpuport_stat_dma)))) {
        return SOC_E_PARAM;
    }
    SOC_CONTROL(unit)->counter_map[port] = &SOC_CTR_DMA_MAP(unit, ctype);
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_counter_detach
 * Purpose:
 *      Finalize counter module.
 * Notes:
 *      Stops counter task if running.
 *      Deallocates counter collection buffers.
 */

int
soc_counter_detach(int unit)
{
    soc_control_t       *soc;
    int                 i;
#ifdef BCM_TRIUMPH2_SUPPORT
    soc_port_t          port;
#endif /* BCM_TRIUMPH2_SUPPORT */

    assert(SOC_UNIT_VALID(unit));

    /*
     * COVERITY
     *
     * assert validates the input
     */
    /* coverity[overrun-local : FALSE] */
    soc = SOC_CONTROL(unit);

    /*
     * COVERITY
     *
     * assert validates the input
     */
    /* coverity[overrun-local : FALSE] */
    SOC_IF_ERROR_RETURN(soc_counter_stop(unit));

#ifdef BCM_TRIUMPH2_SUPPORT
    for (port = 0; port < SOC_MAX_NUM_PORTS; port++) {
        if (soc->counter_timestamp_fifo[port] != NULL) {
            SHR_FIFO_FREE(soc->counter_timestamp_fifo[port]);
            sal_free(soc->counter_timestamp_fifo[port]);
            soc->counter_timestamp_fifo[port] = NULL;
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    if (soc->counter_non_dma != NULL) {
        for (i = 0; i < SOC_COUNTER_NON_DMA_END - SOC_COUNTER_NON_DMA_START;
             i++) {
            if (soc->counter_non_dma[i].flags & _SOC_COUNTER_NON_DMA_ALLOC) {
                soc_cm_sfree(unit, soc->counter_non_dma[i].dma_buf[0]);
            }
        }
        sal_free(soc->counter_non_dma);
        soc->counter_non_dma = NULL;
    }

    if (soc->counter_buf32 != NULL) {
        soc_cm_sfree(unit, soc->counter_buf32);
        soc->counter_buf32 = NULL;
        soc->counter_buf64 = NULL;
    }

    if (soc->counter_hw_val != NULL) {
        sal_free(soc->counter_hw_val);
        soc->counter_hw_val = NULL;
    }

    if (soc->counter_sw_val != NULL) {
        sal_free(soc->counter_sw_val);
        soc->counter_sw_val = NULL;
    }

    if (soc->counter_delta != NULL) {
        sal_free(soc->counter_delta);
        soc->counter_delta = NULL;
    }

#ifdef BCM_SBUSDMA_SUPPORT
    if (_soc_blk_counter_handles[unit] != NULL) {
        sal_free(_soc_blk_counter_handles[unit]);
        _soc_blk_counter_handles[unit] = NULL;
    }
    
    if (soc->blk_ctr_buf != NULL) {
        soc_cm_sfree(unit, soc->blk_ctr_buf);
        soc->blk_ctr_buf = NULL;
    }

    if (soc->blk_counter_hw_val != NULL) {
        sal_free(soc->blk_counter_hw_val);
        soc->blk_counter_hw_val = NULL;
    }

    if (soc->blk_counter_sw_val != NULL) {
        sal_free(soc->blk_counter_sw_val);
        soc->blk_counter_sw_val = NULL;
    }
    
    _soc_counter_pending[unit] = 0;
#endif    
    return SOC_E_NONE;
}

/*
 * StrataSwitch counter register map
 *
 * NOTE: soc_attach verifies this map is correct and prints warnings if
 * not.  The map should contain only registers with a single field named
 * COUNT, and all such registers should be in the map.
 *
 * The soc_counter_map[] array is a list of counter registers in the
 * order found in the internal address map and counter DMA buffer.
 * soc_counter_map[0] corresponds to S-Channel address
 * COUNTER_OFF_MIN, and contains SOC_CTR_MAP_SIZE(unit) entries.
 *
 * This map structure, contents, and size are exposed only to provide a
 * convenient way to loop through all available counters.
 *
 * Accumulated counters are stored as 64-bit values and may be written
 * and read by multiple tasks on a 32-bit processor.  This requires
 * atomic operation to get correct values.  Also, to fetch-and-clear a
 * counter requires atomic operation.
 *
 * These atomic operations are very brief, so rather than using an
 * inefficient mutex we use splhi to lock out interrupts and task
 * switches.  In theory, splhi is only a few instructions on most
 * processors.
 */

#define COUNTER_ATOMIC_DEF              int
#define COUNTER_ATOMIC_BEGIN(s)         ((s) = sal_splhi())
#define COUNTER_ATOMIC_END(s)           (sal_spl(s))

#ifdef BROADCOM_DEBUG

char *soc_ctr_type_names[] = SOC_CTR_TYPE_NAMES_INITIALIZER;

/*
 * Function:
 *      _soc_counter_verify (internal)
 * Purpose:
 *      Verify contents of soc_counter_map[] array
 */
void
_soc_counter_verify(int unit)
{
    int                 i, errors, num_ctrs, found;
    soc_ctr_type_t      ctype;
    soc_reg_t           reg;
    int                 ar_idx;
    uint32              skip_offset = 0;

    soc_cm_debug(DK_VERBOSE, "soc_counter_verify: unit %d begins\n", unit);

    errors = 0;
    if (SOC_IS_TRIUMPH3(unit)) {
        skip_offset = 32;
    }
#if defined(BCM_HURRICANE2_SUPPORT)
    if (SOC_IS_HURRICANE2(unit)) {
        skip_offset = 55;
    }
#endif

    for (ctype = SOC_CTR_TYPE_FE; ctype < SOC_CTR_NUM_TYPES; ctype++) {
        if (SOC_HAS_CTR_TYPE(unit, ctype)) {
            num_ctrs = SOC_CTR_MAP_SIZE(unit, ctype);
            for (i = 0; i < num_ctrs; i++) {
                reg = SOC_CTR_TO_REG(unit, ctype, i);
                ar_idx = SOC_CTR_TO_REG_IDX(unit, ctype, i);
                if (SOC_COUNTER_INVALID(unit, reg)) {
                    continue;
                }
                if (!SOC_REG_IS_COUNTER(unit, reg)) {
                    soc_cm_debug(DK_VERBOSE,
                                 "soc_counter_verify: "
                                 "%s cntr %s (%d) index %d "
                                 "is not a counter\n",
                                 soc_ctr_type_names[ctype],
                                 SOC_REG_NAME(unit, reg), reg, i);
                    errors = 1;
                }
                if (((SOC_REG_CTR_IDX(unit, reg) + ar_idx) -
                     COUNTER_IDX_OFFSET(unit) - skip_offset)
                    != i) {
                    soc_cm_debug(DK_VERBOSE, "soc_counter_verify: "
                         "%s cntr %s (%d) index mismatch.\n"
                         "    (ctr_idx %d + ar_idx %d) - offset %d != "
                         "index in ctr array %d\n",
                                 soc_ctr_type_names[ctype],
                                 SOC_REG_NAME(unit, reg), reg,
                                 SOC_REG_CTR_IDX(unit, reg),
                                 ar_idx,
                                 COUNTER_IDX_OFFSET(unit),
                                 i);
                    errors = 1;
                }
            }
        }
    }

    for (reg = 0; reg < NUM_SOC_REG; reg++) {
        if (SOC_REG_IS_VALID(unit, reg) && SOC_REG_IS_COUNTER(unit, reg)) {
            found = FALSE;
            i = -1;
            for (ctype = SOC_CTR_TYPE_FE; ctype < SOC_CTR_NUM_TYPES; ctype++) {
                if (SOC_HAS_CTR_TYPE(unit, ctype)) {
                    num_ctrs = SOC_CTR_MAP_SIZE(unit, ctype);
                    for (i = 0; i < num_ctrs; i++) {
                        if (SOC_CTR_TO_REG(unit, ctype, i) == reg) {
                            if ((SOC_REG_CTR_IDX(unit, reg) -
                                 COUNTER_IDX_OFFSET(unit) - skip_offset) != i) {
                                soc_cm_debug(DK_VERBOSE,
                                   "soc_counter_verify: "
                                   "%s cntr %s (%d) index mismatch.\n"
                                   "    (ctr_idx %d - offset %d) != "
                                   "index in ctr array %d\n",
                                             soc_ctr_type_names[ctype],
                                             SOC_REG_NAME(unit, reg), reg,
                                             SOC_REG_CTR_IDX(unit, reg),
                                             COUNTER_IDX_OFFSET(unit),
                                             i);
                                errors = 1;
                            }
                            found = TRUE;
                            break;
                        }
                    }
                }
                if (found) {
                    break;
                }
            }

#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_KATANA_SUPPORT)
            if(SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit) || SOC_IS_KATANAX(unit)) {
                /* OAM_SEC_NS_COUNTER_64 is a general purpose counter 
                   which counts time for entire chip */    
                if ((!found) && (OAM_SEC_NS_COUNTER_64r == reg)) {
                    found = TRUE;
                }
            }
#endif
#ifdef BCM_HURRICANE_SUPPORT
            if(SOC_IS_HURRICANEX(unit)) {
                if( (!found) && (
                    (HOLD_COS0r == reg) ||
                    (HOLD_COS1r == reg) ||
                    (HOLD_COS2r == reg) ||
                    (HOLD_COS3r == reg) ||
                    (HOLD_COS4r == reg) ||
                    (HOLD_COS5r == reg) ||
                    (HOLD_COS6r == reg) ||
                    (HOLD_COS7r == reg) ) ) {
                    found = TRUE;
                }
            }
#endif
#ifdef BCM_KATANA2_SUPPORT
            if (SOC_IS_KATANA2(unit)) {
                if ((!found) && (SOC_BLOCK_IS(SOC_REG_INFO(unit, reg).block, SOC_BLK_RXLP))) {
                    found = TRUE;
                }
            }
#endif

            if (!found) {
                soc_cm_debug(DK_VERBOSE,
                             "soc_counter_verify: "
                             "counter %d %s is missing (i=%d, 0x%x)\n",
                             (int)reg, SOC_REG_NAME(unit, reg), i, i);
                errors = 1;
            }
        }
    }

    if (errors) {
        soc_cm_print("\nERRORS found during counter initialization.  "
                     "Set debug verbose for more info.\n\n");
    }

    soc_cm_debug(DK_VERBOSE, "soc_counter_verify: unit %d ends\n", unit);
}

/*
 * Function:
 *      _soc_counter_illegal
 * Purpose:
 *      Routine to display info about bad counter and halt.
 */

void
_soc_counter_illegal(int unit, soc_reg_t ctr_reg)
{
    soc_cm_print("soc_counter_get: unit %d: "
                 "ctr_reg %d (%s) is not a counter\n",
                 unit, ctr_reg, SOC_REG_NAME(unit, ctr_reg));
    /* assert(0); */
}

#endif /* BROADCOM_DEBUG */

/*
 * Function:
 *      soc_counter_autoz
 * Purpose:
 *      Set or clear AUTOZ mode for all MAC counter registers (all ports)
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      enable - if TRUE, enable, otherwise disable AUTOZ mode
 * Returns:
 *      SOC_E_XXX.
 * Notes:
 *      This module requires AUTOZ is off to accumulate counters properly.
 *      On 5665, this also turns on/off the MMU counters AUTOZ mode.
 */

int
soc_counter_autoz(int unit, int enable)
{
    soc_port_t          port;

    soc_cm_debug(DK_COUNTER,
                 "soc_counter_autoz: unit=%d enable=%d\n", unit, enable);

    PBMP_PORT_ITER(unit, port) {
#if defined(BCM_ESW_SUPPORT)
        SOC_IF_ERROR_RETURN(soc_autoz_set(unit, port, enable));
#endif
    }

    return (SOC_E_NONE);
}

/*
 * Function:
 *      _soc_counter_get
 * Purpose:
 *      Given a counter register number and port number, fetch the
 *      64-bit software-accumulated counter value or read from device counter 
 *      register. The software-accumulated counter value is zeroed if requested.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ctr_reg - counter register
 *      zero - if TRUE, current counter is zeroed after reading.
 *      sync_mode - if TRUE, read from device else software accumulated value
 *      val - (OUT) 64-bit counter value.
 * Returns:
 *      SOC_E_XXX.
 * Notes:
 *      Returns 0 if ctr_reg is INVALIDr or not enabled.
 */

STATIC INLINE int
_soc_counter_get(int unit, soc_port_t port, soc_reg_t ctr_reg, int ar_idx,
                 int zero, int sync_mode, uint64 *val)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 port_index, num_entries;
    char                *cname;
    uint64              *vptr;
    uint64              value;
    COUNTER_ATOMIC_DEF  s;

    SOC_IF_ERROR_RETURN(_soc_counter_get_info(unit, port, ctr_reg,
                                              &port_index, &num_entries,
                                              &cname));
    if (ar_idx >= num_entries) {
        return SOC_E_PARAM;
    }

    if (sync_mode == TRUE) {
        /* sync the software counter with hardware counter */
        if (!SOC_COUNTER_INVALID(unit, ctr_reg)) {
#if defined(BCM_XGS_SUPPORT) || defined(BCM_ARAD_SUPPORT)
            soc_counter_collect64(unit, FALSE, port, ctr_reg);
#endif
        }
    }

    if (ar_idx > 0 && ar_idx < num_entries) {
        port_index += ar_idx;
    }

    /* Try to minimize the atomic section as much as possible */
    if (ctr_reg >= NUM_SOC_REG &&
        (soc->counter_non_dma[ctr_reg - SOC_COUNTER_NON_DMA_START].flags &
         _SOC_COUNTER_NON_DMA_CURRENT)) {
        vptr = &soc->counter_hw_val[port_index];
    } else {
        vptr = &soc->counter_sw_val[port_index];
    }
    if (zero) {
        COUNTER_ATOMIC_BEGIN(s);
        value = *vptr;
        COMPILER_64_ZERO(*vptr);
        COUNTER_ATOMIC_END(s);
    } else {
        COUNTER_ATOMIC_BEGIN(s);
        value = *vptr;
        COUNTER_ATOMIC_END(s);
    }

    soc_cm_debug(DK_COUNTER + DK_VERBOSE,
                 "cntr get %s port=%d "
                 "port_index=%d vptr=%p val=0x%08x_%08x\n",
                 cname,
                 port,
                 port_index,
                 (void *)vptr, COMPILER_64_HI(value), COMPILER_64_LO(value));

    *val = value;

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_counter_get, soc_counter_get32
 *      soc_counter_get_zero, soc_counter_get32_zero
 * Purpose:
 *      Given a counter register number and port number, fetch the
 *      64-bit software-accumulated counter value (normal versions)
 *      or the 64-bit software-accumulated counter value truncated to
 *      32 bits (32-bit versions).  The software-accumulated counter
 *      value is zeroed if requested.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ctr_reg - counter register to retrieve.
 *      val - (OUT) 64/32-bit counter value.
 * Returns:
 *      SOC_E_XXX.
 * Notes:
 *      The 32-bit version returns only the lower 32 bits of the 64-bit
 *      counter value.
 */

int
soc_counter_get(int unit, soc_port_t port, soc_reg_t ctr_reg,
                int ar_idx, uint64 *val)
{
    int sync_mode = FALSE; /* read sw accumulated */ 
    return _soc_counter_get(unit, port, ctr_reg, ar_idx, 
                            FALSE, sync_mode, val);
}

int
soc_counter_get32(int unit, soc_port_t port, soc_reg_t ctr_reg,
                  int ar_idx, uint32 *val)
{
    uint64              val64;
    int                 rv;
    int                 sync_mode = FALSE; /* read sw accumulated */ 

    rv = _soc_counter_get(unit, port, ctr_reg, ar_idx, 
                          FALSE, sync_mode, &val64);

    if (rv >= 0) {
        COMPILER_64_TO_32_LO(*val, val64);
    }

    return rv;
}

int
soc_counter_get_zero(int unit, soc_port_t port,
                     soc_reg_t ctr_reg, int ar_idx, uint64 *val)
{
    int sync_mode = FALSE; /* read sw accumulated */ 
    return _soc_counter_get(unit, port, ctr_reg, ar_idx, 
                            TRUE, sync_mode, val);
}

int
soc_counter_get32_zero(int unit, soc_port_t port,
                       soc_reg_t ctr_reg, int ar_idx, uint32 *val)
{
    uint64              val64;
    int                 rv;
    int                 sync_mode = FALSE; /* read sw accumulated */ 

    rv = _soc_counter_get(unit, port, ctr_reg, ar_idx, 
                          TRUE, sync_mode, &val64);

    if (rv >= 0) {
        COMPILER_64_TO_32_LO(*val, val64);
    }

    return rv;
}

/*
 * Function:
 *      soc_counter_sync_get, soc_counter_sync_get32
 * Purpose:
 *      Given a counter register and port number, sync the
 *      counter value from the device register to the sw accumulated
 *      value.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ctr_reg - counter register to retrieve.
 *      val - (OUT) 64/32-bit counter value.
 * Returns:
 *      SOC_E_XXX.
 */

int
soc_counter_sync_get(int unit, soc_port_t port, soc_reg_t ctr_reg,
                     int ar_idx, uint64 *val)
{
    int sync_mode = TRUE; /* read register */
    return _soc_counter_get(unit, port, ctr_reg, ar_idx, 
                            FALSE, sync_mode, val);
}

int
soc_counter_sync_get32(int unit, soc_port_t port, soc_reg_t ctr_reg,
                       int ar_idx, uint32 *val)
{
    uint64              val64;
    int                 rv;
    int                 sync_mode = TRUE; /* read register */

    rv = _soc_counter_get(unit, port, ctr_reg, ar_idx, 
                          FALSE, sync_mode, &val64);

    if (rv >= 0) {
        COMPILER_64_TO_32_LO(*val, val64);
    }

    return rv;
}

/*
 * Function:
 *      soc_counter_get_rate
 * Purpose:
 *      Returns the counter incrementing rate in units of 1/sec.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ctr_reg - counter register to retrieve.
 *      rate - (OUT) 64-bit rate value.
 * Returns:
 *      SOC_E_XXX.
 */

int
soc_counter_get_rate(int unit, soc_port_t port,
                     soc_reg_t ctr_reg, int ar_idx, uint64 *rate)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 port_index, num_entries;

    SOC_IF_ERROR_RETURN(_soc_counter_get_info(unit, port, ctr_reg,
                                              &port_index, &num_entries,
                                              NULL));
    if (ar_idx >= num_entries) {
        return SOC_E_PARAM;
    }

    if (ar_idx > 0 && ar_idx < num_entries) {
        port_index += ar_idx;
    }

    if (soc->counter_interval == 0) {
        COMPILER_64_ZERO(*rate);
    } else {
#ifdef  COMPILER_HAS_DOUBLE
        /*
         * This uses floating point right now because uint64 multiply/divide
         * support is missing from many compiler libraries.
         */

        uint32          delta32, rate32;
        double          interval;

        COMPILER_64_TO_32_LO(delta32, soc->counter_delta[port_index]);

        interval = SAL_USECS_SUB(soc->counter_coll_cur,
                                 soc->counter_coll_prev) / 1000000.0;

        if (interval < 0.0001) {
            rate32 = 0;
        } else {
            rate32 = (uint32) (delta32 / interval + 0.5);
        }

        COMPILER_64_SET(*rate, 0, rate32);
#else   /* !COMPILER_HAS_DOUBLE */
        uint32          delta32, rate32;
        int             interval;

        COMPILER_64_TO_32_LO(delta32, soc->counter_delta[port_index]);

        interval = SAL_USECS_SUB(soc->counter_coll_cur, soc->counter_coll_prev);

        if (interval < 100) {
            rate32 = 0;
        } else {
            rate32 = _shr_div_exp10(delta32, interval, 6);
        }

        COMPILER_64_SET(*rate, 0, rate32);
#endif  /* !COMPILER_HAS_DOUBLE */
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_counter_get32_rate
 * Purpose:
 *      Returns the counter incrementing rate in units of 1/sec.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ctr_reg - counter register to retrieve.
 *      rate - (OUT) 32-bit delta value.
 * Returns:
 *      SOC_E_XXX.
 */

int
soc_counter_get32_rate(int unit, soc_port_t port,
                       soc_reg_t ctr_reg, int ar_idx, uint32 *rate)
{
    uint64              rate64;

    SOC_IF_ERROR_RETURN(soc_counter_get_rate(unit, port, ctr_reg,
                                             ar_idx, &rate64));

    COMPILER_64_TO_32_LO(*rate, rate64);

    return SOC_E_NONE;
}

#if defined(BCM_PETRAB_SUPPORT)
STATIC int
soc_petra_port_counter_get(int unit, soc_port_t port, int ctr_type,
                               uint64 *value);
#endif
/*
 * Function:
 *      soc_counter_set, soc_counter_set32
 * Purpose:
 *      Given a counter register number, port number, and a counter
 *      value, set both the hardware and software-accumulated counters
 *      to the value.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ctr_reg - counter register to retrieve.
 *      val - 64/32-bit counter value.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The 32-bit version sets the upper 32-bits of the 64-bit
 *      software counter to zero.  The specified value is truncated
 *      to the width of the hardware register when storing to hardware.
 *      Use the value 0 to clear counters.
 */

STATIC int
_soc_counter_set(int unit, soc_port_t port, soc_reg_t ctr_reg,
                 int ar_idx, uint64 val64)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 port_index, num_entries;
    int                 index_min, index_max, index;
    soc_counter_non_dma_t *non_dma;
    uint32              entry[SOC_MAX_MEM_FIELD_WORDS], fval[2];
    int                 dma_base_index;
    soc_mem_t           dma_mem;
    COUNTER_ATOMIC_DEF  s;
    int         rv = SOC_E_NONE;
    int                 prime_ctr_write, i;
    uint32              val32;
#ifdef BCM_KATANA_SUPPORT
    uint32              pkt_count = 0;
#endif /* BCM_KATANA_SUPPORT */
    SOC_IF_ERROR_RETURN(_soc_counter_get_info(unit, port, ctr_reg,
                                              &port_index, &num_entries,
                                              NULL));
    if (ar_idx >= num_entries) {
        return SOC_E_PARAM;
    }

    if (ar_idx < 0) {
        index_min = 0;
        index_max = num_entries - 1;
    } else {
        index_min = index_max = ar_idx;
    }

    COUNTER_LOCK(unit);
    for (index = index_min; index <= index_max; index++) {

#if defined(BCM_ARAD_SUPPORT) || defined(BCM_PETRAB_SUPPORT)
    if (!SOC_IS_PETRAB(unit) && !_SOC_CONTROLLED_COUNTER_USE(unit, port)) {
#endif
        if (ctr_reg >= NUM_SOC_REG) {
            non_dma = &soc->counter_non_dma[ctr_reg - SOC_COUNTER_NON_DMA_START];
            if (non_dma->flags & (_SOC_COUNTER_NON_DMA_PEAK |
                                  _SOC_COUNTER_NON_DMA_CURRENT)) {
                continue;
            }
            if (non_dma->mem != INVALIDm) {
                dma_mem = non_dma->dma_mem[0];
                dma_base_index = port_index - non_dma->base_index +
                    non_dma->dma_index_min[0];
                for (i = 1; i < 4; i++) {
                    if (non_dma->dma_buf[i] != NULL &&
                        dma_base_index > non_dma->dma_index_max[i - 1]) {
                        dma_mem = non_dma->dma_mem[i];
                        dma_base_index -= non_dma->dma_index_max[i - 1] + 1;
                        dma_base_index += non_dma->dma_index_min[i];
                    }
                }    
                rv = soc_mem_read(unit, dma_mem, MEM_BLOCK_ANY,
                        dma_base_index + index, entry);
                if (SOC_FAILURE(rv)) {
                    goto counter_set_done;
                }
#ifdef BCM_KATANA_SUPPORT
                if ((SOC_IS_KATANA(unit)) && 
                    (!soc_feature(unit, soc_feature_counter_toggled_read))) {
                    soc_mem_field_get(unit, dma_mem, entry, COUNTf, &pkt_count);
                    if (!SAL_BOOT_BCMSIM) {
                        pkt_count -= 1;
                    }
                    soc_mem_field_set(unit, dma_mem, entry,
                                      COUNTf, &pkt_count);
                
                }
#endif
                fval[0] = COMPILER_64_LO(val64);
                fval[1] = COMPILER_64_HI(val64);
                soc_mem_field_set(unit, dma_mem, entry,
                        non_dma->field, fval);
                rv = soc_mem_write(unit, dma_mem, MEM_BLOCK_ALL,
                        dma_base_index + index, entry);
                if (SOC_FAILURE(rv)) {
                    goto counter_set_done;
                }
            } else if (non_dma->reg != INVALIDr) {
                if (port < 0) {
                    rv = SOC_E_PARAM;
                    goto counter_set_done;
                }
                if (PBMP_MEMBER(non_dma->pbmp, port)) {
                    uint64 regval64;
                    uint32 regval32;

                    if (SOC_IS_TR_VL(unit) || SOC_IS_SC_CQ(unit)) {
                        if (non_dma->reg == TXLLFCMSGCNTr) {
                            if (!IS_HG_PORT( unit, port)) {
                                continue;
                            }
                        } 
                    } 

                    rv = soc_reg_get(unit, non_dma->reg, 
                                     port, index, &regval64);
                    if (SOC_FAILURE(rv)) {
                        goto counter_set_done;
                    }

                    if (SOC_REG_IS_64(unit, non_dma->reg)) {
                        soc_reg64_field_set(unit, non_dma->reg, &regval64,
                                non_dma->field, val64);
                    } else {
                        regval32 = COMPILER_64_LO(regval64);
                        soc_reg_field_set(unit, non_dma->reg, &regval32,
                                non_dma->field, COMPILER_64_LO(val64));
                        COMPILER_64_SET(regval64, 0, regval32);
                    }

                    rv = soc_reg_set(unit, non_dma->reg, 
                                     port, index, regval64);
                    if (SOC_FAILURE(rv)) {
                        goto counter_set_done;
                    }
                }
            }
        } else {
            if (port < 0) {
                rv = SOC_E_PARAM;
                goto counter_set_done;
            }

            if (!(SOC_REG_INFO(unit, ctr_reg).flags & SOC_REG_FLAG_64_BITS) &&
                !SOC_IS_XGS3_SWITCH(unit)) {
                val32 = COMPILER_64_LO(val64);
                rv = soc_reg32_set(unit, ctr_reg, port, index, val32);
                if (SOC_FAILURE(rv)) {
                    goto counter_set_done;
                }
            } else {
                if (soc_feature(unit, soc_feature_prime_ctr_writes) &&
                    /*SOC_REG_INFO(unit, ctr_reg).block == SOC_BLK_EGR) {*/
                    SOC_BLOCK_IS(SOC_REG_INFO(unit, ctr_reg).block, SOC_BLK_EGR)) {
                    prime_ctr_write = TRUE;
                } else {
                    prime_ctr_write = FALSE;
                }

                if (prime_ctr_write) {
                    rv = soc_reg32_get(unit, ctr_reg, port, index, &val32);
                    if (SOC_FAILURE(rv)) {
                        goto counter_set_done;
                    }
                }
                rv = soc_reg_set(unit, ctr_reg, port, index, val64);
                if (SOC_FAILURE(rv)) {
                    goto counter_set_done;
                }
                if (prime_ctr_write) {
                    rv = soc_reg32_get(unit, ctr_reg, port, index, &val32);
                    if (SOC_FAILURE(rv)) {
                        goto counter_set_done;
                    }
                }
            }
        }
#if defined(BCM_ARAD_SUPPORT) || defined(BCM_PETRAB_SUPPORT)
    }
#endif
#if defined(BCM_PETRAB_SUPPORT)
    else if SOC_IS_PETRAB(unit) {
       uint64              ctr_new;

       /* 
        * Soc_petra-B counters are Clear-on-Read. Add ctr_new to 
        * previously stored sw value if discard is not set
        */
       SOC_IF_ERROR_RETURN(soc_petra_port_counter_get(unit, port, ctr_reg, &ctr_new));
   }
#endif


        /* The following section updates 64-bit values and must be atomic */
        COUNTER_ATOMIC_BEGIN(s);
        soc->counter_sw_val[port_index + index] = val64;
        soc->counter_hw_val[port_index + index] = val64;
        COMPILER_64_SET(soc->counter_delta[port_index + index], 0, 0);
        COUNTER_ATOMIC_END(s);
    }

 counter_set_done:
    COUNTER_UNLOCK(unit);
    return rv;
}

int
soc_counter_set(int unit, soc_port_t port, soc_reg_t ctr_reg,
                int ar_idx, uint64 val)
{
    return _soc_counter_set(unit, port, ctr_reg, ar_idx, val);
}

int
soc_counter_set32(int unit, soc_port_t port, soc_reg_t ctr_reg,
                  int ar_idx, uint32 val)
{
    uint64              val64;

    COMPILER_64_SET(val64, 0, val);

    return _soc_counter_set(unit, port, ctr_reg, ar_idx, val64);
}

/*
 * Function:
 *      soc_counter_set_by_port, soc_counter_set32_by_port
 * Purpose:
 *      Given a port bit map, set all counters for the specified ports to
 *      the requested value.  Useful mainly to clear all the counters.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      pbm  - Bit map of all ports to set counters on.
 *      val  - 64/32-bit counter value.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The 32-bit version sets the upper 32-bits of the 64-bit
 *      software counters to zero.  The specified value is truncated
 *      to the size of the hardware register when storing to hardware.
 */

int
soc_counter_set_by_port(int unit, pbmp_t pbmp, uint64 val)
{
    int                 i, rv;
    soc_port_t          port;
    soc_ctr_ref_t       *ctr_ref;

    PBMP_ITER(pbmp, port) {
        if (!IS_LB_PORT(unit, port)
#if defined(BCM_SHADOW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
            && !IS_IL_PORT(unit, port)
#endif
        ) 
        {
            /* No DMA counters for the loopback port */
            for (i = 0; i < PORT_CTR_NUM(unit, port); i++) {
                ctr_ref = PORT_CTR_REG(unit, port, i);
                if (!SOC_COUNTER_INVALID(unit, ctr_ref->reg)) {
                    if (!SOC_REG_PORT_VALID(unit,ctr_ref->reg, port)) {
                        continue;
                    }
                    rv = _soc_counter_set(unit, port, ctr_ref->reg,
                                          ctr_ref->index, val);
                    if (rv == SOC_E_FAIL) {
                        soc_cm_debug(DK_ERR, "Error %d calling soc_counter_set for"
                                     "port: %d, reg: %d, index: %d\n", rv,
                                     port, ctr_ref->reg, ctr_ref->index);
                    }
                }
            }
        }
        for (i = 0; i < SOC_COUNTER_NON_DMA_END - SOC_COUNTER_NON_DMA_START;
             i++) {
             rv = _soc_counter_set(unit, port, SOC_COUNTER_NON_DMA_START + i, -1,
                                 val);
            if (rv == SOC_E_FAIL) {
                soc_cm_debug(DK_ERR, "Error %d calling soc_counter_set for"
                             "port: %d, non-DMA index: %d\n", rv, port, i);
            }
        }
#ifdef BCM_BRADLEY_SUPPORT
        /*
         * The HOLD register is not supported by DMA on HUMV/Bradley, 
         * so we cannot use the standard counter_set function.
         */
        if (SOC_IS_HBX(unit)) {
            if (SOC_REG_IS_VALID(unit, HOLD_Xr) && 
                SOC_REG_IS_VALID(unit, HOLD_Yr)) {
            SOC_IF_ERROR_RETURN
                (WRITE_HOLD_Xr(unit, port, COMPILER_64_LO(val) / 2));
            SOC_IF_ERROR_RETURN
                (WRITE_HOLD_Yr(unit, port, COMPILER_64_LO(val) / 2));
            }
        }
#endif
    }

    return SOC_E_NONE;
}

int
soc_counter_set32_by_port(int unit, pbmp_t pbmp, uint32 val)
{
    uint64              val64;

    COMPILER_64_SET(val64, 0, val);

    return soc_counter_set_by_port(unit, pbmp, val64);
}

int
soc_controlled_counter_clear(int unit, soc_port_t port)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    uint64              ctr_new;
    int                 index, port_base;
    soc_controlled_counter_t* ctr;

    soc_cm_debug(DK_COUNTER + DK_VERBOSE, "soc_controlled_counter_clear: unit=%d "
                 "port=%d\n", unit, port);

    if (!soc_feature(unit, soc_feature_controlled_counters)) {
        return SOC_E_NONE;
    }

    port_base = COUNTER_IDX_PORTBASE(unit, port);

    COUNTER_LOCK(unit);

#ifdef BCM_ARAD_SUPPORT
    if (SOC_IS_ARAD(unit)) {
        SOC_IF_ERROR_RETURN(soc_arad_stat_clear_on_read_set(unit , 1));
    }
#endif

    for (index = 0; soc->controlled_counters[index].controlled_counter_f != NULL; index++) {
        COUNTER_ATOMIC_DEF s;

        /* assume clear on read */
        ctr = &soc->controlled_counters[index];
        ctr->controlled_counter_f(unit, ctr->counter_id, port, &ctr_new);


        /*check if counter is relevant*/
        if(!COUNTER_IS_COLLECTED(soc->controlled_counters[index])) {
            continue;
        }

        /* The following section updates 64-bit values and must be atomic */
        COUNTER_ATOMIC_BEGIN(s);
        COMPILER_64_SET(soc->counter_sw_val[port_base + ctr->counter_idx], 0, 0);
        COMPILER_64_SET(soc->counter_hw_val[port_base + ctr->counter_idx], 0, 0);
        COMPILER_64_SET(soc->counter_delta[port_base + ctr->counter_idx], 0, 0);
        COUNTER_ATOMIC_END(s);

    }

#ifdef BCM_ARAD_SUPPORT
        if (SOC_IS_ARAD(unit)) {
            SOC_IF_ERROR_RETURN(soc_arad_stat_clear_on_read_set(unit , 0));
        }
#endif
    
    COUNTER_UNLOCK(unit);
    
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_counter_set_by_reg, soc_counter_set32_by_reg
 * Purpose:
 *      Given a counter register number and a counter value, set both
 *      the hardware and software-accumulated counters to the value
 *      for all ports.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      ctr_reg - counter register to set.
 *      val - 64/32-bit counter value.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The 32-bit version sets the upper 32-bits of the 64-bit
 *      software counters to zero.  The specified value is truncated
 *      to the size of the hardware register when storing to hardware.
 */

int
soc_counter_set_by_reg(int unit, soc_reg_t ctr_reg, int ar_idx, uint64 val)
{
    soc_port_t          port;

    if (SOC_COUNTER_INVALID(unit, ctr_reg)) {
        return SOC_E_NONE;
    }

#ifdef BROADCOM_DEBUG
    if (!SOC_REG_IS_COUNTER(unit, ctr_reg)) {
        _soc_counter_illegal(unit, ctr_reg);
        return SOC_E_NONE;
    }
#endif /* BROADCOM_DEBUG */


    PBMP_PORT_ITER(unit, port) {
        SOC_IF_ERROR_RETURN(_soc_counter_set(unit, port, ctr_reg,
                                             ar_idx, val));
    }

    return SOC_E_NONE;
}

int
soc_counter_set32_by_reg(int unit, soc_reg_t ctr_reg, int ar_idx, uint32 val)
{
    uint64              val64;

    COMPILER_64_SET(val64, 0, val);

    return soc_counter_set_by_reg(unit, ctr_reg, ar_idx, val64);
}

/*
 * Function:
 *      soc_counter_collect32
 * Purpose:
 *      This routine gets called each time the counter transfer has
 *      completed a cycle.  It collects the 32-bit counters.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      discard - If TRUE, the software counters are not updated; this
 *              results in only synchronizing the previous hardware
 *              count buffer.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Computes the deltas in the hardware counters from the last
 *      time it was called and adds them to the high resolution (64-bit)
 *      software counters in counter_sw_val[].  It takes wrapping into
 *      account for counters that are less than 32 bits wide.
 *      It also computes counter_delta[].
 */

STATIC int
soc_counter_collect32(int unit, int discard)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    soc_port_t          port;
    soc_reg_t           ctr_reg;
    soc_ctr_ref_t       *ctr_ref;
    uint32              ctr_new, ctr_prev, ctr_diff;
    int                 index;
    int                 port_index;
    int                 dma;
    int                 recheck_cntrs;
    int                 ar_idx;

    soc_cm_debug(DK_COUNTER + DK_VERBOSE,
                 "soc_counter_collect32: unit=%d discard=%d\n",
                 unit, discard);

    dma = ((soc->counter_flags & SOC_COUNTER_F_DMA) &&
           !discard);

    recheck_cntrs = soc_feature(unit, soc_feature_recheck_cntrs);

    PBMP_ITER(soc->counter_pbmp, port) {
        if ((IS_HG_PORT(unit, port) || IS_XE_PORT(unit, port)) &&
            (!SOC_IS_XGS3_SWITCH(unit))) {
                continue;
        }
#if defined(BCM_SHADOW_SUPPORT)
        if (SOC_IS_SHADOW(unit) && IS_IL_PORT(unit, port)) {
                continue;
        }
#endif /* BCM_SHADOW_SUPPORT */


        port_index = COUNTER_MIN_IDX_GET(unit, port);

        for (index = 0; index < PORT_CTR_NUM(unit, port);
             index++, port_index++) {
            volatile uint64 *vptr;
            COUNTER_ATOMIC_DEF s;

            ctr_ref = PORT_CTR_REG(unit, port, index);
            ctr_reg = ctr_ref->reg;
            ar_idx = ctr_ref->index;

            if (SOC_COUNTER_INVALID(unit, ctr_reg)) {
                continue;
            }
            if (!SOC_REG_PORT_VALID(unit,ctr_reg, port)) {
                continue;
            }

            ctr_prev = COMPILER_64_LO(soc->counter_hw_val[port_index]);

            if (dma) {
                ctr_new = soc->counter_buf32[port_index];
            } else {                   /* Not DMA.  Just read register */
                SOC_IF_ERROR_RETURN
                    (soc_reg32_get(unit, ctr_reg, port, ar_idx,
                                   &ctr_new));
            }

            if ( (recheck_cntrs == TRUE) && (ctr_new != ctr_prev) ) {
                /* Seeds of doubt, double-check */
                uint32 ctr_new2;
                int suspicious = 0;
                SOC_IF_ERROR_RETURN
                  (soc_reg32_get(unit, ctr_reg, port, 0, &ctr_new2));

                /* Check for partial ordering, with wrap */
                if (ctr_new < ctr_prev) {
                    if ((ctr_new2 < ctr_new) || (ctr_new2 > ctr_prev)) {
                        /* prev < new2 < new, bad data */
                        /* Try again next time */
                        ctr_new = ctr_prev;
                        suspicious = 1;
                    }
                } else {
                    if ((ctr_new2 < ctr_new) && (ctr_new2 > ctr_prev)) {
                        /* prev < new2 < new, bad data */
                        /* Try again next time */
                        ctr_new = ctr_prev;
                        suspicious = 1;
                    }
                }
                /* Otherwise believe ctr_new */

                if (suspicious) {
#if !defined(SOC_NO_NAMES)
                    soc_cm_debug(DK_COUNTER, "soc_counter_collect32: "
                                 "unit %d, port%d: suspicious %s "
                                 "counter read (%s)\n",
                                 unit, port, 
                                 dma?"DMA":"manual",
                                 SOC_REG_NAME(unit, ctr_reg));
#else
                    soc_cm_debug(DK_COUNTER, "soc_counter_collect32: "
                                 "unit %d, port%d: suspicious %s "
                                 "counter read (Counter%d)\n",
                                 unit, port, 
                                 dma?"DMA":"manual",
                                 ctr_reg);
#endif /* SOC_NO_NAMES */
                } /* Suspicious counter change */
            } /* recheck_cntrs == TRUE */

            if (ctr_new == ctr_prev) {
                COUNTER_ATOMIC_BEGIN(s);
                COMPILER_64_SET(soc->counter_delta[port_index], 0, 0);
                COUNTER_ATOMIC_END(s);
                continue;
            }

            if (discard) {
                COUNTER_ATOMIC_BEGIN(s);
                /* Update the DMA location */
                soc->counter_buf32[port_index] = ctr_new;
                /* Update the previous value buffer */
                COMPILER_64_SET(soc->counter_hw_val[port_index], 0, ctr_new);
                COMPILER_64_SET(soc->counter_delta[port_index], 0, 0);
                COUNTER_ATOMIC_END(s);
                continue;
            }

            soc_cm_debug(DK_COUNTER,
                         "soc_counter_collect32: ctr %d => %u\n",
                         port_index, ctr_new);

            vptr = &soc->counter_sw_val[port_index];

            ctr_diff = ctr_new;

            if (ctr_diff < ctr_prev) {
                int             width, i = 0;

                /*
                 * Counter must have wrapped around.  Add the proper
                 * wrap-around amount.  Full 32-bit wide counters do not
                 * need anything added.
                 */
                width = SOC_REG_INFO(unit, ctr_reg).fields[0].len;
                if (soc_feature(unit, soc_feature_counter_parity)
#ifdef BCM_HURRICANE2_SUPPORT
                    || soc_reg_field_valid(unit, ctr_reg, PARITYf)
#endif
                   )
                {
                    while((SOC_REG_INFO(unit, ctr_reg).fields + i) != NULL) { 
                        if (SOC_REG_INFO(unit, ctr_reg).fields[i].field == 
                            COUNTf) {
                            width = SOC_REG_INFO(unit, ctr_reg).fields[i].len;
                            break;
                        }
                        i++;
                    }
                }
                if (width < 32) {
                    ctr_diff += (1UL << width);
                }
            }

            ctr_diff -= ctr_prev;

            COUNTER_ATOMIC_BEGIN(s);
            COMPILER_64_ADD_32(*vptr, ctr_diff);
            COMPILER_64_SET(soc->counter_delta[port_index], 0, ctr_diff);
            COMPILER_64_SET(soc->counter_hw_val[port_index], 0, ctr_new);
            COUNTER_ATOMIC_END(s);

        }

        /* If signaled to exit then return  */
        if (!soc->counter_interval) {
            return SOC_E_NONE;
        }
        
        /*
         * Allow other tasks to run between processing each port.
         */

        sal_thread_yield();
    }

    return SOC_E_NONE;
}

#ifdef BCM_SBUSDMA_SUPPORT
#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
STATIC sbusdma_desc_handle_t
       _soc_port_counter_handles[SOC_MAX_NUM_DEVICES][SOC_MAX_NUM_PP_PORTS][3];
#else
STATIC sbusdma_desc_handle_t
          _soc_port_counter_handles[SOC_MAX_NUM_DEVICES][SOC_MAX_NUM_PORTS][3];
#endif
#endif

#ifdef BCM_PETRAB_SUPPORT
soc_ctr_ref_t soc_dpp_petra_nif_ctrs[SOC_DPP_PETRA_NUM_MAC_COUNTERS] = {
    {SOC_PB_NIF_RX_BCAST_PACKETS,       0},
    {SOC_PB_NIF_RX_MCAST_BURSTS,        0},
    {SOC_PB_NIF_RX_ERR_PACKETS,         0},
    {SOC_PB_NIF_RX_LEN_BELOW_MIN,       0},
    {SOC_PB_NIF_RX_LEN_MIN_59,          0},
    {SOC_PB_NIF_RX_LEN_60,              0},
    {SOC_PB_NIF_RX_LEN_61_123,          0},
    {SOC_PB_NIF_RX_LEN_124_251,         0},
    {SOC_PB_NIF_RX_LEN_252_507,         0},
    {SOC_PB_NIF_RX_LEN_508_1019,        0},
    {SOC_PB_NIF_RX_LEN_1020_1514CFG,    0},
    {SOC_PB_NIF_RX_LEN_1515CFG_MAX,     0},
    {SOC_PB_NIF_RX_LEN_ABOVE_MAX,       0},
    {SOC_PB_NIF_RX_OK_PAUSE_FRAMES,     0},
    {SOC_PB_NIF_RX_ERR_PAUSE_FRAMES,    0},
    {SOC_PB_NIF_RX_PTP_FRAMES,          0},
    {SOC_PB_NIF_RX_FRAME_ERR_PACKETS,   0},
    {SOC_PB_NIF_RX_BCT_ERR_PACKETS,     0},
    {SOC_PB_NIF_TX_BCAST_PACKETS,       0},
    {SOC_PB_NIF_TX_MCAST_BURSTS,        0},
    {SOC_PB_NIF_TX_ERR_PACKETS,         0},
    {SOC_PB_NIF_TX_PAUSE_FRAMES,        0},
    {SOC_PB_NIF_TX_PTP_FRAMES,          0},
    {SOC_PB_NIF_TX_NO_LINK_PACKETS,     0},
    {SOC_PB_NIF_RX_OK_OCTETS,           0},
    {SOC_PB_NIF_TX_OK_OCTETS,           0},
    {SOC_PB_NIF_RX_OK_PACKETS,          0},
    {SOC_PB_NIF_TX_OK_PACKETS,          0}
};

char *soc_dpp_petra_nif_ctr_names[SOC_DPP_PETRA_NUM_MAC_COUNTERS] = {
    "RBCA_PKTS",
    "RMCA_BURSTS",
    "RERR_PKTS",
    "RLEN_BELOW_MIN",
    "RLEN_MIN_59",
    "RLEN_60",
    "RLEN_61_123",
    "RLEN_124_251",
    "RLEN_252_507",
    "RLEN_508_1019",
    "RLEN_1020_1514CFG",
    "RLEN_1515CFG_MAX",
    "RLEN_ABOVE_MAX",
    "ROK_PF",
    "RERR_PF",
    "RPTP_FR",
    "RFR_ERR_PKTS",
    "RBCT_ERR_PKTS",
    "TBCA_PKTS",
    "TMCA_BURSTS",
    "TERR_PKTS",
    "TPF",
    "TPTP_FR",
    "TNO_LINK_PKTS",
    "ROK_OCTETS",
    "TOK_OCTETS",
    "ROK_PKTS",
    "TOK_PKTS"
};

soc_cmap_t soc_dpp_petra_nif_ctr_map = {
    soc_dpp_petra_nif_ctrs, SOC_DPP_PETRA_NUM_MAC_COUNTERS
};
#endif /* BCM_PETRAB_SUPPORT*/

#if defined(BCM_PETRAB_SUPPORT)
/* Function:    soc_petra_port_counter_get
 * Purpose:     Retrieve the nif counter associated with this port.
 */
STATIC int
soc_petra_port_counter_get(int unit, soc_port_t port, int ctr_type,
                               uint64 *value)
{
    int                         soc_sand_rv, soc_sand_dev_id;
    SOC_TMC_PORT2IF_MAPPING_INFO    ifp_mapping, out_mapping;
    SOC_SAND_64CNT                  soc_sand_value;

    if ((value == NULL) || (ctr_type >= SOC_DPP_PETRA_NUM_MAC_COUNTERS)) {
        return SOC_E_PARAM;
    }

    soc_sand_dev_id = BCM_DPP_UNIT_TO_SAND_DEVICE_ID(unit);
    soc_sand_rv = MBCM_DPP_DRIVER_CALL_WITHOUT_DEV_ID(unit, mbcm_dpp_port_to_interface_map_get,
        (soc_sand_dev_id, port, &ifp_mapping, &out_mapping));
    if (SOC_SAND_FAILURE(soc_sand_rv)) {
        soc_cm_debug(DK_ERR, "soc_petra_port_counter_get: failed (SOC_SAND_RV:"
                     "%x) to map port(%d) to NIF_ID. \n", soc_sand_rv, port);
        return SOC_E_INTERNAL; 
    }

    sal_memset(&soc_sand_value, 0, sizeof(soc_sand_value));

    soc_sand_rv = soc_pb_nif_counter_get(soc_sand_dev_id, ifp_mapping.if_id, ctr_type, 
                                 &soc_sand_value);

    if (SOC_SAND_FAILURE(soc_sand_rv)) {
        soc_cm_debug(DK_ERR, "soc_petra_port_counter_get: failed (SOC_SAND_RV:"
                     "%d) to get NIF counter (type:%d) if_id:%d port:0x%x\n",
                     soc_sand_rv, ctr_type, ifp_mapping.if_id, port);
        return SOC_E_INTERNAL; 
    }
    COMPILER_64_SET(*value, soc_sand_value.u64.arr[1], soc_sand_value.u64.arr[0]);

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_petra_counter_collect64
 * Purpose:
 *      This routine gets called each time the counter transfer has
 *      completed a cycle.  It collects the 64-bit counters.
 */
STATIC INLINE int
soc_petra_counter_collect64(int unit, int discard)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    soc_port_t          port;
    uint64              ctr_new;
    int                 index, port_base;
    soc_cmap_t          *cmap = NULL;

    soc_cm_debug(DK_COUNTER + DK_VERBOSE, "soc_petra_counter_collect64: unit=%d "
                 "discard=%d\n", unit, discard);

    if (!SOC_IS_PETRAB(unit)) {
        
        soc_cm_debug(DK_ERR, "soc_petra_counter_collect64: not available. \n");
        return SOC_E_UNAVAIL;
    }

    PBMP_ITER(soc->counter_pbmp, port) {
        
        cmap = &soc_dpp_petra_nif_ctr_map;

        for (index = 0; index < cmap->cmap_size; index++) {
            volatile uint64 *vptr;
            COUNTER_ATOMIC_DEF s;

            port_base = COUNTER_IDX_PORTBASE(unit, port);

            COMPILER_64_SET(ctr_new, 0, 0);
            SOC_IF_ERROR_RETURN(soc_petra_port_counter_get(unit, port, 
                                                             index, &ctr_new));
            /* Soc_petra-B & ARAD NIF counters are Clear-on-Read. Add ctr_new to 
             * previously stored sw value if discard is not set
             */
            if (COMPILER_64_IS_ZERO(ctr_new)) {
                continue;
            }
            if (discard) {
                /* Update the previous value buffer */
                COUNTER_ATOMIC_BEGIN(s);
                soc->counter_hw_val[port_base + index] = ctr_new;
                COMPILER_64_ZERO(soc->counter_delta[port_base + index]);
                COUNTER_ATOMIC_END(s);
                continue;
            }
            soc_cm_debug(DK_COUNTER, "soc_counter_collect64: ctr %d => "
                         "0x%08x_%08x\n", port_base + index,
                         COMPILER_64_HI(ctr_new), COMPILER_64_LO(ctr_new));

            vptr = &soc->counter_sw_val[port_base + index];
            COUNTER_ATOMIC_BEGIN(s);
            COMPILER_64_ADD_64(*vptr, ctr_new);
            soc->counter_delta[port_base + index] = ctr_new;
            soc->counter_hw_val[port_base + index] = ctr_new;
            COUNTER_ATOMIC_END(s);
        }

        /* If signaled to exit then return  */
        if (!soc->counter_interval) {
            return SOC_E_NONE;
        }

        /* Allow other tasks to run between processing each port */
        sal_thread_yield();
    }

    return SOC_E_NONE;
}
#endif /* defined(BCM_PETRAB_SUPPORT) */

/*
 * Function:
 *      soc_controlled_counters_collect64
 * Purpose:
 *      This routine gets called each time the counter transfer has
 *      completed a cycle.  It collects the 64-bit controlled counters.
 */
STATIC INLINE int
soc_controlled_counters_collect64(int unit, int discard)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    soc_port_t          port;
    uint64              ctr_new;
    int                 index, port_base;
    soc_controlled_counter_t* ctr;

    soc_cm_debug(DK_COUNTER + DK_VERBOSE, "soc_controlled_counters_collect64: unit=%d "
                 "discard=%d\n", unit, discard);

     if (!soc_feature(unit, soc_feature_controlled_counters)) {
        return SOC_E_NONE;
    }

    PBMP_ITER(soc->counter_pbmp, port) {
        for (index = 0; soc->controlled_counters[index].controlled_counter_f != NULL; index++) {
            volatile uint64 *vptr;
            COUNTER_ATOMIC_DEF s;

            ctr = &soc->controlled_counters[index];

            if(!COUNTER_IS_COLLECTED(soc->controlled_counters[index])) {
                continue;
            }

            ctr->controlled_counter_f(unit, ctr->counter_id, port, &ctr_new);
    
            /* Counters are Clear-on-Read. Add ctr_new to 
             * previously stored sw value if discard is not set
             */
            if (COMPILER_64_IS_ZERO(ctr_new)) {
                continue;
            }
    
            port_base = COUNTER_IDX_PORTBASE(unit, port);

            if (discard) {
                /* Update the previous value buffer */
                COUNTER_ATOMIC_BEGIN(s);
                soc->counter_hw_val[port_base + ctr->counter_idx] = ctr_new;
                COMPILER_64_ZERO(soc->counter_delta[port_base + ctr->counter_idx]);
                COUNTER_ATOMIC_END(s);
                continue;
            }
            soc_cm_debug(DK_COUNTER, "soc_controlled_counters_collect64: ctr %d => "
                         "0x%08x_%08x\n", port_base + ctr->counter_idx,
                         COMPILER_64_HI(ctr_new), COMPILER_64_LO(ctr_new));
    
            vptr = &soc->counter_sw_val[port_base + ctr->counter_idx];
            COUNTER_ATOMIC_BEGIN(s);
            COMPILER_64_ADD_64(*vptr, ctr_new);
            soc->counter_delta[port_base + ctr->counter_idx] = ctr_new;
            soc->counter_hw_val[port_base + ctr->counter_idx] = ctr_new;
            COUNTER_ATOMIC_END(s);
        }

        /* If signaled to exit then return  */
        if (!soc->counter_interval) {
            return SOC_E_NONE;
        }
    
        /* Allow other tasks to run between processing each port */
        sal_thread_yield();
    }

    return SOC_E_NONE;
}

#ifdef BCM_XGS_SUPPORT

#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
/*
 * Function:
 *      _soc_xgs3_update_link_activity
 * Purpose:
 *      Report the link activities to LED processor. 
 * Parameters:
 *      unit     - switch unit
 *      port     - port number
 *      tx_byte  - transmitted byte during counter DMA interval
 *      rx_byte  - received byte during counter DMA interval
 *
 * Returns:
 *      SOC_E_*
 */
static int
_soc_xgs3_update_link_activity(int unit, soc_port_t port, 
                               int tx_byte, int rx_byte)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                  port_speed, interval, act_byte, byte;
    uint32               portdata;

    SOC_IF_ERROR_RETURN(soc_mac_speed_get(unit, port, &port_speed));
    portdata = 0;
    interval = soc->counter_interval;

    if (tx_byte < rx_byte) {
        act_byte = rx_byte;
    } else {
        act_byte = tx_byte;
    }

    if (act_byte > (((port_speed / 8) * interval) / 2)) {
        /* transmitting or receiving more than 50% */
        portdata |= 0x2;
    } else if (act_byte > (((port_speed / 8) * interval) / 4)) {
        /* transmitting or receiving more than 25% */
        portdata |= 0x4;
    } else if (act_byte > 0) { 
        /* some transmitting or receiving activity on the link */
        portdata |= 0x8;
    }

    if (tx_byte) {
        /* Indicate TX activity */
        portdata |= 0x10;
    }
    if (rx_byte) {
        /* Indicate RX activity */
        portdata |= 0x20;
    }

#ifdef BCM_TRIDENT_SUPPORT
#define TRIDENT_LEDUP0_PORT_MAX        36

    if (SOC_IS_TD_TT(unit)) {
        soc_info_t *si;
        int phy_port;  /* physical port number */
        uint32 dram_base;
        uint32 prev_data;

        si = &SOC_INFO(unit);
        phy_port = si->port_l2p_mapping[port];

        /* trident first 36 ports in ledproc0, the other 36 ports in ledproc1*/
        if (phy_port > TRIDENT_LEDUP0_PORT_MAX) {
            phy_port -= TRIDENT_LEDUP0_PORT_MAX;
            dram_base = CMICE_LEDUP1_DATA_RAM_BASE;
        } else {
            dram_base = CMICE_LEDUP0_DATA_RAM_BASE;
        }

        byte = LS_LED_DATA_OFFSET + phy_port;

        prev_data = soc_pci_read(unit, dram_base + CMIC_LED_REG_SIZE * byte);
        prev_data &= ~0x3e;
        portdata |= prev_data;
        soc_pci_write(unit, dram_base + CMIC_LED_REG_SIZE * byte, portdata);

    } else
#endif
    {
        /* 20 bytes from LS_LED_DATA_OFFSET is used by linkscan callback */
        byte = LS_LED_DATA_OFFSET + 20 + port;
        soc_pci_write(unit, CMIC_LED_DATA_RAM(byte), portdata);
    }
    return SOC_E_NONE;
}
#endif
#endif /* BCM_XGS_SUPPORT */

#if defined(BCM_XGS_SUPPORT) || defined(BCM_ARAD_SUPPORT)
/*
 * Function:
 *      soc_counter_collect64
 * Purpose:
 *      This routine gets called each time the counter transfer has
 *      completed a cycle.  It collects the 64-bit counters.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      discard - If true, the software counters are not updated; this
 *              results in only synchronizing the previous hardware
 *              count buffer.
 *      tmp_port - if -1 sw accumulation of all the counters done.
 *             port valid when hw_ctr_reg is valid
 *      hw_ctr_reg - if -1 sw accumulation of all the counters done.
 *                   else hw register value of the tmp_port synced 
 *                   to sw counter.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      It computes the deltas in the hardware counters from the last
 *      time it was called and adds them to the high resolution (64-bit)
 *      software counters in counter_sw_val[].  It takes wrapping into
 *      account for counters that are less than 64 bits wide.
 *      It also computes counter_delta[].
 */

STATIC int
soc_counter_collect64(int unit, int discard, soc_port_t tmp_port, soc_reg_t hw_ctr_reg)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    soc_port_t          port;
    soc_reg_t           ctr_reg;
    uint64              ctr_new, ctr_prev, ctr_diff;
    int                 index, max_index;
    int                 port_base, port_base_dma;
    int                 dma = 0; 
    int                 recheck_cntrs;
    int                 ar_idx;
    pbmp_t              counter_pbmp;
#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRX_SUPPORT)
    int                 pio_hold, pio_hold_enable;
    uint64              *counter_tmp = NULL;
    int                 tx_byt_to_led = 0;
    int                 rx_byt_to_led = 0;
    int                 rxcnt_xaui = 0;
    int                 skip_xaui_rx_counters;
    soc_reg_t           reg_1a = INVALIDr;
    soc_reg_t           reg_1b = INVALIDr;
    soc_reg_t           reg_2a = INVALIDr;
    soc_reg_t           reg_2b = INVALIDr;
#endif /* BCM_BRADLEY_SUPPORT || BCM_TRX_SUPPORT */

    soc_cm_debug(DK_COUNTER + DK_VERBOSE,
                 "soc_counter_collect64: unit=%d discard=%d\n",
                 unit, discard);

    /* hw_ctr_reg is INVALIDr for normal sw counter accumulation */ 
    if (hw_ctr_reg == INVALIDr) {
        dma = ((soc->counter_flags & SOC_COUNTER_F_DMA) && (discard != TRUE));
    }
    recheck_cntrs = soc_feature(unit, soc_feature_recheck_cntrs);

#ifdef BCM_BRADLEY_SUPPORT
    pio_hold = SOC_IS_HBX(unit);
    pio_hold_enable = (soc->counter_flags & SOC_COUNTER_F_HOLD);

    if (SOC_IS_HBX(unit) && soc_feature(unit, soc_feature_bigmac_rxcnt_bug)) {
        
        rxcnt_xaui = 1;
        reg_1a = MAC_TXPSETHRr;
        reg_1b = IRFCSr;
        reg_2a = MAC_TXMAXSZr;
        reg_2b = IROVRr;
        if (dma) {
            counter_tmp = soc_counter_tbuf[unit];
        }
    }
#endif /* BCM_BRADLEY_SUPPORT */

#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit) && soc_feature(unit, soc_feature_bigmac_rxcnt_bug)) {
        
        rxcnt_xaui = 1;
        reg_1a = MAC_TXPSETHRr;
#ifdef BCM_ENDURO_SUPPORT
        if(SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit)) {
            reg_1b = IRUCAr;
        } else
#endif /* BCM_ENDURO_SUPPORT */
        {
            reg_1b = IRUCr;
        }
        reg_2a = MAC_TXMAXSZr;
        reg_2b = IRFCSr;
        if (dma) {
            counter_tmp = soc_counter_tbuf[unit];
        }
    }
#endif /* BCM_TRX_SUPPORT */

    if (hw_ctr_reg == INVALIDr) {
        /* accumulate sw counters for all ports */
        SOC_PBMP_CLEAR(counter_pbmp);
        SOC_PBMP_ASSIGN(counter_pbmp, soc->counter_pbmp);
    } else {
        /* sync and accumulate sw counter for specified register of port */
        SOC_PBMP_CLEAR(counter_pbmp);
        port = tmp_port;
        SOC_PBMP_PORT_ADD(counter_pbmp, port);
    }      
     
    PBMP_ITER(counter_pbmp, port) {
        if (!SOC_IS_XGS3_SWITCH(unit) && !SOC_IS_ARAD(unit) &&
            !IS_HG_PORT(unit, port) && !IS_XE_PORT(unit, port)) {
                continue;
        }

        port_base = COUNTER_MIN_IDX_GET(unit, port);
        port_base_dma = COUNTER_MIN_IDX_GET(unit, port - soc->counter_ports32);

#ifdef BCM_ARAD_SUPPORT
        if (SOC_IS_ARAD(unit)) {
            if (_SOC_CONTROLLED_COUNTER_USE(unit, port)) {
                continue;
            }
            port_base += _SOC_CONTROLLED_COUNTER_NOF(unit);
        }
#endif /*BCM_ARAD_SUPPORT*/
#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRX_SUPPORT)
        /*
         * Handle XAUI Rx counter bug.
         * 
         * If the IRFCS counter returns the contents of the
         * MAC_TXPSETHR register, and the IROVR counter returns the
         * contents of the MAC_TXMAXSZ register, we assume that the
         * counters are bogus.
         *
         * By using two counters we significantly reduce the chance of
         * getting stuck if the real counter value matches the bogus
         * reference.
         *
         * To protect ourselves from the error condition happening
         * while the counter DMA is in progress, we manually check the
         * counters before and after we copy the DMA buffer contents.
         */
        skip_xaui_rx_counters = 1;
        if (rxcnt_xaui &&
            (IS_GX_PORT(unit, port) ||
             (IS_XG_PORT(unit, port) &&
              (IS_XE_PORT(unit, port) || IS_HG_PORT(unit, port))))) {
            uint64 val_1a, val_1b, val_2a, val_2b;
            uint32 *p1, *p2;
            int rxcnt_appear_valid = 0;

            /* Read reference values and counter values */
            if (soc_reg_get(unit, reg_1a, port, 0, &val_1a) == 0 &&
                soc_reg_get(unit, reg_1b, port, 0, &val_1b) == 0 &&
                soc_reg_get(unit, reg_2a, port, 0, &val_2a) == 0 &&
                soc_reg_get(unit, reg_2b, port, 0, &val_2b) == 0) {

                /* Check for bogus values */
                if (COMPILER_64_NE(val_1a, val_1b) || 
                    COMPILER_64_NE(val_2a, val_2b)) {
                    rxcnt_appear_valid = 1;
                }
                if (rxcnt_appear_valid && counter_tmp) {
                    /* Copy DMA buffer */
                    sal_memcpy(counter_tmp, &soc->counter_buf64[port_base_dma], 
                               SOC_COUNTER_TBUF_SIZE(unit));
                    p1 = (uint32 *)&counter_tmp[SOC_REG_CTR_IDX(unit, reg_1b)];
                    p2 = (uint32 *)&counter_tmp[SOC_REG_CTR_IDX(unit, reg_2b)];
                    /* Read counter values again */
                    if (soc_reg_get(unit, reg_1b, port, 0, &val_1b) == 0 &&
                        soc_reg_get(unit, reg_2b, port, 0, &val_2b) == 0) {
                        /* Check for bogus value again, including DMA buffer */
                        if ((COMPILER_64_NE(val_1a, val_1b) || 
                             COMPILER_64_NE(val_2a, val_2b)) && 
                            (p1[0] != COMPILER_64_LO(val_1a) ||
                             p2[0] != COMPILER_64_LO(val_2a))) {
                            /* Counters appears to be valid */
                            skip_xaui_rx_counters = 0;
                        }
                    }
                } else if (rxcnt_appear_valid && !dma) {
                    /* If not DMA we take the chance and read the counters */
                    skip_xaui_rx_counters = 0;
                }
            }
        }
#endif /* BCM_BRADLEY_SUPPORT || BCM_TRX_SUPPORT */

        if (hw_ctr_reg == INVALIDr) {
            index = 0;
            max_index = PORT_CTR_NUM(unit, port);
        } else {
            int  port_index1, num_entries1;
            char *cname;

            /* get index of counter register */
            SOC_IF_ERROR_RETURN(_soc_counter_get_info(unit, port, hw_ctr_reg,
                                                      &port_index1, &num_entries1,
                                                      &cname));

            index = port_index1 - port_base;
            max_index = index + 1;
        }    
            
        for ( ; index < max_index; index++) {
            volatile uint64 *vptr;
            COUNTER_ATOMIC_DEF s;

            ctr_reg = PORT_CTR_REG(unit, port, index)->reg;
            ar_idx = PORT_CTR_REG(unit, port, index)->index;

            if (SOC_COUNTER_INVALID(unit, ctr_reg)) {
                continue;
            }
            if (!SOC_REG_PORT_IDX_VALID(unit,ctr_reg, port, ar_idx)) {
                continue;
            }

#ifdef BCM_TRIUMPH2_SUPPORT
            /* Trap for BigMAC TX Timestamp FIFO reads
             *
             * MAC_TXTIMESTAMPFIFOREADr is immediately after
             * MAC_RXLLFCMSGCNTr.  It is a single register exposing
             * a HW FIFO which pops on read.  And it is in the counter
             * DMA range.  So we detect MAC_RXLLFCMSGCNTr and record
             * the value of the next piece of data in a SW FIFO for
             * use by the API.
             */
            if (soc_feature(unit, soc_feature_timestamp_counter) &&
                dma && (ctr_reg == MAC_RXLLFCMSGCNTr) &&
                (IS_XE_PORT(unit, port) || IS_HG_PORT(unit, port)) &&
                (soc->counter_timestamp_fifo[port] != NULL) &&
                !SHR_FIFO_IS_FULL(soc->counter_timestamp_fifo[port])) {
                uint32 *ptr =
                    (uint32 *)&soc->counter_buf64[port_base_dma + index + 1];

                if (ptr[0] != 0) { /* Else, HW FIFO empty */
                    SHR_FIFO_PUSH(soc->counter_timestamp_fifo[port], &(ptr[0]));
                }
            }
#endif /* BCM_TRIUMPH2_SUPPORT */

            /* Atomic because soc_counter_set may update 64-bit value */
            COUNTER_ATOMIC_BEGIN(s);
            ctr_prev = soc->counter_hw_val[port_base + index];
            COUNTER_ATOMIC_END(s);

#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRX_SUPPORT)
            if (pio_hold && ctr_reg == HOLDr) {
                if (pio_hold_enable) {
                    uint64 ctr_prev_x, ctr_prev_y;
                    uint32 ctr_x, ctr_y;
                    int idx_x, idx_y;
                    /*
                     * We need to account for the two values of HOLD
                     * separately to get correct rollover behavior.
                     * Use holes in counter space to store HOLD_X/Y values
                     * HOLD_X at RDBGC4r, hole, IUNHGIr
                     * HOLD_Y at IUNKOPCr, hole, RDBGC5r
                     */

                    idx_x = SOC_REG_CTR_IDX(unit, RDBGC4r) + 1;
                    idx_y = SOC_REG_CTR_IDX(unit, IUNKOPCr) + 1;

                    COUNTER_ATOMIC_BEGIN(s);
                    ctr_prev_x = soc->counter_hw_val[port_base + idx_x];
                    ctr_prev_y = soc->counter_hw_val[port_base + idx_y];
                    COUNTER_ATOMIC_END(s);

                    SOC_IF_ERROR_RETURN
                        (soc_reg32_get(unit, HOLD_Xr, port, ar_idx,
                                       &ctr_x));
                    SOC_IF_ERROR_RETURN
                        (soc_reg32_get(unit, HOLD_Yr, port, ar_idx,
                                       &ctr_y));

                    if (discard) {
                        /* Just save the new sum to be used below */
                        COMPILER_64_SET(ctr_new, 0, ctr_x + ctr_y);
                    } else {
                        int             width = 0;
                        uint64          wrap_amt;

                        COMPILER_64_SET(ctr_diff, 0, ctr_x);
                        /* We know the width of HOLDr is < 32 */
                        if (COMPILER_64_LT(ctr_diff, ctr_prev_x)) {
                            width =
                                SOC_REG_INFO(unit, ctr_reg).fields[0].len;
                            COMPILER_64_SET(wrap_amt, 0, 1UL << width);
                            COMPILER_64_ADD_64(ctr_diff, wrap_amt);
                        }
                        /* Calculate HOLD_X diff from previous */
                        COMPILER_64_SUB_64(ctr_diff, ctr_prev_x);
                        ctr_new = ctr_diff;

                        COMPILER_64_SET(ctr_diff, 0, ctr_y);
                        if (COMPILER_64_LT(ctr_diff, ctr_prev_y)) {
                            width =
                                SOC_REG_INFO(unit, ctr_reg).fields[0].len;
                            COMPILER_64_SET(wrap_amt, 0, 1UL << width);
                            COMPILER_64_ADD_64(ctr_diff, wrap_amt);
                        }
                        /* Calculate HOLD_Y diff from previous */
                        COMPILER_64_SUB_64(ctr_diff, ctr_prev_y);

                        /* Combine diffs */
                        COMPILER_64_ADD_64(ctr_new, ctr_diff);
                        /* Add previous value so logic below handles
                         * all of the other updates */
                        COMPILER_64_ADD_64(ctr_new, ctr_prev);
                    }
                        
                    /* Update previous values with new values.
                     * Since these are INVALID registers, the other
                     * counter logic will not touch them. */
                    COMPILER_64_SET(soc->counter_hw_val[port_base + idx_x], 0, ctr_x);
                    COMPILER_64_SET(soc->counter_hw_val[port_base + idx_y], 0, ctr_y);
                } else {
                    /* 
                     * The counter collected by DMA is not reliable 
                     * due to incorrect access type, so force zero.
                     */
                    COMPILER_64_ZERO(ctr_new);
                }
            } else if (IS_HYPLITE_PORT(unit, port) && is_xaui_rx_counter(ctr_reg)) {
                SOC_IF_ERROR_RETURN
                    (soc_reg_read(unit, ctr_reg,
                                    soc_reg_addr(unit, ctr_reg, port, ar_idx),
                                    &ctr_new));
            } else if (rxcnt_xaui && is_xaui_rx_counter(ctr_reg)) {
                if (skip_xaui_rx_counters) {
                    ctr_new = ctr_prev;
                } else if (counter_tmp) {
                    uint32 *ptr = (uint32 *)&counter_tmp[index];
                    /* No need to check for SWAP64 flag */
                    COMPILER_64_SET(ctr_new, ptr[1], ptr[0]);
                } else {
                    SOC_IF_ERROR_RETURN
                        (soc_reg_get(unit, ctr_reg, port, ar_idx,
                                     &ctr_new));
                }
            } else 
#endif /* BCM_BRADLEY_SUPPORT || BCM_TRX_SUPPORT */
            if (dma) {
#ifdef BCM_SBUSDMA_SUPPORT
                if (soc_feature(unit, soc_feature_sbusdma)) {
                    uint32 *bptr = (uint32 *)&soc->counter_buf64[port_base_dma];                    
                    if (index >= soc_counter_mib_offset[unit]) {
                        uint32 *ptr = bptr + soc_counter_mib_offset[unit];
                        uint64 *aptr = ((uint64*)ptr)+(index - soc_counter_mib_offset[unit]);
                        if (SOC_REG_IS_64(unit, ctr_reg)) {
                            ptr = (uint32*)aptr;
                            if (soc->counter_flags & SOC_COUNTER_F_SWAP64) {
                                COMPILER_64_SET(ctr_new, ptr[0], ptr[1]);
                            } else {
                                COMPILER_64_SET(ctr_new, ptr[1], ptr[0]);
                            }
                        } else {
                            ptr = ptr + (index - soc_counter_mib_offset[unit]);
                            COMPILER_64_SET(ctr_new, 0, ptr[0]);
                        }                        
                    } else {
                        uint32 *ptr = bptr+index;
                        COMPILER_64_SET(ctr_new, 0, (ptr[0] & 0x3fffffff));
                    }
                } else 
#endif
                {
                    uint32 *ptr =
                        (uint32 *)&soc->counter_buf64[port_base_dma + index];
                    if (soc->counter_flags & SOC_COUNTER_F_SWAP64) {
                        COMPILER_64_SET(ctr_new, ptr[0], ptr[1]);
                    } else {
                        COMPILER_64_SET(ctr_new, ptr[1], ptr[0]);
                    }
                }
            } else {
                SOC_IF_ERROR_RETURN
                    (soc_reg_get(unit, ctr_reg, port, ar_idx,
                                 &ctr_new));

            }
            if (soc_feature(unit, soc_feature_counter_parity)
#ifdef BCM_HURRICANE2_SUPPORT
                || soc_reg_field_valid(unit, ctr_reg, PARITYf)
#endif
               )
            {
                uint32 temp;
                if (soc_reg_field_valid(unit, ctr_reg, EVEN_PARITYf) || 
                    soc_reg_field_valid(unit, ctr_reg, PARITYf)) {
                    temp = COMPILER_64_LO(ctr_new) & ((1 << soc_reg_field_length
                           (unit, ctr_reg, COUNTf)) - 1);
                    COMPILER_64_SET(ctr_new, COMPILER_64_HI(ctr_new), temp);
                }
            }

            if ( (recheck_cntrs == TRUE) && 
                 COMPILER_64_NE(ctr_new, ctr_prev) ) {
                /* Seeds of doubt, double-check */
                uint64 ctr_new2;
                int suspicious = 0;
                SOC_IF_ERROR_RETURN
                  (soc_reg_get(unit, ctr_reg, port, 0,
                                &ctr_new2));
                
                /* Check for partial ordering, with wrap */
                if (COMPILER_64_LT(ctr_new, ctr_prev)) {
                    if (COMPILER_64_LT(ctr_new2, ctr_new) || 
                        COMPILER_64_GE(ctr_new2, ctr_prev)) {
                        /* prev < new2 < new, bad data */
                        /* Try again next time */
                        ctr_new = ctr_prev;
                        suspicious = 1;
                    }
                } else {
                    if (COMPILER_64_LT(ctr_new2, ctr_new) && 
                        COMPILER_64_GE(ctr_new2, ctr_prev)) {
                        /* prev < new2 < new, bad data */
                        /* Try again next time */
                        ctr_new = ctr_prev;
                        suspicious = 1;
                    }
                }
                /* Otherwise believe ctr_new */

                if (suspicious) {
#if !defined(SOC_NO_NAMES)
                    soc_cm_debug(DK_COUNTER, "soc_counter_collect64: "
                                 "unit %d, port%d: suspicious %s "
                                 "counter read (%s)\n",
                                 unit, port,
                                 dma?"DMA":"manual",
                                 SOC_REG_NAME(unit, ctr_reg));
#else
                    soc_cm_debug(DK_COUNTER, "soc_counter_collect64: "
                                 "unit %d, port%d: suspicious %s "
                                 "counter read (Counter%d)\n",
                                 unit, port,
                                 dma?"DMA":"manual",
                                 ctr_reg);
#endif /* SOC_NO_NAMES */
                } /* Suspicious counter change */
            } /* recheck_cntrs == TRUE */

            if (COMPILER_64_EQ(ctr_new, ctr_prev)) {
                COUNTER_ATOMIC_BEGIN(s);
                COMPILER_64_ZERO(soc->counter_delta[port_base + index]);
                COUNTER_ATOMIC_END(s);
                continue;
            }

            if (discard) {
                if (dma) {
                    uint32 *ptr;
#ifdef BCM_SBUSDMA_SUPPORT
                    if (soc_feature(unit, soc_feature_sbusdma)) {
                        uint32 *bptr = (uint32 *)&soc->counter_buf64[port_base_dma];                    
                        if (index >= soc_counter_mib_offset[unit]) {
                            uint64 *aptr;
                            ptr = bptr + soc_counter_mib_offset[unit];
                            if (SOC_REG_IS_64(unit, ctr_reg)) {
                                aptr = ((uint64*)ptr)+(index - soc_counter_mib_offset[unit]);
                                ptr = (uint32*)aptr;
                            } else {
                                ptr = ptr+(index-soc_counter_mib_offset[unit]);
                            }
                        } else {
                            ptr = bptr+index;
                        }
                    } else 
#endif
                    {
                        ptr = (uint32 *)&soc->counter_buf64[port_base_dma + index];
                    }
                    COUNTER_ATOMIC_BEGIN(s);
                    /* Update the DMA location */
                    if (soc->counter_flags & SOC_COUNTER_F_SWAP64) {
                        COMPILER_64_TO_32_HI(ptr[0], ctr_new);
                        COMPILER_64_TO_32_LO(ptr[1], ctr_new);
                    } else {
                        COMPILER_64_TO_32_LO(ptr[0], ctr_new);
                        COMPILER_64_TO_32_HI(ptr[1], ctr_new);
                    }
                } else {
                    COUNTER_ATOMIC_BEGIN(s);
                }
                /* Update the previous value buffer */
                soc->counter_hw_val[port_base + index] = ctr_new;
                COMPILER_64_ZERO(soc->counter_delta[port_base + index]);
                COUNTER_ATOMIC_END(s);
                continue;
            }

            soc_cm_debug(DK_COUNTER,
                         "soc_counter_collect64: ctr %d => 0x%08x_%08x\n",
                         port_base + index,
                         COMPILER_64_HI(ctr_new), COMPILER_64_LO(ctr_new));

            vptr = &soc->counter_sw_val[port_base + index];

            ctr_diff = ctr_new;

            if (COMPILER_64_LT(ctr_diff, ctr_prev)) {
                int             width, i = 0;
                uint64          wrap_amt;

                /*
                 * Counter must have wrapped around.
                 * Add the proper wrap-around amount.
                 */
                width = SOC_REG_INFO(unit, ctr_reg).fields[0].len;
                if (soc_feature(unit, soc_feature_counter_parity)
#ifdef BCM_HURRICANE2_SUPPORT
                    || soc_reg_field_valid(unit, ctr_reg, PARITYf)
#endif
                   )
                {
                    while((SOC_REG_INFO(unit, ctr_reg).fields + i) != NULL) { 
                        if (SOC_REG_INFO(unit, ctr_reg).fields[i].field == 
                            COUNTf) {
                            width = SOC_REG_INFO(unit, ctr_reg).fields[i].len;
                            break;
                        }
                        i++;
                    }
                }
                if (width < 32) {
                    COMPILER_64_SET(wrap_amt, 0, 1UL << width);
                    COMPILER_64_ADD_64(ctr_diff, wrap_amt);
                } else if (width < 64) {
                    COMPILER_64_SET(wrap_amt, 1UL << (width - 32), 0);
                    COMPILER_64_ADD_64(ctr_diff, wrap_amt);
                }
            }

            COMPILER_64_SUB_64(ctr_diff, ctr_prev);

            COUNTER_ATOMIC_BEGIN(s);
            COMPILER_64_ADD_64(*vptr, ctr_diff);
            soc->counter_delta[port_base + index] = ctr_diff;
            soc->counter_hw_val[port_base + index] = ctr_new;
            COUNTER_ATOMIC_END(s);

#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
            if (soc_feature(unit, soc_feature_ctr_xaui_activity)) {
#ifdef BCM_TRIDENT_SUPPORT
                if (SOC_IS_TD_TT(unit)) {
                    if (ctr_reg == RPKTr) {
                        COMPILER_64_TO_32_LO(rx_byt_to_led, ctr_diff); 
                    } else if (ctr_reg == TPKTr) {
                        COMPILER_64_TO_32_LO(tx_byt_to_led, ctr_diff); 
                    }
                } else
#endif
                {
#ifdef BCM_BRADLEY_SUPPORT
                    if (ctr_reg == IRBYTr) {
                        COMPILER_64_TO_32_LO(rx_byt_to_led, ctr_diff); 
                    } else if (ctr_reg == ITBYTr) {
                        COMPILER_64_TO_32_LO(tx_byt_to_led, ctr_diff); 
                    }
#endif
                }
            }
#endif
        }

#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
        if (soc_feature(unit, soc_feature_ctr_xaui_activity)) {
            _soc_xgs3_update_link_activity(unit, port, tx_byt_to_led,
                                           rx_byt_to_led);
            tx_byt_to_led = 0;
            rx_byt_to_led = 0;
        }
#endif
        /* If signaled to exit then return  */
        if (!soc->counter_interval) {
            return SOC_E_NONE;
        }

        /*
         * Allow other tasks to run between processing each port.
         */

        if (hw_ctr_reg == INVALIDr) { 
            sal_thread_yield();
        }
    }

    return SOC_E_NONE;
}
#endif /*defined(BCM_XGS_SUPPORT) || defined(BCM_ARAD_SUPPORT)*/

#ifdef BCM_KATANA_SUPPORT
STATIC int
_soc_counter_katana_a0_non_dma_entries(int unit, int dma_id) 
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma, *non_dma1;
    int base_index, id, width, i, index;
    int rv, entry_words;
    uint32 fval[2];
    uint64 ctr_new;
    int buffer_seg, seg_addr;
    soc_mem_t buffer_mem, dma_mem;
    uint32 rval = 0;
    COUNTER_ATOMIC_DEF s;
    int field_count;
    soc_field_t field;
   
    soc_cm_debug(DK_COUNTER + DK_VERBOSE,
                 "_soc_counter_katana_a0_non_dma_entries: unit=%d id=%d\n",
                 unit, dma_id);

    soc = SOC_CONTROL(unit);

    if (soc->counter_non_dma == NULL) {
        return SOC_E_RESOURCE;
    }

    non_dma = &soc->counter_non_dma[dma_id];    

    if (non_dma->flags & _SOC_COUNTER_NON_DMA_DO_DMA) {
        non_dma1 = &soc->counter_non_dma[dma_id + 1];
    } else {
        return SOC_E_NONE;
    }
 
    entry_words = soc_mem_entry_words(unit, non_dma->mem);
    if (non_dma->field != INVALIDf) {
        width = soc_mem_field_length(unit, non_dma->mem,
                                     non_dma->field);
    } else {
        width = 32;
    }

    buffer_seg = (soc_feature(unit, soc_feature_cosq_gport_stat_ability)) ?
                 (11 * 1024) : (8 * 1024); 
    buffer_mem = (soc_feature(unit, soc_feature_cosq_gport_stat_ability)) ?
                 CTR_FLEX_COUNT_11m : CTR_FLEX_COUNT_8m;

    for (id = 0; id < 2; id++) {
        index = non_dma->mem - CTR_FLEX_COUNT_0m;
        seg_addr = (id == 0) ? buffer_seg : (index * 1024);
        soc_reg_field_set(unit, CTR_SEGMENT_STARTr, &rval, SEG_STARTf,
                          seg_addr);
        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit,CTR_SEGMENT_STARTr, REG_PORT_ANY,index, rval));
        soc_cm_debug(DK_COUNTER + DK_VERBOSE,
                     "id=%d index=%d seg_addr=%d \n",
                     id, index, seg_addr);
        for (i = 0; i < 4; i++) {
            if (non_dma->flags & _SOC_COUNTER_NON_DMA_DO_DMA) {
                if (non_dma->dma_buf[i] == NULL) {
                    continue;
                }

                dma_mem = (id == 0) ? non_dma->dma_mem[i] : (buffer_mem + i);            
                rv = soc_mem_read_range(unit, dma_mem,
                                        MEM_BLOCK_ANY,
                                        non_dma->dma_index_min[i],
                                        non_dma->dma_index_max[i],
                                        non_dma->dma_buf[i]);
                if (SOC_FAILURE(rv)) {
                    return rv;
                }
            }
        }

        for (field_count = 0; field_count < 2; field_count++) {    
            base_index = (field_count == 0) ? non_dma->base_index : 
                                              non_dma1->base_index;
            field  = (field_count == 0) ? non_dma->field : 
                                          non_dma1->field;
            for (i = 0; i < non_dma->num_entries; i++) {
                soc_mem_field_get(unit, non_dma->mem,
                                  &non_dma->dma_buf[0][entry_words * i],
                                  field, fval);
                if (width <= 32) {
                    COMPILER_64_SET(ctr_new, 0, fval[0]);
                } else {
                    COMPILER_64_SET(ctr_new, fval[1], fval[0]);
                }
            
                COUNTER_ATOMIC_BEGIN(s);
                COMPILER_64_ADD_64(soc->counter_sw_val[base_index + i],
                                   ctr_new);
                soc->counter_hw_val[base_index + i] = ctr_new;
                soc->counter_delta[base_index + i] = ctr_new;
                COUNTER_ATOMIC_END(s);
            }
        }
    }
    return SOC_E_NONE;
}
#endif /* BCM_KATANA_SUPPORT */

STATIC void
soc_counter_collect_non_dma_entries(int unit)
{
    soc_control_t *soc;
    soc_counter_non_dma_t *non_dma;
    int base_index, id, width, i;
    int port;
    uint32 fval[2];
    uint64 ctr_new, ctr_prev, ctr_diff, wrap_amt;
    int rv, count, entry_words;
    COUNTER_ATOMIC_DEF s;

    soc = SOC_CONTROL(unit);

    if (soc->counter_non_dma == NULL) {
        return;
    }

    for (id = 0; id < SOC_COUNTER_NON_DMA_END - SOC_COUNTER_NON_DMA_START;
         id++) {
        non_dma = &soc->counter_non_dma[id];

        if (!(non_dma->flags & _SOC_COUNTER_NON_DMA_VALID)) {
            continue;
        }

        /* If signaled to exit then return  */
        if (!soc->counter_interval) {
            return;
        }

        width = entry_words = 0;

        if (non_dma->mem != INVALIDm) {
#ifdef BCM_KATANA_SUPPORT
            if (soc_feature(unit, soc_feature_counter_toggled_read)) {
                rv = _soc_counter_katana_a0_non_dma_entries(unit, id);   
                if (SOC_FAILURE(rv)) {
                    return;
                } else {
                    continue;  
                }   
            }
#endif
            entry_words = soc_mem_entry_words(unit, non_dma->mem);
            if (non_dma->field != INVALIDf) {
                width = soc_mem_field_length(unit, non_dma->mem,
                                             non_dma->field);
            } else {
                width = 32;
            }

            for (i = 0; i < 4; i++) {
                if (non_dma->flags & _SOC_COUNTER_NON_DMA_DO_DMA) {
                    if (non_dma->dma_buf[i] == NULL) {
                        continue;
                    }
                    rv = soc_mem_read_range(unit, non_dma->dma_mem[i],
                                            MEM_BLOCK_ANY,
                                            non_dma->dma_index_min[i],
                                            non_dma->dma_index_max[i],
                                            non_dma->dma_buf[i]);
                    if (SOC_FAILURE(rv)) {
                        return;
                    }
                }
            }

            base_index = non_dma->base_index;
			
            for (i = 0; i < non_dma->num_entries; i++) {
                soc_mem_field_get(unit, non_dma->mem,
                                  &non_dma->dma_buf[0][entry_words * i],
                                  non_dma->field, fval);
                if (width <= 32) {
                    COMPILER_64_SET(ctr_new, 0, fval[0]);
                } else {
                    COMPILER_64_SET(ctr_new, fval[1], fval[0]);
                }
                if (non_dma->flags & _SOC_COUNTER_NON_DMA_EXTRA_COUNT) {
                    COMPILER_64_SUB_32(ctr_new, 1);
                } 

                COUNTER_ATOMIC_BEGIN(s);
                ctr_prev = soc->counter_hw_val[base_index + i];
                COUNTER_ATOMIC_END(s);

                if (!(non_dma->flags & _SOC_COUNTER_NON_DMA_CLEAR_ON_READ)) {
                    if (COMPILER_64_EQ(ctr_new, ctr_prev)) {
                        COUNTER_ATOMIC_BEGIN(s);
                        COMPILER_64_ZERO(soc->counter_delta[base_index + i]);
                        COUNTER_ATOMIC_END(s);
                        continue;
                    }
                }

                ctr_diff = ctr_new;

                if (non_dma->flags & _SOC_COUNTER_NON_DMA_PEAK) {
                    COUNTER_ATOMIC_BEGIN(s);
                    if (COMPILER_64_GT(ctr_new, ctr_prev)) {
                        soc->counter_sw_val[base_index + i] = ctr_new;
                    }
                    soc->counter_hw_val[base_index + i] = ctr_new;
                    COMPILER_64_ZERO(soc->counter_delta[base_index + i]);
                    COUNTER_ATOMIC_END(s);
                    continue;
                }

                if (non_dma->flags & _SOC_COUNTER_NON_DMA_CURRENT) {
                    COUNTER_ATOMIC_BEGIN(s);
                    soc->counter_sw_val[base_index + i] = ctr_new;
                    soc->counter_hw_val[base_index + i] = ctr_new;
                    COMPILER_64_ZERO(soc->counter_delta[base_index + i]);
                    COUNTER_ATOMIC_END(s);
                    continue;
                }

                if (!(non_dma->flags & _SOC_COUNTER_NON_DMA_CLEAR_ON_READ)) {
                    if (COMPILER_64_LT(ctr_diff, ctr_prev)) {
                        if (width < 32) {
                            COMPILER_64_ADD_32(ctr_diff, 1U << width);
                        } else if (width < 64) {
                            COMPILER_64_SET(wrap_amt, 1U << (width - 32), 0);
                            COMPILER_64_ADD_64(ctr_diff, wrap_amt);
                        }
                    }

                    COMPILER_64_SUB_64(ctr_diff, ctr_prev);
                }

                COUNTER_ATOMIC_BEGIN(s);
                COMPILER_64_ADD_64(soc->counter_sw_val[base_index + i],
                                   ctr_diff);
                soc->counter_hw_val[base_index + i] = ctr_new;
                soc->counter_delta[base_index + i] = ctr_diff;
                COUNTER_ATOMIC_END(s);
            }
        } else if (non_dma->reg != INVALIDr) {
            if (non_dma->field != INVALIDf) {
                width = soc_reg_field_length(unit, non_dma->reg, 
                                             non_dma->field);
            } else {
                width = 32;
            }

            PBMP_ITER(soc->counter_pbmp, port) {
                if (non_dma->entries_per_port == 0) {
                    /* OK to over-write port variable as loop breaks after 1 iter. 
                       Check below for the same codition." */
                    port = REG_PORT_ANY;
                    count = non_dma->num_entries;
                    base_index = non_dma->base_index;
                } else {
                    if (!SOC_PBMP_MEMBER(non_dma->pbmp, port)) {
                        continue;
                    }

                    if (SOC_IS_TR_VL(unit) || SOC_IS_SC_CQ(unit)) {
                        if (non_dma->reg == TXLLFCMSGCNTr) {
                            if (!IS_HG_PORT( unit, port)) {
                                continue;
                            }
                        } 
                    } 

                    if (non_dma->flags & _SOC_COUNTER_NON_DMA_PERQ_REG) {
                        count = num_cosq[unit][port];
                        base_index = non_dma->base_index;
                        for (i = 0; i < port; i++) {
                            base_index += num_cosq[unit][i];
                        }
                    } else {
                        count = non_dma->entries_per_port;
                        base_index = non_dma->base_index + port * count;
                    }
                }

                for (i = 0; i < count; i++) {
                    /* coverity[negative_returns : FALSE] */
                    rv = soc_reg_get(unit, non_dma->reg, port, i, &ctr_new);
                    if (SOC_FAILURE(rv)) {
                        return;
                    }
                    COUNTER_ATOMIC_BEGIN(s);
                    ctr_prev = soc->counter_hw_val[base_index + i];
                    COUNTER_ATOMIC_END(s);

                    if (COMPILER_64_EQ(ctr_new, ctr_prev)) {
                        COUNTER_ATOMIC_BEGIN(s);
                        COMPILER_64_ZERO(soc->counter_delta[base_index + i]);
                        COUNTER_ATOMIC_END(s);
                        continue;
                    }

                    ctr_diff = ctr_new;

                    if (non_dma->flags & _SOC_COUNTER_NON_DMA_PEAK) {
                        COUNTER_ATOMIC_BEGIN(s);
                        if (COMPILER_64_GT(ctr_new, ctr_prev)) {
                            soc->counter_sw_val[base_index + i] = ctr_new;
                        }
                        soc->counter_hw_val[base_index + i] = ctr_new;
                        COMPILER_64_ZERO(soc->counter_delta[base_index + i]);
                        COUNTER_ATOMIC_END(s);
                        continue;
                    }

                    if (non_dma->flags & _SOC_COUNTER_NON_DMA_CURRENT) {
                        COUNTER_ATOMIC_BEGIN(s);
                        soc->counter_sw_val[base_index + i] = ctr_new;
                        soc->counter_hw_val[base_index + i] = ctr_new;
                        COMPILER_64_ZERO(soc->counter_delta[base_index + i]);
                        COUNTER_ATOMIC_END(s);
                        continue;
                    }

                    if (COMPILER_64_LT(ctr_diff, ctr_prev)) {
                        if (width < 32) {
                            COMPILER_64_ADD_32(ctr_diff, 1U << width);
                        } else if (width < 64) {
                            COMPILER_64_SET(wrap_amt, 1U << (width - 32), 0);
                            COMPILER_64_ADD_64(ctr_diff, wrap_amt);
                        }
                    }

                    COMPILER_64_SUB_64(ctr_diff, ctr_prev);

                    COUNTER_ATOMIC_BEGIN(s);
                    COMPILER_64_ADD_64(soc->counter_sw_val[base_index + i],
                                       ctr_diff);
                    soc->counter_hw_val[base_index + i] = ctr_new;
                    soc->counter_delta[base_index + i] = ctr_diff;
                    COUNTER_ATOMIC_END(s);
                }
                if (non_dma->entries_per_port == 0) {
                    break;
                }
            }
        } else {
            continue;
        }
    }
}

#ifdef BCM_XGS3_SWITCH_SUPPORT
/*
 * Function:
 *      _soc_xgs3_counter_dma_setup
 * Purpose:
 *      Configure hardware registers for counter collection.  Used during
 *      soc_counter_thread initialization.
 * Parameters:
 *      unit    - switch unit
 * Returns:
 *      SOC_E_*
 */
STATIC int
_soc_xgs3_counter_dma_setup(int unit)
{
    soc_control_t       *soc;
    int                 csize;
    pbmp_t              pbmp;
    soc_reg_t           reg;
    soc_ctr_type_t      ctype;
    uint32              val;
    uint32              ing_blk, ing_blkoff, ing_pstage, ing_c_cnt;
    uint32              egr_blk, egr_blkoff, egr_pstage, egr_c_cnt;
#ifdef BCM_SHADOW_SUPPORT
    uint32              inv_c_cnt = 0;
#endif
    uint32              gmac_c_cnt, xmac_c_cnt;
    int                 blk, bindex, blk_num, port, phy_port;
    soc_reg_t           blknum_reg, portnum_reg;
    int                 num_blknum, num_portnum, blknum_offset, portnum_offset;
    uint32              blknum_mask, portnum_mask;
    uint32              blknum_map[20];
    uint32              portnum_map[24];
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
#endif
    ing_blk = ing_blkoff = ing_pstage = 0;
    egr_blk = egr_blkoff = egr_pstage = 0;

#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FBX(unit)) {
        ing_blk = SOC_BLK_IPIPE;
        egr_blk = SOC_BLK_EPIPE;
        ing_blkoff = SOC_BLOCK2OFFSET(unit, IPIPE_BLOCK(unit));
        egr_blkoff = SOC_BLOCK2OFFSET(unit, EPIPE_BLOCK(unit));
    }
#endif

    soc = SOC_CONTROL(unit);
    SOC_PBMP_ASSIGN(pbmp, soc->counter_pbmp);
    /*
     * Calculate the number of counters from each block
     */
    ing_c_cnt = egr_c_cnt = gmac_c_cnt = xmac_c_cnt = 0;
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        inv_c_cnt = ing_c_cnt;
    }
#endif
    for (ctype = 0; ctype < SOC_CTR_NUM_TYPES; ctype++) {
        if (!SOC_HAS_CTR_TYPE(unit, ctype)) {
            continue;
        }
        for (csize = SOC_CTR_MAP_SIZE(unit, ctype) - 1; csize > 0; csize--) {
            reg = SOC_CTR_TO_REG(unit, ctype, csize);
            if (SOC_COUNTER_INVALID(unit, reg)) {
                continue;                       /* skip trailing invalids */
            }
            if (SOC_REG_BLOCK_IS(unit, reg, ing_blk) &&
                                 (ing_c_cnt == 0)) {
                ing_c_cnt = csize + 1;
                if (soc_feature(unit, soc_feature_new_sbus_format)) {
                    ing_pstage = (SOC_REG_INFO(unit, reg).offset >> 26) & 0x3F;
                } else {
                    ing_pstage = (SOC_REG_INFO(unit, reg).offset >> 24) & 0xFF;
                }
            }
            if (SOC_REG_BLOCK_IS(unit, reg, egr_blk) &&
                                 (egr_c_cnt == 0)) {
                egr_c_cnt = csize + 1;
                if (soc_feature(unit, soc_feature_new_sbus_format)) {
                    egr_pstage = (SOC_REG_INFO(unit, reg).offset >> 26) & 0x3F;
                } else {
                    egr_pstage = (SOC_REG_INFO(unit, reg).offset >> 24) & 0xFF;
                }
            }
            if ((SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_GPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_QGPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_SPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XGPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XQPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_GXPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XLPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XTPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XWPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_CLPORT)) &&
                (gmac_c_cnt == 0)) {
                gmac_c_cnt = csize + 1;
            }
            if ((SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XGPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XQPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_GXPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XLPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XTPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XWPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_MXQPORT) ||
                 SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_CLPORT)) &&
                (xmac_c_cnt == 0)) {
                xmac_c_cnt = csize + 1;
            }
        }
    }
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        /* Remove leading invalids (Shadow) */
        for (csize = 0; csize < SOC_CTR_MAP_SIZE(unit, SOC_CTR_TYPE_XE); csize++) {
            reg = SOC_CTR_TO_REG(unit, SOC_CTR_TYPE_XE, csize);
            if (SOC_COUNTER_INVALID(unit, reg)) {
                inv_c_cnt++;
            } else {
                break;
            }
        }
    }
#endif /* BCM_SHADOW_SUPPORT */

    egr_c_cnt = (egr_c_cnt) ? (egr_c_cnt - ing_c_cnt) : 0;
    gmac_c_cnt = (gmac_c_cnt) ? (gmac_c_cnt - egr_c_cnt - ing_c_cnt) : 0;
    xmac_c_cnt = (xmac_c_cnt) ? (xmac_c_cnt - egr_c_cnt - ing_c_cnt) : 0;
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        ing_c_cnt -= inv_c_cnt;
    }
#endif

    soc_cm_debug(DK_COUNTER,
                 "ing_c_cnt = %d egr_c_cnt = %d "
                 "gmac_c_cnt = %d xmac_c_cnt = %d\n"
                 "ing_pstage = %d egr_pstage = %d "
                 "ing_blkoff = %d egr_blkoff = %d\n",
                 ing_c_cnt, egr_c_cnt, gmac_c_cnt, xmac_c_cnt,
                 ing_pstage, egr_pstage, ing_blkoff, egr_blkoff);

    /* Prepare for CMIC_STAT_DMA_PORTNUM_MAP and BLKNUM_MAP */
    if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_PORTNUM_MAP_3_0r)) {
        portnum_reg = CMIC_STAT_DMA_PORTNUM_MAP_3_0r;
        num_portnum = 4;
        portnum_offset = 8;
    } else if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_PORTNUM_MAP_5_0r)) {
        portnum_reg = CMIC_STAT_DMA_PORTNUM_MAP_5_0r;
        num_portnum = 6;
        portnum_offset = 5;
    } else if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_PORTNUM_MAP_7_0r)) {
        portnum_reg = CMIC_STAT_DMA_PORTNUM_MAP_7_0r;
        num_portnum = 8;
        portnum_offset = 4;
    } else {
        return SOC_E_INTERNAL;
    }
    portnum_mask =
        (1 << soc_reg_field_length(unit, portnum_reg, SBUS_PORTNUM_0f)) - 1;

    if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_BLKNUM_MAP_4_0r)) {
        blknum_reg = CMIC_STAT_DMA_BLKNUM_MAP_4_0r;
        num_blknum = 5;
        blknum_offset = 6;
    } else if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_BLKNUM_MAP_7_0r)) {
        blknum_reg = CMIC_STAT_DMA_BLKNUM_MAP_7_0r;
        num_blknum = 8;
        blknum_offset = 4;
    } else {
        return SOC_E_INTERNAL;
    }
    blknum_mask =
        (1 << soc_reg_field_length(unit, blknum_reg, SBUS_BLKNUM_0f)) - 1;

    sal_memset(portnum_map, 0, sizeof(portnum_map));
    sal_memset(blknum_map, 0, sizeof(blknum_map));
    PBMP_ITER(pbmp, port) {
        /* Check port value to avoid out of bound array access */
        if ((port / num_blknum) >= 20) {
            return SOC_E_INTERNAL;
        }
        if ((port / num_portnum) >= 24) {
            return SOC_E_INTERNAL;
        }
        if (soc_feature(unit, soc_feature_logical_port_num)) {
            phy_port = SOC_INFO(unit).port_l2p_mapping[port];
        } else {
            phy_port = port;
        }
        blk = SOC_PORT_BLOCK(unit, phy_port);
        bindex = SOC_PORT_BINDEX(unit, phy_port);
        blk_num = SOC_BLOCK2OFFSET(unit, blk);
        /* coverity[overrun-local] */
        blknum_map[port / num_blknum] |=
            (blk_num & blknum_mask) << (blknum_offset * (port % num_blknum));
        /* coverity[overrun-local] */
        portnum_map[port / num_portnum] |=
            (bindex & portnum_mask) << (portnum_offset * (port % num_portnum));
    }

    switch (portnum_reg) {
    case CMIC_STAT_DMA_PORTNUM_MAP_3_0r:
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_3_0r(unit, portnum_map[0]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_7_4r(unit, portnum_map[1]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_11_8r(unit, portnum_map[2]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_15_12r(unit, portnum_map[3]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_19_16r(unit, portnum_map[4]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_23_20r(unit, portnum_map[5]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_27_24r(unit, portnum_map[6]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_31_28r(unit, portnum_map[7]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_35_32r(unit, portnum_map[8]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_39_36r(unit, portnum_map[9]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_43_40r(unit, portnum_map[10]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_47_44r(unit, portnum_map[11]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_51_48r(unit, portnum_map[12]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_55_52r(unit, portnum_map[13]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_59_56r(unit, portnum_map[14]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_63_60r(unit, portnum_map[15]);
        if (!SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_PORTNUM_MAP_67_64r)) {
            break;
        }
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_67_64r(unit, portnum_map[16]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_71_68r(unit, portnum_map[17]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_75_72r(unit, portnum_map[18]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_79_76r(unit, portnum_map[19]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_83_80r(unit, portnum_map[20]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_87_84r(unit, portnum_map[21]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_91_88r(unit, portnum_map[22]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_95_92r(unit, portnum_map[23]);
        break;
    case CMIC_STAT_DMA_PORTNUM_MAP_5_0r:
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_5_0r(unit, portnum_map[0]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_11_6r(unit, portnum_map[1]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_17_12r(unit, portnum_map[2]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_23_18r(unit, portnum_map[3]);
        break;
    case CMIC_STAT_DMA_PORTNUM_MAP_7_0r:
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_7_0r(unit, portnum_map[0]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_15_8r(unit, portnum_map[1]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_23_16r(unit, portnum_map[2]);
        WRITE_CMIC_STAT_DMA_PORTNUM_MAP_31_24r(unit, portnum_map[3]);
        break;
    }

    switch (blknum_reg) {
    case CMIC_STAT_DMA_BLKNUM_MAP_4_0r:
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_4_0r(unit, blknum_map[0]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_9_5r(unit, blknum_map[1]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_14_10r(unit, blknum_map[2]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_19_15r(unit, blknum_map[3]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_24_20r(unit, blknum_map[4]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_29_25r(unit, blknum_map[5]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_34_30r(unit, blknum_map[6]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_39_35r(unit, blknum_map[7]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_44_40r(unit, blknum_map[8]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_49_45r(unit, blknum_map[9]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_54_50r(unit, blknum_map[10]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_59_55r(unit, blknum_map[11]);
        if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_BLKNUM_MAP_63_60r)) {
            WRITE_CMIC_STAT_DMA_BLKNUM_MAP_63_60r(unit, blknum_map[12]);
            break;
        }
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_64_60r(unit, blknum_map[12]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_69_65r(unit, blknum_map[13]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_74_70r(unit, blknum_map[14]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_79_75r(unit, blknum_map[15]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_84_80r(unit, blknum_map[16]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_89_85r(unit, blknum_map[17]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_94_90r(unit, blknum_map[18]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_95r(unit, blknum_map[19]);
        break;
    case CMIC_STAT_DMA_BLKNUM_MAP_7_0r:
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_7_0r(unit, blknum_map[0]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_15_8r(unit, blknum_map[1]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_23_16r(unit, blknum_map[2]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_31_24r(unit, blknum_map[3]);
        if (!SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_BLKNUM_MAP_39_32r)) {
            break;
        }
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_39_32r(unit, blknum_map[4]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_47_40r(unit, blknum_map[5]);
        WRITE_CMIC_STAT_DMA_BLKNUM_MAP_55_48r(unit, blknum_map[6]);

        break;
    }

    /*
     * Reset value in CMIC_STAT_DMA_SETUP register is good. No
     * additional setup necessary for FB/ER
     */
#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm)) {
        soc_pci_write(unit,CMIC_CMCx_STAT_DMA_ADDR_OFFSET(cmc),soc_cm_l2p(unit, soc->counter_buf64));
        soc_pci_write(unit,CMIC_CMCx_STAT_DMA_PORTS_0_OFFSET(cmc),SOC_PBMP_WORD_GET(pbmp, 0));
        if (SOC_REG_IS_VALID(unit, CMIC_CMC0_STAT_DMA_PORTS_1r)) { 
            soc_pci_write(unit,CMIC_CMCx_STAT_DMA_PORTS_1_OFFSET(cmc),SOC_PBMP_WORD_GET(pbmp, 1));
        }
        if (SOC_REG_IS_VALID(unit, CMIC_CMC0_STAT_DMA_PORTS_2r)) {  
            soc_pci_write(unit,CMIC_CMCx_STAT_DMA_PORTS_2_OFFSET(cmc),SOC_PBMP_WORD_GET(pbmp, 2));
        }
        SOC_PBMP_ASSIGN(pbmp, PBMP_HG_ALL(unit));
        SOC_PBMP_OR(pbmp, PBMP_XE_ALL(unit));
        SOC_PBMP_AND(pbmp, soc->counter_pbmp);

        WRITE_CMIC_STAT_DMA_PORT_TYPE_MAP_0r(unit, SOC_PBMP_WORD_GET(pbmp, 0));
        if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_PORT_TYPE_MAP_1r)) {
            WRITE_CMIC_STAT_DMA_PORT_TYPE_MAP_1r
                (unit, SOC_PBMP_WORD_GET(pbmp, 1));
        }
        if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_PORT_TYPE_MAP_2r)) {
            WRITE_CMIC_STAT_DMA_PORT_TYPE_MAP_2r
                (unit, SOC_PBMP_WORD_GET(pbmp, 2));
        }
    } else
#endif /* CMICM Support */
    {
        WRITE_CMIC_STAT_DMA_ADDRr(unit, soc_cm_l2p(unit, soc->counter_buf64));

        WRITE_CMIC_STAT_DMA_PORTSr(unit, SOC_PBMP_WORD_GET(pbmp, 0));
        if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_PORTS_HIr)) {
            WRITE_CMIC_STAT_DMA_PORTS_HIr(unit, SOC_PBMP_WORD_GET(pbmp, 1));
        }
        if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_PORTS_HI_2r)) {
            WRITE_CMIC_STAT_DMA_PORTS_HI_2r(unit, SOC_PBMP_WORD_GET(pbmp, 2));
        }

        if (SOC_IS_HB_GW(unit)) {
            SOC_PBMP_ASSIGN(pbmp, PBMP_PORT_ALL(unit));
        } else {
            SOC_PBMP_ASSIGN(pbmp, PBMP_HG_ALL(unit));
            SOC_PBMP_OR(pbmp, PBMP_XE_ALL(unit));
        }
        SOC_PBMP_AND(pbmp, soc->counter_pbmp);

        WRITE_CMIC_STAT_DMA_PORT_TYPE_MAPr(unit, SOC_PBMP_WORD_GET(pbmp, 0));
        if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_PORT_TYPE_MAP_HIr)) {
            WRITE_CMIC_STAT_DMA_PORT_TYPE_MAP_HIr
                (unit, SOC_PBMP_WORD_GET(pbmp, 1));
        }
        if (SOC_REG_IS_VALID(unit, CMIC_STAT_DMA_PORT_TYPE_MAP_HI_2r)) {
            WRITE_CMIC_STAT_DMA_PORT_TYPE_MAP_HI_2r
                (unit, SOC_PBMP_WORD_GET(pbmp, 2));
        }
    }
    val = 0;
    soc_reg_field_set(unit, CMIC_STAT_DMA_ING_STATS_CFGr,
                      &val, ING_ETH_BLK_NUMf, ing_blkoff);
    soc_reg_field_set(unit, CMIC_STAT_DMA_ING_STATS_CFGr,
                      &val, ING_STAT_COUNTERS_NUMf, ing_c_cnt);
    soc_reg_field_set(unit, CMIC_STAT_DMA_ING_STATS_CFGr,
                      &val, ING_STATS_PIPELINE_STAGE_NUMf, ing_pstage);
    WRITE_CMIC_STAT_DMA_ING_STATS_CFGr(unit, val);

    val = 0;
    soc_reg_field_set(unit, CMIC_STAT_DMA_EGR_STATS_CFGr,
                      &val, EGR_ETH_BLK_NUMf, egr_blkoff);
    soc_reg_field_set(unit, CMIC_STAT_DMA_EGR_STATS_CFGr,
                      &val, EGR_STAT_COUNTERS_NUMf, egr_c_cnt);
    soc_reg_field_set(unit, CMIC_STAT_DMA_EGR_STATS_CFGr,
                      &val, EGR_STATS_PIPELINE_STAGE_NUMf, egr_pstage);
    WRITE_CMIC_STAT_DMA_EGR_STATS_CFGr(unit, val);

    val = 0;
    soc_reg_field_set(unit, CMIC_STAT_DMA_MAC_STATS_CFGr,
                      &val, MAC_G_STAT_COUNTERS_NUMf, gmac_c_cnt);
    soc_reg_field_set(unit, CMIC_STAT_DMA_MAC_STATS_CFGr,
                      &val, MAC_X_STAT_COUNTERS_NUMf, xmac_c_cnt);
    soc_reg_field_set(unit, CMIC_STAT_DMA_MAC_STATS_CFGr,
                      &val, CPU_STATS_PORT_NUMf, CMIC_PORT(unit));
    WRITE_CMIC_STAT_DMA_MAC_STATS_CFGr(unit, val);

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm)) {
        val = soc_pci_read(unit,CMIC_CMCx_STAT_DMA_CFG_OFFSET(cmc));
        soc_reg_field_set(unit, CMIC_CMC0_STAT_DMA_CFGr, &val,
                          ST_DMA_ITER_DONE_CLRf, 1);
        if (soc_feature(unit, soc_feature_new_sbus_format)) {
            soc_reg_field_set(unit, CMIC_CMC0_STAT_DMA_CFGr, &val,
                              EN_TR3_SBUS_STYLEf, 1);
        }
        soc_pci_write(unit,CMIC_CMCx_STAT_DMA_CFG_OFFSET(cmc),val);
        soc_cmicm_intr0_enable(unit, IRQ_CMCx_STAT_ITER_DONE);
    } else
#endif
    {
        WRITE_CMIC_DMA_STATr(unit, DS_STAT_DMA_ITER_DONE_CLR);
        soc_intr_enable(unit, IRQ_STAT_ITER_DONE);
    }
    return SOC_E_NONE;
}

#ifdef BCM_SBUSDMA_SUPPORT
void _soc_sbusdma_port_ctr_cb(int unit, int status, sbusdma_desc_handle_t handle, 
                              void *data)
{
    soc_cm_debug(DK_COUNTER+DK_VERBOSE, "In port counter cb [%d]\n", handle);
    if (status == SOC_E_NONE) {
        soc_cm_debug(DK_COUNTER+DK_VERBOSE, "Complete: port:%d.\n", 
                     SOC_INFO(unit).port_p2l_mapping[PTR_TO_INT(data)]);
    } else {
        uint8 i;
        int blk;
        soc_cm_debug(DK_ERR, "Counter SBUSDMA failed: port:%d.\n", 
                     SOC_INFO(unit).port_p2l_mapping[PTR_TO_INT(data)]);
        if (status == SOC_E_TIMEOUT) {
            (void)soc_sbusdma_desc_delete(unit, handle);
            for (i=0; i<=2; i++) {
                if (_soc_port_counter_handles[unit][PTR_TO_INT(data)][i] == handle) {
                    _soc_port_counter_handles[unit][PTR_TO_INT(data)][i] = 0;
                    break;
                }
                blk = SOC_PORT_BLOCK(unit, PTR_TO_INT(data));
                if ((i == 2) && 
                    (!SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_XLPORT) && 
                     !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_XTPORT) &&
                     !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_XWPORT) &&
                     !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_MXQPORT) &&
                     !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_GPORT) &&
                     !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_CLPORT))) {
                    break;
                }
            }
        }
    }
    _soc_counter_pending[unit]--;
}

STATIC int
_soc_counter_sbusdma_setup(int unit)
{
    soc_control_t       *soc;
    int                 csize;
    soc_ctr_type_t      ctype;
    soc_reg_t           reg, ing_reg, egr_reg, mac_reg, gmac_reg;
    int                 ing_c_cnt, egr_c_cnt, mac_c_cnt, gmac_c_cnt;
    uint32              ing_addr, egr_addr, mac_addr;
    int                 ing_blkoff, egr_blkoff, mac_blkoff;
    uint8               ing_acc_type, egr_acc_type, mac_acc_type;
    int                 blk, port, phy_port, i;
    int                 port_num_blktype;
    soc_pbmp_t          all_pbmp;

    soc = SOC_CONTROL(unit);
    port_num_blktype = SOC_DRIVER(unit)->port_num_blktype > 1 ?
        SOC_DRIVER(unit)->port_num_blktype : 1;

    sal_memset(_soc_port_counter_handles[unit], 0, sizeof(_soc_port_counter_handles[unit]));

    ing_reg = INVALIDr;
    egr_reg = INVALIDr;
    mac_reg = INVALIDr;
    gmac_reg = INVALIDr;
    ing_c_cnt = 0;
    egr_c_cnt = 0;
    mac_c_cnt = 0;
    gmac_c_cnt = 0;
    for (ctype = 0; ctype < SOC_CTR_NUM_TYPES; ctype++) {
        if (!SOC_HAS_CTR_TYPE(unit, ctype)) {
            continue;
        }
        for (csize = SOC_CTR_MAP_SIZE(unit, ctype) - 1; csize >= 0; csize--) {
            reg = SOC_CTR_TO_REG(unit, ctype, csize);
            if (SOC_COUNTER_INVALID(unit, reg)) {
                continue;                       /* skip trailing invalids */
            }
            if (SOC_REG_BLOCK_IS(unit, reg, SOC_BLK_IPIPE)) {
                if (ing_c_cnt == 0) {
                    ing_c_cnt = csize + 1;
                }
                ing_reg = reg;
            }
            if (SOC_REG_BLOCK_IS(unit, reg, SOC_BLK_EPIPE)) {
                if (egr_c_cnt == 0) {
                    egr_c_cnt = csize + 1;
                }
                egr_reg = reg;
            }
            if (SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XLPORT) ||
                SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XTPORT) ||
                SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_XWPORT) ||
                SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_CLPORT) ||
                SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_MXQPORT) ||
                SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_PGW_CL)) {
                if (mac_c_cnt == 0) {
                    mac_c_cnt = csize + 1;
                }
                mac_reg = reg;
            } else if (SOC_REG_BLOCK_IN_LIST(unit, reg, SOC_BLK_GPORT)) {
                if (gmac_c_cnt == 0) {
                    gmac_c_cnt = csize + 1;
                }
                gmac_reg = reg;
            }
        }
    }

    egr_c_cnt = (egr_c_cnt) ? (egr_c_cnt - ing_c_cnt) : 0;
    mac_c_cnt = (mac_c_cnt) ? (mac_c_cnt - egr_c_cnt - ing_c_cnt) : 0;
    gmac_c_cnt = (gmac_c_cnt) ? (gmac_c_cnt - egr_c_cnt - ing_c_cnt) : 0;
    soc_counter_mib_offset[unit] = ing_c_cnt + egr_c_cnt;

    assert(SOC_REG_IS_VALID(unit, ing_reg));
    assert(SOC_REG_IS_VALID(unit, egr_reg));
    ing_addr = soc_reg_addr_get(unit, ing_reg, REG_PORT_ANY, 0, &ing_blkoff,
                                &ing_acc_type);
    egr_addr = soc_reg_addr_get(unit, egr_reg, REG_PORT_ANY, 0, &egr_blkoff,
                                &egr_acc_type);

    soc_cm_debug(DK_COUNTER, "ing_c_cnt = %d egr_c_cnt = %d mac_c_cnt = %d gmac_c_cnt = %d\n"
                 "ing_blkoff = %d ing_acc_type = %d ing_addr = %08x \n"
                 "egr_blkoff = %d egr_acc_type = %d egr_addr = %08x \n",
                 ing_c_cnt, egr_c_cnt, mac_c_cnt, gmac_c_cnt,
                 ing_blkoff, ing_acc_type, ing_addr,
                 egr_blkoff, egr_acc_type, egr_addr);

        SOC_PBMP_CLEAR(all_pbmp);
        SOC_PBMP_ASSIGN(all_pbmp, PBMP_ALL(unit));
#if defined(BCM_KATANA2_SUPPORT)
        if (soc_feature(unit, soc_feature_linkphy_coe) &&
            SOC_INFO(unit).linkphy_enabled) {
            SOC_PBMP_REMOVE(all_pbmp, SOC_INFO(unit).linkphy_pp_port_pbm);
        }
        if (soc_feature(unit, soc_feature_subtag_coe) &&
            SOC_INFO(unit).subtag_enabled) {
            SOC_PBMP_REMOVE(all_pbmp, SOC_INFO(unit).subtag_pp_port_pbm);
        }
#endif
        PBMP_ITER(all_pbmp, port) {

        soc_sbusdma_desc_ctrl_t ctrl;
        soc_sbusdma_desc_cfg_t cfg;

        if (soc_feature(unit, soc_feature_logical_port_num)) {
            phy_port = SOC_INFO(unit).port_l2p_mapping[port];
        } else {
            phy_port = port;
        }

        /* IPIPE */
        ctrl.flags = 0;
        ctrl.cfg_count = 1;
        ctrl.buff = soc->counter_buf64+(port*soc->counter_perport);
        ctrl.cb = _soc_sbusdma_port_ctr_cb;
        ctrl.data = INT_TO_PTR(phy_port);
#define _IP_COUNTERS "IP COUNTERS"
        sal_strncpy(ctrl.name, _IP_COUNTERS, sizeof(_IP_COUNTERS));
        cfg.acc_type = ing_acc_type;
        cfg.blk = ing_blkoff;
        cfg.addr = ing_addr + port;
        cfg.width = 1;
        cfg.count = ing_c_cnt;
        cfg.addr_shift = 8;
        SOC_IF_ERROR_RETURN
            (soc_sbusdma_desc_create(unit, &ctrl, &cfg, 
                                     &_soc_port_counter_handles[unit][phy_port][0]));
        soc_cm_debug(DK_COUNTER, "port %d phy_port %d handle ip: %d\n", 
                     port, phy_port, _soc_port_counter_handles[unit][phy_port][0]);
        
        /* EPIPE */
        ctrl.buff = ((uint32*)ctrl.buff)+ing_c_cnt;
#define _EP_COUNTERS "EP COUNTERS"
        sal_strncpy(ctrl.name, _EP_COUNTERS, sizeof(_EP_COUNTERS));
        cfg.acc_type = egr_acc_type;
        cfg.blk = egr_blkoff;
        cfg.addr = egr_addr + port;
        cfg.count = egr_c_cnt;
        cfg.addr_shift = 8;
        SOC_IF_ERROR_RETURN
            (soc_sbusdma_desc_create(unit, &ctrl, &cfg, 
                                     &_soc_port_counter_handles[unit][phy_port][1]));
        soc_cm_debug(DK_COUNTER, "port %d phy_port %d handle ep: %d\n", 
                     port, phy_port, _soc_port_counter_handles[unit][phy_port][1]);

        /* MAC */
        for (i = 0; i < port_num_blktype; i++) {
            blk = SOC_PORT_IDX_BLOCK(unit, phy_port, i);
            if (blk < 0) {
                continue;
            }
            assert(SOC_REG_IS_VALID(unit, mac_reg) || SOC_REG_IS_VALID(unit, mac_reg));

            if (SOC_REG_IS_VALID(unit, mac_reg)) {
                if (SOC_BLOCK_IN_LIST(SOC_REG_INFO(unit, mac_reg).block,
                                      SOC_BLOCK_TYPE(unit, blk))) {
                    break;
                }
            }
            if (SOC_REG_IS_VALID(unit, gmac_reg)) {
                if (SOC_BLOCK_IN_LIST(SOC_REG_INFO(unit, gmac_reg).block,
                                      SOC_BLOCK_TYPE(unit, blk))) {
                    break;
                }
            }
        }
        if (i == port_num_blktype) {
            continue;
        }

        mac_blkoff = 0;
        mac_addr = 0;
        mac_acc_type = 0;

        if (SOC_BLOCK_IS_CMP(unit, SOC_PORT_BLOCK(unit, phy_port), SOC_BLK_GPORT) ) {
            mac_addr = soc_reg_addr_get(unit, gmac_reg, port, 0, &mac_blkoff,
                                        &mac_acc_type);
            cfg.count = gmac_c_cnt;
            cfg.width = 1;
        } else {
            mac_addr = soc_reg_addr_get(unit, mac_reg, port, 0, &mac_blkoff,
                                        &mac_acc_type);
            cfg.count = mac_c_cnt;
            cfg.width = 2;
        }

        soc_cm_debug(DK_COUNTER,
                     "port %d mac_blkoff = %d, mac_addr = %08x\n", 
                     port, mac_blkoff, mac_addr);

        /* MAC or port group */
        ctrl.buff = ((uint32*)ctrl.buff)+egr_c_cnt;
#define _MIBS "MIBS"
        sal_strncpy(ctrl.name, _MIBS, sizeof(_MIBS));
        cfg.acc_type = mac_acc_type;
        cfg.blk = mac_blkoff;
        cfg.addr = mac_addr;
        cfg.addr_shift = 8;
        SOC_IF_ERROR_RETURN
            (soc_sbusdma_desc_create(unit, &ctrl, &cfg, 
                                     &_soc_port_counter_handles[unit][phy_port][2]));
        soc_cm_debug(DK_COUNTER, "port %d phy_port %d handle mib: %d\n",
                     port, phy_port, _soc_port_counter_handles[unit][phy_port][2]);
    }
    
    if (soc->blk_ctr_desc_count) {
        int rc;
        uint16 cnt, bindex;
        soc_sbusdma_desc_ctrl_t ctrl = {0};
        soc_sbusdma_desc_cfg_t *cfg;
        uint64 *buff, *hwval, *swval;
        sal_memset(_soc_blk_counter_handles[unit], 0, 
                   soc->blk_ctr_desc_count * sizeof(sbusdma_desc_handle_t));
        _blk_ctr_process[unit] = sal_alloc(soc->blk_ctr_desc_count * 
                                     sizeof(soc_blk_ctr_process_t*), 
                                     "blk_ctr_process_ptr");
        buff = soc->blk_ctr_buf;
        hwval = soc->blk_counter_hw_val;
        swval = soc->blk_counter_sw_val;
        for (bindex = 0; bindex < soc->blk_ctr_desc_count; bindex++) {
            cnt = 0;
            while (soc->blk_ctr_desc[bindex].desc[cnt].reg != INVALIDr) {
                cnt++;
            }
            if (cnt) {
                uint8 at;
                uint16 i;
                int ctr_blk;
                soc_blk_ctr_process_t *ctr_process;
                cfg = sal_alloc(cnt * sizeof(soc_sbusdma_desc_cfg_t), 
                      "sbusdma_desc_cfg");
                if (cfg == NULL) {
                    return SOC_E_MEMORY;
                }
                ctrl.flags = 0;
                ctrl.cfg_count = cnt;
                ctrl.buff = buff;
                ctrl.cb = _soc_sbusdma_blk_ctr_cb;
#define _BLK_CTRS "BLK CTRS"
                sal_strncpy(ctrl.name, _BLK_CTRS, sizeof(_BLK_CTRS));
                ctr_process = sal_alloc(sizeof(soc_blk_ctr_process_t), "blk_ctr_process");
                if (cfg == NULL) {
                    sal_free(cfg);
                    return SOC_E_MEMORY;
                }
                ctr_process->blk = soc->blk_ctr_desc[bindex].blk;
                ctr_process->bindex = bindex;
                ctr_process->entries = cnt;
                ctr_process->buff = buff;
                ctr_process->hwval = hwval;
                ctr_process->swval = swval;
                ctrl.data = INT_TO_PTR((uint32)bindex);
                _blk_ctr_process[unit][bindex] = ctr_process;
                for (i=0; i<cnt; i++) {
                    cfg[i].addr = soc_reg_addr_get(unit, 
                                                   soc->blk_ctr_desc[bindex].desc[i].reg,
                                                   REG_PORT_ANY, 0, &ctr_blk, &at);
                    cfg[i].blk = ctr_blk;
                    cfg[i].width = soc->blk_ctr_desc[bindex].desc[i].width;
                    cfg[i].count = soc->blk_ctr_desc[bindex].desc[i].entries;
                    cfg[i].addr_shift = soc->blk_ctr_desc[bindex].desc[i].shift;
                }
                rc = soc_sbusdma_desc_create(unit, &ctrl, cfg, 
                                             &_soc_blk_counter_handles[unit][bindex]);
                sal_free(cfg);
                if (rc != SOC_E_NONE) {
                    soc_cm_debug(DK_ERR, "Desc creation error for handle blk: %d\n",
                                 _soc_blk_counter_handles[unit][bindex]);
                    return rc;
                }
                soc_cm_debug(DK_COUNTER, "handle blk: %d\n", 
                             _soc_blk_counter_handles[unit][bindex]);
            }
            buff += cnt; hwval += cnt; swval += cnt;
        }
    }
    return SOC_E_NONE;
}

STATIC int
_soc_counter_sbudma_desc_free_all(int unit)
{
    soc_control_t *soc = SOC_CONTROL(unit);
    uint8 i, state;
    int ret, err = 0;
    soc_port_t port;
    int blk, bindex, phy_port;
    soc_pbmp_t all_pbmp;

    SOC_PBMP_CLEAR(all_pbmp);
    SOC_PBMP_ASSIGN(all_pbmp, PBMP_ALL(unit));
#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) &&
        SOC_INFO(unit).linkphy_enabled) {
        SOC_PBMP_REMOVE(all_pbmp, SOC_INFO(unit).linkphy_pp_port_pbm);
    }
    if (soc_feature(unit, soc_feature_subtag_coe) &&
        SOC_INFO(unit).subtag_enabled) {
        SOC_PBMP_REMOVE(all_pbmp, SOC_INFO(unit).subtag_pp_port_pbm);
    }
#endif
    PBMP_ITER(all_pbmp, port) {
        if (soc_feature(unit, soc_feature_logical_port_num)) {
            phy_port = SOC_INFO(unit).port_l2p_mapping[port];
        } else {
            phy_port = port;
        }
        blk = SOC_PORT_BLOCK(unit, phy_port);
        bindex = SOC_PORT_BINDEX(unit, phy_port);
        if (_soc_port_counter_handles[unit][phy_port][0] && soc->counter_interval) {
            soc_cm_debug(DK_COUNTER,"port: %d blk: %d, bindex: %d\n", 
                         port, blk, bindex);
        }
        for (i=0; i<=2; i++) {
            if ((i == 2) && 
                (!SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_XLPORT) && 
                 !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_XTPORT) &&
                 !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_XWPORT) &&
                 !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_MXQPORT) &&
                 !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_GPORT) &&
                 !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_CLPORT))) {
                continue;
            }
            if (_soc_port_counter_handles[unit][phy_port][i]) {
                do {
                    (void)soc_sbusdma_desc_get_state(unit, _soc_port_counter_handles[unit][phy_port][i],
                                                     &state);
                    if (state) {
                        sal_usleep(SAL_BOOT_QUICKTURN ? 10000 : 10);
                    }
                } while (state);
                ret = soc_sbusdma_desc_delete(unit, _soc_port_counter_handles[unit][phy_port][i]);
                if (ret) {
                    err++;
                }
                _soc_port_counter_handles[unit][phy_port][i] = 0;
            }
        }  
    }
    if (soc->blk_ctr_desc_count && _soc_blk_counter_handles[unit]) {
        uint16 bindex;
        for (bindex = 0; bindex < soc->blk_ctr_desc_count; bindex++) {
            if (_soc_blk_counter_handles[unit][bindex]) {
                do {
                    (void)soc_sbusdma_desc_get_state(unit, _soc_blk_counter_handles[unit][bindex],
                                                     &state);
                    if (state) {
                        sal_usleep(SAL_BOOT_QUICKTURN ? 10000 : 10);
                    }
                } while (state);
                ret = soc_sbusdma_desc_delete(unit, _soc_blk_counter_handles[unit][bindex]);
                if (ret) {
                    err++;
                }
                _soc_blk_counter_handles[unit][bindex] = 0;
                if (_blk_ctr_process[unit][bindex]) {
                    sal_free(_blk_ctr_process[unit][bindex]);
                    _blk_ctr_process[unit][bindex] = NULL;
                }
            }
        }
        if (_blk_ctr_process[unit]) {
            sal_free(_blk_ctr_process[unit]);
            _blk_ctr_process[unit] = NULL;
        }
    }
    return err;
}

int 
soc_blk_counter_get(int unit, soc_block_t blk, soc_reg_t ctr_reg, int ar_idx,
                    uint64 *val)
{
    uint16 bindex, rindex;
    soc_blk_ctr_process_t *ctr_process;
    soc_ctr_reg_desc_t *reg_desc;
    soc_control_t *soc = SOC_CONTROL(unit);

    if (soc->blk_ctr_desc_count && _blk_ctr_process[unit]) {
        for (bindex = 0; bindex < soc->blk_ctr_desc_count; bindex++) {
            if (_blk_ctr_process[unit][bindex]) {
                ctr_process = _blk_ctr_process[unit][bindex];
                if (ctr_process->blk == blk) {
                    reg_desc = soc->blk_ctr_desc[ctr_process->bindex].desc;
                    for (rindex = 0; reg_desc[rindex].reg != INVALIDr; 
                         rindex++) {
                        if (reg_desc[rindex].reg == ctr_reg) {
                            if (ar_idx < reg_desc[rindex].entries) {
                                *val = ctr_process->swval[ar_idx];
                                return SOC_E_NONE;
                            } else {
                                return SOC_E_PARAM;
                            }
                        }
                    }
                    return SOC_E_PARAM;
                }
            }
        }
        return SOC_E_PARAM;
    } 
    return SOC_E_INIT;
}

#endif /* BCM_SBUSDMA_SUPPORT */
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_XGS_SUPPORT
/*
 * Function:
 *      _soc_counter_dma_setup
 * Purpose:
 *      Configure hardware registers for counter collection.  Used during
 *      soc_counter_thread initialization.
 * Parameters:
 *      unit    - switch unit
 * Returns:
 *      SOC_E_*
 */

static int
_soc_counter_dma_setup(int unit)
{
    soc_control_t       *soc;
    uint32              creg_first, creg_last, cireg_first, cireg_last;
    int                 csize;
    pbmp_t              pbmp;
    soc_reg_t           reg;
    soc_ctr_type_t      ctype;
    uint32              val, offset;

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
#ifdef BCM_SBUSDMA_SUPPORT
        if (soc_feature(unit, soc_feature_sbusdma)) {
            int rv = _soc_counter_sbusdma_setup(unit);
            if (rv) {
                (void)_soc_counter_sbudma_desc_free_all(unit);
            }
            return rv;
        } else 
#endif
        {
            return(_soc_xgs3_counter_dma_setup(unit));
        }
    }
#endif

    soc = SOC_CONTROL(unit);
    SOC_PBMP_ASSIGN(pbmp, soc->counter_pbmp);

    WRITE_CMIC_STAT_DMA_ADDRr(unit, soc_cm_l2p(unit, soc->counter_buf32));
    WRITE_CMIC_STAT_DMA_PORTSr(unit, SOC_PBMP_WORD_GET(pbmp, 0));

    /*
     * Calculate the first and last register offsets for collection
     * Higig offsets are kept separately
     */
    creg_first = creg_last = cireg_first = cireg_last = 0;
    for (ctype = 0; ctype < SOC_CTR_NUM_TYPES; ctype++) {
        if (!SOC_HAS_CTR_TYPE(unit, ctype)) {
            continue;
        }
        if (ctype == SOC_CTR_TYPE_GFE) {        /* offsets screwed up */
            continue;
        }
        reg = SOC_CTR_TO_REG(unit, ctype, 0);
        if (!SOC_COUNTER_INVALID(unit, reg)) {
            if (ctype == SOC_CTR_TYPE_HG) {
                cireg_first = SOC_REG_INFO(unit, reg).offset;
            } else {
                creg_first = SOC_REG_INFO(unit, reg).offset;
                if (cireg_first == 0) {
                    cireg_first = creg_first;
                }
            }
        }
        csize = SOC_CTR_MAP_SIZE(unit, ctype) - 1;
        for (; csize > 0; csize--) {
            reg = SOC_CTR_TO_REG(unit, ctype, csize);
            if (SOC_COUNTER_INVALID(unit, reg)) {
                continue;                       /* skip trailing invalids */
            }
            offset = SOC_REG_INFO(unit, reg).offset;
            csize = SOC_REG_IS_64(unit, reg) ?
                sizeof(uint64) : sizeof(uint32);
            csize = (soc->counter_portsize / csize) - 1;
            if (ctype == SOC_CTR_TYPE_HG) {
                if (offset > cireg_last) {
                    cireg_last = offset;
                    if (cireg_last > cireg_first + csize) {
                        cireg_last = cireg_first + csize;
                    }
                }
            } else {
                if (offset > creg_last) {
                    creg_last = offset;
                    if (creg_last > creg_first + csize) {
                        creg_last = creg_first + csize;
                    }
                }
            }
            break;
        }
    }

    /*
     * Set the various configuration registers
     */
    if (SOC_IS_HERCULES15(unit)) {
        val = 0;
        creg_first = cireg_first;
        creg_last = cireg_last;
        soc_reg_field_set(unit, CMIC_64BIT_STATS_CFGr, &val,
                          L_STAT_REGf, creg_last);
        soc_reg_field_set(unit, CMIC_64BIT_STATS_CFGr, &val,
                          F_STAT_REGf, creg_first);
        WRITE_CMIC_64BIT_STATS_CFGr(unit, val);
    } else {
        val = 0;
        soc_reg_field_set(unit, CMIC_STAT_DMA_SETUPr, &val,
                          L_STAT_REGf, creg_last);
        soc_reg_field_set(unit, CMIC_STAT_DMA_SETUPr, &val,
                          F_STAT_REGf, creg_first);
        WRITE_CMIC_STAT_DMA_SETUPr(unit, val);
    }

    WRITE_CMIC_DMA_STATr(unit, DS_STAT_DMA_ITER_DONE_CLR);


#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_cmicm_intr0_enable(unit, IRQ_CMCx_STAT_ITER_DONE);
        } else
#endif
        {
            soc_intr_enable(unit, IRQ_STAT_ITER_DONE);
        }

    return SOC_E_NONE;
}
#endif /*BCM_XGS_SUPPORT*/
/*
 * Function:
 *      soc_counter_thread
 * Purpose:
 *      Master counter collection and accumulation thread.
 * Parameters:
 *      unit_vp - StrataSwitch unit # (as a void *).
 * Returns:
 *      Nothing, does not return.
 */

void
soc_counter_thread(void *unit_vp)
{
    int                 unit = PTR_TO_INT(unit_vp);
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 rv = SOC_E_NONE;
    int                 interval;
#if defined(WRITE_CMIC_DMA_STATr) || ( defined(BCM_CMICM_SUPPORT) && (defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)) )
    uint32              val;
#endif
    int                 dma;
    sal_usecs_t         cdma_timeout, now;
    COUNTER_ATOMIC_DEF  s;
    int                 sync_gnt = FALSE;
    int                 i;
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)                   
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
#endif
#endif 
#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    static int          _stat_err[SOC_MAX_NUM_DEVICES] = {0};
#endif
    soc_cm_debug(DK_COUNTER, "soc_counter_thread: unit=%d\n", unit);

    /*
     * Create a semaphore used to time the trigger scans, and if DMA is
     * used, monitor for the Stats DMA Iteration Done interrupt.
     */
    cdma_timeout = soc_property_get(unit, spn_CDMA_TIMEOUT_USEC, 1000000);

    dma = ((soc->counter_flags & SOC_COUNTER_F_DMA) != 0);

    if (dma) {
#ifdef BCM_XGS_SUPPORT
        rv = _soc_counter_dma_setup(unit);
#endif /* BCM_XGS_SUPPORT */
        if (rv < 0) {
            goto done;
        }
    }

    /*
     * The hardware timer can only be used for intervals up to about
     * 1.048 seconds.  This implementation uses a software timer (via
     * semaphore timeout) instead of the hardware timer.
     */

    while ((interval = soc->counter_interval) != 0) {
#ifdef COUNTER_BENCH
        sal_usecs_t     start_time;
#endif
        int             err = 0;

        /*
         * Use a semaphore timeout instead of a sleep to achieve the
         * desired delay between scans.  This allows this thread to exit
         * immediately when soc_counter_stop wants it to.
         */

        soc_cm_debug(DK_COUNTER + DK_VERBOSE,
                     "soc_counter_thread: sleep %d\n", interval);

        (void)sal_sem_take(soc->counter_trigger, interval);

        if (soc->counter_interval == 0) {       /* Exit signaled */
            break;
        }

        if (soc->counter_sync_req) {
            sync_gnt = TRUE;
        }

#ifdef COUNTER_BENCH
        start_time = sal_time_usecs();
#endif

        /*
         * If in DMA mode, use DMA to transfer the counters into
         * memory.  Start a DMA pass by enabling DMA and clearing
         * STAT_DMA_DONE bit.  Wait for the pass to finish.
         */
        COUNTER_LOCK(unit);
#ifdef BCM_SBUSDMA_SUPPORT
        if (dma && soc_feature(unit, soc_feature_sbusdma)) {
            uint8 i; 
            int ret;
            soc_port_t port;
            int blk, bindex, phy_port;
            
            PBMP_ITER(soc->counter_pbmp, port) {
                if (soc_feature(unit, soc_feature_logical_port_num)) {
                    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
                } else {
                    phy_port = port;
                }
                blk = SOC_PORT_BLOCK(unit, phy_port);
                bindex = SOC_PORT_BINDEX(unit, phy_port);
                if (_soc_port_counter_handles[unit][phy_port][0] && soc->counter_interval) {
                    soc_cm_debug(DK_COUNTER,"port: %d blk: %d, bindex: %d\n", 
                                 port, blk, bindex);
                }
                for (i=0; i<=2; i++) {
                    if ((i == 2) && 
                        (!SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_XLPORT) && 
                         !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_XTPORT) &&
                         !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_XWPORT) &&
                         !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_MXQPORT) &&
                         !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_GPORT) &&
                         !SOC_BLOCK_IS_CMP(unit, blk, SOC_BLK_CLPORT))) {
                        continue;
                    }
                    if (_soc_port_counter_handles[unit][phy_port][i] && soc->counter_interval) {
                        do {
                            ret = soc_sbusdma_desc_run(unit, _soc_port_counter_handles[unit][phy_port][i]);
                            if ((ret == SOC_E_BUSY) || (ret == SOC_E_INIT)) {
                                sal_usleep(SAL_BOOT_QUICKTURN ? 10000 : 10);
                            } else {
                                SOC_SBUSDMA_DM_LOCK(unit);
                                _soc_counter_pending[unit]++;
                                SOC_SBUSDMA_DM_UNLOCK(unit);
                            }
                        } while(((ret == SOC_E_BUSY) || (ret == SOC_E_INIT)) && soc->counter_interval);
                    } else { 
                        if (soc->counter_interval == 0) {   /* Exit signaled */
                            break;
                        }
                    }
                }  
            }
            
            if (soc->blk_ctr_desc_count && soc->counter_interval) {
                for (i=0; i<soc->blk_ctr_desc_count; i++) {
                    soc_cm_debug(DK_COUNTER,"blk ctr %d\n", i);
                    if (_soc_blk_counter_handles[unit][i] && soc->counter_interval) {
                        do {
                            ret = soc_sbusdma_desc_run(unit, _soc_blk_counter_handles[unit][i]);
                            if ((ret == SOC_E_BUSY) || (ret == SOC_E_INIT)) {
                                sal_usleep(SAL_BOOT_QUICKTURN ? 10000 : 10);
                            }
                        } while(((ret == SOC_E_BUSY) || (ret == SOC_E_INIT)) && soc->counter_interval);
                    } else { 
                        if (soc->counter_interval == 0) {   /* Exit signaled */
                            break;
                        }
                    }
                }
            }
            if (soc->counter_interval == 0) {   /* Exit signaled */
                COUNTER_UNLOCK(unit);
                break;
            }
        }
#endif
        if (dma && !soc_feature(unit, soc_feature_sbusdma)) {
            soc_cm_debug(DK_COUNTER + DK_VERBOSE,
                         "soc_counter_thread: trigger DMA\n");

#ifdef BCM_CMICM_SUPPORT
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) 

            if(soc_feature(unit, soc_feature_cmicm)) {
                /* Clear Status */
                val = soc_pci_read(unit, CMIC_CMCx_STAT_DMA_CFG_OFFSET(cmc));
                soc_reg_field_set(unit, CMIC_CMC0_STAT_DMA_CFGr, &val,
                                  ENf, 0);
                soc_reg_field_set(unit, CMIC_CMC0_STAT_DMA_CFGr, &val,
                                  ST_DMA_ITER_DONE_CLRf, 0);
                soc_pci_write(unit, CMIC_CMCx_STAT_DMA_CFG_OFFSET(cmc), val);
                /* start DMA */
                soc_reg_field_set(unit, CMIC_CMC0_STAT_DMA_CFGr, &val,
                                  ENf, 1);
                soc_pci_write(unit, CMIC_CMCx_STAT_DMA_CFG_OFFSET(cmc), val);
            } else 
#endif            
#endif
            {
                /* Clear Status and start DMA */
#ifdef WRITE_CMIC_DMA_STATr
                WRITE_CMIC_DMA_STATr(unit, DS_STAT_DMA_DONE_CLR);
                READ_CMIC_STAT_DMA_SETUPr(unit, &val);
                soc_reg_field_set(unit, CMIC_STAT_DMA_SETUPr, &val,
                                  ENf, 1);
                WRITE_CMIC_STAT_DMA_SETUPr(unit, val);
#endif
            }
            /* Wait for ISR to wake semaphore */
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                if (sal_sem_take(soc->counter_intr, cdma_timeout) >= 0) {
                    soc_cm_sinval(unit,
                                  (void *)soc->counter_buf32,
                                  soc->counter_bufsize);

                    soc_cm_debug(DK_COUNTER + DK_VERBOSE,
                                 "soc_counter_thread: "
                                 "DMA iter done\n");

#ifdef COUNTER_BENCH
                    soc_cm_debug(DK_VERBOSE,
                                 "Time taken for dma: %d usec\n",
                                 SAL_USECS_SUB(sal_time_usecs(), start_time));
#endif
                } else {
                    soc_cm_debug(DK_ERR,
                                 "soc_counter_thread: "
                                 "DMA did not finish buf32=%p\n",
                                 (void *)soc->counter_buf32);
                    err = 1;
                }
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) 
                
                if (soc_pci_read(unit, CMIC_CMCx_STAT_DMA_STAT_OFFSET(cmc)) & ST_CMCx_DMA_ERR) {
                    soc_cm_debug(DK_ERR,
                                 "soc_counter_thread: unit = %d DMA Error\n",
                                 unit);
                    if (soc_ser_stat_error(unit, -1) != SOC_E_NONE) {
                        _stat_err[unit]++;
                        /* Error is reported atleast one more time after correction.
                           So try few times and abort if correction really fails */
                        if (_stat_err[unit] > 10) {
                            err = 1;
                        }
                    } else {
                        _stat_err[unit] = 0;
                    }
                }
#endif                
            } else
#endif /* CMICm Support */
            {
                if (sal_sem_take(soc->counter_intr, cdma_timeout) >= 0) {
                    soc_cm_sinval(unit,
                                  (void *)soc->counter_buf32,
                                  soc->counter_bufsize);

                    soc_cm_debug(DK_COUNTER + DK_VERBOSE,
                                 "soc_counter_thread: "
                                 "DMA iter done\n");

#ifdef COUNTER_BENCH
                    soc_cm_debug(DK_VERBOSE,
                                 "Time taken for dma: %d usec\n",
                                 SAL_USECS_SUB(sal_time_usecs(), start_time));
#endif
                } else {
                    soc_cm_debug(DK_ERR,
                                 "soc_counter_thread: "
                                 "DMA did not finish buf32=%p\n",
                                 (void *)soc->counter_buf32);
                    err = 1;
                }
#ifdef BCM_XGS3_SWITCH_SUPPORT
                if (SOC_IS_XGS3_SWITCH(unit)) {
                    if (soc_pci_read(unit, CMIC_DMA_STAT) & DS_STAT_DMA_ERROR) {
                        if (soc_feature(unit, soc_feature_stat_dma_error_ack)) {
                            WRITE_CMIC_DMA_STATr(unit, DS_STAT_DMA_ERROR_CLR - 1);
                        } else {
                            WRITE_CMIC_DMA_STATr(unit, DS_STAT_DMA_ERROR_CLR);
                        }
                        soc_cm_debug(DK_ERR,
                                     "soc_counter_thread: unit = %d DMA Error\n",
                                     unit);
                        if (soc_ser_stat_error(unit, -1) != SOC_E_NONE) {
                            _stat_err[unit]++;
                            /* Error is reported atleast one more time after correction.
                               So try few times and abort if correction really fails */
                            if (_stat_err[unit] > 10) {
                                err = 1;
                            }
                        } else {
                            _stat_err[unit] = 0;
                        }
                    }
                }
#endif
            }
            if (soc->counter_interval == 0) {   /* Exit signaled */
                COUNTER_UNLOCK(unit);
                break;
            }
        }

        /*
         * Add up changes to counter values.
         */

#ifdef BCM_SBUSDMA_SUPPORT
        while (_soc_counter_pending[unit] && soc->counter_interval) {
            sal_usleep(SAL_BOOT_QUICKTURN ? 10000 : 10);
        }
#endif
        
        now = sal_time_usecs();
        COUNTER_ATOMIC_BEGIN(s);
        soc->counter_coll_prev = soc->counter_coll_cur;
        soc->counter_coll_cur = now;
        COUNTER_ATOMIC_END(s);

        if ( (!err) && (soc->counter_n32 > 0) && (soc->counter_interval) ) {
            rv = soc_counter_collect32(unit, FALSE);
            if (rv < 0) {
                soc_cm_debug(DK_ERR,
                             "soc_counter_thread: collect32 failed: %s\n",
                             soc_errmsg(rv));
                err = 1;
            }
        }

        if ((!err) && (soc->counter_n64 > 0) && (soc->counter_interval) ) {
            if(SOC_IS_SAND(unit)) {
                if (SOC_IS_PETRAB(unit)) {
#if defined(BCM_PETRAB_SUPPORT)
                    rv = soc_petra_counter_collect64(unit, FALSE);
#endif 
                } else if (SOC_IS_ARAD(unit)) {
#ifdef BCM_ARAD_SUPPORT
                    SOC_DPP_ALLOW_WARMBOOT_WRITE_NO_ERR(soc_counter_collect64(unit, FALSE, -1, INVALIDr), rv);
#endif
                } else {
                    rv = SOC_E_NONE;
                }
            } else {
#ifdef BCM_XGS_SUPPORT
                rv = soc_counter_collect64(unit, FALSE, -1, INVALIDr);
#endif
            }

            if (rv < 0) {
                soc_cm_debug(DK_ERR, "soc_counter_thread: collect64 failed: "
                             "%s\n", soc_errmsg(rv));
                err = 1;
            }
        }

        if ((soc->counter_interval)) {
            soc_controlled_counters_collect64(unit, FALSE);
            if (rv < 0) {
                soc_cm_debug(DK_ERR, "soc_counter_thread: soc_controlled_counters_collect64 failed: "
                             "%s\n", soc_errmsg(rv));
                err = 1;
            }
        }

        /*
         * Check optional non CMIC counter DMA support entries
         * These counters are included in "show counter" output
         */
        if ((soc->counter_interval)) {
            soc_counter_collect_non_dma_entries(unit);
        }
        
    COUNTER_UNLOCK(unit);

        /*
         * Callback for additional work
         * These counters are not included in "show counter" output
         */
        for (i = 0; i < SOC_COUNTER_EXTRA_CB_MAX; i++) {
            if (soc_counter_extra[unit][i] != NULL) {
                soc_counter_extra[unit][i](unit);
            }
        }

        /*
         * Forgive spurious errors
         */

        if (err) {
                soc_cm_debug(DK_ERR, "soc_counter_thread: Too many errors\n");
                rv = SOC_E_INTERNAL;
                goto done;
            }

#ifdef COUNTER_BENCH
        soc_cm_debug(DK_VERBOSE,
                     "Iteration time: %d usec\n",
                     SAL_USECS_SUB(sal_time_usecs(), start_time));
#endif

        if (sync_gnt) {
            soc->counter_sync_req = 0;
            sync_gnt = 0;
        }
    }

    rv = SOC_E_NONE;

 done:
    if (rv < 0) {
        soc_cm_debug(DK_ERR,
                     "soc_counter_thread: Operation failed; exiting\n");
        soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                           SOC_SWITCH_EVENT_THREAD_COUNTER, __LINE__, rv);
    }

    soc_cm_debug(DK_COUNTER, "soc_counter_thread: exiting\n");

    soc->counter_pid = SAL_THREAD_ERROR;
    soc->counter_interval = 0;

    sal_thread_exit(0);
}

/*
 * Function:
 *      soc_counter_start
 * Purpose:
 *      Start the counter collection, S/W accumulation process.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      flags - SOC_COUNTER_F_xxx flags.
 *      interval - collection period in micro-seconds.
 *              Using 0 is the same as calling soc_counter_stop().
 *      pbmp - bit map of ports to collact counters on.
 * Returns:
 *      SOC_E_XXX
 */

int
soc_counter_start(int unit, uint32 flags, int interval, pbmp_t pbmp)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    char                pfmt[SOC_PBMP_FMT_LEN];
    sal_sem_t           sem;
    int         rv = SOC_E_NONE;
    soc_port_t          p;

    soc_cm_debug(DK_COUNTER,
                 "soc_counter_start: unit=%d flags=0x%x "
                 "interval=%d pbmp=%s\n",
                 unit, flags, interval, SOC_PBMP_FMT(pbmp, pfmt));

    /* Stop if already running */

    if (soc->counter_interval != 0) {
        SOC_IF_ERROR_RETURN(soc_counter_stop(unit));
    }

    if (interval == 0) {
        return SOC_E_NONE;
    }

    /* Create fresh semaphores */

    if ((sem = soc->counter_trigger) != NULL) {
        soc->counter_trigger = NULL;    /* Stop others from waking sem */
        sal_sem_destroy(sem);           /* Then destroy it */
    }

    soc->counter_trigger =
        sal_sem_create("counter_trigger", sal_sem_BINARY, 0);

    if ((sem = soc->counter_intr) != NULL) {
        soc->counter_intr = NULL;       /* Stop intr from waking sem */
        sal_sem_destroy(sem);           /* Then destroy it */
    }

    soc->counter_intr =
        sal_sem_create("counter_intr", sal_sem_BINARY, 0);

    if (soc->counter_trigger == NULL || soc->counter_intr == NULL) {
        soc_cm_debug(DK_ERR, "soc_counter_start: sem create failed\n");
        return SOC_E_INTERNAL;
    }

    sal_snprintf(soc->counter_name,
                 sizeof(soc->counter_name),
                 "bcmCNTR.%d", unit);

    SOC_PBMP_ASSIGN(soc->counter_pbmp, pbmp);
    PBMP_ITER(soc->counter_pbmp, p) {
        if ((SOC_PBMP_MEMBER(SOC_PORT_DISABLED_BITMAP(unit,all), p))) {
            SOC_PBMP_PORT_REMOVE(soc->counter_pbmp, p);
        }
        if (IS_LB_PORT(unit, p)) {
            SOC_PBMP_PORT_REMOVE(soc->counter_pbmp, p);
        }
    }
    soc->counter_flags = flags;

    soc->counter_flags &= ~SOC_COUNTER_F_SWAP64; /* HW takes care of this */

    if (!soc_feature(unit, soc_feature_stat_dma) || SOC_IS_RCPU_ONLY(unit)) {
        soc->counter_flags &= ~SOC_COUNTER_F_DMA;
    }

    /*
     * The HOLD register is not supported by DMA on HUMV/Bradley, 
     * but in order to allow certain test scripts to pass, we 
     * optionally collect this register manually.
     */

#ifdef BCM_BRADLEY_SUPPORT
    soc->counter_flags &= ~SOC_COUNTER_F_HOLD;

    if (SOC_IS_HBX(unit)) {
        if (soc_property_get(unit, spn_CDMA_PIO_HOLD_ENABLE, 1)) {
            soc->counter_flags |= SOC_COUNTER_F_HOLD;
        }
    }
#endif /* BCM_BRADLEY_SUPPORT */

#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRX_SUPPORT)
    if (soc_feature(unit, soc_feature_bigmac_rxcnt_bug)) {
        /* Allocate buffer for DMA counter validation */
        soc_counter_tbuf[unit] = sal_alloc(SOC_COUNTER_TBUF_SIZE(unit), 
                                           "counter_tbuf");
        if (soc_counter_tbuf[unit] == NULL) {
            soc_cm_debug(DK_ERR,
                         "soc_counter_thread: unit %d: "
                         "failed to allocate temp counter buffer\n",
                         unit);
        }
    }
#endif /* BCM_BRADLEY_SUPPORT || BCM_TRX_SUPPORT */

    SOC_IF_ERROR_RETURN(soc_counter_autoz(unit, 0));

    /* Synchronize counter 'prev' values with current hardware counters */

    soc->counter_coll_prev = soc->counter_coll_cur = sal_time_usecs();

    if (soc->counter_n32 > 0) {
        COUNTER_LOCK(unit);
        rv = soc_counter_collect32(unit, TRUE);
        COUNTER_UNLOCK(unit);
        SOC_IF_ERROR_RETURN(rv);
    }

    if (soc->counter_n64 > 0) {
        COUNTER_LOCK(unit);
        if(SOC_IS_SAND(unit)) {
            if (SOC_IS_PETRAB(unit)) {
#if defined(BCM_PETRAB_SUPPORT)
                rv = soc_petra_counter_collect64(unit, TRUE);
#endif
            } else {
                rv = SOC_E_NONE;
            }
        } else {
#if defined(BCM_XGS_SUPPORT)
            rv = soc_counter_collect64(unit, TRUE, -1, INVALIDr);
#endif
        }
        COUNTER_UNLOCK(unit);
        SOC_IF_ERROR_RETURN(rv);
    }

    soc_controlled_counters_collect64(unit, TRUE);

    /* Start the thread */

    if (interval != 0) {
        soc->counter_interval = interval;

        soc->counter_pid =
            sal_thread_create(soc->counter_name,
                              SAL_THREAD_STKSZ,
                              soc_property_get(unit,
                                               spn_COUNTER_THREAD_PRI,
                                               50),
                              soc_counter_thread, INT_TO_PTR(unit));

        if (soc->counter_pid == SAL_THREAD_ERROR) {
            soc->counter_interval = 0;
            soc_cm_debug(DK_ERR, "soc_counter_start: thread create failed\n");
            return (SOC_E_INTERNAL);
        }

        soc_cm_debug(DK_COUNTER, "soc_counter_start: complete\n");
    }

    return (SOC_E_NONE);
}

/*
 * Function:
 *      soc_counter_status
 * Purpose:
 *      Get the status of counter collection, S/W accumulation process.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      flags - SOC_COUNTER_F_xxx flags.
 *      interval - collection period in micro-seconds.
 *      pbmp - bit map of ports to collact counters on.
 * Returns:
 *      SOC_E_XXX
 */

int
soc_counter_status(int unit, uint32 *flags, int *interval, pbmp_t *pbmp)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
 
    soc_cm_debug(DK_COUNTER, "soc_counter_status: unit=%d\n", unit);

    *interval = soc->counter_interval;
    *flags = soc->counter_flags;
    SOC_PBMP_ASSIGN(*pbmp, soc->counter_pbmp);

    return (SOC_E_NONE);
}

/*
 * Function:
 *      soc_counter_sync
 * Purpose:
 *      Force an immediate counter update
 * Parameters:
 *      unit - StrataSwitch unit #.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Ensures that ALL counter activity that occurred before the sync
 *      is reflected in the results of any soc_counter_get()-type
 *      routine that is called after the sync.
 */

int
soc_counter_sync(int unit)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    soc_timeout_t       to;
    uint32              stat_sync_timeout;

    if (soc->counter_interval == 0) {
        return SOC_E_DISABLED;
    }

    /* Trigger a collection */

    soc->counter_sync_req = TRUE;

    sal_sem_give(soc->counter_trigger);

    if (SAL_BOOT_QUICKTURN) {
        stat_sync_timeout = STAT_SYNC_TIMEOUT_QT;
    } else if (SAL_BOOT_BCMSIM) {
        stat_sync_timeout = STAT_SYNC_TIMEOUT_BCMSIM;
    } else if (SOC_IS_RCPU_ONLY(unit)) {
        stat_sync_timeout = STAT_SYNC_TIMEOUT*4;
    } else {
        stat_sync_timeout = STAT_SYNC_TIMEOUT;
    }
    stat_sync_timeout = soc_property_get(unit,
                                         spn_BCM_STAT_SYNC_TIMEOUT,
                                         stat_sync_timeout);
    soc_timeout_init(&to, stat_sync_timeout, 0);
    while (soc->counter_sync_req) {
        if (soc_timeout_check(&to)) {
            if (soc->counter_sync_req) {
                soc_cm_debug(DK_ERR,
                             "soc_counter_sync: counter thread not responding\n");
                soc->counter_sync_req = FALSE;
                return SOC_E_TIMEOUT;
            }
        }

        sal_usleep(10000);
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_counter_stop
 * Purpose:
 *      Terminate the counter collection, S/W accumulation process.
 * Parameters:
 *      unit - StrataSwitch unit #.
 * Returns:
 *      SOC_E_XXX
 */

int
soc_counter_stop(int unit)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 rv = SOC_E_NONE;
    soc_timeout_t       to;
    sal_usecs_t         cdma_timeout;
#ifdef BCM_XGS_SUPPORT
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
#endif
#endif

    soc_cm_debug(DK_COUNTER, "soc_counter_stop: unit=%d\n", unit);

    if (SAL_BOOT_QUICKTURN) {
        cdma_timeout = CDMA_TIMEOUT_QT;
    } else if (SAL_BOOT_BCMSIM) {
        cdma_timeout = CDMA_TIMEOUT_BCMSIM;
    } else if (SOC_IS_RCPU_ONLY(unit)) {
        cdma_timeout = CDMA_TIMEOUT*10;
    } else {
        cdma_timeout = CDMA_TIMEOUT;
    }
    cdma_timeout = soc_property_get(unit, spn_CDMA_TIMEOUT_USEC, cdma_timeout);

    /* Stop thread if present. */

    if (soc->counter_interval != 0) {
        sal_thread_t    sample_pid;

        /*
         * Signal by setting interval to 0, and wake up thread to speed
         * its exit.  It may also be waiting for the hardware interrupt
         * semaphore.  Wait a limited amount of time for it to exit.
         */

        soc->counter_interval = 0;

        sal_sem_give(soc->counter_intr);
        sal_sem_give(soc->counter_trigger);

        soc_timeout_init(&to, cdma_timeout, 0);

        while ((sample_pid = soc->counter_pid) != SAL_THREAD_ERROR) {
            if (soc_timeout_check(&to)) {
                soc_cm_debug(DK_ERR,
                             "soc_counter_stop: thread did not exit\n");
                soc->counter_pid = SAL_THREAD_ERROR;
                rv = SOC_E_TIMEOUT;
                break;
            }

            sal_usleep(10000);
        }
    }

#ifdef BCM_XGS_SUPPORT
    if ((soc->counter_flags & SOC_COUNTER_F_DMA) && 
        !soc_feature(unit, soc_feature_sbusdma)) {
        uint32          val;

        /*
         * Disable hardware counter scanning.
         * Turn off all ports to speed up final scan cycle.
         *
         * This cleanup is done here instead of at the end of the
         * counter thread so counter DMA will turn off even if the
         * thread is non responsive.
         */

#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {

            soc_cmicm_intr0_disable(unit, IRQ_CMCx_STAT_ITER_DONE);

            val = soc_pci_read(unit, CMIC_CMCx_STAT_DMA_CFG_OFFSET(cmc));
            soc_reg_field_set(unit, CMIC_CMC0_STAT_DMA_CFGr, &val, ENf, 0);
            soc_reg_field_set(unit, CMIC_CMC0_STAT_DMA_CFGr, &val, E_Tf, 0);
            soc_pci_write(unit, CMIC_CMCx_STAT_DMA_CFG_OFFSET(cmc), val);
            soc_pci_write(unit, CMIC_CMCx_STAT_DMA_PORTS_0_OFFSET(cmc), 0);
            if (SOC_REG_IS_VALID(unit, CMIC_CMC0_STAT_DMA_PORTS_1r)) { 
                soc_pci_write(unit, CMIC_CMCx_STAT_DMA_PORTS_1_OFFSET(cmc), 0);
            }
            if (SOC_REG_IS_VALID(unit, CMIC_CMC0_STAT_DMA_PORTS_2r)) { 
                soc_pci_write(unit, CMIC_CMCx_STAT_DMA_PORTS_2_OFFSET(cmc), 0);
            }
        } else
#endif
        {

            soc_intr_disable(unit, IRQ_STAT_ITER_DONE);

            READ_CMIC_STAT_DMA_SETUPr(unit, &val);
            soc_reg_field_set(unit, CMIC_STAT_DMA_SETUPr, &val, ENf, 0);
            soc_reg_field_set(unit, CMIC_STAT_DMA_SETUPr, &val, E_Tf, 0);
            WRITE_CMIC_STAT_DMA_SETUPr(unit, val);
            WRITE_CMIC_STAT_DMA_PORTSr(unit, 0);
#if defined(BCM_RAPTOR_SUPPORT)
            if (soc_feature(unit, soc_feature_register_hi)) {
                WRITE_CMIC_STAT_DMA_PORTS_HIr(unit, 0);
            }
#endif /* BCM_RAPTOR_SUPPORT */
#if defined(BCM_TRIUMPH_SUPPORT)
            if (SOC_IS_TR_VL(unit)) {
                WRITE_CMIC_STAT_DMA_PORTS_HIr(unit, 0);
            }
#endif /* BCM_TRIUMPH_SUPPORT */
        }

        /*
         * Wait for STAT_DMA_ACTIVE to go away, with a timeout in case
         * it never does.
         */

        soc_cm_debug(DK_COUNTER, "soc_counter_stop: waiting for idle\n");

        soc_timeout_init(&to, cdma_timeout, 0);
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            while (soc_pci_read(unit, CMIC_CMCx_STAT_DMA_STAT_OFFSET(cmc)) & ST_CMCx_DMA_ACTIVE) {
                if (soc_timeout_check(&to)) {
                    rv = SOC_E_INTERNAL;
                    break;
                }
            }
        } else
#endif
        {
            while (soc_pci_read(unit, CMIC_DMA_STAT) & DS_STAT_DMA_ACTIVE) {
                if (soc_timeout_check(&to)) {
                    rv = SOC_E_TIMEOUT;
                    break;
                }
            }
        }
    }

#ifdef BCM_XGS3_SWITCH_SUPPORT
#ifdef BCM_SBUSDMA_SUPPORT
    if ((soc->counter_flags & SOC_COUNTER_F_DMA) && 
        soc_feature(unit, soc_feature_sbusdma)) {
        int err;
        if ((err = _soc_counter_sbudma_desc_free_all(unit)) != 0) {
            soc_cm_debug(DK_ERR, "soc_counter_stop: [%d] Desc free error(s)\n",
                         err);
        }
    } 
#endif
#endif

#ifdef BCM_BRADLEY_SUPPORT
    if (soc_counter_tbuf[unit]) {
        sal_free(soc_counter_tbuf[unit]);
        soc_counter_tbuf[unit] = NULL;
    }
#endif /* BCM_BRADLEY_SUPPORT */
#endif /* BCM_XGS_SUPPORT */

    if (NULL != soc->counter_intr) {
        sal_sem_destroy(soc->counter_intr);
        soc->counter_intr = NULL;
    }
    if (NULL != soc->counter_trigger) {
        sal_sem_destroy(soc->counter_trigger);
        soc->counter_trigger = NULL;
    }

    soc_cm_debug(DK_COUNTER, "soc_counter_stop: stopped\n");

    return (rv);
}

/*
 * Function:
 *      soc_counter_extra_register
 * Purpose:
 *      Register callback for additional counter collection.
 * Parameters:
 *      unit - The SOC unit number
 *      fn   - Callback function.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int 
soc_counter_extra_register(int unit, soc_counter_extra_f fn)
{
    int i;

    if (fn == NULL) {
        return SOC_E_PARAM;
    }

    for (i = 0; i < SOC_COUNTER_EXTRA_CB_MAX; i++) {
        if (soc_counter_extra[unit][i] == fn) {
            return SOC_E_NONE;
        }
    }

    for (i = 0; i < SOC_COUNTER_EXTRA_CB_MAX; i++) {
        if (soc_counter_extra[unit][i] == NULL) {
            soc_counter_extra[unit][i] = fn;
            return SOC_E_NONE;
        }
    }

    return SOC_E_FULL;
}

/*
 * Function:
 *      soc_counter_extra_unregister
 * Purpose:
 *      Unregister callback for additional counter collection.
 * Parameters:
 *      unit - The SOC unit number
 *      fn   - Callback function.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_counter_extra_unregister(int unit, soc_counter_extra_f fn)
{
    int i;

    if (fn == NULL) {
        return SOC_E_PARAM;
    }

    for (i = 0; i < SOC_COUNTER_EXTRA_CB_MAX; i++) {
        if (soc_counter_extra[unit][i] == fn) {
            soc_counter_extra[unit][i] = NULL;
            return SOC_E_NONE;
        }
    }

    return SOC_E_NOT_FOUND;
}

int
soc_counter_timestamp_get(int unit, soc_port_t port,
                          uint32 *timestamp)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    int rv = SOC_E_NOT_FOUND;

    if (!soc_feature(unit, soc_feature_timestamp_counter)) {
        return rv;
    }

    if (soc->counter_timestamp_fifo[port] == NULL) {
        return rv;
    }

    COUNTER_LOCK(unit);
    if (!SHR_FIFO_IS_EMPTY(soc->counter_timestamp_fifo[port])) {
        SHR_FIFO_POP(soc->counter_timestamp_fifo[port], timestamp);
        rv = SOC_E_NONE;
    }
    COUNTER_UNLOCK(unit);
    return rv;
}

#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) */
