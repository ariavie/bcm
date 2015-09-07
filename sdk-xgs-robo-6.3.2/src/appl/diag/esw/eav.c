/*
 * $Id: eav.c 1.12 Broadcom SDK $
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
#include <bcm_int/esw/rx.h>
#include <bcm/error.h>
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
    if (parse_bcm_port((_unit), (_str), &(_port)) < 0) { \
       printk("ERROR: invalid port string: \"%s\"\n", (_str)); \
       return CMD_FAIL;                                          \
    }

static bcm_mac_t    eav_ts_mac = {0,0,0,0,0,0};
static bcm_mac_t    station_mac = {0,0x10,0x18,0x53,0x31,0x40};
static bcm_pkt_t pkt;

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
eav_types(int unit)
{
    printk("Control Type :\n");
    printk("   0 : Time Reference. p0 : time reference value(read only).\n");
    printk("   1 : Time Adjust.    p0 : adjust mode.\n");
    printk("                       p1 : Operation attributes).\n");
    printk("\n");
    printk("Stream Class Type :\n");
    printk("   0 : Class A traffic stream\n");
    printk("   1 : Class B traffic stream\n");
    printk("\n");
    
    return CMD_OK;
}

STATIC bcm_rx_t
eav_cb_handler(int unit, bcm_pkt_t *info, void *cookie)
{
    bcm_rx_t rv = BCM_RX_NOT_HANDLED;
    uint8 *pkt_data = BCM_PKT_DMAC(info);
    
    /* Received Time Sync Packets */
    if (BCM_RX_REASON_GET(info->rx_reasons, bcmRxReasonTimeSync)) {
        printk("[EAV watch] Time Sync packets from port %d, timestamp: 0x%08x\n", info->rx_port, info->rx_timestamp);
        /* Check if DA is Time Sync MAC DA */
        if (sal_memcmp(eav_ts_mac, pkt_data, 6)) {
            printk("[EAV watch] NOT matched MACDA.\n");
        }
        rv = BCM_RX_HANDLED; 
    } else if (BCM_RX_REASON_GET(info->rx_reasons, bcmRxReasonEAVData)) {
        printk("[EAV watch] EAV Data packets from port %d, priority %d.\n",
                info->rx_port, info->prio_int);
        rv = BCM_RX_HANDLED; 
    }
    return rv;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
eav_watch(int unit, args_t *args)
{
    char*               subcmd = NULL;
    int rv;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }

    if(!sal_strcasecmp(subcmd, "start")) {
        /* Register to accept all cos */
        bcm_rx_start(unit, NULL);
        if ((rv = bcm_rx_register(unit, "EAV", eav_cb_handler,
                    101, NULL, BCM_RCO_F_ALL_COS)) < 0) {
            printk("%s: bcm_rx_register failed: %s\n",
                   ARG_CMD(args), bcm_errmsg(rv));
            return CMD_FAIL;
        }
    }

     if(!sal_strcasecmp(subcmd, "stop")) {
        bcm_rx_stop(unit, NULL);
        if ((rv = bcm_rx_unregister(unit, eav_cb_handler, 101)) < 0) {
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
eav_init(int unit)
{
    printk("Ethernet AV init.\n");

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
eav_control_set(int unit, args_t *args)
{
    char*      subcmd = NULL;
    uint32     control_type, param0, param1;
    int rv = BCM_E_NONE;

    EAV_GET_NUMB(control_type, subcmd, args);
    EAV_GET_NUMB(param0, subcmd, args);
    
    switch (control_type) {
        case 1:
            EAV_GET_NUMB(param1, subcmd, args);
            rv = bcm_eav_control_set(unit, bcmEAVControlTimeAdjust, param0, param1);
            break;
        default:
            return CMD_USAGE;
    }
    if (rv < 0) {
        printk("eav control set : failed with control type = %d, parameter0 = %d\n",
            control_type, param0);
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
eav_control_get(int unit, args_t *args)
{
    char*      subcmd = NULL;
    uint32     control_type, param0, param1;
    int rv = BCM_E_NONE;

    EAV_GET_NUMB(control_type, subcmd, args);

    switch (control_type) {
        case 0:
            rv = bcm_eav_control_get(unit, bcmEAVControlTimeBase, &param0, &param1);
            if (rv == BCM_E_NONE) {
                printk("Time Reference = 0x%x\n", param0);
            }
            break;
        case 1:
            rv = bcm_eav_control_get(unit, bcmEAVControlTimeAdjust, &param0, &param1);
            if (rv == BCM_E_NONE) {
                printk("Time Reference mode = 0x%x, Operation Attributes= 0x%x\n",
                        param0, param1);
            }
            break;
        default:
            return CMD_USAGE;
    }
    if (rv < 0) {
        printk("eav control get : failed with control type = %d\n",
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
eav_control(int unit, args_t *args)
{
    char*               subcmd = NULL;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "set")) {
        return eav_control_set(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "get")) {
        return eav_control_get(unit, args);
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
eav_mac_set(int unit, args_t *args)
{
    char*               subcmd = NULL;
    int rv = BCM_E_NONE;
    bcm_mac_t   eav_mac;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if ((rv = parse_macaddr(subcmd, eav_mac)) < 0) {
       printk("ERROR: invalid mac string: \"%s\" (error=%d)\n", subcmd, rv);
       return CMD_FAIL;
    }

    rv = bcm_eav_timesync_mac_set(unit, eav_mac);
        
    if (rv == BCM_E_NONE) {
        sal_memcpy(eav_ts_mac, eav_mac, 6);
        printk("Set Time Sync MACDA = %02x-%02x-",
            eav_mac[0], eav_mac[1]);
        printk("%02x-%02x-%02x-%02x\n",
            eav_mac[2], eav_mac[3], 
            eav_mac[4], eav_mac[5]);
    } else {
        printk("Fail to set Time Sync MACDA!\n");
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
eav_mac_get(int unit, args_t *args)
{   

    printk("Get Time Sync MACDA = %02x-%02x-%02x-",
            eav_ts_mac[0], eav_ts_mac[1], eav_ts_mac[2]);
    printk("%02x-%02x-%02x\n",
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
eav_mac(int unit, args_t *args)
{
    char*               subcmd = NULL;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "set")) {
        return eav_mac_set(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "get")) {
        return eav_mac_get(unit, args);
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
eav_port(int unit, args_t *args)
{
    char*               subcmd = NULL;
    bcm_port_t      data_port;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "enable")) {
        EAV_GET_PORT(unit, data_port, subcmd, args);
        bcm_eav_port_enable_set(unit, data_port, 1);
        printk("Port %d is AV enabled\n", data_port);
    }
    
    if(!sal_strcasecmp(subcmd, "disable")) {
        EAV_GET_PORT(unit, data_port, subcmd, args);
        bcm_eav_port_enable_set(unit, data_port, 0);
        printk("Port %d is AV disabled\n", data_port);
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
eav_status(int unit, args_t *args)
{
    bcm_port_config_t pcfg;
    bcm_port_t      port;
    int      enable;
    uint32   temp, temp2;
    int rv = BCM_E_NONE;

    if (bcm_port_config_get(unit, &pcfg) != BCM_E_NONE) {
        printk("%s: Error: bcm ports not initialized\n", ARG_CMD(args));
        return CMD_FAIL;
    }

    printk("Ethernet AV Status :\n");
    /* Per port AV enable */
    BCM_PBMP_ITER(pcfg.ge, port) {
        bcm_eav_port_enable_get(unit, port, &enable);
        if (enable) {
            printk("Port %d is EAV enabled.\n", port);
            /* Class A bandwidth */
            rv = bcm_eav_bandwidth_get(unit, port, bcmEAVCStreamClassA, &temp, &temp2);
            if (rv == BCM_E_NONE) {
                printk("    Class A bandwidth = %d bytes/sec burst = %d bytes\n",
                temp, temp2);
            }
            
            /* Class B bandwidth */
            rv = bcm_eav_bandwidth_get(unit, port, bcmEAVCStreamClassB, &temp, &temp2);
            if (rv == BCM_E_NONE) {
                printk("    Class B bandwidth = %d bytes/sec burst = %d bytes\n",
                temp, temp2);
            }
        } else {
            printk("Port %d is EAV disabled.\n", port);
        }
    }

    /* Time Adjust */
    rv = bcm_eav_control_get(unit, bcmEAVControlTimeAdjust, 
        &temp, &temp2);
    if (rv == BCM_E_NONE) {
        printk(" Time Reference mode = 0x%x, Attributes = 0x%x.\n",
                       temp, temp2);
    }
    
    printk("Time Sync MAC Address = %02x-%02x-%02x-",
            eav_ts_mac[0], eav_ts_mac[1], eav_ts_mac[2]);
    printk("%02x-%02x-%02x\n", 
            eav_ts_mac[3], eav_ts_mac[4], eav_ts_mac[5]);
    
    return CMD_OK;
}


/*
 * Function:
 * Purpose:
 *     Read back the timestamp value of last egressed TS packets of the port.
 * Parmameters:
 * Returns:
 */
STATIC int
eav_timestamp(int unit, args_t *args)
{
    char*       subcmd = NULL;
    bcm_port_t  data_port;
    uint32      timestamp;
    int rv = BCM_E_NONE;

    EAV_GET_PORT(unit, data_port, subcmd, args);

    rv = bcm_eav_timestamp_get(unit, data_port, &timestamp);
    
    if (rv == BCM_E_NONE) {
        printk("Port %d Egress Time Stamp = 0x%x\n", 
            data_port, timestamp);
    } else {
        printk("Get Port %d Egress Time Stamp failed!\n", data_port);
    }

    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 *    Transmit one Sync message Time-Sync packet with flag BCM_PKT_F_TIMESYNC
 *    enabled to check if egress timestamp could be genenrated.
 * Parmameters:
 * Returns:
 */
STATIC int
eav_tx(int unit, args_t *args)
{
    char* subcmd = NULL;
    bcm_port_t port;
    enet_hdr_t *ep = NULL;    
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
        return CMD_FAIL;
    } else {        
        pkt._pkt_data.data = pkt.alloc_ptr;        
        pkt.pkt_data = &pkt._pkt_data;        
        pkt.blk_count = 1;        
        pkt._pkt_data.len = 64;    
    }        
    /* packet re-init, assume MessageType zero, Sync message */
    sal_memset(pkt.pkt_data[0].data, 0, pkt.pkt_data[0].len);        
    ep = (enet_hdr_t *)(pkt.pkt_data[0].data);    
    /* setup the packet */    
    pkt.flags &= ~BCM_TX_CRC_FLD;    
    pkt.flags |= BCM_TX_CRC_REGEN; 
    pkt.flags |= BCM_PKT_F_TIMESYNC; 
    
    /* always tagged with priority 5 */
    {                      
        ep->en_tag_tpid = bcm_htons(0x8100);
        ep->en_tag_ctrl = bcm_htons(VLAN_CTRL(5, 0, vlan));        
        ep->en_tag_len  = bcm_htons(0x88F7);
    }
    /* assign pbmp */    
    sal_memcpy((uint8 *)&pkt.tx_pbmp, (uint8 *)&tx_pbmp, sizeof(bcm_pbmp_t));    
    sal_memcpy((uint8 *)&pkt.tx_upbmp, (uint8 *)&tx_upbmp, sizeof(bcm_pbmp_t));        
    /* assign mac addr */    
    ENET_SET_MACADDR(ep->en_dhost, eav_ts_mac);    
    ENET_SET_MACADDR(ep->en_shost, station_mac);
    
    if ((rv = bcm_tx(unit, &pkt, NULL)) != BCM_E_NONE) {        
        soc_cm_sfree(unit, pkt.alloc_ptr);
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
eav_srp_set(int unit, args_t *args)
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
eav_srp_get(int unit, args_t *args)
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
eav_srp(int unit, args_t *args)
{
    char*    subcmd = NULL;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "set")) {
        return eav_srp_set(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "get")) {
        return eav_srp_get(unit, args);
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
eav_pcp_set(int unit, args_t *args)
{
    char*    subcmd = NULL;
    uint32   streamclass, pcp, remapped_pcp;
    int rv = BCM_E_NONE;
    
    EAV_GET_NUMB(streamclass, subcmd, args);
    EAV_GET_NUMB(pcp, subcmd, args);
    EAV_GET_NUMB(remapped_pcp, subcmd, args);
     
    rv = bcm_eav_pcp_mapping_set(unit, (int)streamclass, (int)pcp, (int)remapped_pcp);
    if (rv < 0) {
        printk("bcm_eav_pcp_mapping_set : failed %s\n", bcm_errmsg(rv));
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
eav_pcp_get(int unit, args_t *args)
{
    char*    subcmd = NULL;
    uint32   streamclass;
    int      pcp, remapped_pcp;
    int rv = BCM_E_NONE;
    
    EAV_GET_NUMB(streamclass, subcmd, args);
    rv = bcm_eav_pcp_mapping_get(unit, (int)streamclass, &pcp, &remapped_pcp);
    if (rv < 0) {
        printk("bcm_eav_pcp_mapping_get : failed %s\n", bcm_errmsg(rv));
        return CMD_FAIL;
    }
    
    printk("EAV Class %s: PCP = %d, Remapped PCP = %d\n",
            (streamclass == 0)?"A":"B", pcp, remapped_pcp);
            
    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
eav_pcp(int unit, args_t *args)
{
    char*    subcmd = NULL;
    
    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "set")) {
        return eav_pcp_set(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "get")) {
        return eav_pcp_get(unit, args);
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
eav_bandwidth_set(int unit, args_t *args)
{
    char*    subcmd = NULL;
    uint32   streamclass, bw, burst;
    bcm_port_t data_port;
    int rv = BCM_E_NONE;
    
    EAV_GET_PORT(unit, data_port, subcmd, args);
    EAV_GET_NUMB(streamclass, subcmd, args);
    EAV_GET_NUMB(bw, subcmd, args);
    EAV_GET_NUMB(burst, subcmd, args);
     
    rv = bcm_eav_bandwidth_set(unit, data_port, (int)streamclass, bw, burst);
    if (rv < 0) {
        printk("bcm_eav_bandwidth_set : failed %s\n", bcm_errmsg(rv));
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
eav_bandwidth_get(int unit, args_t *args)
{
    char*    subcmd = NULL;
    uint32   streamclass, bw, burst;
    bcm_port_t data_port;
    int rv = BCM_E_NONE;
    
    EAV_GET_PORT(unit, data_port, subcmd, args);
    EAV_GET_NUMB(streamclass, subcmd, args);
     
    rv = bcm_eav_bandwidth_get(unit, data_port, (int)streamclass, &bw, &burst);
    if (rv < 0) {
        printk("bcm_eav_bandwidth_get : failed %s\n", bcm_errmsg(rv));
        return CMD_FAIL;
    }
    
    printk("Port %d (Class %s): Bandwidth = %d bytes/sec, Burst = %d bytes\n",
            data_port, (streamclass == 0)?"A":"B", bw, burst);
            
    return CMD_OK;
}

/*
 * Function:
 * Purpose:
 * Parmameters:
 * Returns:
 */
STATIC int
eav_bandwidth(int unit, args_t *args)
{
    char*    subcmd = NULL;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    
    if(!sal_strcasecmp(subcmd, "set")) {
        return eav_bandwidth_set(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "get")) {
        return eav_bandwidth_get(unit, args);
    }

    return CMD_USAGE;
}

/*
 * Function:
 *      cmd_esw_eav
 * Purpose:
 *      Set/Display the Ethernet AV characteristics.
 * Parameters:
 *      unit - SOC unit #
 *      args - pointer to command line arguments      
 * Returns:
 *    CMD_OK
 */

cmd_result_t
cmd_esw_eav(int unit, args_t *args)
{
    char    *subcmd = NULL;

    if ((subcmd = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
 
    if(!sal_strcasecmp(subcmd, "control")) {
        return eav_control(unit, args);
    }
 
    if(!sal_strcasecmp(subcmd, "mac")) {
        return eav_mac(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "types")) {
        return eav_types(unit);
    }

    if(!sal_strcasecmp(subcmd, "init")) {
        return eav_init(unit);
    }

    if(!sal_strcasecmp(subcmd, "port")) {
        return eav_port(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "watch")) {
        return eav_watch(unit, args);
    }  

    if(!sal_strcasecmp(subcmd, "status")) {
        return eav_status(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "timestamp")) {
        return eav_timestamp(unit, args);
    }

    if(!sal_strcasecmp(subcmd, "tx")) {
        return eav_tx(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "srp")) {
        return eav_srp(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "pcp")) {
        return eav_pcp(unit, args);
    }
    
    if(!sal_strcasecmp(subcmd, "bandwidth")) {
        return eav_bandwidth(unit, args);
    }
    
    return CMD_USAGE;
}

    
