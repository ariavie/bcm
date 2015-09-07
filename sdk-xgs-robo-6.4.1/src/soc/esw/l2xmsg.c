/*
 * $Id: l2xmsg.c,v 1.53 Broadcom SDK $
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
 * File:	l2xmsg.c
 * Purpose:	Provide a reliable stream of L2 insert/delete messages.
 *
 * This module monitors the L2X table for changes and performs callbacks
 * for each insert, delete, or port movement that is detected.
 *
 * There is a time lag from the actual table change to the callback
 * because the l2xmsg task scans the L2X table only periodically.
 */

#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <shared/bsl.h>
#include <sal/core/spl.h>
#include <sal/core/time.h>

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/l2x.h>
#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/trident2.h>
#endif
#ifdef BCM_XGS_SWITCH_SUPPORT

#define L2X_ENTRY_EQL(unit, e1, e2) \
    (!_soc_mem_cmp_l2x_sync(unit, (void *) e1, (void *) e2))

#define L2X_IS_VALID_ENTRY(_u, _mem, _e, bitf)  \
           soc_mem_field32_get((_u), (_mem), (_e), bitf)

#ifdef BCM_TRIUMPH3_SUPPORT
extern int
soc_tr3_l2x_running(int unit, uint32 *flags, sal_usecs_t *interval);
extern int
soc_tr3_l2x_start(int unit, uint32 flags, sal_usecs_t interval);
extern int
soc_tr3_l2x_stop(int unit);
#endif

STATIC void _soc_l2x_thread(void *unit_vp);

/****************************************************************************
 *
 * L2X Message Registration
 *
 ****************************************************************************/

#define L2X_CB_MAX      3

typedef struct l2x_cb_entry_s {
    soc_l2x_cb_fn       fn;
    void                *fn_data;
} l2x_cb_entry_t;

typedef struct l2x_data_s {
    l2x_cb_entry_t              cb[L2X_CB_MAX];
    int                         cb_count;
    soc_mem_t                   l2mem;
    int                         entry_bytes;
    int                         entry_words;
    uint32                      *shadow_tab;
    uint32                      *chunk_buf;
    SHR_BITDCL                  *delete_map;
    SHR_BITDCL                  *callback_map;
} l2x_data_t;

STATIC l2x_data_t l2x_data[SOC_MAX_NUM_DEVICES];

#define L2X_ENTRY_CALLBACK_SET(_u_, _index_)                \
{                                                           \
    SHR_BITSET(l2x_data[(_u_)].callback_map, (_index_));    \
    LOG_VERBOSE(BSL_LS_SOC_ARL, \
                (BSL_META_U(unit, \
                            "set_entry_callback: u:%d i=%d\n"), \
                            _u_, _index_));                             \
}

#define L2X_ENTRY_DELETED_SET(_u_, _index_)                 \
{                                                           \
    SHR_BITSET(l2x_data[(_u_)].delete_map, (_index_));      \
    LOG_VERBOSE(BSL_LS_SOC_ARL, \
                (BSL_META_U(unit, \
                            "set_entry_deleted: u:%d i=%d\n"),  \
                            _u_, _index_));                             \
}

#define L2X_IS_CPU_DELETED(_u_, _index_)                    \
            SHR_BITGET(l2x_data[(_u_)].delete_map, (_index_))

#define SOC_L2X_TGID_TRUNK_INDICATOR(unit) \
        SOC_IS_RAVEN(unit) ? 0x40 : 0x20
#define SOC_L2X_TGID_PORT_TRUNK_MASK           0x1f
#define SOC_L2X_TGID_PORT_TRUNK_MASK_HI        0x60

/*
 * Function:
 *	soc_l2x_register
 * Purpose:
 *	Register a callback routine to be notified of all inserts,
 *	deletes, and updates to the L2X table.
 * Parameters:
 *	unit - StrataSwitch unit number
 *	fn - Callback function to register
 *	fn_data - Extra data passed to callback function
 * Returns:
 *	SOC_E_NONE - Success
 *	SOC_E_MEMORY - Too many callbacks registered
 */

int
soc_l2x_register(int unit, soc_l2x_cb_fn fn, void *fn_data)
{
    l2x_data_t      *ad = &l2x_data[unit];
    int                 i;
#ifdef BCM_XGS3_SWITCH_SUPPORT
    soc_control_t	*soc = SOC_CONTROL(unit);
    int                 mode;

    mode = soc_property_get(unit, spn_L2XMSG_MODE, L2MODE_POLL);

    if ((mode == L2MODE_FIFO) && !soc_feature(unit, soc_feature_l2_modfifo)) {
        mode = L2MODE_POLL;
    }

    if ((mode == L2MODE_POLL) && soc->l2x_external) {
        /* Not supported for external L2 memory configuration. */
        return SOC_E_UNAVAIL;
    }
#endif
    if (!soc_feature(unit, soc_feature_arl_hashed)) {
        return SOC_E_UNAVAIL;
    }

    if (ad->cb_count >= L2X_CB_MAX) {
        return SOC_E_MEMORY;
    }

    for (i = 0; i < ad->cb_count; i++) {
        if ((ad->cb[i].fn == fn &&
             ad->cb[i].fn_data == fn_data)) {
            return SOC_E_NONE; /* Already registered */
        }
    }

    ad->cb[ad->cb_count].fn = fn;
    ad->cb[ad->cb_count].fn_data = fn_data;

    ad->cb_count++;

    return SOC_E_NONE;
}

/*
 * Function:
 *	soc_l2x_unregister
 * Purpose:
 *  Unregister a callback routine; requires same args as when registered
 * Parameters:
 *  unit - StrataSwitch unit number
 *  fn - Callback function to unregister; NULL to unregister all
 *  fn_data - Extra data passed to callback function;
 *  must match registered value unless fn is NULL
 * Returns:
 *  SOC_E_NONE - Success
 *  SOC_E_NOT_FOUND - Matching registered routine not found
 */

int
soc_l2x_unregister(int unit, soc_l2x_cb_fn fn, void *fn_data)
{
    l2x_data_t  *ad = &l2x_data[unit];
    int         i;
    if (fn == NULL) {
        ad->cb_count = 0;
        return SOC_E_NONE;
    }

    for (i = 0; i < ad->cb_count; i++) {
        if ((ad->cb[i].fn == fn &&
             ad->cb[i].fn_data == fn_data)) {

            for (ad->cb_count--; i < ad->cb_count; i++) {
                sal_memcpy(&ad->cb[i], &ad->cb[i + 1],
                           sizeof (l2x_cb_entry_t));
            }

            return SOC_E_NONE;
        }
    }

    return SOC_E_NOT_FOUND;
}

/*
 * Function:
 *	soc_l2x_callback
 * Purpose:
 *	Routine to execute all callbacks on the list.
 * Parameters:
 *	unit - unit number.
 *	entry_del - deleted or updated entry, NULL if none.
 *	entry_add - added or updated entry, NULL if none.
 */

void
soc_l2x_callback(int unit, int flags, l2x_entry_t *entry_del, l2x_entry_t *entry_add)
{
    l2x_data_t      *ad = &l2x_data[unit];
    int         i;
    for (i = 0; i < ad->cb_count; i++) {
        (*ad->cb[i].fn)(unit, flags, entry_del, entry_add, ad->cb[i].fn_data);
    }
}

/*
 * Function:
 * 	soc_l2x_running
 * Purpose:
 *	Determine the L2X sync thread running parameters
 * Parameters:
 *	unit - unit number.
 *	flags (OUT) - if non-NULL, receives the current flag settings
 *	interval (OUT) - if non-NULL, receives the current pass interval
 * Returns:
 *   	Boolean; TRUE if L2X sync thread is running
 */

int
soc_l2x_running(int unit, uint32 *flags, sal_usecs_t *interval)
{
    soc_control_t   *soc = SOC_CONTROL(unit);
#ifdef BCM_TRIUMPH3_SUPPORT
    if SOC_IS_TRIUMPH3(unit) {
        return soc_tr3_l2x_running(unit, flags, interval);
    }
#endif
    if (soc->l2x_pid != SAL_THREAD_ERROR) {
        if (flags != NULL) {
            *flags = soc->l2x_flags;
        }
        if (interval != NULL) {
            *interval = soc->l2x_interval;
        }
    }

    return(soc->l2x_pid != SAL_THREAD_ERROR);
}

/* It only can handle delete so far */
int
_soc_l2x_sync_replace(int unit, l2x_entry_t *l2x_match_data,
                      l2x_entry_t *l2x_match_mask, uint32 flags)
{
    soc_control_t *soc = SOC_CONTROL(unit);
    uint32 *entry, *match_data, *match_mask;
    int index_max, entry_words, idx, i;

    if (soc->l2x_pid == SAL_THREAD_ERROR) {
        /* thread not running */
        return SOC_E_NONE;
    }

    entry = l2x_data[unit].shadow_tab;
    if (entry == NULL) {
        return SOC_E_NONE;
    }
    match_data = (uint32 *)l2x_match_data;
    match_mask = (uint32 *)l2x_match_mask;

    index_max = soc_mem_index_max(unit, L2Xm);
    entry_words = soc_mem_entry_words(unit, L2Xm);

    (void)sal_sem_take(soc->l2x_lock, sal_sem_FOREVER);

    for (idx = 0; idx <= index_max; idx++, entry += entry_words) {
        for (i = 0; i < entry_words; i++) {
            if ((entry[i] ^ match_data[i]) & match_mask[i]) {
                break;
            }
        }
        if (i != entry_words) {
            continue;
        }
        soc_l2x_sync_delete(unit, entry, idx, flags);
    }

    (void)sal_sem_give(soc->l2x_lock);

    return SOC_E_NONE;
}
 
/*
 * Function:
 * 	_soc_l2x_sync_delete_by
 * Purpose:
 *	Search all the entries in the shadow table based on the 
 *	criteria and mark them as deleted.
 * Parameters:
 * Returns:
 *      SOC_E_XX
 */
int
_soc_l2x_sync_delete_by(int unit, uint32 mod, uint32 port,
                        uint16 vid, uint32 tid, int vfi,
                        uint32 flags, uint32 search_key)
{
    soc_control_t   *soc = SOC_CONTROL(unit);
    int             index, max_index, stl2;
    uint32          *entry = NULL;
    soc_field_t     valid_bit = VALIDf;
    int             trunk128, rv = SOC_E_NONE;
    int             static_bit_val;
    uint32          tgid_hi = 0, tgid_lo = 0;
    uint32          tgid_val, port_val, mod_id;
    vlan_id_t       vlan_id;

#ifdef BCM_TRIUMPH3_SUPPORT
    if SOC_IS_TRIUMPH3(unit) {
        return _soc_tr3_l2x_sync_delete_by(unit, mod, port, vid, tid, vfi,
                                           flags, search_key);
    }
#endif 


    if (soc->l2x_pid == SAL_THREAD_ERROR) {
        /* thread not running */
        return SOC_E_NONE;
    }

    max_index = soc_mem_index_max(unit, l2x_data[unit].l2mem);

    stl2 = (flags & SOC_L2X_INC_STATIC) ? 1 : 0;

    (void)sal_sem_take(soc->l2x_lock, sal_sem_FOREVER);

    entry = l2x_data[unit].shadow_tab;
    if (!entry) {
        goto exit;
    }

    if (search_key == SOC_L2X_PORTMOD_DEL) {
        for (index = 0; index < max_index; index++,
            entry += l2x_data[unit].entry_words) {
            if (!L2X_IS_VALID_ENTRY(unit, l2x_data[unit].l2mem, 
                                    entry, valid_bit)) {
                continue;
            }
            if (soc_feature(unit, soc_feature_trunk_group_overlay)) {
                if (soc_mem_field32_get(unit, l2x_data[unit].l2mem, entry,
                                        Tf)) {
                    continue;
                }
                port_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                               entry, PORT_NUMf);
            } else {
                port_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                               entry, TGID_PORTf);
            }
            mod_id = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                         entry, MODULE_IDf);
            if ((port != port_val) || (mod != mod_id)) {
                continue;
            }

            static_bit_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                                 entry, STATIC_BITf);
            if (stl2 || (stl2 == static_bit_val)) {
                soc_l2x_sync_delete(unit, entry, index, flags);
            }
        }
        goto exit;
    }

    if (search_key == SOC_L2X_VLAN_DEL) {
        for (index = 0; index < max_index; index++,
            entry += l2x_data[unit].entry_words) {
            if (!L2X_IS_VALID_ENTRY(unit, l2x_data[unit].l2mem, 
                                    entry, valid_bit)) {
                continue;
            }
            vlan_id = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                          entry, VLAN_IDf);
            if (vid != vlan_id) {
                continue;
            }

            static_bit_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                                 entry, STATIC_BITf);
            if (stl2 || (stl2 == static_bit_val)) {
                soc_l2x_sync_delete(unit, entry, index, flags);
            }
        }
        goto exit;
    }

    if (search_key == SOC_L2X_VFI_DEL) {
#ifdef BCM_TRIUMPH_SUPPORT
        int vfi_id;
        for (index = 0; index < max_index; index++,
            entry += l2x_data[unit].entry_words) {
            if (!L2X_IS_VALID_ENTRY(unit, l2x_data[unit].l2mem,
                                    entry, valid_bit)) {
                continue;
            }
            vfi_id = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                         entry, VFIf);
            if (vfi != vfi_id) {
                continue;
            }

            static_bit_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                                 entry, STATIC_BITf);
            if (stl2 || (stl2 == static_bit_val)) {
                soc_l2x_sync_delete(unit, entry, index, flags);
            }
        }
#endif
        goto exit;
    }

    if (search_key == SOC_L2X_PORTMOD_VLAN_DEL) {
        for (index = 0; index < max_index; index++,
            entry += l2x_data[unit].entry_words) {
            if (!L2X_IS_VALID_ENTRY(unit, l2x_data[unit].l2mem, 
                                    entry, valid_bit)) {
                continue;
            }
            if (soc_feature(unit, soc_feature_trunk_group_overlay)) {
                if (soc_mem_field32_get(unit, l2x_data[unit].l2mem, entry,
                                        Tf)) {
                    continue;
                }
                port_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                               entry, PORT_NUMf);
            } else {
                port_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                               entry, TGID_PORTf);
            }
            mod_id = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                         entry, MODULE_IDf);
            vlan_id = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                          entry, VLAN_IDf);
            if ((vid != vlan_id) ||(port != port_val) ||(mod != mod_id)) {
                continue;
            }

            static_bit_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                                 entry, STATIC_BITf);
            if (stl2 || (stl2 == static_bit_val)) {
                soc_l2x_sync_delete(unit, entry, index, flags);
            }
        }
        goto exit;
    }

    if ((search_key == SOC_L2X_TRUNK_DEL) || 
        (search_key == SOC_L2X_TRUNK_VLAN_DEL)) {
        trunk128 = soc_feature(unit, soc_feature_trunk_extended);

        if (trunk128) {
            if (!soc_feature(unit, soc_feature_trunk_group_overlay)) {
                tgid_hi = (tid & SOC_L2X_TGID_PORT_TRUNK_MASK_HI) >>
                          SOC_TRUNK_BIT_POS(unit);
                tgid_lo = tid & SOC_L2X_TGID_PORT_TRUNK_MASK;
                tgid_lo |= SOC_L2X_TGID_TRUNK_INDICATOR(unit);
            }

            for (index = 0; index < max_index; index++,
                entry += l2x_data[unit].entry_words) {
                if (!L2X_IS_VALID_ENTRY(unit, l2x_data[unit].l2mem, 
                                        entry, valid_bit)) {
                    continue;
                }
                if (soc_feature(unit, soc_feature_trunk_group_overlay)) {
                    if (!soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                              entry, Tf) ||
                        soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                             entry, TGIDf) != tid) {
                        continue;
                    }
                    if ((search_key == SOC_L2X_TRUNK_VLAN_DEL) && 
                        (vid != soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                                    entry, VLAN_IDf))) {
                        continue;

                    }
                } else {
                    port_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                                    entry, TGID_PORTf);
                    tgid_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                                    entry, MODULE_IDf);

                    if ((tgid_lo != port_val) ||(tgid_hi != tgid_val)) {
                        continue;
                    }
                    if ((search_key == SOC_L2X_TRUNK_VLAN_DEL) && 
                        (vid != soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                                    entry, VLAN_IDf))) {
                        continue;

                    }
                }

                static_bit_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                                     entry, STATIC_BITf);
                if (stl2 || (stl2 == static_bit_val)) {
                    soc_l2x_sync_delete(unit, entry, index, flags);
                }
            }
        } else {
            tid &= SOC_L2X_TGID_PORT_TRUNK_MASK;
            tid |= SOC_L2X_TGID_TRUNK_INDICATOR(unit);

            for (index = 0; index < max_index; index++,
                entry += l2x_data[unit].entry_words) {
                if (!L2X_IS_VALID_ENTRY(unit, l2x_data[unit].l2mem, 
                                        entry, valid_bit)) {
                    continue;
                }
                port_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                               entry, TGID_PORTf);
                if (tid != port_val ) {
                    continue;
                }
                if ((search_key == SOC_L2X_TRUNK_VLAN_DEL) && 
                    (vid != soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                                entry, VLAN_IDf))) {
                    continue;

                }
                static_bit_val = soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                                     entry, STATIC_BITf);
                if (stl2 || (stl2 == static_bit_val)) {
                    soc_l2x_sync_delete(unit, entry, index, flags);
                }
            }
        }
        goto exit;
    }

    rv = SOC_E_PARAM;

exit:
    (void)sal_sem_give(soc->l2x_lock);
    return rv;
}

/*
 * Function:
 * 	soc_l2x_sync_delete
 * Purpose:
 *	sync the SW shadow copy for the deleted entry from HW.
 * Parameters:
 *	unit - unit number.
 *	del_entry - entry deleted from the HW l2X memory
 * Returns:
 *      SOC_E_XX
 */

int
soc_l2x_sync_delete(int unit, uint32 *del_entry, int index, uint32 flags)
{
    soc_control_t   *soc = SOC_CONTROL(unit);
    int                 max_index;
    uint32              *tab_p;

    LOG_VERBOSE(BSL_LS_SOC_ARL,
                (BSL_META_U(unit,
                            "soc_l2x_sync_delete: unit=%d index=%d\n"),
                 unit, index));

    if (soc->l2x_pid == SAL_THREAD_ERROR) {
        /* thread not running */
        return SOC_E_NONE;
    }

    if (l2x_data[unit].l2mem == INVALIDm) {
        return SOC_E_NONE;
    }

    max_index = soc_mem_index_max(unit, l2x_data[unit].l2mem);
    if (index > max_index) {
        return SOC_E_PARAM;
    }

    /*
     * validate that the entry in the shadow copy is same as the entry
     * deleted.
     */
    tab_p = l2x_data[unit].shadow_tab + (index * l2x_data[unit].entry_words);
    if (L2X_ENTRY_EQL(unit, del_entry, tab_p)) {
        L2X_ENTRY_DELETED_SET(unit, index);
        /* Bitmap that indicates to suppress callback */
        if ((flags & SOC_L2X_NO_CALLBACKS)) {
            L2X_ENTRY_CALLBACK_SET(unit, index);
        }
    }

    return SOC_E_NONE;
}

/*
 * Function:
 * 	soc_l2x_sync_delete_by_port
 * Purpose:
 *	sync the SW shadow copy for the deleted entryies based on port.
 * Parameters:
 *	unit - unit number.
 *	port - port number.
 * Returns:
 *      SOC_E_XX
 */

int
soc_l2x_sync_delete_by_port(int unit, int mod, soc_port_t port,
                            uint32 flags)
{
    return _soc_l2x_sync_delete_by(unit, mod, port, 0, 0, 0,
                                   flags, SOC_L2X_PORTMOD_DEL);
}

/*
 * Function:
 * 	soc_l2x_sync_delete_by_trunk
 * Purpose:
 *	sync the SW shadow copy for the deleted entryies based on trunk id.
 * Parameters:
 *	unit - unit number.
 *	port - port number.
 * Returns:
 *      SOC_E_XX
 */

int
soc_l2x_sync_delete_by_trunk(int unit, int tid, uint32 flags)
{
    return _soc_l2x_sync_delete_by(unit, 0, 0, 0, tid, 0,
                                   flags, SOC_L2X_TRUNK_DEL);
}

/*
 * Function:
 * 	soc_l2x_start
 * Purpose:
 *   	Initialize shadow table and start L2X thread to maintain it
 * Parameters:
 *	unit - unit number.
 *	flags - SOC_L2X_FLAG_xxx
 *	interval - time between resynchronization passes
 * Returns:
 *	SOC_E_MEMORY if can't create thread.
 */

int
soc_l2x_start(int unit, uint32 flags, sal_usecs_t interval)
{
    soc_control_t   *soc = SOC_CONTROL(unit);
    int         pri;
#ifdef BCM_XGS3_SWITCH_SUPPORT
    int                 mode;
#endif
#ifdef BCM_TRIUMPH3_SUPPORT
    if SOC_IS_TRIUMPH3(unit) {
        return soc_tr3_l2x_start(unit, flags, interval);
    }
#endif
    LOG_INFO(BSL_LS_SOC_ARL,
             (BSL_META_U(unit,
                         "soc_l2x_start: unit=%d flags=0x%x interval=%d\n"),
              unit, flags, interval));

    if (!soc_feature(unit, soc_feature_arl_hashed)) {
        return SOC_E_UNAVAIL;
    }

    if (soc->l2x_interval != 0) {
        SOC_IF_ERROR_RETURN(soc_l2x_stop(unit));
    }

    sal_snprintf(soc->l2x_name, sizeof (soc->l2x_name),
                 "bcmL2X.%d", unit);

    if (soc->l2x_pid == SAL_THREAD_ERROR) {
        pri = soc_property_get(unit, spn_L2XMSG_THREAD_PRI, 50);
#ifdef BCM_XGS3_SWITCH_SUPPORT
        mode = soc_property_get(unit, spn_L2XMSG_MODE, L2MODE_POLL);

        if (soc_feature(unit, soc_feature_l2_modfifo) && (mode == L2MODE_FIFO)) {

            SOC_CONTROL_LOCK(unit);
            soc->l2x_mode = mode;
            soc->l2x_flags = flags;
            soc->l2x_interval = interval;

            if (interval == 0) {
                SOC_CONTROL_UNLOCK(unit);
                return SOC_E_NONE;
            }
#ifdef BCM_CMICM_SUPPORT
            if (SOC_IS_TD2_TT2(unit) || SOC_IS_KATANA2(unit) || 
                soc_feature(unit, soc_feature_fifo_dma_hu2)) {
                _soc_l2mod_start(unit, flags, interval);
            } else 
#endif
            {
                soc_l2mod_start(unit, flags, interval);
            }
            SOC_CONTROL_UNLOCK(unit);
            l2x_data[unit].l2mem = INVALIDm;
        } else
#endif
        {
            if (soc->l2x_external) {
                /* Not supported for external L2 memory configuration. */
                return SOC_E_NONE;
            }

            SOC_CONTROL_LOCK(unit);
            soc->l2x_mode = L2MODE_POLL;
            soc->l2x_flags = flags;
            soc->l2x_interval = interval;

            if (interval == 0) {
                SOC_CONTROL_UNLOCK(unit);
                return SOC_E_NONE;
            }
            soc->l2x_pid = sal_thread_create(soc->l2x_name,
                                             SAL_THREAD_STKSZ,
                                             pri,
                                             _soc_l2x_thread,
                                             INT_TO_PTR(unit));
            if (soc->l2x_pid == SAL_THREAD_ERROR) {
                LOG_ERROR(BSL_LS_SOC_L2,
                          (BSL_META_U(unit,
                                      "soc_l2x_start: Could not start L2X thread\n")));
                SOC_CONTROL_UNLOCK(unit);
                return SOC_E_MEMORY;
            }
            SOC_CONTROL_UNLOCK(unit);
        }
    }

    return SOC_E_NONE;
}

/*
 * Function:
 * 	soc_l2x_stop
 * Purpose:
 *   	Stop L2X-related thread
 * Parameters:
 *	unit - unit number.
 * Returns:
 *	SOC_E_XXX
 */

int
soc_l2x_stop(int unit)
{
    soc_control_t   *soc = SOC_CONTROL(unit);
    int         rv = SOC_E_NONE;
    soc_timeout_t   to;
#ifdef BCM_XGS3_SWITCH_SUPPORT
    int                 mode;
#endif
#ifdef BCM_TRIUMPH3_SUPPORT
    if SOC_IS_TRIUMPH3(unit) {
        return soc_tr3_l2x_stop(unit);
    }
#endif
    LOG_INFO(BSL_LS_SOC_ARL,
             (BSL_META_U(unit,
                         "soc_l2x_stop: unit=%d\n"), unit));

    SOC_CONTROL_LOCK(unit);
    soc->l2x_interval = 0;  /* Request exit */
    SOC_CONTROL_UNLOCK(unit);

    if (soc->l2x_pid != SAL_THREAD_ERROR) {
        /* Wake up thread so it will check the exit flag */
#ifdef BCM_XGS3_SWITCH_SUPPORT
        mode = soc_property_get(unit, spn_L2XMSG_MODE, L2MODE_POLL);
        if (soc_feature(unit, soc_feature_l2_modfifo) &&
            (mode == L2MODE_FIFO)) {
#ifdef BCM_CMICM_SUPPORT
            if (SOC_IS_TD2_TT2(unit) || SOC_IS_KATANA2(unit) ||
                soc_feature(unit, soc_feature_fifo_dma_hu2)) {
                _soc_l2mod_stop(unit);
            } else 
#endif
            {
                soc_l2mod_stop(unit);
            }
        } else
#endif
        {
            sal_sem_give(soc->l2x_notify);
        }

        /* Give thread a few seconds to wake up and exit */
        if (SAL_BOOT_SIMULATION) {
            soc_timeout_init(&to, 30 * 1000000, 0);
        } else {
            soc_timeout_init(&to, 10 * 1000000, 0);
        }

        while (soc->l2x_pid != SAL_THREAD_ERROR) {
            if (soc_timeout_check(&to)) {
                LOG_ERROR(BSL_LS_SOC_L2,
                          (BSL_META_U(unit,
                                      "soc_l2x_stop: thread will not exit\n")));
                rv = SOC_E_INTERNAL;
                break;
            }
        }
    }

    return rv;
}

#if defined(BCM_TRIDENT2_SUPPORT)
STATIC int
_soc_trident2_l2x_sync_multi_buckets(int unit, uint32 *new_entry, 
                                     uint8 shadow_hit_bits, int *mac_moved)
{
    int num_banks = 0;
    int bank_idx = 0;
    int hw_bank = 0;
    int bucket_base = 0;
    int entry_idx = 0;
    int delete_marked, callback_suppress;
    uint32 *shadow_entry = NULL;
    uint32 bucket;
    soc_control_t *soc = SOC_CONTROL(unit);
    soc_mem_t l2mem = l2x_data[unit].l2mem;
    int entry_words = l2x_data[unit].entry_words;
    SHR_BITDCL *delete_map = l2x_data[unit].delete_map;
    SHR_BITDCL *callback_map = l2x_data[unit].callback_map;

    SOC_IF_ERROR_RETURN(
        soc_trident2_hash_bank_count_get(unit, l2mem, &num_banks));
    for (bank_idx = 0; bank_idx < num_banks; bank_idx++) {
        SOC_IF_ERROR_RETURN
            (soc_trident2_hash_bank_number_get(unit, l2mem, bank_idx, &hw_bank));
        SOC_IF_ERROR_RETURN
            (soc_hash_bucket_get(unit, l2mem, hw_bank, new_entry, &bucket));
        bucket_base = soc_hash_index_get(unit, l2mem, hw_bank, bucket);
        shadow_entry = l2x_data[unit].shadow_tab + (bucket_base * entry_words);
        for (entry_idx = 0; entry_idx < SOC_L2X_BUCKET_SIZE; entry_idx++) {
            if (soc_mem_field32_get(unit, l2mem, shadow_entry, VALIDf) &&
                soc_mem_compare_key(unit, l2mem, new_entry, shadow_entry) == 0) {
                break;
            }
            shadow_entry += entry_words;
        }
        if (entry_idx < SOC_L2X_BUCKET_SIZE) {
            break;
        }
    }

    if (bank_idx < num_banks) {
        /* This MAC existed previously in the bucket of another bank.
         * Check if everything at the new location is the same (ignoring
         * hit bit).  If so, there is no delete or change processing.
         */
         if (!(shadow_hit_bits & L2X_SHADOW_HIT_BITS)) {
             if (!(shadow_hit_bits & L2X_SHADOW_HIT_DST)) {
                 soc_L2Xm_field32_set(unit, shadow_entry, HITDAf,
                                      soc_L2Xm_field32_get(unit, new_entry,
                                                           HITDAf));
             }
             if (!(shadow_hit_bits & L2X_SHADOW_HIT_SRC)) {
                 soc_L2Xm_field32_set(unit, shadow_entry, HITSAf,
                                      soc_L2Xm_field32_get(unit, new_entry,
                                                           HITSAf));
             }
         }
        /* Check if the entry has been deleted and is marked for
         * delete.
         */
        if (SOC_L2_DEL_SYNC_LOCK(soc) < 0) {
            return SOC_E_RESOURCE;
        }
        delete_marked = SHR_BITGET(delete_map, bucket_base + entry_idx);
        callback_suppress = SHR_BITGET(callback_map, bucket_base + entry_idx);
        SOC_L2_DEL_SYNC_UNLOCK(soc);
        if (L2X_ENTRY_EQL(unit, shadow_entry, new_entry)) {
            if (delete_marked) {
                /* Entry got learnt again after delete */
                if (callback_suppress) {
                    /* Indicate re-learn */
                    soc_l2x_callback(unit, 0, NULL, (l2x_entry_t *)new_entry);
                } else {
                    /* Indicate delete and re-learn */
                    soc_l2x_callback(unit, 0, (l2x_entry_t *)shadow_entry, NULL);
                    soc_l2x_callback(unit, 0, NULL, (l2x_entry_t *)new_entry);
                }
            }
            /* Invalid entry in shadow table */
            soc_L2Xm_field32_set(unit, shadow_entry, VALIDf, 0);
            *mac_moved = TRUE;
            return SOC_E_NONE;
        }
        /* Issue change (delete & insert) */
        if (!callback_suppress) {
            soc_l2x_callback(unit, 0, (l2x_entry_t *)shadow_entry,
                             (l2x_entry_t *)new_entry);
        }
        /* Invalid entry in the shadow table */
        soc_L2Xm_field32_set(unit, shadow_entry, VALIDf, 0);
        *mac_moved = TRUE;
        return SOC_E_NONE;
    }
    *mac_moved = FALSE;
    return SOC_E_NONE;
}
#endif

/*
 * Function:
 *	_soc_l2x_sync_bucket
 * Purpose:
 *	Compare the old contents of a hash bucket (8 entries) to the
 *	next contents, make any ARL callbacks that are necessary,
 *	and sync that bucket in the shadow table.
 * Parameters:
 *	unit - StrataSwitch unit #
 *	old_bucket - Previous bucket contents (SOC_L2X_BUCKET_SIZE entries)
 *	new_bucket - New bucket contents (SOC_L2X_BUCKET_SIZE entries)
 *	shadow_hit_bits - perform the notify callback even if
 *			only hit bits change.
 *	bucket_index - start index of bucket within chunk
 *	p_chunk_del_map - pointer to chunk's delete map
 * Notes:
 *	It's necessary to process deletions first, then insertions.
 *	If an entry moves within the hash bucket, or some field such
 *	as PORT changes, the application first receives a callback
 *	for a deletion, and then for an insertion.
 */

STATIC void
_soc_l2x_sync_bucket(int unit,
                     uint32 *old_bucket,
                     uint32 *new_bucket,
                     uint8 shadow_hit_bits, int bucket_index,
                     SHR_BITDCL *p_chunk_del_map, 
                     SHR_BITDCL *p_chunk_cb_map)
{
    int         io, in, delete_marked, callback_suppress;
    soc_field_t         valid_bit = VALIDf;
    uint32              *old_p, *new_p;
#if defined(BCM_TRIDENT2_SUPPORT)
    int rv = SOC_E_NONE;
    int mac_moved = 0;
#endif
    /*
     * Process deletions, which will be any entry that was in the bucket
     * before but whose key is no longer found anywhere in the bucket,
     * and changes, which will be any entry whose key is still found but
     * whose associated data (port, etc.) has changed.
     */

    old_p = old_bucket;
    for (io = 0; io < SOC_L2X_BUCKET_SIZE; io++,
        old_p += l2x_data[unit].entry_words) {

        /*
         * No deletion or change is needed for invalid entries.
         */
        if (!soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                 old_p, valid_bit)) {
            continue;
        }

        /*
         * Check if the entry has been deleted and is marked for
         * delete.
         */
        delete_marked = SHR_BITGET(p_chunk_del_map, bucket_index + io);
        callback_suppress = SHR_BITGET(p_chunk_cb_map, bucket_index + io);

        /*
         * Quick check: if the entry is identical, it does not need any
         * deletion or change processing.  It may be valid or not.
         */

        new_p = new_bucket + (io * l2x_data[unit].entry_words);
        if (L2X_ENTRY_EQL(unit, old_p, new_p)) {
            if (delete_marked) {
                /* Entry got learnt again after delete */
                if (callback_suppress) {
                    /* Indicate re-learn */
                    soc_l2x_callback(unit, 0, NULL, (l2x_entry_t *) new_p);
                } else {
                    /* Indicate delete and re-learn */
                    soc_l2x_callback(unit, 0, (l2x_entry_t *) old_p, NULL);
                    soc_l2x_callback(unit, 0, NULL, (l2x_entry_t *) new_p);
                }
            }
            continue;
        }

        /*
         * See if the old key still exists anywhere within the bucket.
         */

        new_p = new_bucket;
        for (in = 0; in < SOC_L2X_BUCKET_SIZE; in++) {
            if (soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                    new_p, valid_bit) &&
                soc_mem_compare_key(unit, l2x_data[unit].l2mem,
                                    old_p, new_p) == 0) {
                break;
            }
            new_p += l2x_data[unit].entry_words;
        }

        if (in == SOC_L2X_BUCKET_SIZE) {
            /* It doesn't, so issue a delete. */
            if (!callback_suppress) {
                soc_l2x_callback(unit, 0, (l2x_entry_t *) old_p, NULL);
            }
            continue;
        }

        /*
         * Check if everything at the new location is the same (ignoring
         * hit bit).  If so, there is no delete or change processing.
         */

        if (!(shadow_hit_bits & L2X_SHADOW_HIT_BITS)) {
            if (!(shadow_hit_bits & L2X_SHADOW_HIT_DST)) { 
                soc_L2Xm_field32_set(unit, old_p, HITDAf,
                                     soc_L2Xm_field32_get(unit, new_p,
                                                          HITDAf));
            }
            if (!(shadow_hit_bits & L2X_SHADOW_HIT_SRC)) {
                soc_L2Xm_field32_set(unit, old_p, HITSAf,
                                     soc_L2Xm_field32_get(unit, new_p,
                                                          HITSAf));
            }
        }

        if (L2X_ENTRY_EQL(unit, old_p, new_p)) {
            if (delete_marked) {
                /* Entry got learnt again after delete */
                if (callback_suppress) {
                    /* Indicate re-learn */
                    soc_l2x_callback(unit, 0, NULL, (l2x_entry_t *) new_p);
                } else {
                    /* Indicate delete and re-learn */
                    soc_l2x_callback(unit, 0, (l2x_entry_t *) old_p, NULL);
                    soc_l2x_callback(unit, 0, NULL, (l2x_entry_t *) new_p);
                }
            }
            continue;
        }

        /* Issue change (delete & insert) */
        if (!callback_suppress) {
            soc_l2x_callback(unit, 0, (l2x_entry_t *) old_p,
                             (l2x_entry_t *) new_p);
        }
    }

    /*
     * Process insertions, which will be any entry whose key was not in
     * the bucket before, but is now.
     */

    new_p = new_bucket;
    for (in = 0; in < SOC_L2X_BUCKET_SIZE; in++,
        new_p += l2x_data[unit].entry_words) {

        /*
         * Quick check: if the entry is identical, it does not need any
         * insertion processing.  It may be valid or not.
         */
        old_p = old_bucket + (in * l2x_data[unit].entry_words);
        if (L2X_ENTRY_EQL(unit, new_p, old_p)) {
            continue;
        }

        /*
         * No insert needed for invalid entries.
         */

        if (!soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                 new_p, valid_bit)) {
            continue;
        }

        /*
         * If the same key already existed elsewhere within the bucket,
         * it has already been taken care of by the change processing above.
         */

        old_p = old_bucket;
        for (io = 0; io < SOC_L2X_BUCKET_SIZE; io++) {
            if (soc_mem_field32_get(unit, l2x_data[unit].l2mem,
                                    old_p, valid_bit) &&
                soc_mem_compare_key(unit, l2x_data[unit].l2mem,
                                    new_p, old_p) == 0) {
                break;
            }
            old_p += l2x_data[unit].entry_words;
        }

        if (io < SOC_L2X_BUCKET_SIZE) {
            continue;
        }

        if (soc_feature(unit, soc_feature_ppa_bypass)) {
            if (SOC_CONTROL(unit)->l2x_ppa_bypass == FALSE &&
                soc_mem_field32_get(unit, L2Xm, new_p, KEY_TYPEf) !=
                TR_L2_HASH_KEY_TYPE_BRIDGE) {
                SOC_CONTROL(unit)->l2x_ppa_bypass = TRUE;
            }
        }

#if defined(BCM_TRIDENT2_SUPPORT)
        /* Check whether this entry in the new bucket existed in 
         * other buckets in another banks previously.
         */
        if (SOC_IS_TD2_TT2(unit) && 
        	  soc_feature(unit, soc_feature_shared_hash_mem)) {
            rv = _soc_trident2_l2x_sync_multi_buckets(unit, new_p, 
                                                      shadow_hit_bits, 
                                                      &mac_moved);
            if (SOC_SUCCESS(rv)) {
                if (mac_moved == TRUE) {
                    continue;
                }
            }
        }
#endif
        soc_l2x_callback(unit, 0, NULL, (l2x_entry_t *) new_p);
    }

    /* Sync shadow copy */

    sal_memcpy(old_bucket, new_bucket,
               SOC_L2X_BUCKET_SIZE * l2x_data[unit].entry_words * 4);
}

/*
 * Function:
 * 	_soc_l2x_thread
 * Purpose:
 *   	Thread control for L2 shadow table maintenance
 * Parameters:
 *	unit_vp - StrataSwitch unit # (as a void *).
 * Returns:
 *	Nothing
 * Notes:
 *	Exits when l2x_rate is set to zero and semaphore is given.  The
 *	table is processed one chunk at a time (spn_L2XMSG_CHUNKS chunks
 *	total) in order to reduce memory requirements for the temporary
 *	DMA buffer and to give the CPU a break from bursts of activity.
 */

STATIC void
_soc_l2x_thread(void *unit_vp)
{
    int         unit = PTR_TO_INT(unit_vp);
    soc_control_t   *soc = SOC_CONTROL(unit);
    int         rv;
    uint32      *shadow_tab = NULL, *chunk_buf = NULL;
    SHR_BITDCL          *delete_map = NULL;
    SHR_BITDCL          *chunk_delete_map = NULL;
    SHR_BITDCL          *callback_map = NULL;
    SHR_BITDCL          *chunk_callback_map = NULL;
    int         index_count;
    int         chunk_count, chunk_size;
    int         chunk_index, bucket_index;
    uint32      *tab_p, *buf_p;
    int         interval;
    sal_usecs_t     stime, etime;

    l2x_data[unit].l2mem = L2Xm;
    l2x_data[unit].entry_bytes = soc_mem_entry_bytes(unit, l2x_data[unit].l2mem);
    l2x_data[unit].entry_words = soc_mem_entry_words(unit, l2x_data[unit].l2mem);

    assert(soc_mem_index_min(unit, l2x_data[unit].l2mem) == 0);
    index_count = soc_mem_index_count(unit, l2x_data[unit].l2mem);
    if (index_count <= 0) {
        LOG_ERROR(BSL_LS_SOC_L2,
                  (BSL_META_U(unit,
                              "soc_l2x_thread: table size is 0 \n")));
        soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                           SOC_SWITCH_EVENT_THREAD_L2X, __LINE__, SOC_E_EMPTY);
        goto cleanup_exit;
    }

    chunk_count = soc_property_get(unit, spn_L2XMSG_CHUNKS, 8);
    chunk_size = index_count / chunk_count;

    assert(chunk_size > 0);
    assert(chunk_size % SOC_L2X_BUCKET_SIZE == 0);

    shadow_tab = sal_alloc(index_count * l2x_data[unit].entry_words * 4,
                           "l2x_old");

    chunk_buf = soc_cm_salloc(unit,
                              chunk_size * l2x_data[unit].entry_words * 4,
                              "l2x_new");

    delete_map = sal_alloc(SHR_BITALLOCSIZE(index_count), "l2x_delete_map");
    callback_map = sal_alloc(SHR_BITALLOCSIZE(index_count), "l2x_callback_map");

    chunk_delete_map = 
    sal_alloc(SHR_BITALLOCSIZE(chunk_size), "l2x_chunk_delete_map");
    chunk_callback_map = 
    sal_alloc(SHR_BITALLOCSIZE(chunk_size), "l2x_chunk_callback_map");

    if (shadow_tab == NULL || chunk_buf == NULL || 
        delete_map == NULL || chunk_delete_map == NULL || 
        callback_map == NULL || chunk_callback_map == NULL) {
        LOG_ERROR(BSL_LS_SOC_L2,
                  (BSL_META_U(unit,
                              "AbnormalThreadExit:soc_l2x_thread: not enough memory \n")));
        soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                           SOC_SWITCH_EVENT_THREAD_L2X, __LINE__, SOC_E_MEMORY);
        goto cleanup_exit;
    }
    /*
     * Start with initially empty shadow table.
     */

    sal_memset(shadow_tab, 0, index_count * l2x_data[unit].entry_words * 4);

    /*
     * Clear the delete and callback map.
     */
    SHR_BITCLR_RANGE(delete_map, 0, index_count);
    SHR_BITCLR_RANGE(callback_map, 0, index_count);

    l2x_data[unit].shadow_tab = shadow_tab;
    l2x_data[unit].chunk_buf = chunk_buf;
    l2x_data[unit].delete_map = delete_map;
    l2x_data[unit].callback_map = callback_map;

    chunk_index = 0;
    tab_p = shadow_tab;

    while ((interval = soc->l2x_interval) != 0) {
        /*
         * Read the next chunk of the L2 table using Table DMA.
         */

        LOG_VERBOSE(BSL_LS_SOC_ARL,
                    (BSL_META_U(unit,
                                "soc_l2x_thread: Process %d-%d\n"),
                     chunk_index, chunk_index + chunk_size - 1));

        stime = sal_time_usecs();

        soc_mem_lock(unit, l2x_data[unit].l2mem);
        if (SOC_L2_DEL_SYNC_LOCK(soc) < 0) {
            LOG_ERROR(BSL_LS_SOC_L2,
                      (BSL_META_U(unit,
                                  "soc_l2x_thread: unable to take mutex\n")));
            soc_mem_unlock(unit, l2x_data[unit].l2mem);
            soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                               SOC_SWITCH_EVENT_THREAD_L2X, __LINE__,
                                SOC_E_RESOURCE);
            goto cleanup_exit;
        }

        if ((rv = soc_mem_read_range(unit, l2x_data[unit].l2mem,
                                     MEM_BLOCK_ANY,chunk_index,
                                     chunk_index + chunk_size - 1,
                                     chunk_buf)) < 0) {
            SOC_L2_DEL_SYNC_UNLOCK(soc);
            soc_mem_unlock(unit, l2x_data[unit].l2mem);
            LOG_ERROR(BSL_LS_SOC_L2,
                      (BSL_META_U(unit,
                                  "soc_l2x_thread: DMA failed: %s\n"),
                                  soc_errmsg(rv)));

            soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                               SOC_SWITCH_EVENT_THREAD_L2X, __LINE__, rv);

            goto cleanup_exit;
        } else {
            SHR_BITCOPY_RANGE(chunk_delete_map, 0, delete_map, chunk_index, 
                              chunk_size);
            SHR_BITCOPY_RANGE(chunk_callback_map, 0, callback_map, chunk_index, 
                              chunk_size);

            SHR_BITCLR_RANGE(delete_map, chunk_index, chunk_size);
            SHR_BITCLR_RANGE(callback_map, chunk_index, chunk_size);

            SOC_L2_DEL_SYNC_UNLOCK(soc);
            soc_mem_unlock(unit, l2x_data[unit].l2mem);

            /*
             * Scan, compare, and sync the old and new chunks one bucket
             * at a time.
             */

            buf_p = chunk_buf;
            for (bucket_index = 0; bucket_index < chunk_size; 
                bucket_index += SOC_L2X_BUCKET_SIZE) {
                _soc_l2x_sync_bucket(unit,tab_p,buf_p,soc->l2x_shadow_hit_bits,
                                     bucket_index, chunk_delete_map, 
                                     chunk_callback_map);
                tab_p += l2x_data[unit].entry_words * SOC_L2X_BUCKET_SIZE;
                buf_p += l2x_data[unit].entry_words * SOC_L2X_BUCKET_SIZE;
            }

            if ((chunk_index += chunk_size) >= index_count) {
                chunk_index = 0;
                tab_p = shadow_tab;
            }
        }

        etime = sal_time_usecs();

        /*
         * Implement the sleep using a semaphore timeout so if the task
         * is requested to exit, it can do so immediately.
         */

        LOG_VERBOSE(BSL_LS_SOC_ARL,
                    (BSL_META_U(unit,
                                "soc_l2x_thread: unit=%d: done in %d usec\n"),
                     unit,
                     SAL_USECS_SUB(etime, stime)));

        sal_sem_take(soc->l2x_notify, interval / chunk_count);
    }

cleanup_exit:

    (void)sal_sem_take(soc->l2x_lock, sal_sem_FOREVER);

    if (chunk_buf != NULL) {
        soc_cm_sfree(unit, chunk_buf);
        l2x_data[unit].chunk_buf = NULL;
    }

    if (shadow_tab != NULL) {
        sal_free(shadow_tab);
        l2x_data[unit].shadow_tab = NULL;
    }

    if (delete_map != NULL) {
        sal_free(delete_map);
        l2x_data[unit].delete_map = NULL;
    }

    if (chunk_delete_map != NULL) {
        sal_free(chunk_delete_map);
    }

    if (callback_map != NULL) {
        sal_free(callback_map);
        l2x_data[unit].callback_map = NULL;
    }

    if (chunk_callback_map != NULL) {
        sal_free(chunk_callback_map);
    }

    (void)sal_sem_give(soc->l2x_lock);

    LOG_INFO(BSL_LS_SOC_ARL,
             (BSL_META_U(unit,
                         "soc_l2x_thread: exiting\n")));

    soc->l2x_pid = SAL_THREAD_ERROR;
    sal_thread_exit(0);
}

#endif /* BCM_XGS_SWITCH_SUPPORT */
