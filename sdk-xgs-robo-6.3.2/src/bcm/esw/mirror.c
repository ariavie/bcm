/*
 * $Id: mirror.c 1.353 Broadcom SDK $
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
 * Mirror - Broadcom StrataSwitch Mirror API.
 *
 * The mirroring code has become more complex after the introduction
 * of XGS3 devices, which support multiple MTPs (mirror-to ports) as
 * well as directed mirroring. When directed mirroring is enabled
 * it is also possible to mirror to a trunk.
 *
 * Non-directed mirroring (aka. Draco1.5-style mirroring and XGS2-style 
 * mirroring) only allows for a single MTP in a system (which can be
 * either a single device or a stack.) In order to mirror a packet to 
 * a remote module in non-directed mode, the local MTP must be the 
 * appropriate stacking port and all modules traversed from the 
 * mirrored port to the MTP need to have mirroring configured to 
 * reflect the desired path for mirror packets.
 *
 * Directed mirroring means that the MTP info includes a module ID,
 * which allows mirror packets to follow the normal path of switched
 * packets, i.e. when mirroring to a remote MTP there is no need to 
 * configure the mirror path separately.
 *
 * Since the original mirror API did not support module IDs in the MTP
 * definition, a new API was introduced to handle this. The new API is
 * called bcm_mirror_port_set/get and allows the application to 
 * configure mirroring with a single API call, whereas the the old API
 * would require two (and in most cases three or more) API calls.
 *
 * For compatibility, the original API will also work on XGS3 devices,
 * and in this case the MTP module ID is automatically set to be the
 * local module ID. Likewise, the new API will also work on pre-XGS3
 * devices as long as the MTP module ID is specified as the local
 * module ID.
 *
 * In addition to normal ingress and egress mirroring, the FP (field
 * processor) can specify actions that include ingress and egress 
 * mirroring. This feature uses the same hardware MTP resources as
 * the mirror API, so in order to coordinate this, the FP APIs must
 * allocate MTP resources through internal reserve/unreserve 
 * functions. Since multiple FP rules can use the same MTP resource
 * the reserve/unreserve functions maintain a reference count for
 * each MTP resource. In the software MTP structure this reference
 * counter is called 'reserved'. Within the same structure, the 
 * mirror API uses the 'pbmp' to indicate whether this MTP resource
 * is being used by the mirror API.
 *
 * Note that the MTP resource management code allows resources to be 
 * shared between the mirror API and the FP whenever the requested 
 * MTP is identical.
 *
 */
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/debug.h>
#ifdef BCM_TRIDENT_SUPPORT
#include <soc/profile_mem.h>
#endif /* BCM_TRIDENT_SUPPORT */

#include <bcm/error.h>
#include <bcm/mirror.h>
#include <bcm/port.h>
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/mirror.h>
#include <bcm_int/esw/trunk.h>
#include <bcm_int/esw/stack.h>
#ifdef BCM_WARM_BOOT_SUPPORT
#include <bcm_int/common/field.h>
#include <bcm_int/esw/switch.h>
#endif /* BCM_WARM_BOOT_SUPPORT */

#include <bcm_int/esw_dispatch.h>

#ifdef BCM_TRIUMPH2_SUPPORT
#include <bcm_int/esw/triumph2.h>
#endif
#ifdef BCM_TRIDENT_SUPPORT
#include <bcm_int/esw/trident.h>
#endif
#ifdef BCM_TRIUMPH3_SUPPORT
#include <bcm_int/esw/triumph3.h>
#endif
#ifdef BCM_TRIUMPH_SUPPORT
#include <bcm_int/esw/triumph.h>
#endif
#ifdef BCM_MPLS_SUPPORT
#include <bcm_int/esw/mpls.h>
#endif

/* local macros */
#define VP_PORT_INVALID  (-1)

/* STATIC FUNCTIONS DECLARATION. */
STATIC int _bcm_esw_mirror_enable(int unit);
STATIC int _bcm_esw_mirror_egress_set(int unit, bcm_port_t port, int enable);
STATIC int _bcm_esw_mirror_egress_get(int unit, bcm_port_t port, int *enable);
STATIC int _bcm_esw_mirror_enable_set(int unit, int port, int enable);
STATIC int _bcm_esw_directed_mirroring_get(int unit, int *enable);

/* LOCAL VARIABLES DECLARATION. */
static int _bcm_mirror_mtp_method_init[BCM_MAX_NUM_UNITS];
static _bcm_mirror_config_p _bcm_mirror_config[BCM_MAX_NUM_UNITS];

#ifdef BCM_TRIUMPH2_SUPPORT
static const soc_field_t _mtp_index_field[] = {
    MTP_INDEX0f, MTP_INDEX1f, MTP_INDEX2f, MTP_INDEX3f
};
static const soc_field_t _non_uc_mtp_index_field[] = {
    NON_UC_EM_MTP_INDEX0f, NON_UC_EM_MTP_INDEX1f, NON_UC_EM_MTP_INDEX2f,
    NON_UC_EM_MTP_INDEX3f
};
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
/* Cache of Egress Mirror Encap Profile Tables */
static soc_profile_mem_t *egr_mirror_encap_profile[BCM_MAX_NUM_UNITS] = {NULL};
#define EGR_MIRROR_ENCAP(_unit_) \
                (egr_mirror_encap_profile[_unit_])
#define EGR_MIRROR_ENCAP_PROFILE_DEFAULT  0

#define EGR_MIRROR_ENCAP_ENTRIES_CONTROL        0
#define EGR_MIRROR_ENCAP_ENTRIES_DATA_1         1
#define EGR_MIRROR_ENCAP_ENTRIES_DATA_2         2
#define EGR_MIRROR_ENCAP_ENTRIES_NUM            3

#endif /* BCM_TRIDENT_SUPPORT */

int
_bcm_esw_mirror_flexible_get(int unit, int *enable)
{
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        *enable = (_bcm_mirror_mtp_method_init[unit] ==
                   BCM_MIRROR_MTP_METHOD_DIRECTED_FLEXIBLE) ? TRUE : FALSE;

        return BCM_E_NONE;
    } else {
        return BCM_E_UNAVAIL;
    }
}

int
_bcm_esw_mirror_flexible_set(int unit, int enable)
{
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        if (enable) {
            _bcm_mirror_mtp_method_init[unit] =
                BCM_MIRROR_MTP_METHOD_DIRECTED_FLEXIBLE;
        } else if (soc_feature(unit, soc_feature_directed_mirror_only)) {
            _bcm_mirror_mtp_method_init[unit] =
                BCM_MIRROR_MTP_METHOD_DIRECTED_LOCKED;
        } else {
            _bcm_mirror_mtp_method_init[unit] =
                BCM_MIRROR_MTP_METHOD_NON_DIRECTED;
        }
        return BCM_E_NONE;
    } else {
        return BCM_E_UNAVAIL;
    }
}

/*
 * Function:
 *	  _bcm_esw_local_modid_get
 * Purpose:
 *	  Get local unit module id. 
 * Parameters:
 *    unit    - (IN) BCM device number.
 *    modid   - (OUT)module id. 
 * Returns:
 *	  BCM_E_XXX
 */
STATIC int
_bcm_esw_local_modid_get(int unit, int *modid)
{
    int  rv;      /* Operation return status. */

    /* Input parameters check. */
    if (NULL == modid) {
        return (BCM_E_PARAM);
    }

    /* Get local module id. */
    rv = bcm_esw_stk_my_modid_get(unit, modid);
    if ((BCM_E_UNAVAIL == rv) || (*modid < 0) ){
        *modid = 0;
        rv = (BCM_E_NONE);
    }
    return (rv);
}

/*
 * Function:
 *      _bcm_mirror_gport_adapt
 * Description:
 *      Adapts gport encoding for dual mode devices
 * Parameters:
 *      unit        - BCM device number
 *      gport       (IN/OUT)- gport to adapt
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_mirror_gport_adapt(int unit, bcm_gport_t *gport)
{
    bcm_module_t    modid;
    bcm_port_t      port;
    bcm_trunk_t     tgid;
    int             id;
    bcm_gport_t     gport_out;
    _bcm_gport_dest_t gport_st;

    if (NULL == gport) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(
        _bcm_esw_gport_resolve(unit, *gport, &modid, 
                               &port, &tgid, &id));

    /* Adaptation is needed only for dual mod devices */
    if ((NUM_MODID(unit) > 1)) {

        if (-1 != id) {
            return BCM_E_PARAM;
        }
    
        if (BCM_TRUNK_INVALID != tgid) {
            gport_st.gport_type = BCM_GPORT_TYPE_TRUNK;
            gport_st.tgid = tgid;
        } else {
            gport_st.gport_type = BCM_GPORT_TYPE_MODPORT;
            gport_st.modid = modid;
            gport_st.port = port;
        }
        BCM_IF_ERROR_RETURN(
            _bcm_esw_gport_construct(unit, &gport_st, &gport_out));
    
        *gport = gport_out;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_mirror_gport_resolve
 * Description:
 *      Resolves gport for mirror module for local ports
 * Parameters:
 *      unit        - (IN) BCM device number
 *      gport       - (IN)- gport to to resolve
 *      port        - (OUT)- port encoded to gport
 *      modid       - (OUT)- modid encoded to gport
 * Returns:
 *      BCM_E_XXX
 * Note :
 *      if modid == NULL port must be local port
 */
STATIC int 
_bcm_mirror_gport_resolve(int unit, bcm_gport_t gport, bcm_port_t *port, 
                          bcm_module_t *modid)
{
    bcm_module_t    lmodid;
    bcm_trunk_t     tgid;
    bcm_port_t      lport;
    int             id, islocal;

    if (NULL == port) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(
        _bcm_esw_gport_resolve(unit,gport, &lmodid, &lport, &tgid, &id));

    if (-1 != id || BCM_TRUNK_INVALID != tgid) {
        return BCM_E_PARAM;
    }

    if (NULL == modid) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_modid_is_local(unit, lmodid, &islocal));
        if (islocal != TRUE) {
            return BCM_E_PARAM;
        }
    } else {
        *modid = lmodid;
    }
    *port = lport;

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_mirror_gport_construct
 * Description:
 *      Constructs gport for mirror module
 * Parameters:
 *      unit        - (IN) BCM device number
 *      port_tgid   - (IN) port or trunk id to construct into a gprot
 *      modid       - (IN) module id to construct into a gport
 *      flags       - (IN) Mirror trunk flag
 *      gport       - (OUT)- gport to to construct
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_mirror_gport_construct(int unit, int port_tgid, int modid, uint32 flags, 
                            bcm_gport_t *gport)
{
    _bcm_gport_dest_t   dest;
    bcm_module_t        mymodid;
    int                 rv;

    _bcm_gport_dest_t_init(&dest);
    if (flags & BCM_MIRROR_PORT_DEST_TRUNK) {
        dest.tgid = port_tgid;
        dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
    } else {
        dest.port = port_tgid;
        if (IS_ST_PORT(unit, port_tgid)) {
            rv = bcm_esw_stk_my_modid_get(unit, &mymodid);
            if (BCM_E_UNAVAIL == rv) {
                dest.gport_type = _SHR_GPORT_TYPE_DEVPORT;
            } else {
                dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
                dest.modid = modid;
            }
        } else {
            dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
            dest.modid = modid;
        }
    }
    BCM_IF_ERROR_RETURN(
        _bcm_esw_gport_construct(unit, &dest, gport));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_mirror_destination_gport_parse
 * Description:
 *      Parse mirror destinations gport.
 * Parameters:
 *      unit      - BCM device number
 *      mirror_dest_id - mirror destination id. 
 *      dest_mod  - (OUT) module id of mirror-to port
 *      dest_port - (OUT) mirror-to port
 *      flags     - (OUT) Trunk flag
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_mirror_destination_gport_parse(int unit, bcm_gport_t mirror_dest_id,
                                    bcm_module_t *dest_mod, bcm_port_t *dest_port,
                                    uint32 *flags)
{
    bcm_mirror_destination_t mirror_dest;
    bcm_module_t             modid;
    bcm_port_t               port;
    bcm_trunk_t              tgid;
    int                      id;


    BCM_IF_ERROR_RETURN
        (bcm_esw_mirror_destination_get(unit, mirror_dest_id, &mirror_dest));

    BCM_IF_ERROR_RETURN(
        _bcm_esw_gport_resolve(unit, mirror_dest.gport, &modid, &port, 
                               &tgid, &id));

    if (-1 != id) {
        return BCM_E_PARAM;
    }

    if (BCM_TRUNK_INVALID != tgid) {
        if (NULL != dest_mod) {
            *dest_mod  = -1;
        }
        if (NULL != dest_port) { 
            *dest_port = tgid;
        }
        if (NULL != flags) {
            *flags |= BCM_MIRROR_PORT_DEST_TRUNK;
        }
    } else {
        if (NULL != dest_mod) {
            *dest_mod = modid;
        }
        if (NULL != dest_port) {
            *dest_port = port;
        }
    }

    return (BCM_E_NONE);
}

#ifdef BCM_TRIDENT_SUPPORT
/*
 * Function:
 *      _bcm_egr_mirror_encap_entry_add
 * Purpose:
 *      Internal function for adding an entry to the EGR_MIRROR_ENCAP_* tables
 *      Adds an entry to the global shared SW copy of the EGR_MIRROR_ENCAP_* 
 *      tables
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Pointer to EGR_MIRROR_ENCAP_* entries array
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_egr_mirror_encap_entry_add(int unit, void **entries, uint32 *index)
{
    return soc_profile_mem_add(unit, EGR_MIRROR_ENCAP(unit), entries,
                             1, index);
}

/*
 * Function:
 *      _bcm_egr_mirror_encap_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the EGR_MIRROR_ENCAP_*
 *      tables
 *      Deletes an entry from the global shared SW copy of the
 *      EGR_MIRROR_ENCAP_* tables
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_egr_mirror_encap_entry_delete(int unit, uint32 index) 
{
    return soc_profile_mem_delete(unit, EGR_MIRROR_ENCAP(unit), index);
}

/*
 * Function:
 *      _bcm_egr_mirror_encap_entry_reference
 * Purpose:
 *      Internal function for indicating that an entry in EGR_MIRROR_ENCAP_*
 *      tables is being used. Updates the global shared SW copy.
 * Parameters:
 *      unit    -  (IN) Device number
 *      index   -  (IN) Base index for the entry to be updated
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_egr_mirror_encap_entry_reference(int unit, uint32 index) 
{
    return soc_profile_mem_reference(unit, EGR_MIRROR_ENCAP(unit),
                                     (int) index, 1);
}

/*
 * Function:
 *      _bcm_egr_mirror_encap_entry_mtp_update
 * Purpose:
 *      Internal function for recording updating the EGR_*_MTP_INDEX
 *      tables when ERSPAN is activated.
 * Parameters:
 *      unit    -  (IN) Device number
 *      index   -  (IN) MTP index for the entries to be updated
 *      profile_index   -  (IN) Encap profile index
 *      flags   -  (IN) Mirror direction flags.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_egr_mirror_encap_entry_mtp_update(int unit, int index,
                                       uint32 profile_index, int flags) 
{
    int                         offset;
    int                         idx;
    int refs = 0;

    refs = 0;
    offset = index * BCM_SWITCH_TRUNK_MAX_PORTCNT;
    for (idx = 0; idx < BCM_SWITCH_TRUNK_MAX_PORTCNT; idx++, offset++) {
        if (flags & BCM_MIRROR_PORT_INGRESS) {
            BCM_IF_ERROR_RETURN
                (soc_mem_field32_modify(unit, EGR_IM_MTP_INDEXm,
                                        offset, MIRROR_ENCAP_INDEXf,
                                        profile_index));
            if (0 == idx) {
                refs++;
            }
        }

        if (flags & BCM_MIRROR_PORT_EGRESS) {
            BCM_IF_ERROR_RETURN
                (soc_mem_field32_modify(unit, EGR_EM_MTP_INDEXm,
                                        offset, MIRROR_ENCAP_INDEXf,
                                        profile_index));
            if (0 == idx) {
                refs++;
            }
        }

        if (soc_feature(unit, soc_feature_egr_mirror_true) &&
            (flags & BCM_MIRROR_PORT_EGRESS_TRUE)) {
            BCM_IF_ERROR_RETURN
                (soc_mem_field32_modify(unit, EGR_EP_REDIRECT_EM_MTP_INDEXm,
                                        offset, MIRROR_ENCAP_INDEXf,
                                        profile_index));
            if (0 == idx) {
                refs++;
            }
        }
    }

    if (refs > 1) {
        /* We should never have more than one mirror direction flag set. */
        return BCM_E_INTERNAL;
    }

    return (BCM_E_NONE);
}
#endif /* BCM_TRIDENT_SUPPORT */

/*
 * Function:
 *      _bcm_mirror_destination_match
 * Description:
 *      Limited match utility used to identify mirror destination
 *      with identical gport. 
 * Parameters:
 *      unit           - (IN) BCM device number
 *      mirror_dest    - (IN) Mirror destination. 
 *      mirror_dest_id - (OUT)Matching mirror destination id. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_mirror_destination_match(int unit, bcm_mirror_destination_t *mirror_dest,
                              bcm_gport_t *mirror_dest_id) 
                        
{
    int idx;                         /* Mirror destinations iteration index.*/
    _bcm_mirror_dest_config_p  mdest;/* Mirror destination description.     */
    bcm_module_t mymodid;            /* Local module id.              */
    int          isLocal;            /* Local modid indicator */
    bcm_module_t dest_mod;           /* Destination module id.        */
    bcm_port_t   dest_port;          /* Destination port number.      */
    _bcm_gport_dest_t gport_st;      /* Structure to construct a GPORT */

    /* Input parameters check. */
    if ((NULL == mirror_dest_id) || (NULL == mirror_dest)) {
        return (BCM_E_PARAM);
    }

    /* Get local modid. */
    BCM_IF_ERROR_RETURN(_bcm_esw_local_modid_get(unit, &mymodid));

    /* Directed  mirroring support check. */
    if (MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit) && !SOC_WARM_BOOT(unit)){
        /* NB:  If Warm Boot, then the ports should already be mapped. */
        /* Set mirror destination to outgoing port on local module. */
        dest_mod = BCM_GPORT_MODPORT_MODID_GET(mirror_dest->gport);
        BCM_IF_ERROR_RETURN(_bcm_esw_modid_is_local(unit, dest_mod, &isLocal));
        if (FALSE == isLocal) {
            BCM_IF_ERROR_RETURN
                (bcm_esw_topo_port_get(unit, dest_mod, &dest_port));
            gport_st.gport_type = BCM_GPORT_TYPE_MODPORT;
            gport_st.modid = mymodid;
            gport_st.port = dest_port;
            BCM_IF_ERROR_RETURN(
                _bcm_esw_gport_construct(unit,&gport_st, 
                                         &(mirror_dest->gport)));
        }
    }

    /* Find unused mirror destination & allocate it. */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->dest_count; idx++) {
        mdest = MIRROR_CONFIG(unit)->dest_arr + idx;
        /* Skip unused entries. */
        if (0 == mdest->ref_count) {
            continue;
        }

        /* Skip tunnel destinations. */
        if (mdest->mirror_dest.flags & BCM_MIRROR_DEST_TUNNELS) { 
            continue;
        }

        if (mdest->mirror_dest.gport == mirror_dest->gport) {
            /* Matching mirror destination found. */
            *mirror_dest_id = mdest->mirror_dest.mirror_dest_id;
            return (BCM_E_NONE);
        }
    }
    return (BCM_E_NOT_FOUND);
}

/*
 * Function:
 *      _bcm_tr2_mirror_shared_mtp_match
 * Description:
 *      Match a mirror-to port with one of the mtp indexes.
 * Parameters:
 *      unit        - (IN)  BCM device number
 *      dest_port   - (IN)  Mirror destination gport.
 *      egress      - (IN) Egress/Ingress indication
 *      match_idx   - (OUT) MTP index matching destination. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_shared_mtp_match(int unit, bcm_gport_t gport, 
                                 int egress, int *match_idx)
{
    int idx;                                 /* Mtp iteration index. */

    /* Input parameters check. */
    if (NULL == match_idx) {
        return (BCM_E_PARAM);
    }

    /* Look for existing MTP in use */
    for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) {
        if (MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx)) {
            if ((gport == MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx)) &&
                egress == MIRROR_CONFIG_SHARED_MTP(unit, idx).egress) {
                *match_idx = idx;
                return (BCM_E_NONE);
            }
        } 
    }
    return (BCM_E_NOT_FOUND);
}


/*
 * Function:
 *      _bcm_esw_mirror_ingress_mtp_match
 * Description:
 *      Match a mirror-to port with one of the mtp indexes.
 * Parameters:
 *      unit        - (IN)  BCM device number
 *      dest_port   - (IN)  Mirror destination gport.
 *      match_idx   - (OUT) MTP index matching destination. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_mirror_ingress_mtp_match(int unit, bcm_gport_t gport, int *match_idx)
{
    int idx;                                 /* Mtp iteration index. */

    /* Input parameters check. */
    if (NULL == match_idx) {
        return (BCM_E_PARAM);
    }

    /* Look for existing MTP in use */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->ing_mtp_count; idx++) {
        if (MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, idx)) {
            if (gport == MIRROR_CONFIG_ING_MTP_DEST(unit, idx)) {
                *match_idx = idx;
                return (BCM_E_NONE);
            }
        } 
    }
    return (BCM_E_NOT_FOUND);
}

/*
 * Function:
 *      _bcm_esw_mirror_egress_mtp_match
 * Description:
 *      Match a mirror-to port with one of the mtp indexes.
 * Parameters:
 *      unit        - (IN)  BCM device number
 *      dest_port   - (IN)  Mirror destination gport.
 *      match_idx   - (OUT) MTP index matching destination. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_mirror_egress_mtp_match(int unit, bcm_gport_t gport, int *match_idx)
{
    int idx;                                 /* Mtp iteration index. */

    /* Input parameters check. */
    if (NULL == match_idx) {
        return (BCM_E_PARAM);
    }

    /* Look for existing MTP in use */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_mtp_count; idx++) {
        if (MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx)) {
            if (gport == MIRROR_CONFIG_EGR_MTP_DEST(unit, idx)) {
                *match_idx = idx;
                return (BCM_E_NONE);
            }
        }
    }
    return (BCM_E_NOT_FOUND);
}

#ifdef BCM_TRIUMPH2_SUPPORT
/*
 * Function:
 *      _bcm_esw_mirror_egress_true_mtp_match
 * Description:
 *      Match a mirror-to port with one of the mtp indexes.
 * Parameters:
 *      unit        - (IN)  BCM device number
 *      dest_port   - (IN)  Mirror destination gport.
 *      match_idx   - (OUT) MTP index matching destination. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_mirror_egress_true_mtp_match(int unit, bcm_gport_t gport,
                                      int *match_idx)
{
    int idx;                                 /* Mtp iteration index. */

    /* Input parameters check. */
    if (NULL == match_idx) {
        return (BCM_E_PARAM);
    }

    /* Look for existing MTP in use */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_true_mtp_count; idx++) {
        if (MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, idx)) {
            if (gport == MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, idx)) {
                *match_idx = idx;
                return (BCM_E_NONE);
            }
        }
    }
    return (BCM_E_NOT_FOUND);
}
#endif /* BCM_TRIUMPH2_SUPPORT */

/*
 * Function:
 *      _bcm_tr2_mirror_vp_port_get
 * Description:
 *      Get the virtual port number and the ingress gport associated
 *      with the vp.
 * Parameters:
 *      unit        - (IN)  BCM device number
 *      gport       - (IN)  vp gport
 *      vp_out      - (OUT) virtual port number
 *      port_out    - (OUT) ingress gport associated with the vp
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_vp_port_get(int unit, bcm_gport_t gport,
                                      int *vp_out, int *port_out)
{
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    int vp;
    bcm_gport_t phy_port_trunk;

    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        bcm_vlan_port_t vlan_vp;

        vp = BCM_GPORT_VLAN_PORT_ID_GET(gport);
        /* Get the physical port or trunk the VP resides on */
        bcm_vlan_port_t_init(&vlan_vp);
        vlan_vp.vlan_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_tr2_vlan_vp_find(unit, &vlan_vp));
        phy_port_trunk = vlan_vp.port;
        *port_out = phy_port_trunk;
        *vp_out = vp;
        return BCM_E_NONE;
    } else
#ifdef BCM_TRIDENT_SUPPORT
    if (BCM_GPORT_IS_NIV_PORT(gport)) {
        bcm_niv_port_t niv_port;

        vp = BCM_GPORT_NIV_PORT_ID_GET(gport);

        /* Get the physical port or trunk the VP resides on */
        bcm_niv_port_t_init(&niv_port);
        niv_port.niv_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_trident_niv_port_get(unit, &niv_port));
        if (niv_port.flags & BCM_NIV_PORT_MATCH_NONE) {
            return BCM_E_PARAM;
        }
        phy_port_trunk = niv_port.port;
        *port_out = phy_port_trunk;
        *vp_out = vp;
        return BCM_E_NONE;
    } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        bcm_extender_port_t extender_port;

        vp = BCM_GPORT_EXTENDER_PORT_ID_GET(gport);

        /* Get the physical port or trunk the VP resides on */
        bcm_extender_port_t_init(&extender_port);
        extender_port.extender_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_tr3_extender_port_get(unit, &extender_port));
        phy_port_trunk = extender_port.port;
        *port_out = phy_port_trunk;
        *vp_out = vp;
        return BCM_E_NONE;
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_MPLS_SUPPORT
    if (BCM_GPORT_IS_MPLS_PORT(gport)) {
        bcm_mpls_port_t mpls_port;
        int vpn0;
 
        vp = BCM_GPORT_MPLS_PORT_ID_GET(gport);
        bcm_mpls_port_t_init(&mpls_port);
        mpls_port.mpls_port_id = gport;
        _BCM_MPLS_VPN_SET(vpn0,_BCM_MPLS_VPN_TYPE_VPWS,0);
        BCM_IF_ERROR_RETURN(bcm_tr_mpls_port_get(unit, 
                      vpn0, &mpls_port));
        phy_port_trunk = mpls_port.port;
        *port_out = phy_port_trunk;
        *vp_out = vp;
        return BCM_E_NONE;
    } else
#endif  /* BCM_MPLS_SUPPORT */
    {}

#endif  /* defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3) */
    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *      _bcm_tr2_mirror_svp_enable_get
 * Description:
 *      check if the ingress mirroring is enabled on the given vp.
 * Parameters:
 *      unit        - (IN)  BCM device number
 *      vp          - (IN)  virtual port number 
 *      enable      - (OUT) enable status 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_svp_enable_get(int unit, int vp,
                                       int *enable)
{
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        source_vp_entry_t svp_entry;

        sal_memset(&svp_entry, 0, sizeof(source_vp_entry_t));
        BCM_IF_ERROR_RETURN(READ_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, 
                                &svp_entry));
        *enable = soc_SOURCE_VPm_field32_get(unit, &svp_entry, 
                                ING_MIRROR_ENABLEf);
        return BCM_E_NONE;
    }
#endif
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_tr2_mirror_svp_enable_set
 * Description:
 *      Enable the ingress mirroring on the given vp.
 * Parameters:
 *      unit        - (IN)  BCM device number
 *      vp          - (IN)  virtual port number 
 *      enable      - (IN)  enable the given MTP 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_svp_enable_set(int unit, int vp,
                                       int enable)
{
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        source_vp_entry_t svp_entry;

        sal_memset(&svp_entry, 0, sizeof(source_vp_entry_t));
        BCM_IF_ERROR_RETURN(READ_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, 
                      &svp_entry));
        soc_SOURCE_VPm_field32_set(unit, &svp_entry, 
                      ING_MIRROR_ENABLEf, enable);
        BCM_IF_ERROR_RETURN(WRITE_SOURCE_VPm(unit, 
                      MEM_BLOCK_ALL, vp, &svp_entry));
        return BCM_E_NONE;
    }
#endif
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_tr2_mirror_dvp_enable_get
 * Description:
 *      check if the egress mirroring is enabled on the given vp.
 * Parameters:
 *      unit        - (IN)  BCM device number
 *      vp          - (IN)  virtual port number
 *      enable      - (OUT) enable status 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_dvp_enable_get(int unit, int vp,
                                       int *enable)
{
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        ing_dvp_table_entry_t dvp_entry;

        sal_memset(&dvp_entry, 0, sizeof(ing_dvp_table_entry_t));
        BCM_IF_ERROR_RETURN(READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, 
                  vp, &dvp_entry));
        *enable = soc_ING_DVP_TABLEm_field32_get(unit, &dvp_entry, 
                           EGR_MIRROR_ENABLEf);
        return BCM_E_NONE;
    }
#endif
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_tr2_mirror_dvp_enable_set
 * Description:
 *      enable the egress mirroring on the given vp.
 * Parameters:
 *      unit        - (IN)  BCM device number
 *      gport       - (IN)  virtual port number 
 *      enable      - (IN)  enable the given MTP
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_dvp_enable_set(int unit, int vp,
                                       int enable)
{
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        ing_dvp_table_entry_t dvp_entry;

        sal_memset(&dvp_entry, 0, sizeof(ing_dvp_table_entry_t));
        BCM_IF_ERROR_RETURN(READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, 
                 vp, &dvp_entry));
        soc_ING_DVP_TABLEm_field32_set(unit, &dvp_entry, EGR_MIRROR_ENABLEf, 
                   enable);
        BCM_IF_ERROR_RETURN(WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, 
                   vp, &dvp_entry));
        return BCM_E_NONE;
    }
#endif
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *	  _bcm_esw_mirror_destination_find
 * Purpose:
 *	  Find mirror destination for all gport types. 
 * Parameters:
 *    unit    - (IN) BCM device number.
 *    port    - (IN) port, gport or mirror gport
 *    modid   - (IN) module id.
 *    flags   - (IN) BCM_MIRROR_PORT_DEST_* flags 
 *    mirror_dest - (OUT) mirror destination 
 * Returns:
 *	  BCM_E_XXX
 */

STATIC int 
_bcm_esw_mirror_destination_find(int unit, bcm_port_t port, bcm_module_t modid, 
                                 uint32 flags, bcm_mirror_destination_t *mirror_dest)
{
    if (NULL == mirror_dest) {
        return BCM_E_PARAM;
    }

    if (BCM_GPORT_IS_SET(port)) {
        /* If gport passed, work with it directly */
        mirror_dest->gport = port;
    } else {
        _bcm_gport_dest_t gport_st;      /* Structure to construct a GPORT */

        /* If not gport then construct the gport from given parameters.*/
        if (flags & BCM_MIRROR_PORT_DEST_TRUNK) {
            /* Mirror destination is a trunk. */
            gport_st.gport_type = BCM_GPORT_TYPE_TRUNK;
            gport_st.tgid = port;
        } else {
            /* Convert port + mod to GPORT format. No trunking destination support. */
            if (-1 == modid) { 
                /* Get local modid. */
                BCM_IF_ERROR_RETURN(
                    _bcm_esw_local_modid_get(unit, &modid));
            } else if (!SOC_MODID_ADDRESSABLE(unit, modid)){
                return BCM_E_PARAM;
            }

            gport_st.gport_type = BCM_GPORT_TYPE_MODPORT;
            gport_st.modid = modid;
            gport_st.port = port;
        }
        BCM_IF_ERROR_RETURN(
            _bcm_esw_gport_construct(unit, &gport_st, &(mirror_dest->gport)));
    }

    /* Adapt miror destination gport */
    BCM_IF_ERROR_RETURN(
        _bcm_mirror_gport_adapt(unit, &(mirror_dest->gport)));

    /* Find matching mirror destination */
    BCM_IF_ERROR_RETURN(
        _bcm_mirror_destination_match(unit, mirror_dest,
                                      &(mirror_dest->mirror_dest_id)));
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_mirror_destination_alloc
 * Purpose:
 *     Allocate mirror destination description.
 * Parameters:
 *      unit           - (IN) BCM device number. 
 *      mirror_dest_id - (OUT) Mirror destination id.
 * Returns:
 *      BCM_X_XXX
 */

/* Create a mirror (destination, encapsulation) pair. */
STATIC int 
_bcm_mirror_destination_alloc(int unit, bcm_gport_t *mirror_dest_id) 
{
    int idx;                          /* Mirror destinations iteration index.*/
    _bcm_mirror_dest_config_p  mdest; /* Mirror destination description.     */

    /* Input parameters check. */
    if (NULL == mirror_dest_id) {
        return (BCM_E_PARAM);
    }

    /* Find unused mirror destination & allocate it. */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->dest_count; idx++) {
        mdest = MIRROR_CONFIG(unit)->dest_arr + idx;
        if (mdest->ref_count) {
            continue;
        }
        mdest->ref_count++;
        *mirror_dest_id = mdest->mirror_dest.mirror_dest_id;
        return (BCM_E_NONE);
    }

    /* All mirror destinations are used. */
    return (BCM_E_RESOURCE);
}

/*
 * Function:
 *     _bcm_mirror_destination_free
 * Purpose:
 *     Free mirror destination description.
 * Parameters:
 *      unit           - (IN) BCM device number. 
 *      mirror_dest_id - (IN) Mirror destination id.
 * Returns:
 *      BCM_X_XXX
 */

/* Create a mirror (destination, encapsulation) pair. */
STATIC int 
_bcm_mirror_destination_free(int unit, bcm_gport_t mirror_dest_id) 
{
    _bcm_mirror_dest_config_p  mdest_cfg; /* Mirror destination config.*/

    mdest_cfg = &MIRROR_DEST_CONFIG(unit, mirror_dest_id); 

    if (mdest_cfg->ref_count > 0) {
        mdest_cfg->ref_count--;

        if (0 == mdest_cfg->ref_count) {
            sal_memset(&mdest_cfg->mirror_dest, 0,
                       sizeof(bcm_mirror_destination_t));
            mdest_cfg->mirror_dest.mirror_dest_id = mirror_dest_id;
            mdest_cfg->mirror_dest.gport = BCM_GPORT_INVALID;
        }
    } else {
        return BCM_E_NOT_FOUND;
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_esw_mirror_destination_create
 * Purpose:
 *     Helper function to API that creates mirror destination description.
 * Parameters:
 *      unit         - (IN) BCM device number. 
 *      mirror_dest  - (IN) Mirror destination description.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int 
_bcm_esw_mirror_destination_create(int unit,
                                   bcm_mirror_destination_t *mirror_dest)
{
    if (mirror_dest->flags & BCM_MIRROR_DEST_WITH_ID) {
        /* Check mirror destination id */
        if ((0 == BCM_GPORT_IS_MIRROR(mirror_dest->mirror_dest_id)) || 
           (BCM_GPORT_MIRROR_GET(mirror_dest->mirror_dest_id) >=
                                    MIRROR_DEST_CONFIG_COUNT(unit))) {
            return (BCM_E_BADID);
        }

        /* Check if mirror destination is being updated  */
        if (0 != MIRROR_DEST_REF_COUNT(unit, mirror_dest->mirror_dest_id)) { 
            if (0 == (mirror_dest->flags & BCM_MIRROR_DEST_REPLACE)) {
                return (BCM_E_EXISTS);
            }
        } else {
            MIRROR_DEST_REF_COUNT(unit, mirror_dest->mirror_dest_id) = 1;
        }
    } else {
        /* Allocate new mirror destination. */
        BCM_IF_ERROR_RETURN(
            _bcm_mirror_destination_alloc(unit, &mirror_dest->mirror_dest_id));
    }

    /* Set mirror destination configuration. */
    
    *(MIRROR_DEST(unit, mirror_dest->mirror_dest_id)) = *mirror_dest; 
    (MIRROR_DEST(unit, mirror_dest->mirror_dest_id))->flags &=
        (_BCM_MIRROR_DESTINATION_LOCAL | BCM_MIRROR_DEST_TUNNELS | BCM_MIRROR_DEST_INT_PRI_SET); 

    return (BCM_E_NONE);
}

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_0


#ifdef BCM_TRX_SUPPORT
STATIC int
_bcm_esw_mirror_mtp_entry_trunk_get(int unit, void *mtp_entry,
                                    bcm_gport_t *port)
{
    bcm_module_t tp_mod;
    bcm_port_t tp_port;
    bcm_gport_t tgid;
    int member_count, idx, rv = BCM_E_NONE;

    /* We're working with IM_MTP_INDEXm, but it is equivalent for
     * all of the .._MTP_INDEXm tables. */
    if (soc_mem_field_valid(unit, IM_MTP_INDEXm, TGIDf)) {
        tgid = soc_IM_MTP_INDEXm_field32_get(unit, mtp_entry, TGIDf);
    } else {
        if (0 == soc_IM_MTP_INDEXm_field32_get(unit, mtp_entry, RTAGf)) {
            /* No ports in trunk, so the trunk id is cached */
            if (soc_mem_field_valid(unit, IM_MTP_INDEXm, PORT_NUM_7f)) {
                tgid = soc_IM_MTP_INDEXm_field32_get(unit, mtp_entry,
                                                     PORT_NUM_7f);
            } else {
                /* This case should not occur. */
                return BCM_E_INTERNAL;
            }
        } else {

            /* Trunk in place, must work backward from mod/port */
            member_count =
                soc_IM_MTP_INDEXm_field32_get(unit, mtp_entry, COUNTf);
                    
            for (idx = 0; idx < member_count; idx++) {
                tp_port = soc_IM_MTP_INDEXm_field32_get(unit, mtp_entry,
                                                        PORT_TGIDf);
                tp_mod = soc_IM_MTP_INDEXm_field32_get(unit, mtp_entry,
                                                       MODULE_IDf);
 
                rv = _bcm_esw_trunk_port_property_get(unit, tp_mod,
                                                      tp_port, &tgid);
                if (BCM_SUCCESS(rv) && (tgid != -1)) {
                    break;
                } else if (BCM_E_NOT_FOUND != rv) {
                    /* Access error */
                    return rv;
                } /* Else keep looking */
            }

            if (idx == member_count) {
                /* Could not determine original trunk */
                return BCM_E_NOT_FOUND;
            }
        }
    }

    BCM_IF_ERROR_RETURN 
        (_bcm_mirror_gport_construct(unit, tgid, 0,
                                     BCM_MIRROR_PORT_DEST_TRUNK, port));
    return BCM_E_NONE;
}
#endif /* BCM_TRX_SUPPORT */

int
_bcm_esw_mirror_mtp_to_modport(int unit, int mtp_index, int modport,
                                int flags, bcm_module_t *modid,
                                bcm_gport_t *port)
{
    im_mtp_index_entry_t im_mtp;
    em_mtp_index_entry_t em_mtp;

    if (0 != (flags & BCM_MIRROR_PORT_INGRESS)) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_read(unit, IM_MTP_INDEXm, MEM_BLOCK_ALL,
                          mtp_index, &im_mtp));
#ifdef BCM_TRX_SUPPORT
        if (soc_mem_field_valid(unit, IM_MTP_INDEXm, Tf)) {
            if (0 != soc_IM_MTP_INDEXm_field32_get(unit, &im_mtp, Tf)) {
                BCM_IF_ERROR_RETURN 
                    (_bcm_esw_mirror_mtp_entry_trunk_get(unit, &im_mtp,
                                                         port));
                *modid = 0;
            } else {
                *port = soc_IM_MTP_INDEXm_field32_get(unit, &im_mtp,
                                                      PORT_NUMf);
                *modid = soc_IM_MTP_INDEXm_field32_get(unit, &im_mtp,
                                                       MODULE_IDf);
            }
        } else
#endif /* BCM_TRX_SUPPORT */
        {
            *port =
                soc_IM_MTP_INDEXm_field32_get(unit, &im_mtp, PORT_TGIDf);
            *modid =
                soc_IM_MTP_INDEXm_field32_get(unit, &im_mtp, MODULE_IDf);
        }
    } else if (0 != (flags & BCM_MIRROR_PORT_EGRESS)) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_read(unit, EM_MTP_INDEXm, MEM_BLOCK_ALL,
                          mtp_index, &em_mtp));
#ifdef BCM_TRX_SUPPORT
        if (soc_mem_field_valid(unit, EM_MTP_INDEXm, Tf)) {
            if (0 != soc_EM_MTP_INDEXm_field32_get(unit, &em_mtp, Tf)) {
                BCM_IF_ERROR_RETURN 
                    (_bcm_esw_mirror_mtp_entry_trunk_get(unit, &em_mtp,
                                                         port));
                *modid = 0;
            } else {
                *port = soc_EM_MTP_INDEXm_field32_get(unit, &em_mtp,
                                                      PORT_NUMf);
                *modid = soc_EM_MTP_INDEXm_field32_get(unit, &em_mtp,
                                                       MODULE_IDf);
            }
        } else
#endif /* BCM_TRX_SUPPORT */
        {
            *port =
                soc_EM_MTP_INDEXm_field32_get(unit, &em_mtp, PORT_TGIDf);
            *modid =
                soc_EM_MTP_INDEXm_field32_get(unit, &em_mtp, MODULE_IDf);
        }
#ifdef BCM_TRIUMPH2_SUPPORT
    } else if (soc_feature(unit, soc_feature_egr_mirror_true) &&
               (0 != (flags & BCM_MIRROR_PORT_EGRESS_TRUE))) {
        ep_redirect_em_mtp_index_entry_t epm_mtp;

        BCM_IF_ERROR_RETURN 
            (soc_mem_read(unit, EP_REDIRECT_EM_MTP_INDEXm,
                          MEM_BLOCK_ALL, mtp_index, &epm_mtp));
        if (0 != soc_EP_REDIRECT_EM_MTP_INDEXm_field32_get(unit,
                                                           &epm_mtp, Tf)) {
            BCM_IF_ERROR_RETURN 
                (_bcm_esw_mirror_mtp_entry_trunk_get(unit, &epm_mtp,
                                                         port));
            *modid = 0;
        } else {
            *port = soc_EP_REDIRECT_EM_MTP_INDEXm_field32_get(unit, &epm_mtp,
                                                              PORT_NUMf);
            *modid =
                soc_EP_REDIRECT_EM_MTP_INDEXm_field32_get(unit, &epm_mtp,
                                                          MODULE_IDf);
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
    } else {
        return BCM_E_PARAM;
    }

    if (!BCM_GPORT_IS_TRUNK(*port)) {
        if (modport) {
            /* Put into modport gport format */
            BCM_IF_ERROR_RETURN 
                (_bcm_mirror_gport_construct(unit, *port, *modid, 0, port));
        } else {
            /* Translate into normalized (modport, port) form */
            BCM_IF_ERROR_RETURN
                (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                         *modid, *port, 
                                         modid, port));
        }
    }

    return BCM_E_NONE;
}

#ifdef BCM_FIELD_SUPPORT
/*
 * Function:
 *  	_bcm_esw_mirror_field_group_reload 
 * Purpose:
 *  	Used as a callback routine to traverse over field groups 
 * Parameters:
 *	    unit        - (IN) BCM device number.
 *      group       - (IN) Group id
 *      user_data   - (IN) User data pointer
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_esw_mirror_field_group_reload(int unit, bcm_field_group_t group, 
                                   void *user_data)
{
    int entry_count, entry_num;
    int alloc_sz, rv = BCM_E_NONE, flags, idx;
    bcm_field_entry_t *entry_array, entry_id;
    bcm_mirror_destination_t  mirror_dest;
    bcm_gport_t gport;
    uint32 param0, param1;
    bcm_field_qset_t group_qset;

    BCM_IF_ERROR_RETURN
        (bcm_esw_field_entry_multi_get(unit, group, 0, NULL, &entry_num));
    if (entry_num == 0) {
        /* In Level 1 the FP may detect extra groups if a slice if the group 
           PBMP is not ALL. These will come across as dummy groups with 
           no entries. */
        return BCM_E_NONE;
    }
    
    alloc_sz = sizeof(bcm_field_entry_t) * entry_num;
    entry_array = sal_alloc(alloc_sz, "Field IDs");
    if (NULL == entry_array) {
        return BCM_E_MEMORY;
    }
    sal_memset(entry_array, 0, alloc_sz);
        
    rv = bcm_esw_field_entry_multi_get(unit, group, entry_num,
                                       entry_array, &entry_count);
    if (BCM_FAILURE(rv)) {
        sal_free(entry_array);
        return rv;
    }
    if (entry_count != entry_num) {
        /* Why didn't we get the number of ID's we were told existed? */
        sal_free(entry_array);
        return BCM_E_INTERNAL;
    }

    for (entry_count = 0; entry_count < entry_num; entry_count++) {
        entry_id = entry_array[entry_count];
        rv = bcm_esw_field_action_get(unit, entry_id,
                                      bcmFieldActionMirrorIngress,
                                      &param0, &param1);
        if (BCM_SUCCESS(rv)) {
            gport = param1;
            if (!BCM_GPORT_IS_SET(gport)) {
                rv = _bcm_mirror_gport_construct(unit, param1, param0,
                                                 0, &gport);
                if (BCM_FAILURE(rv)) {
                    break;
                }
            }
            flags = BCM_MIRROR_PORT_INGRESS;
            bcm_mirror_destination_t_init(&mirror_dest);
            rv = _bcm_esw_mirror_destination_find(unit, gport, 0,
                                                  flags, &mirror_dest); 
            if (BCM_E_NOT_FOUND == rv) {
                rv = BCM_E_INTERNAL;
                /* Should have recovered the destination already. */
            }
            if (BCM_FAILURE(rv)) {
                break;
            }
            if (soc_feature(unit, soc_feature_mirror_flexible)) {
                for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) {
                    if (mirror_dest.mirror_dest_id ==
                        MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx) && 
                        !MIRROR_CONFIG_SHARED_MTP(unit, idx).egress) {
                        break;
                    }
                }
                if (idx < BCM_MIRROR_MTP_COUNT) {
                    MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx) +=
                        (1 << BCM_MIRROR_MTP_REF_FP_OFFSET);
                    MIRROR_DEST_REF_COUNT(unit,
                           MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx))++;
                    /* If we found a mirror action, then mirroring is enabled */
                    MIRROR_CONFIG_MODE(unit) = BCM_MIRROR_L2;
                } else {
                    rv = BCM_E_INTERNAL;
                    break;
                }
            } else {
                for (idx = 0; idx < MIRROR_CONFIG(unit)->ing_mtp_count; idx++) {
                    if (mirror_dest.mirror_dest_id ==
                        MIRROR_CONFIG_ING_MTP_DEST(unit, idx)) {
                        break;
                    }
                }
                if (idx < MIRROR_CONFIG(unit)->ing_mtp_count) {
                    MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, idx)++;
                    MIRROR_DEST_REF_COUNT(unit,
                           MIRROR_CONFIG_ING_MTP_DEST(unit, idx))++;
                    /* If we found a mirror action, then mirroring is enabled */
                    MIRROR_CONFIG_MODE(unit) = BCM_MIRROR_L2;
                } else {
                    rv = BCM_E_INTERNAL;
                    break;
                }
            }
        } else if (rv != BCM_E_NOT_FOUND) {
            break;
        } else {
            FP_VERB(("Mirror module reload, ignore FP error report\n"));
        }

        rv = bcm_esw_field_action_get(unit, entry_id,
                                      bcmFieldActionMirrorEgress,
                                      &param0, &param1);
        if (BCM_SUCCESS(rv)) {
            gport = param1;
            if (!BCM_GPORT_IS_SET(gport)) {
                rv = _bcm_mirror_gport_construct(unit, param1, param0,
                                                 0, &gport);
                if (BCM_FAILURE(rv)) {
                    break;
                }
            }

            /* Initialize the qset */
            BCM_FIELD_QSET_INIT(group_qset);

            /* Get the group Qset info */
            rv = bcm_esw_field_group_get(unit, group, &group_qset);
            if (BCM_FAILURE(rv)) {
                break;
            }

            /* Check if Mirroring is set for Egress Stage */
            if (BCM_FIELD_QSET_TEST(group_qset, bcmFieldQualifyStageEgress)) {
                flags = BCM_MIRROR_PORT_EGRESS_TRUE;
            } else {
                flags = BCM_MIRROR_PORT_EGRESS;
            }

            bcm_mirror_destination_t_init(&mirror_dest);
            rv = _bcm_esw_mirror_destination_find(unit, gport, 0,
                                                  flags, &mirror_dest); 
            if (BCM_E_NOT_FOUND == rv) {
                rv = BCM_E_INTERNAL;
                /* Should have recovered the destination already. */
            }
            if (BCM_FAILURE(rv)) {
                break;
            }

            if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
                for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) {
                    if ((mirror_dest.mirror_dest_id
                        == MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, idx))) {
                        break;
                    }
                }
                if (idx < BCM_MIRROR_MTP_COUNT) {
                    MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, idx)++;
                    MIRROR_DEST_REF_COUNT(unit, mirror_dest.mirror_dest_id)++;
                    /* If we found a mirror action, then mirroring is enabled */
                    MIRROR_CONFIG_MODE(unit) = BCM_MIRROR_L2;
                } else {
                    rv = BCM_E_INTERNAL;
                    break;
                }
            } else if (soc_feature(unit, soc_feature_mirror_flexible)) {
                for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) {
                    if (mirror_dest.mirror_dest_id ==
                        MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx) && 
                        MIRROR_CONFIG_SHARED_MTP(unit, idx).egress) {
                        break;
                    }
                }
                if (idx < BCM_MIRROR_MTP_COUNT) {
                    MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx) +=
                        (1 << BCM_MIRROR_MTP_REF_FP_OFFSET);
                    MIRROR_DEST_REF_COUNT(unit,
                           MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx))++;
                    /* If we found a mirror action, then mirroring is enabled */
                    MIRROR_CONFIG_MODE(unit) = BCM_MIRROR_L2;
                } else {
                    rv = BCM_E_INTERNAL;
                    break;
                }
            } else {
                for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_mtp_count; idx++) {
                    if (mirror_dest.mirror_dest_id ==
                        MIRROR_CONFIG_EGR_MTP_DEST(unit, idx)) {
                        break;
                    }
                }
                if (idx < MIRROR_CONFIG(unit)->egr_mtp_count) {
                    MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx)++;
                    MIRROR_DEST_REF_COUNT(unit,
                           MIRROR_CONFIG_EGR_MTP_DEST(unit, idx))++;
                    /* If we found a mirror action, then mirroring is enabled */
                    MIRROR_CONFIG_MODE(unit) = BCM_MIRROR_L2;
                } else {
                    rv = BCM_E_INTERNAL;
                    break;
                }
            }
        } else if (rv != BCM_E_NOT_FOUND) {
            break;
        } else {
            FP_VERB(("Mirror module reload, ignore FP error report\n"));
        }
    }

    if (BCM_E_NOT_FOUND == rv) {
        rv = BCM_E_NONE; /* Do not propagate this error */
    }

    sal_free(entry_array);
    return rv;
}
#endif /* BCM_FIELD_SUPPORT */


#ifdef BCM_XGS12_FABRIC_SUPPORT
STATIC int
_bcm_xgs12_fabric_mirror_reinit(int unit)
{
    uint32 mirbmap;
    pbmp_t pbmp;
    bcm_port_t port;
  	 
    PBMP_HG_ITER(unit, port) {
        SOC_IF_ERROR_RETURN(READ_ING_MIRTOBMAPr(unit, port, &mirbmap));
        SOC_PBMP_CLEAR(pbmp);
        SOC_PBMP_WORD_SET(pbmp, 0, mirbmap);
        if (SOC_PBMP_NOT_NULL(pbmp)) {
            MIRROR_CONFIG(unit)->mode = BCM_MIRROR_L2;
            break;
        }
    }

    /*
     * Cannot recover MTP info for 5675
     */

    return BCM_E_NONE;
}
#endif /* BCM_XGS12_FABRIC_SUPPORT */

#if defined(BCM_TRIDENT_SUPPORT)
/*
 * Function:
 *  	_bcm_td_mirror_tunnel_reload 
 * Purpose:
 *  	Restores mirroring tunnel destination encap info
 *      for warm boot recovery
 * Parameters:
 *	unit        - (IN) BCM device number.
 *      mirror_dest  - (IN) Mirror destination description.
 *      profile_index   -  (IN) Encap profile index
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_td_mirror_tunnel_reload(int unit, bcm_gport_t dest_id,
                             uint32 profile_index)
{
    bcm_mirror_destination_t *mirror_dest;
    egr_mirror_encap_control_entry_t control_entry;
    egr_mirror_encap_data_1_entry_t data_1_entry;
    egr_mirror_encap_data_2_entry_t data_2_entry;
    void *entries[EGR_MIRROR_ENCAP_ENTRIES_NUM];
    int optional_header;
    uint32 hw_buffer[_BCM_TD_MIRROR_V4_GRE_BUFFER_SZ]; /* Max size needed */

    entries[EGR_MIRROR_ENCAP_ENTRIES_CONTROL] = &control_entry;
    entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_1] = &data_1_entry;
    entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_2] = &data_2_entry;
    
    /* Tunnel type? */
    BCM_IF_ERROR_RETURN
        (soc_profile_mem_get(unit, EGR_MIRROR_ENCAP(unit),
                             profile_index, 1, entries));

    mirror_dest = MIRROR_DEST(unit, dest_id);

    optional_header =
        soc_EGR_MIRROR_ENCAP_CONTROLm_field32_get(unit, &control_entry,
                                      RSPAN__ADD_OPTIONAL_HEADERf);

    if (soc_EGR_MIRROR_ENCAP_CONTROLm_field32_get(unit, &control_entry,
                       ENTRY_TYPEf) == BCM_TD_MIRROR_ENCAP_TYPE_ERSPAN) {
        /* ERSPAN recovery */
        soc_mem_mac_addr_get(unit, EGR_MIRROR_ENCAP_DATA_1m,
                             &data_1_entry, ERSPAN__ERSPAN_HEADER_DAf,
                             mirror_dest->dst_mac);
        soc_mem_mac_addr_get(unit, EGR_MIRROR_ENCAP_DATA_1m,
                             &data_1_entry, ERSPAN__ERSPAN_HEADER_SAf,
                             mirror_dest->src_mac);

        mirror_dest->gre_protocol =
            soc_EGR_MIRROR_ENCAP_DATA_1m_field32_get(unit, &data_1_entry,
                                      ERSPAN__ERSPAN_HEADER_GREf);

        hw_buffer[0] =
            soc_EGR_MIRROR_ENCAP_DATA_1m_field32_get(unit, &data_1_entry,
                                      ERSPAN__ERSPAN_HEADER_VLAN_TAGf);

        mirror_dest->vlan_id = (bcm_vlan_t) (hw_buffer[0] & 0xffff);
        mirror_dest->tpid = (uint16) ((hw_buffer[0] >> 16) & 0xffff);

        soc_EGR_MIRROR_ENCAP_DATA_1m_field_get(unit, &data_1_entry,
                         ERSPAN__ERSPAN_HEADER_V4f, hw_buffer);
        /* See _bcm_trident_mirror_ipv4_gre_tunnel_set for the encoding */
        mirror_dest->version = 4;
        mirror_dest->dst_addr = hw_buffer[0];
        mirror_dest->src_addr = hw_buffer[1];
        mirror_dest->ttl = (uint8) ((hw_buffer[2] >> 24) & 0xff);
        mirror_dest->tos = (uint8) ((hw_buffer[4] >> 16) & 0xff);

        mirror_dest->flags |= BCM_MIRROR_DEST_TUNNEL_IP_GRE;
    } else {
        /* RSPAN recovery */
        hw_buffer[0] =
            soc_EGR_MIRROR_ENCAP_DATA_1m_field32_get(unit, &data_1_entry,
                                                 RSPAN__RSPAN_VLAN_TAGf);

        mirror_dest->vlan_id = (bcm_vlan_t) (hw_buffer[0] & 0xffff);
        mirror_dest->tpid = (uint16) ((hw_buffer[0] >> 16) & 0xffff);

        mirror_dest->flags |= BCM_MIRROR_DEST_TUNNEL_L2;
    }

    if (BCM_TD_MIRROR_HEADER_TRILL == optional_header) {
        /* TRILL recovery */
        soc_EGR_MIRROR_ENCAP_DATA_2m_field_get(unit, &data_2_entry,
                                               HEADER_DATAf, hw_buffer);

        mirror_dest->trill_dst_name =
            (hw_buffer[0] >> BCM_TD_MIRROR_TRILL_DEST_NAME_OFFSET) &
            _BCM_TD_MIRROR_TRILL_NAME_MASK;
        mirror_dest->trill_src_name =
            (hw_buffer[1] & _BCM_TD_MIRROR_TRILL_NAME_MASK);
        mirror_dest->trill_hopcount =
            ((hw_buffer[1] >> BCM_TD_MIRROR_TRILL_HOPCOUNT_OFFSET) &
             _BCM_TD_MIRROR_TRILL_HOPCOUNT_MASK);

        mirror_dest->flags |= BCM_MIRROR_DEST_TUNNEL_TRILL;
    } else if (BCM_TD_MIRROR_HEADER_VNTAG == optional_header) {
        /* NIV recovery */
        hw_buffer[0] =
            soc_EGR_MIRROR_ENCAP_DATA_2m_field32_get(unit, &data_2_entry,
                                                     VNTAG_HEADERf);

        if (0 != (hw_buffer[0] & _BCM_TD_MIRROR_NIV_LOOP_BIT)) {
            mirror_dest->niv_flags = BCM_MIRROR_NIV_LOOP;
        }

        mirror_dest->niv_src_vif =
            (hw_buffer[0] & _BCM_TD_MIRROR_NIV_SRC_VIF_MASK);
        mirror_dest->niv_dst_vif =
            ((hw_buffer[0] >> _BCM_TD_MIRROR_NIV_DST_VIF_OFFSET) &
             _BCM_TD_MIRROR_NIV_DST_VIF_MASK);

        mirror_dest->flags |= BCM_MIRROR_DEST_TUNNEL_NIV;
    } /* Else no additional header to recover */

    return BCM_E_NONE;
}
#endif /* BCM_TRIDENT_SUPPORT */

/*
 * Function:
 *  	_bcm_esw_mirror_reload 
 * Purpose:
 *  	Restores mirroring destination for warm boot recovery
 * Parameters:
 *	    unit        - (IN) BCM device number.
 *      directed    - (IN) indication if directed mirroring is used.
 * Returns:
 *  	BCM_E_XXX
 */

STATIC int
_bcm_esw_mirror_reload(int unit, int directed)
{
    soc_scache_handle_t       scache_handle;
    uint8                     *mtp_scache, *mtp_scache_p;
    int                       mc_enable, enable, enabled = FALSE;
    int	                      idx, port_ix, flags, rv;
    bcm_module_t              modid = 0;
    bcm_gport_t               gport;
    bcm_gport_t               mtp_gport[3 * BCM_MIRROR_MTP_COUNT];
    /* Max MTP * mirror types (ING, EGR, TRUE EGR) */
    bcm_mirror_destination_t  mirror_dest;
    uint32                    reg_val = 0;
    int                       stale_scache = FALSE;
#ifdef BCM_TRIUMPH2_SUPPORT
    uint32                    ms_reg; /* MTP mode register value     */
    int                       mtp_type = 0, mtp_index;
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
    mirror_control_entry_t mc_entry; /* MTP control memory value */
#endif /* BCM_TRIDENT_SUPPORT */


#ifdef BCM_XGS12_FABRIC_SUPPORT
    if (SOC_IS_XGS12_FABRIC(unit)) {
        return _bcm_xgs12_fabric_mirror_reinit(unit);
    }
#endif /* BCM_XGS12_FABRIC_SUPPORT */

    if (SOC_IS_XGS3_SWITCH(unit)) {
        PBMP_ALL_ITER(unit, port_ix) {
            /* Higig port should never drop directed mirror packets
               so setting is always enabled and need not to be considered here*/
            if (IS_ST_PORT(unit, port_ix)) {
                continue;
            }
#if defined(BCM_TRIDENT_SUPPORT)
            if (soc_feature(unit, soc_feature_mirror_control_mem)) {
                BCM_IF_ERROR_RETURN
                    (READ_MIRROR_CONTROLm(unit, MEM_BLOCK_ANY,
                                          port_ix, &mc_entry));
                mc_enable = soc_MIRROR_CONTROLm_field32_get(unit, &mc_entry,
                                                             M_ENABLEf);
            } else
#endif /* BCM_TRIDENT_SUPPORT */
            {
                BCM_IF_ERROR_RETURN
                    (READ_MIRROR_CONTROLr(unit, port_ix, &reg_val));
                mc_enable = soc_reg_field_get(unit, MIRROR_CONTROLr,
                                              reg_val, M_ENABLEf);
            }
            if (mc_enable) {
                enabled = TRUE;
                break;
            }
        }
        
        MIRROR_CONFIG_MODE(unit) =
            enabled ? BCM_MIRROR_L2 : BCM_MIRROR_DISABLE;
    }    

    /* Recover stored destination gports, if available */
    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_MIRROR, 0);
    rv = _bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 0, &mtp_scache, BCM_WB_DEFAULT_VERSION, NULL);

    if (BCM_E_NOT_FOUND == rv) {
        mtp_scache = NULL;
    } else if (BCM_FAILURE(rv)) {
        soc_cm_print("mtp_scache error \n");
        return rv;
    } else {
        mtp_scache_p = mtp_scache;
        if (soc_feature(unit, soc_feature_mirror_flexible)) {
            for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) {
                sal_memcpy(&(mtp_gport[idx]),
                           mtp_scache_p,
                           sizeof(bcm_gport_t));
                mtp_scache_p += sizeof(bcm_gport_t);
            }
        } else {
            for (idx = 0; idx < MIRROR_CONFIG(unit)->ing_mtp_count; idx++) {
                sal_memcpy(&(mtp_gport[idx]),
                           mtp_scache_p,
                           sizeof(bcm_gport_t));
                mtp_scache_p += sizeof(bcm_gport_t);
            }

            for (idx = 0;
                 idx < MIRROR_CONFIG(unit)->egr_mtp_count; idx++) {
                sal_memcpy(&(mtp_gport[idx + BCM_MIRROR_MTP_COUNT]),
                           mtp_scache_p,
                           sizeof(bcm_gport_t));
                mtp_scache_p += sizeof(bcm_gport_t);
            }
        }
#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_feature(unit, soc_feature_egr_mirror_true)) {
            for (idx = 0;
                 idx < MIRROR_CONFIG(unit)->egr_true_mtp_count; idx++) {
                sal_memcpy(&(mtp_gport[idx + (2 * BCM_MIRROR_MTP_COUNT)]),
                           mtp_scache_p,
                           sizeof(bcm_gport_t));
                mtp_scache_p += sizeof(bcm_gport_t);
            }
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        BCM_IF_ERROR_RETURN(READ_MIRROR_SELECTr(unit, &ms_reg));
        mtp_type = soc_reg_field_get(unit, MIRROR_SELECTr, ms_reg, MTP_TYPEf);
        /* ing_mtp_count works for both ingress and egress in shared mode */
        for (idx = 0; idx < MIRROR_CONFIG(unit)->ing_mtp_count; idx++) {
            flags = (mtp_type & (1 << idx)) ? BCM_MIRROR_PORT_EGRESS : BCM_MIRROR_PORT_INGRESS;
            BCM_IF_ERROR_RETURN 
                (_bcm_esw_mirror_mtp_to_modport(unit, idx, TRUE, flags, 
                                                &modid, &gport));
            bcm_mirror_destination_t_init(&mirror_dest);
            rv = _bcm_esw_mirror_destination_find(unit, gport, 0,
                                                  flags, &mirror_dest); 
            if (BCM_E_NOT_FOUND == rv) {
                if ((NULL != mtp_scache) && !stale_scache) {
                    if (BCM_GPORT_IS_MIRROR(mtp_gport[idx])) {
                        /* We have scratch memory of the destination IDs */
                        mirror_dest.mirror_dest_id = mtp_gport[idx];
                        mirror_dest.flags |= BCM_MIRROR_DEST_WITH_ID;
                        BCM_IF_ERROR_RETURN 
                            (_bcm_esw_mirror_destination_create(unit,
                                                                &mirror_dest));
                    } /* Else, we know there isn't an MTP here */
                } else {
                    BCM_IF_ERROR_RETURN 
                        (_bcm_esw_mirror_destination_create(unit, &mirror_dest));
                }
            } else if (BCM_FAILURE(rv)) {
                return rv;
            } else if ((NULL != mtp_scache) && !stale_scache &&
                       (BCM_GPORT_IS_MIRROR(mtp_gport[idx])) &&
                       (mirror_dest.mirror_dest_id != mtp_gport[idx])) {
                /* Warm Boot Level 2, the destination doesn't match! */
                SOC_IF_ERROR_RETURN
                    (soc_event_generate(unit, SOC_SWITCH_EVENT_STABLE_ERROR, 
                                        SOC_STABLE_STALE, 0, 0));
                stale_scache = TRUE;
            }
            if (BCM_GPORT_IS_MIRROR(mirror_dest.mirror_dest_id)) {
                MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx) =
                    mirror_dest.mirror_dest_id;
                if (!directed) {
                    MIRROR_CONFIG_SHARED_MTP(unit, idx).egress = FALSE;
                    MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx)++;
                    /* Egress update */
                    MIRROR_CONFIG_SHARED_MTP_DEST(unit,
                           BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX) =
                        mirror_dest.mirror_dest_id;
                    MIRROR_CONFIG_SHARED_MTP(unit,
                        BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX).egress = TRUE;
                    MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit,
                                  BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX)++;
                } else {
                    MIRROR_CONFIG_SHARED_MTP(unit, idx).egress =
                        (0 != (flags & BCM_MIRROR_PORT_EGRESS));
                }
            }
        }
    } else
#endif /* BCM_TRIUMPH2_SUPPORT */
    {
        for (idx = 0; idx < MIRROR_CONFIG(unit)->ing_mtp_count; idx++) {
            BCM_IF_ERROR_RETURN 
                (_bcm_esw_mirror_mtp_to_modport(unit, idx, TRUE,
                                 BCM_MIRROR_PORT_INGRESS, &modid, &gport));
            flags = BCM_MIRROR_PORT_INGRESS;
            bcm_mirror_destination_t_init(&mirror_dest);
            rv = _bcm_esw_mirror_destination_find(unit, gport, 0,
                                                  flags, &mirror_dest); 
            if (BCM_E_NOT_FOUND == rv) {
                if ((NULL != mtp_scache) && !stale_scache) {
                    if (BCM_GPORT_IS_MIRROR(mtp_gport[idx])) {
                        /* We have scratch memory of the destination IDs */
                        mirror_dest.mirror_dest_id = mtp_gport[idx];
                        mirror_dest.flags |= BCM_MIRROR_DEST_WITH_ID;
                        BCM_IF_ERROR_RETURN 
                            (_bcm_esw_mirror_destination_create(unit,
                                                                &mirror_dest));
                    } /* Else, we know there isn't an MTP here */
                } else {
                    BCM_IF_ERROR_RETURN 
                        (_bcm_esw_mirror_destination_create(unit, &mirror_dest));
                }
            } else if (BCM_FAILURE(rv)) {
                return rv;
            } else if ((NULL != mtp_scache) && !stale_scache &&
                       (BCM_GPORT_IS_MIRROR(mtp_gport[idx])) &&
                       (mirror_dest.mirror_dest_id != mtp_gport[idx])) {
                /* Warm Boot Level 2, the destination doesn't match! */
                SOC_IF_ERROR_RETURN
                    (soc_event_generate(unit, SOC_SWITCH_EVENT_STABLE_ERROR, 
                                        SOC_STABLE_STALE, 0, 0));
                stale_scache = TRUE;
            }
            if (BCM_GPORT_IS_MIRROR(mirror_dest.mirror_dest_id)) {
                MIRROR_CONFIG_ING_MTP_DEST(unit, idx) =
                    mirror_dest.mirror_dest_id;
                if (!directed) {
                    MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, idx)++;
                }
            }
        }

        for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_mtp_count; idx++) {
            BCM_IF_ERROR_RETURN 
                (_bcm_esw_mirror_mtp_to_modport(unit, idx, TRUE,
                                 BCM_MIRROR_PORT_EGRESS, &modid, &gport));
            flags = BCM_MIRROR_PORT_EGRESS;
            bcm_mirror_destination_t_init(&mirror_dest);
            rv = _bcm_esw_mirror_destination_find(unit, gport, 0,
                                                  flags, &mirror_dest); 
            if (BCM_E_NOT_FOUND == rv) {
                if ((NULL != mtp_scache) && !stale_scache) {
                    if (BCM_GPORT_IS_MIRROR(mtp_gport[idx +
                                                      BCM_MIRROR_MTP_COUNT])) {
                        /* We have scratch memory of the destination IDs */
                        mirror_dest.mirror_dest_id = mtp_gport[idx +
                                           BCM_MIRROR_MTP_COUNT];
                        mirror_dest.flags |= BCM_MIRROR_DEST_WITH_ID;
                        BCM_IF_ERROR_RETURN 
                            (_bcm_esw_mirror_destination_create(unit,
                                                                &mirror_dest));
                    } /* Else, we know there isn't an MTP here */
                } else {
                    BCM_IF_ERROR_RETURN 
                        (_bcm_esw_mirror_destination_create(unit, &mirror_dest));
                }
            } else if (BCM_FAILURE(rv)) {
                return rv;
            } else if ((NULL != mtp_scache) && !stale_scache &&
                       (BCM_GPORT_IS_MIRROR(mtp_gport[idx +
                                     BCM_MIRROR_MTP_COUNT])) &&
                       (mirror_dest.mirror_dest_id != mtp_gport[idx +
                                    BCM_MIRROR_MTP_COUNT])) {
                /* Warm Boot Level 2, the destination doesn't match! */
                SOC_IF_ERROR_RETURN
                    (soc_event_generate(unit, SOC_SWITCH_EVENT_STABLE_ERROR, 
                                        SOC_STABLE_STALE, 0, 0));
                stale_scache = TRUE;
            }

            if (BCM_GPORT_IS_MIRROR(mirror_dest.mirror_dest_id)) {
                MIRROR_CONFIG_EGR_MTP_DEST(unit, idx) =
                    mirror_dest.mirror_dest_id;
                if (!directed) {
                    MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx)++;
                }
            }
        }
    }
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_egr_mirror_true) && directed) {
        for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_true_mtp_count; idx++) {
            BCM_IF_ERROR_RETURN 
                (_bcm_esw_mirror_mtp_to_modport(unit, idx, TRUE,
                          BCM_MIRROR_PORT_EGRESS_TRUE, &modid, &gport));
            flags = BCM_MIRROR_PORT_EGRESS_TRUE;
            bcm_mirror_destination_t_init(&mirror_dest);
            rv = _bcm_esw_mirror_destination_find(unit, gport, 0,
                                                  flags, &mirror_dest); 
            if (BCM_E_NOT_FOUND == rv) {
                if ((NULL != mtp_scache) && !stale_scache) {
                    if (BCM_GPORT_IS_MIRROR(mtp_gport[idx +
                                                (2 * BCM_MIRROR_MTP_COUNT)])) {
                        /* We have scratch memory of the destination IDs */
                        mirror_dest.mirror_dest_id =
                            mtp_gport[idx + (2 * BCM_MIRROR_MTP_COUNT)];
                        mirror_dest.flags |= BCM_MIRROR_DEST_WITH_ID;
                        BCM_IF_ERROR_RETURN 
                            (_bcm_esw_mirror_destination_create(unit,
                                                         &mirror_dest));
                    } /* Else, we know there isn't an MTP here */
                } else {
                    BCM_IF_ERROR_RETURN 
                        (_bcm_esw_mirror_destination_create(unit,
                                                            &mirror_dest));
                }
            } else if (BCM_FAILURE(rv)) {
                return rv;
            } else if ((NULL != mtp_scache) && !stale_scache &&
                       (BCM_GPORT_IS_MIRROR(mtp_gport[idx +
                                           (2 * BCM_MIRROR_MTP_COUNT)])) &&
                       (mirror_dest.mirror_dest_id !=
                        mtp_gport[idx + (2 * BCM_MIRROR_MTP_COUNT)])) {
                /* Warm Boot Level 2, the destination doesn't match! */
                SOC_IF_ERROR_RETURN
                    (soc_event_generate(unit, SOC_SWITCH_EVENT_STABLE_ERROR, 
                                        SOC_STABLE_STALE, 0, 0));
                stale_scache = TRUE;
            }

            if (BCM_GPORT_IS_MIRROR(mirror_dest.mirror_dest_id)) {
                MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, idx) =
                    mirror_dest.mirror_dest_id;
                if (!directed) {
                    MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, idx)++;
                }
            }
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    if (directed) {
#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_feature(unit, soc_feature_mirror_flexible)) {
            BCM_IF_ERROR_RETURN(READ_MIRROR_SELECTr(unit, &ms_reg));
            mtp_type = soc_reg_field_get(unit, MIRROR_SELECTr, ms_reg, MTP_TYPEf);
        }
#endif /* BCM_TRIUMPH2_SUPPORT */

        PBMP_ALL_ITER(unit, port_ix) {
#if defined(BCM_TRIDENT_SUPPORT)
            if (soc_feature(unit, soc_feature_mirror_control_mem)) {
                BCM_IF_ERROR_RETURN
                    (READ_MIRROR_CONTROLm(unit, MEM_BLOCK_ANY,
                                          port_ix, &mc_entry));
                mc_enable = soc_MIRROR_CONTROLm_field32_get(unit, &mc_entry,
                                                             M_ENABLEf);
            } else
#endif /* BCM_TRIDENT_SUPPORT */
            {
                BCM_IF_ERROR_RETURN
                    (READ_MIRROR_CONTROLr(unit, port_ix, &reg_val));
                mc_enable = soc_reg_field_get(unit, MIRROR_CONTROLr,
                                              reg_val, M_ENABLEf);
            }
            if (mc_enable) {
#ifdef BCM_TRIUMPH2_SUPPORT
                if (soc_feature(unit, soc_feature_mirror_flexible)) {
                    /* Read ingress mtp enable bitmap for source port. */
                    BCM_IF_ERROR_RETURN
                        (_bcm_port_mirror_enable_get(unit, port_ix, &enable));
                    for (mtp_index = 0; mtp_index < BCM_MIRROR_MTP_COUNT; 
                          mtp_index++) {
                        if (enable & (1 << mtp_index)) {
                            /* Used slot get MTP index*/
#if defined(BCM_TRIDENT_SUPPORT)
                            if (soc_feature(unit,
                                    soc_feature_mirror_control_mem)) {
                                idx = soc_MIRROR_CONTROLm_field32_get(unit,
                                                       &mc_entry,
                                           _mtp_index_field[mtp_index]);
                            } else
#endif /* BCM_TRIDENT_SUPPORT */
                            {
                                idx = soc_reg_field_get(unit,
                                            MIRROR_CONTROLr, reg_val,
                                            _mtp_index_field[mtp_index]);
                            }
                            /* Ingress or egress? */
                            if (mtp_type & (1 << mtp_index)) {
                                /* Ingress mirroring was enabled, but type is
                                 * egress. */
                                return BCM_E_INTERNAL;
                            } else if (TRUE ==
                                MIRROR_CONFIG_SHARED_MTP(unit, idx).egress) {
                                /* Mismatched ingress/egress settings */
                                return BCM_E_INTERNAL;
                            } else {
                                /* Ingress */
                                MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx)++;
                                MIRROR_DEST_REF_COUNT(unit,
                                   MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx))++;
                            }
                        }
                    }
                    /* Read ingress mtp enable bitmap for source port. */
                    BCM_IF_ERROR_RETURN
                        (_bcm_esw_mirror_egress_get(unit, port_ix, &enable));
                    for (mtp_index = 0; mtp_index < BCM_MIRROR_MTP_COUNT; 
                          mtp_index++) {
                        if (enable & (1 << mtp_index)) {
                            /* Used slot get MTP index*/
#if defined(BCM_TRIDENT_SUPPORT)
                            if (soc_feature(unit,
                                    soc_feature_mirror_control_mem)) {
                                idx = soc_MIRROR_CONTROLm_field32_get(unit,
                                                       &mc_entry,
                                           _mtp_index_field[mtp_index]);
                            } else
#endif /* BCM_TRIDENT_SUPPORT */
                            {
                                idx = soc_reg_field_get(unit,
                                            MIRROR_CONTROLr, reg_val,
                                            _mtp_index_field[mtp_index]);
                            }
                            /* Ingress or egress? */
                            if (FALSE ==
                                MIRROR_CONFIG_SHARED_MTP(unit, idx).egress) {
                                /* Mismatched ingress/egress settings */
                                return BCM_E_INTERNAL;
                            } else if (mtp_type & (1 << mtp_index)) {
                                /* Egress */
                                MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx)++;
                                MIRROR_DEST_REF_COUNT(unit, 
                                    MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx))++;
                            } else {
                                /* Egress mirroring was enabled, but type is
                                 * ingress. */
                                return BCM_E_INTERNAL;
                            }
                        }
                    }
                } else 
#endif /* BCM_TRIUMPH2_SUPPORT */
                {
                    BCM_IF_ERROR_RETURN
                        (bcm_esw_mirror_ingress_get(unit, port_ix, &enable));
                    if (enable) {
                        idx = soc_reg_field_get(unit, MIRROR_CONTROLr,
                                                reg_val, IM_MTP_INDEXf);
                        MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, idx)++;
                        MIRROR_DEST_REF_COUNT(unit,
                               MIRROR_CONFIG_ING_MTP_DEST(unit, idx))++;
                    }
                    BCM_IF_ERROR_RETURN
                        (_bcm_esw_mirror_egress_get(unit, port_ix, &enable));
                    if (enable) {
                        idx = soc_reg_field_get(unit, MIRROR_CONTROLr,
                                                reg_val, EM_MTP_INDEXf);
                        MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx)++;
                        MIRROR_DEST_REF_COUNT(unit,
                               MIRROR_CONFIG_EGR_MTP_DEST(unit, idx))++;
                    }
                }
            }

#ifdef BCM_TRIUMPH2_SUPPORT
            if (soc_feature(unit, soc_feature_egr_mirror_true)) {
                BCM_IF_ERROR_RETURN
                    (_bcm_port_mirror_egress_true_enable_get(unit, port_ix,
                                                           &enable));
                for (mtp_index = 0; mtp_index < BCM_MIRROR_MTP_COUNT; 
                      mtp_index++) {
                    if (enable & (1 << mtp_index)) {
                        /* Egress true mirroring doesn't need mtp_slot
                         * remapping. */
                        MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit,
                                                             mtp_index)++;
                        MIRROR_DEST_REF_COUNT(unit,
                               MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit,
                                                               mtp_index))++;
                    }
                }
            }
#endif /* BCM_TRIUMPH2_SUPPORT */
        }

#ifdef BCM_TRIDENT_SUPPORT
        /* Recover EGR_MIRROR_ENCAP references from EGR_MTP &
         * EGR_PORT tables. */
        if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
            egr_im_mtp_index_entry_t egr_mtp_entry;
            egr_port_entry_t egr_port_entry;
            uint32 profile_index;
            int offset;

            /*
             * EGR_*_MTP_INDEX tables are arranged as blocks of
             * BCM_SWITCH_TRUNK_MAX_PORTCNT entries, with
             * BCM_MIRROR_MTP_COUNT sets.  We only need to check the
             * first entry of each block to see if the
             * encap enable is set.  We track one reference count
             * for each set, by each destination type (IM, EM, TRUE EM)
             */
            offset = 0;
            for (mtp_index = 0; mtp_index < BCM_MIRROR_MTP_COUNT; 
                 mtp_index++, offset += BCM_SWITCH_TRUNK_MAX_PORTCNT) {

                BCM_IF_ERROR_RETURN
                    (soc_mem_read(unit, EGR_IM_MTP_INDEXm, MEM_BLOCK_ANY, 
                                                 offset, &egr_mtp_entry));
                if (soc_mem_field32_get(unit, EGR_IM_MTP_INDEXm, 
                                        &egr_mtp_entry,
                                        MIRROR_ENCAP_ENABLEf)) {
                    profile_index =
                        soc_mem_field32_get(unit, EGR_IM_MTP_INDEXm, 
                                            &egr_mtp_entry,
                                            MIRROR_ENCAP_INDEXf);
                    BCM_IF_ERROR_RETURN
                        (_bcm_egr_mirror_encap_entry_reference(unit,
                                                     profile_index));

                    /* Tunnel type info recovery */
                    BCM_IF_ERROR_RETURN
                        (_bcm_td_mirror_tunnel_reload(unit,
                              MIRROR_CONFIG_SHARED_MTP_DEST(unit, mtp_index),
                                                      profile_index));
                }

                BCM_IF_ERROR_RETURN
                    (soc_mem_read(unit, EGR_EM_MTP_INDEXm, MEM_BLOCK_ANY, 
                                                 offset, &egr_mtp_entry));
                if (soc_mem_field32_get(unit, EGR_EM_MTP_INDEXm, 
                                        &egr_mtp_entry,
                                        MIRROR_ENCAP_ENABLEf)) {
                    profile_index =
                        soc_mem_field32_get(unit, EGR_EM_MTP_INDEXm, 
                                            &egr_mtp_entry,
                                            MIRROR_ENCAP_INDEXf);
                    BCM_IF_ERROR_RETURN
                        (_bcm_egr_mirror_encap_entry_reference(unit,
                                                     profile_index));

                    /* Tunnel type info recovery */
                    BCM_IF_ERROR_RETURN
                        (_bcm_td_mirror_tunnel_reload(unit,
                              MIRROR_CONFIG_SHARED_MTP_DEST(unit, mtp_index),
                                                      profile_index));
                }

                if (soc_feature(unit, soc_feature_egr_mirror_true)) {
                    BCM_IF_ERROR_RETURN
                        (soc_mem_read(unit, EGR_EP_REDIRECT_EM_MTP_INDEXm,
                                      MEM_BLOCK_ANY, offset, &egr_mtp_entry));
                    if (soc_mem_field32_get(unit, EGR_EP_REDIRECT_EM_MTP_INDEXm,
                                            &egr_mtp_entry,
                                            MIRROR_ENCAP_ENABLEf)) {
                        profile_index =
                            soc_mem_field32_get(unit,
                                                EGR_EP_REDIRECT_EM_MTP_INDEXm,
                                                &egr_mtp_entry,
                                                MIRROR_ENCAP_INDEXf);
                        BCM_IF_ERROR_RETURN
                            (_bcm_egr_mirror_encap_entry_reference(unit,
                                                             profile_index));

                        /* Tunnel type info recovery */
                        BCM_IF_ERROR_RETURN
                            (_bcm_td_mirror_tunnel_reload(unit,
                              MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, mtp_index),
                                                          profile_index));
                    }
                }
            }

            PBMP_ALL_ITER(unit, port_ix) {
                BCM_IF_ERROR_RETURN
                    (READ_EGR_PORTm(unit, MEM_BLOCK_ANY, port_ix,
                                    &egr_port_entry));

                if (0 != soc_EGR_PORTm_field32_get(unit, &egr_port_entry,
                                                   MIRROR_ENCAP_ENABLEf)) {
                    profile_index =
                        soc_EGR_PORTm_field32_get(unit, &egr_port_entry,
                                                  MIRROR_ENCAP_INDEXf);
                    BCM_IF_ERROR_RETURN
                        (_bcm_egr_mirror_encap_entry_reference(unit,
                                                     profile_index));
                }
            }
        }
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_FIELD_SUPPORT 
        rv = bcm_esw_field_group_traverse(unit,
                                          _bcm_esw_mirror_field_group_reload,
                                          NULL);
#endif /* BCM_FIELD_SUPPORT */
        if (BCM_FAILURE(rv) && (BCM_E_INIT != rv)) {
            return rv;
        }

        if (NULL == mtp_scache) {
            /* Cleanup unused null destination in Warm Boot Level 1 */
            flags = 0;
            BCM_IF_ERROR_RETURN 
                (_bcm_mirror_gport_construct(unit, 0, 0, 0, &gport));
            rv = _bcm_esw_mirror_destination_find(unit, gport, 0,
                                                  flags, &mirror_dest);
            if (BCM_E_NOT_FOUND == rv) {
                /* Nothing to do */
            } else if (BCM_FAILURE(rv)) {
                return rv;
            } else if (MIRROR_DEST_REF_COUNT(unit,
                                             mirror_dest.mirror_dest_id) == 1) {
                BCM_IF_ERROR_RETURN 
                    (bcm_esw_mirror_destination_destroy(unit,
                                                mirror_dest.mirror_dest_id));
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *  	_bcm_esw_mirror_sync 
 * Purpose:
 *  	Stores mirroring destination for warm boot recovery
 * Parameters:
 *	    unit        - (IN)BCM device number.
 * Returns:
 *  	BCM_E_XXX
 */

int
_bcm_esw_mirror_sync(int unit)
{
    soc_scache_handle_t scache_handle;
    uint8               *mtp_scache;
    int idx;

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_MIRROR, 0);
    BCM_IF_ERROR_RETURN
        (_bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 0, &mtp_scache, BCM_WB_DEFAULT_VERSION, NULL));

    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) {
            sal_memcpy(mtp_scache,
                       &(MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx)),
                         sizeof(bcm_gport_t));
            mtp_scache += sizeof(bcm_gport_t);
        }
    } else {
        for (idx = 0; idx < MIRROR_CONFIG(unit)->ing_mtp_count; idx++) {
            sal_memcpy(mtp_scache,
                       &(MIRROR_CONFIG_ING_MTP_DEST(unit, idx)),
                         sizeof(bcm_gport_t));
            mtp_scache += sizeof(bcm_gport_t);
        }

        for (idx = 0;
             idx < MIRROR_CONFIG(unit)->egr_mtp_count; idx++) {
            sal_memcpy(mtp_scache,
                       &(MIRROR_CONFIG_EGR_MTP_DEST(unit, idx)),
                         sizeof(bcm_gport_t));
            mtp_scache += sizeof(bcm_gport_t);
        }
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_egr_mirror_true)) {
        for (idx = 0;
             idx < MIRROR_CONFIG(unit)->egr_true_mtp_count; idx++) {
            sal_memcpy(mtp_scache,
                       &(MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, idx)),
                       sizeof(bcm_gport_t));
            mtp_scache += sizeof(bcm_gport_t);
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    return BCM_E_NONE;
}


#else
#define _bcm_esw_mirror_reload(unit, directed)    (BCM_E_NONE)
#endif /* BCM_WARM_BOOT_SUPPORT */

#if defined(BCM_TRIDENT_SUPPORT)
/*
 * Function:
 *  	_bcm_trident_mirror_egr_dest_get
 * Purpose:
 *  	Get destination port bitmap for egress mirroring.
 * Parameters:
 *      unit        - (IN) BCM device number.
 *	    port        - (IN) port number.
 *      mtp_index   - (IN) mtp index 
 *      dest_bitmap - (OUT) destination port bitmap.
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_trident_mirror_egr_dest_get(int unit, bcm_port_t port, int mtp_index,
                                 bcm_pbmp_t *dest_bitmap)
{
    emirror_control_entry_t entry;
    static const soc_mem_t mem[] = {
        EMIRROR_CONTROLm, EMIRROR_CONTROL1m,
        EMIRROR_CONTROL2m, EMIRROR_CONTROL3m
    };

    if (dest_bitmap == NULL) {
        return BCM_E_PARAM;
    }

    if ((mtp_index < 0) || (mtp_index >= BCM_MIRROR_MTP_COUNT)) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, mem[mtp_index], MEM_BLOCK_ANY, port, &entry));
    soc_mem_pbmp_field_get(unit, mem[mtp_index], &entry, BITMAPf, dest_bitmap);

    return BCM_E_NONE;
}

/*
 * Function:
 *  	_bcm_trident_mirror_egr_dest_set
 * Purpose:
 *  	Set destination port bitmap for egress mirroring.
 * Parameters:
 *      unit        - (IN) BCM device number.
 *	    port        - (IN) port number.
 *      mtp_index   - (IN) mtp index
 *      dest_bitmap - (IN) destination port bitmap.
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_trident_mirror_egr_dest_set(int unit, bcm_port_t port, int mtp_index,
                                 bcm_pbmp_t *dest_bitmap)
{
    emirror_control_entry_t entry;
    int cpu_hg_index = 0;

    static const soc_mem_t mem[] = {
        EMIRROR_CONTROLm, EMIRROR_CONTROL1m,
        EMIRROR_CONTROL2m, EMIRROR_CONTROL3m
    };

    if (dest_bitmap == NULL) {
        return BCM_E_PARAM;
    }
    if ((mtp_index < 0) || (mtp_index >= BCM_MIRROR_MTP_COUNT)) {
        return BCM_E_PARAM;
    }

    /* mtp_index is validated as an egress type previously */

    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, mem[mtp_index], MEM_BLOCK_ANY, port, &entry));
    soc_mem_pbmp_field_set(unit, mem[mtp_index], &entry,
                           BITMAPf, dest_bitmap);
    BCM_IF_ERROR_RETURN
        (soc_mem_write(unit, mem[mtp_index], MEM_BLOCK_ANY, port, &entry));

    /* Configure mirroring of CPU Higig packets as well */
    cpu_hg_index = SOC_IS_KATANA2(unit) ? SOC_INFO(unit).cpu_hg_pp_port_index :
                                          SOC_INFO(unit).cpu_hg_index;
    if (IS_CPU_PORT(unit, port) && cpu_hg_index != -1) {
        BCM_IF_ERROR_RETURN(soc_mem_write(unit, mem[mtp_index], MEM_BLOCK_ANY,
                                          cpu_hg_index, &entry));
    }

    return BCM_E_NONE;
}
#endif /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_TRIUMPH_SUPPORT)
/*
 * Function:
 *  	_bcm_triumph_mirror_egr_dest_get 
 * Purpose:
 *  	Get destination port bitmap for egress mirroring.
 * Parameters:
 *      unit        - (IN) BCM device number.
 *      port        - (IN) port number.
 *      mtp_index   - (IN) mtp index (mtp_slot for flex mirroring)
 *      dest_bitmap - (OUT) destination port bitmap.
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_triumph_mirror_egr_dest_get(int unit, bcm_port_t port, int mtp_index,
                                 bcm_pbmp_t *dest_bitmap)
{
    uint32 fval;
    uint64 mirror;               /* Egress mirror control reg value. */
    static const soc_reg_t reg[] = {
        EMIRROR_CONTROL_64r, EMIRROR_CONTROL1_64r,
        EMIRROR_CONTROL2_64r, EMIRROR_CONTROL3_64r
    };

    /* Input parameters check. */
    if (NULL == dest_bitmap) {
        return BCM_E_PARAM;
    }
    if ((mtp_index < 0) || (mtp_index >= BCM_MIRROR_MTP_COUNT)) {
        return BCM_E_PARAM;
    }

    /* mtp_index is validated as an egress type previously */

    BCM_IF_ERROR_RETURN(soc_reg_get(unit, reg[mtp_index], port, 0, &mirror));

    SOC_PBMP_CLEAR(*dest_bitmap);
    fval = soc_reg64_field32_get(unit, reg[mtp_index], mirror, BITMAP_LOf);
    SOC_PBMP_WORD_SET(*dest_bitmap, 0, fval);

    if (soc_reg_field_valid(unit, reg[mtp_index], BITMAP_HIf)) {
        fval = soc_reg64_field32_get(unit, reg[mtp_index], mirror, BITMAP_HIf);
        SOC_PBMP_WORD_SET(*dest_bitmap, 1, fval);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *  	_bcm_triumph_mirror_egr_dest_set 
 * Purpose:
 *  	Set destination port bitmap for egress mirroring.
 * Parameters:
 *      unit        - (IN) BCM device number.
 *      port        - (IN) Port number.
 *      mtp_index   - (IN) mtp slot number.
 *      dest_bitmap - (IN) Destination port bitmap.
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_triumph_mirror_egr_dest_set(int unit, bcm_port_t port, int mtp_index,
                                 bcm_pbmp_t *dest_bitmap)
{
    static const soc_reg_t reg[] = {
        EMIRROR_CONTROL_64r, EMIRROR_CONTROL1_64r,
        EMIRROR_CONTROL2_64r, EMIRROR_CONTROL3_64r
    };
    static const soc_reg_t hg_reg[] = {
        IEMIRROR_CONTROL_64r, IEMIRROR_CONTROL1_64r,
        IEMIRROR_CONTROL2_64r, IEMIRROR_CONTROL3_64r
    };
    uint32 values[2];
    soc_field_t fields[] = {BITMAP_LOf, BITMAP_HIf};
    int count;

    /* Input parameters check. */
    if (NULL == dest_bitmap) {
        return BCM_E_PARAM;
    }
    if ((mtp_index < 0) || (mtp_index >= BCM_MIRROR_MTP_COUNT)) {
        return BCM_E_PARAM;
    }
    
    /* mtp_index is validated as an egress type previously */
    if (mtp_index >= MIRROR_CONFIG(unit)->port_em_mtp_count) {
        /* Out of range */
        return BCM_E_PARAM;
    }

    values[0] = SOC_PBMP_WORD_GET(*dest_bitmap, 0);
    count = 1;
    if (soc_reg_field_valid(unit, reg[mtp_index], BITMAP_HIf)) {
        values[1] = SOC_PBMP_WORD_GET(*dest_bitmap, 1);
        count++;
    }

    BCM_IF_ERROR_RETURN 
        (soc_reg_fields32_modify(unit, reg[mtp_index], port, count,
                                 fields, values));

    /* Enable mirroring of CPU Higig packets as well */
    if (IS_CPU_PORT(unit, port)) {
        BCM_IF_ERROR_RETURN 
            (soc_reg_fields32_modify(unit, hg_reg[mtp_index], port, count,
                                     fields, values));
    }

    return BCM_E_NONE;
}
#endif /* BCM_TRIUMPH_SUPPORT */

#if defined(BCM_RAPTOR_SUPPORT)
/*
 * Function:
 *  	_bcm_raptor_mirror_egr_dest_get 
 * Purpose:
 *  	Get destination port bitmap for egress mirroring.
 * Parameters:
 *	    unit        - (IN)BCM device number.
 *	    port        - (IN)port number.
 *      dest_bitmap - (OUT) destination port bitmap.
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_raptor_mirror_egr_dest_get(int unit, bcm_port_t port, 
                                bcm_pbmp_t *dest_bitmap)
{
    uint32 mirror;

    /* Input parameters check. */
    if (NULL == dest_bitmap) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(READ_EMIRROR_CONTROLr(unit, port, &mirror));
    SOC_PBMP_WORD_SET(*dest_bitmap, 0, 
        soc_reg_field_get(unit, EMIRROR_CONTROLr, mirror, BITMAPf));
    
    BCM_IF_ERROR_RETURN(READ_EMIRROR_CONTROL_HIr(unit, port, &mirror));
    SOC_PBMP_WORD_SET(*dest_bitmap, 1, 
        soc_reg_field_get(unit, EMIRROR_CONTROL_HIr, mirror, BITMAPf));

    return (BCM_E_NONE);
}


/*
 * Function:
 *  	_bcm_raptor_mirror_egr_dest_set 
 * Purpose:
 *  	Set destination port bitmap for egress mirroring.
 * Parameters:
 *	    unit        - (IN)BCM device number.
 *	    port        - (IN)Port number.
 *      dest_bitmap - (IN)Destination port bitmap.
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_raptor_mirror_egr_dest_set(int unit, bcm_port_t port, 
                                bcm_pbmp_t *dest_bitmap)
{
    uint32 value;
    soc_field_t field = BITMAPf;

    /* Input parameters check. */
    if (NULL == dest_bitmap) {
        return (BCM_E_PARAM);
    }
    value = SOC_PBMP_WORD_GET(*dest_bitmap, 0);

    BCM_IF_ERROR_RETURN
        (soc_reg_fields32_modify(unit, EMIRROR_CONTROLr, port, 
                                 1, &field, &value));

    /* Enable mirroring of CPU Higig packets as well */
    if (IS_CPU_PORT(unit, port)) {
        BCM_IF_ERROR_RETURN
            (soc_reg_fields32_modify(unit, IEMIRROR_CONTROLr, port, 
                                     1, &field, &value));

    }

    value = SOC_PBMP_WORD_GET(*dest_bitmap, 1);

    BCM_IF_ERROR_RETURN
        (soc_reg_fields32_modify(unit, EMIRROR_CONTROL_HIr, port, 
                                 1, &field, &value));

    /* Enable mirroring of CPU Higig packets as well */
    if (IS_CPU_PORT(unit, port)) {
        BCM_IF_ERROR_RETURN
            (soc_reg_fields32_modify(unit, IEMIRROR_CONTROL_HIr, port, 
                                     1, &field, &value));

    }
    return (BCM_E_NONE);
}

#endif /* BCM_RAPTOR_SUPPORT */

#if defined(BCM_XGS12_FABRIC_SUPPORT)
/*
 * Function:
 *	   _bcm_xgs_fabric_mirror_enable_set 
 * Purpose:
 *  	Enable/disable mirroring on a port & set mirror-to port.
 * Parameters:
 *	    unit - BCM device number
 *  	port - port number
 *   	enable - enable mirroring if non-zero
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_xgs_fabric_mirror_enable_set(int unit, int port, int enable)
{
    pbmp_t ppbm;
    int mport;

    if (!IS_HG_PORT(unit, port)) {
        return (BCM_E_UNAVAIL);
    }

    /* Clear port when disabling */
    if (enable && MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0)) {
       BCM_IF_ERROR_RETURN(_bcm_mirror_destination_gport_parse(unit,
                                           MIRROR_CONFIG_ING_MTP_DEST(unit, 0),
                                           NULL , &mport, NULL));
    } else {
        mport = 0;
    }

    SOC_PBMP_CLEAR(ppbm);
    if (enable) {
        SOC_PBMP_PORT_ADD(ppbm, mport);
    }

#ifdef	BCM_HERCULES15_SUPPORT
    if (SOC_IS_HERCULES15(unit)) {
        int    m5670;

        m5670 = soc_property_get(unit, spn_MIRROR_5670_MODE, 0);
        BCM_IF_ERROR_RETURN 
            (soc_reg_field32_modify(unit, ING_CTRLr, port,
                                    DISABLE_MIRROR_CHANGEf,
                                    (m5670 | !enable) ? 1 : 0));
    }
#endif	/* BCM_HERCULES15_SUPPORT */
    BCM_IF_ERROR_RETURN
        (WRITE_ING_MIRTOBMAPr(unit, port,
                              SOC_PBMP_WORD_GET(ppbm, 0)));
    return (BCM_E_NONE);
}
#endif /* BCM_XGS12_FABRIC_SUPPORT */

/*
 * Function:
 *     _bcm_mirror_dest_get_all
 * Purpose:
 *     Get all mirroring destinations.   
 * Parameters:
 *     unit             - (IN) BCM device number. 
 *     flags            - (IN) BCM_MIRROR_PORT_XXX flags.
 *     mirror_dest_size - (IN) Preallocated mirror_dest array size.
 *     mirror_dest      - (OUT)Filled array of port mirroring destinations
 *     mirror_dest_count - (OUT)Actual number of mirroring destinations filled.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_mirror_dest_get_all(int unit, uint32 flags, int mirror_dest_size,
                         bcm_gport_t *mirror_dest, int *mirror_dest_count)
{
    int idx = 0;
    int index = 0;

    /* Input parameters check. */
    if ((NULL == mirror_dest) || (NULL == mirror_dest_count)) {
        return (BCM_E_PARAM);
    }
#ifdef BCM_TRIUMPH2_SUPPORT 
    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        /* Copy all used shared mirror destinations. */
        for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) {
            if ((index < mirror_dest_size) && 
                (MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx))) {
                if ((!(MIRROR_CONFIG_SHARED_MTP(unit, idx).egress) &&
                    (flags & BCM_MIRROR_PORT_INGRESS)) ||
                    (MIRROR_CONFIG_SHARED_MTP(unit, idx).egress &&
                     (flags & BCM_MIRROR_PORT_EGRESS))) {
                    mirror_dest[index] =
                        MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx);
                    index++;
                }
            }
        } 
    } else {
#endif /* BCM_TRIUMPH2_SUPPORT */
        /* Copy all used ingress mirror destinations. */
        if (flags & BCM_MIRROR_PORT_INGRESS) {
            for (idx = 0; idx < MIRROR_CONFIG(unit)->ing_mtp_count; idx++) {
                if ((index < mirror_dest_size) && 
                    (MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, idx))) {
                    mirror_dest[index] =
                        MIRROR_CONFIG_ING_MTP_DEST(unit, idx);
                    index++;
                }
            }
        } 

        /* Copy all used egress mirror destinations. */
        if (flags & BCM_MIRROR_PORT_EGRESS) {
            for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_mtp_count; idx++) {
                if ((index < mirror_dest_size) && 
                    (MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx))) {
                    mirror_dest[index] =
                        MIRROR_CONFIG_EGR_MTP_DEST(unit, idx);
                    index++;
                }
            }
        }

#ifdef BCM_TRIUMPH2_SUPPORT
    }

    if (soc_feature(unit, soc_feature_egr_mirror_true)) {
        /* Copy all used egress mirror destinations. */
        if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
            for (idx = 0;
                 idx < MIRROR_CONFIG(unit)->egr_true_mtp_count;
                 idx++) {
                if ((index < mirror_dest_size) && 
                    (MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, idx))) {
                    mirror_dest[index] =
                        MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, idx);
                    index++;
                }
            }
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    *mirror_dest_count = index;
    return (BCM_E_NONE);
}

#if defined(BCM_XGS3_SWITCH_SUPPORT)

#if defined(BCM_TRX_SUPPORT)

#if defined(BCM_TRIDENT_SUPPORT)

/*
 * Function:
 *	    _bcm_trident_mirror_l2_tunnel_set
 * Purpose:
 *	   Prepare & write L2 mirror tunnel encapsulation on Trident.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     index      - (IN) Mtp index.
 *     flags      - (IN) Mirror direction flags.
 *     entries    - (IN) Pointer to EGR_MIRROR_ENCAP_* entries array
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_trident_mirror_l2_tunnel_set(int unit, int index,
                                  int flags, void **entries)
{
    bcm_mirror_destination_t *mirror_dest;/* Destination & encapsulation. */
    _bcm_mtp_config_p   mtp_cfg;          /* Mtp configuration.           */
    uint32              hw_buffer;        /* HW buffer.                   */
    egr_mirror_encap_control_entry_t *control_entry_p;
    egr_mirror_encap_data_1_entry_t *data_1_entry_p;

    /* These entries were initialized by the calling function */
    control_entry_p = entries[EGR_MIRROR_ENCAP_ENTRIES_CONTROL];
    data_1_entry_p = entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_1];

    /* Get mtp configuration structure by direction & index. */
    mtp_cfg = MIRROR_CONFIG_MTP(unit, index, flags);

    mirror_dest = MIRROR_DEST(unit, mtp_cfg->dest_id);

    /* Outer vlan tag. */
    hw_buffer = (((uint32)mirror_dest->tpid << 16) | 
                  (uint32)mirror_dest->vlan_id);

    /* Setup Mirror Control Memory */
    soc_EGR_MIRROR_ENCAP_CONTROLm_field32_set(unit, control_entry_p,
                         ENTRY_TYPEf, BCM_TD_MIRROR_ENCAP_TYPE_RSPAN);

    soc_EGR_MIRROR_ENCAP_CONTROLm_field32_set(unit, control_entry_p,
                         RSPAN__ADD_OPTIONAL_HEADERf,
                         BCM_TD_MIRROR_HEADER_ONLY);

    if (soc_feature(unit, soc_feature_trill)) {
        soc_EGR_MIRROR_ENCAP_CONTROLm_field32_set(unit, control_entry_p,
                                      RSPAN__ADD_TRILL_OUTER_VLANf, 0);
    }

    /* Setup Mirror Data 1 Memory */
    soc_EGR_MIRROR_ENCAP_DATA_1m_field32_set(unit, data_1_entry_p,
                         RSPAN__RSPAN_VLAN_TAGf, hw_buffer);

    /* Profile entries will be committed to HW by the calling function. */

    return (BCM_E_NONE);
}

/*
 * Function:
 *	    _bcm_trident_mirror_ipv4_gre_tunnel_set
 * Purpose:
 *	   Prepare IPv4 mirror tunnel encapsulation for Trident.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     index      - (IN) Mtp index.
 *     flags      - (IN) Mirror direction flags. 
 *     entries    - (IN) Pointer to EGR_MIRROR_ENCAP_* entries array
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_trident_mirror_ipv4_gre_tunnel_set(int unit, int index, int flags,
                                        void **entries)
{
    bcm_mirror_destination_t *mirror_dest;/* Destination & encapsulation. */
    /*SW tunnel encap buffers.*/
    uint32 ip_buffer[_BCM_TD_MIRROR_V4_GRE_BUFFER_SZ];
    _bcm_mtp_config_p   mtp_cfg;       /* Mtp configuration.           . */
    egr_mirror_encap_control_entry_t *control_entry_p;
    egr_mirror_encap_data_1_entry_t *data_1_entry_p;
    uint32 fldval;
    int                 idx;           /* Headers offset iterator.        */

    /* These entries were initialized by the calling function */
    control_entry_p = entries[EGR_MIRROR_ENCAP_ENTRIES_CONTROL];
    data_1_entry_p = entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_1];

    /* Get mtp configuration structure by direction & index. */
    mtp_cfg = MIRROR_CONFIG_MTP(unit, index, flags);

    mirror_dest = MIRROR_DEST(unit, mtp_cfg->dest_id);

    sal_memset(ip_buffer, 0,
               _BCM_TD_MIRROR_V4_GRE_BUFFER_SZ * sizeof(uint32));

    /* Setup Mirror Control Memory */
    soc_EGR_MIRROR_ENCAP_CONTROLm_field32_set(unit, control_entry_p,
                         ENTRY_TYPEf, BCM_TD_MIRROR_ENCAP_TYPE_ERSPAN);

    soc_EGR_MIRROR_ENCAP_CONTROLm_field32_set(unit, control_entry_p,
                         ERSPAN__ADD_OPTIONAL_HEADERf,
                         BCM_TD_MIRROR_HEADER_ONLY);

    if (BCM_VLAN_VALID(BCM_VLAN_CTRL_ID(mirror_dest->vlan_id))) {
        soc_EGR_MIRROR_ENCAP_CONTROLm_field32_set(unit, control_entry_p,
                             ERSPAN__ADD_ERSPAN_OUTER_VLANf, 1);
    }

    if (mirror_dest->flags & BCM_MIRROR_DEST_PAYLOAD_UNTAGGED) {
        soc_EGR_MIRROR_ENCAP_CONTROLm_field32_set(unit, control_entry_p,
                                                  ERSPAN__UNTAG_PAYLOADf, 1);
    }

    /*
     *   Set mirror tunnel DA SA. 
     */

    soc_mem_mac_addr_set(unit, EGR_MIRROR_ENCAP_DATA_1m,
                         data_1_entry_p, ERSPAN__ERSPAN_HEADER_DAf,
                         mirror_dest->dst_mac);
    soc_mem_mac_addr_set(unit, EGR_MIRROR_ENCAP_DATA_1m,
                         data_1_entry_p, ERSPAN__ERSPAN_HEADER_SAf,
                         mirror_dest->src_mac);

    /* Set tpid & vlan id. */
    if (BCM_VLAN_VALID(BCM_VLAN_CTRL_ID(mirror_dest->vlan_id))) {
        fldval = (((uint32)mirror_dest->tpid << 16) | 
                  (uint32)mirror_dest->vlan_id);    
    } else {
        fldval = 0;
    }
    soc_EGR_MIRROR_ENCAP_DATA_1m_field32_set(unit, data_1_entry_p,
                         ERSPAN__ERSPAN_HEADER_VLAN_TAGf, fldval);

    /* Set ether type to ip. 0x800  */
    soc_EGR_MIRROR_ENCAP_DATA_1m_field32_set(unit, data_1_entry_p,
                         ERSPAN__ERSPAN_HEADER_ETYPEf, 0x800);

    /* Set protocol to given value, or default of GRE. 0x88be */
    soc_EGR_MIRROR_ENCAP_DATA_1m_field32_set(unit, data_1_entry_p,
                         ERSPAN__ERSPAN_HEADER_GREf,
                         (0 != mirror_dest->gre_protocol) ?
                               mirror_dest->gre_protocol : 0x88be);

    /*
     *   Set IPv4 header. 
     */
    /* Version + 5 word no options length.  + Tos */
    /* Length, Id, Flags, Fragmentation offset. */
    idx = 4;

    ip_buffer[idx--] |= ((uint32)(0x45 << 24) |
                         (uint32)((mirror_dest->tos) << 16));

    idx--;
    /* Ttl, Protocol (GRE 0x2f)*/
    ip_buffer[idx--] = (((uint32)mirror_dest->ttl << 24) | (0x2f << 16));

    /* Src Ip. */
    ip_buffer[idx--] = mirror_dest->src_addr;

    /* Dst Ip. */
    ip_buffer[idx] = mirror_dest->dst_addr;

    soc_EGR_MIRROR_ENCAP_DATA_1m_field_set(unit, data_1_entry_p,
                         ERSPAN__ERSPAN_HEADER_V4f, ip_buffer);

    /* Profile entries will be committed to HW by the calling function. */

    return (BCM_E_NONE);
}

/*
 * Function:
 *	    _bcm_trident_mirror_trill_tunnel_set
 * Purpose:
 *	   Prepare Trill mirror tunnel encapsulation for Trident.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     index      - (IN) Mtp index.
 *     flags      - (IN) Mirror direction flags. 
 *     entries    - (IN) Pointer to EGR_MIRROR_ENCAP_* entries array
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_trident_mirror_trill_tunnel_set(int unit, int index, int flags,
                                     void **entries)
{
     _bcm_mtp_config_p   mtp_cfg;       /* Mtp configuration.              . */
     bcm_mirror_destination_t *mirror_dest;/* Destination & encapsulation.   */

     /*SW tunnel encap buffer.*/
    uint32      trill_buffer[_BCM_TD_MIRROR_V4_GRE_BUFFER_SZ];
    /* index to end of TRILL portion of buffer */
    int         idx = _BCM_TD_MIRROR_TRILL_BUFFER_SZ - 1;
    egr_mirror_encap_control_entry_t *control_entry_p;
    egr_mirror_encap_data_2_entry_t *data_2_entry_p;
    
    /* These entries were initialized by the calling function */
    control_entry_p = entries[EGR_MIRROR_ENCAP_ENTRIES_CONTROL];
    data_2_entry_p = entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_2];
    
    /* Get mtp configuration structure by direction & index. */
    mtp_cfg = MIRROR_CONFIG_MTP(unit, index, flags);

    mirror_dest = MIRROR_DEST(unit, mtp_cfg->dest_id);

    sal_memset(trill_buffer, 0,
               _BCM_TD_MIRROR_V4_GRE_BUFFER_SZ * sizeof(uint32));

    trill_buffer[idx--] = ((uint32)(BCM_TD_MIRROR_TRILL_VERSION << 
                                    BCM_TD_MIRROR_TRILL_VERSION_OFFSET) | 
                           (uint32)(mirror_dest->trill_hopcount << 
                                    BCM_TD_MIRROR_TRILL_HOPCOUNT_OFFSET) | 
                           (uint32)(mirror_dest->trill_src_name));
    trill_buffer[idx] = ((uint32)(mirror_dest->trill_dst_name << 
                                  BCM_TD_MIRROR_TRILL_DEST_NAME_OFFSET));

    soc_EGR_MIRROR_ENCAP_DATA_2m_field_set(unit, data_2_entry_p,
                                           HEADER_DATAf, trill_buffer);

    /* ERSPAN and RSPAN use the same field and encoding. */
    soc_EGR_MIRROR_ENCAP_CONTROLm_field32_set(unit, control_entry_p,
                         ERSPAN__ADD_OPTIONAL_HEADERf,
                         BCM_TD_MIRROR_HEADER_TRILL);

    return (BCM_E_NONE);
}

/*
 * Function:
 *	    _bcm_trident_mirror_niv_tunnel_set
 * Purpose:
 *	   Prepare NIV mirror tunnel encapsulation for Trident.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     index      - (IN) Mtp index.
 *     flags      - (IN) Mirror direction flags. 
 *     entries    - (IN) Pointer to EGR_MIRROR_ENCAP_* entries array
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_trident_mirror_niv_tunnel_set(int unit, int index, int flags,
                                   void **entries)
{
    _bcm_mtp_config_p   mtp_cfg;       /* Mtp configuration.           . */
    bcm_mirror_destination_t *mirror_dest;/* Destination & encapsulation. */
    /*SW tunnel encap buffers.*/
    uint32 niv_buffer;
    egr_mirror_encap_control_entry_t *control_entry_p;
    egr_mirror_encap_data_2_entry_t *data_2_entry_p;

    /* These entries were initialized by the calling function */
    control_entry_p = entries[EGR_MIRROR_ENCAP_ENTRIES_CONTROL];
    data_2_entry_p = entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_2];
    
    /* Get mtp configuration structure by direction & index. */
    mtp_cfg = MIRROR_CONFIG_MTP(unit, index, flags);

    mirror_dest = MIRROR_DEST(unit, mtp_cfg->dest_id);

    niv_buffer = 0;

    niv_buffer = (uint32) mirror_dest->niv_src_vif;
    if (mirror_dest->niv_flags & BCM_MIRROR_NIV_LOOP) {
        niv_buffer |= _BCM_TD_MIRROR_NIV_LOOP_BIT;
    }
    niv_buffer |= ((uint32) mirror_dest->niv_dst_vif <<
                   _BCM_TD_MIRROR_NIV_DST_VIF_OFFSET);

    soc_EGR_MIRROR_ENCAP_DATA_2m_field32_set(unit, data_2_entry_p,
                                             VNTAG_HEADERf, niv_buffer);

    /* ERSPAN and RSPAN use the same field and encoding. */
    soc_EGR_MIRROR_ENCAP_CONTROLm_field32_set(unit, control_entry_p,
                         ERSPAN__ADD_OPTIONAL_HEADERf,
                         BCM_TD_MIRROR_HEADER_VNTAG);

    return (BCM_E_NONE);
}

#endif

/*
 * Function:
 *	    _bcm_trx_mirror_egr_erspan_write
 * Purpose:
 *	   Program HW buffer.  
 * Parameters:
 *	   unit     - (IN) BCM device number.
 *     index    - (IN) Mtp index.
 *     buffer   - (IN) Tunnel encapsulation buffer.
 *     flags    - (IN) Mirror direction flags.  
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_trx_mirror_egr_erspan_write(int unit, int index, uint32 *buffer, int flags)
{
    bcm_mirror_destination_t *mirror_dest;/* Destination & encapsulation. */ 
    _bcm_mtp_config_p        mtp_cfg;     /* MTP configuration.           */
    egr_erspan_entry_t       hw_buf;      /* Hw table buffer              */

    /* Get mtp configuration structure by direction & index. */
    mtp_cfg = MIRROR_CONFIG_MTP(unit, index, flags);

    /* Advance index according to flags. */
    if (flags & BCM_MIRROR_PORT_EGRESS) {
        index += 4;
    } else if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) { /* True Egress */
        index += 8;
    }

    mirror_dest = MIRROR_DEST(unit, mtp_cfg->dest_id);

    /* Reset hw buffer. */
    sal_memset(&hw_buf, 0, sizeof(egr_erspan_entry_t));

    /* Enable tunneling for mtp . */
    soc_mem_field32_set(unit, EGR_ERSPANm, &hw_buf, ERSPAN_ENABLEf, 1);

    /* Set untag payload flag. */
    if (mirror_dest->flags & BCM_MIRROR_DEST_PAYLOAD_UNTAGGED) {
        soc_EGR_ERSPANm_field32_set(unit, &hw_buf, UNTAG_PAYLOADf, 1);
    }

    /* Set tunnel header.. */
    if (BCM_VLAN_VALID(BCM_VLAN_CTRL_ID(mirror_dest->vlan_id))) {
        soc_EGR_ERSPANm_field32_set(unit, &hw_buf, USE_TAGGED_HEADERf, 1);
        soc_EGR_ERSPANm_field_set(unit, &hw_buf, HEADER_TAGGEDf, buffer);
    } else {
        soc_EGR_ERSPANm_field_set(unit, &hw_buf, HEADER_UNTAGGEDf, buffer);
    }

    /* Write buffer to hw. */
    BCM_IF_ERROR_RETURN 
        (soc_mem_write(unit, EGR_ERSPANm, MEM_BLOCK_ALL, index, &hw_buf));

    return (BCM_E_NONE);
}

/*
 * Function:
 *	    _bcm_trx_mirror_ipv4_gre_tunnel_set
 * Purpose:
 *	   Prepare IPv4 mirror tunnel encapsulation.
 * Parameters:
 *	   unit       - (IN) BCM device number
 *     index      - (IN) Mtp index.
 *     flags      - (IN) Mirror direction flags. 
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_trx_mirror_ipv4_gre_tunnel_set(int unit, int index, int flags)
{
    bcm_mirror_destination_t *mirror_dest;/* Destination & encapsulation.   */
    uint32 buffer[_BCM_TRX_MIRROR_TUNNEL_BUFFER_SZ];/*SW tunnel encap buffer.*/
    _bcm_mtp_config_p   mtp_cfg;       /* Mtp configuration.              . */
    int                 idx;           /* Headers offset iterator.          */

    /* Get mtp configuration structure by direction & index. */
    mtp_cfg = MIRROR_CONFIG_MTP(unit, index, flags);

    mirror_dest = MIRROR_DEST(unit, mtp_cfg->dest_id);

    sal_memset(buffer, 0, _BCM_TRX_MIRROR_TUNNEL_BUFFER_SZ * sizeof(uint32));

    /*
     *   L2 Header. 
     */
    idx = BCM_VLAN_VALID(BCM_VLAN_CTRL_ID(mirror_dest->vlan_id)) ? 10 : 9;
      
    /* Destination mac address. */
    buffer[idx--] = (((uint32)(mirror_dest->dst_mac)[0]) << 8 | \
                     ((uint32)(mirror_dest->dst_mac)[1]));

    buffer[idx--] = (((uint32)(mirror_dest->dst_mac)[2]) << 24 | \
                     ((uint32)(mirror_dest->dst_mac)[3]) << 16 | \
                     ((uint32)(mirror_dest->dst_mac)[4]) << 8  | \
                     ((uint32)(mirror_dest->dst_mac)[5])); 

    /* Source mac address. */
    buffer[idx--] = (((uint32)(mirror_dest->src_mac)[0]) << 24 | \
                     ((uint32)(mirror_dest->src_mac)[1]) << 16 | \
                     ((uint32)(mirror_dest->src_mac)[2]) << 8  | \
                     ((uint32)(mirror_dest->src_mac)[3])); 

    buffer[idx] = (((uint32)(mirror_dest->src_mac)[4]) << 24 | \
                   ((uint32)(mirror_dest->src_mac)[5]) << 16); 

    /* Set tpid & vlan id. */
    if (BCM_VLAN_VALID(BCM_VLAN_CTRL_ID(mirror_dest->vlan_id))) {
        /* Tpid. */
        buffer[idx--] |= (((uint32)(mirror_dest->tpid >> 8)) << 8 | \
                          ((uint32)(mirror_dest->tpid & 0xff)));

        /* Priority,  Cfi, Vlan id. */
        buffer[idx] = (((uint32)(mirror_dest->vlan_id >> 8)) << 24 | \
                       ((uint32)(mirror_dest->vlan_id & 0xff) << 16));
    }

    /* Set ether type to ip. 0x800  */
    buffer[idx--] |= (uint32)(0x08 << 8);

    /*
     *   IPv4 header. 
     */
    /* Version + 5 word no options length.  + Tos */
    /* Length, Id, Flags, Fragmentation offset. */
    buffer[idx--] |= ((uint32)(0x45 << 24) | \
                      (uint32)(mirror_dest->tos) << 16);

    idx--;
    /* Ttl, Protocol (GRE 0x2f)*/
    buffer[idx--] = (((uint32)mirror_dest->ttl << 24) | (0x2f << 16));

    /* Src Ip. */
    buffer[idx--] = mirror_dest->src_addr;

    /* Dst Ip. */
    buffer[idx--] = mirror_dest->dst_addr;

    /*
     *   Gre header. 
     */

    /* Protocol. 0x88be */
    buffer[idx] = (0 != mirror_dest->gre_protocol) ?
        mirror_dest->gre_protocol : 0x88be;

    /* swap byte in tunnel buffer. */
    /*  _shr_bit_rev8(buffer[idx]); */

    BCM_IF_ERROR_RETURN
        (_bcm_trx_mirror_egr_erspan_write(unit, index, (uint32 *)buffer, flags));

    return (BCM_E_NONE);
}

/*
 * Function:
 *	    _bcm_trx_mirror_rspan_write
 * Purpose:
 *	   Prepare & write L2 mirror tunnel encapsulation.
 * Parameters:
 *	   unit       - (IN) BCM device number
 *     index      - (IN) Mtp index.
 *     port- (IN) 
 *     flags      - (IN) Mirror direction flags.
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_trx_mirror_rspan_write(int unit, int index, bcm_port_t port, int flags)
{
    bcm_mirror_destination_t *mirror_dest;/* Destination & encapsulation. */
    _bcm_mtp_config_p   mtp_cfg;          /* Mtp configuration.           */
    uint32              hw_buffer;        /* HW buffer.                   */


    /* Get mtp configuration structure by direction & index. */
    mtp_cfg = MIRROR_CONFIG_MTP(unit, index, flags);

    mirror_dest = MIRROR_DEST(unit, mtp_cfg->dest_id);

    /* Outer vlan tag. */
    hw_buffer = (((uint32)mirror_dest->tpid << 16) | 
                  (uint32)mirror_dest->vlan_id);

    BCM_IF_ERROR_RETURN(soc_reg_field32_modify(unit, EGR_RSPAN_VLAN_TAGr, 
                                               port, TAGf, hw_buffer));
    return (BCM_E_NONE);
}

/*
 * Function:
 *	    _bcm_trx_mirror_l2_tunnel_set
 * Purpose:
 *	   Programm mirror L2 tunnel 
 * Parameters:
 *	   unit       - (IN) BCM device number
 *     index      - (IN) Mtp index.
 *     trunk_arr  - (IN) Mirror destinations array.
 *     flags      - (IN) Mirror direction flags.
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_trx_mirror_l2_tunnel_set(int unit, int index, 
                              bcm_gport_t *trunk_arr, int flags)
{
    bcm_module_t  my_modid;    /* Local modid.                   */
    int           idx;         /* Trunk members iteration index. */
    bcm_module_t  mod_out;     /* Hw mapped modid.               */
    bcm_port_t    port_out;    /* Hw mapped port number.         */
    bcm_module_t  modid;       /* Application space modid.       */
    bcm_port_t    port;        /* Application space port number. */

    /* Input parameters check. */ 
    if (NULL == trunk_arr) {
        return (BCM_E_PARAM);
    }

    /* Get local base module id. */
    BCM_IF_ERROR_RETURN (bcm_esw_stk_my_modid_get(unit, &my_modid));

    for (idx = 0; idx < BCM_SWITCH_TRUNK_MAX_PORTCNT; idx++) {
        modid = BCM_GPORT_MODPORT_MODID_GET(trunk_arr[idx]);
        port = BCM_GPORT_MODPORT_PORT_GET(trunk_arr[idx]);
        BCM_IF_ERROR_RETURN
            (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET, modid, port, 
                                    &mod_out, &port_out));
        if (mod_out != my_modid) {
            /* Not a local front-panel port.
             * Use bcm_mirror_vlan_set for remote ports. */
            return BCM_E_PARAM;
        }

        if (0 == IS_E_PORT(unit, port_out)) {
            /* Not a local front-panel port.
             * Use bcm_mirror_vlan_set for remote ports. */
            return BCM_E_PARAM;
        }

        BCM_IF_ERROR_RETURN
            (_bcm_trx_mirror_rspan_write(unit, index, port_out, flags));
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *	    _bcm_trx_mirror_tunnel_set
 * Purpose:
 *	   Initialize mirror tunnel 
 * Parameters:
 *     unit       - (IN) BCM device number
 *     index      - (IN) Mtp index.
 *     trunk_arr  - (IN) 
 *     flags      - (IN) Mirror direction flags.
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_trx_mirror_tunnel_set(int unit, int index,
                           bcm_gport_t *trunk_arr, int flags)
{
    bcm_mirror_destination_t *mirror_dest; /* Destination & Encapsulation.*/
    _bcm_mtp_config_p   mtp_cfg;           /* MTP configuration .         */
    int rv = BCM_E_NONE;                   /* Operation return status.    */
#if defined(BCM_TRIDENT_SUPPORT)
    egr_mirror_encap_control_entry_t control_entry;
    egr_mirror_encap_data_1_entry_t data_1_entry;
    egr_mirror_encap_data_2_entry_t data_2_entry;
    void *entries[EGR_MIRROR_ENCAP_ENTRIES_NUM];
    int update_eme = FALSE;                /* EGR_MIRROR_ENCAP_* pointers */
    uint32 profile_index;

    sal_memset(&control_entry, 0, sizeof(control_entry));
    sal_memset(&data_1_entry, 0, sizeof(data_1_entry));
    sal_memset(&data_2_entry, 0, sizeof(data_2_entry));

    entries[EGR_MIRROR_ENCAP_ENTRIES_CONTROL] = &control_entry;
    entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_1] = &data_1_entry;
    entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_2] = &data_2_entry;
#endif /* TRIDENT  */

    /* Get mtp configuration structure by direction & index. */
    mtp_cfg = MIRROR_CONFIG_MTP(unit, index, flags);

    mirror_dest = MIRROR_DEST(unit, mtp_cfg->dest_id);
    if (mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_IP_GRE) {
        if (4 == mirror_dest->version) {
#if defined(BCM_TRIDENT_SUPPORT)
            if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
                rv = _bcm_trident_mirror_ipv4_gre_tunnel_set(unit, index,
                                                             flags, entries);
                update_eme = TRUE;
            } else 
#endif /* TRIDENT  */
            {
                rv = _bcm_trx_mirror_ipv4_gre_tunnel_set(unit, index, flags);
            }            
        } else {
            rv = (BCM_E_UNAVAIL);
        }
    }

    if (BCM_SUCCESS(rv) &&
        (0 != (mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_L2))) {
#if defined(BCM_TRIDENT_SUPPORT)
        if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
            rv = _bcm_trident_mirror_l2_tunnel_set(unit, index,
                                                   flags, entries);
            update_eme = TRUE;
        } else
#endif /* TRIDENT  */
        {
            rv = _bcm_trx_mirror_l2_tunnel_set(unit, index, trunk_arr, flags);
        }
    }
#if defined(BCM_TRIDENT_SUPPORT)
    /*
     * Note: Trill and NIV features are checked when the mirror
     * destination is created.  Thus, the flags will not be
     * set on a device which doesn't support the feature.
     */
    if (BCM_SUCCESS(rv) &&
        (0 != (mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_TRILL))) {
        rv = _bcm_trident_mirror_trill_tunnel_set(unit, index,
                                                  flags, entries);
        update_eme = TRUE;
    }
    if (BCM_SUCCESS(rv) &&
        (0 != (mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_NIV))) {
        rv = _bcm_trident_mirror_niv_tunnel_set(unit, index,
                                                flags, entries);
        update_eme = TRUE;
    }

    if (BCM_SUCCESS(rv) && update_eme) {
        rv = _bcm_egr_mirror_encap_entry_add(unit, entries, &profile_index);
    }

    if (BCM_SUCCESS(rv) && update_eme) {
        /* Supply the correct profile index to the selected MTPs */
        rv = _bcm_egr_mirror_encap_entry_mtp_update(unit, index,
                                                    profile_index, flags);
    }
#endif /* TRIDENT  */

    return (rv);
}
#endif /* BCM_TRX_SUPPORT */


#if defined(BCM_TRIDENT_SUPPORT)
/*
 * Function:
 *	    _bcm_trident_mtp_init
 * Purpose:
 *	   Initialize mirror target port for TRIDENT devices. 
 * Parameters:
 *	   unit       - (IN)BCM device number
 *     index      - (IN)Mtp index.
 *     trunk_arr  - (IN)Trunk members array. 
 *     flags      - (IN)Filled entry flags(BCM_MIRROR_PORT_INGRESS/EGRESS
 *                    or both. In case both flags are specied
 *                    ingress & egress configuration is assumed to be
 *                    idential.
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_trident_mtp_init(int unit, int index, bcm_gport_t *trunk_arr, int flags)
{
    bcm_gport_t             mirror_dest;
    bcm_port_t              port_out;
    bcm_module_t            mod_out;
    _bcm_mtp_config_p       mtp_cfg;
    int                     offset;
    int                     idx, id, isLocal;
    bcm_trunk_t             trunk = BCM_TRUNK_INVALID;
    bcm_module_t            modid = 0;
    bcm_port_t              port = -1;
    uint32                  mtp[4], egr_mtp;
    int                     member_count = 0, rtag;

    /* HW does not store the trunk ID here, so during Warm Boot
     * we must take a mod,port and reverse map it to the trunk
     * This will work because we will have the T bit to indicate
     * a trunk, and the trunk module recovery takes place before
     * mirror, so the reverse mapping is available in SW. */
    static const soc_field_t port_field[] = {
        PORT_NUM_0f, PORT_NUM_1f, PORT_NUM_2f, PORT_NUM_3f, 
        PORT_NUM_4f, PORT_NUM_5f, PORT_NUM_6f, PORT_NUM_7f};
    static const soc_field_t module_field[] = {
            MODULE_ID_0f, MODULE_ID_1f, MODULE_ID_2f, MODULE_ID_3f, 
            MODULE_ID_4f, MODULE_ID_5f, MODULE_ID_6f, MODULE_ID_7f};

    /* Input parameters check */ 
    if (NULL == trunk_arr) {
        return (BCM_E_PARAM);
    }

    /* Get mtp configuration structure by direction & index. */
    mtp_cfg = MIRROR_CONFIG_MTP(unit, index, flags);
    sal_memset(mtp, 0, sizeof(mtp));

    /* Parse destination trunk / port & module. */
    mirror_dest = MIRROR_DEST_GPORT(unit, mtp_cfg->dest_id);
    if (BCM_GPORT_IS_TRUNK(mirror_dest)) {
        trunk = BCM_GPORT_TRUNK_GET(mirror_dest);
        BCM_IF_ERROR_RETURN
            (_bcm_trunk_id_validate(unit, trunk));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_trunk_active_member_get(unit, trunk, NULL, 0, NULL, 
                                              &member_count));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_trunk_rtag_get(unit, trunk, &rtag));
        
        if (member_count > BCM_TD_MIRROR_TRUNK_MAX_PORTCNT) {
            member_count = BCM_TD_MIRROR_TRUNK_MAX_PORTCNT;
        }
        soc_IM_MTP_INDEXm_field32_set(unit, mtp, Tf, 1);

        if (0 == member_count) {
            port = SOC_PORT_ADDR_MAX(unit);
            while (0 <= port) {
                if (!SOC_PORT_VALID(unit, port)) {
                    /* Found invalid port, use as black hole */
                    break;
                }
                port--;
            }
            if (port < 0) {
                /* Couldn't find an usused port, give up on empty trunk */
                return BCM_E_PORT;
            }

            /* Get local modid. */
            BCM_IF_ERROR_RETURN(_bcm_esw_local_modid_get(unit, &modid));

            soc_IM_MTP_INDEXm_field32_set(unit, mtp, COUNTf, 0);
            soc_IM_MTP_INDEXm_field32_set(unit, mtp, RTAGf, 0);

            soc_IM_MTP_INDEXm_field32_set(unit, mtp,
                                          module_field[0], modid);
            soc_IM_MTP_INDEXm_field32_set(unit, mtp,
                                          port_field[0], port);

            /* Cache trunk ID for Warm Boot */
            soc_IM_MTP_INDEXm_field32_set(unit, mtp,
                                          port_field[7], trunk);
        } else {
            /* If RTAG isn't RTAG7, then we must fill out all of the
             * trunk destination modport slots. */
            soc_IM_MTP_INDEXm_field32_set(unit, mtp, COUNTf,
                                          (member_count - 1));
            soc_IM_MTP_INDEXm_field32_set(unit, mtp, RTAGf, rtag);

            for (idx = 0; idx < BCM_TD_MIRROR_TRUNK_MAX_PORTCNT; idx++) {
                /* trunk variable will not be used from this point,
                 * OK to update */
                if ((7 == rtag) && (idx == member_count)) {
                    break;
                }
                BCM_IF_ERROR_RETURN
                    (_bcm_esw_gport_resolve(unit,
                                            trunk_arr[idx % member_count],
                                            &modid, &port, &trunk, &id));
                if ((BCM_TRUNK_INVALID != trunk) || (-1 != id)) {
                    return BCM_E_PARAM;
                }
                soc_IM_MTP_INDEXm_field32_set(unit, mtp,
                                              module_field[idx], modid);
                soc_IM_MTP_INDEXm_field32_set(unit, mtp,
                                              port_field[idx], port);

            }
        }

    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_gport_resolve(unit, mirror_dest,
                                    &modid, &port, &trunk, &id));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_modid_is_local(unit, modid, &isLocal));
        if (TRUE == isLocal) {
            _bcm_esw_stk_modmap_map(unit,BCM_STK_MODMAP_SET, modid, port,
                                    &modid, &port);
        }
        soc_IM_MTP_INDEXm_field32_set(unit, mtp, Tf, 0);
        soc_IM_MTP_INDEXm_field32_set(unit, mtp, COUNTf, 0);
        soc_IM_MTP_INDEXm_field32_set(unit, mtp, MODULE_IDf, modid);
        soc_IM_MTP_INDEXm_field32_set(unit, mtp, PORT_NUMf, port);

    }

    /* HW write. based on mirrored traffic direction. */
    if (flags & BCM_MIRROR_PORT_INGRESS) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_write(unit, IM_MTP_INDEXm, MEM_BLOCK_ALL, index, mtp));
    }

    /* EM_MTP_INDEX has same layout as IM_MTP_INDEX */
    if (flags & BCM_MIRROR_PORT_EGRESS) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_write(unit, EM_MTP_INDEXm, MEM_BLOCK_ALL, index, mtp));
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    /* EP_REDIRECT_EM_MTP_INDEX has same layout as IM_MTP_INDEX */
    if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_write(unit, EP_REDIRECT_EM_MTP_INDEXm,
                           MEM_BLOCK_ALL, index, &mtp));
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    if (BCM_GPORT_IS_TRUNK(mirror_dest) && (0 == member_count)) {
        /* Traffic shouldn't proceed, don't configure other elements */
        return BCM_E_NONE;
    }

    offset = index * BCM_SWITCH_TRUNK_MAX_PORTCNT;
    for (idx = 0; idx < BCM_SWITCH_TRUNK_MAX_PORTCNT; idx++, offset++) {
        egr_mtp = 0;
        BCM_IF_ERROR_RETURN(_bcm_esw_gport_resolve(
                            unit, trunk_arr[idx],&modid,&port,&trunk, &id));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET, modid, port, 
                                    &mod_out, &port_out));
         
        soc_EGR_IM_MTP_INDEXm_field32_set(unit, &egr_mtp, 
                                          MTP_DST_PORTf, port_out);
        soc_EGR_IM_MTP_INDEXm_field32_set(unit, &egr_mtp, 
                                          MTP_DST_MODIDf, mod_out);

        if ((MIRROR_DEST(unit, mtp_cfg->dest_id))->flags
            & (BCM_MIRROR_DEST_TUNNELS)) {
            soc_EGR_IM_MTP_INDEXm_field32_set(unit, &egr_mtp, 
                                              MIRROR_ENCAP_ENABLEf, 1);         
            soc_EGR_IM_MTP_INDEXm_field32_set(unit, &egr_mtp, 
                                              MIRROR_ENCAP_INDEXf, index);         
        }

        if ((MIRROR_DEST(unit, mtp_cfg->dest_id))->flags
                & (BCM_MIRROR_DEST_INT_PRI_SET)) {
            soc_EGR_IM_MTP_INDEXm_field32_set(unit, &egr_mtp, 
                    CHANGE_INT_PRIf, 1);         
            soc_EGR_IM_MTP_INDEXm_field32_set(unit, &egr_mtp, 
                    NEW_INT_PRIf, 
                    (MIRROR_DEST(unit, mtp_cfg->dest_id))->int_pri);         
        } 
        /* HW write. based on mirrored traffic direction. */
        if (flags & BCM_MIRROR_PORT_INGRESS) {
            BCM_IF_ERROR_RETURN 
                (soc_mem_write(unit, EGR_IM_MTP_INDEXm, MEM_BLOCK_ALL, 
                               offset, &egr_mtp));
        }

        /* EGR_EM_MTP_INDEX has same layout as EGR_IM_MTP_INDEX */
        if (flags & BCM_MIRROR_PORT_EGRESS) {
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, EGR_EM_MTP_INDEXm, MEM_BLOCK_ALL, 
                               offset, &egr_mtp));
        }

        /* EGR_EP_REDIRECT_EM_MTP_INDEX has same layout as
         * EGR_IM_MTP_INDEX */
        if (soc_feature(unit, soc_feature_egr_mirror_true) &&
            (flags & BCM_MIRROR_PORT_EGRESS_TRUE)) {
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, EGR_EP_REDIRECT_EM_MTP_INDEXm,
                               MEM_BLOCK_ALL, offset, &egr_mtp));
        }
    }

    if((MIRROR_DEST(unit, mtp_cfg->dest_id))->flags
        & (BCM_MIRROR_DEST_TUNNELS)) {
        BCM_IF_ERROR_RETURN
            (_bcm_trx_mirror_tunnel_set(unit, index, trunk_arr, flags));
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *	    _bcm_td_mtp_reset
 * Purpose:
 *	   Reset mirror target port for TRIDENT devices. 
 * Parameters:
 *	   unit       - (IN)BCM device number
 *     index      - (IN)Mtp index.
 *     flags      - (IN)Filled entry flags(BCM_MIRROR_PORT_INGRESS/EGRESS
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_td_mtp_reset(int unit, int index, int flags)
{
    int                         offset;
    int                         idx, encap_present;
    uint32                      mtp[SOC_MAX_MEM_BYTES/4];
    uint32                      mirror_select, mtp_type, encap_index = 0;
    egr_im_mtp_index_entry_t    entry;

    sal_memset(mtp, 0, sizeof(mtp));

    /* HW write. based on mirrored traffic direction. */
    if (flags & BCM_MIRROR_PORT_INGRESS) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_write(unit, IM_MTP_INDEXm, MEM_BLOCK_ALL, index, mtp));
    }

    /* EM_MTP_INDEX has same layout as IM_MTP_INDEX */
    if (flags & BCM_MIRROR_PORT_EGRESS) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_write(unit, EM_MTP_INDEXm, MEM_BLOCK_ALL, index, mtp));
    }

    /* Reset MTP_SELECT register to 0 */
    BCM_IF_ERROR_RETURN(READ_MIRROR_SELECTr(unit, &mirror_select));
    mtp_type = soc_reg_field_get(unit, MIRROR_SELECTr, mirror_select, MTP_TYPEf);
    mtp_type &= ~(1 << index);
    soc_reg_field_set(unit, MIRROR_SELECTr, &mirror_select, MTP_TYPEf, mtp_type);
    BCM_IF_ERROR_RETURN(WRITE_MIRROR_SELECTr(unit, mirror_select));

    encap_present = FALSE;
    offset = index * BCM_SWITCH_TRUNK_MAX_PORTCNT;
    sal_memset(mtp, 0, sizeof(mtp));

    for (idx = 0; idx < BCM_SWITCH_TRUNK_MAX_PORTCNT; idx++, offset++) {
        /* HW write. based on mirrored traffic direction. */
        if (flags & BCM_MIRROR_PORT_INGRESS) {
            if ((0 == idx) && !encap_present) {
                BCM_IF_ERROR_RETURN
                    (soc_mem_read(unit, EGR_IM_MTP_INDEXm, MEM_BLOCK_ANY, 
                                                 offset, &entry));
                if (soc_mem_field32_get(unit, EGR_IM_MTP_INDEXm, 
                                        &entry, MIRROR_ENCAP_ENABLEf)) {
                    encap_index =
                        soc_mem_field32_get(unit, EGR_IM_MTP_INDEXm, 
                                            &entry, MIRROR_ENCAP_INDEXf);
                    encap_present = TRUE;
                }
            }

            BCM_IF_ERROR_RETURN 
                (soc_mem_write(unit, EGR_IM_MTP_INDEXm, MEM_BLOCK_ALL, 
                               offset, mtp));
        }

        /* EGR_EM_MTP_INDEX has same layout as EGR_IM_MTP_INDEX */
        if (flags & BCM_MIRROR_PORT_EGRESS) {
            if ((0 == idx) && !encap_present) {
                BCM_IF_ERROR_RETURN
                    (soc_mem_read(unit, EGR_EM_MTP_INDEXm, MEM_BLOCK_ANY, 
                                  offset, &entry));
                if (soc_mem_field32_get(unit, EGR_EM_MTP_INDEXm, 
                                        &entry, MIRROR_ENCAP_ENABLEf)) {
                    encap_index =
                        soc_mem_field32_get(unit, EGR_EM_MTP_INDEXm, 
                                            &entry, MIRROR_ENCAP_INDEXf); 
                    encap_present = TRUE;
                }
            }
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, EGR_EM_MTP_INDEXm, MEM_BLOCK_ALL, 
                               offset, mtp));
        }

        /* EP_REDIRECT_EM_MTP_INDEX has same layout as IM_MTP_INDEX */
        if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
            if ((0 == idx) && !encap_present) {
                BCM_IF_ERROR_RETURN
                    (soc_mem_read(unit, EGR_EP_REDIRECT_EM_MTP_INDEXm,
                                  MEM_BLOCK_ANY, offset, &entry));
                if (soc_mem_field32_get(unit, EGR_EP_REDIRECT_EM_MTP_INDEXm, 
                                        &entry, MIRROR_ENCAP_ENABLEf)) {
                    encap_index =
                        soc_mem_field32_get(unit,
                                            EGR_EP_REDIRECT_EM_MTP_INDEXm, 
                                            &entry, MIRROR_ENCAP_INDEXf); 
                    encap_present = TRUE;
                }
            }
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, EGR_EP_REDIRECT_EM_MTP_INDEXm,
                               MEM_BLOCK_ALL, offset, mtp));
        }
    }

    /* At most one of the direction flags is true, and the whole
     * set of trunk block copies are identical.  We need only
     * one delete of our reference to the profile table.
     */
    if (encap_present) {
        BCM_IF_ERROR_RETURN
            (_bcm_egr_mirror_encap_entry_delete(unit, encap_index));
    }

    return (BCM_E_NONE);
}

#endif /* BCM_TRIDENT_SUPPORT */


#if defined(BCM_FIREBOLT_SUPPORT) 
/*
 * Function:
 *	    _bcm_fbx_mtp_init
 * Purpose:
 *	   Initialize mirror target port for FBX devices. 
 * Parameters:
 *	   unit       - (IN)BCM device number
 *     index      - (IN)Mtp index.
 *     trunk_arr  - (IN)Trunk members array. 
 *     flags      - (IN)Filled entry flags(BCM_MIRROR_PORT_INGRESS/EGRESS
 *                    or both. In case both flags are specied
 *                    ingress & egress configuration is assumed to be
 *                    idential.
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_fbx_mtp_init(int unit, int index, bcm_gport_t *trunk_arr, int flags)
{
    bcm_gport_t         mirror_dest;
    bcm_port_t          port_out;
    bcm_module_t        mod_out;
    _bcm_mtp_config_p   mtp_cfg;
    int                 offset;
    int                 idx, id;
    bcm_trunk_t         trunk = BCM_TRUNK_INVALID;
    bcm_module_t        modid = 0;
    bcm_port_t          port = -1;
    uint32              mtp = 0;
    int                 isLocal;
    int                 member_count = 0;

    /* Input parameters check */ 
    if (NULL == trunk_arr) {
        return (BCM_E_PARAM);
    }

    /* Get mtp configuration structure by direction & index. */
    mtp_cfg = MIRROR_CONFIG_MTP(unit, index, flags);

    /* Parse destination trunk / port & module. */
    mirror_dest = MIRROR_DEST_GPORT(unit, mtp_cfg->dest_id);
    if (BCM_GPORT_IS_TRUNK(mirror_dest)) {
        trunk = BCM_GPORT_TRUNK_GET(mirror_dest);
        BCM_IF_ERROR_RETURN
            (bcm_esw_trunk_get(unit, trunk, NULL, 0, NULL, &member_count));
    } else {
        /* If MODPORT GPORT provided resolve already had happened */
        if (BCM_GPORT_IS_MODPORT(mirror_dest)) {
        modid = BCM_GPORT_MODPORT_MODID_GET(mirror_dest);
        port  = BCM_GPORT_MODPORT_PORT_GET(mirror_dest);
        } else {
            BCM_IF_ERROR_RETURN(
                _bcm_esw_gport_resolve(unit, mirror_dest, &modid,  
                                       &port, &trunk, &id));
            if (BCM_TRUNK_INVALID != trunk || id != -1) {
                return BCM_E_PORT;
            }
        }
        BCM_IF_ERROR_RETURN(
            _bcm_esw_modid_is_local(unit, modid, &isLocal));
        if (TRUE == isLocal) {
            BCM_IF_ERROR_RETURN(
            _bcm_esw_stk_modmap_map(unit,BCM_STK_MODMAP_SET, modid, port,
                                        &modid, &port));
        }
    }

    /* Hw buffer preparation. */
    if (soc_feature(unit, soc_feature_trunk_group_overlay)) {
        if (BCM_GPORT_IS_TRUNK(mirror_dest)) {
            soc_IM_MTP_INDEXm_field32_set(unit, &mtp, Tf, 1);
            soc_IM_MTP_INDEXm_field32_set(unit, &mtp, TGIDf, trunk);
        } else {
            soc_IM_MTP_INDEXm_field32_set(unit, &mtp, MODULE_IDf, modid);
            soc_IM_MTP_INDEXm_field32_set(unit, &mtp, PORT_NUMf, port);
        }
    } else {
        if (BCM_GPORT_IS_TRUNK(mirror_dest)) {
            modid = BCM_TRUNK_TO_MODIDf(unit, trunk);
            port  = BCM_TRUNK_TO_TGIDf(unit, trunk);
        }
        soc_IM_MTP_INDEXm_field32_set(unit, &mtp, MODULE_IDf, modid);
        soc_IM_MTP_INDEXm_field32_set(unit, &mtp, PORT_TGIDf, port);
    }

    /* HW write. based on mirrored traffic direction. */
    if (flags & BCM_MIRROR_PORT_INGRESS) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_write(unit, IM_MTP_INDEXm, MEM_BLOCK_ALL, index, &mtp));
    }

    /* EM_MTP_INDEX has same layout as IM_MTP_INDEX */
    if (flags & BCM_MIRROR_PORT_EGRESS) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_write(unit, EM_MTP_INDEXm, MEM_BLOCK_ALL, index, &mtp));
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    /* EP_REDIRECT_EM_MTP_INDEX has same layout as IM_MTP_INDEX */
    if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_write(unit, EP_REDIRECT_EM_MTP_INDEXm,
                           MEM_BLOCK_ALL, index, &mtp));
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    if (BCM_GPORT_IS_TRUNK(mirror_dest) && (0 == member_count)) {
        /* Traffic shouldn't proceed, don't configure other elements */
        return BCM_E_NONE;
    }

    offset = index * BCM_SWITCH_TRUNK_MAX_PORTCNT;
    for (idx = 0; idx < BCM_SWITCH_TRUNK_MAX_PORTCNT; idx++, offset++) {
        mtp = 0;
        if (BCM_GPORT_IS_MODPORT(trunk_arr[idx])) {
            modid = BCM_GPORT_MODPORT_MODID_GET(trunk_arr[idx]);
            port = BCM_GPORT_MODPORT_PORT_GET(trunk_arr[idx]);
        } else {
            BCM_IF_ERROR_RETURN(
                _bcm_esw_gport_resolve(unit, trunk_arr[idx], &modid,  
                                       &port, &trunk, &id));
            if (BCM_TRUNK_INVALID != trunk || id != -1) {
                return BCM_E_PORT;
            }
        }
        BCM_IF_ERROR_RETURN
            (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET, modid, port, 
                                    &mod_out, &port_out));
         
        soc_EGR_IM_MTP_INDEXm_field32_set(unit, &mtp, MTP_DST_PORTf, port_out);
        soc_EGR_IM_MTP_INDEXm_field32_set(unit, &mtp, MTP_DST_MODIDf, mod_out);

        /* HW write. based on mirrored traffic direction. */
        if (flags & BCM_MIRROR_PORT_INGRESS) {
            BCM_IF_ERROR_RETURN 
                (soc_mem_write(unit, EGR_IM_MTP_INDEXm, MEM_BLOCK_ALL,
                               offset, &mtp));
        }

        /* EGR_EM_MTP_INDEX has same layout as EGR_IM_MTP_INDEX */
        if (flags & BCM_MIRROR_PORT_EGRESS) {
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, EGR_EM_MTP_INDEXm, MEM_BLOCK_ALL,
                               offset, &mtp));
        }

#ifdef BCM_TRIUMPH2_SUPPORT
        /* EGR_EP_REDIRECT_EM_MTP_INDEX has same layout as others */
        if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, EGR_EP_REDIRECT_EM_MTP_INDEXm,
                               MEM_BLOCK_ALL, offset, &mtp));
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
    }

#if defined(BCM_TRX_SUPPORT)
    if((MIRROR_DEST(unit, mtp_cfg->dest_id))->flags) {
        BCM_IF_ERROR_RETURN(_bcm_trx_mirror_tunnel_set(unit, index, 
                                                       trunk_arr, flags));
    }
#endif /* BCM_TRX_SUPPORT */

    return (BCM_E_NONE);
}

/*
 * Function:
 *	    _bcm_fbx_mtp_reset
 * Purpose:
 *	   Reset mirror target port for FBX devices. 
 * Parameters:
 *	   unit       - (IN)BCM device number
 *     index      - (IN)Mtp index.
 *     flags      - (IN)Filled entry flags(BCM_MIRROR_PORT_INGRESS/EGRESS
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_fbx_mtp_reset(int unit, int index, int flags)
{
    int             offset;
    int             idx;
    uint32          mtp = 0;
    uint32          mirror_select, mtp_type;

    /* HW write. based on mirrored traffic direction. */
    if (flags & BCM_MIRROR_PORT_INGRESS) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_write(unit, IM_MTP_INDEXm, MEM_BLOCK_ALL, index, &mtp));
    }

    /* EM_MTP_INDEX has same layout as IM_MTP_INDEX */
    if (flags & BCM_MIRROR_PORT_EGRESS) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_write(unit, EM_MTP_INDEXm, MEM_BLOCK_ALL, index, &mtp));
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    /* EP_REDIRECT_EM_MTP_INDEX has same layout as IM_MTP_INDEX */
    if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_write(unit, EP_REDIRECT_EM_MTP_INDEXm,
                           MEM_BLOCK_ALL, index, &mtp));
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    /* Reset MTP_SELECT register to 0 if exists */
    if (SOC_REG_FIELD_VALID(unit, MIRROR_SELECTr, MTP_TYPEf)) {
        BCM_IF_ERROR_RETURN(READ_MIRROR_SELECTr(unit, &mirror_select));
        mtp_type = soc_reg_field_get(unit, MIRROR_SELECTr, 
                                     mirror_select, MTP_TYPEf);
        mtp_type &= ~(1 << index);
        soc_reg_field_set(unit, MIRROR_SELECTr, &mirror_select, 
                          MTP_TYPEf, mtp_type);
        BCM_IF_ERROR_RETURN(WRITE_MIRROR_SELECTr( unit, mirror_select));
    }


#ifdef BCM_TRX_SUPPORT
        if (SOC_MEM_IS_VALID(unit, EGR_ERSPANm)) {
            egr_erspan_entry_t      hw_buf;
            int                     egr_idx;

            /* Reset hw buffer. */
            sal_memset(&hw_buf, 0, sizeof(egr_erspan_entry_t));
            
            /* Get egr_erspan index by direction */
            if (flags & BCM_MIRROR_PORT_INGRESS) {
                egr_idx = index;
            } else if (flags & BCM_MIRROR_PORT_EGRESS) {
                egr_idx = index + 4;
            } else { /* True Egress */
                egr_idx = index + 8;
            }
            BCM_IF_ERROR_RETURN(
               soc_mem_write(unit, EGR_ERSPANm, MEM_BLOCK_ALL,
                             egr_idx, &hw_buf));
        }
#endif /* BCM_TRX_SUPPORT */

    offset = index * BCM_SWITCH_TRUNK_MAX_PORTCNT;
    for (idx = 0; idx < BCM_SWITCH_TRUNK_MAX_PORTCNT; idx++, offset++) {
        mtp = 0;

        /* HW write. based on mirrored traffic direction. */
        if (flags & BCM_MIRROR_PORT_INGRESS) {
            BCM_IF_ERROR_RETURN 
                (soc_mem_write(unit, EGR_IM_MTP_INDEXm, MEM_BLOCK_ALL,
                               offset, &mtp));
        }

        /* EGR_EM_MTP_INDEX has same layout as EGR_IM_MTP_INDEX */
        if (flags & BCM_MIRROR_PORT_EGRESS) {
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, EGR_EM_MTP_INDEXm, MEM_BLOCK_ALL,
                               offset, &mtp));
        }

#ifdef BCM_TRIUMPH2_SUPPORT
        /* EGR_EP_REDIRECT_EM_MTP_INDEX has same layout as others */
        if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, EGR_EP_REDIRECT_EM_MTP_INDEXm,
                               MEM_BLOCK_ALL, offset, &mtp));
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
    }
    return (BCM_E_NONE);
}
#endif /* BCM_FIREBOLT_SUPPORT */

/*
 * Function:
 *	    _bcm_xgs3_mtp_reset
 * Purpose:
 *	   Reset mirror target port for XGS3 devices. 
 * Parameters:
 *	   unit     - (IN)BCM device number
 *     index    - (IN)Mtp index.
 *     flags    - (IN)Filled entry flags(BCM_MIRROR_PORT_INGRESS/EGRESS
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_xgs3_mtp_reset(int unit, int index, int flags)
{
    int rv = BCM_E_UNAVAIL;      /* Operation return status. */
#if defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit, soc_feature_mirror_control_mem)) {
        return _bcm_td_mtp_reset(unit, index, flags);
    }
#endif
#if defined(BCM_FIREBOLT_SUPPORT) 
    if (SOC_IS_FBX(unit)) {
       rv = _bcm_fbx_mtp_reset(unit, index, flags);
    }
#endif /* BCM_FIREBOLT_SUPPORT */
    return rv;
}


/*
 * Function:
 *	    _bcm_xgs3_mtp_init
 * Purpose:
 *	   Initialize mirror target port for XGS3 devices. 
 * Parameters:
 *	   unit     - (IN)BCM device number
 *     index    - (IN)Mtp index.
 *     flags    - (IN)Filled entry flags(BCM_MIRROR_PORT_INGRESS/EGRESS
 *                    or both. In case both flags are specied
 *                    ingress & egress configuration is assumed to be
 *                    idential.
 * Returns:
 *	   BCM_E_XXX
 */
STATIC int
_bcm_xgs3_mtp_init(int unit, int index, int flags)
{
    _bcm_mtp_config_p    mtp_cfg;
                         /* Initialized to 0's to turn off false coverity alarm */
    bcm_gport_t          gport[BCM_SWITCH_TRUNK_MAX_PORTCNT] = {0}; 
    bcm_gport_t          mirror_dest;
    int                  idx;
    int                  rv = BCM_E_UNAVAIL;
    int                  active_member_count = 0;
    bcm_trunk_member_t   active_member_array[BCM_SWITCH_TRUNK_MAX_PORTCNT];
    
    mtp_cfg = MIRROR_CONFIG_MTP(unit, index, flags);

    /* Destination port/trunk id validation. */
    mirror_dest = MIRROR_DEST_GPORT(unit, mtp_cfg->dest_id);
    if (BCM_GPORT_IS_TRUNK(mirror_dest)) {
        rv = _bcm_trunk_id_validate(unit, BCM_GPORT_TRUNK_GET(mirror_dest));
        if (BCM_FAILURE(rv)) {
            return (BCM_E_PORT);
        }

        /* get only active trunk members and the count */
        rv = _bcm_esw_trunk_active_member_get(unit, 
                                              BCM_GPORT_TRUNK_GET(mirror_dest),
                                              NULL,
                                              BCM_SWITCH_TRUNK_MAX_PORTCNT,
                                              active_member_array, 
                                              &active_member_count);
        if (BCM_FAILURE(rv)) {
            return (BCM_E_PORT);
        }

        if (0 < active_member_count) {
            /* Fill gport array with trunk member ports. */
            for (idx = 0; idx < BCM_SWITCH_TRUNK_MAX_PORTCNT; idx++) {
                 gport[idx] = 
                       active_member_array[idx % active_member_count].gport;
            }
        } /* else must pass along the trunk ID for zero member trunks */
    } else {
        bcm_module_t    modid;
        bcm_port_t      port;
        bcm_trunk_t     tgid;
        int             id;

        BCM_IF_ERROR_RETURN(
            _bcm_esw_gport_resolve(unit, mirror_dest,  &modid,  
                                   &port, &tgid,  &id));
        if (BCM_TRUNK_INVALID != tgid || id != -1) {
            return BCM_E_PORT;
        }
        if (!SOC_MODID_ADDRESSABLE(unit, modid)) {
            return (BCM_E_BADID);
        }
        if (!SOC_PORT_ADDRESSABLE(unit, port)) {
            return (BCM_E_PORT);
        }
        /* Fill gport array with destination port only. */
        for (idx = 0; idx < BCM_SWITCH_TRUNK_MAX_PORTCNT; idx++) {
            gport[idx] = mirror_dest;
        }
    }
#if defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit, soc_feature_mirror_control_mem)) {
        return _bcm_trident_mtp_init(unit, index, gport, flags);
    } 
#endif
#if defined(BCM_FIREBOLT_SUPPORT) 
    if (SOC_IS_FBX(unit)) {
       rv = _bcm_fbx_mtp_init(unit, index, gport, flags);
    }
#endif /* BCM_FIREBOLT_SUPPORT */
    return rv;
}

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
/*
 * Function:
 *      _bcm_tr2_mirror_trunk_update
 * Description:
 *      Update mtp programming based on trunk port membership.
 * Parameters:
 *      unit       - (IN)  BCM device number
 *      tid        - (IN)  Trunk id. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_trunk_update(int unit, bcm_trunk_t tid)
{
    int idx;                        /* Mtp iteration index.     */
    bcm_gport_t  gport;             /* Mirror destination.      */
    bcm_gport_t  mirror_dest_id;    /* Mirror destination.      */
    int rv = BCM_E_NONE;            /* Operation return status. */
    int egress;                     /* Mirror ingress/egres     */

    /* Initilize mirror destination. */
    BCM_GPORT_TRUNK_SET(gport, tid);

    MIRROR_LOCK(unit);
    /* Ingress mirroring destions update */
    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) {
            if (MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx)) { 
                mirror_dest_id = MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx);
                egress = MIRROR_CONFIG_SHARED_MTP(unit, idx).egress;
                if (MIRROR_DEST_GPORT(unit, mirror_dest_id) == gport) {
                    rv = _bcm_xgs3_mtp_init(unit, idx, (TRUE == egress) ? 
                                            BCM_MIRROR_PORT_EGRESS : 
                                            BCM_MIRROR_PORT_INGRESS);
                    if (BCM_FAILURE(rv)) {
                        break;
                    }
                }
            }
        }
    } else {
        /* Check all used ingress mirror destinations. */
        for (idx = 0; idx < MIRROR_CONFIG(unit)->ing_mtp_count; idx++) {
            if (MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, idx)) {
                mirror_dest_id = MIRROR_CONFIG_ING_MTP_DEST(unit, idx);
                if (MIRROR_DEST_GPORT(unit, mirror_dest_id) == gport) {
                    rv = _bcm_xgs3_mtp_init(unit, idx,
                                            BCM_MIRROR_PORT_INGRESS);
                    if (BCM_FAILURE(rv)) {
                        break;
                    }
                }
            }
        }

        if (BCM_SUCCESS(rv)) {
            /* Check all used egress mirror destinations. */
            for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_mtp_count; idx++) {
                if (MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx)) {
                    mirror_dest_id = MIRROR_CONFIG_EGR_MTP_DEST(unit, idx);
                    if (MIRROR_DEST_GPORT(unit, mirror_dest_id) == gport) {
                        rv = _bcm_xgs3_mtp_init(unit, idx,
                                                BCM_MIRROR_PORT_EGRESS);
                        if (BCM_FAILURE(rv)) {
                            break;
                        }
                    }
                }
            }
        }
    }

    /* True egress mirroring destinations update */
    if (BCM_SUCCESS(rv) &&
        soc_feature(unit, soc_feature_egr_mirror_true)) {
        for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_true_mtp_count; idx++) {
            if (MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, idx)) { 
                mirror_dest_id = MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, idx);
                if (MIRROR_DEST_GPORT(unit, mirror_dest_id) == gport) {
                    rv = _bcm_xgs3_mtp_init(unit, idx,
                                            BCM_MIRROR_PORT_EGRESS_TRUE);
                    if (BCM_FAILURE(rv)) {
                        break;
                    }
                }
            }
        }
    }

    MIRROR_UNLOCK(unit);
    return (rv);
}
#endif /* BCM_TRIUMPH2_SUPPORT || BCM_TRIDENT_SUPPORT */

/*
 * Function:
 *      _bcm_xgs3_mirror_trunk_update
 * Description:
 *      Update mtp programming based on trunk port membership.
 * Parameters:
 *      unit       - (IN)  BCM device number
 *      tid        - (IN)  Trunk id. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_xgs3_mirror_trunk_update(int unit, bcm_trunk_t tid)
{
    int idx;                        /* Mtp iteration index.     */
    bcm_gport_t  gport;             /* Mirror destination.      */
    bcm_gport_t  mirror_dest_id;    /* Mirror destination.      */
    int rv = BCM_E_NONE;            /* Operation return status. */

    /* Check if mirroring enabled on the device. */
    if (!MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        return _bcm_tr2_mirror_trunk_update(unit, tid);
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    /* Initilize mirror destination. */
    BCM_GPORT_TRUNK_SET(gport, tid);

    MIRROR_LOCK(unit);
    /* Ingress mirroring destions update */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->ing_mtp_count; idx++) {
        if (MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, idx)) { 
            mirror_dest_id = MIRROR_CONFIG_ING_MTP_DEST(unit, idx);
            if (MIRROR_DEST_GPORT(unit, mirror_dest_id) == gport) {
                rv = _bcm_xgs3_mtp_init(unit, idx, BCM_MIRROR_PORT_INGRESS);
                if (BCM_FAILURE(rv)) {
                    MIRROR_UNLOCK(unit);
                    return (rv);
                }
            }
        }
    }

    /* Egress mirroring destinations update */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_mtp_count; idx++) {
        if (MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx)) { 
            mirror_dest_id = MIRROR_CONFIG_EGR_MTP_DEST(unit, idx);
            if (MIRROR_DEST_GPORT(unit, mirror_dest_id) == gport) {
                rv = _bcm_xgs3_mtp_init(unit, idx, BCM_MIRROR_PORT_EGRESS);
                if (BCM_FAILURE(rv)) {
                    break;
                }
            }
        }
    }

    MIRROR_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      _bcm_tr2_mirror_ingress_mtp_reserve
 * Description:
 *      Reserve a mirror-to port for Triumph2 like devices
 * Parameters:
 *      unit       - (IN)  BCM device number
 *      dest_id    - (IN)  Mirror destination id.
 *      index_used - (OUT) MTP index reserved
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the a mirror-to port is reserved more than once
 *      (without being unreserved) then the same MTP index 
 *      will be returned for each call.
 */
STATIC int
_bcm_tr2_mirror_ingress_mtp_reserve(int unit, bcm_gport_t dest_id, 
                                     int *index_used)
{
    int rv, idx;
    int rspan = FALSE; 

    /* Look for existing MTP in use */
    rv = _bcm_tr2_mirror_shared_mtp_match(unit, dest_id, FALSE, &idx);
    if (BCM_SUCCESS(rv)) {
        MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx)++;
        *index_used = idx;
        return (rv);
    }

#if defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
        bcm_mirror_destination_t mdest;    /* Mirror destination info. */

        /* Get mirror destination descriptor. */
        BCM_IF_ERROR_RETURN
            (bcm_esw_mirror_destination_get(unit, dest_id, &mdest));

        rspan = (0 != (mdest.flags & BCM_MIRROR_DEST_TUNNEL_L2));
    }
#endif /* BCM_TRIDENT_SUPPORT */

    /* Reserve free index */
    for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) {
        if (0 == MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx)) {
            if (rspan && (0 == idx)) {
                /* Do not use MTP 0 for RSPAN on Trident
                 * and later devices */
                continue;
            }
            break;
        }
    }

    if (idx < BCM_MIRROR_MTP_COUNT) {
        /* Mark mtp as used. */
        MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx) = dest_id;
        MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx)++;
        MIRROR_CONFIG_SHARED_MTP(unit, idx).egress = FALSE;
        MIRROR_DEST_REF_COUNT(unit, dest_id)++;

        /* Write MTP registers */
        rv = _bcm_xgs3_mtp_init(unit, idx, BCM_MIRROR_PORT_INGRESS);
        if (BCM_FAILURE(rv)) {
            MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx) = BCM_GPORT_INVALID;
            MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx) = 0;
            if (MIRROR_DEST_REF_COUNT(unit, dest_id) > 0) {
                MIRROR_DEST_REF_COUNT(unit, dest_id)--;
            }
        } else {
            if (SOC_REG_FIELD_VALID(unit, MIRROR_SELECTr, MTP_TYPEf)) {
                uint32 rval, fval;
                BCM_IF_ERROR_RETURN(READ_MIRROR_SELECTr(unit, &rval));
                fval = soc_reg_field_get(unit, MIRROR_SELECTr, rval, MTP_TYPEf);
                fval &= ~(1 << idx); 
                soc_reg_field_set(unit, MIRROR_SELECTr, &rval, MTP_TYPEf, fval);
                BCM_IF_ERROR_RETURN(WRITE_MIRROR_SELECTr(unit, rval));
                if (SOC_REG_FIELD_VALID(unit, EGR_MIRROR_SELECTr, MTP_TYPEf)) {
                    BCM_IF_ERROR_RETURN(WRITE_EGR_MIRROR_SELECTr(unit, rval));
                }        
            }

        }
        *index_used = idx;
    } else {
        rv = BCM_E_RESOURCE;
    } 
    return (rv);
}

/*
 * Function:
 *      _bcm_xgs3_mirror_ingress_mtp_reserve
 * Description:
 *      Reserve a mirror-to port
 * Parameters:
 *      unit       - (IN)  BCM device number
 *      dest_id    - (IN)  Mirror destination id.
 *      index_used - (OUT) MTP index reserved
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the a mirror-to port is reserved more than once
 *      (without being unreserved) then the same MTP index 
 *      will be returned for each call.
 */
STATIC int
_bcm_xgs3_mirror_ingress_mtp_reserve(int unit, bcm_gport_t dest_id, 
                                     int *index_used)
{
    int rv;                                  /* Operation return status. */
    int idx = _BCM_MIRROR_INVALID_MTP;       /* Mtp iteration index.     */
    int rspan = FALSE; 

    /* Input parameters check. */
    if (NULL == index_used) {
        return (BCM_E_PARAM);
    }

    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        return _bcm_tr2_mirror_ingress_mtp_reserve(unit, dest_id, index_used);
    }
    /* Look for existing MTP in use */
    rv = _bcm_esw_mirror_ingress_mtp_match(unit, dest_id, &idx);
    if (BCM_SUCCESS(rv)) {
        MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, idx)++;
        *index_used = idx;
        return (rv);
    }

#if defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
        bcm_mirror_destination_t mdest;    /* Mirror destination info. */

        /* Get mirror destination descriptor. */
        BCM_IF_ERROR_RETURN
            (bcm_esw_mirror_destination_get(unit, dest_id, &mdest));

        rspan = (0 != (mdest.flags & BCM_MIRROR_DEST_TUNNEL_L2));
    }
#endif /* BCM_TRIDENT_SUPPORT */

    /* Reserve free index */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->ing_mtp_count; idx++) {
        if (0 == MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, idx)) {
            if (rspan && (0 == idx)) {
                /* Do not use MTP 0 for RSPAN on Trident
                 * and later devices */
                continue;
            }
            break;
        }
    }

    if (idx < MIRROR_CONFIG(unit)->ing_mtp_count) {
        /* Mark mtp as used. */
        MIRROR_CONFIG_ING_MTP_DEST(unit, idx) = dest_id;
        MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, idx)++;
        MIRROR_DEST_REF_COUNT(unit, dest_id)++;

        /* Write MTP registers */
        rv = _bcm_xgs3_mtp_init(unit, idx, BCM_MIRROR_PORT_INGRESS);
        if (BCM_FAILURE(rv)) {
            MIRROR_CONFIG_ING_MTP_DEST(unit, idx) = BCM_GPORT_INVALID;
            MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, idx) = 0;
            if (MIRROR_DEST_REF_COUNT(unit, dest_id) > 0) {
                MIRROR_DEST_REF_COUNT(unit, dest_id)--;
            }
        }
        *index_used = idx;
    } else {
        rv = BCM_E_RESOURCE;
    } 

    return (rv);
}

/*
 * Function:
 *      _bcm_tr2_mirror_egress_mtp_reserve
 * Description:
 *      Reserve a mirror-to port for Triumph2 like devices
 * Parameters:
 *      unit       - (IN)  BCM device number
 *      dest_id    - (IN)  Mirror destination id.
 *      is_port    - (IN)  Reservation is for port based mirror
 *      index_used - (OUT) MTP index reserved
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the a mirror-to port is reserved more than once
 *      (without being unreserved) then the same MTP index 
 *      will be returned for each call.
 */
STATIC int
_bcm_tr2_mirror_egress_mtp_reserve(int unit, bcm_gport_t dest_id, 
                                   int is_port, int *index_used)
{
    int port_limit = 0;
    int rv, idx;
    int rspan = FALSE; 

    /* Look for existing MTP in use */
    rv = _bcm_tr2_mirror_shared_mtp_match(unit, dest_id, TRUE, &idx);
    if (BCM_SUCCESS(rv)) {
        if (is_port) {
            MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx)++;
        } else {
            MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx) +=
                (1 << BCM_MIRROR_MTP_REF_FP_OFFSET);
        }
        *index_used = idx;
        return (rv);
    }

#if defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
        bcm_mirror_destination_t mdest;    /* Mirror destination info. */

        /* Get mirror destination descriptor. */
        BCM_IF_ERROR_RETURN
            (bcm_esw_mirror_destination_get(unit, dest_id, &mdest));

        rspan = (0 != (mdest.flags & BCM_MIRROR_DEST_TUNNEL_L2));
    }
#endif /* BCM_TRIDENT_SUPPORT */

    /* Reserve free index */
    if (MIRROR_CONFIG(unit)->port_em_mtp_count > 1) {
        for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) {
            if (is_port && 
                (MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx) & 
                 BCM_MIRROR_MTP_REF_PORT_MASK) &&
                (TRUE == MIRROR_CONFIG_SHARED_MTP(unit, idx).egress)) {
                port_limit++;
                if (port_limit > MIRROR_CONFIG(unit)->port_em_mtp_count) {
                    return (BCM_E_RESOURCE);
                }
            }
            if (0 == MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx)) {
                if (rspan && (0 == idx)) {
                    /* Do not use MTP 0 for RSPAN on Trident
                     * and later devices */
                    continue;
                }
                break;
            }
        }
    } else {
        /* Not directed mirroring mode */
        if (0 != MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit,
              BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX)) {
            return (BCM_E_RESOURCE);
        } else {
            idx = BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX;
        }
    }

    if (idx < BCM_MIRROR_MTP_COUNT) {
        /* Mark mtp as used. */
        MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx) = dest_id;
        if (is_port) {
            MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx)++;
        } else {
            MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx) += (1 << BCM_MIRROR_MTP_REF_FP_OFFSET);
        }
        MIRROR_CONFIG_SHARED_MTP(unit, idx).egress = TRUE;
        MIRROR_DEST_REF_COUNT(unit, dest_id)++;

        /* Write MTP registers */
        rv = _bcm_xgs3_mtp_init(unit, idx, BCM_MIRROR_PORT_EGRESS);
        if (BCM_FAILURE(rv)) {
            MIRROR_CONFIG_SHARED_MTP_DEST(unit, idx) = BCM_GPORT_INVALID;
            MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, idx) = 0;
            if (MIRROR_DEST_REF_COUNT(unit, dest_id) > 0) {
                MIRROR_DEST_REF_COUNT(unit, dest_id)--;
            }
        } else {
            if (SOC_REG_FIELD_VALID(unit, MIRROR_SELECTr, MTP_TYPEf)) {
                uint32 rval, fval;
                BCM_IF_ERROR_RETURN(READ_MIRROR_SELECTr(unit, &rval));
                fval = soc_reg_field_get(unit, MIRROR_SELECTr, rval, MTP_TYPEf);
                fval |= (1 << idx);
                soc_reg_field_set(unit, MIRROR_SELECTr, &rval, MTP_TYPEf, fval);
                BCM_IF_ERROR_RETURN(WRITE_MIRROR_SELECTr(unit, rval));
                if (SOC_REG_FIELD_VALID(unit, EGR_MIRROR_SELECTr, MTP_TYPEf)) {
                    BCM_IF_ERROR_RETURN(WRITE_EGR_MIRROR_SELECTr(unit, rval));
                }        
            }

        }
        *index_used = idx;
    } else {
        rv = BCM_E_RESOURCE;
    } 
    return (rv);
}

/*
 * Function:
 *      _bcm_xgs3_mirror_egress_mtp_reserve
 * Description:
 *      Reserve a mirror-to port
 * Parameters:
 *      unit       - (IN)  BCM device number
 *      dest_id    - (IN)  Mirror destination id.
 *      is_port    - (IN)  Reservation is for port based mirror
 *      index_used - (OUT) MTP index reserved
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the a mirror-to port is reserved more than once
 *      (without being unreserved) then the same MTP index 
 *      will be returned for each call.
 */
STATIC int
_bcm_xgs3_mirror_egress_mtp_reserve(int unit, bcm_gport_t dest_id, int is_port,
                                     int *index_used)
{
    int rv;                                  /* Operation return status.*/
    int idx = _BCM_MIRROR_INVALID_MTP;       /* Mtp iteration index.    */
    int port_limit = 0;    /* How many mtp in use by port based mirroring */
    int rspan = FALSE; 


    /* Input parameters check. */
    if (NULL == index_used) {
        return (BCM_E_PARAM);
    }

    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        return _bcm_tr2_mirror_egress_mtp_reserve(unit, dest_id, is_port, 
                                                  index_used);
    }

    /* Look for existing MTP in use */
    rv = _bcm_esw_mirror_egress_mtp_match(unit, dest_id, &idx);
    if (BCM_SUCCESS(rv)) {
        if (is_port) {
            MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx)++;
        } else {
            MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx) +=
                (1 << BCM_MIRROR_MTP_REF_FP_OFFSET);
        }
        *index_used = idx;
        return (rv);
    }

#if defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
        bcm_mirror_destination_t mdest;    /* Mirror destination info. */

        /* Get mirror destination descriptor. */
        BCM_IF_ERROR_RETURN
            (bcm_esw_mirror_destination_get(unit, dest_id, &mdest));

        rspan = (0 != (mdest.flags & BCM_MIRROR_DEST_TUNNEL_L2));
    }
#endif /* BCM_TRIDENT_SUPPORT */

    /* Reserve free index */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_mtp_count; idx++) {
        if (is_port && 
            (MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx) & 
             BCM_MIRROR_MTP_REF_PORT_MASK)) {
            port_limit++;
            if (port_limit > MIRROR_CONFIG(unit)->port_em_mtp_count) {
                return (BCM_E_RESOURCE);
            }
        }
        if (0 == MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx)) {
            if (rspan && (0 == idx)) {
                /* Do not use MTP 0 for RSPAN on Trident
                 * and later devices */
                continue;
            }
            break;
        }
    }

    if (idx < MIRROR_CONFIG(unit)->egr_mtp_count) {
        /* Mark mtp as used. */
        MIRROR_CONFIG_EGR_MTP_DEST(unit, idx) = dest_id;
        if (is_port) {
            MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx)++;
        } else {
            MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx) +=
                (1 << BCM_MIRROR_MTP_REF_FP_OFFSET);
        }
        MIRROR_DEST_REF_COUNT(unit, dest_id)++;

        /* Write MTP registers */
        rv = _bcm_xgs3_mtp_init(unit, idx, BCM_MIRROR_PORT_EGRESS);
        if (BCM_FAILURE(rv)) {
            MIRROR_CONFIG_EGR_MTP_DEST(unit, idx) = BCM_GPORT_INVALID;
            MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, idx) = 0;
            if (MIRROR_DEST_REF_COUNT(unit, dest_id) > 0) {
                MIRROR_DEST_REF_COUNT(unit, dest_id)--;
            }
        } 
        *index_used = idx;
    } else {
        rv = BCM_E_RESOURCE;
    } 

    return (rv);
}

#ifdef BCM_TRIUMPH2_SUPPORT
/*
 * Function:
 *      _bcm_xgs3_mirror_egress_true_mtp_reserve
 * Description:
 *      Reserve a mirror-to port
 * Parameters:
 *      unit       - (IN)  BCM device number
 *      dest_id    - (IN)  Mirror destination id.
 *      index_used - (OUT) MTP index reserved
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the a mirror-to port is reserved more than once
 *      (without being unreserved) then the same MTP index 
 *      will be returned for each call.
 */
STATIC int
_bcm_xgs3_mirror_egress_true_mtp_reserve(int unit, bcm_gport_t dest_id,
                                     int *index_used)
{
    int rv;                                  /* Operation return status.*/
    int idx = _BCM_MIRROR_INVALID_MTP;       /* Mtp iteration index.    */
    int rspan = FALSE; 

    /* Input parameters check. */
    if (NULL == index_used) {
        return (BCM_E_PARAM);
    }

    /* Look for existing MTP in use */
    rv = _bcm_esw_mirror_egress_true_mtp_match(unit, dest_id, &idx);
    if (BCM_SUCCESS(rv)) {
        MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, idx)++;
        *index_used = idx;
        return (rv);
    }

#if defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
        bcm_mirror_destination_t mdest;    /* Mirror destination info. */

        /* Get mirror destination descriptor. */
        BCM_IF_ERROR_RETURN
            (bcm_esw_mirror_destination_get(unit, dest_id, &mdest));

        rspan = (0 != (mdest.flags & BCM_MIRROR_DEST_TUNNEL_L2));
    }
#endif /* BCM_TRIDENT_SUPPORT */

    /* Reserve free index */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_true_mtp_count; idx++) {
        if (0 == MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, idx)) {
            if (rspan && (0 == idx)) {
                /* Do not use MTP 0 for RSPAN on Trident
                 * and later devices */
                continue;
            }
            break;
        }
    }

    if (idx < MIRROR_CONFIG(unit)->egr_true_mtp_count) {
        /* Mark mtp as used. */
        MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, idx) = dest_id;
        MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, idx)++;
        MIRROR_DEST_REF_COUNT(unit, dest_id)++;

        /* Write MTP registers */
        rv = _bcm_xgs3_mtp_init(unit, idx, BCM_MIRROR_PORT_EGRESS_TRUE);
        if (BCM_FAILURE(rv)) {
            MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, idx) = BCM_GPORT_INVALID;
            MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, idx) = 0;
            if (MIRROR_DEST_REF_COUNT(unit, dest_id) > 0) {
                MIRROR_DEST_REF_COUNT(unit, dest_id)--;
            }
        }
        *index_used = idx;
    } else {
        rv = BCM_E_RESOURCE;
    } 

    return (rv);
}
#endif /* BCM_TRIUMPH2_SUPPORT */

/*
 * Function:
 *      _bcm_tr2_mirror_ingress_mtp_unreserve
 * Description:
 *      Free ingress  mirror-to port for Triumph_2 like devices
 * Parameters:
 *      unit       - (IN) BCM device number
 *      mtp_index  - (IN) MTP index. 
 *      egress     - (IN) Ingress/Egress indication
 *      is_port    - (IN) Port based mirrorring indication
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Mtp index completely freed only when reference count gets to 0. 
 */
STATIC int
_bcm_tr2_mirror_mtp_unreserve(int unit, int mtp_index, int egress, int is_port)
{
    int rv = BCM_E_NONE;                     /* Operation return status.*/
    bcm_gport_t mirror_dest;                 /* Mirror destination id.  */

    /* Input parameters check. */
    if ((mtp_index < 0) || (mtp_index >= BCM_MIRROR_MTP_COUNT)) {
        return (BCM_E_PARAM);
    }

    /* If MTP is not in use - do nothing */
    if (0 == MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, mtp_index)) {
        return (rv);
    }

    /* Decrement mtp index reference count. */
    if ((MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, mtp_index) > 0) &&
        (egress == MIRROR_CONFIG_SHARED_MTP(unit, mtp_index).egress)) {
        if (is_port) {
            MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, mtp_index)--;
        } else {
            MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, mtp_index) -= (1 << BCM_MIRROR_MTP_REF_FP_OFFSET);
        }
    }

    if (0 == MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, mtp_index)) {
        /* Write MTP registers */
        mirror_dest = MIRROR_CONFIG_SHARED_MTP_DEST(unit, mtp_index);
        rv = _bcm_xgs3_mtp_reset(unit, mtp_index, egress ? BCM_MIRROR_PORT_EGRESS : BCM_MIRROR_PORT_INGRESS);
        MIRROR_CONFIG_SHARED_MTP_DEST(unit, mtp_index) = BCM_GPORT_INVALID;
        if (MIRROR_DEST_REF_COUNT(unit, mirror_dest) > 0) {
            MIRROR_DEST_REF_COUNT(unit, mirror_dest)--;
        }
    }
    return (rv);
}


/*
 * Function:
 *      _bcm_xgs3_mirror_ingress_mtp_unreserve
 * Description:
 *      Free ingress  mirror-to port
 * Parameters:
 *      unit       - (IN) BCM device number
 *      mtp_index  - (IN) MTP index. 
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Mtp index completely freed only when reference count gets to 0. 
 */
STATIC int
_bcm_xgs3_mirror_ingress_mtp_unreserve(int unit, int mtp_index)
{
    int rv = BCM_E_NONE;                     /* Operation return status.*/
    bcm_gport_t mirror_dest;                 /* Mirror destination id.  */

    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        return _bcm_tr2_mirror_mtp_unreserve(unit, mtp_index, FALSE, TRUE);
    }

    /* Input parameters check. */
    if (mtp_index >= MIRROR_CONFIG(unit)->ing_mtp_count) {
        return (BCM_E_PARAM);
    }

    /* If MTP is not in use - do nothing */
    if (0 == MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, mtp_index)) {
        return (rv);
    }


    /* Decrement mtp index reference count. */
    if (MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, mtp_index) > 0) {
        MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, mtp_index)--;
    }

    if (0 == MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, mtp_index)) {
        /* Write MTP registers */
        mirror_dest = MIRROR_CONFIG_ING_MTP_DEST(unit, mtp_index);
        rv = _bcm_xgs3_mtp_reset(unit, mtp_index, BCM_MIRROR_PORT_INGRESS);
        MIRROR_CONFIG_ING_MTP_DEST(unit, mtp_index) = BCM_GPORT_INVALID;
        if (MIRROR_DEST_REF_COUNT(unit, mirror_dest) > 0) {
            MIRROR_DEST_REF_COUNT(unit, mirror_dest)--;
        }
    }
    return (rv);
}

/*
 * Function:
 *      _bcm_xgs3_mirror_egress_mtp_unreserve
 * Description:
 *      Free egress  mirror-to port
 * Parameters:
 *      unit       - (IN) BCM device number
 *      mtp_index  - (IN) MTP index. 
 *      is_port    - (IN) Port based mirror indication.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Mtp index completely freed only when reference count gets to 0. 
 */
STATIC int
_bcm_xgs3_mirror_egress_mtp_unreserve(int unit, int mtp_index, int is_port)
{
    int rv = BCM_E_NONE;                     /* Operation return status.*/
    bcm_gport_t mirror_dest;                 /* Mirror destination id.  */

    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        return _bcm_tr2_mirror_mtp_unreserve(unit, mtp_index, TRUE, is_port);
    }

    /* Input parameters check. */
    if (mtp_index >= MIRROR_CONFIG(unit)->egr_mtp_count) {
        return (BCM_E_PARAM);
    }

    if (0 == MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, mtp_index)) {
        return (rv);
    }

    /* Decrement mtp index reference count. */
    if (MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, mtp_index) > 0) {
        if (is_port) {
            MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, mtp_index)--;
        } else {
            MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, mtp_index) -= (1 << BCM_MIRROR_MTP_REF_FP_OFFSET);
        }
    }


    if (0 == MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, mtp_index)) {
        /* Write MTP registers */
        mirror_dest = MIRROR_CONFIG_EGR_MTP_DEST(unit, mtp_index);
        rv = _bcm_xgs3_mtp_reset(unit, mtp_index, BCM_MIRROR_PORT_EGRESS);
        MIRROR_CONFIG_EGR_MTP_DEST(unit, mtp_index) = BCM_GPORT_INVALID;
        if (MIRROR_DEST_REF_COUNT(unit, mirror_dest) > 0) { 
            MIRROR_DEST_REF_COUNT(unit, mirror_dest)--;
        }
    }
    return (rv);
}

#ifdef BCM_TRIUMPH2_SUPPORT
/*
 * Function:
 *      _bcm_xgs3_mirror_egress_true_mtp_unreserve
 * Description:
 *      Free egress  mirror-to port
 * Parameters:
 *      unit       - (IN) BCM device number
 *      mtp_index  - (IN) MTP index. 
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Mtp index completely freed only when reference count gets to 0. 
 */
STATIC int
_bcm_xgs3_mirror_egress_true_mtp_unreserve(int unit, int mtp_index)
{
    int rv = BCM_E_NONE;                     /* Operation return status.*/
    bcm_gport_t mirror_dest;                 /* Mirror destination id.  */

    /* Input parameters check. */
    if (mtp_index >= MIRROR_CONFIG(unit)->egr_true_mtp_count) {
        return (BCM_E_PARAM);
    }

    if (0 == MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, mtp_index)) {
        return (rv);
    }

    /* Decrement mtp index reference count. */
    if (MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, mtp_index) > 0) {
        MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, mtp_index)--;
    }


    if (0 == MIRROR_CONFIG_EGR_TRUE_MTP_REF_COUNT(unit, mtp_index)) {
        /* Write MTP registers */
        mirror_dest = MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, mtp_index);
        rv = _bcm_xgs3_mtp_reset(unit, mtp_index, BCM_MIRROR_PORT_EGRESS_TRUE);
        MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, mtp_index) = BCM_GPORT_INVALID;
        if (MIRROR_DEST_REF_COUNT(unit, mirror_dest) > 0) { 
            MIRROR_DEST_REF_COUNT(unit, mirror_dest)--;
        }
    }
    return (rv);
}
#endif /* BCM_TRIUMPH2_SUPPORT */

/*
 * Function:
 *      _bcm_xgs3_mirror_mtp_reserve 
 * Description:
 *      Reserve a mirror-to port
 * Parameters:
 *      unit       - (IN)  BCM device number
 *      dest_port  - (IN)  Mirror destination gport.
 *      flags      - (IN)  Mirrored traffic direction. 
 *      index_used - (OUT) MTP index reserved
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the a mirror-to port is reserved more than once
 *      (without being unreserved) then the same MTP index 
 *      will be returned for each call.
 *      Direction should be INGRESS, EGRESS, or EGRESS_TRUE.
 */
STATIC int
_bcm_xgs3_mirror_mtp_reserve(int unit, bcm_gport_t gport, 
                            uint32 flags, int *index_used)
{
    /* Input parameters check. */
    if (NULL == index_used) {
        return (BCM_E_PARAM);
    }

    /* Allocate & initialize mtp based on mirroring direction. */
    if (flags & BCM_MIRROR_PORT_INGRESS) {
        return _bcm_xgs3_mirror_ingress_mtp_reserve(unit, gport, index_used);
    } else if (flags & BCM_MIRROR_PORT_EGRESS) {
        return _bcm_xgs3_mirror_egress_mtp_reserve(unit, gport, TRUE, index_used);
#ifdef BCM_TRIUMPH2_SUPPORT
    } else if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
        return _bcm_xgs3_mirror_egress_true_mtp_reserve(unit, gport,
                                                        index_used);
#endif /* BCM_TRIUMPH2_SUPPORT */
    } 
    return (BCM_E_PARAM);
}


/*
 * Function:
 *      _bcm_xgs3_mirror_mtp_unreserve 
 * Description:
 *      Free a mirror-to port
 * Parameters:
 *      unit       - (IN)  BCM device number
 *      mtp_index  - (IN)  MTP index. 
 *      is_port    - (IN)  Port based mirror indication.
 *      flags      - (IN)  Mirrored traffic direction. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_xgs3_mirror_mtp_unreserve(int unit, int mtp_index, int is_port, 
                               uint32 flags)
{

    /* Free & reset mtp based on mirroring direction. */
    if (flags & BCM_MIRROR_PORT_INGRESS) {
        return _bcm_xgs3_mirror_ingress_mtp_unreserve(unit, mtp_index);
    } else if (flags & BCM_MIRROR_PORT_EGRESS) {
        return _bcm_xgs3_mirror_egress_mtp_unreserve(unit, mtp_index, is_port);
#ifdef BCM_TRIUMPH2_SUPPORT
    } else if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
        return _bcm_xgs3_mirror_egress_true_mtp_unreserve(unit, mtp_index);
#endif /* BCM_TRIUMPH2_SUPPORT */
    } 
    return (BCM_E_PARAM);
}

#ifdef BCM_TRIUMPH2_SUPPORT

/*
 * Function:
 *     _bcm_xgs3_mtp_slot_port_indexes_get
 * Purpose:
 *      Retrieve port MTP indexes in MTP slots
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Local mirror port
 *      mtp_indexes  -  (OUT) MTP index in MTP slots for the given port
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_xgs3_mtp_slot_port_indexes_get(int unit, bcm_port_t port,
                                    uint32 mtp_indexes[BCM_MIRROR_MTP_COUNT])
{
    uint32 mc_val;
    mirror_control_entry_t mc_entry;
    int mtp_slot;

    /* Read mirror control structure to get programmed mtp indexes. */
    if (soc_feature(unit, soc_feature_mirror_control_mem)) {
        BCM_IF_ERROR_RETURN
            (READ_MIRROR_CONTROLm(unit, MEM_BLOCK_ANY, port, &mc_entry));
        BCM_MIRROR_MTP_ITER(MIRROR_CONFIG_MTP_DEV_MASK(unit), mtp_slot) {
            mtp_indexes[mtp_slot] =
                soc_mem_field32_get(unit, MIRROR_CONTROLm, &mc_entry,
                                    _mtp_index_field[mtp_slot]);
        }
    } else {
        BCM_IF_ERROR_RETURN(READ_MIRROR_CONTROLr(unit, port, &mc_val));
        BCM_MIRROR_MTP_ITER(MIRROR_CONFIG_MTP_DEV_MASK(unit), mtp_slot) {
            mtp_indexes[mtp_slot] =
                soc_reg_field_get(unit, MIRROR_CONTROLr, mc_val,
                                  _mtp_index_field[mtp_slot]);
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_xgs3_mtp_slot_port_index_set
 * Purpose:
 *      Set a port's MTP index in the given MTP slot
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Local mirror port
 *      mtp_slot     -  (IN) MTP slot in which to install the MTP index.
 *      mtp_index    -  (IN) HW mtp index.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_xgs3_mtp_slot_port_index_set(int unit, bcm_port_t port,
                                  int mtp_slot, int mtp_index)
{
    uint32 mc_val;
    int cpu_hg_index = 0;
    mirror_control_entry_t mc_entry;

    /* Non-UC fields are only needed for egress mirroring,
     * but configuring them unconditionally will
     * simplify logic without changing device behavior. */

    if (soc_feature(unit, soc_feature_mirror_control_mem)) {
        BCM_IF_ERROR_RETURN
            (READ_MIRROR_CONTROLm(unit, MEM_BLOCK_ANY, port, &mc_entry));
        soc_mem_field32_set(unit, MIRROR_CONTROLm, &mc_entry,
                            _mtp_index_field[mtp_slot], mtp_index);
        soc_mem_field32_set(unit, MIRROR_CONTROLm, &mc_entry,
                            _non_uc_mtp_index_field[mtp_slot], mtp_index);
        BCM_IF_ERROR_RETURN
            (WRITE_MIRROR_CONTROLm(unit, MEM_BLOCK_ANY, port, &mc_entry));

        cpu_hg_index = SOC_IS_KATANA2(unit) ?
                       SOC_INFO(unit).cpu_hg_pp_port_index :
                       SOC_INFO(unit).cpu_hg_index;
        if (IS_CPU_PORT(unit, port) && cpu_hg_index != -1) {
            BCM_IF_ERROR_RETURN(READ_MIRROR_CONTROLm(unit, MEM_BLOCK_ANY,
                                cpu_hg_index, &mc_entry));
            soc_mem_field32_set(unit, MIRROR_CONTROLm, &mc_entry,
                                _mtp_index_field[mtp_slot], mtp_index);
            soc_mem_field32_set(unit, MIRROR_CONTROLm, &mc_entry,
                        _non_uc_mtp_index_field[mtp_slot], mtp_index);
            BCM_IF_ERROR_RETURN(WRITE_MIRROR_CONTROLm(unit, MEM_BLOCK_ANY,
                                cpu_hg_index, &mc_entry));
        }
    } else {
        BCM_IF_ERROR_RETURN(READ_MIRROR_CONTROLr(unit, port, &mc_val));
        soc_reg_field_set(unit, MIRROR_CONTROLr, &mc_val,
                            _mtp_index_field[mtp_slot], mtp_index);
        soc_reg_field_set(unit, MIRROR_CONTROLr, &mc_val,
                         _non_uc_mtp_index_field[mtp_slot], mtp_index);
        BCM_IF_ERROR_RETURN(WRITE_MIRROR_CONTROLr(unit, port, mc_val));

        if (IS_CPU_PORT(unit, port)) {
            BCM_IF_ERROR_RETURN(READ_IMIRROR_CONTROLr(unit, port, &mc_val));
            soc_reg_field_set(unit, IMIRROR_CONTROLr, &mc_val,
                            _mtp_index_field[mtp_slot], mtp_index);
            soc_reg_field_set(unit, IMIRROR_CONTROLr, &mc_val,
                         _non_uc_mtp_index_field[mtp_slot], mtp_index);
            BCM_IF_ERROR_RETURN(WRITE_IMIRROR_CONTROLr(unit, port, mc_val));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_xgs3_mtp_index_port_slot_get
 * Purpose:
 *      Get a port's MTP slot for the given MTP index
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Local mirror port
 *      enables      -  (IN) Bitmap of port's enables
 *      egress       -  (IN) Ingress/Egress indication
 *      mtp_index    -  (IN) HW mtp index.
 *      mtp_slot_p    -  (OUT) MTP slot of the MTP index on the port.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_xgs3_mtp_index_port_slot_get(int unit, bcm_port_t port,
                                  uint32 enables, int egress,
                                  int mtp_index, int *mtp_slot_p)
{
    int mtp_slot, mtp_bit;          /* MTP type value & bitmap            */ 
    uint32 index_val[BCM_MIRROR_MTP_COUNT];

    /* Verify we are still coherent */
    if (egress) {
        if (enables != (enables & MIRROR_CONFIG_MTP_MODE_BMP(unit))) {
            /* Out of sync! */
            return BCM_E_INTERNAL;
        }
    } else {
        if (enables != (enables & ~MIRROR_CONFIG_MTP_MODE_BMP(unit))) {
            /* Out of sync! */
            return BCM_E_INTERNAL;
        }
    }

    /* The enables are of MTP slots, but we have the MTP index
     * Determine which slot has the index */

    /* Read mirror control structure to get programmed mtp slots. */
    BCM_IF_ERROR_RETURN
        (_bcm_xgs3_mtp_slot_port_indexes_get(unit, port, index_val));

    BCM_MIRROR_MTP_ITER(MIRROR_CONFIG_MTP_DEV_MASK(unit), mtp_slot) {
        mtp_bit = 1 << mtp_slot;
        /* Check if MTP slot is enabled on port */
        if (!(enables & mtp_bit)) {
            continue;
        }

        if (mtp_index == index_val[mtp_slot]) {
            *mtp_slot_p = mtp_slot;
            return BCM_E_NONE;            
        }
    }

    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *      _bcm_tr2_mirror_mtp_slot_update
 * Description:
 *      Write the MTP_TYPE data for the MTP slot configuration
 *      when it is changed. 
 * Parameters:
 *      unit       - (IN)  BCM device number.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_mtp_slot_update(int unit)
{
    uint32 ms_reg;             /* MTP mode register value     */

    BCM_IF_ERROR_RETURN(READ_MIRROR_SELECTr(unit, &ms_reg));
    soc_reg_field_set(unit, MIRROR_SELECTr, &ms_reg, MTP_TYPEf,
                      MIRROR_CONFIG_MTP_MODE_BMP(unit));
    BCM_IF_ERROR_RETURN(WRITE_MIRROR_SELECTr(unit, ms_reg));
    BCM_IF_ERROR_RETURN(WRITE_EGR_MIRROR_SELECTr(unit, ms_reg));

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_xgs3_mtp_type_slot_reserve
 * Purpose:
 *      Record used MTP slots forPort/ FP/IPFIX usage. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      flags        -  (IN) BCM_MIRROR_PORT_XXX flags.
 *      port_enables -  (IN) Bitmap of port's enables (Port only)
 *      port         -  (IN) Local mirror port (Port only)
 *      mtp_type     -  (IN) Port/FP/IPFIX.
 *      mtp_index    -  (IN) Allocated hw mtp index.
 *      mtp_slot_p   -  (OUT) MTP slot in which to install the MTP index.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_xgs3_mtp_type_slot_reserve(int unit, uint32 flags, uint32 port_enables,
                                bcm_port_t port, int mtp_type,
                                int mtp_index, int *mtp_slot_p)
{
    int mtp_slot, mtp_bit, free_ptr = -1, free_slot = -1;
    int egress = FALSE;
    int port_mirror;
    bcm_port_t port_ix;
    uint32 index_val[BCM_MIRROR_MTP_COUNT];
 
    if (flags & BCM_MIRROR_PORT_EGRESS) {
        egress = TRUE;
    }

    port_mirror = (mtp_type == BCM_MTP_SLOT_TYPE_PORT);
    if (port_mirror) {
        /* Read mirror control structure to get programmed mtp indexes. */
        BCM_IF_ERROR_RETURN
            (_bcm_xgs3_mtp_slot_port_indexes_get(unit, port, index_val));
    }

    BCM_MIRROR_MTP_ITER(MIRROR_CONFIG_MTP_DEV_MASK(unit), mtp_slot) {
        mtp_bit = 1 << mtp_slot;
        if (egress) {
            if (!(MIRROR_CONFIG_MTP_MODE_BMP(unit) & mtp_bit)) {
                if (MIRROR_CONFIG_MTP_MODE_REF_COUNT(unit,
                                                     mtp_slot) == 0) {
                    /* MTP Container is undecided, note for later */
                    if (free_ptr < 0) {
                        /* Record unallocated MTP container */
                        free_ptr = mtp_slot;
                    }
                } /* Else, container already used for ingress mirrors */
                continue;
            } else {
                /* Already an egress slot, is it the same MTP? */
                if (port_mirror && (mtp_index != index_val[mtp_slot])) {
                    /* No, keep searching */
                    continue;
                }
            }
        } else {
            if (MIRROR_CONFIG_MTP_MODE_BMP(unit) & mtp_bit) {
                /* Slot configured for egress mirroring, skip */
                continue;
            }
        }
        if (port_mirror) {
            if (!(port_enables & mtp_bit)) { /* Slot unused on this port */
                if (free_slot < 0) {
                    /* Record free slot */
                    free_slot = mtp_slot;
                }
            } else {
                /* Check if mtp is already installed. */
                if (mtp_index == index_val[mtp_slot]) {
                    /* Match - return mtp_slot */
                    *mtp_slot_p = mtp_slot;
                    return BCM_E_EXISTS;
                }
            }
        } else {
            if (MIRROR_CONFIG_TYPE_MTP_REF_COUNT(unit, mtp_slot, mtp_type)) {
                if (MIRROR_CONFIG_TYPE_MTP_SLOT(unit, mtp_slot,
                                                mtp_type) == mtp_index) {
                    /* Match - return mtp_slot */
                    *mtp_slot_p = mtp_slot;
                    MIRROR_CONFIG_TYPE_MTP_REF_COUNT(unit, mtp_slot,
                                                     mtp_type)++;
                    return BCM_E_NONE;
                }
            } else { /* Slot unused on this port */
                if (free_slot < 0) {
                    /* Record free slot */
                    free_slot = mtp_slot;
                }
            }
        }
    }

    /* Use previous allocated slot if available. Otherwise use unallocated
     * MTP continaner.  If neither, we're out of resources. */
    if (free_slot < 0) {
        if (free_ptr < 0) {
            return BCM_E_RESOURCE;
        } else {
            free_slot = free_ptr;
        }
    }

    mtp_slot = free_slot;
    mtp_bit = 1 << free_slot;

    /* Record references and new MTP mode allocation if necessary */
    if (egress && !(MIRROR_CONFIG_MTP_MODE_BMP(unit) & mtp_bit)) {
        /* Update MTP_MODE */
        MIRROR_CONFIG_MTP_MODE_BMP(unit) |= mtp_bit;

        BCM_IF_ERROR_RETURN(_bcm_tr2_mirror_mtp_slot_update(unit));
    }
    MIRROR_CONFIG_MTP_MODE_REF_COUNT(unit, mtp_slot)++;

    if (port_mirror) {
        if (egress) {
            /* Must set all ports for egress mirroring */
            PBMP_ALL_ITER(unit, port_ix) {
                BCM_IF_ERROR_RETURN
                    (_bcm_xgs3_mtp_slot_port_index_set(unit, port_ix,
                                                       mtp_slot, mtp_index));
            }
        } else {
            /* Only this port for ingress mirroring */
            BCM_IF_ERROR_RETURN
                (_bcm_xgs3_mtp_slot_port_index_set(unit, port,
                                                   mtp_slot, mtp_index));
        }
    } else {
        MIRROR_CONFIG_TYPE_MTP_SLOT(unit, mtp_slot, mtp_type) = mtp_index;
    }
    MIRROR_CONFIG_TYPE_MTP_REF_COUNT(unit, mtp_slot, mtp_type)++;

    /* This configures the mirror behavior, but the enables are set in 
     * the calling functions. */
    *mtp_slot_p = mtp_slot;

    return BCM_E_NONE;
}     

/*
 * Function:
 *     _bcm_xgs3_mtp_type_slot_unreserve
 * Purpose:
 *      Clear a used MTP slot for Port/FP/IPFIX usage. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      flags        -  (IN) BCM_MIRROR_PORT_XXX flags.
 *      port_enables -  (IN) Bitmap of port's enables (Port only)
 *      port         -  (IN) Local mirror port (Port only)
 *      mtp_type     -  (IN) Port/FP/IPFIX.
 *      mtp_index    -  (IN) Allocated hw mtp index.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_xgs3_mtp_type_slot_unreserve(int unit, uint32 flags,
                                  bcm_port_t port, int mtp_type,
                                  int mtp_index)
{
    int comb_enables;
    int mtp_slot, mtp_bit;
    int egress = FALSE;
    int port_mirror;
    uint32 index_val[BCM_MIRROR_MTP_COUNT];

    if (flags & BCM_MIRROR_PORT_EGRESS) {
        egress = TRUE;
    }

    port_mirror = (mtp_type == BCM_MTP_SLOT_TYPE_PORT);
    if (port_mirror) {
        /* Read mirror control structure to get programmed mtp indexes. */
        BCM_IF_ERROR_RETURN
            (_bcm_xgs3_mtp_slot_port_indexes_get(unit, port, index_val));
    }

    /* Make an effective enable bitmap for "in use" */
    comb_enables = 0;
    BCM_MIRROR_MTP_ITER(MIRROR_CONFIG_MTP_DEV_MASK(unit), mtp_slot) {
        if (MIRROR_CONFIG_TYPE_MTP_REF_COUNT(unit, mtp_slot, mtp_type)) {
            comb_enables |= (1 << mtp_slot);
        }
    }

    if (egress) {
        comb_enables &= MIRROR_CONFIG_MTP_MODE_BMP(unit);
        /* Only egress slots */
    } else {
        comb_enables &= ~MIRROR_CONFIG_MTP_MODE_BMP(unit);
        /* Only ingress slots */
    }

    BCM_MIRROR_MTP_ITER(MIRROR_CONFIG_MTP_DEV_MASK(unit), mtp_slot) {
        mtp_bit = 1 << mtp_slot;
        if (!(comb_enables & mtp_bit)) {
            continue;
        }

        if (port_mirror) {
            if (index_val[mtp_slot] == mtp_index) {
                /* Removed mtp was found -> disable it. */

                /* Calling function should have already disabled
                 * ipipe mirroring on port. */

                /* Reset ipipe mirroring mtp index. */
                BCM_IF_ERROR_RETURN
                    (_bcm_xgs3_mtp_slot_port_index_set(unit, port,
                                                       mtp_slot, 0));

                if (MIRROR_CONFIG_MTP_MODE_REF_COUNT(unit, mtp_slot) > 0) {
                    MIRROR_CONFIG_MTP_MODE_REF_COUNT(unit, mtp_slot)--;
                }
                if (egress) {
                    if (!(MIRROR_CONFIG_MTP_MODE_REF_COUNT(unit,
                                                           mtp_slot))) {
                        /* Free MTP_MODE */
                        MIRROR_CONFIG_MTP_MODE_BMP(unit) &= ~mtp_bit;

                        BCM_IF_ERROR_RETURN
                            (_bcm_tr2_mirror_mtp_slot_update(unit));
                    }
                }
                if (MIRROR_CONFIG_TYPE_MTP_REF_COUNT(unit, mtp_slot,
                                                     mtp_type) > 0) { 
                    MIRROR_CONFIG_TYPE_MTP_REF_COUNT(unit, mtp_slot,
                                                     mtp_type)--;
                }
                break;
            }
        } else {
            if (MIRROR_CONFIG_TYPE_MTP_SLOT(unit, mtp_slot,
                                            mtp_type) == mtp_index) {
                /* Found! */
                if (MIRROR_CONFIG_MTP_MODE_REF_COUNT(unit, mtp_slot) > 0) {
                    MIRROR_CONFIG_MTP_MODE_REF_COUNT(unit, mtp_slot)--;
                }
                if (egress &&
                    !(MIRROR_CONFIG_MTP_MODE_REF_COUNT(unit,
                                                       mtp_slot))) {
                    /* Free MTP_MODE */
                    MIRROR_CONFIG_MTP_MODE_BMP(unit) &= ~mtp_bit;

                    BCM_IF_ERROR_RETURN
                        (_bcm_tr2_mirror_mtp_slot_update(unit));
                }

                if (MIRROR_CONFIG_TYPE_MTP_REF_COUNT(unit, mtp_slot,
                                                     mtp_type) > 0) { 
                    MIRROR_CONFIG_TYPE_MTP_REF_COUNT(unit, mtp_slot,
                                                     mtp_type)--;
                }
                if (0 == MIRROR_CONFIG_TYPE_MTP_REF_COUNT(unit, mtp_slot,
                                                          mtp_type)) {
                    MIRROR_CONFIG_TYPE_MTP_SLOT(unit, mtp_slot,
                                                mtp_type) = 0;
                }
                break;
            }
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr2_mirror_ipipe_egress_mtp_install
 * Description:
 *      Install IPIPE egress reserved mtp index into 
 *      mirror control register. 
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      mtp_index  - (IN)  Mirror to port index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_ipipe_egress_mtp_install(int unit, bcm_port_t port,
                                         int mtp_index)
{
    int enable;            /* Used mtp bit map.           */
    int mtp_slot;          /* MTP type value              */ 
    
    BCM_IF_ERROR_RETURN(_bcm_esw_mirror_egress_get(unit, port, &enable));

    if (MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_xgs3_mtp_type_slot_reserve(unit,
                                             BCM_MIRROR_PORT_EGRESS,
                                             enable, port,
                                             BCM_MTP_SLOT_TYPE_PORT,
                                             mtp_index, &mtp_slot));
    } else {
        /* Otherwise the slot and index is 1-1 */
        mtp_slot = mtp_index;
    }

    /* if mtp is enabled on port - inform caller */
    if (enable & (1 << mtp_slot)) {
        return (BCM_E_EXISTS);
    }

    /* Update egress enables */
    enable |= (1 << mtp_slot);
    BCM_IF_ERROR_RETURN(_bcm_esw_mirror_egress_set(unit, port, enable));

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr2_mirror_egress_true_mtp_install
 * Description:
 *      Install egress true mirroring reserved mtp index into 
 *      mirror control register. 
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      mtp_index  - (IN)  Mirror to port index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_egress_true_mtp_install(int unit, bcm_port_t port,
                                        int mtp_index)
{
    int enable;                /* Used mtp bit map.           */

    /* Read mtp egress true mtp enable bitmap for source port. */
    BCM_IF_ERROR_RETURN
        (_bcm_port_mirror_egress_true_enable_get(unit, port, &enable));

    if (!(enable & (1 << mtp_index))) {
        enable |= (1 << mtp_index);
        BCM_IF_ERROR_RETURN
            (_bcm_port_mirror_egress_true_enable_set(unit, port, enable));
    } else {
        /* GNATS: Nothing to do?  Ref counts? */
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr2_mirror_port_ipipe_dest_get
 * Description:
 *      Get IPIPE ingress/egress mirroring destinations for the
 *      specific port.
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      array_sz   - (IN)  Sizeof dest_array parameter.
 *      dest_array - (OUT) Mirror to port array.
 *      egress     - (IN)  (TRUE/FALSE) Egress mirror.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_port_ipipe_dest_get(int unit, bcm_port_t port,
                                    int array_sz, bcm_gport_t *dest_array,
                                    int egress, int vp)
{
    int index;                      /* Destination iteration index.       */
    int mtp_index;                  /* MTP index                          */
    int mtp_slot, mtp_bit;          /* MTP type value & bitmap            */ 
    int comb_enables;               /* Combined port/MTP type enable bits */
    int flexible_mtp;               /* MTP type reconfig permitted        */
    uint32 index_val[BCM_MIRROR_MTP_COUNT];

    flexible_mtp = MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit);

    /* Fold together the port enables and the MTP direction */
    if (egress) {
        if (vp != VP_PORT_INVALID) {
            BCM_IF_ERROR_RETURN(_bcm_tr2_mirror_dvp_enable_get(unit, vp, 
                                  &comb_enables));
        } else {
            /* Read mtp egress mtp enable bitmap for source port. */
            BCM_IF_ERROR_RETURN
                (_bcm_esw_mirror_egress_get(unit, port, &comb_enables));
        }
        if (flexible_mtp) {
            comb_enables &= MIRROR_CONFIG_MTP_MODE_BMP(unit);
        }
    } else {
        if (vp != VP_PORT_INVALID) {
            BCM_IF_ERROR_RETURN(_bcm_tr2_mirror_svp_enable_get(unit, vp, 
                                  &comb_enables));
        } else {
            /* Read mtp ingress mtp enable bitmap for source port. */
            BCM_IF_ERROR_RETURN
                (_bcm_port_mirror_enable_get(unit, port, &comb_enables));
        }
        if (flexible_mtp) {
            comb_enables &= ~MIRROR_CONFIG_MTP_MODE_BMP(unit);
        }
    }

    if (!comb_enables) {
        return (BCM_E_NONE);
    }

    /* Read mirror control structure to get programmed mtp indexes. */
    BCM_IF_ERROR_RETURN
        (_bcm_xgs3_mtp_slot_port_indexes_get(unit, port, index_val));

    index = 0;
    /* Read mirror control register to compare programmed mtp indexes. */
    BCM_MIRROR_MTP_ITER(MIRROR_CONFIG_MTP_DEV_MASK(unit), mtp_slot) {
        mtp_bit = 1 << mtp_slot;
        /* Check if MTP slot is enabled on port */
        if (!(comb_enables & mtp_bit)) {
            continue;
        }

        /* MTP index is enabled and direction matches */
        mtp_index = index_val[mtp_slot];

        if (flexible_mtp) {
            if (egress) {
                dest_array[index] =
                    MIRROR_CONFIG_EGR_MTP_DEST(unit, mtp_index);
            } else {
                dest_array[index] =
                    MIRROR_CONFIG_ING_MTP_DEST(unit, mtp_index);
            }
        } else {
            /* Validate MTP index direction matches for shared mode */
            if (MIRROR_CONFIG_SHARED_MTP(unit, mtp_index).egress != egress) {
                continue;
            }

            dest_array[index] =
                MIRROR_CONFIG_SHARED_MTP_DEST(unit, mtp_index);
        }
        index++;
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr2_mirror_port_egress_true_dest_get
 * Description:
 *      Get IP ingress/egress mirroring destinations for the specific port.
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      array_sz   - (IN)  Sizeof dest_array parameter.
 *      dest_array - (OUT) Mirror to port array.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_port_egress_true_dest_get(int unit, bcm_port_t port,
                                          int array_sz,
                                          bcm_gport_t *dest_array)
{
    int enable;                /* Mirror enable bitmap.       */
    int index;                 /* Destination iteration index.*/
    int mtp_index;

    /* Input parameters check. */
    if ((NULL == dest_array) || (0 == array_sz)) {
        return (BCM_E_PARAM);
    }

    /* Reset destination array. */
    for (index = 0; index < array_sz; index ++) {
        dest_array[index] = BCM_GPORT_INVALID;
    }

    /* Read mtp egress true mtp enable bitmap for source port. */
    BCM_IF_ERROR_RETURN
        (_bcm_port_mirror_egress_true_enable_get(unit, port, &enable));

    if (!enable) {
        return (BCM_E_NONE);
    }

    index = 0;

    /* Egress true mirroring uses 1-1 MTP index to slot mapping */
    for (mtp_index = 0; mtp_index < BCM_MIRROR_MTP_COUNT;  mtp_index++) {
        if (enable & (1 << mtp_index)) {
            dest_array[index] =
                MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, mtp_index);
            index++;
        }
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr2_mirror_ipipe_egress_mtp_uninstall
 * Description:
 *      Reset IPIPE egress reserved mtp index from 
 *      mirror control register. 
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      mtp_index  - (IN)  Mirror to port index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_ipipe_egress_mtp_uninstall(int unit, bcm_port_t port,
                                           int mtp_index)
{
    int enable;            /* Used mtp bit map.           */
    int mtp_slot;          /* MTP type value              */ 

    BCM_IF_ERROR_RETURN(_bcm_esw_mirror_egress_get(unit, port, &enable));

    if (MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_xgs3_mtp_index_port_slot_get(unit, port, enable, TRUE,
                                               mtp_index, &mtp_slot));
    } else {
        /* Otherwise the slot and index is 1-1 */
        mtp_slot = mtp_index;
    }

    /* if mtp is not enabled on port - do nothing */
    if (!(enable & (1 << mtp_slot))) {
        return (BCM_E_NOT_FOUND);
    }

    /* Update egress enables */
    enable &= ~(1 << mtp_slot);
    BCM_IF_ERROR_RETURN(_bcm_esw_mirror_egress_set(unit, port, enable));

    if (MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_xgs3_mtp_type_slot_unreserve(unit,
                                               BCM_MIRROR_PORT_EGRESS,
                                               port,
                                               BCM_MTP_SLOT_TYPE_PORT,
                                               mtp_index));
            
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr2_mirror_egress_true_mtp_uninstall
 * Description:
 *      Uninstall egress true mirroring reserved mtp index from 
 *      mirror control register. 
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      mtp_index  - (IN)  Mirror to port index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mirror_egress_true_mtp_uninstall(int unit, bcm_port_t port,
                                          int mtp_index)
{
    int enable;                /* Used mtp bit map.           */
    int rv = BCM_E_NOT_FOUND;  /* Operation return status.    */

    /* Read mtp egress true mtp enable bitmap for source port. */
    BCM_IF_ERROR_RETURN
        (_bcm_port_mirror_egress_true_enable_get(unit, port, &enable));

    if ((enable & (1 << mtp_index))) {
        enable &= ~(1 << mtp_index);
        rv =_bcm_port_mirror_egress_true_enable_set(unit, port, enable);
    }

    return rv;
}
#endif /* BCM_TRIUMPH2_SUPPORT */

/*
 * Function:
 *      _bcm_xgs3_mirror_ingress_mtp_install
 * Description:
 *      Install ingress reserved mtp index into 
 *      mirror control register. 
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      mtp_index  - (IN)  Mirror to port index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_xgs3_mirror_ingress_mtp_install(int unit, bcm_port_t port,
                                     int mtp_index)
{
    uint32 reg_val;            /* MTP control register value. */
    int enable = 0;            /* Used mtp bit map.           */
    int rv = BCM_E_RESOURCE;   /* Operation return status.    */
    int hw_mtp;                /* Hw installed mtp index.     */

    /* Read mtp ingress mtp enable bitmap for source port. */
    BCM_IF_ERROR_RETURN(_bcm_port_mirror_enable_get(unit, port, &enable));

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        int mtp_slot;          /* MTP type value              */

        if (MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
           BCM_IF_ERROR_RETURN
                (_bcm_xgs3_mtp_type_slot_reserve(unit,
                                                 BCM_MIRROR_PORT_INGRESS,
                                                 enable, port,
                                                 BCM_MTP_SLOT_TYPE_PORT,
                                                 mtp_index, &mtp_slot));
        } else {
            /* Otherwise the slot and index is 1-1 */
            mtp_slot = mtp_index;
        }

        /* if mtp is enabled on port - inform caller */
        if (enable & (1 << mtp_slot)) {
            return (BCM_E_EXISTS);
        }

        /* Enable mirroring on the port. */
        enable |= (1 << mtp_slot);
        return _bcm_port_mirror_enable_set(unit, port, enable);
    }
#endif /* BCM_TRIUMPH2_SUPPORT */


    /* Read mirror control register to compare programmed mtp indexes. */
    BCM_IF_ERROR_RETURN(READ_MIRROR_CONTROLr(unit, port, &reg_val));

    if (!(enable & BCM_MIRROR_MTP_ONE)) {
        /* Mtp one is available */
        soc_reg_field_set(unit, MIRROR_CONTROLr, &reg_val,
                          IM_MTP_INDEXf, mtp_index);

        /* Set mtp index in Mirror control. */
        BCM_IF_ERROR_RETURN(WRITE_MIRROR_CONTROLr(unit, port, reg_val));

        /* Enable ingress mirroring on the port. */
        enable |= BCM_MIRROR_MTP_ONE;
        BCM_IF_ERROR_RETURN
            (_bcm_port_mirror_enable_set(unit, port, enable));
        if (IS_HG_PORT(unit, port)) {
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IMIRROR_CONTROLr, 
                                        port, IM_MTP_INDEXf,
                                        mtp_index));
        }
        rv = (BCM_E_NONE);
    } else {
        /* Mtp one is in use */
        /* Check if mtp is already installed. */
        hw_mtp = soc_reg_field_get(unit, MIRROR_CONTROLr, reg_val,
                                   IM_MTP_INDEXf);
        if (mtp_index == hw_mtp) {
            rv = (BCM_E_EXISTS);
        }
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_IS_TRX(unit) && BCM_FAILURE(rv) &&
        soc_reg_field_valid(unit, MIRROR_CONTROLr, IM_MTP_INDEX1f)) {
        if (!(enable & BCM_MIRROR_MTP_TWO)) {
            /* Mtp two is available */
            soc_reg_field_set(unit, MIRROR_CONTROLr, &reg_val,
                              IM_MTP_INDEX1f, mtp_index);

            /* Set mtp index in Mirror control. */
            BCM_IF_ERROR_RETURN(WRITE_MIRROR_CONTROLr(unit, port, reg_val));

            /* Enable ingress mirroring on the port. */
            enable |= BCM_MIRROR_MTP_TWO;
            BCM_IF_ERROR_RETURN
                (_bcm_port_mirror_enable_set(unit, port, enable));
            if (IS_HG_PORT(unit, port)) {
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, IMIRROR_CONTROLr, 
                                            port, IM_MTP_INDEX1f,
                                            mtp_index));
            }
            rv = (BCM_E_NONE);
        } else {
            /* Mtp two is in use */
            /* Check if mtp is already installed. */
            hw_mtp = soc_reg_field_get(unit, MIRROR_CONTROLr,reg_val,
                                       IM_MTP_INDEX1f);
            if (mtp_index == hw_mtp) {
                rv = (BCM_E_EXISTS);
            }
        }
    }
#endif /* BCM_TRX_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      _bcm_xgs3_mirror_egress_mtp_install
 * Description:
 *      Install egress reserved mtp index into 
 *      mirror control register. 
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      mtp_index  - (IN)  Mirror to port index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_xgs3_mirror_egress_mtp_install(int unit, bcm_port_t port, int mtp_index)
{
    uint32 reg_val;            /* MTP control register value. */
    int enable = 0;            /* Used mtp bit map.           */
    int port_enable = 0;       /* This port's enabled mtp bit map.*/
    int hw_mtp;                /* Hw installed mtp index.     */
    int rv = BCM_E_RESOURCE;   /* Operation return status.    */
    uint32 values[2];
    soc_field_t fields[2] = {EM_MTP_INDEXf, NON_UC_EM_MTP_INDEXf};
    bcm_port_t port_iterator;

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        return _bcm_tr2_mirror_ipipe_egress_mtp_install(unit, port,
                                                        mtp_index);
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    values[0] = values[1] = mtp_index;

    /* Read mtp egress mtp enable bitmap for source port. */
    BCM_IF_ERROR_RETURN
        (_bcm_esw_mirror_egress_get(unit, port, &port_enable));
    /* Read mtp egress mtp enable bitmap for any ports. */
    BCM_IF_ERROR_RETURN
        (_bcm_esw_mirror_egress_get(unit, BCM_GPORT_INVALID, &enable));

    /* Read mirror control register to compare programmed mtp indexes. */
    BCM_IF_ERROR_RETURN(READ_MIRROR_CONTROLr(unit, port, &reg_val));

    if (!(enable & BCM_MIRROR_MTP_ONE)) {
        /* Mtp one is available */

        /* Set all ingress ports to know the MTP index for the
         * egress port to be mirrored. */
        PBMP_ALL_ITER(unit, port_iterator) {
            BCM_IF_ERROR_RETURN 
                (soc_reg_fields32_modify(unit, MIRROR_CONTROLr,
                                         port_iterator, 2, fields, values));
        }
        /* Also enable the CPU's HG flow */
        BCM_IF_ERROR_RETURN 
            (soc_reg_fields32_modify(unit, IMIRROR_CONTROLr,
                                     CMIC_PORT(unit), 2, fields, values));

        /* Enable egress mirroring. */
        port_enable |= BCM_MIRROR_MTP_ONE;
        BCM_IF_ERROR_RETURN
            (_bcm_esw_mirror_egress_set(unit, port, port_enable));

        rv = (BCM_E_NONE);
    } else {
        /* Mtp one is in use */
        /* Check if mtp is already installed on this port. */
        hw_mtp = soc_reg_field_get (unit, MIRROR_CONTROLr, reg_val,
                                    EM_MTP_INDEXf);

        if (mtp_index == hw_mtp) {
            if (port_enable & BCM_MIRROR_MTP_ONE) {
                rv = (BCM_E_EXISTS);
            } else {
                /* MTP one already configured the correct MTP on other ports.
                 * This port must be enabled for it. */
                port_enable |= BCM_MIRROR_MTP_ONE;
                BCM_IF_ERROR_RETURN
                    (_bcm_esw_mirror_egress_set(unit, port, port_enable));
                rv = (BCM_E_NONE);
            }
        }
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_IS_TRX(unit) && BCM_FAILURE(rv) &&
        soc_reg_field_valid(unit, MIRROR_CONTROLr, EM_MTP_INDEX1f)) {
        if (!(enable & BCM_MIRROR_MTP_TWO)) {
            /* Mtp two is available */
 
            /* Set all ingress ports to know the MTP index for the
             * egress port to be mirrored. */
            fields[0] = EM_MTP_INDEX1f;
            fields[1] = NON_UC_EM_MTP_INDEX1f;
            PBMP_ALL_ITER(unit, port_iterator) {
                BCM_IF_ERROR_RETURN 
                    (soc_reg_fields32_modify(unit, MIRROR_CONTROLr,
                                             port_iterator,
                                             2, fields, values));
            }
            /* Also enable the CPU's HG flow */
            BCM_IF_ERROR_RETURN 
                (soc_reg_fields32_modify(unit, IMIRROR_CONTROLr,
                                         CMIC_PORT(unit),
                                         2, fields, values));

            /* Enable ingress mirroring on the port. */
            port_enable |= BCM_MIRROR_MTP_TWO;
            BCM_IF_ERROR_RETURN
                (_bcm_esw_mirror_egress_set(unit, port, port_enable));

            rv = (BCM_E_NONE);
        } else {
            /* Mtp one is in use */
            /* Check if mtp is already installed on this port. */
            hw_mtp = soc_reg_field_get(unit, MIRROR_CONTROLr, reg_val,
                                       EM_MTP_INDEX1f);
            if (mtp_index == hw_mtp) {
                if (port_enable & BCM_MIRROR_MTP_TWO) {
                    rv = (BCM_E_EXISTS);
                } else {
                    /* MTP one already configured the correct MTP
                     * on other ports.
                     * This port must be enabled for it. */
                    port_enable |= BCM_MIRROR_MTP_TWO;
                    BCM_IF_ERROR_RETURN
                        (_bcm_esw_mirror_egress_set(unit, port,
                                                    port_enable));
                    rv = (BCM_E_NONE);
                }
            }
        }
    }
#endif /* BCM_TRX_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      _bcm_xgs3_mirror_port_ingress_dest_get
 * Description:
 *      Get ingress mirroring destinations for the specific port 
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      array_sz   - (IN)  Sizeof dest_array parameter.
 *      dest_array - (OUT) Mirror to port array.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_xgs3_mirror_port_ingress_dest_get(int unit, bcm_port_t port,
                               int array_sz, bcm_gport_t *dest_array, int vp)
{
    uint32 mtp_value;          /* MTP index value.            */
    uint32 reg_val;            /* MTP control register value. */ 
    int enable;                /* Mirror enable bitmap.       */
    int index;                 /* Destination iteration index.*/

    /* Input parameters check. */
    if ((NULL == dest_array) || (0 == array_sz)) {
        return (BCM_E_PARAM);
    }

    /* Reset destination array. */
    for (index = 0; index < array_sz; index ++) {
        dest_array[index] = BCM_GPORT_INVALID;
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        return _bcm_tr2_mirror_port_ipipe_dest_get(unit, port, array_sz,
                                                   dest_array, FALSE, vp);
    }
#endif /* BCM_TRIUMPH2_SUPPORT */


    /* Read mtp ingress mtp enable bitmap for source port. */
    BCM_IF_ERROR_RETURN(_bcm_port_mirror_enable_get(unit, port, &enable));

    if (!enable) {
        return (BCM_E_NONE);
    }

    /* Read mirror control register to compare programmed mtp indexes. */
    BCM_IF_ERROR_RETURN(READ_MIRROR_CONTROLr(unit, port, &reg_val));

    index = 0;

    if (enable & BCM_MIRROR_MTP_ONE) {
        /* Mtp one is in use */
        mtp_value = soc_reg_field_get(unit, MIRROR_CONTROLr, 
                                      reg_val, IM_MTP_INDEXf);

        dest_array[index] = MIRROR_CONFIG_ING_MTP_DEST(unit, mtp_value);
        index++;
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_IS_TRX(unit) && (index < array_sz) ){
        if (enable & BCM_MIRROR_MTP_TWO) {
            /* Mtp two is in use */
            mtp_value = soc_reg_field_get(unit, MIRROR_CONTROLr, 
                                          reg_val, IM_MTP_INDEX1f);

            dest_array[index] = MIRROR_CONFIG_ING_MTP_DEST(unit, mtp_value);
            index++;
        }
    }
#endif /* BCM_TRX_SUPPORT */
    return (BCM_E_NONE);
}


/*
 * Function:
 *      _bcm_xgs3_mirror_port_egress_dest_get
 * Description:
 *      Get egress mirroring  destinations for the specific port 
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      array_sz   - (IN)  Sizeof dest_array parameter.
 *      dest_array - (OUT) Mirror to port array.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_xgs3_mirror_port_egress_dest_get(int unit, bcm_port_t port, int array_sz,
                                      bcm_gport_t *dest_array, int vp)
{
    uint32 mtp_value;          /* MTP index value.            */
    uint32 reg_val;            /* MTP control register value. */ 
    int enable;                /* Mirror enable bitmap.       */
    int index;                 /* Destination iteration index.*/

    /* Input parameters check. */
    if ((NULL == dest_array) || (0 == array_sz)) {
        return (BCM_E_PARAM);
    }

    /* Reset destination array. */
    for (index = 0; index < array_sz; index ++) {
        dest_array[index] = BCM_GPORT_INVALID;
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        return _bcm_tr2_mirror_port_ipipe_dest_get(unit, port, array_sz,
                                                   dest_array, TRUE, vp);
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    /* Read mtp ingress mtp enable enable bitmap for source port. */
    BCM_IF_ERROR_RETURN(_bcm_esw_mirror_egress_get(unit, port, &enable));

    if (!enable) {
        return (BCM_E_NONE);
    }

    /* Read mirror control register to compare programmed mtp indexes. */
    BCM_IF_ERROR_RETURN(READ_MIRROR_CONTROLr(unit, port, &reg_val));

    index = 0;

    if (enable & BCM_MIRROR_MTP_ONE) {
        /* Mtp one is in use */
        mtp_value = soc_reg_field_get(unit, MIRROR_CONTROLr, 
                                      reg_val, EM_MTP_INDEXf);

        dest_array[index] = MIRROR_CONFIG_EGR_MTP_DEST(unit, mtp_value);
        index++;
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_IS_TRX(unit) && (index < array_sz) ){
        if (enable & BCM_MIRROR_MTP_TWO) {
            /* Mtp two is in use */
            mtp_value = soc_reg_field_get(unit, MIRROR_CONTROLr, 
                                          reg_val, EM_MTP_INDEX1f);

            dest_array[index] = MIRROR_CONFIG_EGR_MTP_DEST(unit, mtp_value);
            index++;
        }
    }
#endif /* BCM_TRX_SUPPORT */
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_xgs3_mirror_ingress_mtp_uninstall
 * Description:
 *      Reset ingress reserved mtp index from 
 *      mirror control register. 
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      mtp_index  - (IN)  Mirror to port index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_xgs3_mirror_ingress_mtp_uninstall(int unit, bcm_port_t port, int mtp_index)
{
    int mtp_value;             /* MTP index value.            */
    uint32 reg_val;            /* MTP control register value. */ 
    int enable;                /* Mirror enable bitmap.       */
    int rv = BCM_E_NOT_FOUND;  /* Operation return status.    */

    /* Read mtp ingress mtp enable enable bitmap for source port. */
    BCM_IF_ERROR_RETURN(_bcm_port_mirror_enable_get(unit, port, &enable));

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        int mtp_slot;          /* MTP type value              */

        if (MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
            BCM_IF_ERROR_RETURN
                (_bcm_xgs3_mtp_index_port_slot_get(unit, port,
                                                   enable, FALSE,
                                                   mtp_index, &mtp_slot));
        } else {
            mtp_slot = mtp_index;
        }

        if (enable & (1 << mtp_slot)) {
            enable &= ~(1 << mtp_slot);
            BCM_IF_ERROR_RETURN
                (_bcm_port_mirror_enable_set(unit, port, enable));
            rv = BCM_E_NONE;
        }

        if (BCM_SUCCESS(rv) &&
            MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
            BCM_IF_ERROR_RETURN
                (_bcm_xgs3_mtp_type_slot_unreserve(unit,
                                                   BCM_MIRROR_PORT_INGRESS,
                                                   port,
                                                   BCM_MTP_SLOT_TYPE_PORT,
                                                   mtp_index));
            
        }
        return rv;
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    if (enable) {
        /* Read mirror control register to compare programmed mtp indexes. */
        BCM_IF_ERROR_RETURN(READ_MIRROR_CONTROLr(unit, port, &reg_val));
    }

    if (enable & BCM_MIRROR_MTP_ONE) {
        /* Mtp one is in use */
        mtp_value = soc_reg_field_get(unit, MIRROR_CONTROLr, 
                                      reg_val, IM_MTP_INDEXf);

        if (mtp_value == mtp_index) {
            /* Removed mtp was found -> disable it. */

            /* Disable ingress mirroring on port. */
            enable &= ~BCM_MIRROR_MTP_ONE;
            BCM_IF_ERROR_RETURN 
                (_bcm_port_mirror_enable_set(unit, port, enable));

            /* Reset ingress mirroring mtp index. */
            BCM_IF_ERROR_RETURN 
                (soc_reg_field32_modify(unit, MIRROR_CONTROLr,
                                        port, IM_MTP_INDEXf, 0));

            if (IS_HG_PORT(unit, port)) {
                BCM_IF_ERROR_RETURN 
                    (soc_reg_field32_modify(unit, IMIRROR_CONTROLr, 
                                            port, IM_MTP_INDEXf, 0));
            }
            rv = (BCM_E_NONE);
        }
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_IS_TRX(unit) && BCM_FAILURE(rv)){
        if (enable & BCM_MIRROR_MTP_TWO) {
            /* Mtp one is in use */
            mtp_value = soc_reg_field_get(unit, MIRROR_CONTROLr, 
                                          reg_val, IM_MTP_INDEX1f);

            if (mtp_value == mtp_index) {
                /* Removed mtp was found -> disable it. */

                /* Disable ingress mirroring on port. */
                enable &= ~BCM_MIRROR_MTP_TWO;
                BCM_IF_ERROR_RETURN 
                    (_bcm_port_mirror_enable_set(unit, port, enable));

                /* Reset ingress mirroring mtp index. */
                BCM_IF_ERROR_RETURN 
                    (soc_reg_field32_modify(unit, MIRROR_CONTROLr,
                                            port, IM_MTP_INDEX1f, 0));

                if (IS_HG_PORT(unit, port)) {
                    BCM_IF_ERROR_RETURN 
                        (soc_reg_field32_modify(unit, IMIRROR_CONTROLr, 
                                                port, IM_MTP_INDEX1f, 0));
                }
                rv = (BCM_E_NONE);
            }
        }
    }
#endif /* BCM_TRX_SUPPORT */
    return (rv);
}

/*
 * Function:
 *      _bcm_xgs3_mirror_egress_mtp_uninstall
 * Description:
 *      Reset egress reserved mtp index from 
 *      mirror control register. 
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      port       - (IN)  Mirror source gport.
 *      mtp_index  - (IN)  Mirror to port index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_xgs3_mirror_egress_mtp_uninstall(int unit, bcm_port_t port,
                                      int mtp_index)
{
    int mtp_value;             /* MTP index value.            */
    uint32 reg_val;            /* MTP control register value. */ 
    int enable;                /* Mirror enable bitmap.       */
    int port_enable;           /* This port's enabled mtp bit map.*/
    int rv = BCM_E_NOT_FOUND;   /* Operation return status.    */
    uint32 values[2] = {0, 0};
    soc_field_t fields[2] = {EM_MTP_INDEXf, NON_UC_EM_MTP_INDEXf};
    bcm_port_t port_iterator;

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        return _bcm_tr2_mirror_ipipe_egress_mtp_uninstall(unit, port,
                                                          mtp_index);
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    /* Read mtp egress mtp enable bitmap for source port. */
    BCM_IF_ERROR_RETURN
        (_bcm_esw_mirror_egress_get(unit, port, &port_enable));

    if (port_enable) {
        /* Read mirror control register to compare programmed mtp indexes. */
        BCM_IF_ERROR_RETURN(READ_MIRROR_CONTROLr(unit, port, &reg_val));
    }

    if (port_enable & BCM_MIRROR_MTP_ONE) {
        /* Mtp one is in use */
        mtp_value = soc_reg_field_get(unit, MIRROR_CONTROLr,
                                      reg_val, EM_MTP_INDEXf);

        if (mtp_value == mtp_index) {
            /* Removed mtp was found -> disable it. */

            /* Disable egress mirroring. */
            port_enable &= ~BCM_MIRROR_MTP_ONE;
            BCM_IF_ERROR_RETURN
                (_bcm_esw_mirror_egress_set(unit, port, port_enable));

            /* Read mtp egress mtp enable bitmap for all ports. */
            BCM_IF_ERROR_RETURN
                (_bcm_esw_mirror_egress_get(unit, BCM_GPORT_INVALID,
                                            &enable));

            if (0 == (enable & BCM_MIRROR_MTP_ONE)) {
                /* This egress MTP is no longer in use by any port.
                 * Reset egress mirroring mtp index. */
                PBMP_ALL_ITER(unit, port_iterator) {
                    BCM_IF_ERROR_RETURN 
                        (soc_reg_fields32_modify(unit, MIRROR_CONTROLr,
                                                 port_iterator,
                                                 2, fields, values));
                }
                /* Also enable the CPU's HG flow */
                BCM_IF_ERROR_RETURN 
                    (soc_reg_fields32_modify(unit, IMIRROR_CONTROLr,
                                             CMIC_PORT(unit),
                                             2, fields, values));
            }

            rv = (BCM_E_NONE);
        }
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_IS_TRX(unit) && BCM_FAILURE(rv)){
        if (port_enable & BCM_MIRROR_MTP_TWO) {
            /* Mtp two is in use */
            mtp_value = soc_reg_field_get(unit, MIRROR_CONTROLr,
                                          reg_val, EM_MTP_INDEX1f);

            if (mtp_value == mtp_index) {
                /* Removed mtp was found -> disable it. */

                /* Disable ingress mirroring. */
                port_enable &= ~BCM_MIRROR_MTP_TWO;
                BCM_IF_ERROR_RETURN
                    (_bcm_esw_mirror_egress_set(unit, port, port_enable));

                /* Read mtp egress mtp enable bitmap for all ports. */
                BCM_IF_ERROR_RETURN
                    (_bcm_esw_mirror_egress_get(unit, BCM_GPORT_INVALID,
                                                &enable));

                if (0 == (enable & BCM_MIRROR_MTP_TWO)) {
                    /* This egress MTP is no longer in use by any port.
                     * Reset egress mirroring mtp index. */
                    fields[0] = EM_MTP_INDEX1f;
                    fields[1] = NON_UC_EM_MTP_INDEX1f;
                    PBMP_ALL_ITER(unit, port_iterator) {
                        BCM_IF_ERROR_RETURN 
                            (soc_reg_fields32_modify(unit, MIRROR_CONTROLr,
                                                     port_iterator,
                                                     2, fields, values));
                    }
                    /* Also enable the CPU's HG flow */
                    BCM_IF_ERROR_RETURN 
                        (soc_reg_fields32_modify(unit, IMIRROR_CONTROLr,
                                                 CMIC_PORT(unit),
                                                 2, fields, values));
                }

                rv = (BCM_E_NONE);
            }
        }
    }
#endif /* BCM_TRX_SUPPORT */
    return (rv);
}


/*
 * Function:
 *	   _bcm_xgs3_mirror_enable_set 
 * Purpose:
 *  	Enable/disable mirroring on a port. 
 * Parameters:
 *	    unit - BCM device number
 *  	port - port number
 *   	enable - enable mirroring if non-zero
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int 
_bcm_xgs3_mirror_enable_set(int unit, int port, int enable)
{
    int cpu_hg_index = 0;

    /* Higig port should never drop directed mirror packets */
    if (IS_ST_PORT(unit, port) && !MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
        enable = 1;
    }

    if (soc_feature(unit, soc_feature_mirror_control_mem)) {
        BCM_IF_ERROR_RETURN
            (soc_mem_field32_modify(unit, MIRROR_CONTROLm, port, M_ENABLEf,
                                    enable));

        cpu_hg_index = SOC_IS_KATANA2(unit) ?
                       SOC_INFO(unit).cpu_hg_pp_port_index :
                       SOC_INFO(unit).cpu_hg_index;
        if (IS_CPU_PORT(unit, port) && cpu_hg_index != -1) {
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify(unit, MIRROR_CONTROLm,
                                cpu_hg_index, M_ENABLEf, enable));
        }
    } else {
        BCM_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, MIRROR_CONTROLr, port, M_ENABLEf,
                                    enable));
        /* Enable mirroring of CPU Higig packets as well */
        if (IS_CPU_PORT(unit, port)) {
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, IMIRROR_CONTROLr, port,
                                        M_ENABLEf, enable));
        }
    }
    return (BCM_E_NONE);
}
/*
 * Function:
 *      bcm_xgs3_mirror_egress_path_set
 * Description:
 *      Set egress mirror packet path for stack ring
 * Parameters:
 *      unit    - (IN) BCM device number
 *      modid   - (IN) Destination module ID (of mirror-to port)
 *      port    - (IN) Stack port for egress mirror packet
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      This function should only be used for XGS3 devices stacked
 *      in a ring configuration with fabric devices that may block
 *      egress mirror packets when the mirror-to port is on a 
 *      different device than the egress port being mirrored.
 *      Currently the only such fabric device is BCM5675 rev A0.
 */
int
bcm_xgs3_mirror_egress_path_set(int unit, bcm_module_t modid, bcm_port_t port)
{
    alternate_emirror_bitmap_entry_t egr_bmp;

    if (!soc_feature(unit, soc_feature_egr_mirror_path)) {
        return (BCM_E_UNAVAIL);
    }

    if ( !SOC_MODID_ADDRESSABLE(unit, modid)){
        return (BCM_E_BADID);
    }
    if (!IS_ST_PORT(unit, port)) {
        return (BCM_E_PORT);
    }

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, ALTERNATE_EMIRROR_BITMAPm,
                                     MEM_BLOCK_ANY, modid, &egr_bmp));
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) ) {
        soc_field_t bmapf, bmapf_zero;
        uint32 shift;

        if (port < 32) {
            bmapf = BITMAP_LOf;
            bmapf_zero = BITMAP_HIf;
            shift = port;
        } else {
            bmapf = BITMAP_HIf;
            bmapf_zero = BITMAP_LOf;
            shift = port - 32;
        }
        soc_ALTERNATE_EMIRROR_BITMAPm_field32_set(unit, &egr_bmp,
                                                  bmapf, 1 << shift);
        soc_ALTERNATE_EMIRROR_BITMAPm_field32_set(unit, &egr_bmp,
                                                  bmapf_zero, 0);
    } else
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        soc_field_t bmap_w[] = {BITMAP_W0f, BITMAP_W1f}; 
        /* index will identicate which field to program with correct value */
        int index = (port < 32) ? 0 : 1;
        uint32 shift = (port < 32) ? port : port - 32;
        int i;

        for (i = 0; i < COUNTOF(bmap_w); i++) {
            if (index == i) {
                soc_ALTERNATE_EMIRROR_BITMAPm_field32_set(unit, &egr_bmp,
                                                          bmap_w[i], 1 << shift);
            } else {
                soc_ALTERNATE_EMIRROR_BITMAPm_field32_set(unit, &egr_bmp,
                                                          bmap_w[i], 0);
            }
        }

    } else 
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
    if (SOC_IS_TD_TT(unit)) {
        soc_field_t bmap_w[] = {BITMAP_W0f, BITMAP_W1f, BITMAP_W2f}; 
        /* index will identicate which field to program with correct value */
        int index = ((port < 32) ? 0 : ((port < 64) ? 1 : 2));
        uint32 shift = ((port < 32) ? port : ((port < 64) ? (port - 32) : (port - 64) ));
        int i;

        for (i = 0; i < COUNTOF(bmap_w); i++) {
            if (index == i) {
                soc_ALTERNATE_EMIRROR_BITMAPm_field32_set(unit, &egr_bmp,
                                                          bmap_w[i], 1 << shift);
            } else {
                soc_ALTERNATE_EMIRROR_BITMAPm_field32_set(unit, &egr_bmp,
                                                          bmap_w[i], 0);
            }
        }

    } else 
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANAX(unit) ) {
        soc_field_t bmapf, bmapf_zero;
        uint32 shift;

        if (port < 32) {
            bmapf = BITMAP_W0f;
            bmapf_zero = BITMAP_W1f;
            shift = port;
        } else {
            bmapf = BITMAP_W1f;
            bmapf_zero = BITMAP_W0f;
            shift = port - 32;
        }
        soc_ALTERNATE_EMIRROR_BITMAPm_field32_set(unit, &egr_bmp,
                                                  bmapf, 1 << shift);
        soc_ALTERNATE_EMIRROR_BITMAPm_field32_set(unit, &egr_bmp,
                                                  bmapf_zero, 0);
    } else
#endif /* BCM_KATANA_SUPPORT */
    {
   
#if defined(BCM_FIREBOLT_SUPPORT)
        if (SOC_IS_FBX(unit)) {
            port -= SOC_HG_OFFSET(unit);
            soc_ALTERNATE_EMIRROR_BITMAPm_field32_set(unit, &egr_bmp,
                                                      BITMAPf, 1 << port);
        } 
#endif /* BCM_FIREBOLT_SUPPORT */
    } 
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, ALTERNATE_EMIRROR_BITMAPm,
                                      MEM_BLOCK_ALL, modid, &egr_bmp));
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_xgs3_mirror_egress_path_get
 * Description:
 *      Get egress mirror packet path for stack ring
 * Parameters:
 *      unit    - (IN) BCM device number
 *      modid   - (IN) Destination module ID (of mirror-to port)
 *      port    - (OUT)Stack port for egress mirror packet
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      See bcm_mirror_alt_egress_pbmp_set for more details.
 */
int
bcm_xgs3_mirror_egress_path_get(int unit, bcm_module_t modid, bcm_port_t *port)
{
    alternate_emirror_bitmap_entry_t egr_bmp;
    uint32 val, p, start = 0;

    if (NULL == port) {
        return (BCM_E_PARAM);
    }

    if (!soc_feature(unit, soc_feature_egr_mirror_path)) {
        return (BCM_E_UNAVAIL);
    }
    if (!SOC_MODID_ADDRESSABLE(unit, modid)) {
        return (BCM_E_BADID);
    }

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, ALTERNATE_EMIRROR_BITMAPm,
                                     MEM_BLOCK_ANY, modid, &egr_bmp));
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit)) {
        val = soc_ALTERNATE_EMIRROR_BITMAPm_field32_get(unit, &egr_bmp, BITMAP_LOf);
        if (val == 0) {
            val = soc_ALTERNATE_EMIRROR_BITMAPm_field32_get(unit, &egr_bmp, 
                                                            BITMAP_HIf);
            start = 32;
        }
    } else
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
    if (SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) {
        val = soc_ALTERNATE_EMIRROR_BITMAPm_field32_get(unit, &egr_bmp, 
                                                        BITMAP_W0f);
        if (val == 0) {
            val = soc_ALTERNATE_EMIRROR_BITMAPm_field32_get(unit, &egr_bmp, 
                                                            BITMAP_W1f);
            start = 32;
        }
        if (val == 0) {
            val = soc_ALTERNATE_EMIRROR_BITMAPm_field32_get(unit, &egr_bmp, 
                                                            BITMAP_W2f);
            start = 64;
        }
    } else 
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANAX(unit) ) {
        val = soc_ALTERNATE_EMIRROR_BITMAPm_field32_get(unit, &egr_bmp, BITMAP_W0f);
        if (val == 0) {
            val = soc_ALTERNATE_EMIRROR_BITMAPm_field32_get(unit, &egr_bmp, 
                                                            BITMAP_W1f);
            start = 32;
        }
    } else
#endif /* BCM_TRIUMPH2_SUPPORT */
    {
        val = soc_ALTERNATE_EMIRROR_BITMAPm_field32_get(unit, &egr_bmp, BITMAPf);
    }
    start --; 

    if (val == 0) {
        /* Return default egress port */
        return bcm_esw_topo_port_get(unit, modid, port);
    }
    for (p = start; val; p++)   {
        val >>= 1;
    }
    if (SOC_IS_FBX(unit) && !SOC_IS_TRIUMPH2(unit) && 
        !SOC_IS_APOLLO(unit) && !SOC_IS_VALKYRIE2(unit) 
        && !(SOC_IS_TD_TT(unit) || SOC_IS_TRIUMPH3(unit)) &&
        !(SOC_IS_KATANAX(unit))) {
        p += SOC_HG_OFFSET(unit);
    }
    *port = p;

    return (BCM_E_NONE);
}

/*
 * Function:
 *  	_bcm_xgs3_mirror_egr_dest_get 
 * Purpose:
 *  	Get destination port bitmap for egress mirroring.
 * Parameters:
 *      unit        - (IN) BCM device number.
 *	    port        - (IN) port number.
 *      mtp_index   - (IN) mtp index
 *      dest_bitmap - (OUT) destination port bitmap.
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_xgs3_mirror_egr_dest_get(int unit, bcm_port_t port, int mtp_index,
                              bcm_pbmp_t *dest_bitmap)
{
    uint32  fval, mirror;
    static const soc_reg_t reg[] = {
        EMIRROR_CONTROLr, EMIRROR_CONTROL1r
    };

    /* Input parameters check. */
    if (NULL == dest_bitmap) {
        return BCM_E_PARAM;
    }

    if ((mtp_index < 0) || 
        (mtp_index >= MIRROR_CONFIG(unit)->port_em_mtp_count)) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(soc_reg32_get(unit, reg[mtp_index], port, 0, &mirror));

    SOC_PBMP_CLEAR(*dest_bitmap);
    fval = soc_reg_field_get(unit, reg[mtp_index], mirror, BITMAPf);
    SOC_PBMP_WORD_SET(*dest_bitmap, 0, fval);

    return BCM_E_NONE;
}

/*
 * Function:
 *  	_bcm_xgs3_mirror_egr_dest_set 
 * Purpose:
 *  	Set destination port bitmap for egress mirroring.
 * Parameters:
 *      unit        - (IN) BCM device number.
 *      port        - (IN) Port number.
 *      mtp_index   - (IN) mtp index.
 *      dest_bitmap - (IN) Destination port bitmap.
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_xgs3_mirror_egr_dest_set(int unit, bcm_port_t port, int mtp_index,
                             bcm_pbmp_t *dest_bitmap)
{
    uint32      value;                  
    soc_field_t field = BITMAPf;

    static const soc_reg_t reg[] = {
        EMIRROR_CONTROLr, EMIRROR_CONTROL1r
    };
    static const soc_reg_t hg_reg[] = {
        IEMIRROR_CONTROLr, IEMIRROR_CONTROL1r
    };

    /* Input parameters checks. */
    if (NULL == dest_bitmap) {
        return BCM_E_PARAM;
    }
    if ((mtp_index < 0) || 
        (mtp_index >= MIRROR_CONFIG(unit)->port_em_mtp_count)) {
        return BCM_E_PARAM;
    }

    value = SOC_PBMP_WORD_GET(*dest_bitmap, 0);

    BCM_IF_ERROR_RETURN
        (soc_reg_fields32_modify(unit, reg[mtp_index],
                                 port, 1, &field, &value));

    /* Enable mirroring of CPU Higig packets as well */
    if (IS_CPU_PORT(unit, port)) {
        BCM_IF_ERROR_RETURN
            (soc_reg_fields32_modify(unit, hg_reg[mtp_index],
                                     port, 1, &field, &value));
    }

    return BCM_E_NONE;
}

#endif /* BCM_XGS3_SWITCH_SUPPORT */

/*
 * Function:
 *  	_bcm_esw_mirror_egr_dest_set
 * Purpose:
 *  	Set destination port bitmap for egress mirroring.
 * Parameters:
 *      unit        - (IN) BCM device number
 *      port        - (IN) Port number
 *      mtp_index   - (IN) mtp index.
 *      dest_bitmap - (IN) destination port bitmap
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_esw_mirror_egr_dest_set(int unit, bcm_port_t port, int mtp_index,
                             bcm_pbmp_t *dest_bitmap)
{
    int rv;

    /* Input parameters check */
    if (NULL == dest_bitmap) {
        return (BCM_E_PARAM);
    }
    if ((mtp_index < 0) || (mtp_index >= BCM_MIRROR_MTP_COUNT)) {
        return BCM_E_PARAM;
    }

#if defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit, soc_feature_mirror_control_mem)) {
        rv = _bcm_trident_mirror_egr_dest_set(unit, port, mtp_index, dest_bitmap);
    } else
#endif /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TR_VL(unit)) {
        rv = _bcm_triumph_mirror_egr_dest_set(unit, port, mtp_index, dest_bitmap);
    } else
#endif /* BCM_TRIUMPH_SUPPORT */

#if defined(BCM_RAPTOR_SUPPORT) 
    if (SOC_IS_RAPTOR(unit)) {
        rv = _bcm_raptor_mirror_egr_dest_set(unit, port, dest_bitmap);
    } else 
#endif /* BCM_RAPTOR_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        rv = _bcm_xgs3_mirror_egr_dest_set(unit, port, mtp_index, dest_bitmap);
    } else
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    {
        rv = BCM_E_UNAVAIL;
    }

    return (rv);
}


/*
 * Function:
 *  	_bcm_esw_mirror_egr_dest_get
 * Purpose:
 *      Get destination port bitmap for egress mirroring.
 * Parameters:
 *  	unit        - (IN) BCM device number.
 *  	port        - (IN) Port number.
 *      mtp_index   - (IN) mtp index.
 *      dest_bitmap - (OUT) destination port bitmap.
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_esw_mirror_egr_dest_get(int unit, bcm_port_t port, int mtp_index,
                             bcm_pbmp_t *dest_bitmap)
{
    int rv;

    /* Input parameters check */
    if (NULL == dest_bitmap) {
        return (BCM_E_PARAM);
    }

    SOC_PBMP_CLEAR(*dest_bitmap);

#if defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit, soc_feature_mirror_control_mem)) {
        rv = _bcm_trident_mirror_egr_dest_get(unit, port, mtp_index, dest_bitmap);
    } else
#endif /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TR_VL(unit)) {
        rv = _bcm_triumph_mirror_egr_dest_get(unit, port, mtp_index, dest_bitmap);
    } else
#endif /* BCM_TRIUMPH_SUPPORT */

#if defined(BCM_RAPTOR_SUPPORT) 
    if (SOC_IS_RAPTOR(unit)) {
        rv = _bcm_raptor_mirror_egr_dest_get(unit, port, dest_bitmap);
    } else 
#endif /* BCM_RAPTOR_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        rv = _bcm_xgs3_mirror_egr_dest_get(unit, port, mtp_index, dest_bitmap);
    } else
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    {
        rv = BCM_E_UNAVAIL;
    }

    return (rv);
}

/*
 * Function:
 *	    _bcm_esw_mirror_egress_get
 * Description:
 * 	    Get the mirroring per egress enabled/disabled status
 * Parameters:
 *  	unit -   (IN)  BCM device number
 *  	port -   (IN)  The port to check
 *  	enable - (OUT) Place to store boolean return value for on/off
 * Returns:
 *  	BCM_E_XXX
 */
STATIC int
_bcm_esw_mirror_egress_get(int unit, bcm_port_t port, int *enable)
{
    bcm_port_t port_iterator;
    bcm_pbmp_t dest_bitmap;
    int value = 0;
    int mtp_slot, mtp_bit;          /* MTP type value & bitmap            */ 

    /* mtp_slot == mtp_index unless directed flexible mirroring is used */

    /* Get destination port bitmap from first valid port. */
    PBMP_ALL_ITER(unit, port_iterator) {
        BCM_MIRROR_MTP_ITER(MIRROR_CONFIG_MTP_DEV_MASK(unit), mtp_slot) {
            mtp_bit = 1 << mtp_slot;

            /* Skip if not egress configured MTP */
            if (!SOC_WARM_BOOT(unit) && /* Else reloading info */
                soc_feature(unit, soc_feature_mirror_flexible)) {
                if (!MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
                    if (!MIRROR_CONFIG_SHARED_MTP(unit, mtp_slot).egress) {
                        continue;
                    }
                } else {
                    if (!(MIRROR_CONFIG_MTP_MODE_BMP(unit) & mtp_bit)) {
                        continue;
                    }
                }
            }

            BCM_IF_ERROR_RETURN
                (_bcm_esw_mirror_egr_dest_get(unit, port_iterator, mtp_slot,
                                              &dest_bitmap));
            if (BCM_GPORT_INVALID == port) {
                /* Get the full EM enable status */
                if (SOC_PBMP_NOT_NULL(dest_bitmap)) {
                    value |= mtp_bit;
                }
            } else {
                if (SOC_PBMP_MEMBER(dest_bitmap, port)) {
                    value |= mtp_bit;
                }
            }
        }
        /* Only care about finding one valid port worth of info */
        break;
    }

    *enable = value;
    return BCM_E_NONE;
}


/*
 * Function:
 * 	   _bcm_esw_mirror_egress_set
 * Description:
 *  	Enable or disable mirroring per egress
 * Parameters:
 *  	unit   - (IN) BCM device number
 *	port   - (IN) The port to affect
 *	enable - (IN) Boolean value for on/off
 * Returns:
 *	    BCM_E_XXX
 * Notes:
 *  	Mirroring must also be globally enabled.
 */
STATIC int
_bcm_esw_mirror_egress_set(int unit, bcm_port_t port, int enable)
{
    bcm_port_t port_iterator;
    bcm_pbmp_t dest_bitmap;
    int mtp_slot, mtp_bit;          /* MTP type value & bitmap            */ 

    /* mtp_slot == mtp_index unless directed flexible mirroring is used */

    PBMP_ALL_ITER(unit, port_iterator) {
        BCM_MIRROR_MTP_ITER(MIRROR_CONFIG_MTP_DEV_MASK(unit), mtp_slot) {
            mtp_bit = 1 << mtp_slot;

            /* Skip if not egress configured MTP */
            if (!SOC_WARM_BOOT(unit) && /* Else reloading info */
                soc_feature(unit, soc_feature_mirror_flexible)) {
                if (!MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
                    if (!MIRROR_CONFIG_SHARED_MTP(unit, mtp_slot).egress) {
                        continue;
                    }
                } else {
                    if (!(MIRROR_CONFIG_MTP_MODE_BMP(unit) & mtp_bit)) {
                        continue;
                    }
                }
            }

            BCM_IF_ERROR_RETURN
                (_bcm_esw_mirror_egr_dest_get(unit, port_iterator, mtp_slot,
                                              &dest_bitmap));

            /* Update egress destination bitmap. */
            if (enable & mtp_bit) {
                SOC_PBMP_PORT_ADD(dest_bitmap, port);
            } else {
                SOC_PBMP_PORT_REMOVE(dest_bitmap, port);
            }

            /* Write egress destination bitmap from each local port. */
            BCM_IF_ERROR_RETURN
                (_bcm_esw_mirror_egr_dest_set(unit, port_iterator, mtp_slot,
                                              &dest_bitmap));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *	  _bcm_esw_directed_mirroring_get
 * Purpose:
 *	  Check if  directed mirroring is enabled on the chip.
 * Parameters:
 *    unit    - (IN) BCM device number.
 *    enable  - (OUT)Directed mirror enabled.
 * Returns:
 *	  BCM_E_XXX
 */
STATIC int
_bcm_esw_directed_mirroring_get(int unit, int *enable)
{
    int  rv = BCM_E_NONE;      /* Operation return status. */

    /* Input parameters check. */
    if (NULL == enable) {
        return (BCM_E_PARAM);
    }

    /* Read switch control to check if directed mirroring is enabled.*/
    rv = bcm_esw_switch_control_get(unit, bcmSwitchDirectedMirroring, enable);
    if (BCM_E_UNAVAIL == rv) {
        *enable = FALSE;
        rv = BCM_E_NONE;
    }
    return (rv);
}


/*
 * Function:
 *      _bcm_esw_mirror_mtp_reserve 
 * Description:
 *      Reserve a mirror-to port
 * Parameters:
 *      unit       - (IN)  BCM device number
 *      dest_id    - (IN)  Mirror destination id.
 *      flags      - (IN)  Mirrored traffic direction. 
 *      index_used - (OUT) MTP index reserved
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the a mirror-to port is reserved more than once
 *      (without being unreserved) then the same MTP index 
 *      will be returned for each call.
 *      Direction should be INGRESS, EGRESS, or EGRESS_TRUE.
 */
STATIC int
_bcm_esw_mirror_mtp_reserve(int unit, bcm_gport_t dest_id, 
                            uint32 flags, int *index_used)
{
    int rv = BCM_E_RESOURCE;            /* Operation return status. */

    /* Input parameters check. */
    if (NULL == index_used) {
        return (BCM_E_PARAM);
    }

    /* Allocate MTP index for mirror destination. */
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit)) {
        rv = _bcm_xgs3_mirror_mtp_reserve(unit, dest_id, flags, index_used);
    } else
#endif
    {
        *index_used = 0;
        /*  If mirroring is already in use -> 
            make sure destination is identical, increment reference count.*/
        if (MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0)) {
            /* Mirror destination match. check. */
            if (MIRROR_CONFIG_ING_MTP_DEST(unit, 0) == dest_id) {
                MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0)++;
                rv = BCM_E_NONE;
            }
        } else { /* Mirroring not in use. */
            MIRROR_CONFIG_ING_MTP_DEST(unit, 0) = dest_id;
            MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0)++;
            MIRROR_DEST_REF_COUNT(unit, dest_id)++;
            rv = BCM_E_NONE;
        }

        /* Ingress & Egress mtp are identical for xgs devices. */
        if (BCM_SUCCESS(rv)) {
            MIRROR_CONFIG_EGR_MTP(unit, 0) = MIRROR_CONFIG_ING_MTP(unit, 0);
        }
    }
    return (rv);
}


/*
 * Function:
 *      _bcm_esw_mirror_mtp_unreserve 
 * Description:
 *      Free  mirror-to port
 * Parameters:
 *      unit       - (IN)  BCM device number.
 *      mtp_index  - (IN)  MTP index. 
 *      is_port    - (IN)  Port based mirror indication.
 *      flags      - (IN)  Mirrored traffic direction. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_esw_mirror_mtp_unreserve(int unit, int mtp_index, int is_port, 
                              uint32 flags)
{
    bcm_gport_t  mirror_dest;
    int          rv = BCM_E_NONE;

    /* Free MTP index for mirror destination. */
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_xgs3_mirror_mtp_unreserve(unit, mtp_index, is_port, flags));
    } else
#endif
    {
        /* Decrement reference counter & reset dest port    */
        /* if destination is no longer in use.              */
        if (MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0) > 0) {
            MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0)--;
            if (0 == MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0)) {
                mirror_dest = MIRROR_CONFIG_ING_MTP_DEST(unit, 0);
                MIRROR_CONFIG_ING_MTP_DEST(unit, 0)= BCM_GPORT_INVALID;
                if (MIRROR_DEST_REF_COUNT(unit, mirror_dest) > 0) {
                    MIRROR_DEST_REF_COUNT(unit, mirror_dest)--;
                }
            }
            MIRROR_CONFIG_EGR_MTP(unit, 0) = MIRROR_CONFIG_ING_MTP(unit, 0);
        }
    }
    return rv;
}
/*
 * Function:
 *	  _bcm_esw_mirror_deinit
 * Purpose:
 *	  Internal routine used to free mirror software module.
 *        control structures. 
 * Parameters:
 *        unit     - (IN) BCM device number.
 *        cfg_ptr  - (IN) Pointer to config structure.
 * Returns:
 *	  BCM_E_XXX
 */
STATIC int
_bcm_esw_mirror_deinit(int unit, _bcm_mirror_config_p *cfg_ptr)
{
    _bcm_mirror_config_p ptr;
    int mtp_type;

    /* Sanity checks. */
    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

#if defined(BCM_TRIDENT_SUPPORT)
    if (NULL != EGR_MIRROR_ENCAP(unit)) {
        BCM_IF_ERROR_RETURN
            (soc_profile_mem_destroy(unit, EGR_MIRROR_ENCAP(unit)));
        sal_free(EGR_MIRROR_ENCAP(unit));
        EGR_MIRROR_ENCAP(unit) = NULL;
    }
#endif /* BCM_TRIDENT_SUPPORT */

    /* If mirror config was not allocated we are done. */
    if (NULL == cfg_ptr) {
        return (BCM_E_PARAM);
    }

    ptr = *cfg_ptr;
    if (NULL == ptr) {
        return (BCM_E_NONE);
    }

    /* Free mirror destination information. */
    if (NULL != ptr->dest_arr) {
        sal_free(ptr->dest_arr);
        ptr->dest_arr = NULL;
    }

    /* Free egress true mtp information. */
    if (NULL != ptr->egr_true_mtp) {
        sal_free(ptr->egr_true_mtp);
        ptr->egr_true_mtp = NULL;
    }

    /* Free MTP types records. */
    for (mtp_type = BCM_MTP_SLOT_TYPE_PORT;
         mtp_type < BCM_MTP_SLOT_TYPES; mtp_type++) {
        if (NULL != ptr->mtp_slot[mtp_type]) {
            sal_free(ptr->mtp_slot[mtp_type]);
        }
    }

    /* Free egress mtp information. */
    if (NULL != ptr->egr_mtp) {
        sal_free(ptr->egr_mtp);
        ptr->egr_mtp = NULL;
    }

    /* Free ingress mtp information. */
    if (NULL != ptr->ing_mtp) {
        sal_free(ptr->ing_mtp);
        ptr->ing_mtp = NULL;
    }

    /* Free shared mtp information. */
    if (NULL != ptr->shared_mtp) {
        sal_free(ptr->shared_mtp);
        ptr->shared_mtp = NULL;
    }

    /* Destroy protection mutex. */
    if (NULL != ptr->mutex) {
        sal_mutex_destroy(ptr->mutex);
        ptr->mutex = NULL;
    }

    /* Free module configuration structue. */
    sal_free(ptr);
    *cfg_ptr = NULL;
    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_esw_mirror_enable_set 
 * Purpose:
 *	Enable/disable mirroring on a port
 * Parameters:
 *	unit - BCM device number
 *	port - port number
 *	enable - enable mirroring if non-zero
 * Returns:
 *	BCM_E_XXX
 * Notes:
 *      For non-XGS3 devices this function will also set the
 *      mirror-to port.
 */
STATIC int
_bcm_esw_mirror_enable_set(int unit, int port, int enable)
{
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit)) {
        return _bcm_xgs3_mirror_enable_set(unit, port, enable);
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#if defined(BCM_XGS12_FABRIC_SUPPORT)
    if (SOC_IS_XGS12_FABRIC(unit)) {
        return _bcm_xgs_fabric_mirror_enable_set(unit, port, enable);
    }
#endif /* BCM_XGS12_FABRIC_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *	  _bcm_esw_mirror_mode_set
 * Description:
 *	  Enable or disable mirroring.  Will wait for bcm_esw_mirror_to_set
 *        to be called to actually do the enable if needed.
 * Parameters:
 *        unit            - (IN)     BCM device number
 *	  mode            - (IN)     One of BCM_MIRROR_(DISABLE|L2|L2_L3)
 * Returns:
 *	  BCM_E_XXX
 */
STATIC int
_bcm_esw_mirror_mode_set(int unit, int mode)
{
    int    menable;            /* Enable mirroring flag.      */
    int	      port;            /* Port iterator.              */
    int      omode;            /* Original mirroring mode.    */
#if defined (BCM_XGS3_SWITCH_SUPPORT)
    int      enable;            /* By direction mirror enable. */
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    int  rv = BCM_E_UNAVAIL;   /* Operation return status.    */

    /* Preserve original module configuration. */
    omode = MIRROR_CONFIG_MODE(unit);  

    /* Update module mode. */
    MIRROR_CONFIG_MODE(unit) = mode;
    menable = (BCM_MIRROR_DISABLE != mode) ? TRUE : FALSE;

    if (!menable) {
        /* If mirroring was originally off - we are done. */
        if (!SOC_IS_XGS12_FABRIC(unit) && (omode == BCM_MIRROR_DISABLE)) {
            return (BCM_E_NONE);
        }
    }

    /* Wait for mirror_to_set() */
    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        if ((BCM_GPORT_INVALID == MIRROR_CONFIG_SHARED_MTP_DEST(unit, 0)) &&
            !SOC_IS_XGS12_FABRIC(unit)) {
            return (BCM_E_NONE);
        }
    } else {
        if ((BCM_GPORT_INVALID == MIRROR_CONFIG_ING_MTP_DEST(unit, 0)) &&
            !SOC_IS_XGS12_FABRIC(unit)) {
            return (BCM_E_NONE);
        }
    }

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit)) {
        PBMP_ALL_ITER(unit, port) {
            if (!IS_PORT(unit, port) && !IS_CPU_PORT(unit, port)) {
                /* Skip special ports (loopback port, etc.) */
                continue;
            }

            rv = bcm_esw_mirror_ingress_get(unit, port, &enable);
            if (BCM_FAILURE(rv)) {
                break;
            }
            if (enable) {
                rv = _bcm_xgs3_mirror_ingress_mtp_install(unit, port, 0);
                if (BCM_E_EXISTS == rv) {
                    /* Configured from a previous mode. */
                    rv = BCM_E_NONE;
                } else if (BCM_FAILURE(rv)) {
                    break;
                }
            }

            rv = bcm_esw_mirror_egress_get(unit, port, &enable);
            if (BCM_FAILURE(rv)) {
                break;
            }

            if (enable) {
                rv = _bcm_xgs3_mirror_egress_mtp_install(unit, port, 0);
                if (BCM_E_EXISTS == rv) {
                    /* Configured from a previous mode. */
                    rv = BCM_E_NONE;
                } else if (BCM_FAILURE(rv)) {
                    break;
                }
            }

            rv = _bcm_esw_mirror_enable_set(unit, port, menable);
            if (BCM_FAILURE(rv)) {
                break;
            }
        }
    } else 
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#if defined(BCM_XGS_SWITCH_SUPPORT)
    if (SOC_IS_XGS_SWITCH(unit)) {
        PBMP_ALL_ITER(unit, port) {
            if (!IS_PORT(unit, port) && !IS_CPU_PORT(unit, port)) {
                /* Skip special ports (loopback port, etc.) */
                continue;
            }
            rv = _bcm_esw_mirror_enable_set(unit, port, menable);
            if (BCM_FAILURE(rv)) {
                break;
            }
        }
    } else 
#endif /* BCM_XGS_SWITCH_SUPPORT */

#if defined(BCM_XGS_FABRIC_SUPPORT)
    if (SOC_IS_XGS_FABRIC(unit)) {
        PBMP_ST_ITER(unit, port) {
            rv = _bcm_esw_mirror_enable_set(unit, port, menable);
            if (BCM_FAILURE(rv)) {
                break;
            }
        }
    } else
#endif /* BCM_XGS_FABRIC_SUPPORT */
    {
        rv = BCM_E_UNAVAIL;
    }
    return (rv);
}

/*
 * Function:
 *	  _bcm_esw_mirror_hw_clear
 * Purpose:
 *	  Clear hw registers/tables & disable mirroring on the device.
 * Parameters:
 *    unit - (IN) BCM device number.
 * Returns:
 *	  BCM_E_XXX
 */
STATIC int
_bcm_esw_mirror_hw_clear(int unit)
{
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    int port;   /* Port iteration index. */
    bcm_pbmp_t pbmp;
    int mtp_index;  /* MTP itteration index */
#if defined(BCM_TRIUMPH2_SUPPORT)
    mirror_control_entry_t mc_entry; /* MTP control memory value (Trident) */
    uint32 mc_reg;                 /* MTP control register value.  */
    int mtp_type_undir = FALSE;    /* MTP_TYPE set for undirected mode */
    int cpu_hg_index = 0;
#endif /* BCM_TRIUMPH2_SUPPORT */

    if (SOC_IS_XGS3_SWITCH(unit)) {
        /* Stacking ports should never drop directed mirror packets */
        /* Other ports should default to no mirroring */

#if defined(BCM_TRIUMPH2_SUPPORT)
        /* Initialize default mirror control settings */
        if (soc_feature(unit, soc_feature_mirror_flexible) ||
            (soc_feature(unit, soc_feature_egr_mirror_true))) {
            if (soc_feature(unit, soc_feature_mirror_control_mem)) {
                sal_memset(&mc_entry, 0, sizeof(mc_entry));
                for (mtp_index = 0; mtp_index < BCM_MIRROR_MTP_COUNT;
                     mtp_index++) {
                    soc_mem_field32_set(unit, MIRROR_CONTROLm, &mc_entry,
                                        _mtp_index_field[mtp_index],
                                        mtp_index);
                    soc_mem_field32_set(unit, MIRROR_CONTROLm, &mc_entry,
                                        _non_uc_mtp_index_field[mtp_index],
                                        mtp_index);
                }
            } else {
                mc_reg = 0;
                for (mtp_index = 0; mtp_index < BCM_MIRROR_MTP_COUNT;
                     mtp_index++) {
                    soc_reg_field_set(unit, MIRROR_CONTROLr, &mc_reg,
                                      _mtp_index_field[mtp_index],
                                          mtp_index);
                    soc_reg_field_set(unit, MIRROR_CONTROLr, &mc_reg,
                                      _non_uc_mtp_index_field[mtp_index],
                                      mtp_index);
                }
            }
        }

        if (soc_feature(unit, soc_feature_mirror_flexible)) {
            if (MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
                /* Draco 1.5 mirroring mode. */
                mtp_type_undir = TRUE;
                MIRROR_CONFIG_SHARED_MTP(unit,
                      BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX).egress = TRUE;
            }
        }

#endif /* BCM_TRIUMPH2_SUPPORT */

        PBMP_ALL_ITER(unit, port) {
#if defined(BCM_TRIUMPH2_SUPPORT)
            /* Set up standard settings for flexible mirroring. */
            if (soc_feature(unit, soc_feature_mirror_flexible) ||
                (soc_feature(unit, soc_feature_egr_mirror_true) &&
                 IS_LB_PORT(unit, port))) {

                /* Set MTP mapping to 1-1 for flexible mirroring,
                   or egress true mirroring LB port */
                if (soc_feature(unit, soc_feature_mirror_control_mem)) {
                    BCM_IF_ERROR_RETURN
                        (WRITE_MIRROR_CONTROLm(unit, MEM_BLOCK_ANY,
                                               port, &mc_entry));
                    cpu_hg_index = SOC_IS_KATANA2(unit) ?
                                   SOC_INFO(unit).cpu_hg_pp_port_index :
                                   SOC_INFO(unit).cpu_hg_index;
                    if (IS_CPU_PORT(unit, port) && cpu_hg_index != -1) {
                        BCM_IF_ERROR_RETURN
                            (WRITE_MIRROR_CONTROLm(unit, MEM_BLOCK_ANY,
                                 cpu_hg_index, &mc_entry));
                    }
                } else {
                    BCM_IF_ERROR_RETURN
                        (WRITE_MIRROR_CONTROLr(unit, port, mc_reg));
                    if (IS_CPU_PORT(unit, port)) {
                        BCM_IF_ERROR_RETURN
                            (WRITE_IMIRROR_CONTROLr(unit, port, mc_reg));
                    }
                }
            }
#endif /* BCM_TRIUMPH2_SUPPORT */

            /* Reset global mirror enable bit after the mirror control
             * register is handled. */
            BCM_IF_ERROR_RETURN
                (_bcm_esw_mirror_enable_set(unit, port, 
                                            IS_ST_PORT(unit, port) ? 1 : 0));

            /* Disable ingress mirroring. */
            BCM_IF_ERROR_RETURN(_bcm_port_mirror_enable_set(unit, port, 0));

            /* Disable egress mirroring for all MTP indexes */ 
            SOC_PBMP_CLEAR(pbmp);

            for (mtp_index = 0; mtp_index < BCM_MIRROR_MTP_COUNT; mtp_index++) {
                (void)(_bcm_esw_mirror_egr_dest_set(unit, port, mtp_index, 
                                                    &pbmp));
            }

#ifdef BCM_FIREBOLT_SUPPORT
            /* Clear RSPAN settings */
            if (SOC_REG_IS_VALID(unit, EGR_RSPAN_VLAN_TAGr)) {
                BCM_IF_ERROR_RETURN(WRITE_EGR_RSPAN_VLAN_TAGr(unit, port, 0));
            }
#endif /* BCM_FIREBOLT_SUPPORT */
        }

        if (SOC_MEM_IS_VALID(unit, IM_MTP_INDEXm)) {
            BCM_IF_ERROR_RETURN
                (soc_mem_clear(unit, IM_MTP_INDEXm, COPYNO_ALL, 0));
        }
        if (SOC_MEM_IS_VALID(unit, EM_MTP_INDEXm)) {
            BCM_IF_ERROR_RETURN
                (soc_mem_clear(unit, EM_MTP_INDEXm, COPYNO_ALL, 0));
        }
#if defined(BCM_TRIUMPH2_SUPPORT)
        if (SOC_MEM_IS_VALID(unit, EP_REDIRECT_EM_MTP_INDEXm)) {
            BCM_IF_ERROR_RETURN
                (soc_mem_clear(unit, EP_REDIRECT_EM_MTP_INDEXm,
                               COPYNO_ALL, 0));
        }
        if (SOC_MEM_IS_VALID(unit, EGR_EP_REDIRECT_EM_MTP_INDEXm)) {
            BCM_IF_ERROR_RETURN
                (soc_mem_clear(unit, EGR_EP_REDIRECT_EM_MTP_INDEXm,
                               COPYNO_ALL, 0));
        }
        /* Clear settings of mirror_to_pbmp_set */
        if (SOC_REG_IS_VALID(unit, IMIRROR_BITMAP_64r)) {
            uint32 values[2];
            soc_field_t fields[] = {BITMAP_LOf, BITMAP_HIf};
            values[0] = values[1] = 0;
            PBMP_ALL_ITER(unit, port) {
                BCM_IF_ERROR_RETURN
                    (soc_reg_fields32_modify(unit, IMIRROR_BITMAP_64r, port,
                                             2, fields, values));
            }
        } else  
        
#endif /* BCM_TRIUMPH2_SUPPORT */
        if (SOC_REG_IS_VALID(unit, IMIRROR_BITMAPr)) {
            PBMP_ALL_ITER(unit, port) {
                BCM_IF_ERROR_RETURN(WRITE_IMIRROR_BITMAPr(unit, port, 0));
            }
        }
#if defined(BCM_TRX_SUPPORT)
        if (SOC_MEM_IS_VALID(unit, EGR_ERSPANm)) {
            BCM_IF_ERROR_RETURN
                (soc_mem_clear(unit, EGR_ERSPANm, COPYNO_ALL, 0));
        }
#endif /* BCM_TRX_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
        if (soc_feature(unit, soc_feature_mirror_control_mem)) {
            int i; 
            imirror_bitmap_entry_t entry;
            soc_mem_t   td_tt_mem_arr[] = { EMIRROR_CONTROLm, EMIRROR_CONTROL1m, 
                EMIRROR_CONTROL2m, EMIRROR_CONTROL3m, EGR_MIRROR_ENCAP_CONTROLm,
                EGR_MIRROR_ENCAP_DATA_1m, EGR_MIRROR_ENCAP_DATA_2m };
           
            /* Clear all valid memories upon init */
            for (i = 0; i < COUNTOF(td_tt_mem_arr); i++) {
                if (SOC_MEM_IS_VALID(unit, td_tt_mem_arr[i])) {
                    BCM_IF_ERROR_RETURN
                        (soc_mem_clear(unit, td_tt_mem_arr[i], COPYNO_ALL, 0));
                }
            }
            /* Clear settings of mirror_to_pbmp_set */
            sal_memset(&entry, 0, sizeof(imirror_bitmap_entry_t));
            PBMP_ALL_ITER(unit, port) {
                BCM_IF_ERROR_RETURN
                    (WRITE_IMIRROR_BITMAPm(unit, MEM_BLOCK_ANY, port, &entry));
            }
        }
#endif /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
        /* Clear settings of mirror_select register */
        if (SOC_REG_IS_VALID(unit, MIRROR_SELECTr)) {
            BCM_IF_ERROR_RETURN(
                 soc_reg_field32_modify(unit, MIRROR_SELECTr, REG_PORT_ANY, 
                                        MTP_TYPEf, mtp_type_undir ?
                                 BCM_MIRROR_MTP_FLEX_EGRESS_D15: 0x0));
        }
#endif /* BCM_TRIUMPH2_SUPPORT */

        /* Mirror is disabled by default on the switch. */
        MIRROR_CONFIG_MODE(unit) = BCM_MIRROR_DISABLE;
       
    } else 
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    { 
        BCM_IF_ERROR_RETURN
            (_bcm_esw_mirror_mode_set(unit, BCM_MIRROR_DISABLE));
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *   	_bcm_esw_mirror_stk_update
 * Description:
 *	Stack callback to re-program path for mirror-to-port when
 *      there is an alternate path available to the unit on which 
 *      MTP is present.
 * Parameters:
 *	unit   - (IN)BCM device number
 *      modid  - (IN)Module id. 
 *      port   - (IN)
 *      pbmp   - (IN)
 * Returns:
 *	    BCM_E_XXX
 */
int
_bcm_esw_mirror_stk_update(int unit, bcm_module_t modid, bcm_port_t port,
                           bcm_pbmp_t pbmp)
{
    /* Initialization check. */
    if (!MIRROR_INIT(unit)) {
        return (BCM_E_INIT);
    }

    /* Input parameters check. */
    if (!SOC_PORT_VALID(unit, port)) {
        return (BCM_E_PORT);
    }

    if (SOC_PBMP_IS_NULL(pbmp)) {
        return (BCM_E_NONE);
    }

#ifdef BCM_XGS_FABRIC_SUPPORT
    if (SOC_IS_HERCULES(unit) || SOC_IS_HERCULES15(unit)) {
        bcm_port_t hg_port;
        bcm_pbmp_t uc_pbmp, mir_pbmp;

        BCM_IF_ERROR_RETURN
            (bcm_esw_stk_ucbitmap_get(unit, port, modid, &uc_pbmp));

        PBMP_HG_ITER(unit, hg_port) {
            uint32 mirbmap, old_bmap;

            BCM_IF_ERROR_RETURN (READ_ING_MIRTOBMAPr(unit, hg_port,  &mirbmap));
            old_bmap = mirbmap;
            SOC_PBMP_CLEAR(mir_pbmp);
            SOC_PBMP_WORD_SET(mir_pbmp, 0, mirbmap);

            if (SOC_PBMP_EQ(mir_pbmp, uc_pbmp)) {
                soc_reg_field_set(unit, ING_MIRTOBMAPr, &mirbmap, BMAPf,
                                  SOC_PBMP_WORD_GET(pbmp, 0));
                if (old_bmap != mirbmap) {
                    BCM_IF_ERROR_RETURN
                        (WRITE_ING_MIRTOBMAPr(unit, hg_port, mirbmap));
                }
            }
        }
    }
#endif
    return (BCM_E_NONE);
}


/*
 * Function:
 *     _bcm_esw_mirror_port_ingress_dest_add 
 * Purpose:
 *      Add ingress mirroring destination to a port. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Port mirrored port.
 *      mirror_dest  -  (IN) Mirroring destination gport. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_esw_mirror_port_ingress_dest_add(int unit, bcm_port_t port,
                                      bcm_gport_t mirror_dest, int vp)
{
    int mtp_index;           /* Mirror to port index.    */
    int rv;                  /* Operation return status. */
    int enable;

    /* Allocate MTP index for mirror destination. */
    rv = _bcm_esw_mirror_mtp_reserve(unit, mirror_dest,
                                     BCM_MIRROR_PORT_INGRESS, &mtp_index);
    /* Check for mtp allocation failure. */
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    /* Enable MTP index on mirror source port */
    if (vp != VP_PORT_INVALID) {
        
        rv = _bcm_tr2_mirror_svp_enable_get(unit, vp, &enable);
        if (BCM_SUCCESS(rv)) {
            enable |= (1 << mtp_index);
            rv = _bcm_tr2_mirror_svp_enable_set(unit, vp, enable);
        }
        /* Check for mtp enable failure. */
        if (BCM_FAILURE(rv)) {
            _bcm_esw_mirror_mtp_unreserve(unit, mtp_index, TRUE, 
                                          BCM_MIRROR_PORT_INGRESS);
        }
    } else if ((-1 != port) && SOC_PORT_VALID(unit, port)) {
#if defined(BCM_XGS3_SWITCH_SUPPORT)
        if (SOC_IS_XGS3_SWITCH(unit)) {
            rv = _bcm_xgs3_mirror_ingress_mtp_install(unit, port, mtp_index);
        } else 
#endif /* BCM_XGS3_SWITCH_SUPPORT */
        {
            rv = bcm_esw_mirror_ingress_set(unit, port, TRUE);
        }

        /* Check for mtp enable failure. */
        if (BCM_FAILURE(rv)) {
            _bcm_esw_mirror_mtp_unreserve(unit, mtp_index, TRUE, 
                                          BCM_MIRROR_PORT_INGRESS);
        }
    }
    return (rv);
}

/*
 * Function:
 *     _bcm_esw_mirror_port_egress_dest_add 
 * Purpose:
 *      Add egress mirroring destination to a port. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Port mirrored port.
 *      mirror_dest  -  (IN) Mirroring destination gport. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_esw_mirror_port_egress_dest_add(int unit, bcm_port_t port, 
                                     bcm_gport_t mirror_dest, int vp)
{
    int mtp_index;           /* Mirror to port index.    */
    int rv;                  /* Operation return status. */

    /* Allocate MTP index for mirror destination. */
    rv = _bcm_esw_mirror_mtp_reserve(unit, mirror_dest,
                                     BCM_MIRROR_PORT_EGRESS, &mtp_index);
    /* Check for mtp allocation failure. */
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    /* Enable MTP index on mirror source port */
    if (vp != VP_PORT_INVALID) {
        
        rv = _bcm_tr2_mirror_dvp_enable_set(unit, vp, 1 << mtp_index);

        /* Check for mtp enable failure. */
        if (BCM_FAILURE(rv)) {
            _bcm_esw_mirror_mtp_unreserve(unit, mtp_index, TRUE, 
                                          BCM_MIRROR_PORT_EGRESS);
        }
    } else if ((-1 != port) && SOC_PORT_VALID(unit, port)) {
#if defined(BCM_XGS3_SWITCH_SUPPORT)
        if (SOC_IS_XGS3_SWITCH(unit)) {
            /* Enable MTP index on mirror source port */
            rv = _bcm_xgs3_mirror_egress_mtp_install(unit, port, mtp_index);
        } else
#endif /* BCM_XGS3_SWITCH_SUPPORT */
        {
            rv = bcm_esw_mirror_egress_set(unit, port, TRUE);
        }

        /* Check for mtp enable failure. */
        if (BCM_FAILURE(rv)) {
            _bcm_esw_mirror_mtp_unreserve(unit, mtp_index, TRUE, 
                                          BCM_MIRROR_PORT_EGRESS);
        }
    }

    return (rv);
}

#if defined(BCM_TRIUMPH2_SUPPORT)
/*
 * Function:
 *     _bcm_esw_mirror_port_egress_true_dest_add 
 * Purpose:
 *      Add egress_true mirroring destination to a port. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Port mirrored port.
 *      mirror_dest  -  (IN) Mirroring destination gport. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_esw_mirror_port_egress_true_dest_add(int unit, bcm_port_t port, 
                                          bcm_gport_t mirror_dest)
{
    int mtp_index;           /* Mirror to port index.    */
    int rv;                  /* Operation return status. */

    /* Allocate MTP index for mirror destination. */
    rv = _bcm_esw_mirror_mtp_reserve(unit, mirror_dest,
                                     BCM_MIRROR_PORT_EGRESS_TRUE, 
                                     &mtp_index);
    /* Check for mtp allocation failure. */
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    /* Enable MTP index on mirror source port */
    if ((-1 != port) && SOC_PORT_VALID(unit, port)) {
        /* Enable MTP index on mirror source port */
        rv = _bcm_tr2_mirror_egress_true_mtp_install(unit, port, mtp_index);

        /* Check for mtp enable failure. */
        if (BCM_FAILURE(rv)) {
            _bcm_esw_mirror_mtp_unreserve(unit, mtp_index, TRUE,
                                          BCM_MIRROR_PORT_EGRESS_TRUE);
        }
    }

    return (rv);
}
#endif /* BCM_TRIUMPH2_SUPPORT */

/*
 * Function:
 *     _bcm_esw_mirror_stacking_dest_update
 * Purpose:
 *      Update mirror_to bitmap for a system when stacking is enabled 
 * Parameters:
 *      unit         -  (IN) BCM device number.
 *      port         -  (IN) Port mirrored port.
 *      mirror_dest  -  (IN) Mirroring destination gport.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_esw_mirror_stacking_dest_update(int unit, bcm_port_t port, 
                                     bcm_gport_t mirror_dest)
{
    int rv = BCM_E_NONE;
    bcm_module_t mymodid;       /* Local module id.                     */
    bcm_module_t rem_modid;     /* Remote module id.                    */
    bcm_port_t port_num;        /* Port number to get to rem_modid.     */
    bcm_pbmp_t pbmp;            /* Mirror destination bitmap.           */
    uint32 mirbmap;             /* Word 0 of mirror destination bitmap. */
    int idx;                    /* Trunk members iterator.              */
    int is_local_modid;         /* Check for local trunk port */

    if (SOC_IS_TD2_TT2(unit)) {
        imirror_bitmap_entry_t  entry;
        sal_memset(&entry, 0, sizeof(entry));

        if (mirror_dest != BCM_GPORT_INVALID) {
            if (BCM_GPORT_IS_TRUNK(mirror_dest)) {
                soc_mem_field32_set(unit, IMIRROR_BITMAPm, &entry,
                                    HG_TRUNK_IDf,
                                    BCM_GPORT_TRUNK_GET(mirror_dest));
                soc_mem_field32_set(unit, IMIRROR_BITMAPm, &entry, ISTRUNKf,
                                    1);
                soc_mem_field32_set(unit, IMIRROR_BITMAPm, &entry, ENABLEf, 1);
            } else {
                rem_modid = BCM_GPORT_MODPORT_MODID_GET(mirror_dest);
                BCM_IF_ERROR_RETURN
                    (_bcm_esw_modid_is_local(unit, rem_modid,
                                             &is_local_modid));
                if (!is_local_modid) {
                    BCM_IF_ERROR_RETURN
                        (bcm_esw_stk_modport_get(unit, rem_modid,
                                                 &port_num));
                    soc_mem_field32_set(unit, IMIRROR_BITMAPm, &entry,
                                        EGRESS_PORTf, port_num);
                    soc_mem_field32_set(unit, IMIRROR_BITMAPm, &entry, ENABLEf,
                                        1);
                }
            }
        }
        BCM_IF_ERROR_RETURN
            (soc_mem_write(unit, IMIRROR_BITMAPm, MEM_BLOCK_ANY, port,
                           &entry));
        if (IS_CPU_PORT(unit, port) && SOC_INFO(unit).cpu_hg_index != -1) {
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, IMIRROR_BITMAPm, MEM_BLOCK_ANY,
                               SOC_INFO(unit).cpu_hg_index, &entry));
        }
        return BCM_E_NONE;
    }

    /* Clear destination pbmp. */
    BCM_PBMP_CLEAR(pbmp);
    mirbmap = 0;

    /* 
     * Clear mirrorto bitmap if devices are not in draco mode 
     * or mirroring is off. 
     */ 
    if (MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit) &&
        (BCM_GPORT_INVALID != mirror_dest)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_local_modid_get(unit, &mymodid));

        if (BCM_GPORT_IS_TRUNK(mirror_dest)) {
            int member_count;
            bcm_trunk_member_t *member_array = NULL;
            bcm_module_t mod_out;
            bcm_port_t port_out;
            bcm_trunk_t tgid_out;
            int id_out;

            /* Get trunk member port/module pairs. */
            BCM_IF_ERROR_RETURN
                (bcm_esw_trunk_get(unit, BCM_GPORT_TRUNK_GET(mirror_dest),
                                    NULL, 0, NULL, &member_count));
            if (member_count > 0) {
                member_array = sal_alloc(sizeof(bcm_trunk_member_t) * member_count,
                        "trunk member array");
                if (NULL == member_array) {
                    return BCM_E_MEMORY;
                }
                rv = bcm_esw_trunk_get(unit, BCM_GPORT_TRUNK_GET(mirror_dest),
                        NULL, member_count, member_array, &member_count);
                if (BCM_FAILURE(rv)) {
                    sal_free(member_array);
                    return rv;
                }
            }

            /* Fill pbmp with trunk members from other modules . */
            for (idx = 0; idx < member_count; idx++) {
                rv = _bcm_esw_gport_resolve(unit, member_array[idx].gport,
                        &mod_out, &port_out, &tgid_out, &id_out);
                if (BCM_FAILURE(rv) || (-1 != tgid_out) || (-1 != id_out)) {
                    sal_free(member_array);
                    return rv;
                }
                rv = _bcm_esw_modid_is_local(unit, mod_out, &is_local_modid);
                if (BCM_FAILURE(rv)) {
                    sal_free(member_array);
                    return rv;
                }
                if (!is_local_modid) {
                    rv = bcm_esw_stk_modport_get(unit, mod_out, &port_num);
                    if (BCM_FAILURE(rv)) {
                        sal_free(member_array);
                        return rv;
                    }
                    /* Set local port used to reach remote module to pbmp. */
                    BCM_PBMP_PORT_SET(pbmp, port_num);
                }
            }

            if (NULL != member_array) {
                sal_free(member_array);
            }
        } else {
            rem_modid = BCM_GPORT_MODPORT_MODID_GET(mirror_dest);
            BCM_IF_ERROR_RETURN(
                _bcm_esw_modid_is_local(unit, rem_modid, &is_local_modid));
            if (!is_local_modid) {
                BCM_IF_ERROR_RETURN(bcm_esw_stk_modport_get(unit, rem_modid,
                                                            &port_num));
                /* Set local port used to reach remote module to pbmp. */
                BCM_PBMP_PORT_SET(pbmp, port_num);
            }
        }

        mirbmap = SOC_PBMP_WORD_GET(pbmp, 0);
        if (SOC_IS_FBX(unit)) {
            mirbmap >>= SOC_HG_OFFSET(unit);
        }
    }
#if defined(BCM_HERCULES_SUPPORT)
    if (SOC_IS_HERCULES(unit)) {
        return WRITE_ING_MIRTOBMAPr(unit, port, mirbmap);
    }
#endif /* BCM_HERCULES_SUPPORT */
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (soc_feature(unit, soc_feature_egr_mirror_path)) {
#ifdef BCM_TRIDENT_SUPPORT
        if (soc_feature(unit, soc_feature_mirror_control_mem)) {
            imirror_bitmap_entry_t  entry;
            
            BCM_IF_ERROR_RETURN(soc_mem_read(unit, IMIRROR_BITMAPm, 
                                              MEM_BLOCK_ALL, port, &entry));
            soc_mem_pbmp_field_set(unit, IMIRROR_BITMAPm, &entry, BITMAPf, &pbmp);
            BCM_IF_ERROR_RETURN(soc_mem_write(unit, IMIRROR_BITMAPm, 
                                              MEM_BLOCK_ANY, port, &entry));
            if (IS_CPU_PORT(unit, port) &&
                (SOC_INFO(unit).cpu_hg_index != -1)) {
                BCM_IF_ERROR_RETURN
                    (soc_mem_write(unit, IMIRROR_BITMAPm, MEM_BLOCK_ANY, 
                                   SOC_INFO(unit).cpu_hg_index, &entry));
            }
        } else
#endif
#ifdef BCM_TRIUMPH2_SUPPORT
        if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
            SOC_IS_VALKYRIE2(unit)) {
            uint32 values[2];
            soc_field_t fields[] = {BITMAP_LOf, BITMAP_HIf};
            values[0] = SOC_PBMP_WORD_GET(pbmp, 0);
            values[1] = SOC_PBMP_WORD_GET(pbmp, 1);
            BCM_IF_ERROR_RETURN
                (soc_reg_fields32_modify(unit, IMIRROR_BITMAP_64r, port,
                                         2, fields, values));
        } else
#endif /* BCM_TRIUMPH2_SUPPORT */
        {
            return WRITE_IMIRROR_BITMAPr(unit, port, mirbmap);
        } 
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_esw_mirror_port_ingress_dest_delete
 * Purpose:
 *      Delete ingress mirroring destination from a port. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Port mirrored port.
 *      mirror_dest  -  (IN) Mirroring destination gport. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_esw_mirror_port_ingress_dest_delete(int unit, bcm_port_t port, 
                                         bcm_gport_t mirror_dest, int vp) 
{
    int enable;              /* Mirror enable check.     */
    int mtp_index;           /* Mirror to port index.    */
    int rv;                  /* Operation return status. */

    /* Look for used MTP index */
    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        rv = _bcm_tr2_mirror_shared_mtp_match(unit, mirror_dest, FALSE, 
                                              &mtp_index);
    } else {
        rv = _bcm_esw_mirror_ingress_mtp_match(unit, mirror_dest, &mtp_index);
    }
    if (BCM_FAILURE(rv)) {
        return (BCM_E_NOT_FOUND);
    }

    /* Disable  MTP index on mirror source port */
    if (vp != VP_PORT_INVALID) {
        
        BCM_IF_ERROR_RETURN(_bcm_tr2_mirror_svp_enable_get(unit, vp, &enable));
        if (enable & (1 << mtp_index)) {
            enable &= ~(1 << mtp_index);
            BCM_IF_ERROR_RETURN(_bcm_tr2_mirror_svp_enable_set(unit, vp, 
                 enable));
        }
        
    } else if ((-1 != port) && SOC_PORT_VALID(unit, port)) {
#if defined(BCM_XGS3_SWITCH_SUPPORT)
        if (SOC_IS_XGS3_SWITCH(unit)) {
            /* Enable MTP index on mirror source port */
            BCM_IF_ERROR_RETURN
                (_bcm_xgs3_mirror_ingress_mtp_uninstall(unit, port, mtp_index));
        } else 
#endif /* BCM_XGS3_SWITCH_SUPPORT */
        {

            BCM_IF_ERROR_RETURN(bcm_esw_mirror_ingress_get(unit, port, &enable));
            if (!enable) {
                return (BCM_E_NOT_FOUND);
            }
            BCM_IF_ERROR_RETURN(bcm_esw_mirror_ingress_set(unit, port, FALSE));
        }
    }

    /* Free MTP index for mirror destination. */
    rv =  _bcm_esw_mirror_mtp_unreserve(unit, mtp_index, TRUE, 
                                        BCM_MIRROR_PORT_INGRESS);

    return (rv);
}

/*
 * Function:
 *     _bcm_esw_mirror_port_egress_dest_delete
 * Purpose:
 *      Delete egress mirroring destination from a port. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Port mirrored port.
 *      mirror_dest  -  (IN) Mirroring destination gport. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_esw_mirror_port_egress_dest_delete(int unit, bcm_port_t port, 
                                        bcm_gport_t mirror_dest, int vp) 
{
    int enable;              /* Mirror enable check.     */
    int mtp_index;           /* Mirror to port index.    */
    int rv;                  /* Operation return status. */

    /* Look for used MTP index */
    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        rv = _bcm_tr2_mirror_shared_mtp_match(unit, mirror_dest, TRUE, 
                                              &mtp_index);
    } else {
        rv = _bcm_esw_mirror_egress_mtp_match(unit, mirror_dest, &mtp_index);
    }
    
    if (BCM_FAILURE(rv)) {
        return (BCM_E_NOT_FOUND);
    }

    /* Disable  MTP index on mirror source port */
    if (vp != VP_PORT_INVALID) {
        
        BCM_IF_ERROR_RETURN(_bcm_tr2_mirror_dvp_enable_get(unit, vp, 
                          &enable));
        /* if mtp is not enabled on port - do nothing */
        if (!(enable & (1 << mtp_index))) {
            return (BCM_E_NOT_FOUND);
        }
        /* Update egress enables */
        enable &= ~(1 << mtp_index);
        BCM_IF_ERROR_RETURN(_bcm_tr2_mirror_dvp_enable_set(unit, vp, enable));

    } else if ((-1 != port) && SOC_PORT_VALID(unit, port)) {
#if defined(BCM_XGS3_SWITCH_SUPPORT)
        if (SOC_IS_XGS3_SWITCH(unit)) {
            /* Enable MTP index on mirror source port */
            BCM_IF_ERROR_RETURN
                (_bcm_xgs3_mirror_egress_mtp_uninstall(unit, port, mtp_index));
        } else 
#endif /* BCM_XGS3_SWITCH_SUPPORT */
        {
            BCM_IF_ERROR_RETURN(bcm_esw_mirror_egress_get(unit, port, &enable));
            if (!enable) {
                return (BCM_E_NONE);
            }
            BCM_IF_ERROR_RETURN(bcm_esw_mirror_egress_set(unit, port, FALSE));
        }
    }

    /* Free MTP index for mirror destination. */
    rv =  _bcm_esw_mirror_mtp_unreserve(unit, mtp_index, TRUE, 
                                        BCM_MIRROR_PORT_EGRESS);

    return (rv);
}

#ifdef BCM_TRIUMPH2_SUPPORT
/*
 * Function:
 *     _bcm_esw_mirror_port_egress_true_dest_delete
 * Purpose:
 *      Delete egress true mirroring destination from a port. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Port mirrored port.
 *      mirror_dest  -  (IN) Mirroring destination gport. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_esw_mirror_port_egress_true_dest_delete(int unit, bcm_port_t port, 
                                        bcm_gport_t mirror_dest) 
{
    int mtp_index;           /* Mirror to port index.    */
    int rv;                  /* Operation return status. */

    /* Look for used MTP index */
    rv = _bcm_esw_mirror_egress_true_mtp_match(unit, mirror_dest,
                                               &mtp_index);
    if (BCM_FAILURE(rv)) {
        return (BCM_E_NOT_FOUND);
    }

    /* Disable MTP index on mirror source port */
    if ((-1 != port) && SOC_PORT_VALID(unit, port)) {
        /* Enable MTP index on mirror source port */
        BCM_IF_ERROR_RETURN
            (_bcm_tr2_mirror_egress_true_mtp_uninstall(unit, port,
                                                        mtp_index));
    }

    /* Free MTP index for mirror destination. */
    rv =  _bcm_esw_mirror_mtp_unreserve(unit, mtp_index, TRUE, 
                                        BCM_MIRROR_PORT_EGRESS_TRUE);

    return (rv);
}
#endif /* BCM_TRIUMPH2_SUPPORT */

#if defined(BCM_FIELD_SUPPORT) && defined(BCM_XGS3_SWITCH_SUPPORT)
/*
 * Function:
 *     _bcm_esw_mirror_fp_dest_add 
 * Purpose:
 *      Add mirroring destination to field processor module. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      modid        -  (IN) Mirroring destination module.
 *      port         -  (IN) Mirroring destination port or GPORT. 
 *      flags        -  (IN) BCM_MIRROR_PORT_XXX flags.
 *      mtp_index    -  (OUT) Allocated hw mtp index.
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_mirror_fp_dest_add(int unit, int modid, int port, 
                            uint32 flags, int *mtp_index) 
{
    bcm_mirror_destination_t mirror_dest;  /* Mirror destination.          */
    bcm_gport_t     mirror_dest_id;  /* Mirror destination id.       */
    int             rv = BCM_E_NONE; /* Operation return status.     */
    uint32          destroy_flag = FALSE; /* mirror destination destroy */

    /* At least one packet direction must be specified. */
    if (!(flags & (BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_EGRESS |
                   BCM_MIRROR_PORT_EGRESS_TRUE))) {
        return (BCM_E_PARAM);
    }

    /* Can't reserve multiple types of mtp in 1 shot. */
    if (((flags & BCM_MIRROR_PORT_INGRESS) &&
        (flags & (BCM_MIRROR_PORT_EGRESS | BCM_MIRROR_PORT_EGRESS_TRUE))) ||
        ((flags & BCM_MIRROR_PORT_EGRESS) &&
         (flags & BCM_MIRROR_PORT_EGRESS_TRUE))) {
        return (BCM_E_PARAM);
    }

    if (!soc_feature(unit, soc_feature_egr_mirror_true) &&
        (flags & BCM_MIRROR_PORT_EGRESS_TRUE)) {
        return (BCM_E_PARAM);
    }

    /* Initialization check */
    if (!MIRROR_INIT(unit)) {
        return (BCM_E_INIT);
    }

    /* Create traditional mirror destination. */
    bcm_mirror_destination_t_init(&mirror_dest);

    if ((flags & BCM_MIRROR_PORT_EGRESS_TRUE) &&
        MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
        return (BCM_E_CONFIG);
    }

    MIRROR_LOCK(unit);

    if (BCM_GPORT_IS_MIRROR(port)) {
        rv = bcm_esw_mirror_destination_get(unit, port, &mirror_dest);
    } else {
        rv = _bcm_esw_mirror_destination_find(unit, port, modid, flags, &mirror_dest); 
        if (BCM_E_NOT_FOUND == rv) {
            mirror_dest.flags |= _BCM_MIRROR_DESTINATION_LOCAL;
            rv = _bcm_esw_mirror_destination_create(unit, &mirror_dest);
            destroy_flag = TRUE;
        }       
    }
    if (BCM_FAILURE(rv)) {
        MIRROR_UNLOCK(unit);
        return rv;
    }
    mirror_dest_id = mirror_dest.mirror_dest_id;
    /* Single mirroring destination for ingress & egress. */
    if (MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
        if (BCM_GPORT_IS_TRUNK(mirror_dest.gport)) {
            if (destroy_flag) {
               (void)bcm_esw_mirror_destination_destroy(unit, 
                                                mirror_dest.mirror_dest_id); 
            }
            MIRROR_UNLOCK(unit);
            return (BCM_E_UNAVAIL);
        }

        if (soc_feature(unit, soc_feature_mirror_flexible)) {
            if (MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, 0) && 
                MIRROR_CONFIG_SHARED_MTP_DEST(unit, 0) != mirror_dest_id) {
                if (destroy_flag) {
                    (void)bcm_esw_mirror_destination_destroy(unit, 
                                         mirror_dest.mirror_dest_id); 
                }
                MIRROR_UNLOCK(unit);
                return (BCM_E_RESOURCE);
            }
        } else {
            if (MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0)) {
                if ((MIRROR_CONFIG_ING_MTP_DEST(unit, 0) != mirror_dest_id) &&
                    (MIRROR_CONFIG_EGR_MTP_DEST(unit, 0) != mirror_dest_id)) {
                    if (destroy_flag) {
                       (void)bcm_esw_mirror_destination_destroy(unit, 
                                            mirror_dest.mirror_dest_id); 
                    }
                    MIRROR_UNLOCK(unit);
                    return (BCM_E_RESOURCE);
                }
            }
        }
    }
     

    /* Reserve & initialize mtp index based on traffic direction. */
    if (flags & BCM_MIRROR_PORT_INGRESS) {
        rv = _bcm_xgs3_mirror_ingress_mtp_reserve(unit, mirror_dest_id, 
                                                  mtp_index);
    } else if (flags & BCM_MIRROR_PORT_EGRESS) {
        rv = _bcm_xgs3_mirror_egress_mtp_reserve(unit, mirror_dest_id,
                                                 FALSE, mtp_index);
#ifdef BCM_TRIUMPH2_SUPPORT
    } else if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
        rv = _bcm_xgs3_mirror_egress_true_mtp_reserve(unit, mirror_dest_id,
                                                      mtp_index);
#endif /* BCM_TRIUMPH2_SUPPORT */
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    /* MTP slot reservation for FP's */
    if (BCM_SUCCESS(rv) &&
        soc_feature(unit, soc_feature_mirror_flexible) &&
        MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        int             mtp_slot;        /* Flexible mirroring slot */

        /* Determine a usable MTP slot for this FP entry */
        if (0 == (flags & BCM_MIRROR_PORT_EGRESS_TRUE)) {
            rv = _bcm_xgs3_mtp_type_slot_reserve(unit, flags,
                                                 0, 0, /* Unused */
                                                 BCM_MTP_SLOT_TYPE_FP,
                                                 *mtp_index, &mtp_slot);
            if (BCM_SUCCESS(rv)) {
                *mtp_index |= (mtp_slot << BCM_MIRROR_MTP_FLEX_SLOT_SHIFT);
            }    
        } else {
            /* Egress true uses 1-1 mapping */
            *mtp_index |= (*mtp_index << BCM_MIRROR_MTP_FLEX_SLOT_SHIFT);
        }
    } 
#endif /* BCM_TRIUMPH2_SUPPORT */

    /* Enable mirroring on a port.  */
    if (BCM_SUCCESS(rv)) { 
        if(!SOC_IS_XGS3_SWITCH(unit) || 
           (BCM_MIRROR_DISABLE == MIRROR_CONFIG_MODE(unit))) {
            rv = _bcm_esw_mirror_enable(unit);
            MIRROR_CONFIG_MODE(unit) = BCM_MIRROR_L2;
        }
    } 

    if (BCM_FAILURE(rv) && destroy_flag) {
        (void)bcm_esw_mirror_destination_destroy(unit, mirror_dest.mirror_dest_id); 
    }
    MIRROR_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *     _bcm_esw_mirror_fp_dest_delete
 * Purpose:
 *      Delete fp mirroring destination.
 * Parameters:
 *      unit         -  (IN) BCM device number.
 *      mtp_index    -  (IN) Mirror destination index.
 *      flags        -  (IN) Mirror direction flags.
 * Returns:
 *      BCM_X_XXX
 * Notes: 
 */
int
_bcm_esw_mirror_fp_dest_delete(int unit, int mtp_index, uint32 flags)
{
    int rv = BCM_E_NONE;                      /* Operation return status. */
    bcm_mirror_destination_t mirror_dest;     /* Mirror destination.       */
    bcm_gport_t              mirror_dest_id;  /* Mirror destination id.    */

    mirror_dest_id = BCM_GPORT_INVALID;

    /* Input parameters check. */
    /* At least one packet direction must be specified. */
    if (!(flags & (BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_EGRESS |
                   BCM_MIRROR_PORT_EGRESS_TRUE))) {
        return (BCM_E_PARAM);
    }

    /* Can't reserve multiple types of mtp in 1 shot. */
    if (((flags & BCM_MIRROR_PORT_INGRESS) &&
        (flags & (BCM_MIRROR_PORT_EGRESS | BCM_MIRROR_PORT_EGRESS_TRUE))) ||
        ((flags & BCM_MIRROR_PORT_EGRESS) &&
         (flags & BCM_MIRROR_PORT_EGRESS_TRUE))) {
        return (BCM_E_PARAM);
    }

    if (!soc_feature(unit, soc_feature_egr_mirror_true) &&
        (flags & BCM_MIRROR_PORT_EGRESS_TRUE)) {
        return (BCM_E_PARAM);
    }

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    MIRROR_LOCK(unit);
    if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
        mirror_dest_id = MIRROR_CONFIG_EGR_TRUE_MTP_DEST(unit, mtp_index);
    } else {
        if (soc_feature(unit, soc_feature_mirror_flexible) &&
            !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
                mirror_dest_id =
                    MIRROR_CONFIG_SHARED_MTP_DEST(unit, mtp_index);
        } else {
            if (flags & BCM_MIRROR_PORT_EGRESS) {
                mirror_dest_id = MIRROR_CONFIG_EGR_MTP_DEST(unit, mtp_index);
            }  else if (flags & BCM_MIRROR_PORT_INGRESS) {
                mirror_dest_id = MIRROR_CONFIG_ING_MTP_DEST(unit, mtp_index);
            } else {
                rv = BCM_E_PARAM;
            }

#ifdef BCM_TRIUMPH2_SUPPORT
            if (BCM_SUCCESS(rv) &&
                soc_feature(unit, soc_feature_mirror_flexible) &&
                MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
                rv = _bcm_xgs3_mtp_type_slot_unreserve(unit, flags,
                                                       0, /* Unused */
                                                       BCM_MTP_SLOT_TYPE_FP,
                                                       mtp_index);
            }
#endif /* BCM_TRIUMPH2_SUPPORT */
        }
    }

    /* Get mirror destination descriptor. */
    if (BCM_SUCCESS(rv)) {
        rv = bcm_esw_mirror_destination_get(unit, mirror_dest_id,
                                            &mirror_dest);
    }

    /* Free MTP index for mirror destination. */
    if (BCM_SUCCESS(rv)) {
        rv = _bcm_esw_mirror_mtp_unreserve(unit, mtp_index, FALSE, flags);
    }

    /* Destroy mirror destination if it was created by fp action add. */ 
    if (BCM_SUCCESS(rv)) {
        if ((mirror_dest.flags & _BCM_MIRROR_DESTINATION_LOCAL) && 
            (1 >= MIRROR_DEST_REF_COUNT(unit, mirror_dest.mirror_dest_id))) {
            rv = bcm_esw_mirror_destination_destroy(unit,
                                            mirror_dest.mirror_dest_id);
        }
    }

    MIRROR_UNLOCK(unit);
    return(rv);
}

#endif /* BCM_FIELD_SUPPORT  && BCM_XGS3_SWITCH_SUPPORT */

/*
 * Function:
 *	    _bcm_esw_mirror_enable
 * Purpose:
 *	    Set mirror enable = TRUE on all ports.
 * Parameters:
 *	    unit - (IN) BCM device number
 * Returns:
 *   	BCM_E_XXX
 * Notes:
 *      When egress or fp mirroring is enabled, we need to enable 
 *      mirroring on all ports even if mirroring is only explicitely
 *      enabled on a single port. This function ensures that the mirror
 *      enable bit is toggled correctly on all ports, when
 *      bcm_mirror_port_dest_xxx style apis are used by application.
 */
STATIC int
_bcm_esw_mirror_enable(int unit)
{
    bcm_port_t port;
    PBMP_ALL_ITER(unit, port) {
        BCM_IF_ERROR_RETURN(_bcm_esw_mirror_enable_set(unit, port, TRUE));
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *	_bcm_esw_mirror_port_dest_search
 * Purpose:
 *	Search to see if the the given port, destination exists.
 * Parameters:
 *	unit         - (IN) BCM device number
 *      port         -  (IN) Port mirrored port.
 *      flags        -  (IN) BCM_MIRROR_PORT_XXX flags.
 *      mirror_dest  -  (IN) Mirroring destination gport. 
 * Returns:
 *   	BCM_E_XXX
 * Notes:
 *      This should be called with only one of the
 *      INGRESS/EGRESS/TRUE_EGRESS flags set.
 */
STATIC int
_bcm_esw_mirror_port_dest_search(int unit, bcm_port_t port, 
                                 uint32 flags, bcm_gport_t mirror_dest)
{
    bcm_gport_t mirror_dest_list[BCM_MIRROR_MTP_COUNT];
    int mirror_dest_count, mtp;

    BCM_IF_ERROR_RETURN
        (bcm_esw_mirror_port_dest_get(unit, port, flags,
                                      BCM_MIRROR_MTP_COUNT,
                                      mirror_dest_list, &mirror_dest_count));

    for (mtp = 0; mtp < mirror_dest_count; mtp++) {
        if (mirror_dest_list[mtp] == mirror_dest) {
            return BCM_E_EXISTS;
        }
    }

    return BCM_E_NOT_FOUND;
}


/*
 * Function:
 *	  bcm_esw_mirror_deinit
 * Purpose:
 *	  Deinitialize mirror software module.
 * Parameters:
 *    unit - (IN) BCM device number.
 * Returns:
 *	  BCM_E_XXX
 */
int
bcm_esw_mirror_deinit(int unit)
{
#ifdef BCM_SHADOW_SUPPORT
    if (soc_feature(unit, soc_feature_no_mirror)) {
        return BCM_E_NONE;
    }
#endif
    /* Call internal sw structures clean up routine. */
    return _bcm_esw_mirror_deinit(unit, &MIRROR_CONFIG(unit));
}

/*
 * Function:
 *	  bcm_esw_mirror_init
 * Purpose:
 *	  Initialize mirror software system.
 * Parameters:
 *    unit - (IN) BCM device number.
 * Returns:
 *	  BCM_E_XXX
 */
int
bcm_esw_mirror_init(int unit)
{
    _bcm_mirror_config_p mirror_cfg_ptr;/* Mirror module config structue. */
    bcm_mirror_destination_t *mdest;    /* Mirror destinations iterator.  */
    int directed;                       /* Directed mirroring enable.     */
    int alloc_sz;                       /* Memory allocation size.        */
    int idx;                            /* MTP iteration index.           */
    int rv;                             /* Operation return status.       */
#ifdef BCM_WARM_BOOT_SUPPORT
    uint8 *mtp_scache;
    int mtp_num;                        /* Maximum number of MTP dests    */
    soc_scache_handle_t scache_handle;  /* SCache reference number        */
#endif /* BCM_WARM_BOOT_SUPPORT */
#ifdef BCM_SHADOW_SUPPORT
    if (soc_feature(unit, soc_feature_no_mirror)) {
        return BCM_E_NONE;
    }
#endif

    /* Deinitialize the module if it was previously initialized. */
    if (NULL != MIRROR_CONFIG(unit)) {
        _bcm_esw_mirror_deinit(unit, &MIRROR_CONFIG(unit));
    }

    /* Allocate mirror config structure. */
    alloc_sz = sizeof(_bcm_mirror_config_t);
    mirror_cfg_ptr = sal_alloc(alloc_sz, "Mirror module");
    if (NULL == mirror_cfg_ptr) {
        return (BCM_E_MEMORY);
    }
    sal_memset(mirror_cfg_ptr, 0, alloc_sz);

    rv = _bcm_esw_directed_mirroring_get(unit, &directed);
    if (BCM_FAILURE(rv)) {
        _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
        return(BCM_E_INTERNAL);
    }

    if (directed) {
        if (soc_feature(unit, soc_feature_mirror_flexible)) {
            
            if (_bcm_mirror_mtp_method_init[unit] ==
                BCM_MIRROR_MTP_METHOD_DIRECTED_FLEXIBLE) {
                mirror_cfg_ptr->mtp_method =
                    BCM_MIRROR_MTP_METHOD_DIRECTED_FLEXIBLE;
            } else {
                mirror_cfg_ptr->mtp_method =
                    BCM_MIRROR_MTP_METHOD_DIRECTED_LOCKED;
            }
        } else {
            mirror_cfg_ptr->mtp_method =
                BCM_MIRROR_MTP_METHOD_DIRECTED_LOCKED;
        }
    } else {
        mirror_cfg_ptr->mtp_method = BCM_MIRROR_MTP_METHOD_NON_DIRECTED;
    }
    _bcm_mirror_mtp_method_init[unit] =
        mirror_cfg_ptr->mtp_method;

#ifdef BCM_TRIDENT_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
        soc_mem_t mem;
        soc_mem_t mems[3];
        int mem_words[3];
        int mems_cnt;
        egr_port_entry_t egr_port_entry;
        int port;
        mem = EGR_MIRROR_ENCAP_CONTROLm;
        if (SOC_MEM_IS_VALID(unit, mem)) {
            if (NULL == EGR_MIRROR_ENCAP(unit)) {
                EGR_MIRROR_ENCAP(unit) =
                    sal_alloc(sizeof(soc_profile_mem_t),
                              "EGR_MIRROR_ENCAP Profile Mems");
                if (NULL == EGR_MIRROR_ENCAP(unit)) {
                    _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
                    return BCM_E_MEMORY;
                }
                soc_profile_mem_t_init(EGR_MIRROR_ENCAP(unit));

                mems_cnt = 0;
                mems[mems_cnt] = mem;
                mem_words[mems_cnt] =
                    sizeof(egr_mirror_encap_control_entry_t) /
                    sizeof(uint32);
                mems_cnt++;

                mem = EGR_MIRROR_ENCAP_DATA_1m;
                if (SOC_MEM_IS_VALID(unit, mem)) {
                    mems[mems_cnt] = mem;
                    mem_words[mems_cnt] =
                        sizeof(egr_mirror_encap_data_1_entry_t) /
                        sizeof(uint32);
                    mems_cnt++;
                }

                mem = EGR_MIRROR_ENCAP_DATA_2m;
                if (SOC_MEM_IS_VALID(unit, mem)) {
                    mems[mems_cnt] = mem;
                    mem_words[mems_cnt] =
                        sizeof(egr_mirror_encap_data_2_entry_t) /
                        sizeof(uint32);
                    mems_cnt++;
                }
                rv = soc_profile_mem_create(unit, mems, mem_words,
                                            mems_cnt,
                                      EGR_MIRROR_ENCAP(unit));
                if (rv < 0) {
                    _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
                    return rv;
                }
                PBMP_ALL_ITER(unit, port) { /* Intialize the Encap index */
                    rv = READ_EGR_PORTm(unit, MEM_BLOCK_ANY, port, &egr_port_entry);
                    if (BCM_SUCCESS(rv)) {
                        soc_EGR_PORTm_field32_set(unit, &egr_port_entry,
                                                  MIRROR_ENCAP_ENABLEf, 0);
                        soc_EGR_PORTm_field32_set(unit, &egr_port_entry,
                                                  MIRROR_ENCAP_INDEXf, 0);
                        rv = WRITE_EGR_PORTm(unit, MEM_BLOCK_ANY, port, &egr_port_entry);
                    }
                }
            }
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */


#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_egr_mirror_true)) {
        mirror_cfg_ptr->egr_true_mtp_count = BCM_MIRROR_MTP_COUNT;
    }
#endif /* BCM_TRIUMPH2_SUPPORT */
    mirror_cfg_ptr->egr_mtp_count = BCM_MIRROR_MTP_COUNT;
    mirror_cfg_ptr->ing_mtp_count = BCM_MIRROR_MTP_COUNT;

#ifdef BCM_WARM_BOOT_SUPPORT 
    /* Determine maximum number used in any config */
    mtp_num =
        mirror_cfg_ptr->egr_mtp_count + mirror_cfg_ptr->ing_mtp_count;
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_egr_mirror_true)) {
       mtp_num += mirror_cfg_ptr->egr_true_mtp_count;
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    alloc_sz = sizeof(bcm_gport_t) * mtp_num;
    SOC_SCACHE_HANDLE_SET(scache_handle,
                          unit, BCM_MODULE_MIRROR, 0);
    rv = _bcm_esw_scache_ptr_get(unit, scache_handle,
                                 (0 == SOC_WARM_BOOT(unit)),
                                 alloc_sz,
                                 &mtp_scache, BCM_WB_DEFAULT_VERSION, NULL);
    if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
        _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
        return rv;
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    if (!directed) {
#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_feature(unit, soc_feature_egr_mirror_true)) {
            mirror_cfg_ptr->egr_true_mtp_count = 0;
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
        mirror_cfg_ptr->egr_mtp_count = 1;
        mirror_cfg_ptr->ing_mtp_count = 1;
    } else if (!soc_feature(unit, soc_feature_mirror_flexible)
               && SOC_IS_TRX(unit)) {
        /* Limited egress mirroring bitmap registers */
        mirror_cfg_ptr->egr_mtp_count = 2;
        
        if (SOC_IS_ENDURO(unit)) {
            /* Limited ingress mirroring bitmap registers */
            mirror_cfg_ptr->ing_mtp_count = 2;
        }
    }

    if (!directed || soc_feature(unit, soc_feature_mirror_flexible)) {
        mirror_cfg_ptr->port_em_mtp_count = mirror_cfg_ptr->egr_mtp_count;
        mirror_cfg_ptr->port_im_mtp_count = mirror_cfg_ptr->ing_mtp_count;
    } else {
        if (SOC_IS_TRX(unit) && !SOC_IS_HURRICANEX(unit)) {
            mirror_cfg_ptr->port_em_mtp_count = 2;
            mirror_cfg_ptr->port_im_mtp_count = 2;
        } else {
            mirror_cfg_ptr->port_em_mtp_count = 1;
            mirror_cfg_ptr->port_im_mtp_count = 1;
        }
    }

    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        mirror_cfg_ptr->mtp_dev_mask = BCM_TR2_MIRROR_MTP_MASK;
    } else if (SOC_IS_HURRICANE(unit) || SOC_IS_HURRICANE2(unit)) {
        mirror_cfg_ptr->mtp_dev_mask = BCM_XGS3_MIRROR_MTP_MASK;
    } else if (SOC_IS_TRX(unit)) {
        mirror_cfg_ptr->mtp_dev_mask = BCM_TRX_MIRROR_MTP_MASK;
    } else {
        mirror_cfg_ptr->mtp_dev_mask = BCM_XGS3_MIRROR_MTP_MASK;
    }

    /* Allocate mirror destinations structure. */
    if (!directed) {
        mirror_cfg_ptr->dest_count = 1;
    } else if (soc_feature(unit, soc_feature_mirror_flexible) &&
               (BCM_MIRROR_MTP_METHOD_DIRECTED_LOCKED ==
                mirror_cfg_ptr->mtp_method)) {
        mirror_cfg_ptr->dest_count = BCM_MIRROR_MTP_COUNT;
    } else {
        mirror_cfg_ptr->dest_count = 
            (mirror_cfg_ptr->egr_mtp_count + mirror_cfg_ptr->ing_mtp_count);
    }
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_egr_mirror_true) && (directed)) {
        mirror_cfg_ptr->dest_count += mirror_cfg_ptr->egr_true_mtp_count;
    }
#endif /* BCM_TRIUMPH2_SUPPORT */
    alloc_sz = 
        mirror_cfg_ptr->dest_count * sizeof(_bcm_mirror_dest_config_t);

    mirror_cfg_ptr->dest_arr = sal_alloc(alloc_sz, "Mirror destinations");
    if (NULL == mirror_cfg_ptr->dest_arr) {
        _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
        return (BCM_E_MEMORY);
    }
    sal_memset(mirror_cfg_ptr->dest_arr, 0, alloc_sz);
    for (idx = 0; idx < mirror_cfg_ptr->dest_count; idx++) {
        mdest = &mirror_cfg_ptr->dest_arr[idx].mirror_dest;
        BCM_GPORT_MIRROR_SET(mdest->mirror_dest_id, idx);
    }

    /* Allocate mirror destinations structure. */
    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        (BCM_MIRROR_MTP_METHOD_DIRECTED_FLEXIBLE !=
                mirror_cfg_ptr->mtp_method)) {
        alloc_sz = BCM_MIRROR_MTP_COUNT * sizeof(_bcm_mtp_config_t);
        mirror_cfg_ptr->shared_mtp = sal_alloc(alloc_sz, "Shared MTP indexes");
        if (NULL == mirror_cfg_ptr->shared_mtp) {
            _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
            return (BCM_E_MEMORY);
        }
        sal_memset(mirror_cfg_ptr->shared_mtp, 0, alloc_sz);
    } else {
        /* Allocate egress mirror destinations structure. */
        alloc_sz = mirror_cfg_ptr->egr_mtp_count * sizeof(_bcm_mtp_config_t);
        mirror_cfg_ptr->egr_mtp  = sal_alloc(alloc_sz, "Egress MTP indexes");
        if (NULL == mirror_cfg_ptr->egr_mtp) {
            _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
            return (BCM_E_MEMORY);
        }
        sal_memset(mirror_cfg_ptr->egr_mtp, 0, alloc_sz);

        /* Allocate ingress mirror destinations structure. */
        alloc_sz = mirror_cfg_ptr->ing_mtp_count * sizeof(_bcm_mtp_config_t);
        mirror_cfg_ptr->ing_mtp  = sal_alloc(alloc_sz, "Ingress MTP indexes");
        if (NULL == mirror_cfg_ptr->ing_mtp) {
            _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
            return (BCM_E_MEMORY);
        }
        sal_memset(mirror_cfg_ptr->ing_mtp, 0, alloc_sz);
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_egr_mirror_true)) {
        /* Allocate egress true mirror destinations structure. */
        alloc_sz =
            mirror_cfg_ptr->egr_true_mtp_count * sizeof(_bcm_mtp_config_t);
        mirror_cfg_ptr->egr_true_mtp  = sal_alloc(alloc_sz,
                                                  "Egress true MTP indexes");
        if (NULL == mirror_cfg_ptr->egr_true_mtp) {
            _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
            return (BCM_E_MEMORY);
        }
        sal_memset(mirror_cfg_ptr->egr_true_mtp, 0, alloc_sz);
    }

    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        (BCM_MIRROR_MTP_METHOD_DIRECTED_FLEXIBLE ==
        mirror_cfg_ptr->mtp_method)) {
        int mtp_type;

        for (mtp_type = BCM_MTP_SLOT_TYPE_PORT;
             mtp_type < BCM_MTP_SLOT_TYPES; mtp_type++) {
            /* Allocate MTP types records. */
            mirror_cfg_ptr->mtp_slot_count[mtp_type] = 4;

            alloc_sz = mirror_cfg_ptr->mtp_slot_count[mtp_type] *
                sizeof(_bcm_mtp_config_t);
            mirror_cfg_ptr->mtp_slot[mtp_type]  = sal_alloc(alloc_sz,
                                                      "Typed MTP indexes");
            if (NULL == mirror_cfg_ptr->mtp_slot[mtp_type]) {
                _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
                return (BCM_E_MEMORY);
            }
            sal_memset(mirror_cfg_ptr->mtp_slot[mtp_type], 0, alloc_sz);
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    /* Create protection mutex. */
    mirror_cfg_ptr->mutex = sal_mutex_create("Meter module mutex");
    if (NULL == mirror_cfg_ptr->mutex) {
        _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
        return (BCM_E_MEMORY);
    } 

    /* Take protection mutex for initial state setting & hw clear. */
    sal_mutex_take(mirror_cfg_ptr->mutex, sal_mutex_FOREVER);

    MIRROR_CONFIG(unit) = mirror_cfg_ptr;

#ifdef BCM_WARM_BOOT_SUPPORT 
    if (SOC_WARM_BOOT(unit)) {
        /* Reload mirror configuration info from HW */
        rv = _bcm_esw_mirror_reload(unit, directed);
    } else 
#endif /* BCM_WARM_BOOT_SUPPORT */
    {
        /* Clear memories/registers. */
        rv  = _bcm_esw_mirror_hw_clear(unit);
    }

    if (BCM_FAILURE(rv)) {
        MIRROR_UNLOCK(unit);
        _bcm_esw_mirror_deinit(unit, &mirror_cfg_ptr);
        MIRROR_CONFIG(unit) = NULL;
        return (BCM_E_FAIL);
    }

    MIRROR_UNLOCK(unit);

    return (BCM_E_NONE);
}



/*
 * Function:
 *	  bcm_esw_mirror_mode_set
 * Description:
 *	  Enable or disable mirroring.  Will wait for bcm_esw_mirror_to_set
 *        to be called to actually do the enable if needed.
 * Parameters:
 *        unit - (IN) BCM device number
 *	  mode - (IN) One of BCM_MIRROR_(DISABLE|L2|L2_L3)
 * Returns:
 *	  BCM_E_XXX
 */
int
bcm_esw_mirror_mode_set(int unit, int mode)
{
    int      rv = BCM_E_UNAVAIL;   /* Operation return status.    */

    /* Initialization check */
    if (0 == MIRROR_INIT(unit)) {
        return (BCM_E_INIT);
    }

    /* Input parameters check. */
    if ((BCM_MIRROR_L2 != mode) && 
        (BCM_MIRROR_L2_L3 != mode) && 
        (BCM_MIRROR_DISABLE != mode)) {
          return (BCM_E_PARAM);
    }

    if (MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        /* Not supported for flexible MTP mode */
        return (BCM_E_CONFIG);
    }

    MIRROR_LOCK(unit);
    rv = _bcm_esw_mirror_mode_set(unit, mode);
    MIRROR_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *   	bcm_esw_mirror_mode_get
 * Description:
 *	    Get mirror mode. (L2/L2_L3/DISABLED).
 * Parameters:
 *	    unit - BCM device number
 *	    mode - (OUT) One of BCM_MIRROR_(DISABLE|L2|L2_L3)
 * Returns:
 *	    BCM_E_XXX
 */
int
bcm_esw_mirror_mode_get(int unit, int *mode)
{
    /* Initialization check */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Input parameters check. */
    if (NULL == mode) {
        return BCM_E_PARAM;
    }
    MIRROR_LOCK(unit);
    *mode = MIRROR_CONFIG_MODE(unit);
    MIRROR_UNLOCK(unit);

    return (BCM_E_NONE);
}

/*
 * Function:
 *	   bcm_esw_mirror_to_set
 * Description:
 *	   Set the mirror-to port for all mirroring, enabling mirroring
 *	   if a mode has previously been set.
 * Parameters:
 *	   unit - (IN) BCM device number
 *	   port - (IN) The port to mirror all ingress/egress selections to
 * Returns:
 *	   BCM_E_XXX
 * Notes:
 *     When mirroring to a remote unit, the mirror-to port
 *     should be the appropriate stack port on the local unit.
 *     This will return BCM_E_CONFIG if the unit is configured for,
 *     or only supports directed mirroring.
 */
int
bcm_esw_mirror_to_set(int unit, bcm_port_t port)
{
    bcm_mirror_destination_t mirror_dest;    /* Destination port/trunk.     */
    int rv;                                  /* Operation return status.    */
    int mod_out, port_out;                   /* Module and port for mapping */
    bcm_gport_t gport;                       /* Local gport operations      */

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    gport = port;
    if (BCM_GPORT_IS_SET(gport)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, gport, &port));
    }
    /* Input parameters check. */
    if (!SOC_PORT_VALID(unit, port)) {
        return (BCM_E_PORT);
    }

    bcm_mirror_destination_t_init(&mirror_dest);

    if (!MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
        return (BCM_E_CONFIG);
    }

    if (BCM_GPORT_IS_SET(gport)) {
        mirror_dest.gport = gport;
    } else {
        BCM_IF_ERROR_RETURN(bcm_esw_port_gport_get(unit, port, &gport));
        BCM_IF_ERROR_RETURN(
            _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                   SOC_GPORT_MODPORT_MODID_GET(gport),
                                   SOC_GPORT_MODPORT_PORT_GET(gport),
                                   &mod_out, &port_out));
        BCM_IF_ERROR_RETURN(
            _bcm_mirror_gport_construct(unit, port_out, mod_out, 0, 
                                        &(mirror_dest.gport)));
    }

    /* Create traditional mirror destination. */
    BCM_GPORT_MIRROR_SET(mirror_dest.mirror_dest_id, 0);
    mirror_dest.flags = BCM_MIRROR_DEST_WITH_ID | BCM_MIRROR_DEST_REPLACE;
    
    MIRROR_LOCK(unit);

    rv = bcm_esw_mirror_destination_create(unit, &mirror_dest);
    if (BCM_FAILURE(rv)) {
        MIRROR_UNLOCK(unit);
        return (rv);
    }

    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        /* Ingress */
        MIRROR_CONFIG_SHARED_MTP_DEST(unit, 0) = mirror_dest.mirror_dest_id;
        MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, 0) = 1;
        /* Egress */
        MIRROR_CONFIG_SHARED_MTP_DEST(unit,
                             BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX) =
            mirror_dest.mirror_dest_id;
        MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit,
                             BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX) = 1;
    } else {
        MIRROR_CONFIG_ING_MTP_DEST(unit, 0) = mirror_dest.mirror_dest_id;
        MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0) = 1;
    }

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        /* Ingress & Egress configuration is identical. */
        if (soc_feature(unit, soc_feature_mirror_flexible)) {
            /* For non-directed flexible mirroring, we use MTP 0 for
             * the ingress and MTP 2 for the egress. */
            rv = _bcm_xgs3_mtp_init(unit, 0, BCM_MIRROR_PORT_INGRESS);
            rv = _bcm_xgs3_mtp_init(unit,
                                    BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX,
                                    BCM_MIRROR_PORT_EGRESS);
        } else {
            MIRROR_CONFIG_EGR_MTP(unit, 0) = MIRROR_CONFIG_ING_MTP(unit, 0);
            /* Write MTP registers */
            rv = _bcm_xgs3_mtp_init(unit, 0, (BCM_MIRROR_PORT_EGRESS |
                                              BCM_MIRROR_PORT_INGRESS));
        }
        if (BCM_FAILURE(rv)) {
            MIRROR_UNLOCK(unit);
            return (rv);
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    rv = bcm_esw_mirror_mode_set(unit, MIRROR_CONFIG_MODE(unit));
    MIRROR_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *	bcm_esw_mirror_to_get
 * Description:
 *	Get the mirror-to port for all mirroring
 * Parameters:
 *	unit - (IN)  BCM device number
 *	port - (OUT) The port to mirror all ingress/egress selections to
 * Returns:
 *	BCM_E_XXX
 */
int
bcm_esw_mirror_to_get(int unit, bcm_port_t *port)
{
    uint32          flags;      /* Mirror destination flags. */
    int             rv;         /* Operation return status  */
    int             isGport;    /* Indicator on which format to return port */
    bcm_module_t    mymodid, modid;    /* module id to construct a gport */
    bcm_gport_t     gport;
    int             mod_out, port_out; /* To do a modmap mapping */
    int             mirror_cnt = 0; 

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Input parameters check. */
    if (NULL == port) {
        return (BCM_E_PARAM);
    }

    flags = 0;
    BCM_IF_ERROR_RETURN(
        bcm_esw_stk_my_modid_get(unit, &mymodid));

    MIRROR_LOCK(unit);

    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        mirror_cnt = MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, 0);
        gport = MIRROR_CONFIG_SHARED_MTP_DEST(unit, 0);
    } else {
        mirror_cnt = MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0);
        gport = MIRROR_CONFIG_ING_MTP_DEST(unit, 0);
    }
    if (mirror_cnt) {
         rv = _bcm_mirror_destination_gport_parse(unit, gport ,&modid, 
                                                  port, &flags);
    } else {
        *port = -1;
        modid = mymodid;
        rv = BCM_E_NONE;
    }
    MIRROR_UNLOCK(unit);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    if (flags & BCM_MIRROR_PORT_DEST_TRUNK) { 
        return (BCM_E_CONFIG);
    }

    BCM_IF_ERROR_RETURN
        (bcm_esw_switch_control_get(unit, bcmSwitchUseGport, &isGport));
    if (isGport) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET, mymodid, *port, 
                                   &mod_out, &port_out));
        BCM_IF_ERROR_RETURN
            (_bcm_mirror_gport_construct(unit, port_out, mod_out,
                                         flags, port)); 
    } else if (*port != -1) {
        BCM_GPORT_MODPORT_SET(gport, modid, *port);
        BCM_IF_ERROR_RETURN
            (bcm_esw_port_local_get(unit, gport, port));
    }
    
    return (BCM_E_NONE);
}

/*
 * Function:
 *   	bcm_esw_mirror_ingress_set
 * Description:
 *	    Enable or disable mirroring per ingress
 * Parameters:
 *   	unit   - (IN) BCM device number
 *	    port   - (IN) The port to affect
 *   	enable - (IN) Boolean value for on/off
 * Returns:
 *	    BCM_E_XXX
 * Notes:
 *	    Mirroring must also be globally enabled.
 */
int
bcm_esw_mirror_ingress_set(int unit, bcm_port_t port, int enable)
{
    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }
    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
    }

    /* Input parameters check. */
    if (!SOC_PORT_VALID(unit, port)) {
        return (BCM_E_PORT);
    }

    if (IS_CPU_PORT(unit, port) && 
        !soc_feature(unit, soc_feature_cpuport_mirror)) {
        return (BCM_E_PORT);
    }

    /* Set ingress mirroring enable in port table. */
    return _bcm_port_mirror_enable_set(unit, port, 
                                       ((enable) ?  BCM_MIRROR_MTP_ONE : (0)));

}

/*
 * Function:
 * 	    bcm_esw_mirror_ingress_get
 * Description:
 * 	    Get the mirroring per ingress enabled/disabled status
 * Parameters:
 *	    unit   - (IN)  BCM device number
 *   	port   - (IN)  The port to check
 *	    enable - (OUT) Place to store boolean return value for on/off
 * Returns:
 *	    BCM_E_XXX
 */
int
bcm_esw_mirror_ingress_get(int unit, bcm_port_t port, int *enable)
{
    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Input parameters check. */
    if (NULL == enable) {
        return (BCM_E_PARAM);
    }

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
    }

    if (!SOC_PORT_VALID(unit, port)) {
        return (BCM_E_PORT);
    }

    if (IS_CPU_PORT(unit, port) &&
        !soc_feature(unit, soc_feature_cpuport_mirror)) {
        return (BCM_E_PORT);
    }

    /* Get ingress mirroring enable from  port table. */
    return _bcm_port_mirror_enable_get(unit, port, enable);
}

/*
 * Function:
 * 	   bcm_esw_mirror_egress_set
 * Description:
 *  	Enable or disable mirroring per egress
 * Parameters:
 *  	unit   - (IN) BCM device number
 *	    port   - (IN) The port to affect
 *	    enable - (IN) Boolean value for on/off
 * Returns:
 *	    BCM_E_XXX
 * Notes:
 *  	Mirroring must also be globally enabled.
 */
int
bcm_esw_mirror_egress_set(int unit, bcm_port_t port, int enable)
{

    int rv;

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
    }

    /* Input parameters check. */
    if (!SOC_PORT_VALID(unit, port)) {
        return BCM_E_PORT;
    }

    if (IS_CPU_PORT(unit, port) &&
        !soc_feature(unit, soc_feature_cpuport_mirror)) {
        return BCM_E_PORT;
    }

    MIRROR_LOCK(unit);

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        /* Enable third MTP for egress since this is for single MTP mode */
        if (MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
            /* Update MTP_MODE */
            MIRROR_CONFIG_MTP_MODE_BMP(unit) |=
                BCM_MIRROR_MTP_FLEX_EGRESS_D15;

            BCM_IF_ERROR_RETURN(_bcm_tr2_mirror_mtp_slot_update(unit));
        } else {
            MIRROR_CONFIG_SHARED_MTP(unit,
                      BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX).egress = TRUE;
        }
        rv = _bcm_esw_mirror_egress_set(unit, port, 
                     (enable) ? BCM_MIRROR_MTP_FLEX_EGRESS_D15 : (0));
    } else 
#endif /* BCM_TRIUMPH2_SUPPORT */
    {
        rv = _bcm_esw_mirror_egress_set(unit, port, 
                                        (enable) ?  BCM_MIRROR_MTP_ONE : (0));
    }

    MIRROR_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *	    bcm_esw_mirror_egress_get
 * Description:
 * 	    Get the mirroring per egress enabled/disabled status
 * Parameters:
 *  	unit -   (IN)  BCM device number
 *  	port -   (IN)  The port to check
 *  	enable - (OUT) Place to store boolean return value for on/off
 * Returns:
 *  	BCM_E_XXX
 */
int
bcm_esw_mirror_egress_get(int unit, bcm_port_t port, int *enable)
{
    int rv; 

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Input parameters check. */
    if (NULL == enable) {
        return (BCM_E_PARAM);
    }

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
    }

    if (!SOC_PORT_VALID(unit, port)) {
        return (BCM_E_PORT);
    }

    if (IS_CPU_PORT(unit, port) &&
        !soc_feature(unit, soc_feature_cpuport_mirror)) {
        return (BCM_E_PORT);
    }

    MIRROR_LOCK(unit); 
    rv = _bcm_esw_mirror_egress_get(unit, port, enable);
    MIRROR_UNLOCK(unit);
    *enable = *enable ? 1 : 0;
    return (rv);
}

/*
 * Function:
 *      bcm_esw_mirror_to_pbmp_set
 * Description:
 *  	Set the mirror-to port bitmap for mirroring on a given port.
 * Parameters:
 *  	unit - (IN) BCM device number
 *  	port - (IN) The port to affect
 *      pbmp - (IN) The port bitmap of mirrored to ports for this port.
 * Returns:
 *	BCM_E_XXX
 * Notes:
 *	This API interface is only supported on XGS fabric devices and
 *      production versions of XGS3 switch devices. For XGS3 devices
 *      this function is normally only used when the XGS3 device is
 *      stacked in a ring configuration with BCM567x fabric devices.
 */
int
bcm_esw_mirror_to_pbmp_set(int unit, bcm_port_t port, pbmp_t pbmp)
{
    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
    }
    /* Input parameters check. */
    if (!SOC_PORT_VALID(unit, port) || !IS_PORT(unit, port)) {
        return BCM_E_PORT;
    }

#if defined(BCM_HERCULES_SUPPORT)
    if (SOC_IS_HERCULES(unit)) {
        MIRROR_CONFIG_MODE(unit) = BCM_MIRROR_L2;
        return WRITE_ING_MIRTOBMAPr(unit, port, SOC_PBMP_WORD_GET(pbmp, 0));
    }
#endif /* BCM_HERCULES_SUPPORT */
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (soc_feature(unit, soc_feature_egr_mirror_path)) {
        int mport;

        /* Both ingress and egress ports must be stack ports */
        if (!IS_ST_PORT(unit, port)) {
            return (BCM_E_PORT);
        }
        
        PBMP_ITER(pbmp, mport) {
            if (!IS_ST_PORT(unit, mport)) {
                return (BCM_E_PORT);
            }
        }

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        imirror_bitmap_entry_t  entry;
        int egr_port;

        sal_memset(&entry, 0, sizeof(entry));

        PBMP_ITER(pbmp, egr_port) {
            soc_mem_field32_set(unit, IMIRROR_BITMAPm, &entry,
                                EGRESS_PORTf, egr_port);
            soc_mem_field32_set(unit, IMIRROR_BITMAPm, &entry, ENABLEf,
                                1);
            break;
        }
        BCM_IF_ERROR_RETURN
            (soc_mem_write(unit, IMIRROR_BITMAPm, MEM_BLOCK_ANY, port,
                           &entry));
        if (IS_CPU_PORT(unit, port) && SOC_INFO(unit).cpu_hg_index != -1) {
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, IMIRROR_BITMAPm, MEM_BLOCK_ANY,
                               SOC_INFO(unit).cpu_hg_index, &entry));
        }
        return BCM_E_NONE;
    }
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
        if (soc_feature(unit, soc_feature_mirror_control_mem)) {
            imirror_bitmap_entry_t entry;
            soc_mem_pbmp_field_set(unit, IMIRROR_BITMAPm, &entry, BITMAPf,
                                   &pbmp);
            return (WRITE_IMIRROR_BITMAPm(unit, MEM_BLOCK_ANY, port, &entry));
        } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
        if (SOC_REG_IS_VALID(unit, IMIRROR_BITMAP_64r)) {
            uint32 values[2];
            soc_field_t fields[] = {BITMAP_LOf, BITMAP_HIf};

            values[0] = SOC_PBMP_WORD_GET(pbmp, 0);
            values[1] = SOC_PBMP_WORD_GET(pbmp, 1);
            return soc_reg_fields32_modify(unit, IMIRROR_BITMAP_64r, port,
                                           2, fields, values);
        } else 
#endif /* BCM_TRIUMPH2_SUPPORT */
        {
            uint32 mirbmap;

            mirbmap = SOC_PBMP_WORD_GET(pbmp, 0);
            if (SOC_IS_FBX(unit)) {
                mirbmap >>= SOC_HG_OFFSET(unit);
            }
            return WRITE_IMIRROR_BITMAPr(unit, port, mirbmap);
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return (BCM_E_UNAVAIL);
}

/*
 * Function:
 *   	bcm_esw_mirror_to_pbmp_get
 * Description:
 *	    Get the mirror-to port bitmap for mirroring on the
 *	    specified port.
 * Parameters:
 *	    unit - (IN) BCM device number
 *	    port - (IN) The port to mirror all ingress/egress selections to
 *      pbmp - (OUT) The port bitmap of mirror-to ports for this port.
 * Returns:
 *	    BCM_E_XXX
 */
int
bcm_esw_mirror_to_pbmp_get(int unit, bcm_port_t port, pbmp_t *pbmp)
{

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
    }

    /* Input parameters check. */
    if (!SOC_PORT_VALID(unit, port) || !IS_PORT(unit, port)) {
        return BCM_E_PORT;
    }

#if defined(BCM_HERCULES_SUPPORT)
    if (SOC_IS_HERCULES(unit)) {
        int rv;
        uint32 mirbmap;

        rv = READ_ING_MIRTOBMAPr(unit, port, &mirbmap);
        SOC_PBMP_CLEAR(*pbmp);
        SOC_PBMP_WORD_SET(*pbmp, 0, mirbmap);
        return rv;
    }
#endif /* BCM_HERCULES_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (soc_feature(unit, soc_feature_egr_mirror_path)) {
        int rv;

#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TD2_TT2(unit)) {
            imirror_bitmap_entry_t entry;
            int egr_port, tgid;
            int member_count;
            bcm_trunk_member_t *member_array = NULL;
            bcm_module_t mod_out;
            bcm_port_t port_out;
            bcm_trunk_t tgid_out;
            int id_out;
            int idx;
            int is_local_modid;

            BCM_IF_ERROR_RETURN
                (READ_IMIRROR_BITMAPm(unit, MEM_BLOCK_ANY, port, &entry));
            BCM_PBMP_CLEAR(*pbmp);
            if (!soc_mem_field32_get(unit, IMIRROR_BITMAPm, &entry, ENABLEf)) {
                return BCM_E_NONE;
            }
            if (soc_mem_field32_get(unit, IMIRROR_BITMAPm, &entry, ISTRUNKf)) {
                tgid = soc_mem_field32_get(unit, IMIRROR_BITMAPm, &entry,
                                           HG_TRUNK_IDf);

                /* Get trunk member port/module pairs. */
                BCM_IF_ERROR_RETURN
                    (bcm_esw_trunk_get(unit, tgid, NULL, 0, NULL,
                                       &member_count));
                if (member_count > 0) {
                    member_array =
                        sal_alloc(sizeof(bcm_trunk_member_t) * member_count,
                                  "trunk member array");
                    if (NULL == member_array) {
                        return BCM_E_MEMORY;
                    }
                    rv = bcm_esw_trunk_get(unit, tgid, NULL, member_count,
                                           member_array, &member_count);
                    if (BCM_FAILURE(rv)) {
                        sal_free(member_array);
                        return rv;
                    }
                }

                /* Fill pbmp with trunk members from other modules . */
                for (idx = 0; idx < member_count; idx++) {
                    rv = _bcm_esw_gport_resolve(unit, member_array[idx].gport,
                                                &mod_out, &port_out,
                                                &tgid_out, &id_out);
                    if (BCM_FAILURE(rv) || -1 != tgid_out || -1 != id_out) {
                        sal_free(member_array);
                        return rv;
                    }
                    rv = _bcm_esw_modid_is_local(unit, mod_out,
                                                 &is_local_modid);
                    if (BCM_FAILURE(rv)) {
                        sal_free(member_array);
                        return rv;
                    }
                    if (!is_local_modid) {
                        rv = bcm_esw_stk_modport_get(unit, mod_out, &egr_port);
                        if (BCM_FAILURE(rv)) {
                            sal_free(member_array);
                            return rv;
                        }
                        BCM_PBMP_PORT_ADD(*pbmp, egr_port);
                    }
                }
                sal_free(member_array);
            } else {
                egr_port = soc_mem_field32_get(unit, IMIRROR_BITMAPm, &entry,
                                                EGRESS_PORTf);
                BCM_PBMP_PORT_SET(*pbmp, egr_port); 
            }
            return BCM_E_NONE;
        }
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
        if (soc_feature(unit, soc_feature_mirror_control_mem)) {
            imirror_bitmap_entry_t entry;
            BCM_IF_ERROR_RETURN
                (READ_IMIRROR_BITMAPm(unit, MEM_BLOCK_ANY, port, &entry));
            soc_mem_pbmp_field_get(unit, IMIRROR_BITMAPm, &entry, BITMAPf,
                                   pbmp);
            rv = BCM_E_NONE;
        } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
        if (SOC_REG_IS_VALID(unit, IMIRROR_BITMAP_64r)) {
            uint64 mirbmap64;
            rv = READ_IMIRROR_BITMAP_64r(unit, port, &mirbmap64);
            SOC_PBMP_CLEAR(*pbmp);
            SOC_PBMP_WORD_SET(*pbmp, 0,
                soc_reg64_field32_get(unit, IMIRROR_BITMAP_64r,
                                      mirbmap64, BITMAP_LOf));
            SOC_PBMP_WORD_SET(*pbmp, 1,
                soc_reg64_field32_get(unit, IMIRROR_BITMAP_64r,
                                      mirbmap64, BITMAP_HIf));
        } else 
#endif /* BCM_TRIUMPH2_SUPPORT */
        {
            uint32 mirbmap;

            rv = READ_IMIRROR_BITMAPr(unit, port, &mirbmap);
            if (SOC_IS_FBX(unit)) {
                mirbmap <<= SOC_HG_OFFSET(unit);
            }
            SOC_PBMP_CLEAR(*pbmp);
            SOC_PBMP_WORD_SET(*pbmp, 0, mirbmap);
        }
        return rv;
    }
#endif
    return (BCM_E_UNAVAIL);
}

#ifdef BCM_TRIDENT_SUPPORT
/*
 * Function:
 *      _bcm_trident_mirror_vlan_set
 * Description:
 *      Set VLAN for egressing mirrored packets on a port (RSPAN)
 *      This will support the legacy mode where RSPAN is a per-egress-port
 *      property.
 * Parameters:
 *      unit    - (IN) Bcm device number.
 *      port    - (IN) Mirror-to port to set (-1 for all ports).
 *      tpid    - (IN) Tag protocol id (0 to disable).
 *      vlan    - (IN) Virtual lan number (0 to disable).
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_trident_mirror_vlan_set(int unit, bcm_port_t port,
                             uint16 tpid, uint16 vlan)
{
    int rv = BCM_E_NONE;
    uint32 profile_index, old_profile_index;
    uint32 hw_buffer;
    egr_mirror_encap_control_entry_t control_entry;
    egr_mirror_encap_data_1_entry_t data_1_entry;
    egr_mirror_encap_data_2_entry_t data_2_entry;
    egr_port_entry_t egr_port_entry;
    void *entries[EGR_MIRROR_ENCAP_ENTRIES_NUM];

    hw_buffer = (uint32)(tpid << 16) | vlan;

    if (0 != hw_buffer) {
        sal_memset(&control_entry, 0, sizeof(control_entry));
        sal_memset(&data_1_entry, 0, sizeof(data_1_entry));
        sal_memset(&data_2_entry, 0, sizeof(data_2_entry));

        entries[0] = &control_entry;
        entries[1] = &data_1_entry;
        entries[2] = &data_2_entry;

        /* Setup Mirror Control Memory */
        soc_EGR_MIRROR_ENCAP_CONTROLm_field32_set(unit, &control_entry,
                             ENTRY_TYPEf, BCM_TD_MIRROR_ENCAP_TYPE_RSPAN);

        soc_EGR_MIRROR_ENCAP_CONTROLm_field32_set(unit, &control_entry,
                             RSPAN__ADD_OPTIONAL_HEADERf,
                                                  BCM_TD_MIRROR_HEADER_ONLY);

        /* Setup Mirror Data 1 Memory */
        soc_EGR_MIRROR_ENCAP_DATA_1m_field32_set(unit, &data_1_entry,
                             RSPAN__RSPAN_VLAN_TAGf, hw_buffer);

        rv = _bcm_egr_mirror_encap_entry_add(unit, entries, &profile_index);

        if (BCM_SUCCESS(rv)) {
            /* Remove the previous profile index if any */
            rv = READ_EGR_PORTm(unit, MEM_BLOCK_ANY, port, &egr_port_entry);

            old_profile_index = -1;
            if (BCM_SUCCESS(rv) &&
                (0 != soc_EGR_PORTm_field32_get(unit, &egr_port_entry,
                                                MIRROR_ENCAP_ENABLEf))) {
                old_profile_index =
                    soc_EGR_PORTm_field32_get(unit, &egr_port_entry,
                                              MIRROR_ENCAP_INDEXf);
            }

            /* Supply the correct profile index to the egress port */
            soc_EGR_PORTm_field32_set(unit, &egr_port_entry,
                                      MIRROR_ENCAP_ENABLEf, 1);
            soc_EGR_PORTm_field32_set(unit, &egr_port_entry,
                                      MIRROR_ENCAP_INDEXf, profile_index);
            rv = WRITE_EGR_PORTm(unit, MEM_BLOCK_ANY, port, &egr_port_entry);

            if (BCM_SUCCESS(rv) && (-1 != old_profile_index)) {
                rv = _bcm_egr_mirror_encap_entry_delete(unit,
                                                        old_profile_index);
            }
        }
    } else {
        rv = READ_EGR_PORTm(unit, MEM_BLOCK_ANY, port, &egr_port_entry);

        if (BCM_SUCCESS(rv) &&
            (0 != soc_EGR_PORTm_field32_get(unit, &egr_port_entry,
                                            MIRROR_ENCAP_ENABLEf))) {
            old_profile_index =
                soc_EGR_PORTm_field32_get(unit, &egr_port_entry,
                                          MIRROR_ENCAP_INDEXf);
            soc_EGR_PORTm_field32_set(unit, &egr_port_entry,
                                      MIRROR_ENCAP_ENABLEf, 0);
            soc_EGR_PORTm_field32_set(unit, &egr_port_entry,
                                      MIRROR_ENCAP_INDEXf, 0);
            rv = WRITE_EGR_PORTm(unit, MEM_BLOCK_ANY, port, &egr_port_entry);

            if (BCM_SUCCESS(rv)) {
                rv = _bcm_egr_mirror_encap_entry_delete(unit,
                                                        old_profile_index);
            }
        } /* Else do nothing */
    }

    return rv;
}

/*
 * Function:
 *      _bcm_trident_mirror_vlan_get
 * Description:
 *      Get VLAN for egressing mirrored packets on a port (RSPAN)
 *      This will support the legacy mode where RSPAN is a per-egress-port
 *      property.
 * Parameters:
 *      unit    - (IN) Bcm device number.
 *      port    - (IN) Mirror-to port to set (-1 for all ports).
 *      tpid    - (OUT) Tag protocol id (0 to disable).
 *      vlan    - (OUT) Virtual lan number (0 to disable).
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_trident_mirror_vlan_get(int unit, bcm_port_t port,
                             uint16 *tpid, uint16 *vlan)
{
    uint32 profile_index;
    uint32 hw_buffer;
    egr_mirror_encap_control_entry_t control_entry;
    egr_mirror_encap_data_1_entry_t data_1_entry;
    egr_mirror_encap_data_2_entry_t data_2_entry;
    egr_port_entry_t egr_port_entry;
    void *entries[EGR_MIRROR_ENCAP_ENTRIES_NUM];

    BCM_IF_ERROR_RETURN
        (READ_EGR_PORTm(unit, MEM_BLOCK_ANY, port, &egr_port_entry));

    if (0 == soc_EGR_PORTm_field32_get(unit, &egr_port_entry,
                                       MIRROR_ENCAP_ENABLEf)) {
        return BCM_E_NOT_FOUND;
    }

    profile_index =
        soc_EGR_PORTm_field32_get(unit, &egr_port_entry,
                                  MIRROR_ENCAP_INDEXf);
    entries[0] = &control_entry;
    entries[1] = &data_1_entry;
    entries[2] = &data_2_entry;
    BCM_IF_ERROR_RETURN
        (soc_profile_mem_get(unit, EGR_MIRROR_ENCAP(unit),
                             profile_index, 1, entries));


    if (soc_EGR_MIRROR_ENCAP_CONTROLm_field32_get(unit, &control_entry,
                       ENTRY_TYPEf) != BCM_TD_MIRROR_ENCAP_TYPE_RSPAN) {
        return BCM_E_CONFIG;
    }

    /* RSPAN recovery */
    hw_buffer =
        soc_EGR_MIRROR_ENCAP_DATA_1m_field32_get(unit, &data_1_entry,
                                                 RSPAN__RSPAN_VLAN_TAGf);

    *vlan = (bcm_vlan_t) (hw_buffer & 0xffff);
    *tpid = (uint16) ((hw_buffer >> 16) & 0xffff);
 
    return BCM_E_NONE;
}
#endif /* BCM_TRIDENT_SUPPORT */

/*
 * Function:
 *      bcm_esw_mirror_vlan_set
 * Description:
 *      Set VLAN for egressing mirrored packets on a port (RSPAN)
 * Parameters:
 *      unit    - (IN) Bcm device number.
 *      port    - (IN) Mirror-to port to set (-1 for all ports).
 *      tpid    - (IN) Tag protocol id (0 to disable).
 *      vlan    - (IN) Virtual lan number (0 to disable).
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_mirror_vlan_set(int unit, bcm_port_t port,
                        uint16 tpid, uint16 vlan)
{
    int rv;

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Vlan id range check. */ 
    if (!BCM_VLAN_VALID(vlan) && vlan != BCM_VLAN_NONE) {
        return (BCM_E_PARAM);
    } 


    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
    }

    if(!SOC_PORT_VALID(unit, port)) {
        return (BCM_E_PORT);
    }

#ifdef BCM_TRIDENT_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
        rv = _bcm_trident_mirror_vlan_set(unit, port, tpid, vlan);
    } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FBX(unit)) {
        rv = WRITE_EGR_RSPAN_VLAN_TAGr(unit, port, (tpid << 16) | vlan);
    } else 
#endif /* BCM_FIREBOLT_SUPPORT */
    {
        rv = BCM_E_UNAVAIL;
    }
    return (rv);
}

/*
 * Function:
 *      bcm_esw_mirror_vlan_get
 * Description:
 *      Get VLAN for egressing mirrored packets on a port (RSPAN)
 * Parameters:
 *      unit    - (IN) BCM device number
 *      port    - (IN) Mirror-to port for which to get tag info
 *      tpid    - (OUT) tag protocol id
 *      vlan    - (OUT) virtual lan number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_mirror_vlan_get(int unit, bcm_port_t port,
                        uint16 *tpid, uint16 *vlan)
{
    int rv;

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Input parameters check. */
    if ((NULL == tpid) || (NULL == vlan)) {
        return (BCM_E_PARAM);
    }

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
    }

    if(!SOC_PORT_VALID(unit, port)) {
        return (BCM_E_PORT);
    }

#ifdef BCM_TRIDENT_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
        rv = _bcm_trident_mirror_vlan_get(unit, port, tpid, vlan);
    } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FBX(unit)) {
        uint32 rspan;

        BCM_IF_ERROR_RETURN(READ_EGR_RSPAN_VLAN_TAGr(unit, port, &rspan));
        *tpid = (rspan >> 16);
        *vlan = (rspan & 0xFFF);

        rv = BCM_E_NONE;
    } else 
#endif /* BCM_FIREBOLT_SUPPORT */
    {
        rv = BCM_E_UNAVAIL;
    }
    return (rv);
}

/*
 * Function:
 *      bcm_esw_mirror_egress_path_set
 * Description:
 *      Set egress mirror packet path for stack ring
 * Parameters:
 *      unit    - (IN) BCM device number
 *      modid   - (IN) Destination module ID (of mirror-to port)
 *      port    - (IN) Stack port for egress mirror packet
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      This function should only be used for XGS3 devices stacked
 *      in a ring configuration with fabric devices that may block
 *      egress mirror packets when the mirror-to port is on a 
 *      different device than the egress port being mirrored.
 *      Currently the only such fabric device is BCM5675 rev A0.
 */
int
bcm_esw_mirror_egress_path_set(int unit, bcm_module_t modid, bcm_port_t port)
{
    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Input validation */
    if (!SOC_MODID_ADDRESSABLE(unit, modid)) {
        return BCM_E_BADID;
    }

    if (BCM_GPORT_IS_SET(port)) {
        bcm_module_t    tmp_modid;
        int             isLocal;

        BCM_IF_ERROR_RETURN(
            _bcm_mirror_gport_resolve(unit, port, &port, &tmp_modid));
        BCM_IF_ERROR_RETURN(_bcm_esw_modid_is_local(unit, tmp_modid, &isLocal));

        if (TRUE != isLocal) {
            return BCM_E_PORT;
        }

    } else {
        /* Actuall physical port passed */
        if (!SOC_PORT_VALID(unit, port) || 
            !SOC_PORT_ADDRESSABLE(unit, port)) {
            return BCM_E_PORT;
        }
    }

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) { 
        return bcm_xgs3_mirror_egress_path_set(unit, modid, port);
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    return (BCM_E_UNAVAIL);
}

/*
 * Function:
 *      bcm_esw_mirror_egress_path_get
 * Description:
 *      Get egress mirror packet path for stack ring
 * Parameters:
 *      unit    - (IN) BCM device number
 *      modid   - (IN) Destination module ID (of mirror-to port)
 *      port    - (OUT)Stack port for egress mirror packet
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      See bcm_mirror_alt_egress_pbmp_set for more details.
 */
int
bcm_esw_mirror_egress_path_get(int unit, bcm_module_t modid, bcm_port_t *port)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    int             rv, isGport;
    bcm_module_t    mod_out;
    bcm_port_t      port_out;
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Input parameters check. */
    if (NULL == port) {
        return (BCM_E_PARAM);
    }

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) { 
        rv = bcm_xgs3_mirror_egress_path_get(unit, modid, port);

        if (BCM_FAILURE(rv)) {
            return rv;
        }
        BCM_IF_ERROR_RETURN(
            bcm_esw_switch_control_get(unit, bcmSwitchUseGport, &isGport));
        if (isGport) {
            BCM_IF_ERROR_RETURN(
                _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET, modid, *port, 
                                       &mod_out, &port_out));
            BCM_IF_ERROR_RETURN(
                _bcm_mirror_gport_construct(unit, port_out, mod_out, 0, port)); 
        } else {
            BCM_IF_ERROR_RETURN(
                _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_SET, modid, *port, 
                                       &mod_out, port));
        }

        return (BCM_E_NONE);
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    return (BCM_E_UNAVAIL);
}


/*
 * Function:
 *      bcm_esw_mirror_port_set
 * Description:
 *      Set mirroring configuration for a port
 * Parameters:
 *      unit      - BCM device number
 *      port      - port to configure
 *      dest_mod  - module id of mirror-to port
 *                  (-1 for local port)
 *      dest_port - mirror-to port ( can be gport or mirror_gport)
 *      flags     - BCM_MIRROR_PORT_* flags
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Setting BCM_MIRROR_PORT_ENABLE without setting _INGRESS or
 *      _EGRESS allows the port to participate in bcm_l2_cache matches
 *      with the BCM_L2_CACHE_MIRROR bit set, and to participate in
 *      bcm_field lookups with the mirror action set.
 *
 *      If bcmSwitchDirectedMirroring is disabled for the unit and
 *      dest_mod is non-negative, then the dest_mod path is looked
 *      up using bcm_topo_port_get.
 *      If bcmSwitchDirectedMirroring is enabled for the unit and
 *      dest_mod is negative, then the local unit's modid is used
 *      as the dest_mod.
 */
int
bcm_esw_mirror_port_set(int unit, bcm_port_t port,
                        bcm_module_t dest_mod, bcm_port_t dest_port,
                        uint32 flags)
{
    int         rv;                        /* Operation return status.        */
    bcm_mirror_destination_t mirror_dest;  /* Mirror destination.             */
    uint32                   destroy_flag = FALSE; /* mirror dest destroy     */

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }
    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
    }

    /* If mirroring is completely disabled, remove all mirror destinations */
    if (flags == 0 && dest_mod == -1 && dest_port == -1) {
        flags = BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_EGRESS |
            BCM_MIRROR_PORT_EGRESS_TRUE;
        return bcm_esw_mirror_port_dest_delete_all(unit, port, flags);
    }

    /* Create traditional mirror destination. */
    bcm_mirror_destination_t_init(&mirror_dest);

    MIRROR_LOCK(unit);
    if (BCM_GPORT_IS_MIRROR(dest_port)) {
        rv = bcm_esw_mirror_destination_get(unit, dest_port, &mirror_dest);
    } else {
        rv = _bcm_esw_mirror_destination_find(unit, dest_port, dest_mod, flags, &mirror_dest); 
        if (BCM_E_NOT_FOUND == rv) {
            if ((flags & (BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_EGRESS)) |
                (soc_feature(unit, soc_feature_egr_mirror_true) &&
                 (flags & BCM_MIRROR_PORT_EGRESS_TRUE))) {
                rv = _bcm_esw_mirror_destination_create(unit, &mirror_dest);
                destroy_flag = TRUE;
            } else {
                MIRROR_UNLOCK(unit); 
                return (BCM_E_NONE);
            }
        }       
    }
    if (BCM_FAILURE(rv)) {
        MIRROR_UNLOCK(unit);
        return rv;
    }

    /* Enable/Disable ingress mirroring. */
    if (flags & BCM_MIRROR_PORT_INGRESS) {
        rv = bcm_esw_mirror_port_dest_add(unit, port, BCM_MIRROR_PORT_INGRESS,
                                          mirror_dest.mirror_dest_id);
        if (BCM_E_EXISTS == rv) {
            /* Since this function is set, not add, it is sufficient if
             * the destination already exists.  Clear the error. */
            rv = BCM_E_NONE;
        }
    } else {
        rv = bcm_esw_mirror_port_dest_delete(unit, port, BCM_MIRROR_PORT_INGRESS,
                                             mirror_dest.mirror_dest_id); 
        if (BCM_E_NOT_FOUND == rv) {
            /* There is no clean way to identify delete. -> 
               if destination is not found assume success. */ 
            rv = BCM_E_NONE;
        } 
    }

    if (BCM_FAILURE(rv)) {
        /* Delete unused mirror destination. */
        if (destroy_flag) {
            (void)bcm_esw_mirror_destination_destroy(unit, mirror_dest.mirror_dest_id);
        }
        MIRROR_UNLOCK(unit); 
        return (rv);
    }

    /* Enable/Disable egress mirroring. */
    if (flags & BCM_MIRROR_PORT_EGRESS) {
        rv = bcm_esw_mirror_port_dest_add(unit, port, BCM_MIRROR_PORT_EGRESS,
                                          mirror_dest.mirror_dest_id); 
        if (BCM_E_EXISTS == rv) {
            /* Since this function is set, not add, it is sufficient if
             * the destination already exists.  Clear the error. */
            rv = BCM_E_NONE;
        }
    } else {
        rv = bcm_esw_mirror_port_dest_delete(unit, port, BCM_MIRROR_PORT_EGRESS,
                                             mirror_dest.mirror_dest_id); 
        if (BCM_E_NOT_FOUND == rv) {
            /* There is no clean way to identify delete. -> 
               if destination is not found assume success. */ 
            rv = BCM_E_NONE;
        } 
    }

    if (BCM_FAILURE(rv)) {
        /* Delete unused mirror destination. */
        if (destroy_flag) {
            (void)bcm_esw_mirror_destination_destroy(unit, mirror_dest.mirror_dest_id);
        }
        MIRROR_UNLOCK(unit); 
        return (rv);
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_egr_mirror_true)) {
        /* Enable/Disable egress true mirroring. */
        if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
            rv = bcm_esw_mirror_port_dest_add(unit, port,
                                              BCM_MIRROR_PORT_EGRESS_TRUE,
                                              mirror_dest.mirror_dest_id); 
            if (BCM_E_EXISTS == rv) {
                /* Since this function is set, not add, it is sufficient if
                 * the destination already exists.  Clear the error. */
                rv = BCM_E_NONE;
            }
        } else {
            rv = bcm_esw_mirror_port_dest_delete(unit, port,
                                                 BCM_MIRROR_PORT_EGRESS_TRUE,
                                                 mirror_dest.mirror_dest_id); 
            if (BCM_E_NOT_FOUND == rv) {
                /* There is no clean way to identify delete. -> 
                   if destination is not found assume success. */ 
                rv = BCM_E_NONE;
            } 
        }

        if (BCM_FAILURE(rv)) {
            /* Delete unused mirror destination. */
            if (destroy_flag) {
                (void)bcm_esw_mirror_destination_destroy(unit,
                                                 mirror_dest.mirror_dest_id);
            }
            MIRROR_UNLOCK(unit); 
            return (rv);
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */  

    /* Delete unused mirror destination. */
    if (1 >= MIRROR_DEST_REF_COUNT(unit, mirror_dest.mirror_dest_id)) {
        rv = bcm_esw_mirror_destination_destroy(unit, mirror_dest.mirror_dest_id);
    }

    MIRROR_UNLOCK(unit); 
    return (rv);
}
/*
 * Function:
 *      bcm_esw_mirror_port_get
 * Description:
 *      Get mirroring configuration for a port
 * Parameters:
 *      unit      - BCM device number
 *      port      - port to get configuration for
 *      dest_mod  - (OUT) module id of mirror-to port
 *      dest_port - (OUT) mirror-to port
 *      flags     - (OUT) BCM_MIRROR_PORT_* flags
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_mirror_port_get(int unit, bcm_port_t port,
                        bcm_module_t *dest_mod, bcm_port_t *dest_port,
                        uint32 *flags)
{
    bcm_gport_t mirror_dest_id;               /* Mirror destination  id.   */
    int enable;                               /* Egress mirror is enabled. */
    int rv;                                   /* Operation return status.  */
    int mirror_dest_count = 0;                /* Mirror destination found. */
    int isGport;                              /* gport indicator */  
    bcm_mirror_destination_t    mirror_dest;  /* mirror destination struct */       
    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Input parameters check. */
    if ((NULL == flags) || (NULL == dest_mod) || (NULL == dest_port)) {
        return (BCM_E_PARAM);
    }

    bcm_mirror_destination_t_init(&mirror_dest);

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
    }

    *flags = 0;

    /* Check if directed mirroring is enabled and gport required. */
    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit, bcmSwitchUseGport, 
                                                   &isGport));
    
    MIRROR_LOCK(unit);

    /* Read port ingress mirroring destination ports. */
    rv = bcm_esw_mirror_port_dest_get(unit, port, BCM_MIRROR_PORT_INGRESS, 
                                      1, &mirror_dest_id,
                                      &mirror_dest_count);
    if (BCM_FAILURE(rv)) {
        MIRROR_UNLOCK(unit);
        return (rv);
    }

    if (mirror_dest_count) {
        rv = bcm_esw_mirror_destination_get(unit, mirror_dest_id,
                                            &mirror_dest);

        if (BCM_FAILURE(rv)) {
            MIRROR_UNLOCK(unit);
            return (rv);
        }
        *flags |= BCM_MIRROR_PORT_INGRESS;

        if (MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
            /* Read mtp egress enable bitmap for source port. */
            rv = _bcm_esw_mirror_egress_get(unit, port, &enable);
            if (BCM_FAILURE(rv)) {
                MIRROR_UNLOCK(unit);
                return (rv);
            }
            if (enable) {
                *flags |= BCM_MIRROR_PORT_EGRESS;
            }
        }
        MIRROR_UNLOCK(unit);

        if (isGport) {
            *dest_port = mirror_dest.gport;
        } else if (BCM_GPORT_IS_TRUNK(mirror_dest.gport)) {
            *flags |= BCM_MIRROR_PORT_DEST_TRUNK;
            *dest_port = BCM_GPORT_TRUNK_GET(mirror_dest.gport);
        } else {
            BCM_IF_ERROR_RETURN(
                _bcm_mirror_gport_resolve(unit, mirror_dest.gport,
                                          dest_port, dest_mod));
            BCM_IF_ERROR_RETURN(
                _bcm_gport_modport_hw2api_map(unit, *dest_mod, *dest_port, 
                                              dest_mod, dest_port));
        }

        return (BCM_E_NONE);
    }

    /* Read port egress mirroring destination ports. */
    rv = bcm_esw_mirror_port_dest_get(unit, port, BCM_MIRROR_PORT_EGRESS, 
                                      1, &mirror_dest_id,
                                      &mirror_dest_count);

    if (BCM_FAILURE(rv)) {
        MIRROR_UNLOCK(unit);
        return (rv);
    }

    if (mirror_dest_count) {
        rv = bcm_esw_mirror_destination_get(unit, mirror_dest_id,
                                            &mirror_dest);

        if (BCM_FAILURE(rv)) {
            MIRROR_UNLOCK(unit);
            return (rv);
        }
        *flags |= BCM_MIRROR_PORT_EGRESS;

        MIRROR_UNLOCK(unit);

        if (isGport) {
            *dest_port = mirror_dest.gport;
        } else if (BCM_GPORT_IS_TRUNK(mirror_dest.gport)) {
            *flags |= BCM_MIRROR_PORT_DEST_TRUNK;
            *dest_port = BCM_GPORT_TRUNK_GET(mirror_dest.gport);
        } else {
            BCM_IF_ERROR_RETURN(
                _bcm_mirror_gport_resolve(unit, mirror_dest.gport,
                                          dest_port, dest_mod));
            BCM_IF_ERROR_RETURN(
                _bcm_gport_modport_hw2api_map(unit, *dest_mod, *dest_port, 
                                              dest_mod, dest_port));
        }

        return (BCM_E_NONE);
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_egr_mirror_true)) {
        /* Read port ingress mirroring destination ports. */
        rv = bcm_esw_mirror_port_dest_get(unit, port,
                                          BCM_MIRROR_PORT_EGRESS_TRUE, 
                                          1, &mirror_dest_id,
                                          &mirror_dest_count);

        if (BCM_FAILURE(rv)) {
            MIRROR_UNLOCK(unit);
            return (rv);
        }

        if (mirror_dest_count) {
            rv = bcm_esw_mirror_destination_get(unit, mirror_dest_id,
                                                &mirror_dest);

            if (BCM_FAILURE(rv)) {
                MIRROR_UNLOCK(unit);
                return (rv);
            }
            *flags |= BCM_MIRROR_PORT_EGRESS_TRUE;

            MIRROR_UNLOCK(unit);

            if (isGport) {
                *dest_port = mirror_dest.gport;
            } else if (BCM_GPORT_IS_TRUNK(mirror_dest.gport)) {
                *flags |= BCM_MIRROR_PORT_DEST_TRUNK;
                *dest_port = BCM_GPORT_TRUNK_GET(mirror_dest.gport);
            } else {
                BCM_IF_ERROR_RETURN
                    (_bcm_mirror_gport_resolve(unit, mirror_dest.gport,
                                               dest_port, dest_mod));
                BCM_IF_ERROR_RETURN
                    (_bcm_gport_modport_hw2api_map(unit, *dest_mod, *dest_port, 
                                                   dest_mod, dest_port));
            }

            return (BCM_E_NONE);
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */  

    MIRROR_UNLOCK(unit);

    return (BCM_E_NONE);
}


/*
 * Function:
 *     bcm_esw_mirror_port_dest_add 
 * Purpose:
 *      Add mirroring destination to a port. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Port mirrored port.
 *      flags        -  (IN) BCM_MIRROR_PORT_XXX flags.
 *      mirror_dest  -  (IN) Mirroring destination gport. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_esw_mirror_port_dest_add(int unit, bcm_port_t port, 
                              uint32 flags, bcm_gport_t mirror_dest) 
{
    int          rv = BCM_E_NONE;   /* Operation return status.      */
    int          orig_gport; 
    int          vp = VP_PORT_INVALID;
    int          vp_mirror = FALSE;
    bcm_gport_t  mirror_dest_list[BCM_MIRROR_MTP_COUNT];
    int          mirror_dest_count, mtp;

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }
    orig_gport = port;
    /* Input parameters check. */

    if (-1 != port) { 
        if (BCM_GPORT_IS_SET(port)) {

            rv = _bcm_tr2_mirror_vp_port_get(unit, port,
                                             &vp, &port);
            if ((rv != BCM_E_NONE) && (rv != BCM_E_NOT_FOUND)) {
                return rv;
            }
            rv = BCM_E_NONE;
            BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
        }

        if (!SOC_PORT_VALID(unit, port)) {
            return (BCM_E_PORT);
        }

        if (IS_CPU_PORT(unit, port) &&
            !soc_feature(unit, soc_feature_cpuport_mirror)) {
            return (BCM_E_PORT);
        }
    }

    if (!soc_feature(unit, soc_feature_egr_mirror_true) &&
        (flags & BCM_MIRROR_PORT_EGRESS_TRUE)) {
        return (BCM_E_PARAM);
    }

    if (!(flags & (BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_EGRESS |
                   BCM_MIRROR_PORT_EGRESS_TRUE))) { 
        return (BCM_E_PARAM);
    }

    if (0 == BCM_GPORT_IS_MIRROR(mirror_dest)) {
        return (BCM_E_PARAM);
    }

    MIRROR_LOCK(unit);

    /* If destination is not valid */
    if (0 == MIRROR_DEST_REF_COUNT(unit, mirror_dest)) {
       MIRROR_UNLOCK(unit);
       return (BCM_E_NOT_FOUND);
    }

    MIRROR_UNLOCK(unit);

    /* check supported conditions for vp mirroring */
    if (vp != VP_PORT_INVALID) {
#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_feature(unit, soc_feature_mirror_flexible) && 
            !MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit) &&
            (!(flags & BCM_MIRROR_PORT_EGRESS_TRUE))) {
            vp_mirror = TRUE;
        }
#endif
        if (vp_mirror == FALSE) {
            return BCM_E_UNAVAIL;
        }
    }

    /* Directed  mirroring support check. */
    if (MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
        /* No mirroring to a trunk. */
        if (BCM_GPORT_IS_TRUNK(MIRROR_DEST_GPORT(unit, mirror_dest))) {
            return (BCM_E_UNAVAIL);
        } 

        if (soc_feature(unit, soc_feature_mirror_flexible)) {
            if (0 != (flags & BCM_MIRROR_PORT_INGRESS)) {
                if (MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, 0)) {
                    if (MIRROR_CONFIG_SHARED_MTP_DEST(unit, 0) !=
                        mirror_dest) {
                        return (BCM_E_RESOURCE);
                    }
                }
            }
            if (0 != (flags & BCM_MIRROR_PORT_EGRESS)) {
                if (MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit,
                           BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX)) {
                    if (MIRROR_CONFIG_SHARED_MTP_DEST(unit,
                               BCM_MIRROR_MTP_FLEX_EGRESS_D15_INDEX) !=
                        mirror_dest) {
                        return (BCM_E_RESOURCE);
                    }
                }
            }
        } else {
            /* Single mirroring destination for ingress & egress. */
            if (MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0)) {
                if (MIRROR_CONFIG_ING_MTP_DEST(unit, 0) != mirror_dest) {
                    return (BCM_E_RESOURCE);
                }
            }
            if (MIRROR_CONFIG_EGR_MTP_REF_COUNT(unit, 0)) {
                if (MIRROR_CONFIG_EGR_MTP_DEST(unit, 0) != mirror_dest) {
                    return (BCM_E_RESOURCE);
                }
            }
        }

        /* Some devices do not support non-directed mode */
        if (soc_feature(unit, soc_feature_directed_mirror_only)) {
            return (BCM_E_CONFIG);
        }

        /* True egress mode does not support non directed mirroring */
        if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
            return (BCM_E_CONFIG);
        }
    }

    MIRROR_LOCK(unit);

    if (flags & BCM_MIRROR_PORT_INGRESS) {
        if (!MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
            /* Is this port to MTP already configured? */
            if (vp == VP_PORT_INVALID) {
                rv = _bcm_esw_mirror_port_dest_search(unit, port,
                                                  BCM_MIRROR_PORT_INGRESS,
                                                  mirror_dest);
            } else {
                rv = bcm_esw_mirror_port_dest_get(unit, orig_gport, 
                                      BCM_MIRROR_PORT_INGRESS,
                                      BCM_MIRROR_MTP_COUNT,
                                      mirror_dest_list, &mirror_dest_count);
                if (BCM_SUCCESS(rv)) {
                    rv = BCM_E_NOT_FOUND;
                    for (mtp = 0; mtp < mirror_dest_count; mtp++) {
                        if (mirror_dest_list[mtp] == mirror_dest) {
                            rv = BCM_E_EXISTS;
                            break;
                        }
                    }
                }
            }
            if (BCM_E_NOT_FOUND != rv) {
                MIRROR_UNLOCK(unit);
                return rv;
            }
        }

        rv = _bcm_esw_mirror_port_ingress_dest_add(unit, port, mirror_dest, vp);
    }

    if (BCM_SUCCESS(rv) && (flags & BCM_MIRROR_PORT_EGRESS)) {
        if (!MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
            /* Is this port to MTP already configured? */
            if (vp == VP_PORT_INVALID) {
                rv = _bcm_esw_mirror_port_dest_search(unit, port,
                                                  BCM_MIRROR_PORT_EGRESS,
                                                  mirror_dest);
            } else {
                rv = bcm_esw_mirror_port_dest_get(unit, orig_gport, 
                                      BCM_MIRROR_PORT_EGRESS,
                                      BCM_MIRROR_MTP_COUNT,
                                      mirror_dest_list, &mirror_dest_count);
                if (BCM_SUCCESS(rv)) {
                    rv = BCM_E_NOT_FOUND;
                    for (mtp = 0; mtp < mirror_dest_count; mtp++) {
                        if (mirror_dest_list[mtp] == mirror_dest) {
                            rv = BCM_E_EXISTS;
                            break;
                        }
                    }
                }
            }
            if (BCM_E_NOT_FOUND == rv) {
                rv = BCM_E_NONE;
            }
        }

        if (BCM_SUCCESS(rv)) {
            rv = _bcm_esw_mirror_port_egress_dest_add(unit, port,
                                                      mirror_dest, vp);
        }

        /* Check for operation failure. */
        if (BCM_FAILURE(rv)) {
            if (flags & BCM_MIRROR_PORT_INGRESS) {
                _bcm_esw_mirror_port_ingress_dest_delete(unit, port,
                                                         mirror_dest, vp);
            }
        }
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (BCM_SUCCESS(rv) && (flags & BCM_MIRROR_PORT_EGRESS_TRUE)) {
        if (!MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
            /* Is this port to MTP already configured? */
            rv = _bcm_esw_mirror_port_dest_search(unit, port,
                                           BCM_MIRROR_PORT_EGRESS_TRUE,
                                                  mirror_dest);
            if (BCM_E_NOT_FOUND == rv) {
                rv = BCM_E_NONE;
            }
        }

        if (BCM_SUCCESS(rv)) {
            rv = _bcm_esw_mirror_port_egress_true_dest_add(unit, port,
                                                           mirror_dest);
        }

        /* Check for operation failure. */
        if (BCM_FAILURE(rv)) {
            if (flags & BCM_MIRROR_PORT_INGRESS) {
                _bcm_esw_mirror_port_ingress_dest_delete(unit, port,
                                            mirror_dest, VP_PORT_INVALID);
            }
            if (flags & BCM_MIRROR_PORT_EGRESS) {
                _bcm_esw_mirror_port_egress_dest_delete(unit, port,
                                              mirror_dest, VP_PORT_INVALID);
            }
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */  
  

    /* Update stacking mirror destination bitmap. */
    if (vp == VP_PORT_INVALID) {
        if (BCM_SUCCESS(rv) && (-1 != port) && IS_ST_PORT(unit, port)) {
            rv = _bcm_esw_mirror_stacking_dest_update
                                 (unit, port, MIRROR_DEST_GPORT(unit, mirror_dest));
            /* Check for operation failure. */
            if (BCM_FAILURE(rv)) {
                if (flags & BCM_MIRROR_PORT_INGRESS) {
                    _bcm_esw_mirror_port_ingress_dest_delete(unit, port,
                                              mirror_dest, VP_PORT_INVALID);
                }
                if (flags & BCM_MIRROR_PORT_EGRESS) {
                    _bcm_esw_mirror_port_egress_dest_delete(unit, port,
                                               mirror_dest, VP_PORT_INVALID);
                }
#ifdef BCM_TRIUMPH2_SUPPORT
                if (flags & BCM_MIRROR_PORT_EGRESS_TRUE) {
                    _bcm_esw_mirror_port_egress_true_dest_delete(unit, port,
                                                             mirror_dest);
                }
#endif /* BCM_TRIUMPH2_SUPPORT */  
            }
        }
    }

    /* Enable mirroring on a port.  */
    if (BCM_SUCCESS(rv)) { 
        if(!SOC_IS_XGS3_SWITCH(unit) || 
           (BCM_MIRROR_DISABLE == MIRROR_CONFIG_MODE(unit))) {
            rv = _bcm_esw_mirror_enable(unit);
            MIRROR_CONFIG_MODE(unit) = BCM_MIRROR_L2;
        }
    }

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    MIRROR_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *     bcm_esw_mirror_port_dest_delete
 * Purpose:
 *      Remove mirroring destination from a port. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Port mirrored port.
 *      flags        -  (IN) BCM_MIRROR_PORT_XXX flags.
 *      mirror_dest  -  (IN) Mirroring destination gport. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_esw_mirror_port_dest_delete(int unit, bcm_port_t port, 
                                uint32 flags, bcm_gport_t mirror_dest) 
{
    int final_rv = BCM_E_NONE;      /* Operation return status. */
    int rv = BCM_E_NONE;            /* Operation return status. */
    int vp = VP_PORT_INVALID;
    int vp_mirror = FALSE;

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Input parameters check. */

    if (-1 != port) { 
        if (BCM_GPORT_IS_SET(port)) {
            rv = _bcm_tr2_mirror_vp_port_get(unit, port,
                                      &vp, &port); 
            if ((rv != BCM_E_NONE) && (rv != BCM_E_NOT_FOUND)) {
                return rv;
            }
            rv = BCM_E_NONE;
            BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
        }

        if (!SOC_PORT_VALID(unit, port)) {
            return (BCM_E_PORT);
        }

        if (IS_CPU_PORT(unit, port) &&
            !soc_feature(unit, soc_feature_cpuport_mirror)) {
            return (BCM_E_PORT);
        }
    }

    if (!soc_feature(unit, soc_feature_egr_mirror_true) &&
        (flags & BCM_MIRROR_PORT_EGRESS_TRUE)) {
        return (BCM_E_PARAM);
    }

    if (!(flags & (BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_EGRESS |
                   BCM_MIRROR_PORT_EGRESS_TRUE))) { 
        return (BCM_E_PARAM);
    }

    if (0 == BCM_GPORT_IS_MIRROR(mirror_dest)) {
        return (BCM_E_PARAM);
    }

    /* check supported conditions for vp mirroring */
    if (vp != VP_PORT_INVALID) {
#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_feature(unit, soc_feature_mirror_flexible) &&
            (!(flags & BCM_MIRROR_PORT_EGRESS_TRUE))) {
            vp_mirror = TRUE;
        }
#endif
        if (vp_mirror == FALSE) {
            return BCM_E_UNAVAIL;
        }
    }

    MIRROR_LOCK(unit);

    if ((flags & BCM_MIRROR_PORT_INGRESS) &&
        (BCM_GPORT_INVALID != mirror_dest)) {
        final_rv = _bcm_esw_mirror_port_ingress_dest_delete(unit, port,
                                                            mirror_dest, vp);
    }

    if ((flags & BCM_MIRROR_PORT_EGRESS) &&
        (BCM_GPORT_INVALID != mirror_dest)) {
        rv = _bcm_esw_mirror_port_egress_dest_delete(unit, port,
                                                     mirror_dest, vp);
        if (!BCM_FAILURE(final_rv)) {
            final_rv = rv;
        }
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if ((flags & BCM_MIRROR_PORT_EGRESS_TRUE) &&
        (BCM_GPORT_INVALID != mirror_dest)) {
        rv = _bcm_esw_mirror_port_egress_true_dest_delete(unit, port,
                                                          mirror_dest);
        if (!BCM_FAILURE(final_rv)) {
            final_rv = rv;
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    /* Update stacking mirror destination bitmap. */
    if (vp == VP_PORT_INVALID) {
        if ((-1 != port) && (IS_ST_PORT(unit, port))) {
            rv = _bcm_esw_mirror_stacking_dest_update(unit, port, BCM_GPORT_INVALID);
            if (!BCM_FAILURE(final_rv)) {
                final_rv = rv;
            }
        }
    } 

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    MIRROR_UNLOCK(unit);
    return (final_rv);
}

/*
 * Function:
 *     bcm_esw_mirror_port_dest_get
 * Purpose:
 *     Get port mirroring destinations.   
 * Parameters:
 *     unit             - (IN) BCM device number. 
 *     port             - (IN) Port mirrored port.
 *     flags            - (IN) BCM_MIRROR_PORT_XXX flags.
 *     mirror_dest_size - (IN) Preallocated mirror_dest array size.
 *     mirror_dest      - (OUT)Filled array of port mirroring destinations
 *     mirror_dest_count - (OUT)Actual number of mirroring destinations filled.
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_esw_mirror_port_dest_get(int unit, bcm_port_t port, uint32 flags, 
                         int mirror_dest_size, bcm_gport_t *mirror_dest,
                         int *mirror_dest_count)
{
#if defined(BCM_XGS3_SWITCH_SUPPORT)
    int         idx;                    /* Mirror to port iteration index.  */
    int         index = 0;              /* Filled destinations index.       */
    bcm_gport_t mtp_dest[BCM_MIRROR_MTP_COUNT]; /* Mirror destinations array. */
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    int         rv = BCM_E_NONE;        /* Operation return status.         */
    int         vp = VP_PORT_INVALID;
    int         vp_mirror = FALSE;

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    if (BCM_GPORT_INVALID == port) {
        if (!MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
            return (BCM_E_PORT);
        }
        MIRROR_LOCK(unit);
        if (soc_feature(unit, soc_feature_mirror_flexible) &&
            !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
            if (MIRROR_CONFIG_SHARED_MTP_REF_COUNT(unit, 0)) {
                if(NULL != mirror_dest) {
                    mirror_dest[0] = MIRROR_CONFIG_SHARED_MTP_DEST(unit, 0);
                }
                *mirror_dest_count = 1;    
            } else {
                if(NULL != mirror_dest) {
                    mirror_dest[0] = BCM_GPORT_INVALID;
                }
                *mirror_dest_count = 0; 
            }   
        } else {
            if (MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0)) {
                if(NULL != mirror_dest) {
                    mirror_dest[0] = MIRROR_CONFIG_ING_MTP_DEST(unit, 0);
                }
                *mirror_dest_count = 1;    
            } else {
                if(NULL != mirror_dest) {
                    mirror_dest[0] = BCM_GPORT_INVALID;
                }
                *mirror_dest_count = 0;    
            }
        }
        MIRROR_UNLOCK(unit);
        return BCM_E_NONE;
    }

    /* Input parameters check. */
    if (BCM_GPORT_IS_SET(port)) {
        rv = _bcm_tr2_mirror_vp_port_get(unit, port,
                                      &vp, &port);
        if ((rv != BCM_E_NONE) && (rv != BCM_E_NOT_FOUND)) {
            return rv;
        }
        rv = BCM_E_NONE; 
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &port));
    }

    if (!SOC_PORT_VALID(unit, port)) {
        return (BCM_E_PORT);
    }

    if (IS_CPU_PORT(unit, port) &&
        !soc_feature(unit, soc_feature_cpuport_mirror)) {
        return (BCM_E_PORT);
    }

    if ((0 != mirror_dest_size) && (NULL == mirror_dest)) {
        return (BCM_E_PARAM);
    }

    if (NULL == mirror_dest_count) {
        return (BCM_E_PARAM);
    }

    if (!soc_feature(unit, soc_feature_egr_mirror_true) &&
        (flags & BCM_MIRROR_PORT_EGRESS_TRUE)) {
        return (BCM_E_PARAM);
    }

    if (!(flags & (BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_EGRESS |
                   BCM_MIRROR_PORT_EGRESS_TRUE))) {
        return (BCM_E_PARAM);
    }

    /* check supported conditions for vp mirroring */
    if (vp != VP_PORT_INVALID) {
#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_feature(unit, soc_feature_mirror_flexible) &&
            (!(flags & BCM_MIRROR_PORT_EGRESS_TRUE))) {
            vp_mirror = TRUE;
        }
#endif
        if (vp_mirror == FALSE) {
            return BCM_E_UNAVAIL;
        }
    }

    MIRROR_LOCK(unit);

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_XGS3_SWITCH(unit))  {
        if (flags & BCM_MIRROR_PORT_INGRESS) {
            rv = _bcm_xgs3_mirror_port_ingress_dest_get(unit, port, 
                                                        BCM_MIRROR_MTP_COUNT,
                                                        mtp_dest, vp);
            if (BCM_SUCCESS(rv)) {
                for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) { 
                    if ((index < mirror_dest_size) && 
                        (BCM_GPORT_INVALID != mtp_dest[idx])) {
                        if(NULL != (mirror_dest + index)) {
                            mirror_dest[index] = mtp_dest[idx];
                        }
                        index++;
                    }
                }
            }
        }
        if ((flags & BCM_MIRROR_PORT_EGRESS) &&
            (index <  mirror_dest_size)) {
            rv = _bcm_xgs3_mirror_port_egress_dest_get(unit, port, 
                                                       BCM_MIRROR_MTP_COUNT,
                                                       mtp_dest, vp);
            if (BCM_SUCCESS(rv)) {
                for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) { 
                    if ((index < mirror_dest_size) && 
                        (BCM_GPORT_INVALID != mtp_dest[idx])) {
                        if(NULL != (mirror_dest + index)) {
                            mirror_dest[index] = mtp_dest[idx];
                        }
                        index++;
                    }
                }
            }
        }
#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_feature(unit, soc_feature_egr_mirror_true) &&
            (flags & BCM_MIRROR_PORT_EGRESS_TRUE) &&
            (index <  mirror_dest_size)) {
            rv = _bcm_tr2_mirror_port_egress_true_dest_get(unit, port, 
                                                       BCM_MIRROR_MTP_COUNT,
                                                       mtp_dest);
            if (BCM_SUCCESS(rv)) {
                for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) { 
                    if ((index < mirror_dest_size) && 
                        (BCM_GPORT_INVALID != mtp_dest[idx])) {
                        if(NULL != (mirror_dest + index)) {
                            mirror_dest[index] = mtp_dest[idx];
                        }
                        index++;
                    }
                }
            }
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
        *mirror_dest_count = index;
    } else 
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    {
        if (MIRROR_CONFIG_ING_MTP_REF_COUNT(unit, 0)) {
            if(NULL != mirror_dest) {
                mirror_dest[0] = MIRROR_CONFIG_ING_MTP_DEST(unit, 0);
            }
            *mirror_dest_count = 1;    
        } else {
            if(NULL != mirror_dest) {
                mirror_dest[0] = BCM_GPORT_INVALID;
            }
            *mirror_dest_count = 0;    
        }
    }

    MIRROR_UNLOCK(unit);

    return (rv);
}

/*
 * Function:
 *     bcm_esw_mirror_port_dest_delete_all
 * Purpose:
 *      Remove all mirroring destinations from a port. 
 * Parameters:
 *      unit         -  (IN) BCM device number. 
 *      port         -  (IN) Port mirrored port.
 *      flags        -  (IN) BCM_MIRROR_PORT_XXX flags.
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_esw_mirror_port_dest_delete_all(int unit, bcm_port_t port, uint32 flags) 
{
    bcm_gport_t mirror_dest[BCM_MIRROR_MTP_COUNT];
    int         mirror_dest_count;
    int         index;
    int         rv; 

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    if (!(flags & (BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_EGRESS |
                   BCM_MIRROR_PORT_EGRESS_TRUE))) { 
        return (BCM_E_PARAM);
    }

    MIRROR_LOCK(unit);

    if (flags & BCM_MIRROR_PORT_INGRESS) {
        if (-1 != port) {
            if (BCM_GPORT_IS_SET(port)) {
                rv = bcm_esw_port_local_get(unit, port, &port);
                if (BCM_FAILURE(rv))
                {
                    MIRROR_UNLOCK(unit);
                    return rv;
                }
            }
            if(!SOC_PORT_VALID(unit, port)) {
                MIRROR_UNLOCK(unit);
                return (BCM_E_PORT);
            }

            /* Read port ingress mirroring destination ports. */
            rv = bcm_esw_mirror_port_dest_get(unit, port, BCM_MIRROR_PORT_INGRESS,
                                              BCM_MIRROR_MTP_COUNT, mirror_dest,
                                              &mirror_dest_count);
        } else {
            /* Get all ingress mirror destinations. */
            rv = _bcm_mirror_dest_get_all(unit, BCM_MIRROR_PORT_INGRESS, 
                                         BCM_MIRROR_MTP_COUNT, mirror_dest, 
                                         &mirror_dest_count);
        }
        if (BCM_FAILURE(rv)) {
            MIRROR_UNLOCK(unit);
            return (rv);
        }

        /* Remove all ingress mirroring destination ports. */
        for (index = 0; index < mirror_dest_count; index++) {
            rv = bcm_esw_mirror_port_dest_delete(unit, port,
                                                 BCM_MIRROR_PORT_INGRESS,
                                                 mirror_dest[index]); 
            if (BCM_FAILURE(rv)) {
                MIRROR_UNLOCK(unit);
                return (rv);
            }
        }
    }

    if (flags & BCM_MIRROR_PORT_EGRESS) {
        /* Read port egress mirroring destination ports. */
        if (-1 != port) {
            if (BCM_GPORT_IS_SET(port)) {
                rv = bcm_esw_port_local_get(unit, port, &port);
                if (BCM_FAILURE(rv))
                {
                    MIRROR_UNLOCK(unit);
                    return rv;
                }
            }
            if(!SOC_PORT_VALID(unit, port)) {
                MIRROR_UNLOCK(unit);
                return (BCM_E_PORT);
            }

            rv = bcm_esw_mirror_port_dest_get(unit, port, BCM_MIRROR_PORT_EGRESS, 
                                              BCM_MIRROR_MTP_COUNT, mirror_dest, 
                                              &mirror_dest_count);
        } else {
            /* Get all egress mirror destinations. */
            rv = _bcm_mirror_dest_get_all(unit, BCM_MIRROR_PORT_EGRESS, 
                                          BCM_MIRROR_MTP_COUNT, mirror_dest, 
                                          &mirror_dest_count);
        }
        if (BCM_FAILURE(rv)) {
            MIRROR_UNLOCK(unit);
            return (rv);
        }

        /* Remove all egress mirroring destination ports. */
        for (index = 0; index < mirror_dest_count; index++) {
            rv = bcm_esw_mirror_port_dest_delete(unit, port,
                                                 BCM_MIRROR_PORT_EGRESS,
                                                 mirror_dest[index]); 
            if (BCM_FAILURE(rv)) {
                MIRROR_UNLOCK(unit);
                return (rv);
            }
        }
    }

    if (soc_feature(unit, soc_feature_egr_mirror_true) &&
        (flags & BCM_MIRROR_PORT_EGRESS_TRUE)) {
        /* Read port egress true mirroring destination ports. */
        if (-1 != port) {
            if (BCM_GPORT_IS_SET(port)) {
                rv = bcm_esw_port_local_get(unit, port, &port);
                if (BCM_FAILURE(rv))
                {
                    MIRROR_UNLOCK(unit);
                    return rv;
                }
            }
            if(!SOC_PORT_VALID(unit, port)) {
                MIRROR_UNLOCK(unit);
                return (BCM_E_PORT);
            }

            rv = bcm_esw_mirror_port_dest_get(unit, port,
                                              BCM_MIRROR_PORT_EGRESS_TRUE, 
                                              BCM_MIRROR_MTP_COUNT,
                                              mirror_dest, 
                                              &mirror_dest_count);
        } else {
            /* Get all egress mirror destinations. */
            rv = _bcm_mirror_dest_get_all(unit, BCM_MIRROR_PORT_EGRESS_TRUE, 
                                          BCM_MIRROR_MTP_COUNT, mirror_dest, 
                                          &mirror_dest_count);
        }
        if (BCM_FAILURE(rv)) {
            MIRROR_UNLOCK(unit);
            return (rv);
        }

        /* Remove all egress mirroring destination ports. */
        for (index = 0; index < mirror_dest_count; index++) {
            rv = bcm_esw_mirror_port_dest_delete(unit, port,
                                                 BCM_MIRROR_PORT_EGRESS_TRUE,
                                                 mirror_dest[index]); 
            if (BCM_FAILURE(rv)) {
                MIRROR_UNLOCK(unit);
                return (rv);
            }
        }
    }

    MIRROR_UNLOCK(unit);
    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcm_esw_mirror_destination_create
 * Purpose:
 *     Create mirror destination description.
 * Parameters:
 *      unit         - (IN) BCM device number. 
 *      mirror_dest  - (IN) Mirror destination description.
 * Returns:
 *      BCM_X_XXX
 */
int 
bcm_esw_mirror_destination_create(int unit, bcm_mirror_destination_t *mirror_dest) 
{
    bcm_module_t mymodid;           /* Local module id.              */
    bcm_module_t dest_mod;          /* Destination module id.        */
    bcm_port_t   dest_port;         /* Destination port number.      */
    bcm_mirror_destination_t  mirror_dest_check;
    int rv;   /* Operation return status. */

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Input parameters check. */
    if (NULL == mirror_dest) {
        return (BCM_E_PARAM);
    }

    /* Check if device supports advanced mirroring mode. */
    if (mirror_dest->flags & (BCM_MIRROR_DEST_TUNNEL_IP_GRE |
                              BCM_MIRROR_DEST_PAYLOAD_UNTAGGED)) {

        if (0 == SOC_MEM_IS_VALID(unit, EGR_ERSPANm) && 
            0 == SOC_MEM_IS_VALID(unit, EGR_MIRROR_ENCAP_CONTROLm)) {
            return (BCM_E_UNAVAIL);
        }

        /* Bypass mode is enabled check. */
        if (SOC_MEM_IS_VALID(unit, EGR_ERSPANm) &&
            0 == soc_mem_index_count(unit, EGR_ERSPANm)) {
            return (BCM_E_UNAVAIL);
        }
        if (SOC_MEM_IS_VALID(unit, EGR_MIRROR_ENCAP_CONTROLm) && 
            0 == soc_mem_index_count(unit, EGR_MIRROR_ENCAP_CONTROLm)) {
            return (BCM_E_UNAVAIL);
        } 
    }
    /* Check if device supports mirroring trill and NIV tunneling. */
    if (mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_TRILL) {
        if (!soc_feature(unit, soc_feature_trill)) {
            return (BCM_E_UNAVAIL);
        } else if (mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_NIV) {
            /* Only one extra encap allowed. */
            return BCM_E_PARAM;
        } /* else OK */
    }
    if ((mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_NIV) &&
        !soc_feature(unit, soc_feature_niv)) {
        return (BCM_E_UNAVAIL);
    }

    /* Untagging payload supported only on IP tunnels. */
    if ((0 == (mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_IP_GRE)) && 
        (mirror_dest->flags & BCM_MIRROR_DEST_PAYLOAD_UNTAGGED)) {
        return (BCM_E_UNAVAIL);
    }

    /* Can't do IP-GRE & L3 tunnel simultaneously. */
    if ((mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_IP_GRE) && 
        (mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_L2)) {
        return (BCM_E_CONFIG);
    }

    /* Resolve miror destination gport */
    BCM_IF_ERROR_RETURN(
        _bcm_mirror_gport_adapt(unit, &(mirror_dest->gport)));

    /* Verify mirror destination port/trunk. */
    if ((0 == BCM_GPORT_IS_MODPORT(mirror_dest->gport)) && 
        (0 == BCM_GPORT_IS_TRUNK(mirror_dest->gport)) &&
        (0 == BCM_GPORT_IS_DEVPORT(mirror_dest->gport))) {
        return (BCM_E_PORT);
    }

    /* Get local modid. */
    BCM_IF_ERROR_RETURN(_bcm_esw_local_modid_get(unit, &mymodid));

    /* Directed  mirroring support check. */
    if (MIRROR_MTP_METHOD_IS_NON_DIRECTED(unit)) {
        int isLocal;

        /* No mirroring to a trunk. */
        if (BCM_GPORT_IS_TRUNK(mirror_dest->gport)) {
            return (BCM_E_UNAVAIL);
        } 

        /* Some devices do not support non-directed mode */
        if (soc_feature(unit, soc_feature_directed_mirror_only)) {
            return (BCM_E_CONFIG);
        }

        /* Set mirror destination to outgoing port on local module. */
        dest_mod = BCM_GPORT_IS_DEVPORT(mirror_dest->gport) ? 
            mymodid : BCM_GPORT_MODPORT_MODID_GET(mirror_dest->gport);

        BCM_IF_ERROR_RETURN
            (_bcm_esw_modid_is_local(unit, dest_mod, &isLocal));

        if (FALSE == isLocal) {
            _bcm_gport_dest_t   gport_st;

            BCM_IF_ERROR_RETURN
                (bcm_esw_topo_port_get(unit, dest_mod, &dest_port));
            gport_st.gport_type = BCM_GPORT_TYPE_MODPORT;
            gport_st.modid = mymodid;
            gport_st.port = dest_port;
            BCM_IF_ERROR_RETURN(
                _bcm_esw_gport_construct(unit, &gport_st, 
                                         &(mirror_dest->gport)));
        }
    }

    /* If we are NOT replacing an existing entry, check if it exists. */
    if (0 == (mirror_dest->flags & BCM_MIRROR_DEST_REPLACE)) {
        rv = _bcm_esw_mirror_destination_find(unit, mirror_dest->gport, 0,
                                              mirror_dest->flags,
                                              &mirror_dest_check);
        if (BCM_SUCCESS(rv)) {
            /* Entry exists and we are not replacing, error */
            return BCM_E_EXISTS;
        } else if (BCM_E_NOT_FOUND != rv) {
            /* If something else went wrong, error */
            return rv;
        }
    }
    /* Else, create destination (which handles replace properly) */
    MIRROR_LOCK(unit);
    rv = _bcm_esw_mirror_destination_create(unit, mirror_dest);
    MIRROR_UNLOCK(unit);
    return (rv);
}


/*
 * Function:
 *     bcm_esw_mirror_destination_destroy
 * Purpose:
 *     Destroy mirror destination description.
 * Parameters:
 *      unit            - (IN) BCM device number. 
 *      mirror_dest_id  - (IN) Mirror destination id.
 * Returns:
 *      BCM_X_XXX
 */
int 
bcm_esw_mirror_destination_destroy(int unit, bcm_gport_t mirror_dest_id) 
{
    int rv;   /* Operation return status. */

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    if (0 == BCM_GPORT_IS_MIRROR(mirror_dest_id)) {
        return (BCM_E_PARAM);
    }

    MIRROR_LOCK(unit);

    /* If destination stil in use - > E_BUSY */
    if (1 < MIRROR_DEST_REF_COUNT(unit, mirror_dest_id)) {
        MIRROR_UNLOCK(unit);
        return (BCM_E_BUSY);
    }

    rv = _bcm_mirror_destination_free(unit, mirror_dest_id); 

    MIRROR_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *     bcm_esw_mirror_destination_get
 * Purpose:
 *     Get mirror destination description.
 * Parameters:
 *      unit            - (IN) BCM device number. 
 *      mirror_dest_id  - (IN) Mirror destination id.
 *      mirror_dest     - (IN/OUT)Mirror destination description.
 * Returns:
 *      BCM_X_XXX
 */
int 
bcm_esw_mirror_destination_get(int unit, bcm_gport_t mirror_dest_id, 
                                   bcm_mirror_destination_t *mirror_dest)
{
    bcm_mirror_destination_t    mirror_destination;
    bcm_port_t                  port, port_out;
    bcm_module_t                modid, modid_out;
    int                         rv = BCM_E_NONE;


    bcm_mirror_destination_t_init(&mirror_destination);

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    if (BCM_GPORT_INVALID == mirror_dest_id) {
        /* Find the mirror destination id from the dest description */
        return _bcm_esw_mirror_destination_find(unit, mirror_dest->gport, 0,
                                                mirror_dest->flags,
                                                mirror_dest);
    }

    if (0 == BCM_GPORT_IS_MIRROR(mirror_dest_id)) {
        return (BCM_E_PARAM);
    }

    if (NULL == mirror_dest) {
        return (BCM_E_PARAM);
    }

    MIRROR_LOCK(unit);

    /* If destination is not valid */ 
    if (0 == MIRROR_DEST_REF_COUNT(unit, mirror_dest_id)) {
        MIRROR_UNLOCK(unit);
        return (BCM_E_NOT_FOUND);
    }

    mirror_destination  = *(MIRROR_DEST(unit, mirror_dest_id)); 
    if (BCM_GPORT_IS_MODPORT(mirror_destination.gport)) {
        port = BCM_GPORT_MODPORT_PORT_GET(mirror_destination.gport);
        modid = BCM_GPORT_MODPORT_MODID_GET(mirror_destination.gport);
        if (NUM_MODID(unit) > 1 && port> 31) {
            rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET, modid, port, 
                                       &modid_out, &port_out);
            if (BCM_FAILURE(rv)) {
                MIRROR_UNLOCK(unit);
                return rv;
            }
            if (!SOC_PORT_ADDRESSABLE(unit, port_out)) {
                MIRROR_UNLOCK(unit);
                return BCM_E_PORT;
            }
            if (!SOC_MODID_ADDRESSABLE(unit, modid_out)) {
                MIRROR_UNLOCK(unit);
                return BCM_E_PARAM;
            }
            port = port_out;
            modid = modid_out;
        }
        rv = _bcm_mirror_gport_construct(unit, port,modid, 0, 
                                        &(mirror_destination.gport));
        if (BCM_FAILURE(rv)) {
            MIRROR_UNLOCK(unit);
            return rv;
        }
    }
    *mirror_dest = mirror_destination; 
    MIRROR_UNLOCK(unit);
    return (rv);
}


/*
 * Function:
 *     bcm_esw_mirror_destination_traverse
 * Purpose:
 *     Traverse installed mirror destinations
 * Parameters:
 *      unit      - (IN) BCM device number. 
 *      cb        - (IN) Mirror destination traverse callback.         
 *      user_data - (IN) User cookie
 * Returns:
 *      BCM_X_XXX
 */
int 
bcm_esw_mirror_destination_traverse(int unit, bcm_mirror_destination_traverse_cb cb, 
                                    void *user_data) 
{
    int idx;                                 /* Mirror destinations index.     */
    _bcm_mirror_dest_config_p  mdest;        /* Mirror destination description.*/
    bcm_mirror_destination_t   mirror_dest;  /* User cb mirror destination.    */
    int rv = BCM_E_NONE;

    /* Initialization check. */
    if (0 == MIRROR_INIT(unit)) {
        return BCM_E_INIT;
    }

    /* Input parameters check. */
    if (NULL == cb) {
        return (BCM_E_PARAM);
    }

    MIRROR_LOCK(unit);
    /* Iterate mirror destinations & call user callback for valid ones. */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->dest_count; idx++) {
        mdest = &MIRROR_CONFIG(unit)->dest_arr[idx];
        if (0 == mdest->ref_count) {
            continue;
        }

        mirror_dest = mdest->mirror_dest;
        rv = (*cb)(unit, &mirror_dest, user_data);
        if (BCM_FAILURE(rv)) {
#ifdef BCM_CB_ABORT_ON_ERR
            if (SOC_CB_ABORT_ON_ERR(unit)) {
                MIRROR_UNLOCK(unit);
                return rv;
            }
#endif
        }
    }
    MIRROR_UNLOCK(unit);
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_esw_mirror_lock
 * Purpose:
 *      Allow other modules to take the mirroring mutex
 * Parameters:
 *      unit - unit #
 * Returns:
 *      None
 */
void bcm_esw_mirror_lock(int unit) {
    MIRROR_LOCK(unit);
}

/*
 * Function:
 *      bcm_esw_mirror_unlock
 * Purpose:
 *      Allow other modules to give up the mirroring mutex
 * Parameters:
 *      unit - unit #
 * Returns:
 *      None
 */
void bcm_esw_mirror_unlock(int unit) {
    MIRROR_UNLOCK(unit);
}

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
STATIC int
my_i2xdigit(int digit)
{
    digit &= 0xf;

    return (digit > 9) ? digit - 10 + 'a' : digit + '0';
}

STATIC void
fmt_macaddr(char buf[SAL_MACADDR_STR_LEN], sal_mac_addr_t macaddr)
{
    int i;

    for (i = 0; i <= 5; i++) {
        *buf++ = my_i2xdigit(macaddr[i] >> 4);
        *buf++ = my_i2xdigit(macaddr[i]);
        *buf++ = ':';
    }

    *--buf = 0;
}

STATIC void
fmt_ip6addr(char buf[IP6ADDR_STR_LEN], ip6_addr_t ipaddr)
{
    sal_sprintf(buf, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", 
            (((uint16)ipaddr[0] << 8) | ipaddr[1]),
            (((uint16)ipaddr[2] << 8) | ipaddr[3]),
            (((uint16)ipaddr[4] << 8) | ipaddr[5]),
            (((uint16)ipaddr[6] << 8) | ipaddr[7]),
            (((uint16)ipaddr[8] << 8) | ipaddr[9]),
            (((uint16)ipaddr[10] << 8) | ipaddr[11]),
            (((uint16)ipaddr[12] << 8) | ipaddr[13]),
            (((uint16)ipaddr[14] << 8) | ipaddr[15]));
}

/*
 * Function:
 *     _bcm_mirror_sw_dump
 * Purpose:
 *     Displays mirror software structure information.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_mirror_sw_dump(int unit)
{
    int             idx, mode;
    _bcm_mirror_config_t  *mcp = MIRROR_CONFIG(unit);
    bcm_mirror_destination_t *mdest;
    _bcm_mtp_config_p mtp_cfg;
    char ip6_str[IP6ADDR_STR_LEN];
    char mac_str[SAL_MACADDR_STR_LEN];
    bcm_gport_t gport;

    soc_cm_print("\nSW Information Mirror - Unit %d\n", unit);
    mode = MIRROR_CONFIG_MODE(unit);
    soc_cm_print("  Mode       : %s\n",
                 (mode == BCM_MIRROR_DISABLE) ? "Disabled" :
                 ((mode == BCM_MIRROR_L2) ? "L2" :
                 ((mode == BCM_MIRROR_L2_L3) ? "L2_L3" : "Unknown")));
    soc_cm_print("  Dest Count : %4d\n", mcp->dest_count);
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        soc_cm_print("  Max Ing MTP for Port: %4d\n", mcp->port_im_mtp_count);
        soc_cm_print("  Max Eng MTP for Port: %4d\n", mcp->port_em_mtp_count);
    } else {
        soc_cm_print("  Ing MTP Count: %4d\n", mcp->ing_mtp_count);
        soc_cm_print("  Egr MTP Count: %4d\n", mcp->egr_mtp_count);
    }
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_egr_mirror_true)) {
        soc_cm_print("  Egr True MTP Count: %4d\n",
                     mcp->egr_true_mtp_count);
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    soc_cm_print("  Directed   : %s\n",
                 MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit) ? "Flexible" :
                 (MIRROR_MTP_METHOD_IS_DIRECTED_LOCKED(unit) ?
                  "Locked" : "No"));

    /* Mirror destinations */
    for (idx = 0; idx < MIRROR_CONFIG(unit)->dest_count; idx++) {
        BCM_GPORT_MIRROR_SET(gport, idx);
        if (0 == MIRROR_DEST_REF_COUNT(unit, gport)) {
            continue;
        }

        mdest = MIRROR_DEST(unit, gport);

        soc_cm_print("  Mirror dest(%d): 0x%08x  Ref count: %4d\n",
                     idx, mdest->mirror_dest_id,
                     MIRROR_DEST_REF_COUNT(unit, gport));
        soc_cm_print("              Gport     : 0x%08x\n",
                     mdest->gport);
        soc_cm_print("              TOS       : 0x%02x\n",
                     mdest->tos);
        soc_cm_print("              TTL       : 0x%02x\n",
                     mdest->ttl);
        soc_cm_print("              IP Version: 0x%02x\n",
                     mdest->version);
        if (mdest->version == 4) {
            soc_cm_print("              Src IP    : 0x%08x\n",
                         mdest->src_addr);
            soc_cm_print("              Dest IP   : 0x%08x\n",
                         mdest->dst_addr);
        } else {
            fmt_ip6addr(ip6_str, mdest->src6_addr);
            soc_cm_print("              Src IP    : %-42s\n",
                         ip6_str);
            fmt_ip6addr(ip6_str, mdest->dst6_addr);
            soc_cm_print("              Dest IP   : %-42s\n",
                         ip6_str);
        }
        fmt_macaddr(mac_str, mdest->src_mac);
        soc_cm_print("              Src MAC   : %-18s\n",
                     mac_str);
        fmt_macaddr(mac_str, mdest->dst_mac);
        soc_cm_print("              Dest MAC  : %-18s\n",
                     mac_str);
        soc_cm_print("              Flow label: 0x%08x\n",
                     mdest->flow_label);
        soc_cm_print("              TPID      : 0x%04x\n",
                     mdest->tpid);
        soc_cm_print("              VLAN      : 0x%04x\n",
                     mdest->vlan_id);
        
        soc_cm_print("              Flags     :");
        if (mdest->flags & BCM_MIRROR_DEST_REPLACE) {
            soc_cm_print("  Replace");
        }
        if (mdest->flags & BCM_MIRROR_DEST_WITH_ID) {
            soc_cm_print("  ID provided");
        }
        if (mdest->flags & BCM_MIRROR_DEST_TUNNEL_L2) {
            soc_cm_print("  L2 tunnel");
        }
        if (mdest->flags & BCM_MIRROR_DEST_TUNNEL_IP_GRE) {
            soc_cm_print("  IP GRE tunnel");
        }
#if defined(BCM_TRIDENT_SUPPORT)
        if (mdest->flags & BCM_MIRROR_DEST_TUNNEL_TRILL) {
            soc_cm_print("  TRILL tunnel");
        }
        if (mdest->flags & BCM_MIRROR_DEST_TUNNEL_NIV) {
            soc_cm_print("  NIV tunnel");
        }
#endif /* TRIDENT  */
        if (mdest->flags & BCM_MIRROR_DEST_PAYLOAD_UNTAGGED) {
            soc_cm_print("  Untagged payload");
        }
        if (mdest->flags & _BCM_MIRROR_DESTINATION_LOCAL) {
            soc_cm_print("  Field destination");
        }
        soc_cm_print("\n");
    }

    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        !MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        
        for (idx = 0; idx < BCM_MIRROR_MTP_COUNT; idx++) {
            mtp_cfg = &MIRROR_CONFIG_SHARED_MTP(unit, idx);
            if (0 == mtp_cfg->ref_count) {
                continue;
            }
            
            soc_cm_print("  %s MTP(%d): 0x%08x  Ref count: %4d\n",
                         (TRUE == mtp_cfg->egress) ? "Egr" : "Ing", 
                         idx, mtp_cfg->dest_id, mtp_cfg->ref_count);
        }
    } else {
        /* Ingress MTPs */
        for (idx = 0; idx < MIRROR_CONFIG(unit)->ing_mtp_count; idx++) {
            mtp_cfg = &MIRROR_CONFIG_ING_MTP(unit, idx);
            if (0 == mtp_cfg->ref_count) {
                continue;
            }

            soc_cm_print("  Ing MTP(%d): 0x%08x  Ref count: %4d\n",
                         idx, mtp_cfg->dest_id, mtp_cfg->ref_count);
        }

        /* Egress MTPs */
        for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_mtp_count; idx++) {
            mtp_cfg = &MIRROR_CONFIG_EGR_MTP(unit, idx);
            if (0 == mtp_cfg->ref_count) {
                continue;
            }

            soc_cm_print("  Egr MTP(%d): 0x%08x  Ref count: %4d\n",
                         idx, mtp_cfg->dest_id, mtp_cfg->ref_count);
        }
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_egr_mirror_true)) {
        for (idx = 0; idx < MIRROR_CONFIG(unit)->egr_true_mtp_count; idx++) {
            mtp_cfg = &MIRROR_CONFIG_EGR_TRUE_MTP(unit, idx);
            if (0 == mtp_cfg->ref_count) {
                continue;
            }

            soc_cm_print("  Egress True MTP(%d): 0x%08x  Ref count: %4d\n",
                         idx, mtp_cfg->dest_id, mtp_cfg->ref_count);
        }
    }

    if (soc_feature(unit, soc_feature_mirror_flexible) &&
        MIRROR_MTP_METHOD_IS_DIRECTED_FLEXIBLE(unit)) {
        int mtp_slot, mtp_bit, mtp_type;

        BCM_MIRROR_MTP_ITER(MIRROR_CONFIG_MTP_DEV_MASK(unit), mtp_slot) {
            mtp_bit = 1 << mtp_slot;
            if (0 != MIRROR_CONFIG_MTP_MODE_REF_COUNT(unit, mtp_slot)) {
                soc_cm_print("  MTP slot(%d): %s  Ref count: %4d\n",
                             mtp_slot,
                             MIRROR_CONFIG_MTP_MODE_BMP(unit) & mtp_bit ?
                             "Egress" : "Ingress",
                             MIRROR_CONFIG_MTP_MODE_REF_COUNT(unit,
                                                              mtp_slot));
                for (mtp_type = BCM_MTP_SLOT_TYPE_PORT;
                     mtp_type < BCM_MTP_SLOT_TYPES;
                     mtp_type++) {
                    if (0 != MIRROR_CONFIG_TYPE_MTP_REF_COUNT(unit,
                                           mtp_slot, mtp_type)) {
                        soc_cm_print
                            ("      MTP type(%d): %5s  Ref count: %4d\n",
                             mtp_type,
                             (BCM_MTP_SLOT_TYPE_PORT == mtp_type) ? "Port": (
                             (BCM_MTP_SLOT_TYPE_FP == mtp_type) ? "Field":
                                                                  "IPFIX"),
                             MIRROR_CONFIG_TYPE_MTP_REF_COUNT(unit,
                                                mtp_slot, mtp_type));
                    }
                }
            }
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
    if (soc_feature(unit, soc_feature_mirror_encap_profile)) {
        egr_mirror_encap_control_entry_t control_entry;
        egr_mirror_encap_data_1_entry_t data_1_entry;
        egr_mirror_encap_data_2_entry_t data_2_entry;
        void *entries[EGR_MIRROR_ENCAP_ENTRIES_NUM];
        int i, rv, ref_count, num_entries;

        entries[EGR_MIRROR_ENCAP_ENTRIES_CONTROL] = &control_entry;
        entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_1] = &data_1_entry;
        entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_2] = &data_2_entry;

        num_entries = soc_mem_index_count(unit, EGR_MIRROR_ENCAP_CONTROLm);

        soc_cm_print("\n  Egress encap profiles\n");
        soc_cm_print("    Number of entries: %d\n", num_entries);

        for (i = 0; i < num_entries; i ++) {
            rv = soc_profile_mem_ref_count_get(unit,
                                               EGR_MIRROR_ENCAP(unit),
                                               i, &ref_count);
            if (SOC_FAILURE(rv)) {
                soc_cm_print
                    (" *** Error retrieving profile reference: %d ***\n",
                     rv);
                break;
            }

            if (ref_count <= 0) {
                continue;
            }

            rv = soc_profile_mem_get(unit, EGR_MIRROR_ENCAP(unit),
                                     i, 1, entries);
            if (SOC_FAILURE(rv)) {
                soc_cm_print
                    (" *** Error retrieving profile data: %d ***\n", rv);
                break;
            }

            soc_cm_print("  %5d %8d\n", i, ref_count);
            soc_mem_entry_dump(unit, EGR_MIRROR_ENCAP_CONTROLm,
                               entries[EGR_MIRROR_ENCAP_ENTRIES_CONTROL]);
            soc_cm_print("\n");
            soc_mem_entry_dump(unit, EGR_MIRROR_ENCAP_DATA_1m,
                               entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_1]);
            soc_cm_print("\n");
            soc_mem_entry_dump(unit, EGR_MIRROR_ENCAP_DATA_2m,
                               entries[EGR_MIRROR_ENCAP_ENTRIES_DATA_2]);
            soc_cm_print("\n");
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */
