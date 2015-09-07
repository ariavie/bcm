/*
 * $Id: chassis.c 1.8 Broadcom SDK $
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
 * Chassis support
 */


#include <appl/diag/shell.h>
#include <appl/diag/system.h>

#if defined(MBZ)
extern int sysIsLM();
/* Chassis failover related stuff */

#include <soc/cm.h>
#include <soc/mem.h>
#include <soc/debug.h>

#include <bcm/link.h>

#define CFM1_PORT_LM   6
#define CFM2_PORT_LM   3
#define DRACO_UNIT1_PORT 8
#define DRACO_UNIT2_PORT 1
#define CFM1_PORT_CX4   5
#define CFM2_PORT_CX4  4
#define LYNX_UNIT1_PORT 6
#define LYNX_UNIT6_PORT 3
#define CFM1_PORT   ((cx4_flag) ? CFM1_PORT_CX4 : CFM1_PORT_LM)
#define CFM2_PORT   ((cx4_flag) ? CFM2_PORT_CX4 : CFM2_PORT_LM)
int cfm_hattached = 0;
int cx4_flag = 0;     


void
bcm_cx4_fabric_setup(int unit, bcm_port_t port)
{
    if (port == CFM1_PORT_CX4) {
        /* CFM in slot 1 */
        soc_cm_debug(
                DK_ERR, "bcm_cx4_fabric_setup: unit=%d port=%d CFM slot-1\n",
                unit, port);
        /* soc_icm_egress_mask_set(int unit, int port, pbmp_t pbmp)*/
        WRITE_ING_EGRMSKBMAPr(unit, LYNX_UNIT6_PORT, 0x020);
        WRITE_ING_EGRMSKBMAPr(unit, LYNX_UNIT1_PORT, 0x020);
        WRITE_ING_EGRMSKBMAPr(unit, CFM2_PORT_CX4, 0x048);
        WRITE_ING_EGRMSKBMAPr(unit, CFM1_PORT_CX4, 0x048);
    } else if (port == CFM2_PORT_CX4) {
        /* CFM in slot 2 */
        soc_cm_debug(DK_ERR,
                "bcm_cx4_fabric_setup: unit=%d port=%d CFM slot-2\n",
                unit, port);
        WRITE_ING_EGRMSKBMAPr(unit, LYNX_UNIT6_PORT, 0x010);
        WRITE_ING_EGRMSKBMAPr(unit, LYNX_UNIT1_PORT, 0x010);
        WRITE_ING_EGRMSKBMAPr(unit, CFM2_PORT_CX4, 0x048);
        WRITE_ING_EGRMSKBMAPr(unit, CFM1_PORT_CX4, 0x048);
    } else {
        /* Both CFMs active */
        soc_cm_debug(DK_ERR,
                "bcm_cx4_fabric_setup: unit=%d port=%d CFM slot-1&2\n",
                unit, port);
        WRITE_ING_EGRMSKBMAPr(unit, LYNX_UNIT6_PORT, 0x010);
        WRITE_ING_EGRMSKBMAPr(unit, LYNX_UNIT1_PORT, 0x020);
        WRITE_ING_EGRMSKBMAPr(unit, CFM2_PORT_CX4, 0x008);
        WRITE_ING_EGRMSKBMAPr(unit, CFM1_PORT_CX4, 0x040);
    }
}

void
bcm_fabric_setup(int unit, bcm_port_t port)
{
    if (port == CFM1_PORT_LM) {
        /* CFM in slot 1 */
        soc_cm_debug(DK_ERR, "bcm_fabric_setup: unit=%d port=%d CFM slot-1\n",
                unit, port);
        /* soc_icm_egress_mask_set(int unit, int port, pbmp_t pbmp)*/
        WRITE_ING_EGRMSKBMAPr(unit, DRACO_UNIT2_PORT, 0x142);
        WRITE_ING_EGRMSKBMAPr(unit, DRACO_UNIT1_PORT, 0x142);
    } else if (port == CFM2_PORT_LM) {
        /* CFM in slot 2 */
        soc_cm_debug(DK_ERR, "bcm_fabric_setup: unit=%d port=%d CFM slot-2\n",
                unit, port);
        WRITE_ING_EGRMSKBMAPr(unit, DRACO_UNIT2_PORT, 0x10a);
        WRITE_ING_EGRMSKBMAPr(unit, DRACO_UNIT1_PORT, 0x10a);
    } else {
        /* Both CFMs active */
        soc_cm_debug(DK_ERR, "bcm_fabric_setup: unit=%d port=%d CFM slot-1&2\n",
                unit, port);
        WRITE_ING_EGRMSKBMAPr(unit, DRACO_UNIT2_PORT, 0x10a);
        WRITE_ING_EGRMSKBMAPr(unit, DRACO_UNIT1_PORT, 0x142);
    }
}

void
bcm_cfm_fail_over(int unit, bcm_port_t port, bcm_port_info_t *info)
{
    int link_status;
    bcm_port_t cfm_port;

    cfm_port = port;

    soc_cm_debug(DK_ERR, "bcm_cfm_fail_over: unit = %d port = %d Link = %s\n",
                unit, port, info->linkstatus ? "Up" : "Down");
    if ((port != CFM1_PORT) && (port != CFM2_PORT))  {
        return;
    }

    if (info->linkstatus) {
        /* Link Up */
        if (bcm_port_link_status_get(unit, (port == CFM1_PORT) ?  CFM2_PORT :
                                CFM1_PORT, &link_status) >= 0) {
            if (link_status) {
                cfm_port = -1; /* Both CFMs active */
            }
        }
    } else {
        /* Link down */
        cfm_port = (port == CFM1_PORT) ? CFM2_PORT : CFM1_PORT;
    }

    if (cx4_flag) {
        bcm_cx4_fabric_setup(unit, cfm_port);
    } else {
        bcm_fabric_setup(unit, cfm_port);
    }
}


void bcm_cfm_failover_attach(int unit)
{
    int link_status;
    bcm_port_t cfm_port;

    if (unit != 0) return;

    cfm_port = CFM1_PORT;

    if (sysIsLM()) {
        if (bcm_port_link_status_get(unit, CFM1_PORT, &link_status) >= 0) {
            if (link_status) {
                cfm_port = CFM1_PORT;
                if (bcm_port_link_status_get(
                        unit, CFM2_PORT, &link_status) >= 0) {
                    if (link_status) {
                        cfm_port = -1; /* Both CFMs active */
                    }
                }
            }
            else {
                cfm_port = CFM2_PORT;
            }
        }

        if (cx4_flag) {
            bcm_cx4_fabric_setup(unit, cfm_port);
        } else {
            bcm_fabric_setup(unit, cfm_port);
        }

        soc_cm_debug(DK_ERR, "bcm_cfm_failover_attach: unit = %d %s\n",
                    unit, "Registered Handler for CFM failover"); 
        bcm_linkscan_register(unit, bcm_cfm_fail_over);
    }
    cfm_hattached = 1;
}

void bcm_cfm_failover_detach(int unit)
{
    bcm_linkscan_unregister(unit, bcm_cfm_fail_over);
    cfm_hattached = 0;
}

/*
 * Function:
 *	if_cfm_failover
 * Purpose:
 *	Activate/Deactivate CFM failover
 * Parameters:
 *	unit - SOC unit #
 *	a - pointer to args
 * Returns:
 *	CMD_OK/CMD_FAIL
 */

char if_cfm_failover_usage[] =
    "Activate CFM failover.\n";

cmd_result_t
if_cfm_failover(int unit, args_t *a)
{
    char           *c;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    if ((c = ARG_GET(a)) != NULL) {
        if (!sal_strcmp(c, "cx4")) {
            cx4_flag = 1; 
            c = ARG_GET(a);
        }
    }

    if (c  != NULL) {
        if (!sal_strcmp(c, "enable")) {
            if (cfm_hattached) bcm_cfm_failover_detach(unit);
            bcm_cfm_failover_attach(unit);
        } else if (!sal_strcmp(c, "disable")) {
            bcm_cfm_failover_detach(unit);
        } else {
            printk("cfmfailover [enable/disable] \n");
        }
    } else {
            printk("cfmfailover [enable/disable] \n");
    }

    return CMD_OK;
}

#endif

int _bcm_diag_chassis_not_empty;
