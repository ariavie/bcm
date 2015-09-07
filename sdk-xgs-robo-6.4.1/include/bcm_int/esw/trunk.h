/*
 * $Id: trunk.h,v 1.55 Broadcom SDK $
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
 * This file contains TRUNK definitions internal to the BCM library.
 */

#ifndef _BCM_INT_TRUNK_H
#define _BCM_INT_TRUNK_H

#include <bcm/trunk.h>
#include <bcm/switch.h>

/* 6-bit field, MSB indicates trunk */
#define BCM_TGID_TRUNK_INDICATOR(unit)     (1 << SOC_TRUNK_BIT_POS(unit))
#define BCM_TGID_PORT_TRUNK_MASK(unit)     ((1 << SOC_TRUNK_BIT_POS(unit)) - 1)
#define BCM_TGID_PORT_TRUNK_MASK_HI(unit)  (0x3 << SOC_TRUNK_BIT_POS(unit))
#define BCM_TGID_PORT_TRUNK_MASK_HI_SHIFT(unit)  (SOC_TRUNK_BIT_POS(unit))

#define BCM_TRUNK_TO_MODIDf(unit, tid) \
 (((tid) & BCM_TGID_PORT_TRUNK_MASK_HI(unit)) >> BCM_TGID_PORT_TRUNK_MASK_HI_SHIFT(unit))
#define BCM_TRUNK_TO_TGIDf(unit, tid) \
 (((tid) & BCM_TGID_PORT_TRUNK_MASK(unit)) | BCM_TGID_TRUNK_INDICATOR(unit))
#define BCM_MODIDf_TGIDf_TO_TRUNK(unit,mod,tgid) \
 ((((mod)<<BCM_TGID_PORT_TRUNK_MASK_HI_SHIFT(unit))&BCM_TGID_PORT_TRUNK_MASK_HI(unit)) | \
 ((tgid) & BCM_TGID_PORT_TRUNK_MASK(unit)))

/*
 * Define:
 *	TRUNK_CHK_TGID32
 * Purpose:
 *	Causes a routine to return BCM_E_PARAM if the specified
 *	trunk group ID is out of range 0-31.
 */

#define TRUNK_CHK_TGID32(unit, tgid) do { \
	if (tgid < 0 || tgid > 31) return BCM_E_PARAM; \
	} while (0);

/*
 * Define:
 *	TRUNK_CHK_TGID128
 * Purpose:
 *	Causes a routine to return BCM_E_PARAM if the specified
 *	trunk group ID is out of range 0-127.
 */

#define TRUNK_CHK_TGID128(unit, tgid) do { \
	if (tgid < 0 || tgid > 127) return BCM_E_PARAM; \
	} while (0);

/*
 * Define:
 *     TRUNK_CHK_TGID_EXTENDED
 * Purpose:
 *     Causes a routine to return BCM_E_PARAM if the specified
 *     trunk group ID is out of range on this unit.
 */
#define TRUNK_CHK_TGID_EXTENDED(unit, tgid) \
    do {                                    \
        if (tgid < 0 || tgid > (soc_mem_index_max(unit, TRUNK_GROUPm))) { \
            return (BCM_E_PARAM);           \
        }                                   \
    } while (0);

typedef struct trunk_private_s {
    int		tid;			/* trunk group ID */
    int		in_use;
    int		psc;			/* port spec criterion */
    int		ipmc_psc;	        /* IPMC port spec criterion */
    int		rtag;			/* hardware rtag value */
    uint32      flags;                  /* BCM_TRUNK_FLAG_FAILOVER_xxx */
    int		dlf_index_spec;		/* requested by user */
    int		dlf_index_used;		/* actually used */
    int		dlf_port_used;		/* actually used or -1 */
    int		mc_index_spec;		/* requested by user */
    int		mc_index_used;		/* actually used */
    int		mc_port_used;		/* actually used or -1 */
    int		ipmc_index_spec;	/* requested by user */
    int		ipmc_index_used;	/* actually used */
    int		ipmc_port_used;		/* actually used or -1 */
    uint32      dynamic_size;           /* number of flows for
                                           dynamic load balancing */
    uint32      dynamic_age;            /* Inactivity duration, in usec */
    uint32      dynamic_load_exponent;  /* The exponent used in the
                                           exponentially weighted moving
                                           average calculation of historical
                                           member load. */
    uint32      dynamic_expected_load_exponent; /* The exponent used in the
                                           exponentially weighted moving
                                           average calculation of historical
                                           expected member load. */
} trunk_private_t;

/* The following structure is very similar to bcm_trunk_add_info_t,
 * except that member_flags, tp, and tm are pointers to dynamically
 * allocated arrays instead of fixed size arrays.
 */
typedef struct _esw_trunk_add_info_s {
    uint32 flags;                       /* BCM_TRUNK_FLAG_xxx. */
    int num_ports;                      /* Number of ports in the trunk group. */
    int psc;                            /* Port selection criteria. */
    int ipmc_psc;                       /* Port selection criteria for software
                                           IPMC trunk resolution. */
    int dlf_index;                      /* DLF/broadcast port for trunk group. */
    int mc_index;                       /* Multicast port for trunk group. */
    int ipmc_index;                     /* IPMC port for trunk group. */
    uint32 *member_flags;               /* BCM_TRUNK_MEMBER_xxx */
    bcm_port_t *tp;                     /* Ports in trunk. */
    bcm_module_t *tm;                   /* Modules per port. */
    int *dynamic_scaling_factor;        /* Per member DLB threshold scaling
                                           factor */
    int *dynamic_load_weight;           /* Weight of load in determining DLB
                                           member quality */
    uint32 dynamic_size;                /* Number of flows for dynamic load
                                           balancing */
    uint32 dynamic_age;                 /* Inactivity duration, in usec */
    uint32 dynamic_load_exponent;       /* The exponent used in the
                                           exponentially weighted moving average
                                           calculation of historical member
                                           load. */
    uint32 dynamic_expected_load_exponent; /* The exponent used in the
                                           exponentially weighted moving average
                                           calculation of historical expected
                                           member load. */
} _esw_trunk_add_info_t;

#define BCM_XGS3_TRUNK_MAX_PORTCNT   16
            /* Max # of ports in a front-panel or fabric trunk group on devices before Trident. */
#define BCM_SWITCH_TRUNK_MAX_PORTCNT   8       
            /* Max # of ports in a front-panel trunk group on devices before Trident */

#define TRUNK_MEMBER_OP_SET    0
#define TRUNK_MEMBER_OP_ADD    1
#define TRUNK_MEMBER_OP_DELETE 2

#define TRUNK_BLOCK_MASK_TYPE_IPMC  0
#define TRUNK_BLOCK_MASK_TYPE_L2MC  1
#define TRUNK_BLOCK_MASK_TYPE_BCAST 2
#define TRUNK_BLOCK_MASK_TYPE_DLF   3

extern int _bcm_esw_trunk_gport_array_resolve(int unit, int fabric_trunk,
                                   int count,
                                   bcm_gport_t *port_array,
                                   bcm_port_t *port_list,
                                   bcm_module_t *modid_list);

extern int _bcm_esw_trunk_gport_construct(int unit, int fabric_trunk,
                               int count,
                               bcm_port_t *port_list,
                               bcm_module_t *modid_list,
                               bcm_gport_t *port_array);

extern int _bcm_esw_trunk_port_property_get(int unit, 
                                            bcm_module_t modid,
                                            bcm_port_t port, 
                                            bcm_trunk_t *tid);

extern int _bcm_esw_trunk_rtag_get(int unit, bcm_trunk_t tid, int *rtag);

extern int _bcm_esw_trunk_active_member_get(int unit, bcm_trunk_t tid, 
                                 bcm_trunk_info_t *trunk_info,
                                 int member_max, 
                                 bcm_trunk_member_t *active_member_array,
                                 int *active_member_count);
extern int _bcm_nuc_tpbm_get(int unit,
                             int num_ports,
                             bcm_module_t tm[BCM_TRUNK_MAX_PORTCNT],
                             uint32 *nuc_tpbm);

#if defined (BCM_XGS3_SWITCH_SUPPORT)
#define _BCM_XGS3_TRUNK_MOD_PORT_MAP_IDX_COUNT(_unit_)     \
      (SOC_MAX_NUM_PORTS * ((SOC_MODID_MAX(_unit_) > 0) ?  \
                            (1 + SOC_MODID_MAX(_unit_)) : 1))
extern int _bcm_xgs3_trunk_mod_port_map_init(int unit);
extern int _bcm_xgs3_trunk_mod_port_map_deinit(int unit);

extern int  _bcm_xgs3_trunk_swfailover_init(int unit);
extern void _bcm_xgs3_trunk_swfailover_detach(int unit);

extern int  _bcm_xgs3_trunk_member_init(int unit);
extern void _bcm_xgs3_trunk_member_detach(int unit);

extern int bcm_xgs3_trunk_modify(int, bcm_trunk_t,
                        bcm_trunk_info_t *, int, bcm_trunk_member_t *,
                        trunk_private_t *, int, bcm_trunk_member_t *);
extern int bcm_xgs3_trunk_get(int, bcm_trunk_t,
                        bcm_trunk_info_t *, int, bcm_trunk_member_t *,
                        int *, trunk_private_t *);
extern int bcm_xgs3_trunk_destroy(int, bcm_trunk_t, trunk_private_t *);
extern int bcm_xgs3_trunk_mcast_join(int, bcm_trunk_t, bcm_vlan_t, 
                               bcm_mac_t, trunk_private_t *);
extern int _bcm_xgs3_trunk_fabric_find(int unit, 
                                       bcm_port_t port, 
                                       bcm_trunk_t *tid);
extern int _bcm_xgs3_trunk_get_port_property(int unit, 
                                             bcm_module_t mod,
                                             bcm_port_t port, 
                                             bcm_trunk_t *tid);

extern int  _bcm_xgs3_trunk_hwfailover_init(int unit);
extern void _bcm_xgs3_trunk_hwfailover_detach(int unit);

extern int _bcm_xgs3_trunk_hwfailover_set(int unit, bcm_trunk_t tid,
                                          int hg_trunk,
                                          bcm_port_t port,
                                          bcm_module_t modid,
                                          int psc,
                                          uint32 flags, int count, 
                                          bcm_port_t *ftp,
                                          bcm_module_t *ftm);
extern int _bcm_xgs3_trunk_hwfailover_get(int unit, bcm_trunk_t tid,
                                          int hg_trunk,
                                          bcm_port_t port,
                                          bcm_module_t modid,
                                          int *psc,
                                          uint32 *flags, int array_size, 
                                          bcm_port_t *ftp,
                                          bcm_module_t *ftm,
                                          int *array_count);
extern int _bcm_xgs3_trunk_property_migrate(int unit,
                int num_leaving, bcm_gport_t *leaving_arr,
                int num_staying, bcm_gport_t *staying_arr,
                int num_joining, bcm_gport_t *joining_arr);

#ifdef BCM_TRIDENT_SUPPORT
extern int _bcm_trident_trunk_init(int unit);
void _bcm_trident_trunk_deinit(int unit);
extern int _bcm_trident_trunk_override_mcast_set(int unit,
                                                 bcm_trunk_t hgtid,
                                                 int idx,
                                                 int enable);
extern int _bcm_trident_trunk_override_mcast_get(int unit,
                                                 bcm_trunk_t hgtid,
                                                 int idx,
                                                 int *enable);
extern int _bcm_trident_trunk_override_ipmc_set(int unit,
                                                bcm_trunk_t hgtid,
                                                int idx,
                                                int enable);
extern int _bcm_trident_trunk_override_ipmc_get(int unit,
                                                bcm_trunk_t hgtid,
                                                int idx,
                                                int *enable);
extern int _bcm_trident_trunk_override_vlan_set(int unit,
                                                bcm_trunk_t hgtid,
                                                int idx,
                                                int enable);
extern int _bcm_trident_trunk_override_vlan_get(int unit,
                                                bcm_trunk_t hgtid,
                                                int idx,
                                                int *enable);
extern int _bcm_trident_trunk_fabric_find(int unit, 
                                          bcm_port_t port, 
                                          bcm_trunk_t *tid);
extern int _bcm_trident_trunk_get_port_property(int unit, bcm_module_t mod, 
                                                bcm_port_t port, int *tid);
extern int _bcm_trident_trunk_hwfailover_set(int unit,
                                             bcm_trunk_t tid, int hg_trunk,
                                             bcm_port_t port, bcm_module_t modid,
                                             int psc, uint32 flags, int count, 
                                             bcm_port_t *ftp, bcm_module_t *ftm);
extern int _bcm_trident_trunk_hwfailover_get(int unit,
                                             bcm_trunk_t tid, int hg_trunk,
                                             bcm_port_t port, bcm_module_t modid,
                                             int *psc, uint32 *flags, int array_size,
                                             bcm_port_t *ftp, bcm_module_t *ftm, 
                                             int *array_count);
extern int _bcm_trident_hg_dlb_config_set(int unit,
                                          bcm_switch_control_t type,
                                          int arg);
extern int _bcm_trident_hg_dlb_config_get(int unit,
                                          bcm_switch_control_t type,
                                          int *arg);
extern int bcm_trident_hg_dlb_member_status_set(int unit, bcm_port_t port,
        int status);
extern int bcm_trident_hg_dlb_member_status_get(int unit, bcm_port_t port,
        int *status);
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
extern void bcm_tr3_lag_dlb_deinit(int unit);
extern int bcm_tr3_lag_dlb_init(int unit);
extern int bcm_tr3_lag_dlb_free_resource(int unit, int tid);
extern int bcm_tr3_lag_dlb_set(int unit, int tid,
        _esw_trunk_add_info_t *add_info,
        int num_dlb_ports, int *mod_array, int *port_array,
        int *scaling_factor_array, int *load_weight_array);
extern int bcm_tr3_lag_dlb_member_attr_get(int unit,
        bcm_module_t mod, bcm_port_t port,
        int *scaling_factor, int *load_weight);
extern int bcm_tr3_lag_dlb_member_status_set(int unit, bcm_module_t mod,
        bcm_port_t port, int status);
extern int bcm_tr3_lag_dlb_member_status_get(int unit, bcm_module_t mod,
        bcm_port_t port, int *status);
extern int bcm_tr3_lag_dlb_ethertype_set(int unit, uint32 flags,
        int ethertype_count, int *ethertype_array);
extern int bcm_tr3_lag_dlb_ethertype_get(int unit, uint32 *flags,
        int ethertype_max, int *ethertype_array, int *ethertype_count);
extern int bcm_tr3_lag_dlb_config_set(int unit, bcm_switch_control_t type,
                                      int arg);
extern int bcm_tr3_lag_dlb_config_get(int unit, bcm_switch_control_t type,
                                      int *arg);
extern int bcm_tr3_lag_dlb_dynamic_size_encode(int dynamic_size,
        int *encoded_value);

extern int bcm_tr3_hg_dlb_ethertype_set(int unit, uint32 flags,
        int ethertype_count, int *ethertype_array);
extern int bcm_tr3_hg_dlb_ethertype_get(int unit, uint32 *flags,
        int ethertype_max, int *ethertype_array, int *ethertype_count);
#endif /* BCM_TRIUMPH3_SUPPORT */

#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_trunk_reinit(int unit);
#if defined (BCM_XGS3_SWITCH_SUPPORT)
extern int _xgs3_trunk_reinit(int unit, int ngroups_fp, trunk_private_t *);
extern int _xgs3_trunk_fabric_reinit(int unit, int min_tid, int max_tid,
                                     trunk_private_t *);

#if defined(BCM_TRX_SUPPORT) || defined(BCM_BRADLEY_SUPPORT)
extern void _bcm_xgs3_hw_failover_flags_set(int unit, bcm_trunk_t tid,
                                            int port_index, int hg_trunk,
                                            uint8 flags);
extern uint8 _bcm_xgs3_hw_failover_flags_get(int unit, bcm_trunk_t tid,
                                             int port_index, int hg_trunk);
#endif /* BCM_TRX_SUPPORT || BCM_BRADLEY_SUPPORT */

extern void _bcm_xgs3_trunk_member_info_set(int unit, bcm_trunk_t tid,
                                uint8 num_ports,
                                uint16 modport[BCM_TRUNK_MAX_PORTCNT],
                                uint32 member_flags[BCM_TRUNK_MAX_PORTCNT]);
extern void _bcm_xgs3_trunk_member_info_get(int unit, bcm_trunk_t tid,
                                uint8 *num_ports,
                                uint16 modport[BCM_TRUNK_MAX_PORTCNT],
                                uint32 member_flags[BCM_TRUNK_MAX_PORTCNT]);

#ifdef BCM_TRIDENT_SUPPORT
extern int _bcm_trident_trunk_mod_port_map_reinit(int unit, bcm_module_t mod,
                                         int base_index, int num_ports); 
extern int _bcm_trident_trunk_lag_reinit(int unit, int ngroups_fp,
                                         trunk_private_t *);
extern int _bcm_trident_trunk_fabric_reinit(int unit, int min_tid, int max_tid,
                                     trunk_private_t *);
extern void _bcm_trident_hw_failover_num_ports_set(int unit, bcm_trunk_t tid,
                                         int hg_trunk, uint16 num_ports);
extern uint16 _bcm_trident_hw_failover_num_ports_get(int unit, bcm_trunk_t tid,
                                         int hg_trunk);
extern int _bcm_trident_hw_failover_flags_set(int unit, bcm_trunk_t tid,
                                         int port_index, int hg_trunk,
                                         uint8 flags);
extern uint8 _bcm_trident_hw_failover_flags_get(int unit, bcm_trunk_t tid,
                                         int port_index, int hg_trunk);
extern int _bcm_trident_trunk_member_info_set(int unit, bcm_trunk_t tid,
                                uint16 num_ports,
                                uint16 *modport,
                                uint32 *member_flags);
extern int _bcm_trident_trunk_member_info_get(int unit, bcm_trunk_t tid,
                                uint16 array_size,
                                uint16 *modport,
                                uint32 *member_flags,
                                uint16 *num_ports);
extern int bcm_trident_hg_dlb_wb_alloc_size_get(int unit, int *size);
extern int bcm_trident_hg_dlb_sync(int unit, uint8 **scache_ptr);
extern int bcm_trident_hg_dlb_scache_recover(int unit, uint8 **scache_ptr);
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
extern int bcm_tr3_lag_dlb_wb_alloc_size_get(int unit, int *size);
extern int bcm_tr3_lag_dlb_sync(int unit, uint8 **scache_ptr);
extern int bcm_tr3_lag_dlb_scache_recover(int unit, uint8 **scache_ptr);
extern int bcm_tr3_lag_dlb_member_recover(int unit);
extern int bcm_tr3_lag_dlb_group_recover(int unit, bcm_trunk_t tid,
                                         trunk_private_t *trunk_info);
extern int bcm_tr3_lag_dlb_quality_parameters_recover(int unit);
#endif /* BCM_TRIUMPH3_SUPPORT */

#endif /* BCM_XGS3_SWITCH_SUPPORT */
#else
#define _bcm_trunk_reinit(unit)     (BCM_E_NONE)
#if defined (BCM_XGS3_SWITCH_SUPPORT)
#define  _xgs3_trunk_reinit(unit, ng_fp, tp)            (BCM_E_UNAVAIL)
#define  _xgs3_trunk_fabric_reinit(unit, min, max, tp)  (BCM_E_UNAVAIL)
#endif /* BCM_XGS3_SWITCH_SUPPORT */
#endif /* BCM_WARM_BOOT_SUPPORT */

extern int _bcm_esw_trunk_lock(int unit);
extern int _bcm_esw_trunk_unlock(int unit);
extern int _bcm_esw_trunk_group_num(int unit);

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_trunk_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_trunk_sw_dump(int unit);
#if defined(BCM_XGS3_SWITCH_SUPPORT)
extern void _bcm_xgs3_trunk_sw_dump(int unit);

#ifdef BCM_TRIDENT_SUPPORT
extern void _bcm_trident_trunk_sw_dump(int unit);
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
extern void bcm_tr3_lag_dlb_sw_dump(int unit);
#endif /* BCM_TRIUMPH3_SUPPORT */

#endif /* BCM_XGS3_SWITCH_SUPPORT */

#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

extern int _bcm_trunk_id_validate(int unit, bcm_trunk_t tid);
extern int _bcm_esw_trunk_local_members_get(int unit, bcm_trunk_t trunk_id, 
                                            int local_member_max, 
                                            bcm_port_t *local_member_array, 
                                            int *local_member_count);
extern int _bcm_esw_trunk_id_is_vp_lag(int unit, bcm_trunk_t tid,
        int *tid_is_vp_lag);
extern int _bcm_esw_trunk_tid_to_vp_lag_vp(int unit, bcm_trunk_t tid,
        int *vp_lag_vp);
extern int _bcm_esw_trunk_vp_lag_vp_to_tid(int unit, int vp_lag_vp,
        bcm_trunk_t *tid);

#endif	/* !_BCM_INT_TRUNK_H */
