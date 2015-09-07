/*
 * $Id: l3.c 1.2 Broadcom SDK $
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
 * File:        l3.c
 * Purpose:     Scorpion L3 functions implementations
 */


#include <soc/defs.h>

#include <assert.h>

#include <sal/core/libc.h>
#if defined(BCM_SCORPION_SUPPORT) && defined(INCLUDE_L3)

#include <shared/util.h>
#include <soc/mem.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/l3x.h>
#include <soc/lpm.h>
#include <soc/tnl_term.h>

#include <bcm/l3.h>
#include <bcm/tunnel.h>
#include <bcm/debug.h>
#include <bcm/error.h>
#include <bcm/stack.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/scorpion.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/xgs3.h>
#include <bcm_int/esw_dispatch.h>

/*                                                                                                
 * Function:                                                                                      
 *      _bcm_sc_defip_init                                                                  
 * Purpose:                                                                                       
 *      Initialize L3 DEFIP table for scorpion devices.                                                
 * Parameters:                                                                                    
 *      unit    - (IN)  SOC unit number.                                                          
 * Returns:                                                                                       
 *      BCM_E_XXX
 */                                                                                               
int                                                                                               
_bcm_sc_defip_init(int unit)
{
     /* Initialize  prefixes offsets, hash/avl  */
     /*   - lookup engine.                      */
     BCM_IF_ERROR_RETURN(soc_fb_lpm_init(unit));

     /* Initialize IPv6 greater than 64 bit prefixes offsets */
     /* hash lookup engine.                                  */
     BCM_IF_ERROR_RETURN(_bcm_trx_defip_128_init(unit));
    
     return (BCM_E_NONE);
}                                         

/*                                                                                                
 * Function:                                                                                      
 *      _bcm_sc_defip_deinit 
 * Purpose:                                                                                       
 *      De-initialize L3 DEFIP table for scorpion devices.                                                
 * Parameters:                                                                                    
 *      unit    - (IN)  SOC unit number.                                                          
 * Returns:                                                                                       
 *      BCM_E_XXX
 */                                                                                               
int                                                                                               
_bcm_sc_defip_deinit(int unit)
{
     /* De-initialize  prefixes offsets, hash/avl  */
     /*   - lookup engine.                      */
     BCM_IF_ERROR_RETURN(soc_fb_lpm_deinit(unit));

     /* De-initialize IPv6 greater than 64 bit prefixes offsets */
     /* hash lookup engine.                                  */
     BCM_IF_ERROR_RETURN(_bcm_trx_defip_128_deinit(unit));
    
     return (BCM_E_NONE);
}                                         
#else /* BCM_SCORPION_SUPPORT && INCLUDE_L3 */
int bcm_esw_scorpion_l3_not_empty;
#endif /* BCM_SCORPION_SUPPORT && INCLUDE_L3 */
