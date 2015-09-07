/*
 * $Id: l2.c 1.1 Broadcom SDK $
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
 * File:        l2.c
 * Purpose:     Hurricane2 L2 functions
 */

#include <soc/drv.h>
#include <bcm/error.h>

#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif
#ifdef BCM_HURRICANE2_SUPPORT
#include <soc/hurricane2.h>
#endif


#if defined(BCM_HURRICANE2_SUPPORT) 

#define RD_DMA_CFG_REG                   0
#define RD_DMA_HOTMEM_THRESHOLD_REG      1
#define RD_DMA_STAT                      2
#define RD_DMA_STAT_CLR                  3

int
_soc_mem_hu2_fifo_dma_get_read_ptr(int unit, int chan, void **host_ptr, int *count)
{
    soc_reg_t cfg_reg, stat_reg;
    int host_entries, data_beats;
    soc_field_t overflow_field;
    uint32 addr, rval, stat, hostmem_addr, read_ptr, write_ptr;
    int cmc = SOC_PCI_CMC(unit);
    
    if (chan < 0 || chan > 3 || host_ptr == NULL) {
        return SOC_E_PARAM;
    }

    if(!soc_feature(unit, soc_feature_cmicm)) {
        return BCM_E_UNAVAIL;
    }

    read_ptr = 0;
    addr = CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_READ_PTR_OFFSET(cmc, chan);
    soc_pci_getreg(unit, addr, &read_ptr);

    addr = CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_WRITE_PTR_OFFSET(cmc, chan);
    write_ptr = 0;
    soc_pci_getreg(unit, addr, &write_ptr);

    if (write_ptr == 0) {
        return SOC_E_EMPTY;
    }

    switch(chan) {
    case 1: overflow_field = FIFO_CH1_DMA_HOSTMEM_OVERFLOWf; break;
    case 2: overflow_field = FIFO_CH2_DMA_HOSTMEM_OVERFLOWf; break;
    case 3: overflow_field = FIFO_CH3_DMA_HOSTMEM_OVERFLOWf; break;
    default: overflow_field = FIFO_CH0_DMA_HOSTMEM_OVERFLOWf; break;
    }

    if (read_ptr == write_ptr) {
        addr = CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_OFFSET(cmc, chan);
        stat = 0;
        soc_pci_getreg(unit, addr, &stat);
        stat_reg = _soc_hu2_fifo_reg_get (unit, cmc, chan, RD_DMA_STAT);
        if (!soc_reg_field_get(unit, stat_reg, stat, overflow_field)) {
            return SOC_E_EMPTY;
        }

        /* Re-read write pointer */
        addr = CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_WRITE_PTR_OFFSET(cmc, chan);
        write_ptr = 0;
        soc_pci_getreg(unit, addr, &write_ptr);
    }

    addr = CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_START_ADDRESS_OFFSET(cmc, chan);
    hostmem_addr = 0;
    soc_pci_getreg(unit, addr, &hostmem_addr);

    addr = CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, chan);
    rval = 0;
    soc_pci_getreg(unit, addr, &rval);

    cfg_reg = _soc_hu2_fifo_reg_get (unit, cmc, chan, RD_DMA_CFG_REG);
    data_beats = soc_reg_field_get(unit, cfg_reg, rval, BEAT_COUNTf);
    if (data_beats <= 0) {
        soc_cm_debug(DK_ERR, "Invalid BEAT_COUNT (%d) in "
                 "CMIC_CMC%d_FIFO_CH%d_RD_DMA_CFG \n", data_beats, cmc, chan);
        return SOC_E_CONFIG;
    }

    switch (soc_reg_field_get(unit, cfg_reg, rval, HOST_NUM_ENTRIES_SELf)) {
    case 0:  host_entries = 64;    break;
    case 1:  host_entries = 128;   break;
    case 2:  host_entries = 256;   break;
    case 3:  host_entries = 512;   break;
    case 4:  host_entries = 1024;  break;
    case 5:  host_entries = 2048;  break;
    case 6:  host_entries = 4096;  break;
    case 7:  host_entries = 8192;  break;
    case 8:  host_entries = 16384; break;
    case 9:  host_entries = 32768; break;
    case 10: host_entries = 65536; break;
    default: return SOC_E_CONFIG;
    }

    *host_ptr = soc_cm_p2l(unit, read_ptr);
    if (read_ptr >= write_ptr) {
        *count = host_entries -
            (read_ptr - hostmem_addr) / data_beats / sizeof(uint32);
    } else {
        *count = (write_ptr - read_ptr) / data_beats / sizeof(uint32);
    }

    return (*count) ? SOC_E_NONE : SOC_E_EMPTY;
}

int
_soc_mem_hu2_fifo_dma_advance_read_ptr(int unit, int chan, int count)
{
    soc_reg_t cfg_reg, statclr_reg;
    soc_field_t overflow_field;
    int host_entries, data_beats;
    uint32 addr, rval, statclr;
    uint32 *host_buf, *read_ptr;
    int cmc = SOC_PCI_CMC(unit);

    if (chan < 0 || chan > 3) {
        return SOC_E_PARAM;
    }

    if(!soc_feature(unit, soc_feature_cmicm)) {
        return BCM_E_UNAVAIL;
    }

    cfg_reg = _soc_hu2_fifo_reg_get (unit, cmc, chan, RD_DMA_CFG_REG);
    addr = CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc, chan);
    rval = 0;
    soc_pci_getreg(unit, addr, &rval);
    data_beats = soc_reg_field_get(unit, cfg_reg, rval, BEAT_COUNTf);

    switch (soc_reg_field_get(unit, cfg_reg, rval, HOST_NUM_ENTRIES_SELf)) {
    case 0:  host_entries = 64;    break;
    case 1:  host_entries = 128;   break;
    case 2:  host_entries = 256;   break;
    case 3:  host_entries = 512;   break;
    case 4:  host_entries = 1024;  break;
    case 5:  host_entries = 2048;  break;
    case 6:  host_entries = 4096;  break;
    case 7:  host_entries = 8192;  break;
    case 8:  host_entries = 16384; break;
    case 9:  host_entries = 32768; break;
    case 10: host_entries = 65536; break;
    default: return SOC_E_CONFIG;
    }

    if (count < 0 || count >= host_entries) {
        return SOC_E_PARAM;
    }

    /* Clear threshold overflow_field bit */
    switch(chan) {
    case 1: overflow_field = FIFO_CH1_DMA_HOSTMEM_OVERFLOWf; break;
    case 2: overflow_field = FIFO_CH2_DMA_HOSTMEM_OVERFLOWf; break;
    case 3: overflow_field = FIFO_CH3_DMA_HOSTMEM_OVERFLOWf; break;
    default: overflow_field = FIFO_CH0_DMA_HOSTMEM_OVERFLOWf; break;
    }

    addr = CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_CLR_OFFSET(cmc, chan);
    statclr_reg = _soc_hu2_fifo_reg_get (unit, cmc, chan, RD_DMA_STAT_CLR);
    statclr = 0;
    soc_reg_field_set(unit, statclr_reg, &statclr, overflow_field, 1);
    soc_pci_write(unit, addr, statclr);

    addr = CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_START_ADDRESS_OFFSET(cmc, chan);
    rval = 0;
    soc_pci_getreg(unit, addr, &rval);
    host_buf = soc_cm_p2l(unit, rval);

    addr = CMIC_CMCx_FIFO_CHy_RD_DMA_HOSTMEM_READ_PTR_OFFSET(cmc, chan);
    rval = 0;
    soc_pci_getreg(unit, addr, &rval);
    read_ptr = soc_cm_p2l(unit, rval);

    read_ptr += count * data_beats;
    if (read_ptr >= &host_buf[host_entries * data_beats]) {
        read_ptr -= host_entries * data_beats;
    }
    rval = soc_cm_l2p(unit, read_ptr);
    soc_pci_write(unit, addr, rval);

    return SOC_E_NONE;
}

soc_reg_t
_soc_hu2_fifo_reg_get(int unit, int cmc, int chan, int type)
{
    switch(type) {
    case RD_DMA_HOTMEM_THRESHOLD_REG:
        switch((cmc << 4) + chan) {
        case 0x00: return CMIC_CMC0_FIFO_CH0_RD_DMA_HOSTMEM_THRESHOLDr;
        case 0x01: return CMIC_CMC0_FIFO_CH1_RD_DMA_HOSTMEM_THRESHOLDr;
        case 0x02: return CMIC_CMC0_FIFO_CH2_RD_DMA_HOSTMEM_THRESHOLDr;
        case 0x03: return CMIC_CMC0_FIFO_CH3_RD_DMA_HOSTMEM_THRESHOLDr;
        case 0x10: return CMIC_CMC1_FIFO_CH0_RD_DMA_HOSTMEM_THRESHOLDr;
        case 0x11: return CMIC_CMC1_FIFO_CH1_RD_DMA_HOSTMEM_THRESHOLDr;
        case 0x12: return CMIC_CMC1_FIFO_CH2_RD_DMA_HOSTMEM_THRESHOLDr;
        case 0x13: return CMIC_CMC1_FIFO_CH3_RD_DMA_HOSTMEM_THRESHOLDr;
        case 0x20: return CMIC_CMC2_FIFO_CH0_RD_DMA_HOSTMEM_THRESHOLDr;
        case 0x21: return CMIC_CMC2_FIFO_CH1_RD_DMA_HOSTMEM_THRESHOLDr;
        case 0x22: return CMIC_CMC2_FIFO_CH2_RD_DMA_HOSTMEM_THRESHOLDr;
        case 0x23: return CMIC_CMC2_FIFO_CH3_RD_DMA_HOSTMEM_THRESHOLDr;
        default: return CMIC_CMC0_FIFO_CH0_RD_DMA_HOSTMEM_THRESHOLDr;
        }
        break;
    case RD_DMA_CFG_REG:
        switch((cmc << 4) + chan) {
        case 0x00: return CMIC_CMC0_FIFO_CH0_RD_DMA_CFGr;
        case 0x01: return CMIC_CMC0_FIFO_CH1_RD_DMA_CFGr;
        case 0x02: return CMIC_CMC0_FIFO_CH2_RD_DMA_CFGr;
        case 0x03: return CMIC_CMC0_FIFO_CH3_RD_DMA_CFGr;
        case 0x10: return CMIC_CMC1_FIFO_CH0_RD_DMA_CFGr;
        case 0x11: return CMIC_CMC1_FIFO_CH1_RD_DMA_CFGr;
        case 0x12: return CMIC_CMC1_FIFO_CH2_RD_DMA_CFGr;
        case 0x13: return CMIC_CMC1_FIFO_CH3_RD_DMA_CFGr;
        case 0x20: return CMIC_CMC2_FIFO_CH0_RD_DMA_CFGr;
        case 0x21: return CMIC_CMC2_FIFO_CH1_RD_DMA_CFGr;
        case 0x22: return CMIC_CMC2_FIFO_CH2_RD_DMA_CFGr;
        case 0x23: return CMIC_CMC2_FIFO_CH3_RD_DMA_CFGr;
        default: return CMIC_CMC0_FIFO_CH0_RD_DMA_CFGr;
        }
    case RD_DMA_STAT:
        switch((cmc << 4) + chan) {
        case 0x00: return CMIC_CMC0_FIFO_CH0_RD_DMA_STATr;
        case 0x01: return CMIC_CMC0_FIFO_CH1_RD_DMA_STATr;
        case 0x02: return CMIC_CMC0_FIFO_CH2_RD_DMA_STATr;
        case 0x03: return CMIC_CMC0_FIFO_CH3_RD_DMA_STATr;
        case 0x10: return CMIC_CMC1_FIFO_CH0_RD_DMA_STATr;
        case 0x11: return CMIC_CMC1_FIFO_CH1_RD_DMA_STATr;
        case 0x12: return CMIC_CMC1_FIFO_CH2_RD_DMA_STATr;
        case 0x13: return CMIC_CMC1_FIFO_CH3_RD_DMA_STATr;
        case 0x20: return CMIC_CMC2_FIFO_CH0_RD_DMA_STATr;
        case 0x21: return CMIC_CMC2_FIFO_CH1_RD_DMA_STATr;
        case 0x22: return CMIC_CMC2_FIFO_CH2_RD_DMA_STATr;
        case 0x23: return CMIC_CMC2_FIFO_CH3_RD_DMA_STATr;
        default: return CMIC_CMC0_FIFO_CH0_RD_DMA_STATr;
        }
    case RD_DMA_STAT_CLR:
        switch((cmc << 4) + chan) {
        case 0x00: return CMIC_CMC0_FIFO_CH0_RD_DMA_STAT_CLRr;
        case 0x01: return CMIC_CMC0_FIFO_CH1_RD_DMA_STAT_CLRr;
        case 0x02: return CMIC_CMC0_FIFO_CH2_RD_DMA_STAT_CLRr;
        case 0x03: return CMIC_CMC0_FIFO_CH3_RD_DMA_STAT_CLRr;
        case 0x10: return CMIC_CMC1_FIFO_CH0_RD_DMA_STAT_CLRr;
        case 0x11: return CMIC_CMC1_FIFO_CH1_RD_DMA_STAT_CLRr;
        case 0x12: return CMIC_CMC1_FIFO_CH2_RD_DMA_STAT_CLRr;
        case 0x13: return CMIC_CMC1_FIFO_CH3_RD_DMA_STAT_CLRr;
        case 0x20: return CMIC_CMC2_FIFO_CH0_RD_DMA_STAT_CLRr;
        case 0x21: return CMIC_CMC2_FIFO_CH1_RD_DMA_STAT_CLRr;
        case 0x22: return CMIC_CMC2_FIFO_CH2_RD_DMA_STAT_CLRr;
        case 0x23: return CMIC_CMC2_FIFO_CH3_RD_DMA_STAT_CLRr;
        default: return CMIC_CMC0_FIFO_CH0_RD_DMA_STAT_CLRr;
        }
    default: return INVALIDr; break;
    }
}

#endif /*BCM_HURRICANE2_SUPPORT*/
