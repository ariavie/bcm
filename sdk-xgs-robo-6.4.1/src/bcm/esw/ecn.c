/*
 * $Id: ecn.c,v 1.33 Broadcom SDK $
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
 * ECN - Broadcom StrataSwitch ECN API.
 */

#include <sal/core/libc.h>
#include <soc/defs.h>

#include <soc/drv.h>
#include <soc/mem.h>
#include <bcm/error.h>

#include <bcm/ecn.h>
#include <bcm/debug.h>
#include <bcm/error.h>
#include <bcm/switch.h>
#include <bcm_int/esw/ecn.h>
#include <bcm_int/esw/xgs3.h>
#include <bcm_int/esw_dispatch.h>


#if defined(BCM_GREYHOUND_SUPPORT)

#define _BCM_ECN_INT_CN_RESPONSIVE_DROP           (0x0)
#define _BCM_ECN_INT_CN_NON_RESPONSIVE_DROP       (0x1)
#define _BCM_ECN_INT_CN_NON_CONGESTION_MARK_ECN   (0x2)
#define _BCM_ECN_INT_CN_CONGESTION_MARK_ECN       (0x3)


int
_bcm_esw_ecn_init(int unit)
{
    responsive_protocol_match_entry_t   entry_res_protocol;
    ip_to_int_cn_mapping_entry_t        entry_ip_int_cn;
    int_cn_to_mmuif_mapping_entry_t     entry_int_cn_mmuif;
    egr_int_cn_update_entry_t           entry_egr_int_cn_update;
    egr_int_cn_to_ip_mapping_entry_t    entry_egr_int_cn_ip_mapping;
    int count, i;
    uint32  fld_val;
    uint64  fld64_val;

    if (!SOC_WARM_BOOT(unit)) {
        /* Responsive Protocol Match table */
        /* Set TCP(0x6) to 1 */
        COMPILER_64_ZERO(fld64_val);
        COMPILER_64_SET(fld64_val, 0x0, (0x1 << 6));
        sal_memset(&entry_res_protocol, 0, sizeof(entry_res_protocol));
        soc_mem_field64_set(unit, RESPONSIVE_PROTOCOL_MATCHm, 
                    &entry_res_protocol, RESPONSIVEf, fld64_val);
        soc_mem_write(unit, RESPONSIVE_PROTOCOL_MATCHm, MEM_BLOCK_ALL,
                0, (void *)&entry_res_protocol);

        /* 
         * IP_TO_INT_CN_MAPPING table
            Address (ECN + responsive)  :     INT_CN
                            0                           2'b01
                            1                           2'b00
                            2                           2'b10
                            3                           2'b10
                            4                           2'b10
                            5                           2'b10
                            6                           2'b11
                            7                           2'b11
         */
        count = soc_mem_index_count(unit, IP_TO_INT_CN_MAPPINGm);
        for (i = 0; i < count; i++) {
            sal_memset(&entry_ip_int_cn, 0, sizeof(entry_ip_int_cn));
            switch (i) {
                case 0:
                    fld_val = _BCM_ECN_INT_CN_NON_RESPONSIVE_DROP;
                    break;
                case 1:
                    fld_val = _BCM_ECN_INT_CN_RESPONSIVE_DROP;
                    break;
                case 6:
                case 7:
                    fld_val = _BCM_ECN_INT_CN_CONGESTION_MARK_ECN;
                    break;
                default:
                    fld_val = _BCM_ECN_INT_CN_NON_CONGESTION_MARK_ECN;
                    break;
            }
            soc_mem_field32_set(unit, IP_TO_INT_CN_MAPPINGm, 
                        &entry_ip_int_cn, INT_CNf, fld_val);
            soc_mem_write(unit, IP_TO_INT_CN_MAPPINGm, MEM_BLOCK_ALL,
                i, (void *)&entry_ip_int_cn);
        }


        /* INT_CN_TO_MMUIF_MAPPING */
        count = soc_mem_index_count(unit, INT_CN_TO_MMUIF_MAPPINGm);
        for (i = 0; i < count; i++) {
            sal_memset(&entry_int_cn_mmuif, 0, sizeof(entry_int_cn_mmuif));
            switch (i) {
                case _BCM_ECN_INT_CN_RESPONSIVE_DROP:
                    soc_mem_field32_set(unit, INT_CN_TO_MMUIF_MAPPINGm, 
                        &entry_int_cn_mmuif, WRED_RESPONSIVEf, 1);
                    soc_mem_field32_set(unit, INT_CN_TO_MMUIF_MAPPINGm, 
                        &entry_int_cn_mmuif, WRED_MARK_ELIGIBLEf, 0);
                    break;
                case _BCM_ECN_INT_CN_NON_RESPONSIVE_DROP:
                    soc_mem_field32_set(unit, INT_CN_TO_MMUIF_MAPPINGm, 
                        &entry_int_cn_mmuif, WRED_RESPONSIVEf, 0);
                    soc_mem_field32_set(unit, INT_CN_TO_MMUIF_MAPPINGm, 
                        &entry_int_cn_mmuif, WRED_MARK_ELIGIBLEf, 0);
                    break;
                case _BCM_ECN_INT_CN_CONGESTION_MARK_ECN:
                case _BCM_ECN_INT_CN_NON_CONGESTION_MARK_ECN:
                    soc_mem_field32_set(unit, INT_CN_TO_MMUIF_MAPPINGm, 
                        &entry_int_cn_mmuif, WRED_RESPONSIVEf, 1);
                    soc_mem_field32_set(unit, INT_CN_TO_MMUIF_MAPPINGm, 
                        &entry_int_cn_mmuif, WRED_MARK_ELIGIBLEf, 1);
                    break;
                default:
                    break;
            }
            soc_mem_write(unit, INT_CN_TO_MMUIF_MAPPINGm, MEM_BLOCK_ALL,
                i, (void *)&entry_int_cn_mmuif);
        }

        /* EGR_INT_CN_UPDATE */
        count = soc_mem_index_count(unit, EGR_INT_CN_UPDATEm);
        for (i = 0; i < count; i++) {
            sal_memset(&entry_egr_int_cn_update, 0, 
                                sizeof(entry_egr_int_cn_update));
            if (i < 0x20) {
                /* INT_CN == 0 */
                soc_mem_field32_set(unit, EGR_INT_CN_UPDATEm, 
                    &entry_egr_int_cn_update, INT_CNf, 
                    _BCM_ECN_INT_CN_RESPONSIVE_DROP);
            } else if (i < 0x40) {
                /* INT_CN == 1 */
                soc_mem_field32_set(unit, EGR_INT_CN_UPDATEm, 
                    &entry_egr_int_cn_update, INT_CNf, 
                    _BCM_ECN_INT_CN_NON_RESPONSIVE_DROP);
            } else if (i > 0x5f) {
                /* INT_CN ==  3 */
                soc_mem_field32_set(unit, EGR_INT_CN_UPDATEm, 
                    &entry_egr_int_cn_update, INT_CNf, 
                    _BCM_ECN_INT_CN_CONGESTION_MARK_ECN);
            } else {
                /* INT_CN == 2 */
                /* Change the INT_CN to 3 if local MMU is congested */
                switch (i) {
                    case 0x50:
                    case 0x54:
                    case 0x58:
                    case 0x5c:
                    case 0x4b:
                    case 0x4f:
                    case 0x5b:
                    case 0x5f:
                    case 0x45:
                    case 0x4d:
                    case 0x55:
                    case 0x5d:
                        soc_mem_field32_set(unit, EGR_INT_CN_UPDATEm, 
                            &entry_egr_int_cn_update, INT_CNf, 
                            _BCM_ECN_INT_CN_CONGESTION_MARK_ECN);
                        soc_mem_field32_set(unit, EGR_INT_CN_UPDATEm, 
                            &entry_egr_int_cn_update, CONGESTION_MARKEDf, 1);
                        break;
                    default:
                        soc_mem_field32_set(unit, EGR_INT_CN_UPDATEm, 
                            &entry_egr_int_cn_update, INT_CNf, 
                            _BCM_ECN_INT_CN_NON_CONGESTION_MARK_ECN);
                        break;
                }
            }
            soc_mem_write(unit, EGR_INT_CN_UPDATEm, MEM_BLOCK_ALL,
                i, (void *)&entry_egr_int_cn_update);
        }

        /* EGR_INT_CN_TO_IP_MAPPING */
        count = soc_mem_index_count(unit, EGR_INT_CN_TO_IP_MAPPINGm);
        for (i = 0; i < count; i++) {
            sal_memset(&entry_egr_int_cn_ip_mapping, 0, 
                                sizeof(entry_egr_int_cn_ip_mapping));

            /* Change the ECN value from 2'b01 or 2'b10 to 2'b11 for INT_CN is 3 */
            if ((i == 13) || (i == 14)) {
                soc_mem_field32_set(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, CHANGE_PACKET_ECNf, 1);
                soc_mem_field32_set(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, ECNf, 3);
                soc_mem_field32_set(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, INCREMENT_ECN_COUNTERf, 1);
            }
            soc_mem_write(unit, EGR_INT_CN_TO_IP_MAPPINGm, MEM_BLOCK_ALL,
                i, (void *)&entry_egr_int_cn_ip_mapping);
        }

        /* 
         * Configure the int_cn of Non-IP packets to Non-Responsive Dropping. 
         */
        BCM_IF_ERROR_RETURN(
            bcm_esw_switch_control_set(unit, 
                bcmSwitchEcnNonIpIntCongestionNotification,
                _BCM_ECN_INT_CN_NON_RESPONSIVE_DROP));
    }

    return BCM_E_NONE;
}

#endif /* BCM_GREYHOUND_SUPPORT */

/*
 * Function:
 *      bcm_esw_ecn_responsive_protocol_get
 * Purpose:
 *      To get the value of responsive indication based on the IP protocol value.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ip_proto - (IN) IP Protocol value.
 *      responsice - (OUT) Responsice indication.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int bcm_esw_ecn_responsive_protocol_get(
    int unit, uint8 ip_proto, int *responsive)
{

#if defined(BCM_GREYHOUND_SUPPORT)
    uint64  fld64_val;
    int mem_idx, field_offset;
    responsive_protocol_match_entry_t   entry_res_protocol;

    if (!soc_feature(unit, soc_feature_ecn_wred)) {
        return BCM_E_UNAVAIL;
    }

    COMPILER_64_ZERO(fld64_val);
    mem_idx = ip_proto / 64;
    field_offset = ip_proto % 64;

    sal_memset(&entry_res_protocol, 0, sizeof(entry_res_protocol));
    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, RESPONSIVE_PROTOCOL_MATCHm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_res_protocol));
    soc_mem_field64_get(unit, RESPONSIVE_PROTOCOL_MATCHm, 
                    &entry_res_protocol, RESPONSIVEf, &fld64_val);

    if (COMPILER_64_BITTEST(fld64_val, field_offset)) {
        *responsive = TRUE;
    } else {
        *responsive = FALSE;
    }

    return BCM_E_NONE;
#else /* !BCM_GREYHOUND_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_GREYHOUND_SUPPORT */
    
}


/*
 * Function:
 *      bcm_esw_ecn_responsive_protocol_get
 * Purpose:
 *      To configure the value of responsive indication based on the IP protocol value.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ip_proto - (IN) IP Protocol value.
 *      responsice - (IN) Responsice indication.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */

int bcm_esw_ecn_responsive_protocol_set(
    int unit, uint8 ip_proto, int responsive)
{
#if defined(BCM_GREYHOUND_SUPPORT)
    uint64  fld64_val, val64;
    int mem_idx, field_offset;
    responsive_protocol_match_entry_t   entry_res_protocol;

    if (!soc_feature(unit, soc_feature_ecn_wred)) {
        return BCM_E_UNAVAIL;
    }
    
    COMPILER_64_ZERO(fld64_val);
    mem_idx = ip_proto / 64;
    field_offset = ip_proto % 64;

    sal_memset(&entry_res_protocol, 0, sizeof(entry_res_protocol));
    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, RESPONSIVE_PROTOCOL_MATCHm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_res_protocol));
    soc_mem_field64_get(unit, RESPONSIVE_PROTOCOL_MATCHm, 
                    &entry_res_protocol, RESPONSIVEf, &fld64_val);

    COMPILER_64_ZERO(val64);
    COMPILER_64_SET(val64, 0x0, 0x1);
    COMPILER_64_SHL(val64, field_offset);
    if (responsive) {
        COMPILER_64_OR(fld64_val, val64);
    } else {
        COMPILER_64_NOT(val64);
        COMPILER_64_AND(fld64_val, val64);
    }

    soc_mem_field64_set(unit, RESPONSIVE_PROTOCOL_MATCHm, 
                    &entry_res_protocol, RESPONSIVEf, fld64_val);
    BCM_IF_ERROR_RETURN(
        soc_mem_write(unit, RESPONSIVE_PROTOCOL_MATCHm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_res_protocol));

    return BCM_E_NONE;
#else /* !BCM_GREYHOUND_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_GREYHOUND_SUPPORT */
}


/*
 * Function:
 *      bcm_esw_ecn_traffic_map_get
 * Purpose:
 *      To get the mapped internal congestion notification (int_cn) value.
 * Parameters:
 *      unit - (IN) Unit number.
 *      map - (INOUT) Internal congestion notification map.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */

int bcm_esw_ecn_traffic_map_get(
    int unit, bcm_ecn_traffic_map_info_t *map)
{
#if defined(BCM_GREYHOUND_SUPPORT)
    int mem_idx;
    ip_to_int_cn_mapping_entry_t        entry_ip_int_cn;
    uint32      fld_val;

    if (!soc_feature(unit, soc_feature_ecn_wred)) {
        return BCM_E_UNAVAIL;
    }

    if (map == NULL) {
        return BCM_E_PARAM;
    }

    if (map->ecn > _BCM_ECN_VALUE_MAX) {
        return BCM_E_PARAM;
    }

    /* Check supported flag */
    if ((map->flags & ~BCM_ECN_TRAFFIC_MAP_RESPONSIVE) != 0) {
        return BCM_E_PARAM;
    }

    /* MEMORY INDEX : ECN + Responsive */
    mem_idx = (map->ecn << 1);
    if (map->flags & BCM_ECN_TRAFFIC_MAP_RESPONSIVE) {
        mem_idx++;
    }

    sal_memset(&entry_ip_int_cn, 0, sizeof(entry_ip_int_cn));

    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, IP_TO_INT_CN_MAPPINGm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_ip_int_cn));

    fld_val = soc_mem_field32_get(unit, IP_TO_INT_CN_MAPPINGm, 
                        &entry_ip_int_cn, INT_CNf);

    map->int_cn = fld_val;
    

    return BCM_E_NONE;
#else /* !BCM_GREYHOUND_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_GREYHOUND_SUPPORT */
}
 
/*
 * Function:
 *      bcm_esw_ecn_traffic_map_set
 * Purpose:
 *      To set the mapped internal congestion notification (int_cn) value.
 * Parameters:
 *      unit - (IN) Unit number.
 *      map - (INOUT) Internal congestion notification map.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int bcm_esw_ecn_traffic_map_set(
    int unit, bcm_ecn_traffic_map_info_t *map)
{
#if defined(BCM_GREYHOUND_SUPPORT)
    int mem_idx;
    ip_to_int_cn_mapping_entry_t        entry_ip_int_cn;
    uint32      fld_val;

    if (!soc_feature(unit, soc_feature_ecn_wred)) {
        return BCM_E_UNAVAIL;
    }

    if (map == NULL) {
        return BCM_E_PARAM;
    }

    if (map->ecn > _BCM_ECN_VALUE_MAX) {
        return BCM_E_PARAM;
    }

    if ((map->int_cn < 0) || (map->int_cn > _BCM_ECN_INT_CN_VALUE_MAX)) {
        return BCM_E_PARAM;
    }

    /* MEMORY INDEX : ECN + Responsive */
    mem_idx = (map->ecn << 1);
    if (map->flags & BCM_ECN_TRAFFIC_MAP_RESPONSIVE) {
        mem_idx++;
    }

    sal_memset(&entry_ip_int_cn, 0, sizeof(entry_ip_int_cn));

    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, IP_TO_INT_CN_MAPPINGm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_ip_int_cn));

    fld_val = map->int_cn;
    soc_mem_field32_set(unit, IP_TO_INT_CN_MAPPINGm, 
                        &entry_ip_int_cn, INT_CNf, fld_val);

    BCM_IF_ERROR_RETURN(
        soc_mem_write(unit, IP_TO_INT_CN_MAPPINGm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_ip_int_cn));

    return BCM_E_NONE;
#else /* !BCM_GREYHOUND_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_GREYHOUND_SUPPORT */
}


#if defined(BCM_GREYHOUND_SUPPORT)
static int
_bcm_esw_ecn_action_enqueue_get(int unit, 
    bcm_ecn_traffic_action_config_t *ecn_config)
{

    int_cn_to_mmuif_mapping_entry_t     entry_int_cn_mmuif;
    int     mem_idx;
    uint32  fld_val;
    
    if ((ecn_config->int_cn < 0) || 
        (ecn_config->int_cn > _BCM_ECN_INT_CN_VALUE_MAX)) {
        return BCM_E_PARAM;
    }

    mem_idx = ecn_config->int_cn;
    sal_memset(&entry_int_cn_mmuif, 0, sizeof(entry_int_cn_mmuif));

    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, INT_CN_TO_MMUIF_MAPPINGm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_int_cn_mmuif));

    /* WRED RESPONSIVE */
    fld_val = soc_mem_field32_get(unit, INT_CN_TO_MMUIF_MAPPINGm, 
                        &entry_int_cn_mmuif, WRED_RESPONSIVEf);
    if (fld_val) {
        ecn_config->action_flags |= 
            BCM_ECN_TRAFFIC_ACTION_ENQUEUE_WRED_RESPONSIVE;
    } else {
        ecn_config->action_flags &= 
            ~BCM_ECN_TRAFFIC_ACTION_ENQUEUE_WRED_RESPONSIVE;
    }

    /* MARK ELIGIBLE */
    fld_val = soc_mem_field32_get(unit, INT_CN_TO_MMUIF_MAPPINGm, 
                        &entry_int_cn_mmuif, WRED_MARK_ELIGIBLEf);
    if (fld_val) {
        ecn_config->action_flags |= 
            BCM_ECN_TRAFFIC_ACTION_ENQUEUE_MARK_ELIGIBLE;
    } else {
        ecn_config->action_flags &= 
            ~BCM_ECN_TRAFFIC_ACTION_ENQUEUE_MARK_ELIGIBLE;
    }

    return BCM_E_NONE;
}

static int
_bcm_esw_ecn_action_enqueue_set(int unit, 
    bcm_ecn_traffic_action_config_t *ecn_config)
{

    int_cn_to_mmuif_mapping_entry_t     entry_int_cn_mmuif;
    int     mem_idx;
    uint32  fld_val;
    
    if ((ecn_config->int_cn < 0) || 
        (ecn_config->int_cn > _BCM_ECN_INT_CN_VALUE_MAX)) {
        return BCM_E_PARAM;
    }

    mem_idx = ecn_config->int_cn;
    sal_memset(&entry_int_cn_mmuif, 0, sizeof(entry_int_cn_mmuif));

    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, INT_CN_TO_MMUIF_MAPPINGm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_int_cn_mmuif));

    /* WRED RESPONSIVE */
    if (ecn_config->action_flags & 
        BCM_ECN_TRAFFIC_ACTION_ENQUEUE_WRED_RESPONSIVE) {
        fld_val = 1;
    } else {
        fld_val = 0;
    }
    soc_mem_field32_set(unit, INT_CN_TO_MMUIF_MAPPINGm, 
                        &entry_int_cn_mmuif, WRED_RESPONSIVEf, fld_val);

    /* MARK ELIGIBLE */
    if (ecn_config->action_flags & 
        BCM_ECN_TRAFFIC_ACTION_ENQUEUE_MARK_ELIGIBLE) {
        fld_val = 1;
    } else {
        fld_val = 0;
    }
    soc_mem_field32_set(unit, INT_CN_TO_MMUIF_MAPPINGm, 
                        &entry_int_cn_mmuif, WRED_MARK_ELIGIBLEf, fld_val);

    BCM_IF_ERROR_RETURN(
        soc_mem_write(unit, INT_CN_TO_MMUIF_MAPPINGm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_int_cn_mmuif));

    return BCM_E_NONE;
}

static int
_bcm_esw_ecn_action_dequeue_get(int unit, 
    bcm_ecn_traffic_action_config_t *ecn_config)
{
    int mem_idx, cng, ce_green, ce_yellow, ce_red, index_max;
    egr_int_cn_update_entry_t           entry_egr_int_cn_update;
    uint32      fld_val;
    
    if ((ecn_config->color != bcmColorGreen) && 
        (ecn_config->color != bcmColorYellow) &&
        (ecn_config->color != bcmColorRed)) {
        return BCM_E_PARAM;
    }

    if ((ecn_config->int_cn < 0) || 
        (ecn_config->int_cn > _BCM_ECN_INT_CN_VALUE_MAX)) {
        return BCM_E_PARAM;
    }

    /* 
     * Memory index : int_cn(2 bits)+ce_green+ce_yellow+ce_red+cng(2bit) 
     * Only the ce_[color] match the value of cng is meaningful.
     * 
    */
    cng = _BCM_COLOR_ENCODING(unit, ecn_config->color);
    index_max = soc_mem_index_max(unit, EGR_INT_CN_UPDATEm);
    ce_green = 0;
    ce_yellow = 0;
    ce_red = 0;
    mem_idx = (ecn_config->int_cn << _BCM_ECN_DEQUEUE_MEM_INDEX_INT_CN_OFFSET) +
        (cng << _BCM_ECN_DEQUEUE_MEM_INDEX_CNG_OFFSET);
    if (mem_idx > index_max) {
        return BCM_E_PARAM;
    }
    
    sal_memset(&entry_egr_int_cn_update, 0, 
                                sizeof(entry_egr_int_cn_update));
    /* Get the updated int_cn when congestion is not exprienced */
    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, EGR_INT_CN_UPDATEm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_egr_int_cn_update));
    fld_val = soc_mem_field32_get(unit, EGR_INT_CN_UPDATEm, 
                    &entry_egr_int_cn_update, INT_CNf);
    if (fld_val != ecn_config->int_cn) {
        ecn_config->action_flags |= 
            BCM_ECN_TRAFFIC_ACTION_DEQUEUE_NON_CONGESTION_INT_CN_UPDATE;
        ecn_config->non_congested_int_cn = fld_val;
    } else {
        ecn_config->action_flags &= 
            ~BCM_ECN_TRAFFIC_ACTION_DEQUEUE_NON_CONGESTION_INT_CN_UPDATE;
    }

    /* Get the updated int_cn when congestion is exprienced */
    if (ecn_config->color == bcmColorGreen) {
        ce_green = 1;
    } else if (ecn_config->color == bcmColorYellow) {
        ce_yellow = 1;
    } else {
        /* Red */
        ce_red = 1;
    }
    mem_idx = (ecn_config->int_cn << _BCM_ECN_DEQUEUE_MEM_INDEX_INT_CN_OFFSET) +
        (ce_green << _BCM_ECN_DEQUEUE_MEM_INDEX_CE_GREEN_OFFSET) +
        (ce_yellow << _BCM_ECN_DEQUEUE_MEM_INDEX_CE_YELLOW_OFFSET) +
        (ce_red << _BCM_ECN_DEQUEUE_MEM_INDEX_CE_RED_OFFSET) +
        (cng << _BCM_ECN_DEQUEUE_MEM_INDEX_CNG_OFFSET);
    
    if (mem_idx > index_max) {
        return BCM_E_PARAM;
    }
    sal_memset(&entry_egr_int_cn_update, 0, 
                                sizeof(entry_egr_int_cn_update));

    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, EGR_INT_CN_UPDATEm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_egr_int_cn_update));
    fld_val = soc_mem_field32_get(unit, EGR_INT_CN_UPDATEm, 
                    &entry_egr_int_cn_update, INT_CNf);

    if (fld_val != ecn_config->int_cn) {
        ecn_config->action_flags |= 
            BCM_ECN_TRAFFIC_ACTION_DEQUEUE_CONGESTION_INT_CN_UPDATE;
        ecn_config->congested_int_cn = fld_val;
    } else {
        ecn_config->action_flags &= 
            ~BCM_ECN_TRAFFIC_ACTION_DEQUEUE_CONGESTION_INT_CN_UPDATE;
    }
    
    return BCM_E_NONE;
    
}


static int
_bcm_esw_ecn_action_dequeue_set(int unit, 
    bcm_ecn_traffic_action_config_t *ecn_config)
{
    int mem_idx, cng, ce_green, ce_yellow, ce_red, index_max;
    egr_int_cn_update_entry_t           entry_egr_int_cn_update;
    uint32      fld_val;
    
    if ((ecn_config->color != bcmColorGreen) && 
        (ecn_config->color != bcmColorYellow) &&
        (ecn_config->color != bcmColorRed)) {
        return BCM_E_PARAM;
    }

    if ((ecn_config->int_cn < 0) || 
        (ecn_config->int_cn > _BCM_ECN_INT_CN_VALUE_MAX)) {
        return BCM_E_PARAM;
    }

    /* 
     * Memory index : int_cn(2 bits)+ce_green+ce_yellow+ce_red+cng(2bit) 
     * Only the ce_[color] match the value of cng is meaningful.
     * 
    */
    cng = _BCM_COLOR_ENCODING(unit, ecn_config->color);
    index_max = soc_mem_index_max(unit, EGR_INT_CN_UPDATEm);
    ce_green = 0;
    ce_yellow = 0;
    ce_red = 0;

    /* Update the int_cn value when congestion is not experienced */
    mem_idx = 
        (ecn_config->int_cn << _BCM_ECN_DEQUEUE_MEM_INDEX_INT_CN_OFFSET) +
        (cng << _BCM_ECN_DEQUEUE_MEM_INDEX_CNG_OFFSET);
    if (mem_idx > index_max) {
        return BCM_E_PARAM;
    }
    sal_memset(&entry_egr_int_cn_update, 0, 
                                sizeof(entry_egr_int_cn_update));
    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, EGR_INT_CN_UPDATEm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_egr_int_cn_update));
    if ((ecn_config->action_flags & 
        BCM_ECN_TRAFFIC_ACTION_DEQUEUE_NON_CONGESTION_INT_CN_UPDATE) &&
        (ecn_config->non_congested_int_cn != ecn_config->int_cn)) {

        if ((ecn_config->non_congested_int_cn < 0) || 
            (ecn_config->non_congested_int_cn > _BCM_ECN_INT_CN_VALUE_MAX)) {
            return BCM_E_PARAM;
        }
        fld_val = ecn_config->non_congested_int_cn;
    } else {
        fld_val = ecn_config->int_cn;
    }
    soc_mem_field32_set(unit, EGR_INT_CN_UPDATEm, 
                        &entry_egr_int_cn_update, INT_CNf, 
                        fld_val);
    BCM_IF_ERROR_RETURN(
            soc_mem_write(unit, EGR_INT_CN_UPDATEm, MEM_BLOCK_ALL,
                    mem_idx, (void *)&entry_egr_int_cn_update));
    

    /* Update the int_cn value when congestion is experienced */
    if (ecn_config->color == bcmColorGreen) {
        ce_green = 1;
    } else if (ecn_config->color == bcmColorYellow) {
        ce_yellow = 1;
    } else {
        /* Red */
        ce_red = 1;
    }
    mem_idx = 
        (ecn_config->int_cn << _BCM_ECN_DEQUEUE_MEM_INDEX_INT_CN_OFFSET) +
        (ce_green << _BCM_ECN_DEQUEUE_MEM_INDEX_CE_GREEN_OFFSET) +
        (ce_yellow << _BCM_ECN_DEQUEUE_MEM_INDEX_CE_YELLOW_OFFSET) +
        (ce_red << _BCM_ECN_DEQUEUE_MEM_INDEX_CE_RED_OFFSET) +
        (cng << _BCM_ECN_DEQUEUE_MEM_INDEX_CNG_OFFSET);
    if (mem_idx > index_max) {
        return BCM_E_PARAM;
    }
    sal_memset(&entry_egr_int_cn_update, 0, 
                                sizeof(entry_egr_int_cn_update));

    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, EGR_INT_CN_UPDATEm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_egr_int_cn_update));
    if ((ecn_config->action_flags & 
        BCM_ECN_TRAFFIC_ACTION_DEQUEUE_CONGESTION_INT_CN_UPDATE) &&
        (ecn_config->congested_int_cn != ecn_config->int_cn)) {

        if ((ecn_config->congested_int_cn < 0) || 
            (ecn_config->congested_int_cn > _BCM_ECN_INT_CN_VALUE_MAX)) {
            return BCM_E_PARAM;
        }
        fld_val = ecn_config->congested_int_cn;
        
    } else {
        fld_val = ecn_config->int_cn;
    }
    soc_mem_field32_set(unit, EGR_INT_CN_UPDATEm, 
                    &entry_egr_int_cn_update, INT_CNf, fld_val);
    BCM_IF_ERROR_RETURN(
        soc_mem_write(unit, EGR_INT_CN_UPDATEm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_egr_int_cn_update));
    
    return BCM_E_NONE;
    
}


static int
_bcm_esw_ecn_action_egress_get(int unit, 
    bcm_ecn_traffic_action_config_t *ecn_config)
{
    int mem_idx, index_max;
    egr_int_cn_to_ip_mapping_entry_t    entry_egr_int_cn_ip_mapping;
    uint32  fld_val;
    
    if ((ecn_config->int_cn < 0) || 
        (ecn_config->int_cn > _BCM_ECN_INT_CN_VALUE_MAX)) {
        return BCM_E_PARAM;
    }

    if (ecn_config->ecn > _BCM_ECN_VALUE_MAX) {
        return BCM_E_PARAM;
    }

    mem_idx = (ecn_config->int_cn << _BCM_ECN_EGRESS_MEM_INDEX_INT_CN_OFFSET) +
        (ecn_config->ecn << _BCM_ECN_EGRESS_MEM_INDEX_ECN_OFFSET);
    index_max = soc_mem_index_max(unit, EGR_INT_CN_TO_IP_MAPPINGm);

    if (mem_idx > index_max) {
        return BCM_E_PARAM;
    }
    sal_memset(&entry_egr_int_cn_ip_mapping, 0, 
                    sizeof(entry_egr_int_cn_ip_mapping));

    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, EGR_INT_CN_TO_IP_MAPPINGm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_egr_int_cn_ip_mapping));
    
    /* Check ECN MARKING action */
    fld_val = soc_mem_field32_get(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, 
                    CHANGE_PACKET_ECNf);
    
    if (fld_val) {
        ecn_config->action_flags |= 
            BCM_ECN_TRAFFIC_ACTION_EGRESS_ECN_MARKING;
        fld_val = soc_mem_field32_get(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, ECNf);
        ecn_config->new_ecn = fld_val;
    } else {
        ecn_config->action_flags &= 
            ~BCM_ECN_TRAFFIC_ACTION_EGRESS_ECN_MARKING;
    }

    /* Check Drop action */
    fld_val = soc_mem_field32_get(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, DROPf);
    if (fld_val) {
        ecn_config->action_flags |= 
            BCM_ECN_TRAFFIC_ACTION_EGRESS_DROP;
    } else {
        ecn_config->action_flags &= 
            ~BCM_ECN_TRAFFIC_ACTION_EGRESS_DROP;
    }
    
    
    return BCM_E_NONE;
}


static int
_bcm_esw_ecn_action_egress_set(int unit, 
    bcm_ecn_traffic_action_config_t *ecn_config)
{
    int mem_idx, index_max;
    egr_int_cn_to_ip_mapping_entry_t    entry_egr_int_cn_ip_mapping;
    uint32  fld_val;
    
    if ((ecn_config->int_cn < 0) || 
        (ecn_config->int_cn > _BCM_ECN_INT_CN_VALUE_MAX)) {
        return BCM_E_PARAM;
    }

    if ((ecn_config->ecn > _BCM_ECN_VALUE_MAX) ||
        (ecn_config->new_ecn > _BCM_ECN_VALUE_MAX)) {
        return BCM_E_PARAM;
    }

    mem_idx = (ecn_config->int_cn << _BCM_ECN_EGRESS_MEM_INDEX_INT_CN_OFFSET) +
        (ecn_config->ecn << _BCM_ECN_EGRESS_MEM_INDEX_ECN_OFFSET);
    index_max = soc_mem_index_max(unit, EGR_INT_CN_TO_IP_MAPPINGm);

    if (mem_idx > index_max) {
        return BCM_E_PARAM;
    }
    sal_memset(&entry_egr_int_cn_ip_mapping, 0, 
                    sizeof(entry_egr_int_cn_ip_mapping));

    BCM_IF_ERROR_RETURN(
        soc_mem_read(unit, EGR_INT_CN_TO_IP_MAPPINGm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_egr_int_cn_ip_mapping));

    /* ECN Marking action */
    if (ecn_config->action_flags & BCM_ECN_TRAFFIC_ACTION_EGRESS_ECN_MARKING) {

        soc_mem_field32_set(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, 
                    CHANGE_PACKET_ECNf, 1);
        soc_mem_field32_set(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, 
                    INCREMENT_ECN_COUNTERf, 1);
        fld_val = ecn_config->new_ecn;
        soc_mem_field32_set(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, 
                    ECNf, fld_val);
    } else {
        soc_mem_field32_set(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, 
                    CHANGE_PACKET_ECNf, 0);
        soc_mem_field32_set(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, 
                    INCREMENT_ECN_COUNTERf, 0);
    }

    /* Drop action */
    if (ecn_config->action_flags & BCM_ECN_TRAFFIC_ACTION_EGRESS_DROP) {
        fld_val = 1;
    } else {
        fld_val = 0;
    }
    soc_mem_field32_set(unit, EGR_INT_CN_TO_IP_MAPPINGm, 
                    &entry_egr_int_cn_ip_mapping, 
                    DROPf, fld_val);

    BCM_IF_ERROR_RETURN(
        soc_mem_write(unit, EGR_INT_CN_TO_IP_MAPPINGm, MEM_BLOCK_ALL,
                mem_idx, (void *)&entry_egr_int_cn_ip_mapping));
    
    
    return BCM_E_NONE;
}
#endif /* BCM_GREYHOUND_SUPPORT */

/*
 * Function:
 *      bcm_esw_ecn_traffic_action_config_get
 * Purpose:
 *      To get the actions of the specified ECN traffic.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ecn_config - (INOUT) ECN traffic action configuration.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */

int 
bcm_esw_ecn_traffic_action_config_get(
    int unit, bcm_ecn_traffic_action_config_t *ecn_config)
{
#if defined(BCM_GREYHOUND_SUPPORT)
    int rv = BCM_E_NONE;
    
    if (!soc_feature(unit, soc_feature_ecn_wred)) {
        return BCM_E_UNAVAIL;
    }

    if (ecn_config == NULL) {
        return BCM_E_PARAM;
    }

    switch(ecn_config->action_type) {
        case BCM_ECN_TRAFFIC_ACTION_TYPE_ENQUEUE:
            rv = _bcm_esw_ecn_action_enqueue_get(unit, ecn_config);
            break;
        case BCM_ECN_TRAFFIC_ACTION_TYPE_DEQUEUE:
            rv = _bcm_esw_ecn_action_dequeue_get(unit, ecn_config);
            break;
        case BCM_ECN_TRAFFIC_ACTION_TYPE_EGRESS:
            rv = _bcm_esw_ecn_action_egress_get(unit, ecn_config);
            break;
        default:
            rv = BCM_E_PARAM;
    }
    
    return rv;
#else /* !BCM_GREYHOUND_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_GREYHOUND_SUPPORT */
}


/*
 * Function:
 *      bcm_esw_ecn_traffic_action_config_set
 * Purpose:
 *      Assign the actions of the specified ECN traffic.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ecn_config - (IN) ECN traffic action configuration.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */

int 
bcm_esw_ecn_traffic_action_config_set(
    int unit, bcm_ecn_traffic_action_config_t *ecn_config)
{
#if defined(BCM_GREYHOUND_SUPPORT)
    int rv = BCM_E_NONE;
    
    if (!soc_feature(unit, soc_feature_ecn_wred)) {
        return BCM_E_UNAVAIL;
    }

    if (ecn_config == NULL) {
        return BCM_E_PARAM;
    }

    if (ecn_config->action_flags &
        ~(BCM_ECN_TRAFFIC_ACTION_ENQUEUE_WRED_RESPONSIVE |
        BCM_ECN_TRAFFIC_ACTION_ENQUEUE_MARK_ELIGIBLE |
        BCM_ECN_TRAFFIC_ACTION_DEQUEUE_CONGESTION_INT_CN_UPDATE |
        BCM_ECN_TRAFFIC_ACTION_DEQUEUE_NON_CONGESTION_INT_CN_UPDATE |
        BCM_ECN_TRAFFIC_ACTION_EGRESS_ECN_MARKING |
        BCM_ECN_TRAFFIC_ACTION_EGRESS_DROP)) {
        return BCM_E_PARAM;
    }

    switch(ecn_config->action_type) {
        case BCM_ECN_TRAFFIC_ACTION_TYPE_ENQUEUE:
            rv = _bcm_esw_ecn_action_enqueue_set(unit, ecn_config);
            break;
        case BCM_ECN_TRAFFIC_ACTION_TYPE_DEQUEUE:
            rv = _bcm_esw_ecn_action_dequeue_set(unit, ecn_config);
            break;
        case BCM_ECN_TRAFFIC_ACTION_TYPE_EGRESS:
            rv = _bcm_esw_ecn_action_egress_set(unit, ecn_config);
            break;
        default:
            rv = BCM_E_PARAM;
    }

    return rv;
#else /* !BCM_GREYHOUND_SUPPORT*/
    return BCM_E_UNAVAIL;
#endif /* BCM_GREYHOUND_SUPPORT */    
}

