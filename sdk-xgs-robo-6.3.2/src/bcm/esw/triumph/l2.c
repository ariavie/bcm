/*
 * $Id: l2.c 1.290.6.3 Broadcom SDK $
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
 * File:        l2.c
 * Purpose:     Triumph L2 function implementations
 */

#include <soc/defs.h>

#if defined(BCM_TRX_SUPPORT)

#include <assert.h>

#include <sal/core/libc.h>

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/hash.h>
#include <soc/l2x.h>
#include <soc/triumph.h>
#ifdef BCM_TRIDENT2_SUPPORT
#include <soc/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */

#include <bcm/l2.h>
#include <bcm/error.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/l2.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/mpls.h>
#include <bcm_int/esw/mim.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/trunk.h>
#include <bcm_int/common/multicast.h>
#include <bcm_int/esw/multicast.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/trident.h>
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <bcm_int/esw/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */

#if defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw/subport.h>
#include <bcm_int/esw/katana2.h>

#define _BCM_PORT_MAC_MAX  170

#define _BCM_OLP_MAC_MAX   16

#define _BCM_XGS_MAC_MAX   1

#define _BCM_L2_STATION_ID_KT2_MAX  (0x1000000)

#define _BCM_L2_STATION_ID_KT2_PORT_MAC_MIN  (_BCM_L2_STATION_ID_KT2_MAX + 1)

#define _BCM_L2_STATION_ID_KT2_PORT_MAC_MAX  \
             (_BCM_L2_STATION_ID_KT2_MAX + _BCM_L2_STATION_ID_KT2_MAX)

#define _BCM_L2_STATION_ID_KT2_OLP_MAC_MIN \
             (_BCM_L2_STATION_ID_KT2_PORT_MAC_MAX + 1)

#define _BCM_L2_STATION_ID_KT2_OLP_MAC_MAX  \
             (_BCM_L2_STATION_ID_KT2_PORT_MAC_MAX + _BCM_L2_STATION_ID_KT2_MAX )

#define _BCM_L2_STATION_ID_KT2_XGS_MAC_MIN \
             (_BCM_L2_STATION_ID_KT2_OLP_MAC_MAX + 1)

#define _BCM_L2_STATION_ID_KT2_XGS_MAC_MAX \
             (_BCM_L2_STATION_ID_KT2_OLP_MAC_MAX + _BCM_L2_STATION_ID_KT2_MAX)

#define DGLP_MODULE_ID_MASK                   0x7f80
#define DGLP_PORT_NO_MASK                     0x7f
#define DGLP_MODULE_ID_SHIFT_BITS             7
#define DGLP_LAG_ID_INDICATOR_SHIFT_BITS      15
#define MAC_MASK_HIGH                         0xffff
#define MAC_MASK_LOW                          0xffffffff
#define OLP_MAC_MASK                          0x1f
#define OLP_LOWEST_MAC                        0
#define OLP_HIGHEST_MAC                       1
#define OLP_MAX_MAC_DIFF                      16
#define OLP_ACTION_ADD                        1
#define OLP_ACTION_UPDATE                     2
#define OLP_ACTION_DELETE                     3
#endif /* BCM_KATANA2_SUPPORT */

#if defined(BCM_TRIUMPH_SUPPORT) /* BCM_TRIUMPH_SUPPORT */

#define GPORT_TYPE(_gport) (((_gport) >> _SHR_GPORT_TYPE_SHIFT) & _SHR_GPORT_TYPE_MASK)

#define _BCM_L2_STATION_ID_BASE (1)
#define _BCM_L2_STATION_ID_MAX  (0x1000000)

#define _BCM_L2_STATION_ENTRY_INSTALLED      (1 << 0)

#define _BCM_L2_STATION_ENTRY_PRIO_NO_CHANGE (1 << 1)

#define _BCM_L2_STATION_ENTRY_TYPE_NON_TCAM  (1 << 2)

#define _BCM_L2_STATION_TERM_FLAGS_MASK    (BCM_L2_STATION_IPV4       \
                                            | BCM_L2_STATION_IPV6     \
                                            | BCM_L2_STATION_ARP_RARP \
                                            | BCM_L2_STATION_MPLS     \
                                            | BCM_L2_STATION_MIM      \
                                            | BCM_L2_STATION_TRILL    \
                                            | BCM_L2_STATION_FCOE     \
                                            | BCM_L2_STATION_OAM)

/*
 * Macro:
 *     ParamMax
 * Purpose:
 *     Determine maximum value that can be stored in the field.
 */
#define ParamMax(_unit_, _mem_, _field_)                                      \
            ((soc_mem_field_length((_unit_), (_mem_) , (_field_)) < 32) ?     \
            ((1 << soc_mem_field_length((_unit_), (_mem_), (_field_))) - 1) : \
            (0xFFFFFFFFUL))                             
/*
 * Macro:
 *     ParamCheck
 * Purpose:
 *     Check if value can fit in the field and return error if it exceeds
 *     the range. 
 */
#define ParamCheck(_unit_, _mem_, _field_, _value_)                          \
        do {                                                                 \
            if (0 == ((uint32)(_value_) <=                                   \
                (uint32)ParamMax((_unit_), (_mem_), (_field_)))) {           \
                    L2_ERR(("L2(unit %d) Error: _value_ %d > %d (max)"       \
                            " mem (%d) field (%d).\n", _unit_, (_value_),    \
                            (uint32)ParamMax((_unit_), (_mem_), (_field_)),  \
                            (_mem_), (_field_)));                            \
                    return (BCM_E_PARAM);                                    \
            }                                                                \
        } while(0)

/*
 * Macro:
 *     SC_LOCK
 * Purpose:
 *     Lock take the Field control mutex
 */
#define SC_LOCK(control) \
            sal_mutex_take((control)->sc_lock, sal_mutex_FOREVER)

/*
 * Macro:
 *     FP_UNLOCK
 * Purpose:
 *     Lock take the Field control mutex
 */
#define SC_UNLOCK(control) \
            sal_mutex_give((control)->sc_lock);

STATIC _bcm_l2_station_control_t *_station_control[BCM_MAX_NUM_UNITS];
STATIC uint32 last_allocated_sid;
int prio_with_no_free_entries = FALSE;

#endif /* !BCM_TRIUMPH_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
STATIC uint32 last_allocated_olp_sid;
STATIC uint32 last_allocated_port_mac_sid;
#endif /* BCM_KATANA2_SUPPORT */
#define DEFAULT_L2DELETE_CHUNKS		64	/* 16k entries / 64 = 256 */

typedef struct _bcm_mac_block_info_s {
    bcm_pbmp_t mb_pbmp;
    int ref_count;
} _bcm_mac_block_info_t;

static _bcm_mac_block_info_t *_mbi_entries[BCM_MAX_NUM_UNITS];
static int _mbi_num[BCM_MAX_NUM_UNITS];

#define L2_DEBUG(flags, stuff)  BCM_DEBUG(flags | BCM_DBG_L2, stuff)
#define L2_ERR(stuff)           L2_DEBUG(BCM_DBG_ERR, stuff)
#define L2_WARN(stuff)          L2_DEBUG(BCM_DBG_WARN, stuff)
#define L2_OUT(stuff)          BCM_DEBUG(BCM_DBG_L2, stuff)
#define L2_VERB(stuff)         L2_DEBUG(BCM_DBG_VERBOSE, stuff)
#define L2_VVERB(stuff)        L2_DEBUG(BCM_DBG_VVERBOSE, stuff)
#define L2_SHOW(stuff)         ((*soc_cm_print) stuff)

#if defined(BCM_SCORPION_SUPPORT) && defined(INCLUDE_L3)
extern int16 * _sc_subport_group_index[BCM_MAX_NUM_UNITS];
#define _SC_SUBPORT_NUM_PORT  (4096)
#define _SC_SUBPORT_NUM_GROUP (4096/8)
#define _SC_SUBPORT_VPG_FIND(unit, vp, grp) \
    do { \
         int ix; \
         grp = -1; \
         for (ix = 0; ix < _SC_SUBPORT_NUM_GROUP; ix++) { \
              if (_sc_subport_group_index[unit][ix] == vp) { \
                  grp = ix * 8; \
                  break;  \
              } \
         } \
       } while ((0))
#endif /* BCM_SCORPION_SUPPORT && INCLUDE_L3 */
#ifdef PLISIM
#ifdef BCM_KATANA_SUPPORT
extern void _bcm_kt_enable_port_age_simulation(uint32 flags, _bcm_l2_replace_t *rep_st);
#endif
#endif

#if defined(BCM_TRIUMPH_SUPPORT)
/*
 * Function:
 *     _bcm_l2_station_tcam_mem_get
 * Purpose:
 *     Get device specific station TCAM memory name.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     tcam_mem  - (OUT) Station TCAM memory name.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_tcam_mem_get(int unit, soc_mem_t *tcam_mem)
{
    if (NULL == tcam_mem) {
        return (BCM_E_PARAM);
    }

    if (SOC_IS_ENDURO(unit) || SOC_IS_TRIUMPH(unit)
        || SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit)
        || SOC_IS_VALKYRIE(unit) || SOC_IS_VALKYRIE2(unit)
        || SOC_IS_HURRICANEX(unit)) {
        
        *tcam_mem = MPLS_STATION_TCAMm;

    } else if (SOC_IS_KATANAX(unit) || SOC_IS_TRIDENT(unit)
               || SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit)) {

        *tcam_mem = MY_STATION_TCAMm;

    } else {

        *tcam_mem = INVALIDm;

        return (BCM_E_UNAVAIL);
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_l2_station_control_deinit
 * Purpose:
 *     Uninitialize device station control information.
 * Parameters:
 *     unit  - (IN) BCM device number
 * Retruns:
 *     BCM_E_XXX
 */
int
_bcm_tr_l2_station_control_deinit(int unit)
{
    _bcm_l2_station_control_t  *sc; /* Station control structure pointer. */

    sc = _station_control[unit];
    if (NULL == sc) {
        return (BCM_E_NONE);
    }

    BCM_IF_ERROR_RETURN
        (bcm_tr_l2_station_delete_all(unit));

    if (NULL != sc->entry_arr) {
        sal_free(sc->entry_arr);
    }

    if (NULL != sc->sc_lock) {
        sal_mutex_destroy(sc->sc_lock);
    }

    _station_control[unit] = NULL;

    sal_free(sc);

    return (BCM_E_NONE);

}

/*
 * Function:
 *     _bcm_l2_station_control_init
 * Purpose:
 *     Initialize device station control information.
 * Parameters:
 *     unit  - (IN) BCM device number
 * Retruns:
 *     BCM_E_XXX
 */
int
_bcm_tr_l2_station_control_init(int unit)
{
    int                        rv;          /* Operation return status.     */
    _bcm_l2_station_control_t  *sc = NULL;  /* Station control structure.   */
    soc_mem_t                  tcam_mem;    /* TCAM memory name.            */
    uint32                     mem_size;    /* Size of entry buffer.        */
    uint32                     max_entries; /* Max no. of entries in table. */
    int                        i;           /* Temporary iterator variable. */

    /* Deinit if control has already been initialized. */
    if (NULL != _station_control[unit]) {
        rv = _bcm_tr_l2_station_control_deinit(unit);
        if (BCM_FAILURE(rv)) {
            return (rv);
        }
    }

    /* Allocate the station control memory for this unit. */
    sc = (_bcm_l2_station_control_t *) sal_alloc
            (sizeof(_bcm_l2_station_control_t), "L2 station control");
    if (NULL == sc) {
        return (BCM_E_MEMORY);
    }

    sal_memset(sc, 0, sizeof(_bcm_l2_station_control_t));

    rv = _bcm_l2_station_tcam_mem_get(unit, &tcam_mem);
    if (BCM_FAILURE(rv)) {
        sal_free(sc);
        return (rv);
    }

    last_allocated_sid = 0;

    max_entries = soc_mem_index_count(unit, tcam_mem);

    sc->entries_total = max_entries;

    sc->entries_free = max_entries;

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {

        last_allocated_olp_sid = _BCM_L2_STATION_ID_KT2_OLP_MAC_MIN;

        last_allocated_port_mac_sid = _BCM_L2_STATION_ID_KT2_PORT_MAC_MIN;

        sc->port_entries_total =_BCM_PORT_MAC_MAX;

        sc->port_entries_free =_BCM_PORT_MAC_MAX;

        sc->olp_entries_total = _BCM_OLP_MAC_MAX; 

        sc->olp_entries_free = _BCM_OLP_MAC_MAX; 
    }
#endif

    sc->entry_count = 0;
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        max_entries = sc->entries_total + sc->port_entries_total +
                      sc->olp_entries_total + 1;
    }
#endif

    mem_size = (max_entries * sizeof(_bcm_l2_station_entry_t *));

    sc->entry_arr = (_bcm_l2_station_entry_t **)
                        sal_alloc(mem_size, "L2 station entry pointers");
    if (NULL == sc->entry_arr) {
        sal_free(sc);
        return (BCM_E_MEMORY);
    }

    for (i = 0; i < max_entries; i++) {
        sc->entry_arr[i] = NULL;
    }

    sc->sc_lock = sal_mutex_create("L2 station control.lock");
    if (NULL == sc->sc_lock) {
        sal_free(sc->entry_arr);
        sal_free(sc);
        return (BCM_E_MEMORY);
    }

    _station_control[unit] = sc;

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_l2_station_control_get
 * Purpose:
 *     Get device station control structure pointer.
 * Parameters:
 *     unit  - (IN) BCM device number
 *     sc    - (OUT) Device station structure pointer. 
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_control_get(int unit, _bcm_l2_station_control_t **sc)
{

    /* Input parameter check. */
    if (NULL == sc) {
        return (BCM_E_PARAM);
    }

    if (NULL == _station_control[unit]) {
        return (BCM_E_INIT);
    }

    *sc = _station_control[unit];

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_l2_station_entry_get
 * Purpose:
 *     Get station entry details by performing
 *     lookup using station ID.
 * Parameters:
 *     unit  - (IN) BCM device number
 *     sid   - (IN) Station ID.
 *     ent_p - (OUT) Station look up result.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_entry_get(int unit,
                          int sid,
                          _bcm_l2_station_entry_t **ent_p)
{
    _bcm_l2_station_control_t *sc;   /* Station control structure pointer. */
    int                       index; /* Entry index.                       */
    int                       count;
    /* Input parameter check. */
    if (NULL == ent_p) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    count = sc->entries_total;
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        count = sc->entries_total + sc->port_entries_total + 
                                          sc->olp_entries_total + 1;
    }
#endif 
    for (index = 0; index < count; index++) {

        if (NULL == sc->entry_arr[index]) {
            continue;
        }

        if (sid == sc->entry_arr[index]->sid) {
            *ent_p = sc->entry_arr[index];
            L2_VVERB(("L2(unit %d) Info: (SID=%d) - found (idx=%d).\n",
                     unit, sid, index));
            return (BCM_E_NONE);
        }
    }

    L2_VVERB(("L2(unit %d) Info: (SID=%d) - not found (idx=%d).\n",
             unit, sid, index));

    /* L2 station entry with ID == sid not found. */
    return (BCM_E_NOT_FOUND);
}
#endif
#ifdef BCM_KATANA2_SUPPORT
STATIC int
_bcm_kt2_l2_station_id_get(int *sid, 
                           int *station_id,
                           bcm_l2_station_t *station)
{
    int rv = BCM_E_NONE;
    if (station->flags & BCM_L2_STATION_WITH_ID) {
        if (((station->flags & BCM_L2_STATION_OAM) &&
           ((*station_id < _BCM_L2_STATION_ID_KT2_PORT_MAC_MIN) && 
           (*station_id > _BCM_L2_STATION_ID_KT2_PORT_MAC_MAX))) ||
           ((station->flags & BCM_L2_STATION_OLP) &&
           ((*station_id < _BCM_L2_STATION_ID_KT2_OLP_MAC_MIN) && 
           (*station_id > _BCM_L2_STATION_ID_KT2_OLP_MAC_MAX))) ||
           ((station->flags & BCM_L2_STATION_XGS_MAC) &&
           ((*station_id < _BCM_L2_STATION_ID_KT2_XGS_MAC_MIN) && 
           (*station_id > _BCM_L2_STATION_ID_KT2_XGS_MAC_MAX)))) {
           rv = BCM_E_PARAM;
        } else {
           *sid = *station_id;
        }
    } else {
        if (station->flags & BCM_L2_STATION_OAM) {
            *sid = ++last_allocated_port_mac_sid;
            if (_BCM_L2_STATION_ID_KT2_PORT_MAC_MAX == *sid) {
               last_allocated_port_mac_sid = _BCM_L2_STATION_ID_KT2_PORT_MAC_MIN;
            }
        } else if (station->flags & BCM_L2_STATION_OLP) {
            *sid = ++last_allocated_olp_sid;
            if (_BCM_L2_STATION_ID_KT2_OLP_MAC_MAX == *sid) {
               last_allocated_olp_sid = _BCM_L2_STATION_ID_KT2_OLP_MAC_MIN;
            }
        } else if (station->flags & BCM_L2_STATION_XGS_MAC) {
            *sid = _BCM_L2_STATION_ID_KT2_XGS_MAC_MIN;
        } else {
            *sid = ++last_allocated_sid;

            if (_BCM_L2_STATION_ID_KT2_MAX == *sid) {
                last_allocated_sid = _BCM_L2_STATION_ID_BASE;
            }
        }
        *station_id = *sid;
    }
    return rv;
}
#endif /* BCM_KATANA2_SUPPORT */
/*
 * Function:
 *      _bcm_tr_l2_from_l2x
 * Purpose:
 *      Convert a Triumph L2X entry to an L2 API data structure
 * Parameters:
 *      unit        Unit number
 *      l2addr      (OUT) L2 API data structure
 *      l2x_entry   Triumph L2X entry
 */
int
_bcm_tr_l2_from_l2x(int unit, bcm_l2_addr_t *l2addr, l2x_entry_t *l2x_entry)
{
    int l2mc_index, mb_index, vfi, rval;

    sal_memset(l2addr, 0, sizeof(*l2addr));

    soc_L2Xm_mac_addr_get(unit, l2x_entry, MAC_ADDRf, l2addr->mac);

    if (soc_L2Xm_field32_get(unit, l2x_entry, KEY_TYPEf) ==
        TR_L2_HASH_KEY_TYPE_VFI) {
        vfi = soc_L2Xm_field32_get(unit, l2x_entry, VFIf);
        COMPILER_REFERENCE(vfi);
        /* VPLS or MIM VPN */
#if defined(INCLUDE_L3)
        if (_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeMpls)) {
            _BCM_MPLS_VPN_SET(l2addr->vid, _BCM_MPLS_VPN_TYPE_VPLS, vfi);
        } else if (_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeMim)) {
            _BCM_MIM_VPN_SET(l2addr->vid, _BCM_MIM_VPN_TYPE_MIM, vfi);
        } else  if (_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeL2Gre)) {
#if defined(BCM_TRIUMPH3_SUPPORT)
                if (soc_feature(unit, soc_feature_l2gre)) {
                    BCM_IF_ERROR_RETURN
                        (_bcm_tr3_l2gre_vpn_get(unit, vfi, &l2addr->vid));
                }
#endif
        } else if (_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeVxlan)) {
#if defined(BCM_TRIDENT2_SUPPORT)
                if (soc_feature(unit, soc_feature_vxlan)) {
                    BCM_IF_ERROR_RETURN(
                      _bcm_td2_vxlan_vpn_get(unit, vfi, &l2addr->vid));
                }
#endif
        }
#endif /* INCLUDE_L3 */
    } else {  
        l2addr->vid = soc_L2Xm_field32_get(unit, l2x_entry, VLAN_IDf); 
    }

    if (BCM_MAC_IS_MCAST(l2addr->mac)) {
        l2addr->flags |= BCM_L2_MCAST;
        l2mc_index = soc_L2Xm_field32_get(unit, l2x_entry, L2MC_PTRf);
        if (soc_mem_field_valid(unit, L2Xm, VPG_TYPEf) &&
            soc_L2Xm_field32_get(unit, l2x_entry, VPG_TYPEf)) {
            if (soc_L2Xm_field32_get(unit, l2x_entry, KEY_TYPEf) ==
                                               TR_L2_HASH_KEY_TYPE_VFI) {
                vfi = soc_L2Xm_field32_get(unit, l2x_entry, VFIf);
                /* VPLS, MIM, L2GRE, VXLAN multicast */
#if defined(INCLUDE_L3)
                if (_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeMpls)) {
                    _BCM_MULTICAST_GROUP_SET(l2addr->l2mc_index,
                            _BCM_MULTICAST_TYPE_VPLS, l2mc_index);
                } else if (_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeMim)) {
                    _BCM_MULTICAST_GROUP_SET(l2addr->l2mc_index,
                            _BCM_MULTICAST_TYPE_MIM, l2mc_index);
                } else  if (_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeVxlan)) {
                    _BCM_MULTICAST_GROUP_SET(l2addr->l2mc_index,
                            _BCM_MULTICAST_TYPE_VXLAN, l2mc_index);
                }
#endif
            } else {
#if defined(INCLUDE_L3)
                int rv;
                rv = _bcm_tr_multicast_ipmc_group_type_get(unit,
                            l2mc_index, &l2addr->l2mc_index);
                if (BCM_E_NOT_FOUND == rv) {
                    /* Assume subport multicast */
                    _BCM_MULTICAST_GROUP_SET(l2addr->l2mc_index,
                            _BCM_MULTICAST_TYPE_SUBPORT, l2mc_index);
                } else if (BCM_FAILURE(rv)) {
                    return rv;
                }
#endif
            }
        } else {
            l2addr->l2mc_index = l2mc_index;
            /* Translate l2mc index */
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_get(unit, bcmSwitchL2McIdxRetType, &rval));
            if (rval) {
               _BCM_MULTICAST_GROUP_SET(l2addr->l2mc_index, _BCM_MULTICAST_TYPE_L2,
                                                                    l2addr->l2mc_index);
            }
        }
    } else {
        _bcm_gport_dest_t       dest;
        int                     isGport = 0;

        _bcm_gport_dest_t_init(&dest);
#if defined(INCLUDE_L3)
        if (soc_L2Xm_field32_get(unit, l2x_entry, DEST_TYPEf) == 2) {
            int vp;
            vp = soc_L2Xm_field32_get(unit, l2x_entry, DESTINATIONf);
            if (soc_L2Xm_field32_get(unit, l2x_entry, KEY_TYPEf) ==
                    TR_L2_HASH_KEY_TYPE_VFI) {
                /* MPLS/MiM virtual port unicast */
                if (_bcm_vp_used_get(unit, vp, _bcmVpTypeMpls)) {
                    dest.mpls_id = vp;
                    dest.gport_type = _SHR_GPORT_TYPE_MPLS_PORT;
                    isGport=1;
                } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeMim)) {
                    dest.mim_id = vp;
                    dest.gport_type = _SHR_GPORT_TYPE_MIM_PORT;
                    isGport=1;
                } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
                    dest.vxlan_id  = vp;
                    dest.gport_type = _SHR_GPORT_TYPE_VXLAN_PORT;
                    isGport=1;
                } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVpLag)) {
                    l2addr->flags |= BCM_L2_TRUNK_MEMBER;
                    BCM_IF_ERROR_RETURN(_bcm_esw_trunk_vp_lag_vp_to_tid(unit,
                                vp, &l2addr->tgid));
                    dest.tgid = l2addr->tgid;
                    dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
                    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                                bcmSwitchUseGport, &isGport));
                } else {
                    /* L2 entries with Stale VPN */
                    dest.gport_type = BCM_GPORT_INVALID;
                    isGport=0;
                }
            } else {
                /* Subport/WLAN unicast */
#if defined(BCM_SCORPION_SUPPORT) && defined(INCLUDE_L3)
                if (SOC_IS_SC_CQ(unit)) {
                    /* Scorpion uses index to L3_NEXT_HOP as VPG */
                    int grp;

                    _SC_SUBPORT_VPG_FIND(unit, vp, grp);
                    if ((vp = grp) == -1) {
                        L2_ERR(("Unit: %d can not find entry for VPG\n", unit));
                    }
                    dest.subport_id = vp;
                    dest.gport_type = _SHR_GPORT_TYPE_SUBPORT_GROUP;
                    isGport=1;
                } else
#endif /* BCM_SCORPION_SUPPORT && INCLUDE_L3 */
                {
                    if (_bcm_vp_used_get(unit, vp, _bcmVpTypeSubport)) {
                        dest.subport_id = vp;
                        dest.gport_type = _SHR_GPORT_TYPE_SUBPORT_GROUP;
                        isGport=1;
                    } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeWlan)) {
                        dest.wlan_id = vp;
                        dest.gport_type = _SHR_GPORT_TYPE_WLAN_PORT;
                        isGport=1;
                    } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
                        dest.vlan_vp_id = vp;
                        dest.gport_type = _SHR_GPORT_TYPE_VLAN_PORT;
                        isGport=1;
                    } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv)) {
                        dest.niv_id = vp;
                        dest.gport_type = _SHR_GPORT_TYPE_NIV_PORT;
                        isGport=1;
                    } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeTrill)) {
                        dest.trill_id  = vp;
                        dest.gport_type = _SHR_GPORT_TYPE_TRILL_PORT;
                        isGport=1;
                    } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                        dest.extender_id = vp;
                        dest.gport_type = _SHR_GPORT_TYPE_EXTENDER_PORT;
                        isGport=1;
                    } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVpLag)) {
                        l2addr->flags |= BCM_L2_TRUNK_MEMBER;
                        BCM_IF_ERROR_RETURN(_bcm_esw_trunk_vp_lag_vp_to_tid(unit,
                                    vp, &l2addr->tgid));
                        dest.tgid = l2addr->tgid;
                        dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
                        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                                    bcmSwitchUseGport, &isGport));
                    } else {
                        /* L2 entries with Stale Gport */
                        dest.gport_type = BCM_GPORT_INVALID;
                        isGport=0;
                    }
                }
            }
        } else
#endif /* INCLUDE_L3 */
        if (soc_L2Xm_field32_get(unit, l2x_entry, Tf)) {
            /* Trunk group */
            int psc = 0; /* psc is dummy, not used */
            l2addr->flags |= BCM_L2_TRUNK_MEMBER;
            l2addr->tgid = soc_L2Xm_field32_get(unit, l2x_entry, TGIDf);
            bcm_esw_trunk_psc_get(unit, l2addr->tgid, &psc);
            if (soc_L2Xm_field32_get(unit, l2x_entry, REMOTE_TRUNKf)) {
                l2addr->flags |= BCM_L2_REMOTE_TRUNK;
            }
            dest.tgid = l2addr->tgid;
            dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_get(unit, bcmSwitchUseGport, &isGport));
        } else {
            bcm_module_t    mod_in, mod_out;
            bcm_port_t      port_in, port_out;

            port_in = soc_L2Xm_field32_get(unit, l2x_entry, PORT_NUMf);
            mod_in = soc_L2Xm_field32_get(unit, l2x_entry, MODULE_IDf);
            BCM_IF_ERROR_RETURN
                (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                        mod_in, port_in, &mod_out, &port_out));
            l2addr->modid = mod_out;
            l2addr->port = port_out;
            dest.port = l2addr->port;
            dest.modid = l2addr->modid;
            dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
            BCM_IF_ERROR_RETURN(
                bcm_esw_switch_control_get(unit, bcmSwitchUseGport, &isGport));
        }

        if (isGport) {
            BCM_IF_ERROR_RETURN(
                _bcm_esw_gport_construct(unit, &dest, &(l2addr->port)));
        }
    }

    if (SOC_MEM_FIELD_VALID(unit, L2Xm, L3f)) {
        if (soc_L2Xm_field32_get(unit, l2x_entry, L3f)) {
            l2addr->flags |= BCM_L2_L3LOOKUP;
        }
    }

    if (SOC_CONTROL(unit)->l2x_group_enable) {
        l2addr->group = soc_L2Xm_field32_get(unit, l2x_entry, CLASS_IDf);
    } else {
        mb_index = soc_L2Xm_field32_get(unit, l2x_entry, MAC_BLOCK_INDEXf);
        if (mb_index) {
            BCM_PBMP_ASSIGN(l2addr->block_bitmap,
                            _mbi_entries[unit][mb_index].mb_pbmp);
        }
    }

    if (soc_L2Xm_field32_get(unit, l2x_entry, RPEf)) {
        l2addr->flags |= BCM_L2_SETPRI;
    }

    l2addr->cos_dst = soc_L2Xm_field32_get(unit, l2x_entry, PRIf);
    l2addr->cos_src = soc_L2Xm_field32_get(unit, l2x_entry, PRIf);

    if (soc_L2Xm_field32_get(unit, l2x_entry, CPUf)) {
        l2addr->flags |= BCM_L2_COPY_TO_CPU;
    }

    if (soc_L2Xm_field32_get(unit, l2x_entry, DST_DISCARDf)) {
        l2addr->flags |= BCM_L2_DISCARD_DST;
    }

    if (soc_L2Xm_field32_get(unit, l2x_entry, SRC_DISCARDf)) {
        l2addr->flags |= BCM_L2_DISCARD_SRC;
    }

    if (soc_L2Xm_field32_get(unit, l2x_entry, SCPf)) {
        l2addr->flags |= BCM_L2_COS_SRC_PRI;
    }


    if (soc_L2Xm_field32_get(unit, l2x_entry, STATIC_BITf)) {
        l2addr->flags |= BCM_L2_STATIC;
    }

    if (SOC_MEM_FIELD_VALID(unit, L2Xm, LIMIT_COUNTEDf)) {
        if (!(l2addr->flags & (BCM_L2_L3LOOKUP | BCM_L2_MCAST |
                               BCM_L2_STATIC | BCM_L2_LEARN_LIMIT))) {
            if (!soc_L2Xm_field32_get(unit, l2x_entry, LIMIT_COUNTEDf)) {
                l2addr->flags |= BCM_L2_LEARN_LIMIT_EXEMPT;
            }
        }
    }

    if (soc_L2Xm_field32_get(unit, l2x_entry, PENDINGf)) {
        l2addr->flags |= BCM_L2_PENDING;
    }

    if (soc_L2Xm_field32_get(unit, l2x_entry, HITDAf)) {
        l2addr->flags |= BCM_L2_DES_HIT;
        l2addr->flags |= BCM_L2_HIT;
    }

    if (soc_L2Xm_field32_get(unit, l2x_entry, HITSAf)) {
        l2addr->flags |= BCM_L2_SRC_HIT;
        l2addr->flags |= BCM_L2_HIT;
    }

    if (SOC_MEM_FIELD_VALID(unit, L2Xm, LOCAL_SAf)) {
        if (soc_L2Xm_field32_get(unit, l2x_entry, LOCAL_SAf)) {
            l2addr->flags |= BCM_L2_NATIVE;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_l2_to_l2x
 * Purpose:
 *      Convert an L2 API data structure to a Triumph L2X entry
 * Parameters:
 *      unit        Unit number
 *      l2x_entry   (OUT) Triumph L2X entry
 *      l2addr      L2 API data structure
 *      key_only    Only construct key portion
 */
int
_bcm_tr_l2_to_l2x(int unit, l2x_entry_t *l2x_entry, bcm_l2_addr_t *l2addr,
                  int key_only)
{
    int vfi;
    int tid_is_vp_lag;

    if (l2addr->cos_dst < 0 || l2addr->cos_dst > 15) {
        return BCM_E_PARAM;
    }

    /*  BCM_L2_MIRROR is not supported starting from Triumph */
    if (l2addr->flags & BCM_L2_MIRROR) {
        return BCM_E_PARAM;
    }

    sal_memset(l2x_entry, 0, sizeof (*l2x_entry));

    if (_BCM_VPN_VFI_IS_SET(l2addr->vid)) {
        _BCM_VPN_GET(vfi, _BCM_VPN_TYPE_VFI, l2addr->vid);
        soc_L2Xm_field32_set(unit, l2x_entry, VFIf, vfi);
        soc_L2Xm_field32_set(unit, l2x_entry, KEY_TYPEf,
                             TR_L2_HASH_KEY_TYPE_VFI);
    } else {
         if (!_BCM_MPLS_VPN_IS_VPWS(l2addr->vid)) {
              VLAN_CHK_ID(unit, l2addr->vid);
              soc_L2Xm_field32_set(unit, l2x_entry, VLAN_IDf, l2addr->vid);
              soc_L2Xm_field32_set(unit, l2x_entry, KEY_TYPEf,
                             TR_L2_HASH_KEY_TYPE_BRIDGE);
         }
    }

    soc_L2Xm_mac_addr_set(unit, l2x_entry, MAC_ADDRf, l2addr->mac);

    if (key_only) {
        return BCM_E_NONE;
    }

    if (BCM_MAC_IS_MCAST(l2addr->mac)) {
        if (l2addr->l2mc_index < 0) {
            return BCM_E_PARAM;
        }
        if (_BCM_MULTICAST_IS_VPLS(l2addr->l2mc_index) ||
            _BCM_MULTICAST_IS_MIM(l2addr->l2mc_index) ||
            _BCM_MULTICAST_IS_WLAN(l2addr->l2mc_index) ||
            _BCM_MULTICAST_IS_VLAN(l2addr->l2mc_index) ||
            _BCM_MULTICAST_IS_NIV(l2addr->l2mc_index) ||
            _BCM_MULTICAST_IS_L2GRE(l2addr->l2mc_index) ||
            _BCM_MULTICAST_IS_VXLAN(l2addr->l2mc_index) ||
            _BCM_MULTICAST_IS_SUBPORT(l2addr->l2mc_index) ||
            _BCM_MULTICAST_IS_EXTENDER(l2addr->l2mc_index)) {
            soc_L2Xm_field32_set(unit, l2x_entry, DEST_TYPEf, 0x3);
        }

        if (_BCM_MULTICAST_IS_SET(l2addr->l2mc_index)) {
            soc_L2Xm_field32_set(unit, l2x_entry, DESTINATIONf,
                                 _BCM_MULTICAST_ID_GET(l2addr->l2mc_index));
        } else {
            soc_L2Xm_field32_set(unit, l2x_entry, L2MC_PTRf, l2addr->l2mc_index);
        }
    } else {
        bcm_port_t      port = -1;
        bcm_trunk_t     tgid = BCM_TRUNK_INVALID;
        bcm_module_t    modid = -1;
        int             gport_id = -1;
        int             vpg_type = 0;

        if (BCM_GPORT_IS_SET(l2addr->port)) {
            _bcm_l2_gport_params_t  g_params;

            if (BCM_GPORT_IS_BLACK_HOLE(l2addr->port)) {
                soc_L2Xm_field32_set(unit, l2x_entry, SRC_DISCARDf, 1);
            } else {
                soc_L2Xm_field32_set(unit, l2x_entry, SRC_DISCARDf, 0);
                BCM_IF_ERROR_RETURN(
                        _bcm_esw_l2_gport_parse(unit, l2addr, &g_params));

                switch (g_params.type) {
                    case _SHR_GPORT_TYPE_TRUNK:
                        tgid = g_params.param0;
                        break;
                    case  _SHR_GPORT_TYPE_MODPORT:
                        port = g_params.param0;
                        modid = g_params.param1;
                        break;
                    case _SHR_GPORT_TYPE_LOCAL_CPU:
                        port = g_params.param0;
                        BCM_IF_ERROR_RETURN(
                                bcm_esw_stk_my_modid_get(unit, &modid));
                        break;
                    case _SHR_GPORT_TYPE_SUBPORT_GROUP:
                        gport_id = g_params.param0;
#if defined(BCM_SCORPION_SUPPORT) && defined(INCLUDE_L3)
                        if (SOC_IS_SC_CQ(unit)) {
                            /* Map the sub_port to index to L3_NEXT_HOP */
                            gport_id = (int) _sc_subport_group_index[unit][gport_id/8];
                        }
#endif /* BCM_SCORPION_SUPPORT && INCLUDE_L3 */
                        vpg_type = 1;
                        break;
                case _SHR_GPORT_TYPE_SUBPORT_PORT:
#if defined(BCM_KATANA2_SUPPORT)
                    if (_BCM_KT2_GPORT_IS_LINKPHY_OR_SUBTAG_SUBPORT_GPORT(unit,
                        l2addr->port)) {
                        port = g_params.param0;
                        modid = g_params.param1;
                    } else
#endif
                    {
                        gport_id = g_params.param0;
                        vpg_type = 1;
                    }
                    break;
                    case _SHR_GPORT_TYPE_MPLS_PORT:
                    gport_id = g_params.param0;
                    vpg_type = 1;
                    break;
                    case _SHR_GPORT_TYPE_MIM_PORT:
                    case _SHR_GPORT_TYPE_WLAN_PORT:
                    case _SHR_GPORT_TYPE_VLAN_PORT:
                    case _SHR_GPORT_TYPE_NIV_PORT:
                    case _SHR_GPORT_TYPE_EXTENDER_PORT:
                        gport_id = g_params.param0;
                        vpg_type = 1;
                        break;
                    case _SHR_GPORT_TYPE_TRILL_PORT:
                    case _SHR_GPORT_TYPE_L2GRE_PORT:
                    case _SHR_GPORT_TYPE_VXLAN_PORT:

                        gport_id = g_params.param0;
                        vpg_type = 0;
                        break;
                    default:
                        return BCM_E_PORT;
                }
            }
        } else if (l2addr->flags & BCM_L2_TRUNK_MEMBER) {
            tgid = l2addr->tgid;

        } else {
            BCM_IF_ERROR_RETURN(
                _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET,
                                       l2addr->modid, l2addr->port,
                                       &modid, &port));
            /* Check parameters */
            if (!SOC_MODID_ADDRESSABLE(unit, modid)) {
                return BCM_E_BADID;
            }
            if (!SOC_PORT_ADDRESSABLE(unit, port)) {
                return BCM_E_PORT;
            }
        }

        /* Setting l2x_entry fields according to parameters */
        if ( BCM_TRUNK_INVALID != tgid) {
            BCM_IF_ERROR_RETURN(_bcm_esw_trunk_id_is_vp_lag(unit, tgid,
                            &tid_is_vp_lag));
            if (tid_is_vp_lag) {
                if (soc_feature(unit, soc_feature_vp_lag)) {
                    int vp_lag_vp;
                    /* Get the VP value representing VP LAG */
                    BCM_IF_ERROR_RETURN(_bcm_esw_trunk_tid_to_vp_lag_vp(unit,
                                tgid, &vp_lag_vp));
                    soc_L2Xm_field32_set(unit, l2x_entry, DEST_TYPEf, 0x2);
                    soc_L2Xm_field32_set(unit, l2x_entry, DESTINATIONf,
                            vp_lag_vp);
                } else {
                    return BCM_E_PORT;
                }
            } else {
                soc_L2Xm_field32_set(unit, l2x_entry, Tf, 1);
                soc_L2Xm_field32_set(unit, l2x_entry, TGIDf, tgid);
                /*
                 * Note:  RTAG is ignored here.  Use bcm_trunk_psc_set to
                 * to set for a given trunk.
                 */
                if (l2addr->flags & BCM_L2_REMOTE_TRUNK) {
                    soc_L2Xm_field32_set(unit, l2x_entry, REMOTE_TRUNKf, 1);
                }
            }
        } else if (-1 != port) {
            soc_L2Xm_field32_set(unit, l2x_entry, MODULE_IDf, modid);
            soc_L2Xm_field32_set(unit, l2x_entry, PORT_NUMf, port);
        } else if (-1 != gport_id) {
            soc_L2Xm_field32_set(unit, l2x_entry, DEST_TYPEf, 0x2);
            if (vpg_type) {
                soc_L2Xm_field32_set(unit, l2x_entry, VPGf, gport_id);
                soc_L2Xm_field32_set(unit, l2x_entry, VPG_TYPEf, vpg_type);
            } else {
                soc_L2Xm_field32_set(unit, l2x_entry, DESTINATIONf, gport_id);
            }
        }
    }

    if (SOC_MEM_FIELD_VALID(unit, L2Xm, L3f)) {
        if (l2addr->flags & BCM_L2_L3LOOKUP) {
            soc_L2Xm_field32_set(unit, l2x_entry, L3f, 1);
        }
    }

    if (SOC_CONTROL(unit)->l2x_group_enable) {
        soc_L2Xm_field32_set(unit, l2x_entry, CLASS_IDf, l2addr->group);
    } /* else MAC_BLOCK_INDEXf is handled in the add/remove functions below */

    if (l2addr->flags & BCM_L2_SETPRI) {
        soc_L2Xm_field32_set(unit, l2x_entry, RPEf, 1);
    }

    if (!SOC_IS_TRIUMPH3(unit)) {
        soc_L2Xm_field32_set(unit, l2x_entry, PRIf, l2addr->cos_dst);
    }

    if (l2addr->flags & BCM_L2_COPY_TO_CPU) {
        soc_L2Xm_field32_set(unit, l2x_entry, CPUf, 1);
    }

    if (l2addr->flags & BCM_L2_DISCARD_DST) {
        soc_L2Xm_field32_set(unit, l2x_entry, DST_DISCARDf, 1);
    }

    if (l2addr->flags & BCM_L2_DISCARD_SRC) {
        soc_L2Xm_field32_set(unit, l2x_entry, SRC_DISCARDf, 1);
    }

    if (l2addr->flags & BCM_L2_COS_SRC_PRI) {
        soc_L2Xm_field32_set(unit, l2x_entry, SCPf, 1);
    }

    if (SOC_MEM_FIELD_VALID(unit, L2Xm, LIMIT_COUNTEDf)) {
        if ((!(l2addr->flags & (BCM_L2_L3LOOKUP | BCM_L2_MCAST | BCM_L2_STATIC |
                               BCM_L2_LEARN_LIMIT_EXEMPT))) || 
            l2addr->flags & BCM_L2_LEARN_LIMIT) {
            soc_L2Xm_field32_set(unit, l2x_entry, LIMIT_COUNTEDf, 1);
        }
    }

    if (l2addr->flags & BCM_L2_PENDING) {
        soc_L2Xm_field32_set(unit, l2x_entry, PENDINGf, 1);
    }

    if (l2addr->flags & BCM_L2_STATIC) {
        soc_L2Xm_field32_set(unit, l2x_entry, STATIC_BITf, 1);
    }

    soc_L2Xm_field32_set(unit, l2x_entry, VALIDf, 1);

    if ((l2addr->flags & BCM_L2_DES_HIT) ||
        (l2addr->flags & BCM_L2_HIT)) {
        soc_L2Xm_field32_set(unit, l2x_entry, HITDAf, 1);
    }

    if ((l2addr->flags & BCM_L2_SRC_HIT) ||
        (l2addr->flags & BCM_L2_HIT)) {
        soc_L2Xm_field32_set(unit, l2x_entry, HITSAf, 1);
    }

    if (SOC_MEM_FIELD_VALID(unit, L2Xm, LOCAL_SAf)) {
        if (l2addr->flags & BCM_L2_NATIVE) {
            soc_L2Xm_field32_set(unit, l2x_entry, LOCAL_SAf, 1);
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_l2_from_ext_l2
 * Purpose:
 *      Convert an EXT_L2_ENTRY to an L2 API data structure

 * Parameters:
 *      unit         Unit number
 *      l2addr       (OUT) L2 API data structure
 *      ext_l2_entry EXT_L2_ENTRY hardware entry
 */
int
_bcm_tr_l2_from_ext_l2(int unit, bcm_l2_addr_t *l2addr,
                       ext_l2_entry_entry_t *ext_l2_entry)
{
    _bcm_gport_dest_t       dest;
    int                     mb_index, vfi;
    bcm_module_t            mod;
    bcm_port_t              port;
    int  isGport = 0;

    sal_memset(l2addr, 0, sizeof(*l2addr));

     if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry,
                             KEY_TYPE_VFIf) == 1) {
         vfi = soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, VFIf);
         COMPILER_REFERENCE(vfi);
         /* VPLS or MIM VPN */
#if defined(INCLUDE_L3)
         if (_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeMpls)) {
              _BCM_MPLS_VPN_SET(l2addr->vid, _BCM_MPLS_VPN_TYPE_VPLS, vfi);
         } else {
              _BCM_MIM_VPN_SET(l2addr->vid, _BCM_MIM_VPN_TYPE_MIM, vfi);
         }
#endif
    } else {
         l2addr->vid = soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry,
                                           VLAN_IDf);
    }

    soc_mem_mac_addr_get(unit, EXT_L2_ENTRYm, ext_l2_entry, MAC_ADDRf,
                         l2addr->mac);

    _bcm_gport_dest_t_init(&dest);
#if defined(INCLUDE_L3)
    if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, DEST_TYPEf) == 0x2) {
         int vp;

         vp = soc_mem_field32_get(unit, EXT_L2_ENTRYm,ext_l2_entry,  DESTINATIONf);
         if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, KEY_TYPE_VFIf) == 0x1) {
              /* MPLS/MiM virtual port unicast */
              if (_bcm_vp_used_get(unit, vp, _bcmVpTypeMpls)) {
                   dest.mpls_id = vp;
                   dest.gport_type = _SHR_GPORT_TYPE_MPLS_PORT;
                   isGport=1;
              } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeMim)) {
                   dest.mim_id = vp;
                   dest.gport_type = _SHR_GPORT_TYPE_MIM_PORT;
                   isGport=1;
              } else {
                   return BCM_E_INTERNAL; /* Cannot reach here */
              }
         } else {
             /* Subport/WLAN unicast */
             if (_bcm_vp_used_get(unit, vp, _bcmVpTypeSubport)) {
                 dest.subport_id = vp;
                 dest.gport_type = _SHR_GPORT_TYPE_SUBPORT_GROUP;
                 isGport=1;
             } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeWlan)) {
                 dest.wlan_id = vp;
                 dest.gport_type = _SHR_GPORT_TYPE_WLAN_PORT;
                 isGport=1;
             } else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
                 dest.vlan_vp_id = vp;
                 dest.gport_type = _SHR_GPORT_TYPE_VLAN_PORT;
                 isGport=1;
             } else {
                 return BCM_E_INTERNAL; /* Cannot reach here */
             }
         }
    } else {
#endif /* INCLUDE_L3 */
        if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, Tf)) {
            int psc = 0;
            l2addr->tgid = soc_mem_field32_get(unit, EXT_L2_ENTRYm,
                                               ext_l2_entry, TGIDf);
            bcm_esw_trunk_psc_get(unit, l2addr->tgid, &psc);
            dest.tgid = l2addr->tgid;
            dest.gport_type = _SHR_GPORT_TYPE_TRUNK;

            l2addr->flags |= BCM_L2_TRUNK_MEMBER;
            if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry,
                                    REMOTE_TRUNKf)) {
                l2addr->flags |= BCM_L2_REMOTE_TRUNK;
            }
        } else {
            mod = soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry,
                                      MODULE_IDf);
            port = soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry,
                                       PORT_NUMf);
            BCM_IF_ERROR_RETURN(_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                                       mod, port, &mod, &port));
            l2addr->modid = mod;
            l2addr->port = port;
            dest.port = l2addr->port;
            dest.modid = l2addr->modid;
            dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
        }
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit, bcmSwitchUseGport,
                                                      &isGport));
#if defined(INCLUDE_L3)
    }
#endif /* INCLUDE_L3 */
	
    if (isGport) {
         BCM_IF_ERROR_RETURN
             (_bcm_esw_gport_construct(unit, &dest, &l2addr->port));
    }

    if (SOC_CONTROL(unit)->l2x_group_enable) {
        l2addr->group = soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry,
                                            CLASS_IDf);
    } else {
        mb_index = soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry,
                                       MAC_BLOCK_INDEXf);
        if (mb_index) {
            BCM_PBMP_ASSIGN(l2addr->block_bitmap,
                            _mbi_entries[unit][mb_index].mb_pbmp);
        }
    }

    if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, RPEf)) {
        l2addr->flags |= BCM_L2_SETPRI;
    }

    l2addr->cos_dst = soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry,
                                          PRIf);
    l2addr->cos_src = l2addr->cos_dst;

    if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, CPUf)) {
        l2addr->flags |= BCM_L2_COPY_TO_CPU;
    }

    if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, DST_DISCARDf)) {
        l2addr->flags |= BCM_L2_DISCARD_DST;
    }

    if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, SRC_DISCARDf)) {
        l2addr->flags |= BCM_L2_DISCARD_SRC;
    }

    if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, SCPf)) {
        l2addr->flags |= BCM_L2_COS_SRC_PRI;
    }

    if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, STATIC_BITf)) {
        l2addr->flags |= BCM_L2_STATIC;
    }

    if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, DST_HITf)) {
        l2addr->flags |= BCM_L2_DES_HIT;
        l2addr->flags |= BCM_L2_HIT;
    }

    if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, ext_l2_entry, SRC_HITf)) {
        l2addr->flags |= BCM_L2_SRC_HIT;
        l2addr->flags |= BCM_L2_HIT;
    }

    if (SOC_MEM_FIELD_VALID(unit, EXT_L2_ENTRYm, LIMIT_COUNTEDf)) {
        if ((!(l2addr->flags & BCM_L2_STATIC)) || 
            l2addr->flags & BCM_L2_LEARN_LIMIT ) {
            if (!soc_L2Xm_field32_get(unit, ext_l2_entry, LIMIT_COUNTEDf)) {
                l2addr->flags |= BCM_L2_LEARN_LIMIT_EXEMPT;
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_l2_to_ext_l2
 * Purpose:
 *      Convert an L2 API data structure to an EXT_L2_ENTRY
 * Parameters:
 *      unit         Unit number
 *      ext_l2_entry (OUT) EXT_L2_ENTRY hardware entry
 *      l2addr       L2 API data structure
 *      key_only     Only construct key portion
 */
int
_bcm_tr_l2_to_ext_l2(int unit, ext_l2_entry_entry_t *ext_l2_entry,
                     bcm_l2_addr_t *l2addr, int key_only)
{
    _bcm_l2_gport_params_t  g_params;
    bcm_module_t            mod;
    bcm_port_t              port;
    uint32                  fval;

    /*  BCM_L2_MIRROR is not supported starting from Triumph */
    if (l2addr->flags & BCM_L2_MIRROR) {
        return BCM_E_PARAM;
    }

    sal_memset(ext_l2_entry, 0, sizeof(*ext_l2_entry));

    if (_BCM_MPLS_VPN_IS_VPLS(l2addr->vid)) {
        _BCM_MPLS_VPN_GET(fval, _BCM_MPLS_VPN_TYPE_VPLS, l2addr->vid);
        if (!SOC_MEM_FIELD32_VALUE_FIT(unit, EXT_L2_ENTRYm, VFIf, fval)) {
            return BCM_E_PARAM;
        }
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, VFIf, fval);
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, KEY_TYPE_VFIf,
                            1);
    } else if (_BCM_IS_MIM_VPN(l2addr->vid)) {
        _BCM_MIM_VPN_GET(fval, _BCM_MIM_VPN_TYPE_MIM,  l2addr->vid);
        if (!SOC_MEM_FIELD32_VALUE_FIT(unit, EXT_L2_ENTRYm, VFIf, fval)) {
            return BCM_E_PARAM;
        }
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, VFIf, fval);
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, KEY_TYPE_VFIf,
                            1);
    } else {
        if (!SOC_MEM_FIELD32_VALUE_FIT(unit, EXT_L2_ENTRYm, VLAN_IDf,
                                       l2addr->vid)) {
            return BCM_E_PARAM;
        }
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, VLAN_IDf,
                            l2addr->vid);
    }

    soc_mem_mac_addr_set(unit, EXT_L2_ENTRYm, ext_l2_entry, MAC_ADDRf,
                         l2addr->mac);

    if (key_only) {
        return BCM_E_NONE;
    }

    if (!BCM_GPORT_IS_SET(l2addr->port)) {
        if (l2addr->flags & BCM_L2_TRUNK_MEMBER) {
            g_params.param0 = l2addr->tgid;
            g_params.type = _SHR_GPORT_TYPE_TRUNK;
        } else {
            PORT_DUALMODID_VALID(unit, l2addr->port);
            BCM_IF_ERROR_RETURN(_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET,
                                                       l2addr->modid, l2addr->port,
                                                       &mod, &port));
            g_params.param0 = port;
            g_params.param1 = mod;
            g_params.type = _SHR_GPORT_TYPE_MODPORT;
        }
    } else {
        BCM_IF_ERROR_RETURN(_bcm_esw_l2_gport_parse(unit, l2addr, &g_params));
    }

    switch (g_params.type) {
    case _SHR_GPORT_TYPE_TRUNK:
        if (!SOC_MEM_FIELD32_VALUE_FIT(unit, EXT_L2_ENTRYm, TGIDf,
                                       g_params.param0)) {
            return BCM_E_PARAM;
        }
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, Tf, 1);
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, TGIDf,
                            g_params.param0);
        if (l2addr->flags & BCM_L2_REMOTE_TRUNK) {
            soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry,
                                REMOTE_TRUNKf, 1);
        }
        break;
    case _SHR_GPORT_TYPE_MODPORT:
        if (!SOC_MODID_ADDRESSABLE(unit, g_params.param1)) {
            return BCM_E_BADID;
        }
        if (!SOC_PORT_ADDRESSABLE(unit, g_params.param0)) {
            return BCM_E_PORT;
        }
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, MODULE_IDf,
                            g_params.param1);
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, PORT_NUMf,
                            g_params.param0);
        break;
    case _SHR_GPORT_TYPE_LOCAL_CPU:
        if (!SOC_PORT_ADDRESSABLE(unit, g_params.param0)) {
            return BCM_E_PORT;
        }
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, PORT_NUMf,
                            g_params.param0);
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &mod));
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, MODULE_IDf,
                            mod);
        break;
    case _SHR_GPORT_TYPE_MPLS_PORT:
        if (!SOC_MEM_FIELD32_VALUE_FIT(unit, EXT_L2_ENTRYm, DESTINATIONf,
                                       g_params.param0)) {
            return BCM_E_PARAM;
        }
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, DESTINATIONf,
                            g_params.param0);
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, DEST_TYPEf, 0x2);
        break;
    case _SHR_GPORT_TYPE_MIM_PORT:
    case _SHR_GPORT_TYPE_SUBPORT_GROUP:
    case _SHR_GPORT_TYPE_SUBPORT_PORT:
    case _SHR_GPORT_TYPE_VLAN_PORT:
        if (!SOC_MEM_FIELD32_VALUE_FIT(unit, EXT_L2_ENTRYm, VPGf,
                                       g_params.param0)) {
            return BCM_E_PARAM;
        }
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, VPGf,
                            g_params.param0);
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, VPG_TYPEf, 1);
        break;
    default:
        return BCM_E_PORT;
    }

    if (SOC_CONTROL(unit)->l2x_group_enable) {
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, CLASS_IDf,
                            l2addr->group);
    } /* else MAC_BLOCK_INDEXf is handled in the add/remove functions */

    if (l2addr->flags & BCM_L2_SETPRI) {
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, RPEf, 1);
    }

    if (!SOC_MEM_FIELD32_VALUE_FIT(unit, EXT_L2_ENTRYm, PRIf,
                                   l2addr->cos_dst)) {
        return BCM_E_PARAM;
    }
    soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, PRIf,
                        l2addr->cos_dst);

    if (l2addr->flags & BCM_L2_COPY_TO_CPU) {
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, CPUf, 1);
    }

    if (l2addr->flags & BCM_L2_DISCARD_DST) {
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, DST_DISCARDf,
                            1);
    }

    if (l2addr->flags & BCM_L2_DISCARD_SRC) {
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, SRC_DISCARDf,
                            1);
    }

    if (l2addr->flags & BCM_L2_COS_SRC_PRI) {
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, SCPf, 1);
    }

    if (l2addr->flags & BCM_L2_STATIC) {
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, STATIC_BITf, 1);
    }

    soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, VALIDf, 1);

    if ((l2addr->flags & BCM_L2_DES_HIT) ||
        (l2addr->flags & BCM_L2_HIT)) {
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, DST_HITf, 1);
    }

    if ((l2addr->flags & BCM_L2_SRC_HIT) ||
        (l2addr->flags & BCM_L2_HIT)) {
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, SRC_HITf, 1);
    }

    if (SOC_MEM_FIELD_VALID(unit, EXT_L2_ENTRYm, LIMIT_COUNTEDf)) {
        if ((!(l2addr->flags & (BCM_L2_L3LOOKUP | BCM_L2_MCAST | BCM_L2_STATIC |
                               BCM_L2_LEARN_LIMIT_EXEMPT))) || 
            l2addr->flags & BCM_L2_LEARN_LIMIT) {
            soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry,
                                LIMIT_COUNTEDf, 1);
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr_compose_ext_l2_entry
 * Description:
 *      Compose ext_l2_entry from its tcam portion, data portion, and hit bit
 *      Hardware does not support read and write to ext_l2_entry view.
 * Parameters:
 *      unit         Device number
 *      tcam_entry   TCAM portion of ESM L2 entry (ext_l2_entry_tcam_entry_t)
 *      data_entry   DATA portion of ESM L2 entry (ext_l2_entry_data_entry_t)
 *      src_hit      SRC_HIT field value
 *      dst_hit      DST_HIT field value
 *      ext_l2_entry (OUT) Buffer to store the composed ext_l2_entry_entry_t
 *                   result
 * Return:
 *      BCM_E_XXX.
 */
int
_bcm_tr_compose_ext_l2_entry(int unit,
                             ext_l2_entry_tcam_entry_t *tcam_entry,
                             ext_l2_entry_data_entry_t *data_entry,
                             int src_hit,
                             int dst_hit,
                             ext_l2_entry_entry_t *ext_l2_entry)
{
    sal_mac_addr_t      mac;
    uint32              fval;
    uint32              fbuf[2];

    if (tcam_entry == NULL || data_entry == NULL || ext_l2_entry == NULL) {
        return BCM_E_PARAM;
    }

    sal_memset(ext_l2_entry, 0, sizeof(ext_l2_entry_entry_t));

    /******************** Values from TCAM *******************************/
    fval = soc_mem_field32_get(unit, EXT_L2_ENTRY_TCAMm, tcam_entry, VLAN_IDf);
    soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, VLAN_IDf, fval);

    soc_mem_mac_addr_get(unit, EXT_L2_ENTRY_TCAMm, tcam_entry, MAC_ADDRf, mac);
    soc_mem_mac_addr_set(unit, EXT_L2_ENTRYm, ext_l2_entry, MAC_ADDRf, mac);

    fval = soc_mem_field32_get(unit, EXT_L2_ENTRY_TCAMm, tcam_entry,
                               KEY_TYPE_VFIf);
    soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, KEY_TYPE_VFIf,
                        fval);

    /******************** Values from DATA *******************************/
    soc_mem_field_get(unit, EXT_L2_ENTRY_DATAm, (uint32 *)data_entry,
                      AD_EXT_L2f, fbuf);
    soc_mem_field_set(unit, EXT_L2_ENTRYm, (uint32 *)ext_l2_entry, AD_EXT_L2f,
                      fbuf);

    fval = soc_mem_field32_get(unit, EXT_L2_ENTRY_DATAm, data_entry, VALIDf);
    soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, VALIDf, fval);

    /******************** Hit Bits *******************************/
    soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, DST_HITf, dst_hit);
    soc_mem_field32_set(unit, EXT_L2_ENTRYm, ext_l2_entry, SRC_HITf, src_hit);

    return BCM_E_NONE;
}

/*
 * function:
 *     _bcm_tr_l2_traverse_mem
 * Description:
 *      Helper function to _bcm_esw_l2_traverse to itterate over given memory
 *      and actually read the table and parse entries for Triumph external
 *      memory
 * Parameters:
 *     unit         device number
 *      mem         External L2 memory to read
 *     trav_st      Traverse structure with all the data.
 * Return:
 *     BCM_E_XXX
 */
#ifdef BCM_TRIUMPH_SUPPORT
int
_bcm_tr_l2_traverse_mem(int unit, soc_mem_t mem, _bcm_l2_traverse_t *trav_st)
{
    _soc_tr_l2e_ppa_info_t    *ppa_info;
    ext_l2_entry_entry_t      ext_l2_entry;
    ext_l2_entry_tcam_entry_t tcam_entry;
    ext_l2_entry_data_entry_t data_entry;
    ext_src_hit_bits_l2_entry_t src_hit_entry;
    ext_dst_hit_bits_l2_entry_t dst_hit_entry;
    int                       src_hit, dst_hit;
    int                       idx, idx_max;

    if (mem != EXT_L2_ENTRYm) {
        return BCM_E_UNAVAIL;
    }

    if (!soc_mem_index_count(unit, mem)) {
        return BCM_E_NONE;
    }

    ppa_info = SOC_CONTROL(unit)->ext_l2_ppa_info;
    if (ppa_info == NULL) {
        return BCM_E_NONE;
    }

    idx_max = soc_mem_index_max(unit, mem);
    for (idx = soc_mem_index_min(unit, mem); idx <= idx_max; idx++ ) {
        if (!(ppa_info[idx].data & _SOC_TR_L2E_VALID)) {
            continue;
        }
        SOC_IF_ERROR_RETURN
            (soc_mem_read(unit, EXT_L2_ENTRY_TCAMm, MEM_BLOCK_ANY, idx,
                          &tcam_entry));
        SOC_IF_ERROR_RETURN
            (soc_mem_read(unit, EXT_L2_ENTRY_DATAm, MEM_BLOCK_ANY, idx,
                          &data_entry));
        SOC_IF_ERROR_RETURN
            (soc_mem_read(unit, EXT_SRC_HIT_BITS_L2m, MEM_BLOCK_ANY, idx >> 5,
                          &src_hit_entry));
        SOC_IF_ERROR_RETURN
            (soc_mem_read(unit, EXT_DST_HIT_BITS_L2m, MEM_BLOCK_ANY, idx >> 5,
                          &dst_hit_entry));
        src_hit = (soc_mem_field32_get
                   (unit, EXT_SRC_HIT_BITS_L2m, &src_hit_entry, SRC_HITf) >>
                   (idx & 0x1f)) & 1;
        dst_hit = (soc_mem_field32_get
                   (unit, EXT_DST_HIT_BITS_L2m, &dst_hit_entry, DST_HITf) >>
                   (idx & 0x1f)) & 1;
        BCM_IF_ERROR_RETURN
            (_bcm_tr_compose_ext_l2_entry(unit, &tcam_entry, &data_entry,
                                          src_hit, dst_hit, &ext_l2_entry));
        trav_st->data = (uint32 *)&ext_l2_entry;
        trav_st->mem = mem;

        BCM_IF_ERROR_RETURN(trav_st->int_cb(unit, trav_st));
    }

    return BCM_E_NONE;
}
#endif /* BCM_TRIUMPH_SUPPORT */

/*
 * Function:
 *      _bcm_mac_block_insert
 * Purpose:
 *      Find or create a MAC_BLOCK table entry matching the given bitmap.
 * Parameters:
 *      unit - Unit number
 *      mb_pbmp - egress port bitmap for source MAC blocking
 *      mb_index - (OUT) Index of MAC_BLOCK table with bitmap.
 * Returns:
 *      BCM_E_NONE              Success
 *      BCM_E_INTERNAL          Chip access failure
 *      BCM_E_RESOURCE          No more MAC_BLOCK entries available
 *      BCM_E_PARAM             Bad bitmap supplied
 */
static int
_bcm_mac_block_insert(int unit, bcm_pbmp_t mb_pbmp, int *mb_index)
{
    int cur_index = 0;
    _bcm_mac_block_info_t *mbi = _mbi_entries[unit];
    mac_block_entry_t mbe;
    bcm_pbmp_t temp_pbmp;

    /* Check for reasonable pbmp */
    BCM_PBMP_ASSIGN(temp_pbmp, mb_pbmp);
    BCM_PBMP_AND(temp_pbmp, PBMP_ALL(unit));
    if (BCM_PBMP_NEQ(mb_pbmp, temp_pbmp)) {
        return BCM_E_PARAM;
    }

    for (cur_index = 0; cur_index < _mbi_num[unit]; cur_index++) {
        if (BCM_PBMP_EQ(mbi[cur_index].mb_pbmp, mb_pbmp)) {
            mbi[cur_index].ref_count++;
            *mb_index = cur_index;
            return BCM_E_NONE;
        }
    }

    /* Not in table already, see if any space free */
    for (cur_index = 1; cur_index < _mbi_num[unit]; cur_index++) {
        if (mbi[cur_index].ref_count == 0) {
            /* Attempt insert */
            sal_memset(&mbe, 0, sizeof(mac_block_entry_t));

            if (soc_mem_field_valid(unit, MAC_BLOCKm, MAC_BLOCK_MASK_LOf)) {
                soc_MAC_BLOCKm_field32_set(unit, &mbe, MAC_BLOCK_MASK_LOf,
                                           SOC_PBMP_WORD_GET(mb_pbmp, 0));
            } else if (soc_mem_field_valid(unit, MAC_BLOCKm,
                                           MAC_BLOCK_MASKf)) {
                soc_mem_pbmp_field_set(unit, MAC_BLOCKm, &mbe, MAC_BLOCK_MASKf,
                                       &mb_pbmp); 
            } else {
                return BCM_E_INTERNAL;
            }
            if (soc_mem_field_valid(unit, MAC_BLOCKm, MAC_BLOCK_MASK_HIf)) {
                soc_MAC_BLOCKm_field32_set(unit, &mbe, MAC_BLOCK_MASK_HIf,
                                           SOC_PBMP_WORD_GET(mb_pbmp, 1));
            }
            SOC_IF_ERROR_RETURN(WRITE_MAC_BLOCKm(unit, MEM_BLOCK_ALL,
                                                 cur_index, &mbe));
            mbi[cur_index].ref_count++;
            BCM_PBMP_ASSIGN(mbi[cur_index].mb_pbmp, mb_pbmp);
            *mb_index = cur_index;
            return BCM_E_NONE;
        }
    }

    /* Didn't find a free slot, out of table space */
    return BCM_E_RESOURCE;
}

/*
 * Function:
 *      _bcm_mac_block_delete
 * Purpose:
 *      Remove reference to MAC_BLOCK table entry matching the given bitmap.
 * Parameters:
 *      unit - Unit number
 *      mb_index - Index of MAC_BLOCK table with bitmap.
 */
static void
_bcm_mac_block_delete(int unit, int mb_index)
{
    if (_mbi_entries[unit][mb_index].ref_count > 0) {
        _mbi_entries[unit][mb_index].ref_count--;
    } else if (mb_index) {
        
        /* Someone reran init without flushing the L2 table */
    } /* else mb_index = 0, as expected for learning */
}

#ifdef BCM_TRIUMPH_SUPPORT
STATIC int
_bcm_tr_l2e_ppa_match(int unit, _bcm_l2_replace_t *rep_st)
{
    _soc_tr_l2e_ppa_info_t      *ppa_info;
    _soc_tr_l2e_ppa_vlan_t      *ppa_vlan;
    int                         i, imin, imax, rv, nmatches;
    soc_mem_t                   mem;
    uint32                      entdata, entmask, entvalue, newdata, newmask;
    ext_l2_entry_entry_t        l2entry, old_l2entry;
    int                         same_dest;

    ppa_info = SOC_CONTROL(unit)->ext_l2_ppa_info;
    ppa_vlan = SOC_CONTROL(unit)->ext_l2_ppa_vlan;
    if (ppa_info == NULL) {
        return BCM_E_NONE;
    }

    mem = EXT_L2_ENTRYm;
    imin = soc_mem_index_min(unit, mem);
    imax = soc_mem_index_max(unit, mem);

    /* convert match data */
    entdata = _SOC_TR_L2E_VALID;
    entmask = _SOC_TR_L2E_VALID;
    if (!(rep_st->flags & BCM_L2_REPLACE_MATCH_STATIC)) {
        entdata |= 0x00000000;
        entmask |= _SOC_TR_L2E_STATIC;
    }
    if (rep_st->flags & BCM_L2_REPLACE_MATCH_VLAN) {
        entdata |= (rep_st->key_vlan & _SOC_TR_L2E_VLAN_MASK) <<
            _SOC_TR_L2E_VLAN_SHIFT;
        entmask |= _SOC_TR_L2E_VLAN_MASK << _SOC_TR_L2E_VLAN_SHIFT;
        imin = ppa_vlan->vlan_min[rep_st->key_vlan];
        imax = ppa_vlan->vlan_max[rep_st->key_vlan];
    }
    if (rep_st->flags & BCM_L2_REPLACE_MATCH_DEST) {
        if (rep_st->match_dest.trunk != -1) {
            entdata |= _SOC_TR_L2E_T |
                ((rep_st->match_dest.trunk & _SOC_TR_L2E_TRUNK_MASK)
                 << _SOC_TR_L2E_TRUNK_SHIFT);
            entmask |= _SOC_TR_L2E_T |
                (_SOC_TR_L2E_TRUNK_MASK << _SOC_TR_L2E_TRUNK_SHIFT);
        } else {
            entdata |= 
                ((rep_st->match_dest.module & _SOC_TR_L2E_MOD_MASK) <<
                 _SOC_TR_L2E_MOD_SHIFT) |
                ((rep_st->match_dest.port & _SOC_TR_L2E_PORT_MASK) <<
                 _SOC_TR_L2E_PORT_SHIFT);
            entmask |= _SOC_TR_L2E_T |
                (_SOC_TR_L2E_MOD_MASK << _SOC_TR_L2E_MOD_SHIFT) |
                (_SOC_TR_L2E_PORT_MASK << _SOC_TR_L2E_PORT_SHIFT);
        }
    }

    nmatches = 0;

    if (imin >= 0) {
        soc_mem_lock(unit, mem);
        for (i = imin; i <= imax; i++) {
            entvalue = ppa_info[i].data;
            if ((entvalue & entmask) != entdata) {
                continue;
            }
            if (rep_st->flags & BCM_L2_REPLACE_MATCH_MAC) {
                if (ENET_CMP_MACADDR(rep_st->key_mac, ppa_info[i].mac)) {
                    continue;
                }
            }
            nmatches += 1;

            /* lookup the matched entry */
            sal_memset(&l2entry, 0, sizeof(l2entry));
            soc_mem_field32_set(unit, mem, &l2entry, VLAN_IDf,
                                (entvalue >> 16) & 0xfff);
            soc_mem_mac_addr_set(unit, mem, &l2entry, MAC_ADDRf,
                                 ppa_info[i].mac);

            /* operate on matched entry */
            if (rep_st->flags & BCM_L2_REPLACE_DELETE) {
                int             mb_index;

                rv = soc_mem_generic_delete(unit, mem, MEM_BLOCK_ANY, 0,
                                            &l2entry, &l2entry, NULL);
                if (rv < 0) {
                    soc_mem_unlock(unit, mem);
                    return rv;
                }
                if (!SOC_CONTROL(unit)->l2x_group_enable) {
                    mb_index = soc_mem_field32_get(unit, mem, &l2entry,
                                                   MAC_BLOCK_INDEXf);
                    _bcm_mac_block_delete(unit, mb_index);
                }
                if (entvalue & _SOC_TR_L2E_LIMIT_COUNTED) {
                    rv = soc_triumph_learn_count_update(unit, &l2entry,
                                                        TRUE, -1);
                }
            } else {
                /* replace destination fields */
                rv = soc_mem_generic_lookup(unit, mem, MEM_BLOCK_ANY, 0,
                                            &l2entry, &l2entry, NULL);
                if (rep_st->flags & BCM_L2_REPLACE_NEW_TRUNK) {
                    newdata = _SOC_TR_L2E_T |
                        ((rep_st->new_dest.trunk & _SOC_TR_L2E_TRUNK_MASK) <<
                         _SOC_TR_L2E_TRUNK_SHIFT);
                    newmask = _SOC_TR_L2E_T |
                        (_SOC_TR_L2E_TRUNK_MASK << _SOC_TR_L2E_TRUNK_SHIFT);
                    soc_mem_field32_set(unit, mem, &l2entry, Tf, 1);
                    soc_mem_field32_set(unit, mem, &l2entry, TGIDf,
                                        rep_st->new_dest.trunk);
                } else {
                    newdata =
                        (rep_st->new_dest.module << _SOC_TR_L2E_MOD_SHIFT) |
                        (rep_st->new_dest.port << _SOC_TR_L2E_PORT_SHIFT);
                    newmask = _SOC_TR_L2E_T |
                        (_SOC_TR_L2E_MOD_MASK << _SOC_TR_L2E_MOD_SHIFT) |
                        (_SOC_TR_L2E_PORT_MASK << _SOC_TR_L2E_PORT_SHIFT);
                    soc_mem_field32_set(unit, mem, &l2entry, MODULE_IDf,
                                        rep_st->new_dest.module);
                    soc_mem_field32_set(unit, mem, &l2entry, PORT_NUMf,
                                        rep_st->new_dest.port);
                }
                same_dest = !((entvalue ^ newdata) & newmask);

                if ((entvalue & _SOC_TR_L2E_LIMIT_COUNTED) && !same_dest) {
                    rv = soc_triumph_learn_count_update(unit, &l2entry, FALSE,
                                                        1);
                    if (SOC_FAILURE(rv)) {
                        soc_mem_unlock(unit, mem);
                        return rv;
                    }
                }

                /* re-insert entry */
                rv = soc_mem_generic_insert(unit, mem, MEM_BLOCK_ANY, 0,
                                            &l2entry, &old_l2entry, NULL);
                if (rv == BCM_E_EXISTS) {
                    rv = BCM_E_NONE;
                }
                if (rv < 0) {
                    soc_mem_unlock(unit, mem);
                    return rv;
                }
                if ((entvalue & _SOC_TR_L2E_LIMIT_COUNTED) && !same_dest) {
                    rv = soc_triumph_learn_count_update(unit, &old_l2entry,
                                                        FALSE, -1);
                    if (SOC_FAILURE(rv)) {
                        soc_mem_unlock(unit, mem);
                        return rv;
                    }
                }
            }
        }
        soc_mem_unlock(unit, EXT_L2_ENTRYm);
    }
    soc_cm_debug(DK_VERBOSE,
                 "tr_l2e_ppa_match: imin=%d imax=%d nmatches=%d flags=0x%x\n",
                 imin, imax, nmatches, rep_st->flags);
    return BCM_E_NONE;
}
#endif /* BCM_TRIUMPH_SUPPORT */

/*
 * Function:
 *      _bcm_l2_hash_dynamic_replace
 * Purpose:
 *      Replace dynamic L2 entries in a dual hash.
 * Parameters:
 *      unit - Unit number
 *      l2x_entry - Entry to insert instead of dynamic entry.
 * Returns:
 *      BCM_E_NONE              Success
 *      BCM_E_XXX          		Error 
 */

STATIC int 
_bcm_l2_hash_dynamic_replace(int unit, l2x_entry_t *l2x_entry)
{
    l2x_entry_t     l2ent;
    int             rv;
    uint32          fval;
    bcm_mac_t       mac;
    int             cf_hit, cf_unhit;
    int             bank, bucket, slot, index;
    int             num_banks;
    int             entries_per_bank, entries_per_row, entries_per_bucket;
    int             bank_base, bucket_offset;

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        BCM_IF_ERROR_RETURN
            (soc_trident2_hash_bank_count_get(unit, L2Xm, &num_banks));
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        num_banks = 2;
    }

    BCM_IF_ERROR_RETURN(soc_l2x_freeze(unit));

    cf_hit = cf_unhit = -1;
    for (bank = 0; bank < num_banks; bank++) {
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TD2_TT2(unit)) {
            rv = soc_trident2_hash_bank_info_get(unit, L2Xm, bank,
                                                 &entries_per_bank,
                                                 &entries_per_row,
                                                 &entries_per_bucket,
                                                 &bank_base, &bucket_offset);
            if (SOC_FAILURE(rv)) {
                (void)soc_l2x_thaw(unit);
                return rv;
            }
            bucket = soc_td2_l2x_bank_entry_hash(unit, bank,
                                                 (uint32 *)l2x_entry);
        } else
#endif
        {
            entries_per_bank = soc_mem_index_count(unit, L2Xm) / 2;
            entries_per_row = SOC_L2X_BUCKET_SIZE;
            entries_per_bucket = entries_per_row / 2;
            bank_base = 0;
            bucket_offset = bank * entries_per_bucket;
            bucket = soc_tr_l2x_bank_entry_hash(unit, bank,
                                                (uint32 *)l2x_entry);
        }

        for (slot = 0; slot < entries_per_bucket; slot++) {
            index = bank_base + bucket * entries_per_row + bucket_offset +
                slot;
            rv = soc_mem_read(unit, L2Xm, MEM_BLOCK_ANY, index, &l2ent);
            if (SOC_FAILURE(rv)) {
                (void)soc_l2x_thaw(unit);
                return rv;
            }

            if (!soc_L2Xm_field32_get(unit, &l2ent, VALIDf)) {
                /* Found invalid entry - stop the search victim found */
                cf_unhit = index; 
                break;
            }

            fval = soc_mem_field32_get(unit, L2Xm, &l2ent, KEY_TYPEf);
#ifdef BCM_TRIDENT2_SUPPORT
            if (SOC_IS_TD2_TT2(unit)) {
                if (fval != TD2_L2_HASH_KEY_TYPE_BRIDGE &&
                    fval != TD2_L2_HASH_KEY_TYPE_VFI) {
                    continue;
                }
            } else
#endif /* BCM_TRIDENT2_SUPPORT */
            if (fval != TR_L2_HASH_KEY_TYPE_BRIDGE &&
                fval != TR_L2_HASH_KEY_TYPE_VFI) {
                continue;
            }

            soc_L2Xm_mac_addr_get(unit, &l2ent, MAC_ADDRf, mac);
            /* Skip static entries */
            if ((soc_L2Xm_field32_get(unit, &l2ent, STATIC_BITf)) ||
                (BCM_MAC_IS_MCAST(mac)) ||
                (soc_mem_field_valid(unit, L2Xm, L3f) && 
                 soc_L2Xm_field32_get(unit, &l2ent, L3f))) {
                continue;
            }

            if (soc_L2Xm_field32_get(unit, &l2ent, HITDAf) || 
                soc_L2Xm_field32_get(unit, &l2ent, HITSAf) ) {
                cf_hit =  index;
            } else {
                /* Found unhit entry - stop search victim found */
                cf_unhit = index;
                break;
            }
        }
        if (cf_unhit != -1) {
            break;
        }
    }

    COMPILER_REFERENCE(entries_per_bank);

    if (cf_unhit >= 0) {
        index = cf_unhit;   /* take last unhit dynamic */
    } else if (cf_hit >= 0) {
        index = cf_hit;     /* or last hit dynamic */
    } else {
        rv = BCM_E_FULL;     /* no dynamics to delete */
        (void) soc_l2x_thaw(unit);
         return rv;
    }

    rv = soc_mem_delete_index(unit, L2Xm, MEM_BLOCK_ALL, index);
    if (SOC_SUCCESS(rv)) {
        rv = soc_mem_generic_insert(unit, L2Xm, MEM_BLOCK_ANY, 0,
                                    l2x_entry, NULL, NULL);
    }
    if (SOC_FAILURE(rv)) {
        (void) soc_l2x_thaw(unit);
        return rv;
    }

    return soc_l2x_thaw(unit);
}

#if defined(BCM_TRIDENT2_SUPPORT)
/*
 * Function:
 *      _bcm_td2_l2_hit_retrieve
 * Purpose:
 *      Retrieve the hit bit from both x and y pipe, then "or" the hitbit values and assign it to 
 *      L2_ENTRY data structure.
 * Parameters:
 *      unit        Unit number
 *      l2addr      (OUT) L2 API data structure
 *      l2x_entry   L2X entry
 *      l2_hw_index   L2X entry hardware index
 */
int
_bcm_td2_l2_hit_retrieve(int unit, l2x_entry_t *l2x_entry,
                                 int l2_hw_index)
{ 
    uint32 hit_sa, hit_da, hit_local_sa;          /* composite hit = hit_x|hit_y */    
    int idx_offset, hit_idx_shift;
    l2_hitsa_only_x_entry_t hit_sa_x;
    l2_hitsa_only_y_entry_t hit_sa_y;
    l2_hitda_only_x_entry_t hit_da_x;
    l2_hitda_only_y_entry_t hit_da_y;  
    soc_field_t hit_saf[] = { HITSA_0f, HITSA_1f, HITSA_2f, HITSA_3f};
    soc_field_t hit_daf[] = { HITDA_0f, HITDA_1f, HITDA_2f, HITDA_3f};   
    soc_field_t hit_local_saf[] = { LOCAL_SA_0f, LOCAL_SA_1f, 
                                    LOCAL_SA_2f, LOCAL_SA_3f};     

    idx_offset = (l2_hw_index & 0x3);
    hit_idx_shift = 2;

    /* Retrieve DA HIT */
    BCM_IF_ERROR_RETURN(
        BCM_XGS3_MEM_READ(unit, L2_HITDA_ONLY_Xm,
          (l2_hw_index >> hit_idx_shift), &hit_da_x));
    
    BCM_IF_ERROR_RETURN(
        BCM_XGS3_MEM_READ(unit, L2_HITDA_ONLY_Ym,
          (l2_hw_index >> hit_idx_shift), &hit_da_y)); 

    hit_da = 0;
    hit_da |= soc_mem_field32_get(unit, L2_HITDA_ONLY_Xm,
                                   &hit_da_x, hit_daf[idx_offset]);
    
    hit_da |= soc_mem_field32_get(unit, L2_HITDA_ONLY_Ym,
                                   &hit_da_y, hit_daf[idx_offset]);
    
    soc_L2Xm_field32_set(unit, l2x_entry, HITDAf, hit_da);  

    /* Retrieve SA HIT */
    BCM_IF_ERROR_RETURN(
        BCM_XGS3_MEM_READ(unit, L2_HITSA_ONLY_Xm,
          (l2_hw_index >> hit_idx_shift), &hit_sa_x));
    
    BCM_IF_ERROR_RETURN(
        BCM_XGS3_MEM_READ(unit, L2_HITSA_ONLY_Ym,
          (l2_hw_index >> hit_idx_shift), &hit_sa_y)); 

    hit_sa = 0;
    hit_sa |= soc_mem_field32_get(unit, L2_HITSA_ONLY_Xm,
                                   &hit_sa_x, hit_saf[idx_offset]);
    
    hit_sa |= soc_mem_field32_get(unit, L2_HITSA_ONLY_Ym,
                                   &hit_sa_y, hit_saf[idx_offset]);
    
    soc_L2Xm_field32_set(unit, l2x_entry, HITSAf, hit_sa);        
    
    /* Retrieve LOCAL_SA HIT */
    hit_local_sa = 0;
    hit_local_sa |= soc_mem_field32_get(unit, L2_HITSA_ONLY_Xm,
                                   &hit_sa_x, hit_local_saf[idx_offset]);
    
    hit_local_sa |= soc_mem_field32_get(unit, L2_HITSA_ONLY_Ym,
                                   &hit_sa_y, hit_local_saf[idx_offset]);
    
    soc_L2Xm_field32_set(unit, l2x_entry, LOCAL_SAf, hit_local_sa);        
    
    return BCM_E_NONE;    
}
#endif /* BCM_TRIDENT2_SUPPORT */

/*
 * Function:
 *      bcm_tr_l2_addr_add
 * Description:
 *      Add a MAC address to the Switch Address Resolution Logic (ARL)
 *      port with the given VLAN ID and parameters.
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      l2addr - Pointer to bcm_l2_addr_t containing all valid fields
 * Returns:
 *      BCM_E_NONE              Success
 *      BCM_E_INTERNAL          Chip access failure
 *      BCM_E_RESOURCE          No MAC_BLOCK entries available
 * Notes:
 *      Use CMIC_PORT(unit) to associate the entry with the CPU.
 *      Use flag of BCM_L2_COPY_TO_CPU to send a copy to the CPU.
 *      Use flag of BCM_L2_TRUNK_MEMBER to set trunking (TGID must be
 *      passed as well with non-zero trunk group ID)
 *      In case the L2X table is full (e.g. bucket full), an attempt
 *      will be made to store the entry in the L2_USER_ENTRY table.
 */
int
bcm_tr_l2_addr_add(int unit, bcm_l2_addr_t *l2addr)
{
    l2x_entry_t  l2x_entry, l2x_lookup;
#ifdef BCM_TRIUMPH_SUPPORT
    _soc_tr_l2e_ppa_info_t *ppa_info;
    ext_l2_entry_entry_t ext_l2_entry, ext_l2_lookup;
    int          exist_in_ext_l2, same_dest, update_limit, limit_counted;
    int          rv1, ext_l2_index;
#endif /* BCM_TRIUMPH_SUPPORT */
    int          enable_ppa_war;
    int          rv, l2_index, mb_index = 0;

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TRIDENT(unit) || SOC_IS_KATANAX(unit) ||
         SOC_IS_TD2_TT2(unit)) {       
        if (soc_mem_is_valid(unit, MY_STATION_TCAMm) &&
            (l2addr->flags & BCM_L2_L3LOOKUP)) {
            BCM_IF_ERROR_RETURN(bcm_td_l2_myStation_add(unit, l2addr));
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */

    BCM_IF_ERROR_RETURN(_bcm_tr_l2_to_l2x(unit, &l2x_entry, l2addr, FALSE));

#ifdef BCM_TRIUMPH_SUPPORT
    exist_in_ext_l2 = FALSE;
    ppa_info = SOC_CONTROL(unit)->ext_l2_ppa_info;
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm) &&
        soc_mem_index_count(unit, EXT_L2_ENTRYm) > 0) {
        same_dest = FALSE;
        update_limit =
            (l2addr->flags & (BCM_L2_STATIC | BCM_L2_LEARN_LIMIT_EXEMPT) && 
             !(l2addr->flags & BCM_L2_LEARN_LIMIT)) ? FALSE : TRUE;
        limit_counted = FALSE;
        BCM_IF_ERROR_RETURN
            (_bcm_tr_l2_to_ext_l2(unit, &ext_l2_entry, l2addr, FALSE));
        soc_mem_lock(unit, EXT_L2_ENTRYm);
        rv1 = soc_mem_generic_lookup(unit, EXT_L2_ENTRYm, MEM_BLOCK_ANY, 0,
                                     &ext_l2_entry, &ext_l2_lookup,
                                     &ext_l2_index);
        if (SOC_SUCCESS(rv1)) {
            exist_in_ext_l2 = TRUE;
        } else if (rv1 != SOC_E_NOT_FOUND) {
            soc_mem_unlock(unit, EXT_L2_ENTRYm);
            return rv1;
        }

        if (!(l2addr->flags & (BCM_L2_L3LOOKUP | BCM_L2_MCAST |
                               BCM_L2_PENDING | BCM_L2_STATIC))) {
            if (exist_in_ext_l2) {
                if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, &ext_l2_entry,
                                        DEST_TYPEf) ==
                    soc_mem_field32_get(unit, EXT_L2_ENTRYm, &ext_l2_lookup,
                                        DEST_TYPEf) &&
                    soc_mem_field32_get(unit, EXT_L2_ENTRYm, &ext_l2_entry,
                                        DESTINATIONf) ==
                    soc_mem_field32_get(unit, EXT_L2_ENTRYm, &ext_l2_lookup,
                                        DESTINATIONf)) {
                    same_dest = TRUE;
                }
                limit_counted =
                    ppa_info[ext_l2_index].data & _SOC_TR_L2E_LIMIT_COUNTED;
            }
            if (update_limit) {
                rv1 = SOC_E_NONE;
                if (!limit_counted) {
                    rv1 = soc_triumph_learn_count_update(unit, &ext_l2_entry,
                                                         TRUE, 1);
                } else if (!same_dest) {
                    rv1 = soc_triumph_learn_count_update(unit, &ext_l2_entry,
                                                         FALSE, 1);
                }
                if (SOC_FAILURE(rv1)) {
                    soc_mem_unlock(unit, EXT_L2_ENTRYm);
                    return rv1;
                }
            }
            if (!SOC_CONTROL(unit)->l2x_group_enable) {
                /* Mac blocking, attempt to associate with bitmap entry */
                rv1 = _bcm_mac_block_insert(unit, l2addr->block_bitmap,
                                           &mb_index);
                if (SOC_FAILURE(rv1)) {
                    soc_mem_unlock(unit, EXT_L2_ENTRYm);
                    return rv1;
                }
                soc_mem_field32_set(unit, EXT_L2_ENTRYm, &ext_l2_entry,
                                    MAC_BLOCK_INDEXf, mb_index);
            }
            rv = soc_mem_generic_insert(unit, EXT_L2_ENTRYm, MEM_BLOCK_ANY, 0,
                                        &ext_l2_entry, &ext_l2_lookup, NULL);
            if (rv == SOC_E_EXISTS) {
                /* entry exists, clear setting for old entry */
                if (!SOC_CONTROL(unit)->l2x_group_enable) {
                    mb_index = soc_mem_field32_get(unit, EXT_L2_ENTRYm,
                                                   &ext_l2_lookup,
                                                   MAC_BLOCK_INDEXf);
                    _bcm_mac_block_delete(unit, mb_index);
                }
                if (limit_counted) {
                    if (!update_limit) {
                        (void)soc_triumph_learn_count_update(unit,
                                                             &ext_l2_lookup,
                                                             TRUE, -1);
                    } else if (!same_dest) {
                        (void)soc_triumph_learn_count_update(unit,
                                                             &ext_l2_lookup,
                                                             FALSE, -1);
                    }
                }
                rv = BCM_E_NONE;
            } else if (SOC_FAILURE(rv)) {
                /* insert fail, undo setting for new entry */
                if (update_limit) {
                    if (!limit_counted) {
                        (void)soc_triumph_learn_count_update(unit,
                                                             &ext_l2_entry,
                                                             TRUE, -1);
                    } else if (!same_dest) {
                        (void)soc_triumph_learn_count_update(unit,
                                                             &ext_l2_entry,
                                                             FALSE, -1);
                    }
                }
            }
            soc_mem_unlock(unit, EXT_L2_ENTRYm);
            if (SOC_SUCCESS(rv)) {
                /* insert to ext_l2_entry OK, delete from l2x if present */
                soc_mem_lock(unit, L2Xm);
                if (SOC_L2_DEL_SYNC_LOCK(SOC_CONTROL(unit)) >= 0) {
                    rv1 = soc_mem_generic_delete(unit, L2Xm, MEM_BLOCK_ANY, 0,
                                                 &l2x_entry, &l2x_lookup,
                                                 &l2_index);
                    if (SOC_SUCCESS(rv1)) {
                        if (!SOC_CONTROL(unit)->l2x_group_enable) {
                            mb_index =
                                soc_mem_field32_get(unit, L2Xm, &l2x_lookup,
                                                    MAC_BLOCK_INDEXf);
                            _bcm_mac_block_delete(unit, mb_index);
                        }
                        rv1 = soc_l2x_sync_delete(unit, (uint32 *)&l2x_lookup,
                                                  l2_index, 0);
                    } else if (rv1 == SOC_E_NOT_FOUND) {
                        rv1 = BCM_E_NONE;
                    }
                    SOC_L2_DEL_SYNC_UNLOCK(SOC_CONTROL(unit));
                } else {
                    rv1 = BCM_E_INTERNAL;
                }
                soc_mem_unlock(unit, L2Xm);
                return rv1;
            }
            if (rv != SOC_E_FULL) {
                goto done;
            }
        } else {
            soc_mem_unlock(unit, EXT_L2_ENTRYm);
        }
    }
#endif

    rv = soc_mem_generic_lookup(unit, L2Xm, MEM_BLOCK_ANY, 0, &l2x_entry,
                                &l2x_lookup, &l2_index);
    if (BCM_FAILURE(rv) && rv != BCM_E_NOT_FOUND) {
        return rv;
    }

    if (!SOC_CONTROL(unit)->l2x_group_enable) {
        /* Mac blocking, attempt to associate with bitmap entry */
        BCM_IF_ERROR_RETURN
            (_bcm_mac_block_insert(unit, l2addr->block_bitmap, &mb_index));
        soc_mem_field32_set(unit, L2Xm, &l2x_entry, MAC_BLOCK_INDEXf,
                            mb_index);
    }

    enable_ppa_war = FALSE;
    if (SOC_CONTROL(unit)->l2x_ppa_bypass == FALSE &&
        soc_feature(unit, soc_feature_ppa_bypass) &&
        soc_mem_field32_get(unit, L2Xm, &l2x_entry, KEY_TYPEf) !=
        TR_L2_HASH_KEY_TYPE_BRIDGE) {
        enable_ppa_war = TRUE;
    }

    rv = soc_mem_insert_return_old(unit, L2Xm, MEM_BLOCK_ANY, 
                                   (void *)&l2x_entry, (void *)&l2x_entry);
    if ((rv == BCM_E_FULL) && (l2addr->flags & BCM_L2_REPLACE_DYNAMIC)) {
        rv = _bcm_l2_hash_dynamic_replace( unit, &l2x_entry);
        if (rv < 0 ) {
            goto done;
        }
    } else if (rv == BCM_E_EXISTS) {
        if (!SOC_CONTROL(unit)->l2x_group_enable) {
            mb_index = soc_mem_field32_get(unit, L2Xm, &l2x_lookup,
                                           MAC_BLOCK_INDEXf);
            _bcm_mac_block_delete(unit, mb_index);
        }
        rv = BCM_E_NONE;
    }

    if (BCM_SUCCESS(rv) && enable_ppa_war) {
        SOC_CONTROL(unit)->l2x_ppa_bypass = TRUE;
    }

#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_SUCCESS(rv)) {
        if (exist_in_ext_l2) {
            soc_mem_lock(unit, EXT_L2_ENTRYm);
            limit_counted =
                ppa_info[ext_l2_index].data & _SOC_TR_L2E_LIMIT_COUNTED;
            rv1 = soc_mem_generic_delete(unit, EXT_L2_ENTRYm, MEM_BLOCK_ANY,
                                         0, &ext_l2_entry, &ext_l2_lookup,
                                         &ext_l2_index);
            if (SOC_SUCCESS(rv1)) {
                if (!SOC_CONTROL(unit)->l2x_group_enable) {
                    mb_index =
                        soc_mem_field32_get(unit, EXT_L2_ENTRYm,
                                            &ext_l2_lookup, MAC_BLOCK_INDEXf);
                    _bcm_mac_block_delete(unit, mb_index);
                }
                if (limit_counted) {
                    (void)soc_triumph_learn_count_update(unit, &ext_l2_lookup,
                                                         TRUE, -1);
                }
            }
            soc_mem_unlock(unit, EXT_L2_ENTRYm);
        }
    }
#endif

done:
    if (rv < 0) {
        _bcm_mac_block_delete(unit, mb_index);
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr_l2_addr_delete
 * Description:
 *      Delete an L2 address (MAC+VLAN) from the device
 * Parameters:
 *      unit - device unit
 *      mac  - MAC address to delete
 *      vid  - VLAN id
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr_l2_addr_delete(int unit, bcm_mac_t mac, bcm_vlan_t vid)
{
    bcm_l2_addr_t  l2addr;
    l2x_entry_t    l2x_entry, l2x_lookup;
#ifdef BCM_TRIUMPH_SUPPORT
    _soc_tr_l2e_ppa_info_t *ppa_info;
    ext_l2_entry_entry_t ext_l2_entry, ext_l2_lookup;
    int limit_counted;
#endif /* BCM_TRIUMPH_SUPPORT */
    int            l2_index, mb_index;
    int            rv;
    soc_control_t  *soc = SOC_CONTROL(unit);

    bcm_l2_addr_t_init(&l2addr, mac, vid);

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TRIDENT(unit) || (SOC_IS_KATANAX(unit)) ||
        SOC_IS_TD2_TT2(unit)) {
        if (soc_mem_is_valid(unit, MY_STATION_TCAMm) && BCM_VLAN_VALID(vid)) {
              rv = bcm_td_l2_myStation_delete (unit, mac, vid, &l2_index);
            if ((rv != BCM_E_NOT_FOUND) && (rv != BCM_E_FULL)) {
                   if (rv != BCM_E_NONE) {
                        return rv;
                   }
              }
         }
    }
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm) &&
        soc_mem_index_count(unit, EXT_L2_ENTRYm)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr_l2_to_ext_l2(unit, &ext_l2_entry, &l2addr, TRUE));
        soc_mem_lock(unit, EXT_L2_ENTRYm);
        rv = soc_mem_generic_lookup(unit, EXT_L2_ENTRYm, MEM_BLOCK_ANY, 0,
                                    &ext_l2_entry, NULL, &l2_index);
        if (BCM_SUCCESS(rv)) {
            ppa_info = SOC_CONTROL(unit)->ext_l2_ppa_info;
            limit_counted =
                ppa_info[l2_index].data & _SOC_TR_L2E_LIMIT_COUNTED;
            rv = soc_mem_generic_delete(unit, EXT_L2_ENTRYm, MEM_BLOCK_ANY, 0,
                                        &ext_l2_entry, &ext_l2_lookup, NULL);
            if (BCM_SUCCESS(rv)) {
                if (!SOC_CONTROL(unit)->l2x_group_enable) {
                    mb_index =
                        soc_mem_field32_get(unit, EXT_L2_ENTRYm,
                                            &ext_l2_lookup, MAC_BLOCK_INDEXf);
                    _bcm_mac_block_delete(unit, mb_index);
                }
                if (limit_counted) {
                    rv = soc_triumph_learn_count_update(unit, &ext_l2_lookup,
                                                        TRUE, -1);
                }
            }
        }
        if (rv != BCM_E_NOT_FOUND) {
            soc_mem_unlock(unit, EXT_L2_ENTRYm);
            return rv;
        }
        soc_mem_unlock(unit, EXT_L2_ENTRYm);
    }
#endif

    BCM_IF_ERROR_RETURN(_bcm_tr_l2_to_l2x(unit, &l2x_entry, &l2addr, TRUE));

    soc_mem_lock(unit, L2Xm);

    rv = soc_mem_search(unit, L2Xm, MEM_BLOCK_ANY, &l2_index,
                       (void *)&l2x_entry, (void *)&l2x_lookup, 0);
    if (BCM_E_NONE != rv) {
        soc_mem_unlock(unit, L2Xm);
        return rv;
    }

    if (!SOC_CONTROL(unit)->l2x_group_enable) {
        mb_index = soc_L2Xm_field32_get(unit, &l2x_lookup, MAC_BLOCK_INDEXf);
        _bcm_mac_block_delete(unit, mb_index);
    }

    if (SOC_L2_DEL_SYNC_LOCK(soc) < 0) {
        soc_mem_unlock(unit, L2Xm);
        return BCM_E_RESOURCE;
    }
    rv = soc_mem_delete_return_old(unit, L2Xm, MEM_BLOCK_ANY,
                                   (void *)&l2x_entry, (void *)&l2x_entry);
    if (rv >= 0) {
        rv = soc_l2x_sync_delete(unit, (uint32 *) &l2x_lookup, l2_index, 0);
    }
    SOC_L2_DEL_SYNC_UNLOCK(soc);
    soc_mem_unlock(unit, L2Xm);

    return rv;
}

#ifdef BCM_TRIUMPH_SUPPORT
STATIC int
bcm_tr_l2_addr_ext_get(int unit, sal_mac_addr_t mac, bcm_vlan_t vid,
                   bcm_l2_addr_t *l2addr)
{
    bcm_l2_addr_t           l2addr_key;
    ext_l2_entry_entry_t    ext_l2_entry, ext_l2_lookup;
    int                     rv;

    bcm_l2_addr_t_init(&l2addr_key, mac, vid);

    BCM_IF_ERROR_RETURN(
        _bcm_tr_l2_to_ext_l2(unit, &ext_l2_entry, &l2addr_key, TRUE));

    soc_mem_lock(unit, EXT_L2_ENTRYm);
    rv = soc_mem_generic_lookup(unit, EXT_L2_ENTRYm, MEM_BLOCK_ANY, 0,
                               &ext_l2_entry, &ext_l2_lookup, NULL);
    soc_mem_unlock(unit, EXT_L2_ENTRYm);

    if (BCM_SUCCESS(rv)) {
        rv = _bcm_tr_l2_from_ext_l2(unit, l2addr, &ext_l2_lookup);
    }

    return rv;
}

#endif /* BCM_TRIUMPH_SUPPORT */

/*
 * Function:
 *      bcm_tr_l2_addr_get
 * Description:
 *      Given a MAC address and VLAN ID, check if the entry is present
 *      in the L2 table, and if so, return all associated information.
 * Parameters:
 *      unit - Device unit number
 *      mac - input MAC address to search
 *      vid - input VLAN ID to search
 *      l2addr - Pointer to bcm_l2_addr_t structure to receive results
 * Returns:
 *      BCM_E_NONE              Success (l2addr filled in)
 *      BCM_E_PARAM             Illegal parameter (NULL pointer)
 *      BCM_E_INTERNAL          Chip access failure
 *      BCM_E_NOT_FOUND Address not found (l2addr not filled in)
 */

int
bcm_tr_l2_addr_get(int unit, sal_mac_addr_t mac, bcm_vlan_t vid,
                   bcm_l2_addr_t *l2addr)
{
    bcm_l2_addr_t l2addr_key;
    l2x_entry_t  l2x_entry, l2x_lookup;
    int          l2_hw_index;
    int          rv;

    bcm_l2_addr_t_init(&l2addr_key, mac, vid);

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TRIDENT(unit) || SOC_IS_KATANAX(unit) ||       
        SOC_IS_TD2_TT2(unit)) {
        if (soc_mem_is_valid(unit, MY_STATION_TCAMm)) {
              rv = bcm_td_l2_myStation_get (unit, mac, vid, l2addr);
              if (BCM_SUCCESS(rv)) {
                   return rv;
              }
         }
    }
#endif /* BCM_TRIDENT_SUPPORT */

    BCM_IF_ERROR_RETURN(
        _bcm_tr_l2_to_l2x(unit, &l2x_entry, &l2addr_key, TRUE));

    soc_mem_lock(unit, L2Xm);

    rv = soc_mem_generic_lookup(unit, L2Xm, MEM_BLOCK_ANY, 0, &l2x_entry,
                                &l2x_lookup, &l2_hw_index);

    /* If not found in Internal memory and external is available serach there */
#ifdef BCM_TRIUMPH_SUPPORT
    if (rv == BCM_E_NOT_FOUND && soc_mem_is_valid(unit, EXT_L2_ENTRYm) &&
        soc_mem_index_count(unit, EXT_L2_ENTRYm) > 0) {
        rv = bcm_tr_l2_addr_ext_get(unit, mac, vid, l2addr);
            soc_mem_unlock(unit, L2Xm);
            return rv;
        }
#endif /* BCM_TRIUMPH_SUPPORT */

    soc_mem_unlock(unit, L2Xm);
    if (SOC_SUCCESS(rv)) {
        /* Retrieve the hit bit for TD2/TT2 */
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TD2_TT2(unit)) {
            BCM_IF_ERROR_RETURN(
                _bcm_td2_l2_hit_retrieve(unit, &l2x_lookup, l2_hw_index));
        }
#endif /* BCM_TRIDENT2_SUPPORT */
        rv = _bcm_tr_l2_from_l2x(unit, l2addr, &l2x_lookup);
    }

    return rv;
}


#ifdef BCM_WARM_BOOT_SUPPORT
#ifdef BCM_KATANA2_SUPPORT
STATIC int
_bcm_kt2_l2_station_non_tcam_reload(int unit, _bcm_l2_station_control_t *sc) 
{
    int index = 0;
    int rv  = BCM_E_NONE; 
    port_tab_entry_t          *ptab_buf;        
    port_tab_entry_t          *ptab;        
    _bcm_l2_station_entry_t   *s_ent_p;          /* Station table entry.      */
    uint32              mac_field[2];
    egr_olp_dgpp_config_entry_t *eolp_dgpp_cfg_buf;
    egr_olp_dgpp_config_entry_t *eolp_dgpp_cfg = NULL;
    int                       entry_mem_size;    /* Size of table entry. */
    egr_olp_config_entry_t    eolp_cfg;
    bcm_mac_t                 mac_addr; 

    entry_mem_size = sizeof(port_tab_entry_t);
    /* Allocate buffer to store the DMAed table entries. */
    ptab_buf = soc_cm_salloc(unit, entry_mem_size * sc->port_entries_total,
                              "Port table entry buffer");
    if (NULL == ptab_buf) {
        return (BCM_E_MEMORY);
    }
    /* Initialize the entry buffer. */
    sal_memset(ptab_buf, 0, sizeof(entry_mem_size) * sc->port_entries_total);

    /* Read the table entries into the buffer. */
    rv = soc_mem_read_range(unit, PORT_TABm, MEM_BLOCK_ALL,
                            0, (sc->port_entries_total - 1), ptab_buf);
    if (BCM_FAILURE(rv)) {
        if (ptab_buf) {
            soc_cm_sfree(unit, ptab_buf);
        }
        return rv;
    }

    /* Iterate over the port table entries. */
    for (index = 0; index < sc->port_entries_total; index++) {
        ptab = soc_mem_table_idx_to_pointer
                    (unit, PORT_TABm, port_tab_entry_t *,
                     ptab_buf, index);

        soc_mem_field_get(unit, PORT_TABm, (uint32 *)ptab, 
                          MAC_ADDRESSf, mac_field);

        if ((mac_field[0] > 0) || (mac_field[1] > 0)) {
            s_ent_p = (_bcm_l2_station_entry_t *)
                sal_alloc(sizeof(_bcm_l2_station_entry_t),
                                    "Sw L2 station entry");
            if (NULL == s_ent_p) {
                if (ptab_buf) {
                    soc_cm_sfree(unit, ptab_buf);
                }
                return (BCM_E_MEMORY);
            }

            s_ent_p->hw_index = index;
            soc_mem_mac_addr_get(unit, PORT_TABm, &ptab,
                                 MAC_ADDRESSf, mac_addr);
            s_ent_p->flags = _BCM_L2_STATION_ENTRY_INSTALLED;
            s_ent_p->flags |= _BCM_L2_STATION_ENTRY_TYPE_NON_TCAM;
            s_ent_p->prio = 0;
            s_ent_p->sid = last_allocated_port_mac_sid++;
            sc->entry_arr[sc->entries_total + index] = s_ent_p;
            sc->port_entries_free--;
        }
    }
    if (ptab_buf) {
        soc_cm_sfree(unit, ptab_buf);
    }

    entry_mem_size = sizeof(egr_olp_dgpp_config_entry_t);

    /* Allocate buffer to store the DMAed table entries. */
    eolp_dgpp_cfg_buf = soc_cm_salloc(unit, entry_mem_size * sc->olp_entries_total,
                              "Port table entry buffer");
    if (NULL == eolp_dgpp_cfg_buf) {
        return (BCM_E_MEMORY);
    }
    /* Initialize the entry buffer. */
    sal_memset(eolp_dgpp_cfg_buf, 0, 
               sizeof(entry_mem_size) * sc->olp_entries_total);

    /* Read the table entries into the buffer. */
    rv = soc_mem_read_range(unit, EGR_OLP_DGPP_CONFIGm, MEM_BLOCK_ALL,
                            0, (sc->olp_entries_total - 1), eolp_dgpp_cfg_buf);
    if (BCM_FAILURE(rv)) {
        if (eolp_dgpp_cfg_buf) {
            soc_cm_sfree(unit, eolp_dgpp_cfg_buf);
        }
        return rv;
    }
    /* Iterate over the olp mac entries. */
    for (index = 0; index < sc->olp_entries_total; index++) {
        eolp_dgpp_cfg = soc_mem_table_idx_to_pointer
                    (unit, EGR_OLP_DGPP_CONFIGm, egr_olp_dgpp_config_entry_t *,
                     eolp_dgpp_cfg_buf, index);

        soc_mem_field_get(unit, EGR_OLP_DGPP_CONFIGm, (uint32 *)eolp_dgpp_cfg, 
                                                  MACDAf, mac_field);

        if ((mac_field[0] > 0) || (mac_field[1] > 0)) {
            s_ent_p = (_bcm_l2_station_entry_t *)
                sal_alloc(sizeof(_bcm_l2_station_entry_t),
                                    "Sw L2 station entry");
            if (NULL == s_ent_p) {
                if (eolp_dgpp_cfg_buf) {
                    soc_cm_sfree(unit, eolp_dgpp_cfg_buf);
                }
                return (BCM_E_MEMORY);
            }
            soc_mem_mac_addr_get(unit, EGR_OLP_DGPP_CONFIGm, &eolp_dgpp_cfg,
                                             MACDAf, mac_addr);
            s_ent_p->flags = _BCM_L2_STATION_ENTRY_INSTALLED;
            s_ent_p->flags |= _BCM_L2_STATION_ENTRY_TYPE_NON_TCAM;
            s_ent_p->prio = 0;
            s_ent_p->sid = last_allocated_port_mac_sid++;
            s_ent_p->sid = last_allocated_olp_sid++;
            sc->entry_arr[sc->entries_total +
                           sc->port_entries_total + index] = s_ent_p;
            s_ent_p->hw_index = index;
            sc->port_entries_free--;
        }
    }

    if (eolp_dgpp_cfg_buf) {
        soc_cm_sfree(unit, eolp_dgpp_cfg_buf);
    }

    SOC_IF_ERROR_RETURN(
             READ_EGR_OLP_CONFIGm(unit, MEM_BLOCK_ANY, 0, &eolp_cfg));

    soc_mem_field_get(unit, EGR_OLP_CONFIGm, (uint32 *)&eolp_cfg, 
                                                  MACSAf, mac_field);
    if ((mac_field[0] > 0) || (mac_field[1] > 0)) {
        s_ent_p = (_bcm_l2_station_entry_t *)
                sal_alloc(sizeof(_bcm_l2_station_entry_t),
                                    "Sw L2 station entry");
        if (NULL == s_ent_p) {
            return (BCM_E_MEMORY);
        }
        soc_mem_mac_addr_get(unit, EGR_OLP_CONFIGm, &eolp_cfg,
                                             MACSAf, mac_addr);
        s_ent_p->flags = _BCM_L2_STATION_ENTRY_INSTALLED;
        s_ent_p->flags |= _BCM_L2_STATION_ENTRY_TYPE_NON_TCAM;
        s_ent_p->prio = 0;
        sc->entry_arr[sc->entries_total + sc->port_entries_total + 
                       sc->olp_entries_total + index] = s_ent_p;
        s_ent_p->hw_index = 0;
    }
    return (BCM_E_NONE);
}
#endif /* BCM_KATANA2_SUPPORT */
/*
 * Function:
 *      _bcm_tr_l2_reload_mbi
 * Description:
 *      Load MAC block info from hardware into software data structures.
 * Parameters:
 *      unit - StrataSwitch unit number.
 */
int
_bcm_tr_l2_reload_mbi(int unit)
{
    _bcm_mac_block_info_t *mbi = _mbi_entries[unit];
    l2x_entry_t         *l2x_entry, *l2x_table;
    mac_block_entry_t   mbe;
    int                 index, mb_index, l2x_size;
    pbmp_t              mb_pbmp;

    /*
     * Refresh MAC Block information from the hardware tables.
     */

    for (mb_index = 0; mb_index < _mbi_num[unit]; mb_index++) {
        SOC_IF_ERROR_RETURN
            (READ_MAC_BLOCKm(unit, MEM_BLOCK_ANY, mb_index, &mbe));

        SOC_PBMP_CLEAR(mb_pbmp);

        if (soc_mem_field_valid(unit, MAC_BLOCKm, MAC_BLOCK_MASK_LOf)) {
            SOC_PBMP_WORD_SET(mb_pbmp, 0,
                              soc_MAC_BLOCKm_field32_get(unit, &mbe, 
                                                         MAC_BLOCK_MASK_LOf));
        } else {
            soc_mem_pbmp_field_set(unit, MAC_BLOCKm, &mbe, MAC_BLOCK_MASKf,
                                       &mb_pbmp); 
        }
        if (soc_mem_field_valid(unit, MAC_BLOCKm, MAC_BLOCK_MASK_HIf)) {
            SOC_PBMP_WORD_SET(mb_pbmp, 1,
                          soc_MAC_BLOCKm_field32_get(unit, &mbe, 
                                                     MAC_BLOCK_MASK_HIf));
        }
        BCM_PBMP_ASSIGN(mbi[mb_index].mb_pbmp, mb_pbmp);
    }

    if (!SOC_CONTROL(unit)->l2x_group_enable) {
        l2x_size = sizeof(l2x_entry_t) * soc_mem_index_count(unit, L2Xm);
        l2x_table = soc_cm_salloc(unit, l2x_size, "l2 reload");
        if (l2x_table == NULL) {
            return BCM_E_MEMORY;
        }

        memset((void *)l2x_table, 0, l2x_size);
        if (soc_mem_read_range(unit, L2Xm, MEM_BLOCK_ANY,
                               soc_mem_index_min(unit, L2Xm),
                               soc_mem_index_max(unit, L2Xm),
                               l2x_table) < 0) {
            soc_cm_sfree(unit, l2x_table);
            return SOC_E_INTERNAL;
        }

        for (index = soc_mem_index_min(unit, L2Xm);
             index <= soc_mem_index_max(unit, L2Xm); index++) {

             l2x_entry = soc_mem_table_idx_to_pointer(unit, L2Xm,
                                                      l2x_entry_t *,
                                                      l2x_table, index);
             if (!soc_L2Xm_field32_get(unit, l2x_entry, VALIDf)) {
                 continue;
             }
  
             mb_index = soc_L2Xm_field32_get(unit, l2x_entry, MAC_BLOCK_INDEXf);
             mbi[mb_index].ref_count++;
        }
        soc_cm_sfree(unit, l2x_table);
    }

    return BCM_E_NONE;
}

#if defined(BCM_TRIUMPH_SUPPORT) /* BCM_TRIUMPH_SUPPORT */
/*
 * Function:
 *     _bcm_tr_l2_station_reload
 * Purpose:
 *     Re-construct L2 station control structure software state
 *     from installed hardware entries
 * Parameters:
 *     unit       - (IN) BCM device number
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_tr_l2_station_reload(int unit)
{
    _bcm_l2_station_control_t *sc;               /* Pointer to station        */
                                                 /* control structure.        */
    int                       rv;                /* Operation return status.  */
    uint32                    *tcam_buf = NULL;  /* Buffer to store TCAM      */
                                                 /* table entries.            */
    soc_mem_t                 tcam_mem;          /* Memory name.              */
    int                       entry_mem_size;    /* Size of TCAM table entry. */
    int                       index;             /* Table index iterator.     */
    _bcm_l2_station_entry_t   *s_ent_p;          /* Station table entry.      */
    mpls_station_tcam_entry_t *mpls_station_ent; /* Table entry pointer.      */
#if defined(BCM_TRIDENT_SUPPORT)
    my_station_tcam_entry_t   *my_station_ent;   /* Table entry pointer.      */
#endif

#if defined(BCM_TRIDENT_SUPPORT)
    if (SOC_IS_KATANAX(unit)
        || SOC_IS_TRIDENT(unit)
        || SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {

        entry_mem_size = sizeof(my_station_tcam_entry_t);

    } else 
#endif

    if (SOC_IS_ENDURO(unit) || SOC_IS_TRIUMPH(unit)
        || SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit)
        || SOC_IS_VALKYRIE(unit) || SOC_IS_VALKYRIE2(unit)
        || SOC_IS_HURRICANEX(unit)) {

        entry_mem_size = sizeof(mpls_station_tcam_entry_t);

    } else {
        entry_mem_size = 0;
    }

    if (0 == entry_mem_size) {
        return (BCM_E_INTERNAL);
    }

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    SC_LOCK(sc);

    rv = _bcm_l2_station_tcam_mem_get(unit, &tcam_mem);
    if (BCM_FAILURE(rv)) {
        SC_UNLOCK(sc);
        return (rv);
    }

    /* Allocate buffer to store the DMAed table entries. */
    tcam_buf = soc_cm_salloc(unit, entry_mem_size * sc->entries_total,
                              "STATION TCAM buffer");
    if (NULL == tcam_buf) {
        SC_UNLOCK(sc);
        return (BCM_E_MEMORY);
    }
    /* Initialize the entry buffer. */
    sal_memset(tcam_buf, 0, sizeof(entry_mem_size) * sc->entries_total);


    /* Read the table entries into the buffer. */
    rv = soc_mem_read_range(unit, tcam_mem, MEM_BLOCK_ALL,
                            0, (sc->entries_total - 1), tcam_buf);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    /* Iterate over the table entries. */
    for (index = 0; index < sc->entries_total; index++) {
#if defined(BCM_TRIDENT_SUPPORT)
        if (SOC_IS_KATANAX(unit)
            || SOC_IS_TRIDENT(unit)
            || SOC_IS_TRIUMPH3(unit)) {

            my_station_ent
                = soc_mem_table_idx_to_pointer
                    (unit, tcam_mem, my_station_tcam_entry_t *,
                     tcam_buf, index);

            if (0 == soc_MY_STATION_TCAMm_field32_get
                        (unit, my_station_ent, VALIDf)) {
                continue;
            }

        } else
#endif
        if (SOC_IS_ENDURO(unit) || SOC_IS_TRIUMPH(unit)
            || SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit)
            || SOC_IS_VALKYRIE(unit) || SOC_IS_VALKYRIE2(unit)
            || SOC_IS_HURRICANEX(unit)) {

            mpls_station_ent
                = soc_mem_table_idx_to_pointer
                    (unit, tcam_mem, mpls_station_tcam_entry_t *,
                     tcam_buf, index);

            if (0 == soc_MPLS_STATION_TCAMm_field32_get
                        (unit, mpls_station_ent, VALIDf)) {
                continue;
            }
        }

        s_ent_p
            = (_bcm_l2_station_entry_t *)
                sal_alloc(sizeof(_bcm_l2_station_entry_t),
                          "Sw L2 station entry");
        if (NULL == s_ent_p) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }

        sal_memset(s_ent_p, 0, sizeof(_bcm_l2_station_entry_t));

        s_ent_p->sid = ++last_allocated_sid;
        s_ent_p->hw_index = index;
        s_ent_p->prio = (sc->entries_total - index);
        s_ent_p->flags |= _BCM_L2_STATION_ENTRY_INSTALLED;
        sc->entries_free--;
        sc->entry_count++;
        sc->entry_arr[index] = s_ent_p;
    }
#if defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_KATANA2(unit)) {
        rv = _bcm_kt2_l2_station_non_tcam_reload(unit, sc); 
    }
#endif 

cleanup:

    SC_UNLOCK(sc);

    if (tcam_buf) {
        soc_cm_sfree(unit, tcam_buf);
    }

    return (rv);
}
#endif /* !BCM_TRIUMPH_SUPPORT */
#endif /* BCM_WARM_BOOT_SUPPORT */



/*
 * Function:
 *      _tr_l2x_delete_all
 * Purpose:
 *      Clear the L2 table by invalidating entries.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      static_too - if TRUE, delete static and non-static entries;
 *                   if FALSE, delete only non-static entries
 * Returns:
 *      SOC_E_XXX
 */

static int
_tr_l2x_delete_all(int unit)
{
    soc_control_t *soc = SOC_CONTROL(unit);
    int index_min, index_max, index, mem_max;
    l2_entry_only_entry_t *l2x_entry;
    int rv = SOC_E_NONE;
    int *buffer = NULL;
    int mem_size, idx;
    int modified;
    uint32 key_type;

#ifdef BCM_TRIDENT_SUPPORT
    if (soc_feature(unit, soc_feature_l2_bulk_control)) {
        l2_bulk_match_mask_entry_t match_mask;
        l2_bulk_match_data_entry_t match_data;
        int field_len;

        sal_memset(&match_mask, 0, sizeof(match_mask));
        sal_memset(&match_data, 0, sizeof(match_data));

        soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask, VALIDf, 1);
        soc_mem_field32_set(unit, L2_BULK_MATCH_DATAm, &match_data, VALIDf, 1);

        field_len = soc_mem_field_length(unit, L2_BULK_MATCH_MASKm, KEY_TYPEf);
        soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask, KEY_TYPEf,
                            (1 << field_len) - 1);

        soc_mem_lock(unit, L2Xm);
        rv = soc_reg_field32_modify(unit, L2_BULK_CONTROLr, REG_PORT_ANY,
                                    ACTIONf, 1);
        if (BCM_SUCCESS(rv)) {
            rv = WRITE_L2_BULK_MATCH_MASKm(unit, MEM_BLOCK_ALL, 0,
                                           &match_mask);
        }

        /* Remove all KEY_TYPE 0 entries */
        if (BCM_SUCCESS(rv)) {
            rv = WRITE_L2_BULK_MATCH_DATAm(unit, MEM_BLOCK_ALL, 0,
                                           &match_data);
            if (BCM_SUCCESS(rv)) {
                rv = soc_l2x_port_age(unit, L2_BULK_CONTROLr, INVALIDr);
            }
        }

        /* Remove all KEY_TYPE 3 entries */
        if (BCM_SUCCESS(rv)) {
            soc_mem_field32_set(unit, L2_BULK_MATCH_DATAm, &match_data,
                                KEY_TYPEf, 3);
            rv = WRITE_L2_BULK_MATCH_DATAm(unit, MEM_BLOCK_ALL, 0,
                                           &match_data);
            if (BCM_SUCCESS(rv)) {
                rv = soc_l2x_port_age(unit, L2_BULK_CONTROLr, INVALIDr);
            }
        }
    } else
#endif /* BCM_TRIDENT_SUPPORT */
    {
        index_min = soc_mem_index_min(unit, L2_ENTRY_ONLYm);
        mem_max = soc_mem_index_max(unit, L2_ENTRY_ONLYm);
        mem_size =  DEFAULT_L2DELETE_CHUNKS * sizeof(l2_entry_only_entry_t);
    
        buffer = soc_cm_salloc(unit, mem_size, "L2_ENTRY_ONLY_delete");
        if (NULL == buffer) {
            return SOC_E_MEMORY;
        }

        soc_mem_lock(unit, L2Xm);
        for (idx = index_min; idx < mem_max; idx += DEFAULT_L2DELETE_CHUNKS) {
            index_max = idx + DEFAULT_L2DELETE_CHUNKS - 1;
            if ( index_max > mem_max) {
                index_max = mem_max;
            }
            if ((rv = soc_mem_read_range(unit, L2_ENTRY_ONLYm, MEM_BLOCK_ALL,
                                         idx, index_max, buffer)) < 0 ) {
                soc_cm_sfree(unit, buffer);
                soc_mem_unlock(unit, L2Xm);
                return rv;
            }
            modified = FALSE;
            for (index = 0; index < DEFAULT_L2DELETE_CHUNKS; index++) {
                l2x_entry =
                    soc_mem_table_idx_to_pointer(unit, L2_ENTRY_ONLYm,
                                                 l2_entry_only_entry_t *,
                                                 buffer, index);
                if (!soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, VALIDf)) {
                    continue;
                }
                key_type = soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry,
                                                          KEY_TYPEf);
                if (key_type ==  TR_L2_HASH_KEY_TYPE_BRIDGE ||
                    key_type == TR_L2_HASH_KEY_TYPE_VFI) {
                    sal_memcpy(l2x_entry,
                               soc_mem_entry_null(unit, L2_ENTRY_ONLYm),
                               sizeof(l2_entry_only_entry_t));
                    modified = TRUE;
                }
            }
            if (!modified) {
                continue;
            }
            if ((rv = soc_mem_write_range(unit, L2_ENTRY_ONLYm, MEM_BLOCK_ALL, 
                                          idx, index_max, buffer)) < 0) {
                soc_cm_sfree(unit, buffer);
                soc_mem_unlock(unit, L2Xm);
                return rv;
            }
        }
        soc_cm_sfree(unit, buffer);
    }

    if (soc->arlShadow != NULL) {
        sal_mutex_take(soc->arlShadowMutex, sal_mutex_FOREVER);
        (void) shr_avl_delete_all(soc->arlShadow);
        sal_mutex_give(soc->arlShadowMutex);
    }

    /* Clear external L2 table if it exists */
    if (soc_mem_is_valid(unit, EXT_L2_ENTRYm) &&
        soc_mem_index_count(unit, EXT_L2_ENTRYm)) {
        SOC_IF_ERROR_RETURN(soc_mem_clear(unit, EXT_L2_ENTRY_TCAMm,
                                          MEM_BLOCK_ALL, TRUE));
        SOC_IF_ERROR_RETURN(soc_mem_clear(unit, EXT_L2_ENTRY_DATAm,
                                          MEM_BLOCK_ALL, TRUE));
    }
    soc_mem_unlock(unit, L2Xm);

    return rv;
}

/*
 * Function:
 *      bcm_tr_l2_init
 * Description:
 *      Initialize chip-dependent parts of L2 module
 * Parameters:
 *      unit - StrataSwitch unit number.
 */

int
bcm_tr_l2_init(int unit)
{
    int         was_running = FALSE;
    uint32      flags;
    sal_usecs_t interval;

    if (soc_l2x_running(unit, &flags, &interval)) { 	 
        was_running = TRUE; 	 
        BCM_IF_ERROR_RETURN(soc_l2x_stop(unit)); 	 
    }

    if (!SOC_WARM_BOOT(unit) && !SOC_IS_RCPU_ONLY(unit)) {
        if (!(SAL_BOOT_QUICKTURN || SAL_BOOT_SIMULATION || SAL_BOOT_BCMSIM
              || SAL_BOOT_XGSSIM)) {
            _tr_l2x_delete_all(unit);
        }
    }

    if (_mbi_entries[unit] != NULL) {
        sal_free(_mbi_entries[unit]);
        _mbi_entries[unit] = NULL;
    }

    _mbi_num[unit] = (SOC_MEM_INFO(unit, MAC_BLOCKm).index_max -
                      SOC_MEM_INFO(unit, MAC_BLOCKm).index_min + 1);
    _mbi_entries[unit] = sal_alloc(_mbi_num[unit] *
                                   sizeof(_bcm_mac_block_info_t),
                                   "BCM L2X MAC blocking info");
    if (!_mbi_entries[unit]) {
        return BCM_E_MEMORY;
    }
    sal_memset(_mbi_entries[unit], 0, _mbi_num[unit] * sizeof(_bcm_mac_block_info_t));

#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TR_VL(unit)
        && !SOC_IS_HURRICANEX(unit))  {
        BCM_IF_ERROR_RETURN(_bcm_tr_l2_station_control_init(unit));
    }
#endif

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_tr_l2_reload_mbi(unit));

#if defined(BCM_TRIUMPH_SUPPORT)
        if (SOC_IS_TR_VL(unit)
            && !SOC_IS_HURRICANEX(unit)) {
            BCM_IF_ERROR_RETURN(_bcm_tr_l2_station_reload(unit));
        }
#endif

    }
#endif

    /* bcm_l2_register clients */
    
    soc_l2x_register(unit,
            _bcm_l2_register_callback,
            NULL);

    if (was_running) {
        interval = (SAL_BOOT_BCMSIM)? BCMSIM_L2XMSG_INTERVAL : interval;
        soc_l2x_start(unit, flags, interval);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr_l2_term
 * Description:
 *      Finalize chip-dependent parts of L2 module
 * Parameters:
 *      unit - StrataSwitch unit number.
 */

int
bcm_tr_l2_term(int unit)
{
    if (_mbi_entries[unit] != NULL) {
        sal_free(_mbi_entries[unit]);
        _mbi_entries[unit] = NULL;
    }

#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TR_VL(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_tr_l2_station_control_deinit(unit));
    }
#endif

    return BCM_E_NONE;
}

STATIC int
_bcm_tr_dual_l2_conflict_get(int unit, bcm_l2_addr_t *addr,
                             bcm_l2_addr_t *cf_array, int cf_max,
                             int *cf_count)
{
    l2x_entry_t     match_entry, entry;
    uint32          fval;
    int             bank, bucket, slot, index;
    int             num_banks;
    int             entries_per_bank, entries_per_row, entries_per_bucket;
    int             bank_base, bucket_offset;

#ifdef BCM_TRIDENT2_SPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        BCM_IF_ERROR_RETURN
            (soc_trident2_hash_bank_count_get(unit, L2Xm, &num_banks));
    } else
#endif
    {
        num_banks = 2;
    }

    *cf_count = 0;
    BCM_IF_ERROR_RETURN(_bcm_tr_l2_to_l2x(unit, &match_entry, addr, TRUE));
    for (bank = 0; bank < num_banks; bank++) {
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TD2_TT2(unit)) {
            BCM_IF_ERROR_RETURN
                (soc_trident2_hash_bank_info_get(unit, L2Xm, bank,
                                                 &entries_per_bank,
                                                 &entries_per_row,
                                                 &entries_per_bucket,
                                                 &bank_base, &bucket_offset));
            bucket = soc_td2_l2x_bank_entry_hash(unit, bank,
                                                 (uint32 *)&match_entry);
        } else
#endif
        {
            entries_per_bank = soc_mem_index_count(unit, L2Xm) / 2;
            entries_per_row = 8;
            entries_per_bucket = entries_per_row / 2;
            bank_base = 0;
            bucket_offset = bank * entries_per_bucket;
            bucket = soc_tr_l2x_bank_entry_hash(unit, bank,
                                                (uint32 *)&match_entry);
        }

        for (slot = 0; slot < entries_per_bucket; slot++) {
            index = bank_base + bucket * entries_per_row + bucket_offset +
                slot;
            SOC_IF_ERROR_RETURN
                (soc_mem_read(unit, L2Xm, MEM_BLOCK_ANY, index, &entry));
            if (!soc_L2Xm_field32_get(unit, &entry, VALIDf)) {
                continue;
            }

            fval = soc_mem_field32_get(unit, L2Xm, &entry, KEY_TYPEf);
#ifdef BCM_TRIDENT2_SUPPORT
            if (SOC_IS_TD2_TT2(unit)) {
                if (fval != TD2_L2_HASH_KEY_TYPE_BRIDGE &&
                    fval != TD2_L2_HASH_KEY_TYPE_VFI) {
                    continue;
                }
            } else
#endif /* BCM_TRIDENT2_SUPPORT */
            if (fval != TR_L2_HASH_KEY_TYPE_BRIDGE &&
                fval != TR_L2_HASH_KEY_TYPE_VFI) {
                continue;
            }

            /* Retrieve the hit bit for TD2/TT2 */
#ifdef BCM_TRIDENT2_SUPPORT
            if (SOC_IS_TD2_TT2(unit)) {
                BCM_IF_ERROR_RETURN
                    (_bcm_td2_l2_hit_retrieve(unit, &entry, index));
            }
#endif /* BCM_TRIDENT2_SUPPORT */   
            BCM_IF_ERROR_RETURN
                (_bcm_tr_l2_from_l2x(unit, &cf_array[*cf_count], &entry));

            *cf_count += 1;
        }
    }

    COMPILER_REFERENCE(entries_per_bank);

    return BCM_E_NONE;
}

int
bcm_tr_l2_conflict_get(int unit, bcm_l2_addr_t *addr,
                          bcm_l2_addr_t *cf_array, int cf_max,
                          int *cf_count)
{
    l2x_entry_t         l2ent;
    uint8               key[XGS_HASH_KEY_SIZE];
    int                 hash_sel, bucket, slot, num_bits;
    uint32              hash_control;

    if (soc_feature(unit, soc_feature_dual_hash)) {
        return _bcm_tr_dual_l2_conflict_get(unit, addr, cf_array,
                                            cf_max, cf_count);
    }

    *cf_count = 0;

    BCM_IF_ERROR_RETURN(_bcm_tr_l2_to_l2x(unit, &l2ent, addr, TRUE));

    /* Get L2 hash select */
    SOC_IF_ERROR_RETURN(READ_HASH_CONTROLr(unit, &hash_control));
    hash_sel = soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                                    L2_AND_VLAN_MAC_HASH_SELECTf);
    num_bits = soc_tr_l2x_base_entry_to_key(unit, &l2ent, key);
    bucket = soc_tr_l2x_hash(unit, hash_sel, num_bits, &l2ent, key);

    for (slot = 0;
         slot < SOC_L2X_BUCKET_SIZE && *cf_count < cf_max;
         slot++) {
        SOC_IF_ERROR_RETURN
            (soc_mem_read(unit, L2Xm, MEM_BLOCK_ANY,
                          bucket * SOC_L2X_BUCKET_SIZE + slot,
                          &l2ent));
        if (!soc_L2Xm_field32_get(unit, &l2ent, VALIDf)) {
            continue;
        }
        if ((soc_L2Xm_field32_get(unit, &l2ent, KEY_TYPEf) == 
                                  TR_L2_HASH_KEY_TYPE_BRIDGE) ||
            (soc_L2Xm_field32_get(unit, &l2ent, KEY_TYPEf) ==
                                  TR_L2_HASH_KEY_TYPE_VFI)) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr_l2_from_l2x(unit, &cf_array[*cf_count], &l2ent));
            *cf_count += 1;
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_vlan_cross_connect_add
 * Purpose:
 *      Add a VLAN cross connect entry
 * Parameters:
 *      unit       - Device unit number
 *      outer_vlan - Outer vlan ID
 *      inner_vlan - Inner vlan ID
 *      port_1     - First port in the cross-connect
 *      port_2     - Second port in the cross-connect
 * Returns:
 *      BCM_E_NONE - Success.
 *      BCM_E_XXX
 */
int 
bcm_tr_l2_cross_connect_add(int unit, bcm_vlan_t outer_vlan, 
                            bcm_vlan_t inner_vlan, bcm_gport_t port_1, 
                            bcm_gport_t port_2)
{
    l2x_entry_t  l2x_entry, l2x_lookup;
    int rv, gport_id, l2_index;
    bcm_port_t port_out;
    bcm_module_t mod_out;
    bcm_trunk_t trunk_id;

    /* Just a safety Check */
    if (!SOC_MEM_IS_VALID(unit,L2Xm)) {
        /* Control should not reach ! */
        return (BCM_E_INTERNAL);
    }
    sal_memset(&l2x_entry, 0, sizeof (l2x_entry));
    if ((outer_vlan < BCM_VLAN_DEFAULT) || (outer_vlan > BCM_VLAN_MAX)) {
        return BCM_E_PARAM;
    } else if (inner_vlan == BCM_VLAN_INVALID) {
        /* Single cross-connect (use only outer_vid) */
        soc_L2Xm_field32_set(unit, &l2x_entry, KEY_TYPEf,
                             TR_L2_HASH_KEY_TYPE_SINGLE_CROSS_CONNECT);
    } else {
        if ((inner_vlan < BCM_VLAN_DEFAULT) || (inner_vlan > BCM_VLAN_MAX)) {
            return BCM_E_PARAM;
        }
        /* Double cross-connect (use both outer_vid and inner_vid) */
        soc_L2Xm_field32_set(unit, &l2x_entry, KEY_TYPEf,
                             TR_L2_HASH_KEY_TYPE_DOUBLE_CROSS_CONNECT);
        soc_L2Xm_field32_set(unit, &l2x_entry, IVIDf, inner_vlan);
    }
    soc_L2Xm_field32_set(unit, &l2x_entry, STATIC_BITf, 1);
    soc_L2Xm_field32_set(unit, &l2x_entry, VALIDf, 1);
    soc_L2Xm_field32_set(unit, &l2x_entry, VLAN_IDf, outer_vlan);

    /* See if the entry already exists */
    rv = soc_mem_generic_lookup(unit, L2Xm, MEM_BLOCK_ANY, 0, &l2x_entry,
                                &l2x_lookup, &l2_index);
                
    if ((rv < 0) && (rv != BCM_E_NOT_FOUND)) {
         return rv;
    } 

    /* Resolve first port */
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port_1, &mod_out, &port_out, &trunk_id,
                                &gport_id));
    if (BCM_GPORT_IS_TRUNK(port_1)) {
        soc_L2Xm_field32_set(unit, &l2x_entry, Tf, 1);
        soc_L2Xm_field32_set(unit, &l2x_entry, TGIDf, trunk_id);
    } else if (BCM_GPORT_IS_SUBPORT_GROUP(port_1)) {
        soc_L2Xm_field32_set(unit, &l2x_entry, VPG_TYPEf, 1);
#if defined(BCM_SCORPION_SUPPORT) && defined(INCLUDE_L3)
           if (SOC_IS_SC_CQ(unit)) {
               /* Map the gport_id to index to L3_NEXT_HOP */
               gport_id = (int) _sc_subport_group_index[unit][gport_id/8];
           }
#endif /* BCM_SCORPION_SUPPORT && INCLUDE_L3 */
        soc_L2Xm_field32_set(unit, &l2x_entry, VPGf, gport_id);
    } else {
        if ((mod_out == -1) ||(port_out == -1)) {
            return BCM_E_PORT;
        }
        soc_L2Xm_field32_set(unit, &l2x_entry, MODULE_IDf, mod_out);
        soc_L2Xm_field32_set(unit, &l2x_entry, PORT_NUMf, port_out);
    }

    /* Resolve second port */
    BCM_IF_ERROR_RETURN 
        (_bcm_esw_gport_resolve(unit, port_2, &mod_out, &port_out, &trunk_id,
                                &gport_id));
    if (BCM_GPORT_IS_TRUNK(port_2)) {
        soc_L2Xm_field32_set(unit, &l2x_entry, T_1f, 1);
        soc_L2Xm_field32_set(unit, &l2x_entry, TGID_1f, trunk_id);
    } else if (BCM_GPORT_IS_SUBPORT_GROUP(port_2)) {
        soc_L2Xm_field32_set(unit, &l2x_entry, VPG_TYPE_1f, 1);
#if defined(BCM_SCORPION_SUPPORT) && defined(INCLUDE_L3)
           if (SOC_IS_SC_CQ(unit)) {
               /* Map the gport_id to index to L3_NEXT_HOP */
               gport_id = (int) _sc_subport_group_index[unit][gport_id/8];
           }
#endif /* BCM_SCORPION_SUPPORT && INCLUDE_L3 */
        soc_L2Xm_field32_set(unit, &l2x_entry, VPG_1f, gport_id);
    } else {
        if ((mod_out == -1) ||(port_out == -1)) {
            return BCM_E_PORT;
        }
        soc_L2Xm_field32_set(unit, &l2x_entry, MODULE_ID_1f, mod_out);
        soc_L2Xm_field32_set(unit, &l2x_entry, PORT_NUM_1f, port_out);
    }

    rv = soc_mem_insert_return_old(unit, L2Xm, MEM_BLOCK_ANY, 
                                   (void *)&l2x_entry, (void *)&l2x_entry);
    if (rv == BCM_E_FULL) {
        rv = _bcm_l2_hash_dynamic_replace( unit, &l2x_entry);
    } 
    if (BCM_SUCCESS(rv)) {
        if (soc_feature(unit, soc_feature_ppa_bypass)) {
            SOC_CONTROL(unit)->l2x_ppa_bypass = TRUE;
        }
    }

    return rv;
}

/*
 * Function:
 *      bcm_vlan_cross_connect_delete
 * Purpose:
 *      Delete a VLAN cross connect entry
 * Parameters:
 *      unit       - Device unit number
 *      outer_vlan - Outer vlan ID
 *      inner_vlan - Inner vlan ID
 * Returns:
 *      BCM_E_NONE - Success.
 *      BCM_E_XXX
 */
int 
bcm_tr_l2_cross_connect_delete(int unit, bcm_vlan_t outer_vlan, 
                               bcm_vlan_t inner_vlan)
{
    l2x_entry_t  l2x_entry, l2x_lookup;
    int rv, l2_index;

    sal_memset(&l2x_entry, 0, sizeof (l2x_entry));
    if ((outer_vlan < BCM_VLAN_DEFAULT) || (outer_vlan > BCM_VLAN_MAX)) {
        return BCM_E_PARAM;
    } else if (inner_vlan == BCM_VLAN_INVALID) {
        /* Single cross-connect (use only outer_vid) */
        soc_L2Xm_field32_set(unit, &l2x_entry, KEY_TYPEf,
                             TR_L2_HASH_KEY_TYPE_SINGLE_CROSS_CONNECT);
    } else {
        if ((inner_vlan < BCM_VLAN_DEFAULT) || (inner_vlan > BCM_VLAN_MAX)) {
            return BCM_E_PARAM;
        }
        /* Double cross-connect (use both outer_vid and inner_vid) */
        soc_L2Xm_field32_set(unit, &l2x_entry, KEY_TYPEf,
                             TR_L2_HASH_KEY_TYPE_DOUBLE_CROSS_CONNECT);
        soc_L2Xm_field32_set(unit, &l2x_entry, IVIDf, inner_vlan);
    }
    soc_L2Xm_field32_set(unit, &l2x_entry, STATIC_BITf, 1);
    soc_L2Xm_field32_set(unit, &l2x_entry, VALIDf, 1);
    soc_L2Xm_field32_set(unit, &l2x_entry, VLAN_IDf, outer_vlan);

    rv = soc_mem_search(unit, L2Xm, MEM_BLOCK_ANY, &l2_index, 
                        (void *)&l2x_entry, (void *)&l2x_lookup, 0);
                 
    if ((rv < 0) && (rv != BCM_E_NOT_FOUND)) {
         return rv;
    } 

    rv = soc_mem_delete(unit, L2Xm, MEM_BLOCK_ANY, (void *)&l2x_entry);
    return rv;
}

/*
 * Function:
 *      bcm_vlan_cross_connect_delete_all
 * Purpose:
 *      Delete all VLAN cross connect entries
 * Parameters:
 *      unit       - Device unit number
 * Returns:
 *      BCM_E_NONE - Success.
 *      BCM_E_XXX
 */
int
bcm_tr_l2_cross_connect_delete_all(int unit)
{
    soc_control_t  *soc = SOC_CONTROL(unit);
    int index_min, index_max, index, mem_max;
    l2_entry_only_entry_t *l2x_entry;
    int rv = SOC_E_NONE;
    int *buffer = NULL;
    int mem_size, idx;
    soc_mem_t mem = L2_ENTRY_ONLYm;
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANAX(unit)) {
        mem = L2Xm;
    }
#endif

#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
       mem = L2Xm;
    }
#endif 

    index_min = soc_mem_index_min(unit, mem);
    mem_max = soc_mem_index_max(unit, mem);
    mem_size =  DEFAULT_L2DELETE_CHUNKS * sizeof(l2_entry_only_entry_t);
    
    buffer = soc_cm_salloc(unit, mem_size, "L2_ENTRY_ONLY_delete");
    if (NULL == buffer) {
        return SOC_E_MEMORY;
    }

    soc_mem_lock(unit, L2Xm);
    for (idx = index_min; idx < mem_max; idx += DEFAULT_L2DELETE_CHUNKS) {
        index_max = idx + DEFAULT_L2DELETE_CHUNKS - 1;
        if ( index_max > mem_max) {
            index_max = mem_max;
        }
        if ((rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ALL,
                                     idx, index_max, buffer)) < 0 ) {
            soc_cm_sfree(unit, buffer);
            soc_mem_unlock(unit, L2Xm);
            return rv;
        }
        for (index = 0; index < DEFAULT_L2DELETE_CHUNKS; index++) {
            l2x_entry =
                soc_mem_table_idx_to_pointer(unit, mem,
                                             l2_entry_only_entry_t *, buffer, index);
            if (soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, VALIDf) &&
                ((soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, KEY_TYPEf) ==
                                      TR_L2_HASH_KEY_TYPE_SINGLE_CROSS_CONNECT) ||
                 (soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, KEY_TYPEf) ==
                                      TR_L2_HASH_KEY_TYPE_DOUBLE_CROSS_CONNECT))) {
                sal_memcpy(l2x_entry, soc_mem_entry_null(unit, mem),
                           sizeof(l2_entry_only_entry_t));
            }
        }
        if ((rv = soc_mem_write_range(unit, mem, MEM_BLOCK_ALL,
                                     idx, index_max, buffer)) < 0) {
            soc_cm_sfree(unit, buffer);
            soc_mem_unlock(unit, L2Xm);
            return rv;
        }
    }

    if (soc->arlShadow != NULL) {
        sal_mutex_take(soc->arlShadowMutex, sal_mutex_FOREVER);
        (void) shr_avl_delete_all(soc->arlShadow);
        sal_mutex_give(soc->arlShadowMutex);
    }
    soc_cm_sfree(unit, buffer);
    soc_mem_unlock(unit, L2Xm);

    return rv;
}

/*
 * Function:
 *      bcm_vlan_cross_connect_traverse
 * Purpose:
 *      Walks through the valid cross connect entries and calls
 *      the user supplied callback function for each entry.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      trav_fn    - (IN) Callback function.
 *      user_data  - (IN) User data to be passed to callback function.
 * Returns:
 *      BCM_E_NONE - Success.
 *      BCM_E_XXX
 */
int
bcm_tr_l2_cross_connect_traverse(int unit,
                                 bcm_vlan_cross_connect_traverse_cb cb,
                                 void *user_data)
{
    int index_min, index_max, index, mem_max;
    l2_entry_only_entry_t *l2x_entry;
    int rv = SOC_E_NONE;
    int *buffer = NULL;
    int mem_size, idx;
    bcm_gport_t port_1, port_2;
    bcm_vlan_t outer_vlan, inner_vlan;
    bcm_port_t port_in, port_out;
    bcm_module_t mod_in, mod_out;
    soc_mem_t mem = L2_ENTRY_ONLYm;
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANAX(unit)) {
        mem = L2Xm;
    }
#endif 

    index_min = soc_mem_index_min(unit, mem);
    mem_max = soc_mem_index_max(unit, mem);
    mem_size =  DEFAULT_L2DELETE_CHUNKS * sizeof(l2_entry_only_entry_t);
    
    buffer = soc_cm_salloc(unit, mem_size, "cross connect traverse");
    if (NULL == buffer) {
        return SOC_E_MEMORY;
    }
    
    soc_mem_lock(unit, L2Xm);
    for (idx = index_min; idx < mem_max; idx += DEFAULT_L2DELETE_CHUNKS) {
        index_max = idx + DEFAULT_L2DELETE_CHUNKS - 1;
        if ( index_max > mem_max) {
            index_max = mem_max;
        }
        if ((rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ALL,
                                     idx, index_max, buffer)) < 0 ) {
            soc_cm_sfree(unit, buffer);
            soc_mem_unlock(unit, L2Xm);
            return rv;
        }
        for (index = 0; index < DEFAULT_L2DELETE_CHUNKS; index++) {
            l2x_entry = 
                soc_mem_table_idx_to_pointer(unit, mem,
                                             l2_entry_only_entry_t *, buffer, index);
            if (soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, VALIDf)) {
                if (soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, KEY_TYPEf) ==
                                     TR_L2_HASH_KEY_TYPE_DOUBLE_CROSS_CONNECT) {
                    /* Double cross-connect entry */
                    inner_vlan = soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, IVIDf);
                } else if (soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, KEY_TYPEf) ==
                                     TR_L2_HASH_KEY_TYPE_SINGLE_CROSS_CONNECT) {
                    /* Single cross-connect entry */
                    inner_vlan = BCM_VLAN_INVALID;
                } else {
                    /* Not a cross-connect entry, ignore */
                    continue;
                }
                outer_vlan = soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, OVIDf);

                /* Get first port params */
                if (SOC_MEM_FIELD_VALID(unit, mem, VPG_TYPEf) && 
                    soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, VPG_TYPEf)) {
                    int vpg;
                    vpg = soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, VPGf);
#if defined(BCM_SCORPION_SUPPORT) && defined(INCLUDE_L3)
                /* Scorpion uses index to L3_NEXT_HOP as VPG */
                    if (SOC_IS_SC_CQ(unit)) {
                        int grp;
                        _SC_SUBPORT_VPG_FIND(unit, vpg, grp);
                        if ((vpg = grp) == -1) {
                            L2_ERR(("Unit: %d can not find entry for VPG\n", unit));
                        }
                    }             
#endif  /* BCM_SCORPION_SUPPORT && INCLUDE_L3 */     
                    BCM_GPORT_SUBPORT_GROUP_SET(port_1, vpg);
                } else if (soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, Tf)) {
                    BCM_GPORT_TRUNK_SET(port_1, 
                        soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, TGIDf));
                } else {
                    port_in = soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, PORT_NUMf);
                    mod_in = soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, MODULE_IDf);
                    BCM_IF_ERROR_RETURN
                        (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                                mod_in, port_in, &mod_out, &port_out));
                    BCM_GPORT_MODPORT_SET(port_1, mod_out, port_out);
                }

                /* Get second port params */
                if (SOC_MEM_FIELD_VALID(unit, mem, VPG_TYPE_1f) && 
                    soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, VPG_TYPE_1f)) {
                    int vpg;
                    vpg = soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, VPG_1f);
#if defined(BCM_SCORPION_SUPPORT) && defined(INCLUDE_L3)
                    if (SOC_IS_SC_CQ(unit)) {
                        int grp;
                        _SC_SUBPORT_VPG_FIND(unit, vpg, grp);
                        if ((vpg = grp) == -1) {
                            L2_ERR(("Unit: %d can not find entry for VPG\n", unit));
                        }
                    }             
#endif  /* BCM_SCORPION_SUPPORT && INCLUDE_L3 */     
                    BCM_GPORT_SUBPORT_GROUP_SET(port_2, vpg);
                } else if (soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, T_1f)) {
                    BCM_GPORT_TRUNK_SET(port_2, 
                        soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, TGID_1f));
                } else {
                    port_in = soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, PORT_NUM_1f);
                    mod_in = soc_L2_ENTRY_ONLYm_field32_get(unit, l2x_entry, MODULE_ID_1f);
                    BCM_IF_ERROR_RETURN
                        (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                                mod_in, port_in, &mod_out, &port_out));
                    BCM_GPORT_MODPORT_SET(port_2, mod_out, port_out);
                }

                /* Call application call-back */
                rv = cb(unit, outer_vlan, inner_vlan, port_1, port_2, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
                if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                    soc_cm_sfree(unit, buffer);
                    soc_mem_unlock(unit, L2Xm);
                    return rv;
                }
#endif
            }
        }
    }
    soc_cm_sfree(unit, buffer);
    soc_mem_unlock(unit, L2Xm);

    return BCM_E_NONE;
}


#if defined(BCM_TRIUMPH_SUPPORT) /* BCM_TRIUMPH_SUPPORT */

STATIC int
_l2_port_mask_gport_resolve(int unit, bcm_gport_t gport,
                       bcm_module_t *modid, bcm_port_t *port,
                       bcm_trunk_t *trunk_id) 
{
    int rv = SOC_E_NONE;

    *modid = -1;
    *port = -1;
    *trunk_id = BCM_TRUNK_INVALID;
    
    if (SOC_GPORT_IS_TRUNK(gport)) {
        *trunk_id = SOC_GPORT_TRUNK_GET(gport);
    } else if (SOC_GPORT_IS_LOCAL(gport)) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_stk_my_modid_get(unit, modid));
        *port = SOC_GPORT_LOCAL_GET(gport);
    } else if (SOC_GPORT_IS_MODPORT(gport)) {
        *modid = SOC_GPORT_MODPORT_MODID_GET(gport);
        *port = SOC_GPORT_MODPORT_PORT_GET(gport);
    } else {
        rv = SOC_E_PORT;
    }
    return rv;
}

/*
 * Function:
 *     _bcm_l2_station_entry_set
 * Purpose:
 *     Set a station entrie's parameters to entry buffer for hardware table
 *     write operation.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     tcam_mem   - (IN) Hardware memory name.
 *     station    - (IN) Station parameters to be set in hardware.
 *     ent_p      - (IN) Software station entry pointer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_entry_set(int unit,
                          soc_mem_t tcam_mem,
                          bcm_l2_station_t *station,
                          _bcm_l2_station_entry_t *ent_p)
{

    soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                        VALIDf, 1);

    soc_mem_mac_addr_set(unit, tcam_mem, ent_p->tcam_ent,
                         MAC_ADDRf, station->dst_mac);

    soc_mem_mac_addr_set(unit, tcam_mem, ent_p->tcam_ent,
                         MAC_ADDR_MASKf, station->dst_mac_mask);

    soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                        VLAN_IDf, station->vlan);

    soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                        VLAN_ID_MASKf, station->vlan_mask);

#if defined(BCM_TRIDENT_SUPPORT)
    if (SOC_IS_KATANAX(unit)
        || SOC_IS_TRIDENT(unit)
        || SOC_IS_TD2_TT2(unit)
        || SOC_IS_TRIUMPH3(unit)) {
        int gport_id;
        bcm_port_t port_out = 0;
        bcm_module_t mod_out = 0;
        bcm_trunk_t trunk_id = BCM_TRUNK_INVALID;
        uint32 mod_port_data = 0; /* concatenated modid and port */
        bcm_port_t port_out_mask;
        bcm_module_t mod_out_mask;
        bcm_trunk_t trunk_id_mask = -1;  
        uint32 mod_port_mask = 0; /* concatenated modid and port */
        int rv=0;

        /* validate both src_port and src_port_mask has the same type */
        if (GPORT_TYPE(station->src_port) != 
                                   GPORT_TYPE(station->src_port_mask)) {
            return BCM_E_PARAM;
        }
 
        if (BCM_GPORT_IS_SET(station->src_port)) {
            rv = _bcm_esw_gport_resolve(unit, station->src_port, 
                          &mod_out, 
                          &port_out, &trunk_id,
                          &gport_id);
            BCM_IF_ERROR_RETURN(rv);

            if (BCM_GPORT_IS_TRUNK(station->src_port)) {
                if (BCM_TRUNK_INVALID == trunk_id) {
                    return BCM_E_PORT;
                }

            } else {
                if ((-1 == mod_out) || (-1 == port_out)) {
                    return BCM_E_PORT;
                }
            }
            /* retrieve the port mask */
            rv = _l2_port_mask_gport_resolve(unit,station->src_port_mask,
                          &mod_out_mask, 
                          &port_out_mask, &trunk_id_mask);
            BCM_IF_ERROR_RETURN(rv);
        } else {
            port_out = station->src_port;
            port_out_mask = station->src_port_mask;
            mod_out = 0;
            mod_out_mask  = 0; 
        }

        if (trunk_id != BCM_TRUNK_INVALID) {
            /* require hardware with valid SOURCE_FIELDf */
            if (!SOC_MEM_FIELD_VALID(unit, tcam_mem, SOURCE_FIELDf)) {
                return BCM_E_PORT;
            }
            /* and the SOURCE_FIELDf supports the trunk operation */
            SOC_IF_ERROR_RETURN(soc_mem_field32_fit(unit, tcam_mem, 
                                SOURCE_FIELD_MASKf, 
                                1 << SOC_TRUNK_BIT_POS(unit)));

            mod_port_data = ((1 << SOC_TRUNK_BIT_POS(unit)) | trunk_id);
            mod_port_mask = trunk_id_mask & 
                            ((1 << SOC_TRUNK_BIT_POS(unit)) - 1);
            mod_port_mask |= (1 << SOC_TRUNK_BIT_POS(unit));
            soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                       SOURCE_FIELDf, mod_port_data);
            soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                       SOURCE_FIELD_MASKf, mod_port_mask);
        } else {
            if (SOC_MEM_FIELD_VALID(unit, tcam_mem, SOURCE_FIELDf)) {
                int num_bits_for_port;
                num_bits_for_port = _shr_popcount(
                                 (unsigned int)SOC_PORT_ADDR_MAX(unit));

                mod_port_data = (mod_out << num_bits_for_port) | port_out;
                mod_port_mask = mod_out_mask & SOC_MODID_MAX(unit);
                mod_port_mask <<= num_bits_for_port;
                mod_port_mask |= (port_out_mask & SOC_PORT_ADDR_MAX(unit));

                /* Clear the trunk ID bit. */
                mod_port_data &= ~(1 << SOC_TRUNK_BIT_POS(unit));

                /* Must match on the T bit (which should be 0) for the devices
                 * supporting the trunk operation with this field
                 */
                if (BCM_GPORT_IS_SET(station->src_port)) {
                if (soc_mem_field32_fit(unit, tcam_mem, 
                                SOURCE_FIELD_MASKf, 
                                1 << SOC_TRUNK_BIT_POS(unit)) == SOC_E_NONE) {
                    mod_port_mask |= (1 << SOC_TRUNK_BIT_POS(unit));
                }
                }
                soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                                    SOURCE_FIELDf, mod_port_data);

                soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                                    SOURCE_FIELD_MASKf, mod_port_mask);
            } else {
                soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                                    ING_PORT_NUMf, port_out);

                soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                            ING_PORT_NUM_MASKf, 
                            port_out_mask & SOC_PORT_ADDR_MAX(unit));
            }
        } 
        soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                            MIM_TERMINATION_ALLOWEDf,
                            ((station->flags & BCM_L2_STATION_MIM) ? 1 : 0));

        soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                            MPLS_TERMINATION_ALLOWEDf,
                            ((station->flags & BCM_L2_STATION_MPLS) ? 1 : 0));
        
        soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                            IPV4_TERMINATION_ALLOWEDf,
                            ((station->flags & BCM_L2_STATION_IPV4) ? 1 : 0));

        soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                            IPV6_TERMINATION_ALLOWEDf,
                            ((station->flags & BCM_L2_STATION_IPV6) ? 1 : 0));
        
        soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                            ARP_RARP_TERMINATION_ALLOWEDf,
                            ((station->flags & BCM_L2_STATION_ARP_RARP) ? 1 : 0));
    }
#endif

#if defined(BCM_TRIDENT_SUPPORT)
    if (SOC_IS_TRIDENT(unit)
        || SOC_IS_TD2_TT2(unit)) {

        soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                            TRILL_TERMINATION_ALLOWEDf,
                            ((station->flags & BCM_L2_STATION_TRILL) ? 1 : 0));

        soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                            FCOE_TERMINATION_ALLOWEDf,
                            ((station->flags & BCM_L2_STATION_FCOE) ? 1 : 0));
    }
#endif

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) {

        if (soc_mem_field_valid(unit, tcam_mem, OAM_TERMINATION_ALLOWEDf)) { 
            soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                                OAM_TERMINATION_ALLOWEDf,
                                ((station->flags & BCM_L2_STATION_OAM) ? 1 :0));
        }

        soc_mem_field32_set(unit, tcam_mem, ent_p->tcam_ent,
                            COPY_TO_CPUf,
                            ((station->flags & BCM_L2_STATION_COPY_TO_CPU)
                            ? 1 : 0));
    }
#endif
    return BCM_E_NONE;
}

#if defined(BCM_KATANA2_SUPPORT)
STATIC int
_bcm_kt2_l2_port_mac_set(int unit, int index, bcm_mac_t port_mac) 
{
    port_tab_entry_t          ptab;        
    egr_port_entry_t          egr_port_entry;
    /* Set MAC address in Port table */
    SOC_IF_ERROR_RETURN(
                READ_PORT_TABm(unit, MEM_BLOCK_ANY, index, &ptab));

    soc_mem_mac_addr_set(unit, PORT_TABm, &ptab, MAC_ADDRESSf, port_mac);

    SOC_IF_ERROR_RETURN( WRITE_PORT_TABm(unit,MEM_BLOCK_ALL, index, &ptab));

    SOC_IF_ERROR_RETURN(READ_EGR_PORTm(unit, MEM_BLOCK_ANY, index,
                                           &egr_port_entry));

    soc_mem_mac_addr_set(unit, EGR_PORTm, &egr_port_entry, 
                                   MAC_ADDRESSf, port_mac);
    SOC_IF_ERROR_RETURN(
            WRITE_EGR_PORTm(unit, MEM_BLOCK_ALL, index, &egr_port_entry));

    return BCM_E_NONE; 
}

STATIC int
_kt_olp_check_olp_dglp(int unit, int dglp, bcm_mac_t *mac)
{
    _bcm_l2_station_control_t *sc;           /* Station control structure. */
    egr_olp_dgpp_config_entry_t eolp_dgpp_cfg;
    int        index = 0;
    int        max_entries = 0;
    soc_field_t field;

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    max_entries = soc_mem_index_count(unit, EGR_OLP_DGPP_CONFIGm);

    for (; index < max_entries; index++) {
        SOC_IF_ERROR_RETURN(
              READ_EGR_OLP_DGPP_CONFIGm(unit, MEM_BLOCK_ANY, 
                                           index, &eolp_dgpp_cfg));
        field = soc_mem_field32_get(unit, EGR_OLP_DGPP_CONFIGm, 
                                  &eolp_dgpp_cfg, DGLPf);
        if (field == dglp) {
            soc_mem_mac_addr_get(unit, EGR_OLP_DGPP_CONFIGm, &eolp_dgpp_cfg, 
                                                    MACDAf, *mac);
            return BCM_E_EXISTS;
        }
    } /* end of for */
    return BCM_E_NOT_FOUND;
}

STATIC int
_kt_olp_mac_addr_mask_get(int unit, uint32 *lowest_mac_addr, uint32 *mac_mask)
{
    _bcm_l2_station_control_t *sc;           /* Station control structure. */
    egr_olp_dgpp_config_entry_t eolp_dgpp_cfg;
    bcm_mac_t  olp_mac;
    int        max_entries = 0;
    uint32     mac_addr[2] = {0, 0};
    uint32     prev_mac_addr[2] = {0, 0};
    uint32     highest_mac_addr[2] = {0, 0};
    int        index = 0;
    int        first_entry = 1; 
    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    max_entries = soc_mem_index_count(unit, EGR_OLP_DGPP_CONFIGm);
    mac_mask[0] = 0;
    mac_mask[1] = 0;

    for (; index < max_entries; index++) {
        SOC_IF_ERROR_RETURN(
              READ_EGR_OLP_DGPP_CONFIGm(unit, MEM_BLOCK_ANY, 
                                           index, &eolp_dgpp_cfg));
        soc_mem_mac_addr_get(unit, EGR_OLP_DGPP_CONFIGm, &eolp_dgpp_cfg, 
                                                    MACDAf, olp_mac);
        SAL_MAC_ADDR_TO_UINT32(olp_mac, mac_addr);
        if ((mac_addr[0] == 0) && (mac_addr[1] == 0)) {
          /* skip the entry */ 
            continue;
        }
        if (first_entry) {
            mac_mask[1] = MAC_MASK_HIGH; 
            mac_mask[0] = MAC_MASK_LOW; 
            prev_mac_addr[0] = mac_addr[0];
            prev_mac_addr[1] = mac_addr[1];
            lowest_mac_addr[0] = mac_addr[0];
            lowest_mac_addr[1] = mac_addr[1];
            highest_mac_addr[0] = mac_addr[0];
            highest_mac_addr[1] = mac_addr[1];
            first_entry = 0;
        } else {
            if (mac_addr[0] > highest_mac_addr[0]) {
                highest_mac_addr[0] = mac_addr[0];
                highest_mac_addr[1] = mac_addr[1];
            } else if (mac_addr[0] < lowest_mac_addr[0]) {
                lowest_mac_addr[0] = mac_addr[0];
                lowest_mac_addr[1] = mac_addr[1];
            } else {
               /* do nothing */
            }
            mac_mask[0] &= (~(mac_addr[0] ^ prev_mac_addr[0]));
        }
    } /* end of for */
    if ((highest_mac_addr[0] - lowest_mac_addr[0]) > OLP_MAX_MAC_DIFF) {
        mac_mask[0] = MAC_MASK_LOW; 
        mac_mask[1] = MAC_MASK_HIGH; 
    }
    if ((lowest_mac_addr[0] == MAC_MASK_LOW) && 
        (lowest_mac_addr[1] == MAC_MASK_HIGH)) {
        /* no entry exists in the table */
        mac_mask[0] = MAC_MASK_LOW; 
        mac_mask[1] = MAC_MASK_HIGH; 
        lowest_mac_addr[0] = 0; 
        lowest_mac_addr[1] = 0; 
    }
    return BCM_E_NONE;
}

STATIC int
_bcm_kt2_l2_olp_mac_set(int unit, int index, bcm_port_t port, bcm_mac_t olp_mac, int action)
{
    egr_olp_dgpp_config_entry_t eolp_dgpp_cfg;
    iarb_ing_physical_port_entry_t entry;
    ing_physical_port_table_entry_t port_entry;
    ing_en_efilter_bitmap_entry_t efilter_entry;
    bcm_module_t module_id = 0;
    bcm_module_t my_modid;
    bcm_port_t port_id;
    bcm_trunk_t trunk_id = BCM_TRUNK_INVALID;
    int         local_id;
    int         dglp = 0;
    uint32      olp_enable = 1;
    pbmp_t      pbmp;
    bcm_l2_addr_t l2addr;
    bcm_vlan_t  vlan;
    uint32      rval;
    uint64      reg_val;
    uint32      mac_addr[2] = {0xFFFFFFFF, 0xFFFF};
    uint32      mac_mask[2] ={ 0, 0 };
    bcm_mac_t   zero_mac = { 0, 0, 0, 0, 0, 0};
    bcm_mac_t   old_mac = { 0, 0, 0, 0, 0, 0};
    _bcm_l2_station_control_t *sc;           /* Station control structure. */

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));
    COMPILER_64_ZERO(reg_val); 
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));

    /* Add/delete static MAC entry with OLP MAC and EGR_OLP_VLAN */
    SOC_IF_ERROR_RETURN(READ_EGR_OLP_VLANr(unit, &rval));
    vlan = soc_reg_field_get(unit, EGR_OLP_VLANr, rval, VLAN_TAGf);

    if (vlan == 0) {
        soc_cm_debug(DK_ERR,
             "_bcm_kt2_l2_olp_mac_set: OLP vlan tag is not configured. \
              So can't add static MAC entry for OLP mac\n");
        return BCM_E_PARAM;
    }

    /* Set MAC address and DGLP in EGR_OLP_DGLP_CONFIG */
    SOC_IF_ERROR_RETURN(
              READ_EGR_OLP_DGPP_CONFIGm(unit, MEM_BLOCK_ANY, 
                                           index, &eolp_dgpp_cfg));
    if (port == 0) {
        port = soc_mem_field32_get(unit, EGR_OLP_DGPP_CONFIGm, 
                                  &eolp_dgpp_cfg, DGLPf);
        module_id = (port & DGLP_MODULE_ID_MASK) >> DGLP_MODULE_ID_SHIFT_BITS;
        port_id = port & DGLP_PORT_NO_MASK;
        dglp = 0;
        olp_enable = 0;
        soc_mem_mac_addr_get(unit, EGR_OLP_DGPP_CONFIGm, &eolp_dgpp_cfg, 
                                                    MACDAf, olp_mac);
    } else {
        if (BCM_GPORT_IS_SET(port)) {
            SOC_IF_ERROR_RETURN(_bcm_esw_gport_resolve(unit, port, &module_id, 
                                               &port_id, &trunk_id, &local_id));
            if (BCM_GPORT_IS_TRUNK(port) && (trunk_id != BCM_TRUNK_INVALID)) {
                /* Set LAG ID indicator bit */
                dglp |= (1 << DGLP_LAG_ID_INDICATOR_SHIFT_BITS);   
            } else if ((BCM_GPORT_IS_SUBPORT_PORT(port)) &&
                  ((_BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(unit, port)) ||
                  (_BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, port)))) {
                soc_cm_debug(DK_ERR,
                     "_bcm_kt2_l2_olp_mac_set: OLP is not allowed on SUBPORT. \
                                                                         \n");
                return BCM_E_PARAM;
            } else {
                dglp |= ((module_id << DGLP_MODULE_ID_SHIFT_BITS) + port_id); 
            }
        } else {
            port_id = port;
            dglp |= ((my_modid << DGLP_MODULE_ID_SHIFT_BITS) + port); 
        }
        if (IS_CPU_PORT(unit, port_id)) {
            soc_cm_debug(DK_ERR,
               "_bcm_kt2_l2_olp_mac_set: OLP is not allowed on CPU port. \n");
                return BCM_E_PARAM;
        }
    }
    /* check if an entry already exists for this dglp */
    if (BCM_E_EXISTS == _kt_olp_check_olp_dglp(unit, dglp, &old_mac)) {
        if (action == OLP_ACTION_UPDATE) {
            /* delete the static l2 entry added */
            BCM_IF_ERROR_RETURN(bcm_esw_l2_addr_delete(unit, old_mac, vlan)); 
        } else if (action == OLP_ACTION_ADD) {
            soc_cm_debug(DK_ERR,
                 "_bcm_kt2_l2_olp_mac_set: OLP MAC is already configured. \
                 for this DGLP\n");
            return BCM_E_PARAM;
        } 
    }

    soc_mem_mac_addr_set(unit, EGR_OLP_DGPP_CONFIGm, &eolp_dgpp_cfg, 
                         MACDAf, (olp_enable ? olp_mac : zero_mac));

    soc_mem_field32_set(unit, EGR_OLP_DGPP_CONFIGm, &eolp_dgpp_cfg, DGLPf, dglp);
    SOC_IF_ERROR_RETURN(WRITE_EGR_OLP_DGPP_CONFIGm(unit, MEM_BLOCK_ALL, 
                                                   index, &eolp_dgpp_cfg));
    /* OLP and HG ports must have OLP_MAC_DA_PREFIX_CHECK_ENABLE set to 0
       and front panel ports must have this value=1 */
    SOC_IF_ERROR_RETURN(soc_mem_read(unit, ING_PHYSICAL_PORT_TABLEm, 
                                     MEM_BLOCK_ANY, port_id, &port_entry));
    if (olp_enable) {
        soc_ING_PHYSICAL_PORT_TABLEm_field32_set(unit, &port_entry, 
                                         OLP_MAC_DA_PREFIX_CHECK_ENABLEf, 0);
    } else {
        soc_ING_PHYSICAL_PORT_TABLEm_field32_set(unit, &port_entry, 
                                         OLP_MAC_DA_PREFIX_CHECK_ENABLEf, 1);

    }
    SOC_IF_ERROR_RETURN(soc_mem_write(unit, ING_PHYSICAL_PORT_TABLEm, 
                                      MEM_BLOCK_ALL, port_id, &port_entry));

    if ((module_id == 0) || (module_id== my_modid)) {
        /* Local port - Enable OLP on this port */
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, IARB_ING_PHYSICAL_PORTm, 
                                           MEM_BLOCK_ANY, port_id, &entry));
        soc_IARB_ING_PHYSICAL_PORTm_field32_set(unit, &entry, 
                                                    OLP_ENABLEf, olp_enable);
        SOC_IF_ERROR_RETURN(soc_mem_write(unit, IARB_ING_PHYSICAL_PORTm, 
                                              MEM_BLOCK_ALL, port_id, &entry));

        /* Disable EFILTER */
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, ING_EN_EFILTER_BITMAPm, 
                                           MEM_BLOCK_ANY, 0, &efilter_entry));
        soc_mem_pbmp_field_get(unit, ING_EN_EFILTER_BITMAPm,
                                           &efilter_entry, BITMAPf, &pbmp);
        if (olp_enable) {
            SOC_PBMP_PORT_REMOVE(pbmp, port_id);
        } else {
            SOC_PBMP_PORT_ADD(pbmp, port_id);
        }
        soc_mem_pbmp_field_set(unit, ING_EN_EFILTER_BITMAPm,
                                          &efilter_entry, BITMAPf, &pbmp);
        SOC_IF_ERROR_RETURN(soc_mem_write(unit, ING_EN_EFILTER_BITMAPm, 
                                              MEM_BLOCK_ALL, 0, &efilter_entry));
    }

    /* Set l2 address info. */
    bcm_l2_addr_t_init(&l2addr, olp_mac, vlan);
        
    if (olp_enable) {
         /* Set l2 entry flags. */
        l2addr.flags = BCM_L2_STATIC;
        /* Set l2 entry port dglp*/
        l2addr.port = port;
        bcm_esw_l2_addr_add(unit, &l2addr); 
    } else {
        BCM_IF_ERROR_RETURN(bcm_esw_l2_addr_delete(unit, olp_mac, vlan)); 
    }
    /* Set mask and data values in ING_OLP_CONFIG_ register */
    BCM_IF_ERROR_RETURN(_kt_olp_mac_addr_mask_get(unit, mac_addr, mac_mask));
    COMPILER_64_SET(reg_val, mac_mask[1], mac_mask[0]);
    SOC_IF_ERROR_RETURN(WRITE_ING_OLP_CONFIG_1_64r(unit, reg_val));

    COMPILER_64_SET(reg_val, mac_addr[1], mac_addr[0]);
    SOC_IF_ERROR_RETURN(WRITE_ING_OLP_CONFIG_0_64r(unit, reg_val));

    return BCM_E_NONE; 
}


STATIC int
_bcm_kt2_l2_xgs_mac_set(int unit, int index, bcm_mac_t xgs_mac) 
{
    egr_olp_config_entry_t    eolp_cfg;
    uint64                    iarb_olp_cfg;
    uint64                    mac_field;

    /* Set MAC address in IARB_OLP_CONFIG */
    SAL_MAC_ADDR_TO_UINT64(xgs_mac, mac_field);

    COMPILER_64_ZERO(iarb_olp_cfg); 

    SOC_IF_ERROR_RETURN(READ_IARB_OLP_CONFIGr(unit, &iarb_olp_cfg));

    soc_reg64_field_set(unit, IARB_OLP_CONFIGr, &iarb_olp_cfg, MACDAf, mac_field);

    SOC_IF_ERROR_RETURN(WRITE_IARB_OLP_CONFIGr(unit, iarb_olp_cfg));
  
    /* Set MAC address in EGR_OLP_CONFIG */

    SOC_IF_ERROR_RETURN(
              READ_EGR_OLP_CONFIGm(unit, MEM_BLOCK_ANY, index, &eolp_cfg));

    soc_mem_mac_addr_set(unit, EGR_OLP_CONFIGm, &eolp_cfg, 
                                   MACSAf, xgs_mac);
    SOC_IF_ERROR_RETURN(
                WRITE_EGR_OLP_CONFIGm(unit, MEM_BLOCK_ALL, 
                                           index, &eolp_cfg));
    return BCM_E_NONE; 
}

/*
 * Function:
 *     _bcm_kt2_l2_station_entry_create
 * Purpose:
 *     Allocate an entry buffer and setup entry parameters for hardware write
 *     operation.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     sid        - (IN) Station ID.
 *     station    - (IN) Station parameters to be set in hardware.
 *     ent_p      - (OUT) Pointer to Station entry type data.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_kt2_l2_station_entry_create(int unit,
                             int sid,
                             bcm_l2_station_t *station,
                             _bcm_l2_station_entry_t **ent_pp)
{
    _bcm_l2_station_control_t *sc;           /* Station control structure. */
    int                       index;         /* Entry index.               */
    _bcm_l2_station_entry_t   *ent_p = NULL; /* L2 station entry pointer.  */
    soc_mem_t                 tcam_mem;      /* TCAM memory name.          */ 
    int                       mem_size = 0;  /* Entry buffer size.         */
    int                       rv;            /* Operation return status.   */
    int                       index_max;     /* Max Entry index.           */
    bcm_module_t       module_id;            /* Module ID           */
    bcm_port_t         port_id;              /* Port ID.            */
    bcm_trunk_t        trunk_id;             /* Trunk ID.           */
    int                local_id;             /* Hardware ID.        */
    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    if (station->flags & BCM_L2_STATION_OAM) {
        if (0 == sc->port_entries_free) {
            return (BCM_E_RESOURCE);
        }
    } else if (station->flags & BCM_L2_STATION_OLP) { 
        if (0 == sc->olp_entries_free) {
            return (BCM_E_RESOURCE);
        }
    } else if (station->flags & BCM_L2_STATION_XGS_MAC) { 
        /* do nothing */
    } else if (0 == sc->entries_free)  {
        return (BCM_E_RESOURCE);
    }

    ent_p = (_bcm_l2_station_entry_t *)
            sal_alloc(sizeof(_bcm_l2_station_entry_t), "Sw L2 station entry");
    if (NULL == ent_p) {
        return (BCM_E_MEMORY);
    }

    sal_memset(ent_p, 0, sizeof(_bcm_l2_station_entry_t));


    ent_p->sid = sid;

    ent_p->prio = station->priority;

    if (station->flags & BCM_L2_STATION_OAM) {
        /* Get index to PORT table from source port */ 
        if (BCM_GPORT_IS_SET(station->src_port)) {
            rv = _bcm_esw_gport_resolve(unit, station->src_port,  &module_id,
                                    &port_id, &trunk_id, &local_id);
            if (BCM_FAILURE(rv)) {
                sal_free(ent_p);
                return (rv);
            }
            rv = _bcm_kt2_modport_to_pp_port_get(unit, module_id, port_id,
                                                &index);
            if (BCM_FAILURE(rv)) {
                sal_free(ent_p);
                return (rv);
            }
        } else {
            index = station->src_port;
        }
        if (NULL == sc->entry_arr[index + sc->entries_total]) {
            ent_p->hw_index = index;
        } else if (!(station->flags & BCM_L2_STATION_REPLACE)) {
            sal_free(ent_p);
            return BCM_E_PARAM;
        }
        ent_p->flags |= _BCM_L2_STATION_ENTRY_TYPE_NON_TCAM;
        /* Set MAC address in Port table */
        rv = _bcm_kt2_l2_port_mac_set(unit, index, station->dst_mac);
        if (BCM_FAILURE(rv)) {
            sal_free(ent_p);
            return (rv);
        }
        index += sc->entries_total;
        sc->port_entries_free--;
    } else if (station->flags & BCM_L2_STATION_OLP) {
        /* Get the first free slot. */
        index = sc->entries_total + sc->port_entries_total;
        index_max = index + sc->olp_entries_total;
        for (; index < index_max; index++) {
            if (NULL == sc->entry_arr[index]) {
                ent_p->hw_index = index - 
                                 (sc->entries_total + sc->port_entries_total);
                break;
            }
        }
        ent_p->flags |= _BCM_L2_STATION_ENTRY_TYPE_NON_TCAM;
        sc->olp_entries_free--;
        /* Set MAC address and DGLP in EGR_OLP_DGLP_CONFIG */
        rv = _bcm_kt2_l2_olp_mac_set(unit, ent_p->hw_index, station->src_port,
                                     station->dst_mac, OLP_ACTION_ADD);
        if (BCM_FAILURE(rv)) {
            sal_free(ent_p);
            sc->olp_entries_free++;
            return (rv);
        }
    } else if (station->flags & BCM_L2_STATION_XGS_MAC) {
        index = sc->entries_total + sc->port_entries_total + 
                                    sc->olp_entries_total;
        ent_p->hw_index = 0;
        ent_p->flags |= _BCM_L2_STATION_ENTRY_TYPE_NON_TCAM;
        /* Set MAC address in IARB_OLP_CONFIG and EGR_OLP_CONFIG */
        rv = _bcm_kt2_l2_xgs_mac_set(unit, 0, station->dst_mac);
        if (BCM_FAILURE(rv)) {
            sal_free(ent_p);
            return (rv);
        }
    } else {
        /* Get the first free slot. */
        for (index = 0; index < sc->entries_total; index++) {
            if (NULL == sc->entry_arr[index]) {
                ent_p->hw_index = index;
                break;
            }
        }

        rv = _bcm_l2_station_tcam_mem_get(unit, &tcam_mem);
        if (BCM_FAILURE(rv)) {
            sal_free(ent_p);
            return (rv);
        }

        mem_size = sizeof(my_station_tcam_entry_t);

        ent_p->tcam_ent = sal_alloc(mem_size, "L2 station entry buffer");
        if (NULL == ent_p->tcam_ent) {
            sal_free(ent_p);
            return (BCM_E_MEMORY);
        }

        sal_memset(ent_p->tcam_ent, 0, mem_size);

        rv = _bcm_l2_station_entry_set(unit, tcam_mem, station, ent_p);
        if (BCM_FAILURE(rv)) {
            sal_free(ent_p);
            return (rv);
        }

        sc->entries_free--;
        sc->entry_count++;
    }
    sc->entry_arr[index] = ent_p;
    *ent_pp = ent_p;

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_kt2_l2_station_entry_update
 * Purpose:
 *     Allocate an entry buffer and setup entry parameters for hardware write
 *     operation.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     sid        - (IN) Station ID.
 *     station    - (IN) Station parameters to be set in hardware.
 *     ent_p      - (OUT) Pointer to Station entry type data.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_kt2_l2_station_entry_update(int unit,
                             int sid,
                             bcm_l2_station_t *station,
                             _bcm_l2_station_entry_t *ent_p)
{
    soc_mem_t   tcam_mem;    /* TCAM memory name.         */
    int         rv;           /* Operation return status. */
    int         mem_size = 0; /* Entry buffer size.       */

    if (station->flags & BCM_L2_STATION_OAM) {
        rv = _bcm_kt2_l2_port_mac_set(unit, ent_p->hw_index, station->dst_mac);
        return rv;
    } else if (station->flags & BCM_L2_STATION_OLP) {
        rv = _bcm_kt2_l2_olp_mac_set(unit, ent_p->hw_index, station->src_port, 
                                     station->dst_mac, OLP_ACTION_UPDATE);
        return rv;
    } else if (station->flags & BCM_L2_STATION_XGS_MAC) {
        rv = _bcm_kt2_l2_xgs_mac_set(unit, 0, station->dst_mac);
        return rv;
    } else {
        rv = _bcm_l2_station_tcam_mem_get(unit, &tcam_mem);
        if (BCM_FAILURE(rv)) {
            return (rv);
        }

        mem_size = sizeof(my_station_tcam_entry_t);

        ent_p->tcam_ent = sal_alloc(mem_size, "L2 station entry buffer");
        if (NULL == ent_p->tcam_ent) {
            return (BCM_E_MEMORY);
        }

        sal_memset(ent_p->tcam_ent, 0, mem_size);
    
        BCM_IF_ERROR_RETURN(_bcm_l2_station_entry_set(unit, tcam_mem, 
                               station, ent_p));
    }
    return (BCM_E_NONE);
}

STATIC int
_bcm_kt2_l2_station_entry_delete(int unit, 
                                 _bcm_l2_station_control_t *sc, 
                                 _bcm_l2_station_entry_t *s_ent,
                                 int station_id) 
{
    int                         rv = BCM_E_NONE;  /* Operation return status. */
    bcm_mac_t                   mac_addr = {0,0,0,0,0,0};

    if ((s_ent->sid >= _BCM_L2_STATION_ID_KT2_PORT_MAC_MIN) &&
              (s_ent->sid < _BCM_L2_STATION_ID_KT2_OLP_MAC_MIN)) {
        rv = _bcm_kt2_l2_port_mac_set(unit, s_ent->hw_index, mac_addr);
        sc->port_entries_free++;
        /* Remove the entry pointer from the list. */
        sc->entry_arr[s_ent->hw_index + sc->entries_total] = NULL;
    } else if ((s_ent->sid >= _BCM_L2_STATION_ID_KT2_OLP_MAC_MIN) &&
              (s_ent->sid < _BCM_L2_STATION_ID_KT2_XGS_MAC_MIN)) {
        rv = _bcm_kt2_l2_olp_mac_set(unit, s_ent->hw_index, 0, 
                                     mac_addr, OLP_ACTION_DELETE);
        sc->entry_arr[s_ent->hw_index + 
                      sc->entries_total + sc->port_entries_total] = NULL;
        sc->olp_entries_free++;
    } else if (s_ent->sid == _BCM_L2_STATION_ID_KT2_XGS_MAC_MIN) { 
        rv = _bcm_kt2_l2_xgs_mac_set(unit, 0, mac_addr);
        sc->entry_arr[s_ent->hw_index + sc->entries_total + 
                      sc->port_entries_total + sc->olp_entries_total] = NULL;
    }
    return (rv);
}


STATIC int
_bcm_kt2_l2_station_entry_get(int unit, _bcm_l2_station_control_t *sc, 
                              _bcm_l2_station_entry_t *s_ent,
                              int station_id, bcm_l2_station_t *station) 
{
    int         rv = BCM_E_NONE;  /* Operation return status. */
    port_tab_entry_t          ptab;        
    egr_olp_config_entry_t    eolp_cfg;
    egr_olp_dgpp_config_entry_t eolp_dgpp_cfg;

    if ((s_ent->sid >= _BCM_L2_STATION_ID_KT2_PORT_MAC_MIN) &&
              (s_ent->sid < _BCM_L2_STATION_ID_KT2_OLP_MAC_MIN)) {

        SOC_IF_ERROR_RETURN(
                READ_PORT_TABm(unit, MEM_BLOCK_ANY, s_ent->hw_index, &ptab));

        station->src_port = s_ent->hw_index;

        soc_mem_mac_addr_get(unit, PORT_TABm, &ptab, 
                             MAC_ADDRESSf, station->dst_mac);

        station->flags = BCM_L2_STATION_OAM; 
    } else if ((s_ent->sid >= _BCM_L2_STATION_ID_KT2_OLP_MAC_MIN) &&
              (s_ent->sid < _BCM_L2_STATION_ID_KT2_XGS_MAC_MIN)) {

        SOC_IF_ERROR_RETURN(
              READ_EGR_OLP_DGPP_CONFIGm(unit, MEM_BLOCK_ANY, 
                                         s_ent->hw_index, &eolp_dgpp_cfg));

        station->src_port = soc_mem_field32_get(unit, EGR_OLP_DGPP_CONFIGm, 
                                  &eolp_dgpp_cfg, DGLPf);

        soc_mem_mac_addr_get(unit, EGR_OLP_DGPP_CONFIGm, &eolp_dgpp_cfg, 
                             MACDAf, station->dst_mac);
        station->flags = BCM_L2_STATION_OLP; 

    } else if (s_ent->sid == _BCM_L2_STATION_ID_KT2_XGS_MAC_MIN) { 
        SOC_IF_ERROR_RETURN(
              READ_EGR_OLP_CONFIGm(unit, MEM_BLOCK_ANY, 0, &eolp_cfg));

         soc_mem_mac_addr_get(unit, EGR_OLP_CONFIGm, &eolp_cfg, 
                                            MACSAf, station->dst_mac);
        station->flags = BCM_L2_STATION_XGS_MAC; 
    }
    return rv;
}

#endif

/*
 * Function:
 *     _bcm_l2_station_entry_update
 * Purpose:
 *     Allocate an entry buffer and setup entry parameters for hardware write
 *     operation.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     sid        - (IN) Station ID.
 *     station    - (IN) Station parameters to be set in hardware.
 *     ent_p      - (OUT) Pointer to Station entry type data.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_entry_update(int unit,
                             int sid,
                             bcm_l2_station_t *station,
                             _bcm_l2_station_entry_t *ent_p)
{
    soc_mem_t   tcam_mem;    /* TCAM memory name.         */
    int         rv;           /* Operation return status. */
    int         mem_size = 0; /* Entry buffer size.       */

    /* Input parameter check. */
    if (NULL == ent_p || NULL == station) {
        return (BCM_E_PARAM);
    }

    L2_VVERB(("L2(unit %d) Info: (SID=%d) (idx=%d) (prio: o=%d n=%d) "
             "- update.\n", unit, sid, ent_p->hw_index, ent_p->prio,
             station->priority));

    if (ent_p->prio == station->priority) {
        ent_p->flags |= _BCM_L2_STATION_ENTRY_PRIO_NO_CHANGE;
    } else {
        ent_p->prio = station->priority;
    }

#if defined(BCM_KATANA2_SUPPORT) 
    if (SOC_IS_KATANA2(unit)) {
        rv = _bcm_kt2_l2_station_entry_update(unit, sid, station, ent_p);
        return (rv);
    }
#endif /* BCM_KATANA2_SUPPORT */

    rv = _bcm_l2_station_tcam_mem_get(unit, &tcam_mem);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    if (SOC_IS_ENDURO(unit) || SOC_IS_TRIUMPH(unit)
        || SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit)
        || SOC_IS_VALKYRIE(unit) || SOC_IS_VALKYRIE2(unit)
        || SOC_IS_HURRICANEX(unit)) {

        mem_size = sizeof(mpls_station_tcam_entry_t);

    } else if (SOC_IS_KATANAX(unit) || SOC_IS_TRIDENT(unit)
               || SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit)) {

        mem_size = sizeof(my_station_tcam_entry_t);

    }

    if (0 == mem_size) {
        /* Must be a valid mem_size value. */
        return (BCM_E_INTERNAL);
    }

    ent_p->tcam_ent = sal_alloc(mem_size, "L2 station entry buffer");
    if (NULL == ent_p->tcam_ent) {
        return (BCM_E_MEMORY);
    }

    sal_memset(ent_p->tcam_ent, 0, mem_size);
    
    BCM_IF_ERROR_RETURN(_bcm_l2_station_entry_set(unit, tcam_mem, 
                           station, ent_p));

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_l2_station_entry_create
 * Purpose:
 *     Allocate an entry buffer and setup entry parameters for hardware write
 *     operation.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     sid        - (IN) Station ID.
 *     station    - (IN) Station parameters to be set in hardware.
 *     ent_p      - (OUT) Pointer to Station entry type data.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_entry_create(int unit,
                             int sid,
                             bcm_l2_station_t *station,
                             _bcm_l2_station_entry_t **ent_pp)
{
    _bcm_l2_station_control_t *sc;           /* Station control structure. */
    int                       index;         /* Entry index.               */
    _bcm_l2_station_entry_t   *ent_p = NULL; /* L2 station entry pointer.  */
    soc_mem_t                 tcam_mem;      /* TCAM memory name.          */ 
    int                       mem_size = 0;  /* Entry buffer size.         */
    int                       rv;            /* Operation return status.   */

    /* Input parameter check. */
    if (NULL == station || NULL == ent_pp) {
        return (BCM_E_PARAM);
    }
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        rv = _bcm_kt2_l2_station_entry_create(unit, sid, station, ent_pp);
        return rv;
    } 
#endif    
    
    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    if (0 == sc->entries_free) {
        return (BCM_E_RESOURCE);
    }

    ent_p = (_bcm_l2_station_entry_t *)
            sal_alloc(sizeof(_bcm_l2_station_entry_t), "Sw L2 station entry");
    if (NULL == ent_p) {
        return (BCM_E_MEMORY);
    }

    sal_memset(ent_p, 0, sizeof(_bcm_l2_station_entry_t));

    /* Get the first free slot. */
    for (index = 0; index < sc->entries_total; index++) {
        if (NULL == sc->entry_arr[index]) {
            ent_p->hw_index = index;
            break;
        }
    }

    ent_p->sid = sid;

    ent_p->prio = station->priority;

    rv = _bcm_l2_station_tcam_mem_get(unit, &tcam_mem);
    if (BCM_FAILURE(rv)) {
        sal_free(ent_p);
        return (rv);
    }

    if (SOC_IS_ENDURO(unit) || SOC_IS_TRIUMPH(unit)
        || SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit)
        || SOC_IS_VALKYRIE(unit) || SOC_IS_VALKYRIE2(unit)
        || SOC_IS_HURRICANEX(unit)) {

        mem_size = sizeof(mpls_station_tcam_entry_t);

    } else if (SOC_IS_KATANAX(unit) || SOC_IS_TRIDENT(unit)
               || SOC_IS_TD2_TT2(unit) || SOC_IS_TRIUMPH3(unit)) {
        mem_size = sizeof(my_station_tcam_entry_t);
    }

    if (0 == mem_size) {
        /* mem_size must be greater than zero. */
        sal_free(ent_p);
        return (BCM_E_INTERNAL);
    }

    ent_p->tcam_ent = sal_alloc(mem_size, "L2 station entry buffer");
    if (NULL == ent_p->tcam_ent) {
        sal_free(ent_p);
        return (BCM_E_MEMORY);
    }

    sal_memset(ent_p->tcam_ent, 0, mem_size);

    rv = _bcm_l2_station_entry_set(unit, tcam_mem, station, ent_p);
    if (BCM_FAILURE(rv)) {
        sal_free(ent_p);
        return (rv);
    }

    /* Decrement free entries count. */
    sc->entries_free--;

    sc->entry_arr[index] = ent_p;

    sc->entry_count++;

    *ent_pp = ent_p;

    return (BCM_E_NONE);

}

/*
 * Function:
 *     _bcm_l2_station_entry_prio_cmp
 * Purpose:
 *     Compare station entry priority values.
 * Parameters:
 *     prio_first   - (IN) First entry priority value.
 *     prio_second  - (IN) Second entry priority value.
 * Returns:
 *     0    - if both priority values are equal.
 *     -1   - if priority value of first entry is lower than second.
 *     1    - if priority value of first entry is greater than second.
 */
STATIC int
_bcm_l2_station_entry_prio_cmp(int prio_first, int prio_second)
{
    int rv; /* Comparison result. */

    if (prio_first == prio_second) {
        rv = 0;
    } else if (prio_first < prio_second) {
        rv = -1;
    } else {
        rv = 1;
    }
    return (rv);
}

/*
 * Function:
 *     _bcm_l2_station_prio_move_required
 * Purpose:
 *     Check if entry move is required based on priority value.
 * Parameters:
 *     unit   - (IN) BCM device number
 *     ent_p  - (IN) Station entry for priority comparison.
 * Returns:
 *     TRUE   - Entry move is required based on priority comparison.
 *     FALSE  - Entry move is not required based on priority comparison.
 */
STATIC int
_bcm_l2_station_prio_move_required(int unit, _bcm_l2_station_entry_t *ent_p)
{
    _bcm_l2_station_control_t *sc;      /* station control structure.   */
    uint32                    count;    /* Maximum number of entries.   */
    int                       i;        /* Temporary iterator variable. */
    int                       ent_prio; /* Entry priority.              */
    int                       flag;     /* Flag value denotes if we are */
                                        /* before or after s_ent.       */

    /* Input parameter check. */
    if (NULL == ent_p) {
        return (BCM_E_INTERNAL);
    }

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    /* Get total number for entries supported. */
    count = sc->entries_total;

    /* Get the entry priority value. */
    ent_prio = ent_p->prio;

    /* Init flag value, we are before s_ent. */
    flag = -1;

    for (i = 0; i < count; i++) {

        if (sc->entry_arr[i] == ent_p) {
            /* Now we are after s_ent. */
            flag = 1;
            continue;
        }

        if (NULL == sc->entry_arr[i]) {
            continue;
        }

        if (-1 == flag) {
            if (_bcm_l2_station_entry_prio_cmp
                    (sc->entry_arr[i]->prio, ent_prio) < 0) {

                L2_VVERB(("L2(unit %d) Info: (SID=%d) found lower prio than "
                         "(prio=%d).\n", unit, ent_p->sid, ent_p->prio));

                /*
                 * An entry  before s_ent has lower priority.
                 * So, move is required.
                 */
                return (TRUE);
            }
        } else {
            if (_bcm_l2_station_entry_prio_cmp
                    (sc->entry_arr[i]->prio, ent_prio) > 0) {
                L2_VVERB(("L2(unit %d) Info: (SID=%d) found higher prio than "
                         "(prio=%d).\n", unit, ent_p->sid, ent_p->prio));
                /*
                 * An entry after s_ent has higher priority than s_ent prio.
                 * So, move is required.
                 */
                return (TRUE);
            }
        }
    }

    L2_VVERB(("L2(unit %d) Info: (SID=%d) (prio=%d) no move.\n",
             unit, ent_p->sid, ent_p->prio));
    return (FALSE);
}

/*
 * Function:
 *     _bcm_l2_station_entry_move_check
 * Purpose:
 *     Calculate number of entries that need to be moved for installing the
 *     new station entry.
 * Parameters:
 *     unit                     - (IN) BCM device number
 *     null_index               - (IN) Free null entry hardware index.
 *     target_index             - (IN) Target hardware index for the entry.
 *     direction                - (IN) Direction of Move.
 *     shifted_entries_count    - (OUT) No. of entries to be moved.
 * Returns:
 *     BCM_E_NONE
 */
STATIC int
_bcm_l2_station_entry_move_check(int unit, int null_index,
                                 int target_index, int direction,
                                 int *shifted_entries_count)
{
    *shifted_entries_count += direction * (null_index - target_index);

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_l2_station_entry_dir_get
 * Purpose:
 *     Find out direction in which entries need to be moved for installing
 *     new station entry in hardware.
 * Parameters:
 *     unit             - (IN) BCM device number
 *     ent_p            - (IN) Station entry pointer.
 *     prev_null_index  - (IN) Location of previous null entry in hardware.
 *     next_null_index  - (IN) Location of next null entry in hardware.
 *     target_index     - (IN) Target index where entry needs to be installed.
 *     dir              - (OUT) Direction in which entries need to be moved.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_entry_dir_get(int unit,
                              _bcm_l2_station_entry_t *ent_p,
                              int prev_null_index,
                              int next_null_index,
                              int target_index,
                              int *dir)
{
    _bcm_l2_station_control_t  *sc;                   /* Station control    */
                                                      /* strucutre.         */
    int                        shift_up = FALSE;      /* Shift UP status.   */
    int                        shift_down = FALSE;    /* Shift DOWN status. */
    int                        shift_up_amount = 0;   /* Shift UP amount.   */
    int                        shift_down_amount = 0; /* Shift DOWN amount. */
    int                        rv;                    /* Operation return   */
                                                      /* status.            */

    /* Input parameter check. */
    if (NULL == ent_p || NULL == dir) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    if (-1 != prev_null_index) {
        rv = _bcm_l2_station_entry_move_check(unit, prev_null_index,
                                              target_index, -1,
                                              &shift_up_amount);
        shift_up = BCM_SUCCESS(rv);
    } else {
        shift_up = FALSE;
    }

    if (-1 != next_null_index) {
        rv = _bcm_l2_station_entry_move_check(unit, next_null_index,
                                              target_index, 1,
                                              &shift_down_amount);
        shift_down = BCM_SUCCESS(rv);
    } else {
        shift_down = FALSE;
    }

    if (TRUE == shift_up) {
        if (TRUE == shift_down) {
            if (shift_up_amount < shift_down_amount) {
                *dir = -1;
            } else {
                *dir = 1;
            }
        } else {
            *dir = -1;
        }
    } else {
        if (TRUE == shift_down) {
            *dir = 1;
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

/*
 * Function:
 *     _bcm_l2_station_entry_move
 * Purpose:
 *     Move a station entry by a specified amount.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     s_ent    - (IN) Station entry pointer.
 *     amount   - (IN) Direction in which entries need to be moved.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_entry_move(int unit,
                           _bcm_l2_station_entry_t *s_ent,
                           int amount)
{
    _bcm_l2_station_control_t   *sc;          /* Station control structure. */
    int                         tcam_idx_old; /* Original entry TCAM index. */
    int                         tcam_idx_new; /* Entry new TCAM index.      */
    int                         rv;           /* Operation return status.   */
    soc_mem_t                   tcam_mem;     /* TCAM memory name.          */
    uint32                      entry[SOC_MAX_MEM_FIELD_WORDS]; /* Entry    */
                                                                /* buffer.  */
    int                         count;
    /* Input parameter check. */
    if (NULL == s_ent) {
        return (BCM_E_PARAM);
    }

    /* Input parameter check. */
    if (0 == amount) {
        return (BCM_E_NONE);
    }

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    tcam_idx_old =  s_ent->hw_index;

    tcam_idx_new = tcam_idx_old + (amount);

    L2_VVERB(("L2(unit %d) Info: (SID=%d) move (oidx=%d nidx=%d) (amt=%d).\n",
             unit, s_ent->sid, s_ent->hw_index, tcam_idx_new, amount));

    count = sc->entries_total;

    if (tcam_idx_old < 0 || tcam_idx_old >= count) {
        return (BCM_E_PARAM);
    }

    rv = _bcm_l2_station_tcam_mem_get(unit, &tcam_mem);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    if (s_ent->flags & _BCM_L2_STATION_ENTRY_INSTALLED) {
        /* Clear entry buffer. */
        sal_memset(entry, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));

        rv = soc_mem_read(unit, tcam_mem, MEM_BLOCK_ANY, tcam_idx_old, entry);
        BCM_IF_ERROR_RETURN(rv);

        rv = soc_mem_write(unit, tcam_mem, MEM_BLOCK_ALL, tcam_idx_new, entry);
        BCM_IF_ERROR_RETURN(rv);

        rv = soc_mem_write(unit, tcam_mem, MEM_BLOCK_ALL, tcam_idx_old,
                           soc_mem_entry_null(unit, tcam_mem));
        BCM_IF_ERROR_RETURN(rv);
    }

    if (FALSE == prio_with_no_free_entries) {
        sc->entry_arr[s_ent->hw_index] = NULL;
    }

    sc->entry_arr[tcam_idx_new] = s_ent;

    s_ent->hw_index = tcam_idx_new;

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_l2_station_entry_shift_down
 * Purpose:
 *     Shift entries down by one index to create a free entry at target
 *     index location.
 * Parameters:
 *     unit             - (IN) BCM device number
 *     target_index     - (IN) Target index for entry install operation.
 *     next_null_index  - (IN) Location of free entry after target index.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_entry_shift_down(int unit,
                                 int target_index,
                                 int next_null_index)
{
    uint16                      empty_idx;   /* Empty index.               */
    _bcm_l2_station_control_t   *sc;         /* Station control structure. */
    int                         tmp_idx1;    /* Temporary index.           */
    int                         tmp_idx2;    /* Temporary index.           */ 
    int                         max_entries; /* Maximum entries supported. */
    int                         rv;          /* Operation return status.   */

    L2_VVERB(("L2(unit %d) Info: Shift UP (tidx=%d null-idx=%d).\n",
             unit, target_index, next_null_index));

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    empty_idx = next_null_index;

    max_entries = sc->entries_total;

    /*
     * Move entries one step down
     *     starting from the last entry
     * IDEA: 
     *     Move till empty_idx > target_index  
     */
    while (empty_idx > target_index) {
        if (empty_idx == 0) {
            tmp_idx1 = 0;
            tmp_idx2 = max_entries - 1;
            rv = _bcm_l2_station_entry_move(unit,
                                            sc->entry_arr[max_entries - 1],
                                            (tmp_idx1 - tmp_idx2));
            BCM_IF_ERROR_RETURN(rv);
        } else {
            tmp_idx1 = empty_idx;
            tmp_idx2 = (empty_idx - 1);
            rv = _bcm_l2_station_entry_move(unit,
                                            sc->entry_arr[empty_idx - 1],
                                            (tmp_idx1 - tmp_idx2));
            BCM_IF_ERROR_RETURN(rv);

            empty_idx--;
        }
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_l2_station_entry_shift_up
 * Purpose:
 *     Shift entries up by one index to create a free entry at target
 *     index location.
 * Parameters:
 *     unit             - (IN) BCM device number
 *     target_index     - (IN) Target index for entry install operation.
 *     prev_null_index  - (IN) Location of free entry before target index.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_entry_shift_up(int unit,
                               int target_index,
                               int prev_null_index)
{
    uint16                      empty_idx;   /* Empty index.               */
    _bcm_l2_station_control_t   *sc;         /* Station control structure. */
    int                         tmp_idx1;    /* Temporary index.           */
    int                         tmp_idx2;    /* Temporary index.           */ 
    int                         max_entries; /* Maximum entries supported. */
    int                         rv;          /* Operation return status.   */

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    L2_VVERB(("L2(unit %d) Info: Shift UP (tidx=%d null-idx=%d).\n",
             unit, target_index, prev_null_index));

    empty_idx = prev_null_index;
    max_entries = sc->entries_total;

    /*
     * Move entries one step up
     *     starting from the first entry
     * IDEA: 
     *     Move till empty_idx < target_index  
     */
    while (empty_idx < target_index) {
        if (empty_idx == (max_entries - 1)) {
            tmp_idx1 = max_entries - 1;
            tmp_idx2 = 0;
            rv = _bcm_l2_station_entry_move(unit,
                                            sc->entry_arr[0],
                                            (tmp_idx1 - tmp_idx2));
            BCM_IF_ERROR_RETURN(rv);
            empty_idx = 0;
        } else {
            tmp_idx1 = empty_idx;
            tmp_idx2 = (empty_idx + 1);
            rv = _bcm_l2_station_entry_move(unit,
                                            sc->entry_arr[empty_idx + 1],
                                            (tmp_idx1 - tmp_idx2));
            BCM_IF_ERROR_RETURN(rv);

            empty_idx++;
        }
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_l2_station_entry_install
 * Purpose:
 *     Write a station entry to hardware
 * Parameters:
 *     unit     - (IN) BCM device number
 *     ent_p    - (IN) Pointer to station entry.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_entry_install(int unit, _bcm_l2_station_entry_t *ent_p)
{
    int     rv;         /* Operation return status. */ 
    soc_mem_t tcam_mem; /* TCAM memory name.        */

    if (NULL == ent_p) {
        return (BCM_E_PARAM);
    }

    rv = _bcm_l2_station_tcam_mem_get(unit, &tcam_mem);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    if (NULL == ent_p->tcam_ent) {
        return (BCM_E_INTERNAL);
    }

    L2_VVERB(("L2(unit %d) Info: (SID=%d) - install (idx=%d).\n",
             unit, ent_p->sid, ent_p->hw_index));

    rv = soc_mem_write(unit, tcam_mem, MEM_BLOCK_ALL, ent_p->hw_index,
                       ent_p->tcam_ent);

    sal_free(ent_p->tcam_ent);

    ent_p->tcam_ent = NULL;

    return (rv);
}

/*
 * Function:
 *     _bcm_l2_station_entry_prio_install
 * Purpose:
 *     Install an entry based on priority value.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     ent_p    - (IN) Pointer to station entry.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_entry_prio_install(int unit, _bcm_l2_station_entry_t *ent_p)
{
    _bcm_l2_station_control_t  *sc;                /* Control structure.     */
    int                        idx_old;            /* Entry old TCAM index.  */
    int                        idx_target;         /* Entry target index.    */
    int                        prev_null_idx = -1; /* Prev NULL entry index. */
    int                        next_null_idx = -1; /* Next NULL entry index. */
    int                        temp;               /* Temporary variable.    */
    int                        dir = -1;           /* -1 = UP, 1 = DOWN      */
    int                        decr_on_shift_up = TRUE; /* Shift up status   */
    int                        rv;                 /* Return status.         */
    int                        flag_no_free_entries = FALSE; /* No free      */
                                                   /* entries status         */
    int                        max_entries; 
    prio_with_no_free_entries = FALSE;

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));
#if defined(BCM_KATANA2_SUPPORT)
    if ((SOC_IS_KATANA2(unit)) && 
        (ent_p->flags & _BCM_L2_STATION_ENTRY_TYPE_NON_TCAM)) {
        return BCM_E_NONE;  
    }
#endif

    if ((ent_p->flags & _BCM_L2_STATION_ENTRY_PRIO_NO_CHANGE)
        || (FALSE == _bcm_l2_station_prio_move_required(unit, ent_p))) {
        goto end;
    }

    idx_old = ent_p->hw_index;

    max_entries = sc->entries_total; 

    if (idx_old >= max_entries) {
        return (BCM_E_INTERNAL);
    }

    if (0 == sc->entries_free) {
        if (0 == (ent_p->flags & _BCM_L2_STATION_ENTRY_INSTALLED)) {
            sc->entry_arr[ent_p->hw_index] = NULL;
            flag_no_free_entries = TRUE;
        } else {
            return (BCM_E_CONFIG);
        }
    }

    for (idx_target = 0; idx_target < max_entries; idx_target++) {

        /* Skip the entry itself. */
        if (ent_p == sc->entry_arr[idx_target]) {
            continue;
        }

        if (NULL == sc->entry_arr[idx_target]) {
            /* Store the null slot index. */
            prev_null_idx = idx_target;
            continue;
        }

        if (_bcm_l2_station_entry_prio_cmp
            (ent_p->prio, sc->entry_arr[idx_target]->prio) > 0) {
            /* Found the entry with priority lower than ent_p prio. */
            break;
        }
    }

    temp = idx_target;

    if (idx_target != (max_entries - 1)) {
        /*  Find next empty slot after ent_p. */
        for (; temp < max_entries; temp++) {
            if (NULL == sc->entry_arr[temp]) {
                next_null_idx = temp;
                break;
            }
        }
    }

    /* Put the entry back. */
    if (TRUE == flag_no_free_entries) {
        sc->entry_arr[ent_p->hw_index] = ent_p;
    }

    /* No empty slot found. */
    if (-1 == prev_null_idx && -1 == next_null_idx) {
        return (BCM_E_CONFIG);
    }

    if (idx_target == max_entries) {
        /* Target location is after the last installed entry. */
        if (prev_null_idx == (max_entries - 1)) {
            idx_target = prev_null_idx;
            goto only_move;
        } else {
            idx_target = (max_entries - 1);
            decr_on_shift_up = FALSE;
        }
    }

    if (FALSE
        == _bcm_l2_station_entry_dir_get(unit,
                                         ent_p,
                                         prev_null_idx,
                                         next_null_idx,
                                         idx_target,
                                         &dir)) {
        return (BCM_E_PARAM);
    }

    if (1 == dir) {
        /* 
         * Move the entry at the target index to target_index+1. This may
         * mean shifting more entries down to make room. In other words,
         * shift the target index and any that follow it down 1 as far as the
         * next empty index.
         */
        if (NULL != sc->entry_arr[idx_target]) {
            BCM_IF_ERROR_RETURN
                (_bcm_l2_station_entry_shift_down(unit,
                                                  idx_target,
                                                  next_null_idx));
        }
    } else {
        if (TRUE == decr_on_shift_up) {
            idx_target--;
            L2_VVERB(("L2(unit %d) Info: Decr. on Shift UP (idx_tgt=%d).\n",
                      unit, idx_target));
        }
        if (NULL != sc->entry_arr[idx_target]) {
            BCM_IF_ERROR_RETURN
                (_bcm_l2_station_entry_shift_up(unit,
                                                idx_target,
                                                prev_null_idx));
        }
    }

    /* Move the entry from old index to new index. */
only_move:
    if (0 != (idx_target - ent_p->hw_index)) {

        if (TRUE == flag_no_free_entries) {
            prio_with_no_free_entries = TRUE;
        }

        rv = _bcm_l2_station_entry_move(unit,
                                        ent_p,
                                        (idx_target - ent_p->hw_index));
        if (BCM_FAILURE(rv)) {
            prio_with_no_free_entries = FALSE;
            return (rv);
        }

        prio_with_no_free_entries = FALSE;
    }

end:

    ent_p->flags &= ~_BCM_L2_STATION_ENTRY_PRIO_NO_CHANGE;

    rv = _bcm_l2_station_entry_install(unit, ent_p);
    if (BCM_FAILURE(rv)) {
        return rv;
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_l2_station_param_check
 * Purpose:
 *     Validate L2 station user input parameters.
 * Parameters:
 *     unit         - (IN) BCM device number
 *     station      - (IN) Station information for lookup by hardware.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_l2_station_param_check(int unit, bcm_l2_station_t *station)
{
    soc_mem_t tcam_mem; /* Hardware table name.     */

    /* Input parameter check. */
    if (NULL == station) {
        return (BCM_E_PARAM);
    }

    if ((SOC_IS_ENDURO(unit) || SOC_IS_TRIUMPH(unit)
        || SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit)
        || SOC_IS_VALKYRIE(unit) || SOC_IS_VALKYRIE2(unit)
        || SOC_IS_HURRICANEX(unit))
        && (_BCM_L2_STATION_TERM_FLAGS_MASK & station->flags)) {
        return (BCM_E_UNAVAIL);
    }

    if ((station->flags & BCM_L2_STATION_FCOE)
        && (!SOC_IS_TRIDENT(unit) && !SOC_IS_TD2_TT2(unit))) {
        /* FCoE Termination is support only on Trident family. */
        return (BCM_E_UNAVAIL);
    }

    if ((station->flags & BCM_L2_STATION_COPY_TO_CPU
        || (station->flags & BCM_L2_STATION_OAM))
        && (!SOC_IS_KATANAX(unit) && !SOC_IS_TRIUMPH3(unit))) {
        return (BCM_E_UNAVAIL);
    }

    if (((0 != station->src_port) || (0 != station->src_port_mask))
        && (!SOC_IS_KATANAX(unit) && !SOC_IS_TRIDENT(unit)
            && !SOC_IS_TD2_TT2(unit) && !SOC_IS_TRIUMPH3(unit))) {
        /* Ingress Port match lookup not supported. */
        return (BCM_E_UNAVAIL);
    }

   /* In case of KT2  OAM, OLP and XGS MAC are not part of TCAM */
   if ((SOC_IS_KATANA2(unit))  && 
       ((station->flags & BCM_L2_STATION_OAM) || 
          (station->flags & BCM_L2_STATION_OLP) || 
          (station->flags & BCM_L2_STATION_XGS_MAC))) { 
          return (BCM_E_NONE);
   } else {
        BCM_IF_ERROR_RETURN(_bcm_l2_station_tcam_mem_get(unit, &tcam_mem));
        ParamCheck(unit, tcam_mem, VLAN_IDf, station->vlan);
        ParamCheck(unit, tcam_mem, VLAN_ID_MASKf, station->vlan_mask);

#if defined(BCM_TRIDENT_SUPPORT)
       if (SOC_IS_KATANAX(unit)
            || SOC_IS_TRIDENT(unit)
            || SOC_IS_TD2_TT2(unit)
            || SOC_IS_TRIUMPH3(unit)) {
           int gport_id;
           bcm_port_t port_out;
           bcm_module_t mod_out;
           bcm_trunk_t trunk_id;
           uint32 mod_port_data = 0; /* concatenated modid and port */
           int num_bits_for_port;

           if (GPORT_TYPE(station->src_port) != 
                                  GPORT_TYPE(station->src_port_mask)) {
               return BCM_E_PARAM;
           }
           num_bits_for_port = _shr_popcount(
                                  (unsigned int)SOC_PORT_ADDR_MAX(unit));
           if (BCM_GPORT_IS_SET(station->src_port)) {
               BCM_IF_ERROR_RETURN(_bcm_esw_gport_resolve(unit, 
                          station->src_port,
                          &mod_out,
                          &port_out, &trunk_id,
                          &gport_id));
               if (BCM_GPORT_IS_TRUNK(station->src_port)) {
                   if (BCM_TRUNK_INVALID == trunk_id) {
                       return BCM_E_PORT;
                   }
                   mod_port_data = ((1 << SOC_TRUNK_BIT_POS(unit)) | trunk_id);
               } else {
                   if ((-1 == mod_out) || (-1 == port_out)) {
                       return BCM_E_PORT;
                   }
                   mod_port_data = (mod_out << num_bits_for_port) | port_out;
               }
           } else {
               int my_modid;
               BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit,&my_modid));
               port_out = station->src_port;
               mod_port_data = station->src_port | 
                               (my_modid << num_bits_for_port);
           } 
           if (SOC_MEM_FIELD_VALID(unit, tcam_mem, SOURCE_FIELDf)) {
               ParamCheck(unit, tcam_mem, SOURCE_FIELDf, mod_port_data);
           } else {
               ParamCheck(unit, tcam_mem, ING_PORT_NUMf, port_out);
           }
        }
#endif
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcm_tr_l2_station_add
 * Purpose:
 *     Create an entry with specified ID or generated ID and install
 *     the entry in hardware.
 * Parameters:
 *     unit         - (IN) BCM device number
 *     station_id   - (IN/OUT) Station entry ID.
 *     station      - (IN) Station information for lookup by hardware.
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr_l2_station_add(int unit,
                      int *station_id,
                      bcm_l2_station_t *station)
{
    _bcm_l2_station_control_t *sc;         /* Control structure.       */
    _bcm_l2_station_entry_t   *ent = NULL; /* Entry pointer.           */
    int                       rv;          /* Operation return status. */
    int                       sid = -1;    /* Entry ID.                */

    /* Input parameter check. */
    if (NULL == station || NULL == station_id)  {
        return (BCM_E_PARAM);
    }

    /* ID needs to be passed for replace operation. */
    if ((station->flags & BCM_L2_STATION_REPLACE)
        && !(station->flags & BCM_L2_STATION_WITH_ID)) {
        return (BCM_E_PARAM);
    }
    rv = _bcm_l2_station_param_check(unit, station);
    if (BCM_FAILURE(rv)) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    SC_LOCK(sc);
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        rv = _bcm_kt2_l2_station_id_get(&sid, station_id, station);
        if (BCM_FAILURE(rv)) {
            SC_UNLOCK(sc);
            return rv;
        }
    } else 
#endif
    if (station->flags & BCM_L2_STATION_WITH_ID) {
        sid = *station_id;
    } else {

        sid = ++last_allocated_sid;

        if (_BCM_L2_STATION_ID_MAX == sid) {
            last_allocated_sid = _BCM_L2_STATION_ID_BASE;
        }

        *station_id = sid;
    }

    rv = _bcm_l2_station_entry_get(unit, sid, &ent);

    /* For replace operation, entry must exist. */
    if (BCM_FAILURE(rv)
        && (station->flags & BCM_L2_STATION_REPLACE)) {
        L2_ERR(("L2(unit %d) Error: Replace (SID=%d) - Invalid (%s).\n",
                unit, sid, bcm_errmsg(rv)));
        SC_UNLOCK(sc);
        return (rv);
    }

    /* For non-replace operations, entry must not exist. */
    if (BCM_SUCCESS(rv)
        && (0 == (station->flags & BCM_L2_STATION_REPLACE))) {
        L2_ERR(("L2(unit %d) Error: (SID=%d) add - failed (%s).\n",
                unit, sid, bcm_errmsg(rv)));
        SC_UNLOCK(sc);
        return (BCM_E_EXISTS);
    }

    if (0 == (station->flags & BCM_L2_STATION_REPLACE)) {
        rv = _bcm_l2_station_entry_create(unit, sid, station, &ent);
        if (BCM_FAILURE(rv)) {
            L2_ERR(("L2(unit %d) Error: Station (SID=%d) add - failed (%s).\n",
                    unit, sid, bcm_errmsg(rv)));
            SC_UNLOCK(sc);
            return (rv);
        }
    } else {
        rv = _bcm_l2_station_entry_update(unit, sid, station, ent);
        if (BCM_FAILURE(rv)) {
            L2_ERR(("L2(unit %d) Error: Station (SID=%d) update - failed (%s).\n",
                    unit, sid, bcm_errmsg(rv)));
            SC_UNLOCK(sc);
            return (rv);
        }
    }

    rv = _bcm_l2_station_entry_prio_install(unit, ent);
    if (BCM_FAILURE(rv)) {
        if (NULL != ent->tcam_ent) {
            sal_free(ent->tcam_ent);
        }
        L2_ERR(("L2(unit %d) Error: (SID=%d) install - failed (%s).\n",
                unit, sid, bcm_errmsg(rv)));
        SC_UNLOCK(sc);
        return (rv);
    }

    /* Mark the entry as installed. */
    ent->flags |= _BCM_L2_STATION_ENTRY_INSTALLED;

    SC_UNLOCK(sc);
    return (rv);
}

/*
 * Function:
 *     bcm_tr_l2_station_delete
 * Purpose:
 *     Deallocate the memory used to track an entry and remove the entry
 *     from hardware.
 * Parameters:
 *     unit         - (IN) BCM device number
 *     station_id   - (IN) Station entry ID.
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr_l2_station_delete(int unit, int station_id)
{
    _bcm_l2_station_entry_t     *s_ent = NULL; /* Entry pointer.           */
    _bcm_l2_station_control_t   *sc = NULL;    /* Control structure.       */
    int                         rv;            /* Operation return status. */
    soc_mem_t                   tcam_mem;      /* TCMA memory name.        */

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    SC_LOCK(sc);

    /* Get the entry by ID. */
    rv = _bcm_l2_station_entry_get(unit, station_id, &s_ent);
    if (BCM_FAILURE(rv)) {
        SC_UNLOCK(sc);
        return (rv);
    }
#ifdef BCM_KATANA2_SUPPORT
    if ((SOC_IS_KATANA2(unit)) && 
        (s_ent->flags & _BCM_L2_STATION_ENTRY_TYPE_NON_TCAM)) {
        rv = _bcm_kt2_l2_station_entry_delete(unit, sc, s_ent, station_id);
        /* Free the entry. */
        sal_free(s_ent);
        SC_UNLOCK(sc);
        return rv;
    }  
#endif

    /* Get the TCAM memory name. */
    rv = _bcm_l2_station_tcam_mem_get(unit, &tcam_mem);
    if (BCM_FAILURE(rv)) {
        SC_UNLOCK(sc);
        return (rv);
    }

    /* Remove the entry from hardware by clearing the index. */
    rv = soc_mem_write(unit, tcam_mem, MEM_BLOCK_ALL, s_ent->hw_index,
                       soc_mem_entry_null(unit, tcam_mem));
    if (BCM_FAILURE(rv)) {
        SC_UNLOCK(sc);
        return (rv);
    }

    /* Free tcam buffer. */
    if (NULL != s_ent->tcam_ent) {
        sal_free(s_ent->tcam_ent);
    }

    /* Remove the entry pointer from the list. */
    sc->entry_arr[s_ent->hw_index] = NULL;

    /* Increment free entries count. */
    sc->entries_free++;

    /* Free the entry. */
    sal_free(s_ent);

    SC_UNLOCK(sc);

    return (rv);
}

/*
 * Function:
 *     bcm_tr_l2_station_delete_all
 * Purpose:
 *     Deallocate all memory used to track entries and remove entries from
 *     hardware.
 * Parameters:
 *     unit     - (IN) BCM device number
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr_l2_station_delete_all(int unit)
{
    _bcm_l2_station_control_t   *sc = NULL; /* Control strucutre.       */
    int                         rv;         /* Operation return status. */
    int                         index;      /* Entry index.             */
    int                         max_entries;
    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));

    SC_LOCK(sc);

    if (NULL == sc->entry_arr) {
        SC_UNLOCK(sc);
        return (BCM_E_NONE);
    }

    max_entries = sc->entries_total;

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        max_entries = sc->entries_total + sc->port_entries_total + 
                                          sc->olp_entries_total + 1;
    }
#endif 

    for (index = 0; index < max_entries; index++) {

        if (NULL == sc->entry_arr[index]) {
            continue;
        }

        rv = bcm_tr_l2_station_delete(unit, sc->entry_arr[index]->sid);
        if (BCM_FAILURE(rv)) {
            SC_UNLOCK(sc);
            return (rv);
        }
    }

    SC_UNLOCK(sc);
    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcm_tr_l2_station_get
 * Purpose:
 *     Get station information for a given station ID.
 * Parameters:
 *     unit         - (IN) BCM device number
 *     station_id   - (IN) Station entry ID.
 *     station      - (OUT) Station entry lookup information.
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr_l2_station_get(int unit, int station_id, bcm_l2_station_t *station)
{
    _bcm_l2_station_entry_t     *s_ent = NULL; /* Entry pointer.           */
    _bcm_l2_station_control_t   *sc = NULL;    /* Control structure.       */
    int                         rv;            /* Operation return status. */
    soc_mem_t                   tcam_mem;      /* TCAM memory name.        */
    uint32                      entry[SOC_MAX_MEM_FIELD_WORDS]; /* Entry   */
                                                                /* buffer. */

    /* Input parameter check. */
    if (NULL == station) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));
    SC_LOCK(sc);

    /* Get the entry by ID. */
    rv = _bcm_l2_station_entry_get(unit, station_id, &s_ent);
    if (BCM_FAILURE(rv)) {
        SC_UNLOCK(sc);
        return (rv);
    }
#ifdef BCM_KATANA2_SUPPORT
    if ((SOC_IS_KATANA2(unit)) && 
        (s_ent->flags & _BCM_L2_STATION_ENTRY_TYPE_NON_TCAM)) {
        rv = _bcm_kt2_l2_station_entry_get(unit, sc, s_ent, 
                                           station_id, station);
        SC_UNLOCK(sc);
        return rv;
    }  
#endif
    rv = _bcm_l2_station_tcam_mem_get(unit, &tcam_mem);
    if (BCM_FAILURE(rv)) {
        SC_UNLOCK(sc);
        return (rv);
    }

    /* Clear entry buffer. */
    sal_memset(entry, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));

    rv = soc_mem_read(unit, tcam_mem, MEM_BLOCK_ANY, s_ent->hw_index, entry);
    if (BCM_FAILURE(rv)) {
        SC_UNLOCK(sc);
        return (rv);
    }

    station->priority = s_ent->prio;

    soc_mem_mac_addr_get(unit, tcam_mem, entry,
                         MAC_ADDRf, station->dst_mac);

    soc_mem_mac_addr_get(unit, tcam_mem, entry,
                         MAC_ADDR_MASKf, station->dst_mac_mask);

    station->vlan
        = soc_mem_field32_get(unit, tcam_mem, entry, VLAN_IDf);

    station->vlan_mask
        = soc_mem_field32_get(unit, tcam_mem, entry, VLAN_ID_MASKf);

#if defined(BCM_TRIDENT_SUPPORT)
    if (SOC_IS_KATANAX(unit)
        || SOC_IS_TRIDENT(unit)
        || SOC_IS_TD2_TT2(unit)
        || SOC_IS_TRIUMPH3(unit)) {
        int src_field;
        int src_mask_field;
        _bcm_gport_dest_t src;
        _bcm_gport_dest_t src_mask;
        int num_bits_for_port;

        if (SOC_MEM_FIELD_VALID(unit, tcam_mem, SOURCE_FIELDf)) {
            src_field= soc_mem_field32_get(unit, tcam_mem, entry,
                            SOURCE_FIELDf);
            src_mask_field = soc_mem_field32_get(unit, tcam_mem, entry,
                                  SOURCE_FIELD_MASKf);

            if ((src_field >> SOC_TRUNK_BIT_POS(unit)) & 0x1) {
                src.tgid = src_field & ((1 << SOC_TRUNK_BIT_POS(unit)) - 1);
                src.gport_type = _SHR_GPORT_TYPE_TRUNK;
                src_mask.tgid = src_mask_field & 
                                    ((1 << SOC_TRUNK_BIT_POS(unit)) - 1);
                src_mask.gport_type = _SHR_GPORT_TYPE_TRUNK;
            } else {
                src.port = src_field & SOC_PORT_ADDR_MAX(unit);
                src.modid = (src_field - src.port) /
                               (SOC_PORT_ADDR_MAX(unit) + 1);
                src.gport_type = _SHR_GPORT_TYPE_MODPORT;
                num_bits_for_port = _shr_popcount(
                                 (unsigned int)SOC_PORT_ADDR_MAX(unit));
                src_mask.port = src_mask_field & SOC_PORT_ADDR_MAX(unit);
                src_mask.gport_type = _SHR_GPORT_TYPE_MODPORT;
                src_mask.modid = (src_mask_field >> num_bits_for_port) & 
                                 SOC_MODID_MAX(unit);
            }
            BCM_IF_ERROR_RETURN(_bcm_esw_gport_construct(unit, &src, 
                                &(station->src_port)));
            BCM_IF_ERROR_RETURN(_bcm_esw_gport_construct(unit, &src_mask, 
                                &(station->src_port_mask)));
        } else {
            station->src_port
                = soc_mem_field32_get(unit, tcam_mem, entry,
                                  ING_PORT_NUMf);
            station->src_port_mask
                = soc_mem_field32_get(unit, tcam_mem, entry,
                                  ING_PORT_NUM_MASKf);
        }
        
        if (soc_mem_field32_get(unit, tcam_mem, entry,
                                MIM_TERMINATION_ALLOWEDf)) {
            station->flags |= BCM_L2_STATION_MIM;
        }

        if (soc_mem_field32_get(unit, tcam_mem, entry,
                                MPLS_TERMINATION_ALLOWEDf)) {
            station->flags |= BCM_L2_STATION_MPLS;
        }
        
        if (soc_mem_field32_get(unit, tcam_mem, entry,
                                IPV4_TERMINATION_ALLOWEDf)) {
            station->flags |= BCM_L2_STATION_IPV4;
        }

        if (soc_mem_field32_get(unit, tcam_mem, entry,
                                IPV6_TERMINATION_ALLOWEDf)) {
            station->flags |= BCM_L2_STATION_IPV6;
        }
        
        if (soc_mem_field32_get(unit, tcam_mem, entry,
                                ARP_RARP_TERMINATION_ALLOWEDf)) {
            station->flags |= BCM_L2_STATION_ARP_RARP;
        }

    }
#endif

#if defined(BCM_TRIDENT_SUPPORT)
    if (SOC_IS_TRIDENT(unit)
        || SOC_IS_TD2_TT2(unit)) {

        if (soc_mem_field32_get(unit, tcam_mem, entry,
                                TRILL_TERMINATION_ALLOWEDf)) {
            station->flags |= BCM_L2_STATION_TRILL;
        }

        if (soc_mem_field32_get(unit, tcam_mem, entry,
                                FCOE_TERMINATION_ALLOWEDf)) {
            station->flags |= BCM_L2_STATION_FCOE;
        }
    }
#endif

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) {

        if (soc_mem_field_valid(unit, tcam_mem, OAM_TERMINATION_ALLOWEDf)) { 
            if (soc_mem_field32_get(unit, tcam_mem, entry,
                                    OAM_TERMINATION_ALLOWEDf)) {
                station->flags |= BCM_L2_STATION_OAM;
            }
        }

        if (soc_mem_field32_get(unit, tcam_mem, entry,
                                COPY_TO_CPUf)) {
            station->flags |= BCM_L2_STATION_COPY_TO_CPU;
        }
    }
#endif

    SC_UNLOCK(sc);

    return (rv);
}

/*
 * Function:
 *     bcm_tr_l2_station_size_get
 * Purpose:
 *     Get number of entries supported by L2 station API.
 * Parameters:
 *     unit   - (IN) BCM device number
 *     size   - (OUT) Station entry ID.
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr_l2_station_size_get(int unit, int *size)       
{
    _bcm_l2_station_control_t   *sc = NULL; /* Station control structure. */
    
    /* Input parameter check. */
    if (NULL == size) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_l2_station_control_get(unit, &sc));
    SC_LOCK(sc);

    *size = sc->entries_total;

    SC_UNLOCK(sc);
    return (BCM_E_NONE);
}
#endif /* !BCM_TRIUMPH_SUPPORT */

/*
 * Function:
 *     _bcm_tr_l2_ppa_setup
 * Description:
 *     Setup hardware L2 PPA registers
 * Parameters:
 *     unit         device number
 *     flags        flags BCM_L2_REPLACE_*
 *     rep_st       structure with information of what to replace
 * Return:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr_l2_ppa_setup(int unit, _bcm_l2_replace_t *rep_st)
{
    soc_field_t field;
    uint32  rval, ppa_mode;
    uint32  rval_limit, limit_en;
    int field_len;

    switch (rep_st->flags &
            (BCM_L2_REPLACE_MATCH_DEST | BCM_L2_REPLACE_MATCH_VLAN |
             BCM_L2_REPLACE_DELETE)) {
    case BCM_L2_REPLACE_MATCH_DEST | BCM_L2_REPLACE_DELETE:
        ppa_mode = 0;
        break;
    case BCM_L2_REPLACE_MATCH_VLAN | BCM_L2_REPLACE_DELETE:
        ppa_mode = 1;
        break;
    case BCM_L2_REPLACE_MATCH_DEST | BCM_L2_REPLACE_MATCH_VLAN |
        BCM_L2_REPLACE_DELETE:
        ppa_mode = 2;
        break;
    case BCM_L2_REPLACE_DELETE:
        ppa_mode = 3;
        break;
    case BCM_L2_REPLACE_MATCH_DEST:
        ppa_mode = 4;
        break;
    case BCM_L2_REPLACE_MATCH_VLAN:
        ppa_mode = 5;
        break;
    case BCM_L2_REPLACE_MATCH_DEST | BCM_L2_REPLACE_MATCH_VLAN :
        ppa_mode = 6;
        break;
    default:
        return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(READ_PER_PORT_REPL_CONTROLr(unit, &rval));
    if ((soc_feature(unit, soc_feature_mac_learn_limit))) {

        SOC_IF_ERROR_RETURN(READ_SYS_MAC_LIMIT_CONTROLr(unit, &rval_limit));
        limit_en = soc_reg_field_get(unit, SYS_MAC_LIMIT_CONTROLr, 
                                 rval_limit, ENABLEf);
        soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, LIMIT_COUNTEDf, 
                          limit_en);
        soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, 
                      USE_OLD_LIMIT_COUNTEDf, limit_en);
    }
    soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, KEY_TYPEf,
                      rep_st->key_type);
    soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, EXCL_STATICf,
                      rep_st->flags & BCM_L2_REPLACE_MATCH_STATIC ? 0 : 1);
    soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, EXCL_NON_PENDINGf,
                      rep_st->flags & BCM_L2_REPLACE_PENDING ? 1 : 0);
    soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, EXCL_PENDINGf,
                      rep_st->flags & BCM_L2_REPLACE_PENDING ? 0 : 1);
    if (!(rep_st->flags & BCM_L2_REPLACE_DELETE)) {
        if (rep_st->new_dest.vp != -1) {
            /* For virtual ports PORT_NUMf provides new VPG,
             * Reset MODULE_IDf and PORT_NUMf as it is overlayed with VPG
             */
            soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, Tf, 0);
            soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, MODULE_IDf, 
                              (rep_st->new_dest.vp >> 6));
            soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, PORT_NUMf, 
                              (rep_st->new_dest.vp & 0x3F));
            if (soc_reg_field_valid(unit, PER_PORT_REPL_CONTROLr, DEST_TYPEf)) {
                soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval,
                                  DEST_TYPEf, 2);
            }
        } else if (rep_st->new_dest.trunk != -1) {
            soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, Tf, 1);
            soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, TGIDf,
                              rep_st->new_dest.trunk);
        } else {
            soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, Tf, 0);
            soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, MODULE_IDf,
                              rep_st->new_dest.module);
            soc_reg_field_set(unit, PER_PORT_REPL_CONTROLr, &rval, PORT_NUMf,
                              rep_st->new_dest.port);
        }
    }
    SOC_IF_ERROR_RETURN(WRITE_PER_PORT_REPL_CONTROLr(unit, rval));

    rval = 0;
    soc_reg_field_set(unit, PER_PORT_AGE_CONTROLr, &rval, PPA_MODEf,
                      ppa_mode);
    if (rep_st->flags & BCM_L2_REPLACE_MATCH_VLAN) {
        soc_reg_field_set(unit, PER_PORT_AGE_CONTROLr, &rval, VLAN_IDf,
                          rep_st->key_vfi != -1 ?
                          rep_st->key_vfi : rep_st->key_vlan);
    }
    if (rep_st->flags & BCM_L2_REPLACE_MATCH_DEST) {
        if (rep_st->match_dest.vp != -1) {
            if (soc_reg_field_valid(unit, PER_PORT_AGE_CONTROLr, DEST_TYPEf)) {
                soc_reg_field_set(unit, PER_PORT_AGE_CONTROLr, &rval,
                                  DEST_TYPEf, 2);
                soc_reg_field_set(unit, PER_PORT_AGE_CONTROLr, &rval, VPGf,
                                  rep_st->match_dest.vp);
            } else {
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, ING_Q_BEGINr, REG_PORT_ANY,
                                            VPG_TYPEf, 1));
                field_len = soc_reg_field_length(unit, PER_PORT_AGE_CONTROLr,
                                                 PORT_NUMf);
                soc_reg_field_set(unit, PER_PORT_AGE_CONTROLr, &rval,
                                  MODULE_IDf,
                                  rep_st->match_dest.vp >> field_len);
                soc_reg_field_set(unit, PER_PORT_AGE_CONTROLr, &rval,
                                  PORT_NUMf,
                                  rep_st->match_dest.vp &
                                  ((1 << field_len) - 1));
            }
        } else if (rep_st->match_dest.trunk != -1) {
            soc_reg_field_set(unit, PER_PORT_AGE_CONTROLr, &rval, Tf, 1);
            soc_reg_field_set(unit, PER_PORT_AGE_CONTROLr, &rval, TGIDf,
                              rep_st->match_dest.trunk);
        } else {
            soc_reg_field_set(unit, PER_PORT_AGE_CONTROLr, &rval, MODULE_IDf,
                              rep_st->match_dest.module);
            soc_reg_field_set(unit, PER_PORT_AGE_CONTROLr, &rval, PORT_NUMf,
                              rep_st->match_dest.port);
        }
    }
    SOC_IF_ERROR_RETURN(WRITE_PER_PORT_AGE_CONTROLr(unit, rval));

    if (SOC_CONTROL(unit)->l2x_mode == L2MODE_FIFO) {
        field = rep_st->flags & BCM_L2_REPLACE_DELETE ?
            L2_MOD_FIFO_ENABLE_PPA_DELETEf : L2_MOD_FIFO_ENABLE_PPA_REPLACEf;
        BCM_IF_ERROR_RETURN(soc_reg_field32_modify
                            (unit, AUX_ARB_CONTROLr, REG_PORT_ANY, field,
                             rep_st->flags & BCM_L2_REPLACE_NO_CALLBACKS ?
                             0 : 1));
    }

    return BCM_E_NONE;
}

#ifdef BCM_TRIUMPH_SUPPORT
/*
 * Function:
 *     _bcm_tr_ext_l2_ppa_setup
 * Description:
 *     Setup hardware external L2 PPA registers
 * Parameters:
 *     unit         device number
 *     rep_st       structure with information of what to replace
 * Return:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr_ext_l2_ppa_setup(int unit, _bcm_l2_replace_t *rep_st)
{
    uint32 rval, ppa_mode;
    ext_l2_mod_fifo_entry_t ext_l2_mod_entry;
    ext_l2_entry_entry_t ext_l2_entry;

    switch (rep_st->flags &
            (BCM_L2_REPLACE_MATCH_DEST | BCM_L2_REPLACE_MATCH_VLAN |
             BCM_L2_REPLACE_DELETE)) {
    case BCM_L2_REPLACE_MATCH_DEST | BCM_L2_REPLACE_DELETE:
        ppa_mode = 0;
        break;
    case BCM_L2_REPLACE_MATCH_VLAN | BCM_L2_REPLACE_DELETE:
        ppa_mode = 1;
        break;
    case BCM_L2_REPLACE_MATCH_DEST | BCM_L2_REPLACE_MATCH_VLAN |
        BCM_L2_REPLACE_DELETE:
        ppa_mode = 2;
        break;
    case BCM_L2_REPLACE_DELETE:
        ppa_mode = 3;
        break;
    case BCM_L2_REPLACE_MATCH_DEST:
        ppa_mode = 4;
        break;
    case BCM_L2_REPLACE_MATCH_VLAN:
        ppa_mode = 5;
        break;
    case BCM_L2_REPLACE_MATCH_DEST | BCM_L2_REPLACE_MATCH_VLAN :
        ppa_mode = 6;
        break;
    default:
        return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(READ_ESM_PER_PORT_REPL_CONTROLr(unit, &rval));
    soc_reg_field_set(unit, ESM_PER_PORT_REPL_CONTROLr, &rval, KEY_TYPE_VFIf,
                      rep_st->key_vfi != -1 ? 1 : 0);
    soc_reg_field_set(unit, ESM_PER_PORT_REPL_CONTROLr, &rval, EXCL_STATICf,
                      rep_st->flags & BCM_L2_REPLACE_MATCH_STATIC ? 0 : 1);
    if (!(rep_st->flags & BCM_L2_REPLACE_DELETE)) {
        if (rep_st->new_dest.trunk != -1) {
            soc_reg_field_set(unit, ESM_PER_PORT_REPL_CONTROLr, &rval, Tf, 1);
            soc_reg_field_set(unit, ESM_PER_PORT_REPL_CONTROLr, &rval, TGIDf,
                              rep_st->new_dest.trunk);
        } else {
            soc_reg_field_set(unit, ESM_PER_PORT_REPL_CONTROLr, &rval, Tf, 0);
            soc_reg_field_set(unit, ESM_PER_PORT_REPL_CONTROLr, &rval,
                              MODULE_IDf, rep_st->new_dest.module);
            soc_reg_field_set(unit, ESM_PER_PORT_REPL_CONTROLr, &rval,
                              PORT_NUMf, rep_st->new_dest.port);
        }
    }
    if (SOC_CONTROL(unit)->l2x_mode == L2MODE_FIFO) {
        soc_reg_field_set(unit, ESM_PER_PORT_REPL_CONTROLr, &rval, CPU_NOTIFYf,
                          rep_st->flags & BCM_L2_REPLACE_NO_CALLBACKS ? 0 : 1);
    }
    SOC_IF_ERROR_RETURN(WRITE_ESM_PER_PORT_REPL_CONTROLr(unit, rval));

    /*
     * Unlike L2_MOD_FIFO, EXT_L2_MOD_FIFO does not report both new and
     * replaced L2 destination for PPA replace command. To workaround
     * the problem, we add an special entry to EXT_L2_MOD_FIFO before
     * issuing the PPA replace command. The special entry has the new
     * destination and a special "type" value. L2 mod fifo processing
     * thread knows all entries after this special entry are associated
     * with this new destination.
     */
    sal_memset(&ext_l2_mod_entry, 0, sizeof(ext_l2_mod_entry));
    sal_memset(&ext_l2_entry, 0, sizeof(ext_l2_entry));

    if (rep_st->new_dest.trunk != -1) {
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, &ext_l2_entry, Tf, 1);
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, &ext_l2_entry, TGIDf,
                            rep_st->new_dest.trunk);
    } else {
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, &ext_l2_entry, MODULE_IDf,
                            rep_st->new_dest.module);
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, &ext_l2_entry, PORT_NUMf,
                            rep_st->new_dest.port);
    }
    soc_mem_field_set(unit, EXT_L2_MOD_FIFOm, (uint32 *)&ext_l2_mod_entry,
                      WR_DATAf, (uint32 *)&ext_l2_entry);
    /* borrow INSERTED type (value 3) as special mark */
    soc_mem_field32_set(unit, EXT_L2_MOD_FIFOm, &ext_l2_mod_entry, TYPf, 3);
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, EXT_L2_MOD_FIFOm, MEM_BLOCK_ANY,
                                      0, &ext_l2_mod_entry));

    rval = 0;
    soc_reg_field_set(unit, ESM_PER_PORT_AGE_CONTROLr, &rval, PPA_MODEf,
                      ppa_mode);
    if (rep_st->flags & BCM_L2_REPLACE_MATCH_VLAN) {
        soc_reg_field_set(unit, ESM_PER_PORT_AGE_CONTROLr, &rval, VLAN_IDf,
                          rep_st->key_vfi != -1 ?
                          rep_st->key_vfi : rep_st->key_vlan);
    }
    if (rep_st->flags & BCM_L2_REPLACE_MATCH_DEST) {
        if (rep_st->match_dest.trunk != -1) {
            soc_reg_field_set(unit, ESM_PER_PORT_AGE_CONTROLr, &rval, Tf, 1);
            soc_reg_field_set(unit, ESM_PER_PORT_AGE_CONTROLr, &rval, TGIDf,
                              rep_st->match_dest.trunk);
        } else {
            soc_reg_field_set(unit, ESM_PER_PORT_AGE_CONTROLr, &rval,
                              MODULE_IDf, rep_st->match_dest.module);
            soc_reg_field_set(unit, ESM_PER_PORT_AGE_CONTROLr, &rval,
                              PORT_NUMf, rep_st->match_dest.port);
        }
    }
    SOC_IF_ERROR_RETURN(WRITE_ESM_PER_PORT_AGE_CONTROLr(unit, rval));

    return BCM_E_NONE;
}
#endif /* BCM_TRIUMPH_SUPPORT */

/*
 * Function:
 *     _bcm_td_l2_bulk_control_setup
 * Description:
 *     Setup hardware L2 bulk control registers
 * Parameters:
 *     unit         device number
 *     rep_st       structure with information of what to replace
 * Return:
 *     BCM_E_XXX
 */
STATIC int
_bcm_td_l2_bulk_control_setup(int unit, _bcm_l2_replace_t *rep_st)
{
    uint32 rval;
    int action;

    SOC_IF_ERROR_RETURN(READ_L2_BULK_CONTROLr(unit, &rval)); 
    if (rep_st->int_flags & _BCM_L2_REPLACE_INT_NO_ACTION) {
        action = 0;
    } else if (rep_st->flags & BCM_L2_REPLACE_DELETE) {
        action = 1;
    } else if (rep_st->flags & BCM_L2_REPLACE_AGE) {
        action = 3;
    } else {
        action = 2;
    }
    soc_reg_field_set(unit, L2_BULK_CONTROLr, &rval, ACTIONf, action);
    if (SOC_CONTROL(unit)->l2x_mode == L2MODE_FIFO) {
        soc_reg_field_set(unit, L2_BULK_CONTROLr, &rval, L2_MOD_FIFO_RECORDf, 
                          rep_st->flags & BCM_L2_REPLACE_NO_CALLBACKS ?
                          0 : 1); 
        soc_reg_field_set(unit, L2_BULK_CONTROLr, &rval, L2_MOD_FIFO_RECORDf, 
                          rep_st->int_flags & BCM_L2_REPLACE_NO_CALLBACKS ?
                          0 : 1); 
    }
    SOC_IF_ERROR_RETURN(WRITE_L2_BULK_CONTROLr(unit, rval)); 

    BCM_IF_ERROR_RETURN
        (WRITE_L2_BULK_MATCH_MASKm(unit, MEM_BLOCK_ALL, 0,
                                   &rep_st->match_mask));
    BCM_IF_ERROR_RETURN
        (WRITE_L2_BULK_MATCH_DATAm(unit, MEM_BLOCK_ALL, 0,
                                   &rep_st->match_data));

    if (!( rep_st->flags & (BCM_L2_REPLACE_DELETE | BCM_L2_REPLACE_AGE) )) {
        BCM_IF_ERROR_RETURN
            (WRITE_L2_BULK_REPLACE_MASKm(unit, MEM_BLOCK_ALL, 0,
                                         &rep_st->new_mask));
        BCM_IF_ERROR_RETURN
            (WRITE_L2_BULK_REPLACE_DATAm(unit, MEM_BLOCK_ALL, 0,
                                         &rep_st->new_data));
    }

    return BCM_E_NONE;
}

/*
 * Match DEST_TYPE + MODULE_ID + PORT_NUM and replace with one of following:
 * DEST_TYPE = 0, new MODULE_ID, new PORT_NUM
 * DEST_TYPE = 1, new TGID
 * Notes:
 *     Need to match all bits of DEST_TYPE + DESTINATION, therefore can not
 *     match on trunk destination because the unused overlay bits can have any
 *     data.
 */
STATIC int
_bcm_td_l2_bulk_replace_modport(int unit, _bcm_l2_replace_t *rep_st)
{
    l2_bulk_match_mask_entry_t match_mask;
    l2_bulk_match_data_entry_t match_data;
    l2_bulk_replace_mask_entry_t repl_mask;
    l2_bulk_replace_data_entry_t repl_data;
    int field_len;

    sal_memset(&match_mask, 0, sizeof(match_mask));
    sal_memset(&match_data, 0, sizeof(match_data));
    sal_memset(&repl_mask, 0, sizeof(repl_mask));
    sal_memset(&repl_data, 0, sizeof(repl_data));

    soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask, VALIDf, 1);
    soc_mem_field32_set(unit, L2_BULK_MATCH_DATAm, &match_data, VALIDf, 1);

    field_len = soc_mem_field_length(unit, L2_BULK_MATCH_MASKm, KEY_TYPEf);
    soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask, KEY_TYPEf,
                        (1 << field_len) - 1);
    /* KEY_TYPE field in data is 0 */

    field_len = soc_mem_field_length(unit, L2_BULK_MATCH_MASKm, DEST_TYPEf);
    soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask, DEST_TYPEf,
                        (1 << field_len) - 1);
    /* DEST_TYPE field in data is 0 */

    field_len = soc_mem_field_length(unit, L2_BULK_MATCH_MASKm, MODULE_IDf);
    soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask, MODULE_IDf,
                        (1 << field_len) - 1);
    soc_mem_field32_set(unit, L2_BULK_MATCH_DATAm, &match_data, MODULE_IDf,
                        rep_st->match_dest.module);

    field_len = soc_mem_field_length(unit, L2_BULK_MATCH_MASKm, PORT_NUMf);
    soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask, PORT_NUMf,
                        (1 << field_len) - 1);
    soc_mem_field32_set(unit, L2_BULK_MATCH_DATAm, &match_data, PORT_NUMf,
                        rep_st->match_dest.port);

    if (!(rep_st->flags & BCM_L2_REPLACE_MATCH_STATIC)) {
        soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask,
                            STATIC_BITf, 1);
        /* STATIC_BIT field in data is 0 */
    }

    soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask, PENDINGf, 1);
    if (rep_st->flags & BCM_L2_REPLACE_PENDING) {
        soc_mem_field32_set(unit, L2_BULK_MATCH_DATAm, &match_data, PENDINGf,
                            1);
    }

    soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask, EVEN_PARITYf,
                        1);

    BCM_IF_ERROR_RETURN
        (WRITE_L2_BULK_MATCH_MASKm(unit, MEM_BLOCK_ALL, 0, &match_mask));

    field_len = soc_mem_field_length(unit, L2_BULK_REPLACE_MASKm, DEST_TYPEf);
    soc_mem_field32_set(unit, L2_BULK_REPLACE_MASKm, &repl_mask, DEST_TYPEf,
                        (1 << field_len) - 1);

    field_len = soc_mem_field_length(unit, L2_BULK_REPLACE_MASKm, MODULE_IDf);
    soc_mem_field32_set(unit, L2_BULK_REPLACE_MASKm, &repl_mask, MODULE_IDf,
                        (1 << field_len) - 1);

    field_len = soc_mem_field_length(unit, L2_BULK_REPLACE_MASKm, PORT_NUMf);
    soc_mem_field32_set(unit, L2_BULK_REPLACE_MASKm, &repl_mask, PORT_NUMf,
                        (1 << field_len) - 1);

    if (rep_st->new_dest.trunk != -1) {
        soc_mem_field32_set(unit, L2_BULK_REPLACE_DATAm, &repl_data, Tf, 1);
        soc_mem_field32_set(unit, L2_BULK_REPLACE_DATAm, &repl_data, TGIDf,
                            rep_st->new_dest.trunk);
    } else {
        /* T field in data is 0 */
        soc_mem_field32_set(unit, L2_BULK_REPLACE_DATAm, &repl_data,
                            MODULE_IDf, rep_st->new_dest.module);
        soc_mem_field32_set(unit, L2_BULK_REPLACE_DATAm, &repl_data, PORT_NUMf,
                            rep_st->new_dest.port);
    }

    soc_mem_field32_set(unit, L2_BULK_REPLACE_MASKm, &repl_mask, EVEN_PARITYf,
                        1);
    BCM_IF_ERROR_RETURN
        (WRITE_L2_BULK_REPLACE_MASKm(unit, MEM_BLOCK_ALL, 0, &repl_mask));

    BCM_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, L2_BULK_CONTROLr, REG_PORT_ANY, ACTIONf,
                                2));

    /* Replace all entries having EVEN_PARITY == 0 */
    /* EVEN_PARITY field in data is 0 */
    BCM_IF_ERROR_RETURN
        (WRITE_L2_BULK_MATCH_DATAm(unit, MEM_BLOCK_ALL, 0, &match_data));

    soc_mem_field32_set(unit, L2_BULK_REPLACE_DATAm, &repl_data, EVEN_PARITYf,
                        1);
    BCM_IF_ERROR_RETURN
        (WRITE_L2_BULK_REPLACE_DATAm(unit, MEM_BLOCK_ALL, 0, &repl_data));

    BCM_IF_ERROR_RETURN(soc_l2x_port_age(unit, L2_BULK_CONTROLr, INVALIDr));

    /* Replace all entries having EVEN_PARITY == 1 */
    soc_mem_field32_set(unit, L2_BULK_MATCH_DATAm, &match_data, EVEN_PARITYf,
                        1);
    BCM_IF_ERROR_RETURN
        (WRITE_L2_BULK_MATCH_DATAm(unit, MEM_BLOCK_ALL, 0, &match_data));

    soc_mem_field32_set(unit, L2_BULK_REPLACE_DATAm, &repl_data, EVEN_PARITYf,
                        0);
    BCM_IF_ERROR_RETURN
        (WRITE_L2_BULK_REPLACE_DATAm(unit, MEM_BLOCK_ALL, 0, &repl_data));

    BCM_IF_ERROR_RETURN(soc_l2x_port_age(unit, L2_BULK_CONTROLr, INVALIDr));

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_tr_l2_replace_by_hw
 * Description:
 *     Helper function to _bcm_l2_replace_by_hw
 * Parameters:
 *     unit         device number
 *     rep_st       structure with information of what to replace
 * Return:
 *     BCM_E_XXX
 */
int
_bcm_tr_l2_replace_by_hw(int unit, _bcm_l2_replace_t *rep_st)
{
    soc_control_t *soc = SOC_CONTROL(unit);
    soc_reg_t int_reg, ext_reg;
    int rv;
    int bulk_replace_modport;
    uint32 parity_diff;
#if defined(BCM_TRIUMPH_SUPPORT)
    int do_l2e_ppa_match = 1;
#endif	/* BCM_TRIUMPH_SUPPORT*/

    if (NULL == rep_st) {
        return BCM_E_PARAM;
    }

    bulk_replace_modport = FALSE;
    if (soc_feature(unit, soc_feature_l2_bulk_bypass_replace) &&
        !(rep_st->int_flags & _BCM_L2_REPLACE_INT_NO_ACTION)) {
        if ((rep_st->flags &
             (BCM_L2_REPLACE_MATCH_DEST | BCM_L2_REPLACE_MATCH_VLAN |
              BCM_L2_REPLACE_DELETE | BCM_L2_REPLACE_AGE)) == BCM_L2_REPLACE_MATCH_DEST) {
            if (rep_st->match_dest.trunk == -1) {
                /* Find out if the new data will cause parity bit change */
                if (rep_st->new_dest.trunk != -1) {
                    parity_diff =
                        rep_st->match_dest.module ^ rep_st->match_dest.port ^
                        rep_st->new_dest.trunk ^ 1;
                } else {
                    parity_diff = 
                        rep_st->match_dest.module ^ rep_st->match_dest.port ^
                        rep_st->new_dest.module ^ rep_st->new_dest.port;
                }
                parity_diff ^= parity_diff >> 4;
                parity_diff ^= parity_diff >> 2;
                parity_diff ^= parity_diff >> 1;
                bulk_replace_modport = parity_diff & 1 ? TRUE : FALSE;
            }
        }
    }

    if (bulk_replace_modport) {
        return _bcm_td_l2_bulk_replace_modport(unit, rep_st);
    } else {
        ext_reg = INVALIDr;
        if (soc_feature(unit, soc_feature_l2_bulk_control)) {
            BCM_IF_ERROR_RETURN(_bcm_td_l2_bulk_control_setup(unit, rep_st));
            int_reg = L2_BULK_CONTROLr;
        } else {
            BCM_IF_ERROR_RETURN(_bcm_tr_l2_ppa_setup(unit, rep_st));
            int_reg = PER_PORT_AGE_CONTROLr;

#if defined(BCM_TRIUMPH_SUPPORT)
            if (soc_mem_is_valid(unit, EXT_L2_ENTRYm) &&
                soc_mem_index_count(unit, EXT_L2_ENTRYm) > 0) {
                if (do_l2e_ppa_match) {
                    BCM_IF_ERROR_RETURN(_bcm_tr_l2e_ppa_match(unit, rep_st));
                } else {
                    BCM_IF_ERROR_RETURN
                        (_bcm_tr_ext_l2_ppa_setup(unit, rep_st));
                    ext_reg = ESM_PER_PORT_AGE_CONTROLr;
                }
            }
#endif	/* BCM_TRIUMPH_SUPPORT*/
        }

#ifdef PLISIM
#ifdef _KATANA_DEBUG /* BCM_KATANA_SUPPORT */
	_bcm_kt_enable_port_age_simulation(flags,rep_st);
#endif
#endif
        BCM_IF_ERROR_RETURN(soc_l2x_port_age(unit, int_reg, ext_reg));

        if (rep_st->int_flags & _BCM_L2_REPLACE_INT_NO_ACTION) {
            return BCM_E_NONE;
        }

        if (!(rep_st->flags & BCM_L2_REPLACE_DELETE)) {
            return BCM_E_NONE;
        }

        if (SOC_L2_DEL_SYNC_LOCK(soc) < 0) {
            return BCM_E_RESOURCE;
        }
        rv = _soc_l2x_sync_replace
            (unit, &rep_st->match_data, &rep_st->match_mask,
             rep_st->flags & BCM_L2_REPLACE_NO_CALLBACKS ?
             SOC_L2X_NO_CALLBACKS : 0);

        SOC_L2_DEL_SYNC_UNLOCK(soc);
    }

    return rv;
}

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_tr_l2_sw_dump
 * Purpose:
 *     Displays L2 information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_tr_l2_sw_dump(int unit)
{
    _bcm_mac_block_info_t *mbi;
     char                 pfmt[SOC_PBMP_FMT_LEN];
     int                  i;

    soc_cm_print("\n");
    soc_cm_print("  TR L2 MAC Blocking Info -\n");
    soc_cm_print("      Number : %d\n", _mbi_num[unit]);

    mbi = _mbi_entries[unit];
    soc_cm_print("      Entries (index: pbmp-count) :\n");
    if (mbi != NULL) {
        for (i = 0; i < _mbi_num[unit]; i++) {
            SOC_PBMP_FMT(mbi[i].mb_pbmp, pfmt);
            soc_cm_print("          %5d: %s-%d\n", i, pfmt, mbi[i].ref_count);
        }
    }
    soc_cm_print("\n");

    soc_cm_print("\n  TR L2 PPA bypass - %s\n",
                 SOC_CONTROL(unit)->l2x_ppa_bypass ? "TRUE" : "FALSE");
    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#else /* BCM_TRX_SUPPORT */
int bcm_esw_triumph_l2_not_empty;
#endif  /* BCM_TRX_SUPPORT */

