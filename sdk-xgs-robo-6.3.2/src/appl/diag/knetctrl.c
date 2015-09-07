/*
 * $Id: knetctrl.c 1.16 Broadcom SDK $
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
 * File:        knetctrl.c
 * Purpose:     Kernel network control
 * Requires:    
 */


#ifdef INCLUDE_KNET

#include <sal/appl/sal.h>

#include <soc/knet.h>

#include <bcm/knet.h>

#include <appl/diag/shell.h>
#include <appl/diag/parse.h>
#include <appl/diag/system.h>

char cmd_knet_ctrl_usage[] =
    "Usages:\n"
#ifdef COMPILER_STRING_CONST_LIMIT
    "  knetctrl <subcmd> [options]\n"
#else
    "  knetctrl netif create [Vlan=<vlan>] [Port=<port>] [RCPU=yes|no]\n"
    "        Create a virtual network interface.\n"
    "        Supported options:\n"
    "          [AddTag=yes|no]      - Add tag before sending to switch port\n"
    "          [IFName=<str>]       - Optional network device name\n"
    "  knetctrl netif destroy <id>\n"
    "        Destroy a virtual network interface.\n"
    "  knetctrl netif show\n"
    "        Show virtual network interfaces.\n"
    "  knetctrl filter create DestType=Null|NetIF|RxAPI [DestID=<id>] [options]\n"
    "        Create a packet filter.\n"
    "        DestID is required for DestType=NetIF only.\n"
    "        Supported options:\n"
    "          [DESCription=<str>]  - Optional filter description\n"
    "          [PRIOrity=<prio>]    - Filter match order priority (0 is highest)\n"
    "          [StripTag=yes|no]    - Strip VLAN tag before sending packet to NetIF\n"
    "          [Mirror=yes|no]      - Copy packet to RxAPI (if DestType=NetIF)\n"
    "          [IngPort=<port>]     - Match specified local ingress port\n"
    "          [SrcPort=<port>]     - Match specified module port\n"
    "          [SrcModid=<modid>]   - Match specified module ID\n"
    "          [Reason=<reason>]    - Match specified reason for copy to CPU\n"
    "          [FPRule=<rule>]      - Match specified FP rule number (if FP match)\n"
    "          [PktOffset=<offet>]  - Match packet data starting at this offset\n"
    "          [PktByteX=<val>]     - Match packet data byte X (X=[0..7] supported)\n"
    "          [PktByte0=<val>]     - Match packet data byte 0 (example)\n"
    "  knetctrl filter destroy <id>\n"
    "        Destroy a packet filter.\n"
    "  knetctrl filter show\n"
    "        Show all packet filters.\n"
#endif
    ;

/* Note:  See knet.h, BCM_KNET_NETIF_T_xxx */
STATIC char *netif_type[] = {
    "unknown", "Vlan", "Port", "Meta", NULL
};

/* Note:  See knet.h, BCM_KNET_DEST_T_xxx */
STATIC char *filter_dest_type[] = {
    "Null", "NetIF", "RxAPI", NULL
};

STATIC char *reason_str[bcmRxReasonCount+1] = BCM_RX_REASON_NAMES_INITIALIZER;

STATIC void
_show_netif(int unit, bcm_knet_netif_t *netif)
{
    char *type_str = "?";
    char *port_str = "n/a";

    switch (netif->type) {
    case BCM_KNET_NETIF_T_TX_CPU_INGRESS:
    case BCM_KNET_NETIF_T_TX_META_DATA:
        type_str = netif_type[netif->type];
        break;
    case BCM_KNET_NETIF_T_TX_LOCAL_PORT:
        type_str = netif_type[netif->type];
        port_str = BCM_PORT_NAME(unit, netif->port);
        break;
    default:
        break;
    }

    printk("Interface ID %d: name=%s type=%s vlan=%d port=%s",
           netif->id, netif->name, type_str, netif->vlan, port_str);

    if (netif->flags & BCM_KNET_NETIF_F_ADD_TAG) {
        printk(" addtag");
    }
    if (netif->flags & BCM_KNET_NETIF_F_RCPU_ENCAP) {
        printk(" rcpu");
    }
    printk("\n");
}

STATIC int
_trav_netif(int unit, bcm_knet_netif_t *netif, void *user_data)
{
    int *count = (int *)user_data;

    _show_netif(unit, netif);

    if (count) {
        (*count)++;
    }

    return BCM_E_NONE;
}

static void
_show_filter(int unit, bcm_knet_filter_t *filter)
{
    char *dest_str = "?";
    int idx, edx;

    switch (filter->dest_type) {
    case BCM_KNET_DEST_T_NULL:
    case BCM_KNET_DEST_T_NETIF:
    case BCM_KNET_DEST_T_BCM_RX_API:
        dest_str = filter_dest_type[filter->dest_type];
        break;
    default:
        break;
    }

    printk("Filter ID %d: prio=%d dest=%s(%d) desc='%s'",
           filter->id, filter->priority, dest_str,
           filter->dest_id, filter->desc);

    if (filter->mirror_type ==  BCM_KNET_DEST_T_BCM_RX_API) {
        printk(" mirror=rxapi");
    } else if (filter->mirror_type ==  BCM_KNET_DEST_T_NETIF) {
        printk(" mirror=netif(%d)", filter->mirror_id);
    }
    if (filter->flags & BCM_KNET_FILTER_F_STRIP_TAG) {
        printk(" striptag");
    }
    if (filter->match_flags & BCM_KNET_FILTER_M_VLAN) {
        printk(" vlan=%d", filter->m_vlan);
    }
    if (filter->match_flags & BCM_KNET_FILTER_M_INGPORT) {
        printk(" ingport=%s", BCM_PORT_NAME(unit, filter->m_ingport));
    }
    if (filter->match_flags & BCM_KNET_FILTER_M_SRC_MODID) {
        printk(" srcmod=%d", filter->m_src_modid);
    }
    if (filter->match_flags & BCM_KNET_FILTER_M_SRC_MODPORT) {
        printk(" srcport=%d", filter->m_src_modport);
    }
    if (filter->match_flags & BCM_KNET_FILTER_M_FP_RULE) {
        printk(" fprule=%d", filter->m_fp_rule);
    }
    if (filter->match_flags & BCM_KNET_FILTER_M_REASON) {
        printk(" reason=");
        for (idx = 0; idx < bcmRxReasonCount; idx++) {
            if (BCM_RX_REASON_GET(filter->m_reason, idx)) {
                printk("%s", reason_str[idx]);
                break;
            }
        }
    }
    if (filter->match_flags & BCM_KNET_FILTER_M_RAW) {
        printk(" rawdata");
        for (idx = 0; idx < filter->raw_size; idx++) {
            if (filter->m_raw_mask[idx]) {
                break;
            }
        }
        for (edx = filter->raw_size - 1; edx > idx; edx--) {
            if (filter->m_raw_mask[edx]) {
                break;
            }
        }
        if (edx < idx) {
            /* Entire mask is empty - should not happen */
            printk("?");
        } else {
            /* Show offset of first valid byte */
            printk("[%d]", idx);
            /* Dump data */
            for (; idx <= edx; idx++) {
                if (filter->m_raw_mask[idx]) {
                    printk(":0x%02x", filter->m_raw_data[idx]);
                    if (filter->m_raw_mask[idx] != 0xff) {
                        printk("/0x%02x", filter->m_raw_mask[idx]);
                    }
                } else {
                    printk(":-"); 
                }
            }
        }
    }
    printk("\n");
}

STATIC int
_trav_filter(int unit, bcm_knet_filter_t *filter, void *user_data)
{
    int *count = (int *)user_data;

    _show_filter(unit, filter);

    if (count) {
        (*count)++;
    }

    return BCM_E_NONE;
}

cmd_result_t
cmd_knet_ctrl(int unit, args_t *args)
{
    parse_table_t  pt;
    char *subcmd;
    int rv;
    int idx, pdx, count;
    int if_id;
    int if_type;
    int if_vlan;
    int if_port;
    int if_addtag;
    int if_rcpu;
    char *if_name;
    int pf_id;
    int pf_prio;
    int pf_dest_type;
    int pf_dest_id;
    int pf_striptag;
    int pf_mirror;
    int pf_mirror_id;
    int pf_vlan;
    int pf_ingport;
    int pf_src_modport;
    int pf_src_modid;
    int pf_reason;
    int pf_fp_rule;
    char *pf_desc;
    uint32 pkt_offset;
    int pkt_data[8];
    bcm_knet_netif_t netif;
    bcm_knet_filter_t filter;

    if ((subcmd = ARG_GET(args)) == NULL) {
        printk("Requires string argument\n");
        return CMD_USAGE;
    }
    if (sal_strcasecmp(subcmd, "netif") == 0) {
        if ((subcmd = ARG_GET(args)) == NULL) {
            printk("Requires additional string argument\n");
            return CMD_USAGE;
        }
        if (sal_strcasecmp(subcmd, "create") == 0) {
            if_type = BCM_KNET_NETIF_T_TX_CPU_INGRESS;
            if_vlan = 1;
            if_port = -1;
            if_addtag = 0;
            if_rcpu = 0;
            if_name = NULL;
            parse_table_init(unit, &pt);
            parse_table_add(&pt, "Type", PQ_DFL|PQ_MULTI, 0, &if_type, netif_type);
            parse_table_add(&pt, "Vlan", PQ_DFL|PQ_INT, 0, &if_vlan, 0);
            parse_table_add(&pt, "Port", PQ_DFL|PQ_PORT, 0, &if_port, 0);
            parse_table_add(&pt, "AddTag", PQ_DFL|PQ_BOOL, 0, &if_addtag, 0);
            parse_table_add(&pt, "RCPU", PQ_DFL|PQ_BOOL, 0, &if_rcpu, 0);
            parse_table_add(&pt, "IFName", PQ_DFL|PQ_STRING, 0, &if_name, 0);
            if (parse_arg_eq(args, &pt) < 0) {
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }
            bcm_knet_netif_t_init(&netif);
            if (if_name) {
                sal_strncpy(netif.name, if_name, sizeof(netif.name) - 1);
            }
            parse_arg_eq_done(&pt);
            /* Force port mode if port was specified */
            if (if_port >= 0) {
                if_type = BCM_KNET_NETIF_T_TX_LOCAL_PORT;
            }
            /* Force meta mode if RCPU was specified */
            if (if_rcpu) {
                if_type = BCM_KNET_NETIF_T_TX_META_DATA;
            }
            netif.type = if_type;
            netif.vlan = if_vlan;
            if (if_port >= 0) {
                netif.port = if_port;
            }
            if (if_addtag) {
                netif.flags |= BCM_KNET_NETIF_F_ADD_TAG;
            }
            if (if_rcpu) {
                netif.flags |= BCM_KNET_NETIF_F_RCPU_ENCAP;
            }
            if ((rv = bcm_knet_netif_create(unit, &netif)) < 0) {
                printk("Error creating network interface: %s\n",
		       bcm_errmsg(rv));
                return CMD_FAIL;
            }
            _show_netif(unit, &netif);
        } else if (sal_strcasecmp(subcmd, "destroy") == 0) {
            if (!ARG_CNT(args) || !isint(ARG_CUR(args))) {
                return(CMD_USAGE);
            }
            if_id = parse_integer(ARG_GET(args));
            if ((rv = bcm_knet_netif_destroy(unit, if_id)) < 0) {
                printk("Error destroying network interface: %s\n",
		       bcm_errmsg(rv));
                return CMD_FAIL;
            }
        } else if (sal_strcasecmp(subcmd, "show") == 0) {
            count = 0;
            if (bcm_knet_netif_traverse(unit, _trav_netif, &count) < 0) {
                return CMD_FAIL;
            }
            if (count == 0) {
                printk("<no network interfaces>\n");
            }
        } else {
            printk("Subcommand not found: %s\n", subcmd);
            return CMD_USAGE;
        }
    } else if (sal_strcasecmp(subcmd, "filter") == 0) {
        if ((subcmd = ARG_GET(args)) == NULL) {
            printk("Requires string argument\n");
            return CMD_USAGE;
        }
        if (sal_strcasecmp(subcmd, "create") == 0) {
            pf_dest_type = -1;
            pf_dest_id = -1;
            pf_mirror = 0;
            pf_mirror_id = 0;
            pf_vlan = -1;
            pf_ingport = -1;
            pf_src_modport = -1;
            pf_src_modid = -1;
            pf_reason = -1;
            pf_fp_rule = -1;
            pkt_offset = 0;
            for (idx = 0; idx < COUNTOF(pkt_data); idx++) {
                pkt_data[idx] = -1;
            }
            pf_desc = NULL;
            pf_prio = 0;
            pf_striptag = 1;
            parse_table_init(unit, &pt);
            parse_table_add(&pt, "DestType", PQ_DFL|PQ_MULTI, 0, &pf_dest_type,
                            filter_dest_type);
            parse_table_add(&pt, "DestID", PQ_DFL|PQ_INT, 0, &pf_dest_id, 0);
            parse_table_add(&pt, "PRIOrity", PQ_DFL|PQ_INT, 0, &pf_prio, 0);
            parse_table_add(&pt, "DESCription", PQ_DFL|PQ_STRING, 0, &pf_desc, 0);
            parse_table_add(&pt, "StripTag", PQ_DFL|PQ_BOOL, 0, &pf_striptag, 0);
            parse_table_add(&pt, "Mirror", PQ_DFL|PQ_BOOL, 0, &pf_mirror, 0);
            parse_table_add(&pt, "MirrorID", PQ_DFL|PQ_INT, 0, &pf_mirror_id, 0);
            parse_table_add(&pt, "Vlan", PQ_DFL|PQ_INT, 0, &pf_vlan, 0);
            parse_table_add(&pt, "IngPort", PQ_DFL|PQ_PORT, 0, &pf_ingport, 0);
            parse_table_add(&pt, "SrcPort", PQ_DFL|PQ_INT, 0, &pf_src_modport, 0);
            parse_table_add(&pt, "SrcModid", PQ_DFL|PQ_INT, 0, &pf_src_modid, 0);
            parse_table_add(&pt, "Reason", PQ_DFL|PQ_MULTI, 0, &pf_reason,
                            reason_str);
            parse_table_add(&pt, "FPRule", PQ_DFL|PQ_INT, 0, &pf_fp_rule, 0);
            parse_table_add(&pt, "PktOffset", PQ_DFL|PQ_HEX, 0, &pkt_offset, 0);
            parse_table_add(&pt, "PktByte0", PQ_DFL|PQ_INT, 0, &pkt_data[0], 0);
            parse_table_add(&pt, "PktByte1", PQ_DFL|PQ_INT, 0, &pkt_data[1], 0);
            parse_table_add(&pt, "PktByte2", PQ_DFL|PQ_INT, 0, &pkt_data[2], 0);
            parse_table_add(&pt, "PktByte3", PQ_DFL|PQ_INT, 0, &pkt_data[3], 0);
            parse_table_add(&pt, "PktByte4", PQ_DFL|PQ_INT, 0, &pkt_data[4], 0);
            parse_table_add(&pt, "PktByte5", PQ_DFL|PQ_INT, 0, &pkt_data[5], 0);
            parse_table_add(&pt, "PktByte6", PQ_DFL|PQ_INT, 0, &pkt_data[6], 0);
            parse_table_add(&pt, "PktByte7", PQ_DFL|PQ_INT, 0, &pkt_data[7], 0);
            if (parse_arg_eq(args, &pt) < 0) {
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }
            bcm_knet_filter_t_init(&filter);
            filter.type = BCM_KNET_FILTER_T_RX_PKT;
            if (pf_desc) {
                sal_strncpy(filter.desc, pf_desc, sizeof(filter.desc) - 1);
            }
            parse_arg_eq_done(&pt);
            if (pf_dest_type < 0) {
                printk("Missing destination\n");
                return CMD_USAGE;
            }
            filter.priority = pf_prio;
            filter.dest_type = pf_dest_type;
            filter.dest_id = pf_dest_id;
            /*
             * Specifying a mirror ID implies mirroring to another
             * netif.  Enabling mirroring while leaving mirror ID at
             * zero implies mirroring to the Rx API.
             */
            if (pf_mirror_id) {
                pf_mirror = 1;
            }
            if (pf_dest_type == BCM_KNET_DEST_T_NETIF && pf_mirror) {
                filter.mirror_type = BCM_KNET_DEST_T_BCM_RX_API;
                if (pf_mirror_id) {
                    filter.mirror_type = BCM_KNET_DEST_T_NETIF;
                    filter.mirror_id = pf_mirror_id;
                }
            }
            if (pf_striptag) {
                filter.flags |= BCM_KNET_FILTER_F_STRIP_TAG;
            }
            if (pf_vlan >= 0) {
                filter.m_vlan = pf_vlan;
                filter.match_flags |= BCM_KNET_FILTER_M_VLAN;
            }
            if (pf_ingport >= 0) {
                filter.m_ingport = pf_ingport;
                filter.match_flags |= BCM_KNET_FILTER_M_INGPORT;
            }
            if (pf_src_modport >= 0) {
                filter.m_src_modport = pf_src_modport;
                filter.match_flags |= BCM_KNET_FILTER_M_SRC_MODPORT;
            }
            if (pf_src_modid >= 0) {
                filter.m_src_modid = pf_src_modid;
                filter.match_flags |= BCM_KNET_FILTER_M_SRC_MODID;
            }
            if (pf_reason >= 0) {
                BCM_RX_REASON_SET(filter.m_reason, pf_reason);
                filter.match_flags |= BCM_KNET_FILTER_M_REASON;
            }
            if (pf_fp_rule >= 0) {
                filter.m_fp_rule = pf_fp_rule;
                filter.match_flags |= BCM_KNET_FILTER_M_FP_RULE;
            }
            for (idx = 0; idx < COUNTOF(pkt_data); idx++) {
                pdx = pkt_offset + idx;
                if (pdx >= sizeof(filter.m_raw_data)) {
                    printk("PktOffset too large - max is %d\n",
                           (int)sizeof(filter.m_raw_data) - COUNTOF(pkt_data));
                    return CMD_USAGE;
                }
                if (pkt_data[idx] >= 0) {
                    filter.raw_size = pdx + 1;
                    filter.match_flags |= BCM_KNET_FILTER_M_RAW;
                    filter.m_raw_data[pdx] = (uint8)pkt_data[idx];
                    filter.m_raw_mask[pdx] = 0xff;
                }
            }
            if ((rv = bcm_knet_filter_create(unit, &filter)) < 0) {
                printk("Error creating packet filter: %s\n",
		       bcm_errmsg(rv));
                return CMD_FAIL;
            }
            _show_filter(unit, &filter);
        } else if (sal_strcasecmp(subcmd, "destroy") == 0) {
            if (!ARG_CNT(args) || !isint(ARG_CUR(args))) {
                return(CMD_USAGE);
            }
            pf_id = parse_integer(ARG_GET(args));
            if ((rv = bcm_knet_filter_destroy(unit, pf_id)) < 0) {
                printk("Error destroying packet filter: %s\n",
		       bcm_errmsg(rv));
                return CMD_FAIL;
            }
        } else if (sal_strcasecmp(subcmd, "show") == 0) {
            count = 0;
            if (bcm_knet_filter_traverse(unit, _trav_filter, &count) < 0) {
                return CMD_FAIL;
            }
            if (count == 0) {
                printk("<no filters>\n");
            }
        } else {
            printk("Subcommand not found: %s\n", subcmd);
            return CMD_USAGE;
        }
    } else {
        printk("Subcommand not found: %s\n", subcmd);
        return CMD_USAGE;
    }
    return CMD_OK;
}

#else
int _knetctrl_not_empty;
#endif
