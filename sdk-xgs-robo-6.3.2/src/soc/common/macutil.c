/*
 * $Id: macutil.c 1.66 Broadcom SDK $
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
 * MAC driver support utilities. 
 */

#include <sal/core/libc.h>

#include <soc/drv.h>
#include <soc/ll.h>
#include <soc/error.h>
#include <soc/portmode.h>
#include <soc/macutil.h>
#include <soc/phyctrl.h>
#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/trident2.h>
#endif
#include <soc/debug.h>

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)

/*
 * Function:
 *      soc_egress_cell_count
 * Purpose:
 *      Return the approximate number of cells of packets pending
 *      in the MMU destined for a specified egress.
 */
int
soc_egress_cell_count(int unit, soc_port_t port, uint32 *count)
{
    uint32              val;
    int                 cos;
#if defined(BCM_TRIDENT_SUPPORT)
    int                 i;
#if defined(BCM_TRIDENT2_SUPPORT)
    soc_mem_t           mem;
    int                 index;
    uint32              entry[SOC_MAX_MEM_WORDS];
#endif /* BCM_TRIDENT2_SUPPORT */
#endif /* BCM_TRIDENT_SUPPORT */

    *count = 0;

    switch (SOC_CHIP_GROUP(unit)) {
    case SOC_CHIP_BCM56504:
    case SOC_CHIP_BCM56102:
    case SOC_CHIP_BCM56304:
    case SOC_CHIP_BCM56218:
    case SOC_CHIP_BCM56112:
    case SOC_CHIP_BCM56314:
    case SOC_CHIP_BCM56514:
    case SOC_CHIP_BCM56224:
    case SOC_CHIP_BCM53314:
    case SOC_CHIP_BCM56142:
    case SOC_CHIP_BCM56150:
       for (cos = 0; cos < NUM_COS(unit); cos++) {
            SOC_IF_ERROR_RETURN(soc_reg_egress_cell_count_get(unit, port, cos, &val));
            *count += val;
        }
#if defined(BCM_HURRICANE_SUPPORT)
       if (SOC_IS_HURRICANE(unit) &&
          (IS_XE_PORT(unit, port) || IS_HG_PORT(unit, port))) {
           if (SOC_REG_PORT_VALID(unit, XP_XBODE_CELL_CNTr, port)) {
               SOC_IF_ERROR_RETURN(
                       READ_XP_XBODE_CELL_CNTr(unit, port, &val));
               *count += soc_reg_field_get(
                       unit, XP_XBODE_CELL_CNTr, val, CELL_CNTf);
           }
           if (SOC_REG_PORT_VALID(unit, GE_GBODE_CELL_CNTr, port)) {
               SOC_IF_ERROR_RETURN(READ_GE_GBODE_CELL_CNTr(unit, port, &val));
               *count += soc_reg_field_get(
                       unit, GE_GBODE_CELL_CNTr, val, CELL_CNTf);
           }
       }
#endif
        break;
#if defined(BCM_HERCULES_SUPPORT)
    case SOC_CHIP_BCM5675:
        
        break;
#endif /* BCM_HERCULES_SUPPORT */
#if defined(BCM_BRADLEY_SUPPORT)
    case SOC_CHIP_BCM56800:
    case SOC_CHIP_BCM56820:
    case SOC_CHIP_BCM56725:
    case SOC_CHIP_BCM88732:
        SOC_IF_ERROR_RETURN(READ_OP_PORT_TOTAL_COUNTr(unit, port, &val));
        *count += soc_reg_field_get(unit, OP_PORT_TOTAL_COUNTr, val,
                                    OP_PORT_TOTAL_COUNTf);
        if (IS_GX_PORT(unit, port) && !SOC_IS_SHADOW(unit)) {
            SOC_IF_ERROR_RETURN(READ_XP_XBODE_CELL_CNTr(unit, port, &val));
            *count += soc_reg_field_get(unit, XP_XBODE_CELL_CNTr, val,
                                        CELL_CNTf);
            SOC_IF_ERROR_RETURN(READ_GE_GBODE_CELL_CNTr(unit, port, &val));
            *count += soc_reg_field_get(unit, GE_GBODE_CELL_CNTr, val,
                                        CELL_CNTf);
        }
        break;
#endif /* BCM_BRADLEY_SUPPORT */
#if defined(BCM_TRIUMPH_SUPPORT)
    case SOC_CHIP_BCM56624:
    case SOC_CHIP_BCM56680:
    case SOC_CHIP_BCM56634:
    case SOC_CHIP_BCM56524:
    case SOC_CHIP_BCM56685:
    case SOC_CHIP_BCM56334:
        SOC_IF_ERROR_RETURN(READ_OP_PORT_TOTAL_COUNT_CELLr(unit, port, &val));
        *count += soc_reg_field_get(unit, OP_PORT_TOTAL_COUNT_CELLr, val,
                                   OP_PORT_TOTAL_COUNT_CELLf);
        break;
    case SOC_CHIP_BCM56640:
    case SOC_CHIP_BCM56340:
        SOC_IF_ERROR_RETURN(READ_OP_PORT_TOTAL_COUNT_CELLr(unit, port, &val));
        *count += soc_reg_field_get(unit, OP_PORT_TOTAL_COUNT_CELLr, val,
                                   TOTAL_COUNTf);
        break;
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
    case SOC_CHIP_BCM56840:
        for (i = 0; i < SOC_INFO(unit).port_num_uc_cosq[port]; i++) {
            SOC_IF_ERROR_RETURN
                (READ_OP_UC_QUEUE_TOTAL_COUNT_CELLr(unit, port, i, &val));
            *count += soc_reg_field_get(unit, OP_UC_QUEUE_TOTAL_COUNT_CELLr,
                                        val, Q_TOTAL_COUNT_CELLf);
        }
        for (i = 0; i < SOC_INFO(unit).port_num_cosq[port]; i++) {
            SOC_IF_ERROR_RETURN
                (READ_OP_QUEUE_TOTAL_COUNT_CELLr(unit, port, i, &val));
            *count += soc_reg_field_get(unit, OP_QUEUE_TOTAL_COUNT_CELLr,
                                        val, Q_TOTAL_COUNT_CELLf);
        }
        if (SOC_INFO(unit).port_num_ext_cosq[port] > 0) {
            for (i = 0; i < SOC_INFO(unit).port_num_uc_cosq[port] +
                     SOC_INFO(unit).port_num_ext_cosq[port]; i++) {
                SOC_IF_ERROR_RETURN
                    (READ_OP_EX_QUEUE_TOTAL_COUNT_CELLr(unit, port, i, &val));
                *count += soc_reg_field_get(unit,
                                            OP_EX_QUEUE_TOTAL_COUNT_CELLr, val,
                                            Q_TOTAL_COUNT_CELLf);
            }
        }
        break;
#endif /* BCM_TRIDENT_SUPPORT */
    case SOC_CHIP_BCM56440:
    case SOC_CHIP_BCM56450:
        
        *count = 0;
        break;
#if defined (BCM_TRIDENT2_SUPPORT)
    case SOC_CHIP_BCM56850:
        /* Unicast queues */
        index = SOC_INFO(unit).port_uc_cosq_base[port];
        if (index < 1480) {
            mem = MMU_THDU_XPIPE_COUNTER_QUEUEm;
        } else {
            index -= 1480;
            mem = MMU_THDU_YPIPE_COUNTER_QUEUEm;
        }
        for (i = 0; i < SOC_INFO(unit).port_num_uc_cosq[port]; i++) {
            SOC_IF_ERROR_RETURN
                (soc_mem_read(unit, mem, MEM_BLOCK_ANY, index + i, entry));
            *count += soc_mem_field32_get(unit, mem, entry, MIN_COUNTf);
            *count += soc_mem_field32_get(unit, mem, entry, SHARED_COUNTf);
        }

        /* Multicast queues */
        index = SOC_INFO(unit).port_cosq_base[port];
        if (index < 568) {
            mem = MMU_THDM_DB_QUEUE_COUNT_0m;
        } else {
            index -= 568;
            mem = MMU_THDM_DB_QUEUE_COUNT_1m;
        }
        for (i = 0; i < SOC_INFO(unit).port_num_cosq[port]; i++) {
            SOC_IF_ERROR_RETURN
                (soc_mem_read(unit, mem, MEM_BLOCK_ANY, index + i, entry));
            *count += soc_mem_field32_get(unit, mem, entry, MIN_COUNTf);
            *count += soc_mem_field32_get(unit, mem, entry, SHARED_COUNTf);
        }
        break;
#endif /* BCM_TRIDENT2_SUPPORT */
    default:
        return SOC_E_UNAVAIL;
    }

    return SOC_E_NONE;
}

#if defined(BCM_TRIDENT2_SUPPORT)
int
_soc_egress_cell_check(int unit, soc_port_t port, int *empty)
{
    soc_info_t          *si = &SOC_INFO(unit);
    int                 bit_pos;
    uint64              rval64;

    switch (SOC_CHIP_GROUP(unit)) {
    case SOC_CHIP_BCM56850:
        if (SAL_BOOT_SIMULATION) {
            
            *empty = TRUE;
            break;
        }
        if (SOC_PBMP_MEMBER(PBMP_XPIPE(unit), port)) {
            SOC_IF_ERROR_RETURN(READ_MMU_ENQ_PORT_EMPTY_BMP0r(unit, &rval64));
        } else {
            SOC_IF_ERROR_RETURN(READ_MMU_ENQ_PORT_EMPTY_BMP1r(unit, &rval64));
        }
        bit_pos = si->port_p2m_mapping[si->port_l2p_mapping[port]] & 0x3f;
        if (bit_pos < 32) {
            *empty = COMPILER_64_LO(rval64) & (1 << bit_pos) ? TRUE : FALSE;
        } else {
            *empty = COMPILER_64_HI(rval64) & (1 << (bit_pos - 32)) ?
                TRUE : FALSE;
        }
        break;
    default:
        return SOC_E_UNAVAIL;
    }

    return SOC_E_NONE;
}
#endif /* BCM_TRIDENT2_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
STATIC int
_soc_port_txfifo_cell_count(int unit, soc_port_t port, int *count)
{
    uint32 rval;

    switch (SOC_CHIP_GROUP(unit)) {
    case SOC_CHIP_BCM56640:
    case SOC_CHIP_BCM56340:
        SOC_IF_ERROR_RETURN(READ_PORT_TXFIFO_CELL_CNTr(unit, port, &rval));
        *count = soc_reg_field_get(unit, PORT_TXFIFO_CELL_CNTr, rval,
                                   CELL_CNTf);
        break;
    case SOC_CHIP_BCM56850:
        /* Should only be called by CMAC driver */
        SOC_IF_ERROR_RETURN(READ_CPORT_TXFIFO_CELL_CNTr(unit, port, &rval));
        *count = soc_reg_field_get(unit, CPORT_TXFIFO_CELL_CNTr, rval,
                                   CELL_CNTf);
        break;
    default:
        return SOC_E_UNAVAIL;
    }

    return SOC_E_NONE;
}
#endif /* BCM_TRIUMPH3_SUPPORT || BCM_TRIDENT2_SUPPORT */

STATIC int
soc_mmu_backpressure_clear(int unit, soc_port_t port) {
    switch (SOC_CHIP_GROUP(unit)) {
    case SOC_CHIP_BCM56840:
    case SOC_CHIP_BCM56640:
    case SOC_CHIP_BCM56850:
    case SOC_CHIP_BCM56340:
        SOC_IF_ERROR_RETURN(WRITE_XPORT_TO_MMU_BKPr(unit, port, 0));
        break;
    case SOC_CHIP_BCM56440:
        if (IS_HG_PORT(unit, port) || IS_XE_PORT(unit, port)) {
            SOC_IF_ERROR_RETURN(WRITE_XPORT_TO_MMU_BKPr(unit, port, 0));
        }
        break;
    default:
        break;
    }

    return SOC_E_NONE;
}

int
soc_mmu_flush_enable(int unit, soc_port_t port, int enable) 
{
#if defined(BCM_RAVEN_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIUMPH_SUPPORT) || defined(BCM_SCORPION_SUPPORT)  
    uint32 flush_ctrl;
#endif

    switch (SOC_CHIP_GROUP(unit)) {
#if defined(BCM_RAVEN_SUPPORT)
    case SOC_CHIP_BCM56224:
    case SOC_CHIP_BCM53314:
        SOC_IF_ERROR_RETURN
            (READ_MMUFLUSHCONTROLr(unit, &flush_ctrl));
        flush_ctrl &= ~(0x1 << port);
        flush_ctrl |= enable ? (0x1 << port) : 0;
        SOC_IF_ERROR_RETURN
            (WRITE_MMUFLUSHCONTROLr(unit, flush_ctrl));
        break;
#endif /* BCM_RAVEN_SUPPORT */

#if defined(BCM_HURRICANE2_SUPPORT)
    case SOC_CHIP_BCM56150:
        SOC_IF_ERROR_RETURN
            (READ_MMUFLUSHCONTROLr(unit, &flush_ctrl));
        flush_ctrl &= ~(0x1 << port);
        flush_ctrl |= enable ? (0x1 << port) : 0;
        SOC_IF_ERROR_RETURN
            (WRITE_MMUFLUSHCONTROLr(unit, flush_ctrl));
        soc_cm_debug(DK_VERBOSE, "port %d MMUFLUSHCONTROl.FLUSH=0x%x\n", port, flush_ctrl);
        break;
#endif /* BCM_HURRICANE2_SUPPORT */

#if defined(BCM_HURRICANE_SUPPORT)
    case SOC_CHIP_BCM56142:
        SOC_IF_ERROR_RETURN
            (READ_MMUFLUSHCONTROLr(unit, &flush_ctrl));
        flush_ctrl &= ~(0x1 << port);
        flush_ctrl |= enable ? (0x1 << port) : 0;
        SOC_IF_ERROR_RETURN
            (WRITE_MMUFLUSHCONTROLr(unit, flush_ctrl));
        soc_cm_debug(DK_VERBOSE, "port %d MMUFLUSHCONTROl.FLUSH=0x%x\n", port, flush_ctrl);
    /* Fall through */
#endif /* BCM_HURRICANE_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
    case SOC_CHIP_BCM56634:
    case SOC_CHIP_BCM56524:
    case SOC_CHIP_BCM56685:
        SOC_IF_ERROR_RETURN
            (READ_FAST_TX_FLUSHr(unit, port, &flush_ctrl));
        soc_reg_field_set(unit, FAST_TX_FLUSHr, &flush_ctrl, IDf,
                          (enable) ? 1 : 0);
        SOC_IF_ERROR_RETURN
            (WRITE_FAST_TX_FLUSHr(unit, port, flush_ctrl)); 
        soc_cm_debug(DK_VERBOSE, "port %d FAST_TX_FLUSH.ID=0x%x\n", port, flush_ctrl);
    /* Fall through */
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
    case SOC_CHIP_BCM56624:
    case SOC_CHIP_BCM56680:
    case SOC_CHIP_BCM56334:
    /*case SOC_CHIP_BCM88732:*/
        if (IS_XE_PORT(unit, port) || IS_HG_PORT(unit, port)) {
            SOC_IF_ERROR_RETURN
                (READ_XP_EGR_PKT_DROP_CTLr(unit, port, &flush_ctrl));
            soc_reg_field_set(unit, XP_EGR_PKT_DROP_CTLr, &flush_ctrl, FLUSHf,
                              (enable) ? 1 : 0);
            SOC_IF_ERROR_RETURN
                (WRITE_XP_EGR_PKT_DROP_CTLr(unit, port, flush_ctrl)); 
           soc_cm_debug(DK_VERBOSE, "port %d XP_EGR_PKT_DROP_CTL.FLUSH=0x%x\n", port, flush_ctrl);
        }
        break;
#endif /* BCM_TRIUMPH_SUPPORT */

#if defined(BCM_SCORPION_SUPPORT)
    case SOC_CHIP_BCM56820:
    case SOC_CHIP_BCM56725:
        if (IS_GX_PORT(unit, port)) {
            SOC_IF_ERROR_RETURN
                (READ_XP_EGR_PKT_DROP_CTLr(unit, port, &flush_ctrl));
            soc_reg_field_set(unit, XP_EGR_PKT_DROP_CTLr, &flush_ctrl, FLUSHf,
                              (enable) ? 1 : 0);
            SOC_IF_ERROR_RETURN
                (WRITE_XP_EGR_PKT_DROP_CTLr(unit, port, flush_ctrl)); 
        }
        break;
#endif /* BCM_TRIUMPH_SUPPORT || BCM_SCORPION_SUPPORT */

#if defined(BCM_TRIDENT_SUPPORT)
    case SOC_CHIP_BCM56840:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLP_TXFIFO_PKT_DROP_CTLr, port,
                                    DROP_ENf, enable ? 1 : 0));
        break;
#endif /* BCM_TRIDENT_SUPPORT */

    case SOC_CHIP_BCM56640:
    case SOC_CHIP_BCM56340:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, PORT_TXFIFO_PKT_DROP_CTLr, port,
                                    DROP_ENf, enable ? 1 : 0));
        break;
    default:
        return SOC_E_NONE;
    }

    return SOC_E_NONE; 
}

STATIC int
_soc_egress_metering_freeze(int unit, soc_port_t port, void *setting)
{
    int      rv;

    rv = SOC_E_NONE;

    SOC_EGRESS_METERING_LOCK(unit);

    switch (SOC_CHIP_GROUP(unit)) {
#if defined(BCM_RAVEN_SUPPORT) || defined(BCM_SCORPION_SUPPORT) || defined(BCM_HURRICANE_SUPPORT)
    case SOC_CHIP_BCM56820:
    case SOC_CHIP_BCM56725:
    case SOC_CHIP_BCM56224:
    case SOC_CHIP_BCM53314:
    case SOC_CHIP_BCM56142:
    case SOC_CHIP_BCM56150:
        {
            uint32 *rval32;
            rval32 = setting;
            rv = READ_EGRMETERINGCONFIGr(unit, port, rval32);
            if (SOC_SUCCESS(rv)) {
                rv = WRITE_EGRMETERINGCONFIGr(unit, port, 0);
            }
        }
        break;
#endif /* BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT || BCM_HURRICANE_SUPPORT */

#if defined(BCM_TRIUMPH_SUPPORT)
    case SOC_CHIP_BCM56624:
    case SOC_CHIP_BCM56680:
    case SOC_CHIP_BCM56634:
    case SOC_CHIP_BCM56524:
    case SOC_CHIP_BCM56685:
    case SOC_CHIP_BCM56334:
    case SOC_CHIP_BCM56840:
    case SOC_CHIP_BCM56640:
    case SOC_CHIP_BCM56340:
        {
            uint64 *rval64, temp_rval64;
            rval64 = setting;

            rv = READ_EGRMETERINGCONFIG_64r(unit, port, rval64);
            if (SOC_SUCCESS(rv)) {
                COMPILER_64_ZERO(temp_rval64);
                rv = WRITE_EGRMETERINGCONFIG_64r(unit, port, temp_rval64);
            }
        }
        break;
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    case SOC_CHIP_BCM56850:
        {
            soc_mem_t mem;
            int phy_port, mmu_port, index;

            phy_port = SOC_INFO(unit).port_l2p_mapping[port];
            if (phy_port == -1) {
                rv = SOC_E_PORT;
                break;
            }
            mmu_port = SOC_INFO(unit).port_p2m_mapping[phy_port];
            if (mmu_port < SOC_INFO(unit).mmu_port_base[1]) {
                mem = MMU_MTRO_EGRMETERINGCONFIG_MEM_0m;
                index = mmu_port;
            } else {
                mem = MMU_MTRO_EGRMETERINGCONFIG_MEM_1m;
                index = mmu_port - SOC_INFO(unit).mmu_port_base[1];
            }
            rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, index, setting);
            if (SOC_SUCCESS(rv)) {
                rv = soc_mem_write(unit, mem, MEM_BLOCK_ANY, index,
                                   soc_mem_entry_null(unit, mem));
            }
        }
        break;
#endif /* BCM_TRIDENT2_SUPPORT */

    case SOC_CHIP_BCM56440:
    default:
        break;
    }

    if (SOC_FAILURE(rv)) {
        /* UNLOCK if fail */
        SOC_EGRESS_METERING_UNLOCK(unit);
    }

    return rv;
}

STATIC int
_soc_egress_metering_thaw(int unit, soc_port_t port, void *setting)
{
    int       rv;

    rv = SOC_E_NONE;

    switch (SOC_CHIP_GROUP(unit)) {
#if defined(BCM_RAVEN_SUPPORT) || defined(BCM_SCORPION_SUPPORT) || defined(BCM_HURRICANE_SUPPORT)
    case SOC_CHIP_BCM56224:
    case SOC_CHIP_BCM56820:
    case SOC_CHIP_BCM56725:
    case SOC_CHIP_BCM53314:
    case SOC_CHIP_BCM56142:
    case SOC_CHIP_BCM56150:
        {
            uint32 *rval32;
            rval32 = setting;
            /* Restore egress metering configuration. */
            rv = WRITE_EGRMETERINGCONFIGr(unit, port, *rval32);
        }
        break;
#endif /* BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT || BCM_HURRICANE_SUPPORT */
#if defined(BCM_TRIUMPH_SUPPORT)
    case SOC_CHIP_BCM56624:
    case SOC_CHIP_BCM56680:
    case SOC_CHIP_BCM56634:
    case SOC_CHIP_BCM56524:
    case SOC_CHIP_BCM56334:
    case SOC_CHIP_BCM56685:
    case SOC_CHIP_BCM56840:
    case SOC_CHIP_BCM56640:
    case SOC_CHIP_BCM56340:
        {
            uint64 *rval64;
            rval64 = setting;
            rv = WRITE_EGRMETERINGCONFIG_64r(unit, port, *rval64);
        }
        break;
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
    case SOC_CHIP_BCM56850:
        {
            soc_mem_t mem;
            int phy_port, mmu_port, index;

            phy_port = SOC_INFO(unit).port_l2p_mapping[port];
            if (phy_port == -1) {
                return SOC_E_PORT;
            }
            mmu_port = SOC_INFO(unit).port_p2m_mapping[phy_port];
            if (mmu_port < SOC_INFO(unit).mmu_port_base[1]) {
                mem = MMU_MTRO_EGRMETERINGCONFIG_MEM_0m;
                index = mmu_port;
            } else {
                mem = MMU_MTRO_EGRMETERINGCONFIG_MEM_1m;
                index = mmu_port - SOC_INFO(unit).mmu_port_base[1];
            }
            rv = soc_mem_write(unit, mem, MEM_BLOCK_ANY, index, setting);
        }
        break;
#endif /* BCM_TRIDENT2_SUPPORT */
    case SOC_CHIP_BCM56440:
    default:
        break;
    }

    SOC_EGRESS_METERING_UNLOCK(unit);

    return rv;
}

int
soc_egress_drain_cells(int unit, soc_port_t port, uint32 drain_timeout)
{
    soc_timeout_t to;
    uint32 meter_key[SOC_MAX_MEM_WORDS];
    uint32 cur_cells, new_cells;
    int rv;

#if defined(BCM_SIRIUS_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
    if (SOC_IS_SIRIUS(unit) || SOC_IS_CALADAN3(unit)) {
      return SOC_E_NONE;
    }
#endif /* BCM_SIRIUS_SUPPORT || BCM_CALADAN3_SUPPORT */

    SOC_IF_ERROR_RETURN(soc_mmu_backpressure_clear(unit, port));

    /*
     **************************************************************
     * NOTE: Must not exit this function without calling 
     *       soc_egress_metering_thaw(). soc_egress_metering_freeze
     *       holds a lock.
     **************************************************************
     */
    SOC_IF_ERROR_RETURN(_soc_egress_metering_freeze(unit, port, &meter_key));

#if defined(BCM_TRIDENT2_SUPPORT)
    
#endif /* BCM_TRIDENT2_SUPPORT */

    cur_cells = 0xffffffff;

    /* Probably not required to continuously check COSLCCOUNT if the fast
     * MMU flush feature is available - done just as an insurance */
    rv = SOC_E_NONE;
    for (;;) {
        if ((rv = soc_egress_cell_count(unit, port, &new_cells)) < 0) {
            break;
        }

        if (new_cells == 0) {
            rv = SOC_E_NONE;
            break;
        }

        if (new_cells < cur_cells) {                    /* Progress made */
            /* Restart timeout */
            soc_timeout_init(&to, drain_timeout, 0);
            cur_cells = new_cells;
        }

        if (soc_timeout_check(&to)) {
            if ((rv = soc_egress_cell_count(unit, port, &new_cells)) < 0) {
                break;
            }

            soc_cm_debug(DK_ERR,
                         "ERROR: port %d:%s: "
                         "timeout draining packets (%d cells remain)\n",
                         unit, SOC_PORT_NAME(unit, port), new_cells);
            rv = SOC_E_INTERNAL;
            break;
        }
    }

    /* Restore egress metering configuration. */
    SOC_IF_ERROR_RETURN(_soc_egress_metering_thaw(unit, port, meter_key));

    return rv;
}

int
soc_txfifo_drain_cells(int unit, soc_port_t port, uint32 drain_timeout)
{
#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
      return SOC_E_NONE;
    }
#endif /* BCM_SIRIUS_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
        soc_timeout_t to;
        int count;

        soc_timeout_init(&to, drain_timeout, 0);
        for (;;) {
            SOC_IF_ERROR_RETURN
                (_soc_port_txfifo_cell_count(unit, port, &count));
            if (!count) {
                break;
            }
            if (soc_timeout_check(&to)) {
                soc_cm_debug(DK_ERR,
                             "ERROR: port %d:%s: timeout draining TXFIFO "
                             "(pending: %d)\n",
                             unit, SOC_PORT_NAME(unit, port), count);
                return SOC_E_INTERNAL;
            }
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT || BCM_TRIDENT2_SUPPORT */

    return SOC_E_NONE;
}

int
soc_port_credit_reset(int unit, soc_port_t port)
{
    int phy_port;
    egr_port_credit_reset_entry_t entry;

    switch (SOC_CHIP_GROUP(unit)) {
    case SOC_CHIP_BCM56850:
        /* Should only be called by XLMAC driver */
        phy_port = SOC_INFO(unit).port_l2p_mapping[port];
        sal_memset(&entry, 0, sizeof(entry));
        soc_mem_field32_set(unit, EGR_PORT_CREDIT_RESETm, &entry, VALUEf, 1);
        SOC_IF_ERROR_RETURN(WRITE_EGR_PORT_CREDIT_RESETm(unit, MEM_BLOCK_ALL,
                                                         phy_port, &entry));
        soc_mem_field32_set(unit, EGR_PORT_CREDIT_RESETm, &entry, VALUEf, 0);
        SOC_IF_ERROR_RETURN(WRITE_EGR_PORT_CREDIT_RESETm(unit, MEM_BLOCK_ALL,
                                                         phy_port, &entry));
        break;
    case SOC_CHIP_BCM56150:
    {
        uint32 rval;
        /* Should only be called by XLMAC driver */
        int bindex;
        static const soc_field_t port_field[] = {
            PORT0f, PORT1f, PORT2f, PORT3f
        };
        phy_port = SOC_INFO(unit).port_l2p_mapping[port];
        bindex = SOC_PORT_BINDEX(unit, phy_port);

        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLPORT_ENABLE_REGr, port,
                                    port_field[bindex], 0));

        sal_memset(&entry, 0, sizeof(entry));
        soc_mem_field32_set(unit, EGR_PORT_CREDIT_RESETm, &entry, VALUEf, 1);
        SOC_IF_ERROR_RETURN
            (WRITE_EGR_PORT_CREDIT_RESETm(unit, MEM_BLOCK_ALL, phy_port, &entry));

        SOC_IF_ERROR_RETURN
            (READ_XLPORT_TXFIFO_CTRLr(unit, port, &rval));
        soc_reg_field_set(unit, XLPORT_TXFIFO_CTRLr, &rval, MAC_CLR_COUNTf,1);
        soc_reg_field_set(unit, XLPORT_TXFIFO_CTRLr, &rval, CORE_CLR_COUNTf,1);
        SOC_IF_ERROR_RETURN
            (WRITE_XLPORT_TXFIFO_CTRLr(unit, port, rval));

        sal_usleep(1000);

        soc_mem_field32_set(unit, EGR_PORT_CREDIT_RESETm, &entry, VALUEf, 0);
        SOC_IF_ERROR_RETURN
            (WRITE_EGR_PORT_CREDIT_RESETm(unit, MEM_BLOCK_ALL, phy_port, &entry));

        soc_reg_field_set(unit, XLPORT_TXFIFO_CTRLr, &rval, MAC_CLR_COUNTf,0);
        soc_reg_field_set(unit, XLPORT_TXFIFO_CTRLr, &rval, CORE_CLR_COUNTf,0);
        SOC_IF_ERROR_RETURN
            (WRITE_XLPORT_TXFIFO_CTRLr(unit, port, rval));

        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLPORT_ENABLE_REGr, port,
                                    port_field[bindex], 1));
    }
        break;
    default:
        break;
    }

    return SOC_E_NONE;
}

int
soc_port_fifo_reset(int unit, soc_port_t port)
{
    int phy_port, block, bindex, i;
    uint32 rval, orig_rval;
    static const soc_field_t fields[] = {
        PORT0f, PORT1f, PORT2f, PORT3f
    };

    switch (SOC_CHIP_GROUP(unit)) {
    case SOC_CHIP_BCM56850:
        phy_port = SOC_INFO(unit).port_l2p_mapping[port];
        SOC_IF_ERROR_RETURN(READ_XLPORT_SOFT_RESETr(unit, port, &rval));
        orig_rval = rval;
        for (i = 0; i < SOC_DRIVER(unit)->port_num_blktype; i++) {
            block = SOC_PORT_IDX_BLOCK(unit, phy_port, i);
            if (SOC_BLOCK_INFO(unit, block).type == SOC_BLK_XLPORT) {
                bindex = SOC_PORT_IDX_BINDEX(unit, phy_port, i);
                soc_reg_field_set(unit, XLPORT_SOFT_RESETr, &rval,
                                  fields[bindex], 1);
                break;
            }
        }
        SOC_IF_ERROR_RETURN(WRITE_XLPORT_SOFT_RESETr(unit, port, rval));
        SOC_IF_ERROR_RETURN(WRITE_XLPORT_SOFT_RESETr(unit, port, orig_rval));
        break;
    default:
        break;
    }

    return SOC_E_NONE;
}

int
soc_port_blk_init(int unit, soc_port_t port)
{    switch (SOC_CHIP_GROUP(unit)) {
#if defined(BCM_RAVEN_SUPPORT)
    case SOC_CHIP_BCM56224:
        if (IS_S_PORT(unit, port)) {
            soc_pbmp_t pbmp_s0, pbmp_s1, pbmp_s3, pbmp_s4;
            uint32     val32;
            int        higig_mode;

            SOC_IF_ERROR_RETURN(READ_GPORT_CONFIGr(unit, port, &val32));
            soc_reg_field_set(unit, GPORT_CONFIGr, &val32, CLR_CNTf, 1);
            soc_reg_field_set(unit, GPORT_CONFIGr, &val32, GPORT_ENf, 1);
            SOC_PBMP_WORD_SET(pbmp_s0, 0, 0x00000002);
            SOC_PBMP_WORD_SET(pbmp_s1, 0, 0x00000004);
            SOC_PBMP_WORD_SET(pbmp_s3, 0, 0x00000010);
            SOC_PBMP_WORD_SET(pbmp_s4, 0, 0x00000020);
 
            higig_mode = IS_ST_PORT(unit, port) ? 1 : 0;
            if (SOC_PBMP_MEMBER(pbmp_s0, port)) {
                /* The "SOP check enables" are not gated with "HiGig2 enables"
                 * and "bond_disable_stacking".
                 * Thus software need to enable/disable the SOP drop check
                 * in HiGig2 mode as desired.
                 */
                soc_reg_field_set(unit, GPORT_CONFIGr, &val32,
                                  DROP_ON_WRONG_SOP_EN_S0f, 0);
                /* Enable HiGig 2 */
                /* Assuming that always use stacking port in HiGig mode */
                /* Actually, stacking port can also be used in ethernet mode */
                soc_reg_field_set(unit, GPORT_CONFIGr, &val32,
                                  HGIG2_EN_S0f, higig_mode); 
            } else if (SOC_PBMP_MEMBER(pbmp_s1, port)) {
                soc_reg_field_set(unit, GPORT_CONFIGr, &val32,
                                  DROP_ON_WRONG_SOP_EN_S1f, 0);
                soc_reg_field_set(unit, GPORT_CONFIGr, &val32,
                                  HGIG2_EN_S1f, higig_mode);
            } else if (SOC_PBMP_MEMBER(pbmp_s3, port)) {
                soc_reg_field_set(unit, GPORT_CONFIGr, &val32,
                                  DROP_ON_WRONG_SOP_EN_S3f, 0);
                soc_reg_field_set(unit, GPORT_CONFIGr, &val32,
                                  HGIG2_EN_S3f, higig_mode); 
            } else if (SOC_PBMP_MEMBER(pbmp_s4, port)) {
                soc_reg_field_set(unit, GPORT_CONFIGr, &val32,
                                  DROP_ON_WRONG_SOP_EN_S4f, 0);
                soc_reg_field_set(unit, GPORT_CONFIGr, &val32,
                                  HGIG2_EN_S4f, higig_mode); 
            }

            SOC_IF_ERROR_RETURN
                (WRITE_GPORT_CONFIGr(unit, port, val32));

            /* Reset the clear-count bit after 64 clocks */
            soc_reg_field_set(unit, GPORT_CONFIGr, &val32, CLR_CNTf, 0);
            SOC_IF_ERROR_RETURN
                (WRITE_GPORT_CONFIGr(unit, port, val32));
        }
        break;
#endif /* BCM_RAVEN_SUPPORT */
    default:
        return SOC_E_NONE;
    }
    return SOC_E_NONE;
}

int
soc_packet_purge_control_init(int unit, soc_port_t port)
{
    uint32 mask;
    mask = soc_property_port_get(unit, port, spn_GPORT_RSV_MASK, 0x070);
    switch (SOC_CHIP_GROUP(unit)) {
#if defined(BCM_RAVEN_SUPPORT)
    case SOC_CHIP_BCM56224:
    case SOC_CHIP_BCM53314:
        /* GPORT_RSV_MASK fields
         * bit
         * 0     frm_align_err_latch
         * 1     rx_frm_stack_vlan_latch
         * 2     rx_carrier_event_latch
         * 3     rx_frm_gmii_err_latch
         * 4     (frm_crc_err_latch | frm_crc_err)
         * 5     frm_length_err_latch & ~frm_truncate_latch
         * 6     frm_truncate_latch
         * 7     ~rx_frm_err_latch & ~frm_truncate_latch
         * 8     rx_frm_mltcast_latch
         * 9     rx_frm_broadcast_latch
         * 10    drbl_nbl_latch
         * 11    cmd_rcv_latch
         * 12    pause_rcv_latch
         * 13    rx_cmd_op_err_latch
         * 14    rx_frm_vlan_latch
         * 15    rx_frm_unicast_latch
         * 16    frm_truncate_latch
         */
        SOC_IF_ERROR_RETURN
            (WRITE_GPORT_RSV_MASKr(unit, port, mask));
        SOC_IF_ERROR_RETURN
            (WRITE_GPORT_STAT_UPDATE_MASKr(unit, port, mask));
        break;
#endif /* BCM_RAVEN_SUPPORT */
#ifdef BCM_TRX_SUPPORT
    case SOC_CHIP_BCM56624:
    case SOC_CHIP_BCM56680:
    case SOC_CHIP_BCM56634:
    case SOC_CHIP_BCM56524:
    case SOC_CHIP_BCM56685:
    case SOC_CHIP_BCM56820:
    case SOC_CHIP_BCM56725:
        SOC_IF_ERROR_RETURN
            (WRITE_GPORT_RSV_MASKr(unit, port, mask));
        SOC_IF_ERROR_RETURN
            (WRITE_GPORT_STAT_UPDATE_MASKr(unit, port, mask));
        break;
    case SOC_CHIP_BCM56334:
    case SOC_CHIP_BCM56142:
    case SOC_CHIP_BCM56150:
        if (IS_XE_PORT(unit, port) || IS_HG_PORT(unit, port) ||
            IS_XQ_PORT(unit, port)) {
            SOC_IF_ERROR_RETURN
                (WRITE_QPORT_RSV_MASKr(unit, port, mask));
            SOC_IF_ERROR_RETURN
                (WRITE_QPORT_STAT_UPDATE_MASKr(unit, port, mask)); 
        } else {
            SOC_IF_ERROR_RETURN
                (WRITE_GPORT_RSV_MASKr(unit, port, mask));
            SOC_IF_ERROR_RETURN
                (WRITE_GPORT_STAT_UPDATE_MASKr(unit, port, mask));
        }
        break;
#endif
    default:
        return SOC_E_NONE;
    }

    return SOC_E_NONE;
}

int
soc_egress_enable(int unit, soc_port_t port, int enable)
{
#ifdef BCM_SHADOW_SUPPORT
    soc_info_t          *si;
#endif

#ifdef BCM_SHADOW_SUPPORT
    si = &SOC_INFO(unit);
#endif

    switch (SOC_CHIP_GROUP(unit)) {
#if defined(BCM_RAVEN_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
    case SOC_CHIP_BCM56820:
    case SOC_CHIP_BCM56224:
    case SOC_CHIP_BCM53314:
    case SOC_CHIP_BCM88732:
    {
        uint32 val32;
        SOC_IF_ERROR_RETURN(READ_EGR_ENABLEr(unit, port, &val32));
#ifdef BCM_SHADOW_SUPPORT
        if (!SOC_PBMP_MEMBER(si->port.disabled_bitmap, port)) {
         soc_reg_field_set(unit, EGR_ENABLEr, &val32, PRT_ENABLEf, 1);
        }
#else
        soc_reg_field_set(unit, EGR_ENABLEr, &val32, PRT_ENABLEf, 1);
#endif
        SOC_IF_ERROR_RETURN(WRITE_EGR_ENABLEr(unit, port, val32));
        break;
    }
#endif /* BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
    default:
        return SOC_E_NONE;
    }
    return SOC_E_NONE;
}

int
soc_port_speed_update(int unit, soc_port_t port, int speed) 
{
    switch (SOC_CHIP_GROUP(unit)) {
    case SOC_CHIP_BCM56850:
#ifdef BCM_TRIDENT2_SUPPORT
        SOC_IF_ERROR_RETURN(soc_trident2_port_speed_update(unit, port, speed));
#endif
        break;
    default:
        break;
    }

    return SOC_E_NONE;

}

#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) */

