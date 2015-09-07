/*
 * $Id: proxy.c,v 1.10 Broadcom SDK $
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
 * File:        proxy.c
 * Purpose:     BCMX Proxy API
 */

#ifdef  INCLUDE_L3

#include <sal/core/libc.h>

#include <bcm/types.h>

#include <bcmx/lport.h>
#include <bcmx/bcmx.h>
#include <bcmx/lplist.h>
#include "bcmx_int.h"

#include <bcmx/proxy.h>

#define BCMX_PROXY_INIT_CHECK    BCMX_READY_CHECK

#define BCMX_PROXY_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_ERROR_CHECK(_unit, _check, _rv)


/*
 * Function:
 *      bcmx_proxy_client_set
 * Purpose:
 *      Enables redirection for a certain traffic type using either 
 *      FFP or FP rule
 * Parameters:
 *      client_lport -  Logical port (Ingress)for which redirection is applied
 *      proto_type   -  Packet type to classify for redirection
 *      server_lport -  Logical Port where redirected packets are 
 *                      destined to
 *      enable       -  toggle to enable or disable redirection
 * Returns:
 *      BCM_E_XXX
 */
int
bcmx_proxy_client_set(bcmx_lport_t client_lport, 
                      bcm_proxy_proto_type_t proto_type,
                      bcmx_lport_t server_lport, int enable)
{
    bcm_port_t client_port, server_port;
    int unit;
    bcm_module_t server_modid;

    BCMX_PROXY_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(client_lport,
                                 &unit, &client_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_modid_port(server_lport,
                                  &server_modid, &server_port,
                                  BCMX_DEST_CONVERT_DEFAULT));

    return (bcm_proxy_client_set(unit, client_port, proto_type,
                                 server_modid, server_port, enable));
}
                   
/*
 * Function:
 *      bcmx_proxy_server_set
 * Purpose:
 *      Enables various kinds of lookups for packets coming from remote
 *      (proxy client) devices
 * Parameters:
 *      server_lport -  Logical Port to which packets are redirected to
 *      mode         -  Indicates lookup type
 *      enable       -  TRUE to enable lookups
 *                      FALSE to disable lookups
 * Returns:
 *      BCM_E_XXX
 */
int
bcmx_proxy_server_set(bcmx_lport_t server_lport, bcm_proxy_mode_t mode,
                      int enable)
{                       
    int unit;
    bcm_port_t server_port;

    BCMX_PROXY_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(server_lport, &unit, &server_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return (bcm_proxy_server_set(unit, server_port, mode, enable));
}

/*
 * Function:
 *      bcmx_proxy_server_get
 * Purpose:
 *      Get the lookup mode of XGS3 device.
 * Parameters:
 *      server_lport -  Logical port to which packets are redirected to
 *      status      -  (OUT) lookup mode
 * Returns:
 *      BCM_E_XXX
 */
int
bcmx_proxy_server_get(bcmx_lport_t server_lport, bcm_proxy_mode_t mode, 
                      int *enable)
{
    int unit;
    bcm_port_t server_port;

    BCMX_PROXY_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(enable);

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(server_lport, &unit, &server_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return (bcm_proxy_server_get(unit, server_port, mode, enable));
}

/*
 * Function:
 *      bcmx_proxy_init
 * Purpose:
 *      Initialize Proxy module system-wide
 * Parameters:
 *      None
 * Returns:
 *      BCM_E_XXX
 */
int
bcmx_proxy_init(void)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_PROXY_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_proxy_init(bcm_unit);
        BCM_IF_ERROR_RETURN(BCMX_PROXY_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *     bcmx_proxy_cleanup
 * Purpose:
 *     Disable Proxy system-wide
 * Parameters:
 *     None
 * Returns:
 *     BCM_E_XXX
 */
int
bcmx_proxy_cleanup(void)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_PROXY_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_proxy_cleanup(bcm_unit);
        BCM_IF_ERROR_RETURN(BCMX_PROXY_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}



#endif  /* INCLUDE_L3 */

int _bcmx_proxy_not_empty;
