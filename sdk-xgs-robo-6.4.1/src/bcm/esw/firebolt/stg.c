/*
 * $Id: stg.c,v 1.17 Broadcom SDK $
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
 * STG support
 *
 * These functions set or get port related fields in the Spanning tree
 * table.
 */

#include <shared/bsl.h>

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/ptable.h>         /* PVP_* defines */

#include <bcm/types.h>
#include <bcm/error.h>
#include <bcm/stg.h>

#include <bcm_int/esw/stg.h>

#define STG_BITS_PER_PORT       2
#define STG_PORT_MASK           ((1 << STG_BITS_PER_PORT)-1)
#define STG_PORTS_PER_WORD      (32 / STG_BITS_PER_PORT)
#define STG_WORD(port)          ((port) / STG_PORTS_PER_WORD)
#define STG_BITS_SHIFT(port)    \
        (STG_BITS_PER_PORT * ((port) % STG_PORTS_PER_WORD))
#define STG_BITS_MASK(port)     (STG_PORT_MASK << (STG_BITS_SHIFT(port)))

/*
 * Function:
 *      _bcm_xgs3_stg_stp_init
 * Purpose:
 *      Initialize spanning tree group on a single device. 
 * Parameters:
 *      unit - (IN)SOC unit number.
 *      stg  - (IN)Spanning tree group id.
 *      mem  - (IN)Spanning tree group table memory.
 * Returns:  
 *      BCM_E_XXX
 */
STATIC int
_bcm_xgs3_stg_stp_init(int unit, bcm_stg_t stg, soc_mem_t mem)
{
    uint32 entry[SOC_MAX_MEM_WORDS];    /* Spanning tree port state map. */
    int stp_state;              /* STP port state.               */
    bcm_pbmp_t stacked;         /* Bitmap of stacked ports.      */
    bcm_port_t port;            /* Port iterator.                */

#ifdef BCM_SHADOW_SUPPORT
    /* EGR_VLAN_STGm not supported in shadow */
    if (SOC_IS_SHADOW(unit) && (mem == EGR_VLAN_STGm)) {
        return SOC_E_NONE;
    }
#endif

    /* Set all ports to PVP_STP_DISABLED */
    sal_memset(entry, 0, sizeof(entry));

    /* Get all stacking ports and set them into forwarding */
    BCM_PBMP_ASSIGN(stacked, PBMP_ST_ALL(unit));
    BCM_PBMP_OR(stacked, SOC_PBMP_STACK_CURRENT(unit));

    stp_state = PVP_STP_FORWARDING;

#ifdef BCM_RCPU_SUPPORT
    if (SOC_IS_RCPU_ONLY(unit)) {
        if (BCM_STG_DEFAULT == stg) {
            PBMP_ALL_ITER(unit, port) {
                entry[STG_WORD(port)] |= stp_state << STG_BITS_SHIFT(port);
            }
        }
    }
#endif /* BCM_RCPU_SUPPORT */

    /* Iterate over stacking ports & set stp state. */
    PBMP_ITER(stacked, port) {
        entry[STG_WORD(port)] |= stp_state << STG_BITS_SHIFT(port);
    }

    /* Write spanning tree group port states to hw. */
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, mem, MEM_BLOCK_ALL, stg, entry));

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_xgs3_stg_stp_set
 * Purpose:
 *      Set the spanning tree state for a port in specified STG.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      stg       - (IN)Spanning tree group id.
 *      port      - (IN)StrataSwitch port number.
 *      stp_state - (IN)STP state to place port in.
 *      mem       - (IN)Spanning tree group table memory.
 * Returns: 
 *      BCM_E_XXX
 */
STATIC int
_bcm_xgs3_stg_stp_set(int unit, bcm_stg_t stg, bcm_port_t port,
                      int stp_state, soc_mem_t mem)
{
    uint32 entry[SOC_MAX_MEM_WORDS];    /* STP group ports state map.   */
    int hw_stp_state;           /* STP port state in hw format. */
    int rv;                     /* Operation return status.     */

    /* Input parameters verification. */
    if (!SOC_PORT_VALID(unit, port)) {
        return BCM_E_PORT;
    }

#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_KATANA_SUPPORT)
    if ((SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit))
        && IS_LB_PORT(unit, port)) {
    } else
#endif
    if (!IS_PORT(unit, port)) {
        return BCM_E_PORT;
    }

    /* Translate API space port state to hw stp port state. */
    BCM_IF_ERROR_RETURN(_bcm_stg_stp_translate(unit, stp_state, &hw_stp_state));

    soc_mem_lock(unit, mem);
    /* Read ports states for the stp group. */
    rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, stg, entry);
    if (BCM_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_BCM_L2,
                  (BSL_META_U(unit,
                              "Error: (%d) reading port states for stg(%d)\n"), rv, stg));
        soc_mem_unlock(unit, mem);
        return rv;
    }

    /* Reset port current state bit. */
    entry[STG_WORD(port)] &= ~(STG_BITS_MASK(port));

    /* Set new port state. */
    entry[STG_WORD(port)] |= (hw_stp_state << STG_BITS_SHIFT(port));

    /* Write spanning tree group port states to hw. */
    rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, stg, entry);
    soc_mem_unlock(unit, mem);

    return rv;
}

/*
 * Function:
 *      _bcm_xgs3_stg_stp_get
 * Purpose:
 *      Retrieve the spanning tree state for a port in specified STG.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      stg       - (IN)Spanning tree group id.
 *      port      - (IN)StrataSwitch port number.
 *      stp_state - (IN/OUT)Port STP state.
 *      mem       - (IN)Spanning tree group table memory.
 * Returns: 
 *      BCM_E_XXX
 */
STATIC int
_bcm_xgs3_stg_stp_get(int unit, bcm_stg_t stg, bcm_port_t port,
                      int *stp_state, soc_mem_t mem)
{
    uint32 entry[SOC_MAX_MEM_WORDS];    /* STP group ports state map.   */
    int hw_stp_state;           /* STP port state in hw format. */
    int rv;                     /* Operation return status.     */

    /* Input parameters check. */
    if (!SOC_PORT_VALID(unit, port) || !IS_PORT(unit, port)) {
        return (BCM_E_PORT);
    }

    soc_mem_lock(unit, mem);
    rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, stg, entry);
    soc_mem_unlock(unit, mem);
    if (BCM_FAILURE(rv)) {
        return rv;
    }
    /* Get specific port state from the entry. */
    hw_stp_state = entry[STG_WORD(port)] >> STG_BITS_SHIFT(port);
    hw_stp_state &= STG_PORT_MASK;

    /* Translate hw stp port state to API format. */
    BCM_IF_ERROR_RETURN(_bcm_stg_pvp_translate(unit, hw_stp_state, stp_state));

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_xgs3_stg_stp_init
 * Purpose:
 *      Initialize spanning tree group on a single device. 
 * Parameters:
 *      unit - (IN)SOC unit number.
 *      stg  - (IN)Spanning tree group id.
 * Returns:  
 *      BCM_E_XXX
 */
int
bcm_xgs3_stg_stp_init(int unit, bcm_stg_t stg)
{
    BCM_IF_ERROR_RETURN(_bcm_xgs3_stg_stp_init(unit, stg, STG_TABm));
    if (SOC_IS_FBX(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_xgs3_stg_stp_init(unit, stg, EGR_VLAN_STGm));
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_xgs3_stg_stp_set
 * Purpose:
 *      Set the spanning tree state for a port in specified STG.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      stg       - (IN)Spanning tree group id.
 *      port      - (IN)StrataSwitch port number.
 *      stp_state - (IN)STP state to place port in.
 * Returns: 
 *      BCM_E_XXX
 */
int
bcm_xgs3_stg_stp_set(int unit, bcm_stg_t stg, bcm_port_t port, int stp_state)
{
    BCM_IF_ERROR_RETURN
        (_bcm_xgs3_stg_stp_set(unit, stg, port, stp_state, STG_TABm));
    if (SOC_IS_FBX(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_xgs3_stg_stp_set(unit, stg, port, stp_state, EGR_VLAN_STGm));
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_xgs3_stg_stp_get
 * Purpose:
 *      Retrieve the spanning tree state for a port in specified STG.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      stg       - (IN)Spanning tree group id.
 *      port      - (IN)StrataSwitch port number.
 *      stp_state - (IN/OUT)Port STP state int the group.
 * Returns: 
 *      BCM_E_XXX
 */
int
bcm_xgs3_stg_stp_get(int unit, bcm_stg_t stg, bcm_port_t port, int *stp_state)
{
    /* Input parameters check. */
    if (!stp_state) {
        return (BCM_E_PARAM);
    }

    /* Get port state in specified group. */
    return _bcm_xgs3_stg_stp_get(unit, stg, port, stp_state, STG_TABm);
}
