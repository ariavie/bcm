/*
 * $Id: stat.c,v 1.192 Broadcom SDK $
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
 * Broadcom StrataSwitch SNMP Statistics API.
 */

#include <sal/types.h>
#include <shared/bsl.h>
#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/counter.h>
#include <soc/ll.h>

#include <bcm/stat.h>
#include <bcm/error.h>

#include <bcm_int/esw/stat.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw/flex_ctr.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/triumph2.h>
#ifdef BCM_KATANA2_SUPPORT
#include <bcm_int/esw/katana2.h>
#endif

#ifndef PLISIM
#define COUNTER_FLAGS_DEFAULT	SOC_COUNTER_F_DMA
#else
#define COUNTER_FLAGS_DEFAULT	0
#endif

#define BCMSIM_STAT_INTERVAL 25000000
#define OBJ_TYPE_NUM_ENTRIES   (sizeof(obj2type)/sizeof(obj2type[0]))

#if  defined(BCM_BRADLEY_SUPPORT)  ||  \
     defined(BCM_TRIUMPH_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
#define  _BCM_HB_GW_TRX_BIT_SET(unit, result, bit) \
        do {                                    \
           if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit)) { \
               *result =  bit;                  \
               return BCM_E_NONE;               \
           }                                    \
        } while ((0))
#else
#define  _BCM_HB_GW_TRX_BIT_SET(unit, result, bit) 
#endif /* BRADLEY || TRIUMPH || SCORPION */

#if defined(BCM_RAPTOR_SUPPORT)
#define _BCM_RAPTOR_RAVEN_BIT_SET(unit, result, bit)          \
       do {                                                   \
          if (SOC_IS_RAPTOR(unit) || SOC_IS_RAVEN(unit)) {    \
              *result = (bit);                                \
              return BCM_E_NONE;                              \
          }                                                   \
       } while ((0))
#else
#define _BCM_RAPTOR_RAVEN_BIT_SET(unit, result, bit)
#endif /* RAPTOR */

#if defined(BCM_RAVEN_SUPPORT)
#define _BCM_RAVEN_BIT_SET(unit, result, bit)    \
       do {                                      \
          if (SOC_IS_RAVEN(unit)) {              \
              *result = (bit);                   \
              return BCM_E_NONE;                 \
          }                                      \
       } while ((0))
#else
#define _BCM_RAVEN_BIT_SET(unit, result, bit)
#endif /* RAVEN */

#if  defined(BCM_BRADLEY_SUPPORT)
#define _BCM_HB_GW_BIT_SET_E_PARAM(unit, result) \
       do {                                      \
          if (SOC_IS_HB_GW(unit)) {              \
              return BCM_E_PARAM;                \
          }                                      \
        } while ((0))
#else
#define _BCM_HB_GW_BIT_SET_E_PARAM(unit, result)             
#endif

#if  defined(BCM_TRIUMPH_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
#define _BCM_TRX_BIT_SET(unit, result, bit)     \
        do {                                    \
           if (SOC_IS_TRX(unit)) {              \
               *result =  bit;                  \
               return BCM_E_NONE;               \
           }                                    \
        } while ((0))

#define _BCM_TRX_BIT_SET_E_PARAM(unit, result)                \
       do {                                                   \
          if (SOC_IS_TRX(unit)) {                             \
              return BCM_E_PARAM;                             \
          }                                                   \
       } while ((0))
#else
#define _BCM_TRX_BIT_SET(unit, result, bit) 
#define _BCM_TRX_BIT_SET_E_PARAM(unit, result)
#endif /* TRIUMPH || SCORPION */  

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_BRADLEY_SUPPORT) || \
    defined(BCM_TRIUMPH_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
#define  _BCM_FB_HB_GW_TRX_BIT_SET(unit, result, bit) \
        do {                                    \
           if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit) || SOC_IS_FB(unit)) { \
               *result =  bit;                  \
               return BCM_E_NONE;               \
           }                                    \
        } while ((0))
#else
#define  _BCM_FB_HB_GW_TRX_BIT_SET(unit, result, bit) 
#endif /* BRADLEY || TRIUMPH || SCORPION || FIREBOLT */

#if defined(BCM_FIREBOLT_SUPPORT)                               
#define _BCM_FB_BIT_SET(unit, result, bit)                      \
       do {                                                     \
          if (SOC_IS_FB(unit)) {                                \
              *result = (bit);                                  \
              return BCM_E_NONE;                                \
          }                                                     \
       } while ((0)) 
#else
#define _BCM_FB_BIT_SET(unit, result, bit)                     
#endif /* FIREBOLT */

#if defined(BCM_FIREBOLT2_SUPPORT)                               
#define _BCM_FB2_BIT_SET(unit, result, bit)                     \
       do {                                                     \
          if (SOC_IS_FIREBOLT2(unit)) {                         \
              *result = (bit);                                  \
              return BCM_E_NONE;                                \
          }                                                     \
       } while ((0)) 
#else
#define _BCM_FB2_BIT_SET(unit, result, bit)                     
#endif /* FIREBOLT2 */

#define _BCM_FBX_BIT_SET(unit, result, bit)                     \
       do {                                                     \
          if (SOC_IS_FBX(unit)) {                               \
              *result = (bit);                                  \
              return BCM_E_NONE;                                \
          }                                                     \
       } while ((0))                                            

#if defined(BCM_TRIDENT2_SUPPORT)                               
#define _BCM_TD2_BIT_SET(unit, result, bit)                     \
       do {                                                     \
          if (SOC_IS_TD2_TT2(unit)) {                           \
              *result = (bit);                                  \
              return BCM_E_NONE;                                \
          }                                                     \
       } while ((0)) 
#else
#define _BCM_TD2_BIT_SET(unit, result, bit)                     
#endif /* TRIDENT2 */

#define _BCM_DEFAULT_BIT_SET(unit, result, bit)                 \
       do {                                                     \
          *result = (bit);                                      \
          return BCM_E_NONE;                                    \
       } while ((0))

#define _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result)              \
       do {                                                     \
          return BCM_E_PARAM;                                   \
       } while ((0))

#define _DBG_CNT_TX_CHAN        1
#define _DBG_CNT_RX_CHAN        2

#ifdef BCM_XGS3_SWITCH_SUPPORT
/*
 * All drop conditions
 */
#define BCM_DBG_CNT_DROP       (BCM_DBG_CNT_RPORTD | \
                                BCM_DBG_CNT_RIPD4  | \
                                BCM_DBG_CNT_RIPD6  | \
                                BCM_DBG_CNT_PDISC  | \
                                BCM_DBG_CNT_RFILDR | \
                                BCM_DBG_CNT_RDISC  | \
                                BCM_DBG_CNT_RDROP  | \
                                BCM_DBG_CNT_VLANDR)
#endif /* BCM_XGS3_SWITCH_SUPPORT */

/* Oversize packet size threshold (soc_property bcm_stat_jumbo[.unit]=n) */
int **_bcm_stat_ovr_threshold;

/* Port control to include/exclude oversize packet errors */
SHR_BITDCL *_bcm_stat_ovr_control;

STATIC _bcm_stat_extra_counter_t **_bcm_stat_extra_counters;

#define _BCM_STAT_OVR_CONTROL_GET(_u_, _port_) \
        SHR_BITGET(&_bcm_stat_ovr_control[(_u_)], (_port_))
#define _BCM_STAT_OVR_CONTROL_SET(_u_, _port_) \
        SHR_BITSET(&_bcm_stat_ovr_control[(_u_)], (_port_))
#define _BCM_STAT_OVR_CONTROL_CLR(_u_, _port_) \
        SHR_BITCLR(&_bcm_stat_ovr_control[(_u_)], (_port_))

void
_bcm_stat_counter_extra_callback(int unit)
{
    soc_reg_t reg;
    uint32 val, diff;
    int rv, i, width;
    _bcm_stat_extra_counter_t *ctr;
    soc_port_t port, port_start, port_end;
    int index;

    for (i = 0; i < _BCM_STAT_EXTRA_COUNTER_COUNT; i++) {
        ctr = &_bcm_stat_extra_counters[unit][i];
        reg = ctr->reg;
        if (reg == INVALIDr) {
            continue;
        }
        if (SOC_REG_INFO(unit, reg).regtype == soc_portreg) {
            port_start = 0;
            port_end = MAX_PORT(unit) - 1;
        } else {
            port_start = port_end = REG_PORT_ANY;
        }
        for (port = port_start; port <= port_end; port++) {
            if (port == REG_PORT_ANY) {
                index = 0;
            } else {
                if (!SOC_PORT_VALID(unit, port)) {
                    continue;
                }
                index = port;
            }
            if (SOC_REG_IS_64(unit, reg)) {
                continue; /* no such case yet */
            } else {
                rv = soc_reg32_get(unit, reg, port, 0, &val);
                if (SOC_FAILURE(rv)) {
                    continue;
                }
                diff = val - ctr->ctr_prev[index];
                /* assume first field is the counter field itself */
                width = SOC_REG_INFO(unit, reg).fields[0].len;
                if (width < 32) {
                    diff &= (1 << width) - 1;
                }
                if (!diff) {
                    continue;
                }
                COMPILER_64_ADD_32(ctr->count64[index], diff);
                ctr->ctr_prev[index] = val;
            }
        }
    }
}

int
_bcm_stat_counter_extra_get(int unit, soc_reg_t reg, soc_port_t port,
                            uint64 *val)
{
    int i;

    for (i = 0; i < _BCM_STAT_EXTRA_COUNTER_COUNT; i++) {
        if(_bcm_stat_extra_counters[unit][i].reg == reg) {
            if (port == REG_PORT_ANY) {
                *val = _bcm_stat_extra_counters[unit][i].count64[0];
            } else {
                *val = _bcm_stat_extra_counters[unit][i].count64[port];
            }
            return BCM_E_NONE;
        }
    }

    COMPILER_64_ZERO(*val);
    return BCM_E_NONE;
}

int
_bcm_stat_counter_non_dma_extra_get(int unit,
                                    soc_counter_non_dma_id_t non_dma_id,
                                    soc_port_t port,
                                    uint64 *val)
{
    COMPILER_64_ZERO(*val);

#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_IS_TR_VL(unit) && !SOC_IS_HURRICANEX(unit) 
         && !SOC_IS_ENDURO(unit)) {
        uint64 val64;
        int ix, rv;
        int max = 1;

        switch (non_dma_id) {
        case SOC_COUNTER_NON_DMA_COSQ_DROP_PKT:
            max = 48;
            break;
        case SOC_COUNTER_NON_DMA_PORT_DROP_PKT_ING:
            max = 1;
            break;
        default:
            /* Shouldn't hit this because it's internal */
            return BCM_E_PARAM;
        }

        for (ix = 0; ix < max; ix++) {
            rv = soc_counter_get(unit, port, non_dma_id, ix, &val64);
            if (SOC_SUCCESS(rv)) {
                COMPILER_64_ADD_64(*val, val64);
            } else {
                /* Ran past the maximum index for this port. */
                break;
            }
        }
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    return BCM_E_NONE;
}

/*

 * Function:
 *	_bcm_esw_stat_detach
 * Description:
 *	De-initializes the BCM stat module.
 * Parameters:
 *	unit -  (IN) BCM device number.
 * Returns:
 *	BCM_E_XXX
 */

int
_bcm_esw_stat_detach(int unit)
{
    _bcm_stat_extra_counter_t *ctr;
    int i, free_units;

    if (NULL != _bcm_stat_extra_counters) {
        if (NULL != _bcm_stat_extra_counters[unit]) {
            soc_counter_extra_unregister(unit,
                                         _bcm_stat_counter_extra_callback);

            for (i = 0; i < _BCM_STAT_EXTRA_COUNTER_COUNT; i++) {
                ctr = &_bcm_stat_extra_counters[unit][i];
                if (ctr->count64 != NULL) {
                    sal_free(ctr->count64);
                    ctr->count64 = NULL;
                }
                if (ctr->ctr_prev != NULL) {
                    sal_free(ctr->ctr_prev);
                    ctr->ctr_prev = NULL;
                }
            }
            sal_free(_bcm_stat_extra_counters[unit]);
            _bcm_stat_extra_counters[unit] = NULL;
        }

        free_units = TRUE;
        for (i = 0; i < BCM_MAX_NUM_UNITS; i++) {
            if (((i != unit) && SOC_UNIT_VALID(i)) || 
               (NULL != _bcm_stat_extra_counters[i])) {
                free_units = FALSE;
                break;
            }
        }
        if (free_units) {
            sal_free(_bcm_stat_extra_counters);
            _bcm_stat_extra_counters = NULL;
        }
    }

    if (NULL != _bcm_stat_ovr_threshold) {
        if (NULL != _bcm_stat_ovr_threshold[unit]) {
            sal_free(_bcm_stat_ovr_threshold[unit]);
            _bcm_stat_ovr_threshold[unit] = NULL;
        }

        free_units = TRUE;
        for (i = 0; i < BCM_MAX_NUM_UNITS; i++) {
            if( ((i != unit) && SOC_UNIT_VALID(i)) || 
                (NULL != _bcm_stat_ovr_threshold[i])) {
                free_units = FALSE;
                break;
            }
        }
        if (free_units) {
            sal_free(_bcm_stat_ovr_threshold);
            _bcm_stat_ovr_threshold = NULL;
        }
    }

    /* Delete oversize control */
    if (NULL != _bcm_stat_ovr_control) {
        SHR_BITCLR_RANGE(&_bcm_stat_ovr_control[unit], 0,
                    SHR_BITALLOCSIZE(SOC_MAX_NUM_PORTS));

        free_units = TRUE;
        for (i = 0; i < BCM_MAX_NUM_UNITS; i++) {
            if( ((i != unit) && SOC_UNIT_VALID(i)) || 
                !SHR_BITNULL_RANGE(&_bcm_stat_ovr_control[i], 0,
                      SHR_BITALLOCSIZE(SOC_MAX_NUM_PORTS))) {
                free_units = FALSE;
                break;
            }
        }

        if (free_units) {
            sal_free(_bcm_stat_ovr_control);
            _bcm_stat_ovr_control = NULL;
        }
    }

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_cleanup(unit));
    }
#endif
    if (soc_feature(unit, soc_feature_linkphy_coe)) {
#if defined(BCM_KATANA2_SUPPORT)
        BCM_IF_ERROR_RETURN(bcm_kt2_subport_counter_cleanup(unit));
#endif
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_esw_stat_gport_validate
 * Description:
 *      Helper funtion to validate port/gport parameter 
 * Parameters:
 *      unit  - (IN) BCM device number
 *      port_in  - (IN) Port / Gport to validate
 *      port_out - (OUT) Port number if valid. 
 * Return Value:
 *      BCM_E_NONE - Port OK 
 *      BCM_E_INIT - Not initialized
 *      BCM_E_PORT - Port Invalid
 */

STATIC int 
_bcm_esw_stat_gport_validate(int unit, bcm_port_t port_in, bcm_port_t *port_out)
{
    if (BCM_GPORT_IS_SET(port_in)) {
        BCM_IF_ERROR_RETURN(
            bcm_esw_port_local_get(unit, port_in, port_out));
    } else if (SOC_PORT_VALID(unit, port_in)) { 
        *port_out = port_in;
    } else {
        return BCM_E_PORT; 
    }

    return BCM_E_NONE;
}

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_3                SOC_SCACHE_VERSION(1,3)
#define BCM_WB_VERSION_1_2                SOC_SCACHE_VERSION(1,2)
#define BCM_WB_VERSION_1_1                SOC_SCACHE_VERSION(1,1)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_3

STATIC int _bcm_esw_stat_warm_boot_alloc(int unit);

static uint16  stat_warmboot_version=0;


/*
 * Function:
 *      _bcm_esw_stat_warm_boot_recover
 * Purpose:
 *      Recover stat overflow flag states from scache
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_stat_warm_boot_recover(int unit)
{
    uint8 *stat_scache_ptr;
    soc_scache_handle_t scache_handle;
    int stable_size = 0;
    int rv = BCM_E_NONE;
    int alloc_sz = BCM_MAX_NUM_UNITS * SHR_BITALLOCSIZE(SOC_MAX_NUM_PORTS);

    
    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
    if (stable_size > 0) {

        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_STAT, 
                              _BCM_STAT_WARM_BOOT_CHUNK_PORTS);
        rv = _bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                     alloc_sz, &stat_scache_ptr, 
                                     BCM_WB_DEFAULT_VERSION, 
                                     &stat_warmboot_version);

        if (BCM_E_NOT_FOUND == rv) {
            return _bcm_esw_stat_warm_boot_alloc(unit);
        } else if (BCM_FAILURE(rv)) {
            return rv;
        }
        if (NULL != stat_scache_ptr) {
            sal_memcpy(_bcm_stat_ovr_control, stat_scache_ptr, alloc_sz);
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_esw_stat_warm_boot_sync
 * Purpose:
 *      Sync STAT overflow flags states to scache
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_stat_warm_boot_sync(int unit)
{
    uint8 *stat_scache_ptr;
    soc_scache_handle_t scache_handle;
    int stable_size = 0;
    int rv = BCM_E_NONE;
    int alloc_sz = BCM_MAX_NUM_UNITS * SHR_BITALLOCSIZE(SOC_MAX_NUM_PORTS);
    
    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
    if (stable_size > 0) {

        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_STAT, 
                              _BCM_STAT_WARM_BOOT_CHUNK_PORTS);

        /* Get scache stat module data pointer */
        rv = _bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                     alloc_sz, &stat_scache_ptr, 
                                     BCM_WB_DEFAULT_VERSION, NULL);

        if (BCM_SUCCESS(rv) && (NULL != stat_scache_ptr) && 
            (NULL !=_bcm_stat_ovr_control)) {
            /* Copy stat overflow control flags to scache */
            sal_memcpy(stat_scache_ptr, _bcm_stat_ovr_control, alloc_sz);
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_esw_stat_warm_boot_alloc
 * Purpose:
 *      Allocate persistent info memory for STAT module - Level 2 Warm Boot
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_stat_warm_boot_alloc(int unit)
{
    int rv;
    uint8 *stat_scache_ptr;
    soc_scache_handle_t scache_handle;
    int alloc_sz = BCM_MAX_NUM_UNITS * SHR_BITALLOCSIZE(SOC_MAX_NUM_PORTS);
    
    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_STAT, 
                          _BCM_STAT_WARM_BOOT_CHUNK_PORTS);

    /* Create scahe pointer */
    rv = _bcm_esw_scache_ptr_get(unit, scache_handle, TRUE,
                                 alloc_sz, &stat_scache_ptr, 
                                 BCM_WB_DEFAULT_VERSION, NULL);

    if (BCM_E_NOT_FOUND == rv) {
        /* Proceed with Level 1 Warm Boot */
        rv = BCM_E_NONE;
    }

    return rv;
}
int
_bcm_esw_stat_sync_version_above_equal(int unit,uint16 version)
{
   if (stat_warmboot_version >= version) {
       return TRUE;
   } 
   return FALSE;
}

/*
 * Function:
 *      _bcm_esw_stat_sync
 * Purpose:
 *      Record Stat module persisitent info for Level 2 Warm Boot
 * Parameters:
 *      unit  - (IN) BCM device number
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_esw_stat_sync(int unit)
{
#if defined(BCM_WARM_BOOT_SUPPORT) && (defined(BCM_TRIUMPH3_SUPPORT) || \
defined(BCM_KATANA_SUPPORT))
    _bcm_esw_stat_warm_boot_sync(unit);
#endif
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_sync(unit));
    }
#endif
    return BCM_E_NONE;
}
/*
 * Function:
 *	_bcm_stat_reload (internal)
 * Description:
 *      Reload the STAT structures from the current hardware configuration.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_stat_reload(int unit)
{

    if ((_bcm_stat_ovr_threshold == NULL) || 
        (_bcm_stat_ovr_threshold[unit] == NULL)) {
        /* These were just allocated! */
        return BCM_E_INTERNAL;
    }

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        uint32 value = 0;
        uint64 value64;
        bcm_port_t port;

        PBMP_PORT_ITER(unit, port) {
#ifdef BCM_TRIDENT2_SUPPORT
            if (SOC_IS_TD2_TT2(unit)) {
                SOC_IF_ERROR_RETURN
                    (READ_PGW_CNTMAXSIZEr(unit, port, &value));
                _bcm_stat_ovr_threshold[unit][port] = value;
                continue;
            }
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_HURRICANE2_SUPPORT) || defined(BCM_GREYHOUND_SUPPORT)
            if (SOC_IS_HURRICANE2(unit) || SOC_IS_GREYHOUND(unit)) {
                if (IS_XL_PORT(unit,port)) {
                    SOC_IF_ERROR_RETURN
                        (READ_XLPORT_CNTMAXSIZEr(unit, port, &value));
                } else {
                    SOC_IF_ERROR_RETURN
                        (READ_GPORT_CNTMAXSIZEr(unit, port, &value));
                }
                _bcm_stat_ovr_threshold[unit][port] = value;
                continue;
            }
#endif /* BCM_HURRICANE2_SUPPORT || BCM_GREYHOUND_SUPPORT */
            if (soc_feature(unit, soc_feature_unified_port)) {
                SOC_IF_ERROR_RETURN
                    (READ_PORT_CNTMAXSIZEr(unit, port, &value));
#if defined(BCM_KATANA2_SUPPORT)
            } else if (SOC_IS_KATANA2(unit) && IS_MXQ_PORT(unit,port)) {
                SOC_IF_ERROR_RETURN
                        (READ_X_GPORT_CNTMAXSIZEr(unit, port, &value));
#endif
            } else if (IS_GE_PORT(unit,port) || IS_FE_PORT(unit,port) ||
                IS_XL_PORT(unit, port) || IS_MXQ_PORT(unit,port)) {
                SOC_IF_ERROR_RETURN
                    (READ_GPORT_CNTMAXSIZEr(unit, port, &value));
            } else {
                SOC_IF_ERROR_RETURN
                    (READ_MAC_CNTMAXSZr(unit, port, &value64));
                COMPILER_64_TO_32_LO(value, value64);
            }

            _bcm_stat_ovr_threshold[unit][port] = value;
        }
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA_SUPPORT)
        /* Recover WB stat overflow flags from scahe */
        BCM_IF_ERROR_RETURN(_bcm_esw_stat_warm_boot_recover(unit));
#endif /* BCM_TRIUMPH3_SUPPORT */
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *	bcm_stat_init
 * Description:
 *	Initializes the BCM stat module.
 * Parameters:
 *	unit - StrataSwitch PCI device unit number (driver internal).
 * Returns:
 *	BCM_E_NONE - Success.
 *	BCM_E_INTERNAL - Chip access failure.
 */

int
bcm_esw_stat_init(int unit)
{
    pbmp_t		pbmp;
    sal_usecs_t		interval;
    uint32		flags;
    int                 config_threshold;
    int                 i;
    _bcm_stat_extra_counter_t *ctr;
    int alloc_size;
    int free_global_arr = FALSE;
#if defined (BCM_KATANA2_SUPPORT)
    soc_port_t          port; 
#endif


    if (soc_property_get_str(unit, spn_BCM_STAT_PBMP) == NULL) {
#if defined (BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            SOC_PBMP_CLEAR(pbmp); 
            for (port =  SOC_INFO(unit).cmic_port;
                 port <= SOC_INFO(unit).lb_port;
                 port++) {
                 if (SOC_INFO(unit).block_valid[SOC_PORT_BLOCK(unit,port)]) {
                     LOG_VERBOSE(BSL_LS_APPL_SHELL,
                                 (BSL_META_U(unit,
                                             "BlockName:%s \n"),
                                             SOC_BLOCK_NAME(unit, SOC_PORT_BLOCK(unit,port))));
                     SOC_PBMP_PORT_ADD(pbmp, port);
                 }
            }
        } else 
#endif
        {
            SOC_PBMP_ASSIGN(pbmp, PBMP_PORT_ALL(unit));
            /*
             * Collect stats on CPU port as well if supported
             */
            if (soc_feature(unit, soc_feature_cpuport_stat_dma)) {
                SOC_PBMP_PORT_ADD(pbmp, CMIC_PORT(unit));
            }
        }
    } else {
#if defined (BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            LOG_CLI((BSL_META_U(unit,
                                "After flex-io operation. You might need "
                                "to add new ports for counter collection manually\n")));
        }
#endif
        pbmp = soc_property_get_pbmp(unit, spn_BCM_STAT_PBMP, 0);
    }

    interval = (SAL_BOOT_BCMSIM) ? BCMSIM_STAT_INTERVAL : 1000000;
    interval = soc_property_get(unit, spn_BCM_STAT_INTERVAL, interval);
    flags =
        soc_property_get(unit, spn_BCM_STAT_FLAGS, COUNTER_FLAGS_DEFAULT);

    if (SOC_IS_RCPU_ONLY(unit)) {
        flags &= ~SOC_COUNTER_F_DMA;
    }

    if (NULL == _bcm_stat_ovr_threshold) {
        alloc_size = BCM_MAX_NUM_UNITS * sizeof(int *);
        _bcm_stat_ovr_threshold =
            sal_alloc(alloc_size, "device oversize packet thresholds");
        if (NULL == _bcm_stat_ovr_threshold) {
            return (BCM_E_MEMORY);
        }
        sal_memset(_bcm_stat_ovr_threshold, 0, alloc_size);
    }
    alloc_size = SOC_MAX_NUM_PORTS * sizeof(int);
    if (NULL == _bcm_stat_ovr_threshold[unit]) {
        _bcm_stat_ovr_threshold[unit] =
            sal_alloc(alloc_size, "device per-port oversize packet thresholds");
        if (NULL == _bcm_stat_ovr_threshold[unit]) {
            if (free_global_arr) {
                sal_free(_bcm_stat_ovr_threshold);
                _bcm_stat_ovr_threshold = NULL;
            }
            return (BCM_E_MEMORY);
        }
    }
    sal_memset(_bcm_stat_ovr_threshold[unit], 0, alloc_size);
    config_threshold = soc_property_get(unit, spn_BCM_STAT_JUMBO, 1518);
    if ((config_threshold < 1518) || 
        (config_threshold > 0x3fff) ) {
        config_threshold = 1518;
    }

    for (i = 0; i < SOC_MAX_NUM_PORTS; i++) {
        _bcm_stat_ovr_threshold[unit][i] = config_threshold;
    }

    /* Initialize Oversize packet control bitmap */
    if (NULL == _bcm_stat_ovr_control) {
        alloc_size = BCM_MAX_NUM_UNITS * SHR_BITALLOCSIZE(SOC_MAX_NUM_PORTS);
        _bcm_stat_ovr_control =
            sal_alloc(alloc_size, "oversize packet error control");
        if (NULL == _bcm_stat_ovr_control) {
            return (BCM_E_MEMORY);
        }
        sal_memset(_bcm_stat_ovr_control, 0, alloc_size);
    }

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_stat_reload(unit));
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        bcm_port_t   port;
        uint64 rval;
        uint32 rval32;
        soc_info_t *si = &SOC_INFO(unit);

        SOC_IF_ERROR_RETURN
            (bcm_esw_stat_custom_set(unit, -1, snmpBcmCustomReceive0, 
                                     BCM_DBG_CNT_DROP));
        SOC_IF_ERROR_RETURN
            (bcm_esw_stat_custom_set(unit, -1, snmpBcmCustomReceive1, 
                                     BCM_DBG_CNT_IMBP));
        SOC_IF_ERROR_RETURN
            (bcm_esw_stat_custom_set(unit, -1, snmpBcmCustomReceive2, 
                                     BCM_DBG_CNT_RIMDR));
        SOC_IF_ERROR_RETURN
            (bcm_esw_stat_custom_set(unit, -1, snmpBcmCustomTransmit0, 
                                     BCM_DBG_CNT_TGIPMC6 | BCM_DBG_CNT_TGIP6));
        SOC_IF_ERROR_RETURN
            (bcm_esw_stat_custom_set(unit, -1, snmpBcmCustomTransmit1, 
                                     BCM_DBG_CNT_TIPMCD6 | BCM_DBG_CNT_TIPD6));
        SOC_IF_ERROR_RETURN
            (bcm_esw_stat_custom_set(unit, -1, snmpBcmCustomTransmit2, 
                                     BCM_DBG_CNT_TGIPMC6));
        SOC_IF_ERROR_RETURN
            (bcm_esw_stat_custom_set(unit, -1, snmpBcmCustomTransmit3, 
                                     BCM_DBG_CNT_TPKTD));
        SOC_IF_ERROR_RETURN
            (bcm_esw_stat_custom_set(unit, -1, snmpBcmCustomTransmit4, 
                                     BCM_DBG_CNT_TGIP4 | BCM_DBG_CNT_TGIP6));
        SOC_IF_ERROR_RETURN
            (bcm_esw_stat_custom_set(unit, -1, snmpBcmCustomTransmit5, 
                                     BCM_DBG_CNT_TIPMCD4 | BCM_DBG_CNT_TIPMCD6));

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_fcoe)) {
            SOC_IF_ERROR_RETURN(
                bcm_esw_stat_custom_add(unit, 0, snmpBcmCustomReceive3,
                                        bcmDbgCntFcmPortClass3RxFrames));
            SOC_IF_ERROR_RETURN(
                bcm_esw_stat_custom_add(unit, 0, snmpBcmCustomReceive4,
                                        bcmDbgCntFcmPortClass3RxDiscards));
            SOC_IF_ERROR_RETURN(
                bcm_esw_stat_custom_add(unit, 0, snmpBcmCustomReceive5,
                                        bcmDbgCntFcmPortClass2RxFrames));
            SOC_IF_ERROR_RETURN(
                bcm_esw_stat_custom_add(unit, 0, snmpBcmCustomReceive6,
                                        bcmDbgCntFcmPortClass2RxDiscards));
            SOC_IF_ERROR_RETURN(
                bcm_esw_stat_custom_add(unit, 0, snmpBcmCustomTransmit6,
                                        bcmDbgCntFcmPortClass3TxFrames));
            SOC_IF_ERROR_RETURN(
                bcm_esw_stat_custom_add(unit, 0, snmpBcmCustomTransmit7,
                                        bcmDbgCntFcmPortClass2TxFrames));
        }

#endif /* BCM_TRIDENT2_SUPPORT */

        PBMP_PORT_ITER(unit, port) {
#ifdef BCM_TRIDENT2_SUPPORT
            if (SOC_IS_TD2_TT2(unit)) {
                SOC_IF_ERROR_RETURN
                    (WRITE_PGW_CNTMAXSIZEr
                     (unit, port, _bcm_stat_ovr_threshold[unit][port]));
                continue;
            }
#endif /* BCM_TRIDENT2_SUPPORT */
#if (defined(BCM_HURRICANE2_SUPPORT) && !defined(_HURRICANE2_DEBUG))
            if (SOC_IS_HURRICANE2(unit)) {
                if (IS_GE_PORT(unit,port)) {
                    SOC_IF_ERROR_RETURN
                        (WRITE_GPORT_CNTMAXSIZEr(unit, port, 
                                                 _bcm_stat_ovr_threshold[unit][port]));
                }
                continue;
            }
#endif
#ifdef BCM_GREYHOUND_SUPPORT
            if (SOC_IS_GREYHOUND(unit)) {
                if (IS_XL_PORT(unit,port)) {
                    SOC_IF_ERROR_RETURN
                        (WRITE_XLPORT_CNTMAXSIZEr(unit, port, 
                                          _bcm_stat_ovr_threshold[unit][port]));
                } else {
                    SOC_IF_ERROR_RETURN
                        (WRITE_GPORT_CNTMAXSIZEr(unit, port, 
                                          _bcm_stat_ovr_threshold[unit][port]));
                }
                continue;
            }
#endif

            if (soc_feature(unit, soc_feature_unified_port)) {
                if (!SOC_IS_HURRICANE2(unit) && !SOC_IS_GREYHOUND(unit)) {
                    SOC_IF_ERROR_RETURN
                        (WRITE_PORT_CNTMAXSIZEr(unit, port, 
                                          _bcm_stat_ovr_threshold[unit][port]));
               }
#if defined (BCM_KATANA_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
            } else if (SOC_IS_KATANAX(unit) && IS_MXQ_PORT(unit,port)) {
                SOC_IF_ERROR_RETURN
                        (WRITE_X_GPORT_CNTMAXSIZEr(unit, port,
                                                 _bcm_stat_ovr_threshold[unit][port]));
#endif
#if defined(BCM_ENDURO_SUPPORT)
            } else if (SOC_IS_ENDURO(unit) && IS_XQ_PORT(unit, port)) {
                SOC_IF_ERROR_RETURN
                    (WRITE_QPORT_CNTMAXSIZEr(unit, port, 
                                             _bcm_stat_ovr_threshold[unit][port]));
#endif /* BCM_ENDURO_SUPPORT */
            } else if (IS_GE_PORT(unit,port) || IS_FE_PORT(unit,port) ||
                IS_XL_PORT(unit, port) || IS_MXQ_PORT(unit,port)) {
                SOC_IF_ERROR_RETURN
                    (WRITE_GPORT_CNTMAXSIZEr(unit, port, 
                                             _bcm_stat_ovr_threshold[unit][port]));
            } else if (IS_IL_PORT(unit, port)) {
                if (!SOC_PBMP_MEMBER(si->il.disabled_bitmap, port)) {
                    SOC_IF_ERROR_RETURN(READ_IL_RX_CONFIGr(unit, port, &rval32));
                    soc_reg_field_set(unit, IL_RX_CONFIGr, &rval32,
                                      IL_RX_MAX_PACKET_SIZEf,
                                      _bcm_stat_ovr_threshold[unit][port]);
                    SOC_IF_ERROR_RETURN(WRITE_IL_RX_CONFIGr(unit, port, rval32));
                }
            } else {
                COMPILER_64_SET(rval, 0, _bcm_stat_ovr_threshold[unit][port]);
                SOC_IF_ERROR_RETURN(WRITE_MAC_CNTMAXSZr(unit, port, rval));
            }

            /* Set Receive Frame length for unimac */
            if ((IS_GE_PORT(unit,port) || IS_FE_PORT(unit,port)) && 
                    soc_feature(unit, soc_feature_unimac) &&
                    (!(IS_HL_PORT(unit,port)))) {
                if (SOC_REG_IS_VALID(unit, FRM_LENGTHr)) {
                    SOC_IF_ERROR_RETURN
                        (WRITE_FRM_LENGTHr(unit, port, _bcm_stat_ovr_threshold[unit][port]));
                }
            }

            /*
             * Devices that use 10G MAC + Trimac/Unimac
             */
#if defined(BCM_ENDURO_SUPPORT) || \
    defined(BCM_HURRICANE_SUPPORT)  
            if (IS_XQ_PORT(unit, port) && (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit))) {
                SOC_IF_ERROR_RETURN
                    (WRITE_QPORT_CNTMAXSIZEr(unit, port, 
                                             _bcm_stat_ovr_threshold[unit][port]));
            } else
#endif
            if (soc_feature(unit, soc_feature_unified_port) && 
                soc_feature(unit, soc_feature_flexible_xgport)) {
                    SOC_IF_ERROR_RETURN
                        (WRITE_PORT_CNTMAXSIZEr(unit, port, 
                                     _bcm_stat_ovr_threshold[unit][port]));
            } else if (IS_GX_PORT(unit, port) ||(IS_XG_PORT(unit, port) &&
                        soc_feature(unit, soc_feature_flexible_xgport))) {
                SOC_IF_ERROR_RETURN
                    (WRITE_GPORT_CNTMAXSIZEr(unit, port, 
                                 _bcm_stat_ovr_threshold[unit][port]));
            }
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    if ((!SAL_BOOT_SIMULATION) || (SAL_BOOT_BCMSIM)) {
        if ((!SAL_BOOT_BCMSIM) || (NULL != _bcm_stat_extra_counters)) {
            /* BCMSim initializes with counters cleared, so don't clear
             * for the first init on BCMSim. */
            SOC_IF_ERROR_RETURN(soc_counter_set32_by_port(unit, pbmp, 0));
        }
        SOC_IF_ERROR_RETURN(soc_counter_start(unit, flags, interval, pbmp));
    }

    if (NULL == _bcm_stat_extra_counters) {
        alloc_size = BCM_MAX_NUM_UNITS * sizeof(_bcm_stat_extra_counter_t *);
        _bcm_stat_extra_counters = sal_alloc(alloc_size, "device extra counters");
        if (NULL == _bcm_stat_extra_counters) {
            return (BCM_E_MEMORY);
        }
        sal_memset(_bcm_stat_extra_counters, 0, alloc_size);
        free_global_arr = TRUE;
    }
    alloc_size = _BCM_STAT_EXTRA_COUNTER_COUNT * sizeof(_bcm_stat_extra_counter_t);
    if (NULL == _bcm_stat_extra_counters[unit]) {
        _bcm_stat_extra_counters[unit] = sal_alloc(alloc_size, "device extra counters");
        if (NULL == _bcm_stat_extra_counters[unit]) {
            if (free_global_arr) {
                sal_free(_bcm_stat_extra_counters);
                _bcm_stat_extra_counters = NULL;
            }
            return (BCM_E_MEMORY);
        }
    }
    sal_memset(_bcm_stat_extra_counters[unit], 0, alloc_size);

    ctr = _bcm_stat_extra_counters[unit];
    ctr->reg = INVALIDr;
    if (SOC_REG_IS_VALID(unit, EGRDROPPKTCOUNTr)) {
        if (ctr->count64 == NULL) {
            ctr->count64 = sal_alloc(MAX_PORT(unit) * sizeof(uint64),
                                     "bcm extra counters");
            if (ctr->count64 == NULL) {
                sal_free(_bcm_stat_extra_counters[unit]);
                _bcm_stat_extra_counters[unit] = NULL;
                if (free_global_arr) {
                    sal_free(_bcm_stat_extra_counters);
                    _bcm_stat_extra_counters = NULL;
                }
                return SOC_E_MEMORY;
            }
            sal_memset(ctr->count64, 0, MAX_PORT(unit) * sizeof(uint64)); 
        }
        if (ctr->ctr_prev == NULL) {
            ctr->ctr_prev = sal_alloc(MAX_PORT(unit) * sizeof(uint32),
                                      "bcm extra counters");
            if (ctr->ctr_prev == NULL) {
                sal_free(ctr->count64);
                ctr->count64 = NULL;
                sal_free(_bcm_stat_extra_counters[unit]);
                _bcm_stat_extra_counters[unit] = NULL;
                if (free_global_arr) {
                    sal_free(_bcm_stat_extra_counters);
                    _bcm_stat_extra_counters = NULL;
                }
                return SOC_E_MEMORY;
            }
            sal_memset(ctr->ctr_prev, 0, MAX_PORT(unit) * sizeof(uint32)); 
        }
        ctr->reg = EGRDROPPKTCOUNTr;
    }
    soc_counter_extra_register(unit, _bcm_stat_counter_extra_callback);

#if defined(BCM_WARM_BOOT_SUPPORT) && (defined(BCM_TRIUMPH3_SUPPORT) || \
defined(BCM_KATANA_SUPPORT))
   if (!SOC_WARM_BOOT(unit)) {
        /* For cold boot, allocate WB scahe STAT module data */
        BCM_IF_ERROR_RETURN(_bcm_esw_stat_warm_boot_alloc(unit));
   }
#endif /* BCM_WARM_BOOT_SUPPORT */

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_init(unit));
    }
#endif
    if (soc_feature(unit, soc_feature_linkphy_coe)) {
#if defined (BCM_KATANA2_SUPPORT)
        BCM_IF_ERROR_RETURN(bcm_kt2_subport_counter_init(unit));
#endif
    }
    return BCM_E_NONE;
}
/*
 * Function:
 *	bcm_esw_stat_flex_pool_info_multi_get
 * Description:
 *	Retrieves the flex counter details for a given objectId
 * Parameters:
 *	
 *  unit             : (IN)  UNIT number      
 *  direction        : (IN)  Direction (Ingress/egress) 
 *  num_pools        : (IN)  Passing a 0, then actual_num_pools will return the number of pools that are available. 
 *                         return the number of pools that are available. 
 *                         Passing n will return up to n pools in the return array
 *  actual_num_pools : (OUT)  Returns actual no of pools available for a given direction
 *  flex_pool_stat   : (OUT)  array that provides the pool info
 *
 * Returns:
 *	BCM_E_*
 *	
 */

int bcm_esw_stat_flex_pool_info_multi_get(
    int unit,
    bcm_stat_flex_direction_t direction,
    uint32 num_pools,
    uint32 *actual_num_pools,
    bcm_stat_flex_pool_stat_info_t *flex_pool_stat)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_stat_flex_pool_info_multi_get(unit,
               direction,num_pools, actual_num_pools, flex_pool_stat);
    } else 
#endif
    { 
        return BCM_E_UNAVAIL;
    }
}


/*
 * Function:
 *	bcm_esw_stat_group_mode_id_create
 * Description:
 *	Create Customized Stat Group mode for given Counter Attributes
 * Parameters:
 *	
 *  unit           : (IN)  UNIT number      
 *  flags          : (IN)  STAT_GROUP_MODE_* flags (INGRESS/EGRESS) 
 *  total_counters : (IN)  Total Counters for Stat Flex Group Mode
 *  num_selectors  : (IN)  Number of Selectors for Stat Flex Group Mode
 *  attr_selectors : (IN)  Attribute Selectors for Stat Flex Group Mode
 *  mode_id        : (OUT) Created Mode Identifier for Stat Flex Group Mode
 *
 * Returns:
 *	BCM_E_*
 *	
 */
int bcm_esw_stat_group_mode_id_create(
    int unit,
    uint32 flags,
    uint32 total_counters,
    uint32 num_selectors,
    bcm_stat_group_mode_attr_selector_t *attr_selectors,
    uint32 *mode_id)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_stat_group_mode_id_create(unit,
               flags,total_counters,num_selectors,attr_selectors,mode_id);
    } else 
#endif
    { 
        return BCM_E_UNAVAIL;
    }
}
/*
 * Function:
 *	bcm_esw_stat_group_mode_id_get
 * Description:
 *	Retrieves Customized Stat Group mode Attributes for given mode_id
 * Parameters:
 *	
 *  unit                : (IN)  UNIT number      
 *  mode_id             : (IN) Created Mode Identifier for Stat Flex Group Mode
 *  flags               : (OUT)  STAT_GROUP_MODE_* flags (INGRESS/EGRESS) 
 *  total_counters      : (OUT)  Total Counters for Stat Flex Group Mode
 *  num_selectors       : (IN)  Number of Selectors for Stat Flex Group Mode
 *  attr_selectors      : (OUT)  Attribute Selectors for Stat Flex Group Mode
 *  actual_num_selectors: (OUT) Actual Number of Selectors for Stat Flex 
 *                              Group Mode
 *
 * Returns:
 *	BCM_E_*
 *	
 */
int bcm_esw_stat_group_mode_id_get(
    int unit,
    uint32 mode_id,
    uint32 *flags,
    uint32 *total_counters,
    uint32 num_selectors,
    bcm_stat_group_mode_attr_selector_t *attr_selectors,
    uint32 *actual_num_selectors)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_stat_group_mode_id_get(unit,
               mode_id, flags, total_counters, 
               num_selectors, attr_selectors, actual_num_selectors);
    } else
#endif
    { 
        return BCM_E_UNAVAIL;
    }
}
/*
 * Function:
 *	bcm_esw_stat_group_mode_id_destroy
 * Description:
 *	Destroys Customized Group mode
 * Parameters:
 *	
 *  unit                : (IN)  UNIT number      
 *  mode_id             : (IN) Created Mode Identifier for Stat Flex Group Mode
 *
 * Returns:
 *	BCM_E_*
 *	
 */
int bcm_esw_stat_group_mode_id_destroy(
    int unit,
    uint32 mode_id)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_stat_group_mode_id_destroy(unit, mode_id);
    } else
#endif
    { 
        return BCM_E_UNAVAIL;
    }
}

/*
 * Function:
 *	bcm_esw_stat_custom_group_create
 * Description:
 *	Associate an accounting object to customized group mode 
 *
 * Parameters:
 *	
 *  unit                : (IN)  UNIT number      
 *  mode_id             : (IN) Created Mode Identifier for Stat Flex Group Mode
 *  object              : (IN) Accounting object
 *  Stat_counter_id     : (OUT) Stat Counter Id
 *  num_entries         : (OUT) Number of Counter entries created
 *
 * Returns:
 *	BCM_E_*
 *	
 */

int bcm_esw_stat_custom_group_create(
    int unit,
    uint32 mode_id,
    bcm_stat_object_t object,
    uint32 *stat_counter_id,
    uint32 *num_entries)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_stat_custom_group_create(unit,
               mode_id, object, stat_counter_id, num_entries);
    } else
#endif
    { 
        return BCM_E_UNAVAIL;
    }
}


/*
 * Function:
 *      bcm_esw_stat_group_create
 * Description:
 *      Reserve HW counter resources as per given group mode and acounting
 *      object and make system ready for further stat collection action
 *     
 * Parameters:
 *    Unit            (IN)  Unit number
 *    object          (IN)  Accounting Object
 *    Group_mode      (IN)  Group Mode
 *    Stat_counter_id (OUT) Stat Counter Id
 *    num_entries     (OUT) Number of Counter entries created
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *     
 */
int bcm_esw_stat_group_create (
            int                   unit,
            bcm_stat_object_t     object,
            bcm_stat_group_mode_t group_mode,
            uint32                *stat_counter_id,
            uint32                *num_entries)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_stat_group_create(
                    unit, 
                    object, 
                    group_mode,
                    stat_counter_id,
                    num_entries);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        int i;
        int fs_index;
        struct obj2type_s {
            bcm_stat_object_t obj;
            _bcm_flex_stat_type_t fs_type;
        } obj2type[] = {
            {bcmStatObjectIngPort,     _bcmFlexStatTypeGport},
            {bcmStatObjectIngVlan,     _bcmFlexStatTypeService},
            {bcmStatObjectIngVlanXlate,_bcmFlexStatTypeVxlt},
            {bcmStatObjectIngVfi,      _bcmFlexStatTypeService},
            {bcmStatObjectIngL3Intf,   _bcmFlexStatTypeGport},
            {bcmStatObjectIngVrf,      _bcmFlexStatTypeVrf},
            {bcmStatObjectIngPolicy,   _bcmFlexStatTypeFp},   
            {bcmStatObjectIngMplsVcLabel, _bcmFlexStatTypeGport},
            {bcmStatObjectIngMplsSwitchLabel,_bcmFlexStatTypeMplsLabel},
            {bcmStatObjectEgrPort,     _bcmFlexStatTypeEgressGport},
            {bcmStatObjectEgrVlan,     _bcmFlexStatTypeEgressService},
            {bcmStatObjectEgrVlanXlate,_bcmFlexStatTypeEgrVxlt},
            {bcmStatObjectEgrVfi,      _bcmFlexStatTypeEgressService},
            {bcmStatObjectEgrL3Intf,   _bcmFlexStatTypeEgressGport}
        };

        if (group_mode != bcmStatGroupModeSingle) {
            return BCM_E_PARAM;
        }
        for (i = 0; i < OBJ_TYPE_NUM_ENTRIES; i++) {
            if (obj2type[i].obj == object) {
                break;
            }
        }
        if (i == OBJ_TYPE_NUM_ENTRIES) {
            return BCM_E_PARAM;
        }

        fs_index = _bcm_esw_flex_stat_free_index_assign(unit,
                                         obj2type[i].fs_type);
        if (!fs_index) {
            return BCM_E_PARAM;  
        }
        *stat_counter_id = _BCM_FLEX_STAT_COUNT_ID(obj2type[i].fs_type,
                                      fs_index);

        *num_entries = 1;
        return BCM_E_NONE;
    } else
#endif
    { 
        return BCM_E_UNAVAIL;
    }
}
/*
 * Function:
 *      bcm_esw_stat_group_destroy
 * Description:
 *      Release HW counter resources as per given counter id and makes system
 *      unavailable for any further stat collection action
 * Parameters:
 *      unit            - (IN) unit number
 *      Stat_counter_id - (IN) Stat Counter Id
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int bcm_esw_stat_group_destroy(
            int    unit,
            uint32 stat_counter_id)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_stat_group_destroy(unit,stat_counter_id);
    } else
#endif
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit,soc_feature_gport_service_counters)) {
        _bcm_flex_stat_type_t fs_type;
        uint32 fs_inx;

        fs_type = _BCM_FLEX_STAT_TYPE(stat_counter_id);
        fs_inx  = _BCM_FLEX_STAT_COUNT_INX(stat_counter_id);

        return _bcm_esw_flex_stat_count_index_remove(unit,fs_type,
                    fs_inx);
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

/*
 * Function:
 *      bcm_esw_stat_id_get_all
 * Description:
 *      Get all stat ids attached to the stat object.
 * Parameters:
 *      unit            - (IN) unit number
 *      object          - (IN) stat object
 *      stat_max        - (IN) max stat id count
 *      stat_array      - (OUT) array of stat ids
 *      stat_count      - (OUT) actual stat id count
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int 
bcm_esw_stat_id_get_all(
    int unit, 
    bcm_stat_object_t object, 
    int stat_max, 
    uint32 *stat_array, 
    int *stat_count)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return _bcm_esw_stat_id_get_all(
                    unit, 
                    object, 
                    stat_max, 
                    stat_array, 
                    stat_count);
    }
#endif

    return BCM_E_UNAVAIL;
}



/*
 * Function:
 *	bcm_stat_sync
 * Description:
 *	Synchronize software counters with hardware
 * Parameters:
 *	unit - StrataSwitch PCI device unit number (driver internal).
 * Returns:
 *	BCM_E_NONE - Success.
 *	BCM_E_INTERNAL - Chip access failure.
 * Notes:
 *	Makes sure all counter hardware activity prior to the call to
 *	bcm_stat_sync is reflected in all bcm_stat_get calls that come
 *	after the call to bcm_stat_sync.
 */

int
bcm_esw_stat_sync(int unit)
{

#if defined(BCM_WARM_BOOT_SUPPORT) && (defined(BCM_TRIUMPH3_SUPPORT) || \
defined(BCM_KATANA_SUPPORT))
    if (SOC_WARM_BOOT(unit)) {
        /* Sync WB stat overflow flags to scahe */
        BCM_IF_ERROR_RETURN(_bcm_esw_stat_warm_boot_recover(unit));
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        _bcm_esw_stat_flex_callback(unit);
    }
#endif
    return soc_counter_sync(unit);
}

/*
 * Function:
 *	_bcm_esw_stat_custom_get
 * Description:
 *	Return character name for specified stat value.
 * Parameters:
 *	unit - StrataSwitch PCI device unit number (driver internal).
 *	port - zero-based port number
 *      sync_mode - if 1 read hw counter else sw accumualated counter
 *	type - SNMP statistics type (see stat.h)
 *	val  - (OUT) 64 bit statistic counter value
 * Returns:
 *	NULL - invalid SNMP varialble type.
 *	!NULL - pointer to snmp variable name.
 */
STATIC int
_bcm_esw_stat_custom_get(int unit, bcm_port_t port, int sync_mode,
                         bcm_stat_val_t type, uint64 *val)
{
    uint64 count;
    REG_MATH_DECL; /* Required for use of the REG_* macros */

    COMPILER_REFERENCE(&count);
    COMPILER_64_ZERO(count);

    switch (type) {
    case snmpBcmCustomReceive0:
        REG_ADD(unit, port, sync_mode, RDBGC0r, count);
        break;
    case snmpBcmCustomReceive1:
        REG_ADD(unit, port, sync_mode, RDBGC1r, count);
        break;
    case snmpBcmCustomReceive2:
        REG_ADD(unit, port, sync_mode, RDBGC2r, count);
        break;
    case snmpBcmCustomReceive3:
        REG_ADD(unit, port, sync_mode, RDBGC3r, count);
        break;
    case snmpBcmCustomReceive4:
        REG_ADD(unit, port, sync_mode, RDBGC4r, count);
        break;
    case snmpBcmCustomReceive5:
        REG_ADD(unit, port, sync_mode, RDBGC5r, count);
        break;
    case snmpBcmCustomReceive6:
        REG_ADD(unit, port, sync_mode, RDBGC6r, count);
        break;
    case snmpBcmCustomReceive7:
        REG_ADD(unit, port, sync_mode, RDBGC7r, count);
        break;
    case snmpBcmCustomReceive8:
        REG_ADD(unit, port, sync_mode, RDBGC8r, count);
        break;
    case snmpBcmCustomTransmit0:
        REG_ADD(unit, port, sync_mode, TDBGC0r, count);
        break;
    case snmpBcmCustomTransmit1:
        REG_ADD(unit, port, sync_mode, TDBGC1r, count);
        break;
    case snmpBcmCustomTransmit2:
        REG_ADD(unit, port, sync_mode, TDBGC2r, count);
        break;
    case snmpBcmCustomTransmit3:
        REG_ADD(unit, port, sync_mode, TDBGC3r, count);
        break;
    case snmpBcmCustomTransmit4:
        REG_ADD(unit, port, sync_mode, TDBGC4r, count);
        break;
    case snmpBcmCustomTransmit5:
        REG_ADD(unit, port, sync_mode, TDBGC5r, count);
        break;
    case snmpBcmCustomTransmit6:
        REG_ADD(unit, port, sync_mode, TDBGC6r, count);
        break;
    case snmpBcmCustomTransmit7:
        REG_ADD(unit, port, sync_mode, TDBGC7r, count);
        break;
    case snmpBcmCustomTransmit8:
        REG_ADD(unit, port, sync_mode, TDBGC8r, count);
        break;
    case snmpBcmCustomTransmit9:
        REG_ADD(unit, port, sync_mode, TDBGC9r, count);
        break;
    case snmpBcmCustomTransmit10:
        REG_ADD(unit, port, sync_mode, TDBGC10r, count);
        break;
    case snmpBcmCustomTransmit11:
        REG_ADD(unit, port, sync_mode, TDBGC11r, count);
        break;
    case snmpBcmCustomTransmit12:
    case snmpBcmCustomTransmit13:
    case snmpBcmCustomTransmit14:
	/* XGS2 specific */
        break;
    default:
        return BCM_E_PARAM;
    }
    *val = count;
    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_stat_get
 * Description:
 *	Get the specified statistic from the StrataSwitch
 * Parameters:
 *	unit - StrataSwitch PCI device unit number (driver internal).
 *	port - zero-based port number
 *	type - SNMP statistics type (see stat.h)
 *      val - (OUT) 64-bit counter value.
 * Returns:
 *	BCM_E_NONE - Success.
 *	BCM_E_PARAM - Illegal parameter.
 *	BCM_E_BADID - Illegal port number.
 *	BCM_E_INTERNAL - Chip access failure.
 *	BCM_E_UNAVAIL - Counter/variable is not implemented
 *				on this current chip.
 * Notes:
 *	Some counters are implemented on a given port only when it is
 *	operating in a specific mode, for example, 10 or 100, and not 
 *	1000. If the counter is not implemented on a given port, OR, 
 *	on the port given its current operating mode, BCM_E_UNAVAIL
 *	is returned.
 */

int
_bcm_esw_stat_get(int unit, bcm_port_t port, int sync_mode, 
                  bcm_stat_val_t type, uint64 *val)
{
    int rv;
    soc_mac_mode_t	mode;

    BCM_IF_ERROR_RETURN(
        _bcm_esw_stat_gport_validate(unit, port, &port));
    
    if (soc_feature(unit, soc_feature_stat_xgs3)) {
        rv = _bcm_esw_stat_custom_get(unit, port, sync_mode, type, val);
        if (BCM_E_PARAM != rv) {
            return rv;
        }
    } 

    if (port == CMIC_PORT(unit)) {
	/* Rudimentary CPU statistics -- needs work */
	switch (type) {
	case snmpIfInOctets:
	    COMPILER_64_SET(*val, 0, SOC_CONTROL(unit)->stat.dma_rbyt);
	    break;
	case snmpIfInUcastPkts:
	    COMPILER_64_SET(*val, 0, SOC_CONTROL(unit)->stat.dma_rpkt);
	    break;
	case snmpIfOutOctets:
	    COMPILER_64_SET(*val, 0, SOC_CONTROL(unit)->stat.dma_tbyt);
	    break;
	case snmpIfOutUcastPkts:
	    COMPILER_64_SET(*val, 0, SOC_CONTROL(unit)->stat.dma_tpkt);
	    break;
	default:
	    COMPILER_64_ZERO(*val);
	    break;
	}
	return (BCM_E_NONE);
    } 

    if (soc_feature(unit, soc_feature_generic_counters)) {
#ifdef BCM_KATANA_SUPPORT
        if (SOC_IS_KATANA(unit)) { 
            if(IS_MXQ_PORT(unit, port)) { 
                return _bcm_stat_generic_get(unit, port, sync_mode, type, val);
            } else if(IS_GE_PORT(unit, port) || IS_FE_PORT(unit, port)) { 
                return (_bcm_stat_ge_get(unit, port, sync_mode, type,
                                         val, TRUE));
            } 
        } else 
#endif
#ifdef BCM_HURRICANE2_SUPPORT
	    if (SOC_IS_HURRICANE2(unit) || SOC_IS_GREYHOUND(unit)) {
		    if(IS_XL_PORT(unit, port)) { 
			    return _bcm_stat_generic_get(unit, port, sync_mode, type, val);
		    } else if(IS_GE_PORT(unit, port) || IS_FE_PORT(unit, port)) { 
			    return (_bcm_stat_ge_get(unit, port, sync_mode, type,
                                         val, TRUE));
		    } 
	    } else 
#endif
        {
            return _bcm_stat_generic_get(unit, port, sync_mode, type, val);
        }
    }

    /* For xgs3 gig ports, always use gig get function */
    if ((IS_FE_PORT(unit, port) && (soc_feature(unit, soc_feature_trimac) 
        || soc_feature(unit, soc_feature_unimac))) ||
        (SOC_IS_XGS3_SWITCH(unit) && IS_GE_PORT(unit, port)))  {
        return (_bcm_stat_ge_get(unit, port, sync_mode, type, val, TRUE));
    }

    SOC_IF_ERROR_RETURN(soc_mac_mode_get(unit, port, &mode));

    switch (mode) {
    case SOC_MAC_MODE_10_100:
	return (_bcm_stat_fe_get(unit, port, sync_mode, type, val));
    case SOC_MAC_MODE_1000_T:
        return (_bcm_stat_ge_get(unit, port, sync_mode, type, val, TRUE));
    case SOC_MAC_MODE_10000:
        if (IS_XE_PORT(unit, port)) {
            SOC_IF_ERROR_RETURN(_bcm_stat_xe_get(unit, port, sync_mode,
                                                 type, val));
            if (IS_XQ_PORT(unit, port)) {
                return SOC_E_NONE;
            } else if (IS_GX_PORT(unit, port)) {
                uint64 val1;
                SOC_IF_ERROR_RETURN(_bcm_stat_ge_get(unit, port, sync_mode, 
                                                     type, &val1, FALSE));
                COMPILER_64_ADD_64(*val, val1);
            }
            return SOC_E_NONE;
        } else {
            return (_bcm_stat_hg_get(unit, port, sync_mode, type, val));
        }
    default:
	assert(0);
    }

    return(BCM_E_NONE);
}

/*
 * Function:
 *	bcm_stat_get
 * Description:
 *	Get the specified statistic from the StrataSwitch
 * Parameters:
 *	unit - StrataSwitch PCI device unit number (driver internal).
 *	port - zero-based port number
 *	type - SNMP statistics type (see stat.h)
 *      val - (OUT) 64-bit counter value.
 * Returns:
 *	BCM_E_NONE - Success.
 *	BCM_E_PARAM - Illegal parameter.
 *	BCM_E_BADID - Illegal port number.
 *	BCM_E_INTERNAL - Chip access failure.
 *	BCM_E_UNAVAIL - Counter/variable is not implemented
 *				on this current chip.
 * Notes:
 *	Some counters are implemented on a given port only when it is
 *	operating in a specific mode, for example, 10 or 100, and not 
 *	1000. If the counter is not implemented on a given port, OR, 
 *	on the port given its current operating mode, BCM_E_UNAVAIL
 *	is returned.
 */

int
bcm_esw_stat_get(int unit, bcm_port_t port, bcm_stat_val_t type, uint64 *val)
{
    /* read software accumulated counters */
    return _bcm_esw_stat_get(unit, port, 0, type, val);
}


/*
 * Function:
 *	bcm_stat_get32
 * Description:
 *	Get the specified statistic from the StrataSwitch
 * Parameters:
 *	unit - StrataSwitch PCI device unit number (driver internal).
 *	port - zero-based port number
 *	type - SNMP statistics type (see stat.h)
 *      val - (OUT) 32-bit counter value.
 * Returns:
 *	BCM_E_NONE - Success.
 *	BCM_E_PARAM - Illegal parameter.
 *	BCM_E_BADID - Illegal port number.
 *	BCM_E_INTERNAL - Chip access failure.
 * Notes:
 *	Same as bcm_stat_get, except converts result to 32-bit.
 */

int
bcm_esw_stat_get32(int unit, bcm_port_t port, bcm_stat_val_t type, uint32 *val)
{
    int			rv;
    uint64		val64;

    rv = bcm_esw_stat_get(unit, port, type, &val64);

    COMPILER_64_TO_32_LO(*val, val64);

    return(rv);
}

/*
 * Function:
 *      bcm_esw_stat_multi_get
 * Purpose:
 *      Get the specified statistics from the device.
 * Parameters:
 *      unit - (IN) Unit number.
 *      port - (IN) <UNDEF>
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Array of SNMP statistics types defined in bcm_stat_val_t
 *      value_arr - (OUT) Collected statistics values
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_stat_multi_get(int unit, bcm_port_t port, int nstat, 
                       bcm_stat_val_t *stat_arr, uint64 *value_arr)
{
    int stix;

    if (nstat <= 0) {
        return BCM_E_PARAM;
    }

    if ((NULL == stat_arr) || (NULL == value_arr)) {
        return BCM_E_PARAM;
    }

    for (stix = 0; stix < nstat; stix++) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_stat_get(unit, port, stat_arr[stix],
                              &(value_arr[stix])));
    }

    return BCM_E_NONE; 
}

/*
 * Function:
 *      bcm_esw_stat_multi_get32
 * Purpose:
 *      Get the specified statistics from the device.  The 64-bit
 *      values may be truncated to fit.
 * Parameters:
 *      unit - (IN) Unit number.
 *      port - (IN) <UNDEF>
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Array of SNMP statistics types defined in bcm_stat_val_t
 *      value_arr - (OUT) Collected 32-bit statistics values
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_stat_multi_get32(int unit, bcm_port_t port, int nstat, 
                         bcm_stat_val_t *stat_arr, uint32 *value_arr)
{
    int stix;

    if (nstat <= 0) {
        return BCM_E_PARAM;
    }

    if ((NULL == stat_arr) || (NULL == value_arr)) {
        return BCM_E_PARAM;
    }

    for (stix = 0; stix < nstat; stix++) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_stat_get32(unit, port, stat_arr[stix],
                                &(value_arr[stix])));
    }

    return BCM_E_NONE; 
}

/*
 * Function:
 *    bcm_stat_sync_get
 * Description:
 *    Get the specified statistic from the StrataSwitch
 * Parameters:
 *    unit - StrataSwitch PCI device unit number (driver internal).
 *    port - zero-based port number
 *    type - SNMP statistics type (see stat.h)
 *    val - (OUT) 64-bit counter value.
 * Returns:
 *    BCM_E_NONE - Success.
 *    BCM_E_PARAM - Illegal parameter.
 *    BCM_E_BADID - Illegal port number.
 *    BCM_E_INTERNAL - Chip access failure.
 *    BCM_E_UNAVAIL - Counter/variable is not implemented
 *                    on this current chip.
 * Notes:
 *    Some counters are implemented on a given port only when it is
 *    operating in a specific mode, for example, 10 or 100, and not 
 *    1000. If the counter is not implemented on a given port, OR, 
 *    on the port given its current operating mode, BCM_E_UNAVAIL
 *    is returned.
 */

int
bcm_esw_stat_sync_get(int unit, bcm_port_t port, 
                           bcm_stat_val_t type, uint64 *val)
{
    /* read counter from the device */
    return _bcm_esw_stat_get(unit, port, 1, type, val);
}


/*
 * Function:
 *    bcm_stat_sync_get32
 * Description:
 *    Get the specified statistic from the StrataSwitch
 * Parameters:
 *    unit - StrataSwitch PCI device unit number (driver internal).
 *    port - zero-based port number
 *    type - SNMP statistics type (see stat.h)
 *    val  - (OUT) 32-bit counter value.
 * Returns:
 *    BCM_E_NONE - Success.
 *    BCM_E_PARAM - Illegal parameter.
 *    BCM_E_BADID - Illegal port number.
 *    BCM_E_INTERNAL - Chip access failure.
 * Notes:
 *    Same as bcm_stat_get, except converts result to 32-bit.
 */

int
bcm_esw_stat_sync_get32(int unit, bcm_port_t port, 
                             bcm_stat_val_t type, uint32 *val)
{
    int    rv;
    uint64 val64;

    rv = bcm_esw_stat_sync_get(unit, port, type, &val64);

    COMPILER_64_TO_32_LO(*val, val64);

    return(rv);
}

/*
 * Function:
 *      bcm_esw_stat_sync_multi_get
 * Purpose:
 *      Get the specified statistics from the device.
 * Parameters:
 *      unit - (IN) Unit number.
 *      port - (IN) <UNDEF>
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Array of SNMP statistics types defined in bcm_stat_val_t
 *      value_arr - (OUT) Collected statistics values
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_stat_sync_multi_get(int unit, bcm_port_t port, int nstat, 
                                 bcm_stat_val_t *stat_arr, uint64 *value_arr)
{
    int stix;

    if (nstat <= 0) {
        return BCM_E_PARAM;
    }

    if ((NULL == stat_arr) || (NULL == value_arr)) {
        return BCM_E_PARAM;
    }

    for (stix = 0; stix < nstat; stix++) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_stat_sync_get(unit, port, stat_arr[stix],
                                        &(value_arr[stix])));
    }

    return BCM_E_NONE; 
}

/*
 * Function:
 *      bcm_esw_stat_sync_multi_get32
 * Purpose:
 *      Get the specified statistics from the device.  The 64-bit
 *      values may be truncated to fit.
 * Parameters:
 *      unit - (IN) Unit number.
 *      port - (IN) <UNDEF>
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Array of SNMP statistics types defined in bcm_stat_val_t
 *      value_arr - (OUT) Collected 32-bit statistics values
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_stat_sync_multi_get32(int unit, bcm_port_t port,
                                   int nstat, bcm_stat_val_t *stat_arr, 
                                    uint32 *value_arr)
{
    int stix;

    if (nstat <= 0) {
        return BCM_E_PARAM;
    }

    if ((NULL == stat_arr) || (NULL == value_arr)) {
        return BCM_E_PARAM;
    }

    for (stix = 0; stix < nstat; stix++) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_stat_sync_get32(unit, port, stat_arr[stix],
                                          &(value_arr[stix])));
    }

    return BCM_E_NONE; 
}

/*
 * Function:
 *	bcm_stat_clear
 * Description:
 *	Clear the port based statistics from the StrataSwitch port.
 * Parameters:
 *	unit - StrataSwitch PCI device unit number (driver internal).
 *	port - zero-based port number
 * Returns:
 *	BCM_E_NONE - Success.
 *	BCM_E_INTERNAL - Chip access failure.
 */

int 
bcm_esw_stat_clear(int unit, bcm_port_t port)
{
    soc_reg_t           reg;
    pbmp_t		pbm;
    int                 i;

    BCM_IF_ERROR_RETURN(
        _bcm_esw_stat_gport_validate(unit, port, &port));

    if (port == CMIC_PORT(unit)) {
        /* Rudimentary CPU statistics -- needs work */
        SOC_CONTROL(unit)->stat.dma_rbyt = 0;
        SOC_CONTROL(unit)->stat.dma_rpkt = 0;
        SOC_CONTROL(unit)->stat.dma_tbyt = 0;
        SOC_CONTROL(unit)->stat.dma_tpkt = 0;
        return (BCM_E_NONE);
    }

    SOC_PBMP_CLEAR(pbm);
    SOC_PBMP_PORT_ADD(pbm, port);
    BCM_IF_ERROR_RETURN(soc_counter_set32_by_port(unit, pbm, 0));

    for (i = 0; i < _BCM_STAT_EXTRA_COUNTER_COUNT; i++) {
        reg = _bcm_stat_extra_counters[unit][i].reg;
        if (reg == INVALIDr) {
            continue;
        }
        if (SOC_REG_INFO(unit, reg).regtype != soc_portreg) {
            continue;
        }
        COMPILER_64_ZERO(_bcm_stat_extra_counters[unit][i].count64[port]);
    }

    return(BCM_E_NONE);
}


#ifdef BCM_XGS3_SWITCH_SUPPORT
#define _DBG_CNTR_IS_VALID(unit, type)                                   \
    ((type >= snmpBcmCustomReceive0 && type <= snmpBcmCustomReceive8) || \
    (type >= snmpBcmCustomTransmit0 && type <= (SOC_IS_FBX(unit) ?  \
    snmpBcmCustomTransmit11 : snmpBcmCustomTransmit14))) 

#define _DBG_CNTR_IS_RSV(type)                                           \
    ((type >= snmpBcmCustomReceive1 && type <= snmpBcmCustomReceive2) || \
     (type >= snmpBcmCustomTransmit0 && type <= snmpBcmCustomTransmit5)) 

#define _DBG_FLAG_IS_VALID(type, flag)                                   \
    (((type == snmpBcmCustomReceive0) && (flag == BCM_DBG_CNT_DROP)) ||  \
     ((type == snmpBcmCustomReceive1) && (flag == BCM_DBG_CNT_IMBP)) ||  \
     ((type == snmpBcmCustomReceive2) && (flag == BCM_DBG_CNT_RIMDR)) || \
     ((type == snmpBcmCustomTransmit0) &&                                \
      (flag == (BCM_DBG_CNT_TGIPMC6 | BCM_DBG_CNT_TGIP6))) ||            \
     ((type == snmpBcmCustomTransmit1) &&                                \
      (flag == (BCM_DBG_CNT_TIPMCD6 | BCM_DBG_CNT_TIPD6))) ||            \
     ((type == snmpBcmCustomTransmit2) &&                                \
      (flag == BCM_DBG_CNT_TGIPMC6)) ||                                  \
     ((type == snmpBcmCustomTransmit3) &&                                \
      (flag == BCM_DBG_CNT_TPKTD)) ||                                    \
     ((type == snmpBcmCustomTransmit4) &&                                \
      (flag == (BCM_DBG_CNT_TGIP4 | BCM_DBG_CNT_TGIP6))) ||              \
     ((type == snmpBcmCustomTransmit5) &&                                \
      (flag == (BCM_DBG_CNT_TIPMCD4 | BCM_DBG_CNT_TIPMCD6)))) 


typedef struct bcm_dbg_cntr_s {
    bcm_stat_val_t   counter;
    soc_reg_t        reg;
    soc_reg_t        select;
} bcm_dbg_cntr_t;


bcm_dbg_cntr_t bcm_dbg_cntr_rx[] = {
    { snmpBcmCustomReceive0,   RDBGC0r,  RDBGC0_SELECTr  },
    { snmpBcmCustomReceive1,   RDBGC1r,  RDBGC1_SELECTr  },
    { snmpBcmCustomReceive2,   RDBGC2r,  RDBGC2_SELECTr  },
    { snmpBcmCustomReceive3,   RDBGC3r,  RDBGC3_SELECTr  },
    { snmpBcmCustomReceive4,   RDBGC4r,  RDBGC4_SELECTr  },
    { snmpBcmCustomReceive5,   RDBGC5r,  RDBGC5_SELECTr  },
    { snmpBcmCustomReceive6,   RDBGC6r,  RDBGC6_SELECTr  },
    { snmpBcmCustomReceive7,   RDBGC7r,  RDBGC7_SELECTr  },
    { snmpBcmCustomReceive8,   RDBGC8r,  RDBGC8_SELECTr  }
};

bcm_dbg_cntr_t bcm_dbg_cntr_tx[] = {
    { snmpBcmCustomTransmit0,  TDBGC0r,  TDBGC0_SELECTr  },
    { snmpBcmCustomTransmit1,  TDBGC1r,  TDBGC1_SELECTr  },
    { snmpBcmCustomTransmit2,  TDBGC2r,  TDBGC2_SELECTr  },
    { snmpBcmCustomTransmit3,  TDBGC3r,  TDBGC3_SELECTr  },
    { snmpBcmCustomTransmit4,  TDBGC4r,  TDBGC4_SELECTr  },
    { snmpBcmCustomTransmit5,  TDBGC5r,  TDBGC5_SELECTr  },
    { snmpBcmCustomTransmit6,  TDBGC6r,  TDBGC6_SELECTr  },
    { snmpBcmCustomTransmit7,  TDBGC7r,  TDBGC7_SELECTr  },
    { snmpBcmCustomTransmit8,  TDBGC8r,  TDBGC8_SELECTr  },
    { snmpBcmCustomTransmit9,  TDBGC9r,  TDBGC9_SELECTr  },
    { snmpBcmCustomTransmit10, TDBGC10r, TDBGC10_SELECTr },
    { snmpBcmCustomTransmit11, TDBGC11r, TDBGC11_SELECTr }
};

#endif /* BCM_XGS3_SWITCH_SUPPORT */

/*
 * Function:
 *      bcm_stat_custom_set 
 * Description:
 *      Set debug counter to count certain packet types.
 * Parameters:
 *      unit  - StrataSwitch PCI device unit number.
 *      port  - Port number, -1 to set all ports. 
 *      type  - SNMP statistics type.
 *      flags - The counter select value (see stat.h for bit definitions). 
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The following debug counters are reserved for maintaining
 *      currently supported SNMP MIB objects:
 *
 *      snmpBcmCustomReceive0 - snmpBcmCustomReceive2
 *      snmpBcmCustomTransmit0 - snmpBcmCustomTransmit5
 */

#define SET(reg, field, flag)						\
	soc_reg_field_set(unit, (reg),					\
		          &ctr_sel, (field), (flags & (flag)) ? 1 : 0)

int
bcm_esw_stat_custom_set(int unit, bcm_port_t port,
		    bcm_stat_val_t type, uint32 flags)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    uint32	ctr_sel = 0;
    int		i;

    if (!SOC_IS_XGS3_SWITCH(unit)) {
        return BCM_E_UNAVAIL;
    }

    if (!_DBG_CNTR_IS_VALID(unit, type)) {
        return BCM_E_PARAM;
    }

    if (_DBG_CNTR_IS_RSV(type)) {
        if (!_DBG_FLAG_IS_VALID(type, flags)) {
            return BCM_E_CONFIG;
        }
    }

    /* port = -1 is valid port in that case */
    if ((port != -1)) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_stat_gport_validate(unit, port, &port));
    }

    for (i = 0; i < COUNTOF(bcm_dbg_cntr_rx); i++) {
        if (bcm_dbg_cntr_rx[i].counter == type) {
            if (SOC_IS_FBX(unit)) {
                SOC_IF_ERROR_RETURN(soc_reg32_get(unit,  bcm_dbg_cntr_rx[i].select,
                                                  REG_PORT_ANY, 0, &ctr_sel));
                if (SOC_IS_FBX(unit)) {
                    uint32 mask;

                    if (soc_feature(unit, soc_feature_dbgc_higig_lkup)) {
                        mask = BCM_FB_B0_DBG_CNT_RMASK; 
                    } else if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit)) {
                        mask = BCM_HB_DBG_CNT_RMASK; 
                    } else if (SOC_IS_RAPTOR(unit) || SOC_IS_RAVEN(unit) ||
                               SOC_IS_HAWKEYE(unit)){
                        mask = BCM_RP_DBG_CNT_RMASK; 
                    } else {
                        mask = BCM_FB_DBG_CNT_RMASK; 
                    }

                    soc_reg_field_set(unit, bcm_dbg_cntr_rx[i].select, 
                                      &ctr_sel, BITMAPf, flags & mask);
                } 
                SOC_IF_ERROR_RETURN(soc_reg32_set(unit, bcm_dbg_cntr_rx[i].select,
                                                  REG_PORT_ANY, 0, ctr_sel));
            }
        }
    }

    for (i = 0; i < COUNTOF(bcm_dbg_cntr_tx); i++) {
        if (bcm_dbg_cntr_tx[i].counter == type) {
            if (SOC_IS_FBX(unit)) {
                uint32 mask;

                SOC_IF_ERROR_RETURN(soc_reg32_get(unit, bcm_dbg_cntr_tx[i].select,
                                                  REG_PORT_ANY, 0, &ctr_sel));

                if (soc_feature(unit, soc_feature_dbgc_higig_lkup)) {
                    mask = BCM_FB_B0_DBG_CNT_TMASK; 
                } else if (SOC_IS_HB_GW(unit)) {
                    mask = BCM_HB_DBG_CNT_TMASK; 
                } else if (SOC_IS_RAPTOR(unit) || SOC_IS_RAVEN(unit) ||
                           SOC_IS_HAWKEYE(unit)){
                    mask = BCM_RP_DBG_CNT_TMASK; 
                } else {
                    mask = BCM_FB_DBG_CNT_TMASK; 
                }

                soc_reg_field_set(unit, bcm_dbg_cntr_tx[i].select, &ctr_sel, 
                                  BITMAPf, flags & mask);

                SOC_IF_ERROR_RETURN(soc_reg32_set(unit, bcm_dbg_cntr_tx[i].select,
                                                  REG_PORT_ANY, 0, ctr_sel));
            }
            else {
                soc_counter_set32_by_reg(unit, bcm_dbg_cntr_tx[i].reg, 0, 0);
            }
            break;
        }
    }

    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
}

/*
 * Function:
 *      bcm_stat_custom_get 
 * Description:
 *      Get debug counter select value.
 * Parameters:
 *      unit  - StrataSwitch PCI device unit number.
 *      port  - Port number (unused).
 *      type  - SNMP statistics type.
 *      flags - (OUT) The counter select value
 *		      (see stat.h for bit definitions).
 * Returns:
 *      BCM_E_XXX
 */

#define GET(reg, field, flag)						\
	if (soc_reg_field_get(unit, (reg), ctr_sel, (field))) {		\
	    tmp_flags |= (flag);					\
	}

int
bcm_esw_stat_custom_get(int unit, bcm_port_t port,
		    bcm_stat_val_t type, uint32 *flags)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    uint32	ctr_sel;
    int		i;

    if (!SOC_IS_XGS3_SWITCH(unit)) {
        return BCM_E_UNAVAIL;
    }

    if (!_DBG_CNTR_IS_VALID(unit, type)) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(
        _bcm_esw_stat_gport_validate(unit, port, &port));

    for (i = 0; i < COUNTOF(bcm_dbg_cntr_rx); i++) {
        if (bcm_dbg_cntr_rx[i].counter == type) {
            if (SOC_IS_FBX(unit)) {
                SOC_IF_ERROR_RETURN(soc_reg32_get(unit, bcm_dbg_cntr_rx[i].select,
                                                  REG_PORT_ANY, 0, &ctr_sel));
                if (SOC_IS_FBX(unit)) {
                    *flags = soc_reg_field_get(unit, bcm_dbg_cntr_rx[i].select,
                                               ctr_sel, BITMAPf);
                } 
            }
            break;
        }
    }

    for (i = 0; i < COUNTOF(bcm_dbg_cntr_tx); i++) {
        if (bcm_dbg_cntr_tx[i].counter == type) {
            if (SOC_IS_FBX(unit)) {
                SOC_IF_ERROR_RETURN(soc_reg32_get(unit, bcm_dbg_cntr_tx[i].select,
                                                  REG_PORT_ANY, 0, &ctr_sel));
                    *flags = soc_reg_field_get(unit, bcm_dbg_cntr_tx[i].select,
                                               ctr_sel, BITMAPf);
	    }
        break;
        }
    }
    return BCM_E_NONE;

#else
    return BCM_E_UNAVAIL;

#endif  /* BCM_XGS3_SWITCH_SUPPORT */
}


/*
 * Function:
 *      _bcm_stat_custom_to_bit 
 * Description:
 *      Calculate the bit that should be turned on for specific trigger 
 * Parameters:
 *      unit  - StrataSwitch PCI device unit number.
 *      chan  - flag to indicate RX or TX DBG counter
 *      trigger - The counter select value
 *		      (see stat.h for bit definitions).
 *      result - [OUT] - bit position that should be turned on.
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_stat_custom_to_bit(int unit, int chan, bcm_custom_stat_trigger_t trigger, 
                        uint32 *result)
{
    if (chan == _DBG_CNT_RX_CHAN) {

    /*  RX Part */

    switch (trigger) {

    case bcmDbgCntRIPD4:
        _BCM_DEFAULT_BIT_SET(unit, result, 0);
        break;
    case bcmDbgCntRIPC4:
        _BCM_DEFAULT_BIT_SET(unit, result, 1);
        break;
    case bcmDbgCntRIPHE4:
        _BCM_DEFAULT_BIT_SET(unit, result, 2);
        break;
    case bcmDbgCntIMRP4:
        _BCM_DEFAULT_BIT_SET(unit, result, 3);
        break;
    case bcmDbgCntRIPD6:
        _BCM_DEFAULT_BIT_SET(unit, result, 4);
        break;
    case bcmDbgCntRIPC6:
        _BCM_DEFAULT_BIT_SET(unit, result, 5);
        break;
    case bcmDbgCntRIPHE6:
        _BCM_DEFAULT_BIT_SET(unit, result, 6);
        break;
    case bcmDbgCntIMRP6:
        _BCM_DEFAULT_BIT_SET(unit, result, 7);
        break;
    case bcmDbgCntRDISC:
        _BCM_DEFAULT_BIT_SET(unit, result, 8);
        break;
    case bcmDbgCntRUC:
        _BCM_DEFAULT_BIT_SET(unit, result, 9);
        break;
    case bcmDbgCntRPORTD:
        _BCM_DEFAULT_BIT_SET(unit, result, 10);
        break;
    case bcmDbgCntPDISC:
        _BCM_DEFAULT_BIT_SET(unit, result, 11);
        break;
    case bcmDbgCntIMBP:
        _BCM_DEFAULT_BIT_SET(unit, result, 12);
        break;
    case bcmDbgCntRFILDR:
        _BCM_DEFAULT_BIT_SET(unit, result, 13);
        break;
    case bcmDbgCntRIMDR:
        _BCM_DEFAULT_BIT_SET(unit, result, 14);
        break;
    case bcmDbgCntRDROP:
        _BCM_DEFAULT_BIT_SET(unit, result, 15);
        break;
    case bcmDbgCntIRPSE:
        _BCM_DEFAULT_BIT_SET(unit, result, 16);
        break;
    case bcmDbgCntIRHOL:
        _BCM_FBX_BIT_SET(unit, result, 17);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntIRIBP:
        _BCM_FBX_BIT_SET(unit, result, 18);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntDSL3HE:
        _BCM_HB_GW_BIT_SET_E_PARAM(unit, result);
        _BCM_DEFAULT_BIT_SET(unit, result, 19);
        break;
    case bcmDbgCntIUNKHDR:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 19);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntDSL4HE:
        _BCM_HB_GW_BIT_SET_E_PARAM(unit, result);
        _BCM_DEFAULT_BIT_SET(unit, result, 20);
        break;
    case bcmDbgCntIMIRROR:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 20);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntDSICMP:
        _BCM_DEFAULT_BIT_SET(unit, result, 21);
        break;
    case bcmDbgCntDSFRAG:
        _BCM_DEFAULT_BIT_SET(unit, result, 22);
        break;
    case bcmDbgCntMTUERR:
        _BCM_DEFAULT_BIT_SET(unit, result, 23);
        break;
    case bcmDbgCntRTUN:
        _BCM_DEFAULT_BIT_SET(unit, result, 24);
        break;
    case bcmDbgCntRTUNE:
        _BCM_DEFAULT_BIT_SET(unit, result, 25);
        break;
    case bcmDbgCntVLANDR:
        _BCM_DEFAULT_BIT_SET(unit, result, 26);
        break;
    case bcmDbgCntRHGUC:
        _BCM_FB_BIT_SET(unit, result, 27);
        _BCM_TRX_BIT_SET(unit, result, 32);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntRHGMC:
        _BCM_FB_BIT_SET(unit, result, 28);
        _BCM_TRX_BIT_SET(unit, result, 33);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntMPLS:                      
        return BCM_E_PARAM;
    case bcmDbgCntMACLMT:
        _BCM_TRX_BIT_SET_E_PARAM(unit, result);
        if (soc_feature(unit, soc_feature_mac_learn_limit)) {
            *result = 29;   /* Value = 0x20000000 */
            return BCM_E_NONE;
        } else {
            return BCM_E_PARAM;
        }
        break;
    case bcmDbgCntMPLSERR:
        return BCM_E_PARAM;
    case bcmDbgCntURPFERR:
        return BCM_E_PARAM;
    case bcmDbgCntHGHDRE:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 27);
        _BCM_FB2_BIT_SET(unit, result, 29);
        _BCM_RAPTOR_RAVEN_BIT_SET(unit, result, 27);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntMCIDXE:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 28);
        _BCM_RAPTOR_RAVEN_BIT_SET(unit, result, 28);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntLAGLUP:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 29);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntLAGLUPD:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 30);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntPARITYD:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 31);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntVFPDR:
        _BCM_FB2_BIT_SET(unit, result, 30);
        _BCM_TRX_BIT_SET(unit, result, 35);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntURPF:
        _BCM_FB2_BIT_SET(unit, result, 31);
        _BCM_TRX_BIT_SET(unit, result, 34);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntDSTDISCARDDROP:
        _BCM_TRX_BIT_SET(unit, result, 36);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntCLASSBASEDMOVEDROP:
        if (soc_feature(unit, soc_feature_class_based_learning)) {
            _BCM_TRX_BIT_SET(unit, result, 37);
        }
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntMACLMT_NODROP:
        _BCM_TRX_BIT_SET(unit, result, 38);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntMACSAEQUALMACDA:
        _BCM_TRX_BIT_SET(unit, result, 39);
        _BCM_RAVEN_BIT_SET(unit, result, 30);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
     case bcmDbgCntMACLMT_DROP:
        _BCM_TRX_BIT_SET(unit, result, 40);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntFcmPortClass2RxDiscards:
        _BCM_TD2_BIT_SET(unit, result, 56);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntFcmPortClass2RxFrames:
        _BCM_TD2_BIT_SET(unit, result, 57);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntFcmPortClass3RxDiscards:
        _BCM_TD2_BIT_SET(unit, result, 58);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntFcmPortClass3RxFrames:
        _BCM_TD2_BIT_SET(unit, result, 59);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;

    default:
        return BCM_E_PARAM;
    }
 
   return BCM_E_NONE;

   } else if  (chan == _DBG_CNT_TX_CHAN) {
        
    /* TX Part */

    switch (trigger) {

    case bcmDbgCntTGIP4:
        _BCM_DEFAULT_BIT_SET(unit, result, 0);
        break;
    case bcmDbgCntTIPD4:
        _BCM_DEFAULT_BIT_SET(unit, result, 1);
        break;
    case bcmDbgCntTGIPMC4:
        _BCM_DEFAULT_BIT_SET(unit, result, 2);
        break;
    case bcmDbgCntTIPMCD4:
        _BCM_DEFAULT_BIT_SET(unit, result, 3);
        break;
    case bcmDbgCntTGIP6:
        _BCM_DEFAULT_BIT_SET(unit, result, 4);
        break;
    case bcmDbgCntTIPD6:
        _BCM_DEFAULT_BIT_SET(unit, result, 5);
        break;
    case bcmDbgCntTGIPMC6:
        _BCM_DEFAULT_BIT_SET(unit, result, 6);
        break;
    case bcmDbgCntTIPMCD6:
        _BCM_DEFAULT_BIT_SET(unit, result, 7);
        break;
    case bcmDbgCntTTNL:
        _BCM_DEFAULT_BIT_SET(unit, result, 8);
        break;
    case bcmDbgCntTTNLE:
        _BCM_DEFAULT_BIT_SET(unit, result, 9);
        break;
    case bcmDbgCntTTTLD:
        _BCM_DEFAULT_BIT_SET(unit, result, 10);
        break;
    case bcmDbgCntTCFID:
        _BCM_DEFAULT_BIT_SET(unit, result, 11);
        break;
    case bcmDbgCntTVLAN:
        _BCM_DEFAULT_BIT_SET(unit, result, 12);
        break;
    case bcmDbgCntTVLAND:
        _BCM_DEFAULT_BIT_SET(unit, result, 13);
        break;
    case bcmDbgCntTVXLTMD:
        _BCM_DEFAULT_BIT_SET(unit, result, 14);
        break;
    case bcmDbgCntTSTGD:
        _BCM_DEFAULT_BIT_SET(unit, result, 15);
        break;
    case bcmDbgCntTAGED:
        _BCM_DEFAULT_BIT_SET(unit, result, 16);
        break;
    case bcmDbgCntTL2MCD:
        _BCM_DEFAULT_BIT_SET(unit, result, 17);
        break;
    case bcmDbgCntTPKTD:
        _BCM_DEFAULT_BIT_SET(unit, result, 18);
        break;
    case bcmDbgCntTMIRR:
        _BCM_DEFAULT_BIT_SET(unit, result, 19);
        break;
    case bcmDbgCntTSIPL:
        _BCM_FB_HB_GW_TRX_BIT_SET(unit, result, 20);
        _BCM_RAVEN_BIT_SET(unit, result, 20);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTHGUC:
        _BCM_FB_HB_GW_TRX_BIT_SET(unit, result, 21);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTHGMC:
        _BCM_FB_HB_GW_TRX_BIT_SET(unit, result, 22);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTHIGIG2:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 23);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTHGI:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 24);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTL2_MTU:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 25);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTPARITY_ERR:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 26);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTIP_LEN_FAIL:
        _BCM_HB_GW_TRX_BIT_SET(unit, result, 27);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTMTUD:
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTSLLD:
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTL2MPLS:
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTL3MPLS:
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTMPLS:
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntTMODIDTOOLARGEDROP:
        _BCM_TRX_BIT_SET(unit, result, 28);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntPKTMODTOOLARGEDROP:
        _BCM_TRX_BIT_SET(unit, result, 29);
        _BCM_DEFAULT_BIT_SET_E_PARAM(unit, result);
        break;
    case bcmDbgCntFcmPortClass2TxFrames:
        _BCM_TD2_BIT_SET(unit, result, 31);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;
    case bcmDbgCntFcmPortClass3TxFrames:
        _BCM_TD2_BIT_SET(unit, result, 32);
        _BCM_DEFAULT_BIT_SET_E_PARAM (unit, result);
        break;

    default:
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;

    }
    return BCM_E_PARAM;
}

/*
 * Function:
 *      _bcm_stat_custom_change
 * Description:
 *      Add a certain packet type to debug counter to count 
 * Parameters:
 *      unit  - StrataSwitch PCI device unit number.
 *      port  - Port number (unused).
 *      type  - SNMP statistics type.
 *      trigger - The counter select value
 *		      (see stat.h for bit definitions).
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_stat_custom_change(int unit, bcm_port_t port, bcm_stat_val_t type,
                        bcm_custom_stat_trigger_t trigger, int value)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
   uint32	ctr_sel, ctrl_bmp, result, mask;
   int		i;
   
   for (i = 0; i < COUNTOF(bcm_dbg_cntr_rx); i++) {
       if (bcm_dbg_cntr_rx[i].counter == type) {
           SOC_IF_ERROR_RETURN(
               _bcm_stat_custom_to_bit(unit, _DBG_CNT_RX_CHAN, trigger, &result));
              
           if (result < 32) {
               SOC_IF_ERROR_RETURN(soc_reg32_get(unit, bcm_dbg_cntr_rx[i].select,
                                                 REG_PORT_ANY, 0, &ctr_sel));

               ctrl_bmp = soc_reg_field_get(unit, bcm_dbg_cntr_rx[i].select, 
                                            ctr_sel, BITMAPf);
               ctrl_bmp = (value) ? (ctrl_bmp | (1 << result)) :    /* add */
                   (ctrl_bmp & (~(1 << result)));  /* delete */

               soc_reg_field_set(unit, bcm_dbg_cntr_rx[i].select, 
                                 &ctr_sel, BITMAPf, ctrl_bmp);
               return soc_reg32_set(unit, bcm_dbg_cntr_rx[i].select,
                                    REG_PORT_ANY, 0, ctr_sel);
           }
#if defined(BCM_TRIUMPH_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
           else if (SOC_IS_TRX(unit) && result < 64) {
               SOC_IF_ERROR_RETURN(READ_RDBGC_SELECT_2r(unit, i, &ctr_sel));
               ctrl_bmp = soc_reg_field_get(unit, RDBGC_SELECT_2r,
                                            ctr_sel, BITMAPf);
               result -= 32;
               ctrl_bmp = (value) ? (ctrl_bmp | (1 << result)) :    /* add */
                   (ctrl_bmp & (~(1 << result)));  /* delete */

               soc_reg_field_set(unit, RDBGC_SELECT_2r,
                                 &ctr_sel, BITMAPf, ctrl_bmp);
               return (WRITE_RDBGC_SELECT_2r(unit, i, ctr_sel));
           }
#endif /* BCM_TRIUMPH_SUPPORT || BCM_SCORPION_SUPPORT */
           else {
               return BCM_E_PARAM;
           }
       }
   }

   for (i = 0; i < COUNTOF(bcm_dbg_cntr_tx); i++) {
       if (bcm_dbg_cntr_tx[i].counter == type) {
           SOC_IF_ERROR_RETURN(soc_reg32_get(unit, bcm_dbg_cntr_tx[i].select,
                                             REG_PORT_ANY, 0, &ctr_sel));
           ctrl_bmp = soc_reg_field_get(unit, bcm_dbg_cntr_tx[i].select, 
                                        ctr_sel, BITMAPf);
           SOC_IF_ERROR_RETURN(
               _bcm_stat_custom_to_bit(unit, _DBG_CNT_TX_CHAN, trigger, &result));

           if (value) { /* called from stat_custom_add */
               mask = (1 << result);
               ctrl_bmp |= mask;
           } else {     /* called from stat_custom_delete */
               mask = ~(1 << result);
               ctrl_bmp &= mask;
           }
           soc_reg_field_set(unit, bcm_dbg_cntr_tx[i].select, 
                             &ctr_sel, BITMAPf, ctrl_bmp);
           return soc_reg32_set(unit, bcm_dbg_cntr_tx[i].select,
                                REG_PORT_ANY, 0, ctr_sel);
       }
   }
   
#endif /* BCM_XGS3_SWITCH_SUPPORT */
   return BCM_E_PARAM;
}


/*
 * Function:
 *      bcm_esw_stat_custom_add 
 * Description:
 *      Add a certain packet type to debug counter to count 
 * Parameters:
 *      unit  - StrataSwitch PCI device unit number.
 *      port  - Port number (unused).
 *      type  - SNMP statistics type.
 *      trigger - The counter select value
 *		      (see stat.h for bit definitions).
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_stat_custom_add(int unit, bcm_port_t port, bcm_stat_val_t type,
                    bcm_custom_stat_trigger_t trigger)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT

   if (!SOC_IS_XGS3_SWITCH(unit)) {
       return BCM_E_UNAVAIL;
   }

   if (!_DBG_CNTR_IS_VALID(unit, type)) {
       return BCM_E_PARAM;
   }
   if (_DBG_CNTR_IS_RSV(type)) {
       return BCM_E_CONFIG;
   }
   BCM_IF_ERROR_RETURN(
       _bcm_esw_stat_gport_validate(unit, port, &port));

   return _bcm_stat_custom_change(unit, port, type, trigger, 1);  
#else
   return BCM_E_UNAVAIL;

#endif /* BCM_XGS3_SWITCH_SUPPORT */
}
/*
 * Function:
 *      bcm_esw_stat_custom_delete 
 * Description:
 *      Deletes a certain packet type from debug counter  
 * Parameters:
 *      unit  - StrataSwitch PCI device unit number.
 *      port  - Port number (unused).
 *      type  - SNMP statistics type.
 *      trigger - The counter select value
 *		      (see stat.h for bit definitions).
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_stat_custom_delete(int unit, bcm_port_t port,bcm_stat_val_t type, 
                       bcm_custom_stat_trigger_t trigger)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT

   if (!SOC_IS_XGS3_SWITCH(unit)) {
       return BCM_E_UNAVAIL;
   }

   if (!_DBG_CNTR_IS_VALID(unit, type)) {
       return BCM_E_PARAM;
   }

   if (_DBG_CNTR_IS_RSV(type)) {
      return BCM_E_CONFIG;
   }

   BCM_IF_ERROR_RETURN(
       _bcm_esw_stat_gport_validate(unit, port, &port));

   return _bcm_stat_custom_change(unit, port, type, trigger, 0);  
#else
   return BCM_E_UNAVAIL;

#endif /* BCM_XGS3_SWITCH_SUPPORT */
}

/*
 * Function:
 *      bcm_esw_stat_custom_delete_all
 * Description:
 *      Deletes a all packet types from debug counter  
 * Parameters:
 *      unit  - StrataSwitch PCI device unit number.
 *      port  - Port number (unused).
 *      type  - SNMP statistics type.
 *
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_stat_custom_delete_all(int unit, bcm_port_t port,bcm_stat_val_t type)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT

   int  i;
   int  res;
   
   for (i = 0; i < bcmDbgCntNum; i++) {
        res = bcm_esw_stat_custom_delete(unit, port, type, i);
        if (res != BCM_E_NONE && res != BCM_E_UNAVAIL && 
            res != BCM_E_NOT_FOUND && res != BCM_E_PARAM) {
            return res;
        }
   }
   return BCM_E_NONE;
#else
   return BCM_E_UNAVAIL;

#endif /* BCM_XGS3_SWITCH_SUPPORT */

}

/*
 * Function:
 *      bcm_esw_stat_custom_check
 * Description:
 *      Check if certain packet types is part of debug counter  
 * Parameters:
 *      unit  - StrataSwitch PCI device unit number.
 *      port  - Port number (unused).
 *      type  - SNMP statistics type.
 *      trigger - The counter select value
 *		      (see stat.h for bit definitions).
 *      result - [OUT] result of a query. 0 if positive , -1 if negative
 *
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_esw_stat_custom_check(int unit, bcm_port_t port, bcm_stat_val_t type, 
                      bcm_custom_stat_trigger_t trigger, int *result)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
   uint32	ctr_sel, ctrl_bmp, res;
   int		i;

   if (!SOC_IS_XGS3_SWITCH(unit)) {
       return BCM_E_UNAVAIL;
   }

   if (!_DBG_CNTR_IS_VALID(unit, type)) {
       return BCM_E_PARAM;
   }

   BCM_IF_ERROR_RETURN(
       _bcm_esw_stat_gport_validate(unit, port, &port));

   for (i = 0; i < COUNTOF(bcm_dbg_cntr_rx); i++) {
       if (bcm_dbg_cntr_rx[i].counter == type) {
           SOC_IF_ERROR_RETURN(
               _bcm_stat_custom_to_bit(unit, _DBG_CNT_RX_CHAN, trigger, &res));
           if (res < 32) {
               SOC_IF_ERROR_RETURN(soc_reg32_get(unit, bcm_dbg_cntr_rx[i].select,
                                                 REG_PORT_ANY, 0, &ctr_sel));
               ctrl_bmp = soc_reg_field_get(unit, bcm_dbg_cntr_rx[i].select, 
                                            ctr_sel, BITMAPf);
               *result = ((ctrl_bmp & (1 << res)) != 0); 
               return BCM_E_NONE;
           }
#if defined(BCM_TRIUMPH_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
           else if (SOC_IS_TRX(unit) && res < 64) {
               SOC_IF_ERROR_RETURN(READ_RDBGC_SELECT_2r(unit, i, &ctr_sel));
               ctrl_bmp = soc_reg_field_get(unit, RDBGC_SELECT_2r,
                                            ctr_sel, BITMAPf);
               *result = ((ctrl_bmp & (1 << (res - 32))) != 0);
               return BCM_E_NONE;
           }
#endif /* BCM_TRIUMPH_SUPPORT || BCM_SCORPION_SUPPORT */
           else {
               return BCM_E_PARAM;
           }
       }
   }

   for (i = 0; i < COUNTOF(bcm_dbg_cntr_tx); i++) {
       if (bcm_dbg_cntr_tx[i].counter == type) {
           SOC_IF_ERROR_RETURN(soc_reg32_get(unit, bcm_dbg_cntr_tx[i].select,
                                             REG_PORT_ANY, 0, &ctr_sel));
           ctrl_bmp = soc_reg_field_get(unit, bcm_dbg_cntr_tx[i].select, 
                                        ctr_sel, BITMAPf);
           SOC_IF_ERROR_RETURN(
               _bcm_stat_custom_to_bit(unit,  _DBG_CNT_TX_CHAN, trigger, &res));

           *result = ((ctrl_bmp & (1 << res)) != 0); 
           return BCM_E_NONE;
       }
   }
   return BCM_E_PARAM;
#else 
   return BCM_E_UNAVAIL;

#endif /* BCM_XGS3_SWITCH_SUPPORT */
}


/*
 * Function:
 *
 *	_bcm_esw_stat_ovr_error_control_set
 *
 * Description:
 *
 *  Sets Oversize error count flag to include/exclude oversized valid packets
 *  Oversize packet count increments  (ROVRr, TOVRr), when the packet size 
 *  exceeds port cnt size (PORT_CNTSIZEr).
 *  This function controls the inclusion of oversize packet counts as part of 
 *  of packet errors. 
 *
 *  Caution: By default the cotrol flag will be TRUE to include
 *  oversize  valid packets as oversize errors. 
 *  The users can turn off this flag to exclude oversize packet errors,
 *  which will differ from MIB RFC, which states that any packet greater 
 *  than the size of 1518 are considered as oversize errors. 
 *
 * Parameters:
 * 
 *      unit - unit number
 *      port - local port number
 *	    value - oversize error control flag
 *
 * Returns:
 *
 *      BCM_E_NONE
 */
int
_bcm_esw_stat_ovr_error_control_set(int unit, bcm_port_t port, int value)
{
    if (_bcm_stat_ovr_control == NULL) {
        return BCM_E_INIT;
    }

    if (port == CMIC_PORT(unit)) { /* Not supported on CMIC */
        return BCM_E_PORT;
    }

    if (value) {
        /* Set ovr control bit map for the port */
        _BCM_STAT_OVR_CONTROL_SET(unit, port);
    } else  {
        /* Set ovr control bit map for the port */
        _BCM_STAT_OVR_CONTROL_CLR(unit, port);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_esw_stat_ovr_error_control_get
 * Description:
 *	Get _bcm_stat_ovr_threshold for the given unit and port.
 * Parameters:
 *      unit - unit number
 *      port - local port number
 *	value - (OUT) pointer to oversize threshold
 * Returns:
 *      BCM_E_NONE
 */
int
_bcm_esw_stat_ovr_error_control_get(int unit, bcm_port_t port, int *value)
{
    if (_bcm_stat_ovr_control == NULL) {
        return BCM_E_INIT;
    }

    if (port == CMIC_PORT(unit)) { /* Not supported on CMIC */
        return BCM_E_PORT;
    }

    /* Get ovr control bit for the port */
    *value = ((_BCM_STAT_OVR_CONTROL_GET(unit, port)) ? 1 : 0);

    return BCM_E_NONE;
}


/*
 * Function:
 *	_bcm_esw_stat_ovr_threshold_set
 * Description:
 *	Set _bcm_stat_ovr_threshold for the given unit and port.
 * Parameters:
 *      unit - unit number
 *      port - local port number
 *	value - oversize threshold
 * Returns:
 *      BCM_E_NONE
 */
int
_bcm_esw_stat_ovr_threshold_set(int unit, bcm_port_t port, int value)
{
    if ((_bcm_stat_ovr_threshold == NULL) || 
        (_bcm_stat_ovr_threshold[unit] == NULL)) {
        return BCM_E_INIT;
    }

    if (port == CMIC_PORT(unit)) { /* Not supported on CMIC */
        return BCM_E_PORT;
    }

    _bcm_stat_ovr_threshold[unit][port] = value;

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        uint64 value64;
        int blk;
        bcm_pbmp_t blk_bitmap;
        bcm_port_t pix;
        if (SOC_IS_TD2_TT2(unit)) {
            SOC_IF_ERROR_RETURN(WRITE_PGW_CNTMAXSIZEr(unit, port, value));
        } else if (soc_feature(unit, soc_feature_unified_port) && 
                !SOC_IS_HURRICANE2(unit) && !SOC_IS_GREYHOUND(unit)) {
            SOC_IF_ERROR_RETURN
                    (WRITE_PORT_CNTMAXSIZEr(unit, port, value));
#if defined (BCM_KATANA_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
        } else if (SOC_IS_KATANAX(unit) && IS_MXQ_PORT(unit,port)) {
            SOC_IF_ERROR_RETURN
                    (WRITE_X_GPORT_CNTMAXSIZEr(unit, port, value));
#endif
#if defined (BCM_ENDURO_SUPPORT) || defined(BCM_HURRICANE_SUPPORT)
        } else if (IS_XQ_PORT(unit, port) && (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit))) {
            SOC_IF_ERROR_RETURN
                    (WRITE_QPORT_CNTMAXSIZEr(unit, port, value));
#endif
        } else if (IS_XL_PORT(unit, port) || IS_MXQ_PORT(unit,port)) {
            if ((SOC_IS_HURRICANE2(unit) || SOC_IS_GREYHOUND(unit)) && 
                    IS_XL_PORT(unit, port)) {
                SOC_IF_ERROR_RETURN
                        (WRITE_XLPORT_CNTMAXSIZEr(unit, port, value));
            } else {
                SOC_IF_ERROR_RETURN
                        (WRITE_GPORT_CNTMAXSIZEr(unit, port, value));
            }
        } else if (IS_GE_PORT(unit,port) || IS_FE_PORT(unit,port)) {
            SOC_IF_ERROR_RETURN(WRITE_GPORT_CNTMAXSIZEr(unit, port, value));
            if (!(soc_feature(unit, soc_feature_unified_port)) && \
                !(soc_feature(unit, soc_feature_unimac))) {
                if (SOC_REG_IS_VALID(unit, FRM_LENGTHr)) {
                    SOC_IF_ERROR_RETURN(WRITE_FRM_LENGTHr(unit, port, value));
                }
            }

            /* On some devices, this is per-PIC register */
            blk = SOC_PORT_BLOCK(unit, port);
            blk_bitmap = SOC_BLOCK_BITMAP(unit, blk);
            PBMP_ITER(blk_bitmap, pix) {
                _bcm_stat_ovr_threshold[unit][pix] = value;
            }
        } else {
            COMPILER_64_SET(value64, 0, value);
            SOC_IF_ERROR_RETURN(WRITE_MAC_CNTMAXSZr(unit, port, value64));

            if (IS_GX_PORT(unit, port)) { 
                SOC_IF_ERROR_RETURN(WRITE_GPORT_CNTMAXSIZEr(unit, port, value)); 
            } 
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_esw_stat_ovr_threshold_get
 * Description:
 *	Get _bcm_stat_ovr_threshold for the given unit and port.
 * Parameters:
 *      unit - unit number
 *      port - local port number
 *	value - (OUT) pointer to oversize threshold
 * Returns:
 *      BCM_E_NONE
 */
int
_bcm_esw_stat_ovr_threshold_get(int unit, bcm_port_t port, int *value)
{
    if ((_bcm_stat_ovr_threshold == NULL) || 
        (_bcm_stat_ovr_threshold[unit] == NULL)) {
        return BCM_E_INIT;
    }

    if (port == CMIC_PORT(unit)) { /* Not supported on CMIC */
        return BCM_E_PORT;
    }

    *value = _bcm_stat_ovr_threshold[unit][port];

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_esw_stat_group_dump_all
 * Description:
 *      Display all important information about all stat object  and groups
 *      Calls common function _bcm_esw_stat_group_dump_info()
 *
 * Parameters:
 *      unit  - (IN) unit number
 * Return Value:
 *      None
 * Notes:
 *      
 */
int
bcm_esw_stat_group_dump_all(int unit)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    _bcm_esw_stat_group_dump_info(unit, TRUE, 0, 0);
#endif
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_esw_stat_group_dump
 * Description:
 *      Display important information about specified stat object and group
 *      Calls common function _bcm_esw_stat_group_dump_info()
 * 
 * Parameters:
 *      unit        - (IN) unit number
 *      object      - (IN)  Accounting Object
 *      Group_mode  - (IN)  Group Mode
 * Return Value:
 *      None
 * Notes:
 *      
 */
int
bcm_esw_stat_group_dump(int unit, bcm_stat_object_t object,
                        bcm_stat_group_mode_t group_mode)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    _bcm_esw_stat_group_dump_info(unit, FALSE, object, group_mode);
#endif
    return BCM_E_NONE;
}

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_stat_sw_dump
 * Purpose:
 *     Displays STAT software structure information.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_stat_sw_dump(int unit)
{
    int             i;

    LOG_CLI((BSL_META_U(unit,
                        "\nSW Information STAT - Unit %d\n"), unit));

    if ((_bcm_stat_ovr_threshold != NULL) &&
        (_bcm_stat_ovr_threshold[unit] != NULL)) {
        LOG_CLI((BSL_META_U(unit,
                            "     Oversize Packet Byte Threshold:\n")));
        for (i = 0; i < SOC_MAX_NUM_PORTS; i++) {
            LOG_CLI((BSL_META_U(unit,
                                "       Port %2d : %4d\n"), i,
                     _bcm_stat_ovr_threshold[unit][i]));
        }
    }

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */
