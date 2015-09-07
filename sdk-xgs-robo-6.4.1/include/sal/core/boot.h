/*
 * $Id: boot.h,v 1.18.152.2 Broadcom SDK $
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
 * File: 	boot.h
 * Purpose: 	Boot definitions
 */

#ifndef _SAL_BOOT_H
#define _SAL_BOOT_H

#include <sal/types.h>

/*
 * Boot flags.
 */

#define BOOT_F_NO_RC            0x1000
#define BOOT_F_NO_PROBE         0x2000
#define BOOT_F_NO_ATTACH        0x4000
#define BOOT_F_SHELL_ON_TRAP    0x8000 /* TTY trap char: no reboot; shell */
#define BOOT_F_QUICKTURN        0x10000
#define BOOT_F_PLISIM           0x20000
#define BOOT_F_RTLSIM           0x80000
#define BOOT_F_RELOAD           0x100000
#define BOOT_F_WARM_BOOT        0x200000
#define BOOT_F_BCMSIM           0x400000
#define BOOT_F_XGSSIM           0x800000
#define BOOT_F_NO_INTERRUPTS    0x1000000

extern uint32 sal_boot_flags_get(void);
extern void sal_boot_flags_set(uint32);
extern char *sal_boot_script(void);

#define sal_boot_flags  sal_boot_flags_get  /* Deprecated */

#define SAL_BOOT_SIMULATION  \
    (sal_boot_flags_get() & (BOOT_F_QUICKTURN|BOOT_F_PLISIM|BOOT_F_RTLSIM|BOOT_F_XGSSIM))
#define SAL_BOOT_QUICKTURN  \
    (sal_boot_flags_get() & BOOT_F_QUICKTURN)
#define SAL_BOOT_PLISIM  \
    (sal_boot_flags_get() & (BOOT_F_PLISIM|BOOT_F_RTLSIM))
#define SAL_BOOT_RTLSIM  \
    (sal_boot_flags_get() & BOOT_F_RTLSIM)
#define SAL_BOOT_BCMSIM  \
    (sal_boot_flags_get() & BOOT_F_BCMSIM)
#define SAL_BOOT_XGSSIM  \
    (sal_boot_flags_get() & BOOT_F_XGSSIM)

#define SAL_BOOT_NO_INTERRUPTS   \
    (sal_boot_flags_get() & BOOT_F_NO_INTERRUPTS)

/*
 * Assertion Handling
 */

typedef void (*sal_assert_func_t)
     (const char* expr, const char* file, int line);

extern void sal_assert_set(sal_assert_func_t f);
extern void _default_assert(const char *expr, const char *file, int line);


/*
 * Init
 */
extern int sal_core_init(void);


/*
 * Returns a string describing the current Operating System
 */
extern const char* sal_os_name(void); 


#endif	/* !_SAL_BOOT_H */


