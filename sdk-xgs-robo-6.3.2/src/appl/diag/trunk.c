/*
 * $Id: trunk.c 1.22 Broadcom SDK $
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
 * Trunk CLI functions that are common for more than one chip architecture
 * (XGS, SBX, possibly Robo)
 */

#include <appl/diag/system.h>
#include <appl/diag/parse.h>

#include <bcm/error.h>
#include <bcm/trunk.h>
#include <bcm/port.h>
#ifdef BCM_ESW_SUPPORT
#include <bcm_int/esw/port.h>
#endif
#include <bcm/stack.h>
#include <bcm/debug.h>




static char *_pscnames[] = BCM_TRUNK_PSC_NAMES_INITIALIZER;

static cmd_result_t
_bcm_diag_trunk_show_range(int unit,
                           args_t *a,
                           int fp,
                           int first,
                           int last,
                           int *found)
{
    bcm_trunk_t tid;
    bcm_trunk_info_t t_add_info;
    pbmp_t pbmp;
    char const *s;
    int i;
    int rv;
    bcm_trunk_member_t *member_array = NULL;
    bcm_module_t tm[BCM_TRUNK_MAX_PORTCNT];
    bcm_port_t   tp[BCM_TRUNK_MAX_PORTCNT];
    int member_count;

    member_array = sal_alloc(sizeof(bcm_trunk_member_t) * BCM_TRUNK_MAX_PORTCNT,
            "member array");
    if (NULL == member_array) {
        printk("%s: failed: %s\n",
                ARG_CMD(a), bcm_errmsg(BCM_E_MEMORY));
        return CMD_FAIL;
    }

    sal_memset(tm, 0, sizeof(bcm_module_t)*BCM_TRUNK_MAX_PORTCNT);
    sal_memset(tp, 0, sizeof(bcm_port_t)*BCM_TRUNK_MAX_PORTCNT);

    for (tid = first; tid <= last; tid++) {
        rv = bcm_trunk_get(unit, tid, &t_add_info, BCM_TRUNK_MAX_PORTCNT,
                member_array, &member_count);

        if (rv == BCM_E_NOT_FOUND) {
            /* Only show existing trunk groups */
            continue;
        }

        if (rv < 0) {
            printk("%s: trunk %d get failed: %s\n",
                   ARG_CMD(a), tid, bcm_errmsg(rv));
            sal_free(member_array);
            return CMD_FAIL;
        }

        if (fp) {
            /* Front panel trunk */
            rv = bcm_trunk_egress_get(unit, tid, &pbmp);

            if (rv == BCM_E_UNAVAIL) {
                /*
                 * Certain devices don't have a egress port per trunk
                 * Instead it provides support to set this up on
                 * a per port basis for each trunk member port
                 * Ignore this on these devices.
                 */
                rv = BCM_E_NONE;
                SOC_PBMP_ASSIGN(pbmp, PBMP_ALL(unit));
            }
            if (rv < 0) {
                printk("%s: trunk %d egress get failed: %s\n",
                       ARG_CMD(a), tid, bcm_errmsg(rv));
                sal_free(member_array);
                return CMD_FAIL;
            }
        } else {
            /* Fabric trunk */
            
            SOC_PBMP_ASSIGN(pbmp, PBMP_ALL(unit));
        }

        (*found) += 1;
        printk("trunk %d: (%s, %d ports)", tid,
               fp ? "front panel" : "fabric",
               member_count);

        if (member_count > 0) {
#ifdef BCM_TRX_SUPPORT
            if (SOC_IS_TRX(unit)) {
                for (i = 0; i < member_count; i++) {
                    int tgid, id;
                    rv = _bcm_esw_gport_resolve(unit, member_array[i].gport,
                            &tm[i],
                            &tp[i],
                            &tgid, &id);
                    if ((rv < 0) || (tgid != -1) || (id != -1)) {
                        sal_free(member_array);
                        return CMD_FAIL;
                    }
                    rv = bcm_stk_modmap_map(unit, BCM_STK_MODMAP_SET,
                            tm[i], tp[i],
                            &tm[i], &tp[i]);
                    if (rv < 0) {
                        sal_free(member_array);
                        return CMD_FAIL;
                    }
                }
            }
#endif
            for (i = 0; i < member_count; i++) {
                printk("%s%s",
                       i == 0 ? "=" : ",",
                       mod_port_name(unit, tm[i], tp[i]));
            }

            s = t_add_info.dlf_index < 0 ? "any" :
                mod_port_name(unit,
                              tm[t_add_info.dlf_index],
                              tp[t_add_info.dlf_index]);
            printk(" dlf=%s", s);
            s = t_add_info.mc_index < 0 ? "any" :
                mod_port_name(unit,
                              tm[t_add_info.mc_index],
                              tp[t_add_info.mc_index]);
            printk(" mc=%s", s);
            s = t_add_info.ipmc_index < 0 ? "any" :
                mod_port_name(unit,
                              tm[t_add_info.ipmc_index],
                              tp[t_add_info.ipmc_index]);
            printk(" ipmc=%s", s);
            if ((t_add_info.psc & 0xf) <= 0 ||
                (t_add_info.psc & 0xf) >= COUNTOF(_pscnames)) {
                s= "unknown";
            } else {
                s = _pscnames[t_add_info.psc&0xf];
            }
            printk(" psc=%s", s);
#ifdef BCM_HERCULES15_SUPPORT
            if (SOC_IS_HERCULES15(unit)) {
                printk("%s%s%s%s%s%s%s%s%s%s%s%s",
                       (t_add_info.psc & BCM_TRUNK_PSC_IPMACSA) ?
                       "+ipmacsa" : "",
                       (t_add_info.psc & BCM_TRUNK_PSC_IPMACDA) ?
                       "+ipmacda" : "",
                       (t_add_info.psc & BCM_TRUNK_PSC_IPTYPE) ?
                       "+iptype" : "",
                       (t_add_info.psc & BCM_TRUNK_PSC_IPVID) ?
                       "+ipvid" : "",
                       (t_add_info.psc & BCM_TRUNK_PSC_IPSA) ?
                       "+ipsa" : "",
                       (t_add_info.psc & BCM_TRUNK_PSC_IPDA) ?
                       "+ipda" : "",
                       (t_add_info.psc & BCM_TRUNK_PSC_L4SS) ?
                       "+l4ss" : "",
                       (t_add_info.psc & BCM_TRUNK_PSC_L4DS) ?
                       "+l4ds" : "",
                       (t_add_info.psc & BCM_TRUNK_PSC_MACSA) ?
                       "+macsa" : "",
                       (t_add_info.psc & BCM_TRUNK_PSC_MACDA) ?
                       "+macda" : "",
                       (t_add_info.psc & BCM_TRUNK_PSC_TYPE) ?
                       "+type" : "",
                       (t_add_info.psc & BCM_TRUNK_PSC_VID) ?
                       "+vid" : "");
            }
#endif /* BCM_HERCULES15_SUPPORT */
            printk(" (0x%x)", t_add_info.psc);
        }
        printk("\n");

        if (SOC_PBMP_NEQ(pbmp, PBMP_ALL(unit))) {
            char	buf[FORMAT_PBMP_MAX];
            format_pbmp(unit, buf, sizeof (buf), pbmp);
            printk("trunk %d: egress ports=%s\n", tid, buf);
        }
    }

    sal_free(member_array);
    return CMD_OK;
}

cmd_result_t
_bcm_diag_trunk_show(int unit, args_t *a)
{
    cmd_result_t retCode = CMD_OK;
    int rv = BCM_E_FAIL;
    bcm_trunk_chip_info_t ti;
    parse_table_t pt;
    uint32 arg_tid = ~0;
    char const *c;
    int tid_min, tid_max, found;
    int tid_fp_min, tid_fp_max;
    int tid_fabric_min, tid_fabric_max;
    int num_fp_tids, num_fabric_tids, num_tids;
    int num_fp_ports, num_fabric_ports;

    if ((rv = bcm_trunk_chip_info_get(unit, &ti)) < 0) {
        printk("%s: %s\n", ARG_CMD(a), bcm_errmsg(rv));
        return CMD_FAIL;
    }

    num_fp_tids = ti.trunk_group_count;
    tid_fp_min = ti.trunk_id_min;
    tid_fp_max = ti.trunk_id_max;
    tid_fabric_min = ti.trunk_fabric_id_min;
    tid_fabric_max = ti.trunk_fabric_id_max;
    num_fp_ports = ti.trunk_ports_max;
    num_fabric_ports = ti.trunk_fabric_ports_max;

    if (tid_fabric_min >= 0) {
        num_fabric_tids = tid_fabric_max - tid_fabric_min + 1;
    } else {
        num_fabric_tids = 0;
    }

    num_tids = num_fp_tids + num_fabric_tids;

    if (num_tids) {
        printk("Device supports %d trunk groups:\n", num_tids);
        if (num_fp_tids > 0) {
            printk("  %d front panel trunks (%d..%d), %d ports/trunk\n",
                   num_fp_tids,
                   tid_fp_min,
                   tid_fp_max,
                   num_fp_ports);
        }
        if (num_fabric_tids > 0) {
            printk("  %d fabric trunks (%d..%d), %d ports/trunk\n",
                   num_fabric_tids,
                   tid_fabric_min,
                   tid_fabric_max,
                   num_fabric_ports);
        }
    }

    if ((c = ARG_CUR(a)) != NULL) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Id", PQ_DFL|PQ_INT, 0, &arg_tid, NULL);
        if (!parseEndOk( a, &pt, &retCode)) {
            return retCode;
        }
        if ((~0) == arg_tid) {
            /*
             *  We use ~0 as a wildcard internally, but we've been asked to
             *  match ID literally -1 (which happens to look the same).  Choose
             *  another invalid ID instead.
             */
            arg_tid = ~1;
        }
    }

    found = 0;
    if (((~0) == arg_tid) || ((tid_fp_min <= arg_tid) &&
                              (tid_fp_max >= arg_tid))) {
        if ((~0) != arg_tid) {
            tid_min = arg_tid;
            tid_max = arg_tid;
        } else {
            tid_min = tid_fp_min;
            tid_max = tid_fp_max;
        }
        retCode = _bcm_diag_trunk_show_range(unit,
                                             a,
                                             TRUE /* front panel */,
                                             tid_min,
                                             tid_max,
                                             &found);
    }

    if ((tid_fabric_min >= 0) &&
        (((~0) == arg_tid) || ((tid_fabric_min <= arg_tid) &&
                               (tid_fabric_max >= arg_tid)))) {
        if ((~0) != arg_tid) {
            tid_min = arg_tid;
            tid_max = arg_tid;
        } else {
            tid_min = tid_fabric_min;
            tid_max = tid_fabric_max;
        }
        retCode = _bcm_diag_trunk_show_range(unit,
                                             a,
                                             FALSE /* not front panel */,
                                             tid_min,
                                             tid_max,
                                             &found);
    }

    if (found == 0) {
        printk("[no matching trunks defined]\n");
    }

    return retCode;
}

