/* 
 * $Id: reg.c 1.39 Broadcom SDK $
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
 * socdiag register commands
 */

#include <sal/core/libc.h>
#include <soc/counter.h>
#include <sal/appl/pci.h>
#include <soc/debug.h>
#ifdef BCM_IPROC_SUPPORT
#include <soc/iproc.h>
#endif

#include <appl/diag/system.h>
#include <appl/diag/sysconf.h>
#include <ibde.h>

/* 
 * Utility routine to concatenate the first argument ("first"), with
 * the remaining arguments, with commas separating them.
 */

static void
collect_comma_args(args_t *a, char *valstr, char *first)
{
    char           *s;

    sal_strcpy(valstr, first);

    while ((s = ARG_GET(a)) != 0) {
	sal_strcat(valstr, ",");
	sal_strcat(valstr, s);
    }
}

/* 
 * Utility routine to determine whether register type requires
 * block and access type information.
 */

static int 
reg_type_is_schan_reg(int reg_type)
{
    switch (reg_type) {
    case soc_schan_reg:
    case soc_genreg:
    case soc_portreg:
    case soc_ppportreg:
    case soc_cosreg:
        return 1;
    default:
        break;
    }
    return 0;
}

/* 
 * modify_reg_fields
 *
 *   Takes a soc_reg_t 'regno', pointer to current value 'val',
 *   and a string 'mod' containing a field replacement spec of the form
 *   "FIELD=value".   The string may contain more than one spec separated
 *   by commas.  Updates the field in the register value accordingly,
 *   leaving all other unmodified fields intact.  If the string CLEAR is
 *   present in the list, the current register value is zeroed.
 *   If mask is non-NULL, it receives a mask of all fields modified.
 *
 *   Examples with modreg:
 *        modreg fe_mac1 lback=1        (modifies only lback field)
 *        modreg config ip_cfg=2        (modifies only ip_cfg field)
 *        modreg config clear,ip_cfg=2,cpu_pri=4  (zeroes all other fields)
 *
 *   Note that if "clear" appears in the middle of the list, the
 *   values in the list before "clear" are ignored.
 *
 *   Returns CMD_FAIL on failure, CMD_OK on success.
 */

static int
modify_reg_fields(int unit, soc_reg_t regno,
		  uint64 *val, uint64 *mask /* out param */, char *mod)
{
    soc_field_info_t *fld;
    char           *fmod, *fval;
    char           *modstr;
    soc_reg_info_t *reg = &SOC_REG_INFO(unit, regno);
    uint64          fvalue;
    uint64          fldmask;
    uint64          tmask;

    if ((modstr = sal_alloc(ARGS_BUFFER, "modify_reg")) == NULL) {
        printk("modify_reg_fields: Out of memory\n");
        return CMD_FAIL;
    }

    sal_strncpy(modstr, mod, ARGS_BUFFER);/* Don't destroy input string */
    modstr[ARGS_BUFFER - 1] = 0;
    mod = modstr;

    if (mask) {
	COMPILER_64_ZERO(*mask);
    }

    while ((fmod = sal_strtok(mod, ",")) != 0) {
	mod = NULL;		       /* Pass strtok NULL next time */
	fval = sal_strchr(fmod, '=');
	if (fval) {		       /* Point fval to arg, NULL if none */
	    *fval++ = 0;	       /* Now fmod holds only field name. */
	}
	if (fmod[0] == 0) {
	    printk("Null field name\n");
            sal_free(modstr);
	    return CMD_FAIL;
	}
	if (!sal_strcasecmp(fmod, "clear")) {
	    COMPILER_64_ZERO(*val);
	    if (mask) {
		COMPILER_64_ALLONES(*mask);
	    }
	    continue;
	}
	for (fld = &reg->fields[0]; fld < &reg->fields[reg->nFields]; fld++) {
	    if (!sal_strcasecmp(fmod, SOC_FIELD_NAME(unit, fld->field))) {
		break;
	    }
	}
	if (fld == &reg->fields[reg->nFields]) {
	    printk("No such field \"%s\" in register \"%s\".\n",
		   fmod, SOC_REG_NAME(unit, regno));
            sal_free(modstr);
	    return CMD_FAIL;
	}
	if (!fval) {
	    printk("Missing %d-bit value to assign to \"%s\" "
		   "field \"%s\".\n",
		   fld->len, SOC_REG_NAME(unit, regno), SOC_FIELD_NAME(unit, fld->field));
            sal_free(modstr);
	    return CMD_FAIL;
	}
	fvalue = parse_uint64(fval);

	/* Check that field value fits in field */
	COMPILER_64_MASK_CREATE(tmask, fld->len, 0);
	COMPILER_64_NOT(tmask);
	COMPILER_64_AND(tmask, fvalue);

	if (!COMPILER_64_IS_ZERO(tmask)) {
	    printk("Value \"%s\" too large for %d-bit field \"%s\".\n",
		   fval, fld->len, SOC_FIELD_NAME(unit, fld->field));
            sal_free(modstr);
	    return CMD_FAIL;
	}

	if (reg->flags & SOC_REG_FLAG_64_BITS) {
	    soc_reg64_field_set(unit, regno, val, fld->field, fvalue);
	} else {
	    uint32          tmp;
            uint32 ftmp;
	    COMPILER_64_TO_32_LO(tmp, *val);
	    COMPILER_64_TO_32_LO(ftmp, fvalue);
	    soc_reg_field_set(unit, regno, &tmp, fld->field, ftmp);
	    COMPILER_64_SET(*val, 0, tmp);
	    COMPILER_64_SET(fvalue, 0, ftmp);
	}

	COMPILER_64_MASK_CREATE(fldmask, fld->len, fld->bp);
	if (mask) {
	    COMPILER_64_OR(*mask, fldmask);
	}
    }

    sal_free(modstr);
    return CMD_OK;
}

#define PRINT_COUNT(str, len, wrap, prefix) \
    if ((wrap > 0) && (len > wrap)) { \
        soc_cm_print("\n%s", prefix); \
        len = sal_strlen(prefix); \
    } \
    printk("%s", str); \
    len += sal_strlen(str)


/* 
 * Print a SOC internal register with fields broken out.
 */
void
reg_print(int unit, soc_regaddrinfo_t *ainfo, uint64 val, uint32 flags,
	  char *fld_sep, int wrap)
{
    soc_reg_info_t *reginfo = &SOC_REG_INFO(unit, ainfo->reg);
    int             f;
    uint64          val64, resval, resfld;
    char            buf[80];
    char            line_buf[256];
    int		    linelen = 0;
    int		    nprint;
    int32           addr32 = ainfo->addr;

    if (flags & REG_PRINT_HEX) {
	if (SOC_REG_IS_64(unit, ainfo->reg)) {
	    printk("%08x%08x\n",
		   COMPILER_64_HI(val),
		   COMPILER_64_LO(val));
	} else {
	    printk("%08x\n",
		   COMPILER_64_LO(val));
	}
	return;
    }

    if (flags & REG_PRINT_CHG) {
	SOC_REG_RST_VAL_GET(unit, ainfo->reg, resval);
	if (COMPILER_64_EQ(val, resval)) {	/* no changed fields */
	    return;
	}
    } else {
	COMPILER_64_ZERO(resval);
    }

    soc_reg_sprint_addr(unit, buf, ainfo);

    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        sal_sprintf(line_buf, "%s[%d][0x%x]=", buf, 
                    SOC_BLOCK_INFO(unit, ainfo->block).cmic,
                    addr32);
    } else {
        sal_sprintf(line_buf, "%s[0x%x]=", buf, addr32);
    }
    PRINT_COUNT(line_buf, linelen, wrap, "   ");

    format_uint64(line_buf, val);
    PRINT_COUNT(line_buf, linelen, -1, "");

    if (flags & REG_PRINT_RAW) {
	printk("\n");
	return;
    }

    PRINT_COUNT(": <", linelen, wrap, "   ");

    nprint = 0;
    for (f = reginfo->nFields - 1; f >= 0; f--) {
	soc_field_info_t *fld = &reginfo->fields[f];
	val64 = soc_reg64_field_get(unit, ainfo->reg, val, fld->field);
	if (flags & REG_PRINT_CHG) {
	    resfld = soc_reg64_field_get(unit, ainfo->reg, resval, fld->field);
	    if (COMPILER_64_EQ(val64, resfld)) {
		continue;
	    }
	}

	if (nprint > 0) {
	    sal_sprintf(line_buf, "%s", fld_sep);
            PRINT_COUNT(line_buf, linelen, -1, "");
	}
	sal_sprintf(line_buf, "%s=", SOC_FIELD_NAME(unit, fld->field));

        PRINT_COUNT(line_buf, linelen, wrap, "   ");
	format_uint64(line_buf, val64);

        PRINT_COUNT(line_buf, linelen, -1, "");
	nprint += 1;
    }

    printk(">\n");
}

/* 
 * Reads and displays all SOC registers specified by alist structure.
 */
int
reg_print_all(int unit, soc_regaddrlist_t *alist, uint32 flags)
{
    int             j;
    uint64          value;
    int             r, rv = 0;
    soc_regaddrinfo_t *ainfo;

    assert(alist);

    for (j = 0; j < alist->count; j++) {
	ainfo = &alist->ainfo[j];
	if ((r = soc_anyreg_read(unit, ainfo, &value)) < 0) {
	    char            buf[80];
	    soc_reg_sprint_addr(unit, buf, ainfo);
	    printk("ERROR: read from register %s failed: %s\n",
		   buf, soc_errmsg(r));
	    rv = -1;
	} else {
	    reg_print(unit, ainfo, value, flags, ",", 62);
	}
    }

    return rv;
}

/*
 * Register Types - for getreg and dump commands
 */

static regtype_entry_t regtypes[] = {
 { "PCIC",	soc_pci_cfg_reg,"PCI Configuration space" },
 { "PCIM",	soc_cpureg,	"PCI Memory space (CMIC)" },
 { "MCS",	soc_mcsreg,	"MicroController Subsystem" },
 { "IPROC",     soc_iprocreg,   "iProc Subsystem" },
 { "SOC",	soc_schan_reg,	"SOC internal registers" },
 { "SCHAN",	soc_schan_reg,	"SOC internal registers" },
 { "PHY",	soc_phy_reg,	"PHY registers via MII (phyID<<8|phyADDR)" },
 { "MW",	soc_hostmem_w,	"Host Memory 32-bit" },
 { "MH",	soc_hostmem_h,	"Host Memory 16-bit" },
 { "MB",	soc_hostmem_b,	"Host Memory 8-bit" },
 { "MEM",	soc_hostmem_w,	"Host Memory 32-bit" },	/* Backward compat */
};

#define regtypes_count	COUNTOF(regtypes)

regtype_entry_t *regtype_lookup_name(char* str)
{
    int i;

    for (i = 0; i < regtypes_count; i++) {
	if (!sal_strcasecmp(str,regtypes[i].name)) {
	    return &regtypes[i];
        }
    }

    return 0;
}

void regtype_print_all(void)
{
    int i;

    printk("Register types supported by setreg, getreg, and dump:\n");

    for (i = 0; i < regtypes_count; i++)
	printk("\t%-10s -%s\n", regtypes[i].name, regtypes[i].help);
}

/* 
 * Get a register by type.
 *
 * doprint:  Boolean.  If set, display data.
 */
int
_reg_get_by_type(int unit, sal_vaddr_t vaddr, soc_block_t block, int acc_type,
         soc_regtype_t regtype, uint64 *outval, uint32 flags)
{
    int             rv = CMD_OK;
    int             r;
    uint16          phy_rd_data;
    uint32          regaddr = (uint32)vaddr;
    soc_regaddrinfo_t ainfo;
    int		    is64 = FALSE;
    int               blk, cmic, schan, i;

    switch (regtype) {
    case soc_pci_cfg_reg:
        if (regaddr & 3) {
            printk("ERROR: PCI config addr must be multiple of 4\n");
            rv = CMD_FAIL;
        } else {
            COMPILER_64_SET(*outval, 0, bde->pci_conf_read(unit, regaddr));
        }
        break;
    case soc_cpureg:
        if (regaddr & 3) {
            printk("ERROR: PCI memory addr must be multiple of 4\n");
            rv = CMD_FAIL;
        } else {
            COMPILER_64_SET(*outval, 0, soc_pci_read(unit, regaddr));
        }
        break;
#ifdef BCM_CMICM_SUPPORT
    case soc_mcsreg:
        if (regaddr & 3) {
            printk("ERROR: MCS memory addr must be multiple of 4\n");
            rv = CMD_FAIL;
        } else {
            COMPILER_64_SET(*outval, 0, soc_pci_mcs_read(unit, regaddr));
        }
        break;
#endif
#ifdef BCM_IPROC_SUPPORT
    case soc_iprocreg:
        if (regaddr & 3) {
            printk("ERROR: iProc memory addr must be multiple of 4\n");
            rv = CMD_FAIL;
        } else {
            COMPILER_64_SET(*outval, 0, soc_cm_iproc_read(unit, regaddr));
        }
        break;
#endif
    case soc_schan_reg:
    case soc_genreg:
    case soc_portreg:
	case soc_cosreg:

        /* Defensive check to ensure that the supplied 'block' is valid */
        blk = -1;
        for (i = 0; SOC_BLOCK_INFO(unit, i).type >= 0; i++) {
            if (regtype == soc_schan_reg) {
                schan = SOC_BLOCK_INFO(unit, i).schan;
                if(schan == block) {
                    blk = i;
                }
            } else {
                cmic = SOC_BLOCK_INFO(unit, i).cmic;
                if(cmic == block) {
                    blk = i;
                }
            }
        }

        if(blk == -1) {
            printk("ERROR: Invalid block specified \n");
            rv = CMD_FAIL;
        }

        if(rv == CMD_OK) {
            soc_regaddrinfo_extended_get (unit, &ainfo, block, acc_type, regaddr);
            /* Defensive check to ensure that the register extracted is valid */
            if (ainfo.reg == -1) {
                printk("ERROR: Unable to resolve register with supplied data \n");
                rv = CMD_FAIL;
            } else if (ainfo.reg >= 0) {
                is64 = SOC_REG_IS_64(unit, ainfo.reg);
            }

            if(rv == CMD_OK) {
                r = soc_anyreg_read(unit, &ainfo, outval);
                if (r < 0) {
                    printk("ERROR: soc_reg32_read failed: %s\n", soc_errmsg(r));
                    rv = CMD_FAIL;
                }
            }
        }

        break;

    case soc_hostmem_w:
        COMPILER_64_SET(*outval, 0, *((uint32 *)INT_TO_PTR(vaddr)));
        break;

    case soc_hostmem_h:
        COMPILER_64_SET(*outval, 0, *((uint16 *)INT_TO_PTR(vaddr)));
        break;

    case soc_hostmem_b:
        COMPILER_64_SET(*outval, 0, *((uint8 *)INT_TO_PTR(vaddr)));
        break;

    case soc_phy_reg:
	/* Leave for MII debug reads */
	if ((r = soc_miim_read(unit,
			       (uint8) (regaddr >> 8 & 0xff),	/* Phy ID */
			       (uint8) (regaddr & 0xff),	/* Phy addr */
			       &phy_rd_data)) < 0) {
	    printk("ERROR: soc_miim_read failed: %s\n", soc_errmsg(r));
	    rv = CMD_FAIL;
	} else {
	    COMPILER_64_SET(*outval, 0, (uint32) phy_rd_data);
	}
	break;

    default:
	assert(0);
	rv = CMD_FAIL;
	break;
    }

    if ((rv == CMD_OK) && (flags & REG_PRINT_DO_PRINT)) {
	if (flags & REG_PRINT_HEX) {
	    if (is64) {
		printk("%08x%08x\n",
		       COMPILER_64_HI(*outval),
		       COMPILER_64_LO(*outval));
	    } else {
		printk("%08x\n",
		       COMPILER_64_LO(*outval));
	    }
	} else {
	    char buf[80];

	    format_uint64(buf, *outval);

	    printk("%s[%p] = %s\n",
		   soc_regtypenames[regtype], INT_TO_PTR(vaddr), buf);
	}
    }

    return rv;
}

/* 
 * Set a register by type.  For SOC registers, is64 is used to
 * indicate if this is a 64 bit register.  Otherwise, is64 is
 * ignored.
 *
 */
int
_reg_set_by_type(int unit, sal_vaddr_t vaddr, soc_block_t block, int acc_type,
         soc_regtype_t regtype, uint64 regval)
{
    int             rv = CMD_OK, r;
    uint32          regaddr = (uint32)vaddr;
    uint32          val32;
    soc_regaddrinfo_t ainfo;
    int               blk, cmic, schan, i;

    COMPILER_64_TO_32_LO(val32, regval);

    switch (regtype) {
    case soc_pci_cfg_reg:
	bde->pci_conf_write(unit, regaddr, val32);
	break;

    case soc_cpureg:
	soc_pci_write(unit, regaddr, val32);
	break;

#ifdef BCM_CMICM_SUPPORT
    case soc_mcsreg:
        soc_pci_mcs_write(unit, regaddr, val32);
        break;
#endif
#ifdef BCM_IPROC_SUPPORT
    case soc_iprocreg:
        soc_cm_iproc_write(unit, regaddr, val32);
        break;
#endif
    case soc_schan_reg:
    case soc_genreg:
    case soc_portreg:
	case soc_cosreg:

        /* Defensive check to ensure that the supplied 'block' is valid */
        blk = -1;
        for (i = 0; SOC_BLOCK_INFO(unit, i).type >= 0; i++) {
            if (regtype == soc_schan_reg) {
                schan = SOC_BLOCK_INFO(unit, i).schan;
                if(schan == block) {
                    blk = i;
                }
            } else {
                cmic = SOC_BLOCK_INFO(unit, i).cmic;
                if(cmic == block) {
                    blk = i;
                }
            }
        }

        if(blk == -1) {
            printk("ERROR: Invalid block specified \n");
            rv = CMD_FAIL;
        }

        if(rv == CMD_OK) {
            soc_regaddrinfo_extended_get(unit, &ainfo, block, acc_type, regaddr);

            /* Defensive check to ensure that the register extracted is valid */
            if (ainfo.reg == -1) {
                printk("ERROR: Unable to resolve register with supplied data \n");
                rv = CMD_FAIL;
            }
            else {
                r = soc_anyreg_write(unit, &ainfo, regval);
                if (r < 0) {
                    printk("ERROR: write reg failed: %s\n", soc_errmsg(r));
                    rv = CMD_FAIL;
                }
           }
       }

        break;

    case soc_hostmem_w:
        *((uint32 *)INT_TO_PTR(vaddr)) = val32;
        break;

    case soc_hostmem_h:
        *((uint16 *)INT_TO_PTR(vaddr)) = val32;
        break;

    case soc_hostmem_b:
        *((uint8 *)INT_TO_PTR(vaddr)) = val32;
        break;

    case soc_phy_reg:
	/* Leave for MII debug writes */
	if ((r = soc_miim_write(unit,
				(uint8) (regaddr >> 8 & 0xff),	/* Phy ID */
				(uint8) (regaddr & 0xff),	/* Phy addr */
				(uint16) val32)) < 0) {
	    printk("ERROR: write miim failed: %s\n", soc_errmsg(r));
	    rv = CMD_FAIL;
	}
	break;

    default:
	assert(0);
	rv = CMD_FAIL;
	break;
    }

    return rv;
}

/* 
 * Gets a memory value or register from the SOC.
 * Syntax: getreg [<regtype>] <offset|symbol>
 */

cmd_result_t
cmd_esw_reg_get(int unit, args_t *a)
{
    uint64          regval;
    uint32          regaddr = 0;
    sal_vaddr_t     vaddr;
    int             rv = CMD_OK, acc_type = 0;
    regtype_entry_t *rt;
    soc_regaddrlist_t alist;
    char           *name, *block, *access_type;
    uint32          flags = REG_PRINT_DO_PRINT;
    soc_block_t blk_num = 0;

    if (0 == sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    /* 
     * If first arg is a register type, take it and use the next argument
     * as the name or address, otherwise default to register type "soc."
     */
    name = ARG_GET(a);

    for (;;) {
        if (name != NULL && !sal_strcasecmp(name, "raw")) {
            flags |= REG_PRINT_RAW;
            name = ARG_GET(a);
        } else if (name != NULL && !sal_strcasecmp(name, "hex")) {
            flags |= REG_PRINT_HEX;
            name = ARG_GET(a);
        } else if (name != NULL && !sal_strcasecmp(name, "chg")) {
            flags |= REG_PRINT_CHG;
            name = ARG_GET(a);
        } else {
            break;
        }
    }

    if (name == NULL) {
        return CMD_USAGE;
    }

    if ((rt = regtype_lookup_name(name)) != 0) {
        if ((name = ARG_GET(a)) == 0) {
            return CMD_USAGE;
        }
    } else {
        rt = regtype_lookup_name("schan");
    }
    if (0 == rt) {
        printk("Unknown register.\n");
        return (CMD_FAIL);
    }

    if (soc_regaddrlist_alloc(&alist) < 0) {
        printk("Could not allocate address list.  Memory error.\n");
        return CMD_FAIL;
    }

    if (isint(name)) {
        /* Numerical address given */
        vaddr = parse_address(name);
        if (!reg_type_is_schan_reg(rt->type)) {
            blk_num = 0;
            acc_type = 0;
        } else if (soc_feature(unit, soc_feature_new_sbus_format)) {
            if ((block = ARG_GET(a)) == 0) {
                printk("ERROR: This format of Getreg requires block-id to be specified.\n");
                return CMD_FAIL;
            } else {
                if (!isint(block)) {
                    printk("ERROR: block-id is not integer.\n");
                    return CMD_FAIL;
                }
                blk_num = parse_address(block);
            }
            if (soc_feature(unit, soc_feature_two_ingress_pipes)) {
                if ((access_type = ARG_GET(a)) == 0) {
                    printk("ERROR: Access-type not specified.\n");
                    return CMD_FAIL;
                } else {
                    if (!isint(access_type)) {
                        printk("ERROR: Access-type is not integer.\n");
                        return CMD_FAIL;
                    }
                    acc_type = parse_address(access_type);
                }
            }
        }
        rv = _reg_get_by_type(unit, vaddr, blk_num, acc_type, rt->type, &regval, flags);
    } else {
        if (*name == '$') {
            name++;
        }

        /* Symbolic name given, print all or some values ... */
        if (rt->type == soc_cpureg) {
            if (parse_cmic_regname(unit, name, &regaddr) < 0) {
                printk("ERROR: bad argument to GETREG PCIM: %s\n", name);
                rv = CMD_FAIL;
            } else {
                rv = _reg_get_by_type(unit, regaddr, -1, -1,
                                      rt->type, &regval, flags);
            }
        } else if (parse_symbolic_reference(unit, &alist, name) < 0) {
            printk("Syntax error parsing \"%s\"\n", name);
            rv = CMD_FAIL;
        } else if (reg_print_all(unit, &alist, flags) < 0) {
            rv = CMD_FAIL;
        }
    }
    soc_regaddrlist_free(&alist);
    return rv;
}

/* 
 * Auxilliary routine to handle setreg and modreg.
 *      mod should be 0 for setreg, which takes either a value or a
 *              list of <field>=<value>, and in the latter case, sets all
 *              non-specified fields to zero.
 *      mod should be 1 for modreg, which does a read-modify-write of
 *              the register and permits value to be specified by a list
 *              of <field>=<value>[,...] only.
 */

STATIC cmd_result_t
do_reg_set(int unit, args_t *a, int mod)
{
    uint64          regval;
    uint32          regaddr = 0;
    sal_vaddr_t     vaddr;
    int             rv = CMD_OK, i, acc_type = 0;
    regtype_entry_t *rt;
    soc_regaddrlist_t alist = {0, NULL};
    soc_regaddrinfo_t *ainfo;
    char           *name, *block, *access_type;
    char           *s, *valstr = NULL;
    soc_block_t blk_num = 0;

    COMPILER_64_ALLONES(regval);

    if (0 == sh_check_attached(ARG_CMD(a), unit)) {
        return  CMD_FAIL;
    }

    if ((name = ARG_GET(a)) == 0) {
        return  CMD_USAGE;
    }

    /* 
     * If first arg is an access type, take it and use the next argument
     * as the name, otherwise use default access type.
     * modreg command does not allow this and assumes soc.
     */

    if ((0 == mod) && (rt = regtype_lookup_name(name)) != 0) {
        if ((name = ARG_GET(a)) == 0) {
            return CMD_USAGE;
        }
    } else {
        rt = regtype_lookup_name("schan");
        if (0 == rt) {
            return CMD_FAIL;
        }
    }

    if (isint(name)) {
        if (!reg_type_is_schan_reg(rt->type)) {
            blk_num = 0;
            acc_type = 0;
        } else if (soc_feature(unit, soc_feature_new_sbus_format)) {
            if ((block = ARG_GET(a)) == 0) {
                printk("ERROR: This format of Setreg requires block-id to be specified.\n");
                return CMD_FAIL;
            } else {
                if (!isint(block)) {
                    printk("ERROR: block-id is not integer.\n");
                    return CMD_FAIL;
                }
                blk_num = parse_address(block);
            }
            if (soc_feature(unit, soc_feature_two_ingress_pipes)) {
                if ((access_type = ARG_GET(a)) == 0) {
                    printk("ERROR: Access-type not specified.\n");
                    return CMD_FAIL;
                } else {
                    if (!isint(access_type)) {
                        printk("ERROR: Access-type is not integer.\n");
                        return CMD_FAIL;
                    }
                    acc_type = parse_address(access_type);
                }
            }
        }
    }
    /* 
     * Parse the value field.  If there are more than one, string them
     * together with commas separating them (to make field-based setreg
     * inputs more convenient).
     */

    if ((s = ARG_GET(a)) == 0) {
        printk("Syntax error: missing value\n");
        return  CMD_USAGE;
    } 
 
    if ((valstr = sal_alloc(ARGS_BUFFER, "reg_set")) == NULL) {
        printk("do_reg_set: Out of memory\n");
        return CMD_FAIL;
    }

    collect_comma_args(a, valstr, s);

    if (mod && isint(valstr)) {
        sal_free(valstr);
        return CMD_USAGE;
    }


    if (soc_regaddrlist_alloc(&alist) < 0) {
        printk("Could not allocate address list.  Memory error.\n");
        sal_free(valstr);
        return CMD_FAIL;
    }

    if (!mod && isint(name)) {
        /* Numerical address given */
        vaddr = parse_address(name);
        regval = parse_uint64(valstr);
        rv = _reg_set_by_type(unit, vaddr, blk_num, acc_type, rt->type, regval);
    } else {
        /* Symbolic name given, set all or some values ... */
        if (*name == '$') {
            name++;
        }
        if (rt->type == soc_cpureg) {
            if (parse_cmic_regname(unit, name, &regaddr) < 0) {
                printk("ERROR: bad argument to SETREG PCIM: %s\n", name);
                rv = CMD_FAIL;
            } else {
                regval = parse_uint64(valstr);
                rv = _reg_set_by_type(unit, regaddr, -1, -1, rt->type, regval);
            }
        } else if (parse_symbolic_reference(unit, &alist, name) < 0) {
            printk("Syntax error parsing \"%s\"\n", name);
            rv = CMD_FAIL;
        } else {
            if (isint(valstr)) { /* valstr is numeric */
                regval = parse_uint64(valstr);
            }

            for (i = 0; i < alist.count; i++) {
                ainfo = &alist.ainfo[i];

                if (!isint(valstr)) { /* valstr is Field/value pairs */
                    /* 
                     * valstr must be a field replacement spec.
                     * In modreg mode, read the current register value,
                     * and modify it.  In setreg mode,
                     * assume a starting value of zero and modify it.
                     */
                    if (mod) { /* read-modify-write */
                        rv = soc_anyreg_read(unit, ainfo, &regval);
                        if (rv < 0) {
                            char buf[80];
                            soc_reg_sprint_addr(unit, buf, ainfo);
                            printk("ERROR: read from register %s failed: %s\n",
                                   buf, soc_errmsg(rv));
                        }
                    } else {
                        COMPILER_64_ZERO(regval);
                    }
                    if (rv == CMD_OK) {
                        rv = modify_reg_fields(unit, ainfo->reg, &regval, NULL,
                                               valstr);
                    }
                }
                if (rv == CMD_OK) {
                    rv = soc_anyreg_write(unit, ainfo, regval);
                    if (rv < 0) {
                        char buf[80];
                        soc_reg_sprint_addr(unit, buf, ainfo);
                        printk("ERROR: write to register %s failed: %s\n",
                               buf, soc_errmsg(rv));
                    }
                }
                if (rv != CMD_OK) {
                    break;
                }
            }
        }
    }
    sal_free(valstr);
    soc_regaddrlist_free(&alist);
    return rv;
}

/* 
 * Sets a memory value or register on the SOC.
 * Syntax 1: setreg [<regtype>] <offset|symbol> <value>
 * Syntax 2: setreg [<regtype>] <offset|symbol> <field>=<value>[,...]
 */
cmd_result_t
cmd_esw_reg_set(int unit, args_t *a)
{
    return do_reg_set(unit, a, 0);
}

/* 
 * Read/modify/write a memory value or register on the SOC.
 * Syntax: modreg [<regtype>] <offset|symbol> <field>=<value>[,...]
 */
cmd_result_t
cmd_esw_reg_mod(int unit, args_t * a)
{
    return do_reg_set(unit, a, 1);
}

char regcmp_usage[] =
#ifdef COMPILER_STRING_CONST_LIMIT
    "Usage: regcmp [args...]\n"
#else
    "Parameters: [-q] [<LOOPS>] <REG> [<VALUE>] [==|!=] <VALUE>\n\t"
    "If the optional <VALUE> on the left is given, starts by writing\n\t"
    "<VALUE> to <REG>.   Then loops up to <LOOPS> times reading <REG>,\n\t"
    "comparing if it is equal (==) or not equal (!=) to the <VALUE> on\n\t"
    "the right, and stopping if the compare fails.  If -q is specified, no\n\t"
    "output is displayed.  <LOOPS> defaults to 1 and may be * for\n\t"
    "indefinite.  If the compare fails, the command result is 1.  If the\n\t"
    "loops expire (compares succeed), the result is 0.  The result may be\n\t"
    "tested in an IF statement.  Also, each <VALUE> can consist of\n\t"
    "<FIELD>=<VALUE>[,...] to compare individual fields.  Examples:\n\t"
    "    if \"regcmp -q 1 rpkt.fe0 == 0\" \"echo RPKT IS ZERO\"\n\t"
    "    if \"regcmp -q config.e0 == fil_en=1\" \"echo FILTERING ENABLED\"\n"
#endif
    ;

cmd_result_t
reg_cmp(int unit, args_t *a)
{
    soc_reg_t       reg;
    soc_regaddrlist_t alist;
    soc_regaddrinfo_t *ainfo;
    char           *name = NULL, *count_str;
    char           *read_str, *write_str, *op_str;
    uint64          read_val, read_mask, write_val, reg_val, tmp_val;
    int             equal, i, quiet, loop;
    int             are_equal;
    int rv = CMD_OK;

    if (!(count_str = ARG_GET(a))) {
        return (CMD_USAGE);
    }

    if (!sal_strcmp(count_str, "-q")) {
        quiet = TRUE;
        if (!(count_str = ARG_GET(a))) {
            return (CMD_USAGE);
        }
    } else {
        quiet = FALSE;
    }

    if (!sal_strcmp(count_str, "*")) {
        loop = -1;
    } else if (isint(count_str)) {
        if ((loop = parse_integer(count_str)) < 0) {
            printk("%s: Invalid loop count: %s\n", ARG_CMD(a), count_str);
            return (CMD_FAIL);
        }
    } else {
        name = count_str;
        loop = 1;
    }

    if (!name && !(name = ARG_GET(a))) {
        return (CMD_USAGE);
    }

    write_str = ARG_GET(a);
    op_str = ARG_GET(a);
    read_str = ARG_GET(a);

    /* Must have WRITE ==|!= READ or ==|!= READ */

    if (!read_str) {
        read_str = op_str;
        op_str = write_str;
        write_str = NULL;
    } else if (ARG_CNT(a)) {
        return (CMD_USAGE);
    }

    if (!read_str || !op_str) {
        return (CMD_USAGE);
    }

    if (!sal_strcmp(op_str, "==")) {
        equal = TRUE;
    } else if (!sal_strcmp(op_str, "!=")) {
        equal = FALSE;
    } else {
        return (CMD_USAGE);
    }

    if (*name == '$') {
        name++;
    }

    /* Symbolic name given, set all or some values ... */

    if (soc_regaddrlist_alloc(&alist) < 0) {
        printk("Could not allocate address list.  Memory error.\n");
        return CMD_FAIL;
    }

    if (parse_symbolic_reference(unit, &alist, name) < 0) {
        printk("%s: Syntax error parsing \"%s\"\n", ARG_CMD(a), name);
        soc_regaddrlist_free(&alist);
        return (CMD_FAIL);
    }

    ainfo = &alist.ainfo[0];
    reg = ainfo->reg;

    COMPILER_64_ALLONES(read_mask);

    if (isint(read_str)) {
        read_val = parse_uint64(read_str);
    } else {
        COMPILER_64_ZERO(read_val);
        if (modify_reg_fields(unit, ainfo->reg, &read_val,
                              &read_mask, read_str) < 0) {
            printk("%s: Syntax error: %s\n", ARG_CMD(a), read_str);
            soc_regaddrlist_free(&alist);
            return (CMD_USAGE);
        }
    }

    if (write_str) {
        if (isint(write_str)) {
            write_val = parse_uint64(write_str);
        } else {
            COMPILER_64_ZERO(write_val);
            if (modify_reg_fields(unit, ainfo->reg, &write_val,
                                  (uint64 *) 0, write_str) < 0) {
                printk("%s: Syntax error: %s\n", ARG_CMD(a), write_str);
                soc_regaddrlist_free(&alist);
                return (CMD_USAGE);
            }
        }
    }

    do {
        for (i = 0; i < alist.count; i++) {
            int             r;

            ainfo = &alist.ainfo[i];
            if (write_str) {
                if ((r = soc_anyreg_write(unit, ainfo, write_val)) < 0) {
                    printk("%s: ERROR: Write register %s.%d failed: %s\n",
                           ARG_CMD(a), SOC_REG_NAME(unit, reg), i, soc_errmsg(r));
                    soc_regaddrlist_free(&alist);
                    return (CMD_FAIL);
                }
            }

            if ((r = soc_anyreg_read(unit, ainfo, &reg_val)) < 0) {
                printk("%s: ERROR: Read register %s.%d failed: %s\n",
                       ARG_CMD(a), SOC_REG_NAME(unit, reg), i, soc_errmsg(r));
                soc_regaddrlist_free(&alist);
                return (CMD_FAIL);
            }

            tmp_val = read_val;
            COMPILER_64_AND(tmp_val, read_mask);
            COMPILER_64_XOR(tmp_val, reg_val);
            are_equal = COMPILER_64_IS_ZERO(tmp_val);
            if ((!are_equal && equal) || (are_equal && !equal)) {
                if (!quiet) {
                    char buf1[80], buf2[80];
                    printk("%s: %s.%d ", ARG_CMD(a), SOC_REG_NAME(unit, reg), i);
                    format_uint64(buf1, read_val);
                    format_uint64(buf2, reg_val);
                    printk("%s %s %s\n", buf1, equal ? "!=" : "==", buf2);
                }
                soc_regaddrlist_free(&alist);
                return (1);
            }
        }

        if (loop > 0) {
            loop--;
        }
    } while (loop);

    soc_regaddrlist_free(&alist);
    return rv;
}

/* 
 * Lists registers containing a specific pattern
 *
 * If use_reset is true, ignores val and uses reset default value.
 */

static void
do_reg_list(int unit, soc_regaddrinfo_t *ainfo, int use_reset, uint64 regval)
{
    soc_reg_t       reg = ainfo->reg;
    soc_reg_info_t *reginfo = &SOC_REG_INFO(unit, reg);
    soc_field_info_t *fld;
    int             f;
    uint32          flags;
    uint64          mask, fldval, rval, rmsk;
    char            buf[80];
    char            rval_str[20], rmsk_str[20], dval_str[20];
    int		    i, copies, disabled;

    if (!SOC_REG_IS_VALID(unit, reg)) {
	printk("Register %s is not valid for chip %s\n",
	       SOC_REG_NAME(unit, reg), SOC_UNIT_NAME(unit));
	return;
    }

    flags = reginfo->flags;

    COMPILER_64_ALLONES(mask);

    SOC_REG_RST_VAL_GET(unit, reg, rval);
    SOC_REG_RST_MSK_GET(unit, reg, rmsk);
    format_uint64(rval_str, rval);
    format_uint64(rmsk_str, rmsk);
    if (use_reset) {
        regval = rval;
        mask = rmsk;
    } else {
	format_uint64(dval_str, regval);
    }

    soc_reg_sprint_addr(unit, buf, ainfo);

    printk("Register: %s", buf);
#if !defined(SOC_NO_ALIAS)
    if (soc_reg_alias[reg] && *soc_reg_alias[reg]) {
        printk(" alias %s", soc_reg_alias[reg]);
    }
#endif /* !defined(SOC_NO_ALIAS) */
    printk(" %s register", soc_regtypenames[reginfo->regtype]);
    printk(" address 0x%08x\n", ainfo->addr);

    printk("Flags:");
    if (flags & SOC_REG_FLAG_64_BITS) {
        printk(" 64-bits");
    } else {
        printk(" 32-bits");
    }
    if (flags & SOC_REG_FLAG_COUNTER) {
        printk(" counter");
    }
    if (flags & SOC_REG_FLAG_ARRAY) {
        printk(" array[%d-%d]", 0, reginfo->numels-1);
    }
    if (flags & SOC_REG_FLAG_NO_DGNL) {
        printk(" no-diagonals");
    }
    if (flags & SOC_REG_FLAG_RO) {
        printk(" read-only");
    }
    if (flags & SOC_REG_FLAG_WO) {
        printk(" write-only");
    }
    if (flags & SOC_REG_FLAG_ED_CNTR) {
        printk(" error/discard-counter");
    }
    if (flags & SOC_REG_FLAG_SPECIAL) {
        printk(" special");
    }
    if (flags & SOC_REG_FLAG_EMULATION) {
        printk(" emulation");
    }
    if (flags & SOC_REG_FLAG_VARIANT1) {
        printk(" variant1");
    }
    if (flags & SOC_REG_FLAG_VARIANT2) {
        printk(" variant2");
    }
    if (flags & SOC_REG_FLAG_VARIANT3) {
        printk(" variant3");
    }
    if (flags & SOC_REG_FLAG_VARIANT4) {
        printk(" variant4");
    }
    printk("\n");

    printk("Blocks:");
    copies = disabled = 0;
    for (i = 0; SOC_BLOCK_INFO(unit, i).type >= 0; i++) {
	/* if (SOC_BLOCK_INFO(unit, i).type & reginfo->block) { */
        /* if (SOC_BLOCK_IN_LIST(reginfo->block, SOC_BLOCK_INFO(unit, i).type)) { */
        if (SOC_BLOCK_IS_TYPE(unit, i, reginfo->block)) {
	    if (SOC_INFO(unit).block_valid[i]) {
		printk(" %s", SOC_BLOCK_NAME(unit, i));
	    } else {
		printk(" [%s]", SOC_BLOCK_NAME(unit, i));
		disabled += 1;
	    }
	    copies += 1;
	}
    }
    printk(" (%d cop%s", copies, copies == 1 ? "y" : "ies");
    if (disabled) {
	printk(", %d disabled", disabled);
    }
    printk(")\n");

#if !defined(SOC_NO_DESC)
    if (soc_reg_desc[reg] && *soc_reg_desc[reg]) {
	printk("Description: %s\n", soc_reg_desc[reg]);
    }
#endif /* !defined(SOC_NO_ALIAS) */
    printk("Displaying:");
    if (use_reset) {
	printk(" reset defaults");
    } else {
	printk(" value %s", dval_str);
    }
    printk(", reset value %s mask %s\n", rval_str, rmsk_str);

    for (f = reginfo->nFields - 1; f >= 0; f--) {
	fld = &reginfo->fields[f];
	printk("  %s<%d", SOC_FIELD_NAME(unit, fld->field),
	       fld->bp + fld->len - 1);
	if (fld->len > 1) {
	    printk(":%d", fld->bp);
	}
	fldval = soc_reg64_field_get(unit, reg, mask, fld->field);
	if (use_reset && COMPILER_64_IS_ZERO(fldval)) {
	    printk("> = x");
	} else {
	    fldval = soc_reg64_field_get(unit, reg, regval, fld->field);
	    format_uint64(buf, fldval);
	    printk("> = %s", buf);
	}
	if (fld->flags & (SOCF_RO|SOCF_WO)) {
	    printk(" [");
	    i = 0;
	    if (fld->flags & SOCF_RO) {
		printk("%sRO", i++ ? "," : "");
	    }
	    if (fld->flags & SOCF_WO) {
		printk("%sWO", i++ ? "," : "");
	    }
	    printk("]");
	}
	printk("\n");
    }
}

#define	PFLAG_ALIAS	0x01
#define	PFLAG_SUMMARY	0x02

static void
_print_regname(int unit, soc_reg_t reg, int *col, int pflags)
{
    int             len;
    soc_reg_info_t *reginfo;

    reginfo = &SOC_REG_INFO(unit, reg);
    len = sal_strlen(SOC_REG_NAME(unit, reg)) + 1;

    if (pflags & PFLAG_SUMMARY) {
	char	tname, *dstr1, *dstr2, *bname;
	int	dlen, copies, i;
	char	nstr[128], bstr[64];

	switch (reginfo->regtype) {
	case soc_schan_reg:	tname = 's'; break;
	case soc_cpureg:	tname = 'c'; break;
	case soc_genreg:	tname = 'g'; break;
	case soc_portreg:	tname = 'p'; break;
	case soc_cosreg:	tname = 'o'; break;
	case soc_hostmem_w:
	case soc_hostmem_h:
	case soc_hostmem_b:	tname = 'm'; break;
	case soc_phy_reg:	tname = 'f'; break;
	case soc_pci_cfg_reg:	tname = 'P'; break;
	default:		tname = '?'; break;
	}
#if !defined(SOC_NO_DESC)
	dstr2 = sal_strchr(soc_reg_desc[reg], '\n');
	if (dstr2 == NULL) {
	    dlen = sal_strlen(soc_reg_desc[reg]);
	} else {
	    dlen = dstr2 - soc_reg_desc[reg];
	}
	if (dlen > 30) {
	    dlen = 30;
	    dstr2 = "...";
	} else {
	    dstr2 = "";
	}
	dstr1 = soc_reg_desc[reg];
#else /* defined(SOC_NO_DESC) */
	dlen = 1;
	dstr1 = "";
	dstr2 = "";
#endif /* defined(SOC_NO_DESC) */
	if (reginfo->flags & SOC_REG_FLAG_ARRAY) {
	    sal_sprintf(nstr, "%s[%d]", SOC_REG_NAME(unit, reg), reginfo->numels);
	} else {
	    sal_sprintf(nstr, "%s", SOC_REG_NAME(unit, reg));
	}

	copies = 0;
	bname = NULL;
	for (i = 0; SOC_BLOCK_INFO(unit, i).type >= 0; i++) {
	    /*if (SOC_BLOCK_INFO(unit, i).type & reginfo->block) {*/
            if (SOC_BLOCK_IS_TYPE(unit, i, reginfo->block)) {
		if (bname == NULL) {
		    bname = SOC_BLOCK_NAME(unit, i);
		}
		copies += 1;
	    }
	}
	if (copies > 1) {
	    sal_sprintf(bstr, "%d/%s", copies, bname);
	} else if (copies == 1) {
	    sal_sprintf(bstr, "%s", bname);
	} else {
	    sal_sprintf(bstr, "none");
	}
	printk(" %c%c%c%c%c  %-26s %-8.8s  %*.*s%s\n",
	       tname,
	       (reginfo->flags & SOC_REG_FLAG_64_BITS) ? '6' : '3',
	       (reginfo->flags & SOC_REG_FLAG_COUNTER) ? 'c' : '-',
	       (reginfo->flags & SOC_REG_FLAG_ED_CNTR) ? 'e' : '-',
	       (reginfo->flags & SOC_REG_FLAG_RO) ? 'r' :
	       (reginfo->flags & SOC_REG_FLAG_WO) ? 'w' : '-',
	       nstr,
	       bstr,
	       dlen, dlen, dstr1, dstr2);
	return;
    }
    if (*col < 0) {
	printk("  ");
	*col = 2;
    }
    if (*col + len > ((pflags & PFLAG_ALIAS) ? 65 : 72)) {
	printk("\n  ");
	*col = 2;
    }
    printk("%s%s ", SOC_REG_NAME(unit, reg), SOC_REG_ARRAY(unit, reg) ? "[]" : "");
#if !defined(SOC_NO_ALIAS)
    if ((pflags & PFLAG_ALIAS) && soc_reg_alias[reg]) {
        len += sal_strlen(soc_reg_alias[reg]) + 8;
        printk("(aka %s) ", soc_reg_alias[reg]);
    }
#endif /* !defined(SOC_NO_ALIAS) */
    *col += len;
}

static void
_list_regs_by_type(int unit, soc_block_t blk, int *col, int pflag)
{
    soc_reg_t       reg;

    *col = -1;
    for (reg = 0; reg < NUM_SOC_REG; reg++) {
        if (!SOC_REG_IS_VALID(unit, reg)) {
            continue;
        }
	/*if (SOC_REG_INFO(unit, reg).block & blk) {*/
        if (SOC_BLOCK_IN_LIST(SOC_REG_INFO(unit, reg).block, blk)) {
            _print_regname(unit, reg, col, pflag);
        }
    }
    printk("\n");
}

cmd_result_t
cmd_esw_reg_list(int unit, args_t *a)
{
    char           *str;
    char           *val;
    uint64          value;
    soc_regaddrinfo_t ainfo;
    int             found;
    int             rv = CMD_OK;
    int             all_regs;
    soc_reg_t       reg;
    int             col;
    int		    pflag;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (!soc_property_get(unit, spn_REGLIST_ENABLE, 1)) {
        return CMD_OK;
    }

    ainfo.reg = INVALIDr;
    pflag = 0;
    col = -1;

    /* Parse options */
    while (((str = ARG_GET(a)) != NULL) && (str[0] == '-')) {
	while (str[0] && str[0] == '-') {
	    str += 1;
	}
        if (sal_strcasecmp(str, "alias") == 0 ||
	    sal_strcasecmp(str, "a") == 0) {	/* list w/ alias */
            pflag |= PFLAG_ALIAS;
	    continue;
	}
        if (sal_strcasecmp(str, "summary") == 0 ||
	    sal_strcasecmp(str, "s") == 0) {	/* list w/ summary */
            pflag |= PFLAG_SUMMARY;
	    continue;
        }
	if (sal_strcasecmp(str, "counters") == 0 ||
	    sal_strcasecmp(str, "c") == 0) {	/* list counters */
            printk("unit %d counters\n", unit);
            for (reg = 0; reg < NUM_SOC_REG; reg++) {
                if (!SOC_REG_IS_VALID(unit, reg))
                    continue;
                if (!SOC_REG_IS_COUNTER(unit, reg))
                    continue;
                _print_regname(unit, reg, &col, pflag);
            }
            printk("\n\n");
            return CMD_OK;
        }
	if (sal_strcasecmp(str, "ed") == 0 ||
	    sal_strcasecmp(str, "e") == 0) {	/* error/discard */
            printk("unit %d error/discard counters\n", unit);
            for (reg = 0; reg < NUM_SOC_REG; reg++) {
                if (!SOC_REG_IS_VALID(unit, reg)) {
                    continue;
                }
                if (!(SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_ED_CNTR)) {
                    continue;
                }
                _print_regname(unit, reg, &col, pflag);
            }
            printk("\n\n");
            return CMD_OK;
        }
	if (sal_strcasecmp(str, "type") == 0 ||
	    sal_strcasecmp(str, "t") == 0) {	/* list by type */
            int	       i;
            soc_info_t *si = &SOC_INFO(unit);

	    for (i = 0; i < COUNTOF(si->has_block); i++) {
		if (!(si->has_block[i])) {
		    continue;
		}
		printk("unit %d %s registers\n",
		       unit,
		       soc_block_name_lookup_ext(si->has_block[i], unit));
		col = -1;
                _list_regs_by_type(unit, si->has_block[i], &col, pflag);
	    }
            printk("\n");
            return CMD_OK;
        }
	printk("ERROR: unrecognized option: %s\n", str);
	return CMD_FAIL;
    }

    if (!str) {
	return CMD_USAGE;
    }

    if ((val = ARG_GET(a)) != NULL) {
	value = parse_uint64(val);
    } else {
	COMPILER_64_ZERO(value);
    }


    if (isint(str)) {
	/* 
	 * Address given, look up SOC register.
	 */
	char            buf[80];
	uint32          addr;
	addr = parse_integer(str);
	soc_regaddrinfo_get(unit, &ainfo, addr);
	if (!ainfo.valid || (int)ainfo.reg < 0) {
	    printk("Unknown register address: 0x%x\n", addr);
	    rv = CMD_FAIL;
	} else {
	    soc_reg_sprint_addr(unit, buf, &ainfo);
	    printk("Address %s\n", buf);
	}
    } else {
        soc_regaddrlist_t alist;

        if (soc_regaddrlist_alloc(&alist) < 0) {
            printk("Could not allocate address list.  Memory error.\n");
            return CMD_FAIL;
        }

	/* 
	 * Symbolic name.
	 * First check if the register is there as exact match.
	 * If not, list all substring matches.
	 */

	all_regs = 0;
	if (*str == '$') {
	    str++;
	} else if (*str == '*') {
	    str++;
	    all_regs = 1;
	}

	if (all_regs || parse_symbolic_reference(unit, &alist, str) < 0) {
	    found = 0;
	    for (reg = 0; reg < NUM_SOC_REG; reg++) {
		if (!SOC_REG_IS_VALID(unit, reg)) {
		    continue;
		}
		if (strcaseindex(SOC_REG_NAME(unit, reg), str) != 0) {
		    if (!found && !all_regs) {
			printk("Unknown register; possible matches are:\n");
		    }
                    _print_regname(unit, reg, &col, pflag);
		    found = 1;
		}
	    }
	    if (!found) {
		printk("No matching register found");
	    }
	    printk("\n");
	    rv = CMD_FAIL;
	} else {
	    ainfo = alist.ainfo[0];
	}

        soc_regaddrlist_free(&alist);
    }

    /* 
     * Now have ainfo -- if reg is no longer INVALIDr
     */

    if (ainfo.reg != INVALIDr) {
	if (val) {
	    do_reg_list(unit, &ainfo, 0, value);
	} else {
            COMPILER_64_ZERO(value);
	    do_reg_list(unit, &ainfo, 1, value);
	}
    }

    return rv;
}

/* 
 * Editreg allows modifying register fields.
 * Works on fully qualified SOC registers only.
 */

cmd_result_t
cmd_esw_reg_edit(int unit, args_t *a)
{
    soc_reg_info_t *reginfo;
    soc_field_info_t *fld;
    soc_regaddrlist_t alist;
    soc_reg_t       reg;
    uint64	    v64;
    uint32          val, dfl, fv;
    char            ans[64], dfl_str[64];
    char           *name = ARG_GET(a);
    int             r, rv = CMD_FAIL;
    int             i, f;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return rv;
    }

    if (!name) {
	return CMD_USAGE;
    }

    if (*name == '$') {
	name++;
    }

    if (soc_regaddrlist_alloc(&alist) < 0) {
        printk("Could not allocate address list.  Memory error.\n");
        return CMD_FAIL;
    }

    if (parse_symbolic_reference(unit, &alist, name) < 0) {
        printk("Syntax error parsing \"%s\"\n", name);
        soc_regaddrlist_free(&alist);
        return (rv);
    }

    reg = alist.ainfo[0].reg;
    reginfo = &SOC_REG_INFO(unit, reg);

    /* 
     * If more than one register was specified, read the first one
     * and write the edited value to all of them.
     */

    if (soc_anyreg_read(unit, &alist.ainfo[0], &v64) < 0) {
        printk("ERROR: read reg failed\n");
        soc_regaddrlist_free(&alist);
        return (rv);
    }

    COMPILER_64_TO_32_LO(val, v64);

    printk("Current value: 0x%x\n", val);

    for (f = 0; f < (int)reginfo->nFields; f++) {
	fld = &reginfo->fields[f];
	dfl = soc_reg_field_get(unit, reg, val, fld->field);
	sal_sprintf(dfl_str, "0x%x", dfl);
	sal_sprintf(ans,		       /* Also use ans[] for prompt */
		"  %s<%d", SOC_FIELD_NAME(unit, fld->field), fld->bp + fld->len - 1);
	if (fld->len > 1) {
	    sal_sprintf(ans + sal_strlen(ans), ":%d", fld->bp);
	}
	sal_strcat(ans, ">? ");
	if (sal_readline(ans, ans, sizeof(ans), dfl_str) == 0 || ans[0] == 0) {
	    printk("Aborted\n");
        soc_regaddrlist_free(&alist);
        return (rv);
	}
	fv = parse_integer(ans);
	if (fv & ~((1 << (fld->len - 1) << 1) - 1)) {
	    printk("Value too big for %d-bit field, try again.\n",
		   fld->len);
	    f--;
	} else {
	    soc_reg_field_set(unit, reg, &val, fld->field, fv);
	}
    }

    printk("Writing new value: 0x%x\n", val);

    for (i = 0; i < alist.count; i++) {
        COMPILER_64_SET(v64, 0, val);

	if ((r = soc_anyreg_write(unit, &alist.ainfo[i], v64)) < 0) {
	    printk("ERROR: write reg 0x%x failed: %s\n",
		   alist.ainfo[i].addr, soc_errmsg(r));
        soc_regaddrlist_free(&alist);
        return (rv);
	}
    }

    rv = CMD_OK;

    soc_regaddrlist_free(&alist);
    return rv;
}
void 
reg_watch_cb(int unit, soc_reg_t reg, uint32 flags, 
             uint32 data_hi, uint32 data_lo, void *user_data)
{   
    if (!SOC_REG_IS_VALID(unit, reg)) {
        return;
    }   
    if (flags & SOC_REG_SNOOP_READ) {
#if !defined(SOC_NO_NAMES)
        printk("Unit=%d REGISTER=%s(Read) \t",unit,soc_reg_name[reg]);
#endif
        if (data_hi != 0) {
            printk("HighData=0x%08x \t",data_hi);
            printk("LowData=0x%08x \n",data_lo);
        } else {
            printk("Data=0x%08x \n",data_lo);
        }
    } else {
        if (flags & SOC_REG_SNOOP_WRITE) {
#if !defined(SOC_NO_NAMES)
            printk("Unit=%d REGISTER=%s(WRITE) \t",unit,soc_reg_name[reg]);
#endif
            if (data_hi != 0) {
                printk("HighData=0x%08x \t",data_hi);
                printk("LowData=0x%08x \n",data_lo);
            } else {
                printk("Data=0x%08x \n",data_lo);
            }
        }
    }
    return;
}

cmd_result_t
reg_watch(int unit, args_t *a)
{
    soc_regaddrlist_t alist;
    soc_reg_t       reg;
    /* uint64	    v64; */
    char           *name = ARG_GET(a);
    char           *c;
    int             rv = CMD_FAIL;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return rv;
    }

    if (!name) {
	return CMD_USAGE;
    }

    if (soc_regaddrlist_alloc(&alist) < 0) {
        printk("Could not allocate address list.  Memory error.\n");
        return CMD_FAIL;
    }

    if (parse_symbolic_reference(unit, &alist, name) < 0) {
        printk("Syntax error parsing \"%s\"\n", name);
        soc_regaddrlist_free(&alist);
        return (rv);
    }
    reg = alist.ainfo[0].reg;

    if (NULL == (c = ARG_GET(a))) {
        return CMD_USAGE;
    }

    if (sal_strcasecmp(c, "off") == 0) {
        /* unregister call back */
        soc_reg_snoop_unregister(unit, reg);
    } else if (sal_strcasecmp(c, "read") == 0) {
        /* register callback with read flag */
       soc_reg_snoop_register(unit,reg,SOC_REG_SNOOP_READ, reg_watch_cb, NULL);
    } else if (sal_strcasecmp(c, "write") == 0) {
        /* register callback with write flag */
      soc_reg_snoop_register(unit,reg,SOC_REG_SNOOP_WRITE, reg_watch_cb, NULL); 
    } else {
        return CMD_USAGE;
    }
    return CMD_OK;
}
