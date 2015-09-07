/*
 * $Id: reg.c,v 1.126 Broadcom SDK $
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
 * Register address and value manipulations.
 */


#include <shared/bsl.h>

#include <sal/core/libc.h>
#include <sal/core/boot.h>

#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/error.h>
#include <soc/cmic.h>
#include <soc/register.h>
#if defined(BCM_PETRA_SUPPORT)
#include <soc/dpp/drv.h>
#endif /* BCM_PETRA_SUPPORT */
#if defined(BCM_DFE_SUPPORT)
#include <soc/dfe/cmn/dfe_drv.h>
#endif /* BCM_DFE_SUPPORT */
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif
#ifdef BCM_IPROC_SUPPORT
#include <soc/iproc.h>
#endif
#if defined(BCM_KATANA2_SUPPORT)
#include <soc/katana2.h>
#endif
#if defined(BCM_GREYHOUND_SUPPORT)
#include <soc/greyhound.h>
#endif

#define REG_FIRST_BLK_TYPE(regblklist) regblklist[0]

/*
 * Function:   soc_reg_datamask
 * Purpose:    Generate data mask for the fields in a register
 *             whose flags match the flags parameter
 * Returns:    The data mask
 *
 * Notes:  flags can be SOCF_RO, SOCF_WO, or zero (read/write)
 */
uint32
soc_reg_datamask(int unit, soc_reg_t reg, int flags)
{
    int              i, start, end;
    soc_field_info_t *fieldp;
    soc_reg_info_t   *regp;
    uint32           result, mask;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    regp = &(SOC_REG_INFO(unit, reg));

    result = 0;
    for (i = 0; i < (int)(regp->nFields); i++) {
        fieldp = &(regp->fields[i]);

        if ((fieldp->flags & flags) == flags) {
            start = fieldp->bp;
            if (start > 31) {
                continue;
            }
            end = fieldp->bp + fieldp->len;
            if (end < 32) {
                mask = (1 << end) - 1;
            } else {
                mask = -1;
            }
            result |= ((uint32)-1 << start) & mask;
        }
    }

    return result;
}

/*
 * Function:   soc_reg64_datamask
 * Purpose:    Generate data mask for the fields in a 64-bit register
 *             whose flags match the flags parameter
 * Returns:    The data mask
 *
 * Notes:  flags can be SOCF_RO, SOCF_WO, or zero (read/write)
 */
uint64
soc_reg64_datamask(int unit, soc_reg_t reg, int flags)
{
    int              i, start, end;
    soc_field_info_t *fieldp;
    soc_reg_info_t   *regp;
    uint64           mask, tmp, result;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    regp = &(SOC_REG_INFO(unit, reg));

    COMPILER_64_ZERO(result);

    for (i = 0; i < (int)(regp->nFields); i++) {
        fieldp = &(regp->fields[i]);

        if ((fieldp->flags & flags) == flags) {
            start = fieldp->bp;
            end = fieldp->bp + fieldp->len;
            COMPILER_64_SET(mask, 0, 1);
            COMPILER_64_SHL(mask, end);
            COMPILER_64_SUB_32(mask, 1);
            COMPILER_64_ZERO(tmp);
    /*    coverity[overflow_assign]    */
            COMPILER_64_SUB_32(tmp, 1);
            COMPILER_64_SHL(tmp, start);
            COMPILER_64_AND(tmp, mask);
            COMPILER_64_OR(result, tmp);
        }
    }

    return result;
}

/*
 * Function:   soc_reg_above_64_datamask
 * Purpose:    Generate data mask for the fields in above 64-bit register
 *             whose flags match the flags parameter
 *
 * Notes:  flags can be SOCF_RO, SOCF_WO, or zero (read/write)
 */
void
soc_reg_above_64_datamask(int unit, soc_reg_t reg, int flags, soc_reg_above_64_val_t datamask)
{
    int              i;
    soc_field_info_t *fieldp;
    soc_reg_info_t   *regp;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    regp = &(SOC_REG_INFO(unit, reg));

    SOC_REG_ABOVE_64_CLEAR(datamask);

    for (i = 0; i < (int)(regp->nFields); i++) {
        fieldp = &(regp->fields[i]);

        if ((fieldp->flags & flags) == flags) {
            SOC_REG_ABOVE_64_CREATE_MASK(datamask, fieldp->len, fieldp->bp);
        }
    }
}

/************************************************************************/
/* Routines for reading/writing SOC internal registers                        */
/************************************************************************/

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT)|| defined(BCM_DFE_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(PORTMOD_SUPPORT)

STATIC 
void _soc_snoop_reg(int unit, soc_block_t block, int acc, uint32 addr, 
                    uint32 flag, uint32 data_hi, uint32 data_lo) {
    soc_reg_info_t    *reg_info_p;
    soc_regaddrinfo_t ainfo;
    soc_reg_t         reg;

    if (bsl_check(bslLayerSoc, bslSourceTests, bslSeverityNormal, unit) == 0) {
        return;
    }
    if ((block == 0) && (acc == 0)) {
         soc_regaddrinfo_get(unit, &ainfo, addr);
    } else {
         soc_regaddrinfo_extended_get(unit, &ainfo, block, acc, addr);
    }
    reg = (int)ainfo.reg;
    if (SOC_REG_IS_VALID(unit, reg)) {
        reg_info_p = &SOC_REG_INFO(unit, reg);
        /* (SOC_REG_SNOOP_READ & reg_info_p->snoop_flags))  */
        if (NULL != reg_info_p->snoop_cb) {
             if (reg_info_p->snoop_flags & flag) {
                 reg_info_p->snoop_cb(unit, reg,&ainfo, flag, data_hi,data_lo,
                                      reg_info_p->snoop_user_data);
             }
        }
    }
    return ;
}
#ifdef BROADCOM_DEBUG

STATIC void
_soc_reg_debug(int unit, int access_width, char *op_str,
               uint32 addr, uint32 data_hi, uint32 data_lo)
{
    soc_regaddrinfo_t ainfo;
    char              buf[80];

    soc_regaddrinfo_get(unit, &ainfo, addr);

    if (!ainfo.valid || (int)ainfo.reg < 0) {
        sal_strncpy(buf, "??", sizeof(buf));
    } else {
        soc_reg_sprint_addr(unit, buf, &ainfo);
    }

    if (data_hi != 0) {
        LOG_INFO(BSL_LS_SOC_REG,
                 (BSL_META_U(unit,
                             "soc_reg%d_%s unit %d: "
                             "%s[0x%x] data=0x%08x_%08x\n"),
                  access_width, op_str, unit,
                  buf, addr, data_hi, data_lo));
    } else {
        LOG_INFO(BSL_LS_SOC_REG,
                 (BSL_META_U(unit,
                             "soc_reg%d_%s unit %d: "
                             "%s[0x%x] data=0x%08x\n"),
                  access_width, op_str, unit,
                  buf, addr, data_lo));
    }
}

STATIC void
_soc_reg_extended_debug(int unit, int access_width, char *op_str,
                        soc_block_t block, int acc, uint32 addr, 
                        uint32 data_hi, uint32 data_lo)
{
    soc_regaddrinfo_t ainfo;
    char              buf[80];

    soc_regaddrinfo_extended_get(unit, &ainfo, block, acc, addr);

    if (!ainfo.valid || (int)ainfo.reg < 0) {
        sal_strncpy(buf, "??", sizeof(buf));
    } else {
        soc_reg_sprint_addr(unit, buf, &ainfo);
    }

    if (data_hi != 0) {
        LOG_INFO(BSL_LS_SOC_REG,
                 (BSL_META_U(unit,
                             "soc_reg%d_%s unit %d: "
                             "%s[%d][0x%x] data=0x%08x_%08x\n"),
                  access_width, op_str, unit,
                  buf, block, addr, data_hi, data_lo));
    } else {
        LOG_INFO(BSL_LS_SOC_REG,
                 (BSL_META_U(unit,
                             "soc_reg%d_%s unit %d: "
                             "%s[%d][0x%x] data=0x%08x\n"),
                  access_width, op_str, unit,
                  buf, block, addr, data_lo));
    }
}

STATIC void
_soc_reg_above_64_debug(int unit, char *op_str, soc_block_t block, 
               uint32 addr, soc_reg_above_64_val_t data)
{
    soc_regaddrinfo_t ainfo;
    char              buf[80];
    int i, first_non_zero;

    soc_regaddrinfo_extended_get(unit, &ainfo, block, 0, addr);

    if (!ainfo.valid || (int)ainfo.reg < 0) {
        sal_strncpy(buf, "??", sizeof(buf));
    } else {
        soc_reg_sprint_addr(unit, buf, &ainfo);
    }

    LOG_INFO(BSL_LS_SOC_REG,
             (BSL_META_U(unit,
                         "soc_reg_above_64_%s unit %d: "
                         "%s[0x%x] data="),
              op_str, unit, 
              buf, addr));

    first_non_zero = 0;
    for(i=SOC_REG_ABOVE_64_MAX_SIZE_U32-1 ; i>=0 ; i--) {
        if(0 == i) {
            LOG_INFO(BSL_LS_SOC_REG,
                     (BSL_META_U(unit,
                                 "0x%08x\n"),data[i]));
        } else {
            if(data[i] != 0) {
                first_non_zero = 1;
            }

            if(1 == first_non_zero) {
                LOG_INFO(BSL_LS_SOC_REG,
                         (BSL_META_U(unit,
                                     "0x%08x_"),data[i]));
            }
        }
    }
   
}

#endif /* BROADCOM_DEBUG */


#ifdef BCM_BIGMAC_SUPPORT

/* List of registers that need iterative read/write operations */
STATIC int
iterative_op_required(soc_reg_t reg)
{
    switch (reg) {
        case MAC_RXCTRLr:
        case MAC_RXMACSAr:
        case MAC_RXMAXSZr:
        case MAC_RXLSSCTRLr:
        case MAC_RXLSSSTATr:
        case MAC_RXSPARE0r:
        case IR64r:
        case IR127r:
        case IR255r:
        case IR511r:
        case IR1023r:
        case IR1518r:
        case IR2047r:
        case IR4095r:
        case IR9216r:
        case IR16383r:
        case IRMAXr:
        case IRPKTr:
        case IRFCSr:
        case IRUCr:
        case IRMCAr:
        case IRBCAr:
        case IRXPFr:
        case IRXPPr:
        case IRXUOr:
        case IRJBRr:
        case IROVRr:
        case IRXCFr:
        case IRFLRr:
        case IRPOKr:
        case IRMEGr:
        case IRMEBr:
        case IRBYTr:
        case IRUNDr:
        case IRFRGr:
        case IRERBYTr:
        case IRERPKTr:
        case IRJUNKr:
        case MAC_RXLLFCMSGCNTr:
        case MAC_RXLLFCMSGFLDSr:
            return TRUE;
            break;
        default:
            return FALSE;
            break;
    }
}

/*
 * Iterative read procedure for MAC registers on Hyperlite ports.
 */
STATIC int
soc_reg64_read_iterative(int unit, uint32 addr, soc_port_t port,
                         uint64 *data)
{
    int rv, i, diff;
    uint64 xgxs_stat;
    uint32 locked;
    sal_usecs_t t1 = 0, t2;
    soc_timeout_t to;
    for (i = 0; i < 100; i++) {
       /* Read PLL lock status */
       soc_timeout_init(&to, 25 * MILLISECOND_USEC, 0);
       do {
           t1 = sal_time_usecs();
           rv = READ_MAC_XGXS_STATr(unit, port, &xgxs_stat);
           locked = soc_reg64_field32_get(unit, MAC_XGXS_STATr, xgxs_stat, 
                                          TXPLL_LOCKf);
           if (locked || SOC_FAILURE(rv)) {
               break;
           }
       } while (!soc_timeout_check(&to));
       if (SOC_FAILURE(rv)) {
           return rv;
       }
       if (!locked) {
           continue;
       }
       /* Read register value */
       SOC_IF_ERROR_RETURN(soc_reg64_read(unit, addr, data));
       /* Read PLL lock status */
       SOC_IF_ERROR_RETURN(READ_MAC_XGXS_STATr(unit, port, &xgxs_stat));
       locked = soc_reg64_field32_get(unit, MAC_XGXS_STATr, xgxs_stat, 
                                      TXPLL_LOCKf);
       t2 = sal_time_usecs();
       diff = SAL_USECS_SUB(t2, t1);
       if (locked && (diff < 20 * MILLISECOND_USEC)) {
           return SOC_E_NONE;
       }
       LOG_VERBOSE(BSL_LS_SOC_COMMON,
                   (BSL_META_U(unit,
                               "soc_reg64_read_iterative: WARNING: "
                               "iteration %d PLL went out of lock"),
                    i));
    }
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "soc_reg64_read_iterative: "
                          "operation failed:\n"))); 
    return SOC_E_FAIL;    
}
#endif /* BCM_BIGMAC_SUPPORT */

/*
 * Read an internal 64-bit SOC register through S-Channel messaging buffer.
 */
int
_soc_reg64_get(int unit, soc_block_t block, int acc, uint32 addr, uint64 *reg)
{
    schan_msg_t schan_msg;
    int rv, allow_intr = 0;
    int data_byte_len;
    int opcode, err;

    /*
     * Write message to S-Channel.
     */
    schan_msg_clear(&schan_msg);

    data_byte_len = SOC_IS_SIRIUS(unit) ? 0 : 8;
    soc_schan_header_cmd_set(unit, &schan_msg.header, READ_REGISTER_CMD_MSG,
                             block, 0, acc, data_byte_len, 0, 0);

    schan_msg.readcmd.address = addr;

    
    if(SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }
    /* Write header word + address DWORD, read header word + data DWORD */
    rv = soc_schan_op(unit, &schan_msg, 2, 3, allow_intr);
    if (SOC_FAILURE(rv)) {
#if defined(BCM_XGS_SUPPORT)
        int rv1, port = 0, index;
#endif /* BCM_XGS_SUPPORT */
        soc_regaddrinfo_t ainfo;
        
        if (!soc_feature(unit, soc_feature_ser_parity)) {
            return rv;
        }    
        soc_regaddrinfo_extended_get(unit, &ainfo, block, acc, addr);
        if (ainfo.reg != INVALIDr) {
            if (SOC_REG_IS_COUNTER(unit, ainfo.reg)) {
                COMPILER_64_SET(*reg, 0, 0);
                /* Force correct */
                if (!SOC_REG_RETURN_SER_ERROR(unit)) {
                    rv = SOC_E_NONE;
                }
            } 
#if defined(BCM_XGS_SUPPORT)
            else if (soc_feature(unit, soc_feature_regs_as_mem)) {
                if (SOC_REG_INFO(unit, ainfo.reg).regtype == soc_portreg) {
                    port = ainfo.port;
                } else if (SOC_REG_INFO(unit, ainfo.reg).regtype == soc_cosreg) {
                    port = ainfo.cos;
                }
                index = ainfo.idx != -1 ? ainfo.idx : 0;
                rv1 = soc_ser_reg_cache_get(unit, ainfo.reg, port, index, reg);
                if (rv1 != SOC_E_NONE) {
                    if (SOC_REG_IS_DYNAMIC(unit, ainfo.reg)) {
                        COMPILER_64_SET(*reg, 0, 0);
                    } else {
                        return rv;
                    }
                }
                /* Force correct */
                if (!SOC_REG_RETURN_SER_ERROR(unit)) {
                    rv = SOC_E_NONE;
                }
            }
#endif /* BCM_XGS_SUPPORT */
        } else {
            return rv;
        }
    }

    /* Check result */
    soc_schan_header_status_get(unit, &schan_msg.header, &opcode, NULL, NULL,
                                &err, NULL, NULL);;
    if (opcode != READ_REGISTER_ACK_MSG || err != 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "_soc_reg64_get: "
                              "invalid S-Channel reply, expected READ_REG_ACK:\n")));
        soc_schan_dump(unit, &schan_msg, 2);
        return SOC_E_INTERNAL;
    }

#ifdef BROADCOM_DEBUG
    if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
        _soc_reg_extended_debug(unit, 64, "read", block, acc, addr,
                                schan_msg.readresp.data[1],
                                schan_msg.readresp.data[0]);
    }
#endif /* BROADCOM_DEBUG */
    _soc_snoop_reg(unit, block, acc, addr,SOC_REG_SNOOP_READ, 
                   schan_msg.readresp.data[1],schan_msg.readresp.data[0]);

    COMPILER_64_SET(*reg,
                    schan_msg.readresp.data[1],
                    schan_msg.readresp.data[0]);

    return SOC_E_NONE;
}

#ifdef BCM_BIGMAC_SUPPORT

/*
 * Iterative read procedure for MAC registers on Hyperlite ports.
 */
STATIC int
soc_reg64_get_iterative(int unit, soc_block_t block, int acc, uint32 addr,
                        soc_port_t port, uint64 *data)
{
    int rv, i, diff;
    uint64 xgxs_stat;
    uint32 locked;
    sal_usecs_t t1 = 0, t2;
    soc_timeout_t to;
    for (i = 0; i < 100; i++) {
       /* Read PLL lock status */
       soc_timeout_init(&to, 25 * MILLISECOND_USEC, 0);
       do {
           t1 = sal_time_usecs();
           rv = READ_MAC_XGXS_STATr(unit, port, &xgxs_stat);
           locked = soc_reg64_field32_get(unit, MAC_XGXS_STATr, xgxs_stat, 
                                          TXPLL_LOCKf);
           if (locked || SOC_FAILURE(rv)) {
               break;
           }
       } while (!soc_timeout_check(&to));
       if (SOC_FAILURE(rv)) {
           return rv;
       }
       if (!locked) {
           continue;
       }
       /* Read register value */
       SOC_IF_ERROR_RETURN(_soc_reg64_get(unit, block, acc, addr, data));
       /* Read PLL lock status */
       SOC_IF_ERROR_RETURN(READ_MAC_XGXS_STATr(unit, port, &xgxs_stat));
       locked = soc_reg64_field32_get(unit, MAC_XGXS_STATr, xgxs_stat, 
                                      TXPLL_LOCKf);
       t2 = sal_time_usecs();
       diff = SAL_USECS_SUB(t2, t1);
       if (locked && (diff < 20 * MILLISECOND_USEC)) {
           return SOC_E_NONE;
       }
       LOG_VERBOSE(BSL_LS_SOC_COMMON,
                   (BSL_META_U(unit,
                               "soc_reg64_get_iterative: WARNING: "
                               "iteration %d PLL went out of lock"),
                    i));
    }
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "soc_reg64_get_iterative: "
                          "operation failed:\n"))); 
    return SOC_E_FAIL;    
}

#endif /* BCM_BIGMAC_SUPPORT */

/*
 * Read an internal SOC register through S-Channel messaging buffer.
 * Checks if the register is 32 or 64 bits.
 */

int
soc_reg_read(int unit, soc_reg_t reg, uint32 addr, uint64 *data)
{
#if defined(BCM_PETRAB_SUPPORT)
    if (SOC_IS_PETRAB(unit)){
        return soc_dpp_reg_read(unit, reg, addr, data);
    }
#endif /* BCM_PETRAB_SUPPORT */

    if (!SOC_REG_IS_VALID(unit, reg)) {
        return SOC_E_PARAM;
    }

    if (SOC_REG_IS_ABOVE_64(unit, reg)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_reg_read: "
                              "Use soc_reg_above_64_get \n")));
        
        return SOC_E_FAIL;
    }
    
    if (SOC_REG_IS_64(unit, reg)) {
        soc_port_t port;
        soc_block_types_t regblktype = SOC_REG_INFO(unit, reg).block;
        int blk, pindex, bindex, block;
        pindex = (addr >> SOC_REGIDX_BP) & 0x3f;
        block = ((addr >> SOC_BLOCK_BP) & 0xf) |
                (((addr >> SOC_BLOCK_MSB_BP) & 0x3) << 4);
        if (SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_PORT) 
#ifdef BCM_PETRA_SUPPORT
           && !SOC_IS_DPP(unit)
#endif /* BCM_PETRA_SUPPORT */
#ifdef BCM_DFE_SUPPORT
           && !SOC_IS_DFE(unit)
#endif /* BCM_DFE_SUPPORT */
#ifdef BCM_BIGMAC_SUPPORT
           && iterative_op_required(reg)
#endif /* BCM_BIGMAC_SUPPORT */
            ) {
            PBMP_HYPLITE_ITER(unit, port) {
                blk = SOC_PORT_BLOCK(unit, port);
                bindex = SOC_PORT_BINDEX(unit, port);
                if ((SOC_BLOCK2SCH(unit, blk) == block) && (bindex == pindex)) {
                    break;
                }
            }         
            if (!IS_HYPLITE_PORT(unit, port)) {
                return soc_reg64_read(unit, addr, data);
            } 
#ifdef BCM_BIGMAC_SUPPORT
            else {   
                return soc_reg64_read_iterative(unit, addr, port, data);
            }
#endif /* BCM_BIGMAC_SUPPORT */           
        } else {
            return soc_reg64_read(unit, addr, data);
        }
    } else {
        uint32 data32;

        SOC_IF_ERROR_RETURN(soc_reg32_read(unit, addr, &data32));
        COMPILER_64_SET(*data, 0, data32);
    }

    return SOC_E_NONE;
}

/*
 * Read an internal SOC register through S-Channel messaging buffer. 
 *  
 * block is cmic block id 
 */

int
soc_direct_memreg_get(int unit, int cmic_block, uint32 addr, uint32 dwc_read, int is_mem, uint32 *data)
{
    schan_msg_t schan_msg;
    uint32 i;
    int allow_intr = 0;
    int data_byte_len;
    int opcode, err;

    /*
     * Write message to S-Channel.
     */
    schan_msg_clear(&schan_msg);

    soc_schan_header_cmd_set(unit, &schan_msg.header, (is_mem? READ_MEMORY_CMD_MSG : READ_REGISTER_CMD_MSG),
                             cmic_block, SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit)),
                             0, dwc_read * 4, 0, 0);

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm)) {
        schan_msg.readcmd.address = addr;
    } else
#endif
    {
        uint32 cmice_addr = addr;
        if (cmic_block >= 0) {
            cmice_addr |= ((cmic_block & 0xf) << SOC_BLOCK_BP) | 
                            (((cmic_block >> 4) & 0x3) << SOC_BLOCK_MSB_BP);
        }
        
        schan_msg.readcmd.address = cmice_addr;
    }

    if(SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }
    
    /* Write header word + address DWORD, read header word + data DWORD */
    SOC_IF_ERROR_RETURN(soc_schan_op(unit, &schan_msg, 2, dwc_read+1, allow_intr));

    /* Check result */
    soc_schan_header_status_get(unit, &schan_msg.header, &opcode, NULL,
                                &data_byte_len, &err, NULL, NULL);
    if (opcode != READ_REGISTER_ACK_MSG || err != 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_direct_reg_get: "
                              "invalid S-Channel reply, expected READ_REG_ACK:\n")));
        soc_schan_dump(unit, &schan_msg, 2);
        return SOC_E_INTERNAL;
    }

    for(i = 0; i < data_byte_len / 4; i++) {
        data[i] = schan_msg.readresp.data[i];
    }

    return SOC_E_NONE;
}

int
soc_direct_reg_get(int unit, int cmic_block, uint32 addr, uint32 dwc_read, uint32 *data)
{
    return soc_direct_memreg_get(unit, cmic_block, addr, dwc_read, 0, data);
}

int
soc_direct_memreg_set(int unit, int cmic_block, uint32 addr, uint32 dwc_write, int is_mem, uint32 *data)
{
    schan_msg_t schan_msg;
    int i, allow_intr = 0;

    /*
     * Setup S-Channel command packet
     *
     * NOTE: the datalen field matters only for the Write Memory and
     * Write Register commands, where it is used only by the CMIC to
     * determine how much data to send, and is in units of bytes.
     */

    schan_msg_clear(&schan_msg);

    soc_schan_header_cmd_set(unit, &schan_msg.header, (is_mem? WRITE_MEMORY_CMD_MSG : WRITE_REGISTER_CMD_MSG),
                             cmic_block, SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit)),
                             0, dwc_write * 4, 0, 0);

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm)) {
        schan_msg.writecmd.address = addr;
    } else
#endif
    {
        uint32 cmice_addr = addr;
        if (cmic_block >= 0) {
            cmice_addr |= ((cmic_block & 0xf) << SOC_BLOCK_BP) |
                         (((cmic_block >> 4) & 0x3) << SOC_BLOCK_MSB_BP);
        }
        
        schan_msg.readcmd.address = cmice_addr;
    }
    
    for(i=0 ; i<dwc_write ; i++)
      schan_msg.writecmd.data[i] = data[i];


    if(SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /* Write header word + address + data DWORD */
    /* Note: The hardware does not send WRITE_REGISTER_ACK_MSG. */
    
    

    return soc_schan_op(unit, &schan_msg, dwc_write+2, 0, allow_intr);
}

int
soc_direct_reg_set(int unit, int cmic_block, uint32 addr, uint32 dwc_write, uint32 *data)
{
    return soc_direct_memreg_set(unit, cmic_block, addr, dwc_write, 0, data);
}

/*
 * Read an internal SOC register through S-Channel messaging buffer.
 */

int
_soc_reg32_get(int unit, soc_block_t block, int acc, uint32 addr, uint32 *data)
{
    schan_msg_t schan_msg;
    int rv, allow_intr = 0;
    int data_byte_len;
    int opcode, err;

    /*
     * Write message to S-Channel.
     */
    schan_msg_clear(&schan_msg);

    data_byte_len = SOC_IS_SIRIUS(unit) ? 0 : 4;
    soc_schan_header_cmd_set(unit, &schan_msg.header, READ_REGISTER_CMD_MSG,
                             block, 0, acc, data_byte_len, 0, 0);

    schan_msg.readcmd.address = addr;

    if(SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /* Write header word + address DWORD, read header word + data DWORD */
    rv = soc_schan_op(unit, &schan_msg, 2, 2, allow_intr);
    if (SOC_FAILURE(rv)) {
#if defined(BCM_XGS_SUPPORT)
        int rv1, port = 0, index;
#endif /* BCM_XGS_SUPPORT */
        soc_regaddrinfo_t ainfo;
        
        if (!soc_feature(unit, soc_feature_ser_parity)) {
            return rv;
        }
        soc_regaddrinfo_extended_get(unit, &ainfo, block, acc, addr);
        if (ainfo.reg != INVALIDr) {
            if (SOC_REG_IS_COUNTER(unit, ainfo.reg)) {
                *data = 0;
                /* Force correct */
                if (!SOC_REG_RETURN_SER_ERROR(unit)) {
                    rv = SOC_E_NONE;
                }
            } 
#if defined(BCM_XGS_SUPPORT)
            else if (soc_feature(unit, soc_feature_regs_as_mem)) {
                if (SOC_REG_INFO(unit, ainfo.reg).regtype == soc_portreg) {
                    port = ainfo.port;
                } else if (SOC_REG_INFO(unit, ainfo.reg).regtype == soc_cosreg) {
                    port = ainfo.cos;
                }
                index = ainfo.idx != -1 ? ainfo.idx : 0;
                rv1 = soc_ser_reg32_cache_get(unit, ainfo.reg, port, index, data);
                if (rv1 != SOC_E_NONE) {
                    if (SOC_REG_IS_DYNAMIC(unit, ainfo.reg)) {
                        *data = 0;
                    } else {
                        return rv;
                    }
                }
                /* Force correct */
                if (!SOC_REG_RETURN_SER_ERROR(unit)) {
                    rv = SOC_E_NONE;
                }
            }
#endif /* BCM_XGS_SUPPORT */
        } else {
            return rv;
        }
    }

    /* Check result */
    soc_schan_header_status_get(unit, &schan_msg.header, &opcode, NULL, NULL,
                                &err, NULL, NULL);
    if (opcode != READ_REGISTER_ACK_MSG || err != 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "_soc_reg32_get: "
                              "invalid S-Channel reply, expected READ_REG_ACK:\n")));
        soc_schan_dump(unit, &schan_msg, 2);
        return SOC_E_INTERNAL;
    }

    *data = schan_msg.readresp.data[0];

#ifdef BROADCOM_DEBUG
    if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
        _soc_reg_extended_debug(unit, 32, "read", block, acc, addr, 0, *data);
    }
#endif /* BROADCOM_DEBUG */
    _soc_snoop_reg(unit, block, acc, addr,SOC_REG_SNOOP_READ, 0,*data);

    return SOC_E_NONE;
}

/*
 * Read an internal SOC register through S-Channel messaging buffer.
 * Checks if the register is 32 or 64 bits.
 */

int
soc_reg_get(int unit, soc_reg_t reg, int port, int index, uint64 *data)
{
    uint32 addr;
    int block;
    int pindex = port; 
    uint8 acc_type;
    if (!SOC_REG_IS_VALID(unit, reg)) {
        return SOC_E_PARAM;
    }

    if (SOC_REG_IS_ABOVE_64(unit, reg)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_reg_get: "
                              "Use soc_reg_above_64_get \n")));
        
        return SOC_E_FAIL;
    }

    if(SOC_INFO(unit).reg_access.reg64_get) {
        return SOC_INFO(unit).reg_access.reg64_get(unit, reg, port, index, data);
    }

    addr = soc_reg_addr_get(unit, reg, port, index, FALSE, &block, &acc_type);
    if (SOC_REG_IS_64(unit, reg)) {
        soc_port_t _port;
        soc_block_types_t regblktype = SOC_REG_INFO(unit, reg).block;
        int blk, bindex;
        if (!soc_feature(unit, soc_feature_new_sbus_format)) {
           return soc_reg_read(unit, reg, addr, data);
        }

        if (SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_PORT) 
#ifdef BCM_PETRA_SUPPORT
           && !SOC_IS_DPP(unit)
#endif /* BCM_PETRA_SUPPORT */
#ifdef BCM_DFE_SUPPORT
           && !SOC_IS_DFE(unit)
#endif /* BCM_DFE_SUPPORT */
#ifdef BCM_BIGMAC_SUPPORT
           && iterative_op_required(reg)
#endif /* BCM_BIGMAC_SUPPORT */
            ) {
            PBMP_HYPLITE_ITER(unit, _port) {
                blk = SOC_PORT_BLOCK(unit, _port);
                bindex = SOC_PORT_BINDEX(unit, _port);
                if ((SOC_BLOCK2SCH(unit, blk) == block) && (bindex == pindex)) {
                    break;
                }
            }         
            if (!IS_HYPLITE_PORT(unit, port)) {
                return _soc_reg64_get(unit, block, acc_type, addr, data);
            }  
#ifdef BCM_BIGMAC_SUPPORT
            else
            {   
                return soc_reg64_get_iterative(unit, block, acc_type, addr,
                                               port, data);
            }
#endif            
        } else {
            return _soc_reg64_get(unit, block, acc_type, addr, data);
        }
    } else {
        uint32 data32;
        if (!soc_feature(unit, soc_feature_new_sbus_format)) {
            SOC_IF_ERROR_RETURN(soc_reg32_read(unit, addr, &data32));
        } else {
            SOC_IF_ERROR_RETURN
                (_soc_reg32_get(unit, block, acc_type, addr, &data32));
        }
        COMPILER_64_SET(*data, 0, data32);
    }

    return SOC_E_NONE;
}

/*
 * Read an internal SOC register through S-Channel messaging buffer.
 * Handle register at any size 
 */
 
int
soc_reg_above_64_get(int unit, soc_reg_t reg, int port, int index, soc_reg_above_64_val_t data)
{
    uint32 addr;
    int block;
    uint8 at; 
    uint64 data64;
    int rc;
    int reg_size;
    
    if (!SOC_REG_IS_VALID(unit, reg)) {
        return SOC_E_PARAM;
    }
    
    SOC_REG_ABOVE_64_CLEAR(data);
    
    if(SOC_INFO(unit).reg_access.reg_above64_get) {
        return SOC_INFO(unit).reg_access.reg_above64_get(unit, reg, port, index, data);
    }

    if (SOC_REG_IS_ABOVE_64(unit, reg)) 
    {
        reg_size = SOC_REG_ABOVE_64_INFO(unit, reg).size;
        addr = soc_reg_addr_get(unit, reg, port, index, FALSE, &block, &at);
        if (!soc_feature(unit, soc_feature_new_sbus_format)) {
            block = ((addr >> SOC_BLOCK_BP) & 0xf) | (((addr >> SOC_BLOCK_MSB_BP) & 0x3) << 4);
        }
        rc = soc_direct_reg_get(unit, block, addr, reg_size, data);

#ifdef BROADCOM_DEBUG
        if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
            _soc_reg_above_64_debug(unit, "get", block, addr, data);
        }
#endif /* BROADCOM_DEBUG */

        return rc;

    } 
    else if (SOC_REG_IS_64(unit, reg)) {
        COMPILER_64_SET(data64, data[1], data[0]);
        rc = soc_reg_get(unit, reg, port, index, &data64);
        data[0] = COMPILER_64_LO(data64);
        data[1] = COMPILER_64_HI(data64);
        return rc;
    } 
    else {
        rc = soc_reg_get(unit, reg, port, index, &data64);
        data[0] = COMPILER_64_LO(data64);
        return rc;
    }
}

/*
 * Read an internal SOC register through S-Channel messaging buffer.
 */

int
soc_reg32_read(int unit,
               uint32 addr,
               uint32 *data)
{
    schan_msg_t schan_msg;
    int rv, allow_intr = 0;
    int dst_blk, src_blk, data_byte_len;
    int opcode, err;

#ifdef BCM_CMICM_SUPPORT
    uint32 fsdata = 0;
    int cmc = SOC_PCI_CMC(unit);
#endif

#if defined(BCM_PETRAB_SUPPORT)
    if (SOC_IS_PETRAB(unit)) {
        return soc_dpp_reg32_read(unit, addr, data);
    }
#endif /* BCM_PETRAB_SUPPORT */

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) &&
        (NULL != SOC_CONTROL(unit)->fschanMutex)) {
        FSCHAN_LOCK(unit);
        soc_pci_write(unit, CMIC_CMCx_FSCHAN_ADDRESS_OFFSET(cmc), addr);
        fsdata = soc_pci_read(unit, CMIC_CMCx_FSCHAN_DATA32_OFFSET(cmc));
        FSCHAN_UNLOCK(unit);
        *data = fsdata;
    } else
#endif
    {
        /*
         * Write message to S-Channel.
         */
        schan_msg_clear(&schan_msg);

        dst_blk = ((addr >> SOC_BLOCK_BP) & 0xf) | 
            (((addr >> SOC_BLOCK_MSB_BP) & 0x3) << 4);
#if defined(BCM_SIRIUS_SUPPORT)
        if (SOC_IS_SIRIUS(unit)) {
            src_blk = 0;
            data_byte_len = 0;
        } else
#endif /* BCM_SIRIUS_SUPPORT */
        {
            src_blk = SOC_IS_SHADOW(unit) ?
                0 : SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
            data_byte_len = SOC_IS_XGS12_FABRIC(unit) ? 8 : 4;
        }
        soc_schan_header_cmd_set(unit, &schan_msg.header,
                                 READ_REGISTER_CMD_MSG, dst_blk, src_blk, 0,
                                 data_byte_len, 0, 0);

        schan_msg.readcmd.address = addr;

        if(SOC_IS_SAND(unit)) {
            allow_intr = 1;
        }

        /* Write header word + address DWORD, read header word + data DWORD */
        rv = soc_schan_op(unit, &schan_msg, 2, 2, allow_intr);
        if (SOC_FAILURE(rv)) {
#if defined(BCM_XGS_SUPPORT)
            int rv1, port = 0, index;
#endif /* BCM_XGS_SUPPORT */
            soc_regaddrinfo_t ainfo;
            
            if (!soc_feature(unit, soc_feature_ser_parity)) {
                return rv;
            }
            soc_regaddrinfo_get(unit, &ainfo, addr);            
            if (ainfo.reg != INVALIDr) {
                if (SOC_REG_IS_COUNTER(unit, ainfo.reg)) {
                    *data = 0;
                    /* Force correct */
                    if (!SOC_REG_RETURN_SER_ERROR(unit)) {
                        rv = SOC_E_NONE;
                    }
                } 
#if defined(BCM_XGS_SUPPORT)
                else if (soc_feature(unit, soc_feature_regs_as_mem)) {
                    if (SOC_REG_INFO(unit, ainfo.reg).regtype == soc_portreg) {
                        port = ainfo.port;
                    } else if (SOC_REG_INFO(unit, ainfo.reg).regtype == soc_cosreg) {
                        port = ainfo.cos;
                    }
                    index = ainfo.idx != -1 ? ainfo.idx : 0;
                    rv1 = soc_ser_reg32_cache_get(unit, ainfo.reg, port, index, data);
                    if (rv1 != SOC_E_NONE) {
                        if (SOC_REG_IS_DYNAMIC(unit, ainfo.reg)) {
                            *data = 0;
                        } else {
                            return rv;
                        }
                    }
                    /* Force correct */
                    if (!SOC_REG_RETURN_SER_ERROR(unit)) {
                        rv = SOC_E_NONE;
                    }
                }
#endif /* BCM_XGS_SUPPORT */
            } else {
                return rv;
            }
        }

        /* Check result */
        soc_schan_header_status_get(unit, &schan_msg.header, &opcode, NULL,
                                    NULL, &err, NULL, NULL);
        if (!SOC_FAILURE(rv) && 
            (opcode != READ_REGISTER_ACK_MSG || err != 0)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "soc_reg32_read: "
                                  "invalid S-Channel reply, expected READ_REG_ACK:\n")));
            soc_schan_dump(unit, &schan_msg, 2);
            return SOC_E_INTERNAL;
        }
        *data = schan_msg.readresp.data[0];
    }
#ifdef BROADCOM_DEBUG
    if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
        _soc_reg_debug(unit, 32, "read", addr, 0, *data);
    }
#endif /* BROADCOM_DEBUG */
    _soc_snoop_reg(unit, 0, 0, addr,SOC_REG_SNOOP_READ, 0, *data);
    return SOC_E_NONE;
}

/*
 * Read an internal SOC register through S-Channel messaging buffer.
 */

int
soc_reg32_get(int unit, soc_reg_t reg, int port, int index, uint32 *data)
{
    uint32 addr;
    int block = 0;
    uint8 acc_type;

    if (!SOC_REG_IS_VALID(unit, reg)) {
        return SOC_E_PARAM;
    }

    if (SOC_REG_IS_ABOVE_32(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is > 32 bit , but called with soc_reg32_get\n"), soc_reg_name[reg]));
#endif
    }
    assert(!SOC_REG_IS_ABOVE_32(unit, reg));

    if(SOC_INFO(unit).reg_access.reg32_get) {
        return SOC_INFO(unit).reg_access.reg32_get(unit, reg, port, index, data);
    }

    addr = soc_reg_addr_get(unit, reg, port, index, FALSE, &block, &acc_type);

    if (!soc_feature(unit, soc_feature_new_sbus_format)) {
        return soc_reg32_read(unit, addr, data);
    }
    return _soc_reg32_get(unit, block, acc_type, addr, data);
}

/*
 * Read an internal 64-bit SOC register through S-Channel messaging buffer.
 */
int
soc_reg64_read(int unit,
               uint32 addr,
               uint64 *reg)
{
    schan_msg_t schan_msg;
    int rv, allow_intr = 0;
    int dst_blk, src_blk, data_byte_len;
    int opcode, err;

#ifdef BCM_CMICM_SUPPORT
    uint32 fsdatal = 0, fsdatah = 0;
    int cmc = SOC_PCI_CMC(unit);
#endif

#if defined(BCM_PETRAB_SUPPORT)
    if (SOC_IS_PETRAB(unit)) {
        return soc_dpp_reg64_read(unit, addr, reg);
    }
#endif /* BCM_PETRAB_SUPPORT */

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) &&
        (NULL != SOC_CONTROL(unit)->fschanMutex)) {
        FSCHAN_LOCK(unit);
        soc_pci_write(unit, CMIC_CMCx_FSCHAN_ADDRESS_OFFSET(cmc), addr);
        fsdatal = soc_pci_read(unit, CMIC_CMCx_FSCHAN_DATA64_LO_OFFSET(cmc));
        fsdatah = soc_pci_read(unit, CMIC_CMCx_FSCHAN_DATA64_HI_OFFSET(cmc));
        FSCHAN_UNLOCK(unit);
        COMPILER_64_SET(*reg, fsdatah, fsdatal);
    } else
#endif
    {
        /*
         * Write message to S-Channel.
         */
        schan_msg_clear(&schan_msg);

        dst_blk = ((addr >> SOC_BLOCK_BP) & 0xf) | 
            (((addr >> SOC_BLOCK_MSB_BP) & 0x3) << 4);
#if defined(BCM_SIRIUS_SUPPORT)
        if (SOC_IS_SIRIUS(unit)) {
            src_blk = 0;
            data_byte_len = 0;
        } else
#endif /* BCM_SIRIUS_SUPPORT */
        {
            src_blk = SOC_IS_SHADOW(unit) ?
                0 : SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
            data_byte_len = 8;
        }
        soc_schan_header_cmd_set(unit, &schan_msg.header,
                                 READ_REGISTER_CMD_MSG, dst_blk, src_blk, 0,
                                 data_byte_len, 0, 0);

        schan_msg.readcmd.address = addr;

        if(SOC_IS_SAND(unit)) {
            allow_intr = 1;
        }

        /* Write header word + address DWORD, read header word + data DWORD */
        rv = soc_schan_op(unit, &schan_msg, 2, 3, allow_intr);
        if (SOC_FAILURE(rv)) {
#if defined(BCM_XGS_SUPPORT)
            int rv1, port = 0, index;
#endif /* BCM_XGS_SUPPORT */
            soc_regaddrinfo_t ainfo;
            
            if (!soc_feature(unit, soc_feature_ser_parity)) {
                return rv;
            }    
            soc_regaddrinfo_get(unit, &ainfo, addr);            
            if (ainfo.reg != INVALIDr) {
                if (SOC_REG_IS_COUNTER(unit, ainfo.reg)) {
                    COMPILER_64_SET(*reg, 0, 0);
                    /* Force correct */
                    if (!SOC_REG_RETURN_SER_ERROR(unit)) {
                        rv = SOC_E_NONE;
                    }
                } 
#if defined(BCM_XGS_SUPPORT)
                else if (soc_feature(unit, soc_feature_regs_as_mem)) {
                    if (SOC_REG_INFO(unit, ainfo.reg).regtype == soc_portreg) {
                        port = ainfo.port;
                    } else if (SOC_REG_INFO(unit, ainfo.reg).regtype == soc_cosreg) {
                        port = ainfo.cos;
                    }
                    index = ainfo.idx != -1 ? ainfo.idx : 0;
                    rv1 = soc_ser_reg_cache_get(unit, ainfo.reg, port, index, reg);
                    if (rv1 != SOC_E_NONE) {
                        if (SOC_REG_IS_DYNAMIC(unit, ainfo.reg)) {
                            COMPILER_64_SET(*reg, 0, 0);
                        } else {
                            return rv;
                        }
                    }
                    /* Force correct */
                    if (!SOC_REG_RETURN_SER_ERROR(unit)) {
                        rv = SOC_E_NONE;
                    }
                }
#endif /* BCM_XGS_SUPPORT */
            } else {
                return rv;
            }
        }

        /* Check result */
        soc_schan_header_status_get(unit, &schan_msg.header, &opcode, NULL,
                                    NULL, &err, NULL, NULL);
        if (opcode != READ_REGISTER_ACK_MSG || err != 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "soc_reg64_read: "
                                  "invalid S-Channel reply, expected READ_REG_ACK:\n")));
            soc_schan_dump(unit, &schan_msg, 2);
            return SOC_E_INTERNAL;
        }
        COMPILER_64_SET(*reg, schan_msg.readresp.data[1],
                        schan_msg.readresp.data[0]);
    }

#ifdef BROADCOM_DEBUG
    if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
        _soc_reg_debug(unit, 64, "read", addr,
                       schan_msg.readresp.data[1],
                       schan_msg.readresp.data[0]);
    }
#endif /* BROADCOM_DEBUG */
    _soc_snoop_reg(unit, 0, 0, addr,SOC_REG_SNOOP_READ, 
                   schan_msg.readresp.data[1], schan_msg.readresp.data[0]);

    return SOC_E_NONE;
}

/*
 * Read an internal 64-bit SOC register through S-Channel messaging buffer.
 */
int
soc_reg64_get(int unit, soc_reg_t reg, int port, int index, uint64 *data)
{
    uint32 addr;
    int block = 0;
    uint8 acc_type;
    addr = soc_reg_addr_get(unit, reg, port, index, FALSE, &block, &acc_type);
    assert(SOC_REG_IS_64(unit, reg));
    if (!soc_feature(unit, soc_feature_new_sbus_format)) {
        return soc_reg64_read(unit, addr, data);
    }
    return _soc_reg64_get(unit, block, acc_type, addr, data);
}

#ifdef BCM_BIGMAC_SUPPORT
/*
 * Iterative write procedure for MAC registers on Hyperlite ports.
 */
STATIC int
soc_reg64_write_iterative(int unit, uint32 addr, soc_port_t port,
                          uint64 data)
{
    int rv, i, diff;
    uint64 xgxs_stat;
    uint32 locked;
    sal_usecs_t t1 = 0, t2;
    soc_timeout_t to;
    for (i = 0; i < 100; i++) {
       /* Read PLL lock status */
       soc_timeout_init(&to, 25 * MILLISECOND_USEC, 0);
       do {
           t1 = sal_time_usecs();
           rv = READ_MAC_XGXS_STATr(unit, port, &xgxs_stat);
           locked = soc_reg64_field32_get(unit, MAC_XGXS_STATr, xgxs_stat, 
                                          TXPLL_LOCKf);
           if (locked || SOC_FAILURE(rv)) {
               break;
           }
       } while (!soc_timeout_check(&to));
       if (SOC_FAILURE(rv)) {
           return rv;
       }
       if (!locked) {
           continue;
       }
       /* Write register value */
       SOC_IF_ERROR_RETURN(soc_reg64_write(unit, addr, data));
       /* Read PLL lock status */
       SOC_IF_ERROR_RETURN(READ_MAC_XGXS_STATr(unit, port, &xgxs_stat));
       locked = soc_reg64_field32_get(unit, MAC_XGXS_STATr, xgxs_stat, 
                                      TXPLL_LOCKf);
       t2 = sal_time_usecs();
       diff = SAL_USECS_SUB(t2, t1);
       if (locked && (diff < 20 * MILLISECOND_USEC)) {
           return SOC_E_NONE;
       }
       LOG_VERBOSE(BSL_LS_SOC_COMMON,
                   (BSL_META_U(unit,
                               "soc_reg64_write_iterative: WARNING: "
                               "iteration %d PLL went out of lock"),
                    i));
    }
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "soc_reg64_write_iterative: "
                          "operation failed:\n"))); 
    return SOC_E_FAIL;    
}

#endif /* BCM_BIGMAC_SUPPORT */

/*
 * Write an internal 64-bit SOC register through S-Channel messaging buffer.
 */
int
_soc_reg64_set(int unit, soc_block_t block, int acc, uint32 addr, uint64 data)
{
    schan_msg_t schan_msg;
    int allow_intr = 0;

    /*
     * Setup S-Channel command packet
     *
     * NOTE: the datalen field matters only for the Write Memory and
     * Write Register commands, where it is used only by the CMIC to
     * determine how much data to send, and is in units of bytes.
     */

    schan_msg_clear(&schan_msg);

    soc_schan_header_cmd_set(unit, &schan_msg.header, WRITE_REGISTER_CMD_MSG,
                             block, 0, acc, 8, 0, 0);

    schan_msg.writecmd.address = addr;
    schan_msg.writecmd.data[0] = COMPILER_64_LO(data);
    schan_msg.writecmd.data[1] = COMPILER_64_HI(data);

#ifdef BROADCOM_DEBUG
    if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
        _soc_reg_extended_debug(unit, 64, "write", block, acc, addr,
                                schan_msg.writecmd.data[1],
                                schan_msg.writecmd.data[0]);
    }
#endif /* BROADCOM_DEBUG */
    _soc_snoop_reg(unit, block, acc, addr,SOC_REG_SNOOP_WRITE, 
                   schan_msg.readresp.data[1],schan_msg.readresp.data[0]);

    if(SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /* Write header word + address + 2*data DWORD */
    /* Note: The hardware does not send WRITE_REGISTER_ACK_MSG. */
    
    

    return soc_schan_op(unit, &schan_msg, 4, 0, allow_intr);
    
}

#ifdef BCM_BIGMAC_SUPPORT

/*
 * Iterative write procedure for MAC registers on Hyperlite ports.
 */
STATIC int
soc_reg64_set_iterative(int unit, soc_block_t block, int acc, uint32 addr,
                        soc_port_t port, uint64 data)
{
    int rv, i, diff;
    uint64 xgxs_stat;
    uint32 locked;
    sal_usecs_t t1 = 0, t2;
    soc_timeout_t to;
    for (i = 0; i < 100; i++) {
       /* Read PLL lock status */
       soc_timeout_init(&to, 25 * MILLISECOND_USEC, 0);
       do {
           t1 = sal_time_usecs();
           rv = READ_MAC_XGXS_STATr(unit, port, &xgxs_stat);
           locked = soc_reg64_field32_get(unit, MAC_XGXS_STATr, xgxs_stat, 
                                          TXPLL_LOCKf);
           if (locked || SOC_FAILURE(rv)) {
               break;
           }
       } while (!soc_timeout_check(&to));
       if (SOC_FAILURE(rv)) {
           return rv;
       }
       if (!locked) {
           continue;
       }
       /* Write register value */
       SOC_IF_ERROR_RETURN(_soc_reg64_set(unit, block, acc, addr, data));
       /* Read PLL lock status */
       SOC_IF_ERROR_RETURN(READ_MAC_XGXS_STATr(unit, port, &xgxs_stat));
       locked = soc_reg64_field32_get(unit, MAC_XGXS_STATr, xgxs_stat, 
                                      TXPLL_LOCKf);
       t2 = sal_time_usecs();
       diff = SAL_USECS_SUB(t2, t1);
       if (locked && (diff < 20 * MILLISECOND_USEC)) {
           return SOC_E_NONE;
       }
       LOG_VERBOSE(BSL_LS_SOC_COMMON,
                   (BSL_META_U(unit,
                               "soc_reg64_set_iterative: WARNING: "
                               "iteration %d PLL went out of lock"),
                    i));
    }
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "soc_reg64_set_iterative: "
                          "operation failed:\n"))); 
    return SOC_E_FAIL;    
}

#endif /* BCM_BIGMAC_SUPPORT */

/*
 * Write an internal SOC register through S-Channel messaging buffer.
 * Checks if the register is 32 or 64 bits.
 */

int
soc_reg_write(int unit, soc_reg_t reg, uint32 addr, uint64 data)
{
#if defined(BCM_PETRAB_SUPPORT)
    if (SOC_IS_PETRAB(unit)) {
        return soc_dpp_reg_write(unit, reg, addr, data);
    }
#endif /* BCM_PETRAB_SUPPORT */

    if (!SOC_REG_IS_VALID(unit, reg)) {
        return SOC_E_PARAM;
    }

    if (SOC_REG_IS_ABOVE_64(unit, reg)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_reg_write: "
                              "Use soc_reg_above_64_set \n")));
        
        return SOC_E_FAIL;
    }
    
    if (SOC_REG_IS_64(unit, reg)) {
        soc_port_t port;
        soc_block_types_t regblktype = SOC_REG_INFO(unit, reg).block;
        int blk, pindex, bindex, block;
        pindex = (addr >> SOC_REGIDX_BP) & 0x3f;
        block = ((addr >> SOC_BLOCK_BP) & 0xf) |
                (((addr >> SOC_BLOCK_MSB_BP) & 0x3) << 4);
        if (SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_PORT) 
#ifdef BCM_PETRA_SUPPORT
           && !SOC_IS_DPP(unit)
#endif /* BCM_PETRA_SUPPORT */
#ifdef BCM_DFE_SUPPORT
           && !SOC_IS_DFE(unit)
#endif /* BCM_DFE_SUPPORT */
#ifdef BCM_BIGMAC_SUPPORT
           && iterative_op_required(reg)
#endif /* BCM_BIGMAC_SUPPORT */
            ) {
            PBMP_HYPLITE_ITER(unit, port) {
                blk = SOC_PORT_BLOCK(unit, port);
                bindex = SOC_PORT_BINDEX(unit, port);
                if ((SOC_BLOCK2SCH(unit, blk) == block) && (bindex == pindex)) {
                    break;
                }
            }         
            if (!IS_HYPLITE_PORT(unit, port)) {
                return soc_reg64_write(unit, addr, data);
            } 
#ifdef BCM_BIGMAC_SUPPORT
            else {   
                return soc_reg64_write_iterative(unit, addr, port, data);
            }
#endif /* BCM_BIGMAC_SUPPORT */           
        } else {
            return soc_reg64_write(unit, addr, data);
        }
    } else {
        if (COMPILER_64_HI(data)) {
            LOG_WARN(BSL_LS_SOC_COMMON,
                     (BSL_META_U(unit,
                                 "soc_reg_write: WARNING: "
                                 "write to 32-bit reg %s with hi order data, 0x%x\n"),
                      SOC_REG_NAME(unit, reg),
                      COMPILER_64_HI(data)));
        }
        SOC_IF_ERROR_RETURN(soc_reg32_write(unit, addr,
                                            COMPILER_64_LO(data)));
    }

    return SOC_E_NONE;
}

/*
 * Write an internal SOC register through S-Channel messaging buffer.
 */
int
_soc_reg32_set(int unit, soc_block_t block, int acc, uint32 addr, uint32 data)
{
    schan_msg_t schan_msg;
    int allow_intr=0;

    /*
     * Setup S-Channel command packet
     *
     * NOTE: the datalen field matters only for the Write Memory and
     * Write Register commands, where it is used only by the CMIC to
     * determine how much data to send, and is in units of bytes.
     */

    schan_msg_clear(&schan_msg);

    soc_schan_header_cmd_set(unit, &schan_msg.header, WRITE_REGISTER_CMD_MSG,
                             block, 0, acc, 4, 0, 0);

    schan_msg.writecmd.address = addr;
    schan_msg.writecmd.data[0] = data;

#ifdef BROADCOM_DEBUG
    if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
        _soc_reg_extended_debug(unit, 32, "write", block, acc, addr, 0, data);
    }
#endif /* BROADCOM_DEBUG */
    _soc_snoop_reg(unit, block, acc, addr,SOC_REG_SNOOP_WRITE, 0,data);

    if(SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /* Write header word + address + data DWORD */
    /* Note: The hardware does not send WRITE_REGISTER_ACK_MSG. */
    
    

    return soc_schan_op(unit, &schan_msg, 3, 0, allow_intr);
}

/*
 * Write an internal SOC register through S-Channel messaging buffer.
 * Checks if the register is 32 or 64 bits.
 */

int
soc_reg_set(int unit, soc_reg_t reg, int port, int index, uint64 data)
{
    uint32 addr;
    int block;
    int pindex = port;
    uint8 acc_type;
    if (!SOC_REG_IS_VALID(unit, reg)) {
        return SOC_E_PARAM;
    }

    if (SOC_REG_IS_ABOVE_64(unit, reg)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_reg_set: "
                              "Use soc_reg_above_64_set \n")));
        
        return SOC_E_FAIL;
    }

    /* if reloading, dont write to register */
    if (SOC_IS_RELOADING(unit))
    {
        return SOC_E_NONE;
    }

    if(SOC_INFO(unit).reg_access.reg64_set) {
        return SOC_INFO(unit).reg_access.reg64_set(unit, reg, port, index, data);
    }
    
    addr = soc_reg_addr_get(unit, reg, port, index, TRUE, &block, &acc_type);

    if (SOC_REG_IS_64(unit, reg)) {
        soc_port_t _port;
        soc_block_types_t regblktype = SOC_REG_INFO(unit, reg).block;
        int blk, bindex;

#if defined(BCM_XGS_SUPPORT)
        if (soc_feature(unit, soc_feature_regs_as_mem)) {
            (void)soc_ser_reg_cache_set(unit, reg, port, index, data);
        }
#endif /* BCM_XGS_SUPPORT */
        if (!soc_feature(unit, soc_feature_new_sbus_format)) {
           return soc_reg_write(unit, reg, addr, data);
        }
        if (SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_PORT) 
#ifdef BCM_PETRA_SUPPORT
           && !SOC_IS_DPP(unit)
#endif /* BCM_PETRA_SUPPORT */
#ifdef BCM_DFE_SUPPORT
           && !SOC_IS_DFE(unit)
#endif /* BCM_DFE_SUPPORT */
#ifdef BCM_BIGMAC_SUPPORT
           && iterative_op_required(reg)
#endif /* BCM_BIGMAC_SUPPORT */
            ) {
            PBMP_HYPLITE_ITER(unit, _port) {
                blk = SOC_PORT_BLOCK(unit, _port);
                bindex = SOC_PORT_BINDEX(unit, _port);
                if ((SOC_BLOCK2SCH(unit, blk) == block) && (bindex == pindex)) {
                    break;
                }
            }         
            if (!IS_HYPLITE_PORT(unit, port)) {
                return _soc_reg64_set(unit, block, acc_type, addr, data);
            } 
#ifdef BCM_BIGMAC_SUPPORT
            else {   
                return soc_reg64_set_iterative(unit, block, acc_type, addr,
                                               port, data);
            }
#endif /* BCM_BIGMAC_SUPPORT */           
        } else {
            return _soc_reg64_set(unit, block, acc_type, addr, data);
        }
    } else {
        uint32 data32;
        if (COMPILER_64_HI(data)) {
            LOG_WARN(BSL_LS_SOC_COMMON,
                     (BSL_META_U(unit,
                                 "soc_reg_set: WARNING: "
                                 "write to 32-bit reg %s with hi order data, 0x%x\n"),
                      SOC_REG_NAME(unit, reg),
                      COMPILER_64_HI(data)));
        }
        data32 = COMPILER_64_LO(data);
#if defined(BCM_XGS_SUPPORT)
        if (soc_feature(unit, soc_feature_regs_as_mem)) {
            (void)soc_ser_reg32_cache_set(unit, reg, port, index, data32);
        }
#endif /* BCM_XGS_SUPPORT */
        if (!soc_feature(unit, soc_feature_new_sbus_format)) {
            return soc_reg32_write(unit, addr, data32);
        }
        return _soc_reg32_set(unit, block, acc_type, addr, data32);
    }

    return SOC_E_NONE;
}

/* Write to h/w - do not update reg cache */
int
soc_reg_set_nocache(int unit, soc_reg_t reg, int port, int index, uint64 data)
{
    uint32 addr;
    int block;
    int pindex = port;
    uint8 acc_type;
    if (!SOC_REG_IS_VALID(unit, reg)) {
        return SOC_E_PARAM;
    }

    if (SOC_REG_IS_ABOVE_64(unit, reg)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_reg_set: "
                              "Use soc_reg_above_64_set \n")));
        
        return SOC_E_FAIL;
    }

    /* if reloading, dont write to register */
    if (SOC_IS_RELOADING(unit))
    {
        return SOC_E_NONE;
    }

    
    addr = soc_reg_addr_get(unit, reg, port, index, TRUE, &block, &acc_type);

    if (SOC_REG_IS_64(unit, reg)) {
        soc_port_t _port;
        soc_block_types_t regblktype = SOC_REG_INFO(unit, reg).block;
        int blk, bindex;

        if (!soc_feature(unit, soc_feature_new_sbus_format)) {
           return soc_reg_write(unit, reg, addr, data);
        }
        if (SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_PORT) 
#ifdef BCM_PETRA_SUPPORT
           && !SOC_IS_DPP(unit)
#endif /* BCM_PETRA_SUPPORT */
#ifdef BCM_DFE_SUPPORT
           && !SOC_IS_DFE(unit)
#endif /* BCM_DFE_SUPPORT */
#ifdef BCM_BIGMAC_SUPPORT
           && iterative_op_required(reg)
#endif /* BCM_BIGMAC_SUPPORT */
            ) {
            PBMP_HYPLITE_ITER(unit, _port) {
                blk = SOC_PORT_BLOCK(unit, _port);
                bindex = SOC_PORT_BINDEX(unit, _port);
                if ((SOC_BLOCK2SCH(unit, blk) == block) && (bindex == pindex)) {
                    break;
                }
            }         
            if (!IS_HYPLITE_PORT(unit, port)) {
                return _soc_reg64_set(unit, block, acc_type, addr, data);
            } 
#ifdef BCM_BIGMAC_SUPPORT
            else {   
                return soc_reg64_set_iterative(unit, block, acc_type, addr,
                                               port, data);
            }
#endif /* BCM_BIGMAC_SUPPORT */           
        } else {
            return _soc_reg64_set(unit, block, acc_type, addr, data);
        }
    } else {
        uint32 data32;
        if (COMPILER_64_HI(data)) {
            LOG_WARN(BSL_LS_SOC_COMMON,
                     (BSL_META_U(unit,
                                 "soc_reg_set: WARNING: "
                                 "write to 32-bit reg %s with hi order data, 0x%x\n"),
                      SOC_REG_NAME(unit, reg),
                      COMPILER_64_HI(data)));
        }
        data32 = COMPILER_64_LO(data);
        if (!soc_feature(unit, soc_feature_new_sbus_format)) {
            return soc_reg32_write(unit, addr, data32);
        }
        return _soc_reg32_set(unit, block, acc_type, addr, data32);
    }

    return SOC_E_NONE;
}

/*
 * Write an internal SOC register through S-Channel messaging buffer.
 * Handle register at any size 
 */

int
soc_reg_above_64_set(int unit, soc_reg_t reg, int port, int index, soc_reg_above_64_val_t data)
{
    uint32 addr;
    int block, reg_size;
    uint8 at; 
    uint64 data64;

    /* if reloading, dont write to register */
    if (SOC_IS_RELOADING(unit))
    {
        return SOC_E_NONE;
    }    
    
    if (!SOC_REG_IS_VALID(unit, reg)) {
        return SOC_E_PARAM;
    }
                                                                
    if(SOC_INFO(unit).reg_access.reg_above64_set) {
        return SOC_INFO(unit).reg_access.reg_above64_set(unit, reg, port, index, data);
    }
    
    if (SOC_REG_IS_ABOVE_64(unit, reg)) 
    {
        reg_size = SOC_REG_ABOVE_64_INFO(unit, reg).size;
        addr = soc_reg_addr_get(unit, reg, port, index, TRUE, &block, &at);
        if (!soc_feature(unit, soc_feature_new_sbus_format)) {
            block = ((addr >> SOC_BLOCK_BP) & 0xf) | (((addr >> SOC_BLOCK_MSB_BP) & 0x3) << 4);
        }

#ifdef BROADCOM_DEBUG
        if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
            _soc_reg_above_64_debug(unit, "set", block, addr, data);
        }
#endif /* BROADCOM_DEBUG */

        return soc_direct_reg_set(unit, block, addr, reg_size, data);
    } 
    else if (SOC_REG_IS_64(unit, reg)) {
        COMPILER_64_SET(data64, data[1], data[0]);
        return soc_reg_set(unit, reg, port, index, data64);
    } 
    else {
        COMPILER_64_SET(data64, 0, data[0]);
        return soc_reg_set(unit, reg, port, index, data64);
    }
}

/*
 * Write an internal SOC register through S-Channel messaging buffer.
 */

int
soc_reg32_write(int unit,
                uint32 addr,
                uint32 data)
{
    schan_msg_t schan_msg;
    int allow_intr=0;
    int dst_blk, src_blk, data_byte_len;
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
#endif

#if defined(BCM_PETRAB_SUPPORT)
    if (SOC_IS_PETRAB(unit)) {
        return soc_dpp_reg32_write(unit, addr, data);
    }
#endif /* BCM_PETRAB_SUPPORT */

#ifdef BROADCOM_DEBUG
    if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
        _soc_reg_debug(unit, 32, "write", addr, 0, data);
    }
#endif /* BROADCOM_DEBUG */
    _soc_snoop_reg(unit, 0, 0, addr,SOC_REG_SNOOP_WRITE, 0, data);

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) &&
        (NULL != SOC_CONTROL(unit)->fschanMutex)) {
        FSCHAN_LOCK(unit);
        soc_pci_write(unit, CMIC_CMCx_FSCHAN_ADDRESS_OFFSET(cmc), addr);
        SOC_IF_ERROR_RETURN(soc_pci_write(unit, 
                            CMIC_CMCx_FSCHAN_DATA32_OFFSET(cmc), 
                            data));
        fschan_wait_idle(unit);
        FSCHAN_UNLOCK(unit);
        return SOC_E_NONE;
    }
#endif /* CMICM */
    /*
     * Setup S-Channel command packet
     *
     * NOTE: the datalen field matters only for the Write Memory and
     * Write Register commands, where it is used only by the CMIC to
     * determine how much data to send, and is in units of bytes.
     */

    schan_msg_clear(&schan_msg);

    dst_blk = ((addr >> SOC_BLOCK_BP) & 0xf) | 
        (((addr >> SOC_BLOCK_MSB_BP) & 0x3) << 4);
    data_byte_len = SOC_IS_XGS12_FABRIC(unit) ? 8 : 4;
#if defined(BCM_SIRIUS_SUPPORT)
    if (SOC_IS_SIRIUS(unit)) {
        src_blk = 0;
    } else
#endif /* BCM_SIRIUS_SUPPORT */
    {
        src_blk = SOC_IS_SHADOW(unit) ?
            0 : SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
    }
    soc_schan_header_cmd_set(unit, &schan_msg.header, WRITE_REGISTER_CMD_MSG,
                             dst_blk, src_blk, 0, data_byte_len, 0, 0);

    schan_msg.writecmd.address = addr;
    schan_msg.writecmd.data[0] = data;

    if(SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /* Write header word + address + data DWORD */
    /* Note: The hardware does not send WRITE_REGISTER_ACK_MSG. */
    
    

    return soc_schan_op(unit, &schan_msg, 3, 0, allow_intr);

}

/*
 * Write an internal SOC register through S-Channel messaging buffer.
 */

int
soc_reg32_set(int unit, soc_reg_t reg, int port, int index, uint32 data)
{
    uint32 addr;
    int block = 0;
    uint8 acc_type;

    /* if reloading, dont write to register */
    if (SOC_IS_RELOADING(unit))
    {
        return SOC_E_NONE;
    }

    if(SOC_INFO(unit).reg_access.reg32_set) {
        return SOC_INFO(unit).reg_access.reg32_set(unit, reg, port, index, data);
    }

    addr = soc_reg_addr_get(unit, reg, port, index, TRUE, &block, &acc_type);
    if (SOC_REG_IS_ABOVE_32(unit, reg)) {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is not 32 bit\n"), soc_reg_name[reg]));
#endif
#endif

    }
    assert(!SOC_REG_IS_ABOVE_32(unit, reg));
#if defined(BCM_XGS_SUPPORT)
    if (soc_feature(unit, soc_feature_regs_as_mem)) {
        (void)soc_ser_reg32_cache_set(unit, reg, port, index, data);
    }
#endif /* BCM_XGS_SUPPORT */
    if (!soc_feature(unit, soc_feature_new_sbus_format)) {
        return soc_reg32_write(unit, addr, data);
    }
    return _soc_reg32_set(unit, block, acc_type, addr, data);
}

/*
 * Write an internal 64-bit SOC register through S-Channel messaging buffer.
 */
int
soc_reg64_write(int unit,
                uint32 addr,
                uint64 data)
{
    schan_msg_t schan_msg;
    int allow_intr=0;
    int dst_blk, src_blk;
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
#endif

#if defined(BCM_PETRAB_SUPPORT)
    if (SOC_IS_PETRAB(unit)) {
        return soc_dpp_reg64_write(unit, addr, data);
    }
#endif /* BCM_PETRAB_SUPPORT */



#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) &&
        (NULL != SOC_CONTROL(unit)->fschanMutex)) {
        FSCHAN_LOCK(unit);
        soc_pci_write(unit, CMIC_CMCx_FSCHAN_ADDRESS_OFFSET(cmc), addr);
       SOC_IF_ERROR_RETURN(soc_pci_write(unit, 
                                          CMIC_CMCx_FSCHAN_DATA64_HI_OFFSET(cmc), 
                                          COMPILER_64_HI(data)));
        SOC_IF_ERROR_RETURN(soc_pci_write(unit, 
                                          CMIC_CMCx_FSCHAN_DATA64_LO_OFFSET(cmc), 
                                          COMPILER_64_LO(data)));

        fschan_wait_idle(unit);
        FSCHAN_UNLOCK(unit);
        return SOC_E_NONE;
    }
#endif /* CMICM */
    /*
     * Setup S-Channel command packet
     *
     * NOTE: the datalen field matters only for the Write Memory and
     * Write Register commands, where it is used only by the CMIC to
     * determine how much data to send, and is in units of bytes.
     */

    schan_msg_clear(&schan_msg);

    dst_blk = ((addr >> SOC_BLOCK_BP) & 0xf) | 
        (((addr >> SOC_BLOCK_MSB_BP) & 0x3) << 4);
#if defined(BCM_SIRIUS_SUPPORT)
    if (SOC_IS_SIRIUS(unit)) {
        src_blk = 0;
    } else
#endif /* BCM_SIRIUS_SUPPORT */
    {
        src_blk = SOC_IS_SHADOW(unit) ?
            0 : SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
    }
    soc_schan_header_cmd_set(unit, &schan_msg.header, WRITE_REGISTER_CMD_MSG,
                             dst_blk, src_blk, 0, 8, 0, 0);

    schan_msg.writecmd.address = addr;
    schan_msg.writecmd.data[0] = COMPILER_64_LO(data);
    schan_msg.writecmd.data[1] = COMPILER_64_HI(data);

#ifdef BROADCOM_DEBUG
    if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
        _soc_reg_debug(unit, 64, "write", addr,
                       schan_msg.writecmd.data[1],
                       schan_msg.writecmd.data[0]);
    }
#endif /* BROADCOM_DEBUG */
    _soc_snoop_reg(unit, 0, 0, addr,SOC_REG_SNOOP_WRITE, 
                   schan_msg.writecmd.data[1],schan_msg.writecmd.data[0]);

    if(SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /* Write header word + address + 2*data DWORD */
    /* Note: The hardware does not send WRITE_REGISTER_ACK_MSG. */
    
    

    return soc_schan_op(unit, &schan_msg, 4, 0, allow_intr);

}

#if defined(BCM_PETRAB_SUPPORT)
uint32
soc_dpp_reg_addr(int unit, soc_reg_t reg, int port, int index)
{
    uint32  base;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    base = SOC_REG_INFO(unit, reg).offset;
    

    return base;
}
#endif /* defined(BCM_PETRAB_SUPPORT) */

/*
 * Write an internal 64-bit SOC register through S-Channel messaging buffer.
 */
int
soc_reg64_set(int unit, soc_reg_t reg, int port, int index, uint64 data)
{
    uint32 addr;
    int block = 0;
    uint8 acc_type;
    addr = soc_reg_addr_get(unit, reg, port, index, TRUE, &block, &acc_type);
    assert(SOC_REG_IS_64(unit, reg));
#if defined(BCM_XGS_SUPPORT)
    if (soc_feature(unit, soc_feature_regs_as_mem)) {
        (void)soc_ser_reg_cache_set(unit, reg, port, index, data);
    }
#endif /* BCM_XGS_SUPPORT */
    if (!soc_feature(unit, soc_feature_new_sbus_format)) {
        return soc_reg64_write(unit, addr, data);
    }
    return _soc_reg64_set(unit, block, acc_type, addr, data);
}

/*
 * Write internal register for a group of ports.
 * The specified register must be a port reg (type soc_portreg).
 */

int
soc_reg_write_ports(int unit,
                    soc_reg_t reg,
                    pbmp_t pbmp,
                    uint32 value)
{
    soc_port_t        port;
    soc_block_t       ptype; 
    soc_block_types_t rtype;

    /* assert(reg is a port register) */
    if (!SOC_REG_IS_VALID(unit, reg) ||
        SOC_REG_INFO(unit, reg).regtype != soc_portreg) {
        return SOC_E_UNAVAIL;
    }

    rtype = SOC_REG_INFO(unit, reg).block;

    /*
     * each port block type must match one of the register block types
     * or the register block type can be the MMU
     */
    PBMP_ITER(pbmp, port) {
        ptype = SOC_PORT_TYPE(unit, port);
        if (SOC_BLOCK_IN_LIST(rtype, ptype) || SOC_BLOCK_IN_LIST(rtype, SOC_BLK_MMU)) {
            if (soc_feature(unit, soc_feature_new_sbus_format)) {
                SOC_IF_ERROR_RETURN(soc_reg32_set(unit, reg,
                                                  port, 0, value));
            } else {
#if defined(BCM_XGS_SUPPORT)
                if (soc_feature(unit, soc_feature_regs_as_mem)) {
                    (void)soc_ser_reg32_cache_set(unit, reg, port, 0, value);
                }
#endif /* BCM_XGS_SUPPORT */
                SOC_IF_ERROR_RETURN(soc_reg32_write(unit,
                                    soc_reg_addr(unit, reg, port, 0), value));
            }
        }
    }
    return SOC_E_NONE;
}

/*
 * Write internal register for a block or group of blocks.
 * The specified register must be a generic reg (type soc_genreg).
 *
 * This routine will write to all possible blocks for the given
 * register.
 */
int
soc_reg64_write_all_blocks(int unit,
                           soc_reg_t reg,
                           uint64 value)
{
    int               blk, port;
    soc_block_types_t rtype;

    /* assert(reg is not a port or cos register) */
    if (!SOC_REG_IS_VALID(unit, reg) ||
        SOC_REG_INFO(unit, reg).regtype != soc_genreg) {
        return SOC_E_UNAVAIL;
    }

    rtype = SOC_REG_INFO(unit, reg).block;

    SOC_BLOCKS_ITER(unit, blk, rtype) {
        port = SOC_BLOCK_PORT(unit, blk);
#if defined(BCM_XGS_SUPPORT)
        if (soc_feature(unit, soc_feature_regs_as_mem)) {
            (void)soc_ser_reg_cache_set(unit, reg, port, 0, value);
        }
#endif /* BCM_XGS_SUPPORT */        
        if (soc_feature(unit, soc_feature_new_sbus_format)) {
            SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, value));
        } else {
            SOC_IF_ERROR_RETURN(soc_reg_write(unit, reg,
                                soc_reg_addr(unit, reg, port, 0), value));
        }
    }
    return SOC_E_NONE;
}

/*
 * Write internal register for a block or group of blocks.
 * The specified register must be a generic reg (type soc_genreg).
 *
 * This routine will write to all possible blocks for the given
 * register.
 */
int
soc_reg_write_all_blocks(int unit,
                         soc_reg_t reg,
                         uint32 value)
{
    uint64 val64;

    if (!SOC_REG_IS_VALID(unit, reg)) {
        return SOC_E_PARAM;
    }

    COMPILER_64_SET(val64, 0, value);
    return soc_reg64_write_all_blocks(unit, reg, val64);
}

/*
 * Read a general register from any block that has a copy
 */
int
soc_reg64_read_any_block(int unit,
                         soc_reg_t reg,
                         uint64 *datap)
{
    int               blk, port;
    soc_block_types_t rtype;

    /* assert(reg is not a port or cos register) */
    if (!SOC_REG_IS_VALID(unit, reg) ||
        SOC_REG_INFO(unit, reg).regtype != soc_genreg) {
        return SOC_E_UNAVAIL;
    }

    rtype = SOC_REG_INFO(unit, reg).block;
    SOC_BLOCKS_ITER(unit, blk, rtype) {
        port = SOC_BLOCK_PORT(unit, blk);
        if (soc_feature(unit, soc_feature_new_sbus_format)) {
            SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, datap));
        } else {
            SOC_IF_ERROR_RETURN
                (soc_reg_read(unit, reg, soc_reg_addr(unit, reg, port, 0),
                              datap));
        }
        break;
    }
    return SOC_E_NONE;
}

/*
 * Read a general register from any block that has a copy
 */
int
soc_reg_read_any_block(int unit,
                     soc_reg_t reg,
                     uint32 *datap)
{
    uint64 val64;

    SOC_IF_ERROR_RETURN(soc_reg64_read_any_block(unit, reg, &val64));
    COMPILER_64_TO_32_LO(*datap, val64);

    return SOC_E_NONE;
}

/****************************************************************
 * Register field manipulation functions
 ****************************************************************/

/* Define a macro so the assertion printout is informative. */
#define        REG_FIELD_IS_VALID        finfop

/*
 * Function:     soc_reg_field_length
 * Purpose:      Return the length of a register field in bits.
 *               Value is 0 if field is not found.
 * Returns:      bits in field
 */
int
soc_reg_field_length(int unit, soc_reg_t reg, soc_field_t field)
{
    soc_field_info_t *finfop;

    if (!SOC_REG_IS_VALID(unit, reg)) {
        return 0;
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
    if (finfop == NULL) {
        return 0;
    }
    return finfop->len;
}

/*
 * Function:     soc_reg_field_valid
 * Purpose:      Determine if a field in a register is valid.
 * Returns:      Returns TRUE  if field is found.
 *               Returns FALSE if field is not found.
 */
int
soc_reg_field_valid(int unit, soc_reg_t reg, soc_field_t field)
{
    soc_field_info_t *finfop;

    if (!SOC_REG_IS_VALID(unit, reg)) {
        return FALSE;
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
    return (finfop != NULL);
}


/*
 * Function:     soc_reg_field_get
 * Purpose:      Get the value of a field from a register
 * Parameters:
 * Returns:      Value of field
 */
uint32
soc_reg_field_get(int unit, soc_reg_t reg, uint32 regval, soc_field_t field)
{
    soc_field_info_t *finfop;
    uint32           val;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);

    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    }

    /*
     * COVERITY
     *
     * assert validates the input for NULL
     */
    /* coverity[var_deref_op : FALSE] */
    val = regval >> finfop->bp;
    if (finfop->len < 32) {
        return val & ((1 << finfop->len) - 1);
    } else {
        return val;
    }
}

/*
 * Function:     soc_reg64_field_get
 * Purpose:      Get the value of a field from a 64-bit register
 * Parameters:
 * Returns:      Value of field (64 bits)
 */
uint64
soc_reg64_field_get(int unit, soc_reg_t reg, uint64 regval, soc_field_t field)
{
    soc_field_info_t *finfop;
    uint64           mask;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    }

    /*
     * COVERITY
     *
     * assert validates the input for NULL
     */
    /* coverity[var_deref_op : FALSE] */
    COMPILER_64_MASK_CREATE(mask, finfop->len, 0);
    COMPILER_64_SHR(regval, finfop->bp);
    COMPILER_64_AND(regval, mask);

    return regval;
}

/* 
 * Function:     soc_reg64_field32_get
 * Purpose:      Get the value of a field from a 64-bit register
 * Parameters:
 * Returns:      Value of field (32 bits)
 */
uint32
soc_reg64_field32_get(int unit, soc_reg_t reg, uint64 regval,
                      soc_field_t field)
{
    soc_field_info_t *finfop;
    uint32           val32;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    }

    /*
     * COVERITY
     *
     * assert validates the input for NULL
     */
    /* coverity[var_deref_op : FALSE] */
    COMPILER_64_SHR(regval, finfop->bp);
    COMPILER_64_TO_32_LO(val32, regval);
    if (finfop->len < 32) {
        return val32 & ((1 << finfop->len) - 1);
    } else {
        return val32;
    }
}

/*
 * Function:     soc_reg_above_64_field_get
 * Purpose:      Get the value of a field from a register
 * Parameters:
 * Returns:      Value of field
 */
void
soc_reg_above_64_field_get(int unit, soc_reg_t reg, soc_reg_above_64_val_t regval, 
                           soc_field_t field, soc_reg_above_64_val_t field_val)
{
    soc_field_info_t *finfop;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);

    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    }
    
    
    SOC_REG_ABOVE_64_CLEAR(field_val);
    /*
     * COVERITY
     *
     * assert validates the input for NULL
     */
    /* coverity[var_deref_op : FALSE] */
    SHR_BITCOPY_RANGE(field_val, 0, regval, finfop->bp, finfop->len);
    
}
/*
 * Function:     soc_reg_above_64_field_read
 * Purpose:      Read a register of any size, and get the value of a field sized up to any size.
 * Parameters:
 * Returns:      Value of field (32 bits)
 */
int
soc_reg_above_64_field_read(int unit, soc_reg_t reg, soc_port_t port, int index, soc_field_t field, soc_reg_above_64_val_t out_field_val)
{   
    int rc = SOC_E_NONE;
    soc_reg_above_64_val_t data;
    SOC_REG_ABOVE_64_CLEAR(data);
    rc = soc_reg_above_64_get(unit, reg, port, index, data);
    if (rc != SOC_E_NONE){
        return rc;
    }
    soc_reg_above_64_field_get(unit, reg, data, field, out_field_val);
    return rc;
}
/* 
 * Function:     soc_reg_above_64_field32_get
 * Purpose:      Get the value of a field sized up to 32 bit from a register of any size
 * Parameters:
 * Returns:      Value of field (32 bits)
 */
uint32
soc_reg_above_64_field32_get(int unit, soc_reg_t reg, soc_reg_above_64_val_t regval, 
                           soc_field_t field)
{
    soc_field_info_t *finfop;
    uint32 field_val = 0;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);

    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    } else if (finfop->len > 32) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s has a size of %u bits which is greater than 32\n"),
                 soc_reg_name[reg], soc_fieldnames[field], (unsigned)finfop->len));
#endif
        assert(0);
    } else {
    
        SHR_BITCOPY_RANGE(&field_val, 0, regval, finfop->bp, finfop->len); /* get the field value */

    }
    return field_val;
}
/*
 * Function:     soc_reg_above_64_field32_read
 * Purpose:      Read a register of any size, and get the value of a field sized up to 32 bit from it.
 * Parameters:
 * Returns:      Value of field (32 bits)
 */

int
soc_reg_above_64_field32_read(int unit, soc_reg_t reg, soc_port_t port, int index, soc_field_t field, uint32* out_field_val)
{   
    int rc = SOC_E_NONE;
    soc_reg_above_64_val_t data;
    SOC_REG_ABOVE_64_CLEAR(data);
    rc = soc_reg_above_64_get(unit, reg, port, index, data);
    if (rc != SOC_E_NONE){
        return rc;
    }
    *out_field_val = soc_reg_above_64_field32_get(unit,reg, data, field);
    return rc;
}
/* 
 * Function:     soc_reg_above_64_field64_get
 * Purpose:      Get the value of a field sized up to 32 bit from a register of any size
 * Parameters:
 * Returns:      Value of field (32 bits)
 */

uint64
soc_reg_above_64_field64_get(int unit, soc_reg_t reg, soc_reg_above_64_val_t regval, 
                           soc_field_t field)
{
    soc_field_info_t *finfop;
    uint64 fieldval;
    
    COMPILER_64_ZERO(fieldval);
    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);

    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    } else if (finfop->len > 64) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s has a size of %u bits which is greater than 32\n"),
                 soc_reg_name[reg], soc_fieldnames[field], (unsigned)finfop->len));
#endif
        assert(0);
    } else {
        uint32 low = 0, hi = 0;
        if (finfop->len > 32) {
            SHR_BITCOPY_RANGE(&low, 0, regval, finfop->bp, 32); /* get the field value lsb word */
            SHR_BITCOPY_RANGE(&hi, 0, regval, finfop->bp + 32, finfop->len - 32); /* get the field value msb word */
        } else {
            SHR_BITCOPY_RANGE(&low, 0, regval, finfop->bp, finfop->len); /* get the field value */
        }
        COMPILER_64_SET(fieldval, hi, low);
    }
    return fieldval;
}
/*
 * Function:     soc_reg_above_64_field64_read
 * Purpose:      Read a register of any size, and get the value of a field sized up to 64 bit from it.
 * Parameters:
 * Returns:      Value of field (32 bits)
 */
int
soc_reg_above_64_field64_read(int unit, soc_reg_t reg, soc_port_t port, int index, soc_field_t field, uint64* out_field_val)
{   
    int rc = SOC_E_NONE;
    soc_reg_above_64_val_t data;
    SOC_REG_ABOVE_64_CLEAR(data);
    rc = soc_reg_above_64_get(unit, reg, port, index, data);
    if (rc != SOC_E_NONE){
        return rc;
    }
    *out_field_val = soc_reg_above_64_field64_get(unit,reg, data, field);
    return rc;
}

/* Define a macro so the assertion printout is informative. */
#define VALUE_TOO_BIG_FOR_FIELD                ((value & ~mask) != 0)

/*
 * Function:     soc_reg_field_validate
 * Purpose:      Validate the value of a register's field.
 * Parameters:
 * Returns:      SOC_E_XXX
 */
int
soc_reg_field_validate(int unit, soc_reg_t reg, soc_field_t field, uint32 value)
{
    soc_field_info_t *finfop;
    uint32           mask;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
                   
    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    }

    /*
     * COVERITY
     *
     * assert validates the input for NULL
     */
    /* coverity[var_deref_op : FALSE] */
    if (finfop->len < 32) {
      mask = (1 << finfop->len) - 1; 
      if  (VALUE_TOO_BIG_FOR_FIELD) {
            return SOC_E_PARAM;
      }
     }
    
    return SOC_E_NONE;
}

/*
 * Function:     soc_reg_field_set
 * Purpose:      Set the value of a register's field.
 * Parameters:
 * Returns:      void
 */
void
soc_reg_field_set(int unit, soc_reg_t reg, uint32 *regval,
                  soc_field_t field, uint32 value)
{
    soc_field_info_t *finfop;
    uint32           mask;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    }

    if (finfop->len < 32) {
        mask = (1 << finfop->len) - 1;
   if  (VALUE_TOO_BIG_FOR_FIELD) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is too big\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(!VALUE_TOO_BIG_FOR_FIELD);
   }
    } else {
        mask = -1;
    }

    *regval = (*regval & ~(mask << finfop->bp)) | value << finfop->bp;
}

/*
 * Function:     soc_reg_field_set
 * Purpose:      Set the value of a register's field.
 * Parameters:
 * Returns:      void
 */
void
soc_reg_above_64_field_set(int unit, soc_reg_t reg, soc_reg_above_64_val_t regval,
                            soc_field_t field, CONST soc_reg_above_64_val_t value)
{
    soc_field_info_t *finfop;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    }
    { /* Check if the field value will fit into the field: Verify that value's bits that do not fit in the field are all zeroes. */

        /*
         * COVERITY
         *
         * assert validates the input for NULL
         */
        /* coverity[var_deref_op : FALSE] */
        unsigned msb_bits = finfop->len % 32; /* Bits in msb word of field value */
        unsigned idx      = finfop->len / 32; /* word Iteration index, starts with msb word of field. */

        if (msb_bits) { /* if the msb vaule word has left over bits unused */
            assert (!(value[idx] & (((uint32)0xffffffff) << msb_bits))); /* verify the left over bits are zeros */
            ++idx;
        }
        for (; idx < SOC_REG_ABOVE_64_MAX_SIZE_U32; ++idx) { /* verify the remaining words are zeros */
            assert (!(value[idx]));
        }
    }
    
    SHR_BITCOPY_RANGE(regval, finfop->bp, value, 0, finfop->len);

}

/* 
 * Function:     soc_reg_above_64_field32_set
 * Purpose:      Set the value of a register's field; field must be <= 32 bits, any register size supported
 * Parameters:
 * Returns:      void
 */
void
soc_reg_above_64_field32_set(int unit, soc_reg_t reg, soc_reg_above_64_val_t regval,
                            soc_field_t field, uint32 value)
{
    soc_field_info_t *finfop;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    /* Check if the field value will fit into the field: Verify that value's bits that do not fit in the field are all zeroes. */
    } else if (finfop->len > 32) {
        SHR_BITCLR_RANGE(regval, finfop->bp + 32, finfop->len - 32);
        SHR_BITCOPY_RANGE(regval, finfop->bp, &value, 0, 32);
    } else {
        if (finfop->len < 32 && value >= (((uint32)1) << finfop->len)) {
#if !defined(SOC_NO_NAMES)
            LOG_CLI((BSL_META_U(unit,
                                "reg %s field %s is too small for value 0x%lx\n"),
                     soc_reg_name[reg], soc_fieldnames[field],(unsigned long)value));
#endif
            assert (0);
        }
        SHR_BITCOPY_RANGE(regval, finfop->bp, &value, 0, finfop->len);
    }

}


/* 
 * Function:     soc_reg_above_64_field64_set
 * Purpose:      Set the value of a register's field; field must be <= 64 bits, any register size supported
 * Parameters:
 * Returns:      void
 */
void
soc_reg_above_64_field64_set(int unit, soc_reg_t reg, soc_reg_above_64_val_t regval,
                            soc_field_t field, uint64 value)
{
    soc_field_info_t *finfop;
    uint32 value32;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    /* Check if the field value will fit into the field: Verify that value's bits that do not fit in the field are all zeroes. */
    } else if (finfop->len > 64) {
        SHR_BITCLR_RANGE(regval, finfop->bp + 64, finfop->len - 64);

        value32 = COMPILER_64_LO(value);
        SHR_BITCOPY_RANGE(regval, finfop->bp, &value32, 0, 32);

        value32 = COMPILER_64_HI(value);
        SHR_BITCOPY_RANGE(regval, finfop->bp + 32, &value32, 0, 32);

    } else if (finfop->len <= 32 ) {
        if (finfop->len < 32 && COMPILER_64_LO(value) >= (((uint32)1) << finfop->len)) {
#if !defined(SOC_NO_NAMES)
            LOG_CLI((BSL_META_U(unit,
                                "reg %s field %s is too small for value 0x%lx\n"),
                     soc_reg_name[reg], soc_fieldnames[field],(unsigned long)COMPILER_64_LO(value)));
#endif
            assert (0);
        }
        value32 = COMPILER_64_LO(value);
        SHR_BITCOPY_RANGE(regval, finfop->bp, &value32, 0, finfop->len);
    } else { /*32<field lengh<=64*/
        if (finfop->len < 64 && COMPILER_64_HI(value) >= (((uint32)1) << (finfop->len - 32))) {
#if !defined(SOC_NO_NAMES)
            LOG_CLI((BSL_META_U(unit,
                                "reg %s field %s is too small for value 0x%lx\n"),
                     soc_reg_name[reg], soc_fieldnames[field],(unsigned long)COMPILER_64_HI(value)));
#endif
            assert (0);
        }
        value32 = COMPILER_64_LO(value);
        SHR_BITCOPY_RANGE(regval, finfop->bp, &value32, 0, 32);

        value32 = COMPILER_64_HI(value);
        SHR_BITCOPY_RANGE(regval, finfop->bp + 32, &value32, 0, finfop->len - 32);

    }

}
#define        VALUE_TOO_BIG_FOR_FIELD64(mask)        (!COMPILER_64_IS_ZERO(mask))

/*
 * Function:     soc_reg64_field_validate
 * Purpose:      Validate the value of a register's field.
 * Parameters:
 * Returns:      SOC_E_XXX
 */
int
soc_reg64_field_validate(int unit, soc_reg_t reg, soc_field_t field, uint64 value)
{
    soc_field_info_t *finfop;
    uint64           mask;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
                   
    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    }

    /*
     * COVERITY
     *
     * assert validates the input for NULL
     */
    /* coverity[var_deref_op : FALSE] */
    if (finfop->len < 64) {
        COMPILER_64_ZERO(mask);
        COMPILER_64_ADD_32(mask, 1);
        COMPILER_64_SHL(mask, finfop->len);
        COMPILER_64_SUB_32(mask, 1);
        COMPILER_64_NOT(mask);
        COMPILER_64_AND(mask, value);
        if(VALUE_TOO_BIG_FOR_FIELD64(mask))
          return SOC_E_PARAM;

    } 
    
    return SOC_E_NONE;
}

/*
 * Function:     soc_reg64_field_set
 * Purpose:      Set the value of a register's field.
 * Parameters:
 * Returns:      void
 */
void
soc_reg64_field_set(int unit, soc_reg_t reg, uint64 *regval,
                    soc_field_t field, uint64 value)
{
    soc_field_info_t *finfop;
    uint64           mask, tmp;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    }

    /*
     * COVERITY
     *
     * assert validates the input for NULL
     */
    /* coverity[var_deref_op : FALSE] */
    if (finfop->len < 64) {
        COMPILER_64_SET(mask, 0, 1);
        COMPILER_64_SHL(mask, finfop->len);
        COMPILER_64_SUB_32(mask, 1);
#ifndef NDEBUG
        /* assert(!VALUE_TOO_BIG_FOR_FIELD); */
        tmp = mask;
        COMPILER_64_NOT(tmp);
        COMPILER_64_AND(tmp, value);
        assert(!VALUE_TOO_BIG_FOR_FIELD64(tmp));
#endif
    } else {
        COMPILER_64_SET(mask, -1, -1);
    }

    /* *regval = (*regval & ~(mask << finfop->bp)) | value << finfop->bp; */
    tmp = mask;
    COMPILER_64_SHL(tmp, finfop->bp);
    COMPILER_64_NOT(tmp);
    COMPILER_64_AND(*regval, tmp);
    COMPILER_64_SHL(value, finfop->bp);
    COMPILER_64_OR(*regval, value);
}

/* 
 * Function:     soc_reg64_field32_set
 * Purpose:      Set the value of a register's field; field must be < 32 bits
 * Parameters:
 * Returns:      void
 */
void
soc_reg64_field32_set(int unit, soc_reg_t reg, uint64 *regval,
                      soc_field_t field, uint32 value)
{
    soc_field_info_t *finfop;
    uint64           mask, tmp;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    SOC_FIND_FIELD(field,
                   SOC_REG_INFO(unit, reg).fields,
                   SOC_REG_INFO(unit, reg).nFields,
                   finfop);
    if (finfop == NULL) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s field %s is invalid\n"),
                 soc_reg_name[reg], soc_fieldnames[field]));
#endif
        assert(finfop);
    }

    /*
     * COVERITY
     *
     * assert validates the input for NULL
     */
    /* coverity[var_deref_op : FALSE] */
    if (finfop->len < 64) {
        COMPILER_64_SET(mask, 0, 1);
        COMPILER_64_SHL(mask, finfop->len);
        COMPILER_64_SUB_32(mask, 1);
    } else {
        COMPILER_64_SET(mask, -1, -1);
    }

    /* *regval = (*regval & ~(mask << finfop->bp)) | value << finfop->bp; */
    COMPILER_64_SHL(mask, finfop->bp);
    COMPILER_64_NOT(mask);
    COMPILER_64_AND(*regval, mask);
    if (value != 0) {
        COMPILER_64_SET(tmp, 0, value);
        COMPILER_64_SHL(tmp, finfop->bp);
        COMPILER_64_OR(*regval, tmp);
    }
}

/*
 * Function:    soc_reg_addr
 * Purpose:     calculate the address of a register
 * Parameters:
 *              unit  switch unit
 *              reg   register number
 *              port  port number or REG_PORT_ANY
 *              index array index (or cos number)
 * Returns:     register address suitable for soc_reg_read and friends
 * Notes:       the block number to access is determined by the register
 *              and the port number
 *
 * cpureg       00SSSSSS 00000000 0000RRRR RRRRRRRR
 * genreg       00SSSSSS BBBB1000 0000RRRR RRRRRRRR
 * portreg      00SSSSSS BBBB00PP PPPPRRRR RRRRRRRR
 * cosreg       00SSSSSS BBBB01CC CCCCRRRR RRRRRRRR
 *
 * all regs of bcm88230
 *              00000000 00001000 0000RRRR RRRRRRRR
 *
 * where        B+ is the 4 bit block number
 *              P+ is the 6 bit port number (within a block or chip wide)
 *              C+ is the 6 bit class of service
 *              R+ is the 12 bit register number
 *              S+ is the 6 bit Pipe stage
 */
uint32
soc_reg_addr(int unit, soc_reg_t reg, int port, int index)
{
    uint32            base;   /* base address from reg_info */
    int               block = -1;  /* block number */
    int               pindex = -1; /* register port/cos field */
    int               gransh; /* index granularity shift */
    soc_block_types_t regblktype;
    soc_block_t       portblktype;
    int               phy_port;
    int               instance_mask = 0;
     
#if defined(BCM_PETRAB_SUPPORT)
    if (SOC_IS_PETRAB(unit)) {
        return soc_dpp_reg_addr(unit, reg, port, index);
    }
#endif /* BCM_PETRAB_SUPPORT */

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

#define SOC_REG_ADDR_INVALID_PORT 0 /* for asserts */


#ifdef  BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        portblktype = SOC_BLK_SBX_PORT;
    } 
    else 
#endif
    {
        portblktype = SOC_BLK_PORT;
    }

    regblktype = SOC_REG_INFO(unit, reg).block;
    if(REG_PORT_ANY != port) {
        instance_mask = port & SOC_REG_ADDR_INSTANCE_MASK;
        port &= (~SOC_REG_ADDR_INSTANCE_MASK);
    }
    

    if(!instance_mask) {
        if (port >= 0) {
            if (SOC_BLOCK_IN_LIST(regblktype, portblktype)) {
                assert(SOC_PORT_VALID(unit, port));
                if (soc_feature(unit, soc_feature_logical_port_num)) {
                    /*
                     * COVERITY
                     *
                     * assert validates the port
                     */
                    /* coverity[overrun-local : FALSE] */
                    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
                } else {
                    phy_port = port;
                }
                block = SOC_PORT_BLOCK(unit, phy_port);
                pindex = SOC_PORT_BINDEX(unit, phy_port);
            } else {
                block = pindex = -1; /* multiple non-port block */
            }
        } else if (port == REG_PORT_ANY) {
            block = pindex = -1;
            if(soc_portreg == SOC_REG_INFO(unit, reg).regtype 
#ifdef  BCM_SIRIUS_SUPPORT
           || SOC_IS_SIRIUS(unit)
#endif
           ) {
                PBMP_ALL_ITER(unit, port) { /* try enabled ports */
                    if (soc_feature(unit, soc_feature_logical_port_num)) {
                        phy_port = SOC_INFO(unit).port_l2p_mapping[port];
                    } else {
                        phy_port = port;
                    }
                    block = SOC_PORT_BLOCK(unit, phy_port);
                    pindex = SOC_PORT_BINDEX(unit, phy_port);
                    if (SOC_BLOCK_IN_LIST(regblktype, portblktype)) { /* match reg type */
                        if (SOC_BLOCK_IS_TYPE(unit, block, regblktype)) {
                            break;
                        }
                        block = -1;
                    } else { /* match any port */
                        break;
                    }
                }
                if (block < 0) {
                    assert(SOC_REG_ADDR_INVALID_PORT); /* invalid port */
                }
            }
        } else {
        port &= ~SOC_REG_ADDR_INSTANCE_MASK;
        block = pindex = -1;
        }
    }

#ifdef  BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        if (!SOC_BLOCK_IN_LIST(regblktype, portblktype)) {
            switch (REG_FIRST_BLK_TYPE(regblktype)) {
            case SOC_BLK_CMIC:
                block = CMIC_BLOCK(unit);
                break;
            case SOC_BLK_BP:
                block = BP_BLOCK(unit);
                break;
            case SOC_BLK_CI:
                if (port >= 10) {
                    assert(SOC_REG_ADDR_INVALID_PORT); /* invalid port */
                } else {
                    block = CI_BLOCK(unit, port);
                }
                break;
            case SOC_BLK_CS:
                block = CS_BLOCK(unit);
                break;
            case SOC_BLK_EB:
                block = EB_BLOCK(unit);
                break;
            case SOC_BLK_EP:
                block = EP_BLOCK(unit);
                break;
            case SOC_BLK_ES:
                block = ES_BLOCK(unit);
                break;
            case SOC_BLK_FD:
                block = FD_BLOCK(unit);
                break;
            case SOC_BLK_FF:
                block = FF_BLOCK(unit);
                break;
            case SOC_BLK_FR:
                block = FR_BLOCK(unit);
                break;
            case SOC_BLK_TX:
                block = TX_BLOCK(unit);
                break;
            case SOC_BLK_QMA:
                block = QMA_BLOCK(unit);
                break;
            case SOC_BLK_QMB:
                block = QMB_BLOCK(unit);
                break;
            case SOC_BLK_QMC:
                block = QMC_BLOCK(unit);
                break;
            case SOC_BLK_QSA:
                block = QSA_BLOCK(unit);
                break;
            case SOC_BLK_QSB:
                block = QSB_BLOCK(unit);
                break;
            case SOC_BLK_RB:
                block = RB_BLOCK(unit);
                break;
            case SOC_BLK_SC_TOP:
                block = SC_TOP_BLOCK(unit);
                break;
            case SOC_BLK_SF_TOP:
                block = SF_TOP_BLOCK(unit);
                break;
            case SOC_BLK_TS:
                block = TS_BLOCK(unit);
                break;
            case SOC_BLK_OTPC:
                block = OTPC_BLOCK(unit);
                break;
            default:
                block = -1; /* unknown non-port block */
                break;
            }
        }
    } else {
#endif

        if (REG_PORT_ANY == port ||instance_mask || !SOC_BLOCK_IN_LIST(regblktype, portblktype)) {
            switch (REG_FIRST_BLK_TYPE(regblktype)) {
            case SOC_BLK_ARL:
                block = ARL_BLOCK(unit);
                break;
            case SOC_BLK_IPIPE:
                block = IPIPE_BLOCK(unit);
                break;
            case SOC_BLK_IPIPE_HI:
                block = IPIPE_HI_BLOCK(unit);
                break;
            case SOC_BLK_EPIPE:
                block = EPIPE_BLOCK(unit);
                break;
            case SOC_BLK_EPIPE_HI:
                block = EPIPE_HI_BLOCK(unit);
                break;
            case SOC_BLK_IGR:
                block = IGR_BLOCK(unit);
                break;
            case SOC_BLK_EGR:
                block = EGR_BLOCK(unit);
                break;
            case SOC_BLK_BSE:
                block = BSE_BLOCK(unit);
                break;
            case SOC_BLK_CSE:
                block = CSE_BLOCK(unit);
                break;
            case SOC_BLK_HSE:
                block = HSE_BLOCK(unit);
                break;
            case SOC_BLK_BSAFE:
                block = BSAFE_BLOCK(unit);
                break;
            case SOC_BLK_OTPC:
                block = OTPC_BLOCK(unit);
                break;
            case SOC_BLK_MMU:
                block = MMU_BLOCK(unit);
                break;
            case SOC_BLK_MCU:
                block = MCU_BLOCK(unit);
                break;
            case SOC_BLK_CMIC:
                block = CMIC_BLOCK(unit);
                break;
            case SOC_BLK_IPROC:
                block = IPROC_BLOCK(unit);
                break;
            case SOC_BLK_ESM:
                block = ESM_BLOCK(unit);
                break;
            case SOC_BLK_PORT_GROUP4:
                block = PG4_BLOCK(unit, port);
                break;
            case SOC_BLK_PORT_GROUP5:
                block = PG5_BLOCK(unit, port);
                break;
            case SOC_BLK_TOP:
                block = TOP_BLOCK(unit);
                break;
            case SOC_BLK_LLS:
                block = LLS_BLOCK(unit);
                break;
            case SOC_BLK_CES:
                block = CES_BLOCK(unit);
                break;
            case SOC_BLK_CI:
                if (port >= 3) {
                    assert(SOC_REG_ADDR_INVALID_PORT); /* invalid instance */
                } else {
                    block = CI_BLOCK(unit, port);
                }
                break;
            case SOC_BLK_IL:
                if (SOC_IS_SHADOW(unit)) {
                    if (port == 9) {
                        block = IL0_BLOCK(unit);
                    } else if (port == 13) {
                        block = IL1_BLOCK(unit);
                    }
                    pindex = 0;
                }
                break;
            case SOC_BLK_MS_ISEC:
                if (SOC_IS_SHADOW(unit)) {
                    if (port >= 1 && port <= 4) {
                        block = MS_ISEC0_BLOCK(unit);
                        pindex = port - 1;
                    } else {
                        block = MS_ISEC1_BLOCK(unit);
                        pindex = port - 5;
                    }
                }
                break;
            case SOC_BLK_MS_ESEC:
                if (SOC_IS_SHADOW(unit)) {
                    if (port >= 1 && port <= 4) {
                        block = MS_ESEC0_BLOCK(unit);
                        pindex = port - 1;
                    } else {
                        block = MS_ESEC1_BLOCK(unit);
                        pindex = port - 5;
                    }
                }
                break;
#if defined(BCM_KATANA2_SUPPORT)
            case SOC_BLK_TXLP:
                if (SOC_IS_KATANA2(unit)) {
                    soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                        SOC_BLK_TXLP, &block, &pindex);
                }
                break;
            case SOC_BLK_RXLP:
                if (SOC_IS_KATANA2(unit)) {
                    soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                        SOC_BLK_RXLP, &block, &pindex);
                }
                break;
#endif
            case SOC_BLK_CW:
                if (SOC_IS_SHADOW(unit)) {
                    block = CW_BLOCK(unit);
                }
                break;
            case SOC_BLK_ECI:
                block = ECI_BLOCK(unit);
                break;
            case SOC_BLK_OCCG:
                block = OCCG_BLOCK(unit);
                break;
            case SOC_BLK_DCH:
                if(REG_PORT_ANY != port)
                    block = DCH_BLOCK(unit, port);
                else
                    block = DCH_BLOCK(unit, 0);
                break;
            case SOC_BLK_DCL:
                if(REG_PORT_ANY != port)
                    block = DCL_BLOCK(unit, port);
                else
                    block = DCL_BLOCK(unit, 0);
                break;
            case SOC_BLK_DCMA:
                if(REG_PORT_ANY != port)
                    block = DCMA_BLOCK(unit, port);
                else
                    block = DCMA_BLOCK(unit, 0);
                break;
            case SOC_BLK_DCMB:
                if(REG_PORT_ANY != port)
                    block = DCMB_BLOCK(unit, port);
                else
                    block = DCMB_BLOCK(unit, 0);
                break;
            case SOC_BLK_DCMC:
                block = DCMC_BLOCK(unit);
                break;
            case SOC_BLK_CCS:
                if(REG_PORT_ANY != port)
                    block = CCS_BLOCK(unit, port);
                else
                    block = CCS_BLOCK(unit, 0);
                break;
            case SOC_BLK_RTP:
                block = RTP_BLOCK(unit);
                break;
            case SOC_BLK_MESH_TOPOLOGY:
                block = MESH_TOPOLOGY_BLOCK(unit);
                break;
            case SOC_BLK_FMAC:
                if(REG_PORT_ANY != port)
                    block = FMAC_BLOCK(unit, port);
                else
                    block = FMAC_BLOCK(unit, 0);
                break;
            case SOC_BLK_FSRD:
                if(REG_PORT_ANY != port)
                    block = FSRD_BLOCK(unit, port);
                else
                    block = FSRD_BLOCK(unit, 0);
                break;
            case SOC_BLK_BRDC_FMACH:
                block = BRDC_FMACH_BLOCK(unit);
                break;
            case SOC_BLK_BRDC_FMACL:
                block = BRDC_FMACL_BLOCK(unit);
                break;
            case SOC_BLK_BRDC_FSRD:
                block = BRDC_FSRD_BLOCK(unit);
                break;
            default:
                    block = -1; /* unknown non-port block */
                    break;
            }
        }
#ifdef  BCM_SIRIUS_SUPPORT
    }
#endif

    assert(block >= 0); /* block must be valid */

    /* determine final block, pindex, and index */
    gransh = 0;
    switch (SOC_REG_INFO(unit, reg).regtype) {
    case soc_cpureg:
    case soc_mcsreg:
    case soc_iprocreg:
        block = -1;
        pindex = 0;
        gransh = 2; /* 4 byte granularity */
        break;
    case soc_portreg:
        if (!SOC_BLOCK_IN_LIST(regblktype, portblktype) &&
            !(SOC_IS_SHADOW(unit) &&
             (SOC_BLOCK_IS(regblktype, SOC_BLK_MS_ISEC) || 
              SOC_BLOCK_IS(regblktype, SOC_BLK_MS_ESEC)))
#if defined(BCM_KATANA2_SUPPORT)
              && !(SOC_IS_KATANA2(unit) &&
             (SOC_BLOCK_IS(regblktype, SOC_BLK_TXLP) || 
              SOC_BLOCK_IS(regblktype, SOC_BLK_RXLP)))
#endif
            ) {
            if (soc_feature(unit, soc_feature_logical_port_num) &&
                block == MMU_BLOCK(unit)) {
                phy_port = SOC_INFO(unit).port_l2p_mapping[port];
                pindex = SOC_INFO(unit).port_p2m_mapping[phy_port];
            } else {
                pindex = port;
            }
        }
        break;
    case soc_cosreg:
        assert(index >= 0 && index < NUM_COS(unit));
        pindex = index;
        index = 0;
        break;
    case soc_genreg:
        pindex = 0;
        break;
    default:
        assert(0); /* unknown register type */
        break;
    }

    /* put together address: base|block|pindex + index */
    base = SOC_REG_INFO(unit, reg).offset;
    LOG_VERBOSE(BSL_LS_SOC_REG,
                (BSL_META_U(unit,
                            "base: %x "), base));
        
    if (block >= 0) {
        base |= ((SOC_BLOCK2OFFSET(unit, block) & 0xf) << SOC_BLOCK_BP) |
                 (((SOC_BLOCK2OFFSET(unit, block) >> 4) & 0x3) <<
                 SOC_BLOCK_MSB_BP);
    }
    
    if (pindex) {
        base |= pindex << SOC_REGIDX_BP;
    }
    
    if (SOC_REG_IS_ARRAY(unit, reg)) {
        assert(index >= 0 && index < SOC_REG_NUMELS(unit, reg));
        base += index*SOC_REG_ELEM_SKIP(unit, reg);
    } else if (index && SOC_REG_ARRAY(unit, reg)) {
        assert(index >= 0 && index < SOC_REG_NUMELS(unit, reg));
        if (index && SOC_REG_ARRAY2(unit, reg)) {
            base += ((index*2) << gransh);
        } else if (index && SOC_REG_ARRAY4(unit, reg)) {
            base += ((index * 4) << gransh);
        } else {
            base += (index << gransh);
        }
    }
    LOG_VERBOSE(BSL_LS_SOC_REG,
                (BSL_META_U(unit,
                            "addr: %x, block: %d, index: %d, pindex: %d, gransh: %d\n"),
                 base, block, index, pindex, gransh));
    return base;
}

/*
 * Function:    soc_reg_addr_get
 * Purpose:     calculate the address of a register
 * Parameters:
 *              unit  switch unit
 *              reg   register number
 *              port  port number or REG_PORT_ANY
 *              index array index (or cos number)
 *              is_write is this call for a write operation, which may result in a different block number
 * Returns:     register address suitable for soc_reg_get and friends
 * Notes:       the block number to access is determined by the register
 *              and the port number
 *
 * cpureg       00SSSSSS 00000000 0000RRRR RRRRRRRR
 * genreg       00SSSSSS BBBB1000 0000RRRR RRRRRRRR
 * portreg      00SSSSSS BBBB00PP PPPPRRRR RRRRRRRR
 * cosreg       00SSSSSS BBBB01CC CCCCRRRR RRRRRRRR
 *
 * all regs of bcm88230
 *              00000000 00001000 0000RRRR RRRRRRRR
 *
 * where        B+ is the 4 bit block number
 *              P+ is the 6 bit port number (within a block or chip wide)
 *              C+ is the 6 bit class of service
 *              R+ is the 12 bit register number
 *              S+ is the 6 bit Pipe stage
 */
uint32
soc_reg_addr_get(int unit, soc_reg_t reg, int port, int index, int is_write,
                 int *blk, uint8 *acc_type)
{
    uint32            base;        /* base address from reg_info */
    int               block = -1;  /* block number */
    int               pindex = -1; /* register port/cos field */
    int               gransh;      /* index granularity shift */
    soc_block_types_t regblktype;
    soc_block_t       portblktype;
    int               phy_port, i;
    int               instance = -1;
    int               instance_mask = 0;
    int               port_num_blktype;
    int               block_core = 0;
    
    if (!soc_feature(unit, soc_feature_new_sbus_format)) {
        return soc_reg_addr(unit, reg, port, index);
    }
    
    if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

#define SOC_REG_ADDR_INVALID_PORT 0 /* for asserts */

    *acc_type = SOC_REG_ACC_TYPE(unit, reg);

    portblktype = SOC_BLK_PORT;
    port_num_blktype = SOC_DRIVER(unit)->port_num_blktype > 1 ?
        SOC_DRIVER(unit)->port_num_blktype : 1;

    *blk = 0;/* not really needed, just to avoid coverity defect */

#if defined(BCM_CALADAN3_SUPPORT)
    if (SOC_IS_CALADAN3(unit)) {
        /* C3 has MSB set for port when it passes an instance number */
        if ((port != REG_PORT_ANY) && (port & SOC_REG_ADDR_INSTANCE_MASK)) {
            port &= (~SOC_REG_ADDR_INSTANCE_MASK);
            instance = port;
        }
        block = pindex = -1;
    }
#endif

    if ((REG_PORT_ANY != port) && (SOC_CORE_ALL != port)) {
        instance_mask = port & SOC_REG_ADDR_INSTANCE_MASK;
        port &= (~SOC_REG_ADDR_INSTANCE_MASK);
    }

#ifdef BCM_ARAD_SUPPORT
    /* Use core broadcast writes when possible for more efficient writes */
    if (SOC_IS_ARAD(unit)) {
        block_core = (port == SOC_CORE_ALL || port == REG_PORT_ANY) ?
                     (is_write ? SOC_CORE_ALL : 0) : port;
    }
#endif /* BCM_ARAD_SUPPORT */
    
    regblktype = SOC_REG_INFO(unit, reg).block;
    if (!instance_mask && port >= 0) {
        if (SOC_BLOCK_IN_LIST(regblktype, portblktype)) {
            assert(SOC_PORT_VALID(unit, port));
            if (soc_feature(unit, soc_feature_logical_port_num)) {
                /*
                 * COVERITY
                 *
                 * assert validates the port
                 */
                /* coverity[overrun-local : FALSE] */ 
                phy_port = SOC_INFO(unit).port_l2p_mapping[port];
            } else {
                phy_port = port;
            }
            for (i = 0; i < port_num_blktype; i++) {
#ifdef BCM_KATANA2_SUPPORT
                /* Override port blocks with Linkphy Blocks.. */
                if(SOC_IS_KATANA2(unit) &&
                    (REG_FIRST_BLK_TYPE(regblktype) == SOC_BLK_TXLP) ) {
                    soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                        SOC_BLK_TXLP, &block, &pindex);
                    break;
                } else if(SOC_IS_KATANA2(unit) &&
                    (REG_FIRST_BLK_TYPE(regblktype) == SOC_BLK_RXLP) ) {
                    soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                        SOC_BLK_RXLP, &block, &pindex);
                    break;
                }
#endif
#ifdef BCM_GREYHOUND_SUPPORT
                if(SOC_IS_GREYHOUND(unit) &&
                    (REG_FIRST_BLK_TYPE(regblktype) == SOC_BLK_XLPORT) ) {
                    if (soc_greyhound_pgw_reg_blk_index_get(unit, reg, port, NULL,
                        &block, &pindex, 0) > 0){
                        break;
                    }                        
                }
#endif
                block = SOC_PORT_IDX_BLOCK(unit, phy_port, i);
                if (block < 0) {
                    break;
                }
                if (SOC_BLOCK_IN_LIST(regblktype,
                                      SOC_BLOCK_TYPE(unit, block))) {
                    pindex = SOC_PORT_IDX_BINDEX(unit, phy_port, i);
                    break;
                }
            }
        } else {
            block = pindex = -1; /* multiple non-port block */
        }
    } else if (port == REG_PORT_ANY) {
        block = pindex = -1;
        if (SOC_BLOCK_IN_LIST(regblktype, portblktype)) {
            PBMP_ALL_ITER(unit, port) { /* try enabled ports */
                if (soc_feature(unit, soc_feature_logical_port_num)) {
                    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
                } else {
                    phy_port = port;
                }
                for (i = 0; i < port_num_blktype; i++) {
                    block = SOC_PORT_IDX_BLOCK(unit, phy_port, i);
                    if (block < 0) {
                        break;
                    }
                    if (SOC_BLOCK_IN_LIST
                        (regblktype, SOC_BLOCK_TYPE(unit, block))) {
                        pindex = SOC_PORT_IDX_BINDEX(unit, phy_port,
                                                     i);
                        break;
                    }
                }
                if (i == port_num_blktype) {
                    continue;
                }
                if (block >= 0) {
                    break;
                }
            }
            if (block < 0) {
                assert(SOC_REG_ADDR_INVALID_PORT); /* invalid port */
            }
        } else { /* match any port */
            if (!SOC_IS_SAND(unit)) {
                PBMP_ALL_ITER(unit, port) { /* try enabled ports */
                    break;
                }
            }
        }
    } else {
        port &= ~SOC_REG_ADDR_INSTANCE_MASK;
        instance = port;
        block = pindex = -1;
    }
    
#if defined (BCM_KATANA2_SUPPORT)
    if (SOC_IS_KATANA2(unit)) {
        if (((SOC_BLK_RXLP == REG_FIRST_BLK_TYPE(regblktype)) || 
             (SOC_BLK_TXLP == REG_FIRST_BLK_TYPE(regblktype))) &&
            (soc_portreg != SOC_REG_INFO(unit, reg).regtype)) {
            instance_mask = 1;
        }
    }
#endif

    if (REG_PORT_ANY == port || instance_mask || !SOC_BLOCK_IN_LIST(regblktype, portblktype)) {
        int blkport = port;
        if (port == REG_PORT_ANY) {
            blkport = 0;
        }
        switch (REG_FIRST_BLK_TYPE(regblktype)) {
        case SOC_BLK_ARL:
            block = ARL_BLOCK(unit);
            break;
        case SOC_BLK_IPIPE:
            block = IPIPE_BLOCK(unit);
            break;
        case SOC_BLK_IPIPE_HI:
            block = IPIPE_HI_BLOCK(unit);
            break;
        case SOC_BLK_EPIPE:
            block = EPIPE_BLOCK(unit);
            break;
        case SOC_BLK_EPIPE_HI:
            block = EPIPE_HI_BLOCK(unit);
            break;
        case SOC_BLK_IGR:
            block = IGR_BLOCK(unit);
            break;
        case SOC_BLK_EGR:
            block = EGR_BLOCK(unit);
            break;
        case SOC_BLK_BSE:
            block = BSE_BLOCK(unit);
            break;
        case SOC_BLK_CSE:
            block = CSE_BLOCK(unit);
            break;
        case SOC_BLK_HSE:
            block = HSE_BLOCK(unit);
            break;
        case SOC_BLK_SYS:
            break;
        case SOC_BLK_BSAFE:
            block = BSAFE_BLOCK(unit);
            break;
        case SOC_BLK_OTPC:
            block = OTPC_BLOCK(unit);
            break;
        case SOC_BLK_MMU:
            block = MMU_BLOCK(unit);
            break;
        case SOC_BLK_MCU:
            block = MCU_BLOCK(unit);
            break;
        case SOC_BLK_CMIC:
            block = CMIC_BLOCK(unit);
            break;
        case SOC_BLK_IPROC:
            block = IPROC_BLOCK(unit);
            break;
        case SOC_BLK_ESM:
            block = ESM_BLOCK(unit);
            break;
        case SOC_BLK_PORT_GROUP4:
            block = PG4_BLOCK(unit, port);
            break;
        case SOC_BLK_PORT_GROUP5:
            block = PG5_BLOCK(unit, port);
            break;
        case SOC_BLK_TOP:
            block = TOP_BLOCK(unit);
            break;
        case SOC_BLK_SER:
            block = SER_BLOCK(unit);
            break;
        case SOC_BLK_AXP:
            block = AXP_BLOCK(unit);
            break;
        case SOC_BLK_ISM:
            block = ISM_BLOCK(unit);
            break;
        case SOC_BLK_ETU:
            block = ETU_BLOCK(unit);
            break;
        case SOC_BLK_ETU_WRAP:
            block = ETU_WRAP_BLOCK(unit);
            break;
        case SOC_BLK_IBOD:
            block = IBOD_BLOCK(unit);
            break;
        case SOC_BLK_LLS:
            block = LLS_BLOCK(unit);
            break;
        case SOC_BLK_CES:
            block = CES_BLOCK(unit);
            break;
        case SOC_BLK_PGW_CL:
            if (instance_mask) {
                instance = port;
            } else{
                instance = SOC_INFO(unit).port_group[port];
            }
            block = PGW_CL_BLOCK(unit, instance);
            break;
        case SOC_BLK_PMQ:
            block = PMQ_BLOCK(unit, blkport);
            break;
        case SOC_BLK_AVS:
            block = AVS_BLOCK(unit);
            break;
        case SOC_BLK_IL:
            if (SOC_IS_SHADOW(unit)) {
                if (port == 9) {
                    block = IL0_BLOCK(unit);
                } else if (port == 13) {
                    block = IL1_BLOCK(unit);
                }
                pindex = 0;
            }
            if (SOC_IS_CALADAN3(unit)) {
                block = (instance == 0) ? IL0_BLOCK(unit):IL1_BLOCK(unit);
            }
            break;
        case SOC_BLK_MS_ISEC:
            if (SOC_IS_SHADOW(unit)) {
                if (port >= 1 && port <= 4) {
                    block = MS_ISEC0_BLOCK(unit);
                    pindex = port - 1;
                } else {
                    block = MS_ISEC1_BLOCK(unit);
                    pindex = port - 5;
                }
            }
            break;
        case SOC_BLK_MS_ESEC:
            if (SOC_IS_SHADOW(unit)) {
                if (port >= 1 && port <= 4) {
                    block = MS_ESEC0_BLOCK(unit);
                    pindex = port - 1;
                } else {
                    block = MS_ESEC1_BLOCK(unit);
                    pindex = port - 5;
                }
            }
            break;
        case SOC_BLK_CW:
            if (SOC_IS_SHADOW(unit)) {
                block = CW_BLOCK(unit);
            }
            break;
        case SOC_BLK_CM:
            if (SOC_IS_CALADAN3(unit)) {
               block = CM_BLOCK(unit);
            }
            break;
        case SOC_BLK_CO:
            if (SOC_IS_CALADAN3(unit)) {
                block = CO_BLOCK(unit, port);
            }
            break;
        case SOC_BLK_CI:
            if (SOC_IS_CALADAN3(unit) || SOC_IS_KATANA2(unit)) {
                block = CI_BLOCK(unit, port);
            }
            break;
        case SOC_BLK_CX:
            if (SOC_IS_CALADAN3(unit)) {
               block = CX_BLOCK(unit);
            }
            break;
        case SOC_BLK_LRA:
            if (SOC_IS_CALADAN3(unit)) {
               block = LRA_BLOCK(unit);
            }
            break;
        case SOC_BLK_LRB:
            if (SOC_IS_CALADAN3(unit)) {
               block = LRB_BLOCK(unit);
            }
            break;
        case SOC_BLK_OC:
            if (SOC_IS_CALADAN3(unit)) {
               block = OC_BLOCK(unit);
            }
            break;
        case SOC_BLK_PB:
            if (SOC_IS_CALADAN3(unit)) {
               block = PB_BLOCK(unit);
            }
            break;
        case SOC_BLK_PD:
            if (SOC_IS_CALADAN3(unit)) {
               block = PD_BLOCK(unit);
            }
            break;
        case SOC_BLK_PP:
            if (SOC_IS_CALADAN3(unit)) {
               block = PP_BLOCK(unit);
            }
            break;
        case SOC_BLK_PR:
            if (SOC_IS_CALADAN3(unit)) {
               block = PR_BLOCK(unit, port);
            }
            break;
        case SOC_BLK_PT:
            if (SOC_IS_CALADAN3(unit)) {
                block = PT_BLOCK(unit, port);
            }
            break;
        case SOC_BLK_QM:
            if (SOC_IS_CALADAN3(unit)) {
               block = QM_BLOCK(unit);
            }
            break;
        case SOC_BLK_RC:
            if (SOC_IS_CALADAN3(unit)) {
               block = RC_BLOCK(unit);
            }
            break;
        case SOC_BLK_TMA:
            if (SOC_IS_CALADAN3(unit)) {
               block = TMA_BLOCK(unit);
            }
            break;
        case SOC_BLK_TMB:
            if (SOC_IS_CALADAN3(unit)) {
               block = TMB_BLOCK(unit);
            }
            break;
        case SOC_BLK_TM_QE:
            if (SOC_IS_CALADAN3(unit)) {
                block = TM_QE_BLOCK(unit, port);
            }
            break;
        case SOC_BLK_TP:
            if (SOC_IS_CALADAN3(unit)) {
                block = TP_BLOCK(unit, port);
            }
            break;
#if defined(BCM_KATANA2_SUPPORT)
        case SOC_BLK_TXLP:
            if (SOC_IS_KATANA2(unit)) {
                soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                    SOC_BLK_TXLP, &block, &pindex);
            }
            break;
        case SOC_BLK_RXLP:
            if (SOC_IS_KATANA2(unit)) {
                soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                    SOC_BLK_RXLP, &block, &pindex);
            }
            break;
#endif
            /* DPP */
        case SOC_BLK_CFC:
            block = CFC_BLOCK(unit);
            break;
        case SOC_BLK_OCB:
            block = OCB_BLOCK(unit);
            break;
        case SOC_BLK_CRPS:
            block = CRPS_BLOCK(unit);
            break;
        case SOC_BLK_ECI:
            block = ECI_BLOCK(unit);
            break;
        case SOC_BLK_EGQ:
            block = EGQ_BLOCK(unit, block_core);
            break;
        case SOC_BLK_FCR:
            block = FCR_BLOCK(unit);
            break;
        case SOC_BLK_FCT:
            block = FCT_BLOCK(unit);
            break;
        case SOC_BLK_FDR:
            block = FDR_BLOCK(unit);
            break;
        case SOC_BLK_FDA:
            block = FDA_BLOCK(unit);
            break;
        case SOC_BLK_FDT:
            block = FDT_BLOCK(unit);
            break;
        case SOC_BLK_MESH_TOPOLOGY:
            block = MESH_TOPOLOGY_BLOCK(unit);
            break;
        case SOC_BLK_IDR:
            block = IDR_BLOCK(unit);
            break;
        case SOC_BLK_IHB:
            block = IHB_BLOCK(unit, block_core);
            break;
        case SOC_BLK_IHP:
            block = IHP_BLOCK(unit, block_core);
            break;
        case SOC_BLK_IPS:
            block = IPS_BLOCK(unit, block_core);
            break;
        case SOC_BLK_IPT:
            block = IPT_BLOCK(unit);
            break;
        case SOC_BLK_IQM:
            block = IQM_BLOCK(unit, block_core);
            break;
        case SOC_BLK_IRE:
            block = IRE_BLOCK(unit);
            break;
        case SOC_BLK_IRR:
            block = IRR_BLOCK(unit);
            break;
        case SOC_BLK_FMAC:
            block = FMAC_BLOCK(unit, blkport);
            break;
        case SOC_BLK_XLP:
            block = XLP_BLOCK(unit, blkport);
            break;
        case SOC_BLK_CLP:
            block = CLP_BLOCK(unit, blkport);
            break;
        case SOC_BLK_NBI:
            block = NBI_BLOCK(unit);
            break;
        case SOC_BLK_CGM:
            block = CGM_BLOCK(unit, block_core);
            break;
        case SOC_BLK_OAMP:
            block = OAMP_BLOCK(unit);
            break;
        case SOC_BLK_OLP:
            block = OLP_BLOCK(unit);
            break;
        case SOC_BLK_FSRD:
            block = FSRD_BLOCK(unit, blkport);
            break;
        case SOC_BLK_RTP:
            block = RTP_BLOCK(unit);
            break;
        case SOC_BLK_SCH:
            block = SCH_BLOCK(unit, block_core);
            break;
        case SOC_BLK_EPNI:
            block = EPNI_BLOCK(unit, block_core);
            break;
        case SOC_BLK_DRCA:
            block = DRCA_BLOCK(unit);
            break;
        case SOC_BLK_DRCB:
            block = DRCB_BLOCK(unit);
            break;
        case SOC_BLK_DRCC:
            block = DRCC_BLOCK(unit);
            break;
        case SOC_BLK_DRCD:
            block = DRCD_BLOCK(unit);
            break;
        case SOC_BLK_DRCE:
            block = DRCE_BLOCK(unit);
            break;
        case SOC_BLK_DRCF:
            block = DRCF_BLOCK(unit);
            break;
        case SOC_BLK_DRCG:
            block = DRCG_BLOCK(unit);
            break;
        case SOC_BLK_DRCH:
            block = DRCH_BLOCK(unit);
            break;
        case SOC_BLK_EDB:
            block = EDB_BLOCK(unit);
            break;
        case SOC_BLK_ILKN_PMH:
            block = ILKN_PMH_BLOCK(unit);
            break;
        case SOC_BLK_IPST:
            block = IPST_BLOCK(unit);
            break;
        case SOC_BLK_IQMT:
            block = IQMT_BLOCK(unit);
            break;
        case SOC_BLK_PPDB_A:
            block = PPDB_A_BLOCK(unit);
            break;
        case SOC_BLK_PPDB_B:
            block = PPDB_B_BLOCK(unit);
            break;
        case SOC_BLK_ILKN_PML:
            block = ILKN_PML_BLOCK(unit);
            break;
        case SOC_BLK_MRPS:
            block = MRPS_BLOCK(unit, blkport);
            break;
        case SOC_BLK_NBIL:
            block = NBIL_BLOCK(unit);
            break;
        case SOC_BLK_NBIH:
            block = NBIH_BLOCK(unit);
            break;
        case SOC_BLK_DRCBROADCAST:
            block = DRCBROADCAST_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_FSRD:
            block = BRDC_FSRD_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_FMAC:
            block = BRDC_FMAC_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_CGM:
            block = BRDC_CGM_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_EGQ:
            block = BRDC_EGQ_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_EPNI:
            block = BRDC_EPNI_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_IHB:
            block = BRDC_IHB_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_IHP:
            block = BRDC_IHP_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_IPS:
            block = BRDC_IPS_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_IQM:
            block = BRDC_IQM_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_SCH:
            block = BRDC_SCH_BLOCK(unit);
            break;
#if defined(BCM_DNX_P3_SUPPORT)
       /* JERICHO-2-P3 */
        case SOC_BLK_CFG:
                block = P3_CFG_BLOCK(unit);
                break;
        case SOC_BLK_EM:
                block = P3_EM_BLOCK(unit);
                break;
        case SOC_BLK_TABLE:
                block = P3_TABLE_BLOCK(unit);
                break;
        case SOC_BLK_TCAM:
                block = P3_TCAM_BLOCK(unit);
                break;
#endif /* BCM_DNX_P3_SUPPORT */
        /* DFE  blocks*/
        case SOC_BLK_DCH:
            block=DCH_BLOCK(unit,blkport);
            break;
        case SOC_BLK_DCL:
            block=DCL_BLOCK(unit,blkport);
            break;
        case SOC_BLK_OCCG:
            block=OCCG_BLOCK(unit);
            break;
        case SOC_BLK_DCM:
            block=DCM_BLOCK(unit,blkport);
            break;
        case SOC_BLK_DCMC:
            block = DCMC_BLOCK(unit);
            break;
        case SOC_BLK_CCS:
            block=CCS_BLOCK(unit,blkport);
            break;
        case SOC_BLK_BRDC_FMAC_AC:
            block=BRDC_FMAC_AC_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_FMAC_BD:
            block=BRDC_FMAC_BD_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_DCH:
            block=BRDC_DCH_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_DCL:
            block=BRDC_DCL_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_DCM:
            block=BRDC_DCM_BLOCK(unit);
            break;
        case SOC_BLK_BRDC_CCS:
            block=BRDC_CCS_BLOCK(unit);
            break;
        case SOC_BLK_GPORT:
            block=GPORT_BLOCK(unit, blkport);
            break;
        default:
            block = -1; /* unknown non-port block */
            break;
        }
    }
#if defined(BCM_CALADAN3_SUPPORT)
    else {
        /* instance support for port blocks */

        if (instance >= 0) {
            switch (REG_FIRST_BLK_TYPE(regblktype)) {
                
            case SOC_BLK_CLPORT:
                if (SOC_IS_CALADAN3(unit)) {
                    assert(instance >= 0);
                    pindex = -1;
                    block = CLPORT_BLOCK(unit, instance);
                }
                break;
                
            default:
                /* nop */
                break;
            }
        }

    }
#endif

    assert(block >= 0); /* block must be valid */

    /* determine final block, pindex, and index */
    gransh = 0;
    switch (SOC_REG_INFO(unit, reg).regtype) {
    case soc_cpureg:
    case soc_mcsreg:
    case soc_iprocreg:
        block = -1;
        pindex = 0;
        gransh = 2; /* 4 byte granularity */
        break;
    case soc_ppportreg:
    case soc_portreg:
        if (!SOC_BLOCK_IN_LIST(regblktype, portblktype) && 
            !(SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_MS_ISEC)) && 
            !(SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_MS_ESEC)) &&
            !(SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_TXLP)) && 
            !(SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_RXLP))) {
            if (soc_feature(unit, soc_feature_logical_port_num) &&
                block == MMU_BLOCK(unit)) {
                phy_port = SOC_INFO(unit).port_l2p_mapping[port];
                pindex = SOC_INFO(unit).port_p2m_mapping[phy_port];
                if (pindex < 0) {
                    pindex = SOC_INFO(unit).max_port_p2m_mapping[phy_port];
                }
                assert(pindex >= 0);
#ifdef BCM_TRIUMPH3_SUPPORT
                /* We do not want any more of these exceptions in the code */
                if (SOC_IS_TRIUMPH3(unit) && (reg == MMU_INTFO_CONGST_STr)) {
                    pindex = port;
                }
#endif
            } else {
                pindex = port;
            }
            gransh = 8;
        }
#if defined(BCM_CALADAN3_SUPPORT)
        if (SOC_IS_CALADAN3(unit)) {
            /* port registers must not have instance */
            assert(instance < 0);
        }
#endif
        break;
    case soc_cosreg:
        assert(index >= 0 && index < NUM_COS(unit));
        pindex = index;
        index = 0;
        break;
    case soc_genreg:
        gransh = 8;
        pindex = 0;
        break;
    default:
        assert(0); /* unknown register type */
        break;
    }

    /* put together address: base|block|pindex + index */
    base = SOC_REG_INFO(unit, reg).offset;
    LOG_VERBOSE(BSL_LS_SOC_REG,
                (BSL_META_U(unit,
                            "base: %x "), base));

    if (block >= 0) {
        *blk = SOC_BLOCK_INFO(unit, block).cmic;
    }
    
    if (pindex != -1) {
        base |= pindex;
    }
    
    if (SOC_REG_IS_ARRAY(unit, reg)) {
        assert(index >= 0 && index < SOC_REG_NUMELS(unit, reg));
        base += index*SOC_REG_ELEM_SKIP(unit, reg);
    }  else if (index && SOC_REG_ARRAY(unit, reg)) {
        assert(index >= 0 && index < SOC_REG_NUMELS(unit, reg));
        if (index && SOC_REG_ARRAY2(unit, reg)) {
            base += ((index*2) << gransh);
        } else if (index && SOC_REG_ARRAY4(unit, reg)) {
            base += ((index * 4) << gransh);             
        } else {
            base += (index << gransh);
        }
    }
    LOG_VERBOSE(BSL_LS_SOC_REG,
                (BSL_META_U(unit,
                            "addr new: %x, block: %d, index: %d, pindex: %d, gransh: %d\n"),
                 base, *blk, index, pindex, gransh));
    return base;
}

int
soc_regaddrlist_alloc(soc_regaddrlist_t *addrlist)
{
    if ((addrlist->ainfo = sal_alloc(_SOC_MAX_REGLIST *
                sizeof(soc_regaddrinfo_t), "regaddrlist")) == NULL) {
        return SOC_E_MEMORY;
    }
    addrlist->count = 0;

    return SOC_E_NONE;
}

int
soc_regaddrlist_free(soc_regaddrlist_t *addrlist)
{
    if (addrlist->ainfo) {
        sal_free(addrlist->ainfo);
    }

    return SOC_E_NONE;
}

/*
 * Function:   soc_reg_fields32_modify
 * Purpose:    Modify the value of a fields in a register.
 * Parameters:
 *       unit         - (IN) SOC unit number.
 *       reg          - (IN) Register.
 *       port         - (IN) Port number.
 *       field_count  - (IN) Number of fields to modify.
 *       fields       - (IN) Modified fields array.
 *       values       - (IN) New value for each member of fields array.
 * Returns:
 *       BCM_E_XXX
 */
int
soc_reg_fields32_modify(int unit, soc_reg_t reg, soc_port_t port,
                        int field_count, soc_field_t *fields, uint32 *values)
{
    uint64 data64;      /* Current 64 bit register data.  */
    uint64 odata64;     /* Original 64 bit register data. */
    uint32 data32;      /* Current 32 bit register data.  */
    uint32 odata32;     /* Original 32 bit register data. */
    uint32 reg_addr;    /* Register address.              */
    int idx;            /* Iteration index.               */
    uint32 max_val;     /* Max value to fit the field     */
    int field_len;      /* Bit length of the field        */

    /* Check that register is a valid one for this unit. */
    if (!SOC_REG_IS_VALID(unit, reg)) {
        return SOC_E_PARAM;
    }

    if ((NULL == fields) || (NULL == values)) {
        return SOC_E_PARAM;
    }

    /*  Fields & values sanity check. */
    for (idx = 0; idx < field_count; idx++) {

        /* Make sure field is present in register. */
        if (!soc_reg_field_valid(unit, reg, fields[idx])) {
            return SOC_E_PARAM;
        }
        /* Make sure value can fit into field */
        field_len = soc_reg_field_length(unit, reg, fields[idx]);
        max_val = (field_len < 32) ? ((1 << field_len) - 1) : 0xffffffff;
        if (values[idx] > max_val) {
            return SOC_E_PARAM;
        }
    }

    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        if (SOC_REG_IS_64(unit, reg)) {

            /* Read current register value. */
            SOC_IF_ERROR_RETURN(soc_reg64_get(unit, reg, port, 0, &data64));
            odata64 = data64;

            /* Update fields with new values. */
            for (idx = 0; idx < field_count; idx ++) {
                soc_reg64_field32_set(unit, reg, &data64, fields[idx], values[idx]);
            }
            if (COMPILER_64_NE(data64, odata64)) {
                /* Write new register value back to hw. */
                SOC_IF_ERROR_RETURN(soc_reg64_set(unit, reg, port, 0, data64));
            }
        } else {
            if (soc_cpureg == SOC_REG_TYPE(unit,  reg)) {
                reg_addr = soc_reg_addr(unit, reg, REG_PORT_ANY, port);
                /* Read PCI register value. */
                SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, reg_addr, &data32));
#ifdef BCM_IPROC_SUPPORT
            } else if (soc_iprocreg == SOC_REG_TYPE(unit,  reg)) {
                reg_addr = soc_reg_addr(unit, reg, REG_PORT_ANY, port);
                SOC_IF_ERROR_RETURN(soc_iproc_getreg(unit, reg_addr, &data32));
#endif
            } else {
                reg_addr = 0;  /* Compiler warning defense. */
                SOC_IF_ERROR_RETURN(soc_reg32_get(unit, reg, port, 0, &data32));
            }
            odata32 = data32;

            for (idx = 0; idx < field_count; idx ++) {
                soc_reg_field_set(unit, reg, &data32, fields[idx], values[idx]);
            }
            if (data32 != odata32) {
                /* Write new register value back to hw. */
                if (soc_cpureg == SOC_REG_TYPE(unit,  reg)) {
                    SOC_IF_ERROR_RETURN(soc_pci_write(unit, reg_addr, data32));
#ifdef BCM_IPROC_SUPPORT
                } else if (soc_iprocreg == SOC_REG_TYPE(unit,  reg)) {
                    SOC_IF_ERROR_RETURN(soc_iproc_setreg(unit, reg_addr, data32));
#endif
                } else {
                    SOC_IF_ERROR_RETURN(soc_reg32_set(unit, reg, port, 0, data32));
                }
            }
        }
    } else {
        /* Calculate register address. */
        reg_addr = soc_reg_addr(unit, reg, port, 0);

        if (SOC_REG_IS_64(unit, reg)) {

            /* Read current register value. */
            SOC_IF_ERROR_RETURN(soc_reg64_read(unit, reg_addr, &data64));
            odata64 = data64;

            /* Update fields with new values. */
            for (idx = 0; idx < field_count; idx ++) {
                soc_reg64_field32_set(unit, reg, &data64, fields[idx], values[idx]);
            }
            if (COMPILER_64_NE(data64, odata64)) {
#if defined(BCM_XGS_SUPPORT)
                if (soc_feature(unit, soc_feature_regs_as_mem)) {
                    (void)soc_ser_reg_cache_set(unit, reg, port, 0, data64);
                }
#endif /* BCM_XGS_SUPPORT */                
                /* Write new register value back to hw. */
                SOC_IF_ERROR_RETURN(soc_reg64_write(unit, reg_addr, data64));
            }
        } else {
            if (soc_cpureg == SOC_REG_TYPE(unit,  reg)) {
                reg_addr = soc_reg_addr(unit, reg, REG_PORT_ANY, port);
                /* Read PCI register value. */
                SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, reg_addr, &data32));
            } else {
                SOC_IF_ERROR_RETURN(soc_reg32_read(unit, reg_addr, &data32));
            }

            odata32 = data32;

            for (idx = 0; idx < field_count; idx ++) {
                soc_reg_field_set(unit, reg, &data32, fields[idx], values[idx]);
            }
            if (data32 != odata32) {
                /* Write new register value back to hw. */
                if (soc_cpureg == SOC_REG_TYPE(unit,  reg)) {
                    SOC_IF_ERROR_RETURN(soc_pci_write(unit, reg_addr, data32));
                } else {
#if defined(BCM_XGS_SUPPORT)
                    if (soc_feature(unit, soc_feature_regs_as_mem)) {
                        (void)soc_ser_reg32_cache_set(unit, reg, port, 0, data32);
                    }
#endif /* BCM_XGS_SUPPORT */
                    SOC_IF_ERROR_RETURN(soc_reg32_write(unit, reg_addr, data32));
                }
            }
        }
    }
    return (SOC_E_NONE);
}

/*
 * Function:   soc_reg_field32_modify
 * Purpose:    Modify the value of a field in a register.
 * Parameters:
 *       unit  - (IN) SOC unit number.
 *       reg   - (IN) Register.
 *       port  - (IN) Port number.
 *       field - (IN) Modified field.
 *       value - (IN) New field value.
 * Returns:
 *       SOC_E_XXX
 */
int
soc_reg_field32_modify(int unit, soc_reg_t reg, soc_port_t port, 
                       soc_field_t field, uint32 value)
{
    return soc_reg_fields32_modify(unit, reg, port, 1, &field, &value);
}

/*
 * Function:   soc_reg_above_64_field32_modify
 * Purpose:    Modify the value of a 32 bit field in any size register.
 * Parameters:
 *       unit  - (IN) SOC unit number.
 *       reg   - (IN) Register.
 *       port  - (IN) Port number.
 *       index - (IN) instance index
 *       field - (IN) Modified field.
 *       value - (IN) New field value.
 * Returns:
 *       SOC_E_XXX
 */
int
soc_reg_above_64_field32_modify(int unit, soc_reg_t reg, soc_port_t port, 
                       int index, soc_field_t field, uint32 value)
{
    int rc;
    soc_reg_above_64_val_t data;
    SOC_REG_ABOVE_64_CLEAR(data);
    rc = soc_reg_above_64_get(unit, reg, port, index, data);
    if (rc != SOC_E_NONE){
        return rc;
    }
    soc_reg_above_64_field32_set(unit, reg, data, field, value);
    rc = soc_reg_above_64_set(unit, reg, port, index, data);
    if (rc != SOC_E_NONE){
        return rc;
    }
    return SOC_E_NONE;
}

/*
 * Function:   soc_reg_above_64_field32_modify
 * Purpose:    Modify the value of a 32 bit field in any size register.
 * Parameters:
 *       unit  - (IN) SOC unit number.
 *       reg   - (IN) Register.
 *       port  - (IN) Port number.
 *       index - (IN) instance index
 *       field - (IN) Modified field.
 *       value - (IN) New field value.
 * Returns:
 *       SOC_E_XXX
 */
int
soc_reg_above_64_field64_modify(int unit, soc_reg_t reg, soc_port_t port, 
                       int index, soc_field_t field, uint64 value)
{
    int rc;
    soc_reg_above_64_val_t 
        data;
    SOC_REG_ABOVE_64_CLEAR(data);
    rc = soc_reg_above_64_get(unit, reg, port, index, data);
    if (rc != SOC_E_NONE){
        return rc;
    }
    soc_reg_above_64_field64_set(unit, reg, data, field, value);
    rc = soc_reg_above_64_set(unit, reg, port, index, data);
    if (rc != SOC_E_NONE){
        return rc;
    }
    return SOC_E_NONE;
}

/*
 * Function:    soc_reg_port_idx_valid
 * Purpose:     Determine if a register of a given 
 *                   index and port is valid.
 * Returns:      Returns TRUE  if register is valid.
 *               Returns FALSE if register is not valid.
 */
int
soc_reg_port_idx_valid(int unit, soc_reg_t reg, soc_port_t port, int idx)
{
    soc_numelport_set_t *numelports;
    uint32 *portslist;
    int i, numellist_idx, indx;

    /* idx is -1 means "any/all indexes..". so, check for Index 0 */
    indx = (idx == -1) ? 0 : idx;

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        
        if (SOC_BLOCK_IS(SOC_REG_INFO(unit, reg).block, SOC_BLK_TXLP)) {
            if (soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                SOC_BLK_TXLP, NULL, NULL) != SOC_E_NONE) {
                /* Not a TXLP Port */
                return FALSE;
            }
        } else if (SOC_BLOCK_IS(SOC_REG_INFO(unit, reg).block, SOC_BLK_RXLP)) {
            if (soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                SOC_BLK_RXLP, NULL, NULL) != SOC_E_NONE) {
                /* Not a RXLP Port */
                return FALSE;
            }
        }
    }
#endif

    numellist_idx = SOC_REG_NUMELPORTLIST_IDX(unit, reg);
    if (numellist_idx == -1) {
     /* No PORTLIST or NUMEL_PERPORT */
        return TRUE;
    }

    numelports = soc_numelports_list[numellist_idx];
    i=0;
    while (numelports[i].f_idx != -1) {
        if ((indx >= numelports[i].f_idx) && (indx <= numelports[i].l_idx)) {
            portslist = soc_ports_list[numelports[i].pl_idx];
            if (portslist[port /32] & (1 << (port % 32))) {
                return TRUE;
            } else {
                return FALSE;
            }
        }
        i++;
    }
    /* If idx is not found in the numel list, then it is just PORTLIST, not NUMELS_PERPORT */
    portslist = soc_ports_list[numelports[0].pl_idx];
    if (portslist[port /32] & (1 << (port % 32))) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * Function:   soc_reg_egress_cell_count_get
 * Purpose:    Retrieves the number of egress cells for a <port, cos> pair.
 * Parameters:
 *       unit  - (IN) SOC unit number.
 *       port  - (IN) Port number.
 *       cos   - (IN) COS queue.
 *       data  - (OUT) Cell count.
 * Returns:
 *       SOC_E_XXX
 */
int
soc_reg_egress_cell_count_get(int unit, soc_port_t port, int cos, uint32 *data)
{
    if (!SOC_PORT_VALID(unit, port) || cos < 0 || cos >= NUM_COS(unit)) {
        return SOC_E_PARAM;
    }
    SOC_IF_ERROR_RETURN(READ_COSLCCOUNTr(unit, port, cos, data));
    return SOC_E_NONE;
}

#ifdef BCM_CMICM_SUPPORT

soc_cmicm_reg_t cmicm_regs[] = CMICM_REG_INIT;

STATIC soc_cmicm_reg_t
*soc_cmicm_srch (uint32 addr) {
    int start = 0;
    int end = NUM_CMICM_REGS - 1;
    int mid = (start + end) >> 1;
    while (start <= end && cmicm_regs[mid].addr != addr) {
        if (cmicm_regs[mid].addr > addr) {
            end = mid - 1;
        } else {
            start = mid + 1;
        }
        mid = (start + end) >> 1;
    }
    return (cmicm_regs[mid].addr == addr) ? &(cmicm_regs[mid]) : NULL;
}

soc_cmicm_reg_t
*soc_cmicm_reg_get (uint32 idx) {
    return &(cmicm_regs[idx]);
}

soc_reg_t
soc_cmicm_addr_reg (uint32 addr) {
    soc_cmicm_reg_t *cmreg = soc_cmicm_srch(addr);
    return (cmreg == NULL)?INVALIDr:cmreg->reg;
}

char *
soc_cmicm_addr_name (uint32 addr) {
    soc_cmicm_reg_t *cmreg = soc_cmicm_srch(addr);
    return (cmreg == NULL)?"???":cmreg->name;
}

uint32
soc_pci_mcs_read(int unit, uint32 addr) {
    uint32 page = (addr & 0xffff8000);
    uint32 off = (addr & 0x00007fff);
    uint32 data;


     soc_pci_write(unit, CMIC_PIO_MCS_ACCESS_PAGE_OFFSET, page);

    /* Hardcoded for now.. use reg addr if & when available in regfile */
    data = soc_pci_read(unit, (0x38000 + off)); 
#ifdef BROADCOM_DEBUG
    if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
        _soc_reg_debug(unit, 32, "read", addr, 0, data);
    }
#endif /* BROADCOM_DEBUG */
    /* _soc_snoop_reg(unit, 0, 0, addr,SOC_REG_SNOOP_READ,0,data); */
    return data;
}

int
soc_pci_mcs_getreg(int unit, uint32 addr, uint32 *data_ptr) {
    *data_ptr = soc_pci_mcs_read(unit, addr);
    return SOC_E_NONE;
}

int
soc_pci_mcs_write(int unit, uint32 addr, uint32 data) {
    uint32 page = (addr & 0xffff8000);
    uint32 off = (addr & 0x00007fff);

#ifdef BROADCOM_DEBUG
    if (bsl_check(bslLayerSoc, bslSourceReg, bslSeverityNormal, unit)) {
        _soc_reg_debug(unit, 32, "write", addr, 0, data);
    }
#endif /* BROADCOM_DEBUG */
    /* _soc_snoop_reg(unit, 0, 0, addr,SOC_REG_SNOOP_WRITE,0,data); */


    soc_pci_write(unit, CMIC_PIO_MCS_ACCESS_PAGE_OFFSET, page);
   

    /* Hardcoded for now.. use reg addr if & when available in regfile */
    return soc_pci_write(unit, (0x38000 + off), data); 
}

#endif /* BCM_CMICM_SUPPORT */

/* Get register length in bytes */
int
soc_reg_bytes(int unit, soc_reg_t reg) {

    int              bits = 0;
    soc_reg_info_t   *regp;
    int              i, bytes;
    soc_field_info_t *fieldp;

     if (!SOC_REG_IS_VALID(unit, reg)) {
#if !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif /* !SOC_NO_NAMES */
        assert(SOC_REG_IS_VALID(unit, reg));
    }
    regp = &(SOC_REG_INFO(unit, reg));

    for (i = 0; i < (int)(regp->nFields); i++) {
        fieldp = &(regp->fields[i]);
        bits = ((fieldp->len + fieldp->bp) > bits)? 
            (fieldp->len + fieldp->bp): bits;
    }
    bytes = BITS2BYTES(bits);
    return bytes;
}
/* 
 * Function:     
 *     soc_reg_snoop_register 
 * Purpose:
 *      Registers a snooping call back for specific memory.
 *      Call back will be called on Read or Write operations
 *      on the register according to specified flags
 * Parameters:          
 *      unit         -  (IN) BCM device number.
 *      reg          -  (IN) Register to register a call back for.
 *      flags        -  (IN) SOC_REGS_SNOOP_XXX flags.
 *      snoop_cv     -  (IN) User provided call back, NULL for unregister
 *      user_data    -  (IN) user provided data to be passed to call back function
 * Returns:
 *      None
 */     
void 
soc_reg_snoop_register(int unit, soc_reg_t reg, uint32 flags,
                      soc_reg_snoop_cb_t snoop_cb, void *user_data)
{  
    soc_reg_info_t      *reg_info_p;
        
    if (!SOC_REG_IS_VALID(unit, reg)) {
#if defined(BCM_ESW_SUPPORT) && !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    reg_info_p = &SOC_REG_INFO(unit, reg);

    assert(NULL != snoop_cb);

    reg_info_p->snoop_cb = snoop_cb;
    reg_info_p->snoop_user_data = user_data;
    reg_info_p->snoop_flags = flags;

    return;
}

/*
 * Function:
 *     soc_reg_snoop_unregister
 * Purpose:
 *      Ubregisters a snooping call back for specific register.
 *      this function will not fail even if call back was not previously
 *      registered.
 * Parameters:
 *      unit         -  (IN) BCM device number.
 *      reg          -  (IN) Register to register a call back for.
 * Returns:
 *      None
 */
void
soc_reg_snoop_unregister(int unit, soc_reg_t reg)
{  
    soc_reg_info_t      *reg_info_p;

    if (!SOC_REG_IS_VALID(unit, reg)) {
#if defined(BCM_ESW_SUPPORT) && !defined(SOC_NO_NAMES)
        LOG_CLI((BSL_META_U(unit,
                            "reg %s is invalid\n"), soc_reg_name[reg]));
#endif
        assert(SOC_REG_IS_VALID(unit, reg));
    }

    reg_info_p = &SOC_REG_INFO(unit, reg);

    reg_info_p->snoop_cb = NULL;
    reg_info_p->snoop_user_data = NULL;
    reg_info_p->snoop_flags = 0;

    return;
}

int 
soc_reg_access_func_register(int unit, soc_reg_access_t* reg_access) 
{
    SOC_INFO(unit).reg_access = *reg_access;

    return SOC_E_NONE;
}

#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRAB_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(PORTMOD_SUPPORT)*/


