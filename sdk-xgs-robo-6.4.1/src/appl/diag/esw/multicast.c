/*
 * $Id: multicast.c,v 1.11 Broadcom SDK $
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
 * File:    multicast.c
 * Purpose: Multicast CLI commands
 */

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <shared/bsl.h>
#include <bcm/error.h>

#include <bcm/multicast.h>
#include <bcm_int/common/multicast.h>

char cmd_multicast_usage[] =
    "Usages:\n"
    "\tcreate Type=L2|L3|VPLS|SUBPORT|MIN|WLAN|TRILL [Group=<num>]\n"
    "\tdestroy Group=<num>\n"
    "\tl3encap Group=<num> Port=<num> Intf=<num>\n"
    "\tl2encap Group=<num> Port=<num> Vlan=<num>\n"
    "\tvplsencap Group=<num> Port=<num> MplsPortId=<num>\n"
    "\tsubportencap Group=<num> Port=<num> SubPort=<num>\n"
    "\tmimencap Group=<num> Port=<num> MimPortId=<num>\n"
    "\twlanencap Group=<num> Port=<num> WlanPortId=<num>\n"
    "\tadd Group=<num> Port=<num> EncapId=<num>\n"
    "\tdelete Group=<num> [Port=<num> EncapId=<num>]\n"
    "\tshow [Group=<num>]\n";

const char *cmd_multicast_parse_type[] = {
    "NONE",
    "L2",
    "L3",
    "VPLS",
    "SUBPORT",
    "MIM",
    "WLAN",
    "VLAN",
    "TRILL",
    "NIV",
    "EGRESS_OBJECT",
    "L2GRE",
    "VXLAN",
    "PORTS_GROUP",
    "EXTENDER"
};

int
_cmd_multicast_traverse_cb(int unit, bcm_multicast_t group, uint32 flags, 
                           void *user_data)
{
    int rv, i, port_count;
    bcm_gport_t *port_array = NULL;
    bcm_if_t    *encap_id_array = NULL;
    int         port_max = 0;
 
    /* first, get the port count.
     * port_max = 0, port_array & encap_id_array = NULL
     */
    rv = bcm_multicast_egress_get(unit, group, port_max, port_array,
                                  encap_id_array, &port_count);
    if (rv < 0) {
        cli_out("%s ERROR: egress port count get failed - %s\n",
                ARG_CMD((args_t *)user_data), bcm_errmsg(rv));
        return rv;
    }
   
    if (port_count == 0) {
        return BCM_E_NONE;
    }
 
    /* allocate proper size port_array & encap_id_array */
    port_array = (bcm_gport_t *)sal_alloc(sizeof(bcm_gport_t) * port_count,
                                 "_cmd_multicast_traverse_cb : port_array");
    if (port_array == NULL) {
        cli_out("%s ERROR: port_array mem alloc failed\n",
                ARG_CMD((args_t *)user_data));
        return BCM_E_MEMORY;
    }
    encap_id_array = (bcm_if_t *)sal_alloc(sizeof(bcm_if_t) *port_count,
                               "_cmd_multicast_traverse_cb : encap_id_array");
    if (encap_id_array == NULL) {
        cli_out("%s ERROR: encap_id_array mem alloc failed\n",
                ARG_CMD((args_t *)user_data));
        sal_free(port_array);
        return BCM_E_MEMORY;        
    }
	 
    rv = bcm_multicast_egress_get(unit, group, port_count,
                                  port_array, encap_id_array,
                                  &port_count);
    if (rv < 0) {
        cli_out("%s ERROR: egress get failure - %s\n",
                ARG_CMD((args_t *)user_data), bcm_errmsg(rv));
        sal_free(port_array);
        sal_free(encap_id_array);
        return rv;
    }
    if (_BCM_MULTICAST_TYPE_GET(group) >= COUNTOF(cmd_multicast_parse_type)) {
        cli_out("Group 0x%x (%s)\n", group,
                "UNKNOWN");
    } else {
        cli_out("Group 0x%x (%s)\n", group,
                cmd_multicast_parse_type[_BCM_MULTICAST_TYPE_GET(group)]);
    }
    for (i = 0; i < port_count; i++) {
        cli_out("\tport %s, encap id %d\n",
                mod_port_name(unit, -1, port_array[i]), encap_id_array[i]);
    }

    sal_free(port_array);
    sal_free(encap_id_array);
    return BCM_E_NONE;
}

cmd_result_t
cmd_multicast(int unit, args_t *a)
{
    char *subcmd = NULL;
    parse_table_t pt;
    int rv, type;
    uint32 flags;
    bcm_multicast_t group;
    bcm_gport_t port;
    bcm_if_t intf; /* for l3_encap_get */
    bcm_vlan_t vlan; /* for l2_encap_get */
    bcm_gport_t mpls_port_id; /* for vpls_encap_get */
    bcm_gport_t subport; /* for subport_encap_get */
    bcm_gport_t mim_port_id; /* for mim_encap_get */
    bcm_gport_t wlan_port_id; /* for wlan_encap_get */
    bcm_if_t encap_id;
    char buf[20];

    subcmd = ARG_GET(a);
    if (subcmd == NULL) {
        return CMD_USAGE;
    }

    if (!sal_strcasecmp(subcmd, "create")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Type", PQ_MULTI, 0, &type,
                        cmd_multicast_parse_type);
        parse_table_add(&pt, "Group", PQ_INT, (void *)-1, &group, 0);
        if (parse_arg_eq(a, &pt) < 0) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_FAIL;
        }
        parse_arg_eq_done(&pt);

        switch (type) {
        case _BCM_MULTICAST_TYPE_L2:
            flags = BCM_MULTICAST_TYPE_L2;
            break;
        case _BCM_MULTICAST_TYPE_L3:
            flags = BCM_MULTICAST_TYPE_L3;
            break;
        case _BCM_MULTICAST_TYPE_VPLS:
            flags = BCM_MULTICAST_TYPE_VPLS;
            break;
        case _BCM_MULTICAST_TYPE_SUBPORT:
            flags = BCM_MULTICAST_TYPE_SUBPORT;
            break;
        case _BCM_MULTICAST_TYPE_MIM:
            flags = BCM_MULTICAST_TYPE_MIM;
            break;
        case _BCM_MULTICAST_TYPE_WLAN:
            flags = BCM_MULTICAST_TYPE_WLAN;
            break;
        case _BCM_MULTICAST_TYPE_TRILL:
            flags = BCM_MULTICAST_TYPE_TRILL;
            break;
        default:
            return CMD_FAIL;
        }
        if (group != -1) {
            flags |= BCM_MULTICAST_WITH_ID;
            if (!_BCM_MULTICAST_IS_SET(group)) {
                /* Encode the group properly for the selected type */
                _BCM_MULTICAST_GROUP_SET(group, type, group);
            }
        }
        rv = bcm_multicast_create(unit, flags, &group);
        if (rv < 0) {
            cli_out("%s ERROR: fail to create %s group - %s\n",
                    ARG_CMD(a), cmd_multicast_parse_type[type],
                    bcm_errmsg(rv));
            return CMD_FAIL;
        }

        cli_out("group id 0x%x\n", group);
        sal_sprintf(buf, "0x%x", group);
        var_set("multicast_group", buf, TRUE, FALSE);
        return CMD_OK;
    }

    if (!sal_strcasecmp(subcmd, "destroy")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Group", PQ_INT, (void *)-1, &group, 0);
        if (parse_arg_eq(a, &pt) < 0 || group == -1) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_FAIL;
        }
        parse_arg_eq_done(&pt);

        rv = bcm_multicast_destroy(unit, group);
        if (rv < 0) {
            cli_out("%s ERROR: fail to destroy group %d - %s\n",
                    ARG_CMD(a), group, bcm_errmsg(rv));
            return CMD_FAIL;
        }
        return CMD_OK;
    }

    if (!sal_strcasecmp(subcmd, "l3encap")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Group", PQ_INT, (void *)-1, &group, 0);
        parse_table_add(&pt, "Port", PQ_PORT, (void *)BCM_GPORT_INVALID, &port,
                        0);
        parse_table_add(&pt, "Intf", PQ_INT, (void *)-1, &intf, 0);
        if (parse_arg_eq(a, &pt) < 0) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_FAIL;
        }
        parse_arg_eq_done(&pt);

        if (intf == -1) {
            cli_out("%s: Interface not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }

        if (!BCM_GPORT_IS_SET(port)) {
            BCM_GPORT_LOCAL_SET(port, port);
        }
        rv = bcm_multicast_l3_encap_get(unit, group, port, intf, &encap_id);
        if (rv < 0) {
            cli_out("%s ERROR: fail to get L3 encap - %s\n",
                    ARG_CMD(a), bcm_errmsg(rv));
            return CMD_FAIL;
        }
        cli_out("Encap ID %d\n", encap_id);
        return CMD_OK;
    }

    if (!sal_strcasecmp(subcmd, "l2encap")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Group", PQ_INT, (void *)-1, &group, 0);
        parse_table_add(&pt, "Port", PQ_PORT, (void *)BCM_GPORT_INVALID, &port,
                        0);
        parse_table_add(&pt, "Vlan", PQ_INT, (void *)BCM_VLAN_INVALID, &vlan,
                        0);
        if (parse_arg_eq(a, &pt) < 0) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_FAIL;
        }
        parse_arg_eq_done(&pt);

        if (!BCM_GPORT_IS_SET(port)) {
            BCM_GPORT_LOCAL_SET(port, port);
        }
        rv = bcm_multicast_l2_encap_get(unit, group, port, vlan, &encap_id);
        if (rv < 0) {
            cli_out("%s ERROR: fail to get L2 encap - %s\n",
                    ARG_CMD(a), bcm_errmsg(rv));
            return CMD_FAIL;
        }
        cli_out("Encap ID %d\n", encap_id);
        return CMD_OK;
    }

    if (!sal_strcasecmp(subcmd, "vplsencap")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Group", PQ_INT, (void *)-1, &group, 0);
        parse_table_add(&pt, "Port", PQ_PORT, (void *)BCM_GPORT_INVALID, &port,
                        0);
        parse_table_add(&pt, "MplsPortId", PQ_INT, (void *)BCM_GPORT_INVALID,
                        &mpls_port_id, 0);
        if (parse_arg_eq(a, &pt) < 0) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_FAIL;
        }
        parse_arg_eq_done(&pt);

        if (group < 0) {
            cli_out("%s: Group not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        if (port == BCM_GPORT_INVALID) {
            cli_out("%s: Port not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        if (mpls_port_id == BCM_GPORT_INVALID) {
            cli_out("%s: MplsPortId not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }

        if (!BCM_GPORT_IS_SET(port)) {
            BCM_GPORT_LOCAL_SET(port, port);
        }
        if (!BCM_GPORT_IS_SET(mpls_port_id)) {
            BCM_GPORT_MPLS_PORT_ID_SET(mpls_port_id, mpls_port_id);
        }
        rv = bcm_multicast_vpls_encap_get(unit, group, port, mpls_port_id,
                                          &encap_id);
        if (rv < 0) {
            cli_out("%s ERROR: fail to get VPLS encap - %s\n",
                    ARG_CMD(a), bcm_errmsg(rv));
            return CMD_FAIL;
        }
        cli_out("Encap ID %d\n", encap_id);
        return CMD_OK;
    }

    if (!sal_strcasecmp(subcmd, "subportEncap")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Group", PQ_INT, (void *)-1, &group, 0);
        parse_table_add(&pt, "Port", PQ_PORT, (void *)BCM_GPORT_INVALID, &port,
                        0);
        parse_table_add(&pt, "SubPort", PQ_INT, (void *)BCM_GPORT_INVALID,
                        &subport, 0);
        if (parse_arg_eq(a, &pt) < 0) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_FAIL;
        }
        parse_arg_eq_done(&pt);

        if (group < 0) {
            cli_out("%s: Group not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        if (port == BCM_GPORT_INVALID) {
            cli_out("%s: Port not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        if (subport == BCM_GPORT_INVALID) {
            cli_out("%s: SubPort not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }

        if (!BCM_GPORT_IS_SET(port)) {
            BCM_GPORT_LOCAL_SET(port, port);
        }
        if (!BCM_GPORT_IS_SET(subport)) {
            BCM_GPORT_SUBPORT_PORT_SET(subport, subport);
        }
        rv = bcm_multicast_subport_encap_get(unit, group, port, subport,
                                             &encap_id);
        if (rv < 0) {
            cli_out("%s ERROR: fail to get subport encap - %s\n",
                    ARG_CMD(a), bcm_errmsg(rv));
            return CMD_FAIL;
        }
        cli_out("Encap ID %d\n", encap_id);
        return CMD_OK;
    }

    if (!sal_strcasecmp(subcmd, "mimencap")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Group", PQ_INT, (void *)-1, &group, 0);
        parse_table_add(&pt, "Port", PQ_PORT, (void *)BCM_GPORT_INVALID, &port,
                        0);
        parse_table_add(&pt, "MimPortId", PQ_INT, (void *)BCM_GPORT_INVALID,
                        &mim_port_id, 0);
        if (parse_arg_eq(a, &pt) < 0) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_FAIL;
        }
        parse_arg_eq_done(&pt);

        if (group < 0) {
            cli_out("%s: Group not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        if (port == BCM_GPORT_INVALID) {
            cli_out("%s: Port not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        if (mim_port_id == BCM_GPORT_INVALID) {
            cli_out("%s: MimPortId not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }

        if (!BCM_GPORT_IS_SET(port)) {
            BCM_GPORT_LOCAL_SET(port, port);
        }
        if (!BCM_GPORT_IS_SET(mim_port_id)) {
            BCM_GPORT_MIM_PORT_ID_SET(mim_port_id, mim_port_id);
        }
        rv = bcm_multicast_mim_encap_get(unit, group, port, mim_port_id,
                                         &encap_id);
        if (rv < 0) {
            cli_out("%s ERROR: fail to get MIM encap - %s\n",
                    ARG_CMD(a), bcm_errmsg(rv));
            return CMD_FAIL;
        }
        cli_out("Encap ID %d\n", encap_id);
        return CMD_OK;
    }

    if (!sal_strcasecmp(subcmd, "wlanencap")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Group", PQ_INT, (void *)-1, &group, 0);
        parse_table_add(&pt, "Port", PQ_PORT, (void *)BCM_GPORT_INVALID, &port,
                        0);
        parse_table_add(&pt, "WlanPortId", PQ_INT, (void *)BCM_GPORT_INVALID,
                        &wlan_port_id, 0);
        if (parse_arg_eq(a, &pt) < 0) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_FAIL;
        }
        parse_arg_eq_done(&pt);

        if (group < 0) {
            cli_out("%s: Group not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        if (port == BCM_GPORT_INVALID) {
            cli_out("%s: Port not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        if (wlan_port_id == BCM_GPORT_INVALID) {
            cli_out("%s: WlanPortId not specified\n", ARG_CMD(a));
            return CMD_FAIL;
        }

        if (!BCM_GPORT_IS_SET(port)) {
            BCM_GPORT_LOCAL_SET(port, port);
        }
        if (!BCM_GPORT_IS_SET(wlan_port_id)) {
            BCM_GPORT_WLAN_PORT_ID_SET(wlan_port_id, wlan_port_id);
        }
        rv = bcm_multicast_wlan_encap_get(unit, group, port, wlan_port_id,
                                          &encap_id);
        if (rv < 0) {
            cli_out("%s ERROR: fail to get WLAN encap - %s\n",
                    ARG_CMD(a), bcm_errmsg(rv));
            return CMD_FAIL;
        }
        cli_out("Encap ID %d\n", encap_id);
        return CMD_OK;
    }

    if (!sal_strcasecmp(subcmd, "add")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Group", PQ_INT, (void *)-1, &group, 0);
        parse_table_add(&pt, "Port", PQ_PORT, (void *)BCM_GPORT_INVALID, &port,
                        0);
        parse_table_add(&pt, "EncapId", PQ_INT, (void *)BCM_IF_INVALID,
                        &encap_id, 0);

        if (parse_arg_eq(a, &pt) < 0 || group < 0 ||
            port == BCM_GPORT_INVALID) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_FAIL;
        }
        parse_arg_eq_done(&pt);

        if (!BCM_GPORT_IS_SET(port)) {
            BCM_GPORT_LOCAL_SET(port, port);
        }
        rv = bcm_multicast_egress_add(unit, group, port, encap_id);
        if (rv < 0) {
            cli_out("%s ERROR: egress add failure - %s\n",
                    ARG_CMD(a), bcm_errmsg(rv));
            return CMD_FAIL;
        }
        return CMD_OK;
    }

    if (!sal_strcasecmp(subcmd, "delete")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Group", PQ_INT, (void *)-1, &group, 0);
        parse_table_add(&pt, "Port", PQ_PORT, (void *)BCM_GPORT_INVALID, &port,
                        0);
        parse_table_add(&pt, "EncapId", PQ_INT, (void *)BCM_IF_INVALID,
                        &encap_id, 0);
        if (parse_arg_eq(a, &pt) < 0 || group < 0) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_FAIL;
        }
        parse_arg_eq_done(&pt);

        if (port == BCM_GPORT_INVALID && encap_id == BCM_IF_INVALID) {
            rv = bcm_multicast_egress_delete_all(unit, group);
            if (rv < 0) {
                cli_out("%s ERROR: egress delete all failure - %s\n",
                        ARG_CMD(a), bcm_errmsg(rv));
                return CMD_FAIL;
            }
        } else {
            if (port == BCM_GPORT_INVALID || encap_id == BCM_IF_INVALID) {
                return CMD_FAIL;
            }
            if (!BCM_GPORT_IS_SET(port)) {
                BCM_GPORT_LOCAL_SET(port, port);
            }
            rv = bcm_multicast_egress_delete(unit, group, port, encap_id);
            if (rv < 0) {
                cli_out("%s ERROR: egress delete failure - %s\n",
                        ARG_CMD(a), bcm_errmsg(rv));
                return CMD_FAIL;
            }
        }
        return CMD_OK;
    }

    if (!sal_strcasecmp(subcmd, "show")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Group", PQ_INT, (void *)-1, &group, 0);
        if (parse_arg_eq(a, &pt) < 0) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_FAIL;
        }
        parse_arg_eq_done(&pt);


        if (group < 0) {
            rv = bcm_multicast_group_traverse(unit,
                                              &_cmd_multicast_traverse_cb,
                                              BCM_MULTICAST_TYPE_MASK, a);
        } else {
            rv = _cmd_multicast_traverse_cb(unit, group, 0, a);
        }

        if (BCM_FAILURE(rv)) {
            return CMD_FAIL;
        }

        return CMD_OK;
    }

    return CMD_USAGE;
}
