/*
 * $Id: bcmx.h,v 1.15 Broadcom SDK $
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
 * File:        bcmx.h
 * Purpose:     
 */

#ifndef   _DIAG_BCMX_H_
#define   _DIAG_BCMX_H_

#include <bcmx/lport.h>
#include <bcmx/l2.h>

#include <bcmx/lplist.h>
#include <bcmx/bcmx.h>
#include <appl/diag/diag.h>

#define BCMX_UNIT_ITER(unit, i)                                           \
    for (i = 0, unit = bcmx_unit_list[0];                                 \
         i < bcmx_unit_count;                                             \
         unit = bcmx_unit_list[++i])

extern int bcmx_cmd_cnt;
extern cmd_t bcmx_cmd_list[];
extern int bcmx_unit_list[];

/*
 * Uport
 */
#define BCMX_UPORT_STR_LENGTH_MIN      12
#define BCMX_UPORT_STR_LENGTH_DEFAULT  BCMX_UPORT_STR_LENGTH_MIN

extern char * bcmx_lport_to_uport_str(bcmx_lport_t lport);

typedef void (*bcmx_uport_to_str_f)(bcmx_uport_t uport, char *buf, int len);
extern bcmx_uport_to_str_f bcmx_uport_to_str;

typedef bcmx_uport_t (*bcmx_uport_parse_f)(char *buf, char **end);
extern bcmx_uport_parse_f bcmx_uport_parse;


/* In bcmxdiag/util.c */
extern int bcmx_lplist_parse(bcmx_lplist_t *list, char *buf);

#if defined(INCLUDE_ACL)
extern cmd_result_t bcmx_cmd_acl(int unit, args_t *args);
extern char         bcmx_cmd_acl_usage[];
#endif /* INCLUDE_ACL */

#ifdef BCM_FIELD_SUPPORT
extern cmd_result_t bcmx_cmd_field(int unit, args_t *args);
extern char         bcmx_cmd_field_usage[];
#endif

extern cmd_result_t bcmx_cmd_ipmc(int unit, args_t *args);
extern char         bcmx_cmd_ipmc_usage[];

extern cmd_result_t bcmx_cmd_l2(int unit, args_t *args);
extern char         bcmx_cmd_l2_usage[];

extern void         bcmx_dump_l2_addr(char *pfx, bcmx_l2_addr_t *l2addr,
                                      bcmx_lplist_t *port_block);

extern void         bcmx_dump_l2_cache_addr(char *pfx, bcmx_l2_cache_addr_t *l2addr);

extern cmd_result_t bcmx_cmd_l3(int unit, args_t *args);
extern char         bcmx_cmd_l3_usage[];

extern cmd_result_t bcmx_cmd_mcast(int unit, args_t *args);
extern char         bcmx_cmd_mcast_usage[];

extern cmd_result_t bcmx_cmd_mirror(int unit, args_t *args);
extern char         bcmx_cmd_mirror_usage[];

extern cmd_result_t bcmx_cmd_port(int unit, args_t *args);
extern char         bcmx_cmd_port_usage[];

extern cmd_result_t bcmx_cmd_stat(int unit, args_t *args);
extern char         bcmx_cmd_stat_usage[];

extern cmd_result_t bcmx_cmd_trunk(int unit, args_t *args);
extern char         bcmx_cmd_trunk_usage[];

extern cmd_result_t bcmx_cmd_vlan(int unit, args_t *args);
extern char         bcmx_cmd_vlan_usage[];

extern cmd_result_t bcmx_cmd_stg(int unit, args_t *args);
extern char         bcmx_cmd_stg_usage[];

extern cmd_result_t bcmx_cmd_rate(int unit, args_t *args);
extern char         bcmx_cmd_rate_usage[];

extern cmd_result_t bcmx_cmd_link(int unit, args_t *args);
extern char         bcmx_cmd_link_usage[];

extern char         bcmx_cmd_tx_usage[];
extern cmd_result_t bcmx_cmd_tx(int unit, args_t *args);

extern char         bcmx_cmd_rx_usage[];
extern cmd_result_t bcmx_cmd_rx(int unit, args_t *args);

#if 0

typedef int user_port_t;

/* For now, we support this many boards in sample configurations */
#define BCMX_BOARDS_MAX 4

typedef struct bcmx_config_desc_s {
    int num_entries;
    int stk_mode;                /* How should boards be added? */
    bcmx_cfg_params_t *params;
    bcmx_system_trx_t *trx_funs;    /* Translation functions (see above) */
    bcmx_dest_route_get_f dest_route_get;
    char *name;
    bcmx_cpu_info_t cpu_info[BCMX_BOARDS_MAX];
    int exit_units[BCMX_BOARDS_MAX];   /* How to get off board */
    int exit_ports[BCMX_BOARDS_MAX];
} bcmx_config_desc_t;

extern bcmx_config_desc_t bcmx_configs[];
extern int CFG_COUNT;

extern bcmx_lport_t general_uport_str_to_lport(char *uport_str, char **end);
extern bcmx_lport_t general_lport_next(bcmx_lport_t lport);

/* Commands */

extern cmd_result_t bcmx_cmd_mcpu_setup(int unit, args_t *args);
extern char bcmx_cmd_mcpu_setup_usage[];
extern cmd_result_t bcmx_cmd_mcpu_start(int unit, args_t *args);
extern char bcmx_cmd_mcpu_start_usage[];

/* RDP related functions */
extern int bcmx_rdp_client_tx(int cpuid, char *buf, int buf_len);
extern void bcmx_rdp_client_rx(uint32 cid, void *data, int length, int s_cpu);
extern int bcmx_rdp_server_tx(int cpu, char *buf, int buf_len);
extern void bcmx_rdp_server_rx(uint32 clientid, void *data, int length,
                                int s_cpu);
extern int bcmx_rdp_send(int cpuid, char *buf, int buf_len, int data_len);

/* BCMX in a stack */
extern void stack_simplex_bcmx_init(void);
extern void stack_duplex_bcmx_init(void);
#endif

#endif /* _DIAG_BCMX_H_ */
