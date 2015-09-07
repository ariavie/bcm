/*
 * $Id: port.c,v 1.2 Broadcom SDK $
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
 * File:    port.c
 * Purpose: Port Management
 */

#include <soc/defs.h>
#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/drv.h>
#include <soc/trident2.h>
#include <bcm/port.h>
#include <bcm/error.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/common/lock.h>
#include <bcm_int/esw/trident2.h>
#include <bcm_int/esw_dispatch.h>

int
bcm_td2_port_init(int unit)
{
    bcm_port_t port;
    uint16 dev_id;
    uint8 rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);
    
    if (BCM56855_DEVICE_ID == dev_id) {
        for (port = SOC_TD2_56855_LOOPBACK_HIGIG_PORT_START; 
             port < SOC_TD2_56855_LOOPBACK_HIGIG_PORT_END + 1; port++) {
            SOC_IF_ERROR_RETURN
                (bcm_esw_port_loopback_set(unit, port, BCM_PORT_LOOPBACK_MAC));
        }
    }
    return BCM_E_NONE;
}

int
bcm_td2_port_rate_ingress_set(int unit, bcm_port_t port, uint32 bandwidth,
                              uint32 burst)
{
    int phy_port, mmu_port, index;
    static soc_mem_t config_mem[] = {
        MMU_MTRI_BKPMETERINGCONFIG_MEM_0m,
        MMU_MTRI_BKPMETERINGCONFIG_MEM_1m
    };
    soc_mem_t mem;
    uint32 rval;
    mmu_mtri_bkpmeteringconfig_mem_0_entry_t entry;
    uint32 pause, refresh_rate, bucketsize, granularity, flags;
    int refresh_bitsize, bucket_bitsize;

    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
    mmu_port = SOC_INFO(unit).port_p2m_mapping[phy_port];
    mem = config_mem[mmu_port >> 6];
    index = mmu_port & 0x3f;

    sal_memset(&entry, 0, sizeof(entry));

    /* If metering is disabled on this ingress port we are done. */
    if (!bandwidth || !burst) {
        SOC_IF_ERROR_RETURN
            (soc_mem_write(unit, mem, MEM_BLOCK_ANY, index, &entry));
        return BCM_E_NONE;
    }

    flags = 0;
    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &rval));
    if (soc_reg_field_get(unit, MISCCONFIGr, rval, ITU_MODE_SELf)) {
        flags |= _BCM_TD_METER_FLAG_NON_LINEAR;
    }

    /* Discard threshold will be set to 12.5 % above pause threshold */
    pause = (burst * 8) / 9;
    refresh_bitsize = soc_mem_field_length(unit, mem, REFRESHCOUNTf);
    bucket_bitsize = soc_mem_field_length(unit, mem, PAUSE_THDf);
    BCM_IF_ERROR_RETURN
        (_bcm_td_rate_to_bucket_encoding(unit, bandwidth, pause, flags,
                                         refresh_bitsize, bucket_bitsize,
                                         &refresh_rate, &bucketsize,
                                         &granularity));

    soc_mem_field32_set(unit, mem, &entry, METER_GRANULARITYf, granularity);
    soc_mem_field32_set(unit, mem, &entry, REFRESHCOUNTf, refresh_rate);
    soc_mem_field32_set(unit, mem, &entry, PAUSE_THDf, bucketsize);
    soc_mem_field32_set(unit, mem, &entry, BKPDISCARD_ENf, 1);
    /* Set discard threshold to 12.5 % above pause threshold */
    soc_mem_field32_set(unit, mem, &entry, DISCARD_THDf, 3);
    /* Set resume threshold to 75% above pause threshold */
    /* RESUME_THD field in entry is 0 */

    SOC_IF_ERROR_RETURN
        (soc_mem_write(unit, mem, MEM_BLOCK_ANY, index, &entry));

    return BCM_E_NONE;
}

int
bcm_td2_port_rate_ingress_get(int unit, bcm_port_t port, uint32 *bandwidth,
                              uint32 *burst)
{
    int phy_port, mmu_port, index;
    static soc_mem_t config_mem[] = {
        MMU_MTRI_BKPMETERINGCONFIG_MEM_0m,
        MMU_MTRI_BKPMETERINGCONFIG_MEM_1m
    };
    soc_mem_t mem;
    uint32 rval;
    mmu_mtri_bkpmeteringconfig_mem_0_entry_t entry;
    uint32 pause, refresh_rate, bucketsize, granularity, flags;

    if (bandwidth == NULL || burst == NULL) {
        return BCM_E_PARAM;
    }

    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
    mmu_port = SOC_INFO(unit).port_p2m_mapping[phy_port];
    mem = config_mem[mmu_port >> 6];
    index = mmu_port & 0x3f;

    SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY, index, &entry));

    if (!soc_mem_field32_get(unit, mem, &entry, BKPDISCARD_ENf)) {
        *bandwidth = *burst = 0;
        return BCM_E_NONE;
    }

    refresh_rate = soc_mem_field32_get(unit, mem, &entry, REFRESHCOUNTf);
    bucketsize = soc_mem_field32_get(unit, mem, &entry, PAUSE_THDf);
    granularity = soc_mem_field32_get(unit, mem, &entry, METER_GRANULARITYf);

    flags = 0;
    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &rval));
    if (soc_reg_field_get(unit, MISCCONFIGr, rval, ITU_MODE_SELf)) {
        flags |= _BCM_TD_METER_FLAG_NON_LINEAR;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_td_bucket_encoding_to_rate(unit, refresh_rate, bucketsize,
                                         granularity, flags, bandwidth,
                                         &pause));

    switch (soc_mem_field32_get(unit, mem, &entry, DISCARD_THDf)) {
    case 0:
        *burst = pause * 7 / 4; /* 75% above PAUSE_THD */
        break;
    case 1:
        *burst = pause * 3 / 2; /* 50% above PAUSE_THD */
        break;
    case 2:
        *burst = pause * 5 / 4; /* 25% above PAUSE_THD */
        break;
    case 3:
        *burst = pause * 9 / 8; /* 12.5% above PAUSE_THD */
        break;
    default:
        /* Should never happen */
        *burst = 0;
        break;
    }

    return BCM_E_NONE;
}

int
bcm_td2_port_rate_pause_get(int unit, bcm_port_t port, uint32 *pause,
                            uint32 *resume)
{
    int phy_port, mmu_port, index;
    static soc_mem_t config_mem[] = {
        MMU_MTRI_BKPMETERINGCONFIG_MEM_0m,
        MMU_MTRI_BKPMETERINGCONFIG_MEM_1m
    };
    soc_mem_t mem;
    uint32 rval;
    mmu_mtri_bkpmeteringconfig_mem_0_entry_t entry;
    uint32 burst, bandwidth, bucketsize, granularity, flags;

    if (pause == NULL || resume == NULL) {
        return BCM_E_PARAM;
    }

    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
    mmu_port = SOC_INFO(unit).port_p2m_mapping[phy_port];
    mem = config_mem[mmu_port >> 6];
    index = mmu_port & 0x3f;

    SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY, index, &entry));

    if (!soc_mem_field32_get(unit, mem, &entry, BKPDISCARD_ENf)) {
        *pause = *resume = 0;
        return BCM_E_NONE;
    }

    bucketsize = soc_mem_field32_get(unit, mem, &entry, PAUSE_THDf);
    granularity = soc_mem_field32_get(unit, mem, &entry, METER_GRANULARITYf);

    flags = 0;
    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &rval));
    if (soc_reg_field_get(unit, MISCCONFIGr, rval, ITU_MODE_SELf)) {
        flags |= _BCM_TD_METER_FLAG_NON_LINEAR;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_td_bucket_encoding_to_rate(unit, 0, bucketsize, granularity,
                                         flags, &bandwidth, pause));

    switch (soc_mem_field32_get(unit, mem, &entry, RESUME_THDf)) {
    case 0:
        *resume = *pause * 3 / 4; /* 75% of PAUSE_THD */
        break;
    case 1:
        *resume = *pause * 1 / 2; /* 50% of PAUSE_THD */
        break;
    case 2:
        *resume = *pause * 1 / 4; /* 25% of PAUSE_THD */
        break;
    case 3:
        *resume = *pause * 1 / 8; /* 12.5% of PAUSE_THD */
        break;
    default:
        /* Should never happen */
        *resume = 0;
        break;
    }

    switch (soc_mem_field32_get(unit, mem, &entry, DISCARD_THDf)) {
    case 0:
        burst = *pause * 7 / 4; /* 75% above PAUSE_THD */
        break;
    case 1:
        burst = *pause * 3 / 2; /* 50% above PAUSE_THD */
        break;
    case 2:
        burst = *pause * 5 / 4; /* 25% above PAUSE_THD */
        break;
    case 3:
        burst = *pause * 9 / 8; /* 12.5% above PAUSE_THD */
        break;
    default:
        /* Should never happen */
        burst = 0;
        break;
    }

    *pause = burst - *pause;
    *resume = burst - *resume;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_port_rate_egress_get
 * Purpose:
 *      Get egress metering information
 * Parameters:
 *      unit       - (IN) Unit number
 *      port       - (IN) Port number
 *      bandwidth  - (IN) Kilobits per second or packets per second
 *                        zero if rate limiting is disabled
 *      burst      - (IN) Maximum burst size in kilobits or packets
 *      mode       - (IN) _BCM_PORT_RATE_BYTE_MODE or _BCM_PORT_RATE_PPS_MODE
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_port_rate_egress_set(int unit, bcm_port_t port, uint32 bandwidth,
                             uint32 burst, uint32 mode)
{
    int phy_port, mmu_port, index;
    static soc_mem_t config_mem[] = {
        MMU_MTRO_EGRMETERINGCONFIG_MEM_0m,
        MMU_MTRO_EGRMETERINGCONFIG_MEM_1m
    };
    soc_mem_t mem;
    uint32 rval;
    mmu_mtro_egrmeteringconfig_mem_0_entry_t entry;
    uint32 refresh_rate, bucketsize, granularity, flags;
    int refresh_bitsize, bucket_bitsize;

    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
    mmu_port = SOC_INFO(unit).port_p2m_mapping[phy_port];
    mem = config_mem[mmu_port >> 6];
    index = mmu_port & 0x3f;

    sal_memset(&entry, 0, sizeof(entry));

    /* If metering is disabled on this ingress port we are done. */
    if (!bandwidth || !burst) {
        SOC_IF_ERROR_RETURN
            (soc_mem_write(unit, mem, MEM_BLOCK_ANY, index, &entry));
        return BCM_E_NONE;
    }

    flags = mode == _BCM_PORT_RATE_PPS_MODE ?
        _BCM_TD_METER_FLAG_PACKET_MODE : 0;
    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &rval));
    if (soc_reg_field_get(unit, MISCCONFIGr, rval, ITU_MODE_SELf)) {
        flags |= _BCM_TD_METER_FLAG_NON_LINEAR;
    }

    refresh_bitsize = soc_mem_field_length(unit, mem, REFRESHf);
    bucket_bitsize = soc_mem_field_length(unit, mem, THD_SELf);
    BCM_IF_ERROR_RETURN
        (_bcm_td_rate_to_bucket_encoding(unit, bandwidth, burst, flags,
                                         refresh_bitsize, bucket_bitsize,
                                         &refresh_rate, &bucketsize,
                                         &granularity));
    soc_mem_field32_set(unit, mem, &entry, MODEf,
                        mode == _BCM_PORT_RATE_PPS_MODE ? 1 : 0);
    soc_mem_field32_set(unit, mem, &entry, METER_GRANf, granularity);
    soc_mem_field32_set(unit, mem, &entry, REFRESHf, refresh_rate);
    soc_mem_field32_set(unit, mem, &entry, THD_SELf, bucketsize);

    SOC_IF_ERROR_RETURN
        (soc_mem_write(unit, mem, MEM_BLOCK_ANY, index, &entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_port_rate_egress_get
 * Purpose:
 *      Get egress metering information
 * Parameters:
 *      unit       - (IN) Unit number
 *      port       - (IN) Port number
 *      bandwidth  - (OUT) Kilobits per second or packets per second
 *                         zero if rate limiting is disabled
 *      burst      - (OUT) Maximum burst size in kilobits or packets
 *      mode       - (OUT) _BCM_PORT_RATE_BYTE_MODE or _BCM_PORT_RATE_PPS_MODE
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_port_rate_egress_get(int unit, bcm_port_t port, uint32 *bandwidth,
                             uint32 *burst, uint32 *mode)
{
    int phy_port, mmu_port, index;
    static soc_mem_t config_mem[] = {
        MMU_MTRO_EGRMETERINGCONFIG_MEM_0m,
        MMU_MTRO_EGRMETERINGCONFIG_MEM_1m
    };
    soc_mem_t mem;
    uint32 rval;
    mmu_mtri_bkpmeteringconfig_mem_0_entry_t entry;
    uint32 refresh_rate, bucketsize, granularity, flags;

    if (bandwidth == NULL || burst == NULL) {
        return BCM_E_PARAM;
    }

    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
    mmu_port = SOC_INFO(unit).port_p2m_mapping[phy_port];
    mem = config_mem[mmu_port >> 6];
    index = mmu_port & 0x3f;

    SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY, index, &entry));

    refresh_rate = soc_mem_field32_get(unit, mem, &entry, REFRESHf);
    bucketsize = soc_mem_field32_get(unit, mem, &entry, THD_SELf);
    granularity = soc_mem_field32_get(unit, mem, &entry, METER_GRANf);

    flags = soc_mem_field32_get(unit, mem, &entry, MODEf) ?
        _BCM_TD_METER_FLAG_PACKET_MODE : 0;
    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &rval));
    if (soc_reg_field_get(unit, MISCCONFIGr, rval, ITU_MODE_SELf)) {
        flags |= _BCM_TD_METER_FLAG_NON_LINEAR;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_td_bucket_encoding_to_rate(unit, refresh_rate, bucketsize,
                                         granularity, flags, bandwidth,
                                         burst));
    *mode = flags & _BCM_TD_METER_FLAG_PACKET_MODE ?
        _BCM_PORT_RATE_PPS_MODE : _BCM_PORT_RATE_BYTE_MODE;

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_td2_port_drain_cells
 * Purpose:
 *     To drain cells associated to the port.
 * Parameters:
 *     unit       - (IN) unit number
 *     port       - (IN) Port
 * Returns:
 *     BCM_E_XXX
 */
int _bcm_td2_port_drain_cells(int unit, int port)
{
    mac_driver_t *macd;
    int rv = BCM_E_NONE;

    PORT_LOCK(unit);
    rv = soc_mac_probe(unit, port, &macd);

    if (BCM_SUCCESS(rv)) {
        rv = MAC_CONTROL_SET(macd, unit, port, SOC_MAC_CONTROL_EGRESS_DRAIN, 1);

    }
    PORT_UNLOCK(unit);
    return rv;
}

int bcm_td2_port_drain_cells(int unit, int port)
{
    BCM_IF_ERROR_RETURN
        (_bcm_td2_port_drain_cells(unit, port));

    BCM_IF_ERROR_RETURN
        (_bcm_td2_cosq_port_queue_state_check(unit, port));

    return BCM_E_NONE;
}

/* Port table field programming - assumes PORT_LOCK is taken */
int
_bcm_td2_egr_port_set(int unit, bcm_port_t port, 
                      soc_field_t field, int value)

{
    egr_port_entry_t pent;
    soc_mem_t mem = EGR_PORTm;
    int rv, cur_val;

    if (!SOC_MEM_FIELD_VALID(unit, mem, field)) {
        return (BCM_E_UNAVAIL);
    }
    sal_memset(&pent, 0, sizeof(egr_port_entry_t));
    rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, port, &pent);

    if (BCM_SUCCESS(rv)) {
        cur_val = soc_EGR_PORTm_field32_get(unit, &pent, field);
        if (value != cur_val) {
            soc_EGR_PORTm_field32_set(unit, &pent, field, value);
            rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, port, &pent);
        }
    }
    return rv;
}

#else /* BCM_TRIDENT2_SUPPORT */
int _td2_port_not_empty;
#endif  /* BCM_TRIDENT2_SUPPORT */
