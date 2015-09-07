/*
 * $Id: linux-uk-proxy.c,v 1.9 Broadcom SDK $
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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <string.h>

#include <linux-uk-proxy.h>

static int _dev_fd = -1;

int 
linux_uk_proxy_open(void)
{ 
    if (_dev_fd >= 0) {
	return _dev_fd;
    }

    if ((_dev_fd = open(LUK_DEVICE_NAME, O_RDWR | O_SYNC | O_DSYNC | O_RSYNC)) < 0) {
	/* Try inserting it from the local directory */
	perror("open " LUK_DEVICE_NAME ": ");
	return -1;
    }	
    
    return _dev_fd;
}

int
linux_uk_proxy_close(void)
{
    close(_dev_fd);
    return 0;
}

int
linux_uk_proxy_service_create(const char* service, unsigned int q_size, unsigned int flags)
{
    luk_ioctl_t io;

    strcpy(io.service, service);
    io.args.create.q_size = q_size;
    io.args.create.flags  = flags;
    if (ioctl(_dev_fd, LUK_SERVICE_CREATE, &io) >= 0) {
        return (io.rc);
    }

    return (-1);
}

int
linux_uk_proxy_service_destroy(const char* service)
{
    luk_ioctl_t io;

    strcpy(io.service, service);
    if (ioctl(_dev_fd, LUK_SERVICE_DESTROY, &io) >= 0) {
        return (io.rc);
    }

    return (-1);
}
	
int
linux_uk_proxy_recv(const char* service, void* data, unsigned int* len)
{
    luk_ioctl_t io;

    strcpy(io.service, service);
    io.args.recv.data = (uint64_t)(unsigned long)data;
    io.args.recv.len  = *len;

    /*
     * Return code -2 means that a signal was received (e.g. SIGSTOP 
     * from GDB). In this case no message is ready, and the operation
     * should simply be retried.
     */
    do {
        if (ioctl(_dev_fd, LUK_SERVICE_RECV, &io) < 0) {
            return -1;
        }
    } while (io.rc == -2);

    *len = io.args.recv.len;
    return io.rc;
}

int
linux_uk_proxy_send(const char* service, void* data, unsigned int len)
{
    luk_ioctl_t io;

    strcpy(io.service, service);
    io.args.send.data = (uint64_t)(unsigned long)data;
    io.args.send.len  = len;

    /*
     * Return code -2 means that a signal was received (e.g. SIGSTOP 
     * from GDB). In this case the message was not sent, and the 
     * operation should simply be retried.
     */
    do {
        if (ioctl(_dev_fd, LUK_SERVICE_SEND, &io) < 0) {
            return -1;
        }
    } while (io.rc == -2);

    return io.rc;
}

int
linux_uk_proxy_suspend(const char* service)
{
    luk_ioctl_t io;

    strcpy(io.service, service);

    ioctl(_dev_fd, LUK_SERVICE_SUSPEND, &io);
    return io.rc;
}

int
linux_uk_proxy_resume(const char* service)
{
    luk_ioctl_t io;

    strcpy(io.service, service);

    ioctl(_dev_fd, LUK_SERVICE_RESUME, &io);
    return io.rc;
}
