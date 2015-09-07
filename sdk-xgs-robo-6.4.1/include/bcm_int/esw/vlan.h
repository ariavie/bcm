/*
 * $Id: vlan.h,v 1.44 Broadcom SDK $
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
 * This file contains VLAN definitions internal to the BCM library.
 */

#ifndef _BCM_INT_VLAN_H
#define _BCM_INT_VLAN_H

#include <sal/types.h>
#include <bcm/types.h>
#include <bcm/vlan.h>

/*
 * Define:
 *	VLAN_CHK_ID
 * Purpose:
 *	Causes a routine to return BCM_E_PARAM if the specified
 *	VLAN ID is out of range.
 */

#define VLAN_CHK_ID(unit, vid) do { \
	if (vid > BCM_VLAN_MAX) return BCM_E_PARAM; \
	} while (0);

/*
 * Define:
 *	VLAN_CHK_PRIO
 * Purpose:
 *	Causes a routine to return BCM_E_PARAM if the specified
 *	priority (802.1p CoS) is out of range.
 */

#define VLAN_CHK_PRIO(unit, prio) do { \
	if ((prio < BCM_PRIO_MIN || prio > BCM_PRIO_MAX)) return BCM_E_PARAM; \
	} while (0);

#define VLAN_CHK_ACTION(unit, action) do { \
	if (action < bcmVlanActionNone || action > bcmVlanActionCopy) return BCM_E_PARAM; \
	} while (0);

/*
 * XGS3 Vlan Translate macros
 */
#ifdef BCM_XGS3_SWITCH_SUPPORT
#define BCM_VLAN_XLATE_PRIO_MASK        0x007
#define BCM_VLAN_XLATE_MODID_MASK       0x3f0
#define BCM_VLAN_XLATE_INGRESS_MASK     0x400
#define BCM_VLAN_XLATE_ADDVID_MASK      0x800

#define BCM_VLAN_XLATE_MODID_SHIFT      4

#define BCM_VLAN_XLATE_ING              0
#define BCM_VLAN_XLATE_EGR              1
#define BCM_VLAN_XLATE_DTAG             2
#endif

#define _BCM_VLAN_PORT_ADD              1
#define _BCM_VLAN_PORT_DELETE           0

/*
 * VLAN internal struct used for book keeping
 */
typedef struct vbmp_s {
    SHR_BITDCL  *w;
} vbmp_t;

extern int _bcm_esw_vlan_stk_update(int unit, uint32 flags);
extern int _bcm_esw_vlan_flood_default_set(int unit, bcm_vlan_mcast_flood_t mode);
extern int _bcm_esw_vlan_flood_default_get(int unit, bcm_vlan_mcast_flood_t *mode);
extern int _bcm_esw_higig_flood_l2_set(int unit, bcm_vlan_mcast_flood_t mode);
extern int _bcm_esw_higig_flood_l2_get(int unit, bcm_vlan_mcast_flood_t *mode);
extern int _bcm_esw_higig_flood_l3_set(int unit, bcm_vlan_mcast_flood_t mode);
extern int _bcm_esw_higig_flood_l3_get(int unit, bcm_vlan_mcast_flood_t *mode);
extern int bcm_esw_vlan_detach(int unit);
extern int _bcm_esw_vlan_xlate_key_type_value_get(int unit, int key_type,
                                                  int *key_value);
extern int _bcm_esw_vlan_xlate_key_type_get(int unit, int key_type_value, 
                                            int *key_type);
extern int _bcm_esw_pt_vtkey_type_value_get(int unit,
                     int key_type, int *key_type_value);
extern int _bcm_esw_pt_vtkey_type_get(int unit,
                     int key_type_value, int *key_type);

#if defined(BCM_TRIUMPH3_SUPPORT)
extern int _bcm_tr3_vxlate_extd2vxlate(int unit,vlan_xlate_extd_entry_t *vxxent,
                vlan_xlate_entry_t *vxent, int use_svp);
extern int _bcm_tr3_vxlate2vxlate_extd(int unit, vlan_xlate_entry_t *vxent,
                vlan_xlate_extd_entry_t *vxxent);
#endif

/*
 * The entire vlan_info structure is protected by BCM_LOCK.
 */

typedef struct bcm_vlan_info_s {
    int                    init;      /* TRUE if VLAN module has been inited */
    bcm_vlan_t             defl;     /* Default VLAN */
    vbmp_t                 bmp;       /* Bitmap of existing VLANs */
    int                    count;     /* Number of existing VLANs */
    /* Number of vlan egress xlate entires used to blok port_class settings */
    uint16                 old_egr_xlate_cnt; 
    bcm_vlan_mcast_flood_t flood_mode;/* Default flood mode */
    uint32                 *ing_trans;  /* Ingress Vlan Translate info */
    uint32                 *egr_trans;  /* Egress Vlan Translate info */
#if defined(BCM_EASY_RELOAD_SUPPORT)
    vbmp_t                 egr_vlan_bmp; /* Bitmap of valid EGR_VLAN */
    vbmp_t                 vlan_tab_bmp; /* Bitmap of valid VLAN_TAB */
#endif
    SHR_BITDCL             *qm_bmp;   /* Bitmap of allocated queue map */
    SHR_BITDCL             *qm_it_bmp;/* Bitmap of queue map entry using inner
                                         tag */
    vbmp_t                 pre_cfg_bmp; /* per vlan pre-configuration flag */ 
    SHR_BITDCL vp_mode[_SHR_BITDCLSIZE(BCM_VLAN_COUNT)]; /* VP control mode */
} bcm_vlan_info_t;

extern bcm_vlan_info_t vlan_info[BCM_MAX_NUM_UNITS];

typedef struct _bcm_vlan_translate_data_s{
    bcm_port_t      port;
    bcm_vlan_t      old_vlan;
    bcm_vlan_t      *new_vlan;
    int             *prio;
}_bcm_vlan_translate_data_t;


/*
 * Vlan Translate support routines
 */

#define BCM_VTCACHE_VALID_SET(_c, _valid)       _c |= ((_valid & 1) << 31)
#define BCM_VTCACHE_ADD_SET(_c, _add)           _c |= ((_add & 1) << 30)
#define BCM_VTCACHE_PORT_SET(_c, _port)         _c |= ((_port & 0xff) << 16)
#define BCM_VTCACHE_VID_SET(_c, _vid)           _c |= ((_vid & 0xffff) << 0)

#define BCM_VTCACHE_VALID_GET(_c)               ((_c >> 31) & 1)
#define BCM_VTCACHE_ADD_GET(_c)                 ((_c >> 30) & 1)
#define BCM_VTCACHE_PORT_GET(_c)                ((_c >> 16) & 0xff)
#define BCM_VTCACHE_VID_GET(_c)                 ((_c >> 0) & 0xffff)

#define VLAN_MEM_CHUNKS_DEFAULT                 256


typedef struct _translate_action_range_traverse_cb_s {
    bcm_vlan_translate_action_range_traverse_cb usr_cb;
} _translate_action_range_traverse_cb_t;

typedef struct _translate_range_traverse_cb_s {
    bcm_vlan_translate_range_traverse_cb usr_cb;
} _translate_range_traverse_cb_t;

typedef struct _dtag_range_traverse_cb_s {
    bcm_vlan_dtag_range_traverse_cb usr_cb;
} _dtag_range_traverse_cb_t;

typedef struct _translate_action_traverse_cb_s {
    bcm_vlan_translate_action_traverse_cb usr_cb;
} _translate_action_traverse_cb_t;

typedef struct _translate_egress_action_traverse_cb_s {
    bcm_vlan_translate_egress_action_traverse_cb usr_cb;
} _translate_egress_action_traverse_cb_t;

typedef struct _translate_traverse_cb_s {
    bcm_vlan_translate_traverse_cb usr_cb;
} _translate_traverse_cb_t;

typedef struct _translate_egress_traverse_cb_s {
    bcm_vlan_translate_egress_traverse_cb usr_cb;
} _translate_egress_traverse_cb_t;

typedef struct _dtag_traverse_cb_s {
    bcm_vlan_dtag_traverse_cb usr_cb;
} _dtag_traverse_cb_t;


typedef int (*_bcm_vlan_translate_traverse_int_cb)( int unit, void *trv_info, 
                                                    int *stop);

typedef struct _bcm_vlan_translate_traverse_s {
    _bcm_vlan_translate_traverse_int_cb int_cb;
    void                                *user_cb_st;
    void                                *user_data;
    bcm_vlan_action_set_t               *action;
    bcm_gport_t                         gport;
    uint32                              key_type;
    int                                 port_class;
    bcm_vlan_t                          outer_vlan;
    bcm_vlan_t                          inner_vlan;
    bcm_vlan_t                          outer_vlan_high;
    bcm_vlan_t                          inner_vlan_high;
}_bcm_vlan_translate_traverse_t;

extern int _bcm_esw_vlan_translate_action_traverse_int_cb(int unit, 
                                                         void* trv_info,
                                                          int *stop);
extern int _bcm_esw_vlan_translate_egress_action_traverse_int_cb(int unit, 
                                                                void* trv_info,
                                                                 int *stop);
extern int _bcm_esw_vlan_translate_traverse_int_cb(int unit, void* trv_info, 
                                                   int *stop);
extern int _bcm_esw_vlan_translate_egress_traverse_int_cb(int unit, 
                                                          void* trv_info,
                                                          int *stop);
extern int _bcm_esw_vlan_dtag_traverse_int_cb(int unit, void* trv_info, 
                                              int *stop);
extern int _bcm_esw_vlan_translate_traverse_mem(int unit, soc_mem_t mem, 
                                   _bcm_vlan_translate_traverse_t *trvs_info);

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_vlan_cleanup(int unit);
#else
#define _bcm_vlan_cleanup(u)   (BCM_E_NONE)
#endif /* BCM_WARM_BOOT_SUPPORT */


extern int _bcm_esw_vlan_untag_update(int unit, bcm_port_t port, int untag);

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_vlan_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_vlan_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
extern int _bcm_vlan_valid_check(int unit, int table, vlan_tab_entry_t *vt, 
                                 bcm_vlan_t vid); 
#endif

#if defined(BCM_EASY_RELOAD_SUPPORT)
int _bcm_vlan_valid_set(int unit, int table, bcm_vlan_t vid, int val);
#endif

extern int _bcm_esw_vlan_port_source_vp_lag_set(int unit, bcm_gport_t gport,
        int vp_lag_vp);
extern int _bcm_esw_vlan_port_source_vp_lag_clear(int unit, bcm_gport_t gport,
        int vp_lag_vp);
extern int _bcm_esw_vlan_port_source_vp_lag_get(int unit, bcm_gport_t gport,
        int *vp_lag_vp);

extern int _bcm_esw_vlan_system_reserved_get(int unit, int *arg);
extern int _bcm_esw_vlan_system_reserved_set(int unit, int arg);

extern int _bcm_vlan_count_get(int unit, int *arg);

#endif	/* !_BCM_INT_VLAN_H */
