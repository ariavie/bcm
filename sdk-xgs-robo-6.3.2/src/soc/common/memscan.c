/*
 * $Id: memscan.c 1.20.2.2 Broadcom SDK $
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
 * Memory Error Scan
 *
 * This is an optional module that can be used to detect and correct
 * memory errors in certain static hardware memories (tables).
 *
 * Additional CPU time is required to do the scanning.  Table DMA is
 * used to reduce this overhead.  The application may choose the overall
 * scan rate in entries/sec.
 *
 * There is also a requirement of additional memory needed to store
 * backing caches of the chip memories.  The backing caches are the same
 * ones used by the soc_mem_cache() mechanism.  Note that enabling the
 * memory cache for frequently updates tables can have significant
 * performance benefits.
 *
 * When the memory scanning thread is enabled, it simply scans all
 * memories for which caching is enabled, because these static memories
 * are the ones amenable to software error scanning.
 */

#ifdef INCLUDE_MEM_SCAN

#include <sal/core/libc.h>

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cmic.h>
#include <soc/error.h>
#include <soc/drv.h>
#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */

STATIC void _soc_mem_scan_thread(void *unit_vp);
static void *_soc_ser_info_p[SOC_MAX_NUM_DEVICES];
static int _soc_ser_info_is_generic[SOC_MAX_NUM_DEVICES];

typedef struct _soc_mem_scan_table_info_s {
    uint32 *xy_tcam_cache;            /* Standard mem cache uses DM encoding
                                       * Memscan needs the HW XY encoding. */
    SHR_BITDCL *overlay_tcam_bitmap;  /* Which overlay table view is valid
                                       * per entry? */
    /* Cache table-specific info to avoid recalculating on each pass */
    soc_mem_t mem;
    uint32 ser_flags;
    uint32 entry_dw;
    uint32 size;
    uint32 index_min;
    uint32 index_max;
    uint32 mask[SOC_MAX_MEM_WORDS];
    uint32 null_entry[SOC_MAX_MEM_WORDS];
} _soc_mem_scan_table_info_t;

typedef struct _soc_mem_scan_info_s {
    int num_tables;
    _soc_mem_scan_table_info_t *table_info;
} _soc_mem_scan_info_t;

static _soc_mem_scan_info_t *scan_info[SOC_MAX_NUM_DEVICES];

#define _MEM_SCAN_TABLE_INFO(unit, table_index) \
    (&(scan_info[(unit)]->table_info[(table_index)]))

/*
 * Function:
 *     soc_mem_scan_ser_list_register
 * Purpose:
 *       Provide the SER TCAM information list for a device.
 * Parameters:
 *    unit - StrataSwitch unit # (as a void *).
 *    ser_info - Pointer to the _soc_ser_parity_info_t for a device
 * Returns:
 *    SOC_E_XXX
 * Notes:
 */

void
soc_mem_scan_ser_list_register(int unit, int generic, void *ser_info)
{
    /* Record the format and pointer for SER TCAM info */
    _soc_ser_info_is_generic[unit] = generic;
    _soc_ser_info_p[unit] = ser_info;
}

#if defined(BCM_TRIDENT_SUPPORT)
/* Emulate the HW SER TCAM engine bit masking */
STATIC void
_soc_mem_scan_ser_tcam_datamask_get(int words, int bits, uint32 *mask_buf)
{
    int b;
    uint32 tmp;

    for (b = 0; b < words; b++) {
        mask_buf[b] = 0;
    }

    for (b = 0; b <= bits / 32; b++) {
        tmp = -1;

        if (b == 0) {
            tmp &= -1;
        }

        if (b == bits / 32) {
            tmp &= (1 << (bits % 32) << 1) - 1;
        }

        /* We check that this is not a Big Endian HW table during SER init */
        mask_buf[b] |= tmp;
    }
}

/* Device specific values to interpret FP_GM_FIELDS table */
#define SOC_TD_IFP_LOW_SLICE_SIZE  (128)
#define SOC_TD_IFP_HIGH_SLICE_SIZE  (256)
#define SOC_TD_IFP_LOW_SLICES (4) 
#define SOC_TD2_IFP_LOW_SLICE_SIZE  (512)
#define SOC_TD2_IFP_HIGH_SLICE_SIZE  (256)
#define SOC_TD2_IFP_LOW_SLICES (4) 

/*
 * Function:
 * 	_soc_mem_scan_overlay_range_get
 * Purpose:
 *   	Retrieve the range of overlay case entries for device specific
 *      overlay TCAMs.
 * Parameters:
 *	unit - unit number.
 *      start_index - current index of the table
 *      last_index - (OUT) boundary index of the table range
 *      overlay - (OUT) current table range is overlay case
 * Notes:
 *      This is very HW specific.  A future reorganization would
 *      put some of this logic in the appropriate chip-specific files.
 *      The purpose of this function is to determine if the start_index
 *      is in a valid range of the overlay case table, and if so
 *      the end of the current boundary.
 *      Specifically, this describes the FP_GM_FIELDS table of the
 *      Trident-class devices.
 * Returns:
 *	SOC_E_MEMORY if can't allocate cache.
 */

STATIC int
_soc_mem_scan_overlay_range_get(int unit, int start_index,
                                int *last_index, int *overlay)
{
    int slice, low_slices, low_size, high_size, boundary;
    fp_port_field_sel_entry_t pfs_entry;
    soc_field_t _trx_slice_pairing_field[] = {
        SLICE1_0_PAIRINGf,
        SLICE3_2_PAIRINGf,
        SLICE5_4_PAIRINGf,
        SLICE7_6_PAIRINGf,
        SLICE9_8_PAIRINGf,
        SLICE11_10_PAIRINGf
    };

    if (SOC_IS_TD2_TT2(unit)) {
        low_slices = SOC_TD2_IFP_LOW_SLICES;
        low_size = SOC_TD2_IFP_LOW_SLICE_SIZE;
        high_size = SOC_TD2_IFP_HIGH_SLICE_SIZE;
    } else if (SOC_IS_TD_TT(unit)) {
        low_slices = SOC_TD_IFP_LOW_SLICES;
        low_size = SOC_TD_IFP_LOW_SLICE_SIZE;
        high_size = SOC_TD_IFP_HIGH_SLICE_SIZE;
    } else {
        /* This is not a valid device for this function. */
        return SOC_E_INTERNAL;
    }

    /*
     * The FP slices are composed of two different slice sizes.
     * This logic determines in which slice a given index appears.
     * From the defines above, the structure is
     * TD/TD+: 4 small slices (low) + 6 large slices (high)
     * TD2:    4 large slices (low) + 8 small slices (high)
     * The "boundary" is the first index of the high slices segment,
     * or (last low slice segment index + 1), to determine in which
     * section the provided index lies, high or low.
     */
    boundary = low_slices * low_size;
    if (start_index < boundary) {
        slice = start_index / low_size;
        *last_index = ((slice +1) * low_size) - 1;
    } else {
        slice = ((start_index - boundary) / high_size);
        *last_index = boundary + ((slice +1) * high_size) - 1;
        slice += low_slices;
    }

    if (slice % 2) {
        /*  FP_PORT_FIELD_SEL entry 0 */
        SOC_IF_ERROR_RETURN
            (READ_FP_PORT_FIELD_SELm(unit, MEM_BLOCK_ANY, 0, &pfs_entry));

        if (soc_FP_PORT_FIELD_SELm_field32_get(unit, &pfs_entry,
                                   _trx_slice_pairing_field[slice / 2])) {
            *overlay = TRUE;
        } else {
            *overlay = FALSE;
        }
    }

    return SOC_E_NONE;       
}
#endif /* BCM_TRIDENT_SUPPORT */

STATIC void
_soc_mem_scan_info_free(int unit)
{
    soc_stat_t *stat = SOC_STAT(unit);
    _soc_mem_scan_table_info_t *table_info;
    uint32 *last_xy_tcam_p, *last_otb_p;
    int            ser_idx;

    if (NULL == scan_info[unit]) {
        /* Nothing to do */
        return;
    }

    if (NULL == scan_info[unit]->table_info) {
        sal_free(scan_info[unit]);
        return;
    }

    last_xy_tcam_p = NULL;
    last_otb_p = NULL;
    for (ser_idx = 0; ser_idx < scan_info[unit]->num_tables; ser_idx++) {
        table_info = &(scan_info[unit]->table_info[ser_idx]);

        /* We can have multiple table entries pointing to the
         * same XY TCAM cache when different HW pipes have
         * identical info.  Only free the memory once. */
        if ((NULL != table_info->xy_tcam_cache) &&
            (last_xy_tcam_p != table_info->xy_tcam_cache)){
            last_xy_tcam_p = table_info->xy_tcam_cache;
            sal_free(table_info->xy_tcam_cache);
        }
        /* The overlay bitmap is used by all of the memories
         * which are HW overlays.  Only free the memory once. */
        if ((NULL != table_info->overlay_tcam_bitmap) &&
            (last_otb_p != table_info->overlay_tcam_bitmap)){
            last_otb_p = table_info->overlay_tcam_bitmap;
            sal_free(table_info->overlay_tcam_bitmap);
        }

        stat->mem_cache_count--;
        stat->mem_cache_size -=
            WORDS2BYTES(table_info->size * table_info->entry_dw);
    }

    sal_free(scan_info[unit]->table_info);
    sal_free(scan_info[unit]);
    scan_info[unit] = NULL;
    return;
}

/*
 * Function:
 * 	_soc_mem_scan_info_init
 * Purpose:
 *   	Create TCAM memory information structure and 
 *      allocate XY TCAM cache used by SW comparison tables.
 * Parameters:
 *	unit - unit number.
 * Returns:
 *	SOC_E_MEMORY if can't allocate cache.
 */

STATIC int
_soc_mem_scan_info_init(int unit)
{
    int bits, entry_dw, index_cnt;
    soc_mem_t      mem;
    _soc_ser_parity_info_t *ser_info;
    _soc_generic_ser_info_t *gen_ser_info;
    _soc_mem_scan_table_info_t *table_info;
    int            ser_idx;
    uint32         ser_flags;
    int generic; /* Booleans */
    int alloc_size, idx;
#if defined(BCM_TRIDENT_SUPPORT)
    soc_mem_t      last_mem = INVALIDm;
    int blk, cache_offset, slice_last_idx, rv, overlay, no_cache;
    int overlay_created = FALSE;
    uint8 *vmap;
    uint32 *cache;
    soc_stat_t *stat = SOC_STAT(unit);
    soc_pbmp_t pbmp_data, pbmp_mask;
#endif /* BCM_TRIDENT_SUPPORT */

    /* Any future non-TCAM init should be put here */

    /* TCAM info analysis */
    if (NULL == _soc_ser_info_p[unit]) {
        return SOC_E_NONE;
    }

    /* Clean up any previous state */
    _soc_mem_scan_info_free(unit);

    ser_idx = 0;
    generic = _soc_ser_info_is_generic[unit];

    if (generic) {
        gen_ser_info = (_soc_generic_ser_info_t *)_soc_ser_info_p[unit];
        ser_info = NULL;
    } else {
        ser_info = (_soc_ser_parity_info_t *)_soc_ser_info_p[unit];
        gen_ser_info = NULL;
    }

    while (generic ? (NULL != gen_ser_info) : (NULL != ser_info)) {
        if (generic) {
            mem = gen_ser_info[ser_idx].mem;
        } else {
            mem = ser_info[ser_idx].mem;
        }

        if (INVALIDm == mem) {
            break;
        }
        if (!SOC_MEM_IS_VALID(unit, mem)) {
            soc_cm_debug(DK_WARN,
                         "Unit:%d SER Scan mem invalid at index %d\n",
                         unit, ser_idx);
            return SOC_E_INTERNAL;
        }
        ser_idx++;
    }

    if (0 == ser_idx) {
        /* No TCAMs to track */
        return SOC_E_NONE;
    }

    alloc_size = sizeof(_soc_mem_scan_info_t);
    if ((scan_info[unit] = sal_alloc(alloc_size, "memscan info")) == NULL) {
        return SOC_E_MEMORY;
    }

    scan_info[unit]->num_tables = ser_idx;

    alloc_size = ser_idx * sizeof(_soc_mem_scan_table_info_t);
    if ((scan_info[unit]->table_info =
         sal_alloc(alloc_size, "memscan table info")) == NULL) {
        sal_free(scan_info[unit]);
        return SOC_E_MEMORY;
    }
    sal_memset(scan_info[unit]->table_info, 0, alloc_size);

    for (ser_idx = 0; ser_idx < scan_info[unit]->num_tables; ser_idx++) {
        table_info = _MEM_SCAN_TABLE_INFO(unit, ser_idx);
        if (generic) {
            mem = gen_ser_info[ser_idx].mem;
            ser_flags = gen_ser_info[ser_idx].ser_flags;
            bits = 0;
            for (idx = 0; idx < SOC_NUM_GENERIC_PROT_SECTIONS; idx++) {
                if (gen_ser_info[ser_idx].start_end_bits[idx].start_bit != -1) {
                    if (bits <
                        gen_ser_info[ser_idx].start_end_bits[idx].end_bit) {
                        bits =
                            gen_ser_info[ser_idx].start_end_bits[idx].end_bit;
                    }
                }
            }
        } else {
            mem = ser_info[ser_idx].mem;
            ser_flags = ser_info[ser_idx].ser_flags;
            bits = ser_info[ser_idx].bit_offset;
        }

        table_info->mem = mem;
        table_info->ser_flags = ser_flags;
 
        entry_dw = soc_mem_entry_words(unit, mem);
        index_cnt = soc_mem_index_count(unit, mem);
        table_info->index_min = soc_mem_index_min(unit, mem);
        table_info->index_max = soc_mem_index_max(unit, mem);
        table_info->entry_dw = entry_dw;
        table_info->size = index_cnt;

#if defined(BCM_TRIDENT_SUPPORT)
        if (0 != (ser_flags & _SOC_SER_FLAG_SW_COMPARE)) {
            no_cache = FALSE;
            SOC_MEM_BLOCK_ITER(unit, mem, blk) {
                cache = SOC_MEM_STATE(unit, mem).cache[blk];
                if (NULL == cache) {
                    no_cache = TRUE;
                }
            }
            if (no_cache) {
                /* Caching is disabled for this memory.
                 * We cannot do SW comparison, so skip the setup.
                 * Disable mem scan on this table.
                 */
                table_info->size = 0;
                continue;
            }
            _soc_mem_scan_ser_tcam_datamask_get(entry_dw, bits,
                                                table_info->mask);
            if (SOC_IS_TD_TT(unit) &&
                (mem == FP_GLOBAL_MASK_TCAMm)) {
                soc_mem_pbmp_field_get(unit, mem, table_info->mask,
                                       IPBMf, &pbmp_data);
                soc_mem_pbmp_field_get(unit, mem, table_info->mask,
                                       IPBM_MASKf, &pbmp_mask);
                if (0 == (ser_flags & _SOC_SER_FLAG_ACC_TYPE_MASK)) {
                    SOC_PBMP_AND(pbmp_data, PBMP_XPIPE(unit));
                    SOC_PBMP_AND(pbmp_mask, PBMP_XPIPE(unit));
                } else {
                    SOC_PBMP_AND(pbmp_data, PBMP_YPIPE(unit));
                    SOC_PBMP_AND(pbmp_mask, PBMP_YPIPE(unit));
                }
                soc_mem_pbmp_field_set(unit, mem, table_info->mask,
                                       IPBMf, &pbmp_data);
                soc_mem_pbmp_field_set(unit, mem, table_info->mask,
                                       IPBM_MASKf, &pbmp_mask);
            }

            _soc_mem_tcam_dm_to_xy(unit, mem, 1,
                                   soc_mem_entry_null(unit, mem),
                                   table_info->null_entry,
                                   NULL);
            
            /* Need the XY tcam cache */
            if (last_mem == mem) {
                /* Same memory but different pipes,
                 * copy the previous info */
                table_info->xy_tcam_cache =
                    _MEM_SCAN_TABLE_INFO(unit, ser_idx - 1)->xy_tcam_cache;
                if ((0 != (ser_flags & _SOC_SER_FLAG_OVERLAY)) &&
                    overlay_created) {
                    table_info->overlay_tcam_bitmap =
                        _MEM_SCAN_TABLE_INFO(unit, ser_idx - 1)->overlay_tcam_bitmap;
                }
            } else {
                /* Time to create the XY cache */
                alloc_size = WORDS2BYTES(index_cnt * entry_dw);
                if ((table_info->xy_tcam_cache =
                     sal_alloc(alloc_size,
                               "memscan XY TCAM info")) == NULL) {
                    _soc_mem_scan_info_free(unit);
                    return SOC_E_MEMORY;
                }
                sal_memset(table_info->xy_tcam_cache, 0, alloc_size);
                stat->mem_cache_count++;
                stat->mem_cache_size += alloc_size;

                if (0 != (ser_flags & _SOC_SER_FLAG_OVERLAY)) {
                    if (overlay_created) {
                        table_info->overlay_tcam_bitmap =
                            _MEM_SCAN_TABLE_INFO(unit, ser_idx - 1)->overlay_tcam_bitmap;
                    } else {
                        alloc_size = SHR_BITALLOCSIZE(index_cnt);
                        if ((table_info->overlay_tcam_bitmap =
                             sal_alloc(alloc_size,
                                 "memscan overlay TCAM info")) == NULL) {
                            _soc_mem_scan_info_free(unit);
                            return SOC_E_MEMORY;
                        }
                        sal_memset(table_info->overlay_tcam_bitmap, 0,
                                   alloc_size);
                        overlay_created = TRUE;
                    }
                }
                /* Copy the current state of the cache */
                SOC_MEM_BLOCK_ITER(unit, mem, blk) {
                    cache = SOC_MEM_STATE(unit, mem).cache[blk];
                    vmap = SOC_MEM_STATE(unit, mem).vmap[blk];
                    if ((0 != (ser_flags & _SOC_SER_FLAG_OVERLAY)) &&
                        (0 != (ser_flags & _SOC_SER_FLAG_OVERLAY_CASE))) {
                        slice_last_idx = -1;
                    } else {
                        /* For any non-overlay table, skip all of the
                         * complex logic in the loop below.  Just
                         * convert the DM cache to XY. */
                        slice_last_idx = table_info->index_max;
                    }
                    overlay = FALSE;
                    for (idx = table_info->index_min;
                         idx <= table_info->index_max;
                         idx++) {
			if (!CACHE_VMAP_TST(vmap, idx)) {
			    continue;
			}
                        if (idx > slice_last_idx) {
                            /* We must be the overlay table, in a new slice.
                             * Get the new the slice index limit, and
                             * check if this range is overlay view. */
                            rv = _soc_mem_scan_overlay_range_get(unit, idx,
                                               &slice_last_idx, &overlay);
                            if (SOC_FAILURE(rv)) {
                                _soc_mem_scan_info_free(unit);
                                return rv;
                            }
                        }
                        cache_offset = idx * entry_dw;
                        /* Is this a valid overlay entry? Note that. */
                        if (overlay &&
                            (soc_FP_GM_FIELDSm_field32_get(unit,
                                       cache + cache_offset, VALIDf))) {
                            SHR_BITSET(table_info->overlay_tcam_bitmap, idx);
                        }
                        /* Synchronized entry available,
                         * convert if necessary */
                        _soc_mem_tcam_dm_to_xy(unit, mem, 1,
                                               cache + cache_offset,
                                 table_info->xy_tcam_cache + cache_offset,
                                               NULL);
                    }
                }
            }
        }
        last_mem = mem;
#endif /* BCM_TRIDENT_SUPPORT */
    }

    return SOC_E_NONE;
}

void
soc_mem_scan_tcam_cache_update(int unit, soc_mem_t mem,
                               int index_begin, int index_end,
                               uint32 *xy_entries)
{
    int            ser_idx, num_tables, entry_dw, num_entries;
    uint32         ser_flags;
    uint32         *cache;
    _soc_mem_scan_table_info_t *table_info = NULL;
    SHR_BITDCL     *overlay_bitmap;

    if (NULL == scan_info[unit]) {
        /* Not yet initialized */
        return;
    }

    num_tables = scan_info[unit]->num_tables;
    for (ser_idx = 0; ser_idx < num_tables; ser_idx++) {
        table_info = _MEM_SCAN_TABLE_INFO(unit, ser_idx);
        if (mem == table_info->mem) {
            break;
        }
    }

    if ((ser_idx == num_tables) || (NULL == table_info) ||
        (0 == (table_info->ser_flags &_SOC_SER_FLAG_SW_COMPARE))) {
        /* Nothing to do */
        return;
    }

    ser_flags = table_info->ser_flags;
    entry_dw = table_info->entry_dw;
    cache = table_info->xy_tcam_cache;
    overlay_bitmap = table_info->overlay_tcam_bitmap;
    num_entries = index_end - index_begin + 1;

    sal_memcpy(cache + (index_begin * entry_dw), xy_entries,
               WORDS2BYTES(num_entries * entry_dw));

    if (0 != (ser_flags & _SOC_SER_FLAG_OVERLAY)) {
        /* Record last overlay table view to write to HW */
        if (0 != (ser_flags & _SOC_SER_FLAG_OVERLAY_CASE)) {
            SHR_BITSET_RANGE(overlay_bitmap, index_begin, num_entries);
        } else {
            SHR_BITCLR_RANGE(overlay_bitmap, index_begin, num_entries);
        }
    }
}


/*
 * Function:
 * 	soc_mem_scan_running
 * Purpose:
 *   	Boolean to indicate if the memory scan thread is running
 * Parameters:
 *	unit - unit number.
 *	rate - (OUT) if non-NULL, number of entries scanned per interval
 *	interval - (OUT) if non-NULL, receives current wake-up interval.
 */

int
soc_mem_scan_running(int unit, int *rate, sal_usecs_t *interval)
{
    soc_control_t	*soc = SOC_CONTROL(unit);

    if (soc->mem_scan_pid  != SAL_THREAD_ERROR) {
        if (rate != NULL) {
            *rate = soc->mem_scan_rate;
        }

        if (interval != NULL) {
            *interval = soc->mem_scan_interval;
        }
    }

    return (soc->mem_scan_pid != SAL_THREAD_ERROR);
}

/*
 * Function:
 * 	soc_mem_scan_start
 * Purpose:
 *   	Start memory scan thread
 * Parameters:
 *	unit - unit number.
 *	rate - maximum number of entries to scan each time thread runs
 *	interval - how often the thread should run (microseconds).
 * Returns:
 *	SOC_E_MEMORY if can't create thread.
 */

int
soc_mem_scan_start(int unit, int rate, sal_usecs_t interval)
{
    soc_control_t	*soc = SOC_CONTROL(unit);
    int			pri;

    if (soc->mem_scan_pid != SAL_THREAD_ERROR) {
	SOC_IF_ERROR_RETURN(soc_mem_scan_stop(unit));
    }

    sal_snprintf(soc->mem_scan_name, sizeof (soc->mem_scan_name),
		 "bcmMEM_SCAN.%d", unit);

    soc->mem_scan_rate = rate;
    soc->mem_scan_interval = interval;

    if (interval == 0) {
	return SOC_E_NONE;
    }

    if (NULL == _soc_ser_info_p[unit]) {
        SOC_IF_ERROR_RETURN(_soc_mem_scan_info_init(unit));
    }

    if (soc->mem_scan_pid == SAL_THREAD_ERROR) {
	pri = soc_property_get(unit, spn_MEM_SCAN_THREAD_PRI, 50);
	soc->mem_scan_pid = sal_thread_create(soc->mem_scan_name,
					      SAL_THREAD_STKSZ,
					      pri,
					      _soc_mem_scan_thread,
					      INT_TO_PTR(unit));

	if (soc->mem_scan_pid == SAL_THREAD_ERROR) {
	    soc_cm_debug
		(DK_ERR,
		 "soc_mem_scan_start: Could not start mem_scan thread\n");
	    return SOC_E_MEMORY;
	}
    }

    return SOC_E_NONE;
}

/*
 * Function:
 * 	soc_mem_scan_stop
 * Purpose:
 *   	Stop memory scan thread
 * Parameters:
 *	unit - unit number.
 * Returns:
 *	SOC_E_XXX
 */

int
soc_mem_scan_stop(int unit)
{
    soc_control_t	*soc = SOC_CONTROL(unit);
    int			rv = SOC_E_NONE;
    soc_timeout_t	to;

    soc->mem_scan_interval = 0;		/* Request exit */

    /* check if needs support for sirius */
#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
      return SOC_E_INTERNAL;
    }
#endif

    if (soc->mem_scan_pid != SAL_THREAD_ERROR) {
	/* Wake up thread so it will check the exit flag */
	sal_sem_give(soc->mem_scan_notify);

	/* Give thread a few seconds to wake up and exit */
#ifdef PLISIM
#	define	TOSEC	15
#else
#	define	TOSEC	5
#endif
	soc_timeout_init(&to, TOSEC * 1000000, 0);

	while (soc->mem_scan_pid != SAL_THREAD_ERROR) {
	    if (soc_timeout_check(&to)) {
		soc_cm_debug(DK_ERR,
			     "soc_mem_scan_stop: thread will not exit\n");
		rv = SOC_E_INTERNAL;
		break;
	    }
	}
    }

    _soc_mem_scan_info_free(unit);

    return rv;
}

#if defined(BCM_TRIDENT_SUPPORT)
#define SOC_MEM_SCAN_READ_LOCK(_unit_, _mem_, _ser_flags_)      \
            if (0 != (_ser_flags_ & _SOC_SER_FLAG_OVERLAY)) {   \
                 MEM_LOCK(_unit_, FP_GLOBAL_MASK_TCAMm);        \
                 MEM_LOCK(_unit_, FP_GM_FIELDSm);               \
            } else MEM_LOCK(_unit_, _mem_)
        
#define SOC_MEM_SCAN_READ_UNLOCK(_unit_, _mem_, _ser_flags_)    \
            if (0 != (_ser_flags_ & _SOC_SER_FLAG_OVERLAY)) {   \
                 MEM_UNLOCK(_unit_, FP_GM_FIELDSm);             \
                 MEM_UNLOCK(_unit_, FP_GLOBAL_MASK_TCAMm);      \
            } else MEM_UNLOCK(_unit_, _mem_)
#else
#define SOC_MEM_SCAN_READ_LOCK(_unit_, _mem_, _ser_flags_)      \
            MEM_LOCK(_unit_, _mem_)
        
#define SOC_MEM_SCAN_READ_UNLOCK(_unit_, _mem_, _ser_flags_)    \
            MEM_UNLOCK(_unit_, _mem_)

#endif
        
/*
 * Function:
 *     _soc_mem_scan_ser_read_ranges
 * Purpose:
 *       Perform TCAM SER engine protected tables ranged reads,
 *       or SW comparison of non-HW protected tables
 * Parameters:
 *    unit_vp - StrataSwitch unit # (as a void *).
 * Returns:
 *    SOC_E_XXX
 * Notes:
 */

STATIC int
_soc_mem_scan_ser_read_ranges(int unit, int chunk_size, uint32 *read_buf)
{
    soc_control_t  *soc = SOC_CONTROL(unit);
    int            rv = SOC_E_NONE;
    int            interval;
    int            entries_interval;
    int            entries_pass, scan_iter = 0;
    soc_mem_t      mem;
    int            entry_dw;
    int            blk;
    int            idx, idx_count, test_count, i, dw;
    uint32         *cache;
    uint32         *null_entry;
    uint32         *mask;
    uint32         *cmp_entry, *hw_entry;
    uint8          *vmap;
    SHR_BITDCL     *overlay_bitmap;
    _soc_mem_scan_table_info_t *table_info;
    int            ser_idx, num_tables;
    uint32         ser_flags;
#ifdef BCM_ESW_SUPPORT
    char errstr[80 + SOC_MAX_MEM_WORDS * 9];
    uint8          at;
    uint32         error_index, err_addr;
    _soc_ser_correct_info_t spci;
    soc_stat_t     *stat = SOC_STAT(unit);
#endif /* BCM_ESW_SUPPORT */

    if (NULL == _soc_ser_info_p[unit]) {
        return SOC_E_NONE;
    }

    entries_interval = 0;
    entries_pass = 0;

    if (NULL == scan_info[unit]) {
       /* Not yet initialized, but SER info was checked above,
        * so do init now.  */
        SOC_IF_ERROR_RETURN(_soc_mem_scan_info_init(unit));
    }

    num_tables = scan_info[unit]->num_tables;

    cache = NULL;
    null_entry = NULL;
    mask = NULL;
    vmap = NULL;
    overlay_bitmap = NULL;

    for (ser_idx = 0; ser_idx < num_tables; ser_idx++) {
        if (soc->mem_scan_interval == 0) {
            break;
        }
        table_info = _MEM_SCAN_TABLE_INFO(unit, ser_idx);
        mem = table_info->mem;

        if (0 == table_info->size) {
            /* This configuration doesn't use the memory, skip */
            continue;
        }
        ser_flags = table_info->ser_flags;
        entry_dw = table_info->entry_dw;

        SOC_MEM_BLOCK_ITER(unit, mem, blk) {
            if (soc->mem_scan_interval == 0) {
                break;
            }

            if (0 != (ser_flags & _SOC_SER_FLAG_SW_COMPARE)) {
                cache = table_info->xy_tcam_cache;
                vmap = SOC_MEM_STATE(unit, mem).vmap[blk];
                null_entry = table_info->null_entry;
                mask = table_info->mask;
                overlay_bitmap = table_info->overlay_tcam_bitmap;
            }
            
            for (idx = table_info->index_min;
                 idx <= table_info->index_max;
                 idx += idx_count) {
                if (soc->mem_scan_interval == 0) {
                    break;
                }

                idx_count = table_info->index_max - idx + 1;

                if (idx_count > chunk_size) {
                    idx_count = chunk_size;
                }

                if (entries_interval + idx_count > soc->mem_scan_rate) {
                    idx_count = soc->mem_scan_rate - entries_interval;
                }

                SOC_MEM_SCAN_READ_LOCK(unit, mem, ser_flags);
                    
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || \
        defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
                soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                             "Scan: unit=%d %s%s.%s[%d-%d]\n", unit, 
                             SOC_MEM_NAME(unit, mem), 
                             (ser_flags & _SOC_SER_FLAG_ACC_TYPE_MASK) ?
                             "(Y)" : "",
                             SOC_BLOCK_NAME(unit, blk),
                             idx, idx + idx_count - 1);
#endif

                if (0 != (ser_flags & _SOC_SER_FLAG_XY_READ)) {
                    rv = soc_mem_ser_read_range(unit, mem, blk,
                                                idx, idx + idx_count - 1,
                                                ser_flags, read_buf);
                } else {
                    rv = soc_mem_read_range(unit, mem, blk,
                                            idx, idx + idx_count - 1, 
                                            read_buf); 	           
                }

                if (rv < 0) {
                    soc_cm_debug(DK_ERR,
                           "soc_mem_scan_thread: TCAM read failed: %s\n",
                                 soc_errmsg(rv));
                    soc_event_generate(unit,
                                       SOC_SWITCH_EVENT_THREAD_ERROR, 
                                       SOC_SWITCH_EVENT_THREAD_MEMSCAN, 
                                       __LINE__, rv);
                    SOC_MEM_SCAN_READ_UNLOCK(unit, mem, ser_flags);
                    return rv;
                }

                entries_interval += idx_count;
                test_count = 
                    (ser_flags & _SOC_SER_FLAG_SW_COMPARE) ? idx_count : 0;

                for (i = 0; i < test_count; i++) {
                    if (soc->mem_scan_interval == 0) {
                        break;
                    }

                    if (0 != (ser_flags & _SOC_SER_FLAG_OVERLAY)) {
                        if (0 != (ser_flags & _SOC_SER_FLAG_OVERLAY_CASE)) {
                            if (!SHR_BITGET(overlay_bitmap, idx + i)) {
                                /* Not this table view, skip entry */
                                continue;
                            }
                        } else {
                            if (SHR_BITGET(overlay_bitmap, idx + i)) {
                                /* Not this table view, skip entry */
                                continue;
                            }
                        }
                    }

                    hw_entry = &(read_buf[i * entry_dw]);
                    if (CACHE_VMAP_TST(vmap, idx + i)) {
                        cmp_entry = &(cache[(idx + i) * entry_dw]);
                    } else {
                        cmp_entry = null_entry;
                    }

                    for (dw = 0; dw < entry_dw; dw++) {
                        if (((hw_entry[dw] ^ cmp_entry[dw]) &
                             mask[dw]) != 0) {
                            break;
                        }
                    }
                    if (dw < entry_dw) {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || \
        defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
                        soc_cm_debug(DK_WARN,
                                     "Memory error detected on unit %d "
                                     "in %s%s.%s[%d]\n", unit, 
                                     SOC_MEM_NAME(unit, mem),
                               (ser_flags & _SOC_SER_FLAG_ACC_TYPE_MASK) ?
                                     "(Y)" : "",
                                     SOC_BLOCK_NAME(unit, blk),
                                     idx + i);
#endif

#ifdef BCM_ESW_SUPPORT
                        error_index = idx + i;
                        errstr[0] = 0;
                        for (dw = 0; dw < entry_dw; dw++) {
                            sal_sprintf(errstr + sal_strlen(errstr),
                                        " %08x", 
                                        cmp_entry[dw]);
                        }
                        soc_cm_debug(DK_WARN, "    WAS:%s\n", errstr);

                        errstr[0] = 0;
                        for (dw = 0; dw < entry_dw; dw++) {
                            sal_sprintf(errstr + sal_strlen(errstr),
                                        " %08x", 
                                        hw_entry[dw]);
                        }
                        soc_cm_debug(DK_WARN, "    BAD:%s\n", errstr);

                        err_addr = soc_mem_addr_get(unit, mem, 0, blk, 
                                      error_index, &at);
                        soc_event_generate(unit,
                                           SOC_SWITCH_EVENT_PARITY_ERROR, 
                                           0, err_addr, 0);
                        stat->ser_err_tcam++;
                        sal_memset(&spci, 0, sizeof(spci));
                        spci.flags = SOC_SER_SRC_MEM | SOC_SER_REG_MEM_KNOWN;
                        spci.reg = INVALIDr;
                        spci.mem = mem;
                        spci.blk_type = blk;
                        spci.index = error_index;
                        if (0 == (ser_flags & _SOC_SER_FLAG_ACC_TYPE_MASK)) {
                            spci.acc_type = at;
                            spci.pipe_num = 0;
                        } else {
                            spci.acc_type =
                                ser_flags & _SOC_SER_FLAG_ACC_TYPE_MASK;
                            spci.pipe_num = 1;
                        }
                        if ((rv = soc_ser_correction(unit, &spci)) < 0) {
                            soc_cm_debug(DK_WARN,
                                         "    CORRECTION FAILED: %s\n", 
                                         soc_errmsg(rv));
                            soc_event_generate(unit, 
                                      SOC_SWITCH_EVENT_THREAD_ERROR,
                                      SOC_SWITCH_EVENT_THREAD_MEMSCAN,
                                               __LINE__, rv);
                            SOC_MEM_SCAN_READ_UNLOCK(unit, mem, ser_flags);
                            return rv;
                        }
#endif /* BCM_ESW_SUPPORT */
                    } 
                    entries_pass++;
                }

                SOC_MEM_SCAN_READ_UNLOCK(unit, mem, ser_flags);
            
                if ((interval = soc->mem_scan_interval) != 0) {
                    if (entries_interval == soc->mem_scan_rate) {
                        sal_sem_take(soc->mem_scan_notify, interval);
                        entries_interval = 0;
                    }
                } else {
                    return rv;
                }
            }
        }
    } 
    soc_cm_debug(DK_TESTS,
                 "Done: %d mem scan iterations\n", ++scan_iter);

    return rv;
}

/*
 * Function:
 * 	_soc_mem_scan_thread (internal)
 * Purpose:
 *   	Thread control for L2 shadow table maintenance
 * Parameters:
 *	unit_vp - StrataSwitch unit # (as a void *).
 * Returns:
 *	Nothing
 * Notes:
 *	Exits when mem_scan_interval is set to zero and semaphore is given.
 */

STATIC void
_soc_mem_scan_thread(void *unit_vp)
{
    int			unit = PTR_TO_INT(unit_vp);
    soc_control_t	*soc = SOC_CONTROL(unit);
    int			rv;
    int			interval;
    int			chunk_size;
    int			entries_interval;
    int			entries_pass;
    uint32		*read_buf = NULL;
    uint32		mask[SOC_MAX_MEM_WORDS];
    soc_mem_t		mem;
    int			entry_dw;
    int			blk;
    int			idx, idx_count, i, dw;
    uint32		*cache = NULL;
    uint8		*vmap = NULL;

    chunk_size = soc_property_get(unit, spn_MEM_SCAN_CHUNK_SIZE, 256);

    read_buf = soc_cm_salloc(unit,
			     chunk_size * SOC_MAX_MEM_WORDS * 4,
			     "mem_scan_new");

    if (read_buf == NULL) {
	soc_cm_debug(DK_ERR,
		     "soc_mem_scan_thread: not enough memory, exiting\n");
        soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                           SOC_SWITCH_EVENT_THREAD_MEMSCAN, __LINE__, 
                           SOC_E_MEMORY);
	goto cleanup_exit;
    }

    /*
     * Implement the sleep using a semaphore timeout so if the task is
     * requested to exit, it can do so immediately.
     */

    entries_interval = 0;

    while ((interval = soc->mem_scan_interval) != 0) {
        entries_pass = 0;

        if (soc_feature(unit, soc_feature_ser_parity)) {
            rv = _soc_mem_scan_ser_read_ranges(unit, chunk_size, read_buf);

            if (rv < 0) {
                soc_cm_debug(DK_ERR,
                             "soc_mem_scan_thread: TCAM scan failed: %s\n",
                             soc_errmsg(rv));
                soc_event_generate(unit,
                                   SOC_SWITCH_EVENT_THREAD_ERROR, 
                                   SOC_SWITCH_EVENT_THREAD_MEMSCAN, 
                                   __LINE__, rv);
                goto cleanup_exit;
            }
            /* For now, only the TCAM entries are scanned */
            if (soc->mem_scan_interval != 0) {
                sal_sem_take(soc->mem_scan_notify, interval);
                entries_interval = 0;
            }
            continue;
        }

        for (mem = 0; mem < NUM_SOC_MEM; mem++) {
            if (!SOC_MEM_IS_VALID(unit, mem)) {
                continue;
            }
            if (soc_feature(unit, soc_feature_ser_parity)) {
                if ((SOC_MEM_INFO(unit, mem).flags &
                      SOC_MEM_FLAG_SER_ENGINE)) {
                    /* Handled separately above */
                    continue;
                }
            }
            if (SOC_IS_ROBO(unit)) {
#ifdef BCM_ROBO_SUPPORT
                soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                             "Unit:%d Scan mem: %s\n",
                             unit, SOC_ROBO_MEM_NAME(unit, mem));
#endif
            } else {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || \
        defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
                soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                             "Unit:%d Scan mem: %s\n",
                             unit, SOC_MEM_NAME(unit, mem));
#endif
            }
	    entry_dw = soc_mem_entry_words(unit, mem);
	    soc_mem_datamask_get(unit, mem, mask);

	    SOC_MEM_BLOCK_ITER(unit, mem, blk) {
		if (soc->mem_scan_interval == 0) {
		    break;
		}

		for (idx = soc_mem_index_min(unit, mem);
		     idx <= soc_mem_index_max(unit, mem);
		     idx += idx_count) {
		    if (soc->mem_scan_interval == 0) {
			break;
		    }

		    idx_count = soc_mem_index_count(unit, mem) - idx;

		    if (idx_count > chunk_size) {
			idx_count = chunk_size;
		    }

		    if (entries_interval + idx_count > soc->mem_scan_rate) {
			idx_count = soc->mem_scan_rate - entries_interval;
		    }

		    MEM_LOCK(unit, mem);
                    
                    /*    coverity[negative_returns : FALSE]    */
                    cache = SOC_MEM_STATE(unit, mem).cache[blk];
                    vmap = SOC_MEM_STATE(unit, mem).vmap[blk];
                        
                    if (cache == NULL) {
                        /* Cache disabled
                         * (possibly since last iteration) */
                        MEM_UNLOCK(unit, mem);
                        continue;
                    }
                        
                    /* Temporarily disable cache for raw hardware read */
                    SOC_MEM_STATE(unit, mem).cache[blk] = NULL;

                    rv = soc_mem_read_range(unit, mem, blk,
                                            idx, idx + idx_count - 1,
                                            read_buf);

                    SOC_MEM_STATE(unit, mem).cache[blk] = cache;
            
                    if (rv < 0) {
                        soc_cm_debug(DK_ERR,
                               "soc_mem_scan_thread: read failed: %s\n",
                                     soc_errmsg(rv));
                        soc_event_generate(unit,
                                           SOC_SWITCH_EVENT_THREAD_ERROR, 
                                           SOC_SWITCH_EVENT_THREAD_MEMSCAN, 
                                           __LINE__, rv);
                        MEM_UNLOCK(unit, mem);
                        goto cleanup_exit;
                    }
                    if (SOC_IS_ROBO(unit)) {
#ifdef BCM_ROBO_SUPPORT                        
                        soc_cm_debug(DK_TESTS,
                                     "Scan: unit=%d %s.%s[%d-%d]\n", unit, 
                                     SOC_ROBO_MEM_NAME(unit, mem), 
                                     SOC_BLOCK_NAME(unit, blk),
                                     idx, idx + idx_count - 1);
#endif
                    } else {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || \
        defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
                        soc_cm_debug(DK_TESTS,
                                     "Scan: unit=%d %s.%s[%d-%d]\n", unit, 
                                     SOC_MEM_NAME(unit, mem), 
                                     SOC_BLOCK_NAME(unit, blk),
                                     idx, idx + idx_count - 1);
#endif
                    }

                    for (i = 0; i < idx_count; i++) {
                        if (!CACHE_VMAP_TST(vmap, idx + i)) {
                            continue;
                        }
                        for (dw = 0; dw < entry_dw; dw++) {
                            if (((read_buf[i * entry_dw + dw] ^
                                  cache[(idx + i) * entry_dw + dw]) &
                                 mask[dw]) != 0) {
                                break;
                            }
                        }
                        if (dw < entry_dw) {
                            char errstr[80 + SOC_MAX_MEM_WORDS * 9];
                            if (SOC_IS_ROBO(unit)) {
#ifdef BCM_ROBO_SUPPORT
                                soc_cm_debug(DK_WARN,
                                       "Memory error detected on unit %d "
                                             "in %s.%s[%d]\n", unit, 
                                             SOC_ROBO_MEM_NAME(unit, mem),
                                             SOC_BLOCK_NAME(unit, blk), idx + i);
#endif /* BCM_ROBO_SUPPORT */
                            } else {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || \
        defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)                            
                                soc_cm_debug(DK_WARN,
                                       "Memory error detected on unit %d "
                                             "in %s.%s[%d]\n", unit, 
                                             SOC_MEM_NAME(unit, mem),
                                             SOC_BLOCK_NAME(unit, blk),
                                             idx + i);
#endif
                            }
                            errstr[0] = 0;
                            for (dw = 0; dw < entry_dw; dw++) {
                                sal_sprintf(errstr + sal_strlen(errstr),
                                            " %08x", 
                                            cache[(idx + i) *
                                                  entry_dw + dw]);
                            }
                            soc_cm_debug(DK_WARN, "    WAS:%s\n", errstr);

                            errstr[0] = 0;
                            for (dw = 0; dw < entry_dw; dw++) {
                                sal_sprintf(errstr + sal_strlen(errstr),
                                            " %08x", 
                                            read_buf[i * entry_dw + dw]);
                            }
                            soc_cm_debug(DK_WARN, "    BAD:%s\n", errstr);

                            if ((rv = soc_mem_write(unit, mem, blk, idx + i,
                                        cache + (idx + i) * entry_dw)) < 0) {
                                soc_cm_debug(DK_WARN,
                                             "    CORRECTION FAILED: %s\n", 
                                             soc_errmsg(rv));
                                soc_event_generate(unit, 
                                          SOC_SWITCH_EVENT_THREAD_ERROR, 
                                          SOC_SWITCH_EVENT_THREAD_MEMSCAN,
                                                   __LINE__, rv);
                                MEM_UNLOCK(unit, mem);
                                goto cleanup_exit;
                            } else {
                                soc_cm_debug(DK_WARN,
                                             "    Corrected by writing back cached data\n");
                            }
                        } 
    
                        entries_pass++;
                    }
                    MEM_UNLOCK(unit, mem);
            
                    entries_interval += idx_count;
            
                    if (entries_interval == soc->mem_scan_rate) {
                        sal_sem_take(soc->mem_scan_notify, interval);
                        entries_interval = 0;
                    }
                }
            }
        }

        soc_cm_debug(DK_TESTS,
                     "Done: %d mem entries checked\n", entries_pass);
        if (soc->mem_scan_interval != 0) {
            /* Extra sleep in main loop, in case no caches are enabled. */
            sal_sem_take(soc->mem_scan_notify, interval);
            entries_interval = 0;
        }
    }

cleanup_exit:

    if (read_buf != NULL) {
        soc_cm_sfree(unit, read_buf);
    }

    soc->mem_scan_pid = SAL_THREAD_ERROR;
    sal_thread_exit(0);
}

#endif    /* INCLUDE_MEM_SCAN */
