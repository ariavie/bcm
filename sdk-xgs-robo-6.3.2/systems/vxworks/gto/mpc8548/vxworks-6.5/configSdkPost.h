/*
 * $Id: configSdkPost.h 1.6 Broadcom SDK $
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
 * Description: This file configure the VxWorks kernel to includes
 *              additional components necessary for SDK.
 *              Includes this file immediately after configAll.h
 *              in your BSP config.h
 */

#ifndef _SDK_CONFIG_POST_H_
#define _SDK_CONFIG_POST_H_

#ifdef BROADCOM_BSP
#if 0
#define INCLUDE_LOGGING
#define INCLUDE_LOG_STARTUP
#endif

/* Include NFS support */
#define  INCLUDE_NFS
#undef  INCLUDE_NFS_CLIENT_ALL  /* Include All NFS clients */
#undef  INCLUDE_NFS3_CLIENT     /* Include NFS v3 client */
#undef  INCLUDE_NFS_MOUNT_ALL   /* Include Mount all exported FS */
#define INCLUDE_NFS2_CLIENT     /* Incldue NFS v2 client */



/* Including essential network components */
#define IP_PORT_VXWORKS 65
#define INCLUDE_BSD_SOCKET
#define INCLUDE_ETHERNET
#define INCLUDE_GTF_TIMER_START
#define INCLUDE_INET_LIB
#define INCLUDE_IPV4
#define INCLUDE_MIB2_IF
#define INCLUDE_NBIO_LOG
#define INCLUDE_NET_BOOT_CONFIG
#define INCLUDE_NET_DAEMON
#define INCLUDE_NET_REM_IO
#define INCLUDE_NET_SYSCTL
#define INCLUDE_XDR
#define INCLUDE_NETWORK
#define INCLUDE_NET_INIT
#define INCLUDE_BOOT_LINE_INIT
#define INCLUDE_RPC
#define INCLUDE_POSIX_AIO
#define INCLUDE_POSIX_AIO_SYSDRV

#ifdef INCLUDE_IPV4
#define INCLUDE_IPATTACH
#define INCLUDE_ICMPV4
#define INCLUDE_PING
#define INCLUDE_RAWV4
#define INCLUDE_TCPV4
#define INCLUDE_UDPV4
#endif /* INCLUDE_IPV4 */

#ifdef INCLUDE_IPV6
#define INCLUDE_IP6ATTACH
#define INCLUDE_ICMPV6
#define INCLUDE_PING6
#define INCLUDE_RAWV6
#define INCLUDE_TCPV6
#define INCLUDE_UDPV6
#define INCLUDE_MLD
#endif /* INCLUDE_IPV6 */

/* Including Application Components */
#define INCLUDE_DHCPC_LEASE_CLEAN
#define INCLUDE_APPL_LOG_UTIL
#define INCLUDE_NET_GETOPT
#define INCLUDE_STRICMP

/* Including Network Driver */
#define INCLUDE_MIILIB

/* Networking include dependicies */
#ifdef INCLUDE_BSD_SOCKET
#define INCLUDE_SOCKLIB
#define INCLUDE_NET_POOL
#endif /* INCLUDE_BSD_SOCKET */

#ifdef INCLUDE_NET_POOL
#define INCLUDE_NETBUFLIB
#define INCLUDE_NETBUFPOOL
#endif /* INCLUDE_NET_POOL */

#ifdef INCLUDE_ETHERNET
#define INCLUDE_MUX
#define INCLUDE_END
#endif /* INCLUDE_ETHERNET */

#ifdef INCLUDE_GTF_TIMER_START
#define INCLUDE_GTF
#endif /* INCLUDE_GTF_TIMER_START */

#ifdef INCLUDE_NET_BOOT_CONFIG
#define INCLUDE_NET_BOOT
#define INCLUDE_NET_BOOT_CONFIG
#define INCLUDE_IPATTACH
#define INCLUDE_NETMASK_GET
#define INCLUDE_SOCKLIB
#define INCLUDE_NET_INIT
#define INCLUDE_NETWORK
#endif /* INCLUDE_NET_BOOT_CONFIG */

#ifdef INCLUDE_NET_REM_IO
#define INCLUDE_NET_DRV
#define INCLUDE_BOOT_LINE_INIT
#define INCLUDE_NET_BOOT
#define INCLUDE_NET_HOST_SETUP
#endif /* INCLUDE_NET_REM_IO */

#ifdef  STANDALONE
#define STANDALONE_NET
#endif  /* STANDALONE */


/* For debugging from VxShell */
#define INCLUDE_NET_SHOW
#define INCLUDE_IFMEDIA
#define INCLUDE_VXBUS_SHOW
#define INCLUDE_IFCONFIG
#define INCLUDE_NET_IF_SHOW
#define INCLUDE_SHOW_ROUTINES
#define INCLUDE_PCI_CFGSHOW
#define INCLUDE_ISR_OBJECTS

/* For flash file system support */
#define INCLUDE_XBD             /* Extended block device */
#define INCLUDE_XBD_PART_LIB    /* XBD Partition Library */
#define INCLUDE_XBD_BLK_DEV     /* XBD-block device wrapper */
#define INCLUDE_XBD_RAMDRV      /* XBD Ram Drive */

/* Initialize SDK and start Broadcom shell */
#ifndef BOOTAPP
#define INCLUDE_USER_APPL
#define USER_APPL_INIT { extern void vxSpawn(void); vxSpawn(); }
#endif

#define INCLUDE_SHELL
#define INCLUDE_DEBUG
#define INCLUDE_SHELL_VI_MODE
#define INCLUDE_SHELL_INTERP_C
#define INCLUDE_SHELL_INTERP_CMD
#define INCLUDE_SYM_TBL_SHOW

/* For vxshell configuration */
#undef  INCLUDE_SHELL_BANNER
#undef  SHELL_START_AT_BOOT        /* Defined in configAll.h */
#define SHELL_START_AT_BOOT FALSE  /* Do not start VxShell at boot */

#endif /* BROADCOM_BSP */

#endif /* _SDK_CONFIG_POST_H_ */
