/*
 * $Id: uc.c,v 1.2 Broadcom SDK $
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


#include <shared/bsl.h>

#include <soc/types.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/error.h>
#include <shared/util.h>
#include <soc/uc_msg.h>

#if defined(BCM_CMICM_SUPPORT) || defined(BCM_IPROC_SUPPORT)

#ifdef BCM_IPROC_SUPPORT
#include <soc/iproc.h>
#define CORE1_LUT_ADDR          0xffff042c
#endif


uint32
soc_uc_mem_read(int unit, uint32 addr)
{
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_mcs)) {
        return soc_pci_mcs_read(unit, addr);
    }
#endif
#ifdef BCM_IPROC_SUPPORT
    if (soc_feature(unit, soc_feature_iproc)) {
#ifdef BCM_HURRICANE2_SUPPORT
        if (SOC_IS_HURRICANE2(unit)) {
            return soc_pci_mcs_read(unit, addr);
        } else
#endif /* BCM_HURRICANE2_SUPPORT */
        return soc_cm_iproc_read(unit, addr);
    }
#endif
    assert(0);
    return 0;
}

int
soc_uc_mem_write(int unit, uint32 addr, uint32 value)
{
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_mcs)) {
        return soc_pci_mcs_write(unit, addr, value);
    }
#endif
#ifdef BCM_IPROC_SUPPORT
    if (soc_feature(unit, soc_feature_iproc)) {
#ifdef BCM_HURRICANE2_SUPPORT
        if (SOC_IS_HURRICANE2(unit)) {
            return soc_pci_mcs_write(unit, addr, value);
        } else
#endif /* BCM_HURRICANE2_SUPPORT */
        soc_cm_iproc_write(unit, addr, value);
        return SOC_E_NONE;
    }
#endif
    assert(0);
    return SOC_E_FAIL;
}

int
soc_uc_addr_to_pcie(int unit, int uC, uint32 addr)
{
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_mcs)) {
        /* If in TCM space */
        if (addr < 0x100000) {
            /* TCMs base addrs from PCIe perspective */
            addr += (uC == 0) ? 0x100000 : 0x200000;
        }
        return addr;
    }
#endif
#ifdef BCM_IPROC_SUPPORT
    if (soc_feature(unit, soc_feature_iproc)) {
        return addr;
    }
#endif
    assert(0);
    return 0;
}

static
int
soc_uc_sram_extents(int unit, uint32 *addr, uint32 *size)
/*
 * Function: 	soc_uc_sram_extents
 * Purpose:	Get the SRAM address and length
 * Parameters:	unit - unit number
 * Returns:	SOC_E_xxx
 */
{
    *size = 0;
    *addr = 0;

#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        *size = 0x00100000;
        *addr = 0x00400000;
    }
#endif
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        *size = 0x00080000;
        *addr = 0x00400000;
    }
#endif        
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        *size = 0x00080000;
        *addr = 0x00400000;
    }
#endif        
#ifdef BCM_CALADAN_SUPPORT
    if (SOC_IS_CALADAN3(unit)) {
        *size = 0x00080000;
        *addr = 0x00400000;
    }
#endif        
#ifdef BCM_ARAD_SUPPORT
    if (SOC_IS_ARAD(unit)) {
        *size = 0x00080000;
        *addr = 0x00400000;
    }
#endif        
#ifdef BCM_HELIX4_SUPPORT
    if (SOC_IS_HELIX4(unit)) {
        *size = 512 * 1024;
        *addr = 0x1b000000;
    }
#endif        
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        *size = 512 * 1024;
        *addr = 0x1b000000;
    }
#endif        
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        *size = 1 * 1024 * 1024;
        *addr = 0x00000000;
    }
#endif        
    return (SOC_E_NONE);
}


#ifdef BCM_CMICM_SUPPORT

static
int
soc_uc_mcs_init(int unit)
/*
 * Function: 	soc_uc_msc_init
 * Purpose:	Initialize the CMICm MCS
 * Parameters:	unit - unit number
 * Returns:	SOC_E_xxx
 */
{
    int rc;
    uint32 sram_size, sram_addr, i;
 
    rc = soc_uc_sram_extents(unit, &sram_addr, &sram_size);
    if (rc != SOC_E_NONE) {
        return (rc);
    }

    i = 0;
    while (i < sram_size) {
        soc_uc_mem_write(unit, sram_addr + i, 0x12345678);
        i += 4;
        if ((i & 0x0ffff) == 0) {
            sal_usleep(1000);
        } 
    }
    return (SOC_E_NONE);
}

static
int
soc_uc_mcs_reset(int unit, int uC)
/*
 * Function: 	soc_uc_mcs_reset
 * Purpose:	Put an MCS into reset state
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	SOC_E_xxxx
 */
{
    uint32      config, rst_control;

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

static
int
soc_uc_mcs_in_reset(int unit, int uC)
/*
 * Function: 	soc_uc_mcs_in_reset
 * Purpose:	Return 1 if in RESET state, 0 otherwise
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	1 or 0
 */
{
    uint32 rst, halt, ctrl;

    if (uC == 0) {
        READ_UC_0_RST_CONTROLr(unit, &ctrl);
        rst = soc_reg_field_get(unit, UC_0_RST_CONTROLr, ctrl, CORE_RESET_Nf);
        halt = soc_reg_field_get(unit, UC_0_RST_CONTROLr, ctrl, CPUHALT_Nf);
    }
    else {
        READ_UC_1_RST_CONTROLr(unit, &ctrl);
        rst = soc_reg_field_get(unit, UC_1_RST_CONTROLr, ctrl, CORE_RESET_Nf);
        halt = soc_reg_field_get(unit, UC_1_RST_CONTROLr, ctrl, CPUHALT_Nf);
    }
    /* If either RESET or HALT is asserted (0), dev is in RESET */
    return (rst == 0) || (halt == 0);
}

static
int
soc_uc_mcs_start(int unit, int uC, uint32 addr)
/*
 * Function: 	soc_uc_mcs_start
 * Purpose:	Start MCS execution
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 *              addr - address to start at
 * Returns:	SOC_E_xxx
 */
{
    uint32      rst_control;

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

#endif /* BCM_CMICM_SUPPORT */

#ifdef BCM_IPROC_SUPPORT

/*
 * Function: 	soc_uc_iproc_init
 * Purpose:	Initialize iProc/MCS
 * Parameters:	unit - unit number
 * Returns:	SOC_E_xxxx
 */
static
int
soc_uc_iproc_init(int unit)
{
    int rc;
    uint32 sram_size, sram_addr, i;
 
#ifdef BCM_IPROC_DDR_SUPPORT
    if (soc_feature(unit, soc_feature_iproc_ddr)) {
        soc_iproc_ddr_init(unit);
    }
#endif /* BCM_IPROC_DDR_SUPPORT */

    rc = soc_uc_sram_extents(unit, &sram_addr, &sram_size);
    if (rc != SOC_E_NONE) {
        return (rc);
    }

    i = 0;
    while (i < sram_size) {
        soc_uc_mem_write(unit, sram_addr + i, 0);
        i += 4;
        if ((i & 0x0ffff) == 0) {
            sal_usleep(1000);
        } 
    }

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        /* JIRA CMICD-110:  The last 48 bytes of SRAM are used to cache the values for
         * CMIC_BS0_ and CMIC_BS1_ registers.  So write the current register values there
         */
        uint32 rval;

        /* store reset values in SRAM for BS0 */
        uint32 sram_saved_regs = sram_addr + 0x7ffd0;            /* end of 512K memory */
        READ_CMIC_BS0_CONFIGr(unit, &rval);
        soc_uc_mem_write(unit, sram_saved_regs + 0, rval);       /* CONFIG: mode, output enables */
        READ_CMIC_BS0_CLK_CTRLr(unit, &rval);
        soc_uc_mem_write(unit, sram_saved_regs + 4, rval);       /* CLK_CTRL: enable, bitclk high/low */
        READ_CMIC_BS0_HEARTBEAT_CTRLr(unit, &rval);
        soc_uc_mem_write(unit, sram_saved_regs + 8, rval);       /* HEARBEAT_CTRL: HB enable */
        READ_CMIC_BS0_HEARTBEAT_DOWN_DURATIONr(unit, &rval);
        soc_uc_mem_write(unit, sram_saved_regs + 12, 0x3ffffff); /* HEARTBEAT_DOWN_DURATION */
        READ_CMIC_BS0_HEARTBEAT_UP_DURATIONr(unit, &rval);
        soc_uc_mem_write(unit, sram_saved_regs + 16, 0x3ffffff); /* HEARTBEAT_UP_DURATION */

        /* store reset values in SRAM for BS1 */
        sram_saved_regs += 20;
        READ_CMIC_BS1_CONFIGr(unit, &rval);
        soc_uc_mem_write(unit, sram_saved_regs + 0, rval);       /* CONFIG: mode, output enables */
        READ_CMIC_BS1_CLK_CTRLr(unit, &rval);
        soc_uc_mem_write(unit, sram_saved_regs + 4, rval);       /* CLK_CTRL: enable, bitclk high/low */
        READ_CMIC_BS1_HEARTBEAT_CTRLr(unit, &rval);
        soc_uc_mem_write(unit, sram_saved_regs + 8, rval);       /* HEARBEAT_CTRL: HB enable */
        READ_CMIC_BS1_HEARTBEAT_DOWN_DURATIONr(unit, &rval);
        soc_uc_mem_write(unit, sram_saved_regs + 12, 0x3ffffff); /* HEARTBEAT_DOWN_DURATION */
        READ_CMIC_BS1_HEARTBEAT_UP_DURATIONr(unit, &rval);
        soc_uc_mem_write(unit, sram_saved_regs + 16, 0x3ffffff); /* HEARTBEAT_UP_DURATION */
    }
#endif

    return (SOC_E_NONE);
}

static
int
soc_uc_iproc_in_reset(int unit, int uC)
/*
 * Function: 	soc_uc_mcs_in_reset
 * Purpose:	Return 1 if in RESET state, 0 otherwise
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	1 or 0
 */
{
    uint32 rst;
    uint32 regval;

    READ_IHOST_PROC_RST_A9_CORE_SOFT_RSTNr(unit, &regval);
    if (uC == 0) {
        rst = soc_reg_field_get(unit, IHOST_PROC_RST_A9_CORE_SOFT_RSTNr,
                                regval, A9_CORE_0_SOFT_RSTNf);
    }
    else {
        rst = soc_reg_field_get(unit, IHOST_PROC_RST_A9_CORE_SOFT_RSTNr,
                                regval, A9_CORE_1_SOFT_RSTNf);
    }
    return (rst == 0);
}

/*
 * Function: 	soc_uc_iproc_reset
 * Purpose:	Put an IPROC into reset state
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	SOC_E_xxxx
 */
static
int
soc_uc_iproc_reset(int unit, int uC)
{
    int         rc;
    uint32      i;
    uint32      sram_base, sram_size;
    uint32      iproc_addr;
    uint32      regval;

    if (!soc_feature(unit, soc_feature_iproc)) {
        return (SOC_E_FAIL);
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "iproc_reset uC %d\n"), uC));

    if (SOC_IS_HELIX4(unit) || SOC_IS_KATANA2(unit)) {
        /* Assumes booting in QSPI mode with MDIO tied high */
        if (uC == 0) {
            rc = soc_uc_sram_extents(unit, &sram_base, &sram_size);
            if (rc != SOC_E_NONE) {
                return (rc);
            }
            iproc_addr = sram_base + sram_size - 8;

            /* load WFI loop into SRAM (ARM mode) */
            soc_cm_iproc_write(unit, iproc_addr, 0xe320f003);
            soc_cm_iproc_write(unit, iproc_addr + 4, 0xeafffffd);

            /* Update LUT to point at WFI loop */
            for (i = 0; i < 8; ++i) {
                soc_cm_iproc_write(unit, 0xffff0400 + i*4, iproc_addr);
            }
            /* core 0 should be in WFI now */
        }
    }

    /* Now safe to reset the ARM core */
    READ_IHOST_PROC_RST_A9_CORE_SOFT_RSTNr(unit, &regval);
    if (uC == 0) {
        soc_reg_field_set(unit, IHOST_PROC_RST_A9_CORE_SOFT_RSTNr,
                          &regval, A9_CORE_0_SOFT_RSTNf, 0);
        soc_reg_field_set(unit, IHOST_PROC_RST_A9_CORE_SOFT_RSTNr,
                          &regval, A9_CORE_1_SOFT_RSTNf, 0);
    }
    else {
        soc_reg_field_set(unit, IHOST_PROC_RST_A9_CORE_SOFT_RSTNr,
                          &regval, A9_CORE_1_SOFT_RSTNf, 0);
    }
    WRITE_IHOST_PROC_RST_WR_ACCESSr(unit, 0x00a5a501);
    WRITE_IHOST_PROC_RST_A9_CORE_SOFT_RSTNr(unit, regval);
    WRITE_IHOST_PROC_RST_WR_ACCESSr(unit, 0);

    return (SOC_E_NONE);
}

/*
 * Function: 	soc_uc_iproc_l2cache_purge
 * Purpose:	Clean l2 cache
 * Parameters:	unit - unit number
 *		start - start address
 *              len - length
 * Returns:	SOC_E_xxx
 */
static
int
soc_uc_iproc_l2cache_purge(int unit, uint32 start, uint32 len)
{
    uint32 addr;
    uint32 regval;
    uint32 linesize = 32;
    uint32 pcie0;
    uint32 pcie1;

    /* Save config and disable cache */
    if (soc_cm_get_bus_type(unit) & SOC_DEV_BUS_ALT) {
        /* PAXB-1 */
        READ_PAXB_1_PCIE_EP_AXI_CONFIGr(unit, &pcie1);
        WRITE_PAXB_1_PCIE_EP_AXI_CONFIGr(unit, 0);
    } else {
        /* PAXB-0 */
        READ_PAXB_0_PCIE_EP_AXI_CONFIGr(unit, &pcie0);
        WRITE_PAXB_0_PCIE_EP_AXI_CONFIGr(unit, 0);
    }

    READ_IHOST_L2C_CACHE_IDr(unit, &regval);
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "iproc_reset L2C_CACHE_ID 0x%08x\n"), regval));

    READ_IHOST_L2C_CONTROLr(unit, &regval);
    if (regval & 0x01) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "L2 cache enabled, clean %d bytes at 0x%08x\n"),
                     len, start));
        for (addr = start; addr < (start + len); addr += linesize) {
            WRITE_IHOST_L2C_CLEAN_PAr(unit, addr);
            WRITE_IHOST_L2C_CACHE_SYNCr(unit, addr);
        }
    }
    else {
        /* L2 cache disabled, skip */
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "L2 cache disabled 0x%08x\n"), regval));
    }

    /* Restore config values */
    if (soc_cm_get_bus_type(unit) & SOC_DEV_BUS_ALT) {
        WRITE_PAXB_1_PCIE_EP_AXI_CONFIGr(unit, pcie1);
    } else {
        WRITE_PAXB_0_PCIE_EP_AXI_CONFIGr(unit, pcie0);
    }

    return (SOC_E_NONE);
}

/*
 * Function: 	soc_uc_iproc_preload
 * Purpose:	Prepare to load into iProc memory
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	SOC_E_xxx
 */
static
int
soc_uc_iproc_preload(int unit, int uC)
{
    int rc;
    uint32 addr, size;

    rc = soc_uc_sram_extents(unit, &addr, &size);
    if (rc != SOC_E_NONE) {
        return (rc);
    }
    return soc_uc_iproc_l2cache_purge(unit, addr, size);
}

/*
 * Function: 	soc_uc_iproc_start
 * Purpose:	Start IPROC execution
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	SOC_E_xxx
 */
static
int
soc_uc_iproc_start(int unit, int uC, uint32 iproc_address)
{
    uint32      i;
    uint32      regval;
    uint32 iproc_addr = iproc_address + (16*1024); /* first 16K is MMU L1 table */

    if (!soc_feature(unit, soc_feature_iproc)) {
        return (SOC_E_FAIL);
    } 

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "iproc_start uC %d addr 0x%08x\n"), uC, iproc_addr));

    READ_IHOST_PROC_RST_A9_CORE_SOFT_RSTNr(unit, &regval);
    if (uC == 0) {
        /* Update LUT to point at SRAM */
        for (i = 0; i < 8; ++i) {
            soc_cm_iproc_write(unit, 0xffff0400 + i*4, iproc_addr);
        }
        /* Release reset on core 0 */
        soc_reg_field_set(unit, IHOST_PROC_RST_A9_CORE_SOFT_RSTNr,
                          &regval, A9_CORE_0_SOFT_RSTNf, 1);
    }
    else {
        /* Update LUT to point at SRAM */
        soc_cm_iproc_write(unit, CORE1_LUT_ADDR, iproc_addr);
        /* Release reset on core 1 */
        soc_reg_field_set(unit, IHOST_PROC_RST_A9_CORE_SOFT_RSTNr,
                          &regval, A9_CORE_1_SOFT_RSTNf, 1);
    }
    /* Release reset */
    WRITE_IHOST_PROC_RST_WR_ACCESSr(unit, 0x00a5a501);
    WRITE_IHOST_PROC_RST_A9_CORE_SOFT_RSTNr(unit, regval);
    WRITE_IHOST_PROC_RST_WR_ACCESSr(unit, 0);

    if (uC == 1) {
        /* Wait 100ms for core 1 to set up LUT */
        sal_usleep(100 * 1000);

        /* Update LUT to point at SRAM */
        soc_cm_iproc_write(unit, CORE1_LUT_ADDR, iproc_addr);
    }
    return (SOC_E_NONE);
}

#endif /* BCM_IPROC_SUPPORT */

int
soc_uc_init(int unit)
/*
 * Function: 	soc_uc_init
 * Purpose:	Initialize the CMICm MCS
 * Parameters:	unit - unit number
 * Returns:	SOC_E_xxx
 */
{
    if (!soc_feature(unit, soc_feature_uc)) {
        return (SOC_E_FAIL);
    } 
#ifdef BCM_IPROC_SUPPORT
    if (soc_feature(unit, soc_feature_iproc)) {
        return soc_uc_iproc_init(unit);
    } 
#endif /* BCM_IPROC_SUPPORT */
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_mcs)) {
        return soc_uc_mcs_init(unit);
    } 
#endif /* BCM_CMICM_SUPPORT */
    return (SOC_E_FAIL);
}

int
soc_uc_in_reset(int unit, int uC)
/*
 * Function: 	soc_uc_in_reset
 * Purpose:	Return reset state of core
 * Parameters:	unit - unit number
 *              uC - core number
 * Returns:	0 or 1
 */
{
    if (!soc_feature(unit, soc_feature_uc)) {
        return (SOC_E_FAIL);
    } 
#ifdef BCM_IPROC_SUPPORT
    if (soc_feature(unit, soc_feature_iproc)) {
        return soc_uc_iproc_in_reset(unit, uC);
    } 
#endif /* BCM_IPROC_SUPPORT */
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_mcs)) {
        return soc_uc_mcs_in_reset(unit, uC);
    } 
#endif /* BCM_CMICM_SUPPORT */
    return 1;
}

int
soc_uc_load(int unit, int uC, uint32 addr, int len, unsigned char *data)
/*
 * Function: 	soc_uc_load
 * Purpose:	Load a chunk into MCS memory
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 *              addr - Address of chunk from MCS perspective
 *              len - Length of chunk to load in bytes
 *              data - Pointer to data to load
 * Returns:	SOC_E_xxx
 */
{
    int i;
    uint32 wdata;
    int rc = SOC_E_NONE;        /* Be optimistic */

    if (!soc_feature(unit, soc_feature_uc)) {
        return (SOC_E_FAIL);
    }

    /* Convert the UC-centric address to a PCI address */
    addr = soc_uc_addr_to_pcie(unit, uC, addr);

    for (i = 0; i < len; i += 4, addr += 4) {
#ifndef LE_HOST
        /* We swap the bytes here for the LE ARMs. */
        wdata = _shr_swap32(*((uint32 *) &data[i]));
#else
        wdata = *((uint32 *) &data[i]);
#endif
        rc = soc_uc_mem_write(unit, addr, wdata);
        if (rc != SOC_E_NONE) {
            break;
        } 
    }
    return (rc);
}

int
soc_uc_preload(int unit, int uC)
/*
 * Function: 	soc_uc_preload
 * Purpose:	Prepare to load code
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	SOC_E_xxx
 */
{
    if (!soc_feature(unit, soc_feature_uc)) {
        return (SOC_E_FAIL);
    } 
#ifdef BCM_IPROC_SUPPORT
    if (soc_feature(unit, soc_feature_iproc)) {
        return soc_uc_iproc_preload(unit, uC);
    } 
#endif /* BCM_IPROC_SUPPORT */
#if 0
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_mcs)) {
        return soc_uc_mcs_preload(unit, uC);
    } 
#endif /* BCM_CMICM_SUPPORT */
#endif
    return (SOC_E_FAIL);
}

int
soc_uc_reset(int unit, int uC)
/*
 * Function: 	soc_uc_reset
 * Purpose:	Reset the requested core
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	SOC_E_xxx
 */
{
    if (!soc_feature(unit, soc_feature_uc)) {
        return (SOC_E_FAIL);
    } 
#ifdef BCM_IPROC_SUPPORT
    if (soc_feature(unit, soc_feature_iproc)) {
        return soc_uc_iproc_reset(unit, uC);
    } 
#endif /* BCM_IPROC_SUPPORT */
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_mcs)) {
        return soc_uc_mcs_reset(unit, uC);
    } 
#endif /* BCM_CMICM_SUPPORT */
    return (SOC_E_FAIL);
}

int
soc_uc_start(int unit, int uC, uint32 addr)
/*
 * Function: 	soc_uc_start
 * Purpose:	Start MCS execution
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	SOC_E_xxx
 */
{
    if (!soc_feature(unit, soc_feature_uc)) {
        return (SOC_E_FAIL);
    } 
#ifdef BCM_IPROC_SUPPORT
    if (soc_feature(unit, soc_feature_iproc)) {
        return soc_uc_iproc_start(unit, uC, addr);
    } 
#endif /* BCM_IPROC_SUPPORT */
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_mcs)) {
        return soc_uc_mcs_start(unit, uC, addr);
    } 
#endif /* BCM_CMICM_SUPPORT */
    return (SOC_E_FAIL);
}

char *soc_uc_firmware_version(int unit, int uC)
/*
 * Function: 	soc_uc_firmware_version
 * Purpose:	Return the firmware version string
 * Parameters:	unit - unit number
 *		uC - microcontroller num
 * Returns:	char * string pointer (to be freed with soc_cm_sfree())
 */
{
    int rc;
    mos_msg_data_t send;
    mos_msg_data_t reply;
    int len = 256;
    char *version_buffer;

    if (!soc_feature(unit, soc_feature_uc)) {
        return NULL;
    } 

    version_buffer = soc_cm_salloc(unit, len, "Version info buffer");
    if (version_buffer) {
        send.s.mclass = MOS_MSG_CLASS_SYSTEM;
        send.s.subclass = MOS_MSG_SUBCLASS_SYSTEM_VERSION;
        send.s.len = soc_htons(len);
        send.s.data = soc_htonl(soc_cm_l2p(unit, version_buffer));

        soc_cm_sinval(unit, version_buffer, len);

        rc = soc_cmic_uc_msg_send(unit, uC, &send, 5 * 1000 * 1000);
        if (rc != SOC_E_NONE) {
            soc_cm_sfree(unit, version_buffer);
            return NULL;
        }
        rc = soc_cmic_uc_msg_receive(unit, uC, MOS_MSG_CLASS_VERSION,
                                     &reply, 5 * 1000 * 1000);
        if (rc != SOC_E_NONE) {
            soc_cm_sfree(unit, version_buffer);
            return NULL;
        }
    }
    return version_buffer;
}

#endif
