/*
 * $Id: symtab.c,v 1.72 Broadcom SDK $
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
 * Register mnemonic address parsing routines.
 * See description for parse_symbolic_reference.
 * This module is used by command line shells.
 */

#include <shared/bsl.h>

#include <sal/core/libc.h>
#include <shared/bsl.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/register.h>
#include <soc/util.h>
#include <appl/diag/system.h>
#include <appl/diag/dport.h>
#if defined(BCM_KATANA2_SUPPORT)
#include <soc/katana2.h>
#endif
#if defined(BCM_GREYHOUND_SUPPORT)
#include <soc/greyhound.h>
#endif
/*
 * Symbol hash table entry with buckets represented by linked list.
 */

typedef struct symtab_entry_t {
    soc_reg_t             esw_reg;
    soc_reg_t             robo_reg;
    char                  *name;
    struct symtab_entry_t *next;
} symtab_entry_t;

static symtab_entry_t     *symtab_table;

/*
 * Statically allocated table with pointers
 * to globally allocated registers table.
 */

static symtab_entry_t     *hashtable[NUM_SOC_REG];

/*
 * Case-independent hash routine, where we store a symbol table
 * entry (front end) in the hash table. If we
 * hit on that entry, then we chain it at the
 * same base memory address.
 */

static int
hash_func(char *s)
{
    uint32 h = 0, c;

    while ((c = *s++) != 0)
    h = (h << 3) ^ h ^ (h >> 7) ^ (islower(c) ? sal_toupper(c) : (c));
    return (h % NUM_SOC_REG);
}

/*
 * Initialize register symbol table. Hash all of the values in
 * the registers file into an address table we can lookup by
 * string mnemonic.
 */

void
_add_symbol(char *name, soc_reg_t reg, int ent, int *chained, int isRobo)
{
    int key;
    symtab_entry_t *table_entry;

    key = hash_func(name);

    /* Chain entry */
    table_entry = &symtab_table[ent];
    table_entry->name = name;
    if (isRobo) {
        table_entry->robo_reg = reg;
    } else {
        table_entry->esw_reg = reg;
    }
    
    table_entry->next = hashtable[key];
    if (hashtable[key]) {
        (*chained)++;
    }
    hashtable[key] = table_entry;
}


/*
 * Lookup a register in the symbol table.
 * Returns register number from matching entry
 * There may be both a register name and an alias that are
 * identical (AGE_TIMER, for example).  If one is a valid
 * register for this unit and another is invalid, the
 * valid register is returned.
 *
 * NOTE: hash_func is case-independent, so lookup_register is also.
 */

static symtab_entry_t*
lookup_register(int unit, char *regname)
{
    symtab_entry_t *loc, *floc;

    floc = NULL;
    for (loc = hashtable[hash_func(regname)]; loc; loc = loc->next) {
        if(!sal_strcasecmp(loc->name, regname)) {
            if (floc == NULL) {        /* save first match */
                floc = loc;
            }
            if (SOC_IS_ROBO(unit)) {
#if defined(BCM_ROBO_SUPPORT)
                if (SOC_REG_IS_VALID(unit, loc->robo_reg)) {
                    return loc;
                }
#endif
            } else {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT)
                if (SOC_REG_IS_VALID(unit, loc->esw_reg)) {
                    return loc;
                }
#endif
            }
        }
    }

    return (floc != NULL) ? floc : NULL;
}


void
init_symtab()
{
    soc_reg_t      reg;
    int            c, d, lin, emp, lon, len, ent;
    symtab_entry_t *te;

    if (symtab_table != NULL) {
        return;
    }

    /* count how many table entries to allocate for ESW */
    ent = 0;
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT)  || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    for (reg = 0; reg < NUM_SOC_REG; reg++) {
        ent++;
#if !defined(SOC_NO_ALIAS)    
        if (soc_reg_alias[reg] && *soc_reg_alias[reg]) {
            ent++;
        }
#endif /* !defined(SOC_NO_ALIAS) */
    }
#endif
#ifdef BCM_ROBO_SUPPORT
    /* count how many table entries to allocate for ROBO */
    for (reg = 0; reg < NUM_SOC_ROBO_REG; reg++) {
        ent++;
#if !defined(SOC_NO_ALIAS)    
        if (soc_robo_reg_alias[reg] && *soc_robo_reg_alias[reg]) {
            ent++;
        }
#endif /* !defined(SOC_NO_ALIAS) */
    }
#endif
    symtab_table = sal_alloc(ent * sizeof(symtab_entry_t), "symtab_table");
    if (NULL == symtab_table) {
        return; /* how to signal failure? */
    }

    ent = 0;
    d = 1;
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    /* Load the hashtable with each reserved word */
    /* ESW */
    for (reg = 0; reg < NUM_SOC_REG; reg++) {
#if !defined(SOC_NO_NAMES)
        _add_symbol(soc_reg_name[reg], reg, ent, &d, 0);
        ent += 1;
#endif /* !SOC_NO_NAMES */
#if !defined(SOC_NO_ALIAS)    
        if (soc_reg_alias[reg] && *soc_reg_alias[reg]) {
            _add_symbol(soc_reg_alias[reg], reg, ent, &d, 0);
            ent += 1;
        }
#endif /* !defined(SOC_NO_ALIAS) */
    }
#endif    
#ifdef BCM_ROBO_SUPPORT
    /* ROBO */
    for (reg = 0; reg < NUM_SOC_ROBO_REG; reg++) {
#if !defined(SOC_NO_NAMES)
        _add_symbol(soc_robo_reg_name[reg], reg, ent, &d, 1);
        ent += 1;
#endif /* !SOC_NO_NAMES */
#if !defined(SOC_NO_ALIAS)    
        if (soc_robo_reg_alias[reg] && *soc_robo_reg_alias[reg]) {
            _add_symbol(soc_robo_reg_alias[reg], reg, ent, &d, 1);
            ent += 1;
        }
#endif /* !defined(SOC_NO_ALIAS) */
    }
#endif

    c = ent + 1;
    lin = d-c > c-d ? d-c : c-d;
    emp = lon = 0;

    /* count number of empties and longest chain */
    for (reg = 0; reg < NUM_SOC_REG; reg++) {
        te = hashtable[reg];
        if (te == NULL) {
            emp += 1;
            continue;
        }
        len = 1;
        while (te->next != NULL) {
            len += 1;
            te = te->next;
        }
        if (len > lon) {
            lon = len;
        }
    }

    /*
     * this actually happens too early to print out
     * a printf here shows:
     * symtab: init 1028 regs, 1119 symbols, 674 linear, 445 chained
     *         345 empty, 5 longest chain
     */
    LOG_INFO(BSL_LS_APPL_SYMTAB,
             (BSL_META("symtab: init %d regs, %d symbols, %d linear, %d chained,\n"
                       "\t\t%d empty, %d longest chain\n"),
              NUM_SOC_REG, c, lin, d, emp, lon));

#ifdef BCM_ROBO_SUPPORT
    for (reg = 0; reg < NUM_SOC_ROBO_REG; reg++) {
        te = hashtable[reg];
        if (te == NULL) {
            emp += 1;
            continue;
        }
        len = 1;
        while (te->next != NULL) {
            len += 1;
            te = te->next;
        }
        if (len > lon) {
            lon = len;
        }
    }

/*
     * this actually happens too early to print out
     * a printf here shows:
     * symtab: init 1028 regs, 1119 symbols, 674 linear, 445 chained
     *          345 empty, 5 longest chain
 */
    LOG_INFO(BSL_LS_APPL_SYMTAB,
             (BSL_META("symtab: init %d regs, %d symbols, %d linear, %d chained,\n"
                       "\t\t%d empty, %d longest chain\n"),
              NUM_SOC_ROBO_REG, c, lin, d, emp, lon));
#endif /* BCM_ROBO_SUPPORT */
}


static void
_handle_array_refs(int unit, soc_regaddrlist_t *alist,
                   soc_reg_t reg, int block,
                   soc_cos_t cos, soc_port_t port,
                   int minidx, int maxidx)
{
    int idx;
    soc_regaddrinfo_t *a;
    uint32 flags = SOC_REG_INFO(unit, reg).flags;

    bsl_log_add(BSL_APPL_SYMTAB, bslSeverityNormal, bslSinkIgnore, unit,
                "symtab: lookup reg %d blk %d cos %d port %d index %d:%d\n",
                reg, block, cos, port, minidx, maxidx);

    /* Ignore invalid ports, don't check if the port is really a instance id */
    if ((!(port & SOC_REG_ADDR_INSTANCE_MASK)) && port != REG_PORT_ANY && !SOC_PORT_VALID(unit, port) &&
        (SOC_REG_INFO(unit, reg).regtype != soc_ppportreg)) {
        return;
    }

    for (idx = minidx; idx <= maxidx; idx++) {
        if (flags & SOC_REG_FLAG_NO_DGNL) {
            /* Skip diagonals.  */
            if (idx == port) {
                continue;
            }
        }
        a = &alist->ainfo[alist->count++];
        a->valid = 1;
        a->idx = idx;
        a->reg = reg;
        a->block = block;
        a->port = port;
        a->field = INVALIDf;
        a->cos = cos;

        /* calculate address based on reg type */
        switch (SOC_REG_INFO(unit, reg).regtype) {
        case soc_cosreg:
            idx = cos;
            /* FALLTHROUGH */
        case soc_cpureg:
        case soc_mcsreg:
        case soc_iprocreg:
        case soc_portreg:
        case soc_ppportreg:
        case soc_genreg:
        case soc_phy_reg:
            if (SOC_IS_ROBO(unit)){
#ifdef BCM_ROBO_SUPPORT      
            a->addr = (DRV_SERVICES(unit)->reg_addr)(unit, reg, port, idx);
#endif
            } else {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#if defined(BCM_PETRAB_SUPPORT) 
                if (SOC_IS_PETRAB(unit)){
                    
                    a->addr = SOC_REG_INFO(unit, reg).offset;
                } else
#endif
                {
                    if (!soc_feature(unit, soc_feature_new_sbus_format)) {
                        a->addr = soc_reg_addr(unit, reg, port, idx);
                    } else {
                        int blk;
                        uint8 at;
                        a->addr = soc_reg_addr_get(unit, reg, port, idx, FALSE, &blk, &at);
                    }
                }
#endif
            }    
            break;
        default:
            assert(0);
            a->addr = 0;
            break;
        }

        /*
         * Check if this register is actually implemented
         * in HW for the specified port/cos.
         */
        if (reg_mask_subset(unit, a, NULL)) {
            alist->count--;
        }
    }
}

/* Check string starting at name for [ and ] */
static int
_parse_array(char *name, char **out1, char **out2)
{
    char *idx1, *idx2, *tmpchr;
    int rv = 0;

    if ((idx1 = sal_strchr(name, '[')) != 0) {
        rv = 1;
        *idx1++ = 0;
        if ((tmpchr = sal_strchr(idx1, ']')) == 0) {
            LOG_WARN(BSL_LS_APPL_SYMTAB,
                     (BSL_META("Could not parse index in %s.\n"), name));
        } else {
            *tmpchr = 0;
        }
        if ((idx2 = sal_strchr(idx1, '-')) != 0) {
            *idx2++ = 0;
        }
    } else if ((idx1 = sal_strchr(name, '(')) != 0) {
        rv = 1;
        *idx1++ = 0;
        if ((tmpchr = sal_strchr(idx1, ')')) == 0) {
            LOG_WARN(BSL_LS_APPL_SYMTAB,
                     (BSL_META("Could not parse index in %s.\n"), name));
        } else {
            *tmpchr = 0;
        }
        if ((idx2 = sal_strchr(idx1, '-')) != 0) {
            *idx2++ = 0;
        }
    } else {
        idx2 = 0;
    }

    *out1 = idx1;
    *out2 = idx2;

    return rv;
}

/*
 * Parse a symbolic register reference.
 *
 * Fills in soc_regaddrlist_t, which is an array of soc_regaddrinfo_t.
 * Returns 0 on success, -1 on syntax error.
 *
 * Understands the following:
 *
 * regname            -Port register, all ports
 * regname.PBMP            -Port register, selected ports
 *
 * regname            -Generic register, all PICs or MMU
 * regname.BLK_RANGE        -Generic register, selected PICs
 *
 * regname            -COS register, all COS values, all PICs or MMU
 * regname.COS_RANGE        -COS register, selected COS's, all PICs
 * regname..BLK_RANGE        -COS register, all COS values, selected PICs
 * regname.COS_RANGE.BLK_RANGE    -COS register, selected COS's, selected PICs
 *
 * In addition, [index range] may be specified.  It can be specified for
 * register, but for non-array registers, it must be 0.  All array indices
 * are 0-based.
 *
 * If a leading '$' is present, it is ignored.
 *
 * See util.c:parse_pbmp() for PBMP format
 * See util.c:parse_cos_range() for COS_RANGE format
 * See util.c:parse_block_range() for BLK_RANGE format
 *
 * Register addressing
 *    soc_reg_addr(unit, port, index)
 *    - port is REG_PORT_ANY for non-pic regs
 *    - index is the cos number for COS regs
 *
 * Prints a warning and returns -1 if unit does not support the
 * register.
 */

int
parse_symbolic_reference(int unit, soc_regaddrlist_t *alist, char *ref)
{
    char rname[80], *range1, *range2, *idx1, *idx2;
    soc_port_t port, phy_port, dport;
    soc_cos_t cos_min, cos_max, cos;
    int blk_min, blk_max, blk = 0;
    soc_block_t portblktype;
    soc_block_types_t regblktype, bmask = NULL;
    soc_reg_t reg;
    pbmp_t bm;
    soc_reg_info_t *reginfo;
    int maxidx, minidx, idxfound = 0;
    char pfmt[SOC_PBMP_FMT_LEN];
    symtab_entry_t  *et;
    int idx = 0, match = 0, max_block=0, port_num_blktype;
#if defined(BCM_ROBO_SUPPORT)
    soc_regaddrinfo_t *a = NULL;
#endif
    
    LOG_INFO(BSL_LS_APPL_SYMTAB,
             (BSL_META_U(unit,
                         "parsing: %s\n"), ref));

    if (*ref == '$') {
        ref++;
    }

    sal_strncpy(rname, ref, 79);
    rname[79] = 0;

    if ((range1 = sal_strchr(rname, '.')) != 0) {
        *range1++ = '\0';
        idxfound = _parse_array(range1, &idx1, &idx2);

        if ((range2 = sal_strchr(range1, '.')) != 0) {
            *range2++ = '\0';
            if (*range2 == '\0') {
                range2 = 0;
            }
        }
        if (*range1 == '\0') {
            range1 = 0;
        }
    } else {
        range2 = 0;
    }

    if (!idxfound) {
        idxfound = _parse_array(rname, &idx1, &idx2);
    }

    et = lookup_register(unit, rname);
    if (NULL == et) {
        return -1;
    }

    if (SOC_IS_ROBO(unit)) {
        reg = et->robo_reg;
    } else {
        reg = et->esw_reg;
    }
    

    
    if (!SOC_REG_IS_VALID(unit, reg)) {
        LOG_WARN(BSL_LS_APPL_SYMTAB,
                 (BSL_META_U(unit,
                             "Register %s %d is not valid for chip %s\n"),
                             et->name, reg, SOC_UNIT_NAME(unit)));
        return -1;
    }

    reginfo = &SOC_REG_INFO(unit, reg);
    if (!SOC_REG_IS_ENABLED(unit, reg)) {
        if (SOC_IS_ROBO(unit)){
#ifdef BCM_ROBO_SUPPORT
            LOG_WARN(BSL_LS_APPL_SYMTAB,
                     (BSL_META_U(unit,
                                 "Register %s is not enabled for this configuration\n"),
                                 SOC_ROBO_REG_NAME(unit, reg)));
#endif
        } else {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT)
            LOG_WARN(BSL_LS_APPL_SYMTAB,
                     (BSL_META_U(unit,
                                 "Register %s is not enabled for this configuration\n"),
                                 SOC_REG_NAME(unit, reg)));
#endif
        }
        return -1;
    }
    regblktype = reginfo->block;

    if (SOC_IS_ROBO(unit)) {
#ifdef BCM_ROBO_SUPPORT
        max_block = _soc_robo_max_blocks_per_entry_get();
#endif
    } else {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT)  || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
        /* max_block = _soc_max_blocks_per_entry_get(); */
        /* _soc_max_blocks_per_entry_get() + 1 is also enough but for 
           future safe  purpose*/
        max_block = SOC_MAX_NUM_BLKS;
#endif
    }

    bmask = sal_alloc(sizeof(uint32) * max_block, 
                      "parse_bmask");
    if (NULL == bmask) {
        return -1;
    }
    sal_memset(bmask, 0, sizeof(uint32) * max_block);

    if (diag_parse_range(idx1, idx2, &minidx, &maxidx,
                         SOC_REG_ARRAY(unit, reg) ? 0 : -1,
                         SOC_REG_ARRAY(unit, reg) ? SOC_REG_NUMELS(unit,reg)-1 : -1) < 0) {
        LOG_WARN(BSL_LS_APPL_SYMTAB,
                 (BSL_META_U(unit,
                             "Register %s: bad index specification\n"), ref));
        goto _return_error;
    }

    bsl_log_start(BSL_APPL_SYMTAB, bslSeverityNormal, unit,
                  "symtab: index %d:%d type %d regblktype [ ",
                  minidx, maxidx, reginfo->regtype);
    while (regblktype[idx] != SOC_BLOCK_TYPE_INVALID) {
        bsl_log_add(BSL_APPL_SYMTAB, bslSeverityNormal, bslSinkIgnore,
                    unit, "%s ",
                    soc_block_name_lookup_ext(regblktype[idx], unit));
        idx++;
    }
    bsl_log_end(BSL_APPL_SYMTAB, bslSeverityNormal, bslSinkIgnore,
                unit, "]\n");

    alist->count = 0;

    switch (reginfo->regtype) {
#if defined(BCM_ROBO_SUPPORT)
    case soc_phy_reg:
        if (range2) {
            goto _return_error;
        }
        /* parse port spec in range1 */
        if (! range1) {
            if (SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_PORT) ||
                SOC_BLOCK_IS(regblktype, SOC_BLK_INTER) ||
                SOC_BLOCK_IS(regblktype, SOC_BLK_EXTER)) {
                match = 1;
            }
            /*if (regblktype & (SOC_BLK_PORT|SOC_BLK_INTER|SOC_BLK_EXTER)) {}*/
            if (match) { 
                SOC_PBMP_CLEAR(bm);
                SOC_BLOCKS_ITER(unit, blk, regblktype) {
                    SOC_PBMP_OR(bm, SOC_BLOCK_BITMAP(unit, blk));
                }
            }else {
                SOC_PBMP_ASSIGN(bm, PBMP_ALL(unit));
            }
        } else if (parse_pbmp(unit, range1, &bm) < 0) {
            goto _return_error;
        }
        LOG_VERBOSE(BSL_LS_APPL_SYMTAB,
                    (BSL_META_U(unit,
                                "symtab: port bitmap %s\n"),
                                SOC_PBMP_FMT(bm, pfmt)));

        bsl_log_start(BSL_APPL_SYMTAB, bslSeverityNormal, unit, "");
        /* Coverity
         * DPORT_SOC_PBMP_ITER checks that dport is valid.
         */
        /* coverity[overrun-local] */
        DPORT_SOC_PBMP_ITER(unit, bm, dport, port) {
            if (SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_PORT)) {
                if (soc_feature(unit, soc_feature_logical_port_num)) {
                    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
                } else {
                    phy_port = port;
                }
                blk = SOC_PORT_BLOCK(unit, phy_port);
                if (!(SOC_BLOCK_IS_TYPE(unit, blk, regblktype))) {
                    goto _return_error;
                }
            } 
            _handle_array_refs(unit, alist, reg,
                               regblktype[0],
                               -1,        /* cos */
                               port,
                               minidx, maxidx);
        }
        bsl_log_end(BSL_APPL_SYMTAB, bslSeverityNormal, bslSinkIgnore,
                    unit, "");
        break;
#endif /* ROBO ONLY */
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    case soc_cpureg:
    case soc_mcsreg:
    case soc_iprocreg:
        if (range2) {
            goto _return_error;
        }

        /* parse block spec in range1 */
        if (parse_block_range(unit, regblktype, range1,
                              &blk_min, &blk_max, bmask) < 0) {
            goto _return_error;
        }
        idx = 0;
        while (bmask[idx] != 0) {
            if (SOC_BLOCK_IN_LIST(regblktype, bmask[idx])) {
                break; 
            }
            idx++;
        }
        if ((idx > 0) && (match == 0)) {
            goto _return_error;
        }
        if (blk_min < 0 || blk_max > SOC_INFO(unit).block_num) {
            goto _return_error;
        }

        blk = CMIC_BLOCK(unit);
        if (blk_min > blk || blk_max < blk) {
            goto _return_error;
        }

        bsl_log_start(BSL_APPL_SYMTAB, bslSeverityNormal, unit, "");
        _handle_array_refs(unit, alist, reg,
                           blk,
                           -1,            /* cos */
                           REG_PORT_ANY,    /* port */
                           minidx, maxidx);
        bsl_log_end(BSL_APPL_SYMTAB, bslSeverityNormal, bslSinkIgnore,
                    unit, "");
        break;
#endif /* ESW ONLY */
    case soc_ppportreg:
        /* parse port spec in range1 */
        if (! range1) {
            SOC_PBMP_ASSIGN(bm, PBMP_PP_ALL(unit));
        } else if (parse_pp_pbmp(unit, range1, &bm) < 0) {
            goto _return_error;
        }
        LOG_VERBOSE(BSL_LS_APPL_SYMTAB,
                    (BSL_META_U(unit,
                                "symtab: ppport bitmap %s\n"),
                                SOC_PBMP_FMT(bm, pfmt)));
        switch (regblktype[0]) {
        case SOC_BLK_IPIPE:
            blk = IPIPE_BLOCK(unit);
            break;
        case SOC_BLK_EPIPE:
            blk = EPIPE_BLOCK(unit);
            break;
        default:
            goto _return_error;
        }
        bsl_log_start(BSL_APPL_SYMTAB, bslSeverityNormal, unit, "");
        SOC_PBMP_ITER(bm, port) {
            _handle_array_refs(unit, alist, reg,
                               blk,
                               -1,      /* cos */
                               port,
                               minidx, maxidx);
        }
        bsl_log_end(BSL_APPL_SYMTAB, bslSeverityNormal, bslSinkIgnore,
                    unit, "");
        break;
    case soc_portreg:
        if (range2) {
            goto _return_error;
        }

#ifdef BCM_SBX_CMIC_SUPPORT
        if (SOC_IS_SIRIUS(unit) || SOC_IS_CALADAN3(unit)) {
            portblktype = SOC_BLK_SBX_PORT;
        } else 
#endif
        {
            portblktype = SOC_BLK_PORT;
        }
        port_num_blktype = SOC_DRIVER(unit)->port_num_blktype > 1 ?
            SOC_DRIVER(unit)->port_num_blktype : 1;

        /* parse port spec in range1 */
        if (! range1) {
            if (SOC_BLOCK_IN_LIST(regblktype, portblktype)) {
                SOC_PBMP_CLEAR(bm);
                SOC_BLOCKS_ITER(unit, blk, regblktype) {
                    SOC_PBMP_OR(bm, SOC_BLOCK_BITMAP(unit, blk));
                }
            } else {
                SOC_PBMP_ASSIGN(bm, PBMP_ALL(unit));
                if ((soc_feature(unit, soc_feature_linkphy_coe) &&
                     SOC_INFO(unit).linkphy_enabled) ||
                    (soc_feature(unit, soc_feature_subtag_coe) &&
                    SOC_INFO(unit).subtag_enabled)) {
                    SOC_PBMP_CLEAR(bm);
                    SOC_PBMP_ASSIGN(bm, PBMP_PORT_ALL(unit));
                    SOC_PBMP_PORT_ADD(bm, CMIC_PORT(unit));
                    SOC_PBMP_PORT_ADD(bm, LB_PORT(unit));
                }
                SOC_PBMP_OR(bm, PBMP_MMU(unit));
            } 
#ifdef BCM_GREYHOUND_SUPPORT
            if (SOC_IS_GREYHOUND(unit) && (regblktype[0] == SOC_BLK_XLPORT)) {
                int rv;
                rv = soc_greyhound_pgw_reg_blk_index_get(unit, reg, REG_PORT_ANY, 
                    &bm, NULL, NULL, 0);
                if ((rv < 0) && (rv != SOC_E_NOT_FOUND)){
                    goto _return_error;
                }
            }
#endif
        } else if (parse_pbmp(unit, range1, &bm) < 0) {
            goto _return_error;
        }
        LOG_VERBOSE(BSL_LS_APPL_SYMTAB,
                    (BSL_META_U(unit,
                                "symtab: port bitmap %s\n"),
                                SOC_PBMP_FMT(bm, pfmt)));

        bsl_log_start(BSL_APPL_SYMTAB, bslSeverityNormal, unit, "");
        /* Coverity
         * DPORT_SOC_PBMP_ITER checks that dport is valid.
         */
        /* coverity[overrun-local] */
        DPORT_SOC_PBMP_ITER(unit, bm, dport, port) {
            if (SOC_BLOCK_IN_LIST(regblktype, portblktype)) {
                if (soc_feature(unit, soc_feature_logical_port_num)) {
                    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
                } else {
                    phy_port = port;
                }
                for (idx = 0; idx < port_num_blktype; idx++) {
#ifdef BCM_KATANA2_SUPPORT
                    /* Override port blocks with Linkphy Blocks.. */
                    if(SOC_IS_KATANA2(unit) && (regblktype[0] == SOC_BLK_TXLP) ) {
                        soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                                                             SOC_BLK_TXLP, &blk, NULL);
                        break;
                    } else if(SOC_IS_KATANA2(unit) &&
                              (regblktype[0] == SOC_BLK_RXLP)) {
                        soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                                                             SOC_BLK_RXLP, &blk, NULL);
                        break;
                    }
#endif
#ifdef BCM_GREYHOUND_SUPPORT
                    if (SOC_IS_GREYHOUND(unit) && (regblktype[0] == SOC_BLK_XLPORT)) {
                        if (soc_greyhound_pgw_reg_blk_index_get(unit, reg, port, 
                            NULL, &blk, NULL, 0) > 0){
                            break;
                        }
                    }
#endif
                    blk = SOC_PORT_IDX_BLOCK(unit, phy_port, idx);
                    if (blk < 0) {
                        break;
                    }
                    if (SOC_BLOCK_IN_LIST(regblktype,
                                          SOC_BLOCK_TYPE(unit, blk))) {
                        break;
                    }
                }
#ifdef BCM_GREYHOUND_SUPPORT
                if (SOC_IS_GREYHOUND(unit) && (regblktype[0] == SOC_BLK_XLPORT)) {
                    if (soc_greyhound_pgw_reg_blk_index_get(unit, reg, port, 
                        NULL, &blk, NULL, 1) > 0){
                        continue;
                    }
                }
#endif                
                if (blk < 0 || idx == port_num_blktype) {
                    goto _return_error;
                }
            } else {
                /*  This big switch statement will go away soon */
#ifdef  BCM_SIRIUS_SUPPORT
                if (SOC_IS_SIRIUS(unit)) {
                    switch (regblktype[0]) {
                    case SOC_BLK_CMIC:
                        blk = CMIC_BLOCK(unit);
                        break;
                    case SOC_BLK_BP:
                        blk = BP_BLOCK(unit);
                        break;
                    case SOC_BLK_CS:
                        blk = CS_BLOCK(unit);
                        break;
                    case SOC_BLK_EB:
                        blk = EB_BLOCK(unit);
                        break;
                    case SOC_BLK_EP:
                        blk = EP_BLOCK(unit);
                        break;
                    case SOC_BLK_ES:
                        blk = ES_BLOCK(unit);
                        break;
                    case SOC_BLK_FD:
                        blk = FD_BLOCK(unit);
                        break;
                    case SOC_BLK_FF:
                        blk = FF_BLOCK(unit);
                        break;
                    case SOC_BLK_FR:
                        blk = FR_BLOCK(unit);
                        break;
                    case SOC_BLK_TX:
                        blk = TX_BLOCK(unit);
                        break;
                    case SOC_BLK_QMA:
                        blk = QMA_BLOCK(unit);
                        break;
                    case SOC_BLK_QMB:
                        blk = QMB_BLOCK(unit);
                        break;
                    case SOC_BLK_QMC:
                        blk = QMC_BLOCK(unit);
                        break;
                    case SOC_BLK_QSA:
                        blk = QSA_BLOCK(unit);
                        break;
                    case SOC_BLK_QSB:
                        blk = QSB_BLOCK(unit);
                        break;
                    case SOC_BLK_RB:
                        blk = RB_BLOCK(unit);
                        break;
                    case SOC_BLK_SC_TOP:
                        blk = SC_TOP_BLOCK(unit);
                        break;
                    case SOC_BLK_SF_TOP:
                        blk = SF_TOP_BLOCK(unit);
                        break;
                    case SOC_BLK_TS:
                        blk = TS_BLOCK(unit);
                        break;
                    case SOC_BLK_OTPC:
                        blk = OTPC_BLOCK(unit);
                        break;
                    default:
                        /* search for a matching block... */
                        SOC_BLOCKS_ITER(unit, blk, regblktype) {
                        break;
                        }
                    }
                } 
                else 
#endif
                {
                    int instance;
                    switch (regblktype[0]) {
                    case SOC_BLK_IPIPE:
                        blk = IPIPE_BLOCK(unit);
                        break;
                    case SOC_BLK_IPIPE_HI:
                        blk = IPIPE_HI_BLOCK(unit);
                        if (!PBMP_MEMBER(SOC_BLOCK_BITMAP(unit, blk), port)) {
                            continue;
                        }
                        break;
                    case SOC_BLK_EPIPE:
                        blk = EPIPE_BLOCK(unit);
                        break;
                    case SOC_BLK_EPIPE_HI:
                        blk = EPIPE_HI_BLOCK(unit);
                        if (!PBMP_MEMBER(SOC_BLOCK_BITMAP(unit, blk), port)) {
                            continue;
                        }
                        break;
                    case SOC_BLK_IGR:
                        blk = IGR_BLOCK(unit);
                        break;
                    case SOC_BLK_EGR:
                        blk = EGR_BLOCK(unit);
                        break;
                    case SOC_BLK_BSE: /* Aliased with IL */
                        if (SOC_IS_SHADOW(unit)) {
                            if (port == 9) {
                                blk = IL0_BLOCK(unit);
                            } else if (port == 13) {
                                blk = IL1_BLOCK(unit);
                            }
                        } else {
                            blk = BSE_BLOCK(unit);
                        }
                        break;
                    case SOC_BLK_CSE: /* Aliased with MS_ISEC */
                        if (SOC_IS_SHADOW(unit)) {
                            if (port >=1 && port <= 4) {
                                blk = MS_ISEC0_BLOCK(unit);
                            } else if (port >= 5 && port <= 8) {
                                blk = MS_ISEC1_BLOCK(unit);
                            }
                        } else {
                            blk = CSE_BLOCK(unit);
                        }
                        break;
                    case SOC_BLK_HSE: /* Aliased with MS_ESEC */
                        if (SOC_IS_SHADOW(unit)) {
                            if (port >= 1 && port <= 4) {
                                blk = MS_ESEC0_BLOCK(unit);
                            } else if (port >= 5 && port <= 8) {
                                blk = MS_ESEC1_BLOCK(unit);
                            }
                        } else {
                            blk = HSE_BLOCK(unit);
                        }
                        break;
                    case SOC_BLK_SYS: /* Aliased with CW */
                        if (SOC_IS_SHADOW(unit)) {
                            blk = CW_BLOCK(unit);
                        }
                        break;
                    case SOC_BLK_BSAFE:
                        blk = BSAFE_BLOCK(unit);
                        break;
                    case SOC_BLK_ESM:
                        blk = ESM_BLOCK(unit);
                        break;
                    case SOC_BLK_ARL:
                        blk = ARL_BLOCK(unit);
                        break;
                    case SOC_BLK_MMU:
                        blk = MMU_BLOCK(unit);
                        break;
                    case SOC_BLK_MCU:
                        blk = MCU_BLOCK(unit);
                        break;
                    case SOC_BLK_PGW_CL:
                        instance = SOC_INFO(unit).port_group[port];
                        blk = PGW_CL_BLOCK(unit, instance);
                        break;
                    default:
                        /* search for a matching block... */
                        SOC_BLOCKS_ITER(unit, blk, regblktype) {
                            break;
                        }
                    }
                }
            }
            _handle_array_refs(unit, alist, reg,
                               blk,
                               -1,        /* cos */
                               port,
                               minidx, maxidx);
        }
        bsl_log_end(BSL_APPL_SYMTAB, bslSeverityNormal, bslSinkIgnore,
                    unit, "");
        break;
    case soc_cosreg:
        /* parse cos spec in range1 */
        if (parse_cos_range(unit, range1, &cos_min, &cos_max) < 0) {
            goto _return_error;
        }
        /* parse block spec in range2 */
        if (parse_block_range(unit, regblktype, range2,
                              &blk_min, &blk_max, bmask) < 0) {
            goto _return_error;
        }
        idx = 0;
        while (bmask[idx] != 0) {
            if (SOC_BLOCK_IN_LIST(regblktype, bmask[idx])) {
                break; 
            }
            idx++;
        }
        if ((idx > 0) && (match == 0)) {
            goto _return_error;
        }
        if (blk_min < 0 || blk_max > SOC_INFO(unit).block_num) {
            goto _return_error;
        }

        bsl_log_start(BSL_APPL_SYMTAB, bslSeverityNormal, unit, "");
        for (cos = cos_min; cos <= cos_max; cos++) {
            SOC_BLOCKS_ITER(unit, blk, regblktype) {
                if (blk < blk_min || blk > blk_max) {
                    continue;
                }
                port = SOC_BLOCK_PORT(unit, blk);
                _handle_array_refs(unit, alist, reg,
                                   blk,
                                   cos,
                                   port,
                                   -1, -1);
            }
        }
        bsl_log_end(BSL_APPL_SYMTAB, bslSeverityNormal, bslSinkIgnore,
                    unit, "");
        break;

    case soc_genreg:
        if (range2) {
            goto _return_error;
        }
        if (SOC_IS_ROBO(unit)) {
#if defined(BCM_ROBO_SUPPORT)            
            if (SOC_BLOCK_IS(regblktype, SOC_BLK_SYS)) {
                bsl_log_start(BSL_APPL_SYMTAB, bslSeverityNormal, unit, "");
                _handle_array_refs(unit, alist, reg,
                                   blk,
                                   -1,        /* cos */
                                   REG_PORT_ANY,
                                   minidx, maxidx);
                bsl_log_end(BSL_APPL_SYMTAB, bslSeverityNormal, bslSinkIgnore,
                            unit, "");
            } else {
                if (! range1) {
                    if (SOC_BLOCK_IN_LIST(regblktype, SOC_BLK_PORT) ||
                        SOC_BLOCK_IS(regblktype, SOC_BLK_EXTER)) {
                        SOC_PBMP_CLEAR(bm);
                        SOC_BLOCKS_ITER(unit, blk, regblktype) {
                            SOC_PBMP_OR(bm, SOC_BLOCK_BITMAP(unit, blk));
                        }
                    } else {
                        SOC_PBMP_ASSIGN(bm, PBMP_ALL(unit));
                    }
                } else if (parse_pbmp(unit, range1, &bm) < 0) {
                    goto _return_error;
                }
                LOG_VERBOSE(BSL_LS_APPL_SYMTAB,
                            (BSL_META_U(unit,
                                        "symtab: port bitmap %s\n"),
                                        SOC_PBMP_FMT(bm, pfmt)));

                bsl_log_start(BSL_APPL_SYMTAB, bslSeverityNormal, unit, "");
                /* Coverity
                 * DPORT_SOC_PBMP_ITER checks that dport is valid.
                 */
                /* coverity[overrun-local] */
                DPORT_SOC_PBMP_ITER(unit, bm, dport, port) {
                    _handle_array_refs(unit, alist, reg,
                                       blk,
                                       -1,        /* cos */
                                       port,
                                       -1, -1);
                }
                bsl_log_end(BSL_APPL_SYMTAB, bslSeverityNormal, bslSinkIgnore,
                            unit, "");
            }
#endif /* ROBO */
        } else {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
            /* parse block spec in range1 */
            if (parse_block_range(unit, regblktype, range1,
                  &blk_min, &blk_max, bmask) < 0) {
                goto _return_error;
            }
            idx = 0;
            while (bmask[idx] != 0) {
                if (SOC_BLOCK_IN_LIST(regblktype, bmask[idx])) {
                    break; 
                }
                idx++;
            }
            if ((idx > 0) && (match == 0)) {
                goto _return_error;
            }
            if (blk_min < 0 || blk_max > SOC_INFO(unit).block_num) {
                goto _return_error;
            }

            bsl_log_start(BSL_APPL_SYMTAB, bslSeverityNormal, unit, "");
            SOC_BLOCKS_ITER(unit, blk, regblktype) {
            if (blk < blk_min || blk > blk_max) {
                continue;
            }
            switch (regblktype[0]) {
            case SOC_BLK_PGW_CL:
                port = SOC_REG_ADDR_INSTANCE_MASK | SOC_BLOCK_NUMBER(unit, blk);
                break;
            default:
                if (SOC_IS_CALADAN3(unit) && (regblktype[0] == SOC_BLK_IL)) {
                    port = SOC_REG_ADDR_INSTANCE_MASK | SOC_BLOCK_NUMBER(unit, blk);
                } else {
                    port = SOC_BLOCK_PORT(unit, blk);
                }
                break;
            }
#ifdef BCM_GREYHOUND_SUPPORT
            if (SOC_IS_GREYHOUND(unit) && ((regblktype[0] == SOC_BLK_XLPORT) ||
                (regblktype[0] == SOC_BLK_GXPORT))) {
                if (soc_greyhound_pgw_reg_blk_index_get(unit, reg, port, 
                                            NULL, NULL, NULL, 0) == SOC_E_NOT_FOUND){
                    /* port register not belong to pgw_gx and pgw_xl */
                    if (!PBMP_MEMBER(SOC_BLOCK_BITMAP(unit, blk), port)){
                        /* check the valid blk bitmap*/
                        continue;
                    }
                } else if (regblktype[0] == SOC_BLK_XLPORT){
                    if (soc_greyhound_pgw_reg_blk_index_get(unit, reg, port, 
                        NULL, &blk, NULL, 1) > 0){
                        continue;
                    } 
                 }
            }
#endif
            _handle_array_refs(unit, alist, reg,
                               blk,
                               -1,        /* cos */
                               port,
                               minidx, maxidx);
            }
            bsl_log_end(BSL_APPL_SYMTAB, bslSeverityNormal, bslSinkIgnore,
                        unit, "");
#endif            
        }
        break;

#if defined(BCM_ROBO_SUPPORT)
    case soc_spi_reg:
        if (range2) {
            goto _return_error;
        }

        if (!range1) {
            for (idx = 0; idx < reginfo->numels; idx++) {
                a = &alist->ainfo[alist->count++];
                a->valid = 1;
                a->idx = idx;
                a->reg = reg;
                a->port = -1;
                a->field = INVALID_Rf;
                a->cos = -1;

                /* calculate address based on reg type */
                a->addr =  (DRV_SERVICES(unit)->reg_addr)(unit, reg, 0, idx);
            }
        } else {
            idx = *range1 - 48;

            a = &alist->ainfo[alist->count++];
            a->valid = 1;
            a->idx = idx;
            a->reg = reg;
            a->port = -1;
            a->field = INVALID_Rf;
            a->cos = -1;

            /* calculate address based on reg type */
            a->addr =  (DRV_SERVICES(unit)->reg_addr)(unit, reg, 0, idx);
        }
        break;
#endif /* ROBO ONLY */
    default:
    assert(0);
        break;
    }

    if (bsl_check(bslLayerAppl, bslSourceSymtab, bslSeverityNormal, unit)) {
        int i;

        bsl_log_start(BSL_APPL_SHELL, bslSeverityNormal, unit, "");
        for (i = 0; i < alist->count; i++) {
            char buf[80];
            sal_memset(buf, 0x0, 80);
            if (SOC_IS_ROBO(unit)) {
#ifdef BCM_ROBO_SUPPORT     
                soc_robo_reg_sprint_addr(unit,  buf, &alist->ainfo[i]);
#endif
            } else {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(BCM_DPP_SUPPORT)
                soc_reg_sprint_addr(unit,  buf, &alist->ainfo[i]);
#endif
            }
            bsl_log_add(BSL_APPL_SHELL, bslSeverityNormal, bslSinkIgnore,
                        unit, "symtab:  %d: %s %x\n",
                        i, buf, alist->ainfo[i].addr);
        }
        bsl_log_end(BSL_APPL_SHELL, bslSeverityNormal, bslSinkIgnore,
                    unit, "");
    }

    if (alist->count == 0) {
        goto _return_error;
    }
    if (bmask) {
        sal_free(bmask);
    }
    return 0;

_return_error:
    sal_free(bmask);
    return -1;
}
