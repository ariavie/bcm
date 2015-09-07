/*
 * $Id: shmoo_ddr40.h,v 1.13 Broadcom SDK $
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
 * DDR3 Memory support
 */
     
#ifndef _SHMOO_DDR40_H__
#define _SHMOO_DDR40_H__

#ifdef BCM_DDR3_SUPPORT

/**
 * ddr40 register access
 * m = memory, c = core, r = register, f = field, d = data.
 */
#define DDR40_REG_READ(_unit, _pc, flags, _reg_addr, _val) \
            soc_ddr40_phy_reg_ci_read((_unit), (_pc), (_reg_addr), (_val))
#define DDR40_REG_WRITE(_unit, _pc, _flags, _reg_addr, _val) \
            soc_ddr40_phy_reg_ci_write((_unit), (_pc), (_reg_addr), (_val))
#define DDR40_REG_MODIFY(_unit, _pc, _flags, _reg_addr, _val, _mask) \
            soc_ddr40_phy_reg_ci_modify((_unit), (_pc), (_reg_addr), (_val), (_mask))
#define DDR40_GET_FIELD(m,c,r,f) \
            GET_FIELD(m,c,r,f)
#define DDR40_SET_FIELD(m,c,r,f,d) \
            SET_FIELD(m,c,r,f,d)

enum drc_reg_set {
    DRC_AC_OPERATING_CONDITIONS_1r,
    DRC_AC_OPERATING_CONDITIONS_2r,
    DRC_AC_OPERATING_CONDITIONS_3r,
    DRC_AC_OPERATING_CONDITIONS_4r,
    DRC_AC_OPERATING_CONDITIONS_5r,
    DRC_ADDR_BANK_MAPr,
    DRC_ADDR_WITH_RD_CRCr,
    DRC_ADDR_WITH_WR_CRCr,
    DRC_ALIGN_ERR_INDICATIONr,
    DRC_AUXS_MUXr,
    DRC_BIST_CONFIGURATIONSr,
    DRC_BIST_DBI_ERR_OCCUREDr,
    DRC_BIST_EDC_ERR_OCCUREDr,
    DRC_BIST_END_ADDRESSr,
    DRC_BIST_ERROR_OCCURREDr,
    DRC_BIST_FULL_MASK_ERROR_COUNTERr,
    DRC_BIST_FULL_MASK_WORD_0r,
    DRC_BIST_FULL_MASK_WORD_1r,
    DRC_BIST_FULL_MASK_WORD_2r,
    DRC_BIST_FULL_MASK_WORD_3r,
    DRC_BIST_FULL_MASK_WORD_4r,
    DRC_BIST_FULL_MASK_WORD_5r,
    DRC_BIST_FULL_MASK_WORD_6r,
    DRC_BIST_FULL_MASK_WORD_7r,
    DRC_BIST_GLOBAL_ERROR_COUNTERr,
    DRC_BIST_LAST_ADDR_ERRr,
    DRC_BIST_LAST_DATA_ERRr,
    DRC_BIST_LAST_DATA_ERR_LSBr,
    DRC_BIST_LAST_DATA_ERR_MSBr,
    DRC_BIST_LAST_READ_DATA_LSBr,
    DRC_BIST_LAST_READ_DATA_MSBr,
    DRC_BIST_MPR_STAGGER_PATTERNSr,
    DRC_BIST_MPR_STAGGER_WEIGHTSr,
    DRC_BIST_NUMBER_OF_ACTIONSr,
    DRC_BIST_PATTERN_REPETITIONr,
    DRC_BIST_PATTERN_WORD_0r,
    DRC_BIST_PATTERN_WORD_1r,
    DRC_BIST_PATTERN_WORD_2r,
    DRC_BIST_PATTERN_WORD_3r,
    DRC_BIST_PATTERN_WORD_4r,
    DRC_BIST_PATTERN_WORD_5r,
    DRC_BIST_PATTERN_WORD_6r,
    DRC_BIST_PATTERN_WORD_7r,
    DRC_BIST_PRBS_ADDR_SEEDr,
    DRC_BIST_PRBS_DATA_SEED_LSBr,
    DRC_BIST_PRBS_DATA_SEED_MSBr,
    DRC_BIST_RATE_LIMITERr,
    DRC_BIST_REPEAT_PATTERN_1_LSBr,
    DRC_BIST_REPEAT_PATTERN_1_MSBr,
    DRC_BIST_REPEAT_PATTERN_2_LSBr,
    DRC_BIST_REPEAT_PATTERN_2_MSBr,
    DRC_BIST_REPEAT_PATTERN_3_LSBr,
    DRC_BIST_REPEAT_PATTERN_3_MSBr,
    DRC_BIST_REPEAT_PATTERN_4_LSBr,
    DRC_BIST_REPEAT_PATTERN_4_MSBr,
    DRC_BIST_REPEAT_PATTERN_5_LSBr,
    DRC_BIST_REPEAT_PATTERN_5_MSBr,
    DRC_BIST_REPEAT_PATTERN_6_LSBr,
    DRC_BIST_REPEAT_PATTERN_6_MSBr,
    DRC_BIST_REPEAT_PATTERN_7_LSBr,
    DRC_BIST_REPEAT_PATTERN_7_MSBr,
    DRC_BIST_SINGLE_BIT_MASKr,
    DRC_BIST_SINGLE_BIT_MASK_ERROR_COUNTERr,
    DRC_BIST_START_ADDRESSr,
    DRC_BIST_STATUSESr,
    DRC_CALIBRATION_SEQUENCE_ADDRESSr,
    DRC_CALIB_BIST_ERROR_OCCURREDr,
    DRC_CALIB_BIST_FULL_MASK_ERROR_COUNTERr,
    DRC_CLAM_SHELLr,
    DRC_CNT_BIST_DBI_ERR_GLOBALr,
    DRC_CNT_BIST_EDC_ERR_GLOBALr,
    DRC_CNT_GDDR_5_BIST_ADT_ERR_GLOBALr,
    DRC_CNT_GDDR_5_BIST_DATA_ERR_GLOBALr,
    DRC_CNT_GDDR_5_BIST_DBI_ERR_GLOBALr,
    DRC_CNT_GDDR_5_BIST_EDC_ERR_GLOBALr,
    DRC_CPU_COMMANDSr,
    DRC_DATA_LOCKr,
    DRC_DATA_LOCK_TIMEOUT_PRDr,
    DRC_DDR_2_EXTENDED_MODE_WR_3_REGISTERr,
    DRC_DDR_CONTROLLER_TRIGGERSr,
    DRC_DDR_EXTENDED_MODE_REGISTER_1r,
    DRC_DDR_EXTENDED_MODE_REGISTER_2r,
    DRC_DDR_EXTENDED_MODE_REGISTER_3r,
    DRC_DDR_MODE_REGISTER_1r,
    DRC_DDR_MODE_REGISTER_2r,
    DRC_DDR_PHY_PLL_CTRLr,
    DRC_DDR_PHY_PLL_FREQ_CTRLr,
    DRC_DDR_PHY_PLL_RCr,
    DRC_DDR_PHY_PLL_RESETr,
    DRC_DDR_PHY_PLL_STATUSr,
    DRC_DDR_PHY_UPDATE_ENABLEr,
    DRC_DPI_POWERr,
    DRC_DPI_STATUSr,
    DRC_DPI_STAT_CNTRLr,
    DRC_DPRC_ENABLERSr,
    DRC_DQ_BYTE_0_MAPr,
    DRC_DQ_BYTE_1_MAPr,
    DRC_DQ_BYTE_2_MAPr,
    DRC_DQ_BYTE_3_MAPr,
    DRC_DRAM_COMPLIANCE_CONFIGURATION_REGISTERr,
    DRC_DRAM_ERROR_DETECTr,
    DRC_DRAM_ERROR_INNITATEr,
    DRC_DRAM_ERROR_RECOVERYr,
    DRC_DRAM_INIT_FINISHEDr,
    DRC_DRAM_MR_0r,
    DRC_DRAM_MR_1r,
    DRC_DRAM_MR_2r,
    DRC_DRAM_MR_3r,
    DRC_DRAM_MR_4r,
    DRC_DRAM_MR_5r,
    DRC_DRAM_MR_6r,
    DRC_DRAM_MR_7r,
    DRC_DRAM_MR_8r,
    DRC_DRAM_MR_12r,
    DRC_DRAM_MR_14r,
    DRC_DRAM_MR_15r,
    DRC_DRAM_SETr,
    DRC_DRAM_SPECIAL_FEATURESr,
    DRC_DRAM_TIME_PARAMSr,
    DRC_DWF_WATERMARKr,
    DRC_ECC_1B_ERR_CNTr,
    DRC_ECC_2B_ERR_CNTr,
    DRC_ECC_ERR_1B_INITIATEr,
    DRC_ECC_ERR_1B_MONITOR_MEM_MASKr,
    DRC_ECC_ERR_2B_INITIATEr,
    DRC_ECC_ERR_2B_MONITOR_MEM_MASKr,
    DRC_ECC_INTERRUPT_REGISTERr,
    DRC_ECC_INTERRUPT_REGISTER_MASKr,
    DRC_ECC_INTERRUPT_REGISTER_TESTr,
    DRC_ENABLE_DYNAMIC_MEMORY_ACCESSr,
    DRC_ERROR_INITIATION_DATAr,
    DRC_EVENT_TRIGGER_ON_PARITY_PAD_CONFIGr,
    DRC_EXTENDED_MODE_WR_2_REGISTERr,
    DRC_EXT_PHY_CTRL_STATUSr,
    DRC_GDDR_5_BIST_ADT_ADDR_DQ_MAPr,
    DRC_GDDR_5_BIST_ADT_ERR_OCCUREDr,
    DRC_GDDR_5_BIST_ADT_PATTERNS_0r,
    DRC_GDDR_5_BIST_ADT_PATTERNS_1r,
    DRC_GDDR_5_BIST_CONFIGURATIONSr,
    DRC_GDDR_5_BIST_DATA_ERR_OCCUREDr,
    DRC_GDDR_5_BIST_DBI_ERR_OCCUREDr,
    DRC_GDDR_5_BIST_EDC_ERR_OCCUREDr,
    DRC_GDDR_5_BIST_FINISH_PRDr,
    DRC_GDDR_5_BIST_PRBS_ADT_SEEDr,
    DRC_GDDR_5_BIST_PRBS_DATA_SEED_LSBr,
    DRC_GDDR_5_BIST_PRBS_DATA_SEED_MSBr,
    DRC_GDDR_5_BIST_PRBS_DBI_SEEDr,
    DRC_GDDR_5_BIST_RD_FIFO_PATTERN_0r,
    DRC_GDDR_5_BIST_RD_FIFO_PATTERN_1r,
    DRC_GDDR_5_BIST_RD_FIFO_PATTERN_2r,
    DRC_GDDR_5_BIST_RD_FIFO_PATTERN_3r,
    DRC_GDDR_5_BIST_RD_FIFO_PATTERN_4r,
    DRC_GDDR_5_BIST_RD_FIFO_PATTERN_5r,
    DRC_GDDR_5_BIST_RD_FIFO_PATTERN_6r,
    DRC_GDDR_5_BIST_RD_FIFO_PATTERN_7r,
    DRC_GDDR_5_BIST_STATUSESr,
    DRC_GDDR_5_BIST_TOTAL_NUMBER_OF_COMMANDSr,
    DRC_GDDR_5_BIST_WR_RD_DBI_PATTERNr,
    DRC_GDDR_5_BIST_WR_RD_PATTERN_LSBr,
    DRC_GDDR_5_BIST_WR_RD_PATTERN_MSBr,
    DRC_GDDR_5_SPECIAL_CMD_TIMINGr,
    DRC_GDDR_BIST_LAST_READ_DATA_LSBr,
    DRC_GDDR_BIST_LAST_READ_DATA_MSBr,
    DRC_GENERAL_CONFIGURATIONSr,
    DRC_GLOBALFr,
    DRC_GLOBAL_MEM_OPTIONSr,
    DRC_GLUE_LOGIC_REGISTERr,
    DRC_GTIMER_CONFIGURATIONr,
    DRC_GTIMER_TRIGGERr,
    DRC_INDIRECT_COMMAND_ADDRESSr,
    DRC_INDIRECT_COMMAND_DATA_INCREMENTr,
    DRC_INDIRECT_COMMAND_RD_DATAr,
    DRC_INDIRECT_COMMAND_WR_DATAr,
    DRC_INDIRECT_FORCE_BUBBLEr,
    DRC_INITIALIZATION_CONTROLLr,
    DRC_INIT_SEQUENCE_REGISTERr,
    DRC_INTERRUPT_MASK_REGISTERr,
    DRC_INTERRUPT_REGISTERr,
    DRC_INTERRUPT_REGISTER_TESTr,
    DRC_INTIAL_CALIB_USE_MPRr,
    DRC_LOOPBACK_ADDR_CTRL_ERROR_OCCURREDr,
    DRC_LOOPBACK_ADDR_CTRL_FULL_ERR_CNTr,
    DRC_LOOPBACK_ADDR_CTRL_MASK_WORDr,
    DRC_LOOPBACK_ADDR_CTRL_PATTERN_WORDr,
    DRC_LOOPBACK_CONFIGr,
    DRC_LOOPBACK_CONTROLr,
    DRC_LOOPBACK_DBI_ERROR_OCCURREDr,
    DRC_LOOPBACK_DBI_FULL_ERR_CNTr,
    DRC_LOOPBACK_DBI_MASK_WORDr,
    DRC_LOOPBACK_DBI_PATTERN_WORDr,
    DRC_LOOPBACK_ERROR_OCCURREDr,
    DRC_LOOPBACK_FULL_ERR_CNTr,
    DRC_LOOPBACK_MASK_WORDr,
    DRC_LOOPBACK_PATTERN_WORDr,
    DRC_LOOPBACK_PRBS_ADDR_CTRL_SEEDr,
    DRC_LOOPBACK_PRBS_DATA_SEEDr,
    DRC_LOOPBACK_PRBS_DBI_SEEDr,
    DRC_ODT_CONFIGURATION_REGISTERr,
    DRC_PARITY_ERR_CNTr,
    DRC_PAR_ERR_INITIATEr,
    DRC_PAR_ERR_MEM_MASKr,
    DRC_PHY_CALIBRATIONr,
    DRC_PHY_CALIB_FINISHEDr,
    DRC_PPR_ACTIVEr,
    DRC_PPR_CFGr,
    DRC_PWD_CLOCK_CONTROLr,
    DRC_RBUS_ADDRr,
    DRC_RBUS_RDATAr,
    DRC_RBUS_RD_RESULTr,
    DRC_RBUS_WDATAr,
    DRC_REGISTER_ACCESS_RD_DATA_VALIDr,
    DRC_REGISTER_CONTROLr,
    DRC_REGISTER_CONTROL_RDATAr,
    DRC_REGISTER_CONTROL_WDATAr,
    DRC_REG_0085r,
    DRC_REG_0087r,
    DRC_REG_0090r,
    DRC_REG_90r,
    DRC_REG_0091r,
    DRC_REG_0092r,
    DRC_REG_0093r,
    DRC_REG_0209r,
    DRC_REG_0210r,
    DRC_REG_0211r,
    DRC_REG_0212r,
    DRC_REG_0242r,
    DRC_REG_0243r,
    DRC_REG_0300r,
    DRC_REG_0301r,
    DRC_REG_00ADr,
    DRC_REG_00AEr,
    DRC_REG_00AFr,
    DRC_REG_00B0r,
    DRC_REG_00B1r,
    DRC_REG_00B2r,
    DRC_REG_00B3r,
    DRC_REG_00B4r,
    DRC_REG_00B5r,
    DRC_REG_00B6r,
    DRC_REG_00B7r,
    DRC_REG_00B8r,
    DRC_REG_00BAr,
    DRC_REG_00BBr,
    DRC_REG_00BDr,
    DRC_REG_00BEr,
    DRC_REG_020Ar,
    DRC_REG_020Br,
    DRC_REG_020Cr,
    DRC_REG_020Dr,
    DRC_REG_020Er,
    DRC_REG_020Fr,
    DRC_REG_021Cr,
    DRC_REG_02FBr,
    DRC_REG_02FCr,
    DRC_REG_02FDr,
    DRC_REG_02FEr,
    DRC_REG_02FFr,
    DRC_RETRANSMIT_STATUSr,
    DRC_RXI_WATERMARKr,
    DRC_SBUS_BROADCAST_IDr,
    DRC_SBUS_LAST_IN_CHAINr,
    DRC_SHADOW_DRAM_MR_0r,
    DRC_SHADOW_DRAM_MR_1r,
    DRC_SHADOW_DRAM_MR_2r,
    DRC_SHADOW_DRAM_MR_3r,
    DRC_SHADOW_DRAM_MR_4r,
    DRC_SHADOW_DRAM_MR_5r,
    DRC_SHADOW_DRAM_MR_6r,
    DRC_SHADOW_DRAM_MR_7r,
    DRC_SHADOW_DRAM_MR_8r,
    DRC_SHADOW_DRAM_MR_12r,
    DRC_SHADOW_DRAM_MR_14r,
    DRC_SHADOW_DRAM_MR_15r,
    DRC_SPARE_REGISTERr,
    DRC_SPARE_REGISTER_3r,
    DRC_TRAIN_INIT_TRIGERSr,
    DRC_VDL_CNTRLr,
    DRC_WRITE_READ_RATESr,
    DRC_WR_CRC_WATERMARKr,
    DRC_REG_COUNT
};

/* Convenience Macros - Arad Memory Controller - Recheck when MemC files change */

uint32 DRC_REG_READ(int unit, uint32 channel, soc_reg_t reg, uint32 *rvp); 

uint32 DRC_REG_WRITE(int unit, uint32 channel, soc_reg_t reg, uint32 rv);

#define DRCA     DRCA_AC_OPERATING_CONDITIONS_1r
#define DRCB     DRCB_AC_OPERATING_CONDITIONS_1r
#define DRCC     DRCC_AC_OPERATING_CONDITIONS_1r
#define DRCD     DRCD_AC_OPERATING_CONDITIONS_1r
#define DRCE     DRCE_AC_OPERATING_CONDITIONS_1r
#define DRCF     DRCF_AC_OPERATING_CONDITIONS_1r
#define DRCG     DRCG_AC_OPERATING_CONDITIONS_1r
#define DRCH     DRCH_AC_OPERATING_CONDITIONS_1r
#define DRCALL   DRCBROADCAST_AC_OPERATING_CONDITIONS_1r

/* Conveniece Macros - recheck when ddr40.h changes */
#define DDR40_PHY_ADDR_CTL_MIN (0x0)
#define DDR40_PHY_ADDR_CTL_MAX (0x00c0)
#define DDR40_PHY_BYTE_LANE0_ADDR_MIN (0x0200)
#define DDR40_PHY_BYTE_LANE0_ADDR_MAX (0x03ac)
#define DDR40_PHY_BYTE_LANE1_ADDR_MIN (0x0400)
#define DDR40_PHY_BYTE_LANE1_ADDR_MAX (0x05ac)

#define READ_DDR40_PHY_WORD_LANE_x_VDL_OVRIDE_BYTEy_BITz_R_Pr(_x, _y, _z, _unit, _pc, _val) \
        DDR40_REG_READ((_unit), (_pc), 0x00, (0x00000234+(_x*0x200)+(_y*0xa0)+(_z*8)), (_val))

#define WRITE_DDR40_PHY_WORD_LANE_x_VDL_OVRIDE_BYTEy_BITz_R_Pr(_x, _y, _z, _unit, _pc, _val) \
        DDR40_REG_WRITE((_unit), (_pc), 0x00, (0x00000234+(_x*0x200)+(_y*0xa0)+(_z*8)), (_val))

#define WRITE_DDR40_PHY_WORD_LANE_x_VDL_OVRIDE_BYTEy_BITz_R_Nr(_x, _y, _z, _unit, _pc, _val) \
        DDR40_REG_WRITE((_unit), (_pc), 0x00, (0x00000238+(_x*0x200)+(_y*0xa0)+(_z*8)), (_val))

#define READ_DDR40_PHY_WORD_LANE_x_VDL_OVRIDE_BYTEy_BITz_Wr(_x, _y, _z, _unit,_pc,_val) \
        DDR40_REG_READ((_unit), (_pc), 0x00, (0x00000210+(_x*0x200)+(_y*0xa0)+(_z*4)), (_val))

#define WRITE_DDR40_PHY_WORD_LANE_x_VDL_OVRIDE_BYTEy_BITz_Wr(_x, _y, _z, _unit,_pc,_val) \
        DDR40_REG_WRITE((_unit), (_pc), 0x00, (0x00000210+(_x*0x200)+(_y*0xa0)+(_z*4)), (_val))


/* DDR Phy Registers Read/Write */
extern int soc_ddr40_phy_reg_ci_read(int unit, int ci, uint32 reg_addr, uint32 *reg_data);
extern int soc_ddr40_phy_reg_ci_write(int unit, int ci, uint32 reg_addr, uint32 reg_data);
extern int soc_ddr40_phy_reg_ci_modify(int unit, uint32 ci, uint32 reg_addr, uint32 data, uint32 mask);

extern int soc_ddr40_katana_phy_reg_ci_read(int unit, int ci, uint32 reg_addr, uint32 *reg_data);
extern int soc_ddr40_katana_phy_reg_ci_write(int unit, int ci, uint32 reg_addr, uint32 reg_data);
extern int soc_ddr40_katana_phy_reg_ci_modify(int unit, uint32 ci, uint32 reg_addr, uint32 data, uint32 mask);

extern int soc_ddr40_arad_phy_reg_ci_read(int unit, int ci, uint32 reg_addr, uint32 *reg_data);
extern int soc_ddr40_arad_phy_reg_ci_write(int unit, int ci, uint32 reg_addr, uint32 reg_data);
extern int soc_ddr40_arad_phy_reg_ci_modify(int unit, uint32 ci, uint32 reg_addr, uint32 data, uint32 mask);

/* DDR Registers Read/Write */
extern int soc_ddr40_read(int unit, int ci, uint32 addr, uint32 *pData0,
                uint32 *pData1, uint32 *pData2, uint32 *pData3,
                uint32 *pData4, uint32 *pData5, uint32 *pData6,
                uint32 *pData7);

extern int soc_ddr40_write(int unit, int ci, uint32 addr, uint32 uData0,
                uint32 uData1, uint32 uData2, uint32 uData3,
                uint32 uData4, uint32 uData5, uint32 uData6,
                uint32 uData7);

/* ------------------- */
/* PRE-TUNE Parameters */
/* ------------------- */
#define SHMOO_USE_PRETUNE 0
#define SHMOO_CI0_WL0_PRETUNE_RD_EN 13
#define SHMOO_CI0_WL0_PRETUNE_RD_DQS 47
#define SHMOO_CI0_WL0_PRETUNE_RD_DQ 37
#define SHMOO_CI0_WL0_PRETUNE_WR_DQ 30
#define SHMOO_CI0_WL1_PRETUNE_RD_EN (SHMOO_CI0_WL0_PRETUNE_RD_EN+8)
#define SHMOO_CI0_WL1_PRETUNE_RD_DQS (SHMOO_CI0_WL0_PRETUNE_RD_DQS-4)
#define SHMOO_CI0_WL1_PRETUNE_RD_DQ (SHMOO_CI0_WL0_PRETUNE_RD_DQ+8)
#define SHMOO_CI0_WL1_PRETUNE_WR_DQ (SHMOO_CI0_WL0_PRETUNE_WR_DQ+16)

#define SHMOO_CI0_WL0_PRETUNE_ADDR  35

#define SHMOO_CI2_WL0_PRETUNE_RD_EN (SHMOO_CI0_WL0_PRETUNE_RD_EN+2)
#define SHMOO_CI2_WL0_PRETUNE_RD_DQS SHMOO_CI0_WL0_PRETUNE_RD_DQS
#define SHMOO_CI2_WL0_PRETUNE_RD_DQ SHMOO_CI0_WL0_PRETUNE_RD_DQ
#define SHMOO_CI2_WL0_PRETUNE_WR_DQ SHMOO_CI0_WL0_PRETUNE_WR_DQ

/* --------------------- */
/* CI1 Offset Parameters */
/* --------------------- */
#define SHMOO_CI1_OFFSET_RD_EN         0
#define SHMOO_CI1_OFFSET_RD_DQS        0
#define SHMOO_CI1_OFFSET_RD_DQ         0        /* 15 */       /* 20 */     /* 10+5 */
#define SHMOO_CI1_OFFSET_WR_DQ         10       /* 20 */     /* 10 */

/* for the name consistency sake */
#define SHMOO_CI0_WL1_OFFSET_RD_EN     SHMOO_CI1_OFFSET_RD_EN
#define SHMOO_CI0_WL1_OFFSET_RD_DQS    SHMOO_CI1_OFFSET_RD_DQS
#define SHMOO_CI0_WL1_OFFSET_RD_DQ     SHMOO_CI1_OFFSET_RD_DQ
#define SHMOO_CI0_WL1_OFFSET_WR_DQ     SHMOO_CI1_OFFSET_WR_DQ

#define SHMOO_CI02_WL0_OFFSET_RD_EN    0
#define SHMOO_CI02_WL0_OFFSET_RD_DQS   0
#define SHMOO_CI02_WL0_OFFSET_RD_DQ    0        /* 10 */
#define SHMOO_CI02_WL0_OFFSET_WR_DQ    0        /* 10 */

/* Shmoo Functions */
#define SHMOO_INIT_VDL_RESULT 0
#define SHMOO_RD_EN 1
#define SHMOO_RD_DQ 2
#define SHMOO_WR_DQ 3
#define SHMOO_ADDRC 4
#define SHMOO_WR_DM 5 /* 13->9 */

#define DDR_PHYTYPE_RSVP       0
#define DDR_PHYTYPE_RSVP_STR   "rsvp"
#define DDR_PHYTYPE_NS         1
#define DDR_PHYTYPE_NS_STR     "ns"
#define DDR_PHYTYPE_ENG        2
#define DDR_PHYTYPE_ENG_STR    "eng"
#define DDR_PHYTYPE_AND        3
#define DDR_PHYTYPE_AND_STR    "and"
#define DDR_PHYTYPE_CE         4
#define DDR_PHYTYPE_CE_STR     "ce"
                               
#define DDR_CTLR_TRSVP         0
#define DDR_CTLR_TRSVP_STR     "trsvp" 
#define DDR_CTLR_T0            1
#define DDR_CTLR_T0_STR        "t0" 
#define DDR_CTLR_T1            2
#define DDR_CTLR_T1_STR        "t1"
#define DDR_CTLR_T2            3
#define DDR_CTLR_T2_STR        "t2"
#define DDR_CTLR_T3            4
#define DDR_CTLR_T3_STR        "t3"
                               
#define DDR_FREQ_400           400
#define DDR_FREQ_500           500
#define DDR_FREQ_533           533
#define DDR_FREQ_667           667
#define DDR_FREQ_800           800
#define DDR_FREQ_933           933
#define DDR_FREQ_1066          1066

#define MEM_GRADE_080808        0x080808
#define MEM_GRADE_090909        0x090909
#define MEM_GRADE_101010        0x101010
#define MEM_GRADE_111111        0x111111
#define MEM_GRADE_121212        0x121212
#define MEM_GRADE_131313        0x131313
#define MEM_GRADE_141414        0x141414

#define MEM_ROWS_16K        16384
#define MEM_ROWS_32K        32768

#define MEM_1GBITS_IN_MBYTES (128) /* 128Mbytes for 1 GBits */

enum freq_set {
   FREQ_400,
   FREQ_500,
   FREQ_533,
   FREQ_667,
   FREQ_800,
   FREQ_933,
   FREQ_1066,
   FREQ_COUNT
};

enum phy_set {
    PHY_RSVP,
    PHY_NS,
    PHY_ENG,
    PHY_AND,
    PHY_CE,
    PHY_COUNT
};

enum ctlr_set {
    CTLR_TRSVP,
    CTLR_T0,
    CTLR_T1,
    CTLR_T2,
    CTLR_T3,
    CTLR_COUNT
};

enum grade_set {
    GRADE_DEFAULT,
    GRADE_080808,
	GRADE_090909,
    GRADE_101010,
	GRADE_111111,
    GRADE_121212,
	GRADE_131313,
    GRADE_141414,
    GRADE_COUNT
};

enum mem_size_set {
    MEM_4G,
    MEM_2G,
    MEM_1G,
    MEM_COUNT
};

typedef struct {
    int ndiv;
    int mdiv;
} phy_freq_div_set_t;

typedef struct {
    int twl, twr, trc;  /* CONFIG0 */
    int trtw, twtr, tfaw, tread_enb;  /* CONFIG2 */
    int bank_unavail_rd, bank_unavail_wr;  /* CONFIG3 */
    int rr_read, rr_write;  /* CONFIG3 */
    int refrate; /* CONFIG4 */
    int trp_read, trp_write; /* CONFIG6 */
    int trfc[MEM_COUNT]; /* CONFIG6 */
    int cl, cwl, wr; /* PHY_STRAP0 */
    int jedec, mhz; /* PHY_STRAP1 */
    uint32 mr0,mr2;
} KATANAfreq_grade_mem_set_set_t;

typedef struct {
    int trp_read, trp_write, trrd, tmrdtmod, tfaw, trc;  /* CONFIG0 */
    int tread_enb, twl, trdtwr, twrtrd;                  /* CONFIG1 */
    int disable_tmu_schedule_refresh, trefi;             /* CONFIG3 */
    int tzqci;                                           /* CONFIG4 */
    int tzqcs, tzqoper, tzqinit;                         /* CONFIG5 */
    int tref_holdoff;                                    /* CONFIG6 */
    int al, cl, cwl, wr;                                 /* PHY_STRAP0 */
    int jedec, mhz;                                      /* PHY_STRAP1 */
    uint32 mr0_clear, mr0_set, mr2_clear, mr2_set;       /* MR0, MR2   */
    int tmb_tfaw, tmb_trc, tmb_refresh, tmb_refresh_delay;  /* TMB config */
} CALADAN3freq_grade_mem_set_set_t;

#if 0 /* Uncomment this when required */
typedef struct {
    uint32 Jedec;
    uint32 Cl;
    uint32 Cwl;
    uint32 Wr;
    uint32 DdrMhz;
    uint32 ReadDataDelay;
    uint32 ModeRegWr1;
    uint32 ModeRegWr2;
    uint32 ExtModeWr1;
    uint32 ExtModeWr2;
    uint32 ExtModeWr3;
    uint32 EMR2;
    uint32 EMR3;
    uint32 DDRtRST;
    uint32 DDRtDLL;
    uint32 DDRtRC;
    uint32 DDRtRRD;
    uint32 DDRtRFC;
    uint32 DDRtRCDR;
    uint32 DDRtRCDW;
    uint32 InitWaitPRD;
    uint32 CntRASRDPRD;
    uint32 CntRASWRPRD;
    uint32 CntRDAPPRD;
    uint32 CntWRAPPRD;
    uint32 IndWrRdAddrMode;
    uint32 NumCols;
    uint32 APBitPos;
    uint32 RefreshBurstSize;
    uint32 RefreshDelayPrd;
    uint32 DDRtZQCS;
    uint32 DDRtFAW;
    uint32 CntRdPrd;
    uint32 CntWrPrd;
    uint32 Enable8Banks;
    uint32 DDRResetPolarity;
    uint32 StaticOdtEn;
    uint32 AddrTermHalf;
    uint32 DRAMType;
    uint32 BurstSizeMode;
    uint32 WrLatency;
    uint32 CntRDWRPRD;
    uint32 CntWRRDPRD;
    uint32 DDRtREFI;
    uint32 DynOdtLength;
    uint32 DynOdtStartDelay;
    uint32 DDR3ZQCalibGenPrd;
    uint32 ReadDelay;
    uint32 IntialCalibMprAddr;
} ARADfreq_grade_mem_set_set_t;
#endif

typedef struct {
    int chip_siz;
    int ad_width;
} size_mem_type_set_t;

typedef struct {
    char result[129];
} bit_shmoo;

typedef struct {
    bit_shmoo bs[16];
    uint32 step[16];
    uint32 raw_result[132];
    uint32 uncapped_rd_en_step[2];
    bit_shmoo bytes[2];
} word_shmoo;
typedef word_shmoo vref_word_shmoo[64];

typedef struct soc_ddr_shmoo_param_s {
    int type, wl;
    uint32 result[132];
    uint32 init_step[16];
    uint32 new_step[16];
    vref_word_shmoo *vwsPtr;
} soc_ddr_shmoo_param_t;

extern void soc_ddr40_set_shmoo_dram_config(uint32 unit, uint32 dram_config);
extern int soc_ddr40_phy_pll_ctl  (int u, int ci, uint32 freq, uint32 phyType, int stat);
extern int soc_ddr40_phy_calibrate(int u, int ci, uint32 phyType, int stat);
extern int soc_ddr40_ctlr_ctl     (int u, int ci, uint32 ctlType, int stat);
extern int soc_ddr40_shmoo_ctl    (int u, int ci, uint32 phyType, uint32 ctlType, int stat, int isplot);
extern int soc_ddr40_shmoo_savecfg(int unit, int ci);
extern int soc_ddr40_shmoo_restorecfg(int unit, int ci);
extern int soc_ddr40_phy_pll_rst  (int u, int ci, uint32 phyType, int cnt);
extern int soc_ddr40_ctlr_reset   (int u, int ci, uint32 ctlType, int stat);
extern int soc_ddr40_ctlr_zqcal_ctl(int u, int ci, uint32 ctlType, int stat);

#endif /* DDR3 Support */

#endif /* _ESW_DDR40_H__ */
