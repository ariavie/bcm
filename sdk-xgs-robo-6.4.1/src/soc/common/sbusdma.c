/*
 * $Id: sbusdma.c,v 1.107 Broadcom SDK $
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
 * Purpose: SOC SBUS-DMA and FIFO DMA driver.
 *
 */

#include <shared/bsl.h>

#include <sal/core/boot.h>
#include <sal/core/libc.h>
#include <shared/alloc.h>

#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/cmicm.h>
#include <soc/l2x.h>

#ifdef BCM_TRIUMPH3_SUPPORT
#include <soc/er_tcam.h>
#include <soc/triumph3.h>
#endif

#ifdef BCM_KATANA2_SUPPORT
#include <soc/katana2.h>
#include <soc/katana.h>
#endif

#ifdef BCM_CALADAN3_SUPPORT
#include <soc/sbx/caladan3.h>
#endif

#if defined(BCM_ESW_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || \
    defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#ifdef BCM_SBUSDMA_SUPPORT
#if 0
#include <limits.h>
#else
#if (!defined(_LIMITS_H)) && !defined(_LIBC_LIMITS_H_)

#if (!defined(INT_MIN)) && !defined(INT_MAX)
#define INT_MIN (((int)1)<<(sizeof(int)*8-1))
#define INT_MAX (~INT_MIN)
#endif

#ifndef UINT_MAX
#define UINT_MAX ((unsigned)-1)
#endif

#endif
#endif /* #if 0 */

#ifdef BCM_KATANA2_SUPPORT
#define RD_DMA_CFG_REG                   0
#define RD_DMA_HOTMEM_THRESHOLD_REG      1
#define RD_DMA_STAT                      2
#define RD_DMA_STAT_CLR                  3
#endif 

#define SOC_SBUSDMA_FP_RETRIES 200

STATIC uint32 _soc_irq_sbusdma_ch[] = {
    IRQ_SBUSDMA_CH0_DONE, 
    IRQ_SBUSDMA_CH1_DONE,
    IRQ_SBUSDMA_CH2_DONE
};

STATIC void
_soc_sbusdma_curr_op_details(int unit, int cmc, int ch)
{
    uint32 rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch));
    LOG_WARN(BSL_LS_SOC_COMMON,
             (BSL_META_U(unit,
                         "STATUS: 0x%08x\n"), rval));
    rval = soc_pci_read(unit, 
                        CMIC_CMCx_SBUSDMA_CHy_CUR_SBUSDMA_CONFIG_OPCODE(cmc, ch));
    LOG_WARN(BSL_LS_SOC_COMMON,
             (BSL_META_U(unit,
                         "OPCODE: 0x%08x\n"), rval));
    rval = soc_pci_read(unit, 
                        CMIC_CMCx_SBUSDMA_CHy_CUR_SBUSDMA_CONFIG_ADDR(cmc, ch));
    LOG_WARN(BSL_LS_SOC_COMMON,
             (BSL_META_U(unit,
                         "START ADDR: 0x%08x\n"), rval));
    rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_CUR_SBUSADDR(cmc, ch));
    LOG_WARN(BSL_LS_SOC_COMMON,
             (BSL_META_U(unit,
                         "CUR ADDR: 0x%08x\n"), rval));
}

STATIC void
_soc_sbusdma_error_details(int unit, uint32 rval)
{
    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                          rval, DESCRD_ERRORf)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Error while reading descriptor from host memory.\n")));
    }
    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                          rval, SBUSACK_TIMEOUTf)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "sbus ack not received within configured time.\n")));
    }
    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                          rval, SBUSACK_ERRORf)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "H/W received sbus ack with error bit set.\n")));
    }
    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                          rval, SBUSACK_NACKf)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "H/W received sbus nack with error bit set.\n")));
    }
    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                          rval, SBUSACK_WRONG_OPCODEf)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Received sbus ack has wrong opcode.\n")));
    }
    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                          rval, SBUSACK_WRONG_BEATCOUNTf)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Received sbus ack data size is not same is in rep_words "
                              "fields.\n")));
    }
    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                          rval, SER_CHECK_FAILf)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Received sbus ack has wrong opcode.\n")));
    }
    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                          rval, HOSTMEMRD_ERRORf)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Error while copying SBUSDMA data from Host Memory.\n")));
    }
    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                          rval, HOSTMEMWR_ERRORf)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "DMA operation encountered a schan response error "
                              "or host side error.\n")));
    }
}

/*
 * Function:
 *     _soc_mem_sbusdma_read
 * Purpose:
 *     DMA acceleration for soc_mem_read_range()
 * Parameters:
 *     buffer -- must be pointer to sufficiently large
 *               DMA-able block of memory
 */
int
_soc_mem_sbusdma_read(int unit, soc_mem_t mem, int copyno,
                      int index_min, int index_max,
                      uint32 ser_flags, void *buffer)
{
    return _soc_mem_array_sbusdma_read(unit, mem, 0, copyno,
                                       index_min, index_max, ser_flags, buffer);
}

int
_soc_mem_array_sbusdma_read(int unit, soc_mem_t mem, unsigned array_index,
                            int copyno, int index_min, int index_max,
                            uint32 ser_flags, void *buffer)
{
    soc_control_t *soc = SOC_CONTROL(unit);
    uint32        start_addr;
    uint32        count;
    uint32        data_beats;
    uint32        spacing;
    int           rv = SOC_E_NONE;
    uint32        ctrl, rval;
    uint8         at;
    int           ch;
    int           cmc = SOC_PCI_CMC(unit);
    schan_msg_t   msg;
    int           dst_blk, acc_type, dma;
#ifdef PRINT_DMA_TIME
    sal_usecs_t   start_time;
    int           diff_time;
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
    uint32        wa_rval, wa_val = 0;
#endif

    LOG_INFO(BSL_LS_SOC_DMA,
             (BSL_META_U(unit,
                         "_soc_mem_sbusdma_read: unit %d"
                         " mem %s.%s[%u] index %d-%d SER flags 0x%08x buffer %p\n"),
              unit, SOC_MEM_UFNAME(unit, mem), SOC_BLOCK_NAME(unit, copyno),
              array_index, index_min, index_max, ser_flags, buffer));

    data_beats = soc_mem_entry_words(unit, mem);

    ch = soc->tdma_ch;

#if defined(BCM_CALADAN3_SUPPORT)
    if (SOC_IS_CALADAN3(unit)) {
        soc_sbx_caladan3_sbusdma_cmc_ch_map(unit, mem,
                                     SOC_SBUSDMA_TYPE_TDMA, &cmc, &ch);
    }
#endif

    assert((ch >= 0) && (ch < soc->max_sbusdma_channels));
    count = index_max - index_min + 1;
    if (count < 1) {
        return SOC_E_NONE;
    }
    schan_msg_clear(&msg);
    acc_type = SOC_MEM_ACC_TYPE(unit, mem);
    dst_blk = SOC_BLOCK2SCH(unit, copyno);
    
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (0 != (ser_flags & _SOC_SER_FLAG_MULTI_PIPE)) {
        if (ser_flags & _SOC_SER_FLAG_ACC_TYPE_MASK) {
            /* Override ACC_TYPE in opcode */
            acc_type = ser_flags & _SOC_SER_FLAG_ACC_TYPE_MASK;
;
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    if (SOC_IS_ARAD(unit) || SOC_IS_CALADAN3(unit)) {
        /* DMA transaction indicator on SBUS */
        dma = 1;
    } else {
        dma = 0;
    }
    soc_schan_header_cmd_set(unit, &msg.header, READ_MEMORY_CMD_MSG,
                             dst_blk, 0, acc_type, 4, dma, 0);  
    start_addr = soc_mem_addr_get(unit, mem, array_index,
                                  copyno, index_min, &at);

    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        SBUS_DMA_LOCK(unit, cmc, ch);
    } else {
        TABLE_DMA_LOCK(unit);
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_esm_correction) &&
    	  ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) || 
         (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU))) {
        SOC_ESM_LOCK(unit);
    }
#endif

    ctrl = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch));
    /* Set mode, clear abort, start (clears status and errors) */
    soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl,
                      MODEf, 0);
    soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl,
                      ABORTf, 0);
    soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl,
                      STARTf, 0);
    soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch), ctrl);
    /* Set 1st schan ctrl word as opcode */
    soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_OPCODE(cmc, ch), msg.dwords[0]);
    soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_HOSTADDR(cmc, ch), 
                  soc_cm_l2p(unit, buffer));
    soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_ADDRESS(cmc, ch), start_addr);
    soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_COUNT(cmc, ch), count);

    /* Program beats, increment etc */
    rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_REQUEST(cmc, ch));
    soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                      REP_WORDSf, data_beats);

#if defined(BCM_CALADAN3_SUPPORT)
    if (SOC_IS_CALADAN3(unit)) {
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                          REQ_WORDSf, 0);
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                          INCR_SHIFTf, 0);
        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_REQUEST(cmc, ch), rval);
    }
#endif    

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_etu_support)) {
        if (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU) {
            int index0, index1, increment;
            soc_mem_t real_mem;
    
            soc_tcam_mem_index_to_raw_index(unit, mem, 0, &real_mem, &index0);
            soc_tcam_mem_index_to_raw_index(unit, mem, 1, &real_mem, &index1);
            increment = _shr_popcount(index1 - index0 - 1);
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                              INCR_SHIFTf, increment);
            SOC_IF_ERROR_RETURN(READ_ETU_TX_REQ_FIFO_CTLr(unit, &wa_rval));
            wa_val = soc_reg_field_get(unit, ETU_TX_REQ_FIFO_CTLr, wa_rval, 
                                       CP_ACC_THRf);
            soc_reg_field_set(unit, ETU_TX_REQ_FIFO_CTLr, &wa_rval,
                              CP_ACC_THRf, 0);
            SOC_IF_ERROR_RETURN(WRITE_ETU_TX_REQ_FIFO_CTLr(unit, wa_rval));
        } else {
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                              INCR_SHIFTf, 0);
            if ((mem == EXT_L2_ENTRY_1m) || (mem == EXT_L2_ENTRY_2m)) {
                SOC_IF_ERROR_RETURN(READ_ETU_TX_REQ_FIFO_CTLr(unit, &wa_rval));
                wa_val = soc_reg_field_get(unit, ETU_TX_REQ_FIFO_CTLr, wa_rval, 
                                           CP_ACC_THRf);
                soc_reg_field_set(unit, ETU_TX_REQ_FIFO_CTLr, &wa_rval,
                                  CP_ACC_THRf, 0);
                SOC_IF_ERROR_RETURN(WRITE_ETU_TX_REQ_FIFO_CTLr(unit, wa_rval));
            }
        }
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
#if (defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT))
    if (SOC_IS_DPP(unit) || SOC_IS_DFE(unit)) {
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                          INCR_SHIFTf, 0);
    } else
#endif
    {
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                          INCR_SHIFTf, 0);
    }
    if (soc_feature(unit, soc_feature_multi_sbus_cmds)) {
        
        if (soc->sbusCmdSpacing < 0) {
            spacing = data_beats > 7 ? data_beats + 1 : 8;
        } else {
            spacing = soc->sbusCmdSpacing;
        }
        if (!((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_IPIPE) ||
             (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_EPIPE))) {
            spacing = 0;
        }
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                          PEND_CLOCKSf, spacing);
    }
    soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_REQUEST(cmc, ch), rval);
    soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl, STARTf, 1);
    /* Start DMA */
#ifdef PRINT_DMA_TIME
    start_time = sal_time_usecs();
#endif
    soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch), ctrl);

    LOG_INFO(BSL_LS_SOC_DMA,
             (BSL_META_U(unit,
                         "_soc_mem_sbusdma_read: %d entries %d beats "
                         "addr 0x%x (index %d-%d) Interrupt-Mode(%d)\n"),
              count, data_beats, start_addr, index_min, index_max,
              soc->tableDmaIntrEnb));
    
    rv = SOC_E_TIMEOUT;
    if (soc->tableDmaIntrEnb) {
        soc_cmicm_cmcx_intr0_enable(unit, cmc, _soc_irq_sbusdma_ch[ch]);
        if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
            if (sal_sem_take(soc->sbusDmaIntrs[cmc][ch], soc->tableDmaTimeout) < 0) {
                rv = SOC_E_TIMEOUT;
            }
        } else {
            if (sal_sem_take(soc->tableDmaIntr, soc->tableDmaTimeout) < 0) {
                rv = SOC_E_TIMEOUT;
            }
        }
#ifdef PRINT_DMA_TIME
        diff_time = SAL_USECS_SUB(sal_time_usecs(), start_time);
        LOG_VERBOSE(BSL_LS_SOC_DMA,
                    (BSL_META_U(unit,
                                "HW dma read time: %d usecs, [%d nsecs per op]\n"),
                     diff_time, diff_time*1000/count));
#endif
        soc_cmicm_cmcx_intr0_disable(unit, cmc, _soc_irq_sbusdma_ch[ch]);

        rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch));
        if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                              rval, DONEf)) {
            rv = SOC_E_NONE;
            if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                                  rval, ERRORf)) {
                rv = SOC_E_FAIL;
                _soc_sbusdma_curr_op_details(unit, cmc, ch);
            }
        }
    } else {
        soc_timeout_t to;
        soc_timeout_init(&to, soc->tableDmaTimeout, 0);
        do {
            rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch));
            if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                                  rval, DONEf)) {
                rv = SOC_E_NONE;
#ifdef PRINT_DMA_TIME
                diff_time = SAL_USECS_SUB(sal_time_usecs(), start_time);
                LOG_VERBOSE(BSL_LS_SOC_DMA,
                            (BSL_META_U(unit,
                                        "HW dma read poll time: %d usecs, [%d nsecs per op]\n"), 
                             diff_time, diff_time*1000/count));
#endif
                if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                                      rval, ERRORf)) {
                    rv = SOC_E_FAIL;
                    _soc_sbusdma_curr_op_details(unit, cmc, ch);
                }   
                break;
            }       
        } while(!(soc_timeout_check(&to)));
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_etu_support)) {
        if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU) || 
            ((mem == EXT_L2_ENTRY_1m) || (mem == EXT_L2_ENTRY_2m))) {
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ETU_TX_REQ_FIFO_CTLr, REG_PORT_ANY,
                                        CP_ACC_THRf, wa_val));
        }
    }
#endif

    if (rv != SOC_E_NONE) {
        if (rv != SOC_E_TIMEOUT) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "%s: %s.%s failed(ERR)\n"),
                       FUNCTION_NAME(), SOC_MEM_UFNAME(unit, mem),
                       SOC_BLOCK_NAME(unit, copyno)));
            _soc_sbusdma_error_details(unit,
                soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch)));
        } else { /* Timeout cleanup */
            soc_timeout_t to;

            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "%s: %s.%s %s timeout\n"),
                       FUNCTION_NAME(), SOC_MEM_UFNAME(unit, mem),
                       SOC_BLOCK_NAME(unit, copyno),
                       soc->tableDmaIntrEnb ? "interrupt" : "polling"));
#ifdef BCM_88650_A0
            if (SOC_IS_ARAD(unit)) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "%s: This DMA failure may be due to wrong PCI configuration. "
                                      "Timeout configured to %dus.\n"), FUNCTION_NAME(), soc->tableDmaTimeout));
            }
#endif /* BCM_88650_A0 */

            /* Abort Table DMA */
            ctrl = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch));
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl, ABORTf, 1);
            soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch), ctrl);

            /* Check the done bit to confirm */
            soc_timeout_init(&to, soc->tableDmaTimeout, 0);
            while (1) {
                rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch));
                if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr, 
                    rval, DONEf)) {
                    break;
                }
                if (soc_timeout_check(&to)) {
                    LOG_ERROR(BSL_LS_SOC_COMMON,
                              (BSL_META_U(unit,
                                          "_soc_mem_sbusdma_read: Abort Failed\n")));
                    break;
                }
            }
        }
    }

    soc_cm_sinval(unit, (void *)buffer, WORDS2BYTES(data_beats) * count);
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_esm_correction) &&
    	  ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) || 
         (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU))) {
        SOC_ESM_UNLOCK(unit);
    }
#endif
    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        SBUS_DMA_UNLOCK(unit, cmc, ch);
    } else {
        TABLE_DMA_UNLOCK(unit);
    }

    return rv;
}

/*
 * Function:
 *     _soc_mem_sbusdma_write
 * Purpose:
 *      DMA acceleration for soc_mem_write_range() on FB/ER
 * Parameters:
 *      buffer -- must be pointer to sufficiently large
 *                DMA-able block of memory
 *      index_begin <= index_end - Forward direction
 *      index_begin >  index_end - Reverse direction
 *      mem_clear -- if TRUE: simply clear memory using one
 *                   mem chunk from the supplied buffer 
 */

int
_soc_mem_array_sbusdma_write(int unit, soc_mem_t mem, unsigned array_index_start, 
                             unsigned array_index_end, int copyno,
                             int index_begin, int index_end, 
                             void *buffer, uint8 mem_clear, 
                             int clear_buf_ent)
{
    soc_control_t *soc = SOC_CONTROL(unit);
    uint32        start_addr;
    uint32        count = 0;
    uint32        data_beats;
    uint32        spacing;
    int           rv = SOC_E_NONE;
    uint32        ctrl, rval;
    uint8         at, rev_slam = 0;
    int           ch;
    int           cmc = SOC_PCI_CMC(unit);
    schan_msg_t   msg;
    int           dst_blk, acc_type, data_byte_len, dma;
    unsigned      array_index;
    int           dma_retries=0;
    int           max_retries=0;

#ifdef PRINT_DMA_TIME
    sal_usecs_t   start_time;
    int           diff_time;
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
    uint32        wa_rval, wa_val = 0;
#endif

    LOG_INFO(BSL_LS_SOC_DMA,
             (BSL_META_U(unit,
                         "_soc_mem_sbusdma_write: unit %d"
                         " mem %s[%u-%u].%s index %d-%d buffer %p\n"),
              unit, SOC_MEM_UFNAME(unit, mem), array_index_start, array_index_end, 
              SOC_BLOCK_NAME(unit, copyno), index_begin, index_end, buffer));

    data_beats = soc_mem_entry_words(unit, mem);

    ch = soc->tslam_ch;

#if defined(BCM_CALADAN3_SUPPORT)
    if (SOC_IS_CALADAN3(unit)) {
        soc_sbx_caladan3_sbusdma_cmc_ch_map(unit, mem, SOC_SBUSDMA_TYPE_SLAM, &cmc, &ch);
    }
#endif

    assert((ch >= 0) && (ch < soc->max_sbusdma_channels));

    if (index_begin > index_end) {
        count = index_begin - index_end + 1;
        rev_slam = 1;
    } else {
        count = index_end - index_begin + 1;
    }
    if (count < 1) {
        return SOC_E_NONE;
    }

    schan_msg_clear(&msg);
    acc_type = SOC_MEM_ACC_TYPE(unit, mem);
    dst_blk = SOC_BLOCK2SCH(unit, copyno);
    data_byte_len = data_beats * sizeof(uint32);

    if (SOC_IS_ARAD(unit) || SOC_IS_CALADAN3(unit)) {
        dma = 1;
    } else {
        dma = 0;
    }

    soc_schan_header_cmd_set(unit, &msg.header, WRITE_MEMORY_CMD_MSG,
                             dst_blk, 0, acc_type, data_byte_len, dma, 0);  

    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        SBUS_DMA_LOCK(unit, cmc, ch);
    } else {
        TSLAM_DMA_LOCK(unit);
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_esm_correction) &&
    	  ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) || 
         (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU))) {
        SOC_ESM_LOCK(unit);
    }
#endif

    for (array_index = array_index_start; array_index <= array_index_end; ++array_index) {
        start_addr = soc_mem_addr_get(unit, mem, array_index, copyno, index_begin, &at);

        ctrl = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch));
        /* Set mode, clear abort, start (clears status and errors) */
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl,
                          MODEf, 0);
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl,
                          ABORTf, 0);
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl,
                          STARTf, 0);
        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch), ctrl);
        /* Set 1st schan ctrl word as opcode */
        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_OPCODE(cmc, ch), msg.dwords[0]);
        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_HOSTADDR(cmc, ch), 
                      soc_cm_l2p(unit, buffer));
        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_ADDRESS(cmc, ch), start_addr);
        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_COUNT(cmc, ch), count);
        
        /* Program beats, increment/decrement, single etc */
        rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_REQUEST(cmc, ch));
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                          REQ_WORDSf, data_beats);

#if defined(BCM_CALADAN3_SUPPORT)
        if (SOC_IS_CALADAN3(unit)) {
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                              REP_WORDSf, 0);
            
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                              INCR_SHIFTf, 0);
            /* Caladan3 chip uses a fast polling technique to increase performance */
            max_retries = SOC_SBUSDMA_FP_RETRIES;
        }
#endif

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (soc_feature(unit, soc_feature_etu_support)) {
            if (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU) {
                int index0, index1, increment;
                soc_mem_t real_mem;
        
                soc_tcam_mem_index_to_raw_index(unit, mem, 0, &real_mem, &index0);
                soc_tcam_mem_index_to_raw_index(unit, mem, 1, &real_mem, &index1);
                increment = _shr_popcount(index1 - index0 - 1);
                soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                                INCR_SHIFTf, increment);
                SOC_IF_ERROR_RETURN(READ_ETU_TX_REQ_FIFO_CTLr(unit, &wa_rval));
                wa_val = soc_reg_field_get(unit, ETU_TX_REQ_FIFO_CTLr, wa_rval, 
                                           CP_ACC_THRf);
                soc_reg_field_set(unit, ETU_TX_REQ_FIFO_CTLr, &wa_rval,
                                  CP_ACC_THRf, 0);
                SOC_IF_ERROR_RETURN(WRITE_ETU_TX_REQ_FIFO_CTLr(unit, wa_rval));
            } else {
                soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                                  INCR_SHIFTf, 0);
                if ((mem == EXT_L2_ENTRY_1m) || (mem == EXT_L2_ENTRY_2m)) {
                    SOC_IF_ERROR_RETURN(READ_ETU_TX_REQ_FIFO_CTLr(unit, &wa_rval));
                    wa_val = soc_reg_field_get(unit, ETU_TX_REQ_FIFO_CTLr, wa_rval, 
                                               CP_ACC_THRf);
                    soc_reg_field_set(unit, ETU_TX_REQ_FIFO_CTLr, &wa_rval,
                                      CP_ACC_THRf, 0);
                    SOC_IF_ERROR_RETURN(WRITE_ETU_TX_REQ_FIFO_CTLr(unit, wa_rval));
                }
            }
        } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
#if (defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT))
        if (SOC_IS_DPP(unit) || SOC_IS_DFE(unit)) {
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                              INCR_SHIFTf, 0);
        } else
#endif
        {
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                              INCR_SHIFTf, 0);
        }
        if (mem_clear) {
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                              REQ_SINGLEf, 1);
        } else {
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                              REQ_SINGLEf, 0);
        }
        if (rev_slam) {
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                              DECRf, 1);
        } else {
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                              DECRf, 0);
        }
        if (soc_feature(unit, soc_feature_multi_sbus_cmds)) {
            
            if (soc->sbusCmdSpacing < 0) {
                spacing = data_beats > 7 ? data_beats + 1 : 8;
            } else {
                spacing = soc->sbusCmdSpacing;
            }
            if (!((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_IPIPE) ||
                 (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_EPIPE))) {
                spacing = 0;
            }
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &rval,
                              PEND_CLOCKSf, spacing);
        }
        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_REQUEST(cmc, ch), rval);
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl, STARTf, 1);
        /* Start DMA */
#ifdef PRINT_DMA_TIME
        start_time = sal_time_usecs();
#endif
        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch), ctrl);
        
        LOG_INFO(BSL_LS_SOC_DMA,
                 (BSL_META_U(unit,
                             "_soc_mem_sbusdma_write: %d entries %d beats "
                             "addr 0x%x (index %d-%d) Interrupt-Mode(%d)\n"),
                  count, data_beats, start_addr, 
                  index_begin, index_end,
                  soc->tslamDmaIntrEnb));
        
        if (mem_clear) {
            /* clear_buf_ent should be multiple of 4 and >0 */
            soc_cm_sflush(unit, buffer, WORDS2BYTES(data_beats) * clear_buf_ent);
        } else {
            soc_cm_sflush(unit, buffer, WORDS2BYTES(data_beats) * count);
        }
        
        rv = SOC_E_TIMEOUT;
        if (soc->tslamDmaIntrEnb) {
            soc_cmicm_cmcx_intr0_enable(unit, cmc, _soc_irq_sbusdma_ch[ch]);
            if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
                if (sal_sem_take(soc->sbusDmaIntrs[cmc][ch], soc->tslamDmaTimeout) < 0) {
                    rv = SOC_E_TIMEOUT;
                }    
            } else {
                if (sal_sem_take(soc->tslamDmaIntr, soc->tslamDmaTimeout) < 0) {
                    rv = SOC_E_TIMEOUT;
                }
            }
#ifdef PRINT_DMA_TIME
            diff_time = SAL_USECS_SUB(sal_time_usecs(), start_time);
            LOG_VERBOSE(BSL_LS_SOC_DMA,
                        (BSL_META_U(unit,
                                    "HW dma write time: %d usecs, [%d nsecs per op]\n"),
                         diff_time, diff_time*1000/count));
#endif
            soc_cmicm_cmcx_intr0_disable(unit, cmc, _soc_irq_sbusdma_ch[ch]);
        
            rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch));
            if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                                  rval, DONEf)) {
                rv = SOC_E_NONE;
                if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                                      rval, ERRORf)) {
                    rv = SOC_E_FAIL;
                    _soc_sbusdma_curr_op_details(unit, cmc, ch);
                }
            }
        } else {
            soc_timeout_t to;
            soc_timeout_init(&to, soc->tslamDmaTimeout, 0);
            /* Accessing the memory location prior to read loop improves performance */
            rval=soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch));
            do {
                rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch));
                if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                                      rval, DONEf)) {
#ifdef PRINT_DMA_TIME
                    diff_time = SAL_USECS_SUB(sal_time_usecs(), start_time);
                    LOG_VERBOSE(BSL_LS_SOC_DMA,
                                (BSL_META_U(unit,
                                            "HW dma write poll time: %d usecs, [%d nsecs per op]\n"), 
                                 diff_time, diff_time*1000/count));
#endif
                    rv = SOC_E_NONE;
                    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                                          rval, ERRORf)) {
                        rv = SOC_E_FAIL;
                        _soc_sbusdma_curr_op_details(unit, cmc, ch);
                    }
                    break;
                }
            } while(dma_retries++ < max_retries || !(soc_timeout_check(&to)));
        }

    
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (soc_feature(unit, soc_feature_etu_support)) {
            if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU) || 
                ((mem == EXT_L2_ENTRY_1m) || (mem == EXT_L2_ENTRY_2m))) {
                SOC_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, ETU_TX_REQ_FIFO_CTLr, REG_PORT_ANY,
                                            CP_ACC_THRf, wa_val));
            }
        }
#endif

        if (rv < 0) {
            if (rv != SOC_E_TIMEOUT) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "%s: %s[%u].%s failed(ERR)\n"),
                           FUNCTION_NAME(), SOC_MEM_UFNAME(unit, mem), array_index,
                           SOC_BLOCK_NAME(unit, copyno)));
                _soc_sbusdma_error_details(unit,
                    soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch)));
            } else { /* Timeout cleanup */
                soc_timeout_t to;
        
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "%s: %s[%u].%s %s timeout\n"),
                           FUNCTION_NAME(), SOC_MEM_UFNAME(unit, mem), array_index,
                           SOC_BLOCK_NAME(unit, copyno),
                           soc->tslamDmaIntrEnb ? "interrupt" : "polling"));
                
                /* Abort Table DMA */
                ctrl = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch));
                soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl, ABORTf, 1);
                soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch), ctrl);
        
                /* Check the done bit to confirm */
                soc_timeout_init(&to, soc->tslamDmaTimeout, 0);
                while (1) {
                    rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch));
                    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr, 
                        rval, DONEf)) {
                        break;
                    }
                    if (soc_timeout_check(&to)) {
                        LOG_ERROR(BSL_LS_SOC_COMMON,
                                  (BSL_META_U(unit,
                                              "_soc_mem_sbusdma_write: Abort Failed\n")));
                        break;
                    }
                }            
            }
        }
    } /* end of loop over array indices */
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_esm_correction) &&
    	  ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) || 
         (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU))) {
        SOC_ESM_UNLOCK(unit);
    }
#endif
    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        SBUS_DMA_UNLOCK(unit, cmc, ch);
    } else {
        TSLAM_DMA_UNLOCK(unit);
    }

    return rv;
}

/*
 * Function:
 *     _soc_mem_sbusdma_write
 * Purpose:
 *      DMA acceleration for soc_mem_write_range() on FB/ER
 * Parameters:
 *      buffer -- must be pointer to sufficiently large
 *                DMA-able block of memory
 *      index_begin <= index_end - Forward direction
 *      index_begin >  index_end - Reverse direction
 *      mem_clear -- if TRUE: simply clear memory using one
 *                   mem chunk from the supplied buffer 
 */

int
_soc_mem_sbusdma_write(int unit, soc_mem_t mem, int copyno,
                       int index_begin, int index_end, 
                       void *buffer, uint8 mem_clear, 
                       int clear_buf_ent)
{
    return _soc_mem_array_sbusdma_write(unit, mem, 0, 0, copyno, index_begin,
                                        index_end, buffer, mem_clear,
                                        clear_buf_ent);
}

/*
 * Function:
 *     _soc_mem_sbusdma_clear
 * Purpose:
 *     clear a specific memory/table region using DMA write (slam) acceleration.
 */
int
_soc_mem_sbusdma_clear_specific(int unit, soc_mem_t mem,
                                unsigned array_index_min, unsigned array_index_max,
                                int copyno,
                                int index_min, int index_max,
                                void *null_entry)
{
    int    rv = 0, chunk_size, chunk_entries, mem_size, entry_words;
    int    index, blk, tmp;
    uint32 *buf;

    if (SOC_WARM_BOOT(unit) || SOC_IS_RELOADING(unit)) {
        return SOC_E_NONE;
    }
    chunk_size = SOC_MEM_CLEAR_CHUNK_SIZE_GET(unit);  
    /* get legal values for indices, if too small/big use the memory's boundaries */
    tmp = soc_mem_index_min(unit, mem);
    if (index_min < soc_mem_index_min(unit, mem)) {
      index_min = tmp;
    }
    if (index_max < index_min) {
        index_max = index_min;
    } else {
         tmp = soc_mem_index_max(unit, mem);
         if (index_max > tmp) {
             index_max = tmp;
         }
    }

    entry_words = soc_mem_entry_words(unit, mem);
    mem_size = (index_max - index_min + 1) * entry_words * 4;
    if (mem_size < chunk_size) {
        chunk_size = mem_size;
    }
#ifdef BCM_88650_A0
    if (SOC_IS_ARAD(unit) && (entry_words * 4 > chunk_size)) {
        /* The default is to use a one table entry buffer, if not given a bigger chunk size */
        chunk_size = entry_words * 4; /* make sure the buffer size holds at least one table entry */
    }
#endif

    buf = soc_cm_salloc(unit, chunk_size, "mem_clear_buf");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }
    chunk_entries = chunk_size / (entry_words * 4);
    for (index = 0; index < chunk_entries; index++) {
        sal_memcpy(buf + (index * entry_words),
                   null_entry, entry_words * 4);
    }

    /* get legal values for memory array indices */
    if (SOC_MEM_IS_ARRAY(unit, mem)) {
        soc_mem_array_info_t *maip = SOC_MEM_ARRAY_INFOP(unit, mem);
        if (maip) {
            if (array_index_max >= maip->numels) {
                array_index_max = maip->numels - 1;
            }
        } else {
            array_index_max = 0;
        }
        if (array_index_min > array_index_max) {
            array_index_min = array_index_max;
        }
    } else {
        array_index_min = array_index_max = 0;
    }

    MEM_LOCK(unit, mem);
    SOC_MEM_BLOCK_ITER(unit, mem, blk) {
        if (copyno != COPYNO_ALL && copyno != blk) {
            continue;
        }
        rv = _soc_mem_array_sbusdma_write(unit, mem, array_index_min, array_index_max, blk, index_min, 
                                     index_max, buf, TRUE, chunk_entries);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "soc_mem_sbusdma_clear: %s.%s[%d-%d] failed: %s\n"),
                       SOC_MEM_UFNAME(unit, mem),
                       SOC_BLOCK_NAME(unit, blk),
                       index_min, index_max, soc_errmsg(rv)));
        }
    }
    MEM_UNLOCK(unit, mem);
    soc_cm_sfree(unit, buf);
    return rv;
}


/*
 * Function:
 *     _soc_mem_sbusdma_clear
 * Purpose:
 *     soc_mem_clear acceleration using table DMA write (slam)
 */
int
_soc_mem_sbusdma_clear(int unit, soc_mem_t mem, int copyno, void *null_entry)
{
    return _soc_mem_sbusdma_clear_specific(unit, mem, 0, UINT_MAX, copyno, INT_MIN, INT_MAX, null_entry);
}

/* SBUSDMA descriptor mode */
STATIC void
_soc_sbusdma_desc(void *unit_vp)
{
    int rv, i;
    int unit = PTR_TO_INT(unit_vp);
    soc_control_t *soc = SOC_CONTROL(unit);
    sal_usecs_t interval;
    sal_usecs_t stime, etime;
    uint32 ctrl, rval;
    int cmc = SOC_PCI_CMC(unit);
    int ch = soc->desc_ch;    
    _soc_sbusdma_state_t *swd;
    int big_pio, big_packet, big_other;
    
#if defined(BCM_CALADAN3_SUPPORT)
    if (SOC_IS_CALADAN3(unit)) {
        soc_sbx_caladan3_sbusdma_cmc_ch_map(unit, INVALIDm, SOC_SBUSDMA_TYPE_DESC, &cmc, &ch);
    }
#endif

    assert((ch >= 0) && (ch < soc->max_sbusdma_channels));
    soc_cm_get_endian(unit, &big_pio, &big_packet, &big_other);
    
    while((interval = SOC_SBUSDMA_DM_TO(unit)) != 0) {
        /* wait for run indication */
        (void)sal_sem_take(SOC_SBUSDMA_DM_INTR(unit), sal_sem_FOREVER);
        if (!SOC_SBUSDMA_DM_TO(unit)) {
            goto cleanup_exit;
        }
        if (!SOC_SBUSDMA_DM_ACTIVE(unit) || !SOC_SBUSDMA_DM_WORKING(unit)) {
            continue;
        }
        rv = SOC_E_TIMEOUT;
        LOG_VERBOSE(BSL_LS_SOC_DMA,
                    (BSL_META_U(unit,
                                "_soc_sbusdma_desc: Process \n")));
        stime = sal_time_usecs();
        SOC_SBUSDMA_DM_LOCK(unit);
        swd = SOC_SBUSDMA_DM_WORKING(unit);
        
        ctrl = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch));
        if ( (!soc_feature(unit, soc_feature_iproc)) ||
             ( soc_feature(unit, soc_feature_iproc) && !(soc_cm_get_bus_type(unit) & SOC_PCI_DEV_TYPE)) )
        {
            /* Set endianess, mode, clear abort, start (clears status and errors) */
            if (big_other) {
                soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl,
                                  DESCRIPTOR_ENDIANESSf, 1);
            }
        }
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl,
                          MODEf, 1);
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl,
                          ABORTf, 0);
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl,
                          STARTf, 0);
        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch), ctrl);
        
        /* write desc address */
        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_DESCADDR(cmc, ch), 
                      soc_cm_l2p(unit, swd->desc));
        
        /* Debug stuff */
        LOG_VERBOSE(BSL_LS_SOC_DMA,
                    (BSL_META_U(unit,
                                "Count: %d\n"), swd->ctrl.cfg_count));
        for (i=0; i<swd->ctrl.cfg_count; i++) {
            LOG_VERBOSE(BSL_LS_SOC_DMA,
                        (BSL_META_U(unit,
                                    "cntrl: %08x, req: %08x, count: %08x, "
                                    "opcode: %08x, saddr: %08x, haddr: %08x\n"),
                         swd->desc[i].cntrl, swd->desc[i].req, swd->desc[i].count,
                         swd->desc[i].opcode, swd->desc[i].addr, swd->desc[i].hostaddr));
        }
        
        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl, STARTf, 1);
        /* Start DMA */
        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch), ctrl);    

        if (SOC_SBUSDMA_DM_INTRENB(unit)) {
            soc_cmicm_intr0_enable(unit, _soc_irq_sbusdma_ch[ch]);
            if (sal_sem_take(SOC_SBUSDMA_DM_INTR(unit), 
                             SOC_SBUSDMA_DM_TO(unit)) < 0) {
                rv = SOC_E_TIMEOUT;
            }
            soc_cmicm_intr0_disable(unit, _soc_irq_sbusdma_ch[ch]);
        
            rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch));
            if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                                  rval, DONEf)) {
                rv = SOC_E_NONE;
                if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                                      rval, ERRORf)) {
                    rv = SOC_E_FAIL;
                    _soc_sbusdma_curr_op_details(unit, cmc, ch);
                }
            }
        } else {
            soc_timeout_t to;
            soc_timeout_init(&to, SOC_SBUSDMA_DM_TO(unit), 0);
            rv = SOC_E_TIMEOUT;
            do {
                rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch));
                
                if (SAL_BOOT_PLISIM && !SAL_BOOT_BCMSIM) {
                    rv = SOC_E_NONE;
                    break;
                }
                if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                                      rval, DONEf)) {
                    rv = SOC_E_NONE;
                    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr,
                                          rval, ERRORf)) {
                        rv = SOC_E_FAIL;
                        _soc_sbusdma_curr_op_details(unit, cmc, ch);
                    }
                    break;
                }
            } while(!(soc_timeout_check(&to)));
        }
        
        if (rv != SOC_E_NONE) {
            if (rv != SOC_E_TIMEOUT) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "%s: %s failed(ERR)\n"),
                           FUNCTION_NAME(), swd->ctrl.name));
                _soc_sbusdma_error_details(unit, 
                    soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch)));
            } else { /* Timeout cleanup */
                soc_timeout_t to;
        
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "%s: %s %s timeout\n"),
                           FUNCTION_NAME(), swd->ctrl.name,
                           SOC_SBUSDMA_DM_INTRENB(unit) ? "interrupt" : "polling"));
            
                /* Abort DMA */
                ctrl = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch));
                soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &ctrl, ABORTf, 1);
                soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, ch), ctrl);
                
                /* Check the done bit to confirm */
                soc_timeout_init(&to, SOC_SBUSDMA_DM_TO(unit), 0);
                while (1) {
                    rval = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_STATUS(cmc, ch));
                    if (soc_reg_field_get(unit, CMIC_CMC0_SBUSDMA_CH0_STATUSr, 
                        rval, DONEf)) {
                        break;
                    }
                    if (soc_timeout_check(&to)) {
                        LOG_ERROR(BSL_LS_SOC_COMMON,
                                  (BSL_META_U(unit,
                                              "_soc_sbusdma_desc: Abort Failed\n")));
                        break;
                    }
                }
            }
        } else {
            etime = sal_time_usecs();
            LOG_VERBOSE(BSL_LS_SOC_DMA,
                        (BSL_META_U(unit,
                                    "_soc_sbusdma_desc: unit=%d mode(%s) done in %d usec\n"),
                         unit, SOC_SBUSDMA_DM_INTRENB(unit) ? "interrupt" : "polling",
                         SAL_USECS_SUB(etime, stime)));
        }
        /* execute callback */
        swd->status = 0;
        swd->ctrl.cb(unit, rv, swd->handle, swd->ctrl.data);
        SOC_SBUSDMA_DM_ACTIVE(unit) = 0;
        SOC_SBUSDMA_DM_UNLOCK(unit);
    }
cleanup_exit:

    /* cleanup the any pending/aborted descriptor */
    if (SOC_SBUSDMA_DM_ACTIVE(unit) && SOC_SBUSDMA_DM_WORKING(unit)) {
        SOC_SBUSDMA_DM_LOCK(unit);
        swd = SOC_SBUSDMA_DM_WORKING(unit);
        swd->status = 0;
        SOC_SBUSDMA_DM_ACTIVE(unit) = 0;
        SOC_SBUSDMA_DM_UNLOCK(unit);
    }

    LOG_INFO(BSL_LS_SOC_DMA,
             (BSL_META_U(unit,
                         "_soc_sbusdma_desc: exiting\n")));
    SOC_SBUSDMA_DM_PID(unit) = SAL_THREAD_ERROR;
    sal_thread_exit(0);
}

int
soc_sbusdma_desc_abort(int unit)
{
    soc_timeout_t to;
    /* signal abort */
    SOC_SBUSDMA_DM_TO(unit) = 0;
    if (SOC_SBUSDMA_DM_PID(unit) != SAL_THREAD_ERROR) {
        /* Wake up thread so it will check the exit flag */
        sal_sem_give(SOC_SBUSDMA_DM_INTR(unit));

        /* Give thread a few seconds to wake up and exit */
        if (SAL_BOOT_SIMULATION) {
            soc_timeout_init(&to, 50 * 1000000, 0);
        } else {
            soc_timeout_init(&to, 10 * 1000000, 0);
        }

        while (SOC_SBUSDMA_DM_PID(unit) != SAL_THREAD_ERROR) {
            if (soc_timeout_check(&to)) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "soc_sbusdma_desc_detach: SBUDMA Desc Mode thread will not exit\n")));
                return SOC_E_INTERNAL;
            }
        }
    }
    return SOC_E_NONE;
}

int
soc_sbusdma_desc_detach(int unit)
{
    int i, rv = SOC_E_NONE;
    _soc_sbusdma_state_t *swd; 
    
    if (!SOC_SBUSDMA_DM_INFO(unit)) {
        return rv;
    }
    SOC_SBUSDMA_DM_INIT(unit) = 2;
    /* Check if something is running and wait for it to complete (or issue an abort ?) */
    if (soc_sbusdma_desc_abort(unit)) {
        return SOC_E_INTERNAL;
    }
    
    for (i=1; i<SOC_SBUSDMA_MAX_DESC; i++) {
        if (SOC_SBUSDMA_DM_HANDLES(unit)[i]) {
            swd = SOC_SBUSDMA_DM_HANDLES(unit)[i];
            sal_free(swd->cfg);
            if (!(swd->ctrl.flags & SOC_SBUSDMA_CFG_USE_SUPPLIED_DESC)) {
                soc_cm_sfree(unit, swd->desc);
            }
            sal_free(swd);
            SOC_SBUSDMA_DM_HANDLES(unit)[i] = 0;
            SOC_SBUSDMA_DM_COUNT(unit)--;
        }
    }
    
    if (SOC_SBUSDMA_DM_MUTEX(unit)) {
        sal_mutex_destroy(SOC_SBUSDMA_DM_MUTEX(unit));
        SOC_SBUSDMA_DM_MUTEX(unit) = NULL;
    }
    if (SOC_SBUSDMA_DM_INTR(unit)) {
        sal_sem_destroy(SOC_SBUSDMA_DM_INTR(unit));
        SOC_SBUSDMA_DM_INTR(unit) = NULL;
    }
    SOC_SBUSDMA_DM_INIT(unit) = 0;
    sal_free(SOC_SBUSDMA_DM_INFO(unit));
    SOC_SBUSDMA_DM_INFO(unit) = NULL;
    return rv;
}

int
soc_sbusdma_desc_init(int unit, int interval, uint8 intrEnb)
{
    int rv;
    if (SOC_SBUSDMA_DM_INFO(unit) && (SOC_SBUSDMA_DM_INIT(unit) == 2)) {
        return SOC_E_BUSY;
    }
    if (SOC_SBUSDMA_DM_INFO(unit)) {
        SOC_SBUSDMA_DM_INIT(unit) = 2;
        if ((rv = soc_sbusdma_desc_detach(unit)) != SOC_E_NONE) {
            return rv;
        }
    }    
    SOC_SBUSDMA_DM_INFO(unit) = sal_alloc(sizeof(soc_sbusdma_desc_info_t),
                                          "sbusdma dm info");
    if (SOC_SBUSDMA_DM_INFO(unit) == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(SOC_SBUSDMA_DM_INFO(unit), 0, sizeof(soc_sbusdma_desc_info_t));
    SOC_SBUSDMA_DM_INIT(unit) = 2;

    if ((SOC_SBUSDMA_DM_MUTEX(unit) = 
           sal_mutex_create("sbusdma dm lock")) == NULL) {
        (void)soc_sbusdma_desc_detach(unit);
        return SOC_E_MEMORY;
    }
    if ((SOC_SBUSDMA_DM_INTR(unit) = 
           sal_sem_create("Desc DMA interrupt", sal_sem_BINARY, 0)) == NULL) {
        (void)soc_sbusdma_desc_detach(unit);
        return SOC_E_MEMORY;
    }
    if (intrEnb) {
        SOC_SBUSDMA_DM_INTRENB(unit) = 1;
    } else {
        SOC_SBUSDMA_DM_INTRENB(unit) = 0;
    }
    SOC_SBUSDMA_DM_TO(unit) = interval ? interval : SAL_BOOT_QUICKTURN ? 
                                  30000000 : 10000000;
    sal_snprintf(SOC_SBUSDMA_DM_NAME(unit), sizeof(SOC_SBUSDMA_DM_NAME(unit)),
                 "socdmadesc.%d", unit);
    SOC_SBUSDMA_DM_PID(unit) = sal_thread_create(SOC_SBUSDMA_DM_NAME(unit),
                                                 SAL_THREAD_STKSZ,
                                                 soc_property_get(unit, spn_SBUS_DMA_DESC_THREAD_PRI, 50),
                                                 _soc_sbusdma_desc,
                                                 INT_TO_PTR(unit));
    if (SOC_SBUSDMA_DM_PID(unit) == SAL_THREAD_ERROR) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_sbusdma_desc_init: Could not start SBUDMA Desc Mode thread.\n")));
        (void)soc_sbusdma_desc_detach(unit);
        return SOC_E_MEMORY;
    }

    SOC_SBUSDMA_DM_COUNT(unit) = 0;
    SOC_SBUSDMA_DM_INIT(unit) = 1;
    return SOC_E_NONE;
}

int
soc_sbusdma_desc_create(int unit, soc_sbusdma_desc_ctrl_t *ctrl, 
                        soc_sbusdma_desc_cfg_t *cfg, 
                        sbusdma_desc_handle_t *desc_handle)
{
    uint32 i;
    _soc_sbusdma_state_t *swd = NULL;
    int big_pio, big_packet, big_other;
    schan_msg_t msg;
    int opcode, dst_blk, acc_type;
    
    if ((SOC_SBUSDMA_DM_INFO(unit) == NULL) || !SOC_SBUSDMA_DM_INIT(unit)) {
        return SOC_E_INIT;
    }
    if (!ctrl || !cfg || !ctrl->cb || !ctrl->cfg_count) {
        return SOC_E_PARAM;
    }
    soc_cm_get_endian(unit, &big_pio, &big_packet, &big_other);
    SOC_SBUSDMA_DM_LOCK(unit);
    
    
    swd = sal_alloc(sizeof(_soc_sbusdma_state_t), "_soc_sbusdma_state_t");
    if (swd == NULL) {
        SOC_SBUSDMA_DM_UNLOCK(unit);
        return SOC_E_MEMORY;
    }
    sal_memset(swd, 0, sizeof(_soc_sbusdma_state_t));
    /* save s/w ctrl and config */
    sal_memcpy(&swd->ctrl, ctrl, sizeof(soc_sbusdma_desc_ctrl_t));
    if (ctrl->cfg_count == 1) { /* single mode */
        swd->cfg = sal_alloc(sizeof(soc_sbusdma_desc_cfg_t), "soc_sbusdma_desc_cfg_t");
        if (swd->cfg == NULL) {
            sal_free(swd);
            SOC_SBUSDMA_DM_UNLOCK(unit);
            return SOC_E_MEMORY;
        }
        sal_memcpy(swd->cfg, cfg, sizeof(soc_sbusdma_desc_cfg_t));
        if (swd->ctrl.buff) { /* ctrl buff over-rides cfg buff */
            swd->cfg->buff = swd->ctrl.buff;
        }
        assert(swd->cfg->buff);
        if (!(ctrl->flags & SOC_SBUSDMA_CFG_USE_SUPPLIED_DESC) || !ctrl->hw_desc) {
            swd->desc = soc_cm_salloc(unit, sizeof(soc_sbusdma_desc_t), 
                                      "soc_sbusdma_desc_t");
            if (swd->desc == NULL) {
                sal_free(swd->cfg);
                sal_free(swd);
                SOC_SBUSDMA_DM_UNLOCK(unit);
                return SOC_E_MEMORY;
            }
            sal_memset(swd->desc, 0, sizeof(soc_sbusdma_desc_t));
            schan_msg_clear(&msg);
            opcode = (ctrl->flags & SOC_SBUSDMA_CFG_COUNTER_IS_MEM) ?
                READ_MEMORY_CMD_MSG : READ_REGISTER_CMD_MSG;
            acc_type = cfg->acc_type;
            dst_blk = cfg->blk;
            soc_schan_header_cmd_set(unit, &msg.header, opcode, dst_blk, 0,
                                     acc_type, 4, 0, 0);  

            /* Prepare h/w desc */
            swd->desc->cntrl |= SOC_SBUSDMA_CTRL_LAST;
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, 
                              &(swd->desc->req), REP_WORDSf, cfg->width);
            soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, 
                              &(swd->desc->req), INCR_SHIFTf, cfg->addr_shift);
            if ( (!soc_feature(unit, soc_feature_iproc)) ||
                 ( soc_feature(unit, soc_feature_iproc) && !(soc_cm_get_bus_type(unit) & SOC_PCI_DEV_TYPE)) )
            {
                if (big_other) {
                    soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, 
                                      &(swd->desc->req), HOSTMEMWR_ENDIANESSf, 1);
                    soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, 
                                      &(swd->desc->req), HOSTMEMRD_ENDIANESSf, 1);
                }
            }
            swd->desc->count = cfg->count;
            swd->desc->opcode = msg.dwords[0];
            swd->desc->addr = cfg->addr;
            swd->desc->hostaddr = soc_cm_l2p(unit, swd->cfg->buff);
        }
    } else { /* chain mode */
        uint32 count;
        uint8 append = 0;
        uint32 *hptr;
        if (ctrl->buff) {
            append = 1;
        }
        swd->cfg = sal_alloc(sizeof(soc_sbusdma_desc_cfg_t) * ctrl->cfg_count, 
                             "soc_sbusdma_desc_cfg_t");
        if (swd->cfg == NULL) {
            sal_free(swd);
            SOC_SBUSDMA_DM_UNLOCK(unit);
            return SOC_E_MEMORY;
        }
        sal_memcpy(swd->cfg, cfg, sizeof(soc_sbusdma_desc_cfg_t) * 
                   ctrl->cfg_count);
        if (!(ctrl->flags & SOC_SBUSDMA_CFG_USE_SUPPLIED_DESC) || !ctrl->hw_desc) {
            swd->desc = soc_cm_salloc(unit, sizeof(soc_sbusdma_desc_t) * 
                                      ctrl->cfg_count, "soc_sbusdma_desc_t");
            if (swd->desc == NULL) {
                sal_free(swd->cfg);
                sal_free(swd);
                SOC_SBUSDMA_DM_UNLOCK(unit);
                return SOC_E_MEMORY;
            }
            sal_memset(swd->desc, 0, sizeof(soc_sbusdma_desc_t) * 
                       ctrl->cfg_count);
            hptr = (uint32*)ctrl->buff;
            for (count = 0; count < ctrl->cfg_count; count++) {
                schan_msg_clear(&msg);
                opcode = (ctrl->flags & SOC_SBUSDMA_CFG_COUNTER_IS_MEM) ?
                    READ_MEMORY_CMD_MSG : READ_REGISTER_CMD_MSG;
                acc_type = cfg[count].acc_type;
                dst_blk = cfg[count].blk;
                soc_schan_header_cmd_set(unit, &msg.header, opcode,
                                         dst_blk, 0, acc_type, 4, 0, 0);  

                /* Prepare h/w desc */
                if (append && count) {
                    swd->desc[count].cntrl |= SOC_SBUSDMA_CTRL_APND;
                }
                if (count == ctrl->cfg_count-1) {
                    swd->desc[count].cntrl |= SOC_SBUSDMA_CTRL_LAST;
                }
                soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, 
                                  &(swd->desc[count].req), REP_WORDSf, 
                                  cfg[count].width);
                soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, 
                                  &(swd->desc[count].req), INCR_SHIFTf, 
                                  cfg[count].addr_shift);
                if ( (!soc_feature(unit, soc_feature_iproc)) ||
                     ( soc_feature(unit, soc_feature_iproc) && !(soc_cm_get_bus_type(unit) & SOC_PCI_DEV_TYPE)) )
                {
                    if (big_other) {
                        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, 
                                          &(swd->desc[count].req), 
                                          HOSTMEMWR_ENDIANESSf, 1);
                        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, 
                                          &(swd->desc[count].req), 
                                          HOSTMEMRD_ENDIANESSf, 1);
                    }
                }
                swd->desc[count].count = cfg[count].count;
                swd->desc[count].opcode = msg.dwords[0];
                swd->desc[count].addr = cfg[count].addr;
                if (append) {
                    if (count) {
                        
                        swd->desc[count].hostaddr = soc_cm_l2p(unit, hptr);
                    } else {
                        swd->desc[count].hostaddr = soc_cm_l2p(unit, hptr);
                    }
                    hptr += (cfg[count].count * cfg[count].width);
                } else {
                    swd->desc[count].hostaddr = soc_cm_l2p(unit, cfg[count].buff);
                }
            }
        }
    }
    /* find a free handle */
    for (i=1; i<SOC_SBUSDMA_MAX_DESC;) {
        if (SOC_SBUSDMA_DM_HANDLES(unit)[i] == 0) {
            break;
        }
        i++;
    }
    *desc_handle = i; 
    swd->handle = *desc_handle;
    SOC_SBUSDMA_DM_HANDLES(unit)[i] = swd;
    SOC_SBUSDMA_DM_COUNT(unit)++;
    SOC_SBUSDMA_DM_UNLOCK(unit);
    if (ctrl->cfg_count == 1) {
        LOG_INFO(BSL_LS_SOC_DMA,
                 (BSL_META_U(unit,
                             "Create Single:: Handle: %d, desc count: %d, addr: %x, "
                             "opcount: %d, buff: %p\n"), 
                  swd->handle, swd->ctrl.cfg_count, 
                  swd->desc->addr, swd->desc->count, swd->cfg->buff));
    } else {
        LOG_INFO(BSL_LS_SOC_DMA,
                 (BSL_META_U(unit,
                             "Create Chain:: Handle: %d, desc count: %d\n"), 
                  swd->handle, swd->ctrl.cfg_count));
    }
    
    LOG_INFO(BSL_LS_SOC_DMA,
             (BSL_META_U(unit,
                         "SBD DM count: %d\n"), SOC_SBUSDMA_DM_COUNT(unit)));
    return SOC_E_NONE;
}

int
soc_sbusdma_desc_get_state(int unit, sbusdma_desc_handle_t desc_handle, uint8 *state)
{
    _soc_sbusdma_state_t *swd;
    
    if ((SOC_SBUSDMA_DM_INFO(unit) == NULL) || !SOC_SBUSDMA_DM_INIT(unit) ||
        !SOC_SBUSDMA_DM_COUNT(unit)) {
        return SOC_E_INIT;
    }
    SOC_SBUSDMA_DM_LOCK(unit);
    if ((desc_handle > 0) && (desc_handle <= SOC_SBUSDMA_MAX_DESC) 
        && SOC_SBUSDMA_DM_HANDLES(unit)[desc_handle]) {
        swd = SOC_SBUSDMA_DM_HANDLES(unit)[desc_handle];
    } else {
        SOC_SBUSDMA_DM_UNLOCK(unit);
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Get state request for invalid or non-existing descriptor handle: %d\n"), 
                   desc_handle));
        return SOC_E_PARAM;
    }
    if (swd->handle != desc_handle) {
        SOC_SBUSDMA_DM_UNLOCK(unit);
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Handle mismatch found: %d<=>%d\n"), swd->handle,
                   desc_handle));
        return SOC_E_INTERNAL;
    }
    *state = swd->status;
    SOC_SBUSDMA_DM_UNLOCK(unit);
    return SOC_E_NONE;
}

int
soc_sbusdma_desc_run(int unit, sbusdma_desc_handle_t desc_handle)
{
    _soc_sbusdma_state_t *swd;
    
    if ((SOC_SBUSDMA_DM_INFO(unit) == NULL) || !SOC_SBUSDMA_DM_INIT(unit) ||
        !SOC_SBUSDMA_DM_COUNT(unit)) {
        return SOC_E_INIT;
    }
    SOC_SBUSDMA_DM_LOCK(unit);
    if (SOC_SBUSDMA_DM_ACTIVE(unit)) {
        SOC_SBUSDMA_DM_UNLOCK(unit);
        return SOC_E_BUSY;
    }
    if ((desc_handle > 0) && (desc_handle <= SOC_SBUSDMA_MAX_DESC) 
        && SOC_SBUSDMA_DM_HANDLES(unit)[desc_handle]) {
        swd = SOC_SBUSDMA_DM_HANDLES(unit)[desc_handle];
    } else {
        SOC_SBUSDMA_DM_UNLOCK(unit);
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Run request for invalid or non-existing descriptor handle: %d\n"), 
                   desc_handle));
        return SOC_E_PARAM;
    }
    if (swd->handle != desc_handle) {
        SOC_SBUSDMA_DM_UNLOCK(unit);
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Handle mismatch found: %d<=>%d\n"), swd->handle,
                   desc_handle));
        return SOC_E_INTERNAL;
    }
    /* process swd->desc */
    swd->status = 1;
    SOC_SBUSDMA_DM_ACTIVE(unit) = 1;
    SOC_SBUSDMA_DM_WORKING(unit) = swd;
    
    if (swd->ctrl.cfg_count == 1) {
        LOG_INFO(BSL_LS_SOC_DMA,
                 (BSL_META_U(unit,
                             "Run Single:: Handle: %d, desc count: %d, addr: %x, "
                             "opcount: %d, buff: %p\n"), 
                  swd->handle, swd->ctrl.cfg_count, 
                  swd->desc->addr, swd->desc->count, swd->cfg->buff));
    } else {
        LOG_INFO(BSL_LS_SOC_DMA,
                 (BSL_META_U(unit,
                             "Run Chain:: Handle: %d, desc count: %d\n"), 
                  swd->handle, swd->ctrl.cfg_count));
    }
    sal_sem_give(SOC_SBUSDMA_DM_INTR(unit));
    
    SOC_SBUSDMA_DM_UNLOCK(unit);
    return SOC_E_NONE;
}

int
soc_sbusdma_desc_delete(int unit, sbusdma_desc_handle_t handle)
{
    _soc_sbusdma_state_t *swd;
    
    if ((SOC_SBUSDMA_DM_INFO(unit) == NULL) || !SOC_SBUSDMA_DM_INIT(unit) ||
        !SOC_SBUSDMA_DM_COUNT(unit)) {
        return SOC_E_INIT;
    }
    SOC_SBUSDMA_DM_LOCK(unit);    
    if ((handle > 0) && (handle <= SOC_SBUSDMA_MAX_DESC) 
        && SOC_SBUSDMA_DM_HANDLES(unit)[handle]) {
        swd = SOC_SBUSDMA_DM_HANDLES(unit)[handle];
        if (swd->handle != handle) {
            SOC_SBUSDMA_DM_UNLOCK(unit);
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Handle mismatch found: %d<=>%d\n"), swd->handle,
                       handle));
            return SOC_E_INTERNAL;
        }
        if (swd->ctrl.cfg_count == 1) {
            LOG_INFO(BSL_LS_SOC_DMA,
                     (BSL_META_U(unit,
                                 "Delete Single:: Handle: %d, desc count: %d, addr: %x, "
                                 "opcount: %d, buff: %p\n"), 
                      swd->handle, swd->ctrl.cfg_count, 
                      swd->desc->addr, swd->desc->count, swd->cfg->buff));
        } else {
            LOG_INFO(BSL_LS_SOC_DMA,
                     (BSL_META_U(unit,
                                 "Delete Chain:: Handle: %d, desc count: %d\n"), 
                      swd->handle, swd->ctrl.cfg_count));
        }
        sal_free(swd->cfg);
        if (!(swd->ctrl.flags & SOC_SBUSDMA_CFG_USE_SUPPLIED_DESC)) {
            soc_cm_sfree(unit, swd->desc);
        }
        sal_free(swd);
        SOC_SBUSDMA_DM_HANDLES(unit)[handle] = 0;
        SOC_SBUSDMA_DM_COUNT(unit)--;
        LOG_INFO(BSL_LS_SOC_DMA,
                 (BSL_META_U(unit,
                             "SBD DM count: %d\n"), SOC_SBUSDMA_DM_COUNT(unit)));
    } else {
        SOC_SBUSDMA_DM_UNLOCK(unit);
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Del request for invalid or non-existing descriptor handle: %d\n"), 
                   handle));
        return SOC_E_PARAM;
    }
    SOC_SBUSDMA_DM_UNLOCK(unit);
    return SOC_E_NONE;
}

#endif /* BCM_SBUSDMA_SUPPORT */

#ifdef BCM_CMICM_SUPPORT
/* DMA FIFO init for registers or memories */

/**
 * Function used to (partially) configure the DMA to write to a redirect writes from a register/memory 
 * to a host memory. 
 * 
 * 
 * @param unit 
 * @param ch - Channel used. should be between 0 and 3.
 * @param is_mem 
 * @param mem -name of memory, if memory is used (0 otherwise).
 * @param reg - name of register, if register used (0 otherwise).
 * @param copyno 
 * @param force_entry_size - in a case that entry size does not match the register\memory size  - ignore when equals to 0. 
 * @param host_entries  - number of host entries used by the DMA. 
 *  Must be a power of 2 between 64 and 16384.
 *  Note that only after host_entries writes does the DMA wrap around the host_buff.
 * @param host_buff - local memory on which the DMA writes. should be big enough to allow host_entries writes, 
 * in other words whould be the size of host_entries * ( size of memory or register used, in bytes).
 *  
 * Other register the caller may need to configure seperatly: 
 *  CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_THRESHOLD - The amount of writes by the DMA until a
 *  threshold based interrupt occurs. 
 *  
 *  TIMEOUT_COUNT field in CMIC_CMCx_FIFO_CHy_RD_DMA - Time between a DMA write and a timeout based interrupt.
 *  (May be set to 0 to disable timeout based interrupts).
 *  
 *  Endianess field in CMIC_CMCx_FIFO_CHy_RD_DMA.
 *  
 * @return int - success or failure.
 */
int
_soc_mem_sbus_fifo_dma_start_memreg(int unit, int ch, 
                                    int is_mem, soc_mem_t mem, soc_reg_t reg, 
                                    int copyno, int force_entry_size, int host_entries, void *host_buff)
{
    uint8 at;
    uint32 rval, data_beats, sel;
    int cmc;
    int blk;
    schan_msg_t msg;
    int acc_type;

    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        if (ch < 0 || ch > 12 || host_buff == NULL) {
            return SOC_E_PARAM;
        }
        cmc = ch / 4;
        ch = ch % 4;
    } else {
        cmc = SOC_PCI_CMC(unit);
        if (ch < 0 || ch > 3 || host_buff == NULL) {
            return SOC_E_PARAM;
        }
    }

    switch (host_entries) {
    case 64:    sel = 0; break;
    case 128:   sel = 1; break;
    case 256:   sel = 2; break;
    case 512:   sel = 3; break;
    case 1024:  sel = 4; break;
    case 2048:  sel = 5; break;
    case 4096:  sel = 6; break;
    case 8192:  sel = 7; break;
    case 16384: sel = 8; break;
    default:
        return SOC_E_PARAM;
    }

#ifdef BCM_IPFIX_SUPPORT        
#ifdef BCM_PETRA_SUPPORT
    if (!SOC_IS_DPP(unit) && !SOC_IS_DFE(unit))
#endif
    {
        if (mem != ING_IPFIX_EXPORT_FIFOm && mem != EGR_IPFIX_EXPORT_FIFOm &&
            mem !=  L2_MOD_FIFOm && mem != FT_EXPORT_FIFOm &&
            mem != FT_EXPORT_DATA_ONLYm &&
#ifdef BCM_CALADAN3_SUPPORT
            mem != CM_EJECTION_FIFOm && mem != CO_WATCHDOG_TIMER_EXPIRED_FIFOm &&
            mem != TMB_UPDATER_RSP_FIFO0m && mem != TMB_UPDATER_RSP_FIFO1m &&
            mem != TMB_UPDATER_RECYCLE_CHAIN_FIFOm &&
#endif
            mem != EGR_SER_FIFOm && mem != ING_SER_FIFOm
           ) {
            return SOC_E_BADID;
        }
    }
#endif        

    /* Differentiate between reg and mem to get address, size and block */
    if ((!is_mem) && SOC_REG_IS_VALID(unit, reg)) {
        data_beats = BYTES2WORDS(soc_reg_bytes(unit, reg));
        rval = soc_reg_addr_get(unit, reg, REG_PORT_ANY, 0, FALSE, &blk, &at);
    } else {
        data_beats = soc_mem_entry_words(unit, mem);
        if (copyno == MEM_BLOCK_ANY) {
            copyno = SOC_MEM_BLOCK_ANY(unit, mem);
        }
        rval = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);
        blk = SOC_BLOCK2SCH(unit, copyno);
    }
    /*Force entry size*/
    if (force_entry_size > 0)
    {
        data_beats = BYTES2WORDS(force_entry_size);
    }
    
    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_SBUS_START_ADDRESS_OFFSET(cmc, ch), rval);

    schan_msg_clear(&msg);
    if (is_mem) {
        acc_type = SOC_MEM_ACC_TYPE(unit, mem);
    } else {
        acc_type = 0;
    }
    soc_schan_header_cmd_set(unit, &msg.header, FIFO_POP_CMD_MSG, blk, 0,
                             acc_type, 4, 0, 0);  

    /* Set 1st schan ctrl word as opcode */
    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_OPCODE_OFFSET(cmc, ch), msg.dwords[0]);

    rval = soc_cm_l2p(unit, host_buff);
    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_START_ADDRESS_OFFSET(cmc, ch),
                  rval);

    rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch));
    soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_CFGr, &rval, BEAT_COUNTf, 
                      data_beats);
    soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_CFGr, &rval,
                      HOST_NUM_ENTRIES_SELf, sel);
    soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_CFGr, &rval, ABORTf, 0);

#if !defined(BCM_CALADAN3_SUPPORT)
    /* Note: Following might need to be tuned */
    soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_CFGr, &rval,
                      TIMEOUT_COUNTf, 1000);
    /* Note: Following could be removed ?? */
    soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_CFGr, &rval,
                      NACK_FATALf, 1);
#endif

    if (soc_feature(unit, soc_feature_multi_sbus_cmds)) {
        
    }
    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch), rval);

#if defined(BCM_CALADAN3_SUPPORT)
    if (SOC_IS_CALADAN3(unit)) {
        /* Note: Following might need to be tuned */
        soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_THRESHOLD_OFFSET(cmc, ch), 
                      host_entries - 64);
    }
#else
    if (SOC_IS_ARAD(unit)) {
        /* Always 1 in Arad when interrupt mechanism is not enabled */
        soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_THRESHOLD_OFFSET(cmc, ch), 
                     1);
    } else {
        /* Note: Following might need to be tuned */
        soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_THRESHOLD_OFFSET(cmc, ch), 
                      host_entries/10);
    }
#endif
    soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_CFGr, &rval, ENABLEf, 1);
    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch), rval);
    return SOC_E_NONE;
}

#ifdef BCM_KATANA2_SUPPORT
    int
_soc_kt2_mem_sbus_fifo_dma_start_memreg(int unit, int ch, 
        int is_mem, soc_mem_t mem, soc_reg_t reg, 
        int copyno, int host_entries, void *host_buff)
{
    uint8 at;
    uint32 rval, data_beats, sel;
    int cmc;
    int blk;
    schan_msg_t msg;
    soc_reg_t cfg_reg;
    int acc_type;
    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        if (ch < 0 || ch > 12 || host_buff == NULL) {
            return SOC_E_PARAM;
        }
        cmc = ch / 4;
        ch = ch % 4;
    } else {
        cmc = SOC_PCI_CMC(unit);
        if (ch < 0 || ch > 3 || host_buff == NULL) {
            return SOC_E_PARAM;
        }
    }

    switch (host_entries) {
        case 64:    sel = 0; break;
        case 128:   sel = 1; break;
        case 256:   sel = 2; break;
        case 512:   sel = 3; break;
        case 1024:  sel = 4; break;
        case 2048:  sel = 5; break;
        case 4096:  sel = 6; break;
        case 8192:  sel = 7; break;
        case 16384: sel = 8; break;
        default:
                    return SOC_E_PARAM;
    }

    if (mem != L2_MOD_FIFOm) {
        return SOC_E_BADID;
    }

    /* Differentiate between reg and mem to get address, size and block */
    if ((!is_mem) && SOC_REG_IS_VALID(unit, reg)) {
        data_beats = BYTES2WORDS(soc_reg_bytes(unit, reg));
        rval = soc_reg_addr_get(unit, reg, REG_PORT_ANY, 0, FALSE, &blk, &at);
    } else {
        data_beats = soc_mem_entry_words(unit, mem);
        if (copyno == MEM_BLOCK_ANY) {
            copyno = SOC_MEM_BLOCK_ANY(unit, mem);
        }
        rval = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);
        blk = SOC_BLOCK2SCH(unit, copyno);
    }

    soc_pci_write(unit, 
            CMIC_CMCx_FIFO_CHy_RD_DMA_SBUS_START_ADDRESS_OFFSET(cmc, ch), rval);

    schan_msg_clear(&msg);
    if (is_mem) {
        acc_type = SOC_MEM_ACC_TYPE(unit, mem);
    } else {
        acc_type = 0;
    }
    soc_schan_header_cmd_set(unit, &msg.header, FIFO_POP_CMD_MSG, blk, 0, acc_type, 4, 0, 0); 


    /* Set 1st schan ctrl word as opcode */
    soc_pci_write(unit, 
            CMIC_CMCx_FIFO_CHy_RD_DMA_OPCODE_OFFSET(cmc, ch), msg.dwords[0]);

    rval = soc_cm_l2p(unit, host_buff);
    soc_pci_write(unit, 
            CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_START_ADDRESS_OFFSET(cmc, ch),
            rval);

    rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch));
    cfg_reg = _soc_kt_fifo_reg_get (unit, cmc, ch, RD_DMA_CFG_REG);

    soc_reg_field_set(unit, cfg_reg, &rval, BEAT_COUNTf, 
            data_beats);
    soc_reg_field_set(unit, cfg_reg, &rval,
            HOST_NUM_ENTRIES_SELf, sel);
    soc_reg_field_set(unit, cfg_reg, &rval, ABORTf, 0);

    /* Note: Following might need to be tuned */
    soc_reg_field_set(unit, cfg_reg, &rval,
            TIMEOUT_COUNTf, 1000);
    /* Note: Following could be removed ?? */
    soc_reg_field_set(unit, cfg_reg, &rval,
            NACK_FATALf, 1);

    if (soc_feature(unit, soc_feature_multi_sbus_cmds)) {
        
    }
    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch), rval);

    /* Note: Following might need to be tuned */
    soc_pci_write(unit, 
            CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_THRESHOLD_OFFSET(cmc, ch), 
            host_entries/10);
    soc_reg_field_set(unit, cfg_reg, &rval, ENABLEf, 1);
    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch), rval);
    return SOC_E_NONE;
}

#endif 

int
_soc_mem_sbus_fifo_dma_start(int unit, int ch, soc_mem_t mem, int copyno,
                             int host_entries, void *host_buff)
{

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        return _soc_kt2_mem_sbus_fifo_dma_start_memreg(unit, ch, 
                TRUE /* is_mem */, mem, 0, 
                copyno, host_entries, host_buff);
    }
#endif   
    return _soc_mem_sbus_fifo_dma_start_memreg(unit, ch, 
                                               TRUE /* is_mem */, mem, 0, 
                                               copyno, 0, host_entries, host_buff);

}

#ifdef BCM_KATANA2_SUPPORT
int
_soc_kt2_mem_sbus_fifo_dma_stop(int unit, int ch)
{
    int cmc, iter = 0;
    uint32 rval;
    int to = SAL_BOOT_QUICKTURN ? 30000000 : 10000000;
    soc_reg_t cfg_reg , cfg_stat;

    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        if (ch < 0 || ch > 12) {
            return SOC_E_PARAM;
        }
        cmc = ch / 4;
        ch = ch % 4;
    } else {
        cmc = SOC_PCI_CMC(unit);
        if (ch < 0 || ch > 3) {
            return SOC_E_PARAM;
        }
    }
    cfg_reg = _soc_kt_fifo_reg_get (unit, cmc, ch, RD_DMA_CFG_REG);
    cfg_stat= _soc_kt_fifo_reg_get (unit, cmc, ch,RD_DMA_STAT);
    rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch));
    if (!soc_reg_field_get(unit, cfg_reg, rval, ENABLEf)) {
        return SOC_E_NONE;
    }
    rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch));
    soc_reg_field_set(unit, cfg_reg, &rval, ABORTf, 1);
    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch), rval);
    
    sal_udelay(1000);
    rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_OFFSET(cmc, ch));
 
    while (soc_reg_field_get(unit, cfg_stat, rval, DONEf) == 0 &&
           iter++ <to) {
        sal_udelay(1000);
        rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_OFFSET(cmc, ch));
    }
    rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch));
    soc_reg_field_set(unit, cfg_reg, &rval, ENABLEf, 0);
    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch), rval);

    if (iter >= to) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "FIFO DMA abort failed !!\n")));
        return SOC_E_INTERNAL;
    }
    return SOC_E_NONE;
}
#endif


int
_soc_mem_sbus_fifo_dma_stop(int unit, int ch)
{
    int cmc, iter = 0;
    uint32 rval;
    int to = SAL_BOOT_QUICKTURN ? 30000000 : 10000000;

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit) ) {
        return _soc_kt2_mem_sbus_fifo_dma_stop(unit,ch);
    }
#endif

    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        if (ch < 0 || ch > 12) {
            return SOC_E_PARAM;
        }
        cmc = ch / 4;
        ch = ch % 4;
    } else {
        cmc = SOC_PCI_CMC(unit);
        if (ch < 0 || ch > 3) {
            return SOC_E_PARAM;
        }
    }

    rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch));
    if (!soc_reg_field_get(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_CFGr, rval, ENABLEf)) {
        return SOC_E_NONE;
    }
    rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch));
    soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_CFGr, &rval, ABORTf, 1);
    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch), rval);
    
    sal_udelay(1000);
    rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_OFFSET(cmc, ch));
 
    while (soc_reg_field_get(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_STATr, rval, DONEf) == 0 &&
           iter++ <to) {
        sal_udelay(1000);
        rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_OFFSET(cmc, ch));
    }
    rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch));
    soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_CFGr, &rval, ENABLEf, 0);
    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, ch), rval);

    if (iter >= to) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "FIFO DMA abort failed !!\n")));
        return SOC_E_INTERNAL;
    }
    return SOC_E_NONE;
}

int
_soc_mem_sbus_fifo_dma_get_num_entries(int unit, int ch, int *count)
{
    uint32 val = 0;
    int cmc;

    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        if (ch < 0 || ch > 12) {
            return SOC_E_PARAM;
        }
        cmc = ch / 4;
        ch = ch % 4;
    } else {
        cmc = SOC_PCI_CMC(unit);
        if (ch < 0 || ch > 3) {
            return SOC_E_PARAM;
        }
    }

    val = soc_pci_read(unit, 
                  CMIC_CMCx_FIFO_CHy_RD_DMA_NUM_OF_ENTRIES_VALID_IN_HOSTMEM_OFFSET(cmc, ch));
    *count = val;
    if (val) {
        return SOC_E_NONE;             
    }
    return SOC_E_EMPTY;
}

int
_soc_mem_sbus_fifo_dma_set_entries_read(int unit, int ch, uint32 num)
{
    int cmc;

    if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
        if (ch < 0 || ch > 12) {
            return SOC_E_PARAM;
        }
        cmc = ch / 4;
        ch = ch % 4;
    } else {
        cmc = SOC_PCI_CMC(unit);
        if (ch < 0 || ch > 3) {
            return SOC_E_PARAM;
        }
    }

    

    soc_pci_write(unit, 
        CMIC_CMCx_FIFO_CHy_RD_DMA_NUM_OF_ENTRIES_READ_FRM_HOSTMEM_OFFSET(cmc, ch), num);
    return SOC_E_NONE;
}

#ifdef BCM_XGS3_SWITCH_SUPPORT /* DPPCOMPILEENABLE */
extern uint32 soc_mem_fifo_delay_value;

STATIC void
_soc_l2mod_sbus_fifo_dma_thread(void *unit_vp)
{
    uint8 overflow, timeout;
    int i, count, adv_threshold;
    int unit = PTR_TO_INT(unit_vp);
    int cmc = SOC_PCI_CMC(unit);
    soc_control_t *soc = SOC_CONTROL(unit);
    uint32 entries_per_buf, rval;
    int rv, interval, non_empty;
    uint8 ch;
    uint32 *buff_max, intr_mask;
    void *host_entry, *host_buff; 
    int entry_words;
 
    if (SOC_IS_TD2_TT2(unit)) {
        /*
         * Channel 0 is for X pipe
         * Channel 1 is for Y pipe but nothing will go into Y pipe L2_MOD_FIFO.
         */ 
        ch = SOC_MEM_FIFO_DMA_CHANNEL_0; 
    } else {
        ch = SOC_MEM_FIFO_DMA_CHANNEL_1;
    }
    intr_mask = IRQ_CMCx_FIFO_CH_DMA(ch);

    entries_per_buf = soc_property_get(unit, spn_L2XMSG_HOSTBUF_SIZE, 1024);
    adv_threshold = entries_per_buf / 2;

    entry_words = soc_mem_entry_words(unit, L2_MOD_FIFOm);
    host_buff = soc_cm_salloc(unit, entries_per_buf * entry_words * sizeof(uint32),
                             "L2_MOD DMA Buffer");
    if (host_buff == NULL) {
        soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                           SOC_SWITCH_EVENT_THREAD_L2MOD_DMA, __LINE__, 
                           SOC_E_MEMORY);
        goto cleanup_exit;
    }
    
    rv = _soc_mem_sbus_fifo_dma_start(unit, ch, L2_MOD_FIFOm, MEM_BLOCK_ANY,
                                      entries_per_buf, host_buff);
    if (SOC_FAILURE(rv)) {
        soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                           SOC_SWITCH_EVENT_THREAD_L2MOD_DMA, __LINE__, rv);
        goto cleanup_exit;
    }
   
    host_entry = host_buff;
    buff_max = (uint32 *)host_entry + (entries_per_buf * entry_words);

    while ((interval = soc->l2x_interval)) {
        overflow = 0; timeout = 0;
        if (soc->l2modDmaIntrEnb) {
            soc_cmicm_intr0_enable(unit, intr_mask);
            if (sal_sem_take(soc->arl_notify, interval) < 0) {
                LOG_VERBOSE(BSL_LS_SOC_INTR,
                            (BSL_META_U(unit,
                                        "%s polling timeout soc_mem_fifo_delay_value=%d\n"), 
                             soc->l2x_name, soc_mem_fifo_delay_value));
            } else {
                LOG_VERBOSE(BSL_LS_SOC_INTR,
                            (BSL_META_U(unit,
                                        "%s woken up soc_mem_fifo_delay_value=%d\n"), 
                             soc->l2x_name, soc_mem_fifo_delay_value));
                /* check for timeout or overflow and either process or continue */
                rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_OFFSET(cmc, ch));
                timeout = soc_reg_field_get(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_STATr,
                                            rval, HOSTMEM_TIMEOUTf);
                if (!timeout) {
                    overflow = soc_reg_field_get(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_STATr,
                                                 rval, HOSTMEM_OVERFLOWf);
                    timeout |= overflow;
                }
            }

        } else {
            sal_usleep(interval);
        }


        do {
            non_empty = FALSE;
            /* get entry count, process if nonzero else continue */
            rv = _soc_mem_sbus_fifo_dma_get_num_entries(unit, ch, &count);
            if (SOC_SUCCESS(rv)) {
                non_empty = TRUE;
                if (count > adv_threshold) {
                    count = adv_threshold;
                }
                for (i = 0; i < count; i++) {
                    if(!soc->l2x_interval) {
                        goto cleanup_exit;
                    }
#ifdef BCM_TRIUMPH3_SUPPORT
                    if (SOC_IS_TRIUMPH3(unit)) {
                        soc_tr3_l2mod_fifo_process(unit, soc->l2x_flags,
                                                   host_entry);
                    } else 
#endif
                    {
#ifdef BCM_KATANA2_SUPPORT
                        if (SOC_IS_KATANA2(unit)) {
                            _soc_kt_l2mod_fifo_process(unit, soc->l2x_flags,
                                    host_entry);

                        } else 
#endif
#ifdef BCM_HURRICANE2_SUPPORT
                        if (soc_feature(unit, soc_feature_fifo_dma_hu2)) {
                            _soc_hu2_l2mod_fifo_process(unit, soc->l2x_flags,
                                    host_entry);

                        } else 
#endif
                        { 
#ifdef BCM_TRIDENT2_SUPPORT
                            _soc_td2_l2mod_fifo_process(unit, soc->l2x_flags,
                                    host_entry);
#endif /* BCM_TRIDENT2_SUPPORT*/
                        }
                    }
                    host_entry = (uint32 *)host_entry + entry_words;
                    /* handle roll over */
                    if ((uint32 *)host_entry >= buff_max) {
                        host_entry = host_buff;
                    }
                    /*
                     * PPA may wait for available space in mod_fifo, lower the
                     * threshold when ppa is running, therefore read point can
                     * be updated sooner.
                     */
                    if (SOC_CONTROL(unit)->l2x_ppa_in_progress && i >= 63) {
                        i++;
                        break;
                    }
                }
                (void)_soc_mem_sbus_fifo_dma_set_entries_read(unit, ch, i);
            }

            /* check and clear error */
            rval = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_OFFSET(cmc, ch));
            if (soc_reg_field_get(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_STATr, rval,
                                  DONEf)) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "FIFO DMA engine terminated for cmc[%d]:ch[%d]\n"), 
                           cmc, ch));
                if (soc_reg_field_get(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_STATr, rval,
                                  ERRORf)) {
                    LOG_ERROR(BSL_LS_SOC_COMMON,
                              (BSL_META_U(unit,
                                          "FIFO DMA engine encountered error: [0x%x]\n"),
                               rval));
                }
                goto cleanup_exit;
            }  

            if (!SOC_CONTROL(unit)->l2x_ppa_in_progress) {
                sal_thread_yield();
            }
        } while (non_empty);
        /* Clearing of the FIFO_CH_DMA_INT interrupt by resetting
           overflow & timeout status in FIFO_CHy_RD_DMA_STAT_CLR reg */
        if (timeout) {
            rval = 0;
            soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_STAT_CLRr, &rval, 
                              HOSTMEM_OVERFLOWf, 1);
            soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_STAT_CLRr, &rval, 
                              HOSTMEM_TIMEOUTf, 1);
            soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_CLR_OFFSET(cmc, ch), 
                          rval);
        } 
    }

cleanup_exit:
    (void)_soc_mem_sbus_fifo_dma_stop(unit, ch);

    if (host_buff != NULL) {
        soc_cm_sfree(unit, host_buff);
    }
    soc->l2x_pid = SAL_THREAD_ERROR;
    sal_thread_exit(0);
}

STATIC int
_soc_l2mod_sbus_fifo_enable(int unit, uint8 val)
{
    uint32 rval = 0;
    soc_reg_field_set(unit, L2_MOD_FIFO_ENABLEr, &rval, 
                      INTERNAL_L2_ENTRYf, val);
    if (soc_feature(unit, soc_feature_esm_support)) {
        soc_reg_field_set(unit, L2_MOD_FIFO_ENABLEr, &rval, 
                          EXTERNAL_L2_ENTRYf, val);
    }
    soc_reg_field_set(unit, L2_MOD_FIFO_ENABLEr, &rval, L2_DELETEf, val);
    soc_reg_field_set(unit, L2_MOD_FIFO_ENABLEr, &rval, L2_INSERTf, val);
    soc_reg_field_set(unit, L2_MOD_FIFO_ENABLEr, &rval, L2_LEARNf, val);
    soc_reg_field_set(unit, L2_MOD_FIFO_ENABLEr, &rval, L2_MEMWRf, val);
    SOC_IF_ERROR_RETURN(WRITE_L2_MOD_FIFO_ENABLEr(unit, rval));
    return SOC_E_NONE;
}

STATIC int
_soc_td2_l2mod_sbus_fifo_enable(int unit, int enable)
{
    uint32 rval = 0;

    soc_reg_field_set(unit, AUX_ARB_CONTROLr, &rval,
                      L2_MOD_FIFO_ENABLE_L2_DELETEf, enable);
    soc_reg_field_set(unit, AUX_ARB_CONTROLr, &rval, L2_MOD_FIFO_ENABLE_AGEf,
                      enable);
    soc_reg_field_set(unit, AUX_ARB_CONTROLr, &rval, L2_MOD_FIFO_ENABLE_LEARNf,
                      enable);
    SOC_IF_ERROR_RETURN(WRITE_AUX_ARB_CONTROLr(unit, rval));
    return SOC_E_NONE;
}

/*
 * Function:
 *     soc_l2mod_running
 * Purpose:
 *     Determine the L2MOD sync thread running parameters
 * Parameters:
 *     unit - unit number.
 *     flags (OUT) - if non-NULL, receives the current flag settings
 *     interval (OUT) - if non-NULL, receives the current pass interval
 * Returns:
 *     Boolean; TRUE if L2MOD sync thread is running
 */

int
_soc_l2mod_running(int unit, uint32 *flags, sal_usecs_t *interval)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    soc_control_t *soc = SOC_CONTROL(unit);

    if (SOC_IS_XGS3_SWITCH(unit)) {
        if (soc->l2x_pid != SAL_THREAD_ERROR) {
            if (flags != NULL) {
                *flags = soc->l2x_flags;
            }
            if (interval != NULL) {
                *interval = soc->l2x_interval;
            }
        }

        return (soc->l2x_pid != SAL_THREAD_ERROR);
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *     _soc_l2mod_stop
 * Purpose:
 *     Stop L2MOD-related thread
 * Parameters:
 *     unit - unit number.
 * Returns:
 *     SOC_E_XXX
 */
int
_soc_l2mod_stop(int unit)
{
    soc_control_t * soc = SOC_CONTROL(unit);
    int rv = SOC_E_NONE;
    uint8 ch = SOC_MEM_FIFO_DMA_CHANNEL_1; 

    if (SOC_IS_XGS3_SWITCH(unit)) {
        LOG_INFO(BSL_LS_SOC_ARL,
                 (BSL_META_U(unit,
                             "soc_l2mod_stop: unit=%d\n"), unit));

        if (SOC_IS_TRIUMPH3(unit)) {
            _soc_l2mod_sbus_fifo_enable(unit, 0);
        } else {
            _soc_td2_l2mod_sbus_fifo_enable(unit, 0);
        }
        if (!soc_feature(unit, soc_feature_fifo_dma)) {
            soc_cmicm_intr0_disable(unit, IRQ_CMCx_FIFO_CH_DMA(ch));
            soc->l2x_interval = 0;  /* Request exit */
            /* Wake up thread so it will check the exit flag */
            sal_sem_give(soc->arl_notify);
        }
        return rv;
    }
    return SOC_E_UNAVAIL;
}

int
_soc_l2mod_start(int unit, uint32 flags, sal_usecs_t interval)
{
    soc_control_t * soc = SOC_CONTROL(unit);
    int             pri;
    uint8 ch =      SOC_MEM_FIFO_DMA_CHANNEL_1; 

    if (!soc_feature(unit, soc_feature_arl_hashed)) {
        return SOC_E_UNAVAIL;
    }

    if (soc->l2x_interval != 0) {
        SOC_IF_ERROR_RETURN(_soc_l2mod_stop(unit));
    }

    sal_snprintf(soc->l2x_name, sizeof(soc->l2x_name), "bcmL2MOD.%d", unit);

    soc->l2x_flags = flags;
    soc->l2x_interval = interval;

    if (interval == 0) {
        return SOC_E_NONE;
    }

    if (soc->l2x_pid == SAL_THREAD_ERROR) {
        pri = soc_property_get(unit, spn_L2XMSG_THREAD_PRI, 50);

        soc->l2x_pid =
                sal_thread_create(soc->l2x_name, SAL_THREAD_STKSZ, pri,
                                  _soc_l2mod_sbus_fifo_dma_thread, INT_TO_PTR(unit));
        if (soc->l2x_pid == SAL_THREAD_ERROR) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "soc_l2mod_start: Could not start L2MOD thread\n")));
            return SOC_E_MEMORY;
        }
    }

    if (!soc_feature(unit, soc_feature_fifo_dma)) {
        soc_cmicm_intr0_enable(unit, IRQ_CMCx_FIFO_CH_DMA(ch));
    }
    if (SOC_IS_TRIUMPH3(unit)) {
        _soc_l2mod_sbus_fifo_enable(unit, 1);
    } else {
        _soc_td2_l2mod_sbus_fifo_enable(unit, 1);
    }

    return SOC_E_NONE;
}

#endif /* BCM_XGS3_SWITCH_SUPPORT */ /* DPPCOMPILEENABLE */
#endif /* BCM_CMICM_SUPPORT */
#endif /* BCM_ESW_SUPPORT */

