/*
 * $Id: diag_cint_data.c 1.47.2.1 Broadcom SDK $
 *
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
 */

#include <sdk_config.h>

#include <bcm/port.h>

#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/error.h>
#include <soc/register.h>
#include <soc/i2c.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#include <soc/mcm/memregs.h>
#endif
#include <soc/mem.h>
#ifdef BCM_PETRA_SUPPORT
#include <soc/dpp/drv.h>
#endif

#if defined(INCLUDE_LIB_CINT)

#include <appl/diag/system.h>

#include <cint_config.h>
#include <cint_types.h>
#include <cint_porting.h>
#include <sal/core/libc.h>
#include <sal/core/boot.h>
#ifdef BCM_88650_A0
#include <soc/dpp/ARAD/arad_tbl_access.h>
#include <soc/dpp/ARAD/arad_init.h>
#include <soc/dpp/ARAD/arad_sw_db.h>
#include <soc/dpp/ARAD/arad_ingress_packet_queuing.h>
#endif
#ifdef BCM_88750_A0
#include <soc/dfe/fe1600/fe1600_drv.h>
#endif

#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif

#if (defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INTR)
#include <appl/dcmn/rx_los/rx_los.h>
#endif /*(defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INT)*/

typedef soc_reg_above_64_val_t reg_val;
typedef uint32                 mem_val[SOC_MAX_MEM_WORDS];

#define DIAG_CINT_DATA_VERB(stuff)         DIAG_DEBUG(SOC_DBG_VERBOSE, stuff)

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)

static int diag_reg_get(int unit, char *name, reg_val value)
{
    int             rv = SOC_E_NONE;
    soc_regaddrlist_t alist;
    soc_regaddrinfo_t    *ainfo;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }

    if (name == NULL) {
        return SOC_E_PARAM;
    }

    if (soc_regaddrlist_alloc(&alist) < 0) {
        printk("Could not allocate address list.  Memory error.\n");
        return SOC_E_PARAM;
    }

    if (*name == '$') {
        name++;
    }

#ifdef BCM_PETRAB_SUPPORT
    /* direct 32 bit register access by address for Petra without indentifying the register */
    if ((SOC_IS_PETRAB(unit)) && isint(name)) {    /* Numerical address given */
        uint32 regaddr = parse_integer(name);

        SOC_REG_ABOVE_64_CLEAR(value);
        rv = soc_dpp_reg32_read(unit, regaddr, value);
        /* printk("read 0x%x=0x%x %ssuccessful\n",regaddr, value[0], rv ? "un" : ""); */
    } else
#endif /* BCM_PETRAB_SUPPORT */

    /* Symbolic name given, print all or some values ... */
    if (parse_symbolic_reference(unit, &alist, name) < 0) {
        printk("Syntax error parsing \"%s\"\n", name);
        rv = SOC_E_PARAM;
    } else {
        if (alist.count >1) {
            /* a list is not supported, only a single address */
            printk("Only a single address can be read %s.\n", name);
            rv = SOC_E_PARAM;
        }
        else {
            ainfo = &alist.ainfo[0];
            if(soc_cpureg == SOC_REG_INFO(unit,ainfo->reg).regtype){
                SOC_REG_ABOVE_64_CLEAR(value);
                value[0] = soc_pci_read(unit, SOC_REG_INFO(unit,ainfo->reg).offset);
                rv = BCM_E_NONE;
            } else if (( rv = soc_reg_above_64_get(unit, ainfo->reg, ainfo->port, ainfo->idx, value)) < 0) {
                char            buf[80];
                soc_reg_sprint_addr(unit, buf, ainfo);
                printk("ERROR: read from register %s failed: %s\n",
                       buf, soc_errmsg(rv));
            }
        }
    }
    soc_regaddrlist_free(&alist);
    return rv;
}

static int diag_reg_field_get(int unit, char *name, char* field_name, reg_val value)
{
    int             rv = SOC_E_NONE;
    soc_regaddrlist_t alist;
    soc_regaddrinfo_t    *ainfo;
    soc_reg_info_t *reginfo;
    int             f;
    reg_val value_all;
    int field_found = 0;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }

    if (name == NULL) {
        return SOC_E_PARAM;
    }

    if (soc_regaddrlist_alloc(&alist) < 0) {
        printk("Could not allocate address list.  Memory error.\n");
        return SOC_E_PARAM;
    }

    if (*name == '$') {
        name++;
    }

#ifdef BCM_PETRAB_SUPPORT
    /* direct 32 bit register access by address for Petra without indentifying the register */
    if ((SOC_IS_PETRAB(unit)) && isint(name)) {    /* Numerical address given */
        uint32 regaddr = parse_integer(name);

        SOC_REG_ABOVE_64_CLEAR(value);
        rv = soc_dpp_reg32_read(unit, regaddr, value);
        /* printk("read 0x%x=0x%x %ssuccessful\n",regaddr, value[0], rv ? "un" : ""); */
    } else
#endif /* BCM_PETRAB_SUPPORT */

    

    /* Symbolic name given, print all or some values ... */
    if (parse_symbolic_reference(unit, &alist, name) < 0) {
        printk("Syntax error parsing \"%s\"\n", name);
        rv = SOC_E_PARAM;
    } else {
        if (alist.count >1) {
            /* a list is not supported, only a single address */
            printk("Only a single address can be read %s.\n", name);
            rv = SOC_E_PARAM;
        }
        else {
            ainfo = &alist.ainfo[0];
            reginfo = &SOC_REG_INFO(unit, ainfo->reg);
            if(soc_cpureg == SOC_REG_INFO(unit,ainfo->reg).regtype){
                SOC_REG_ABOVE_64_CLEAR(value);
                value[0] = soc_pci_read(unit, SOC_REG_INFO(unit,ainfo->reg).offset);
                rv = BCM_E_NONE

                    ;
            } else if (( rv = soc_reg_above_64_get(unit, ainfo->reg, ainfo->port, ainfo->idx, value_all)) < 0) {
                char            buf[80];
                soc_reg_sprint_addr(unit, buf, ainfo);
                printk("ERROR: read from register %s failed: %s\n",
                       buf, soc_errmsg(rv));
            }

            /*search field by name*/
            for (f = reginfo->nFields - 1; f >= 0; f--) {
                soc_field_info_t *fld = &reginfo->fields[f];
                if (sal_strcasecmp(SOC_FIELD_NAME(unit, fld->field),field_name)) {
                    continue;
                }
                field_found = 1;
                soc_reg_above_64_field_get(unit, ainfo->reg, value_all, fld->field, value);
                break;
            }
            if (field_found == 0) {
                rv = SOC_E_NOT_FOUND;
            }


        }
    }
    soc_regaddrlist_free(&alist);
    return rv;
}




static int diag_reg_set(int unit, char *name, reg_val value)
{
    int rv = SOC_E_NONE, i;
    soc_regaddrlist_t   alist;
    soc_regaddrinfo_t   *ainfo;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }

    if (name == NULL) {
        return SOC_E_PARAM;
    }

    if (soc_regaddrlist_alloc(&alist) < 0) {
        printk("Could not allocate address list.  Memory error.\n");
        return SOC_E_PARAM;
    }

    if (*name == '$') {
        name++;
    }

#ifdef BCM_PETRAB_SUPPORT
    /* direct 32 bit register access by address for Petra without indentifying the register */
    if ((SOC_IS_PETRAB(unit)) && isint(name)) {    /* Numerical address given */
        uint32 regaddr = parse_integer(name);

        rv = soc_dpp_reg32_write(unit, regaddr, value[0]);
        /* printk("write 0x%x=0x%x %ssuccessful\n",regaddr, value[0], rv ? "un" : ""); */
    } else
#endif /* BCM_PETRAB_SUPPORT */

    /* Symbolic name given, print all or some values ... */
    if (parse_symbolic_reference(unit, &alist, name) < 0) {
        printk("Syntax error parsing \"%s\"\n", name);
        rv = SOC_E_PARAM;
    } else {
        for(i=0 ; i<alist.count && SOC_E_NONE == rv ; i++) {
            ainfo = &alist.ainfo[0];
            if(soc_cpureg == SOC_REG_INFO(unit,ainfo->reg).regtype){
                soc_pci_write(unit, SOC_REG_INFO(unit,ainfo->reg).offset, value[0]);
                rv = BCM_E_NONE;
            } else if (( rv = soc_reg_above_64_set(unit, ainfo->reg, ainfo->port, ainfo->idx, value)) < 0) {
                char            buf[80];
                soc_reg_sprint_addr(unit, buf, ainfo);
                printk("ERROR: write to register %s failed: %s\n",
                       buf, soc_errmsg(rv));
            }
        }
    }
    soc_regaddrlist_free(&alist);
    return rv;
}

static int diag_reg_field_set(int unit, char *name, char* field_name,reg_val value)
{
    int rv = SOC_E_NONE, i;
    soc_regaddrlist_t   alist;
    soc_regaddrinfo_t   *ainfo;
    soc_reg_info_t *reginfo;
    int             f;
    reg_val value_all;
    int field_found = 0;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }

    if (name == NULL) {
        return SOC_E_PARAM;
    }

    if (soc_regaddrlist_alloc(&alist) < 0) {
        printk("Could not allocate address list.  Memory error.\n");
        return SOC_E_PARAM;
    }

    if (*name == '$') {
        name++;
    }

#ifdef BCM_PETRAB_SUPPORT
    /* direct 32 bit register access by address for Petra without indentifying the register */
    if ((SOC_IS_PETRAB(unit)) && isint(name)) {    /* Numerical address given */
        uint32 regaddr = parse_integer(name);

        rv = soc_dpp_reg32_write(unit, regaddr, value[0]);
        /* printk("write 0x%x=0x%x %ssuccessful\n",regaddr, value[0], rv ? "un" : ""); */
    } else
#endif /* BCM_PETRAB_SUPPORT */

    /* Symbolic name given, print all or some values ... */
    if (parse_symbolic_reference(unit, &alist, name) < 0) {
        printk("Syntax error parsing \"%s\"\n", name);
        rv = SOC_E_PARAM;
    } else {
        for(i=0 ; i<alist.count && SOC_E_NONE == rv ; i++) {
            ainfo = &alist.ainfo[0];
            reginfo = &SOC_REG_INFO(unit, ainfo->reg);
            if(soc_cpureg == SOC_REG_INFO(unit,ainfo->reg).regtype){
              /*  soc_pci_write(unit, SOC_REG_INFO(unit,ainfo->reg).offset, value[0]);
                rv = BCM_E_NONE; */
                SOC_REG_ABOVE_64_CLEAR(value);
                value[0] = soc_pci_read(unit, SOC_REG_INFO(unit,ainfo->reg).offset);

            } else if (( rv = soc_reg_above_64_get(unit, ainfo->reg, ainfo->port, ainfo->idx, value_all)) < 0){
                  char            buf[80];
                  soc_reg_sprint_addr(unit, buf, ainfo);
                  printk("ERROR: read from register %s failed: %s\n",
                         buf, soc_errmsg(rv));
              }

                /*search field by name*/
                for (f = reginfo->nFields - 1; f >= 0; f--) {
                    soc_field_info_t *fld = &reginfo->fields[f];
                    if (sal_strcasecmp(SOC_FIELD_NAME(unit, fld->field),field_name)) {
                        continue;
                    }
                    field_found = 1;
                    soc_reg_above_64_field_set(unit, ainfo->reg, value_all, fld->field, value);
                    break;
                }
                if (field_found == 0) {
                    rv = SOC_E_NOT_FOUND;
                } else {
                    if(soc_cpureg == SOC_REG_INFO(unit,ainfo->reg).regtype){
                        soc_pci_write(unit, SOC_REG_INFO(unit,ainfo->reg).offset, value_all[0]);
                        rv = BCM_E_NONE;
                    } else if (( rv = soc_reg_above_64_set(unit, ainfo->reg, ainfo->port, ainfo->idx, value_all)) < 0) {
                        char            buf[80];
                        soc_reg_sprint_addr(unit, buf, ainfo);
                        printk("ERROR: write to register %s failed: %s\n",
                               buf, soc_errmsg(rv));
                    }

                }
            }
    }
    soc_regaddrlist_free(&alist);
    return rv;
}

static int diag_mem_get(int unit, char *name, int index, mem_val value)
{
    int         copyno;
    soc_mem_t   mem;
    int         rv = SOC_E_NONE;
    unsigned    array_index;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;  
    }

    if (name == NULL) {
        return SOC_E_PARAM;
    }

    if (parse_memory_name(unit, &mem, name, &copyno, &array_index) < 0) {
        printk("ERROR: unknown table \"%s\"\n",name);
        return SOC_E_PARAM;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        printk("Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        return SOC_E_PARAM;
    }

    /* Created entry, now write it */
    if ((rv = soc_mem_array_read(unit, mem, array_index, copyno, index, value)) < 0) {
        printk("Read ERROR: table %s.%d[%d]: %s\n",
               SOC_MEM_UFNAME(unit, mem), copyno==COPYNO_ALL ? 0 : copyno, index,
               soc_errmsg(rv));
    }


    return rv;
}


/* get the value of a memory field */
static int diag_mem_field_get(int unit, char *name, char* field_name, int index, mem_val value)
{
    int         copyno;
    soc_field_info_t *fieldp;
    soc_mem_t   mem;
    soc_mem_info_t *memp;
    int f;
    uint32 fval[SOC_MAX_MEM_FIELD_WORDS];
    char tmp[(SOC_MAX_MEM_FIELD_WORDS * 8) + 3];
    int         rv = SOC_E_NONE;
    unsigned    array_index;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;  
    }

    if (name == NULL) {
        return SOC_E_PARAM;
    }

    if (parse_memory_name(unit, &mem, name, &copyno, &array_index) < 0) {
        printk("ERROR: unknown table \"%s\"\n",name);
        return SOC_E_PARAM;
    }

    memp = &SOC_MEM_INFO(unit, mem);

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        printk("Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        return SOC_E_PARAM;
    }

    sal_memset(fval, 0, (sizeof(uint32)) * SOC_MAX_MEM_FIELD_WORDS);
    sal_memset(value, 0, sizeof (mem_val));
    sal_memset(tmp, 0, sizeof(char) * ((SOC_MAX_MEM_FIELD_WORDS * 8) + 3) );

    if ((rv = soc_mem_array_read(unit, mem, array_index, copyno, index, fval)) < 0) {
        printk("Read ERROR: table %s.%d[%d]: %s\n",
               SOC_MEM_UFNAME(unit, mem), copyno==COPYNO_ALL ? 0 : copyno, index,
               soc_errmsg(rv));
    }
    for (f = memp->nFields - 1; f >= 0; f--) {
        fieldp = &memp->fields[f];
        if (!(sal_strcasecmp(SOC_FIELD_NAME(unit, fieldp->field),field_name))) {
            soc_mem_field_get(unit, mem, fval, fieldp->field, value);
            _shr_format_long_integer(tmp, value, SOC_MAX_MEM_FIELD_WORDS);
#if !defined(SOC_NO_NAMES)
            DIAG_CINT_DATA_VERB(("%s=", soc_fieldnames[fieldp->field])); 
#endif
            DIAG_CINT_DATA_VERB(("%s\n", tmp));
            break;
        }
    }
    return rv;
}

static int diag_soc_mem_read_range(int unit, char *name, int index1, int index2, mem_val value)
{
    int         copyno;
    soc_mem_t   mem;
    int         rv = SOC_E_NONE;
    unsigned    array_index;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }

    if (name == NULL) {
        return SOC_E_PARAM;
    }

    if (parse_memory_name(unit, &mem, name, &copyno, &array_index) < 0) {
        printk("ERROR: unknown table \"%s\"\n",name);
        return SOC_E_PARAM;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        printk("Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        return SOC_E_PARAM;
    }

    /* Created entry, now write it */
    if ((rv = soc_mem_array_read_range(unit, mem, array_index, copyno, index1, index2, value)) < 0) {
        printk("Read ERROR: table %s.%d[%d-%d]: %s\n",
               SOC_MEM_UFNAME(unit, mem), copyno==COPYNO_ALL ? 0 : copyno, index1, index2,
               soc_errmsg(rv));
    }

    return rv;
}

static int diag_mem_set(int unit, char *name, int start, int count, mem_val value)
{
    int         index, copyno;
    soc_mem_t   mem;
    int         rv = SOC_E_NONE;
    unsigned    array_index;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }

    if (name == NULL) {
        return SOC_E_PARAM;
    }

    if (parse_memory_name(unit, &mem, name, &copyno, &array_index) < 0) {
        printk("ERROR: unknown table \"%s\"\n",name);
        return SOC_E_PARAM;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        printk("Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        return SOC_E_PARAM;
    }

    if (soc_mem_is_readonly(unit, mem)) {
        printk("ERROR: Table %s is read-only\n",
               SOC_MEM_UFNAME(unit, mem));
        return SOC_E_PARAM;
    }

    /* Created entry, now write it */
    for (index = start; index < start + count && SOC_E_NONE == rv; index++)
    {
        if ((rv = soc_mem_array_write(unit, mem, array_index, copyno, index, value)) < 0) {
            printk("Write ERROR: table %s.%d[%d]: %s\n",
                   SOC_MEM_UFNAME(unit, mem), copyno==COPYNO_ALL ? 0 : copyno, index,
                   soc_errmsg(rv));
        }
    }

    return rv;
}

static int diag_mem_field_set(int unit, char *name, char* field_name, int start, int count, mem_val value)
{
    int         index, copyno;
    soc_mem_t   mem;
    int         rv = SOC_E_NONE;
    soc_field_info_t *fieldp;
    soc_mem_info_t *memp;
    int f;
    uint32 fval[SOC_MAX_MEM_FIELD_WORDS];
    unsigned    array_index;



    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }

    if (name == NULL) {
        return SOC_E_PARAM;
    }

    if (parse_memory_name(unit, &mem, name, &copyno, &array_index) < 0) {
        printk("ERROR: unknown table \"%s\"\n",name);
        return SOC_E_PARAM;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        printk("Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        return SOC_E_PARAM;
    }

    if (soc_mem_is_readonly(unit, mem)) {
        printk("ERROR: Table %s is read-only\n",
               SOC_MEM_UFNAME(unit, mem));
        return SOC_E_PARAM;
    }
    memp = &SOC_MEM_INFO(unit, mem);
    /* Created entry, now write it */
    for (index = start; index < start + count && SOC_E_NONE == rv; index++)
    {
        sal_memset(fval, 0, sizeof (fval));
        sal_memset(fval, 0, sizeof (value));

        if ((rv = soc_mem_array_read(unit, mem, array_index, copyno, index, fval)) < 0) {
            printk("Read ERROR: table %s.%d[%d]: %s\n",
                   SOC_MEM_UFNAME(unit, mem), copyno==COPYNO_ALL ? 0 : copyno, index,
                   soc_errmsg(rv));
        }
        for (f = memp->nFields - 1; f >= 0; f--) {
            fieldp = &memp->fields[f];
            if (sal_strcasecmp(SOC_FIELD_NAME(unit, fieldp->field),field_name)) {
                continue;
            }
            soc_mem_field_set(unit, mem, fval, fieldp->field, value);
        }

        if ((rv = soc_mem_array_write(unit, mem, array_index, copyno, index, fval)) < 0) {
            printk("Write ERROR: table %s.%d[%d]: %s\n",
                   SOC_MEM_UFNAME(unit, mem), copyno==COPYNO_ALL ? 0 : copyno, index,
                   soc_errmsg(rv));
        }
    }

    return rv;
}


static int diag_soc_mem_write_range(int unit, char *name, int index1, int index2, mem_val value)
{
    int         copyno;
    soc_mem_t   mem;
    int         rv = SOC_E_NONE;
    unsigned    array_index;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }

    if (name == NULL) {
        return SOC_E_PARAM;
    }

    if (parse_memory_name(unit, &mem, name, &copyno, &array_index) < 0) {
        printk("ERROR: unknown table \"%s\"\n",name);
        return SOC_E_PARAM;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        printk("Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        return SOC_E_PARAM;
    }

    if (soc_mem_is_readonly(unit, mem)) {
        printk("ERROR: Table %s is read-only\n",
               SOC_MEM_UFNAME(unit, mem));
        return SOC_E_PARAM;
    }

    /* Created entry, now write it */
    if ((rv = soc_mem_array_write_range(unit, 0, mem, array_index, copyno, index1, index2, value)) < 0) {
        printk("Write ERROR: table %s.%d[%d-%d]: %s\n",
               SOC_MEM_UFNAME(unit, mem), copyno==COPYNO_ALL ? 0 : copyno, index1, index2,
               soc_errmsg(rv));
    }

    return rv;
}

/* fill the table with the same given entry */
static int diag_fill_table(int unit, char *name, mem_val value)
{
    int         copyno;
    soc_mem_t   mem;
    int         rv = SOC_E_NONE;
    unsigned    array_index;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }

    if (name == NULL) {
        return SOC_E_PARAM;
    }

    if (parse_memory_name(unit, &mem, name, &copyno, &array_index) < 0) {
        printk("ERROR: unknown table \"%s\"\n",name);
        return SOC_E_PARAM;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        printk("Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        return SOC_E_PARAM;
    }

    if (soc_mem_is_readonly(unit, mem)) {
        printk("ERROR: Table %s is read-only\n",
               SOC_MEM_UFNAME(unit, mem));
        return SOC_E_PARAM;
    }

#ifdef BCM_88650_A0
    if(SOC_IS_ARAD(unit)) {
        if ((value) && ((rv = arad_fill_partial_table_with_entry(unit, mem, array_index, array_index, copyno, 0, soc_mem_index_max(unit, mem), value)))) {
            printk("Fill ERROR: table %s.%d: 0x%x\n", SOC_MEM_UFNAME(unit, mem), copyno==COPYNO_ALL ? 0 : copyno, rv);
        }
    } else
#endif
    {
        printk("fast table filling not supported on this device\n");
        rv = SOC_E_UNIT;
    }

    return rv;
}


/* fill the table with the same given entry */
static int diag_mbist(int unit, int skip_errors)
{
    uint32 rv = SOC_E_NONE;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }

#ifdef BCM_88650_A0
    if (SOC_IS_ARAD(unit)) {
        rv = soc_bist_all_arad(unit, skip_errors);
        if (rv) {
            printk("MBIST failed\n");
        }
    } else
#endif
#ifdef BCM_88750_A0
    if (SOC_IS_FE1600(unit)) {
        rv = soc_bist_all_fe1600(unit, skip_errors);
        if (rv) {
            printk("MBIST failed\n");
        }
    } else
#endif
    {
        printk("MBIST not supported on this device\n");
        rv = SOC_E_UNIT;
    }

    return rv;
}


/* clear, print or re-populate the Arad direct mapping modport2sysport table */
static int diag_modport2sysport(int unit, int func)
{
    uint32 rv = SOC_E_NONE;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }

#ifdef BCM_88650_A0
    if(SOC_IS_ARAD(unit)) {
        if (ARAD_IS_VOQ_MAPPING_INDIRECT(unit)) {
          printk("not supported in indirect mapping mode\n");
          return SOC_E_UNAVAIL;
        } else if (!func) { /* print mapping */
            uint32 mod, port;
            ARAD_SYSPORT sysport;
            for (mod = 0; mod < ARAD_NOF_FAPS_IN_SYSTEM; ++mod) {
                for (port = 0; port < ARAD_NOF_FAP_PORTS; ++port) {
                    if (arad_sw_db_modport2sysport_get(unit, mod, port, &sysport)) {
                         printk("failed to get the mapping for module %u port %u\n", mod, port);
                         return SOC_E_FAIL;
                    } else if (sysport != ARAD_NOF_SYS_PHYS_PORTS) {
                         printk("%u , %u -> %u\n", mod, port, sysport);
                    }
                }
            }
        } else if (func == 1 ) { /* clear mapping */
             if ( (rv = arad_sw_db_modport2sysport_destroy(unit)) ||
                  (rv = arad_sw_db_modport2sysport_create(unit)) ) {
                 printk("failed to clear the mapping. It may now be unusable\n");
                 return SOC_E_FAIL;
             }
        } else if (func == 2 ) { /* populate mapping from hardware tables */
            if (arad_populate_sw_db_modport2sysport_from_hw_unsafe(unit)) {
                 printk("failed to populate the mapping.\n");
                 return SOC_E_FAIL;
            }
        } else {
            printk("argument must be one of:\n0 - print current mapping\n1 - clear mapping\n2 - populate mapping from hardware\n");
            return SOC_E_PARAM;
        }
    } else
#endif
    {
        printk("not supported on this device\n");
        rv = SOC_E_UNIT;
    }

    return rv;
}

int
fill_memory_with_incremental_field(const int unit, const soc_mem_t mem, const soc_field_t field,
                                unsigned array_index_min, unsigned array_index_max,
                                const int copyno,
                                int index_min, int index_max,
                                const void *initial_entry)
{
    int    rv = 0, mem_words, mem_size, entry_words, indices_num;
    int    index, blk, tmp;
    unsigned array_index;
    uint32 *buf;
    uint32 *buf2;
    const uint32 *input_entry = initial_entry;
    uint32 field_buf[4] = {0}; /* To hold the field, max size 128 bits */

    if (initial_entry == NULL) {
        return SOC_E_PARAM;
    }

    /* get legal values for indices, if too small/big use the memory's boundaries */
    tmp = soc_mem_index_min(unit, mem);
    if (index_min < soc_mem_index_min(unit, mem)) {
      index_min = tmp;
    }
    if (index_max < index_min) {
        index_max = index_min;
    } else {
         tmp = soc_mem_index_max(unit, mem);
         if (index_max > tmp) {
             index_max = tmp;
         }
    }

    entry_words = soc_mem_entry_words(unit, mem);
    indices_num = index_max - index_min + 1;
    mem_words = indices_num * entry_words;
    mem_size = mem_words * 4;

    /* get the initial field from the input */
    soc_mem_field_get(unit, mem, initial_entry, field, field_buf);

    buf = soc_cm_salloc(unit, mem_size, "mem_clear_buf"); /* allocate DMA memory buffer */
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }

    /* get legal values for memory array indices */
    if (SOC_MEM_IS_ARRAY(unit, mem)) {
        soc_mem_array_info_t *maip = SOC_MEM_ARRAY_INFOP(unit, mem);
        if (maip) {
            if (array_index_max >= maip->numels) {
                array_index_max = maip->numels - 1;
            }
        } else {
            array_index_max = 0;
        }
        if (array_index_min > array_index_max) {
            array_index_min = array_index_max;
        }
    } else {
        array_index_min = array_index_max = 0;
    }

    /* fill the allocated memory with the input entry */
    for (index = 0; index < mem_words; ++index) {
        buf[index] = input_entry[index % entry_words];
    }

    SOC_MEM_BLOCK_ITER(unit, mem, blk) {
        if (copyno != COPYNO_ALL && copyno != blk) {
            continue;
        }
        for (array_index = array_index_min; array_index <= array_index_min; ++array_index) {
            /* update the field of all the entries in the buffer */
            for (index = 0, buf2 = buf; index < indices_num; ++index, buf2+=entry_words) {
                soc_mem_field_set(unit, mem, buf2, field, field_buf); /* set the index */
                /* increment the field, to be used in next entry */
                if (!++field_buf[0]) {
                    if (!++field_buf[1]) {
                        if (!++field_buf[2]) {
                            ++field_buf[3];
                        }
                    }
                }
            }

#if PLISIM
            for (index = index_min, buf2 = buf; index <= index_max; ++index, buf2+=entry_words) {
                if ((rv = soc_mem_array_write(unit, mem, array_index, blk, index, buf2)) < 0) {
                    printk("Write ERROR: table %s.%d[%d]: %s\n",
                       SOC_MEM_UFNAME(unit, mem), copyno==COPYNO_ALL ? 0 : copyno, index,
                       soc_errmsg(rv));
                }
            }
#else
            if ((rv = soc_mem_array_write_range(unit, 0, mem, array_index, blk, index_min, index_max, buf)) < 0) {
                printk("Write ERROR: table %s.%d[%d-%d]: %s\n",
                   SOC_MEM_UFNAME(unit, mem), copyno==COPYNO_ALL ? 0 : copyno, index_min, index_max,
                   soc_errmsg(rv));
            }
#endif
        }
    }
    soc_cm_sfree(unit, buf);
    return rv;
}

#ifdef BCM_88650_A0

int test_bcm_mc_arad_basic(int unit);
int test_bcm_mc_arad_basic_open(int unit);
int test_bcm_mc_arad_mid_size(int unit);
int test_bcm_mc_arad_replications(int unit, unsigned iter_num);
int test_bcm_mc_arad_1(int unit);
int test_bcm_mc_arad_delete_egress_replications(int unit);
void mc_test_set_assert_type(int unit, int atype);
void mc_test_set_debug_level(int unit, int level);

/* perform the multicast test specified by name, supported only for ARAD */
static int diag_test_mc(int unit, char *test_name)
{
    int         rv = SOC_E_NONE;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    }
    if (!SOC_IS_ARAD(unit)) {
        printk("Device not supported\n");
        return SOC_E_UNIT;
    }

    if (test_name == NULL) {
        return SOC_E_PARAM;
    }
    if (!sal_strcmp(test_name, "mc_open")) {
        if ((rv = test_bcm_mc_arad_basic_open(unit))) {
            printk("ERROR: test_bcm_mc_arad_basic returned %d\n",rv);
        }
        return rv;
    }
    if (!sal_strcmp(test_name, "mc_basic")) {
        if ((rv = test_bcm_mc_arad_basic(unit))) {
            printk("ERROR: test_bcm_mc_arad_basic returned %d\n",rv);
        }
        return rv;
    }
    if (!sal_strcmp(test_name, "mc_mid_size")) {
        if ((rv = test_bcm_mc_arad_mid_size(unit))) {
            printk("ERROR: test_bcm_mc_arad_mid_size returned %d\n",rv);
        }
        return rv;
    }
    if (!sal_strcmp(test_name, "mc_1")) {
        if ((rv = test_bcm_mc_arad_1(unit))) {
            printk("ERROR: test_bcm_mc_arad_1 returned %d\n",rv);
        }
        return rv;
    }    if (!sal_strcmp(test_name, "mc_egress_delete")) {
        if ((rv = test_bcm_mc_arad_delete_egress_replications(unit))) {
            printk("ERROR: test_bcm_mc_arad_delete_egress_replications returned %d\n",rv);
        }
        return rv;
    }
    if (!sal_strcmp(test_name, "print_all")) {
        mc_test_set_debug_level(unit, -1);
        return SOC_E_NONE;
    }
    if (!sal_strcmp(test_name, "print_errors_info")) {
        mc_test_set_debug_level(unit, 0);
        return SOC_E_NONE;
    }
    if (!sal_strcmp(test_name, "print_errors")) {
        mc_test_set_debug_level(unit, 1);
        return SOC_E_NONE;
    }
    if (!sal_strcmp(test_name, "assert")) {
        mc_test_set_assert_type(unit, 1);
        return SOC_E_NONE;
    }
    if (!sal_strcmp(test_name, "assert_0")) {
        mc_test_set_assert_type(unit, 0);
        return SOC_E_NONE;
    }
    if (!sal_strcmp(test_name, "assert_none")) {
        mc_test_set_assert_type(unit, 2);
        return SOC_E_NONE;
    }
    if (!sal_strcmp(test_name, "get_mc_asserts")) {
        uint32 num = _arad_swdb_get_nof_asserts();
        if (num) {
            printk("%lu multicast implementation asserts occurred\n", (unsigned long)num);
        }
        return num;
    } else if (!sal_strcmp(test_name, "get_nof_mcids")) {
        return SOC_DPP_CONFIG(unit)->tm.nof_mc_ids;
    } else if (!sal_strcmp(test_name, "get_nof_egress_bitmap_groups")) {
        return SOC_DPP_CONFIG(unit)->tm.multicast_egress_bitmap_group_range.mc_id_high + 1;
    }

    printk("ERROR: unknown test name, use one of: mc_open, mc_basic, mc_mid_size, mc_egress_delete,  print_all, print_errors_info, print_errors, assert, assert_0, assert_none, get_mc_asserts, get_nof_mcids, get_nof_egress_bitmap_groups\n");
    return SOC_E_PARAM;
}

/* perform the multicast test specified by name, supported only for ARAD */
static int diag_test_mc2(int unit, char *test_name, int param)
{
    int         rv = SOC_E_NONE;

    if (!SOC_UNIT_VALID(unit)) {
        printk("Invalid unit.\n");
        return SOC_E_UNIT;
    } else if (!SOC_IS_ARAD(unit)) {
        printk("Device not supported\n");
        return SOC_E_UNIT;
    } else if (!test_name) {
        return SOC_E_PARAM;
    } else if (!sal_strcmp(test_name, "mc_replications")) {
        if (param < 0) {
        printk("Illegal number of iterations\n");
        return SOC_E_PARAM;
        }
        if ((rv = test_bcm_mc_arad_replications(unit, param))) {
            printk("ERROR: test_bcm_mc_arad_replications(%d, %d) returned %d\n",unit, param, rv);
        }
        return rv;
    } else if (!sal_strcmp(test_name, "get_irdb")) {
        int out = -1;
        uint32 entry[2];
        const unsigned bit_offset = 2 * (param % 16);
        const int table_index = param / 16;
        if (param < 0 || param > ARAD_MULT_ID_MAX) {
            printk("Illegal mcid %d\n", param);
        } else if ((out = READ_IRR_IRDBm(unit, MEM_BLOCK_ANY, table_index, entry)) < 0) {
            printk("failed to read IRDB for group %d\n", param);
        } else {
            out = ((*entry) >> bit_offset) & 3;
        }
        return out;
    } else if (!sal_strcmp(test_name, "swdb_asserts_enable")) {
        arad_swdb_asserts_enabled = param;
        return SOC_E_NONE;
    }

    printk("ERROR: unknown test name, use one of: mc_replications get_irdb swdb_asserts_enable\n");
    return SOC_E_PARAM;
}
#endif /* BCM_88650_A0 */

#endif /*defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) */

static void diag_printk(char *str)
{
    printk(str);
}

#define DIAG_PRINTK_FLAGS_HEX 0x1

static void diag_printk_int(int num, int flags)
{
    if(flags & DIAG_PRINTK_FLAGS_HEX) {
        printk("%x",(uint32)num);
    } else {
        printk("%d",num);
    }
}

static void diag_pcie_read(int unit, uint32 addr, uint32 *val, int swap)
{

    *val = CMVEC(unit).read(&CMDEV(unit).dev, addr);

    if(swap) {
        *val = _shr_swap32(*val);
    }

    /* printk("%s(): unit=%d, addr=0x%x, swap=%d. after swap: *val=0x%x",__FUNCTION__, unit, addr, swap, *val); */
}

static void diag_pcie_write(int unit, uint32 addr, uint32 val, int swap)
{
    /* printk("%s(): unit=%d, addr=0x%x, val=0x%x, swap=%d",__FUNCTION__, unit, addr, val, swap); */

    if(swap) {
        val = _shr_swap32(val);
    }

    CMVEC(unit).write(&CMDEV(unit).dev, addr, val);

}

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
static int diag_soc_device_reset(int unit, int mode, int action)
{
    int rv = 0;

    rv = soc_device_reset(unit, mode, action);
    if (rv) {
        printk("Error: soc_device_reset Failed. rv=%d\n", rv);
        return rv;
    }

    return rv;
}

static int diag_cm_get_unit_by_id(uint16 dev_id, int occur, int *unit)
{
    int 
        rv = 0,
        num_devices = 0,
        tmp_occur = 0,
        tmp_unit,
        found = 0;
    uint16 
        tmp_dev_id;
    uint8 
        tmp_rev_id;


    num_devices = soc_cm_get_num_devices();

    for (tmp_unit = 0 ; tmp_unit < num_devices ; tmp_unit++ ) {
        soc_cm_get_id(tmp_unit, &tmp_dev_id, &tmp_rev_id);

        if (tmp_dev_id == dev_id) {
            if (tmp_occur == occur) {
                found = 1;
                break;
            }
            tmp_occur++;
        }
    }

    if (found == 1) {
        *unit = tmp_unit;
    } else {
        *unit = 0xffffffff;
    }

    return rv;
}
#endif /* #if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) */

#if (defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INTR)
CINT_FWRAPPER_CREATE_RP1(int, int, 0, 0,
                         rx_los_dump,
                         int,int,unit,0,0);

CINT_FWRAPPER_CREATE_RP2(int, int, 0, 0,
                         rx_los_dump_port,
                         int,int,unit,0,0,
                         bcm_port_t, bcm_port_t, port,0,0);

CINT_FWRAPPER_CREATE_RP2(int, int, 0, 0,
                         rx_los_handle,
                         int,int,unit,0,0,
                         bcm_port_t ,bcm_port_t ,port,0,0);

CINT_FWRAPPER_CREATE_RP4(int ,int, 0,0,
                         rx_los_port_enable,
                         int, int, unit, 0, 0,
                         bcm_port_t, bcm_port_t, port, 0, 0,
                         int, int, enable, 0, 0,
                         int, int, warm_boot, 0, 0);

CINT_FWRAPPER_CREATE_RP4(int ,int, 0,0,
                         rx_los_port_stable,
                         int, int, unit, 0, 0,
                         bcm_port_t, bcm_port_t, port, 0, 0,
                         int, int, timeout, 0, 0,
                         rx_los_state_t *, rx_los_state_t, state, 1, 0);

CINT_FWRAPPER_CREATE_RP3(int, int, 0, 0,
                         rx_los_unit_attach,
                         int,int,unit,0,0,
                         pbmp_t,pbmp_t,pbmp,0,0,
                         int, int, warm_boot, 0, 0);

CINT_FWRAPPER_CREATE_RP1(int, int, 0, 0,
                         rx_los_unit_detach,
                         int,int,unit,0,0);

CINT_FWRAPPER_CREATE_RP0(int, int, 0, 0,
                         rx_los_exit_thread);

CINT_FWRAPPER_CREATE_RP2(int, int, 0, 0,
                         rx_los_register,
                         int,int,unit,0,0,
                         rx_los_callback_t ,rx_los_callback_t ,callback,0,0);

#endif /*(defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INTR)*/

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)

CINT_FWRAPPER_CREATE_RP3(int, int, 0, 0,
                         diag_reg_get,
                         int,int,unit,0,0,
                         char*,char,name,1,0,
                         uint32*,uint32,value,1,0);

CINT_FWRAPPER_CREATE_RP4(int, int, 0, 0,
                         diag_reg_field_get,
                         int,int,unit,0,0,
                         char*,char,name,1,0,
                         char*,char,field_name,1,0,
                         uint32*,uint32,value,1,0);

CINT_FWRAPPER_CREATE_RP3(int, int, 0, 0,
                         diag_reg_set,
                         int,int,unit,0,0,
                         char*,char,name,1,0,
                         uint32*,uint32,value,1,0);

CINT_FWRAPPER_CREATE_RP4(int, int, 0, 0,
                         diag_reg_field_set,
                         int,int,unit,0,0,
                         char*,char,name,1,0,
                         char*,char,field_name,1,0,
                         uint32*,uint32,value,1,0);

CINT_FWRAPPER_CREATE_RP3(int, int, 0, 0,
                         diag_soc_device_reset,
                         int,int,unit,0,0,
                         int,int,mode,0,0,
                         int,int,action,0,0);

CINT_FWRAPPER_CREATE_RP3(int, int, 0, 0,
                         diag_cm_get_unit_by_id,
                         uint16,uint16,dev_id,0,0,
                         int,int,occur,0,0,
                         int*,int,unit,1,0);

CINT_FWRAPPER_CREATE_RP4(int, int, 0, 0,
                         diag_mem_get,
                         int,int,unit,0,0,
                         char*,char,name,1,0,
                         int,int,start,0,0,
                         uint32*,uint32,value,1,0);

CINT_FWRAPPER_CREATE_RP5(int, int, 0, 0,
                         diag_mem_field_get,
                         int,int,unit,0,0,
                         char*,char,name,1,0,
                         char*,char,field_name,1,0,
                         int,int,start,0,0,
                         uint32*,uint32,value,1,0);

CINT_FWRAPPER_CREATE_RP5(int, int, 0, 0,
                         diag_mem_set,
                         int,int,unit,0,0,
                         char*,char,name,1,0,
                         int,int,start,0,0,
                         int,int,count,0,0,
                         uint32*,uint32,value,1,0);

CINT_FWRAPPER_CREATE_RP6(int, int, 0, 0,
                         diag_mem_field_set,
                         int,int,unit,0,0,
                         char*,char,name,1,0,
                         char*,char,field_name,1,0,
                         int,int,start,0,0,
                         int,int,count,0,0,
                         uint32*,uint32,value,1,0);


CINT_FWRAPPER_CREATE_RP5(int, int, 0, 0,
                         diag_soc_mem_read_range,
                         int,int,unit,0,0,
                         char*,char,name,1,0,
                         int,int,index1,0,0,
                         int,int,index2,0,0,
                         uint32*,uint32,value,1,0);

CINT_FWRAPPER_CREATE_RP5(int, int, 0, 0,
                         diag_soc_mem_write_range,
                         int,int,unit,0,0,
                         char*,char,name,1,0,
                         int,int,index1,0,0,
                         int,int,index2,0,0,
                         uint32*,uint32,value,1,0);

CINT_FWRAPPER_CREATE_RP3(int, int, 0, 0,
                         diag_fill_table,
                         int,int,unit,0,0,
                         char*,char,name,1,0,
                         uint32*,uint32,value,1,0);

CINT_FWRAPPER_CREATE_RP2(int, int, 0, 0,
                         diag_mbist,
                         int,int,unit,0,0,
                         int,int,skip_errors,0,0);

CINT_FWRAPPER_CREATE_RP2(int, int, 0, 0,
                         diag_modport2sysport,
                         int,int,unit,0,0,
                         int,int,skip_errors,0,0);

#ifdef BCM_88650_A0
CINT_FWRAPPER_CREATE_RP2(int, int, 0, 0,
                         diag_test_mc,
                         int,int,unit,0,0,
                         char*,char,test_name,1,0);
CINT_FWRAPPER_CREATE_RP3(int, int, 0, 0,
                         diag_test_mc2,
                         int,int,unit,0,0,
                         char*,char,test_name,1,0,
                         int,int,param,0,0);
#endif /* BCM_88650_A0 */

CINT_FWRAPPER_CREATE_RP5(int, int, 0, 0,
                         soc_direct_reg_get,
                         int,int,unit,0,0,
                         int,int,cmic_block,0,0,
                         uint32,uint32,addr,0,0,
                         uint32,uint32,dwc_read,0,0,
                         uint32*,uint32,data,1,0);

CINT_FWRAPPER_CREATE_RP5(int, int, 0, 0,
                         soc_direct_reg_set,
                         int,int,unit,0,0,
                         int,int,cmic_block,0,0,
                         uint32,uint32,addr,0,0,
                         uint32,uint32,dwc_write,0,0,
                         uint32*,uint32,data,1,0);

#endif /*defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) */

#if defined(INCLUDE_I2C) && defined (__DUNE_GTO_BCM_CPU__)
CINT_FWRAPPER_CREATE_RP4(int, int, 0, 0,
                         cpu_i2c_write,
                         int, int, chip, 0, 0,
                         int, int, addr, 0, 0,
                         CPU_I2C_BUS_LEN, CPU_I2C_BUS_LEN, alen, 0, 0,
                         int, int, val, 0, 0);

CINT_FWRAPPER_CREATE_RP4(int, int, 0, 0,
                         cpu_i2c_read,
                         int, int, chip, 0, 0,
                         int, int, addr, 0, 0,
                         CPU_I2C_BUS_LEN, CPU_I2C_BUS_LEN, alen, 0, 0,
                         int*, int, p_val, 1, 0);
#endif

CINT_FWRAPPER_CREATE_VP1(diag_printk,
                         char*, char, str, 1, 0);

CINT_FWRAPPER_CREATE_VP2(diag_printk_int,
                         int, int, num, 0, 0,
                         int, int, flags, 0, 0);

CINT_FWRAPPER_CREATE_VP4(diag_pcie_read,
                         int,int,unit,0,0,
                         uint32,uint32,addr,0,0,
                         uint32*,uint32,val,1,0,
                         int,int,swap,0,0);

CINT_FWRAPPER_CREATE_VP4(diag_pcie_write,
                         int,int,unit,0,0,
                         uint32,uint32,addr,0,0,
                         uint32,uint32,val,0,0,
                         int,int,swap,0,0);


static cint_function_t __cint_diag_functions[] =
{
#if (defined(BCM_ARAD_SUPPORT) || defined(BCM_DFE_SUPPORT)) && defined(INCLUDE_INTR)
    CINT_FWRAPPER_ENTRY(rx_los_dump),
    CINT_FWRAPPER_ENTRY(rx_los_dump_port),
    CINT_FWRAPPER_ENTRY(rx_los_handle),
    CINT_FWRAPPER_ENTRY(rx_los_port_enable),
    CINT_FWRAPPER_ENTRY(rx_los_port_stable),
    CINT_FWRAPPER_ENTRY(rx_los_unit_attach),
    CINT_FWRAPPER_ENTRY(rx_los_unit_detach),
    CINT_FWRAPPER_ENTRY(rx_los_exit_thread),
    CINT_FWRAPPER_ENTRY(rx_los_register),
#endif
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    CINT_FWRAPPER_ENTRY(diag_reg_get),
    CINT_FWRAPPER_ENTRY(diag_reg_field_get),
    CINT_FWRAPPER_ENTRY(diag_reg_set),
    CINT_FWRAPPER_ENTRY(diag_reg_field_set),
    CINT_FWRAPPER_ENTRY(diag_mem_get),
    CINT_FWRAPPER_ENTRY(diag_mem_field_get),
    CINT_FWRAPPER_ENTRY(diag_mem_set),
    CINT_FWRAPPER_ENTRY(diag_mem_field_set),
    CINT_FWRAPPER_ENTRY(diag_soc_mem_read_range),
    CINT_FWRAPPER_ENTRY(diag_soc_mem_write_range),
    CINT_FWRAPPER_ENTRY(diag_fill_table),
    CINT_FWRAPPER_ENTRY(diag_mbist),
    CINT_FWRAPPER_ENTRY(diag_modport2sysport),
#ifdef BCM_88650_A0
    CINT_FWRAPPER_ENTRY(diag_test_mc),
    CINT_FWRAPPER_ENTRY(diag_test_mc2),
#endif /* BCM_88650_A0 */
    CINT_FWRAPPER_ENTRY(soc_direct_reg_get),
    CINT_FWRAPPER_ENTRY(soc_direct_reg_set),
#endif /*defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) */

#if defined(INCLUDE_I2C) && defined (__DUNE_GTO_BCM_CPU__)
    CINT_FWRAPPER_ENTRY(cpu_i2c_write),
    CINT_FWRAPPER_ENTRY(cpu_i2c_read),
#endif
    CINT_FWRAPPER_ENTRY(diag_printk),
    CINT_FWRAPPER_ENTRY(diag_printk_int),
    CINT_FWRAPPER_ENTRY(diag_pcie_read),
    CINT_FWRAPPER_ENTRY(diag_pcie_write),
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    CINT_FWRAPPER_ENTRY(diag_soc_device_reset),
    CINT_FWRAPPER_ENTRY(diag_cm_get_unit_by_id),
#endif

    CINT_ENTRY_LAST
};

static cint_parameter_desc_t __cint_typedefs[] =
{
    {
        "uint32",
        "reg_val",
        0,
        SOC_REG_ABOVE_64_MAX_SIZE_U32
    },
    {
        "uint32",
        "mem_val",
        0,
        SOC_MAX_MEM_WORDS
    },
    {NULL}
};

#if defined(INCLUDE_I2C) && defined (__DUNE_GTO_BCM_CPU__)
static cint_enum_map_t __cint_diag_enum_map__CPU_I2C_BUS_LEN[] =
{
  { "CPU_I2C_ALEN_NONE_DLEN_BYTE", CPU_I2C_ALEN_NONE_DLEN_BYTE },
  { "CPU_I2C_ALEN_NONE_DLEN_WORD", CPU_I2C_ALEN_NONE_DLEN_WORD },
  { "CPU_I2C_ALEN_NONE_DLEN_LONG", CPU_I2C_ALEN_NONE_DLEN_LONG },
  { "CPU_I2C_ALEN_BYTE_DLEN_BYTE", CPU_I2C_ALEN_BYTE_DLEN_BYTE },
  { "CPU_I2C_ALEN_BYTE_DLEN_WORD", CPU_I2C_ALEN_BYTE_DLEN_WORD },
  { "CPU_I2C_ALEN_WORD_DLEN_WORD", CPU_I2C_ALEN_WORD_DLEN_WORD },
  { "CPU_I2C_ALEN_BYTE_DLEN_LONG", CPU_I2C_ALEN_BYTE_DLEN_LONG },
  { "CPU_I2C_ALEN_WORD_DLEN_LONG", CPU_I2C_ALEN_WORD_DLEN_LONG },
  { "CPU_I2C_ALEN_LONG_DLEN_LONG", CPU_I2C_ALEN_LONG_DLEN_LONG },
  { NULL }
};
#endif

#if (defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INTR)
static cint_enum_map_t __cint_diag_enum_map__rx_los_state_t[] =
{
  { "rx_los_state_ideal_state",     rx_los_state_ideal_state },
  { "rx_los_state_no_signal",       rx_los_state_no_signal },
  { "rx_los_state_no_signal_active",rx_los_state_no_signal_active },
  { "rx_los_state_rx_seq_change",   rx_los_state_rx_seq_change },
  { "rx_los_state_sleep_one",       rx_los_state_sleep_one },
  { "rx_los_state_restart",         rx_los_state_restart },
  { "rx_los_state_sleep_two",       rx_los_state_sleep_two },
  { "rx_los_state_link_up_check",   rx_los_state_link_up_check },
  { "rx_los_state_long_sleep",      rx_los_state_long_sleep },
  { "rx_los_states_count",          rx_los_states_count },
  { NULL }
};
#endif /*(defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INTR)*/

static cint_enum_type_t __cint_diag_enums[] =
{
#if defined(INCLUDE_I2C) && defined (__DUNE_GTO_BCM_CPU__)
  { "CPU_I2C_BUS_LEN", __cint_diag_enum_map__CPU_I2C_BUS_LEN },
#endif
#if (defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INTR)
  { "rx_los_state_t", __cint_diag_enum_map__rx_los_state_t },
#endif /*defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INTR)*/
  { NULL }
};

static cint_constants_t __cint_diag_constants[] =
{
   { "DIAG_PRINTK_FLAGS_HEX", DIAG_PRINTK_FLAGS_HEX },
   { NULL }
};

static cint_function_pointer_t __cint_diag_function_pointers[2];

#if (defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INTR)
static int
__cint_fpointer__rx_los_callback_t(int unit,
                                   bcm_port_t port,
                                   rx_los_state_t state)
{
    int returnVal;
    cint_interpreter_callback(__cint_diag_function_pointers+0,
                              3,
                              0,
                              &unit,
                              &port,
                              &state,
                              &returnVal);
    return returnVal;
}

static cint_parameter_desc_t __cint_parameters__rx_los_callback_t[] =
{
    {
        "void"
        "",
        0,
        0
    },
    {
        "int",
        "unit",
        0,
        0
    },
    {
        "bcm_port_t",
        "port",
        0,
        0
    },
    {
        "rx_los_state_t",
        "state",
        0,
        0
    },
    CINT_ENTRY_LAST
};
#endif /*(defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INTR)*/
static cint_function_pointer_t __cint_diag_function_pointers[] =
{
#if (defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INTR)
    {
        "rx_los_callback_t",
        (cint_fpointer_t) __cint_fpointer__rx_los_callback_t,
        __cint_parameters__rx_los_callback_t
    },
#endif /*(defined(BCM_DFE_SUPPORT) || defined(BCM_ARAD_SUPPORT)) && defined(INCLUDE_INTR)*/
    CINT_ENTRY_LAST
};

cint_data_t diag_cint_data =
{
    NULL,
    __cint_diag_functions,
    NULL,
    __cint_diag_enums,
    __cint_typedefs,
    __cint_diag_constants,
    __cint_diag_function_pointers
};


#endif /* INCLUDE_LIB_CINT*/

