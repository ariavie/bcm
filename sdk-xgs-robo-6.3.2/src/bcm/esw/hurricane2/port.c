/*
 * $Id: port.c 1.2 Broadcom SDK $
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

#include <soc/defs.h>

#include <assert.h>

#include <sal/core/libc.h>
#if defined(BCM_HURRICANE2_SUPPORT)

#include <shared/util.h>
#include <soc/mem.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/hurricane2.h>
#include <bcm/error.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/xgs3.h>

#include <bcm_int/esw_dispatch.h>

/*
 * Each TSC can be configured into following 5 mode:
 *   Lane number    0    1    2    3
 *   ------------  ---  ---  ---  ---
 *    single port  40G   x    x    x
 *      dual port  20G   x   20G   x
 *   tri_023 port  20G   x   10G  10G
 *   tri_012 port  10G  10G  20G   x
 *      quad port  10G  10G  10G  10G
 *
 *          lanes                mode         valid lane index
 *       ------------      ----------------   ----------------
 *       new  current        new    current
 *       ---  -------      -------  -------
 * #1     4      1         single    quad            0
 * #2     4      1         single   tri_012          0
 * #3     4      2         single   tri_023          0
 * #4     4      2         single    dual            0
 * #5     2      1         tri_023   quad            0
 * #6     2      1         tri_012   quad            2
 * #7     2      1          dual    tri_023          2
 * #8     2      1          dual    tri_012          0
 * #9     2      4          dual    single           0
 * #10    1      2         tri_023   dual            2
 * #11    1      2         tri_012   dual            0
 * #12    1      2          quad    tri_023          0
 * #13    1      2          quad    tri_012          2
 * #14    1      4          quad    single           0
 * Following mode change requires 2 transition
 *   - from single to tri_023: #9 + #10
 *   - from single to tri_012: #9 + #11
 * Following mode change are the result of lane change on multiple ports
 *   - from quad to dual: #12 + #7 or #13 + #8
 *   - from dual to quad: #10 + #12 or #11 + # 13
 *   - from tri_023 to tri_012: #7 + #11 or #12 + #6
 *   - from tri_012 to tri_023: #8 + #10 or #13 + #5
 *
 * Logical port number will stay the same after conversion, for example
 *     converting single port to dual port, the logical port number of lane 0
 *     will be changed.
 */
int
soc_hurricane2_port_lanes_set(int unit, soc_port_t port_base, int lanes,
                            int *cur_lanes, int *phy_ports, int *phy_ports_len)
{
    soc_info_t *si = &SOC_INFO(unit);
    soc_field_t fields[2];
    uint32 values[2];
    int mode, cur_mode;
    int phy_port_base, port, i;
    int blk, bindex;
    uint32 rval;

    /* Find physical port number and lane index of the specified port */
    phy_port_base = si->port_l2p_mapping[port_base];
    if (phy_port_base == -1) {
        return SOC_E_PORT;
    }

    bindex = -1;
    for (i = 0; i < SOC_DRIVER(unit)->port_num_blktype; i++) {
        blk = SOC_PORT_IDX_BLOCK(unit, phy_port_base, i);
        if (SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_XLPORT) {
            bindex = SOC_PORT_IDX_BINDEX(unit, phy_port_base, i);
            break;
        }
    }

    /* Get the current mode */
    SOC_IF_ERROR_RETURN(READ_XLPORT_MODE_REGr(unit, port_base, &rval));
    cur_mode = soc_reg_field_get(unit, XLPORT_MODE_REGr, rval,
                                 XPORT0_CORE_PORT_MODEf);

    /* Figure out the current number of lane from current mode */
    switch (cur_mode) {
    case SOC_HU2_PORT_MODE_QUAD:
        *cur_lanes = 1;
        break;
    case SOC_HU2_PORT_MODE_TRI_012:
        *cur_lanes = bindex == 0 ? 1 : 2;
        break;
    case SOC_HU2_PORT_MODE_TRI_023:
        *cur_lanes = bindex == 0 ? 2 : 1;
        break;
    case SOC_HU2_PORT_MODE_DUAL:
        *cur_lanes = 2;
        break;
    case SOC_HU2_PORT_MODE_SINGLE:
        *cur_lanes = 4;
        break;
    default:
        return SOC_E_FAIL;
    }

    if (lanes == 4 || *cur_lanes == 4) {
        if (bindex & 0x3) {
            return SOC_E_PARAM;
        }
    } else if (lanes == 2 || *cur_lanes == 2) {
        if (bindex & 0x1) {
            return SOC_E_PARAM;
        }
    }

    if (lanes == *cur_lanes) {
        return SOC_E_NONE;
    }

    /* Figure out new mode */
    if (lanes == 4) {
        mode = SOC_HU2_PORT_MODE_SINGLE;
    } else if (lanes == 2) {
        if (cur_mode == SOC_HU2_PORT_MODE_QUAD) {
            mode = bindex == 0 ?
                SOC_HU2_PORT_MODE_TRI_023 : SOC_HU2_PORT_MODE_TRI_012;
        } else {
            mode = SOC_HU2_PORT_MODE_DUAL;
        }
    } else{
        if (cur_mode == SOC_HU2_PORT_MODE_DUAL) {
            mode = bindex == 0 ?
                SOC_HU2_PORT_MODE_TRI_012 : SOC_HU2_PORT_MODE_TRI_023;
        } else {
            mode = SOC_HU2_PORT_MODE_QUAD;
        }
    }

    *phy_ports_len = 0;
    if (lanes > *cur_lanes) { /* Figure out which port(s) to be removed */
        if (lanes == 4) {
            if (cur_mode == SOC_HU2_PORT_MODE_TRI_012 ||
                cur_mode == SOC_HU2_PORT_MODE_QUAD) {
                if (si->port_p2l_mapping[phy_port_base + 1] != -1) {
                    phy_ports[(*phy_ports_len)++] = phy_port_base + 1;
                }
            }
            if (si->port_p2l_mapping[phy_port_base + 2] != -1) {
                phy_ports[(*phy_ports_len)++] = phy_port_base + 2;
            }
            if (cur_mode == SOC_HU2_PORT_MODE_TRI_023 ||
                cur_mode == SOC_HU2_PORT_MODE_QUAD) {
                if (si->port_p2l_mapping[phy_port_base + 3] != -1) {
                    phy_ports[(*phy_ports_len)++] = phy_port_base + 3;
                }
            }
        } else {
            if (si->port_p2l_mapping[phy_port_base + 1] != -1) {
                phy_ports[(*phy_ports_len)++] = phy_port_base + 1;
            }
        }
    } else { /* Figure out which port(s) to be added */
        if (lanes == 2) {
            if (si->port_p2l_mapping[phy_port_base + 2] != -1) {
                phy_ports[(*phy_ports_len)++] = phy_port_base + 2;
            }
        } else {
            if (si->port_p2l_mapping[phy_port_base + 1] != -1) {
                phy_ports[(*phy_ports_len)++] = phy_port_base + 1;
            }
            if (cur_mode == SOC_HU2_PORT_MODE_SINGLE) {
                if (si->port_p2l_mapping[phy_port_base + 2] != -1) {
                    phy_ports[(*phy_ports_len)++] = phy_port_base + 2;
                }
                if (si->port_p2l_mapping[phy_port_base + 3] != -1) {
                    phy_ports[(*phy_ports_len)++] = phy_port_base + 3;
                }
            }
        }
    }

    if (soc_cm_debug_check(DK_VERBOSE)) {
        static char *mode_name[] = {
            "QUAD", "TRI_012", "TRI_023", "DUAL", "SINGLE"
        };
        soc_cm_print("port %d physical port %d bindex %d\n",
                     port_base, phy_port_base, bindex);
        soc_cm_print("mode (new:%s cur:%s) lanes (new:%d cur:%d)\n",
                     mode_name[mode], mode_name[cur_mode], lanes, *cur_lanes);
        for (i = 0; i < *phy_ports_len; i++) {
            soc_cm_print("%s physical port %d (port %d)\n",
                         lanes > *cur_lanes ? "del" : "add",
                         phy_ports[i], si->port_p2l_mapping[phy_ports[i]]);
        }
    }

    /* Update soc_control information */
    SOC_CONTROL_LOCK(unit);
    if (lanes > *cur_lanes) { /* port(s) to be removed */
        for (i = 0; i < *phy_ports_len; i++) {
            port = si->port_p2l_mapping[phy_ports[i]];
            SOC_PBMP_PORT_ADD(si->all.disabled_bitmap, port);
            if (IS_HG_PORT(unit, phy_ports[i])) {
                SOC_PBMP_PORT_REMOVE(si->st.bitmap, port);
                SOC_PBMP_PORT_REMOVE(si->hg.bitmap, port);
            } else {
                SOC_PBMP_PORT_REMOVE(si->ether.bitmap, port);
                if (IS_GE_PORT(unit, port)) {
                    SOC_PBMP_PORT_REMOVE(si->ge.bitmap, port);
                } else {
                    SOC_PBMP_PORT_REMOVE(si->xe.bitmap, port);
                }
            }
        }
    } else { /* port(s) to be added */
        for (i = 0; i < *phy_ports_len; i++) {
            port = si->port_p2l_mapping[phy_ports[i]];
            SOC_PBMP_PORT_REMOVE(si->all.disabled_bitmap, port);
            if (IS_HG_PORT(unit, port)) {
                SOC_PBMP_PORT_ADD(si->st.bitmap, port);
                SOC_PBMP_PORT_ADD(si->hg.bitmap, port);
            } else {
                SOC_PBMP_PORT_ADD(si->ether.bitmap, port);
                if (IS_GE_PORT(unit, port)) {
                    SOC_PBMP_PORT_ADD(si->ge.bitmap, port);
                } else {
                    SOC_PBMP_PORT_ADD(si->xe.bitmap, port);
                }
            }
        }
    }

#define RECONFIGURE_PORT_TYPE_INFO(ptype) \
    si->ptype.num = 0; \
    si->ptype.min = si->ptype.max = -1; \
    PBMP_ITER(si->ptype.bitmap, port) { \
        si->ptype.port[si->ptype.num++] = port; \
        if (si->ptype.min < 0) { \
            si->ptype.min = port; \
        } \
        if (port > si->ptype.max) { \
            si->ptype.max = port; \
        } \
    }

    RECONFIGURE_PORT_TYPE_INFO(ether);
    RECONFIGURE_PORT_TYPE_INFO(st);
    RECONFIGURE_PORT_TYPE_INFO(hg);
    RECONFIGURE_PORT_TYPE_INFO(xe);
    RECONFIGURE_PORT_TYPE_INFO(ge);

#undef RECONFIGURE_PORT_TYPE_INFO

    /* Update num of lanes info which is used by SerDes driver */
    SOC_PORT_NUM_LANES_SET(unit, port_base, lanes);
    for (i = 0; i < *phy_ports_len; i++) {
        port = si->port_p2l_mapping[phy_ports[i]];
        SOC_PORT_NUM_LANES_SET(unit, port, lanes > *cur_lanes ? 0 : lanes);
    }

    soc_dport_map_update(unit);
    SOC_CONTROL_UNLOCK(unit);

    fields[0] = XPORT0_CORE_PORT_MODEf;
    values[0] = mode;
    fields[1] = XPORT0_PHY_PORT_MODEf;
    values[1] = mode;
    SOC_IF_ERROR_RETURN(soc_reg_fields32_modify(unit, XLPORT_MODE_REGr,
                                                port_base, 2, fields, values));

    if (*phy_ports_len == 0) {
        return SOC_E_NONE;
    }

    /* Update TDM */
#ifdef _HURRICANE2_DEBUG
    SOC_IF_ERROR_RETURN
        (_soc_hurricane2_port_lanes_update_tdm(unit, port_base, lanes,
                                             *cur_lanes, phy_ports,
                                             *phy_ports_len));
#endif
    return SOC_E_NONE;
}


int
soc_hurricane2_port_lanes_get(int unit, soc_port_t port_base, int *cur_lanes)
{
    soc_info_t *si = &SOC_INFO(unit);
    int cur_mode;
    int phy_port_base, i;
    int blk, bindex;
    uint32 rval;

    /* Find physical port number and lane index of the specified port */
    phy_port_base = si->port_l2p_mapping[port_base];
    if (phy_port_base == -1) {
        return SOC_E_PORT;
    }

    bindex = -1;
    for (i = 0; i < SOC_DRIVER(unit)->port_num_blktype; i++) {
        blk = SOC_PORT_IDX_BLOCK(unit, phy_port_base, i);
        if (SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_XLPORT) {
            bindex = SOC_PORT_IDX_BINDEX(unit, phy_port_base, i);
            break;
        }
    }

    /* Get the current mode */
    SOC_IF_ERROR_RETURN(READ_XLPORT_MODE_REGr(unit, port_base, &rval));
    cur_mode = soc_reg_field_get(unit, XLPORT_MODE_REGr, rval,
                                 XPORT0_CORE_PORT_MODEf);

    /* Figure out the current number of lane from current mode */
    switch (cur_mode) {
    case SOC_HU2_PORT_MODE_QUAD:
        *cur_lanes = 1;
        break;
    case SOC_HU2_PORT_MODE_TRI_012:
        *cur_lanes = bindex == 0 ? 1 : 2;
        break;
    case SOC_HU2_PORT_MODE_TRI_023:
        *cur_lanes = bindex == 0 ? 2 : 1;
        break;
    case SOC_HU2_PORT_MODE_DUAL:
        *cur_lanes = 2;
        break;
    case SOC_HU2_PORT_MODE_SINGLE:
        *cur_lanes = 4;
        break;
    default:
        return SOC_E_FAIL;
    }

    return SOC_E_NONE;
}
#endif
