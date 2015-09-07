/*
 * $Id: reg_skip.c,v 1.137 Broadcom SDK $
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
 * Diag routines to identify registers that are only implemented
 * on a subset of ports/cos.
 */

#include <appl/diag/system.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#include <soc/mcm/memregs.h>
#endif
#ifdef BCM_TRIDENT2_SUPPORT
#include <soc/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */

#define _MAP_REG_MASK_SUBSETS(unit, i, count, arr) \
    for (i = 0; i < count; i++) {\
        *arr[i] = SOC_REG_MASK_SUBSET(unit)[i];\
    }

#define _INIT_REG_MASK_SUBSETS(unit, i, count, arr) \
    SOC_REG_MASK_SUBSET(unit) = sal_alloc(count * sizeof(pbmp_t *),\
                                          "per unit reg mask subsets");\
    if (NULL == SOC_REG_MASK_SUBSET(unit)) {\
        return SOC_E_MEMORY;\
    }\
    sal_memset(SOC_REG_MASK_SUBSET(unit), 0, count * sizeof(pbmp_t *));\
    for (i = 0; i < count; i++) {\
        SOC_REG_MASK_SUBSET(unit)[i] = sal_alloc(sizeof(pbmp_t),\
                                                 "reg mask subsets");\
        if (NULL == SOC_REG_MASK_SUBSET(unit)[i]) {\
            return SOC_E_MEMORY;\
        }\
        *mask_map_arr[i] = SOC_REG_MASK_SUBSET(unit)[i];\
    }

#define CHECK_FEATURE_BLK(feat, blk) \
    (SOC_REG_BLOCK_IS(unit, ainfo->reg, blk) && \
    (!soc_feature(unit, feat)))

/*
 * Description:
 *  There are many per-port registers in Triumph which only exist on
 *  a subset of ports as described by PORTLIST and PERPORT_MASKBITS
 *  attributes in the regsfile. There are also several indexed registers
 *  with different number of elements depending on the port as described
 *  by the NUMELS_PERPORT attribute. These routines identify whether
 *  a register should be skipped for a given port/cos index. These routines 
 *  also adjust the mask for these special registers so unimplemented
 *  bits get skipped during register tests. 
 */



#ifdef BCM_TRIUMPH_SUPPORT
STATIC int
reg_mask_subset_tr(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    pbmp_t *tr_8pg_ports;
    pbmp_t *tr_higig_ports;
    pbmp_t *tr_24q_ports;
    pbmp_t *tr_non_cpu_ports;
    pbmp_t *tr_all_ports;
#define _REG_MASK_SUBSET_MAX_TR 5
    pbmp_t **mask_map_arr[_REG_MASK_SUBSET_MAX_TR];
    int i, count = _REG_MASK_SUBSET_MAX_TR;
    uint64 temp_mask;
    pbmp_t *pbmp;

    mask_map_arr[0] = &tr_8pg_ports;
    mask_map_arr[1] = &tr_higig_ports;
    mask_map_arr[2] = &tr_24q_ports;
    mask_map_arr[3] = &tr_non_cpu_ports;
    mask_map_arr[4] = &tr_all_ports;
    if (NULL == SOC_REG_MASK_SUBSET(unit)) {
        _INIT_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);
        
        /* 8PG_PORTS = [2,14,26..31] */
        SOC_PBMP_CLEAR(*tr_8pg_ports);
        if (IS_XE_PORT(unit, 2) || IS_HG_PORT(unit, 2)) {
            SOC_PBMP_PORT_ADD(*tr_8pg_ports, 2);
        }
        if (IS_XE_PORT(unit, 14) || IS_HG_PORT(unit, 14)) {
            SOC_PBMP_PORT_ADD(*tr_8pg_ports, 14);
        }
        if (IS_XE_PORT(unit, 26) || IS_HG_PORT(unit, 26)) {
            SOC_PBMP_PORT_ADD(*tr_8pg_ports, 26);
        }
        if (IS_XE_PORT(unit, 27) || IS_HG_PORT(unit, 27)) {
            SOC_PBMP_PORT_ADD(*tr_8pg_ports, 27);
        }
        SOC_PBMP_PORT_ADD(*tr_8pg_ports, 28);
        SOC_PBMP_PORT_ADD(*tr_8pg_ports, 29);
        SOC_PBMP_PORT_ADD(*tr_8pg_ports, 30); 
        SOC_PBMP_PORT_ADD(*tr_8pg_ports, 31);

        /* Higig ports [0,2,14,26,27,28,29,30,31] */
        SOC_PBMP_CLEAR(*tr_higig_ports);
        SOC_PBMP_PORT_ADD(*tr_higig_ports, 0);
        SOC_PBMP_PORT_ADD(*tr_higig_ports, 2);
        SOC_PBMP_PORT_ADD(*tr_higig_ports, 14);
        SOC_PBMP_PORT_ADD(*tr_higig_ports, 26);
        SOC_PBMP_PORT_ADD(*tr_higig_ports, 27);
        SOC_PBMP_PORT_ADD(*tr_higig_ports, 28);
        SOC_PBMP_PORT_ADD(*tr_higig_ports, 29);
        SOC_PBMP_PORT_ADD(*tr_higig_ports, 30);
        SOC_PBMP_PORT_ADD(*tr_higig_ports, 31);

        /*  24Q_PORTS = [2,3,14,15,26..32,43] */
        SOC_PBMP_CLEAR(*tr_24q_ports);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 2);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 3);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 14);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 15);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 26);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 27);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 28);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 29);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 30);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 31);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 32);
        SOC_PBMP_PORT_ADD(*tr_24q_ports, 43);

        /* all port except CMIC */
        SOC_PBMP_CLEAR(*tr_non_cpu_ports);
        SOC_PBMP_ASSIGN(*tr_non_cpu_ports, PBMP_ALL(unit));
        SOC_PBMP_PORT_REMOVE(*tr_non_cpu_ports, 0);
 
        /* All ports */
        SOC_PBMP_CLEAR(*tr_all_ports); 
        SOC_PBMP_ASSIGN(*tr_all_ports, PBMP_ALL(unit)); 
    }
    if (!SOC_PORT_VALID(unit, ainfo->port)) {
        return 0;
    }
    _MAP_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);
 
    switch(ainfo->reg) {
        case MAC_CTRLr:
        case MAC_XGXS_CTRLr:
        case MAC_XGXS_STATr:
        case MAC_TXMUXCTRLr:
        case MAC_CNTMAXSZr:
        case MAC_CORESPARE0r:
        case MAC_TXCTRLr:
        case MAC_TXMACSAr:
        case MAC_TXMAXSZr:
        case MAC_TXPSETHRr:
        case MAC_TXSPARE0r:
        case ITPOKr:
        case ITXPFr: 
        case ITFCSr:
        case ITUCr:
        case ITMCAr:
        case ITBCAr:
        case ITOVRr:
        case ITFRGr:
        case ITPKTr:
        case IT64r:
        case IT127r:
        case IT255r:
        case IT511r:
        case IT1023r:
        case IT1518r:
        case IT2047r:
        case IT4095r:
        case IT9216r:
        case IT16383r:
        case ITMAXr:
        case ITUFLr:
        case ITERRr:
        case ITBYTr:
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
        case IRXCFr:
        case IRXPFr:
        case IRXUOr:
        case IRJBRr:
        case IROVRr:
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
        case MAC_TXLLFCCTRLr:
        case MAC_TXLLFCMSGFLDSr:
        case MAC_RXLLFCMSGFLDSr:
        case MAC_RXLLFCMSGCNTr:
            if (IS_GE_PORT(unit, ainfo->port)) {
                goto skip;
            }
            break;
        default:
            break;
    }

    switch(ainfo->reg) {
        case IE2E_CONTROLr:
        case ING_MODMAP_CTRLr:
        case IHG_LOOKUPr:
        case ICONTROL_OPCODE_BITMAPr:
        case UNKNOWN_HGI_BITMAPr:
        case IUNHGIr:
        case ICTRLr:
        case IBCASTr:
        case ILTOMCr:
        case IIPMCr:
        case IUNKOPCr:
            pbmp = tr_higig_ports;
            break;
        case PG_THRESH_SELr:
        case PORT_PRI_GRP0r:
        case PORT_PRI_GRP1r:
        case PORT_PRI_XON_ENABLEr:
            pbmp = tr_8pg_ports;
            break;
        case ECN_CONFIGr:
        case HOL_STAT_PORTr:
        case PORT_WREDPARAM_CELLr:
        case PORT_WREDPARAM_YELLOW_CELLr:
        case PORT_WREDPARAM_RED_CELLr:
        case PORT_WREDPARAM_NONTCP_CELLr:
        case PORT_WREDCONFIG_CELLr:
        case PORT_WREDAVGQSIZE_CELLr:
        case PORT_WREDPARAM_PACKETr:
        case PORT_WREDPARAM_YELLOW_PACKETr:
        case PORT_WREDPARAM_RED_PACKETr:
        case PORT_WREDPARAM_NONTCP_PACKETr:
        case PORT_WREDCONFIG_PACKETr:
        case PORT_WREDAVGQSIZE_PACKETr:
        case ESCONFIGr:
        case COSMASKr:
        case MINSPCONFIGr:
            pbmp = tr_non_cpu_ports;
            break;
        /* ING_PORTS_24Q */
        case ING_COS_MODEr:
        /* MMU_24Q_PORTS */
        case COS_MODEr:
        case S1V_CONFIGr:
        case S1V_COSWEIGHTSr:
        case S1V_COSMASKr:
        case S1V_MINSPCONFIGr:
        case S1V_WDRRCOUNTr:
            pbmp = tr_24q_ports;
            break;
        /* MMU_PERPORTPERCOS_REGS */
        case OP_QUEUE_CONFIG_CELLr:
        case OP_QUEUE_CONFIG1_CELLr:
        case OP_QUEUE_CONFIG_PACKETr:
        case OP_QUEUE_CONFIG1_PACKETr:
        case OP_QUEUE_LIMIT_YELLOW_CELLr:
        case OP_QUEUE_LIMIT_YELLOW_PACKETr:
        case OP_QUEUE_LIMIT_RED_CELLr:
        case OP_QUEUE_LIMIT_RED_PACKETr:
        case OP_QUEUE_RESET_OFFSET_CELLr:
        case OP_QUEUE_RESET_OFFSET_PACKETr:
        case OP_QUEUE_MIN_COUNT_CELLr:
        case OP_QUEUE_MIN_COUNT_PACKETr:
        case OP_QUEUE_SHARED_COUNT_CELLr:
        case OP_QUEUE_SHARED_COUNT_PACKETr:
        case OP_QUEUE_TOTAL_COUNT_CELLr:
        case OP_QUEUE_TOTAL_COUNT_PACKETr:
        case OP_QUEUE_RESET_VALUE_CELLr:
        case OP_QUEUE_RESET_VALUE_PACKETr:
        case DROP_PKT_CNTr:
        case DROP_BYTE_CNTr:
            if (ainfo->port == 0) {
                return 0;
            } else if (ainfo->idx < 8) {
                pbmp = tr_all_ports;
            } else if (ainfo->idx < 24) {
                pbmp = tr_24q_ports;
            } else {
                goto skip;
            }
            break;
        /* MMU_PERPORTPERCOS_NOCPU_REGS */
        case WREDPARAM_CELLr:
        case WREDPARAM_YELLOW_CELLr:
        case WREDPARAM_RED_CELLr:
        case WREDPARAM_NONTCP_CELLr:
        case WREDCONFIG_CELLr:
        case WREDAVGQSIZE_CELLr:
        case WREDPARAM_PACKETr:
        case WREDPARAM_YELLOW_PACKETr:
        case WREDPARAM_RED_PACKETr:
        case WREDPARAM_NONTCP_PACKETr:
        case WREDCONFIG_PACKETr:
        case WREDAVGQSIZE_PACKETr:
        /* MMU_MTRO_REGS */
        case MINBUCKETCONFIG_64r:
        case MINBUCKETr:
        case MAXBUCKETCONFIG_64r:
        case MAXBUCKETr:
        /* MMU_ES_REGS */
        case COSWEIGHTSr:
        case WDRRCOUNTr:
            if (ainfo->port == 0) {
                goto skip;
            } else if (ainfo->idx < 8) {
                pbmp = tr_all_ports;
            } else {
                pbmp = tr_24q_ports;
            }
            break;
        /* MMU_PERPORTPERPRI_REGS */
        case PG_RESET_OFFSET_CELLr:
        case PG_RESET_OFFSET_PACKETr:
        case PG_RESET_FLOOR_CELLr:
        case PG_MIN_CELLr:
        case PG_MIN_PACKETr:
        case PG_HDRM_LIMIT_CELLr:
        case PG_HDRM_LIMIT_PACKETr:
        case PG_COUNT_CELLr:
        case PG_COUNT_PACKETr:
        case PG_MIN_COUNT_CELLr:
        case PG_MIN_COUNT_PACKETr:
        case PG_PORT_MIN_COUNT_CELLr:
        case PG_PORT_MIN_COUNT_PACKETr:
        case PG_SHARED_COUNT_CELLr:
        case PG_SHARED_COUNT_PACKETr:
        case PG_HDRM_COUNT_CELLr:
        case PG_HDRM_COUNT_PACKETr:
        case PG_GBL_HDRM_COUNTr:
        case PG_RESET_VALUE_CELLr:
        case PG_RESET_VALUE_PACKETr:
            if (ainfo->idx == 0) {
                pbmp = tr_all_ports;
            } else {
                pbmp = tr_8pg_ports;
            }
            break;
        default:
            pbmp = tr_all_ports;
            break; 
    }

    if (!SOC_PBMP_MEMBER(*pbmp, ainfo->port)) {
        goto skip; 
    }

    if (mask != NULL) {
        switch(ainfo->reg) {
            case HOL_STAT_PORTr:
            case ECN_CONFIGr:
                if (!SOC_PBMP_MEMBER(*tr_24q_ports, ainfo->port)) {
                    /* adjust mask for ports without 24 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x000000ff);
                    COMPILER_64_AND(*mask, temp_mask);
                }
                break;
            case TOQ_QUEUESTAT_64r:
            case TOQ_ACTIVATEQ_64r:
            case TOQEMPTY_64r:
            case DEQ_AGINGMASK_64r:
                if (SOC_PBMP_MEMBER(*tr_24q_ports, ainfo->port)) {
                    /* adjust mask for ports with 24 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x00ffffff);
                    COMPILER_64_AND(*mask, temp_mask);
                } else if (ainfo->port != 0) {
                    /* remaining ports only have 8 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x000000ff);
                    COMPILER_64_AND(*mask, temp_mask);
                }
                break;
        }
    }
    return 0; 
skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }
    return 1;
}
#endif /* BCM_TRIUMPH_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
STATIC int
reg_mask_subset_sh(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    pbmp_t *sh_non_cpu_ports;
    pbmp_t *sh_non_il_non_cpu_ports;
    pbmp_t *sh_port_link_ports;
    pbmp_t *sh_il_ports;
    pbmp_t *sh_all_ports;
#define _REG_MASK_SUBSET_MAX_SH 5
    pbmp_t **mask_map_arr[_REG_MASK_SUBSET_MAX_SH];
    int i, count = _REG_MASK_SUBSET_MAX_SH;
    pbmp_t *pbmp;

    mask_map_arr[0] = &sh_non_cpu_ports;
    mask_map_arr[1] = &sh_non_il_non_cpu_ports;
    mask_map_arr[2] = &sh_port_link_ports;
    mask_map_arr[3] = &sh_il_ports;
    mask_map_arr[4] = &sh_all_ports;
    if (NULL == SOC_REG_MASK_SUBSET(unit)) {
        _INIT_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);
        
        /* all port except CMIC */
        SOC_PBMP_CLEAR(*sh_non_cpu_ports);
        SOC_PBMP_ASSIGN(*sh_non_cpu_ports, PBMP_ALL(unit));
        SOC_PBMP_PORT_REMOVE(*sh_non_cpu_ports, 0);

        /* All ports except Interlaken and CPU */
        SOC_PBMP_CLEAR(*sh_non_il_non_cpu_ports);
        SOC_PBMP_ASSIGN(*sh_non_il_non_cpu_ports, PBMP_PORT_ALL(unit));
        SOC_PBMP_REMOVE(*sh_non_il_non_cpu_ports, PBMP_IL_ALL(unit));

        /* All port link ports */
        SOC_PBMP_CLEAR(*sh_port_link_ports);
        SOC_PBMP_ASSIGN(*sh_port_link_ports, PBMP_ALL(unit));
        SOC_PBMP_PORT_REMOVE(*sh_port_link_ports, 0);
        SOC_PBMP_WORD_SET(*sh_port_link_ports, 0, 
                          (SOC_PBMP_WORD_GET(*sh_port_link_ports, 0) & 0x1ff));

        /* All Interlaken ports */
        SOC_PBMP_CLEAR(*sh_il_ports);
        SOC_PBMP_ASSIGN(*sh_il_ports, PBMP_PORT_ALL(unit));
        SOC_PBMP_REMOVE(*sh_il_ports, *sh_port_link_ports);

        /* All ports */
        SOC_PBMP_CLEAR(*sh_all_ports);
        SOC_PBMP_ASSIGN(*sh_all_ports, PBMP_ALL(unit));
    }
    if (!SOC_PORT_VALID(unit, ainfo->port)) {
        return 0;
    }
    _MAP_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);

    switch(ainfo->reg) {
        case BUCKET_ECCr:
        case CONFIG_ECCr:
        case HOL_STAT_PORTr:
        case TOQ_QUEUESTATr:
        case TOQ_ACTIVATEQr:
        case TOQEMPTYr:
        case ECN_CONFIGr:
        case ES_PORTGRP_WDRR_WEIGHTSr:
        case MTRO_SHAPE_MINMASKr:
        case MMU_TO_XLP_BKP_STATUSr:
        case XLP_TO_MMU_BKP_STATUSr:
        case PORT_WREDPARAM_CELLr:
        case PORT_WREDPARAM_YELLOW_CELLr:
        case PORT_WREDPARAM_RED_CELLr:
        case PORT_WREDPARAM_NONTCP_CELLr:
        case PORT_WREDCONFIG_CELLr:
        case PORT_WREDAVGQSIZE_CELLr:
        case PORT_LB_WREDAVGQSIZE_CELLr:
        case PORT_WREDCONFIG_ECCPr:
        case PORT_WREDPARAM_END_CELLr:
        case PORT_WREDPARAM_PRI0_END_CELLr:
        case PORT_WREDPARAM_PRI0_START_CELLr:
        case PORT_WREDPARAM_RED_END_CELLr:
        case PORT_WREDPARAM_RED_START_CELLr:
        case PORT_WREDPARAM_START_CELLr:
        case PORT_WREDPARAM_YELLOW_END_CELLr:
        case PORT_WREDPARAM_YELLOW_START_CELLr:
        case PORT_WRED_THD_0_ECCPr:
        case PORT_WRED_THD_1_ECCPr:
        case WREDPARAM_CELLr:
        case WREDPARAM_YELLOW_CELLr:
        case WREDPARAM_RED_CELLr:
        case WREDPARAM_NONTCP_CELLr:
        case WREDCONFIG_CELLr:
        case WREDAVGQSIZE_CELLr:
        case WREDPARAM_END_CELLr:
        case WREDPARAM_PRI0_END_CELLr:
        case WREDPARAM_PRI0_START_CELLr:
        case WREDPARAM_RED_END_CELLr:
        case WREDPARAM_RED_START_CELLr:
        case WREDPARAM_START_CELLr:
        case WREDPARAM_YELLOW_END_CELLr:
        case WREDPARAM_YELLOW_START_CELLr:
        case WREDCONFIG_ECCPr:
        case WRED_DEBUG_ENQ_DROP_PORTr:
        case WRED_THD_0_ECCPr:
        case WRED_THD_1_ECCPr:
        case COSWEIGHTSr:
        case MINSPCONFIGr:
        case WDRRCOUNTr:
        case MINBUCKETCONFIGr:
        case MINBUCKETCONFIG1r:
        case MINBUCKETr:
        case MAXBUCKETCONFIGr:
        case MAXBUCKETCONFIG1r:
        case MAXBUCKETr:
        case MMU_TO_XPORT_BKPr:
        case DEQ_AGINGMASKr:
        case MMU_LLFC_RX_CONFIGr:
        case XPORT_TO_MMU_BKPr:
        case UNIMAC_PFC_CTRLr:
        case MAC_PFC_REFRESH_CTRLr:
        case MAC_PFC_TYPEr:
        case MAC_PFC_OPCODEr:
        case MAC_PFC_DA_0r:
        case MAC_PFC_DA_1r:
        case MACSEC_PROG_TX_CRCr:
        case MACSEC_CNTRLr:
        case TS_STATUS_CNTRLr:
        case TX_TS_DATAr:
        case BMAC_PFC_CTRLr:
        case BMAC_PFC_TYPEr:
        case BMAC_PFC_OPCODEr:
        case BMAC_PFC_DA_LOr:
        case BMAC_PFC_DA_HIr:
        case BMAC_PFC_COS0_XOFF_CNTr:
        case BMAC_PFC_COS1_XOFF_CNTr:
        case BMAC_PFC_COS2_XOFF_CNTr:
        case BMAC_PFC_COS3_XOFF_CNTr:
        case BMAC_PFC_COS4_XOFF_CNTr:
        case BMAC_PFC_COS5_XOFF_CNTr:
        case BMAC_PFC_COS6_XOFF_CNTr:
        case BMAC_PFC_COS7_XOFF_CNTr:
        case BMAC_PFC_COS8_XOFF_CNTr:
        case BMAC_PFC_COS9_XOFF_CNTr:
        case BMAC_PFC_COS10_XOFF_CNTr:
        case BMAC_PFC_COS11_XOFF_CNTr:
        case BMAC_PFC_COS12_XOFF_CNTr:
        case BMAC_PFC_COS13_XOFF_CNTr:
        case BMAC_PFC_COS14_XOFF_CNTr:
        case BMAC_PFC_COS15_XOFF_CNTr:
        case PFC_COS0_XOFF_CNTr:
        case PFC_COS1_XOFF_CNTr:
        case PFC_COS2_XOFF_CNTr:
        case PFC_COS3_XOFF_CNTr:
        case PFC_COS4_XOFF_CNTr:
        case PFC_COS5_XOFF_CNTr:
        case PFC_COS6_XOFF_CNTr:
        case PFC_COS7_XOFF_CNTr:
        case PFC_COS8_XOFF_CNTr:
        case PFC_COS9_XOFF_CNTr:
        case PFC_COS10_XOFF_CNTr:
        case PFC_COS11_XOFF_CNTr:
        case PFC_COS12_XOFF_CNTr:
        case PFC_COS13_XOFF_CNTr:
        case PFC_COS14_XOFF_CNTr:
        case PFC_COS15_XOFF_CNTr:
            pbmp = sh_non_cpu_ports;
            break;
        default:
            pbmp = sh_all_ports;
            break;
    }

    switch(ainfo->reg) {
        /* EP_PERPORTPERCOS_REGS */
        case EGR_PERQ_XMT_COUNTERSr:
        /* MMU_PERPORTPERCOS_REGS */
        case OP_QUEUE_CONFIGr:
        case OP_QUEUE_RESET_OFFSETr:
        case OP_QUEUE_MIN_COUNTr:
        case OP_QUEUE_SHARED_COUNTr:
        case OP_QUEUE_TOTAL_COUNTr:
        case OP_QUEUE_RESET_VALUEr:
        case OP_QUEUE_LIMIT_YELLOWr:
        case OP_QUEUE_LIMIT_REDr:
        case HOLDROP_PKT_CNTr:
            if (ainfo->port == 0) {
                return 0;
            } else if (ainfo->idx < 10) { 
                pbmp = sh_all_ports;
            } else {
                goto skip;
            }
            break;
        default:
            break;
    }

    if (SOC_REG_BLOCK_IS(unit, ainfo->reg, SOC_BLK_XLPORT)) {
        pbmp = sh_non_il_non_cpu_ports;
    } else if ((SOC_REG_BLOCK_IS(unit, ainfo->reg, SOC_BLK_MS_ISEC)) ||
               (SOC_REG_BLOCK_IS(unit, ainfo->reg, SOC_BLK_MS_ESEC))) { 
        pbmp = sh_port_link_ports;
    } else if (SOC_REG_BLOCK_IS(unit, ainfo->reg, SOC_BLK_IL)) {
        pbmp = sh_il_ports;
    }

    if (!SOC_PBMP_MEMBER(*pbmp, ainfo->port)) {
        goto skip;
    }
    return 0;

skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }
    return 1;
}
#endif /* BCM_SHADOW_SUPPORT */

#ifdef BCM_SCORPION_SUPPORT
STATIC int
reg_mask_subset_sc(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    pbmp_t *sc_no_ports;
    pbmp_t *sc_xports;
    pbmp_t *sc_non_cpu_ports;
    pbmp_t *sc_all_ports;
#define _REG_MASK_SUBSET_MAX_SC 4
    pbmp_t **mask_map_arr[_REG_MASK_SUBSET_MAX_SC];
    int i, count = _REG_MASK_SUBSET_MAX_SC;
    pbmp_t *pbmp;

    mask_map_arr[0] = &sc_no_ports;
    mask_map_arr[1] = &sc_xports;
    mask_map_arr[2] = &sc_non_cpu_ports;
    mask_map_arr[3] = &sc_all_ports;
    if (NULL == SOC_REG_MASK_SUBSET(unit)) {
        _INIT_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);
        
        /* all port except CMIC */
        SOC_PBMP_CLEAR(*sc_non_cpu_ports);
        SOC_PBMP_ASSIGN(*sc_non_cpu_ports, PBMP_ALL(unit));
        SOC_PBMP_PORT_REMOVE(*sc_non_cpu_ports, 0);

        SOC_PBMP_CLEAR(*sc_xports);
        SOC_PBMP_ASSIGN(*sc_xports, PBMP_GX_ALL(unit));
        SOC_PBMP_PORT_REMOVE(*sc_xports, 0);

        /* All ports */
        SOC_PBMP_CLEAR(*sc_all_ports);
        SOC_PBMP_ASSIGN(*sc_all_ports, PBMP_ALL(unit));

        /* No ports */
        SOC_PBMP_CLEAR(*sc_no_ports);
    }
    if (!SOC_PORT_VALID(unit, ainfo->port)) {
        return 0;
    }
    _MAP_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);

    switch(ainfo->reg) {
        case HOL_STAT_PORTr:
        case TOQ_QUEUESTATr:
        case TOQ_ACTIVATEQr:
        case TOQEMPTYr:
        case ECN_CONFIGr:
        case PORT_WREDPARAM_CELLr:
        case PORT_WREDPARAM_YELLOW_CELLr:
        case PORT_WREDPARAM_RED_CELLr:
        case PORT_WREDPARAM_NONTCP_CELLr:
        case PORT_WREDCONFIG_CELLr:
        case PORT_WREDAVGQSIZE_CELLr:
        case WREDPARAM_CELLr:
        case WREDPARAM_YELLOW_CELLr:
        case WREDPARAM_RED_CELLr:
        case WREDPARAM_NONTCP_CELLr:
        case WREDCONFIG_CELLr:
        case WREDAVGQSIZE_CELLr:
        case COSWEIGHTSr:
        case MINSPCONFIGr:
        case WDRRCOUNTr:
        case MINBUCKETCONFIGr:
        case MINBUCKETCONFIG1r:
        case MINBUCKETr:
        case MAXBUCKETCONFIGr:
        case MAXBUCKETCONFIG1r:
        case MAXBUCKETr:
        case MMU_TO_XPORT_BKPr:
        case DEQ_AGINGMASKr:
            pbmp = sc_non_cpu_ports;
            break;
        case MMU_LLFC_RX_CONFIGr:
        case XPORT_TO_MMU_BKPr:
            pbmp = sc_xports;
            break;
        case UNIMAC_PFC_CTRLr:
        case MAC_PFC_REFRESH_CTRLr:
        case MAC_PFC_TYPEr:
        case MAC_PFC_OPCODEr:
        case MAC_PFC_DA_0r:
        case MAC_PFC_DA_1r:
        case MACSEC_PROG_TX_CRCr:
        case MACSEC_CNTRLr:
        case TS_STATUS_CNTRLr:
        case TX_TS_DATAr:
            if (soc_feature(unit, soc_feature_priority_flow_control)) {
                pbmp = sc_non_cpu_ports;
            } else {
                pbmp = sc_no_ports;
            }
            break;
        case BMAC_PFC_CTRLr:
        case BMAC_PFC_TYPEr:
        case BMAC_PFC_OPCODEr:
        case BMAC_PFC_DA_LOr:
        case BMAC_PFC_DA_HIr:
        case BMAC_PFC_COS0_XOFF_CNTr:
        case BMAC_PFC_COS1_XOFF_CNTr:
        case BMAC_PFC_COS2_XOFF_CNTr:
        case BMAC_PFC_COS3_XOFF_CNTr:
        case BMAC_PFC_COS4_XOFF_CNTr:
        case BMAC_PFC_COS5_XOFF_CNTr:
        case BMAC_PFC_COS6_XOFF_CNTr:
        case BMAC_PFC_COS7_XOFF_CNTr:
        case BMAC_PFC_COS8_XOFF_CNTr:
        case BMAC_PFC_COS9_XOFF_CNTr:
        case BMAC_PFC_COS10_XOFF_CNTr:
        case BMAC_PFC_COS11_XOFF_CNTr:
        case BMAC_PFC_COS12_XOFF_CNTr:
        case BMAC_PFC_COS13_XOFF_CNTr:
        case BMAC_PFC_COS14_XOFF_CNTr:
        case BMAC_PFC_COS15_XOFF_CNTr:
        case PFC_COS0_XOFF_CNTr:
        case PFC_COS1_XOFF_CNTr:
        case PFC_COS2_XOFF_CNTr:
        case PFC_COS3_XOFF_CNTr:
        case PFC_COS4_XOFF_CNTr:
        case PFC_COS5_XOFF_CNTr:
        case PFC_COS6_XOFF_CNTr:
        case PFC_COS7_XOFF_CNTr:
        case PFC_COS8_XOFF_CNTr:
        case PFC_COS9_XOFF_CNTr:
        case PFC_COS10_XOFF_CNTr:
        case PFC_COS11_XOFF_CNTr:
        case PFC_COS12_XOFF_CNTr:
        case PFC_COS13_XOFF_CNTr:
        case PFC_COS14_XOFF_CNTr:
        case PFC_COS15_XOFF_CNTr:
            if (soc_feature(unit, soc_feature_priority_flow_control)) {
                pbmp = sc_xports;
            } else {
                pbmp = sc_no_ports;
            }
            break;
        default:
            pbmp = sc_all_ports;
            break;
    }

    switch(ainfo->reg) {
        /* EP_PERPORTPERCOS_REGS */
        case EGR_PERQ_XMT_COUNTERSr:
        /* MMU_PERPORTPERCOS_REGS */
        case OP_QUEUE_CONFIGr:
        case OP_QUEUE_RESET_OFFSETr:
        case OP_QUEUE_MIN_COUNTr:
        case OP_QUEUE_SHARED_COUNTr:
        case OP_QUEUE_TOTAL_COUNTr:
        case OP_QUEUE_RESET_VALUEr:
        case OP_QUEUE_LIMIT_YELLOWr:
        case OP_QUEUE_LIMIT_REDr:
        case HOLDROP_PKT_CNTr:
            if (ainfo->port == 0) {
                return 0;
            } else if (ainfo->idx < 10) { 
                pbmp = sc_all_ports;
            } else {
                goto skip;
            }
            break;
        default:
            break;
    }
    
    if (!SOC_PBMP_MEMBER(*pbmp, ainfo->port)) {
        goto skip;
    }
    return 0;

skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }
    return 1;
}
#endif /* BCM_SCORPION_SUPPORT */

#ifdef BCM_TRIUMPH2_SUPPORT
STATIC int
reg_mask_subset_tr2(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    pbmp_t *ch_2pg_ports;
    pbmp_t *ch_8pg_ports;
    pbmp_t *ch_24q_ports;
    pbmp_t *ch_24q_ports_with_cpu;
    pbmp_t *ch_ext_ports;
    pbmp_t *ch_non_cpu_ports;
    pbmp_t *ch_all_ports;
    pbmp_t *ch_lb_ports;
    pbmp_t *ch_all_ports_with_mmu;
    pbmp_t *ch_gxports;
    pbmp_t *ch_cmic;
#define _REG_MASK_SUBSET_MAX_TR2 11
    pbmp_t **mask_map_arr[_REG_MASK_SUBSET_MAX_TR2];
    int i, count = _REG_MASK_SUBSET_MAX_TR2;
    uint64 temp_mask;
    pbmp_t *pbmp;

    mask_map_arr[0] = &ch_2pg_ports;
    mask_map_arr[1] = &ch_8pg_ports;
    mask_map_arr[2] = &ch_24q_ports;
    mask_map_arr[3] = &ch_24q_ports_with_cpu;
    mask_map_arr[4] = &ch_ext_ports;
    mask_map_arr[5] = &ch_non_cpu_ports;
    mask_map_arr[6] = &ch_all_ports;
    mask_map_arr[7] = &ch_lb_ports;
    mask_map_arr[8] = &ch_all_ports_with_mmu;
    mask_map_arr[9] = &ch_gxports;
    mask_map_arr[10] = &ch_cmic;
    if (NULL == SOC_REG_MASK_SUBSET(unit)) {
        _INIT_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);

        /* 8PG_PORTS = [26..30,34,38,42,46,50,54] */
        SOC_PBMP_CLEAR(*ch_8pg_ports);
        SOC_PBMP_PORT_ADD(*ch_8pg_ports, 26);
        SOC_PBMP_PORT_ADD(*ch_8pg_ports, 27);
        SOC_PBMP_PORT_ADD(*ch_8pg_ports, 28);
        SOC_PBMP_PORT_ADD(*ch_8pg_ports, 29);
        SOC_PBMP_PORT_ADD(*ch_8pg_ports, 30); 
        SOC_PBMP_PORT_ADD(*ch_8pg_ports, 34);
        SOC_PBMP_PORT_ADD(*ch_8pg_ports, 38);
        SOC_PBMP_PORT_ADD(*ch_8pg_ports, 42);
        SOC_PBMP_PORT_ADD(*ch_8pg_ports, 46);
        SOC_PBMP_PORT_ADD(*ch_8pg_ports, 50);
        SOC_PBMP_PORT_ADD(*ch_8pg_ports, 54);

        /*  24Q_PORTS = [26..31,34,38,39,42,43,46,50,51,54] */
        SOC_PBMP_CLEAR(*ch_24q_ports);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 26);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 27);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 28);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 29);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 30);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 31);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 34);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 38);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 39);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 42);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 43);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 46);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 50);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 51);
        SOC_PBMP_PORT_ADD(*ch_24q_ports, 54);
        SOC_PBMP_ASSIGN(*ch_24q_ports_with_cpu, *ch_24q_ports);
        SOC_PBMP_PORT_ADD(*ch_24q_ports_with_cpu, 0);

        /* 2PG_PORTS = [0,1..25,31..33,35..37,39..41,43..45,47..49,51..53,55,56] */
        SOC_PBMP_CLEAR(*ch_2pg_ports);
        SOC_PBMP_ASSIGN(*ch_2pg_ports, PBMP_ALL(unit));
        SOC_PBMP_PORT_REMOVE(*ch_2pg_ports, 26);
        SOC_PBMP_PORT_REMOVE(*ch_2pg_ports, 27);
        SOC_PBMP_PORT_REMOVE(*ch_2pg_ports, 28);
        SOC_PBMP_PORT_REMOVE(*ch_2pg_ports, 29);
        SOC_PBMP_PORT_REMOVE(*ch_2pg_ports, 30); 
        SOC_PBMP_PORT_REMOVE(*ch_2pg_ports, 34);
        SOC_PBMP_PORT_REMOVE(*ch_2pg_ports, 38);
        SOC_PBMP_PORT_REMOVE(*ch_2pg_ports, 42);
        SOC_PBMP_PORT_REMOVE(*ch_2pg_ports, 46);
        SOC_PBMP_PORT_REMOVE(*ch_2pg_ports, 50);
        SOC_PBMP_PORT_REMOVE(*ch_2pg_ports, 54);
        SOC_PBMP_PORT_ADD(*ch_2pg_ports, 55);
        SOC_PBMP_PORT_ADD(*ch_2pg_ports, 56);

        /* GXPORTS = [26, 27, 28, 29] */
        SOC_PBMP_CLEAR(*ch_gxports);
        SOC_PBMP_PORT_ADD(*ch_gxports, 26);
        SOC_PBMP_PORT_ADD(*ch_gxports, 27);
        SOC_PBMP_PORT_ADD(*ch_gxports, 28);
        SOC_PBMP_PORT_ADD(*ch_gxports, 29);
       
        /* Loopback pbmp */
        SOC_PBMP_CLEAR(*ch_lb_ports);
        SOC_PBMP_ASSIGN(*ch_lb_ports, PBMP_LB(unit));

        /* All ports except CMIC */
        SOC_PBMP_CLEAR(*ch_non_cpu_ports);
        SOC_PBMP_ASSIGN(*ch_non_cpu_ports, PBMP_ALL(unit));
        SOC_PBMP_PORT_REMOVE(*ch_non_cpu_ports, 0);

        /* All ports except CMIC and loopback - external ports */
        SOC_PBMP_CLEAR(*ch_ext_ports);
        SOC_PBMP_ASSIGN(*ch_ext_ports, PBMP_ALL(unit));
        SOC_PBMP_PORT_REMOVE(*ch_ext_ports, 0);
        SOC_PBMP_PORT_REMOVE(*ch_ext_ports, 54);
 
        /* All ports (excluding the internal MMU ports) */
        SOC_PBMP_CLEAR(*ch_all_ports); 
        SOC_PBMP_ASSIGN(*ch_all_ports, PBMP_ALL(unit)); 

        /* All ports (including the internal MMU ports) */
        SOC_PBMP_CLEAR(*ch_all_ports_with_mmu); 
        SOC_PBMP_ASSIGN(*ch_all_ports_with_mmu, PBMP_ALL(unit)); 
        SOC_PBMP_PORT_ADD(*ch_all_ports_with_mmu, 55);
        SOC_PBMP_PORT_ADD(*ch_all_ports_with_mmu, 56);

        /* CPU port */
        SOC_PBMP_CLEAR(*ch_cmic); 
        SOC_PBMP_ASSIGN(*ch_cmic, PBMP_CMIC(unit));
    }
    if (!SOC_PORT_VALID(unit, ainfo->port)) {
        return 0;
    }
    _MAP_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);
 
    switch(ainfo->reg) {
        case MAC_CTRLr:
        case MAC_XGXS_CTRLr:
        case MAC_XGXS_STATr:
        case MAC_TXMUXCTRLr:
        case MAC_CNTMAXSZr:
        case MAC_CORESPARE0r:
        case MAC_TXCTRLr:
        case MAC_TXMACSAr:
        case MAC_TXMAXSZr:
        case MAC_TXPSETHRr:
        case MAC_TXSPARE0r:
        case MAC_TXPPPCTRLr:
        case ITPOKr:
        case ITXPFr: 
        case ITFCSr:
        case ITXPPr:
        case ITUCr:
        case ITMCAr:
        case ITBCAr:
        case ITOVRr:
        case ITFRGr:
        case ITPKTr:
        case IT64r:
        case IT127r:
        case IT255r:
        case IT511r:
        case IT1023r:
        case IT1518r:
        case IT2047r:
        case IT4095r:
        case IT9216r:
        case IT16383r:
        case ITMAXr:
        case ITUFLr:
        case ITERRr:
        case ITBYTr:
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
        case IRXCFr:
        case IRXPFr:
        case IRXPPr:
        case IRXUOr:
        case IRJBRr:
        case IROVRr:
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
        case MAC_TXLLFCCTRLr:
        case MAC_TXLLFCMSGFLDSr:
        case MAC_RXLLFCMSGFLDSr:
        case MAC_RXLLFCMSGCNTr:
            if (IS_GE_PORT(unit, ainfo->port)) {
                goto skip;
            }
            break;
        case EGR_PORT_REQUESTSr:
            if (IS_LB_PORT(unit, ainfo->port)) {
                goto skip;
            }
            break;
        case MAC_PFC_COS0_XOFF_CNTr:
        case MAC_PFC_COS1_XOFF_CNTr:
        case MAC_PFC_COS2_XOFF_CNTr:
        case MAC_PFC_COS3_XOFF_CNTr:
        case MAC_PFC_COS4_XOFF_CNTr:
        case MAC_PFC_COS5_XOFF_CNTr:
        case MAC_PFC_COS6_XOFF_CNTr:
        case MAC_PFC_COS7_XOFF_CNTr:
        case MAC_PFC_COS8_XOFF_CNTr:
        case MAC_PFC_COS9_XOFF_CNTr:
        case MAC_PFC_COS10_XOFF_CNTr:
        case MAC_PFC_COS11_XOFF_CNTr:
        case MAC_PFC_COS12_XOFF_CNTr:
        case MAC_PFC_COS13_XOFF_CNTr:
        case MAC_PFC_COS14_XOFF_CNTr:
        case MAC_PFC_COS15_XOFF_CNTr:
            /* Not implemented in HW */
            goto skip;
            break;
        default:
            break;
    }

    switch(ainfo->reg) {
        case PG_THRESH_SELr:
        case PORT_PRI_GRP0r:
        case PORT_PRI_GRP1r:
        case PORT_PRI_XON_ENABLEr:
            pbmp = ch_8pg_ports;
            break;
        case PG_THRESH_SEL2r:
        case PORT_PRI_GRP2r:
            pbmp = ch_2pg_ports;
            break;
        case ECN_CONFIGr:
        case HOL_STAT_PORTr:
        case PORT_WREDPARAM_CELLr:
        case PORT_WREDPARAM_YELLOW_CELLr:
        case PORT_WREDPARAM_RED_CELLr:
        case PORT_WREDPARAM_NONTCP_CELLr:
        case PORT_WREDCONFIG_CELLr:
        case PORT_WREDAVGQSIZE_CELLr:
        case PORT_WREDPARAM_PACKETr:
        case PORT_WREDPARAM_YELLOW_PACKETr:
        case PORT_WREDPARAM_RED_PACKETr:
        case PORT_WREDPARAM_NONTCP_PACKETr:
        case PORT_WREDCONFIG_PACKETr:
        case PORT_WREDAVGQSIZE_PACKETr:
        case BKPMETERINGCONFIG_64r:
        case BKPMETERINGBUCKETr:
        case MTRI_IFGr:
            pbmp = ch_ext_ports;
            break;
        case COSMASKr:
        case MINSPCONFIGr:
            pbmp = ch_non_cpu_ports;
            break;
        case S1V_CONFIGr:
        case S1V_COSWEIGHTSr:
        case S1V_COSMASKr:
        case S1V_MINSPCONFIGr:
        case S1V_WDRRCOUNTr:
            pbmp = ch_24q_ports;
            break;
        /* MMU_WLP_PERCOS_REGS */
        case OP_QUEUE_FIRST_FRAGMENT_CONFIG_CELLr:
        case OP_QUEUE_FIRST_FRAGMENT_RESET_OFFSET_CELLr:
        case OP_QUEUE_FIRST_FRAGMENT_CONFIG_PACKETr:
        case OP_QUEUE_FIRST_FRAGMENT_RESET_OFFSET_PACKETr:
        case OP_QUEUE_FIRST_FRAGMENT_COUNT_CELLr:
        case OP_QUEUE_FIRST_FRAGMENT_COUNT_PACKETr:
        case OP_QUEUE_REDIRECT_CONFIG_CELLr:
        case OP_QUEUE_REDIRECT_RESET_OFFSET_CELLr:
        case OP_QUEUE_REDIRECT_CONFIG_PACKETr:
        case OP_QUEUE_REDIRECT_RESET_OFFSET_PACKETr:
        case OP_QUEUE_REDIRECT_COUNT_CELLr:
        case OP_QUEUE_REDIRECT_COUNT_PACKETr:
        case OP_QUEUE_REDIRECT_XQ_CONFIG_PACKETr:
        case OP_QUEUE_REDIRECT_XQ_RESET_OFFSET_PACKETr:
        case OP_QUEUE_REDIRECT_XQ_COUNT_PACKETr:
        case OP_PORT_FIRST_FRAGMENT_DISC_RESUME_THD_CELLr:
        case OP_PORT_FIRST_FRAGMENT_DISC_RESUME_THD_PACKETr:
        case OP_PORT_FIRST_FRAGMENT_DISC_SET_THD_CELLr:
        case OP_PORT_FIRST_FRAGMENT_DISC_SET_THD_PACKETr:
        case OP_PORT_FIRST_FRAGMENT_COUNT_CELLr:
        case OP_PORT_FIRST_FRAGMENT_COUNT_PACKETr:
        case FIRST_FRAGMENT_DROP_STATE_CELLr:
        case FIRST_FRAGMENT_DROP_STATE_PACKETr:
        case OP_PORT_REDIRECT_COUNT_CELLr:
        case OP_PORT_REDIRECT_COUNT_PACKETr:
        case REDIRECT_DROP_STATE_CELLr:
        case REDIRECT_DROP_STATE_PACKETr:
        case REDIRECT_XQ_DROP_STATE_PACKETr: 
        case OP_PORT_REDIRECT_DISC_RESUME_THD_CELLr:
        case OP_PORT_REDIRECT_DISC_RESUME_THD_PACKETr:
        case OP_PORT_REDIRECT_DISC_SET_THD_CELLr:
        case OP_PORT_REDIRECT_DISC_SET_THD_PACKETr:
        case OP_PORT_REDIRECT_XQ_DISC_RESUME_THD_PACKETr:
        case OP_PORT_REDIRECT_XQ_DISC_SET_THD_PACKETr:
        case OP_PORT_REDIRECT_XQ_COUNT_PACKETr:
            pbmp = ch_lb_ports;
            break;
        case PORT_MIN_CELLr:
        case PORT_MIN_PACKETr:
        case PORT_SHARED_LIMIT_CELLr:
        case PORT_SHARED_LIMIT_PACKETr:
        case PORT_COUNT_CELLr:
        case PORT_COUNT_PACKETr:
        case PORT_MIN_COUNT_CELLr:
        case PORT_MIN_COUNT_PACKETr:
        case PORT_SHARED_COUNT_CELLr:
        case PORT_SHARED_COUNT_PACKETr:
            pbmp = ch_all_ports_with_mmu;
            break;
        default:
            pbmp = ch_all_ports;
            break;
    }

    switch(ainfo->reg) {
        /* MMU_PERPORTPERCOS_REGS */
        case OP_QUEUE_CONFIG_CELLr:
        case OP_QUEUE_CONFIG1_CELLr:
        case OP_QUEUE_CONFIG_PACKETr:
        case OP_QUEUE_CONFIG1_PACKETr:
        case OP_QUEUE_LIMIT_YELLOW_CELLr:
        case OP_QUEUE_LIMIT_YELLOW_PACKETr:
        case OP_QUEUE_LIMIT_RED_CELLr:
        case OP_QUEUE_LIMIT_RED_PACKETr:
        case OP_QUEUE_RESET_OFFSET_CELLr:
        case OP_QUEUE_RESET_OFFSET_PACKETr:
        case OP_QUEUE_MIN_COUNT_CELLr:
        case OP_QUEUE_MIN_COUNT_PACKETr:
        case OP_QUEUE_SHARED_COUNT_CELLr:
        case OP_QUEUE_SHARED_COUNT_PACKETr:
        case OP_QUEUE_TOTAL_COUNT_CELLr:
        case OP_QUEUE_TOTAL_COUNT_PACKETr:
        case OP_QUEUE_RESET_VALUE_CELLr:
        case OP_QUEUE_RESET_VALUE_PACKETr:
        case OP_QUEUE_LIMIT_RESUME_COLOR_CELLr:
        case OP_QUEUE_LIMIT_RESUME_COLOR_PACKETr:
        /* MMU_PERPORTPERCOS_REGS_CTR */
        case DROP_PKT_CNTr:
        case DROP_BYTE_CNTr:
            if (ainfo->port == 0) {
                return 0;
            } else if (ainfo->idx < 8) {
                pbmp = ch_all_ports;
            } else if ((ainfo->idx < 24) && (ainfo->port == 54)) {
                goto skip;
            } else if ((ainfo->idx < 24)) {
                pbmp = ch_24q_ports;
            } else {
                pbmp = ch_cmic;
            }
            break;

        /* MMU_PERPORTPERCOS_NOCPU_REGS */
        case WREDPARAM_CELLr:
        case WREDPARAM_YELLOW_CELLr:
        case WREDPARAM_RED_CELLr:
        case WREDPARAM_NONTCP_CELLr:
        case WREDCONFIG_CELLr:
        case WREDAVGQSIZE_CELLr:
        case WREDPARAM_PACKETr:
        case WREDPARAM_YELLOW_PACKETr:
        case WREDPARAM_RED_PACKETr:
        case WREDPARAM_NONTCP_PACKETr:
        case WREDCONFIG_PACKETr:
        case WREDAVGQSIZE_PACKETr:
            if (ainfo->port == 0) {
                goto skip;
            } else if (ainfo->idx < 8) {
                pbmp = ch_non_cpu_ports;
            } else {
                pbmp = ch_24q_ports;
            }
            break;

        /* MMU_MTRO_REGS */
        case MINBUCKETCONFIG_64r:
        case MINBUCKETr:
        case MAXBUCKETCONFIG_64r:
        case MAXBUCKETr:
            if (ainfo->idx < 8) {
                pbmp = ch_all_ports;
            } else if (ainfo->idx < 26) {
                pbmp = ch_24q_ports_with_cpu;
            } else {
                pbmp = ch_cmic;
            }
            break;

        /* MMU_ES_REGS */
        case COSWEIGHTSr:
        case WDRRCOUNTr:
            if (ainfo->idx < 8) {
                pbmp = ch_all_ports;
            } else if (ainfo->idx < 10) {
                pbmp = ch_24q_ports_with_cpu;
            } else {
                pbmp = ch_cmic;
            }
            break;

        /* MMU_PERPORTPERPRI_REGS */
        case PG_RESET_OFFSET_CELLr:
        case PG_RESET_OFFSET_PACKETr:
        case PG_RESET_FLOOR_CELLr:
        
        case PG_MIN_CELLr:
        case PG_MIN_PACKETr:
        case PG_HDRM_LIMIT_CELLr:
        case PG_HDRM_LIMIT_PACKETr:
        case PG_COUNT_CELLr:
        case PG_COUNT_PACKETr:
        case PG_MIN_COUNT_CELLr:
        case PG_MIN_COUNT_PACKETr:
        case PG_PORT_MIN_COUNT_CELLr:
        case PG_PORT_MIN_COUNT_PACKETr:
        case PG_SHARED_COUNT_CELLr:
        case PG_SHARED_COUNT_PACKETr:
        case PG_HDRM_COUNT_CELLr:
        case PG_HDRM_COUNT_PACKETr:
        case PG_GBL_HDRM_COUNTr:
        case PG_RESET_VALUE_CELLr:
        case PG_RESET_VALUE_PACKETr:
            if (ainfo->idx < 2) {
                pbmp = ch_all_ports_with_mmu;
            } else {
                pbmp = ch_8pg_ports;
            }
            break;
 
        default:
            break; 
    }

    if (!SOC_PBMP_MEMBER(*pbmp, ainfo->port)) {
        goto skip; 
    }

    if (mask != NULL) {
        switch(ainfo->reg) {
            case HOL_STAT_PORTr:
            case ECN_CONFIGr:
                if (!SOC_PBMP_MEMBER(*ch_24q_ports, ainfo->port)) {
                    /* adjust mask for ports without 24 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x000000ff);
                    COMPILER_64_AND(*mask, temp_mask);
                }
                break;
            case TOQ_QUEUESTAT_64r:
            case TOQ_ACTIVATEQ_64r:
            case TOQEMPTY_64r:
            case DEQ_AGINGMASK_64r:
            case SHAPING_MODEr:
            case EAV_MAXBUCKET_64r:
            case EAV_MINBUCKET_64r:
                if (SOC_PBMP_MEMBER(*ch_24q_ports, ainfo->port)) {
                    /* adjust mask for ports with 24 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x00ffffff);
                    COMPILER_64_AND(*mask, temp_mask);
                } else if (ainfo->port != 0) {
                    /* remaining ports only have 8 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x000000ff);
                    COMPILER_64_AND(*mask, temp_mask);
                }
                break;
        }
    }
    return 0; 
skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }
    return 1;
}
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_ENDURO_SUPPORT
STATIC int
reg_mask_subset_en(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    pbmp_t *en_8pg_ports;
    pbmp_t *en_higig_ports;
    pbmp_t *en_24q_ports;
    pbmp_t *en_24q_ports_with_cpu;
    pbmp_t *en_non_cpu_ports;
    pbmp_t *en_all_ports;
    pbmp_t *en_cmic;
#define _REG_MASK_SUBSET_MAX_EN 7
    pbmp_t **mask_map_arr[_REG_MASK_SUBSET_MAX_EN];
    int i, count = _REG_MASK_SUBSET_MAX_EN;
    uint64 temp_mask;
    pbmp_t *pbmp;

    mask_map_arr[0] = &en_8pg_ports;
    mask_map_arr[1] = &en_higig_ports;
    mask_map_arr[2] = &en_24q_ports;
    mask_map_arr[3] = &en_24q_ports_with_cpu;
    mask_map_arr[4] = &en_non_cpu_ports;
    mask_map_arr[5] = &en_all_ports;
    mask_map_arr[6] = &en_cmic;
    if (NULL == SOC_REG_MASK_SUBSET(unit)) {
        _INIT_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);

        /* Higig ports [0,26,27,28,29] */
        SOC_PBMP_CLEAR(*en_higig_ports);
        SOC_PBMP_PORT_ADD(*en_higig_ports, 0);
        SOC_PBMP_PORT_ADD(*en_higig_ports, 26);
        SOC_PBMP_PORT_ADD(*en_higig_ports, 27);
        SOC_PBMP_PORT_ADD(*en_higig_ports, 28);
        SOC_PBMP_PORT_ADD(*en_higig_ports, 29);

        /*  24Q_PORTS = [26,27,28,29] */
        SOC_PBMP_CLEAR(*en_24q_ports);
        SOC_PBMP_PORT_ADD(*en_24q_ports, 26);
        SOC_PBMP_PORT_ADD(*en_24q_ports, 27);
        SOC_PBMP_PORT_ADD(*en_24q_ports, 28);
        SOC_PBMP_PORT_ADD(*en_24q_ports, 29);
        SOC_PBMP_ASSIGN(*en_24q_ports_with_cpu, *en_24q_ports);
        SOC_PBMP_PORT_ADD(*en_24q_ports_with_cpu, 0);

        SOC_PBMP_CLEAR(*en_8pg_ports);
        SOC_PBMP_PORT_ADD(*en_8pg_ports, 26);
        SOC_PBMP_PORT_ADD(*en_8pg_ports, 27);
        SOC_PBMP_PORT_ADD(*en_8pg_ports, 28);
        SOC_PBMP_PORT_ADD(*en_8pg_ports, 29);

        /* all port except CMIC */
        SOC_PBMP_CLEAR(*en_non_cpu_ports);
        SOC_PBMP_ASSIGN(*en_non_cpu_ports, PBMP_ALL(unit));
        SOC_PBMP_PORT_REMOVE(*en_non_cpu_ports, 0);
 
        /* All ports */
        SOC_PBMP_CLEAR(*en_all_ports); 
        SOC_PBMP_ASSIGN(*en_all_ports, PBMP_ALL(unit)); 
 
        /* CPU port */
        SOC_PBMP_CLEAR(*en_cmic); 
        SOC_PBMP_ASSIGN(*en_cmic, PBMP_CMIC(unit));
    }
    if (!SOC_PORT_VALID(unit, ainfo->port)) {
        return 0;
    }
    _MAP_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);
 
    switch(ainfo->reg) {
        case MAC_CTRLr:
        case MAC_XGXS_CTRLr:
        case MAC_XGXS_STATr:
        case MAC_TXMUXCTRLr:
        case MAC_CNTMAXSZr:
        case MAC_CORESPARE0r:
        case MAC_TXCTRLr:
        case MAC_TXMACSAr:
        case MAC_TXMAXSZr:
        case MAC_TXPSETHRr:
        case MAC_TXSPARE0r:
        case MAC_TXPPPCTRLr:
        case ITPOKr:
        case ITXPFr: 
        case ITFCSr:
        case ITUCr:
        case ITUCAr:
        case ITMCAr:
        case ITBCAr:
        case ITOVRr:
        case ITFRGr:
        case ITPKTr:
        case IT64r:
        case IT127r:
        case IT255r:
        case IT511r:
        case IT1023r:
        case IT1518r:
        case IT2047r:
        case IT4095r:
        case IT9216r:
        case IT16383r:
        case ITMAXr:
        case ITUFLr:
        case ITERRr:
        case ITBYTr:
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
        case IRUCAr:
        case IRMCAr:
        case IRBCAr:
        case IRXCFr:
        case IRXPFr:
        case IRXUOr:
        case IRJBRr:
        case IROVRr:
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
        case MAC_TXLLFCCTRLr:
        case MAC_TXLLFCMSGFLDSr:
        case MAC_RXLLFCMSGFLDSr:
        case MAC_RXLLFCMSGCNTr:
            if (IS_GE_PORT(unit, ainfo->port)) {
                goto skip;
            }
            break;
        default:
            break;
    }

    switch(ainfo->reg) {
        case IE2E_CONTROLr:
        case ING_MODMAP_CTRLr:
        case IHG_LOOKUPr:
        case ICONTROL_OPCODE_BITMAPr:
        case UNKNOWN_HGI_BITMAPr:
        case IUNHGIr:
        case ICTRLr:
        case IBCASTr:
        case ILTOMCr:
        case IIPMCr:
        case IUNKOPCr:
            pbmp = en_higig_ports;
            break;
        case PG_THRESH_SELr:
        case PORT_PRI_GRP0r:
        case PORT_PRI_GRP1r:
        case PORT_PRI_XON_ENABLEr:
            pbmp = en_8pg_ports;
            break;
        case ECN_CONFIGr:
        case HOL_STAT_PORTr:
        case PORT_WREDPARAM_CELLr:
        case PORT_WREDPARAM_YELLOW_CELLr:
        case PORT_WREDPARAM_RED_CELLr:
        case PORT_WREDPARAM_NONTCP_CELLr:
        case PORT_WREDCONFIG_CELLr:
        case PORT_WREDAVGQSIZE_CELLr:
        case PORT_WREDPARAM_PACKETr:
        case PORT_WREDPARAM_YELLOW_PACKETr:
        case PORT_WREDPARAM_RED_PACKETr:
        case PORT_WREDPARAM_NONTCP_PACKETr:
        case PORT_WREDCONFIG_PACKETr:
        case PORT_WREDAVGQSIZE_PACKETr:
        case COSMASKr:
        case MINSPCONFIGr:
        case EGRMETERINGCONFIG_64r:
        case EGRMETERINGBUCKETr:
            pbmp = en_non_cpu_ports;
            break;
        case ING_COS_MODEr:
        case COS_MODEr:
        case S1V_CONFIGr:
        case S1V_COSWEIGHTSr:
        case S1V_COSMASKr:
        case S1V_MINSPCONFIGr:
        case S1V_WDRRCOUNTr:
            pbmp = en_24q_ports;
            break;
        default:
            pbmp = en_all_ports;
            break;
    }

    switch(ainfo->reg) {
        /* MMU_PERPORTPERCOS_REGS */
        case OP_QUEUE_CONFIG_CELLr:
        case OP_QUEUE_CONFIG1_CELLr:
        case OP_QUEUE_CONFIG_PACKETr:
        case OP_QUEUE_CONFIG1_PACKETr:
        case OP_QUEUE_LIMIT_YELLOW_CELLr:
        case OP_QUEUE_LIMIT_YELLOW_PACKETr:
        case OP_QUEUE_LIMIT_RED_CELLr:
        case OP_QUEUE_LIMIT_RED_PACKETr:
        case OP_QUEUE_RESET_OFFSET_CELLr:
        case OP_QUEUE_RESET_OFFSET_PACKETr:
        case OP_QUEUE_MIN_COUNT_CELLr:
        case OP_QUEUE_MIN_COUNT_PACKETr:
        case OP_QUEUE_SHARED_COUNT_CELLr:
        case OP_QUEUE_SHARED_COUNT_PACKETr:
        case OP_QUEUE_TOTAL_COUNT_CELLr:
        case OP_QUEUE_TOTAL_COUNT_PACKETr:
        case OP_QUEUE_RESET_VALUE_CELLr:
        case OP_QUEUE_RESET_VALUE_PACKETr:
        case OP_QUEUE_LIMIT_RESUME_COLOR_CELLr:
        case OP_QUEUE_LIMIT_RESUME_COLOR_PACKETr:
        /* MMU_PERPORTPERCOS_REGS_CTR */
        case DROP_PKT_CNTr:
        case DROP_BYTE_CNTr:
            if (ainfo->port == 0) {
                return 0;
            } else if (ainfo->idx < 8) {
                pbmp = en_all_ports;
            } else if (ainfo->idx < 24) {
                pbmp = en_24q_ports;
            } else {
                pbmp = en_cmic;
            }
            break;
        /* MMU_PERPORTPERCOS_NOCPU_REGS */
        case WREDPARAM_CELLr:
        case WREDPARAM_YELLOW_CELLr:
        case WREDPARAM_RED_CELLr:
        case WREDPARAM_NONTCP_CELLr:
        case WREDCONFIG_CELLr:
        case WREDAVGQSIZE_CELLr:
        case WREDPARAM_PACKETr:
        case WREDPARAM_YELLOW_PACKETr:
        case WREDPARAM_RED_PACKETr:
        case WREDPARAM_NONTCP_PACKETr:
        case WREDCONFIG_PACKETr:
        case WREDAVGQSIZE_PACKETr:
            if (ainfo->port == 0) {
                goto skip;
            } else if (ainfo->idx < 8) {
                pbmp = en_non_cpu_ports;
            } else {
                pbmp = en_24q_ports;
            }
            break;


        /* MMU_MTRO_REGS */
        case MINBUCKETCONFIG_64r:
        case MINBUCKETr:
        case MAXBUCKETCONFIG_64r:
        case MAXBUCKETr:
            if (ainfo->port == 0) {
                goto skip;
            } else if (ainfo->idx < 8) {
                pbmp = en_all_ports;
            } else if (ainfo->idx < 26) {
                pbmp = en_24q_ports_with_cpu;
            } else {
                pbmp = en_cmic;
            }
            break;

        /* MMU_ES_REGS */
        case COSWEIGHTSr:
        case WDRRCOUNTr:
            if (ainfo->idx < 8) {
                pbmp = en_all_ports;
            } else if (ainfo->idx < 10) {
                pbmp = en_24q_ports_with_cpu;
            } else {
                pbmp = en_cmic;
            }
            break;
        /* MMU_PERPORTPERPRI_REGS */
        case PG_RESET_OFFSET_CELLr:
        case PG_RESET_OFFSET_PACKETr:
        case PG_RESET_FLOOR_CELLr:
        
        case PG_MIN_CELLr:
        case PG_MIN_PACKETr:
        case PG_HDRM_LIMIT_CELLr:
        case PG_HDRM_LIMIT_PACKETr:
        case PG_COUNT_CELLr:
        case PG_COUNT_PACKETr:
        case PG_MIN_COUNT_CELLr:
        case PG_MIN_COUNT_PACKETr:
        case PG_PORT_MIN_COUNT_CELLr:
        case PG_PORT_MIN_COUNT_PACKETr:
        case PG_SHARED_COUNT_CELLr:
        case PG_SHARED_COUNT_PACKETr:
        case PG_HDRM_COUNT_CELLr:
        case PG_HDRM_COUNT_PACKETr:
        case PG_GBL_HDRM_COUNTr:
        case PG_RESET_VALUE_CELLr:
        case PG_RESET_VALUE_PACKETr:
            if (ainfo->idx != 0) {
                pbmp = en_8pg_ports;
            }
            break;
        default:
            break; 
    }

    if (!SOC_PBMP_MEMBER(*pbmp, ainfo->port)) {
        goto skip; 
    }

    if (mask != NULL) {
        switch(ainfo->reg) {
            case HOL_STAT_PORTr:
            case ECN_CONFIGr:
                if (!SOC_PBMP_MEMBER(*en_24q_ports, ainfo->port)) {
                    /* adjust mask for ports without 24 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x000000ff);
                    COMPILER_64_AND(*mask, temp_mask);
                }
                break;
            case TOQ_QUEUESTAT_64r:
            case TOQ_ACTIVATEQ_64r:
            case TOQEMPTY_64r:
            case DEQ_AGINGMASK_64r:
                if (SOC_PBMP_MEMBER(*en_24q_ports, ainfo->port)) {
                    /* adjust mask for ports with 24 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x00ffffff);
                    COMPILER_64_AND(*mask, temp_mask);
                } else if (ainfo->port != 0) {
                    /* remaining ports only have 8 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x000000ff);
                    COMPILER_64_AND(*mask, temp_mask);
                }
                break;
        }
    }
    return 0; 
skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }
    return 1;
}
#endif /* BCM_ENDURO_SUPPORT */

#ifdef BCM_HURRICANE_SUPPORT
STATIC int
reg_mask_subset_hu(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    pbmp_t *hu_8pg_ports;
    pbmp_t *hu_24q_ports;
    pbmp_t *hu_24q_ports_with_cpu;
    pbmp_t *hu_higig_ports;
    pbmp_t *hu_non_cpu_ports;
    pbmp_t *hu_all_ports;
#define _REG_MASK_SUBSET_MAX_HU 6
    pbmp_t **mask_map_arr[_REG_MASK_SUBSET_MAX_HU];
    int i, count = _REG_MASK_SUBSET_MAX_HU;
    uint64 temp_mask;
    pbmp_t *pbmp;

    mask_map_arr[0] = &hu_8pg_ports;
    mask_map_arr[1] = &hu_24q_ports;
    mask_map_arr[2] = &hu_24q_ports_with_cpu;
    mask_map_arr[3] = &hu_higig_ports;
    mask_map_arr[4] = &hu_non_cpu_ports;
    mask_map_arr[5] = &hu_all_ports;
    if (NULL == SOC_REG_MASK_SUBSET(unit)) {
        _INIT_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);

        /* Higig ports [0,26,27,28,29] */
        SOC_PBMP_CLEAR(*hu_higig_ports);
        SOC_PBMP_PORT_ADD(*hu_higig_ports, 0);
        SOC_PBMP_PORT_ADD(*hu_higig_ports, 26);
        SOC_PBMP_PORT_ADD(*hu_higig_ports, 27);
        SOC_PBMP_PORT_ADD(*hu_higig_ports, 28);
        SOC_PBMP_PORT_ADD(*hu_higig_ports, 29);

        /*  24Q_PORTS = NONE */
        SOC_PBMP_CLEAR(*hu_24q_ports);
        SOC_PBMP_ASSIGN(*hu_24q_ports_with_cpu, *hu_24q_ports);
        SOC_PBMP_PORT_ADD(*hu_24q_ports_with_cpu, 0);

        SOC_PBMP_CLEAR(*hu_8pg_ports);
        SOC_PBMP_PORT_ADD(*hu_8pg_ports, 26);
        SOC_PBMP_PORT_ADD(*hu_8pg_ports, 27);
        SOC_PBMP_PORT_ADD(*hu_8pg_ports, 28);
        SOC_PBMP_PORT_ADD(*hu_8pg_ports, 29);

        /* all port except CMIC */
        SOC_PBMP_CLEAR(*hu_non_cpu_ports);
        SOC_PBMP_ASSIGN(*hu_non_cpu_ports, PBMP_ALL(unit));
        SOC_PBMP_PORT_REMOVE(*hu_non_cpu_ports, 0);
 
        /* All ports */
        SOC_PBMP_CLEAR(*hu_all_ports); 
        SOC_PBMP_ASSIGN(*hu_all_ports, PBMP_ALL(unit)); 
    }
    if (!SOC_PORT_VALID(unit, ainfo->port)) {
        return 0;
    }
    if (!SOC_REG_IS_VALID(unit, ainfo->reg)) {
        goto skip;
    }
    _MAP_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);
 
    switch(ainfo->reg) {
        case MAC_CTRLr:
        case MAC_XGXS_CTRLr:
        case MAC_XGXS_STATr:
        case MAC_TXMUXCTRLr:
        case MAC_CNTMAXSZr:
        case MAC_CORESPARE0r:
        case MAC_TXCTRLr:
        case MAC_TXMACSAr:
        case MAC_TXMAXSZr:
        case MAC_TXPSETHRr:
        case MAC_TXSPARE0r:
        case MAC_TXPPPCTRLr:
        case ITPOKr:
        case ITXPFr: 
        case ITFCSr:
        case ITUCr:
        case ITUCAr:
        case ITMCAr:
        case ITBCAr:
        case ITOVRr:
        case ITFRGr:
        case ITPKTr:
        case IT64r:
        case IT127r:
        case IT255r:
        case IT511r:
        case IT1023r:
        case IT1518r:
        case IT2047r:
        case IT4095r:
        case IT9216r:
        case IT16383r:
        case ITMAXr:
        case ITUFLr:
        case ITERRr:
        case ITBYTr:
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
        case IRUCAr:
        case IRMCAr:
        case IRBCAr:
        case IRXCFr:
        case IRXPFr:
        case IRXUOr:
        case IRJBRr:
        case IROVRr:
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
        case ITXPPr:
        case IRXPPr:
        case MAC_TXLLFCCTRLr:
        case MAC_TXLLFCMSGFLDSr:
        case MAC_RXLLFCMSGFLDSr:
        case MAC_RXLLFCMSGCNTr:
            if (IS_GE_PORT(unit, ainfo->port)) {
                goto skip;
            }
            break;
        default:
            break;
    }

    switch(ainfo->reg) {
        case IE2E_CONTROLr:
        case ING_MODMAP_CTRLr:
        case IHG_LOOKUPr:
        case ICONTROL_OPCODE_BITMAPr:
        case UNKNOWN_HGI_BITMAPr:
        case IUNHGIr:
        case ICTRLr:
        case IBCASTr:
        case ILTOMCr:
        case IIPMCr:
        case IUNKOPCr:
            pbmp = hu_higig_ports;
            break;
        case HOL_STAT_PORTr:
            pbmp = hu_non_cpu_ports;
            break;
        case ING_COS_MODEr:
            pbmp = hu_24q_ports;
            break;
        default:
            pbmp = hu_all_ports;
            break;
    }

    if (!SOC_PBMP_MEMBER(*pbmp, ainfo->port)) {
        goto skip; 
    }


    if (mask != NULL) {
        switch(ainfo->reg) {
            case HOL_STAT_PORTr:
                if (!SOC_PBMP_MEMBER(*hu_24q_ports, ainfo->port)) {
                    /* adjust mask for ports without 24 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x000000ff);
                    COMPILER_64_AND(*mask, temp_mask);
                }
                break;
            case TOQ_QUEUESTAT_64r:
            case TOQ_ACTIVATEQ_64r:
            case TOQEMPTY_64r:
            case DEQ_AGINGMASK_64r:
                if (SOC_PBMP_MEMBER(*hu_24q_ports, ainfo->port)) {
                    /* adjust mask for ports with 24 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x00ffffff);
                    COMPILER_64_AND(*mask, temp_mask);
                } else if (ainfo->port != 0) {
                    /* remaining ports only have 8 queues */
                    COMPILER_64_SET(temp_mask, 0, 0x000000ff);
                    COMPILER_64_AND(*mask, temp_mask);
                }
                break;
            case MAC_MODEr:
                /* skip LINK_STATUS bit */
                COMPILER_64_SET(temp_mask, 0, 0x0000000f);
                COMPILER_64_AND(*mask, temp_mask);
                break;
        }
    }
    return 0; 
skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }
    return 1;
}
#endif /* BCM_HURRICANE_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
STATIC int
reg_mask_subset_trident(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    soc_info_t *si;
    uint64 temp_mask;

    if (!SOC_PORT_VALID(unit, ainfo->port)) {
        return 0;
    }

    si = &SOC_INFO(unit);

    switch (ainfo->reg) {
    case XMAC_CTRLr:
    case XMAC_MODEr:
    case XMAC_SPARE0r:
    case XMAC_SPARE1r:
    case XMAC_TX_CTRLr:
    case XMAC_TX_MAC_SAr:
    case XMAC_RX_CTRLr:
    case XMAC_RX_MAC_SAr:
    case XMAC_RX_MAX_SIZEr:
    case XMAC_RX_VLAN_TAGr:
    case XMAC_RX_LSS_CTRLr:
    case XMAC_RX_LSS_STATUSr:
    case XMAC_CLEAR_RX_LSS_STATUSr:
    case XMAC_PAUSE_CTRLr:
    case XMAC_PFC_CTRLr:
    case XMAC_PFC_TYPEr:
    case XMAC_PFC_OPCODEr:
    case XMAC_PFC_DAr:
    case XMAC_LLFC_CTRLr:
    case XMAC_TX_LLFC_MSG_FIELDSr:
    case XMAC_RX_LLFC_MSG_FIELDSr:
    case XMAC_HCFC_CTRLr:
    case XMAC_TX_TIMESTAMP_FIFO_DATAr:
    case XMAC_TX_TIMESTAMP_FIFO_STATUSr:
    case XMAC_FIFO_STATUSr:
    case XMAC_CLEAR_FIFO_STATUSr:
    case XMAC_TX_FIFO_CREDITSr:
    case XMAC_EEE_CTRLr:
    case XMAC_EEE_TIMERSr:
        /* Skip Xmac registers when the XLPORT is used as GE port */
        if (IS_GE_PORT(unit, ainfo->port)) {
            goto skip;
        }
        break;


    case DROP_PKT_CNT_OVQr:
        /* @OVQ_PORT_DROP_REGS (all ports except cpu port) */
    case DEQ_AGINGMASKr:
    case MCQ_FIFO_BASE_REGr:
    case MCQ_FIFO_EMPTY_REGr:
        /* PORTLIST => [1..65] */
    case COSMASKr:
    case MINSPCONFIGr:
    case S3_CONFIGr:
    case S3_CONFIG_MCr:
    case S3_COSMASKr:
    case S3_COSMASK_MCr:
    case S3_MINSPCONFIGr:
    case S3_MINSPCONFIG_MCr:
    case S2_CONFIGr:
    case S2_COSWEIGHTSr:
    case S2_COSMASKr:
    case S2_MINSPCONFIGr:
    case S2_WERRCOUNTr:
    case S2_S3_ROUTINGr:
        /* @MMU_NO_CPU_PORTS (all ports except cpu port) */
    case PORT_LLFC_CFGr:
        /* @PFC_PORT_CFG_REGS (all ports except cpu port) */
    case S3_MINBUCKETCONFIG_64r:
    case S3_MINBUCKETr:
    case S3_MAXBUCKETCONFIG_64r:
    case S3_MAXBUCKETr:
        /* @MMU_MTRO_S3_REG (all ports except cpu port) */
    case S2_MINBUCKETCONFIG_64r:
    case S2_MINBUCKETr:
    case S2_MAXBUCKETCONFIG_64r:
    case S2_MAXBUCKETr:
        /* @MMU_MTRO_S2_REG (all ports except cpu port) */
    case OVQ_MCQ_CREDITSr:
    case OVQ_MCQ_STATEr:
        /* @OVQ_PORT_COS_REGS (all ports except cpu port) */
        if (IS_CPU_PORT(unit, ainfo->port)) {
            goto skip;
        }
        break;

    case XPORT_TO_MMU_BKPr:
        /* @PFC_PORT_STS_REGS (all ports except cpu and lb port) */
    case BKPMETERINGCONFIG_64r:
    case BKPMETERINGBUCKETr:
    case MTRI_IFGr:
        /* @MMU_MTRI_PORTS (all ports except cpu and lb port) */
    case OP_UC_PORT_CONFIG_CELLr:
    case OP_UC_PORT_CONFIG1_CELLr:
    case OP_UC_PORT_LIMIT_COLOR_CELLr:
    case OP_UC_PORT_SHARED_COUNT_CELLr:
    case OP_UC_PORT_LIMIT_RESUME_COLOR_CELLr:
    case UCQ_COS_EMPTY_REGr:
    case PORT_SP_WRED_CONFIGr:
    case PORT_SP_WRED_AVG_QSIZEr:
        /* PORTLIST => [1..32,34..65] */
    case OP_UC_QUEUE_MIN_COUNT_CELLr:
    case OP_UC_QUEUE_SHARED_COUNT_CELLr:
    case OP_UC_QUEUE_TOTAL_COUNT_CELLr:
    case OP_UC_QUEUE_RESET_VALUE_CELLr:
        /* @THDO_UC_PORT_COS_REGS (all ports except cpu and lb port) */
    case WRED_CONFIGr:
    case WRED_AVG_QSIZEr:
        /* @MMU_PERPORTPERCOS_NOCPU_REGS (all ports except cpu and lb port) */
        if (IS_CPU_PORT(unit, ainfo->port) || IS_LB_PORT(unit, ainfo->port)) {
            goto skip;
        }
        break;

    case OP_EX_QUEUE_MIN_COUNT_CELLr:
    case OP_EX_QUEUE_SHARED_COUNT_CELLr:
    case OP_EX_QUEUE_TOTAL_COUNT_CELLr:
    case OP_EX_QUEUE_RESET_VALUE_CELLr:
        /* @THDO_EX_PORT_COS_REGS (extended queue ports) */
    case OP_EX_PORT_CONFIG_SPID_0r:
    case OP_EX_PORT_CONFIG_SPID_1r:
    case OP_EX_PORT_CONFIG_SPID_2r:
    case OP_EX_PORT_CONFIG_SPID_3r:
    case OP_EX_PORT_CONFIG_SPID_4r:
    case OP_EX_PORT_CONFIG_COS_MIN_0r:
    case OP_EX_PORT_CONFIG_COS_MIN_1r:
    case OP_EX_PORT_CONFIG_COS_MIN_2r:
    case UCQ_EXTCOS1_EMPTY_REGr:
        /* PORTLIST => [1..4,34..37] */
    case DMVOQ_WRED_CONFIGr:
    case VOQ_WRED_AVG_QSIZEr:
        /* @MMU_PERPORTPERVOQ_NOCPU_REGS (extended queue ports) */
        if (!si->port_num_ext_cosq[ainfo->port]) {
            goto skip;
        }
        break;

    case COSWEIGHTSr:
    case WERRCOUNTr:
        /* @MMU_ES_REGS */
        if (!IS_CPU_PORT(unit, ainfo->port)) {
            if (ainfo->idx >= 7) {
                goto skip;
            }
        }
        break;

    case S3_COSWEIGHTSr:
    case S3_WERRCOUNTr:
        /* @MMU_ES_S3_REGS */
        if (!si->port_num_ext_cosq[ainfo->port]) {
            if (IS_CPU_PORT(unit, ainfo->port) || ainfo->idx >= 8) {
                goto skip;
            }
        }
        break;

    case MINBUCKETCONFIG_64r:
    case MINBUCKETr:
    case MAXBUCKETCONFIG_64r:
    case MAXBUCKETr:
        /* @MMU_MTRO_REGS */
        if (IS_CPU_PORT(unit, ainfo->port)) {
            if (ainfo->idx >= 48) {
                goto skip;
            }
        } else if (IS_LB_PORT(unit, ainfo->port)) {
            if (ainfo->idx >= 5) {
                goto skip;
            }
        } else if (!si->port_num_ext_cosq[ainfo->port]) {
            if (ainfo->idx >= 15) {
                goto skip;
            }
        }
        break;

    case OP_QUEUE_CONFIG_CELLr:
    case OP_QUEUE_CONFIG1_CELLr:
    case OP_QUEUE_LIMIT_COLOR_CELLr:
    case OP_QUEUE_RESET_OFFSET_CELLr:
    case OP_QUEUE_MIN_COUNT_CELLr:
    case OP_QUEUE_SHARED_COUNT_CELLr:
    case OP_QUEUE_TOTAL_COUNT_CELLr:
    case OP_QUEUE_RESET_VALUE_CELLr:
    case OP_QUEUE_LIMIT_RESUME_COLOR_CELLr:
        /* @THDO_PORT_COS_REGS */
        if (!IS_CPU_PORT(unit, ainfo->port)) {
            if (ainfo->idx >= 5) {
                goto skip;
            }
        }
        break;
    default:
        break;
    }

    if (mask != NULL) {
        switch (ainfo->reg) {
        case SHAPING_CONTROLr:
        case RESET_ON_EMPTY_MAX_64r:
            if (!si->port_num_ext_cosq[ainfo->port]) {
                COMPILER_64_SET(temp_mask, 0, 0x007fffff);
                COMPILER_64_AND(*mask, temp_mask);
            }
            break;
        default:
            break;
        }
    }

    return 0;
skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }
    return 1;
}
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
STATIC int
reg_mask_subset_trident2(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    soc_info_t *si;
    int port, phy_port = 0, mmu_port = 0;
    int pgw, xlp, blk, index;
    int instance_mask = 0;

    port = ainfo->port;
    if (REG_PORT_ANY != port) {
        instance_mask = port & SOC_REG_ADDR_INSTANCE_MASK;
        port &= (~SOC_REG_ADDR_INSTANCE_MASK);
    }

    if (!instance_mask) {
        if (!SOC_PORT_VALID(unit, port)) {
            return 0;
        }
        si = &SOC_INFO(unit);
        phy_port = si->port_l2p_mapping[ainfo->port];
        mmu_port = si->port_p2m_mapping[phy_port];
    }


    switch (ainfo->reg) {
    case PORT_INITIAL_COPY_COUNT_WIDTHr:
    case MMU_PORT_TO_LOGIC_PORT_MAPPINGr:
    case MMU_PORT_TO_PHY_PORT_MAPPINGr:
    case MMU_TO_XPORT_BKPr:
    case CHFC2PFC_STATEr:
    case QCN_PFC_STATEr:
        /* @MMU_PORT_LIST */
    case XPORT_TO_MMU_BKPr:
        /* @PFC_PORT_STS_REGS */
    case MTRI_IFGr:
        /* @MMU_MTRI_PORTS */
    case THDI_INPUT_PORT_XON_ENABLESr:
    case THDI_PORT_PRI_GRP0r:
    case THDI_PORT_PRI_GRP1r:
    case THDI_PORT_PG_SPIDr:
    case THDI_PORT_LIMIT_STATESr:
    case THDI_FLOW_CONTROL_XOFF_STATEr:
        /* MMU_THDI_PORTS */
    case THDU_PORT_E2ECC_COS_SPIDr:
        /* THDU_UC_PORT_LIST */
    case ASF_PORT_CFGr:
    case ENQ_ASF_ERRORr:
        /* @ENQ_PORT_LIST */
        break;
    case HSP_SCHED_PORT_CONFIGr:
    case HSP_SCHED_L0_NODE_CONFIGr:
    case HSP_SCHED_L0_NODE_WEIGHTr:
    case HSP_SCHED_L0_NODE_CONNECTION_CONFIGr:
    case HSP_SCHED_L1_NODE_CONFIGr:
    case HSP_SCHED_L1_NODE_WEIGHTr:
    case HSP_SCHED_L2_UC_QUEUE_CONFIGr:
    case HSP_SCHED_L2_MC_QUEUE_CONFIGr:
    case HSP_SCHED_L2_UC_QUEUE_WEIGHTr:
    case HSP_SCHED_L2_MC_QUEUE_WEIGHTr:
        /* @HSP_PORT_LIST => [0..15,64..79] */
        if (!(mmu_port >= 0 && mmu_port <= 15) &&
            !(mmu_port >= 64 && mmu_port <= 79)) {
            goto skip;
        }
        break;
    case MMU_THDM_DB_PORTSP_SHARED_COUNTr:
    case MMU_THDM_DB_PORTSP_TOTAL_COUNTr:
    case MMU_THDM_DB_PORTSP_THRESHOLD_PROFILE_SELr:
    case MMU_THDM_MCQE_PORTSP_SHARED_COUNTr:
    case MMU_THDM_MCQE_PORTSP_TOTAL_COUNTr:
    case MMU_THDM_MCQE_PORTSP_THRESHOLD_PROFILE_SELr:
        /* @THDM_PORT_LIST */
        break;
    case PGW_OBM0_CONTROLr:
    case PGW_OBM0_SHARED_CONFIGr:
    case PGW_OBM0_NIV_ETHERTYPEr:
    case PGW_OBM0_PE_ETHERTYPEr:
    case PGW_OBM0_OUTER_TPIDr:
    case PGW_OBM0_INNER_TPIDr:
    case PGW_OBM0_PRIORITY_MAPr:
    case PGW_OBM0_THRESHOLDr:
    case PGW_OBM0_ECC_ENABLEr:
    case PGW_OBM0_ECC_STATUSr:
    case PGW_OBM0_USE_COUNTERr:
    case PGW_OBM0_MAX_USAGEr:
    case PGW_OBM0_LOW_PRI_PKT_DROPr:
    case PGW_OBM0_LOW_PRI_BYTE_DROPr:
    case PGW_OBM0_HIGH_PRI_PKT_DROPr:
    case PGW_OBM0_HIGH_PRI_BYTE_DROPr:
    case PGW_OBM1_CONTROLr:
    case PGW_OBM1_SHARED_CONFIGr:
    case PGW_OBM1_NIV_ETHERTYPEr:
    case PGW_OBM1_PE_ETHERTYPEr:
    case PGW_OBM1_OUTER_TPIDr:
    case PGW_OBM1_INNER_TPIDr:
    case PGW_OBM1_PRIORITY_MAPr:
    case PGW_OBM1_THRESHOLDr:
    case PGW_OBM1_ECC_ENABLEr:
    case PGW_OBM1_ECC_STATUSr:
    case PGW_OBM1_USE_COUNTERr:
    case PGW_OBM1_MAX_USAGEr:
    case PGW_OBM1_LOW_PRI_PKT_DROPr:
    case PGW_OBM1_LOW_PRI_BYTE_DROPr:
    case PGW_OBM1_HIGH_PRI_PKT_DROPr:
    case PGW_OBM1_HIGH_PRI_BYTE_DROPr:
    case PGW_OBM2_CONTROLr:
    case PGW_OBM2_SHARED_CONFIGr:
    case PGW_OBM2_NIV_ETHERTYPEr:
    case PGW_OBM2_PE_ETHERTYPEr:
    case PGW_OBM2_OUTER_TPIDr:
    case PGW_OBM2_INNER_TPIDr:
    case PGW_OBM2_PRIORITY_MAPr:
    case PGW_OBM2_THRESHOLDr:
    case PGW_OBM2_ECC_ENABLEr:
    case PGW_OBM2_ECC_STATUSr:
    case PGW_OBM2_USE_COUNTERr:
    case PGW_OBM2_MAX_USAGEr:
    case PGW_OBM2_LOW_PRI_PKT_DROPr:
    case PGW_OBM2_LOW_PRI_BYTE_DROPr:
    case PGW_OBM2_HIGH_PRI_PKT_DROPr:
    case PGW_OBM2_HIGH_PRI_BYTE_DROPr:
    case PGW_OBM3_CONTROLr:
    case PGW_OBM3_SHARED_CONFIGr:
    case PGW_OBM3_NIV_ETHERTYPEr:
    case PGW_OBM3_PE_ETHERTYPEr:
    case PGW_OBM3_OUTER_TPIDr:
    case PGW_OBM3_INNER_TPIDr:
    case PGW_OBM3_PRIORITY_MAPr:
    case PGW_OBM3_THRESHOLDr:
    case PGW_OBM3_ECC_ENABLEr:
    case PGW_OBM3_ECC_STATUSr:
    case PGW_OBM3_USE_COUNTERr:
    case PGW_OBM3_MAX_USAGEr:
    case PGW_OBM3_LOW_PRI_PKT_DROPr:
    case PGW_OBM3_LOW_PRI_BYTE_DROPr:
    case PGW_OBM3_HIGH_PRI_PKT_DROPr:
    case PGW_OBM3_HIGH_PRI_BYTE_DROPr:
        switch (ainfo->reg) {
        case PGW_OBM0_CONTROLr:
        case PGW_OBM0_SHARED_CONFIGr:
        case PGW_OBM0_NIV_ETHERTYPEr:
        case PGW_OBM0_PE_ETHERTYPEr:
        case PGW_OBM0_OUTER_TPIDr:
        case PGW_OBM0_INNER_TPIDr:
        case PGW_OBM0_PRIORITY_MAPr:
        case PGW_OBM0_THRESHOLDr:
        case PGW_OBM0_ECC_ENABLEr:
        case PGW_OBM0_ECC_STATUSr:
        case PGW_OBM0_USE_COUNTERr:
        case PGW_OBM0_MAX_USAGEr:
        case PGW_OBM0_LOW_PRI_PKT_DROPr:
        case PGW_OBM0_LOW_PRI_BYTE_DROPr:
        case PGW_OBM0_HIGH_PRI_PKT_DROPr:
        case PGW_OBM0_HIGH_PRI_BYTE_DROPr:
            xlp = 0;
            break;
        case PGW_OBM1_CONTROLr:
        case PGW_OBM1_SHARED_CONFIGr:
        case PGW_OBM1_NIV_ETHERTYPEr:
        case PGW_OBM1_PE_ETHERTYPEr:
        case PGW_OBM1_OUTER_TPIDr:
        case PGW_OBM1_INNER_TPIDr:
        case PGW_OBM1_PRIORITY_MAPr:
        case PGW_OBM1_THRESHOLDr:
        case PGW_OBM1_ECC_ENABLEr:
        case PGW_OBM1_ECC_STATUSr:
        case PGW_OBM1_USE_COUNTERr:
        case PGW_OBM1_MAX_USAGEr:
        case PGW_OBM1_LOW_PRI_PKT_DROPr:
        case PGW_OBM1_LOW_PRI_BYTE_DROPr:
        case PGW_OBM1_HIGH_PRI_PKT_DROPr:
        case PGW_OBM1_HIGH_PRI_BYTE_DROPr:
            xlp = 1;
            break;
        case PGW_OBM2_CONTROLr:
        case PGW_OBM2_SHARED_CONFIGr:
        case PGW_OBM2_NIV_ETHERTYPEr:
        case PGW_OBM2_PE_ETHERTYPEr:
        case PGW_OBM2_OUTER_TPIDr:
        case PGW_OBM2_INNER_TPIDr:
        case PGW_OBM2_PRIORITY_MAPr:
        case PGW_OBM2_THRESHOLDr:
        case PGW_OBM2_ECC_ENABLEr:
        case PGW_OBM2_ECC_STATUSr:
        case PGW_OBM2_USE_COUNTERr:
        case PGW_OBM2_MAX_USAGEr:
        case PGW_OBM2_LOW_PRI_PKT_DROPr:
        case PGW_OBM2_LOW_PRI_BYTE_DROPr:
        case PGW_OBM2_HIGH_PRI_PKT_DROPr:
        case PGW_OBM2_HIGH_PRI_BYTE_DROPr:
            xlp = 2;
            break;
        case PGW_OBM3_CONTROLr:
        case PGW_OBM3_SHARED_CONFIGr:
        case PGW_OBM3_NIV_ETHERTYPEr:
        case PGW_OBM3_PE_ETHERTYPEr:
        case PGW_OBM3_OUTER_TPIDr:
        case PGW_OBM3_INNER_TPIDr:
        case PGW_OBM3_PRIORITY_MAPr:
        case PGW_OBM3_THRESHOLDr:
        case PGW_OBM3_ECC_ENABLEr:
        case PGW_OBM3_ECC_STATUSr:
        case PGW_OBM3_USE_COUNTERr:
        case PGW_OBM3_MAX_USAGEr:
        case PGW_OBM3_LOW_PRI_PKT_DROPr:
        case PGW_OBM3_LOW_PRI_BYTE_DROPr:
        case PGW_OBM3_HIGH_PRI_PKT_DROPr:
        case PGW_OBM3_HIGH_PRI_BYTE_DROPr:
            xlp = 3;
            break;
        default:
            return SOC_E_INTERNAL;
        }
        pgw = port;
        index = pgw * 4 + xlp;
        for (blk = 0; SOC_BLOCK_TYPE(unit, blk) >= 0; blk++) {
            if (SOC_BLOCK_TYPE(unit, blk) != SOC_BLK_XLPORT) {
                continue;
            }
            if (SOC_BLOCK_NUMBER(unit, blk) == index) {
                if (!SOC_INFO(unit).block_valid[blk]) {
                    goto skip;
                }
            }
        }
        break;
    default:
        break;
    }

    return 0;
skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }
    return 1;
}
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_CALADAN3_SUPPORT

extern int c3_port_init[];

STATIC int
reg_mask_subset_c3(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    static pbmp_t c3_all_ports;
    static pbmp_t c3_cl_ports;
    static pbmp_t c3_e_ports;
    int port;
    pbmp_t *pbmp;
    soc_info_t *si = &SOC_INFO(unit);
    uint16 dev_id;
    uint8 rev_id;

    if (!c3_port_init[unit]) {

        /* CMAC ports */
        SOC_PBMP_CLEAR(c3_cl_ports); 
        SOC_PBMP_OR(c3_cl_ports, si->ce.bitmap);

        /* XMAC ports */
        SOC_PBMP_CLEAR(c3_e_ports); 
        SOC_PBMP_OR(c3_e_ports, si->xe.bitmap);
        SOC_PBMP_OR(c3_e_ports, si->ge.bitmap);

        SOC_PBMP_ITER(si->hg.bitmap, port) {
            if (si->port_speed_max[port] >= 100000) {
                SOC_PBMP_PORT_ADD(c3_cl_ports, port);
            } else {
                SOC_PBMP_PORT_ADD(c3_e_ports, port);
            }
        }

        /* All ports */
        SOC_PBMP_CLEAR(c3_all_ports); 
        SOC_PBMP_ASSIGN(c3_all_ports, PBMP_ALL(unit)); 

        c3_port_init[unit] = 1; 
    }

    /* Non-port register */
    if (mask != NULL) {
        switch(ainfo->reg) {
        case CX_BS_PLL_RESETr:
        case CX_BS_PLL_STATUSr:
        case CX_DDR03_PLL_RESETr:
        case CX_DDR0_PLL_STATUSr:
        case CX_DDR1_PLL_STATUSr:
        case CX_DDR2_PLL_STATUSr:
        case CX_DDR3_PLL_STATUSr:
        case CX_HPP_PLL_RESETr:
        case CX_HPP_PLL_STATUSr:
        case CX_MAC_PLL_RESETr:
        case CX_MAC_PLL_STATUSr:
        case CX_SE0_PLL_RESETr:
        case CX_SE0_PLL_STATUSr:
        case CX_SE1_PLL_RESETr:
        case CX_SE1_PLL_STATUSr:
        case CX_SWS_PLL_RESETr:
        case CX_SWS_PLL_STATUSr:
        case CX_TMU_PLL_RESETr:
        case CX_TMU_PLL_STATUSr:
        case CX_TS_PLL_RESETr:
        case CX_TS_PLL_STATUSr:
        case CX_WC_PLL_RESETr:
        case CX_WC_PLL_STATUSr:
        case CX_PLL_LOCK_LOST_STATUSr:
            
            if (mask != NULL) {
                COMPILER_64_SET(*mask, 0, 0);
            }
            return 0;
        default:
            break;
        }
    }

    /* filter block access */
    if ((ainfo->reg) && 
            (SOC_BLOCK_IN_LIST(SOC_REG_INFO(unit, ainfo->reg).block, SOC_BLK_SBX_PORT)) &&
                (SOC_BLOCK_PORT(unit, ainfo->block) < 0)) {
        goto skip;
    }

    if (!SOC_PORT_VALID(unit, ainfo->port)) {
        if (soc_property_get(unit, spn_LRP_BYPASS, 0)) {
            switch (ainfo->reg) {
            case IL_ECC_DEBUG0r:
            case IL_ECC_ERROR_L2_INTR_MASKr:
            case IL_FLOWCONTROL_L2_INTR_MASKr:
            case IL_GLOBAL_CONFIGr:
            case IL_HCFC_INTR_MASKr:
            case IL_HCFC_RX_REMAP0r:
            case IL_HCFC_RX_REMAP1r:
            case IL_HCFC_RX_REMAP10r:
            case IL_HCFC_RX_REMAP11r:
            case IL_HCFC_RX_REMAP12r:
            case IL_HCFC_RX_REMAP2r:
            case IL_HCFC_RX_REMAP3r:
            case IL_HCFC_RX_REMAP4r:
            case IL_HCFC_RX_REMAP5r:
            case IL_HCFC_RX_REMAP6r:
            case IL_HCFC_RX_REMAP7r:
            case IL_HCFC_RX_REMAP8r:
            case IL_HCFC_RX_REMAP9r:
            case IL_HCFC_TX_CONFIG1r:
            case IL_HCFC_TX_REMAP0r:
            case IL_HCFC_TX_REMAP1r:
            case IL_HCFC_TX_REMAP10r:
            case IL_HCFC_TX_REMAP11r:
            case IL_HCFC_TX_REMAP12r:
            case IL_HCFC_TX_REMAP2r:
            case IL_HCFC_TX_REMAP3r:
            case IL_HCFC_TX_REMAP4r:
            case IL_HCFC_TX_REMAP5r:
            case IL_HCFC_TX_REMAP6r:
            case IL_HCFC_TX_REMAP7r:
            case IL_HCFC_TX_REMAP8r:
            case IL_HCFC_TX_REMAP9r:
            case IL_IEEE_CRC32_CONFIGr:
            case IL_MU_LLFC_CONTROLr:
            case IL_RX_CHAN_ENABLE0r:
            case IL_RX_CHAN_ENABLE1r:
            case IL_RX_CONFIGr:
            case IL_RX_CORE_CONFIG0r:
            case IL_RX_CORE_CONFIG1r:
            case IL_RX_ERRDET0_L2_INTR_MASKr:
            case IL_RX_ERRDET1_L2_INTR_MASKr:
            case IL_RX_ERRDET2_L2_INTR_MASKr:
            case IL_RX_ERRDET3_L2_INTR_MASKr:
            case IL_RX_ERRDET4_L2_INTR_MASKr:
            case IL_RX_ERRDET5_L2_INTR_MASKr:
            case IL_RX_LANE_SWAP_CONTROL_1r:
            case IL_RX_LANE_SWAP_CONTROL_10r:
            case IL_RX_LANE_SWAP_CONTROL_11r:
            case IL_RX_LANE_SWAP_CONTROL_2r:
            case IL_RX_LANE_SWAP_CONTROL_3r:
            case IL_RX_LANE_SWAP_CONTROL_4r:
            case IL_RX_LANE_SWAP_CONTROL_5r:
            case IL_RX_LANE_SWAP_CONTROL_6r:
            case IL_RX_LANE_SWAP_CONTROL_7r:
            case IL_RX_LANE_SWAP_CONTROL_8r:
            case IL_RX_LANE_SWAP_CONTROL_9r:
            case IL_STATS_RXSAT0_INTR_MASKr:
            case IL_STATS_RXSAT1_INTR_MASKr:
            case IL_STATS_TXSAT0_INTR_MASKr:
            case IL_STATS_TXSAT1_INTR_MASKr:
            case IL_TX_CHAN_ENABLE0r:
            case IL_TX_CHAN_ENABLE1r:
            case IL_TX_CONFIGr:
            case IL_TX_CORE_CONFIG0r:
            case IL_TX_CORE_CONFIG2r:
            case IL_TX_ERRDET0_L2_INTR_MASKr:
            case IL_TX_LANE_SWAP_CONTROL_1r:
            case IL_TX_LANE_SWAP_CONTROL_10r:
            case IL_TX_LANE_SWAP_CONTROL_11r:
            case IL_TX_LANE_SWAP_CONTROL_2r:
            case IL_TX_LANE_SWAP_CONTROL_3r:
            case IL_TX_LANE_SWAP_CONTROL_4r:
            case IL_TX_LANE_SWAP_CONTROL_5r:
            case IL_TX_LANE_SWAP_CONTROL_6r:
            case IL_TX_LANE_SWAP_CONTROL_7r:
            case IL_TX_LANE_SWAP_CONTROL_8r:
            case IL_TX_LANE_SWAP_CONTROL_9r:
                if (mask != NULL) {
                    COMPILER_64_SET(*mask, 0, 0);
                }
                break;
            }
        }
        return 0;
    }

    switch (ainfo->reg) {
    case CMAC_CTRLr:
    case CMAC_MODEr:
    case CMAC_SPARE0r:
    case CMAC_SPARE1r:
    case CMAC_TX_CTRLr:
    case CMAC_TX_MAC_SAr:
    case CMAC_RX_CTRLr:
    case CMAC_RX_MAC_SAr:
    case CMAC_RX_MAX_SIZEr:
    case CMAC_RX_VLAN_TAGr:
    case CMAC_RX_LSS_CTRLr:
    case CMAC_RX_LSS_STATUSr:
    case CMAC_CLEAR_RX_LSS_STATUSr:
    case CMAC_PAUSE_CTRLr:
    case CMAC_PFC_CTRLr:
    case CMAC_PFC_TYPEr:
    case CMAC_PFC_OPCODEr:
    case CMAC_PFC_DAr:
    case CMAC_LLFC_CTRLr:
    case CMAC_TX_LLFC_MSG_FIELDSr:
    case CMAC_RX_LLFC_MSG_FIELDSr:
    case CMAC_HCFC_CTRLr:
    case CMAC_TX_TIMESTAMP_FIFO_DATAr:
    case CMAC_TX_TIMESTAMP_FIFO_STATUSr:
    case CMAC_FIFO_STATUSr:
    case CMAC_CLEAR_FIFO_STATUSr:
    case CMAC_TX_FIFO_CREDITSr:
    case CMAC_EEE_CTRLr:
    case CMAC_EEE_TIMERSr:
    case CMAC_EEE_1_SEC_LINK_STATUS_TIMERr:
    case CMAC_TIMESTAMP_ADJUSTr:
    case CMAC_MACSEC_CTRLr:
    case CMAC_ECC_CTRLr:
    case CMAC_ECC_STATUSr:
    case CMAC_CLEAR_ECC_STATUSr:
    case CMAC_VERSION_IDr:
        /* CMAC registers (Only CL configured ports)*/
        pbmp = &c3_cl_ports;
        break;
    case XMAC_CTRLr:
    case XMAC_MODEr:
    case XMAC_SPARE0r:
    case XMAC_SPARE1r:
    case XMAC_TX_CTRLr:
    case XMAC_TX_MAC_SAr:
    case XMAC_RX_CTRLr:
    case XMAC_RX_MAC_SAr:
    case XMAC_RX_MAX_SIZEr:
    case XMAC_RX_VLAN_TAGr:
    case XMAC_RX_LSS_CTRLr:
    case XMAC_RX_LSS_STATUSr:
    case XMAC_CLEAR_RX_LSS_STATUSr:
    case XMAC_PAUSE_CTRLr:
    case XMAC_PFC_CTRLr:
    case XMAC_PFC_TYPEr:
    case XMAC_PFC_OPCODEr:
    case XMAC_PFC_DAr:
    case XMAC_LLFC_CTRLr:
    case XMAC_TX_LLFC_MSG_FIELDSr:
    case XMAC_RX_LLFC_MSG_FIELDSr:
    case XMAC_HCFC_CTRLr:
    case XMAC_TX_TIMESTAMP_FIFO_DATAr:
    case XMAC_TX_TIMESTAMP_FIFO_STATUSr:
    case XMAC_FIFO_STATUSr:
    case XMAC_CLEAR_FIFO_STATUSr:
    case XMAC_TX_FIFO_CREDITSr:
    case XMAC_EEE_CTRLr:
    case XMAC_EEE_TIMERSr:
        pbmp = &c3_e_ports;
        break;
    default:
        pbmp = &c3_all_ports;
        break;
    }

    if ((soc_portreg == SOC_REG_INFO(unit, ainfo->reg).regtype) &&
        (!SOC_PBMP_MEMBER(*pbmp, ainfo->port))) {
        goto skip; 
    }
    if (((ainfo->reg == PORT_XGXS1_CTRL_REGr) ||
        (ainfo->reg == PORT_XGXS2_CTRL_REGr)) &&  
        (SOC_PBMP_MEMBER(si->xl.bitmap, ainfo->port))) {
        /* Not implemented in the xlport */
        goto skip;
    }
        
    if (ainfo->block == SOC_BLK_ETU) {
        soc_cm_get_id(unit, &dev_id, &rev_id);
        if (rev_id == BCM88034_DEVICE_ID) {
            goto skip;
        }
    }

    return 0;

skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }
    return 1;
}
#endif /* BCM_CALADAN3_SUPPORT */

#ifdef BCM_SIRIUS_SUPPORT
STATIC int
reg_mask_subset_ss(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    switch (ainfo->reg) {
        /* if there are any field has name debug_rptr or debug_wptr
         * skip the register
         */
        case TS_PRI_SB_DEBUGr:
        case SB_DEBUG_QS_CIr:
        case SB_DEBUG_TX_CIr:
        case SB_DEBUG_RB_CIr:
        case QMA_QS_SB_DEBUGr:
        case QMA_RB_SB_DEBUGr:
        case QMB_ENQD_SB_DEBUGr:
        case QMB_DEQD_SB_DEBUGr:
        case QS_RATE_SB_DEBUGr:
        case QSA_ENQDEQD_SB_DEBUGr:
        case QM_QSB_RAND_SB_DEBUGr:
        case QM_QSB_RATE_SB_DEBUGr:
        case TS_QSB_RATE_SB_DEBUGr:
        case RB_ENQRESP_SB_DEBUGr:
        case CI_RB_RBTAG_SB_DEBUGr:
        case QM_TX_DEQREQ_SB_DEBUGr:
        case QS_TX_GRANT_SB_DEBUGr:
        case CI0_TX_SB_DEBUGr:
        case CI1_TX_SB_DEBUGr:
        case CI2_TX_SB_DEBUGr:
        case CI3_TX_SB_DEBUGr:
        case CI4_TX_SB_DEBUGr:
        case CI5_TX_SB_DEBUGr:
        case CI6_TX_SB_DEBUGr:
        case CI7_TX_SB_DEBUGr:
        case CI8_TX_SB_DEBUGr:
        case CI9_TX_SB_DEBUGr:
    case SC_TOP_SI_SD_STATUSr:
    case SI_SD_STATUSr:
        /* hardware update in FIC mode, read only, ignore */
        case QMA_DEBUG1r:
        case RB_DEBUG_TEST_CONFIGr:
        case TS_DEBUG_INFOr:
        /* reset value based on hw configration, not known at reset time, skipfor tr 1 test */ 
        case MAC_CTRLr:
    case XPORT_XGXS_NEWCTL_REGr:
        /* required to be modified to allow MAC register read, skip for tr 1 test */
        case MAC_RXCTRLr:
        case MAC_TXCTRLr:
        /* rw value limited by the hardware, some fields not really writable, skip for tr 3 test */
            goto skip;
        default:
        break;
    }

    switch(ainfo->reg) {
    /* higig 4-7 doesn't really support these registers */
        case MAC_CTRLr:
        case MAC_XGXS_CTRLr:
        case MAC_XGXS_STATr:
        case MAC_TXMUXCTRLr:
        case MAC_CNTMAXSZr:
        case MAC_CORESPARE0r:
        case MAC_TXCTRLr:
        case MAC_TXMACSAr:
        case MAC_TXMAXSZr:
        case MAC_TXPSETHRr:
        case MAC_TXSPARE0r:
        case MAC_TXPPPCTRLr:
        case ITPOKr:
        case ITXPFr: 
        case ITFCSr:
        case ITUCr:
        case ITUCAr:
        case ITMCAr:
        case ITBCAr:
        case ITOVRr:
        case ITFRGr:
        case ITPKTr:
        case IT64r:
        case IT127r:
        case IT255r:
        case IT511r:
        case IT1023r:
        case IT1518r:
        case IT2047r:
        case IT4095r:
        case IT9216r:
        case IT16383r:
        case ITMAXr:
        case ITUFLr:
        case ITERRr:
        case ITBYTr:
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
        case IRUCAr:
        case IRMCAr:
        case IRBCAr:
        case IRXCFr:
        case IRXPFr:
        case IRXUOr:
        case IRJBRr:
        case IROVRr:
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
        case ITXPPr:
        case IRXPPr:
        case MAC_TXLLFCCTRLr:
        case MAC_TXLLFCMSGFLDSr:
        case MAC_RXLLFCMSGFLDSr:
        case MAC_RXLLFCMSGCNTr:
    case XHOL_RX_MODID_MODEr:
    case XHOL_RX_MODID_DATAr:
        /* skip if higig 4-7 */
        if ((ainfo->port >= 34) &&
        (ainfo->port >= 36)) {
                goto skip;
            }
        goto skip;
        default:
        return 0;
    }

skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }  
    /* registers are implemented in hw, so return 0 here */
    return 0;      
}
#endif /* BCM_SIRIUS_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
STATIC int
reg_mask_subset_tr3(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    pbmp_t *tr3_8mcq_ports_32_35;
    pbmp_t *tr3_8mcq_ports_36_39;
    pbmp_t *tr3_10mcq_ports_40_47;
    pbmp_t *tr3_10mcq_ports_48_55;
    pbmp_t *tr3_wlan_encap_ports;
    pbmp_t *tr3_pass_thru_ports;
    pbmp_t *tr3_cpu_ports;
    pbmp_t *tr3_high_speed_ports;
    pbmp_t *tr3_all_ports;
    pbmp_t *tr3_cl_ports;
    pbmp_t *tr3_repl_head_ports;
    pbmp_t *tr3_ports_none;
#define _REG_MASK_SUBSET_MAX_TR3 12
    pbmp_t **mask_map_arr[_REG_MASK_SUBSET_MAX_TR3];
    int i, count = _REG_MASK_SUBSET_MAX_TR3;
    uint64 temp_mask;
    pbmp_t *pbmp;
    soc_port_t port;
    uint16 dev_id;
    uint8 rev_id;

    mask_map_arr[0] = &tr3_8mcq_ports_32_35;
    mask_map_arr[1] = &tr3_8mcq_ports_36_39;
    mask_map_arr[2] = &tr3_10mcq_ports_40_47;
    mask_map_arr[3] = &tr3_10mcq_ports_48_55;
    mask_map_arr[4] = &tr3_wlan_encap_ports;
    mask_map_arr[5] = &tr3_pass_thru_ports;
    mask_map_arr[6] = &tr3_cpu_ports;
    mask_map_arr[7] = &tr3_high_speed_ports;
    mask_map_arr[8] = &tr3_all_ports;
    mask_map_arr[9] = &tr3_cl_ports;
    mask_map_arr[10] = &tr3_repl_head_ports;
    mask_map_arr[11] = &tr3_ports_none;
    if (NULL == SOC_REG_MASK_SUBSET(unit)) {
        soc_info_t *si = &SOC_INFO(unit);
        soc_port_t mmu_port, phy_port;
        
        _INIT_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);

        /* 8MCQ = MMU[32-35] */
        SOC_PBMP_CLEAR(*tr3_8mcq_ports_32_35);
        for (mmu_port = 32; mmu_port < 36; mmu_port++) {
            phy_port = si->port_m2p_mapping[mmu_port];
            port = (phy_port == -1) ? -1 : si->port_p2l_mapping[phy_port];
            if (-1 != port) {
                SOC_PBMP_PORT_ADD(*tr3_8mcq_ports_32_35, port);
            }
        }

        /* 8MCQ = MMU[36-39] */
        SOC_PBMP_CLEAR(*tr3_8mcq_ports_36_39);
        for (mmu_port = 36; mmu_port < 40; mmu_port++) {
            phy_port = si->port_m2p_mapping[mmu_port];
            port = (phy_port == -1) ? -1 : si->port_p2l_mapping[phy_port];
            if (-1 != port) {
                SOC_PBMP_PORT_ADD(*tr3_8mcq_ports_36_39, port);
            }
        }

        /* 10MCQ = MMU[40-55] */
        SOC_PBMP_CLEAR(*tr3_10mcq_ports_40_47);
        for (mmu_port = 40; mmu_port < 48; mmu_port++) {
            phy_port = si->port_m2p_mapping[mmu_port];
            port = (phy_port == -1) ? -1 : si->port_p2l_mapping[phy_port];
            if (-1 != port) {
                SOC_PBMP_PORT_ADD(*tr3_10mcq_ports_40_47, port);
            }
        }

        SOC_PBMP_CLEAR(*tr3_10mcq_ports_48_55);
        for (mmu_port = 48; mmu_port < 56; mmu_port++) {
            phy_port = si->port_m2p_mapping[mmu_port];
            port = (phy_port == -1) ? -1 : si->port_p2l_mapping[phy_port];
            if (-1 != port) {
                SOC_PBMP_PORT_ADD(*tr3_10mcq_ports_48_55, port);
            }
        }

        /* CPU port = MMU[59] */
        SOC_PBMP_CLEAR(*tr3_cpu_ports);
        SOC_PBMP_PORT_ADD(*tr3_cpu_ports, CMIC_PORT(unit));

        /* WLAN Encap = MMU[56] */
        SOC_PBMP_CLEAR(*tr3_wlan_encap_ports);
        SOC_PBMP_PORT_ADD(*tr3_wlan_encap_ports,
                          AXP_PORT(unit, SOC_AXP_NLF_WLAN_ENCAP));

        /* Pass thru = MMU[58] */
        SOC_PBMP_CLEAR(*tr3_pass_thru_ports);
        SOC_PBMP_PORT_ADD(*tr3_pass_thru_ports,
                          AXP_PORT(unit, SOC_AXP_NLF_PASSTHRU));

        /* High speed ports = MMU[48,52] */
        SOC_PBMP_CLEAR(*tr3_high_speed_ports);
        phy_port = si->port_m2p_mapping[48];
        port = (phy_port == -1) ? -1 : si->port_p2l_mapping[phy_port];
        if (-1 != port) {
            SOC_PBMP_PORT_ADD(*tr3_high_speed_ports, port);
        }
        phy_port = si->port_m2p_mapping[52];
        port = (phy_port == -1) ? -1 : si->port_p2l_mapping[phy_port];
        if (-1 != port) {
            SOC_PBMP_PORT_ADD(*tr3_high_speed_ports, port);
        }

        /* CE ports */
        SOC_PBMP_CLEAR(*tr3_cl_ports);
        PBMP_CE_ITER(unit, port) {
            SOC_PBMP_PORT_ADD(*tr3_cl_ports, port);
        }
        PBMP_HG_ITER(unit, port) {
            if (si->port_speed_max[port] >= 100000) {
                SOC_PBMP_PORT_ADD(*tr3_cl_ports, port);
            }
        }

        /* All ports */
        SOC_PBMP_CLEAR(*tr3_all_ports); 
        SOC_PBMP_ASSIGN(*tr3_all_ports, PBMP_ALL(unit)); 

        /* Ports for REPL_HEAD_PTR_REPLACE */
        SOC_PBMP_ASSIGN(*tr3_repl_head_ports, *tr3_all_ports);
        SOC_PBMP_PORT_REMOVE(*tr3_repl_head_ports, CMIC_PORT(unit));

        SOC_PBMP_CLEAR(*tr3_ports_none);
    }

    /* Non-port register */
    if (mask != NULL) {
        switch(ainfo->reg) {
        case ETU_CONFIG0r:
            if (!soc_feature(unit, soc_feature_etu_support)) {
                /* HBIT_MEM_PSM_VDD bits are stuck without ESM */
                COMPILER_64_SET(temp_mask, 0, 0x00027fff);
                COMPILER_64_AND(*mask, temp_mask);
            }
            break;
        case TOP_XG_PLL0_CTRL_0r:
        case TOP_XG_PLL1_CTRL_0r:
        case TOP_XG_PLL2_CTRL_0r:
        case TOP_XG_PLL3_CTRL_0r:
        case AUX_ARB_CONTROL_2r:
        case TOP_TAP_CONTROLr:
            
            goto skip;
        case ICFG_EN_COR_ERR_RPTr:
        case IPARS_EN_COR_ERR_RPTr:
        case IESMIF_EN_COR_ERR_RPTr:
        case EGR_EL3_EN_COR_ERR_RPTr:
        case EGR_EHCPM_EN_COR_ERR_RPTr:
        case EGR_1588_TIMER_VALUEr:
        case EGR_1588_EGRESS_CTRLr:
        case EGR_1588_TIMER_DEBUGr:
        case EGR_1588_PARSING_CONTROLr:
        case LLS_CONFIG0r:
        case LLS_SP_WERR_DYN_CHANGE_0r:
        case LLS_SP_WERR_DYN_CHANGE_1r:
        case LLS_SP_WERR_DYN_CHANGE_2r:
        case LLS_SP_WERR_DYN_CHANGE_3r:
        case LLS_SP_WERR_DYN_CHANGE_0Ar:
        case LLS_SP_WERR_DYN_CHANGE_1Ar:
        case LLS_SP_WERR_DYN_CHANGE_2Ar:
        case LLS_SP_WERR_DYN_CHANGE_3Ar:
        case LLS_SP_WERR_DYN_CHANGE_0Br:
        case LLS_SP_WERR_DYN_CHANGE_1Br:
        case LLS_SP_WERR_DYN_CHANGE_2Br:
        case LLS_SP_WERR_DYN_CHANGE_3Br:
        case LLS_TDM_CAL_CFGr:
        case LLS_FC_CONFIGr:   
        case ISM_INTR_MASKr:
        case PORT_MODE_REGr:
        case PORT_TS_TIMER_31_0_REGr:
        case PORT_TS_TIMER_47_32_REGr:
        case XMAC_OSTS_TIMESTAMP_ADJUSTr:
            soc_cm_get_id(unit, &dev_id, &rev_id);
            if (rev_id < BCM56640_B0_REV_ID) {
                goto skip;
            }
        default:
            break;
        }
    }

    if (!SOC_PORT_VALID(unit, ainfo->port)) {
        return 0;
    }
    _MAP_REG_MASK_SUBSETS(unit, i, count, mask_map_arr);

    switch (ainfo->reg) {
    case MCQ_FIFO_BASE_REG_32_35r:
        /* PORTLIST => MMU[32..35] 8 MCQ (High-speed) */
        pbmp = tr3_8mcq_ports_32_35;
        break;
    case MCQ_FIFO_BASE_REG_36_39r:
        /* PORTLIST => MMU[36..39] 8 MCQ (High-speed) */
        pbmp = tr3_8mcq_ports_36_39;
        break;
    case MCQ_FIFO_BASE_REGr:
        /* PORTLIST => MMU[40..55] 10 MCQ (High-speed) */
        pbmp = tr3_10mcq_ports_40_47;
        break;
    case MCQ_FIFO_BASE_REG_48_55r:
        /* PORTLIST => MMU[40..55] 10 MCQ (High-speed) */
        pbmp = tr3_10mcq_ports_48_55;
        break;
    case MCQ_FIFO_BASE_REG_56r:
        /* PORTLIST => [56] */
        pbmp = tr3_wlan_encap_ports;
        break;
    case MCQ_FIFO_BASE_REG_PASSTHRUr:
        /* PORTLIST => [58] */
        pbmp = tr3_pass_thru_ports;
        break;
    case MCQ_FIFO_BASE_REG_CPUr:
        /* PORTLIST => [59] */
        pbmp = tr3_cpu_ports;
        break;

    case HES_PORT_CONFIGr:
    case HES_L0_CONFIGr:
    case HES_Q_MINSPr:
    case HES_Q_COSMASKr:
    case HES_Q_COSWEIGHTSr:
    case HES_Q_WERRCOUNTr:
        /* @MMU_HSP_PORT_NUMBERS (High-speed ports [48,52])*/
        pbmp = tr3_high_speed_ports;
        break;

    case CMAC_CTRLr:
    case CMAC_MODEr:
    case CMAC_SPARE0r:
    case CMAC_SPARE1r:
    case CMAC_TX_CTRLr:
    case CMAC_TX_MAC_SAr:
    case CMAC_RX_CTRLr:
    case CMAC_RX_MAC_SAr:
    case CMAC_RX_MAX_SIZEr:
    case CMAC_RX_VLAN_TAGr:
    case CMAC_RX_LSS_CTRLr:
    case CMAC_RX_LSS_STATUSr:
    case CMAC_CLEAR_RX_LSS_STATUSr:
    case CMAC_PAUSE_CTRLr:
    case CMAC_PFC_CTRLr:
    case CMAC_PFC_TYPEr:
    case CMAC_PFC_OPCODEr:
    case CMAC_PFC_DAr:
    case CMAC_LLFC_CTRLr:
    case CMAC_TX_LLFC_MSG_FIELDSr:
    case CMAC_RX_LLFC_MSG_FIELDSr:
    case CMAC_HCFC_CTRLr:
    case CMAC_TX_TIMESTAMP_FIFO_DATAr:
    case CMAC_TX_TIMESTAMP_FIFO_STATUSr:
    case CMAC_FIFO_STATUSr:
    case CMAC_CLEAR_FIFO_STATUSr:
    case CMAC_TX_FIFO_CREDITSr:
    case CMAC_EEE_CTRLr:
    case CMAC_EEE_TIMERSr:
    case CMAC_EEE_1_SEC_LINK_STATUS_TIMERr:
    case CMAC_TIMESTAMP_ADJUSTr:
    case CMAC_MACSEC_CTRLr:
    case CMAC_ECC_CTRLr:
    case CMAC_ECC_STATUSr:
    case CMAC_CLEAR_ECC_STATUSr:
    case CMAC_VERSION_IDr:
        /* CMAC registers (Only CL configured ports)*/
        pbmp = tr3_cl_ports;
        break;

    case REPL_HEAD_PTR_REPLACEr:
        if (!soc_feature(unit, soc_feature_repl_head_ptr_replace)) {
            pbmp = tr3_ports_none;
            break;
        }
        /* [0..56,58,60,61] */
        pbmp = tr3_repl_head_ports;
        break;
    
    case THDO_PORT_E2ECC_COS_SPIDr:
    case PORT_SW_FLOW_CONTROLr:
        soc_cm_get_id(unit, &dev_id, &rev_id);
        if ((rev_id < BCM56640_B0_REV_ID) && !SOC_IS_HELIX4(unit)) {
            pbmp = tr3_ports_none;
            break;
        }
        /* Fall through */

    default:
        pbmp = tr3_all_ports;
        break;
    }

    if ((soc_portreg == SOC_REG_INFO(unit, ainfo->reg).regtype) &&
        (!SOC_PBMP_MEMBER(*pbmp, ainfo->port))) {
        goto skip; 
    }

    switch (ainfo->reg) {

    case DEQ_AGINGMASKr:
        /* PORTLIST => MMU[0..58,60..62] Non-CPU */
    case XPORT_TO_MMU_BKPr:
        /* @PFC_PORT_STS_REGS Non-CPU */
    case BKPMETERINGCONFIG_64r:
    case BKPMETERINGBUCKETr:
    case MTRI_IFGr:
        /* @MMU_MTRI_PORTS = MMU[0..58,60..62] */
        if (IS_CPU_PORT(unit, ainfo->port)) {
            goto skip;
        }
        break;
    case OP_PORT_CONFIG_CELLr:
    case OP_PORT_CONFIG1_CELLr:
    case OP_PORT_LIMIT_COLOR_CELLr:
    case OP_PORT_SHARED_COUNT_CELLr:
    case OP_PORT_TOTAL_COUNT_CELLr:
    case OP_PORT_LIMIT_RESUME_COLOR_CELLr:
        /* @THDO_MC_PORT_LIST = MMU[0..56,58..62] */
        if (ainfo->port > AXP_PORT(unit, SOC_AXP_NLF_SM)) {
            goto skip;
        }
        break;

    case MCQ_FIFO_BASE_REG_CPUr:
        /* PORTLIST => [59] */
    case OP_CPU_QUEUE_BST_STATr:
    case OP_CPU_QUEUE_BST_THRESHOLDr:
        /* @THDO_CPU_BST_REGS */
        if (!IS_CPU_PORT(unit, ainfo->port)) {
            goto skip;
        }
        break;

    case L0_COSWEIGHTSr:
    case L0_WERRCOUNTr:
        /* @MMU_ES_REGS[0-9]=[48,52] */
        if (!SOC_PBMP_MEMBER(*tr3_high_speed_ports, ainfo->port)) {
            goto skip;
        }
        break;

    case OVQ_MCQ_CREDITSr:
    case OVQ_MCQ_STATEr:
        /* @OVQ_PORT_COS_REGS (all ports except cpu and lb port) */
        if (ainfo->port == AXP_PORT(unit, SOC_AXP_NLF_WLAN_DECAP)) {
            goto skip;
        }
        /* Fall thru */
    case OP_QUEUE_CONFIG_CELLr:
    case OP_QUEUE_CONFIG1_CELLr:
    case OP_QUEUE_LIMIT_COLOR_CELLr:
    case OP_QUEUE_RESET_OFFSET_CELLr:
    case OP_QUEUE_MIN_COUNT_CELLr:
    case OP_QUEUE_SHARED_COUNT_CELLr:
    case OP_QUEUE_TOTAL_COUNT_CELLr:
    case OP_QUEUE_RESET_VALUE_CELLr:
    case OP_QUEUE_LIMIT_RESUME_COLOR_CELLr:
        /* @THDO_PORT_COS_REGS */
        if ((ainfo->port == AXP_PORT(unit, SOC_AXP_NLF_SM)) || 
            (ainfo->port == AXP_PORT(unit, SOC_AXP_NLF_WLAN_DECAP))) {
             if (ainfo->idx >= 1) {
                goto skip;
            }
        } else if (SOC_PBMP_MEMBER(*tr3_10mcq_ports_40_47, ainfo->port) ||
                   SOC_PBMP_MEMBER(*tr3_10mcq_ports_48_55, ainfo->port)) {
             if (ainfo->idx >= 10) {
                goto skip;
            }
        } else if (!IS_CPU_PORT(unit, ainfo->port)) {
             if (ainfo->idx >= 8) {
                goto skip;
            }
        }
        /* Else CPU port, all queues allowed */
        break;
    case TOP_HW_TAP_MEM_ECC_CTRLr:
        if (SOC_IS_HELIX4(unit)) {
            goto skip;
        }
    default:
        break;
    }

    return 0;
skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }
    return 1;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#if 0 /* #ifdef BCM_KATANA_SUPPORT */
/* Remove this completely once the new function is fully verified */
STATIC int
reg_mask_subset_katana(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    soc_info_t *si;
    int i;
    static int kt_port_init = 0;
    static pbmp_t kt_ipipe_higig_ports;
    static pbmp_t kt_epipe_ports_subports;
    static pbmp_t kt_ports_0_35;
    static pbmp_t kt_ports_25_28;
    static pbmp_t kt_mmu_mtri_ports;
    static pbmp_t kt_all_ports;
    uint64 temp_mask;
    pbmp_t *pbmp;

    if (!kt_port_init) {

        /* IPIPE_HIGIG_PORTS = [0,25,26,27,28,29,30,31] */
        SOC_PBMP_CLEAR(kt_ipipe_higig_ports);
        SOC_PBMP_PORT_ADD(kt_ipipe_higig_ports, 0);
        for (i = 25; i <= 31; i++) {
            SOC_PBMP_PORT_ADD(kt_ipipe_higig_ports, i);
        }

        /* EPIPE_PORTS_SUBPORTS = [0..38] */
        SOC_PBMP_CLEAR(kt_epipe_ports_subports);
        for (i = 0; i <= 38; i++) {
            SOC_PBMP_PORT_ADD(kt_epipe_ports_subports, i);
        }

        /* [0..35] */
        /*  @MMU_THDO_PORTS = [0..35] */
        /*  @MMU_RQE_PORTS = [0..35]; */
        SOC_PBMP_CLEAR(kt_ports_0_35);
        for (i = 0; i <= 35; i++) {
            SOC_PBMP_PORT_ADD(kt_ports_0_35, i);
        }

        /* PFC_PORT_STS_REGS = [25..28] */
        SOC_PBMP_CLEAR(kt_ports_25_28);
        for (i = 25; i <= 28; i++) {
            SOC_PBMP_PORT_ADD(kt_ports_25_28, i);
        }
        /* @MMU_MTRI_PORTS = [1..35] */
        SOC_PBMP_CLEAR(kt_mmu_mtri_ports);
        for (i = 1; i <= 35; i++) {
            SOC_PBMP_PORT_ADD(kt_mmu_mtri_ports, i);
        }
                
        /* All ports */
        SOC_PBMP_CLEAR(kt_all_ports); 
        SOC_PBMP_ASSIGN(kt_all_ports, PBMP_ALL(unit)); 
 
        kt_port_init = 1; 
    }


    if (!SOC_PORT_VALID(unit, ainfo->port)) {
        return 0;
    }

    si = &SOC_INFO(unit);

    switch (ainfo->reg) {
        /* IPIPE_HIGIG_PORTS = [0,25,26,27,28,29,30,31] */
        case IUNHGIr:
        case ICTRLr:
        case IBCASTr:
        case ILTOMCr:
        case IIPMCr:
        case IUNKOPCr:
            pbmp = &kt_ipipe_higig_ports;
            break;

        /* EPIPE_PORTS_SUBPORTS = [0..38] */
        case EGR_VLAN_CONTROL_1r:
        case EGR_IPMC_CFG2r:
        case EGR_PORT_TO_NHI_MAPPINGr:
        case EGR_VLAN_CONTROL_2r:
        case EGR_VLAN_CONTROL_3r:
        case EGR_PVLAN_EPORT_CONTROLr:
        case EGR_INGRESS_PORT_TPID_SELECTr:
        case EGR_VLAN_LOGICAL_TO_PHYSICAL_MAPPINGr:
        case EGR_DBGr:
        case EGR_MODMAP_CTRLr:
        case EGR_SF_SRC_MODID_CHECKr:
        case EGR_SHAPING_CONTROLr:
        case EGR_COUNTER_CONTROLr:
        case EGR_MTUr:
        case EGR_PORT_1r:
        case EGR_1588_INGRESS_CTRLr:
        case EGR_1588_EGRESS_CTRLr:
        case EGR_1588_LINK_DELAYr:
        case EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr:
        case TDBGC0r:
        case TDBGC1r:
        case TDBGC2r:
        case TDBGC3r:
        case TDBGC4r:
        case TDBGC5r:
        case TDBGC6r:
        case TDBGC7r:
        case TDBGC8r:
        case TDBGC9r:
        case TDBGC10r:
        case TDBGC11r:
            pbmp = &kt_epipe_ports_subports;
            break;

        /* [0..35] */
        case DEQ_EFIFO_CFGr:
        case DEQ_EFIFO_STATUS_DEBUGr:
        case DEQ_EFIFO_EMPTY_FULL_STATUS_DEBUGr:
        case DEQ_EFIFO_WATERMARK_DEBUGr:
        case MMU_TO_XPORT_BKPr:
        case PORT_MAX_SHARED_CELLr:
        case THDIEMA_PORT_MAX_SHARED_CELLr:
        case THDIEXT_PORT_MAX_SHARED_CELLr:
        case THDIQEN_PORT_MAX_SHARED_CELLr:
        case THDIRQE_PORT_MAX_SHARED_CELLr:
        case PORT_MAX_PKT_SIZEr:
        case THDIEMA_PORT_MAX_PKT_SIZEr:
        case THDIEXT_PORT_MAX_PKT_SIZEr:
        case THDIQEN_PORT_MAX_PKT_SIZEr:
        case THDIRQE_PORT_MAX_PKT_SIZEr:
        case PORT_RESUME_LIMIT_CELLr:
        case THDIEMA_PORT_RESUME_LIMIT_CELLr:
        case THDIEXT_PORT_RESUME_LIMIT_CELLr:
        case THDIQEN_PORT_RESUME_LIMIT_CELLr:
        case THDIRQE_PORT_RESUME_LIMIT_CELLr:
        case PORT_PG_SPIDr:
        case THDIEMA_PORT_PG_SPIDr:
        case THDIEXT_PORT_PG_SPIDr:
        case THDIQEN_PORT_PG_SPIDr:
        case THDIRQE_PORT_PG_SPIDr:
        case PG_PORT_MIN_COUNT_CELLr:
        case THDIEMA_PG_PORT_MIN_COUNT_CELLr:
        case THDIEXT_PG_PORT_MIN_COUNT_CELLr:
        case THDIQEN_PG_PORT_MIN_COUNT_CELLr:
        case THDIRQE_PG_PORT_MIN_COUNT_CELLr:
        case PORT_FC_STATUSr:
        case THDIEMA_PORT_FC_STATUSr:
        case THDIEXT_PORT_FC_STATUSr:
        case THDIQEN_PORT_FC_STATUSr:
        case THDIRQE_PORT_FC_STATUSr:
        case FLOW_CONTROL_XOFF_STATEr:
        case THDIEMA_FLOW_CONTROL_XOFF_STATEr:
        case THDIEXT_FLOW_CONTROL_XOFF_STATEr:
        case THDIQEN_FLOW_CONTROL_XOFF_STATEr:
        case THDIRQE_FLOW_CONTROL_XOFF_STATEr:
        case TOQ_EG_CREDITr:
        case TOQ_PORT_BW_CTRLr:
        case E2EFC_PORT_MAPPINGr:
            pbmp = &kt_ports_0_35;
            break;

        case PG_SHARED_LIMIT_CELLr:
        case THDIEMA_PG_SHARED_LIMIT_CELLr:
        case THDIEXT_PG_SHARED_LIMIT_CELLr:
        case THDIQEN_PG_SHARED_LIMIT_CELLr:
        case THDIRQE_PG_SHARED_LIMIT_CELLr:
        case PG_RESET_OFFSET_CELLr:
        case THDIEMA_PG_RESET_OFFSET_CELLr:
        case THDIEXT_PG_RESET_OFFSET_CELLr:
        case THDIQEN_PG_RESET_OFFSET_CELLr:
        case THDIRQE_PG_RESET_OFFSET_CELLr:
        case PG_RESET_FLOOR_CELLr:
        case THDIEMA_PG_RESET_FLOOR_CELLr:
        case THDIEXT_PG_RESET_FLOOR_CELLr:
        case THDIQEN_PG_RESET_FLOOR_CELLr:
        case THDIRQE_PG_RESET_FLOOR_CELLr:
        case PG_MIN_CELLr:
        case THDIEMA_PG_MIN_CELLr:
        case THDIEXT_PG_MIN_CELLr:
        case THDIQEN_PG_MIN_CELLr:
        case THDIRQE_PG_MIN_CELLr:
        case PG_HDRM_LIMIT_CELLr:
        case THDIEMA_PG_HDRM_LIMIT_CELLr:
        case THDIEXT_PG_HDRM_LIMIT_CELLr:
        case THDIQEN_PG_HDRM_LIMIT_CELLr:
        case THDIRQE_PG_HDRM_LIMIT_CELLr:
        case PG_MIN_COUNT_CELLr:
        case THDIEMA_PG_MIN_COUNT_CELLr:
        case THDIEXT_PG_MIN_COUNT_CELLr:
        case THDIQEN_PG_MIN_COUNT_CELLr:
        case THDIRQE_PG_MIN_COUNT_CELLr:
        case PG_SHARED_COUNT_CELLr:
        case THDIEMA_PG_SHARED_COUNT_CELLr:
        case THDIEXT_PG_SHARED_COUNT_CELLr:
        case THDIQEN_PG_SHARED_COUNT_CELLr:
        case THDIRQE_PG_SHARED_COUNT_CELLr:
        case PG_HDRM_COUNT_CELLr:
        case THDIEMA_PG_HDRM_COUNT_CELLr:
        case THDIEXT_PG_HDRM_COUNT_CELLr:
        case THDIQEN_PG_HDRM_COUNT_CELLr:
        case THDIRQE_PG_HDRM_COUNT_CELLr:
        case PG_GBL_HDRM_COUNTr:
        case THDIEMA_PG_GBL_HDRM_COUNTr:
        case THDIEXT_PG_GBL_HDRM_COUNTr:
        case THDIQEN_PG_GBL_HDRM_COUNTr:
        case THDIRQE_PG_GBL_HDRM_COUNTr:
        case PG_RESET_VALUE_CELLr:
        case THDIEMA_PG_RESET_VALUE_CELLr:
        case THDIEXT_PG_RESET_VALUE_CELLr:
        case THDIQEN_PG_RESET_VALUE_CELLr:
        case THDIRQE_PG_RESET_VALUE_CELLr:
            if (ainfo->idx == 0) {
                pbmp = &kt_ports_0_35;
            } else {
                pbmp = &kt_ports_25_28;
            }
            break;

        /* PFC_PORT_STS/CFG_REGS = [25..28] */
        case XPORT_TO_MMU_BKPr:
        case PORT_LLFC_CFGr:
        case INTFI_PORT_CFGr:
        case PORT_MIN_CELLr:
        case THDIEMA_PORT_MIN_CELLr:
        case THDIEXT_PORT_MIN_CELLr:
        case THDIQEN_PORT_MIN_CELLr:
        case THDIRQE_PORT_MIN_CELLr:
        case PORT_PRI_GRP0r:
        case THDIEMA_PORT_PRI_GRP0r:
        case THDIEXT_PORT_PRI_GRP0r:
        case THDIQEN_PORT_PRI_GRP0r:
        case THDIRQE_PORT_PRI_GRP0r:
        case PORT_PRI_GRP1r:
        case THDIEMA_PORT_PRI_GRP1r:
        case THDIEXT_PORT_PRI_GRP1r:
        case THDIQEN_PORT_PRI_GRP1r:
        case THDIRQE_PORT_PRI_GRP1r:
        case PORT_PRI_XON_ENABLEr:
        case THDIEMA_PORT_PRI_XON_ENABLEr:
        case THDIEXT_PORT_PRI_XON_ENABLEr:
        case THDIQEN_PORT_PRI_XON_ENABLEr:
        case THDIRQE_PORT_PRI_XON_ENABLEr:
        case PORT_COUNT_CELLr:
        case THDIEMA_PORT_COUNT_CELLr:
        case THDIEXT_PORT_COUNT_CELLr:
        case THDIQEN_PORT_COUNT_CELLr:
        case THDIRQE_PORT_COUNT_CELLr:
        case PORT_MIN_COUNT_CELLr:
        case THDIEMA_PORT_MIN_COUNT_CELLr:
        case THDIEXT_PORT_MIN_COUNT_CELLr:
        case THDIQEN_PORT_MIN_COUNT_CELLr:
        case THDIRQE_PORT_MIN_COUNT_CELLr:
        case PORT_SHARED_COUNT_CELLr:
        case THDIEMA_PORT_SHARED_COUNT_CELLr:
        case THDIEXT_PORT_SHARED_COUNT_CELLr:
        case THDIQEN_PORT_SHARED_COUNT_CELLr:
        case THDIRQE_PORT_SHARED_COUNT_CELLr:
        case PORT_SHARED_MAX_PG_ENABLEr:
        case THDIEMA_PORT_SHARED_MAX_PG_ENABLEr:
        case THDIEXT_PORT_SHARED_MAX_PG_ENABLEr:
        case THDIQEN_PORT_SHARED_MAX_PG_ENABLEr:
        case THDIRQE_PORT_SHARED_MAX_PG_ENABLEr:
        case PORT_MIN_PG_ENABLEr:
        case THDIEMA_PORT_MIN_PG_ENABLEr:
        case THDIEXT_PORT_MIN_PG_ENABLEr:
        case THDIQEN_PORT_MIN_PG_ENABLEr:
        case THDIRQE_PORT_MIN_PG_ENABLEr:
            pbmp = &kt_ports_25_28;
            break;

        /* @MMU_MTRI_PORTS = [1..35] */
        case BKPMETERINGCONFIG_64r:
        case BKPMETERINGBUCKETr:
        case MTRI_IFGr:
            pbmp = &kt_mmu_mtri_ports;
            break;

        /*  @MMU_THDO_PORTS = [0..35] */
        case OP_E2ECC_PORT_CONFIGr:
            pbmp = &kt_ports_0_35;
            break;

        /*  @MMU_RQE_PORTS = [0..35]; */
        case RQE_PORT_CONFIGr:
        case RQE_MIRROR_CONFIGr:
            pbmp = &kt_ports_0_35;
            break;

        case MAC_PFC_CTRLr:
        case XMAC_LH_HDR_1r:
        case XMAC_LH_HDR_2r:
        case XMAC_LH_HDR_3r:
        case XMAC_LH_HDR_0_SCH_IDLEr:
        case XMAC_SCH_CTRLr:
            /* These are not implemented in Katana */
            goto skip;

        default:
            pbmp = &kt_all_ports;
            break;
    }

    if ((soc_portreg == SOC_REG_INFO(unit, ainfo->reg).regtype) &&
        (!SOC_PBMP_MEMBER(*pbmp, ainfo->port))) {
        goto skip; 
    }

    if (mask != NULL) {
        switch (ainfo->reg) {
        case SHAPING_CONTROLr:
        case RESET_ON_EMPTY_MAX_64r:
            if (!si->port_num_ext_cosq[ainfo->port]) {
                COMPILER_64_SET(temp_mask, 0, 0x007fffff);
                COMPILER_64_AND(*mask, temp_mask);
            }
            break;
        default:
            break;
        }
    }

    return 0;

skip:
    /* set mask to all 0's so test will skip it */
    if (mask != NULL) {
        COMPILER_64_SET(*mask, 0, 0);
    }
    return 1;
}
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
STATIC int
reg_mask_subset_katanax(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    soc_info_t *si;
    uint64 temp_mask;
    int rv = 0;

    si = &SOC_INFO(unit);

    if (!SOC_REG_PORT_IDX_VALID(unit,ainfo->reg, ainfo->port, ainfo->idx)) {
        /* set mask to all 0's so test will skip it */
        if (mask != NULL) {
            COMPILER_64_SET(*mask, 0, 0);
        }
        return 1;
    }

    /* Validate block accesses against variant features */
    if (CHECK_FEATURE_BLK(soc_feature_ces, SOC_BLK_CES) ||
        CHECK_FEATURE_BLK(soc_feature_ddr3, SOC_BLK_CI)) {
        if (mask != NULL) {
            COMPILER_64_SET(*mask, 0, 0);
        }
        return 1;
    }

    if (mask != NULL) {
        switch (ainfo->reg) {
        case SHAPING_CONTROLr:
        case RESET_ON_EMPTY_MAX_64r:
            if (!si->port_num_ext_cosq[ainfo->port]) {
                COMPILER_64_SET(temp_mask, 0, 0x007fffff);
                COMPILER_64_AND(*mask, temp_mask);
            }
            break;
        case CFAPIFULLRESETPOINTr:
        case CFAPEFULLRESETPOINTr:
        case CFAPIFULLSETPOINTr:
        case CFAPEFULLSETPOINTr:
        case MAC_PFC_CTRLr:
        case CFAPICONFIGr:
        case CHFC_TC2PRI_TBL_ECC_CONFIGr:
        case MMU_INTFO_CONGST_STr:
        case RXLP_DFC_CPU_UPDATE_REFRESHr:
        case RXLP_DFC_STATUS_HIr:
        case RXLP_DFC_STATUS_LOr:
        case THDI_TO_OOBFC_SP_STr:
        case THDO_TO_OOBFC_SP_STr:
        case CFAPIOTPCONFIGr:
            COMPILER_64_SET(*mask, 0, 0);
            rv = 1;
            break;
        default:
            break;
        }
    }

    return rv;
}
#endif /* BCM_KATANAX_SUPPORT */

#ifdef BCM_HURRICANE2_SUPPORT
STATIC int
reg_mask_subset_hurricane2(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    if (!SOC_REG_PORT_IDX_VALID(unit,ainfo->reg, ainfo->port, ainfo->idx)) {
        /* set mask to all 0's so test will skip it */
        if (mask != NULL) {
            COMPILER_64_SET(*mask, 0, 0);
        }
        return 1;
    }

    return 0;
}
#endif /* BCM_HURRICANE2_SUPPORT */

#ifdef BCM_GREYHOUND_SUPPORT
STATIC int
reg_mask_subset_greyhound(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
    int rv = 0;

    if (!SOC_REG_PORT_IDX_VALID(unit,ainfo->reg, ainfo->port, ainfo->idx)) {
        /* set mask to all 0's so test will skip it */
        if (mask != NULL) {
            COMPILER_64_SET(*mask, 0, 0);
        }
        return 1;
    }

    if (mask != NULL) {
        switch (ainfo->reg) {
        case TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_0r:
        case TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r:
        case TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_0r:
        case TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r:
        case TOP_XG0_LCPLL_FBDIV_CTRL_0r:
        case TOP_XG0_LCPLL_FBDIV_CTRL_1r:
        case TOP_XG1_LCPLL_FBDIV_CTRL_0r:
        case TOP_XG1_LCPLL_FBDIV_CTRL_1r:
            COMPILER_64_SET(*mask, 0, 0);
            rv = 1;
            break;
        default:
            break;
        }
    }

    return rv;
}
#endif /* BCM_GREYHOUND_SUPPORT */

/*
 * Function: reg_mask_subset
 * 
 * Description: 
 *   Returns 1 if the register is not implemented in HW for the 
 *   specified port/cos. Returns 0 if the register is implemented
 *   in HW. If "mask" argument is supplied, the mask is modified
 *   to only include bits that are implemented in HW for the specified
 *   port/cos index.
 */
int
reg_mask_subset(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask)
{
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit)) {
        return (reg_mask_subset_tr2(unit, ainfo, mask));
    }
#endif
#ifdef BCM_HURRICANE_SUPPORT
    if (SOC_IS_HURRICANE(unit)) {
        return (reg_mask_subset_hu(unit, ainfo, mask));
    }
#endif
#ifdef BCM_ENDURO_SUPPORT
    if (SOC_IS_ENDURO(unit)) {
        return (reg_mask_subset_en(unit, ainfo, mask));
    }
#endif
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        return (reg_mask_subset_tr3(unit, ainfo, mask));
    }
#endif
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        return (reg_mask_subset_trident2(unit, ainfo, mask));
    }
#endif
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        return (reg_mask_subset_trident(unit, ainfo, mask));
    }
#endif
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        return (reg_mask_subset_katanax(unit, ainfo, mask));
    }
#endif
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        return (reg_mask_subset_katanax(unit, ainfo, mask));
    }
#endif
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        return (reg_mask_subset_hurricane2(unit, ainfo, mask));
    }
#endif
#ifdef BCM_GREYHOUND_SUPPORT
    if (SOC_IS_GREYHOUND(unit)) {
        return (reg_mask_subset_greyhound(unit, ainfo, mask));
    }
#endif
#ifdef BCM_TRIUMPH_SUPPORT
    /* This case is a sort of 'catch-All" for triumph Family. So
     * Make sure this comes after others like Hu, En, Kt, Kt2 etc */
    if (SOC_IS_TR_VL(unit)) {
        return (reg_mask_subset_tr(unit, ainfo, mask));
    }
#endif
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        return (reg_mask_subset_sh(unit, ainfo, mask));
    }
#endif
#ifdef BCM_SCORPION_SUPPORT
    if (SOC_IS_SC_CQ(unit)) {
        return (reg_mask_subset_sc(unit, ainfo, mask));
    }
#endif
#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        return (reg_mask_subset_ss(unit, ainfo, mask));
    }
#endif
#ifdef BCM_CALADAN3_SUPPORT
    if (SOC_IS_CALADAN3(unit)) {
        return (reg_mask_subset_c3(unit, ainfo, mask));
    }
#endif
    return 0;
}

/* Some registers need to be skipped only in reset value test,
 * -Not- in RW test. So, we can't use the NOTEST flag. List those regs here
 * For RW test, use the reg_mask_subset
 */
int
rval_test_skip_reg(int unit, soc_regaddrinfo_t *ainfo) {
#ifdef BCM_POLAR_SUPPORT
        if (SOC_IS_POLAR(unit)) {
            switch (ainfo->reg) {
                case EEE_EN_CTRLr:
                case EEE_TXQ_CONG_THr:
                case EEE_RX_IDLE_SYMBOLr:
                case LNKSTSr:
                case INT_STSr:
                case PLL_STSr:
                case TEMP_MON_RESUr:
                case PEAK_TEMP_MON_RESUr:
                case MODEL_IDr:
                case CHIP_REVIDr:
                case RM_PINS_DEBUGr:
                case SWITCH_CTRLr:
                case MDIO_PORT_ADDRr:
                case NSE_DPLL_7_Nr:
                case NSE_NCO_1_Nr:
                case DUPSTSr:
                case LNKSTSCHGr:
                case PAUSESTSr:
                case RESET_STATUSr:
                case SPDSTSr:
                case STRAP_PIN_STATUSr:
                case FC_CHIP_INFOr:
                case FC_GIGA_INFOr:
                case FC_PEAK_RXBYTEr:
                case FC_PEAK_TOTAL_USEDr:
                case FC_TOTAL_USEDr:
                case FC_TOTAL_TH_DROP_Qr:
                case FC_TOTAL_TH_DROP_Q45r:
                case FC_TOTAL_TH_HYST_Qr:
                case FC_TOTAL_TH_HYST_Q45r:
                case FC_TOTAL_TH_PAUSE_Qr:
                case FC_TOTAL_TH_PAUSE_Q45r:
                case FC_TOTAL_TH_RSRV_Qr:
                case FC_TXQ_TH_DROP_Qr:
                case FC_TXQ_TH_DROP_Q45r:
                case FC_TXQ_TH_PAUSE_Qr:
                case FC_TXQ_TH_PAUSE_Q45r:
                case FC_TXQ_TH_RSRV_Qr:
                case FC_TXQ_TH_RSRV_Q45r:
                case FC_TOTAL_TH_DROP_Q_IMPr:
                case FC_TOTAL_TH_HYST_Q_IMPr:
                case FC_TOTAL_TH_PAUSE_Q_IMPr:
                case FC_TXQ_TH_DROP_Q_IMPr:
                case FC_TXQ_TH_PAUSE_Q_IMPr:
                case FC_TXQ_TH_RSRV_Q_IMPr:
                case FC_TOTAL_TH_DROP_Q_WANr:
                case FC_TOTAL_TH_HYST_Q_WANr:
                case FC_TOTAL_TH_PAUSE_Q_WANr:
                case FC_TXQ_TH_DROP_Q_WANr:
                case FC_TXQ_TH_PAUSE_Q_WANr:
                case FC_TXQ_TH_RSRV_Q_WANr:
                    /* Skip these registers */
                    return 1;
                default:
                    if (((ainfo->addr & 0xf000) == 0x1000) ||
                        ((ainfo->addr & 0xf000) == 0x8000)) {
                        /* Skip PHY */
                        return 1;
                    }
                    return 0;
            }
        }
#endif /* BCM_POLAR_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        switch (ainfo->reg) {
            case TOP_MISC_STATUSr:
            case TOP_SWITCH_FEATURE_ENABLE_1r:
            case TOP_SWITCH_FEATURE_ENABLE_2r:
            case TOP_SWITCH_FEATURE_ENABLE_4r:
            case TOP_THERMAL_PVTMON_RESULT_0r:
            case TOP_THERMAL_PVTMON_RESULT_1r:
            case TOP_THERMAL_PVTMON_RESULT_2r:
            case TOP_THERMAL_PVTMON_RESULT_3r:
            case TOP_XGXS_MDIO_CONFIG_0r:
            case TOP_XGXS_MDIO_CONFIG_1r:
            case TOP_XGXS_MDIO_CONFIG_2r:
            case TOP_XGXS_MDIO_CONFIG_3r:
            case TOP_MMU_PLL1_CTRL1r:
            case TOP_MMU_PLL1_CTRL3r:
            case TOP_MMU_PLL2_CTRL1r:
            case TOP_MMU_PLL2_CTRL3r:
            case TOP_MMU_PLL3_CTRL1r:
            case TOP_MMU_PLL3_CTRL3r:
            case TOP_MMU_PLL_STATUS0r:
            case TOP_MMU_PLL_STATUS1r:
            case QUAD0_SERDES_STATUS0r:
            case QUAD0_SERDES_STATUS1r:
            case QUAD1_SERDES_STATUS0r:
            case QUAD0_SERDES_CTRLr:
            case QUAD1_SERDES_CTRLr:
            case MAC_MODEr:
            case OAM_SEC_NS_COUNTER_64r:
            case MMU_ITE_DEBUG_STATUS_0r:
            case TOP_MISC_CONTROL_2r:
            case XPORT_XGXS_NEWCTL_REGr:
            case CI0_TO_EMC_CELL_DATA_RETURN_COUNT_DEBUGr:
            case CI0_TO_EMC_WTAG_RETURN_COUNT_DEBUGr:
            case CI1_TO_EMC_CELL_DATA_RETURN_COUNT_DEBUGr:
            case CI1_TO_EMC_WTAG_RETURN_COUNT_DEBUGr:
            case CI2_TO_EMC_CELL_DATA_RETURN_COUNT_DEBUGr:
            case CI2_TO_EMC_WTAG_RETURN_COUNT_DEBUGr:
            case EMC_BUFFER_EMPTY_FULL_STATUS_DEBUGr:
            case EMC_CFGr:
            case EMC_CI_FULL_STATUS_DEBUGr:
            case EMC_CSDB_0_BUFFER_PAR_STATUS_DEBUGr:
            case EMC_CSDB_1_BUFFER_PAR_STATUS_DEBUGr:
            case EMC_CSDB_2_BUFFER_PAR_STATUS_DEBUGr:
            case EMC_CSDB_MEM_DEBUGr:
            case EMC_ECC_DEBUGr:
            case EMC_ERRB_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_ERRB_BUFFER_FILL_LEVEL_DEBUGr:
            case EMC_ERRB_BUFFER_WATERMARK_DEBUGr:
            case EMC_ERRB_MEM_DEBUGr:
            case EMC_ERRORr:
            case EMC_ERROR_0r:
            case EMC_ERROR_1r:
            case EMC_ERROR_2r:
            case EMC_ERROR_MASK_0r:
            case EMC_ERROR_MASK_1r:
            case EMC_ERROR_MASK_2r:
            case EMC_EWRB_BUFFER_0_ECC_STATUS_DEBUGr:
            case EMC_EWRB_BUFFER_1_ECC_STATUS_DEBUGr:
            case EMC_EWRB_BUFFER_FILL_LEVEL_DEBUGr:
            case EMC_EWRB_BUFFER_WATERMARK_DEBUGr:
            case EMC_EWRB_MEM_DEBUGr:
            case EMC_FREE_POOL_SIZESr:
            case EMC_GLOBAL_1B_ECC_ERROR_COUNT_DEBUGr:
            case EMC_GLOBAL_2B_ECC_ERROR_COUNT_DEBUGr:
            case EMC_IRRB_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_IRRB_BUFFER_FILL_LEVEL_DEBUGr:
            case EMC_IRRB_BUFFER_WATERMARK_DEBUGr:
            case EMC_IRRB_THRESHOLDSr:
            case EMC_IWRB_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_IWRB_BUFFER_FILL_LEVEL_DEBUGr:
            case EMC_IWRB_BUFFER_WATERMARK_DEBUGr:
            case EMC_IWRB_MEM_DEBUGr:
            case EMC_IWRB_SIZEr:
            case EMC_RFCQ_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_RFCQ_BUFFER_FILL_LEVEL_DEBUGr:
            case EMC_RFCQ_BUFFER_WATERMARK_DEBUGr:
            case EMC_RFCQ_MEM_DEBUGr:
            case EMC_RSFP_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_RSFP_BUFFER_FILL_LEVEL_DEBUGr:
            case EMC_RSFP_MEM_DEBUGr:
            case EMC_SWAT_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_SWAT_MEM_DEBUGr:
            case EMC_TO_CI0_RD_REQ_COUNT_DEBUGr:
            case EMC_TO_CI0_WR_REQ_COUNT_DEBUGr:
            case EMC_TO_CI1_RD_REQ_COUNT_DEBUGr:
            case EMC_TO_CI1_WR_REQ_COUNT_DEBUGr:
            case EMC_TO_CI2_RD_REQ_COUNT_DEBUGr:
            case EMC_TO_CI2_WR_REQ_COUNT_DEBUGr:
            case EMC_WCMT_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WCMT_MEM_DEBUGr:
            case EMC_WLCT0_LOWER_A_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT0_LOWER_B_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT0_UPPER_A_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT0_UPPER_B_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT1_LOWER_A_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT1_LOWER_B_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT1_UPPER_A_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT1_UPPER_B_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT2_LOWER_A_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT2_LOWER_B_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT2_UPPER_A_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT2_UPPER_B_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WLCT_MEM_DEBUGr:
            case EMC_WTFP_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WTFP_BUFFER_FILL_LEVEL_DEBUGr:
            case EMC_WTFP_MEM_DEBUGr:
            case EMC_WTOQ_BUFFER_ECC_STATUS_DEBUGr:
            case EMC_WTOQ_BUFFER_FILL_LEVEL_DEBUGr:
            case EMC_WTOQ_MEM_DEBUGr:
            case TOP_SWITCH_FEATURE_ENABLE_3r:
            case QSTRUCT_FAPOTPCONFIG_0r:
            case QSTRUCT_FAPOTPCONFIG_1r:
            case QSTRUCT_FAPOTPCONFIG_2r:
            case QSTRUCT_FAPOTPCONFIG_3r:
            case QSTRUCT_FAPCONFIG_0r:
            case QSTRUCT_FAPCONFIG_1r:
            case QSTRUCT_FAPCONFIG_2r:
            case QSTRUCT_FAPCONFIG_3r:
            case CFAPEOTPCONFIGr:
                return 1;   /* Skip these registers */
            default:
                return 0;
        }
    }
#endif 
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        switch (ainfo->reg) {
            case TOP_HW_TAP_MEM_ECC_CTRLr:
            case TOP_MISC_CONTROL_1r:
            case TOP_MISC_STATUS_0r:
            case TOP_MMU_PLL_INITr:
            case TOP_MMU_PLL_STATUS0r:
            case TOP_MMU_PLL_STATUS1r:
            case TOP_PVTMON_RESULT_0r:
            case TOP_PVTMON_RESULT_1r:
            case TOP_PVTMON_RESULT_2r:
            case TOP_PVTMON_RESULT_3r:
            case TOP_SWITCH_FEATURE_ENABLE_0r:
            case TOP_SWITCH_FEATURE_ENABLE_1r:
            case TOP_SWITCH_FEATURE_ENABLE_2r:
            case TOP_SWITCH_FEATURE_ENABLE_3r:
            case TOP_SWITCH_FEATURE_ENABLE_4r:
            case TOP_SWITCH_FEATURE_ENABLE_5r:
            case TOP_SWITCH_FEATURE_ENABLE_6r:
            case TOP_SW_BOND_OVRD_CTRL0r:
            case TOP_SW_BOND_OVRD_CTRL1r:
            case TOP_UC_TAP_READ_DATAr:
            case MMU_ITE_DEBUG_STATUS_0r:
            case XMAC_RX_LSS_STATUSr:
            case TOP_MISC_CONTROL_2r:
            case TOP_XGXS_MDIO_CONFIG_0r:
            case TOP_XGXS_MDIO_CONFIG_1r:
            case TOP_XGXS_MDIO_CONFIG_2r:
            case TOP_XGXS_MDIO_CONFIG_3r:
            case TOP_MMU_PLL1_CTRL1r:
            case TOP_MMU_PLL1_CTRL3r:
            case TOP_MMU_PLL2_CTRL1r:
            case TOP_MMU_PLL2_CTRL3r:
            case TOP_MMU_PLL3_CTRL1r:
            case TOP_MMU_PLL3_CTRL3r:
            case XPORT_XGXS_NEWCTL_REGr:
            case TOP_TAP_CONTROLr:
            case TOP_MEM_PWR_DWN_BITMAP1_STATUSr:
            case TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r:
            case TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_0r:
            case TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r:
            case TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r:
            case TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_0r:
            case TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_1r:
                return 1;   /* Skip these registers */
            default:
                return 0;
        }
    }
#endif
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        switch (ainfo->reg) {
            case TOP_MISC_CONTROL_3r:
            case CMICTXCOSMASKr:
            case MAC_MODEr:
            case TOP_UC_TAP_READ_DATAr:
            case XLMAC_RX_LSS_STATUSr:
                return 1;   /* Skip these registers */
            default:
                return 0;
        }
    }
#endif
#ifdef BCM_GREYHOUND_SUPPORT
    if(SOC_IS_GREYHOUND(unit)) {
        switch (ainfo->reg) {
            case CMICTXCOSMASKr:
            case PGW_CTRL_0r:
            case XLPORT_LED_CHAIN_CONFIGr:
            case TOP_XG1_LCPLL_FBDIV_CTRL_1r:
            case TOP_XG_PLL0_CTRL_6r:
            case TOP_XG_PLL1_CTRL_1r:
            case TOP_XG_PLL1_CTRL_6r:
            case SPEED_LIMIT_ENTRY0_PHYSICAL_PORT_NUMBERr:
            case SPEED_LIMIT_ENTRY4_PHYSICAL_PORT_NUMBERr:
            case SPEED_LIMIT_ENTRY8_PHYSICAL_PORT_NUMBERr:
            case SPEED_LIMIT_ENTRY13_PHYSICAL_PORT_NUMBERr:
            case SPEED_LIMIT_ENTRY14_PHYSICAL_PORT_NUMBERr:
            case SPEED_LIMIT_ENTRY15_PHYSICAL_PORT_NUMBERr:
            case SPEED_LIMIT_ENTRY16_PHYSICAL_PORT_NUMBERr:
            case SPEED_LIMIT_ENTRY18_PHYSICAL_PORT_NUMBERr:
            case SPEED_LIMIT_ENTRY22_PHYSICAL_PORT_NUMBERr:
            case SPEED_LIMIT_ENTRY23_PHYSICAL_PORT_NUMBERr:
                return 1;
            default:
                break;
        }
    }
#endif
#ifdef BCM_88750_A0
    if(SOC_IS_FE1600(unit)) {
        /*Don't test initialization registers 
          and registers which aren't consistent
          between blocks of the same type*/
        switch(ainfo->reg) {
            case FMAC_ASYNC_FIFO_CONFIGURATIONr:
            case CCS_REG_0058r:
            case DCL_REG_005Cr:
            case FMAC_SBUS_LAST_IN_CHAINr:
            case FSRD_REG_0058r:
            case ECI_FE_1600_SOFT_RESETr:
            case ECI_FE_1600_SOFT_INITr:
            case ECI_SB_RSTN_AND_POWER_DOWNr:
            case FMAC_SBUS_BROADCAST_IDr:
            case CCS_REG_0080r:
            /*skip the following register - 
              expecting a write to eci gloabl register in Fe1600_A0 in order to get reset value
              in contrast to Fe1600_B0*/
            case FMAC_REG_005Ar:
            case DCH_REG_005Ar:
            case DCMA_REG_005Ar:
            case DCMB_REG_005Ar:
            case DCMC_REG_005Ar:
            case DCL_REG_005Ar:
            case RTP_REG_005Ar:
            case MESH_TOPOLOGY_GLOBAL_REGr:
                return 1; /* Skip these registers */
            default:
                break;
        }
    }
#endif  
#ifdef BCM_ARAD_SUPPORT
    if(SOC_IS_ARAD(unit)) {
        switch(ainfo->reg) {
            case ECI_VERSION_REGISTERr:
               return 1; /* Skip these registers */
            default:
                break;
        }
    }
#endif    
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)) {
        uint16 dev_id;
        uint8  rev_id;
        int    skew_id;
        uint32 rval;

        READ_TOP_DEV_REV_IDr(unit, &rval);
        skew_id = soc_reg_field_get(unit, TOP_DEV_REV_IDr, rval, DEVICE_SKEWf);
        soc_cm_get_id(unit, &dev_id, &rev_id);
        switch (ainfo->reg) {
            case TOP_CORE_CLK_FREQ_SELr: 
                if (rev_id == BCM56850_A0_REV_ID || 
                    (BCM56851_DEVICE_ID == dev_id && skew_id == 2)) { /*56851P*/
                    return 1;
                }
                break;
            default:
                break;
        }
    }
#endif
    if (SOC_REG_INFO(unit, ainfo->reg).flags &
        (SOC_REG_FLAG_IGNORE_DEFAULT | SOC_REG_FLAG_RO | SOC_REG_FLAG_WO | SOC_REG_FLAG_INTERRUPT | SOC_REG_FLAG_GENERAL_COUNTER | SOC_REG_FLAG_SIGNAL)) {
        return 1;   /* no testable bits */
    }

    return 0;
}

