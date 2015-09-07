/*
 * $Id: iproc.c,v 1.17 Broadcom SDK $
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
 * iProc support
 */

#include <shared/bsl.h>

#include <sal/core/boot.h>
#include <sal/core/libc.h>
#include <sal/types.h>
#include <shared/alloc.h>
#include <soc/memtune.h>

#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/cm.h>
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif
#include <ibde.h>

#ifdef BCM_IPROC_SUPPORT
#include <soc/iproc.h>

#ifdef BCM_IPROC_DDR_SUPPORT
#include <soc/shmoo_ddr40.h>
#include <sal/appl/config.h>
#endif

/*
 * Function:
 *      soc_iproc_init
 * Purpose:
 *      Initialize iProc subsystem
 * Parameters:
 *      unit - unit number
 * Returns:
 *      SOC_E_XXX
 */
int soc_iproc_init(int unit)
{
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_iproc_deinit
 * Purpose:
 *      Free up resources aquired by init.
 * Parameters:
 *      unit - unit number
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *       This function is currently not used. 
 */
int soc_iproc_deinit(int unit)
{
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_iproc_getreg
 * Purpose:
 *      Read iProc register outside CMIC
 * Parameters:
 *      unit - unit number
 * Returns:
 *      SOC_E_XXX
 */
int soc_iproc_getreg(int unit, uint32 addr, uint32 *data)
{
    *data = soc_cm_iproc_read(unit, addr);
    return SOC_E_NONE;
}


/*
 * Function:
 *      soc_iproc_setreg
 * Purpose:
 *      Write iProc register outside CMIC
 * Parameters:
 *      unit - unit number
 * Returns:
 *      SOC_E_XXX
 */
int soc_iproc_setreg(int unit, uint32 addr, uint32 data)
{
    soc_cm_iproc_write(unit, addr, data);
    return SOC_E_NONE;
}

/*
 * Function: 	soc_iproc_shutdown
 * Purpose:	Put an IPROC into sleep state
 * Parameters:	unit - unit number
 *              cpu_mask - mask of cores to shutdown
 *              level - shutdown level: 0=full, 1=partial
 * Returns:	SOC_E_xxxx
 */
int
soc_iproc_shutdown(int unit, uint32 cpu_mask, int level)
{
    uint32      i, rval;
    uint32      sram_base, sram_size;
    uint32      iproc_addr;

    if (!soc_feature(unit, soc_feature_iproc)) {
        return (SOC_E_FAIL);
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "Unit:%d iproc_shutdown\n"), unit));

    if (SOC_IS_HELIX4(unit)) {
        /* Assumes booting in QSPI mode with MDIO tied high */
        sram_base = 0x1b000000;
        sram_size = 512 * 1024;
        iproc_addr = sram_base + sram_size - 8;

        /* load WFI loop into SRAM (ARM mode) */
        soc_cm_iproc_write(unit, iproc_addr, 0xe320f003);
        soc_cm_iproc_write(unit, iproc_addr + 4, 0xeafffffd);

        /* Update LUT to point at WFI loop */
        for (i = 0; i < 8; ++i) {
            soc_cm_iproc_write(unit, 0xffff0400 + i*4, iproc_addr);
        }
        /* core 0 should be in WFI now */
        if (level == 0) {
            if (cpu_mask & 0x2) {
                READ_CRU_IHOST_PWRDWN_ENr(unit, &rval);
                soc_reg_field_set(unit, CRU_IHOST_PWRDWN_ENr, &rval, 
                                  LOGIC_CLAMP_ON_NEON1f, 1);
                WRITE_CRU_IHOST_PWRDWN_ENr(unit, rval);
                soc_reg_field_set(unit, CRU_IHOST_PWRDWN_ENr, &rval, 
                                  LOGIC_PWRDOWN_NEON1f, 1);
                WRITE_CRU_IHOST_PWRDWN_ENr(unit, rval);
                soc_reg_field_set(unit, CRU_IHOST_PWRDWN_ENr, &rval, 
                                  LOGIC_CLAMP_ON_CPU1f, 1);
                WRITE_CRU_IHOST_PWRDWN_ENr(unit, rval);
                soc_reg_field_set(unit, CRU_IHOST_PWRDWN_ENr, &rval, 
                                  RAM_CLAMP_ON_CPU1f, 1);
                WRITE_CRU_IHOST_PWRDWN_ENr(unit, rval);
                soc_reg_field_set(unit, CRU_IHOST_PWRDWN_ENr, &rval, 
                                  LOGIC_PWRDOWN_CPU1f, 1);
                WRITE_CRU_IHOST_PWRDWN_ENr(unit, rval);
                soc_reg_field_set(unit, CRU_IHOST_PWRDWN_ENr, &rval, 
                                  RAM_PWRDOWN_CPU1f, 1);
                WRITE_CRU_IHOST_PWRDWN_ENr(unit, rval);
            }
            /* Put cpu0 into shutdown after 100 clocks */
            READ_CRU_CPU0_POWERDOWNr(unit, &rval);
            soc_reg_field_set(unit, CRU_CPU0_POWERDOWNr, &rval, 
                              START_CPU0_POWERDOWN_SEQf, 1);
            soc_reg_field_set(unit, CRU_CPU0_POWERDOWNr, &rval, 
                              CPU0_POWERDOWN_TIMERf, 100);
            WRITE_CRU_CPU0_POWERDOWNr(unit, rval);
        }
    }
    return (SOC_E_NONE);
}

#ifdef BCM_IPROC_DDR_SUPPORT

#ifdef BCM_HURRICANE2_SUPPORT
const static unsigned int hr2_ddr_init_tab[] = {
     14, 0x00000001,
     36, 0x01000000,
     37, 0x10000000,
     38, 0x00100400,
     39, 0x00000400,
     40, 0x00000100,
     42, 0x00000001,
     61, 0x00010100,
     78, 0x01000200,
     79, 0x02000040,
     80, 0x00400100,
     81, 0x00000200,
     83, 0x01ffff0a,
     84, 0x01010101,
     85, 0x01010101,
     86, 0x0f000003,
     87, 0x0000010c,
     88, 0x00010000,
    112, 0x00000200,
    116, 0x0d000000,
    117, 0x00000028,
    119, 0x00010001,
    120, 0x00010001,
    121, 0x00010001,
    122, 0x00010001,
    123, 0x00010001,
    130, 0x00000001,
    139, 0x00000001,
    148, 0x00000001,
    149, 0x00000000,
    150, 0x00000000,
    152, 0x03030303,
    153, 0x03030303,
    156, 0x02006400,
    157, 0x02020202,
    158, 0x00020202,
    160, 0x01000000,
    161, 0x01010064,
    162, 0x01010101,
    163, 0x00000101,
    165, 0x00020000,
    166, 0x00000064,
    168, 0x000b0b00,
    170, 0x02000200,
    171, 0x02000200,
    175, 0x02000200,
    176, 0x02000200,
    180, 0x80000100,
    181, 0x04070303,
    182, 0x0000000a,
    185, 0x0010ffff,
    187, 0x0000000f,
    194, 0x00000204,
    205, 0x00000000,
    0xffffffff
};

const static unsigned int hr2_ddr_init_tab_800[] = { 
      0, 0x00000600,
      1, 0x00000000,
      3, 0x00000050,
      4, 0x000000c8,
      5, 0x0c050c00,
      6, 0x04040405,
      7, 0x06041018,
      8, 0x04101804,
      9, 0x0c040406,
     10, 0x03006db0,
     11, 0x0c040404,
     12, 0x03006db0,
     13, 0x01010004,
     15, 0x000c0c00,
     16, 0x03000200,
     17, 0x00001212,
     18, 0x06060000,
     19, 0x00000000,
     20, 0x00009001,
     21, 0x00900c28,
     22, 0x00050c28,
     23, 0x00000300,
     24, 0x000a0003,
     25, 0x0000000a,
     26, 0x00000000,
     27, 0x02000000,
     28, 0x0200006c,
     29, 0x0000006c,
     30, 0x05000001,
     31, 0x00050505,
     32, 0x00000000,
     35, 0x00000000,
     41, 0x00000000,
     43, 0x00000000,
     44, 0x00042000,
     45, 0x00000046,
     46, 0x00460420,
     47, 0x00000000,
     48, 0x04200000,
     49, 0x00000046,
     50, 0x00460420,
     51, 0x00000000,
     52, 0x04200000,
     53, 0x00000046,
     54, 0x00460420,
     55, 0x00000000,
     56, 0x04200000,
     57, 0x00000046,
     58, 0x00460420,
     59, 0x00000000,
     60, 0x00000000,
     62, 0x00000000,
     63, 0x00000000,
     64, 0x00000000,
     65, 0x00000000,
     66, 0x00000000,
     67, 0x00000000,
     68, 0x00000000,
     69, 0x00000000,
     70, 0x00000000,
     71, 0x00000000,
     72, 0x00000000,
     73, 0x00000000,
     74, 0x00000000,
     75, 0x00000000,
     76, 0x00000000,
     77, 0x00000000,
     82, 0x01010001,
     89, 0x00000000,
     90, 0x00000000,
     91, 0x00000000,
     92, 0x00000000,
     93, 0x00000000,
     94, 0x00000000,
     95, 0x00000000,
     96, 0x00000000,
     97, 0x00000000,
     98, 0x00000000,
     99, 0x00000000,
    100, 0x00000000,
    101, 0x00000000,
    102, 0x00000000,
    103, 0x00000000,
    104, 0x00000000,
    105, 0x00000000,
    106, 0x00000000,
    107, 0x00000000,
    108, 0x02040108,
    109, 0x08010402,
    110, 0x02020002,
    111, 0x01000200,
    113, 0x00000000,
    114, 0x00000000,
    115, 0x00000000,
    118, 0x00000000,
    124, 0x00000000,
    125, 0x00000000,
    126, 0x00000000,
    127, 0x00000000,
    128, 0x00212100,
    129, 0x21210001,
    131, 0x00000000,
    132, 0x00000000,
    133, 0x00012121,
    134, 0x00012121,
    135, 0x00000000,
    136, 0x00000000,
    137, 0x00212100,
    138, 0x21210001,
    140, 0x00000000,
    141, 0x00000000,
    142, 0x00012121,
    143, 0x00012121,
    144, 0x00000000,
    145, 0x00000000,
    146, 0x00212100,
    147, 0x21210001,
    151, 0x00000000,
    167, 0x00000000,
    169, 0x0c280000,
    172, 0x00000c28,
    173, 0x00007990,
    174, 0x0c280505,
    177, 0x00000c28,
    178, 0x00007990,
    179, 0x02020505,
    183, 0x00000000,
    184, 0x00000000,
    186, 0x00070303,
    188, 0x00000000,
    189, 0x00000000,
    190, 0x00000000,
    191, 0x00000000,
    192, 0x00000000,
    193, 0x00000000,
    195, 0x00000000,
    196, 0x00000000,
    197, 0x00000000,
    198, 0x00000000,
    199, 0x00000000,
    200, 0x00000000,
    201, 0x00000000,
    202, 0x00000004,
    203, 0x00000004,
    204, 0x00000000,
    206, 0x02040401,
    207, 0x00000002,
    208, 0x00000000,
    209, 0x00000000,
    210, 0x06060000,
    211, 0x00000606,
    212, 0x00000040,
    213, 0x00000000,
    214, 0x01010606,
    215, 0x00000101,
    216, 0x00001c1c,
    217, 0x00000000,
    0xffffffff
};

#endif /* BCM_HURRICANE2_SUPPORT */

/*
 * Function:
 *      soc_iproc_ddr_reg_table_init
 * Purpose:
 *      Initialize DDR registers based on the supplied table
 * Parameters:
 *      unit - unit number
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
soc_iproc_ddr_reg_table_init(int unit, int start, const unsigned int *tblptr)
{
    int rv;
    uint32 base;
	unsigned int offset;
    
    if (tblptr == NULL) {
        return SOC_E_PARAM;
    }

    /* Get register base address */
    base = soc_reg_addr(unit, start, REG_PORT_ANY, 0);
    
    offset = *tblptr;
	while(offset != 0xffffffff) {
		tblptr++;
        rv = soc_iproc_setreg(unit, base + offset * 4, *tblptr);
        if (rv != SOC_E_NONE) {
            return rv;
        }
		tblptr++;
		offset = *tblptr;
	}
    
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_iproc_ddr_init
 * Purpose:
 *      Initialize DDR of the embedded iProc subsystem
 * Parameters:
 *      unit - unit number
 * Returns:
 *      SOC_E_XXX
 */
int soc_iproc_ddr_init(int unit)
{
    if (!soc_feature(unit, soc_feature_iproc_ddr)) {
        return SOC_E_UNAVAIL;
    }
    
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {

        soc_timeout_t to;
        sal_usecs_t to_val;
        uint32 rval;
        
        /* Skip if DDR has been already initialized */
        SOC_IF_ERROR_RETURN(READ_DDR_DENALI_CTL_00r(unit, &rval));
        if (soc_reg_field_get(unit, DDR_DENALI_CTL_00r, rval, STARTf) == 1) {
            return SOC_E_NONE;
        }

        /* Set up default DDR configuration */
        SOC_DDR3_CLOCK_MHZ(unit) = DDR_FREQ_400;
        SOC_DDR3_MEM_GRADE(unit) = 0; /* to use default */

        /* Bring DDR controller out of reset */
        SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                                DDR_S1_IDM_RESET_CONTROLr, REG_PORT_ANY, 
                                RESETf, 0));
        SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                                DDR_S2_IDM_RESET_CONTROLr, REG_PORT_ANY, 
                                RESETf, 0));
        
        /* Set default speed */
        SOC_IF_ERROR_RETURN(READ_DDR_S1_IDM_IO_CONTROL_DIRECTr(unit, &rval));
        soc_reg_field_set(unit, DDR_S1_IDM_IO_CONTROL_DIRECTr, &rval, 
                                I_PHY_DDR_MHZf, SOC_DDR3_CLOCK_MHZ(unit));
        SOC_IF_ERROR_RETURN(WRITE_DDR_S1_IDM_IO_CONTROL_DIRECTr(unit, rval));
        
        /* Wait for PHY ready */
        to_val = 50000;     /* 50 mS */
        soc_timeout_init(&to, to_val, 0);
        for(;;) {
            SOC_IF_ERROR_RETURN(
                READ_DDR_PHY_CONTROL_REGS_REVISIONr(unit, &rval));
            if (rval != 0) {
                break;
            }
            if (soc_timeout_check(&to)) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit, 
                            "Timed out waiting for PHY to be ready\n")));

                return SOC_E_TIMEOUT;
            }
        }
        
        /* Set Strap and parameter per speed and grade */
        SOC_IF_ERROR_RETURN(
            READ_DDR_PHY_CONTROL_REGS_STRAP_CONTROLr(unit, &rval));
        soc_reg_field_set(unit, DDR_PHY_CONTROL_REGS_STRAP_CONTROLr, &rval, 
                                MHZf, SOC_DDR3_CLOCK_MHZ(unit));
        soc_reg_field_set(unit, DDR_PHY_CONTROL_REGS_STRAP_CONTROLr, &rval, 
                                AD_WIDTHf, 3);
        soc_reg_field_set(unit, DDR_PHY_CONTROL_REGS_STRAP_CONTROLr, &rval, 
                                BUS16f, 1);
        soc_reg_field_set(unit, DDR_PHY_CONTROL_REGS_STRAP_CONTROLr, &rval, 
                                CHIP_WIDTHf, 1);
        soc_reg_field_set(unit, DDR_PHY_CONTROL_REGS_STRAP_CONTROLr, &rval, 
                                CHIP_SIZEf, 3);
        soc_reg_field_set(unit, DDR_PHY_CONTROL_REGS_STRAP_CONTROLr, &rval, 
                                JEDEC_TYPEf, 25);
        soc_reg_field_set(unit, DDR_PHY_CONTROL_REGS_STRAP_CONTROLr, &rval, 
                                STRAPS_VALIDf, 1);
        SOC_IF_ERROR_RETURN(
            WRITE_DDR_PHY_CONTROL_REGS_STRAP_CONTROLr(unit, rval));
        SOC_IF_ERROR_RETURN(
            READ_DDR_PHY_CONTROL_REGS_STRAP_CONTROL2r(unit, &rval));
        soc_reg_field_set(unit, DDR_PHY_CONTROL_REGS_STRAP_CONTROL2r, &rval, 
                                DDR3f, 1);
        soc_reg_field_set(unit, DDR_PHY_CONTROL_REGS_STRAP_CONTROL2r, &rval, 
                                ALf, 3);
        soc_reg_field_set(unit, DDR_PHY_CONTROL_REGS_STRAP_CONTROL2r, &rval, 
                                CWLf, 9);
        soc_reg_field_set(unit, DDR_PHY_CONTROL_REGS_STRAP_CONTROL2r, &rval, 
                                CLf, 13);
        SOC_IF_ERROR_RETURN(
            WRITE_DDR_PHY_CONTROL_REGS_STRAP_CONTROL2r(unit, rval));
        
        /* Perform 40nm DDR PHY calibration */
        soc_ddr40_set_shmoo_dram_config(unit, 1);
        soc_ddr40_phy_calibrate(unit, 0, DDR_PHYTYPE_ENG, 0);
        
        /* DDR controller initialization */
        soc_iproc_ddr_reg_table_init(
            unit, DDR_DENALI_CTL_00r, hr2_ddr_init_tab);
        soc_iproc_ddr_reg_table_init(
            unit, DDR_DENALI_CTL_00r, hr2_ddr_init_tab_800);
        
        /* Start DDR controller */
        SOC_IF_ERROR_RETURN(READ_DDR_DENALI_CTL_00r(unit, &rval));
        soc_reg_field_set(unit, DDR_DENALI_CTL_00r, &rval, STARTf, 1);
        SOC_IF_ERROR_RETURN(WRITE_DDR_DENALI_CTL_00r(unit, rval));

        /* Wait for DDR ready */
        soc_timeout_init(&to, to_val, 0);
        for(;;) {
            SOC_IF_ERROR_RETURN(READ_DDR_DENALI_CTL_89r(unit, &rval));
            if (soc_reg_field_get(unit, DDR_DENALI_CTL_89r, rval, INT_STATUSf) 
                & 0x100) {
                break;
            }
            if (soc_timeout_check(&to)) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit, 
                      "Timed out waiting for DDR controller to be ready\n")));

                return SOC_E_TIMEOUT;
            }
        }
        
        /* Connect DDR port to BIST for running SHMOO */
        rval = 0;
        soc_reg_field_set(unit, DDR_BISTCONFIGr, &rval, 
                                BUS16_MODEf, 1);
        soc_reg_field_set(unit, DDR_BISTCONFIGr, &rval, 
                                ENABLE_8_BANKS_MODEf, 1);
        soc_reg_field_set(unit, DDR_BISTCONFIGr, &rval, 
                                AXI_PORT_SELf, 1);
        soc_reg_field_set(unit, DDR_BISTCONFIGr, &rval, 
                                BIST_RESETBf, 1);
        WRITE_DDR_BISTCONFIGr(unit, rval);
        rval = 0;
        
        /* Run or restore SHMOO */
        if (soc_property_get(unit, spn_DDR3_AUTO_TUNE, TRUE)) {
            soc_ddr40_shmoo_ctl(unit, 0, DDR_PHYTYPE_ENG, DDR_CTLR_T1, 0, 1);
            LOG_INFO(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit, "DDR tuning completed.\n")));
            
            soc_ddr40_shmoo_savecfg(unit, 0);
            if (soc_mem_config_set != NULL) {
                soc_mem_config_set("ddr3_auto_tune","0");
            }
            LOG_INFO(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit, 
                    "Please save the config to avoid re-tuning.\n")));
        } else {
            soc_ddr40_shmoo_restorecfg(unit, 0);
        }

        /* Connect DDR port back to NIC */
        READ_DDR_BISTCONFIGr(0, &rval);
        soc_reg_field_set(unit, DDR_BISTCONFIGr, &rval, AXI_PORT_SELf, 0);
        WRITE_DDR_BISTCONFIGr(0, rval);

        return SOC_E_NONE;

    }
#endif

    return SOC_E_UNAVAIL;
}
#endif /* BCM_IPROC_DDR_SUPPORT */

#endif
