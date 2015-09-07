/*
 * $Id: linux-uk-proxy.h,v 1.10 Broadcom SDK $
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

#ifndef __LINUX_UK_PROXY_H__
#define __LINUX_UK_PROXY_H__

#ifndef __KERNEL__
#include <stdint.h>
#endif

/* Device Info */
#define LUK_DEVICE_NAME "/dev/linux-uk-proxy"
#define LUK_DEVICE_MAJOR 125
#define LUK_DEVICE_MINOR 0

#define LUK_MAX_SERVICE_NAME 64
#define LUK_MAX_DATA_SIZE (2*1024)

/* Control structure passed to all ioctls */
typedef struct {
    uint32_t v;
} luk_ioctl_version_t;

typedef struct {
    uint32_t q_size;
    uint32_t flags;
} luk_ioctl_create_t;

typedef struct {
    uint32_t flags;
} luk_ioctl_destroy_t;

typedef struct {
    uint64_t data;
    uint32_t len;
} luk_ioctl_recv_t;

typedef struct {
    uint64_t data;
    uint32_t len;
} luk_ioctl_send_t;    
    
typedef struct  {
    char service[LUK_MAX_SERVICE_NAME]; 
    int rc;                               /* Operation Return Code */    
    union {
        luk_ioctl_version_t version;
        luk_ioctl_create_t  create;
        luk_ioctl_destroy_t destroy;
        luk_ioctl_recv_t    recv;
        luk_ioctl_send_t    send;
    } args;
} luk_ioctl_t;

/* Linux User/Kernel Proxy ioctls */
#define LUK_MAGIC 'L'

#define LUK_VERSION 		_IO(LUK_MAGIC, 0)
#define LUK_SERVICE_CREATE	_IO(LUK_MAGIC, 1)
#define LUK_SERVICE_DESTROY	_IO(LUK_MAGIC, 2)
#define LUK_SERVICE_RECV	_IO(LUK_MAGIC, 3)
#define LUK_SERVICE_SEND	_IO(LUK_MAGIC, 4)
#define LUK_SERVICE_SUSPEND	_IO(LUK_MAGIC, 5)
#define LUK_SERVICE_RESUME	_IO(LUK_MAGIC, 6)

#ifdef __KERNEL__

#include <linux/ioctl.h>

/* Kernel API */
extern int linux_uk_proxy_service_create(const char *service, uint32_t q_size, uint32_t flags); 
extern int linux_uk_proxy_send(const char *service, void *data, uint32_t len); 
extern int linux_uk_proxy_recv(const char *service, void *data, uint32_t *len); 
extern int linux_uk_proxy_service_destroy(const char *service); 

#else

/* User API */
extern int linux_uk_proxy_open(void); 
extern int linux_uk_proxy_service_create(const char *service, uint32_t q_size, uint32_t flags); 
extern int linux_uk_proxy_service_destroy(const char *service); 
extern int linux_uk_proxy_send(const char *service, void *data, uint32_t len); 
extern int linux_uk_proxy_recv(const char *service, void *data, uint32_t *len); 
extern int linux_uk_proxy_suspend(const char *service); 
extern int linux_uk_proxy_resume(const char *service); 
extern int linux_uk_proxy_close(void); 

#endif /* __KERNEL__ */

#endif /* __LINUX_UK_PROXY_H__ */
