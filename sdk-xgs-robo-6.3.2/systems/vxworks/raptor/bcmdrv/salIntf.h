/*
 * $Id: salIntf.h 1.5 Broadcom SDK $
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
#include <vxWorks.h>
#include <cacheLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysLib.h>
#include <string.h>


#ifndef SALINTF_H
#define SALINTF_H

#define	SAL_THREAD_STKSZ	16384	/* Default Stack Size */

#define sal_dma_alloc(s,c)  (cacheDmaMalloc((( (s) + 3) >> 2) << 2) )
#define sal_dma_free(s)     (cacheDmaFree(s))
#define sal_alloc(s,c)    (malloc((s)))
#define sal_free(s)       (free((s)))
#define sal_memset(d,v,l) (memset((d),(v),(l)))
#define sal_memcpy(d,s,l) (memcpy((d),(s),(l)))
#define soc_cm_salloc(u,s,c)  sal_dma_alloc(s,c)
#define soc_cm_sfree(u,s)   sal_dma_free(s)
#define soc_cm_sflush(d,a,l)  sal_dma_flush((a),(l))
#define soc_cm_sinval(d,a,l)  sal_dma_inval((a),(l))

typedef struct sal_thread_s{
    char thread_opaque_type;
} *sal_thread_t;

sal_thread_t sal_thread_create(char *name, int ss, int prio, void (f)(void *), void *arg);
int sal_thread_destroy(sal_thread_t thread);

typedef struct sal_mutex_s{
    char mutex_opaque_type;
} *sal_mutex_t;

#define sal_mutex_FOREVER	(-1)
#define sal_mutex_NOWAIT	0

sal_mutex_t 	sal_mutex_create(char *desc);
void		    sal_mutex_destroy(sal_mutex_t m);
int		        sal_mutex_take(sal_mutex_t m, int usec);
int		        sal_mutex_give(sal_mutex_t m);



int sal_spl(int level);
int sal_splhi(void);

typedef struct sal_sem_s{
    char sal_opaque_type;
} *sal_sem_t;

#define sal_sem_FOREVER		(-1)
#define sal_sem_BINARY		1
#define sal_sem_COUNTING	0

sal_sem_t	sal_sem_create(char *desc, int binary, int initial_count);
void		sal_sem_destroy(sal_sem_t b);
int		sal_sem_take(sal_sem_t b, int usec);
int		sal_sem_give(sal_sem_t b);

unsigned int sal_time_usecs(void);

void sal_thread_exit(int rc);


void sal_dma_flush(void *addr, int len);
void sal_dma_inval(void *addr, int len);

#endif
