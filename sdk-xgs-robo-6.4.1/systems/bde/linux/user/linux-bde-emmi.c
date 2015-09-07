/*
 * $Id: linux-bde-emmi.c,v 1.3 Broadcom SDK $
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
#include <sal/types.h> 
#include <ibde.h>
#include "linux-bde-emmi.h"

static uint32 _bde_emmi_mode = bdeEmmiModeInvalid;
static _bde_emmi_device_port_t _bde_emmi_ports[BDE_MAX_EMMI_DEVICES];

static uint16 _bde_emmi_dev_count = 0;
static int _bde_emmi_base_unit = BDE_EMMI_INVALID_BASE_UNIT;
static bde_emmi_device_ctrl_t _bde_emmi_device_ctrl[BDE_MAX_EMMI_DEVICES];

int
_bde_emmi_socket_read(bde_emmi_socket_ctrl_t *ctrl, uint8 *buf, int len, int flag)
{
    int rv = -1;
    int address_len;
    
    if (NULL == ctrl || buf == NULL) {
        EMMI_ERROR(("Null pointer\n"));
        return -1;
    }

    if (ctrl->state == bdeEmmiSocketStateConnected) {
        rv = recv(ctrl->client_fd, buf, len, flag);
        if(rv <= 0){
            EMMI_ERROR(("Socket disconnected\n"));
            ctrl->state = bdeEmmiSocketStateDisconnect;
            ctrl->client_fd = -1;
        } else {
            return rv;
        }
    }

    switch(ctrl->state){
        case bdeEmmiSocketStateUninit:
        case bdeEmmiSocketStateConnecting:
            EMMI_ERROR(("Socket state %d\n",ctrl->state));
            break;
        case bdeEmmiSocketStateInit:
        case bdeEmmiSocketStateDisconnect:
            while (ctrl->client_fd < 0) {
                ctrl->state = bdeEmmiSocketStateConnecting;
                address_len = sizeof(struct sockaddr_in);
                ctrl->client_fd = accept(ctrl->server_fd,
                    (struct sockaddr*)&(ctrl->cin), (socklen_t *)&address_len);
            }
            ctrl->state = bdeEmmiSocketStateConnected;
            rv = -1;
            break;
        default:
            rv  = -1;
             break;
    }
    
    return rv;
}

int
_bde_emmi_socket_write(bde_emmi_socket_ctrl_t *ctrl, uint8 *buf, int len, int flag)
{
    int address_len;

    if (NULL == ctrl || buf == NULL) {
        return -1;
    }

    if (ctrl->state == bdeEmmiSocketStateConnected) {
        return send(ctrl->client_fd, buf, len, flag);
    } else {
        switch (ctrl->state) {
            case bdeEmmiSocketStateUninit:
            case bdeEmmiSocketStateConnecting:
                EMMI_ERROR(("Socket state %d\n",ctrl->state));
                break;
            case bdeEmmiSocketStateInit:
            case bdeEmmiSocketStateDisconnect:
                while (ctrl->client_fd < 0) {
                    ctrl->state = bdeEmmiSocketStateConnecting;
                    address_len = sizeof(struct sockaddr_in);
                    ctrl->client_fd = accept(ctrl->server_fd,
                            (struct sockaddr*)&(ctrl->cin),
                            (socklen_t *)&address_len);
                }
                ctrl->state = bdeEmmiSocketStateConnected;
                break;
            default:
                break;
        }
    }

    if (ctrl->state == bdeEmmiSocketStateConnected) {
        return send(ctrl->client_fd, buf, len, flag);
    } else {
        return -1;
    }
}

int
_bde_emmi_socket_init(bde_emmi_socket_ctrl_t *ctrl)
{
    uint8 unit;
    uint8 core;
    uint16 port;
    
    if (NULL == ctrl) {
        EMMI_ERROR(("null ctrl.\n"));
        return -1;
    }

    unit = ctrl->unit;
    core = ctrl->core;
    
    port = _bde_emmi_dev_port_get(unit, core);
    
    if (port == 0) {
        EMMI_ERROR(("port num error.\n"));
        return -1;    
    }
    
    ctrl->server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (-1 == ctrl->server_fd) {
        EMMI_ERROR(("Create socket failed.\n"));
        return -1;
    }

    bzero((void *)&(ctrl->sin),sizeof(ctrl->sin));
    ctrl->sin.sin_family = AF_INET;
    ctrl->sin.sin_addr.s_addr = htonl(INADDR_ANY);
    ctrl->sin.sin_port = htons(port);
    if (bind(ctrl->server_fd, (struct sockaddr *)&(ctrl->sin),
        sizeof(ctrl->sin))) {
        EMMI_ERROR(("Bind server socket failed.\n"));
        return -1;
    }

    if (listen(ctrl->server_fd, EMMI_SOCKET_ALLOW_CONNECTED_NUM) == -1) {
        EMMI_ERROR(("Listen failed.\n"));
        return -1;
    }

    ctrl->state = bdeEmmiSocketStateInit;
    ctrl->client_fd = -1;

    return 0;
}

void
_bde_emmi_socket_conf_init(void)
{
    int n_emmi;
    int n_core;
    bde_emmi_socket_ctrl_t *sc;
    
    if (_bde_emmi_mode != bdeEmmiModeSocket){
        return ;
    }
    
    memset((void *)&_bde_emmi_device_ctrl, 0x0, sizeof(_bde_emmi_device_ctrl));
    
    for (n_emmi = 0; n_emmi < _bde_emmi_dev_count; n_emmi++) {
        for (n_core = 0; n_core < BDE_CORE_NUM_PER_EMMI_DEVICE; n_core++) { 
            sc = &_bde_emmi_device_ctrl[n_emmi].sock_ctrl[n_core];
            sc->unit = n_emmi;
            sc->core = n_core;
            sc->init = _bde_emmi_socket_init;
            sc->read = _bde_emmi_socket_read;
            sc->write = _bde_emmi_socket_write;
        }
    }

    return;
}

void 
_bde_emmi_dev_init(int emmi_mode, int emmi_count, int emmi_base_unit)
{
    _bde_emmi_base_unit = emmi_base_unit;
    _bde_emmi_dev_count = emmi_count;
    _bde_emmi_mode = emmi_mode;
    
    return;  
}

int
_bde_emmi_dev_port_set(int unit, int core, uint16 port)
{
    if ((unit >= 0) 
        && (unit < BDE_MAX_EMMI_DEVICES) 
        && (core >= 0) 
        && (core < BDE_CORE_NUM_PER_EMMI_DEVICE)) {
        _bde_emmi_ports[unit].port[core] = port;
        return 0;               
    }        
    
    return -1;
}

uint16 
_bde_emmi_dev_port_get(int unit, int core)
{
    if ((unit >= 0) 
        && (unit < BDE_MAX_EMMI_DEVICES) 
        && (core >= 0) 
        && (core < BDE_CORE_NUM_PER_EMMI_DEVICE)) {
        return _bde_emmi_ports[unit].port[core];;
    }        
    
    return 0;
}

int
_bde_emmi_read(int d, uint32 addr, uint8 *buf, int len)
{
    int unit = d - _bde_emmi_base_unit;
    int core = BDE_EMMI_CORE_ID(addr);
    bde_emmi_socket_ctrl_t *sc;
    uint32 bde_par = 0;
    int flag = 0;
    
    if (unit < 0 || unit >= BDE_MAX_EMMI_DEVICES) {
        EMMI_ERROR(("unit error\n"));
        return -1;
    }

    if (core >= BDE_CORE_NUM_PER_EMMI_DEVICE) {
        EMMI_ERROR(("core error\n"));
        return -1;
    }
    
    sc = &(_bde_emmi_device_ctrl[unit].sock_ctrl[core]);
    
    switch (BDE_EMMI_OPCODE(addr)) {
        case bdeEmmiOpDataRead:
            if (sc->read) {
                return sc->read(sc, buf, len, flag);    
            }
            break;
        case bdeEmmiOpParGet:
            if (buf != NULL && len == sizeof(uint32)) {
                bde_par = BDE_EMMI_PARA_PACK(_bde_emmi_mode,
                    BDE_CORE_NUM_PER_EMMI_DEVICE, _bde_emmi_base_unit);
                memcpy(buf, &bde_par, sizeof(uint32));
                return sizeof(uint32);
            }
            break;
        default:
            break;            
    } 
    
    return -1;
}

int
_bde_emmi_write(int d, uint32 addr, uint8 *buf, int len)
{
    int unit = d - _bde_emmi_base_unit;
    int core = BDE_EMMI_CORE_ID(addr);
    bde_emmi_socket_ctrl_t *sc;
    int flag = 0;
     
    if (unit < 0 || unit >= BDE_MAX_EMMI_DEVICES) {
        EMMI_ERROR(("unit error\n"));
        return -1;
    }
    
    if (core >= BDE_CORE_NUM_PER_EMMI_DEVICE) {
        EMMI_ERROR(("core error\n"));
        return -1;
    }

    sc = &(_bde_emmi_device_ctrl[unit].sock_ctrl[core]);
    
    switch (BDE_EMMI_OPCODE(addr)) {
        case bdeEmmiOpInit:
            if (sc->init) {
                return sc->init(sc);      
            }
            break;
        default:
            if (sc->write) {
                return sc->write(sc, buf, len, flag);   
            }
            break;        
    }
    
    return -1;
}

