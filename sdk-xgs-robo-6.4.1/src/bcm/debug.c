/*
 * $Id: debug.c,v 1.13 Broadcom SDK $
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
 * File:        debug.c
 * Purpose:	BCM API Debug declarations
 * Requires:    
 */

#include <shared/bsl.h>

#include <soc/defs.h>
#include <soc/cm.h>
#include <bcm/error.h>

#if defined(BROADCOM_DEBUG)

/*
 * API Debug Message Helper.
 * Called from the BCM_API macro in <bcm/debug.h>
 * The BCM_API macro is called from the bcm api dispatcher.
 */
void
_bcm_debug_api(char *api,
	       int nargs, int ninargs,
	       int arg1, int arg2, int arg3, int rv)
{
    char	*rvstr;

    if (rv < 0) {
	rvstr = bcm_errmsg(rv);
    } else {
	rvstr = bcm_errmsg(BCM_E_NONE);
    }

    switch (ninargs) {
    case 0:
	LOG_VERBOSE(BSL_LS_BCM_API,
                    (BSL_META("API: %s(%s) -> %d %s\n"),
                     api,
                     nargs > 0 ? "..." : "",
                     rv, rvstr));
	break;
    case 1:
	LOG_VERBOSE(BSL_LS_BCM_API,
                    (BSL_META("API: %s(%d%s) -> %d %s\n"),
                     api, arg1,
                     nargs > 1 ? ",..." : "",
                     rv, rvstr));
	break;
    case 2:
	LOG_VERBOSE(BSL_LS_BCM_API,
                    (BSL_META("API: %s(%d,%d%s) -> %d %s\n"),
                     api, arg1, arg2,
                     nargs > 2 ? ",..." : "",
                     rv, rvstr));
	break;
    default:
	LOG_VERBOSE(BSL_LS_BCM_API,
                    (BSL_META("API: %s(%d,%d,%d%s) -> %d %s\n"),
                     api, arg1, arg2, arg3,
                     nargs > 3 ? ",..." : "",
                     rv, rvstr));
	break;
    }
}

#endif /* BROADCOM_DEBUG */
