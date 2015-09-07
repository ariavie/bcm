/*
 * $Id: txrx.c,v 1.4 Broadcom SDK $
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
 * socdiag tx (transmit) and rx (receive) commands
 */

#include <sal/core/libc.h>

#include <soc/types.h>
#include <soc/debug.h>
#include <soc/cm.h>

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/in.h>
#else
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#endif

#ifdef BCM_ROBO_SUPPORT
#include <soc/robo/mcm/driver.h>
#endif /* BCM_ROBO_SUPPORT */
#include <soc/dma.h>
#include <sal/types.h>
#include <sal/appl/sal.h>
#include <sal/appl/io.h>
#include <sal/core/thread.h>

#include <bcm/tx.h>
#include <bcm/pkt.h>
#include <bcm_int/robo/rx.h>
#include <bcm/port.h>
#include <bcm/error.h>

#include <appl/diag/system.h>

/*
 * Function:
 * Purpose:
 * Parameters:
 * Returns:
 */
cmd_result_t
cmd_ea_tx(int u, args_t *a)
{
	return CMD_OK;
}

/*
 * Function:    tx_count
 * Purpose: Print out status of any currently running TX command.
 * Parameters:  u - unit number.
 *      a - arguments, none expected.
 * Returns: CMD_OK
 */
cmd_result_t
cmd_ea_tx_count(int u, args_t *a)
{
	return CMD_OK;
}

/*
 * Function:    tx_start
 * Purpose: Start off a background TX thread.
 * Parameters:  u - unit number
 *      a - arguments.
 * Returns: CMD_XXX
 */
cmd_result_t
cmd_ea_tx_start(int u, args_t *a)
{
	return CMD_OK;
}


/*
 * Function:    tx_stop
 * Purpose: Stop a currently running TX command
 * Parameters:  u - unit number.
 *      a - arguments (none expected).
 * Returns: CMD_OK/CMD_USAGE/CMD_FAIL
 */
cmd_result_t
cmd_ea_tx_stop(int u, args_t *a)
{
	return CMD_OK;
}


cmd_result_t
cmd_ea_rx_cfg(int unit, args_t *args)
/*
 * Function:    rx
 * Purpose:     Perform simple RX test
 * Parameters:  unit - unit number
 *              args - arguments
 * Returns:     CMD_XX
 */
{
	return CMD_OK;
}


/****************************************************************
 *
 * RX commands
 *
 ****************************************************************/
cmd_result_t
cmd_ea_rx_init(int unit, args_t *args)
{
	return CMD_OK;
}
