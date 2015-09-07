/*
 * $Id: 8e0158d7fdee4d0268c875e1b2589d3e6c957422 $
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
 * File:    rate.c
 * Rate - Broadcom EA tk371x Rate Limiting API.
 *
 */
#include <sal/types.h>

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/debug.h>
#include <bcm/rate.h>
#include <bcm/port.h>
#include <bcm/types.h>
#include <bcm/error.h>

#include <bcm_int/tk371x_dispatch.h>
/*
 * Function:
 *  bcm_tk371x_rate_bcast_get
 * Description:
 *  Get rate limit for BCAST packets
 * Parameters:
 *  unit - EA tk371x device unit number
 *  pps - (OUT) Rate limit value in packets/second
 *  flags - (OUT) Bitmask with one or more of BCM_RATE_*
 *  port - Port number for which BCAST limit is requested
 * Returns:
 *  BCM_E_XXX
 * Notes:
 *  - The rate_limit value in API is packet per second, but Drv layer
 *      will use this value by Kb per second.
 */
int
bcm_tk371x_rate_bcast_get(
		int unit,
		int *pps,
		int *flags,
		int port){
	return BCM_E_UNAVAIL;
}

/*
 * Function:
 *  bcm_tk371x_rate_bcast_set
 * Description:
 *  Configure rate limit for BCAST packets
 * Parameters:
 *  unit - EA tk371x device unit number
 *  pps - Rate limit value in packets/second
 *  flags - Bitmask with one or more of BCM_RATE_*
 *      port - Port number for which BCAST limit needs to be set
 * Returns:
 *  BCM_E_NONE - Success.
 *  BCM_E_UNAVAIL - Not supported.
 * Notes:
 *  - EA chip on rate/storm control use the same rate_limit value.
 *    This limit value is system basis but port basis.
 *  - The rate_limit value in API is packet per second, but Drv layer
 *      will view this value as Kb per second.
 */
int
bcm_tk371x_rate_bcast_set(
		int unit,
		int pps,
		int flags,
		int port){
	return BCM_E_UNAVAIL;
}

