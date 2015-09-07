/*
 * $Id: port.c,v 1.4 Broadcom SDK $
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
 */

#include <soc/defs.h>
#include <sal/core/libc.h>

#if defined(BCM_TRIUMPH3_SUPPORT)

#include <soc/drv.h>
#include <bcm/error.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/esw_dispatch.h>

/*
 * Function:
 *      bcm_tr3_port_esm_eligible_set
 * Description:
 *      Set a port's ESM lookup eligibility. The port may be a remote port.
 * Parameters:
 *      unit  - (IN) Device number
 *      port  - (IN) Port number
 *      value - (IN) TRUE = eligible, FALSE = not eligible.
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_tr3_port_esm_eligible_set(int unit, bcm_port_t port, int value)
{
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t tid_out;
    int id_out;
    int stm_index;
    bcm_module_t my_modid;
    int rv = BCM_E_NONE;
    source_trunk_map_table_entry_t stm_entry;

    if (!soc_feature(unit, soc_feature_etu_support)) {
        return BCM_E_UNAVAIL;
    }

    /* Derive index to SOURCE_TRUNK_MAP table based on module ID and port */
    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_gport_resolve(unit, port,
                    &mod_out, &port_out, &tid_out, &id_out));
        if (-1 != tid_out || -1 != id_out) {
            return BCM_E_PORT;
        }
        BCM_IF_ERROR_RETURN(_bcm_esw_src_mod_port_table_index_get(unit,
                    mod_out, port_out, &stm_index));
    } else {
        if (!SOC_PORT_VALID(unit, port)) {
            return BCM_E_PORT;
        }
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
        BCM_IF_ERROR_RETURN(_bcm_esw_src_mod_port_table_index_get(unit,
                    my_modid, port, &stm_index));
    }

    /* Update ESM fields */
    soc_mem_lock(unit, SOURCE_TRUNK_MAP_TABLEm);
    rv = READ_SOURCE_TRUNK_MAP_TABLEm(unit, MEM_BLOCK_ANY, stm_index,
            &stm_entry); 
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, SOURCE_TRUNK_MAP_TABLEm);
        return rv;
    }

    if (value) {
        soc_SOURCE_TRUNK_MAP_TABLEm_field32_set(unit, &stm_entry,
                ESM_SEARCH_OFFSETf, 1);
        soc_SOURCE_TRUNK_MAP_TABLEm_field32_set(unit, &stm_entry,
                ESM_SEARCH_PRIORITYf, 7);
    } else {
        soc_SOURCE_TRUNK_MAP_TABLEm_field32_set(unit, &stm_entry,
                ESM_SEARCH_OFFSETf, 0);
        soc_SOURCE_TRUNK_MAP_TABLEm_field32_set(unit, &stm_entry,
                ESM_SEARCH_PRIORITYf, 0);
    }

    rv = WRITE_SOURCE_TRUNK_MAP_TABLEm(unit, MEM_BLOCK_ALL, stm_index,
            &stm_entry); 
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, SOURCE_TRUNK_MAP_TABLEm);
        return rv;
    }

    soc_mem_unlock(unit, SOURCE_TRUNK_MAP_TABLEm);
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_port_esm_eligible_get
 * Description:
 *      Get a port's ESM lookup eligibility. The port may be a remote port.
 * Parameters:
 *      unit  - (IN) Device number
 *      port  - (IN) Port number
 *      value - (OUT) TRUE = eligible, FALSE = not eligible.
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_tr3_port_esm_eligible_get(int unit, bcm_port_t port, int *value)
{
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t tid_out;
    int id_out;
    int stm_index;
    bcm_module_t my_modid;
    source_trunk_map_table_entry_t stm_entry;

    if (!soc_feature(unit, soc_feature_etu_support)) {
        return BCM_E_UNAVAIL;
    }

    /* Derive index to SOURCE_TRUNK_MAP table based on module ID and port */
    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_gport_resolve(unit, port,
                    &mod_out, &port_out, &tid_out, &id_out));
        if (-1 != tid_out || -1 != id_out) {
            return BCM_E_PORT;
        }
        BCM_IF_ERROR_RETURN(_bcm_esw_src_mod_port_table_index_get(unit,
                    mod_out, port_out, &stm_index));
    } else {
        if (!SOC_PORT_VALID(unit, port)) {
            return BCM_E_PORT;
        }
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
        BCM_IF_ERROR_RETURN(_bcm_esw_src_mod_port_table_index_get(unit,
                    my_modid, port, &stm_index));
    }

    /* Get ESM fields */
    SOC_IF_ERROR_RETURN(READ_SOURCE_TRUNK_MAP_TABLEm(unit, MEM_BLOCK_ANY,
                stm_index, &stm_entry)); 
    if (soc_SOURCE_TRUNK_MAP_TABLEm_field32_get(unit, &stm_entry,
                ESM_SEARCH_OFFSETf) &&
        soc_SOURCE_TRUNK_MAP_TABLEm_field32_get(unit, &stm_entry,
                ESM_SEARCH_PRIORITYf)) {
        *value = TRUE;
    } else {
        *value = FALSE;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_port_extender_type_set
 * Description:
 *      Set Port Extender port type.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 *      value - (IN) Port type 
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_tr3_port_extender_type_set(int unit, bcm_port_t port, int value)
{
    switch (value) {
        case BCM_PORT_EXTENDER_TYPE_SWITCH:
            BCM_IF_ERROR_RETURN(bcm_td_port_niv_type_set(unit, port,
                        BCM_PORT_NIV_TYPE_SWITCH));
            break;
        case BCM_PORT_EXTENDER_TYPE_UPLINK:
            BCM_IF_ERROR_RETURN(bcm_td_port_niv_type_set(unit, port,
                        BCM_PORT_NIV_TYPE_UPLINK));
            break;
        case BCM_PORT_EXTENDER_TYPE_DOWNLINK_ACCESS:
            BCM_IF_ERROR_RETURN(bcm_td_port_niv_type_set(unit, port,
                        BCM_PORT_NIV_TYPE_DOWNLINK_ACCESS));
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, VNTAG_ACTIONS_IF_NOT_PRESENTf,
                        2));
            break;
        case BCM_PORT_EXTENDER_TYPE_DOWNLINK_TRANSIT:
            BCM_IF_ERROR_RETURN(bcm_td_port_niv_type_set(unit, port,
                        BCM_PORT_NIV_TYPE_DOWNLINK_TRANSIT));
            break;
        default:
            return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_port_extender_type_get
 * Description:
 *      Get Port Extender port type.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 *      value - (IN) Port type 
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_tr3_port_extender_type_get(int unit, bcm_port_t port, int *value)
{
    int type;

    BCM_IF_ERROR_RETURN(bcm_td_port_niv_type_get(unit, port, &type));
    switch (type) {
        case BCM_PORT_NIV_TYPE_SWITCH:
            *value = BCM_PORT_EXTENDER_TYPE_SWITCH;
            break;
        case BCM_PORT_NIV_TYPE_UPLINK:
            *value = BCM_PORT_EXTENDER_TYPE_UPLINK;
            break;
        case BCM_PORT_NIV_TYPE_DOWNLINK_ACCESS:
            *value = BCM_PORT_EXTENDER_TYPE_DOWNLINK_ACCESS;
            break;
        case BCM_PORT_NIV_TYPE_DOWNLINK_TRANSIT:
            *value = BCM_PORT_EXTENDER_TYPE_DOWNLINK_TRANSIT;
            break;
        default:
            return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

#if defined(INCLUDE_L3) 
/*
 * Function:
 *      bcm_tr3_port_etag_pcp_de_source_set
 * Description:
 *      Set source of ETAG's PCP and DE fields.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 *      value - (IN) PCP and DE source
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_tr3_port_etag_pcp_de_source_set(int unit, bcm_port_t port, int value)
{
    int source;

    switch (value) {
        case BCM_EXTENDER_PCP_DE_SELECT_OUTER_TAG:
            source = 0;
            break;
        case BCM_EXTENDER_PCP_DE_SELECT_INNER_TAG:
            source = 1;
            break;
        case BCM_EXTENDER_PCP_DE_SELECT_DEFAULT:
            source = 2;
            break;
        case BCM_EXTENDER_PCP_DE_SELECT_PHB:
            source = 3;
            break;
        default:
            return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                ETAG_PCP_DE_SOURCEf, source));
    BCM_IF_ERROR_RETURN(soc_mem_field32_modify(unit, EGR_ING_PORTm, port, 
                ETAG_PCP_DE_SOURCEf, source));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_port_etag_pcp_de_source_get
 * Description:
 *      Get source of ETAG's PCP and DE fields.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 *      value - (IN) PCP and DE source
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_tr3_port_etag_pcp_de_source_get(int unit, bcm_port_t port, int *value)
{
    int source;

    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_get(unit, port, ETAG_PCP_DE_SOURCEf,
                &source));

    switch (source) {
        case 0:
            *value = BCM_EXTENDER_PCP_DE_SELECT_OUTER_TAG;
            break;
        case 1:
            *value = BCM_EXTENDER_PCP_DE_SELECT_INNER_TAG;
            break;
        case 2:
            *value = BCM_EXTENDER_PCP_DE_SELECT_DEFAULT;
            break;
        default:
            return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}
#endif /* INCLUDE_L3 */

int
bcm_tr3_port_pps_rate_egress_set(int unit, bcm_port_t port,
                                uint32 pps, uint32 burst)
{
    uint32 miscval;
    uint64 regval;
    uint32 refresh_rate, bucketsize, granularity = 3, flags = 0;
    int    refresh_bitsize, bucket_bitsize;

    /*
     * The procedure followed is first disable the egress metering i.e.
     * reset EGRMETERINGCONFIG, then clear the buckets and re-enable the
     * egress metering. Finally enable the MISCCONFING,Metering clocking
     *
     * NOTE: During the period of disabling and re-enabling the Egress metering
     * may be in-effective for couple of cycles
     */

    /* Disable egress metering for this port */
    BCM_IF_ERROR_RETURN(READ_EGRMETERINGCONFIG_64r(unit, port, &regval));
    soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, REFRESHf, 0);
    soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, THD_SELf, 0);
    BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIG_64r(unit, port, regval));

    /* Reset metering bucket. */
    BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGBUCKETr(unit, port, 0));

    /* If pps=0, metering is disabled on this egress port*/
    if (pps) {
        /* Enable metering for this port */

        BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
        if (soc_reg_field_get(unit, MISCCONFIGr, miscval, ITU_MODE_SELf)) {
            flags |= _BCM_TD_METER_FLAG_NON_LINEAR;
        }
        flags |= _BCM_TD_METER_FLAG_PACKET_MODE;
        refresh_bitsize = soc_reg_field_length(unit, EGRMETERINGCONFIG_64r, REFRESHf);
        bucket_bitsize = soc_reg_field_length(unit, EGRMETERINGCONFIG_64r, THD_SELf);
        BCM_IF_ERROR_RETURN
            (_bcm_td_rate_to_bucket_encoding(unit, pps, burst, flags,
                               refresh_bitsize, bucket_bitsize,
                               &refresh_rate, &bucketsize, &granularity));

        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, MODEf, 1);
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                                  METER_GRANf, granularity);
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                              REFRESHf, refresh_rate);
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                              THD_SELf, bucketsize);
        BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIG_64r(unit, port, regval));
    }

    return BCM_E_NONE;
}

int
bcm_tr3_port_pps_rate_egress_get(int unit, bcm_port_t port,
                                uint32 *pps, uint32 *burst)
{
    uint64 regval; 
    uint32 miscval;
    uint32 refresh_rate = 0, bucketsize = 0, granularity = 3, flags = 0;

    COMPILER_64_ZERO(regval);

    /* Input parameters check. */
    if (!pps || !burst) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
    if (soc_reg_field_get(unit, MISCCONFIGr, miscval, ITU_MODE_SELf)) {
        flags |= _BCM_TD_METER_FLAG_NON_LINEAR;
    }

    BCM_IF_ERROR_RETURN(READ_EGRMETERINGCONFIG_64r(unit, port, &regval));

    bucketsize = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r,
                                       regval, THD_SELf);
    refresh_rate = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r, 
                                         regval, REFRESHf);
    flags |= _BCM_TD_METER_FLAG_PACKET_MODE;

    granularity = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r,
                                            regval, METER_GRANf);
    BCM_IF_ERROR_RETURN
        (_bcm_td_bucket_encoding_to_rate(unit, refresh_rate, bucketsize,
                                         granularity, flags, pps, burst));

    return (BCM_E_NONE);
}

int
bcm_tr3_port_rate_egress_set(int unit, bcm_port_t port,
                             uint32 kbits_sec, uint32 kbits_burst, uint32 mode)
{
    uint32 miscval;
    uint64 regval;
    uint32 refresh_rate, bucketsize, granularity = 3, flags = 0;
    int    refresh_bitsize, bucket_bitsize;

    if (mode == _BCM_PORT_RATE_PPS_MODE) {
        return bcm_tr3_port_pps_rate_egress_set(
                            unit, port, kbits_sec, kbits_burst);
    }

    /*
     * The procedure followed is first disable the egress metering i.e.
     * reset EGRMETERINGCONFIG, then clear the buckets and re-enable the
     * egress metering. Finally enable the MISCCONFING,Metering clocking
     *
     * NOTE: During the period of disabling and re-enabling the Egress metering
     * may be in-effective for couple of cycles
     */

    /* Disable egress metering for this port */
    BCM_IF_ERROR_RETURN(READ_EGRMETERINGCONFIG_64r(unit, port, &regval));
    soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, REFRESHf, 0);
    soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, THD_SELf, 0);
    BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIG_64r(unit, port, regval));

    /* Reset metering bucket. */
    BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGBUCKETr(unit, port, 0));

    /* If kbits_sec=0, metering is disabled on this egress port*/
    if (kbits_sec) {
        /* Enable metering for this port */

        BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
        if (soc_reg_field_get(unit, MISCCONFIGr, miscval, ITU_MODE_SELf)) {
            flags |= _BCM_TD_METER_FLAG_NON_LINEAR;
        }
        refresh_bitsize = soc_reg_field_length(unit, EGRMETERINGCONFIG_64r,
                                               REFRESHf);
        bucket_bitsize = soc_reg_field_length(unit, EGRMETERINGCONFIG_64r,
                                              THD_SELf);
        BCM_IF_ERROR_RETURN
            (_bcm_td_rate_to_bucket_encoding(unit, kbits_sec, kbits_burst, flags,
                               refresh_bitsize, bucket_bitsize,
                               &refresh_rate, &bucketsize, &granularity));
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, MODEf, 0);
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                                  METER_GRANf, granularity);
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                              REFRESHf, refresh_rate);
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                              THD_SELf, bucketsize);
        BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIG_64r(unit, port, regval));
    }

    return BCM_E_NONE;
}

int
bcm_tr3_port_rate_egress_get(int unit, bcm_port_t port,
                        uint32 *kbits_sec, uint32 *kbits_burst, uint32 *mode)
{
    uint64 regval;                   /* EGRMETERINGCONFIGr register value. */
    uint32 miscval;
    uint32 refresh_rate = 0, bucketsize = 0, granularity = 3, flags = 0;

    COMPILER_64_ZERO(regval);

    /* Input parameters check. */
    if (!kbits_sec || !kbits_burst) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
    if (soc_reg_field_get(unit, MISCCONFIGr, miscval, ITU_MODE_SELf)) {
        flags |= _BCM_TD_METER_FLAG_NON_LINEAR;
    }

    BCM_IF_ERROR_RETURN(READ_EGRMETERINGCONFIG_64r(unit, port, &regval));

    if (1 == soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r, 
                                    regval, MODEf)) {
        *mode = _BCM_PORT_RATE_PPS_MODE;
        return bcm_tr3_port_pps_rate_egress_get(unit, port, 
                                            kbits_sec, kbits_burst);
    }

    bucketsize = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r,
                                       regval, THD_SELf);
    refresh_rate = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r, 
                                         regval, REFRESHf);
    granularity = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r,
                                            regval, METER_GRANf);
    BCM_IF_ERROR_RETURN
        (_bcm_td_bucket_encoding_to_rate(unit, refresh_rate, bucketsize,
                                           granularity, flags,
                                           kbits_sec, kbits_burst));

    return (BCM_E_NONE);
}

#endif /* BCM_TRIUMPH3_SUPPORT */
