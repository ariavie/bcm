/*
 * $Id: l2.h,v 1.30 Broadcom SDK $
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
 */

#ifndef _BCM_INT_L2_H
#define _BCM_INT_L2_H

#include <bcm/types.h>
#include <bcm/l2.h>

#include <sal/core/sync.h>

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_l2_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */


#define     L2_MEM_CHUNKS_DEFAULT   100
typedef int (*_bcm_l2_traverse_int_cb)(int unit, void *trav_st);

typedef struct _bcm_l2_traverse_s {
    uint32                      *data;      /* L2 Entry */
    soc_mem_t                   mem;        /* Traversed memory */
    int                         mem_idx;    /* Index of currently read entry */
    bcm_l2_traverse_cb          user_cb;    /* User callback function */
    void                        *user_data; /* Data provided by the user, cookie */
    _bcm_l2_traverse_int_cb     int_cb;     /* Internal callback function */
}_bcm_l2_traverse_t;

extern int _bcm_esw_l2_traverse(int unit, _bcm_l2_traverse_t *trav_st);

typedef struct _bcm_l2_replace_dest_s {
    bcm_module_t        module;
    bcm_port_t          port;
    bcm_trunk_t         trunk;
    int                 vp;
} _bcm_l2_replace_dest_t;

/* _bcm_l2_replaces_t.int_flags (internal flags) */
#define _BCM_L2_REPLACE_INT_NO_ACTION               0x0001
#define _BCM_L2_REPLACE_INT_MATCH_DISCARD_SRC       0x0002
#define _BCM_L2_REPLACE_INT_MATCH_SRC_HIT           0x0004
#define _BCM_L2_REPLACE_INT_MATCH_DES_HIT           0x0008
#define _BCM_L2_REPLACE_INT_MATCH_NATIVE            0x0010

typedef struct _bcm_l2_replace_s {
    uint32              flags;           /* BCM_L2_REPLACE_XXX */
    uint32              int_flags;       /* _BCM_L2_REPLACE_INT_XXX */
    uint32              key_l2_flags;    /* bcm_l2_addr_t.flags */
    int                 key_type;
    bcm_mac_t           key_mac;
    bcm_vlan_t          key_vlan;
    int                 key_vfi;
    _bcm_l2_replace_dest_t match_dest;
    _bcm_l2_replace_dest_t new_dest;
    l2x_entry_t         match_data;
    l2x_entry_t         match_mask;
    l2x_entry_t         new_data;
    l2x_entry_t         new_mask;
    bcm_l2_traverse_cb  trav_fn;
    void               *user_data;
} _bcm_l2_replace_t;

typedef struct _bcm_l2_match_s {
    bcm_l2_addr_t *match_addr; 
    bcm_l2_addr_t *addr_mask; 
    bcm_l2_traverse_cb trav_fn; 
    void *user_data;
} _bcm_l2_match_t;


#define _BCM_L2_MATCH_ENTRY_BUF_SIZE      (16 * 1024)
#define _BCM_L2_MATCH_ENTRY_BUF_COUNT     32
typedef struct _bcm_l2_match_ctrl_s {
    sal_sem_t sem;
    int entry_wr_idx;
    l2x_entry_t *l2x_entry_buf[_BCM_L2_MATCH_ENTRY_BUF_COUNT];
    int preserved;
} _bcm_l2_match_ctrl_t;

typedef struct  _bcm_l2_gport_params_s {
    int     param0;
    int     param1;
    uint32  type;
}_bcm_l2_gport_params_t;

extern int _l2_init[BCM_MAX_NUM_UNITS];
extern  _bcm_l2_match_ctrl_t *_bcm_l2_match_ctrl[BCM_MAX_NUM_UNITS];
/*
 * Define:
 *	L2_INIT
 * Purpose:
 *	Causes a routine to return BCM_E_INIT (or some other
 *	error) if L2 software module is not yet initialized.
 */

#define L2_INIT(unit) do { \
	if (_l2_init[unit] < 0) return _l2_init[unit]; \
	if (_l2_init[unit] == 0) return BCM_E_INIT; \
	} while (0);

extern int _bcm_esw_l2_gport_parse(int unit, bcm_l2_addr_t *l2addr, 
                                   _bcm_l2_gport_params_t *params);
extern int _bcm_esw_l2_gport_construct(int unit, bcm_l2_addr_t *l2addr, 
                                       _bcm_l2_gport_params_t *params);

extern int _bcm_get_op_from_flags(uint32 flags, uint32 *op, uint32 *sync_op);
extern int _bcm_esw_l2_from_l2x(int unit, soc_mem_t mem, bcm_l2_addr_t *l2addr,
                                uint32 *l2_entry);
extern void _bcm_l2_register_callback(int unit, int flags, l2x_entry_t *entry_del,
                                      l2x_entry_t *entry_add, void *fn_data);

/* extended aging per_port_age_control mode field values */
#define	XGS_PPAMODE_PORTMOD		            0x0
#define	XGS_PPAMODE_VLAN		            0x1
#define	XGS_PPAMODE_PORTMOD_VLAN	        0x2
#define	XGS_PPAMODE_RSVD		            0x3
#define	XGS_PPAMODE_PORTMOD_REPLACE		    0x4	
#define	XGS_PPAMODE_VLAN_REPLACE		    0x5	
#define	XGS_PPAMODE_PORTMOD_VLAN_REPLACE	0x6	

#define BCMSIM_L2XMSG_INTERVAL 60000000

/* L2 Learn class defines */
#define _BCM_L2_LEARN_CLASSID_MIN      0
#define _BCM_L2_LEARN_CLASSID_MAX      3
#define BCM_L2_LEARN_CLASSID_VALID(_cid_) do { \
        if ((_cid_ < _BCM_L2_LEARN_CLASSID_MIN) || \
        (_cid_ > _BCM_L2_LEARN_CLASSID_MAX)) { return BCM_E_PARAM; } \
        } while(0);

#if defined(BCM_TRIUMPH_SUPPORT) /* BCM_TRIUMPH_SUPPORT */
typedef struct _bcm_l2_station_entry_s {
    int                     sid;        /* Entry identifier.             */
    int                     prio;       /* Entry priority.               */
    uint32                  hw_index;   /* Entry hardware index.         */
    uint32                  flags;      /* Entry flags.                  */
    uint32                  *tcam_ent;  /* TCAM entry.                   */
} _bcm_l2_station_entry_t;

typedef struct _bcm_l2_station_control_s {
    sal_mutex_t             sc_lock;
    _bcm_l2_station_entry_t **entry_arr;    /* L2 station entry array.  */
    int                     entries_total;  /* Total number of entries. */
    int                     entries_free;   /* Count of unused entries. */
    int                     entry_count;    /* Count of used entries.   */
#ifdef BCM_KATANA2_SUPPORT
    int                     port_entries_total; /* Count of total Port MAC 
                                                                entries */
    int                     port_entries_free; /* Count of total PORT MAC
                                                                 entries */
    int                     olp_entries_total;  /* Count of total OLP MAC 
                                                                entries */
    int                     olp_entries_free;   /* Count of total OLP MAC 
                                                                entries */
#endif
} _bcm_l2_station_control_t;
#endif /* !BCM_TRIUMPH_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT)

    typedef struct l2_count_s {
        int l2_entry_1;
        int l2_entry_2;
        int ext_l2_entry_1;
        int ext_l2_entry_2;
    } l2_count_t;

    typedef struct l2_count_data_s {
        uint32     flags;
        l2_count_t l2_count;
    } l2_count_data_t;

#define L2_DUMP_COUNT 1<<0

extern l2_count_data_t _l2_addr_counts;

#endif

#endif	/* !_BCM_INT_L2_H */
