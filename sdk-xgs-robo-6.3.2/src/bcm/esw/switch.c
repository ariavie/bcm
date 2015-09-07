/*
 * $Id: switch.c 1.682.2.6 Broadcom SDK $
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
 * File:        switch.c
 * Purpose:     BCM definitions  for bcm_switch_control and
 *              bcm_switch_port_control functions
 */

#include <sal/core/libc.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/hash.h>
#include <soc/ism_hash.h>
#include <soc/pbsmh.h>
#include <soc/higig.h>
#if defined(BCM_FIREBOLT2_SUPPORT)
#include <soc/lpm.h>
#endif /* BCM_FIREBOLT2_SUPPORT */
#if defined(BCM_BRADLEY_SUPPORT)
#include <soc/bradley.h>
#endif /* BCM_BRADLEY_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
#include <soc/trident.h>
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <soc/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
#include <soc/katana.h>
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
#include <soc/katana2.h>
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_SHADOW_SUPPORT)
#include <soc/shadow.h>
#endif /* BCM_SHADOW_SUPPORT */
#if defined(BCM_HURRICANE2_SUPPORT)
#include <soc/hurricane2.h>
#endif /* BCM_HURRICANE2_SUPPORT */

#include <soc/l2x.h>

#include <bcm/switch.h>
#include <bcm/error.h>

#include <bcm_int/esw/vlan.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/mcast.h>
#include <bcm_int/esw/ipmc.h>
#include <bcm_int/esw/link.h>
#include <bcm_int/esw/mirror.h>
#include <bcm_int/esw/trunk.h>
#include <bcm_int/esw/cosq.h>
#include <bcm_int/esw/virtual.h>
#ifdef BCM_WARM_BOOT_SUPPORT
#include <bcm_int/esw/stg.h>
#include <bcm_int/esw/field.h>
#include <bcm_int/esw/mpls.h>
#include <bcm_int/esw/oam.h>
#include <bcm_int/esw/qos.h>
#include <bcm_int/esw/trill.h>
#if defined(BCM_TRIDENT2_SUPPORT)
#include <bcm_int/esw/vxlan.h>
#endif
#include <bcm_int/esw/multicast.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/stat.h>
#include <bcm_int/esw/niv.h>
#include <bcm_int/esw/fcoe.h>
#ifdef BCM_IPFIX_SUPPORT
#include <bcm/ipfix.h>
#include <bcm_int/esw/ipfix.h>
#endif
#endif /* BCM_WARM_BOOT_SUPPORT */

#if defined(BCM_XGS3_SWITCH_SUPPORT)
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
#if defined(BCM_TRX_SWITCH_SUPPORT)
#include <bcm_int/esw/trx.h>
#endif  /* BCM_TRX_SUPPORT */
#if defined(BCM_TRIUMPH_SUPPORT)
#include <bcm_int/esw/triumph.h>
#endif  /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <bcm_int/esw/triumph2.h>
#endif  /* BCM_TRIUMPH2_SUPPORT */
#ifdef BCM_TRIDENT_SUPPORT
#include <bcm_int/esw/trident.h>
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
#include <bcm_int/esw/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT)
#include <bcm_int/bst.h>
#endif

#if defined(INCLUDE_REGEX)
#include <bcm_int/esw/fb4regex.h>
#endif  /* INCLUDE_REGEX */

#include <bcm_int/control.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw_dispatch.h>
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
#include <bcm_int/esw/policer.h>
#endif
#include <bcm_int/esw/lpmv6.h>

typedef int (*xlate_arg_f)(int unit, int arg, int set);

typedef struct {
    bcm_switch_control_t        type;
    uint32                      chip;
    soc_reg_t                   reg;
    soc_field_t                 field;
    xlate_arg_f                 xlate_arg;
    soc_feature_t               feature;
} bcm_switch_binding_t;

/* chip field */
#define FB      SOC_INFO_CHIP_FB
#define FB2     SOC_INFO_CHIP_FIREBOLT2
#define FH      SOC_INFO_CHIP_FHX
#define HXFX    SOC_INFO_CHIP_FX_HX
#define BR      SOC_INFO_CHIP_BRADLEY
#define GW      SOC_INFO_CHIP_GOLDWING
#define HBGW    SOC_INFO_CHIP_HB_GW
#define RP      SOC_INFO_CHIP_RAPTOR
#define TR      SOC_INFO_CHIP_TRIUMPH
#define RV      SOC_INFO_CHIP_RAVEN
#define SC      SOC_INFO_CHIP_SCORPION
#define TRX     SOC_INFO_CHIP_TRX
#define HK      SOC_INFO_CHIP_HAWKEYE
#define TRVL    SOC_INFO_CHIP_TR_VL
#define SCQ     SOC_INFO_CHIP_SC_CQ
#define TR2     SOC_INFO_CHIP_TRIUMPH2
#define AP      SOC_INFO_CHIP_APOLLO
#define VL      SOC_INFO_CHIP_VALKYRIE
#define VL2     SOC_INFO_CHIP_VALKYRIE2
#define EN      SOC_INFO_CHIP_ENDURO
#define TD      SOC_INFO_CHIP_TRIDENT
#define TT      SOC_INFO_CHIP_TITAN
#define HU      SOC_INFO_CHIP_HURRICANE
#define HU2     SOC_INFO_CHIP_HURRICANE2
#define TR3     SOC_INFO_CHIP_TRIUMPH3
#define HX4     SOC_INFO_CHIP_HELIX4
#define KT      SOC_INFO_CHIP_KATANA
#define KT2     SOC_INFO_CHIP_KATANA2
#define TD2     SOC_INFO_CHIP_TRIDENT2

/*
 * Function:
 *          _bcm_esw_switch_control_gport_resolve
 * Description:
 *          Decodes local physical port from a gport
 * Parameters:
 *          unit - StrataSwitch PCI device unit number (driver internal).
 *      gport - a gport to decode
 *      port - (Out) Local physical port encoded in gport
 * Returns:
 *      BCM_E_xxxx
 * Note
 *      In case of gport contains other then local port error will be returned.
 */


STATIC int
_bcm_esw_switch_control_gport_resolve(int unit, bcm_gport_t gport, bcm_port_t *port)
{
    bcm_module_t    modid;
    bcm_trunk_t     tgid;
    bcm_port_t      local_port;
    int             id, isMymodid;

    if (NULL == port) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(
        _bcm_esw_gport_resolve(unit, gport, &modid, &local_port, &tgid, &id));

    if ((BCM_TRUNK_INVALID != tgid) || (-1 != id)) {
        return BCM_E_PARAM;
    }
    /* Check if modid is local */
    BCM_IF_ERROR_RETURN(
        _bcm_esw_modid_is_local(unit, modid, &isMymodid));

    if (isMymodid != TRUE) {
        return BCM_E_PORT;
    }

    *port = local_port;
    return (BCM_E_NONE);
}


#ifdef BCM_XGS12_SWITCH_SUPPORT
/*
 * Function:
 *      _bcm_hashselect_set
 * Description:
 *      Set the Hash Select for L2. L3 and Multipath types
 *      are not supported on these devices.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to set.
 *      arg  - BCM_HASH_*
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_hashselect_set(int unit,
                    bcm_switch_control_t type,
                    int arg)
{
    uint32      hash_control, val=0;

    if (type != bcmSwitchHashL2) {
        return BCM_E_UNAVAIL;
    }

    switch (arg) {
        case BCM_HASH_ZERO:
            val = XGS_HASH_ZERO;
            break;
        case BCM_HASH_LSB:
            val = XGS_HASH_LSB;
            break;
        case BCM_HASH_CRC16L:
            val = XGS_HASH_CRC16_LOWER;
            break;
        case BCM_HASH_CRC16U:
            val = XGS_HASH_CRC16_UPPER;
            break;
        case BCM_HASH_CRC32L:
            val = XGS_HASH_CRC32_LOWER;
            break;
        case BCM_HASH_CRC32U:
            val = XGS_HASH_CRC32_UPPER;
            break;
        default:
            return BCM_E_PARAM;
    }
    BCM_IF_ERROR_RETURN(READ_HASH_CONTROLr(unit, &hash_control));
    soc_reg_field_set(unit, HASH_CONTROLr, &hash_control,
                      HASH_SELECTf, val);
    BCM_IF_ERROR_RETURN(WRITE_HASH_CONTROLr(unit, hash_control));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_hashselect_get
 * Description:
 *      Get the current Hash Select settings. Value returned
 *      is of the form BCM_HASH_*.
 *      All switch chip ports are configured with the same settings.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to retrieve.
 *      arg - Pointer to where the retrieved value will be written.
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_hashselect_get(int unit,
                    bcm_switch_control_t type,
                    int *arg)
{
    uint32      hash_control, val=0;

    if (type != bcmSwitchHashL2) {
        return BCM_E_UNAVAIL;
    }
    BCM_IF_ERROR_RETURN(READ_HASH_CONTROLr(unit, &hash_control));
    val = soc_reg_field_get(unit, HASH_CONTROLr,
                            hash_control, HASH_SELECTf);
    switch (val) {
        case XGS_HASH_ZERO:
            *arg = BCM_HASH_ZERO;
            break;
        case XGS_HASH_LSB:
            *arg = BCM_HASH_LSB;
            break;
        case XGS_HASH_CRC16_LOWER:
            *arg = BCM_HASH_CRC16L;
            break;
        case XGS_HASH_CRC16_UPPER:
            *arg = BCM_HASH_CRC16U;
            break;
        case XGS_HASH_CRC32_LOWER:
            *arg = BCM_HASH_CRC32L;
            break;
        case XGS_HASH_CRC32_UPPER:
            *arg = BCM_HASH_CRC32U;
            break;
    }
    return BCM_E_NONE;
}

/*
 * CPU control register set/get functions.
 * If port is specified (port < 0), only val or ival is used as appropriate.
 * Skip stack ports if port is not specified.
 */

STATIC int
_bcm_sw_cc_get(int unit, bcm_port_t port, uint64 *val)
{
    if (IS_E_PORT(unit, port)) {
        return READ_CPU_CONTROLr(unit, port, val);
    } else if (IS_HG_PORT(unit, port)) {
        return READ_ICPU_CONTROLr(unit, port, val);
    }

    return BCM_E_PORT;
}

STATIC int
_bcm_sw_cc_set(int unit, bcm_port_t port, uint64 val)
{
    if (IS_E_PORT(unit, port)) {
        return WRITE_CPU_CONTROLr(unit, port, val);
    } else if (IS_HG_PORT(unit, port)) {
        return WRITE_ICPU_CONTROLr(unit, port, val);
    }

    return BCM_E_PORT;
}

/* Map unit/port/type to a field in CPU CONTROL. Returns INVALIDf on error */

STATIC soc_field_t
_bcm_sw_field_get(int unit, bcm_port_t port, bcm_switch_control_t type)
{
    /* Choose the field */
    switch(type) {
    case bcmSwitchCpuProtocolPrio:
        return CPU_PROTOCOL_PRIORITYf;
        break;
    case bcmSwitchCpuUnknownPrio:
        return CPU_LKUPFAIL_PRIORITYf;
        break;
    case bcmSwitchCpuDefaultPrio:
        return CPU_DEFAULT_PRIORITYf;
        break;
    case bcmSwitchL2StaticMoveToCpu:
        return STATICMOVE_TOCPUf;
        break;
    case bcmSwitchUnknownVlanToCpu:
        return UVLAN_TOCPUf;
        break;
    case bcmSwitchUnknownIpmcToCpu:
        return UIPMC_TOCPUf;
        break;
    case bcmSwitchUnknownMcastToCpu:
        return UMC_TOCPUf;
        break;
    case bcmSwitchUnknownUcastToCpu:
        return UUCAST_TOCPUf;
        break;
    case bcmSwitchL3HeaderErrToCpu:
        return L3ERR_TOCPUf;
        break;
    case bcmSwitchUnknownL3SrcToCpu:
        return UNRESOLVEDL3SRC_TOCPUf;
        break;
    case bcmSwitchUnknownL3DestToCpu:
        return L3DSTMISS_TOCPUf;
        break;
    case bcmSwitchIpmcPortMissToCpu:
        return IPMCPORTMISS_TOCPUf;
        break;
    default:
        break;
    }

    return INVALIDf;
}
#endif

#ifdef BCM_WARM_BOOT_SUPPORT

int
_bcm_esw_scache_ptr_get(int unit, soc_scache_handle_t handle, int create,
                        uint32 size, uint8 **scache_ptr, 
                        uint16 default_ver, uint16 *recovered_ver)
{
    int stable_size, stable_used, alloc_size, rv;
    uint32 alloc_get;
    uint16 version = default_ver;

    if (NULL == scache_ptr) {
        return BCM_E_PARAM;
    }

    SOC_SCACHE_ALIGN_SIZE(size);

    /* Individual BCM module implementations are version-aware. The 
     * default_ver is the latest version that the module is aware of.
     * Each module should be able to understand versions <= default_ver.
     * The actual recovered_ver is returned to the calling module during
     * warm boot initialization. The individual module needs to parse its
     * scache based on the recovered_ver.
     */
    alloc_size = size + SOC_WB_SCACHE_CONTROL_SIZE;

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
    SOC_IF_ERROR_RETURN(soc_stable_used_get(unit, &stable_used));

    rv = soc_scache_ptr_get(unit, handle, scache_ptr, &alloc_get);
    if ((SOC_E_NOT_FOUND == rv) &&
        create && (alloc_size > (stable_size - stable_used))) {
        /* scache out of space - requested scache chunk
         * is greater than available scache space  */
        if (stable_size > 0) {
            BCM_ERR(("Scache out of space."
                     "max=%d bytes, used=%d bytes, alloc_size=%d bytes\n ",
                     stable_size, stable_used, alloc_size ));
            return BCM_E_RESOURCE;
        } else { /* level 1 recovery */
            BCM_VERB(("Scache not found...Level 1 recovery\n"));
            return BCM_E_NOT_FOUND;
        }
    } else {
        if (create) {
            if (SOC_E_NOT_FOUND == rv) {
                SOC_IF_ERROR_RETURN
                    (soc_scache_alloc(unit, handle, alloc_size));
                rv = soc_scache_ptr_get(unit, handle, scache_ptr,
                                        &alloc_get);
            } else if (alloc_size != alloc_get) {
                BCM_VERB(("Reallocating %d bytes of scache space\n",
                          (alloc_size - alloc_get)));
                BCM_IF_ERROR_RETURN
                   (soc_scache_realloc(unit, handle, (alloc_size - alloc_get)));
            }
            BCM_VERB(("Allocated raw scache pointer=%p, %d bytes\n",
                      scache_ptr, alloc_size));
        }
        if (SOC_FAILURE(rv)) {
            return rv;
        } else if ((0 != size) && (alloc_get != alloc_size) && 
                   !SOC_WARM_BOOT(unit) && (!create)) {
            /* Expected size doesn't match retrieved size.
             * This is expected during 'sync' operation 
             * during SDK downgrade */
            BCM_VERB(("Reallocating %d bytes of scache space\n",
                      (alloc_size - alloc_get)));
            BCM_IF_ERROR_RETURN
                (soc_scache_realloc(unit, handle, (alloc_size - alloc_get)));
        } else if (NULL == *scache_ptr) {
            return BCM_E_MEMORY;
        }

        if (!SOC_WARM_BOOT(unit)) {
            /* Newly allocated, set up version info */
            sal_memcpy(*scache_ptr, &version, sizeof(uint16));
        }

        if (SOC_WARM_BOOT(unit)) {
            /* Warm Boot recovery, verify the correct version */
            sal_memcpy(&version, *scache_ptr, sizeof(uint16));
            BCM_VERB(("Obtained scache pointer=%p, %d bytes, "
                      "version=%d.%d\n",
                      scache_ptr, alloc_get,
                      SOC_SCACHE_VERSION_MAJOR(version),
                      SOC_SCACHE_VERSION_MINOR(version)));

            if (version > default_ver) {
                BCM_ERR(("Downgrade detected.  "
                         "Current version=%d.%d  found %d.%d\n",
                         SOC_SCACHE_VERSION_MAJOR(default_ver),
                         SOC_SCACHE_VERSION_MINOR(default_ver),
                         SOC_SCACHE_VERSION_MAJOR(version),
                         SOC_SCACHE_VERSION_MINOR(version)));
                /* Notify the application with an event */
                /* The application will then need to reconcile the 
                   version differences using the documented behavioral
                   differences on per module (handle) basis */
                SOC_IF_ERROR_RETURN
                    (soc_event_generate(unit, 
                                        SOC_SWITCH_EVENT_WARM_BOOT_DOWNGRADE, 
                                        handle, version, default_ver));
                rv = BCM_E_NONE;

            } else if (version < default_ver) {
                BCM_VERB(("Upgrade scenario supported.  "
                         "Current version=%d.%d  found %d.%d\n",
                         SOC_SCACHE_VERSION_MAJOR(default_ver),
                         SOC_SCACHE_VERSION_MINOR(default_ver),
                         SOC_SCACHE_VERSION_MAJOR(version),
                         SOC_SCACHE_VERSION_MINOR(version)));
                rv = BCM_E_NONE;
            } 
            if (recovered_ver) {
                /* Modules that support multiple versions will suppy a pointer
                 * to the recovered_ver. Others will not care */
                *recovered_ver = version;
            }
        }
    }

    /* Advance over scache control info */
    *scache_ptr += SOC_WB_SCACHE_CONTROL_SIZE;
    return BCM_E_NONE;
}


#define _BCM_SYNC_SUCCESS(unit) \
        (BCM_SUCCESS(rv) || (BCM_E_INIT == rv) || (BCM_E_UNAVAIL == rv))

#ifdef BCM_XGS3_SWITCH_SUPPORT
STATIC int
_bcm_switch_control_sync(int unit, int arg)
{
    int rv = BCM_E_NONE;

#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        /* Only few modules are applicable for Shadow */
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_vlan_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_port_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_link_sync(unit);
        }
    } else 
#endif /* BCM_SHADOW_SUPPORT */
    {
        /* Call each module to write Level 2 Warm Boot info */
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_vlan_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_stg_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_cosq_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_mirror_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_field_scache_sync(unit);
        }
#ifdef INCLUDE_L3
#ifdef BCM_TRX_SUPPORT
        if (_BCM_SYNC_SUCCESS(rv)) {
            if (SOC_IS_TRX(unit) && !(SOC_IS_TITAN(unit) || SOC_IS_TITAN2(unit))) {
                if (soc_feature(unit, soc_feature_virtual_switching)) {
                    rv = _bcm_esw_virtual_sync(unit);
                }
            }
        }
#endif /* BCM_TRX_SUPPORT */
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_l3_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_ipmc_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            if (soc_feature(unit, soc_feature_mpls)) {
                rv = _bcm_esw_mpls_sync(unit);
            }
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_multicast_sync(unit);
        }
#endif /* INCLUDE_L3 */
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_trunk_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_link_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_port_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_oam_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_qos_sync(unit);
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_stk_sync(unit);
        }
#ifdef BCM_IPFIX_SUPPORT
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_ipfix_sync(unit);
        }
#endif /* BCM_IPFIX_SUPPORT */
#ifdef INCLUDE_L3
#ifdef BCM_TRIDENT_SUPPORT
        if (_BCM_SYNC_SUCCESS(rv)) {
            if (SOC_IS_TRIDENT(unit) || SOC_IS_TRIDENT2(unit)) {
                if (soc_feature(unit, soc_feature_trill)) {
                    rv = _bcm_esw_trill_sync(unit);
                }
            }
        }
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
        if (_BCM_SYNC_SUCCESS(rv)) {
            if (soc_feature(unit, soc_feature_vxlan)) {
                rv = _bcm_esw_vxlan_sync(unit);
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_niv_sync(unit);
        }

#endif /* INCLUDE_L3 */

#if defined(INCLUDE_REGEX)
        if (_BCM_SYNC_SUCCESS(rv)) {
            rv = _bcm_esw_regex_sync(unit);
        }
#endif /* INCLUDE_REGEX */

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || \
    defined(BCM_TRIDENT2_SUPPORT)
        if (_BCM_SYNC_SUCCESS(rv)) {
            if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                SOC_IS_TD2_TT2(unit)) {
                rv = _bcm_esw_global_meter_policer_sync(unit);
            }
        }
        if (_BCM_SYNC_SUCCESS(rv)) {
            if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                SOC_IS_TD2_TT2(unit)) {
                rv = _bcm_esw_stat_sync(unit);
            }
        }    
    if (_BCM_SYNC_SUCCESS(rv)) {
        if (SOC_IS_TRIDENT2(unit)) {
            if (soc_feature(unit, soc_feature_fcoe)) {
                rv = _bcm_esw_fcoe_sync(unit);
            }
        }
    }    
#endif /* BCM_KATANA_SUPPORT */
    }
    /* Now send all data to the persistent storage */
    if (_BCM_SYNC_SUCCESS(rv)) {
        rv = soc_scache_commit(unit);
    }
    /* Mark scache as clean */
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 0;
    SOC_CONTROL_UNLOCK(unit);

    if (_BCM_SYNC_SUCCESS(rv)) {
        rv = _bcm_mem_scache_sync(unit);
        if (rv == BCM_E_UNAVAIL) {
            rv = BCM_E_NONE;
        }
    }
    return rv;
}
#endif /* BCM_XGS3_SWITCH_SUPPORT */
#endif /* BCM_WARM_BOOT_SUPPORT */


#ifdef BCM_XGS3_SWITCH_SUPPORT

/*
 * Function:
 *      _bcm_vrf_max_get
 * Description:
 *      Get the current maximum vrf value.
 * Parameters:
 *      unit - (IN) BCM device number.
 *  arg -  (OUT)Pointer to where the retrieved value will be written.
 * Returns:
 *      BCM_E_xxxx
 */
STATIC int
_bcm_vrf_max_get(int unit, int *arg)
{
    *arg = SOC_VRF_MAX(unit);
    return (BCM_E_NONE);
}


/* Helper routines for argument translation */

STATIC int
_bool_invert(int unit, int arg, int set)
{
    /* Same for both set/get */
    return !arg;
}

/* Helper routines for "chip-wide" packet aging set/get */

STATIC int
_bcm_fb_pkt_age_set(int unit, int arg)
{
    if (!arg) {
        /* Disable packet aging on all COSQs */
        SOC_IF_ERROR_RETURN(WRITE_PKTAGINGTIMERr(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_PKTAGINGLIMITr(unit, 0)); /* Aesthetic */
    } else if ((arg < 0) || (arg > 7162)) {
        return BCM_E_PARAM;
    } else {
        /* Set all COSQs to the same value */
        SOC_IF_ERROR_RETURN(WRITE_PKTAGINGTIMERr(unit, ((8 * arg) + 6) / 7));
        SOC_IF_ERROR_RETURN(WRITE_PKTAGINGLIMITr(unit, 0));
    }

    return BCM_E_NONE;
}

#ifdef BCM_TRX_SUPPORT
STATIC int
_bcm_ts_pkt_age_set(int unit, int arg)
{
    if (!arg) {
        /* Disable packet aging on all COSQs */
        SOC_IF_ERROR_RETURN(WRITE_PKTAGINGTIMERr(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_PKTAGINGLIMIT0r(unit, 0)); /* Aesthetic */
        SOC_IF_ERROR_RETURN(WRITE_PKTAGINGLIMIT1r(unit, 0)); /* Aesthetic */
    } else if ((arg < 0) || (arg > 7162)) {
        return BCM_E_PARAM;
    } else {
        /* Set all COSQs to the same value */
        SOC_IF_ERROR_RETURN(WRITE_PKTAGINGTIMERr(unit, ((8 * arg) + 6) / 7));
        SOC_IF_ERROR_RETURN(WRITE_PKTAGINGLIMIT0r(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_PKTAGINGLIMIT1r(unit, 0));
    }

    return BCM_E_NONE;
}
#endif

#ifdef BCM_KATANA_SUPPORT
STATIC int
_bcm_kt_pkt_age_set(int unit, int arg)
{
    if (!arg) {
        /* Disable packet aging on all COSQs */
        SOC_IF_ERROR_RETURN(WRITE_PKTINTAGINGTIMERr(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_PKTINTAGINGLIMIT0r(unit, 0)); /* Aesthetic */
        SOC_IF_ERROR_RETURN(WRITE_PKTINTAGINGLIMIT1r(unit, 0)); /* Aesthetic */
        SOC_IF_ERROR_RETURN(WRITE_PKTEXTAGINGTIMERr(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_PKTEXTAGINGLIMIT0r(unit, 0)); /* Aesthetic */
        SOC_IF_ERROR_RETURN(WRITE_PKTEXTAGINGLIMIT1r(unit, 0)); /* Aesthetic */

    } else if ((arg < 0) || (arg > 7162)) {
        return BCM_E_PARAM;
    } else {
        /* Set all COSQs to the same value */
        SOC_IF_ERROR_RETURN(WRITE_PKTINTAGINGTIMERr(unit, ((8 * arg) + 6) / 7));
        SOC_IF_ERROR_RETURN(WRITE_PKTINTAGINGLIMIT0r(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_PKTINTAGINGLIMIT1r(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_PKTEXTAGINGTIMERr(unit, ((8 * arg) + 6) / 7));
        SOC_IF_ERROR_RETURN(WRITE_PKTEXTAGINGLIMIT0r(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_PKTEXTAGINGLIMIT1r(unit, 0));
    }

    return BCM_E_NONE;
}
#endif

STATIC int
_bcm_fb_pkt_age_get(int unit, int *arg)
{
    uint32 timerval, limitval, mask;
    int i, loopcount;

    /* Check that all COSQs have the same value */
    SOC_IF_ERROR_RETURN(READ_PKTAGINGLIMITr(unit, &limitval));
    limitval &= 0xFFFFFF; /* AGINGLIMITCOSn fields only */
    loopcount = mask = soc_reg_field_get(unit, PKTAGINGLIMITr,
                                         limitval, AGINGLIMITCOS0f);
    /* Make mask of eight identical AGINGLIMITCOSn fields */
    for (i = 1; i < 8; i++) {
        mask = (mask << 3) | loopcount;
    }

    if (mask != limitval) {
        /* COSQs currently are not programmed identically */
        return BCM_E_CONFIG;
    }

    /* Return the (common) setting */
    loopcount = 7 - loopcount;
    SOC_IF_ERROR_RETURN(READ_PKTAGINGTIMERr(unit, &timerval));

    *arg =
      (loopcount *
       soc_reg_field_get(unit, PKTAGINGTIMERr, timerval, DURATIONSELECTf)) / 8;

    return BCM_E_NONE;
}

#ifdef BCM_TRX_SUPPORT
STATIC int
_bcm_ts_pkt_age_get(int unit, int *arg)
{
    uint32 timerval, limitval, mask;
    int i, loopcount;

    /* Check that all COSQs have the same value */
    SOC_IF_ERROR_RETURN(READ_PKTAGINGLIMIT0r(unit, &limitval));
    limitval &= 0xFFFFFF; /* AGINGLIMITPRIn fields only */
    loopcount = mask = soc_reg_field_get(unit, PKTAGINGLIMIT0r,
                                         limitval, AGINGLIMITPRI0f);
    /* Make mask of eight identical AGINGLIMITPRIn fields */
    for (i = 1; i < 8; i++) {
        mask = (mask << 3) | loopcount;
    }

    if (mask != limitval) {
        /* COSQs currently are not programmed identically */
        return BCM_E_CONFIG;
    }

    /* Check second register */
    SOC_IF_ERROR_RETURN(READ_PKTAGINGLIMIT1r(unit, &limitval));
    limitval &= 0xFFFFFF; /* AGINGLIMITPRIn fields only */
    if (mask != limitval) {
        /* COSQs currently are not programmed identically */
        return BCM_E_CONFIG;
    }

    /* Return the (common) setting */
    loopcount = 7 - loopcount;
    SOC_IF_ERROR_RETURN(READ_PKTAGINGTIMERr(unit, &timerval));

    *arg =
      (loopcount *
       soc_reg_field_get(unit, PKTAGINGTIMERr, timerval, DURATIONSELECTf)) / 8;

    return BCM_E_NONE;
}
#endif

#ifdef BCM_KATANA_SUPPORT
STATIC int
_bcm_kt_pkt_age_get(int unit, int *arg)
{
    uint32 timerval, limitval, mask;
    int i, loopcount;

    /* Check that all COSQs have the same value */
    SOC_IF_ERROR_RETURN(READ_PKTINTAGINGLIMIT0r(unit, &limitval));
    limitval &= 0xFFFFFF; /* AGINGLIMITPRIn fields only */
    loopcount = mask = soc_reg_field_get(unit, PKTINTAGINGLIMIT0r,
                                        limitval, AGINGLIMITPRI0f);
    /* Make mask of eight identical AGINGLIMITPRIn fields */
    for (i = 1; i < 8; i++) {
        mask = (mask << 3) | loopcount;
    }

    if (mask != limitval) {
        /* COSQs currently are not programmed identically */
        return BCM_E_CONFIG;
    }

    /* Check second register */
    SOC_IF_ERROR_RETURN(READ_PKTINTAGINGLIMIT1r(unit, &limitval));
    limitval &= 0xFFFFFF; /* AGINGLIMITPRIn fields only */
    if (mask != limitval) {
        /* COSQs currently are not programmed identically */
        return BCM_E_CONFIG;
    }

    /* Return the (common) setting */
    loopcount = 7 - loopcount;
    SOC_IF_ERROR_RETURN(READ_PKTINTAGINGTIMERr(unit, &timerval));

    *arg =
      (loopcount *
       soc_reg_field_get(unit, PKTINTAGINGTIMERr, timerval, DURATIONSELECTf)) / 8;

    return BCM_E_NONE;
}
#endif

STATIC int
_bcm_fb_er_color_mode_set(int unit, bcm_port_t port, int arg)
{
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT) \
 || defined(BCM_RAPTOR_SUPPORT)
    if (SOC_IS_FIREBOLT2(unit) || SOC_IS_TRX(unit) || SOC_IS_RAVEN(unit) || SOC_IS_HAWKEYE(unit)) {
        uint32               val, oval, rval, cfi_as_cng;

        BCM_IF_ERROR_RETURN
            (READ_EGR_VLAN_CONTROL_1r(unit, port, &val));

        oval = val;

        switch (arg) {
        case BCM_COLOR_PRIORITY:
            cfi_as_cng = 0;
            break;
        case BCM_COLOR_OUTER_CFI:
            cfi_as_cng = 0xf;
            break;
        case BCM_COLOR_INNER_CFI:
            cfi_as_cng = 0x1;
            break;
        default:
            return BCM_E_PARAM;
        }

        soc_reg_field_set(unit, EGR_VLAN_CONTROL_1r, &val,
                          CFI_AS_CNGf, cfi_as_cng);
        if (oval != val) {
            BCM_IF_ERROR_RETURN
                (_bcm_esw_port_config_set(unit, port,
                                          _bcmPortCfiAsCng,
                                          cfi_as_cng));
            /* Workaround for FB2 HW bug. For ports with DT_MODE = 0,
             * CNG is incorrectly picked from CNG_MAPr
             * instead of ING_PRI_CNG_MAP table.
             * GNATS 9151 */
            if (SOC_IS_FIREBOLT2(unit)) {
                BCM_IF_ERROR_RETURN(READ_ING_CONFIGr(unit, &rval));
                if((soc_reg_field_get(unit, ING_CONFIGr, rval,
                                      DT_MODEf) == 0) &&
                   (arg == BCM_COLOR_OUTER_CFI ||
                    arg == BCM_COLOR_INNER_CFI)) {
                    BCM_IF_ERROR_RETURN
                        (_bcm_esw_port_config_set(unit, port,
                                                  _bcmPortNni,
                                                  1));
                }
            }
            BCM_IF_ERROR_RETURN
                (WRITE_EGR_VLAN_CONTROL_1r(unit, port, val));
        }
    } else
#endif
    if (SOC_IS_HUMV(unit)) {
        return BCM_E_UNAVAIL;
    } else {
        uint32 val, oval, eval = 0, oeval;
        int inner_cfg = soc_feature(unit, soc_feature_color_inner_cfi);
        soc_reg_t cfg_reg, egr_cfg_reg;

        cfg_reg = ING_CONFIGr;
        egr_cfg_reg = EGR_CONFIGr;

        SOC_IF_ERROR_RETURN(soc_reg_read_any_block(unit, cfg_reg, &val));
        oval = val;
        if (egr_cfg_reg != INVALIDr) {
            SOC_IF_ERROR_RETURN
                (soc_reg_read_any_block(unit, egr_cfg_reg, &eval));
        }
        oeval = eval;
        switch (arg) {
        case BCM_COLOR_PRIORITY:
            soc_reg_field_set(unit, cfg_reg, &val, CFI_AS_CNGf, 0);
            if (inner_cfg) {
                 soc_reg_field_set(unit, cfg_reg, &val, CVLAN_CFI_AS_CNGf, 0);
            }
            if (egr_cfg_reg != INVALIDr) {
                soc_reg_field_set(unit, egr_cfg_reg, &eval, CFI_AS_CNGf, 0);
            }
            break;
        case BCM_COLOR_OUTER_CFI:
            soc_reg_field_set(unit, cfg_reg, &val, CFI_AS_CNGf, 1);
            if (inner_cfg) {
                soc_reg_field_set(unit, cfg_reg, &val, CVLAN_CFI_AS_CNGf, 0);
            }
            if (egr_cfg_reg != INVALIDr) {
                soc_reg_field_set(unit, egr_cfg_reg, &eval, CFI_AS_CNGf, 1);
            }
            break;
        case BCM_COLOR_INNER_CFI:
            if (inner_cfg) {
                soc_reg_field_set(unit, cfg_reg, &val, CFI_AS_CNGf, 0);
                soc_reg_field_set(unit, cfg_reg, &val, CVLAN_CFI_AS_CNGf, 1);
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        default:
            return BCM_E_PARAM;
        }
        if (oval != val) {
            SOC_IF_ERROR_RETURN(soc_reg_write_all_blocks(unit, cfg_reg, val));
        }
        if ((egr_cfg_reg != INVALIDr) && (oeval != eval)) {
            SOC_IF_ERROR_RETURN
                (soc_reg_write_all_blocks(unit, egr_cfg_reg, eval));
        }
    }
    return BCM_E_NONE;
}

STATIC int
_bcm_fb_er_color_mode_get(int unit, bcm_port_t port, int *arg)
{

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT) \
 || defined(BCM_RAPTOR_SUPPORT)
    if (SOC_IS_FIREBOLT2(unit) || SOC_IS_TRX(unit) || SOC_IS_RAVEN(unit) || SOC_IS_HAWKEYE(unit)) {
        uint32               val, cfi_as_cng;

        BCM_IF_ERROR_RETURN
            (READ_EGR_VLAN_CONTROL_1r(unit, port, &val));

        cfi_as_cng = soc_reg_field_get(unit, EGR_VLAN_CONTROL_1r, val,
                                       CFI_AS_CNGf);
        if (cfi_as_cng == 0) {
            *arg = BCM_COLOR_PRIORITY;
        } else if (cfi_as_cng == 1) {
            *arg = BCM_COLOR_INNER_CFI;
        } else {
            *arg = BCM_COLOR_OUTER_CFI;
        }
    } else
#endif
    {
        uint32 val;
        soc_reg_t cfg_reg;
        cfg_reg = ING_CONFIGr;

        SOC_IF_ERROR_RETURN(soc_reg_read_any_block(unit, cfg_reg, &val));

        
        if (soc_reg_field_get(unit, cfg_reg, val, CFI_AS_CNGf)) {
            *arg = BCM_COLOR_OUTER_CFI;
        } else if (soc_feature(unit, soc_feature_color_inner_cfi) &&
                   soc_reg_field_get(unit, cfg_reg, val, CVLAN_CFI_AS_CNGf)) {
            *arg = BCM_COLOR_INNER_CFI;
        } else {
            *arg = BCM_COLOR_PRIORITY;
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_fb_er_hashselect_set
 * Description:
 *      Set the Hash Select for L2, L3 or Multipath type.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to set.
 *      arg  - BCM_HASH_*
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_fb_er_hashselect_set(int unit, bcm_switch_control_t type, int arg)
{
    uint32      hash_control, val=0;
    soc_field_t field;
    soc_reg_t   hash_reg = HASH_CONTROLr;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
        defined(BCM_TRX_SUPPORT)
    int         dual = FALSE;
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */

    switch (type) {
        case bcmSwitchHashL2:
            field = L2_AND_VLAN_MAC_HASH_SELECTf;
            if (!soc_reg_field_valid(unit, hash_reg, field)) {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashL3:
            if (soc_reg_field_valid(unit, HASH_CONTROLr, L3_HASH_SELECTf) &&
                soc_feature(unit, soc_feature_l3)) {
                field = L3_HASH_SELECTf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashMultipath:
            if (soc_reg_field_valid(unit, HASH_CONTROLr, ECMP_HASH_SELf) &&
                soc_feature(unit, soc_feature_l3)) {
                field = ECMP_HASH_SELf;
                if ((arg == BCM_HASH_CRC16L) || (arg == BCM_HASH_CRC16U)) {
                    return BCM_E_PARAM;
                }
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
        defined(BCM_TRX_SUPPORT)
        case bcmSwitchHashL2Dual:
            dual = TRUE;
            hash_reg = L2_AUX_HASH_CONTROLr;
            field = HASH_SELECTf;
            if (!soc_reg_field_valid(unit, hash_reg, field)) {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashL3Dual:
            if (soc_reg_field_valid(unit, L3_AUX_HASH_CONTROLr, HASH_SELECTf) &&
                soc_feature(unit, soc_feature_l3)) {
                dual = TRUE;
                hash_reg = L3_AUX_HASH_CONTROLr;
                field = HASH_SELECTf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */
#if defined(BCM_TRIUMPH_SUPPORT)
        case bcmSwitchHashIpfixIngress:
            if (soc_reg_field_valid(unit, ING_IPFIX_HASH_CONTROLr, HASH_SELECT_Af)) {
                hash_reg = ING_IPFIX_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashIpfixEgress:
            if (soc_reg_field_valid(unit, EGR_IPFIX_HASH_CONTROLr, HASH_SELECT_Af)) {
                hash_reg = EGR_IPFIX_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashIpfixIngressDual:
            if (soc_reg_field_valid(unit, ING_IPFIX_HASH_CONTROLr, HASH_SELECT_Bf)) {
                hash_reg = ING_IPFIX_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashIpfixEgressDual:
            if (soc_reg_field_valid(unit, EGR_IPFIX_HASH_CONTROLr, HASH_SELECT_Bf)) {
                hash_reg = EGR_IPFIX_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashMPLS:
            if (soc_reg_field_valid(unit, MPLS_ENTRY_HASH_CONTROLr, HASH_SELECT_Af)) {
                hash_reg = MPLS_ENTRY_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashMPLSDual:
            if (soc_reg_field_valid(unit, MPLS_ENTRY_HASH_CONTROLr, HASH_SELECT_Bf)) {
                hash_reg = MPLS_ENTRY_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#else
        case bcmSwitchHashIpfixIngress:
        case bcmSwitchHashIpfixEgress:
        case bcmSwitchHashIpfixIngressDual:
        case bcmSwitchHashIpfixEgressDual:
        case bcmSwitchHashMPLS:
        case bcmSwitchHashMPLSDual:
            return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
        case bcmSwitchHashVlanTranslate:
            if (soc_reg_field_valid(unit, VLAN_XLATE_HASH_CONTROLr, HASH_SELECT_Af)) {
                hash_reg = VLAN_XLATE_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashVlanTranslateDual:
            if (soc_reg_field_valid(unit, VLAN_XLATE_HASH_CONTROLr, HASH_SELECT_Bf)) {
                hash_reg = VLAN_XLATE_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashEgressVlanTranslate:
            if (soc_reg_field_valid(unit, EGR_VLAN_XLATE_HASH_CONTROLr, HASH_SELECT_Af)) {
                hash_reg = EGR_VLAN_XLATE_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashEgressVlanTranslateDual:
            if (soc_reg_field_valid(unit, EGR_VLAN_XLATE_HASH_CONTROLr, HASH_SELECT_Bf)) {
                hash_reg = EGR_VLAN_XLATE_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#else
        case bcmSwitchHashVlanTranslate:
        case bcmSwitchHashVlanTranslateDual:
        case bcmSwitchHashEgressVlanTranslate:
        case bcmSwitchHashEgressVlanTranslateDual:
            return BCM_E_UNAVAIL;
#endif /* BCM_TRX_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchHashWlanPort:
            if (soc_feature(unit, soc_feature_wlan) &&
               (soc_reg_field_valid(unit, AXP_WRX_SVP_HASH_CTRLr, SELECT_Af))) {
                hash_reg = AXP_WRX_SVP_HASH_CTRLr;
                field = SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashWlanPortDual:
            if (soc_feature(unit, soc_feature_wlan) &&
               (soc_reg_field_valid(unit, AXP_WRX_SVP_HASH_CTRLr, SELECT_Bf))) {
                hash_reg = AXP_WRX_SVP_HASH_CTRLr;
                field = SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashWlanClient:
            if (soc_feature(unit, soc_feature_wlan) &&
               (soc_reg_field_valid(unit, AXP_WRX_WCD_HASH_CTRLr, SELECT_Af))) {
                hash_reg =  AXP_WRX_WCD_HASH_CTRLr;
                field = SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashWlanClientDual:
            if (soc_feature(unit, soc_feature_wlan) &&
               (soc_reg_field_valid(unit, AXP_WRX_WCD_HASH_CTRLr, SELECT_Bf))) {
                hash_reg =  AXP_WRX_WCD_HASH_CTRLr;
                field = SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashRegexAction:
            if (soc_feature(unit, soc_feature_regex) &&
               (soc_reg_field_valid(unit, FT_HASH_CONTROLr, HASH_SELECT_Af))) {
                hash_reg = AXP_WRX_SVP_HASH_CTRLr;
                field = SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashRegexActionDual:
            if (soc_feature(unit, soc_feature_regex) &&
               (soc_reg_field_valid(unit, FT_HASH_CONTROLr, HASH_SELECT_Bf))) {
                hash_reg = AXP_WRX_SVP_HASH_CTRLr;
                field = SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#else
        case bcmSwitchHashWlanPort:
        case bcmSwitchHashWlanPortDual:
        case bcmSwitchHashWlanClient:
        case bcmSwitchHashWlanClientDual:
        case bcmSwitchHashRegexAction:
        case bcmSwitchHashRegexActionDual:
            return BCM_E_UNAVAIL;
#endif /* defined(BCM_TRIUMPH3_SUPPORT) */
#if defined(BCM_TRIDENT2_SUPPORT)
        case bcmSwitchHashL3DNATPool:
            if (soc_feature(unit, soc_feature_nat) && soc_reg_field_valid(unit,
                        ING_DNAT_ADDRESS_TYPE_HASH_CONTROLr, HASH_SELECT_Af)) {
                hash_reg = ING_DNAT_ADDRESS_TYPE_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashL3DNATPoolDual:
            if (soc_feature(unit, soc_feature_nat) && soc_reg_field_valid(unit,
                        ING_DNAT_ADDRESS_TYPE_HASH_CONTROLr, HASH_SELECT_Bf)) {
                hash_reg = ING_DNAT_ADDRESS_TYPE_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashVpVlanMemberIngress:
        case bcmSwitchHashVpVlanMemberIngressDual:
            if (soc_feature(unit, soc_feature_ing_vp_vlan_membership)) {
                hash_reg = ING_VP_VLAN_MEMBERSHIP_HASH_CONTROLr;
                field = type == bcmSwitchHashVpVlanMemberIngress?
                           HASH_SELECT_Af: HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashVpVlanMemberEgress:
        case bcmSwitchHashVpVlanMemberEgressDual:
            if (soc_feature(unit, soc_feature_egr_vp_vlan_membership)) {
                hash_reg = EGR_VP_VLAN_MEMBERSHIP_HASH_CONTROLr;
                field = type == bcmSwitchHashVpVlanMemberEgress?
                           HASH_SELECT_Af: HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashL2Endpoint:
        case bcmSwitchHashL2EndpointDual:
            if (SOC_REG_IS_VALID(unit, L2_ENDPOINT_ID_HASH_CONTROLr)) {
                hash_reg = L2_ENDPOINT_ID_HASH_CONTROLr;
                field = (type == bcmSwitchHashL2Endpoint) ?
                        HASH_SELECT_Af: HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashEndpointQueueMap:
        case bcmSwitchHashEndpointQueueMapDual:
            if (SOC_REG_IS_VALID(unit, ENDPOINT_QUEUE_MAP_HASH_CONTROLr)) {
                hash_reg = ENDPOINT_QUEUE_MAP_HASH_CONTROLr;
                field = (type == bcmSwitchHashEndpointQueueMap) ?
                        HASH_SELECT_Af: HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
        case bcmSwitchHashOamEgress:
            if (soc_reg_field_valid(unit, EGR_MP_GROUP_HASH_CONTROLr, 
                                    HASH_SELECT_Af)) {
                hash_reg = EGR_MP_GROUP_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashOamEgressDual:
            if (soc_reg_field_valid(unit, EGR_MP_GROUP_HASH_CONTROLr, 
                                    HASH_SELECT_Bf)) {
                hash_reg = EGR_MP_GROUP_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif /* BCM_KATANA2_SUPPORT */
        default:
            return BCM_E_PARAM;
    }

    switch (arg) {
        case BCM_HASH_ZERO:
            val = FB_HASH_ZERO;
            break;
        case BCM_HASH_LSB:
            val = FB_HASH_LSB;
            break;
        case BCM_HASH_CRC16L:
            val = FB_HASH_CRC16_LOWER;
            break;
        case BCM_HASH_CRC16U:
            val = FB_HASH_CRC16_UPPER;
            break;
        case BCM_HASH_CRC32L:
            val = FB_HASH_CRC32_LOWER;
            break;
        case BCM_HASH_CRC32U:
            val = FB_HASH_CRC32_UPPER;
            break;
        default:
            return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN
        (soc_reg_read_any_block(unit, hash_reg, &hash_control));
    soc_reg_field_set(unit, hash_reg, &hash_control, field, val);
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
        defined(BCM_TRX_SUPPORT)
    if (dual) {
        uint32 base_hash_control, base_hash;
        soc_field_t base_field;

        base_field = (type == bcmSwitchHashL2Dual) ?
            L2_AND_VLAN_MAC_HASH_SELECTf : L3_HASH_SELECTf;

        BCM_IF_ERROR_RETURN(READ_HASH_CONTROLr(unit, &base_hash_control));
        base_hash = soc_reg_field_get(unit, HASH_CONTROLr,
                                      base_hash_control, base_field);
        soc_reg_field_set(unit, hash_reg, &hash_control,
                          ENABLEf, (base_hash == val) ? 0 : 1);
#if defined (BCM_RAVEN_SUPPORT)
        /* Also write to VLAN_MAC_AUX_HASH_CONTROL for bcmSwitchHashL2Dual */
        if (type == bcmSwitchHashL2Dual &&
            SOC_REG_IS_VALID(unit, VLAN_MAC_AUX_HASH_CONTROLr)) {
            SOC_IF_ERROR_RETURN
                (READ_VLAN_MAC_AUX_HASH_CONTROLr(unit, &hash_control));
            soc_reg_field_set(unit, VLAN_MAC_AUX_HASH_CONTROLr, &hash_control,
                                HASH_SELECTf, val);
            soc_reg_field_set(unit, VLAN_MAC_AUX_HASH_CONTROLr, &hash_control,
                              ENABLEf, (base_hash == val) ? 0 : 1);
            SOC_IF_ERROR_RETURN
                (WRITE_VLAN_MAC_AUX_HASH_CONTROLr(unit, hash_control));
        }
#endif /* BCM_RAVEN_SUPPORT */
    }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */
    SOC_IF_ERROR_RETURN
        (soc_reg_write_all_blocks(unit, hash_reg, hash_control));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_fb_er_hashselect_get
 * Description:
 *      Get the current Hash Select settings. Value returned
 *      is of the form BCM_HASH_*.
 *      All switch chip ports are configured with the same settings.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to retrieve.
 *      arg - Pointer to where the retrieved value will be written.
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_fb_er_hashselect_get(int unit, bcm_switch_control_t type, int *arg)
{
    uint32      hash_control, val=0;
    soc_reg_t   hash_reg = HASH_CONTROLr;
    soc_field_t field;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
        defined(BCM_TRX_SUPPORT)
    int         dual = FALSE;
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */

    switch (type) {
        case bcmSwitchHashL2:
            field = L2_AND_VLAN_MAC_HASH_SELECTf;
            if (!soc_reg_field_valid(unit, hash_reg, field)) {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashL3:
            if (soc_reg_field_valid(unit, HASH_CONTROLr, L3_HASH_SELECTf) &&
                soc_feature(unit, soc_feature_l3)) {
                field = L3_HASH_SELECTf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashMultipath:
            if (soc_reg_field_valid(unit, HASH_CONTROLr, ECMP_HASH_SELf) &&
                soc_feature(unit, soc_feature_l3)) {
                field = ECMP_HASH_SELf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
        defined(BCM_TRX_SUPPORT)
        case bcmSwitchHashL2Dual:
            dual = TRUE;
            hash_reg = L2_AUX_HASH_CONTROLr;
            field = HASH_SELECTf;
            if (!soc_reg_field_valid(unit, hash_reg, field)) {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashL3Dual:
            if (soc_reg_field_valid(unit, L3_AUX_HASH_CONTROLr, HASH_SELECTf) &&
                soc_feature(unit, soc_feature_l3)) {
                dual = TRUE;
                hash_reg = L3_AUX_HASH_CONTROLr;
                field = HASH_SELECTf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */
#if defined(BCM_TRIUMPH_SUPPORT)
        case bcmSwitchHashIpfixIngress:
            if (soc_reg_field_valid(unit, ING_IPFIX_HASH_CONTROLr, HASH_SELECT_Af) ) {
                hash_reg = ING_IPFIX_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashIpfixEgress:
            if (soc_reg_field_valid(unit, EGR_IPFIX_HASH_CONTROLr, HASH_SELECT_Af) ) {
                hash_reg = EGR_IPFIX_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashIpfixIngressDual:
            if (soc_reg_field_valid(unit, ING_IPFIX_HASH_CONTROLr, HASH_SELECT_Bf) ) {
                hash_reg = ING_IPFIX_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashIpfixEgressDual:
            if (soc_reg_field_valid(unit, EGR_IPFIX_HASH_CONTROLr, HASH_SELECT_Bf) ) {
                hash_reg = EGR_IPFIX_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashMPLS:
            if (soc_reg_field_valid(unit, MPLS_ENTRY_HASH_CONTROLr, HASH_SELECT_Af)) {
                hash_reg = MPLS_ENTRY_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashMPLSDual:
            if (soc_reg_field_valid(unit, MPLS_ENTRY_HASH_CONTROLr, HASH_SELECT_Bf)) {
                hash_reg = MPLS_ENTRY_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#else
        case bcmSwitchHashIpfixIngress:
        case bcmSwitchHashIpfixEgress:
        case bcmSwitchHashIpfixIngressDual:
        case bcmSwitchHashIpfixEgressDual:
        case bcmSwitchHashMPLS:
        case bcmSwitchHashMPLSDual:
            return BCM_E_UNAVAIL;
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
        case bcmSwitchHashVlanTranslate:
            if (soc_reg_field_valid(unit, VLAN_XLATE_HASH_CONTROLr, HASH_SELECT_Af)) {
                hash_reg = VLAN_XLATE_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashVlanTranslateDual:
            if (soc_reg_field_valid(unit, VLAN_XLATE_HASH_CONTROLr, HASH_SELECT_Bf)) {
                hash_reg = VLAN_XLATE_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashEgressVlanTranslate:
            if (soc_reg_field_valid(unit, EGR_VLAN_XLATE_HASH_CONTROLr, HASH_SELECT_Af)) {
                hash_reg = EGR_VLAN_XLATE_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashEgressVlanTranslateDual:
            if (soc_reg_field_valid(unit, EGR_VLAN_XLATE_HASH_CONTROLr, HASH_SELECT_Bf)) {
                hash_reg = EGR_VLAN_XLATE_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#else
        case bcmSwitchHashVlanTranslate:
        case bcmSwitchHashVlanTranslateDual:
        case bcmSwitchHashEgressVlanTranslate:
        case bcmSwitchHashEgressVlanTranslateDual:
            return BCM_E_UNAVAIL;
#endif /* BCM_TRX_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchHashWlanPort:
            if (soc_feature(unit, soc_feature_wlan) &&
               (soc_reg_field_valid(unit, AXP_WRX_SVP_HASH_CTRLr, SELECT_Af))) {
                hash_reg = AXP_WRX_SVP_HASH_CTRLr;
                field = SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashWlanPortDual:
            if (soc_feature(unit, soc_feature_wlan) &&
               (soc_reg_field_valid(unit, AXP_WRX_SVP_HASH_CTRLr, SELECT_Bf))) {
                hash_reg = AXP_WRX_SVP_HASH_CTRLr;
                field = SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashWlanClient:
            if (soc_feature(unit, soc_feature_wlan) &&
               (soc_reg_field_valid(unit, AXP_WRX_WCD_HASH_CTRLr, SELECT_Af))) {
                hash_reg =  AXP_WRX_WCD_HASH_CTRLr;
                field = SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashWlanClientDual:
            if (soc_feature(unit, soc_feature_wlan) &&
               (soc_reg_field_valid(unit, AXP_WRX_WCD_HASH_CTRLr, SELECT_Bf))) {
                hash_reg =  AXP_WRX_WCD_HASH_CTRLr;
                field = SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashRegexAction:
            if (soc_feature(unit, soc_feature_regex) &&
               (soc_reg_field_valid(unit, FT_HASH_CONTROLr, HASH_SELECT_Af))) {
                hash_reg = AXP_WRX_SVP_HASH_CTRLr;
                field = SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashRegexActionDual:
            if (soc_feature(unit, soc_feature_regex) &&
               (soc_reg_field_valid(unit, FT_HASH_CONTROLr, HASH_SELECT_Bf))) {
                hash_reg = AXP_WRX_SVP_HASH_CTRLr;
                field = SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#else
        case bcmSwitchHashWlanPort:
        case bcmSwitchHashWlanPortDual:
        case bcmSwitchHashWlanClient:
        case bcmSwitchHashWlanClientDual:
        case bcmSwitchHashRegexAction:
        case bcmSwitchHashRegexActionDual:
            return BCM_E_UNAVAIL;
#endif /* defined(BCM_TRIUMPH3_SUPPORT) */
#if defined(BCM_TRIDENT2_SUPPORT)
        case bcmSwitchHashL3DNATPool:
            if (soc_feature(unit, soc_feature_nat) && soc_reg_field_valid(unit,
                        ING_DNAT_ADDRESS_TYPE_HASH_CONTROLr, HASH_SELECT_Af)) {
                hash_reg = ING_DNAT_ADDRESS_TYPE_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashL3DNATPoolDual:
            if (soc_feature(unit, soc_feature_nat) && soc_reg_field_valid(unit,
                        ING_DNAT_ADDRESS_TYPE_HASH_CONTROLr, HASH_SELECT_Bf)) {
                hash_reg = ING_DNAT_ADDRESS_TYPE_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashVpVlanMemberIngress:
        case bcmSwitchHashVpVlanMemberIngressDual:
            if (soc_feature(unit, soc_feature_ing_vp_vlan_membership)) {
                hash_reg = ING_VP_VLAN_MEMBERSHIP_HASH_CONTROLr;
                field = type == bcmSwitchHashVpVlanMemberIngress?
                           HASH_SELECT_Af: HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashVpVlanMemberEgress:
        case bcmSwitchHashVpVlanMemberEgressDual:
            if (soc_feature(unit, soc_feature_egr_vp_vlan_membership)) {
                hash_reg = EGR_VP_VLAN_MEMBERSHIP_HASH_CONTROLr;
                field = type == bcmSwitchHashVpVlanMemberEgress?
                           HASH_SELECT_Af: HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashL2Endpoint:
        case bcmSwitchHashL2EndpointDual:
            if (SOC_REG_IS_VALID(unit, L2_ENDPOINT_ID_HASH_CONTROLr)) {
                hash_reg = L2_ENDPOINT_ID_HASH_CONTROLr;
                field = (type == bcmSwitchHashL2Endpoint) ?
                        HASH_SELECT_Af: HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashEndpointQueueMap:
        case bcmSwitchHashEndpointQueueMapDual:
            if (SOC_REG_IS_VALID(unit, ENDPOINT_QUEUE_MAP_HASH_CONTROLr)) {
                hash_reg = ENDPOINT_QUEUE_MAP_HASH_CONTROLr;
                field = (type == bcmSwitchHashEndpointQueueMap) ?
                        HASH_SELECT_Af: HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
        case bcmSwitchHashOamEgress:
            if (soc_reg_field_valid(unit, EGR_MP_GROUP_HASH_CONTROLr, 
                                    HASH_SELECT_Af)) {
                hash_reg = EGR_MP_GROUP_HASH_CONTROLr;
                field = HASH_SELECT_Af;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashOamEgressDual:
            if (soc_reg_field_valid(unit, EGR_MP_GROUP_HASH_CONTROLr, 
                                    HASH_SELECT_Bf)) {
                hash_reg = EGR_MP_GROUP_HASH_CONTROLr;
                field = HASH_SELECT_Bf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif /* BCM_KATANA2_SUPPORT */
        default:
            return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN
        (soc_reg_read_any_block(unit, hash_reg, &hash_control));
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
        defined(BCM_TRX_SUPPORT)
    if (dual &&
        !soc_reg_field_get(unit, hash_reg, hash_control, ENABLEf)) {
        /* Dual hash not enabled, just return primary hash */
        field = (type == bcmSwitchHashL2Dual) ?
            L2_AND_VLAN_MAC_HASH_SELECTf : L3_HASH_SELECTf;
        hash_reg = HASH_CONTROLr;
        BCM_IF_ERROR_RETURN(READ_HASH_CONTROLr(unit, &hash_control));
    }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */
    val = soc_reg_field_get(unit, hash_reg, hash_control, field);

    switch (val) {
        case FB_HASH_ZERO:
            *arg = BCM_HASH_ZERO;
            break;
        case FB_HASH_LSB:
            *arg = BCM_HASH_LSB;
            break;
        case FB_HASH_CRC16_LOWER:
            *arg = BCM_HASH_CRC16L;
            break;
        case FB_HASH_CRC16_UPPER:
            *arg = BCM_HASH_CRC16U;
            break;
        case FB_HASH_CRC32_LOWER:
            *arg = BCM_HASH_CRC32L;
            break;
        case FB_HASH_CRC32_UPPER:
            *arg = BCM_HASH_CRC32U;
            break;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_hashcontrol_set
 * Description:
 *      Set the Hash Control for L2, L3 or Multipath type.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      arg  - BCM_HASH_CONTROL*
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_hashcontrol_set(int unit, int arg)
{
    uint32      hash_control, val=0;

    if (!soc_feature(unit, soc_feature_l3)) {
        if ((arg & BCM_HASH_CONTROL_MULTIPATH_L4PORTS) ||
            (arg & BCM_HASH_CONTROL_MULTIPATH_DIP)) {
            return BCM_E_UNAVAIL;
        }
    }

    BCM_IF_ERROR_RETURN(READ_HASH_CONTROLr(unit, &hash_control));

    val = (arg & BCM_HASH_CONTROL_MULTIPATH_L4PORTS) ? 1 : 0;
    soc_reg_field_set(unit, HASH_CONTROLr, &hash_control,
                          USE_TCP_UDP_PORTSf, val);

    if (soc_reg_field_valid(unit, HASH_CONTROLr, ECMP_HASH_USE_DIPf)) {
        val = (arg & BCM_HASH_CONTROL_MULTIPATH_DIP) ? 1 : 0;
        soc_reg_field_set(unit, HASH_CONTROLr, &hash_control,
                          ECMP_HASH_USE_DIPf, val);

        val = BCM_HASH_CONTROL_MULTIPATH_USERDEF_VAL(arg);
        soc_reg_field_set(unit, HASH_CONTROLr, &hash_control,
                          ECMP_HASH_UDFf, val);
    }

    val = (arg & BCM_HASH_CONTROL_TRUNK_UC_XGS2) ? 1 : 0;
    soc_reg_field_set(unit, HASH_CONTROLr, &hash_control,
                      ENABLE_DRACO1_5_HASHf, val);

    val = (arg & BCM_HASH_CONTROL_TRUNK_UC_SRCPORT) ? 1 : 0;
    soc_reg_field_set(unit, HASH_CONTROLr, &hash_control,
                      UC_TRUNK_HASH_USE_SRC_PORTf, val);

    val = (arg & BCM_HASH_CONTROL_TRUNK_NUC_DST) ? 1 : 0;
    soc_reg_field_set(unit, HASH_CONTROLr, &hash_control,
                      NON_UC_TRUNK_HASH_DST_ENABLEf, val);

    val = (arg & BCM_HASH_CONTROL_TRUNK_NUC_SRC) ? 1 : 0;
    soc_reg_field_set(unit, HASH_CONTROLr, &hash_control,
                      NON_UC_TRUNK_HASH_SRC_ENABLEf, val);

    val = (arg & BCM_HASH_CONTROL_TRUNK_NUC_MODPORT) ? 1 : 0;
    soc_reg_field_set(unit, HASH_CONTROLr, &hash_control,
                      NON_UC_TRUNK_HASH_MOD_PORT_ENABLEf, val);

#ifdef BCM_BRADLEY_SUPPORT
    if (SOC_IS_HB_GW(unit) || (SOC_IS_TRX(unit) && !SOC_IS_HURRICANEX(unit))) {
        val = (arg & BCM_HASH_CONTROL_ECMP_ENHANCE) ? 1 : 0;
        soc_reg_field_set(unit, HASH_CONTROLr, &hash_control,
                          ECMP_HASH_USE_RTAG7f, val);

        val = (arg & BCM_HASH_CONTROL_TRUNK_NUC_ENHANCE) ? 1 : 0;
        soc_reg_field_set(unit, HASH_CONTROLr, &hash_control,
                          NON_UC_TRUNK_HASH_USE_RTAG7f, val);
    }
#endif /* BCM_BRADLEY_SUPPORT */

    BCM_IF_ERROR_RETURN(WRITE_HASH_CONTROLr(unit, hash_control));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_hashcontrol_get
 * Description:
 *      Get the current Hash Control settings.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      arg - Pointer to where the retrieved value will be written.
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_hashcontrol_get(int unit, int *arg)
{
    uint32      hash_control, val=0;

    *arg = 0;
    BCM_IF_ERROR_RETURN(READ_HASH_CONTROLr(unit, &hash_control));


    if (soc_feature(unit, soc_feature_l3)) {
        if (soc_reg_field_valid(unit, HASH_CONTROLr, USE_TCP_UDP_PORTSf)) {
            val = soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                                    USE_TCP_UDP_PORTSf);
            if (val) *arg |= BCM_HASH_CONTROL_MULTIPATH_L4PORTS;
        }
        if (soc_reg_field_valid(unit, HASH_CONTROLr, ECMP_HASH_USE_DIPf)) {
            val = soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                                    ECMP_HASH_USE_DIPf);
            if (val) *arg |= BCM_HASH_CONTROL_MULTIPATH_DIP;
        }

        if (soc_reg_field_valid(unit, HASH_CONTROLr, ECMP_HASH_UDFf)) {
            val = soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                                    ECMP_HASH_UDFf);
            *arg |= BCM_HASH_CONTROL_MULTIPATH_USERDEF(val);
        }
    }

    val = soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                            ENABLE_DRACO1_5_HASHf);
    if (val) *arg |= BCM_HASH_CONTROL_TRUNK_UC_XGS2;

    val = soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                            UC_TRUNK_HASH_USE_SRC_PORTf);
    if (val) *arg |= BCM_HASH_CONTROL_TRUNK_UC_SRCPORT;

    val = soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                            NON_UC_TRUNK_HASH_DST_ENABLEf);
    if (val) *arg |= BCM_HASH_CONTROL_TRUNK_NUC_DST;

    val = soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                            NON_UC_TRUNK_HASH_SRC_ENABLEf);
    if (val) *arg |= BCM_HASH_CONTROL_TRUNK_NUC_SRC;

    val = soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                            NON_UC_TRUNK_HASH_MOD_PORT_ENABLEf);
    if (val) *arg |= BCM_HASH_CONTROL_TRUNK_NUC_MODPORT;

#ifdef BCM_BRADLEY_SUPPORT
    if (SOC_IS_HB_GW(unit) || (SOC_IS_TRX(unit) && !SOC_IS_HURRICANEX(unit))) {
        val = soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                                ECMP_HASH_USE_RTAG7f);
        if (val) *arg |= BCM_HASH_CONTROL_ECMP_ENHANCE;

        val = soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                                NON_UC_TRUNK_HASH_USE_RTAG7f);
        if (val) *arg |= BCM_HASH_CONTROL_TRUNK_NUC_ENHANCE;
    }
#endif /* BCM_BRADLEY_SUPPORT */

    return BCM_E_NONE;
}

/* IGMP/MLD values */
#define BCM_SWITCH_FORWARD_VALUE        0
#define BCM_SWITCH_DROP_VALUE           1
#define BCM_SWITCH_FLOOD_VALUE          2
#define BCM_SWITCH_RESERVED_VALUE       3

#ifdef BCM_TRIUMPH2_SUPPORT
STATIC int
_bcm_tr2_protocol_pkt_index_get(int unit, bcm_port_t port, int *index)
{
    port_tab_entry_t entry;
    soc_mem_t mem;
    int port_index;

    if (!soc_mem_field_valid(unit, PORT_TABm, PROTOCOL_PKT_INDEXf)) {
        return BCM_E_INTERNAL;
    }

    mem = PORT_TABm;
    port_index = port;
    if (IS_CPU_PORT(unit, port)) {
        if (SOC_MEM_IS_VALID(unit, IPORT_TABLEm)) {
            mem = IPORT_TABLEm;
        } else {
            port_index = SOC_IS_KATANA2(unit) ?
                         SOC_INFO(unit).cpu_hg_pp_port_index :
                         SOC_INFO(unit).cpu_hg_index;
        }
    }
    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, mem, MEM_BLOCK_ANY, port_index, &entry));
    *index = soc_mem_field32_get(unit, mem, &entry, PROTOCOL_PKT_INDEXf);
    return BCM_E_NONE;
}

STATIC int
_bcm_tr2_prot_pkt_profile_set(int unit, soc_reg_t reg, bcm_port_t port,
                              int count, soc_field_t *fields, uint32 *values)
{
    soc_mem_t mem;
    port_tab_entry_t entry;
    int port_index, i;
    uint32 prot_pkt_ctrl, igmp_mld_pkt_ctrl, *rval_ptr;
    uint32 old_index, index;

    if (reg == PROTOCOL_PKT_CONTROLr) {
        rval_ptr = &prot_pkt_ctrl;
    } else if (reg == IGMP_MLD_PKT_CONTROLr) {
        rval_ptr = &igmp_mld_pkt_ctrl;
    } else {
        return BCM_E_INTERNAL;
    }

    mem = PORT_TABm;
    port_index = port;
    if (IS_CPU_PORT(unit, port)) {
        if (SOC_MEM_IS_VALID(unit, IPORT_TABLEm)) {
            mem = IPORT_TABLEm;
        } else {
            port_index = SOC_IS_KATANA2(unit) ?
                         SOC_INFO(unit).cpu_hg_pp_port_index :
                         SOC_INFO(unit).cpu_hg_index;
        }
    }
    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, mem, MEM_BLOCK_ANY, port_index, &entry));
    old_index = soc_mem_field32_get(unit, mem, &entry, PROTOCOL_PKT_INDEXf);

    BCM_IF_ERROR_RETURN
        (_bcm_prot_pkt_ctrl_get(unit, old_index, &prot_pkt_ctrl,
                                &igmp_mld_pkt_ctrl));
    for (i = 0; i < count; i++) {
        soc_reg_field_set(unit, reg, rval_ptr, fields[i], values[i]);
    }
    BCM_IF_ERROR_RETURN
        (_bcm_prot_pkt_ctrl_add(unit, prot_pkt_ctrl, igmp_mld_pkt_ctrl,
                                &index));
    BCM_IF_ERROR_RETURN(_bcm_prot_pkt_ctrl_delete(unit, old_index));
    return soc_mem_field32_modify(unit, PORT_TABm, port_index,
                                  PROTOCOL_PKT_INDEXf, index);
}

STATIC int
_bcm_tr2_prot_pkt_action_set(int unit,
                             bcm_port_t port,
                             bcm_switch_control_t type,
                             int arg)
{
    soc_field_t field;
    uint32 value;

    value = arg ? 1 : 0;

    switch(type) {
    case bcmSwitchArpReplyToCpu:
        field = ARP_REPLY_TO_CPUf;
        break;
    case bcmSwitchArpReplyDrop:
        field = ARP_REPLY_DROPf;
        break;
    case bcmSwitchArpRequestToCpu:
        field = ARP_REQUEST_TO_CPUf;
        break;
    case bcmSwitchArpRequestDrop:
        field = ARP_REQUEST_DROPf;
        break;
    case bcmSwitchNdPktToCpu:
        field = ND_PKT_TO_CPUf;
        break;
    case bcmSwitchNdPktDrop:
        field = ND_PKT_DROPf;
        break;
    case bcmSwitchDhcpPktToCpu:
        field = DHCP_PKT_TO_CPUf;
        break;
    case bcmSwitchDhcpPktDrop:
        field = DHCP_PKT_DROPf;
        break;
    default:
        return BCM_E_INTERNAL;
    }

    return _bcm_tr2_prot_pkt_profile_set(unit, PROTOCOL_PKT_CONTROLr, port, 1,
                                         &field, &value);
}
#endif

/*
 * Function:
 *      _bcm_xgs3_igmp_action_set
 * Description:
 *      Set the IGMP/MLD registers
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to set.
 *      arg  - IGMP / MLD actions
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_igmp_action_set(int unit,
                         bcm_port_t port,
                         bcm_switch_control_t type,
                         int arg)
{
    uint32      values[3];
    soc_field_t fields[3];
    int         idx;
    soc_reg_t   reg = INVALIDr ;
    int         fcount = 1;
    int         value = (arg) ? 1 : 0;

    for (idx = 0; idx < COUNTOF(values); idx++) {
        values[idx] = value;
        fields[idx] = INVALIDf;
    }
#if defined (BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || \
    defined (BCM_TRX_SUPPORT)
    if (soc_feature(unit, soc_feature_igmp_mld_support)) {
        reg = IGMP_MLD_PKT_CONTROLr;
        /* Given control type select register field. */
        switch (type) {
            case bcmSwitchIgmpPktToCpu:
                fields[0] = IGMP_REP_LEAVE_TO_CPUf;
                fields[1] = IGMP_QUERY_TO_CPUf;
                fields[2] = IGMP_UNKNOWN_MSG_TO_CPUf;
                fcount = 3;
                break;
            case bcmSwitchIgmpPktDrop:
                fields[0] = IGMP_REP_LEAVE_FWD_ACTIONf;
                fields[1] = IGMP_QUERY_FWD_ACTIONf;
                fields[2] = IGMP_UNKNOWN_MSG_FWD_ACTIONf;
                fcount = 3;
                values[0] = values[1] = values[2] =
                    (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchMldPktToCpu:
                fields[0] = MLD_REP_DONE_TO_CPUf;
                fields[1] = MLD_QUERY_TO_CPUf;
                fcount = 2;
                break;
            case bcmSwitchMldPktDrop:
                fields[0] = MLD_REP_DONE_FWD_ACTIONf;
                fields[1] = MLD_QUERY_FWD_ACTIONf;
                fcount = 2;
                values[0] =
                    (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchV4ResvdMcPktToCpu:
                fields[0] = IPV4_RESVD_MC_PKT_TO_CPUf;
                break;
            case bcmSwitchV4ResvdMcPktFlood:
                fields[0] = IPV4_RESVD_MC_PKT_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchV4ResvdMcPktDrop:
                fields[0] = IPV4_RESVD_MC_PKT_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchV6ResvdMcPktToCpu:
                fields[0] = IPV6_RESVD_MC_PKT_TO_CPUf;
                break;
            case bcmSwitchV6ResvdMcPktFlood:
                fields[0] = IPV6_RESVD_MC_PKT_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchV6ResvdMcPktDrop:
                fields[0] = IPV6_RESVD_MC_PKT_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchIgmpReportLeaveDrop:
                fields[0] = IGMP_REP_LEAVE_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchIgmpReportLeaveFlood:
                fields[0] = IGMP_REP_LEAVE_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchIgmpReportLeaveToCpu:
                fields[0] = IGMP_REP_LEAVE_TO_CPUf;
                break;
            case bcmSwitchIgmpQueryDrop:
                fields[0] = IGMP_QUERY_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchIgmpQueryFlood:
                fields[0] = IGMP_QUERY_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchIgmpQueryToCpu:
                fields[0] = IGMP_QUERY_TO_CPUf;
                break;
            case bcmSwitchIgmpUnknownDrop:
                fields[0] = IGMP_UNKNOWN_MSG_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchIgmpUnknownFlood:
                fields[0] = IGMP_UNKNOWN_MSG_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchIgmpUnknownToCpu:
                fields[0] = IGMP_UNKNOWN_MSG_TO_CPUf;
                break;
            case bcmSwitchMldReportDoneDrop:
                fields[0] = MLD_REP_DONE_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchMldReportDoneFlood:
                fields[0] = MLD_REP_DONE_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchMldReportDoneToCpu:
                fields[0] = MLD_REP_DONE_TO_CPUf;
                break;
            case bcmSwitchMldQueryDrop:
                fields[0] = MLD_QUERY_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchMldQueryFlood:
                fields[0] = MLD_QUERY_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchMldQueryToCpu:
                fields[0] = MLD_QUERY_TO_CPUf;
                break;
            case bcmSwitchIpmcV4RouterDiscoveryDrop:
                fields[0] = IPV4_MC_ROUTER_ADV_PKT_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchIpmcV4RouterDiscoveryFlood:
                fields[0] = IPV4_MC_ROUTER_ADV_PKT_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchIpmcV4RouterDiscoveryToCpu:
                fields[0] = IPV4_MC_ROUTER_ADV_PKT_TO_CPUf;
                break;
            case bcmSwitchIpmcV6RouterDiscoveryDrop:
                fields[0] = IPV6_MC_ROUTER_ADV_PKT_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchIpmcV6RouterDiscoveryFlood:
                fields[0] = IPV6_MC_ROUTER_ADV_PKT_FWD_ACTIONf;
                values[0] =
                    (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
                break;
            case bcmSwitchIpmcV6RouterDiscoveryToCpu:
                fields[0] = IPV6_MC_ROUTER_ADV_PKT_TO_CPUf;
                break;
            default:
                return (BCM_E_UNAVAIL);
        }
    } else
#endif /* BCM_RAPTOR_SUPPORT || BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */
    if (soc_feature(unit, soc_feature_proto_pkt_ctrl)){
        reg = PROTOCOL_PKT_CONTROLr;
        switch (type) {
            case bcmSwitchIgmpPktToCpu:
                fields[0] = IGMP_PKT_TO_CPUf;
                break;
            case bcmSwitchIgmpPktDrop:
                fields[0] = IGMP_PKT_DROPf;
                break;
            case bcmSwitchV4ResvdMcPktToCpu:
                fields[0] = IPV4_RESVD_MC_PKT_TO_CPUf;
                break;
            case bcmSwitchV4ResvdMcPktDrop:
                fields[0] = IPV4_RESVD_MC_PKT_DROPf;
                break;
            case bcmSwitchV6ResvdMcPktToCpu:
                fields[0] = IPV6_RESVD_MC_PKT_TO_CPUf;
                break;
            case bcmSwitchV6ResvdMcPktDrop:
                fields[0] = IPV6_RESVD_MC_PKT_DROPf;
                break;
            default:
                return (BCM_E_UNAVAIL);
        }
    } else {
        return (BCM_E_UNAVAIL);
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_REG_INFO(unit, reg).regtype != soc_portreg) {
        return _bcm_tr2_prot_pkt_profile_set(unit, reg, port, fcount, fields,
                                             values);
    } else
#endif /* BCM_TRIUMPH2_SUPPORT */
    {
        return soc_reg_fields32_modify(unit, reg, port, fcount, fields,
                                       values);
    }
}

/*
 * Function:
 *      _bcm_xgs3_igmp_action_get
 * Description:
 *      Get the IGMP/MLD registers
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to set.
 *      *arg  - IGMP / MLD actions
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_igmp_action_get(int unit,
                         bcm_port_t port,
                         bcm_switch_control_t type,
                         int *arg)
{
    uint32      igmp;
    soc_field_t field = INVALIDf;
    soc_reg_t   reg = INVALIDr;
    int         act_value = BCM_SWITCH_RESERVED_VALUE;
    int         hw_value;

#if defined (BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || \
    defined (BCM_TRX_SUPPORT)
    if (soc_feature(unit, soc_feature_igmp_mld_support)) {
        reg = IGMP_MLD_PKT_CONTROLr;
        /* Given control type select register field. */
        switch (type) {
            case bcmSwitchIgmpPktToCpu:
                field = IGMP_REP_LEAVE_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchIgmpPktDrop:
                field = IGMP_REP_LEAVE_FWD_ACTIONf;
                act_value = BCM_SWITCH_DROP_VALUE;
                break;
            case bcmSwitchMldPktToCpu:
                field = MLD_REP_DONE_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchMldPktDrop:
                field = MLD_REP_DONE_FWD_ACTIONf;
                act_value = BCM_SWITCH_DROP_VALUE;
                break;
            case bcmSwitchV4ResvdMcPktToCpu:
                field = IPV4_RESVD_MC_PKT_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchV4ResvdMcPktFlood:
                field = IPV4_RESVD_MC_PKT_FWD_ACTIONf;
                act_value = BCM_SWITCH_FLOOD_VALUE;
                break;
            case bcmSwitchV4ResvdMcPktDrop:
                field = IPV4_RESVD_MC_PKT_FWD_ACTIONf;
                act_value = BCM_SWITCH_DROP_VALUE;
                break;
            case bcmSwitchV6ResvdMcPktToCpu:
                field = IPV6_RESVD_MC_PKT_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchV6ResvdMcPktFlood:
                field = IPV6_RESVD_MC_PKT_FWD_ACTIONf;
                act_value = BCM_SWITCH_FLOOD_VALUE;
                break;
            case bcmSwitchV6ResvdMcPktDrop:
                field = IPV6_RESVD_MC_PKT_FWD_ACTIONf;
                act_value = BCM_SWITCH_DROP_VALUE;
                break;
            case bcmSwitchIgmpReportLeaveDrop:
                field = IGMP_REP_LEAVE_FWD_ACTIONf;
                act_value = BCM_SWITCH_DROP_VALUE;
                break;
            case bcmSwitchIgmpReportLeaveFlood:
                field = IGMP_REP_LEAVE_FWD_ACTIONf;
                act_value = BCM_SWITCH_FLOOD_VALUE;
                break;
            case bcmSwitchIgmpReportLeaveToCpu:
                field = IGMP_REP_LEAVE_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchIgmpQueryDrop:
                field = IGMP_QUERY_FWD_ACTIONf;
                act_value = BCM_SWITCH_DROP_VALUE;
                break;
            case bcmSwitchIgmpQueryFlood:
                field = IGMP_QUERY_FWD_ACTIONf;
                act_value = BCM_SWITCH_FLOOD_VALUE;
                break;
            case bcmSwitchIgmpQueryToCpu:
                field = IGMP_QUERY_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchIgmpUnknownDrop:
                field = IGMP_UNKNOWN_MSG_FWD_ACTIONf;
                act_value = BCM_SWITCH_DROP_VALUE;
                break;
            case bcmSwitchIgmpUnknownFlood:
                field = IGMP_UNKNOWN_MSG_FWD_ACTIONf;
                act_value = BCM_SWITCH_FLOOD_VALUE;
                break;
            case bcmSwitchIgmpUnknownToCpu:
                field = IGMP_UNKNOWN_MSG_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchMldReportDoneDrop:
                field = MLD_REP_DONE_FWD_ACTIONf;
                act_value = BCM_SWITCH_DROP_VALUE;
                break;
            case bcmSwitchMldReportDoneFlood:
                field = MLD_REP_DONE_FWD_ACTIONf;
                act_value = BCM_SWITCH_FLOOD_VALUE;
                break;
            case bcmSwitchMldReportDoneToCpu:
                field = MLD_REP_DONE_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchMldQueryDrop:
                field = MLD_QUERY_FWD_ACTIONf;
                act_value = BCM_SWITCH_DROP_VALUE;
                break;
            case bcmSwitchMldQueryFlood:
                field = MLD_QUERY_FWD_ACTIONf;
                act_value = BCM_SWITCH_FLOOD_VALUE;
                break;
            case bcmSwitchMldQueryToCpu:
                field = MLD_QUERY_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchIpmcV4RouterDiscoveryDrop:
                field = IPV4_MC_ROUTER_ADV_PKT_FWD_ACTIONf;
                act_value = BCM_SWITCH_DROP_VALUE;
                break;
            case bcmSwitchIpmcV4RouterDiscoveryFlood:
                field = IPV4_MC_ROUTER_ADV_PKT_FWD_ACTIONf;
                act_value = BCM_SWITCH_FLOOD_VALUE;
                break;
            case bcmSwitchIpmcV4RouterDiscoveryToCpu:
                field = IPV4_MC_ROUTER_ADV_PKT_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchIpmcV6RouterDiscoveryDrop:
                field = IPV6_MC_ROUTER_ADV_PKT_FWD_ACTIONf;
                act_value = BCM_SWITCH_DROP_VALUE;
                break;
            case bcmSwitchIpmcV6RouterDiscoveryFlood:
                field = IPV6_MC_ROUTER_ADV_PKT_FWD_ACTIONf;
                act_value = BCM_SWITCH_FLOOD_VALUE;
                break;
            case bcmSwitchIpmcV6RouterDiscoveryToCpu:
                field = IPV6_MC_ROUTER_ADV_PKT_TO_CPUf;
                act_value = 1;
                break;
            default:
                return (BCM_E_UNAVAIL);
        }
    } else
#endif /* BCM_RAPTOR_SUPPORT || BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */
    if (soc_feature(unit, soc_feature_proto_pkt_ctrl)){
        reg = PROTOCOL_PKT_CONTROLr;
        switch (type) {
            case bcmSwitchIgmpPktToCpu:
                field = IGMP_PKT_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchIgmpPktDrop:
                field = IGMP_PKT_DROPf;
                act_value = 1;
                break;
            case bcmSwitchV4ResvdMcPktToCpu:
                field = IPV4_RESVD_MC_PKT_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchV4ResvdMcPktDrop:
                field = IPV4_RESVD_MC_PKT_DROPf;
                act_value = 1;
                break;
            case bcmSwitchV6ResvdMcPktToCpu:
                field = IPV6_RESVD_MC_PKT_TO_CPUf;
                act_value = 1;
                break;
            case bcmSwitchV6ResvdMcPktDrop:
                field = IPV6_RESVD_MC_PKT_DROPf;
                act_value = 1;
                break;
            default:
                return (BCM_E_UNAVAIL);
        }
    } else {
        return (BCM_E_UNAVAIL);
    }

    if (soc_reg_field_valid(unit, reg, field)) {
#if defined(BCM_TRIUMPH2_SUPPORT)
        if (soc_mem_field_valid(unit, PORT_TABm, PROTOCOL_PKT_INDEXf)) {
            int index;

            BCM_IF_ERROR_RETURN
                (_bcm_tr2_protocol_pkt_index_get(unit, port, &index));
            if(SOC_REG_INFO(unit, reg).regtype == soc_portreg) {
                BCM_IF_ERROR_RETURN(soc_reg32_get(unit, reg, (soc_port_t)index, 
                                                  0, &igmp));
            } else {
                BCM_IF_ERROR_RETURN(soc_reg32_get(unit, reg, REG_PORT_ANY, 
                                                  index, &igmp));
            }
        } else
#endif
        {
#if defined (BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || \
        defined (BCM_TRX_SUPPORT)
            if (IGMP_MLD_PKT_CONTROLr == reg) {
                BCM_IF_ERROR_RETURN(READ_IGMP_MLD_PKT_CONTROLr(unit, port, &igmp));
            } else
#endif /* BCM_RAPTOR_SUPPORT  || BCM_TRX_SUPPORT || BCM_FIREBOLT2_SUPPORT */
            {
                BCM_IF_ERROR_RETURN(READ_PROTOCOL_PKT_CONTROLr(unit, port, &igmp));
            }
        }
        hw_value = soc_reg_field_get(unit, reg, igmp, field);
        *arg = (act_value == hw_value) ? 1 : 0;
        return (BCM_E_NONE);
    }

    return (BCM_E_UNAVAIL);
}

#if defined(BCM_TRIUMPH2_SUPPORT)

/*
 * Function:
 *      _bcm_tr2_ep_redirect_action_set
 * Description:
 *      Programs action on EP, for EP redirect feature
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      port - Port number
 *      type - The desired action to set.
 *      arg  - Action value
 * Returns:
 *      BCM_E_xxxx
 */
STATIC int
_bcm_tr2_ep_redirect_action_set(int unit,
                                bcm_port_t port,
                                bcm_switch_control_t type,
                                int arg)
{

    uint32      values[2];
    soc_field_t fields[2];
    int         idx;
    soc_reg_t   reg = INVALIDr ;
    int         fcount = 1;
    int         value = (arg) ? 1 : 0;

    
    if (!SOC_IS_TRIUMPH2(unit) && !SOC_IS_APOLLO(unit) &&
        !SOC_IS_VALKYRIE2(unit)) {
        return BCM_E_UNAVAIL;
    }

    for (idx = 0; idx < COUNTOF(values); idx++) {
        values[idx] = value;
        fields[idx] = INVALIDf;
    }

    reg = EGR_CPU_CONTROLr;
    switch (type) {
        case bcmSwitchUnknownVlanToCpu:
            fields[0] = VLAN_TOCPUf;
            break;
        case bcmSwitchL3HeaderErrToCpu:
            fields[0] = L3ERR_TOCPUf;
            break;
        case bcmSwitchIpmcTtlErrToCpu:
            fields[0] = TTL_DROP_TOCPUf;
            break;
        case bcmSwitchHgHdrErrToCpu:
            fields[0] = HIGIG_TOCPUf;
            break;
        case bcmSwitchStgInvalidToCpu:
            fields[0] = STG_TOCPUf;
            break;
        case bcmSwitchVlanTranslateEgressMissToCpu:
            fields[0] = VXLT_TOCPUf;
            break;
        case bcmSwitchTunnelErrToCpu:
            fields[0] = TUNNEL_TOCPUf;
            break;
        case bcmSwitchL3PktErrToCpu:
            fields[0] = L3PKT_ERR_TOCPUf;
            break;
        case bcmSwitchMtuFailureToCpu:
            fields[0] = MTU_TOCPUf;
            break;
        case bcmSwitchSrcKnockoutToCpu:
            fields[0] = PRUNE_TOCPUf;
            break;
        case bcmSwitchWlanTunnelMismatchToCpu:
            fields[0] = WLAN_MOVE_TOCPUf;
            break;
        case bcmSwitchWlanTunnelMismatchDrop:
            fields[0] = WLAN_MOVE_DROPf;
            break;
        case bcmSwitchWlanPortMissToCpu:
            fields[0] = WLAN_SVP_MISS_TOCPUf;
            break;
        default:
            return(BCM_E_UNAVAIL);
    }

    for (idx = 0; idx < fcount; idx++) {
        if (!SOC_REG_FIELD_VALID(unit, reg, fields[idx])) {
            return BCM_E_UNAVAIL;
        }
    }

    return soc_reg_fields32_modify(unit, reg, port, fcount, fields, values);
}


/*
 * Function:
 *      _bcm_tr2_ep_redirect_action_get
 * Description:
 *      Gets action on EP, for EP redirect feature
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      port - Port number
 *      type - The desired action to set.
 *      arg  - (OUT) Action value programmed
 * Returns:
 *      BCM_E_xxxx
 */
STATIC int
_bcm_tr2_ep_redirect_action_get(int unit,
                                bcm_port_t port,
                                bcm_switch_control_t type,
                                int *arg)
{
    int         egr_arg, ing_arg;
    uint64      ing_reg_val, egr_reg_val;
    uint32      ing_reg, egr_reg, ing_field, egr_field;

    
    if (!SOC_IS_TRIUMPH2(unit) && !SOC_IS_APOLLO(unit) &&
        !SOC_IS_VALKYRIE2(unit)) {
        return BCM_E_UNAVAIL;
    }

    if (!soc_feature(unit, soc_feature_internal_loopback)) {
        return BCM_E_UNAVAIL;
    }

    ing_reg = INVALIDr;
    ing_field = INVALIDf;
    egr_reg = EGR_CPU_CONTROLr;

    switch (type) {
        case bcmSwitchUnknownVlanToCpu:
            /* Egress vs ingress comparison necessary only for overwrriten */
            /* switch controls */
            if (SOC_IS_TR_VL(unit)) {
                ing_reg = CPU_CONTROL_0r;
            } else {
                ing_reg = CPU_CONTROL_1r;
            }
            ing_field = UVLAN_TOCPUf;
            egr_field = VLAN_TOCPUf;
            break;
        case bcmSwitchL3HeaderErrToCpu:
            /* Egress vs ingress comparison necessary only for overwrriten */
            /* switch controls */
            ing_reg = CPU_CONTROL_1r;
            ing_field = V4L3ERR_TOCPUf;
            egr_field = L3ERR_TOCPUf;
            break;
        case bcmSwitchIpmcTtlErrToCpu:
            /* Egress vs ingress comparison necessary only for overwrriten */
            /* switch controls */
            ing_reg = CPU_CONTROL_1r;
            ing_field = IPMC_TTL_ERR_TOCPUf;
            egr_field = TTL_DROP_TOCPUf;
            break;
        case bcmSwitchHgHdrErrToCpu:
            /* Egress vs ingress comparison necessary only for overwrriten */
            /* switch controls */
            ing_reg = CPU_CONTROL_1r;
            ing_field = HG_HDR_ERROR_TOCPUf;
            egr_field = HIGIG_TOCPUf;
            break;
        case bcmSwitchTunnelErrToCpu:
            /* Egress vs ingress comparison necessary only for overwrriten */
            /* switch controls */
            ing_reg = CPU_CONTROL_1r;
            ing_field = TUNNEL_ERR_TOCPUf;
            egr_field = TUNNEL_TOCPUf;
            break;
        case bcmSwitchStgInvalidToCpu:
            egr_field = STG_TOCPUf;
            break;
        case bcmSwitchVlanTranslateEgressMissToCpu:
            egr_field = VXLT_TOCPUf;
            break;
        case bcmSwitchL3PktErrToCpu:
            egr_field = L3PKT_ERR_TOCPUf;
            break;
        case bcmSwitchMtuFailureToCpu:
            egr_field = MTU_TOCPUf;
            break;
        case bcmSwitchSrcKnockoutToCpu:
            egr_field = PRUNE_TOCPUf;
            break;
        case bcmSwitchWlanTunnelMismatchToCpu:
            egr_field = WLAN_MOVE_TOCPUf;
            break;
        case bcmSwitchWlanTunnelMismatchDrop:
            egr_field = WLAN_MOVE_DROPf;
            break;
        case bcmSwitchWlanPortMissToCpu:
            egr_field = WLAN_SVP_MISS_TOCPUf;
            break;

        default:
            return(BCM_E_UNAVAIL);
    }

    SOC_IF_ERROR_RETURN(soc_reg_get(unit, egr_reg, port, 0,
                                     &egr_reg_val));
    egr_arg = soc_reg64_field32_get(unit, egr_reg, egr_reg_val,
                                    egr_field);
    if (ing_reg != INVALIDr) {
        SOC_IF_ERROR_RETURN(soc_reg_get(unit, ing_reg, port, 0,
                                         &ing_reg_val));
        ing_arg = soc_reg64_field32_get(unit, ing_reg, ing_reg_val,
                                        ing_field);
        if (egr_arg != ing_arg) {
            return(BCM_E_CONFIG);
        }
    }

    *arg = egr_arg;
    return(BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr2_ep_redirect_action_cosq_set
 * Description:
 *      Programs CPU COSQ for EP redirect actions
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      port - Port number
 *      type - The desired action to set.
 *      arg  - Action value
 * Returns:
 *      BCM_E_xxxx
 */
STATIC int
_bcm_tr2_ep_redirect_action_cosq_set(int unit,
                                     bcm_port_t port,
                                     bcm_switch_control_t type,
                                     int arg)
{
    uint32      values[2];
    soc_field_t fields[2];
    int         idx;
    soc_reg_t   reg = INVALIDr ;
    int         fcount = 1;

    
    if (!SOC_IS_TRIUMPH2(unit) && !SOC_IS_APOLLO(unit) &&
        !SOC_IS_VALKYRIE2(unit)) {
        return BCM_E_UNAVAIL;
    }

    if (!soc_feature(unit, soc_feature_internal_loopback)) {
        return BCM_E_UNAVAIL;
    }

    if (arg < 0 || arg > NUM_CPU_COSQ(unit)) {
        return BCM_E_PARAM;
    }

    for (idx = 0; idx < COUNTOF(values); idx++) {
        values[idx] = arg;
        fields[idx] = INVALIDf;
    }

    reg = EGR_CPU_COS_CONTROL_1_64r;
    switch (type) {
        case bcmSwitchUnknownVlanToCpuCosq:
            fields[0] = VLAN_CPU_COSf;
            break;
        case bcmSwitchStgInvalidToCpuCosq:
            fields[0] = STG_CPU_COSf;
            break;
        case bcmSwitchVlanTranslateEgressMissToCpuCosq:
            fields[0] = VXLT_CPU_COSf;
            break;
        case bcmSwitchTunnelErrToCpuCosq:
            fields[0] = TUNNEL_CPU_COSf;
            break;
        case bcmSwitchL3HeaderErrToCpuCosq:
            fields[0] = L3ERR_CPU_COSf;
            break;
        case bcmSwitchL3PktErrToCpuCosq:
            fields[0] = L3PKT_ERR_CPU_COSf;
            break;
        case bcmSwitchIpmcTtlErrToCpuCosq:
            fields[0] = TTL_DROP_CPU_COSf;
            break;
        case bcmSwitchMtuFailureToCpuCosq:
            fields[0] = MTU_CPU_COSf;
            break;
        case bcmSwitchHgHdrErrToCpuCosq:
            fields[0] = HIGIG_CPU_COSf;
            break;
        case bcmSwitchSrcKnockoutToCpuCosq:
            fields[0] = PRUNE_CPU_COSf;
            break;
        case bcmSwitchWlanTunnelMismatchToCpuCosq:
            reg = EGR_CPU_COS_CONTROL_2r;
            fields[0] = WLAN_MOVE_CPU_COSf;
            break;
        case bcmSwitchWlanPortMissToCpuCosq:
            reg = EGR_CPU_COS_CONTROL_2r;
            fields[0] = WLAN_SVP_MISS_CPU_COSf;
            break;
        default:
            return(BCM_E_UNAVAIL);
    }

    return soc_reg_fields32_modify(unit, reg, port, fcount, fields, values);
}

/*
 * Function:
 *      _bcm_tr2_ep_redirect_action_cosq_get
 * Description:
 *      Gets action on EP, for EP redirect feature
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      port - Port number
 *      type - The desired action to set.
 *      arg  - (OUT) Action value programmed
 * Returns:
 *      BCM_E_xxxx
 */
STATIC int
_bcm_tr2_ep_redirect_action_cosq_get(int unit,
                                bcm_port_t port,
                                bcm_switch_control_t type,
                                int *arg)
{
    uint64      reg_val;
    uint32      reg, field;

    
    if (!SOC_IS_TRIUMPH2(unit) && !SOC_IS_APOLLO(unit) &&
        !SOC_IS_VALKYRIE2(unit)) {
        return BCM_E_UNAVAIL;
    }

    if (!soc_feature(unit, soc_feature_internal_loopback)) {
        return BCM_E_UNAVAIL;
    }

    reg = EGR_CPU_COS_CONTROL_1_64r;
    switch (type) {
        case bcmSwitchUnknownVlanToCpuCosq:
            field = VLAN_CPU_COSf;
            break;
        case bcmSwitchStgInvalidToCpuCosq:
            field = STG_CPU_COSf;
            break;
        case bcmSwitchVlanTranslateEgressMissToCpuCosq:
            field = VXLT_CPU_COSf;
            break;
        case bcmSwitchTunnelErrToCpuCosq:
            field = TUNNEL_CPU_COSf;
            break;
        case bcmSwitchL3HeaderErrToCpuCosq:
            field = L3ERR_CPU_COSf;
            break;
        case bcmSwitchL3PktErrToCpuCosq:
            field = L3PKT_ERR_CPU_COSf;
            break;
        case bcmSwitchIpmcTtlErrToCpuCosq:
            field = TTL_DROP_CPU_COSf;
            break;
        case bcmSwitchMtuFailureToCpuCosq:
            field = MTU_CPU_COSf;
            break;
        case bcmSwitchHgHdrErrToCpuCosq:
            field = HIGIG_CPU_COSf;
            break;
        case bcmSwitchSrcKnockoutToCpuCosq:
            field = PRUNE_CPU_COSf;
            break;
        case bcmSwitchWlanTunnelMismatchToCpuCosq:
            reg = EGR_CPU_COS_CONTROL_2r;
            field = WLAN_MOVE_CPU_COSf;
            break;
        case bcmSwitchWlanPortMissToCpuCosq:
            reg = EGR_CPU_COS_CONTROL_2r;
            field = WLAN_SVP_MISS_CPU_COSf;
            break;
        default:
            return(BCM_E_UNAVAIL);
    }

    SOC_IF_ERROR_RETURN(
        soc_reg_get(unit, reg, port, 0, &reg_val));
    *arg = soc_reg64_field32_get(unit, reg, reg_val, field);
    return(BCM_E_NONE);
}

STATIC int
_bcm_tr2_layered_qos_resolution_set(int unit,
                                    bcm_port_t port,
                                    bcm_switch_control_t type,
                                    int arg)
{
    uint64 rval64;
    uint32 rval, value;

    
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) || SOC_IS_KATANA(unit) || SOC_IS_HURRICANEX(unit)) {
        value = arg ? 0 : 1; /* layered resolution  : serial resolution */
        BCM_IF_ERROR_RETURN(READ_ING_CONFIG_64r(unit, &rval64));
        soc_reg64_field32_set(unit, ING_CONFIG_64r, &rval64,
                              IGNORE_PPD0_PRESERVE_QOSf, value);
        soc_reg64_field32_set(unit, ING_CONFIG_64r, &rval64,
                              IGNORE_PPD2_PRESERVE_QOSf, value);
        if((!SOC_IS_ENDURO(unit)) && (!SOC_IS_HURRICANEX(unit))) {
            soc_reg64_field32_set(unit, ING_CONFIG_64r, &rval64,
                                  IGNORE_PPD3_PRESERVE_QOSf, value);
        }
        SOC_IF_ERROR_RETURN(WRITE_ING_CONFIG_64r(unit, rval64));
        BCM_IF_ERROR_RETURN(READ_EGR_CONFIG_1r(unit, &rval));
        soc_reg_field_set(unit, EGR_CONFIG_1r, &rval,
                          DISABLE_PPD0_PRESERVE_QOSf, value);
        soc_reg_field_set(unit, EGR_CONFIG_1r, &rval,
                          DISABLE_PPD2_PRESERVE_QOSf, value);
        if((!SOC_IS_ENDURO(unit)) && (!SOC_IS_HURRICANEX(unit))) {
            soc_reg_field_set(unit, EGR_CONFIG_1r, &rval,
                              DISABLE_PPD3_PRESERVE_QOSf, value);
        }
        SOC_IF_ERROR_RETURN(WRITE_EGR_CONFIG_1r(unit, rval));

        return BCM_E_NONE;
    }

    return BCM_E_UNAVAIL;
}

STATIC int
_bcm_tr2_layered_qos_resolution_get(int unit,
                                    bcm_port_t port,
                                    bcm_switch_control_t type,
                                    int *arg)
{
    uint64 rval64;

    
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) || SOC_IS_KATANA(unit) || SOC_IS_HURRICANEX(unit)) {
        BCM_IF_ERROR_RETURN(READ_ING_CONFIG_64r(unit, &rval64));
        *arg = soc_reg64_field32_get(unit, ING_CONFIG_64r, rval64,
                                     IGNORE_PPD0_PRESERVE_QOSf) ? 0 : 1;
        return BCM_E_NONE;
    }

    return BCM_E_UNAVAIL;
}

STATIC int
_bcm_tr2_ehg_error2cpu_set(int unit, bcm_port_t port, int arg)
{
    if (soc_feature(unit, soc_feature_embedded_higig)) {
        uint32  rval;
        soc_field_t ehg_field;

        if (!IS_ST_PORT(unit, port)) {
            return (BCM_E_CONFIG);
        }
        if (SOC_REG_FIELD_VALID(unit, CPU_CONTROL_0r, EHG_NONHG_TOCPUf)) {
            ehg_field = EHG_NONHG_TOCPUf;
        } else if (SOC_REG_FIELD_VALID(unit, CPU_CONTROL_0r,
                                       EHG_NONHG_TO_CPUf)) {
            ehg_field = EHG_NONHG_TO_CPUf;
        } else {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN(READ_CPU_CONTROL_0r(unit, &rval));
        soc_reg_field_set(unit, CPU_CONTROL_0r, &rval,
                          ehg_field, arg ? 1 : 0);
        BCM_IF_ERROR_RETURN(WRITE_CPU_CONTROL_0r(unit, rval));

        return (BCM_E_NONE);
    }

    return (BCM_E_UNAVAIL);
}

STATIC int
_bcm_tr2_ehg_error2cpu_get(int unit, bcm_port_t port, int *arg)
{
    if (soc_feature(unit, soc_feature_embedded_higig)) {
        uint32  rval;
        soc_field_t ehg_field;

        if (!IS_ST_PORT(unit, port)) {
            return (BCM_E_CONFIG);
        }
        if (SOC_REG_FIELD_VALID(unit, CPU_CONTROL_0r, EHG_NONHG_TOCPUf)) {
            ehg_field = EHG_NONHG_TOCPUf;
        } else if (SOC_REG_FIELD_VALID(unit, CPU_CONTROL_0r,
                                       EHG_NONHG_TO_CPUf)) {
            ehg_field = EHG_NONHG_TO_CPUf;
        } else {
            return BCM_E_UNAVAIL;
        }
        BCM_IF_ERROR_RETURN(READ_CPU_CONTROL_0r(unit, &rval));
        *arg = soc_reg_field_get(unit, CPU_CONTROL_0r, rval, ehg_field);

        return (BCM_E_NONE);
    }

    return (BCM_E_UNAVAIL);
}

STATIC int
_bcm_tr2_mirror_egress_true_set(int unit, bcm_port_t port,
                                bcm_switch_control_t type, int arg)
{
    uint32      values[2];
    soc_field_t fields[2];

    if (soc_feature(unit, soc_feature_egr_mirror_true)) {
        if (type == bcmSwitchMirrorEgressTrueColorSelect) {
            fields[0] = REDIRECT_DROP_PRECEDENCEf;
            switch (arg) {
            case bcmColorGreen:
                values[0] = 1;
                break;
            case bcmColorYellow:
                values[0] = 2;
                break;
            case bcmColorRed:
                values[0] = 3;
                break;
            default:
                values[0] = 0; /* No change */
                break;
            }
            return soc_reg_fields32_modify(unit, EGR_PORT_64r, port, 1,
                                           fields, values);
        } else if (type == bcmSwitchMirrorEgressTruePriority) {
            fields[0] = REDIRECT_INT_PRI_SELf;
            fields[1] = REDIRECT_INT_PRIf;
            if ((arg < 0) || (arg > 0xf)) {
                values[0] = 0;
                values[1] = 0;
            } else {
                values[0] = 1;
                values[1] = arg & 0xf;
            }
            return soc_reg_fields32_modify(unit, EGR_PORT_64r, port, 2,
                                           fields, values);
        }
    }
    return (BCM_E_UNAVAIL);
}

STATIC int
_bcm_tr2_mirror_egress_true_get(int unit, bcm_port_t port,
                                bcm_switch_control_t type, int *arg)
{
    uint64 rval64;

    if (soc_feature(unit, soc_feature_egr_mirror_true)) {
        BCM_IF_ERROR_RETURN(READ_EGR_PORT_64r(unit, port, &rval64));
        if (type == bcmSwitchMirrorEgressTrueColorSelect) {
            switch (soc_reg64_field32_get(unit, EGR_PORT_64r, rval64,
                              REDIRECT_DROP_PRECEDENCEf)) {
            case 1:
                *arg = bcmColorGreen;
                break;
            case 2:
                *arg = bcmColorYellow;
                break;
            case 3:
                *arg = bcmColorRed;
                break;
            case 0:
            default:
                *arg = -1; /* No change */
                break;
            }
            return BCM_E_NONE;
        } else if (type == bcmSwitchMirrorEgressTruePriority) {
            if (soc_reg64_field32_get(unit, EGR_PORT_64r, rval64,
                              REDIRECT_INT_PRI_SELf)) {
                *arg = soc_reg64_field32_get(unit, EGR_PORT_64r, rval64,
                                             REDIRECT_INT_PRIf);
            } else {
                *arg = -1;
            }
            return BCM_E_NONE;
        }
    }
    return (BCM_E_UNAVAIL);
}
#endif /* BCM_TRIUMPH2_SUPPORT */

#if defined(BCM_HAWKEYE_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT)
/*
 * Function:
 *      _bcm_xgs3_switch_ethertype_set
 * Description:
 *      Set the ethertype field for MMRP, and SRP registers
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control to set the ethertype for
 *      arg         - argument to set as Ethrtype
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_switch_ethertype_set(int unit, bcm_port_t port, 
                               bcm_switch_control_t type,
                               int arg)
{
    soc_reg_t   reg = INVALIDr ;
    uint16      ethertype;
    
    if (!soc_feature(unit, soc_feature_timesync_support)) {
        return BCM_E_UNAVAIL;
    }
    ethertype = (uint16)arg;

    /* Given control type select register. */
    switch (type) {
        case bcmSwitchMMRPEthertype:
            reg = MMRP_CONTROL_2r;
            break;
        case bcmSwitchTimeSyncEthertype: 
            reg = TS_CONTROLr;
            break;
        case bcmSwitchSRPEthertype:
            reg = SRP_CONTROL_2r;
            break;
        default:
            return BCM_E_PARAM;
    }

    if (!SOC_REG_FIELD_VALID(unit, reg, ETHERTYPEf)) {
        return BCM_E_UNAVAIL;
    }

    return soc_reg_field32_modify(unit, reg, port, ETHERTYPEf, ethertype);
}

/*
 * Function:
 *      _bcm_xgs3_switch_ethertype_get
 * Description:
 *      Get the ethertype field for MMRP, and SRP registers
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control to get the ethertype for
 *      arg         - Ethrtype to get
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_switch_ethertype_get(int unit, bcm_port_t port, 
                               bcm_switch_control_t type,
                               int *arg)
{
    soc_reg_t   reg = INVALIDr ;
    uint16      ethertype;
    uint32      regval;

    if (!soc_feature(unit, soc_feature_timesync_support)) {
        return BCM_E_UNAVAIL;
    }

    /* Given control type select register. */
    switch (type) {
        case bcmSwitchMMRPEthertype:
            reg = MMRP_CONTROL_2r;
            break;
        case bcmSwitchTimeSyncEthertype: 
            reg = TS_CONTROLr;
            break;
        case bcmSwitchSRPEthertype:
            reg = SRP_CONTROL_2r;
            break;
        default:
            return BCM_E_PARAM;
    }

    if (!soc_reg_field_valid(unit, reg, ETHERTYPEf)) {
        return BCM_E_UNAVAIL;
    }
    SOC_IF_ERROR_RETURN(
        soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &regval));
    ethertype = soc_reg_field_get(unit, reg, regval, ETHERTYPEf);

    *arg = (int)ethertype;

    return BCM_E_NONE;
}
/*
 * Function:
 *      _bcm_xgs3_switch_mac_lo_set
 * Description:
 *      Set the low 24 bits of MAC address field for MMRP, and SRP registers
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control type to set the mac for
 *      mac_lo      - MAC address
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_switch_mac_lo_set(int unit, bcm_port_t port, 
                            bcm_switch_control_t type,
                            uint32 mac_lo)
{
    uint32      regval, fieldval;
    soc_reg_t   reg = INVALIDr ;
    soc_field_t f_lo = INVALIDf;


    if (!soc_feature(unit, soc_feature_timesync_support)) {
        return BCM_E_UNAVAIL;
    }

    /* Given control type select register. */
    switch (type) {
        case bcmSwitchMMRPDestMacNonOui:
            reg = MMRP_CONTROL_1r;
            f_lo = MAC_DA_LOWERf;
            break;
        case bcmSwitchTimeSyncDestMacNonOui: 
            reg = TS_CONTROL_1r;
            f_lo = MAC_DA_LOWERf;
            break;
        case bcmSwitchSRPDestMacNonOui:
            reg = SRP_CONTROL_1r;
            f_lo = MAC_DA_LOWERf;
            break;
        default:
            return BCM_E_PARAM;
    }

    if (!soc_reg_field_valid(unit, reg, f_lo)) {
        return BCM_E_UNAVAIL;
    }
    SOC_IF_ERROR_RETURN(soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &regval));
    fieldval = soc_reg_field_get(unit, reg, regval, f_lo);
    fieldval |= ((mac_lo << 8) >> 8);
    soc_reg_field_set(unit, reg, &regval, f_lo, fieldval);

    SOC_IF_ERROR_RETURN(soc_reg32_set(unit, reg, REG_PORT_ANY, 0, regval));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_switch_mac_hi_set
 * Description:
 *      Set the upper 24 bits of MAC address field for MMRP, and SRP registers
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control type to set upper MAC for
 *      mac_hi      - MAC address
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_switch_mac_hi_set(int unit, bcm_port_t port, 
                            bcm_switch_control_t type,
                            uint32 mac_hi)
{
    soc_reg_t   reg1, reg2;
    soc_field_t f_lo, f_hi;
    uint32      fieldval, regval1, regval2;

    reg1 = reg2 = INVALIDr;

    if (!soc_feature(unit, soc_feature_timesync_support)) {
        return BCM_E_UNAVAIL;
    }

    /* Given control type select register. */
    switch (type) {
        case bcmSwitchMMRPDestMacOui:
            reg1 = MMRP_CONTROL_1r;
            reg2 = MMRP_CONTROL_2r;
            f_lo = MAC_DA_LOWERf;
            f_hi = MAC_DA_UPPERf;
            break;
        case bcmSwitchTimeSyncDestMacOui: 
            reg1 = TS_CONTROL_1r;
            reg2 = TS_CONTROL_2r;
            f_lo = MAC_DA_LOWERf;
            f_hi = MAC_DA_UPPERf;
            break;
        case bcmSwitchSRPDestMacOui:
            reg1 = SRP_CONTROL_1r;
            reg2 = SRP_CONTROL_2r;
            f_lo = MAC_DA_LOWERf;
            f_hi = MAC_DA_UPPERf;
            break;
        default:
            return BCM_E_PARAM;
    }

    if (!soc_reg_field_valid(unit, reg1, f_lo)) {
        return BCM_E_UNAVAIL;
    }
    if (!soc_reg_field_valid(unit, reg2, f_hi)) {
        return BCM_E_UNAVAIL;
    }
    
    SOC_IF_ERROR_RETURN(soc_reg32_get(unit, reg1, REG_PORT_ANY, 0, &regval1));
    SOC_IF_ERROR_RETURN(soc_reg32_get(unit, reg2, REG_PORT_ANY, 0, &regval2));
    
    fieldval = (mac_hi << 24);
    soc_reg_field_set(unit, reg1, &regval1, f_lo, fieldval);
    fieldval = (mac_hi >> 8) & 0xffff;
    soc_reg_field_set(unit, reg2, &regval2, f_hi, fieldval);

    SOC_IF_ERROR_RETURN(
        soc_reg32_set(unit, reg1, REG_PORT_ANY, 0, regval1));
    SOC_IF_ERROR_RETURN(
        soc_reg32_set(unit, reg2, REG_PORT_ANY, 0, regval2));
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_xgs3_switch_mac_lo_get
 * Description:
 *      Get the lower 24 bits of MAC address field for MMRP, and SRP registers
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control type to get MAC for
 *      arg         - arg to get the lower MAC address
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_switch_mac_lo_get(int unit, bcm_port_t port, 
                            bcm_switch_control_t type,
                            int *arg)
{
    soc_reg_t   reg = INVALIDr ;
    soc_field_t f_lo = INVALIDf; 
    uint32      regval, mac;

    if (!soc_feature(unit, soc_feature_timesync_support)) {
        return BCM_E_UNAVAIL;
    }

    /* Given control type select register. */
    switch (type) {
        case bcmSwitchMMRPDestMacNonOui:
            reg = MMRP_CONTROL_1r;
            f_lo = MAC_DA_LOWERf;
            break;
        case bcmSwitchTimeSyncDestMacNonOui: 
            reg = TS_CONTROL_1r;
            f_lo = MAC_DA_LOWERf;
            break;
        case bcmSwitchSRPDestMacNonOui:
            reg = SRP_CONTROL_1r;
            f_lo = MAC_DA_LOWERf;
            break;
        case bcmSwitchRemoteCpuLocalMacNonOui:
            reg = CMIC_PKT_LMAC0_LOr;
            f_lo = MAC0_LOf;
            break;
        case bcmSwitchRemoteCpuDestMacNonOui:
            reg = CMIC_PKT_RMACr;
            f_lo = MAC0_LOf;
            break;
        default:
            return BCM_E_PARAM;
    }

    if (!soc_reg_field_valid(unit, reg, f_lo)) {
        return BCM_E_UNAVAIL;
    }

    SOC_IF_ERROR_RETURN(
            soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &regval));
    mac = soc_reg_field_get(unit, reg, regval, f_lo);

    *arg = (mac << 8) >> 8;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_switch_mac_hi_get
 * Description:
 *      Get the upper 24 bits of MAC address field for MMRP, and SRP registers
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control type to get MAC for
 *      arg         - arg to get the upper MAC address
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_switch_mac_hi_get(int unit, bcm_port_t port, 
                            bcm_switch_control_t type,
                         int *arg)
{
    soc_reg_t   reg1, reg2;
    soc_field_t f_lo, f_hi;
    uint32      mac, regval1, regval2;

    reg1 = reg2 = INVALIDr;

    if (!soc_feature(unit, soc_feature_timesync_support)) {
        return BCM_E_UNAVAIL;
    }

    /* Given control type select register. */
    switch (type) {
        case bcmSwitchMMRPDestMacOui:
            reg1 = MMRP_CONTROL_1r;
            reg2 = MMRP_CONTROL_2r;
            f_lo = MAC_DA_LOWERf;
            f_hi = MAC_DA_UPPERf;
            break;
        case bcmSwitchTimeSyncDestMacOui: 
            reg1 = TS_CONTROL_1r;
            reg2 = TS_CONTROL_2r;
            f_lo = MAC_DA_LOWERf;
            f_hi = MAC_DA_UPPERf;
            break;
        case bcmSwitchSRPDestMacOui:
            reg1 = SRP_CONTROL_1r;
            reg2 = SRP_CONTROL_2r;
            f_lo = MAC_DA_LOWERf;
            f_hi = MAC_DA_UPPERf;
            break;
        default:
            return BCM_E_PARAM;
    }

    if (!soc_reg_field_valid(unit, reg1, f_lo)) {
        return BCM_E_UNAVAIL;
    }
    if (!soc_reg_field_valid(unit, reg2, f_hi)) {
        return BCM_E_UNAVAIL;
    }
    SOC_IF_ERROR_RETURN(
        soc_reg32_get(unit, reg1, REG_PORT_ANY, 0, &regval1));
    mac = (soc_reg_field_get(unit, reg1, regval1, f_lo) >> 24);

    SOC_IF_ERROR_RETURN(
        soc_reg32_get(unit, reg2, REG_PORT_ANY, 0, &regval2));
    mac |= (soc_reg_field_get(unit, reg2, regval2, f_hi) << 8);

    *arg = (int)mac;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_eav_action_set
 * Description:
 *      Set the Time-Sync, MMRP, and SRP registers
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to set.
 *      arg  - EAV actions
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_eav_action_set(int unit,
                         bcm_port_t port,
                         bcm_switch_control_t type,
                         int arg)
{
    uint32      values[3];
    soc_field_t fields[3];
    int         idx;
    soc_reg_t   reg = INVALIDr ;
    int         fcount = 1;
    int         value = (arg) ? 1 : 0;

    if (!soc_feature(unit, soc_feature_timesync_support)) {
        return BCM_E_UNAVAIL;
    }

    for (idx = 0; idx < COUNTOF(values); idx++) {
        values[idx] = value;
        fields[idx] = INVALIDf;
    }

    reg = PROTOCOL_PKT_CONTROLr;
    /* Given control type select register field. */
    switch (type) {
        case bcmSwitchTimeSyncPktToCpu:
            if (soc_reg_field_valid(unit, reg, TS_PKT_TO_CPUf)) {
                fields[0] = TS_PKT_TO_CPUf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchTimeSyncPktDrop:
            if (soc_reg_field_valid(unit, reg, TS_FWD_ACTIONf)) {
                fields[0] = TS_FWD_ACTIONf;
                values[0] = (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchTimeSyncPktFlood:
            if (soc_reg_field_valid(unit, reg, TS_FWD_ACTIONf)) {
                fields[0] = TS_FWD_ACTIONf;
                values[0] = (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchMmrpPktToCpu:
            if (soc_reg_field_valid(unit, reg, MMRP_PKT_TO_CPUf)) {
                fields[0] = MMRP_PKT_TO_CPUf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchMmrpPktDrop:
            if (soc_reg_field_valid(unit, reg, MMRP_FWD_ACTIONf)) {
                fields[0] = MMRP_FWD_ACTIONf;
                values[0] = (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchMmrpPktFlood:
            if (soc_reg_field_valid(unit, reg, MMRP_FWD_ACTIONf)) {
                fields[0] = MMRP_FWD_ACTIONf;
                values[0] = (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchSrpPktToCpu:
            if (soc_reg_field_valid(unit, reg, SRP_PKT_TO_CPUf)) {
                fields[0] = SRP_PKT_TO_CPUf;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchSrpPktDrop:
            if (soc_reg_field_valid(unit, reg, SRP_FWD_ACTIONf)) {
                fields[0] = SRP_FWD_ACTIONf;
                values[0] = (arg) ? BCM_SWITCH_DROP_VALUE : BCM_SWITCH_FORWARD_VALUE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchSrpPktFlood:
            if (soc_reg_field_valid(unit, reg, SRP_FWD_ACTIONf)) {
                fields[0] = SRP_FWD_ACTIONf;
                values[0] = (arg) ? BCM_SWITCH_FLOOD_VALUE : BCM_SWITCH_FORWARD_VALUE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        default:
                return (BCM_E_UNAVAIL);
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_REG_INFO(unit, reg).regtype != soc_portreg) {
        return _bcm_tr2_prot_pkt_profile_set(unit, reg, port, fcount, fields,
                                             values);
    } else
#endif /* BCM_TRIUMPH2_SUPPORT */
    {
        return soc_reg_fields32_modify(unit, reg, port, fcount, fields,
                                       values);
    }
}

/*
 * Function:
 *      _bcm_xgs3_eav_action_get
 * Description:
 *      Get the Time-Sync, MMRP, and SRP registers
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to set.
 *      *arg  - EAV actions
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_eav_action_get(int unit,
                         bcm_port_t port,
                         bcm_switch_control_t type,
                         int *arg)
{
    uint32      eav;
    soc_field_t field = INVALIDf;
    soc_reg_t   reg = INVALIDr;
    int         act_value = BCM_SWITCH_RESERVED_VALUE;
    int         hw_value = -1;

    reg = PROTOCOL_PKT_CONTROLr;
    /* Given control type select register field. */
    switch (type) {
        case bcmSwitchTimeSyncPktToCpu:
            field = TS_PKT_TO_CPUf;
            act_value = 1;
            break;
        case bcmSwitchTimeSyncPktDrop:
            field = TS_FWD_ACTIONf;
            act_value = BCM_SWITCH_DROP_VALUE;
            break;
        case bcmSwitchTimeSyncPktFlood:
            field = TS_FWD_ACTIONf;
            act_value = BCM_SWITCH_FLOOD_VALUE;
            break;
        case bcmSwitchMmrpPktToCpu:
            field = MMRP_PKT_TO_CPUf;
            act_value = 1;
            break;
        case bcmSwitchMmrpPktDrop:
            field = MMRP_FWD_ACTIONf;
            act_value = BCM_SWITCH_DROP_VALUE;
            break;
        case bcmSwitchMmrpPktFlood:
            field = MMRP_FWD_ACTIONf;
            act_value = BCM_SWITCH_FLOOD_VALUE;
            break;
        case bcmSwitchSrpPktToCpu:
            field = SRP_PKT_TO_CPUf;
            act_value = 1;
            break;
        case bcmSwitchSrpPktDrop:
            field = SRP_FWD_ACTIONf;
            act_value = BCM_SWITCH_DROP_VALUE;
            break;
        case bcmSwitchSrpPktFlood:
            field = SRP_FWD_ACTIONf;
            act_value = BCM_SWITCH_FLOOD_VALUE;
            break;
        default:
                return (BCM_E_UNAVAIL);
    }

    if (soc_reg_field_valid(unit, reg, field)) {
#ifdef BCM_TRIUMPH2_SUPPORT
        if (SOC_REG_INFO(unit, reg).regtype != soc_portreg) {
            int index;

            BCM_IF_ERROR_RETURN
                (_bcm_tr2_protocol_pkt_index_get(unit, port, &index));
            BCM_IF_ERROR_RETURN(soc_reg32_get(unit, reg, REG_PORT_ANY, index, &eav));
        } else
#endif /* BCM_TRIUMPH2_SUPPORT */
        {
            BCM_IF_ERROR_RETURN(READ_PROTOCOL_PKT_CONTROLr(unit, port, &eav));
        }
        hw_value = soc_reg_field_get(unit, reg, eav, field);
    } else {
        return BCM_E_UNAVAIL;
    }
    
    *arg = (act_value == hw_value) ? 1 : 0;
    return (BCM_E_NONE);
}
#endif /* HAWKEYE, TRIUMPH2, APOLLO */

#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRX_SUPPORT)
/*
 * Function:
 *      _bcm_xgs3_selectcontrol_set
 * Description:
 *      Set the enhanced (aka rtag 7) hash select control.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      arg  - BCM_HASH_CONTROL*
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_selectcontrol_set(int unit, int arg)
{
    uint64      hash_control;
    uint32      val;

#if defined(BCM_HURRICANE_SUPPORT)
        if(SOC_IS_HURRICANEX(unit)) {
            return (BCM_E_UNAVAIL);
        }
#endif

    BCM_IF_ERROR_RETURN(READ_RTAG7_HASH_CONTROLr(unit, &hash_control));

    val = (arg & BCM_HASH_FIELD0_DISABLE_IP4 ) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                          DISABLE_HASH_IPV4_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_IP4) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                          DISABLE_HASH_IPV4_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_IP6) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                          DISABLE_HASH_IPV6_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_IP6) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                          DISABLE_HASH_IPV6_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_MPLS) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                          DISABLE_HASH_MPLS_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_MPLS) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                          DISABLE_HASH_MPLS_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_IP) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                          DISABLE_HASH_INNER_IPV4_OVER_IPV4_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_IP) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                          DISABLE_HASH_INNER_IPV4_OVER_IPV4_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_IP) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                          DISABLE_HASH_INNER_IPV6_OVER_IPV4_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_IP) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                          DISABLE_HASH_INNER_IPV6_OVER_IPV4_Bf, val);

#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_REG_FIELD_VALID(unit, RTAG7_HASH_CONTROLr, DISABLE_HASH_MIM_Af)) {
        val = (arg & BCM_HASH_FIELD0_DISABLE_MIM) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_MIM_Af, val);

        val = (arg & BCM_HASH_FIELD1_DISABLE_MIM) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_MIM_Bf, val);
    }
#endif

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_REG_FIELD_VALID(unit, RTAG7_HASH_CONTROLr, DISABLE_FCOE_HASH_Af)) {
        val = (arg & BCM_HASH_FIELD0_DISABLE_FCOE) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_FCOE_HASH_Af, val);

        val = (arg & BCM_HASH_FIELD1_DISABLE_FCOE) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_FCOE_HASH_Bf, val);
    }
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_IP6) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_IPV6_Af, val);

        val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_IP6) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_IPV6_Bf, val);

        val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_IP6) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_IPV6_Af, val);

        val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_IP6) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_IPV6_Bf, val);

        val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_GRE) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV4_Af, val);
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV6_Af, val);

        val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_GRE) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV4_Bf, val);
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV6_Bf, val);

        val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_GRE) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV4_Af, val);
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV6_Af, val);

        val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_GRE) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV4_Bf, val);
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV6_Bf, val);
    } else
#endif
    {
        val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_GRE) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_GRE_Af, val);

        val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_GRE) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_GRE_Bf, val);

        val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_GRE) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_GRE_Af, val);

        val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_GRE) ? 1 : 0;
        soc_reg64_field32_set(unit, RTAG7_HASH_CONTROLr, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_GRE_Bf, val);
    }

    BCM_IF_ERROR_RETURN(WRITE_RTAG7_HASH_CONTROLr(unit, hash_control));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_selectcontrol_get
 * Description:
 *      Get the current enhanced (aka rtag 7) hash control settings.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      arg - Pointer to where the retrieved value will be written.
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_selectcontrol_get(int unit, int *arg)
{
    uint64      hash_control;
    uint32      val;

#if defined(BCM_HURRICANE_SUPPORT)
        if(SOC_IS_HURRICANEX(unit)) {
                return (BCM_E_UNAVAIL);
        }
#endif

    *arg = 0;
    BCM_IF_ERROR_RETURN(READ_RTAG7_HASH_CONTROLr(unit, &hash_control));

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                DISABLE_HASH_IPV4_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_IP4;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                DISABLE_HASH_IPV4_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_IP4;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                DISABLE_HASH_IPV6_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_IP6;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                DISABLE_HASH_IPV6_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_IP6;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                DISABLE_HASH_MPLS_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_MPLS;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                DISABLE_HASH_MPLS_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_MPLS;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                DISABLE_HASH_INNER_IPV4_OVER_IPV4_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_IP;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                DISABLE_HASH_INNER_IPV4_OVER_IPV4_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_IP;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                DISABLE_HASH_INNER_IPV6_OVER_IPV4_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_IP;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                DISABLE_HASH_INNER_IPV6_OVER_IPV4_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_IP;

#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_REG_FIELD_VALID(unit, RTAG7_HASH_CONTROLr, DISABLE_HASH_MIM_Af)) {
        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_MIM_Af);
        if (val) *arg |= BCM_HASH_FIELD0_DISABLE_MIM;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_MIM_Bf);
        if (val) *arg |= BCM_HASH_FIELD1_DISABLE_MIM;
    }
#endif

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_REG_FIELD_VALID(unit, RTAG7_HASH_CONTROLr, DISABLE_FCOE_HASH_Af)) {
        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_FCOE_HASH_Af);
        if (val) *arg |= BCM_HASH_FIELD0_DISABLE_FCOE;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_FCOE_HASH_Bf);
        if (val) *arg |= BCM_HASH_FIELD1_DISABLE_FCOE;
    }
#endif

#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV4_OVER_IPV6_Af);
        if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_IP6;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV4_OVER_IPV6_Bf);
        if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_IP6;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV6_OVER_IPV6_Af);
        if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_IP6;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV6_OVER_IPV6_Bf);
        if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_IP6;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV4_Af);
        if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_GRE;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV4_Bf);
        if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_GRE;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV4_Af);
        if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_GRE;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV4_Bf);
        if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_GRE;
    } else
#endif
    {
        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV4_OVER_GRE_Af);
        if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_GRE;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV4_OVER_GRE_Bf);
        if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_GRE;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV6_OVER_GRE_Af);
        if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_GRE;

        val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, hash_control,
                                    DISABLE_HASH_INNER_IPV6_OVER_GRE_Bf);
        if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_GRE;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_kt2_selectcontrol_set
 * Description:
 *      Set the enhanced (aka rtag 7) hash select control.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      arg  - BCM_HASH_CONTROL*
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_kt2_selectcontrol_set(int unit, int arg)
{
    uint64      hash_control;
    uint32      val;


    BCM_IF_ERROR_RETURN(READ_RTAG7_HASH_CONTROL_64r(unit, &hash_control));

    val = (arg & BCM_HASH_FIELD0_DISABLE_IP4 ) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                          DISABLE_HASH_IPV4_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_IP4) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                          DISABLE_HASH_IPV4_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_IP6) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                          DISABLE_HASH_IPV6_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_IP6) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                          DISABLE_HASH_IPV6_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_MPLS) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                          DISABLE_HASH_MPLS_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_MPLS) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                          DISABLE_HASH_MPLS_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_IP) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                          DISABLE_HASH_INNER_IPV4_OVER_IPV4_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_IP) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                          DISABLE_HASH_INNER_IPV4_OVER_IPV4_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_IP) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                          DISABLE_HASH_INNER_IPV6_OVER_IPV4_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_IP) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                          DISABLE_HASH_INNER_IPV6_OVER_IPV4_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_MIM) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_MIM_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_MIM) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_MIM_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_IP6) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_IPV6_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_IP6) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_IPV6_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_IP6) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_IPV6_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_IP6) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_IPV6_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_GRE) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV4_Af, val);
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV6_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_GRE) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV4_Bf, val);
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV6_Bf, val);

    val = (arg & BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_GRE) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV4_Af, val);
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV6_Af, val);

    val = (arg & BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_GRE) ? 1 : 0;
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV4_Bf, val);
    soc_reg64_field32_set(unit, RTAG7_HASH_CONTROL_64r, &hash_control,
                              DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV6_Bf, val);

    BCM_IF_ERROR_RETURN(WRITE_RTAG7_HASH_CONTROL_64r(unit, hash_control));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_kt2_selectcontrol_get
 * Description:
 *      Get the current enhanced (aka rtag 7) hash control settings.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      arg - Pointer to where the retrieved value will be written.
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_kt2_selectcontrol_get(int unit, int *arg)
{
    uint64      hash_control;
    uint32      val;


    *arg = 0;
    BCM_IF_ERROR_RETURN(READ_RTAG7_HASH_CONTROL_64r(unit, &hash_control));

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                DISABLE_HASH_IPV4_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_IP4;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                DISABLE_HASH_IPV4_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_IP4;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                DISABLE_HASH_IPV6_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_IP6;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                DISABLE_HASH_IPV6_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_IP6;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                DISABLE_HASH_MPLS_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_MPLS;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                DISABLE_HASH_MPLS_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_MPLS;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                DISABLE_HASH_INNER_IPV4_OVER_IPV4_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_IP;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                DISABLE_HASH_INNER_IPV4_OVER_IPV4_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_IP;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                DISABLE_HASH_INNER_IPV6_OVER_IPV4_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_IP;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                DISABLE_HASH_INNER_IPV6_OVER_IPV4_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_IP;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                    DISABLE_HASH_MIM_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_MIM;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                    DISABLE_HASH_MIM_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_MIM;


    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                    DISABLE_HASH_INNER_IPV4_OVER_IPV6_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_IP6;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                    DISABLE_HASH_INNER_IPV4_OVER_IPV6_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_IP6;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                    DISABLE_HASH_INNER_IPV6_OVER_IPV6_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_IP6;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                    DISABLE_HASH_INNER_IPV6_OVER_IPV6_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_IP6;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                    DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV4_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP4_GRE;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                    DISABLE_HASH_INNER_IPV4_OVER_GRE_IPV4_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP4_GRE;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                    DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV4_Af);
    if (val) *arg |= BCM_HASH_FIELD0_DISABLE_TUNNEL_IP6_GRE;

    val = soc_reg64_field32_get(unit, RTAG7_HASH_CONTROL_64r, hash_control,
                                    DISABLE_HASH_INNER_IPV6_OVER_GRE_IPV4_Bf);
    if (val) *arg |= BCM_HASH_FIELD1_DISABLE_TUNNEL_IP6_GRE;

    return BCM_E_NONE;
}



enum ENHANCE_XGS3_HASH {
     /* WARNING: values given match hardware register; do not modify */
     ENHANCE_XGS3_HASH_CRC32HI    = 11,
     ENHANCE_XGS3_HASH_CRC32LO    = 10,
     ENHANCE_XGS3_HASH_CRC16CCITT = 9,
     ENHANCE_XGS3_HASH_XOR16      = 8,
     ENHANCE_XGS3_HASH_CRC16XOR8  = 7,
     ENHANCE_XGS3_HASH_CRC16XOR4  = 6,
     ENHANCE_XGS3_HASH_CRC16XOR2  = 5,
     ENHANCE_XGS3_HASH_CRC16XOR1  = 4,
     ENHANCE_XGS3_HASH_CRC16      = 3
};

/*
 * Function:
 *      _bcm_xgs3_fieldconfig_set
 * Description:
 *      Set the enhanced (aka rtag 7) field config settings.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to set.
 *      arg  - BCM_HASH_CONTROL*
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_fieldconfig_set(int unit,
                          bcm_switch_control_t type, int arg)
{
    soc_reg_t   reg;
    soc_field_t field;
    uint64      hash_control;
    uint32      val;

#if defined(BCM_HURRICANE_SUPPORT)
                if(SOC_IS_HURRICANEX(unit)) {
                        return (BCM_E_UNAVAIL);
                }
#endif

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)|| SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) {
        reg = RTAG7_HASH_CONTROL_3r;
        switch (type) {
            case bcmSwitchHashField0Config:
                field = HASH_A0_FUNCTION_SELECTf;
                break;
            case bcmSwitchHashField0Config1:
                field = HASH_A1_FUNCTION_SELECTf;
                break;
            case bcmSwitchHashField1Config:
                field = HASH_B0_FUNCTION_SELECTf;
                break;
            case bcmSwitchHashField1Config1:
                field = HASH_B1_FUNCTION_SELECTf;
                break;
            case bcmSwitchMacroFlowHashFieldConfig:
                reg = RTAG7_HASH_CONTROL_2r;
                field = MACRO_FLOW_HASH_FUNC_SELf;
                break;
            default:
                return BCM_E_UNAVAIL;
        }
        switch (arg) {
            case BCM_HASH_FIELD_CONFIG_CRC16XOR8:
                val = ENHANCE_XGS3_HASH_CRC16XOR8;
                break;
            case BCM_HASH_FIELD_CONFIG_CRC16XOR4:
                val = ENHANCE_XGS3_HASH_CRC16XOR4;
                break;
            case BCM_HASH_FIELD_CONFIG_CRC16XOR2:
                val = ENHANCE_XGS3_HASH_CRC16XOR2;
                break;
            case BCM_HASH_FIELD_CONFIG_CRC16XOR1:
                val = ENHANCE_XGS3_HASH_CRC16XOR1;
                break;
            case BCM_HASH_FIELD_CONFIG_CRC16:
                val = ENHANCE_XGS3_HASH_CRC16;
                break;
            case BCM_HASH_FIELD_CONFIG_XOR16:
                val = ENHANCE_XGS3_HASH_XOR16;
                break;
            case BCM_HASH_FIELD_CONFIG_CRC16CCITT:
                val = ENHANCE_XGS3_HASH_CRC16CCITT;
                break;
            case BCM_HASH_FIELD_CONFIG_CRC32LO:
                val = ENHANCE_XGS3_HASH_CRC32LO;
                break;
            case BCM_HASH_FIELD_CONFIG_CRC32HI:
                val = ENHANCE_XGS3_HASH_CRC32HI;
                break;
            default:
                return BCM_E_UNAVAIL;
        }
    } else
#endif /* BCM_TRIDENT_SUPPORT */
    {
        reg = RTAG7_HASH_CONTROLr;
        switch (type) {
            case bcmSwitchHashField0Config:
                field = HASH_MODE_Af;
                break;
            case bcmSwitchHashField1Config:
                field = HASH_MODE_Bf;
                break;
            default:
                return BCM_E_UNAVAIL;
        }
        switch (arg) {
            case BCM_HASH_FIELD_CONFIG_CRC16XOR8:
                val = ENHANCE_XGS3_HASH_CRC16XOR8;
                break;
            case BCM_HASH_FIELD_CONFIG_CRC16XOR4:
                val = ENHANCE_XGS3_HASH_CRC16XOR4;
                break;
            case BCM_HASH_FIELD_CONFIG_CRC16XOR2:
                val = ENHANCE_XGS3_HASH_CRC16XOR2;
                break;
            case BCM_HASH_FIELD_CONFIG_CRC16XOR1:
                val = ENHANCE_XGS3_HASH_CRC16XOR1;
                break;
            case BCM_HASH_FIELD_CONFIG_CRC16:
                val = ENHANCE_XGS3_HASH_CRC16;
                break;
            default:
                return BCM_E_UNAVAIL;
        }
    }

    if (!SOC_REG_FIELD_VALID(unit, reg, field)) {
        return BCM_E_UNAVAIL;
    }

    BCM_IF_ERROR_RETURN(soc_reg_get(unit, reg, REG_PORT_ANY, 0, &hash_control));
    soc_reg64_field32_set(unit, reg, &hash_control, field, val);
    BCM_IF_ERROR_RETURN(soc_reg_set(unit, reg, REG_PORT_ANY, 0, hash_control));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_fieldconfig_get
 * Description:
 *      Get the current enhanced (aka rtag 7) field config settings.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to retrieve.
 *      arg - Pointer to where the retrieved value will be written.
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_fieldconfig_get(int unit,
                          bcm_switch_control_t type, int *arg)
{
    soc_reg_t   reg;
    soc_field_t field;
    uint64      hash_control;
    uint32      val;

#if defined(BCM_HURRICANE_SUPPORT)
                if(SOC_IS_HURRICANEX(unit)) {
                        return (BCM_E_UNAVAIL);
                }
#endif

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) {
        reg = RTAG7_HASH_CONTROL_3r;
        switch (type) {
            case bcmSwitchHashField0Config:
                field = HASH_A0_FUNCTION_SELECTf;
                break;
            case bcmSwitchHashField0Config1:
                field = HASH_A1_FUNCTION_SELECTf;
                break;
            case bcmSwitchHashField1Config:
                field = HASH_B0_FUNCTION_SELECTf;
                break;
            case bcmSwitchHashField1Config1:
                field = HASH_B1_FUNCTION_SELECTf;
                break;
            case bcmSwitchMacroFlowHashFieldConfig:
                reg = RTAG7_HASH_CONTROL_2r;
                field = MACRO_FLOW_HASH_FUNC_SELf;
                break;
            default:
                return BCM_E_UNAVAIL;
        }
    } else
#endif /* BCM_TRIDENT_SUPPORT */
    {
        reg = RTAG7_HASH_CONTROLr;
        switch (type) {
            case bcmSwitchHashField0Config:
                field = HASH_MODE_Af;
                break;
            case bcmSwitchHashField1Config:
                field = HASH_MODE_Bf;
                break;
            default:
                return BCM_E_UNAVAIL;
        }
    }

    if (!SOC_REG_FIELD_VALID(unit, reg, field)) {
        return BCM_E_UNAVAIL;
    }
    BCM_IF_ERROR_RETURN(soc_reg_get(unit, reg, REG_PORT_ANY, 0, &hash_control));
    val = soc_reg64_field32_get(unit, reg, hash_control, field);

    switch (val) {
        case ENHANCE_XGS3_HASH_CRC16XOR8:
            *arg = BCM_HASH_FIELD_CONFIG_CRC16XOR8;
            break;
        case ENHANCE_XGS3_HASH_CRC16XOR4:
            *arg = BCM_HASH_FIELD_CONFIG_CRC16XOR4;
            break;
        case ENHANCE_XGS3_HASH_CRC16XOR2:
            *arg = BCM_HASH_FIELD_CONFIG_CRC16XOR2;
            break;
        case ENHANCE_XGS3_HASH_CRC16XOR1:
            *arg = BCM_HASH_FIELD_CONFIG_CRC16XOR1;
            break;
        case ENHANCE_XGS3_HASH_CRC16:
            *arg = BCM_HASH_FIELD_CONFIG_CRC16;
            break;
        case ENHANCE_XGS3_HASH_XOR16:
            *arg = BCM_HASH_FIELD_CONFIG_XOR16;
            break;
        case ENHANCE_XGS3_HASH_CRC16CCITT:
            *arg = BCM_HASH_FIELD_CONFIG_CRC16CCITT;
            break;
        case ENHANCE_XGS3_HASH_CRC32LO:
            *arg = BCM_HASH_FIELD_CONFIG_CRC32LO;
            break;
        case ENHANCE_XGS3_HASH_CRC32HI:
            *arg = BCM_HASH_FIELD_CONFIG_CRC32HI;
            break;
    }

    return BCM_E_NONE;
}

typedef struct {
   uint32     flag;
   uint32     hw_map;
} _hash_bmap_t;

typedef struct {
    bcm_switch_control_t        type;
    _hash_bmap_t                *map;
    int                         size;
    soc_reg_t                   reg;
    soc_field_t                 field;
} _hash_fieldselect_t;

_hash_bmap_t hash_select_common[] =
{
    { BCM_HASH_FIELD_DSTMOD,    0x0001 },
    { BCM_HASH_FIELD_DSTPORT,   0x0002 },
    { BCM_HASH_FIELD_SRCMOD,    0x0004 },
    { BCM_HASH_FIELD_SRCPORT,   0x0008 }
};

_hash_bmap_t hash_select_ip4[] =
{
    { BCM_HASH_FIELD_PROTOCOL,  0x0010 },
    { BCM_HASH_FIELD_DSTL4,     0x0020 },
    { BCM_HASH_FIELD_SRCL4,     0x0040 },
    { BCM_HASH_FIELD_VLAN,      0x0080 },
    { BCM_HASH_FIELD_IP4DST_LO, 0x0100 },
    { BCM_HASH_FIELD_IP4DST_HI, 0x0200 },
    { BCM_HASH_FIELD_IP4SRC_LO, 0x0400 },
    { BCM_HASH_FIELD_IP4SRC_HI, 0x0800 }
};

_hash_bmap_t hash_select_ip6[] =
{
    { BCM_HASH_FIELD_NXT_HDR,   0x0010 },
    { BCM_HASH_FIELD_DSTL4,     0x0020 },
    { BCM_HASH_FIELD_SRCL4,     0x0040 },
    { BCM_HASH_FIELD_VLAN,      0x0080 },
    { BCM_HASH_FIELD_IP6DST_LO, 0x0100 },
    { BCM_HASH_FIELD_IP6DST_HI, 0x0200 },
    { BCM_HASH_FIELD_IP6SRC_LO, 0x0400 },
    { BCM_HASH_FIELD_IP6SRC_HI, 0x0800 }
};

_hash_bmap_t hash_select_l2[] =
{
    { BCM_HASH_FIELD_VLAN,      0x0010 },
    { BCM_HASH_FIELD_ETHER_TYPE,0x0020 },
    { BCM_HASH_FIELD_MACDA_LO,  0x0040 },
    { BCM_HASH_FIELD_MACDA_MI,  0x0080 },
    { BCM_HASH_FIELD_MACDA_HI,  0x0100 },
    { BCM_HASH_FIELD_MACSA_LO,  0x0200 },
    { BCM_HASH_FIELD_MACSA_MI,  0x0400 },
    { BCM_HASH_FIELD_MACSA_HI,  0x0800 }
};

_hash_bmap_t hash_select_mpls[] =
{
    { BCM_HASH_FIELD_VLAN,       0x0010 },
    { BCM_HASH_FIELD_IP4DST_LO,  0x0020 },
    { BCM_HASH_FIELD_IP4DST_HI,  0x0040 },
    { BCM_HASH_FIELD_IP4SRC_LO,  0x0080 },
    { BCM_HASH_FIELD_IP4SRC_HI,  0x0100 },
    { BCM_HASH_FIELD_IP6DST_LO,  0x0020 },
    { BCM_HASH_FIELD_IP6DST_HI,  0x0040 },
    { BCM_HASH_FIELD_IP6SRC_LO,  0x0080 },
    { BCM_HASH_FIELD_IP6SRC_HI,  0x0100 },
    { BCM_HASH_FIELD_RSVD_LABELS,0x0200 },
    { BCM_HASH_FIELD_2ND_LABEL,  0x0400 },
    { BCM_HASH_FIELD_TOP_LABEL,  0x0800 }
};

_hash_bmap_t hash_select_unknown[] =
{
    { BCM_HASH_FIELD_LOAD_BALANCE,0x0010 }
};


_hash_bmap_t hash_field_payload_control[] =
{
    { BCM_HASH_SELECT_INNER_L2, 0x0000 },
    { BCM_HASH_SELECT_INNER_L3, 0x0001 }
};

_hash_bmap_t hash_field_tunnel_control[]=
{
    { BCM_HASH_SELECT_OUTER_L2,         0x0000},
    { BCM_HASH_SELECT_TUNNEL_INNER_L2,  0x0002},
    { BCM_HASH_SELECT_INNER_L3,         0x0003}
};


_hash_bmap_t hash_field_ipv6_collapse_control[] =
{
    { BCM_HASH_IP6_COLLAPSE_XOR, 0x0000},
    { BCM_HASH_IP6_COLLAPSE_LSB, 0x0001}
};


_hash_fieldselect_t hash_select_info[] =
{
    { bcmSwitchHashIP4Field0,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_HASH_FIELD_BMAP_1r, IPV4_FIELD_BITMAP_Af},
    { bcmSwitchHashIP4Field1,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_HASH_FIELD_BMAP_1r, IPV4_FIELD_BITMAP_Bf},
    { bcmSwitchHashIP6Field0,
        hash_select_ip6, COUNTOF(hash_select_ip6),
        RTAG7_HASH_FIELD_BMAP_2r, IPV6_FIELD_BITMAP_Af},
    { bcmSwitchHashIP6Field1,
        hash_select_ip6, COUNTOF(hash_select_ip6),
        RTAG7_HASH_FIELD_BMAP_2r, IPV6_FIELD_BITMAP_Bf},
    { bcmSwitchHashL2Field0,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_HASH_FIELD_BMAP_3r, L2_FIELD_BITMAP_Af},
    { bcmSwitchHashL2Field1,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_HASH_FIELD_BMAP_3r, L2_FIELD_BITMAP_Bf},
    { bcmSwitchHashMPLSField0,
        hash_select_mpls, COUNTOF(hash_select_mpls),
        RTAG7_HASH_FIELD_BMAP_4r, MPLS_FIELD_BITMAP_Af},
    { bcmSwitchHashMPLSField1,
        hash_select_mpls, COUNTOF(hash_select_mpls),
        RTAG7_HASH_FIELD_BMAP_4r, MPLS_FIELD_BITMAP_Bf},
    { bcmSwitchHashHG2UnknownField0,
        hash_select_unknown, COUNTOF(hash_select_unknown),
        RTAG7_HASH_FIELD_BMAP_5r, UNKNOWN_PPD_FIELD_BITMAP_Af},
    { bcmSwitchHashHG2UnknownField1,
        hash_select_unknown, COUNTOF(hash_select_unknown),
        RTAG7_HASH_FIELD_BMAP_5r, UNKNOWN_PPD_FIELD_BITMAP_Bf}

};

#if defined(BCM_TRIUMPH2_SUPPORT)
_hash_bmap_t hash_select_mpls_l2[] =
{
    { BCM_HASH_FIELD_VLAN,      0x0010 },
    { BCM_HASH_FIELD_ETHER_TYPE,0x0020 },
    { BCM_HASH_FIELD_MACDA_LO,  0x0040 },
    { BCM_HASH_FIELD_MACDA_MI,  0x0080 },
    { BCM_HASH_FIELD_MACDA_HI,  0x0100 },
    { BCM_HASH_FIELD_MACSA_LO,  0x0200 },
    { BCM_HASH_FIELD_MACSA_MI,  0x0400 },
    { BCM_HASH_FIELD_MACSA_HI,  0x0800 }
};

_hash_bmap_t hash_select_mpls_l3[] =
{
    { BCM_HASH_FIELD_PROTOCOL,  0x0010 },
    { BCM_HASH_FIELD_DSTL4,     0x0020 },
    { BCM_HASH_FIELD_SRCL4,     0x0040 },
    { BCM_HASH_FIELD_VLAN,      0x0080 },
    { BCM_HASH_FIELD_IP4DST_LO, 0x0100 },
    { BCM_HASH_FIELD_IP4DST_HI, 0x0200 },
    { BCM_HASH_FIELD_IP4SRC_LO, 0x0400 },
    { BCM_HASH_FIELD_IP4SRC_HI, 0x0800 }
};

_hash_bmap_t hash_select_mpls_tunnel[] =
{
    { BCM_HASH_FIELD_2ND_LABEL,  0x0010 },
    { BCM_HASH_FIELD_TOP_LABEL,  0x0020 },
    { BCM_HASH_FIELD_MACDA_LO,  0x0040 },
    { BCM_HASH_FIELD_MACDA_MI,  0x0080 },
    { BCM_HASH_FIELD_MACDA_HI,  0x0100 },
    { BCM_HASH_FIELD_MACSA_LO,  0x0200 },
    { BCM_HASH_FIELD_MACSA_MI,  0x0400 },
    { BCM_HASH_FIELD_MACSA_HI,  0x0800 }
};

_hash_bmap_t hash_select_mim[] =
{
    { BCM_HASH_FIELD_VLAN,      0x0010 },
    { BCM_HASH_FIELD_ETHER_TYPE,0x0020 },
    { BCM_HASH_FIELD_MACDA_LO,  0x0040 },
    { BCM_HASH_FIELD_MACDA_MI,  0x0080 },
    { BCM_HASH_FIELD_MACDA_HI,  0x0100 },
    { BCM_HASH_FIELD_MACSA_LO,  0x0200 },
    { BCM_HASH_FIELD_MACSA_MI,  0x0400 },
    { BCM_HASH_FIELD_MACSA_HI,  0x0800 }
};

_hash_bmap_t hash_select_mim_tunnel[] =
{
    { BCM_HASH_FIELD_LOOKUP_ID_LO, 0x0010 },
    { BCM_HASH_FIELD_LOOKUP_ID_HI, 0x0020 },
    { BCM_HASH_FIELD_MACDA_LO,     0x0040 },
    { BCM_HASH_FIELD_MACDA_MI,     0x0080 },
    { BCM_HASH_FIELD_MACDA_HI,     0x0100 },
    { BCM_HASH_FIELD_MACSA_LO,     0x0200 },
    { BCM_HASH_FIELD_MACSA_MI,     0x0400 },
    { BCM_HASH_FIELD_MACSA_HI,     0x0800 }
};

_hash_fieldselect_t hash_select_info_tr2[] =
{
    { bcmSwitchHashIP4Field0,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_HASH_FIELD_BMAP_1r, IPV4_FIELD_BITMAP_Af},
    { bcmSwitchHashIP4Field1,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_HASH_FIELD_BMAP_1r, IPV4_FIELD_BITMAP_Bf},
    { bcmSwitchHashIP6Field0,
        hash_select_ip6, COUNTOF(hash_select_ip6),
        RTAG7_HASH_FIELD_BMAP_2r, IPV6_FIELD_BITMAP_Af},
    { bcmSwitchHashIP6Field1,
        hash_select_ip6, COUNTOF(hash_select_ip6),
        RTAG7_HASH_FIELD_BMAP_2r, IPV6_FIELD_BITMAP_Bf},
    { bcmSwitchHashL2Field0,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_HASH_FIELD_BMAP_3r, L2_FIELD_BITMAP_Af},
    { bcmSwitchHashL2Field1,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_HASH_FIELD_BMAP_3r, L2_FIELD_BITMAP_Bf},
    { bcmSwitchHashL2MPLSField0,
        hash_select_mpls_l2, COUNTOF(hash_select_mpls_l2),
        RTAG7_MPLS_L2_PAYLOAD_HASH_FIELD_BMAPr, MPLS_L2_PAYLOAD_BITMAP_Af},
    { bcmSwitchHashL2MPLSField1,
        hash_select_mpls_l2, COUNTOF(hash_select_mpls_l2),
        RTAG7_MPLS_L2_PAYLOAD_HASH_FIELD_BMAPr, MPLS_L2_PAYLOAD_BITMAP_Bf},
    { bcmSwitchHashL3MPLSField0,
        hash_select_mpls_l3, COUNTOF(hash_select_mpls_l3),
        RTAG7_MPLS_L3_PAYLOAD_HASH_FIELD_BMAPr, MPLS_L3_PAYLOAD_BITMAP_Af},
    { bcmSwitchHashL3MPLSField1,
        hash_select_mpls_l3, COUNTOF(hash_select_mpls_l3),
        RTAG7_MPLS_L3_PAYLOAD_HASH_FIELD_BMAPr, MPLS_L3_PAYLOAD_BITMAP_Bf},
    { bcmSwitchHashMPLSTunnelField0,
        hash_select_mpls_tunnel, COUNTOF(hash_select_mpls_tunnel),
        RTAG7_MPLS_OUTER_HASH_FIELD_BMAPr, MPLS_OUTER_BITMAP_Af},
    { bcmSwitchHashMPLSTunnelField1,
        hash_select_mpls_tunnel, COUNTOF(hash_select_mpls_tunnel),
        RTAG7_MPLS_OUTER_HASH_FIELD_BMAPr, MPLS_OUTER_BITMAP_Bf},
    { bcmSwitchHashMIMField0,
        hash_select_mim, COUNTOF(hash_select_mim),
        RTAG7_MIM_PAYLOAD_HASH_FIELD_BMAPr, MIM_PAYLOAD_BITMAP_Af},
    { bcmSwitchHashMIMField1,
        hash_select_mim, COUNTOF(hash_select_mim),
        RTAG7_MIM_PAYLOAD_HASH_FIELD_BMAPr, MIM_PAYLOAD_BITMAP_Bf},
    { bcmSwitchHashMIMTunnelField0,
        hash_select_mim_tunnel, COUNTOF(hash_select_mim_tunnel),
        RTAG7_MIM_OUTER_HASH_FIELD_BMAPr, MIM_OUTER_BITMAP_Af},
    { bcmSwitchHashMIMTunnelField1,
        hash_select_mim_tunnel, COUNTOF(hash_select_mim_tunnel),
        RTAG7_MIM_OUTER_HASH_FIELD_BMAPr, MIM_OUTER_BITMAP_Bf},
    { bcmSwitchHashHG2UnknownField0,
        hash_select_unknown, COUNTOF(hash_select_unknown),
        RTAG7_HASH_FIELD_BMAP_5r, UNKNOWN_PPD_FIELD_BITMAP_Af},
    { bcmSwitchHashHG2UnknownField1,
        hash_select_unknown, COUNTOF(hash_select_unknown),
        RTAG7_HASH_FIELD_BMAP_5r, UNKNOWN_PPD_FIELD_BITMAP_Bf}
};
#endif

#ifdef BCM_TRIDENT_SUPPORT
_hash_bmap_t hash_select_td_common[] =
{
    { BCM_HASH_FIELD_DSTMOD,        0x0001 },
    { BCM_HASH_FIELD_DSTPORT,       0x0002 },
    { BCM_HASH_FIELD_SRCMOD,        0x0004 },
    { BCM_HASH_FIELD_SRCPORT,       0x0008 },
    { BCM_HASH_FIELD_CNTAG_FLOW_ID, 0x1000 }
};
_hash_bmap_t hash_select_td_mpls_tunnel[] =
{
    { BCM_HASH_MPLS_FIELD_3RD_LABEL,     0x0010 },
    { BCM_HASH_MPLS_FIELD_IP4SRC_LO,     0x0020 },
    { BCM_HASH_MPLS_FIELD_IP4SRC_HI,     0x0040 },
    { BCM_HASH_MPLS_FIELD_IP4DST_LO,     0x0080 },
    { BCM_HASH_MPLS_FIELD_IP4DST_HI,     0x0100 },
    { BCM_HASH_MPLS_FIELD_LABELS_4MSB,   0x0200 },
    { BCM_HASH_MPLS_FIELD_2ND_LABEL,     0x0400 },
    { BCM_HASH_MPLS_FIELD_TOP_LABEL,     0x0800 },
};

_hash_bmap_t hash_select_td_fcoe[] =
{
    { BCM_HASH_FCOE_FIELD_VLAN,                   0x0010 },
    { BCM_HASH_FCOE_FIELD_VIRTUAL_FABRIC_ID,      0x0020 },
    { BCM_HASH_FCOE_FIELD_RESPONDER_EXCHANGE_ID,  0x0040 },
    { BCM_HASH_FCOE_FIELD_ORIGINATOR_EXCHANGE_ID, 0x0080 },
    { BCM_HASH_FCOE_FIELD_DST_ID_LO,              0x0100 },
    { BCM_HASH_FCOE_FIELD_DST_ID_HI,              0x0200 },
    { BCM_HASH_FCOE_FIELD_SRC_ID_LO,              0x0400 },
    { BCM_HASH_FCOE_FIELD_SRC_ID_HI,              0x0800 },
};

_hash_bmap_t hash_select_td_trill[] =
{
    { BCM_HASH_TRILL_FIELD_VLAN,                  0x0010 },
    { BCM_HASH_TRILL_FIELD_ETHER_TYPE,            0x0020 },
    { BCM_HASH_TRILL_FIELD_MACSA_LO,              0x0040 },
    { BCM_HASH_TRILL_FIELD_MACSA_MI,              0x0080 },
    { BCM_HASH_TRILL_FIELD_MACDA_LO,              0x0100 },
    { BCM_HASH_TRILL_FIELD_MACDA_MI,              0x0200 },
    { BCM_HASH_TRILL_FIELD_ING_RBRIDGE_NAME,      0x0400 },
    { BCM_HASH_TRILL_FIELD_EGR_RBRIDGE_NAME,      0x0800 },
};

_hash_fieldselect_t hash_select_info_td[] =
{
    { bcmSwitchHashIP4Field0,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_HASH_FIELD_BMAP_1r, IPV4_FIELD_BITMAP_Af},
    { bcmSwitchHashIP4Field1,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_HASH_FIELD_BMAP_1r, IPV4_FIELD_BITMAP_Bf},
    { bcmSwitchHashIP4TcpUdpField0,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_IPV4_TCP_UDP_HASH_FIELD_BMAP_2r, IPV4_TCP_UDP_FIELD_BITMAP_Af},
    { bcmSwitchHashIP4TcpUdpField1,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_IPV4_TCP_UDP_HASH_FIELD_BMAP_2r, IPV4_TCP_UDP_FIELD_BITMAP_Bf},
    { bcmSwitchHashIP4TcpUdpPortsEqualField0,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_IPV4_TCP_UDP_HASH_FIELD_BMAP_1r,
        IPV4_TCP_UDP_SRC_EQ_DST_FIELD_BITMAP_Af},
    { bcmSwitchHashIP4TcpUdpPortsEqualField1,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_IPV4_TCP_UDP_HASH_FIELD_BMAP_1r,
        IPV4_TCP_UDP_SRC_EQ_DST_FIELD_BITMAP_Bf},
    { bcmSwitchHashIP6Field0,
        hash_select_ip6, COUNTOF(hash_select_ip6),
        RTAG7_HASH_FIELD_BMAP_2r, IPV6_FIELD_BITMAP_Af},
    { bcmSwitchHashIP6Field1,
        hash_select_ip6, COUNTOF(hash_select_ip6),
        RTAG7_HASH_FIELD_BMAP_2r, IPV6_FIELD_BITMAP_Bf},
    { bcmSwitchHashIP6TcpUdpField0,
        hash_select_ip6, COUNTOF(hash_select_ip6),
        RTAG7_IPV6_TCP_UDP_HASH_FIELD_BMAP_2r, IPV6_TCP_UDP_FIELD_BITMAP_Af},
    { bcmSwitchHashIP6TcpUdpField1,
        hash_select_ip6, COUNTOF(hash_select_ip6),
        RTAG7_IPV6_TCP_UDP_HASH_FIELD_BMAP_2r, IPV6_TCP_UDP_FIELD_BITMAP_Bf},
    { bcmSwitchHashIP6TcpUdpPortsEqualField0,
        hash_select_ip6, COUNTOF(hash_select_ip6),
        RTAG7_IPV6_TCP_UDP_HASH_FIELD_BMAP_1r,
        IPV6_TCP_UDP_SRC_EQ_DST_FIELD_BITMAP_Af},
    { bcmSwitchHashIP6TcpUdpPortsEqualField1,
        hash_select_ip6, COUNTOF(hash_select_ip6),
        RTAG7_IPV6_TCP_UDP_HASH_FIELD_BMAP_1r,
        IPV6_TCP_UDP_SRC_EQ_DST_FIELD_BITMAP_Bf},
    { bcmSwitchHashL2Field0,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_HASH_FIELD_BMAP_3r, L2_FIELD_BITMAP_Af},
    { bcmSwitchHashL2Field1,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_HASH_FIELD_BMAP_3r, L2_FIELD_BITMAP_Bf},
    { bcmSwitchHashL2MPLSField0,
        hash_select_mpls_l2, COUNTOF(hash_select_mpls_l2),
        RTAG7_MPLS_L2_PAYLOAD_HASH_FIELD_BMAPr, MPLS_L2_PAYLOAD_BITMAP_Af},
    { bcmSwitchHashL2MPLSField1,
        hash_select_mpls_l2, COUNTOF(hash_select_mpls_l2),
        RTAG7_MPLS_L2_PAYLOAD_HASH_FIELD_BMAPr, MPLS_L2_PAYLOAD_BITMAP_Bf},
    { bcmSwitchHashL3MPLSField0,
        hash_select_mpls_l3, COUNTOF(hash_select_mpls_l3),
        RTAG7_MPLS_L3_PAYLOAD_HASH_FIELD_BMAPr, MPLS_L3_PAYLOAD_BITMAP_Af},
    { bcmSwitchHashL3MPLSField1,
        hash_select_mpls_l3, COUNTOF(hash_select_mpls_l3),
        RTAG7_MPLS_L3_PAYLOAD_HASH_FIELD_BMAPr, MPLS_L3_PAYLOAD_BITMAP_Bf},
    { bcmSwitchHashMPLSTunnelField0,
        hash_select_td_mpls_tunnel, COUNTOF(hash_select_td_mpls_tunnel),
        RTAG7_MPLS_OUTER_HASH_FIELD_BMAPr, MPLS_OUTER_BITMAP_Af},
    { bcmSwitchHashMPLSTunnelField1,
        hash_select_td_mpls_tunnel, COUNTOF(hash_select_mpls_tunnel),
        RTAG7_MPLS_OUTER_HASH_FIELD_BMAPr, MPLS_OUTER_BITMAP_Bf},
    { bcmSwitchHashMIMField0,
        hash_select_mim, COUNTOF(hash_select_mim),
        RTAG7_MIM_PAYLOAD_HASH_FIELD_BMAPr, MIM_PAYLOAD_BITMAP_Af},
    { bcmSwitchHashMIMField1,
        hash_select_mim, COUNTOF(hash_select_mim),
        RTAG7_MIM_PAYLOAD_HASH_FIELD_BMAPr, MIM_PAYLOAD_BITMAP_Bf},
    { bcmSwitchHashMIMTunnelField0,
        hash_select_mim_tunnel, COUNTOF(hash_select_mim_tunnel),
        RTAG7_MIM_OUTER_HASH_FIELD_BMAPr, MIM_OUTER_BITMAP_Af},
    { bcmSwitchHashMIMTunnelField1,
        hash_select_mim_tunnel, COUNTOF(hash_select_mim_tunnel),
        RTAG7_MIM_OUTER_HASH_FIELD_BMAPr, MIM_OUTER_BITMAP_Bf},
    { bcmSwitchHashFCOEField0,
        hash_select_td_fcoe, COUNTOF(hash_select_td_fcoe),
        RTAG7_FCOE_HASH_FIELD_BMAPr, FCOE_FIELD_BITMAP_Af},
    { bcmSwitchHashFCOEField1,
        hash_select_td_fcoe, COUNTOF(hash_select_td_fcoe),
        RTAG7_FCOE_HASH_FIELD_BMAPr, FCOE_FIELD_BITMAP_Bf},
    { bcmSwitchHashL2TrillField0,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_TRILL_PAYLOAD_L2_HASH_FIELD_BMAPr, TRILL_PAYLOAD_L2_BITMAP_Af},
    { bcmSwitchHashL2TrillField1,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_TRILL_PAYLOAD_L2_HASH_FIELD_BMAPr, TRILL_PAYLOAD_L2_BITMAP_Bf},
    { bcmSwitchHashL3TrillField0,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_TRILL_PAYLOAD_L3_HASH_FIELD_BMAPr, TRILL_PAYLOAD_L3_BITMAP_Af},
    { bcmSwitchHashL3TrillField1,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_TRILL_PAYLOAD_L3_HASH_FIELD_BMAPr, TRILL_PAYLOAD_L3_BITMAP_Bf},
    { bcmSwitchHashTrillTunnelField0,
        hash_select_td_trill, COUNTOF(hash_select_td_trill),
        RTAG7_TRILL_TUNNEL_HASH_FIELD_BMAPr, TRILL_TUNNEL_BITMAP_Af},
    { bcmSwitchHashTrillTunnelField1,
        hash_select_td_trill, COUNTOF(hash_select_td_trill),
        RTAG7_TRILL_TUNNEL_HASH_FIELD_BMAPr, TRILL_TUNNEL_BITMAP_Bf},
    { bcmSwitchHashL2VxlanField0,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_VXLAN_PAYLOAD_L2_HASH_FIELD_BMAPr, VXLAN_PAYLOAD_L2_BITMAP_Af},
    { bcmSwitchHashL2VxlanField1,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_VXLAN_PAYLOAD_L2_HASH_FIELD_BMAPr, VXLAN_PAYLOAD_L2_BITMAP_Bf},
    { bcmSwitchHashL3VxlanField0,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_VXLAN_PAYLOAD_L3_HASH_FIELD_BMAPr, VXLAN_PAYLOAD_L3_BITMAP_Af},
    { bcmSwitchHashL3VxlanField1,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_VXLAN_PAYLOAD_L3_HASH_FIELD_BMAPr, VXLAN_PAYLOAD_L3_BITMAP_Bf},
     { bcmSwitchHashL2L2GreField0,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_L2GRE_PAYLOAD_L2_HASH_FIELD_BMAPr, L2GRE_PAYLOAD_L2_BITMAP_Af},
    { bcmSwitchHashL2L2GreField1,
        hash_select_l2, COUNTOF(hash_select_l2),
        RTAG7_L2GRE_PAYLOAD_L2_HASH_FIELD_BMAPr, L2GRE_PAYLOAD_L2_BITMAP_Bf},
    { bcmSwitchHashL3L2GreField0,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_L2GRE_PAYLOAD_L3_HASH_FIELD_BMAPr, L2GRE_PAYLOAD_L3_BITMAP_Af},
    { bcmSwitchHashL3L2GreField1,
        hash_select_ip4, COUNTOF(hash_select_ip4),
        RTAG7_L2GRE_PAYLOAD_L3_HASH_FIELD_BMAPr, L2GRE_PAYLOAD_L3_BITMAP_Bf},
    { bcmSwitchHashHG2UnknownField0,
        hash_select_unknown, COUNTOF(hash_select_unknown),
        RTAG7_HASH_FIELD_BMAP_5r, UNKNOWN_PPD_FIELD_BITMAP_Af},
    { bcmSwitchHashHG2UnknownField1,
        hash_select_unknown, COUNTOF(hash_select_unknown),
        RTAG7_HASH_FIELD_BMAP_5r, UNKNOWN_PPD_FIELD_BITMAP_Bf}
};

_hash_fieldselect_t hash_select_control_td[] =
{
    { bcmSwitchHashTrillPayloadSelect0, 
       hash_field_payload_control, COUNTOF(hash_field_payload_control),
       RTAG7_HASH_CONTROLr, TRILL_PAYLOAD_HASH_SELECT_Af},
    { bcmSwitchHashTrillPayloadSelect1, 
       hash_field_payload_control, COUNTOF(hash_field_payload_control),
       RTAG7_HASH_CONTROLr, TRILL_PAYLOAD_HASH_SELECT_Bf},
    { bcmSwitchHashTrillTunnelSelect0, 
       hash_field_tunnel_control, COUNTOF(hash_field_tunnel_control),
       RTAG7_HASH_CONTROLr, TRILL_TUNNEL_HASH_SELECT_Af},
    { bcmSwitchHashTrillTunnelSelect1, 
       hash_field_tunnel_control, COUNTOF(hash_field_tunnel_control),
       RTAG7_HASH_CONTROLr, TRILL_TUNNEL_HASH_SELECT_Bf},
    { bcmSwitchHashIP6AddrCollapseSelect0, 
       hash_field_ipv6_collapse_control, COUNTOF(hash_field_ipv6_collapse_control),
       RTAG7_HASH_CONTROLr, IPV6_COLLAPSED_ADDR_SELECT_Af},
    { bcmSwitchHashIP6AddrCollapseSelect1, 
       hash_field_ipv6_collapse_control, COUNTOF(hash_field_ipv6_collapse_control),
       RTAG7_HASH_CONTROLr, IPV6_COLLAPSED_ADDR_SELECT_Bf}
};

_hash_fieldselect_t hash_select_control_kt2[] =
{
    { bcmSwitchHashIP6AddrCollapseSelect0, 
       hash_field_ipv6_collapse_control, COUNTOF(hash_field_ipv6_collapse_control),
       RTAG7_HASH_CONTROL_64r, IPV6_COLLAPSED_ADDR_SELECT_Af},
    { bcmSwitchHashIP6AddrCollapseSelect1, 
       hash_field_ipv6_collapse_control, COUNTOF(hash_field_ipv6_collapse_control),
       RTAG7_HASH_CONTROL_64r, IPV6_COLLAPSED_ADDR_SELECT_Bf}
};

#endif /* BCM_TRIDENT_SUPPORT */

/*
 * Function:
 *      _bcm_xgs3_fieldselect_set
 * Description:
 *      Set the enhanced (aka rtag 7) field select settings.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to set.
 *      arg  - BCM_HASH_CONTROL*
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_fieldselect_set(int unit,
                          bcm_switch_control_t type, int arg)
{
    uint32   hash_control;
    int      i, j, count, common_count;
    uint32   flags = 0;
    _hash_fieldselect_t *info, *base_info = hash_select_info;
    _hash_bmap_t *common_bmap;

#if defined(BCM_HURRICANE_SUPPORT)
                        if(SOC_IS_HURRICANEX(unit)) {
                                return (BCM_E_UNAVAIL);
                        }
#endif

    common_bmap = hash_select_common;
    common_count = COUNTOF(hash_select_common);
    count = COUNTOF(hash_select_info);
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_KATANAX(unit)) {
#ifdef BCM_TRIDENT_SUPPORT
        if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
            SOC_IS_KATANAX(unit)) {
            common_bmap = hash_select_td_common;
            common_count = COUNTOF(hash_select_td_common);
            base_info = hash_select_info_td;
            count = COUNTOF(hash_select_info_td);
        } else
#endif /* BCM_TRIDENT_SUPPORT */
        {
            base_info = hash_select_info_tr2;
            count = COUNTOF(hash_select_info_tr2);
        }
    }
#endif

    for (i = 0; i < count; i++) {
         info = base_info + i;
         if (info->type != type) {
             continue;
         }

         for (j = 0; j < common_count; j++) {
              if (common_bmap[j].flag & arg) {
                  flags |= common_bmap[j].hw_map;
              }
         }

         for (j = 0; j < info->size; j++) {
              if (info->map[j].flag & arg) {
                  flags |= info->map[j].hw_map;
              }
         }

         SOC_IF_ERROR_RETURN
             (soc_reg32_get(unit, info->reg,
                             REG_PORT_ANY, 0, &hash_control));
         soc_reg_field_set(unit, info->reg,
                           &hash_control, info->field, flags);
         SOC_IF_ERROR_RETURN
             (soc_reg32_set(unit, info->reg, REG_PORT_ANY, 0, hash_control));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_fieldselect_get
 * Description:
 *      Get the current enhanced (aka rtag 7) field select settings.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to retrieve.
 *      arg - Pointer to where the retrieved value will be written.
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_fieldselect_get(int unit,
                          bcm_switch_control_t type, int *arg)
{
    uint32   hash_control;
    int      i, j, count, common_count;
    uint32   flags;
    _hash_fieldselect_t *info, *base_info = hash_select_info;
    _hash_bmap_t *common_bmap;

#if defined(BCM_HURRICANE_SUPPORT)
        if(SOC_IS_HURRICANEX(unit)) {
                return (BCM_E_UNAVAIL);
        }
#endif

    common_bmap = hash_select_common;
    common_count = COUNTOF(hash_select_common);
    count = COUNTOF(hash_select_info);
#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_KATANAX(unit)) {
#ifdef BCM_TRIDENT_SUPPORT
        if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
            SOC_IS_KATANAX(unit)) {
            common_bmap = hash_select_td_common;
            common_count = COUNTOF(hash_select_td_common);
            base_info = hash_select_info_td;
            count = COUNTOF(hash_select_info_td);
        } else
#endif /* BCM_TRIDENT_SUPPORT */
        {
            base_info = hash_select_info_tr2;
            count = COUNTOF(hash_select_info_tr2);
        }
    }
#endif

    for (i = 0; i < count; i++) {
         info = base_info + i;
         if (info->type != type) {
             continue;
         }

         SOC_IF_ERROR_RETURN
             (soc_reg32_get(unit, info->reg,
                             REG_PORT_ANY, 0, &hash_control));
         flags = soc_reg_field_get(unit, info->reg,
                                   hash_control, info->field);
         *arg = 0;
         for (j = 0; j < common_count; j++) {
              if (common_bmap[j].hw_map & flags) {
                  *arg |= common_bmap[j].flag;
              }
         }

         for (j = 0; j < info->size; j++) {
              if (info->map[j].hw_map & flags) {
                  *arg |= info->map[j].flag;
              }
         }
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_xgs3_field_control_get
 * Description:
 *      Set RTAG7 field controls for hash blocks.
 * Parameters:
 *      unit - unit no.
 *      type - switch control enum type.
 *      arg  - BCM_HASH_CONTROL*
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_field_control_get(int unit,
                            bcm_switch_control_t type, int *arg)
{
    uint64   hash_control;
    uint32   val;
    int      cnt, i;
    uint32   count = 0;
    _hash_fieldselect_t *base_info = NULL, *info;

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_KATANA(unit)) {
        base_info = hash_select_control_td;
        count = COUNTOF(hash_select_control_td);
    }
    else if (SOC_IS_KATANA2(unit)) {
        base_info = hash_select_control_kt2;
        count = COUNTOF(hash_select_control_kt2);
    } 
#endif /* BCM_TRIDENT_SUPPORT */

    if (0 == count) {
        return BCM_E_UNAVAIL;
    }

    for (i = 0; i < count; i++) 
    {
        info = base_info + i;
        if (info->type != type) {
            continue;
        }

        SOC_IF_ERROR_RETURN
             (soc_reg_get(unit, info->reg,
                             REG_PORT_ANY, 0, &hash_control));

        val = soc_reg64_field32_get(unit, info->reg,
                                   hash_control, info->field);

        for (cnt = 0; cnt < info->size; cnt++) {

            if (info->map[cnt].hw_map == val) {
                break;
            }
        }
        if (info->size == cnt) {
            return BCM_E_INTERNAL;
        }

        *arg = info->map[cnt].flag;

    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_field_control_set
 * Description:
 *      Get RTAG7 field controls for hash blocks.
 * Parameters:
 *      unit - unit no.
 *      type - switch control enum type.
 *      *arg  - BCM_HASH_CONTROL*
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_field_control_set(int unit,
                            bcm_switch_control_t type, int arg)
{
    uint64   hash_control;
    int      cnt, i;
    uint32   count = 0;
    _hash_fieldselect_t *base_info = NULL, *info;

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_KATANA(unit)) {
        base_info = hash_select_control_td;
        count = COUNTOF(hash_select_control_td);
    } 
    else if (SOC_IS_KATANA2(unit)) {
        base_info = hash_select_control_kt2;
        count = COUNTOF(hash_select_control_kt2);
    } 
#endif /* BCM_TRIDENT_SUPPORT */

    if (0 == count) {
        return BCM_E_UNAVAIL;
    }

    for (i = 0; i < count; i++) 
    {
        info = base_info + i;
        if (info->type != type) {
            continue;
        }

        for (cnt = 0; cnt < info->size; cnt++) {
            if (info->map[cnt].flag == arg) {
                break;
            } 
        }
        if (info->size == cnt) {
            return BCM_E_PARAM;
        }

         SOC_IF_ERROR_RETURN
             (soc_reg_get(unit, info->reg,
                             REG_PORT_ANY, 0, &hash_control));
         soc_reg64_field32_set(unit, info->reg,
                           &hash_control, info->field, info->map[cnt].hw_map);
         SOC_IF_ERROR_RETURN
             (soc_reg_set(unit, info->reg, REG_PORT_ANY, 0, hash_control));
    }

    return BCM_E_NONE;
}


typedef struct {
    int         idx;
    int         hash_concat;
    int         regmem;
    soc_field_t sub_f;
    soc_field_t offset_f;
    soc_field_t concat_f;
} hash_offset_info_t;

/*
 * Enhanced hash bit selection/offset helper function
 */

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int
_bcm_tr3_hash_offset(int unit, bcm_port_t port, bcm_switch_control_t type,
                     hash_offset_info_t *info)
{
    
    /* Resolve to Physical port */
    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_switch_control_gport_resolve(unit, port, &port));
    }

    /* The RTAG7 port index is of 0 .. 319. The indexes are arranged with 0 -
     * 255 lport followed  by physical ports index 0 .. 63 
     */
    info->idx = port + soc_mem_index_count(unit, LPORT_TABm);

    switch (type) {
        case bcmSwitchTrunkHashSet0UnicastOffset:
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_TRUNK_UCf;
            info->offset_f = OFFSET_TRUNK_UCf;
            info->concat_f = CONCATENATE_HASH_FIELDS_TRUNK_UCf;
            info->regmem = RTAG7_PORT_BASED_HASHm;
            break;
        case bcmSwitchTrunkHashSet0NonUnicastOffset:
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_TRUNK_NONUCf;
            info->offset_f = OFFSET_TRUNK_NONUCf;
            info->concat_f = CONCATENATE_HASH_FIELDS_TRUNK_NONUCf;
            info->regmem = RTAG7_PORT_BASED_HASHm;
            break;
        case bcmSwitchTrunkFailoverHashOffset:
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_PLFSf;
            info->offset_f = OFFSET_PLFSf;
            info->concat_f = CONCATENATE_HASH_FIELDS_PLFSf;
            info->regmem = RTAG7_PORT_BASED_HASHm;
            break;
        case bcmSwitchFabricTrunkHashSet0UnicastOffset:
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_HG_TRUNK_UCf;
            info->offset_f = OFFSET_HG_TRUNK_UCf;
            info->concat_f = CONCATENATE_HASH_FIELDS_HG_TRUNK_UCf;
            info->regmem = RTAG7_PORT_BASED_HASHm;
            break;
        case bcmSwitchFabricTrunkHashSet0NonUnicastOffset:
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_HG_TRUNK_NONUCf;
            info->offset_f = OFFSET_HG_TRUNK_NONUCf;
            info->concat_f = CONCATENATE_HASH_FIELDS_HG_TRUNK_NONUCf;
            info->regmem = RTAG7_PORT_BASED_HASHm;
            break;
        case bcmSwitchFabricTrunkFailoverHashOffset:
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_HG_TRUNK_FAILOVERf;
            info->offset_f = OFFSET_HG_TRUNK_FAILOVERf;
            info->concat_f = CONCATENATE_HASH_FIELDS_HG_TRUNK_FAILOVERf;
            info->regmem = RTAG7_PORT_BASED_HASHm;
            break;
        case bcmSwitchFabricTrunkDynamicHashOffset:
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_DLB_HGTf;
            info->offset_f = OFFSET_DLB_HGTf;
            info->concat_f = CONCATENATE_HASH_FIELDS_DLB_HGTf;
            info->regmem = RTAG7_PORT_BASED_HASHm;
            break;
        case bcmSwitchTrunkDynamicHashOffset:
            if (SOC_IS_TRIUMPH3(unit)) {
                info->hash_concat = 0;
                info->sub_f = SUB_SEL_DLB_LAGf;
                info->offset_f = OFFSET_DLB_LAGf;
                info->concat_f = CONCATENATE_HASH_FIELDS_DLB_LAGf;
                info->regmem = RTAG7_PORT_BASED_HASHm;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchEcmpDynamicHashOffset:
            if (soc_feature(unit, soc_feature_ecmp_dlb)) {
                info->hash_concat = 0;
                info->sub_f = SUB_SEL_DLB_ECMPf;
                info->offset_f = OFFSET_DLB_ECMPf;
                info->concat_f = CONCATENATE_HASH_FIELDS_DLB_ECMPf;
                info->regmem = RTAG7_PORT_BASED_HASHm;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchFabricTrunkResilientHashOffset:
            if (soc_feature(unit, soc_feature_hg_resilient_hash)) {
                info->hash_concat = 0;
                info->sub_f = SUB_SEL_RH_HGTf;
                info->offset_f = OFFSET_RH_HGTf;
                info->concat_f = CONCATENATE_HASH_FIELDS_RH_HGTf;
                info->regmem = RTAG7_PORT_BASED_HASHm;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchTrunkResilientHashOffset:
            if (soc_feature(unit, soc_feature_lag_resilient_hash)) {
                info->hash_concat = 0;
                info->sub_f = SUB_SEL_RH_LAGf;
                info->offset_f = OFFSET_RH_LAGf;
                info->concat_f = CONCATENATE_HASH_FIELDS_RH_LAGf;
                info->regmem = RTAG7_PORT_BASED_HASHm;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchEcmpResilientHashOffset:
            if (soc_feature(unit, soc_feature_ecmp_resilient_hash)) {
                info->hash_concat = 0;
                info->sub_f = SUB_SEL_RH_ECMPf;
                info->offset_f = OFFSET_RH_ECMPf;
                info->concat_f = CONCATENATE_HASH_FIELDS_RH_ECMPf;
                info->regmem = RTAG7_PORT_BASED_HASHm;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchECMPVxlanHashOffset:
            if (SOC_IS_TRIDENT2(unit)) {
                info->hash_concat = 0;
                info->sub_f = SUB_SEL_VXLAN_ECMPf;
                info->offset_f = OFFSET_VXLAN_ECMPf;
                info->concat_f = CONCATENATE_HASH_FIELDS_VXLAN_ECMPf;
                info->regmem = RTAG7_PORT_BASED_HASHm;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchECMPL2GreHashOffset:
            if (SOC_IS_KATANA2(unit)) {
                return BCM_E_UNAVAIL;
            } else {
                info->hash_concat = 0;
                info->sub_f = SUB_SEL_L2GRE_ECMPf;
                info->offset_f = OFFSET_L2GRE_ECMPf;
                info->concat_f = CONCATENATE_HASH_FIELDS_L2GRE_ECMPf;
                info->regmem = RTAG7_PORT_BASED_HASHm;
            }
            break;
        case bcmSwitchECMPTrillHashOffset:
            if (SOC_IS_KATANA2(unit)) {
                return BCM_E_UNAVAIL;
            } else {
                info->hash_concat = 0;
                info->sub_f = SUB_SEL_TRILL_ECMPf;
                info->offset_f = OFFSET_TRILL_ECMPf;
                info->concat_f = CONCATENATE_HASH_FIELDS_TRILL_ECMPf;
                info->regmem = RTAG7_PORT_BASED_HASHm;
            }
            break;
        case bcmSwitchECMPMplsHashOffset:
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_MPLS_ECMPf;
            info->offset_f = OFFSET_MPLS_ECMPf;
            info->concat_f = CONCATENATE_HASH_FIELDS_MPLS_ECMPf;
            info->regmem = RTAG7_PORT_BASED_HASHm;
            break;
        case bcmSwitchVirtualPortDynamicHashOffset:
            if (SOC_IS_TRIDENT2(unit)) {
                info->hash_concat = 0;
                info->sub_f = SUB_SEL_VPLAGf;
                info->offset_f = OFFSET_VPLAGf;
                info->concat_f = CONCATENATE_HASH_FIELDS_VPLAGf;
                info->regmem = RTAG7_PORT_BASED_HASHm;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchLoadBalanceHashSet0UnicastOffset:
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_LBID_UCf;
            info->offset_f = OFFSET_LBID_UCf;
            info->concat_f = CONCATENATE_HASH_FIELDS_LBID_UCf;
            info->regmem =  RTAG7_PORT_BASED_HASHm;
            break;
        case bcmSwitchLoadBalanceHashSet0NonUnicastOffset:
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_LBID_NONUCf;
            info->offset_f = OFFSET_LBID_NONUCf;
            info->concat_f = CONCATENATE_HASH_FIELDS_LBID_NONUCf;
            info->regmem =  RTAG7_PORT_BASED_HASHm;
            break;
        case bcmSwitchECMPHashSet0Offset:
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_ECMPf;
            info->offset_f = OFFSET_ECMPf;
            info->concat_f = CONCATENATE_HASH_FIELDS_ECMPf;
            info->regmem = RTAG7_PORT_BASED_HASHm;
            break;
        case bcmSwitchEntropyHashSet0Offset:
            info->hash_concat = 1; /* required; mpls label is of 20 bits */
            info->sub_f = SUB_SEL_ENTROPY_LABELf;
            info->offset_f = OFFSET_ENTROPY_LABELf;
            info->concat_f = CONCATENATE_HASH_FIELDS_ENTROPY_LABELf;
            info->regmem = RTAG7_PORT_BASED_HASHm;
            break;
        default:
            return BCM_E_UNAVAIL;
    }
    return BCM_E_NONE;
}
#endif /* BCM_TRIUMPH3_SUPPORT */


STATIC int
_bcm_hash_offset(int unit, bcm_port_t port, bcm_switch_control_t type,
                 hash_offset_info_t *info)
{
    switch (type) {
        case bcmSwitchTrunkHashSet0UnicastOffset:
            info->idx = 0;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_UCf;
            info->offset_f = OFFSET_UCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_TRUNKr;
            break;
        case bcmSwitchTrunkHashSet0NonUnicastOffset:
            info->idx = 0;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_NONUCf;
            info->offset_f = OFFSET_NONUCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_TRUNKr;
            break;
        case bcmSwitchTrunkHashSet1NonUnicastOffset:
            info->idx = 1;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_NONUCf;
            info->offset_f = OFFSET_NONUCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_TRUNKr;
            break;
        case bcmSwitchTrunkHashSet1UnicastOffset:
            info->idx = 1;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_UCf;
            info->offset_f = OFFSET_UCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_TRUNKr;
            break;
        case bcmSwitchTrunkFailoverHashOffset:
            info->idx = 0;
            info->hash_concat = 0;
            info->sub_f = SUB_SELf;
            info->offset_f = OFFSETf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_PLFSr;
            break;
        case bcmSwitchFabricTrunkHashSet0UnicastOffset:
            info->idx = 0;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_UCf;
            info->offset_f = OFFSET_UCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_HG_TRUNKr;
            break;
        case bcmSwitchFabricTrunkHashSet0NonUnicastOffset:
            info->idx = 0;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_NONUCf;
            info->offset_f = OFFSET_NONUCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_HG_TRUNKr;
            break;
        case bcmSwitchFabricTrunkHashSet1UnicastOffset:
            info->idx = 1;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_UCf;
            info->offset_f = OFFSET_UCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_HG_TRUNKr;
            break;
        case bcmSwitchFabricTrunkHashSet1NonUnicastOffset:
            info->idx = 1;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_NONUCf;
            info->offset_f = OFFSET_NONUCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_HG_TRUNKr;
            break;
        case bcmSwitchFabricTrunkFailoverHashOffset:
            info->idx = 0;
            info->hash_concat = 0;
            info->sub_f = SUB_SELf;
            info->offset_f = OFFSETf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_HG_TRUNK_FAILOVERr;
            break;
        case bcmSwitchFabricTrunkDynamicHashOffset:
            if (SOC_REG_IS_VALID(unit, RTAG7_HASH_DLB_HGTr)) {
                info->idx = 0;
                info->hash_concat = 0;
                info->sub_f = SUB_SELf;
                info->offset_f = OFFSETf;
                info->concat_f = -1;
                info->regmem = RTAG7_HASH_DLB_HGTr;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchLoadBalanceHashSet0UnicastOffset:
            info->idx = 0;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_UCf;
            info->offset_f = OFFSET_UCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_LBIDr;
            break;
        case bcmSwitchLoadBalanceHashSet0NonUnicastOffset:
            info->idx = 0;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_NONUCf;
            info->offset_f = OFFSET_NONUCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_LBIDr;
            break;
        case bcmSwitchLoadBalanceHashSet1UnicastOffset:
            info->idx = 1;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_UCf;
            info->offset_f = OFFSET_UCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_LBIDr;
            break;
        case bcmSwitchLoadBalanceHashSet1NonUnicastOffset:
            info->idx = 1;
            info->hash_concat = 0;
            info->sub_f = SUB_SEL_NONUCf;
            info->offset_f = OFFSET_NONUCf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_LBIDr;
            break;
        case bcmSwitchECMPHashSet0Offset:
            info->idx = 0;
            info->hash_concat = 0;
            info->sub_f = SUB_SELf;
            info->offset_f = OFFSETf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_ECMPr;
            break;
        case bcmSwitchECMPHashSet1Offset:
            info->idx = 1;
            info->hash_concat = 0;
            info->sub_f = SUB_SELf;
            info->offset_f = OFFSETf;
            info->concat_f = -1;
            info->regmem = RTAG7_HASH_ECMPr;
            break;
 #ifdef BCM_KATANA_SUPPORT
        case bcmSwitchEntropyHashSet0Offset:
            info->idx = 0;
            info->hash_concat = 1;
            info->sub_f = SUB_SEL_ENTROPY_LABELf;
            info->offset_f = OFFSET_ENTROPY_LABELf;
            info->concat_f = CONCATENATE_HASH_FIELDS_ENTROPY_LABELf;
            info->regmem = RTAG7_HASH_ENTROPY_LABELr;
            break;
        case bcmSwitchEntropyHashSet1Offset:
            info->idx = 1;
            info->hash_concat = 1;
            info->sub_f = SUB_SEL_ENTROPY_LABELf;
            info->offset_f = OFFSET_ENTROPY_LABELf;
            info->concat_f = CONCATENATE_HASH_FIELDS_ENTROPY_LABELf;
            info->regmem = RTAG7_HASH_ENTROPY_LABELr;
            break;
#endif /* BCM_KATANA_SUPPORT */
        default:
            return BCM_E_UNAVAIL;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_fieldoffset_set
 * Description:
 *      Set the enhanced (aka rtag 7) bits selection offset.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      port - port
 *      type - The desired configuration parameter to set.
 *      arg  - BCM_HASH_CONTROL*
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_fieldoffset_set(int unit, bcm_port_t port,
                          bcm_switch_control_t type, int arg)
{
    uint32      hash_control;
    int         sub_field_width[8];
    int         total_width, offset = -1, i;
    hash_offset_info_t info;

#if defined(BCM_HURRICANE_SUPPORT) 
        if(SOC_IS_HURRICANEX(unit)) {
                return (BCM_E_UNAVAIL);
        }
#endif

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit) || 
        SOC_IS_KATANA2(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_hash_offset(unit, port, type, &info));
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        BCM_IF_ERROR_RETURN(_bcm_hash_offset(unit, port, type, &info));
    }


#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) {
        sub_field_width[0] = 16;  /* HASH_A0 (16bit) */
        sub_field_width[1] = 16;  /* HASH_B0 (16bit) */
        sub_field_width[2] = 4;   /* LBN (4bit)      */
        sub_field_width[3] = 16;  /* MH.DPORT/ HASH_A0(16bit)           */
        sub_field_width[4] = 8;   /* MH.LBID / IRSEL local LBID (8 bit) */
        sub_field_width[5] = 8;   /* SW1 LBID ( 8 bit)*/
        sub_field_width[6] = 16;  /* HASH_A1 (16 bit) */
        sub_field_width[7] = 16;  /* HASH_B1 (16 bit) */
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT || BCM_TRIDENT_SUPPORT */
    {
        sub_field_width[0] = 16; /* HASH_A0 (16bit) */
        sub_field_width[1] = 16; /* HASH_B0 (16bit) */
        sub_field_width[2] = 4;  /* LBN (4bit)      */
        sub_field_width[3] = 5;  
        sub_field_width[4] = 8;
        sub_field_width[5] = 0;
        sub_field_width[6] = 0;
        sub_field_width[7] = 0;
    }

    /* Get hash bits width */
    total_width = 0;
    for (i = 0; i < 8; i++) {
        total_width += sub_field_width[i];
    }
    /* Concatenate if offset value exceeds total hash width */
    if (arg > total_width) {
        info.hash_concat = 1;
    }

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_KATANA_SUPPORT)
    /* Concatenate if enforced or if offset value exceeds total hash width */
    if ((info.hash_concat == 1)  && (info.concat_f != -1)) {
        /* Concatenation hash computation order is referenced from 
         *  fb_Irsel1_arch::compute_rtag7_hash 
         */
        if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD_TT(unit)) {
            sub_field_width[0] = 64;  /* HASH_A0 B0 A1 B1(64bit)*/
            sub_field_width[1] = 4;   /* LBN (4bit)      */
            sub_field_width[2] = 16;  /* MH.DPORT/ HASH_A0(16bit)           */
            sub_field_width[3] = 8;   /* MH.LBID / IRSEL local LBID (8 bit) */
            sub_field_width[4] = 8;   /* SW1 LBID ( 8 bit)*/
            sub_field_width[5] = 16;  /* HASH_A1 (16 bit) */
            sub_field_width[6] = 0;
            sub_field_width[7] = 0;
        } else  if (SOC_IS_KATANA(unit)) {
            sub_field_width[0] = 64; /* HASH_A0 B0 A1 B1(64bit)*/
            sub_field_width[1] = 4;  /* LBN (4bit) */
            sub_field_width[2] = 5;
            sub_field_width[3] = 8;  
            sub_field_width[4] = 0;
            sub_field_width[5] = 0;
            sub_field_width[6] = 0;
            sub_field_width[7] = 0;
        }

        /* Get concatenated hash bits width */
        total_width = 0;
        for (i = 0; i < 8; i++) {
            total_width += sub_field_width[i];
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT || BCM_TRIDENT_SUPPORT || BCM_KATANA_SUPPORT */

    /* Select hash sub select and hash bit offset */
    offset = arg % total_width;
    for (i = 0; i < 8; i++) {
         offset -= sub_field_width[i];
         if (offset < 0) {
             offset += sub_field_width[i];
             break;
         }
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_MEM_IS_VALID(unit, info.regmem)) {
        uint32 hash_control_entry[SOC_MAX_MEM_WORDS];
        SOC_IF_ERROR_RETURN
            (soc_mem_read(unit, info.regmem,
                          MEM_BLOCK_ANY, info.idx, hash_control_entry));
        soc_mem_field32_set(unit, 
                            info.regmem, hash_control_entry, info.sub_f, i);
        soc_mem_field32_set(unit, 
                            info.regmem, hash_control_entry, info.offset_f, offset);
        if ((info.hash_concat == 1) && (info.concat_f != -1)) {
            soc_mem_field32_set(unit, 
                                info.regmem, hash_control_entry, info.concat_f, 1);
        }
        SOC_IF_ERROR_RETURN
            (soc_mem_write(unit, 
                           info.regmem, MEM_BLOCK_ALL, info.idx, hash_control_entry));  
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
    if (SOC_REG_IS_VALID(unit, info.regmem)) {
        SOC_IF_ERROR_RETURN
            (soc_reg32_get(unit, info.regmem,
                           REG_PORT_ANY, info.idx, &hash_control));
        soc_reg_field_set(unit, 
                          info.regmem, &hash_control, info.sub_f, i);
        soc_reg_field_set(unit, 
                          info.regmem, &hash_control, info.offset_f, offset);
        if ((info.hash_concat == 1) && (info.concat_f != -1)) {
            soc_reg_field_set(unit, 
                             info.regmem, &hash_control, info.concat_f, 1);
        }
        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, 
                           info.regmem, REG_PORT_ANY, info.idx, hash_control));
    }    

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_fieldoffset_get
 * Description:
 *      Get the current enhanced (aka rtag 7) bit selection offset.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      type - The desired configuration parameter to retrieve.
 *      arg - Pointer to where the retrieved value will be written.
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_xgs3_fieldoffset_get(int unit, bcm_port_t port,
                          bcm_switch_control_t type, int *arg)
{
    uint32      hash_control;
    int         sub_field_width[8];
    int         idx = 0, i;
    hash_offset_info_t info;
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_KATANA_SUPPORT)
    int         concat = 0;
#endif

#if defined(BCM_HURRICANE_SUPPORT)
    if(SOC_IS_HURRICANEX(unit)) {
            return (BCM_E_UNAVAIL);
    }
#endif

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit) ||
        SOC_IS_KATANA2(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_tr3_hash_offset(unit, port, type, &info));
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        BCM_IF_ERROR_RETURN(_bcm_hash_offset(unit, port, type, &info));
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_MEM_IS_VALID(unit, info.regmem)) {
        uint32 hash_control_entry[SOC_MAX_MEM_WORDS];
        SOC_IF_ERROR_RETURN
            (soc_mem_read(unit, info.regmem,
                            MEM_BLOCK_ANY, info.idx, hash_control_entry));
        idx = soc_mem_field32_get(unit, info.regmem, hash_control_entry, info.sub_f);
        *arg  = soc_mem_field32_get(unit, info.regmem, hash_control_entry, info.offset_f);
        if (info.concat_f != -1) {
            concat = soc_mem_field32_get(unit, info.regmem, hash_control_entry, info.concat_f);
        }
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
    if (SOC_REG_IS_VALID(unit, info.regmem)) {
        SOC_IF_ERROR_RETURN
            (soc_reg32_get(unit, info.regmem, REG_PORT_ANY, info.idx, &hash_control));
        idx = soc_reg_field_get(unit, info.regmem, hash_control, info.sub_f);
        *arg  = soc_reg_field_get(unit, info.regmem, hash_control, info.offset_f);
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_KATANA_SUPPORT)
        if (info.concat_f != -1) {
            concat = soc_reg_field_get(unit, info.regmem, hash_control, info.concat_f);
        }
#endif
    }

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) {
        sub_field_width[0] = 16;  /* HASH_A0 (16bit) */
        sub_field_width[1] = 16;  /* HASH_B0 (16bit) */
        sub_field_width[2] = 4;   /* LBN (4bit)      */
        sub_field_width[3] = 16;  /* MH.DPORT/ HASH_A0(16bit)           */
        sub_field_width[4] = 8;   /* MH.LBID / IRSEL local LBID (8 bit) */
        sub_field_width[5] = 8;   /* SW1 LBID ( 8 bit)*/
        sub_field_width[6] = 16;  /* HASH_A1 (16 bit) */
        sub_field_width[7] = 16;  /* HASH_B1 (16 bit) */
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT || BCM_TRIDENT_SUPPORT */
    {
        sub_field_width[0] = 16; /* HASH_A0 (16bit) */
        sub_field_width[1] = 16; /* HASH_B0 (16bit) */
        sub_field_width[2] = 4;  /* LBN (4bit)      */
        sub_field_width[3] = 5;  
        sub_field_width[4] = 8;
        sub_field_width[5] = 0;
        sub_field_width[6] = 0;
        sub_field_width[7] = 0;
    }


#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_KATANA_SUPPORT)
    if (concat) {
        if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) {
            /* Concatenation hash computation order is referenced from 
             *  fb_Irsel1_arch::compute_rtag7_hash 
             */
            sub_field_width[0] = 64;  /* HASH_A0 B0 A1 B1(64bit)*/
            sub_field_width[1] = 4;   /* LBN (4bit)      */
            sub_field_width[2] = 16;  /* MH.DPORT/ HASH_A0(16bit)           */
            sub_field_width[3] = 8;   /* MH.LBID / IRSEL local LBID (8 bit) */
            sub_field_width[4] = 8;   /* SW1 LBID ( 8 bit)*/
            sub_field_width[5] = 16;  /* HASH_A1 (16 bit) */
            sub_field_width[6] = 0;
            sub_field_width[7] = 0;
        } else if (SOC_IS_KATANA(unit)) {
            sub_field_width[0] = 64; /* HASH_A0 B0 A1 B1(64bit)*/
            sub_field_width[1] = 4;  /* LBN (4bit) */
            sub_field_width[2] = 5;
            sub_field_width[3] = 8;  
            sub_field_width[4] = 0;
            sub_field_width[5] = 0;
            sub_field_width[6] = 0;
            sub_field_width[7] = 0;
        }
    } 
#endif /* BCM_TRIDENT_SUPPORT || BCM_TRIUMPH3_SUPPORT || BCM_KATANA_SUPPORT*/ 

    for (i = 0; i < idx; i++) {
         *arg += sub_field_width[i];
    }

    return BCM_E_NONE;
}
#endif /*  BCM_BRADLEY_SUPPORT || BCM_TRX_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
STATIC int
_bcm_td_macroflow_param_get(int unit, int **sub_field_base, int *min_offset,
                            int *max_offset, int *stride_offset)
{
    /* sub field width: 16, 16, 4, 16, 8, 8, 16, 16 */
    static int base[] = { 0, 16, 32, 36, 52, 60, 68, 84, 100 };
    rtag7_flow_based_hash_entry_t entry;
    int min, max, stride;
    int index, offset;
    uint32 fval, sub_sel;

    if (!SOC_MEM_IS_VALID(unit, RTAG7_FLOW_BASED_HASHm)) {
        return BCM_E_UNAVAIL;
    }

    SOC_IF_ERROR_RETURN
        (soc_mem_read(unit, RTAG7_FLOW_BASED_HASHm, MEM_BLOCK_ANY, 0, &entry));
    sub_sel = soc_mem_field32_get(unit, RTAG7_FLOW_BASED_HASHm, &entry,
                                  SUB_SEL_ECMPf);
    fval = soc_mem_field32_get(unit, RTAG7_FLOW_BASED_HASHm, &entry,
                               OFFSET_ECMPf);
    min = base[sub_sel] + fval;

    SOC_IF_ERROR_RETURN
        (soc_mem_read(unit, RTAG7_FLOW_BASED_HASHm, MEM_BLOCK_ANY, 1, &entry));
    sub_sel = soc_mem_field32_get(unit, RTAG7_FLOW_BASED_HASHm, &entry,
                                  SUB_SEL_ECMPf);
    fval = soc_mem_field32_get(unit, RTAG7_FLOW_BASED_HASHm, &entry,
                               OFFSET_ECMPf);
    max = base[sub_sel] + fval;
    stride = max - min;

    if (stride != 0) {
        for (index = 2;
             index <= soc_mem_index_max(unit, RTAG7_FLOW_BASED_HASHm);
             index++) {
            SOC_IF_ERROR_RETURN
                (soc_mem_read(unit, RTAG7_FLOW_BASED_HASHm, MEM_BLOCK_ANY,
                              index, &entry));
            sub_sel = soc_mem_field32_get(unit, RTAG7_FLOW_BASED_HASHm, &entry,
                                          SUB_SEL_ECMPf);
            fval = soc_mem_field32_get(unit, RTAG7_FLOW_BASED_HASHm, &entry,
                                       OFFSET_ECMPf);
            offset = base[sub_sel] + fval;
            if (offset != max + stride) {
                break;
            }
            max = offset;
        }
    }

    if (sub_field_base != NULL) {
        *sub_field_base = base;
    }
    if (min_offset != NULL) {
        *min_offset = min;
    }
    if (max_offset != NULL) {
        *max_offset = max;
    }
    if (stride_offset != NULL) {
        *stride_offset = stride;
    }
    return BCM_E_NONE;
}


STATIC int
_bcm_td_macroflow_offset_set(int unit, bcm_switch_control_t type, int arg)
{
    int *sub_field_base;
    rtag7_flow_based_hash_entry_t entry;
    int min_offset, max_offset, stride_offset;
    int index, offset;
    uint32 sub_sel;

    BCM_IF_ERROR_RETURN
        (_bcm_td_macroflow_param_get(unit, &sub_field_base, &min_offset,
                                     &max_offset, &stride_offset));

    if (arg < -1 || arg >= sub_field_base[8]) {
        return BCM_E_PARAM;
    }

    switch (type) {
    case bcmSwitchMacroFlowHashMinOffset:
        if (arg == min_offset) {
            return BCM_E_NONE;
        } else if (arg > max_offset) {
            return BCM_E_PARAM;
        }
        min_offset = arg == -1 ? 0 : arg;
        break;
    case bcmSwitchMacroFlowHashMaxOffset:
        if (arg == max_offset) {
            return BCM_E_NONE;
        } else if (arg < min_offset) {
            return BCM_E_PARAM;
        }
        max_offset = arg == -1 ? sub_field_base[8] - 1 : arg;
        break;
    case bcmSwitchMacroFlowHashStrideOffset:
        if (arg == stride_offset) {
            return BCM_E_NONE;
        } else if (arg < 0) {
            return BCM_E_PARAM;
        }
        stride_offset = arg;
        break;
    default:
        return BCM_E_INTERNAL;
    }

    if (stride_offset == 0 && min_offset != max_offset) {
        stride_offset = 1;
    }

    if ((type == bcmSwitchMacroFlowHashStrideOffset) && 
        (stride_offset == 1) && (min_offset == max_offset)) {
        max_offset += stride_offset;
    }
    sal_memset(&entry, 0, sizeof(entry));
    offset = min_offset;
    for (index = 0; index <= soc_mem_index_max(unit, RTAG7_FLOW_BASED_HASHm);
         index++) {
        for (sub_sel = 0; sub_sel < 7; sub_sel++) {
            if (offset < sub_field_base[sub_sel + 1]) {
                break;
            }
        }

        soc_mem_field32_set(unit, RTAG7_FLOW_BASED_HASHm, &entry,
                            SUB_SEL_ECMPf, sub_sel);
        soc_mem_field32_set(unit, RTAG7_FLOW_BASED_HASHm, &entry,
                            OFFSET_ECMPf, offset - sub_field_base[sub_sel]);
        BCM_IF_ERROR_RETURN
            (soc_mem_write(unit, RTAG7_FLOW_BASED_HASHm, MEM_BLOCK_ANY,
                           index, &entry));
        offset += stride_offset;
        if (offset > max_offset) {
            offset = min_offset;
        }
    }

    return BCM_E_NONE;
}
#endif /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
STATIC int
_bcm_td2_random_hash_seed_get(int unit, hash_offset_info_t *hash_info, int seed, 
                             int *offset, int *subsel)
{
    int concat, random;
    uint32 hash_entry[SOC_MAX_MEM_WORDS];
    
    /* Check for hash Concatenation */
     SOC_IF_ERROR_RETURN
            (soc_mem_read(unit, hash_info->regmem, 
                          MEM_BLOCK_ANY, 0, &hash_entry));
     concat =  soc_mem_field32_get(unit, hash_info->regmem, &hash_entry,
                                    hash_info->concat_f);

    /* Generate random number for the seed as described in C standards */
    random = seed * 1103515245 + 12345;
    /* random_seed / 65536) % 32768 */
    random = (random >> 16) & 0x7FFF;

    if (concat) {
        *offset = random & 0x3f;
        /* Use HASH A0_A1_B0_B1 */
        *subsel = 0; 
    } else {
        *offset = random & 0xf;
        /* User any of hash A0, A1, B0, B1 */
        *subsel = (random & 0x30) >> 4;
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_td2_macroflow_hash_get(int unit, bcm_switch_control_t type, int *arg)
{
    uint32          hash_entry[SOC_MAX_MEM_WORDS];
    hash_offset_info_t hash_info;

    sal_memset(&hash_info, 0, sizeof(hash_offset_info_t));
    hash_info.regmem = RTAG7_FLOW_BASED_HASHm;

    switch(type)
    {
        /* Get RTAG7 Macro Flow Concatenation */
        case bcmSwitchMacroFlowEcmpHashConcatEnable:
            hash_info.concat_f = CONCATENATE_HASH_FIELDS_ECMPf;
            break;

        case bcmSwitchMacroFlowLoadBalanceHashConcatEnable:
            hash_info.concat_f = CONCATENATE_HASH_FIELDS_LBID_OR_ENTROPY_LABELf;
            break;

        case bcmSwitchMacroFlowTrunkHashConcatEnable:
            hash_info.concat_f = CONCATENATE_HASH_FIELDS_TRUNKf;
            break;

        case bcmSwitchMacroFlowHigigTrunkHashConcatEnable:
            hash_info.concat_f = CONCATENATE_HASH_FIELDS_HG_TRUNKf;
            break;

         /* Get RTAG7 Macro Flow Hash Seed cannot be supported as the
          * seed values being programmed are random. 
          */
        case bcmSwitchMacroFlowECMPHashSeed:
        case bcmSwitchMacroFlowLoadBalanceHashSeed:
        case bcmSwitchMacroFlowTrunkHashSeed:
        case bcmSwitchMacroFlowHigigTrunkHashSeed:
        default:
            return BCM_E_PARAM;
    }

    if (soc_mem_is_valid(unit, hash_info.regmem)) {
        sal_memset(&hash_entry, 0, SOC_MAX_MEM_WORDS);

         SOC_IF_ERROR_RETURN
        (soc_mem_read(unit, hash_info.regmem, 
                      MEM_BLOCK_ANY, 0, &hash_entry));
        *arg =  soc_mem_field32_get(unit, hash_info.regmem, &hash_entry,
                                hash_info.concat_f);
    } else {
        return BCM_E_UNAVAIL;
    }

    return BCM_E_NONE;
}


STATIC int
_bcm_td2_macroflow_hash_set(int unit, bcm_switch_control_t type, int arg)
{
    soc_mem_t       hash_sel_regmem;
    uint32          hash_entry[SOC_MAX_MEM_WORDS];
    hash_offset_info_t hash_info;
    soc_field_t     hash_flow_select[5];
    uint32          rval;
    int             index = 0, min, max;
    int             offset=0, subsel=0;

    sal_memset(&hash_info, 0, sizeof(hash_offset_info_t));
    sal_memset(&hash_flow_select, 0, sizeof(hash_flow_select));

    hash_sel_regmem = RTAG7_HASH_SELr;
    hash_info.regmem = RTAG7_FLOW_BASED_HASHm;

    switch(type)
    {
        /* Set RTAG7 Macro Flow Concatenation */
        case bcmSwitchMacroFlowEcmpHashConcatEnable:
            hash_info.concat_f = CONCATENATE_HASH_FIELDS_ECMPf;
            hash_info.hash_concat = 1;
            break;

        case bcmSwitchMacroFlowLoadBalanceHashConcatEnable:
            hash_info.concat_f = CONCATENATE_HASH_FIELDS_LBID_OR_ENTROPY_LABELf;
            hash_info.hash_concat = 1;
            break;

        case bcmSwitchMacroFlowTrunkHashConcatEnable:
            hash_info.concat_f = CONCATENATE_HASH_FIELDS_TRUNKf;
            hash_info.hash_concat = 1;
            break;

        case bcmSwitchMacroFlowHigigTrunkHashConcatEnable:
            hash_info.concat_f = CONCATENATE_HASH_FIELDS_HG_TRUNKf;
            hash_info.hash_concat = 1;
            break;

        /* Set RTAG7 Macro Flow Hash Seed */
        case bcmSwitchMacroFlowECMPHashSeed:
            hash_info.sub_f     = SUB_SEL_ECMPf;
            hash_info.offset_f  = OFFSET_ECMPf;
            hash_info.concat_f  = CONCATENATE_HASH_FIELDS_ECMPf;
            hash_flow_select[index++] = USE_FLOW_SEL_MPLS_ECMPf;
            hash_flow_select[index++] = USE_FLOW_SEL_L2GRE_ECMPf;
            hash_flow_select[index++] = USE_FLOW_SEL_VXLAN_ECMPf;
            hash_flow_select[index++] = USE_FLOW_SEL_RH_ECMPf;
            break;

        case bcmSwitchMacroFlowLoadBalanceHashSeed:
            hash_info.sub_f     = SUB_SEL_LBID_OR_ENTROPY_LABELf;
            hash_info.offset_f  = OFFSET_LBID_OR_ENTROPY_LABELf;
            hash_info.concat_f = CONCATENATE_HASH_FIELDS_LBID_OR_ENTROPY_LABELf;
            hash_flow_select[index++] = USE_FLOW_SEL_LBID_UCf;
            hash_flow_select[index++] = USE_FLOW_SEL_LBID_NONUCf;
            hash_flow_select[index++] = USE_FLOW_SEL_ENTROPY_LABELf;
            break;

        case bcmSwitchMacroFlowTrunkHashSeed:
            hash_info.sub_f     = SUB_SEL_TRUNKf;
            hash_info.offset_f  = OFFSET_TRUNKf;
            hash_info.concat_f = CONCATENATE_HASH_FIELDS_TRUNKf;
            hash_flow_select[index++] = USE_FLOW_SEL_RH_LAGf;
            hash_flow_select[index++] = USE_FLOW_SEL_VPLAGf;
            hash_flow_select[index++] = USE_FLOW_SEL_TRUNK_NONUCf;
            break;

        case bcmSwitchMacroFlowHigigTrunkHashSeed:
            hash_info.sub_f     = SUB_SEL_HG_TRUNKf;
            hash_info.offset_f  = OFFSET_HG_TRUNKf;
            hash_info.concat_f = CONCATENATE_HASH_FIELDS_HG_TRUNKf;
            hash_flow_select[index++] = USE_FLOW_SEL_HG_TRUNK_FAILOVERf;
            hash_flow_select[index++] = USE_FLOW_SEL_HG_TRUNK_NONUCf;
            hash_flow_select[index++] = USE_FLOW_SEL_DLB_HGTf;
            hash_flow_select[index++] = USE_FLOW_SEL_RH_HGTf;
            break;

        default:
            return BCM_E_PARAM;
    }

    if (!soc_mem_is_valid(unit, hash_info.regmem)) {
        return BCM_E_UNAVAIL;
    }
    min   = soc_mem_index_min(unit, hash_info.regmem);
    max   = soc_mem_index_max(unit, hash_info.regmem);

    if (hash_info.hash_concat) {
        min   = soc_mem_index_min(unit, hash_info.regmem);
        max   = soc_mem_index_max(unit, hash_info.regmem);
        for (index = min; index <= max; index++ ) {
            SOC_IF_ERROR_RETURN
                (soc_mem_read(unit, hash_info.regmem, 
                              MEM_BLOCK_ANY, index, &hash_entry));
            soc_mem_field32_set(unit, hash_info.regmem, &hash_entry,
                                hash_info.concat_f, (arg > 0) ? 1 : 0);
            SOC_IF_ERROR_RETURN
                (soc_mem_write(unit, hash_info.regmem,
                               MEM_BLOCK_ALL, index, &hash_entry));
        }
    } else {
        /* Set random generated hash offset and sub select */
        SOC_IF_ERROR_RETURN
            (_bcm_td2_random_hash_seed_get(unit, &hash_info, arg, 
                                                &offset, &subsel));

        for (index = min; index <= max; index++ ) {
            SOC_IF_ERROR_RETURN
                (soc_mem_read(unit, hash_info.regmem, 
                              MEM_BLOCK_ANY, index, &hash_entry));
            soc_mem_field32_set(unit, hash_info.regmem, &hash_entry,
                                hash_info.sub_f, subsel);
            soc_mem_field32_set(unit, hash_info.regmem, &hash_entry,
                                hash_info.offset_f, offset);
            SOC_IF_ERROR_RETURN
                (soc_mem_write(unit, hash_info.regmem,
                               MEM_BLOCK_ALL, index, &hash_entry));
        }

        /* Enable Macro flow bits */
        SOC_IF_ERROR_RETURN
            (soc_reg32_get(unit, hash_sel_regmem, REG_PORT_ANY, 0, &rval));

        for (index = 0; 
            index < (sizeof(hash_flow_select)/sizeof(soc_field_t)); index++ ) {
            if (soc_reg_field_valid(unit, hash_sel_regmem,
                                    hash_flow_select[index])) {
                soc_reg_field_set(unit, hash_sel_regmem, &rval, hash_flow_select[index], 1);
            }
        }

        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, hash_sel_regmem, REG_PORT_ANY, 0, rval));
    }

    return BCM_E_NONE;
}
#endif /* BCM_TRIDENT2_SUPPORT || BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIDENT2_SUPPORT)
STATIC int
_bcm_td2_port_asf_enable_set(int unit, bcm_port_t port, int enable)
{
    uint32 rval;
    int phy_port;
    egr_edb_xmit_ctrl_entry_t entry;
    
    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
    if (phy_port == -1) {
        return BCM_E_PORT;
    }

    SOC_IF_ERROR_RETURN(READ_ASF_PORT_CFGr(unit, port, &rval));
    soc_reg_field_set(unit, ASF_PORT_CFGr, &rval,
                      MC_ASF_ENABLEf, enable ? 1 : 0);
    soc_reg_field_set(unit, ASF_PORT_CFGr, &rval,
                      UC_ASF_ENABLEf, enable ? 1 : 0);
    SOC_IF_ERROR_RETURN(WRITE_ASF_PORT_CFGr(unit, port, rval));

    SOC_IF_ERROR_RETURN(soc_mem_read(unit, EGR_EDB_XMIT_CTRLm,
                                     MEM_BLOCK_ALL, phy_port, &entry));
    if (SOC_PBMP_MEMBER(SOC_INFO(unit).oversub_pbm, port)) {
        soc_mem_field32_set(unit, EGR_EDB_XMIT_CTRLm, &entry,
                            WAIT_FOR_2ND_MOPf,  enable ? 0 : 1);
    } else {
        soc_mem_field32_set(unit, EGR_EDB_XMIT_CTRLm, &entry,
                            WAIT_FOR_MOPf,  enable ? 0: 1);
    }
    SOC_IF_ERROR_RETURN(soc_mem_write(unit, EGR_EDB_XMIT_CTRLm,
                                      MEM_BLOCK_ALL, phy_port, &entry));
    return BCM_E_NONE;
}

STATIC int
_bcm_td2_port_asf_enable_get(int unit, bcm_port_t port, int *enable)
{
    uint32 rval;                
    SOC_IF_ERROR_RETURN(READ_ASF_PORT_CFGr(unit, port, &rval));
    *enable = (soc_reg_field_get(unit, ASF_PORT_CFGr, rval, UC_ASF_ENABLEf))
            & (soc_reg_field_get(unit, ASF_PORT_CFGr, rval, MC_ASF_ENABLEf));
    
    return BCM_E_NONE;
}
#endif

STATIC int
_bcm_fb_mod_lb_set(int unit, bcm_port_t port, int arg)
{
    if (soc_feature(unit, soc_feature_module_loopback)) {
        return _bcm_esw_port_config_set(unit, port, _bcmPortModuleLoopback,
                                      (arg ? 1 : 0));
    }
    return BCM_E_UNAVAIL;
}

STATIC int
_bcm_fb_mod_lb_get(int unit, bcm_port_t port, int *arg)
{
    if (soc_feature(unit, soc_feature_module_loopback)) {
        return _bcm_esw_port_config_get(unit, port, _bcmPortModuleLoopback,
                                        arg);
    }
    return BCM_E_UNAVAIL;
}

#define BCM_RAPTOR_BMC_IFG_0    0
#define BCM_RAPTOR_BMC_IFG_16   1
#define BCM_RAPTOR_BMC_IFG_20   2
#define BCM_RAPTOR_BMC_IFG_24   3

#define BCM_FB2_BMC_IFG_MAX     0x3f

#define BCM_SC_BMC_IFG_MAX      0x1f

#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) \
    || defined(BCM_SCORPION_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
STATIC int
_bcm_xgs3_ing_rate_limit_ifg_set(int unit, bcm_port_t port, int arg)
{
#if defined(BCM_RAPTOR_SUPPORT)
    if (SOC_IS_RAPTOR(unit)) {
        int ifg_sel = 0;
        if (arg >= 20) {
            ifg_sel = BCM_RAPTOR_BMC_IFG_24;
        } else if (arg >= 16) {
            ifg_sel = BCM_RAPTOR_BMC_IFG_20;
        } else if (arg > 0) {
            ifg_sel = BCM_RAPTOR_BMC_IFG_16;
        } else if (arg == 0) {
            ifg_sel = BCM_RAPTOR_BMC_IFG_0;
        } else { /* arg < 0 */
            return BCM_E_PARAM;
        }
        return soc_reg_field32_modify(unit, BKPMETERINGCONFIGr, port,
                                      IFG_ACCT_SELf, ifg_sel);
    } else
#endif /* BCM_RAPTOR_SUPPORT */
#if defined(BCM_SCORPION_SUPPORT)
    if (SOC_IS_SCORPION(unit)) {
        if (arg > BCM_SC_BMC_IFG_MAX) {
            arg = BCM_SC_BMC_IFG_MAX;
        } else if (arg < 0) {
            return BCM_E_PARAM;
        }
        return soc_reg_field32_modify(unit, BKPMETERINGCONFIG1r, port,
                                      PACKET_IFG_BYTESf, arg);
    } else
#endif /* BCM_SCORPION_SUPPORT */
#if defined (BCM_RAVEN_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
    if (SOC_REG_FIELD_VALID(unit, BKPMETERINGCONFIG_EXTr, IFG_ACCT_SELf)) {
        if (arg > BCM_FB2_BMC_IFG_MAX) {
            arg = BCM_FB2_BMC_IFG_MAX;
        } else if (arg < 0) {
            return BCM_E_PARAM;
        }
        return soc_reg_field32_modify(unit, BKPMETERINGCONFIG_EXTr, port,
                                      IFG_ACCT_SELf, arg);
    }
#endif /* BCM_RAVEN_SUPPORT || BCM_FIREBOLT2_SUPPORT || BCM_HURRICANE_SUPPORT */
    return BCM_E_UNAVAIL;
}

STATIC int
_bcm_xgs3_ing_rate_limit_ifg_get(int unit, bcm_port_t port, int *arg)
{
    uint32 bmc_reg;

#if defined(BCM_RAPTOR_SUPPORT)
    if (SOC_IS_RAPTOR(unit)) {
        int ifg_sel;
        BCM_IF_ERROR_RETURN(READ_BKPMETERINGCONFIGr(unit, port, &bmc_reg));
        ifg_sel = soc_reg_field_get(unit, BKPMETERINGCONFIGr, bmc_reg,
                                    IFG_ACCT_SELf);
        switch (ifg_sel) {
        case BCM_RAPTOR_BMC_IFG_24:
            *arg = 24;
            break;
        case BCM_RAPTOR_BMC_IFG_20:
            *arg = 20;
            break;
        case BCM_RAPTOR_BMC_IFG_16:
            *arg = 16;
            break;
        case BCM_RAPTOR_BMC_IFG_0:
            *arg = 0;
            break;
        }
        return BCM_E_NONE;
    }  else
#endif /* BCM_RAPTOR_SUPPORT */
#if defined(BCM_SCORPION_SUPPORT)
    if (SOC_IS_SCORPION(unit)) {
        BCM_IF_ERROR_RETURN
            (READ_BKPMETERINGCONFIG1r(unit, port, &bmc_reg));
        *arg = soc_reg_field_get(unit, BKPMETERINGCONFIG1r, bmc_reg,
                                 PACKET_IFG_BYTESf);
        return BCM_E_NONE;
    } else
#endif /* BCM_SCORPION_SUPPORT */
#if defined (BCM_RAVEN_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
    if (SOC_REG_FIELD_VALID(unit, BKPMETERINGCONFIG_EXTr, IFG_ACCT_SELf)) {
        BCM_IF_ERROR_RETURN
            (READ_BKPMETERINGCONFIG_EXTr(unit, port, &bmc_reg));
        *arg = soc_reg_field_get(unit, BKPMETERINGCONFIG_EXTr, bmc_reg,
                                 IFG_ACCT_SELf);
        return BCM_E_NONE;
    }
#endif /* BCM_RAVEN_SUPPORT || BCM_FIREBOLT2_SUPPORT || BCM_HURRICANE_SUPPORT */
    return BCM_E_UNAVAIL;
}
#endif /*  BCM_RAPTOR_SUPPORT || BCM_FIREBOLT2_SUPPORT || BCM_HURRICANE_SUPPORT 
           || BCM_SCORPION_SUPPORT */



#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
STATIC int
_bcm_xgs3_urpf_port_mode_set(int unit, bcm_port_t port, int arg)
{
    if (!soc_feature(unit, soc_feature_urpf)) {
        return (BCM_E_UNAVAIL);
    }

    switch (arg){
      case BCM_SWITCH_URPF_DISABLE:
      case BCM_SWITCH_URPF_LOOSE:
      case BCM_SWITCH_URPF_STRICT:
#ifdef BCM_TRIDENT2_SUPPORT
#if defined(INCLUDE_L3)
        if (soc_feature(unit, soc_feature_virtual_port_routing) &&
            (BCM_GPORT_IS_NIV_PORT(port) ||
             BCM_GPORT_IS_EXTENDER_PORT(port) ||
             BCM_GPORT_IS_VLAN_PORT(port))) {
            return _bcm_td2_vp_urpf_mode_set(unit, port, arg);
        } else 
#endif
#endif /* BCM_TRIDENT2_SUPPORT */          
        {
            return _bcm_esw_port_config_set(unit, port, _bcmPortL3UrpfMode, arg);
        }
      default:
          return (BCM_E_PARAM);
    }
    return (BCM_E_PARAM);
}

STATIC int
_bcm_xgs3_urpf_def_gw_enable(int unit, bcm_port_t port, int arg)
{
    int enable = (arg) ? TRUE : FALSE;

    if (!soc_feature(unit, soc_feature_urpf)) {
        return (BCM_E_UNAVAIL);
    }

    return _bcm_esw_port_config_set(unit, port,
                                    _bcmPortL3UrpfDefaultRoute, enable);
}

STATIC int
_bcm_xgs3_urpf_route_enable(int unit,  int arg)
{
#ifdef INCLUDE_L3
    int orig_val, rv = BCM_E_NONE;
    uint32 reg_val;

    if (!SOC_REG_FIELD_VALID(unit, L3_DEFIP_RPF_CONTROLr, DEFIP_RPF_ENABLEf)) {
        return (BCM_E_UNAVAIL);
    }

    if (!soc_feature(unit, soc_feature_urpf)) {
        return (BCM_E_UNAVAIL);
    }

    BCM_IF_ERROR_RETURN(READ_L3_DEFIP_RPF_CONTROLr(unit, &reg_val));

    /* Handle nothing changed case. */
    orig_val = soc_reg_field_get(unit, L3_DEFIP_RPF_CONTROLr,
                                 reg_val, DEFIP_RPF_ENABLEf);
    if (orig_val == (arg ? 1 : 0)) {
        return (BCM_E_NONE);
    }

    /* State changed -> Delete all the routes. */
    BCM_IF_ERROR_RETURN(bcm_xgs3_defip_del_all(unit));

    /* Destroy Hash/Avl quick lookup structure. */
    BCM_IF_ERROR_RETURN(bcm_xgs3_l3_fbx_defip_deinit(unit));

    /* Set urpf route enable bit. */
    BCM_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, L3_DEFIP_RPF_CONTROLr, REG_PORT_ANY,
                                DEFIP_RPF_ENABLEf, (arg ? 1 : 0)));


    /* Reinit Hash/Avl quick lookup structure. */
    BCM_IF_ERROR_RETURN(bcm_xgs3_l3_fbx_defip_init(unit));

    /* Lock the DEFIP tables so the SOC TCAM scanning logic doesn't
     * trip up on the URPF reconfiguration.
     */
    soc_mem_lock(unit, L3_DEFIPm);
    if (SOC_MEM_IS_ENABLED(unit, L3_DEFIP_PAIR_128m)) {
        soc_mem_lock(unit, L3_DEFIP_PAIR_128m);
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit) &&
       !BCM_TR3_ESM_LPM_TBL_PRESENT(unit, EXT_IPV4_DEFIPm)) {
        rv = _bcm_tr3_l3_defip_urpf_enable(unit, arg);
    }
#endif

#ifdef BCM_TRIDENT2_SUPPORT
    if (BCM_SUCCESS(rv) && SOC_IS_TRIDENT2(unit)) {
        /* BCM_SUCCESS test unneeded, but likely to avoid
         * Coverity complaints */
        rv = _bcm_l3_defip_urpf_enable(unit, arg);
    }
#endif
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->l3_defip_urpf = arg ? 1 : 0;
    SOC_CONTROL_UNLOCK(unit);

    /* Clear h/w memory before use */
    if (BCM_SUCCESS(rv)) {
        rv = soc_mem_clear(unit, L3_DEFIPm, MEM_BLOCK_ALL, 0);
    }
    if (SOC_MEM_IS_ENABLED(unit, L3_DEFIP_PAIR_128m)) {
        if (BCM_SUCCESS(rv)) {
            rv = soc_mem_clear(unit, L3_DEFIP_PAIR_128m,
                               MEM_BLOCK_ALL, 0);
        }
        soc_mem_unlock(unit, L3_DEFIP_PAIR_128m);
    }

    soc_mem_unlock(unit, L3_DEFIPm);
    /* Must release the locks before return. */

    return rv;
#endif /* INCLUDE_L3 */
    return (BCM_E_UNAVAIL);
}
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */

#if defined(BCM_TRIUMPH_SUPPORT)
/*
 * Function:
 *      _bcm_switch_default_cml_set
 * Purpose:
 *      Set Gport default cml
 * Parameters:
 *      unit    -  (IN) Device number.
 *      type     -  (IN) SwitchControl Type
 *      arg  -  (IN) cml value
 * Returns:
 *      Boolean
 */
int
_bcm_switch_default_cml_set(int unit, bcm_switch_control_t type, int arg)
{
   int vp = 0;
   int cml=0;
   int rv = BCM_E_NONE;
    source_vp_entry_t svp;


    if (!(arg & BCM_PORT_LEARN_FWD)) {
       cml |= (1 << 0);
    }
    if (arg & BCM_PORT_LEARN_CPU) {
       cml |= (1 << 1);
    }
    if (arg & BCM_PORT_LEARN_PENDING) {
       cml |= (1 << 2);
    }
    if (arg & BCM_PORT_LEARN_ARL) {
       cml |= (1 << 3);
    }

     BCM_IF_ERROR_RETURN
          (READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp));

    switch (type) {
        case bcmSwitchGportAnyDefaultL2Learn:
            if (cml != 0x8) {
                soc_SOURCE_VPm_field32_set(unit, &svp, CLASS_IDf, 0x1);
            } else {
                soc_SOURCE_VPm_field32_set(unit, &svp, CLASS_IDf, 0x0);
            }
            soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_NEWf, cml);
            break;

        case bcmSwitchGportAnyDefaultL2Move:
            if (cml != 0x8) {
                soc_SOURCE_VPm_field32_set(unit, &svp, DVPf, 0x1);
            } else {
                soc_SOURCE_VPm_field32_set(unit, &svp, DVPf, 0x0);
            }
            soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_MOVEf, cml);
            break;

        default:
            break;
    }

    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
    return rv;
}

/*
 * Function:
 *      _bcm_switch_default_cml_get
 * Purpose:
 *      Get Gport default cml
 * Parameters:
 *      unit    -  (IN) Device number.
 *      type     -  (IN) SwitchControl Type
 *      arg  -  (OUT) cml value
 * Returns:
 *      Boolean
 */
int
_bcm_switch_default_cml_get(int unit, bcm_switch_control_t type, int *arg)
{
   int vp = 0;
   int cml=0x8; /* Default Value of cml */
   int rv = BCM_E_NONE;
    source_vp_entry_t svp;
   int cml_default_enable=0;

     BCM_IF_ERROR_RETURN
          (READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp));

  
    switch (type) {
        case bcmSwitchGportAnyDefaultL2Learn:
          cml_default_enable = soc_SOURCE_VPm_field32_get(unit, &svp,
                                                         CLASS_IDf);
          if (cml_default_enable) {
                cml = soc_SOURCE_VPm_field32_get(unit, &svp,
                                                         CML_FLAGS_NEWf);
            }
            break;

        case bcmSwitchGportAnyDefaultL2Move:
            cml_default_enable = soc_SOURCE_VPm_field32_get(unit, &svp,
                                                         DVPf);
            if (cml_default_enable) {
                cml = soc_SOURCE_VPm_field32_get(unit, &svp,
                                                         CML_FLAGS_MOVEf);
            }
            break;

        default:
            break;
    }

    *arg = 0;
    if (!(cml & (1 << 0))) {
       *arg |= BCM_PORT_LEARN_FWD;
    }
    if (cml & (1 << 1)) {
       *arg |= BCM_PORT_LEARN_CPU;
    }
    if (cml & (1 << 2)) {
       *arg |= BCM_PORT_LEARN_PENDING;
    }
    if (cml & (1 << 3)) {
       *arg |= BCM_PORT_LEARN_ARL;
    }

    return rv;
}

/* Assumption: Use this SC for Dynamic uRPF update, only if EXT-DEFIP entry used */
STATIC int
_bcm_xgs3_urpf_route_enable_external(int unit, int arg)
{
#ifdef INCLUDE_L3
    int orig_val;
    uint32 reg_val;

    if (!SOC_REG_FIELD_VALID(unit, L3_DEFIP_RPF_CONTROLr, DEFIP_RPF_ENABLEf)) {
         return (BCM_E_UNAVAIL);
    }

    if (!soc_feature(unit, soc_feature_urpf)) {
         return (BCM_E_UNAVAIL);
    }

    /* Check if any INT-DEFIP entry configured */
    BCM_IF_ERROR_RETURN(bcm_xgs3_defip_verify_internal_mem_usage(unit));

    BCM_IF_ERROR_RETURN(READ_L3_DEFIP_RPF_CONTROLr(unit, &reg_val));

    /* Handle nothing changed case. */
    orig_val = soc_reg_field_get(unit, L3_DEFIP_RPF_CONTROLr,
                                         reg_val, DEFIP_RPF_ENABLEf);
    if (orig_val == (arg ? 1 : 0)) {
         return (BCM_E_NONE);
    }

    /* State changed -> Set urpf route enable bit. */
   BCM_IF_ERROR_RETURN
           (soc_reg_field32_modify(unit, L3_DEFIP_RPF_CONTROLr, REG_PORT_ANY,
                             DEFIP_RPF_ENABLEf, (arg ? 1 : 0)));
    return (BCM_E_NONE);
#endif /* INCLUDE_L3 */
    return (BCM_E_UNAVAIL);
}
#endif

#if defined(BCM_SCORPION_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT)
STATIC int
_bcm_tr2_sc_pfc_sctype_to_class(bcm_switch_control_t type)
{
    int class = 0;

    switch (type) {
    case bcmSwitchPFCClass7Queue:
        class = 7;
        break;
    case bcmSwitchPFCClass6Queue:
        class = 6;
        break;
    case bcmSwitchPFCClass5Queue:
        class = 5;
        break;
    case bcmSwitchPFCClass4Queue:
        class = 4;
        break;
    case bcmSwitchPFCClass3Queue:
        class = 3;
        break;
    case bcmSwitchPFCClass2Queue:
        class = 2;
        break;
    case bcmSwitchPFCClass1Queue:
        class = 1;
        break;
    case bcmSwitchPFCClass0Queue:
        class = 0;
        break;
    default:
        class = -1;
        break;
    }
    return class;
}

STATIC int
_bcm_tr2_sc_pfc_priority_to_cos_set(int unit, bcm_switch_control_t type, 
                                    int arg)
{
    uint32 rval;
    int class = 0;

    if (soc_feature(unit, soc_feature_priority_flow_control)) {
        class = _bcm_tr2_sc_pfc_sctype_to_class(type);
        if (class < 0) {
            return BCM_E_INTERNAL;
        }
        if ((arg < 0) || (arg > 7)) {
            return BCM_E_PARAM;
        }
        if (SOC_REG_IS_VALID(unit, PRIO2COSr)) {
            BCM_IF_ERROR_RETURN(READ_PRIO2COSr(unit, class, &rval));
            soc_reg_field_set(unit, PRIO2COSr, &rval, COS0_7_BMPf,
                              (1 << arg));
            BCM_IF_ERROR_RETURN(WRITE_PRIO2COSr(unit, class, rval));
        } else {
            if (SOC_REG_IS_VALID(unit, PRIO2COS_PROFILEr)) {
                BCM_IF_ERROR_RETURN(READ_PRIO2COS_PROFILEr(unit, class, &rval));
                soc_reg_field_set(unit, PRIO2COS_PROFILEr, &rval, COS_BMPf,
                                  (1 << arg));
                BCM_IF_ERROR_RETURN(WRITE_PRIO2COS_PROFILEr(unit, class, rval));
            } else  {
                BCM_IF_ERROR_RETURN(READ_PRIO2COS_LLFCr(unit, class, &rval));
                soc_reg_field_set(unit, PRIO2COS_LLFCr, &rval, COS0_23_BMPf,
                                  (1 << arg));
                BCM_IF_ERROR_RETURN(WRITE_PRIO2COS_LLFCr(unit, class, rval));
            }
        }
        return BCM_E_NONE;
    }
    return BCM_E_UNAVAIL;
}

STATIC int
_bcm_tr2_sc_pfc_priority_to_cos_get(int unit, bcm_switch_control_t type,
                                int *arg)
{
    uint32 rval;
    int cosbmp, cos, class = 0;

    if (soc_feature(unit, soc_feature_priority_flow_control)) {
        if (arg == NULL) {
            return BCM_E_PARAM;
        }
        class = _bcm_tr2_sc_pfc_sctype_to_class(type);
        if (class < 0) {
            return BCM_E_INTERNAL;
        }
        if (SOC_REG_IS_VALID(unit, PRIO2COSr)) {
            BCM_IF_ERROR_RETURN(READ_PRIO2COSr(unit, class, &rval));
            cosbmp = soc_reg_field_get(unit, PRIO2COSr, rval, COS0_7_BMPf);
        } else {
            if (SOC_REG_IS_VALID(unit, PRIO2COS_PROFILEr)) {
                BCM_IF_ERROR_RETURN(READ_PRIO2COS_PROFILEr(unit, class, &rval));
                cosbmp = soc_reg_field_get(unit, PRIO2COS_PROFILEr, rval, 
                                           COS_BMPf);
            } else {
                BCM_IF_ERROR_RETURN(READ_PRIO2COS_LLFCr(unit, class, &rval));
                cosbmp = soc_reg_field_get(unit, PRIO2COS_LLFCr, rval, 
                                           COS0_23_BMPf);
            }
        }
        for (cos = 0; cos < 8; cos++) {
            if (cosbmp & (1 << cos)) {
                *arg = cos;
                return BCM_E_NONE;
            }
        }
        return BCM_E_INTERNAL;
    }
    return BCM_E_UNAVAIL;
}
#endif /* BCM_SCORPION_SUPPORT || BCM_TRIUMPH2_SUPPORT */

bcm_switch_binding_t xgs3_bindings[] =
{
#if defined (BCM_RAVEN_SUPPORT)
    { bcmSwitchSTPBlockedFieldBypass, RV | HK,
        ING_CONFIGr, BLOCKED_PORTS_FP_DISABLEf,
        NULL, 0},
#endif /* BCM_RAVEN_SUPPORT */

#if defined (BCM_SCORPION_SUPPORT)
    { bcmSwitchSharedVlanMismatchToCpu, SCQ,
        CPU_CONTROL_1r, PVLAN_VID_MISMATCH_TOCPUf,
        NULL, 0},
    { bcmSwitchForceForwardFabricTrunk, SCQ,
        ING_MISC_CONFIGr, LOCAL_SW_DISABLE_HGTRUNK_RES_ENf,
        NULL, 0},
#endif /* BCM_SCORPION_SUPPORT */

#if defined(BCM_RAPTOR_SUPPORT)
    { bcmSwitchLearnLimitPriority, RV | HK,
        CPU_CONTROL_3r, CPU_MAC_LIMIT_PRIORITYf,
        NULL, soc_feature_mac_learn_limit },
    { bcmSwitchIpmcTunnelToCpu, RP,
        CPU_CONTROL_1r, IPMC_TUNNEL_TO_CPUf,
        NULL,0},
    { bcmSwitchCpuToCpuEnable, RP | RV | HK,
        ING_CONFIGr, DISABLE_COPY_TO_CPU_FOR_CPU_PORTf,
        _bool_invert, 0 },
#endif /* BCM_RAPTOR_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT)
    {bcmSwitchWlanClientAuthorizeAll, TR3 | HX4,
        AXP_WRX_MASTER_CTRLr, WCD_DISABLEf,
        NULL, 0},
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
    { bcmSwitchWlanClientUnauthToCpu, TR2 | AP | VL2,
        CPU_CONTROL_Mr, WLAN_DOT1X_DROPf,
        NULL, 0},
    { bcmSwitchWlanClientRoamedOutErrorToCpu, TR2 | AP | VL2,
        CPU_CONTROL_Mr, WLAN_ROAM_ERRORf,
        NULL, 0},
    { bcmSwitchWlanClientSrcMacMissToCpu, TR2 | AP | VL2,
        CPU_CONTROL_Mr, WCD_SA_MISSf,
        NULL, 0},
    { bcmSwitchWlanClientDstMacMissToCpu, TR2 | AP | VL2,
        CPU_CONTROL_Mr, WCD_DA_MISSf,
        NULL, 0},
    { bcmSwitchOamHeaderErrorToCpu, TR2 | AP | VL2 | EN | TD | TR3 | HX4 | KT | KT2
        | TD2 , CPU_CONTROL_Mr, OAM_HEADER_ERROR_TOCPUf,
        NULL, 0},
    { bcmSwitchOamUnknownVersionToCpu, TR2 | AP | VL2 | EN | TD | TR3 | HX4 | KT | KT2
        | TD2 , CPU_CONTROL_Mr, OAM_UNKNOWN_OPCODE_VERSION_TOCPUf,
        NULL, 0},
    { bcmSwitchL3SrcBindFailToCpu, TR2 | AP | VL2 | TD | TR3 | HX4 | KT | KT2 | TD2,
        CPU_CONTROL_Mr, MAC_BIND_FAILf,
        NULL, 0},
    { bcmSwitchTunnelIp4IdShared, TR2 | AP | VL2 | TD | TR3 | HX4 | KT | KT2 | TD2,
        EGR_TUNNEL_ID_MASKr, SHARED_FRAG_ID_ENABLEf,
        NULL, 0},
    { bcmSwitchIpfixRateViolationDataInsert, TR2 | AP | VL2,
        ING_IPFIX_FLOW_RATE_CONTROLr, SUSPECT_FLOW_INSERT_DISABLEf,
        _bool_invert, soc_feature_ipfix_rate },
    { bcmSwitchIpfixRateViolationPersistent, TR2 | AP | VL2,
        ING_IPFIX_FLOW_RATE_CONTROLr, SUSPECT_FLOW_CONVERT_DISABLEf,
        NULL, 0},
    { bcmSwitchCustomerQueuing, TR2 | AP | VL2 | EN | KT | KT2,
        ING_MISC_CONFIGr, PHB2_COS_MODEf,
        NULL, soc_feature_vlan_queue_map},
    { bcmSwitchOamUnknownVersionDrop, TR2 | AP | VL2 | EN | TD | TR3 | HX4 | KT 
        | TD2 , OAM_DROP_CONTROLr, OAM_UNKNOWN_OPCODE_VERSION_DROPf,
        NULL, 0},
    { bcmSwitchDirectedMirroring, TR2 | AP | VL2,
        EGR_PORT_64r, EM_SRCMOD_CHANGEf,
        NULL, soc_feature_src_modid_blk },
    { bcmSwitchDirectedMirroring, TR2 | AP | VL2,
        IEGR_PORT_64r, EM_SRCMOD_CHANGEf,
        NULL, soc_feature_src_modid_blk },
#endif

#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    { bcmSwitchOamUnexpectedPktToCpu, EN | TD | TR3 | HX4 | KT | KT2 | TD2,
        CPU_CONTROL_Mr, OAM_UNEXPECTED_PKT_TOCPUf,
        NULL, 0},
#endif

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || \
    defined(BCM_TRIDENT2_SUPPORT)
    { bcmSwitchOamCcmToCpu, KT | KT2 | TR3 | HX4 | TD2,
        CCM_COPYTO_CPU_CONTROLr, ERROR_CCM_COPY_TOCPUf,
        NULL, 0},
    { bcmSwitchOamXconCcmToCpu, KT | KT2 | TR3 | HX4 | TD2,
        CCM_COPYTO_CPU_CONTROLr, XCON_CCM_COPY_TOCPUf,
        NULL, 0},
    { bcmSwitchOamXconOtherToCpu, KT | KT2 | TR3 | HX4 | TD2,
        CCM_COPYTO_CPU_CONTROLr, XCON_OTHER_COPY_TOCPUf,
        NULL, 0},
#endif /* !BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIDENT_SUPPORT)
    { bcmSwitchL3InterfaceUrpfEnable, TD | TR3 | HX4 | KT | KT2 | TD2,
       ING_CONFIG_64r, L3IIF_URPF_SELECTf,
        NULL, soc_feature_urpf },
#endif

#if defined(BCM_ENDURO_SUPPORT)
    { bcmSwitchMiMDefaultSVP, EN,
        ING_Q_BEGINr, MIM_EN_DEF_NETWORK_SVPf,
        NULL, 0},
    { bcmSwitchEgressDroppedReportZeroLength, EN,
        EGR_Q_BEGINr, DWRR_OR_SHAPINGf,
        NULL, 0},
#endif

#if defined(BCM_TRX_SUPPORT)
    { bcmSwitchClassBasedMoveFailPktToCpu, TRX,
        CPU_CONTROL_1r, CLASS_BASED_SM_PREVENTED_TOCPUf,
        NULL, soc_feature_class_based_learning },
    { bcmSwitchL3UrpfFailToCpu,            TRX,
        CPU_CONTROL_1r, URPF_MISS_TOCPUf,
        NULL, soc_feature_urpf },
    { bcmSwitchClassBasedMoveFailPktDrop,  TRX,
        ING_MISC_CONFIG2r, CLASS_BASED_SM_PREVENTED_DROPf,
        NULL, soc_feature_class_based_learning },
    { bcmSwitchSTPBlockedFieldBypass,      TRX,
        ING_MISC_CONFIG2r, BLOCKED_PORTS_FP_DISABLEf,
        NULL, 0},
    { bcmSwitchRateLimitLinear,            TRX,
        MISCCONFIGr, ITU_MODE_SELf,
        _bool_invert, 0},
    { bcmSwitchRemoteLearnTrust,           TRX,
        ING_CONFIG_64r, IGNORE_HG_HDR_DONOT_LEARNf,
        _bool_invert, 0 },
    { bcmSwitchMldDirectAttachedOnly,      TRX,
        ING_CONFIG_64r, MLD_CHECKS_ENABLEf,
        NULL, soc_feature_igmp_mld_support },
    { bcmSwitchSharedVlanL2McastEnable,    TRX,
        ING_CONFIG_64r, LOOKUP_L2MC_WITH_FID_IDf,
        NULL, 0},
    { bcmSwitchMldUcastEnable,             TRX,
        ING_CONFIG_64r, MLD_PKTS_UNICAST_IGNOREf,
        _bool_invert, soc_feature_igmp_mld_support },
    { bcmSwitchSharedVlanEnable, TRX,
        ING_CONFIG_64r, SVL_ENABLEf,
        NULL, 0},
    { bcmSwitchIgmpReservedMcastEnable,    TRX,
        ING_CONFIG_64r, IPV4_RESERVED_MC_ADDR_IGMP_ENABLEf,
        NULL, soc_feature_igmp_mld_support },
    { bcmSwitchMldReservedMcastEnable,     TRX,
        ING_CONFIG_64r, IPV6_RESERVED_MC_ADDR_MLD_ENABLEf,
        NULL, soc_feature_igmp_mld_support },
    { bcmSwitchIgmpUcastEnable,            TRX,
        ING_CONFIG_64r, IGMP_PKTS_UNICAST_IGNOREf,
        _bool_invert, soc_feature_igmp_mld_support },
    { bcmSwitchPortEgressBlockL3,          TRX,
        ING_CONFIG_64r, APPLY_EGR_MASK_ON_L3f,
        NULL, 0},
    { bcmSwitchPortEgressBlockL2,          TRX,
        ING_CONFIG_64r, APPLY_EGR_MASK_ON_L2f,
        NULL, 0},
    { bcmSwitchHgHdrExtLengthEnable,       TRX,
        ING_CONFIG_64r, IGNORE_HG_HDR_HDR_EXT_LENf,
        _bool_invert, 0 },
    { bcmSwitchIp4McastL2DestCheck,        TRX,
        ING_CONFIG_64r, IPV4_MC_MACDA_CHECK_ENABLEf,
        NULL, 0},
    { bcmSwitchIp6McastL2DestCheck,        TRX,
        ING_CONFIG_64r, IPV6_MC_MACDA_CHECK_ENABLEf,
        NULL, 0},
    { bcmSwitchL3TunnelUrpfMode,           TRX,
        ING_CONFIG_64r, TUNNEL_URPF_MODEf,
        NULL, soc_feature_urpf },
    { bcmSwitchL3TunnelUrpfDefaultRoute,   TRX,
        ING_CONFIG_64r, TUNNEL_URPF_DEFAULTROUTECHECKf,
        _bool_invert, soc_feature_urpf },
    { bcmSwitchMirrorStackMode, TRX,
        ING_CONFIG_64r, STACK_MODEf,
        NULL, soc_feature_src_modid_blk },
    { bcmSwitchDirectedMirroring, TRX,
        ING_CONFIG_64r, DRACO1_5_MIRRORf,
        _bool_invert, soc_feature_xgs1_mirror },
    { bcmSwitchMirrorSrcModCheck, TRX,
        ING_CONFIG_64r, FB_A0_COMPATIBLEf,
        NULL, soc_feature_src_modid_blk },
    { bcmSwitchProxySrcKnockout, TRX,
        IHG_LOOKUPr, REMOVE_MH_SRC_PORTf,
        NULL, soc_feature_proxy_port_property },
#endif /* BCM_TRX_SUPPORT */

#if defined(BCM_TRIUMPH_SUPPORT)
    { bcmSwitchDosAttackToCpu, TRVL|HU2|KT2,
        CPU_CONTROL_0r, DOSATTACK_TOCPUf,
        NULL, 0 },
    { bcmSwitchParityErrorToCpu, TRVL|KT2,
        CPU_CONTROL_0r, PARITY_ERR_TOCPUf,
        NULL, soc_feature_parity_err_tocpu },
    { bcmSwitchUnknownVlanToCpu, TRVL|KT2,
        CPU_CONTROL_0r, UVLAN_TOCPUf,
        NULL, 0 },
    { bcmSwitchSourceMacZeroDrop, TRVL|KT2,
        CPU_CONTROL_0r, MACSA_ALL_ZERO_DROPf,
        NULL, 0},
    { bcmSwitchMplsSequenceErrToCpu, TRVL|KT2,
        ING_MISC_CONFIGr, MPLS_SEQ_NUM_FAIL_TOCPUf,
        NULL, 0},
    { bcmSwitchMplsLabelMissToCpu, TRVL|KT2,
        CPU_CONTROL_Mr, MPLS_LABEL_MISSf,
        NULL, 0},
    { bcmSwitchMplsTtlErrToCpu, TRVL|KT2,
        CPU_CONTROL_Mr, MPLS_TTL_CHECK_FAILf,
        NULL, 0},
    { bcmSwitchMplsInvalidL3PayloadToCpu, TRVL|KT2,
        CPU_CONTROL_Mr, MPLS_INVALID_PAYLOADf,
        NULL, 0},
    { bcmSwitchMplsInvalidActionToCpu, TRVL|KT2,
        CPU_CONTROL_Mr, MPLS_INVALID_ACTIONf,
        NULL, 0},
    { bcmSwitchSharedVlanMismatchToCpu, TRVL|KT2,
        CPU_CONTROL_0r, PVLAN_VID_MISMATCH_TOCPUf ,
        NULL, 0},
    { bcmSwitchForceForwardFabricTrunk, TRVL|KT2,
        ING_MISC_CONFIGr, LOCAL_SW_DISABLE_HGTRUNK_RES_ENf,
        NULL, 0},
    { bcmSwitchFieldMultipathHashSeed, TRVL|KT2,
        FP_ECMP_HASH_CONTROLr, ECMP_HASH_SALTf,
        NULL, 0},
    { bcmSwitchMplsPortIndependentLowerRange1, TRVL|KT2,
        GLOBAL_MPLS_RANGE_1_LOWERr, LABELf,
        NULL, 0},
    { bcmSwitchMplsPortIndependentUpperRange1, TRVL|KT2,
        GLOBAL_MPLS_RANGE_1_UPPERr, LABELf,
        NULL, 0},
    { bcmSwitchMplsPortIndependentLowerRange2, TRVL|KT2,
       GLOBAL_MPLS_RANGE_2_LOWERr, LABELf,
       NULL, 0},
    { bcmSwitchMplsPortIndependentUpperRange2, TRVL|KT2,
        GLOBAL_MPLS_RANGE_2_UPPERr, LABELf,
        NULL, 0},       
    { bcmSwitchL3UrpfRouteEnableExternal, TRVL | TD2,
       L3_DEFIP_RPF_CONTROLr, DEFIP_RPF_ENABLEf,
       NULL, soc_feature_urpf},
    { bcmSwitchMplsPWControlWordToCpu, TRVL|KT2,
       ING_MISC_CONFIGr, PWACH_TOCPUf,
       NULL, soc_feature_mpls},
    { bcmSwitchMplsPWControlTypeToCpu, TRVL|KT2,
       ING_MISC_CONFIGr, OTHER_CW_TYPE_TOCPUf,
       NULL, soc_feature_mpls},
    { bcmSwitchMplsPWCountPktsAll, TR | VL | EN,
       ING_MISC_CONFIGr, PW_COUNT_ALLf,
       NULL, soc_feature_mpls},
#endif /* BCM_TRIUMPH_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT)
    { bcmSwitchL3UrpfFailToCpu, FB2,
        CPU_CONTROL_1r, URPF_MISS_TOCPUf,
        NULL, soc_feature_urpf },
    { bcmSwitchCpuLookupFpCopyPrio, FB2,
        CPU_CONTROL_2r, CPU_VFPCOPY_PRIORITYf,
        NULL, 0 },
    { bcmSwitchL3TunnelUrpfMode, FB2,
        ING_CONFIGr, TUNNEL_URPF_MODEf,
        NULL, soc_feature_urpf },
    { bcmSwitchL3TunnelUrpfDefaultRoute, FB2,
        ING_CONFIGr, TUNNEL_URPF_DEFAULTROUTECHECKf,
        _bool_invert, soc_feature_urpf },
    { bcmSwitchHgHdrExtLengthEnable, FB2,
        ING_CONFIGr, IGNORE_HG_HDR_HDR_EXT_LENf,
        _bool_invert, 0 },
    { bcmSwitchUniformFabricTrunkDistribution, FB2,
        ING_MISC_CONFIGr, UNIFORM_HG_TRUNK_DIST_ENABLEf,
        NULL, soc_feature_trunk_group_size },
    { bcmSwitchRateLimitLinear, FB2,
        MISCCONFIGr, ITU_MODE_SELf,
        _bool_invert, 0},
#endif /* BCM_FIREBOLT2_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
    { bcmSwitchCpuToCpuEnable, FB2 | TRX,
        ING_MISC_CONFIGr, DO_NOT_COPY_FROM_CPU_TO_CPUf,
        _bool_invert, 0 },
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */

#if defined(BCM_FIREBOLT_SUPPORT) 
    { bcmSwitchCpuProtocolPrio, FB | FH,
        CPU_CONTROL_2r, CPU_PROTOCOL_PRIORITYf,
        NULL, 0 },
#endif /* BCM_FIREBOLT_SUPPORT */

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_HELIX_SUPPORT)
    { bcmSwitchAlternateStoreForward, FB | FH | HXFX,
        ASFCONFIGr, ASF_ENf,
        NULL, soc_feature_asf },
   { bcmSwitchRemoteLearnTrust, FB | FH,
        ING_CONFIGr, DO_NOT_LEARN_ENABLEf,
        NULL, soc_feature_remote_learn_trust },
#endif /* BCM_FIREBOLT_SUPPORT || BCM_HELIX_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT)
    { bcmSwitchIgmpUcastEnable, FB2 | RP | RV | HK,
        ING_CONFIGr, IGMP_PKTS_UNICAST_IGNOREf,
        _bool_invert, soc_feature_igmp_mld_support },
    { bcmSwitchMldUcastEnable, FB2 | RP | RV | HK,
        ING_CONFIGr, MLD_PKTS_UNICAST_IGNOREf,
        _bool_invert, soc_feature_igmp_mld_support },
    { bcmSwitchIgmpReservedMcastEnable, FB2 | RP | RV | HK,
        ING_CONFIGr, IPV4_RESERVED_MC_ADDR_IGMP_ENABLEf,
        NULL, soc_feature_igmp_mld_support },
    { bcmSwitchMldReservedMcastEnable, FB2 | RP | RV | HK,
        ING_CONFIGr, IPV6_RESERVED_MC_ADDR_MLD_ENABLEf,
        NULL, soc_feature_igmp_mld_support },
    { bcmSwitchMldDirectAttachedOnly, FB2 | RP | RV | HK,
        ING_CONFIGr, MLD_CHECKS_ENABLEf,
        NULL, soc_feature_igmp_mld_support },
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAPTOR_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT)
    { bcmSwitchIp4McastL2DestCheck, FB2 | RV | HK,
        ING_CONFIGr, IPV4_MC_MACDA_CHECK_ENABLEf,
        NULL, 0},
    { bcmSwitchIp6McastL2DestCheck, FB2 | RV | HK,
        ING_CONFIGr, IPV6_MC_MACDA_CHECK_ENABLEf,
        NULL, 0},
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT) 
    { bcmSwitchUniformUcastTrunkDistribution, FB2,
        ING_MISC_CONFIGr, UNIFORM_TRUNK_DIST_ENABLEf,
        NULL, soc_feature_trunk_group_size },
#endif /* BCM_FIREBOLT2_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
    { bcmSwitchL3UrpfRouteEnable, FB2 | TRX,
        L3_DEFIP_RPF_CONTROLr, DEFIP_RPF_ENABLEf,
        NULL, soc_feature_urpf},        
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */

#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_TRX_SUPPORT)
    { bcmSwitchStationMoveOverLearnLimitToCpu, RV | TRX | HK,
        CPU_CONTROL_1r, MACLMT_STNMV_TOCPUf,
        NULL, soc_feature_mac_learn_limit},
#endif /* BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */

#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_RAPTOR_SUPPORT)
    { bcmSwitchControlOpcodePriority, HBGW | RP | RV | HK,
        CPU_CONTROL_2r, CPU_MH_CONTROL_PRIORITYf,
        NULL, 0 },
#endif /* BCM_BRADLEY_SUPPORT || BCM_RAPTOR_SUPPORT */

#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_SCORPION_SUPPORT) || \
defined(BCM_TRIDENT_SUPPORT)
    { bcmSwitchAlternateStoreForward, HBGW | SCQ | TD | TT,
        OP_THR_CONFIGr, ASF_ENABLEf,
        NULL, soc_feature_asf },
#endif /* BCM_BRADLEY_SUPPORT  || BCM_SCORPION_SUPPORT || BCM_TRIDENT_SUPPORT */

#if defined(BCM_BRADLEY_SUPPORT)
    { bcmSwitchSharedVlanEnable, HBGW,
        ING_CONFIGr, SVL_ENABLEf,
        NULL, 0},
#endif /* BCM_BRADLEY_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
    defined(BCM_TRX_SUPPORT)
    { bcmSwitchBpduInvalidVlanDrop, FB2 | RP | RV | TRX | HK,
        EGR_CONFIG_1r, BPDU_INVALID_VLAN_DROPf,
        NULL, soc_feature_igmp_mld_support },
    { bcmSwitchMirrorInvalidVlanDrop, FB2 | RP | RV | TRX | HK,
        EGR_CONFIG_1r, MIRROR_INVALID_VLAN_DROPf,
        NULL, soc_feature_igmp_mld_support },
    { bcmSwitchMcastFloodBlocking, FB2 | RP | RV | TRX | HK,
        IGMP_MLD_PKT_CONTROLr, PFM_RULE_APPLYf,
        NULL, soc_feature_igmp_mld_support },
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_TRX_SUPPORT)
    { bcmSwitchL3SlowpathToCpu, FB2 | RV | TRX | HK,
        CPU_CONTROL_1r, IPMC_TTL1_ERR_TOCPUf,
        NULL, 0},
    { bcmSwitchL3SlowpathToCpu, FB2 | RV | TRX | HK,
        CPU_CONTROL_1r, L3UC_TTL1_ERR_TOCPUf,
        NULL, 0 },
    { bcmSwitchIpmcTtl1ToCpu, FB2 | RV | TRX | HK,
        CPU_CONTROL_1r, IPMC_TTL1_ERR_TOCPUf,
        NULL, 0 },
    { bcmSwitchL3UcastTtl1ToCpu, FB2 | RV | TRX | HK,
        CPU_CONTROL_1r, L3UC_TTL1_ERR_TOCPUf,
        NULL, 0 },
    { bcmSwitchDhcpLearn, FB2 | RV | TRX | HK,
        ING_MISC_CONFIG2r, DO_NOT_LEARN_DHCPf,
        NULL, 0 },
    { bcmSwitchUnknownIpmcAsMcast, FB2 | RV | TRX | HK,
        ING_MISC_CONFIG2r, IPMC_MISS_AS_L2MCf,
        NULL, 0 },
    { bcmSwitchTunnelUnknownIpmcDrop, FB2 | RV | TRX | HK,
        ING_MISC_CONFIG2r, UNKNOWN_TUNNEL_IPMC_DROPf,
        NULL, 0 },
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */

#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_BRADLEY_SUPPORT) || \
    defined(BCM_TRX_SUPPORT)
    { bcmSwitchMcastUnknownErrToCpu, BR | GW | RP | TRX | RV | HK,
        CPU_CONTROL_1r, MC_INDEX_ERROR_TOCPUf,
        NULL,0},
#endif /* BCM_RAPTOR_SUPPORT || BCM_BRADLEY_SUPPORT || BCM_TRX_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT) ||\
    defined(BCM_RAPTOR_SUPPORT)
    { bcmSwitchDosAttackMACSAEqualMACDA, FB2 | RP | TRX | RV | HK,
        DOS_CONTROLr, MACSA_EQUALS_MACDA_DROPf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackFlagZeroSeqZero, FB2 | RP | TRX | RV | HK,
        DOS_CONTROLr, TCP_FLAGS_CTRL0_SEQ0_ENABLEf,
        NULL, 0 },
    { bcmSwitchDosAttackSynFrag, FB2 | RP | TRX | RV | HK,
        DOS_CONTROLr, TCP_FLAGS_SYN_FRAG_ENABLEf,
        NULL, 0},
    { bcmSwitchDosAttackIcmp, FB2 | RP | TRX | RV,
        DOS_CONTROL_2r, ICMP_V4_PING_SIZE_ENABLEf,
        NULL, 0 },
    { bcmSwitchDosAttackIcmp, FB2 | RP | TRX | RV,
        DOS_CONTROL_2r, ICMP_V6_PING_SIZE_ENABLEf,
        NULL, 0 },
    { bcmSwitchDosAttackIcmpV4, FB2 | RP | TRX | RV,
        DOS_CONTROL_2r, ICMP_V4_PING_SIZE_ENABLEf,
        NULL, 0 },
    { bcmSwitchDosAttackIcmpV6, FB2 | RP | TRX | RV,
        DOS_CONTROL_2r, ICMP_V6_PING_SIZE_ENABLEf,
        NULL, 0 },
    { bcmSwitchDosAttackIcmpFragments, FB2 | RP | TRX | RV,
        DOS_CONTROL_2r, ICMP_FRAG_PKTS_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackTcpOffset, FB2 | RP | TRX | RV,
        DOS_CONTROL_2r, TCP_HDR_OFFSET_EQ1_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackUdpPortsEqual, FB2 | RP | TRX | RV,
        DOS_CONTROL_2r, UDP_SPORT_EQ_DPORT_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackTcpPortsEqual, FB2 | RP | TRX | RV,
        DOS_CONTROL_2r, TCP_SPORT_EQ_DPORT_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackTcpFlagsSF, FB2 | RP | TRX | RV,
        DOS_CONTROL_2r, TCP_FLAGS_SYN_FIN_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackTcpFlagsFUP, FB2 | RP | TRX | RV,
        DOS_CONTROL_2r, TCP_FLAGS_FIN_URG_PSH_SEQ0_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackTcpHdrPartial, FB2 | RP | TRX | RV,
        DOS_CONTROL_2r, TCP_HDR_PARTIAL_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },


#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT || BCM_RAPTOR_SUPPORT */

#if defined(BCM_HAWKEYE_SUPPORT)
    { bcmSwitchDosAttackIcmp, HK,
        DOS_CONTROLr, ICMP_V4_PING_SIZE_ENABLEf,
        NULL, 0 },
    { bcmSwitchDosAttackIcmp, HK,
        DOS_CONTROLr, ICMP_V6_PING_SIZE_ENABLEf,
        NULL, 0 },
    { bcmSwitchDosAttackIcmpV4, HK,
        DOS_CONTROLr, ICMP_V4_PING_SIZE_ENABLEf,
        NULL, 0 },
    { bcmSwitchDosAttackIcmpV6, HK,
        DOS_CONTROLr, ICMP_V6_PING_SIZE_ENABLEf,
        NULL, 0 },
    { bcmSwitchDosAttackIcmpFragments, HK,
        DOS_CONTROLr, ICMP_FRAG_PKTS_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackTcpOffset, HK,
        DOS_CONTROLr, TCP_HDR_OFFSET_EQ1_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackUdpPortsEqual, HK,
        DOS_CONTROLr, UDP_SPORT_EQ_DPORT_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackTcpPortsEqual, HK,
        DOS_CONTROLr, TCP_SPORT_EQ_DPORT_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackTcpFlagsSF, HK,
        DOS_CONTROLr, TCP_FLAGS_SYN_FIN_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackTcpFlagsFUP, HK,
        DOS_CONTROLr, TCP_FLAGS_FIN_URG_PSH_SEQ0_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackTcpHdrPartial, HK,
        DOS_CONTROLr, TCP_HDR_PARTIAL_ENABLEf,
        NULL, soc_feature_enhanced_dos_ctrl },
    { bcmSwitchDosAttackIcmpPktOversize, HK,
        DOS_CONTROL_2r, BIG_ICMP_PKT_SIZEf,
        NULL, 0 },

#endif /* BCM_HAWKEYE_SUPPORT */
#if defined (BCM_TRIDENT_SUPPORT) || defined (BCM_KATANA_SUPPORT)
    { bcmSwitchDosAttackIcmpPktOversize, TD | TR3 | HX4 | KT | KT2 | TD2,
        DOS_CONTROL_3r, BIG_ICMP_PKT_SIZEf,
        NULL, 0 },
    { bcmSwitchDosAttackIcmpV6PingSize, TD | TR3 | HX4 | KT | KT2 | TD2,
        DOS_CONTROL_3r, BIG_ICMPV6_PKT_SIZEf,
        NULL, soc_feature_big_icmpv6_ping_check },
    { bcmSwitchDosAttackMinTcpHdrSize, TD | TR3 | HX4 | KT | KT2 | TD2,
        DOS_CONTROL_3r, MIN_TCPHDR_SIZEf,
        NULL, 0 },
    { bcmSwitchIpmcSameVlanPruning, TD,
        CPQLINKMEMDEBUGr, IPMC_IND_MODEf,
        _bool_invert, 0 },
#endif /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_SCORPION_SUPPORT) ||\
    defined(BCM_RAPTOR_SUPPORT)
    { bcmSwitchSourceMacZeroDrop, FB2 | RP | RV | SCQ | HK,
        ING_MISC_CONFIG2r, MACSA_ALL_ZERO_DROPf,
        NULL, 0},
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_SCORPION_SUPPORT || BCM_RAPTOR_SUPPORT */

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) ||\
    defined(BCM_BRADLEY_SUPPORT)
    { bcmSwitchRemoteLearnTrust, BR | GW | RP | FB2 | RV | HK,
        ING_CONFIGr, IGNORE_HG_HDR_DONOT_LEARNf,
        _bool_invert, 0 },
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_BRADLEY_SUPPORT || BCM_RAPTOR_SUPPORT */

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_HELIX_SUPPORT) ||\
    defined(BCM_BRADLEY_SUPPORT)
    { bcmSwitchDirectedMirroring, FB | HBGW | FH,
        ING_CONFIGr, DRACO1_5_MIRRORf,
        _bool_invert, soc_feature_xgs1_mirror },
    { bcmSwitchDirectedMirroring, FB | HBGW | FH | TRX,
        EGR_PORTr, EM_SRCMOD_CHANGEf,
        NULL, soc_feature_src_modid_blk },
    { bcmSwitchDirectedMirroring, FB | HBGW | FH | TRX,
        IEGR_PORTr, EM_SRCMOD_CHANGEf,
        NULL, soc_feature_src_modid_blk },
    { bcmSwitchMirrorStackMode, FB | HBGW | FH | HXFX,
        ING_CONFIGr, STACK_MODEf,
        NULL, soc_feature_src_modid_blk },
    { bcmSwitchMirrorStackMode, FB | HBGW | FH | TRVL,
        MISCCONFIGr, STACK_MODEf,
        NULL, soc_feature_src_modid_blk },
    { bcmSwitchMirrorSrcModCheck, FB | HBGW | FH | HXFX,
        ING_CONFIGr, FB_A0_COMPATIBLEf,
        NULL, soc_feature_src_modid_blk },
    { bcmSwitchSrcModBlockMirrorCopy, FB | HBGW | FH | HXFX,
        MIRROR_CONTROLr, SRC_MODID_BLOCK_MIRROR_COPYf,
        NULL, soc_feature_src_modid_blk },
    { bcmSwitchSrcModBlockMirrorOnly, FB | HBGW | FH | HXFX,
        MIRROR_CONTROLr, SRC_MODID_BLOCK_MIRROR_ONLY_PKTf,
        NULL, soc_feature_src_modid_blk },
#endif /* BCM_FIREBOLT_SUPPORT || BCM_HELIX_SUPPORT || BCM_BRADLEY_SUPPORT */

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_HELIX_SUPPORT) ||\
    defined(BCM_RAVEN_SUPPORT)
    { bcmSwitchDestPortHGTrunk, FB | FH | HXFX | RV | HK | HU,
        ING_MISC_CONFIGr, USE_DEST_PORTf,
        NULL, soc_feature_port_trunk_index },
#endif /* BCM_FIREBOLT_SUPPORT || BCM_HELIX_SUPPORT || BCM_RAVEN_SUPPORT */

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_BRADLEY_SUPPORT) || \
    defined(BCM_HELIX_SUPPORT) || defined(BCM_TRX_SUPPORT)
    { bcmSwitchDosAttackIcmpPktOversize, FB | BR | GW | FH | HXFX | TRX,
        DOS_CONTROLr, BIG_ICMP_PKT_SIZEf,
        NULL, 0 },
    { bcmSwitchDirectedMirroring, FB | HBGW | FH | TRX,
        MISCCONFIGr, DRACO_1_5_MIRRORING_MODE_ENf,
        _bool_invert, soc_feature_xgs1_mirror },
    { bcmSwitchDosAttackIcmpV6PingSize, FB | FH | HXFX | BR | GW | TRX,
        DOS_CONTROL_2r, BIG_ICMPV6_PKT_SIZEf,
        NULL, soc_feature_big_icmpv6_ping_check },
    { bcmSwitchMirrorUnmarked, FB | HBGW | FH | HXFX | TRX,
        EGR_CONFIG_1r, RING_MODEf,
        _bool_invert, 0 },
    { bcmSwitchMirrorStackMode, FB | HBGW | FH | TRX,
        EGR_CONFIG_1r, STACK_MODEf,
        NULL, soc_feature_src_modid_blk },
    { bcmSwitchMeterAdjust, FB | HBGW | FH | HXFX | TRX,
        EGR_SHAPING_CONTROLr, PACKET_IFG_BYTESf,
        NULL, soc_feature_meter_adjust },
    { bcmSwitchMeterAdjust, FB | HBGW | FH | HXFX | TRX,
        EGR_SHAPING_CONTROLr, PACKET_IFG_BYTES_2f,
        NULL, soc_feature_meter_adjust },
    { bcmSwitchMeterAdjust, FB | HBGW | FH | HXFX | TRX,
        FP_METER_CONTROLr, PACKET_IFG_BYTESf,
        NULL, soc_feature_meter_adjust },
    { bcmSwitchMeterAdjust, TRVL,
        EFP_METER_CONTROL_2r, PACKET_IFG_BYTESf,
        NULL, soc_feature_meter_adjust },
    { bcmSwitchMeterAdjust, AP | TR2 | TD | TR3 | HX4 | VL2 | TD2,
        EGR_COUNTER_CONTROLr, PACKET_IFG_BYTESf,
        NULL, soc_feature_meter_adjust },
    { bcmSwitchMeterAdjust, AP | TR2 | TD | TR3 | HX4 | VL2 | TD2,
        EGR_COUNTER_CONTROLr, PACKET_IFG_BYTES_2f,
        NULL, soc_feature_meter_adjust },
    { bcmSwitchMeterAdjust, TRVL,
        MTRI_IFGr, PACKET_IFG_BYTESf,
        NULL, soc_feature_meter_adjust },
    { bcmSwitchEgressBlockUcast, FB | HBGW | FH | HXFX | TRX,
        ING_MISC_CONFIGr, APPLY_EGR_MASK_ON_UC_ONLYf,
        NULL, soc_feature_egress_blk_ucast_override },
    { bcmSwitchSourceModBlockUcast, FB | HBGW | FH | HXFX | TRX,
        ING_MISC_CONFIGr, APPLY_SRCMOD_BLOCK_ON_UC_ONLYf,
        NULL, soc_feature_src_modid_blk_ucast_override },
    { bcmSwitchHgHdrMcastFloodOverride, FB | BR | GW | FH | HXFX | TRX,
        EGR_CONFIG_1r, FORCE_STATIC_MH_PFMf,
        NULL, soc_feature_static_pfm },
    { bcmSwitchShaperAdjust, FB | HBGW | FH | HXFX | TRX,
        EGR_SHAPING_CONTROLr, PACKET_IFG_BYTESf,
        NULL, soc_feature_meter_adjust },
    { bcmSwitchShaperAdjust, FB | HBGW | FH | HXFX | TRX,
        EGR_SHAPING_CONTROLr, PACKET_IFG_BYTES_2f,
        NULL, soc_feature_meter_adjust },
    { bcmSwitchShaperAdjust, TRVL,
        MTRI_IFGr, PACKET_IFG_BYTESf,
        NULL, soc_feature_meter_adjust },
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_BRADLEY_SUPPORT ||
          BCM_HELIX_SUPPORT || BCM_TRX_SUPPORT */

#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_BRADLEY_SUPPORT) || \
    defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
    { bcmSwitchHgHdrErrToCpu, FB2 | HBGW | RP | TRX | RV | HK,
        CPU_CONTROL_1r, HG_HDR_ERROR_TOCPUf,
        NULL, soc_feature_cpu_proto_prio},
    { bcmSwitchClassTagPacketsToCpu, FB2 | HBGW | RP | TRX | RV | HK,
        CPU_CONTROL_1r, HG_HDR_TYPE1_TOCPUf,
        NULL, soc_feature_cpu_proto_prio},
    { bcmSwitchRemoteLearnTrust, BR | GW | RP | RV | FB2 | TRX | HK,
        EGR_CONFIG_1r, IGNORE_HG_HDR_DONOT_LEARNf,
        _bool_invert, 0 },
    { bcmSwitchSourceModBlockControlOpcode, FB2 | HBGW | RP | TRX | RV | HK,
        ING_MISC_CONFIGr, DO_NOT_APPLY_SRCMOD_BLOCK_ON_SCf,
        _bool_invert, soc_feature_src_modid_blk_opcode_override },
#endif /* BCM_RAPTOR_SUPPORT || BCM_BRADLEY_SUPPORT ||
          BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */

#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_BRADLEY_SUPPORT) || \
    defined(BCM_FIREBOLT2_SUPPORT) 
    { bcmSwitchCpuProtoBpduPriority, FB2 | BR | GW | RP | RV | HK,
        CPU_CONTROL_3r, CPU_PROTO_BPDU_PRIORITYf,
        NULL, soc_feature_cpu_proto_prio },
    { bcmSwitchCpuProtoArpPriority, FB2 | BR | GW | RP | RV | HK,
        CPU_CONTROL_3r, CPU_PROTO_ARP_PRIORITYf,
        NULL, soc_feature_cpu_proto_prio },
    { bcmSwitchCpuProtoIgmpPriority, FB2 | BR | GW | RP | RV | HK,
        CPU_CONTROL_3r, CPU_PROTO_IGMP_PRIORITYf,
        NULL, soc_feature_cpu_proto_prio },
    { bcmSwitchCpuProtoDhcpPriority, FB2 | BR | GW | RP | RV | HK,
        CPU_CONTROL_3r, CPU_PROTO_DHCP_PRIORITYf,
        NULL, soc_feature_cpu_proto_prio },
    { bcmSwitchCpuProtoIpmcReservedPriority, FB2 | BR | GW | RP | RV | HK,
        CPU_CONTROL_3r, CPU_PROTO_IPMC_RESERVED_PRIORITYf,
        NULL, soc_feature_cpu_proto_prio },
    { bcmSwitchCpuProtoIpOptionsPriority, FB2 | BR | GW | RP | RV | HK,
        CPU_CONTROL_3r, CPU_PROTO_IP_OPTIONS_PRIORITYf,
        NULL, soc_feature_cpu_proto_prio },
    { bcmSwitchCpuProtoExceptionsPriority, FB2 | BR | GW | RP | RV | HK,
        CPU_CONTROL_3r, CPU_PROTO_EXCEPTIONS_PRIORITYf,
        NULL, soc_feature_cpu_proto_prio },
#endif /* BCM_RAPTOR_SUPPORT || BCM_BRADLEY_SUPPORT ||
          BCM_FIREBOLT2_SUPPORT */


#if defined(BCM_HELIX_SUPPORT) || defined(BCM_FIREBOLT_SUPPORT) || \
    defined(BCM_BRADLEY_SUPPORT)
    { bcmSwitchUnknownIpmcToCpu, FB | BR | GW | FH | HXFX,
    	CPU_CONTROL_1r, UIPMC_TOCPUf,
        NULL, soc_feature_unknown_ipmc_tocpu },
    { bcmSwitchIcmpRedirectToCpu, FB | BR | GW | FH | HXFX,
        CPU_CONTROL_1r, ICMP_REDIRECT_TOCPUf,
        NULL, 0 },
    { bcmSwitchCpuUnknownPrio, FB | BR | GW | FH | HXFX,
	    CPU_CONTROL_2r, CPU_LKUPFAIL_PRIORITYf,
        NULL, 0 },
    { bcmSwitchCpuSamplePrio, FB | BR | GW | FH | HXFX,
    	CPU_CONTROL_2r, CPU_SFLOW_PRIORITYf,
        NULL, 0 },
    { bcmSwitchCpuDefaultPrio, FB | BR | GW | FH | HXFX,
    	CPU_CONTROL_2r, CPU_DEFAULT_PRIORITYf,
        NULL, 0 },
    { bcmSwitchCpuMtuFailPrio, FB | BR | GW | FH | HXFX,
	    CPU_CONTROL_2r, CPU_MTUFAIL_PRIORITYf,
        NULL, 0 },
    { bcmSwitchCpuMirrorPrio, FB | BR | GW | FH | HXFX,
	    CPU_CONTROL_2r, CPU_MIRROR_PRIORITYf,
        NULL, 0 },
    { bcmSwitchCpuIcmpRedirectPrio, FB | BR | GW | FH | HXFX,
	    CPU_CONTROL_2r, CPU_ICMP_REDIRECT_PRIORITYf,
        NULL, 0 },
     { bcmSwitchCpuFpCopyPrio, FB | HBGW | FH | HXFX,
    	CPU_CONTROL_2r, CPU_FPCOPY_PRIORITYf,
        NULL, 0 },
    { bcmSwitchDosAttackTcpFlags, FB | BR | GW | FH,
        DOS_CONTROLr, TCP_FLAGS_CHECK_ENABLEf,
        NULL, soc_feature_basic_dos_ctrl },
    { bcmSwitchDosAttackL4Port,   FB | BR | GW | FH,
        DOS_CONTROLr, L4_PORT_CHECK_ENABLEf,
        NULL, soc_feature_basic_dos_ctrl },
    { bcmSwitchDosAttackTcpFrag,  FB | BR | GW | FH,
        DOS_CONTROLr, TCP_FRAG_CHECK_ENABLEf,
        NULL, soc_feature_basic_dos_ctrl },
    { bcmSwitchDosAttackIcmp,     FB | BR | GW | FH,
        DOS_CONTROLr, ICMP_CHECK_ENABLEf,
        NULL, soc_feature_basic_dos_ctrl },
#endif /*  BCM_HELIX_SUPPORT || BCM_FIREBOLT_SUPPORT ||
           BCM_BRADLEY_SUPPORT */

#if defined(BCM_HELIX_SUPPORT) || defined(BCM_FIREBOLT_SUPPORT) || \
    defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRX_SUPPORT)
    { bcmSwitchUnknownMcastToCpu, FB | BR | GW | FH | HXFX | TRX,
    	CPU_CONTROL_1r, UMC_TOCPUf,
        NULL, 0 },
    { bcmSwitchUnknownUcastToCpu, FB | BR | GW | FH | HXFX | TRX,
    	CPU_CONTROL_1r, UUCAST_TOCPUf,
        NULL, 0 },
    { bcmSwitchL2StaticMoveToCpu, FB | BR | GW | FH | HXFX | TRX,
    	CPU_CONTROL_1r, STATICMOVE_TOCPUf,
        NULL, 0 },
    { bcmSwitchL3HeaderErrToCpu, FB | BR | GW | FH | HXFX | TRX, /* compat */
    	CPU_CONTROL_1r, V4L3ERR_TOCPUf,
        NULL, 0 },
    { bcmSwitchUnknownL3SrcToCpu, FB | BR | GW | FH | HXFX | TRX,
	    CPU_CONTROL_1r, UNRESOLVEDL3SRC_TOCPUf,
        NULL, 0 },
    { bcmSwitchUnknownL3DestToCpu, FB | BR | GW | FH | HXFX | TRX, /* compat */
    	CPU_CONTROL_1r, V4L3DSTMISS_TOCPUf,
        NULL, 0 },
    { bcmSwitchIpmcPortMissToCpu, FB | BR | GW | FH | HXFX | TRX,
    	CPU_CONTROL_1r, IPMCPORTMISS_TOCPUf,
        NULL, 0 },
    { bcmSwitchIpmcErrorToCpu, FB | BR | GW | FH | HXFX | TRX,
    	CPU_CONTROL_1r, IPMCERR_TOCPUf,
        NULL, 0 },
    { bcmSwitchL2NonStaticMoveToCpu, FB | BR | GW | FH | HXFX | TRX,
	    CPU_CONTROL_1r, NONSTATICMOVE_TOCPUf,
        NULL, 0 },
    { bcmSwitchV6L3ErrToCpu, FB | BR | GW | FH | HXFX | TRX,
     	CPU_CONTROL_1r, V6L3ERR_TOCPUf,
        NULL, 0 },
    { bcmSwitchV6L3DstMissToCpu, FB | BR | GW | FH | HXFX | TRX,
	    CPU_CONTROL_1r, V6L3DSTMISS_TOCPUf,
        NULL, 0 },
    { bcmSwitchV4L3ErrToCpu, FB | BR | GW | FH | HXFX | TRX,
    	CPU_CONTROL_1r, V4L3ERR_TOCPUf,
        NULL, 0 },
    { bcmSwitchV4L3DstMissToCpu, FB | BR | GW | FH | HXFX | TRX,
    	CPU_CONTROL_1r, V4L3DSTMISS_TOCPUf,
        NULL, 0 },
    { bcmSwitchTunnelErrToCpu, FB | BR | GW | FH | HXFX | TRX,
    	CPU_CONTROL_1r, TUNNEL_ERR_TOCPUf,
        NULL, 0 },
    { bcmSwitchMartianAddrToCpu, FB | BR | GW | FH | HXFX | TRX,
     	CPU_CONTROL_1r, MARTIAN_ADDR_TOCPUf,
        NULL, 0 },
    { bcmSwitchL3UcTtlErrToCpu, FB | BR | GW | FH | HXFX | TRX,
    	CPU_CONTROL_1r, L3UC_TTL_ERR_TOCPUf,
        NULL, 0 },
    { bcmSwitchL3SlowpathToCpu, FB | BR | GW | FH | HXFX | TRX,
    	CPU_CONTROL_1r, L3_SLOWPATH_TOCPUf,
        NULL, 0 },
    { bcmSwitchIpmcTtlErrToCpu, FB | BR | GW | FH | HXFX | TRX,
    	CPU_CONTROL_1r, IPMC_TTL_ERR_TOCPUf,
        NULL, 0 },
    { bcmSwitchSampleIngressRandomSeed, FB | BR | GW | FH | HXFX | TRX,
	    SFLOW_ING_RAND_SEEDr, SEEDf,
        NULL, 0 },
    { bcmSwitchSampleEgressRandomSeed, FB | BR | GW | FH | HXFX | TRX,
    	SFLOW_EGR_RAND_SEEDr, SEEDf,
        NULL, 0 },
    { bcmSwitchArpReplyToCpu, FB | BR | GW | FH | HXFX | TRX,
	    PROTOCOL_PKT_CONTROLr, ARP_REPLY_TO_CPUf,
        NULL, 0 },
    { bcmSwitchArpReplyDrop, FB | BR | GW | FH | HXFX | TRX,
    	PROTOCOL_PKT_CONTROLr, ARP_REPLY_DROPf,
        NULL, 0 },
    { bcmSwitchArpRequestToCpu, FB | BR | GW | FH | HXFX | TRX,
    	PROTOCOL_PKT_CONTROLr, ARP_REQUEST_TO_CPUf,
        NULL, 0 },
    { bcmSwitchArpRequestDrop, FB | BR | GW | FH | HXFX | TRX,
    	PROTOCOL_PKT_CONTROLr, ARP_REQUEST_DROPf,
        NULL, 0 },
    { bcmSwitchNdPktToCpu, FB | BR | GW | FH | HXFX | TRX,
    	PROTOCOL_PKT_CONTROLr, ND_PKT_TO_CPUf,
        NULL, 0 },
    { bcmSwitchNdPktDrop, FB | BR | GW | FH | HXFX | TRX,
    	PROTOCOL_PKT_CONTROLr, ND_PKT_DROPf,
        NULL, 0 },
    { bcmSwitchDhcpPktToCpu, FB | BR | GW | FH | HXFX | TRX,
        PROTOCOL_PKT_CONTROLr, DHCP_PKT_TO_CPUf,
        NULL, 0 },
    { bcmSwitchDhcpPktDrop, FB | BR | GW | FH | HXFX | TRX,
        PROTOCOL_PKT_CONTROLr, DHCP_PKT_DROPf,
        NULL, 0 },
    { bcmSwitchDosAttackSipEqualDip, FB | BR | GW | FH | HXFX | TRX,
        DOS_CONTROLr, DROP_IF_SIP_EQUALS_DIPf,
        NULL, 0 },
    { bcmSwitchDosAttackMinTcpHdrSize, FB | BR | GW | FH | HXFX | TRX,
        DOS_CONTROLr, MIN_TCPHDR_SIZEf,
        NULL, 0 },
    { bcmSwitchDosAttackV4FirstFrag, FB | BR | GW | FH | HXFX | TRX,
        DOS_CONTROLr, IPV4_FIRST_FRAG_CHECK_ENABLEf,
        NULL, 0 },
    { bcmSwitchNonIpL3ErrToCpu, FB | BR | GW |  FH | HXFX | TRX,
        CPU_CONTROL_1r, NIP_L3ERR_TOCPUf,
        NULL, soc_feature_nip_l3_err_tocpu },
    { bcmSwitchSourceRouteToCpu, FB | BR | GW | FH | HXFX | TRX,
	    CPU_CONTROL_1r, SRCROUTE_TOCPUf,
        NULL, 0 },
    { bcmSwitchParityErrorToCpu, FB | HBGW | FH | HXFX | SCQ,
        CPU_CONTROL_1r, PARITY_ERR_TOCPUf,
        NULL, soc_feature_parity_err_tocpu },
    { bcmSwitchDirectedMirroring, FB | HBGW | FH | TRX,
        EGR_CONFIGr, DRACO1_5_MIRRORf,
        _bool_invert, soc_feature_xgs1_mirror},
#endif /* BCM_HELIX_SUPPORT || BCM_FIREBOLT_SUPPORT ||
          BCM_BRADLEY_SUPPORT || BCM_TRX_SUPPORT */

#if defined(BCM_HELIX_SUPPORT) || defined(BCM_FIREBOLT_SUPPORT) || \
    defined(BCM_BRADLEY_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT)
    { bcmSwitchDosAttackToCpu, FB | BR | GW | FH | HXFX | SCQ,
    	CPU_CONTROL_1r, DOSATTACK_TOCPUf,
        NULL, 0 },
#endif /* BCM_HELIX_SUPPORT || BCM_FIREBOLT_SUPPORT || 
          BCM_BRADLEY_SUPPORT || BCM_SCORPION_SUPPORT*/

#if defined(BCM_BRADLEY_SUPPORT) || \
    defined(BCM_RAPTOR_SUPPORT) || defined(BCM_SCORPION_SUPPORT)
    { bcmSwitchUnknownVlanToCpu, HBGW | RP | RV | SCQ | HK,
        CPU_CONTROL_1r, UVLAN_TOCPUf,
        NULL, 0 },
        
#endif /* BCM_BRADLEY_SUPPORT || 
          BCM_RAPTOR_SUPPORT   || BCM_SCORPION_SUPPORT */

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_HELIX_SUPPORT) || \
    defined(BCM_BRADLEY_SUPPORT) || defined(BCM_RAPTOR_SUPPORT)
    { bcmSwitchPortEgressBlockL3, FB | FH | HXFX | HBGW | RP | RV | HK,
        ING_CONFIGr, APPLY_EGR_MASK_ON_L3f,
        NULL, soc_feature_port_egr_block_ctl },
    { bcmSwitchPortEgressBlockL2, FB | FH | HXFX | HBGW | RP | RV | HK,
        ING_CONFIGr, APPLY_EGR_MASK_ON_L2f,
        NULL, soc_feature_port_egr_block_ctl },
#endif /* BCM_FIREBOLT_SUPPORT || BCM_HELIX_SUPPORT ||
          BCM_BRADLEY_SUPPORT || BCM_RAPTOR_SUPPORT */

#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_HELIX_SUPPORT) || \
    defined(BCM_BRADLEY_SUPPORT) || defined(BCM_RAVEN_SUPPORT) ||  \
    defined(BCM_TRX_SUPPORT)
    { bcmSwitchL3MtuFailToCpu, FB | BR | GW | FH | HXFX | TRX | HK,
        CPU_CONTROL_1r, L3_MTU_FAIL_TOCPUf,
        NULL, soc_feature_l3mtu_fail_tocpu },
    { bcmSwitchIpmcSameVlanL3Route, FB | BR | GW | FH | HXFX | TRX | RV | HK,
        EGR_CONFIG_1r, IPMC_ROUTE_SAME_VLANf,
        NULL, soc_feature_l3},
    { bcmSwitchIpmcSameVlanPruning, FB | BR | GW | FH | HXFX | TRX | RV | HK,
    	MISCCONFIGr, IPMC_IND_MODEf,
        _bool_invert, 0 },
#endif /* BCM_FIREBOLT_SUPPORT || BCM_HELIX_SUPPORT ||
          BCM_BRADLEY_SUPPORT || BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */

#if defined(BCM_HAWKEYE_SUPPORT)
    { bcmSwitchCpuProtoTimeSyncPrio, HK,
        CPU_CONTROL_4r, CPU_PROTO_TS_PRIORITYf,
        NULL, soc_feature_cpu_proto_prio },
    { bcmSwitchCpuProtoMmrpPrio, HK,
        CPU_CONTROL_4r, CPU_PROTO_MMRP_PRIORITYf,
        NULL, soc_feature_cpu_proto_prio },
    { bcmSwitchCpuProtoSrpPrio, HK,
        CPU_CONTROL_4r, CPU_PROTO_SRP_PRIORITYf,
        NULL, soc_feature_cpu_proto_prio },

#endif /* BCM_HAWKEYE_SUPPORT */

#if defined(BCM_TRIDENT_SUPPORT)
    { bcmSwitchPrioDropToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, DOT1P_ADMITTANCE_DROP_TOCPUf,
        NULL, soc_feature_niv},
    { bcmSwitchNivPrioDropToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, DOT1P_ADMITTANCE_DROP_TOCPUf,
        NULL, soc_feature_niv},
    { bcmSwitchNivInterfaceMissToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, NIV_FORWARDING_DROP_TOCPUf,
        NULL, soc_feature_niv},
    { bcmSwitchNivRpfFailToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, NIV_RPF_CHECK_FAIL_TOCPUf,
        NULL, soc_feature_niv},
    { bcmSwitchNivTagInvalidToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, VNTAG_FORMAT_DROP_TOCPUf,
        NULL, soc_feature_niv},
    { bcmSwitchNivTagDropToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, DISCARD_VNTAG_PRESENT_TOCPUf,
        NULL, soc_feature_niv},
    { bcmSwitchNivUntagDropToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, DISCARD_VNTAG_NOT_PRESENT_TOCPUf,
        NULL, soc_feature_niv},
    { bcmSwitchTrillTtlErrToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, TRILL_HOPCOUNT_CHECK_FAIL_TOCPUf,
        NULL, soc_feature_trill},
    { bcmSwitchTrillHeaderErrToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, TRILL_ERROR_FRAMES_TOCPUf,
        NULL, soc_feature_trill},
    { bcmSwitchTrillMismatchToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, TRILL_UNEXPECTED_FRAMES_TOCPUf,
        NULL, soc_feature_trill},
    { bcmSwitchTrillNameMissToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, TRILL_RBRIDGE_LOOKUP_MISS_TOCPUf,
        NULL, soc_feature_trill},
    { bcmSwitchTrillRpfFailToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, TRILL_RPF_CHECK_FAIL_TOCPUf,
        NULL, soc_feature_trill},
    { bcmSwitchTrillOptionsToCpu, TD | TR3 | HX4 | TD2,
        CPU_CONTROL_0r, TRILL_OPTIONS_TOCPUf,
        NULL, soc_feature_trill},
    { bcmSwitchTrillNameErrDrop, TD | TR3 | HX4 | TD2,
        TRILL_DROP_CONTROLr, INGRESS_RBRIDGE_EQ_EGRESS_RBRIDGE_DROPf,
        NULL, soc_feature_trill},
    { bcmSwitchTrillHeaderVersionErrDrop, TD | TR3 | HX4 | TD2,
        TRILL_DROP_CONTROLr, TRILL_HDR_VERSION_NON_ZERO_DROPf,
        NULL, soc_feature_trill},
    { bcmSwitchTrillNameMissDrop, TD | TR3 | HX4 | TD2,
        TRILL_DROP_CONTROLr, UNKNOWN_INGRESS_RBRIDGE_DROPf,
        NULL, soc_feature_trill},
    { bcmSwitchTrillAdjacencyFailDrop, TD | TR3 | HX4 | TD2,
        TRILL_DROP_CONTROLr, TRILL_ADJACENCY_FAIL_DROPf,
        NULL, soc_feature_trill},
    { bcmSwitchTrillHeaderErrDrop, TD | TR3 | HX4 | TD2,
        TRILL_DROP_CONTROLr, UC_TRILL_HDR_MC_MACDA_DROPf,
        NULL, soc_feature_trill},
    { bcmSwitchTrillRpfFailDrop, TD | TR3 | HX4 | TD2,
        TRILL_DROP_CONTROLr, RPF_CHECK_FAIL_DROPf,
        NULL, soc_feature_trill},
    { bcmSwitchIngParseL2TunnelTermDipSipSelect, TD | TR3 | HX4 | TD2,
        ING_L2_TUNNEL_PARSE_CONTROLr, IFP_L2_TUNNEL_PAYLOAD_FIELD_SELf ,
        NULL, 0},
    { bcmSwitchIngParseL3L4IPv4, TD | TR3 | HX4 | TD2,
        ING_L2_TUNNEL_PARSE_CONTROLr, IGMP_ENABLEf ,
        NULL, 0},
    { bcmSwitchIngParseL3L4IPv6, TD | TR3 | HX4 | TD2,
        ING_L2_TUNNEL_PARSE_CONTROLr, MLD_ENABLEf ,
        NULL, 0},
    { bcmSwitchV6L3LocalLinkDrop, TD | TR3 | HX4 | TD2,
        ING_MISC_CONFIG2r, IPV6_SIP_LINK_LOCAL_DROPf,
        NULL, 0},
    /* This switch control is valid for TD+ only.
     * 'soc_feature_l3_ecmp_1k_groups' is used to 
     * distinguish between TD and TD+ */
    { bcmSwitchEcmpMacroFlowHashEnable, TD,
        ING_CONFIG_2r, ECMP_HASH_16BITSf,
        NULL, soc_feature_l3_ecmp_1k_groups },
#endif /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    { bcmSwitchExtenderInterfaceMissToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_0r, NIV_FORWARDING_DROP_TOCPUf,
        NULL, soc_feature_port_extension},
    { bcmSwitchExtenderRpfFailToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_0r, NIV_RPF_CHECK_FAIL_TOCPUf,
        NULL, soc_feature_port_extension},
    { bcmSwitchEtagInvalidToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_0r, VNTAG_FORMAT_DROP_TOCPUf,
        NULL, soc_feature_port_extension},
    { bcmSwitchEtagDropToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_0r, DISCARD_VNTAG_PRESENT_TOCPUf,
        NULL, soc_feature_port_extension},
    { bcmSwitchNonEtagDropToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_0r, DISCARD_VNTAG_NOT_PRESENT_TOCPUf,
        NULL, soc_feature_port_extension},
    { bcmSwitchMplsGalAlertLabelToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_Mr, MPLS_GAL_EXPOSED_TO_CPUf,
        NULL, 0},
    { bcmSwitchMplsRalAlertLabelToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_Mr, MPLS_RAL_EXPOSED_TO_CPUf,
        NULL, 0},
    { bcmSwitchMplsIllegalReservedLabelToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_Mr, MPLS_ILLEGAL_RSVD_LABEL_TO_CPUf,
        NULL, 0},
    { bcmSwitchMplsUnknownAchTypeToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_Mr, MPLS_UNKNOWN_ACH_TYPE_TO_CPUf,
        NULL, 0},
    { bcmSwitchMplsUnknownAchVersionToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_Mr, MPLS_UNKNOWN_ACH_VERSION_TOCPUf,
        NULL, 0},
#endif /* BCM_TRIUMPH3_SUPPORT || BCM_TRIDENT2_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    { bcmSwitchFailoverDropToCpu, TR3 | HX4,
        CPU_CONTROL_1r, PROTECTION_DATA_DROP_TOCPUf,
        NULL, 0},
    { bcmSwitchMplsReservedEntropyLabelToCpu, TR3 | HX4,
        CPU_CONTROL_Mr, ENTROPY_LABEL_IN_0_15_RANGE_TO_CPUf,
        NULL, 0},
    { bcmSwitchL3SrcBindMissToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_Mr, MAC_IP_BIND_LOOKUP_MISS_TOCPUf,
        NULL, 0},
    { bcmSwitchMplsLookupsExceededToCpu, TR3 | HX4,
        CPU_CONTROL_Mr, MPLS_OUT_OF_LOOKUPS_TOCPUf,
        NULL, 0},
    { bcmSwitchTimesyncUnknownVersionToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_Mr, UNKNOWN_1588_VERSION_TO_CPUf,
        NULL, 0},
    { bcmSwitchTimesyncIngressVersion, TR3 | HX4 | TD2,
        ING_1588_PARSING_CONTROLr, VERSION_CONTROLf,
        NULL, 0},
    { bcmSwitchTimesyncEgressVersion, TR3 | HX4 | TD2,
        EGR_1588_PARSING_CONTROLr, VERSION_CONTROLf,
        NULL, 0},
    { bcmSwitchCongestionCnmToCpu, TR3 | HX4 | TD2,
        IE2E_CONTROLr, ICNM_TO_CPUf,
        NULL, 0},
    { bcmSwtichCongestionCnmProxyErrorToCpu, TR3 | HX4 | TD2,
        ING_MISC_CONFIGr, QCN_CNM_PRP_DLF_TO_CPUf,
        NULL, 0},
    { bcmSwtichCongestionCnmProxyToCpu, TR3 | HX4 | TD2,
        ING_MISC_CONFIGr, QCN_CNM_PRP_TO_CPUf,
        NULL, 0},
    { bcmSwitchL2GreTunnelMissToCpu, TR3 | HX4,
        CPU_CONTROL_Mr, L2GRE_SIP_LOOKUP_FAIL_COPY_TOCPUf,
        NULL, 0},
    { bcmSwitchL2GreVpnIdMissToCpu, TR3 | HX4 | TD2,
        CPU_CONTROL_Mr, L2GRE_VPNID_LOOKUP_FAIL_COPY_TOCPUf,
        NULL, 0},
    { bcmSwitchWlanTunnelErrorDrop, TR3 | HX4,
        ING_CONFIG_64r, WLAN_DECRYPT_OFFLOAD_ENABLEf,
        _bool_invert, 0},
    { bcmSwitchMplsReservedEntropyLabelDrop, TR3 | HX4,
        DROP_CONTROL_0r, ENTROPY_LABEL_IN_0_15_RANGE_DO_NOT_DROPf,
        _bool_invert, 0},
    { bcmSwitchRemoteProtectionTrust, TR3 | HX4 | TD2,
        ING_CONFIG_64r, USE_PROT_STATUSf,
        NULL, 0},
    { bcmSwitchVxlanTunnelMissToCpu, TD2,
        CPU_CONTROL_Mr, VXLAN_SIP_LOOKUP_FAIL_COPY_TOCPUf,
        NULL, 0},
    { bcmSwitchVxlanVnIdMissToCpu, TD2,
        CPU_CONTROL_Mr, VXLAN_VN_ID_LOOKUP_FAIL_COPY_TOCPUf,
        NULL, 0},

#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
    { bcmSwitchL3NATEnable, TD2,
        ING_CONFIG_64r, NAT_ENABLEf,
        NULL, 0},
    { bcmSwitchL3DNATHairpinToCpu, TD2,
        CPU_CONTROL_Mr, DNAT_HAIRPIN_COPY_TOCPUf,
        NULL, 0},
    { bcmSwitchL3DNATMissToCpu, TD2,
        CPU_CONTROL_Mr, DNAT_MISS_COPY_TOCPUf,
        NULL, 0},
    { bcmSwitchL3SNATMissToCpu, TD2,
        CPU_CONTROL_Mr, SNAT_MISS_COPY_TOCPUf,
        NULL, 0},
    { bcmSwitchL3NatOtherToCpu, TD2,
        CPU_CONTROL_Mr, NAT_OTHER_COPY_TOCPUf,
        NULL, 0},
    { bcmSwitchL3NatRealmCrossingIcmpToCpu, TD2,
        CPU_CONTROL_Mr, NAT_REALM_CROSSING_ICMP_COPY_TOCPUf,
        NULL, 0},
    { bcmSwitchL3NatFragmentsToCpu, TD2,
        CPU_CONTROL_Mr, NAT_FRAGMENTS_COPY_TOCPUf,
        NULL, 0},
    { bcmSwitchHashUseL2GreTunnelGreKey0, TD2,
        RTAG7_HASH_CONTROL_2r, L2GRE_TUNNEL_USE_GRE_KEY_Af,
        NULL, soc_feature_l2gre},
    { bcmSwitchHashUseL2GreTunnelGreKey1, TD2,
        RTAG7_HASH_CONTROL_2r, L2GRE_TUNNEL_USE_GRE_KEY_Bf,
        NULL, soc_feature_l2gre},
    { bcmSwitchHashL2GrePayloadSelect0, TD2,
        RTAG7_HASH_CONTROL_2r, L2GRE_PAYLOAD_HASH_SELECT_Af,
        NULL, soc_feature_l2gre},
    { bcmSwitchHashL2GrePayloadSelect1, TD2,
        RTAG7_HASH_CONTROL_2r, L2GRE_PAYLOAD_HASH_SELECT_Bf,
        NULL, soc_feature_l2gre},
    { bcmSwitchHashL2GreNetworkPortPayloadDisable0, TD2,
        RTAG7_HASH_CONTROL_2r, DISABLE_HASH_L2GRE_Af,
        NULL, soc_feature_l2gre},
    { bcmSwitchHashL2GreNetworkPortPayloadDisable1, TD2,
        RTAG7_HASH_CONTROL_2r, DISABLE_HASH_L2GRE_Bf,
        NULL, soc_feature_l2gre},
    { bcmSwitchEcmpMacroFlowHashEnable, TD2,
        RTAG7_HASH_SELr, USE_FLOW_SEL_ECMPf,
        NULL, 0},
#endif /* BCM_TRIDENT2_SUPPORT */
};
#endif /* BCM_XGS3_SWITCH_SUPPORT */

static uint8 *_mod_type_table[BCM_LOCAL_UNITS_MAX]; 

STATIC int
_bcm_switch_module_type_set(int unit, bcm_module_t mod, uint32 mod_type)
{
    int modid_cnt;

    if (mod > SOC_MODID_MAX(unit)) {
        return BCM_E_PARAM;
    } else {
        if (NULL == _mod_type_table[unit]) {
            if (SOC_MODID_MAX(unit)) {
                modid_cnt = SOC_MODID_MAX(unit);
            } else {
                modid_cnt = 1;
            }
            _mod_type_table[unit] = (uint8 *)sal_alloc(
                (sizeof(uint8) * modid_cnt), "MOD TYPE");
            if (NULL == _mod_type_table[unit]) {
                return BCM_E_MEMORY;
            }
            sal_memset(_mod_type_table[unit], BCM_SWITCH_MODULE_UNKNOWN, 
                       (sizeof(uint8) * modid_cnt));
        }
        _mod_type_table[unit][mod] = mod_type;
    }
    return BCM_E_NONE;
}

int
_bcm_switch_module_type_get(int unit, bcm_module_t mod, uint32 *mod_type)
{
    *mod_type = BCM_SWITCH_MODULE_UNKNOWN;
    if (mod > SOC_MODID_MAX(unit)) {
        return BCM_E_PARAM;
    } else {
        if (NULL != _mod_type_table[unit]) {
            *mod_type = _mod_type_table[unit][mod];
        }
    }
    return BCM_E_NONE;
}

#define CLEARMEM(_unit, _mem) \
        if (SOC_MEM_IS_VALID(_unit, _mem) && SOC_MEM_SIZE(_unit, _mem) > 0) {\
            SOC_IF_ERROR_RETURN(soc_mem_clear(_unit, _mem, MEM_BLOCK_ALL, 0));\
        }
/*
 * Bulk clear of various hit bit tables
 */
STATIC int
_bcm_esw_switch_hit_clear_set(int unit, bcm_switch_control_t type, int arg)
{
    if (arg != 1) {
        return BCM_E_PARAM;
    }

    switch (type) {
    case bcmSwitchL2HitClear:
        if (SOC_IS_FBX(unit)) {
            CLEARMEM(unit, L2_HITSA_ONLYm);
            CLEARMEM(unit, L2_HITDA_ONLYm);
            if (soc_feature(unit, soc_feature_esm_support)) {
                CLEARMEM(unit, EXT_SRC_HIT_BITS_L2m);
                CLEARMEM(unit, EXT_DST_HIT_BITS_L2m);
            }
            return BCM_E_NONE;
        }
        break;
    case bcmSwitchL2SrcHitClear:
        if (SOC_IS_FBX(unit)) {
            CLEARMEM(unit, L2_HITSA_ONLYm);
            if (soc_feature(unit, soc_feature_esm_support)) {
                CLEARMEM(unit, EXT_SRC_HIT_BITS_L2m);
            }
            return BCM_E_NONE;
        }
        break;
    case bcmSwitchL2DstHitClear:
        if (SOC_IS_FBX(unit)) {
            CLEARMEM(unit, L2_HITDA_ONLYm);
            if (soc_feature(unit, soc_feature_esm_support)) {
                CLEARMEM(unit, EXT_DST_HIT_BITS_L2m);
            }
            return BCM_E_NONE;
        }
        break;
    case bcmSwitchL3HostHitClear:
        if (!soc_feature(unit, soc_feature_l3) ||
            soc_feature(unit, soc_feature_fp_based_routing)) {
            break;
        }
        if (SOC_IS_FBX(unit)) {
            CLEARMEM(unit, L3_ENTRY_HIT_ONLYm);
            return BCM_E_NONE;
        }
        break;
    case bcmSwitchL3RouteHitClear:
        if (!soc_feature(unit, soc_feature_l3) ||
            soc_feature(unit, soc_feature_fp_based_routing)) {
            break;
        }
        if (SOC_IS_FBX(unit)) {
            CLEARMEM(unit, L3_DEFIP_HIT_ONLYm);
            CLEARMEM(unit, L3_DEFIP_128_HIT_ONLYm);
            if (soc_feature(unit, soc_feature_esm_support)) {
                CLEARMEM(unit, EXT_SRC_HIT_BITS_IPV4m);
                CLEARMEM(unit, EXT_SRC_HIT_BITS_IPV6_64m);
                CLEARMEM(unit, EXT_SRC_HIT_BITS_IPV6_128m);
                CLEARMEM(unit, EXT_DST_HIT_BITS_IPV4m);
                CLEARMEM(unit, EXT_DST_HIT_BITS_IPV6_64m);
                CLEARMEM(unit, EXT_DST_HIT_BITS_IPV6_128m);
            }
            return BCM_E_NONE;
        }
        break;
    default:
        break;
    }

    return BCM_E_UNAVAIL;
}

#undef CLEARMEM

/*
 * Return an error if a hit clear is not supportable on this device
 */
STATIC int
_bcm_esw_switch_hit_clear_get(int unit, bcm_switch_control_t type, int *arg)
{
    int         rv;

    rv = BCM_E_UNAVAIL;

    switch (type) {
    case bcmSwitchL2HitClear:
        if (SOC_IS_FBX(unit)) {
            rv = BCM_E_NONE;
            break;
        }
        break;
    case bcmSwitchL2SrcHitClear:
        if (SOC_IS_FBX(unit)) {
            rv = BCM_E_NONE;
            break;
        }
        break;
    case bcmSwitchL2DstHitClear:
        if (SOC_IS_FBX(unit)) {
            rv = BCM_E_NONE;
            break;
        }
        break;
    case bcmSwitchL3HostHitClear:
        if (!soc_feature(unit, soc_feature_l3) ||
            soc_feature(unit, soc_feature_fp_based_routing)) {
            break;
        }
        if (SOC_IS_FBX(unit)) {
            rv = BCM_E_NONE;
            break;
        }
        break;
    case bcmSwitchL3RouteHitClear:
        if (!soc_feature(unit, soc_feature_l3) ||
            soc_feature(unit, soc_feature_fp_based_routing)) {
            break;
        }
        if (SOC_IS_FBX(unit)) {
            rv = BCM_E_NONE;
            break;
        }
        break;
    default:
        break;
    }
    if (rv == BCM_E_NONE && arg != NULL) {
        *arg = 0;
    }
    return rv;
}

#if defined(BCM_HAWKEYE_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT) \
                                 || defined(BCM_APOLLO_SUPPORT)
/*
 * Function:
 *      _bcm_switch_class_pkt_prio_set
 * Description:
 *      Helper function to program packet priority per traffic class.
 * Parameters:
 *      unit - Device unit number
 *  type - The desired configuration parameter to modify
 *  arg - The value with which to set the parameter
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_switch_class_pkt_prio_set(int unit,
                               bcm_switch_control_t type,
                               int arg)
{
#if defined(BCM_HAWKEYE_SUPPORT)
    if (SOC_IS_HAWKEYE(unit)) {
        soc_reg_t   reg[2];
        soc_field_t field;
        int         i;

        reg[0] = reg[1] = INVALIDr;
        field = INVALIDf;

        switch (type) {
            case bcmSwitchTimeSyncClassAPktPrio:
                reg[0] = ING_EAV_CLASSr;
                reg[1] = EGR_EAV_CLASSr;
                field = CLASS_Af;
                break;
            case bcmSwitchTimeSyncClassBPktPrio:
                reg[0] = ING_EAV_CLASSr;
                reg[1] = EGR_EAV_CLASSr;
                field = CLASS_Bf;
                break;
            case bcmSwitchTimeSyncClassAExeptionPktPrio:
                reg[0] = EGR_EAV_CLASSr;
                field = REMAP_CLASS_Af;
                break;
            case bcmSwitchTimeSyncClassBExeptionPktPrio:
                reg[0] =  EGR_EAV_CLASSr;
                field = REMAP_CLASS_Bf;
                break;
            default:
                return BCM_E_PARAM;
        }

        for (i = 0; i < 2; i++) {
            if (INVALIDr != reg[i]) {
                BCM_IF_ERROR_RETURN(
                soc_reg_field32_modify(unit, reg[i], REG_PORT_ANY, field, arg));
            }
        }

        return BCM_E_NONE;
    }
#endif /* BCM_HAWKEYE_SUPPORT */
    return BCM_E_UNAVAIL;
}
#endif /*BCM_HAWKEYE_SUPPORT || BCM_TRIUMPH2_SUPPORT || BCM_APOLLO_SUPPORT */

/*
 * Function:
 *      _bcm_switch_class_pkt_prio_get
 * Description:
 *      Helper function to read packet priority per traffic class.
 * Parameters:
 *      unit - Device unit number
 *  type - The desired configuration parameter to read
 *  arg - The value with which is programmed
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_switch_class_pkt_prio_get(int unit,
                               bcm_switch_control_t type,
                               int *arg)
{
#if defined(BCM_HAWKEYE_SUPPORT)
    if (SOC_IS_HAWKEYE(unit)) {
        soc_reg_t   reg;
        soc_field_t field;
        uint32      rval;
        
        switch (type) {
            case bcmSwitchTimeSyncClassAPktPrio:
                reg = ING_EAV_CLASSr;
                field = CLASS_Af;
                break;
            case bcmSwitchTimeSyncClassBPktPrio:
                reg = ING_EAV_CLASSr;
                field = CLASS_Bf;
                break;
            case bcmSwitchTimeSyncClassAExeptionPktPrio:
                reg = EGR_EAV_CLASSr;
                field = REMAP_CLASS_Af;
                break;
            case bcmSwitchTimeSyncClassBExeptionPktPrio:
                reg =  EGR_EAV_CLASSr;
                field = REMAP_CLASS_Bf;
                break;
            default:
                return BCM_E_PARAM;
        }

        BCM_IF_ERROR_RETURN(
            soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &rval));
        *arg = soc_reg_field_get(unit, reg, rval, field);

        return BCM_E_NONE;
    }
#endif /* BCM_HAWKEYE_SUPPORT */
    return BCM_E_UNAVAIL;
}


#if (defined(BCM_RCPU_SUPPORT) || defined(BCM_OOB_RCPU_SUPPORT)) && \
     defined(BCM_XGS3_SWITCH_SUPPORT)
/*
 * Function:
 *      _bcm_rcpu_switch_regall_idx_set
 * Description:
 *      Helper function to set register to all ones if arg is non zero
 *  and to all zeros otherwise.
 * Parameters:
 *      unit - Device unit number
 *  reg - The desired register to program
 *  idx - The register index
 *  arg - The value with which decission is made
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_rcpu_switch_regall_idx_set(int unit, 
                                soc_reg_t reg,
                                int idx, 
                                soc_field_t field,
                                int arg)
{
    uint32  raddr, rval, value, width;

    rval = 0;
    if (arg) {
        width = soc_reg_field_length(unit, reg, field);
        if (32 == width) {
            value = 0xffffffff;
        } else {
            value = (1 << width) - 1;
        }
         soc_reg_field_set(unit, reg, &rval, field, value);
    }
    raddr = soc_reg_addr(unit, reg, REG_PORT_ANY, idx);
    return soc_pci_write(unit, raddr, rval);
}
/*
 * Function:
 *      _bcm_rcpu_switch_regall_set
 * Description:
 *      Helper function to set register to all ones if arg is non zero
 *  and to all zeros otherwise.
 * Parameters:
 *      unit - Device unit number
 *  reg - The desired register to program
 *  arg - The value with which decission is made
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_rcpu_switch_regall_set(int unit, soc_reg_t reg, soc_field_t field, int arg)
{
    return _bcm_rcpu_switch_regall_idx_set(unit, reg, 0, field, arg);
}

/*
 * Function:
 *      _bcm_rcpu_switch_regall_get
 * Description:
 *      Helper function to return arg = 1 if register is set to all ones 
 *  and to zero otherwise.
 * Parameters:
 *      unit - Device unit number
 *  reg - The desired register to read
 *  arg - The value returnrd
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_rcpu_switch_regall_idx_get(int unit, soc_reg_t reg, int idx, int *arg)
{
    uint32  raddr, rval;

    raddr = soc_reg_addr(unit, reg, REG_PORT_ANY, idx);
    BCM_IF_ERROR_RETURN(
        soc_pci_getreg(unit, raddr, &rval));
    
    *arg = (!rval) ? 0 : 1;

    return BCM_E_NONE;
}

STATIC int
_bcm_rcpu_switch_regall_get(int unit, soc_reg_t reg, int *arg)
{
    return _bcm_rcpu_switch_regall_idx_get(unit, reg, 0, arg);
}

/*
 * Function:
 *      _bcm_rcpu_switch_vlan_tpid_sig_set
 * Description:
 *      Helper function to set VLAN, TPID ot Signature for RCPU  
 * Parameters:
 *      unit - Device unit number
 *  type - The desired configuration parameter to modify
 *  arg - The value to set
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_rcpu_switch_vlan_tpid_sig_set(int unit, 
                                   bcm_switch_control_t type, 
                                   int arg)
{
    soc_reg_t   reg = INVALIDr;
    soc_field_t field = INVALIDf;
    uint32      value, raddr, rval;

    if (!soc_feature(unit, soc_feature_rcpu_1)) {
        return BCM_E_UNAVAIL;
    }

    switch (type) {
        case bcmSwitchRemoteCpuVlan:
            reg = CMIC_PKT_VLANr;
            field = VLAN_IDf;
            value = (arg & 0xffff);
            break;
        case bcmSwitchRemoteCpuTpid:
            reg = CMIC_PKT_VLANr;
            field = TPIDf;
            value = (arg & 0xffff);
            break;
        case bcmSwitchRemoteCpuSignature:
            reg = CMIC_PKT_ETHER_SIGr;
            field = SIGNATUREf;
            value = (arg & 0xffff);
            break;
        case bcmSwitchRemoteCpuEthertype:
            reg = CMIC_PKT_ETHER_SIGr;
            field = ETHERTYPEf;
            value = (arg & 0xffff);
            break;
        default:
            return BCM_E_PARAM;
    }

    raddr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
    
    BCM_IF_ERROR_RETURN(
        soc_pci_getreg(unit, raddr, &rval));
    soc_reg_field_set(unit, reg, &rval, field, value);

    return soc_pci_write(unit, raddr, rval);
}

/*
 * Function:
 *      _bcm_rcpu_switch_vlan_tpid_sig_get
 * Description:
 *      Helper function to get VLAN, TPID ot Signature for RCPU  
 * Parameters:
 *      unit - Device unit number
 *  type - The desired configuration parameter to get
 *  arg - The value returned
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_rcpu_switch_vlan_tpid_sig_get(int unit, 
                                   bcm_switch_control_t type, 
                                   int *arg)
{
    soc_reg_t   reg;
    soc_field_t field;
    uint32  raddr, rval;


    if (!soc_feature(unit, soc_feature_rcpu_1)) {
        return BCM_E_UNAVAIL;
    }

    switch (type) {
        case bcmSwitchRemoteCpuVlan:
            reg = CMIC_PKT_VLANr;
            field = VLAN_IDf;
            break;
        case bcmSwitchRemoteCpuTpid:
            reg = CMIC_PKT_VLANr;
            field = TPIDf;
            break;
        case bcmSwitchRemoteCpuSignature:
            reg = CMIC_PKT_ETHER_SIGr;
            field = SIGNATUREf;
            break;
        case bcmSwitchRemoteCpuEthertype:
            reg = CMIC_PKT_ETHER_SIGr;
            field = ETHERTYPEf;
            break;
        default:
            return BCM_E_PARAM;
    }

    raddr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
    BCM_IF_ERROR_RETURN(
        soc_pci_getreg(unit, raddr, &rval));

    *arg = soc_reg_field_get(unit, reg, rval, field);

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_rcpu_switch_mac_lo_set
 * Description:
 *      Set the low 24 bits of MAC address field for RCPU MAC registers
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control type to set the mac for
 *      mac_lo      - MAC address
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_rcpu_switch_mac_lo_set(int unit,
                            bcm_switch_control_t type,
                            uint32 mac_lo)
{
    uint32      regval, fieldval;
    soc_reg_t   reg = INVALIDr ;
    soc_field_t f_lo = INVALIDf;


    /* Given control type select register. */
    switch (type) {
        case bcmSwitchRemoteCpuLocalMacNonOui:
            reg = CMIC_PKT_LMAC0_LOr;
            f_lo = MAC0_LOf;
            break;
        case bcmSwitchRemoteCpuDestMacNonOui:
            reg = CMIC_PKT_RMACr;
            f_lo = MAC_LOf;
            break;
        default:
            return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(
        soc_pci_getreg(unit, soc_reg_addr(unit, reg, REG_PORT_ANY, 0), 
                       &regval));

    fieldval = soc_reg_field_get(unit, reg, regval, f_lo);
    fieldval |= ((mac_lo << 8) >> 8);
    soc_reg_field_set(unit, reg, &regval, f_lo, fieldval);

    return soc_pci_write(unit, soc_reg_addr(unit, reg, REG_PORT_ANY, 0), 
                         regval);
}

/*
 * Function:
 *      _bcm_rcpu_switch_mac_hi_set
 * Description:
 *      Set the upper 24 bits of MAC address field for RCPU registers
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control type to set upper MAC for
 *      mac_hi      - MAC address
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_rcpu_switch_mac_hi_set(int unit,
                            bcm_switch_control_t type,
                            uint32 mac_hi)
{
    soc_reg_t   reg1, reg2;
    soc_field_t f_lo, f_hi;
    uint32      fieldval, regval1, regval2, raddr1, raddr2;

    reg1 = reg2 = INVALIDr;

    /* Given control type select register. */
    switch (type) {
        case bcmSwitchRemoteCpuLocalMacOui:
            reg1 = CMIC_PKT_LMAC0_LOr;
            reg2 = CMIC_PKT_LMAC0_HIr;
            f_lo = MAC0_LOf;
            f_hi = MAC0_HIf;
            break;
        case bcmSwitchRemoteCpuDestMacOui:
            reg1 = CMIC_PKT_RMACr;
            reg2 = CMIC_PKT_RMAC_HIr;
            f_lo = MAC_LOf;
            f_hi = MAC_HIf;
            break;
        default:
            return BCM_E_PARAM;
    }

    regval1 = regval2 = 0;

    raddr1 = soc_reg_addr(unit, reg1, REG_PORT_ANY, 0);
    raddr2 = soc_reg_addr(unit, reg2, REG_PORT_ANY, 0);

    BCM_IF_ERROR_RETURN(
        soc_pci_getreg(unit, raddr1, &regval1));

    BCM_IF_ERROR_RETURN(
        soc_pci_getreg(unit, raddr2, &regval2));

    fieldval = (mac_hi << 24);
    soc_reg_field_set(unit, reg1, &regval1, f_lo, fieldval);
    fieldval = (mac_hi >> 8) & 0xffff;
    soc_reg_field_set(unit, reg2, &regval2, f_hi, fieldval);

    SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr1, regval1));
    SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr2, regval2));

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_rcpu_switch_mac_lo_get
 * Description:
 *      Get the lower 24 bits of MAC address field for RCPU MAC registers
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control type to get MAC for
 *      arg         - arg to get the lower MAC address
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_rcpu_switch_mac_lo_get(int unit, 
                            bcm_switch_control_t type,
                            int *arg)
{
    soc_reg_t   reg = INVALIDr ;
    soc_field_t f_lo = INVALIDf; 
    uint32      regval, mac;

    /* Given control type select register. */
    switch (type) {
        case bcmSwitchRemoteCpuLocalMacNonOui:
            reg = CMIC_PKT_LMAC0_LOr;
            f_lo = MAC0_LOf;
            break;
        case bcmSwitchRemoteCpuDestMacNonOui:
            reg = CMIC_PKT_RMACr;
            f_lo = MAC_LOf;
            break;
        default:
            return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(
        soc_pci_getreg(unit, soc_reg_addr(unit, reg, REG_PORT_ANY, 0), 
                       &regval));
    mac = soc_reg_field_get(unit, reg, regval, f_lo);

    *arg = (mac << 8) >> 8;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_rcpu_switch_mac_hi_get
 * Description:
 *      Get the upper 24 bits of MAC address field for RCPU MAC registers
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control type to get MAC for
 *      arg         - arg to get the upper MAC address
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_rcpu_switch_mac_hi_get(int unit, 
                            bcm_switch_control_t type,
                         int *arg)
{
    soc_reg_t   reg1, reg2;
    soc_field_t f_lo, f_hi;
    uint32      mac, regval1, regval2;

    reg1 = reg2 = INVALIDr;

    /* Given control type select register. */
    switch (type) {
        case bcmSwitchRemoteCpuLocalMacOui:
            reg1 = CMIC_PKT_LMAC0_LOr;
            reg2 = CMIC_PKT_LMAC0_HIr;
            f_lo = MAC0_LOf;
            f_hi = MAC0_HIf;
            break;
        case bcmSwitchRemoteCpuDestMacOui:
            reg1 = CMIC_PKT_RMACr;
            reg2 = CMIC_PKT_RMAC_HIr;
            f_lo = MAC_LOf;
            f_hi = MAC_HIf;
            break;
        default:
            return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(
        soc_pci_getreg(unit, soc_reg_addr(unit, reg1, REG_PORT_ANY, 0), 
                       &regval1));
    mac = (soc_reg_field_get(unit, reg1, regval1, f_lo) >> 24);

    SOC_IF_ERROR_RETURN(
        soc_pci_getreg(unit, soc_reg_addr(unit, reg2, REG_PORT_ANY, 0), 
                       &regval2));

    mac |= (soc_reg_field_get(unit, reg2, regval2, f_hi) << 8);

    *arg = (int)mac;

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_rcpu_pipe_bypass_header_set
 * Description:
 *      Prepare and program the SOBMH header for RCPU usage 
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control type to get MAC for
 *      arg         - port number to specify in the SOBMH
 * Returns:
 *      BCM_E_xxxx
 */
STATIC int 
_bcm_rcpu_pipe_bypass_header_set(int unit, 
                            bcm_switch_control_t type,
                            int arg)
{
    uint32          pbh[3];     /* 3 words SOBMH header to program */
    bcm_port_t      port;
    bcm_module_t    modid; 
    uint32          raddr;
    int qnum;

    if (BCM_GPORT_IS_SET(arg)) {
        bcm_trunk_t tgid;
        int         id;

        BCM_IF_ERROR_RETURN(
            _bcm_esw_gport_resolve(unit, arg, &modid, &port, &tgid, &id));
        if ((BCM_TRUNK_INVALID != tgid) || (-1 != id) ) {
            return BCM_E_PORT;
        }
    } else {
        port = arg;
        modid = SOC_DEFAULT_DMA_SRCMOD_GET(unit);
    }

    if (!SOC_PORT_VALID(unit, port)) {
        return BCM_E_PORT;
    }

    /* Setup SOBMH header according to supplied argument */
    if (SOC_DCB_TYPE(unit) == 23 || SOC_DCB_TYPE(unit) == 26 || SOC_DCB_TYPE(unit) == 29){
#ifdef BCM_KATANA2_SUPPORT
        if (SOC_DCB_TYPE(unit) == 29) {
            qnum = SOC_INFO(unit).port_cosq_base[port];
        } else
#endif
        {
            qnum = SOC_INFO(unit).port_uc_cosq_base[port];
        }
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_DCB_TYPE(unit) == 26) {
            qnum = soc_td2_logical_qnum_hw_qnum(unit, port, qnum, 1);
        }
#endif /* BCM_TRIDENT2_SUPPORT */
        PBS_MH_V7_W0_START_SET(pbh);
#ifdef BCM_KATANA2_SUPPORT
        if (SOC_DCB_TYPE(unit) == 29) {
            PBS_MH_V8_W1_DPORT_IPRI_SET(pbh, port, 0);
            PBS_MH_V8_W2_SMOD_COS_QNUM_SET(pbh, modid, 1, 0, qnum);
        } else
#endif
        {
            PBS_MH_V7_W1_DPORT_QNUM_SET(pbh, port, qnum);
            PBS_MH_V7_W2_SMOD_COS_QNUM_SET(pbh, modid, 1, 0, qnum, 0);
        }
    } else {
        PBS_MH_W0_START_SET(pbh);
        PBS_MH_W1_RSVD_SET(pbh);
        if ((SOC_DCB_TYPE(unit) == 11) || (SOC_DCB_TYPE(unit) == 12) ||
            (SOC_DCB_TYPE(unit) == 15) || (SOC_DCB_TYPE(unit) == 17) ||
            (SOC_DCB_TYPE(unit) == 18)) {
            PBS_MH_V2_W2_SMOD_DPORT_COS_SET(pbh, modid, port, 0, 0, 0);
        } else if ((SOC_DCB_TYPE(unit) == 14) ||
                   (SOC_DCB_TYPE(unit) == 19) ||
                   (SOC_DCB_TYPE(unit) == 20)) {

            PBS_MH_V3_W2_SMOD_DPORT_COS_SET(pbh, modid, port,0, 0, 0);
        } else if (SOC_DCB_TYPE(unit) == 16) {
            PBS_MH_V4_W2_SMOD_DPORT_COS_SET(pbh, modid, port,0, 0, 0);
        } else if (SOC_DCB_TYPE(unit) == 21) {
            PBS_MH_V5_W1_SMOD_SET(pbh, modid, 1, 0, 0);
            PBS_MH_V5_W2_DPORT_COS_SET(pbh, port, 0, 0);
        } else if (SOC_DCB_TYPE(unit) == 24) {
            PBS_MH_V5_W1_SMOD_SET(pbh, modid, 1, 0, 0);
            PBS_MH_V6_W2_DPORT_COS_QNUM_SET(pbh, port, 0, 
                             (SOC_INFO(unit).port_cosq_base[port]), 0);
        } else {
            PBS_MH_V1_W2_SMOD_DPORT_COS_SET(pbh, modid, port, 0, 0, 0);
        }
    }

    raddr = soc_reg_addr(unit, CMIC_PKT_RMH0r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, pbh[0]));
    raddr = soc_reg_addr(unit, CMIC_PKT_RMH1r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, pbh[1]));
    raddr = soc_reg_addr(unit, CMIC_PKT_RMH2r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, pbh[2]));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_rcpu_pipe_bypass_header_get
 * Description:
 *      Get the SOBMH header for RCPU usage and retrieve the port numebr 
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      port        - port number.
 *      type        - The required switch control type to get MAC for
 *      arg         - port number in the SOBMH header 
 * Returns:
 *      BCM_E_xxxx
 */
STATIC int 
_bcm_rcpu_pipe_bypass_header_get(int unit, 
                            bcm_switch_control_t type,
                            int *arg)
{
    soc_pbsmh_hdr_t *pbh;
    uint32          hw_buff[3];     /* 3 words SOBMH header */
    uint32          raddr;
    int             isGport;

    if (NULL == arg) {
        return BCM_E_PARAM;
    }

    raddr = soc_reg_addr(unit, CMIC_PKT_RMH0r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &(hw_buff[0])));
    raddr = soc_reg_addr(unit, CMIC_PKT_RMH1r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &(hw_buff[1])));
    raddr = soc_reg_addr(unit, CMIC_PKT_RMH2r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &(hw_buff[2])));

    pbh = (soc_pbsmh_hdr_t *)hw_buff;

    BCM_IF_ERROR_RETURN(
        bcm_esw_switch_control_get(unit, bcmSwitchUseGport, &isGport));

    if (isGport) {
        _bcm_gport_dest_t gport_st; 

        gport_st.gport_type = BCM_GPORT_TYPE_MODPORT;
        gport_st.modid = soc_pbsmh_field_get(unit, pbh, PBSMH_src_mod);
        gport_st.port = soc_pbsmh_field_get(unit, pbh, PBSMH_dst_port);
        BCM_IF_ERROR_RETURN(
            _bcm_esw_gport_construct(unit, &gport_st, arg));
    } else {
        *arg = soc_pbsmh_field_get(unit, pbh, PBSMH_dst_port);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_rcpu_higig_header_set
 * Description:
 *      Prepare and program the HIGIG header for RCPU usage 
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      arg         - port number to specify in the SOBMH
 * Returns:
 *      BCM_E_xxxx
 */
STATIC int 
_bcm_rcpu_higig_header_set(int unit, int arg)
{
#if defined(BCM_HIGIG2_SUPPORT)
    uint32           hghdr[4];     /* 4 words HIGIG2 header to program */
    soc_higig2_hdr_t *xgh = (soc_higig2_hdr_t *)hghdr;
    bcm_port_t      port;
    bcm_module_t    modid; 
    uint32          raddr, rval, vlan_val;

    if (BCM_GPORT_IS_SET(arg)) {
        bcm_trunk_t tgid;
        int         id;

        BCM_IF_ERROR_RETURN(
            _bcm_esw_gport_resolve(unit, arg, &modid, &port, &tgid, &id));
        if ((BCM_TRUNK_INVALID != tgid) || (-1 != id) ) {
            return BCM_E_PORT;
        }
    } else {
        port = arg;
        modid = SOC_DEFAULT_DMA_SRCMOD_GET(unit);
    }

    if (!SOC_PORT_VALID(unit, port)) {
        return BCM_E_PORT;
    }

    sal_memset(hghdr, 0, sizeof(soc_higig2_hdr_t));
    soc_higig2_field_set(unit, xgh, HG_start, SOC_HIGIG2_START);
    soc_higig2_field_set(unit, xgh, HG_ppd_type, 0);
    soc_higig2_field_set(unit, xgh, HG_dst_mod, modid);
    soc_higig2_field_set(unit, xgh, HG_dst_port, port);

    raddr = soc_reg_addr(unit, CMIC_PKT_VLANr, REG_PORT_ANY, 0);
    BCM_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
    vlan_val = soc_reg_field_get(unit, CMIC_PKT_VLANr, rval, VLAN_IDf);
    if (0 == vlan_val) {
        vlan_val = DEFAULT_RCPU_VLAN;
    }
    soc_higig2_field_set(unit, xgh, HG_vlan_tag, vlan_val);
    soc_higig2_field_set(unit, xgh, HG_opcode, SOC_HIGIG_OP_CPU);

    raddr = soc_reg_addr(unit, CMIC_PKT_RMH0r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, hghdr[0]));
    raddr = soc_reg_addr(unit, CMIC_PKT_RMH1r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, hghdr[1]));
    raddr = soc_reg_addr(unit, CMIC_PKT_RMH2r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, hghdr[2]));
    raddr = soc_reg_addr(unit, CMIC_PKT_RMH3r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, hghdr[3]));

    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif /* BCM_HIGIG2_SUPPORT */
}

/*
 * Function:
 *      _bcm_rcpu_higig_header_get
 * Description:
 *      Get the SOBMH header for RCPU usage and retrieve the port numebr 
 * Parameters:
 *      unit        - StrataSwitch PCI device unit number (driver internal).
 *      type        - The required switch control type to get MAC for
 *      arg         - port number in the SOBMH header 
 * Returns:
 *      BCM_E_xxxx
 */
STATIC int 
_bcm_rcpu_higig_header_get(int unit, int *arg)
{
#if defined(BCM_HIGIG2_SUPPORT)
    uint32          hghdr[4];     /* 4 words HIGIG2 header to program */
    soc_higig2_hdr_t *xgh = (soc_higig2_hdr_t *)hghdr;
    uint32          raddr;
    int             isGport;

    if (NULL == arg) {
        return BCM_E_PARAM;
    }

    raddr = soc_reg_addr(unit, CMIC_PKT_RMH0r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &(hghdr[0])));
    raddr = soc_reg_addr(unit, CMIC_PKT_RMH1r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &(hghdr[1])));
    raddr = soc_reg_addr(unit, CMIC_PKT_RMH2r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &(hghdr[2])));
    raddr = soc_reg_addr(unit, CMIC_PKT_RMH3r, REG_PORT_ANY, 0);
    SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &(hghdr[3])));

    BCM_IF_ERROR_RETURN(
        bcm_esw_switch_control_get(unit, bcmSwitchUseGport, &isGport));

    if (isGport) {
        _bcm_gport_dest_t gport_st; 

        gport_st.gport_type = BCM_GPORT_TYPE_MODPORT;
        gport_st.modid = soc_higig2_field_get(unit, xgh, HG_dst_mod);
        gport_st.port = soc_higig2_field_get(unit, xgh, HG_dst_port);
        BCM_IF_ERROR_RETURN(
            _bcm_esw_gport_construct(unit, &gport_st, arg));
    } else {
        *arg = soc_higig2_field_get(unit, xgh, HG_dst_port);
    }

    return BCM_E_NONE;
#else
    return BCM_E_UNAVAIL;
#endif
}
#endif /* RCPU and XGS3 SUPPORT */

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || \
    defined(BCM_TRIDENT2_SUPPORT)
STATIC int
_bcm_switch_qcn_set(int unit, bcm_switch_control_t type, int arg)
{
    uint32 rval, misc_config;

    if (!soc_feature(unit, soc_feature_qcn)) {
        return BCM_E_UNAVAIL;
    }

    switch (type) {
    case bcmSwitchCongestionCntag:
        BCM_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, ING_QCN_CNTAG_ETHERTYPEr,
                                    REG_PORT_ANY, ENABLEf, arg ? 1 : 0));
        return soc_reg_field32_modify(unit, EGR_QCN_CNTAG_ETHERTYPEr,
                                      REG_PORT_ANY, ENABLEf, arg ? 1 : 0);
    case bcmSwitchCongestionCntagEthertype:
        if (arg < 0 || arg > 0xffff) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, ING_QCN_CNTAG_ETHERTYPEr,
                                    REG_PORT_ANY, ETHERTYPEf, arg));
        return soc_reg_field32_modify(unit, EGR_QCN_CNTAG_ETHERTYPEr,
                                      REG_PORT_ANY, ETHERTYPEf, arg);
    case bcmSwitchCongestionCnm:
        BCM_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, ING_QCN_CNM_ETHERTYPEr,
                                    REG_PORT_ANY, ENABLEf, arg ? 1 : 0));
        return soc_reg_field32_modify(unit, EGR_QCN_CNM_ETHERTYPEr,
                                      REG_PORT_ANY, ENABLEf, arg ? 1 : 0);
    case bcmSwitchCongestionCnmEthertype:
        if (arg < 0 || arg > 0xffff) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, ING_QCN_CNM_ETHERTYPEr,
                                    REG_PORT_ANY, ETHERTYPEf, arg));
        return soc_reg_field32_modify(unit, EGR_QCN_CNM_ETHERTYPEr,
                                      REG_PORT_ANY, ETHERTYPEf, arg);
    case bcmSwitchCongestionNotificationIdHigh:
        if (arg < 0 || arg > 0xffffff) {
            return BCM_E_PARAM;
        }
        return soc_mem_field32_modify(unit, EGR_QCN_CNM_CONTROL_TABLEm,
                                      0, QCN_CPID_PREFIXf, arg);
    case bcmSwitchCongestionNotificationIdQueue:
        return soc_reg_field32_modify(unit, EGR_QCN_CNM_CONTROL_2r,
                                      REG_PORT_ANY, QCN_CNM_CPID_MODEf,
                                      arg ? 0 : 1);
    case bcmSwitchCongestionUseOuterTpid:
        if (arg < -1 || arg > 0xffff) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_2r(unit, &rval));
        if (arg == -1) {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_2r, &rval,
                              QCN_CNM_USE_DEFAULT_OUTER_TPIDf, 0);
        } else {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_2r, &rval,
                              QCN_CNM_USE_DEFAULT_OUTER_TPIDf, 1);
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_2r, &rval,
                              QCN_CNM_DEFAULT_OUTER_TPIDf, arg);
        }
        return WRITE_EGR_QCN_CNM_CONTROL_2r(unit, rval);
    case bcmSwitchCongestionUseOuterVlan:
        if (arg < -1 || arg > 0xfff) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_1r(unit, &rval));
        if (arg == -1) {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_USE_DEFAULT_OUTER_VLAN_IDf, 0);
        } else {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_USE_DEFAULT_OUTER_VLAN_IDf, 1);
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_DEFAULT_OUTER_VLAN_IDf, arg);
        }
        return WRITE_EGR_QCN_CNM_CONTROL_1r(unit, rval);
    case bcmSwitchCongestionUseOuterPktPri:
        if (arg < -1 || arg > 7) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_1r(unit, &rval));
        if (arg == -1) {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_CHANGE_OUTER_DOT1Pf, 0);
        } else {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_CHANGE_OUTER_DOT1Pf, 1);
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_OUTER_DOT1Pf, arg);
        }
        return WRITE_EGR_QCN_CNM_CONTROL_1r(unit, rval);
    case bcmSwitchCongestionUseOuterCfi:
        if (arg < -1 || arg > 1) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_1r(unit, &rval));
        if (arg == -1) {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_CHANGE_OUTER_CFIf, 0);
        } else {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_CHANGE_OUTER_CFIf, 1);
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_OUTER_CFIf, arg);
        }
        return WRITE_EGR_QCN_CNM_CONTROL_1r(unit, rval);
    case bcmSwitchCongestionUseInnerPktPri:
        if (arg < -1 || arg > 7) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_1r(unit, &rval));
        if (arg == -1) {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_CHANGE_INNER_DOT1Pf, 0);
        } else {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_CHANGE_INNER_DOT1Pf, 1);
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_INNER_DOT1Pf, arg);
        }
        return WRITE_EGR_QCN_CNM_CONTROL_1r(unit, rval);
    case bcmSwitchCongestionUseInnerCfi:
        if (arg < -1 || arg > 1) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_1r(unit, &rval));
        if (arg == -1) {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_CHANGE_INNER_CFIf, 0);
        } else {
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_CHANGE_INNER_CFIf, 1);
            soc_reg_field_set(unit, EGR_QCN_CNM_CONTROL_1r, &rval,
                              QCN_CNM_INNER_CFIf, arg);
        }
        return WRITE_EGR_QCN_CNM_CONTROL_1r(unit, rval);
    case bcmSwitchCongestionMissingCntag:
        if (arg < -1 || arg > 1) {
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN(READ_ING_MISC_CONFIGr(unit, &misc_config));
        if (arg == -1) { /* Do not generate CNM */
            soc_reg_field_set(unit, ING_MISC_CONFIGr, &misc_config,
                              QCN_DO_NOT_GENERATE_CNM_IF_NO_CNTAGf, 1);
        } else {
            soc_reg_field_set(unit, ING_MISC_CONFIGr, &misc_config,
                              QCN_DO_NOT_GENERATE_CNM_IF_NO_CNTAGf, 0);
            BCM_IF_ERROR_RETURN(READ_EGR_CONFIG_1r(unit, &rval));
            soc_reg_field_set(unit, EGR_CONFIG_1r, &rval,
                              QCN_SEND_NULL_CNTAG_IF_NO_CNTAGf, arg ? 1 : 0);
            BCM_IF_ERROR_RETURN(WRITE_EGR_CONFIG_1r(unit, rval));
        }
        return WRITE_ING_MISC_CONFIGr(unit, misc_config);
    default:
        break;
    }

    return BCM_E_INTERNAL;
}

STATIC int
_bcm_switch_qcn_get(int unit, bcm_switch_control_t type, int *arg)
{
    uint32 rval;
    egr_qcn_cnm_control_table_entry_t entry;

    if (!soc_feature(unit, soc_feature_qcn)) {
        return BCM_E_UNAVAIL;
    }

    if (arg == NULL) {
        return BCM_E_PARAM;
    }

    switch (type) {
    case bcmSwitchCongestionCntag:
        /* Ingress setting should match with egress setting */
        BCM_IF_ERROR_RETURN(READ_ING_QCN_CNTAG_ETHERTYPEr(unit, &rval));
        *arg = soc_reg_field_get(unit, ING_QCN_CNTAG_ETHERTYPEr, rval,
                                 ENABLEf);
        return BCM_E_NONE;
    case bcmSwitchCongestionCntagEthertype:
        /* Ingress setting should match with egress setting */
        BCM_IF_ERROR_RETURN(READ_ING_QCN_CNTAG_ETHERTYPEr(unit, &rval));
        *arg = soc_reg_field_get(unit, ING_QCN_CNTAG_ETHERTYPEr, rval,
                                 ETHERTYPEf);
        return BCM_E_NONE;
    case bcmSwitchCongestionCnm:
        /* Ingress setting should match with egress setting */
        BCM_IF_ERROR_RETURN(READ_ING_QCN_CNM_ETHERTYPEr(unit, &rval));
        *arg = soc_reg_field_get(unit, ING_QCN_CNM_ETHERTYPEr, rval,
                                 ENABLEf);
        return BCM_E_NONE;
    case bcmSwitchCongestionCnmEthertype:
        /* Ingress setting should match with egress setting */
        BCM_IF_ERROR_RETURN(READ_ING_QCN_CNM_ETHERTYPEr(unit, &rval));
        *arg = soc_reg_field_get(unit, ING_QCN_CNM_ETHERTYPEr, rval,
                                 ETHERTYPEf);
        return BCM_E_NONE;
    case bcmSwitchCongestionNotificationIdHigh:
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, EGR_QCN_CNM_CONTROL_TABLEm,
                                         MEM_BLOCK_ANY, 0, &entry));
        *arg = soc_mem_field32_get(unit, EGR_QCN_CNM_CONTROL_TABLEm,
                                   &entry, QCN_CPID_PREFIXf);
        return BCM_E_NONE;
    case bcmSwitchCongestionNotificationIdQueue:
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_2r(unit, &rval));
        *arg = soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_2r, rval,
                                 QCN_CNM_CPID_MODEf) ? 0 : 1;
        return BCM_E_NONE;
    case bcmSwitchCongestionUseOuterTpid:
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_2r(unit, &rval));
        if (soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_2r, rval,
                              QCN_CNM_USE_DEFAULT_OUTER_TPIDf)) {
            *arg = soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_2r, rval,
                                     QCN_CNM_DEFAULT_OUTER_TPIDf);
        } else {
            *arg = -1;
        }
        return BCM_E_NONE;
    case bcmSwitchCongestionUseOuterVlan:
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_1r(unit, &rval));
        if (soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_1r, rval,
                              QCN_CNM_USE_DEFAULT_OUTER_VLAN_IDf)) {
            *arg = soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_1r, rval,
                                     QCN_CNM_DEFAULT_OUTER_VLAN_IDf);
        } else {
            *arg = -1;
        }
        return BCM_E_NONE;
    case bcmSwitchCongestionUseOuterPktPri:
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_1r(unit, &rval));
        if (soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_1r, rval,
                              QCN_CNM_CHANGE_OUTER_DOT1Pf)) {
            *arg = soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_1r,
                                     rval, QCN_CNM_OUTER_DOT1Pf);
        } else {
            *arg = -1;
        }
        return BCM_E_NONE;
    case bcmSwitchCongestionUseOuterCfi:
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_1r(unit, &rval));
        if (soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_1r, rval,
                              QCN_CNM_CHANGE_OUTER_CFIf)) {
            *arg = soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_1r,
                                     rval, QCN_CNM_OUTER_CFIf);
        } else {
            *arg = -1;
        }
        return BCM_E_NONE;
    case bcmSwitchCongestionUseInnerPktPri:
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_1r(unit, &rval));
        if (soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_1r, rval,
                              QCN_CNM_CHANGE_INNER_DOT1Pf)) {
            *arg = soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_1r,
                                     rval, QCN_CNM_INNER_DOT1Pf);
        } else {
            *arg = -1;
        }
        return BCM_E_NONE;
    case bcmSwitchCongestionUseInnerCfi:
        BCM_IF_ERROR_RETURN(READ_EGR_QCN_CNM_CONTROL_1r(unit, &rval));
        if (soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_1r, rval,
                              QCN_CNM_CHANGE_INNER_CFIf)) {
            *arg = soc_reg_field_get(unit, EGR_QCN_CNM_CONTROL_1r,
                                     rval, QCN_CNM_INNER_CFIf);
        } else {
            *arg = -1;
        }
        return BCM_E_NONE;
    case bcmSwitchCongestionMissingCntag:
        BCM_IF_ERROR_RETURN(READ_ING_MISC_CONFIGr(unit, &rval));
        if (soc_reg_field_get(unit, ING_MISC_CONFIGr, rval,
                              QCN_DO_NOT_GENERATE_CNM_IF_NO_CNTAGf)) {
            *arg = -1;
        } else {
            BCM_IF_ERROR_RETURN(READ_EGR_CONFIG_1r(unit, &rval));
            *arg = soc_reg_field_get(unit, EGR_CONFIG_1r, rval,
                                     QCN_SEND_NULL_CNTAG_IF_NO_CNTAGf) ? 1 : 0;
        }
        return BCM_E_NONE;
    default:
        break;
    }

    return BCM_E_INTERNAL;
}

/*
 * Function:
 *      _bcm_switch_fcoe_ing_ethertype_set
 * Description:
 *      Set the ethertype field(ingress) for FCOE
 * Parameters:
 *      unit        -  unit number
 *      arg         - argument to set as Ethrtype
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_switch_fcoe_ing_ethertype_set(int unit, int arg) 
{
    uint32   rval;

    BCM_IF_ERROR_RETURN(READ_ING_FCOE_ETHERTYPEr(unit, &rval));
    if (arg > 0) {
        /* Enable and Set  FCOE ethertype */
        soc_reg_field_set(unit, ING_FCOE_ETHERTYPEr, &rval, ENABLEf, 1);
        soc_reg_field_set(unit, ING_FCOE_ETHERTYPEr, &rval, ETHERTYPEf, arg);
    } else {
        /* Reset FCOE ethertype */
        soc_reg_field_set(unit, ING_FCOE_ETHERTYPEr, &rval, ENABLEf, 0);
        soc_reg_field_set(unit, ING_FCOE_ETHERTYPEr, &rval, ETHERTYPEf, 0);
    }
    SOC_IF_ERROR_RETURN(WRITE_ING_FCOE_ETHERTYPEr(unit, rval));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_switch_fcoe_ing_ethertype_get
 * Description:
 *      Get the ethertype field(ingress) for FCOE
 * Parameters:
 *      unit        - unit number 
 *      arg         - Ethrtype to get
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_switch_fcoe_ing_ethertype_get(int unit, int *arg) 
{
    uint32   rval;
    int      enable;

    BCM_IF_ERROR_RETURN(READ_ING_FCOE_ETHERTYPEr(unit, &rval));
    /* If ethertype enabled, return ethertype */
    enable = soc_reg_field_get(unit, ING_FCOE_ETHERTYPEr, rval, ENABLEf);
    if (enable) {
        *arg = soc_reg_field_get(unit, ING_FCOE_ETHERTYPEr, rval, ETHERTYPEf);
    } else {
        *arg = 0;
    }
    return BCM_E_NONE;
}

#if defined(BCM_TRIDENT2_SUPPORT)
/*
 * Function:
 *      _bcm_switch_fcoe_egr_ethertype_set
 * Description:
 *      Set the EGR ethertype field for FCOE
 * Parameters:
 *      unit        -  unit number
 *      arg         - argument to set as Ethrtype
 * Returns:
 *      BCM_E_xxxx
 */

STATIC int
_bcm_switch_fcoe_egr_ethertype_set(int unit, int arg) 
{
    uint32   rval;

    BCM_IF_ERROR_RETURN(READ_EGR_FCOE_ETHERTYPEr(unit, &rval));
    if (arg > 0) {
        /* Enable and Set  FCOE EGR ethertype */
        soc_reg_field_set(unit, EGR_FCOE_ETHERTYPEr, &rval, ENABLEf, 1);
        soc_reg_field_set(unit, EGR_FCOE_ETHERTYPEr, &rval, ETHERTYPEf, arg);
    } else {
        /* Reset FCOE EGR ethertype */
        soc_reg_field_set(unit, EGR_FCOE_ETHERTYPEr, &rval, ENABLEf, 0);
        soc_reg_field_set(unit, EGR_FCOE_ETHERTYPEr, &rval, ETHERTYPEf, 0);
    }
    SOC_IF_ERROR_RETURN(WRITE_EGR_FCOE_ETHERTYPEr(unit, rval));
    return BCM_E_NONE;
}
#endif /* defined(BCM_TRIDENT2_SUPPORT) */

#endif /* defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) */

#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || defined(BCM_KATANA_SUPPORT) 
STATIC int
_bcm_switch_sync_port_select_set(int unit, uint32 val)
{
    uint32 rval;
    /* Note: 24, 25 are invalid, 
     *       but we are just setting 0 in those cases 
     */ 
    if (SOC_IS_HURRICANE(unit) || SOC_IS_ENDURO(unit)) {
       static int16 _L1_port_val_map[] = {
           0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
           4, 5, 6, 7, 8, 9, 10, 11, 0, 0, 12, 12, 13, 13
       };
       if (val >= COUNTOF(_L1_port_val_map)) {
          return BCM_E_PARAM;
       }
       BCM_IF_ERROR_RETURN(
            READ_CMIC_MISC_CONTROLr(unit, &rval));
       if ((val <= 3) || ((val >= 8) && (val <= 15)) ||
           (val == 26) || (val == 28)) {
           soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                              L1_CLK0_RECOVERY_MUXf,
                             _L1_port_val_map[val]);
       } else {
           soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                              L1_CLK1_RECOVERY_MUXf,
                             _L1_port_val_map[val]);
       }
       BCM_IF_ERROR_RETURN(
           WRITE_CMIC_MISC_CONTROLr(unit, rval));
    } else if (SOC_IS_KATANA(unit)) {
      BCM_IF_ERROR_RETURN(
           READ_TOP_MISC_CONTROLr(unit, &rval));
      soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                        L1_CLK0_RECOVERY_MUX_SELf,
                        val);
      BCM_IF_ERROR_RETURN(
            WRITE_TOP_MISC_CONTROLr(unit, rval));
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_switch_sync_port_backup_select_set(int unit, uint32 val)
{
    uint32 rval;
    if (SOC_IS_KATANA(unit)) {
       BCM_IF_ERROR_RETURN(
           READ_TOP_MISC_CONTROLr(unit, &rval));
       soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                         L1_CLK1_RECOVERY_MUX_SELf,
                         val);
       BCM_IF_ERROR_RETURN(
            WRITE_TOP_MISC_CONTROLr(unit, rval));
    }
    return BCM_E_NONE;
}


STATIC int
_bcm_switch_sync_port_select_get(int unit, uint8 type, uint32 *val)
{
    uint32 rval, port;
    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit)) {
        static int16 _L1_val_port_map0[] = {
            0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15, 26, 28
        };
        static int16 _L1_val_port_map1[] = {
            4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 27, 29
        };
        READ_CMIC_MISC_CONTROLr(unit, &rval);
        if (type) { /* primary (1) */
            port = soc_reg_field_get(unit, CMIC_MISC_CONTROLr, rval,
                                     L1_CLK0_RECOVERY_MUXf);
           *val = _L1_val_port_map0[port];
        } else { /* backup (0) */
           port = soc_reg_field_get(unit, CMIC_MISC_CONTROLr, rval,
                                    L1_CLK1_RECOVERY_MUXf);
          *val = _L1_val_port_map1[port];
        }
    } else if (SOC_IS_KATANA(unit)) {
      BCM_IF_ERROR_RETURN(
           READ_TOP_MISC_CONTROLr(unit, &rval));
      if (type) { /* primary (1) */
         *val = soc_reg_field_get(unit, TOP_MISC_CONTROLr, rval,
                                  L1_CLK0_RECOVERY_MUX_SELf);
      } else { /* backup (0) */
         *val = soc_reg_field_get(unit, TOP_MISC_CONTROLr, rval,
                                     L1_CLK1_RECOVERY_MUX_SELf);
      }
    }
    return BCM_E_NONE;
}

STATIC int
 _bcm_switch_div_ctrl_select_set(int unit, uint32 val)
{
    uint32 rval;
    if (SOC_IS_HURRICANE(unit) || SOC_IS_ENDURO(unit)) {
       if (val == 1) {
          BCM_IF_ERROR_RETURN(
              READ_CMIC_MISC_CONTROLr(unit, &rval));
              soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                                 L1_CLK0_RECOVERY_DIV_CTRLf,
                                 0x0);
          BCM_IF_ERROR_RETURN(
              WRITE_CMIC_MISC_CONTROLr(unit, rval));

          BCM_IF_ERROR_RETURN(
              READ_CMIC_MISC_CONTROLr(unit, &rval));
              soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                                L1_CLK0_RECOVERY_DIV_CTRLf,
                                0x2);
          BCM_IF_ERROR_RETURN(
              WRITE_CMIC_MISC_CONTROLr(unit, rval));   

          BCM_IF_ERROR_RETURN(
              READ_CMIC_MISC_CONTROLr(unit, &rval));
              soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                                L1_CLK0_RECOVERY_DIV_CTRLf,
                                0x3);
          BCM_IF_ERROR_RETURN(
              WRITE_CMIC_MISC_CONTROLr(unit, rval));
        } else {
           BCM_IF_ERROR_RETURN(
               READ_CMIC_MISC_CONTROLr(unit, &rval));
               soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                                 L1_CLK0_RECOVERY_DIV_CTRLf,
                                 0x0);
           BCM_IF_ERROR_RETURN(
               WRITE_CMIC_MISC_CONTROLr(unit, rval));
        } 
    }  else if (SOC_IS_KATANA(unit)) {
         if (val == 1) {
            BCM_IF_ERROR_RETURN(
               READ_TOP_MISC_CONTROLr(unit, &rval));
               soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                                 L1_CLK0_RECOVERY_DIV_CTRLf,
                                 0x0);
            BCM_IF_ERROR_RETURN(
               WRITE_TOP_MISC_CONTROLr(unit, rval));

            BCM_IF_ERROR_RETURN(
               READ_TOP_MISC_CONTROLr(unit, &rval));
               soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                                 L1_CLK0_RECOVERY_DIV_CTRLf,
                                 0x2);
            BCM_IF_ERROR_RETURN(
               WRITE_TOP_MISC_CONTROLr(unit, rval));

            BCM_IF_ERROR_RETURN(
               READ_TOP_MISC_CONTROLr(unit, &rval));
               soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                                 L1_CLK0_RECOVERY_DIV_CTRLf,
                                 0x3);
            BCM_IF_ERROR_RETURN(
               WRITE_TOP_MISC_CONTROLr(unit, rval));
         } else {
            BCM_IF_ERROR_RETURN(
               READ_TOP_MISC_CONTROLr(unit, &rval));
               soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                                 L1_CLK0_RECOVERY_DIV_CTRLf,
                                 0x0);
            BCM_IF_ERROR_RETURN(
               WRITE_TOP_MISC_CONTROLr(unit, rval));
         }
    } else {
         return BCM_E_UNAVAIL;
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_switch_div_ctrl_backup_select_set(int unit, uint32 val)
{

    uint32 rval;
    if (SOC_IS_HURRICANE(unit) || SOC_IS_ENDURO(unit)) {
       if (val == 1) {
          BCM_IF_ERROR_RETURN(
              READ_CMIC_MISC_CONTROLr(unit, &rval));
              soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                                 L1_CLK1_RECOVERY_DIV_CTRLf,
                                 0x0);
          BCM_IF_ERROR_RETURN(
              WRITE_CMIC_MISC_CONTROLr(unit, rval));

          BCM_IF_ERROR_RETURN(
              READ_CMIC_MISC_CONTROLr(unit, &rval));
              soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                                L1_CLK1_RECOVERY_DIV_CTRLf,
                               0x2);
          BCM_IF_ERROR_RETURN(
              WRITE_CMIC_MISC_CONTROLr(unit, rval));

          BCM_IF_ERROR_RETURN(
              READ_CMIC_MISC_CONTROLr(unit, &rval));
              soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                                L1_CLK1_RECOVERY_DIV_CTRLf,
                                0x3);
          BCM_IF_ERROR_RETURN(
              WRITE_CMIC_MISC_CONTROLr(unit, rval));
        } else {
          BCM_IF_ERROR_RETURN(
              READ_CMIC_MISC_CONTROLr(unit, &rval));
              soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                                L1_CLK1_RECOVERY_DIV_CTRLf,
                                0x0);
          BCM_IF_ERROR_RETURN(
              WRITE_CMIC_MISC_CONTROLr(unit, rval));
        }
    }  else if (SOC_IS_KATANA(unit)) {
         if (val == 1) {
            BCM_IF_ERROR_RETURN(
               READ_TOP_MISC_CONTROLr(unit, &rval));
               soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                                 L1_CLK1_RECOVERY_DIV_CTRLf,
                                 0x0);
            BCM_IF_ERROR_RETURN(
                WRITE_TOP_MISC_CONTROLr(unit, rval));

            BCM_IF_ERROR_RETURN(
                READ_TOP_MISC_CONTROLr(unit, &rval));
                soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                                  L1_CLK1_RECOVERY_DIV_CTRLf,
                                  0x2);
            BCM_IF_ERROR_RETURN(
                WRITE_TOP_MISC_CONTROLr(unit, rval));

            BCM_IF_ERROR_RETURN(
                READ_TOP_MISC_CONTROLr(unit, &rval));
                soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                                  L1_CLK1_RECOVERY_DIV_CTRLf,
                                  0x3);
            BCM_IF_ERROR_RETURN(
                WRITE_TOP_MISC_CONTROLr(unit, rval));
         } else {
            BCM_IF_ERROR_RETURN(
               READ_TOP_MISC_CONTROLr(unit, &rval));
               soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                                 L1_CLK1_RECOVERY_DIV_CTRLf,
                                 0x0);
            BCM_IF_ERROR_RETURN(
               WRITE_TOP_MISC_CONTROLr(unit, rval));
         }
    } else {
         return BCM_E_UNAVAIL;
    }

    return BCM_E_NONE;
}
STATIC int
_bcm_switch_div_ctrl_select_get(int unit, uint8 type, uint32 *val)
{
    uint32 rval;
    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit)) {
        READ_CMIC_MISC_CONTROLr(unit, &rval);
        if (type) { /* primary (1) */
            *val = soc_reg_field_get(unit, CMIC_MISC_CONTROLr, rval,
                                     L1_CLK0_RECOVERY_DIV_CTRLf);
           
        } else { /* backup (0) */
           *val = soc_reg_field_get(unit, CMIC_MISC_CONTROLr, rval,
                                    L1_CLK1_RECOVERY_DIV_CTRLf);
        }
    } else if (SOC_IS_KATANA(unit)) {
        BCM_IF_ERROR_RETURN(
             READ_TOP_MISC_CONTROLr(unit, &rval));
        if (type) { /* primary (1) */
           *val = soc_reg_field_get(unit, TOP_MISC_CONTROLr, rval,
                                  L1_CLK0_RECOVERY_DIV_CTRLf);
        } else { /* backup (0) */
           *val = soc_reg_field_get(unit, TOP_MISC_CONTROLr, rval,
                                     L1_CLK1_RECOVERY_DIV_CTRLf);
        }
    } else {
        return BCM_E_UNAVAIL;
    }
    return BCM_E_NONE;
}
#endif

/*
 * Function:
 *      bcm_esw_switch_control_port_proxy_validate
 * Description:
 *      Checks that switch control is valid for a PROXY GPORT.
 *      Resolves port to expected port value for the given port control type.
 * Parameters:
 *      unit      - (IN) Device number
 *      port      - (IN) Port to resolve
 *      type      - (IN) Switch control type
 *      port_out  - (OUT) Returns port (gport or BCM format) that
 *                  is expected for corresponding switch control type.
 * Return Value:
 *      BCM_E_XXX
 */
STATIC int
bcm_esw_switch_control_port_proxy_validate(int unit, bcm_port_t port,
                                           bcm_switch_control_t type,
                                           bcm_port_t *port_out)
{
    int rv = BCM_E_PORT;
    int support = FALSE;

    if (!soc_feature(unit, soc_feature_proxy_port_property)) {
        return BCM_E_PORT;
    }

    switch(type) {
    case bcmSwitchModuleLoopback:
        support = TRUE;
        break;
    default:
        support = FALSE;
        break;
    }

    if (support) {
        *port_out = port;
        rv = BCM_E_NONE;
    }

    return rv;
}

/*
 * Function:
 *      bcm_esw_switch_control_port_validate
 * Description:
 *      Checks that port is valid and that control is supported
 *      for the given port type.  Resolves port to expected
 *      port value for the given switch control type.
 *      Current valid port types are:
 *          GPORT PROXY
 *          GPORT (various types) that resolves to local physical port
 *          BCM port (non-gport)
 * Parameters:
 *      unit      - (IN) Device number
 *      port      - (IN) Port to resolve
 *      type      - (IN) Switch control type
 *      port_out  - (OUT) Returns port (gport or BCM format) that
 *                  is expected for corresponding switch control type.
 * Return Value:
 *      BCM_E_XXX
 */
STATIC int
bcm_esw_switch_control_port_validate(int unit, bcm_port_t port,
                                     bcm_switch_control_t type,
                                     bcm_port_t *port_out)
{
    int rv = BCM_E_PORT;

    if (BCM_GPORT_IS_PROXY(port)) {
        rv = bcm_esw_switch_control_port_proxy_validate(unit, port,
                                                        type, port_out);
    } else {
        /* All other types are supported for valid physical local ports */
        rv = _bcm_esw_switch_control_gport_resolve(unit, port, port_out);
    }

    return rv;
}

/*
 * Function:
 *      bcm_switch_control_port_set
 * Description:
 *      Specify general switch behaviors on a per-port basis.
 * Parameters:
 *      unit - Device unit number
 *      port - Port to affect
 *      type - The desired configuration parameter to modify
 *      arg - The value with which to set the parameter
 * Returns:
 *      BCM_E_xxx
 */

int
bcm_esw_switch_control_port_set(int unit,
                                bcm_port_t port,
                                bcm_switch_control_t type,
                                int arg)
{
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        return BCM_E_UNAVAIL;
    }
#endif /* BCM_SHADOW_SUPPORT */
    if ((type == bcmSwitchFailoverStackTrunk) ||
        (type == bcmSwitchFailoverEtherTrunk)) {
        return BCM_E_UNAVAIL;
    }

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_switch_control_port_validate(unit, port, type, &port));
    }

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        uint64 oval64, val64;
        int i, found;

        switch(type) {
        case bcmSwitchPktAge:
#ifdef BCM_KATANA_SUPPORT
            if (SOC_IS_KATANAX(unit)) {
                return _bcm_kt_pkt_age_set(unit, arg);
            } else
#endif
#ifdef BCM_TRX_SUPPORT
            if (SOC_IS_TRX(unit) && !SOC_IS_HURRICANEX(unit)) {
                return _bcm_ts_pkt_age_set(unit, arg);
            } else
#endif
            if (SOC_IS_FBX(unit)) {
                return _bcm_fb_pkt_age_set(unit, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashL2:
        case bcmSwitchHashL3:
        case bcmSwitchHashMultipath:
        case bcmSwitchHashIpfixIngress:
        case bcmSwitchHashIpfixEgress:
        case bcmSwitchHashIpfixIngressDual:
        case bcmSwitchHashIpfixEgressDual:
        case bcmSwitchHashVlanTranslate:
        case bcmSwitchHashMPLS:
#if defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchHashWlanPort:
        case bcmSwitchHashWlanPortDual:
        case bcmSwitchHashWlanClient:
        case bcmSwitchHashWlanClientDual:
        case bcmSwitchHashRegexAction:
        case bcmSwitchHashRegexActionDual:
#endif
#ifdef BCM_TRIDENT2_SUPPORT
        case bcmSwitchHashL3DNATPool:
        case bcmSwitchHashL3DNATPoolDual:
        case bcmSwitchHashVpVlanMemberIngress:
        case bcmSwitchHashVpVlanMemberIngressDual:
        case bcmSwitchHashVpVlanMemberEgress:
        case bcmSwitchHashVpVlanMemberEgressDual:
        case bcmSwitchHashL2Endpoint:
        case bcmSwitchHashL2EndpointDual:
        case bcmSwitchHashEndpointQueueMap:
        case bcmSwitchHashEndpointQueueMapDual:
#endif
            return _bcm_fb_er_hashselect_set(unit, type, arg);
            break;
        case bcmSwitchHashControl:
            return _bcm_xgs3_hashcontrol_set(unit, arg);
            break;
#if defined(BCM_TRIUMPH_SUPPORT)
        case bcmSwitchFieldMultipathHashSelect:
            return _bcm_field_tr_multipath_hashcontrol_set(unit, arg);
            break;
        case bcmSwitchL3UrpfRouteEnableExternal:
#if defined(BCM_ENDURO_SUPPORT)
            if(SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit)) {
                return BCM_E_UNAVAIL;
            }
#endif /* BCM_ENDURO_SUPPORT */
            if ((soc_feature(unit, soc_feature_urpf)) && (soc_feature(unit, soc_feature_esm_support))) {
                 return _bcm_xgs3_urpf_route_enable_external(unit, arg);
            }
            return BCM_E_UNAVAIL;       
            break;
        case bcmSwitchL3IngressMode:
#ifdef INCLUDE_L3
            if ((soc_feature(unit, soc_feature_l3)) && (soc_feature(unit,soc_feature_l3_ingress_interface))) {
                 return bcm_xgs3_l3_ingress_mode_set(unit, arg);
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchL3IngressInterfaceMapSet:
#ifdef INCLUDE_L3
            if ((soc_feature(unit, soc_feature_l3)) && (soc_feature(unit,soc_feature_l3_ingress_interface))) {
                 return bcm_xgs3_l3_ingress_intf_map_set(unit, arg);
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;

#endif  /* BCM_TRIUMPH_SUPPORT */
        case bcmSwitchColorSelect:
            return _bcm_fb_er_color_mode_set(unit, port, arg);
            break;
        case bcmSwitchModuleLoopback:
            return _bcm_fb_mod_lb_set(unit, port, arg);
            break;
#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) \
            || defined(BCM_SCORPION_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        case bcmSwitchIngressRateLimitIncludeIFG:
            return _bcm_xgs3_ing_rate_limit_ifg_set(unit, port, arg);
            break;
#endif /* BCM_RAPTOR_SUPPORT || BCM_FIREBOLT2_SUPPORT || BCM_HURRICANE_SUPPORT || BCM_SCORPION_SUPPORT */
        case bcmSwitchVrfMax:
#if defined(BCM_HAWKEYE_SUPPORT) || defined(BCM_HURRICANE_SUPPORT)
            if(SOC_IS_HAWKEYE(unit) || SOC_IS_HURRICANEX(unit)) {
                return BCM_E_UNAVAIL;
            }
#endif /* BCM_HAWKEYE_SUPPORT || BCM_HURRICANE_SUPPORT */
            return soc_max_vrf_set(unit, arg);
            break;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
        case bcmSwitchL3UrpfRouteEnable:
            if (soc_feature(unit, soc_feature_urpf)) {
                return _bcm_xgs3_urpf_route_enable(unit, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchL3UrpfMode:
            if (soc_feature(unit, soc_feature_urpf)) {
                return _bcm_xgs3_urpf_port_mode_set(unit, port, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchL3UrpfDefaultRoute:
            if (soc_feature(unit, soc_feature_urpf)) {
                return _bcm_xgs3_urpf_def_gw_enable(unit, port, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
        defined(BCM_TRX_SUPPORT)
        case bcmSwitchHashL2Dual:
        case bcmSwitchHashL3Dual:
        case bcmSwitchHashVlanTranslateDual:
        case bcmSwitchHashMPLSDual:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                return _bcm_fb_er_hashselect_set(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */
#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRIUMPH_SUPPORT)
        case bcmSwitchHashSelectControl:
            if (SOC_IS_KATANA2(unit)) {
                return _bcm_kt2_selectcontrol_set(unit,arg);
            } else if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit)) {
                return _bcm_xgs3_selectcontrol_set(unit, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashIP4Field0:
        case bcmSwitchHashIP4Field1:
        case bcmSwitchHashIP6Field0:
        case bcmSwitchHashIP6Field1:
        case bcmSwitchHashL2Field0:
        case bcmSwitchHashL2Field1:
        case bcmSwitchHashMPLSField0:
        case bcmSwitchHashMPLSField1:
        case bcmSwitchHashHG2UnknownField0:
        case bcmSwitchHashHG2UnknownField1:
            if ((type == bcmSwitchHashMPLSField0 || type == bcmSwitchHashMPLSField1)
                && (SOC_IS_ENDURO(unit) || SOC_IS_TRIUMPH2(unit) ||
                    SOC_IS_APOLLO(unit) || SOC_IS_VALKYRIE2(unit) ||
                    SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
                    SOC_IS_KATANAX(unit))) {
                return BCM_E_UNAVAIL;
            }
            if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit)) {
                return _bcm_xgs3_fieldselect_set(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#ifdef BCM_TRIUMPH2_SUPPORT
        case bcmSwitchHashL2MPLSField0:
        case bcmSwitchHashL2MPLSField1:
        case bcmSwitchHashL3MPLSField0:
        case bcmSwitchHashL3MPLSField1:
        case bcmSwitchHashMPLSTunnelField0:
        case bcmSwitchHashMPLSTunnelField1:
        case bcmSwitchHashMIMTunnelField0:
        case bcmSwitchHashMIMTunnelField1:
        case bcmSwitchHashMIMField0:
        case bcmSwitchHashMIMField1:
            if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
                SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
                SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
                SOC_IS_KATANAX(unit)) {
                return _bcm_xgs3_fieldselect_set(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif
        case bcmSwitchHashL2VxlanField0:
        case bcmSwitchHashL2VxlanField1:
        case bcmSwitchHashL3VxlanField0:
        case bcmSwitchHashL3VxlanField1:
#if defined(BCM_TRIDENT2_SUPPORT)
           if (SOC_IS_TD2_TT2(unit) && 
               soc_feature(unit, soc_feature_vxlan)) {
                return _bcm_xgs3_fieldselect_set(unit, type, arg);
            }
#endif
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchHashL2L2GreField0:
        case bcmSwitchHashL2L2GreField1:
        case bcmSwitchHashL3L2GreField0:
        case bcmSwitchHashL3L2GreField1:
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
           if ((SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit)) && 
               soc_feature(unit, soc_feature_l2gre)) {
                return _bcm_xgs3_fieldselect_set(unit, type, arg);
            }
#endif
            return BCM_E_UNAVAIL;
            break;

#ifdef BCM_TRIDENT_SUPPORT
        case bcmSwitchHashFCOEField0:
        case bcmSwitchHashFCOEField1:
        case bcmSwitchHashL2TrillField0:
        case bcmSwitchHashL2TrillField1:
        case bcmSwitchHashL3TrillField0:
        case bcmSwitchHashL3TrillField1:
        case bcmSwitchHashTrillTunnelField0:
        case bcmSwitchHashTrillTunnelField1:
           if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) {
                return _bcm_xgs3_fieldselect_set(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;

 
        case bcmSwitchHashIP4TcpUdpField0:
        case bcmSwitchHashIP4TcpUdpField1:
        case bcmSwitchHashIP4TcpUdpPortsEqualField0:
        case bcmSwitchHashIP4TcpUdpPortsEqualField1:
        case bcmSwitchHashIP6TcpUdpField0:
        case bcmSwitchHashIP6TcpUdpField1:
        case bcmSwitchHashIP6TcpUdpPortsEqualField0:
        case bcmSwitchHashIP6TcpUdpPortsEqualField1:
            if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) || 
                SOC_IS_KATANAX(unit)) {
                return _bcm_xgs3_fieldselect_set(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashField0PreProcessEnable:
            if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
                return (soc_reg_field32_modify(unit, RTAG7_HASH_CONTROL_3r,
                            REG_PORT_ANY, HASH_PRE_PROCESSING_ENABLE_Af, arg));
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashField1PreProcessEnable:
            if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
                return (soc_reg_field32_modify(unit, RTAG7_HASH_CONTROL_3r,
                            REG_PORT_ANY, HASH_PRE_PROCESSING_ENABLE_Bf, arg));
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_TRIDENT_SUPPORT */

        case bcmSwitchHashField0Config:
        case bcmSwitchHashField0Config1:
        case bcmSwitchHashField1Config:
        case bcmSwitchHashField1Config1:
        case bcmSwitchMacroFlowHashFieldConfig:
            if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit) ||
                SOC_IS_TD_TT(unit) || SOC_IS_TR_VL(unit) ) {
                return _bcm_xgs3_fieldconfig_set(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchTrunkHashSet1UnicastOffset:
        case bcmSwitchTrunkHashSet1NonUnicastOffset:
        case bcmSwitchFabricTrunkHashSet1UnicastOffset:
        case bcmSwitchFabricTrunkHashSet1NonUnicastOffset:
        case bcmSwitchLoadBalanceHashSet1UnicastOffset:
        case bcmSwitchLoadBalanceHashSet1NonUnicastOffset:
        case bcmSwitchECMPHashSet1Offset:
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
             if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
                 /* Previous devices had two hash config sets(Set0 & Set1), 
                  * which each port can select. 
                  * In Triumph3 each port can be configured with its
                  * own hash configs. 
                  */ 
                 return BCM_E_UNAVAIL;
             } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
            if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit) ||
                SOC_IS_TD_TT(unit) || SOC_IS_TR_VL(unit)) {
                return _bcm_xgs3_fieldoffset_set(unit, port, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchTrunkHashSet0UnicastOffset:
        case bcmSwitchTrunkHashSet0NonUnicastOffset:
        case bcmSwitchTrunkFailoverHashOffset:
        case bcmSwitchFabricTrunkHashSet0UnicastOffset:
        case bcmSwitchFabricTrunkHashSet0NonUnicastOffset:
        case bcmSwitchLoadBalanceHashSet0UnicastOffset:
        case bcmSwitchLoadBalanceHashSet0NonUnicastOffset:
        case bcmSwitchFabricTrunkFailoverHashOffset:
        case bcmSwitchFabricTrunkDynamicHashOffset:
        case bcmSwitchTrunkDynamicHashOffset:
        case bcmSwitchEcmpDynamicHashOffset:
        case bcmSwitchFabricTrunkResilientHashOffset:
        case bcmSwitchTrunkResilientHashOffset:
        case bcmSwitchEcmpResilientHashOffset:
        case bcmSwitchECMPHashSet0Offset:
        case bcmSwitchECMPVxlanHashOffset:
        case bcmSwitchECMPL2GreHashOffset:
        case bcmSwitchECMPTrillHashOffset:
        case bcmSwitchECMPMplsHashOffset:
        case bcmSwitchVirtualPortDynamicHashOffset:
            if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit) ||
                SOC_IS_TD_TT(unit) || SOC_IS_TR_VL(unit)) {
                return _bcm_xgs3_fieldoffset_set(unit, port, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_BRADLEY_SUPPORT || BCM_TRIUMPH_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
        case bcmSwitchArpReplyToCpu:
        case bcmSwitchArpReplyDrop:
        case bcmSwitchArpRequestToCpu:
        case bcmSwitchArpRequestDrop:
        case bcmSwitchNdPktToCpu:
        case bcmSwitchNdPktDrop:
        case bcmSwitchDhcpPktToCpu:
        case bcmSwitchDhcpPktDrop:
            if (SOC_REG_INFO(unit, PROTOCOL_PKT_CONTROLr).regtype !=
                soc_portreg) {
                return _bcm_tr2_prot_pkt_action_set(unit, port, type, arg);
            }
            break;
#endif /* BCM_TRIUMPH2_SUPPORT */
        case bcmSwitchIgmpPktToCpu:
        case bcmSwitchIgmpPktDrop:
        case bcmSwitchMldPktToCpu:
        case bcmSwitchMldPktDrop:
        case bcmSwitchV4ResvdMcPktToCpu:
        case bcmSwitchV4ResvdMcPktFlood:
        case bcmSwitchV4ResvdMcPktDrop:
        case bcmSwitchV6ResvdMcPktToCpu:
        case bcmSwitchV6ResvdMcPktFlood:
        case bcmSwitchV6ResvdMcPktDrop:
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
            defined (BCM_TRX_SUPPORT)
        case bcmSwitchIgmpReportLeaveDrop:
        case bcmSwitchIgmpReportLeaveFlood:
        case bcmSwitchIgmpReportLeaveToCpu:
        case bcmSwitchIgmpQueryDrop:
        case bcmSwitchIgmpQueryFlood:
        case bcmSwitchIgmpQueryToCpu:
        case bcmSwitchIgmpUnknownDrop:
        case bcmSwitchIgmpUnknownFlood:
        case bcmSwitchIgmpUnknownToCpu:
        case bcmSwitchMldReportDoneDrop:
        case bcmSwitchMldReportDoneFlood:
        case bcmSwitchMldReportDoneToCpu:
        case bcmSwitchMldQueryDrop:
        case bcmSwitchMldQueryFlood:
        case bcmSwitchMldQueryToCpu:
        case bcmSwitchIpmcV4RouterDiscoveryDrop:
        case bcmSwitchIpmcV4RouterDiscoveryFlood:
        case bcmSwitchIpmcV4RouterDiscoveryToCpu:
        case bcmSwitchIpmcV6RouterDiscoveryDrop:
        case bcmSwitchIpmcV6RouterDiscoveryFlood:
        case bcmSwitchIpmcV6RouterDiscoveryToCpu:
#endif /* defined(BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) ||
          defined(BCM_TRX_SUPPORT) */
            return _bcm_xgs3_igmp_action_set(unit, port, type, arg);
            break;

        case bcmSwitchDirectedMirroring:
            if (!soc_feature(unit, soc_feature_xgs1_mirror)) {
                /* Directed mirroring cannot be turned off */
                return (arg == 0) ? BCM_E_PARAM : BCM_E_NONE;
            }
            break;

        case bcmSwitchFlexibleMirrorDestinations:
            return _bcm_esw_mirror_flexible_set(unit, arg);

        case bcmSwitchL3EgressMode:
#ifdef INCLUDE_L3
            if (soc_feature(unit, soc_feature_l3)) {
                return bcm_xgs3_l3_egress_mode_set(unit, arg);
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;

         case bcmSwitchL3HostAsRouteReturnValue:
#ifdef INCLUDE_L3
            if (soc_feature(unit, soc_feature_l3)) {
                 return bcm_xgs3_l3_host_as_route_return_set(unit, arg);
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchL3DefipMultipathCountUpdate:
#ifdef INCLUDE_L3
            if(soc_feature(unit, soc_feature_l3_defip_ecmp_count)) {
                return _bcm_xgs3_l3_multipathCountUpdate(unit, arg);
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchIpmcReplicationSharing:
#ifdef INCLUDE_L3
            if (soc_feature(unit, soc_feature_ip_mcast_repl)) {
                SOC_IPMCREPLSHR_SET(unit, arg);
#ifdef BCM_WARM_BOOT_SUPPORT
                {
                    int rv;
                    /* Record this value in HW for Warm Boot recovery. */
                    rv = _bcm_esw_ipmc_repl_wb_flags_set(unit,
                                  (arg ? _BCM_IPMC_WB_REPL_LIST : 0),
                                  _BCM_IPMC_WB_REPL_LIST);
                    if (BCM_FAILURE(rv) && (BCM_E_UNAVAIL != rv)) {
                        return rv;
                    }
                }
#endif /* BCM_WARM_BOOT_SUPPORT */
                return BCM_E_NONE;
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchIpmcReplicationAvailabilityThreshold:
#ifdef INCLUDE_L3
            if (soc_feature(unit, soc_feature_ip_mcast_repl)) {
                return _bcm_esw_ipmc_repl_threshold_set(unit, arg);
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepth:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthL2:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_L2X(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthMpls:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_MPLS(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthVlan:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_VLAN(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthEgressVlan:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_EGRESS_VLAN(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
#if defined(INCLUDE_L3)
        case bcmSwitchHashDualMoveDepthL3:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_L3X(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchHashDualMoveDepthWlanPort:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_WLAN_PORT(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthWlanClient:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_WLAN_CLIENT(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
#endif
        case bcmSwitchL2PortBlocking:
            if (soc_feature(unit, soc_feature_src_mac_group)) {
                SOC_L2X_GROUP_ENABLE_SET(unit, _bool_invert(unit, arg, 0));
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
#if defined(BCM_HAWKEYE_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT)
        case bcmSwitchTimeSyncPktDrop:
        case bcmSwitchTimeSyncPktFlood:
        case bcmSwitchTimeSyncPktToCpu:
        case bcmSwitchMmrpPktDrop:
        case bcmSwitchMmrpPktFlood:
        case bcmSwitchMmrpPktToCpu:
        case bcmSwitchSrpPktDrop:
        case bcmSwitchSrpPktFlood:
        case bcmSwitchSrpPktToCpu:
            return _bcm_xgs3_eav_action_set(unit, port, type, arg);
            break;
        case bcmSwitchSRPEthertype:
        case bcmSwitchMMRPEthertype:
        case bcmSwitchTimeSyncEthertype: 
            return _bcm_xgs3_switch_ethertype_set(unit, port, type, arg); 
            break;
        case bcmSwitchSRPDestMacOui:
        case bcmSwitchMMRPDestMacOui:
        case bcmSwitchTimeSyncDestMacOui:
            return _bcm_xgs3_switch_mac_hi_set(unit, port, type, arg); 
            break;
        case bcmSwitchSRPDestMacNonOui:
        case bcmSwitchMMRPDestMacNonOui:
        case bcmSwitchTimeSyncDestMacNonOui:
            return _bcm_xgs3_switch_mac_lo_set(unit, port, type, arg); 
            break;
        case bcmSwitchTimeSyncMessageTypeBitmap:
            if (soc_reg_field_valid(unit, TS_CONTROLr, TS_MSG_BITMAPf)) {
                return soc_reg_field32_modify(unit, TS_CONTROLr, port, 
                                              TS_MSG_BITMAPf, arg);
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchTimeSyncClassAPktPrio:
        case bcmSwitchTimeSyncClassBPktPrio:
        case bcmSwitchTimeSyncClassAExeptionPktPrio:
        case bcmSwitchTimeSyncClassBExeptionPktPrio:
            return _bcm_switch_class_pkt_prio_set(unit, type, arg);
            break;

#endif /* defined(BCM_HAWKEYE_SUPPORT)*/
#if defined(BCM_TRIUMPH2_SUPPORT)
        case bcmSwitchUnknownVlanToCpu:
        case bcmSwitchL3HeaderErrToCpu:
        case bcmSwitchIpmcTtlErrToCpu:
        case bcmSwitchHgHdrErrToCpu:
        case bcmSwitchTunnelErrToCpu:
            if (SOC_REG_IS_VALID(unit, EGR_CPU_CONTROLr) &&
                    soc_feature(unit, soc_feature_internal_loopback)) {
                BCM_IF_ERROR_RETURN(
                    _bcm_tr2_ep_redirect_action_set(unit, port, type, arg));
            }
            break;
        case bcmSwitchStgInvalidToCpu:
        case bcmSwitchVlanTranslateEgressMissToCpu:
        case bcmSwitchL3PktErrToCpu:
        case bcmSwitchMtuFailureToCpu:
        case bcmSwitchSrcKnockoutToCpu:
        case bcmSwitchWlanTunnelMismatchToCpu:
        case bcmSwitchWlanTunnelMismatchDrop:
        case bcmSwitchWlanPortMissToCpu:
            return _bcm_tr2_ep_redirect_action_set(unit, port, type, arg);
            break;
        case bcmSwitchUnknownVlanToCpuCosq:
        case bcmSwitchStgInvalidToCpuCosq:
        case bcmSwitchVlanTranslateEgressMissToCpuCosq:
        case bcmSwitchTunnelErrToCpuCosq:
        case bcmSwitchL3HeaderErrToCpuCosq:
        case bcmSwitchL3PktErrToCpuCosq:
        case bcmSwitchIpmcTtlErrToCpuCosq:
        case bcmSwitchMtuFailureToCpuCosq:
        case bcmSwitchHgHdrErrToCpuCosq:
        case bcmSwitchSrcKnockoutToCpuCosq:
        case bcmSwitchWlanTunnelMismatchToCpuCosq:
        case bcmSwitchWlanPortMissToCpuCosq:
            return _bcm_tr2_ep_redirect_action_cosq_set(unit, port, type, arg);
            break;
        case bcmSwitchLayeredQoSResolution:
            return _bcm_tr2_layered_qos_resolution_set(unit, port, type, arg);
            break;
        case bcmSwitchEncapErrorToCpu:
            return _bcm_tr2_ehg_error2cpu_set(unit, port, arg);
            break;
        case bcmSwitchMirrorEgressTrueColorSelect:
        case bcmSwitchMirrorEgressTruePriority:
            return _bcm_tr2_mirror_egress_true_set(unit, port, type, arg);
            break;
#endif /* BCM_TRIUMPH2_SUPPORT */
        case bcmSwitchLinkDownInfoSkip:
            return _bcm_esw_link_port_info_skip_set(unit, port, arg); 
            break;

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchCongestionNotificationIdLow:
            if (!soc_feature(unit, soc_feature_qcn)) {
                return BCM_E_UNAVAIL;
            }
            if (arg < 0 || arg > 0xffff) {
                return BCM_E_PARAM;
            }
            return soc_mem_field32_modify(unit, EGR_PORTm, port,
                                          QCN_CNM_CPID_PORT_PREFIXf, arg);
            break;
#endif /* defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) */

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || \
    defined(BCM_TRIDENT2_SUPPORT)
        case bcmSwitchPFCClass0Queue:
        case bcmSwitchPFCClass1Queue:
        case bcmSwitchPFCClass2Queue:
        case bcmSwitchPFCClass3Queue:
        case bcmSwitchPFCClass4Queue:
        case bcmSwitchPFCClass5Queue:
        case bcmSwitchPFCClass6Queue:
        case bcmSwitchPFCClass7Queue:
        case bcmSwitchPFCClass0McastQueue:
        case bcmSwitchPFCClass1McastQueue:
        case bcmSwitchPFCClass2McastQueue:
        case bcmSwitchPFCClass3McastQueue:
        case bcmSwitchPFCClass4McastQueue:
        case bcmSwitchPFCClass5McastQueue:
        case bcmSwitchPFCClass6McastQueue:
        case bcmSwitchPFCClass7McastQueue:
        case bcmSwitchPFCClass0DestmodQueue:
        case bcmSwitchPFCClass1DestmodQueue:
        case bcmSwitchPFCClass2DestmodQueue:
        case bcmSwitchPFCClass3DestmodQueue:
        case bcmSwitchPFCClass4DestmodQueue:
        case bcmSwitchPFCClass5DestmodQueue:
        case bcmSwitchPFCClass6DestmodQueue:
        case bcmSwitchPFCClass7DestmodQueue:
            if (SOC_IS_TD2_TT2(unit)) {
#if defined(BCM_TRIDENT2_SUPPORT)
                if (arg < 0) {
                    return bcm_td2_cosq_port_pfc_op(unit, port, type,
                                               _BCM_COSQ_OP_CLEAR, &arg, 0);
                } else {
                    return bcm_td2_cosq_port_pfc_op(unit, port, type,
                                               _BCM_COSQ_OP_ADD, &arg, 1);
                }
#endif 
            } else if (SOC_IS_TD_TT(unit)) {
#if defined(BCM_TRIDENT_SUPPORT)
                if (arg < 0) {
                    return bcm_td_cosq_port_pfc_op(unit, port, type,
                                               _BCM_COSQ_OP_CLEAR, &arg, 0);
                } else {
                    return bcm_td_cosq_port_pfc_op(unit, port, type,
                                               _BCM_COSQ_OP_SET, &arg, 1);
                }
#endif 
            } else if (SOC_IS_TRIUMPH3(unit)) {
#if defined(BCM_TRIUMPH3_SUPPORT)
                if (arg < 0) {
                    return bcm_tr3_cosq_port_pfc_op(unit, port, type,
                                               _BCM_COSQ_OP_CLEAR, &arg, 0);
                } else {
                    return bcm_tr3_cosq_port_pfc_op(unit, port, type,
                                               _BCM_COSQ_OP_ADD, &arg, 1);
                }
#endif 
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) */

#ifdef BCM_KATANA_SUPPORT
        case bcmSwitchEntropyHashSet1Offset:
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
             if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit)) {
                 /* Previous devices had two hash config sets(Set0 & Set1), 
                  * which each port can select. 
                  * In Triumph3 each port can be configured with its
                  * own hash configs. 
                  */ 
                 return BCM_E_UNAVAIL;
             } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
            if (SOC_IS_KATANAX(unit)) {
                return _bcm_xgs3_fieldoffset_set(unit, port, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchEntropyHashSet0Offset:
            if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                SOC_IS_TRIDENT2(unit)) {
                return _bcm_xgs3_fieldoffset_set(unit, port, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif
#ifdef BCM_TRIDENT_SUPPORT
        case bcmSwitchMiMDefaultSVP:
            if (SOC_IS_TRIDENT(unit) || SOC_IS_TRIDENT2(unit) ||
                SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) {
                return _bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                           MIM_ENABLE_DEFAULT_NETWORK_SVPf, (arg) ? 1 : 0);
            }
            break;
#endif
#ifdef BCM_TRIDENT_SUPPORT
        case bcmSwitchEcmpMacroFlowHashEnable:
            if (SOC_IS_TRIDENT(unit) &&
                soc_feature(unit, soc_feature_l3_ecmp_1k_groups)) {
                uint32 rval, index;
                for (index = 0; index < 2; index++) {
                    rval = 0;
                    SOC_IF_ERROR_RETURN(READ_RTAG7_HASH_ECMPr(unit, index, &rval));
                    soc_reg_field_set(unit, RTAG7_HASH_ECMPr, &rval, USE_FLOW_HASHf,
                                                                     (arg) ? 1 : 0);
                    SOC_IF_ERROR_RETURN(WRITE_RTAG7_HASH_ECMPr(unit, index, rval));
                }
            } else if(SOC_IS_TD2_TT2(unit)) {
                /*Avoid return unvailbale, USE_FLOW_SEL_ECMPf of RTAG7_HASH_SELr
                             was set in xgs3_bindings*/
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif
#ifdef BCM_TRIDENT2_SUPPORT
        case bcmSwitchAlternateStoreForward:
            if (SOC_IS_TD2_TT2(unit)) {
                return _bcm_td2_port_asf_enable_set(unit, port, arg);
            }
            break;
#endif
        case bcmSwitchSystemReservedVlan:
            return _bcm_esw_vlan_system_reserved_set(unit, arg);

        default:
            break;
        }

        found = 0;              /* True if one or more matches found */

        for (i = 0; i < COUNTOF(xgs3_bindings); i++) {
            bcm_switch_binding_t *b = &xgs3_bindings[i];
            uint32 max;
            int fval, fwidth, prt, idx;
            if (b->type == type && (SOC_INFO(unit).chip & b->chip) != 0) {
                if (b->feature && !soc_feature(unit, b->feature)) {
                    continue;
                }

                if (!soc_reg_field_valid(unit, b->reg, b->field)) {
                    continue;
                }

                if (b->xlate_arg) {
                    if ((fval = (b->xlate_arg)(unit, arg, 1)) < 0) {
                        return fval;
                    }
                } else {
                    fval = arg;
                }

                /* Negative values should be treated as errors */
                if (fval < 0) {
                    return BCM_E_PARAM;
                }

                if (SOC_REG_INFO(unit, b->reg).regtype == soc_portreg) {
#ifdef BCM_TRIUMPH2_SUPPORT
                    if (soc_mem_field_valid(unit, PORT_TABm,
                                            PROTOCOL_PKT_INDEXf) && 
                        ((b->reg == PROTOCOL_PKT_CONTROLr) ||
                         (b->reg == IGMP_MLD_PKT_CONTROLr))) {
                        int index;
                        
                        BCM_IF_ERROR_RETURN
                            (_bcm_tr2_protocol_pkt_index_get(unit, port,
                                                             &index));
                        prt = index;
                        idx = 0;
                    } else
#endif                    
                    {
                        prt = port;
                        idx = 0;
                    }
                } else {
#ifdef BCM_TRIUMPH2_SUPPORT
                    if (soc_mem_field_valid(unit, PORT_TABm, 
                                            PROTOCOL_PKT_INDEXf) && 
                        ((b->reg == PROTOCOL_PKT_CONTROLr) ||
                         (b->reg == IGMP_MLD_PKT_CONTROLr))) {
                        int index;

                        BCM_IF_ERROR_RETURN
                            (_bcm_tr2_protocol_pkt_index_get(unit, port,
                                                             &index));
                        prt = REG_PORT_ANY;
                        idx = index;
                    } else
#endif                    
                    {
                        prt = REG_PORT_ANY;
                        idx = 0;
                    }
                }

                fwidth = soc_reg_field_length(unit, b->reg, b->field);
                if (fwidth < 32) {
                    max = (1 << fwidth) - 1;
                } else {
                    max = -1;
                }
                SOC_IF_ERROR_RETURN(soc_reg_get(unit, b->reg, prt, idx, &val64));
                oval64 = val64;
                soc_reg64_field32_set(unit, b->reg, &val64, b->field,
                                    ((uint32)fval > max) ? max : (uint32)fval);
                if (COMPILER_64_NE(val64, oval64)) {
                    SOC_IF_ERROR_RETURN(soc_reg_set(unit, b->reg, prt, idx, val64));
                }

                found = 1;
            }
        }

        return (found ? BCM_E_NONE : BCM_E_UNAVAIL);
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_switch_control_port_get
 * Description:
 *      Retrieve general switch behaviors on a per-port basis
 * Parameters:
 *      unit - Device unit number
 *      port - Port to check
 *      type - The desired configuration parameter to retrieve
 *      arg - Pointer to where the retrieved value will be written
 * Returns:
 *      BCM_E_xxx
 */

int
bcm_esw_switch_control_port_get(int unit,
                                bcm_port_t port,
                                bcm_switch_control_t type,
                                int *arg)
{
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        return BCM_E_UNAVAIL;
    }
#endif /* BCM_SHADOW_SUPPORT */
    if ((type == bcmSwitchFailoverStackTrunk) ||
        (type == bcmSwitchFailoverEtherTrunk)) {
        return BCM_E_UNAVAIL;
    }

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_switch_control_port_validate(unit, port, type, &port));
    }

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        uint64 val64;
        int i;

        switch(type) {
        case bcmSwitchPktAge:
#ifdef BCM_KATANA_SUPPORT
            if (SOC_IS_KATANAX(unit)) {
                return _bcm_kt_pkt_age_get(unit, arg);
            } else
#endif
#ifdef BCM_TRX_SUPPORT
            if (SOC_IS_TRX(unit) && !SOC_IS_HURRICANEX(unit)) {
                return _bcm_ts_pkt_age_get(unit, arg);
            } else
#endif
            if (SOC_IS_FBX(unit)) {
                return _bcm_fb_pkt_age_get(unit, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashL2:
        case bcmSwitchHashL3:
        case bcmSwitchHashMultipath:
        case bcmSwitchHashIpfixIngress:
        case bcmSwitchHashIpfixEgress:
        case bcmSwitchHashIpfixIngressDual:
        case bcmSwitchHashIpfixEgressDual:
        case bcmSwitchHashVlanTranslate:
        case bcmSwitchHashMPLS:
#if defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchHashWlanPort:
        case bcmSwitchHashWlanPortDual:
        case bcmSwitchHashWlanClient:
        case bcmSwitchHashWlanClientDual:
        case bcmSwitchHashRegexAction:
        case bcmSwitchHashRegexActionDual:
#endif
#ifdef BCM_TRIDENT2_SUPPORT
        case bcmSwitchHashL3DNATPool:
        case bcmSwitchHashL3DNATPoolDual:
        case bcmSwitchHashVpVlanMemberIngress:
        case bcmSwitchHashVpVlanMemberIngressDual:
        case bcmSwitchHashVpVlanMemberEgress:
        case bcmSwitchHashVpVlanMemberEgressDual:
        case bcmSwitchHashL2Endpoint:
        case bcmSwitchHashL2EndpointDual:
        case bcmSwitchHashEndpointQueueMap:
        case bcmSwitchHashEndpointQueueMapDual:
#endif
            return _bcm_fb_er_hashselect_get(unit, type, arg);
            break;
        case bcmSwitchHashControl:
            return _bcm_xgs3_hashcontrol_get(unit, arg);
            break;
#if defined(BCM_TRIUMPH_SUPPORT)
        case bcmSwitchFieldMultipathHashSelect:
            return _bcm_field_tr_multipath_hashcontrol_get(unit, arg);
            break;
                        
        case bcmSwitchL3IngressMode:
#ifdef INCLUDE_L3
            if ((soc_feature(unit, soc_feature_l3)) && 
                (soc_feature(unit,soc_feature_l3_ingress_interface))) {
                return bcm_xgs3_l3_ingress_mode_get(unit, arg);
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchL3IngressInterfaceMapSet:
#ifdef INCLUDE_L3
            if ((soc_feature(unit, soc_feature_l3)) && 
                (soc_feature(unit,soc_feature_l3_ingress_interface))) {
                return bcm_xgs3_l3_ingress_intf_map_get(unit, arg);
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;

#endif  /* BCM_TRIUMPH_SUPPORT */
        case bcmSwitchColorSelect:
            return _bcm_fb_er_color_mode_get(unit, port, arg);
            break;
        case bcmSwitchModuleLoopback:
            return _bcm_fb_mod_lb_get(unit, port, arg);
            break;
#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) \
            || defined(BCM_SCORPION_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        case bcmSwitchIngressRateLimitIncludeIFG:
            return _bcm_xgs3_ing_rate_limit_ifg_get(unit, port, arg);
            break;
#endif /* BCM_RAPTOR_SUPPORT || BCM_FIREBOLT2_SUPPORT || BCM_HURRICANE_SUPPORT || BCM_SCORPION_SUPPORT */
        case bcmSwitchVrfMax:
#if defined(BCM_HAWKEYE_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) 
            if(SOC_IS_HAWKEYE(unit) || SOC_IS_HURRICANEX(unit)) {
                return BCM_E_UNAVAIL;
            }
#endif /* BCM_HAWKEYE_SUPPORT || BCM_HURRICANE_SUPPORT */
            return _bcm_vrf_max_get(unit, arg);
            break;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT)
        case bcmSwitchL3UrpfMode:
            if (soc_feature(unit, soc_feature_urpf)) {
#ifdef BCM_TRIDENT2_SUPPORT
#if defined(INCLUDE_L3)
                if (soc_feature(unit, soc_feature_virtual_port_routing) &&
                    (BCM_GPORT_IS_NIV_PORT(port) ||
                     BCM_GPORT_IS_EXTENDER_PORT(port) ||
                     BCM_GPORT_IS_VLAN_PORT(port))) {
                    return _bcm_td2_vp_urpf_mode_get(unit, port, arg);
                } else 
#endif /* INCLUDE_L3 */
#endif /* BCM_TRIDENT2_SUPPORT */          
                {        
                    return _bcm_esw_port_config_get(unit, port,
                                            _bcmPortL3UrpfMode, arg);
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchL3UrpfDefaultRoute:
            if (soc_feature(unit, soc_feature_urpf)) {
                return _bcm_esw_port_config_get(unit, port,
                                            _bcmPortL3UrpfDefaultRoute, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
        defined(BCM_TRX_SUPPORT)
        case bcmSwitchHashL2Dual:
        case bcmSwitchHashL3Dual:
        case bcmSwitchHashVlanTranslateDual:
        case bcmSwitchHashMPLSDual:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                return _bcm_fb_er_hashselect_get(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAPTOR_SUPPORT || BCM_TRX_SUPPORT */
#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRIUMPH_SUPPORT)
        case bcmSwitchHashSelectControl:
            if (SOC_IS_KATANA2(unit)) {
                return _bcm_kt2_selectcontrol_get(unit, arg);
            } else if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit)) {
                return _bcm_xgs3_selectcontrol_get(unit, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashIP4Field0:
        case bcmSwitchHashIP4Field1:
        case bcmSwitchHashIP6Field0:
        case bcmSwitchHashIP6Field1:
        case bcmSwitchHashL2Field0:
        case bcmSwitchHashL2Field1:
        case bcmSwitchHashMPLSField0:
        case bcmSwitchHashMPLSField1:
        case bcmSwitchHashHG2UnknownField0:
        case bcmSwitchHashHG2UnknownField1:
            if ((type == bcmSwitchHashMPLSField0 || type == bcmSwitchHashMPLSField1)
                && (SOC_IS_ENDURO(unit) || SOC_IS_TRIUMPH2(unit) ||
                    SOC_IS_APOLLO(unit) || SOC_IS_VALKYRIE2(unit) ||
                    SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
                    SOC_IS_KATANAX(unit))) {
                return BCM_E_UNAVAIL;
            }
            if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit)) {
                return _bcm_xgs3_fieldselect_get(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#ifdef BCM_TRIUMPH2_SUPPORT
        case bcmSwitchHashL2MPLSField0:
        case bcmSwitchHashL2MPLSField1:
        case bcmSwitchHashL3MPLSField0:
        case bcmSwitchHashL3MPLSField1:
        case bcmSwitchHashMPLSTunnelField0:
        case bcmSwitchHashMPLSTunnelField1:
        case bcmSwitchHashMIMTunnelField0:
        case bcmSwitchHashMIMTunnelField1:
        case bcmSwitchHashMIMField0:
        case bcmSwitchHashMIMField1:
            if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
                SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
                SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
                SOC_IS_KATANAX(unit)) {
                return _bcm_xgs3_fieldselect_get(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif
        case bcmSwitchHashL2VxlanField0:
        case bcmSwitchHashL2VxlanField1:
        case bcmSwitchHashL3VxlanField0:
        case bcmSwitchHashL3VxlanField1:
#if defined(BCM_TRIDENT2_SUPPORT)
           if (SOC_IS_TD2_TT2(unit) && 
               soc_feature(unit, soc_feature_vxlan)) {
                return _bcm_xgs3_fieldselect_get(unit, type, arg);
            }
#endif
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchHashL2L2GreField0:
        case bcmSwitchHashL2L2GreField1:
        case bcmSwitchHashL3L2GreField0:
        case bcmSwitchHashL3L2GreField1:
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
           if ((SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit)) && 
               soc_feature(unit, soc_feature_l2gre)) {
                return _bcm_xgs3_fieldselect_get(unit, type, arg);
            }
#endif
            return BCM_E_UNAVAIL;
            break;

#ifdef BCM_TRIDENT_SUPPORT
        case bcmSwitchHashFCOEField0:
        case bcmSwitchHashFCOEField1:
        case bcmSwitchHashL2TrillField0:
        case bcmSwitchHashL2TrillField1:
        case bcmSwitchHashL3TrillField0:
        case bcmSwitchHashL3TrillField1:
        case bcmSwitchHashTrillTunnelField0:
        case bcmSwitchHashTrillTunnelField1:
            if (SOC_IS_TD_TT(unit)) {
                return _bcm_xgs3_fieldselect_get(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashIP4TcpUdpField0:
        case bcmSwitchHashIP4TcpUdpField1:
        case bcmSwitchHashIP4TcpUdpPortsEqualField0:
        case bcmSwitchHashIP4TcpUdpPortsEqualField1:
        case bcmSwitchHashIP6TcpUdpField0:
        case bcmSwitchHashIP6TcpUdpField1:
        case bcmSwitchHashIP6TcpUdpPortsEqualField0:
        case bcmSwitchHashIP6TcpUdpPortsEqualField1:
            if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) || 
                SOC_IS_KATANAX(unit)) {
                return _bcm_xgs3_fieldselect_get(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashField0PreProcessEnable:
            if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
                uint32 rtag7_hash_control_3;
                SOC_IF_ERROR_RETURN
                    (READ_RTAG7_HASH_CONTROL_3r(unit, &rtag7_hash_control_3));
                *arg = soc_reg_field_get(unit, RTAG7_HASH_CONTROL_3r,
                            rtag7_hash_control_3, HASH_PRE_PROCESSING_ENABLE_Af);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashField1PreProcessEnable:
            if (SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit)) {
                uint32 rtag7_hash_control_3;
                SOC_IF_ERROR_RETURN
                    (READ_RTAG7_HASH_CONTROL_3r(unit, &rtag7_hash_control_3));
                *arg = soc_reg_field_get(unit, RTAG7_HASH_CONTROL_3r,
                            rtag7_hash_control_3, HASH_PRE_PROCESSING_ENABLE_Bf);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_TRIDENT_SUPPORT */

        case bcmSwitchHashField0Config:
        case bcmSwitchHashField0Config1:
        case bcmSwitchHashField1Config:
        case bcmSwitchHashField1Config1:
        case bcmSwitchMacroFlowHashFieldConfig:
            if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit) ||
                SOC_IS_TD_TT(unit) || SOC_IS_TR_VL(unit) ) {
                return _bcm_xgs3_fieldconfig_get(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchTrunkHashSet1UnicastOffset:
        case bcmSwitchTrunkHashSet1NonUnicastOffset:
        case bcmSwitchFabricTrunkHashSet1UnicastOffset:
        case bcmSwitchFabricTrunkHashSet1NonUnicastOffset:
        case bcmSwitchLoadBalanceHashSet1UnicastOffset:
        case bcmSwitchLoadBalanceHashSet1NonUnicastOffset:
        case bcmSwitchECMPHashSet1Offset:
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
             if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
                 /* Previous devices had two hash config sets(Set0 & Set1), 
                  * which each port can select. 
                  * In Triumph3 each port can be configured with its
                  * own hash configs.
                  */ 
                 return BCM_E_UNAVAIL;
             } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
            if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit) ||
                SOC_IS_TD_TT(unit) || SOC_IS_TR_VL(unit) ) {
                return _bcm_xgs3_fieldoffset_get(unit, port, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchTrunkHashSet0UnicastOffset:
        case bcmSwitchTrunkHashSet0NonUnicastOffset:
        case bcmSwitchTrunkFailoverHashOffset:
        case bcmSwitchFabricTrunkHashSet0UnicastOffset:
        case bcmSwitchFabricTrunkHashSet0NonUnicastOffset:
        case bcmSwitchLoadBalanceHashSet0UnicastOffset:
        case bcmSwitchLoadBalanceHashSet0NonUnicastOffset:
        case bcmSwitchFabricTrunkFailoverHashOffset:
        case bcmSwitchFabricTrunkDynamicHashOffset:
        case bcmSwitchTrunkDynamicHashOffset:
        case bcmSwitchEcmpDynamicHashOffset:
        case bcmSwitchFabricTrunkResilientHashOffset:
        case bcmSwitchTrunkResilientHashOffset:
        case bcmSwitchEcmpResilientHashOffset:
        case bcmSwitchECMPHashSet0Offset:
        case bcmSwitchECMPVxlanHashOffset:
        case bcmSwitchECMPL2GreHashOffset:
        case bcmSwitchECMPTrillHashOffset:
        case bcmSwitchECMPMplsHashOffset:
        case bcmSwitchVirtualPortDynamicHashOffset:
            if (SOC_IS_HB_GW(unit) || SOC_IS_TRX(unit) ||
                SOC_IS_TD_TT(unit) || SOC_IS_TR_VL(unit) ) {
                return _bcm_xgs3_fieldoffset_get(unit, port, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_BRADLEY_SUPPORT || BCM_TRIUMPH_SUPPORT */
        case bcmSwitchIgmpPktToCpu:
        case bcmSwitchIgmpPktDrop:
        case bcmSwitchMldPktToCpu:
        case bcmSwitchMldPktDrop:
        case bcmSwitchV4ResvdMcPktToCpu:
        case bcmSwitchV4ResvdMcPktFlood:
        case bcmSwitchV4ResvdMcPktDrop:
        case bcmSwitchV6ResvdMcPktToCpu:
        case bcmSwitchV6ResvdMcPktFlood:
        case bcmSwitchV6ResvdMcPktDrop:
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) ||\
            defined (BCM_TRX_SUPPORT)
        case bcmSwitchIgmpReportLeaveDrop:
        case bcmSwitchIgmpReportLeaveFlood:
        case bcmSwitchIgmpReportLeaveToCpu:
        case bcmSwitchIgmpQueryDrop:
        case bcmSwitchIgmpQueryFlood:
        case bcmSwitchIgmpQueryToCpu:
        case bcmSwitchIgmpUnknownDrop:
        case bcmSwitchIgmpUnknownFlood:
        case bcmSwitchIgmpUnknownToCpu:
        case bcmSwitchMldReportDoneDrop:
        case bcmSwitchMldReportDoneFlood:
        case bcmSwitchMldReportDoneToCpu:
        case bcmSwitchMldQueryDrop:
        case bcmSwitchMldQueryFlood:
        case bcmSwitchMldQueryToCpu:
        case bcmSwitchIpmcV4RouterDiscoveryDrop:
        case bcmSwitchIpmcV4RouterDiscoveryFlood:
        case bcmSwitchIpmcV4RouterDiscoveryToCpu:
        case bcmSwitchIpmcV6RouterDiscoveryDrop:
        case bcmSwitchIpmcV6RouterDiscoveryFlood:
        case bcmSwitchIpmcV6RouterDiscoveryToCpu:
#endif /* defined(BCM_RAPTOR_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT) ||
          defined(BCM_TRX_SUPPORT) */
            return _bcm_xgs3_igmp_action_get(unit, port, type, arg);
            break;

#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || \
        defined(BCM_TRIDENT_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) 
        case bcmSwitchDirectedMirroring:
            if (SOC_IS_RAPTOR(unit) || SOC_IS_RAVEN(unit) || 
                SOC_IS_HAWKEYE(unit) || SOC_IS_HURRICANEX(unit) ||
                SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) ||
                SOC_IS_KATANAX(unit)) {
                *arg = 1;
                return BCM_E_NONE;
            }
            break;
#endif
        case bcmSwitchFlexibleMirrorDestinations:
            return _bcm_esw_mirror_flexible_get(unit, arg);

        case bcmSwitchL3EgressMode:
#ifdef INCLUDE_L3
            if (soc_feature(unit, soc_feature_l3)) {
                return bcm_xgs3_l3_egress_mode_get(unit, arg);
            }
#endif /* INCLUDE_L3 */
            break;

        case bcmSwitchL3HostAsRouteReturnValue:
#ifdef INCLUDE_L3
            if (soc_feature(unit, soc_feature_l3)) {
                  return bcm_xgs3_l3_host_as_route_return_get(unit, arg);
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchL3DefipMultipathCountUpdate:
#ifdef INCLUDE_L3        
            if(soc_feature(unit, soc_feature_l3_defip_ecmp_count)) {
              *arg = 1;
              return BCM_E_NONE;
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchIpmcReplicationSharing:
#ifdef INCLUDE_L3
            if (soc_feature(unit, soc_feature_ip_mcast_repl)) {
                *arg = SOC_IPMCREPLSHR_GET(unit);
                return BCM_E_NONE;
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchIpmcReplicationAvailabilityThreshold:
#ifdef INCLUDE_L3
            if (soc_feature(unit, soc_feature_ip_mcast_repl)) {
                return _bcm_esw_ipmc_repl_threshold_get(unit, arg);
            }
#endif /* INCLUDE_L3 */
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepth:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthL2:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_L2X(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthMpls:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_MPLS(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthVlan:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_VLAN(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthEgressVlan:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_EGRESS_VLAN(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
#if defined(INCLUDE_L3)
        case bcmSwitchHashDualMoveDepthL3:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_L3X(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchHashDualMoveDepthWlanPort:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_WLAN_PORT(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthWlanClient:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_WLAN_CLIENT(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
#endif
        case bcmSwitchL2PortBlocking:
            if (soc_feature(unit, soc_feature_src_mac_group)) {
                *arg = (!SOC_L2X_GROUP_ENABLE_GET(unit));
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
#if defined(BCM_HAWKEYE_SUPPORT)|| defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT)
        case bcmSwitchTimeSyncPktDrop:
        case bcmSwitchTimeSyncPktFlood:
        case bcmSwitchTimeSyncPktToCpu:
        case bcmSwitchMmrpPktDrop:
        case bcmSwitchMmrpPktFlood:
        case bcmSwitchMmrpPktToCpu:
        case bcmSwitchSrpPktDrop:
        case bcmSwitchSrpPktFlood:
        case bcmSwitchSrpPktToCpu:
            return _bcm_xgs3_eav_action_get(unit, port, type, arg);
            break;
        case bcmSwitchSRPEthertype:
        case bcmSwitchMMRPEthertype:
        case bcmSwitchTimeSyncEthertype: 
            return _bcm_xgs3_switch_ethertype_get(unit, port, type, arg); 
            break;
        case bcmSwitchSRPDestMacOui:
        case bcmSwitchMMRPDestMacOui:
        case bcmSwitchTimeSyncDestMacOui:
            return _bcm_xgs3_switch_mac_hi_get(unit, port, type, arg); 
            break;
        case bcmSwitchSRPDestMacNonOui:
        case bcmSwitchMMRPDestMacNonOui:
        case bcmSwitchTimeSyncDestMacNonOui:
            return _bcm_xgs3_switch_mac_lo_get(unit, port, type, arg); 
            break;
        case bcmSwitchTimeSyncMessageTypeBitmap:
            if (soc_reg_field_valid(unit, TS_CONTROLr, TS_MSG_BITMAPf)) {
                uint32 ts_control_rval;
                SOC_IF_ERROR_RETURN
                   (READ_TS_CONTROLr(unit, &ts_control_rval));
                *arg = soc_reg_field_get(unit, TS_CONTROLr, ts_control_rval, 
                                         TS_MSG_BITMAPf);
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchTimeSyncClassAPktPrio:
        case bcmSwitchTimeSyncClassBPktPrio:
        case bcmSwitchTimeSyncClassAExeptionPktPrio:
        case bcmSwitchTimeSyncClassBExeptionPktPrio:
             return _bcm_switch_class_pkt_prio_get(unit, type, arg);
            break;


#endif /* defined(BCM_HAWKEYE_SUPPORT)*/
#if defined(BCM_TRIUMPH2_SUPPORT)
        case bcmSwitchUnknownVlanToCpu:
        case bcmSwitchL3HeaderErrToCpu:
        case bcmSwitchIpmcTtlErrToCpu:
        case bcmSwitchHgHdrErrToCpu:
        case bcmSwitchTunnelErrToCpu:
             if (SOC_REG_IS_VALID(unit, EGR_CPU_CONTROLr) &&
                    soc_feature(unit, soc_feature_internal_loopback)) {
                 return _bcm_tr2_ep_redirect_action_get(unit, port, type, arg);
             }
             break;
        case bcmSwitchStgInvalidToCpu:
        case bcmSwitchVlanTranslateEgressMissToCpu:
        case bcmSwitchL3PktErrToCpu:
        case bcmSwitchMtuFailureToCpu:
        case bcmSwitchSrcKnockoutToCpu:
        case bcmSwitchWlanTunnelMismatchToCpu:
        case bcmSwitchWlanTunnelMismatchDrop:
        case bcmSwitchWlanPortMissToCpu:
            return _bcm_tr2_ep_redirect_action_get(unit, port, type, arg);
            break;
        case bcmSwitchUnknownVlanToCpuCosq:
        case bcmSwitchStgInvalidToCpuCosq:
        case bcmSwitchVlanTranslateEgressMissToCpuCosq:
        case bcmSwitchTunnelErrToCpuCosq:
        case bcmSwitchL3HeaderErrToCpuCosq:
        case bcmSwitchL3PktErrToCpuCosq:
        case bcmSwitchIpmcTtlErrToCpuCosq:
        case bcmSwitchMtuFailureToCpuCosq:
        case bcmSwitchHgHdrErrToCpuCosq:
        case bcmSwitchSrcKnockoutToCpuCosq:
        case bcmSwitchWlanTunnelMismatchToCpuCosq:
        case bcmSwitchWlanPortMissToCpuCosq:
            return _bcm_tr2_ep_redirect_action_cosq_get(unit, port, type, arg);
            break;
        case bcmSwitchLayeredQoSResolution:
            return _bcm_tr2_layered_qos_resolution_get(unit, port, type, arg);
            break;
        case bcmSwitchEncapErrorToCpu:
            return _bcm_tr2_ehg_error2cpu_get(unit, port, arg);
            break;
        case bcmSwitchMirrorEgressTrueColorSelect:
        case bcmSwitchMirrorEgressTruePriority:
            return _bcm_tr2_mirror_egress_true_get(unit, port, type, arg);
            break;
#endif /* BCM_TRIUMPH2_SUPPORT */

#if defined(BCM_ENDURO_SUPPORT)
        case bcmSwitchL3UrpfRouteEnableExternal:
            if(SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit)) {
                return BCM_E_UNAVAIL;
            }
            break;
#endif /* BCM_ENDURO_SUPPORT */
        case bcmSwitchLinkDownInfoSkip:
            return _bcm_esw_link_port_info_skip_get(unit, port, arg); 
            break;

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchCongestionNotificationIdLow:
            if (soc_feature(unit, soc_feature_qcn)) {
                egr_port_entry_t entry;
                BCM_IF_ERROR_RETURN
                    (soc_mem_read(unit, EGR_PORTm, MEM_BLOCK_ANY, port,
                                  &entry));
                *arg = soc_mem_field32_get(unit, EGR_PORTm, &entry,
                                           QCN_CNM_CPID_PORT_PREFIXf);
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif /* defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) */

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchPFCClass0Queue:
        case bcmSwitchPFCClass1Queue:
        case bcmSwitchPFCClass2Queue:
        case bcmSwitchPFCClass3Queue:
        case bcmSwitchPFCClass4Queue:
        case bcmSwitchPFCClass5Queue:
        case bcmSwitchPFCClass6Queue:
        case bcmSwitchPFCClass7Queue:
        case bcmSwitchPFCClass0McastQueue:
        case bcmSwitchPFCClass1McastQueue:
        case bcmSwitchPFCClass2McastQueue:
        case bcmSwitchPFCClass3McastQueue:
        case bcmSwitchPFCClass4McastQueue:
        case bcmSwitchPFCClass5McastQueue:
        case bcmSwitchPFCClass6McastQueue:
        case bcmSwitchPFCClass7McastQueue:
        case bcmSwitchPFCClass0DestmodQueue:
        case bcmSwitchPFCClass1DestmodQueue:
        case bcmSwitchPFCClass2DestmodQueue:
        case bcmSwitchPFCClass3DestmodQueue:
        case bcmSwitchPFCClass4DestmodQueue:
        case bcmSwitchPFCClass5DestmodQueue:
        case bcmSwitchPFCClass6DestmodQueue:
        case bcmSwitchPFCClass7DestmodQueue:
            if (SOC_IS_TD2_TT2(unit)) {
#if defined(BCM_TRIDENT2_SUPPORT)
                int count;
                return bcm_td2_cosq_port_pfc_get(unit, port, type,
                                                (bcm_gport_t *)arg, 1, &count);
#endif 
            } else if (SOC_IS_TD_TT(unit)) {
#if defined(BCM_TRIDENT_SUPPORT)
                int count;
                return bcm_td_cosq_port_pfc_get(unit, port, type,
                                                (bcm_gport_t *)arg, 1, &count);
#endif 
            } else if (SOC_IS_TRIUMPH3(unit)) {
#if defined(BCM_TRIUMPH3_SUPPORT)
                int count;
                return bcm_tr3_cosq_port_pfc_get(unit, port, type,
                                                (bcm_gport_t *)arg, 1, &count);
#endif 
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) */

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchEntropyHashSet1Offset:
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
             if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit)) {
                 /* Previous devices had two hash config sets(Set0 & Set1), 
                  * which each port can select. 
                  * In Triumph3 each port can be configured with its
                  * own hash configs. 
                  */ 
                 return BCM_E_UNAVAIL;
             } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
            if (SOC_IS_KATANAX(unit)) {
                return _bcm_xgs3_fieldoffset_get(unit, port, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchEntropyHashSet0Offset:
            if (SOC_IS_KATANAX(unit)|| SOC_IS_TRIUMPH3(unit) ||
                SOC_IS_TRIDENT2(unit)) {
                return _bcm_xgs3_fieldoffset_get(unit, port, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
        case bcmSwitchMiMDefaultSVP:
            if (SOC_IS_TRIDENT(unit) || SOC_IS_TRIDENT2(unit) ||
                SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) {
                return _bcm_esw_port_tab_get(unit, port,
                           MIM_ENABLE_DEFAULT_NETWORK_SVPf, arg);
            }
            break;
#endif
#ifdef BCM_TRIDENT2_SUPPORT
        case bcmSwitchAlternateStoreForward:
            if (SOC_IS_TD2_TT2(unit)) {
                return _bcm_td2_port_asf_enable_get(unit, port, arg);
            }
            break;
#endif
        default :
            break;
        }

        for (i = 0; i < COUNTOF(xgs3_bindings); i++) {
            bcm_switch_binding_t *b = &xgs3_bindings[i];
            int prt, idx;

            if (b->type == type && (SOC_INFO(unit).chip & b->chip) != 0) {
                if (b->feature && !soc_feature(unit, b->feature)) {
                    continue;
                }
                if (!SOC_REG_IS_VALID(unit, b->reg)) {
                    continue;
                }

                if (!soc_reg_field_valid(unit, b->reg, b->field)) {
                    continue;
                }
                if (SOC_REG_INFO(unit, b->reg).regtype == soc_portreg) {
#ifdef BCM_TRIUMPH2_SUPPORT
                    if (soc_mem_field_valid(unit, PORT_TABm,
                                            PROTOCOL_PKT_INDEXf) && 
                        ((b->reg == PROTOCOL_PKT_CONTROLr) || 
                         (b->reg == IGMP_MLD_PKT_CONTROLr))) {
                        int index;
                    
                        BCM_IF_ERROR_RETURN
                            (_bcm_tr2_protocol_pkt_index_get(unit, port,
                                                             &index));
                        prt = index;
                        idx = 0;
                    } else
#endif                    
                    {
                        prt = port;
                        idx = 0;
                    }
                } else {
#ifdef BCM_TRIUMPH2_SUPPORT
                    if (soc_mem_field_valid(unit, PORT_TABm, 
                                            PROTOCOL_PKT_INDEXf) && 
                        ((b->reg == PROTOCOL_PKT_CONTROLr) || 
                         (b->reg == IGMP_MLD_PKT_CONTROLr))) { 
                        int index;

                        BCM_IF_ERROR_RETURN
                            (_bcm_tr2_protocol_pkt_index_get(unit, port,
                                                             &index));
                        prt = REG_PORT_ANY;
                        idx = index;
                    } else
#endif                    
                    {
                        prt = REG_PORT_ANY;
                        idx = 0;
                    }
                }

                SOC_IF_ERROR_RETURN(soc_reg_get(unit, b->reg, prt, idx, &val64));
                *arg = soc_reg64_field32_get(unit, b->reg, val64, b->field);

                if (b->xlate_arg) {
                    *arg = (b->xlate_arg)(unit, *arg, 0);
                }
                return BCM_E_NONE;
            }
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    return BCM_E_UNAVAIL;
}

#define _BCM_SWITCH_ETHERTYPE_VLAN_TAG_MASK     0xFFFF

/*
 * Function:
 *      bcm_switch_control_set
 * Description:
 *      Specify general switch behaviors.
 * Parameters:
 *      unit - Device unit number
 *      type - The desired configuration parameter to modify
 *      arg - The value with which to set the parameter
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      For behaviors which are port-specific, all non-stack ports
 *      will be set.
 */

int
bcm_esw_switch_control_set(int unit,
                           bcm_switch_control_t type,
                           int arg)
{
    bcm_port_t    port;
    int                 rv, found;
#if defined(BCM_WARM_BOOT_SUPPORT) && defined(BCM_XGS3_SWITCH_SUPPORT)
    int                 stable_select;
#endif

#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        return BCM_E_UNAVAIL;
    }
#endif /* BCM_SHADOW_SUPPORT */

    /* Check for device wide, non port specific controls */
    switch (type) {
    case bcmSwitchWarmBoot:
        /* If true, set the Warm Boot state; clear otherwise */
        if (arg) {
#ifdef BCM_WARM_BOOT_SUPPORT
            /* Cannot be set to anything but FALSE */
            return BCM_E_PARAM;
#else
            return SOC_E_UNAVAIL;
#endif
        } else {
#ifdef BCM_WARM_BOOT_SUPPORT
            SOC_WARM_BOOT_DONE(unit);
#else
            return SOC_E_UNAVAIL;
#endif
        }
        return BCM_E_NONE;
        break;
    case bcmSwitchStableSelect:
        /* Cannot be called once bcm_init is complete */
        return SOC_E_UNAVAIL;
        break;
    case bcmSwitchStableSize:
        /* Cannot be called once bcm_init is complete */
        return SOC_E_UNAVAIL;
        break;
    case bcmSwitchControlSync:
#if defined(BCM_WARM_BOOT_SUPPORT) && defined(BCM_XGS3_SWITCH_SUPPORT)
        rv = _bcm_switch_control_sync(unit, arg);
        return rv;
#else
        return SOC_E_UNAVAIL;
#endif
        break;
    case bcmSwitchControlAutoSync:
#if defined(BCM_WARM_BOOT_SUPPORT) && defined(BCM_XGS3_SWITCH_SUPPORT)
        BCM_IF_ERROR_RETURN
            (bcm_esw_switch_control_get(unit, bcmSwitchStableSelect,
                                        &stable_select));
        if (BCM_SWITCH_STABLE_NONE != stable_select) {
            SOC_CONTROL_LOCK(unit);
            SOC_CONTROL(unit)->autosync = arg ? 1 : 0;
            SOC_CONTROL_UNLOCK(unit);
            return BCM_E_NONE;
        } else {
            return BCM_E_NOT_FOUND;
        }
#else
        return SOC_E_UNAVAIL;
#endif
        break;
    case bcmSwitchModuleType:
        return(_bcm_switch_module_type_set(unit,
                                           BCM_SWITCH_CONTROL_MODID_UNPACK(arg),
                                           BCM_SWITCH_CONTROL_MODTYPE_UNPACK(arg)));
        break;
#ifdef BCM_CB_ABORT_ON_ERR
    case bcmSwitchCallbackAbortOnError:
        if (arg >= 0) {
            SOC_CB_ABORT_ON_ERR(unit) = arg ? 1 : 0;
            return BCM_E_NONE;
        } else {
            return BCM_E_PARAM;
        }
        break;
#endif
    case bcmSwitchIpmcGroupMtu:
        if (soc_feature(unit, soc_feature_ipmc_group_mtu)) {
            uint32 val;

            if (arg <= 0 || arg > 0x3fff) {
                return BCM_E_PARAM;
            }

            rv = READ_IPMC_L3_MTUr(unit, 0, &val);
            if (rv >= 0) {
                soc_reg_field_set(unit, IPMC_L3_MTUr, &val, MTU_LENf, arg);
                rv = WRITE_IPMC_L3_MTUr(unit, 0, val);
            }
            return rv;
        }
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchMcastFloodDefault:
        return(_bcm_esw_vlan_flood_default_set(unit, arg));
        break;

    case bcmSwitchUseGport:
        SOC_USE_GPORT_SET(unit, arg);
        return BCM_E_NONE;
        break;

    case bcmSwitchHgHdrMcastFlood:
        return(_bcm_esw_higig_flood_l2_set(unit, arg));
        break;

    case bcmSwitchHgHdrIpMcastFlood:
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchFailoverStackTrunk:
    case bcmSwitchFailoverEtherTrunk:
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchMcastL2Range:
#ifdef BCM_BRADLEY_SUPPORT
        if (SOC_IS_HBX(unit) || SOC_IS_SC_CQ(unit)) {
            return(soc_hbx_mcast_size_set(unit, arg));
        }
#endif
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchMcastL3Range:
#ifdef BCM_BRADLEY_SUPPORT
        if (SOC_IS_HBX(unit) || SOC_IS_SC_CQ(unit)) {
            return(soc_hbx_ipmc_size_set(unit, arg));
        }
#endif
        return BCM_E_UNAVAIL;
        break;

    /*    coverity[equality_cond]    */
    case bcmSwitchHgHdrMcastVlanRange:
    case bcmSwitchHgHdrMcastL2Range:
    case bcmSwitchHgHdrMcastL3Range:
#ifdef BCM_BRADLEY_SUPPORT
        if (SOC_IS_HBX(unit) || SOC_IS_TRX(unit)) {
            int bc_size, mc_size, ipmc_size;
            BCM_IF_ERROR_RETURN
                (soc_hbx_higig2_mcast_sizes_get(unit, &bc_size,
                                                &mc_size, &ipmc_size));
            switch (type) {
            case bcmSwitchHgHdrMcastVlanRange:
                bc_size = arg;
                break;
            case bcmSwitchHgHdrMcastL2Range:
                mc_size = arg;
                break;
            case bcmSwitchHgHdrMcastL3Range:
                ipmc_size = arg;
                break;
            /* Defensive Default */
            /* coverity[dead_error_begin] */
            default:
                return BCM_E_PARAM;
            }
            return soc_hbx_higig2_mcast_sizes_set(unit, bc_size,
                                                  mc_size, ipmc_size);
        }
#endif
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchHashSeed0:
#ifdef BCM_BRADLEY_SUPPORT
        if ((SOC_IS_HBX(unit) || SOC_IS_TRX(unit)) && (!(SOC_IS_HURRICANEX(unit)))) {
            return soc_reg_field32_modify
                (unit, RTAG7_HASH_SEED_Ar, REG_PORT_ANY, HASH_SEED_Af, arg);
        }
#endif /* BCM_BRADLEY_SUPPORT */
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchHashSeed1:
#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || \
    defined(BCM_TRIDENT_SUPPORT)
        if ((SOC_IS_HBX(unit) || SOC_IS_TD_TT(unit) || SOC_IS_TRX(unit) ||
            SOC_IS_TRIUMPH3(unit)) && 
            (!(SOC_IS_HURRICANEX(unit)))) {
            return soc_reg_field32_modify
                (unit, RTAG7_HASH_SEED_Br, REG_PORT_ANY, HASH_SEED_Bf, arg);
        }
#endif /* BCM_BRADLEY_SUPPORT */
        return BCM_E_UNAVAIL;
        break;

#ifdef BCM_TRIDENT_SUPPORT
        case bcmSwitchMacroFlowHashMinOffset:
        case bcmSwitchMacroFlowHashMaxOffset:
        case bcmSwitchMacroFlowHashStrideOffset:
            return _bcm_td_macroflow_offset_set(unit, type, arg);
            break;
#endif /* BCM_TRIDENT_SUPPORT */

        /* RTAG7 Macro Flow Concatenation */
        case bcmSwitchMacroFlowEcmpHashConcatEnable:
        case bcmSwitchMacroFlowLoadBalanceHashConcatEnable:
        case bcmSwitchMacroFlowTrunkHashConcatEnable:
        case bcmSwitchMacroFlowHigigTrunkHashConcatEnable:
        /* RTAG7 Macro Flow Hash Seed */
        case bcmSwitchMacroFlowECMPHashSeed:
        case bcmSwitchMacroFlowLoadBalanceHashSeed:
        case bcmSwitchMacroFlowTrunkHashSeed:
        case bcmSwitchMacroFlowHigigTrunkHashSeed:
#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
            if (SOC_IS_TRIDENT2(unit) || SOC_IS_TRIUMPH3(unit)) {
                return _bcm_td2_macroflow_hash_set(unit, type, arg);
            }
#endif /* BCM_TRIDENT2_SUPPORT */
            return BCM_E_UNAVAIL;
            break;

        /* Hash Field select controls */ 
        case bcmSwitchHashTrillPayloadSelect0:
        case bcmSwitchHashTrillPayloadSelect1: 
        case bcmSwitchHashTrillTunnelSelect0:
        case bcmSwitchHashTrillTunnelSelect1: 
#if defined(BCM_TRIDENT_SUPPORT)
            if ((SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) &&
                soc_feature(unit, soc_feature_trill)) {
                return _bcm_xgs3_field_control_set(unit, type, arg);
            }
#endif /* BCM_TRIDENT_SUPPORT */
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchHashIP6AddrCollapseSelect0:
        case bcmSwitchHashIP6AddrCollapseSelect1: 
#if defined(BCM_TRIDENT_SUPPORT)
            if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) || 
                SOC_IS_KATANAX(unit)) {
                return _bcm_xgs3_field_control_set(unit, type, arg);
            }
#endif /* BCM_TRIDENT_SUPPORT */
            return BCM_E_UNAVAIL;
            break;

    case bcmSwitchL2McastAllRouterPortsAddEnable:
        SOC_MCAST_ADD_ALL_ROUTER_PORTS(unit) = arg ? 1 : 0;
        return BCM_E_NONE;
        break;

    case bcmSwitchPFCClass0Queue:
    case bcmSwitchPFCClass1Queue:
    case bcmSwitchPFCClass2Queue:
    case bcmSwitchPFCClass3Queue:
    case bcmSwitchPFCClass4Queue:
    case bcmSwitchPFCClass5Queue:
    case bcmSwitchPFCClass6Queue:
    case bcmSwitchPFCClass7Queue:
#if defined(BCM_SCORPION_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT)
        if (SOC_IS_SC_CQ(unit) || SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
            SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit)) {
            return _bcm_tr2_sc_pfc_priority_to_cos_set(unit, type, arg);
        }
#endif /* BCM_SCORPION_SUPPORT || BCM_TRIUMPH2_SUPPORT */
       return BCM_E_UNAVAIL;
       break;

    case bcmSwitchL2HitClear:
    case bcmSwitchL2SrcHitClear:
    case bcmSwitchL2DstHitClear:
    case bcmSwitchL3HostHitClear:
    case bcmSwitchL3RouteHitClear:
       return _bcm_esw_switch_hit_clear_set(unit, type, arg);
       break;

    /* This swithc control should be supported only per port. */
    case bcmSwitchEncapErrorToCpu:
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchSnapNonZeroOui:
#if defined(BCM_TRX_SUPPORT)
        if (SOC_IS_TRX(unit)) {
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ING_CONFIG_64r, REG_PORT_ANY,
                                        SNAP_OTHER_DECODE_ENABLEf,
                                        arg ? 1 : 0));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_CONFIGr, REG_PORT_ANY,
                                        SNAP_OTHER_DECODE_ENABLEf,
                                        arg ? 1 : 0));
            return BCM_E_NONE;
        } else
#endif /* BCM_TRX_SUPPORT */
#if defined(BCM_FIREBOLT_SUPPORT)
        if (SOC_IS_FB(unit) || SOC_IS_FX_HX(unit) || SOC_IS_HB_GW(unit)) {
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ING_CONFIGr, REG_PORT_ANY,
                                        SNAP_OTHER_DECODE_ENABLEf,
                                        arg ? 1 : 0));
            if (SOC_IS_FIREBOLT2(unit)) {
                SOC_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, EGR_CONFIGr, REG_PORT_ANY,
                                            SNAP_OTHER_DECODE_ENABLEf,
                                            arg ? 1 : 0));
            }
            return BCM_E_NONE;
        }
#endif /* BCM_FIREBOLT_SUPPORT */

        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchL2McIdxRetType:
        return _bcm_esw_mcast_idx_ret_type_set(unit, arg);
        break;

    case bcmSwitchL3McIdxRetType:
#ifdef INCLUDE_L3
        return _bcm_esw_ipmc_idx_ret_type_set(unit, arg);
#endif
        return BCM_E_UNAVAIL;
        break;
    case bcmSwitchL2SourceDiscardMoveToCpu:
        {
            soc_reg_t   reg;

            if (SOC_REG_FIELD_VALID(unit, ING_CONFIGr, STNMOVE_ON_L2SRC_DISCf)){
                reg = ING_CONFIGr;
            } else if (SOC_REG_FIELD_VALID(unit, ING_CONFIG_64r, 
                                           STNMOVE_ON_L2SRC_DISCf)) {
                reg = ING_CONFIG_64r;
            } else {
                return BCM_E_UNAVAIL;
            }

            BCM_IF_ERROR_RETURN(
                soc_reg_field32_modify(unit, reg, REG_PORT_ANY, 
                                       STNMOVE_ON_L2SRC_DISCf, arg ? 1: 0));
            return BCM_E_NONE;
        }
        break;

#ifdef BCM_TRIDENT_SUPPORT
    case bcmSwitchFabricTrunkDynamicSampleRate:
    case bcmSwitchFabricTrunkDynamicEgressBytesExponent:
    case bcmSwitchFabricTrunkDynamicQueuedBytesExponent:
    case bcmSwitchFabricTrunkDynamicEgressBytesDecreaseReset:
    case bcmSwitchFabricTrunkDynamicQueuedBytesDecreaseReset:
    case bcmSwitchFabricTrunkDynamicEgressBytesMinThreshold:
    case bcmSwitchFabricTrunkDynamicEgressBytesMaxThreshold:
    case bcmSwitchFabricTrunkDynamicQueuedBytesMinThreshold:
    case bcmSwitchFabricTrunkDynamicQueuedBytesMaxThreshold:
        if (!soc_feature(unit, soc_feature_hg_dlb)) {
            return BCM_E_UNAVAIL;
        }
        return _bcm_trident_hg_dlb_config_set(unit, type, arg);
        break;
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
    case bcmSwitchTrunkDynamicSampleRate:
    case bcmSwitchTrunkDynamicAccountingSelect:
    case bcmSwitchTrunkDynamicEgressBytesExponent:
    case bcmSwitchTrunkDynamicQueuedBytesExponent:
    case bcmSwitchTrunkDynamicEgressBytesDecreaseReset:
    case bcmSwitchTrunkDynamicQueuedBytesDecreaseReset:
    case bcmSwitchTrunkDynamicEgressBytesMinThreshold:
    case bcmSwitchTrunkDynamicEgressBytesMaxThreshold:
    case bcmSwitchTrunkDynamicQueuedBytesMinThreshold:
    case bcmSwitchTrunkDynamicQueuedBytesMaxThreshold:
    case bcmSwitchTrunkDynamicExpectedLoadMinThreshold:
    case bcmSwitchTrunkDynamicExpectedLoadMaxThreshold:
    case bcmSwitchTrunkDynamicImbalanceMinThreshold:
    case bcmSwitchTrunkDynamicImbalanceMaxThreshold:
        if (!soc_feature(unit, soc_feature_lag_dlb)) {
            return BCM_E_UNAVAIL;
        }
        return bcm_tr3_lag_dlb_config_set(unit, type, arg);
        break;
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    case bcmSwitchEcmpDynamicSampleRate:
    case bcmSwitchEcmpDynamicAccountingSelect:
    case bcmSwitchEcmpDynamicEgressBytesExponent:
    case bcmSwitchEcmpDynamicQueuedBytesExponent:
    case bcmSwitchEcmpDynamicEgressBytesDecreaseReset:
    case bcmSwitchEcmpDynamicQueuedBytesDecreaseReset:
    case bcmSwitchEcmpDynamicEgressBytesMinThreshold:
    case bcmSwitchEcmpDynamicEgressBytesMaxThreshold:
    case bcmSwitchEcmpDynamicQueuedBytesMinThreshold:
    case bcmSwitchEcmpDynamicQueuedBytesMaxThreshold:
    case bcmSwitchEcmpDynamicExpectedLoadMinThreshold:
    case bcmSwitchEcmpDynamicExpectedLoadMaxThreshold:
    case bcmSwitchEcmpDynamicImbalanceMinThreshold:
    case bcmSwitchEcmpDynamicImbalanceMaxThreshold:
        if (!soc_feature(unit, soc_feature_ecmp_dlb)) {
            return BCM_E_UNAVAIL;
        }
        return _bcm_tr3_ecmp_dlb_config_set(unit, type, arg);
        break;
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

#if (defined(BCM_RCPU_SUPPORT) || defined(BCM_OOB_RCPU_SUPPORT)) && \
     defined(BCM_XGS3_SWITCH_SUPPORT)
        case bcmSwitchRemoteCpuSchanEnable:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, CMIC_PKT_CTRLr,
                        REG_PORT_ANY, ENABLE_SCHAN_REQUESTf, arg));
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuMatchLocalMac:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, 
                      CMIC_PKT_CTRLr, REG_PORT_ANY, LMAC0_MATCHf, arg));
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuFromCpuEnable:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                   CMIC_PKT_CTRLr, REG_PORT_ANY, ENABLE_FROMCPU_PACKETf, arg));
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuToCpuEnable:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                   CMIC_PKT_CTRLr, REG_PORT_ANY, ENABLE_TOCPU_PACKETf, arg));
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuCmicEnable:
#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm)) {
                SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                   CMIC_PKT_CTRLr, REG_PORT_ANY, ENABLE_CMIC_REQUESTf, arg));
                return BCM_E_NONE;
            } else
#endif
            {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuMatchVlan:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit,
                            CMIC_PKT_CTRLr, REG_PORT_ANY, VLAN_MATCHf, arg));
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuForceScheduling:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm) && 
                     (SOC_IS_KATANAX(unit) || 
                      SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit))) {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_COS_0r, COSf)) {
                        BCM_IF_ERROR_RETURN(
                            _bcm_rcpu_switch_regall_set(unit, CMIC_PKT_COS_0r, 
                                                    COSf, arg));
                    }
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_COS_1r, COSf)) {
                        BCM_IF_ERROR_RETURN(
                          _bcm_rcpu_switch_regall_set(unit, CMIC_PKT_COS_1r, 
                                                  COSf, arg));
                    }
                } else
#endif
                {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_COSr, COSf)) {
                        BCM_IF_ERROR_RETURN(
                            _bcm_rcpu_switch_regall_set(unit, CMIC_PKT_COSr, 
                                                    COSf, arg));
                    }
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_COS_HIr, COSf)) {
                        BCM_IF_ERROR_RETURN(
                          _bcm_rcpu_switch_regall_set(unit, CMIC_PKT_COS_HIr, 
                                                  COSf, arg));
                    }
                }
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuToCpuDestPortAllReasons:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit))) {
                    
                        int i;

                    if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASON_DIRECT_0_TYPEr) && 
                        SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_DIRECT_0_TYPEr, REASONSf)) {

                        for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_DIRECT_0_TYPEr); i++) {
                            BCM_IF_ERROR_RETURN(
                            _bcm_rcpu_switch_regall_idx_set(unit, 
                                                    CMIC_PKT_REASON_DIRECT_0_TYPEr, 
                                                    i, REASONSf, arg));
                        } 
                    }

                    if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASON_DIRECT_1_TYPEr) && 
                        SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_DIRECT_1_TYPEr, REASONSf)) {

                        for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_DIRECT_1_TYPEr); i++) {
                            BCM_IF_ERROR_RETURN(
                            _bcm_rcpu_switch_regall_idx_set(unit, 
                                                    CMIC_PKT_REASON_DIRECT_1_TYPEr, 
                                                    i, REASONSf, arg));
                        }
                   } 

                   if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASON_DIRECT_2_TYPEr) &&
                       SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_DIRECT_2_TYPEr, REASONSf)) {

                       for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_DIRECT_2_TYPEr); i++) {
                            BCM_IF_ERROR_RETURN(
                            _bcm_rcpu_switch_regall_idx_set(unit, 
                                                    CMIC_PKT_REASON_DIRECT_2_TYPEr, 
                                                    i, REASONSf, arg));
                        }
                    }
                    
                } else
#endif
                {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_DIRECTr, 
                                        REASONSf)) {
                        BCM_IF_ERROR_RETURN(
                            _bcm_rcpu_switch_regall_set(unit, 
                                                    CMIC_PKT_REASON_DIRECTr, 
                                                    REASONSf, arg));
                    }
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_DIRECT_HIr, 
                                        REASONSf)) {
                        BCM_IF_ERROR_RETURN(
                         _bcm_rcpu_switch_regall_set(unit, 
                                             CMIC_PKT_REASON_DIRECT_HIr, 
                                                  REASONSf,arg));
                    }
                }
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuToCpuDestMacAllReasons:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                int i;

                if (soc_feature(unit, soc_feature_cmicm) 
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit))) {

                    if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASON_0_TYPEr) && 
                        SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_0_TYPEr, REASONSf)) {
                        for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_0_TYPEr); i++) {
                            BCM_IF_ERROR_RETURN(
                            _bcm_rcpu_switch_regall_idx_set(unit, 
                                                    CMIC_PKT_REASON_0_TYPEr, 
                                                    i, REASONSf, arg));
                        }
                    }

                    if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASON_1_TYPEr) && 
                        SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_1_TYPEr, REASONSf)) {
                        for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_1_TYPEr); i++) {
                            BCM_IF_ERROR_RETURN(
                            _bcm_rcpu_switch_regall_idx_set(unit, 
                                                    CMIC_PKT_REASON_1_TYPEr, 
                                                    i, REASONSf, arg));
                        }
                    } 

                    if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASON_2_TYPEr) && 
                        SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_2_TYPEr, REASONSf)) {
                        for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_2_TYPEr); i++) {
                            BCM_IF_ERROR_RETURN(
                            _bcm_rcpu_switch_regall_idx_set(unit, 
                                                    CMIC_PKT_REASON_2_TYPEr, 
                                                    i, REASONSf, arg));
                        }
                    }
                } else
#endif
                {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASONr, REASONSf)) {
                        BCM_IF_ERROR_RETURN(
                            _bcm_rcpu_switch_regall_set(unit, CMIC_PKT_REASONr, 
                                                    REASONSf, arg));
                    }
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_HIr, REASONSf)) {
                        BCM_IF_ERROR_RETURN(
                          _bcm_rcpu_switch_regall_set(unit, 
                                                  CMIC_PKT_REASON_HIr, 
                                                  REASONSf, arg));
                    }
                }
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRxRedirectPktReasons:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit))) {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_0_TYPEr, REASONSf)) {
                        uint32 raddr;
                        int i;

                        for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_0_TYPEr); i++) {
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_0_TYPEr,
                                REG_PORT_ANY, i);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));                
                    }
                    }
                } else
#endif
                {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASONr, REASONSf)) {
                        uint32 raddr;
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASONr,
                            REG_PORT_ANY, 0);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));                
                    }
                }
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchRxRedirectPktReasonsExtended:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit))) {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_1_TYPEr, REASONSf)) {
                        uint32 raddr;
                        int i;

                        for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_1_TYPEr); i++) {
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_1_TYPEr, 
                                    REG_PORT_ANY, i);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
                    }
                    }
                } else
#endif
                {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_HIr, REASONSf)) {
                        uint32 raddr;
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_HIr, 
                                REG_PORT_ANY, 0);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
                    }
                }
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchRxRedirectHigigPktReasons:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit))) {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_DIRECT_0_TYPEr, REASONSf)) {
                        uint32 raddr;
                        int i;
 
                        for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_DIRECT_0_TYPEr); i++) {
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_DIRECT_0_TYPEr, 
                                    REG_PORT_ANY, i);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
                    }
                    }
                } else
#endif
                {
                    if (SOC_REG_FIELD_VALID(unit,
                            CMIC_PKT_REASON_DIRECTr, REASONSf)) {
                        uint32 raddr;
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_DIRECTr, 
                                REG_PORT_ANY, 0);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
                    }
                }
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchRxRedirectHigigPktReasonsExtended:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit))) {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_DIRECT_1_TYPEr,
                                REASONSf)) {
                        uint32 raddr;
                        int i;

                        for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_DIRECT_1_TYPEr); i++) {
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_DIRECT_1_TYPEr,
                                    REG_PORT_ANY, i);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
                    }
                    }
                } else
#endif
                {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_DIRECT_HIr,
                                REASONSf)) {
                        uint32 raddr;
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_DIRECT_HIr,
                                REG_PORT_ANY, 0);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
                    }
                }
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchRxRedirectTruncatedPktReasons:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit))) {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_MINI_0_TYPEr, 
                                REASONSf)) {
                        uint32 raddr;
                        int i;  

                        for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_MINI_0_TYPEr); i++) {
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_MINI_0_TYPEr,
                                    REG_PORT_ANY, i);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
                    }
                    }
                } else
#endif
                {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_MINIr, 
                                REASONSf)) {
                        uint32 raddr;
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_MINIr,
                                REG_PORT_ANY, 0);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
                    }
                }
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchRxRedirectTruncatedPktReasonsExtended:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit))) {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_MINI_1_TYPEr,
                                REASONSf)) {
                        uint32 raddr;
                        int i;  

                        for (i = 0; i < SOC_REG_NUMELS(unit, CMIC_PKT_REASON_MINI_1_TYPEr); i++) {
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_MINI_1_TYPEr,
                                    REG_PORT_ANY, i);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
                    }
                    }
                } else
#endif
                {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_MINI_HIr,
                                REASONSf)) {
                        uint32 raddr;
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_MINI_HIr,
                                REG_PORT_ANY, 0);
                        SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
                    }
                }
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchRxRedirectPktCos:
#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm) &&
                SOC_REG_IS_VALID(unit, CMIC_PKT_COS_QUEUES_LOr)) {
                uint32 raddr;
                raddr = soc_reg_addr(unit, CMIC_PKT_COS_QUEUES_LOr,
                            REG_PORT_ANY, 0);
                SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
 return BCM_E_NONE;
            } else
#endif
            {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRxRedirectPktCosExtended:
#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm) &&
                SOC_REG_IS_VALID(unit, CMIC_PKT_COS_QUEUES_HIr)) {
                uint32 raddr;
                raddr = soc_reg_addr(unit, CMIC_PKT_COS_QUEUES_HIr,
                            REG_PORT_ANY, 0);
                SOC_IF_ERROR_RETURN(soc_pci_write(unit, raddr, arg));
 return BCM_E_NONE;
            } else
#endif
            {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuLocalMacOui: 
        case bcmSwitchRemoteCpuDestMacOui:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                return _bcm_rcpu_switch_mac_hi_set(unit, type, arg); 
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuTcMappingMacOui:
            if (soc_feature(unit, soc_feature_rcpu_tc_mapping)) {
                uint32 fieldval, regval;
                BCM_IF_ERROR_RETURN(
                    READ_REMOTE_CPU_DA_LSr(unit, &regval));
                fieldval = soc_reg_field_get(unit, REMOTE_CPU_DA_LSr, regval,
                                             DAf);
                fieldval = (arg << 24);
                soc_reg_field_set(unit, REMOTE_CPU_DA_LSr, &regval, DAf, fieldval);
                BCM_IF_ERROR_RETURN(
                    WRITE_REMOTE_CPU_DA_LSr(unit, regval));
                BCM_IF_ERROR_RETURN(
                    READ_REMOTE_CPU_DA_MSr(unit, &regval));
                fieldval = soc_reg_field_get(unit, REMOTE_CPU_DA_MSr, regval,
                                             DAf);
                fieldval = (arg >> 8) & 0xffff;
                soc_reg_field_set(unit, REMOTE_CPU_DA_MSr, &regval, DAf, fieldval);
                BCM_IF_ERROR_RETURN(
                    WRITE_REMOTE_CPU_DA_MSr(unit, regval));
 return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuLocalMacNonOui: 
        case bcmSwitchRemoteCpuDestMacNonOui:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                return _bcm_rcpu_switch_mac_lo_set(unit, type, arg); 
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuTcMappingMacNonOui:
            if (soc_feature(unit, soc_feature_rcpu_tc_mapping)) {
                uint32 fieldval, regval;
                BCM_IF_ERROR_RETURN(
                    READ_REMOTE_CPU_DA_LSr(unit, &regval));
                fieldval = soc_reg_field_get(unit, REMOTE_CPU_DA_LSr, regval,
                                             DAf);
                fieldval |= ((arg << 8) >> 8);
                soc_reg_field_set(unit, REMOTE_CPU_DA_LSr, &regval, DAf, fieldval);
                BCM_IF_ERROR_RETURN(
                    WRITE_REMOTE_CPU_DA_LSr(unit,  regval));
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuEthertype:
        case bcmSwitchRemoteCpuVlan:
        case bcmSwitchRemoteCpuTpid:
        case bcmSwitchRemoteCpuSignature:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                return _bcm_rcpu_switch_vlan_tpid_sig_set(unit, type, arg); 
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuTcMappingEthertype:
            if (soc_feature(unit, soc_feature_rcpu_tc_mapping)) {
                BCM_IF_ERROR_RETURN(WRITE_REMOTE_CPU_LENGTH_TYPEr(unit, arg));
 return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuDestPort:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                return _bcm_rcpu_pipe_bypass_header_set(unit, type, arg);
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuHigigDestPort:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                return _bcm_rcpu_higig_header_set(unit, arg);
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif 
#if defined(BCM_TRIUMPH2_SUPPORT) \
    ||  defined(BCM_ENDURO_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
        case bcmSwitchServiceTpidReplace:
            if (SOC_IS_TRIUMPH2(unit) 
                || SOC_IS_ENDURO(unit) || SOC_IS_TRIDENT(unit)) {
            /*  Register settings requires "0" to enable TPID Replace and
             *  "1" to disable TPID Modifications, for "NOP" SD Action.
             */
                BCM_IF_ERROR_RETURN(
                    soc_reg_field32_modify(unit, EGR_SD_TAG_CONTROLr,
                        REG_PORT_ANY, DO_NOT_MOD_TPID_ENABLEf, (arg) ? 0 : 1));
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break; 

#endif /* TRIUMPH2 | ENDURO | TRIDENT */
#if  defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchCongestionCntag:
        case bcmSwitchCongestionCntagEthertype:
        case bcmSwitchCongestionCnm:
        case bcmSwitchCongestionCnmEthertype:
        case bcmSwitchCongestionNotificationIdHigh:
        case bcmSwitchCongestionNotificationIdQueue:
        case bcmSwitchCongestionUseOuterTpid:
        case bcmSwitchCongestionUseOuterVlan:
        case bcmSwitchCongestionUseOuterPktPri:
        case bcmSwitchCongestionUseOuterCfi:
        case bcmSwitchCongestionUseInnerPktPri:
        case bcmSwitchCongestionUseInnerCfi:
        case bcmSwitchCongestionMissingCntag:
            return _bcm_switch_qcn_set(unit, type, arg);
            break;
#endif /* BCM_TRIDENT_SUPPORT */

        case bcmSwitchLoadBalanceHashSelect:
            {
                soc_reg_t   reg; 

                if (SOC_REG_FIELD_VALID(unit, ING_CONFIG_64r, LBID_RTAGf)){
                    reg = ING_CONFIG_64r;
                } else if (SOC_REG_FIELD_VALID(unit, ING_CONFIGr, LBID_RTAGf)) {
                    reg = ING_CONFIGr;
                } else {
                    return BCM_E_UNAVAIL;
                }              

                return soc_reg_field32_modify(unit, reg, REG_PORT_ANY,
                                              LBID_RTAGf, arg);

            }
            break;
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
        case bcmSwitchTrillEthertype:
             if (soc_feature(unit, soc_feature_trill)) {
                  return bcm_td_trill_ethertype_set(unit, arg);
             }
             break;
             
        case bcmSwitchTrillISISEthertype:
             if (soc_feature(unit, soc_feature_trill)) {
                  return bcm_td_trill_ISIS_ethertype_set(unit, arg);
             }
             break;

        case bcmSwitchTrillISISDestMacOui:
        case bcmSwitchTrillBroadcastDestMacOui:
        case bcmSwitchTrillEndStationDestMacOui:
              if (soc_feature(unit, soc_feature_trill)) {
                   return bcm_td_trill_mac_set(unit, type, arg, 1);
              }
              break;

        case bcmSwitchTrillISISDestMacNonOui:
        case bcmSwitchTrillBroadcastDestMacNonOui:
        case bcmSwitchTrillEndStationDestMacNonOui:
              if (soc_feature(unit, soc_feature_trill)) {
                   return bcm_td_trill_mac_set(unit, type, arg, 0);
              }
             break;

        case bcmSwitchTrillMinTtl:
              if (soc_feature(unit, soc_feature_trill)) {
                   return bcm_td_trill_MinTtl_set(unit, arg);
              }
             break;

        case bcmSwitchTrillTtlCheckEnable:
              if (soc_feature(unit, soc_feature_trill)) {
                   return bcm_td_trill_TtlCheckEnable_set(unit, arg);
              }
             break;

#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */
#ifdef BCM_TRIDENT_SUPPORT
        case bcmSwitchFcoeEtherType:
            if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) {
                rv = _bcm_switch_fcoe_ing_ethertype_set(unit, arg);

#if defined(BCM_TRIDENT2_SUPPORT)
                if ((rv == BCM_E_NONE) && SOC_IS_TRIDENT2(unit)) {
                    /* set egress fcoe ethertype register */
                    return _bcm_switch_fcoe_egr_ethertype_set(unit, arg);
            }
#endif
                return rv;
            }
        break;
#endif
#if defined(BCM_FIREBOLT_SUPPORT) 
#if defined(INCLUDE_L3)
        case bcmSwitchL3TunnelIpV4ModeOnly:
              return soc_tunnel_term_block_size_set (unit, arg);
              break;
        case bcmSwitchL3SrcHitEnable:
            if (SOC_REG_FIELD_VALID(unit, ING_CONFIG_64r, L3SRC_HIT_ENABLEf)) {
                return soc_reg_field32_modify(unit, ING_CONFIG_64r, 
                                              REG_PORT_ANY, L3SRC_HIT_ENABLEf,
                                              (uint32)arg);
            } else if (SOC_REG_FIELD_VALID(unit, ING_CONFIGr, 
                                           L3SRC_HIT_ENABLEf)){
                return soc_reg_field32_modify(unit, ING_CONFIGr, REG_PORT_ANY, 
                                              L3SRC_HIT_ENABLEf, (uint32)arg);
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif /* INCLUDE_L3*/
#endif /* BCM_FIREBOLT_SUPPORT */
        case bcmSwitchL2DstHitEnable:
            if (SOC_REG_FIELD_VALID(unit, ING_CONFIG_64r, L2DST_HIT_ENABLEf)) {
                return soc_reg_field32_modify(unit, ING_CONFIG_64r, 
                                              REG_PORT_ANY, L2DST_HIT_ENABLEf,
                                              (uint32)arg);
            } else if (SOC_REG_FIELD_VALID(unit, ING_CONFIGr, 
                                           L2DST_HIT_ENABLEf)){
                return soc_reg_field32_modify(unit, ING_CONFIGr, REG_PORT_ANY, 
                                              L2DST_HIT_ENABLEf, (uint32)arg);
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchHashDualMoveDepth:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthL2:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_L2X(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthMpls:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_MPLS(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthVlan:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_VLAN(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthEgressVlan:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_EGRESS_VLAN(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
#if defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchHashDualMoveDepthWlanPort:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_WLAN_PORT(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthWlanClient:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_WLAN_CLIENT(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchL2GreProtocolType:  
            /* Set protocol-type for L2-GRE */
            if (soc_feature(unit, soc_feature_l2gre)) {
                   return soc_reg_field32_modify(unit, L2GRE_CONTROLr,
                                                 REG_PORT_ANY,
                                                 ETHERTYPEf, arg);
            } else {
                  return BCM_E_UNAVAIL;
            }
            break;

        case bcmSwitchL2GreVpnIdSizeSet: 
            /* Set bit-size of VPNID within L2-GRE key */
            if (soc_feature(unit, soc_feature_l2gre)) {
                uint32 regval;
                   switch (arg) {
                      case 16:  
                                      regval = 3;
                                      break;
                      case 20:  
                                      regval = 2;
                                      break;
                      case 24:  
                                      regval = 1;
                                      break;
                      case 32:  
                                      regval = 0;
                                      break;
                      default:   
                                      return BCM_E_PARAM;
                   }
                   BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, L2GRE_CONTROLr,
                                                 REG_PORT_ANY,
                                                 GRE_KEY_ENTROPY_SIZEf, regval));
                   BCM_IF_ERROR_RETURN
                     (soc_reg_field32_modify(unit, EGR_L2GRE_CONTROLr,
                                                 REG_PORT_ANY,
                                                 GRE_KEY_ENTROPY_SIZEf, regval));
                   return BCM_E_NONE;
            } else {
                  return BCM_E_UNAVAIL;
            }
            break;
    case bcmSwitchRemoteEncapsulationMode:
        /* We set this mode even if the device doesn't support
         * remote encap natively so that TX packets may use the
         * Higig2 encoding to a remote system that does. */
        SOC_REMOTE_ENCAP_SET(unit, (arg) ? 1 : 0);
        if (soc_feature(unit, soc_feature_remote_encap)) {
            return soc_reg_field32_modify(unit, ING_CONFIG_64r,
                                          REG_PORT_ANY,
                                          REPLICATION_MODEf,
                                          (arg) ? 1 : 0);
        }
        return BCM_E_NONE;
        break;

#endif
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
        case bcmSwitchVxlanUdpDestPortSet:  
            /* Set UDP Dest port for VXLAN */
            if (soc_feature(unit, soc_feature_vxlan)) {
                  return bcm_td2_vxlan_udpDestPort_set(unit, arg);
            } else {
                  return BCM_E_UNAVAIL;
            }
          break;

        case bcmSwitchVxlanEntropyEnable: 
            /* Enable UDP Source Port Hash for VXLAN */
            if (soc_feature(unit, soc_feature_vxlan)) {
                  return bcm_td2_vxlan_udpSourcePort_set(unit, arg);
            } else {
                  return BCM_E_UNAVAIL;
            }
          break;
#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */

#if defined(INCLUDE_L3)
        case bcmSwitchHashDualMoveDepthL3:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    SOC_DUAL_HASH_MOVE_MAX_L3X(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashL3DualLeastFull:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    if (SOC_REG_FIELD_VALID(unit, L3_AUX_HASH_CONTROLr, 
                                            INSERT_LEAST_FULL_HALFf)) {
                        BCM_IF_ERROR_RETURN
                            (soc_reg_field32_modify(unit, L3_AUX_HASH_CONTROLr, 
                                                    REG_PORT_ANY,
                                                    INSERT_LEAST_FULL_HALFf, 
                                                    (arg ? 1 : 0)));
                        return BCM_E_NONE;
                    }
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
#endif
        case bcmSwitchHashMultiMoveDepth:
            if (soc_feature(unit, soc_feature_ism_memory) ||
                soc_feature(unit, soc_feature_shared_hash_mem)) {
                if (arg >= 0) {
                    SOC_MULTI_HASH_MOVE_MAX(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMultiMoveDepthL2:
            if (soc_feature(unit, soc_feature_ism_memory) ||
                soc_feature(unit, soc_feature_shared_hash_mem)) {
                if (arg >= 0) {
                    SOC_MULTI_HASH_MOVE_MAX_L2(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMultiMoveDepthL3:
            if (soc_feature(unit, soc_feature_ism_memory) ||
                soc_feature(unit, soc_feature_shared_hash_mem)) {
                if (arg >= 0) {
                    SOC_MULTI_HASH_MOVE_MAX_L3(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMultiMoveDepthMpls:
            if (soc_feature(unit, soc_feature_ism_memory)) {
                if (arg >= 0) {
                    SOC_MULTI_HASH_MOVE_MAX_MPLS(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMultiMoveDepthVlan:
            if (soc_feature(unit, soc_feature_ism_memory)) {
                if (arg >= 0) {
                    SOC_MULTI_HASH_MOVE_MAX_VLAN(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMultiMoveDepthEgressVlan:
            if (soc_feature(unit, soc_feature_ism_memory)) {
                if (arg >= 0) {
                    SOC_MULTI_HASH_MOVE_MAX_EGRESS_VLAN(unit) = arg;
                    return BCM_E_NONE;
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashL2DualLeastFull:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    if (SOC_REG_FIELD_VALID(unit, L2_AUX_HASH_CONTROLr, 
                                            INSERT_LEAST_FULL_HALFf)) {
                        BCM_IF_ERROR_RETURN
                            (soc_reg_field32_modify(unit, L2_AUX_HASH_CONTROLr, 
                                                    REG_PORT_ANY,
                                                    INSERT_LEAST_FULL_HALFf, 
                                                    (arg ? 1 : 0)));
                        return BCM_E_NONE;
                    }
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMPLSDualLeastFull:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (arg >= 0) {
                    if (SOC_REG_FIELD_VALID(unit, MPLS_ENTRY_HASH_CONTROLr, 
                                            INSERT_LEAST_FULL_HALFf)) {
                        BCM_IF_ERROR_RETURN
                            (soc_reg_field32_modify(unit, MPLS_ENTRY_HASH_CONTROLr, 
                                                    REG_PORT_ANY,
                                                    INSERT_LEAST_FULL_HALFf, 
                                                    (arg ? 1 : 0)));
                        return BCM_E_NONE;
                    }
                } else {
                    return BCM_E_PARAM;
                }
            }
            return BCM_E_UNAVAIL;
            break;
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT) || defined(BCM_ENDURO_SUPPORT) || \
    defined(BCM_HURRICANE_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) || \
    defined(BCM_KATANA_SUPPORT)
        case bcmSwitchSynchronousPortClockSource:
            if (soc_feature(unit, soc_feature_gmii_clkout)) {
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || \
    defined(BCM_KATANA_SUPPORT)
                if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit) || SOC_IS_KATANA(unit)) {
                    return _bcm_switch_sync_port_select_set(unit, (uint32)arg);
                } else 
#endif
                {
                    uint32 mask = soc_reg_field_length(unit, EGR_L1_CLK_RECOVERY_CTRLr, 
                                                       PRI_PORT_SELf);
                    mask = (1 << mask) - 1;
                    return soc_reg_field32_modify(unit, EGR_L1_CLK_RECOVERY_CTRLr,
                                                  REG_PORT_ANY, PRI_PORT_SELf,
                                                  (uint32)arg & mask);
                } 
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchSynchronousPortClockSourceBkup:
            if (soc_feature(unit, soc_feature_gmii_clkout)) {
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || defined(BCM_KATANA_SUPPORT)
                if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit)) {
                    return _bcm_switch_sync_port_select_set(unit, (uint32)arg);
                } else if (SOC_IS_KATANA(unit)) {

                    return _bcm_switch_sync_port_backup_select_set(unit,(uint32)arg);
                } else
#endif
                {
                    uint32 mask = soc_reg_field_length(unit, EGR_L1_CLK_RECOVERY_CTRLr, 
                                                       BKUP_PORT_SELf);
                    mask = (1 << mask) - 1;
                    return soc_reg_field32_modify(unit, EGR_L1_CLK_RECOVERY_CTRLr,
                                                  REG_PORT_ANY, BKUP_PORT_SELf,
                                                  (uint32)arg & mask);
                }
            } else {
                return BCM_E_UNAVAIL;
            }
            break;

    case bcmSwitchSynchronousPortClockSourceDivCtrl:
            if (soc_feature(unit, soc_feature_gmii_clkout)) {
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || defined(BCM_KATANA_SUPPORT)
                if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit) || SOC_IS_KATANA(unit)) {
                   return _bcm_switch_div_ctrl_select_set(unit, (uint32)arg);
            } else {
                   return BCM_E_UNAVAIL;
            }
#else 
            {
                   return BCM_E_UNAVAIL;
            }
#endif
            } else {
                   return BCM_E_UNAVAIL;
            }
            break;

    case bcmSwitchSynchronousPortClockSourceBkupDivCtrl:
           if (soc_feature(unit, soc_feature_gmii_clkout)) {
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || defined(BCM_KATANA_SUPPORT)
              if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit) || SOC_IS_KATANA(unit)) {
                 return _bcm_switch_div_ctrl_backup_select_set(unit, (uint32)arg);
           } else {
                 return BCM_E_UNAVAIL;
           }
#else 
           {
                 return BCM_E_UNAVAIL;
           }
           
#endif  
           } else {
                 return BCM_E_UNAVAIL;
           }
#endif
           break;
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    case bcmSwitchNivEthertype:
         if (soc_feature(unit, soc_feature_niv)) {
              return bcm_trident_niv_ethertype_set(unit, arg);
         }
         return BCM_E_UNAVAIL;
         break;
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    case bcmSwitchEtagEthertype:
         if (soc_feature(unit, soc_feature_port_extension)) {
              return bcm_tr3_etag_ethertype_set(unit, arg);
         }
         return BCM_E_UNAVAIL;
         break;
    case bcmSwitchExtenderMulticastLowerThreshold:
         if (soc_feature(unit, soc_feature_port_extension)) {
             return soc_reg_field32_modify(unit, ETAG_MULTICAST_RANGEr,
                     REG_PORT_ANY, ETAG_VID_MULTICAST_LOWERf, arg);
         }
         return BCM_E_UNAVAIL;
         break;
    case bcmSwitchExtenderMulticastHigherThreshold:
         if (soc_feature(unit, soc_feature_port_extension)) {
             return soc_reg_field32_modify(unit, ETAG_MULTICAST_RANGEr,
                     REG_PORT_ANY, ETAG_VID_MULTICAST_UPPERf, arg);
         }
         return BCM_E_UNAVAIL;
         break;
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    case bcmSwitchWESPProtocolEnable:
        if (soc_feature(unit, soc_feature_wesp)) {
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ING_WESP_PROTO_CONTROLr,
                                        REG_PORT_ANY,
                                        WESP_PROTO_NUMBER_ENABLEf,
                                        arg ? 1 : 0));
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_WESP_PROTO_CONTROLr,
                                        REG_PORT_ANY,
                                        WESP_PROTO_NUMBER_ENABLEf,
                                        arg ? 1 : 0));
            return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;
    case bcmSwitchWESPProtocol:
        if (soc_feature(unit, soc_feature_wesp)) {
            if (arg < 0 || arg > 255) {
                return BCM_E_PARAM;
            }
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ING_WESP_PROTO_CONTROLr,
                                        REG_PORT_ANY, WESP_PROTO_NUMBERf,
                                        arg));
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_WESP_PROTO_CONTROLr,
                                        REG_PORT_ANY, WESP_PROTO_NUMBERf,
                                        arg));
            return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;

#if defined(BCM_TRIDENT_SUPPORT)
    case bcmSwitchIp6CompressEnable:
        if (SOC_REG_FIELD_VALID(unit, ING_MISC_CONFIG2r, 
            IPV6_TO_IPV4_ADDRESS_MAP_ENABLEf)) {
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ING_MISC_CONFIG2r, REG_PORT_ANY,
                    IPV6_TO_IPV4_ADDRESS_MAP_ENABLEf, arg ? 1 : 0));
            return BCM_E_NONE;
        } 
        return BCM_E_UNAVAIL;
        break;
        
    case bcmSwitchIp6CompressDefaultOffset:
        if (SOC_REG_FIELD_VALID(unit, ING_MISC_CONFIG2r, 
            IPV6_TO_IPV4_MAP_OFFSET_DEFAULTf)) {
            if (arg < 0 || arg > 63) {
                return BCM_E_PARAM;
            }
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ING_MISC_CONFIG2r, REG_PORT_ANY,
                    IPV6_TO_IPV4_MAP_OFFSET_DEFAULTf, arg));
            return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;
#endif /* !BCM_TRIDENT_SUPPORT */
    case bcmSwitchMirrorPktChecksEnable:
        if (SOC_REG_FIELD_VALID(unit, EGR_CONFIG_1r,
            DISABLE_MIRROR_CHECKSf)) {
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_CONFIG_1r, REG_PORT_ANY,
                    DISABLE_MIRROR_CHECKSf, arg ? 0 : 1));
            return BCM_E_NONE;
        } 
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchStableSaveLongIds:
#if defined(BCM_FIELD_SUPPORT) && defined(BCM_WARM_BOOT_SUPPORT)
        {
            _field_control_t *fc;

            BCM_IF_ERROR_RETURN(_field_control_get(unit, &fc));
            
            if (arg) {
                fc->flags |= _FP_STABLE_SAVE_LONG_IDS;
            } else {
                fc->flags &= ~_FP_STABLE_SAVE_LONG_IDS;
            }
        }
        return (BCM_E_NONE);
#else
        return (BCM_E_UNAVAIL);
#endif
        break;

#if defined(BCM_TRIUMPH_SUPPORT)
    case bcmSwitchGportAnyDefaultL2Learn:
    case bcmSwitchGportAnyDefaultL2Move:
            if (soc_feature(unit, soc_feature_virtual_switching)) {
                if(!SOC_IS_HURRICANEX(unit)) {
                    return _bcm_switch_default_cml_set(unit, type, arg);
                }
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_TRIUMPH_SUPPORT */

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    case bcmSwitchOamCcmToCpu:
        if (soc_feature(unit, soc_feature_oam)) {
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, CCM_COPYTO_CPU_CONTROLr, REG_PORT_ANY,
                    ERROR_CCM_COPY_TOCPUf, (arg ? 1 : 0)));
            return BCM_E_NONE;
        } 
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchOamXconCcmToCpu:
        if (soc_feature(unit, soc_feature_oam)) {
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, CCM_COPYTO_CPU_CONTROLr, REG_PORT_ANY,
                    XCON_CCM_COPY_TOCPUf, (arg ? 1 : 0)));
            return BCM_E_NONE;
        } 
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchOamXconOtherToCpu:
        if (soc_feature(unit, soc_feature_oam)) {
            if (soc_reg_field_valid(unit, CCM_COPYTO_CPU_CONTROLr, 
                                                  XCON_OTHER_COPY_TOCPUf)) {
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, CCM_COPYTO_CPU_CONTROLr, 
                                      REG_PORT_ANY, 
                                      XCON_OTHER_COPY_TOCPUf, (arg ? 1 : 0)));
            }
            return BCM_E_NONE;
        } 
        return BCM_E_UNAVAIL;
        break;
#endif /* !BCM_TRIUMPH3_SUPPORT*/
#if defined(BCM_KATANA_SUPPORT)
    case bcmSwitchSetMplsEntropyLabelTtl:
        if (soc_feature(unit, soc_feature_mpls_entropy)) {
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_MPLS_ENTROPY_LABEL_CONTROLr,
                                        REG_PORT_ANY, TTLf,
                                        arg));
            return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;
    case bcmSwitchSetMplsEntropyLabelPri:
        if (soc_feature(unit, soc_feature_mpls_entropy)) {
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_MPLS_ENTROPY_LABEL_CONTROLr,
                                        REG_PORT_ANY, TCf,
                                        arg));
            return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchSetMplsEntropyLabelOffset:
        if (soc_feature(unit, soc_feature_mpls_entropy)) {
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_MPLS_ENTROPY_LABEL_CONTROLr,
                                        REG_PORT_ANY, LABEL_OFFSETf,
                                        arg));
            return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;
#endif

#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    case bcmSwitchMultipathCompress:
        if (soc_feature(unit, soc_feature_l3)) {
            return bcm_tr2_l3_ecmp_defragment(unit);
        }
        break;

    case bcmSwitchMultipathCompressBuffer:
        if (soc_feature(unit, soc_feature_l3)) {
            return bcm_tr2_l3_ecmp_defragment_buffer_set(unit, arg);
        }
        break;
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */

    case bcmSwitchBstEnable:
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
            return _bcm_bst_cmn_control_set(unit, bcmSwitchBstEnable, arg);
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchBstTrackingMode:
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
            return _bcm_bst_cmn_control_set(unit, bcmSwitchBstTrackingMode, arg);
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
        return BCM_E_UNAVAIL;
        break;

#ifdef BCM_TRIDENT_SUPPORT
    case bcmSwitchFabricTrunkAutoIncludeDisable:
        if (!soc_feature(unit, soc_feature_modport_map_dest_is_port_or_trunk)) {
            return BCM_E_UNAVAIL;
        }
        return bcm_td_modport_map_mode_set(unit, type, arg);
        break;
#endif /* BCM_TRIDENT_SUPPORT */

    case bcmSwitchL2OverflowEvent:
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TRIDENT2(unit)) {
            if (arg) {
                SOC_CONTROL_LOCK(unit);
                SOC_CONTROL(unit)->l2_overflow_enable = TRUE;
                SOC_CONTROL_UNLOCK(unit);
                return soc_td2_l2_overflow_start(unit);
            } else {
                SOC_CONTROL_LOCK(unit);
                SOC_CONTROL(unit)->l2_overflow_enable = FALSE;
                SOC_CONTROL_UNLOCK(unit);
                return soc_td2_l2_overflow_stop(unit);
            }    
        }else
#endif/*BCM_TRIDENT2_SUPPORT*/   
#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            if (arg) {
                SOC_CONTROL_LOCK(unit);
                SOC_CONTROL(unit)->l2_overflow_enable = TRUE;
                SOC_CONTROL_UNLOCK(unit);
                return soc_tr3_l2_overflow_start(unit);
            } else {
                SOC_CONTROL_LOCK(unit);
                SOC_CONTROL(unit)->l2_overflow_enable = FALSE;
                SOC_CONTROL_UNLOCK(unit);
                return soc_tr3_l2_overflow_stop(unit);
            }    
        }else
#endif /* BCM_TRIUMPH3_SUPPORT */
        {
            return BCM_E_UNAVAIL;
        }
        break;

#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
        case bcmSwitchMiMDefaultSVPValue:
            if (SOC_IS_TRIDENT(unit) || SOC_IS_TRIDENT2(unit) ||
                SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) {
                if (!_bcm_vp_used_get(unit, arg, _bcmVpTypeMim)) {
                    return BCM_E_PARAM;
                }
                SOC_IF_ERROR_RETURN(
                     soc_reg_field32_modify(unit, MIM_DEFAULT_NETWORK_SVPr, 
                                            REG_PORT_ANY, SVPf, arg)); 
                return BCM_E_NONE;
            }
            break;
#endif
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
        case bcmSwitchIpmcSameVlanPruning:
            if (SOC_IS_TRIUMPH3(unit) || SOC_IS_HELIX4(unit) ||
                SOC_IS_TRIDENT2(unit)) {
                BCM_IF_ERROR_RETURN(
                    soc_reg_field32_modify(unit, ING_MISC_CONFIGr, REG_PORT_ANY, 
                                                 IPMC_IND_MODEf, (arg ? 0 : 1)));
                if (SOC_IS_TRIUMPH3(unit) || SOC_IS_HELIX4(unit)) {
                    BCM_IF_ERROR_RETURN(
                        soc_reg_field32_modify(unit, MCQ_CONFIGr, REG_PORT_ANY, 
                                                   IPMC_IND_MODEf, (arg ? 0 : 1)));
                } else if (SOC_IS_TRIDENT2(unit)) {
                    BCM_IF_ERROR_RETURN(
                        soc_reg_field32_modify(unit, TOQ_MC_CFG0r, REG_PORT_ANY,
                                                   IPMC_IND_MODEf, (arg ? 0 : 1)));
                }
                return BCM_E_NONE;
            }
        break;
#endif
#if defined(BCM_KATANA2_SUPPORT)
        case bcmSwitchOamHeaderErrorToCpu:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, CCM_COPYTO_CPU_CONTROLr, 
                                  REG_PORT_ANY,
                                  OAM_HEADER_ERROR_TOCPUf, (arg ? 1 : 0)));
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, EGR_CPU_CONTROLr, 
                                  REG_PORT_ANY,
                                  OAM_HEADER_ERROR_TOCPUf, (arg ? 1 : 0)));
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;
   
        case bcmSwitchOamUnknownVersionToCpu:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, CCM_COPYTO_CPU_CONTROLr, 
                              REG_PORT_ANY,
                              OAM_UNKNOWN_OPCODE_VERSION_TOCPUf, (arg ? 1 : 0)));
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, EGR_CPU_CONTROLr, 
                              REG_PORT_ANY,
                              OAM_UNKNOWN_VERSION_TOCPUf, (arg ? 1 : 0)));
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchOamUnknownVersionDrop:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, OAM_DROP_CONTROLr, 
                        REG_PORT_ANY,
                        IFP_OAM_UNKNOWN_OPCODE_VERSION_DROPf, (arg ? 1 : 0)));
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchOamUnexpectedPktToCpu:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, CCM_COPYTO_CPU_CONTROLr, 
                                    REG_PORT_ANY,
                                    OAM_UNEXPECTED_PKT_TOCPUf, (arg ? 1 : 0)));
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, EGR_CPU_CONTROLr, 
                                    REG_PORT_ANY,
                                    OAM_UNEXPECTED_PKT_TOCPUf, (arg ? 1 : 0)));
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchOamVersionCheckDisable:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, OAM_CONTROL_1r, 
                                    REG_PORT_ANY,
                                    OAM_VER_CHECK_DISABLEf, (arg ? 1 : 0)));
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, EGR_OAM_ERROR_CONTROLr, 
                                    REG_PORT_ANY,
                                    OAM_VER_CHECK_DISABLEf, (arg ? 1 : 0)));
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchOamOlpChipEtherType:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                arg &= _BCM_SWITCH_ETHERTYPE_VLAN_TAG_MASK;
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, IARB_OLP_CONFIGr, 
                                    REG_PORT_ANY, ETHERTYPEf, arg));
                BCM_IF_ERROR_RETURN
                    (soc_mem_field32_modify(unit, EGR_OLP_CONFIGm, 
                                    0, ETHERTYPEf, arg));
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;


        case bcmSwitchOamOlpChipTpid:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                arg &= _BCM_SWITCH_ETHERTYPE_VLAN_TAG_MASK;
                BCM_IF_ERROR_RETURN
                    (soc_mem_field32_modify(unit, EGR_OLP_CONFIGm, 
                                    0, VLAN_TPIDf, arg));
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;


        case bcmSwitchOamOlpChipVlan: 
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                arg &= _BCM_SWITCH_ETHERTYPE_VLAN_TAG_MASK;
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, EGR_OLP_VLANr, 
                                    REG_PORT_ANY, VLAN_TAGf, arg));
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_KATANA2_SUPPORT */
        case bcmSwitchFieldStageEgressToCpu:
            if (soc_feature(unit, soc_feature_field_egress_tocpu)) {
                SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, EGR_CPU_CONTROLr,
                                         REG_PORT_ANY, EFP_TOCPUf, arg));
               return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
#if defined(BCM_KATANA2_SUPPORT)
        case bcmSwitchHashOamEgress:
        case bcmSwitchHashOamEgressDual:
            if (SOC_IS_KATANA2(unit)) {
                return _bcm_fb_er_hashselect_set(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_ENDURO_SUPPORT)
        case bcmSwitchWredForAllPkts:
            if(SOC_IS_ENDURO(unit)) {
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, ING_MISC_CONFIGr,
                         REG_PORT_ANY, TREAT_ALL_PKTS_AS_TCPf, arg ? 1 : 0));
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_ENDURO_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)

    case bcmSwitchFcoeNpvModeEnable:
        if (soc_feature(unit, soc_feature_fcoe)) {
            return soc_reg_field32_modify(unit, FCOE_ING_CONTROLr,
                                          REG_PORT_ANY, FCOE_NPV_MODEf, 
                                          (arg) ? 1 : 0);
        }
        break;

    case bcmSwitchFcoeDomainRoutePrefixLength:
        if (soc_feature(unit, soc_feature_fcoe)) {

            /* 5 bits value but only 1-24 is valid */
            if ((arg >= 1) && (arg <= 24)) {
                return soc_reg_field32_modify(unit, FCOE_ING_CONTROLr,
                                              REG_PORT_ANY, 
                                              FCOE_DOMAIN_ROUTE_PREFIXf,
                                              arg);
            } else {
                return BCM_E_PARAM;
            }
        }
        break;

    case bcmSwitchFcoeCutThroughEnable:
        if (soc_feature(unit, soc_feature_fcoe)) {
            return soc_reg_field32_modify(unit, FCOE_ING_CONTROLr,
                                          REG_PORT_ANY, 
                                          FCOE_CUT_THROUGH_ENABLEf, 
                                          (arg) ? 1 : 0);
        }
        break;

    case bcmSwitchFcoeSourceBindCheckAction:
        if (soc_feature(unit, soc_feature_fcoe)) {
            return soc_reg_field32_modify(unit, FCOE_ING_CONTROLr,
                                          REG_PORT_ANY, 
                                          FCOE_SRC_BIND_CHECK_ACTIONf, 
                                          (arg) ? 1 : 0);
        }
        break;

    case bcmSwitchFcoeSourceFpmaPrefixCheckAction:
        if (soc_feature(unit, soc_feature_fcoe)) {
            return soc_reg_field32_modify(
                                    unit, FCOE_ING_CONTROLr,
                                    REG_PORT_ANY,
                                    FCOE_SRC_FPMA_PREFIX_CHECK_ACTIONf, 
                                    (arg) ? 1 : 0);
        }
        break;

    case bcmSwitchFcoeVftHopCountExpiryToCpu:
        if (soc_feature(unit, soc_feature_fcoe)) {
            return soc_reg_field32_modify(unit, CPU_CONTROL_0r,
                                          REG_PORT_ANY,
                                          FCOE_VFT_HOPCOUNT_TOCPUf, 
                                          (arg) ? 1 : 0);
        }
        break;

    case bcmSwitchFcoeVftHopCountExpiryAction:
        if (soc_feature(unit, soc_feature_fcoe)) {
            BCM_IF_ERROR_RETURN(
                    soc_mem_field32_modify(unit, EGR_FCOE_CONTROL_1m, 0,
                                           FCOE_VFT_HOPCOUNT_EXPIRE_CONTROLf,
                                           (arg) ? 1 : 0));
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofT1Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            BCM_IF_ERROR_RETURN(
                    soc_mem_field32_modify(unit, EGR_FCOE_CONTROL_1m, 0,
                                           FCOE_FC_EOF_T_1f, arg));
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofT2Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            BCM_IF_ERROR_RETURN(
                    soc_mem_field32_modify(unit, EGR_FCOE_CONTROL_1m, 0,
                                           FCOE_FC_EOF_T_2f, arg));
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofA1Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            BCM_IF_ERROR_RETURN(
                    soc_mem_field32_modify(unit, EGR_FCOE_CONTROL_1m, 0,
                                           FCOE_FC_EOF_A_1f, arg));
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofA2Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            BCM_IF_ERROR_RETURN(
                    soc_mem_field32_modify(unit, EGR_FCOE_CONTROL_1m, 0,
                                           FCOE_FC_EOF_A_2f, arg));
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofN1Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            BCM_IF_ERROR_RETURN(
                    soc_mem_field32_modify(unit, EGR_FCOE_CONTROL_1m, 0,
                                           FCOE_FC_EOF_N_1f, arg));
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofN2Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            BCM_IF_ERROR_RETURN(
                    soc_mem_field32_modify(unit, EGR_FCOE_CONTROL_1m, 0,
                                           FCOE_FC_EOF_N_2f, arg));
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofNI1Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            BCM_IF_ERROR_RETURN(
                    soc_mem_field32_modify(unit, EGR_FCOE_CONTROL_1m, 0,
                                           FCOE_FC_EOF_NI_1f, arg));
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofNI2Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            BCM_IF_ERROR_RETURN(
                    soc_mem_field32_modify(unit, EGR_FCOE_CONTROL_1m, 0,
                                           FCOE_FC_EOF_NI_2f, arg));
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeZoneCheckFailToCpu:
        if (soc_feature(unit, soc_feature_fcoe)) {
            return soc_reg_field32_modify(unit, CPU_CONTROL_Mr,
                                          REG_PORT_ANY,
                                          FCOE_ZONE_CHECK_FAIL_COPY_TOCPUf,
                                          (arg) ? 1 : 0);
        }
        break;

    case bcmSwitchFcoeZoneCheckMissDrop:
        if (soc_feature(unit, soc_feature_fcoe)) {
            return soc_reg_field32_modify(unit, ING_MISC_CONFIG2r,
                                          REG_PORT_ANY,
                                          FCOE_ZONE_CHECK_MISS_DROPf,
                                          (arg) ? 1 : 0);
        }
        break;

#endif  /* BCM_TRIDENT2_SUPPORT */
    case bcmSwitchUnknownSubportPktTagToCpu:
        if (soc_feature(unit, soc_feature_subtag_coe)) {
            return soc_reg_field32_modify(unit, CPU_CONTROL_1r,
                                          REG_PORT_ANY,
                                          UNKNOWN_SUBTENDING_PORT_TOCPUf,
                                          (arg) ? 1 : 0);
        }
        break;
    default:
        break;
    }

    /* Iterate over all ports for port specific controls */
    found = 0;

    PBMP_E_ITER(unit, port) {
        if (SOC_IS_STACK_PORT(unit, port)) {
            continue;
        }
        /* coverity[stack_use_callee] */
        /* coverity[stack_use_overflow] */
        rv = bcm_esw_switch_control_port_set(unit, port, type, arg);
        if (rv == BCM_E_NONE) {
          found = 1;
        } else if (rv != BCM_E_UNAVAIL) {
            return rv;
        }
    }

    PBMP_HG_ITER(unit, port) {
        if (SOC_IS_STACK_PORT(unit, port) &&
            (type != bcmSwitchDirectedMirroring)) {
            continue;
        }
        rv = bcm_esw_switch_control_port_set(unit, port, type, arg);
        if (rv == BCM_E_NONE) {
           found = 1;
        } else if (rv != BCM_E_UNAVAIL) {
            return rv;
        }
    }

    return (found ? BCM_E_NONE : BCM_E_UNAVAIL);
}

/*
 * Function:
 *      bcm_switch_control_get
 * Description:
 *      Retrieve general switch behaviors.
 * Parameters:
 *      unit - Device unit number
 *      type - The desired configuration parameter to retrieve.
 *      arg - Pointer to where the retrieved value will be written.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      For behaviors which are port-specific, retrieves the setting
 *      for one port arbitrarily chosen from those which support the
 *      setting and are non-stack.
 */

int
bcm_esw_switch_control_get(int unit,
                           bcm_switch_control_t type,
                           int *arg)
{
    bcm_port_t  port;
    int   rv;
    uint32 rval;
    uint64 rval64;

#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        return BCM_E_UNAVAIL;
    }
#endif /* BCM_SHADOW_SUPPORT */
    /* Check for device wide, non port specific controls */
    switch (type) {
    case bcmSwitchStableSelect:
#ifdef BCM_WARM_BOOT_SUPPORT
        {
            uint32 flags;
            return soc_stable_get(unit, arg, &flags);
        }
#else
        return SOC_E_UNAVAIL;
#endif
        break;
    case bcmSwitchStableSize:
#ifdef BCM_WARM_BOOT_SUPPORT
        return soc_stable_size_get(unit, arg);
#else
        return SOC_E_UNAVAIL;
#endif
        break;
    case bcmSwitchStableUsed:
#ifdef BCM_WARM_BOOT_SUPPORT
        return soc_stable_used_get(unit, arg);
#else
        return SOC_E_UNAVAIL;
#endif
        break;
    case bcmSwitchControlSync:
#ifdef BCM_WARM_BOOT_SUPPORT
        *arg = SOC_CONTROL(unit)->scache_dirty;
        return BCM_E_NONE;
#else
        return SOC_E_UNAVAIL;
#endif
        break;
    case bcmSwitchControlAutoSync:
#ifdef BCM_WARM_BOOT_SUPPORT
        *arg = SOC_CONTROL(unit)->autosync;
        return BCM_E_NONE;
#else
        return SOC_E_UNAVAIL;
#endif
        break;
    case bcmSwitchModuleType: {
        bcm_module_t modid;
        uint32 mod_type;

        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &modid));
        BCM_IF_ERROR_RETURN(_bcm_switch_module_type_get(unit, modid,
                                                           &mod_type));
        *arg = BCM_SWITCH_CONTROL_MOD_TYPE_PACK(modid, mod_type);
        return BCM_E_NONE;
    }
    break;
#ifdef BCM_CB_ABORT_ON_ERR
    case bcmSwitchCallbackAbortOnError:
        *arg = SOC_CB_ABORT_ON_ERR(unit);
        return BCM_E_NONE;
        break;
#endif
    case bcmSwitchIpmcGroupMtu:
        if (soc_feature(unit, soc_feature_ipmc_group_mtu)) {
            uint32 val;

            rv = READ_IPMC_L3_MTUr(unit, 0, &val);
            if (rv >= 0) {
                *arg = soc_reg_field_get(unit, IPMC_L3_MTUr, val, MTU_LENf);
            }
            return rv;
        }
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchMcastFloodDefault: {
        bcm_vlan_mcast_flood_t mode;
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_flood_default_get(unit, &mode));
        *arg = mode;
        return BCM_E_NONE;
    }
    break;

    case bcmSwitchHgHdrMcastFlood: {
        bcm_vlan_mcast_flood_t mode;
        BCM_IF_ERROR_RETURN(_bcm_esw_higig_flood_l2_get(unit, &mode));
        *arg = mode;
        return BCM_E_NONE;
    }
    break;

    case bcmSwitchHgHdrIpMcastFlood: {
        return BCM_E_UNAVAIL;
    }
    break;

    case bcmSwitchUseGport:
        *arg = SOC_USE_GPORT(unit);
        return BCM_E_NONE;
        break;

    case bcmSwitchFailoverStackTrunk:
        *arg = (SOC_IS_XGS3_SWITCH(unit) || SOC_IS_XGS3_FABRIC(unit)) ? 1 : 0;
        return BCM_E_NONE;
        break;

    case bcmSwitchFailoverEtherTrunk:
        *arg = 0;
        return BCM_E_NONE;
        break;

    case bcmSwitchMcastL2Range:
#ifdef BCM_BRADLEY_SUPPORT
        if (SOC_IS_HBX(unit) || SOC_IS_SC_CQ(unit)) {
            int mc_base, mc_size;
            BCM_IF_ERROR_RETURN
                (soc_hbx_mcast_size_get(unit, &mc_base, &mc_size));
            *arg = mc_size;
            return BCM_E_NONE;
        }
#endif
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchMcastL3Range:
#ifdef BCM_BRADLEY_SUPPORT
        if (SOC_IS_HBX(unit) || SOC_IS_SC_CQ(unit)) {
            int ipmc_base, ipmc_size;
            BCM_IF_ERROR_RETURN
                (soc_hbx_ipmc_size_get(unit, &ipmc_base, &ipmc_size));
            *arg = ipmc_size;
            return BCM_E_NONE;
        }
#endif
        return BCM_E_UNAVAIL;
        break;
    /*    coverity[equality_cond]    */

    case bcmSwitchHgHdrMcastVlanRange:
    case bcmSwitchHgHdrMcastL2Range:
    case bcmSwitchHgHdrMcastL3Range:
#ifdef BCM_BRADLEY_SUPPORT
        if (SOC_IS_HBX(unit) || SOC_IS_TRX(unit)) {
            int bc_size, mc_size, ipmc_size;
            BCM_IF_ERROR_RETURN
                (soc_hbx_higig2_mcast_sizes_get(unit, &bc_size,
                                                &mc_size, &ipmc_size));
            switch (type) {
            case bcmSwitchHgHdrMcastVlanRange:
                *arg = bc_size;
                break;
            case bcmSwitchHgHdrMcastL2Range:
                *arg = mc_size;
                break;
            case bcmSwitchHgHdrMcastL3Range:
                *arg = ipmc_size;
                break;
            /* Defensive Default */
            /* coverity[dead_error_begin] */
            default:
                return BCM_E_PARAM;
            }
            return BCM_E_NONE;
        }
#endif
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchHashSeed0:
#ifdef BCM_BRADLEY_SUPPORT
        if ((SOC_IS_HBX(unit) || SOC_IS_TRX(unit)) && (!(SOC_IS_HURRICANEX(unit)))) {
            rval = 0;
            SOC_IF_ERROR_RETURN(READ_RTAG7_HASH_SEED_Ar(unit, &rval));
            *arg = soc_reg_field_get(unit, RTAG7_HASH_SEED_Ar, rval,
                                     HASH_SEED_Af);
            return BCM_E_NONE;
        }
#endif /* BCM_BRADLEY_SUPPORT */
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchHashSeed1:
#if defined(BCM_BRADLEY_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || \
    defined(BCM_TRIDENT_SUPPORT)
        if ((SOC_IS_HBX(unit) || SOC_IS_TD_TT(unit) || SOC_IS_TRX(unit) ||
            SOC_IS_TRIUMPH3(unit)) && 
            (!(SOC_IS_HURRICANEX(unit)))) {
            rval = 0;
            SOC_IF_ERROR_RETURN(READ_RTAG7_HASH_SEED_Br(unit, &rval));
            *arg = soc_reg_field_get(unit, RTAG7_HASH_SEED_Br, rval,
                                     HASH_SEED_Bf);
            return BCM_E_NONE;
        }
#endif /* BCM_BRADLEY_SUPPORT */
        return BCM_E_UNAVAIL;
        break;

#ifdef BCM_TRIDENT_SUPPORT
        case bcmSwitchMacroFlowHashMinOffset:
            return _bcm_td_macroflow_param_get(unit, NULL, arg, NULL, NULL);
            break;
        case bcmSwitchMacroFlowHashMaxOffset:
            return _bcm_td_macroflow_param_get(unit, NULL, NULL, arg, NULL);
            break;
        case bcmSwitchMacroFlowHashStrideOffset:
            return _bcm_td_macroflow_param_get(unit, NULL, NULL, NULL, arg);
            break;
#endif /* BCM_TRIDENT_SUPPORT */

        /* RTAG7 Macro Flow Concatenation */
        case bcmSwitchMacroFlowEcmpHashConcatEnable:
        case bcmSwitchMacroFlowLoadBalanceHashConcatEnable:
        case bcmSwitchMacroFlowTrunkHashConcatEnable:
        case bcmSwitchMacroFlowHigigTrunkHashConcatEnable:
        /* RTAG7 Macro Flow Hash Seed */
        case bcmSwitchMacroFlowECMPHashSeed:
        case bcmSwitchMacroFlowLoadBalanceHashSeed:
        case bcmSwitchMacroFlowTrunkHashSeed:
        case bcmSwitchMacroFlowHigigTrunkHashSeed:
#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
            if (SOC_IS_TRIDENT2(unit) || SOC_IS_TRIUMPH3(unit)) {
                return _bcm_td2_macroflow_hash_get(unit, type, arg);
            }
#endif /* BCM_TRIDENT2_SUPPORT */
            return BCM_E_UNAVAIL;
            break;

        /* Hash Field select controls */ 
        case bcmSwitchHashTrillPayloadSelect0:
        case bcmSwitchHashTrillPayloadSelect1:
        case bcmSwitchHashTrillTunnelSelect0:
        case bcmSwitchHashTrillTunnelSelect1:
#if defined(BCM_TRIDENT_SUPPORT)
            if ((SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) &&
                soc_feature(unit, soc_feature_trill)) {
                return _bcm_xgs3_field_control_get(unit, type, arg);
            }
#endif /* BCM_TRIDENT_SUPPORT */
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchHashIP6AddrCollapseSelect0:
        case bcmSwitchHashIP6AddrCollapseSelect1: 
#if defined(BCM_TRIDENT_SUPPORT)
            if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit) || 
                SOC_IS_KATANAX(unit)) {
                return _bcm_xgs3_field_control_get(unit, type, arg);
            }
#endif /* BCM_TRIDENT_SUPPORT */
            return BCM_E_UNAVAIL;
            break;

    case bcmSwitchL2McastAllRouterPortsAddEnable:
        *arg = SOC_MCAST_ADD_ALL_ROUTER_PORTS(unit);
        return BCM_E_NONE;
        break;

    case bcmSwitchBypassMode:
        *arg = SOC_SWITCH_BYPASS_MODE(unit);
        return BCM_E_NONE;
        break;

    case bcmSwitchPFCClass0Queue:
    case bcmSwitchPFCClass1Queue:
    case bcmSwitchPFCClass2Queue:
    case bcmSwitchPFCClass3Queue:
    case bcmSwitchPFCClass4Queue:
    case bcmSwitchPFCClass5Queue:
    case bcmSwitchPFCClass6Queue:
    case bcmSwitchPFCClass7Queue:
#if defined(BCM_SCORPION_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT)
        if (SOC_IS_SC_CQ(unit) || SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
            SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit)) {
            return _bcm_tr2_sc_pfc_priority_to_cos_get(unit, type, arg);
        }
#endif /* BCM_SCORPION_SUPPORT || BCM_TRIUMPH2_SUPPORT */
       return BCM_E_UNAVAIL;
       break;

    case bcmSwitchL2HitClear:
    case bcmSwitchL2SrcHitClear:
    case bcmSwitchL2DstHitClear:
    case bcmSwitchL3HostHitClear:
    case bcmSwitchL3RouteHitClear:
       return _bcm_esw_switch_hit_clear_get(unit, type, arg);
       break;

    /* This swithc control should be supported only per port. */
    case bcmSwitchEncapErrorToCpu:
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchSnapNonZeroOui:
#if defined(BCM_TRX_SUPPORT)
        if (SOC_IS_TRX(unit)) {
            rval = 0;

            COMPILER_64_ZERO(rval64);
            SOC_IF_ERROR_RETURN(READ_ING_CONFIG_64r(unit, &rval64));
            *arg = soc_reg64_field32_get(unit, ING_CONFIG_64r, rval64,
                                         SNAP_OTHER_DECODE_ENABLEf);
            SOC_IF_ERROR_RETURN(READ_EGR_CONFIGr(unit, &rval));
            *arg = soc_reg_field_get(unit, EGR_CONFIGr, rval,
                                     SNAP_OTHER_DECODE_ENABLEf);
            return BCM_E_NONE;
        } else
#endif /* BCM_TRX_SUPPORT */
#if defined(BCM_FIREBOLT_SUPPORT)
        if (SOC_IS_FB(unit) || SOC_IS_FX_HX(unit) || SOC_IS_HB_GW(unit)) {
            rval = 0;
            SOC_IF_ERROR_RETURN(READ_ING_CONFIGr(unit, &rval));
            *arg = soc_reg_field_get(unit, ING_CONFIGr, rval,
                                     SNAP_OTHER_DECODE_ENABLEf);
            if (SOC_IS_FIREBOLT2(unit)) {
                SOC_IF_ERROR_RETURN(READ_EGR_CONFIGr(unit, &rval));
                *arg = soc_reg_field_get(unit, EGR_CONFIGr, rval,
                                         SNAP_OTHER_DECODE_ENABLEf);
            }
            return BCM_E_NONE;
        }
#endif /* BCM_FIREBOLT_SUPPORT */
        break;

    case bcmSwitchL2McIdxRetType:
        return _bcm_esw_mcast_idx_ret_type_get(unit, arg);
        break;

    case bcmSwitchL3McIdxRetType:
#ifdef INCLUDE_L3
        return _bcm_esw_ipmc_idx_ret_type_get(unit, arg);
#endif
        return BCM_E_UNAVAIL;
        break;
    case bcmSwitchL2SourceDiscardMoveToCpu:
        {
            soc_reg_t   reg;
            uint64      regval, f_val;

            if (SOC_REG_IS_VALID(unit, ING_CONFIGr)){
                reg = ING_CONFIGr;
            } else if (SOC_REG_IS_VALID(unit, ING_CONFIG_64r)) {
                reg = ING_CONFIG_64r;
            } else {
                return BCM_E_UNAVAIL;
            }

            if (soc_reg_field_valid(unit, reg, STNMOVE_ON_L2SRC_DISCf)) {
                BCM_IF_ERROR_RETURN(
                    soc_reg_get(unit, reg, REG_PORT_ANY, 0, &regval));
                f_val = soc_reg64_field_get(unit, reg, regval, 
                                         STNMOVE_ON_L2SRC_DISCf);
                COMPILER_64_TO_32_LO(*arg, f_val);
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
        }
        break;

#ifdef BCM_TRIDENT_SUPPORT
    case bcmSwitchFabricTrunkDynamicSampleRate:
    case bcmSwitchFabricTrunkDynamicEgressBytesExponent:
    case bcmSwitchFabricTrunkDynamicQueuedBytesExponent:
    case bcmSwitchFabricTrunkDynamicEgressBytesDecreaseReset:
    case bcmSwitchFabricTrunkDynamicQueuedBytesDecreaseReset:
    case bcmSwitchFabricTrunkDynamicEgressBytesMinThreshold:
    case bcmSwitchFabricTrunkDynamicEgressBytesMaxThreshold:
    case bcmSwitchFabricTrunkDynamicQueuedBytesMinThreshold:
    case bcmSwitchFabricTrunkDynamicQueuedBytesMaxThreshold:
        if (!soc_feature(unit, soc_feature_hg_dlb)) {
            return BCM_E_UNAVAIL;
        }
        return _bcm_trident_hg_dlb_config_get(unit, type, arg);
        break;
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
    case bcmSwitchTrunkDynamicSampleRate:
    case bcmSwitchTrunkDynamicAccountingSelect:
    case bcmSwitchTrunkDynamicEgressBytesExponent:
    case bcmSwitchTrunkDynamicQueuedBytesExponent:
    case bcmSwitchTrunkDynamicEgressBytesDecreaseReset:
    case bcmSwitchTrunkDynamicQueuedBytesDecreaseReset:
    case bcmSwitchTrunkDynamicEgressBytesMinThreshold:
    case bcmSwitchTrunkDynamicEgressBytesMaxThreshold:
    case bcmSwitchTrunkDynamicQueuedBytesMinThreshold:
    case bcmSwitchTrunkDynamicQueuedBytesMaxThreshold:
    case bcmSwitchTrunkDynamicExpectedLoadMinThreshold:
    case bcmSwitchTrunkDynamicExpectedLoadMaxThreshold:
    case bcmSwitchTrunkDynamicImbalanceMinThreshold:
    case bcmSwitchTrunkDynamicImbalanceMaxThreshold:
        if (!soc_feature(unit, soc_feature_lag_dlb)) {
            return BCM_E_UNAVAIL;
        }
        return bcm_tr3_lag_dlb_config_get(unit, type, arg);
        break;
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    case bcmSwitchEcmpDynamicSampleRate:
    case bcmSwitchEcmpDynamicAccountingSelect:
    case bcmSwitchEcmpDynamicEgressBytesExponent:
    case bcmSwitchEcmpDynamicQueuedBytesExponent:
    case bcmSwitchEcmpDynamicEgressBytesDecreaseReset:
    case bcmSwitchEcmpDynamicQueuedBytesDecreaseReset:
    case bcmSwitchEcmpDynamicEgressBytesMinThreshold:
    case bcmSwitchEcmpDynamicEgressBytesMaxThreshold:
    case bcmSwitchEcmpDynamicQueuedBytesMinThreshold:
    case bcmSwitchEcmpDynamicQueuedBytesMaxThreshold:
    case bcmSwitchEcmpDynamicExpectedLoadMinThreshold:
    case bcmSwitchEcmpDynamicExpectedLoadMaxThreshold:
    case bcmSwitchEcmpDynamicImbalanceMinThreshold:
    case bcmSwitchEcmpDynamicImbalanceMaxThreshold:
        if (!soc_feature(unit, soc_feature_ecmp_dlb)) {
            return BCM_E_UNAVAIL;
        }
        return _bcm_tr3_ecmp_dlb_config_get(unit, type, arg);
        break;
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

#if (defined(BCM_RCPU_SUPPORT) || defined(BCM_OOB_RCPU_SUPPORT)) && \
     defined(BCM_XGS3_SWITCH_SUPPORT)
        case bcmSwitchRemoteCpuSchanEnable:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                rval = 0;
                
                SOC_IF_ERROR_RETURN(READ_CMIC_PKT_CTRLr(unit, &rval));
                *arg = soc_reg_field_get(unit, CMIC_PKT_CTRLr, rval, 
                                         ENABLE_SCHAN_REQUESTf);
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuMatchLocalMac:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                rval = 0;
                
                SOC_IF_ERROR_RETURN(READ_CMIC_PKT_CTRLr(unit, &rval));
                *arg = soc_reg_field_get(unit, CMIC_PKT_CTRLr, rval, 
                                         LMAC0_MATCHf);
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuFromCpuEnable:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                rval = 0;
                
                SOC_IF_ERROR_RETURN(READ_CMIC_PKT_CTRLr(unit, &rval));
                *arg = soc_reg_field_get(unit, CMIC_PKT_CTRLr, rval, 
                                         ENABLE_FROMCPU_PACKETf);
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuToCpuEnable:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                rval = 0;
                
                SOC_IF_ERROR_RETURN(READ_CMIC_PKT_CTRLr(unit, &rval));
                *arg = soc_reg_field_get(unit, CMIC_PKT_CTRLr, rval, 
                                         ENABLE_TOCPU_PACKETf);
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuCmicEnable:
#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm)) {
                rval = 0;

                SOC_IF_ERROR_RETURN(READ_CMIC_PKT_CTRLr(unit, &rval));
                *arg = soc_reg_field_get(unit, CMIC_PKT_CTRLr, rval, 
                                         ENABLE_CMIC_REQUESTf);
                return BCM_E_NONE;
            } else
#endif
            {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuMatchVlan:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                rval = 0;
                
                SOC_IF_ERROR_RETURN(READ_CMIC_PKT_CTRLr(unit, &rval));
                *arg = soc_reg_field_get(unit, CMIC_PKT_CTRLr, rval, 
                                         VLAN_MATCHf);
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuForceScheduling:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                int l_arg, h_arg;

                l_arg = h_arg = 0;
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)) {
                    BCM_IF_ERROR_RETURN(
                        _bcm_rcpu_switch_regall_get(unit, CMIC_PKT_COS_0r, &l_arg));
                    if (SOC_REG_IS_VALID(unit, CMIC_PKT_COS_1r)) {
                        BCM_IF_ERROR_RETURN(
                          _bcm_rcpu_switch_regall_get(unit, CMIC_PKT_COS_1r, &h_arg));
                    }
                } else
#endif
                {
                    BCM_IF_ERROR_RETURN(
                        _bcm_rcpu_switch_regall_get(unit, CMIC_PKT_COSr, &l_arg));
                    if (SOC_REG_IS_VALID(unit, CMIC_PKT_COS_HIr)) {
                        BCM_IF_ERROR_RETURN(
                          _bcm_rcpu_switch_regall_get(unit, CMIC_PKT_COS_HIr, &h_arg));
                    }
                }
                *arg = l_arg | h_arg;

                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuToCpuDestPortAllReasons:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                int l_arg, h_arg;

                l_arg = h_arg = 0;
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit))) {
                    /* In Katana only Index 0 of the reasoning code register is used */
                    BCM_IF_ERROR_RETURN(
                        _bcm_rcpu_switch_regall_idx_get(unit, CMIC_PKT_REASON_DIRECT_0_TYPEr, 0,
                                                    &l_arg));
                    if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASON_DIRECT_1_TYPEr)) {
                        BCM_IF_ERROR_RETURN(
                          _bcm_rcpu_switch_regall_idx_get(unit, 
                                                 CMIC_PKT_REASON_DIRECT_1_TYPEr, 0, &h_arg));
                    }
                } else
#endif
                {
                   if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASON_DIRECTr)) {
                       BCM_IF_ERROR_RETURN(
                          _bcm_rcpu_switch_regall_get(unit, CMIC_PKT_REASON_DIRECTr, 
                                                    &l_arg));
                    }
                    if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASON_DIRECT_HIr)) {
                        BCM_IF_ERROR_RETURN(
                          _bcm_rcpu_switch_regall_get(unit, 
                                                 CMIC_PKT_REASON_DIRECT_HIr, &h_arg));
                    }
                }
                *arg = l_arg | h_arg;
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuToCpuDestMacAllReasons:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                int l_arg, h_arg;

                l_arg = h_arg = 0;
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit))) {
                    /* In Katana only Index 0 of the reasoning code register is used */
                    BCM_IF_ERROR_RETURN(
                        _bcm_rcpu_switch_regall_idx_get(unit, CMIC_PKT_REASON_0_TYPEr, 0,
                                                    &l_arg));
                    if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASON_1_TYPEr)) {
                        BCM_IF_ERROR_RETURN(
                          _bcm_rcpu_switch_regall_idx_get(unit, 
                                                 CMIC_PKT_REASON_1_TYPEr, 0, &h_arg));
                    }
                } else
#endif
                {
                   if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASONr)) {
                       BCM_IF_ERROR_RETURN(
                          _bcm_rcpu_switch_regall_get(unit, CMIC_PKT_REASONr, &l_arg));
                    }
                    if (SOC_REG_IS_VALID(unit, CMIC_PKT_REASON_HIr)) {
                        BCM_IF_ERROR_RETURN(
                          _bcm_rcpu_switch_regall_get(unit, 
                                                      CMIC_PKT_REASON_HIr, &h_arg));
                    }
                }
                *arg = l_arg | h_arg;
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRxRedirectPktReasons:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                uint32 raddr;
                rval = 0;
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit) || SOC_IS_HURRICANE2(unit))) {
                    /* Only Idx 0 is used in Katana */
                    raddr = soc_reg_addr(unit, CMIC_PKT_REASON_0_TYPEr, REG_PORT_ANY, 0);
                } else
#endif
                {
                    raddr = soc_reg_addr(unit, CMIC_PKT_REASONr, REG_PORT_ANY, 0);
                }

                SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
                *arg = rval;
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRxRedirectPktReasonsExtended:
                if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                    if (soc_feature(unit, soc_feature_cmicm)
                        && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                            SOC_IS_TD2_TT2(unit))) {
                        if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_1_TYPEr, REASONSf)) {
                            uint32 raddr;
                            rval = 0;
                            raddr = soc_reg_addr(unit, CMIC_PKT_REASON_1_TYPEr, REG_PORT_ANY, 0);
                            SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
                            *arg = rval;
                            return BCM_E_NONE;
                        }
                    } else
#endif
                    {
                        if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_HIr, REASONSf)) {
                            uint32 raddr;
                            rval = 0;
                            raddr = soc_reg_addr(unit, CMIC_PKT_REASON_HIr, REG_PORT_ANY, 0);
                            SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
                            *arg = rval;
                            return BCM_E_NONE;
                        }
                    }
                } 
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchRxRedirectHigigPktReasons:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                uint32 raddr;
                rval = 0;
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit) || SOC_IS_HURRICANE2(unit))) {
                    raddr = soc_reg_addr(unit, CMIC_PKT_REASON_DIRECT_0_TYPEr, REG_PORT_ANY, 0);
                } else                
#endif
                {
                    raddr = soc_reg_addr(unit, CMIC_PKT_REASON_DIRECTr, REG_PORT_ANY, 0);
                }
                SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
                *arg = rval;
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRxRedirectHigigPktReasonsExtended:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit) || SOC_IS_HURRICANE2(unit))) {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_DIRECT_1_TYPEr, REASONSf)) {
                        uint32 raddr;
                        rval = 0;
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_DIRECT_1_TYPEr, REG_PORT_ANY, 0);
                        SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
                        *arg = rval;
                        return BCM_E_NONE;
                    }
                } else                
#endif
                {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_DIRECT_HIr, REASONSf)) {
                        uint32 raddr;
                        rval = 0;
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_DIRECT_HIr, REG_PORT_ANY, 0);
                        SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
                        *arg = rval;
                        return BCM_E_NONE;
                    }
                }
            } 
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchRxRedirectTruncatedPktReasons:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                uint32 raddr;
                rval = 0;
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit) || SOC_IS_HURRICANE2(unit))) {
                    raddr = soc_reg_addr(unit, CMIC_PKT_REASON_MINI_0_TYPEr, REG_PORT_ANY, 0);
                } else                
#endif
                {
                    raddr = soc_reg_addr(unit, CMIC_PKT_REASON_MINIr, REG_PORT_ANY, 0);
                }
                SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
                *arg = rval;
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRxRedirectTruncatedPktReasonsExtended:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)
                    && (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                        SOC_IS_TD2_TT2(unit))) {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_MINI_1_TYPEr, REASONSf)) {
                        uint32 raddr;
                        rval = 0;
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_MINI_1_TYPEr, REG_PORT_ANY, 0);
                        SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
                        *arg = rval;
                        return BCM_E_NONE;
                    }
                } else                
#endif
                {
                    if (SOC_REG_FIELD_VALID(unit, CMIC_PKT_REASON_MINI_HIr, REASONSf)) {
                        uint32 raddr;
                        rval = 0;
                        raddr = soc_reg_addr(unit, CMIC_PKT_REASON_MINI_HIr, REG_PORT_ANY, 0);
                        SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
                        *arg = rval;
                        return BCM_E_NONE;
                    }
                }
            } 
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchRxRedirectPktCos:
#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm) &&
                SOC_REG_IS_VALID(unit, CMIC_PKT_COS_QUEUES_LOr)) {
                uint32 raddr;
                rval = 0;
                raddr = soc_reg_addr(unit, CMIC_PKT_COS_QUEUES_LOr, REG_PORT_ANY, 0);
                SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
                *arg = rval;
                return BCM_E_NONE;
            } else
#endif
            {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRxRedirectPktCosExtended:
#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm) &&
                SOC_REG_IS_VALID(unit, CMIC_PKT_COS_QUEUES_HIr)) {
                uint32 raddr;
                rval = 0;
                raddr = soc_reg_addr(unit, CMIC_PKT_COS_QUEUES_HIr, REG_PORT_ANY, 0);
                SOC_IF_ERROR_RETURN(soc_pci_getreg(unit, raddr, &rval));
                *arg = rval;
                return BCM_E_NONE;
            } else
#endif
            {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuLocalMacOui: 
        case bcmSwitchRemoteCpuDestMacOui:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                return _bcm_rcpu_switch_mac_hi_get(unit, type, arg); 
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuTcMappingMacOui:
            if (soc_feature(unit, soc_feature_rcpu_tc_mapping)) {
                uint32 fieldval, regval;
                BCM_IF_ERROR_RETURN(
                    READ_REMOTE_CPU_DA_LSr(unit, &regval));
                fieldval = soc_reg_field_get(unit, REMOTE_CPU_DA_LSr, regval,
                                             DAf);
                *arg = fieldval >> 24;
                BCM_IF_ERROR_RETURN(
                    READ_REMOTE_CPU_DA_MSr(unit, &regval));
                fieldval = soc_reg_field_get(unit, REMOTE_CPU_DA_MSr, regval,
                                             DAf);
                *arg |= (fieldval << 8 );
 return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuLocalMacNonOui: 
        case bcmSwitchRemoteCpuDestMacNonOui:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
           return _bcm_rcpu_switch_mac_lo_get(unit, type, arg); 
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuTcMappingMacNonOui:
            if (soc_feature(unit, soc_feature_rcpu_tc_mapping)) {
                uint32 fieldval, regval;
                BCM_IF_ERROR_RETURN(
                    READ_REMOTE_CPU_DA_LSr(unit, &regval));
                fieldval = soc_reg_field_get(unit, REMOTE_CPU_DA_LSr, regval,
                                             DAf);
                *arg  = ((fieldval << 8) >> 8);
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuEthertype:
        case bcmSwitchRemoteCpuVlan:
        case bcmSwitchRemoteCpuTpid:
        case bcmSwitchRemoteCpuSignature:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                return _bcm_rcpu_switch_vlan_tpid_sig_get(unit, type, arg); 
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuTcMappingEthertype:
            if (soc_feature(unit, soc_feature_rcpu_tc_mapping)) {
                uint32 regval;
                BCM_IF_ERROR_RETURN(
                    READ_REMOTE_CPU_LENGTH_TYPEr(unit, &regval));
                *arg = soc_reg_field_get(unit, REMOTE_CPU_LENGTH_TYPEr, regval,
                                         LENGTH_TYPEf);
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuDestPort:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                return _bcm_rcpu_pipe_bypass_header_get(unit, type, arg);
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
        case bcmSwitchRemoteCpuHigigDestPort:
            if (soc_feature(unit, soc_feature_rcpu_1)) {
                return _bcm_rcpu_higig_header_get(unit, arg);
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif 
#if defined(BCM_TRIUMPH2_SUPPORT) \
    ||  defined(BCM_ENDURO_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
        case bcmSwitchServiceTpidReplace:
            if (SOC_IS_TRIUMPH2(unit) 
                || SOC_IS_ENDURO(unit) || SOC_IS_TRIDENT(unit)) {
                rval = 0;
                /*  Register settings requires "0" to enable TPID Replace and
                 *  "1" to disable TPID Modifications, when SD Action is "NOP"
                 */
                SOC_IF_ERROR_RETURN(READ_EGR_SD_TAG_CONTROLr(unit, &rval));
                *arg = soc_reg_field_get (unit, 
                           EGR_SD_TAG_CONTROLr, rval, DO_NOT_MOD_TPID_ENABLEf);
                *arg = (*arg) ? 0 : 1;
                return BCM_E_NONE;
            } else {
                return BCM_E_UNAVAIL;
            }
            break;
#endif /* TRIUMPH2 | ENDURO | TRIDENT */
#if  defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchCongestionCntag:
        case bcmSwitchCongestionCntagEthertype:
        case bcmSwitchCongestionCnm:
        case bcmSwitchCongestionCnmEthertype:
        case bcmSwitchCongestionNotificationIdHigh:
        case bcmSwitchCongestionNotificationIdQueue:
        case bcmSwitchCongestionUseOuterTpid:
        case bcmSwitchCongestionUseOuterVlan:
        case bcmSwitchCongestionUseOuterPktPri:
        case bcmSwitchCongestionUseOuterCfi:
        case bcmSwitchCongestionUseInnerPktPri:
        case bcmSwitchCongestionUseInnerCfi:
        case bcmSwitchCongestionMissingCntag:
            return _bcm_switch_qcn_get(unit, type, arg);
            break;
#endif /* defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) */
        case bcmSwitchLoadBalanceHashSelect:
            {
                soc_reg_t   reg; 
 
                COMPILER_64_ZERO(rval64);
                if (SOC_REG_FIELD_VALID(unit, ING_CONFIG_64r, LBID_RTAGf)){
                    reg = ING_CONFIG_64r;
                } else if (SOC_REG_FIELD_VALID(unit, ING_CONFIGr, LBID_RTAGf)) {
                    reg = ING_CONFIGr;
                } else {
                    return BCM_E_UNAVAIL;
                }

                BCM_IF_ERROR_RETURN(soc_reg_get(unit, reg, REG_PORT_ANY, 0, 
                                    &rval64));
                *arg = soc_reg64_field32_get(unit, reg, rval64, LBID_RTAGf);
                return (BCM_E_NONE);
            }
            break;
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
        case bcmSwitchTrillEthertype:
             if (soc_feature(unit, soc_feature_trill)) {
                  return bcm_td_trill_ethertype_get(unit, arg);
             }
             break;

        case bcmSwitchTrillISISEthertype:
             if (soc_feature(unit, soc_feature_trill)) {
                  return bcm_td_trill_ISIS_ethertype_get(unit, arg);
             }
             break;

        case bcmSwitchTrillISISDestMacOui:
        case bcmSwitchTrillBroadcastDestMacOui:
        case bcmSwitchTrillEndStationDestMacOui:
             if (soc_feature(unit, soc_feature_trill)) {
                  return bcm_td_trill_mac_get(unit, type, (uint32 *)arg, 1);
             }
             break;

        case bcmSwitchTrillISISDestMacNonOui:
        case bcmSwitchTrillBroadcastDestMacNonOui:
        case bcmSwitchTrillEndStationDestMacNonOui:
             if (soc_feature(unit, soc_feature_trill)) {
                  return bcm_td_trill_mac_get(unit, type, (uint32 *)arg, 0);
             }
             break;

        case bcmSwitchTrillMinTtl:
              if (soc_feature(unit, soc_feature_trill)) {
                   return bcm_td_trill_MinTtl_get(unit, arg);
              }
             break;
 
        case bcmSwitchTrillTtlCheckEnable:
              if (soc_feature(unit, soc_feature_trill)) {
                   return bcm_td_trill_TtlCheckEnable_get(unit, arg);
              }
             break;
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */
#ifdef BCM_TRIDENT_SUPPORT
        case bcmSwitchFcoeEtherType:
            if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) {
                return _bcm_switch_fcoe_ing_ethertype_get(unit, arg);
            }
        break;
#endif
#if defined(BCM_FIREBOLT_SUPPORT) 
#if defined(INCLUDE_L3)
        case bcmSwitchL3TunnelIpV4ModeOnly:
             return soc_tunnel_term_block_size_get (unit, arg);
             break;
        case bcmSwitchL3SrcHitEnable:
            {
                soc_reg_t   reg;

                COMPILER_64_ZERO(rval64);
                if (SOC_REG_FIELD_VALID(unit, ING_CONFIG_64r, 
                                        L3SRC_HIT_ENABLEf)){
                    reg = ING_CONFIG_64r;
                } else if (SOC_REG_FIELD_VALID(unit, ING_CONFIGr, 
                                               L3SRC_HIT_ENABLEf)) {
                    reg = ING_CONFIGr;
                } else {
                    return BCM_E_UNAVAIL;
                }

                BCM_IF_ERROR_RETURN(soc_reg_get(unit, reg, REG_PORT_ANY, 0, &rval64));
                *arg = soc_reg64_field32_get(unit, reg, rval64, L3SRC_HIT_ENABLEf);
                return (BCM_E_NONE);
            }
            break;
#endif /* INCLUDE_L3*/
#endif /* BCM_FIREBOLT_SUPPORT */
        case bcmSwitchL2DstHitEnable:
            {
                soc_reg_t   reg;
    
                COMPILER_64_ZERO(rval64);
                if (SOC_REG_FIELD_VALID(unit, ING_CONFIG_64r, 
                                        L2DST_HIT_ENABLEf)){
                    reg = ING_CONFIG_64r;
                } else if (SOC_REG_FIELD_VALID(unit, ING_CONFIGr, 
                                               L2DST_HIT_ENABLEf)) {
                    reg = ING_CONFIGr;
                } else {
                    return BCM_E_UNAVAIL;
                }
    
                BCM_IF_ERROR_RETURN(soc_reg_get(unit, reg, REG_PORT_ANY, 0, &rval64));
                *arg = soc_reg64_field32_get(unit, reg, rval64, L2DST_HIT_ENABLEf);
                return (BCM_E_NONE);
            }
            break;
        case bcmSwitchHashDualMoveDepth:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthL2:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_L2X(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthMpls:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_MPLS(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthVlan:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_VLAN(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
        case bcmSwitchHashDualMoveDepthEgressVlan:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_EGRESS_VLAN(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
#if defined(BCM_TRIUMPH3_SUPPORT)
        case bcmSwitchHashDualMoveDepthWlanPort:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_WLAN_PORT(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashDualMoveDepthWlanClient:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_WLAN_CLIENT(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchL2GreProtocolType:  
            /* Get protocol-type for L2-GRE */
            if (soc_feature(unit, soc_feature_l2gre)) {
                rval = 0;

                SOC_IF_ERROR_RETURN(READ_L2GRE_CONTROLr(unit, &rval));
                *arg = soc_reg_field_get(unit, L2GRE_CONTROLr, rval, 
                                         ETHERTYPEf);
                return BCM_E_NONE;
            } else {
                  return BCM_E_UNAVAIL;
            }
            break;

        case bcmSwitchL2GreVpnIdSizeSet: 
            /* Get bit-size of VPNID within L2-GRE key */
            if (soc_feature(unit, soc_feature_l2gre)) {
                uint32 regval;
                rval = 0;

                SOC_IF_ERROR_RETURN(READ_L2GRE_CONTROLr(unit, &rval));
                regval = soc_reg_field_get(unit, L2GRE_CONTROLr, rval, 
                                         GRE_KEY_ENTROPY_SIZEf);
                switch (regval) {
                  case 3:  
                                  *arg = 16;
                                  break;
                  case 2:  
                                 *arg = 20;
                                  break;
                  case 1:  
                                 *arg = 24;
                                  break;
                  case 0:  
                                 *arg = 32;
                                  break;
                  default:   
                                  return BCM_E_PARAM;
                }
                return BCM_E_NONE;
            } else {
                  return BCM_E_UNAVAIL;
            }
            break;

    case bcmSwitchRemoteEncapsulationMode: 
        *arg = SOC_REMOTE_ENCAP(unit);
        return BCM_E_NONE;
        break;

#endif

#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
        case bcmSwitchVxlanUdpDestPortSet:  
            /* Get UDP Dest port for VXLAN */
            if (soc_feature(unit, soc_feature_vxlan)) {
                  return bcm_td2_vxlan_udpDestPort_get(unit, arg);
            } else {
                  return BCM_E_UNAVAIL;
            }
          break;

        case bcmSwitchVxlanEntropyEnable: 
            /* Enable UDP Source Port Hash for VXLAN */
            if (soc_feature(unit, soc_feature_vxlan)) {
                  return bcm_td2_vxlan_udpSourcePort_get(unit, arg);
            } else {
                  return BCM_E_UNAVAIL;
            }
          break;
#endif /* defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3) */

#if defined(INCLUDE_L3)
        case bcmSwitchHashDualMoveDepthL3:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                *arg = SOC_DUAL_HASH_MOVE_MAX_L3X(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashL3DualLeastFull:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (SOC_REG_FIELD_VALID(unit, L3_AUX_HASH_CONTROLr, 
                                        INSERT_LEAST_FULL_HALFf)) {
                    rval = 0;
                    BCM_IF_ERROR_RETURN(READ_L3_AUX_HASH_CONTROLr(unit, &rval));
                    *arg = soc_reg_field_get(unit, L3_AUX_HASH_CONTROLr,
                                             rval, INSERT_LEAST_FULL_HALFf);
                    return BCM_E_NONE;
                }
            }
            return BCM_E_UNAVAIL;
            break;
#endif
        case bcmSwitchHashMultiMoveDepth:
            if (soc_feature(unit, soc_feature_ism_memory) ||
                soc_feature(unit, soc_feature_shared_hash_mem)) {
                *arg = SOC_MULTI_HASH_MOVE_MAX(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMultiMoveDepthL2:
            if (soc_feature(unit, soc_feature_ism_memory) ||
                soc_feature(unit, soc_feature_shared_hash_mem)) {
                *arg = SOC_MULTI_HASH_MOVE_MAX_L2(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMultiMoveDepthL3:
            if (soc_feature(unit, soc_feature_ism_memory) ||
                soc_feature(unit, soc_feature_shared_hash_mem)) {
                *arg = SOC_MULTI_HASH_MOVE_MAX_L3(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMultiMoveDepthMpls:
            if (soc_feature(unit, soc_feature_ism_memory)) {
                *arg = SOC_MULTI_HASH_MOVE_MAX_MPLS(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMultiMoveDepthVlan:
            if (soc_feature(unit, soc_feature_ism_memory)) {
                *arg = SOC_MULTI_HASH_MOVE_MAX_VLAN(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMultiMoveDepthEgressVlan:
            if (soc_feature(unit, soc_feature_ism_memory)) {
                *arg = SOC_MULTI_HASH_MOVE_MAX_EGRESS_VLAN(unit);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashL2DualLeastFull:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (SOC_REG_FIELD_VALID(unit, L2_AUX_HASH_CONTROLr, 
                                        INSERT_LEAST_FULL_HALFf)) {
                    rval = 0;
                    BCM_IF_ERROR_RETURN(READ_L2_AUX_HASH_CONTROLr(unit, &rval));
                    *arg = soc_reg_field_get(unit, L2_AUX_HASH_CONTROLr,
                                             rval, INSERT_LEAST_FULL_HALFf);
                    return BCM_E_NONE;
                }
            }
            return BCM_E_UNAVAIL;
            break;
        case bcmSwitchHashMPLSDualLeastFull:
            if (soc_feature(unit, soc_feature_dual_hash)) {
                if (SOC_REG_FIELD_VALID(unit, MPLS_ENTRY_HASH_CONTROLr, 
                                        INSERT_LEAST_FULL_HALFf)) {
                    rval = 0;
                    BCM_IF_ERROR_RETURN(READ_MPLS_ENTRY_HASH_CONTROLr(unit, &rval));
                    *arg = soc_reg_field_get(unit, MPLS_ENTRY_HASH_CONTROLr,
                                             rval, INSERT_LEAST_FULL_HALFf);
                    return BCM_E_NONE;
                }
            }
            return BCM_E_UNAVAIL;
            break;
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
        case bcmSwitchSynchronousPortClockSource:
            {
                uint32 val = 0;
                if (soc_feature(unit, soc_feature_gmii_clkout)) {
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || defined(BCM_KATANA_SUPPORT)
                    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit)  || SOC_IS_KATANA(unit)) {
                        return _bcm_switch_sync_port_select_get(unit, 1, (uint32 *)arg);
                    } else 
#endif
                    {
                        SOC_IF_ERROR_RETURN(READ_EGR_L1_CLK_RECOVERY_CTRLr(unit, &val));
                        *arg = soc_reg_field_get(unit, EGR_L1_CLK_RECOVERY_CTRLr,
                                                 val, PRI_PORT_SELf);
                        return (BCM_E_NONE);
                    }
                } else {
                    return BCM_E_UNAVAIL;
                }
            }
            break;
        case bcmSwitchSynchronousPortClockSourceBkup:
            {
                uint32 val = 0;
                if (soc_feature(unit, soc_feature_gmii_clkout)) {
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || defined(BCM_KATANA_SUPPORT)
                    if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit) || SOC_IS_KATANA(unit)) {
                        return _bcm_switch_sync_port_select_get(unit, 0, (uint32 *)arg);
                    } else 
#endif
                    {
                        SOC_IF_ERROR_RETURN(READ_EGR_L1_CLK_RECOVERY_CTRLr(unit, &val));
                        *arg = soc_reg_field_get(unit, EGR_L1_CLK_RECOVERY_CTRLr,
                                                 val, BKUP_PORT_SELf);
                        return (BCM_E_NONE);
                    }
                } else {
                    return BCM_E_UNAVAIL;
                }
            }
            break;

         case bcmSwitchSynchronousPortClockSourceDivCtrl:
              if (soc_feature(unit, soc_feature_gmii_clkout)) {
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || defined(BCM_KATANA_SUPPORT)
                 if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit) || SOC_IS_KATANA(unit)) {
                    return _bcm_switch_div_ctrl_select_get(unit, 1, (uint32 *)arg);
                 } else  {
                    return BCM_E_UNAVAIL;
                 }
#else 
                 {
                    return BCM_E_UNAVAIL;
                 }
#endif
            } else {
                 return BCM_E_UNAVAIL;
            }
            break;
             
         case bcmSwitchSynchronousPortClockSourceBkupDivCtrl:
             if (soc_feature(unit, soc_feature_gmii_clkout)) {
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || defined(BCM_KATANA_SUPPORT)
                if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit) || SOC_IS_KATANA(unit)) {
                   return _bcm_switch_div_ctrl_select_get(unit, 0, (uint32 *)arg);
                } else {
                   return BCM_E_UNAVAIL;
                }
#else
                {  
                   return BCM_E_UNAVAIL;
                }
#endif
             } else {
                   return BCM_E_UNAVAIL;
             }
#endif
            break;
              
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
        case bcmSwitchNivEthertype:
             if (soc_feature(unit, soc_feature_niv)) {
                  return bcm_trident_niv_ethertype_get(unit, arg);
             }
             return BCM_E_UNAVAIL;
             break;
#endif /* BCM_TRIDENT_SUPPORT && INCLUDE_L3 */

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
        case bcmSwitchEtagEthertype:
             if (soc_feature(unit, soc_feature_port_extension)) {
                 return bcm_tr3_etag_ethertype_get(unit, arg);
             }
             return BCM_E_UNAVAIL;
             break;
    case bcmSwitchExtenderMulticastLowerThreshold:
         if (soc_feature(unit, soc_feature_port_extension)) {
             uint32 etag_range = 0;
             SOC_IF_ERROR_RETURN(READ_ETAG_MULTICAST_RANGEr(unit, &etag_range));
             *arg = soc_reg_field_get(unit, ETAG_MULTICAST_RANGEr, etag_range,
                     ETAG_VID_MULTICAST_LOWERf);
             return BCM_E_NONE;
         }
         return BCM_E_UNAVAIL;
         break;
    case bcmSwitchExtenderMulticastHigherThreshold:
         if (soc_feature(unit, soc_feature_port_extension)) {
             uint32 etag_range = 0;
             SOC_IF_ERROR_RETURN(READ_ETAG_MULTICAST_RANGEr(unit, &etag_range));
             *arg = soc_reg_field_get(unit, ETAG_MULTICAST_RANGEr, etag_range,
                     ETAG_VID_MULTICAST_UPPERf);
             return BCM_E_NONE;
         }
         return BCM_E_UNAVAIL;
         break;
#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

    case bcmSwitchWESPProtocolEnable:
        if (soc_feature(unit, soc_feature_wesp)) {
            rval = 0;;
            BCM_IF_ERROR_RETURN(READ_ING_WESP_PROTO_CONTROLr(unit, &rval));
            *arg = soc_reg_field_get(unit, ING_WESP_PROTO_CONTROLr, rval,
                                     WESP_PROTO_NUMBER_ENABLEf);
            return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;
    case bcmSwitchWESPProtocol:
        if (soc_feature(unit, soc_feature_wesp)) {
            rval = 0;
            BCM_IF_ERROR_RETURN(READ_ING_WESP_PROTO_CONTROLr(unit, &rval));
            *arg = soc_reg_field_get(unit, ING_WESP_PROTO_CONTROLr, rval,
                                     WESP_PROTO_NUMBERf);
            return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;

#if defined(BCM_TRIDENT_SUPPORT)
    case bcmSwitchIp6CompressEnable:
        if (SOC_REG_FIELD_VALID(unit, ING_MISC_CONFIG2r,
            IPV6_TO_IPV4_ADDRESS_MAP_ENABLEf)) {
            rval = 0;
            BCM_IF_ERROR_RETURN(READ_ING_MISC_CONFIG2r(unit, &rval));
            *arg = soc_reg_field_get(unit, ING_MISC_CONFIG2r, rval,
                        IPV6_TO_IPV4_ADDRESS_MAP_ENABLEf);
            return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchIp6CompressDefaultOffset:
        if (SOC_REG_FIELD_VALID(unit, ING_MISC_CONFIG2r,
            IPV6_TO_IPV4_MAP_OFFSET_DEFAULTf)) {
            rval = 0;
            BCM_IF_ERROR_RETURN(READ_ING_MISC_CONFIG2r(unit, &rval));
            *arg = soc_reg_field_get(unit, ING_MISC_CONFIG2r, rval,
                        IPV6_TO_IPV4_MAP_OFFSET_DEFAULTf);
            return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;
#endif
    case bcmSwitchMirrorPktChecksEnable:
        if (SOC_REG_FIELD_VALID(unit, EGR_CONFIG_1r,
            DISABLE_MIRROR_CHECKSf)) {
            rval = 0;
            BCM_IF_ERROR_RETURN(READ_EGR_CONFIG_1r(unit, &rval));
            if (soc_reg_field_get(unit, EGR_CONFIG_1r, rval,
                                  DISABLE_MIRROR_CHECKSf)) {
                *arg = 0;
            } else {
                *arg = 1;
            }
            return BCM_E_NONE;
        } 
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchStableSaveLongIds:
#if defined(BCM_FIELD_SUPPORT) && defined(BCM_WARM_BOOT_SUPPORT)
        {
            _field_control_t *fc;

            BCM_IF_ERROR_RETURN(_field_control_get(unit, &fc));
            
            *arg = ((fc->flags & _FP_STABLE_SAVE_LONG_IDS) != 0);
        }
        return (BCM_E_NONE);
#else
        return (BCM_E_UNAVAIL);
#endif
        break;

#if defined(BCM_TRIUMPH_SUPPORT)
    case bcmSwitchGportAnyDefaultL2Learn:
    case bcmSwitchGportAnyDefaultL2Move:
            if (soc_feature(unit, soc_feature_virtual_switching)) {
                if(!SOC_IS_HURRICANEX(unit)) {
                    return _bcm_switch_default_cml_get(unit, type, arg);
                }
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_TRIUMPH_SUPPORT */

#if defined(BCM_KATANA_SUPPORT)
    case bcmSwitchSetMplsEntropyLabelTtl:
        if (soc_feature(unit, soc_feature_mpls_entropy)) {
            rval = 0;

                SOC_IF_ERROR_RETURN(READ_EGR_MPLS_ENTROPY_LABEL_CONTROLr(unit, &rval));
                *arg = soc_reg_field_get(unit, EGR_MPLS_ENTROPY_LABEL_CONTROLr, rval, TTLf);
                return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchSetMplsEntropyLabelPri:
        if (soc_feature(unit, soc_feature_mpls_entropy)) {
            rval = 0;

                SOC_IF_ERROR_RETURN(READ_EGR_MPLS_ENTROPY_LABEL_CONTROLr(unit, &rval));
                *arg = soc_reg_field_get(unit, EGR_MPLS_ENTROPY_LABEL_CONTROLr, rval, TCf);
                return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchSetMplsEntropyLabelOffset:
        if (soc_feature(unit, soc_feature_mpls_entropy)) {
            rval = 0;

                SOC_IF_ERROR_RETURN(READ_EGR_MPLS_ENTROPY_LABEL_CONTROLr(unit, &rval));
                *arg = soc_reg_field_get(unit, EGR_MPLS_ENTROPY_LABEL_CONTROLr, rval, LABEL_OFFSETf);
                return BCM_E_NONE;
        }
        return BCM_E_UNAVAIL;
        break;
#endif

#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    case bcmSwitchMultipathCompress:
        if (soc_feature(unit, soc_feature_l3)) {
            *arg = TRUE;
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchMultipathCompressBuffer:
        if (soc_feature(unit, soc_feature_l3)) {
            return bcm_tr2_l3_ecmp_defragment_buffer_get(unit, arg);
        }
        break;
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */

    case bcmSwitchBstEnable:
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
            return _bcm_bst_cmn_control_get(unit, bcmSwitchBstEnable, arg);
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

        return BCM_E_UNAVAIL;
        break;

    case bcmSwitchBstTrackingMode:
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
            return _bcm_bst_cmn_control_get(unit, bcmSwitchBstTrackingMode, arg);
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

        return BCM_E_UNAVAIL;
        break;

#ifdef BCM_TRIDENT_SUPPORT
    case bcmSwitchFabricTrunkAutoIncludeDisable:
        if (!soc_feature(unit, soc_feature_modport_map_dest_is_port_or_trunk)) {
            return BCM_E_UNAVAIL;
        }
        return bcm_td_modport_map_mode_get(unit, type, arg);
        break;
#endif /* BCM_TRIDENT_SUPPORT */

    case bcmSwitchL2OverflowEvent:
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
        if (soc_feature(unit, soc_feature_l2_overflow)) {
            *arg = SOC_CONTROL(unit)->l2_overflow_enable ? 1 : 0;
            return BCM_E_NONE;
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

        return BCM_E_UNAVAIL;
        break;
#ifdef BCM_TRIDENT_SUPPORT
        case bcmSwitchMiMDefaultSVPValue:
            if (SOC_IS_TRIDENT(unit) || SOC_IS_TRIDENT2(unit) ||
                SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) {
                rval = 0;
                BCM_IF_ERROR_RETURN(READ_MIM_DEFAULT_NETWORK_SVPr(unit, &rval));
                *arg = soc_reg_field_get(unit, MIM_DEFAULT_NETWORK_SVPr, rval,
                                  SVPf);
                return BCM_E_NONE;
            }
            break;
#endif
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
        case bcmSwitchIpmcSameVlanPruning:
            if (SOC_IS_TRIUMPH3(unit) || SOC_IS_HELIX4(unit) ||
                SOC_IS_TRIDENT2(unit)) {
                rval = 0;
                SOC_IF_ERROR_RETURN(READ_ING_MISC_CONFIGr(unit, &rval));
                *arg = soc_reg_field_get(unit, ING_MISC_CONFIGr, rval, IPMC_IND_MODEf);
                *arg = (*arg ? 0 : 1);
                return BCM_E_NONE;
            }
        break;
#endif

#if defined(BCM_KATANA2_SUPPORT)
        case bcmSwitchOamCcmToCpu:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                rval = 0;
                SOC_IF_ERROR_RETURN(READ_CCM_COPYTO_CPU_CONTROLr(unit, &rval));
                *arg = soc_reg_field_get(unit, CCM_COPYTO_CPU_CONTROLr, rval, 
                                         ERROR_CCM_COPY_TOCPUf);
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchOamXconCcmToCpu:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                rval = 0;
                SOC_IF_ERROR_RETURN(READ_CCM_COPYTO_CPU_CONTROLr(unit, &rval));
                *arg = soc_reg_field_get(unit, CCM_COPYTO_CPU_CONTROLr, rval, 
                                         XCON_CCM_COPY_TOCPUf);
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchOamHeaderErrorToCpu:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                rval = 0;
                SOC_IF_ERROR_RETURN(READ_CCM_COPYTO_CPU_CONTROLr(unit, &rval));
                *arg = soc_reg_field_get(unit, CCM_COPYTO_CPU_CONTROLr, rval, 
                                         OAM_HEADER_ERROR_TOCPUf);
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;
   
        case bcmSwitchOamUnknownVersionToCpu:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                rval = 0;
                SOC_IF_ERROR_RETURN(READ_CCM_COPYTO_CPU_CONTROLr(unit, &rval));
                *arg = soc_reg_field_get(unit, CCM_COPYTO_CPU_CONTROLr, rval, 
                                         OAM_UNKNOWN_OPCODE_VERSION_TOCPUf);
                return BCM_E_NONE;
            } 
            break;

        case bcmSwitchOamUnknownVersionDrop:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                rval = 0;
                SOC_IF_ERROR_RETURN(READ_OAM_DROP_CONTROLr(unit, &rval));
                *arg = soc_reg_field_get(unit, OAM_DROP_CONTROLr, rval, 
                                       IFP_OAM_UNKNOWN_OPCODE_VERSION_DROPf);
                return BCM_E_NONE;
            } 
            break;

        case bcmSwitchOamUnexpectedPktToCpu:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                rval = 0;
                SOC_IF_ERROR_RETURN(READ_CCM_COPYTO_CPU_CONTROLr(unit, &rval));
                *arg = soc_reg_field_get(unit, CCM_COPYTO_CPU_CONTROLr, rval, 
                                         OAM_UNEXPECTED_PKT_TOCPUf);
                return BCM_E_NONE;
            } 
            break;

        case bcmSwitchOamVersionCheckDisable:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                rval = 0;
                SOC_IF_ERROR_RETURN(READ_OAM_CONTROL_1r(unit, &rval));
                *arg = soc_reg_field_get(unit, OAM_CONTROL_1r, rval, 
                                    OAM_VER_CHECK_DISABLEf);
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchOamOlpChipEtherType:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                COMPILER_64_ZERO(rval64); 
                SOC_IF_ERROR_RETURN(READ_IARB_OLP_CONFIGr(unit, &rval64));
                *arg = soc_reg64_field32_get(unit, IARB_OLP_CONFIGr, rval64, 
                                         ETHERTYPEf);
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;


        case bcmSwitchOamOlpChipTpid:
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                egr_olp_config_entry_t  eolp_cfg;
                SOC_IF_ERROR_RETURN(READ_EGR_OLP_CONFIGm(unit, MEM_BLOCK_ANY, 
                                                           0, &eolp_cfg));
                *arg = soc_mem_field32_get(unit, EGR_OLP_CONFIGm, 
                                           &eolp_cfg, VLAN_TPIDf);
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;


        case bcmSwitchOamOlpChipVlan: 
            if (SOC_IS_KATANA2(unit) && (soc_feature(unit, soc_feature_oam))) {
                rval = 0;
                SOC_IF_ERROR_RETURN(READ_EGR_OLP_VLANr(unit, &rval));
                *arg = soc_reg_field_get(unit, EGR_OLP_VLANr, rval, VLAN_TAGf);
                return BCM_E_NONE;
            } 
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_KATANA2_SUPPORT */
        case bcmSwitchFieldStageEgressToCpu:
            if (soc_feature(unit, soc_feature_field_egress_tocpu)) {
               rval = 0;
               SOC_IF_ERROR_RETURN(READ_EGR_CPU_CONTROLr(unit, &rval));
               *arg = soc_reg_field_get(unit, EGR_CPU_CONTROLr,
                                        rval, EFP_TOCPUf);
               return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;

        case bcmSwitchSystemReservedVlan:
            return _bcm_esw_vlan_system_reserved_get(unit, arg);

#if defined(BCM_KATANA2_SUPPORT)
        case bcmSwitchHashOamEgress:
        case bcmSwitchHashOamEgressDual:
            if (SOC_IS_KATANA2(unit)) {
                return _bcm_fb_er_hashselect_get(unit, type, arg);
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_ENDURO_SUPPORT)
        case bcmSwitchWredForAllPkts:
            if(SOC_IS_ENDURO(unit)) {
                rval = 0;
                SOC_IF_ERROR_RETURN(READ_ING_MISC_CONFIGr(unit, &rval));
                *arg = soc_reg_field_get(unit, ING_MISC_CONFIGr,
                                         rval, TREAT_ALL_PKTS_AS_TCPf);
                return BCM_E_NONE;
            }
            return BCM_E_UNAVAIL;
            break;
#endif /* BCM_ENDURO_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)

    case bcmSwitchFcoeNpvModeEnable:
        if (soc_feature(unit, soc_feature_fcoe)) {
            uint32 rval;

            SOC_IF_ERROR_RETURN(READ_FCOE_ING_CONTROLr(unit, &rval));
            *arg = soc_reg_field_get(unit, FCOE_ING_CONTROLr, rval, 
                                     FCOE_NPV_MODEf);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeDomainRoutePrefixLength:
        if (soc_feature(unit, soc_feature_fcoe)) {
            uint32 rval;

            SOC_IF_ERROR_RETURN(READ_FCOE_ING_CONTROLr(unit, &rval));
            *arg = soc_reg_field_get(unit, FCOE_ING_CONTROLr, rval, 
                                     FCOE_DOMAIN_ROUTE_PREFIXf);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeCutThroughEnable:
        if (soc_feature(unit, soc_feature_fcoe)) {
            uint32 rval;

            SOC_IF_ERROR_RETURN(READ_FCOE_ING_CONTROLr(unit, &rval));
            *arg = soc_reg_field_get(unit, FCOE_ING_CONTROLr, rval, 
                                     FCOE_CUT_THROUGH_ENABLEf);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeSourceBindCheckAction:
        if (soc_feature(unit, soc_feature_fcoe)) {
            uint32 rval;

            SOC_IF_ERROR_RETURN(READ_FCOE_ING_CONTROLr(unit, &rval));
            *arg = soc_reg_field_get(unit, FCOE_ING_CONTROLr, rval, 
                                     FCOE_SRC_BIND_CHECK_ACTIONf);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeSourceFpmaPrefixCheckAction:
        if (soc_feature(unit, soc_feature_fcoe)) {
            uint32 rval;

            SOC_IF_ERROR_RETURN(READ_FCOE_ING_CONTROLr(unit, &rval));
            *arg = soc_reg_field_get(unit, FCOE_ING_CONTROLr, rval, 
                                     FCOE_SRC_FPMA_PREFIX_CHECK_ACTIONf);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeVftHopCountExpiryToCpu:
        if (soc_feature(unit, soc_feature_fcoe)) {
            uint32 rval;

            SOC_IF_ERROR_RETURN(READ_CPU_CONTROL_0r(unit, &rval));
            *arg = soc_reg_field_get(unit, CPU_CONTROL_0r, rval, 
                                     FCOE_VFT_HOPCOUNT_TOCPUf);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeVftHopCountExpiryAction:
        if (soc_feature(unit, soc_feature_fcoe)) {
            egr_fcoe_control_1_entry_t egr_fcoe_control;

            SOC_IF_ERROR_RETURN(
                READ_EGR_FCOE_CONTROL_1m(unit, MEM_BLOCK_ANY, 
                                         0, &egr_fcoe_control));
            *arg = soc_mem_field32_get(unit, EGR_FCOE_CONTROL_1m, 
                                       &egr_fcoe_control, 
                                       FCOE_VFT_HOPCOUNT_EXPIRE_CONTROLf);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofT1Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            egr_fcoe_control_1_entry_t egr_fcoe_control;

            SOC_IF_ERROR_RETURN(
                READ_EGR_FCOE_CONTROL_1m(unit, MEM_BLOCK_ANY, 
                                         0, &egr_fcoe_control));
            *arg = soc_mem_field32_get(unit, EGR_FCOE_CONTROL_1m, 
                                       &egr_fcoe_control, FCOE_FC_EOF_T_1f);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofT2Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            egr_fcoe_control_1_entry_t egr_fcoe_control;

            SOC_IF_ERROR_RETURN(
                READ_EGR_FCOE_CONTROL_1m(unit, MEM_BLOCK_ANY, 
                                         0, &egr_fcoe_control));
            *arg = soc_mem_field32_get(unit, EGR_FCOE_CONTROL_1m, 
                                       &egr_fcoe_control, FCOE_FC_EOF_T_2f);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofA1Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            egr_fcoe_control_1_entry_t egr_fcoe_control;

            SOC_IF_ERROR_RETURN(
                READ_EGR_FCOE_CONTROL_1m(unit, MEM_BLOCK_ANY, 
                                         0, &egr_fcoe_control));
            *arg = soc_mem_field32_get(unit, EGR_FCOE_CONTROL_1m, 
                                       &egr_fcoe_control, FCOE_FC_EOF_A_1f);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofA2Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            egr_fcoe_control_1_entry_t egr_fcoe_control;

            SOC_IF_ERROR_RETURN(
                READ_EGR_FCOE_CONTROL_1m(unit, MEM_BLOCK_ANY, 
                                         0, &egr_fcoe_control));
            *arg = soc_mem_field32_get(unit, EGR_FCOE_CONTROL_1m, 
                                       &egr_fcoe_control, FCOE_FC_EOF_A_2f);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofN1Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            egr_fcoe_control_1_entry_t egr_fcoe_control;

            SOC_IF_ERROR_RETURN(
                READ_EGR_FCOE_CONTROL_1m(unit, MEM_BLOCK_ANY, 
                                         0, &egr_fcoe_control));
            *arg = soc_mem_field32_get(unit, EGR_FCOE_CONTROL_1m, 
                                       &egr_fcoe_control, FCOE_FC_EOF_N_1f);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofN2Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            egr_fcoe_control_1_entry_t egr_fcoe_control;

            SOC_IF_ERROR_RETURN(
                READ_EGR_FCOE_CONTROL_1m(unit, MEM_BLOCK_ANY, 
                                         0, &egr_fcoe_control));
            *arg = soc_mem_field32_get(unit, EGR_FCOE_CONTROL_1m, 
                                       &egr_fcoe_control, FCOE_FC_EOF_N_2f);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofNI1Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            egr_fcoe_control_1_entry_t egr_fcoe_control;

            SOC_IF_ERROR_RETURN(
                READ_EGR_FCOE_CONTROL_1m(unit, MEM_BLOCK_ANY, 
                                         0, &egr_fcoe_control));
            *arg = soc_mem_field32_get(unit, EGR_FCOE_CONTROL_1m, 
                                       &egr_fcoe_control, FCOE_FC_EOF_NI_1f);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeFcEofNI2Value:
        if (soc_feature(unit, soc_feature_fcoe)) {
            egr_fcoe_control_1_entry_t egr_fcoe_control;

            SOC_IF_ERROR_RETURN(
                READ_EGR_FCOE_CONTROL_1m(unit, MEM_BLOCK_ANY, 
                                         0, &egr_fcoe_control));
            *arg = soc_mem_field32_get(unit, EGR_FCOE_CONTROL_1m, 
                                       &egr_fcoe_control, FCOE_FC_EOF_NI_2f);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeZoneCheckFailToCpu:
        if (soc_feature(unit, soc_feature_fcoe)) {
            uint32 rval;
                                       
            SOC_IF_ERROR_RETURN(READ_CPU_CONTROL_Mr(unit, &rval));
            *arg = soc_reg_field_get(unit, CPU_CONTROL_Mr, rval,
                                     FCOE_ZONE_CHECK_FAIL_COPY_TOCPUf);
            return BCM_E_NONE;
        }
        break;

    case bcmSwitchFcoeZoneCheckMissDrop:
        if (soc_feature(unit, soc_feature_fcoe)) {
            uint32 rval;
                                       
            SOC_IF_ERROR_RETURN(READ_ING_MISC_CONFIG2r(unit, &rval));
            *arg = soc_reg_field_get(unit, ING_MISC_CONFIG2r, rval,
                                     FCOE_ZONE_CHECK_MISS_DROPf);
            return BCM_E_NONE;
        }
        break;
#endif  /* BCM_TRIDENT2_SUPPORT */
    case bcmSwitchNetworkGroupDepth:
        if (soc_feature(unit, soc_feature_multiple_split_horizon_group)) {
            *arg = soc_mem_index_count(unit, ING_NETWORK_PRUNE_CONTROLm);
            return BCM_E_NONE;
        }
        break;
    case bcmSwitchUnknownSubportPktTagToCpu:
        if (soc_feature(unit, soc_feature_subtag_coe)) {
            uint32 rval;
                                       
            SOC_IF_ERROR_RETURN(READ_CPU_CONTROL_1r(unit, &rval));
            *arg = soc_reg_field_get(unit, CPU_CONTROL_1r, rval,
                       UNKNOWN_SUBTENDING_PORT_TOCPUf);
            return BCM_E_NONE;
        }
        break;
        default:
            break;
    }

    /* Stop on first supported port for port specific controls */
    PBMP_E_ITER(unit, port) {
        if (SOC_IS_STACK_PORT(unit, port)) {
            continue;
        }
        rv = bcm_esw_switch_control_port_get(unit, port, type, arg);
        if (rv != BCM_E_UNAVAIL) { /* Found one, or real error */
            return rv;
        }
    }

    PBMP_HG_ITER(unit, port) {
        if (SOC_IS_STACK_PORT(unit, port)) {
            continue;
        }
        rv = bcm_esw_switch_control_port_get(unit, port, type, arg);
        if (rv != BCM_E_UNAVAIL) { /* Found one, or real error */
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_switch_init
 * Description:
 *      Initialize switch controls to their different values.
 * Parameters:
 *      unit        - Device unit number
 * Returns:
 *      BCM_E_xxx
 */
int 
_bcm_esw_switch_init(int unit)
{
    int cos = 0;

    /* Unit validation check */
    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    if (SOC_IS_RCPU_ONLY(unit)) {
        return BCM_E_NONE;
    }

#if (defined(BCM_HURRICANE2_SUPPORT) && !defined(_HURRICANE2_DEBUG))
    if (SOC_IS_HURRICANE2(unit)) {
        return BCM_E_NONE;
    }
#endif

    BCM_IF_ERROR_RETURN(_bcm_esw_switch_detach(unit));

    /* Clear encapsulation priority mappings */
    if (soc_feature(unit, soc_feature_rcpu_priority)) {
        for (cos = 0; cos < NUM_CPU_COSQ(unit); cos++) {
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_rcpu_encap_priority_map_set(unit, 0, cos, 0));
        }
    }

#if defined(BCM_HAWKEYE_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT)
    /* Initialize tymesync MMRP and SRP switch controls to default values */
    if (soc_feature(unit, soc_feature_timesync_support)) {
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_ENDURO(unit) || SOC_IS_HURRICANEX(unit) || SOC_IS_KATANAX(unit)) {
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchTimeSyncEthertype, 
                                           TS_ETHERTYPE_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchTimeSyncDestMacOui, 
                                           TS_MAC_OUI_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchTimeSyncDestMacNonOui, 
                                           TS_MAC_NONOUI_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchTimeSyncMessageTypeBitmap, 
                                           TS_MAC_MSGBITMAP_DEFAULT));         
        } else 
#endif /* BCM_ENDURO_SUPPORT  || BCM_KATANA_SUPPORT*/
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
             BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchMMRPEthertype, 
                                           MMRP_ETHERTYPE_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchSRPEthertype, 
                                           SRP_ETHERTYPE_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchMMRPDestMacOui, 
                                           MMRP_MAC_OUI_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchSRPDestMacOui, 
                                           SRP_MAC_OUI_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchMMRPDestMacNonOui, 
                                           MMRP_MAC_NONOUI_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchSRPDestMacNonOui, 
                                           SRP_MAC_NONOUI_DEFAULT));
        } else 
#endif
        {  
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchMMRPEthertype, 
                                           MMRP_ETHERTYPE_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchSRPEthertype, 
                                           SRP_ETHERTYPE_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchTimeSyncEthertype, 
                                           TS_ETHERTYPE_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchMMRPDestMacOui, 
                                           MMRP_MAC_OUI_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchSRPDestMacOui, 
                                           SRP_MAC_OUI_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchTimeSyncDestMacOui, 
                                           TS_MAC_OUI_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchMMRPDestMacNonOui, 
                                           MMRP_MAC_NONOUI_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchSRPDestMacNonOui, 
                                           SRP_MAC_NONOUI_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchTimeSyncDestMacNonOui, 
                                           TS_MAC_NONOUI_DEFAULT)); 
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_set(unit, bcmSwitchTimeSyncMessageTypeBitmap, 
                                           TS_MAC_MSGBITMAP_DEFAULT)); 
        }
    }
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        BCM_IF_ERROR_RETURN(
            bcm_td2_switch_hash_entry_init(unit));
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Egress copy-to-cpu Enable */
    if (soc_feature(unit, soc_feature_field_egress_tocpu)) {
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_set(unit, 
               bcmSwitchFieldStageEgressToCpu, EP_COPY_TOCPU_DEFAULT));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_switch_detach
 * Description:
 *      Detach switch control module, free all dynamically allocated memories
 * Parameters:
 *      unit        - Device unit number
 * Returns:
 *      BCM_E_xxx
 */
int 
_bcm_esw_switch_detach(int unit)
{

    if (_mod_type_table[unit] != NULL) {
        sal_free((void *)_mod_type_table[unit]);
        _mod_type_table[unit] = NULL;
    }

#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        BCM_IF_ERROR_RETURN(
            bcm_td2_switch_hash_entry_detach(unit));
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_switch_stable_register
 * Description:
 *      Register read/write functions for the application provided stable for 
 *      Level 2 Warm Boot
 * Parameters:
 *      unit - Device unit number
 *      rf   - Read function signature to register for the application provided 
 *             stable for Level 2 Warm Boot.
 *      wf   - Write function signature to register for the application provided 
 *             stable for Level 2 Warm Boot. 
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_esw_switch_stable_register(int unit, bcm_switch_read_func_t rf, 
                               bcm_switch_write_func_t wf)
{
#ifdef BCM_WARM_BOOT_SUPPORT
    return soc_switch_stable_register(unit, (soc_read_func_t)rf, 
                                      (soc_write_func_t)wf,
                                      NULL, NULL);
#else
    return BCM_E_UNAVAIL;
#endif
}


/*
 * Function:
 *      bcm_switch_event_register
 * Description:
 *      Registers a call back function for switch critical events
 * Parameters:
 *      unit        - Device unit number
 *  cb          - The desired call back function to register for critical events.
 *  userdata    - Pointer to any user data to carry on.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      
 *      Several call back functions could be registered, they all will be called upon
 *      critical event. If registered callback is called it is adviced to log the 
 *  information and reset the chip. 
 *  Same call back function with different userdata is allowed to be registered. 
 */
int 
bcm_esw_switch_event_register(int unit, bcm_switch_event_cb_t cb, 
                              void *userdata)
{
    return soc_event_register(unit, (soc_event_cb_t)cb, userdata);
}


/*
 * Function:
 *      bcm_switch_event_unregister
 * Description:
 *      Unregisters a call back function for switch critical events
 * Parameters:
 *      unit        - Device unit number
 *  cb          - The desired call back function to unregister for critical events.
 *  userdata    - Pointer to any user data associated with a call back function
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      
 *  If userdata = NULL then all matched call back functions will be unregistered,
 */
int 
bcm_esw_switch_event_unregister(int unit, bcm_switch_event_cb_t cb, 
                                void *userdata)
{
    return soc_event_unregister(unit, (soc_event_cb_t)cb, userdata);
}

#if defined(BCM_RCPU_SUPPORT)
/*
 * Function:
 *      _bcm_esw_switch_rcpu_encap_input_validate
 * Description:
 *      Helper function to validate input of API calls
 * Parameters:
 *      unit                - Device unit number
 *  flags               - RCPU encapsulation flags BCM_SWITCH_REMOTE_CPU_ENCAP_XXX
 *  internal_cpu_pri    - Priority to set 
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_esw_switch_rcpu_encap_input_validate(int unit, uint32 flags, 
                                          int internal_cpu_pri)
{
    if (!soc_feature(unit, soc_feature_rcpu_priority)) {
        return BCM_E_UNAVAIL;
    }
    /* Priority mapping is mutual exclusive */
    if ((flags & BCM_SWITCH_REMOTE_CPU_ENCAP_IEEE) && 
        (flags & BCM_SWITCH_REMOTE_CPU_ENCAP_HIGIG2)) {
       return BCM_E_PARAM;
    }

    if (internal_cpu_pri < 0 || internal_cpu_pri > NUM_CPU_COSQ(unit)) {
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}
#endif /* BCM_RCPU_SUPPORT */
/*
 * Function:
 *      bcm_esw_switch_rcpu_encap_priority_map_set
 * Description:
 *      Configure encapsulation priority for the internal priority queue.
 * Parameters:
 *      unit                -(IN) Device unit number
 *  flags               -(IN) Flags BCM_SWITCH_REMOTE_CPU_ENCAP_XXX
 *  interntal_cpu_pri   -(IN) Internal CPU priority queue
 *  encap_pri           -(IN) Encapsulation priority
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      
 *  flags = 0 means disable priority mapping
 */
int 
bcm_esw_switch_rcpu_encap_priority_map_set(int unit, uint32 flags, 
                                           int internal_cpu_pri, 
                                           int encap_pri)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_RCPU_SUPPORT)

    soc_field_t efields[3] = {IEEE_802_1_PRI_MAP_ENABLEf, MH_TC_MAP_ENABLEf, CPU_TC_ENABLEf};
    soc_field_t fields[3] = {IEEE_802_1_Pf, MH_TCf, CPU_TCf};
    uint32      values[3] = {0,0,0}, evalues[3] = {0,0,0};
    int         ieee_idx = 0, hg2_idx = 1, cpu_tc_idx = 2; /* index to values array */
    soc_reg_t   read_reg = CMIC_PKT_PRI_MAP_TABLEr;
    soc_reg_t   regs[] = {
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_0r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_1r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_2r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_3r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_3r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_5r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_4r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_7r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_8r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_9r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_10r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_11r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_12r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_13r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_13r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_15r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_14r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_17r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_28r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_29r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_20r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_21r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_22r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_23r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_23r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_25r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_24r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_27r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_28r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_29r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_30r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_31r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_32r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_33r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_33r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_35r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_34r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_37r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_38r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_39r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_40r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_41r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_42r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_43r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_43r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_45r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_44r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_47r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_48r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_49r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_50r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_51r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_52r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_53r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_53r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_55r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_54r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_57r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_58r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_59r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_60r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_61r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_62r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_63r
    };
    
    BCM_IF_ERROR_RETURN(
      _bcm_esw_switch_rcpu_encap_input_validate(unit, flags, internal_cpu_pri)); 

    if (encap_pri < 0)  {
        return BCM_E_PARAM;
    }

    if (soc_feature(unit, soc_feature_cmicm)) {
        read_reg = regs[0];
    }

    if (!flags) {   /* Empty flags mean disable mapping */
        
        values[ieee_idx] = evalues[ieee_idx] = 0 ;
        values[hg2_idx] = evalues[hg2_idx] = 0;
        values[cpu_tc_idx] = evalues[cpu_tc_idx] = 0;

    } else if (flags & BCM_SWITCH_REMOTE_CPU_ENCAP_IEEE) {
        int max_flen = (1 << soc_reg_field_length(unit, read_reg, IEEE_802_1_Pf)) - 1;

        if (encap_pri > max_flen) {
            return BCM_E_PARAM;
        }
        values[hg2_idx] = evalues[hg2_idx] = 0;
        values[cpu_tc_idx] = evalues[cpu_tc_idx] = 0;
        values[ieee_idx] = encap_pri;
        evalues[ieee_idx] = 1;

    } else if (flags & BCM_SWITCH_REMOTE_CPU_ENCAP_HIGIG2) {
        int max_flen = (1 << soc_reg_field_length(unit, read_reg, MH_TCf)) - 1;

        if (encap_pri > max_flen) {
            return BCM_E_PARAM;
        }
        values[ieee_idx] = evalues[ieee_idx] = 0;
        values[cpu_tc_idx] = evalues[cpu_tc_idx] = 0 ;
        values[hg2_idx] = encap_pri; 
        evalues[hg2_idx] = 1;
    } else if (flags & BCM_SWITCH_REMOTE_CPU_ENCAP_IEEE_CPU_TC) {
        int max_flen = (1 << soc_reg_field_length(unit, read_reg, CPU_TCf)) - 1;

        if (encap_pri > max_flen) {
            return BCM_E_PARAM;
        }
        values[ieee_idx] = evalues[ieee_idx] = 0 ;
        values[hg2_idx] = evalues[hg2_idx] = 0;
        values[cpu_tc_idx] = encap_pri; 
        evalues[cpu_tc_idx] = 1;
    } else {    /* Some illegal combination passed */
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(
        soc_reg_fields32_modify(unit, CMIC_PKT_CTRLr, REG_PORT_ANY, 
                                COUNTOF(efields), efields, evalues));
    if (soc_feature(unit, soc_feature_cmicm)) {
        BCM_IF_ERROR_RETURN
            (soc_reg_fields32_modify(unit, regs[internal_cpu_pri],
                                     REG_PORT_ANY, COUNTOF(fields), fields,
                                     values));
    } else {
        BCM_IF_ERROR_RETURN(
            soc_reg_fields32_modify(unit, CMIC_PKT_PRI_MAP_TABLEr, 
                                    internal_cpu_pri, COUNTOF(fields), 
                                    fields, values));
    }
    rv = BCM_E_NONE;

#endif /* BCM_RCPU_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_esw_switch_rcpu_encap_priority_map_get
 * Description:
 *      Retrieve encapsulation priority of the internal priority queue.
 * Parameters:
 *  unit                -(IN) Device unit number
 *  flags               -(IN) Flags BCM_SWITCH_REMOTE_CPU_ENCAP_XXX
 *  interntal_cpu_pri   -(IN) Internal CPU priority queue
 *  encap_pri           -(OUT) Encapsulation priority
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      
 *  encap_pri = -1 means disabled priority mapping
 */

int 
bcm_esw_switch_rcpu_encap_priority_map_get(int unit, uint32 flags, 
                                           int internal_cpu_pri, 
                                           int *encap_pri)
{

    int rv = BCM_E_UNAVAIL;
#if defined(BCM_RCPU_SUPPORT)
    uint32  regval, pkt_ctrl;
    soc_reg_t   read_reg = CMIC_PKT_PRI_MAP_TABLEr;
    soc_reg_t   regs[] = {
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_0r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_1r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_2r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_3r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_3r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_5r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_4r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_7r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_8r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_9r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_10r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_11r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_12r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_13r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_13r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_15r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_14r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_17r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_28r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_29r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_20r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_21r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_22r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_23r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_23r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_25r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_24r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_27r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_28r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_29r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_30r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_31r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_32r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_33r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_33r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_35r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_34r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_37r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_38r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_39r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_40r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_41r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_42r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_43r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_43r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_45r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_44r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_47r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_48r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_49r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_50r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_51r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_52r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_53r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_53r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_55r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_54r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_57r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_58r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_59r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_60r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_61r,
        CMIC_PKT_PRI_MAP_TABLE_ENTRY_62r, CMIC_PKT_PRI_MAP_TABLE_ENTRY_63r
    };

    BCM_IF_ERROR_RETURN(
      _bcm_esw_switch_rcpu_encap_input_validate(unit, flags, internal_cpu_pri)); 

    if (NULL == encap_pri)  {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(READ_CMIC_PKT_CTRLr(unit, &pkt_ctrl));

    if (soc_feature(unit, soc_feature_cmicm)) {
        BCM_IF_ERROR_RETURN(
            soc_pci_getreg(unit, soc_reg_addr(unit, regs[internal_cpu_pri],
                            REG_PORT_ANY, 0), &regval));
        read_reg = regs[internal_cpu_pri];
    } else {
        BCM_IF_ERROR_RETURN(
            READ_CMIC_PKT_PRI_MAP_TABLEr(unit, internal_cpu_pri, &regval));
    }

    if (flags & BCM_SWITCH_REMOTE_CPU_ENCAP_IEEE) {
        if (!soc_reg_field_get(unit, CMIC_PKT_CTRLr, pkt_ctrl,
                               IEEE_802_1_PRI_MAP_ENABLEf)) {
            *encap_pri = -1;
        } else {
            *encap_pri = soc_reg_field_get(unit, read_reg, regval, IEEE_802_1_Pf); 
        }
    } else if (flags & BCM_SWITCH_REMOTE_CPU_ENCAP_HIGIG2) {
        if (!soc_reg_field_get(unit, CMIC_PKT_CTRLr, pkt_ctrl, MH_TC_MAP_ENABLEf)) {
            *encap_pri = -1;
        } else {
            *encap_pri = soc_reg_field_get(unit, read_reg, regval, MH_TCf); 
        }
    } else if (flags & BCM_SWITCH_REMOTE_CPU_ENCAP_IEEE_CPU_TC) {
        if (!soc_reg_field_get(unit, CMIC_PKT_CTRLr, pkt_ctrl, CPU_TC_ENABLEf)) {
            *encap_pri = -1;
        } else {
            *encap_pri = soc_reg_field_get(unit, read_reg, regval, CPU_TCf); 
        }
    } else {  /* Some illegal combination passed, on get empty flags illegal */
        return BCM_E_PARAM;
    }
    
    rv = BCM_E_NONE;

#endif /* BCM_RCPU_SUPPORT */

    return rv;
}

/*
 * Function:
 *      bcm_esw_switch_rcpu_decap_priority_map_set
 * Description:
 *      Configure decapsulation priority for the internal priority queue.
 * Parameters:
 *  unit                -(IN) Device unit number
 *  decap_pri           -(IN) CPU traffic class in CMIC header
 *  interntal_cpu_pri   -(IN) Internal CPU priority queue

 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      
 *  flags = 0 means disable priority mapping
 */
int 
bcm_esw_switch_rcpu_decap_priority_map_set(int unit, 
                                           int decap_pri,
                                           int internal_cpu_pri)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_RCPU_SUPPORT)
    cpu_ts_map_entry_t cpu_ts_map_entry;

    if (!soc_feature(unit, soc_feature_rcpu_tc_mapping)) {
        return BCM_E_UNAVAIL;
    }

    if (decap_pri < 0 || decap_pri >= soc_mem_index_count(unit, CPU_TS_MAPm)) {
        return BCM_E_PARAM;
    }

    if (internal_cpu_pri < 0 || internal_cpu_pri >= NUM_CPU_COSQ(unit)) {
        return BCM_E_PARAM;
    }

    sal_memset(&cpu_ts_map_entry, 0, sizeof(cpu_ts_map_entry_t));

    soc_CPU_TS_MAPm_field32_set(unit, &cpu_ts_map_entry, CPU_QUEUE_IDf, internal_cpu_pri);

    BCM_IF_ERROR_RETURN(
        WRITE_CPU_TS_MAPm(unit, MEM_BLOCK_ALL, decap_pri, &cpu_ts_map_entry));

    rv = BCM_E_NONE;
#endif /* BCM_RCPU_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_esw_switch_rcpu_decap_priority_map_get
 * Description:
 *      Retrieve decapsulation priority of the internal priority queue.
 * Parameters:
 *  unit                -(IN) Device unit number
 *  decap_pri           -(IN) CPU traffic class in CMIC header
 *  interntal_cpu_pri   -(OUT) Internal CPU priority queue
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      
 *  encap_pri = -1 means disabled priority mapping
 */

int 
bcm_esw_switch_rcpu_decap_priority_map_get(int unit,
                                           int decap_pri,
                                           int *internal_cpu_pri)
{
    int rv = BCM_E_UNAVAIL;
#if defined(BCM_RCPU_SUPPORT)
    cpu_ts_map_entry_t cpu_ts_map_entry;

    if (!soc_feature(unit, soc_feature_rcpu_tc_mapping)) {
        return BCM_E_UNAVAIL;
    }

    if (decap_pri < 0 || decap_pri >= soc_mem_index_count(unit, CPU_TS_MAPm)) {
        return BCM_E_PARAM;
    }

    if (NULL == internal_cpu_pri)  {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(
        READ_CPU_TS_MAPm(unit, MEM_BLOCK_ALL, decap_pri, &cpu_ts_map_entry));

    *internal_cpu_pri = soc_CPU_TS_MAPm_field32_get(unit, &cpu_ts_map_entry,
                           CPU_QUEUE_IDf);

    rv = BCM_E_NONE;
#endif /* BCM_RCPU_SUPPORT */
    return rv;
}

/*
 * Function: 
 *      bcm_esw_switch_hash_entry_create
 * Purpose: 
 *      Create a blank flex hash entry.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      group      - (IN)  BCM field group
 *      *entry     - (OUT) BCM hash entry
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_switch_hash_entry_create (int unit, 
                                 bcm_field_group_t group, 
                                 bcm_hash_entry_t *entry)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcm_td2_switch_hash_entry_create (unit, group, entry);
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function: 
 *      bcm_esw_switch_hash_entry_destroy
 * Purpose: 
 *      Destroy a flex hash entry.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      entry      - (IN) BCM hash entry
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_switch_hash_entry_destroy (int unit, 
                                  bcm_hash_entry_t entry)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcm_td2_switch_hash_entry_destroy (unit, entry);
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function: 
 *      bcm_esw_switch_hash_entry_install
 * Purpose: 
 *      Install a flex hash entry into hardware tables.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      entry      - (IN) BCM hash entry
 *      offset     - (IN) BCM hash offset
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_switch_hash_entry_install (int unit, 
                                  bcm_hash_entry_t entry, 
                                  uint32 offset)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcm_td2_switch_hash_entry_install (unit, entry, offset);
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function: 
 *      bcm_esw_switch_hash_entry_reinstall
 * Purpose: 
 *      Re-install a flex hash entry into hardware tables.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      entry      - (IN) BCM hash entry
 *      offset     - (IN) BCM hash offset
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_switch_hash_entry_reinstall (int unit, 
                                    bcm_hash_entry_t entry, 
                                    uint32 offset)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcm_td2_switch_hash_entry_reinstall (unit, entry, offset);
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function: 
 *      bcm_esw_switch_hash_entry_remove
 * Purpose: 
 *      Remove a flex hash entry from hardware tables.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      entry      - (IN) BCM hash entry
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_switch_hash_entry_remove (int unit, 
                                 bcm_hash_entry_t entry)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcm_td2_switch_hash_entry_remove (unit, entry);
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function: 
 *      bcm_esw_switch_hash_qualify_data
 * Purpose: 
 *      Add flex hash field qualifiers into hardware tables.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      entry      - (IN) BCM hash entry
 *      qual_id    - (IN) BCM qualifier id
 *      data       - (IN) BCM hash field qualifier
 *      mask       - (IN) BCM hash field mask
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */

int 
bcm_esw_switch_hash_qualify_data (int unit, 
                                 bcm_hash_entry_t entry, 
                                 int qual_id, 
                                 uint32 data, 
                                 uint32 mask)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcm_td2_switch_hash_qualify_data (unit, entry, 
                                        qual_id, data, mask);
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    return BCM_E_UNAVAIL;
}


/* 
 * Gets the maximum number of banks associated with a particular hash
 * memory.
 */
int 
bcm_esw_switch_hash_banks_max_get(int unit, 
                                  bcm_switch_hash_table_t hash_table, 
                                  uint32 *bank_count)
{
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_shared_hash_mem)) {
        soc_mem_t mem;
        int num_banks;
        switch (hash_table) {
        case bcmHashTableL2:
            mem = L2Xm;
            break;
        case bcmHashTableL3:
            mem = L3_ENTRY_ONLYm;
            break;
        default:
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN
            (soc_trident2_hash_bank_count_get(unit, mem, &num_banks));
        *bank_count = num_banks;
        return BCM_E_NONE;
    } else 
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_ISM_SUPPORT
    if (soc_feature(unit, soc_feature_ism_memory)) {
        soc_ism_mem_type_t table;
        uint8 count;
        switch (hash_table) {
        case bcmHashTableL2:
            table = SOC_ISM_MEM_L2_ENTRY;
            break;
        case bcmHashTableL3:
            table = SOC_ISM_MEM_L3_ENTRY;
            break;
        case bcmHashTableVlanTranslate:
            table = SOC_ISM_MEM_VLAN_XLATE;
            break;
        case bcmHashTableEgressVlanTranslate:
            table = SOC_ISM_MEM_EP_VLAN_XLATE;
            break;
        case bcmHashTableMPLS:
            table = SOC_ISM_MEM_MPLS;
            break;
        default:
            return BCM_E_PARAM;
        }
        if (soc_ism_get_banks(unit, table, NULL, NULL, &count)) {
            return BCM_E_INTERNAL;
        }
        *bank_count = count;
        return BCM_E_NONE;
    } else 
#endif
    {
        return BCM_E_UNAVAIL;
    }
}

/* Configure the hash type for a bank of the particular hash memory. */
int 
bcm_esw_switch_hash_banks_config_set(int unit, 
                                     bcm_switch_hash_table_t hash_table, 
                                     uint32 bank_num, int hash_type, 
                                     uint32 hash_offset)
{
#ifdef BCM_ISM_SUPPORT
    if (soc_feature(unit, soc_feature_ism_memory)) {
        uint8 count, banks[_SOC_ISM_MAX_BANKS];
        uint8 zero = FALSE, offset;
        uint32 bank_size[_SOC_ISM_MAX_BANKS];
        int ofs;
        soc_ism_mem_type_t table;
        switch (hash_table) {
        case bcmHashTableL2:
            table = SOC_ISM_MEM_L2_ENTRY;
            break;
        case bcmHashTableL3:
            table = SOC_ISM_MEM_L3_ENTRY;
            break;
        case bcmHashTableVlanTranslate:
            table = SOC_ISM_MEM_VLAN_XLATE;
            break;
        case bcmHashTableEgressVlanTranslate:
            table = SOC_ISM_MEM_EP_VLAN_XLATE;
            break;
        case bcmHashTableMPLS:
            table = SOC_ISM_MEM_MPLS;
            break;
        default:
            return BCM_E_PARAM;
        }
        if (soc_ism_get_banks(unit, table, banks, bank_size, &count)) {
            return BCM_E_INTERNAL;
        }
        if (bank_num >= count) {
            return BCM_E_PARAM;
        }
        switch (hash_type) {
        case BCM_HASH_ZERO:
            offset = 48;
            zero = TRUE;
            break;
        case BCM_HASH_LSB:
            offset = 48;
            break;
        case BCM_HASH_CRC16L:
            offset = 32;
            break;
        case BCM_HASH_CRC16U:
            ofs = soc_ism_get_hash_bits(bank_size[bank_num]);
            if (ofs == SOC_E_PARAM) {
                return BCM_E_INTERNAL;
            }
            offset = 48 - ofs;
            break;
        case BCM_HASH_CRC32L:
            offset = 0;
            break;
        case BCM_HASH_CRC32U:
            ofs = soc_ism_get_hash_bits(bank_size[bank_num]);
            if (ofs == SOC_E_PARAM) {
                return BCM_E_INTERNAL;
            }
            offset = 32 - ofs;
            break;
        case BCM_HASH_OFFSET:
            if (hash_offset > 63) {
                return BCM_E_PARAM;
            }
            offset = (uint8)hash_offset;
            break;
        default:
            return BCM_E_PARAM;
        }
        if (soc_ism_hash_offset_config(unit, banks[bank_num], offset)) {
            return BCM_E_INTERNAL;
        }
        if (zero || (offset == 63)) {
            /* Clear lsb_zero for the table  */
            if (soc_ism_table_hash_config(unit, table, 0)) {
                return BCM_E_INTERNAL;
            } 
        } else {
            /* Set lsb_zero for the table  */
            if (soc_ism_table_hash_config(unit, table, 1)) {
                return BCM_E_INTERNAL;
            }
        }
        return BCM_E_NONE;
    } else 
#endif /* BCM_ISM_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_shared_hash_mem)) {
        int ofs = 14, use_lsb = 1, offset;
        uint8 zero = FALSE;
        soc_mem_t mem;
        int phy_bank_num;
        
        switch (hash_table) {
        case bcmHashTableL2:
            mem = L2Xm;
            break;
        case bcmHashTableL3:
            mem = L3_ENTRY_ONLYm;
            break;
        default:
            return BCM_E_PARAM;
        }
        SOC_IF_ERROR_RETURN
            (soc_trident2_hash_bank_number_get(unit, mem, bank_num,
                                               &phy_bank_num));
        switch (hash_type) {
        case BCM_HASH_ZERO:
            offset = 48;
            zero = TRUE;
            break;
        case BCM_HASH_LSB:
            offset = 48;
            break;
        case BCM_HASH_CRC16L:
            offset = 32;
            break;
        case BCM_HASH_CRC32L:
            offset = 0;
            break;
        case BCM_HASH_OFFSET:
            if (hash_offset > 63) {
                return BCM_E_PARAM;
            }
            offset = hash_offset;
            break;
        case BCM_HASH_CRC16U:
        case BCM_HASH_CRC32U:
            if (mem == L2Xm) {
                if (phy_bank_num < 2) {
                    ofs = 12;
                }
            } else {
                if (phy_bank_num > 5) {
                    ofs = 10;
                }
            }
            if (hash_type == BCM_HASH_CRC16U) {
                offset = 48 - ofs;
            } else {
                offset = 32 - ofs;
            }    
            break;
        default:
            return BCM_E_PARAM;
        }
        if (zero || (offset == 63)) {
            use_lsb = 0; 
        }
        return soc_td2_hash_offset_set(unit, mem, phy_bank_num, offset,
                                       use_lsb);
    } else 
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        return BCM_E_UNAVAIL;
    }
}

/* 
 * Gets the configured hash type for a bank of the particular hash
 * memory.
 */
int 
bcm_esw_switch_hash_banks_config_get(int unit, 
                                     bcm_switch_hash_table_t hash_table, 
                                     uint32 bank_num, int *hash_type, 
                                     uint32 *hash_offset)
{
#ifdef BCM_ISM_SUPPORT
    if (soc_feature(unit, soc_feature_ism_memory)) {
        uint8 count, banks[_SOC_ISM_MAX_BANKS];
        uint8 offset;
        uint32 bank_size[_SOC_ISM_MAX_BANKS];
        int ofs;
        soc_ism_mem_type_t table;
        
        switch (hash_table) {
        case bcmHashTableL2:
            table = SOC_ISM_MEM_L2_ENTRY;
            break;
        case bcmHashTableL3:
            table = SOC_ISM_MEM_L3_ENTRY;
            break;
        case bcmHashTableVlanTranslate:
            table = SOC_ISM_MEM_VLAN_XLATE;
            break;
        case bcmHashTableEgressVlanTranslate:
            table = SOC_ISM_MEM_EP_VLAN_XLATE;
            break;
        case bcmHashTableMPLS:
            table = SOC_ISM_MEM_MPLS;
            break;
        default:
            return BCM_E_PARAM;
        }
        if (soc_ism_get_banks(unit, table, banks, bank_size, &count)) {
            return BCM_E_INTERNAL;
        }
        if (bank_num >= count) {
            return BCM_E_PARAM;
        }
        if (soc_ism_hash_offset_config_get(unit, banks[bank_num], &offset)) {
            return BCM_E_INTERNAL;
        }
        switch (offset) {
        case 0: *hash_type = BCM_HASH_CRC32L;
            break;
        case 32: *hash_type = BCM_HASH_CRC16L;
            break;
        case 63: *hash_type = BCM_HASH_ZERO;
            break;
        case 48: *hash_type = BCM_HASH_LSB;
            break;
        default:
            ofs = soc_ism_get_hash_bits(bank_size[bank_num]);
            if (ofs == SOC_E_PARAM) {
                return BCM_E_INTERNAL;
            }
            if (32 - ofs == offset) {
                *hash_type = BCM_HASH_CRC32U;
            } else if (48 - ofs == offset) {
                *hash_type = BCM_HASH_CRC16U;
            } else {
                *hash_type = BCM_HASH_OFFSET;
            }
        }
        *hash_offset = offset;
        return BCM_E_NONE;
    } else 
#endif /* BCM_ISM_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_shared_hash_mem)) {
        soc_mem_t mem;
        int ofs = 14, offset, use_lsb;
        int phy_bank_num;
        
        switch (hash_table) {
        case bcmHashTableL2:
            mem = L2Xm;
            break;
        case bcmHashTableL3:
            mem = L3_ENTRY_ONLYm;
            break;
        default:
            return BCM_E_PARAM;
        }
        SOC_IF_ERROR_RETURN
            (soc_trident2_hash_bank_number_get(unit, mem, bank_num,
                                               &phy_bank_num));
        SOC_IF_ERROR_RETURN
            (soc_td2_hash_offset_get(unit, mem, phy_bank_num, &offset,
                                     &use_lsb));
        switch (offset) {
        case 0: *hash_type = BCM_HASH_CRC32L;
            break;
        case 32: *hash_type = BCM_HASH_CRC16L;
            break;
        case 63: *hash_type = BCM_HASH_ZERO;
            break;
        case 48: *hash_type = BCM_HASH_LSB;
            break;
        default:
            if (mem == L2Xm) {
                if (phy_bank_num < 2) {
                    ofs = 12;
                }
            } else {
                if (phy_bank_num > 5) {
                    ofs = 10;
                }
            }
            if (32 - ofs == offset) {
                *hash_type = BCM_HASH_CRC32U;
            } else if (48 - ofs == offset) {
                *hash_type = BCM_HASH_CRC16U;
            } else {
                *hash_type = BCM_HASH_OFFSET;
            }
        }
        *hash_offset = offset;
        return BCM_E_NONE;
    } else 
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        return BCM_E_UNAVAIL;
    }
}

/*
 * Function:
 *      bcm_esw_switch_pkt_info_hash_get
 * Purpose:
 *      Gets the hash result for the specified link aggregation method
 *      using provided packet parameters and device configuration.
 * Parameters:
 *      unit - (IN) Unit number.
 *      pkt_info - (IN) Packet parameter information for hash calculation.
 *      dst_gport - (OUT) Destination module and port.
 *      dst_intf - (OUT) Destination L3 interface ID egress object.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_switch_pkt_info_hash_get(int unit, bcm_switch_pkt_info_t *pkt_info,
                                 bcm_gport_t *dst_gport, bcm_if_t *dst_intf)
{
    if (SOC_IS_TRIDENT(unit)) {
        return _bcm_switch_pkt_info_ecmp_hash_get(unit, pkt_info,
                                                  dst_gport, dst_intf);
    }
    else if (SOC_IS_TRIDENT2(unit)) {
        return _bcm_td2_switch_pkt_info_hash_get(unit, pkt_info,
                                                 dst_gport, dst_intf);
    }
  
    return BCM_E_UNAVAIL; 
}

/*
 * Function:
 *      bcm_esw_switch_temperature_monitor_get
 * Purpose:
 *      There are temperature monitors embedded on the various points of the
 *      switch chip for the purpose of monitoring the health of the chip. 
 *      This routine retrieves each temperature monitor's current value and 
 *      peak value. The unit is 0.1 celsius degree.
 * Parameters:
 *      unit - (IN) Unit number.
 *      temperature_max - (IN) max number of entries of the temperature_array.
 *      temperature_array - (OUT) the buffer array to hold the retrived values.
 *      temperature_count - (OUT) number of actual entries retrieved.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int
bcm_esw_switch_temperature_monitor_get(int unit, 
          int temperature_max,
          bcm_switch_temperature_monitor_t *temperature_array, 
          int *temperature_count)
{
    int rv = BCM_E_UNAVAIL;

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        rv = soc_trident2_temperature_monitor_get(unit,
               temperature_max,temperature_array,temperature_count);
    } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        rv = soc_trident_temperature_monitor_get(unit,
               temperature_max,temperature_array,temperature_count);
    } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        rv = soc_tr3_temperature_monitor_get(unit,
               temperature_max,temperature_array,temperature_count);
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        rv = soc_katana_temperature_monitor_get(unit,
               temperature_max,temperature_array,temperature_count);
    } else
#endif /* BCM_KATANA_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        rv = soc_kt2_temperature_monitor_get(unit,
               temperature_max,temperature_array,temperature_count);
    } else
#endif /* BCM_KATANA_SUPPORT */
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        rv = soc_shadow_temperature_monitor_get(unit,
               temperature_max,temperature_array,temperature_count);
    } else
#endif
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        rv = soc_hu2_temperature_monitor_get(unit,
                temperature_max,temperature_array,temperature_count);
    } else
#endif
    {}

    return rv;
}

/*
 * Function:
 *    bcm_esw_switch_network_group_config_get
 * Parameters: 
 *    unit                   (IN)device number 
 *    source_network_group_id (IN) source split horizon group ID 
 *    config                 (IN/OUT) network group config information 
 * Purpose: 
 *    This API is used for enable/disable ingress pruning, egress pruning and
 *    ingress IPMC group remap for a given pair of
 *    source split horizon group and destination split horizon group.
 *    Split horizon group 0 is reserved for default case.
 *    Katana2 (BCM5645x) family of devices support 8 split horizon group.
 * Returns:
 *    BCM_E_XXX
 */

int
bcm_esw_switch_network_group_config_get(int unit,
    bcm_switch_network_group_t source_network_group_id,
    bcm_switch_network_group_config_t *config)
{
    int rv = BCM_E_NONE;
    int network_group_depth = 0;
    soc_mem_t mem;
    soc_field_t field;
    bcm_pbmp_t prune_pbmp;
    ing_network_prune_control_entry_t ing_entry;
    egr_network_prune_control_entry_t egr_entry;

    if (!soc_feature(unit, soc_feature_multiple_split_horizon_group)) {
        return BCM_E_UNAVAIL;
    }

    if (config == NULL) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
        bcmSwitchNetworkGroupDepth, &network_group_depth));

    if (network_group_depth <= 0) {
        return BCM_E_UNAVAIL;
    }

    if (source_network_group_id >= network_group_depth) {
        return BCM_E_PARAM;
    }

    if (config->dest_network_group_id >= network_group_depth) {
        return BCM_E_PARAM;
    }

    config->config_flags &=
        ~(BCM_SWITCH_NETWORK_GROUP_MCAST_REMAP_ENABLE |
          BCM_SWITCH_NETWORK_GROUP_INGRESS_PRUNE_ENABLE |
          BCM_SWITCH_NETWORK_GROUP_EGRESS_PRUNE_ENABLE);

    BCM_PBMP_CLEAR(prune_pbmp);
    mem = ING_NETWORK_PRUNE_CONTROLm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, COPYNO_ALL,
            source_network_group_id, &ing_entry));
        field = REMAP_IPMC_GROUPf;
        if (SOC_MEM_FIELD_VALID(unit, mem, field)) {
            if (soc_mem_field32_get(unit, mem, &ing_entry, field)) {
                config->config_flags |=
                    BCM_SWITCH_NETWORK_GROUP_MCAST_REMAP_ENABLE;
            }
        }

        field = PRUNE_ENABLE_BITMAPf;
        if (SOC_MEM_FIELD_VALID(unit, mem, field)) {
            soc_mem_pbmp_field_get(unit, mem, &ing_entry, field, &prune_pbmp);
            if (BCM_PBMP_MEMBER(prune_pbmp, config->dest_network_group_id)) {
                config->config_flags |=
                    BCM_SWITCH_NETWORK_GROUP_INGRESS_PRUNE_ENABLE;
            }
        }
    }

    BCM_PBMP_CLEAR(prune_pbmp);
    mem = EGR_NETWORK_PRUNE_CONTROLm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, COPYNO_ALL,
            source_network_group_id, &egr_entry));
        field = PRUNE_ENABLE_BITMAPf;
        if (SOC_MEM_FIELD_VALID(unit, mem, field)) {
            soc_mem_pbmp_field_get(unit, mem, &egr_entry, field, &prune_pbmp);
            if (BCM_PBMP_MEMBER(prune_pbmp, config->dest_network_group_id)) {
                config->config_flags |=
                    BCM_SWITCH_NETWORK_GROUP_EGRESS_PRUNE_ENABLE;
            }
        }
    }
    return rv;
}

/*
 * Function:
 *    bcm_esw_switch_network_group_config_set
 * Parameters:
 *    unit                   (IN)device number 
 *    source_network_group_id (IN) source split horizon group ID 
 *    config                 (IN) network group config information 
 * Purpose:
 *    This API is used to get the enable/disable status of ingress pruning, egress pruning and
 *    ingress IPMC group remap for a given pair of
 *    source split horizon group and destination split horizon group.
 *    Katana2 (BCM5645x) family of devices support 8 split horizon group.
 * Returns:
 *    BCM_E_XXX
 */
int
bcm_esw_switch_network_group_config_set(int unit,
    bcm_switch_network_group_t source_network_group_id,
    bcm_switch_network_group_config_t *config)
{
    int rv = BCM_E_NONE;
    int network_group_depth = 0;
    soc_mem_t mem;
    soc_field_t field;
    bcm_pbmp_t prune_pbmp;
    ing_network_prune_control_entry_t ing_entry;
    egr_network_prune_control_entry_t egr_entry;

    if (!soc_feature(unit, soc_feature_multiple_split_horizon_group)) {
        return BCM_E_UNAVAIL;
    }

    if (config == NULL) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
        bcmSwitchNetworkGroupDepth, &network_group_depth));

    if (network_group_depth <= 0) {
        return BCM_E_UNAVAIL;
    }

    /* network group id 0 is reserved */
    if ((source_network_group_id == 0) ||
        (source_network_group_id >= network_group_depth)) {
        return BCM_E_PARAM;
    }

    if (config->dest_network_group_id >= network_group_depth) {
        return BCM_E_PARAM;
    }

    BCM_PBMP_CLEAR(prune_pbmp);
    mem = ING_NETWORK_PRUNE_CONTROLm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, COPYNO_ALL,
            source_network_group_id, &ing_entry));

        field = REMAP_IPMC_GROUPf;
        if (SOC_MEM_FIELD_VALID(unit, mem, field)) {
            soc_mem_field32_set(unit, mem, &ing_entry, field,
                (config->config_flags &
                BCM_SWITCH_NETWORK_GROUP_MCAST_REMAP_ENABLE) ? 1 : 0);
        }

        field = PRUNE_ENABLE_BITMAPf;
        if (SOC_MEM_FIELD_VALID(unit, mem, field)) {
            soc_mem_pbmp_field_get(unit, mem, &ing_entry, field, &prune_pbmp);
            if (config->config_flags &
                    BCM_SWITCH_NETWORK_GROUP_INGRESS_PRUNE_ENABLE) {
                BCM_PBMP_PORT_ADD(prune_pbmp, config->dest_network_group_id);
            } else {
                BCM_PBMP_PORT_REMOVE(prune_pbmp, config->dest_network_group_id);
            }
            soc_mem_pbmp_field_set(unit, mem, &ing_entry, field, &prune_pbmp);
        }

        BCM_IF_ERROR_RETURN(soc_mem_write(unit, mem, COPYNO_ALL,
            source_network_group_id, &ing_entry));
    }

    BCM_PBMP_CLEAR(prune_pbmp);
    mem = EGR_NETWORK_PRUNE_CONTROLm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, COPYNO_ALL,
            source_network_group_id, &egr_entry));

        field = PRUNE_ENABLE_BITMAPf;
        if (SOC_MEM_FIELD_VALID(unit, mem, field)) {
            soc_mem_pbmp_field_get(unit, mem, &egr_entry, field, &prune_pbmp);
            if (config->config_flags &
                    BCM_SWITCH_NETWORK_GROUP_EGRESS_PRUNE_ENABLE) {
                BCM_PBMP_PORT_ADD(prune_pbmp, config->dest_network_group_id);
            } else {
                BCM_PBMP_PORT_REMOVE(prune_pbmp, config->dest_network_group_id);
            }
            soc_mem_pbmp_field_set(unit, mem, &egr_entry, field, &prune_pbmp);
        }

        BCM_IF_ERROR_RETURN(soc_mem_write(unit, mem, COPYNO_ALL,
            source_network_group_id, &egr_entry));
    }
    return rv;

}
