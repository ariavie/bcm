/*
 * $Id: vxbde_emmi.h 1.3 Broadcom SDK $
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
 */        
#ifndef _VXBDE_EMMI_H
#define _VXBDE_EMMI_H
#include <sal/types.h>
#ifdef VXWORKS
#include <netinet/in.h>
#include "vxWorks.h"
#include "sockLib.h"
#include "inetLib.h"
#include "in.h"
#include "taskLib.h"
#include "stdioLib.h"
#include "strLib.h"
#include "ioLib.h"
#include "fioLib.h"
#define EMMI_ERROR(x)               printf x    
#endif

#ifdef UNIX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h> 
#define EMMI_ERROR(x)                   printf x
#define EMMI_DBG_LOG()                  printf("%s,%d\n",__FUNCTION__,__LINE__)
#endif

#define EMMI_DRV_ERR                    -1
#define EMMI_DRV_ERR_NONE               0

#define BDE_CORE_NUM_PER_EMMI_DEVICE    2
#define BDE_MAX_EMMI_DEVICES            4
#define BDE_EMMI_INVALID_BASE_UNIT      -1
#define BDE_EMMI_INVALID_SOCKET_FD      -1
#define BDE_EMMI_PORT_MASK              0x0000FFFF
#define EMMI_SOCKET_ALLOW_CONNECTED_NUM 20

#define BDE_EMMI_CORE_ID(x)             (((x)&0xF0000000)>>28)
#define BDE_EMMI_OPCODE(x)              ((x)&0x0FFFFFFF)

#define BDE_EMMI_PARA_PACK(m, c, u) \
    ((m)&0xFF)<<28 | ((c&0xFF)<<16) | ((u&0xFF)<<8)

typedef enum {
    bdeEmmiModeInvalid = 0,
    bdeEmmiModeAsyncbus,
    bdeEmmiModeSocket,
    bdeEmmiModeCount
} bde_emmi_mode_t;

typedef enum {
    bdeEmmiOpInit   = BDE_DEV_OP_EMMI_INIT,
    bdeEmmiOpParGet,
    bdeEmmiOpDataRead,
    bdeEmmiOpDataWrite,
    bdeEmmiOpCount    
} bde_emmi_op_t;

typedef uint16 _bde_emmi_ports_t[BDE_CORE_NUM_PER_EMMI_DEVICE];

typedef struct {
    uint16 port[BDE_CORE_NUM_PER_EMMI_DEVICE];
} _bde_emmi_device_port_t; 

typedef enum {
    bdeEmmiSocketStateUninit = 0,
    bdeEmmiSocketStateInit,
    bdeEmmiSocketStateConnecting,
    bdeEmmiSocketStateConnected,
    bdeEmmiSocketStateDisconnect,
    bdeEmmiSocketStateCount
} bde_emmi_socket_state_t;

typedef struct bde_emmi_socket_ctrl_s{
    uint8 unit;
    uint8 core;

    int state;

    int server_fd;
    struct sockaddr_in sin;
    int client_fd;
    struct sockaddr_in cin;

    int (*init)(struct bde_emmi_socket_ctrl_s *);
    int (*read)(struct bde_emmi_socket_ctrl_s *, uint8 *, int, int);
    int (*write)(struct bde_emmi_socket_ctrl_s *, uint8 *, int, int);
} bde_emmi_socket_ctrl_t;

typedef struct {
    bde_emmi_socket_ctrl_t sock_ctrl[BDE_CORE_NUM_PER_EMMI_DEVICE];
} bde_emmi_device_ctrl_t;

extern void _bde_emmi_socket_conf_init(void);

extern void _bde_emmi_dev_init(int emmi_mode, int emmi_count, int emmi_base_unit);

extern int _bde_emmi_dev_port_set(int unit, int core, uint16 port);

extern uint16 _bde_emmi_dev_port_get(int unit, int core);

extern void _bde_emmi_dev_setup(void);

extern int _bde_emmi_read(int d, uint32 addr, uint8 *buf, int len);

extern int _bde_emmi_write(int d, uint32 addr, uint8 *buf, int len);

#endif /* _VXBDE_EMMI_H */

