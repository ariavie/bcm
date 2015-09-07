/*
 * $Id: miim.c,v 1.70 Broadcom SDK $
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
 * MII Management (MDIO) bus access routines for reading and writing PHY
 * registers.
 *
 * BCM5690 has an internal MDIO bus in addition to the external one.
 * The internal bus is accessed by setting bit 7 in the PHY ID.
 */

#include <shared/bsl.h>

#include <sal/core/libc.h>
#include <sal/core/boot.h>

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/error.h>
#include <soc/cmic.h>
#include <soc/phyreg.h>
#include <soc/linkctrl.h>
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif

#ifdef BCM_SBX_SUPPORT
#include <soc/sbx/sbx_drv.h>
#endif

#if defined(BCM_DFE_SUPPORT) || defined(BCM_DPP_SUPPORT)

int
soc_dcmn_miim_operation(int unit, int is_write, int clause, uint32 phy_id,
                  uint32 phy_reg_addr, uint16 *phy_data)
{
    int rv = SOC_E_NONE;
    int clause45;
    uint32 reg_val, real_phy_id, real_bus_id, internal_select, lsrv;

    clause45 = (45 == clause);

    /* Pause link scanning */
    rv = soc_linkctrl_linkscan_pause(unit);
    if (rv != SOC_E_NONE) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_linkctrl_linkscan_pause failed\n")));
        return rv;
    }

    /* hold MIIM lock */
    MIIM_LOCK(unit);

    /*  write reg_addr and c45 extention address or if c22: just reg addr */
    reg_val = 0;
    if (clause45){
        soc_reg_field_set(unit, CMIC_MIIM_ADDRESSr, &reg_val, CLAUSE_45_REGADRf, SOC_PHY_CLAUSE45_REGAD(phy_reg_addr));
        soc_reg_field_set(unit, CMIC_MIIM_ADDRESSr, &reg_val, CLAUSE_45_DTYPEf,  SOC_PHY_CLAUSE45_DEVAD(phy_reg_addr));
    } else {
        soc_reg_field_set(unit, CMIC_MIIM_ADDRESSr, &reg_val, CLAUSE_22_REGADRf, phy_reg_addr);
    }
    WRITE_CMIC_MIIM_ADDRESSr(unit, reg_val);

    /* write the parameters */
    reg_val = 0;
    internal_select = (phy_id & 0x80) ? 1 : 0;
    real_phy_id = phy_id & ~(PHY_ID_BUS_UPPER_MASK | PHY_ID_BUS_LOWER_MASK);
    real_phy_id &= ~(0x80); /*clear internal / external bit*/ 
    real_bus_id = PHY_ID_BUS_NUM(phy_id);

    if(bsl_check(bslLayerSoc, bslSourceCommon, bslSeverityVerbose, unit) || is_write) {
        if (clause45){
        LOG_INFO(BSL_LS_SOC_MIIM,
                 (BSL_META_U(unit,
                             "soc_dcmn_miim_operation cl45 %s: "
                             "(id=0x%02x readr=0x%x dtype=0x%x data=0x%04x clause45=%d, real_phy_id=%d, real_bus_id=%d)\n"),
                  is_write ? "write" : "read ", phy_id, SOC_PHY_CLAUSE45_REGAD(phy_reg_addr), SOC_PHY_CLAUSE45_DEVAD(phy_reg_addr), *phy_data, clause45,
                  real_phy_id,real_bus_id));
        } else {
        LOG_INFO(BSL_LS_SOC_MIIM,
                 (BSL_META_U(unit,
                             "soc_dcmn_miim_operation cl22 %s: "
                             "(id=0x%02x addr=0x%02x data=0x%04x clause45=%d, real_phy_id=%d, real_bus_id=%d)\n"),
                  is_write ? "write" : "read ", phy_id, phy_reg_addr, *phy_data, clause45,real_phy_id,real_bus_id));
        }
    } 


    soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &reg_val, INTERNAL_SELf, internal_select);
    soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &reg_val, C45_SELf, clause45);
    soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &reg_val, PHY_IDf, real_phy_id);
    if(is_write) {
        soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &reg_val, PHY_DATAf, *phy_data);
    } else {
        soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &reg_val, PHY_DATAf, 0x0);
    }
    soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &reg_val, BUS_IDf, real_bus_id);
    WRITE_CMIC_MIIM_PARAMr(unit, reg_val);

    /*  start the access */
    READ_CMIC_SCHAN_CTRLr(unit, &reg_val);
    if(is_write) {
        /*MIIM_WR_STARTf*/
        WRITE_CMIC_SCHAN_CTRLr(unit, 0x91);
    } else {
        /*MIIM_RD_STARTf*/
        WRITE_CMIC_SCHAN_CTRLr(unit, 0x90);
    }


    /* Wait for completion using either the interrupt or polling method */

    if (SOC_CONTROL(unit)->miimIntrEnb) {
        soc_intr_enable(unit, IRQ_MIIM_OP_DONE);

        if (sal_sem_take(SOC_CONTROL(unit)->miimIntr,
                         SOC_CONTROL(unit)->miimTimeout) != 0) {
            rv = SOC_E_TIMEOUT;
        } else {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Interrupt received\n")));
        }

        soc_intr_disable(unit, IRQ_MIIM_OP_DONE);     

    } else {
        soc_timeout_t to;

        soc_timeout_init(&to, SOC_CONTROL(unit)->miimTimeout, 2500);

        while ((soc_pci_read(unit, SOC_REG_INFO(unit,CMIC_SCHAN_CTRLr).offset) &
                SC_MIIM_OP_DONE_TST) == 0) {
            if (soc_timeout_check(&to)) {
                rv = SOC_E_TIMEOUT;
                break;
            }
        }
        
        if (rv == SOC_E_NONE) {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Done in %d polls\n"), to.polls));
        }

        WRITE_CMIC_SCHAN_CTRLr(unit, SC_MIIM_OP_DONE_CLR);
        
    }

    if (rv == SOC_E_TIMEOUT) {
        if(is_write) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "MiimTimeOut:soc_dcmn_miim_operation write, "
                                  "timeout (id=0x%02x addr=0x%02x data=0x%04x)\n"),
                       phy_id, phy_reg_addr, *phy_data));
        } else /*read*/{
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "MiimTimeOut:soc_dcmn_miim_operation read, "
                                  "timeout (id=0x%02x addr=0x%02x)\n"),
                       phy_id, phy_reg_addr));
        }
        SOC_CONTROL(unit)->stat.err_mii_tmo++;
    }

    /* read data */
    if(SOC_E_NONE == rv && !is_write) {
        *phy_data = (uint16)soc_pci_read(unit, SOC_REG_INFO(unit,CMIC_MIIM_READ_DATAr).offset);
        if(bsl_check(bslLayerSoc, bslSourceCommon, bslSeverityVerbose, unit)) {
            LOG_INFO(BSL_LS_SOC_MIIM,
                     (BSL_META_U(unit,
                                 "soc_dcmn_miim_operation read data: %d \n"),*phy_data));
        } 
    }

    /* Release linkscan pause and MIIM lock */
    MIIM_UNLOCK(unit);

    lsrv = soc_linkctrl_linkscan_continue(unit);
    if(SOC_E_NONE != lsrv) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_linkctrl_linkscan_continue failed\n")));
        rv = lsrv;
    }

    return rv;
}

#if defined(BCM_CMICM_SUPPORT)

int
soc_dcmn_cmicm_miim_operation(int unit, int is_write, int clause, uint32 phy_id,
                  uint32 phy_reg_addr, uint16 *phy_data)
{
    int rv = SOC_E_NONE;
    int clause45, cmc;
    uint32 reg_val, real_phy_id, real_bus_id, internal_select, lsrv;

    cmc = SOC_PCI_CMC(unit);

    clause45 = (45 == clause);

    /* Pause link scanning */
    rv = soc_linkctrl_linkscan_pause(unit);
    if (rv != SOC_E_NONE) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_linkctrl_linkscan_pause failed\n")));
        return rv;
    }

    MIIM_LOCK(unit);

    /*  write reg_addr and c45 extention address or if c22: just reg addr */
    reg_val = 0;
    if (clause45){
        soc_reg_field_set(unit, CMIC_CMC0_MIIM_ADDRESSr, &reg_val, CLAUSE_45_REGADRf, SOC_PHY_CLAUSE45_REGAD(phy_reg_addr));
        soc_reg_field_set(unit, CMIC_CMC0_MIIM_ADDRESSr, &reg_val, CLAUSE_45_DTYPEf,  SOC_PHY_CLAUSE45_DEVAD(phy_reg_addr));
    } else {
        soc_reg_field_set(unit, CMIC_CMC0_MIIM_ADDRESSr, &reg_val, CLAUSE_22_REGADRf, phy_reg_addr);
    }
    soc_pci_write(unit, CMIC_CMCx_MIIM_ADDRESS_OFFSET(cmc), reg_val);

    /* write the parameters */
    reg_val = 0;
    internal_select = (phy_id & 0x80) ? 1 : 0;
    real_phy_id = phy_id & ~(PHY_ID_BUS_UPPER_MASK | PHY_ID_BUS_LOWER_MASK);
    real_phy_id &= ~(0x80); /*clear internal / external bit*/ 
    real_bus_id = PHY_ID_BUS_NUM(phy_id);

    if(bsl_check(bslLayerSoc, bslSourceCommon, bslSeverityVerbose, unit) || is_write) {
        if (clause45){
            LOG_INFO(BSL_LS_SOC_MIIM,
                     (BSL_META_U(unit,
                                 "soc_dcmn_cmicm_miim_operation cl45 %s: "
                                 "(id=0x%02x readr=0x%x dtype=0x%x data=0x%04x clause45=%d, real_phy_id=%d, real_bus_id=%d)\n"),
                      is_write ? "write" : "read ", phy_id, SOC_PHY_CLAUSE45_REGAD(phy_reg_addr), SOC_PHY_CLAUSE45_DEVAD(phy_reg_addr), *phy_data, clause45,
                      real_phy_id,real_bus_id));
        } else {
            LOG_INFO(BSL_LS_SOC_MIIM,
                     (BSL_META_U(unit,
                                 "soc_dcmn_cmicm_miim_operation cl22 %s: "
                                 "(id=0x%02x addr=0x%02x data=0x%04x clause45=%d, real_phy_id=%d, real_bus_id=%d)\n"),
                      is_write ? "write" : "read ", phy_id, phy_reg_addr, *phy_data, clause45,real_phy_id,real_bus_id));
        }
    } 


    soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &reg_val, INTERNAL_SELf, internal_select);
    soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &reg_val, C45_SELf, clause45);
    soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &reg_val, PHY_IDf, real_phy_id);
    if(is_write) {
        soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &reg_val, PHY_DATAf, *phy_data);
    } else {
        soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &reg_val, PHY_DATAf, 0x0);
    }
    soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &reg_val, BUS_IDf, real_bus_id);
    soc_pci_write(unit, CMIC_CMCx_MIIM_PARAM_OFFSET(cmc), reg_val);

    /*  start the access */
    reg_val = soc_pci_read(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc));
    if(is_write) {
        soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &reg_val, MIIM_WR_STARTf, 1);
    } else {
        soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &reg_val, MIIM_RD_STARTf, 1);
    }
    soc_pci_write(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc), reg_val);


    /* Wait for completion using either the interrupt or polling method */

    if (SOC_CONTROL(unit)->miimIntrEnb) {

        soc_cmicm_intr0_enable(unit, IRQ_CMCx_MIIM_OP_DONE);

        if (sal_sem_take(SOC_CONTROL(unit)->miimIntr,
                         SOC_CONTROL(unit)->miimTimeout) != 0) {
            rv = SOC_E_TIMEOUT;
        } else {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Interrupt received\n")));
        }

        soc_cmicm_intr0_disable(unit, IRQ_CMCx_MIIM_OP_DONE);

    } else {
        soc_timeout_t to;

        soc_timeout_init(&to, SOC_CONTROL(unit)->miimTimeout, 2500);

        while ((soc_pci_read(unit, CMIC_CMCx_MIIM_STAT_OFFSET(cmc)) &
                CMIC_MIIM_OPN_DONE) == 0) {
            if (soc_timeout_check(&to)) {
                rv = SOC_E_TIMEOUT;
                break;
            }
        }
        
        if (rv == SOC_E_NONE) {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Done in %d polls\n"), to.polls));
        }

        reg_val = soc_pci_read(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc));
        soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &reg_val, MIIM_RD_STARTf, 0);
        soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &reg_val, MIIM_WR_STARTf, 0);
        soc_pci_write(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc), reg_val);
        
    }

    if (rv == SOC_E_TIMEOUT) {
        if(is_write) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "MiimTimeOut:soc_dcmn_miim_operation write, "
                                  "timeout (id=0x%02x addr=0x%02x data=0x%04x)\n"),
                       phy_id, phy_reg_addr, *phy_data));
        } else /*read*/{
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "MiimTimeOut:soc_dcmn_miim_operation read, "
                                  "timeout (id=0x%02x addr=0x%02x)\n"),
                       phy_id, phy_reg_addr));
        }
        SOC_CONTROL(unit)->stat.err_mii_tmo++;
    }

    /* read data */
    if(SOC_E_NONE == rv && !is_write) {
        *phy_data = (uint16)soc_pci_read(unit, CMIC_CMCx_MIIM_READ_DATA_OFFSET(cmc));
        if(bsl_check(bslLayerSoc, bslSourceCommon, bslSeverityVerbose, unit)) {
            LOG_INFO(BSL_LS_SOC_MIIM,
                     (BSL_META_U(unit,
                                 "soc_dcmn_miim_operation read data: %d \n"),*phy_data));
        } 

    }

    /* Release linkscan pause and MIIM lock */
    MIIM_UNLOCK(unit);

    lsrv = soc_linkctrl_linkscan_continue(unit);
    if(SOC_E_NONE != lsrv) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_linkctrl_linkscan_continue failed\n")));
        rv = lsrv;
    }


    return rv;
}

#endif /*BCM_CMICM_SUPPORT*/

/*
 * Function:
 *      soc_dcmn_miim_write
 * Purpose:
 *      Interface to write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 */

/*user defined function for external phy*/
#ifdef USER_DEFINED_EXT_MDIO_FUNCTION
extern int _user_defined_ext_mdio_write(int unit, int clause, uint32 phy_id,
                   uint32 phy_reg_addr, uint16 phy_wr_data);
#endif 

int 
soc_dcmn_miim_write(int unit, int clause, uint32 phy_id,
                   uint32 phy_reg_addr, uint16 phy_wr_data)
{
#ifdef USER_DEFINED_EXT_MDIO_FUNCTION
    uint32 internal_select;
#endif 

#ifdef USER_DEFINED_EXT_MDIO_FUNCTION
    internal_select = (phy_id & 0x80) ? 1 : 0;
    if (!internal_select) {
        return _user_defined_ext_mdio_write(unit, clause, phy_id, phy_reg_addr, phy_wr_data);
    }
#endif 

#if defined(BCM_CMICM_SUPPORT)
    if(soc_feature(unit, soc_feature_cmicm)) {
        return soc_dcmn_cmicm_miim_operation(unit, TRUE, clause, phy_id,
                          phy_reg_addr, &phy_wr_data);
    } else 
#endif
    {
        return soc_dcmn_miim_operation(unit, TRUE, clause, phy_id,
                          phy_reg_addr, &phy_wr_data);
    }
}


int 
soc_dcmn_miim_cl22_write(int unit, uint32 phy_id,
                   uint32 phy_reg_addr, uint16 phy_wr_data)
{
    return soc_dcmn_miim_write(unit, 22, phy_id, phy_reg_addr, phy_wr_data);
}

int 
soc_dcmn_miim_cl45_write(int unit, uint32 phy_id,
                   uint32 phy_reg_addr, uint16 phy_wr_data)
{
    return soc_dcmn_miim_write(unit, 45, phy_id, phy_reg_addr, phy_wr_data);
}



/*
 * Function:
 *      soc_dcmn_miim_read
 * Purpose:
 *      Interface to read a value from a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - Data read.
 * Returns:
 *      SOC_E_XXX
 */
/*user defined function for external phy*/
#ifdef USER_DEFINED_EXT_MDIO_FUNCTION
extern int _user_defined_ext_mdio_read(int unit,  int clause, uint32 phy_id,
                  uint32 phy_reg_addr, uint16 *phy_rd_data);
#endif 

int
soc_dcmn_miim_read(int unit, int clause, uint32 phy_id,
                  uint32 phy_reg_addr, uint16 *phy_rd_data)
{
#ifdef USER_DEFINED_EXT_MDIO_FUNCTION
    uint32 internal_select;
#endif
 

#ifdef USER_DEFINED_EXT_MDIO_FUNCTION
    internal_select = (phy_id & 0x80) ? 1 : 0;
    if (!internal_select) {
        return _user_defined_ext_mdio_read(unit, clause, phy_id, phy_reg_addr, phy_rd_data);
    }
#endif 

#if defined(BCM_CMICM_SUPPORT)
    if(soc_feature(unit, soc_feature_cmicm)) {
        return soc_dcmn_cmicm_miim_operation(unit, FALSE, clause, phy_id,
                          phy_reg_addr, phy_rd_data);
    } else 
#endif
    {
        return soc_dcmn_miim_operation(unit, FALSE, clause, phy_id,
                          phy_reg_addr, phy_rd_data);
    }
}


int
soc_dcmn_miim_cl22_read(int unit, uint32 phy_id,
                  uint32 phy_reg_addr, uint16 *phy_rd_data)
{
    return soc_dcmn_miim_read(unit, 22, phy_id, phy_reg_addr, phy_rd_data);
}

int
soc_dcmn_miim_cl45_read(int unit, uint32 phy_id,
                  uint32 phy_reg_addr, uint16 *phy_rd_data)
{
    return soc_dcmn_miim_read(unit, 45, phy_id, phy_reg_addr, phy_rd_data);
}

#endif /*BCM_DFE_SUPPORT || BCM_DPP_SUPPORT*/



#if defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT)
/*
 */

/*
 * Function:
 *      soc_rcpu_miim_write
 * Purpose:
 *      Write a value to a MIIM register throuth RCPU packets.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 *      If remote interrupt packet is enabled for RCPU MIIM access, the register
 *      CMIC_INTR_PKT_PACING_DELAY should be programmed properly to allow
 *      continuous generation of interrupt packets.
 */
int 
soc_rcpu_miim_write(int unit, uint16 phy_id,
               uint8 phy_reg_addr, uint16 phy_wr_data)
{
    int                 rv = SOC_E_NONE;
    int                 clause45;
    uint32              phy_param, val;

    assert(!sal_int_context());

    LOG_INFO(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_rcpu_miim_write: id=0x%02x addr=0x%02x data=0x%04x\n"),
              phy_id, phy_reg_addr, phy_wr_data));

    if(!soc_feature(unit, soc_feature_cmicm)) {
        return SOC_E_UNAVAIL;
    }

    clause45 = soc_feature(unit, soc_feature_phy_cl45);

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_CMICM_SUPPORT)
    /*
     * Trident Switch And Later
     * internal select bit 25
     * BUS_ID  bit 22-24
     * C45_SEL bit 21
     */
    if (SOC_IS_TD_TT(unit) || soc_feature(unit, soc_feature_cmicm)) {

        uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);

        /* set 5-bit PHY MDIO address */
        phy_param = (uint32)phy_wr_data |
                    (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

        /* select internal MDIO bus if set */
        if (phy_id & 0x80) { /* Internal/External select */
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 9));
        }

        /* set MDIO bus number */
        phy_param |= (bus_sel << (MIIM_PARAM_ID_OFFSET + 6));
    } else
#endif

#ifdef BCM_TRX_SUPPORT
    
    if (SOC_IS_TRX(unit)) {
        int bus_sel = (phy_id & 0x60) >> 5;
    
        phy_param = (uint32)phy_wr_data |
                    (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

        if (phy_id & 0x80) { /* Internal/External select */
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 7));
        }
        if (bus_sel == 1) {
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 6));
        } else if (bus_sel == 2) {
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 8));
        }
    } else
#endif /* BCM_TRX_SUPPORT */
    {
        phy_param = ((uint32)phy_id << MIIM_PARAM_ID_OFFSET |
                     (uint32)phy_wr_data);
    }

    /* Pause link scanning - and hold MIIM lock */
    if(!soc_feature(unit, soc_feature_cmicm)) {
       soc_linkscan_pause(unit);
    }
    MIIM_LOCK(unit);

    /* Write parameter register and tell CMIC to start */

    /* Clause 45 support changes Clause 22 access method */
    if (clause45) {
        soc_pci_write(unit, CMIC_RPE_MIIM_ADDRESS_OFFSET, phy_reg_addr);
    } else {
        phy_param |= (uint32)phy_reg_addr << MIIM_PARAM_REG_ADDR_OFFSET;
    }

    soc_pci_write(unit, CMIC_RPE_MIIM_PARAM_OFFSET, phy_param);
    val = soc_pci_read(unit, CMIC_RPE_MIIM_CTRL_OFFSET);
    soc_reg_field_set(unit, CMIC_RPE_MIIM_CTRLr, &val, MIIM_WR_STARTf, 1);
    soc_pci_write(unit, CMIC_RPE_MIIM_CTRL_OFFSET, val);

    /* Wait for completion using either the interrupt or polling method */
    if (SOC_CONTROL(unit)->miimIntrEnb) {
        soc_cmicm_rcpu_intr0_enable(unit, IRQ_RCPU_MIIM_OP_DONE);

        if (sal_sem_take(SOC_CONTROL(unit)->miimIntr,
                         SOC_CONTROL(unit)->miimTimeout) != 0) {
            rv = SOC_E_TIMEOUT;
        } else {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Interrupt received\n")));
        }

        soc_cmicm_rcpu_intr0_disable(unit, IRQ_RCPU_MIIM_OP_DONE);
    } else {
        soc_timeout_t to;

        soc_timeout_init(&to, SOC_CONTROL(unit)->miimTimeout, 250);

        while ((soc_pci_read(unit, CMIC_RPE_MIIM_STAT_OFFSET) &
                                   CMIC_MIIM_OPN_DONE) == 0) {
            if (soc_timeout_check(&to)) {
                rv = SOC_E_TIMEOUT;
                break;
            }
        }

        if (rv == SOC_E_NONE) {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Done in %d polls\n"), to.polls));
        }

        val = soc_pci_read(unit, CMIC_RPE_MIIM_CTRL_OFFSET);
        soc_reg_field_set(unit, CMIC_RPE_MIIM_CTRLr, &val, MIIM_WR_STARTf, 0);
        soc_pci_write(unit, CMIC_RPE_MIIM_CTRL_OFFSET, val);
    }
    if (rv == SOC_E_TIMEOUT) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "MiimTimeOut:soc_rcpu_miim_write, "
                              "timeout (id=0x%02x addr=0x%02x data=0x%04x)\n"),
                   phy_id, phy_reg_addr, phy_wr_data));
        SOC_CONTROL(unit)->stat.err_mii_tmo++;
    }

    /* Release linkscan pause and MIIM lock */

    MIIM_UNLOCK(unit);

    if(!soc_feature(unit, soc_feature_cmicm)) {
       soc_linkscan_continue(unit);
    }

    return rv;

}
/*
 * Function:
 *      soc_rcpu_miim_read
 * Purpose:
 *      Read a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 *      If remote interrupt packet is enabled for RCPU MIIM access, the register
 *      CMIC_INTR_PKT_PACING_DELAY should be programmed properly to allow
 *      continuous generation of interrupt packets. 
 */

int
soc_rcpu_miim_read(int unit, uint16 phy_id,
              uint8 phy_reg_addr, uint16 *phy_rd_data)
{
    int                 rv = SOC_E_NONE;
    int                 clause45;
    uint32              val, phy_param;

    assert(!sal_int_context());
    assert(phy_rd_data);

    LOG_INFO(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_rcpu_miim_read: id=0x%02x addr=0x%02x  "),
              phy_id, phy_reg_addr));

    if(!soc_feature(unit, soc_feature_cmicm)) {
        return SOC_E_UNAVAIL;
    }


    clause45 = soc_feature(unit, soc_feature_phy_cl45);

    /* 
     * Trident Switch And Katana
     * internal select bit 25
     * BUS_ID  bit 22-24
     * C45_SEL bit 21
     */
    if (SOC_IS_TD_TT(unit) || SOC_IS_KATANA(unit) || SOC_IS_TRIUMPH3(unit)) {
        
        uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);
        
        /* set 5-bit PHY MDIO address */ 
        phy_param = (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

        /* select internal MDIO bus if set */
        if (phy_id & 0x80) { /* Internal/External select */
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 9));
        }
  
        /* set MDIO bus number */
        phy_param |= (bus_sel << (MIIM_PARAM_ID_OFFSET + 6));
    } else 
#ifdef BCM_TRX_SUPPORT
    
    if (SOC_IS_TRX(unit)) {
        int bus_sel = (phy_id & 0x60) >> 5;

        phy_param = (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

        if (phy_id & 0x80) { /* Internal/External select */
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 7));
        }
        if (bus_sel == 1) {
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 6));
        } else if (bus_sel == 2) {
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 8));
        }
    } else
#endif /* BCM_TRX_SUPPORT */
    {
        phy_param = (uint32)phy_id << MIIM_PARAM_ID_OFFSET;
    }

    /* Pause link scanning - and hold MIIM lock */
    if(!soc_feature(unit, soc_feature_cmicm)) {
       soc_linkscan_pause(unit);
    }
    MIIM_LOCK(unit);

    /* Write parameter register and tell CMIC to start */

    /* Clause 45 support changes Clause 22 access method */
    if (clause45) {
        soc_pci_write(unit, CMIC_RPE_MIIM_ADDRESS_OFFSET, phy_reg_addr);
    } else {
        phy_param |= (uint32)phy_reg_addr << MIIM_PARAM_REG_ADDR_OFFSET;
    }

    soc_pci_write(unit, CMIC_RPE_MIIM_PARAM_OFFSET, phy_param);
    val = soc_pci_read(unit, CMIC_RPE_MIIM_CTRL_OFFSET);
    soc_reg_field_set(unit, CMIC_RPE_MIIM_CTRLr, &val, MIIM_RD_STARTf, 1);
    soc_pci_write(unit, CMIC_RPE_MIIM_CTRL_OFFSET, val);

    /* Wait for completion using either the interrupt or polling method */
    if (SOC_CONTROL(unit)->miimIntrEnb) {
        soc_cmicm_rcpu_intr0_enable(unit, IRQ_RCPU_MIIM_OP_DONE);

        if (sal_sem_take(SOC_CONTROL(unit)->miimIntr,
                         SOC_CONTROL(unit)->miimTimeout) != 0) {
            rv = SOC_E_TIMEOUT;
        } else {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Interrupt received\n")));
        }

        soc_cmicm_rcpu_intr0_disable(unit, IRQ_RCPU_MIIM_OP_DONE);
    } else {
        soc_timeout_t to;

        soc_timeout_init(&to, SOC_CONTROL(unit)->miimTimeout, 250);

        while ((soc_pci_read(unit, CMIC_RPE_MIIM_STAT_OFFSET) &
                                     CMIC_MIIM_OPN_DONE) == 0) {
            if (soc_timeout_check(&to)) {
                rv = SOC_E_TIMEOUT;
                break;
            }
        }

        if (rv == SOC_E_NONE) {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Done in %d polls\n"), to.polls));
        }

        val = soc_pci_read(unit, CMIC_RPE_MIIM_CTRL_OFFSET);
        soc_reg_field_set(unit, CMIC_RPE_MIIM_CTRLr, &val, MIIM_RD_STARTf, 0);
        soc_pci_write(unit, CMIC_RPE_MIIM_CTRL_OFFSET, val);
    }

    if (rv == SOC_E_TIMEOUT) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "MiimTimeOut:soc_rcpu_miim_read, "
                              "timeout (id=0x%02x addr=0x%02x)\n"),
                   phy_id, phy_reg_addr));
        SOC_CONTROL(unit)->stat.err_mii_tmo++;
    } else {
        *phy_rd_data = (uint16)soc_pci_read(unit, CMIC_RPE_MIIM_READ_DATA_OFFSET);

        LOG_INFO(BSL_LS_SOC_MIIM,
                 (BSL_META_U(unit,
                             "data=0x%04x\n"), *phy_rd_data));
    }

    /* Release linkscan pause and MIIM lock */

    MIIM_UNLOCK(unit);

    if(!soc_feature(unit, soc_feature_cmicm)) {
       soc_linkscan_continue(unit);
    }

    return rv;

}
/*
 * Function:
 *      soc_rcpu_miimc45_write
 * Purpose:
 *      Write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 *      If remote interrupt packet is enabled for RCPU MIIM access, the register
 *      CMIC_INTR_PKT_PACING_DELAY should be programmed properly to allow
 *      continuous generation of interrupt packets.
 */

int 
soc_rcpu_miimc45_write(int unit, uint16 phy_id, uint8 phy_devad,
                  uint16 phy_reg_addr, uint16 phy_wr_data)
{
    int                 rv = SOC_E_NONE;
    uint32              val;
    uint32              phy_param;
    uint32              phy_miim_addr;

    assert(!sal_int_context());

    if (SOC_WARM_BOOT(unit) || SOC_IS_RELOADING(unit)) {
        return SOC_E_NONE;
    }

    LOG_INFO(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_miimc45_write: id=0x%02x phy_devad=0x%02x "
                         "addr=0x%02x data=0x%04x\n"),
              phy_id, phy_devad, phy_reg_addr, phy_wr_data));

    if (!soc_feature(unit, soc_feature_phy_cl45) ||
        !soc_feature(unit, soc_feature_cmicm)) {
        return SOC_E_UNAVAIL;
    }

    /* Pause link scanning - and hold MIIM lock */
    if(!soc_feature(unit, soc_feature_cmicm)) {
       soc_linkscan_pause(unit);
    }
    MIIM_LOCK(unit);

    /* Write parameter registers and tell CMIC to start */
    phy_param = 0;
    soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param,
                           MIIM_CYCLEf, MIIM_CYCLE_AUTO);
    if (phy_id & 0x80) {
        phy_id &= (~0x80);
            soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr,
                              &phy_param, INTERNAL_SELf, 1);
    }

    if (soc_feature(unit, soc_feature_mdio_enhanced)) {
        soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param, C45_SELf, 1);
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_CMICM_SUPPORT)
        if (SOC_IS_TD_TT(unit) || soc_feature(unit, soc_feature_cmicm)) {
            uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);
            soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param,
                              BUS_IDf, bus_sel);
            phy_id &= ~(PHY_ID_BUS_UPPER_MASK | PHY_ID_BUS_LOWER_MASK);
        } else
#endif
#if defined(BCM_TRX_SUPPORT)
        if (SOC_IS_TRX(unit)) {
            soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param, 
                              BUS0_1_SELf, ((phy_id & 0x20) ? 1 : 0));
            phy_id &= (~0x20);

            soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param, 
                              BUS2_SELf, ((phy_id & 0x40) ? 1 : 0));
            phy_id &= (~0x40);
        } else 
#endif /* BCM_TRX_SUPPORT */
        {
            soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param, HG_SELf,
                              ((phy_id & 0x40) ? 1 : 0));
            phy_id &= (~0x40);
        }
    }

    soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param, PHY_IDf, phy_id);
    soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param,
                      PHY_DATAf, phy_wr_data);
    phy_miim_addr = 0;
    soc_reg_field_set(unit, CMIC_RPE_MIIM_ADDRESSr, &phy_miim_addr,
                      CLAUSE_45_DTYPEf, phy_devad);
    soc_reg_field_set(unit, CMIC_RPE_MIIM_ADDRESSr, &phy_miim_addr,
                      CLAUSE_45_REGADRf, phy_reg_addr);

    soc_pci_write(unit, CMIC_RPE_MIIM_ADDRESS_OFFSET, phy_miim_addr);
    soc_pci_write(unit, CMIC_RPE_MIIM_PARAM_OFFSET, phy_param);
    
    val = soc_pci_read(unit, CMIC_RPE_MIIM_CTRL_OFFSET);
    soc_reg_field_set(unit, CMIC_RPE_MIIM_CTRLr, &val, MIIM_WR_STARTf, 1);
    soc_pci_write(unit, CMIC_RPE_MIIM_CTRL_OFFSET, val);

    /* Wait for completion using either the interrupt or polling method */
    if (SOC_CONTROL(unit)->miimIntrEnb) {
        soc_cmicm_rcpu_intr0_enable(unit, IRQ_RCPU_MIIM_OP_DONE);

        if (sal_sem_take(SOC_CONTROL(unit)->miimIntr,
                         SOC_CONTROL(unit)->miimTimeout) != 0) {
            rv = SOC_E_TIMEOUT;
        } else {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Interrupt received\n")));
        }

        soc_cmicm_rcpu_intr0_disable(unit, IRQ_RCPU_MIIM_OP_DONE);
    } else {
        soc_timeout_t to;

        soc_timeout_init(&to, SOC_CONTROL(unit)->miimTimeout, 250);

        while ((soc_pci_read(unit, CMIC_RPE_MIIM_STAT_OFFSET) &
                                   CMIC_MIIM_OPN_DONE) == 0) {
            if (soc_timeout_check(&to)) {
                rv = SOC_E_TIMEOUT;
                break;
            }
        }

        if (rv == SOC_E_NONE) {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Done in %d polls\n"), to.polls));
        }

        val = soc_pci_read(unit, CMIC_RPE_MIIM_CTRL_OFFSET);
        soc_reg_field_set(unit, CMIC_RPE_MIIM_CTRLr, &val, MIIM_WR_STARTf, 0);
        soc_pci_write(unit, CMIC_RPE_MIIM_CTRL_OFFSET, val);
    }

    if (rv == SOC_E_TIMEOUT) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "MiimTimeOut:soc_miimc45_write, "
                              "timeout (id=0x%02x addr=0x%02x data=0x%04x)\n"),
                   phy_id, phy_reg_addr, phy_wr_data));
        SOC_CONTROL(unit)->stat.err_mii_tmo++;
    }

    /* Release linkscan pause and MIIM lock */
    MIIM_UNLOCK(unit);

    if(!soc_feature(unit, soc_feature_cmicm)) {
       soc_linkscan_continue(unit);
    }

    return rv;

}
/*
 * Function:
 *      soc_rcpu_miimc45_read
 * Purpose:
 *      Read a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 *      If remote interrupt packet is enabled for RCPU MIIM access, the register
 *      CMIC_INTR_PKT_PACING_DELAY should be programmed properly to allow
 *      continuous generation of interrupt packets.
 */

int
soc_rcpu_miimc45_read(int unit, uint16 phy_id, uint8 phy_devad,
                 uint16 phy_reg_addr, uint16 *phy_rd_data)
{
    int                 rv = SOC_E_NONE;    
    uint32              val=0, phy_param, phy_miim_addr;

    assert(!sal_int_context());
    assert(phy_rd_data);

    LOG_INFO(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_rcpu_miimc45_read: id=0x%02x "
                         "phy_devad=0x%02x addr=0x%02x\n"),
              phy_id, phy_devad, phy_reg_addr));

    if (!soc_feature(unit, soc_feature_cmicm)|| 
        !soc_feature(unit, soc_feature_phy_cl45)) {
        return SOC_E_UNAVAIL;
    }

    /* Pause link scanning - and hold MIIM lock */
    if(!soc_feature(unit, soc_feature_cmicm)) {
       soc_linkscan_pause(unit);
    }
    MIIM_LOCK(unit);

    /* Write parameter registers and tell CMIC to start */
    phy_param = 0;

    soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param,
                            MIIM_CYCLEf, MIIM_CYCLE_AUTO);

    if (phy_id & 0x80) {
        phy_id &= (~0x80);
        soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr,
                          &phy_param, INTERNAL_SELf, 1);

        if (soc_feature(unit, soc_feature_mdio_enhanced)) {
            soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr,
                              &phy_param, C45_SELf, 1);
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_CMICM_SUPPORT)
            if (SOC_IS_TD_TT(unit) || soc_feature(unit, soc_feature_cmicm)) {
                uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);
                soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param,
                                  BUS_IDf, bus_sel);
                phy_id &= ~(PHY_ID_BUS_UPPER_MASK | PHY_ID_BUS_LOWER_MASK);
            } else
#endif
#if defined(BCM_TRX_SUPPORT)
            if (SOC_IS_TRX(unit)) {
                soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param,
                                  BUS0_1_SELf, ((phy_id & 0x20) ? 1 : 0));
                phy_id &= (~0x20);

                soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param,
                                  BUS2_SELf, ((phy_id & 0x40) ? 1 : 0));
                phy_id &= (~0x40);
            } else
#endif
            {
                soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param, HG_SELf,
                                  ((phy_id & 0x40) ? 1 : 0));
                phy_id &= (~0x40);
            }
        }

        soc_reg_field_set(unit, CMIC_RPE_MIIM_PARAMr, &phy_param, PHY_IDf, phy_id);
        phy_miim_addr = 0;
        soc_reg_field_set(unit, CMIC_RPE_MIIM_ADDRESSr, &phy_miim_addr,
                          CLAUSE_45_DTYPEf, phy_devad);
        soc_reg_field_set(unit, CMIC_RPE_MIIM_ADDRESSr, &phy_miim_addr,
                          CLAUSE_45_REGADRf, phy_reg_addr);

        soc_pci_write(unit, CMIC_RPE_MIIM_ADDRESS_OFFSET, phy_miim_addr);
        soc_pci_write(unit, CMIC_RPE_MIIM_PARAM_OFFSET, phy_param);
    
        val = soc_pci_read(unit, CMIC_RPE_MIIM_CTRL_OFFSET);
        soc_reg_field_set(unit, CMIC_RPE_MIIM_CTRLr, &val, MIIM_RD_STARTf, 1);
        soc_pci_write(unit, CMIC_RPE_MIIM_CTRL_OFFSET, val);

        /* Wait for completion using either the interrupt or polling method */
        if (SOC_CONTROL(unit)->miimIntrEnb) {
            soc_cmicm_rcpu_intr0_enable(unit, IRQ_RCPU_MIIM_OP_DONE);

            if (sal_sem_take(SOC_CONTROL(unit)->miimIntr,
                             SOC_CONTROL(unit)->miimTimeout) != 0) {
                rv = SOC_E_TIMEOUT;
            } else {
                LOG_VERBOSE(BSL_LS_SOC_MIIM,
                            (BSL_META_U(unit,
                                        "  Interrupt received\n")));
            }

            soc_cmicm_rcpu_intr0_disable(unit, IRQ_RCPU_MIIM_OP_DONE);
        } else {
            soc_timeout_t to;

            soc_timeout_init(&to, SOC_CONTROL(unit)->miimTimeout, 250);

            while ((soc_pci_read(unit, CMIC_RPE_MIIM_STAT_OFFSET) &
                                         CMIC_MIIM_OPN_DONE) == 0) {
                if (soc_timeout_check(&to)) {
                    rv = SOC_E_TIMEOUT;
                    break;
                }
            }

            if (rv == SOC_E_NONE) {
                LOG_VERBOSE(BSL_LS_SOC_MIIM,
                            (BSL_META_U(unit,
                                        "  Done in %d polls\n"), to.polls));
            }

            val = soc_pci_read(unit, CMIC_RPE_MIIM_CTRL_OFFSET);
            soc_reg_field_set(unit, CMIC_RPE_MIIM_CTRLr, &val, MIIM_RD_STARTf, 0);
            soc_pci_write(unit, CMIC_RPE_MIIM_CTRL_OFFSET, val);
        }

        if (rv == SOC_E_TIMEOUT) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "MiimTimeOut:soc_rcpu_miimc45_read, "
                                  "timeout (id=0x%02x addr=0x%02x)\n"),
                       phy_id, phy_reg_addr));
            SOC_CONTROL(unit)->stat.err_mii_tmo++;
        } else {
            *phy_rd_data = (uint16)soc_pci_read(unit, CMIC_RPE_MIIM_READ_DATA_OFFSET);

            LOG_INFO(BSL_LS_SOC_MIIM,
                     (BSL_META_U(unit,
                                 "soc_rcpu_miimc45_read: read data=0x%04x\n"), *phy_rd_data));
        }

        /* Release linkscan pause and MIIM lock */
        MIIM_UNLOCK(unit);

        if(!soc_feature(unit, soc_feature_cmicm)) {
           soc_linkscan_continue(unit);
        }
    }

    return rv;

}
#endif /* defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT) */

/*
 * Function:
 *      soc_esw_miim_write
 * Purpose:
 *      New interface to write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 * Added to have the same interface for PHY register access among 
 * ESW, ROBO, SBX
 */
int 
soc_esw_miim_write(int unit, uint32 phy_id,
                   uint32 phy_reg_addr, uint16 phy_wr_data)
{
    return soc_miim_write(unit, (uint16)phy_id,
                          (uint16)phy_reg_addr, phy_wr_data);
}

/*
 * Function:
 *      soc_esw_miim_read
 * Purpose:
 *      New interface to read a value from a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - Data read.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 * Added to have the same interface for PHY register access among 
 * ESW, ROBO, SBX.
 */
int
soc_esw_miim_read(int unit, uint32 phy_id,
                  uint32 phy_reg_addr, uint16 *phy_rd_data)
{
    return soc_miim_read(unit, (uint16)phy_id, 
                         (uint16)phy_reg_addr, phy_rd_data);
}

/*
 * Function:
 *      soc_miim_modify
 * Purpose:
 *      Modify a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 *      phy_rd_mask - 16bit mask to indicate the bits to modify.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */

int
soc_miim_modify(int unit, uint16 phy_id, uint16 phy_reg_addr, 
                uint16 phy_rd_data, uint16 phy_rd_mask)
{
    uint16  tmp, otmp;

    phy_rd_data = phy_rd_data & phy_rd_mask;

    SOC_IF_ERROR_RETURN
        (soc_miim_read(unit, phy_id, phy_reg_addr, &tmp));
    otmp = tmp;
    tmp &= ~(phy_rd_mask);
    tmp |= phy_rd_data;

    if (otmp != tmp) {
        SOC_IF_ERROR_RETURN
            (soc_miim_write(unit, phy_id, phy_reg_addr, tmp));
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_esw_miimc45_write
 * Purpose:
 *      Write a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write (Encoded with PMA/PMD, PCS, PHY XS)
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_write(int unit, uint32 phy_id,
                     uint32 phy_reg_addr, uint16 phy_wr_data)
{
    uint8  dev_addr;
    uint16 reg_addr;

    dev_addr = SOC_PHY_CLAUSE45_DEVAD(phy_reg_addr);
    reg_addr = SOC_PHY_CLAUSE45_REGAD(phy_reg_addr);
 
    return soc_miimc45_write(unit, (uint16)phy_id, dev_addr,
                            reg_addr, phy_wr_data);
}

/*
 * Function:
 *      soc_esw_miimc45_write
 * Purpose:
 *      Write a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write (Encoded with PMA/PMD, PCS, PHY XS)
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_read(int unit, uint32 phy_id,
                    uint32 phy_reg_addr, uint16 *phy_rd_data)
{
    uint8  dev_addr;
    uint16 reg_addr;

    dev_addr   = SOC_PHY_CLAUSE45_DEVAD(phy_reg_addr);
    reg_addr   = SOC_PHY_CLAUSE45_REGAD(phy_reg_addr);
 
    return soc_miimc45_read(unit, (uint16)phy_id, dev_addr,
                          reg_addr, phy_rd_data);
}
/*
 * Function:
 *      soc_miimc45_modify
 * Purpose:
 *      Modify a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 *      phy_rd_mask - 16bit mask to indicate the bits to modify.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */

int
soc_miimc45_modify(int unit, uint16 phy_id, uint8 phy_devad,
                 uint16 phy_reg_addr, uint16 phy_rd_data, uint16 phy_rd_mask)
{
    uint16  tmp = 0, otmp;

    phy_rd_data = phy_rd_data & phy_rd_mask;

    SOC_IF_ERROR_RETURN
        (soc_miimc45_read(unit, phy_id, phy_devad, phy_reg_addr, &tmp));
    otmp = tmp;
    tmp &= ~(phy_rd_mask);
    tmp |= phy_rd_data;

    if (otmp != tmp) {
        SOC_IF_ERROR_RETURN
            (soc_miimc45_write(unit, phy_id, phy_devad, phy_reg_addr, tmp));
    }
    return SOC_E_NONE;
}

/*
 * Customer specific MDIO wrapper support
 *
 * Some customers of the SDK wish to wrap the underlying MDIO access
 * routines (both clause 22 and clasue 45) with customer specific
 * functions that may provide an alternate access method, some predicate
 * controls on whether actual operations occur, or some translation
 * mechanisms.
 *
 * The method discussed here is an interim mechanism that is sufficient
 * for release to selected customers.  A more extensible long term
 * mechanism will be defined in a future SDK release.  This method will
 * work for Enterprise Switch (XGS) operations but will not work for
 * SMB/Robo or SBX switch operations.
 *
 * 1. Replace the file $SDK/soc/common/miim.c with this file.
 *    It moves the basic miim access functions to the end of the file
 *    and provides mechanism to rename them to _soc_miim*.
 * 2. Compile the entire SDK with the following defines on the command line:
 *    -Dsoc_miim_read=cust_miim_read
 *    -Dsoc_miim_write=cust_miim_write
 *    -Dsoc_miimc45_read=cust_miimc45_read
 *    -Dsoc_miimc45_write=cust_miic45_write
 * 3. Provide implementation of the cust_miim* functions.
 *    They are defined identically to the soc_miim* functions below:
 *    int cust_miim_write(int unit, uint8 phy_id,
 *                uint8 phy_reg_addr, uint16 phy_wr_data)
 *    int cust_miim_read(int unit, uint8 phy_id,
 *               uint8 phy_reg_addr, uint16 *phy_rd_data)
 *    int cust_miimc45_write(int unit, uint8 phy_id, uint8 phy_devad,
 *                   uint16 phy_reg_addr, uint16 phy_wr_data)
 *    int cust_miimc45_read(int unit, uint8 phy_id, uint8 phy_devad,
 *                  uint16 phy_reg_addr, uint16 *phy_rd_data)
 * 4. If the cust_miim* functions need to access the original defintions
 *    of the soc_miim* functions, they need to refer to them as _soc_miim*
 *    and provide extern definitions of those functions:
 *    extern int _soc_miim_write(int unit, uint8 phy_id,
 *                       uint8 phy_reg_addr, uint16 phy_wr_data);
 *    extern int _soc_miim_read(int unit, uint8 phy_id,
 *                      uint8 phy_reg_addr, uint16 *phy_rd_data);
 *    extern int _soc_miimc45_write(int unit, uint8 phy_id, uint8 phy_devad,
 *                          uint16 phy_reg_addr, uint16 phy_wr_data);
 *    extern int _soc_miimc45_read(int unit, uint8 phy_id, uint8 phy_devad,
 *                         uint16 phy_reg_addr, uint16 *phy_rd_data);
 */
#ifdef soc_miim_read
    #undef soc_miim_read
    #define soc_miim_read _soc_miim_read
#endif
#ifdef soc_miim_write
    #undef soc_miim_write
    #define soc_miim_write _soc_miim_write
#endif
#ifdef soc_miimc45_read
    #undef soc_miimc45_read
    #define soc_miimc45_read _soc_miimc45_read
#endif
#ifdef soc_miimc45_write
    #undef soc_miimc45_write
    #define soc_miimc45_write _soc_miimc45_write
#endif

/*
 * Function:
 *      soc_miim_write
 * Purpose:
 *      Write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 */

int 
soc_miim_write(int unit, uint16 phy_id,
               uint8 phy_reg_addr, uint16 phy_wr_data)
{
    int                 rv = SOC_E_NONE;
#if defined(BCM_ESW_SUPPORT) || defined (BCM_DFE_SUPPORT)
    int                 clause45;
    uint32              phy_param;
#endif
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
    uint32 val=0;
    cmc = cmc;  /* get compiler to stop complaining */
    val = val;  /* get compiler to stop complaining */
#endif

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(unit)) {
        LOG_INFO(BSL_LS_SOC_MIIM,
                 (BSL_META_U(unit,
                             "soc_miim_write: id=0x%02x, drive to ROBO miim_write.\n"),
                  phy_id));
        return (soc_robo_miim_write
            (unit, phy_id, phy_reg_addr, phy_wr_data));
    }
#endif
#ifdef BCM_SBX_SUPPORT
    if (SOC_IS_SBX(unit)) {
        return (soc_sbx_miim_write(unit, phy_id, phy_reg_addr, phy_wr_data));
    }
#endif
#if defined(BCM_ESW_SUPPORT) || defined (BCM_DFE_SUPPORT)
    assert(!sal_int_context());

    LOG_INFO(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_miim_write: id=0x%02x addr=0x%02x data=0x%04x\n"),
              phy_id, phy_reg_addr, phy_wr_data));

    clause45 = soc_feature(unit, soc_feature_phy_cl45);

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_CMICM_SUPPORT)
    /*
     * Trident Switch And Later
     * internal select bit 25
     * BUS_ID  bit 22-24
     * C45_SEL bit 21
     */
    if (SOC_IS_TD_TT(unit) || soc_feature(unit, soc_feature_cmicm)) {

        uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);

        /* set 5-bit PHY MDIO address */
        phy_param = (uint32)phy_wr_data |
                    (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

        /* select internal MDIO bus if set */
        if (phy_id & 0x80) { /* Internal/External select */
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 9));
        }

        /* set MDIO bus number */
        phy_param |= (bus_sel << (MIIM_PARAM_ID_OFFSET + 6));
    } else
#endif
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {

        uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);

        /* set 5-bit PHY MDIO address */
        phy_param = (uint32)phy_wr_data |
                    (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

        /* select internal MDIO bus if set */
        if (phy_id & 0x80) { /* Internal/External select */
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 9));
        }

        /* set MDIO bus number */
        phy_param |= (bus_sel << (MIIM_PARAM_ID_OFFSET + 6));
    } else
#endif /* BCM_SHADOW_SUPPORT */

#ifdef BCM_TRX_SUPPORT
    
    if (SOC_IS_TRX(unit)) {
        int bus_sel = (phy_id & 0x60) >> 5;
    
        phy_param = (uint32)phy_wr_data |
                    (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

        if (phy_id & 0x80) { /* Internal/External select */
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 7));
        }
        if (bus_sel == 1) {
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 6));
        } else if (bus_sel == 2) {
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 8));
        }
    } else
#endif /* BCM_TRX_SUPPORT */
    {
        phy_param = ((uint32)phy_id << MIIM_PARAM_ID_OFFSET |
                     (uint32)phy_wr_data);
    }

    if(!soc_feature(unit, soc_feature_cmicm)) {
       /* Pause link scanning - and hold MIIM lock */
       soc_linkscan_pause(unit);
    }
    MIIM_LOCK(unit);

    /* Write parameter register and tell CMIC to start */

    /* Clause 45 support changes Clause 22 access method */
    if (clause45) {
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_pci_write(unit, CMIC_CMCx_MIIM_ADDRESS_OFFSET(cmc), phy_reg_addr);
        } else
#endif
        {
            WRITE_CMIC_MIIM_ADDRESSr(unit, phy_reg_addr);
        }
    } else {
        phy_param |= (uint32)phy_reg_addr << MIIM_PARAM_REG_ADDR_OFFSET;
    }

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm)) {
        soc_pci_write(unit, CMIC_CMCx_MIIM_PARAM_OFFSET(cmc), phy_param);
        val = soc_pci_read(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc));
        soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &val,
                                                 MIIM_WR_STARTf, 1);
        soc_pci_write(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc), val);
    } else
#endif
    {
        soc_pci_write(unit, CMIC_MIIM_PARAM, phy_param);
        soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_WR_START_SET);
    }

    /* Wait for completion using either the interrupt or polling method */

    if (SOC_CONTROL(unit)->miimIntrEnb) {
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_cmicm_intr0_enable(unit, IRQ_CMCx_MIIM_OP_DONE);
        } else
#endif
        {
            soc_intr_enable(unit, IRQ_MIIM_OP_DONE);
        }
        if (sal_sem_take(SOC_CONTROL(unit)->miimIntr,
                         SOC_CONTROL(unit)->miimTimeout) != 0) {
            rv = SOC_E_TIMEOUT;
        } else {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Interrupt received\n")));
        }

#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_cmicm_intr0_disable(unit, IRQ_CMCx_MIIM_OP_DONE);
        } else
#endif
        {
            soc_intr_disable(unit, IRQ_MIIM_OP_DONE);
        }
    } else {
        soc_timeout_t to;

        soc_timeout_init(&to, SOC_CONTROL(unit)->miimTimeout, 250);

#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            while ((soc_pci_read(unit, CMIC_CMCx_MIIM_STAT_OFFSET(cmc)) &
                                         CMIC_MIIM_OPN_DONE) == 0) {
                if (soc_timeout_check(&to)) {
                    rv = SOC_E_TIMEOUT;
                    break;
                }
            }
        } else
#endif
        {
            while ((soc_pci_read(unit, CMIC_SCHAN_CTRL) &
                    SC_MIIM_OP_DONE_TST) == 0) {
                if (soc_timeout_check(&to)) {
                    rv = SOC_E_TIMEOUT;
                    break;
                }
            }
        }
        if (rv == SOC_E_NONE) {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Done in %d polls\n"), to.polls));
        }
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            val = soc_pci_read(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc));
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &val,
                                                     MIIM_WR_STARTf, 0);
            soc_pci_write(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc), val);
        } else
#endif
        {
            soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_OP_DONE_CLR);
        }
    }
    if (rv == SOC_E_TIMEOUT) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "MiimTimeOut:soc_miim_write, "
                              "timeout (id=0x%02x addr=0x%02x data=0x%04x)\n"),
                   phy_id, phy_reg_addr, phy_wr_data));
        SOC_CONTROL(unit)->stat.err_mii_tmo++;
    }

    /* Release linkscan pause and MIIM lock */

    MIIM_UNLOCK(unit);

    if(!soc_feature(unit, soc_feature_cmicm)) {
       soc_linkscan_continue(unit);
    }
#endif
    return rv;

}

/*
 * Function:
 *      soc_miim_read
 * Purpose:
 *      Read a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 */

int
soc_miim_read(int unit, uint16 phy_id,
              uint8 phy_reg_addr, uint16 *phy_rd_data)
{
    int                 rv = SOC_E_NONE;
#if defined(BCM_ESW_SUPPORT) || defined(BCM_DFE_SUPPORT)
    int                 clause45;
    uint32              phy_param;
#endif
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
    uint32 val=0;
    cmc = cmc;  /* get compiler to stop complaining */
    val = val;  /* get compiler to stop complaining */
#endif
    assert(!sal_int_context());
    assert(phy_rd_data);

#ifdef BCM_ROBO_SUPPORT
    LOG_INFO(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_miim_read: id=0x%02x, drive to ROBO miim_read.\n"),
              phy_id));
    if (SOC_IS_ROBO(unit)) {
        return (soc_robo_miim_read
            (unit, phy_id, phy_reg_addr, phy_rd_data));
    }
#endif
#ifdef BCM_SBX_SUPPORT
    if (SOC_IS_SBX(unit)) {
        return (soc_sbx_miim_read(unit, phy_id, phy_reg_addr, phy_rd_data));
    }
#endif
#if defined(BCM_ESW_SUPPORT) || defined(BCM_DFE_SUPPORT)
    LOG_INFO(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_miim_read: id=0x%02x addr=0x%02x  "),
              phy_id, phy_reg_addr));

    clause45 = soc_feature(unit, soc_feature_phy_cl45);
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_CMICM_SUPPORT)
    /* 
     * Trident Switch And Later
     * internal select bit 25
     * BUS_ID  bit 22-24
     * C45_SEL bit 21
     */
    if (SOC_IS_TD_TT(unit) || soc_feature(unit, soc_feature_cmicm)) {
        
        uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);
        
        /* set 5-bit PHY MDIO address */ 
        phy_param = (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

        /* select internal MDIO bus if set */
        if (phy_id & 0x80) { /* Internal/External select */
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 9));
        }
  
        /* set MDIO bus number */
        phy_param |= (bus_sel << (MIIM_PARAM_ID_OFFSET + 6));
    } else
#endif
#ifdef BCM_SHADOW_SUPPORT
    /* 
     * Shadow device
     * internal select bit 25
     * BUS_ID  bit 22-24
     * C45_SEL bit 21
     */
    if (SOC_IS_SHADOW(unit)) {
        
        uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);
        
        /* set 5-bit PHY MDIO address */ 
        phy_param = (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

        /* select internal MDIO bus if set */
        if (phy_id & 0x80) { /* Internal/External select */
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 9));
        }
  
        /* set MDIO bus number */
        phy_param |= (bus_sel << (MIIM_PARAM_ID_OFFSET + 6));
    } else
#endif /* BCM_SHADOW_SUPPORT */


#ifdef BCM_TRX_SUPPORT
    
    if (SOC_IS_TRX(unit)) {
        int bus_sel = (phy_id & 0x60) >> 5;

        phy_param = (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

        if (phy_id & 0x80) { /* Internal/External select */
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 7));
        }
        if (bus_sel == 1) {
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 6));
        } else if (bus_sel == 2) {
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 8));
        }
    } else
#endif /* BCM_TRX_SUPPORT */
    {
        phy_param = (uint32)phy_id << MIIM_PARAM_ID_OFFSET;
    }

    /* Pause link scanning - and hold MIIM lock */
    if(!soc_feature(unit, soc_feature_cmicm)) {
       soc_linkscan_pause(unit);
    }
    MIIM_LOCK(unit);

    /* Write parameter register and tell CMIC to start */

    /* Clause 45 support changes Clause 22 access method */
    if (clause45) {
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_pci_write(unit, CMIC_CMCx_MIIM_ADDRESS_OFFSET(cmc), phy_reg_addr);
        } else
#endif
        {
            WRITE_CMIC_MIIM_ADDRESSr(unit, phy_reg_addr);
        }
    } else {
        phy_param |= (uint32)phy_reg_addr << MIIM_PARAM_REG_ADDR_OFFSET;
    }

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm)) {
        soc_pci_write(unit, CMIC_CMCx_MIIM_PARAM_OFFSET(cmc), phy_param);

        val = soc_pci_read(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc));
        soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &val,
                                                 MIIM_RD_STARTf, 1);
        soc_pci_write(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc), val);
    } else
#endif
    {
        soc_pci_write(unit, CMIC_MIIM_PARAM, phy_param);
        soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_RD_START_SET);
    }

    /* Wait for completion using either the interrupt or polling method */

    if (SOC_CONTROL(unit)->miimIntrEnb) {
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_cmicm_intr0_enable(unit, IRQ_CMCx_MIIM_OP_DONE);
        } else
#endif
        {
            soc_intr_enable(unit, IRQ_MIIM_OP_DONE);
        }

        if (sal_sem_take(SOC_CONTROL(unit)->miimIntr,
                         SOC_CONTROL(unit)->miimTimeout) != 0) {
            rv = SOC_E_TIMEOUT;
        } else {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Interrupt received\n")));
        }

#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_cmicm_intr0_disable(unit, IRQ_CMCx_MIIM_OP_DONE);
        } else
#endif
        {
            soc_intr_disable(unit, IRQ_MIIM_OP_DONE);
        }
    } else {
        soc_timeout_t to;

        soc_timeout_init(&to, SOC_CONTROL(unit)->miimTimeout, 250);

#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            while ((soc_pci_read(unit, CMIC_CMCx_MIIM_STAT_OFFSET(cmc)) &
                                         CMIC_MIIM_OPN_DONE) == 0) {
                if (soc_timeout_check(&to)) {
                    rv = SOC_E_TIMEOUT;
                    break;
                }
            }
        } else
#endif
        {
            while ((soc_pci_read(unit, CMIC_SCHAN_CTRL) &
                    SC_MIIM_OP_DONE_TST) == 0) {
                if (soc_timeout_check(&to)) {
                    rv = SOC_E_TIMEOUT;
                    break;
                }
            }
        }

        if (rv == SOC_E_NONE) {
            LOG_VERBOSE(BSL_LS_SOC_MIIM,
                        (BSL_META_U(unit,
                                    "  Done in %d polls\n"), to.polls));
        }
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            val = soc_pci_read(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc));
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &val,
                                                     MIIM_RD_STARTf, 0);
            soc_pci_write(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc), val);
        } else
#endif
        {
            soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_OP_DONE_CLR);
        }
    }

    if (rv == SOC_E_TIMEOUT) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "MiimTimeOut:soc_miim_read, "
                              "timeout (id=0x%02x addr=0x%02x)\n"),
                   phy_id, phy_reg_addr));
        SOC_CONTROL(unit)->stat.err_mii_tmo++;
    } else {
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            *phy_rd_data = (uint16)soc_pci_read(unit, CMIC_CMCx_MIIM_READ_DATA_OFFSET(cmc));
        } else
#endif
        {
            *phy_rd_data = (uint16)soc_pci_read(unit, CMIC_MIIM_READ_DATA);
        }

        LOG_INFO(BSL_LS_SOC_MIIM,
                 (BSL_META_U(unit,
                             "data=0x%04x\n"), *phy_rd_data));
    }

    /* Release linkscan pause and MIIM lock */

    MIIM_UNLOCK(unit);

    if(!soc_feature(unit, soc_feature_cmicm)) {
       soc_linkscan_continue(unit);
    }
#endif    
    return rv;

}

/*
 * Function:
 *      soc_miimc45_write
 * Purpose:
 *      Write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 */

int 
soc_miimc45_write(int unit, uint16 phy_id, uint8 phy_devad,
                  uint16 phy_reg_addr, uint16 phy_wr_data)
{
    int                 rv = SOC_E_NONE;
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
    uint32 val=0;
    cmc = cmc;  /* get compiler to stop complaining */
    val = val;  /* get compiler to stop complaining */
#endif
    assert(!sal_int_context());

    if ((SOC_WARM_BOOT(unit)) && (phy_devad != 0x1e)) {
        return SOC_E_NONE;
    }

    LOG_INFO(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_miimc45_write: id=0x%02x phy_devad=0x%02x "
                         "addr=0x%02x data=0x%04x\n"),
              phy_id, phy_devad, phy_reg_addr, phy_wr_data));

    if (!soc_feature(unit, soc_feature_phy_cl45)) {
        return SOC_E_UNAVAIL;
    }
#ifdef BCM_SBX_SUPPORT
    if (SOC_IS_SBX(unit)) {
        return (soc_sbx_miimc45_write(unit, phy_id,
                                      SOC_PHY_CLAUSE45_ADDR(phy_devad,
                                                            phy_reg_addr),
                                      phy_wr_data));
    }
#endif
#ifdef BCM_ESW_SUPPORT

    if ((SOC_IS_RELOADING(unit)) && (phy_devad != 0x1e)) {
        return SOC_E_NONE;
    }

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    {
        uint32          phy_param;
        uint32          phy_miim_addr;

    /* Pause link scanning - and hold MIIM lock */
        if(!soc_feature(unit, soc_feature_cmicm)) {
           soc_linkscan_pause(unit);
        }
        MIIM_LOCK(unit);

    /* Write parameter registers and tell CMIC to start */
        phy_param = 0;
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &phy_param,
                              MIIM_CYCLEf, MIIM_CYCLE_AUTO);
        } else 
#endif
        {
            soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param,
                              MIIM_CYCLEf, MIIM_CYCLE_AUTO);
        }
        if (phy_id & 0x80) {
            phy_id &= (~0x80);
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr,
                                  &phy_param, INTERNAL_SELf, 1);
            } else 
#endif
            {
                soc_reg_field_set(unit, CMIC_MIIM_PARAMr,
                                  &phy_param, INTERNAL_SELf, 1);
            }
        }
#if defined(BCM_XGS3_SWITCH_SUPPORT)
        if (soc_feature(unit, soc_feature_mdio_enhanced)) {
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr,
                                  &phy_param, C45_SELf, 1);
            } else
#endif
            {
                soc_reg_field_set(unit, CMIC_MIIM_PARAMr,
                                  &phy_param, C45_SELf, 1);
            }
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_CMICM_SUPPORT)
            if (SOC_IS_TD_TT(unit) || soc_feature(unit, soc_feature_cmicm)) {
                uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);
#ifdef BCM_CMICM_SUPPORT
                if(soc_feature(unit, soc_feature_cmicm)) {
                    soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &phy_param,
                                      BUS_IDf, bus_sel);
                } else
#endif
                {
                    soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param,
                                      BUS_IDf, bus_sel);
                }
                phy_id &= ~(PHY_ID_BUS_UPPER_MASK | PHY_ID_BUS_LOWER_MASK);
            } else
#endif
#if defined(BCM_SHADOW_SUPPORT)
            if (SOC_IS_SHADOW(unit)) {
                uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);
                soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param,
                                  BUS_IDf, bus_sel);
                phy_id &= ~(PHY_ID_BUS_UPPER_MASK | PHY_ID_BUS_LOWER_MASK);
            } else
#endif
#if defined(BCM_TRX_SUPPORT)
            if (SOC_IS_TRX(unit)) {
                soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param, 
                                  BUS0_1_SELf, ((phy_id & 0x20) ? 1 : 0));
                phy_id &= (~0x20);

                soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param, 
                                  BUS2_SELf, ((phy_id & 0x40) ? 1 : 0));
                phy_id &= (~0x40);
            } else 
#endif /* BCM_TRX_SUPPORT */
            {
                soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param, HG_SELf,
                                  ((phy_id & 0x40) ? 1 : 0));
                phy_id &= (~0x40);
            }
        }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &phy_param, PHY_IDf, phy_id);
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &phy_param,
                              PHY_DATAf, phy_wr_data);
            phy_miim_addr = 0;
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_ADDRESSr, &phy_miim_addr,
                              CLAUSE_45_DTYPEf, phy_devad);
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_ADDRESSr, &phy_miim_addr,
                              CLAUSE_45_REGADRf, phy_reg_addr);
        } else
#endif
        {
            soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param, PHY_IDf, phy_id);
            soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param,
                              PHY_DATAf, phy_wr_data);
            phy_miim_addr = 0;
            soc_reg_field_set(unit, CMIC_MIIM_ADDRESSr, &phy_miim_addr,
                              CLAUSE_45_DTYPEf, phy_devad);
            soc_reg_field_set(unit, CMIC_MIIM_ADDRESSr, &phy_miim_addr,
                              CLAUSE_45_REGADRf, phy_reg_addr);
        }
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_pci_write(unit, CMIC_CMCx_MIIM_ADDRESS_OFFSET(cmc), phy_miim_addr);
            soc_pci_write(unit, CMIC_CMCx_MIIM_PARAM_OFFSET(cmc), phy_param);
    
            val = soc_pci_read(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc));
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &val,
                                                     MIIM_WR_STARTf, 1);
            soc_pci_write(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc), val);
        } else
#endif
        {
            WRITE_CMIC_MIIM_ADDRESSr(unit, phy_miim_addr);
            soc_pci_write(unit, CMIC_MIIM_PARAM, phy_param);
            soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_WR_START_SET);
        }

        /* Wait for completion using either the interrupt or polling method */

        if (SOC_CONTROL(unit)->miimIntrEnb) {
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                soc_cmicm_intr0_enable(unit, IRQ_CMCx_MIIM_OP_DONE);
            } else
#endif
            {
                soc_intr_enable(unit, IRQ_MIIM_OP_DONE);
            }

            if (sal_sem_take(SOC_CONTROL(unit)->miimIntr,
                             SOC_CONTROL(unit)->miimTimeout) != 0) {
                rv = SOC_E_TIMEOUT;
            } else {
                LOG_VERBOSE(BSL_LS_SOC_MIIM,
                            (BSL_META_U(unit,
                                        "  Interrupt received\n")));
            }

#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                soc_cmicm_intr0_disable(unit, IRQ_CMCx_MIIM_OP_DONE);
            } else
#endif
            {
                soc_intr_disable(unit, IRQ_MIIM_OP_DONE);
            }
        } else {
            soc_timeout_t to;

            soc_timeout_init(&to, SOC_CONTROL(unit)->miimTimeout, 250);

#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                while ((soc_pci_read(unit, CMIC_CMCx_MIIM_STAT_OFFSET(cmc)) &
                                             CMIC_MIIM_OPN_DONE) == 0) {
                    if (soc_timeout_check(&to)) {
                        rv = SOC_E_TIMEOUT;
                        break;
                    }
                }
            } else
#endif
            {
                while ((soc_pci_read(unit, CMIC_SCHAN_CTRL) &
                        SC_MIIM_OP_DONE_TST) == 0) {
                    if (soc_timeout_check(&to)) {
                        rv = SOC_E_TIMEOUT;
                        break;
                    }
                }
            }

            if (rv == SOC_E_NONE) {
                LOG_VERBOSE(BSL_LS_SOC_MIIM,
                            (BSL_META_U(unit,
                                        "  Done in %d polls\n"), to.polls));
            }
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                val = soc_pci_read(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc));
                soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &val,
                                                         MIIM_WR_STARTf, 0);
                soc_pci_write(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc), val);
            } else
#endif
            {
                soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_OP_DONE_CLR);
            }
        }

        if (rv == SOC_E_TIMEOUT) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "MiimTimeOut:soc_miimc45_write, "
                                  "timeout (id=0x%02x addr=0x%02x data=0x%04x)\n"),
                       phy_id, phy_reg_addr, phy_wr_data));
            SOC_CONTROL(unit)->stat.err_mii_tmo++;
        }

        /* Release linkscan pause and MIIM lock */
        MIIM_UNLOCK(unit);

        if(!soc_feature(unit, soc_feature_cmicm)) {
           soc_linkscan_continue(unit);
        }
    }
#endif
#endif
    return rv;

}

/*
 * Function:
 *      soc_miimc45_read
 * Purpose:
 *      Read a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 */

int
soc_miimc45_read(int unit, uint16 phy_id, uint8 phy_devad,
                 uint16 phy_reg_addr, uint16 *phy_rd_data)
{
    int                 rv = SOC_E_NONE;    
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
    uint32 val=0;
    cmc = cmc;  /* get compiler to stop complaining */
    val = val;  /* get compiler to stop complaining */
#endif
    assert(!sal_int_context());
    assert(phy_rd_data);

    LOG_INFO(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_miimc45_read: id=0x%02x "
                         "phy_devad=0x%02x addr=0x%02x\n"),
              phy_id, phy_devad, phy_reg_addr));

    if (!soc_feature(unit, soc_feature_phy_cl45)) {
        return SOC_E_UNAVAIL;
    }
#ifdef BCM_SBX_SUPPORT
    if (SOC_IS_SBX(unit)) {
        return (soc_sbx_miimc45_read(unit, phy_id,
                                     SOC_PHY_CLAUSE45_ADDR(phy_devad,
                                                           phy_reg_addr),
                                     phy_rd_data));
    }
#endif
#ifdef BCM_ESW_SUPPORT

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    {
        uint32          phy_param;
        uint32          phy_miim_addr;

        /* Pause link scanning - and hold MIIM lock */
        if(!soc_feature(unit, soc_feature_cmicm)) {
           soc_linkscan_pause(unit);
        }
        MIIM_LOCK(unit);

    /* Write parameter registers and tell CMIC to start */
        phy_param = 0;
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &phy_param,
                              MIIM_CYCLEf, MIIM_CYCLE_AUTO);
        } else 
#endif
        {
            soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param,
                              MIIM_CYCLEf, MIIM_CYCLE_AUTO);
        }
        if (phy_id & 0x80) {
            phy_id &= (~0x80);
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr,
                                  &phy_param, INTERNAL_SELf, 1);
            } else 
#endif
            {
                soc_reg_field_set(unit, CMIC_MIIM_PARAMr,
                                  &phy_param, INTERNAL_SELf, 1);
            }
        }
#if defined(BCM_XGS3_SWITCH_SUPPORT)
        if (soc_feature(unit, soc_feature_mdio_enhanced)) {
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr,
                                  &phy_param, C45_SELf, 1);
            } else
#endif
            {
                soc_reg_field_set(unit, CMIC_MIIM_PARAMr,
                                  &phy_param, C45_SELf, 1);
            }
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_CMICM_SUPPORT)
            if (SOC_IS_TD_TT(unit) || soc_feature(unit, soc_feature_cmicm)) {
                uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);
#ifdef BCM_CMICM_SUPPORT
                if(soc_feature(unit, soc_feature_cmicm)) {
                    soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &phy_param,
                                      BUS_IDf, bus_sel);
                } else
#endif
                {
                    soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param,
                                      BUS_IDf, bus_sel);
                }
                phy_id &= ~(PHY_ID_BUS_UPPER_MASK | PHY_ID_BUS_LOWER_MASK);
            } else
#endif
#if defined(BCM_SHADOW_SUPPORT)
            if (SOC_IS_SHADOW(unit)) {
                uint32 bus_sel = PHY_ID_BUS_NUM(phy_id);
                soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param,
                                  BUS_IDf, bus_sel);
                phy_id &= ~(PHY_ID_BUS_UPPER_MASK | PHY_ID_BUS_LOWER_MASK);
            } else
#endif
#if defined(BCM_TRX_SUPPORT)
            if (SOC_IS_TRX(unit)) {
                soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param,
                                  BUS0_1_SELf, ((phy_id & 0x20) ? 1 : 0));
                phy_id &= (~0x20);

                soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param,
                                  BUS2_SELf, ((phy_id & 0x40) ? 1 : 0));
                phy_id &= (~0x40);
            } else
#endif
            {
                soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param, HG_SELf,
                                  ((phy_id & 0x40) ? 1 : 0));
                phy_id &= (~0x40);
            }
        }
#endif
#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_PARAMr, &phy_param, PHY_IDf, phy_id);
            phy_miim_addr = 0;
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_ADDRESSr, &phy_miim_addr,
                              CLAUSE_45_DTYPEf, phy_devad);
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_ADDRESSr, &phy_miim_addr,
                              CLAUSE_45_REGADRf, phy_reg_addr);
        } else
#endif
        {
            soc_reg_field_set(unit, CMIC_MIIM_PARAMr, &phy_param, PHY_IDf, phy_id);
            phy_miim_addr = 0;
            soc_reg_field_set(unit, CMIC_MIIM_ADDRESSr, &phy_miim_addr,
                              CLAUSE_45_DTYPEf, phy_devad);
            soc_reg_field_set(unit, CMIC_MIIM_ADDRESSr, &phy_miim_addr,
                              CLAUSE_45_REGADRf, phy_reg_addr);
        }

#ifdef BCM_CMICM_SUPPORT
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_pci_write(unit, CMIC_CMCx_MIIM_ADDRESS_OFFSET(cmc), phy_miim_addr);
            soc_pci_write(unit, CMIC_CMCx_MIIM_PARAM_OFFSET(cmc), phy_param);
    
            val = soc_pci_read(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc));
            soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &val,
                                                     MIIM_RD_STARTf, 1);
            soc_pci_write(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc), val);
        } else
#endif
        {
            WRITE_CMIC_MIIM_ADDRESSr(unit, phy_miim_addr);
            soc_pci_write(unit, CMIC_MIIM_PARAM, phy_param);
            soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_RD_START_SET);
        }

        /* Wait for completion using either the interrupt or polling method */

        if (SOC_CONTROL(unit)->miimIntrEnb) {
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                soc_cmicm_intr0_enable(unit, IRQ_CMCx_MIIM_OP_DONE);
            } else
#endif
            {
                soc_intr_enable(unit, IRQ_MIIM_OP_DONE);
            }

            if (sal_sem_take(SOC_CONTROL(unit)->miimIntr,
                             SOC_CONTROL(unit)->miimTimeout) != 0) {
                rv = SOC_E_TIMEOUT;
            } else {
                LOG_VERBOSE(BSL_LS_SOC_MIIM,
                            (BSL_META_U(unit,
                                        "  Interrupt received\n")));
            }

#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                soc_cmicm_intr0_disable(unit, IRQ_CMCx_MIIM_OP_DONE);
            } else
#endif
            {
                soc_intr_disable(unit, IRQ_MIIM_OP_DONE);
            }
        } else {
            soc_timeout_t to;

            soc_timeout_init(&to, SOC_CONTROL(unit)->miimTimeout, 250);

#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                while ((soc_pci_read(unit, CMIC_CMCx_MIIM_STAT_OFFSET(cmc)) &
                                             CMIC_MIIM_OPN_DONE) == 0) {
                    if (soc_timeout_check(&to)) {
                        rv = SOC_E_TIMEOUT;
                        break;
                    }
                }
            } else
#endif
            {
                while ((soc_pci_read(unit, CMIC_SCHAN_CTRL) &
                        SC_MIIM_OP_DONE_TST) == 0) {
                    if (soc_timeout_check(&to)) {
                        rv = SOC_E_TIMEOUT;
                        break;
                    }
                }
            }

            if (rv == SOC_E_NONE) {
                LOG_VERBOSE(BSL_LS_SOC_MIIM,
                            (BSL_META_U(unit,
                                        "  Done in %d polls\n"), to.polls));
            }
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                val = soc_pci_read(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc));
                soc_reg_field_set(unit, CMIC_CMC0_MIIM_CTRLr, &val,
                                                         MIIM_RD_STARTf, 0);
                soc_pci_write(unit, CMIC_CMCx_MIIM_CTRL_OFFSET(cmc), val);
            } else
#endif
            {
                soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_OP_DONE_CLR);
            }
        }

        if (rv == SOC_E_TIMEOUT) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "MiimTimeOut:soc_miimc45_read, "
                                  "timeout (id=0x%02x addr=0x%02x)\n"),
                       phy_id, phy_reg_addr));
            SOC_CONTROL(unit)->stat.err_mii_tmo++;
        } else {
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                *phy_rd_data = (uint16)soc_pci_read(unit, CMIC_CMCx_MIIM_READ_DATA_OFFSET(cmc));
            } else
#endif
            {
                *phy_rd_data = (uint16)soc_pci_read(unit, CMIC_MIIM_READ_DATA);
            }

            LOG_INFO(BSL_LS_SOC_MIIM,
                     (BSL_META_U(unit,
                                 "soc_miimc45_read: read data=0x%04x\n"), *phy_rd_data));
        }

        /* Release linkscan pause and MIIM lock */
        MIIM_UNLOCK(unit);

        if(!soc_feature(unit, soc_feature_cmicm)) {
           soc_linkscan_continue(unit);
        }
    }
#endif
#endif
    return rv;

}

/*
 * Function:
 *      soc_sbus_mdio_write
 * Purpose:
 *      Write to an MII register via CMIC SBUS (S-channel) interface.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - 32-bit data to write
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      This function is simply a wrapper for a device-specific implmentation.
 */

int
soc_sbus_mdio_write(int unit, uint16 phy_id,
                    uint32 phy_reg_addr, uint32 phy_wr_data)
{
    soc_sbus_mdio_write_f sbus_write;

    sbus_write = SOC_FUNCTIONS(unit)->soc_sbus_mdio_write;
    if (sbus_write != NULL) {
        return sbus_write(unit, phy_id, phy_reg_addr, phy_wr_data);
    }
    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *      soc_sbus_mdio_read
 * Purpose:
 *      Read from an MII register via CMIC SBUS (S-channel) interface.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 32-bit data to read into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      This function is simply a wrapper for a device-specific implmentation.
 */

int
soc_sbus_mdio_read(int unit, uint16 phy_id,
                   uint32 phy_reg_addr, uint32 *phy_rd_data)
{
    soc_sbus_mdio_read_f sbus_read;

    sbus_read = SOC_FUNCTIONS(unit)->soc_sbus_mdio_read;
    if (sbus_read != NULL) {
        return sbus_read(unit, phy_id, phy_reg_addr, phy_rd_data);
    }
    return SOC_E_UNAVAIL;
}


