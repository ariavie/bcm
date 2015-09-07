/* 
 * $Id: pli.c 1.15 Broadcom SDK $
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
 * File:        pli.c
 *
 * Provides:
 *      pli_reset_service
 *      pli_getreg_service
 *      pli_setreg_service
 *
 * Requires:
 *      soc_internal_reset
 *      soc_internal_<mem>
 *      soc_internal_send_int
 *      pcid_dma_stat_write
 *      pcid_dma_ctrl_write
 *      soc_internal_arl_init
 *
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory.h>

#include <soc/cmic.h>
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif
#include <soc/debug.h>
#include <sal/appl/io.h>
#include <bde/pli/verinet.h>

#include "pcid.h"
#include "mem.h"
#include "cmicsim.h"
#include "dma.h"
#include "pli.h"

/* 
 * pli_reset_service
 *
 * Set PCI memory state to reset defaults
 */
int pli_reset_service(pcid_info_t *pcid_info)
{
    soc_internal_reset(pcid_info);
    return 0;
}

/* 
 * PLI protocol compatible "get register" routines
 */

STATIC void 
pli_cmice_getreg_service(pcid_info_t * pcid_info, int unit, uint32 type,
                         uint32 regnum, uint32 *value)
{
    uint32 r;
    switch (type) {
    case PCI_CONFIG:
        r = regnum & 0xfff;
        if (r < PCIC_SIZE) {
            *value = PCIC(pcid_info, r);
        }
        break;
    case PCI_MEMORY:
        if (pcid_info->regmem_cb) {
            if ((pcid_info->regmem_cb(pcid_info, BCMSIM_PCI_MEM_READ,
                                      regnum, value,
                                      BYTES2BITS(sizeof(uint32)))) == 0)
                break;
        }
        r = regnum & 0xffff;
        if (r == CMIC_LED_CTRL || r == CMIC_LED_STATUS) {
            *value = 0;
        } else if (r < PCIM_SIZE(unit)) {
            *value = PCIM(pcid_info, r);
        } else {
            *value = 0xdeadbeef;
        }
        break;
    case I2C_CONFIG:
        break;
    case PLI_CONFIG:
        break;
    case JTAG_CONFIG:
        break;

    case SOC_INTERNAL:
        soc_internal_read_reg(pcid_info, regnum, value); 
        break;
    }
}

#ifdef BCM_CMICM_SUPPORT
STATIC void
pli_cmicm_getreg_service(pcid_info_t * pcid_info, int unit, uint32 type,
                         uint32 regnum, uint32 *value)
{
    uint32 r;
    switch (type) {
    case PCI_CONFIG:
        r = regnum & 0xfff;
        if (r < PCIC_SIZE) {
            *value = PCIC(pcid_info, r);
        }
        break;
    case PCI_MEMORY:
        if (pcid_info->regmem_cb) {
            if ((pcid_info->regmem_cb(pcid_info, BCMSIM_PCI_MEM_READ,
                                      regnum, value,
                                      BYTES2BITS(sizeof(uint32)))) == 0)
                break;
        }
        r = regnum & 0xfffff;
        if (r < PCIM_CMICM_SIZE) {
            *value = PCIM(pcid_info, r);
        } else {
            *value = 0xdeadbeef;
        }
        break;
    case I2C_CONFIG:
        break;
    case PLI_CONFIG:
        break;
    case JTAG_CONFIG:
        break;

    case SOC_INTERNAL:
        soc_internal_read_reg(pcid_info, regnum, value); 
        break;
    }
}
#endif

uint32
pli_getreg_service(pcid_info_t * pcid_info, int unit, uint32 type,
                   uint32 regnum)
{
    char            buffer[35];
    uint32          value, rv;

    PCID_MEM_LOCK(pcid_info);

    pcid_type_name(buffer, type);

    if ((regnum & 3) && type != SOC_INTERNAL) {
        sal_printf("pli_getreg_service: unaligned access, "
               "type=0x%x addr=0x%x\n", type, regnum);
        regnum &= ~3;
    }

    value = 0xffffffff;                /* Default value */

#ifdef BCM_CMICM_SUPPORT
    if (pcid_info->cmicm >= CMC0) {
        pli_cmicm_getreg_service(pcid_info, unit, type, regnum, &value);
    } else 
#endif
    {
        pli_cmice_getreg_service(pcid_info, unit, type, regnum, &value);
    }

    if (pcid_info->opt_pli_verbose) {
        debugk(DK_VERBOSE, "%s READ @0x%x => 0x%x\n", buffer, regnum, value);
    }

    rv = soc_internal_endian_swap(pcid_info, value, MF_ES_PIO);

    PCID_MEM_UNLOCK(pcid_info);

    return rv;
}

/* 
 * PLI protocol compatible "set register" routines
 */

STATIC void
pli_cmice_setreg_service(pcid_info_t * pcid_info, int unit, uint32 type,
                         uint32 regnum, uint32 value)
{
    uint32          r;

    switch (type) {
    case SOC_INTERNAL:
        soc_internal_write_reg(pcid_info, regnum, &value); 
        break;

    case PCI_CONFIG:
        r = regnum & 0xfff;
        if (r < PCIC_SIZE) {
            PCIC(pcid_info, r) = value;
        }
        break;
    case PCI_MEMORY:
        r = regnum & 0xffff;

        if (pcid_info->i2crom_fp) {
            fputc((r >> 8) & 0xff, pcid_info->i2crom_fp);
            fputc((r >> 0) & 0xff, pcid_info->i2crom_fp);
            fputc((value >> 24) & 0xff, pcid_info->i2crom_fp);
            fputc((value >> 16) & 0xff, pcid_info->i2crom_fp);
            fputc((value >> 8) & 0xff, pcid_info->i2crom_fp);
            fputc((value >> 0) & 0xff, pcid_info->i2crom_fp);
        }
        if (pcid_info->regmem_cb) {
            if ((pcid_info->regmem_cb(pcid_info, BCMSIM_PCI_MEM_WRITE,
                                      regnum, &value,
                                      BYTES2BITS(sizeof(uint32)))) == 0)
                break;
        }
        switch (r) {
        case CMIC_SCHAN_CTRL:
            soc_internal_schan_ctrl_write(pcid_info, value);
            break;
        case CMIC_CONFIG:
            if (value & CC_RESET_CPS) {
                soc_internal_reset(pcid_info);
            } else {
                PCIM(pcid_info, r) = value;
            }
            break;
        case CMIC_IRQ_MASK:
            PCIM(pcid_info, r) = value;
            soc_internal_send_int(pcid_info);       /* Send int if pending */
            break;
        case CMIC_DMA_STAT:
            pcid_dma_stat_write(pcid_info, value);
            break;
        case CMIC_DMA_CTRL:
            pcid_dma_ctrl_write(pcid_info, value);
            break;
        case CMIC_TABLE_DMA_CFG:
            PCIM(pcid_info, r) = value;
            soc_internal_xgs3_table_dma(pcid_info);
            break;
        case CMIC_SLAM_DMA_CFG:
            PCIM(pcid_info, r) = value;
            soc_internal_xgs3_tslam_dma(pcid_info);
            break;
        case CMIC_LED_CTRL:
        case CMIC_LED_STATUS:
            break;
        default:
            if (r < PCIM_SIZE(unit)) {
                PCIM(pcid_info, r) = value;
                if (soc_feature(unit, soc_feature_schmsg_alias)) {
                    if (r < 0x50) {
                        PCIM(pcid_info, r + 0x800) = value;
                    } else if (r >= 0x800 && r < 0x850) {
                        PCIM(pcid_info, r - 0x800) = value;
                    }
                }
            }
            break;
        }
        break;
    case I2C_CONFIG:
        break;
    case PLI_CONFIG:
        break;
    case JTAG_CONFIG:
        break;
    }

}

#ifdef BCM_CMICM_SUPPORT
STATIC void
pli_cmicm_setreg_service(pcid_info_t * pcid_info, int unit, uint32 type,
                         uint32 regnum, uint32 value)
{
    uint32          r;
   
    switch (type) {
    case SOC_INTERNAL:
        soc_internal_write_reg(pcid_info, regnum, &value); 
        break;

    case PCI_CONFIG:
        r = regnum & 0xfff;
        if (r < PCIC_SIZE) {
            PCIC(pcid_info, r) = value;
        }
        break;
    case PCI_MEMORY:
        r = regnum & 0xfffff;
        if (pcid_info->i2crom_fp) {
            fputc((r >> 8) & 0xff, pcid_info->i2crom_fp);
            fputc((r >> 0) & 0xff, pcid_info->i2crom_fp);
            fputc((value >> 24) & 0xff, pcid_info->i2crom_fp);
            fputc((value >> 16) & 0xff, pcid_info->i2crom_fp);
            fputc((value >> 8) & 0xff, pcid_info->i2crom_fp);
            fputc((value >> 0) & 0xff, pcid_info->i2crom_fp);
        }
        if (pcid_info->regmem_cb) {
            if ((pcid_info->regmem_cb(pcid_info, BCMSIM_PCI_MEM_WRITE,
                                      regnum, &value,
                                      BYTES2BITS(sizeof(uint32)))) == 0)
                break;
        }
        switch (r) {
        case CMIC_CMC0_SCHAN_CTRL_OFFSET:
            soc_internal_cmicm_schan_ctrl_write(pcid_info, regnum, value);
            break;
        case CMIC_CPS_RESET_OFFSET:
            if (value & CPS_RESET) {
                soc_internal_reset(pcid_info);
            } else {
                PCIM(pcid_info, r) = value;
            }
            break;
        case CMIC_CMC0_PCIE_IRQ_MASK0_OFFSET:
        case CMIC_CMC0_PCIE_IRQ_MASK1_OFFSET:
        case CMIC_CMC0_PCIE_IRQ_MASK2_OFFSET:
            PCIM(pcid_info, r) = value;
            soc_internal_cmicm_send_int(pcid_info, r); /* Send int if pending */
            break;
        case CMIC_CMC0_CH0_DMA_CTRL_OFFSET:
        case CMIC_CMC0_CH1_DMA_CTRL_OFFSET:
        case CMIC_CMC0_CH2_DMA_CTRL_OFFSET:
        case CMIC_CMC0_CH3_DMA_CTRL_OFFSET:
            pcid_dma_cmicm_ctrl_write(pcid_info, r, value);
            break;
        case CMIC_CMC0_DMA_STAT_CLR_OFFSET:
            if (value & 0xf) {
                PCIM(pcid_info, CMIC_CMC0_DMA_STAT_OFFSET) &= ~(value & 0xf);
                switch (value & 0xf) {
                case 1: 
/*                   PCIM(pcid_info, CMIC_CMC0_IRQ_STAT0_OFFSET) &= ~(IRQ_CMCx_DESC_DONE(0)); */
                   pcid_reg_and_write(pcid_info, CMIC_CMC0_IRQ_STAT0_OFFSET, IRQ_CMCx_DESC_DONE(0));
                   break;
                case 2: 
/*                   PCIM(pcid_info, CMIC_CMC0_IRQ_STAT0_OFFSET) &= ~(IRQ_CMCx_DESC_DONE(1)); */
                   pcid_reg_and_write(pcid_info, CMIC_CMC0_IRQ_STAT0_OFFSET, IRQ_CMCx_DESC_DONE(1));
                    break;
                case 4: 
/*                   PCIM(pcid_info, CMIC_CMC0_IRQ_STAT0_OFFSET) &= ~(IRQ_CMCx_DESC_DONE(2)); */
                   pcid_reg_and_write(pcid_info, CMIC_CMC0_IRQ_STAT0_OFFSET, IRQ_CMCx_DESC_DONE(2));
                    break;
                case 8: 
/*                   PCIM(pcid_info, CMIC_CMC0_IRQ_STAT0_OFFSET) &= ~(IRQ_CMCx_DESC_DONE(3)); */
                   pcid_reg_and_write(pcid_info, CMIC_CMC0_IRQ_STAT0_OFFSET, IRQ_CMCx_DESC_DONE(3));
                }
            }
            break;
        case CMIC_CMC0_TABLE_DMA_CFG_OFFSET:
            PCIM(pcid_info, r) = value;
            soc_internal_xgs3_cmicm_table_dma(pcid_info);
            break;
        case CMIC_CMC0_SLAM_DMA_CFG_OFFSET:
            PCIM(pcid_info, r) = value;
            soc_internal_xgs3_cmicm_tslam_dma(pcid_info);
            break;
        case CMIC_CMCx_SBUSDMA_CH0_CONTROL_OFFSET(0):
        case CMIC_CMCx_SBUSDMA_CH0_CONTROL_OFFSET(1):
        case CMIC_CMCx_SBUSDMA_CH0_CONTROL_OFFSET(2):
        case CMIC_CMCx_SBUSDMA_CH1_CONTROL_OFFSET(0):
        case CMIC_CMCx_SBUSDMA_CH1_CONTROL_OFFSET(1):
        case CMIC_CMCx_SBUSDMA_CH1_CONTROL_OFFSET(2):
        case CMIC_CMCx_SBUSDMA_CH2_CONTROL_OFFSET(0):
        case CMIC_CMCx_SBUSDMA_CH2_CONTROL_OFFSET(1):
        case CMIC_CMCx_SBUSDMA_CH2_CONTROL_OFFSET(2):
            PCIM(pcid_info, r) = value;
            soc_internal_cmicm_sbusdma(pcid_info, r);
            break;
        default:
            if (r < PCIM_CMICM_SIZE) {
                PCIM(pcid_info, r) = value;
            }
            break;
        }
        break;
    case I2C_CONFIG:
        break;
    case PLI_CONFIG:
        break;
    case JTAG_CONFIG:
        break;
    }

}
#endif

uint32
pli_setreg_service(pcid_info_t * pcid_info, int unit, uint32 type,
                   uint32 regnum, uint32 value)
{
    char            buffer[35];

    PCID_MEM_LOCK(pcid_info);

    pcid_type_name(buffer, type);

    value = soc_internal_endian_swap(pcid_info, value, MF_ES_PIO);

    if ((regnum & 3) && type != SOC_INTERNAL) {
        sal_printf("pli_setreg_service: unaligned access, "
               "type=0x%x addr=0x%x value=0x%x\n", type, regnum, value);
        regnum &= ~3;
    }

/*    debugk(DK_VERBOSE, "%s WRITE @0x%x <= 0x%x\n", buffer, regnum, value); */
    if (pcid_info->opt_pli_verbose) {
        debugk(DK_VERBOSE, "%s WRITE @0x%x <= 0x%x\n", buffer, regnum, value);
    }

#ifdef BCM_CMICM_SUPPORT
    if (pcid_info->cmicm >= CMC0) {
        pli_cmicm_setreg_service(pcid_info, unit, type, regnum, value);
    } else 
#endif
    {
        pli_cmice_setreg_service(pcid_info, unit, type, regnum, value);
    }
    
    PCID_MEM_UNLOCK(pcid_info);
    
    return 0;
}
