/*
 * $Id: bcm-knet-kcom.c,v 1.4 Broadcom SDK $
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
 * File: 	kcom.c
 * Purpose: 	Provides a kcom interface using device IOCTL
 */

#ifdef INCLUDE_KNET

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <kcom.h>

#include <bcm-knet.h>

#define KNET_DEVICE_NAME "/dev/linux-bcm-knet"
#define KNET_DEVICE_MAJOR 122
#define KNET_DEVICE_MINOR 0

static int kcom_fd = -1;

char *
bcm_knet_kcom_open(char *name)
{ 
    if (kcom_fd >= 0) {
	return name;
    }

    if ((kcom_fd = open(KNET_DEVICE_NAME,
                        O_RDWR | O_SYNC | O_DSYNC | O_RSYNC)) < 0) {
	perror("open " KNET_DEVICE_NAME ": ");
	return NULL;
    }	
    
    return name;
}

int
bcm_knet_kcom_close(void *handle)
{
    close(kcom_fd);

    return 0;
}

int
bcm_knet_kcom_msg_send(void *handle, void *msg,
                       unsigned int len, unsigned int bufsz)
{
    bkn_ioctl_t io;

    io.len = len;
    io.bufsz = bufsz;

    /*
     * Pass pointer as 64-bit int in order to support 32-bit
     * applications running on a 64-bit kernel.
     */
    io.buf = (uint64_t)(unsigned long)msg;

    if (ioctl(kcom_fd, 0, &io) < 0) {
	perror("send " KNET_DEVICE_NAME ": ");
        return -1;
    }

    if (io.rc < 0) {
        return -1;
    }
    return io.len;
}

int
bcm_knet_kcom_msg_recv(void *handle, void *msg,
                       unsigned int bufsz)
{
    bkn_ioctl_t io;

    io.len = 0;
    io.bufsz = bufsz;

    /*
     * Pass pointer as 64-bit int in order to support 32-bit
     * applications running on a 64-bit kernel.
     */
    io.buf = (uint64_t)(unsigned long)msg;

    do {
        if (ioctl(kcom_fd, 0, &io) < 0) {
            perror("recv " KNET_DEVICE_NAME ": ");
            return -1;
        }

        if (io.rc < 0) {
            return -1;
        }
    } while (io.len == 0);

    return io.len;
}

#endif

int _bcm_knet_kcom_c_not_empty;
