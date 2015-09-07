/*
 * $Id: link.c,v 1.70 Broadcom SDK $
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
 * Hardware Linkscan module
 *
 * Hardware linkscan is available, but its use is not recommended
 * because a software linkscan task is very low overhead and much more
 * flexible.
 *
 * If hardware linkscan is used, each MII operation must temporarily
 * disable it and wait for the current scan to complete, increasing the
 * latency.  PHY status register 1 may contain clear-on-read bits that
 * will be cleared by hardware linkscan and not seen later.  Special
 * support is provided for the Serdes MAC.
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
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif
#ifdef  BCM_SBX_SUPPORT
#include <soc/sbx/sbx_drv.h>
#endif
#include <soc/defs.h>

/*
 * Function:    
 *      _soc_link_update
 * Purpose:
 *      Update the forwarding state in the chip (EPC_LINK).
 * Parameters:  
 *      unit - StrataSwitch unit #.
 * Returns:
 *      SOC_E_XXX
 * NOTE:
 * soc_link_fwd_set and soc_link_mask2_set call
 * this function to update EPC_LINK_BMAP. soc_link_fwd_set is called 
 * with LINK_LOCK and soc_link_mask2_set is called with PORT_LOCK.
 * No synchronization mechanism is implemented in this function. Therefore,
 * the user must make sure that the call to this function is synchronized 
 * between linkscan thread and calling thread. 
 */

STATIC int
_soc_link_update(int unit)
{
    pbmp_t      	pbm;
    soc_control_t	*soc = SOC_CONTROL(unit);
    soc_persist_t	*sop = SOC_PERSIST(unit);
    char        pfmtl[SOC_PBMP_FMT_LEN],
        pfmtm2[SOC_PBMP_FMT_LEN],
        pfmtp[SOC_PBMP_FMT_LEN];

    COMPILER_REFERENCE(pfmtl);
    COMPILER_REFERENCE(pfmtm2);
    COMPILER_REFERENCE(pfmtp);

    if (SOC_IS_ROBO(unit)) {
        SOC_PBMP_ASSIGN(pbm, soc->link_fwd);        
    } else {
        SOC_PBMP_ASSIGN(pbm, sop->link_fwd);
    }
    SOC_PBMP_AND(pbm, soc->link_mask2);

    if (SOC_IS_ROBO(unit)){
        LOG_VERBOSE(BSL_LS_SOC_LINK,
                    (BSL_META_U(unit,
                                "_soc_link_update: link=%s pbm=%s\n"),
                     SOC_PBMP_FMT(soc->link_fwd, pfmtl),
                     SOC_PBMP_FMT(pbm, pfmtp)));

        return SOC_E_NONE;
    }
    LOG_VERBOSE(BSL_LS_SOC_LINK,
                (BSL_META_U(unit,
                            "_soc_link_update: link=%s m2=%s pbm=%s\n"),
                 SOC_PBMP_FMT(sop->link_fwd, pfmtl),
                 SOC_PBMP_FMT(soc->link_mask2, pfmtm2),
                 SOC_PBMP_FMT(pbm, pfmtp)));

#ifdef BCM_HERCULES_SUPPORT
    if (SOC_IS_HERCULES(unit)) {
        int port;
        uint32  nlink, olink;

        nlink = SOC_PBMP_WORD_GET(pbm, 0);
        olink = -1;
        PBMP_PORT_ITER(unit, port) {
            SOC_IF_ERROR_RETURN
                (READ_ING_EPC_LNKBMAPr(unit, port, &olink));
            break;
        }

        if (nlink != olink) {
            PBMP_PORT_ITER(unit, port) {
                SOC_IF_ERROR_RETURN
                    (WRITE_ING_EPC_LNKBMAPr(unit, port, nlink));
            }
        }
        return SOC_E_NONE;
    }
#endif /* BCM_HERCULES_SUPPORT */

#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANAX(unit)) {
        epc_link_bmap_entry_t entry;
        sal_memset(&entry, 0, sizeof(entry));
        soc_mem_pbmp_field_set(unit, EPC_LINK_BMAPm, &entry, PORT_BITMAPf,
                               &pbm);
        SOC_IF_ERROR_RETURN(WRITE_EPC_LINK_BMAPm(unit, MEM_BLOCK_ALL, 0,
                                                 &entry));
        return SOC_E_NONE;
    } else if (SOC_IS_TR_VL(unit) && !SOC_IS_ENDURO(unit) && !SOC_IS_HURRICANE(unit)) {
        uint64      nlink64, olink64;

        COMPILER_64_SET(nlink64, SOC_PBMP_WORD_GET(pbm, 1),
                        SOC_PBMP_WORD_GET(pbm, 0));

        SOC_IF_ERROR_RETURN
            (soc_reg64_read_any_block(unit, EPC_LINK_BMAP_64r, &olink64));
        if (COMPILER_64_NE(nlink64, olink64)) {
            SOC_IF_ERROR_RETURN
                (soc_reg64_write_all_blocks(unit, EPC_LINK_BMAP_64r, nlink64));
        }
        return SOC_E_NONE;
    } else if(SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit)){
        uint32          olink, nlink;
        nlink = SOC_PBMP_WORD_GET(pbm, 0);

        SOC_IF_ERROR_RETURN
            (soc_reg_read_any_block(unit, EPC_LINK_BMAP_64r, &olink));
        if (nlink != olink) {
            SOC_IF_ERROR_RETURN
                (soc_reg_write_all_blocks(unit, EPC_LINK_BMAP_64r, nlink));
        }
        return SOC_E_NONE;    
    }
#endif

#ifdef BCM_ESW_SUPPORT
    if (SOC_IS_XGS_SWITCH(unit)) {
        uint32          olink, nlink;

#if defined(BCM_RAPTOR_SUPPORT)
        if (soc_feature(unit, soc_feature_register_hi)) {
            SOC_IF_ERROR_RETURN
                (WRITE_EPC_LINK_BMAP_HIr(unit, SOC_PBMP_WORD_GET(pbm, 1)));
        }
#endif /* BCM_RAPTOR_SUPPORT */

        SOC_IF_ERROR_RETURN
            (soc_reg_read_any_block(unit, EPC_LINK_BMAPr, &olink));
        nlink = SOC_PBMP_WORD_GET(pbm, 0);
        if (nlink != olink) {
            if (SOC_IS_SHADOW(unit)) {
                /* Deal with aggregated Interlaken ports */
                if (SOC_PBMP_MEMBER(pbm, 9) &&
                   !(SOC_PBMP_MEMBER(SOC_PORT_DISABLED_BITMAP(unit,all), 9))) {
                     SOC_PBMP_PORT_ADD(pbm, 10);
                     SOC_PBMP_PORT_ADD(pbm, 11);
                     SOC_PBMP_PORT_ADD(pbm, 12);
                }
                if (SOC_PBMP_MEMBER(pbm, 13) &&
                   !(SOC_PBMP_MEMBER(SOC_PORT_DISABLED_BITMAP(unit,all), 13))) {
                     SOC_PBMP_PORT_ADD(pbm, 14);
                     SOC_PBMP_PORT_ADD(pbm, 15);
                     SOC_PBMP_PORT_ADD(pbm, 16);
                }
                nlink = SOC_PBMP_WORD_GET(pbm, 0);
            }
            SOC_IF_ERROR_RETURN
                (soc_reg_write_all_blocks(unit, EPC_LINK_BMAPr, nlink));
        }
        return SOC_E_NONE; 
    }
#endif /* BCM_ESW_SUPPORT */    

    return SOC_E_NONE;  /* SOC_E_UNAVAIL? */
}

/*
 * Function:
 *      soc_link_fwd_set
 * Purpose:
 *      Sets EPC_LINK independent of chip type.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      pbmp - Value.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      EPC_LINK should be manipulated only through this routine and
 *      soc_link_maskX_set.
 */

int
soc_link_fwd_set(int unit, pbmp_t fwd)
{
    if (SOC_IS_ROBO(unit)) {
        SOC_CONTROL(unit)->link_fwd = fwd;
    } else {
        SOC_PERSIST(unit)->link_fwd = fwd;
    }

    return _soc_link_update(unit);
}

/*
 * Function:
 *      soc_link_fwd_get
 * Purpose:
 *      Gets EPC_LINK independent of chip type.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      pbmp - (OUT) Value.
 */

void
soc_link_fwd_get(int unit, pbmp_t *fwd)
{
    if (SOC_IS_ROBO(unit)) {
        *fwd = SOC_CONTROL(unit)->link_fwd;
    } else {
        *fwd = SOC_PERSIST(unit)->link_fwd;
    }
}

/*
 * Function:
 *      soc_link_mask2_set
 * Purpose:
 *      Mask bits in EPC_LINK independent of soc_link_fwd value.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      mask - Value.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      This routine is used to clear bits in the EPC_LINK to support
 *      the mac_fe/ge_enable_set() calls.
 */

int
soc_link_mask2_set(int unit, pbmp_t mask)
{
    SOC_CONTROL(unit)->link_mask2 = mask;
#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) &&
        SOC_INFO(unit).linkphy_enabled) {
        SOC_PBMP_REMOVE(SOC_CONTROL(unit)->link_mask2,
                    SOC_INFO(unit).linkphy_pbm);
        SOC_PBMP_OR(SOC_CONTROL(unit)->link_mask2,
                    SOC_INFO(unit).linkphy_pp_port_pbm);
    }
    if (soc_feature(unit, soc_feature_subtag_coe) &&
        SOC_INFO(unit).subtag_enabled) {
        SOC_PBMP_REMOVE(SOC_CONTROL(unit)->link_mask2,
                    SOC_INFO(unit).subtag_pbm);
        SOC_PBMP_OR(SOC_CONTROL(unit)->link_mask2,
                    SOC_INFO(unit).subtag_pp_port_pbm);
    }
#endif

    return _soc_link_update(unit);
}

/*
 * Function:
 *      soc_link_mask2_get
 * Purpose:
 *      Counterpart to soc_link_mask2_set
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      mask - (OUT) Value.
 */

void
soc_link_mask2_get(int unit, pbmp_t *mask)
{
    *mask = SOC_CONTROL(unit)->link_mask2;
}

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) 
/*
 * Function:
 *      _soc_link_scan_ports_write
 * Purpose:
 *      Writes the CMIC_SCAN_PORTS register(s) of the device with the
 *      provided HW linkscan port configuration.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      hw_mii_pbm - Scan ports.
 * Returns:
 *      Nothing
 * Notes:
 *      Assumes interrupt suspension handled in the calling function
 */

STATIC void
_soc_link_scan_ports_write(int unit, pbmp_t hw_mii_pbm)
{
    uint32      link_pbmp;
    soc_pbmp_t  tmp_pbmp;
    soc_port_t  phy_port, port;

    link_pbmp = SOC_PBMP_WORD_GET(hw_mii_pbm, 0); 
#if defined(BCM_GOLDWING_SUPPORT) 
    if (SOC_IS_GOLDWING(unit)) {
        /* (MSB) 15-14-19-18-17-16-13-12-11-10-9-8-7-6-5-4-3-2-1-0 (LSB) */
        link_pbmp =  (link_pbmp & 0x00003FFF) |
            ((link_pbmp & 0x000F0000) >> 2) |
            ((link_pbmp & 0x0000C000) << 4);
    }
#endif /* BCM_GOLDWING_SUPPORT */
#if defined (BCM_SCORPION_SUPPORT)
    if (SOC_IS_SC_CQ(unit)) {
        /* CMIC port not included in link status */
        link_pbmp >>=  1;
    }
#endif /* BCM_SCORPION_SUPPORT */
    if (soc_feature(unit, soc_feature_logical_port_num)) {
        SOC_PBMP_CLEAR(tmp_pbmp);
        PBMP_ITER(hw_mii_pbm, port) {
            phy_port = SOC_INFO(unit).port_l2p_mapping[port];
            if (phy_port == 0) {
                continue;
            }
            if (SOC_IS_HURRICANE2(unit) ||
                SOC_IS_GREYHOUND(unit)) {
                SOC_PBMP_PORT_ADD(tmp_pbmp, phy_port);
            } else {
                SOC_PBMP_PORT_ADD(tmp_pbmp, phy_port - 1);
            }
        }
        link_pbmp = SOC_PBMP_WORD_GET(tmp_pbmp, 0); 
    } else {
        SOC_PBMP_ASSIGN(tmp_pbmp, hw_mii_pbm);
    }
#if defined (BCM_CMICM_SUPPORT)
    if (soc_feature(unit, soc_feature_cmicm)) {
        WRITE_CMIC_MIIM_SCAN_PORTS_0r(unit, link_pbmp);
        if (SOC_REG_IS_VALID(unit, CMIC_MIIM_SCAN_PORTS_1r)) {
            WRITE_CMIC_MIIM_SCAN_PORTS_1r(unit,
                                          SOC_PBMP_WORD_GET(tmp_pbmp, 1));
        }
        if (SOC_REG_IS_VALID(unit, CMIC_MIIM_SCAN_PORTS_2r)) {
            WRITE_CMIC_MIIM_SCAN_PORTS_2r(unit,
                                          SOC_PBMP_WORD_GET(tmp_pbmp, 2));
        }
        if (SOC_REG_IS_VALID(unit, CMIC_MIIM_SCAN_PORTS_3r)) {
            WRITE_CMIC_MIIM_SCAN_PORTS_3r(unit,
                                          SOC_PBMP_WORD_GET(tmp_pbmp, 3));
        }
    } else
#endif
    {
        soc_pci_write(unit, CMIC_SCAN_PORTS, link_pbmp);

        if (((SOC_IS_TR_VL(unit) || SOC_IS_SIRIUS(unit) || SOC_IS_CALADAN3(unit))
             && !SOC_IS_ENDURO(unit) && !SOC_IS_HURRICANE(unit)) ||
            soc_feature(unit, soc_feature_register_hi)) {
            WRITE_CMIC_SCAN_PORTS_HIr
                (unit, SOC_PBMP_WORD_GET(tmp_pbmp, 1));
        }
#if defined(BCM_TRIDENT_SUPPORT)
        if (SOC_REG_IS_VALID(unit, CMIC_SCAN_PORTS_HI_2r)) {
            WRITE_CMIC_SCAN_PORTS_HI_2r
                (unit, SOC_PBMP_WORD_GET(tmp_pbmp, 2));
        }
#endif
    }    
}
#endif /* BCM_ESW_SUPPORT | BCM_SIRIUS_SUPPORT  | BCM_CALADAN3_SUPPORT*/

/*
 * Function:
 *      soc_linkscan_pause
 * Purpose:
 *      Pauses link scanning, without disabling it.
 *      This call is used to pause scanning temporarily.
 * Parameters:
 *      unit - StrataSwitch unit #.
 * Returns:
 *      Nothing
 * Notes:
 *      Nesting pauses is provided for.
 *      Software must ensure every pause is accompanied by a continue
 *      or linkscan will never resume.
 */
void
soc_linkscan_pause(int unit)
{
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)  || defined(BCM_CALADAN3_SUPPORT)
    soc_control_t	*soc = SOC_CONTROL(unit);
    int			s, stall_count;
    pbmp_t		pbm;
    uint32              schan_ctrl;

    if (SOC_IS_ESW(unit) || SOC_IS_SIRIUS(unit) || SOC_IS_CALADAN3(unit)) {
        SOC_LINKSCAN_LOCK(unit, s);    /* Manipulate flags & regs atomically */

        if (soc->soc_link_pause++ == 0 &&
            (soc->soc_flags & SOC_F_LSE)) {
            /* Stop link scan and wait for current pass to finish */

            SOC_PBMP_CLEAR(pbm);

            /* First clear HW linkscan ports */
            _soc_link_scan_ports_write(unit, pbm);
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                /* Turn off HW linkscan */
                READ_CMIC_MIIM_SCAN_CTRLr(unit,&schan_ctrl);
                soc_reg_field_set(unit, CMIC_MIIM_SCAN_CTRLr, &schan_ctrl,
                                  MIIM_LINK_SCAN_ENf, 0);
                WRITE_CMIC_MIIM_SCAN_CTRLr(unit,schan_ctrl);
                if (soc_feature(unit, soc_feature_linkscan_pause_timeout)) {
                     soc_timeout_t to;
                    /* Wait for Linkscan stopped signal */
                    soc_timeout_init(&to, 1000000 /*1 sec*/, 100);  
                    while (soc_pci_read(unit, CMIC_MIIM_SCAN_STATUS_OFFSET) &
                           CMIC_MIIM_SCAN_BUSY) {
                        if (soc_timeout_check(&to)) {
                           LOG_ERROR(BSL_LS_SOC_COMMON,
                                     (BSL_META_U(unit,
                                                 "soc_linkscan_pause: pausing hw linkscan failed\n")));
                           break;
                        }
                    }
                } else {
	                /* Wait for Linkscan stopped signal */
	                while (soc_pci_read(unit, CMIC_MIIM_SCAN_STATUS_OFFSET) &
	                       CMIC_MIIM_SCAN_BUSY) {
	                    /* Nothing */
	                }
				}
				
            } else
#endif
            {
                /* Turn off HW linkscan */
                soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_LINK_SCAN_EN_CLR);

                if (soc_feature(unit, soc_feature_linkscan_pause_timeout)) {
                    soc_timeout_t to;
                    /* Wait for Linkscan stopped signal */
                    soc_timeout_init(&to, 1000000 /*1 sec*/, 100);  
                    while (soc_pci_read(unit, CMIC_SCHAN_CTRL) &
                           SC_MIIM_SCAN_BUSY_TST) {
                        if (soc_timeout_check(&to)) {
                            LOG_ERROR(BSL_LS_SOC_COMMON,
                                      (BSL_META_U(unit,
                                                  "soc_linkscan_pause: pausing hw linkscan failed\n")));
                            break;
                        }
                    }
                } else {
	            /* Wait for Linkscan stopped signal */
	            while (soc_pci_read(unit, CMIC_SCHAN_CTRL) &
	                   SC_MIIM_SCAN_BUSY_TST) {
	                /* Nothing */
	            }
                }
            }

            COMPILER_REFERENCE(schan_ctrl);
            /* Wait > 1us for last HW linkscan operation to complete. */
            for (stall_count = 0; stall_count < 4; stall_count++) {
                /* We're using this PCI operation to pass some time
                 * since we can't use sal_usleep safely with the
                 * interrupts suspended.  We only record the read value
                 * to prevent any complaint about an uninspected return
                 * value.
                 */
#ifdef BCM_CMICM_SUPPORT
                if(soc_feature(unit, soc_feature_cmicm)) {
                    schan_ctrl = soc_pci_read(unit, CMIC_MIIM_SCAN_STATUS_OFFSET);
                } else
#endif
                {
                    schan_ctrl = soc_pci_read(unit, CMIC_SCHAN_CTRL);
                }
            }
        }

        SOC_LINKSCAN_UNLOCK(unit, s);
    }
#endif /* BCM_ESW_SUPPORT | BCM_SIRIUS_SUPPORT | BCM_CALADAN3_SUPPORT */
}

/*
 * Function:
 *      soc_linkscan_continue
 * Purpose:
 *      Continue link scanning after it has been paused.
 * Parameters:
 *      unit - StrataSwitch unit #.
 * Returns:
 *      Nothing
 * Notes:
 *      This routine is designed so if soc_linkscan_config is called,
 *      it won't be confused whether or not a pause is in effect.
 */

void
soc_linkscan_continue(int unit)
{
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) 
    soc_control_t	*soc = SOC_CONTROL(unit);
    int			s;
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
    uint32 schan_ctrl;
#endif

    if (SOC_IS_ESW(unit) || SOC_IS_SIRIUS(unit) || SOC_IS_CALADAN3(unit)) {
        SOC_LINKSCAN_LOCK(unit, s);    /* Manipulate flags & regs atomically */

        if (soc->soc_link_pause <= 0) {
            SOC_LINKSCAN_UNLOCK(unit, s);
            assert(0);      /* Continue not preceded by a pause */
        }

        if (--soc->soc_link_pause == 0 &&
            (soc->soc_flags & SOC_F_LSE)) {
            /*
             * NOTE: whenever hardware linkscan is running, the PHY_REG_ADDR
             * field of the MIIM_PARAM register must be set to 1 (PHY Link
             * Status register address).
             */
#ifdef BCM_CMICM_SUPPORT
            if(soc_feature(unit, soc_feature_cmicm)) {
                if (soc_feature(unit, soc_feature_phy_cl45))  {
                    /*
                    ** Clause 22 Register 0x01 (MII_STAT) for FE/GE.
                    ** Clause 45 Register 0x01 (MII_STAT) Devad = 0x1 (PMA_PMD) 
                    ** for XE.
                    */
                    uint32 phy_miim_addr = 0;
                    soc_reg_field_set(unit, CMIC_CMC0_MIIM_ADDRESSr, &phy_miim_addr,
                                  CLAUSE_45_DTYPEf, 0x01);
                    soc_reg_field_set(unit, CMIC_CMC0_MIIM_ADDRESSr, &phy_miim_addr,
                                  CLAUSE_45_REGADRf, 0x01);
                    /* To set 0x1 to CLAUSE_22_REGADRf is not required.
                     *  - in all CMICM device, the CLAUSE_22_REGADRf in register 
                     * CMIC_CMCx_MIIM_ADDRESSr is the field overlay from bit0 to
                     *    bit4 of CLAUSE_45_REGADRf.
                     */
                    if (SOC_REG_FIELD_VALID(unit, CMIC_CMC0_MIIM_ADDRESSr, 
                            CLAUSE_22_REGADRf)) {
                        soc_reg_field_set(unit, CMIC_CMC0_MIIM_ADDRESSr, 
                                &phy_miim_addr, CLAUSE_22_REGADRf, 0x01);
                    }
                    soc_pci_write(unit, CMIC_CMCx_MIIM_ADDRESS_OFFSET(cmc), phy_miim_addr);
                } else {
                    soc_pci_write(unit, CMIC_CMCx_MIIM_PARAM_OFFSET(cmc), (uint32) 0x01 << 24);
                }
                _soc_link_scan_ports_write(unit, soc->hw_linkscan_pbmp);
                READ_CMIC_MIIM_SCAN_CTRLr(unit,&schan_ctrl);
                soc_reg_field_set(unit, CMIC_MIIM_SCAN_CTRLr, &schan_ctrl,
                                                         MIIM_LINK_SCAN_ENf, 1);
                WRITE_CMIC_MIIM_SCAN_CTRLr(unit,schan_ctrl);
            } else
#endif
            {
                if (soc_feature(unit, soc_feature_phy_cl45))  {
                    /*
                    ** Clause 22 Register 0x01 (MII_STAT) for FE/GE.
                    ** Clause 45 Register 0x01 (MII_STAT) Devad = 0x1 (PMA_PMD) 
                    ** for XE.
                    */
                    uint32 phy_miim_addr = 0;
                    soc_reg_field_set(unit, CMIC_MIIM_ADDRESSr, &phy_miim_addr,
                                  CLAUSE_45_DTYPEf, 0x01);
                    soc_reg_field_set(unit, CMIC_MIIM_ADDRESSr, &phy_miim_addr,
                                  CLAUSE_45_REGADRf, 0x01);
                    soc_reg_field_set(unit, CMIC_MIIM_ADDRESSr, &phy_miim_addr,
                                  CLAUSE_22_REGADRf, 0x01);
                    WRITE_CMIC_MIIM_ADDRESSr(unit, phy_miim_addr);
                } else {
                    soc_pci_write(unit, CMIC_MIIM_PARAM, (uint32) 0x01 << 24);
                }
                _soc_link_scan_ports_write(unit, soc->hw_linkscan_pbmp);
                soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_LINK_SCAN_EN_SET);
            }
        }

        SOC_LINKSCAN_UNLOCK(unit, s);
    }
#endif /* BCM_ESW_SUPPORT | BCM_SIRIUS_SUPPORT | BCM_CALADAN3_SUPPORT */
}

/*
 * Function:
 *      soc_linkscan_register
 * Purpose:
 *      Provide a callout made when CMIC link scanning detects a link change.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      f    - Function called when link status change is detected.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Handler called in interrupt context.
 */

int
soc_linkscan_register(int unit, void (*f)(int))
{
    soc_control_t	*soc = SOC_CONTROL(unit);

    if (f != NULL && soc->soc_link_callout != NULL) {
        return SOC_E_EXISTS;
    }

    soc->soc_link_callout = f;

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_linkscan_config
 * Purpose:
 *      Set ports to scan in CMIC.
 * Parameters:
 *      unit - StrataSwich Unit #
 *      mii_pbm - Port bit map of ports to scan with MIIM registers
 *      direct_pbm - Port bit map of ports to scan using NON MII.
 * Returns:
 *      SOC_E_XXX
 */

int
soc_linkscan_config(int unit, pbmp_t hw_mii_pbm, pbmp_t hw_direct_pbm)
{
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) 
    soc_control_t	*soc = SOC_CONTROL(unit);
    uint32		cmic_config;
    int			s, has_mge, has_dge;
    pbmp_t		pbm;

    if (SOC_IS_ROBO(unit)) {
        return SOC_E_NONE;
    }

    SOC_PBMP_ASSIGN(pbm, hw_mii_pbm);
    SOC_PBMP_AND(pbm, hw_direct_pbm);
    assert(SOC_PBMP_IS_NULL(pbm));      /* !(hw_mii_pbm & hw_direct_pbm) */

    /*
     * Hardware (direct) scanning is NOT supported on 10/100 ports.
     */
    SOC_PBMP_ASSIGN(pbm, hw_direct_pbm);
    SOC_PBMP_AND(pbm, PBMP_FE_ALL(unit));
    if (SOC_PBMP_NOT_NULL(pbm)) {
        return SOC_E_UNAVAIL;
    }

    /*
     * The LINK_SCAN_GIG control affects ALL ports. Thus, all ports
     * being scanned by H/W must be either MIIM scanned or scanned
     * using the direct connection.
     */
    SOC_PBMP_ASSIGN(pbm, PBMP_GE_ALL(unit));
    SOC_PBMP_AND(pbm, hw_mii_pbm);
    has_mge = SOC_PBMP_NOT_NULL(pbm);
    SOC_PBMP_ASSIGN(pbm, PBMP_GE_ALL(unit));
    SOC_PBMP_AND(pbm, hw_direct_pbm);
    has_dge = SOC_PBMP_NOT_NULL(pbm);
    if (has_mge && has_dge) {
        return SOC_E_UNAVAIL;
    }

    /*
     * soc_linkscan_pause/continue combination will result in the
     * registers being setup and started properly if we are enabling for
     * the first time.
     */

    SOC_LINKSCAN_LOCK(unit, s);

    soc_linkscan_pause(unit);

    /* Check if disabling port scanning */

    SOC_PBMP_ASSIGN(pbm, hw_mii_pbm);
    SOC_PBMP_OR(pbm, hw_direct_pbm);
    if (SOC_PBMP_NOT_NULL(pbm)) {
        /*
         * NOTE: we are no longer using CC_LINK_STAT_EN since it is
         * unavailable on 5695 and 5665.  EPC_LINK will be updated by
         * software anyway, it will just take a few extra milliseconds.
         */
        soc->soc_flags |= SOC_F_LSE;
    } else {
        soc->soc_flags &= ~SOC_F_LSE;
    }

    if (soc_reg_field_valid(unit, CMIC_CONFIGr, LINK_STAT_ENf)) {
        
        cmic_config = soc_pci_read(unit, CMIC_CONFIG);
        soc_reg_field_set(unit, CMIC_CONFIGr, &cmic_config, LINK_STAT_ENf,
                          SOC_PBMP_NOT_NULL(pbm) ? 1 : 0);
        soc_pci_write(unit, CMIC_CONFIG, cmic_config);
    }

    /* The write of the HW linkscan ports is moved to the linkscan
     * continue below.  Note that though the continue function
     * will not write to the CMIC scan ports register if linkscan
     * was disabled above, that is only the case when the port bitmap
     * is empty.  Since linkscan pause clears the bitmap, this is the
     * desired result.
     */
    SOC_PBMP_ASSIGN(soc->hw_linkscan_pbmp, hw_mii_pbm);

    soc_linkscan_continue(unit);

    SOC_LINKSCAN_UNLOCK(unit, s);

#endif /* BCM_ESW_SUPPORT | BCM_SIRIUS_SUPPORT | BCM_CALADAN3_SUPPORT */
    return SOC_E_NONE;
}

#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
STATIC int
_soc_linkscan_hw_port_init(int unit, soc_port_t port)
{
    STATIC const soc_reg_t protocol_map_reg[] = {
        CMIC_MIIM_PROTOCOL_MAPr,
        CMIC_MIIM_PROTOCOL_MAP_HIr,
        CMIC_MIIM_PROTOCOL_MAP_HI_2r
    };
#if defined (BCM_CMICM_SUPPORT)
    STATIC const soc_reg_t protocol_map_reg_cmicm[] = {
        CMIC_MIIM_PROTOCOL_MAP_0r,
        CMIC_MIIM_PROTOCOL_MAP_1r,
        CMIC_MIIM_PROTOCOL_MAP_2r,
        CMIC_MIIM_PROTOCOL_MAP_3r
    };
#endif
    STATIC const soc_reg_t *protocol_map = protocol_map_reg;
    STATIC const soc_reg_t int_sel_map_reg[] = {
        CMIC_MIIM_INT_SEL_MAPr,
        CMIC_MIIM_INT_SEL_MAP_HIr,
        CMIC_MIIM_INT_SEL_MAP_HI_2r
    };
#if defined (BCM_CMICM_SUPPORT)
    STATIC const soc_reg_t int_sel_map_reg_cmicm[] = {
        CMIC_MIIM_INT_SEL_MAP_0r,
        CMIC_MIIM_INT_SEL_MAP_1r,
        CMIC_MIIM_INT_SEL_MAP_2r,
        CMIC_MIIM_INT_SEL_MAP_3r
    };
#endif
    STATIC const soc_reg_t *int_sel_map = int_sel_map_reg;
    STATIC const soc_reg_t port_type_map_reg[] = {
        CMIC_MIIM_PORT_TYPE_MAPr,
        CMIC_MIIM_PORT_TYPE_MAP_HIr
    };
    STATIC const soc_reg_t port_type_map_bus2_reg[] = {
        CMIC_MIIM_PORT_TYPE_MAP_BUS2r,
        CMIC_MIIM_PORT_TYPE_MAP_BUS2_HIr
    };
    STATIC const soc_reg_t bus_map_reg[] = {
        CMIC_MIIM_BUS_MAP_9_0r, CMIC_MIIM_BUS_MAP_19_10r,
        CMIC_MIIM_BUS_MAP_29_20r, CMIC_MIIM_BUS_MAP_39_30r,
        CMIC_MIIM_BUS_MAP_49_40r, CMIC_MIIM_BUS_MAP_59_50r,
        CMIC_MIIM_BUS_MAP_69_60r, CMIC_MIIM_BUS_MAP_79_70r
    };
#if defined (BCM_CMICM_SUPPORT)
    STATIC const soc_reg_t bus_map_reg_cmicm[] = {
        CMIC_MIIM_BUS_SEL_MAP_9_0r, CMIC_MIIM_BUS_SEL_MAP_19_10r,
        CMIC_MIIM_BUS_SEL_MAP_29_20r, CMIC_MIIM_BUS_SEL_MAP_39_30r,
        CMIC_MIIM_BUS_SEL_MAP_49_40r, CMIC_MIIM_BUS_SEL_MAP_59_50r,
        CMIC_MIIM_BUS_SEL_MAP_69_60r, CMIC_MIIM_BUS_SEL_MAP_79_70r,
        CMIC_MIIM_BUS_SEL_MAP_89_80r, CMIC_MIIM_BUS_SEL_MAP_99_90r,
        CMIC_MIIM_BUS_SEL_MAP_109_100r, CMIC_MIIM_BUS_SEL_MAP_119_110r,
        CMIC_MIIM_BUS_SEL_MAP_127_120r
    };
#endif
    STATIC const soc_reg_t *bus_map = bus_map_reg;
    STATIC const soc_field_t bus_map_9_0_field[] = {
        PORT_0_BUS_NUMf, PORT_1_BUS_NUMf, PORT_2_BUS_NUMf, PORT_3_BUS_NUMf,
        PORT_4_BUS_NUMf, PORT_5_BUS_NUMf, PORT_6_BUS_NUMf, PORT_7_BUS_NUMf,
        PORT_8_BUS_NUMf, PORT_9_BUS_NUMf
    };
    STATIC const soc_reg_t phy_map_reg[] = {
        CMIC_MIIM_EXT_PHY_ADDR_MAP_3_0r, CMIC_MIIM_EXT_PHY_ADDR_MAP_7_4r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_11_8r, CMIC_MIIM_EXT_PHY_ADDR_MAP_15_12r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_19_16r, CMIC_MIIM_EXT_PHY_ADDR_MAP_23_20r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_27_24r, CMIC_MIIM_EXT_PHY_ADDR_MAP_31_28r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_35_32r, CMIC_MIIM_EXT_PHY_ADDR_MAP_39_36r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_43_40r, CMIC_MIIM_EXT_PHY_ADDR_MAP_47_44r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_51_48r, CMIC_MIIM_EXT_PHY_ADDR_MAP_55_52r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_59_56r, CMIC_MIIM_EXT_PHY_ADDR_MAP_63_60r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_67_64r, CMIC_MIIM_EXT_PHY_ADDR_MAP_71_68r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_75_72r, CMIC_MIIM_EXT_PHY_ADDR_MAP_79_76r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_83_80r, CMIC_MIIM_EXT_PHY_ADDR_MAP_87_84r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_91_88r, CMIC_MIIM_EXT_PHY_ADDR_MAP_95_92r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_99_96r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_103_100r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_107_104r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_111_108r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_115_112r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_119_116r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_123_120r,
        CMIC_MIIM_EXT_PHY_ADDR_MAP_127_124r
    };
    STATIC const soc_field_t phy_map_3_0_field[] = {
        PHY_ID_0f, PHY_ID_1f, PHY_ID_2f, PHY_ID_3f
    };
    soc_reg_t reg;
    soc_field_t field;
    int addr;
    uint32 rval;
    int bus_sel;
    soc_port_t phy_port, port_bit;
    int embedded_phy_port = 0;

    if (soc_feature(unit, soc_feature_logical_port_num)) {
        phy_port = SOC_INFO(unit).port_l2p_mapping[port];
    } else {
        phy_port = port;
    }

    port_bit = phy_port;
#ifdef BCM_SCORPION_SUPPORT
    if (SOC_IS_SC_CQ(unit) || SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_KATANAX(unit)) {
        /*
         * SC/CQ devices omit the CMIC port from the PHY counting.  All
         * "port bitmaps" are shifted by 1.
         */
        if (port == CMIC_PORT(unit)) {
            /* This should not be triggered */
            return SOC_E_PORT;
        }
        if (!SOC_IS_KATANAX(unit)) {
            port_bit -= 1;
        }
    }
#endif

#if defined (BCM_CMICM_SUPPORT)
    if (soc_feature(unit, soc_feature_cmicm)) {
        protocol_map = protocol_map_reg_cmicm;
        int_sel_map = int_sel_map_reg_cmicm;
        bus_map = bus_map_reg_cmicm;
    }
#endif
    /*
     * Check If Hardware Linkscan should use Clause 45 mode
     */
    if ((IS_XE_PORT(unit, port) || IS_CE_PORT(unit, port) || 
         IS_HG_PORT(unit, port)) && PHY_CLAUSE45_MODE(unit, port)) {
        reg = protocol_map[port_bit / 32];
        if (SOC_REG_IS_VALID(unit, reg)) {
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            rval = soc_pci_read(unit, addr);
            rval |= 1 << (port_bit % 32);
            soc_pci_write(unit, addr, rval);
        }
    }

    /*
     *  Select the appropriate MDIO bus
     */
    if (SOC_IS_TD_TT(unit)) {
        bus_sel = ((PHY_ADDR(unit, port) & 0x100) >> 6) |
            ((PHY_ADDR(unit, port) & 0x60) >> 5) ;
    } else if (SOC_IS_TRX(unit)) {
        bus_sel = (PHY_ADDR(unit, port) & 0x60) >> 5; /* bus 0, 1, and 2 */
    } else {
        bus_sel = (PHY_ADDR(unit, port) & 0x40) >> 6; /* bus 0 and 1 */
    }

    if (SOC_REG_IS_VALID(unit, bus_map[0])) {
        reg = bus_map[port_bit / 10];
        field = bus_map_9_0_field[port_bit % 10];
        if (SOC_REG_IS_VALID(unit, reg)) {
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            rval = soc_pci_read(unit, addr);
            soc_reg_field_set(unit, bus_map[0], &rval, field, bus_sel);
            soc_pci_write(unit, addr, rval);
        }
    } else {
        reg = INVALIDr;
        if (bus_sel == 1) {
            reg = port_type_map_reg[port_bit / 32];
        } else if ((bus_sel == 2) && !SOC_IS_SHADOW(unit)) {
            reg = port_type_map_bus2_reg[port_bit / 32];
        }
        if (SOC_REG_IS_VALID(unit, reg)) {
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            rval = soc_pci_read(unit, addr);
            rval |= 1 << (port_bit % 32);
            soc_pci_write(unit, addr, rval);
        }
    }

    /*
     * Check If Hardware Linkscan should use internal phy
     */
    embedded_phy_port = 0;
    if (SOC_IS_HURRICANE2(unit)) {
        if (soc_feature(unit, soc_feature_gphy) && (phy_port < 18)) {
            embedded_phy_port = 1;
        }
    }
    if (!PHY_EXTERNAL_MODE(unit, port) || embedded_phy_port) {
        reg = int_sel_map[port_bit / 32];
        if (SOC_REG_IS_VALID(unit, reg)) {
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            rval = soc_pci_read(unit, addr);
            rval |= 1 << (port_bit % 32);
            soc_pci_write(unit, addr, rval);
        }
    }

    /* Re-initialize the phy port map for the unit */
    /* Use MDIO address re-mapping for hardware linkscan */
    assert(port_bit >= 0);
    assert(port_bit / 4 < sizeof(phy_map_reg) / sizeof(phy_map_reg[0]));

    /*
     * COVERITY
     *
     * assert validates the input
     */
    /* coverity[overrun-local : FALSE] */
    reg = phy_map_reg[port_bit / 4];
    field = phy_map_3_0_field[port_bit % 4];
    addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
    rval = soc_pci_read(unit, addr);
    soc_reg_field_set(unit, phy_map_reg[0], &rval, field,
                      PHY_ADDR(unit, port) & 0x1f);
    soc_pci_write(unit, addr, rval);

#if defined (BCM_CMICM_SUPPORT)
    if (soc_feature(unit, soc_feature_cmicm)) {
        READ_CMIC_MIIM_SCAN_CTRLr(unit, &rval);
        soc_reg_field_set(unit, CMIC_MIIM_SCAN_CTRLr, &rval, 
                          MIIM_ADDR_MAP_ENABLEf, 1);
        WRITE_CMIC_MIIM_SCAN_CTRLr(unit, rval);
    } else
#endif
    {
        READ_CMIC_CONFIGr(unit, &rval);
        soc_reg_field_set(unit, CMIC_CONFIGr, &rval, MIIM_ADDR_MAP_ENABLEf, 1);
        WRITE_CMIC_CONFIGr(unit, rval);
    }

    /* configure remote/local faults to trigger link interrupt */
    if (SOC_REG_IS_VALID(unit, XLPORT_FAULT_LINK_STATUSr)) {
        int blk;
        int blktype;
        uint32 rval;
        int i;

        for (i = 0; i < SOC_DRIVER(unit)->port_num_blktype; i++) {
            blk = SOC_PORT_IDX_BLOCK(unit, phy_port, i);
            blktype = SOC_BLOCK_INFO(unit, blk).type;
            if (blktype == SOC_BLK_XLPORT) {  
                rval = 0;
  
                soc_reg_field_set(unit, XLPORT_FAULT_LINK_STATUSr, &rval, 
                      REMOTE_FAULTf, 1);
                soc_reg_field_set(unit, XLPORT_FAULT_LINK_STATUSr, &rval, 
                      LOCAL_FAULTf, 0);
                SOC_IF_ERROR_RETURN(WRITE_XLPORT_FAULT_LINK_STATUSr(unit, 
                               port, rval));
            }
        }
    }
   
    return SOC_E_NONE;
}
#endif /* BCM_XGS3_SWITCH_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_CALADAN3_SUPPORT */


#if defined (BCM_ESW_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
STATIC int
_soc_linkscan_hw_link_get(int unit, soc_pbmp_t *hw_link)
{
    uint32              link_stat;
    uint32              link_pbmp;
    soc_pbmp_t          tmp_pbmp;
    soc_port_t          port_bit, phy_port, port;
    int                 num_phy_port = 0;

    if (NULL == hw_link) {
        return SOC_E_PARAM;
    }

    SOC_PBMP_CLEAR(tmp_pbmp);
     /*
      * Read CMIC link status to determine which ports that
      * actually need to be scanned.
      */
#if defined (BCM_CMICM_SUPPORT)
    if (soc_feature(unit, soc_feature_cmicm)) {
        SOC_IF_ERROR_RETURN
            (READ_CMIC_MIIM_LINK_STATUS_0r(unit, &link_stat));
    } else
#endif
        {
            SOC_IF_ERROR_RETURN
                (READ_CMIC_LINK_STATr(unit, &link_stat));
        }
#if defined(BCM_FIREBOLT_SUPPORT)
    if (soc_feature(unit, soc_feature_status_link_fail)) {
        uint32  intsel_reg;

        SOC_IF_ERROR_RETURN
            (READ_CMIC_MIIM_INT_SEL_MAPr(unit, &intsel_reg));
        link_stat ^= intsel_reg;
    }
#endif /* BCM_FIREBOLT_SUPPORT */

#if defined (BCM_CMICM_SUPPORT)
    if (soc_feature(unit, soc_feature_cmicm)) {
        link_pbmp = soc_reg_field_get(unit, CMIC_MIIM_LINK_STATUS_0r,
                                      link_stat, PORT_BITMAPf);
    } else
#endif
        {
            link_pbmp = soc_reg_field_get(unit, CMIC_LINK_STATr,
                                          link_stat, PORT_BITMAPf);
        }
#if defined (BCM_GOLDWING_SUPPORT)
    if (SOC_IS_GOLDWING(unit)) {
        /* (MSB) 15-14-19-18-17-16-13-12-11-10-9-8-7-6-5-4-3-2-1-0 (LSB) */
        link_pbmp =  (link_pbmp & 0x00003FFF) |
            ((link_pbmp & 0x000C0000) >> 4) |
            ((link_pbmp & 0x0003C000) << 2);
    }
#endif /* BCM_GOLDWING_SUPPORT */

#if defined (BCM_SCORPION_SUPPORT)
    if (SOC_IS_SC_CQ(unit)) {
        /* CMIC port not included in link status */
        link_pbmp <<=  1;
    }
#endif /* BCM_SCORPION_SUPPORT */

    SOC_PBMP_WORD_SET(tmp_pbmp, 0, link_pbmp);

#if defined (BCM_RAPTOR_SUPPORT) || defined (BCM_TRIUMPH_SUPPORT) || defined (BCM_KATANA_SUPPORT)
    /* Check for more than 32 ports per unit */
#if defined (BCM_CMICM_SUPPORT)
    if (soc_feature(unit, soc_feature_cmicm) &&
        SOC_REG_IS_VALID(unit, CMIC_MIIM_LINK_STATUS_1r) ) {
        SOC_IF_ERROR_RETURN
            (READ_CMIC_MIIM_LINK_STATUS_1r(unit, &link_stat));
        SOC_PBMP_WORD_SET(tmp_pbmp, 1,
                          soc_reg_field_get(unit, CMIC_MIIM_LINK_STATUS_1r,
                                            link_stat, PORT_BITMAPf));
    } else
#endif
        if ((SOC_IS_TR_VL(unit) && !SOC_IS_ENDURO(unit) && !SOC_IS_HURRICANE(unit)) ||
            soc_feature(unit, soc_feature_register_hi)) {
            SOC_IF_ERROR_RETURN
                (READ_CMIC_LINK_STAT_HIr(unit, &link_stat));
            SOC_PBMP_WORD_SET(tmp_pbmp, 1,
                              soc_reg_field_get(unit, CMIC_LINK_STAT_HIr,
                                                link_stat, PORT_BITMAPf));
        }
#endif
#ifdef BCM_TRIDENT_SUPPORT
#if defined (BCM_CMICM_SUPPORT)
    if (soc_feature(unit, soc_feature_cmicm) &&
        SOC_REG_IS_VALID(unit, CMIC_MIIM_LINK_STATUS_2r) ) {
        SOC_IF_ERROR_RETURN(READ_CMIC_MIIM_LINK_STATUS_2r(unit, &link_stat));
        link_pbmp = soc_reg_field_get(unit, CMIC_MIIM_LINK_STATUS_2r,
                                      link_stat, PORT_BITMAPf);
        SOC_PBMP_WORD_SET(tmp_pbmp, 2, link_pbmp);
    } else
#endif
    {
        if (SOC_REG_IS_VALID(unit, CMIC_LINK_STAT_HI_2r)) {
            SOC_IF_ERROR_RETURN(READ_CMIC_LINK_STAT_HI_2r(unit, &link_stat));
            link_pbmp = soc_reg_field_get(unit, CMIC_LINK_STAT_HI_2r,
                                          link_stat, PORT_BITMAPf);
            SOC_PBMP_WORD_SET(tmp_pbmp, 2, link_pbmp);
        }
    }  
#endif /* BCM_TRIDENT_SUPPORT */

    if (soc_feature(unit, soc_feature_cmicm) &&
        SOC_REG_IS_VALID(unit, CMIC_MIIM_LINK_STATUS_3r)) {
        SOC_IF_ERROR_RETURN(READ_CMIC_MIIM_LINK_STATUS_3r(unit, &link_stat));
        link_pbmp = soc_reg_field_get(unit, CMIC_MIIM_LINK_STATUS_3r,
                                      link_stat, PORT_BITMAPf);
        SOC_PBMP_WORD_SET(tmp_pbmp, 3, link_pbmp);
    }

    if (soc_feature(unit, soc_feature_logical_port_num)) {
        num_phy_port = soc_mem_index_count
            (unit, ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm);
                                           
        SOC_PBMP_CLEAR(*hw_link);
        PBMP_ITER(tmp_pbmp, port_bit) {
            if (SOC_IS_HURRICANE2(unit) ||
                SOC_IS_GREYHOUND(unit)) {
                phy_port = port_bit;
            } else {
                phy_port = port_bit + 1;
            }
            if (phy_port >= num_phy_port) {
                break;
            }
            /*
             * COVERITY
             *
             * The check above will protect any real cases so the
             * index does not exceed the declared size.
             */
            /* coverity[overrun-local] */
            port = SOC_INFO(unit).port_p2l_mapping[phy_port];
            if (port != -1) {
                SOC_PBMP_PORT_ADD(*hw_link, port);
            }
        }
    } else {
        SOC_PBMP_ASSIGN(*hw_link, tmp_pbmp);
    }

    return SOC_E_NONE;
}
#endif /* BCM_ESW_SUPPORT || BCM_CALADAN3_SUPPORT*/

int
soc_linkscan_hw_init(int unit)
{
#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
#ifdef BCM_XGS3_SWITCH_SUPPORT
    int automedium = 0;
    int fiber_preferred = 0;
    uint32 rval = 0;
#endif

    if (SOC_IS_XGS3_SWITCH(unit) || SOC_IS_SIRIUS(unit) || SOC_IS_CALADAN3(unit)) {
        soc_port_t port;
        PBMP_ITER(PBMP_PORT_ALL(unit), port) {
            /*
             * COVERITY
             *
             * The iterator above will protect any real cases so the
             * port index does not exceed the declared size.
             */
            /* coverity[overrun-call] */
            SOC_IF_ERROR_RETURN
                (_soc_linkscan_hw_port_init(unit, port));
        }
#ifdef BCM_XGS3_SWITCH_SUPPORT
        PBMP_ITER(PBMP_PORT_ALL(unit), port) {
            automedium |= soc_property_port_get(unit, port,
                                    spn_PHY_AUTOMEDIUM, 0);
            fiber_preferred |= soc_property_port_get(unit, port,
                                         spn_PHY_FIBER_PREF, 0);
        }
        if ((!automedium) && (fiber_preferred == 1)) {
            /*
             * Setting the Auto Scan Address for HW linkscan
             * Bit 2 of 0x01 register (MII_STATUS for Cu and Fiber)
             */
            if (soc_feature(unit, soc_feature_cmicm)) {
                READ_CMIC_MIIM_AUTO_SCAN_ADDRESSr(unit, &rval);
                soc_reg_field_set(unit, CMIC_MIIM_AUTO_SCAN_ADDRESSr, &rval,
                                       MIIM_LINK_STATUS_BIT_POSITIONf, 0x2);
                soc_reg_field_set(unit, CMIC_MIIM_AUTO_SCAN_ADDRESSr, &rval,
                                                    CLAUSE_22_REGADRf, 0x1);
                WRITE_CMIC_MIIM_AUTO_SCAN_ADDRESSr(unit, rval);
            } else if (SOC_IS_HAWKEYE(unit) || (SOC_IS_TR_VL(unit) && 
                                          !SOC_IS_GREYHOUND(unit))) {
                READ_CMIC_MIIM_AUTO_SCAN_ADDRESSr(unit, &rval);
                soc_reg_field_set(unit, CMIC_MIIM_AUTO_SCAN_ADDRESSr, &rval,
                                       MIIM_LINK_STATUS_BIT_POSITIONf, 0x2);
                soc_reg_field_set(unit, CMIC_MIIM_AUTO_SCAN_ADDRESSr, &rval,
                                                 MIIM_DEVICE_ADDRESSf, 0x1);
                WRITE_CMIC_MIIM_AUTO_SCAN_ADDRESSr(unit, rval);
            } else {
                if (SOC_REG_IS_VALID(unit, CMIC_MIIM_ADDRESSr)) {
                    if (soc_reg_field_valid(unit, CMIC_MIIM_AUTO_SCAN_ADDRESSr, 
                                            CLAUSE_22_REGADRf)) {
                        READ_CMIC_MIIM_ADDRESSr(unit, &rval);
                        soc_reg_field_set(unit, CMIC_MIIM_AUTO_SCAN_ADDRESSr,
                                          &rval, CLAUSE_22_REGADRf, 0x1);
                        WRITE_CMIC_MIIM_ADDRESSr(unit, rval);
                    }
                }
            } 
        }
#endif
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT | BCM_SIRIUS_SUPPORT | BCM_CALADAN3_SUPPORT*/

    return SOC_E_NONE;
}

int
soc_linkscan_hw_link_get(int unit, soc_pbmp_t *hw_link)
{
    int rv;

    rv = SOC_E_UNAVAIL;

    

#if defined (BCM_ESW_SUPPORT) || defined (BCM_CALADAN3_SUPPORT)
    rv = _soc_linkscan_hw_link_get(unit, hw_link);
#endif /* BCM_ESW_SUPPORT */ 

    return rv;
}

