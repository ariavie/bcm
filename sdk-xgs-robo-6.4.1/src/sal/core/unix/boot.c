/*
 * $Id: boot.c,v 1.20 Broadcom SDK $
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
 * File: 	boot.c
 * Purpose:	Kernel initialization
 */

#include <stdio.h>
#include <stdlib.h>


#include <soc/debug.h>



#include <sal/core/libc.h>
#include <sal/core/boot.h>
#include <sal/core/spl.h>
#include <sal/core/dpc.h>
#include <sal/core/sync.h>

#ifdef PLISIM
#define DEFAULT_BOOT_FLAGS (BOOT_F_PLISIM | BOOT_F_NO_PROBE)
#else
#define DEFAULT_BOOT_FLAGS 0
#endif

/*
 * Function:
 *	sal_core_init
 * Purpose:
 *	Initialize the Kernel SAL
 * Returns:
 *	0 on success, -1 on failure
 */

int
sal_core_init(void)
{    
    sal_udelay(0);	/* Cause sal_udelay() to self-calibrate */

    sal_thread_main_set(sal_thread_self());

    if (sal_spl_init()) {
        return -1;
    }

    if (sal_dpc_init()) {
        return -1;
    }

    if (sal_global_lock_init()) {
        return -1;
    }

    return 0;
}

/*
 * Function:
 *	sal_boot_flags_get
 * Purpose:
 *	Return boot flags from startup
 *	Flags are set for PLISIM and NO_PROBE by default, and can be
 *	overridden by setting the environment variable SOC_BOOT_FLAGS.
 * Parameters:
 *	None
 * Returns:
 *	32-bit flags value, 0 if not supported or no flags set.
 */

static int	_sal_boot_flags_init = FALSE;
static uint32	_sal_boot_flags = 0;

uint32
sal_boot_flags_get(void)
{
    if (!_sal_boot_flags_init) {
	char *s = getenv("SOC_BOOT_FLAGS");
	if (s == NULL) {
	    _sal_boot_flags = DEFAULT_BOOT_FLAGS;
	} else {
	    _sal_boot_flags = sal_ctoi(s, NULL);
	}
	_sal_boot_flags_init = TRUE;
    }

    return _sal_boot_flags;
}

/*
 * Function:
 *	sal_boot_flags_set
 * Purpose:
 *	Change boot flags
 * Parameters:
 *	flags - New boot flags
 */

void
sal_boot_flags_set(uint32 flags)
{
    _sal_boot_flags_init = TRUE;
    _sal_boot_flags = flags;
}

/*
 * Function:
 *	sal_boot_script
 * Purpose:
 *	Return name of boot script from startup
 * Parameters:
 *	None
 * Returns:
 *	Name of boot script, NULL if none
 */
char *
sal_boot_script(void)
{
    return getenv("SOC_BOOT_SCRIPT");
}


/*
 * Function:
 *      sal_os_name
 * Purpose:
 *      Provide a description of the underlying operating system
 * Parameters:
 *      None
 * Returns:
 *      String describing the OS
 */
const char* 
sal_os_name(void)
{
    return "Unix (Posix)"; 
}
