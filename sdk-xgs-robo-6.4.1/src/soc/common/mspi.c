/*
 * $Id: mspi.c,v 1.17 Broadcom SDK $
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
 * File:        mspi.c
 * Purpose:     SPI Master mode Implementation
 * Requires:    
 */

#include <shared/bsl.h>

#include <sal/core/boot.h>
#include <sal/core/libc.h>
#include <shared/alloc.h>

#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/mspi.h>
#include <soc/cmicm.h>

#ifdef BCM_CMICM_SUPPORT

int soc_mspi_init(int unit) {
    uint32 rval;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    uint32 fval;
#endif /* BCM_KATANA_SUPPORT || BCM_TRIUMPH3_SUPPORT || BCM_TRIDENT2_SUPPORT */

    if (!soc_feature(unit, soc_feature_cmicm)) {
        return SOC_E_FAIL;
    }

    /* Claim the SPI bus */
    READ_CMIC_OVERRIDE_STRAPr(unit, &rval);
    soc_reg_field_set(unit, CMIC_OVERRIDE_STRAPr, &rval, ENABLE_OVERRIDE_SPI_MASTER_SLAVE_MODEf, 1);
    soc_reg_field_set(unit, CMIC_OVERRIDE_STRAPr, &rval, SPI_MASTER_SLAVE_MODEf, 1);
    WRITE_CMIC_OVERRIDE_STRAPr(unit, rval);

    READ_CMICM_BSPI_MAST_N_BOOTr(unit, &rval);
    soc_reg_field_set(unit, CMICM_BSPI_MAST_N_BOOTr, &rval, MAST_N_BOOTf, 1);
    WRITE_CMICM_BSPI_MAST_N_BOOTr(unit, rval);

    /* 
     * Set speed and transfer size
     */
    READ_MSPI_SPCR0_LSBr(unit, &rval);
    soc_reg_field_set(unit, MSPI_SPCR0_LSBr, &rval, SPBRf, 0x08);
    WRITE_MSPI_SPCR0_LSBr(unit, rval);

#if 0
    READ_MSPI_SPCR0_MSBr(unit, &rval);
    rval |= 0x08 << 2; /* Bits per transfer - BITS = 0x08 */
    WRITE_MSPI_SPCR0_MSBr(unit, rval);
#endif

    READ_MSPI_SPCR1_LSBr(unit, &rval);
    rval |= 0x01;  /* Delay after transfer- DTL = 0x01 */
    WRITE_MSPI_SPCR1_LSBr(unit, rval);

    READ_MSPI_SPCR1_MSBr(unit, &rval);
    rval |= 0x01;  /* DSCLK = 0x01 */
    WRITE_MSPI_SPCR1_MSBr(unit, rval);

#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        /* For Now, Hard code GPIO2 and GPIO3 as Output for SPI_CS_SEL */
        SOC_IF_ERROR_RETURN(READ_CMIC_GP_OUT_ENr(unit,&rval));
        fval = soc_reg_field_get(unit, CMIC_GP_OUT_ENr, rval, OUT_ENABLEf);
        fval |= 0xc;
        soc_reg_field_set(unit, CMIC_GP_OUT_ENr, &rval, OUT_ENABLEf, fval);
        SOC_IF_ERROR_RETURN(WRITE_CMIC_GP_OUT_ENr(unit,rval));
    }
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit)) {
        /* Drive GPIO1 high for SPI_CS_SEL */
        SOC_IF_ERROR_RETURN(READ_CMIC_GP_OUT_ENr(unit,&rval));
        fval = soc_reg_field_get(unit, CMIC_GP_OUT_ENr, rval, OUT_ENABLEf);
        fval |= 0x2;
        soc_reg_field_set(unit, CMIC_GP_OUT_ENr, &rval, OUT_ENABLEf, fval);
        SOC_IF_ERROR_RETURN(WRITE_CMIC_GP_OUT_ENr(unit,rval));
    }
#endif /* BCM_TRIUMPH3_SUPPORT || BCM_TRIDENT2_SUPPORT */

    return SOC_E_NONE;
}

int soc_mspi_config(int unit, int device, int cpol, int cpha) {
    uint32 rval;
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    uint32 fval;
    int selval;
#endif /* BCM_KATANA_SUPPORT || BCM_TRIUMPH3_SUPPORT || BCM_TRIDENT2_SUPPORT */
    if (!soc_feature(unit, soc_feature_cmicm)) {
        return SOC_E_FAIL;
    }
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        if(device != -1) {
            /* Set GPIOs Outputs accordingly */
            switch (device) {
                case MSPI_FLASH:
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "MSPI: Selecting Flash\n")));
                    selval = 0;
                    break;
                case MSPI_LIU:
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "MSPI: Selecting LIU\n")));
                    selval = 1;
                    break;
                case MSPI_DPLL:
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "MSPI: Selecting DPLL\n")));
                    selval = 2;
                    break;
                case MSPI_EXTRA:
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "MSPI: Selecting Extra Mux\n")));
                    selval = 3;
                    break;
                default:
                    /* What Device? */
                    return SOC_E_PARAM;
            }
            SOC_IF_ERROR_RETURN(READ_CMIC_GP_DATA_OUTr(unit,&rval));
            fval = soc_reg_field_get(unit, CMIC_GP_DATA_OUTr, rval, DATA_OUTf);
            fval &= 0x3;
            fval |= (selval<<2);
            soc_reg_field_set(unit, CMIC_GP_DATA_OUTr, &rval, DATA_OUTf, fval);
            SOC_IF_ERROR_RETURN(WRITE_CMIC_GP_DATA_OUTr(unit,rval));
        }
    }
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit)) {
        if(device != -1) {
            /* Set GPIOs Outputs accordingly */
            switch (device) {
                case MSPI_DPLL:
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "MSPI: Selecting Flash\n")));
                    selval = 1;
                    break;
                case MSPI_EXTRA:
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "MSPI: Selecting DPLL\n")));
                    selval = 0;
                    break;
                default:
                    /* What Device? */
                    return SOC_E_PARAM;
            }
            SOC_IF_ERROR_RETURN(READ_CMIC_GP_DATA_OUTr(unit,&rval));
            fval = soc_reg_field_get(unit, CMIC_GP_DATA_OUTr, rval, DATA_OUTf);
            fval &= 0xd;
            fval |= (selval<<1);
            soc_reg_field_set(unit, CMIC_GP_DATA_OUTr, &rval, DATA_OUTf, fval);
            SOC_IF_ERROR_RETURN(WRITE_CMIC_GP_DATA_OUTr(unit,rval));
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT || BCM_TRIDENT2_SUPPORT */
    if ((cpol != -1) || (cpha != -1)) {
        SOC_IF_ERROR_RETURN(READ_MSPI_SPCR0_MSBr(unit, &rval));
        if(cpol != -1) {
            soc_reg_field_set(unit, MSPI_SPCR0_MSBr, &rval, CPOLf, (cpol ? 1 : 0));
        }
        if(cpha != -1) {
            soc_reg_field_set(unit, MSPI_SPCR0_MSBr, &rval, CPHAf, (cpha ? 1 : 0));
        }
        SOC_IF_ERROR_RETURN(WRITE_MSPI_SPCR0_MSBr(unit, rval));
    }

    return SOC_E_NONE;
}

int soc_mspi_writeread8(int unit, uint8 *wbuf, int wlen, uint8 *rbuf, int rlen)
{
    int i, tlen, rv = SOC_E_TIMEOUT;
    uint8 *datptr;
    uint32 rval=0;
    soc_timeout_t to;

    if (!soc_feature(unit, soc_feature_cmicm)) {
        return SOC_E_FAIL;
    }

    
    READ_MSPI_SPCR0_LSBr(unit, &rval);
    soc_reg_field_set(unit, MSPI_SPCR0_LSBr, &rval, SPBRf, 0x08);
    WRITE_MSPI_SPCR0_LSBr(unit, rval);

    WRITE_MSPI_STATUSr(unit, 0);

    tlen = wlen + rlen;
    if (tlen > 16) {
        return SOC_E_PARAM;
    }
    if ((wbuf != NULL) && (wlen > 0)) {
        datptr = wbuf;
        for (i=0; i<wlen; i++) {
            /* Use only Even index of TXRAM for 8 bit xmit */
            soc_pci_write(unit, MSPI_TXRAM_nn((2*i)), (uint32) *datptr);
            datptr++;
        }
    }

    for (i=0; i<tlen; i++) {
        /* Release CS ony on last byute */
        soc_pci_write(unit, MSPI_CDRAM_nn(i), (i == (tlen-1)) ? 0 : 0x80);
    }
    /* Set queue pointers */
    WRITE_MSPI_NEWQPr(unit,0);
    WRITE_MSPI_ENDQPr(unit,(tlen-1));

    /* Start SPI transfer */
    rval = 0x40; /* SPE=1, SPIFIE=WREN=WRT0=LOOPQ=HIE=HALT=0 */
    WRITE_MSPI_SPCR2r(unit,rval);

    soc_timeout_init(&to, 10000, 1000);
    
    do {
        READ_MSPI_STATUSr(unit,&rval);
        if (soc_reg_field_get(unit, MSPI_STATUSr,rval, SPIFf)) {
            rv = SOC_E_NONE;
            break;
        }
    } while(!(soc_timeout_check(&to)));

    if (rv == SOC_E_TIMEOUT) {
        return SOC_E_TIMEOUT;
    }

    if ((rbuf != NULL) && (rlen > 0)) {
        datptr = rbuf;
        for (i=wlen; i<tlen; i++) {
            /* Use only Odd index of TXRAM for 8 bit Recv */
            *datptr = (uint8) (soc_pci_read(unit, MSPI_RXRAM_nn(((2*i)+1))) & 0xff);
            datptr++;
        }
    }
    return SOC_E_NONE;
}

int soc_mspi_read8(int unit, uint8 *buf, int len)
{
    return  soc_mspi_writeread8(unit, NULL, 0, buf, len);
}

int soc_mspi_write8(int unit, uint8 *buf, int len)
{
    return  soc_mspi_writeread8(unit, buf, len, NULL, 0);
}

#endif /* ifdef BCM_CMICM_SUPPORT */    

