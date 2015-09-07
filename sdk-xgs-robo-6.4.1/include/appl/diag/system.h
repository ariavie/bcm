/*
 * $Id: system.h,v 1.63 Broadcom SDK $
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
 * This header file defines important constants and structures for
 * use with the SOC Diagnostics Shell which is built upon the SOC
 * driver and SAL (System Abstraction Layer)
 */

#ifndef _DIAG_SYSTEM_H
#define _DIAG_SYSTEM_H

#ifndef __KERNEL__
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#else
#include <sal/core/libc.h>
#endif

#include <soc/drv.h>
#include <soc/debug.h>

#include <sal/core/thread.h>

#include <bcm/types.h>

#include <appl/diag/cmdlist.h>
#include <sal/appl/sal.h>

#include <bcm/l2.h>
#include <bcm/l3.h>
#include <bcm/tunnel.h>

#include <bcm/field.h>
#include <bcm/switch.h>

/*
 * Define:      SOC_SCRIPT_EXT
 * Purpose:     Define standard BCM script file name extension
 */
#ifndef SOC_SCRIPT_EXT
#define SOC_SCRIPT_EXT  ".soc"
#endif

/*
 * Define:      SOC_INIT_RC
 * Purpose:     Defines a default run command/init file to load on startup.
 *              This is read from the default directory, which is commonly
 *              the same as where the image was booted from.
 *              This can be overridden globally by defining SOC_INIT_RC
 *              on the compile line, or on a per-unit basis by calling
 *              diag_rc_set() before diag_shell().
 */
#ifndef SOC_INIT_RC
#define SOC_INIT_RC     "rc.soc"
#endif

/*
 * Register types we understand
 */

typedef struct regtype_entry_t {
    char *name;         /* Name of register for setreg/getreg commands */
    soc_regtype_t type; /* Type of register (e.g. soc_cpureg) */
    char *help;         /* Description of register for help command */
} regtype_entry_t;

extern regtype_entry_t *regtype_lookup_name(char *str);
extern void regtype_print_all(void);

#if defined(BCM_ROBO_SUPPORT)
extern regtype_entry_t *robo_regtype_lookup_name(char *str);
extern void robo_regtype_print_all(void);
#endif

/*
 * Diagnostic shell utilities: diag/util.c
 */

#define FORMAT_PBMP_MAX   148   /* "0xdeadbeefdeadbeef; fe0,fe2,....ge2,hg" */
#define FORMAT_GPORT_MAX  128

 extern int     i2xdigit(int digit);
 extern int     xdigit2i(int digit);
 extern int     isint(char *);
 extern void    tabify_line(char *out, char *in, int tab_width);
 extern sal_vaddr_t parse_address(char *str);
 extern uint32  parse_integer(char *str);
 extern int     parse_small_integers(int unit, char *s, soc_pbmp_t *pbmp);
 extern int     diag_parse_range(char *low, char *high,
                         int *min, int *max,
                         int legal_min, int legal_max);
 extern void    parse_long_integer(uint32 *val, int nval, char *str);
 extern void    format_long_integer(char *buf, uint32 *val, int nval);
 extern void    format_spec_long_integer(char *buf, uint32 *val, int nval);
 extern void    add_long_integer(uint32 *dst, uint32 *src, int nval);
 extern void    sub_long_integer(uint32 *dst, uint32 *src, int nval);
 extern void    neg_long_integer(uint32 *dst, int nval);
 extern void    format_uint64(char *buf, uint64 n);
 extern void    format_uint64_decimal(char *buf, uint64 n, int comma);
 extern uint64  parse_uint64(char *str);
 extern int     parse_macaddr(char *str, sal_mac_addr_t macaddr);
 extern void    format_macaddr(char buf[MACADDR_STR_LEN], sal_mac_addr_t macaddr);
 extern void    increment_macaddr(sal_mac_addr_t macaddr, int amount);
 extern int     parse_ipaddr(char *str, ip_addr_t *ipaddr);
 extern int     parse_ip6addr(char *str, ip6_addr_t ipaddr);
 extern void    increment_ip6addr(ip6_addr_t ipaddr, uint32 byte_pos, int amount);
 extern void    format_ipaddr(char buf[IPADDR_STR_LEN], ip_addr_t ipaddr);
 extern void    format_ipaddr_mask(char *buf, ip_addr_t ipaddr, ip_addr_t mask);
 extern void    format_ip6addr(char buf[IP6ADDR_STR_LEN], ip6_addr_t ipaddr);
 extern int     parse_pbmp(int unit, char *s, soc_pbmp_t *pbmp);
 extern int     parse_pp_pbmp(int unit, char *s, soc_pbmp_t *pbmp);
 extern int     parse_bcm_pbmp(int unit, char *s, bcm_pbmp_t *pbmp);
 extern int     parse_pbmp_no_error(int unit, char *s, soc_pbmp_t *pbmp);
 extern char   *format_pbmp(int unit, char *buf, int bufsize, soc_pbmp_t pbmp);
 extern char   *format_bcm_pbmp(int unit, char *buf, int bufsize, bcm_pbmp_t pbmp);
 extern char   *bcm_port_name(int unit, int port);
 extern char   *mod_port_name(int unit, int modid, int port);
 extern int     parse_port(int unit, char *s, soc_port_t *portp);
 extern char   *format_gport(char *buf, bcm_gport_t gport);
 extern int     parse_gport(int unit, char *s, soc_port_t *portp);
 extern int     parse_bcm_port(int unit, char *s, bcm_port_t *portp);
 extern int     parse_mod_port(int unit, char *s, bcm_mod_port_t *mp);
 extern bcm_field_qualify_t parse_field_qualifier(char *str);
 extern char   *format_field_qualifier(char *buf, bcm_field_qualify_t qual,
                                       int brief);
 extern int     parse_field_qset(char *str, bcm_field_qset_t *qset);
 extern char   *format_field_qset(char *buf, bcm_field_qset_t qset,
                                  char *separator);
 extern bcm_field_group_mode_t parse_field_group_mode(char *str);
 extern char   *format_field_group_mode(char *buf, bcm_field_group_mode_t mode,
                                        int brief);
 extern bcm_field_action_t parse_field_action(char *str);
 extern char   *format_field_action(char *buf, bcm_field_action_t action,
                                    int brief);
 extern int     parse_field_aset(char *str, bcm_field_aset_t *aset, uint8 add);
 extern char   *format_field_aset(char *buf, bcm_field_aset_t aset,
                                  char *separator);

 extern bcm_field_decap_t parse_field_decap(char *str);
 extern char   *format_field_decap(char *buf, bcm_field_decap_t decap);
 extern int     parse_cos_range(int unit, char *s, soc_cos_t *min, soc_cos_t *max);
 extern int     parse_block_range(int unit, soc_block_types_t regtype, char *s,
                          int *min, int *max, soc_block_types_t mask);
 extern int     parse_memory_name(int unit, soc_mem_t *result, char *str, int *copyno, unsigned *array_index);
 extern int     parse_memory_index(int unit, soc_mem_t mem, char *val_str);
 extern unsigned parse_memory_array_index(int unit, soc_mem_t mem, char *val_str);
 extern int     parse_cmic_regname(int unit, char *name, uint32 *offset_ptr);
 extern int     parse_port_mode(char *str, soc_port_mode_t *mode);
 extern int     parse_port_ability(char *str, soc_port_ability_t *mode);
 extern void    format_port_mode(char *buf, int bufsize,
                         soc_port_mode_t mode, int abbrev);
 extern bcm_switch_control_t parse_switch_control(char *str);
 extern char   *format_system_control(bcm_switch_control_t control);

 extern pbmp_t   soc_property_get_bcm_pbmp(int unit, const char *name, int defneg);
 extern uint32   soc_property_bcm_port_get(int unit, bcm_port_t aport, const char *name, uint32 defl);

 extern int     parse_phy_control_longreach_ability(char *str, 
                         soc_phy_control_longreach_ability_t *ability);
 extern void    format_phy_control_longreach_ability(char *buf, int bufsize, 
                         soc_phy_control_longreach_ability_t ability);


#ifdef INCLUDE_L3
 extern bcm_tunnel_type_t parse_tunnel_type(char *str);
#endif /* INCLUDE_L3 */
#ifdef __KERNEL__
#include <linux/kernel.h>
 extern char *strtok(char *s, const char *delim);
#endif
 extern const char *strcaseindex(const char *s, const char *sub);
 extern char    *strrepl(char *s, int start, int len, const char *repstr);

 extern uint32  packet_load(uint8 *addr, int size);
 extern void    packet_random_store(uint8 *buf, int size);
 extern uint32  packet_store(uint8 *buf, int size, uint32 val, uint32 inc);
 extern int     packet_compare(uint8 *p1, uint8 *p2, int size);
 extern void    dump_l2_addr(int unit, char *pfx, bcm_l2_addr_t *l2addr);
 extern void    dump_l2_cache_addr(int unit, char *pfx, bcm_l2_cache_addr_t *l2caddr);


#define BCM_PORT_NAME(_u,_p) bcm_port_name(_u,_p)

/*
 * A couple of routines in reg.c
 */

#define REG_PRINT_HEX           0x01
#define REG_PRINT_RAW           0x02
#define REG_PRINT_DO_PRINT      0x04
#define REG_PRINT_CHG           0x08
#define REG_PRINT_ADDR          0x10

 extern void reg_print(int unit, soc_regaddrinfo_t *ainfo, uint64 val, uint32 flags,
               char *fld_sep, int wrap);
 extern int reg_print_all(int unit, soc_regaddrlist_t *alist, uint32 flags);
#if defined(BCM_ROBO_SUPPORT)
 extern int reg_set_all(int unit, soc_regaddrlist_t *alist, uint64 value);
 extern void robo_reg_print(int unit, soc_regaddrinfo_t *ainfo, uint64 val, uint32 flags,
               const char *fld_sep, int wrap);
 extern int robo_reg_print_all(int unit, soc_regaddrlist_t *alist, uint32 flags);

#endif /* BCM_ROBO_SUPPORT */

/*
 * symtab.c
 */
 extern void init_symtab(void);

 extern int parse_symbolic_reference(int unit, soc_regaddrlist_t *alist, char *ref);

/*
 * reg_skip.c
 */
 extern int reg_mask_subset(int unit, soc_regaddrinfo_t *ainfo, uint64 *mask);
 extern int rval_test_skip_reg(int unit, soc_regaddrinfo_t *ainfo);

/* 
 * Shell routines: internal, used for diagnostics shell.
 */

 extern void    diag_init(void);
 extern void    diag_rc_set(int unit, const char *fname);
 extern void    diag_rc_get(int unit, const char **fname);
 extern int     diag_rc_load(int unit);
 extern void    diag_shell(void);

 extern int     parseEndOk(args_t *, parse_table_t *, cmd_result_t *);

#if defined(INCLUDE_EDITLINE)
 extern int diag_list_possib(char *regpref, char ***avp);
 extern char *diag_complete(char *regpref, int *unique);
#endif

#if defined (BCM_ESW_SUPPORT) ||  defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT)
 extern int bist_args_to_mems(int unit, args_t *a, soc_mem_t *mems, int *num_mems);
#endif

/* Packet Watcher */

 extern int pw_running(int unit);
 extern cmd_result_t pw_stop(int unit, int sync);

/* Build Information */

extern char *_build_release;
extern char *_build_user;
extern char *_build_host;
extern char *_build_date;
extern char *_build_datestamp;
extern char *_build_tree;
extern char *_build_arch;
extern char *_build_os;

#define UNSUPPORTED_COMMAND(unit, chiptype, a)\
    if (SOC_CHIP_GROUP(unit) == chiptype) { \
        bsl_error(BSL_APPL_SHELL, unit,                 \
                  "%s: command unsupported on %s\n",    \
                  ARG_CMD(a), soc_dev_name(unit));      \
        ARG_DISCARD(a);\
        return CMD_OK; /* Don't error out, or scripts may fail */\
    }

/* counter.c */

#define SHOW_CTR_CHANGED        0x01
#define SHOW_CTR_SAME           0x02
#define SHOW_CTR_Z              0x04
#define SHOW_CTR_NZ             0x08
#define SHOW_CTR_HEX            0x10
#define SHOW_CTR_RAW            0x20
#define SHOW_CTR_ED             0x40
#define SHOW_CTR_MS             0x80
#define SHOW_TOTAL              0x100
#define SHOW_SELECTED_PORTS     0x200
#define SHOW_FABRIC             0x400
#define SHOW_NIF                0x800
#define SHOW_COMPACT            0x1000
#define SHOW_FULL               0x2000

#define SHOW_INTR_Z             0x01
#define SHOW_INTR_MASK          0x02
#define SHOW_INTR_UNMASK        0x04
#define SHOW_INTR_ALL           0x08

 extern void    do_show_counter(int unit, soc_port_t port, soc_reg_t ctr_reg,
                        int ar_idx, int flags);
 extern int     do_show_counters(int unit, soc_reg_t ctr_reg, soc_pbmp_t pbmp,
                         int flags);


 extern int     do_resync_counters(int unit, soc_pbmp_t pbmp);

 extern void    counter_val_set_by_port(int unit, soc_pbmp_t pbmp, uint64 val);

#if defined(BCM_SBX_SUPPORT)
 extern void    sbx_do_show_counter(int unit, soc_port_t port, soc_reg_t ctr_reg,
                        int ar_idx, int flags);

 extern int     sbx_do_show_counters(int unit, soc_reg_t ctr_reg, pbmp_t pbmp,
                         int flags);

 extern int     sbx_do_resync_counters(int unit, pbmp_t pbmp);

 extern void    sbx_counter_val_set_by_port(int unit, pbmp_t pbmp, uint64 val);
#endif

 extern int     system_init(int unit);
 extern int     system_shutdown(int unit, int cleanup);

#if defined(BCM_ROBO_SUPPORT)
 extern void    robo_do_show_counter(int unit, soc_port_t port, soc_reg_t ctr_reg,
                        int ar_idx, int flags);
 extern int     robo_do_show_counters(int unit, soc_reg_t ctr_reg, pbmp_t pbmp,
                         int flags);


 extern int     robo_do_resync_counters(int unit, pbmp_t pbmp);

 extern void    robo_counter_val_set_by_port(int unit, pbmp_t pbmp, uint64 val);
 
#endif

extern int sys_board_type_mng_get(void);  

#endif /* !_DIAG_SYSTEM_H */
