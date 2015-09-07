/*
 * $Id: counter.c 1.49 Broadcom SDK $
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
 * Counter CLI support
 */

#include <appl/diag/system.h>
#include <appl/diag/dport.h>

#include <bcm/stat.h>
#include <bcm/error.h>
#include <soc/mem.h>

/*
 * Data structure for saving the previous value of counters so we can
 * tell which counters that have changed since last shown.
 */

STATIC uint64 *counter_val[SOC_MAX_NUM_DEVICES];

STATIC void
counter_val_set(int unit, soc_port_t port, soc_reg_t ctr_reg,
                int ar_idx, uint64 val)
{
    static int		n;
    int			ind;
    soc_counter_non_dma_t *non_dma;

    if (counter_val[unit] == NULL) {

	n = SOC_CONTROL(unit)->counter_n32 + SOC_CONTROL(unit)->counter_n64 +
            SOC_CONTROL(unit)->counter_n64_non_dma;
	counter_val[unit] = sal_alloc(n * sizeof (uint64), "save_ctrs");

	if (counter_val[unit] == NULL) {
	    return;
	}

	sal_memset(counter_val[unit], 0, n * sizeof (uint64));
    }

    if (ctr_reg >= SOC_COUNTER_NON_DMA_START && 
        ctr_reg < SOC_COUNTER_NON_DMA_END) { 
        if (ar_idx < 0) { 
            if (SOC_CONTROL(unit)->counter_non_dma == NULL) { 
                return; 
            } 
            non_dma = &SOC_CONTROL(unit)-> 
            counter_non_dma[ctr_reg - SOC_COUNTER_NON_DMA_START]; 
            for (ar_idx = 0; ar_idx < non_dma->entries_per_port; ar_idx++) { 
                counter_val_set(unit, port, ctr_reg, ar_idx, val); 
            } 
            return; 
        } 
    }

    ind = soc_counter_idx_get(unit, ctr_reg, ar_idx, port);

    if (ctr_reg >= SOC_COUNTER_NON_DMA_END) {
        soc_cm_debug(DK_COUNTER,
                     "cval_set: Illegal counter index -- "
                     "ar_idx=%d p=%d idx=%d vh=%d vl=%d\n",
                     ar_idx, port, ind,
                     COMPILER_64_HI(val),
                     COMPILER_64_LO(val));

    } else if (ctr_reg >= SOC_COUNTER_NON_DMA_START &&
               ctr_reg < SOC_COUNTER_NON_DMA_END) {
        non_dma =
            &SOC_CONTROL(unit)->counter_non_dma[ctr_reg -
                                                SOC_COUNTER_NON_DMA_START];
        soc_cm_debug(DK_COUNTER,
                     "cval_set: %s ar_idx=%d p=%d idx=%d vh=%d vl=%d\n",
                     non_dma->cname,
                     ar_idx, port, ind,
                     COMPILER_64_HI(val),
                     COMPILER_64_LO(val));
    } else {
        soc_cm_debug(DK_COUNTER,
                     "cval_set: %s ar_idx=%d p=%d idx=%d vh=%d vl=%d\n",
                     SOC_REG_NAME(unit, ctr_reg),
                     ar_idx, port, ind,
                     COMPILER_64_HI(val),
                     COMPILER_64_LO(val));
    }

    /* Unsupported non-DMA counters will return out-of-range index (-1) */
    if (ind >= 0 && ind < n) {
        counter_val[unit][ind] = val;
    }
}

void
counter_val_set_by_port(int unit, soc_pbmp_t pbmp, uint64 val)
{
    int			i;
    soc_port_t		port, dport;
    soc_reg_t		ctr_reg;
    soc_cmap_t          *cmap;

    DPORT_SOC_PBMP_ITER(unit, pbmp, dport, port) {
        /* Non-DMA counters */ 
        for (i = SOC_COUNTER_NON_DMA_START; i < SOC_COUNTER_NON_DMA_END; i++) {
            counter_val_set(unit, port, i, -1, val);
        }
        /* DMA counters */
        cmap = soc_port_cmap_get(unit, port);
	if (cmap == NULL) {
	    continue;
	}
        for (i = 0; i < cmap->cmap_size; i++) {
            ctr_reg = cmap->cmap_base[i].reg;
            if (ctr_reg != INVALIDr) {
                counter_val_set(unit, port, ctr_reg, -1, val);
            }
        }
    }
}

STATIC void
counter_val_get(int unit, soc_port_t port, soc_reg_t ctr_reg,
                int ar_idx, uint64 *val)
{
    int			ind;
    soc_counter_non_dma_t *non_dma;

    if (counter_val[unit] == NULL) {
	COMPILER_64_SET(*val, 0, 0);
    } else {
	ind = soc_counter_idx_get(unit, ctr_reg, ar_idx, port);

	*val = counter_val[unit][ind];

        if (ctr_reg >= SOC_COUNTER_NON_DMA_END) {
            soc_cm_debug(DK_COUNTER,
                         "cval_get: Illegal counter index -- "
                         "ar_idx=%d p=%d idx=%d vh=%d vl=%d\n",
                         ar_idx, port, ind,
                         COMPILER_64_HI(*val),
                         COMPILER_64_LO(*val));

        } else if (ctr_reg >= SOC_COUNTER_NON_DMA_START &&
                   ctr_reg < SOC_COUNTER_NON_DMA_END) {
            non_dma =
                &SOC_CONTROL(unit)->counter_non_dma[ctr_reg -
                                               SOC_COUNTER_NON_DMA_START];
            soc_cm_debug(DK_COUNTER,
                         "cval_get: %s ar_idx=%d p=%d idx=%d vh=%d vl=%d\n",
                         non_dma->cname,
                         ar_idx, port, ind,
                         COMPILER_64_HI(*val),
                         COMPILER_64_LO(*val));
        } else {
            soc_cm_debug(DK_COUNTER,
                         "cval_get: %s ar_idx=%d p=%d idx=%d vh=%d vl=%d\n",
                         SOC_REG_NAME(unit, ctr_reg),
                         ar_idx, port, ind,
                         COMPILER_64_HI(*val),
                         COMPILER_64_LO(*val));
        }
    }
}

/*
 * Function:
 *	do_resync_counters
 * Purpose:
 *	Resynchronize the previous counter values (stored in counter_val)
 *	with the actual counters.
 */

int
do_resync_counters(int unit, soc_pbmp_t pbmp)
{
    soc_port_t		port, dport;
    int			i;
    soc_cmap_t          *cmap;
    soc_reg_t           reg;
    uint64		val;
    int                 ar_idx;

    DPORT_SOC_PBMP_ITER(unit, pbmp, dport, port) {
        /* Non-DMA counters */ 
        for (i = SOC_COUNTER_NON_DMA_START; i < SOC_COUNTER_NON_DMA_END; i++) {
            if (soc_counter_get(unit, port, i, -1, &val) == SOC_E_NONE )
                counter_val_set(unit, port, i, -1, val);
        }
        /* DMA counters */
        cmap = soc_port_cmap_get(unit, port);
	for (i = 0; i < cmap->cmap_size; i++) {
	    reg = cmap->cmap_base[i].reg;
	    if (reg != INVALIDr) {
                if (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_ARRAY) {
                    for (ar_idx = 0;
                         ar_idx < SOC_REG_INFO(unit, reg).numels;
                         ar_idx++) {
                        if (soc_counter_get(unit, port, reg, ar_idx, &val) == SOC_E_NONE )
                             counter_val_set(unit, port, reg, ar_idx, val);
                    }
                } else {
                    if (soc_counter_get(unit, port, reg, 0, &val) == SOC_E_NONE )
                          counter_val_set(unit, port, reg, 0, val);
                }
	    }
	}
    }

    return 0;
}

/*
 * do_show_counters
 *
 *   Displays counters for specified register(s) and port(s).
 *
 *	SHOW_CTR_HEX displays values in hex instead of decimal.
 *
 *	SHOW_CTR_RAW displays only the counter value, not the register
 *	name or delta.
 *
 *	SHOW_CTR_CHANGED: includes counters that have changed since the
 *	last call to do_show_counters().
 *
 *	SHOW_CTR_NOT_CHANGED: includes counters that have not changed since
 *	the last call to do_show_counters().
 *
 *   NOTE: to get any output, you need at least one of the above two flags.
 *
 *	SHOW_CTR_ZERO: includes counters that are zero.
 *
 *	SHOW_CTR_NOT_ZERO: includes counters that are not zero.
 *
 *   NOTE: to get any output, you need at least one of the above two flags.
 *
 *   SHOW_CTR_ED:
 *      Show only those registers marked as Error/Discard.
 *
 *   Columns will remain lined up properly until the following maximum
 *   limits are exceeded:
 *
 *	Current count: 99,999,999,999,999,999
 *	Increment: +99,999,999,999,999
 *	Rate: 999,999,999,999/s
 */

void
do_show_counter(int unit, soc_port_t port, soc_reg_t ctr_reg,
                int ar_idx, int flags)
{
    soc_counter_non_dma_t *non_dma;
    uint64		val, prev_val, diff, rate;
    int                 num_entries;
    int			changed;
    int                 is_ed_cntr, is_mmu_status;
    int			tabwidth = soc_property_get(unit, spn_DIAG_TABS, 8);
    int			commachr = soc_property_get(unit, spn_DIAG_COMMA, ',');

    if (ctr_reg >= SOC_COUNTER_NON_DMA_START &&
        ctr_reg < SOC_COUNTER_NON_DMA_END) {
        if (SOC_CONTROL(unit)->counter_non_dma == NULL) {
            return;
        }
        is_ed_cntr = FALSE;
        is_mmu_status = FALSE;
        non_dma = &SOC_CONTROL(unit)->
            counter_non_dma[ctr_reg - SOC_COUNTER_NON_DMA_START];
        if (non_dma->flags &
            (_SOC_COUNTER_NON_DMA_PEAK | _SOC_COUNTER_NON_DMA_CURRENT)) {
            is_mmu_status = TRUE;
        }
        if (ar_idx < 0) {
            num_entries =
                port < 0 ? non_dma->num_entries : non_dma->entries_per_port;
            for (ar_idx = 0; ar_idx < num_entries; ar_idx++) {
                do_show_counter(unit, port, ctr_reg, ar_idx, flags);
            }
            return;
        }
    } else {
        is_ed_cntr = SOC_REG_INFO(unit, ctr_reg).flags & SOC_REG_FLAG_ED_CNTR;
        is_mmu_status = FALSE;

        if (!(SOC_REG_INFO(unit, ctr_reg).flags & SOC_REG_FLAG_ARRAY)) {
            ar_idx = 0;
        } else {
            if (ar_idx < 0) { /* Set all elts of array */
                for (ar_idx = 0; ar_idx < SOC_REG_INFO(unit, ctr_reg).numels;
                     ar_idx++) {
                    do_show_counter(unit, port, ctr_reg, ar_idx, flags);
                }
                return;
            }
        }
    }

    if (soc_counter_get(unit, port, ctr_reg, ar_idx, &val) < 0) {
        return;
    }
    counter_val_get(unit, port, ctr_reg, ar_idx, &prev_val);

    soc_counter_get_rate(unit, port, ctr_reg, ar_idx, &rate);

    diff = val;
    COMPILER_64_SUB_64(diff, prev_val);

    if (COMPILER_64_IS_ZERO(diff)) {
	changed = 0;
    } else {
	counter_val_set(unit, port, ctr_reg, ar_idx, val);
	changed = 1;
    }

    if ((( changed && (flags & SHOW_CTR_CHANGED)) ||
	 (!changed && (flags & SHOW_CTR_SAME))) &&
	(( COMPILER_64_IS_ZERO(val) && (flags & SHOW_CTR_Z)) ||
	 (!COMPILER_64_IS_ZERO(val) && (flags & SHOW_CTR_NZ))) &&
        ((!(flags & SHOW_CTR_ED) ||
          ((flags & SHOW_CTR_ED) && is_ed_cntr))) &&
        ((!(flags & SHOW_CTR_MS) && !is_mmu_status)||
          (((flags & SHOW_CTR_MS) && is_mmu_status)))
        ) {
    /* size of line must be equal or more then sum of cname, buf_val, 
      buf_diff and buf_rate. */
    /* The size of tabby must be greater than size of line */

	char		tabby[130], line[120];
	char		cname[28];
	char		buf_val[32], buf_diff[32], buf_rate[32];

        if (ctr_reg >= SOC_COUNTER_NON_DMA_START &&
            ctr_reg < SOC_COUNTER_NON_DMA_END) {
            /* Non-DMA Counters */
            non_dma = &SOC_CONTROL(unit)->counter_non_dma[ctr_reg -
                                                    SOC_COUNTER_NON_DMA_START];
            if(!(SOC_PBMP_MEMBER(non_dma->pbmp, port))) {
                /* Not a dma port member; don't show counters */
                return;
            }
            if(sal_strlen(non_dma->cname) > 13) {
                /* Truncate Name to fit in line */
                sal_memcpy(cname, non_dma->cname, 13);
                if (non_dma->entries_per_port > 1) {
                    sal_sprintf(&cname[13], "(%d).%s", ar_idx, SOC_PORT_NAME(unit, port));
                } else if (non_dma->entries_per_port == 1) {
                    sal_sprintf(&cname[13], ".%s", SOC_PORT_NAME(unit, port));
                } else {
                    sal_sprintf(&cname[13], "(%d)", ar_idx);
                }
            } else {
                if (non_dma->entries_per_port > 1) {
                    sal_sprintf(cname, "%s(%d).%s", non_dma->cname, ar_idx,
                                SOC_PORT_NAME(unit, port));
                } else if (non_dma->entries_per_port == 1) {
                    sal_sprintf(cname, "%s.%s", non_dma->cname,
                                SOC_PORT_NAME(unit, port));
                } else {
                    sal_sprintf(cname, "%s(%d)", non_dma->cname, ar_idx);
                }
            }
        } else  {
            /* DMA Counters */
            if (sal_strlen(SOC_REG_NAME(unit, ctr_reg)) > 13) {
                sal_memcpy(cname, SOC_REG_NAME(unit, ctr_reg), 13);
                /* Truncate Name to fit in line */
                if (SOC_REG_INFO(unit, ctr_reg).flags & SOC_REG_FLAG_ARRAY) {
                    sal_sprintf(&cname[13], "(%d).%s", ar_idx, SOC_PORT_NAME(unit, port));
                } else {
                    sal_sprintf(&cname[13], ".%s", SOC_PORT_NAME(unit, port));
                }
            } else {
                if (SOC_REG_INFO(unit, ctr_reg).flags & SOC_REG_FLAG_ARRAY) {
                    sal_sprintf(cname, "%s(%d).%s", SOC_REG_NAME(unit, ctr_reg),
                                       ar_idx, SOC_PORT_NAME(unit, port));
                } else {
                    sal_sprintf(cname, "%s.%s", SOC_REG_NAME(unit, ctr_reg),
                                               SOC_PORT_NAME(unit, port));
                }
            }
        }

	if (flags & SHOW_CTR_RAW) {
	    if (flags & SHOW_CTR_HEX) {
		sal_sprintf(line, "0x%08x%08x",
			COMPILER_64_HI(val), COMPILER_64_LO(val));
	    } else {
		format_uint64_decimal(buf_val, val, 0);
		sal_sprintf(line, "%s", buf_val);
	    }
	} else {
	    if (flags & SHOW_CTR_HEX) {
                if (is_mmu_status) {
                    sal_sprintf(line, "%-18s: 0x%08x%08x",
                                cname,
                                COMPILER_64_HI(val), COMPILER_64_LO(val));
                } else {
		    sal_sprintf(line, "%-18s: 0x%08x%08x +0x%08x%08x 0x%08x%08x/s",
			cname,
			COMPILER_64_HI(val), COMPILER_64_LO(val),
			COMPILER_64_HI(diff), COMPILER_64_LO(diff),
			COMPILER_64_HI(rate), COMPILER_64_LO(rate));
                }
	    } else {
                if (is_mmu_status) {
		    format_uint64_decimal(buf_val, val, commachr);
		    sal_sprintf(line, "%-24s:%22s", cname, buf_val);
                } else {
		    format_uint64_decimal(buf_val, val, commachr);
		    buf_diff[0] = '+';
		    format_uint64_decimal(buf_diff + 1, diff, commachr);
		    sal_sprintf(line, "%-24s:%22s%20s",
			    cname, buf_val, buf_diff);
		    if (!COMPILER_64_IS_ZERO(rate)) {
		        format_uint64_decimal(buf_rate, rate, commachr);
		        sal_sprintf(line + sal_strlen(line), "%16s/s", buf_rate);
		    }
                }
	    }
	}

	/*
	 * Display is tremendously faster with tabify.
	 * Set diag_tab property to desired value, or 0 to turn it off.
	 */

	tabify_line(tabby, line, tabwidth);

	printk("%s\n", tabby);
    }
}

int
do_show_counters(int unit, soc_reg_t ctr_reg, soc_pbmp_t pbmp, int flags)
{
    soc_control_t	*soc = SOC_CONTROL(unit);
    soc_counter_non_dma_t *non_dma;
    soc_port_t		port, dport;
    int			i;
    soc_cmap_t          *cmap;
    soc_reg_t           reg;
    int                 numregs;
    char		pfmt[SOC_PBMP_FMT_LEN];

    DPORT_SOC_PBMP_ITER(unit, pbmp, dport, port) {
        cmap = soc_port_cmap_get(unit, port);
        if (!cmap) {
	        printk("NOTE: Unit %d: No counter map found for port %d\n", unit, port);
            continue;
        }
        numregs = cmap->cmap_size;

	if (ctr_reg != INVALIDr) {
        /* coverity[negative_returns] : FALSE */
	    do_show_counter(unit, port, ctr_reg, -1, flags);
	} else {
            for (i = 0; i < numregs; i++) {
                reg = cmap->cmap_base[i].reg;
                if (reg != INVALIDr) {
                    do_show_counter(unit, port, reg,
                                    cmap->cmap_base[i].index, flags);
                }
            }
            for (i = SOC_COUNTER_NON_DMA_START; i < SOC_COUNTER_NON_DMA_END;
                 i++) {
                non_dma = &SOC_CONTROL(unit)->
                    counter_non_dma[i - SOC_COUNTER_NON_DMA_START]; 
                if (non_dma->entries_per_port == 0) {
                    continue;
                }
                /* coverity[negative_returns] : FALSE */
                do_show_counter(unit, port, i, -1, flags);
            }
        }
    }
    for (i = SOC_COUNTER_NON_DMA_START; i < SOC_COUNTER_NON_DMA_END; i++) {
        non_dma = &SOC_CONTROL(unit)->
            counter_non_dma[i - SOC_COUNTER_NON_DMA_START]; 
        if (non_dma->entries_per_port != 0) {
            continue;
        }
        /* coverity[negative_returns] : FALSE */
        do_show_counter(unit, -1, i, -1, flags);
    }

    SOC_PBMP_REMOVE(pbmp, soc->counter_pbmp);
    if (soc->counter_interval == 0) {
	printk("NOTE: counter collection is not running\n");
    } else if (SOC_PBMP_NOT_NULL(pbmp)) {
	printk("NOTE: counter collection is not active for ports %s\n",
	       SOC_PBMP_FMT(pbmp, pfmt));
    }

    return 0;
}

/*
 * Counter collection configuration
 */

cmd_result_t
cmd_esw_counter(int unit, args_t *a)
{
    soc_control_t	*soc = SOC_CONTROL(unit);
    soc_pbmp_t		pbmp;
    int			usec, dma, r;
    parse_table_t	pt;
    uint32		flags;
    int			sync = 0;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }


    flags = soc->counter_flags;
    usec = soc->counter_interval;
    SOC_PBMP_ASSIGN(pbmp, soc->counter_pbmp);
    dma = (flags & SOC_COUNTER_F_DMA) != 0;

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "Interval", 	PQ_DFL|PQ_INT,
		    (void *)( 0), &usec, NULL);
    parse_table_add(&pt, "PortBitMap", 	PQ_DFL|PQ_PBMP,
		    (void *)(0), &pbmp, NULL);
    parse_table_add(&pt, "DMA", 	PQ_DFL|PQ_BOOL,
		    INT_TO_PTR(dma), &dma, NULL);

    if (!ARG_CNT(a)) {			/* Display settings */
	printk("Current settings:\n");
	parse_eq_format(&pt);
	parse_arg_eq_done(&pt);
	return(CMD_OK);
    }

    if (parse_arg_eq(a, &pt) < 0) {
	printk("%s: Error: Unknown option: %s\n", ARG_CMD(a), ARG_CUR(a));
	parse_arg_eq_done(&pt);
	return(CMD_FAIL);
    }
    parse_arg_eq_done(&pt);

    if (ARG_CNT(a) > 0 && !sal_strcasecmp(_ARG_CUR(a), "off")) {
	ARG_NEXT(a);
	usec = 0;
    }

    if (ARG_CNT(a) > 0 && !sal_strcasecmp(_ARG_CUR(a), "sync")) {
	ARG_NEXT(a);
	sync = 1;
    }

    if (sync) {
        /* Sync driver info to HW */
	if ((r = soc_counter_sync(unit)) < 0) {
	    printk("%s: Error: Could not sync counters: %s\n",
		   ARG_CMD(a), soc_errmsg(r));
	    return CMD_FAIL;
	}
        /* Sync diag info with driver */
	do_resync_counters(unit, pbmp);
	return CMD_OK;
    }

    if (soc_feature(unit, soc_feature_cpuport_stat_dma)) {
        soc_pbmp_t temp_pbmp;
        SOC_PBMP_CLEAR(temp_pbmp);
        SOC_PBMP_OR(temp_pbmp, PBMP_PORT_ALL(unit));
        SOC_PBMP_OR(temp_pbmp, PBMP_CMIC(unit));
        SOC_PBMP_AND(pbmp, temp_pbmp);
    } else {
        SOC_PBMP_AND(pbmp, PBMP_PORT_ALL(unit));
    }

    if (dma) {
	flags |= SOC_COUNTER_F_DMA;
    } else {
	flags &= ~SOC_COUNTER_F_DMA;
    }

    if (usec > 0) {
	r = soc_counter_start(unit, flags, usec, pbmp);
    } else {
	r = soc_counter_stop(unit);
    }

    if (r < 0) {
	printk("%s: Error: Could not set counter mode: %s\n",
	       ARG_CMD(a), soc_errmsg(r));
	return CMD_FAIL;
    }

    return CMD_OK;
}

/*
 * routines for custom stats 
 */


#define COL_SZ     0x05

#define RDBGC             0x01
#define TDBGC             0x02
#define BOTH              0x03

#define SET_FLAGS         0x01
#define LIST_FLAGS        0x02
#define SHOW_FLAGS        0x03
#define SHOW_COUNTERS     0x04
#define SHOW_TABLE        0x05

#define ERROR_SET_FLAGS(_err) \
        if (cmd_type == SET_FLAGS) { \
            printk("Error: " _err); \
            return CMD_FAIL;  \
        }

char      *head =
    "\n+------------------Programmable Statistics Counters[Port %4s]-------\
-----------+";
char      *menu =
    "\n| Type | No. |       Value       |                Enabled For       \
            |";
char      *line =
    "\n+-------------------------------------------------------------------\
------------+";
char      *trg_s = "\n|      |     |                   | ";

char      *stat_names[] = BCM_STAT_NAME_INITIALIZER;
typedef struct trig_name_s {
    char      *name;
    char      *about;
} trig_name_t;

trig_name_t _trig_names[] = {
    {"RIPD4", "         Rx IPv4 L3 discard packets"},
    {"RIPC4", "         Good rx L3 (V4 packets) includes tunneled"},
    {"RIPHE4", "        Rx IPv4 header error packets"},
    {"IMRP4", "         Routed IPv4 multicast packets"},
    {"RIPD6", "         Rx IPv6 L3 discard packet"},
    {"RIPC6", "         Good rx L3 (V6 packets) includes tunneled"},
    {"RIPHE6", "        Rx IPv6 header error packets"},
    {"IMRP6", "         Routed IPv6 multicast packets"},
    {"RDISC", "         IBP discard and CBP full"},
    {"RUC", "           Good rx unicast (L2+L3) packets"},
    {"RPORTD",
     "        Packets droppped when ingress port is not in forwarding state"},
    {"PDISC",
     "         Rx policy discard - DST_DISCARD, SRC_DISCARD, RATE_CONTROL"},
    {"IMBP", "          Bridged multicast packets"},
    {"RFILDR", "        Packets dropped by FP"},
    {"RIMDR", "         Multicast (L2+L3) packets that are dropped"},
    {"RDROP", "         Port bitmap zero drop condition"},
    {"IRPSE", "         HiGig IPIC pause rx"},
    {"IRHOL", "         HiGig End-to-End HOL rx packets"},
    {"IRIBP", "         HiGig End-to-End IBP rx packets"},
    {"DSL3HE", "        DOS L3 header error packets"},
    {"IUNKHDR", "       Unknown HiGig header type packet"},
    {"DSL4HE", "        DOS L4 header error packets"},
    {"IMIRROR", "       HiGig mirror packet"},
    {"DSICMP", "        DOS ICMP error packets"},
    {"DSFRAG", "        DOS fragment error packets"},
    {"MTUERR",
     "        Packets trapped to CPU due to egress L3 MTU violation"},
    {"RTUN", "          Number of tunnel packets received"},
    {"RTUNE", "         Rx tunnel error packets"},
    {"VLANDR", "        Rx VLAN drops"},
    {"RHGUC", "         Rx HiGig lookup UC cases"},
    {"RHGMC", "         Rx HiGig lookup MC cases"},
    {"MPLS", "          Received MPLS Packets"},
    {"MACLMT", "        packets dropped due to MAC Limit is hit"},
    {"MPLSERR", "       Received MPLS Packets with Error"},
    {"URPFERR", "       L3 src URPF check Fail"},
    {"HGHDRE", "        Higig Header error packets"},
    {"MCIDXE", "        Multicast Index error packets"},
    {"LAGLUP", "        LAG failover loopback packets"},
    {"LAGLUPD", "       LAG backup port down"},
    {"PARITYD", "       Parity error packets"},
    {"VFPDR", "         VLAN FP drop case"},
    {"URPF", "          Unicast RPF drop case"},
    {"DSTDISCARDROP", "      L2/L3 lookup DST_DISCARD drop"},
    {"CLASSBASEDMOVEDROP", " Class based station movement drop"},
    {"MACLMT_NODROP", "      Mac limit exceeded and packet not dropped"},
    {"MACSAEQUALMACDA", "    Dos Attack L2 Packets"},
    {"MACLMT_DROP", "        Mac limit exceeded and packet dropped"},
    {"TGIP4", "         Tx Good IPv4 L3 UC packets"},
    {"TIPD4", "         Tx IPv4 L3 UC Aged and Drop packets"},
    {"TGIPMC4", "       Tx Good IPv4 IPMC packets"},
    {"TIPMCD4", "       Tx IPv4 IPMC Aged and Drop packets"},
    {"TGIP6", "         Tx Good IPv6 L3 UC packets"},
    {"TIPD6", "         Tx IPv6 L3 UC Aged and Drop Packets"},
    {"TGIPMC6", "       Tx Good IPv6 IPMC packets"},
    {"TIPMCD6", "       Tx IPv6 IPMC Aged and Drop packets"},
    {"TTNL", "          Tx Tunnel packets"},
    {"TTNLE", "         Tx Tunnel error packets"},
    {"TTTLD", "         Pkts dropped due to TTL threshold counter"},
    {"TCFID",
     "         Pkts dropped when CFI set & pkt is untagged or L3 \
switched for IPMC"},
    {"TVLAN", "         Tx VLAN tagged packets"},
    {"TVLAND", "        Pkts dropped due to invalid VLAN counter"},
    {"TVXLTMD", "       Pkts dropped due to miss in VXLT table counter"},
    {"TSTGD",
     "         Pkts dropped due to Spanning Tree State not in forwarding \
state"},
    {"TAGED", "         Pkts dropped due to packet aged counter"},
    {"TL2MCD", "        L2 MC packet drop counter"},
    {"TPKTD", "         Pkts dropped due to any condition"},
    {"TMIRR", "         mirroring flag"},
    {"TSIPL", "         SIP Link Local Drop flag"},
    {"THGUC", "         Higig Lookedup L3UC Pkts"},
    {"THGMC", "         Higig Lookedup L3MC Pkts"},
    {"THIGIG2", "       Unknown HiGig2 Drop"},
    {"THGI", "          Unknown HiGig drop"},
    {"TL2_MTU", "       L2 MTU fail drop"},
    {"TPARITY_ERR", "   Parity Error drop"},
    {"TIP_LEN_FAIL", "  IP Length check fail drop"},
    {"TMTUD", "         EBAD_MTU_DROP"},
    {"TSLLD", "         ESIP_LINK_LOCAL"},
    {"TL2MPLS", "       L2_MPLS_ENCAP_TX"},
    {"TL3MPLS", "       L3_MPLS_ENCAP_TX"},
    {"TMPLS", "         MPLS_TX"},
    {"MODIDTOOLARGEDROP","  MODID greater than 31 drop counter"},
    {"PKTMODTOOLARGEDROP"," Byte additions too large drop counter"},
    {"RX_SYMBOL", "     Fabric I/F => Rx Errors (e.g.8B10B)"},
    {"TIME_ALIGNMENT_EVEN", " Fabric I/F => Time Alignment (even)"},
    {"TIME_ALIGNMENT_ODD", " Fabric I/F => Time Alignment (odd)"},
    {"BIT_INTERLEAVED_PARITY_EVEN", " Fabric I/F => BIP (even)"},
    {"BIT_INTERLEAVED_PARITY_ODD", " Fabric I/F => BIP (odd)"},
    {"FcmPortClass3RxFrames",   " FCOE L3 RX frames"},
    {"FcmPortClass3RxDiscards", " FCOE L3 RX discarded frames "},
    {"FcmPortClass3TxFrames",   " FCOE L3 TX frames "},
    {"FcmPortClass2RxFrames",   " FCOE L2 RX frames "},
    {"FcmPortClass2RxDiscards", " FCOE L2 RX discarded frames "},
    {"FcmPortClass2TxFrames",   " FCOE L2 TX frames "}
};

/*
 * Function:
 *      get_counter
 * Description:
 *      Get numeric counter value from input string.
 * Parameters:
 *      c       - Input string.
 *      rx_tx   - Counter type (RX/TX)?
 *      counter - Counter value to be returned.
 * Returns:
 *      SUCCESS(0)/ FAILURE(-1).
 */
static int
get_counter(char *c, int rx_tx, int *counter)
{
    int        i;
    int        n;

    i = 0;
    n = 0;

    while (isdigit((unsigned) c[i])) {
        n = n * 10 + c[i++] - '0';
    }

    if (n < 0 || c[i]) {
        return -1;
    }

    if (rx_tx == RDBGC) {
        n += snmpBcmCustomReceive0;
        if (n < snmpBcmCustomReceive0 || n >= snmpBcmCustomTransmit0) {
            return -1;
        }
    } else if (rx_tx == TDBGC) {
        n += snmpBcmCustomTransmit0;
        if (n < snmpBcmCustomTransmit0 || n >= snmpValCount) {
            return -1;
        }
    }

    *counter = n;
    return 0;

}

/*
 * Function:
 *      set_triggers
 * Description:
 *      Set triggers for given counter.
 * Parameters:
 *      unit    - StrataSwitch PCI device unit number.
 *      port    - port number.
 *      counter - counter number.
 *      a       - argc/argv (sorta) structure for parsing arguments.
 * Returns:
 *      None.
 */
static int
set_triggers(int unit, int port, uint32 counter, args_t * a)
{

    char      *c;
    char       act = 5;
    int        cur_config = 0;
    int        rv = 0;
    uint32     trig_typ = 0;

    while ((c = ARG_GET(a)) != NULL) {
        if (c[0] == '+') {
            act = 1;            /* add */
            c++;
        } else if (c[0] == '-') {
            act = 2;            /* remove */
            c++;
        }
        for (trig_typ = 0; trig_typ < bcmDbgCntNum; trig_typ++) {
            if (parse_cmp(_trig_names[trig_typ].name, c, '\0')) {
                break;
            }
        }

        if (trig_typ == bcmDbgCntNum) {
            printk("%s Not available: see CustomSTAT LS\n", c);
            continue;
        }

        switch (act) {
            case 1:
                rv = bcm_stat_custom_add(unit, port, counter, trig_typ);
                break;
            case 2:
                rv = bcm_stat_custom_delete(unit, port, counter, trig_typ);
                break;
            default:           /* toggle */
                rv = bcm_stat_custom_check(unit, port,
                                           counter, trig_typ, &cur_config);
                if (BCM_SUCCESS(rv)) {
                    if (cur_config == 1) {
                        rv = bcm_stat_custom_delete(unit, port, counter,
                                                    trig_typ);
                    } else {
                        rv = bcm_stat_custom_add(unit, port, counter,
                                                 trig_typ);
                    }
                }
        }

        if (BCM_FAILURE(rv)) {
            printk("\t%18s\t%s %s (stat %d, port %d): %s\n", "-",
                   stat_names[counter], _trig_names[trig_typ].name,
                   counter, port, bcm_errmsg(rv));
        }
    }
    return rv;
}

/*
 * Function:
 *      cstat_flags_show
 * Description:
 *      Show programmed triggers for given counter.
 * Parameters:
 *      unit    - StrataSwitch PCI device unit number.
 *      pbmp    - Bitmap of ports.
 *      rx_tx   - counter type.
 *      counter - counter number.
 *                -1 means all.
 * Returns:
 *      None.
 */
static void
cstat_flags_show(int unit, bcm_pbmp_t pbmp, int rx_tx, int counter)
{
    uint32     count_index;
    uint32     start;
    uint32     end;
    int        port, dport;
    int        title;
    int        i = 0;
    int        rv = 0;
    int        trig_typ = 0;
    int        cur_config = 0;

    if (rx_tx == RDBGC) {
        port = 0;
        printk
            ("Custom Receive stats are enabled for following triggers\n");
        printk("[Triggers are common for all ports]\n");

        start = (counter == -1) ? snmpBcmCustomReceive0 : counter;
        end = (counter == -1) ? snmpBcmCustomTransmit0 : counter + 1;

        for (count_index = start; count_index < end; count_index++) {
            title = 1;
            for (trig_typ = 0, i = 0; trig_typ < bcmDbgCntNum; trig_typ++) {
                cur_config = 0;
                rv = bcm_stat_custom_check(unit, port, count_index,
                                           trig_typ, &cur_config);
                if (BCM_SUCCESS(rv) && cur_config) {
                    if (title) {
                        printk("\n%23s-\n\t\t", stat_names[count_index]);
                        title = 0;
                    }
                    printk("%s%s", _trig_names[trig_typ].name,
                           !((i + 1) % 5) ? "\n\t\t" : " ");
                    i++;
                } else {
                    continue;
                }
            }
        }
        printk("\n");
        return;
    }

    if (rx_tx == TDBGC) {
        start = (counter == -1) ? snmpBcmCustomTransmit0 : counter;
        end = (counter == -1) ? snmpValCount : counter + 1;
        printk
            ("Custom Transmit stats are enabled for following triggers\n");
        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
            BCM_PBMP_CLEAR(pbmp);
            printk("[Triggers are common for all ports]\n");
            for (count_index = start; count_index < end; count_index++) {
                title = 1;
                for (trig_typ = 0, i = 0; trig_typ < bcmDbgCntNum;
                     trig_typ++) {
                    cur_config = 0;
                    rv = bcm_stat_custom_check(unit, port, count_index,
                                               trig_typ, &cur_config);
                    if (BCM_SUCCESS(rv) && cur_config) {
                        if (title) {
                            printk("\n%23s-\n\t\t",
                                   stat_names[count_index]);
                            title = 0;
                        }
                        printk("%s%s", _trig_names[trig_typ].name,
                               !((i + 1) % 5) ? "\n\t\t" : " ");
                        i++;
                    } else {
                        continue;
                    }
                }
            }
        }
        printk("\n");
        return;
    }

}

/*
 * Function:
 *      cstat_stats_show
 * Description:
 *      Show programmable counters.
 * Parameters:
 *      unit    - StrataSwitch PCI device unit number.
 *      pbmp    - Bitmap of ports.
 *      rx_tx   - counter type to be displayed.
 *      counter - counter number to be displayed.
 *                -1 means all.
 * Returns:
 *      SUCCESS(0)/FAILURE(-1).
 */
static int
cstat_stats_show(int unit, bcm_pbmp_t pbmp, int rx_tx, int counter)
{
    uint32     count_index;
    uint32     start;
    uint32     end;
    uint32     title;
    uint64     value;
    int        port, dport;
    int        rv = 0;

    printk("%s Custom Statistics :\n", rx_tx == RDBGC ? "RX" : "TX");

    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
        switch (rx_tx) {
            case RDBGC:
                start = (counter == -1) ? snmpBcmCustomReceive0 : counter;
                end = (counter == -1) ? snmpBcmCustomTransmit0 :
                    counter + 1;
                title = 1;
                for (count_index = start; count_index < end; count_index++) {
                    rv = bcm_stat_get(unit, port, count_index, &value);
                    if (BCM_FAILURE(rv)) {
                        printk("\t%18s\t%s (stat %d): %s\n", "-",
                               stat_names[count_index], count_index,
                               bcm_errmsg(rv));
                        continue;
                    }
                    if (COMPILER_64_IS_ZERO(value)) {
                        continue;
                    }
                    if (title) {
                        printk(" %s:\n", SOC_PORT_NAME(unit, port));
                        title = 0;
                    }
                    if (COMPILER_64_HI(value) == 0) {
                        printk("    %18u\t%s (stat %d)\n",
                               COMPILER_64_LO(value),
                               stat_names[count_index], count_index);
                    } else {
                        printk("    0x%08x%08x\t%s (stat %d)\n",
                               COMPILER_64_HI(value),
                               COMPILER_64_LO(value),
                               stat_names[count_index], count_index);
                    }
                }
                break;
            case TDBGC:
                start = (counter == -1) ? snmpBcmCustomTransmit0 : counter;
                end = (counter == -1) ? snmpValCount : counter + 1;
                title = 1;
                for (count_index = start; count_index < end; count_index++) {
                    rv = (bcm_stat_get(unit, port, count_index, &value));
                    if (BCM_FAILURE(rv)) {
                        printk("\t%18s\t%s (stat %d): %s\n", "-",
                               stat_names[count_index], count_index,
                               bcm_errmsg(rv));
                        continue;
                    }
                    if (COMPILER_64_IS_ZERO(value)) {
                        continue;
                    }
                    if (title) {
                        printk(" %s:\n", SOC_PORT_NAME(unit, port));
                        title = 0;
                    }
                    if (COMPILER_64_HI(value) == 0) {
                        printk("    %18u\t%s (stat %d)\n",
                               COMPILER_64_LO(value),
                               stat_names[count_index], count_index);
                    } else {
                        printk("    0x%08x%08x\t%s (stat %d)\n",
                               COMPILER_64_HI(value),
                               COMPILER_64_LO(value),
                               stat_names[count_index], count_index);
                    }
                }
                break;
            default:
                printk("Invalid custom stats: valid values [RX,TX]\n");
                return -1;
        }
    }
    printk("\n");
    return rv;
}

/*
 * Function:
 *      cstat_flags_set
 * Description:
 *      Set triggers for counters.
 * Parameters:
 *      unit    - StrataSwitch PCI device unit number.
 *      pbmp    - Bitmap of ports.
 *      rx_tx   - trigger type to be set.
 *      counter - counter number to be programmed.
 *      a       - argc/argv (sorta) structure for parsing arguments.
 * Returns:
 *      SUCCESS(0)/FAILURE(-1).
 */
static int
cstat_flags_set(int unit, bcm_pbmp_t pbmp, int rx_tx, int counter,
                args_t * a)
{

    int        flags_index;
    bcm_port_t port, dport;
    uint32     count_index;
    uint32     start;
    uint32     end;

    if (!ARG_CUR(a)) {
        printk("Error: No input triggers\n");
        return -1;
    }
    /*
     * save index of first flag 
     */
    flags_index = a->a_arg;

    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
        switch (rx_tx) {
            case RDBGC:
                start = (counter == -1) ? snmpBcmCustomReceive3 : counter;
                end = (counter == -1) ? snmpBcmCustomTransmit0 :
                    counter + 1;
                for (count_index = start; count_index < end; count_index++) {
                    a->a_arg = flags_index;
                    set_triggers(unit, port, count_index, a);
                }
                break;
            case TDBGC:
                start = (counter == -1) ? snmpBcmCustomTransmit6 : counter;
                end = (counter == -1) ? snmpBcmCustomTransmit12 : counter + 1;
                for (count_index = start; count_index < end; count_index++) {
                    a->a_arg = flags_index;
                    set_triggers(unit, port, count_index, a);
                }
                break;
            default:
                printk
                    ("Error: Invalid custom stats: valid values [RX,TX]\n");
                return -1;
        }
        break;
    }
    return 0;
}

/*
 * Function:
 *      list_flags
 * Description:
 *      Display the triggers available for current device.
 * Parameters:
 *      unit  - StrataSwitch PCI device unit number.
 *      about - (0/1) whether to display short description of each trigger?
 *      rx_tx - trigger type to be displayed.
 * Returns:
 *      None.
 */
static void
list_flags(int unit, int about, int rx_tx)
{
    int        trig_typ;
    int        res;
    int        loop;
    int        dummy_port = SOC_PORT_MIN(unit, port);

    printk("Custom stat\n");
    if (rx_tx == RDBGC || rx_tx == BOTH) {
        printk(" Available Rx trigger flags :\n  ");
        for (trig_typ = 0, loop = 0; trig_typ < bcmDbgCntNum; trig_typ++) {
            if (BCM_SUCCESS(bcm_stat_custom_check(unit, dummy_port,
                                                  snmpBcmCustomReceive0,
                                                  trig_typ, &res))) {
                if (!about) {
                    printk("%-19s%s", _trig_names[trig_typ].name,
                           !((loop + 1) % 4) ? "\n  " : "");
                } else {
                    printk("%s%s\n  ", _trig_names[trig_typ].name,
                           _trig_names[trig_typ].about);
                }
                loop++;
            }
        }
        printk("\n");
    }

    if (rx_tx == TDBGC || rx_tx == BOTH) {
        printk(" Available Tx trigger flags :\n  ");
        for (trig_typ = 0, loop = 0; trig_typ < bcmDbgCntNum; trig_typ++) {
            if (BCM_SUCCESS(bcm_stat_custom_check(unit, dummy_port,
                                                  snmpBcmCustomTransmit0,
                                                  trig_typ, &res))) {
                if (!about) {
                    printk("%-19s%s", _trig_names[trig_typ].name,
                           !((loop + 1) % 4) ? "\n  " : "");
                } else {
                    printk("%s%s\n  ", _trig_names[trig_typ].name,
                           _trig_names[trig_typ].about);
                }
                loop++;
            }
        }
        printk("\n");
    }
}

/*
 * Function:
 *      display_val
 * Description:
 *      Display value of counter.
 * Parameters:
 *      val          - Value to be displayed.
 *      count_index  - Index of counter.
 * Returns:
 *      None.
 */
static void
display_val(uint64 val, int count_index)
{

    char      *typ_s;
    char      *no_s;

    typ_s = "  ";
    no_s = "   ";

    if (count_index >= snmpBcmCustomTransmit0) {
        if (count_index == snmpBcmCustomTransmit0) {
            typ_s = "TX";
        }
        if (count_index < snmpBcmCustomTransmit6) {
            no_s = "(R)";
        }
        count_index -= snmpBcmCustomTransmit0;
    } else {
        if (count_index == snmpBcmCustomReceive0) {
            typ_s = "RX";
        }
        if (count_index < snmpBcmCustomReceive3) {
            no_s = "(R)";
        }
        count_index -= snmpBcmCustomReceive0;
    }

    printk("\n|  %s  |%2d%s|", typ_s, count_index, no_s);

    if (COMPILER_64_HI(val) == 0) {
        printk("%19u| ", COMPILER_64_LO((val)));
    } else {
        printk(" 0x%08x%08x| ", COMPILER_64_HI(val), COMPILER_64_LO(val));
    }
}

/*
 * Function:
 *      list_table
 * Description:
 *      Display the counters and triggers in tabular format.
 * Parameters:
 *      unit  - StrataSwitch PCI device unit number.
 *      pbmp  - Bitmap of ports.
 *      show_zero - (0/1) Whether to include counters which are zero?
 * Returns:
 *      None.
 */
static void
list_table(int unit, bcm_pbmp_t pbmp, int show_zero)
{

    int        n;
    int        i;
    int        j;
    int        rv;
    int        trig;
    int        port = 0;
    int        dport;
    int        conf;
    int        val_f;
    int        title;
    int        mid;
    int        end;
    char      *blnk = "                                             ";
    uint64     value;

    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
        title = 1;
        mid = 1;
        end = 0;
        for (n = snmpBcmCustomReceive0; n < snmpValCount; n++) {
            rv = bcm_stat_get(unit, port, n, &value);
            if (BCM_FAILURE(rv)) {
                continue;
            }
            if (!show_zero && (COMPILER_64_IS_ZERO(value))) {
                continue;
            }
            if (title) {
                /* coverity[non_const_printf_format_string] */
                printk(head, SOC_PORT_NAME(unit, (port)));
                printk("%s%s", menu, line);
                title = 0;
                end = 1;
            }
            if (n >= snmpBcmCustomTransmit0 && mid) {
                printk("%s", line);
                mid = 0;
            }
            val_f = 1;
            j = 0;
            for (trig = 0, i = 0; trig < bcmDbgCntNum; trig++) {
                conf = 0;
                rv = bcm_stat_custom_check(unit, port, n, trig, &conf);
                if (BCM_SUCCESS(rv) && conf) {
                    if (val_f) {
                        display_val(value, n);
                        val_f = 0;
                    }
                    if (!((i + 1) % COL_SZ)) {
                        printk("%.*s|%s", (int) (sal_strlen(blnk) - j),
                               blnk, trg_s);
                        j = 0;
                    }
                    printk("%s ", _trig_names[trig].name);
                    j = j + sal_strlen(_trig_names[trig].name) + 1;
                    i++;
                }
            }
            if (j) {
                printk("%.*s|", (int) (sal_strlen(blnk) - j), blnk);
            }
        }
        if (end) {
            printk("%s", line);
            end = 0;
            printk("\n\n");
        }
    }

}                               /* end of list_table */

/*
 * Function:
 *      cmd_esw_custom_stat
 * Description:
 *      Main function to process CustomSTAT command.
 * Parameters:
 *      unit  - StrataSwitch PCI device unit number.
 *      a     - argc/argv (sorta) structure for parsing arguments.
 * Returns:
 *      Result of command processing, SUCCESS, FAIL or print command usage.
 */
cmd_result_t
cmd_esw_custom_stat(int unit, args_t * a)
{

    bcm_pbmp_t pbmp;
    uint32     cmd_type;        /* show flags, set flags, show counters */
    char      *c;
    int        rx_tx;
    int        rv = 0;
    int        counter;         /* selected DBGC map, input */
    int        display_zero;
    int        display_info;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    BCM_PBMP_CLEAR(pbmp);
    BCM_PBMP_PORT_ADD(pbmp, 0); /* ge0 */
    counter = -1;               /* all counters */
    cmd_type = SHOW_TABLE;
    rx_tx = BOTH;
    display_zero = FALSE;
    display_info = FALSE;

    if (!(c = ARG_GET(a))) {
        goto do_stat_command;
    } else {
        if (!sal_strcasecmp(c, "ls")) {
            cmd_type = LIST_FLAGS;
        } else if (!sal_strcasecmp(c, "info")) {
            cmd_type = LIST_FLAGS;
            display_info = TRUE;
        } else if (!sal_strcasecmp(c, "set")) {
            cmd_type = SET_FLAGS;
            c = ARG_GET(a);
        } else if (!sal_strcasecmp(c, "get")) {
            cmd_type = SHOW_FLAGS;
            c = ARG_GET(a);
        } else if (!sal_strcasecmp(c, "raw")) {
            cmd_type = SHOW_COUNTERS;
            c = ARG_GET(a);
        }
    }

    /*
     * no input pbm 
     */
    if (!c) {
        ERROR_SET_FLAGS("No input port\n");
        goto do_stat_command;
    } else if (cmd_type != LIST_FLAGS) {
        if (parse_bcm_pbmp(unit, c, &pbmp) < 0) {
            printk("%s: Error: unrecognized port bitmap: %s\n", ARG_CMD(a),
                   c);
            return CMD_FAIL;
        }
    }

    /*
     * RX/TX ? 
     */
    if (!(c = ARG_GET(a))) {
        ERROR_SET_FLAGS("No input type[rx|tx]\n");
        goto do_stat_command;
    } else {
        if (!sal_strcasecmp(c, "rx")) {
            rx_tx = RDBGC;
        } else if (!sal_strcasecmp(c, "tx")) {
            rx_tx = TDBGC;
        } else if (sal_toupper((int)c[0]) == 'Z') {
            display_zero = TRUE;
            goto do_stat_command;
        } else {
            printk("Invalid input '%s'\n", c);
            return CMD_FAIL;
        }

    /*    coverity[new_values]    */
        if (cmd_type == LIST_FLAGS) {
            goto do_stat_command;
        }
    }

    /*
     * counter bitmap 
     */
    if (!(c = ARG_GET(a))) {
        ERROR_SET_FLAGS("No input counter\n");
        goto do_stat_command;
    } else {
        if (!sal_strcasecmp(c, "all")) {
            goto do_stat_command;
        } else if (!get_counter(c, rx_tx, &counter)) {
            goto do_stat_command;
        } else {
            printk("Invalid counter\n");
            return CMD_FAIL;
        }
    }

  do_stat_command:
    switch (cmd_type) {
        case SHOW_FLAGS:
            if (rx_tx == BOTH) {
                cstat_flags_show(unit, pbmp, RDBGC, counter);
                cstat_flags_show(unit, pbmp, TDBGC, counter);
            } else {
                cstat_flags_show(unit, pbmp, rx_tx, counter);
            }
            break;
        case SHOW_COUNTERS:
            if (rx_tx == BOTH) {
                rv = cstat_stats_show(unit, pbmp, RDBGC, counter);
                rv = cstat_stats_show(unit, pbmp, TDBGC, counter);
            } else {
                rv = cstat_stats_show(unit, pbmp, rx_tx, counter);
            }
            break;
        case SHOW_TABLE:
            list_table(unit, pbmp, display_zero);
            break;
        case SET_FLAGS:
            rv = cstat_flags_set(unit, pbmp, rx_tx, counter, a);
            break;
        case LIST_FLAGS:
            list_flags(unit, display_info, rx_tx);
            break;
        default:
            /*
             * should not reach here
             */
            return CMD_FAIL;
    }

    if (rv < 0) {
        return CMD_FAIL;
    }

    return CMD_OK;

}
