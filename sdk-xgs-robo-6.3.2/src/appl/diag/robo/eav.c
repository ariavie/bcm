/*
 * $Id: eav.c 1.16 Broadcom SDK $
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
 * Ethernet AV related CLI commands
 */

#include <sal/core/libc.h>
#include <sal/types.h>
#include <sal/appl/sal.h>
#include <sal/appl/io.h>
#include <sal/core/libc.h>

#include <soc/cm.h>
#include <soc/drv.h>

#include <bcm/types.h>
#include <bcm/rx.h>
#include <bcm/error.h>
#include <bcm_int/robo/rx.h>
#include <bcm/eav.h>

#include <appl/diag/shell.h>
#include <appl/diag/system.h>


/*
 * Macro:
 *     EAV_GET_NUMB
 * Purpose:
 *     Get a numerical value from stdin.
 */
#define EAV_GET_NUMB(numb, str, args) \
    if (((str) = ARG_GET(args)) == NULL) { \
        return CMD_USAGE; \
    } \
    (numb) = parse_integer(str);

/*
 * Macro:
 *     EAV_GET_PORT
 * Purpose:
 *     Get a numerical value from stdin.
 */
#define EAV_GET_PORT(_unit, _port, _str, _args)                   \
    if (((_str) = ARG_GET(_args)) == NULL) {                     \
        return CMD_USAGE;                                        \
    }                                                            \
    if (parse_port((_unit), (_str), &(_port)) < 0) {             \
       printk("ERROR: invalid port string: \"%s\"\n", (_str)); \
       return CMD_FAIL;                                          \
    }



static bcm_mac_t    eav_ts_mac = {0,0,0,0,0,0};
static bcm_mac_t    station_mac = {0,0x10,0x18,0x53,0x95,0x0};
static bcm_pkt_t pkt;

/* Default data for configuring RX system */
static bcm_rx_cfg_t eav_rx_cfg = {
    ROBO_RX_PKT_SIZE_DFLT,       /* packet alloc size */
    ROBO_RX_PPC_DFLT,            /* Packets per chain */
    0,                      /* Default pkt rate, global */
    1,                      /* Burst */
    {                       /* Just configure channel 1 */
        {                   /* Only Channel 0, default RX */
            ROBO_RX_CHAINS_DFLT, /* DV count (number of chains) */
            0,              /* Default pkt rate, DEPRECATED */
            0,              /* No flags */
            0xff            /* All COS to this channel */
        }
    },
    NULL,          /* alloc function */
    NULL,           /* free function */
    0                       /* flags */
};    
/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_types(int unit)
{

    printk("Control Type :\n");
    printk("   1 : Enable time stamp to IMP port.\n");
    printk("       Parameter = 0 means disable, otherwize enable.\n");
    printk("   2 : Max AV packet size.\n");
    printk("       Paramter indicates the max AV packets size.\n");
    printk("\n");
    printk("Time Sync Type :\n");
    printk("   1 : Time Base. p0 : time base value(ns)\n");
    printk("   2 : Time Adjust. p0 : increment time(ns) per tick.\n");
    printk("       p1 : adjust period(ticks).\n");
    printk("   3 : Tick Counter. p0 : tick counter.\n");
    printk("   4 : Slot Number. p0 : slot number.\n");
    printk("   5 : Macro Slot Time. p0 : slot time for  class 4 traffic.\n");
    printk("       only 1, 2, 4ms are allowed\n");
    printk("   6 : Slot Adjust. p0 : number of ticks in the slot.\n");
    printk("       only 3124, 3125, 3126 are allowed.\n");
    printk("       p1 : adjust period(slots).\n");
    printk("\n");
    printk("Queue Control Type :\n");
    printk("   1 : Queue 4 bandwidth. p0 = bandwidth (bytes/slot time)\n");
    printk("   2 : Queue 5 bandwidth. p0 = bandwidth (bytes/slot time)\n");
    printk("   3 : Queue 5 Window(jitter control).\n");
    printk("       p0 = 0 means disable, otherwize enable.\n");
    printk("\n");
    
    return CMD_OK;
}

STATIC bcm_rx_t
robo_eav_cb_handler(int unit, bcm_pkt_t *info, void *cookie)
{

    if (info->flags & BCM_TX_TIME_STAMP_REPORT) {
        /* Report Egress Transmit time */
        soc_cm_print("Receive EAV Egress Time Stamp Packet : tx_port = %d\n",
                info->rx_port);
        soc_cm_print("Depature time is 0x%x.\n", info->rx_timestamp);
    } else if (info->flags & BCM_PKT_F_TIMESYNC) {
        /* Received Time Sync Packets */
        soc_cm_print("Receive EAV Time Sync Packet : rx_port = %d\n", 
                info->rx_port);
        soc_cm_print("Time is 0x%x.\n", info->rx_timestamp);
    } else {
        return BCM_RX_NOT_HANDLED;
    }  

    return BCM_RX_HANDLED;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_watch(int unit, args_t *args)
{
    char*               subcmd = NULL;
    int rv;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }

    if(!sal_strcasecmp(subcmd, "start")) {
        /* Register to accept all cos */
        bcm_rx_start(unit, &eav_rx_cfg);
        if ((rv = bcm_rx_register(unit, "EAV", robo_eav_cb_handler,
                    101, NULL, BCM_RCO_F_ALL_COS)) < 0) {
            printk("%s: bcm_rx_register failed: %s\n",
                   ARG_CMD(args), bcm_errmsg(rv));
            return CMD_FAIL;
        }
    }

     if(!sal_strcasecmp(subcmd, "stop")) {
        bcm_rx_stop(unit, &eav_rx_cfg);
        if ((rv = bcm_rx_unregister(unit, robo_eav_cb_handler, 101)) < 0) {
            printk("%s: bcm_rx_unregister failed: %s\n",
                   ARG_CMD(args), bcm_errmsg(rv));
            return CMD_FAIL;
        }
     }
  
    return CMD_OK;
}


/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_init(int unit)
{
    soc_cm_print("Ethernet AV init.\n");

    /* 1. Perform Eav initialization*/
    bcm_eav_init(unit);

    /* 2. Get the timesync MACDA and EtherType */
    bcm_eav_timesync_mac_get(unit, eav_ts_mac);

    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_control_set(int unit, args_t *args)
{
    char*               subcmd = NULL;
    uint32     control_type, param;
    int rv = SOC_E_NONE;

    EAV_GET_NUMB(control_type, subcmd, args);
    EAV_GET_NUMB(param, subcmd, args);

    switch (control_type) {
        case 1:
            rv = DRV_EAV_CONTROL_SET
                    (unit, DRV_EAV_CONTROL_TIME_STAMP_TO_IMP, param);
            break;
        case 2:
            rv = DRV_EAV_CONTROL_SET
                    (unit, DRV_EAV_CONTROL_MAX_AV_SIZE, param);
            break;
        default:
            return CMD_USAGE;
    }
    if (rv < 0) {
     printk("eav control set : failed with control type = %d, parameter = %d\n",
            control_type, param);
        return CMD_FAIL;
    }

    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_control_get(int unit, args_t *args)
{
    char*               subcmd = NULL;
    uint32     control_type, param;
    int rv = SOC_E_NONE;

    EAV_GET_NUMB(control_type, subcmd, args);

    switch (control_type) {
        case 1:
            rv = DRV_EAV_CONTROL_GET
                    (unit, DRV_EAV_CONTROL_TIME_STAMP_TO_IMP, &param);
            if (rv == SOC_E_NONE) {
                soc_cm_print(" Time Stamp to IMP port is ");
                if (param == 0) {
                    soc_cm_print("Disabled.\n");
                } else {
                    soc_cm_print("Enabled.\n");
                }
            }
            break;
        case 2:
            rv = DRV_EAV_CONTROL_GET
                    (unit, DRV_EAV_CONTROL_MAX_AV_SIZE, &param);
            if (rv == SOC_E_NONE) {
                soc_cm_print(" Max Ethernet AV size  is %d\n", param);
            }
            break;
        default:
            return CMD_USAGE;
    }
    if (rv < 0) {
        soc_cm_print("eav control get : failed with control type = %d\n",
            control_type);
        return CMD_FAIL;
    }

    return CMD_OK;
}


/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_control(int unit, args_t *args)
{
    char*               subcmd = NULL;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "set")) {
        return robo_eav_control_set(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "get")) {
        return robo_eav_control_get(unit, args);
    }

    return CMD_USAGE;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_mac_set(int unit, args_t *args)
{
    char*               subcmd = NULL;
    int rv = CMD_OK;
    bcm_mac_t   robo_eav_mac;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if ((rv = parse_macaddr(subcmd, robo_eav_mac)) < 0) {
       printk("ERROR: invalid mac string: \"%s\" (error=%d)\n", subcmd, rv);
       return CMD_FAIL;
    }

    rv = bcm_eav_timesync_mac_set(unit, robo_eav_mac);
        
    if (rv == SOC_E_NONE) {
        sal_memcpy(eav_ts_mac, robo_eav_mac, 6);
        soc_cm_print("Set Time Sync MACDA = %02x-%02x-",
            robo_eav_mac[0], robo_eav_mac[1]);
        soc_cm_print("%02x-%02x-%02x-%02x\n",
            robo_eav_mac[2], robo_eav_mac[3], 
            robo_eav_mac[4], robo_eav_mac[5]);
    } else {
        soc_cm_print("Fail to set Time Sync MACDA!\n");
        return CMD_FAIL;
    }

    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_mac_get(int unit, args_t *args)
{   

    soc_cm_print("Get Time Sync MACDA = %02x-%02x-%02x-",
            eav_ts_mac[0], eav_ts_mac[1], eav_ts_mac[2]);
    soc_cm_print("%02x-%02x-%02x\n",
            eav_ts_mac[3], eav_ts_mac[4], eav_ts_mac[5]);

    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_mac(int unit, args_t *args)
{
    char*               subcmd = NULL;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "set")) {
        return robo_eav_mac_set(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "get")) {
        return robo_eav_mac_get(unit, args);
    }

    return CMD_USAGE;
}


/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_port(int unit, args_t *args)
{
    char*               subcmd = NULL;
    bcm_port_t      data_port;
    int  r;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "enable")) {
        EAV_GET_PORT(unit, data_port, subcmd, args);
        r = bcm_eav_port_enable_set(unit, data_port, 1);
        if (r < 0) {
            printk("%s: ERROR: %s\n", ARG_CMD(args), bcm_errmsg(r));
            return CMD_FAIL;
        } else {
            soc_cm_print("Port %d is AV enabled\n", data_port);
        }
    }
    
    if(!sal_strcasecmp(subcmd, "disable")) {
        EAV_GET_PORT(unit, data_port, subcmd, args);
        r = bcm_eav_port_enable_set(unit, data_port, 0);
        if (r < 0) {
            printk("%s: ERROR: %s\n", ARG_CMD(args), bcm_errmsg(r));
            return CMD_FAIL;
        } else {
            soc_cm_print("Port %d is AV disabled\n", data_port);
        }
    }

    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_link(int unit, args_t *args)
{
    char*               subcmd = NULL;
    bcm_port_t      data_port;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "on")) {
        EAV_GET_PORT(unit, data_port, subcmd, args);
        bcm_eav_link_status_set(unit, data_port, 1);
        soc_cm_print("Port %d's EAV link status is on\n", data_port);
    }
    
    if(!sal_strcasecmp(subcmd, "off")) {
        EAV_GET_PORT(unit, data_port, subcmd, args);
        bcm_eav_link_status_set(unit, data_port, 0);
        soc_cm_print("Port %d's EAV link status is off\n", data_port);
    }

    return CMD_OK;
}


/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
eav_queue_set(int unit, args_t *args)
{
    char*               subcmd = NULL;
    uint32     control_type, param;
    int rv = SOC_E_NONE;
    bcm_port_t      data_port;

    EAV_GET_PORT(unit, data_port, subcmd, args);
    EAV_GET_NUMB(control_type, subcmd, args);
    EAV_GET_NUMB(param, subcmd, args);

    switch (control_type) {
        case 1:
            rv = DRV_EAV_QUEUE_CONTROL_SET
                    (unit, data_port, DRV_EAV_QUEUE_Q4_BANDWIDTH, param);
            break;
        case 2:
            rv = DRV_EAV_QUEUE_CONTROL_SET
                    (unit, data_port, DRV_EAV_QUEUE_Q5_BANDWIDTH, param);
            break;
        case 3:
            rv = DRV_EAV_QUEUE_CONTROL_SET
                    (unit, data_port, DRV_EAV_QUEUE_Q5_WINDOW, param);
            break;
        default:
            return CMD_USAGE;
    }
    if (rv < 0) {
        soc_cm_print("eav queue control set : failed with port = %d, ",
            data_port);
        soc_cm_print("control type = %d, parameter = %d\n",
            control_type, param);
        return CMD_FAIL;
    }

    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
eav_queue_get(int unit, args_t *args)
{
    char*               subcmd = NULL;
    uint32     control_type, param;
    int rv = SOC_E_NONE;
    bcm_port_t      data_port;

    EAV_GET_PORT(unit, data_port, subcmd, args);
    EAV_GET_NUMB(control_type, subcmd, args);

    switch (control_type) {
        case 1:
            rv = DRV_EAV_QUEUE_CONTROL_GET
                    (unit, data_port, DRV_EAV_QUEUE_Q4_BANDWIDTH, &param);
            if (rv == SOC_E_NONE) {
     printk("Eav queue control get : port = %d, Q4 bandwidth = %d bytes/slot\n",
                data_port, param);
            }
            break;
        case 2:
            rv = DRV_EAV_QUEUE_CONTROL_GET
                    (unit, data_port, DRV_EAV_QUEUE_Q5_BANDWIDTH, &param);
            if (rv == SOC_E_NONE) {
     printk("Eav queue control get : port = %d, Q5 bandwidth = %d bytes/slot\n",
                data_port, param);
            }
            break;
        case 3:
            rv = DRV_EAV_QUEUE_CONTROL_GET
                    (unit, data_port, DRV_EAV_QUEUE_Q5_WINDOW, &param);
            if (rv == SOC_E_NONE) {
           printk("Eav queue control get : port = %d, Q5 window is ",data_port);
            if (param) {
                soc_cm_print("enabled.\n");
            } else {
                soc_cm_print("disabled.\n");
            }
            }
            break;
        default:
            return CMD_USAGE;
    }
    if (rv < 0) {
    printk("eav queue control get : failed with port = %d, control type = %d\n",
            data_port, control_type);
        return CMD_FAIL;
    }

    return CMD_OK;
}


/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_queue(int unit, args_t *args)
{
    char*               subcmd = NULL;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "set")) {
        return eav_queue_set(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "get")) {
        return eav_queue_get(unit, args);
    }

    return CMD_USAGE;
}


/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_status(int unit, args_t *args)
{
    bcm_port_t      port;
    uint32   temp = 0, temp2;
    int rv = SOC_E_NONE;

    soc_cm_print("Ethernet AV Status :\n");
    /* Per port AV enable */
    PBMP_GE_ITER(unit, port) {
        bcm_eav_port_enable_get(unit, port, (int *) &temp);
        if (temp) {
            soc_cm_print("GE%d is AV enabled.\n", port);
            /* EAV link LED */
            bcm_eav_link_status_get(unit, port, (int *) &temp);
            if (temp) {
                soc_cm_print("GE%d's EAV led is on.\n", port);
            } else {
                soc_cm_print("GE%d's EAV led is off.\n", port);
            }
            /* Q4 bandwidth */
            rv = DRV_EAV_QUEUE_CONTROL_GET
                    (unit, port, DRV_EAV_QUEUE_Q4_BANDWIDTH, &temp);
            if (rv == SOC_E_NONE) {
                soc_cm_print("    Q4 bandwidth = %d bytes/slot\n",
                temp);
            }
            /* Q5 bandwidth */
            rv = DRV_EAV_QUEUE_CONTROL_GET
                    (unit, port, DRV_EAV_QUEUE_Q5_BANDWIDTH, &temp);
            if (rv == SOC_E_NONE) {
                soc_cm_print("    Q5 bandwidth = %d bytes/slot\n",
                temp);
            }
            /* Q5 Window */
            rv = DRV_EAV_QUEUE_CONTROL_GET
                    (unit, port, DRV_EAV_QUEUE_Q5_WINDOW, &temp);
            if (rv == SOC_E_NONE) {
                soc_cm_print("    Q5 window is ");
                if (temp) {
                    soc_cm_print("enable.\n");
                } else {
                    soc_cm_print("disable.\n");
                }
            }
        } else {
            soc_cm_print("Port %d is AV disabled.\n", port);
        }
    }

    /* Max AV Packet Size */
    rv = bcm_eav_control_get(unit, bcmEAVControlMaxFrameSize, 
        &temp, &temp2);
    if (rv == SOC_E_NONE) {
        soc_cm_print(" Max Ethernet AV size  is %d\n", temp);
    }
    
    /* Time base */
    rv = bcm_eav_control_get(unit, bcmEAVControlTimeBase, 
        &temp, &temp2);
    if (rv == SOC_E_NONE) {
        soc_cm_print(" TimeSync : current Time Base = 0x%x ns.\n", temp);
    }
    
    /* Slot number */
    rv = bcm_eav_control_get(unit, bcmEAVControlSlotNumber, 
        &temp, &temp2);
    if (rv == SOC_E_NONE) {
        soc_cm_print(" TimeSync : current Slot Number = %d.\n", temp);
    }
    
    /* Tick number per slot */
    rv = bcm_eav_control_get(unit, bcmEAVControlTickCounter, 
        &temp, &temp2);
    if (rv == SOC_E_NONE) {
        soc_cm_print(" TimeSync : current Tick Counter = %d.\n", temp);
    }
    
    /* Macro slot time */
    rv = bcm_eav_control_get(unit, bcmEAVControlMacroSlotTime, 
        &temp, &temp2);
    if (rv == SOC_E_NONE) {
        soc_cm_print(" TimeSync : Macro Slot Period = %d ms.\n", temp);
    }

    soc_cm_print("Time Sync MACDA = %02x-%02x-%02x-",
            eav_ts_mac[0], eav_ts_mac[1], eav_ts_mac[2]);
    soc_cm_print("%02x-%02x-%02x\n", 
            eav_ts_mac[3], eav_ts_mac[4], eav_ts_mac[5]);
    
    return CMD_OK;
}


/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_timestamp(int unit, args_t *args)
{
    char*               subcmd = NULL;
    bcm_port_t      data_port;
    uint32  timestamp;
    int rv = SOC_E_NONE;

    EAV_GET_PORT(unit, data_port, subcmd, args);

    rv = bcm_eav_timestamp_get(unit, data_port, &timestamp);
    
    if (rv == SOC_E_NONE) {
        soc_cm_print("Port %d Egress Time Stamp = 0x%x\n", 
            data_port, timestamp);
    } else {
        soc_cm_print("Get Port %d Egress Time Stamp failed!\n", data_port);
    }

    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_tx(int unit, args_t *args)
{
    char*               subcmd = NULL;
    bcm_port_t      port;
    enet_hdr_t   *ep = NULL;    
    uint32 vlan = 0;        
    int rv = 0;
    pbmp_t  tx_pbmp, tx_upbmp;

    EAV_GET_PORT(unit, port, subcmd, args);
    EAV_GET_NUMB(vlan, subcmd, args);

    _SHR_PBMP_PORT_SET(tx_pbmp, port);
    _SHR_PBMP_CLEAR(tx_upbmp);
      
    /* alloc packet body */    
    pkt.alloc_ptr = (uint8 *)soc_cm_salloc(unit, 64, "TX");        
    if (pkt.alloc_ptr == NULL) {        
        printk("WARNING: Could not alloc tx buffer. Memory error.\n");    
    } else {        
        pkt._pkt_data.data = pkt.alloc_ptr;        
        pkt.pkt_data = &pkt._pkt_data;        
        pkt.blk_count = 1;        
        pkt._pkt_data.len = 64;    
    }        
    /* packet re-init */    
    sal_memset(pkt.pkt_data[0].data, 0, pkt.pkt_data[0].len);        
    ep = (enet_hdr_t *)(pkt.pkt_data[0].data);    
    /* setup the packet */    
    pkt.flags &= ~BCM_TX_CRC_FLD;    
    pkt.flags |= BCM_TX_CRC_REGEN; 
    pkt.flags |= BCM_TX_TIME_STAMP_REPORT; 
    
    /* check_pkt_fields */
    /* always tagged with priority 5*/
    {                      
        /* Tagged format */        
        ep->en_tag_ctrl = soc_ntohs(VLAN_CTRL(5, 0, vlan));        
        ep->en_tag_len  = soc_ntohs(64);        
        ep->en_tag_tpid = soc_ntohs(0x8100);    
    }
    /* assign pbmp */    
    sal_memcpy((uint8 *)&pkt.tx_pbmp, (uint8 *)&tx_pbmp, sizeof(bcm_pbmp_t));    
    sal_memcpy((uint8 *)&pkt.tx_upbmp, (uint8 *)&tx_upbmp, sizeof(bcm_pbmp_t));        
    /* assign mac addr */    
    ENET_SET_MACADDR(ep->en_dhost, eav_ts_mac);    
    ENET_SET_MACADDR(ep->en_shost, station_mac);
    
    if ((rv = bcm_tx(unit, &pkt, NULL)) != BCM_E_NONE) {        
        soc_cm_debug(DK_ERR, "bcm_tx failed: Unit %d: %s\n",                   
                     unit, bcm_errmsg(rv));        
    }

    return CMD_OK;
}


/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_srp_set(int unit, args_t *args)
{
    char*    subcmd = NULL;
    uint32   param;
    int rv = BCM_E_NONE;
    
    bcm_mac_t   srp_mac;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if ((rv = parse_macaddr(subcmd, srp_mac)) < 0) {
       printk("ERROR: invalid mac string: \"%s\" (error=%d)\n", subcmd, rv);
       return CMD_FAIL;
    }
    
    EAV_GET_NUMB(param, subcmd, args);
    
    rv = bcm_eav_srp_mac_ethertype_set(unit, srp_mac, (uint16)param);
    if (rv < 0) {
     printk("bcm_eav_srp_mac_ethertype_set : failed with mac = %02x:%02x:%02x:%02x:%02x:%02x, parameter = 0x%x\n",
            srp_mac[0], srp_mac[1], srp_mac[2], srp_mac[3], srp_mac[4], srp_mac[5], param);
        return CMD_FAIL;
    }

    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_srp_get(int unit, args_t *args)
{
    uint16   param;
    int rv = BCM_E_NONE;
    bcm_mac_t   srp_mac;
    
    rv = bcm_eav_srp_mac_ethertype_get(unit, srp_mac, &param);
    if (rv < 0) {
     printk("bcm_eav_srp_mac_ethertype_get : failed %s\n", bcm_errmsg(rv));
        return CMD_FAIL;
    }
    printk("SRP: MAC = %02x:%02x:%02x:%02x:%02x:%02x, Ethertype = 0x%x\n",
            srp_mac[0], srp_mac[1], srp_mac[2], srp_mac[3], srp_mac[4], srp_mac[5], param);
    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_srp(int unit, args_t *args)
{
    char*    subcmd = NULL;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "set")) {
        return robo_eav_srp_set(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "get")) {
        return robo_eav_srp_get(unit, args);
    }

    return CMD_USAGE;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
eav_timesync_set(int unit, args_t *args)
{
    char*               subcmd = NULL;
    uint32     control_type, p0, p1=0;
    int rv = SOC_E_NONE;

    EAV_GET_NUMB(control_type, subcmd, args);
    EAV_GET_NUMB(p0, subcmd, args);

    switch (control_type) {
        case 1:
            rv = DRV_EAV_TIME_SYNC_SET
                    (unit, DRV_EAV_TIME_SYNC_TIME_BASE, p0, p1);
            break;
        case 2:
            EAV_GET_NUMB(p1, subcmd, args);
            rv = DRV_EAV_TIME_SYNC_SET
                    (unit, DRV_EAV_TIME_SYNC_TIME_ADJUST, p0, p1);
            break;
        case 3:
            rv = DRV_EAV_TIME_SYNC_SET
                    (unit, DRV_EAV_TIME_SYNC_TICK_COUNTER, p0, p1);
            break;
        case 4:
            rv = DRV_EAV_TIME_SYNC_SET
                    (unit, DRV_EAV_TIME_SYNC_SLOT_NUMBER, p0, p1);
            break;
        case 5:
            rv = DRV_EAV_TIME_SYNC_SET
                    (unit, DRV_EAV_TIME_SYNC_MACRO_SLOT_PERIOD, p0, p1);
            break;
        case 6:
            EAV_GET_NUMB(p1, subcmd, args);
            rv = DRV_EAV_TIME_SYNC_SET
                    (unit, DRV_EAV_TIME_SYNC_SLOT_ADJUST, p0, p1);
            break;
        default:
            return CMD_USAGE;
    }
    if (rv < 0) {
  printk("eav timesync set : failed with control type = %d, p0 = %d, p1 = %d\n",
            control_type, p0, p1);
        return CMD_FAIL;
    }

    return CMD_OK;
}


/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
eav_timesync_get(int unit, args_t *args)
{
    char*               subcmd = NULL;
    uint32     control_type, p0, p1=0;
    int rv = SOC_E_NONE;

    EAV_GET_NUMB(control_type, subcmd, args);

    switch (control_type) {
        case 1:
            rv = bcm_eav_control_get(unit, bcmEAVControlTimeBase, 
                &p0, &p1);
            if (rv == SOC_E_NONE) {
                printk(" TimeSync Get : Time Base = 0x%x ns.\n", p0);
            }
            break;
        case 2:
            rv = bcm_eav_control_get(unit, bcmEAVControlTimeAdjust, 
                &p0, &p1);
            if (rv == SOC_E_NONE) {
  printk(" TimeSync Get : Time Increment = %d ns, Adjust Period = %d ticks.\n", 
                    p0, p1);
            }
            break;
        case 3:
            rv = bcm_eav_control_get(unit, bcmEAVControlTickCounter, 
                &p0, &p1);
            if (rv == SOC_E_NONE) {
                printk(" TimeSync Get : Tick Counter = %d.\n", p0);
            }
            break;
        case 4:
            rv = bcm_eav_control_get(unit, bcmEAVControlSlotNumber, 
                &p0, &p1);
            if (rv == SOC_E_NONE) {
                printk(" TimeSync Get : Slot Number = %d.\n", p0);
            }
            break;
        case 5:
            rv = bcm_eav_control_get(unit, bcmEAVControlMacroSlotTime, 
                &p0, &p1);
            if (rv == SOC_E_NONE) {
                printk(" TimeSync Get : Macro Slot Period = %d ms.\n", p0);
            }
            break;
        case 6:
            rv = bcm_eav_control_get(unit, bcmEAVControlSlotAdjust, 
                &p0, &p1);
            if (rv == SOC_E_NONE) {
   printk(" TimeSync Get : Slot Adjust = %d ticks, Adjust Period = %d slots.\n",
                    p0, p1);
            }
            break;
        default:
            return CMD_USAGE;
    }
    if (rv < 0) {
        printk("eav timesyncl get : failed with control type = %d\n",
            control_type);
        return CMD_FAIL;
    }

    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
robo_eav_timesync(int unit, args_t *args)
{
    char*               subcmd = NULL;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "set")) {
        return eav_timesync_set(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "get")) {
        return eav_timesync_get(unit, args);
    }

    return CMD_USAGE;
}

/*
 * Function:
 *      cmd_robo_eav
 * Purpose:
 *      Set/Display the Ethernet AV characteristics.
 * Parameters:
 *      unit - SOC unit #
 *      args - pointer to command line arguments      
 * Returns:
 *    CMD_OK
 */

cmd_result_t
cmd_robo_eav(int unit, args_t *args)
{
    char*                       subcmd = NULL;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
 
    if(!sal_strcasecmp(subcmd, "control")) {
        return robo_eav_control(unit, args);
    }
 
    if(!sal_strcasecmp(subcmd, "mac")) {
        return robo_eav_mac(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "types")) {
        return robo_eav_types(unit);
    }

    if(!sal_strcasecmp(subcmd, "init")) {
        return robo_eav_init(unit);
    }

    if(!sal_strcasecmp(subcmd, "port")) {
        return robo_eav_port(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "link")) {
        return robo_eav_link(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "queue")) {
        return robo_eav_queue(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "watch")) {
        return robo_eav_watch(unit, args);
    }  

    if(!sal_strcasecmp(subcmd, "status")) {
        return robo_eav_status(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "timestamp")) {
        return robo_eav_timestamp(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "timesync")) {
        return robo_eav_timesync(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "tx")) {
        return robo_eav_tx(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "srp")) {
        return robo_eav_srp(unit, args);
    }

    return CMD_USAGE;
}

    
