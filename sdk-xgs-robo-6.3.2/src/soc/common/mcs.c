/*
 * $Id: mcs.c 1.4 Broadcom SDK $
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
 * File:        mcs.c
 * Purpose:     MCS initialize and load utilities
 * Requires:
 */


#include <soc/types.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/error.h>
#include <shared/util.h>

#ifdef BCM_CMICM_SUPPORT

#include <soc/mcs.h>

int
soc_mcs_init(int unit)
/*
 * Function: 	soc_msc_init
 * Purpose:	Initialize the MCS
 * Parameters:	unit - unit number
 * Returns:	SOC_E_xxx
 */
{
uint32 sram_size, sram_addr, i;
 
#if 0
    uint32 statr, sram_addr2, cfgr;
#endif    

    if (!soc_feature(unit, soc_feature_mcs)) {
        return (SOC_E_FAIL);
    } 

    if (SOC_IS_KATANA(unit)) {
        sram_size = 0x00100000;
        sram_addr = 0x00400000;
#ifdef BCM_TRIUMPH3_SUPPORT        
    } else if (SOC_IS_TRIUMPH3(unit)) {
        sram_size = 0x00080000;
        sram_addr = 0x00400000;
#endif        
#ifdef BCM_TRIDENT2_SUPPORT        
    } else if (SOC_IS_TD2_TT2(unit)) {
        sram_size = 0x00080000;
        sram_addr = 0x00400000;
#endif        
#ifdef BCM_CALADAN_SUPPORT        
    } else if (SOC_IS_CALADAN3(unit)) {
        sram_size = 0x00080000;
        sram_addr = 0x00400000;
#endif        
#ifdef BCM_ARAD_SUPPORT        
    } else if (SOC_IS_ARAD(unit)) {
        sram_size = 0x00080000;
        sram_addr = 0x00400000;
#endif        
    } else {
        return (SOC_E_UNIT);
    }

#if 0
    /* Write the first word of SRAM */
    for (i = 0; i < 0x200; i += 4) {
        soc_pci_mcs_write(unit, sram_addr + i, 0x12345678);
    }
    soc_cm_print("DMA3 %x\n",     soc_pci_mcs_read(unit, sram_addr));

    /* Do DMA from first word to rest of SRAM to init ECC */
    WRITE_CMIC_CMC1_CCM_DMA_HOST0_MEM_START_ADDRr(unit, sram_addr);
    WRITE_CMIC_CMC1_CCM_DMA_HOST1_MEM_START_ADDRr(unit, sram_addr + 0x200);
    WRITE_CMIC_CMC1_CCM_DMA_ENTRY_COUNTr(unit, (sram_size - 0x200) >> 2);
    READ_CMIC_CMC1_CCM_DMA_STATr(unit, &statr);  
    READ_CMIC_CMC1_CCM_DMA_CFGr(unit, &cfgr);  
    soc_cm_print("DMA1 %x %x\n", statr, cfgr);
    /* Start DMA */
    WRITE_CMIC_CMC1_CCM_DMA_CFGr(unit, 1);

    while (1) {
        READ_CMIC_CMC1_CCM_DMA_STATr(unit, &statr);
        if (statr != 0) {
            break;
        } 
    }

    READ_CMIC_CMC1_CCM_DMA_CUR_HOST0_ADDRr(unit, &sram_addr);
    READ_CMIC_CMC1_CCM_DMA_CUR_HOST1_ADDRr(unit, &sram_addr2);
    READ_CMIC_CMC1_CCM_DMA_ENTRY_COUNTr(unit, &sram_size);
    READ_CMIC_CMC1_CCM_DMA_CFGr(unit, &cfgr);  
    soc_cm_print("DMA2 %x %x %x %x %x\n", statr, sram_addr, sram_addr2, sram_size, cfgr);

    soc_cm_print("DMA3 %x\n",     soc_pci_mcs_read(unit, 0x0040000));
    soc_cm_print("AXI_SRAM_MEMC_CONFIG %x\n", AXI_SRAM_MEMC_CONFIGr);

#else
    i = 0;
    while (i < sram_size) {
        soc_pci_mcs_write(unit, sram_addr + i, 0x12345678);
        i += 4;
        if ((i & 0x0ffff) == 0) {
            sal_usleep(1000);
        } 
        
    }
#endif    
    return (SOC_E_NONE);
}

int
soc_mcs_reset(int unit, int uC)
/*
 * Function: 	soc_msc_reset
 * Purpose:	Put an MCS into reset state
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	SOC_E_xxxx
 */
{
    uint32      config, rst_control;

    if (!soc_feature(unit, soc_feature_mcs)) {
        return (SOC_E_FAIL);
    } 

    /* Put the uC in LE mode */
    if (uC == 0) {
        READ_UC_0_CONFIGr(unit, &config);
        soc_reg_field_set(unit, UC_0_CONFIGr, &config,
                          OVERWRITE_OTP_CONFIGf, 1);
        soc_reg_field_set(unit, UC_0_CONFIGr, &config,
                          CFGIEf, 1);
        soc_reg_field_set(unit, UC_0_CONFIGr, &config,
                          CFGEEf, 1);
        WRITE_UC_0_CONFIGr(unit, config);
    } else {
        READ_UC_1_CONFIGr(unit, &config);
        soc_reg_field_set(unit, UC_1_CONFIGr, &config,
                          OVERWRITE_OTP_CONFIGf, 1);
        soc_reg_field_set(unit, UC_1_CONFIGr, &config,
                          CFGIEf, 1);
        soc_reg_field_set(unit, UC_1_CONFIGr, &config,
                          CFGEEf, 1);
        WRITE_UC_1_CONFIGr(unit, config);
    } 

    /* Take the processor in and out of reset but halted */
    if (uC == 0) {
        READ_UC_0_RST_CONTROLr(unit, &rst_control);
        soc_reg_field_set(unit, UC_0_RST_CONTROLr, &rst_control,
                          BYPASS_RSTFSM_CTRLf, 1);
        soc_reg_field_set(unit, UC_0_RST_CONTROLr, &rst_control,
                          CPUHALT_Nf, 0);
        soc_reg_field_set(unit, UC_0_RST_CONTROLr, &rst_control,
                          SYS_PORESET_Nf, 0);
        soc_reg_field_set(unit, UC_0_RST_CONTROLr, &rst_control,
                          CPUHALT_Nf, 0);
        WRITE_UC_0_RST_CONTROLr(unit, rst_control);
        soc_reg_field_set(unit, UC_0_RST_CONTROLr, &rst_control,
                          SYS_PORESET_Nf, 1);
        soc_reg_field_set(unit, UC_0_RST_CONTROLr, &rst_control,
                          CORE_RESET_Nf, 1);
        WRITE_UC_0_RST_CONTROLr(unit, rst_control);
    } else {
        READ_UC_1_RST_CONTROLr(unit, &rst_control);
        soc_reg_field_set(unit, UC_1_RST_CONTROLr, &rst_control,
                          CPUHALT_Nf, 0);
        soc_reg_field_set(unit, UC_1_RST_CONTROLr, &rst_control,
                          SYS_PORESET_Nf, 0);
        soc_reg_field_set(unit, UC_1_RST_CONTROLr, &rst_control,
                          CPUHALT_Nf, 0);
        WRITE_UC_1_RST_CONTROLr(unit, rst_control);
        soc_reg_field_set(unit, UC_1_RST_CONTROLr, &rst_control,
                          SYS_PORESET_Nf, 1);
        soc_reg_field_set(unit, UC_1_RST_CONTROLr, &rst_control,
                          CORE_RESET_Nf, 1);
        WRITE_UC_1_RST_CONTROLr(unit, rst_control);
    }

    return (SOC_E_NONE);
}

int
soc_mcs_load(int unit, int uC, uint32 mcs_addr, int len, unsigned char *data)
/*
 * Function: 	soc_msc_load
 * Purpose:	Load a chunk into MCS memory
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 *              mcs_addr - Address of chunk from MCS perspective
 *              len - Length of chunk to load in bytes
 *              data - Pointer to data to load
 * Returns:	SOC_E_xxx
 */
{
    int i;
    uint32 wdata;
    int rc = SOC_E_NONE;        /* Be optimistic */

    if (!soc_feature(unit, soc_feature_mcs)) {
        return (SOC_E_FAIL);
    } 

    /* Convert the UC-centric address to a PCI address */
    if (mcs_addr < 0x100000) {           /* If in TCM space */
        /* TCMs base addrs from PCIe perspective */
        mcs_addr += (uC == 0) ? 0x100000 : 0x200000;
    } 

    for(i=0; i<len; i+=4, mcs_addr+=4) {
#ifndef LE_HOST
        /* We swap the bytes here for the LE ARMs. */
        wdata = _shr_swap32(*((uint32 *) &data[i]));
#else
        wdata = *((uint32 *) &data[i]);
#endif
        rc = soc_pci_mcs_write(unit, mcs_addr, wdata);
        if (rc != SOC_E_NONE) {
            break;
        } 
    }

    return (rc);
}

int
soc_mcs_start(int unit, int uC)
/*
 * Function: 	soc_msc_start
 * Purpose:	Start MCS execution
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	SOC_E_xxx
 */
{
    uint32      rst_control;

    if (!soc_feature(unit, soc_feature_mcs)) {
        return (SOC_E_FAIL);
    } 

    if (uC == 0) {
        READ_UC_0_RST_CONTROLr(unit, &rst_control);
        soc_reg_field_set(unit, UC_0_RST_CONTROLr, &rst_control,
                          CPUHALT_Nf, 1);
        WRITE_UC_0_RST_CONTROLr(unit, rst_control);
    } else {
        READ_UC_1_RST_CONTROLr(unit, &rst_control);
        soc_reg_field_set(unit, UC_1_RST_CONTROLr, &rst_control,
                          CPUHALT_Nf, 1);
        WRITE_UC_1_RST_CONTROLr(unit, rst_control);
    }

    return (SOC_E_NONE);
}


#endif
