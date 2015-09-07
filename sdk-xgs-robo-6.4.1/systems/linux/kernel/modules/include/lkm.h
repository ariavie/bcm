/*
 * $Id: lkm.h,v 1.22 Broadcom SDK $
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

#ifndef __COMMON_LINUX_KRN_LKM_H__
#define __COMMON_LINUX_KRN_LKM_H__

#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/init.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#include <linux/config.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)
#include <linux/kconfig.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
#include <linux/smp_lock.h>
#endif
#include <linux/module.h>

/* Helper defines for multi-version kernel  support */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#define LKM_2_4
#else
#define LKM_2_6
#endif

#include <linux/kernel.h>   /* printk() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/stat.h>
#include <linux/sched.h>

#include <asm/io.h>
#include <asm/hardirq.h>
#include <asm/uaccess.h>

#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

/* Compatibility Macros */

#ifdef LKM_2_4

#include <linux/compatmac.h>
#include <linux/wrapper.h>
#define LKM_MOD_PARAM(n,ot,nt,d) MODULE_PARM(n,ot)
#define LKM_MOD_PARAM_ARRAY(n,ot,nt,c,d) MODULE_PARM(n,ot)
#define LKM_EXPORT_SYM(s)
#define _free_netdev kfree

#else /* LKM_2_6 */

#define LKM_MOD_PARAM(n,ot,nt,d) module_param(n,nt,d)
#define LKM_MOD_PARAM_ARRAY(n,ot,nt,c,d) module_param_array(n,nt,c,d)
#define LKM_EXPORT_SYM(s) EXPORT_SYMBOL(s)
#define _free_netdev free_netdev

#endif /* LKM_2_x */

#ifndef list_for_each_safe
#define list_for_each_safe(l,t,i) t = 0; list_for_each((l),(i))
#endif

#ifndef reparent_to_init
#define reparent_to_init()
#endif

#ifndef MODULE_LICENSE
#define MODULE_LICENSE(str)
#endif

#ifndef EXPORT_NO_SYMBOLS
#define EXPORT_NO_SYMBOLS
#endif

#ifndef DEFINE_SPINLOCK
#define DEFINE_SPINLOCK(_lock) spinlock_t _lock = SPIN_LOCK_UNLOCKED
#endif

#ifndef __SPIN_LOCK_UNLOCKED
#define __SPIN_LOCK_UNLOCKED(_lock) SPIN_LOCK_UNLOCKED
#endif

#ifndef lock_kernel
#ifdef preempt_disable
#define lock_kernel() preempt_disable()
#else
#define lock_kernel()
#endif
#endif

#ifndef unlock_kernel
#ifdef preempt_enable
#define unlock_kernel() preempt_enable()
#else
#define unlock_kernel()
#endif
#endif

#ifndef init_MUTEX_LOCKED
#define init_MUTEX_LOCKED(_sem) sema_init(_sem, 0)
#endif

#ifdef CONFIG_BCM98245
#define CONFIG_BMW
#endif

#endif /* __COMMON_LINUX_KRN_LKM_H__ */
