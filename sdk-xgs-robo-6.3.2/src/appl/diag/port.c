/*
 * $Id: port.c 1.286 Broadcom SDK $
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
 * File:        port.c
 * Purpose:     Functions to support CLI port commands
 * Requires:    
 */

#include <sal/core/libc.h>
#include <sal/types.h>
#include <sal/appl/sal.h>
#include <sal/appl/io.h>
#include <sal/core/libc.h>

#ifdef INCLUDE_PHY_SYM_DBG
/* For timebeing adding the headers for socket calls directly */
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#ifdef VXWORKS
#include <vxWorks.h>
#include <sockLib.h>
#include <selectLib.h>
#else
#include<sys/select.h>
#endif
#endif



#include <assert.h>

#include <soc/debug.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT)
#include <soc/xaui.h>
#endif
#include <soc/phyctrl.h>
#include <soc/phy.h>
#include <soc/eyescan.h>

#include <appl/diag/shell.h>
#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/diag.h>
#include <appl/diag/dport.h>

#include <bcm/init.h>
#include <bcm/port.h>
#include <bcm/stg.h>
#include <bcm/error.h>

#if defined(BCM_ESW_SUPPORT) 
#include <bcm_int/esw/port.h>
#endif

#if defined(BCM_SBX_SUPPORT)
#include <soc/sbx/sbx_drv.h>
#endif


#ifdef BCM_DFE_SUPPORT
#include <bcm_int/dfe/port.h>
#endif

#ifdef BCM_DPP_SUPPORT
#include <bcm_int/dpp/port.h>
#endif
/*
 * Function:
 *	_if_fmt_speed
 * Purpose:
 *	Format a speed as returned from bcm_xxx for display.
 * Parameters:
 *	b     - buffer to format into.
 *	speed - speed as returned from bcm_port_speed_get
 * Returns:
 *	Pointer to buffer (b).
 */
static char *
_if_fmt_speed(char *b, int speed)
{
    if (speed >= 1000) {                /* Use Gb */
        if (speed % 1000) {             /* Use Decimal */
            sal_sprintf(b, "%d.%dG", speed / 1000, (speed % 1000) / 100);
        } else {
            sal_sprintf(b, "%dG", speed / 1000);
        }
    } else if (speed == 0) {
	sal_sprintf(b, "-");
    } else {                            /* Use Mb */
        sal_sprintf(b, "%dM", speed);
    }
    return(b);
}

/*
 * These are ordered according the corresponding enumerated types.
 * See soc/portmode.h, bcm/port.h and bcm/link.h for more information
 */

/* Note:  See eyescan.h soc_stat_eyscan_counter_e */
char    *eyescan_counter[] = {
	"RelativePhy", "PrbsPhy", "PrbsMac", "CrcMac", "BerMac", "Custom", NULL
};

/* Note:  See port.h, bcm_port_discard_e */
char    *discard_mode[] = {
    "None", "All", "Untag", "Tag", NULL, NULL
};
/* Note:  See link.h, bcm_linkscan_mode_e */
char            *linkscan_mode[] = {
    "None", "SW", "HW", "OFF", "ON", NULL
};
/* Note:  See portmode.h, soc_port_if_e */
char            *interface_mode[] = SOC_PORT_IF_NAMES_INITIALIZER;

/* Note:  See portmode.h, soc_port_ms_e */
char            *phymaster_mode[] = {
    "Slave", "Master", "Auto", "None", NULL
};
/* Note:  See port.h, bcm_port_loopback_e */
char            *loopback_mode[] = {
    "NONE", "MAC", "PHY", "RMT", "C57", NULL
};
/* Note:  See port.h, bcm_port_stp_e */
char            *forward_mode[] = {
    "Disable", "Block", "LIsten", "LEarn", "Forward", NULL
};
/* Note: See port.h, bcm_port_encap_e */
char            *encap_mode[] = {
    "IEEE", "HIGIG", "B5632", "HIGIG2", NULL
};
/* Note: See port.h, bcm_port_mdix_e */
char            *mdix_mode[] = {
    "Auto", "ForcedAuto", "ForcedNormal", "ForcedXover", NULL
};
/* Note: See port.h, bcm_port_mdix_status_e */
char            *mdix_status[] = {
    "Normal", "Xover", NULL
};
/* Note: See port.h, bcm_port_medium_t */
char           *medium_status[] = {
    "None", "Copper", "Fiber", NULL
};
/* Note: See port.h, bcm_port_mcast_flood_t */
char           *mcast_flood[] = {
    "FloodAll", "FloodUnknown", "FloodNone", NULL
};
/* Note: See port.h, bcm_port_phy_control_t */
char           *phy_control[] = {
    "WAN                     ", 
    "Preemphasis             ", 
    "DriverCurrent           ", 
    "PreDriverCurrent        ", 
    "EqualizerBoost          ", 
    "Interface               ",
    "InterfaceMAX            ",
    "MacsecSwitchFixed       ", 
    "MacsecSwitchFixedSpeed  ", 
    "MacsecSwitchFixedDuplex ", 
    "MacsecSwitchFixedPause  ", 
    "MacsecPauseRxForward    ", 
    "MacsecPauseTxForward    ", 
    "MacsecLineIPG           ", 
    "MacsecSwitchIPG         ", 
    "SPeed                   ",
    "PAirs                   ",
    "GAin                    ",
    "AutoNeg                 ",
    "LocalAbility            ",
    "RemoteAbility      (RO) ",
    "CurrentAbility     (RO) ",
    "MAster                  ", 
    "Active             (RO) ", 
    "ENable                  ", 
    "PrePremphasis           ",
    "Encoding                ",
    "Scrambler               ",
    "PrbsPolynomial          ",
    "PrbsTxInvertData        ",
    "PrbsTxEnable            ",
    "PrbsRxEnable            ",
    "PrbsRxStatus            ",
    "SerdesDriverTune        ",
    "SerdesDriverEqualTuneStatus",
    "8b10b                   ",
    NULL
};

void
brief_port_info_header(int unit)
{
    char *disp_str =
	"%10s "          /* port number */
	"%4s "           /* enable/link state */
	"%7s "           /* speed/duplex */
	"%4s "           /* link scan mode */
	"%4s "           /* auto negotiate? */
	"%7s   "         /* spantree state */
	"%5s  "          /* pause tx/rx */
	"%6s "           /* discard mode */
	"%3s "           /* learn to CPU, ARL, FWD or discard */
	"%6s "           /* interface */
	"%5s "           /* max frame */
	"%5s\n";         /* loopback */

    printk(disp_str,
	   " ",          /* port number */
	   "ena/",       /* enable/link state */
	   "speed/",     /* speed/duplex */
	   "link",       /* link scan mode */
	   "auto",       /* auto negotiate? */
	   " STP ",      /* spantree state */
	   " ",          /* pause tx/rx */
	   " ",          /* discard mode */
	   "lrn",        /* learn to CPU, ARL, FWD or discard */
	   "inter",      /* interface */
	   "max",        /* max frame */
	   "loop");      /* loopback */
    printk(disp_str,
	   "port",       /* port number */
	   "link",       /* enable/link state */
	   "duplex",     /* speed/duplex */
	   "scan",       /* link scan mode */
	   "neg?",       /* auto negotiate? */
	   "state",      /* spantree state */
	   "pause",      /* pause tx/rx */
	   "discrd",     /* discard mode */
	   "ops",        /* learn to CPU, ARL, FWD or discard */
	   "face",       /* interface */
	   "frame",      /* max frame */
	   "back");      /* loopback */
}



#ifdef INCLUDE_PHY_SYM_DBG
/* 
 * Interface to PHY GUI for debugging 
 */
/*
 * MDIO Message format
 */
typedef struct phy_sym_dbg_mdio_msg_s
{
    uint8 op;
    uint8 port_id;
    uint8 device;
    uint8 c45;
    uint16 reg_addr;
    uint16 data;
}phy_sym_dbg_mdio_msg_t;

enum phy_sum_dbg_mdio_op {
    MDIO_HANDSHAKE,
    MDIO_WRITE,
    MDIO_READ,
    MDIO_RESP,
    MDIO_ERROR,
    MDIO_CLOSE
};
#define PHY_SYM_DBG_MDIO_MSG_LEN sizeof(phy_sym_dbg_mdio_msg_t)

typedef struct sym_dbg_s {
    int unit;
    int tcpportnum;
    sal_mutex_t sym_dbg_lock;
    int flags;
}sym_dbg_t;

sym_dbg_t sym_dbg_arg;

#define SYM_DBG_LOCK(sym_dbg_arg) sal_mutex_take(sym_dbg_arg.sym_dbg_lock, \
				                 sal_mutex_FOREVER)
#define SYM_DBG_UNLOCK(sym_dbg_arg) sal_mutex_give(sym_dbg_arg.sym_dbg_lock)

#define SYM_DBG_THREAD_START 1
#define SYM_DBG_THREAD_STOP  2

#define PHY_SYM_DBG_DEBUG_PRINT  0


static void
_phy_sym_debug_server(void *unit)
{
    int sockfd, newsockfd, clilen;
    struct sockaddr_in serv_addr, client_addr;
    phy_sym_dbg_mdio_msg_t buffer, mdio_rsp;
    uint16 phy_data, reg_addr, data;
    int n, rv = 0;
    int u, tcpportno;
    int flags = 0;
    struct timeval tout;
    fd_set sock_fdset;
    int i;

    /* Not yet initialized */
    SYM_DBG_LOCK(sym_dbg_arg);
    flags = sym_dbg_arg.flags;
    SYM_DBG_UNLOCK(sym_dbg_arg);

    if (!(flags & SYM_DBG_THREAD_START)) {
        sal_thread_exit(-1);
    }

    u = sym_dbg_arg.unit;
    tcpportno = sym_dbg_arg.tcpportnum;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printk("Error opeing a socket\n");
        sal_thread_exit(-1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(tcpportno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printk("Error: socket bind falied\n");
        close(sockfd);
        sal_thread_exit(-1);
    }
    printk("Listening on the port %d for client connection\n", tcpportno);

    listen(sockfd, 1);
    clilen = sizeof(client_addr);
    printk("accepting on the socket %d for client connection\n", sockfd);

    while(1) {
        FD_ZERO(&sock_fdset);
        FD_SET(sockfd, &sock_fdset);
        newsockfd = -1;
        do {
#if PHY_SYM_DBG_DEBUG_PRINT
            printk("Calling select\n");
#endif

            tout.tv_sec = 20; /* 5 seconds */
            tout.tv_usec = 0; 
            newsockfd = select(FD_SETSIZE, &sock_fdset, &sock_fdset, NULL, &tout);

#if PHY_SYM_DBG_DEBUG_PRINT
            printk("Returned from select %d\n", newsockfd);
#endif
            SYM_DBG_LOCK(sym_dbg_arg);
            if (sym_dbg_arg.flags & SYM_DBG_THREAD_STOP) {
                printk("Closing Socket, user aborted\n");
                close(sockfd);
                sal_thread_exit(-1);
            }
            SYM_DBG_UNLOCK(sym_dbg_arg);
        } while(newsockfd <= 0);

#if PHY_SYM_DBG_DEBUG_PRINT
        printk("Select returned \n");
#endif
        newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&clilen);
        if(newsockfd < 0) {
            printk("Error in socket accept \n");
            close(sockfd);
            sal_thread_exit(-1);
        }
#if PHY_SYM_DBG_DEBUG_PRINT
        printk("Accepted Client connection %d\n", newsockfd);
#endif

#if 0
    bzero(&buffer, PHY_SYM_DBG_MDIO_MSG_LEN);
    n = recv(newsockfd, &buffer, PHY_SYM_DBG_MDIO_MSG_LEN, 0);
    if ((n < 0) || (n < PHY_SYM_DBG_MDIO_MSG_LEN)) {
        printk("Error reading from socket \n");
        rv = -1;
        goto sock_close;
    }
    if (buffer.op != MDIO_HANDSHAKE) {
        printk("Unknown connection, Handshake not received: Closing Socket\n");
        rv = -1;
        goto sock_close;
    }

    /* Sending back the handshake */
    bzero(&mdio_rsp, PHY_SYM_DBG_MDIO_MSG_LEN);
    mdio_rsp.op = MDIO_HANDSHAKE;
    n = send(newsockfd, &mdio_rsp, PHY_SYM_DBG_MDIO_MSG_LEN, 0);
    if(n < 0) {
        printk("Handshake:Error writing to socket\n");
        rv = -1;
        goto sock_close;
    }
    printk("Handshake with client completed\n");
#endif
        while (1) {
            /* Check if the thread needs to be aborted */
            SYM_DBG_LOCK(sym_dbg_arg);
            if (sym_dbg_arg.flags & SYM_DBG_THREAD_STOP) {
                break;
            }
            SYM_DBG_UNLOCK(sym_dbg_arg);
            bzero((char *)&buffer, PHY_SYM_DBG_MDIO_MSG_LEN);
            n = recv(newsockfd, (void *)&buffer, PHY_SYM_DBG_MDIO_MSG_LEN, 0);
#if PHY_SYM_DBG_DEBUG_PRINT
            printk("Size read %d %d\n", n, PHY_SYM_DBG_MDIO_MSG_LEN);
#endif
            if (n > 0) {
            for (i = 0; i < n; i++) {
#if PHY_SYM_DBG_DEBUG_PRINT
                printk("%x ", *(((char *)&buffer)+i));
#endif
            }
            }
            if ((n < 0) || (n < PHY_SYM_DBG_MDIO_MSG_LEN)) {
                printk("MDIO message corrupted or garbage read from socket: Closing socket\n");
                break;
            }
            reg_addr = ntohs(buffer.reg_addr);
            data = ntohs(buffer.data);

            bzero((char *)&mdio_rsp, PHY_SYM_DBG_MDIO_MSG_LEN);
            if (buffer.op == MDIO_WRITE) {
#if PHY_SYM_DBG_DEBUG_PRINT
                printk("MDIO WRITE operation\n");
                printk("%x %x %x %x %x\n", buffer.port_id, buffer.device, reg_addr, buffer.c45, data);
#endif
                if (buffer.c45) {
                    rv = soc_miimc45_write(u, buffer.port_id, buffer.device,
                                           reg_addr, data);
                    /* Read the register */ 
                    if (rv >= 0) {
                        rv = soc_miimc45_read(u, buffer.port_id, buffer.device, 
                                           reg_addr, &phy_data);
                    }
                } else {
                    rv = soc_miim_write(u, buffer.port_id, reg_addr, data);
                    /* Read the register */ 
                    if (rv >= 0) {
                        rv = soc_miim_read(u, buffer.port_id, reg_addr, &phy_data);
#if PHY_SYM_DBG_DEBUG_PRINT
                        printk("read data = %x\n", phy_data);
#endif
                    }
                }
                if (rv < 0) {
                    printk("ERROR: MII Addr %d: soc_miim_write failed: %s\n",
                           buffer.port_id, soc_errmsg(rv));
                    rv = -1;
                    break;
                }
                mdio_rsp.data = htons(phy_data);
                mdio_rsp.op = MDIO_RESP;
            } else {
                if (buffer.op == MDIO_READ) {
#if PHY_SYM_DBG_DEBUG_PRINT
                    printk("MDIO READ operation\n");
                    printk("%x %x %x %x %x\n", buffer.port_id, buffer.device, reg_addr, buffer.c45, data);
#endif

                    if (buffer.c45) {
                        rv = soc_miimc45_read(u, buffer.port_id, buffer.device, 
                                              reg_addr, &phy_data);
                    } else {
                        rv = soc_miim_read(u, buffer.port_id, reg_addr, &phy_data);
                    }
                    if (rv < 0) {
                        printk("ERROR: MII Addr %d: soc_miim_read failed: %s\n",
                               buffer.port_id, soc_errmsg(rv));
                        rv = -1;
                        break;
                    }
                    mdio_rsp.data = htons(phy_data);
                    mdio_rsp.op = MDIO_RESP;
#if PHY_SYM_DBG_DEBUG_PRINT
                    printk("READ data = %x %x\n", mdio_rsp.data, phy_data);
#endif
                } else {
                    if (buffer.op == MDIO_CLOSE) {
                        printk("Closing connection\n");
                        break;
                    } else {
                        printk("Unknown MDIO operation\n");
                        mdio_rsp.op = MDIO_ERROR;
                    }
                }
            }
#if PHY_SYM_DBG_DEBUG_PRINT
            printk("sending response:\n");
            for (i = 0; i < n; i++) {
                printk("%x ", *(((char *)&mdio_rsp)+i));
            }
#endif
            n = send(newsockfd, (void *)&mdio_rsp, PHY_SYM_DBG_MDIO_MSG_LEN, 0);
            if(n < 0) {
#if PHY_SYM_DBG_DEBUG_PRINT
                printk("Error writing to socket\n");
#endif
                rv = -1;
                break;
            }
        }
    }
/*sock_close:*/
    close(newsockfd);
    close(sockfd);
    sal_thread_exit(rv);
}

#endif

#define _CHECK_PRINT(flags, mask, str, val) \
    if ((flags) & (mask)) printk(str, val); \
    else printk(str, "")

int
brief_port_info(char *port_ref, bcm_port_info_t *info, uint32 flags)
{
    char        *spt_str, *discrd_str;
    char        sbuf[6];
    int         lrn_ptr;
    char        lrn_str[4];

    spt_str = FORWARD_MODE(info->stp_state);
    discrd_str = DISCARD_MODE(info->discard);

    /* port number (7)
     * enable/link state (4)
     * speed/duplex (6)
     * link scan mode (4)
     * auto negotiate? (4)
     * spantree state (7)
     * pause tx/rx (5)
     * discard mode (6)
     * learn to CPU, ARL, FWD or discard (3)
     */
    printk("%10s  %4s ", port_ref,
	   !info->enable ? "!ena" :
           (info->linkstatus== BCM_PORT_LINK_STATUS_FAILED) ? "fail" :
           (info->linkstatus== BCM_PORT_LINK_STATUS_UP ? "up  " : "down"));
    _CHECK_PRINT(flags, BCM_PORT_ATTR_SPEED_MASK,
                 "%4s ", _if_fmt_speed(sbuf, info->speed));
    _CHECK_PRINT(flags, BCM_PORT_ATTR_DUPLEX_MASK,
                 "%2s ", info->speed == 0 ? "" : info->duplex ? "FD" : "HD");
    _CHECK_PRINT(flags, BCM_PORT_ATTR_LINKSCAN_MASK,
                 "%4s ", LINKSCAN_MODE(info->linkscan));
    _CHECK_PRINT(flags, BCM_PORT_ATTR_AUTONEG_MASK,
                 "%4s ", info->autoneg ? " Yes" : " No ");
    _CHECK_PRINT(flags, BCM_PORT_ATTR_STP_STATE_MASK,
                 " %7s  ", spt_str);
    _CHECK_PRINT(flags, BCM_PORT_ATTR_PAUSE_TX_MASK,
                 "%2s ", info->pause_tx ? "TX" : "");
    _CHECK_PRINT(flags, BCM_PORT_ATTR_PAUSE_RX_MASK,
                 "%2s ", info->pause_rx ? "RX" : "");
    _CHECK_PRINT(flags, BCM_PORT_ATTR_DISCARD_MASK,
                 "%6s  ", discrd_str);

    lrn_ptr = 0;
    sal_memset(lrn_str, 0, sizeof(lrn_str));
    lrn_str[0] = 'D';
    if (info->learn & BCM_PORT_LEARN_FWD) {
        lrn_str[lrn_ptr++] = 'F';
    }
    if (info->learn & BCM_PORT_LEARN_ARL) {
        lrn_str[lrn_ptr++] = 'A';
    }
    if (info->learn & BCM_PORT_LEARN_CPU) {
        lrn_str[lrn_ptr++] = 'C';
    }
    _CHECK_PRINT(flags, BCM_PORT_ATTR_LEARN_MASK,
                 "%3s ", lrn_str);
    _CHECK_PRINT(flags, BCM_PORT_ATTR_INTERFACE_MASK,
		 "%6s ", INTERFACE_MODE(info->interface));
    if (flags & BCM_PORT_ATTR_FRAME_MAX_MASK) {
	printk("%5d ", info->frame_max);
    } else {
	printk("%5s ", "");
    }
    _CHECK_PRINT(flags, BCM_PORT_ATTR_LOOPBACK_MASK,
		 "%s",
		 info->loopback != BCM_PORT_LOOPBACK_NONE ?
		 LOOPBACK_MODE(info->loopback) : "");

    printk("\n");
    return 0;
}

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *	if_port_policer
 * Purpose:
 *	Table display of port information
 * Parameters:
 *	unit - SOC unit #
 *	a - pointer to args
 * Returns:
 *	CMD_OK/CMD_FAIL
 */
cmd_result_t
if_esw_port_policer(int unit, args_t *a)
{
    pbmp_t              pbm;
    char *subcmd, *argpbm, *argpid;
    bcm_policer_t pid = 0;
    bcm_port_config_t   pcfg;
    soc_port_t          port, dport;
    int rv;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    if (bcm_port_config_get(unit, &pcfg) != BCM_E_NONE) {
        printk("%s: Error: bcm ports not initialized\n", ARG_CMD(a));
        return CMD_FAIL;
    }

    if ((subcmd = ARG_GET(a)) == NULL) {
	subcmd = "Get";
    }

    if ((argpbm = ARG_GET(a)) == NULL) {
        pbm = pcfg.port;
    } else {
        if (parse_bcm_pbmp(unit, argpbm, &pbm) < 0) {
            printk("%s: ERROR: unrecognized port bitmap: %s\n",
                    ARG_CMD(a), argpbm);
            return CMD_FAIL;
        }

        BCM_PBMP_AND(pbm, pcfg.port);
    }

    if (sal_strcasecmp(subcmd, "Get") == 0) {
	rv = BCM_E_NONE;
        DPORT_BCM_PBMP_ITER(unit, pbm, dport, port) {
	    if ((rv = bcm_port_policer_get(unit, port, &pid)) < 0) {
                printk("Error retrieving info for port %s: %s\n",
		       BCM_PORT_NAME(unit, port), bcm_errmsg(rv));
		break;
	    }

	    printk("Port %s policer id is %d\n",
		   BCM_PORT_NAME(unit, port), pid);
        }
        return (rv < 0) ? CMD_FAIL : CMD_OK;
    } else if (sal_strcasecmp(subcmd, "Set") == 0) {
        if ((argpid = ARG_GET(a)) == NULL) {
            printk("Missing PID for set.\n");
            return CMD_USAGE;
        }
        pid = sal_ctoi(argpid, 0);
    } else {
        return CMD_USAGE;
    }

    /* Set Policer id as indicated */

    rv = BCM_E_NONE;

    DPORT_BCM_PBMP_ITER(unit, pbm, dport, port) {
	if ((rv = bcm_port_policer_set(unit, port, pid)) < 0) {
	    printk("Error setting port %s default PID to %d: %s\n",
		   BCM_PORT_NAME(unit, port), pid, bcm_errmsg(rv));

	    if ((rv == BCM_E_NOT_FOUND) ||
		(rv == BCM_E_CONFIG)) {
		printk("Error in setting PID %x to port \n", pid);
	    }
	    break;
	}
    }
    return CMD_OK;
}
#endif /* BCM_KATANA_SUPPORT or BCM_TRIUMPH3_SUPPORT */
/*
 * Function:
 *	if_port_stat
 * Purpose:
 *	Table display of port information
 * Parameters:
 *	unit - SOC unit #
 *	a - pointer to args
 * Returns:
 *	CMD_OK/CMD_FAIL
 */
cmd_result_t
if_esw_port_stat(int unit, args_t *a)
{
    pbmp_t              pbm, tmp_pbm;
    bcm_port_info_t    *info_all;
    bcm_port_config_t   pcfg;
    soc_port_t          p, dport;
    int                 r;
    char               *c;
    uint32              mask;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    if (bcm_port_config_get(unit, &pcfg) != BCM_E_NONE) {
        printk("%s: Error: bcm ports not initialized\n", ARG_CMD(a));
        return CMD_FAIL;
    }

    if ((c = ARG_GET(a)) == NULL) {
        BCM_PBMP_ASSIGN(pbm, pcfg.port);
    } else if (parse_bcm_pbmp(unit, c, &pbm) < 0) {
        printk("%s: Error: unrecognized port bitmap: %s\n",
               ARG_CMD(a), c);
        return CMD_FAIL;
    }
    BCM_PBMP_AND(pbm, pcfg.port);
    if (BCM_PBMP_IS_NULL(pbm)) {
        printk("No ports specified.\n");
        return CMD_OK;
    }
    
    mask = BCM_PORT_ATTR_ALL_MASK;

    info_all = sal_alloc(SOC_MAX_NUM_PORTS * sizeof(bcm_port_info_t), 
                         "if_port_stat");
    if (info_all == NULL) {
        printk("Insufficient memory.\n");
        return CMD_FAIL;
    }

    DPORT_BCM_PBMP_ITER(unit, pbm, dport, p) {
        port_info_init(unit, p, &info_all[p], mask);
        if ((r = bcm_port_selective_get(unit, p, &info_all[p])) < 0) {
            printk("%s: Error: Could not get port %s information: %s\n",
                   ARG_CMD(a), BCM_PORT_NAME(unit, p), bcm_errmsg(r));
            sal_free(info_all);
            return (CMD_FAIL);
        }
    }

    brief_port_info_header(unit);

#define _call_bpi(pbm, pbm_mask) \
    tmp_pbm = pbm_mask; \
    BCM_PBMP_AND(tmp_pbm, pbm); \
    DPORT_BCM_PBMP_ITER(unit, tmp_pbm, dport, p) { \
        brief_port_info(BCM_PORT_NAME(unit, p), &info_all[p], mask); \
    }

    if (soc_property_get(unit, spn_DPORT_MAP_ENABLE, TRUE)) {
        /* If port mapping is enabled, then use port order */
        _call_bpi(pbm, pcfg.port);
    } else {
        /* If no port mapping, ensure that ports are grouped by type */
        _call_bpi(pbm, pcfg.fe);
        _call_bpi(pbm, pcfg.ge);
        _call_bpi(pbm, pcfg.xe);
        _call_bpi(pbm, pcfg.ce);
        _call_bpi(pbm, pcfg.hg);
    }

    sal_free(info_all);

    return CMD_OK;
}

/*
 * Function:
 *	if_port_rate
 * Purpose:
 *	Set/display of port rate metering characteristics
 * Parameters:
 *	unit - SOC unit #
 *	a - pointer to args
 * Returns:
 *	CMD_OK/CMD_FAIL
 */
cmd_result_t
if_esw_port_rate(int unit, args_t *a)
{
    pbmp_t          pbm;
    bcm_port_config_t pcfg;
    soc_port_t      p, dport;
    int             operation = 0;
    int             rv;
    int             header;
    uint32          rate = 0xFFFFFFFF;
    uint32          burst = 0xFFFFFFFF;
    char           *c;

#define SHOW    1
#define INGRESS 2
#define EGRESS  4
#define PAUSE   8

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    /* Check for metering capabilities */
    if (! soc_feature(unit, soc_feature_ingress_metering) &&
	! soc_feature(unit, soc_feature_egress_metering) ) {
        printk("%s: Error: metering unavailable for this device\n", 
	       ARG_CMD(a));
        return CMD_OK;
    }

    if (bcm_port_config_get(unit, &pcfg) != BCM_E_NONE) {
        printk("%s: Error: bcm ports not initialized\n", ARG_CMD(a));
        return CMD_FAIL;
    }

    if ((c = ARG_GET(a)) == NULL) {
        BCM_PBMP_ASSIGN(pbm, pcfg.port);
    } else if (parse_bcm_pbmp(unit, c, &pbm) < 0) {
        printk("%s: Error: unrecognized port bitmap: %s\n",
               ARG_CMD(a), c);
        return CMD_FAIL;
    }

    /* Apply PortRate only to those ports which support it */
    BCM_PBMP_AND(pbm, pcfg.e);
    if (BCM_PBMP_IS_NULL(pbm)) {
        printk("No ports specified.\n");
        return CMD_OK;
    }
    
    /* Ingress, egress or show both */
    if ((c = ARG_GET(a)) == NULL) {
        operation = SHOW;
	if (soc_feature(unit, soc_feature_ingress_metering)) {
	    operation |= INGRESS;
	}
	if (soc_feature(unit, soc_feature_egress_metering)) {
	    operation |= EGRESS;
	}
    }
    else if (!sal_strncasecmp(c, "ingress", sal_strlen(c))) {
	if (soc_feature(unit, soc_feature_ingress_metering)) {
	    operation = INGRESS;
	} else {
	    printk("%s: Error: ingress metering unavailable for "
		   "this device\n", ARG_CMD(a));
	    return CMD_OK;
	}
    }
    else if (!sal_strncasecmp(c, "egress", sal_strlen(c))) {
	if (soc_feature(unit, soc_feature_egress_metering)) {
	    operation = EGRESS;
	} else {
	    printk("%s: Error: egress metering unavailable for "
		   "this device\n", ARG_CMD(a));
	    return CMD_OK;
	}
    }
    else if (SOC_IS_TUCANA(unit) && 
            !sal_strncasecmp(c, "pause", sal_strlen(c))) {
        operation = PAUSE;
    }
    else {
        printk("%s: Error: unrecognized port rate type: %s\n",
               ARG_CMD(a), c);
        return CMD_FAIL;
    }

    /* Set or get */
    if ((c = ARG_GET(a)) != NULL) {
        rate = parse_integer(c);
        if ((c = ARG_GET(a)) != NULL) {
            burst = parse_integer(c);
        }
        else {
            printk("%s: Error: missing port burst size\n",
                   ARG_CMD(a));
            return CMD_FAIL;
        }
    }
    else {
        operation |= SHOW;
    }

    DPORT_BCM_PBMP_ITER(unit, pbm, dport, p) {
        if (operation & SHOW) {
            /* Display current setting */
            header = 0;
            if (operation & (INGRESS | PAUSE)) {
                rv = bcm_port_rate_ingress_get(unit, p, &rate, &burst); 
                if (rv < 0) {
                    printk("%s port %s: ERROR: bcm_port_rate_ingress_get: "
                           "%s\n",
                           ARG_CMD(a), BCM_PORT_NAME(unit, p), bcm_errmsg(rv));
                    return CMD_FAIL;
                }
                if (rate) {
                    printk("%4s:", BCM_PORT_NAME(unit, p));
                    header = 1;
                    if (rate < 64) { 
                        printk("\tIngress meter: ? kbps ? kbits max burst.\n");
                    }
                    else {
                        printk("\tIngress meter: "
                               "%8d kbps %8d kbits max burst.\n",
                               rate, burst);
                    }
                    if (SOC_IS_TUCANA(unit)) {
                        rv = bcm_port_rate_pause_get(unit, p, &rate, &burst);
                        if (rv < 0) {
                            printk("%s port %s: ERROR: "
                                   "bcm_port_rate_pause_get: %s\n",
                                   ARG_CMD(a), BCM_PORT_NAME(unit, p),
                                   bcm_errmsg(rv));
                            return CMD_FAIL;
                        }
                        if (rate) { 
                            printk("\tPause frames: Pause = %8d kbits, "
                                   "Resume = %8d kbits.\n", rate, burst);
                        }
                    }
                }
            }
            if (operation & EGRESS) { 
                rv = bcm_port_rate_egress_get(unit, p, &rate, &burst);
                if (rv < 0) {
                    printk("%s port %s: ERROR: bcm_port_rate_egress_get: %s\n",
                           ARG_CMD(a), BCM_PORT_NAME(unit, p), bcm_errmsg(rv));
                    return CMD_FAIL;
                }
                if (rate) { 
                    if (!header)
                        printk("%4s:", BCM_PORT_NAME(unit, p));
                    printk("\tEgress meter:  %8d kbps %8d kbits max burst.\n",
                           rate, burst);
                }
            }
        }
        else {
            /* New setting */
            if (!rate || !burst)
                rate = burst = 0; /* Disable port metering */
            if (operation & INGRESS) {
                rv = bcm_port_rate_ingress_set(unit, p, rate, burst); 
                if (rv < 0) {
                    printk("%s: ERROR: bcm_port_rate_ingress_set: %s\n",
                           ARG_CMD(a), bcm_errmsg(rv));
                    return CMD_FAIL;
                }
            }
            else if (operation & PAUSE) {
                rv = bcm_port_rate_pause_set(unit, p, rate, burst); 
                if (rv < 0) {
                    printk("%s: ERROR: bcm_port_rate_pause_set: %s\n",
                           ARG_CMD(a), bcm_errmsg(rv));
                    return CMD_FAIL;
                }
            }
            else if (operation & EGRESS) {
                rv = bcm_port_rate_egress_set(unit, p, rate, burst);
                if (rv < 0) {
                    printk("%s: ERROR: bcm_port_rate_egress_set: %s\n",
                           ARG_CMD(a), bcm_errmsg(rv));
                    return CMD_FAIL;
                }
            }
        }
    }
#undef SHOW 
#undef INGRESS
#undef EGRESS
#undef PAUSE

    return CMD_OK;
}

/*
 * Function:
 *      if_port_samp_rate
 * Purpose:
 *      Set/display of sflow port sampling rates.
 * Parameters:
 *      unit - SOC unit #
 *      args - pointer to comand line arguments
 * Returns:
 *      CMD_OK/CMD_FAIL
 */
char if_port_samp_rate_usage[] =
    "Set/Display port sampling rate characteristics.\n"
    "Parameters: <pbm> [ingress_rate] [egress_rate]\n"
    "\tOn average, every 1/ingress_rate packets will be sampled.\n"
    "\tA rate of 0 indicates no sampling.\n"
    "\tA rate of 1 indicates all packets sampled.\n";

cmd_result_t
if_port_samp_rate(int unit, args_t *args)
{
#define SHOW    0x01
#define SET     0x02
    pbmp_t          pbm;
    bcm_port_config_t pcfg;
    char           *ch;
    int             operation    = SET; /* Set or Show */
    int             ingress_rate = -1;
    int             egress_rate  = -1;
    soc_port_t      soc_port, dport;
    int             retval;

    if (!sh_check_attached(ARG_CMD(args), unit)) {
        return CMD_FAIL;
    }

    if (bcm_port_config_get(unit, &pcfg) != BCM_E_NONE) {
        printk("%s: Error: bcm ports not initialized\n", ARG_CMD(args));
        return CMD_FAIL;
    }

    /* get port bitmap */
    if ((ch = ARG_GET(args)) == NULL) {
        BCM_PBMP_ASSIGN(pbm, pcfg.port);
    } else if (parse_bcm_pbmp(unit, ch, &pbm) < 0) {
        printk("%s: Error: unrecognized port bitmap: %s\n",
               ARG_CMD(args), ch);
        return CMD_FAIL;
    }

    /* read in ingress_rate and egress_rate if given */
    if ((ch = ARG_GET(args)) != NULL) {
        ingress_rate = parse_integer(ch);
        if ((ch = ARG_GET(args)) != NULL) {
            egress_rate = parse_integer(ch);
        }
        else {
            printk("%s: Error: missing egress rate \n", ARG_CMD(args));
            return CMD_FAIL;
        }
    }
    else {
        operation = SHOW;
    }

    /* Iterate through port bitmap and perform 'operation' on them. */
    DPORT_BCM_PBMP_ITER(unit, pbm, dport, soc_port) {
        if (operation == SHOW) {
        /* Show port sflow sample rate(s) */
            retval = bcm_port_sample_rate_get(unit, soc_port, &ingress_rate,
                                              &egress_rate);
            if (retval != BCM_E_NONE) {
                printk("%s port %s: ERROR: bcm_port_sample_rate_get: "
                       "%s\n", ARG_CMD(args),
                       BCM_PORT_NAME(unit, soc_port), bcm_errmsg(retval));
                return CMD_FAIL;
            }

            printk("%4s:", BCM_PORT_NAME(unit, soc_port));
 
            if ( ingress_rate == 0 ) {
                printk("\tingress: not sampling,");
            }
            else {
                printk("\tingress: 1 out of %d packets,", ingress_rate);
            }
            if ( egress_rate == 0 ) {
                printk("\tegress: not sampling,");
            }
            else {
                printk("\tegress: 1 out of %d packets,", egress_rate);
            }
            printk("\n");
        }
        else {
        /* Set port sflow sample rate(s) */
            retval = bcm_port_sample_rate_set(unit, soc_port, ingress_rate, egress_rate);
            if (retval != BCM_E_NONE) {
                printk("%s port %s: ERROR: bcm_port_sample_rate_set: "
                       "%s\n", ARG_CMD(args),
                       BCM_PORT_NAME(unit, soc_port), bcm_errmsg(retval));
                return CMD_FAIL;
            }
        }
    }

#undef SHOW
#undef SET
    return CMD_OK;
}
 
/*
 * Function:
 *	disp_port_info
 * Purpose:
 *	Display selected port information
 * Parameters:
 *	info	    - pointer to structure with info to display
 *	port_ref    - Port reference to print
 *	st_port     - Is the port a hi-gig port?
 * Returns:     
 *	Nothing
 * Notes:
 *	Assumes link status info always valid
 */

void
disp_port_info(char *port_ref, bcm_port_info_t *info, 
               int st_port, uint32 flags)
{
    int 		r;
    int 		no_an_props = 0;   /* Do not show AN props */
    bcm_port_ability_t *local = &info->port_ability;
    bcm_port_ability_t *remote = &info->remote_ability;
    bcm_port_ability_t *advert = &info->local_ability;

    /* Assume link status always available. */
    printk(" %c%-7s ", info->linkstatus ? '*' : ' ', port_ref);

    if (info->linkstatus == BCM_PORT_LINK_STATUS_FAILED) {
        printk("%s", "FAILED ");
    }

    if (flags & BCM_PORT_ATTR_ENABLE_MASK) {
        printk("%s", info->enable ? "" : "DISABLED ");
    }

    if (flags & BCM_PORT_ATTR_LINKSCAN_MASK) {
        if (info->linkscan) {
            printk("LS(%s) ", LINKSCAN_MODE(info->linkscan));
        }
    }
    if (st_port) {
        printk("%s(", ENCAP_MODE(info->encap_mode & 0x3));
    } else if (flags & BCM_PORT_ATTR_AUTONEG_MASK) {
        if (info->autoneg) {
            if (!info->linkstatus) {
                printk("Auto(no link) ");
                no_an_props = 1;
            } else {
                printk("Auto(");
            }
        } else {
            printk("Forced(");
        }
    } else {
        printk("AN?(");
    }

    /* If AN is enabled, but not complete, don't show port settings */
    if (!no_an_props) {
        if (flags & BCM_PORT_ATTR_SPEED_MASK) {
	    char	buf[6];
            printk("%s", _if_fmt_speed(buf, info->speed));
        }
        if (flags & BCM_PORT_ATTR_DUPLEX_MASK) {
            printk("%s", info->speed == 0 ? "" : info->duplex ? "FD" : "HD");
        }
        if (flags & BCM_PORT_ATTR_PAUSE_MASK) {
    	    if (info->pause_tx && info->pause_rx) {
                printk(",pause");
    	    } else if (info->pause_tx) {
                printk(",pause_tx");
    	    } else if (info->pause_rx) {
                printk(",pause_rx");
    	    }
        }
        printk(") ");
    }

    if (flags & BCM_PORT_ATTR_AUTONEG_MASK) {
        if (info->autoneg) {
	    char	buf[80];

            if (flags & BCM_PORT_ATTR_ABILITY_MASK) {
                format_port_speed_ability(buf, sizeof (buf), local->speed_full_duplex);
                printk("Ability (fd = %s ", buf);
                format_port_speed_ability(buf, sizeof (buf), local->speed_half_duplex);
                printk("hd = %s ", buf);
                format_port_intf_ability(buf, sizeof (buf), local->interface);
                printk("intf = %s ", buf);
                format_port_medium_ability(buf, sizeof (buf), local->medium);
                printk("medium = %s ", buf);
                format_port_pause_ability(buf, sizeof (buf), local->pause);
                printk("pause = %s ", buf);
                format_port_lb_ability(buf, sizeof (buf), local->loopback);
                printk("lb = %s ", buf);
                format_port_flags_ability(buf, sizeof (buf), local->flags);
                printk("flags = %s )", buf);
            }

            if (flags & BCM_PORT_ATTR_LOCAL_ADVERT_MASK) {
                format_port_speed_ability(buf, sizeof (buf), advert->speed_full_duplex);
                printk("Local (fd = %s ", buf);
                format_port_speed_ability(buf, sizeof (buf), advert->speed_half_duplex);
                printk("hd = %s ", buf);
                format_port_intf_ability(buf, sizeof (buf), advert->interface);
                printk("intf = %s", buf);
                format_port_medium_ability(buf, sizeof (buf), advert->medium);
                printk("medium = %s ", buf);
                format_port_pause_ability(buf, sizeof (buf), advert->pause);
                printk("pause = %s ", buf);
                format_port_lb_ability(buf, sizeof (buf), advert->loopback);
                printk("lb = %s ", buf);
                format_port_flags_ability(buf, sizeof (buf), advert->flags);
                printk("flags = %s )", buf);
            }

            if ((flags & BCM_PORT_ATTR_REMOTE_ADVERT_MASK) &&
                info->remote_advert_valid && info->linkstatus) {
                format_port_speed_ability(buf, sizeof (buf), remote->speed_full_duplex);
                printk("Remote (fd = %s ", buf);
                format_port_speed_ability(buf, sizeof (buf), remote->speed_half_duplex);
                printk("hd = %s ", buf);
                format_port_intf_ability(buf, sizeof (buf), remote->interface);
                printk("intf = %s ", buf);
                format_port_medium_ability(buf, sizeof (buf), remote->medium);
                printk("medium = %s ", buf);
                format_port_pause_ability(buf, sizeof (buf), remote->pause);
                printk("pause = %s ", buf);
                format_port_lb_ability(buf, sizeof (buf), remote->loopback);
                printk("lb = %s ", buf);
                format_port_flags_ability(buf, sizeof (buf), remote->flags);
                printk("flags = %s )", buf);
            }
        }
    }

    if (flags & BCM_PORT_ATTR_PAUSE_MAC_MASK) {
	if ((info->pause_mac[0] | info->pause_mac[1] |
	     info->pause_mac[2] | info->pause_mac[3] |
	     info->pause_mac[4] | info->pause_mac[5]) != 0) {
	    printk("Stad(%02x:%02x:%02x:%02x:%02x:%02x) ",
		   info->pause_mac[0], info->pause_mac[1],
		   info->pause_mac[2], info->pause_mac[3],
		   info->pause_mac[4], info->pause_mac[5]);
	}
    }

    if (flags & BCM_PORT_ATTR_STP_STATE_MASK) {
        printk("STP(%s) ", FORWARD_MODE(info->stp_state));
    }

    if (!st_port) {
        if (flags & BCM_PORT_ATTR_DISCARD_MASK) {
            switch (info->discard) {
            case BCM_PORT_DISCARD_NONE:
                break;
            case BCM_PORT_DISCARD_ALL:
                printk("Disc(all) ");
                break;
            case BCM_PORT_DISCARD_UNTAG:
                printk("Disc(untagged) ");
                break;
            case BCM_PORT_DISCARD_TAG:
                printk("Disc(tagged) ");
                break;
            default:
                printk("Disc(?) ");
                break;
            }
        }

        if (flags & BCM_PORT_ATTR_LEARN_MASK) {
            printk("Lrn(");

            r = 0;

            if (info->learn & BCM_PORT_LEARN_ARL) {
                printk("ARL");
                r = 1;
            }

            if (info->learn & BCM_PORT_LEARN_CPU) {
                printk("%sCPU", r ? "," : "");
                r = 1;
            }

            if (info->learn & BCM_PORT_LEARN_FWD) {
                printk("%sFWD", r ? "," : "");
                r = 1;
            }

            if (!r) {
                printk("disc");
            }
            printk(") ");
        }

        if (flags & BCM_PORT_ATTR_UNTAG_PRI_MASK) {
            printk("UtPri(");

            if (info->untagged_priority < 0) {
                printk("off");
            } else {
                printk("%d", info->untagged_priority);
            }
            printk(") ");
        }

        if (flags & BCM_PORT_ATTR_PFM_MASK) {
            printk("Pfm(%s) ",  MCAST_FLOOD(info->pfm));
        }
    } /* !st_port */
            
    if (flags & BCM_PORT_ATTR_INTERFACE_MASK) {
        if (info->interface >= 0 && info->interface < SOC_PORT_IF_COUNT) {
            printk("IF(%s) ", INTERFACE_MODE(info->interface));
        }
    }

    if (flags & BCM_PORT_ATTR_PHY_MASTER_MASK) {
        if (info->phy_master >= 0 &&
	    info->phy_master < SOC_PORT_MS_COUNT &&
	    info->phy_master != SOC_PORT_MS_NONE) {
            printk("PH(%s) ", PHYMASTER_MODE(info->phy_master));
        }
    }

    if (flags & BCM_PORT_ATTR_LOOPBACK_MASK) {
        if (info->loopback == BCM_PORT_LOOPBACK_PHY) {
            printk("LB(PHY) ");
        } else if (info->loopback == BCM_PORT_LOOPBACK_MAC) {
            printk("LB(MAC) ");
        }
    }

    if (flags & BCM_PORT_ATTR_FRAME_MAX_MASK) {
        printk("Max_frame(%d) ", info->frame_max);
    }

    if ((flags & BCM_PORT_ATTR_MDIX_MASK) &&
        (0 <= info->mdix) && 
        (info->mdix < BCM_PORT_MDIX_COUNT)) {
        printk("MDIX(%s", MDIX_MODE(info->mdix));

        if ((flags & BCM_PORT_ATTR_MDIX_STATUS_MASK) &&
            (0 <= info->mdix_status) && 
            (info->mdix_status < BCM_PORT_MDIX_STATUS_COUNT)) {
            printk(", %s", MDIX_STATUS(info->mdix_status));
        }

        printk(") ");
    }

    if ((flags & BCM_PORT_ATTR_MEDIUM_MASK) &&
        (0 <= info->medium) && (info->medium < BCM_PORT_MEDIUM_COUNT)) {
        printk("Medium(%s) ", MEDIUM_STATUS(info->medium));
    }

    if ((flags & BCM_PORT_ATTR_FAULT_MASK) && (info->fault)) { 
        printk("Fault(%s%s) ", (info->fault & BCM_PORT_FAULT_LOCAL) ? "Local" : "", 
                               (info->fault & BCM_PORT_FAULT_REMOTE) ? "Remote" : "");
    }

    if ((flags & BCM_PORT_ATTR_VLANFILTER_MASK) && (info->vlanfilter > 0)) {
        printk("VLANFILTER(%d) ", info->vlanfilter);
    }
    
    printk("\n");
}

/* This maps the above list to the masks for the proper attributes
 * Note that the order of this attribute map should match that of
 * the parse-table entry/creation below.
 */
static int port_attr_map[] = {
    BCM_PORT_ATTR_ENABLE_MASK,       /* Enable */
    BCM_PORT_ATTR_AUTONEG_MASK,      /* AutoNeg */
    BCM_PORT_ATTR_LOCAL_ADVERT_MASK, /* ADVert */
    BCM_PORT_ATTR_SPEED_MASK,        /* SPeed */
    BCM_PORT_ATTR_DUPLEX_MASK,       /* FullDuplex */
    BCM_PORT_ATTR_LINKSCAN_MASK,     /* LinkScan */
    BCM_PORT_ATTR_LEARN_MASK,        /* LeaRN */
    BCM_PORT_ATTR_DISCARD_MASK,      /* DISCard */
    BCM_PORT_ATTR_VLANFILTER_MASK,   /* VlanFilter */
    BCM_PORT_ATTR_UNTAG_PRI_MASK,    /* PRIOrity */
    BCM_PORT_ATTR_PFM_MASK,          /* PortFilterMode */
    BCM_PORT_ATTR_PHY_MASTER_MASK,   /* PHymaster */
    BCM_PORT_ATTR_INTERFACE_MASK,    /* InterFace */
    BCM_PORT_ATTR_LOOPBACK_MASK,     /* LoopBack */
    BCM_PORT_ATTR_STP_STATE_MASK,    /* SpanningTreeProtocol */
    BCM_PORT_ATTR_PAUSE_MAC_MASK,    /* STationADdress */
    BCM_PORT_ATTR_PAUSE_TX_MASK,     /* TxPAUse */
    BCM_PORT_ATTR_PAUSE_RX_MASK,     /* RxPAUse */
    BCM_PORT_ATTR_ENCAP_MASK,        /* Port encapsulation mode */
    BCM_PORT_ATTR_FRAME_MAX_MASK,    /* Max receive frame size */
    BCM_PORT_ATTR_MDIX_MASK,         /* MDIX mode */
    BCM_PORT_ATTR_MEDIUM_MASK,       /* port MEDIUM */
};

/*
 * Function:
 *	port_parse_setup
 * Purpose:
 *	Setup the parse table for a port command
 * Parameters:
 *	pt	- the table
 *	info	- port info structure to hold parse results
 * Returns:
 *	Nothing
 */

void
port_parse_setup(int unit, parse_table_t *pt, bcm_port_info_t *info)
{
    int i;

    /*
     * NOTE: ENTRIES IN THIS TABLE ARE POSITION-DEPENDENT!
     * See references to PQ_PARSED below.
     */
    parse_table_init(unit, pt);
    parse_table_add(pt, "Enable", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->enable, 0);
    parse_table_add(pt, "AutoNeg", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->autoneg, 0);
    if (info->action_mask2 & BCM_PORT_ATTR2_PORT_ABILITY) {
        parse_table_add(pt, "ADVert", PQ_PORTABIL | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->local_ability, 0);
    } else {
        parse_table_add(pt, "ADVert", PQ_PORTMODE | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->local_advert, 0);
    }
    parse_table_add(pt, "SPeed",       PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->speed, 0);
    parse_table_add(pt, "FullDuplex",  PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->duplex, 0);
    parse_table_add(pt, "LinkScan",    PQ_MULTI | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->linkscan, linkscan_mode);
    parse_table_add(pt, "LeaRN",   PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->learn, 0);
    parse_table_add(pt, "DISCard", PQ_MULTI | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->discard, discard_mode);
    parse_table_add(pt, "VlanFilter", PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->vlanfilter, 0);
    parse_table_add(pt, "PRIOrity", PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->untagged_priority, 0);
    parse_table_add(pt, "PortFilterMode", PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->pfm, 0);
    parse_table_add(pt, "PHymaster", PQ_MULTI | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->phy_master, phymaster_mode);
    parse_table_add(pt, "InterFace", PQ_MULTI | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->interface, interface_mode);
    parse_table_add(pt, "LoopBack", PQ_MULTI | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->loopback, loopback_mode);
    parse_table_add(pt, "SpanningTreeProtocol",
                    PQ_MULTI | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->stp_state, forward_mode);
    parse_table_add(pt, "STationADdress", PQ_MAC | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->pause_mac, 0);
    parse_table_add(pt, "TxPAUse",     PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->pause_tx, 0);
    parse_table_add(pt, "RxPAUse",     PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                    0, &info->pause_rx, 0);
    parse_table_add(pt, "ENCapsulation", PQ_MULTI | PQ_DFL,
                    0, &info->encap_mode, encap_mode);
    parse_table_add(pt, "FrameMax", PQ_INT | PQ_DFL,
                    0, &info->frame_max, 0);
    parse_table_add(pt, "MDIX", PQ_MULTI | PQ_DFL,
                    0, &info->mdix, mdix_mode);
    parse_table_add(pt, "Medium", PQ_MULTI | PQ_DFL,
                    0, &info->medium, medium_status);
    
    if (SOC_IS_ARAD(unit)) {
#ifdef BCM_ARAD_SUPPORT
        for (i = 0; i < pt->pt_cnt; i++) {
            if (~_BCM_DPP_PORT_ATTRS & port_attr_map[i]) {
                pt->pt_entries[i].pq_type |= PQ_IGNORE;
            }
        }
#endif
    } else if (SOC_IS_DFE(unit)) {
#ifdef BCM_DFE_SUPPORT
        for (i = 0; i < pt->pt_cnt; i++) {
            if (~_BCM_DFE_PORT_ATTRS & port_attr_map[i]) {
                pt->pt_entries[i].pq_type |= PQ_IGNORE;
            }
        }
#endif
    } else if (SOC_IS_XGS12_FABRIC(unit)) {
        /* For Hercules, ignore some StrataSwitch attributes */
        for (i = 0; i < pt->pt_cnt; i++) {
            if (~BCM_PORT_HERC_ATTRS & port_attr_map[i]) {
                pt->pt_entries[i].pq_type |= PQ_IGNORE;
            }
        }
    } else if (!SOC_IS_XGS(unit) && !SOC_IS_SIRIUS(unit)) {
	/* For all non-XGS chips, ignore special XGS attributes */
        for (i = 0; i < pt->pt_cnt; i++) {
            if (BCM_PORT_XGS_ATTRS & port_attr_map[i]) {
                pt->pt_entries[i].pq_type |= PQ_IGNORE;
            }
        }
    }
}


/*
 * Function:
 *	port_parse_mask_get
 * Purpose:
 *	Given PT has been parsed, set seen and parsed flags
 * Parameters:
 *	pt	- the table
 *	seen	- which parameters occurred w/o =
 *	parsed	- which parameters occurred w =
 * Returns:
 *	Nothing
 */

void
port_parse_mask_get(parse_table_t *pt, uint32 *seen, uint32 *parsed)
{
    uint32		were_parsed = 0;
    uint32		were_seen = 0;
    int			i;

    /* Check that either all parameters are parsed or are seen (no =) */

    for (i = 0; i < pt->pt_cnt; i++) {
        if (pt->pt_entries[i].pq_type & PQ_SEEN) {
            were_seen |= port_attr_map[i];
        }

        if (pt->pt_entries[i].pq_type & PQ_PARSED) {
            were_parsed |= port_attr_map[i];
        }
    }

    *seen = were_seen;
    *parsed = were_parsed;
}

/* Invalid unit number ( < 0) is permitted */
void
port_info_init(int unit, int port, bcm_port_info_t *info, uint32 actions)
{
    bcm_port_info_t_init(info);

    info->action_mask = actions;

    /* We generally need to get link state */
    info->action_mask |= BCM_PORT_ATTR_LINKSTAT_MASK;

    /* Add the autoneg and advert masks if any of actions is possibly 
     * related to the autoneg. 
     */ 
    if (actions & (BCM_PORT_AN_ATTRS | 
                   BCM_PORT_ATTR_AUTONEG_MASK |
                   BCM_PORT_ATTR_LOCAL_ADVERT_MASK)) {     
        info->action_mask |= BCM_PORT_ATTR_LOCAL_ADVERT_MASK;
        info->action_mask |= BCM_PORT_ATTR_REMOTE_ADVERT_MASK;
        info->action_mask |= BCM_PORT_ATTR_AUTONEG_MASK;
    } 

    if (unit >= 0) {
        
        if (unit >= 0 && SOC_IS_XGS12_FABRIC(unit)) {
            info->action_mask |= BCM_PORT_ATTR_ENCAP_MASK;
        }

        /* Clear rate for HG/HL ports */
        if (IS_ST_PORT(unit, port)) {
            info->action_mask &= ~(BCM_PORT_ATTR_RATE_MCAST_MASK |
                                   BCM_PORT_ATTR_RATE_BCAST_MASK |
                                   BCM_PORT_ATTR_RATE_DLFBC_MASK);
        }

        if (SOC_IS_SC_CQ(unit) || SOC_IS_TRIUMPH2(unit) || 
            SOC_IS_APOLLO(unit) || SOC_IS_VALKYRIE2(unit) ||
            SOC_IS_TD_TT(unit) || SOC_IS_HAWKEYE(unit) ||
            SOC_IS_TRIUMPH3(unit)) {
            info->action_mask2 |= BCM_PORT_ATTR2_PORT_ABILITY;
        }
    }
}

/* Given a maximum speed, return the mask of bcm_port_ability_t speeds
 * while are less than or equal to the given speed. */
bcm_port_abil_t
port_speed_max_mask(bcm_port_abil_t max_speed)
{
    bcm_port_abil_t speed_mask = 0;
    /* This is a giant fall through switch */
    switch (max_speed) {
        
    case 42000:
        speed_mask |= BCM_PORT_ABILITY_42GB;
    case 40000:
        speed_mask |= BCM_PORT_ABILITY_40GB;
    case 30000:
        speed_mask |= BCM_PORT_ABILITY_30GB;
    case 25000:
        speed_mask |= BCM_PORT_ABILITY_25GB;
    case 24000:
        speed_mask |= BCM_PORT_ABILITY_24GB;
    case 21000:
        speed_mask |= BCM_PORT_ABILITY_21GB;
    case 20000:
        speed_mask |= BCM_PORT_ABILITY_20GB;
    case 16000:
        speed_mask |= BCM_PORT_ABILITY_16GB;
    case 15000:
        speed_mask |= BCM_PORT_ABILITY_15GB;
    case 13000:
        speed_mask |= BCM_PORT_ABILITY_13GB;
    case 12500:
        speed_mask |= BCM_PORT_ABILITY_12P5GB;
    case 12000:
        speed_mask |= BCM_PORT_ABILITY_12GB;
    case 10000:
        speed_mask |= BCM_PORT_ABILITY_10GB;
    case 6000:
        speed_mask |= BCM_PORT_ABILITY_6000MB;
    case 5000:
        speed_mask |= BCM_PORT_ABILITY_5000MB;
    case 3000:
        speed_mask |= BCM_PORT_ABILITY_3000MB;
    case 2500:
        speed_mask |= BCM_PORT_ABILITY_2500MB;
    case 1000:
        speed_mask |= BCM_PORT_ABILITY_1000MB;
    case 100:
        speed_mask |= BCM_PORT_ABILITY_100MB;
    case 10:
        speed_mask |= BCM_PORT_ABILITY_10MB;
    default:
        break;
    }
    return speed_mask;
}

/*
 * Function:
 *	port_parse_port_info_set
 * Purpose:
 *	Set/change values in a destination according to parsing
 * Parameters:
 *	flags	- What values to change
 *	src	- Where to get info from
 *	dest	- Where to put info
 * Returns:
 *	-1 on error.  0 on success
 * Notes:
 *	The speed_max and abilities values must be
 *      set in the src port info structure before this is called.
 *
 *      Assumes linkstat and autoneg are valid in the dest structure
 *      If autoneg is specified in flags, assumes local advert
 *      is valid in the dest structure.
 */

int
port_parse_port_info_set(uint32 flags,
                         bcm_port_info_t *src,
                         bcm_port_info_t *dest)
{
    int info_speed_adj;

    if (flags & BCM_PORT_ATTR_AUTONEG_MASK) {
        dest->autoneg = src->autoneg;
    }

    if (flags & BCM_PORT_ATTR_ENABLE_MASK) {
        dest->enable = src->enable;
    }

    if (flags & BCM_PORT_ATTR_STP_STATE_MASK) {
        dest->stp_state = src->stp_state;
    }

    /*
     * info_speed_adj is the same as src->speed except a speed of 0
     * is replaced by the maximum speed supported by the port.
     */

    info_speed_adj = src->speed;

    if ((flags & BCM_PORT_ATTR_SPEED_MASK) && (info_speed_adj == 0)) {
        info_speed_adj = src->speed_max;
    }

    /*
     * If local_advert was parsed, use it.  Otherwise, calculate a
     * reasonable local advertisement from the given values and current
     * values of speed/duplex.
     */

    if ((flags & BCM_PORT_ATTR_LOCAL_ADVERT_MASK) != 0) {
        if (dest->action_mask2 & BCM_PORT_ATTR2_PORT_ABILITY) {
            /* Copy source advert info to destination, converting the 
             * format in the process */
            BCM_IF_ERROR_RETURN
                (soc_port_mode_to_ability(src->local_advert,
                                          &(dest->local_ability)));

            /* in case the PQ_PORTABIL is used for parsing */
            if (!src->local_advert) {
                dest->local_ability = src->local_ability;
            }
        } else {
            dest->local_advert = src->local_advert;
        }
    } else if (dest->autoneg) {
        int                 cur_speed, cur_duplex;
        int                 cur_pause_tx, cur_pause_rx;
        int                 new_speed, new_duplex;
        int                 new_pause_tx, new_pause_rx;
        bcm_port_abil_t     mode;

        /*
         * Update link advertisements for speed/duplex/pause.  All
         * speeds less than or equal to the requested speed are
         * advertised.
         */

        if (dest->action_mask2 & BCM_PORT_ATTR2_PORT_ABILITY) {
            bcm_port_ability_t  *dab, *sab;

            dab = &(dest->local_ability);
            sab = &(src->port_ability);

            cur_speed =
                BCM_PORT_ABILITY_SPEED_MAX(dab->speed_full_duplex |
                                           dab->speed_half_duplex);
            cur_duplex = (dab->speed_full_duplex ?
                          SOC_PORT_DUPLEX_FULL : SOC_PORT_DUPLEX_HALF);
            cur_pause_tx = (dab->pause & BCM_PORT_ABILITY_PAUSE_TX) != 0;
            cur_pause_rx = (dab->pause & BCM_PORT_ABILITY_PAUSE_RX) != 0;

            new_speed = (flags & BCM_PORT_ATTR_SPEED_MASK ?
                         info_speed_adj : cur_speed);
            new_duplex = (flags & BCM_PORT_ATTR_DUPLEX_MASK ?
                          src->duplex : cur_duplex);
            new_pause_tx = (flags & BCM_PORT_ATTR_PAUSE_TX_MASK ?
                            src->pause_tx : cur_pause_tx);
            new_pause_rx = (flags & BCM_PORT_ATTR_PAUSE_RX_MASK ?
                            src->pause_rx : cur_pause_rx);

            if (new_duplex != SOC_PORT_DUPLEX_HALF) {
                mode = sab->speed_full_duplex;
                mode &= port_speed_max_mask(new_speed);
                dab->speed_full_duplex = mode;
            } else {
                dab->speed_full_duplex = 0;
            }
            
            mode = sab->speed_half_duplex;
            mode &= port_speed_max_mask(new_speed);
            dab->speed_half_duplex = mode;

            if (!(sab->pause &
                  BCM_PORT_ABILITY_PAUSE_ASYMM) &&
                (new_pause_tx != new_pause_rx)) {
                printk("port parse: Error: Asymmetrical pause not available\n");
                return -1;
            }

            if (!new_pause_tx) {
                dab->pause &= ~BCM_PORT_ABILITY_PAUSE_TX;
            } else {
                dab->pause |= BCM_PORT_ABILITY_PAUSE_TX;
            }

            if (!new_pause_rx) {
                dab->pause &= ~BCM_PORT_ABILITY_PAUSE_RX;
            } else {
                dab->pause |= BCM_PORT_ABILITY_PAUSE_RX;
            }
            dab->eee = sab->eee;
        } else {
            mode = dest->local_advert;

            cur_speed = BCM_PORT_ABIL_SPD_MAX(mode);
            cur_duplex = ((mode & BCM_PORT_ABIL_FD) ?
                          SOC_PORT_DUPLEX_FULL : SOC_PORT_DUPLEX_HALF);
            cur_pause_tx = (mode & BCM_PORT_ABIL_PAUSE_TX) != 0;
            cur_pause_rx = (mode & BCM_PORT_ABIL_PAUSE_RX) != 0;

            new_speed = (flags & BCM_PORT_ATTR_SPEED_MASK ?
                         info_speed_adj : cur_speed);
            new_duplex = (flags & BCM_PORT_ATTR_DUPLEX_MASK ?
                          src->duplex : cur_duplex);
            new_pause_tx = (flags & BCM_PORT_ATTR_PAUSE_TX_MASK ?
                            src->pause_tx : cur_pause_tx);
            new_pause_rx = (flags & BCM_PORT_ATTR_PAUSE_RX_MASK ?
                            src->pause_rx : cur_pause_rx);

            /* Start with maximum ability and cut down */

            mode = src->ability;

            if (new_duplex == SOC_PORT_DUPLEX_HALF) {
                mode &= ~BCM_PORT_ABIL_FD;
            }

            if (new_speed < 13000) {
            	mode &= ~BCM_PORT_ABIL_13GB;
            }

            if (new_speed < 12000) {
                mode &= ~BCM_PORT_ABIL_12GB;
            }

            if (new_speed < 10000) {
                mode &= ~BCM_PORT_ABIL_10GB;
            }

            if (new_speed < 2500) {
                mode &= ~BCM_PORT_ABIL_2500MB;
            }

            if (new_speed < 1000) {
                mode &= ~BCM_PORT_ABIL_1000MB;
            }

            if (new_speed < 100) {
                mode &= ~BCM_PORT_ABIL_100MB;
            }

            if (!(mode & BCM_PORT_ABIL_PAUSE_ASYMM) &&
                (new_pause_tx != new_pause_rx)) {
                printk("port parse: Error: Asymmetrical pause not available\n");
                return -1;
            }

            if (!new_pause_tx) {
                mode &= ~BCM_PORT_ABIL_PAUSE_TX;
            }

            if (!new_pause_rx) {
                mode &= ~BCM_PORT_ABIL_PAUSE_RX;
            }

            dest->local_advert = mode;
        }
    } else {
        /* Update forced values for speed/duplex/pause */

        if (flags & BCM_PORT_ATTR_SPEED_MASK) {
            dest->speed = info_speed_adj;
        }

        if (flags & BCM_PORT_ATTR_DUPLEX_MASK) {
            dest->duplex = src->duplex;
        }

        if (flags & BCM_PORT_ATTR_PAUSE_TX_MASK) {
            dest->pause_tx = src->pause_tx;
        }

        if (flags & BCM_PORT_ATTR_PAUSE_RX_MASK) {
            dest->pause_rx = src->pause_rx;
        }
    }

    if (flags & BCM_PORT_ATTR_PAUSE_MAC_MASK) {
        sal_memcpy(dest->pause_mac, src->pause_mac, sizeof (sal_mac_addr_t));
    }

    if (flags & BCM_PORT_ATTR_LINKSCAN_MASK) {
        dest->linkscan = src->linkscan;
    }

    if (flags & BCM_PORT_ATTR_LEARN_MASK) {
        dest->learn = src->learn;
    }

    if (flags & BCM_PORT_ATTR_DISCARD_MASK) {
        dest->discard = src->discard;
    }

    if (flags & BCM_PORT_ATTR_VLANFILTER_MASK) {
        dest->vlanfilter = src->vlanfilter;
    }

    if (flags & BCM_PORT_ATTR_UNTAG_PRI_MASK) {
        dest->untagged_priority = src->untagged_priority;
    }

    if (flags & BCM_PORT_ATTR_PFM_MASK) {
        dest->pfm = src->pfm;
    }

    if (flags & BCM_PORT_ATTR_PHY_MASTER_MASK) {
        dest->phy_master = src->phy_master;
    }

    if (flags & BCM_PORT_ATTR_INTERFACE_MASK) {
        dest->interface = src->interface;
    }

    if (flags & BCM_PORT_ATTR_LOOPBACK_MASK) {
        dest->loopback = src->loopback;
    }

    if (flags & BCM_PORT_ATTR_ENCAP_MASK) {
        dest->encap_mode = src->encap_mode;
    }

    if (flags & BCM_PORT_ATTR_FRAME_MAX_MASK) {
        dest->frame_max = src->frame_max;
    }

    if (flags & BCM_PORT_ATTR_MDIX_MASK) {
        dest->mdix = src->mdix;
    }

    return 0;
}

/* Iterate thru a port bitmap with the given mask; display info */
STATIC int
_port_disp_iter(int unit, pbmp_t pbm, pbmp_t pbm_mask, uint32 seen)
{
    bcm_port_info_t info;
    soc_port_t port, dport;
    int r;

    BCM_PBMP_AND(pbm, pbm_mask);
    DPORT_BCM_PBMP_ITER(unit, pbm, dport, port) {

        sal_memset(&info, 0, sizeof(bcm_port_info_t));
        port_info_init(unit, port, &info, seen);

        if ((r = bcm_port_selective_get(unit, port, &info)) < 0) {
            printk("Error: Could not get port %s information: %s\n",
                   BCM_PORT_NAME(unit, port), bcm_errmsg(r));
            return (CMD_FAIL);
        }

        disp_port_info(BCM_PORT_NAME(unit, port), &info, 
                       IS_ST_PORT(unit, port), seen);
    }

    return CMD_OK;
}

/*
 * Function:
 *	if_port
 * Purpose:
 *	Configure port specific parameters.
 * Parameters:
 *	u	- SOC unit #
 *	a	- pointer to args
 * Returns:
 *	CMD_OK/CMD_FAIL
 */
cmd_result_t
if_esw_port(int u, args_t *a)
{
    pbmp_t              pbm;
    bcm_port_config_t   pcfg;
    bcm_port_info_t     *info_all;
    bcm_port_info_t     info_given;
    bcm_port_ability_t  *ability_all;       /* Abilities for all ports */
    bcm_port_ability_t  ability_port;       /* Ability for current port */
    bcm_port_ability_t  ability_given;  
    char                *c;
    int                 r, rv = 0, cmd_rv = CMD_OK;
    soc_port_t          p, dport;
    parse_table_t       pt;
    uint32              seen = 0;
    uint32              parsed = 0, parsed_adj, pa_speed;
    char                pfmt[SOC_PBMP_FMT_LEN];
    int                 eee_tx_idle_time=0, eee_tx_wake_time=0, eee_state, i;
    int                 eee_tx_ev_cnt, eee_tx_dur, eee_rx_ev_cnt, eee_rx_dur;
    char                *eee_enable=NULL, *stats=NULL, *str=NULL;
    uint32              flags;

    if (!sh_check_attached(ARG_CMD(a), u)) {
        return CMD_FAIL;
    }

    if (bcm_port_config_get(u, &pcfg) != BCM_E_NONE) {
        printk("%s: Error: bcm ports not initialized\n", ARG_CMD(a));
        return CMD_FAIL;
    }

    if ((c = ARG_GET(a)) == NULL) {
        return(CMD_USAGE);
    }

    info_all = sal_alloc(SOC_MAX_NUM_PORTS * sizeof(bcm_port_info_t), 
                         "if_port");

    if (info_all == NULL) {
        printk("Insufficient memory.\n");
        return CMD_FAIL;
    }

    ability_all = sal_alloc(SOC_MAX_NUM_PORTS * sizeof(bcm_port_ability_t), 
        "if_port");

    if (ability_all == NULL) {
        printk("Insufficient memory.\n");
        sal_free(info_all);
        return CMD_FAIL;
    }

    sal_memset(&info_given, 0, sizeof(bcm_port_info_t));
    sal_memset(info_all, 0, SOC_MAX_NUM_PORTS * sizeof(bcm_port_info_t));
    sal_memset(&ability_given, 0, sizeof(bcm_port_ability_t));
    sal_memset(&ability_port, 0, sizeof(bcm_port_ability_t));
    sal_memset(ability_all, 0, SOC_MAX_NUM_PORTS * sizeof(bcm_port_ability_t));

    if (parse_bcm_pbmp(u, c, &pbm) < 0) {
        printk("%s: Error: unrecognized port bitmap: %s\n",
               ARG_CMD(a), c);
        sal_free(info_all);
        sal_free(ability_all);
        return CMD_FAIL;
    }

    BCM_PBMP_AND(pbm, pcfg.port);

    if (BCM_PBMP_IS_NULL(pbm)) {
        ARG_DISCARD(a);
        sal_free(info_all);
        sal_free(ability_all);
        return CMD_OK;
    }

    if (ARG_CNT(a) == 0) {
        seen = BCM_PORT_ATTR_ALL_MASK;
    } else {
        /*
         * Otherwise, arguments are given.  Use them to determine which
         * properties need to be gotten/set.
         *
         * Probe and detach, hidden commands.
         */
        if (!sal_strcasecmp(_ARG_CUR(a), "detach")) {
            pbmp_t detached;
            bcm_port_detach(u, pbm, &detached);
            printk("Detached port bitmap %s\n", SOC_PBMP_FMT(detached, pfmt));
            ARG_GET(a);
            sal_free(info_all);
            sal_free(ability_all);
            return CMD_OK;
        } else if ((!sal_strcasecmp(_ARG_CUR(a), "probe")) ||
                   (!sal_strcasecmp(_ARG_CUR(a), "attach"))) {
            pbmp_t probed;
            bcm_port_probe(u, pbm, &probed);
            printk("Probed port bitmap %s\n", SOC_PBMP_FMT(probed, pfmt));
            ARG_GET(a);
            sal_free(info_all);
            sal_free(ability_all);
            return CMD_OK;
        } else if (!sal_strcasecmp(_ARG_CUR(a), "lanes")) {
            int lanes;
            ARG_GET(a);
            if ((c = ARG_GET(a)) == NULL) {
                sal_free(info_all);
                sal_free(ability_all);
                return CMD_USAGE;
            }
            lanes = sal_ctoi(c, 0);
            DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {
                rv = bcm_port_control_set(u, p, bcmPortControlLanes, lanes);
                if (rv < 0) {
                    break;
                }
            }
            ARG_GET(a);
            sal_free(info_all);
            sal_free(ability_all);
            if (rv < 0) {
                return CMD_FAIL;
            } else {
                return CMD_OK;
            }
        }else if (!sal_strcasecmp(_ARG_CUR(a), "EEE")) {
            sal_free(info_all);
            sal_free(ability_all);
            c = ARG_GET(a);
            if ((c = ARG_CUR(a)) != NULL) {

                parse_table_init(u, &pt);
                parse_table_add(&pt, "ENable", PQ_DFL | PQ_STRING,
                                0, &eee_enable, 0);
                parse_table_add(&pt, "TxIDleTime", PQ_DFL | PQ_INT,
                                0, &eee_tx_idle_time, 0);
                parse_table_add(&pt, "TxWakeTime", PQ_DFL | PQ_INT,
                                0, &eee_tx_wake_time, 0);
                parse_table_add(&pt, "STats", PQ_DFL | PQ_STRING,
                            0, &stats, 0);

                if (parse_arg_eq(a, &pt) < 0) {
                    parse_arg_eq_done(&pt);
                    return CMD_USAGE;
                }
                if (ARG_CNT(a) > 0) {
                    printk("%s: Unknown argument %s\n", ARG_CMD(a), ARG_CUR(a));
                    parse_arg_eq_done(&pt);
                    return CMD_USAGE;
                }

                flags=0;

                for (i = 0; i < pt.pt_cnt; i++) {
                    if (pt.pt_entries[i].pq_type & PQ_PARSED) {
                        flags |= (1 << i);
                    }
                }

                DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
                    printk("Port = %5s(%3d)\n", SOC_PORT_NAME(u, p), p);
                    if (flags & 0x1) {
                        if (sal_strcasecmp(eee_enable, "enable") == 0) {
                            rv = bcm_port_control_set(u, p, 
                                     bcmPortControlEEEEnable, 1);
                        }
                        if (sal_strcasecmp(eee_enable, "disable") == 0) {
                            rv = bcm_port_control_set(u, p, 
                                     bcmPortControlEEEEnable, 0);
                        }

                        if (rv == BCM_E_NONE) {
                                printk("EEE state set to %s\n", eee_enable);
                        } else {
                                if (rv == BCM_E_UNAVAIL) {
                                    printk("EEE %s is not available\n", eee_enable);
                                } else {
                                    printk("EEE %s is unsuccessful\n", eee_enable);
                                }
                        }
                        rv = bcm_port_control_get(u, p, 
                                 bcmPortControlEEEEnable, &eee_state);
                        if (rv == BCM_E_NONE) {
                            if (eee_state == 1) {
                                str = "Enable";
                            } else {
                                str = "Disable";
                            }
                        } else {
                                str = "NA";
                        }
                        printk("EEE State = %s\n", str);
                    } 

                    if (flags & 0x2) {
                        if ((rv = bcm_port_control_set(u, p,
                                      bcmPortControlEEETransmitIdleTime,
                                      eee_tx_idle_time)) != BCM_E_NONE) {
                            printk("Port control set:bcmPortControlEEETransmitIdleTime "
                                       "failed with %d\n", rv);
                            return CMD_FAIL;
                        }
                        if ((rv = bcm_port_control_get(u, p,
                                      bcmPortControlEEETransmitIdleTime,
                                      &eee_tx_idle_time)) != BCM_E_NONE) {
                            printk("Port control get:bcmPortControlEEETransmitIdleTime "
                                       "failed with %d\n", rv);
                            return CMD_FAIL;
                        }
                        printk("EEE Transmit Idle Time is = %d(us)\n", eee_tx_idle_time);
                    }

                    if (flags & 0x4) {
                        if ((rv = bcm_port_control_set(u, p,
                                      bcmPortControlEEETransmitWakeTime,
                                      eee_tx_wake_time)) != BCM_E_NONE) {
                            printk("Port control set:bcmPortControlEEETransmitWakeTime "
                                       "failed with %d\n", rv);
                            return CMD_FAIL;
                        }
                        if ((rv = bcm_port_control_get(u, p,
                                      bcmPortControlEEETransmitWakeTime,
                                      &eee_tx_wake_time)) != BCM_E_NONE) {
                            printk("Port control get:bcmPortControlEEETransmitWakeTime "
                                       "failed with %d\n", rv);
                            return CMD_FAIL;
                        }
                        printk("EEE Transmit Wake Time is = %d(us)\n", eee_tx_wake_time);
                    }

                    if (flags & 0x8) {
                        if (sal_strcasecmp(stats, "get") == 0) {
                            if ((rv = bcm_port_control_get(u, p,
                                          bcmPortControlEEETransmitEventCount,
                                          &eee_tx_ev_cnt)) != BCM_E_NONE) {
                                printk("Port control get:bcmPortControlEEETransmitEventCount "
                                       "failed with %d\n", rv);
                                return CMD_FAIL;
                            }
                            if ((rv = bcm_port_control_get(u, p, 
                                          bcmPortControlEEETransmitDuration,
                                          &eee_tx_dur)) != BCM_E_NONE) {
                                printk("Port control get:bcmPortControlEEETransmitDuration "
                                       "failed with %d\n", rv);
                                return CMD_FAIL;
                            }
                            if ((rv = bcm_port_control_get(u, p, 
                                          bcmPortControlEEEReceiveEventCount,
                                          &eee_rx_ev_cnt)) != BCM_E_NONE) {
                                printk("Port control get:bcmPortControlEEEReceiveEventCount "
                                       "failed with %d\n", rv);
                                return CMD_FAIL;
                            }
                            if ((rv = bcm_port_control_get(u, p,
                                          bcmPortControlEEEReceiveDuration,
                                          &eee_rx_dur)) != BCM_E_NONE) {
                                printk("Port control get:bcmPortControlEEEReceiveDuration "
                                       "failed with %d\n", rv);
                                return CMD_FAIL;
                            }
                            printk("Tx events = %d TX Duration = %d(us) RX events = %d RX "
                                    "Duration = %d(us)\n", eee_tx_ev_cnt, eee_tx_dur, 
                                              eee_rx_ev_cnt, eee_rx_dur);
                        }
                    }
                }
                /* free allocated memory from arg parsing */
                parse_arg_eq_done(&pt); 
            } else {
                printk("EEE Details:\n");
                printk("%10s %9s %14s %14s %12s %14s %12s %14s\n",
                   "port", "EEE-State", "TxIdleTime(us)", "TxWakeTime(us)", 
                   "TxEventCount", "TxDuration(us)", "RxEventCount", 
                   "RxDuration(us)");
                DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {                  
                
                    if ((rv = bcm_port_control_get(u, p, bcmPortControlEEEEnable,
                                  &eee_state)) != BCM_E_NONE) {
                        printk("Port control get:bcmPortControlEEEEnable "
                               "failed with %d\n", rv);
                        return CMD_FAIL;
                    }
                    if ((rv = bcm_port_control_get(u, p, 
                                  bcmPortControlEEETransmitIdleTime,
                                  &eee_tx_idle_time)) != BCM_E_NONE) {
                        printk("Port control get:bcmPortControlEEETransmitIdleTime "
                               "failed with %d\n", rv);
                        return CMD_FAIL;
                    }
                    if ((rv = bcm_port_control_get(u, p, 
                                  bcmPortControlEEETransmitWakeTime,
                                  &eee_tx_wake_time)) != BCM_E_NONE) {
                        printk("Port control get:bcmPortControlEEETransmitWakeTime "
                               "failed with %d\n", rv);
                        return CMD_FAIL;
                    }
                    if (eee_state) {
                        if ((rv = bcm_port_control_get(u, p,
                                      bcmPortControlEEETransmitEventCount,
                                      &eee_tx_ev_cnt)) != BCM_E_NONE) {
                            printk("Port control get:bcmPortControlEEETransmitEventCount "
                                   "failed with %d\n", rv);
                            return CMD_FAIL;
                        }
                        if ((rv = bcm_port_control_get(u, p,
                                      bcmPortControlEEETransmitDuration,
                                      &eee_tx_dur)) != BCM_E_NONE) {
                            printk("Port control get:bcmPortControlEEETransmitDuration "
                                   "failed with %d\n", rv);
                            return CMD_FAIL;
                        }
                        if ((rv = bcm_port_control_get(u, p,
                                      bcmPortControlEEEReceiveEventCount,
                                      &eee_rx_ev_cnt)) != BCM_E_NONE) {
                            printk("Port control get:bcmPortControlEEEReceiveEventCount "
                                   "failed with %d\n", rv);
                            return CMD_FAIL;
                        }
                        if ((rv = bcm_port_control_get(u, p,
                                      bcmPortControlEEEReceiveDuration,
                                      &eee_rx_dur)) != BCM_E_NONE) {
                            printk("Port control get:bcmPortControlEEEReceiveDuration "
                                   "failed with %d\n", rv);
                            return CMD_FAIL;
                        }
                    }
                    else {
                        eee_tx_ev_cnt = -1;
                        eee_tx_dur = -1; 
                        eee_rx_ev_cnt = -1;
                        eee_rx_dur = -1;
                    }
                    
                    printk("%5s(%3d) %9s %14d %14d %12d %14d %12d %14d\n",
                       SOC_PORT_NAME(u, p), p,
                       eee_state ? "Enable" : "Disable", eee_tx_idle_time, 
                       eee_tx_wake_time, eee_tx_ev_cnt, eee_tx_dur, 
                       eee_rx_ev_cnt, eee_rx_dur);
                }
            }
            return CMD_OK;
        }

        if (!sal_strcmp(_ARG_CUR(a), "=")) {
            /*
             * For "=" where the user is prompted to enter all the parameters,
             * use the parameters from the first selected port as the defaults.
             */
            if (ARG_CNT(a) != 1) {
                sal_free(info_all);
                sal_free(ability_all);
                return CMD_USAGE;
            }
            DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {
                break;    /* Find first port in bitmap */
            }
            port_info_init(u,p,&info_given,0);
            if ((rv = bcm_port_info_get(u, p, &info_given)) < 0) {
                printk("%s: Error: Failed to get port info\n", ARG_CMD(a));
                sal_free(info_all);
                sal_free(ability_all);
                return CMD_FAIL;
            }
        } else {
            DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {
                break;    /* Find first port in bitmap */
            }
            port_info_init(u,p,&info_given,0);
        }

        /*
         * Parse the arguments.  Determine which ones were actually given.
         */
        port_parse_setup(u, &pt, &info_given);
        
        if (parse_arg_eq(a, &pt) < 0) {
            parse_arg_eq_done(&pt);
            sal_free(info_all);
            sal_free(ability_all);
            return(CMD_FAIL);
        }

        /* Translate port_info into port abilities. */

        if (ARG_CNT(a) > 0) {
            printk("%s: Unknown argument %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            sal_free(info_all);
            sal_free(ability_all);
            return(CMD_FAIL);
        }

        /*
         * Find out what parameters specified.  Record values specified.
         */
        port_parse_mask_get(&pt, &seen, &parsed);
        parse_arg_eq_done(&pt);
    }

    if (seen && parsed) {
        soc_cm_print("%s: Cannot get and set "
                "port properties in one command\n", ARG_CMD(a));
        sal_free(info_all);
        sal_free(ability_all);
        return CMD_FAIL;
    } else if (seen) { /* Show selected information */
        printk("%s: Status (* indicates PHY link up)\n", ARG_CMD(a));
        /* Display the information by port type */
#define _call_pdi(u, p, mp, s) \
        if (_port_disp_iter(u, p, mp, s) != CMD_OK) { \
             sal_free(info_all); \
             sal_free(ability_all); \
             return CMD_FAIL; \
        }
        _call_pdi(u, pbm, pcfg.fe, seen);
        _call_pdi(u, pbm, pcfg.ge, seen);
        _call_pdi(u, pbm, pcfg.xe, seen);
        _call_pdi(u, pbm, pcfg.ce, seen);
        _call_pdi(u, pbm, pcfg.hg, seen);
        sal_free(info_all);
        sal_free(ability_all);
        return(CMD_OK);
    }

    /* Some set information was given */

    if (parsed & BCM_PORT_ATTR_LINKSCAN_MASK) {
        /* Map ON --> S/W, OFF--> None */
        if (info_given.linkscan > 2) {
            info_given.linkscan -= 3;
        }
    }

    parsed_adj = parsed;
    if (parsed & (BCM_PORT_ATTR_SPEED_MASK | BCM_PORT_ATTR_DUPLEX_MASK)) {
        parsed_adj |= BCM_PORT_ATTR_SPEED_MASK | BCM_PORT_ATTR_DUPLEX_MASK;
    }

    /*
     * Retrieve all requested port information first, then later write
     * back all port information.  That prevents a problem with loopback
     * cables where setting one port's info throws another into autoneg
     * causing it to return info in flux (e.g. suddenly go half duplex).
     */

    DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {
        port_info_init(u, p, &info_all[p], parsed_adj);
        if ((r = bcm_port_selective_get(u, p, &info_all[p])) < 0) {
            printk("%s: Error: Could not get port %s information: %s\n",
                   ARG_CMD(a), BCM_PORT_NAME(u, p), bcm_errmsg(r));
            sal_free(info_all);
            sal_free(ability_all);
            return (CMD_FAIL);
        }
    }

    /*
     * Loop through all the specified ports, changing whatever field
     * values were actually given.  This avoids copying unaffected
     * information from one port to another and prevents having to
     * re-parse the arguments once per port.
     */

    DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {
        if ((rv = bcm_port_speed_max(u, p, &info_given.speed_max)) < 0) {
            printk("port parse: Error: Could not get port %s max speed: %s\n",
                   BCM_PORT_NAME(u, p), bcm_errmsg(rv));
            continue;
        }

        if ((rv = bcm_port_ability_local_get(u, p, &info_given.port_ability)) < 0) {
            printk("port parse: Error: Could not get port %s ability: %s\n",
                   BCM_PORT_NAME(u, p), bcm_errmsg(rv));
            continue;
        }
        if ((rv = soc_port_ability_to_mode(&info_given.port_ability,
                                           &info_given.ability)) < 0) {
            printk("port parse: Error: Could not transform port %s ability to mode: %s\n",
                   BCM_PORT_NAME(u, p), bcm_errmsg(rv));
            continue;
        }

        if ((r = port_parse_port_info_set(parsed,
                                          &info_given, &info_all[p])) < 0) {
            printk("%s: Error: Could not parse port %s info: %s\n",
                   ARG_CMD(a), BCM_PORT_NAME(u, p), bcm_errmsg(r));
            cmd_rv = CMD_FAIL;
            continue;
        }

        /* If AN is on, do not set speed, duplex, pause */
        if (info_all[p].autoneg) {
            info_all[p].action_mask &= ~BCM_PORT_AN_ATTRS;
        } else if (parsed &
                   (BCM_PORT_ATTR_SPEED_MASK | BCM_PORT_ATTR_DUPLEX_MASK)) {
            pa_speed = SOC_PA_SPEED(info_all[p].speed);
            if (info_all[p].duplex) {
                if (!(info_given.port_ability.speed_full_duplex & pa_speed)) {
                    printk("%s: port %s does not support %d Mbps full duplex\n", ARG_CMD(a), BCM_PORT_NAME(u, p), info_all[p].speed);
                    continue;
                }
            } else {
                if (!(info_given.port_ability.speed_half_duplex & pa_speed)) {
                    printk("%s: port %s does not support %d Mbps half duplex\n", ARG_CMD(a), BCM_PORT_NAME(u, p), info_all[p].speed);
                    continue;
                }
            }
        }

        if ((r = bcm_port_selective_set(u, p, &info_all[p])) < 0) {
            printk("%s: Error: Could not set port %s information: %s\n",
                   ARG_CMD(a), BCM_PORT_NAME(u, p), bcm_errmsg(r));
            cmd_rv = CMD_FAIL;
            continue;
        }
    }

    sal_free(info_all);
    sal_free(ability_all);

    return(cmd_rv);
}


cmd_result_t
if_esw_gport(int u, args_t *a)
{
    char            *c;
    soc_port_t      arg_port = 0;
    parse_table_t	pt;
    int             retCode;

    if (!sh_check_attached(ARG_CMD(a), u)) {
        return CMD_FAIL;
    }

    if ((c = ARG_CUR(a)) == NULL) {
        return(CMD_USAGE);
    }
    parse_table_init(u, &pt);
    parse_table_add(&pt, "Port", PQ_DFL|PQ_PORT, 0, &arg_port, NULL);
    if (!parseEndOk(a, &pt, &retCode)) {
        printk(" Unable to Parse GPORT !!! \n");
        return retCode;
    }

    printk(" gport is = 0x%x\n", arg_port);
    return CMD_OK;
}



char if_egress_usage[] =
    "Usages:\n\t"
    "  egress set [<Port=port#>] [<Modid=modid>] <PBmp=val>\n\t"
    "        - Set allowed egress bitmap for (modid,port) or all\n\t"
    "  egress show [<Port=port#>] [<Module=modid>]\n\t"
    "        - Show allowed egress bitmap for (modid,port)\n";

/*
 * Note:
 *
 * Since these port numbers are likely on different modules, we cannot
 * use PBMP qualifiers by this unit.
 */

cmd_result_t
if_egress(int unit, args_t *a)
{
    char 		*subcmd, *c;
    int                 port, arg_port = -1, min_port = 0,
                        max_port = 31;
    int		        modid, arg_modid = -1, mod_min = 0,
                        mod_max = SOC_MODID_MAX(unit);
    bcm_pbmp_t 		pbmp;
    int 		r;
    bcm_pbmp_t 	arg_pbmp;
    parse_table_t	pt;
    cmd_result_t	retCode;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if ((subcmd = ARG_GET(a)) == NULL) {
	return CMD_USAGE;
    }

    BCM_PBMP_CLEAR(pbmp);
    BCM_PBMP_CLEAR(arg_pbmp);

    /* Egress show command */
    if (sal_strcasecmp(subcmd, "show") == 0) {

	if ((c = ARG_CUR(a)) != NULL) {
            parse_table_init(unit, &pt);
            parse_table_add(&pt, "Port", PQ_DFL|PQ_PORT, 0, &arg_port, NULL);
            parse_table_add(&pt, "Modid",   PQ_DFL|PQ_INT,  0, &arg_modid,
                            NULL);
            if (!parseEndOk(a, &pt, &retCode)) {
                return retCode;
            }

            if (BCM_GPORT_IS_SET(arg_port)) {
#if defined(BCM_ESW_SUPPORT)
                int tgid, id;

                if (SOC_IS_ESW(unit)) {
                    r = _bcm_esw_gport_resolve(unit, arg_port, &arg_modid, 
                                               &arg_port, &tgid, &id);
                    if ((tgid != BCM_TRUNK_INVALID) || (id != -1)) {
                        return CMD_FAIL;
                    }
                } else {
                    return CMD_FAIL;
                }
#else
                return CMD_FAIL;
#endif
            }
            if (arg_modid >= 0) {
                mod_min = mod_max = arg_modid;
            }
            if (arg_port >= 0) {
                min_port = max_port = arg_port;
            }
	}

	for (modid = mod_min; modid <= mod_max; modid++) {
            for (port = min_port; port <= max_port; port++) {
                r = bcm_port_egress_get(unit, port, modid, &pbmp);
                if (r < 0) {
                    printk("%s: egress (modid=%d, port=%d) get failed: %s\n",
                           ARG_CMD(a), modid, port, bcm_errmsg(r));
                    return CMD_FAIL;
                }

                if (BCM_PBMP_NEQ(pbmp, PBMP_ALL(unit))) {
		    char	buf[FORMAT_PBMP_MAX];
                    format_bcm_pbmp(unit, buf, sizeof (buf), pbmp);
                    printk("Module %d, port %d:  Enabled egress ports %s\n",
                           modid, port, buf);
                }
            }
        }

	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "set") == 0) {
	parse_table_init(unit, &pt);
        parse_table_add(&pt, "Port", PQ_DFL|PQ_PORT, 0, &arg_port, NULL);
        parse_table_add(&pt, "Modid",   PQ_DFL|PQ_INT,  0, &arg_modid,  NULL);
	parse_table_add(&pt, "Pbmp", PQ_DFL|PQ_PBMP|PQ_BCM, 0, &arg_pbmp, NULL);
	if (!parseEndOk(a, &pt, &retCode)) {
	    return retCode;
        }

        SOC_PBMP_ASSIGN(pbmp, arg_pbmp);

	r = bcm_port_egress_set(unit, arg_port, arg_modid, pbmp);

    } else {
	return CMD_USAGE;
    }

    if (r < 0) {
	printk("%s: ERROR: %s\n", ARG_CMD(a), bcm_errmsg(r));
	return CMD_FAIL;
    }

    return CMD_OK;
}

cmd_result_t
if_esw_dscp(int unit, args_t *a)
{
    int         rv, port, dport, srccp, mapcp, prio, cng, mode, count, i;
    bcm_port_config_t pcfg;
    bcm_pbmp_t	pbm;
    bcm_pbmp_t  tpbm;
    int         use_global = 0;
    char	*s;
    parse_table_t pt;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (bcm_port_config_get(unit, &pcfg) != BCM_E_NONE) {
        printk("%s: Error: bcm ports not initialized\n", ARG_CMD(a));
        return CMD_FAIL;
    }

    if ((s = ARG_GET(a)) == NULL) {
	printk("%s: ERROR: missing port bitmap\n", ARG_CMD(a));
	return CMD_FAIL;
    }
    if (parse_bcm_pbmp(unit, s, &pbm) < 0) {
	printk("%s: ERROR: unrecognized port bitmap: %s\n", ARG_CMD(a), s);
	return CMD_FAIL;
    }

    BCM_PBMP_ASSIGN(tpbm, pbm);
    BCM_PBMP_XOR(tpbm, pcfg.e);
    if (BCM_PBMP_IS_NULL(tpbm)) {
        /* global table */
        use_global = 1;
    }

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "Mode", PQ_INT, (void *)-1, &mode, NULL);
    rv = parse_arg_eq(a, &pt);
    parse_arg_eq_done(&pt);
    if (BCM_FAILURE(rv)) {
        return CMD_FAIL;
    }

    srccp = -1;
    s = ARG_GET(a);
    if (s) {
        srccp = parse_integer(s);
    }

    if ((s = ARG_GET(a)) == NULL) {
        if (mode != -1) {
            printk("%s: WARNING: ignore mode argument\n", ARG_CMD(a));
        }
        if (srccp < 0) {
            srccp = 0;
            count = 64;
        } else {
            count = 1;
        }
        if (BCM_PBMP_IS_NULL(pbm) && 
            !soc_feature(unit, soc_feature_dscp_map_per_port) ) {
            port = -1;
            rv = bcm_port_dscp_map_get(unit, port, 0, &mapcp, &prio);
            if (rv == BCM_E_PORT) {
                port = -1;
                printk("%d: dscp map:\n", unit);
            }
            for (i = 0; i < count; i++) {
                rv = bcm_port_dscp_map_get(unit, port, srccp + i, &mapcp,
                                           &prio);
                if (rv < 0) {
                    printk("ERROR: dscp map get %d failed: %s\n",
                           srccp + i, bcm_errmsg(rv));
                    return CMD_FAIL;
                }
                if (srccp + i != mapcp || count == 1) {
                    printk(" %d->%d prio=%d cng=%d\n",
                           srccp + i, mapcp,
                           prio & BCM_PRIO_MASK,
                           (prio & BCM_PRIO_DROP_FIRST) ? 1 : 0);

                }
            }
            printk("\n");
        } else {
        DPORT_BCM_PBMP_ITER(unit, pbm, dport, port) {
            if (use_global) {
                rv = bcm_port_dscp_map_get(unit, port, 0, &mapcp, &prio);
                if (rv == BCM_E_PORT) {
                    port = -1;
                    printk("%d: dscp map:\n", unit);
                }
            } else {
                printk("%d:%s dscp map:\n", unit, BCM_PORT_NAME(unit, port));
            }
	    for (i = 0; i < count; i++) {
		rv = bcm_port_dscp_map_get(unit, port, srccp + i, &mapcp,
                                           &prio);
		if (rv < 0) {
		    printk("ERROR: dscp map get %d failed: %s\n",
			   srccp + i, bcm_errmsg(rv));
		    return CMD_FAIL;
		}
		if (srccp + i != mapcp || count == 1) {
		    printk(" %d->%d prio=%d cng=%d\n",
			   srccp + i, mapcp,
			   prio & BCM_PRIO_MASK,
			   (prio & BCM_PRIO_DROP_FIRST) ? 1 : 0);

                    }
                }
	    printk("\n");
            if (port == -1) {
                break;
            }
	}
        }
	return CMD_OK;
    }

    mapcp = parse_integer(s);
    prio = -1;
    cng = 0;

    if ((s = ARG_GET(a)) != NULL) {
	prio = parse_integer(s);
	if ((s = ARG_GET(a)) != NULL) {
	    cng = parse_integer(s);
	}
    }
    if (cng) prio |= BCM_PRIO_DROP_FIRST;

    /* Allow empty pbmp to configure devices that don't support per port */
    /*  dscp mapping */
    if (BCM_PBMP_IS_NULL(pbm) && 
        !soc_feature(unit, soc_feature_dscp_map_per_port) ) {
        port = -1;

        if (mode != -1) {
            rv = bcm_port_dscp_map_mode_set(unit, port, mode);
            if (rv < 0) {
                printk("%d:%s ERROR: dscp mode set mode=%d: %s\n",
                       unit, (port == -1) ? "" : BCM_PORT_NAME(unit, port),
                       mode, bcm_errmsg(rv));
                return CMD_FAIL;
            }
        }

        rv = bcm_port_dscp_map_set(unit, port, srccp, mapcp, prio);
        if (rv < 0) {
            printk("%d:%s ERROR: "
               "dscp map set %d->%d prio=%d cng=%d failed: %s\n",
               unit, (port == -1) ? "" : BCM_PORT_NAME(unit, port),
               srccp, mapcp, prio, cng, bcm_errmsg(rv));
            return CMD_FAIL;
        }
    } else {
    DPORT_BCM_PBMP_ITER(unit, pbm, dport, port) {
        if (use_global) {
            port = -1;
        }

        if (mode != -1) {
            rv = bcm_port_dscp_map_mode_set(unit, port, mode);
            if (rv < 0) {
                printk("%d:%s ERROR: dscp mode set mode=%d: %s\n",
                       unit, (port == -1) ? "" : BCM_PORT_NAME(unit, port),
                       mode, bcm_errmsg(rv));
                return CMD_FAIL;
            }
        }

	rv = bcm_port_dscp_map_set(unit, port, srccp, mapcp, prio);
	if (rv < 0) {
	    printk("%d:%s ERROR: "
		   "dscp map set %d->%d prio=%d cng=%d failed: %s\n",
		   unit, (port == -1) ? "" : BCM_PORT_NAME(unit, port),
		   srccp, mapcp, prio, cng, bcm_errmsg(rv));
	    return CMD_FAIL;
	}
        if (port == -1) {
            break;
        }
    }
    }
    return CMD_OK;
}

cmd_result_t
if_esw_ipg(int unit, args_t *a)
/*
 * Function: 	if_ipg
 * Purpose:	Configure default IPG values.
 * Parameters:	unit - SOC unit #
 *		a - arguments
 * Returns:	CMD_OK/CMD_FAIL
 */
{
    parse_table_t      pt;
    pbmp_t             arg_pbmp;
    int                arg_speed, speed;
    bcm_port_duplex_t  arg_duplex, duplex;
    int                arg_gap;
    int arg_stretch;
    int stretch;
    cmd_result_t       retCode;
    int                real_ifg;
    int                rv;
    int                i;
    bcm_port_config_t  pcfg;
    bcm_port_t         port, dport;

    const char *header = "        "
                         "    10HD"
                         "    10FD"
                         "   100HD"
                         "   100FD"
                         "  1000HD"
                         "  1000FD"
                         "  2500HD"
                         "  2500FD"
                         " 10000FD"
                         " STRETCH";


    const int speeds[] = {10, 100, 1000, 2500, 10000};
    const int num_speeds = sizeof(speeds) / sizeof(int);

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    if (bcm_port_config_get(unit, &pcfg) != BCM_E_NONE) {
        printk("%s: Error: bcm ports not initialized\n", ARG_CMD(a));
        return CMD_FAIL;
    }

    /*
     * Assign the defaults
     */
    BCM_PBMP_ASSIGN(arg_pbmp, pcfg.port);
    arg_speed  = 0;
    arg_duplex = BCM_PORT_DUPLEX_COUNT; 
    arg_gap    = 0;
    arg_stretch = -1;

    /*
     * Parse the arguments
     */
    if (ARG_CNT(a)) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "PortBitMap", PQ_DFL | PQ_PBMP | PQ_BCM,
                        0, &arg_pbmp, NULL);
        parse_table_add(&pt, "SPeed", PQ_DFL | PQ_INT,
                        0, &arg_speed, NULL);
        parse_table_add(&pt, "FullDuplex", PQ_DFL | PQ_BOOL,
                        0, &arg_duplex, NULL);
        parse_table_add(&pt, "Gap", PQ_DFL | PQ_INT,
                        0, &arg_gap, NULL);
        parse_table_add(&pt, "STretch", PQ_DFL | PQ_INT,
                        0, &arg_stretch, NULL);

        if (!parseEndOk(a, &pt, &retCode)) {
            return retCode;
        }
    }

    printk("%s\n", header);
    /*
     * Display IPG settings for all the specified ports
     */
    DPORT_BCM_PBMP_ITER(unit, arg_pbmp, dport, port) {
        printk("%-8.8s", BCM_PORT_NAME(unit, port));
        for (i = 0; i < num_speeds; i++) {
            speed = speeds[i];
            
            for (duplex = BCM_PORT_DUPLEX_HALF; 
                 duplex < BCM_PORT_DUPLEX_COUNT;
                 duplex++) {
                /*
                 * Skip the illegal 10000HD combination
                 */
                if (speed == 10000 && duplex == BCM_PORT_DUPLEX_HALF) {
                    continue;
                }
                
                /*
                 * Skip an entry if the speed has been explicitly specified
                 */
                if (arg_speed != 0 && speed != arg_speed) {
                    printk("%8.8s", " ");
                    continue;
                }
            
                /*
                 * Skip an entry if duplex has been explicitly specified
                 * and the entry doesn't match
                 */
                if (arg_duplex != BCM_PORT_DUPLEX_COUNT &&
                    arg_duplex != duplex) {
                    printk("%8.8s", " ");
                    continue;
                }
                
                if (arg_gap != 0) {
                    rv = bcm_port_ifg_set(unit, port, speed, duplex, arg_gap);
                }
                
                rv = bcm_port_ifg_get(unit, port, speed, duplex, &real_ifg);
                
                if (rv == BCM_E_NONE) {
                    printk("%8d", real_ifg);
                } else {
                    printk("%8.8s", "n/a");
                }
            }
        }
        if (arg_stretch >= 0) { 
              rv = bcm_port_control_set(unit, port, 
                    bcmPortControlFrameSpacingStretch, arg_stretch); 
        } 
        rv = bcm_port_control_get(unit, port,  bcmPortControlFrameSpacingStretch, &stretch); 
  
        if (rv == BCM_E_NONE) { 
            printk("%8d", stretch); 
        } else { 
            printk("%8.8s", "n/a"); 
        } 
        printk("\n");
    }

    return(CMD_OK);
}

cmd_result_t
if_esw_dtag(int unit, args_t *a)
{
    char		*subcmd, *c;
    bcm_port_config_t   pcfg;
    bcm_pbmp_t		pbmp;
    bcm_port_t		port, dport;
    int			mode, r;
    uint16		tpid;
    int                 dt_mode_mask;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (bcm_port_config_get(unit, &pcfg) != BCM_E_NONE) {
        printk("%s: Error: bcm ports not initialized\n", ARG_CMD(a));
        return CMD_FAIL;
    }

    dt_mode_mask = BCM_PORT_DTAG_MODE_INTERNAL |
                   BCM_PORT_DTAG_MODE_EXTERNAL;

    if ((subcmd = ARG_GET(a)) == NULL) {
	subcmd = "show";
    }

    c = ARG_GET(a);
    if (c == NULL) {
	BCM_PBMP_ASSIGN(pbmp, pcfg.e);
    } else {
	if (parse_bcm_pbmp(unit, c, &pbmp) < 0) {
	    printk("%s: ERROR: unrecognized port bitmap: %s\n",
		   ARG_CMD(a), c);
	    return CMD_FAIL;
	}
    }

    r = 0;
    if (sal_strcasecmp(subcmd, "show") == 0) {
        char *mode_flag; 

        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
	    r = bcm_port_dtag_mode_get(unit, port, &mode);
	    if (r < 0) {
		goto bcm_err;
	    }
	    r = bcm_port_tpid_get(unit, port, &tpid);
	    if (r < 0) {
		goto bcm_err;
	    }
	    switch (mode & dt_mode_mask) {
	    case BCM_PORT_DTAG_MODE_NONE:
		c = "none (disabled)";
		break;
	    case BCM_PORT_DTAG_MODE_INTERNAL:
		c = "internal (service provider)";
		break;
	    case BCM_PORT_DTAG_MODE_EXTERNAL:
		c = "external (customer)";
		break;
	    default:
		c = "unknown";
		break;
	    }
 
            mode_flag = "";
            if (mode & BCM_PORT_DTAG_REMOVE_EXTERNAL_TAG) {
                mode_flag = " remove customer tag";
            } else if (mode & BCM_PORT_DTAG_ADD_EXTERNAL_TAG) {
                mode_flag = " add customer tag";
            }

	    printk("port %d:%s\tdouble tag mode %s%s, tpid 0x%x\n",
		   unit, BCM_PORT_NAME(unit, port), c, mode_flag, tpid);
	}
	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "mode") == 0) {
	c = ARG_GET(a);
	if (c == NULL) {
            char *mode_flag;
            DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {

		r = bcm_port_dtag_mode_get(unit, port, &mode);
		if (r < 0) {
		    goto bcm_err;
		}
		switch (mode & dt_mode_mask) {
		case BCM_PORT_DTAG_MODE_NONE:
		    c = "none (disabled)";
		    break;
		case BCM_PORT_DTAG_MODE_INTERNAL:
		    c = "internal (service provider)";
		    break;
		case BCM_PORT_DTAG_MODE_EXTERNAL:
		    c = "external (customer)";
		    break;
		default:
		    c = "unknown";
		    break;
		}
              
                mode_flag = "";
                if (mode & BCM_PORT_DTAG_REMOVE_EXTERNAL_TAG) {
                    mode_flag = " remove customer tag";
                } else if (mode & BCM_PORT_DTAG_ADD_EXTERNAL_TAG) {
                    mode_flag = " add customer tag";
                }

		printk("port %d:%s\tdouble tag mode %s%s\n",
		       unit, BCM_PORT_NAME(unit, port), c, mode_flag);
	    }
	    return CMD_OK;
	}
	if (sal_strcasecmp(c, "none") == 0) {
	    mode = BCM_PORT_DTAG_MODE_NONE;
	} else if (sal_strcasecmp(c, "internal") == 0) {
	    mode = BCM_PORT_DTAG_MODE_INTERNAL;
	} else if (sal_strcasecmp(c, "external") == 0) {
	    mode = BCM_PORT_DTAG_MODE_EXTERNAL;
	} else {
	    return CMD_USAGE;
	}
        c = ARG_GET(a);
        if (c != NULL) {
            if (sal_strcasecmp(c, "addInnerTag") == 0) {
                mode |= BCM_PORT_DTAG_ADD_EXTERNAL_TAG;
            } else if (sal_strcasecmp(c, "removeInnerTag") == 0) {
                mode |= BCM_PORT_DTAG_REMOVE_EXTERNAL_TAG;
            } else {
                return CMD_OK;
            }
        }
        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
	    r = bcm_port_dtag_mode_set(unit, port, mode);
	    if (r < 0) {
		goto bcm_err;
	    }
	}
	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "tpid") == 0) {
	c = ARG_GET(a);
	if (c == NULL) {
            DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
		r = bcm_port_tpid_get(unit, port, &tpid);
		if (r < 0) {
		    goto bcm_err;
		}
		printk("port %d:%s\ttpid 0x%x\n",
		       unit, BCM_PORT_NAME(unit, port), tpid);
	    }
	} else {
	    tpid = parse_integer(c);
            DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
		r = bcm_port_tpid_set(unit, port, tpid);
		if (r < 0) {
		    goto bcm_err;
		}
	    }
	}
	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "addTpid") == 0) {
	c = ARG_GET(a);
	if (c == NULL) {
            DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
		r = bcm_port_tpid_get(unit, port, &tpid);
		if (r < 0) {
		    goto bcm_err;
		}
		printk("port %d:%s\ttpid 0x%x\n",
		       unit, BCM_PORT_NAME(unit, port), tpid);
	    }
	} else {
	    tpid = parse_integer(c);
            DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
		r = bcm_port_tpid_add(unit, port, tpid, BCM_COLOR_PRIORITY);
		if (r < 0) {
		    goto bcm_err;
		}
	    }
	}
	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "deleteTpid") == 0) {
	c = ARG_GET(a);
	if (c == NULL) {
            DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
		r = bcm_port_tpid_get(unit, port, &tpid);
		if (r < 0) {
		    goto bcm_err;
		}
		printk("port %d:%s\ttpid 0x%x\n",
		       unit, BCM_PORT_NAME(unit, port), tpid);
	    }
	} else {
            if (sal_strcasecmp(c, "all") == 0) {
            DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
                    r = bcm_port_tpid_delete_all(unit, port);
                    if (r < 0) {
                        goto bcm_err;
                    } 
                } 
            } else {
	        tpid = parse_integer(c);
                DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
		    r = bcm_port_tpid_delete(unit, port, tpid);
		    if (r < 0) {
		        goto bcm_err;
		    }
                }
	    }
	}
	return CMD_OK;
    }

    return CMD_USAGE;

 bcm_err:
    printk("%s: ERROR: %s\n", ARG_CMD(a), bcm_errmsg(r));
    return CMD_FAIL;
}


cmd_result_t
if_esw_linkscan(int unit, args_t *a)
{
    parse_table_t	pt;
    char		*c;
    int			us, rv;
    pbmp_t      pbm_sw, pbm_hw, pbm_none, pbm_force;
    pbmp_t	pbm_temp;
    pbmp_t	pbm_sw_pre,pbm_hw_pre,pbm_none_pre;
    bcm_port_config_t   pcfg;
    soc_port_t		port, dport;
    char		pfmt[SOC_PBMP_FMT_LEN];

    
    /*
     * Workaround that allows "linkscan off" at the beginning of rc.soc
     */

    if (ARG_CNT(a) == 1 && sal_strcasecmp(_ARG_CUR(a), "off") == 0) {
	rv = bcm_init_check(unit);
	if (rv == BCM_E_UNIT) {
	    (void) ARG_GET(a);
	    return(CMD_OK);
	}
    }

    if (bcm_port_config_get(unit, &pcfg) != BCM_E_NONE) {
        printk("%s: Error: bcm ports not initialized\n", ARG_CMD(a));
        return CMD_FAIL;
    }

    /*
     * First get current linkscan state.  (us == 0 if disabled).
     */

    if ((rv = bcm_linkscan_enable_get(unit, &us)) < 0) {
	printk("%s: Error: Failed to recover enable status: %s\n",
	       ARG_CMD(a), bcm_errmsg(rv));
	return(CMD_FAIL);
    }

    BCM_PBMP_CLEAR(pbm_sw);
    BCM_PBMP_CLEAR(pbm_hw);
    BCM_PBMP_CLEAR(pbm_force);

    DPORT_BCM_PBMP_ITER(unit, pcfg.port, dport, port) {
	int		mode;

	if ((rv = bcm_linkscan_mode_get(unit, port, &mode)) < 0) {
        /* Ignore requeue ports for linkscan */
        if (!IS_REQ_PORT(unit, port)) {
            printk("%s: Error: Could not get linkscan state for port %s\n",
                   ARG_CMD(a), BCM_PORT_NAME(unit, port));
        }
	} else {
	    switch (mode) {
	    case BCM_LINKSCAN_MODE_SW:
		BCM_PBMP_PORT_ADD(pbm_sw, port);
		break;
	    case BCM_LINKSCAN_MODE_HW:
		BCM_PBMP_PORT_ADD(pbm_hw, port);
		break;
	    default:
		break;
	    }
	}
    }
    BCM_PBMP_ASSIGN(pbm_sw_pre, pbm_sw);
    BCM_PBMP_ASSIGN(pbm_hw_pre, pbm_hw);
    BCM_PBMP_REMOVE(pbm_sw_pre, PBMP_HG_SUBPORT_ALL(unit));
    BCM_PBMP_REMOVE(pbm_hw_pre, PBMP_HG_SUBPORT_ALL(unit));

    /*
     * If there are no arguments, just display the status.
     */

    if (ARG_CNT(a) == 0) {
	char		buf[FORMAT_PBMP_MAX];
	pbmp_t		pbm;

	if (us) {
	    printk("%s: Linkscan enabled\n", ARG_CMD(a));
	    printk("%s:   Software polling interval: %d usec\n",
		   ARG_CMD(a), us);
	    format_bcm_pbmp(unit, buf, sizeof (buf), pbm_sw);
	    printk("%s:   Software Port BitMap %s: %s\n",
		   ARG_CMD(a), SOC_PBMP_FMT(pbm_sw, pfmt), buf);
	    format_bcm_pbmp(unit, buf, sizeof (buf), pbm_hw);
	    printk("%s:   Hardware Port BitMap %s: %s\n",
		   ARG_CMD(a), SOC_PBMP_FMT(pbm_hw, pfmt), buf);
	    BCM_PBMP_ASSIGN(pbm_temp, pbm_sw);
	    BCM_PBMP_OR(pbm_temp, pbm_hw);
	    BCM_PBMP_REMOVE(pbm_temp, PBMP_HG_SUBPORT_ALL(unit));
	    BCM_PBMP_ASSIGN(pbm, pcfg.port);
	    BCM_PBMP_XOR(pbm, pbm_temp);
	    format_bcm_pbmp(unit, buf, sizeof (buf), pbm);
	    printk("%s:   Disabled Port BitMap %s: %s\n",
		   ARG_CMD(a), SOC_PBMP_FMT(pbm, pfmt), buf);
	} else {
	    printk("%s: Linkscan disabled\n", ARG_CMD(a));
	}

	return(CMD_OK);
    }

    us = soc_property_get(unit, spn_BCM_LINKSCAN_INTERVAL, 250000);

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "SwPortBitMap", PQ_PBMP|PQ_DFL|PQ_BCM, 0, &pbm_sw, 0);
    parse_table_add(&pt, "HwPortBitMap", PQ_PBMP|PQ_DFL|PQ_BCM, 0, &pbm_hw, 0);
    parse_table_add(&pt, "Force", PQ_PBMP|PQ_DFL|PQ_BCM, 0, &pbm_force, 0);
    parse_table_add(&pt, "Interval", PQ_INT|PQ_DFL, 0, &us, 0);

    if (parse_arg_eq(a, &pt) < 0) {
	printk("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
	parse_arg_eq_done(&pt);
	return(CMD_FAIL);
    }
    parse_arg_eq_done(&pt);

    /*
     * Handle backward compatibility, allowing a raw interval to be
     * specified directly on the command line, as well as "on" or "off".
     */

    if (ARG_CUR(a) != NULL) {
	c = ARG_GET(a);

	if (!sal_strcasecmp(c, "off") ||
	    !sal_strcasecmp(c, "disable") ||
	    !sal_strcasecmp(c, "no")) {
	    us = 0;
	} else if (!sal_strcasecmp(c, "on") ||
		   !sal_strcasecmp(c, "enable") ||
		   !sal_strcasecmp(c, "yes")) {
	    us = soc_property_get(unit, spn_BCM_LINKSCAN_INTERVAL, 250000);
	} else if (isint(c)) {
	    us = parse_integer(c);
	} else {
	    return(CMD_USAGE);
	}
    }

    if (us == 0) {
	/* Turn off linkscan */

	if ((rv = bcm_linkscan_enable_set(unit, 0)) < 0) {
	    printk("%s: Error: Failed to disable linkscan: %s\n",
		   ARG_CMD(a), bcm_errmsg(rv));
	    return(CMD_FAIL);
	}

	return(CMD_OK);
    }

    BCM_PBMP_AND(pbm_sw, pcfg.port);
    BCM_PBMP_AND(pbm_hw, pcfg.port);
    BCM_PBMP_ASSIGN(pbm_none, pcfg.port);
    BCM_PBMP_REMOVE(pbm_sw, PBMP_HG_SUBPORT_ALL(unit));
    BCM_PBMP_REMOVE(pbm_hw, PBMP_HG_SUBPORT_ALL(unit));
    BCM_PBMP_REMOVE(pbm_none, PBMP_HG_SUBPORT_ALL(unit));

    BCM_PBMP_ASSIGN(pbm_temp, pbm_sw);
    BCM_PBMP_OR(pbm_temp, pbm_hw);
    BCM_PBMP_XOR(pbm_none, pbm_temp);
    BCM_PBMP_AND(pbm_force, pcfg.port);

    BCM_PBMP_ASSIGN(pbm_temp, pbm_sw);
    BCM_PBMP_AND(pbm_temp, pbm_hw);
    if (BCM_PBMP_NOT_NULL(pbm_temp)) {
	printk("%s: Error: Same port can't use both "
	       "software and hardware linkscan\n",
	       ARG_CMD(a));
	return(CMD_FAIL);
    }

    if ((rv = bcm_linkscan_mode_set_pbm(unit, pbm_sw,
					BCM_LINKSCAN_MODE_SW)) < 0) {
	printk("%s: Failed to set software link scanning: PBM=%s: %s\n",
	       ARG_CMD(a), SOC_PBMP_FMT(pbm_sw, pfmt), bcm_errmsg(rv));
	return(CMD_FAIL);
    }


    if ((rv = bcm_linkscan_mode_set_pbm(unit, pbm_hw,
					BCM_LINKSCAN_MODE_HW)) < 0) {
	printk("%s: Failed to set hardware link scanning: PBM=%s: %s\n",
	       ARG_CMD(a), SOC_PBMP_FMT(pbm_hw, pfmt), bcm_errmsg(rv));
	return(CMD_FAIL);
    }

    /* Only set the port to mode_none state if it is not previously in this mode */
    BCM_PBMP_ASSIGN(pbm_none_pre, pcfg.port);
    BCM_PBMP_ASSIGN(pbm_temp, pbm_sw_pre);
    BCM_PBMP_OR(pbm_temp, pbm_hw_pre);
    BCM_PBMP_XOR(pbm_none_pre, pbm_temp);
    BCM_PBMP_XOR(pbm_none_pre, pbm_none); 
    BCM_PBMP_AND(pbm_none, pbm_none_pre);  /* the one changed from 0 to 1 */
    
    if ((rv = bcm_linkscan_mode_set_pbm(unit, pbm_none,   
                                          BCM_LINKSCAN_MODE_NONE)) < 0) {   
          printk("%s: Failed to disable link scanning: PBM=%s: %s\n",   
                 ARG_CMD(a), SOC_PBMP_FMT(pbm_none, pfmt), bcm_errmsg(rv));   
          return(CMD_FAIL);   
      }   
   
    if ((rv = bcm_linkscan_enable_set(unit, us)) < 0) {
	printk("%s: Error: Failed to enable linkscan: %s\n",
	       ARG_CMD(a), bcm_errmsg(rv));
	return(CMD_FAIL);
    }

    if ((rv = bcm_link_change(unit, pbm_force)) < 0) {
	printk("%s: Failed to force link scan: PBM=%s: %s\n",
	       ARG_CMD(a), SOC_PBMP_FMT(pbm_force, pfmt), bcm_errmsg(rv));
	return(CMD_FAIL);
    }

    return(CMD_OK);
}

#define DUMP_PHY_COLS	4
#define PHY_UPDATE(_flags, _control) ((_flags) & (1 << (_control))) 
#define IS_LONGREACH(_type) (((_type)>=BCM_PORT_PHY_CONTROL_LONGREACH_SPEED) && \
                            ((_type)<=BCM_PORT_PHY_CONTROL_LONGREACH_ENABLE)) 

cmd_result_t
port_phy_control_update(int u, bcm_port_t p, bcm_port_phy_control_t type,
                        uint32 val, uint32 flags, int *print_header)
{
    int    rv;
    uint32 oval; 
    char buffer[100];

    oval = 0;
    rv = bcm_port_phy_control_get(u, p, type, &oval);
    if (BCM_FAILURE(rv) && BCM_E_UNAVAIL != rv) {
        printk("%s\n", bcm_errmsg(rv));
        return CMD_FAIL;
    } else if (BCM_SUCCESS(rv)) {
        if ((val != oval) && PHY_UPDATE(flags, type)) {
            if (BCM_FAILURE(bcm_port_phy_control_set(u, p, type, val))) {
                printk("%s\n", bcm_errmsg(rv));
                return CMD_FAIL;
            }
            oval = val;
        }
        if (*print_header) { 
            printk("Current PHY control settings of %s ->\n", 
                    BCM_PORT_NAME(u, p)); 
            *print_header = FALSE;
        }
        if (IS_LONGREACH(type)) {
            switch (type) {
            case BCM_PORT_PHY_CONTROL_LONGREACH_SPEED:
            case BCM_PORT_PHY_CONTROL_LONGREACH_PAIRS:
            case BCM_PORT_PHY_CONTROL_LONGREACH_GAIN:
                sal_sprintf(buffer, "%d", oval);
                break;
            case BCM_PORT_PHY_CONTROL_LONGREACH_LOCAL_ABILITY:
            case BCM_PORT_PHY_CONTROL_LONGREACH_REMOTE_ABILITY:
            case BCM_PORT_PHY_CONTROL_LONGREACH_CURRENT_ABILITY:
                format_phy_control_longreach_ability(buffer, sizeof(buffer), 
                    (soc_phy_control_longreach_ability_t)oval);
                break;
            case BCM_PORT_PHY_CONTROL_LONGREACH_AUTONEG:
            case BCM_PORT_PHY_CONTROL_LONGREACH_MASTER:
            case BCM_PORT_PHY_CONTROL_LONGREACH_ACTIVE:
            case BCM_PORT_PHY_CONTROL_LONGREACH_ENABLE:
                sal_sprintf(buffer, "%s", (oval==1) ? "True" : "False");
                break;
            /* coverity[dead_error_begin] */
            default:
                buffer[0] = 0;
                break;

            }
            printk("%s = %s\n", phy_control[type], buffer);
        } else
        if ( type == BCM_PORT_PHY_CONTROL_LOOPBACK_EXTERNAL ) {
            printk("        ENable = %s\n", (oval==1) ? "True" : "False");
        } else  
        if ( type == BCM_PORT_PHY_CONTROL_CLOCK_ENABLE ) {
            printk("Extraction to clock out (PR)         - %s\n", (oval==1) ? "Enabled" : "Disabled");
        } else
        if ( type == BCM_PORT_PHY_CONTROL_CLOCK_SECONDARY_ENABLE ) {
            printk("Extraction to clock (SE)condary out  - %s\n", (oval==1) ? "Enabled" : "Disabled");
        } else
        if ( type == BCM_PORT_PHY_CONTROL_CLOCK_FREQUENCY ) {
            printk("Extraction / Input (FR)equency       - %d KHz\n", oval);
        } else
        if ( type == BCM_PORT_PHY_CONTROL_PORT_PRIMARY ) {
            printk("(BA)se port of chip                  - %d\n", oval);
        } else
        if ( type == BCM_PORT_PHY_CONTROL_PORT_OFFSET ) {
            printk("Port (OF)fset within the chip        - %d\n", oval);
        } else
        {
            printk("%s = 0x%0x\n", phy_control[type], oval);
        }
    }
    return CMD_OK;
}

/* definition for phy low power control command */

typedef struct {
    bcm_pbmp_t pbm;
    int registered;
} phy_power_ctrl_t; 

static phy_power_ctrl_t phy_pctrl_desc[SOC_MAX_NUM_DEVICES];

STATIC void
_phy_power_linkscan_cb (int unit, soc_port_t port, bcm_port_info_t *info)
{
    int found = FALSE;
    soc_port_t p, dport;
    phy_power_ctrl_t *pDesc;
    bcm_port_cable_diag_t cds;
    int rv;

    pDesc = &phy_pctrl_desc[unit];

    DPORT_BCM_PBMP_ITER(unit, pDesc->pbm, dport, p) {
        if (p == port) {
            found = TRUE;
            break;
        }
    }
   
    if (found == TRUE) {
        if (info->linkstatus) {
            /* link down->up transition */

            /* Check if giga link */ 
            if (info->speed != 1000) {
                return;
            }

            /* Run cable diag if giga link*/
            sal_memset(&cds, 0, sizeof(bcm_port_cable_diag_t));
            rv = bcm_port_cable_diag(unit, p, &cds);
            if (SOC_FAILURE(rv)) {
                return;
            }

            if (cds.pair_len[0] >= 0 && cds.pair_len[0] < 10) {
                /* Enable low power mode */
                (void)bcm_port_phy_control_set(unit,p,
                                           BCM_PORT_PHY_CONTROL_POWER,
                                           BCM_PORT_PHY_CONTROL_POWER_LOW);
            }
        } else {
            /* link up->down transition */
            /* disable low-power mode */
            (void)bcm_port_phy_control_set(unit,p,
                                           BCM_PORT_PHY_CONTROL_POWER,
                                           BCM_PORT_PHY_CONTROL_POWER_FULL);

        } 
    }
}

STATIC int
_phy_auto_low_start (int unit,bcm_pbmp_t pbm,int enable)
{
    soc_port_t p, dport;
    phy_power_ctrl_t *pDesc;

    pDesc = &phy_pctrl_desc[unit];

    if (enable) {
        if (!pDesc->registered) {
            pDesc->pbm = pbm;
            (void)bcm_linkscan_register(unit, _phy_power_linkscan_cb);
            pDesc->registered = TRUE;
        }   
    } else {
        if (pDesc->registered) {
            (void)bcm_linkscan_unregister(unit, _phy_power_linkscan_cb);
            pDesc->registered = FALSE;
            DPORT_BCM_PBMP_ITER(unit, pbm, dport, p) {
                (void)bcm_port_phy_control_set(unit,p,
                                          BCM_PORT_PHY_CONTROL_POWER,
                                          BCM_PORT_PHY_CONTROL_POWER_FULL);
            }
        }
    }
    return SOC_E_NONE;
}


STATIC cmd_result_t
_phy_diag_phy_unit_get(int unit_value, int *phy_dev)
{
    *phy_dev = PHY_DIAG_DEV_DFLT;
    if (unit_value == 0) {   /* internal PHY */
        *phy_dev = PHY_DIAG_DEV_INT;
    } else if ((unit_value > 0) && (unit_value < 4)) {
        *phy_dev = PHY_DIAG_DEV_EXT;
    } else if (unit_value != -1) {
        printk("unit is numeric value: 0,1,2, ...\n");
        return CMD_FAIL;
    }
    return CMD_OK;
}

STATIC cmd_result_t
_phy_diag_phy_if_get(char *if_str, int *dev_if)
{
    *dev_if = PHY_DIAG_INTF_DFLT;
    if (if_str) {
        if (sal_strcasecmp(if_str, "sys") == 0) {
            *dev_if = PHY_DIAG_INTF_SYS;
        } else if (sal_strcasecmp(if_str, "line") == 0) {
            *dev_if = PHY_DIAG_INTF_LINE;
        } else if (if_str[0] != 0) {
            printk("InterFace must be sys or line.\n");
            return CMD_FAIL;
        }
    } else {
        printk("Invalid Interface string\n");
        return CMD_FAIL;
    }
    return CMD_OK;
}

STATIC cmd_result_t 
_phy_diag_loopback(int unit, bcm_pbmp_t pbmp,args_t *args) 
{
    parse_table_t       pt;
    bcm_port_t          port, dport;
    int                 rv;
    char   *if_str = NULL, *mode_str = NULL;
    int    phy_unit = 0, phy_unit_value = -1, phy_unit_if;     
    int    mode = 0;
    uint32 inst;

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "unit", PQ_DFL|PQ_INT, (void *)(0), 
                    &phy_unit_value, NULL);
    parse_table_add(&pt, "InterFace", PQ_STRING, 0, &if_str, NULL);
    parse_table_add(&pt, "mode", PQ_STRING, 0, &mode_str, NULL);
    
    if (parse_arg_eq(args, &pt) < 0) {
        printk("Error: invalid option: %s\n", ARG_CUR(args));
        parse_arg_eq_done(&pt);
        return CMD_USAGE;
    }
  
    rv = _phy_diag_phy_if_get(if_str, &phy_unit_if);
    if (rv == CMD_OK) {
        rv = _phy_diag_phy_unit_get(phy_unit_value, &phy_unit);
    }
 
    if (mode_str) {
        if (sal_strcasecmp(mode_str, "remote") == 0) {
            mode = 1;
        } else if (sal_strcasecmp(mode_str, "local") == 0) {
            mode = 1; /* for now only remote loopback */
        } else if (sal_strcasecmp(mode_str, "none") == 0) {
            mode = 0; /* no loopback */
        } else {
            printk("valid modes: remote,local and none\n");
            rv = CMD_FAIL;
        }
    }    
    /* Now free allocated strings */
    parse_arg_eq_done(&pt);

    if (rv != CMD_OK) {
        return rv;
    }

    inst = PHY_DIAG_INSTANCE(phy_unit,phy_unit_if,PHY_DIAG_LN_DFLT);
        
    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
        rv = soc_phyctrl_diag_ctrl(unit, port,inst,
                   PHY_DIAG_CTRL_SET,                   
                   SOC_PHY_CONTROL_LOOPBACK_REMOTE,
                             mode? (void *)TRUE:  (void *)FALSE);                        
        if (rv != BCM_E_NONE) {                                 
            printk("Setting prbs polynomial failed: %s\n",      
                    bcm_errmsg(rv));                             
            return CMD_FAIL;                                    
        }
    } 
    return CMD_OK;                                    
}

STATIC cmd_result_t
_phy_diag_dsc(int unit, bcm_pbmp_t pbmp, args_t *args)
{
    parse_table_t       pt;
    bcm_port_t          port, dport;
    int                 rv;
    char               *if_str;
    int					phy_unit, phy_unit_value = 0, phy_unit_if;     
    uint32				inst;

	parse_table_init(unit, &pt);

	/*
	 * unit:  phy_unit_value: phy chain number
	 * if  :  if_str="sys"|"line"
	 */
	parse_table_add(&pt, "unit", PQ_DFL|PQ_INT, (void *)(0), 
					&phy_unit_value, NULL);
	parse_table_add(&pt, "if", PQ_STRING, 0, &if_str, NULL);
	if (parse_arg_eq(args, &pt) < 0) {
		printk("Error: invalid option: %s\n", ARG_CUR(args));
		parse_arg_eq_done(&pt);
		return CMD_USAGE;
	}
  
    rv = _phy_diag_phy_if_get(if_str, &phy_unit_if);
    if (rv == CMD_OK) {
        rv = _phy_diag_phy_unit_get(phy_unit_value, &phy_unit);
    }
 
    /* Now free allocated strings */
    parse_arg_eq_done(&pt);

    if (rv != CMD_OK) {
        return rv;
    }

    inst = PHY_DIAG_INSTANCE(phy_unit, phy_unit_if, PHY_DIAG_LN_DFLT);
        
    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
		rv = soc_phyctrl_diag_ctrl(unit, port, inst, PHY_DIAG_CTRL_CMD, 
								   PHY_DIAG_CTRL_DSC, (void *)0);
		if (rv != BCM_E_NONE) {                                     
			return CMD_FAIL;                                        
		}                                                           
    }
    return CMD_OK;
}



STATIC cmd_result_t
_phy_diag_prbs(int unit, bcm_pbmp_t pbmp,args_t *args)
{
    parse_table_t       pt;
    bcm_port_t          port, dport;
    int                 rv, cmd, enable;
    char               *cmd_str, *if_str;
    int                 poly = 0, invert = 0;
    int    phy_unit, phy_unit_value = -1,phy_unit_if;     
    uint32 inst;

    enum { _PHY_PRBS_SET_CMD, _PHY_PRBS_GET_CMD, _PHY_PRBS_CLEAR_CMD };
    enum { _PHY_PRBS_SI_MODE, _PHY_PRBS_HC_MODE };

    if ((cmd_str = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    if (sal_strcasecmp(cmd_str, "set") == 0) {
        cmd = _PHY_PRBS_SET_CMD;
        enable = 1;
    } else if (sal_strcasecmp(cmd_str, "get") == 0) {
        cmd = _PHY_PRBS_GET_CMD;
        enable = 0;
    } else if (sal_strcasecmp(cmd_str, "clear") == 0) {
        cmd = _PHY_PRBS_CLEAR_CMD;
        enable = 0;
    } else return CMD_USAGE;

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "unit", PQ_DFL|PQ_INT, (void *)(0), 
                    &phy_unit_value, NULL);
    parse_table_add(&pt, "if", PQ_STRING, 0, &if_str, NULL);
    if (cmd == _PHY_PRBS_SET_CMD) {
        parse_table_add(&pt, "Polynomial", PQ_DFL|PQ_INT,
                        (void *)(0), &poly, NULL);
        parse_table_add(&pt, "Invert", PQ_DFL|PQ_INT,
                        (void *)(0), &invert, NULL);
    }
    if (parse_arg_eq(args, &pt) < 0) {
        printk("Error: invalid option: %s\n", ARG_CUR(args));
        parse_arg_eq_done(&pt);
        return CMD_USAGE;
    }
  
    rv = _phy_diag_phy_if_get(if_str, &phy_unit_if);
    if (rv == CMD_OK) {
        rv = _phy_diag_phy_unit_get(phy_unit_value, &phy_unit);
    }
 
    /* Now free allocated strings */
    parse_arg_eq_done(&pt);

    if (rv != CMD_OK) {
        return rv;
    }

    inst = PHY_DIAG_INSTANCE(phy_unit,phy_unit_if,PHY_DIAG_LN_DFLT);
        
    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
        if (cmd == _PHY_PRBS_SET_CMD || cmd == _PHY_PRBS_CLEAR_CMD) {
            if (poly >= 0 && poly <= 7) {                               
                /* Set polynomial */                                    
                rv = soc_phyctrl_diag_ctrl(unit, port,inst,PHY_DIAG_CTRL_SET,                   
                                          SOC_PHY_CONTROL_PRBS_POLYNOMIAL,
                                          INT_TO_PTR(poly));                        
                if (rv != BCM_E_NONE) {                                 
                    printk("Setting prbs polynomial failed: %s\n",      
                           bcm_errmsg(rv));                             
                    return CMD_FAIL;                                    
                }                                                       
            } else {
                printk("Polynomial must be 0..7.\n");                   
                return CMD_FAIL;                                        
            }                                                           
                
            /* Set invert */                                    
            rv = soc_phyctrl_diag_ctrl(unit, port,inst,PHY_DIAG_CTRL_SET,                   
                                      SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA,
                                      INT_TO_PTR(invert));                        
            if (rv != BCM_E_NONE) {                                 
                printk("Setting prbs invertion failed: %s\n",      
                       bcm_errmsg(rv));                             
                return CMD_FAIL;                                    
            }                                                       
                                                          
            rv = soc_phyctrl_diag_ctrl(unit, port, inst,PHY_DIAG_CTRL_SET,                      
                                      SOC_PHY_CONTROL_PRBS_TX_ENABLE,       
                                      INT_TO_PTR(enable));                            
            if (rv != BCM_E_NONE) {                                     
                printk("Setting prbs tx enable failed: %s\n",           
                       bcm_errmsg(rv));                                 
                return CMD_FAIL;                                        
            }                                                           

            rv = soc_phyctrl_diag_ctrl(unit, port, inst,PHY_DIAG_CTRL_SET,                      
                                      SOC_PHY_CONTROL_PRBS_RX_ENABLE,       
                                      INT_TO_PTR(enable));                            
            if (rv != BCM_E_NONE) {                                     
                printk("Setting prbs rx enable failed: %s\n",           
                       bcm_errmsg(rv));                                 
                return CMD_FAIL;                                        
            } 
        } else { /* _PHY_PRBS_GET_CMD */
            int status;                                                 
                
            rv = soc_phyctrl_diag_ctrl(unit, port,inst,PHY_DIAG_CTRL_GET,                       
                                      SOC_PHY_CONTROL_PRBS_RX_STATUS,       
                                      (void *)&status);                         
            if (rv != BCM_E_NONE) {                                     
                printk("Getting prbs rx status failed: %s\n",           
                       bcm_errmsg(rv));     
                return CMD_FAIL;                                        
            }                             
                
            switch (status) {
            case 0:
                printk("%s (%2d):  PRBS OK!\n", BCM_PORT_NAME(unit, port), port);
                break;
            case -1:
                printk("%s (%2d):  PRBS Failed!\n", BCM_PORT_NAME(unit, port), port);
                break;
            default:
                printk("%s (%2d):  PRBS has %d errors!\n", BCM_PORT_NAME(unit, port), 
                       port, status);
                break;
            }
        }
    }
    return CMD_OK;
}

STATIC cmd_result_t 
_phy_diag_mfg(int unit, bcm_pbmp_t pbmp, args_t *args) 
{
#ifndef  NO_FILEIO
    parse_table_t       pt;
    bcm_port_t          port, dport;
    int                 rv, i, data_len = 0;
    char                *file_name = NULL, *buffer = NULL, *buf_ptr = NULL, *dptr;
    int                 test, test_cmd = 0, num_ports, data;
    FILE                *ofp = NULL;

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "Test",       PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &test, 0);
    parse_table_add(&pt, "Data",       PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &data, 0);
    parse_table_add(&pt, "File", PQ_STRING, 0, &file_name, NULL);
    
    if (parse_arg_eq(args, &pt) < 0) {
        printk("Error: invalid option: %s\n", ARG_CUR(args));
        parse_arg_eq_done(&pt);
        return CMD_USAGE;
    }

    switch (test) {
    case 1:
        test_cmd = PHY_DIAG_CTRL_MFG_HYB_CANC;
        data_len = 770*4;
        break;
    case 2:
        test_cmd = PHY_DIAG_CTRL_MFG_DENC;
        data_len = 44*4;
        break;
    case 3:
        test_cmd = PHY_DIAG_CTRL_MFG_TX_ON;
        break;
    case 0:
        test_cmd = PHY_DIAG_CTRL_MFG_EXIT;
        break;
    default:
        printk("Test should be : 1 (HYB_CANC), 2 (DENC), 3 (TX_ON) or 0 (EXIT)\n");
        return CMD_FAIL;                                    
        break;
    }
        
    if (data_len) {
        if ((ofp = sal_fopen(file_name, "w")) == NULL) {
            printk("ERROR: Can't open the file : %s\n", file_name);
            return CMD_FAIL;                                    
        }
        switch (test) {
        case 1:
            sal_fprintf(ofp, "PHY_DIAG_CTRL_MFG_HYB_CANC\n");
            break;
        case 2:
            sal_fprintf(ofp, "PHY_DIAG_CTRL_MFG_DENC\n");
            break;
        }
    }

    /* Now free allocated strings */
    parse_arg_eq_done(&pt);
  
    num_ports = 0;
    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
        rv = soc_phyctrl_diag_ctrl(unit, port, 0,
                   PHY_DIAG_CTRL_SET, test_cmd, INT_TO_PTR(data));
        if (rv != SOC_E_NONE) {                                 
            printk("Error: PHY_DIAG_CTRL_SET u=%d p=%d test_cmd=%d\n", unit, port, test_cmd);
        }
        num_ports++;
    } 

    /* every port has an extra 32 bit scratch pad */
    buffer = sal_alloc(num_ports * (data_len + 32), "mfg_test_results");
    if (buffer == NULL) {
        printk("Insufficient memory.\n");
        if (ofp) {
            sal_fclose(ofp);
        }
        return CMD_FAIL;
    }
    buf_ptr = buffer;

    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
        buf_ptr[0] = 0;
        rv = soc_phyctrl_diag_ctrl(unit, port, 0,
                   PHY_DIAG_CTRL_GET, test_cmd, (void *)(buf_ptr + 32));
        if (rv != SOC_E_NONE) {                                 
            printk("Error: PHY_DIAG_CTRL_GET u=%d p=%d test_cmd=%d\n", unit, port, test_cmd);
        } else {
            buf_ptr[0] = -1; /* indicates that the test result is valid */
        }
        buf_ptr += (data_len + 32);
    } 

    if (data_len) {
        buf_ptr = buffer;
        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
            i = 0;
            dptr = buf_ptr + 32;
            if (buf_ptr[0]) {
                sal_fprintf(ofp, "\n\nOutput data for port %s\n", BCM_PORT_NAME(unit, port));
                while ( i < data_len) {
                    if ((i & 0x1f) == 0) { /* i % 32 */
                        sal_fprintf(ofp, "\n");
                    }
                    /* data in ARM memory is big endian (network byte order) */
                    sal_fprintf(ofp, "0x%08x", soc_ntohl_load(dptr) );
                    dptr += 4;
                    i += 4;
                    if (i >= data_len) {
                        sal_fprintf(ofp, "\n");
                        break;
                    } else {
                        sal_fprintf(ofp, ", ");
                    }
                }
            } else {
                sal_fprintf(ofp, "\n\nTest failed for port %s\n", BCM_PORT_NAME(unit, port));
            }
            buf_ptr += (data_len + 32);
        }
    }

    if (ofp) {
        sal_fclose(ofp);
    }
    sal_free(buffer);

    return CMD_OK;                                    
#else
    printk("This command is not supported without file I/O\n");
    return CMD_FAIL;
#endif
}

STATIC cmd_result_t 
_phy_diag_state(int unit, bcm_pbmp_t pbmp, args_t *args) 
{
#ifndef  NO_FILEIO
    parse_table_t       pt;
    bcm_port_t          port, dport;
    int                 rv, i, data_len = 0;
    char                *file_name = NULL, *buffer = NULL, *buf_ptr = NULL, *dptr;
    XGPHY_DIAG_DATA_t   *diag_dptr;
    int                 test, test_cmd = 0, num_ports, data;
    FILE                *ofp = NULL;

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "Test",       PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &test, 0);
    parse_table_add(&pt, "Data",       PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &data, 0);
    parse_table_add(&pt, "File", PQ_STRING, 0, &file_name, NULL);
    
    if (parse_arg_eq(args, &pt) < 0) {
        printk("Error: invalid option: %s\n", ARG_CUR(args));
        parse_arg_eq_done(&pt);
        return CMD_USAGE;
    }

    switch (test) {
    case 1:
        test_cmd = PHY_DIAG_CTRL_STATE_TRACE1;
        data_len = 2048*4;
        break;
    case 2:
        test_cmd = PHY_DIAG_CTRL_STATE_TRACE2;
        data_len = 1024*4;
        break;
    case 3:
        test_cmd = PHY_DIAG_CTRL_STATE_WHEREAMI;
        data_len = sizeof(XGPHY_DIAG_DATA_t);
        break;
    case 4:
        test_cmd = PHY_DIAG_CTRL_STATE_TEMP;
        data_len = sizeof(XGPHY_DIAG_DATA_t);
        break;
    default:
        printk("Test should be : 1 (STATE_TRACE1), 2 (STATE_TRACE2), 3 (WHERE_AM_I), 4 (TEMP)\n");
        return CMD_FAIL;                                    
        break;
    }
        
    if (data_len) {
        if ((ofp = sal_fopen(file_name, "a+")) == NULL) {
            printk("ERROR: Can't open the file : %s\n", file_name);
            return CMD_FAIL;                                    
        }
        sal_fprintf(ofp, "\n------------------------------------------------------------"
                     "-------------------------------------------------\n");
        switch (test) {
        case 1:
            sal_fprintf(ofp, "PHY_DIAG_CTRL_STATE_TRACE1\n");
            break;
        case 2:
            sal_fprintf(ofp, "PHY_DIAG_CTRL_STATE_TRACE2\n");
            break;
        case 3:
            sal_fprintf(ofp, "PHY_DIAG_CTRL_STATE_WHERE_AM_I\n");
            break;
        case 4:
            sal_fprintf(ofp, "PHY_DIAG_CTRL_STATE_TEMP\n");
            break;
        }
    }

    /* Now free allocated strings */
    parse_arg_eq_done(&pt);
  
    num_ports = 0;
    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
        rv = soc_phyctrl_diag_ctrl(unit, port, 0,
                   PHY_DIAG_CTRL_SET, test_cmd, INT_TO_PTR(data));
        if (rv != SOC_E_NONE) {                                 
            printk("Error: PHY_DIAG_CTRL_SET u=%d p=%d test_cmd=%d\n", unit, port, test_cmd);
        }
        num_ports++;
    } 

    /* every port has an extra 32 bit scratch pad */
    buffer = sal_alloc(num_ports * (data_len + 32), "state_test_results");
    if (buffer == NULL) {
        printk("Insufficient memory.\n");
        if (ofp) {
            sal_fclose(ofp);
        }
        return CMD_FAIL;
    }
    buf_ptr = buffer;

    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
        buf_ptr[0] = 0;
        rv = soc_phyctrl_diag_ctrl(unit, port, 0,
                   PHY_DIAG_CTRL_GET, test_cmd, (void *)(buf_ptr + 32));
        if (rv != SOC_E_NONE) {                                 
            printk("Error: PHY_DIAG_CTRL_GET u=%d p=%d test_cmd=%d\n", unit, port, test_cmd);
        } else {
            buf_ptr[0] = -1; /* indicates that the test result is valid */
        }
        buf_ptr += (data_len + 32);
    } 

    if (data_len) {
        buf_ptr = buffer;
        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
            i = 0;
            dptr = buf_ptr + 32;
            diag_dptr = (XGPHY_DIAG_DATA_t *) dptr;

            if (buf_ptr[0]) {
                sal_fprintf(ofp, "\n\nOutput data for port %s\n", BCM_PORT_NAME(unit, port));
                if (test_cmd == PHY_DIAG_CTRL_STATE_WHEREAMI) {
                    int32 valA, valB, valC, valD, statA, statB, statC, statD;

                    if (diag_dptr->flags & PHY_DIAG_FULL_SNR_SUPPORT) {
                        valA = soc_letohl_load(&diag_dptr->snr_block[16]) - 32768; 
                        valB = soc_letohl_load(&diag_dptr->snr_block[20]) - 32768; 
                        valC = soc_letohl_load(&diag_dptr->snr_block[24]) - 32768; 
                        valD = soc_letohl_load(&diag_dptr->snr_block[28]) - 32768; 

                        statA = soc_letohl_load(&diag_dptr->snr_block[0]); 
                        statB = soc_letohl_load(&diag_dptr->snr_block[4]); 
                        statC = soc_letohl_load(&diag_dptr->snr_block[8]); 
                        statD = soc_letohl_load(&diag_dptr->snr_block[12]); 

                        sal_fprintf(ofp, "\nsnrA = %d.%d (%s) snrB = %d.%d (%s) "
                            "snrC = %d.%d (%s) snrD = %d.%d (%s) "
                            "serr = %d cerr = %d block_lock = %d block_point_id = %d\n",
                             valA / 10, valA % 10, statA ? "OK":"Not OK",
                             valB / 10, valB % 10, statB ? "OK":"Not OK",
                             valC / 10, valC % 10, statC ? "OK":"Not OK",
                             valD / 10, valD % 10, statD ? "OK":"Not OK",
                             diag_dptr->serr, diag_dptr->cerr,
                             diag_dptr->block_lock, diag_dptr->block_point_id);

                    } else {
                        valA = soc_letohl_load(&diag_dptr->snr_block[0]); 
                        valB = soc_letohl_load(&diag_dptr->snr_block[4]); 
                        valC = soc_letohl_load(&diag_dptr->snr_block[8]); 
                        valD = soc_letohl_load(&diag_dptr->snr_block[12]); 

                        sal_fprintf(ofp, "\nmseA = %d mseB = %d mseC = %d mseD = %d "
                            "serr = %d cerr = %d block_lock = 0x%x block_point_id = 0x%x\n",
                             valA, valB, valC, valD, diag_dptr->serr, diag_dptr->cerr,
                             diag_dptr->block_lock, diag_dptr->block_point_id);
                    }
                } else if (test_cmd == PHY_DIAG_CTRL_STATE_TEMP) {
                    sal_fprintf(ofp, "\nTemperature = %d C,  %d F\n",
                    diag_dptr->digital_temp, (diag_dptr->digital_temp * 9)/5 + 32);
                } else {
                    while ( i < data_len) {
                        if ((i & 0x1f) == 0) { /* i % 32 */
                            sal_fprintf(ofp, "\n");
                        }
                        sal_fprintf(ofp, "0x%08x", soc_letohl_load(&dptr[0]));
                        dptr += 4;
                        i += 4;
                        if (i >= data_len) {
                            sal_fprintf(ofp, "\n");
                            break;
                        } else {
                            sal_fprintf(ofp, ", ");
                        }
                    }
                }
            } else {
                sal_fprintf(ofp, "\n\nTest failed for port %s\n", BCM_PORT_NAME(unit, port));
            }
            buf_ptr += (data_len + 32);
        }
    }

    if (ofp) {
        sal_fclose(ofp);
    }
    sal_free(buffer);

    return CMD_OK;                                    
#else
    printk("This command is not supported without file I/O\n");
    return CMD_FAIL;
#endif
}

cmd_result_t 
_phy_diag_eyescan(int unit, bcm_pbmp_t pbmp,args_t *args) 
{
	int i, speed, port_count;
    cmd_result_t ret = CMD_OK;
    soc_port_phy_eyescan_params_t params;
    soc_port_phy_eyescan_results_t *results;
	int phy_unit, phy_unit_if, phy_unit_value = 0;
    parse_table_t       pt;
    int                 rv, flags = 0;
    uint32 nof_ports;
	uint32 inst;
	char *if_str;
    soc_port_t *ports, port, dport;
    int *local_lane_num, lane_num = 0xff;
    char *eyescan_counters_strs[socPortPhyEyescanNofCounters*2 + 1];
    char numbers_strs[socPortPhyEyescanNofCounters][3];
    

    for( i= 0 ; i < socPortPhyEyescanNofCounters ; i++){
		sal_sprintf(numbers_strs[i], "%d", i);
		eyescan_counters_strs[i] = eyescan_counter[i];
		eyescan_counters_strs[socPortPhyEyescanNofCounters + i] = numbers_strs[i];
    }
    eyescan_counters_strs[socPortPhyEyescanNofCounters*2] = NULL;


    BCM_PBMP_COUNT(pbmp, port_count);
    if (port_count == 0) {
        return CMD_OK;
    }

    
    results= sal_alloc((port_count) * sizeof(soc_port_phy_eyescan_results_t), "eyescan results array");  /*array of result per port*/
    ports = sal_alloc((port_count) * sizeof(soc_port_t), "eyescan ports"); 
    local_lane_num = sal_alloc((port_count) * sizeof(int), "eyescan lane_num array"); 
    if (results == NULL || ports == NULL || local_lane_num == NULL) {
        printk("ERROR, in phy_diag_eyescan: failed to allocate results");
        return CMD_FAIL;
    }


    /*default values*/
    params.sample_time = 1000;
    params.sample_resolution = 1;
    params.bounds.horizontal_max = 31;
    params.bounds.horizontal_min = -31;
    params.bounds.vertical_max = 31;
    params.bounds.vertical_min = -31;
    params.counter =socPortPhyEyescanCounterRelativePhy;
    params.error_threshold = 20;
    params.time_upper_bound = 256000;

    sal_memset(results, 0, sizeof(soc_port_phy_eyescan_results_t)*port_count);

    nof_ports = 0;
    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
        ports[nof_ports] = port;
        nof_ports++;
    }

    params.nof_threshold_links = nof_ports/2;
    if (params.nof_threshold_links < 1) {
        params.nof_threshold_links = 1;
    }

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "vertical_max", PQ_DFL|PQ_INT, (void *)(0), 
                        &(params.bounds.vertical_max), NULL);
    parse_table_add(&pt, "vertical_min", PQ_DFL|PQ_INT, (void *)(0), 
                        &(params.bounds.vertical_min), NULL);
    parse_table_add(&pt, "horizontal_max", PQ_DFL|PQ_INT, (void *)(0), 
                        &(params.bounds.horizontal_max), NULL);
    parse_table_add(&pt, "horizontal_min", PQ_DFL|PQ_INT, (void *)(0), 
                    &(params.bounds.horizontal_min), NULL);
    parse_table_add(&pt, "sample_resolution", PQ_DFL|PQ_INT, (void *)(0), 
                    &(params.sample_resolution), NULL);
    parse_table_add(&pt, "sample_time", PQ_DFL|PQ_INT, (void *)(0), 
                    &(params.sample_time), NULL);
    parse_table_add(&pt, "counter", PQ_MULTI | PQ_DFL | PQ_NO_EQ_OPT, (void *)(0), 
                    &(params.counter), &eyescan_counters_strs);
    parse_table_add(&pt, "flags", PQ_DFL|PQ_INT, (void *)(0), 
                    &flags, NULL);
    parse_table_add(&pt, "lane", PQ_DFL|PQ_INT, (void *)(0), 
                    &lane_num, NULL);
    parse_table_add(&pt, "error_threshold", PQ_DFL|PQ_INT, (void *)(0), 
                    &(params.error_threshold), NULL);
    parse_table_add(&pt, "time_upper_bound", PQ_DFL|PQ_INT, (void *)(0), 
                    &(params.time_upper_bound), NULL);
    parse_table_add(&pt, "nof_threshold_links", PQ_DFL|PQ_INT, (void *)(0), 
                    &(params.nof_threshold_links), NULL);
    parse_table_add(&pt, "unit", PQ_DFL|PQ_INT, (void *)(0),
                    &phy_unit_value, NULL);
    parse_table_add(&pt, "if", PQ_STRING, 0, &if_str, NULL);

    if (parse_arg_eq(args, &pt) < 0) {
        printk("ERROR: could not parse parameters\n");
        parse_arg_eq_done(&pt);
        ret = CMD_FAIL;
        goto exit;
    }

    if (ARG_CNT(args) > 0) {
        printk("%s: Unknown argument %s\n", ARG_CMD(args), ARG_CUR(args));
        parse_arg_eq_done(&pt);
        ret = CMD_FAIL;
        goto exit;
    }
	params.counter = params.counter % socPortPhyEyescanNofCounters;

    phy_unit = 0;
    rv = _phy_diag_phy_if_get(if_str, &phy_unit_if);
    if (rv == CMD_OK) {
        rv = _phy_diag_phy_unit_get(phy_unit_value, &phy_unit);
    }

    /* inst has unit, intf, lane */
    inst = PHY_DIAG_INSTANCE(phy_unit, phy_unit_if, 
                             lane_num == 0xff ? PHY_DIAG_LN_DFLT : lane_num);

    for (i = 0; i < nof_ports; i++) {
        results[i].ext_done = 0;
    }

    if (lane_num == 0xff) {
        local_lane_num = NULL;
    } else {
        if (lane_num > 9) {
            printk("ERROR, in phy_diag_eyescan: lane num is wrong \n");
            return CMD_FAIL;
        } else {
            /*now check the correct lane num input for each port */
            for(i=0 ; i<nof_ports; i++) {
                phy_ctrl_t *pc;
                int start_lane, end_lane;               
                pc = INT_PHY_SW_STATE(unit, ports[i]);
                 start_lane = pc->lane_num;
                if (pc->phy_mode == PHYCTRL_ONE_LANE_PORT) {
                    end_lane = start_lane;
                } else if (pc->phy_mode == PHYCTRL_DUAL_LANE_PORT) {
                    end_lane = start_lane + 1;
                } else {
                    end_lane = 9;
                }
                if ((lane_num < start_lane) || (lane_num > end_lane)) {
                    printk("ERROR, in phy_diag_eyescan: lane num is wrong \n");
                    return CMD_FAIL;
                }
            }
            sal_memset(local_lane_num, lane_num, sizeof(int)*port_count);
        }
    }

     if (params.sample_time > params.time_upper_bound) {
            printk("ERROR, in phy_diag_eyescan: sample time %d greater than upper bound time %d\n", params.sample_time, params.time_upper_bound);
            return CMD_FAIL;
    } 

    rv = soc_port_phy_eyescan_run(unit, inst, flags, &params, nof_ports, ports, local_lane_num, results);
    rv = soc_port_phy_eyescan_extrapolate(unit, flags, &params, nof_ports, ports, results);
    
    if (rv != SOC_E_NONE) {
        ret = CMD_FAIL;
        goto exit;
    }

    for(i=0 ; i<nof_ports; i++) {
        
        rv = bcm_port_speed_get(unit, ports[i], &speed);
        if (rv != SOC_E_NONE) {
            ret = CMD_FAIL;
            goto exit;
        }

        printk("Eye\\Cross-section Results For Port %d (with rate %d):\n",ports[i],speed);
 
        rv = soc_port_phy_eyescan_res_print(unit, params.sample_resolution, &(params.bounds), &(results[i]));
              
        if (rv != SOC_E_NONE) {
            ret = CMD_FAIL;
            goto exit;
        }
    }
    ret = CMD_OK; 
      
exit:
    if(results != NULL) {
        sal_free(results);
    }
    if(ports != NULL) {
        sal_free(ports);
    }
    return ret;
}


/* 
 * Diagnostic utilities for serdes and PHY devices.
 *
 * Command format used in BCM diag shell:
 * phy diag <pbm> <sub_cmd> [sub cmd parameters]
 * All sub commands take two general parameters: unit and if. This identifies
 * the instance the command targets to.
 * unit = 0,1, ....  
 *   unit takes numeric values identifying the instance of the PHY devices 
 *   associated with the given port. A value 0 indicates the internal 
 *   PHY(serdes) the one directly connected to the MAC. A value 1 indicates
 *   the first external PHY.
 * if(interface) = [sys | line] 
 *   interface identifies the system side interface or line side interface of
 *   PHY device.
 * The list of sub commands:
 *   dsc - display tx/rx equalization information. Warpcore(WC) only.
 *   veye - vertical eye margin mesurement. WC only. All eye margin functions
 *          are used in conjunction with PRBS utility in the configed speed mode
 *   heye_r - right horizontal eye margin mesurement. WC only 
 *   heye_l - right horizontal eye margin mesurement. WC only 
 *   for the WarpLite, there are three additional parameters, live link or not, par1 for the target BER value for example: -18 stands for 10^(-18)
 *   par2 will be percentage range for example, 5 means checxk for +-%5 of the target BER
 *   loopback - put the device in the given loopback mode 
 *              parameter: mode=[remote | local | none]
 *   prbs - perform various PRBS functions. Takes all parameters of the
 *          "phy prbs" command except the mode parameter.
 *          Example:
 *            A port has a WC serdes and 84740 PHY connected. A typical usage
 *            is to use the PRBS to check the link between WC and the system
 *            side of the 84740. Use port xe0 as an example:
 *            BCM.0> phy diag xe0 prbs set unit=0 p=3
 *            BCM.0> phy diag xe0 prbs set unit=1 if=sys p=3
 *            BCM.0> phy diag xe0 prbs get unit=1 if=sys
 *            BCM.0> phy diag xe0 prbs get unit=0
 *      
 *   mfg - run manufacturing test on PHY BCM8483X
 *              parameter: (t)est=num (d)ata=<val> (f)ile=filename
 *              Test should be : 
 *              1 (HYB_CANC), needs (f)ile=filename 
 *              2 (DENC),     needs (f)ile=filename 
 *              3 (TX_ON)     needs (d)ata = bit map for turning TX off on pairs DCBA
 *              0 (EXIT)
 */
STATIC cmd_result_t
_phy_diag(int unit, args_t *args)
{
    bcm_port_t          port, dport;
    bcm_pbmp_t          pbmp;
    int                 rv, cmd;
    int                 par[3];
    int                 *pData;
    int                 lscan_time;
    char               *cmd_str, *port_str;
    parse_table_t       pt;

    rv = CMD_OK;
    par[0] = 0 ; par[1] = 0 ; par[2] = 0 ; 
    pData = &par[0];
    if ((port_str = ARG_GET(args)) == NULL) {
	return CMD_USAGE;
    }

    BCM_PBMP_CLEAR(pbmp);
    if (parse_bcm_pbmp(unit, port_str, &pbmp) < 0) {
        printk("Error: unrecognized port bitmap: %s\n", port_str);
        return CMD_FAIL;
    }

   if ((cmd_str = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }

    /* linkscan should be disabled. soc_phyctrl_diag_ctrl() doesn't
     * assume exclusive access to the device.
     */
    BCM_IF_ERROR_RETURN
        (bcm_linkscan_enable_get(unit, &lscan_time));
    if (lscan_time != 0) {
        BCM_IF_ERROR_RETURN(bcm_linkscan_enable_set(unit, 0));
        /* Give enough time for linkscan task to exit. */
        sal_usleep(lscan_time * 2); 
    }
	if (sal_strcasecmp(cmd_str, "peek") == 0) {
        cmd = PHY_DIAG_CTRL_PEEK;
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "fb", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        parse_table_add(&pt, "lane", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        if (parse_arg_eq(args, &pt) < 0) {
            printk("Error: invalid option: %s\n", ARG_CUR(args));
            parse_arg_eq_done(&pt);
            return CMD_USAGE;
        }
        /* command targets to Internal serdes device for now*/
        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
            if (soc_phyctrl_diag_ctrl(unit, port,PHY_DIAG_INT,
                   PHY_DIAG_CTRL_CMD,cmd,&par) != SOC_E_NONE) {
                rv = CMD_FAIL;
            }
        }
    } else if (sal_strcasecmp(cmd_str, "poke") == 0) {
        cmd = PHY_DIAG_CTRL_POKE;
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "fb", PQ_DFL|PQ_INT, (void *)(0),
                        pData++, NULL);
        parse_table_add(&pt, "val", PQ_DFL|PQ_INT, (void *)(0),
                        pData++, NULL);
        if (parse_arg_eq(args, &pt) < 0) {
            printk("Error: invalid option: %s\n", ARG_CUR(args));
            parse_arg_eq_done(&pt);
            return CMD_USAGE;
        }
        /* command targets to Internal serdes device for now*/
        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
            if (soc_phyctrl_diag_ctrl(unit, port,PHY_DIAG_INT,
                   PHY_DIAG_CTRL_CMD,cmd,&par) != SOC_E_NONE) {
                rv = CMD_FAIL;
            }
        }
   } else if (sal_strcasecmp(cmd_str, "load_uc") == 0) {
      cmd = PHY_DIAG_CTRL_LOAD_UC;
      parse_table_init(unit, &pt);
      parse_table_add(&pt, "crc", PQ_DFL|PQ_INT, (void *)(0),
                      pData++, NULL);
      parse_table_add(&pt, "debug", PQ_DFL|PQ_INT, (void *)(0),
                      pData++, NULL);
      if (parse_arg_eq(args, &pt) < 0) {
            printk("Error: invalid option: %s\n", ARG_CUR(args));
            parse_arg_eq_done(&pt);
            return CMD_USAGE;
      }
      /* command targets to Internal serdes device for now*/
      DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
         if (soc_phyctrl_diag_ctrl(unit, port,PHY_DIAG_INT,
             PHY_DIAG_CTRL_CMD,cmd,&par) != SOC_E_NONE) {
            rv = CMD_FAIL;
         }
      }
   } else if (sal_strcasecmp(cmd_str, "veye") == 0) {
        cmd = PHY_DIAG_CTRL_EYE_MARGIN_VEYE;
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "live", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        parse_table_add(&pt, "BER", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        parse_table_add(&pt, "range", PQ_DFL|PQ_INT, (void *)(0), 
                        pData, NULL);
        if (parse_arg_eq(args, &pt) < 0) {
            printk("Error: invalid option: %s\n", ARG_CUR(args));
            parse_arg_eq_done(&pt);
            return CMD_USAGE;
        }
        /* command targets to Internal serdes device for now*/
        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
            if (soc_phyctrl_diag_ctrl(unit, port,PHY_DIAG_INT,
                   PHY_DIAG_CTRL_CMD,cmd,&par) != SOC_E_NONE) {
                rv = CMD_FAIL;
            }
        }
    } else if (sal_strcasecmp(cmd_str, "veye_u") == 0) {
        cmd = PHY_DIAG_CTRL_EYE_MARGIN_VEYE_UP;
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "live", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        parse_table_add(&pt, "BER", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        parse_table_add(&pt, "range", PQ_DFL|PQ_INT, (void *)(0), 
                        pData, NULL);
        if (parse_arg_eq(args, &pt) < 0) {
            printk("Error: invalid option: %s\n", ARG_CUR(args));
            parse_arg_eq_done(&pt);
            return CMD_USAGE;
        }
        /* command targets to Internal serdes device for now*/
        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
            if (soc_phyctrl_diag_ctrl(unit, port,PHY_DIAG_INT,
                   PHY_DIAG_CTRL_CMD,cmd,&par) != SOC_E_NONE) {
                rv = CMD_FAIL;
            }
        }
    } else if (sal_strcasecmp(cmd_str, "veye_d") == 0) {
        cmd = PHY_DIAG_CTRL_EYE_MARGIN_VEYE_DOWN;
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "live", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        parse_table_add(&pt, "BER", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        parse_table_add(&pt, "range", PQ_DFL|PQ_INT, (void *)(0), 
                        pData, NULL);
        if (parse_arg_eq(args, &pt) < 0) {
            printk("Error: invalid option: %s\n", ARG_CUR(args));
            parse_arg_eq_done(&pt);
            return CMD_USAGE;
        }
        /* command targets to Internal serdes device for now*/
        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
            if (soc_phyctrl_diag_ctrl(unit, port,PHY_DIAG_INT,
                   PHY_DIAG_CTRL_CMD,cmd,&par) != SOC_E_NONE) {
                rv = CMD_FAIL;
            }
        }
    } else if (sal_strcasecmp(cmd_str, "heye_r") == 0) {
        cmd = PHY_DIAG_CTRL_EYE_MARGIN_HEYE_RIGHT;
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "live", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        parse_table_add(&pt, "BER", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        parse_table_add(&pt, "range", PQ_DFL|PQ_INT, (void *)(0), 
                        pData, NULL);
        if (parse_arg_eq(args, &pt) < 0) {
            printk("Error: invalid option: %s\n", ARG_CUR(args));
            parse_arg_eq_done(&pt);
            return CMD_USAGE;
        }
        /* command targets to Internal serdes device for now*/
        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
            if (soc_phyctrl_diag_ctrl(unit, port,PHY_DIAG_INT,
                   PHY_DIAG_CTRL_CMD,cmd,&par) != SOC_E_NONE) {
                rv = CMD_FAIL;
            }
        }
    } else if (sal_strcasecmp(cmd_str, "heye_l") == 0) {
        cmd = PHY_DIAG_CTRL_EYE_MARGIN_HEYE_LEFT;
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "live", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        parse_table_add(&pt, "BER", PQ_DFL|PQ_INT, (void *)(0), 
                        pData++, NULL);
        parse_table_add(&pt, "range", PQ_DFL|PQ_INT, (void *)(0), 
                        pData, NULL);
        if (parse_arg_eq(args, &pt) < 0) {
            printk("Error: invalid option: %s\n", ARG_CUR(args));
            parse_arg_eq_done(&pt);
            return CMD_USAGE;
        }
        /* command targets to Internal serdes device for now*/
        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
            if (soc_phyctrl_diag_ctrl(unit, port,PHY_DIAG_INT,
                   PHY_DIAG_CTRL_CMD,cmd,&par) != SOC_E_NONE) {
                rv = CMD_FAIL;
            }
        }

	} else if (sal_strcasecmp(cmd_str, "dsc") == 0) {
		rv = _phy_diag_dsc(unit, pbmp, args);
    } else if (sal_strcasecmp(cmd_str, "eyescan") == 0) {
        rv = _phy_diag_eyescan(unit, pbmp, args);
    } else if (sal_strcasecmp(cmd_str, "LoopBack") == 0) {
        rv = _phy_diag_loopback(unit, pbmp, args);
    } else if (sal_strcasecmp(cmd_str, "prbs") == 0) {
        rv = _phy_diag_prbs(unit, pbmp, args);
    } else if (sal_strcasecmp(cmd_str, "mfg") == 0) {
        rv = _phy_diag_mfg(unit, pbmp, args);
    } else if (sal_strcasecmp(cmd_str, "state") == 0) {
        rv = _phy_diag_state(unit, pbmp, args);
    } else {
        rv = CMD_FAIL;
    }

    if (lscan_time != 0) {
        BCM_IF_ERROR_RETURN
            (bcm_linkscan_enable_set(unit, lscan_time));
    }

    return rv;
}

STATIC cmd_result_t
_phy_margin(int unit, args_t *args)
{
    parse_table_t       pt;
    bcm_port_t          port, dport;
    bcm_pbmp_t          pbmp;
    int                 rv, cmd, enable;
    char               *cmd_str, *port_str;
    int                 marginval = 0;

    enum { _PHY_MARGIN_MAX_GET_CMD, 
           _PHY_MARGIN_SET_CMD, 
           _PHY_MARGIN_VALUE_SET_CMD, 
           _PHY_MARGIN_VALUE_GET_CMD, 
           _PHY_MARGIN_CLEAR_CMD };

    if ((port_str = ARG_GET(args)) == NULL) {
	return CMD_USAGE;
    }

    BCM_PBMP_CLEAR(pbmp);
    if (parse_bcm_pbmp(unit, port_str, &pbmp) < 0) {
        printk("Error: unrecognized port bitmap: %s\n", port_str);
        return CMD_FAIL;
    }

    if ((cmd_str = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    if (sal_strcasecmp(cmd_str, "maxget") == 0) {
        cmd = _PHY_MARGIN_MAX_GET_CMD;
        enable = 0;
    } else if (sal_strcasecmp(cmd_str, "set") == 0) {
        cmd = _PHY_MARGIN_SET_CMD;
        enable = 1;
    } else if (sal_strcasecmp(cmd_str, "valueset") == 0) {
        cmd = _PHY_MARGIN_VALUE_SET_CMD;
        enable = 0;
    }else if (sal_strcasecmp(cmd_str, "valueget") == 0) {
        cmd = _PHY_MARGIN_VALUE_GET_CMD;
        enable = 0;
    } else if (sal_strcasecmp(cmd_str, "clear") == 0) {
        cmd = _PHY_MARGIN_CLEAR_CMD;
        enable = 0;
    } else return CMD_USAGE;

    parse_table_init(unit, &pt);
    if (cmd == _PHY_MARGIN_VALUE_SET_CMD) {
        parse_table_add(&pt, "marginval", PQ_DFL|PQ_INT,
                        (void *)(0), &marginval, NULL);
    }
    if (parse_arg_eq(args, &pt) < 0) {
        printk("Error: invalid option: %s\n", ARG_CUR(args));
        parse_arg_eq_done(&pt);
        return CMD_USAGE;
    }
        
    /* Now free allocated strings */
    parse_arg_eq_done(&pt);
        
    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
        
        switch (cmd) 
            {
            
            case _PHY_MARGIN_SET_CMD:
            case _PHY_MARGIN_CLEAR_CMD:
                
                rv = bcm_port_control_set(unit, port,                       
                                          bcmPortControlSerdesTuneMarginMode,       
                                          enable);                            
                if (rv != BCM_E_NONE) {                                     
                    printk("Setting margin enable failed: %s\n",           
                           bcm_errmsg(rv));                                 
                    return CMD_FAIL;                                        
                }                                                           
                
                break;

            case _PHY_MARGIN_VALUE_SET_CMD:
                
                rv = bcm_port_control_set(unit, port,                       
                                          bcmPortControlSerdesTuneMarginValue,       
                                          marginval);                         
                if (rv != BCM_E_NONE) {                                     
                    printk("Getting margin value failed: %s\n",           
                           bcm_errmsg(rv));     
                    return CMD_FAIL;                                        
                }
                printk("margin value(%d)\n", marginval);
                break;
            case _PHY_MARGIN_MAX_GET_CMD:
                
                rv = bcm_port_control_get(unit, port,                       
                                          bcmPortControlSerdesTuneMarginMax,       
                                          &marginval);                         
                if (rv != BCM_E_NONE) {                                     
                    printk("Getting margin max value failed: %s\n",           
                           bcm_errmsg(rv));     
                    return CMD_FAIL;                                        
                }
                printk("margin max value(%d)\n", marginval);
                break;

            case _PHY_MARGIN_VALUE_GET_CMD:
                
                rv = bcm_port_control_get(unit, port,                       
                                          bcmPortControlSerdesTuneMarginValue,       
                                          &marginval);                         
                if (rv != BCM_E_NONE) {                                     
                    printk("Getting margin value failed: %s\n",           
                           bcm_errmsg(rv));     
                    return CMD_FAIL;                                        
                }
                printk("margin value(%d)\n", marginval);
                break;

            default:
                break;
        }
    }
    
    return CMD_OK;
}

STATIC cmd_result_t
_phy_prbs(int unit, args_t *args)
{
    parse_table_t       pt;
    bcm_port_t          port, dport;
    bcm_pbmp_t          pbmp;
    int                 rv, cmd, enable, mode = 0;
    char               *cmd_str, *port_str, *mode_str;
    int                 poly = 0, lb = 0;

    enum { _PHY_PRBS_SET_CMD, _PHY_PRBS_GET_CMD, _PHY_PRBS_CLEAR_CMD };
    enum { _PHY_PRBS_SI_MODE, _PHY_PRBS_HC_MODE };

    if ((port_str = ARG_GET(args)) == NULL) {
	return CMD_USAGE;
    }

    BCM_PBMP_CLEAR(pbmp);
    if (parse_bcm_pbmp(unit, port_str, &pbmp) < 0) {
        printk("Error: unrecognized port bitmap: %s\n", port_str);
        return CMD_FAIL;
    }

    if ((cmd_str = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    if (sal_strcasecmp(cmd_str, "set") == 0) {
        cmd = _PHY_PRBS_SET_CMD;
        enable = 1;
    } else if (sal_strcasecmp(cmd_str, "get") == 0) {
        cmd = _PHY_PRBS_GET_CMD;
        enable = 0;
    } else if (sal_strcasecmp(cmd_str, "clear") == 0) {
        cmd = _PHY_PRBS_CLEAR_CMD;
        enable = 0;
    } else return CMD_USAGE;

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "Mode", PQ_STRING, 0, &mode_str, NULL);
    if (cmd == _PHY_PRBS_SET_CMD) {
        parse_table_add(&pt, "Polynomial", PQ_DFL|PQ_INT,
                        (void *)(0), &poly, NULL);
        parse_table_add(&pt, "LoopBack", PQ_DFL|PQ_BOOL,
                        (void *)(0), &lb, NULL);
    }
    if (parse_arg_eq(args, &pt) < 0) {
        printk("Error: invalid option: %s\n", ARG_CUR(args));
        parse_arg_eq_done(&pt);
        return CMD_USAGE;
    }
   
    if (mode_str) {
        if (sal_strcasecmp(mode_str, "si") == 0) {
            mode = 1;
        } else if (sal_strcasecmp(mode_str, "hc") == 0) {
            mode = 0;
        } else {
#ifdef BCM_QE2000_SUPPORT
	    /* There is only 1 mode on QE2000 */
	    if (!SOC_IS_SBX_QE2000(unit)) {
#endif   
		printk("Prbs mode must be si, mac, phy or hc.\n");
		return CMD_FAIL;
#ifdef BCM_QE2000_SUPPORT
	    }
#endif
        }
    }
        
    /* Now free allocated strings */
    parse_arg_eq_done(&pt);
        
    DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {

#ifdef BCM_QE2000_SUPPORT
      /* There is only 1 mode on QE2000 */
      if (!SOC_IS_SBX_QE2000(unit)) {
#endif
        /* First set prbs mode */
        rv = bcm_port_control_set(unit, port, bcmPortControlPrbsMode,
                                  mode);
        if (rv != BCM_E_NONE) {
            printk("Setting prbs mode failed: %s\n", bcm_errmsg(rv));
            return CMD_FAIL;
        }
      
#ifdef BCM_QE2000_SUPPORT
      }
#endif      
        if (cmd == _PHY_PRBS_SET_CMD || cmd == _PHY_PRBS_CLEAR_CMD) {
            if (poly >= 0 && poly <= 6) {                               
                /* Set polynomial */                                    
                rv = bcm_port_control_set(unit, port,                   
                                          bcmPortControlPrbsPolynomial,
                                          poly);                        
                if (rv != BCM_E_NONE) {                                 
                    printk("Setting prbs polynomial failed: %s\n",      
                           bcm_errmsg(rv));                             
                    return CMD_FAIL;                                    
                }                                                       
            } else {
                printk("Polynomial must be 0..6.\n");                   
                return CMD_FAIL;                                        
            }                                                           
                
            /* 
             * Set tx/rx enable. If clear, enable == 0
             * Note that the order of enabling is important. The following
             * steps are listed in the SI Block description in the uArch Spec.
             * - Disable the normal receive path at the receive end. This is done
             *     via SI_CONFIG0.enable;
             * - Enable the PRBS generator at the transmit end,
             *     SI_CONFIG0.prbs_generator_en;
             * - Allow enough time for the PRBS stream to be received at the
             *     monitor end
             * - Enable the PRBS monitor, SI_CONFIG0.prbs_monitor_en
             * - Read the PRBS error register to clear the error counter
             */                                      
            if (lb) {
                enable |= 0x8000;
            }
            rv = bcm_port_control_set(unit, port,                       
                                      bcmPortControlPrbsTxEnable,       
                                      enable);                            
            if (rv != BCM_E_NONE) {                                     
                printk("Setting prbs tx enable failed: %s\n",           
                       bcm_errmsg(rv));                                 
                return CMD_FAIL;                                        
            }                                                           

            rv = bcm_port_control_set(unit, port,                       
                                      bcmPortControlPrbsRxEnable,       
                                      enable);                            
            if (rv != BCM_E_NONE) {                                     
                printk("Setting prbs rx enable failed: %s\n",           
                       bcm_errmsg(rv));                                 
                return CMD_FAIL;                                        
            }                                                           
        } else { /* _PHY_PRBS_GET_CMD */
            int status;                                                 
            int i;
                
            for (i = 0; i <= 1; i++) {
                /* Read twice to clear errors */
                rv = bcm_port_control_get(unit, port,                       
                                          bcmPortControlPrbsRxStatus,       
                                          &status);                         
                if (rv != BCM_E_NONE) {                                     
                    printk("Getting prbs rx status failed: %s\n",           
                           bcm_errmsg(rv));     
                    return CMD_FAIL;                                        
                }                             
                sal_sleep(1);
            }
                
            switch (status) {
            case 0:
                printk("%s (%2d):  PRBS OK!\n", BCM_PORT_NAME(unit, port), port);
                break;
            case -1:
                printk("%s (%2d):  PRBS Failed!\n", BCM_PORT_NAME(unit, port), port);
                break;
            default:
                printk("%s (%2d):  PRBS has %d errors!\n", BCM_PORT_NAME(unit, port), 
                       port, status);
                break;
            }
        }
    }
    return CMD_OK;
}

void _print_timesync_egress_message_mode(char *message, bcm_port_phy_timesync_event_message_egress_mode_t mode) 
{

    soc_cm_print("%s (no,uc,rc,ct) - ", message);

    switch (mode) {
    case bcmPortPhyTimesyncEventMessageEgressModeNone:
        soc_cm_print("NOne\n");
        break;
    case bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField:
        soc_cm_print("Update_Correctionfield\n");
        break;
    case bcmPortPhyTimesyncEventMessageEgressModeReplaceCorrectionFieldOrigin:
        soc_cm_print("Replace_Correctionfield_origin\n");
        break;
    case bcmPortPhyTimesyncEventMessageEgressModeCaptureTimestamp:
        soc_cm_print("Capture_Timestamp\n");
        break;
    default:
        soc_cm_print("\n");
        break;
    }

}
void _print_inband_timesync_matching_criterion(uint32 flags) {
    uint8 delimit = 0;
    soc_cm_print("InBand TimeStamp MATCH (ip, mac, pnum, vlid) - ");

    if(flags & BCM_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_IP_ADDR) {
        soc_cm_print("%s",delimit?", ip" : "ip");
        delimit = 1;
    }
    if(flags & BCM_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_MAC_ADDR) {
        soc_cm_print("%s",delimit?", mac" : "mac");
        delimit = 1;
    }
    if(flags & BCM_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_SRC_PORT_NUM) {
        soc_cm_print("%s",delimit?", pnum" : "pnum");
        delimit = 1;
    }
    if(flags & BCM_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_VLAN_ID) {
        soc_cm_print("%s",delimit?", vlid" : "vlid");
    }
    soc_cm_print("\n");
}


bcm_port_phy_timesync_event_message_egress_mode_t _convert_timesync_egress_message_str(char *str, 
    bcm_port_phy_timesync_event_message_egress_mode_t def) 
{
    int i;
    struct s_array {
        char *s;
        bcm_port_phy_timesync_event_message_egress_mode_t value;
    } data[] = {
    {"no",bcmPortPhyTimesyncEventMessageEgressModeNone},
    {"uc",bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField},
    {"rc",bcmPortPhyTimesyncEventMessageEgressModeReplaceCorrectionFieldOrigin},
    {"ct",bcmPortPhyTimesyncEventMessageEgressModeCaptureTimestamp} };

    for (i = 0; i < (sizeof(data)/sizeof(data[0])); i++) {
        if (!sal_strncmp(str, data[i].s, 2)) {
            return data[i].value;
        }
    }
    return def;
}

void _print_timesync_ingress_message_mode(char *message, bcm_port_phy_timesync_event_message_ingress_mode_t mode)
{

    soc_cm_print("%s (no,uc,it,id) - ", message);

    switch (mode) {
    case bcmPortPhyTimesyncEventMessageIngressModeNone:
        soc_cm_print("NOne\n");
        break;
    case bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField:
        soc_cm_print("Update_Correctionfield\n");
        break;
    case bcmPortPhyTimesyncEventMessageIngressModeInsertTimestamp:
        soc_cm_print("Insert_Timestamp\n");
        break;
    case bcmPortPhyTimesyncEventMessageIngressModeInsertDelaytime:
        soc_cm_print("Insert_Delaytime\n");
        break;
    default:
        soc_cm_print("\n");
        break;
    }

}

bcm_port_phy_timesync_event_message_ingress_mode_t _convert_timesync_ingress_message_str(char *str, 
    bcm_port_phy_timesync_event_message_ingress_mode_t def) 
{
    int i;
    struct s_array {
        char *s;
        bcm_port_phy_timesync_event_message_ingress_mode_t value;
    } data[] = {
    {"no",bcmPortPhyTimesyncEventMessageIngressModeNone},
    {"uc",bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField},
    {"it",bcmPortPhyTimesyncEventMessageIngressModeInsertTimestamp},
    {"id",bcmPortPhyTimesyncEventMessageIngressModeInsertDelaytime} };

    for (i = 0; i < (sizeof(data)/sizeof(data[0])); i++) {
        if (!sal_strncmp(str, data[i].s, 2)) {
            return data[i].value;
        }
    }
    return def;
}

void _print_timesync_gmode(char *message, bcm_port_phy_timesync_global_mode_t mode)
{

    soc_cm_print("%s (fr,si,cp) - ", message);

    switch (mode) {
    case bcmPortPhyTimesyncModeFree:
        soc_cm_print("FRee\n");
        break;
    case bcmPortPhyTimesyncModeSyncin:
        soc_cm_print("SyncIn\n");
        break;
    case bcmPortPhyTimesyncModeCpu:
        soc_cm_print("CPu\n");
        break;
    default:
        soc_cm_print("\n");
        break;
    }

}

bcm_port_phy_timesync_global_mode_t _convert_timesync_gmode_str(char *str, 
    bcm_port_phy_timesync_global_mode_t def) 
{
    int i;
    struct s_array {
        char *s;
        bcm_port_phy_timesync_global_mode_t value;
    } data[] = {
    {"fr",bcmPortPhyTimesyncModeFree},
    {"si",bcmPortPhyTimesyncModeSyncin},
    {"cp",bcmPortPhyTimesyncModeCpu} };

    for (i = 0; i < (sizeof(data)/sizeof(data[0])); i++) {
        if (!sal_strncmp(str, data[i].s, 2)) {
            return data[i].value;
        }
    }
    return def;
}

void _print_framesync_mode(char *message, bcm_port_phy_timesync_framesync_mode_t  mode)
{

    soc_cm_print("%s (fno,fs0,fs1,fss,fsc) - ", message);

    switch (mode) {
    case bcmPortPhyTimesyncFramesyncNone:
        soc_cm_print("FramesyncNOne\n");
        break;
    case bcmPortPhyTimesyncFramesyncSyncin0:
        soc_cm_print("FramesyncSyncIn0\n");
        break;
    case bcmPortPhyTimesyncFramesyncSyncin1:
        soc_cm_print("FramesyncSyncIn1\n");
        break;
    case bcmPortPhyTimesyncFramesyncSyncout:
        soc_cm_print("FrameSyncSyncout\n");
        break;
    case bcmPortPhyTimesyncFramesyncCpu:
        soc_cm_print("FrameSyncCpu\n");
        break;
    default:
        soc_cm_print("\n");
        break;
    }

}

bcm_port_phy_timesync_framesync_mode_t _convert_framesync_mode_str(char *str, 
    bcm_port_phy_timesync_framesync_mode_t def) 
{
    int i;
    struct s_array {
        char *s;
        bcm_port_phy_timesync_framesync_mode_t value;
    } data[] = {
    {"fno",bcmPortPhyTimesyncFramesyncNone},
    {"fs0",bcmPortPhyTimesyncFramesyncSyncin0},
    {"fs1",bcmPortPhyTimesyncFramesyncSyncin1},
    {"fss",bcmPortPhyTimesyncFramesyncSyncout},
    {"fsc",bcmPortPhyTimesyncFramesyncCpu} };

    for (i = 0; i < (sizeof(data)/sizeof(data[0])); i++) {
        if (!sal_strncmp(str, data[i].s, 3)) {
            return data[i].value;
        }
    }
    return def;
}

void _print_syncout_mode(char *message, bcm_port_phy_timesync_syncout_mode_t mode)
{

    soc_cm_print("%s (sod,sot,spt,sps) - ", message);

    switch (mode) {
    case bcmPortPhyTimesyncSyncoutDisable:
        soc_cm_print("SyncOutDisable\n");
        break;
    case bcmPortPhyTimesyncSyncoutOneTime:
        soc_cm_print("SyncoutOneTime\n");
        break;
    case bcmPortPhyTimesyncSyncoutPulseTrain:
        soc_cm_print("SyncoutPulseTrain\n");
        break;
    case bcmPortPhyTimesyncSyncoutPulseTrainWithSync:
        soc_cm_print("SyncoutPulsetrainSync\n");
        break;
    default:
        soc_cm_print("\n");
        break;
    }

}

bcm_port_phy_timesync_syncout_mode_t _convert_syncout_mode_str(char *str, 
    bcm_port_phy_timesync_syncout_mode_t def) 
{
    int i;
    struct s_array {
        char *s;
        bcm_port_phy_timesync_syncout_mode_t value;
    } data[] = {
    {"sod",bcmPortPhyTimesyncSyncoutDisable},
    {"sot",bcmPortPhyTimesyncSyncoutOneTime},
    {"spt",bcmPortPhyTimesyncSyncoutPulseTrain},
    {"sps",bcmPortPhyTimesyncSyncoutPulseTrainWithSync} };

    for (i = 0; i < (sizeof(data)/sizeof(data[0])); i++) {
        if (!sal_strncmp(str, data[i].s, 3)) {
            return data[i].value;
        }
    }
    return def;
}
void
_set_inband_timesync_matching_criterion(char *str, uint32 *inband_ts_ctrl_flags) {
 int i;
    struct s_array {
        char *s;
        uint32 flag;
    } data[] = {
    {"ip", SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_IP_ADDR},
    {"mac", SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_MAC_ADDR},
    {"pnum", SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_SRC_PORT_NUM},
    {"vlid", SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_VLAN_ID}};

    for (i = 0; i < (sizeof(data)/sizeof(data[0])); i++) {
        if (!sal_strcmp(str, data[i].s)) {
            *inband_ts_ctrl_flags |= data[i].flag;
        }
        if(!sal_strcmp(str, "none")) {
             *inband_ts_ctrl_flags &= 
                 ~( SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_IP_ADDR 
                         | SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_MAC_ADDR 
                         |  SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_SRC_PORT_NUM 
                         | SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_VLAN_ID);
        }
    }

}





void _print_timesync_config(bcm_port_phy_timesync_config_t *conf)
{

    printk("ENable (Y or N) - %s\n", conf->flags & BCM_PORT_PHY_TIMESYNC_ENABLE ? "Yes" : "No");

    printk("CaptureTS (Y or N) - %s\n", conf->flags & BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE ? "Yes" : "No");

    printk("HeartbeatTS (Y or N) - %s\n", conf->flags & BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE ? "Yes" : "No");

    printk("RxCrc (Y or N) - %s\n", conf->flags & BCM_PORT_PHY_TIMESYNC_RX_CRC_ENABLE ? "Yes" : "No");

    printk("AS (Y or N) - %s\n", conf->flags & BCM_PORT_PHY_TIMESYNC_8021AS_ENABLE ? "Yes" : "No");

    printk("L2 (Y or N) - %s\n", conf->flags & BCM_PORT_PHY_TIMESYNC_L2_ENABLE ? "Yes" : "No");

    printk("IP4 (Y or N) - %s\n", conf->flags & BCM_PORT_PHY_TIMESYNC_IP4_ENABLE ? "Yes" : "No");

    printk("IP6 (Y or N) - %s\n", conf->flags & BCM_PORT_PHY_TIMESYNC_IP6_ENABLE ? "Yes" : "No");

    printk("ExtClock (Y or N) - %s\n", conf->flags & BCM_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT ? "Yes" : "No");

    printk("ITpid = 0x%04x\n", conf->itpid);

    printk("OTpid = 0x%04x\n", conf->otpid);

    printk("OriginalTimecodeSeconds = 0x%08x%08x\n",
            COMPILER_64_HI(conf->original_timecode.seconds),
            COMPILER_64_LO(conf->original_timecode.seconds));

    printk("OriginalTimecodeNanoseconds = 0x%08x\n", conf->original_timecode.nanoseconds);

    _print_timesync_gmode("GMode", conf->gmode);

    _print_framesync_mode("FramesyncMode", conf->framesync.mode);

    _print_syncout_mode("SyncoutMode", conf->syncout.mode);

    printk("TxOffset = %d\n", conf->tx_timestamp_offset);

    printk("RxOffset = %d\n", conf->rx_timestamp_offset);

    _print_timesync_egress_message_mode("TxSync", conf->tx_sync_mode);
    _print_timesync_egress_message_mode("TxDelayReq", conf->tx_delay_request_mode);
    _print_timesync_egress_message_mode("TxPdelayReq", conf->tx_pdelay_request_mode);
    _print_timesync_egress_message_mode("TxPdelayreS", conf->tx_pdelay_response_mode);
                                            
    _print_timesync_ingress_message_mode("RxSync", conf->rx_sync_mode);
    _print_timesync_ingress_message_mode("RxDelayReq", conf->rx_delay_request_mode);
    _print_timesync_ingress_message_mode("RxPdelayReq", conf->rx_pdelay_request_mode);
    _print_timesync_ingress_message_mode("RxPdelayreS", conf->rx_pdelay_response_mode);
                                            
}
                   
void _print_inband_timesync_config(bcm_port_phy_timesync_config_t *conf) {
 printk("InBand TimeStamp Sync (Y or N) - %s\n", conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_SYNC_ENABLE ? "Yes" : "No");
 printk("InBand TimeStamp Delay Request (Y or N) - %s\n", conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_DELAY_RQ_ENABLE ? "Yes" : "No");
 printk("InBand TimeStamp Pdelay Request (Y or N) - %s\n", conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RQ_ENABLE ? "Yes" : "No");
 printk("InBand TimeStamp Pdelay reSponse (Y or N) - %s\n", conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RESP_ENABLE ? "Yes" : "No");
 printk("InBand TimeStamp ID - 0x%04x\n", conf->inband_ts_control.resv0_id);
 printk("InBand TimeStamp ID CHhecK (Y or N) - %s\n", conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_CHECK ? "Yes" : "No");
 printk("InBand TimeStamp ID UPDate (Y or N) - %s\n", conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_UPDATE ? "Yes" : "No");
_print_inband_timesync_matching_criterion(conf->inband_ts_control.flags);
}


void _print_heartbeat_ts(int unit, bcm_port_t port)
{
    int rv;
    uint64 time;

    rv = bcm_port_control_phy_timesync_get(unit, port, bcmPortControlPhyTimesyncHeartbeatTimestamp, &time);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_get failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    printk("Heartbeat TS = %08x%08x\n",
           COMPILER_64_HI(time), COMPILER_64_LO(time));
}

void _print_capture_ts(int unit, bcm_port_t port)
{
    int rv;
    uint64 time;

    rv = bcm_port_control_phy_timesync_get(unit, port, bcmPortControlPhyTimesyncCaptureTimestamp, &time);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_get failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    printk("Capture   TS = %08x%08x\n",
           COMPILER_64_HI(time), COMPILER_64_LO(time));
}

/*
 * Function: 	if_phy
 * Purpose:	Show/configure phy registers.
 * Parameters:	u - SOC unit #
 *		a - pointer to args
 * Returns:	CMD_OK/CMD_FAIL/
 */
cmd_result_t
if_esw_phy(int u, args_t *a)
{
    soc_pbmp_t pbm, pbm_phys, pbm_temp;
    soc_port_t p, dport;
    char *c, drv_name[64];
    uint16 phy_data, phy_reg, phy_devad = 0;
    uint16 phy_addr;
    int intermediate = 0;
    int is_c45 = 0;
#if defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT)
    int is_rcpu = 0;
#endif /* defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT) */
    int rv = CMD_OK;
    char pfmt[SOC_PBMP_FMT_LEN];
    int i;
    int p_devad[] = {PHY_C45_DEV_PMA_PMD,
                     PHY_C45_DEV_WIS,
                     PHY_C45_DEV_PCS,
                     PHY_C45_DEV_PHYXS,
                     PHY_C45_DEV_DTEXS,
                     PHY_C45_DEV_AN,
                     PHY_C45_DEV_USER};
    char *p_devad_str[] = {"DEV_PMA_PMD",
                           "DEV_WIS",
                           "DEV_PCS",
                           "DEV_PHYXS",
                           "DEV_DTEXS",
                           "DEV_AN",
                           "DEV_USER"};

    if (!sh_check_attached(ARG_CMD(a), u)) {
        return CMD_FAIL;
    }

    c = ARG_GET(a);

    if (c != NULL && sal_strcasecmp(c, "info") == 0) {
        SOC_PBMP_ASSIGN(pbm, PBMP_PORT_ALL(u));
        SOC_PBMP_REMOVE(pbm, PBMP_HG_SUBPORT_ALL(u));
        SOC_PBMP_REMOVE(pbm, PBMP_REQ_ALL(u));
        printk("Phy mapping dump:\n");
        printk("%10s %5s %5s %5s %5s %23s %10s\n",
               "port", "id0", "id1", "addr", "iaddr", "name", "timeout");
        DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
            if (phy_port_info[u] == NULL) {
                continue;
            }
            printk("%5s(%3d) %5x %5x %5x %5x %23s %10d\n",
                   SOC_PORT_NAME(u, p), p,
                   soc_phy_id0reg_get(u, p),
                   soc_phy_id1reg_get(u, p),
                   soc_phy_addr_of_port(u, p),
                   soc_phy_addr_int_of_port(u, p),
                   soc_phy_name_get(u, p),
                   soc_phy_an_timeout_get(u, p));
        }

        return CMD_OK;
    }

    if (c != NULL && sal_strcasecmp(c, "eee") == 0) {
        char *str, *latency_str;
        uint32 eee_mode_value, eee_auto_mode_value;
        uint32 latency, idle = 0, tx_events, tx_duration, rx_events, rx_duration;
        uint32 flags;
        int    i;
        parse_table_t    pt;
        char *mode_type=NULL, *lstr=NULL, *stats=NULL;
        int idle_th = -1;

        if (((c = ARG_GET(a)) == NULL) || (parse_bcm_pbmp(u, c, &pbm) < 0)) {
            printk("%s: ERROR: unrecognized port bitmap: %s\n", ARG_CMD(a), c);
            return CMD_FAIL;
        }

        if ((c = ARG_CUR(a)) != NULL) {

            parse_table_init(u, &pt);
            parse_table_add(&pt, "MOde", PQ_DFL | PQ_STRING,
                            0, &mode_type, 0);

            parse_table_add(&pt, "LAtency", PQ_DFL | PQ_STRING,
                            0, &lstr, 0);

            parse_table_add(&pt, "IDle_th", PQ_DFL | PQ_INT,
                            0, &idle_th, 0);

            parse_table_add(&pt, "STats", PQ_DFL | PQ_STRING,
                            0, &stats, 0);

            if (parse_arg_eq(a, &pt) < 0) {
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }
            if (ARG_CNT(a) > 0) {
                printk("%s: Unknown argument %s\n", ARG_CMD(a), ARG_CUR(a));
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }

            flags=0;

            for (i = 0; i < pt.pt_cnt; i++) {
                if (pt.pt_entries[i].pq_type & PQ_PARSED) {
                    flags |= (1 << i);
                }
            }

            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {

            if (flags & 0x1) {

                if (sal_strcasecmp(mode_type, "native") == 0) {
                    rv = bcm_port_phy_control_set(u,p,BCM_PORT_PHY_CONTROL_EEE,
                                               1);
                }
                if (sal_strcasecmp(mode_type, "auto") == 0) {
                    rv = bcm_port_phy_control_set(u,p,BCM_PORT_PHY_CONTROL_EEE_AUTO,
                                                   1);
                }
                if (sal_strcasecmp(mode_type, "none") == 0) {
                    rv = bcm_port_phy_control_set(u,p,BCM_PORT_PHY_CONTROL_EEE_AUTO,
                                                   0);
                    rv = bcm_port_phy_control_set(u,p,BCM_PORT_PHY_CONTROL_EEE,
                                               0);
                }

                if (rv == BCM_E_NONE) {
                    printk("Port %s EEE mode set to %s EEE mode\n", SOC_PORT_NAME(u, p), mode_type);
                } else {
                    if (rv == BCM_E_UNAVAIL) {
                        printk("Port %s EEE %s mode not available\n", SOC_PORT_NAME(u, p), mode_type);
                    } else {
                        printk("Port %s EEE %s mode set unsuccessful\n", SOC_PORT_NAME(u, p), mode_type);
                    }
                }
                rv = bcm_port_phy_control_get(u,p,BCM_PORT_PHY_CONTROL_EEE_AUTO,
                                          &eee_auto_mode_value);
                if ((rv == BCM_E_NONE) && (eee_auto_mode_value == 1)) {
                        str = "auto";
                } else {
                    rv = bcm_port_phy_control_get(u,p,
                                 BCM_PORT_PHY_CONTROL_EEE,
                                 &eee_mode_value);
                    if (rv == BCM_E_NONE) {
                        if (eee_mode_value == 0 && eee_mode_value == 0) {
                            str = "none";
                        } else {
                            str = "native";
                        }
                    } else {
                        str = "NA";
                    }
                }
                printk("Port %s EEE mode = %s\n", SOC_PORT_NAME(u, p), str);
            } 

            if (flags & 0x2) {
                if (sal_strcasecmp(lstr, "fixed") == 0) {
                    rv = bcm_port_phy_control_set(u,p,BCM_PORT_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY,
                                               1);
                }
                if (sal_strcasecmp(lstr, "variable") == 0) {
                    rv = bcm_port_phy_control_set(u,p,BCM_PORT_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY,
                                                   0);
                }
                rv = bcm_port_phy_control_get(u,p,
                                      BCM_PORT_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY,
                                      &latency);
                if (rv == BCM_E_NONE) {
                    if (latency) {
                        str = "fixed";
                    } else {
                        str = "variable";
                    }
                    printk("Port %s EEE Auto mode Latency = %s\n", SOC_PORT_NAME(u, p), str);
                }
            }


            if (flags & 0x4) {
                if ((rv = bcm_port_phy_control_set(u,p,BCM_PORT_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD,
                                           idle_th)) != BCM_E_NONE) {
                    return CMD_FAIL;
                }
                if ((rv = bcm_port_phy_control_get(u,p,
                                  BCM_PORT_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD,
                                  &idle)) != BCM_E_NONE) {
                    return CMD_FAIL;
                }
                printk("Port %s EEE Auto mode IDLE Threshold = %d\n", SOC_PORT_NAME(u, p), idle);
            }

            if (flags & 0x8) {
            if (sal_strcasecmp(stats, "get") == 0) {
                (void)bcm_port_phy_control_get(u,p,BCM_PORT_PHY_CONTROL_EEE_TRANSMIT_EVENTS,
                                           &tx_events);
                (void)bcm_port_phy_control_get(u,p,BCM_PORT_PHY_CONTROL_EEE_TRANSMIT_DURATION,
                                           &tx_duration);
                (void)bcm_port_phy_control_get(u,p,BCM_PORT_PHY_CONTROL_EEE_RECEIVE_EVENTS,
                                           &rx_events);
                (void)bcm_port_phy_control_get(u,p,BCM_PORT_PHY_CONTROL_EEE_RECEIVE_DURATION,
                                           &rx_duration);
                printk("Port %s Tx events = %d TX Duration = %d RX events = %d RX "
                        "Duration = %d\n", SOC_PORT_NAME(u, p), tx_events, tx_duration, 
                                          rx_events, rx_duration);
            }
            if (sal_strcasecmp(stats, "clear") == 0) {
                (void)bcm_port_phy_control_set(u,p,BCM_PORT_PHY_CONTROL_EEE_STATISTICS_CLEAR,
                                           1);
                printk("Port %s Statistics Cleared \n", SOC_PORT_NAME(u, p));
            }

            if (sal_strcasecmp(stats, "all") == 0) {
                (void)bcm_port_phy_control_get(u,p,
                                     BCM_PORT_PHY_CONTROL_EEE_TRANSMIT_EVENTS,
                                     &tx_events);
                (void)bcm_port_phy_control_get(u,p,
                                 BCM_PORT_PHY_CONTROL_EEE_TRANSMIT_DURATION,
                                 &tx_duration);
                (void)bcm_port_phy_control_get(u,p,
                                 BCM_PORT_PHY_CONTROL_EEE_RECEIVE_EVENTS,
                                 &rx_events);
                rv = bcm_port_phy_control_get(u,p,
                                 BCM_PORT_PHY_CONTROL_EEE_RECEIVE_DURATION,
                                 &rx_duration);
                if (rv == BCM_E_NONE) {
                    printk("Port %s EEE Statistics\n", SOC_PORT_NAME(u, p));
                    printk("\tEEE Transmit Events %d\n", tx_events);
                    printk("\tEEE Transmit Duration %d\n", tx_duration);
                    printk("\tEEE Receive Events %d\n", rx_events);
                    printk("\tEEE Receive Duration %d\n", rx_duration);
                }
            }
         }
      }
        /* free allocated memory from arg parsing */
        parse_arg_eq_done(&pt);
    } else {

            printk("EEE Details:\n");
            printk("%10s %16s %16s %14s %14s\n",
               "port", "name", "eee mode", "latency mode","Idle Threshold(ms)");
            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
                latency_str = "NA";

                if ((rv = bcm_port_phy_control_get(u,p,BCM_PORT_PHY_CONTROL_EEE_AUTO,
                                      &eee_auto_mode_value)) == BCM_E_FAIL) {
                    printk("Phy control get:BCM_PORT_PHY_CONTROL_EEE_AUTO failed\n");
                    return BCM_E_FAIL;
                }
                if (eee_auto_mode_value == 1) {
                    str = "auto";
                    rv = bcm_port_phy_control_get(u,p,
                            BCM_PORT_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY,
                            &latency);
                    if (rv == BCM_E_NONE) {
                        if (latency) {
                            latency_str = "fixed";
                        } else {
                            latency_str = "variable";
                        }
                    } else {
                        latency_str = "NA";
                    }
                    rv = bcm_port_phy_control_get(u,p,
                          BCM_PORT_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD,
                          &idle);
                    if (rv != BCM_E_NONE) {
                        idle = 0;
                    }
                } else {
                    if ((rv = bcm_port_phy_control_get(u,p,BCM_PORT_PHY_CONTROL_EEE,
                                              &eee_mode_value)) == BCM_E_FAIL) {
                        printk("Phy control get:BCM_PORT_PHY_CONTROL_EEE failed\n");
                        return BCM_E_FAIL;
                    }
                    if (eee_mode_value == 1){
                        str = "native";
                    } else {
                        str = "none";
                    }
                }
                printk("%5s(%3d) %16s %14s %14s %10d\n",
                       SOC_PORT_NAME(u, p), p,
                       soc_phy_name_get(u, p),
                       str, latency_str, idle);
            }
        }
        return CMD_OK;
    }

    if (c != NULL && sal_strcasecmp(c, "timesync") == 0) {
        bcm_port_phy_timesync_config_t conf;
        uint64 val64;

        sal_memset(&conf, 0, sizeof(conf));

        if (((c = ARG_GET(a)) == NULL) || (parse_bcm_pbmp(u, c, &pbm) < 0)) {
            printk("%s: ERROR: unrecognized port bitmap: %s\n", ARG_CMD(a), c);
            return CMD_FAIL;
        }
        if ((c = ARG_CUR(a)) != NULL) {
            parse_table_t    pt;
            uint32 enable, capture_ts, heartbeat_ts, rx_crc, as, l2, ip4, ip6, ec, /* bool */
                   itpid, otpid, tx_offset, rx_offset, original_timecode_seconds, original_timecode_nanoseconds,
                   load_all, frame_sync, flags,flags_2, ibts_sync = 0;
           uint32 ibts_dreq = 0, ibts_pdreq = 0, ibts_pdrsp = 0,
                  ibts_resv_id_chk = 0, ibts_resv_id_upd = 0, ibts_resv_id=0; 
            char   *gmode_str=NULL, *tx_sync_mode_str=NULL, *tx_delay_request_mode_str=NULL, 
                   *tx_pdelay_request_mode_str=NULL, *tx_pdelay_response_mode_str=NULL,
                   *rx_sync_mode_str=NULL, *rx_delay_request_mode_str=NULL,
                   *rx_pdelay_request_mode_str=NULL, *rx_pdelay_response_mode_str=NULL,
                   *framesync_mode_str=NULL, *syncout_mode_str=NULL, *ibts_match = NULL;

            parse_table_init(u, &pt);
            parse_table_add(&pt, "ENable", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, /* index 0 */
                            0, &enable, 0);
            parse_table_add(&pt, "CaptureTS", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, /* index 1 */
                            0, &capture_ts, 0);
            parse_table_add(&pt, "HeartbeatTS", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, /* index 2 */
                            0, &heartbeat_ts, 0);
            parse_table_add(&pt, "RxCrc", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, /* index 3 */
                            0, &rx_crc, 0);
            parse_table_add(&pt, "AS", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, /* index 4 */
                            0, &as, 0);
            parse_table_add(&pt, "L2", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, /* index 5 */
                            0, &l2, 0);
            parse_table_add(&pt, "IP4", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, /* index 6 */
                            0, &ip4, 0);
            parse_table_add(&pt, "IP6", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, /* index 7 */
                            0, &ip6, 0);
            parse_table_add(&pt, "ExtClock", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, /* index 8 */
                            0, &ec, 0);
            parse_table_add(&pt, "ITpid", PQ_DFL | PQ_INT, /* index 9 */
                            0, &itpid, 0);
            parse_table_add(&pt, "OTpid", PQ_DFL | PQ_INT, /* index 10 */
                            0, &otpid, 0);
            parse_table_add(&pt, "GMode", PQ_DFL | PQ_STRING, /* index 11 */
                            0, &gmode_str, 0);
            parse_table_add(&pt, "TxOffset", PQ_DFL | PQ_INT, /* index 12 */
                            0, &tx_offset, 0);
            parse_table_add(&pt, "RxOffset", PQ_DFL | PQ_INT, /* index 13 */
                            0, &rx_offset, 0);
            parse_table_add(&pt, "TxSync", PQ_DFL | PQ_STRING, /* index 14 */
                            0, &tx_sync_mode_str, 0);
            parse_table_add(&pt, "TxDelayReq", PQ_DFL | PQ_STRING, /* index 15 */
                            0, &tx_delay_request_mode_str, 0);
            parse_table_add(&pt, "TxPdelayReq", PQ_DFL | PQ_STRING, /* index 16 */
                            0, &tx_pdelay_request_mode_str, 0);
            parse_table_add(&pt, "TxPdelayreS", PQ_DFL | PQ_STRING, /* index 17 */
                            0, &tx_pdelay_response_mode_str, 0);
            parse_table_add(&pt, "RxSync", PQ_DFL | PQ_STRING, /* index 18 */
                            0, &rx_sync_mode_str, 0);
            parse_table_add(&pt, "RxDelayReq", PQ_DFL | PQ_STRING, /* index 19 */
                            0, &rx_delay_request_mode_str, 0);
            parse_table_add(&pt, "RxPdelayReq", PQ_DFL | PQ_STRING, /* index 20 */
                            0, &rx_pdelay_request_mode_str, 0);
            parse_table_add(&pt, "RxPdelayreS", PQ_DFL | PQ_STRING, /* index 21 */
                            0, &rx_pdelay_response_mode_str, 0);
            parse_table_add(&pt, "OriginalTimecodeSeconds", PQ_DFL | PQ_INT, /* index 22 */
                            0, &original_timecode_seconds, 0);
            parse_table_add(&pt, "OriginalTimecodeNanoseconds", PQ_DFL | PQ_INT, /* index 23 */
                            0, &original_timecode_nanoseconds, 0);

            parse_table_add(&pt, "FramesyncMode", PQ_DFL | PQ_STRING, /* index 24 */
                            0, &framesync_mode_str, 0);
            parse_table_add(&pt, "SyncoutMode", PQ_DFL | PQ_STRING, /* index 25 */
                            0, &syncout_mode_str, 0);
            parse_table_add(&pt, "LoadAll", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, /* index 26 */
                            0, &load_all, 0);
            parse_table_add(&pt, "FrameSync", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, /* index 27 */
                            0, &frame_sync, 0);
     
            parse_table_add(&pt, "InBandTimeStampSync", 
                    PQ_DFL | PQ_BOOL | PQ_NO_EQ_OPT, 0, &ibts_sync, 0);/* index 28 */
            parse_table_add(&pt, "InBandTimeStampDelayRequest", 
                    PQ_DFL | PQ_BOOL | PQ_NO_EQ_OPT, 0, &ibts_dreq, 0);/* index 29 */
            parse_table_add(&pt, "InBandTimeStampPdelayRequest", 
                    PQ_DFL | PQ_BOOL | PQ_NO_EQ_OPT, 0, &ibts_pdreq, 0);/* index 30 */
            parse_table_add(&pt, "InBandTimeStampPdelayreSponse", 
                    PQ_DFL | PQ_BOOL | PQ_NO_EQ_OPT, 0, &ibts_pdrsp, 0);/* index 31 */
            parse_table_add(&pt, "InBandTimeStampID", 
                    PQ_DFL | PQ_INT, 0, &ibts_resv_id, 0)/* flag_2 index 0 */;
            parse_table_add(&pt, "InBandTimeStampIDCHecK", 
                    PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, 0, &ibts_resv_id_chk, 0);/* flag_2 index 1 */
            parse_table_add(&pt, "InBandTimeStampIDUPDate", 
                    PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT, 0, &ibts_resv_id_upd, 0);/* flag_2 index 2 */
            parse_table_add(&pt, "InBandTimeStampMATCH", 
                    PQ_DFL | PQ_STRING, "none", &ibts_match, 0); /* flag_2 index 3 */

            if (parse_arg_eq(a, &pt) < 0) {
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }
            if (ARG_CNT(a) > 0) {
                printk("%s: Unknown argument %s\n", ARG_CMD(a), ARG_CUR(a));
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }

            flags=0;
            flags_2 = 0;

            for (i = 0; i < pt.pt_cnt; i++) {
                if (pt.pt_entries[i].pq_type & PQ_PARSED) {
                    if(i>=(sizeof(flags)*8)) {
                        flags_2 |= (1 << (i - (sizeof(flags)*8)));
                    } else {
                        flags |= (1 << i);
                    }

                }
            }

            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
                conf.validity_mask = 0xffffffff & (~(BCM_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL));
                if ((rv = bcm_port_phy_timesync_config_get(u,p,&conf)) == BCM_E_FAIL) {
                    printk("bcm_port_phy_timesync_config_get() failed, u=%d, p=%d\n", u, p);
                    return BCM_E_FAIL;
                }

                conf.validity_mask &= ~BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL;

                if (flags & (1U << 0)) {
                    conf.flags &= ~BCM_PORT_PHY_TIMESYNC_ENABLE;
                    conf.flags |= enable ? BCM_PORT_PHY_TIMESYNC_ENABLE : 0;
                }

                if (flags & (1U << 1)) {
                    conf.flags &= ~BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;
                    conf.flags |= capture_ts ? BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE : 0;
                }

                if (flags & (1U << 2)) {
                    conf.flags &= ~BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE;
                    conf.flags |= heartbeat_ts ? BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE : 0;
                }

                if (flags & (1U << 3)) {
                    conf.flags &= ~BCM_PORT_PHY_TIMESYNC_RX_CRC_ENABLE;
                    conf.flags |= rx_crc ? BCM_PORT_PHY_TIMESYNC_RX_CRC_ENABLE : 0;
                }

                if (flags & (1U << 4)) {
                    conf.flags &= ~BCM_PORT_PHY_TIMESYNC_8021AS_ENABLE;
                    conf.flags |= as ? BCM_PORT_PHY_TIMESYNC_8021AS_ENABLE : 0;
                }

                if (flags & (1U << 5)) {
                    conf.flags &= ~BCM_PORT_PHY_TIMESYNC_L2_ENABLE;
                    conf.flags |= l2 ? BCM_PORT_PHY_TIMESYNC_L2_ENABLE : 0;
                }

                if (flags & (1U << 6)) {
                    conf.flags &= ~BCM_PORT_PHY_TIMESYNC_IP4_ENABLE;
                    conf.flags |= ip4 ? BCM_PORT_PHY_TIMESYNC_IP4_ENABLE : 0;
                }

                if (flags & (1U << 7)) {
                    conf.flags &= ~BCM_PORT_PHY_TIMESYNC_IP6_ENABLE;
                    conf.flags |= ip6 ? BCM_PORT_PHY_TIMESYNC_IP6_ENABLE : 0;
                }

                if (flags & (1U << 8)) {
                    conf.flags &= ~BCM_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT;
                    conf.flags |= ec ? BCM_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT : 0;
                }

                if (flags & (1U << 9)) {
                    conf.itpid = itpid;
                }

                if (flags & (1U << 10)) {
                    conf.otpid = otpid;
                }

                if (flags & (1U << 11)) {
                    conf.gmode = _convert_timesync_gmode_str(gmode_str, conf.gmode);
                }

                if (flags & (1U << 12)) {
                    conf.tx_timestamp_offset = tx_offset;
                }

                if (flags & (1U << 13)) {
                    conf.rx_timestamp_offset = rx_offset;
                }

                if (flags & (1U << 14)) {
                    conf.tx_sync_mode = _convert_timesync_egress_message_str(tx_sync_mode_str, conf.tx_sync_mode);
                }

                if (flags & (1U << 15)) {
                    conf.tx_delay_request_mode = _convert_timesync_egress_message_str(tx_delay_request_mode_str, 
                                                   conf.tx_delay_request_mode);
                }

                if (flags & (1U << 16)) {
                    conf.tx_pdelay_request_mode = _convert_timesync_egress_message_str(tx_pdelay_request_mode_str,
                                                   conf.tx_pdelay_request_mode);
                }

                if (flags & (1U << 17)) {
                    conf.tx_pdelay_response_mode = _convert_timesync_egress_message_str(tx_pdelay_response_mode_str,
                                                    conf.tx_pdelay_response_mode);
                }

                if (flags & (1U << 18)) {
                    conf.rx_sync_mode = _convert_timesync_ingress_message_str(rx_sync_mode_str, conf.rx_sync_mode);
                }

                if (flags & (1U << 19)) {
                    conf.rx_delay_request_mode = _convert_timesync_ingress_message_str(rx_delay_request_mode_str,
                                                  conf.rx_delay_request_mode);
                }

                if (flags & (1U << 20)) {
                    conf.rx_pdelay_request_mode = _convert_timesync_ingress_message_str(rx_pdelay_request_mode_str,
                                                   conf.rx_pdelay_request_mode);
                }

                if (flags & (1U << 21)) {
                    conf.rx_pdelay_response_mode = _convert_timesync_ingress_message_str(rx_pdelay_response_mode_str,
                                                    conf.rx_pdelay_response_mode);
                }

                if (flags & (1U << 22)) {

                    COMPILER_64_SET(conf.original_timecode.seconds, 0,
                                    original_timecode_seconds);
                }

                if (flags & (1U << 23)) {
                    conf.original_timecode.nanoseconds = original_timecode_nanoseconds;
                }

                if (flags & (1U << 24)) {
                    conf.framesync.mode = _convert_framesync_mode_str(framesync_mode_str, conf.framesync.mode);
                }

                if (flags & (1U << 25)) {
                    conf.syncout.mode = _convert_syncout_mode_str(syncout_mode_str, conf.syncout.mode);
                }
                if(flags & (1U << 28)) {
                    conf.validity_mask |= BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL;
                    conf.inband_ts_control.flags &= ~BCM_PORT_PHY_TIMESYNC_INBAND_TS_SYNC_ENABLE;
                    conf.inband_ts_control.flags |= ibts_sync? BCM_PORT_PHY_TIMESYNC_INBAND_TS_SYNC_ENABLE : 0;
                }
                if(flags & (1U << 29)) {
                    conf.validity_mask |= BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL;
                    conf.inband_ts_control.flags &= ~BCM_PORT_PHY_TIMESYNC_INBAND_TS_DELAY_RQ_ENABLE;
                    conf.inband_ts_control.flags |= ibts_dreq ? BCM_PORT_PHY_TIMESYNC_INBAND_TS_DELAY_RQ_ENABLE : 0;
                }
                if(flags & (1U << 30)) {
                    conf.validity_mask |= BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL;
                    conf.inband_ts_control.flags &= ~BCM_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RQ_ENABLE; 
                    conf.inband_ts_control.flags |= ibts_pdreq? BCM_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RQ_ENABLE : 0; 
                }
                if(flags & (1U << 31)) {
                    conf.validity_mask |= BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL;
                    conf.inband_ts_control.flags &= ~BCM_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RESP_ENABLE;
                    conf.inband_ts_control.flags |= ibts_pdrsp? BCM_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RESP_ENABLE : 0;
                }
                if(flags_2 & (1U << 0)) {
                    conf.validity_mask |= BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL;
                    conf.inband_ts_control.resv0_id = ibts_resv_id;
                }
                if(flags_2 & (1U << 1)) {
                    conf.validity_mask |= BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL;
                    conf.inband_ts_control.flags &= ~BCM_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_CHECK;
                    conf.inband_ts_control.flags |= ibts_resv_id_chk? BCM_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_CHECK : 0;
                }
                if(flags_2 & (1U << 2)) {
                    conf.validity_mask |= BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL;
                    conf.inband_ts_control.flags &= ~BCM_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_UPDATE;
                    conf.inband_ts_control.flags |= ibts_resv_id_upd? BCM_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_UPDATE : 0;
                }
                if(flags_2 & (1U << 3)) {
                    conf.validity_mask |= BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL;
                    _set_inband_timesync_matching_criterion(ibts_match, &conf.inband_ts_control.flags);     
                        
                }

                conf.validity_mask = 0xffffffff & ~BCM_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL;

                if ((rv = bcm_port_phy_timesync_config_set(u,p,&conf)) == BCM_E_FAIL) {
                    printk("bcm_port_phy_timesync_config_set() failed, u=%d, p=%d\n", u, p);
                    return BCM_E_FAIL;
                }
                if (flags & (1U << 26)) {
                    COMPILER_64_SET(val64, 0, 0xaaaaaaaa);
                    rv = bcm_port_control_phy_timesync_set(u, p, bcmPortControlPhyTimesyncLoadControl, val64);
                    if (rv != BCM_E_NONE) {
                       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                    }
                }
                if (flags & (1U << 27)) {
                    COMPILER_64_SET(val64, 0, 1);
                    rv = bcm_port_control_phy_timesync_set(u, p, bcmPortControlPhyTimesyncFrameSync, val64);
                    if (rv != BCM_E_NONE) {
                       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                    }
                }

            }

            /* free allocated memory from arg parsing */
            parse_arg_eq_done(&pt);

        } else {

            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
                int offset;

                soc_phyctrl_offset_get(u, p, &offset); /* return value not checked on purpose */

                printk("\n\nIEEE1588 settings for %s(%3d) %s, offset = %d\n",
                       SOC_PORT_NAME(u, p), p,
                       soc_phy_name_get(u, p), offset);

                conf.validity_mask = 0xffffffff & ~BCM_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL;
                if ((rv = bcm_port_phy_timesync_config_get(u,p,&conf)) == BCM_E_FAIL) {
                    printk("bcm_port_phy_timesync_config_get() failed, u=%d, p=%d\n", u, p);
                    return BCM_E_FAIL;
                }
                _print_timesync_config(&conf);
                _print_inband_timesync_config(&conf);
                _print_heartbeat_ts(u, p);
                _print_capture_ts(u, p);
                /*_print_enhanced_capture(u, p);*/
            }

        }
        return CMD_OK;
    }

#ifdef INCLUDE_PHY_SYM_DBG
    if (c != NULL && sal_strcasecmp(c, "SymDebug") == 0) {
        int portnum;
        sal_thread_t sym_dbg_thread;

        if (sym_dbg_arg.flags == SYM_DBG_THREAD_START) {
            printk("Thread already running\n");
            return CMD_OK;
        }

        /* Get TCP port number to listen on */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	portnum = sal_ctoi(c, 0);

        printk("Entering PHY Symbolic Debug mode. In this mode the link scan "
                "is DISABLED\n");
        printk("Listening on TCP port %d\n", portnum);

        sym_dbg_arg.unit = u;
        sym_dbg_arg.tcpportnum = portnum;
        sym_dbg_arg.flags = SYM_DBG_THREAD_START;
        sym_dbg_arg.sym_dbg_lock = sal_mutex_create("sym_dbg_lock");

        /* Create a thread to execute the sock functions */
        sym_dbg_thread = sal_thread_create("phy_sym_dbg", 0, SAL_THREAD_STKSZ,
                                           _phy_sym_debug_server, (void *)&u);
        if (sym_dbg_thread == NULL) {
            printk("Unable to create thread\n");
            return CMD_FAIL;
        }
        return CMD_OK;
    }

    if (c != NULL && sal_strcasecmp(c, "SymDebugOff") == 0) {
        printk("Stopping server thread\n");
        SYM_DBG_LOCK(sym_dbg_arg);
        sym_dbg_arg.flags = SYM_DBG_THREAD_STOP;
        SYM_DBG_UNLOCK(sym_dbg_arg);
        return CMD_OK;
    }
#endif

    if (c != NULL && sal_strcasecmp(c, "firmware") == 0) {
#ifdef  NO_FILEIO
        printk("This command is not supported without file I/O\n");
        return CMD_FAIL;
#else
        parse_table_t    pt;
        int count;
        FILE      *fp = NULL;
        uint8 *buf;
        int len;
        int offset;
        char    input[32];
        int no_confirm = FALSE;
        #define FIRMWARE_BUF_LEN   0x80000

        char *filename = NULL;

        if (((c = ARG_GET(a)) == NULL) || (parse_bcm_pbmp(u, c, &pbm) < 0)) {
            printk("%s: ERROR: unrecognized port bitmap: %s\n", ARG_CMD(a), c);
            return CMD_FAIL;
        }

        SOC_PBMP_COUNT(pbm, count);

        if (count > 1) { 
            printk("ERROR: too many ports specified : %d\n", count);
            return CMD_FAIL;
        }
        if ((c = ARG_CUR(a)) != NULL) {

            if (c[0] == '=') {
                return CMD_USAGE;        /* '=' unsupported */
            }

            parse_table_init(u, &pt);
            parse_table_add(&pt, "set", PQ_DFL | PQ_STRING,0,
                    &filename, NULL);
            parse_table_add(&pt, "-y", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,0,
                    &no_confirm, NULL);

            if (parse_arg_eq(a, &pt) < 0) {
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }
            if (ARG_CNT(a) > 0) {
                printk("%s: Unknown argument %s\n", ARG_CMD(a), ARG_CUR(a));
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }
        }
        
        if (!filename) {
            printk("ERROR: file name %s not found\n", filename);
            return CMD_FAIL;
        }

        if ((fp = sal_fopen(filename, "rb")) == NULL) {
            parse_arg_eq_done(&pt);
            printk("ERROR: Can't open the file : %s\n", filename);
            return CMD_FAIL;
        }

        /* filename points to a allocated memory from arg parsing, calling 
         * the routine below frees it
         */ 
        parse_arg_eq_done(&pt);

        if ( !no_confirm ) {
            /* prompt user for confirmation */
            printk("Warning!!!\n"
                     "The PHY device will become un-usable if the power is off\n"
                     "during this process or a wrong file is given. The file must\n"
                     "be in BINARY format. The only way to recover is to program\n"
                     "the non-volatile storage device with a rom burner\n");

            if ((NULL == sal_readline("Are you sure you want to continue(yes/no)?",
                                     input, sizeof(input), "no")) || 
                (sal_strlen(input) != sal_strlen("yes")) ||
                (sal_strncasecmp("yes", input, sal_strlen(input)))) {
                sal_fclose(fp);
                printk("Firmware updating aborted. No writes to the PHY device's "
                       "non-volatile storage\n");
                return CMD_FAIL;
            }
        }

        buf = sal_alloc(FIRMWARE_BUF_LEN,"temp_buf");
        if (buf == NULL) {
            sal_fclose(fp);
            printk("ERROR: Can't allocate enough buffer : 0x%x\n", 
                    FIRMWARE_BUF_LEN);
            return CMD_FAIL;
        }

        printk("Firmware updating in progress. ");
        DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {
            offset = 0;
            len = 0;
            do {
                offset += len;
                /*    coverity[tainted_data_argument]    */
                len = sal_fread(buf, 1, FIRMWARE_BUF_LEN, fp);
                printk("Data length: %d\n",len);
                printk("Please wait ....\n");
                
                /* for now, only allow external phy. If internal phy is
                 * intended use the flag BCM_PORT_PHY_INTERNAL
                 */
                rv = bcm_port_phy_firmware_set(u,p,0,offset,buf,len);
            } while (len >= FIRMWARE_BUF_LEN);
            break;
        }
        sal_fclose(fp);
                /*    coverity[tainted_data]    */
        sal_free(buf);
        if (rv == SOC_E_NONE) {
            printk("Successfully Done!!!\n");
        } else if (rv == SOC_E_UNAVAIL) {
            printk("Exit. The feature is not available to this phy device\n");
        } else {
            printk("Failed. Phy device may not be usable\n");
        }
        return CMD_OK;
#endif
    }


     if (c != NULL && sal_strcasecmp(c, "oam") == 0) {
        bcm_port_config_phy_oam_t conf;
        uint64 val64;
        
        sal_memset(&conf, 0, sizeof(bcm_port_config_phy_oam_t));
        COMPILER_64_SET(val64,0,0);
        
        if (((c = ARG_GET(a)) == NULL) || (parse_bcm_pbmp(u, c, &pbm) < 0)) {
            printk("%s: ERROR: unrecognized port bitmap: %s\n", ARG_CMD(a), c);
            return CMD_FAIL;
        }
        if ((c = ARG_CUR(a)) != NULL) {
            parse_table_t    pt;
            uint32 mac_addr_hi = 0, mac_addr_low = 0, flags;
            uint32 mac_addr_hi_1 = 0, mac_addr_low_1 = 0;
            uint32 mac_addr_hi_2 = 0, mac_addr_low_2 = 0;
            uint32 mac_addr_hi_3 = 0, mac_addr_low_3 = 0;
            uint32 eth_type = 0;
            uint32  mac_check_en = 0, cw_en = 0, entropy_en = 0,
                   mac_index = 0, timestamp = 0;
            char * mode,  * dir, *ts_format;
            uint8 tx=0,rx=0, oam_mode;
            uint32 type;

            parse_table_init(u, &pt);
            parse_table_add(&pt, "MODE", PQ_DFL | PQ_STRING, 
                            0, &mode, 0);
            parse_table_add(&pt, "DIR", PQ_DFL | PQ_STRING, 
                            0, &dir, 0);
            parse_table_add(&pt, "MacCheck", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &mac_check_en, 0);
            parse_table_add(&pt, "ControlWord", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &cw_en, 0);
            parse_table_add(&pt, "ENTROPY", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &entropy_en, 0);
            parse_table_add(&pt, "TimeStampFormat", PQ_DFL | PQ_STRING,
                            0, &ts_format, 0);
            parse_table_add(&pt, "ETHerType", PQ_INT | PQ_DFL,
                            0, &eth_type, 0);
            parse_table_add(&pt, "MACIndex", PQ_INT | PQ_DFL,
                            0, &mac_index, 0);
            parse_table_add(&pt, "MACAddrHi", PQ_INT | PQ_DFL,
                            0, &mac_addr_hi, 0);
            parse_table_add(&pt, "MACAddrLow", PQ_INT | PQ_DFL,
                            0, &mac_addr_low, 0);
            parse_table_add(&pt, "MACAddrHi1", PQ_INT | PQ_DFL,
                            0, &mac_addr_hi_1, 0);
            parse_table_add(&pt, "MACAddrLow1", PQ_INT | PQ_DFL,
                            0, &mac_addr_low_1, 0);
            parse_table_add(&pt, "MACAddrHi2", PQ_INT | PQ_DFL,
                            0, &mac_addr_hi_2, 0);
            parse_table_add(&pt, "MACAddrLow2", PQ_INT | PQ_DFL,
                            0, &mac_addr_low_2, 0);
            parse_table_add(&pt, "MACAddrHi3", PQ_INT | PQ_DFL,
                            0, &mac_addr_hi_3, 0);
            parse_table_add(&pt, "MACAddrLow3", PQ_INT | PQ_DFL,
                            0, &mac_addr_low_3, 0);


            if (parse_arg_eq(a, &pt) < 0) {
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }
            if (ARG_CNT(a) > 0) {
                printk("%s: Unknown argument %s\n", ARG_CMD(a), ARG_CUR(a));
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }

            flags=0;

            for (i = 0; i < pt.pt_cnt; i++) {
                if (pt.pt_entries[i].pq_type & PQ_PARSED) {
                    flags |= (1 << i);
                }
            }

            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
                if ((rv = bcm_port_config_phy_oam_get(u,p,&conf)) == BCM_E_FAIL) {
                    printk("bcm_port_config_phy_oam_get() failed, u=%d, p=%d\n", u, p);
                    return CMD_FAIL;
                }

                if (flags & (1U << 1)) {
                    if(!sal_strcmp(dir, "tx")) {
                        tx = 1;
                    } else if(!sal_strcmp(dir, "rx")) {
                        rx = 1;
                    } else {
                        return CMD_USAGE;
                    }
                } else {
                    tx = 1; rx = 1;
                }

                if(flags & (1U << 0)) {
                    if(!sal_strcmp(mode, "y1731")) {
                        oam_mode = bcmPortConfigPhyOamDmModeY1731;
                    } else if(!sal_strcmp(mode, "bhh")) {
                        oam_mode = bcmPortConfigPhyOamDmModeBhh;
                    } else if (!sal_strcmp(mode, "ietf")) {
                        oam_mode = bcmPortConfigPhyOamDmModeIetf;
                    } else {
                        return CMD_USAGE;
                    }
                    
                    if(tx) {
                        conf.tx_dm_config.mode = oam_mode;
                    } 
                    if(rx) {
                        conf.rx_dm_config.mode = oam_mode;
                    } 
                }

                if (flags & (1U << 2)) {
                    if(tx) {
                        conf.tx_dm_config.flags &= ~BCM_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE;
                        conf.tx_dm_config.flags |= mac_check_en? 
                                                      BCM_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE : 0;
                    }
                    if(rx) {
                        conf.rx_dm_config.flags &= ~BCM_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE;
                        conf.rx_dm_config.flags |= mac_check_en? 
                                                      BCM_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE : 0;
                    }
                }
                if (flags & (1U << 3)) {
                    if(tx) {
                        conf.tx_dm_config.flags &= ~BCM_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE;
                        conf.tx_dm_config.flags |= cw_en? 
                                                      BCM_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE : 0;
                    }
                    if(rx) {
                        conf.rx_dm_config.flags &= ~BCM_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE;
                        conf.rx_dm_config.flags |= cw_en? 
                                                      BCM_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE : 0;
                    }
                }
                if (flags & (1U << 4)) {
                    if(tx) {
                        conf.tx_dm_config.flags &= ~BCM_PORT_PHY_OAM_DM_ENTROPY_ENABLE;
                        conf.tx_dm_config.flags |= entropy_en? 
                                                      BCM_PORT_PHY_OAM_DM_ENTROPY_ENABLE : 0;
                    }
                    if(rx) {
                        conf.rx_dm_config.flags &= ~BCM_PORT_PHY_OAM_DM_ENTROPY_ENABLE;
                        conf.rx_dm_config.flags |= entropy_en? 
                                                      BCM_PORT_PHY_OAM_DM_ENTROPY_ENABLE : 0;
                    }
                }
                if (flags & (1U << 5)) {
                     if(!sal_strcmp(ts_format, "ntp")) {
                        timestamp = 1;
                    } else if(!sal_strcmp(ts_format, "ptp")) {
                        timestamp = 0;
                    } else {
                        return CMD_USAGE;
                    }

                    if(tx) {
                        conf.tx_dm_config.flags &= ~BCM_PORT_PHY_OAM_DM_TS_FORMAT;
                        conf.tx_dm_config.flags |= timestamp? 
                                                      BCM_PORT_PHY_OAM_DM_TS_FORMAT : 0;
                    }
                    if(rx) {
                        conf.rx_dm_config.flags &= ~BCM_PORT_PHY_OAM_DM_TS_FORMAT;
                        conf.rx_dm_config.flags |= timestamp? 
                                                      BCM_PORT_PHY_OAM_DM_TS_FORMAT : 0;
                    }
                }

               /* CONFIG SET */ 
                if ((rv = bcm_port_config_phy_oam_set(u,p,&conf)) == BCM_E_FAIL) {
                    printk("bcm_port_config_phy_oam_set() failed, u=%d, p=%d\n", u, p);
                    return CMD_FAIL;
                }


                if (flags & (1U << 6)) {
                    COMPILER_64_SET(val64, 0, eth_type);
                    if(tx) {
                        rv = bcm_port_control_phy_oam_set(u, p, 
                                bcmPortControlPhyOamDmTxEthertype, val64);
                        if (rv != BCM_E_NONE) {
                            printk("bcm_port_control_phy_oam_set failed with error \
                                    u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                            return CMD_FAIL;
                        }
                    }
                    if(rx) {
                        rv = bcm_port_control_phy_oam_set(u, p, 
                                bcmPortControlPhyOamDmRxEthertype, val64);
                        if (rv != BCM_E_NONE) {
                            printk("bcm_port_control_phy_oam_set failed with error  \
                                    u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                            return CMD_FAIL;
                        }
                    }
                }

                if (flags & (1U << 7)) {
                    if(mac_index == 0) {
                        return CMD_USAGE;
                    }
                    COMPILER_64_SET(val64, 0, mac_index);
                    if(tx) {
                        rv = bcm_port_control_phy_oam_set(u, p, 
                                bcmPortControlPhyOamDmTxPortMacAddressIndex, val64);
                        if (rv != BCM_E_NONE) {
                            printk("bcm_port_control_phy_oam_set failed with error  \
                                    u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                            return CMD_FAIL;
                        }
                    }
                    if(rx) {
                        rv = bcm_port_control_phy_oam_set(u, p, 
                                bcmPortControlPhyOamDmRxPortMacAddressIndex, val64);
                        if (rv != BCM_E_NONE) {
                            printk("bcm_port_control_phy_oam_set failed with error  \
                                    u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                            return CMD_FAIL;
                        }
                    }
                }
                /* If mac_index is not provided and mac_addr needs to be set
                   use mac_addr_1/2/3 instead.
                 */
                if(!(flags & (1U << 7)) && ((flags & (1U << 8)) || (flags & (1U << 9)))) {
					printk(" MACAddrHi/MACAddrLow cannot be used without 'MACIndex. Please use MACAddrHi1/2/3 for updating MAC address\n"); 
                    return CMD_FAIL;
                }
                if ((flags & (1U << 8))  || (flags & (1U << 9))){
                    switch(mac_index) {
                        case 1:
                            type = bcmPortControlPhyOamDmMacAddress1;
                            break;
                        case 2:
                            type = bcmPortControlPhyOamDmMacAddress2;
                            break;
                        case 3:
                            type = bcmPortControlPhyOamDmMacAddress3;
                            break;
                        default:
                            return CMD_FAIL; 
                    }   

                    COMPILER_64_SET(val64, mac_addr_hi, mac_addr_low);

                    rv = bcm_port_control_phy_oam_set(u, p, type, val64); 
                    if (rv != BCM_E_NONE) {
                        printk("bcm_port_control_phy_oam_set failed with error  \
                                u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                        return CMD_FAIL;
                    }
                }
                if ((flags & (1U << 10)) || (flags & (1U << 11))) {
                    COMPILER_64_SET(val64, mac_addr_hi_1, mac_addr_low_1);

                    rv = bcm_port_control_phy_oam_set(u, p, bcmPortControlPhyOamDmMacAddress1, val64); 
                    if (rv != BCM_E_NONE) {
                        printk("bcm_port_control_phy_oam_set failed with error  \
                                u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                        return CMD_FAIL;
                    }
                }
                if ((flags & (1U << 12)) || (flags & (1U << 13))) {
                    COMPILER_64_SET(val64, mac_addr_hi_2, mac_addr_low_2);

                    rv = bcm_port_control_phy_oam_set(u, p, bcmPortControlPhyOamDmMacAddress2, val64); 
                    if (rv != BCM_E_NONE) {
                        printk("bcm_port_control_phy_oam_set failed with error  \
                                u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                        return CMD_FAIL;
                    }
                }
                if ((flags & (1U << 14)) || (flags & (1U << 15))) {
                    COMPILER_64_SET(val64, mac_addr_hi_3, mac_addr_low_3);

                    rv = bcm_port_control_phy_oam_set(u, p, bcmPortControlPhyOamDmMacAddress3, val64); 
                    if (rv != BCM_E_NONE) {
                        printk("bcm_port_control_phy_oam_set failed with error  \
                                u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                        return CMD_FAIL;
                    }
                }

            }

            /* free allocated memory from arg parsing */
            parse_arg_eq_done(&pt);

        } else {

            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
                int offset;
                uint64 tx_ethtype, rx_ethtype;
                uint64  mac1, mac2, mac3; 
                uint64 tx_mac_index, rx_mac_index;

                COMPILER_64_SET(tx_ethtype, 0, 0);
                COMPILER_64_SET(rx_ethtype, 0, 0);
                COMPILER_64_SET(mac1, 0, 0);
                COMPILER_64_SET(mac2, 0, 0);
                COMPILER_64_SET(mac3, 0, 0);
                COMPILER_64_SET(tx_mac_index, 0, 0);
                COMPILER_64_SET(rx_mac_index, 0, 0);

                soc_phyctrl_offset_get(u, p, &offset); /* return value not checked on purpose */

                printk("\n\nOAM settings for %s(%3d) %s, offset = %d\n\n",
                       SOC_PORT_NAME(u, p), p,
                       soc_phy_name_get(u, p), offset);

                if ((rv = bcm_port_config_phy_oam_get(u,p,&conf)) == BCM_E_FAIL) {
                    printk("bcm_port_config_phy_oam_get() failed, u=%d, p=%d\n", u, p);
                    return CMD_FAIL;
                }

                rv = bcm_port_control_phy_oam_get(u, p, 
                        bcmPortControlPhyOamDmTxEthertype, &tx_ethtype);
                if (rv != BCM_E_NONE) {
                    printk("bcm_port_control_phy_oam_get (TxEthertype) failed with error \
                            u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                    return CMD_FAIL;
                }
                rv = bcm_port_control_phy_oam_get(u, p, 
                        bcmPortControlPhyOamDmRxEthertype, &rx_ethtype);
                if (rv != BCM_E_NONE) {
                    printk("bcm_port_control_phy_oam_get (RxEthertype) failed with error  \
                            u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                    return CMD_FAIL;
                }
                rv = bcm_port_control_phy_oam_get(u, p, 
                        bcmPortControlPhyOamDmTxPortMacAddressIndex, &tx_mac_index);
                if (rv != BCM_E_NONE) {
                    printk("bcm_port_control_phy_oam_get (TxMACIndex)failed with error  \
                            u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                    return CMD_FAIL;
                }
                rv = bcm_port_control_phy_oam_get(u, p, 
                        bcmPortControlPhyOamDmRxPortMacAddressIndex, &rx_mac_index);
                if (rv != BCM_E_NONE) {
                    printk("bcm_port_control_phy_oam_get (RxMACIndex)failed with error  \
                            u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                    return CMD_FAIL;
                }

                rv = bcm_port_control_phy_oam_get(u, p, bcmPortControlPhyOamDmMacAddress1, &mac1); 
                if (rv != BCM_E_NONE) {
                    printk("bcm_port_control_phy_oam_get (MacAddr1)failed with error  \
                            u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                    return CMD_FAIL;
                }
                rv = bcm_port_control_phy_oam_get(u, p, bcmPortControlPhyOamDmMacAddress2, &mac2); 
                if (rv != BCM_E_NONE) {
                    printk("bcm_port_control_phy_oam_get (MacAddr2)failed with error  \
                            u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                    return CMD_FAIL;
                }
                rv = bcm_port_control_phy_oam_get(u, p, bcmPortControlPhyOamDmMacAddress3, &mac3); 
                if (rv != BCM_E_NONE) {
                    printk("bcm_port_control_phy_oam_get (MacAddr3)failed with error  \
                            u=%d p=%d %s\n", u, p, bcm_errmsg(rv));
                    return CMD_FAIL;
                }



                printk("\nPHY OAM TX Config Settings:\n");
                printk("=============================\n");
                printk("MODE (Y1731, BHH or IETF) - %s\n", 
                        (conf.tx_dm_config.mode == bcmPortConfigPhyOamDmModeY1731)? "Y.1731" :
                        (conf.tx_dm_config.mode == bcmPortConfigPhyOamDmModeBhh)? "BHH" :
                        (conf.tx_dm_config.mode == bcmPortConfigPhyOamDmModeIetf)? "IETF" : "NONE");
                printk("MacCheck (Y or N) - %s\n", 
                        conf.tx_dm_config.flags & BCM_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE? "Y" : "N");    
                printk("ControlWord (Y or N) - %s\n", 
                        conf.tx_dm_config.flags & BCM_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE? "Y" : "N"); 
                printk("ENTROPY (Y or N) - %s\n", 
                        conf.tx_dm_config.flags & BCM_PORT_PHY_OAM_DM_ENTROPY_ENABLE? "Y" : "N"); 
                printk("TimeStampFormat (PTP or NTP) - %s\n", 
                        conf.tx_dm_config.flags & BCM_PORT_PHY_OAM_DM_TS_FORMAT? "NTP" : "PTP"); 
                printk("EtherType- 0x%x\n", COMPILER_64_LO(tx_ethtype));
                printk("MacIndex- %d\n", COMPILER_64_LO(tx_mac_index));


                printk("\nPHY OAM RX Config Settings:\n");
                printk("=============================\n");
                printk("MODE (Y1731, BHH or IETF) - %s\n", 
                        (conf.rx_dm_config.mode == bcmPortConfigPhyOamDmModeY1731)? "Y.1731" :
                        (conf.rx_dm_config.mode == bcmPortConfigPhyOamDmModeBhh)? "BHH" :
                        (conf.rx_dm_config.mode == bcmPortConfigPhyOamDmModeIetf)? "IETF" : "NONE");
                printk("MacCheck (Y or N) - %s\n", 
                        conf.rx_dm_config.flags & BCM_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE? "Y" : "N");    
                printk("ControlWord (Y or N) - %s\n", 
                        conf.rx_dm_config.flags & BCM_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE? "Y" : "N"); 
                printk("ENTROPY (Y or N) - %s\n", 
                        conf.rx_dm_config.flags & BCM_PORT_PHY_OAM_DM_ENTROPY_ENABLE? "Y" : "N"); 
                printk("TimeStampFormat (PTP or NTP) - %s\n", 
                        conf.rx_dm_config.flags & BCM_PORT_PHY_OAM_DM_TS_FORMAT? "NTP" : "PTP"); 
                printk("EtherType- 0x%x\n", COMPILER_64_LO(rx_ethtype));
                printk("MacIndex- %d\n", COMPILER_64_LO(rx_mac_index));
   
                printk("\nOther Settings:\n");
                printk("=============================\n");
                printk("MAC Address 1 - 0x%08x%08x\n", COMPILER_64_HI(mac1), COMPILER_64_LO(mac1));
                printk("MAC Address 2 - 0x%08x%08x\n", COMPILER_64_HI(mac2), COMPILER_64_LO(mac2));
                printk("MAC Address 3 - 0x%08x%08x\n", COMPILER_64_HI(mac3), COMPILER_64_LO(mac3));

            }

        }
        return CMD_OK;
    }

    if (c != NULL && sal_strcasecmp(c, "power") == 0) {
        parse_table_t    pt;
        char *mode_type = 0;
        uint32 mode_value;
        int sleep_time = -1;
        int wake_time = -1;

        if (((c = ARG_GET(a)) == NULL) || (parse_bcm_pbmp(u, c, &pbm) < 0)) {
            printk("%s: ERROR: unrecognized port bitmap: %s\n", ARG_CMD(a), c);
            return CMD_FAIL;
        }

        if ((c = ARG_CUR(a)) != NULL) {

            if (c[0] == '=') {
                return CMD_USAGE;        /* '=' unsupported */
            }

            parse_table_init(u, &pt);
            parse_table_add(&pt, "mode", PQ_DFL | PQ_STRING,0,
                    &mode_type, NULL);

            parse_table_add(&pt, "Sleep_Time", PQ_DFL | PQ_INT,
                        0, &sleep_time, NULL);

            parse_table_add(&pt, "Wake_Time", PQ_DFL | PQ_INT,
                        0, &wake_time, NULL);

            if (parse_arg_eq(a, &pt) < 0) {
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }
            if (ARG_CNT(a) > 0) {
                printk("%s: Unknown argument %s\n", ARG_CMD(a), ARG_CUR(a));
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }
        } else {
            char * str;
            printk("Phy Power Mode dump:\n");
            printk("%10s %16s %14s %14s %14s\n",
               "port", "name", "power_mode","sleep_time(ms)","wake_time(ms)");
            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
                mode_value = 0;
                sleep_time = 0;
                wake_time  = 0;
                rv = bcm_port_phy_control_get(u,p,BCM_PORT_PHY_CONTROL_POWER,
                                          &mode_value);
                if (rv == SOC_E_NONE) {
                    if (mode_value == BCM_PORT_PHY_CONTROL_POWER_AUTO) {
                        str = "auto_down";
                        if ((rv = bcm_port_phy_control_get(u,p,
                                        BCM_PORT_PHY_CONTROL_POWER_AUTO_SLEEP_TIME,
                                        (uint32*) &sleep_time)) != SOC_E_NONE) {
                            sleep_time = 0;
                        }
                        if ((rv = bcm_port_phy_control_get(u,p,
                                        BCM_PORT_PHY_CONTROL_POWER_AUTO_WAKE_TIME,
                                        (uint32*) &wake_time)) != SOC_E_NONE) {
                            wake_time = 0;
                        }
                         
                    } else if (mode_value == BCM_PORT_PHY_CONTROL_POWER_LOW) {
                        str = "low";
                    } else {
                       str = "full";
                    }
                } else {
                    str = "unavail";
                }                
                printk("%5s(%3d) %16s %14s ",
                       SOC_PORT_NAME(u, p), p,
                       soc_phy_name_get(u, p),
                       str);
                if (sleep_time && wake_time) {
                    printk("%10d %14d\n", sleep_time,wake_time);
                } else {
                    printk("%10s %14s\n", "N/A","N/A");
                }
            }
            return CMD_OK;
        }

        if (sal_strcasecmp(mode_type, "auto_low") == 0) {
            (void)_phy_auto_low_start(u,pbm,1);
        } else if (sal_strcasecmp(mode_type, "auto_off") == 0) {
            (void)_phy_auto_low_start(u,pbm,0);
        } else if (sal_strcasecmp(mode_type, "low") == 0) {
            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
                (void)bcm_port_phy_control_set(u,p,BCM_PORT_PHY_CONTROL_POWER,
                                          BCM_PORT_PHY_CONTROL_POWER_LOW);
            }
        } else if (sal_strcasecmp(mode_type, "full") == 0) {
            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
                (void)bcm_port_phy_control_set(u,p,BCM_PORT_PHY_CONTROL_POWER,
                                          BCM_PORT_PHY_CONTROL_POWER_FULL);
            }
        } else if (sal_strcasecmp(mode_type, "auto_down") == 0) {
            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
                (void)bcm_port_phy_control_set(u,p,BCM_PORT_PHY_CONTROL_POWER,
                                          BCM_PORT_PHY_CONTROL_POWER_AUTO);
                if (sleep_time >= 0) {
                    (void)bcm_port_phy_control_set(u,p,
                                  BCM_PORT_PHY_CONTROL_POWER_AUTO_SLEEP_TIME,
                                          sleep_time);
                }
                if (wake_time >= 0) {
                    (void)bcm_port_phy_control_set(u,p,
                                  BCM_PORT_PHY_CONTROL_POWER_AUTO_WAKE_TIME,
                                          wake_time);
                }
            }
        }

        /* free allocated memory from arg parsing */
        parse_arg_eq_done(&pt);
        return CMD_OK;
    }
    if (c != NULL && sal_strcasecmp(c, "margin") == 0) {
        return _phy_margin(u, a);
    }

    if (c != NULL && sal_strcasecmp(c, "prbs") == 0) {
        return _phy_prbs(u, a);
    }

    if (c != NULL && sal_strcasecmp(c, "diag") == 0) {
        return _phy_diag(u, a);
    }

    if (c != NULL && sal_strcasecmp(c, "longreach") == 0) {
        cmd_result_t cmd_rv; 
        parse_table_t    pt;
        int print_header;
        uint32 flags;
        uint32 longreach_speed, longreach_pairs;
        uint32 longreach_gain, longreach_autoneg;
        uint32 longreach_local_ability, longreach_remote_ability;
        uint32 longreach_current_ability, longreach_master;
        uint32 longreach_active, longreach_enable;

        if (((c = ARG_GET(a)) == NULL) || (parse_bcm_pbmp(u, c, &pbm) < 0)) {
            printk("%s: ERROR: unrecognized port bitmap: %s\n", ARG_CMD(a), c);
            return CMD_FAIL;
        }

        {

            if (c[0] == '=') {
                return CMD_USAGE;        /* '=' unsupported */
            }

            parse_table_init(u, &pt);

            parse_table_add(&pt, "SPeed",       PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &longreach_speed, 0);
            parse_table_add(&pt, "PAirs",       PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &longreach_pairs, 0);
            parse_table_add(&pt, "GAin",       PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &longreach_gain, 0);
            parse_table_add(&pt, "AutoNeg", PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &longreach_autoneg, 0);
            parse_table_add(&pt, "LocalAbility", PQ_LR_PHYAB | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &longreach_local_ability, 0);
            parse_table_add(&pt, "RemoteAbility", PQ_LR_PHYAB | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &longreach_remote_ability, 0);
            parse_table_add(&pt, "CurrentAbility", PQ_LR_PHYAB | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &longreach_current_ability, 0);
            parse_table_add(&pt, "MAster",       PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &longreach_master, 0);
            parse_table_add(&pt, "Active",       PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &longreach_active, 0);
            parse_table_add(&pt, "ENable",       PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &longreach_enable, 0);


            if (parse_arg_eq(a, &pt) < 0) {
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }
            if (ARG_CNT(a) > 0) {
                printk("%s: Unknown argument %s\n", ARG_CMD(a), ARG_CUR(a));
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }

            flags=0;

            for (i = 0; i < pt.pt_cnt; i++) {
                if (pt.pt_entries[i].pq_type & PQ_PARSED) {
                    flags |= (1 << (SOC_PHY_CONTROL_LONGREACH_SPEED+i));
                }
            }
            /* free allocated memory from arg parsing */
            parse_arg_eq_done(&pt);



            DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {
                print_header = FALSE;

                printk("\nCurrent Longreach settings of %s ->\n", BCM_PORT_NAME(u, p)); 

                /* Read and set the longreach speed */
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_LONGREACH_SPEED,
                                   longreach_speed,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }

                /* Read and set the longreach pairs */
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_LONGREACH_PAIRS,
                                   longreach_pairs,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }

                /* Read and set the longreach gain */
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_LONGREACH_GAIN,
                                   longreach_gain,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }

                /* Read and set the longreach autoneg (LDS) */
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_LONGREACH_AUTONEG,
                                   longreach_autoneg,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }

                /* Read and set the longreach local ability */
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_LONGREACH_LOCAL_ABILITY,
                                   longreach_local_ability,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }
                /* Read the longreach remote ability */
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_LONGREACH_REMOTE_ABILITY,
                                   longreach_remote_ability,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }

                /* Read the longreach current ability (GCD - read only) */
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_LONGREACH_CURRENT_ABILITY,
                                   longreach_current_ability,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }

                /* Read and set the longreach master (when no LDS) */
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_LONGREACH_MASTER,
                                   longreach_master,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }
                /* Read and set the longreach active (LR is active - read only) */
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_LONGREACH_ACTIVE,
                                   longreach_active,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }
                /* Read and set the longreach active (Enable LR) */
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_LONGREACH_ENABLE,
                                   longreach_enable,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }

            }

        }

        return CMD_OK;
    }

    if (c != NULL && sal_strcasecmp(c, "extlb") == 0) {
        cmd_result_t cmd_rv; 
        parse_table_t    pt;
        int print_header;
        uint32 flags;
        uint32 enable;

        if (((c = ARG_GET(a)) == NULL) || (parse_bcm_pbmp(u, c, &pbm) < 0)) {
            printk("%s: ERROR: unrecognized port bitmap: %s\n", ARG_CMD(a), c);
            return CMD_FAIL;
        }

        {

            if (c[0] == '=') {
                return CMD_USAGE;        /* '=' unsupported */
            }

            parse_table_init(u, &pt);

            parse_table_add(&pt, "ENable",       PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &enable, 0);


            if (parse_arg_eq(a, &pt) < 0) {
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }

            if (ARG_CNT(a) > 0) {
                printk("%s: Unknown argument %s\n", ARG_CMD(a), ARG_CUR(a));
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }

            flags=0;

            for (i = 0; i < pt.pt_cnt; i++) {
                if (pt.pt_entries[i].pq_type & PQ_PARSED) {
                    flags |= (1 << (SOC_PHY_CONTROL_LOOPBACK_EXTERNAL+i));
                }
            }
            /* free allocated memory from arg parsing */
            parse_arg_eq_done(&pt);



            DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {
                print_header = FALSE;

                printk("\nExternal loopback plug mode setting of %s ->\n", BCM_PORT_NAME(u, p)); 

                /* Get and set the external loopback plug mode */
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_LOOPBACK_EXTERNAL,
                                   enable,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }

            }

        }

        return CMD_OK;
    }

    if (c != NULL && sal_strcasecmp(c, "clock") == 0) {
        cmd_result_t cmd_rv; 
        parse_table_t    pt;
        int print_header;
        uint32 flags;
        uint32 pri_enable, sec_enable, frequency, base, offset;

        if (((c = ARG_GET(a)) == NULL) || (parse_bcm_pbmp(u, c, &pbm) < 0)) {
            printk("%s: ERROR: unrecognized port bitmap: %s\n", ARG_CMD(a), c);
            return CMD_FAIL;
        }

        {

            if (c[0] == '=') {
                return CMD_USAGE;        /* '=' unsupported */
            }

            parse_table_init(u, &pt);

            parse_table_add(&pt, "PRImary",       PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &pri_enable, 0);
            parse_table_add(&pt, "SECondary",     PQ_BOOL | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &sec_enable, 0);
            parse_table_add(&pt, "FRequency",       PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &frequency, 0);
            parse_table_add(&pt, "BAse",       PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &base, 0);
            parse_table_add(&pt, "OFfset",       PQ_INT | PQ_DFL | PQ_NO_EQ_OPT,
                            0, &offset, 0);


            if (parse_arg_eq(a, &pt) < 0) {
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }

            if (ARG_CNT(a) > 0) {
                printk("%s: Unknown argument %s\n", ARG_CMD(a), ARG_CUR(a));
                parse_arg_eq_done(&pt);
                return CMD_USAGE;
            }

            flags=0;

            for (i = 0; i < pt.pt_cnt; i++) {
                if (pt.pt_entries[i].pq_type & PQ_PARSED) {
                    flags |= (1 << (SOC_PHY_CONTROL_CLOCK_ENABLE+i));
                }
            }
            /* free allocated memory from arg parsing */
            parse_arg_eq_done(&pt);

            DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {
                print_header = FALSE;

                printk("\nClock extraction setting of %s ->\n", BCM_PORT_NAME(u, p)); 

                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_CLOCK_ENABLE,
                                   pri_enable,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_CLOCK_SECONDARY_ENABLE,
                                   sec_enable,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_CLOCK_FREQUENCY,
                                   frequency,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_PORT_PRIMARY,
                                   base,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }
                cmd_rv = port_phy_control_update(u, p,
                                   BCM_PORT_PHY_CONTROL_PORT_OFFSET,
                                   offset,
                                   flags, &print_header);
                if (cmd_rv != CMD_OK) {
                    return cmd_rv;
                }

            }

        }

        return CMD_OK;
    }

    if (c != NULL && sal_strcasecmp(c, "wr") == 0) {
        cmd_result_t cmd_rv; 
        bcm_port_t   port;
        uint32       block;
        uint32       address;
        uint32       value;

        /* Get port */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	port = sal_ctoi(c, 0);
        if (!SOC_PORT_VALID(u, port)) {
            printk("%s: Invalid port\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        
        /* Get block */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	block = sal_ctoi(c, 0);

        /* Get address */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	address = sal_ctoi(c, 0);

        /* Get value */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	value = sal_ctoi(c, 0);

        /* Write the phy register */
        cmd_rv = bcm_port_phy_set(u, port, BCM_PORT_PHY_INTERNAL, 
                                  BCM_PORT_PHY_REG_INDIRECT_ADDR
                                  (0, block, address), value);
        return cmd_rv;
    }

    if (c != NULL && sal_strcasecmp(c, "rd_cp") == 0) {
        cmd_result_t cmd_rv; 
        bcm_port_t   port;
        uint32       block;
        uint32       address;
        uint32       value;
        uint32       rval;

        /* Get port */
	c = ARG_GET(a);
	port = sal_ctoi(c, 0);
        if (!SOC_PORT_VALID(u, port)) {
            printk("%s: Invalid port\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        
        /* Get block */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	block = sal_ctoi(c, 0);

        /* Get address */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	address = sal_ctoi(c, 0);

        /* Get compare value */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	value = sal_ctoi(c, 0);

        /* Read the phy register */
        cmd_rv = bcm_port_phy_get(u, port, BCM_PORT_PHY_INTERNAL, 
                                  BCM_PORT_PHY_REG_INDIRECT_ADDR
                                  (0, block, address), &rval);

        if (value != rval) {
            printk("Error: block %x, register %x expected %x, got %x\n", 
                   block, address, value, rval);
        } else {
            printk("Pass\n");
        }
        return cmd_rv;
    }

    if (c != NULL && sal_strcasecmp(c, "rd_cp2") == 0) {
        cmd_result_t cmd_rv; 
        bcm_port_t   port;
        uint32       block;
        uint32       address;
        uint32       value;
        uint32       mask;
        uint32       rval;

        /* Get port */
	c = ARG_GET(a);
	port = sal_ctoi(c, 0);
        if (!SOC_PORT_VALID(u, port)) {
            printk("%s: Invalid port\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        
        /* Get block */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	block = sal_ctoi(c, 0);

        /* Get address */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	address = sal_ctoi(c, 0);

        /* Get compare value */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	value = sal_ctoi(c, 0);

        /* Get mask */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	mask = sal_ctoi(c, 0);

        /* Read the phy register */
        cmd_rv = bcm_port_phy_get(u, port, BCM_PORT_PHY_INTERNAL, 
                                  BCM_PORT_PHY_REG_INDIRECT_ADDR
                                  (0, block, address), &rval);

        if ((value & mask) != (rval & mask)) {
            printk("Error: block %x, register %x expected %x, got %x\n", 
                   block, address, (value & mask), (rval & mask));
        } else {
            printk("Pass\n");
        }
        return cmd_rv;
    }

    if (c != NULL && sal_strcasecmp(c, "mod") == 0) {
        cmd_result_t cmd_rv; 
        bcm_port_t port;
        int block;
        uint32 flags;
        uint32 address;
        uint32 value;
        uint32 mask;

        /* Get port */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	port = sal_ctoi(c, 0);
        if (!SOC_PORT_VALID(u, port)) {
            printk("%s: Invalid port\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        
        /* Get block */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	block = sal_ctoi(c, 0);

        /* Get address */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	address = sal_ctoi(c, 0);

        /* Get value */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	value = sal_ctoi(c, 0);

        /* Get mask */
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
	mask = sal_ctoi(c, 0);

        /* Modify the phy register */
        flags = 0;
        if (block >= 0) {
            flags = BCM_PORT_PHY_INTERNAL;
            address = BCM_PORT_PHY_REG_INDIRECT_ADDR(0, block, address);
        }
        cmd_rv = bcm_port_phy_modify(u, port, flags, address, value, mask);

        return cmd_rv;
    }

    if (c != NULL && sal_strcasecmp(c, "control") == 0) {
        uint32       wan_mode, preemphasis, predriver_current, driver_current;
        uint32       sw_rx_los_nval = 0, sw_rx_los_oval;
        uint32       eq_boost;
        uint32       interface;
        uint32       interfacemax;
        uint32       flags;
        int          print_header;
        cmd_result_t cmd_rv; 
        bcm_error_t  bcm_rv;
        bcm_port_config_t pcfg;
        int eq_tune = FALSE;
        int eq_status = FALSE;
        int dump = FALSE;
        int farEndEqValue = 0;
#ifdef INCLUDE_MACSEC
        uint32 macsec_switch_fixed, macsec_switch_fixed_speed;
        uint32 macsec_switch_fixed_duplex, macsec_switch_fixed_pause;
        uint32 macsec_pause_rx_fwd, macsec_pause_tx_fwd;
        uint32 macsec_line_ipg, macsec_switch_ipg;

        macsec_switch_fixed = 0;
        macsec_switch_fixed_speed = 0;
        macsec_switch_fixed_duplex = 0;
        macsec_switch_fixed_pause = 0;
        macsec_pause_rx_fwd = 0;
        macsec_pause_tx_fwd = 0;
        macsec_line_ipg = 0;
        macsec_switch_ipg = 0;
#endif

        if (bcm_port_config_get(u, &pcfg) != BCM_E_NONE) {
            printk("%s: Error: bcm ports not initialized\n", ARG_CMD(a));
            return CMD_FAIL;
        }

        wan_mode          = 0;
        preemphasis       = 0;
        predriver_current = 0;
        driver_current    = 0;
        interface         = 0;
        interfacemax      = 0;
        eq_boost          = 0;
       
        if ((c = ARG_GET(a)) == NULL) {
            SOC_PBMP_ASSIGN(pbm, pcfg.port);
        } else if (parse_bcm_pbmp(u, c, &pbm) < 0) {
            printk("%s: ERROR: unrecognized port bitmap: %s\n", ARG_CMD(a), c);
            return CMD_FAIL;
        }

	BCM_PBMP_AND(pbm, pcfg.port);

        flags = 0;
        if ((c = ARG_CUR(a)) != NULL) {
            parse_table_t    pt;
            int              i;

            if (c[0] == '=') {
                return CMD_USAGE;        /* '=' unsupported */
            }
            if (sal_strcasecmp(c, "RxTune") == 0) {

                /* go to next argument */
                ARG_NEXT(a);

                /* Get far end equalization value  */
                if ((c = ARG_GET(a)) == NULL) {
                    printk("far end equalization value not input, using 0\n");
                    farEndEqValue = 0;
                } else {
                    farEndEqValue = sal_ctoi(c, 0);
                    printk("far end equalization value input (%d)\n", farEndEqValue);
                }
                eq_tune = TRUE;
            }
            if (sal_strcasecmp(c, "Dump") == 0) {
                c = ARG_GET(a); 
                dump = TRUE;
            }

            if ((eq_tune == FALSE) && (dump == FALSE)) {
                parse_table_init(u, &pt);
                parse_table_add(&pt, "WanMode", PQ_DFL|PQ_BOOL, 
                                0, &wan_mode, 0);
                parse_table_add(&pt, "Preemphasis", PQ_DFL|PQ_INT, 
                                0, &preemphasis, 0);
                parse_table_add(&pt, "DriverCurrent", PQ_DFL|PQ_INT, 
                                0, &driver_current, 0);
                parse_table_add(&pt, "PreDriverCurrent", PQ_DFL|PQ_INT, 
                            0, &predriver_current, 0);
                parse_table_add(&pt, "EqualizerBoost", PQ_DFL|PQ_INT, 
                            0, &eq_boost, 0);
                parse_table_add(&pt, "Interface", PQ_DFL|PQ_INT, 
                            0, &interface, 0);
                parse_table_add(&pt, "InterfaceMax", PQ_DFL|PQ_INT, 
                            0, &interfacemax, 0);
                parse_table_add(&pt, "SwRxLos", PQ_DFL|PQ_INT, 
                            0, &sw_rx_los_nval, 0);

#ifdef INCLUDE_MACSEC
                parse_table_add(&pt, "MacsecSwitchFixed", PQ_DFL|PQ_BOOL, 
                                0, &macsec_switch_fixed, 0);
                parse_table_add(&pt, "MacsecSwitchFixedSpeed", PQ_DFL|PQ_INT, 
                            0, &macsec_switch_fixed_speed, 0);
                parse_table_add(&pt, "MacsecSwitchFixedDuplex", PQ_DFL|PQ_BOOL, 
                            0, &macsec_switch_fixed_duplex, 0);
                parse_table_add(&pt, "MacsecSwitchFixedPause", PQ_DFL|PQ_BOOL, 
                                0, &macsec_switch_fixed_pause, 0);
                parse_table_add(&pt, "MacsecPauseRXForward", PQ_DFL|PQ_BOOL, 
                            0, &macsec_pause_rx_fwd, 0);
                parse_table_add(&pt, "MacsecPauseTXForward", PQ_DFL|PQ_BOOL, 
                                0, &macsec_pause_tx_fwd, 0);
                parse_table_add(&pt, "MacsecLineIPG", PQ_DFL|PQ_INT, 
                                0, &macsec_line_ipg, 0);
                parse_table_add(&pt, "MacsecSwitchIPG", PQ_DFL|PQ_INT, 
                                0, &macsec_switch_ipg, 0);
#endif

                if (parse_arg_eq(a, &pt) < 0) {
                    parse_arg_eq_done(&pt);
                    return CMD_USAGE;
                }
                if (ARG_CNT(a) > 0) {
                    printk("%s: Unknown argument %s\n", ARG_CMD(a), ARG_CUR(a));
                    parse_arg_eq_done(&pt);
                    return CMD_USAGE;
                }

                for (i = 0; i < pt.pt_cnt; i++) {
                    if (pt.pt_entries[i].pq_type & PQ_PARSED) {
                        flags |= (1 << i);
                    }
                }
                parse_arg_eq_done(&pt);
            }
        } 
        DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {
            print_header = TRUE;

            if (eq_tune == TRUE) {

                cmd_rv = bcm_port_control_set(u, p, bcmPortControlSerdesDriverEqualizationFarEnd, farEndEqValue);

                cmd_rv = bcm_port_control_set(u, p, 
                             bcmPortControlSerdesDriverTune, 1);
                if (cmd_rv != CMD_OK) {
                    printk("unit %d port %d Tuning function not available\n", 
                           u,p);
                    continue;
                }
                printk("Rx Equalization Tuning start\n");
                sal_usleep(1000000);
                cmd_rv = bcm_port_control_get(u, p,
                       bcmPortControlSerdesDriverEqualizationTuneStatusFarEnd, 
                                             &eq_status);

                printk("unit %d port %d Tuning done, Status: %s\n", 
                             u,p,
                             ((cmd_rv == CMD_OK) && eq_status)? "OK":"FAIL");
                continue;
            }
			if (dump == TRUE) {
				cmd_rv = bcm_port_phy_control_set(u, p, BCM_PORT_PHY_CONTROL_DUMP, 1);
				continue;
			}

            cmd_rv = port_phy_control_update(u, p, BCM_PORT_PHY_CONTROL_WAN,
                                             wan_mode, flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set WAN mode */
            cmd_rv = port_phy_control_update(u, p, BCM_PORT_PHY_CONTROL_WAN,
                                             wan_mode, flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set Preemphasis */
            cmd_rv = port_phy_control_update(u, p, 
                                             BCM_PORT_PHY_CONTROL_PREEMPHASIS,
                                             preemphasis, flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set Driver Current */ 
            cmd_rv = port_phy_control_update(u, p, 
                                          BCM_PORT_PHY_CONTROL_DRIVER_CURRENT,
                                          driver_current, flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set Pre-driver Current */
            cmd_rv = port_phy_control_update(u, p, 
                                    BCM_PORT_PHY_CONTROL_PRE_DRIVER_CURRENT,
                                    predriver_current, flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set Equalizer Boost */
            cmd_rv = port_phy_control_update(u, p, 
                                    BCM_PORT_PHY_CONTROL_EQUALIZER_BOOST,
                                    eq_boost, flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set the interface */
            cmd_rv = port_phy_control_update(u, p,
                                    BCM_PORT_PHY_CONTROL_INTERFACE,
                                    interface, flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }
            /* Read and set(is noop) the interface */
            cmd_rv = port_phy_control_update(u, p,
                                    SOC_PHY_CONTROL_INTERFACE_MAX,
                                    interfacemax, flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set(is noop) the interface */
            bcm_rv = bcm_port_phy_control_get(u, p, 
                                  BCM_PORT_PHY_CONTROL_SOFTWARE_RX_LOS, &sw_rx_los_oval);
            if (BCM_FAILURE(bcm_rv) && BCM_E_UNAVAIL != bcm_rv) {
                printk("%s\n", bcm_errmsg(bcm_rv));
                return CMD_FAIL;
            } else if (BCM_SUCCESS(bcm_rv)) {
                if ((sw_rx_los_nval != sw_rx_los_oval) && (flags & (1<<7))) { 
                    bcm_rv = bcm_port_phy_control_set(u, p, 
                                      BCM_PORT_PHY_CONTROL_SOFTWARE_RX_LOS,
                                                      sw_rx_los_nval);
                    if (BCM_FAILURE(bcm_rv)) {
                        printk("%s\n", bcm_errmsg(bcm_rv));
                        return CMD_FAIL;
                    }
                    sw_rx_los_oval = sw_rx_los_nval;
                } 
                cmd_rv = CMD_OK;
                printk("Rx LOS (s/w) enable         - %d\n", sw_rx_los_oval);
            }

#ifdef INCLUDE_MACSEC

            /* Read and set the Switch fixed */
            cmd_rv = port_phy_control_update(u, p,
                               BCM_PORT_PHY_CONTROL_MACSEC_SWITCH_FIXED,
                               macsec_switch_fixed, 
                               flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set the Switch fixed Speed */
            cmd_rv = port_phy_control_update(u, p,
                               BCM_PORT_PHY_CONTROL_MACSEC_SWITCH_FIXED_SPEED,
                               macsec_switch_fixed_speed, 
                               flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set the Switch fixed Duplex */
            cmd_rv = port_phy_control_update(u, p,
                               BCM_PORT_PHY_CONTROL_MACSEC_SWITCH_FIXED_DUPLEX,
                               macsec_switch_fixed_duplex, 
                               flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set the Switch fixed Pause */
            cmd_rv = port_phy_control_update(u, p,
                               BCM_PORT_PHY_CONTROL_MACSEC_SWITCH_FIXED_PAUSE,
                               macsec_switch_fixed_pause, 
                               flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set Pause Receive Forward */
            cmd_rv = port_phy_control_update(u, p,
                               BCM_PORT_PHY_CONTROL_MACSEC_PAUSE_RX_FORWARD,
                               macsec_pause_rx_fwd, 
                               flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set Pause transmit Forward */
            cmd_rv = port_phy_control_update(u, p,
                               BCM_PORT_PHY_CONTROL_MACSEC_PAUSE_TX_FORWARD,
                               macsec_pause_tx_fwd, 
                               flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set Line Side IPG */
            cmd_rv = port_phy_control_update(u, p,
                               BCM_PORT_PHY_CONTROL_MACSEC_LINE_IPG,
                               macsec_line_ipg, 
                               flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }

            /* Read and set Switch Side IPG */
            cmd_rv = port_phy_control_update(u, p,
                               BCM_PORT_PHY_CONTROL_MACSEC_SWITCH_IPG,
                               macsec_switch_ipg, 
                               flags, &print_header);
            if (cmd_rv != CMD_OK) {
                return cmd_rv;
            }
#endif
        }
        return CMD_OK;
    }

    /* All access to an MII register */
    if (c != NULL && sal_strcasecmp(c, "dumpall") == 0) {
        uint8 phy_addr_start = 0;
        uint8 phy_addr_end = 0xFF;

        if ((c = ARG_GET(a)) == NULL) {
            printk("%s: Error: expecting \"c45\" or \"c22\"\n", ARG_CMD(a));
            return CMD_USAGE;
        }
        is_c45 = 0;
        if (sal_strcasecmp(c, "c45") == 0) {
            is_c45 = 1;
            if (!soc_feature(u, soc_feature_phy_cl45)) {
                printk("%s: Error: Device does not support clause 45\n", ARG_CMD(a));
                return CMD_USAGE;
            }
        } else if (sal_strcasecmp(c, "c22") != 0) {
            printk("%s: Error: expecting \"c45\" or \"c22\"\n", ARG_CMD(a));
            return CMD_USAGE;
        }
        if ((c = ARG_GET(a)) != NULL) {
            char *end;

            phy_addr_start = sal_strtoul(c, &end, 0);
            if (*end) {
                printk("%s: Error: Expecting PHY start address [%s]\n", ARG_CMD(a), c);
                return CMD_USAGE;
            }
            if ((c = ARG_GET(a)) != NULL) {
                phy_addr_end = sal_strtoul(c, &end, 0);
                if (*end) {
                    printk("%s: Error: Expecting PHY end address [%s]\n", ARG_CMD(a),
                           c);
                    return CMD_USAGE;
                }
            } else {
		/* If start specified but no end, just print one phy address */
                phy_addr_end = phy_addr_start;
	    }
        }
        if (is_c45) {
            printk("%4s%5s %5s %3s: %-6s\n", "", "PRTAD", "DEVAD", "REG", "VALUE");
            for (phy_addr = phy_addr_start; phy_addr <= phy_addr_end; phy_addr++) {
		/* Clause 45 supports 32 devices per phy. */
                for (phy_devad = 0; phy_devad <= 0x1f; phy_devad++) {
		    /* Device ID is in registers 2 and 3 */
                    for (phy_reg = 2; phy_reg <= 3; phy_reg++) {
                        rv = soc_miimc45_read(u, phy_addr, phy_devad,
                                              phy_reg, &phy_data);
                        if (rv < 0) {
                            printk("ERROR: MII Addr %d: soc_miim_read failed: %s\n",
                                   phy_addr, soc_errmsg(rv));
                            return CMD_FAIL;
                        }
			/* Assume device doesn't exist in phy if read back is all 0/1 */
                        if ((phy_data != 0xFFFF) && (phy_data != 0x0000)) {
                            printk("%4s 0x%02X 0x%02X 0x%02X: 0x%04X\n",
                                   "", phy_addr, phy_devad, phy_reg, phy_data);
                        }
                    }
                }
            }
        } else {
            printk("%4s%5s %3s: %-6s\n", "", "PRTAD", "REG", "VALUE");
            for (phy_addr = phy_addr_start; phy_addr <= phy_addr_end; phy_addr++) {
		/* Device ID is in registers 2 and 3 */
                for (phy_reg = 2; phy_reg <= 3; phy_reg++) {
                    rv = soc_miim_read(u, phy_addr, phy_reg, &phy_data);
                    if (rv < 0) {
                        printk("ERROR: MII Addr %d: soc_miim_read failed: %s\n",
                               phy_addr, soc_errmsg(rv));
                        return CMD_FAIL;
                    }
                    if ((phy_data != 0xFFFF) && (phy_data != 0x0000)) {
                        printk("%4s0x%02X 0x%02x: 0x%04x\n",
                               "", phy_addr, phy_reg, phy_data);
                    }
                }
            }
        }
        return CMD_OK;
    }

    /* Raw access to an MII register */
    if (c != NULL && sal_strcasecmp(c, "raw") == 0) {
        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
  
        if (soc_feature(u, soc_feature_phy_cl45)) {
            if (sal_strcasecmp(c, "c45") == 0) {
                is_c45 = 1;
                if ((c = ARG_GET(a)) == NULL) {
                    return CMD_USAGE;
                }
            }
        }
        phy_addr = sal_strtoul(c, NULL, 0);
        if ((c = ARG_GET(a)) == NULL) { /* Get register number */
            return CMD_USAGE;
        }
        if (is_c45) {
            phy_devad = sal_strtoul(c, NULL, 0);
            if (phy_devad > 0x1f) { 
                printk("ERROR: Invalid devad 0x%x, max=0x%x\n", 
                phy_devad, 0x1f); 
                return CMD_FAIL; 
            } 
            if ((c = ARG_GET(a)) == NULL) { /* Get register number */
                return CMD_USAGE;
            }
        }
        phy_reg = sal_strtoul(c, NULL, 0);
        if ((c = ARG_GET(a)) == NULL) { /* Read register */
            if (is_c45) {
                rv = soc_miimc45_read(u, phy_addr, phy_devad, phy_reg, &phy_data);
                if (rv < 0) {
                    printk("ERROR: MII Addr %d: soc_miim_read failed: %s\n",
                           phy_addr, soc_errmsg(rv));
                    return CMD_FAIL;
                }
            } else {
                rv = soc_miim_read(u, phy_addr, phy_reg, &phy_data);
                if (rv < 0) {
                    printk("ERROR: MII Addr %d: soc_miim_read failed: %s\n",
                           phy_addr, soc_errmsg(rv));
                    return CMD_FAIL;
                }
            }
            var_set_hex("phy_reg_data", phy_data, TRUE, FALSE); 
            printk("%s\t0x%02x: 0x%04x\n", "", phy_reg, phy_data);
        } else { /* write */
            phy_data = sal_strtoul(c, NULL, 0);
            if (is_c45) {
                rv = soc_miimc45_write(u, phy_addr, phy_devad, phy_reg, phy_data);
            } else {
                rv = soc_miim_write(u, phy_addr, phy_reg, phy_data);
            }
            if (rv < 0) {
                printk("ERROR: MII Addr %d: soc_miim_write failed: %s\n",
                       phy_addr, soc_errmsg(rv));
                return CMD_FAIL;
            }
        }
        return CMD_OK;
    }

#if defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT)
    if (c != NULL && sal_strcasecmp(c, "rcpu") == 0) {
        is_rcpu = 1;
        c = ARG_GET(a);
    }
#endif /* defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT) */

    if (c != NULL && sal_strcasecmp(c, "int") == 0) {
        intermediate = 1;
        c = ARG_GET(a);
    }

    if (c == NULL) {
        return(CMD_USAGE);
    }

    /* Parse the bitmap. */
    if (parse_pbmp(u, c, &pbm) < 0) {
        printk("%s: ERROR: unrecognized port bitmap: %s\n",
               ARG_CMD(a), c);
        return CMD_FAIL;
    }

    SOC_PBMP_ASSIGN(pbm_phys, pbm);
    SOC_PBMP_AND(pbm_phys, PBMP_PORT_ALL(u));
    if (SOC_PBMP_IS_NULL(pbm_phys)) {
        printk("Ports specified do not have PHY drivers.\n");
    } else {
	SOC_PBMP_ASSIGN(pbm_temp, pbm);
	SOC_PBMP_REMOVE(pbm_temp, PBMP_PORT_ALL(u));
	if (SOC_PBMP_NOT_NULL(pbm_temp)) {
        printk("Not all ports given have PHY drivers.  Using %s\n",
               SOC_PBMP_FMT(pbm_phys, pfmt));
	}
    }
    SOC_PBMP_ASSIGN(pbm, pbm_phys);

    if (ARG_CNT(a) == 0) {	/*  show information for all registers */
        DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
	    phy_addr = (intermediate ?
			PORT_TO_PHY_ADDR_INT(u, p) :
			PORT_TO_PHY_ADDR(u, p));
	    if (phy_addr == 0xff) {
		printk("Port %s: No %sPHY address assigned\n",
		       SOC_PORT_NAME(u, p),
		       intermediate ? "intermediate " : "");
		continue;
	    }
	    if (intermediate) {
		printk("Port %s (intermediate PHY addr 0x%02x):",
		       SOC_PORT_NAME(u, p), phy_addr);
	    } else {
                int ap = p;
                BCM_API_XLATE_PORT_P2A(u, &ap); /* Use BCM API port */
                BCM_IF_ERROR_RETURN(bcm_port_phy_drv_name_get(u, ap, drv_name, 64));
                printk("Port %s (PHY addr 0x%02x): %s (%s)",
		       SOC_PORT_NAME(u, p), phy_addr,
		       soc_phy_name_get(u, p), drv_name);
	    }
            if (soc_phy_is_c45_miim(u, p) && (!intermediate)) {
                for(i = 0; i < COUNTOF(p_devad); i++) {
                    phy_devad = p_devad[i];
                    printk("\nDevAd = %d(%s)", phy_devad, p_devad_str[i]);
                    for (phy_reg = PHY_MIN_REG; phy_reg <= PHY_MAX_REG; phy_reg++) {
#if defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT)
                        if (1 == is_rcpu) {
                            rv = soc_rcpu_miimc45_read(u, phy_addr,
                                              phy_devad, phy_reg, &phy_data);
                        } else
#endif /* defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT) */
                        if (EXT_PHY_SW_STATE(u, p) && 
                            (EXT_PHY_SW_STATE(u, p)->read)) {
                            rv = EXT_PHY_SW_STATE(u, p)->read(u, phy_addr,
                                  BCM_PORT_PHY_CLAUSE45_ADDR(phy_devad,phy_reg),
                                  &phy_data);
                        } else {
                            rv = soc_miimc45_read(u, phy_addr,
                                              phy_devad, phy_reg, &phy_data);
                        }

                        if (rv < 0) {
                            printk("\nERROR: Port %s: soc_miim_read failed: %s\n",
                                   SOC_PORT_NAME(u, p), soc_errmsg(rv));
                            rv = CMD_FAIL;
                            goto done;
                        }
                        printk("%s\t0x%04x: 0x%04x",
                               ((phy_reg % DUMP_PHY_COLS) == 0) ? "\n" : "",
                               phy_reg, phy_data);
                    }
                }
            } else {
                for (phy_reg = PHY_MIN_REG; phy_reg <= PHY_MAX_REG; phy_reg++) {
#if defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT)
                    if (1 == is_rcpu) {
                        rv = soc_rcpu_miim_read(u, phy_addr, phy_reg, &phy_data);
                    } else
#endif /* defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT) */
                    if (!intermediate) {
                        if (EXT_PHY_SW_STATE(u, p) && 
                            (EXT_PHY_SW_STATE(u, p)->read)) {
                            rv = EXT_PHY_SW_STATE(u, p)->read(u, phy_addr, phy_reg,
                                                         &phy_data);
                        } else {
                            rv = soc_miim_read(u, phy_addr, phy_reg, &phy_data);
                        }
                    } else {
                        if (INT_PHY_SW_STATE(u, p) &&
                            (INT_PHY_SW_STATE(u, p)->read)) {
                            rv = INT_PHY_SW_STATE(u, p)->read(u, phy_addr, phy_reg,
                                                &phy_data);
                        } else {
                            rv = soc_miim_read(u, phy_addr, phy_reg, &phy_data);
                        }
                    }
                    if (rv < 0) {
                        printk("\nERROR: Port %s: soc_miim_read failed: %s\n",
                               SOC_PORT_NAME(u, p), soc_errmsg(rv));
                        rv = CMD_FAIL;
                        goto done;
                    }
                    printk("%s\t0x%02x: 0x%04x",
                           ((phy_reg % DUMP_PHY_COLS) == 0) ? "\n" : "",
                           phy_reg, phy_data);
                }
            }
	    printk("\n");
        }
    } else {				/* get register argument */
	c = ARG_GET(a);
	phy_reg = sal_ctoi(c, 0);

	if (ARG_CNT(a) == 0) {		/* no more args; show this register */
            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
		phy_addr = (intermediate ?
			    PORT_TO_PHY_ADDR_INT(u, p) :
			    PORT_TO_PHY_ADDR(u, p));
		if (phy_addr == 0xff) {
		    printk("Port %s: No %sPHY address assigned\n",
			   SOC_PORT_NAME(u, p),
			   intermediate ? "intermediate " : "");
		    continue;
		}
        if ((!intermediate) && (soc_phy_is_c45_miim(u, p))) {
            for(i = 0; i < COUNTOF(p_devad); i++) {
                phy_devad = p_devad[i];
                printk("Port %s (PHY addr 0x%02x) DevAd %d(%s) Reg 0x%04x: ",
                       SOC_PORT_NAME(u, p), phy_addr, phy_devad,
                       p_devad_str[i], phy_reg);
#if defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT)
                if (1 == is_rcpu) {
                    rv = soc_rcpu_miimc45_read(u, phy_addr,
                                      phy_devad, phy_reg, &phy_data);
                } else
#endif /* defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT) */
                if (EXT_PHY_SW_STATE(u, p) &&
                    (EXT_PHY_SW_STATE(u, p)->read)) {
                    rv = EXT_PHY_SW_STATE(u, p)->read(u, phy_addr, 
                                   BCM_PORT_PHY_CLAUSE45_ADDR(phy_devad,phy_reg),
                                   &phy_data);
                } else {
                    rv = soc_miimc45_read(u, phy_addr,
                                      phy_devad, phy_reg, &phy_data);
                }

                if (rv < 0) {
                    printk("\nERROR: Port %s: soc_miim_read failed: %s\n",
                       SOC_PORT_NAME(u, p), soc_errmsg(rv));
                    rv = CMD_FAIL;
                    goto done;
                }
                var_set_hex("phy_reg_data", phy_data, TRUE, FALSE); 
                printk("0x%04x\n", phy_data);
            }
        } else {
            printk("Port %s (PHY addr 0x%02x) Reg %d: ",
                       SOC_PORT_NAME(u, p), phy_addr, phy_reg);
#if defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT)
            if (1 == is_rcpu) {
                rv = soc_rcpu_miim_read(u, phy_addr, phy_reg, &phy_data);
            } else
#endif /* defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT) */
            if (!intermediate) {
                if (EXT_PHY_SW_STATE(u, p) &&
                    (EXT_PHY_SW_STATE(u, p)->read)) {
                    rv = EXT_PHY_SW_STATE(u, p)->read(u, phy_addr, phy_reg,                                                          &phy_data);
                } else {
                    rv = soc_miim_read(u, phy_addr, phy_reg, &phy_data);
                }
            } else {
                if (INT_PHY_SW_STATE(u, p) &&
                    (INT_PHY_SW_STATE(u, p)->read)) {
                    rv = INT_PHY_SW_STATE(u, p)->read(u, phy_addr, phy_reg,                                                 &phy_data);
                } else {
                    rv = soc_miim_read(u, phy_addr, phy_reg, &phy_data);
                }
            }

            if (rv < 0) {
                printk("\nERROR: Port %s: soc_miim_read failed: %s\n",
                   SOC_PORT_NAME(u, p), soc_errmsg(rv));
                rv = CMD_FAIL;
                goto done;
            }
            var_set_hex("phy_reg_data", phy_data, TRUE, FALSE); 
            printk("0x%04x\n", phy_data);
        }
        }
        } else {	/* set the reg to given value for the indicated phys */
            c = ARG_GET(a);
            phy_data = phy_devad = sal_ctoi(c, 0);

            DPORT_SOC_PBMP_ITER(u, pbm, dport, p) {
		phy_addr = (intermediate ?
			    PORT_TO_PHY_ADDR_INT(u, p) :
			    PORT_TO_PHY_ADDR(u, p));
		if (phy_addr == 0xff) {
		    printk("Port %s: No %sPHY address assigned\n",
			   SOC_PORT_NAME(u, p),
			   intermediate ? "intermediate " : "");
		    rv = CMD_FAIL;
		    goto done;
		}
        if ((!intermediate) && (soc_phy_is_c45_miim(u, p))) {
            for(i = 0; i < COUNTOF(p_devad); i++) {
                if (phy_devad == p_devad[i]) {
                    break;
                }
            }
            if (i >= COUNTOF(p_devad)) {
                printk("\nERROR: Port %s: Invalid DevAd %d\n",
                   SOC_PORT_NAME(u, p), phy_devad);
                rv = CMD_FAIL;
                continue;
            }
            if (ARG_CNT(a) == 0) {	/* no more args; show this register */
            printk("Port %s (PHY addr 0x%02x) DevAd %d(%s) Reg 0x%04x: ",
                   SOC_PORT_NAME(u, p), phy_addr, phy_devad,
                   p_devad_str[i], phy_reg);
#if defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT)
            if (1 == is_rcpu) {
                rv = soc_rcpu_miimc45_read(u, phy_addr,
                              phy_devad, phy_reg, &phy_data);
            } else
#endif /* defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT) */
            if (EXT_PHY_SW_STATE(u, p) &&
                (EXT_PHY_SW_STATE(u, p)->read)) {
                rv = EXT_PHY_SW_STATE(u, p)->read(u, phy_addr, 
                                BCM_PORT_PHY_CLAUSE45_ADDR(phy_devad,phy_reg),
                                         &phy_data);
            } else {
                rv = soc_miimc45_read(u, phy_addr,
                                  phy_devad, phy_reg, &phy_data);
            }

            if (rv < 0) {
                printk("\nERROR: Port %s: soc_miim_read failed: %s\n",
                   SOC_PORT_NAME(u, p), soc_errmsg(rv));
                rv = CMD_FAIL;
                goto done;
            }
            var_set_hex("phy_reg_data", phy_data, TRUE, FALSE); 
            printk("0x%04x\n", phy_data);
            } else { /* write */
                c = ARG_GET(a);
                phy_data = sal_ctoi(c, 0);
#if defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT)
                if (1 == is_rcpu) {
                    rv = soc_rcpu_miimc45_write(u, phy_addr,
                                          phy_devad, phy_reg, phy_data);
                } else
#endif /* defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT) */
                if (EXT_PHY_SW_STATE(u, p) &&
                    (EXT_PHY_SW_STATE(u, p)->write)) {
                    rv = EXT_PHY_SW_STATE(u, p)->write(u, phy_addr, 
                                BCM_PORT_PHY_CLAUSE45_ADDR(phy_devad,phy_reg),
                                            phy_data);
                } else {
                    rv = soc_miimc45_write(u, phy_addr,
                                      phy_devad, phy_reg, phy_data);
                }

                if (rv < 0) {
                    printk("ERROR: Port %s: soc_miim_write failed: %s\n",
                       SOC_PORT_NAME(u, p), soc_errmsg(rv));
                    rv = CMD_FAIL;
                    goto done;
                }
            }
        } else {
#if defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT)
                if (1 == is_rcpu) {
                    rv = soc_rcpu_miim_write(u, phy_addr, phy_reg, phy_data);
                } else
#endif /* defined(BCM_RCPU_SUPPORT) && defined(BCM_CMICM_SUPPORT) */
                if (!intermediate) {
                    if (EXT_PHY_SW_STATE(u, p) &&
                        (EXT_PHY_SW_STATE(u, p)->write)) {
                        rv = EXT_PHY_SW_STATE(u, p)->write(u, phy_addr, phy_reg,
                                                     phy_data);
                    } else {
                        rv = soc_miim_write(u, phy_addr, phy_reg, phy_data);                              }
                } else {
                    if (INT_PHY_SW_STATE(u, p) &&
                        (INT_PHY_SW_STATE(u, p)->write)) {
                        rv = INT_PHY_SW_STATE(u, p)->write(u, phy_addr, phy_reg,
                                            phy_data);
                    } else {
                        rv = soc_miim_write(u, phy_addr, phy_reg, phy_data);
                    }
                }
		if (rv < 0) {
		    printk("ERROR: Port %s: soc_miim_write failed: %s\n",
			   SOC_PORT_NAME(u, p), soc_errmsg(rv));
		    rv = CMD_FAIL;
		    goto done;
		}
	    }
        }
	}
    }

 done:
    return rv;
}

/***********************************************************************
 *
 * Combo port support
 *
 ***********************************************************************/


/*
 * Function:	if_combo_dump
 * Purpose:	Dump the contents of a bcm_phy_config_t
 */

STATIC int
if_combo_dump(args_t *a, int u, int p, int medium)
{
    char		pm_str[80];
    bcm_port_medium_t   active_medium;
    int			r;
    bcm_phy_config_t	cfg;

    /*
     * Get active medium so we can put an asterisk next to the status if
     * it is active.
     */

    if ((r = bcm_port_medium_get(u, p, &active_medium)) < 0) {
	return r;
    }
    if ((r = bcm_port_medium_config_get(u, p, medium, &cfg)) < 0) {
	return r;
    }

    printk("%s:\t%s medium%s\n",
	   BCM_PORT_NAME(u, p),
	   MEDIUM_STATUS(medium),
	   (medium == active_medium) ? " (active)" : "");

    format_port_mode(pm_str, sizeof (pm_str), cfg.autoneg_advert, TRUE);

    printk("\tenable=%d preferred=%d "
	   "force_speed=%d force_duplex=%d master=%s\n",
	   cfg.enable, cfg.preferred,
	   cfg.force_speed, cfg.force_duplex,
	   PHYMASTER_MODE(cfg.master));
    printk("\tautoneg_enable=%d autoneg_advert=%s(0x%x)\n",
	   cfg.autoneg_enable, pm_str, cfg.autoneg_advert);
    printk("\tMDIX=%s\n",
	   MDIX_MODE(cfg.mdix));

    return BCM_E_NONE;
}

static int combo_watch[SOC_MAX_NUM_DEVICES][SOC_MAX_NUM_PORTS];

static void
if_combo_watch(int unit, bcm_port_t port, bcm_port_medium_t medium, void *arg) 
{
    printk("Unit %d: %s: Active medium switched to %s\n",
           unit, BCM_PORT_NAME(unit, port), MEDIUM_STATUS(medium));

    /* 
     * Increment the number of medium changes. Remember, that we pass the 
     * address of combo_watch[unit][port] when we register this callback
     */
    (*((int *)arg))++; 
}

/*
 * Function: 	if_combo
 * Purpose:	Control combo ports
 * Parameters:	u - SOC unit #
 *		a - pointer to args
 * Returns:	CMD_OK/CMD_FAIL/
 */
cmd_result_t
if_esw_combo(int u, args_t *a)
{
    pbmp_t		pbm;
    soc_port_t		p, dport;
    int			specified_medium = BCM_PORT_MEDIUM_COUNT;
    int			r, rc, rf;
    char		*c;
    parse_table_t	pt;
    bcm_phy_config_t	cfg, cfg_opt;

    enum if_combo_cmd_e {
        COMBO_CMD_DUMP,
        COMBO_CMD_SET,
        COMBO_CMD_WATCH,
        CONBO_CMD_COUNT
    } cmd;

    enum if_combo_watch_arg_e {
        COMBO_CMD_WATCH_SHOW,
        COMBO_CMD_WATCH_ON,
        COMBO_CMD_WATCH_OFF,
        COMBO_CMD_WATCH_COUNT
    } watch_arg = COMBO_CMD_WATCH_SHOW;

    cfg_opt.enable = -1;
    cfg_opt.preferred = -1;
    cfg_opt.autoneg_enable = -1;
    cfg_opt.autoneg_advert = 0xffffffff;
    cfg_opt.force_speed = -1;
    cfg_opt.force_duplex = -1;
    cfg_opt.master = -1;
    cfg_opt.mdix = -1;

    if (!sh_check_attached(ARG_CMD(a), u)) {
	return CMD_FAIL;
    }

    if ((c = ARG_GET(a)) == NULL) {
	return CMD_USAGE;
    }

    if (parse_bcm_pbmp(u, c, &pbm) < 0) {
	printk("%s: ERROR: unrecognized port bitmap: %s\n",
	       ARG_CMD(a), c);
	return CMD_FAIL;
    }

    SOC_PBMP_AND(pbm, PBMP_PORT_ALL(u));

    c = ARG_GET(a);		/* NULL or media type or command */

    if (c == NULL) {
        cmd = COMBO_CMD_DUMP;
        specified_medium = BCM_PORT_MEDIUM_COUNT;
    } else if (sal_strcasecmp(c, "copper") == 0 || 
               sal_strcasecmp(c, "c") == 0) {
        cmd = COMBO_CMD_SET;
        specified_medium = BCM_PORT_MEDIUM_COPPER;
    } else if (sal_strcasecmp(c, "fiber") == 0 || 
               sal_strcasecmp(c, "f") == 0) {
        cmd = COMBO_CMD_SET;
        specified_medium = BCM_PORT_MEDIUM_FIBER;
    } else if (sal_strcasecmp(c, "watch") == 0 || 
               sal_strcasecmp(c, "w") == 0) {
        cmd = COMBO_CMD_WATCH;
    } else {
	    return CMD_USAGE;
    }

    switch (cmd) {
    case COMBO_CMD_SET:
	if ((c = ARG_CUR(a)) != NULL) {
	    if (c[0] == '=') {
		return CMD_USAGE;	/* '=' unsupported */
	    }

	    parse_table_init(u, &pt);
	    parse_table_add(&pt, "Enable", PQ_DFL|PQ_BOOL, 0,
			    &cfg_opt.enable, 0);
	    parse_table_add(&pt, "PREFerred", PQ_DFL|PQ_BOOL, 0,
			    &cfg_opt.preferred, 0);
	    parse_table_add(&pt, "Autoneg_Enable", PQ_DFL|PQ_BOOL, 0,
			    &cfg_opt.autoneg_enable, 0);
	    parse_table_add(&pt, "Autoneg_Advert", PQ_DFL|PQ_PORTMODE, 0,
			    &cfg_opt.autoneg_advert, 0);
	    parse_table_add(&pt, "Force_Speed", PQ_DFL|PQ_INT, 0,
			    &cfg_opt.force_speed, 0);
	    parse_table_add(&pt, "Force_Duplex", PQ_DFL|PQ_BOOL, 0,
			    &cfg_opt.force_duplex, 0);
	    parse_table_add(&pt, "MAster", PQ_DFL|PQ_BOOL, 0,
			    &cfg_opt.master, 0);
            parse_table_add(&pt, "MDIX", PQ_DFL|PQ_MULTI, 0,
                            &cfg_opt.mdix, mdix_mode);

	    if (parse_arg_eq(a, &pt) < 0) {
		parse_arg_eq_done(&pt);
		return CMD_USAGE;
	    }
	    parse_arg_eq_done(&pt);

	    if (ARG_CUR(a) != NULL) {
		return CMD_USAGE;
	    }
	} else {
            cmd = COMBO_CMD_DUMP;
        }

        break;
        
    case COMBO_CMD_WATCH:
        c = ARG_GET(a);

        if (c == NULL) {
            watch_arg = COMBO_CMD_WATCH_SHOW;
        } else if (sal_strcasecmp(c, "on") == 0) {
            watch_arg = COMBO_CMD_WATCH_ON;
        } else if (sal_strcasecmp(c, "off") == 0) {
            watch_arg = COMBO_CMD_WATCH_OFF;
        } else {
            return CMD_USAGE;
        }
        break;

    default:
        break;
    }

    DPORT_BCM_PBMP_ITER(u, pbm, dport, p) {
	switch (cmd) {
        case COMBO_CMD_DUMP:
	    printk("Port %s:\n", BCM_PORT_NAME(u, p));
            
            rc = rf = BCM_E_UNAVAIL;
            if (specified_medium == BCM_PORT_MEDIUM_COPPER ||
                specified_medium == BCM_PORT_MEDIUM_COUNT) {
                rc = if_combo_dump(a, u, p, BCM_PORT_MEDIUM_COPPER);
                if (rc != BCM_E_NONE && rc != BCM_E_UNAVAIL) {
                    printk("%s:\tERROR(copper): %s\n",
                           BCM_PORT_NAME(u, p),
                           bcm_errmsg(rc));
                }
            } 

            if (specified_medium == BCM_PORT_MEDIUM_FIBER ||
                specified_medium == BCM_PORT_MEDIUM_COUNT) {
                rf = if_combo_dump(a, u, p, BCM_PORT_MEDIUM_FIBER);
                if (rf != BCM_E_NONE && rf != BCM_E_UNAVAIL) {
                    printk("%s:\tERROR(fiber): %s\n",
                           BCM_PORT_NAME(u, p),
                           bcm_errmsg(rf));
                }
            }

            /*
             * If there were problems getting medium-specific info on 
             * individual mediums, then they will be printed above. However,
             * if BCM_E_UNAVAIL is returned for both copper and fiber mediums
             * we'll print only one error message
             */
            if (rc == BCM_E_UNAVAIL && rf == BCM_E_UNAVAIL) {
                printk("%s:\tmedium info unavailable\n",
                       BCM_PORT_NAME(u, p));
            } 
           break;

        case COMBO_CMD_SET:
            /*
             * Update the medium operating mode.
             */
            r = bcm_port_medium_config_get(u, p,
                                           specified_medium,
                                           &cfg);

            if (r < 0) {
                printk("%s: port %s: Error getting medium config: %s\n",
                       ARG_CMD(a), BCM_PORT_NAME(u, p), bcm_errmsg(r));
                return CMD_FAIL;
            }

            if (cfg_opt.enable != -1) {
                cfg.enable = cfg_opt.enable;
            }
            
            if (cfg_opt.preferred != -1) {
                cfg.preferred = cfg_opt.preferred;
            }
            
            if (cfg_opt.autoneg_enable != -1) {
                cfg.autoneg_enable = cfg_opt.autoneg_enable;
            }
            
            if (cfg_opt.autoneg_advert != 0xffffffff) {
                cfg.autoneg_advert = cfg_opt.autoneg_advert;
            }
            
            if (cfg_opt.force_speed != -1) {
                cfg.force_speed = cfg_opt.force_speed;
            }
            
            if (cfg_opt.force_duplex != -1) {
                cfg.force_duplex = cfg_opt.force_duplex;
            }
            
            if (cfg_opt.master != -1) {
                cfg.master = cfg_opt.master;
            }
            
            if (cfg_opt.mdix != -1) {
                cfg.mdix = cfg_opt.mdix;
            }
            
            r = bcm_port_medium_config_set(u, p,
                                           specified_medium,
                                           &cfg);
        
            if (r < 0) {
                printk("%s: port %s: Error setting medium config: %s\n",
                       ARG_CMD(a), BCM_PORT_NAME(u, p), bcm_errmsg(r));
                return CMD_FAIL;
            }

            break;

        case COMBO_CMD_WATCH:
            switch (watch_arg) {
            case COMBO_CMD_WATCH_SHOW:
                if (combo_watch[u][p]) {
                    printk("Port %s: Medium status change watch is  ON. "
                           "Medim changed %d times\n",
                           BCM_PORT_NAME(u, p),
                           combo_watch[u][p] - 1);
                } else {
                    printk("Port %s: Medium status change watch is OFF.\n",
                            BCM_PORT_NAME(u, p));
                }
                break;

            case COMBO_CMD_WATCH_ON:
                if (!combo_watch[u][p]) {
                    r = bcm_port_medium_status_register(u, p, 
                                                        if_combo_watch,
                                                        &combo_watch[u][p]);
                    if (r < 0) {
                        printk("Error registerinig medium status change "
                               "callback for %s: %s\n",
                               BCM_PORT_NAME(u, p), soc_errmsg(r));
                        return (CMD_FAIL);
                    }

                    combo_watch[u][p] = 1;
                }

                printk("Port %s: Medium change watch is ON\n",
                       BCM_PORT_NAME(u, p));

                break;

            case COMBO_CMD_WATCH_OFF:
                if (combo_watch[u][p]) {
                    r = bcm_port_medium_status_unregister(u, p, 
                                                          if_combo_watch,
                                                          &combo_watch[u][p]);
                    if (r < 0) {
                        printk("Error unregisterinig medium status change "
                               "callback for %s: %s\n",
                               BCM_PORT_NAME(u, p), soc_errmsg(r));
                        return (CMD_FAIL);
                    }

                    combo_watch[u][p] = 0;
                }

                printk("Port %s: Medium change watch is OFF\n",
                       BCM_PORT_NAME(u, p));

                break;

            default:
                return CMD_FAIL;
            }
           
            break;

        default:
            return CMD_FAIL;
        }
    }

    return CMD_OK;
}

/*
 * Function:
 *	cmd_cablediag
 * Purpose:
 *	Run cable diagnostics (if available)
 */

cmd_result_t
cmd_esw_cablediag(int unit, args_t *a)
{
    char	*s;
    bcm_pbmp_t	pbm;
    bcm_port_t	port, dport;
    int		rv, i;
    bcm_port_cable_diag_t	cd;
    char	*statename[] = _SHR_PORT_CABLE_STATE_NAMES_INITIALIZER;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if ((s = ARG_GET(a)) == NULL) {
	return CMD_USAGE;
    }

    if (parse_bcm_pbmp(unit, s, &pbm) < 0) {
        printk("%s: ERROR: unrecognized port bitmap: %s\n",
               ARG_CMD(a), s);
        return CMD_FAIL;
    }

    sal_memset(&cd, 0, sizeof(bcm_port_cable_diag_t));

    DPORT_BCM_PBMP_ITER(unit, pbm, dport, port) {
        rv = bcm_port_cable_diag(unit, port, &cd);
        if (rv < 0) {
            printk("%s: ERROR: port %s: %s\n",
                    ARG_CMD(a), BCM_PORT_NAME(unit, port), bcm_errmsg(rv));
            continue;
        }
        if (cd.fuzz_len == 0) {
            printk("port %s: cable (%d pairs)\n",
                    BCM_PORT_NAME(unit, port), cd.npairs);
        } else {
            printk("port %s: cable (%d pairs, length +/- %d meters)\n",
                    BCM_PORT_NAME(unit, port), cd.npairs, cd.fuzz_len);
        }
        for (i = 0; i < cd.npairs; i++) {
            printk("\tpair %c %s, length %d meters\n",
                    'A' + i, statename[cd.pair_state[i]], cd.pair_len[i]);
        }
    }

    return CMD_OK;
}

/* Must stay in sync with bcm_color_t enum (bcm/types.h) */
const char *diag_parse_color[] = {
    "Green",
    "Yellow",
    "Red",
    NULL
};

char cmd_color_usage[] =
    "Usages:\n\t"
    "  color set Port=<port> Prio=<prio> CFI=<cfi>\n\t"
    "        Color=<Green|Yellow|Red>\n\t"
    "  color show Port=<port>\n\t"
    "  color map Port=<port> PktPrio=<prio> CFI=<cfi>\n\t"
    "        IntPrio=<prio> Color=<Green|Yellow|Red>\n\t"
    "  color unmap Port=<port> IntPrio=<prio> Color=<Green|Yellow|Red>\n\t"
    "        PktPrio=<prio> CFI=<cfi>\n";

cmd_result_t
cmd_color(int unit, args_t *a)
{
    int                 port = 0, prio = -1, cfi = -1, color_parse = bcmColorRed;
    bcm_color_t         color;
    char 		*subcmd;
    int 		r;
    parse_table_t	pt;
    cmd_result_t	retCode;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if ((subcmd = ARG_GET(a)) == NULL) {
        return CMD_USAGE;
    }

    if (sal_strcasecmp(subcmd, "set") == 0) {

        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Port", PQ_DFL|PQ_PORT, 0, &port, NULL);
        parse_table_add(&pt, "PRio", PQ_INT|PQ_DFL, 0, &prio, NULL);
        parse_table_add(&pt, "CFI", PQ_INT|PQ_DFL, 0, &cfi, NULL);
        parse_table_add(&pt, "Color", PQ_MULTI|PQ_DFL, 0,
                        &color_parse, diag_parse_color);

        if (!parseEndOk( a, &pt, &retCode)) {
            return retCode;
        }
        if (!SOC_PORT_VALID(unit,port)) {
            printk("%s: ERROR: Invalid port selection %d\n",
                   ARG_CMD(a), port);
            return CMD_FAIL;
        }

        color = (bcm_color_t) color_parse;

        if (prio < 0) {
            if (cfi < 0) {
                /* No selection to assign color */
                printk("%s: ERROR: No parameter to assign color\n",
                       ARG_CMD(a));
                return CMD_FAIL;
            } else {
                if ((r = bcm_port_cfi_color_set(unit, port,
                                                cfi, color)) < 0) {
                    goto bcm_err;
                }
            }
        } else {
            if (cfi < 0) {
                if (prio > BCM_PRIO_MAX) {
                    printk("%s: ERROR: Priority %d exceeds maximum\n",
                           ARG_CMD(a), prio);
                    return CMD_FAIL;
                } else {
                    if ((r = bcm_port_priority_color_set(unit, port, prio,
                                                         color)) < 0) {
                        goto bcm_err;
                    }
                }
            }

        }
        return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "show") == 0) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Port", PQ_DFL|PQ_PORT, 0, &port, NULL);
        if (!parseEndOk( a, &pt, &retCode)) {
            return retCode;
        }
        if (!SOC_PORT_VALID(unit,port)) {
            printk("%s: ERROR: Invalid port selection %d\n",
                   ARG_CMD(a), port);
            return CMD_FAIL;
        }

        printk("Color settings for port %s\n", BCM_PORT_NAME(unit, port));
        for (prio = BCM_PRIO_MIN; prio <= BCM_PRIO_MAX; prio++) {
            if ((r = bcm_port_priority_color_get(unit, port, prio,
                                                 &color)) < 0) {
                goto bcm_err;
            }
            printk("Priority %d\t%s\n", prio, diag_parse_color[color]);
        }

        if ((r = bcm_port_cfi_color_get(unit, port, FALSE, &color)) < 0) {
            goto bcm_err;
        }
        printk("No CFI     \t%s\n", diag_parse_color[color]);

        if ((r = bcm_port_cfi_color_get(unit, port, TRUE, &color)) < 0) {
            goto bcm_err;
        }
        printk("CFI        \t%s\n", diag_parse_color[color]);
        return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "map") == 0) {
        int pkt_prio, int_prio;

        pkt_prio = int_prio = -1;

        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Port", PQ_DFL|PQ_PORT, 0, &port, NULL);
        parse_table_add(&pt, "PktPrio", PQ_INT|PQ_DFL, 0, &pkt_prio, NULL);
        parse_table_add(&pt, "CFI", PQ_INT|PQ_DFL, 0, &cfi, NULL);
        parse_table_add(&pt, "IntPrio", PQ_INT|PQ_DFL, 0, &int_prio, NULL);
        parse_table_add(&pt, "Color", PQ_MULTI|PQ_DFL, 0,
                        &color_parse, diag_parse_color);

        if (!parseEndOk( a, &pt, &retCode)) {
            return retCode;
        }

        if (!SOC_PORT_VALID(unit,port)) {
            printk("%s: ERROR: Invalid port selection %d\n",
                   ARG_CMD(a), port);
            return CMD_FAIL;
        }

        if (pkt_prio < 0 || cfi < 0 || int_prio < 0) {
            printk("Color map settings for port %s\n", 
                                      BCM_PORT_NAME(unit, port));

            for (prio = BCM_PRIO_MIN; prio <= BCM_PRIO_MAX; prio++) {
                for (cfi = 0; cfi <= 1; cfi++) {
                    if ((r = bcm_port_vlan_priority_map_get(unit, port, 
                              prio, cfi, &int_prio, &color)) < 0) {
                        goto bcm_err; 
                    }
                    printk("Packet Prio=%d, CFI=%d, Internal Prio=%d, "
                           "Color=%s\n",
                            prio, cfi, int_prio, diag_parse_color[color]);
                } 
            }
         } else {
             color = (bcm_color_t) color_parse;
             if ((r = bcm_port_vlan_priority_map_set(unit, port, pkt_prio, cfi,
                                                int_prio, color)) < 0) {
                 goto bcm_err;
             }
         }

        return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "unmap") == 0) {
        int pkt_prio, int_prio;

        pkt_prio = int_prio = -1;

        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Port", PQ_DFL | PQ_PORT, 0, &port, NULL);
        parse_table_add(&pt, "PktPrio", PQ_INT|PQ_DFL, 0, &pkt_prio, NULL);
        parse_table_add(&pt, "CFI", PQ_INT|PQ_DFL, 0, &cfi, NULL);
        parse_table_add(&pt, "IntPrio", PQ_INT|PQ_DFL, 0, &int_prio, NULL);
        parse_table_add(&pt, "Color", PQ_MULTI|PQ_DFL, 0,
                        &color_parse, diag_parse_color);

        if (!parseEndOk( a, &pt, &retCode)) {
            return retCode;
        }

        if (!SOC_PORT_VALID(unit,port)) {
            printk("%s: ERROR: Invalid port selection %d\n",
                   ARG_CMD(a), port);
            return CMD_FAIL;
        }

        if (pkt_prio < 0 || cfi < 0 || int_prio < 0) {
            printk("Color unmap settings for port %s\n", 
                                      BCM_PORT_NAME(unit, port));

            for (prio = BCM_PRIO_MIN; prio <= BCM_PRIO_MAX; prio++) {
                for (color = bcmColorGreen; 
                     color <= bcmColorRed; 
                     color++) {
                    if ((r = bcm_port_vlan_priority_unmap_get(unit, port, 
                              prio, color, &pkt_prio, &cfi)) < 0) {
                        goto bcm_err; 
                    }
                    printk("Internal Prio=%d, Color=%s, Packet Prio=%d, "
                           "CFI=%d\n",
                            prio, diag_parse_color[color], pkt_prio, cfi);
                } 
            }
         } else {
             color = (bcm_color_t) color_parse;
             if ((r = bcm_port_vlan_priority_unmap_set(unit, port, int_prio, 
                                             color, pkt_prio, cfi)) < 0) {
                 goto bcm_err;
             }
         }

        return CMD_OK;
 
    }

    printk("%s: ERROR: Unknown color subcommand: %s\n",
           ARG_CMD(a), subcmd);

    return CMD_USAGE;

 bcm_err:

    printk("%s: ERROR: %s\n", ARG_CMD(a), bcm_errmsg(r));

    return CMD_FAIL;
}

#ifdef BCM_TRIUMPH3_SUPPORT
char ibod_sync_usage[] =
    "Parameters: [off] [Interval=<interval>]\n\t"
    "Starts the IBOD recovery thread running every <interval>\n\t"
    "microseconds. If <interval> is 0, stops the task.\n\t"
    "If <interval> is omitted, prints current setting.\n";

cmd_result_t
cmd_ibod_sync(int unit, args_t *a)
{
    sal_usecs_t		usec;
    parse_table_t	pt;
    int			r;
    uint64              event_count;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (!SOC_IS_TRIUMPH3(unit)) {
	printk("%s: IBOD sync is not needed\n", ARG_CMD(a));
	return CMD_OK;
    }

    if (_bcm_esw_ibod_sync_recovery_running(unit, &usec, &event_count) < 0) {
        usec = 0;
    }

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "Interval", PQ_DFL|PQ_INT,
		    (void *) 0, &usec, NULL);

    if (!ARG_CNT(a)) {			/* Display settings */
	printk("Current settings:\n");
	parse_eq_format(&pt);
        printk("Unit %d: IBOD sync event count: %d%d\n", unit,
               usec ? COMPILER_64_HI(event_count) : 0,
               usec ? COMPILER_64_LO(event_count) : 0);
	parse_arg_eq_done(&pt);
	return CMD_OK;
    }

    if (parse_arg_eq(a, &pt) < 0) {
	printk("%s: Error: Unknown option: %s\n", ARG_CMD(a), ARG_CUR(a));
	parse_arg_eq_done(&pt);
	return CMD_FAIL;
    }
    parse_arg_eq_done(&pt);

    if (ARG_CNT(a) > 0 && !sal_strcasecmp(_ARG_CUR(a), "off")) {
	ARG_NEXT(a);
	usec = 0;
    }

    if (ARG_CNT(a) > 0) {
	return CMD_USAGE;
    }

    if (usec > 0) {
        r = _bcm_esw_ibod_sync_recovery_start(unit, usec);
    } else {
        r = _bcm_esw_ibod_sync_recovery_stop(unit);
    }

    if (r < 0) {
	printk("%s: Error: Could not set IBOD SYNCe: %s\n",
	       ARG_CMD(a), soc_errmsg(r));
	return CMD_FAIL;
    }

    return CMD_OK;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
/* XAUI BERT Test */

char cmd_xaui_usage[] =
    "Run XAUI BERT.\n"
    "Usages:\n\t"
    "  xaui bert SrcPort=<port> DestPort=<port> Duration=<usec> Verb=0/1\n";

#define XAUI_PREEMPHASIS_MIN  (0)
#define XAUI_PREEMPHASIS_MAX  (15)
#define XAUI_IDRIVER_MIN      (0)
#define XAUI_IDRIVER_MAX      (15)
#define XAUI_EQUALIZER_MIN    (0)
#define XAUI_EQUALIZER_MAX    (7)

typedef struct _xaui_bert_info_s {
    bcm_port_t          src_port;
    bcm_port_t          dst_port;
    soc_xaui_config_t   src_config;
    soc_xaui_config_t   dst_config;
    soc_xaui_config_t   test_config;
    bcm_port_info_t     src_info;
    bcm_port_info_t     dst_info;
    int                 duration;
    int                 linkscan_us;
    int                 verbose;
} _xaui_bert_info_t;

/*
 * Function:
 *      _xaui_bert_counter_check 
 * Purpose:
 *      Check BERT counters after the test. 
 * Parameters:
 *      (IN) unit       - BCM unit number
 *      (IN) test_info - Test configuration 
 * Returns:
 *      BCM_E_NONE - success
 *      BCM_E_XXXX - failed.
 * Notes:
 */
static int
_xaui_bert_counter_check(int unit, _xaui_bert_info_t *test_info) 
{
    bcm_port_t src_port, dst_port;
    uint32 tx_pkt, tx_byte, rx_pkt, rx_byte, bit_err, byte_err, pkt_err;
    int    prbs_lock, lock;

    src_port = test_info->src_port;
    dst_port = test_info->dst_port;

    /* Read Tx counters */
    SOC_IF_ERROR_RETURN
        (soc_xaui_txbert_pkt_count_get(unit, src_port, &tx_pkt));
    SOC_IF_ERROR_RETURN
        (soc_xaui_txbert_byte_count_get(unit, src_port, &tx_byte));

    lock = 1;
    /* Read Rx counters */
    SOC_IF_ERROR_RETURN
        (soc_xaui_rxbert_pkt_count_get(unit, dst_port, 
                                       &rx_pkt, &prbs_lock));
    lock &= prbs_lock;

    SOC_IF_ERROR_RETURN
        (soc_xaui_rxbert_byte_count_get(unit, dst_port, 
                                        &rx_byte, &prbs_lock));
    lock &= prbs_lock;

    SOC_IF_ERROR_RETURN
        (soc_xaui_rxbert_bit_err_count_get(unit, dst_port, 
                                           &bit_err, &prbs_lock));
    lock &= prbs_lock;

    SOC_IF_ERROR_RETURN
        (soc_xaui_rxbert_byte_err_count_get(unit, dst_port, 
                                            &byte_err, &prbs_lock));
    lock &= prbs_lock;

    SOC_IF_ERROR_RETURN
        (soc_xaui_rxbert_pkt_err_count_get(unit, dst_port, 
                                           &pkt_err, &prbs_lock));
    lock &= prbs_lock;

    if (test_info->verbose) {
        printk(" %4s->%4s, 0x%08x, 0x%08x, 0x%08x, %s, ", 
               BCM_PORT_NAME(unit, src_port), BCM_PORT_NAME(unit, dst_port),
               tx_byte, rx_byte, bit_err, 
               lock? "       OK": "      !OK");
    }

    /* Check TX/RX counters */
    if ((tx_byte == 0) || (tx_pkt == 0) ||
        (tx_byte != rx_byte) || (tx_pkt != rx_pkt) || !lock) {
        return BCM_E_FAIL;
    }

   /* Check error counters */
    if ((bit_err != 0) || (byte_err != 0) || (pkt_err != 0)) {
        return BCM_E_FAIL;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _xaui_bert_test 
 * Purpose:
 *      Run BERT test with requested port configuration. 
 * Parameters:
 *      (IN) unit       - BCM unit number
 *      (IN) test_info - Test configuration 
 * Returns:
 *      BCM_E_NONE - success
 *      BCM_E_XXXX - failed.
 * Notes:
 */
static int
_xaui_bert_test(int unit, _xaui_bert_info_t *test_info)
{
    int            result1, result2;
    bcm_port_t     src_port, dst_port;

    src_port = test_info->src_port;
    dst_port = test_info->dst_port;

    BCM_IF_ERROR_RETURN
        (bcm_port_speed_set(unit, src_port, 10000));
    BCM_IF_ERROR_RETURN
        (bcm_port_speed_set(unit, dst_port, 10000));

    BCM_IF_ERROR_RETURN
        (soc_xaui_config_set(unit, src_port, &test_info->test_config));
    BCM_IF_ERROR_RETURN
        (soc_xaui_config_set(unit, dst_port, &test_info->test_config));

    /* Wait up to 0.1 sec for TX PLL lock */ 
    sal_usleep(100000); 

    /* Enable RX BERT on both ports first */
    BCM_IF_ERROR_RETURN(soc_xaui_rxbert_enable(unit, dst_port, TRUE));
    if (src_port != dst_port) {
        BCM_IF_ERROR_RETURN
            (soc_xaui_rxbert_enable(unit, src_port, TRUE));
    }

    /* Enable TX BERT on both ports */
    BCM_IF_ERROR_RETURN(soc_xaui_txbert_enable(unit, src_port, TRUE));
    if (src_port != dst_port) {
        BCM_IF_ERROR_RETURN
            (soc_xaui_txbert_enable(unit, dst_port, TRUE));
    }

    /* Run test for requested duration */
    sal_usleep(test_info->duration);

    /* Disable TX BERT */
    BCM_IF_ERROR_RETURN
       (soc_xaui_txbert_enable(unit, src_port, FALSE));
    if (src_port != dst_port) {
        BCM_IF_ERROR_RETURN
            (soc_xaui_txbert_enable(unit, dst_port, FALSE));
    }

    /* Give enough time to complete tx/rx */ 
    sal_usleep(500);
    result1 = _xaui_bert_counter_check(unit, test_info);
    result2 = _xaui_bert_counter_check(unit, test_info);
    if (BCM_SUCCESS(result1) && BCM_SUCCESS(result2)) {
        printk(" ( P ) "); 
    } else {
        printk(" ( F ) "); 
    }

    if (test_info->verbose) {
        printk("\n");
    }

    /* Disable RX BERT only after reading the counter. 
     * Otherwise, counters always read zero.
     */
    BCM_IF_ERROR_RETURN
        (soc_xaui_rxbert_enable(unit, src_port, FALSE));
    if (src_port != dst_port) {
        BCM_IF_ERROR_RETURN
            (soc_xaui_rxbert_enable(unit, dst_port, FALSE));
    }

    if ((BCM_E_NONE != result1) && (BCM_E_FAIL != result1)) {
        return result1;
    }
    if ((BCM_E_NONE != result2) && (BCM_E_FAIL != result2)) {
        return result2;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _xaui_bert_save_config 
 * Purpose:
 *      Disable linkscan and save current port configuration. 
 * Parameters:
 *      (IN) unit       - BCM unit number
 *      (OUT) test_info - Current port configuration 
 * Returns:
 *      BCM_E_NONE - success
 *      BCM_E_XXXX - failed.
 * Notes:
 */
static int
_xaui_bert_save_config(int unit, _xaui_bert_info_t *test_info) {

    /* If linkscan is enabled, disable linkscan */
    BCM_IF_ERROR_RETURN
        (bcm_linkscan_enable_get(unit, &test_info->linkscan_us));
    if (test_info->linkscan_us != 0) {
        BCM_IF_ERROR_RETURN(bcm_linkscan_enable_set(unit, 0));
        /* Give enough time for linkscan task to exit. */
        sal_usleep(test_info->linkscan_us * 5); 
    }

    /* Save current settings */ 
    BCM_IF_ERROR_RETURN
        (soc_xaui_config_get(unit, test_info->src_port, 
                             &test_info->src_config));
    BCM_IF_ERROR_RETURN
        (soc_xaui_config_get(unit, test_info->dst_port, 
                             &test_info->dst_config));

    /* Save original speed settings */
    BCM_IF_ERROR_RETURN
        (bcm_port_info_save(unit, test_info->src_port, &test_info->src_info));
    BCM_IF_ERROR_RETURN
        (bcm_port_info_save(unit, test_info->dst_port, &test_info->dst_info));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _xaui_bert_restore_config 
 * Purpose:
 *      Restore original port configuration. 
 * Parameters:
 *      (IN) unit      - BCM unit number
 *      (IN) test_info - Port configuration to be restored 
 * Returns:
 *      BCM_E_NONE - success
 *      BCM_E_XXXX - failed.
 * Notes:
 */
static int
_xaui_bert_restore_config(int unit, _xaui_bert_info_t *test_info) {
 
    BCM_IF_ERROR_RETURN
        (bcm_port_info_restore(unit, test_info->src_port, 
                               &test_info->src_info));
    BCM_IF_ERROR_RETURN
        (bcm_port_info_restore(unit, test_info->dst_port, 
                               &test_info->dst_info));

#if 0
    /* Restore original configuration */
    BCM_IF_ERROR_RETURN
        (soc_xaui_config_set(unit, test_info->src_port, 
                             &test_info->src_config));
    BCM_IF_ERROR_RETURN
        (soc_xaui_config_set(unit, test_info->dst_port, 
                             &test_info->dst_config));
#endif

    if (test_info->linkscan_us != 0) {
        BCM_IF_ERROR_RETURN
            (bcm_linkscan_enable_set(unit, test_info->linkscan_us));
    }  

    return BCM_E_NONE;
}

char bert_header[] =
     "                                   Equalizer\n"
     "I_Driver     0      1      2      3      4      5      6      7\n";

char bert_header_v[] =
     "\n Preemph, I_Driver, Equalizer,   Src->Des,"
     "    tx_byte,    rx_byte,    bit_err, PRBS Lock,"
     "    Des->Src,    tx_byte,    rx_byte,    bit_err,"
     " PRBS Lock\n";

/*
 * Function:
 *      cmd_xaui 
 * Purpose:
 *      Entry point to XAUI related CLI commands.
 * Parameters:
 *      (IN) unit - BCM unit number
 *      (IN) a    - Arguments for the command 
 * Returns:
 *      BCM_E_NONE - success
 *      BCM_E_XXXX - failed.
 * Notes:
 */
cmd_result_t
cmd_xaui(int unit, args_t *a)
{
    char          *subcmd;
    parse_table_t  pt;
    int            rv;
    cmd_result_t   retCode;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    if ((subcmd = ARG_GET(a)) == NULL) {
        return CMD_USAGE;
    }

    rv = BCM_E_NONE;
    if (sal_strcasecmp(subcmd, "bert") == 0) {
        uint32            preemphasis, idriver, equalizer;
        _xaui_bert_info_t test_info;

        sal_memset(&test_info, 0, sizeof(test_info));
        test_info.duration  = 10;

        parse_table_init(unit, &pt);
        parse_table_add(&pt, "SrcPort", PQ_DFL|PQ_PORT, 0, 
                        &test_info.src_port, NULL);
        parse_table_add(&pt, "DestPort", PQ_DFL|PQ_PORT, 0, 
                        &test_info.dst_port, NULL);
        parse_table_add(&pt, "Duration", PQ_INT|PQ_DFL, 0, 
                        &test_info.duration, NULL);
        parse_table_add(&pt, "Verbose", PQ_BOOL|PQ_DFL, 0, 
                        &test_info.verbose, NULL);

        if (!parseEndOk( a, &pt, &retCode)) {
            return retCode;
        }

        /* Run only on HG and XE port */
        if ((!IS_HG_PORT(unit, test_info.src_port) && 
             !IS_XE_PORT(unit, test_info.src_port)) || 
            (!IS_HG_PORT(unit, test_info.dst_port) && 
             !IS_XE_PORT(unit, test_info.dst_port))) {
            printk("%s: ERROR: Invalid port selection %d, %d\n",
                   ARG_CMD(a), test_info.src_port, test_info.dst_port);
            return CMD_FAIL;
        }
        
        if ((rv = _xaui_bert_save_config(unit, &test_info)) < 0) {
            goto cmd_xaui_err;
        }

        test_info.test_config = test_info.src_config;
        for (preemphasis = XAUI_PREEMPHASIS_MIN; 
             preemphasis <= XAUI_PREEMPHASIS_MAX;
             preemphasis++) {
            test_info.test_config.preemphasis = preemphasis;
             
            if (!test_info.verbose) {
                printk("\nPreemphasis = %d\n", preemphasis);
            } 

            printk("%s", test_info.verbose? bert_header_v: bert_header);

            for (idriver = XAUI_IDRIVER_MIN;
                 idriver <= XAUI_IDRIVER_MAX;
                 idriver++) {
                test_info.test_config.idriver = idriver;
                if (!test_info.verbose) {
                    printk("%8d  ", idriver);
                }
                for (equalizer = XAUI_EQUALIZER_MIN;
                     equalizer <= XAUI_EQUALIZER_MAX;
                     equalizer++) {
                   
                    if (test_info.verbose) {
                        printk("%8d, %8d, %9d,", preemphasis, idriver,
                               equalizer);
                    }

                    test_info.test_config.equalizer_ctrl = equalizer;
                    if ((rv = _xaui_bert_test(unit, &test_info)) < 0) {
                        _xaui_bert_restore_config(unit, &test_info);
                         goto cmd_xaui_err;
                    }
                }
                printk("\n");
            }
        }

        if ((rv = _xaui_bert_restore_config(unit, &test_info)) < 0) {
            goto cmd_xaui_err;
        }
        return CMD_OK;
    }
    printk("%s: ERROR: Unknown xaui subcommand: %s\n",
           ARG_CMD(a), subcmd);
    return CMD_USAGE;

cmd_xaui_err:
    printk("%s: ERROR: %s\n", ARG_CMD(a), bcm_errmsg(rv));
    return CMD_FAIL;
}
#endif /* BCM_XGS3_SWITCH_SUPPORT */


#define INCLUDE_TIMESYNC_DVT_TESTS
#ifdef INCLUDE_TIMESYNC_DVT_TESTS

uint64 _compiler_64_arg (uint32 hi, uint32 low)
{
    uint64 val;
   COMPILER_64_SET(val, hi, low);
   return val;
}

/* Test case 1 */
void config_i1e1_in(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE;

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;

    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}

void config_i1e1_out(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE;

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;

    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}

/* Test case 2 */
void config_i0e2_in(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE;

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;

    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         BCM_PORT_PHY_TIMESYNC_TIMECODE_LOAD));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}


void config_i0e2_out(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE;

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeReplaceCorrectionFieldOrigin;

    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;

    config.original_timecode.seconds = _compiler_64_arg(0, 0xbeef8000);

    config.original_timecode.nanoseconds = 0xaaaabbbb;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         BCM_PORT_PHY_TIMESYNC_TIMECODE_LOAD));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}


/* Test case 3 */
void config_i2e0_in(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_GMODE;

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeInsertTimestamp;
    /* config.gmode = bcmPortPhyTimesyncModeCpu; */
    config.gmode = bcmPortPhyTimesyncModeFree;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}

void config_i2e0_out(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE | BCM_PORT_PHY_TIMESYNC_VALID_GMODE;

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;
    /* config.gmode = bcmPortPhyTimesyncModeCpu; */
    config.gmode = bcmPortPhyTimesyncModeFree;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         BCM_PORT_PHY_TIMESYNC_TIMECODE_LOAD));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}



void config_tc5p1(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS |  BCM_PORT_PHY_TIMESYNC_VALID_GMODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE;

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.syncout.mode = bcmPortPhyTimesyncSyncoutOneTime;
    config.syncout.pulse_1_length = 20000;
    config.syncout.syncout_ts = _compiler_64_arg(0, 80000);
    config.gmode = bcmPortPhyTimesyncModeCpu;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         0xffffffff));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}

void config_tc6p1(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS |  BCM_PORT_PHY_TIMESYNC_VALID_GMODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE;

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.syncout.mode = bcmPortPhyTimesyncSyncoutPulseTrain;
    config.syncout.pulse_1_length = 20000;
    config.syncout.pulse_2_length = 10000;
    config.syncout.syncout_ts = _compiler_64_arg(0, 80000);
    config.gmode = bcmPortPhyTimesyncModeCpu;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         0xffffffff));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}

void config_tc7p1(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS |  BCM_PORT_PHY_TIMESYNC_VALID_GMODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE;

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.syncout.mode = bcmPortPhyTimesyncSyncoutPulseTrainWithSync;
    config.syncout.pulse_1_length = 20000;
    config.syncout.pulse_2_length = 10000;
    config.syncout.syncout_ts = _compiler_64_arg(0, 80000);
    config.gmode = bcmPortPhyTimesyncModeCpu;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         0xffffffff));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}

void config_tc8p1(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS |  BCM_PORT_PHY_TIMESYNC_VALID_GMODE | BCM_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE;

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.framesync.mode = bcmPortPhyTimesyncFramesyncSyncin0;
    config.framesync.length_threshold = 20000;
    config.gmode = bcmPortPhyTimesyncModeCpu;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         0xffffffff));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}


/* Test case 1: MPLS label 1 : all ptp pkts  */
void config_i1e1_all_pkts_mpls1_en_in(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL;
    config.mpls_control.size=1;
    /* config.mpls_control.labels = sal_alloc(config.mpls_control.size * sizeof(_shr_port_phy_timesync_mpls_label_t), "timesync_mpls_label"); */

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);
    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;
    
    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;
    
    config.tx_delay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_delay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;
    
    config.tx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;
    
    config.tx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;
    
    config.mpls_control.flags = BCM_PORT_PHY_TIMESYNC_MPLS_ENABLE
                                | BCM_PORT_PHY_TIMESYNC_MPLS_CONTROL_WORD_ENABLE;
   
    rv = bcm_port_phy_timesync_config_set(unit, port, &config);
    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
    /* if(config.mpls_control.labels != NULL) {
      sal_free(config.mpls_control.labels);
    } */

}

void config_i1e1_all_pkts_mpls1_en_out(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL;

    config.mpls_control.size=1;
    /* config.mpls_control.labels = sal_alloc(config.mpls_control.size * sizeof(_shr_port_phy_timesync_mpls_label_t), "timesync_mpls_label");*/
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);
    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;
    
    config.tx_delay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_delay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;
    
    config.tx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;
    
    config.tx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;
    
    config.mpls_control.flags = BCM_PORT_PHY_TIMESYNC_MPLS_ENABLE
                                | BCM_PORT_PHY_TIMESYNC_MPLS_CONTROL_WORD_ENABLE;
    
    rv = bcm_port_phy_timesync_config_set(unit, port, &config);
    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
    /* if(config.mpls_control.labels != NULL) {
      sal_free(config.mpls_control.labels);
    } */
}

/* Test case 2 : MPLS label 1 : all ptp pkts  */
void config_i0e2_all_pkts_mpls1_en_in(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL;

    config.mpls_control.size=1;
    /* config.mpls_control.labels = sal_alloc(config.mpls_control.size * sizeof(_shr_port_phy_timesync_mpls_label_t), "timesync_mpls_label"); */
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;

    config.tx_delay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_delay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;
    
    config.tx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;
    
    config.tx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;
    
    config.mpls_control.flags = BCM_PORT_PHY_TIMESYNC_MPLS_ENABLE 
                                | BCM_PORT_PHY_TIMESYNC_MPLS_CONTROL_WORD_ENABLE;
    
    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         BCM_PORT_PHY_TIMESYNC_TIMECODE_LOAD));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
    /* if(config.mpls_control.labels != NULL) {
      sal_free(config.mpls_control.labels);
    } */
}


void config_i0e2_all_pkts_mpls1_en_out(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_GMODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL;

    config.mpls_control.size=1;
    /* config.mpls_control.labels = sal_alloc(config.mpls_control.size * sizeof(_shr_port_phy_timesync_mpls_label_t), "timesync_mpls_label"); */
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeReplaceCorrectionFieldOrigin;
    /*config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;*/

    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;
    
    config.tx_delay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeReplaceCorrectionFieldOrigin;
    config.rx_delay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;
    
    config.tx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeReplaceCorrectionFieldOrigin;
    config.rx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;
    
    config.tx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageEgressModeReplaceCorrectionFieldOrigin;
    config.rx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;

    config.original_timecode.seconds = _compiler_64_arg(0, 0xbeef8000);
    config.original_timecode.nanoseconds = 0xaaaabbbb;

    config.framesync.mode = bcmPortPhyTimesyncFramesyncCpu /*bcmPortPhyTimesyncFramesyncSyncin0*/; 
    config.framesync.length_threshold = 0x4;
    config.framesync.event_offset = 0x8;

    config.gmode = bcmPortPhyTimesyncModeCpu; 
    
    config.mpls_control.flags = BCM_PORT_PHY_TIMESYNC_MPLS_ENABLE
                                | BCM_PORT_PHY_TIMESYNC_MPLS_CONTROL_WORD_ENABLE;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         BCM_PORT_PHY_TIMESYNC_TIMECODE_LOAD));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
    /* if(config.mpls_control.labels != NULL) {
      sal_free(config.mpls_control.labels);
    } */
}


/* Test case 3 : MPLS label 1 : all ptp pkts  */
void config_i2e0_all_pkts_mpls1_en_in(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_GMODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL;

    config.mpls_control.size=1;
    /* config.mpls_control.labels = sal_alloc(config.mpls_control.size * sizeof(_shr_port_phy_timesync_mpls_label_t), "timesync_mpls_label"); */
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeInsertTimestamp;
	
    config.tx_delay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_delay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeInsertTimestamp;
    
    config.tx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeInsertTimestamp;
    
    config.tx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageIngressModeInsertTimestamp;

    /* config.gmode = bcmPortPhyTimesyncModeCpu; */
    config.gmode = bcmPortPhyTimesyncModeFree;
    
    config.mpls_control.flags = BCM_PORT_PHY_TIMESYNC_MPLS_ENABLE
                                | BCM_PORT_PHY_TIMESYNC_MPLS_CONTROL_WORD_ENABLE;
	
    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
    /* if(config.mpls_control.labels != NULL) {
      sal_free(config.mpls_control.labels);
    } */
}

void config_i2e0_all_pkts_mpls1_en_out(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE
                           | BCM_PORT_PHY_TIMESYNC_VALID_GMODE 
                           | BCM_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL;

    config.mpls_control.size=1;
    /* config.mpls_control.labels = sal_alloc(config.mpls_control.size * sizeof(_shr_port_phy_timesync_mpls_label_t), "timesync_mpls_label"); */
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;

    config.tx_delay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_delay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;
    
    config.tx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_pdelay_request_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;
    
    config.tx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageEgressModeNone;
    config.rx_pdelay_response_mode = bcmPortPhyTimesyncEventMessageIngressModeNone;

#if 0
    config.gmode = bcmPortPhyTimesyncModeCpu; 
#else
    config.gmode = bcmPortPhyTimesyncModeFree;
#endif

    config.mpls_control.flags = BCM_PORT_PHY_TIMESYNC_MPLS_ENABLE
                                | BCM_PORT_PHY_TIMESYNC_MPLS_CONTROL_WORD_ENABLE;
    
    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         BCM_PORT_PHY_TIMESYNC_TIMECODE_LOAD));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
    if(config.mpls_control.labels != NULL) {
      sal_free(config.mpls_control.labels);
    }
}



void config_tc5p1_fs(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS |  BCM_PORT_PHY_TIMESYNC_VALID_GMODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE;


    config.mpls_control.size=1;
    /* config.mpls_control.labels = sal_alloc(config.mpls_control.size * sizeof(_shr_port_phy_timesync_mpls_label_t), "timesync_mpls_label"); */
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.syncout.interval = 0x00000080;
    config.syncout.pulse_1_length = 0x0020; 
    config.syncout.pulse_2_length = 0x040;
    config.syncout.syncout_ts = _compiler_64_arg(0, 80000);
    config.syncout.mode = bcmPortPhyTimesyncSyncoutOneTime;
    config.framesync.mode = bcmPortPhyTimesyncFramesyncCpu;
    config.gmode = bcmPortPhyTimesyncModeCpu;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         0xffffffff));
    
    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncFrameSync, _compiler_64_arg(0 ,
         bcmPortPhyTimesyncFramesyncSyncin0));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    /* if(config.mpls_control.labels != NULL) {
      sal_free(config.mpls_control.labels);
    } */
}

void config_tc6p1_fs(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS |  BCM_PORT_PHY_TIMESYNC_VALID_GMODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE;

    config.mpls_control.size=1;
    /* config.mpls_control.labels = sal_alloc(config.mpls_control.size * sizeof(_shr_port_phy_timesync_mpls_label_t), "timesync_mpls_label"); */
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.syncout.interval = 0x00000080;
    config.syncout.pulse_1_length = 0x0020; 
    config.syncout.pulse_2_length = 0x040;
    config.syncout.syncout_ts = _compiler_64_arg(0, 80000);
    config.syncout.mode = bcmPortPhyTimesyncSyncoutPulseTrain;
    config.framesync.mode = bcmPortPhyTimesyncFramesyncCpu;
    config.gmode = bcmPortPhyTimesyncModeCpu;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         0xffffffff));

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncFrameSync, _compiler_64_arg(0 ,
         bcmPortPhyTimesyncFramesyncSyncin0));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
    /* if(config.mpls_control.labels != NULL) {
      sal_free(config.mpls_control.labels);
    } */
}

void config_tc7p1_fs(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS |  BCM_PORT_PHY_TIMESYNC_VALID_GMODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE;

    config.mpls_control.size=1;
    /* config.mpls_control.labels = sal_alloc(config.mpls_control.size * sizeof(_shr_port_phy_timesync_mpls_label_t), "timesync_mpls_label"); */
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.syncout.interval = 0x00000080;
    config.syncout.pulse_1_length = 0x0020; 
    config.syncout.pulse_2_length = 0x040;
    config.syncout.syncout_ts = _compiler_64_arg(0, 80000);
    config.syncout.mode = bcmPortPhyTimesyncSyncoutPulseTrainWithSync;
    config.framesync.mode = bcmPortPhyTimesyncFramesyncCpu;
    config.gmode = bcmPortPhyTimesyncModeCpu;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         0xffffffff));

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncFrameSync, _compiler_64_arg(0 ,
         bcmPortPhyTimesyncFramesyncSyncin0));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
    /* if(config.mpls_control.labels != NULL) {
      sal_free(config.mpls_control.labels);
    } */
}

void config_tc8p1_fs(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS |  BCM_PORT_PHY_TIMESYNC_VALID_GMODE | BCM_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE;

    config.mpls_control.size=1;
    /* config.mpls_control.labels = sal_alloc(config.mpls_control.size * sizeof(_shr_port_phy_timesync_mpls_label_t), "timesync_mpls_label"); */
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    config.framesync.mode = bcmPortPhyTimesyncFramesyncSyncin0;
    config.framesync.length_threshold = 20000;
    config.gmode = bcmPortPhyTimesyncModeCpu;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         0xffffffff));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
    /* if(config.mpls_control.labels != NULL) {
      sal_free(config.mpls_control.labels);
    } */
}


void cpu_sync(int unit, bcm_port_t port)
{
    int rv;

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncFrameSync, _compiler_64_arg(0,1));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

}


void set_localtime(int unit, bcm_port_t port, uint64 localtime)
{
    int rv;

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLocalTime, localtime);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         BCM_PORT_PHY_TIMESYNC_LOCAL_TIME_LOAD));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

}

void print_local_time(int unit, bcm_port_t port)
{
    int rv;
    uint64 time;

    rv = bcm_port_control_phy_timesync_get(unit, port, bcmPortControlPhyTimesyncLocalTime, &time);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_get failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    printk("Current Local Time u=%d p=%d  Time = %08x%08x\n",
           unit, port, COMPILER_64_HI(time), COMPILER_64_LO(time));
}

void test1588(int port1, int port2)
{

config_i2e0_in( 0, port1);
config_i2e0_out( 0, port2);

set_localtime(0, port1, _compiler_64_arg(0xa33, 0x44556677));

cpu_sync(0, port1);
print_local_time(0, port1);
_print_heartbeat_ts(0, port1);
_print_capture_ts(0, port1);

}

void test1588_1(int port1, int port2)
{
config_i1e1_in( 0, port1);
config_i1e1_out( 0, port2);
cpu_sync(0, port1);
cpu_sync(0, port1);
}

void test1588_2(int port1, int port2)
{
config_i0e2_in( 0, port1);
config_i0e2_out( 0, port2);
cpu_sync(0, port1);
cpu_sync(0, port1);
}

void test1588_3(int port1, int port2)
{
config_i2e0_in( 0, port1);
config_i2e0_out( 0, port2);
cpu_sync(0, port1);
cpu_sync(0, port1);
}

void test1588_4(int port1, int port2)
{
config_i0e2_in( 0, port1);
config_i0e2_in( 0, port2);
cpu_sync(0, port1);
cpu_sync(0, port1);
}

void test1588_5(int port1)
{
config_tc5p1( 0, port1);
cpu_sync(0, port1);
}

void test1588_6(int port1)
{
config_tc6p1( 0, port1);
cpu_sync(0, port1);
}

void test1588_7(int port1)
{
config_tc7p1( 0, port1);
cpu_sync(0, port1);
}

void test1588_8(int port1)
{
config_tc8p1( 0, port1);
cpu_sync(0, port1);
}

void test1588_1_2(int port1, int port2)
{
config_i1e1_all_pkts_mpls1_en_in( 0, port1);
config_i1e1_all_pkts_mpls1_en_out( 0, port2);
}

void test1588_2_2(int port1, int port2)
{
config_i0e2_all_pkts_mpls1_en_in( 0, port1);
config_i0e2_all_pkts_mpls1_en_out( 0, port2);
}

void test1588_3_2(int port1, int port2)
{
config_i2e0_all_pkts_mpls1_en_in( 0, port1);
config_i2e0_all_pkts_mpls1_en_out( 0, port2);
}

void test1588_4_2(int port1, int port2)
{
config_i0e2_all_pkts_mpls1_en_in( 0, port1);
config_i0e2_all_pkts_mpls1_en_in( 0, port2);
}

void test1588_5_2(int port1)
{
config_tc5p1_fs( 0, port1);
}

void test1588_6_2(int port1)
{
config_tc6p1_fs( 0, port1);
}

void test1588_7_2(int port1)
{
config_tc7p1_fs( 0, port1);
}

void test1588_8_2(int port1)
{
config_tc8p1_fs( 0, port1);
}

void config_fs_test(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;
    uint64 time, temp;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS |  BCM_PORT_PHY_TIMESYNC_VALID_GMODE | BCM_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE;

    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncFrameSync, _compiler_64_arg(0,1));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_get(unit, port, bcmPortControlPhyTimesyncHeartbeatTimestamp, &time);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_get failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    printk("Heartbeat TS = %08x%08x\n",
           COMPILER_64_HI(time), COMPILER_64_LO(time));

    /* Add delta of 4.3 sec */
    temp = _compiler_64_arg(0x1, 0);
    COMPILER_64_ADD_64(time , temp);
    config.syncout.syncout_ts = time;

    printk("Syncout   TS = %08x%08x\n",
           COMPILER_64_HI(config.syncout.syncout_ts), COMPILER_64_LO(config.syncout.syncout_ts));


    config.syncout.interval = 0x00000080;
    config.syncout.pulse_1_length = 25000; 
    config.syncout.pulse_2_length = 0x040;
    config.syncout.mode = bcmPortPhyTimesyncSyncoutOneTime;
    config.gmode = bcmPortPhyTimesyncModeCpu;
    config.framesync.mode = bcmPortPhyTimesyncFramesyncSyncin0;
    config.framesync.length_threshold = 20000;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_GMODE | BCM_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE |
                           BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE ;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         BCM_PORT_PHY_TIMESYNC_SYNCOUT_LOAD));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncFrameSync, _compiler_64_arg(0,1));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_get(unit, port, bcmPortControlPhyTimesyncHeartbeatTimestamp, &time);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_get failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    printk("Heartbeat TS = %08x%08x\n",
           COMPILER_64_HI(time), COMPILER_64_LO(time));

    sal_usleep(5000000);

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncFrameSync, _compiler_64_arg(0,1));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_get(unit, port, bcmPortControlPhyTimesyncHeartbeatTimestamp, &time);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_get failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    printk("Heartbeat TS = %08x%08x\n",
           COMPILER_64_HI(time), COMPILER_64_LO(time));

#if 0
    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS |  BCM_PORT_PHY_TIMESYNC_VALID_GMODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE;


    config.mpls_control.size=1;
    config.mpls_control.labels = sal_alloc(config.mpls_control.size * sizeof(_shr_port_phy_timesync_mpls_label_t), "timesync_mpls_label");
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
         0xffffffff));
    
    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncFrameSync, _compiler_64_arg(0 ,
         bcmPortPhyTimesyncFramesyncSyncin0));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    if(config.mpls_control.labels != NULL) {
      sal_free(config.mpls_control.labels);
    }
#endif
}

void config_port_sync_1k(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    sal_memset(&config, 0, sizeof(config));

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS | BCM_PORT_PHY_TIMESYNC_VALID_GMODE |
        BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE |
        BCM_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE_DELTA |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K1 | BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K2 |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K3;

    config.flags = BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | 
                 BCM_PORT_PHY_TIMESYNC_L2_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE |
                 BCM_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT;

    config.gmode = bcmPortPhyTimesyncModeCpu;
    config.framesync.mode = bcmPortPhyTimesyncFramesyncSyncin1;
    config.syncout.mode = bcmPortPhyTimesyncSyncoutDisable;
    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;
    /* for 1KHz SyncIn */
    config.phy_1588_dpll_ref_phase = _compiler_64_arg(0, 0);
    config.phy_1588_dpll_ref_phase_delta = 0x000f4240;
    config.phy_1588_dpll_k1 = 0x2a;
    config.phy_1588_dpll_k2 = 0x28;
    config.phy_1588_dpll_k3 = 0;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLocalTime, 
         _compiler_64_arg(0xa33, 0x44556677));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
        BCM_PORT_PHY_TIMESYNC_TN_LOAD |
        BCM_PORT_PHY_TIMESYNC_TIMECODE_LOAD |
        BCM_PORT_PHY_TIMESYNC_SYNCOUT_LOAD |
        BCM_PORT_PHY_TIMESYNC_LOCAL_TIME_LOAD |
        BCM_PORT_PHY_TIMESYNC_NCO_DIVIDER_LOAD |
        BCM_PORT_PHY_TIMESYNC_NCO_ADDEND_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K3_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K2_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K1_LOAD ));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

}

void config_port_sync_4k(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    sal_memset(&config, 0, sizeof(config));

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS | BCM_PORT_PHY_TIMESYNC_VALID_GMODE |
        BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE |
        BCM_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE_DELTA |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K1 | BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K2 |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K3;

    config.flags = BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | 
                 BCM_PORT_PHY_TIMESYNC_L2_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE |
                 BCM_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT;

    config.gmode = bcmPortPhyTimesyncModeCpu;
    config.framesync.mode = bcmPortPhyTimesyncFramesyncSyncin1;
    config.syncout.mode = bcmPortPhyTimesyncSyncoutDisable;
    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;
    /* for 4KHz SyncIn */
    config.phy_1588_dpll_ref_phase = _compiler_64_arg(0, 0);
    config.phy_1588_dpll_ref_phase_delta = 0x0003d090;
    config.phy_1588_dpll_k1 = 0x2a;
    config.phy_1588_dpll_k2 = 0x26;
    config.phy_1588_dpll_k3 = 0;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLocalTime, 
         _compiler_64_arg(0xa33, 0x44556677));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
        BCM_PORT_PHY_TIMESYNC_TN_LOAD |
        BCM_PORT_PHY_TIMESYNC_TIMECODE_LOAD |
        BCM_PORT_PHY_TIMESYNC_SYNCOUT_LOAD |
        BCM_PORT_PHY_TIMESYNC_LOCAL_TIME_LOAD |
        BCM_PORT_PHY_TIMESYNC_NCO_DIVIDER_LOAD |
        BCM_PORT_PHY_TIMESYNC_NCO_ADDEND_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K3_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K2_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K1_LOAD ));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

}

void config_port_sync_12_5m(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    sal_memset(&config, 0, sizeof(config));

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS | BCM_PORT_PHY_TIMESYNC_VALID_GMODE |
        BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE |
        BCM_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE_DELTA |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K1 | BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K2 |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K3;

    config.flags = BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | 
                 BCM_PORT_PHY_TIMESYNC_L2_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE |
                 BCM_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT;

    config.gmode = bcmPortPhyTimesyncModeCpu;
    config.framesync.mode = bcmPortPhyTimesyncFramesyncSyncin1;
    config.syncout.mode = bcmPortPhyTimesyncSyncoutDisable;
    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;
    /* for 12.5MHz SyncIn */
    config.phy_1588_dpll_ref_phase = _compiler_64_arg(0, 0);
    config.phy_1588_dpll_ref_phase_delta = 80 ;
    config.phy_1588_dpll_k1 = 43;
    config.phy_1588_dpll_k2 = 27;
    config.phy_1588_dpll_k3 = 0;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLocalTime, 
         _compiler_64_arg(0xa33, 0x44556677));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
        BCM_PORT_PHY_TIMESYNC_TN_LOAD |
        BCM_PORT_PHY_TIMESYNC_TIMECODE_LOAD |
        BCM_PORT_PHY_TIMESYNC_SYNCOUT_LOAD |
        BCM_PORT_PHY_TIMESYNC_LOCAL_TIME_LOAD |
        BCM_PORT_PHY_TIMESYNC_NCO_DIVIDER_LOAD |
        BCM_PORT_PHY_TIMESYNC_NCO_ADDEND_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K3_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K2_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K1_LOAD ));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

}


void config_port_syncout_1k(int unit, bcm_port_t port, int mode)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    sal_memset(&config, 0, sizeof(config));

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS | BCM_PORT_PHY_TIMESYNC_VALID_GMODE |
        BCM_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE |
        BCM_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE_DELTA |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K1 | BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K2 |
        BCM_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K3;

    config.flags = BCM_PORT_PHY_TIMESYNC_ENABLE | BCM_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE | 
                 BCM_PORT_PHY_TIMESYNC_L2_ENABLE | BCM_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE |
                 BCM_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT;

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLocalTime, 
         _compiler_64_arg(0xa33, 0x44556677));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.syncout.syncout_ts = _compiler_64_arg(0xa34, 0x00000000);
    config.syncout.interval = 25000;
    config.syncout.pulse_1_length = 0x800;
    config.syncout.pulse_2_length = 0x400;

    config.gmode = bcmPortPhyTimesyncModeFree;
    config.framesync.mode = bcmPortPhyTimesyncFramesyncNone;

    config.syncout.mode = bcmPortPhyTimesyncSyncoutDisable;
    config.tx_sync_mode = bcmPortPhyTimesyncEventMessageEgressModeUpdateCorrectionField;
    config.rx_sync_mode = bcmPortPhyTimesyncEventMessageIngressModeUpdateCorrectionField;

    /* for 1KHz SyncIn */
    config.phy_1588_dpll_ref_phase = _compiler_64_arg(0, 0);
    config.phy_1588_dpll_ref_phase_delta = 0x000f4240;
    config.phy_1588_dpll_k1 = 0x2a;
    config.phy_1588_dpll_k2 = 0x28;
    config.phy_1588_dpll_k3 = 0;

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncLoadControl, _compiler_64_arg(0 ,
        BCM_PORT_PHY_TIMESYNC_TN_LOAD |
        BCM_PORT_PHY_TIMESYNC_TIMECODE_LOAD |
        BCM_PORT_PHY_TIMESYNC_SYNCOUT_LOAD |
        BCM_PORT_PHY_TIMESYNC_LOCAL_TIME_LOAD | 
        BCM_PORT_PHY_TIMESYNC_NCO_DIVIDER_LOAD |
        BCM_PORT_PHY_TIMESYNC_NCO_ADDEND_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K3_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K2_LOAD |
        BCM_PORT_PHY_TIMESYNC_DPLL_K1_LOAD ));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_timesync_set(unit, port, bcmPortControlPhyTimesyncFrameSync, _compiler_64_arg(0,1));

    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_timesync_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_GMODE |
        BCM_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE | BCM_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE ;

    config.gmode = bcmPortPhyTimesyncModeCpu;
    config.framesync.mode = bcmPortPhyTimesyncFramesyncNone;

    if (mode == 3) {
        config.syncout.mode = bcmPortPhyTimesyncSyncoutPulseTrainWithSync;
    } else if (mode == 2) {
        config.syncout.mode = bcmPortPhyTimesyncSyncoutPulseTrain;
    } else {
        config.syncout.mode = bcmPortPhyTimesyncSyncoutOneTime;
    }

    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

}


void sync_clock_1k(int port1)
{
    config_port_sync_1k( 0, port1);

    print_local_time(0, port1);
    _print_heartbeat_ts(0, port1);
    _print_capture_ts(0, port1);
}

void sync_clock_4k(int port1)
{
    config_port_sync_4k( 0, port1);

    print_local_time(0, port1);
    _print_heartbeat_ts(0, port1);
    _print_capture_ts(0, port1);
}

void sync_clock_12_5m(int port1)
{
    config_port_sync_12_5m( 0, port1);

    print_local_time(0, port1);
    _print_heartbeat_ts(0, port1);
    _print_capture_ts(0, port1);
}

void config_inband_ts_tc_ip_cap(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS;
    
                                     
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
    
    config.inband_ts_control.flags = BCM_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_CHECK
                                 | BCM_PORT_PHY_TIMESYNC_INBAND_TS_SYNC_ENABLE
                                 | BCM_PORT_PHY_TIMESYNC_INBAND_TS_DELAY_RQ_ENABLE
                                 | BCM_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RQ_ENABLE
                                 | BCM_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RESP_ENABLE;
    config.inband_ts_control.resv0_id = 0xf000 | port;

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE;
    
    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}

void config_inband_ts_tc_da_cap(int unit, bcm_port_t port)
{
    int rv;
    bcm_port_phy_timesync_config_t config;

    config.validity_mask = BCM_PORT_PHY_TIMESYNC_VALID_FLAGS;
    
                                     
    rv = bcm_port_phy_timesync_config_get(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_get failed with error u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
    
    config.inband_ts_control.flags = BCM_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_CHECK
                                 | BCM_PORT_PHY_TIMESYNC_INBAND_TS_SYNC_ENABLE
                                 | BCM_PORT_PHY_TIMESYNC_INBAND_TS_DELAY_RQ_ENABLE
                                 | BCM_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RQ_ENABLE
                                 | BCM_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RESP_ENABLE
                                 | BCM_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_MAC_ADDR;
    config.inband_ts_control.resv0_id = 0xf000 | port;

    config.flags |= BCM_PORT_PHY_TIMESYNC_ENABLE;
    
    rv = bcm_port_phy_timesync_config_set(unit, port, &config);

    if (rv != BCM_E_NONE) {
       printk("bcm_port_phy_timesync_config_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }
}




/* PHY OAM tests */

/* Y.1731 */
void phy_oam_y1731_rx(int unit, int port) {
    int rv;
    bcm_port_config_phy_oam_t config;

    sal_memset(&config, 0, sizeof(soc_port_config_phy_oam_t));

    config.rx_dm_config.flags = 0;
    config.rx_dm_config.mode = bcmPortConfigPhyOamDmModeY1731;
    rv = bcm_port_config_phy_oam_set(unit, port, &config);
    if (rv != BCM_E_NONE) {
       printk("bcm_port_config_phy_oam_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_oam_set(unit, port, bcmPortControlPhyOamDmMacAddress1, _compiler_64_arg(0x00000000,0x00000100));
    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_oam_set failed with error  u=%d p=%d (type %d)%s\n", unit, port, bcmPortControlPhyOamDmMacAddress1, bcm_errmsg(rv));
    }
    
    rv = bcm_port_control_phy_oam_set(unit, port, bcmPortControlPhyOamDmRxPortMacAddressIndex, _compiler_64_arg(0, 0x1));
    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_oam_set failed with error  u=%d p=%d (type %d) %s\n", unit, port, bcmPortControlPhyOamDmRxPortMacAddressIndex, bcm_errmsg(rv));
    }
}

void phy_oam_y1731_tx(int unit, int port) {
    int rv;
    bcm_port_config_phy_oam_t config;

    sal_memset(&config, 0, sizeof(soc_port_config_phy_oam_t));

    config.tx_dm_config.flags = 0;
    config.tx_dm_config.mode = bcmPortConfigPhyOamDmModeY1731;
    rv = bcm_port_config_phy_oam_set(unit, port, &config);
    if (rv != BCM_E_NONE) {
       printk("bcm_port_config_phy_oam_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_oam_set(unit, port, bcmPortControlPhyOamDmMacAddress1, _compiler_64_arg(0x0000a000,0x00000100));
    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_oam_set failed with error  u=%d p=%d (type %d)%s\n", unit, port, bcmPortControlPhyOamDmMacAddress1, bcm_errmsg(rv));
    }
    
    rv = bcm_port_control_phy_oam_set(unit, port, bcmPortControlPhyOamDmTxPortMacAddressIndex, _compiler_64_arg(0,0x1));
    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_oam_set failed with error  u=%d p=%d (type %d) %s\n", unit, port, bcmPortControlPhyOamDmTxPortMacAddressIndex, bcm_errmsg(rv));
    }
}

void phy_oam_y1731_mac_da_rx(int unit, int port) {
    int rv;
    bcm_port_config_phy_oam_t config;

    sal_memset(&config, 0, sizeof(soc_port_config_phy_oam_t));

    config.rx_dm_config.flags = BCM_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE;
    config.rx_dm_config.mode = bcmPortConfigPhyOamDmModeY1731;
    rv = bcm_port_config_phy_oam_set(unit, port, &config);
    if (rv != BCM_E_NONE) {
       printk("bcm_port_config_phy_oam_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_oam_set(unit, port, bcmPortControlPhyOamDmMacAddress1, _compiler_64_arg(0x00000000,0x00000100));
    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_oam_set failed with error  u=%d p=%d (type %d)%s\n", unit, port, bcmPortControlPhyOamDmMacAddress1, bcm_errmsg(rv));
    }
    
    rv = bcm_port_control_phy_oam_set(unit, port, bcmPortControlPhyOamDmRxPortMacAddressIndex, _compiler_64_arg(0, 0x1));
    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_oam_set failed with error  u=%d p=%d (type %d) %s\n", unit, port, bcmPortControlPhyOamDmRxPortMacAddressIndex, bcm_errmsg(rv));
    }
}

void phy_oam_y1731_mac_sa_tx(int unit, int port) {
    int rv;
    bcm_port_config_phy_oam_t config;

    sal_memset(&config, 0, sizeof(soc_port_config_phy_oam_t));

    config.tx_dm_config.flags = BCM_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE;
    config.tx_dm_config.mode = bcmPortConfigPhyOamDmModeY1731;
    rv = bcm_port_config_phy_oam_set(unit, port, &config);
    if (rv != BCM_E_NONE) {
       printk("bcm_port_config_phy_oam_set failed with error  u=%d p=%d %s\n", unit, port, bcm_errmsg(rv));
    }

    rv = bcm_port_control_phy_oam_set(unit, port, bcmPortControlPhyOamDmMacAddress2, _compiler_64_arg(0x0000a000,0x00000100));
    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_oam_set failed with error  u=%d p=%d (type %d)%s\n", unit, port, bcmPortControlPhyOamDmMacAddress1, bcm_errmsg(rv));
    }
    
    rv = bcm_port_control_phy_oam_set(unit, port, bcmPortControlPhyOamDmTxPortMacAddressIndex, _compiler_64_arg(0, 0x2));
    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_oam_set failed with error  u=%d p=%d (type %d) %s\n", unit, port, bcmPortControlPhyOamDmTxPortMacAddressIndex, bcm_errmsg(rv));
    }
}

void phy_oam_ctrl_tx_ethtype(int unit, int port) {
    int rv;
 
    rv = bcm_port_control_phy_oam_set(unit, port,bcmPortControlPhyOamDmTxEthertype, _compiler_64_arg(0, 0xaaaa));
    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_oam_set failed with error  u=%d p=%d (type %d) %s\n", unit, port, bcmPortControlPhyOamDmTxEthertype, bcm_errmsg(rv));
    }

}
void phy_oam_ctrl_rx_ethtype(int unit, int port) {
    int rv;
 
    rv = bcm_port_control_phy_oam_set(unit, port,bcmPortControlPhyOamDmRxEthertype, _compiler_64_arg(0, 0xbbbb));
    if (rv != BCM_E_NONE) {
       printk("bcm_port_control_phy_oam_set failed with error  u=%d p=%d (type %d) %s\n", unit, port, bcmPortControlPhyOamDmRxEthertype, bcm_errmsg(rv));
    }
}

void phy_oam_tests(void) {
    printk(" \n\tTests:\n");
    printk("1.1. phy_oam_y1731_rx\n");
    printk("1.2. phy_oam_y1731_tx\n");
    printk("1.3. phy_oam_y1731_mac_da_rx\n");
    printk("1.4. phy_oam_y1731_mac_sa_tx\n");
    printk("1.5. phy_oam_ctrl_rx_ethtype\n");
    printk("1.6. phy_oam_ctrl_tx_ethtype\n");
}

#endif
