/*
 * $Id: topo.c 1.3 Broadcom SDK $
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
 * File:        topo.c
 * Purpose:     Topology mapping, when so programmed.
 * Requires:    
 */


#include <soc/drv.h>

#include <bcm/topo.h>
#include <bcm/error.h>
#include <bcm/stack.h>

static bcm_topo_map_f _topo_map;


/*
 * Function:
 *      bcm_topo_map_set/get
 * Purpose:
 *      Set/get the map used by topo_port_resolve
 * Parameters:
 *      _map        - (OUT for get) The function pointer to use
 * Returns:
 *      BCM_E_NONE
 * Notes:
 *      These functions are not dispatchable.  The board type
 *      and topology information determine the map to use.
 */

int
bcm_topo_map_set(bcm_topo_map_f _map)
{
    _topo_map = _map;

    return BCM_E_NONE;
}

int
bcm_topo_map_get(bcm_topo_map_f *_map)
{
    *_map = _topo_map;

    return BCM_E_NONE;
}


/*
 * Default topology mapping function.
 * This routine is used when no board specific mapping function
 * has been programmed.  It knows about common single board configurations.
 */

STATIC int
_bcm_topo_default_map(int unit, int dest_modid, bcm_port_t *exit_port)
{
    int		rv, local_mod, local_mod_count;
    bcm_port_t	port;
    bcm_pbmp_t	pbmp;

    /* is the destination local? */
    rv = bcm_stk_my_modid_get(unit, &local_mod);
    if (BCM_E_NONE != rv) {
        return rv;
    }
    rv = bcm_stk_modid_count(unit, &local_mod_count);
    if (BCM_E_NONE != rv) {
        return rv;
    }

    if (dest_modid >= local_mod && dest_modid < local_mod + local_mod_count) {
	*exit_port = -1;
	return BCM_E_NONE;
    }

    /* handles xgs switches */
    rv = bcm_stk_modport_get(unit, dest_modid, exit_port);
    if (rv != BCM_E_UNAVAIL) {
	return rv;
    }

    /* handles xgs fabrics */
    rv = bcm_stk_ucbitmap_get(unit, -1, dest_modid, &pbmp);
    if (rv != BCM_E_UNAVAIL) {
	if (rv >= 0) {
	    BCM_PBMP_ITER(pbmp, port) {
		*exit_port = port;
		break;
	    }
	}
	return rv;
    }

    /* handles strata back to back boards */
    
    SOC_PBMP_STACK_ACTIVE_GET(unit, pbmp);
    if (BCM_PBMP_NOT_NULL(pbmp)) {
	BCM_PBMP_ITER(pbmp, port) {
	    *exit_port = port;	/* just grab the first stack port */
	    break;
	}
	return BCM_E_NONE;
    }

    /* out of ideas */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_topo_port_get
 * Purpose:
 *      Helper function to bcm_topo_port_get API
 * Parameters:
 *      unit        - The source unit
 *      dest_modid  - Destination to reach
 *      exit_port   - (OUT) Port to exit device on
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The code for the local board is programmed by 
 */

int
_bcm_topo_port_get(int unit, int dest_modid, bcm_port_t *exit_port)
{
    if (_topo_map == NULL) {
	return _bcm_topo_default_map(unit, dest_modid, exit_port);
    }

    return _topo_map(unit, dest_modid, exit_port);
}

