/*
 * $Id: regtest.c,v 1.92 Broadcom SDK $
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
 * Register Tests
 */

#include <shared/bsl.h>

#include <sal/types.h>
#include <shared/bsl.h>
#include <soc/defs.h>
#include <soc/debug.h>
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <soc/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#include <bcm/link.h>

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/test.h>
#include <bcm_int/control.h>
#ifdef BCM_TRIUMPH2_SUPPORT
#include <soc/triumph2.h>
#endif
#ifdef BCM_KATANA_SUPPORT
#include <soc/katana.h>
#endif
#ifdef BCM_SIRIUS_SUPPORT
#include <soc/sbx/sirius.h>
#endif

#ifdef BCM_CALADAN3_SUPPORT
#include <sal/appl/config.h>
#endif

#ifdef BCM_DFE_SUPPORT
#include <soc/dfe/cmn/dfe_drv.h>
#endif /*BCM_DFE_SUPPORT*/

#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_POLAR_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)

#if (defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT))
#ifdef BCM_ARAD_SUPPORT
#include <soc/dpp/drv.h>
#endif
extern int bcm_common_linkscan_enable_set(int,int);
#endif

STATIC int rval_test_proc_dispatch(int unit, soc_regaddrinfo_t *ainfo, void *data);

STATIC int rval_test_proc(int unit, soc_regaddrinfo_t *ainfo, void *data);
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
static int rval_test_proc_above_64(int unit, soc_regaddrinfo_t *ainfo, void *data);
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */

/*
 * this is a special marker that is used in soc_reg_iterate()
 * to indicate that no more variations of the current register
 * should be iterated over.
 */
#define SOC_E_IGNORE -6000

struct reg_data {
    int unit;
    int error;
    int flags;
};

#define	REGTEST_FLAG_MINIMAL	0x0001
#define	REGTEST_FLAG_MASK64	0x0002	

/*
 * reg_test
 *
 * Read/write/addressing tests of all SOC internal register R/W bits
 */
STATIC int
try_reg_value(struct reg_data *rd,
              soc_regaddrinfo_t *ainfo,
              char *regname,
              uint32 pattern,
              uint64 mask)
{
    uint64  pat64, rd64, wr64, rrd64, notmask;
    char    wr_str[20], mask_str[20], pat_str[20], rrd_str[20];
    int r;

    /* skip 64b registers in sim */
    if (SAL_BOOT_PLISIM) {
        if (!SOC_IS_XGS(rd->unit) && SOC_REG_IS_64(rd->unit,ainfo->reg)) {
            LOG_WARN(BSL_LS_APPL_COMMON,
                     (BSL_META("Skipping 64 bit %s register in sim\n"),regname));
            return 0;
      }
    }
#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(rd->unit)) {
        if ((r = soc_robo_anyreg_read(rd->unit, ainfo, &rd64)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR: read reg %s failed: %s\n"),
                       regname, soc_errmsg(r)));
            return -1;
        }
    } else 
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
        if ((r = soc_anyreg_read(rd->unit, ainfo, &rd64)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR: read reg %s failed: %s\n"),
                       regname, soc_errmsg(r)));
            return -1;
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    }

    COMPILER_64_SET(pat64, pattern, pattern);
    COMPILER_64_AND(pat64, mask);

    notmask = mask;
    COMPILER_64_NOT(notmask);

    wr64 = rd64;
    COMPILER_64_AND(wr64, notmask);
    COMPILER_64_OR(wr64, pat64);

    format_uint64(wr_str, wr64);
    format_uint64(mask_str, mask);

    LOG_INFO(BSL_LS_APPL_TESTS,
             (BSL_META("Write %s: value %s mask %s\n"),
              regname, wr_str, mask_str));
#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(rd->unit)) {
        if ((r = soc_robo_anyreg_write(rd->unit, ainfo, wr64)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR: write reg %s failed: %s wrote %s (mask %s)\n"),
                       regname, soc_errmsg(r), wr_str, mask_str));
            rd->error = r;
            return -1;
        }
    } else 
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
        if ((r = soc_anyreg_write(rd->unit, ainfo, wr64)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR: write reg %s failed: %s wrote %s (mask %s)\n"),
                       regname, soc_errmsg(r), wr_str, mask_str));
            rd->error = r;
            return -1;
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    }

#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(rd->unit)) {
        if ((r = soc_robo_anyreg_read(rd->unit, ainfo, &rrd64)) < 0) {    	
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR: reread reg %s failed: %s after wrote %s (mask %s)\n"),
                       regname, soc_errmsg(r), wr_str, mask_str));
            rd->error = r;
            return -1;
        }
    } else 
#endif /* BCM_POLAR_SUPPORT */
	{
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
        if ((r = soc_anyreg_read(rd->unit, ainfo, &rrd64)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR: reread reg %s failed: %s after wrote %s (mask %s)\n"),
                       regname, soc_errmsg(r), wr_str, mask_str));
            rd->error = r;
            return -1;
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    }

    COMPILER_64_AND(rrd64, mask);
    format_uint64(rrd_str, rrd64);
    format_uint64(pat_str, pat64);

    LOG_INFO(BSL_LS_APPL_TESTS,
             (BSL_META("Read  %s: value %s expecting %s\n"),
              regname, rrd_str, pat_str));

    if (COMPILER_64_NE(rrd64, pat64)) {
        LOG_ERROR(BSL_LS_APPL_COMMON,
                  (BSL_META("ERROR %s: wrote %s read %s (mask %s)\n"),
                   regname, pat_str, rrd_str, mask_str));
        rd->error = SOC_E_FAIL;
    }

    /* put the register back the way we found it */
#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(rd->unit)) {
        if ((r = soc_robo_anyreg_write(rd->unit, ainfo, rd64)) < 0) {    	
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR: rewrite reg %s failed: %s\n"),
                       regname, soc_errmsg(r)));
            rd->error = r;
            return -1;        
        }
    } else 
#endif /* BCM_POLAR_SUPPORT */
	{
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
        if ((r = soc_anyreg_write(rd->unit, ainfo, rd64)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR: rewrite reg %s failed: %s\n"),
                       regname, soc_errmsg(r)));
            rd->error = r;
            return -1;        
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    }

    return 0;
}

#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
/*
 * Test a register above 64 bit
 * If reg_data.flag can control a minimal test
 */
STATIC int
try_reg_above_64_value(struct reg_data *rd,
                       soc_regaddrinfo_t *ainfo,
                       char *regname,
                       uint32 pattern,
                       soc_reg_above_64_val_t mask)
{
    char    wr_str[256], mask_str[256], pat_str[256], rrd_str[256];
    int r;
    soc_reg_above_64_val_t rd_val, pat, notmask, wr_val, rrd_val;

    /* skip 64b registers in sim */
    if (SAL_BOOT_PLISIM) {
        if (SOC_REG_IS_64(rd->unit,ainfo->reg)) {
            LOG_WARN(BSL_LS_APPL_COMMON,
                     (BSL_META("Skipping 64 bit %s register in sim\n"),regname));
            return 0;
      }
    }

    if ((r = soc_reg_above_64_get(rd->unit, ainfo->reg, (ainfo->port >= 0) ? ainfo->port : REG_PORT_ANY, 0, rd_val)) < 0) {
        LOG_ERROR(BSL_LS_APPL_COMMON,
                  (BSL_META("ERROR: read reg %s failed: %s\n"),
                   regname, soc_errmsg(r)));
        return -1;
    }

    SOC_REG_ABOVE_64_SET_PATTERN(pat, pattern);
    SOC_REG_ABOVE_64_AND(pat, mask);

    SOC_REG_ABOVE_64_COPY(notmask, mask);
    SOC_REG_ABOVE_64_NOT(notmask);

    SOC_REG_ABOVE_64_COPY(wr_val, rd_val);
    SOC_REG_ABOVE_64_AND(wr_val, notmask);
    SOC_REG_ABOVE_64_OR(wr_val, pat);

    format_long_integer(wr_str, wr_val, SOC_REG_ABOVE_64_MAX_SIZE_U32);
    format_long_integer(mask_str, mask, SOC_REG_ABOVE_64_MAX_SIZE_U32);

    LOG_INFO(BSL_LS_APPL_TESTS,
             (BSL_META("Write %s: value %s mask %s\n"),
              regname, wr_str, mask_str));

    if ((r = soc_reg_above_64_set(rd->unit, ainfo->reg, (ainfo->port >= 0) ? ainfo->port : REG_PORT_ANY, 0, wr_val)) < 0) {
	    LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR: write reg %s failed: %s wrote %s (mask %s)\n"),
                       regname, soc_errmsg(r), wr_str, mask_str));
        rd->error = r;
	    return -1;
    }

    if ((r = soc_reg_above_64_get(rd->unit, ainfo->reg, (ainfo->port >= 0) ? ainfo->port : REG_PORT_ANY, 0, rrd_val)) < 0) {
	    LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR: reread reg %s failed: %s after wrote %s (mask %s)\n"),
                       regname, soc_errmsg(r), wr_str, mask_str));
        rd->error = r;
	    return -1;
    }

    SOC_REG_ABOVE_64_AND(rrd_val, mask);
    format_long_integer(rrd_str, rrd_val, SOC_REG_ABOVE_64_MAX_SIZE_U32);
    format_long_integer(pat_str, pat, SOC_REG_ABOVE_64_MAX_SIZE_U32);

    LOG_INFO(BSL_LS_APPL_TESTS,
             (BSL_META("Read  %s: value %s expecting %s\n"),
              regname, rrd_str, pat_str));

    if (!SOC_REG_ABOVE_64_IS_EQUAL(rrd_val, pat)) {
 	    LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR %s: wrote %s read %s (mask %s)\n"),
                       regname, pat_str, rrd_str, mask_str));
	    rd->error = SOC_E_FAIL;
    }

    /* put the register back the way we found it */
    if ((r = soc_reg_above_64_set(rd->unit, ainfo->reg, (ainfo->port >= 0) ? ainfo->port : REG_PORT_ANY, 0, rd_val)) < 0) {
	    LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META("ERROR: rewrite reg %s failed: %s\n"),
                       regname, soc_errmsg(r)));
        rd->error = r;
	    return -1;
    }

    return 0;
}
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */

/*
 * Test a register
 * If reg_data.flag can control a minimal test
 */
STATIC int
try_reg(int unit, soc_regaddrinfo_t *ainfo, void *data)
{
    struct reg_data	*rd = data;
    uint64		mask, mask2, mask3;
    uint32     temp_mask_hi, temp_mask_lo;
    char		regname[80];
#ifdef BCM_HAWKEYE_SUPPORT
    uint32              miscconfig;
    int                 meter = 0;
#endif

    if (!SOC_REG_IS_VALID(unit, ainfo->reg)) {
        return SOC_E_IGNORE;		/* invalid register */
    }

    if (SOC_REG_INFO(unit, ainfo->reg).flags &
        (SOC_REG_FLAG_RO | SOC_REG_FLAG_WO | SOC_REG_FLAG_INTERRUPT | SOC_REG_FLAG_GENERAL_COUNTER | SOC_REG_FLAG_SIGNAL)) {
        return SOC_E_IGNORE;		/* no testable bits */
    }

    if (SOC_REG_INFO(unit, ainfo->reg).regtype == soc_portreg &&
	!SOC_PORT_VALID(unit, ainfo->port)) {
	return 0;			/* skip invalid ports */
    }

#ifdef BCM_ENDURO_SUPPORT
    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit) || SOC_IS_KATANA(unit)) {
        if (ainfo->reg == OAM_SEC_NS_COUNTER_64r) {
            LOG_WARN(BSL_LS_APPL_COMMON,
                     (BSL_META_U(unit,
                                 "Skipping OAM_SEC_NS_COUNTER_64 register\n")));
            return 0;               /* skip OAM_SEC_NS_COUNTER_64 register */
        }
    }
#endif    
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        if ( !soc_feature(unit,soc_feature_ces)  && (SOC_REG_BLOCK_IS(unit, ainfo->reg, SOC_BLK_CES)) ) {
            return 0;
        }
        if ( !soc_feature(unit,soc_feature_ddr3)  && (SOC_REG_BLOCK_IS(unit, ainfo->reg, SOC_BLK_CI)) ) {
            return 0;
        }
    }
#endif

    /*
     * set mask to read-write bits fields
     * (those that are not marked untestable, reserved, read-only,
     * or write-only)
     */
    if (rd->flags & REGTEST_FLAG_MASK64) {
    	mask = soc_reg64_datamask(unit, ainfo->reg, 0);
    	mask2 = soc_reg64_datamask(unit, ainfo->reg, SOCF_RES);
    	mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_RO);
    	COMPILER_64_OR(mask2, mask3);
   		mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_SIG);
    	COMPILER_64_OR(mask2, mask3);
    	mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_WO);
    	COMPILER_64_OR(mask2, mask3);
    	mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_INTR);
    	COMPILER_64_OR(mask2, mask3);
    	mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_W1TC);
    	COMPILER_64_OR(mask2, mask3);
    	mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_COR);
    	COMPILER_64_OR(mask2, mask3);
    	mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_PUNCH);
    	COMPILER_64_OR(mask2, mask3);
    	mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_WVTC);
    	COMPILER_64_OR(mask2, mask3);
    	mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_RWBW);
    	COMPILER_64_OR(mask2, mask3);
    	
    	COMPILER_64_NOT(mask2);
    	COMPILER_64_AND(mask, mask2);
    	
    	
    } else {
	
    	volatile uint32	m32;

    	m32 = soc_reg_datamask(unit, ainfo->reg, 0);
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_RES);
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_RO);
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_WO);
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_W1TC);
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_COR);
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_SIG);
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_INTR);
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_PUNCH);
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_WVTC);
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_RWBW);
#ifdef BCM_POLAR_SUPPORT
        if (SOC_IS_POLAR(unit)) {
            m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_SC);            
        }
#endif /* BCM_POLAR_SUPPORT */
    	
#ifdef BCM_SIRIUS_SUPPORT
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_WVTC);
    	m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_RWBW);
#endif
    	COMPILER_64_SET(mask, 0, m32);
    	
    }

   /* if (mask == 0) {
         return SOC_E_IGNORE;;
    }*/
    COMPILER_64_TO_32_HI(temp_mask_hi,mask);
    COMPILER_64_TO_32_LO(temp_mask_lo,mask);

    if ((temp_mask_hi == 0) && (temp_mask_lo == 0))  {
        return SOC_E_IGNORE;
    }
    
#ifdef BCM_HAWKEYE_SUPPORT
    if (SOC_IS_HAWKEYE(unit) && soc_feature(unit, soc_feature_eee)) {
        if (ainfo->reg == EEE_DELAY_ENTRY_TIMERr) {
        /* The register r/w test should skip bit20 for register 
         *     EEE_DELAY_ENTRY_TIMER.
         * Because bit20 is a constant value before H/W one second delay 
         *     for EEE feature.
         */
            COMPILER_64_SET(mask2, 0, ~0x100000);
            COMPILER_64_AND(mask, mask2);
        }
    }
#endif /* BCM_HAWKEYE_SUPPORT */

    if (COMPILER_64_IS_ZERO(mask)) {
	return SOC_E_IGNORE;		/* no testable bits */
    }

    /*
     * Check if this register is actually implemented in HW for the
     * specified port/cos. If so, the mask is adjusted for the
     * specified port/cos based on what is acutually in HW.
     */
    if (reg_mask_subset(unit, ainfo, &mask)) {
        /* Skip this register. Returning SOC_E_NONE, instead of 
         * SOC_E_IGNORE since we may not want to skip this register
         * all the time (only for certain ports/cos)
         */
        return SOC_E_NONE;
    }

#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(unit)) {
        soc_robo_reg_sprint_addr(unit, regname, ainfo);
    } else 
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
        soc_reg_sprint_addr(unit, regname, ainfo);
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    } 

#ifdef BCM_HAWKEYE_SUPPORT
    if (SOC_IS_HAWKEYE(unit) && (ainfo->reg != MISCCONFIGr)) {
        if (READ_MISCCONFIGr(unit, &miscconfig) < 0) {
            test_error(unit, "Miscconfig read failed\n");
             return SOC_E_IGNORE;
        }
        
        meter = soc_reg_field_get(unit, MISCCONFIGr, 
                              miscconfig, METERING_CLK_ENf);
    
        if(meter){
            soc_reg_field_set(unit, MISCCONFIGr, 
                              &miscconfig, METERING_CLK_ENf, 0);
    
            if (WRITE_MISCCONFIGr(unit, miscconfig) < 0) {
                test_error(unit, "Miscconfig setting failed\n");
                 return SOC_E_IGNORE;
            }
        }
    } 
#endif

    /*
     * minimal test
     * just Fs and 5s
     * only do first instance of each register
     * (only first port, cos, array index, and/or block)
     */
    if (rd->flags & REGTEST_FLAG_MINIMAL) {
	if (try_reg_value(rd, ainfo, regname, 0xffffffff, mask) < 0) {
	    return SOC_E_IGNORE;
	}

	if (try_reg_value(rd, ainfo, regname, 0x55555555, mask) < 0) {
	    return SOC_E_IGNORE;
	}
	return SOC_E_IGNORE;	/* skip other than first instance */
    }

    /*
     * full test
     */
    if (try_reg_value(rd, ainfo, regname, 0x00000000, mask) < 0) {
	return SOC_E_IGNORE;
    }

    if (try_reg_value(rd, ainfo, regname, 0xffffffff, mask) < 0) {
	return SOC_E_IGNORE;
    }

    if (try_reg_value(rd, ainfo, regname, 0x55555555, mask) < 0) {
	return SOC_E_IGNORE;
    }

    if (try_reg_value(rd, ainfo, regname, 0xaaaaaaaa, mask) < 0) {
	return SOC_E_IGNORE;
    }

#ifdef BCM_HAWKEYE_SUPPORT
     if (SOC_IS_HAWKEYE(unit) && (ainfo->reg != MISCCONFIGr)) {
        if (WRITE_MISCCONFIGr(unit, miscconfig) < 0) {
            test_error(unit, "Miscconfig setting failed\n");
            return SOC_E_IGNORE;
        }
    }
#endif

    return 0;
}

#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
STATIC int
try_reg_above_64(int unit, soc_regaddrinfo_t *ainfo, void *data)
{
    struct reg_data	*rd = data;
    char		regname[80];
    soc_reg_above_64_val_t mask, mask2, mask3;

    if (!SOC_REG_IS_VALID(unit, ainfo->reg)) {
        return SOC_E_IGNORE;		/* invalid register */
    }

    if (SOC_REG_INFO(unit, ainfo->reg).flags &
        (SOC_REG_FLAG_RO | SOC_REG_FLAG_WO | SOC_REG_FLAG_INTERRUPT | SOC_REG_FLAG_GENERAL_COUNTER | SOC_REG_FLAG_SIGNAL)) {
        return SOC_E_IGNORE;		/* no testable bits */
    }

    if(SOC_REG_IS_ABOVE_64(unit, ainfo->reg)) {
	    if(SOC_REG_ABOVE_64_INFO(unit, ainfo->reg).size + 2 > CMIC_SCHAN_WORDS(unit)) {
	        return SOC_E_IGNORE;                /* size + header larget than CMIC buffer */
	    }
    }

    if (SOC_REG_INFO(unit, ainfo->reg).regtype == soc_portreg &&
	    !SOC_PORT_VALID(unit, ainfo->port)) {
	        return 0;			/* skip invalid ports */
    }  

    /*
     * set mask to read-write bits fields
     * (those that are not marked untestable, reserved, read-only,
     * or write-only)
     */
     soc_reg_above_64_datamask(unit, ainfo->reg, 0, mask);
    if (SOC_REG_ABOVE_64_IS_ZERO(mask)) {
	    return SOC_E_IGNORE;		/* no testable bits */
    }

    if(SOC_REG_IS_ABOVE_64(unit, ainfo->reg)) {
        if(SOC_REG_ABOVE_64_INFO(unit, ainfo->reg).size + 2 > CMIC_SCHAN_WORDS(unit)) {
            return SOC_E_IGNORE;		/* size + header larget than CMIC buffer */
        }
    }

    soc_reg_sprint_addr(unit, regname, ainfo);




        soc_reg_above_64_datamask(unit, ainfo->reg, 0, mask);
        soc_reg_above_64_datamask(unit, ainfo->reg, SOCF_RES, mask2);
        soc_reg_above_64_datamask(unit, ainfo->reg, SOCF_RO, mask3);
        SOC_REG_ABOVE_64_OR(mask2, mask3);
        soc_reg_above_64_datamask(unit, ainfo->reg, SOCF_SIG, mask3);
        SOC_REG_ABOVE_64_OR(mask2, mask3);
        soc_reg_above_64_datamask(unit, ainfo->reg, SOCF_WO,mask3);
        SOC_REG_ABOVE_64_OR(mask2, mask3);
        soc_reg_above_64_datamask(unit, ainfo->reg, SOCF_INTR,mask3);
        SOC_REG_ABOVE_64_OR(mask2, mask3);
      
        SOC_REG_ABOVE_64_NOT(mask2);
        SOC_REG_ABOVE_64_AND(mask, mask2);
            
   
        if (SOC_REG_ABOVE_64_IS_ZERO(mask)  == TRUE)  {
            return SOC_E_IGNORE;
        }


    /*
     * minimal test
     * just Fs and 5s
     * only do first instance of each register
     * (only first port, cos, array index, and/or block)
     */
    if (rd->flags & REGTEST_FLAG_MINIMAL) {
    	if (try_reg_above_64_value(rd, ainfo, regname, 0xffffffff, mask) < 0) {
    	    return SOC_E_IGNORE;
    	}
    
    	if (try_reg_above_64_value(rd, ainfo, regname, 0x55555555, mask) < 0) {
    	    return SOC_E_IGNORE;
    	}
    	
    	return SOC_E_IGNORE;	/* skip other than first instance */
    }

    /*
     * full test
     */
    if (try_reg_above_64_value(rd, ainfo, regname, 0x00000000, mask) < 0) {
	    return SOC_E_IGNORE;
    }

    if (try_reg_above_64_value(rd, ainfo, regname, 0xffffffff, mask) < 0) {
	    return SOC_E_IGNORE;
    }

    if (try_reg_above_64_value(rd, ainfo, regname, 0x55555555, mask) < 0) {
	    return SOC_E_IGNORE;
    }

    if (try_reg_above_64_value(rd, ainfo, regname, 0xaaaaaaaa, mask) < 0) {
	    return SOC_E_IGNORE;
    }

    return 0;
}
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */

STATIC int
try_reg_dispatch(int unit, soc_regaddrinfo_t *ainfo, void *data)
{

#if defined (BCM_DFE_SUPPORT)
    if(SOC_IS_DFE(unit)) 
    { 
        int rv;
        int is_filtered = 0;
        
        rv = MBCM_DFE_DRIVER_CALL(unit, mbcm_dfe_drv_test_reg_filter, (unit, ainfo->reg, &is_filtered));
        if (rv != SOC_E_NONE)
        {
            return rv;
        }
        if (is_filtered)
        {
            return SOC_E_IGNORE;
        }
    }
#endif /*BCM_DFE_SUPPORT*/
#if defined (BCM_ARAD_SUPPORT)
        if(SOC_IS_ARAD(unit)) {
            switch(ainfo->reg) {
                case ECI_REG_0001r:
                case EGQ_INDIRECT_COMMANDr:
                        return SOC_E_IGNORE;
                default:
                    break;
            }

        }
#endif
#ifdef BCM_POLAR_SUPPORT
        if (SOC_IS_POLAR(unit)) {
            switch (ainfo->reg) {
                case CFP_DATAr:
                case CFP_MASKr:
                case EGRESS_VID_RMK_TBL_ACSr:
                case PEAK_TEMP_MON_RESUr:
                case TEMP_MON_RESUr:
                case WATCH_DOG_CTRLr:
                case LED_EN_MAPr:
                case INT_STSr:
                    /* Skip these registers */
                    return SOC_E_IGNORE;
                default:
                    if (((ainfo->addr & 0xf000) == 0x1000) ||
                        ((ainfo->addr & 0xf000) == 0x8000) ||
                        ((ainfo->addr & 0xfff0) == 0xe020) ||
                        ((ainfo->addr & 0xfff0) == 0x0040)) {
                        /* Skip PHY, IO PAD and POWER registers */
                        return SOC_E_IGNORE;
                    }
                    break;
            }
        }
#endif /* BCM_POLAR_SUPPORT */

#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
    if(SOC_REG_IS_ABOVE_64(unit, ainfo->reg)) {
        return try_reg_above_64(unit, ainfo, data);
    } else 
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    { /* To work on 64 AND '32' bits regs */
        return try_reg(unit, ainfo, data);
    }
}
STATIC int
rval_test_proc_dispatch(int unit, soc_regaddrinfo_t *ainfo, void *data)
{
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
    if(SOC_REG_IS_ABOVE_64(unit, ainfo->reg)) {
        return rval_test_proc_above_64(unit, ainfo, data);
    } else 
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    { /* To work on 64 AND '32' bits regs */
        return rval_test_proc(unit, ainfo, data);
    }
}

/*
 * Register read/write test (tr 3)
 */
int
reg_test(int unit, args_t *a, void *pa)
{
    struct reg_data rd;
    int r, rv = 0;
    char *s;
#if defined(BCM_HAWKEYE_SUPPORT) || defined (BCM_ESW_SUPPORT) || \
    defined (BCM_SIRIUS_SUPPORT) /* DPPCOMPILEENABLE */
    uint32 tem;
#endif /* BCM_HAWKEYE_SUPPORT || BCM_ESW_SUPPORT  || DPPCOMPILEENABE */
    COMPILER_REFERENCE(pa);

    LOG_INFO(BSL_LS_APPL_TESTS,
             (BSL_META_U(unit,
                         "Register read/write test\n")));

    rd.unit = unit;
    rd.error = SOC_E_NONE;
    rd.flags = 0;
    while ((s = ARG_GET(a)) != NULL) {
        if (sal_strcasecmp(s, "mini") == 0 ||
            sal_strcasecmp(s, "minimal") == 0) {
            rd.flags |= REGTEST_FLAG_MINIMAL;
            continue;
        }
        if (sal_strcasecmp(s, "mask64") == 0 ||
            sal_strcasecmp(s, "datamask64") == 0) {
            rd.flags |= REGTEST_FLAG_MASK64;
            continue;
        }
        LOG_WARN(BSL_LS_APPL_COMMON,
                 (BSL_META_U(unit,
                             "WARNING: unknown argument '%s' ignored\n"), s));
    }

    if (!SOC_UNIT_VALID(unit)) {
        return SOC_E_UNIT;
    }
#ifdef BCM_ARAD_SUPPORT
    if (SOC_IS_ARAD(unit))
    {
      soc_counter_stop(unit);
    }
#endif
        if (BCM_UNIT_VALID(unit)) {
            rv = bcm_linkscan_enable_set(unit, 0); /* disable linkscan */
            if(rv != SOC_E_UNAVAIL) { /* if unavail - no need to disable */
                BCM_IF_ERROR_RETURN(rv);
            }
        }

#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        if ((r = soc_sirius_reset(unit)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META_U(unit,
                                  "ERROR: Unable to reset unit %d: %s\n"),
                       unit, soc_errmsg(r)));
            goto done;
        }
    }
    else
#endif
    {
#if (defined (BCM_ARAD_SUPPORT))    
        if (SOC_IS_ARAD(unit))
        {
             if ((r = soc_dpp_device_reset(unit, SOC_DPP_RESET_MODE_REG_ACCESS,SOC_DPP_RESET_ACTION_INOUT_RESET)) < 0) {
                   LOG_ERROR(BSL_LS_APPL_COMMON,
                             (BSL_META_U(unit,
                                         "ERROR: Unable to reinit unit %d: %s\n"), unit, soc_errmsg(r)));
                goto done;
            }
        } else
#endif
#ifdef BCM_DFE_SUPPORT
        if (SOC_IS_DFE(unit))
        {
            r = MBCM_DFE_DRIVER_CALL(unit, mbcm_dfe_drv_blocks_reset, (unit, 0 , NULL));
            if (r != SOC_E_NONE)
            {
                LOG_ERROR(BSL_LS_APPL_COMMON,
                          (BSL_META_U(unit,
                                      "ERROR: Unable to reinit unit %d: %s\n"), unit, soc_errmsg(r)));
                goto done;
            }
        } else 
#endif
        {
#ifdef BCM_CALADAN3_SUPPORT
            if (SOC_IS_CALADAN3(unit)) {
                sal_config_set("diag_emulator_partial_init", "1");
            }
#endif
            if ((r = soc_reset_init(unit)) < 0) {
                LOG_ERROR(BSL_LS_APPL_COMMON,
                          (BSL_META_U(unit,
                                      "ERROR: Unable to reset unit %d: %s\n"),
                           unit, soc_errmsg(r)));
                goto done;
            }
        }
    }
    
    if (SOC_IS_HB_GW(unit) && soc_feature(unit, soc_feature_bigmac_rxcnt_bug)) {
        /*
         * We need to wait for auto-negotiation to complete to ensure
         * that the BigMAC Rx counters will respond correctly.
         */
        sal_usleep(500000);
    }

    /* When doing E2EFC register/memory read/write tests, it is strongly recommended to set this parity genrate 
     *  enable bit to 0. This will prevent hardware automatically overwritting the ECC and
     *  parity field in E2EFC registers/memories contents and failing the tests.
     */
#if defined(BCM_HAWKEYE_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) /* DPPCOMPILEENABLE */
    if (soc_reg_field_valid(unit, MISCCONFIGr, E2EFC_PARITY_GEN_ENf)) {
        SOC_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &tem));
        soc_reg_field_set(unit, MISCCONFIGr, &tem, E2EFC_PARITY_GEN_ENf, 0);
        SOC_IF_ERROR_RETURN(WRITE_MISCCONFIGr(unit, tem));
    } else if (soc_reg_field_valid(unit, MISCCONFIGr, PARITY_CHECK_ENf)) {
        /* If there is no E2EFC_PARITY_GEN_EN field, 
         * try to disable PARITY_ENABLE_EN 
         */
        SOC_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &tem));
        soc_reg_field_set(unit, MISCCONFIGr, &tem, PARITY_CHECK_ENf, 0);
        SOC_IF_ERROR_RETURN(WRITE_MISCCONFIGr(unit, tem)); 
    }
#endif     /* DPPCOMPILEENABLE */
   
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        soc_port_t port;
        soc_port_t port_max = SOC_INFO(unit).cpu_hg_index;

        /* Initialize the MMU port mapping so the backpressure
         * registers work properly.
         */
        for (port = 0; port < port_max; port++) {
            SOC_IF_ERROR_RETURN
                (WRITE_MMU_TO_PHY_PORT_MAPPINGr(unit, port, port));
        }

        SOC_IF_ERROR_RETURN
            (WRITE_MMU_TO_PHY_PORT_MAPPINGr(unit, 0, 59));
        SOC_IF_ERROR_RETURN
            (WRITE_MMU_TO_PHY_PORT_MAPPINGr(unit, 59, 0));

        /* Turn off the background MMU processes */
        if ((rv = _soc_triumph3_mem_parity_control(unit, INVALIDm,
                                           SOC_BLOCK_ALL, FALSE)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META_U(unit,
                                  "ERROR: Unable to stop HW updates on unit %d: %s\n"),
                       unit, soc_errmsg(r)));
            goto done;
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */ 
    /*
     * If try_reg returns -1, there was a register access failure rather
     * than a bit error.
     *
     * If try_reg returns 0, then rd.error is -1 if there was a bit
     * error.
     */
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, IARB_REGS_DEBUGr, REG_PORT_ANY,
                                    DEBUGf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, IPARS_REGS_DEBUGr, REG_PORT_ANY,
                                    DEBUGf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, IVLAN_REGS_DEBUGr, REG_PORT_ANY,
                                    DEBUGf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, ISW1_REGS_DEBUGr, REG_PORT_ANY,
                                    DEBUGf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, ISW2_REGS_DEBUGr, REG_PORT_ANY,
                                    DEBUGf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, IL_DEBUG_CONFIGr, 9,
                                    IL_TREX2_DEBUG_LOCKf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, IL_DEBUG_CONFIGr, 13,
                                    IL_TREX2_DEBUG_LOCKf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_THD_1_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_THD_0_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_PORT_THD_0_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_PORT_THD_1_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_CFG_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_PORT_CFG_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G0_BUCKET_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G0_CONFIG_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G1_BUCKET_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G1_CONFIG_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G2_BUCKET_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G2_CONFIG_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G3_BUCKET_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G3_CONFIG_ENABLE_ECCf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_CPU_PKT_ENABLE_ECCf, 0));
    }
#endif

#ifdef BCM_CALADAN3_SUPPORT
    if (SOC_IS_CALADAN3(unit)) {
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, RC_GLOBAL_DEBUGr, REG_PORT_ANY,
                                    TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, QM_DEBUGr, REG_PORT_ANY,
                                    QM_TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, CI_RESETr, REG_PORT_ANY,
                                    TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, PT_GEN_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK | 0),
                                    PT_TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, PT_GEN_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK | 1),
                                    PT_TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, PR_GLOBAL_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK | 0),
                                    TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, PR_GLOBAL_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK | 1),
                                    TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, PB_CONFIGr, REG_PORT_ANY,
                                    PB_TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, CX_GLOBAL_DEBUGr, REG_PORT_ANY,
                                    TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, TMA_DEBUGr, REG_PORT_ANY,
                                    TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, TMB_DEBUGr, REG_PORT_ANY,
                                    TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, CO_GLOBAL_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK|0),
                                    TREX2_DEBUG_ENABLEf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, CO_GLOBAL_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK|1),
                                    TREX2_DEBUG_ENABLEf, 1));
    
    
    }
#endif

#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(unit)) {
        if (soc_robo_reg_iterate(unit, try_reg_dispatch, &rd) < 0) {
            LOG_INFO(BSL_LS_APPL_TESTS,
                     (BSL_META_U(unit,
                                 "Continuing test.\n")));
   	        rv = 0;
        } else {
            rv = rd.error;
        }
    } else 
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
        if (soc_reg_iterate(unit, try_reg_dispatch, &rd) < 0) {
            LOG_INFO(BSL_LS_APPL_TESTS,
                     (BSL_META_U(unit,
                                 "Continuing test.\n")));
	        rv = 0;
        } else {
	        rv = rd.error;
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    } 

#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT)
	/* Re-Enable Parity Gen */
    if (soc_reg_field_valid(unit, MISCCONFIGr, E2EFC_PARITY_GEN_ENf)) {
		SOC_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &tem));
		soc_reg_field_set(unit, MISCCONFIGr, &tem, E2EFC_PARITY_GEN_ENf, 1);
		SOC_IF_ERROR_RETURN(WRITE_MISCCONFIGr(unit, tem));
    } else if (soc_reg_field_valid(unit, MISCCONFIGr, PARITY_CHECK_ENf)) {
        /* Re-enable PARITY_ENABLE */
        if (soc_property_get(unit, spn_PARITY_ENABLE, TRUE)) {
            SOC_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &tem));
            soc_reg_field_set(unit, MISCCONFIGr, &tem, PARITY_CHECK_ENf, 1);
            SOC_IF_ERROR_RETURN(WRITE_MISCCONFIGr(unit, tem)); 
        }
    }
#endif
   
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        if ((rv = _soc_triumph3_mem_parity_control(unit, INVALIDm,
                                           SOC_BLOCK_ALL, TRUE)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META_U(unit,
                                  "ERROR: Unable to restart HW updates on unit %d: %s\n"),
                       unit, soc_errmsg(r)));
            goto done;
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

done:
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, IARB_REGS_DEBUGr, REG_PORT_ANY,
                                    DEBUGf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, IPARS_REGS_DEBUGr, REG_PORT_ANY,
                                    DEBUGf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, IVLAN_REGS_DEBUGr, REG_PORT_ANY,
                                    DEBUGf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, ISW1_REGS_DEBUGr, REG_PORT_ANY,
                                    DEBUGf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, ISW2_REGS_DEBUGr, REG_PORT_ANY,
                                    DEBUGf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, IL_DEBUG_CONFIGr, 9,
                                    IL_TREX2_DEBUG_LOCKf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, IL_DEBUG_CONFIGr, 13,
                                    IL_TREX2_DEBUG_LOCKf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_THD_1_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_THD_0_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_PORT_THD_0_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_PORT_THD_1_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_CFG_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    WRED_PORT_CFG_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G0_BUCKET_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G0_CONFIG_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G1_BUCKET_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G1_CONFIG_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G2_BUCKET_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G2_CONFIG_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G3_BUCKET_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_SHAPE_G3_CONFIG_ENABLE_ECCf, 1));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MMU_ECC_DEBUG1r, REG_PORT_ANY,
                                    MTRO_CPU_PKT_ENABLE_ECCf, 1));
    }
#endif
#ifdef BCM_CALADAN3_SUPPORT
    if (SOC_IS_CALADAN3(unit)) {
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, RC_GLOBAL_DEBUGr, REG_PORT_ANY,
                                    TREX2_DEBUG_ENABLEf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, QM_DEBUGr, REG_PORT_ANY,
                                    QM_TREX2_DEBUG_ENABLEf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, CI_RESETr, REG_PORT_ANY,
                                    TREX2_DEBUG_ENABLEf, 0));

        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, PT_GEN_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK | 0),
                                    PT_TREX2_DEBUG_ENABLEf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, PT_GEN_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK | 1),
                                    PT_TREX2_DEBUG_ENABLEf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, PR_GLOBAL_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK | 0),
                                    TREX2_DEBUG_ENABLEf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, PR_GLOBAL_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK | 1),
                                    TREX2_DEBUG_ENABLEf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, PB_CONFIGr, REG_PORT_ANY,
                                    PB_TREX2_DEBUG_ENABLEf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, CX_GLOBAL_DEBUGr, REG_PORT_ANY,
                                    TREX2_DEBUG_ENABLEf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, TMA_DEBUGr, REG_PORT_ANY,
                                    TREX2_DEBUG_ENABLEf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, TMB_DEBUGr, REG_PORT_ANY,
                                    TREX2_DEBUG_ENABLEf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, CO_GLOBAL_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK|0),
                                    TREX2_DEBUG_ENABLEf, 0));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, CO_GLOBAL_CONFIGr, (SOC_REG_ADDR_INSTANCE_MASK|1),
                                    TREX2_DEBUG_ENABLEf, 0));
    
        sal_config_set("diag_emulator_partial_init", "0");
        if ((r = soc_reset_init(unit)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META_U(unit,
                                  "ERROR: Unable to reset unit %d: %s\n"), unit, soc_errmsg(r)));
            return r;
        }
    }
#endif
    if (rv < 0) {
	test_error(unit, "Register read/write test failed\n");
	return rv;
    }

    return rv;
}

#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
/*
 * rval_test
 *
 * Reset SOC and compare reset values of all SOC registers with regsfile
 */

STATIC int
rval_test_proc_above_64(int unit, soc_regaddrinfo_t *ainfo, void *data)
{
    struct reg_data *rd = data;
    char            buf[80];
    soc_reg_above_64_val_t          rmsk, rval, rrd_val;
    int             r;
    char    wr_str[256], mask_str[256], rval_str[256], rrd_str[256];
    soc_reg_t       reg;

    reg = ainfo->reg;

   

    SOC_REG_ABOVE_64_RST_VAL_GET(unit, reg, rval);
    SOC_REG_ABOVE_64_RST_MSK_GET(unit, reg, rmsk);
    SOC_REG_ABOVE_64_AND(rval, rmsk);

    if (rval_test_skip_reg(unit, ainfo)) {
        /* soc_reg_sprint_addr(unit, buf, ainfo);
            LOG_WARN(BSL_LS_APPL_COMMON,
                     (BSL_META_U(unit,
                                 "Skipping register %s\n"), buf)); */
        return 0;
    }

    if (SOC_REG_ABOVE_64_IS_ZERO(rmsk)) {
        return 0;   /* No reset value */
    }

    if(SOC_REG_IS_ABOVE_64(unit, ainfo->reg)) {
        if(SOC_REG_ABOVE_64_INFO(unit, ainfo->reg).size + 2 > CMIC_SCHAN_WORDS(unit)) {
            return SOC_E_IGNORE;  /* size + header larget than CMIC buffer */
            }
    }

    soc_reg_sprint_addr(unit, buf, ainfo);


    if ((r = soc_reg_above_64_get(rd->unit, ainfo->reg, (ainfo->port >= 0) ? ainfo->port : REG_PORT_ANY, 0, rrd_val)) < 0) {
        LOG_ERROR(BSL_LS_APPL_COMMON,
                  (BSL_META_U(unit,
                              "ERROR: reread reg %s failed: %s after wrote %s (mask %s)\n"),
                   buf, soc_errmsg(r), wr_str, mask_str));
        rd->error = SOC_E_FAIL; 
    }

  
    SOC_REG_ABOVE_64_AND(rrd_val, rmsk);
   
    format_long_integer(rrd_str, rrd_val, SOC_REG_ABOVE_64_MAX_SIZE_U32);
    format_long_integer(rval_str, rval, SOC_REG_ABOVE_64_MAX_SIZE_U32);
    format_long_integer(mask_str, rmsk, SOC_REG_ABOVE_64_MAX_SIZE_U32);


    if (!SOC_REG_ABOVE_64_IS_EQUAL(rrd_val, rval)) {
        LOG_ERROR(BSL_LS_APPL_COMMON,
                  (BSL_META_U(unit,
                              "ERROR %s: default %s read %s (mask %s)\n"),
                   buf, rval_str, rrd_str, mask_str));
        rd->error = SOC_E_FAIL;
    }

    return 0;
}
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */

/*
 * rval_test
 *
 * Reset SOC and compare reset values of all SOC registers with regsfile
 */

STATIC int
rval_test_proc(int unit, soc_regaddrinfo_t *ainfo, void *data)
{
    struct reg_data *rd = data;
    char            buf[80];
    uint64          value, rmsk, rval, chk;
    char            val_str[20], rmsk_str[20], rval_str[20];
    int             r;
    uint64          mask2,mask3;
    uint32          temp_mask_hi, temp_mask_lo;
    soc_reg_t       reg;

    reg = ainfo->reg;

    /* NOTE: This is experimental */
#ifdef  BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit) && 
        (reg == EGR_OUTER_TPIDr || reg == ING_MPLS_TPIDr ||
         reg == ING_OUTER_TPIDr)) {
        reg = reg+ainfo->idx+1;
    }
#endif
    SOC_REG_RST_MSK_GET(unit, reg, rmsk);

    if (rval_test_skip_reg(unit, ainfo)) {
        /* soc_reg_sprint_addr(unit, buf, ainfo);
         LOG_WARN(BSL_LS_APPL_COMMON,
                  (BSL_META_U(unit,
                              "Skipping register %s\n"), buf));*/

        return 0;
    }

#ifdef BCM_ENDURO_SUPPORT
    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit)) {
        if (ainfo->reg == OAM_SEC_NS_COUNTER_64r) {
            LOG_WARN(BSL_LS_APPL_COMMON,
                     (BSL_META_U(unit,
                                 "Skipping OAM_SEC_NS_COUNTER_64 register\n")));
            return 0;                /* skip OAM_SEC_NS_COUNTER_64 register */
        }
    }
#endif    

#ifdef BCM_HAWKEYE_SUPPORT
    if (SOC_IS_HAWKEYE(unit)) {
        if (ainfo->reg == MAC_MODEr) {
            LOG_WARN(BSL_LS_APPL_COMMON,
                     (BSL_META_U(unit,
                                 "Skipping MAC_MODE register\n")));
            return 0;                /* skip MAC_MODE register */
        }
    }
#endif    
#ifdef BCM_KATANA_SUPPORT
        if (SOC_IS_KATANA(unit)) {
            if ( !soc_feature(unit,soc_feature_ces)  && (SOC_REG_BLOCK_IS(unit, ainfo->reg, SOC_BLK_CES)) ) {
                return 0;
            }
            if ( !soc_feature(unit,soc_feature_ddr3)  && (SOC_REG_BLOCK_IS(unit, ainfo->reg, SOC_BLK_CI)) ) {
                return 0;
            }
        }
#endif    

#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        if (ainfo->reg == TOP_XG_PLL0_CTRL_3r) {
            LOG_WARN(BSL_LS_APPL_COMMON,
                     (BSL_META_U(unit,
                                 "Skipping TOP_XG_PLL0_CTRL_3 register\n"))); 
            return 0;
        }
    }
#endif
    
    if (SAL_BOOT_PLISIM) {
        if (!SOC_IS_XGS(rd->unit) && SOC_REG_IS_64(rd->unit,ainfo->reg)) {
#ifdef BCM_POLAR_SUPPORT
            if (SOC_IS_POLAR(unit)) {
                soc_robo_reg_sprint_addr(unit, buf, ainfo);
            } else 
#endif /* BCM_POLAR_SUPPORT */
            {
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT)
                soc_reg_sprint_addr(unit, buf, ainfo);
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
            }
            LOG_WARN(BSL_LS_APPL_COMMON,
                     (BSL_META_U(unit,
                                 "Skipping 64 bit %s register in sim\n"),buf));
            return 0;
      }
    }

    if (SOC_IS_DPP(unit) || SOC_IS_DFE(unit)) { /* NOTE: Without this check tr 1 breaks for all XGS chips */
        if (rd->flags & REGTEST_FLAG_MASK64) {
            rmsk = soc_reg64_datamask(unit, ainfo->reg, 0);
            mask2 = soc_reg64_datamask(unit, ainfo->reg, SOCF_IGNORE_DEFAULT_TEST);
            mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_WO);

            COMPILER_64_OR(mask2, mask3); 

        /*    mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_INTR);
            COMPILER_64_OR(mask2, mask3);

            mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_COR);
            COMPILER_64_OR(mask2, mask3); */
            mask3 = soc_reg64_datamask(unit, ainfo->reg, SOCF_W1TC);
            COMPILER_64_OR(mask2, mask3);
            
            COMPILER_64_NOT(mask2);
            COMPILER_64_AND(rmsk, mask2);
            
            
        } else {
        
            volatile uint32 m32;
    
            m32 = soc_reg_datamask(unit, ainfo->reg, 0);
            m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_IGNORE_DEFAULT_TEST);
            m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_WO);

            
#ifdef BCM_SIRIUS_SUPPORT
            m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_PUNCH);
            m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_WVTC);
            m32 &= ~soc_reg_datamask(unit, ainfo->reg, SOCF_RWBW);
#endif
            COMPILER_64_SET(rmsk, 0, m32);
            
        }

        COMPILER_64_TO_32_HI(temp_mask_hi,rmsk);
        COMPILER_64_TO_32_LO(temp_mask_lo,rmsk);
    
        if ((temp_mask_hi == 0) && (temp_mask_lo == 0))  {
            return 0;
        }
    }

    /*
     * Check if this register is actually implemented in HW for the
     * specified port/cos. If so, the mask is adjusted for the
     * specified port/cos based on what is acutually in HW.
     */
    if (reg_mask_subset(unit, ainfo, &rmsk)) {
        return 0;
    }

    if (COMPILER_64_IS_ZERO(rmsk)) {
        return 0;   /* No reset value */
    }

    SOC_REG_RST_VAL_GET(unit, reg, rval);
#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(unit)) {
        soc_robo_reg_sprint_addr(unit, buf, ainfo);
    } else 
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT)
        soc_reg_sprint_addr(unit, buf, ainfo);
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    }

#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(rd->unit)) {
        if ((r = soc_robo_anyreg_read(rd->unit, ainfo, &value)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META_U(unit,
                                  "ERROR: read reg %s (0x%x) failed: %s\n"),
                       buf, ainfo->addr, soc_errmsg(r)));
            rd->error = r;
            return -1;
        }
    } else 
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
        if ((r = soc_anyreg_read(rd->unit, ainfo, &value)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META_U(unit,
                                  "ERROR: read reg %s (0x%x) failed: %s\n"),
                       buf, ainfo->addr, soc_errmsg(r)));
            rd->error = r;
        return -1;        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    }

    format_uint64(val_str, value);
    format_uint64(rmsk_str, rmsk);
    format_uint64(rval_str, rval);
    LOG_INFO(BSL_LS_APPL_TESTS,
             (BSL_META_U(unit,
                         "Read %s: reset mask %s, reset value %s, read %s\n"),
              buf, rmsk_str, rval_str, val_str));

    /* Check reset value is correct */
    COMPILER_64_ZERO(chk);
    COMPILER_64_ADD_64(chk, value);
    COMPILER_64_XOR(chk, rval);
    COMPILER_64_AND(chk, rmsk);

    if (!COMPILER_64_IS_ZERO(chk)) {
        COMPILER_64_AND(rval, rmsk);
        format_uint64(rval_str, rval);
        COMPILER_64_AND(value, rmsk);
        format_uint64(val_str, value);
        LOG_ERROR(BSL_LS_APPL_COMMON,
                  (BSL_META_U(unit,
                              "ERROR: %s: expected %s, got %s, reset mask %s\n"),
                   buf, rval_str, val_str, rmsk_str));
        rd->error = SOC_E_FAIL;
    }

    return 0;
}

/*
 * Register reset value test (tr 1)
 */
int
rval_test(int unit, args_t *a, void *pa)
{
    struct reg_data rd;
    int r, rv = -1;
    char *s;

    COMPILER_REFERENCE(pa);

    LOG_INFO(BSL_LS_APPL_TESTS,
             (BSL_META_U(unit,
                         "Register reset value test\n")));

    rd.unit = unit;
    rd.error = SOC_E_NONE;
    rd.flags = 0;
    while ((s = ARG_GET(a)) != NULL) {
        LOG_WARN(BSL_LS_APPL_COMMON,
                 (BSL_META_U(unit,
                             "WARNING: unknown argument '%s' ignored\n"), s));
    }

    if (!SOC_UNIT_VALID(unit)) {
        return SOC_E_UNIT;
    }
  /*  if (!SOC_IS_ARAD(unit)) {*/
        if (BCM_UNIT_VALID(unit)) {
            rv = bcm_linkscan_enable_set(unit, 0); /* disable linkscan */
            if(rv != SOC_E_UNAVAIL) { /* if unavail - no need to disable */
                BCM_IF_ERROR_RETURN(rv);
            }
        }
   /* } */
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit)) {
        soc_triumph2_pipe_mem_clear(unit);
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        if ((r = soc_sirius_reset(unit)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META_U(unit,
                                  "ERROR: Unable to reset unit %d: %s\n"),
                       unit, soc_errmsg(r)));
            goto done;
        }
    }
    else
#endif
    {

#if defined(BCM_ARAD_SUPPORT)
        if (SOC_IS_ARAD(unit))
        {
            soc_counter_stop(unit);
            if ((r = soc_dpp_device_reset(unit, SOC_DPP_RESET_MODE_REG_ACCESS,SOC_DPP_RESET_ACTION_INOUT_RESET)) < 0) {
                LOG_ERROR(BSL_LS_APPL_COMMON,
                          (BSL_META_U(unit,
                                      "ERROR: Unable to reinit unit %d: %s\n"), unit, soc_errmsg(r)));
                goto done;
            }
        } else 
#endif
#if defined(BCM_DFE_SUPPORT)
        if (SOC_IS_DFE(unit))
        {
            soc_counter_stop(unit);
            sal_sleep(1);            
            r = MBCM_DFE_DRIVER_CALL(unit, mbcm_dfe_drv_blocks_reset, (unit, 0 , NULL));
            if (r != SOC_E_NONE)
            {
                LOG_ERROR(BSL_LS_APPL_COMMON,
                          (BSL_META_U(unit,
                                      "ERROR: Unable to reinit unit %d: %s\n"), unit, soc_errmsg(r)));
                goto done;
            }
            sal_sleep(1);

        } else 
#endif
        {
#ifdef BCM_CALADAN3_SUPPORT
            if (SOC_IS_CALADAN3(unit)) {
                sal_config_set("diag_emulator_partial_init", "1");
            }
#endif
#if defined(BCM_ARAD_SUPPORT)
            if (!SOC_IS_ARAD(unit)) 
#endif
            {
#ifdef BCM_POLAR_SUPPORT
                if (SOC_IS_POLAR(unit)) {
                    if ((r = soc_robo_chip_reset(unit)) < 0) {
                        LOG_ERROR(BSL_LS_APPL_COMMON,
                                  (BSL_META_U(unit,
                                              "ERROR: Unable to reset unit %d: %s\n"),
                                   unit, soc_errmsg(r)));
                        goto done;
                    }
                } else 
#endif /* BCM_POLAR_SUPPORT */
                {
                    if ((r = soc_reset_init(unit)) < 0) {
                        LOG_ERROR(BSL_LS_APPL_COMMON,
                                  (BSL_META_U(unit,
                                              "ERROR: Unable to reset unit %d: %s\n"),
                                   unit, soc_errmsg(r)));
                        goto done;
                    }
                }
            }
        }
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) 
        || SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit)
        || SOC_IS_HURRICANE(unit)) {
        /* reset the register XQPORT_XGXS_NEWCTL_REG to 0x0 */
        soc_port_t port;
        PBMP_PORT_ITER(unit, port) {
            switch(port) {
            case 26:
            case 27:
            case 28:
            case 29:
                if(SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit)) {
                    SOC_IF_ERROR_RETURN(WRITE_XQPORT_XGXS_NEWCTL_REGr(
                                                       unit, port, 0x0));
                } else {
                    SOC_IF_ERROR_RETURN(WRITE_XPORT_XGXS_NEWCTL_REGr(
                                                       unit, port, 0x0));
                }
                break;
            case 30:
            case 34:
            case 38:
            case 42:
            case 46:
            case 50:
                SOC_IF_ERROR_RETURN(WRITE_XQPORT_XGXS_NEWCTL_REGr(unit, port, 0x0));
                break;
            default:
                break;
            }
        }
    }
#endif

    sal_usleep(10000);

#ifdef BCM_POLAR_SUPPORT
    if (SOC_IS_POLAR(unit)) {
        if (soc_robo_reg_iterate(unit, rval_test_proc_dispatch, &rd) < 0) {
            goto done;
        }
    } else 
#endif /* BCM_POLAR_SUPPORT */
    {
#if defined (BCM_ESW_SUPPORT) || defined (BCM_SIRIUS_SUPPORT) || defined (BCM_PETRA_SUPPORT) || defined (BCM_DFE_SUPPORT)
        if (soc_reg_iterate(unit, rval_test_proc_dispatch, &rd) < 0) {
            goto done;
        }
#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT || BCM_DFE_SUPPORT */
    }

    rv = rd.error;

 done:
    if (rv < 0) {
        test_error(unit, "Register reset value test failed\n");
    }
#ifdef BCM_CALADAN3_SUPPORT
    if (SOC_IS_CALADAN3(unit)) {
        sal_config_set("diag_emulator_partial_init", "0");
    }
#endif

#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        if ((r = soc_sirius_reset(unit)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META_U(unit,
                                  "ERROR: Unable to reset unit %d: %s\n"),
                       unit, soc_errmsg(r)));
        }
    }
    else
#endif
    {
#ifdef BCM_ARAD_SUPPORT
        if (SOC_IS_ARAD(unit))
        {
            if ((r = soc_dpp_device_reset(unit, SOC_DPP_RESET_MODE_REG_ACCESS,SOC_DPP_RESET_ACTION_INOUT_RESET)) < 0) {
                LOG_ERROR(BSL_LS_APPL_COMMON,
                          (BSL_META_U(unit,
                                      "ERROR: Unable to reinit unit %d: %s\n"), unit, soc_errmsg(r)));
            
            }
        } else 
#endif
#ifdef BCM_DFE_SUPPORT
        if (SOC_IS_DFE(unit))
        {
            r = MBCM_DFE_DRIVER_CALL(unit, mbcm_dfe_drv_blocks_reset, (unit, 0 , NULL));
            if (r != SOC_E_NONE)
            {
                LOG_ERROR(BSL_LS_APPL_COMMON,
                          (BSL_META_U(unit,
                                      "ERROR: Unable to reinit unit %d: %s\n"), unit, soc_errmsg(r)));
                goto done;
            }
        } else 
#endif
        if ((r = soc_reset_init(unit)) < 0) {
            LOG_ERROR(BSL_LS_APPL_COMMON,
                      (BSL_META_U(unit,
                                  "ERROR: Unable to reset unit %d: %s\n"),
                       unit, soc_errmsg(r)));
        }
    }

    return rv;
}


#endif /* BCM_ESW_SUPPORT || BCM_SIRIUS_SUPPORT */

