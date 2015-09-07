/*
 * $Id: mem.c,v 1.81 Broadcom SDK $
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
 * socdiag memory commands
 */

#include <shared/bsl.h>

#include <sal/core/libc.h>
#include <soc/mem.h>
#include <soc/debug.h>
#include <shared/bsl.h>
#include <soc/error.h>
#include <soc/l2x.h>
#include <soc/l3x.h>
#include <soc/soc_ser_log.h>
#ifdef BCM_ISM_SUPPORT
#include <soc/ism.h>
#endif
#include <appl/diag/system.h>
#ifdef BCM_TRIDENT2_SUPPORT
#include <soc/trident2.h>
#endif

#ifdef __KERNEL__
#define atoi _shr_ctoi
#endif

/*
 * Utility routine to concatenate the first argument ("first"), with
 * the remaining arguments, with commas separating them.
 */

static void
collect_comma_args(args_t *a, char *valstr, char *first)
{
    char *s;

    sal_strcpy(valstr, first);

    while ((s = ARG_GET(a)) != 0) {
	sal_strcat(valstr, ",");
	sal_strcat(valstr, s);
    }
}

static void
check_global(int unit, soc_mem_t mem, char *s, int *is_global)
{
    soc_field_info_t	*fld;
    soc_mem_info_t	*m = &SOC_MEM_INFO(unit, mem);
    char                *eqpos;
    
    eqpos = strchr(s, '=');
    if (eqpos != NULL) {
        *eqpos++ = 0;
    }
    for (fld = &m->fields[0]; fld < &m->fields[m->nFields]; fld++) {
        if (!sal_strcasecmp(s, SOC_FIELD_NAME(unit, fld->field)) &&
            (fld->flags & SOCF_GLOBAL)) {
    	    break;
        }
    }
    if (fld == &m->fields[m->nFields]) {
        *is_global = 0;
    } else {
        *is_global = 1;
    }
}

static int
collect_comma_args_with_view(args_t *a, char *valstr, char *first, 
                             char *view, int unit, soc_mem_t mem)
{
    char *s, *s_copy = NULL, *f_copy = NULL;
    int is_global, rv = 0;

    if ((f_copy = sal_alloc(sal_strlen(first) + 1, "first")) == NULL) {
        cli_out("cmd_esw_mem_write : Out of memory\n");
        rv = -1;
        goto done;
    }
    sal_memset(f_copy, 0, sal_strlen(first) + 1);
    sal_strcpy(f_copy, first);

    /* Check if field is global before applying view prefix */
    check_global(unit, mem, f_copy, &is_global);
    if (!is_global) {
        sal_strcpy(valstr, view);
        sal_strcat(valstr, first);
    } else {
        sal_strcpy(valstr, first);
    }

    while ((s = ARG_GET(a)) != 0) {
        if ((s_copy = sal_alloc(sal_strlen(s) + 1, "s_copy")) == NULL) {
            cli_out("cmd_esw_mem_write : Out of memory\n");
            rv = -1;
            goto done;
        }
        sal_memset(s_copy, 0, sal_strlen(s) + 1);
        sal_strcpy(s_copy, s);
        check_global(unit, mem, s_copy, &is_global);
        sal_free(s_copy);
        sal_strcat(valstr, ",");
        if (!is_global) {
            sal_strcat(valstr, view);
            sal_strcat(valstr, s);
        } else {
            sal_strcat(valstr, s);
        }
    }
done:
    if (f_copy != NULL) {
        sal_free(f_copy);
    }
    return rv;
}

/*
 * modify_mem_fields
 *
 *   Verify similar to modify_reg_fields (see reg.c) but works on
 *   memory table entries instead of register values.  Handles fields
 *   of any length.
 *
 *   If mask is non-NULL, it receives an entry which is a mask of all
 *   fields modified.
 *
 *   Values may be specified with optional increment or decrement
 *   amounts; for example, a MAC address could be 0x1234+2 or 0x1234-1
 *   to specify an increment of +2 or -1, respectively.
 *
 *   If incr is FALSE, the increment is ignored and the plain value is
 *   stored in the field (e.g. 0x1234).
 *
 *   If incr is TRUE, the value portion is ignored.  Instead, the
 *   increment value is added to the existing value of the field.  The
 *   field value wraps around on overflow.
 *
 *   Returns -1 on failure, 0 on success.
 */

static int
modify_mem_fields(int unit, soc_mem_t mem, uint32 *entry,
		  uint32 *mask, char *mod, int incr)
{
    soc_field_info_t	*fld;
    char		*fmod, *fval, *s;
    char		*modstr = NULL;
    char		*tokstr = NULL;
    uint32		fvalue[SOC_MAX_MEM_FIELD_WORDS];
    uint32		fincr[SOC_MAX_MEM_FIELD_WORDS];
    int			i, entry_dw;
    soc_mem_info_t	*m = &SOC_MEM_INFO(unit, mem);

    entry_dw = BYTES2WORDS(m->bytes);

    if ((modstr = sal_alloc(ARGS_BUFFER, "modify_mem")) == NULL) {
        cli_out("modify_mem_fields: Out of memory\n");
        return CMD_FAIL;
    }

    sal_strncpy(modstr, mod, ARGS_BUFFER);/* Don't destroy input string */
    modstr[ARGS_BUFFER - 1] = 0;
    mod = modstr;

    if (mask) {
	memset(mask, 0, entry_dw * 4);
    }

    while ((fmod = sal_strtok_r(mod, ",", &tokstr)) != 0) {
	mod = NULL;			/* Pass strtok NULL next time */
	fval = strchr(fmod, '=');
	if (fval != NULL) {		/* Point fval to arg, NULL if none */
	    *fval++ = 0;		/* Now fmod holds only field name. */
	}
	if (fmod[0] == 0) {
	    cli_out("Null field name\n");
            sal_free(modstr);
	    return -1;
	}
	if (!sal_strcasecmp(fmod, "clear")) {
	    sal_memset(entry, 0, entry_dw * sizeof (*entry));
	    if (mask) {
		memset(mask, 0xff, entry_dw * sizeof (*entry));
	    }
	    continue;
	}
	for (fld = &m->fields[0]; fld < &m->fields[m->nFields]; fld++) {
	    if (!sal_strcasecmp(fmod, SOC_FIELD_NAME(unit, fld->field))) {
		break;
	    }
	}
	if (fld == &m->fields[m->nFields]) {
	    cli_out("No such field \"%s\" in memory \"%s\".\n",
                    fmod, SOC_MEM_UFNAME(unit, mem));
            sal_free(modstr);
	    return -1;
	}
	if (!fval) {
	    cli_out("Missing %d-bit value to assign to \"%s\" field \"%s\".\n",
                    fld->len,
                    SOC_MEM_UFNAME(unit, mem), SOC_FIELD_NAME(unit, fld->field));
            sal_free(modstr);
	    return -1;
	}
	s = strchr(fval, '+');
	if (s == NULL) {
	    s = strchr(fval, '-');
	}
	if (s == fval) {
	    s = NULL;
	}
	if (incr) {
	    if (s != NULL) {
		parse_long_integer(fincr, SOC_MAX_MEM_FIELD_WORDS,
				   s[1] ? &s[1] : "1");
		if (*s == '-') {
		    neg_long_integer(fincr, SOC_MAX_MEM_FIELD_WORDS);
		}
		if (fld->len & 31) {
		    /* Proper treatment of sign extension */
		    fincr[fld->len / 32] &= ~(0xffffffff << (fld->len & 31));
		}
		soc_mem_field_get(unit, mem, entry, fld->field, fvalue);
		add_long_integer(fvalue, fincr, SOC_MAX_MEM_FIELD_WORDS);
		if (fld->len & 31) {
		    /* Proper treatment of sign extension */
		    fvalue[fld->len / 32] &= ~(0xffffffff << (fld->len & 31));
		}
		soc_mem_field_set(unit, mem, entry, fld->field, fvalue);
	    }
	} else {
	    if (s != NULL) {
		*s = 0;
	    }
	    parse_long_integer(fvalue, SOC_MAX_MEM_FIELD_WORDS, fval);
	    for (i = fld->len; i < SOC_MAX_MEM_FIELD_BITS; i++) {
		if (fvalue[i / 32] & 1 << (i & 31)) {
		    cli_out("Value \"%s\" too large for "
                            "%d-bit field \"%s\".\n",
                            fval, fld->len, SOC_FIELD_NAME(unit, fld->field));
                    sal_free(modstr);
		    return -1;
		}
	    }
	    soc_mem_field_set(unit, mem, entry, fld->field, fvalue);
	}
	if (mask) {
	    sal_memset(fvalue, 0, sizeof (fvalue));
	    for (i = 0; i < fld->len; i++) {
		fvalue[i / 32] |= 1 << (i & 31);
	    }
	    soc_mem_field_set(unit, mem, mask, fld->field, fvalue);
	}
    }

    sal_free(modstr);
    return 0;
}

/*
 * mmudebug command
 *    No argument: print current state
 *    Argument is 1: enable
 *    Argument is 0: disable
 */

char mmudebug_usage[] =
    "Parameters: [on | off]\n\t"
    "Puts the MMU in debug mode (on) or normal mode (off).\n\t"
    "With no parameter, displays the current mode.\n\t"
    "MMU must be in debug mode to access CBP memories.\n";

cmd_result_t
mem_mmudebug(int unit, args_t *a)
{
    char *s = ARG_GET(a);

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (s) {
	if (!sal_strcasecmp(s, "on")) {
	    cli_out("Entering debug mode ...\n");
	    if (soc_mem_debug_set(unit, 1) < 0) {
                return CMD_FAIL;
            }
	} else if (!sal_strcasecmp(s, "off")) {
	    cli_out("Leaving debug mode ...\n");
	    if (soc_mem_debug_set(unit, 0) < 0) {
                return CMD_FAIL;
            }
	} else
	    return CMD_USAGE;
    } else {
	int enable = 0;
	soc_mem_debug_get(unit, &enable);
	cli_out("MMU debug mode is %s\n", enable ? "on" : "off");
    }

    return CMD_OK;
}

/*
 * Initialize Cell Free Address pool.
 */

char cfapinit_usage[] =
    "Parameters: none\n\t"
    "Run diags on CFAP pool and initialize CFAP with good entries only\n";

cmd_result_t
mem_cfapinit(int unit, args_t *a)
{
    int r;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if ((r = soc_mem_cfap_init(unit)) < 0) {
	cli_out("NOTICE: error initializing CFAP: %s\n", soc_errmsg(r));
	return CMD_FAIL;
    }

    return CMD_OK;
}

#ifdef BCM_HERCULES_SUPPORT
/*
 * Initialize Cell Free Address pool.
 */

char llainit_usage[] =
    "Parameters: [force]\n\t"
    "Run diags on LLA and PP and initialize LLA with good entries only\n";

cmd_result_t
mem_llainit(int unit, args_t *a)
{
    char *argforce;
    int force = FALSE;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if ((argforce = ARG_GET(a)) != NULL) {
        if (!sal_strcasecmp(argforce, "force")) {
            force = TRUE;
        } else {
            return CMD_USAGE;
        }
    }

    if (force) {
        if (SOC_CONTROL(unit)->lla_map != NULL) {
            int i;
	    PBMP_PORT_ITER(unit, i) {
                if (SOC_CONTROL(unit)->lla_map[i] != NULL) {
                    sal_free(SOC_CONTROL(unit)->lla_map[i]);
                    SOC_CONTROL(unit)->lla_map[i] = NULL;
                }
            }
        }
    }

    return CMD_OK;
}
#endif

static int
parse_dwords(int count, uint32 *dw, args_t *a)
{
    char	*s;
    int		i;

    for (i = 0; i < count; i++) {
	if ((s = ARG_GET(a)) == NULL) {
	    cli_out("Not enough data values (have %d, need %d)\n",
                    i, count);
	    return -1;
	}

	dw[i] = parse_integer(s);
    }

    if (ARG_CNT(a) > 0) {
	cli_out("Ignoring extra data on command line "
                "(only %d words needed)\n",
                count);
    }

    return 0;
}

cmd_result_t
cmd_esw_mem_write(int unit, args_t *a)
{
    int			i, index, start, count, copyno;
    char		*tab, *idx, *cnt, *s, *memname, *slam_buf = NULL;
    soc_mem_t		mem;
    uint32		entry[SOC_MAX_MEM_WORDS];
    int			entry_dw, view_len, entry_bytes;
    char		copyno_str[8];
    int			r, update;
    int			rv = CMD_FAIL;
    char		*valstr = NULL, *view = NULL;
    int			no_cache = 0;
    int                 use_slam = 0;
    int			acc_type = 0;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	goto done;
    }

    tab = ARG_GET(a);
    if (tab != NULL && sal_strcasecmp(tab, "nocache") == 0) {
	no_cache = 1;
	tab = ARG_GET(a);
    }
    if (tab != NULL && sal_strcasecmp(tab, "pipe_x") == 0) {
	acc_type = _SOC_MEM_ADDR_ACC_TYPE_PIPE_X;
	tab = ARG_GET(a);
    }
    if (tab != NULL && sal_strcasecmp(tab, "pipe_y") == 0) {
	acc_type = _SOC_MEM_ADDR_ACC_TYPE_PIPE_Y;
	tab = ARG_GET(a);
    }
    idx = ARG_GET(a);
    cnt = ARG_GET(a);
    s = ARG_GET(a);

    /* you will need at least one value and all the args .. */
    if (!tab || !idx || !cnt || !s || !isint(cnt)) {
	return CMD_USAGE;
    }

    /* Deal with VIEW:MEMORY if applicable */
    memname = sal_strstr(tab, ":");
    view_len = 0;
    if (memname != NULL) {
        memname++;
        view_len = memname - tab;
    } else {
        memname = tab;
    }

    if (parse_memory_name(unit, &mem, memname, &copyno, 0) < 0) {
	cli_out("ERROR: unknown table \"%s\"\n",tab);
	goto done;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        LOG_ERROR(BSL_LS_APPL_SOCMEM,
                  (BSL_META_U(unit,
                              "Error: Memory %s not valid for chip %s.\n"),
                              SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit)));
        goto done;
    }

#if defined(BCM_HAWKEYE_SUPPORT)
    if (SOC_IS_HAWKEYE(unit) && (soc_mem_index_max(unit, mem) <= 0)) {
        LOG_ERROR(BSL_LS_APPL_SOCMEM,
                  (BSL_META_U(unit,
                              "Error: Memory %s not valid for chip %s.\n"),
                              SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit)));
        goto done;
    }
#endif /* BCM_HAWKEYE_SUPPORT */

    if (soc_mem_is_readonly(unit, mem)) {
	LOG_ERROR(BSL_LS_APPL_SOCMEM,
                  (BSL_META_U(unit,
                              "ERROR: Table %s is read-only\n"),
                              SOC_MEM_UFNAME(unit, mem)));
	goto done;
    }

    start = parse_memory_index(unit, mem, idx);
    count = parse_integer(cnt);

    if (copyno == COPYNO_ALL) {
	copyno_str[0] = 0;
    } else {
	sal_sprintf(copyno_str, ".%d", copyno);
    }

    entry_dw = soc_mem_entry_words(unit, mem);

    if ((valstr = sal_alloc(ARGS_BUFFER, "reg_set")) == NULL) {
        cli_out("cmd_esw_mem_write : Out of memory\n");
        goto done;
    }

    /*
     * If a list of fields were specified, generate the entry from them.
     * Otherwise, generate it by reading raw dwords from command line.
     */

    if (!isint(s)) {
	/* List of fields */

	if (view_len == 0) {
            collect_comma_args(a, valstr, s);
        } else {
            if ((view = sal_alloc(view_len + 1, "view_name")) == NULL) {
                cli_out("cmd_esw_mem_write : Out of memory\n");
                goto done;
            }
            sal_memset(view, 0, view_len + 1);
            sal_memcpy(view, tab, view_len);
            if (collect_comma_args_with_view(a, valstr, s, view, unit, mem) < 0) {
                cli_out("Out of memory: aborted\n");
                goto done;
            }
        }

	memset(entry, 0, sizeof (entry));

	if (modify_mem_fields(unit, mem, entry, NULL, valstr, FALSE) < 0) {
	    cli_out("Syntax error: aborted\n");
	    goto done;
	}

	update = TRUE;
    } else {
	/* List of numeric values */

	ARG_PREV(a);

	if (parse_dwords(entry_dw, entry, a) < 0) {
	    goto done;
	}

	update = FALSE;
    }

    if (bsl_check(bslLayerSoc, bslSourceSocmem, bslSeverityNormal, unit)) {
	cli_out("WRITE[%s%s], DATA:", SOC_MEM_UFNAME(unit, mem), copyno_str);
	for (i = 0; i < entry_dw; i++) {
	    cli_out(" 0x%x", entry[i]);
	}
	cli_out("\n");
    }

    /*
     * Created entry, now write it
     */

    use_slam = soc_property_get(unit, spn_DIAG_SHELL_USE_SLAM, 0);
    if (use_slam && count > 1) {
        entry_bytes = soc_mem_entry_bytes(unit, mem);
        slam_buf = soc_cm_salloc(unit, count * entry_bytes, "slam_entry");
        if (slam_buf == NULL) {
            cli_out("cmd_esw_mem_write : Out of memory\n");
            goto done;
        }
        for (i = 0; i < count; i++) {
            sal_memcpy(slam_buf + i * entry_bytes, entry, entry_bytes);
        }
        r = soc_mem_write_range(unit, mem, copyno, start, start + count - 1, slam_buf);
        soc_cm_sfree(unit, slam_buf);
        if (r < 0) {
            cli_out("Slam ERROR: table %s.%s: %s\n",
                    SOC_MEM_UFNAME(unit, mem), copyno_str, soc_errmsg(r));
            goto done;
        }
        for (index = start; index < start + count; index++) {
	    if (update) {
	        modify_mem_fields(unit, mem, entry, NULL, valstr, TRUE);
	    }
        }
    } else {
        for (index = start; index < start + count; index++) {
            if (acc_type) {
                if ((r = soc_mem_pipe_select_write(unit,
                                                   no_cache ?
                                                   SOC_MEM_DONT_USE_CACHE:
                                                   SOC_MEM_NO_FLAGS,
                                                   mem, copyno, acc_type, index,
                                                   entry)) < 0) {
                    cli_out("Write ERROR: table %s(Y).%s[%d]: %s\n",
                            SOC_MEM_UFNAME(unit, mem), copyno_str,
                            index, soc_errmsg(r));
                    goto done;
                }
            } else if ((r = soc_mem_write_extended(unit,
                                                   no_cache ?
                                                   SOC_MEM_DONT_USE_CACHE:
                                                   SOC_MEM_NO_FLAGS,mem,
                                                   copyno, index, entry)) < 0) {
	        cli_out("Write ERROR: table %s.%s[%d]: %s\n",
                        SOC_MEM_UFNAME(unit, mem), copyno_str,
                        index, soc_errmsg(r));
	        goto done;
	    }

	    if (update) {
	        modify_mem_fields(unit, mem, entry, NULL, valstr, TRUE);
	    }
        }
    }
    rv = CMD_OK;

 done:
    if (valstr != NULL) {
       sal_free(valstr);
    }
    if (view != NULL) {
       sal_free(view);
    }
    return rv;
}


/*
 * Modify the fields of a table entry
 */
cmd_result_t
cmd_esw_mem_modify(int unit, args_t *a)
{
    int			index, start, count, copyno, i, view_len;
    char		*tab, *idx, *cnt, *s, *memname;
    soc_mem_t		mem;
    uint32		entry[SOC_MAX_MEM_WORDS], mask[SOC_MAX_MEM_WORDS];
    uint32		changed[SOC_MAX_MEM_WORDS];
    char		*valstr = NULL, *view = NULL;
    int			r, rv = CMD_FAIL;
    int			blk;
#ifdef BCM_TRIDENT2_SUPPORT
    int                 check_view = FALSE;
    soc_mem_t           mview;
#endif /* BCM_TRIDENT2_SUPPORT */

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    tab = ARG_GET(a);
    idx = ARG_GET(a);
    cnt = ARG_GET(a);
    s = ARG_GET(a);

    /* you will need at least one dword and all the args .. */
    if (!tab || !idx || !cnt || !s || !isint(cnt)) {
	return CMD_USAGE;
    }

    if ((valstr = sal_alloc(ARGS_BUFFER, "mem_modify")) == NULL) {
        cli_out("cmd_esw_mem_modify : Out of memory\n");
        goto done;
    }

    memname = sal_strstr(tab, ":"); 
    view_len = 0; 
    if (memname != NULL) { 
        memname++; 
        view_len = memname - tab; 
    } else { 
        memname = tab; 
    } 

    if (parse_memory_name(unit, &mem, memname, &copyno, 0) < 0) {
	cli_out("ERROR: unknown table \"%s\"\n",tab);
    goto done;
    }

    if (view_len == 0) {
        collect_comma_args(a, valstr, s);
    } else {
        if ((view = sal_alloc(view_len + 1, "view_name")) == NULL) {
            cli_out("cmd_esw_mem_modify : Out of memory\n");
            goto done;
        }

        sal_memcpy(view, tab, view_len);
        view[view_len] = 0;

        if (collect_comma_args_with_view(a, valstr, s, view, unit, mem) < 0) {
            cli_out("Out of memory: aborted\n");
            goto done;
        }
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        LOG_ERROR(BSL_LS_APPL_SOCMEM,
                  (BSL_META_U(unit,
                              "Error: Memory %s not valid for chip %s.\n"),
                              SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit)));
        goto done;
    }

#if defined(BCM_HAWKEYE_SUPPORT)
    if (SOC_IS_HAWKEYE(unit) && (soc_mem_index_max(unit, mem) <= 0)) {
        LOG_ERROR(BSL_LS_APPL_SOCMEM,
                  (BSL_META_U(unit,
                              "Error: Memory %s not valid for chip %s.\n"),
                              SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit)));
        goto done;
    }
#endif /* BCM_HAWKEYE_SUPPORT */

    if (soc_mem_is_readonly(unit, mem)) {
	cli_out("ERROR: Table %s is read-only\n", SOC_MEM_UFNAME(unit, mem));
        goto done;
    }

    sal_memset(changed, 0, sizeof (changed));

    if (modify_mem_fields(unit, mem, changed, mask, valstr, FALSE) < 0) {
	cli_out("Syntax error: aborted\n");
        goto done;
    }

    start = parse_memory_index(unit, mem, idx);
    count = parse_integer(cnt);

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit) &&
        (mem == L3_DEFIP_ALPM_IPV4m || mem == L3_DEFIP_ALPM_IPV4_1m ||
         mem == L3_DEFIP_ALPM_IPV6_64m || mem == L3_DEFIP_ALPM_IPV6_64_1m ||
         mem == L3_DEFIP_ALPM_IPV6_128m)) {
        check_view = TRUE;
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    /*
     * Take lock to ensure atomic modification of memory.
     */

    soc_mem_lock(unit, mem);

    rv = CMD_OK;

    for (index = start; index < start + count; index++) {
	SOC_MEM_BLOCK_ITER(unit, mem, blk) {
	    if (copyno != COPYNO_ALL && copyno != blk) {
		continue;
	    }
#ifdef BCM_TRIDENT2_SUPPORT
            if (check_view) {
                mview = _soc_trident2_alpm_bkt_view_get(unit, index);
                if (!(mview == INVALIDm || mem == mview)) {
                    continue;
                }
            }
#endif /* BCM_TRIDENT2_SUPPORT */

	    /*
	     * Retrieve the current entry, set masked fields to changed
	     * values, and write back.
	     */

            r = soc_mem_read(unit, mem, blk, index, entry);

	    if (r < 0) {
		cli_out("ERROR: read from %s table copy %d failed: %s\n",
                        SOC_MEM_UFNAME(unit, mem), blk, soc_errmsg(r));
		rv = CMD_FAIL;
		break;
	    }

	    for (i = 0; i < SOC_MAX_MEM_WORDS; i++) {
		entry[i] = (entry[i] & ~mask[i]) | changed[i];
	    }

            r = soc_mem_write(unit, mem, blk, index, entry);

	    if (r < 0) {
		cli_out("ERROR: write to %s table copy %d failed: %s\n",
                        SOC_MEM_UFNAME(unit, mem), blk, soc_errmsg(r));
		rv = CMD_FAIL;
		break;
	    }
	}

	if (rv != CMD_OK) {
	    break;
	}

	modify_mem_fields(unit, mem, changed, NULL, valstr, TRUE);
    }

    soc_mem_unlock(unit, mem);

 done:
    if (view != NULL) {
        sal_free(view);
    }
    sal_free(valstr);
    return rv;
}

/*
 * Auxiliary routine to perform either insert or delete
 */

#define TBL_OP_INSERT		0
#define TBL_OP_DELETE		1
#define TBL_OP_LOOKUP		2

static cmd_result_t
do_ins_del_lkup(int unit, args_t *a, int tbl_op)
{
    soc_control_t	*soc = SOC_CONTROL(unit);
    uint32		entry[SOC_MAX_MEM_WORDS];
    uint32		result[SOC_MAX_MEM_WORDS];
    int			entry_dw = 0, index;
    char		*ufname, *s;
    int			rv = CMD_FAIL;
    char		valstr[1024];
    soc_mem_t		mem;
    int			copyno, update, view_len = 0;
    int			swarl = 0;	/* Indicates operating on
					   software ARL/L2 table */
    char		*tab, *memname, *view = NULL;
    int			count = 1;
    soc_mem_t		arl_mem = INVALIDm;
    shr_avl_compare_fn	arl_cmp = NULL;
    int			quiet = 0;
    int32               banks = 0;
    int			i=0;
    int8                banknum = SOC_MEM_HASH_BANK_ALL;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    for (;;) {
	if ((tab = ARG_GET(a)) == NULL) {
	    return CMD_USAGE;
	}

	if (isint(tab)) {
	    count = parse_integer(tab);
	    continue;
	}

	if (sal_strcasecmp(tab, "quiet") == 0) {
	    quiet = 1;
	    continue;
	}
        
        if (soc_feature(unit, soc_feature_ism_memory) ||
            soc_feature(unit, soc_feature_shared_hash_mem)) {
            char *bnum;
            int8 num = -1;
            bnum = sal_strstr(tab, "bank");
            if (bnum != NULL) {
                num = sal_atoi(bnum+4);
                if (num >= 0 && num < 20) { 
                    banknum = num;
                }
	        continue;
            }
            
        } else {
	    if (sal_strcasecmp(tab, "bank0") == 0) {
	        banks = SOC_MEM_HASH_BANK0_ONLY;
	        continue;
	    }

	    if (sal_strcasecmp(tab, "bank1") == 0) {
	        banks = SOC_MEM_HASH_BANK1_ONLY;
	        continue;
	    }
        }  

	break;
    }

#ifdef BCM_XGS_SWITCH_SUPPORT
    if (soc_feature(unit, soc_feature_arl_hashed)) {
	arl_mem = L2Xm;
	arl_cmp = soc_l2x_entry_compare_key;
    }
#endif

    if (!sal_strcasecmp(tab, "sa") || !sal_strcasecmp(tab, "sarl")) {
	if (soc->arlShadow == NULL) {
	    cli_out("ERROR: No software ARL table\n");
	    goto done;
	}
	mem = arl_mem;
        if (mem != INVALIDm) {
            copyno = SOC_MEM_BLOCK_ANY(unit, mem);
        }
	swarl = 1;
    } else {
        memname = sal_strstr(tab, ":");
  	if (memname != NULL) {
  	    memname++;
  	    view_len = memname - tab;
  	} else {
  	    memname = tab;
  	}
        if (parse_memory_name(unit, &mem, memname, &copyno, 0) < 0) {
	    cli_out("ERROR: unknown table \"%s\"\n", tab);
	    goto done;
        }
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        LOG_ERROR(BSL_LS_APPL_SOCMEM,
                  (BSL_META_U(unit,
                              "Error: Memory %s not valid for chip %s.\n"),
                              (mem != INVALIDm) ? SOC_MEM_UFNAME(unit, mem) : "INVALID",
                   SOC_UNIT_NAME(unit)));
        goto done;
    }

    if (!soc_mem_is_sorted(unit, mem) &&
	!soc_mem_is_hashed(unit, mem) &&
        !soc_mem_is_cam(unit, mem) &&
        !soc_mem_is_cmd(unit, mem)) {
	cli_out("ERROR: %s table is not sorted, hashed, CAM, or command\n",
                SOC_MEM_UFNAME(unit, mem));
	goto done;
    }

    if ((soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) ||
        (soc_feature(unit, soc_feature_shared_hash_mem) && ((mem == L2Xm) ||
         (mem == L3_ENTRY_ONLYm) || (mem == L3_ENTRY_IPV4_UNICASTm) || 
         (mem == L3_ENTRY_IPV4_MULTICASTm) || (mem == L3_ENTRY_IPV6_UNICASTm) || 
         (mem == L3_ENTRY_IPV6_MULTICASTm)))) {
        if (banknum != SOC_MEM_HASH_BANK_ALL) {
            banks |= ((int32)1 << banknum);
        } else if (banks == 0) {
            banks = SOC_MEM_HASH_BANK_ALL;
        } 
    }
    /* All hash tables have multiple banks from TRX onwards only */
#ifndef BCM_TRX_SUPPORT
    if (soc_feature(unit, soc_feature_dual_hash) && !SOC_IS_TRX(unit)) {
        if (banks != 0) {
            switch (mem) {
            case L2Xm:
            case MPLS_ENTRYm:
            case VLAN_XLATEm:
            case EGR_VLAN_XLATEm: 
            case L3_ENTRY_IPV4_UNICASTm:
            case L3_ENTRY_IPV4_MULTICASTm:
            case L3_ENTRY_IPV6_UNICASTm:
            case L3_ENTRY_IPV6_MULTICASTm:
                 break;
            default:
                cli_out("ERROR: %s table does not have multiple banks\n",
                        SOC_MEM_UFNAME(unit, mem));
                goto done;
            }
        }
    }
#endif

    entry_dw = soc_mem_entry_words(unit, mem);
    ufname = (swarl ? "SARL" : SOC_MEM_UFNAME(unit, mem));

    if (tbl_op == TBL_OP_LOOKUP && copyno == COPYNO_ALL) {
	copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    /*
     * If a list of fields were specified, generate the entry from them.
     * Otherwise, generate it by reading raw dwords from command line.
     */

    if ((s = ARG_GET(a)) == 0) {
	cli_out("ERROR: missing data for entry to %s\n",
                tbl_op == TBL_OP_INSERT ? "insert" :
                tbl_op == TBL_OP_DELETE ? "delete" : "lookup");
	goto done;
    }

    if (!isint(s)) {
	/* List of fields */
        sal_memset(valstr, 0, 1024);
        if (view_len == 0) {
            collect_comma_args(a, valstr, s);
	} else {
  	    if ((view = sal_alloc(view_len + 1, "view_name")) == NULL) {
  	        cli_out("cmd_esw_mem_modify : Out of memory\n");
  	        goto done;
  	    }
  	    sal_memset(view, 0, view_len + 1);
  	    sal_memcpy(view, tab, view_len);
  	    if (collect_comma_args_with_view(a, valstr, s, view, unit, mem) < 0) {
  	        cli_out("Out of memory: aborted\n");
  	        goto done;
  	    }
  	}

	memset(entry, 0, sizeof (entry));

	if (modify_mem_fields(unit, mem, entry, NULL, valstr, FALSE) < 0) {
	    cli_out("Syntax error in field specification\n");
	    goto done;
	}

	update = TRUE;
    } else {
	/* List of numeric values */

	ARG_PREV(a);

	if (parse_dwords(entry_dw, entry, a) < 0) {
	    goto done;
	}

	update = FALSE;
    }

    if (bsl_check(bslLayerSoc, bslSourceSocmem, bslSeverityNormal, unit)) {
	cli_out("%s[%s], DATA:",
                tbl_op == TBL_OP_INSERT ? "INSERT" :
                tbl_op == TBL_OP_DELETE ? "DELETE" : "LOOKUP", ufname);
	for (i = 0; i < entry_dw; i++) {
	    cli_out(" 0x%x", entry[i]);
	}
	cli_out("\n");
    }

    /*
     * Have entry data, now insert, delete, or lookup.
     * For delete and lookup, all fields except the key are ignored.
     * Software ARL/L2 table requires special processing.
     */

    while (count--) {
	switch (tbl_op) {
	case TBL_OP_INSERT:
	    if (swarl) {
		if (soc->arlShadow != NULL) {
		    sal_mutex_take(soc->arlShadowMutex, sal_mutex_FOREVER);
		    i = shr_avl_insert(soc->arlShadow,
				       arl_cmp,
				       (shr_avl_datum_t *) entry);
		    sal_mutex_give(soc->arlShadowMutex);
		} else {
		    i = SOC_E_NONE;
		}
#ifdef BCM_TRIUMPH_SUPPORT
            } else if (mem == EXT_L2_ENTRYm) {
                i = soc_mem_generic_insert(unit, mem, MEM_BLOCK_ANY, 0,
                                           entry, NULL, 0);
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_ISM_SUPPORT
            } else if (soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) {
                if (banks == SOC_MEM_HASH_BANK_ALL) {
                    /* coverity[stack_use_callee] */
                    /* coverity[stack_use_overflow] */
                    i = soc_mem_insert(unit, mem, copyno, entry);
                } else {
                    i = soc_mem_bank_insert(unit, mem, banks,
                                            copyno, (void *)entry, NULL);
                }
#endif
#ifdef BCM_FIREBOLT2_SUPPORT
            } else if (banks != 0) {
                switch (mem) {
                case L2Xm:
                    if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                        i = soc_mem_bank_insert(unit, mem, banks,
                                                copyno, (void *)entry, NULL);
                    } else {
                        i = soc_fb_l2x_bank_insert(unit, banks,
                                                   (void *)entry);
                    }
                    break;
#if defined(INCLUDE_L3)
                case L3_ENTRY_IPV4_UNICASTm:
                case L3_ENTRY_IPV4_MULTICASTm:
                case L3_ENTRY_IPV6_UNICASTm:
                case L3_ENTRY_IPV6_MULTICASTm:
                    if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                        i = soc_mem_bank_insert(unit, mem, banks,
                                                copyno, (void *)entry, NULL);
                    } else {
                        i = soc_fb_l3x_bank_insert(unit, banks,
                                   (void *)entry);
                    }
                    break;
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
                case MPLS_ENTRYm:
#endif
                case VLAN_XLATEm:
                case VLAN_MACm:
                case AXP_WRX_WCDm:
                case AXP_WRX_SVP_ASSIGNMENTm:
#if defined(BCM_TRIUMPH3_SUPPORT)
                case FT_SESSIONm:
                case FT_SESSION_IPV6m:
#endif
                    i = soc_mem_bank_insert(unit, mem, banks,
                                            copyno, (void *)entry, NULL);
                    break;
                default:
                    i = SOC_E_INTERNAL;
                    goto done;
                }
#endif /* BCM_FIREBOLT2_SUPPORT */
	    } else {
		i = soc_mem_insert(unit, mem, copyno, entry);
	    }

            if (i == SOC_E_EXISTS) {
                i = SOC_E_NONE;
            }

	    if (quiet && i == SOC_E_FULL) {
		i = SOC_E_NONE;
	    }

	    if (i < 0) {
		cli_out("Insert ERROR: %s table insert failed: %s\n",
                        ufname, soc_errmsg(i));
		goto done;
	    }

	    break;

	case TBL_OP_DELETE:
	    if (swarl) {
		if (soc->arlShadow != NULL) {
		    sal_mutex_take(soc->arlShadowMutex, sal_mutex_FOREVER);
		    i = shr_avl_delete(soc->arlShadow,
				       arl_cmp,
				       (shr_avl_datum_t *) entry);
		    sal_mutex_give(soc->arlShadowMutex);
		} else {
		    i = SOC_E_NONE;
		}
#ifdef BCM_TRX_SUPPORT
            } else if (soc_feature(unit, soc_feature_generic_table_ops)) {
                i = soc_mem_generic_delete(unit, mem, MEM_BLOCK_ANY,
                                           banks, entry, NULL, 0);
#endif /* BCM_TRX_SUPPORT */
#ifdef BCM_FIREBOLT2_SUPPORT
            } else if (banks != 0) {
                switch (mem) {
                case L2Xm:
                    i = soc_fb_l2x_bank_delete(unit, banks,
                                               (void *)entry);
                    break;
#if defined(INCLUDE_L3)
                case L3_ENTRY_IPV4_UNICASTm:
                case L3_ENTRY_IPV4_MULTICASTm:
                case L3_ENTRY_IPV6_UNICASTm:
                case L3_ENTRY_IPV6_MULTICASTm:
                    i = soc_fb_l3x_bank_delete(unit, banks,
                               (void *)entry);
                    break;
#endif
                default:
                    i = SOC_E_INTERNAL;
                    goto done;
                }
#endif
	    } else {
		i = soc_mem_delete(unit, mem, copyno, entry);
	    }

	    if (quiet && i == SOC_E_NOT_FOUND) {
		i = SOC_E_NONE;
	    }

	    if (i < 0) {
		cli_out("Delete ERROR: %s table delete failed: %s\n",
                        ufname, soc_errmsg(i));
		goto done;
	    }

	    break;

	case TBL_OP_LOOKUP:
	    if (swarl) {
		if (soc->arlShadow != NULL) {
		    sal_mutex_take(soc->arlShadowMutex, sal_mutex_FOREVER);
		    i = shr_avl_lookup(soc->arlShadow,
				       arl_cmp,
				       (shr_avl_datum_t *) entry);
		    sal_mutex_give(soc->arlShadowMutex);
                    if (i) {
                        i = SOC_E_NONE;
                        index = -1;
                        sal_memcpy(result, entry,
                               soc_mem_entry_words(unit, mem) * 4);
                    } else {
                        i = SOC_E_NOT_FOUND;
                    }
		} else {
		    i = SOC_E_NONE;
                    goto done;
		}
#ifdef BCM_TRX_SUPPORT
            } else if (soc_feature(unit, soc_feature_generic_table_ops)) {
                i = soc_mem_generic_lookup(unit, mem, MEM_BLOCK_ANY,
                                           banks, entry, result, &index);
#endif /* BCM_TRX_SUPPORT */
#ifdef BCM_FIREBOLT2_SUPPORT
            } else if (banks != 0) {
                switch (mem) {
                case L2Xm:
                    i = soc_fb_l2x_bank_lookup(unit, banks,
                                               (void *)entry,
                                               (void *)result,
                                               &index);
                    break;
#if defined(INCLUDE_L3)
                case L3_ENTRY_IPV4_UNICASTm:
                case L3_ENTRY_IPV4_MULTICASTm:
                case L3_ENTRY_IPV6_UNICASTm:
                case L3_ENTRY_IPV6_MULTICASTm:
                    i = soc_fb_l3x_bank_lookup(unit, banks,
                                               (void *)entry,
                                               (void *)result,
                                               &index);
                    break;
#endif
                default:
                    i = SOC_E_INTERNAL;
                    goto done;
                }
#endif /* BCM_FIREBOLT2_SUPPORT */
	    } else {
		i = soc_mem_search(unit, mem, copyno,
                                   &index, entry, result, 0);
	    }

	    if (i < 0) {		/* Error not fatal for lookup */
                if (i == SOC_E_NOT_FOUND) {
                    cli_out("Lookup: No matching entry found\n");
                } else {
                    cli_out("Lookup ERROR: read error during search: %s\n",
                            soc_errmsg(i));
                }
	    } else { /* Entry found */
		if (index < 0) {
		    cli_out("Found in %s.%s: ",
                            ufname, SOC_BLOCK_NAME(unit, copyno));
		} else {
		    cli_out("Found %s.%s[%d]: ",
                            ufname, SOC_BLOCK_NAME(unit, copyno), index);
		}
		soc_mem_entry_dump(unit, mem, result);
		cli_out("\n");
            }

	    break;

	default:
	    assert(0);
	    break;
	}

	if (update) {
	    modify_mem_fields(unit, mem, entry, NULL, valstr, TRUE);
	}
    }

    rv = CMD_OK;

 done:
    if (view != NULL) {
        sal_free(view);
    }
    return rv;
}

/*
 * Insert an entry by key into a sorted table
 */

char insert_usage[] =
    "\nParameters 1: [quiet] [<COUNT>] [bank#] <TABLE> <DW0> .. <DWN>\n"
    "Parameters 2: [quiet] [<COUNT>] <TABLE> <FIELD>=<VALUE> ...\n\t"
    "Number of <DW> must be appropriate to table entry size.\n\t"
    "Entry is inserted in ascending sorted order by key field.\n\t"
    "The quiet option indicates not to complain on table/bucket full.\n\t"
    "Some tables allow restricting the insert to a single bank.\n";

cmd_result_t
mem_insert(int unit, args_t *a)
{
    return do_ins_del_lkup(unit, a, TBL_OP_INSERT);
}


/*
 * Delete entries by key from a sorted table
 */

char delete_usage[] =
    "\nParameters 1: [quiet] [<COUNT>] [bank#] <TABLE> <DW0> .. <DWN>\n"
    "Parameters 2: [quiet] [<COUNT>] <TABLE> <FIELD>=<VALUE> ...\n\t"
    "Number of <DW> must be appropriate to table entry size.\n\t"
    "The entry is deleted by key.\n\t"
    "All fields other than the key field(s) are ignored.\n\t"
    "The quiet option indicates not to complain on entry not found.\n\t"
    "Some tables allow restricting the delete to a single bank.\n";

cmd_result_t
mem_delete(int unit, args_t *a)
{
    return do_ins_del_lkup(unit, a, TBL_OP_DELETE);
}

/*
 * Lookup entry an key in a sorted table
 */

char lookup_usage[] =
    "\nParameters 1: [<COUNT>] [bank#] <TABLE> <DW0> .. <DWN>\n"
    "Parameters 2: [<COUNT>] <TABLE> <FIELD>=<VALUE> ...\n\t"
    "Number of <DW> must be appropriate to table entry size.\n\t"
    "The entry is looked up by key.\n\t"
    "All fields other than the key field(s) are ignored.\n\t"
    "Some tables allow restricting the lookup to a single bank.\n";

cmd_result_t
mem_lookup(int unit, args_t *a)
{
    return do_ins_del_lkup(unit, a, TBL_OP_LOOKUP);
}

/*
 * Remove (delete) entries by index from a sorted table
 */

char remove_usage[] =
    "Parameters: <TABLE> <INDEX> [COUNT]\n\t"
    "Deletes an entry from a table by index.\n\t"
    "(For the ARL, reads the table entry and deletes by key).\n";

cmd_result_t
mem_remove(int unit, args_t *a)
{
    char		*tab, *ind, *cnt, *ufname;
    soc_mem_t		mem;
    int			copyno;
    int			r, rv = CMD_FAIL;
    int			index, count;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	goto done;
    }

    tab = ARG_GET(a);
    ind = ARG_GET(a);
    cnt = ARG_GET(a);

    if (!tab || !ind) {
	return CMD_USAGE;
    }

    if (parse_memory_name(unit, &mem, tab, &copyno, 0) < 0) {
	cli_out("ERROR: unknown table \"%s\"\n", tab);
	goto done;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        LOG_ERROR(BSL_LS_APPL_SOCMEM,
                  (BSL_META_U(unit,
                              "Error: Memory %s not valid for chip %s.\n"),
                              SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit)));
        goto done;
    }

    ufname = SOC_MEM_UFNAME(unit, mem);

    if (!soc_mem_is_sorted(unit, mem) &&
	!soc_mem_is_hashed(unit, mem) &&
	!soc_mem_is_cam(unit, mem)) {
	cli_out("ERROR: %s table is not sorted, hashed, or CAM\n",
                ufname);
	goto done;
    }

    count = cnt ? parse_integer(cnt) : 1;

    index = parse_memory_index(unit, mem, ind);

    while (count-- > 0)
	if ((r = soc_mem_delete_index(unit, mem, copyno, index)) < 0) {
	    cli_out("ERROR: delete %s table index 0x%x failed: %s\n",
                    ufname, index, soc_errmsg(r));
	    goto done;
	}

    rv = CMD_OK;

 done:
    return rv;
}

/*  
 * Pop an entry from a FIFO
 */

char pop_usage[] =
    "\nParameters: [quiet] [<COUNT>] <TABLE>\n"
    "The quiet option indicates not to complain on FIFO empty.\n";

cmd_result_t
mem_pop(int unit, args_t *a)
{   
    uint32              result[SOC_MAX_MEM_WORDS];
    char                *ufname;
    int                 rv = CMD_FAIL;
    soc_mem_t           mem;
    int                 copyno;
    char                *tab; 
    int                 count = 1;
    int                 quiet = 0;
    int                 i;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    if (!soc_feature(unit, soc_feature_mem_push_pop)) {
        /* feature not supported */
        return CMD_FAIL;
    }

    for (;;) {
        if ((tab = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }

        if (isint(tab)) {
            count = parse_integer(tab);
            continue;
        }

        if (sal_strcasecmp(tab, "quiet") == 0) {
            quiet = 1;
            continue;
        }
        break;
    }

    if (parse_memory_name(unit, &mem, tab, &copyno, 0) < 0) {
        cli_out("ERROR: unknown table \"%s\"\n", tab);
        goto done;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        LOG_ERROR(BSL_LS_APPL_SOCMEM,
                  (BSL_META_U(unit,
                              "Error: Memory %s not valid for chip %s.\n"),
                              SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit)));
        goto done;
    }

    switch (mem) {
#if defined (BCM_TRIUMPH_SUPPORT)
        case ING_IPFIX_EXPORT_FIFOm:
        case EGR_IPFIX_EXPORT_FIFOm:
        case EXT_L2_MOD_FIFOm:
#endif /* BCM_TRIUMPH_SUPPORT */
        case L2_MOD_FIFOm:
            break;
        default:
            cli_out("ERROR: %s table does not support FIFO push/pop\n",
                    SOC_MEM_UFNAME(unit, mem));
            goto done;
    }

    ufname = SOC_MEM_UFNAME(unit, mem);

    if (copyno == COPYNO_ALL) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    if (bsl_check(bslLayerSoc, bslSourceSocmem, bslSeverityNormal, unit)) {
        cli_out("POP[%s]", ufname);
        cli_out("\n");
    }

    while (count--) {
        i = soc_mem_pop(unit, mem, copyno, result);

        if (i < 0) {            /* Error not fatal for lookup */
            if (i != SOC_E_NOT_FOUND) {
                cli_out("Pop ERROR: read error during pop: %s\n",
                        soc_errmsg(i));
            } else if (!quiet) {
                cli_out("Pop: Fifo empty\n");
            }
        } else { /* Entry popped */
            cli_out("Popped in %s.%s: ",
                    ufname, SOC_BLOCK_NAME(unit, copyno));
            soc_mem_entry_dump(unit, mem, result);
            cli_out("\n");
        }
    }

    rv = CMD_OK;

 done:
    return rv;
}

/*
 * Push an entry onto a FIFO
 */

char push_usage[] =
    "\nParameters: [quiet] [<COUNT>] <TABLE>\n"
    "The quiet option indicates not to complain on FIFO full.\n";

cmd_result_t
mem_push(int unit, args_t *a)
{
    uint32              entry[SOC_MAX_MEM_WORDS];
    int                 entry_dw;
    char                *ufname, *s;
    int                 rv = CMD_FAIL;
    char                valstr[1024];
    soc_mem_t           mem;
    int                 copyno;
    char                *tab;
    int                 count = 1;
    int                 quiet = 0;
    int                 i;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    if (!soc_feature(unit, soc_feature_mem_push_pop)) {
        /* feature not supported */
        return CMD_FAIL;
    }

    for (;;) {
        if ((tab = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }

        if (isint(tab)) {
            count = parse_integer(tab);
            continue;
        }

        if (sal_strcasecmp(tab, "quiet") == 0) {
            quiet = 1;
            continue;
        }
        break;
    }

    if (parse_memory_name(unit, &mem, tab, &copyno, 0) < 0) {
        cli_out("ERROR: unknown table \"%s\"\n", tab);
        goto done;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        LOG_ERROR(BSL_LS_APPL_SOCMEM,
                  (BSL_META_U(unit,
                              "Error: Memory %s not valid for chip %s.\n"),
                              SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit)));
        goto done;
    }

    switch (mem) { 
#if defined (BCM_TRIUMPH_SUPPORT)
        case ING_IPFIX_EXPORT_FIFOm:
        case EGR_IPFIX_EXPORT_FIFOm:
        case EXT_L2_MOD_FIFOm:
#endif /* BCM_TRIUMPH_SUPPORT */
        case L2_MOD_FIFOm:
            break;
        default:
            cli_out("ERROR: %s table does not support FIFO push/pop\n",
                    SOC_MEM_UFNAME(unit, mem));
            goto done;
    }
    
    entry_dw = soc_mem_entry_words(unit, mem);
    ufname = SOC_MEM_UFNAME(unit, mem);
    
    if (copyno == COPYNO_ALL) { 
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    /* 
     * If a list of fields were specified, generate the entry from them.
     * Otherwise, generate it by reading raw dwords from command line.
     */

    if ((s = ARG_GET(a)) == 0) { 
        cli_out("ERROR: missing data for entry to push\n");
        goto done;
    }

    if (!isint(s)) {
        /* List of fields */

        collect_comma_args(a, valstr, s);

        sal_memset(entry, 0, sizeof (entry));

        if (modify_mem_fields(unit, mem, entry, NULL, valstr, FALSE) < 0) {
            cli_out("Syntax error in field specification\n");
            goto done;
        }
    } else {
        /* List of numeric values */

        ARG_PREV(a);

        if (parse_dwords(entry_dw, entry, a) < 0) {
            goto done;
        }
    }

    if (bsl_check(bslLayerSoc, bslSourceSocmem, bslSeverityNormal, unit)) {
        cli_out("PUSH[%s], DATA:", ufname);
        for (i = 0; i < entry_dw; i++) {
            cli_out(" 0x%x", entry[i]);
        }
        cli_out("\n");
    }

    /*
     * Have entry data, push entry.
     */

    while (count--) {
        i = soc_mem_push(unit, mem, copyno, entry);
        if (quiet && i == SOC_E_FULL) {
            i = SOC_E_NONE;
        }

        if (i < 0) {
            cli_out("Push ERROR: %s table push failed: %s\n",
                    ufname, soc_errmsg(i));
            goto done;
        }
    }

    rv = CMD_OK;

 done:
    return rv;
}

/*
 * do_search_entry
 *
 *  Auxiliary routine for command_search()
 *  Returns byte position within entry where pattern is found.
 *  Returns -1 if not found.
 *  Finds pattern in either big- or little-endian order.
 */

static int
do_search_entry(uint8 *entry, int entry_len,
		uint8 *pat, int pat_len)
{
    int start, i;

    for (start = 0; start <= entry_len - pat_len; start++) {
	for (i = 0; i < pat_len; i++) {
	    if (entry[start + i] != pat[i]) {
		break;
	    }
	}

	if (i == pat_len) {
	    return start;
	}

	for (i = 0; i < pat_len; i++) {
	    if (entry[start + i] != pat[pat_len - 1 - i]) {
		break;
	    }
	}

	if (i == pat_len) {
	    return start;
	}
    }

    return -1;
}

/*
 * Search a table for a byte pattern
 */

#define SRCH_MAX_PAT		16

char search_usage[] =
    "1. Parameters: [ALL | BIN | FBIN] <TABLE>[.<COPY>] 0x<PATTERN>\n"
#ifndef COMPILER_STRING_CONST_LIMIT
    "\tDumps table entries containing the specified byte pattern\n\t"
    "anywhere in the entry in big- or little-endian order.\n\t"
    "Example: search arl 0x112233445566\n"
#endif
    "2. Parameters: [ALL | BIN | FBIN] <TABLE>[.<COPY>]"
         "<FIELD>=<VALUE>[,...]\n"
#ifndef COMPILER_STRING_CONST_LIMIT
    "\tDumps table entries where the specified fields are equal\n\t"
    "to the specified values.\n\t"
    "Example: search l3 ip_addr=0xf4000001,pnum=10\n\t"
    "The ALL flag indicates to search sorted tables in their\n\t"
    "entirety instead of only entries known to contain data.\n\t"
    "The BIN flag does a binary search of sorted tables in\n\t"
    "the portion known to contain data.\n\t"
    "The FBIN flag does a binary search, and guarantees if there\n\t"
    "are duplicates the lowest matching index will be returned.\n"
#endif
    ;

cmd_result_t
mem_search(int unit, args_t *a)
{
    int			t, rv = CMD_FAIL;
    char		*tab, *patstr;
    soc_mem_t		mem;
    uint32		pat_entry[SOC_MAX_MEM_WORDS];
    uint32		pat_mask[SOC_MAX_MEM_WORDS];
    uint32		entry[SOC_MAX_MEM_WORDS];
    char		valstr[1024];
    uint8		pat[SRCH_MAX_PAT];
    char		*ufname;
    int			patsize = 0, found_count = 0, entry_bytes, entry_dw;
    int			copyno, index, min, max;
    int			byte_search;
    int			all_flag = 0, bin_flag = 0, lowest_match = 0;
#ifdef BCM_TRIDENT2_SUPPORT
    int       check_view = FALSE;
    soc_mem_t view;
#endif /* BCM_TRIDENT2_SUPPORT */

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	goto done;
    }

    tab = ARG_GET(a);

    while (tab) {
	if (!sal_strcasecmp(tab, "all")) {
	    all_flag = 1;
	} else if (!sal_strcasecmp(tab, "bin")) {
	    bin_flag = 1;
	} else if (!sal_strcasecmp(tab, "fbin")) {
	    bin_flag = 1;
	    lowest_match = 1;
	} else {
	    break;
	}
	tab = ARG_GET(a);
    }

    patstr = ARG_GET(a);

    if (!tab || !patstr) {
	return CMD_USAGE;
    }

    if (parse_memory_name(unit, &mem, tab, &copyno, 0) < 0) {
	cli_out("ERROR: unknown table \"%s\"\n", tab);
	goto done;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        LOG_ERROR(BSL_LS_APPL_SOCMEM,
                  (BSL_META_U(unit,
                              "Error: Memory %s not valid for chip %s.\n"),
                              SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit)));
        goto done;
    }

    if (copyno == COPYNO_ALL) {
	copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    entry_bytes = soc_mem_entry_bytes(unit, mem);
    entry_dw = BYTES2WORDS(entry_bytes);
    min = soc_mem_index_min(unit, mem);
    max = soc_mem_index_max(unit, mem);
    ufname = SOC_MEM_UFNAME(unit, mem);

    byte_search = isint(patstr);

    if (byte_search) {
	char *s;

	/*
	 * Search by pattern string.
	 * Parse pattern string (long hex number) into array of bytes.
	 */

	if (patstr[0] != '0' ||
	    (patstr[1] != 'x' && patstr[1] != 'X') || patstr[2] == 0) {
	    cli_out("ERROR: illegal search pattern, need hex constant\n");
	    goto done;
	}

	patstr += 2;

	for (s = patstr; *s; s++) {
	    if (!isxdigit((unsigned) *s)) {
		cli_out("ERROR: invalid hex digit in search pattern\n");
		goto done;
	    }
	}

	while (s > patstr + 1) {
	    s -= 2;
	    pat[patsize++] = xdigit2i(s[0]) << 4 | xdigit2i(s[1]);
	}

	if (s > patstr) {
	    s--;
	    pat[patsize++] = xdigit2i(s[0]);
	}

#if 0
	cli_out("PATTERN: ");
	for (index = 0; index < patsize; index++) {
	    cli_out("%02x ", pat[index]);
	}
	cli_out("\n");
#endif
    } else {
	/*
	 * Search by pat_entry and pat_mask.
	 * Collect list of fields
	 */

	collect_comma_args(a, valstr, patstr);

	memset(pat_entry, 0, sizeof (pat_entry));

	if (modify_mem_fields(unit, mem,
			      pat_entry, pat_mask,
			      valstr, FALSE) < 0) {
	    cli_out("Syntax error: aborted\n");
	    goto done;
	}
    }

    if (bin_flag) {
	if (!soc_mem_is_sorted(unit, mem)) {
	    cli_out("ERROR: can only binary search sorted tables\n");
	    goto done;
	}

	if (byte_search) {
	    cli_out("ERROR: can only binary search for "
                    "<FIELD>=<VALUE>[,...]\n");
	    goto done;
	}

	cli_out("Searching %s...\n", ufname);

	t = soc_mem_search(unit, mem, copyno, &index,
				pat_entry, entry, lowest_match);
        if ((t < 0) && (t != SOC_E_NOT_FOUND)) {
	    cli_out("Read ERROR: table[%s]: %s\n", ufname, soc_errmsg(t));
	    goto done;
	}

	if (t == SOC_E_NONE) {
	    cli_out("%s[%d]: ", ufname, index);
	    soc_mem_entry_dump(unit, mem, entry);
	    cli_out("\n");
	} else {
	    cli_out("Nothing found\n");
        }

	rv = CMD_OK;
	goto done;
    }

    if (!all_flag && soc_mem_is_sorted(unit, mem)) {
	max = soc_mem_index_last(unit, mem, copyno);
    }

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit) &&
        (mem == L3_DEFIP_ALPM_IPV4m || mem == L3_DEFIP_ALPM_IPV4_1m ||
         mem == L3_DEFIP_ALPM_IPV6_64m || mem == L3_DEFIP_ALPM_IPV6_64_1m ||
         mem == L3_DEFIP_ALPM_IPV6_128m)) {
        check_view = TRUE;
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    cli_out("Searching %s table indexes 0x%x through 0x%x...\n",
            ufname, min, max);

    for (index = min; index <= max; index++) {
	int match;

#ifdef BCM_TRIDENT2_SUPPORT
        if (check_view) {
            view = _soc_trident2_alpm_bkt_view_get(unit, index);
            if (!(view == INVALIDm || mem == view)) {
                continue;
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */

	if ((t = soc_mem_read(unit, mem, copyno, index, entry)) < 0) {
	    cli_out("Read ERROR: table %s.%s[%d]: %s\n",
                    ufname, SOC_BLOCK_NAME(unit, copyno), index, soc_errmsg(t));
	    goto done;
	}

#ifdef BCM_XGS_SWITCH_SUPPORT
	if (!all_flag) {
#ifdef BCM_XGS12_SWITCH_SUPPORT
            if SOC_IS_XGS12_SWITCH(unit) {
	        if (mem == L2Xm &&
		    !soc_L2Xm_field32_get(unit, entry, VALID_BITf)) {
		    continue;
	        }

	        if (mem == L3Xm &&
		    !soc_L3Xm_field32_get(unit, entry, L3_VALIDf)) {
		    continue;
	        }
            }
#endif /* BCM_XGS12_SWITCH_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
#ifdef BCM_FIREBOLT_SUPPORT
            if (SOC_IS_FBX(unit) || SOC_IS_RAPTOR(unit) || SOC_IS_RAVEN(unit)) { 
                switch (mem) {
                    case L2Xm:
                        if (!soc_L2Xm_field32_get(unit, entry, VALIDf)) {
                            continue;
                        }
                        break; 
                    case L2_USER_ENTRYm:
                        if (!soc_L2_USER_ENTRYm_field32_get(unit, 
                               entry, VALIDf)) {
                            continue;
                        }
                        break;
                    case L3_ENTRY_IPV4_UNICASTm:
                        if (!soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, 
                               entry, VALIDf)) {
                            continue;
                        }
                        break;
                    case L3_ENTRY_IPV4_MULTICASTm:
                        if (soc_mem_field_valid(unit, mem, VALIDf)) {
                            if (!soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit,
                                   entry, VALIDf)) {
                                continue;
                            }
                        }

                        if (soc_mem_field_valid(unit, mem, VALID_0f) &&
                            soc_mem_field_valid(unit, mem, VALID_1f)) {
                            if (!(soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit,
                                   entry, VALID_0f) &&
                                  soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit,
                                   entry, VALID_1f))) {
                                continue;
                            }
                        }
                        break;
                    case L3_ENTRY_IPV6_UNICASTm:
                        if (!(soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit,
                                entry, VALID_0f) && 
                              soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit,
                                entry, VALID_1f))) {
                            continue;
                        }
                        break;
                    case L3_ENTRY_IPV6_MULTICASTm:
                        if (!(soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(unit,
                                entry, VALID_0f) &&
                              soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(unit,
                                entry, VALID_1f) &&
                              soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(unit,
                                entry, VALID_2f) &&
                              soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(unit, 
                                entry, VALID_3f))) {
                            continue;
                        }
                        break;
                    default:
                        break;
                }
            }
#endif /* BCM_FIREBOLT_SUPPORT */
#endif /* BCM_XGS3_SWITCH_SUPPORT */
	}
#endif /* BCM_XGS_SWITCH_SUPPORT */

	if (byte_search) {
	    match = (do_search_entry((uint8 *) entry, entry_bytes,
				     pat, patsize) >= 0);
	} else {
	    int i;

	    for (i = 0; i < entry_dw; i++)
		if ((entry[i] & pat_mask[i]) != pat_entry[i])
		    break;

	    match = (i == entry_dw);
	}

	if (match) {
	    cli_out("%s.%s[%d]: ",
                    ufname, SOC_BLOCK_NAME(unit, copyno), index);
	    soc_mem_entry_dump(unit, mem, entry);
	    cli_out("\n");
	    found_count++;
	}
    }

    if (found_count == 0) {
	cli_out("Nothing found\n");
    }

    rv = CMD_OK;

 done:
    return rv;
}

/*
 * Print a one line summary for matching memories
 * If substr_match is NULL, match all memories.
 * If substr_match is non-NULL, match any memories whose name
 * or user-friendly name contains that substring.
 */

static void
mem_list_summary(int unit, char *substr_match)
{
    soc_mem_t		mem;
    int			i, copies, dlen;
    int			found = 0;
    char		*dstr;

    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
	if (!soc_mem_is_valid(unit, mem)) {
	    continue;
        }

#if defined(BCM_HAWKEYE_SUPPORT)
        if (SOC_IS_HAWKEYE(unit) && (soc_mem_index_max(unit, mem) <= 0)) {
            continue;
        }
#endif /* BCM_HAWKEYE_SUPPORT */

	if (substr_match != NULL &&
	    strcaseindex(SOC_MEM_NAME(unit, mem), substr_match) == NULL &&
	    strcaseindex(SOC_MEM_UFNAME(unit, mem), substr_match) == NULL) {
	    continue;
	}

	copies = 0;
	SOC_MEM_BLOCK_ITER(unit, mem, i) {
	    copies += 1;
	}

	dlen = sal_strlen(SOC_MEM_DESC(unit, mem));
	if (dlen > 38) {
	    dlen = 34;
	    dstr = "...";
	} else {
	    dstr = "";
	}
	if (!found) {
	    cli_out(" %-6s  %-22s%5s/%-4s %s\n",
                    "Flags", "Name", "Entry",
                    "Copy", "Description");
	    found = 1;
	}

	cli_out(" %c%c%c%c%c%c  %-22s%5d",
                soc_mem_is_readonly(unit, mem) ? 'r' : '-',
                soc_mem_is_debug(unit, mem) ? 'd' : '-',
                soc_mem_is_sorted(unit, mem) ? 's' :
                soc_mem_is_hashed(unit, mem) ? 'h' :
                soc_mem_is_cam(unit, mem) ? 'A' : '-',
                soc_mem_is_cbp(unit, mem) ? 'c' : '-',
                (soc_mem_is_bistepic(unit, mem) ||
                soc_mem_is_bistffp(unit, mem) ||
                soc_mem_is_bistcbp(unit, mem)) ? 'b' : '-',
                soc_mem_is_cachable(unit, mem) ? 'C' : '-',
                SOC_MEM_UFNAME(unit, mem),
                soc_mem_index_count(unit, mem));
	if (copies == 1) {
	    cli_out("%5s %*.*s%s\n",
                    "",
                    dlen, dlen, SOC_MEM_DESC(unit, mem), dstr);
	} else {
	    cli_out("/%-4d %*.*s%s\n",
                    copies,
                    dlen, dlen, SOC_MEM_DESC(unit, mem), dstr);
	}
    }

    if (found) {
	cli_out("Flags: (r)eadonly, (d)ebug, (s)orted, (h)ashed\n"
                "       C(A)M, (c)bp, (b)ist-able, (C)achable\n");
    } else if (substr_match != NULL) {
	cli_out("No memory found with the substring '%s' in its name.\n",
                substr_match);
    }
}

/*
 * List the tables, or fields of a table entry
 */

cmd_result_t
cmd_esw_mem_list(int unit, args_t *a)
{
    soc_mem_info_t	*m;
    soc_field_info_t	*fld;
    char		*tab, *s;
    soc_mem_t		mem;
    uint32		entry[SOC_MAX_MEM_WORDS];
    uint32		mask[SOC_MAX_MEM_WORDS];
    int			have_entry, i, dw, copyno;
    int			copies, disabled, dmaable;
    char		*dmastr;
    uint32		flags;
    int			minidx, maxidx;
    uint8               at;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (!soc_property_get(unit, spn_MEMLIST_ENABLE, 1)) {
        return CMD_OK;
    }
    tab = ARG_GET(a);

    if (!tab) {
	mem_list_summary(unit, NULL);
	return CMD_OK;
    }

    if (parse_memory_name(unit, &mem, tab, &copyno, 0) < 0) {
	if ((s = strchr(tab, '.')) != NULL) {
	    *s = 0;
	}
	mem_list_summary(unit, tab);
	return CMD_OK;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
	cli_out("ERROR: Memory \"%s\" not valid for this unit\n", tab);
	return CMD_FAIL;
    }

#if defined(BCM_HAWKEYE_SUPPORT)
    if (SOC_IS_HAWKEYE(unit) && (soc_mem_index_max(unit, mem) <= 0)) {
        cli_out("ERROR: Memory \"%s\" not valid for this unit\n", tab);
        return CMD_FAIL;
    }
#endif /* BCM_HAWKEYE_SUPPORT */

    if (copyno < 0) {
	copyno = SOC_MEM_BLOCK_ANY(unit, mem);
        if (copyno < 0) {
            cli_out("ERROR: Memory \"%s\" has no instance on this unit\n", tab);
            return CMD_FAIL;
        }
    } else if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
	cli_out("ERROR: Invalid copy number %d for memory %s\n", copyno, tab);
	return CMD_FAIL;
    }

    m = &SOC_MEM_INFO(unit, mem);
    flags = m->flags;

    dw = BYTES2WORDS(m->bytes);

    if ((s = ARG_GET(a)) == 0) {
	have_entry = 0;
    } else {
	for (i = 0; i < dw; i++) {
	    if (s == 0) {
		cli_out("Not enough data specified (%d words needed)\n", dw);
		return CMD_FAIL;
	    }
	    entry[i] = parse_integer(s);
	    s = ARG_GET(a);
	}
	if (s) {
	    cli_out("Extra data specified (ignored)\n");
	}
	have_entry = 1;
    }

    cli_out("Memory: %s.%s",
            SOC_MEM_UFNAME(unit, mem),
            SOC_BLOCK_NAME(unit, copyno));
    s = SOC_MEM_UFALIAS(unit, mem);
    if (s && *s && sal_strcmp(SOC_MEM_UFNAME(unit, mem), s) != 0) {
	cli_out(" alias %s", s);
    }
    cli_out(" address 0x%08x\n", soc_mem_addr_get(unit, mem, 0, copyno, 0, &at));

    cli_out("Flags:");
    if (flags & SOC_MEM_FLAG_READONLY) {
	cli_out(" read-only");
    }
    if (flags & SOC_MEM_FLAG_VALID) {
	cli_out(" valid");
    }
    if (flags & SOC_MEM_FLAG_DEBUG) {
	cli_out(" debug");
    }
    if (flags & SOC_MEM_FLAG_SORTED) {
	cli_out(" sorted");
    }
    if (flags & SOC_MEM_FLAG_CBP) {
	cli_out(" cbp");
    }
    if (flags & SOC_MEM_FLAG_CACHABLE) {
	cli_out(" cachable");
    }
    if (flags & SOC_MEM_FLAG_BISTCBP) {
	cli_out(" bist-cbp");
    }
    if (flags & SOC_MEM_FLAG_BISTEPIC) {
	cli_out(" bist-epic");
    }
    if (flags & SOC_MEM_FLAG_BISTFFP) {
	cli_out(" bist-ffp");
    }
    if (flags & SOC_MEM_FLAG_UNIFIED) {
	cli_out(" unified");
    }
    if (flags & SOC_MEM_FLAG_HASHED) {
	cli_out(" hashed");
    }
    if (flags & SOC_MEM_FLAG_WORDADR) {
	cli_out(" word-addressed");
    }
    if (flags & SOC_MEM_FLAG_BE) {
	cli_out(" big-endian");
    }
    if (flags & SOC_MEM_FLAG_MULTIVIEW) {
        cli_out(" multiview");
    }
    cli_out("\n");

    cli_out("Blocks: ");
    copies = disabled = dmaable = 0;
    SOC_MEM_BLOCK_ITER(unit, mem, i) {
	if (SOC_INFO(unit).block_valid[i]) {
	    dmastr = "";
#ifdef BCM_XGS_SWITCH_SUPPORT
	    if (soc_mem_dmaable(unit, mem, i)) {
		dmastr = "/dma";
		dmaable += 1;
	    }
#endif
	    cli_out(" %s%s", SOC_BLOCK_NAME(unit, i), dmastr);
	} else {
	    cli_out(" [%s]", SOC_BLOCK_NAME(unit, i));
	    disabled += 1;
	}
	copies += 1;
    }
    cli_out(" (%d cop%s", copies, copies == 1 ? "y" : "ies");
    if (disabled) {
	cli_out(", %d disabled", disabled);
    }
#ifdef BCM_XGS_SWITCH_SUPPORT
    if (dmaable) {
	cli_out(", %d dmaable", dmaable);
    }
#endif
    cli_out(")\n");

    minidx = soc_mem_index_min(unit, mem);
    maxidx = soc_mem_index_max(unit, mem);
    cli_out("Entries: %d with indices %d-%d (0x%x-0x%x)",
            maxidx - minidx + 1,
            minidx,
            maxidx,
            minidx,
            maxidx);
    cli_out(", each %d bytes %d words\n", m->bytes, dw);

    cli_out("Entry mask:");
    soc_mem_datamask_get(unit, mem, mask);
    for (i = 0; i < dw; i++) {
	if (mask[i] == 0xffffffff) {
	    cli_out(" -1");
	} else if (mask[i] == 0) {
	    cli_out(" 0");
	} else {
	    cli_out(" 0x%08x", mask[i]);
	}
    }
    cli_out("\n");

#ifdef BCM_ISM_SUPPORT
    if (soc_feature(unit, soc_feature_ism_memory)) {
        uint8 banks[_SOC_ISM_MAX_BANKS] = {0};
        uint32 bank_size[_SOC_ISM_MAX_BANKS];
        uint8 count;
        if (soc_ism_get_banks_for_mem(unit, mem, banks, bank_size, &count) 
            == SOC_E_NONE) {
            if (count) {
                uint8 bc;
                uint32 bmask = 0;
                for (bc = 0; bc < count; bc++) {
                    bmask |= 1 << banks[bc];
                }
                cli_out("ISM bank mask: 0x%x\n", bmask);
            }
        }
    }
#endif

    s = SOC_MEM_DESC(unit, mem);
    if (s && *s) {
	cli_out("Description: %s\n", s);
    }

    for (fld = &m->fields[m->nFields - 1]; fld >= &m->fields[0]; fld--) {
	cli_out("  %s<%d", SOC_FIELD_NAME(unit, fld->field), fld->bp + fld->len - 1);
	if (fld->len > 1) {
	    cli_out(":%d", fld->bp);
	}
	if (have_entry) {
	    uint32 fval[SOC_MAX_MEM_FIELD_WORDS];
	    char tmp[132];

	    sal_memset(fval, 0, sizeof (fval));
	    soc_mem_field_get(unit, mem, entry, fld->field, fval);
	    format_long_integer(tmp, fval, SOC_MAX_MEM_FIELD_WORDS);
	    cli_out("> = %s\n", tmp);
	} else {
	    cli_out(">\n");
	}
    }

    return CMD_OK;
}

/*
 * Turn on/off software caching of hardware tables
 */

cmd_result_t
mem_esw_cache(int unit, args_t *a)
{
    soc_mem_t		mem;
    int			copyno;
    char		*c;
    int			enable, r;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (ARG_CNT(a) == 0) {
	for (enable = 0; enable < 2; enable++) {
	    cli_out("Caching is %s for:\n", enable ? "on" : "off");

	    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
		int any_printed;

		if ((!SOC_MEM_IS_VALID(unit, mem)) ||
                    (!soc_mem_is_cachable(unit, mem))) {
		    continue;
                }

		any_printed = 0;

		SOC_MEM_BLOCK_ITER(unit, mem, copyno) {
		    if ((!enable) ==
			(!soc_mem_cache_get(unit, mem, copyno))) {
			if (!any_printed) {
			    cli_out("    ");
			}
			cli_out(" %s.%s",
                                SOC_MEM_UFNAME(unit, mem),
                                SOC_BLOCK_NAME(unit, copyno));
			any_printed = 1;
		    }
		}

		if (any_printed) {
		    cli_out("\n");
		}
	    }
	}

	return CMD_OK;
    }

    while ((c = ARG_GET(a)) != 0) {
	switch (*c) {
	case '+':
	    enable = 1;
	    c++;
	    break;
	case '-':
	    enable = 0;
	    c++;
	    break;
	default:
	    enable = 1;
	    break;
	}

	if (parse_memory_name(unit, &mem, c, &copyno, 0) < 0) {
	    cli_out("%s: Unknown table \"%s\"\n", ARG_CMD(a), c);
	    return CMD_FAIL;
	}

        if (!SOC_MEM_IS_VALID(unit, mem)) {
            LOG_ERROR(BSL_LS_APPL_SOCMEM,
                      (BSL_META_U(unit,
                                  "Error: Memory %s not valid for chip %s.\n"),
                                  SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit)));
            continue;
        }

	if (!soc_mem_is_cachable(unit, mem)) {
	    cli_out("%s: Memory %s is not cachable\n",
                    ARG_CMD(a), SOC_MEM_UFNAME(unit, mem));
	    return CMD_FAIL;
	}

	if ((r = soc_mem_cache_set(unit, mem, copyno, enable)) < 0) {
	    cli_out("%s: Error setting cachability for %s: %s\n",
                    ARG_CMD(a), SOC_MEM_UFNAME(unit, mem), soc_errmsg(r));
	    return CMD_FAIL;
	}
    }

    return CMD_OK;
}

#ifdef INCLUDE_MEM_SCAN

/*
 * Turn on/off memory scan thread
 */

char memscan_usage[] =
    "Parameters: [Interval=<USEC>] [Rate=<ENTRIES>] [on] [off]\n\t"
    "Interval specifies how often to run (0 to stop)\n\t"
    "Rate specifies the number of entries to process per interval\n\t"
    "Any memories that are cached will be scanned (see 'cache' command)\n";

cmd_result_t
mem_scan(int unit, args_t *a)
{
    parse_table_t	pt;
    int			running, rv;
    sal_usecs_t	interval = 0;
    int		rate = 0;
    char		*c;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if ((running = soc_mem_scan_running(unit, &rate, &interval)) < 0) {
	cli_out("soc_mem_scan_running %d: ERROR: %s\n",
                unit, soc_errmsg(running));
	return CMD_FAIL;
    }

    if (!running) {
	interval = 100000;
	rate = 64;
    }

    if (ARG_CNT(a) == 0) {
	cli_out("%s: %s on unit %d\n",
                ARG_CMD(a), running ? "Running" : "Not running", unit);
	cli_out("%s:   Interval: %d usec\n",
                ARG_CMD(a), interval);
	cli_out("%s:   Rate: %d\n",
                ARG_CMD(a), rate);

	return CMD_OK;
    }

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "Interval", PQ_INT | PQ_DFL, 0, &interval, 0);
    parse_table_add(&pt, "Rate", PQ_INT | PQ_DFL, 0, &rate, 0);

    if (parse_arg_eq(a, &pt) < 0) {
	cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
	parse_arg_eq_done(&pt);
	return CMD_FAIL;
    }

    parse_arg_eq_done(&pt);

    if ((c = ARG_GET(a)) != NULL) {
	if (sal_strcasecmp(c, "off") == 0) {
	    interval = 0;
	    rate = 0;
	} else if (!sal_strcasecmp(c, "on") == 0) {
	    return CMD_USAGE;
	}
    }

    if (interval == 0) {
	if ((rv = soc_mem_scan_stop(unit)) < 0) {
	    cli_out("soc_mem_scan_stop %d: ERROR: %s\n",
                    unit, soc_errmsg(rv));
	    return CMD_FAIL;
	}

	cli_out("%s: Stopped on unit %d\n", ARG_CMD(a), unit);
    } else {
	if ((rv = soc_mem_scan_start(unit, rate, interval)) < 0) {
	    cli_out("soc_mem_scan_start %d: ERROR: %s\n",
                    unit, soc_errmsg(rv));
	    return CMD_FAIL;
	}

	cli_out("%s: Started on unit %d\n", ARG_CMD(a), unit);
    }

    return CMD_OK;
}

#endif /* INCLUDE_MEM_SCAN */


#if defined(SER_TR_TEST_SUPPORT)
char cmd_esw_ser_usage[] =
    "Usages:\n"
#ifdef COMPILER_STRING_CONST_LIMIT
    "SER [options]\t"
#else
    " SER INFO   (NOT YET IMPLEMENTED)\n"
    " SER SHOW   (NOT YET IMPLEMENTED)\n"
    " SER LOG\n"
    " SER INJECT memory=<memID> [Index=<Index>] [Pipe=pipe_x|pipe_y]\n"
    " SER INJECT register=<regID> port=<portNum> [Index=<Index>]"
    " [Pipe=pipe_x|pipe_y](NOT YET IMPLEMENTED\n"
    "\tWith no optional parameters, Index will default to 0, and Pipe to"
    " pipe_x\n"
#endif
    ;

/*
 * Function:
 *      cmd_esw_ser
 * Purpose:
 *      Entry point and argument parser for SER diag shell utilities
 * Parameters:
 *      unit    - (IN) Device Number
 *      a       - (IN) A string of user input.  Only the first parameter will
 *                     be used by this function.
 * Returns:
 *      CMD_OK if injection succeeds, CMD_USAGE if there is a problem with user
 *      input, and CMD_FAIL if preconditions are not met or injection fails for
 *      a reason not related to incorrect input.
 */
cmd_result_t
esw_ser_inject(int unit, args_t *a) {
    cmd_result_t rv = CMD_USAGE;
    char *mem_name = "", *pipe_name = "";
    int index, tcam_mask;
    uint32  flags = 0;
    soc_block_t block = MEM_BLOCK_ANY;
    soc_mem_t mem;
    soc_pipe_select_t pipe;
    parse_table_t pt;
    parse_table_init(unit, &pt);
    parse_table_add(&pt, "Memory", PQ_STRING,
                    SOC_MEM_NAME(unit,INVALIDm), &(mem_name), NULL);
    parse_table_add(&pt, "Index", PQ_INT,
                    (void *)0, &index, NULL);
    parse_table_add(&pt, "Pipe", PQ_STRING,
                    "pipe_x", &(pipe_name), NULL);
    parse_table_add(&pt, "TCAM", PQ_BOOL,
                    FALSE, &(tcam_mask), NULL);
    if (parse_default_fill(&pt) < 0) {
        cli_out("Invalid default fill:\n");
        return rv;
    } else if(parse_arg_eq(a, &pt) < 0) {
        cli_out("%s: Invalid option %s\n", ARG_CMD(a), ARG_CUR(a));
        return rv;
    }
    if (!(parse_memory_name(unit,&(mem),mem_name,&block,0) >=0)) {
        cli_out("Invalid memory selected.\n");
        return rv;
    }
    if (!sal_strcasecmp(pipe_name, "pipe_x")) {
        pipe = SOC_PIPE_SELECT_X_COMMON;
    } else if (!sal_strcasecmp(pipe_name, "pipe_y")) {
        pipe = SOC_PIPE_SELECT_Y_COMMON;
    } else {
        cli_out("Invalid pipe selected. Valid entries are:"
                " 'pipe_x' 'pipe_y\n");
        return rv;
    }
    if (!SOC_SUCCESS(soc_ser_inject_support(unit, mem, pipe))){
        if (soc_ser_inject_support(unit,mem,pipe) == SOC_E_FAIL) {
            cli_out("Injection failed because miscellaneous"
                    " initializations have not yet been performed.\n"
                    "Type 'init misc' to do so and try again.\n");
        } else {
            cli_out("The selected memory: %s is valid, but SER correction "
                    "for it is not currently supported.\n", mem_name);
        }
        rv = CMD_FAIL;
        return rv;
    }
    if (index < 0) {
        cli_out("Invalid index selected");
        return rv;
    }
    if(tcam_mask) {
        flags |= SOC_INJECT_ERROR_TCAM_FLAG;
    }
    if (soc_ser_inject_error(unit, flags, mem, pipe, block, index) == SOC_E_FAIL) {
        cli_out("Injection failed as the system is not initialized.\n");
        rv = CMD_FAIL;
    } else {
        cli_out("Error injected on %s at index %d %s\n", mem_name, index,
                pipe_name);
        rv = CMD_OK;
    }

    return rv;
}

/*
 * Function:
 *      cmd_esw_ser
 * Purpose:
 *      Entry point and argument parser for SER diag shell utilities
 * Parameters:
 *      unit    - (IN) Device Number
 *      a       - (IN) A string of user input.  Only the first parameter will
 *                     be used by this function.
 * Returns:
 *      CMD_OK if the command succeeds, CMD_USAGE if there is a problem with the
 *       input from the user.  Other returns  of type cmd_result_t are possible
 *       depending on the implementation of each utility.
 */
cmd_result_t
cmd_esw_ser(int unit, args_t *a){
    char* arg1;
    cmd_result_t rv = CMD_USAGE;
    arg1 = ARG_GET(a);
    if (arg1 != NULL && !sal_strcasecmp(arg1, "inject")) {
        rv = esw_ser_inject(unit, a);
    } else if (arg1 != NULL && !sal_strcasecmp(arg1, "info")) {
        cli_out("Info option not yet implemented\n");
        rv = CMD_NOTIMPL;
    } else if (arg1 != NULL && !sal_strcasecmp(arg1, "show")) {
        cli_out("Show option not yet implemented\n");
        rv = CMD_NOTIMPL;
    } else if (arg1 != NULL && !sal_strcasecmp(arg1, "log")) {
        rv = soc_ser_log_print_all(unit);
    } else {
        cli_out("Invalid mode selected.\n");
    }
    return rv;
}

#endif /*defined(SER_TR_TEST_SUPPORT)*/

void 
mem_watch_cb(int unit, soc_mem_t mem, uint32 flags, int copyno, int index_min, 
             int index_max, void *data_buffer, void *user_data)
{
    int i;
    
    if (INVALIDm == mem) {
        cli_out("Invalid memory....\n");
        return;
    }
    if (index_max < index_min) {
        cli_out("Wrong indexes ....\n");
        return;
    }
    if (NULL == data_buffer) {
        cli_out("Buffer is NULL .... \n");
        return;
    }
    for (i = 0; i <= (index_max - index_min); i++) {
        cli_out("Unit = %d, mem = %s (%d), copyno = %d index = %d, Entry - ", 
                unit, SOC_MEM_NAME(unit, mem), mem, copyno, i + index_min);
        soc_mem_entry_dump(unit,mem, (char *)data_buffer + i);
    }

    cli_out("\n");

    return;
}

cmd_result_t
mem_watch(int unit, args_t *a)
{
    soc_mem_t       mem;
    char		    *c, *memname;
    int             copyno;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }
    if (ARG_CNT(a) == 0) {
        return CMD_USAGE;
    }
    if (NULL == (c = ARG_GET(a))) {
        return CMD_USAGE;
    }

    /* Deal with VIEW:MEMORY if applicable */
    memname = sal_strstr(c, ":");
    if (memname != NULL) {
        memname++;
    } else {
        memname = c;
    }

    if (parse_memory_name(unit, &mem, memname, &copyno, 0) < 0) {
        cli_out("ERROR: unknown table \"%s\"\n",c);
        return CMD_FAIL;
    }

    if (NULL == (c = ARG_GET(a))) {
        return CMD_USAGE;
    }

    if (sal_strcasecmp(c, "off") == 0) {
        /* unregister call back */
        soc_mem_snoop_unregister(unit, mem);
    } else if (sal_strcasecmp(c, "read") == 0) {
        /* register callback with read flag */
       soc_mem_snoop_register(unit,mem, SOC_MEM_SNOOP_READ, mem_watch_cb, NULL);
    } else if (sal_strcasecmp(c, "write") == 0) {
        /* register callback with write flag */
      soc_mem_snoop_register(unit,mem, SOC_MEM_SNOOP_WRITE, mem_watch_cb, NULL); 
    } else {
        return CMD_USAGE;
    }

    return CMD_OK;
}
