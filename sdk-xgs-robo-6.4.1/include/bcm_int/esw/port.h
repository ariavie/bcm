/*
 * $Id: port.h,v 1.78 Broadcom SDK $
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
 * This file contains PORT definitions internal to the BCM library.
 */

#ifndef _BCM_INT_PORT_H
#define _BCM_INT_PORT_H

#include <bcm/port.h>

#define  BCM_PORT_L3_V4_ENABLE     (1 << 0)    /* Enable L3 IPv4 switching. */
#define  BCM_PORT_L3_V6_ENABLE     (1 << 1)    /* Enable L3 IPv6 switching. */

#define  BCM_PORT_JUMBO_MAXSZ    0x3FE8

#define BCM_EGR_PORT_TYPE_ETHERNET      0x0
#define BCM_EGR_PORT_TYPE_HIGIG         0x1
#define BCM_EGR_PORT_TYPE_LB            0x2
#define BCM_EGR_PORT_TYPE_EHG           0x3

#define BCM_EGR_PORT_HIGIG_PLUS         0x0
#define BCM_EGR_PORT_HIGIG2             0x1

#define _BCM_PORT_EHG_L2_HEADER_BUFFER_SZ       8    /* 24 bytes  = 6 words + 2 words padding*/
#define _BCM_PORT_EHG_IP_GRE_HEADER_BUFFER_SZ   12    /* 48 bytes  = 12 words */

#define _BCM_CPU_TABS_NONE        0
#define _BCM_CPU_TABS_ETHER     0x1
#define _BCM_CPU_TABS_HIGIG     0x2
#define _BCM_CPU_TABS_BOTH      0x3

#define _BCM_PORT_RATE_PPS_MODE  1  /* Rate Limiting in packet mode */
#define _BCM_PORT_RATE_BYTE_MODE 0  /* Rate Limiting in byte mode   */

/*
 * Define:
 *     LPORT_PROFILE_LPORT_TAB/LPORT_PROFILE_RTAG7_TAB
 * Purpose:
 *     Table indexes for LPORT Profile Table.
 */
#define LPORT_PROFILE_LPORT_TAB 0
#define LPORT_PROFILE_RTAG7_TAB 1

/*
 * Define:
 *      _BCM_PORT_VD_PBVL_ESIZE
 * Purpose:
 *      Number of bits per entry in vd_pbvl bitmap
 */

#define _BCM_PORT_VD_PBVL_ESIZE      8

/*
 * Define:
 *      _BCM_PORT_VD_PBVL_SET, _BCM_PORT_VD_PBVL_CLEAR
 * Purpose:
 *      Set/clear bit in vp_pbvl bitmap for unit, port, index
 */
#define _BCM_PORT_VD_PBVL_SET(_pinfo, _idx) \
        (_pinfo)->p_vd_pbvl[(_idx) / _BCM_PORT_VD_PBVL_ESIZE] |= \
                (1 << ((_idx) % _BCM_PORT_VD_PBVL_ESIZE))

#define _BCM_PORT_VD_PBVL_CLEAR(_pinfo, _idx) \
        (_pinfo)->p_vd_pbvl[(_idx)/ _BCM_PORT_VD_PBVL_ESIZE] &= \
                ~(1 << ((_idx) % _BCM_PORT_VD_PBVL_ESIZE))

/*
 * Define:
 *      PORT_VD_PBVL_IS_SET
 * Purpose:
 *      Test if bit in vp_pbvl bitmap is set for unit, port, index
 */
#define _BCM_PORT_VD_PBVL_IS_SET(_pinfo, _idx) \
        (((_pinfo)->p_vd_pbvl[(_idx) / _BCM_PORT_VD_PBVL_ESIZE] \
                >> ((_idx) % _BCM_PORT_VD_PBVL_ESIZE)) & 1)

#define _PORT_INFO_STOP_TX              1
#define _PORT_INFO_PROBE_RESET_SERDES   2

/*
 * Define:
 *      PORT_LOCK/PORT_UNLOCK
 * Purpose:
 *      Serialization Macros.
 *      Here the cmic memory lock also serves to protect the
 *      bcm_port_info structure and EPC_PFM registers.
 */

#define PORT_LOCK(unit) \
        BCM_LOCK(unit); \
        if (soc_mem_is_valid(unit, PORT_TABm)) \
        { soc_mem_lock(unit, PORT_TABm); }


#define PORT_UNLOCK(unit) \
        BCM_UNLOCK(unit); \
        if (soc_mem_is_valid(unit, PORT_TABm)) \
        { soc_mem_unlock(unit, PORT_TABm); }

typedef struct _bcm_port_metering_info_s {
    uint32 refresh;
    uint32 threshold;
    uint32 bucket;
    uint8  in_profile;
} _bcm_port_metering_info_t; 

/*
 * The port structure and hardware PTABLE are protected by the PORT_LOCK.
 */
typedef struct _bcm_port_info_s {
    mac_driver_t       *p_mac;          /* Per port MAC driver */
    int                 p_ut_prio;      /* Untagged priority   */
    uint8               *p_vd_pbvl;     /* Proto-based VLAN_DATA state */
    int                 dtag_mode;      /* Double-tag mode */
    int                 vlan_prot_ptr;  /* VLAN_PROTOCOL_DATA index */
    int                 vp_count;       /* Number of VPs on a physical port */
    bcm_port_congestion_config_t *e2ecc_config; 
                                        /* End-to-end congestion control configuration */ 
    uint32              cosmask;        /* Cached h/w value on a port stop */
    _bcm_port_metering_info_t m_info;   /* Cached h/w value on a port stop */
    uint8               flags;          /* Bits to indicate port stop etc */
    uint8               rx_los;         /* Cached PHY serdes rx_los status */
    int                 enable;         /* Cached port enable status */   	
} _bcm_port_info_t;

/*
 * Internal port configuration enum.
 * Used internally by sdk to set port configuration from 
 */
typedef enum _bcm_port_config_e {
   _bcmPortL3UrpfMode,          /* L3 unicast packet RPF check mode.  */
   _bcmPortL3UrpfDefaultRoute,  /* UPRF check accept default gateway. */
   _bcmPortVlanTranslate,       /* Enable Vlan translation on the port. */
   _bcmPortVlanPrecedence,      /* Set Vlan lookup precedence between
                                 * IP table and MAC table             */
   _bcmPortVTMissDrop,          /* Drop if VLAN translate miss.       */
   _bcmPortLookupMACEnable,     /* Enable MAC-based VLAN              */
   _bcmPortLookupIPEnable,      /* Enable subnet-based VLAN           */
   _bcmPortUseInnerPri,         /* Trust the inner vlan tag PRI bits  */
   _bcmPortUseOuterPri,         /* Trust the outer vlan tag PRI bits  */
   _bcmPortVerifyOuterTpid,     /* Verify outer TPID against VLAN TPID*/
   _bcmPortVTKeyTypeFirst,      /* First Vlan Translation Key Type    */
   _bcmPortVTKeyPortFirst,      /* First Per-port VT Key Type         */
   _bcmPortVTKeyTypeSecond,     /* Second Vlan Translation Key Type   */
   _bcmPortVTKeyPortSecond,     /* Second Per-port VT Key Type        */
   _bcmPortIpmcV4Enable,        /* Enable IPv4 IPMC processing        */
   _bcmPortIpmcV6Enable,        /* Enable IPv4 IPMC processing        */
   _bcmPortIpmcVlanKey,         /* Include VLAN in L3 host key for IPMC */
   _bcmPortCfiAsCng,            /* Configure CFI to CNG mapping       */
   _bcmPortNni,                 /* Set port as NNI                    */
   _bcmPortHigigTrunkId,        /* Higig trunk, -1 to disable         */
   _bcmPortModuleLoopback,      /* Allow loopback to same modid       */
   _bcmPortOuterTpidEnables,    /* Bitmap of outer TPID valid bits    */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
   _bcmPortSvcMeterIndex,       /* Service Meter(global meter) index  */
   _bcmPortSvcMeterOffsetMode,  /* Service Meter offset mode          */
#endif
#if defined(BCM_KATANA2_SUPPORT)
   _bcmPortOamUseXlatedInnerVlan, /* Use Xlated C vlan for OAM lookup  */
#endif
   _bcmPortCount                /* Always last max value of the enum. */
} _bcm_port_config_t;

/*
 * Port configuration structure.
 * An abstraction of data in the PTABLE (strata) and other places.
 */

typedef struct bcm_port_cfg_s {
    int		pc_frame_type;
    int		pc_ether_type;

    int		pc_stp_state;	  /* spanning tree state of port */
    int		pc_cpu;		  /* CPU learning */
    int		pc_disc;	  /* discard state */
    int		pc_bpdu_disable;  /* Where is this in Draco? */
    int		pc_trunk;	  /* trunking on for this port */
    int		pc_tgid;	  /* trunk group id */
    int		pc_mirror_ing;	  /* mirror on ingress */
    int		pc_ptype;	  /* port type */
    int		pc_jumbo;
    int		pc_cml;		  /* CML bits */
    int     pc_cml_move;  /* CML move bits on supporting devices */  

    bcm_pbmp_t	pc_pbm;		  /* port bitmaps for port based vlan */
    bcm_pbmp_t	pc_ut_pbm;
    bcm_vlan_t	pc_vlan;	  /* port based vlan tag */
    bcm_vlan_t	pc_ivlan;	  /* port based inner-tag vlan tag */
    int         pc_vlan_action;   /* port based vlan action profile pointer */

#if defined(BCM_STRATA2_SUPPORT)
    int		pc_stg;		  /* stg value */
    int		pc_rsvd1;	  /* Reserved field (must be ones) */
#endif

    int		pc_l3_flags;	  /* L3 flags. */

#if defined(BCM_STRATA2_SUPPORT) || defined(BCM_XGS_SWITCH_SUPPORT)
    int		pc_remap_pri_en;  /* remap priority enable */
#endif

    int	        pc_new_opri;      /* new outer packet priority */
    int	        pc_new_ocfi;      /* new outer cfi */
    int	        pc_new_ipri;      /* new inner packet priority */
    int	        pc_new_icfi;      /* new inner cfi */

    int		pc_dse_mode;	  /* DSCP mapping (off, or on/mode) */
    int		pc_dscp;	  /* Resultant diffserv code point */

#if defined(BCM_XGS_SWITCH_SUPPORT)
    int         pc_en_ifilter;    /* Enable Ingress Filtering */
    int         pc_pfm;           /* In the port table for Draco */
    int         pc_dscp_prio;     /* For Draco15 & Tucana */
    int         pc_bridge_port;   /* FB/ER, allows egress=ingress */
    int         pc_nni_port;      /* FB, indicates non-customer port */
#endif

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    int     pc_urpf_mode;         /* Unicast rpf lookup mode.      */
    int     pc_urpf_def_gw_check; /* Unicast rpf check default gw. */ 
    int     pc_pvlan_enable;      /* Private (force) vlan enable */
#endif

} bcm_port_cfg_t;

typedef struct _bcm_gport_dest_s {
    bcm_port_t      port;
    bcm_module_t    modid;
    bcm_trunk_t     tgid;
    int             mpls_id;
    int             mim_id;
    int             wlan_id;
    int             trill_id;
    int             l2gre_id;
    int             vxlan_id;
    int             vlan_vp_id;
    int             niv_id;
    int             extender_id;
    int             subport_id;
    int             scheduker_id;
    uint32          gport_type;
} _bcm_gport_dest_t;

typedef struct _bcm_port_egr_dest_s {
    bcm_port_t      in_port;
    bcm_module_t    in_modid;
    bcm_port_t      out_min_port;
    bcm_module_t    out_min_modid;
    bcm_port_t      out_max_port;
    bcm_module_t    out_max_modid;
} _bcm_port_egr_dest_t;

enum NIV_VNTAG_ACTIONS {
    VNTAG_NOOP = 0,
    VNTAG_ADD = 1,
    VNTAG_REPLACE = 2,
    VNTAG_DELETE = 3,
    VNTAG_ACTION_COUNT
};

extern int _bcm_esw_port_mon_start(int unit);
extern int _bcm_esw_port_mon_stop(int unit);
extern int _bcm_esw_ibod_sync_recovery_start(int unit, sal_usecs_t interval);
extern int _bcm_esw_ibod_sync_recovery_stop(int unit);
extern int _bcm_esw_ibod_sync_recovery_running(int unit,
                                               sal_usecs_t *interval,
                                               uint64 *event_count);
extern int _bcm_esw_port_deinit(int unit);
extern int _bcm_port_probe(int unit, bcm_port_t p, int *okay);
extern int _bcm_port_mirror_enable_set(int unit, bcm_port_t port,
                                       int enable);
extern int _bcm_port_mirror_enable_get(int unit, bcm_port_t port,
                                       int *enable);
extern int _bcm_port_mirror_egress_true_enable_set(int unit, bcm_port_t port,
                                                   int enable);
extern int _bcm_port_mirror_egress_true_enable_get(int unit, bcm_port_t port,
                                                   int *enable);
extern int _bcm_port_link_get(int unit, bcm_port_t port, int hw, int *link);
extern int _bcm_port_pd10_handler(int unit, bcm_port_t port, int link_up);
extern int _bcm_port_untagged_vlan_get(int unit, bcm_port_t port,
				bcm_vlan_t *vid_ptr);
extern int _bcm_esw_port_config_set(int unit, bcm_port_t port, 
                                    _bcm_port_config_t type, int value);
extern int _bcm_esw_port_config_get(int unit, bcm_port_t port, 
                                    _bcm_port_config_t type, int *value);
extern void _bcm_gport_dest_t_init(_bcm_gport_dest_t *gport_dest);
extern int _bcm_gport_modport_hw2api_map(int, bcm_module_t, bcm_port_t,
                                         bcm_module_t *, bcm_port_t *);
extern int _bcm_esw_gport_construct(int unit, _bcm_gport_dest_t *gport_dest, 
                                    bcm_gport_t *gport);
extern int _bcm_esw_gport_resolve(int unit, bcm_gport_t gport,
                                  bcm_module_t *modid, bcm_port_t *port, 
                                  bcm_trunk_t *trunk_id, int *id);
extern int _bcm_esw_modid_is_local(int unit, bcm_module_t modid, int *result);
extern int _bcm_port_info_get(int unit, bcm_port_t port, _bcm_port_info_t **pinfo);
extern int _bcm_esw_port_gport_validate(int unit, bcm_port_t port_in, 
                                        bcm_port_t *port_out);
extern int _bcm_port_ehg_header_write(int, bcm_port_t, uint32 *, uint32 *, 
                                          int);
extern int _bcm_port_mode_setup(int unit, bcm_port_t port, int enable);
extern void _bcm_port_info_access(int unit, bcm_port_t port, 
                                  _bcm_port_info_t **info);
extern unsigned _bcm_esw_valid_flex_port_controlling_port(int unit, bcm_port_t port);
extern int _bcm_esw_port_tab_set(int unit, bcm_port_t port, int cpu_tabs,
                      soc_field_t field, int value);
extern int _bcm_esw_port_tab_get(int unit, bcm_port_t port, 
                      soc_field_t field, int *value);
extern int
bcm_esw_port_lport_fields_set(int unit, bcm_port_t port,
                              int table_id, int field_count,
                              soc_field_t *fields, uint32 *values);
extern int
bcm_esw_port_lport_fields_get(int unit, bcm_port_t port,
                              int table_id, int field_count,
                              soc_field_t *fields, uint32 *values);
extern int
bcm_esw_port_lport_field_set(int unit, bcm_port_t port,
                             int table_id, soc_field_t field, uint32 value);
extern int
bcm_esw_port_lport_field_get(int unit, bcm_port_t port,
                             int table_id, soc_field_t field, uint32 *value);
extern int 
_bcm_esw_link_force_for_disabled_loopback_port(int unit);

extern int _bcm_esw_port_mac_failover_notify(int,bcm_port_t);
extern int _bcm_esw_port_stat_get(int unit, int sync_mode,
                                  bcm_gport_t port, bcm_port_stat_t stat,
                                  uint64 *val);
#ifdef BCM_KATANA2_SUPPORT
void bcm_esw_kt2_port_lock(int unit);
void bcm_esw_kt2_port_unlock(int unit);
#endif
#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_port_sync(int unit);
extern int _bcm_port_cleanup(int unit);
#else
#define _bcm_port_cleanup(u)        (BCM_E_NONE)
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_port_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#ifdef BCM_TRIUMPH3_SUPPORT
extern int _bcm_tr3_port_ur_chk(int unit, int num_ports, bcm_port_t port);
#endif

#endif	/* !_BCM_INT_PORT_H */
