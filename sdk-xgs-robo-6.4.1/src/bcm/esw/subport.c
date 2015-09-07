/*
 * $Id: subport.c,v 1.44 Broadcom SDK $
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
 * File:    subport.c
 * Purpose: Manages SUBPORT functions
 */

#include <shared/bsl.h>

#include <sal/core/libc.h>

#include <soc/defs.h>
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <bcm/l2.h>
#include <bcm/port.h>
#include <bcm/error.h>
#include <bcm/subport.h>
#include <bcm/types.h>
#include <bcm_int/esw/subport.h>
#include <bcm_int/esw/mbcm.h>

#if defined(BCM_FIREBOLT_SUPPORT)
#include <bcm_int/esw/firebolt.h>
#endif /* BCM_FIREBOLT_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
#include <bcm_int/esw/triumph.h>
#endif /* BCM_TRX_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <bcm_int/esw/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/xgs3.h>
#if defined(BCM_KATANA2_SUPPORT)
#include <soc/katana2.h>
#include <bcm_int/esw/katana2.h>
#endif /* BCM_KATANA2_SUPPORT */

/*
 * Function:
 *      bcm_subport_init
 * Purpose:
 *      Initialize the SUBPORT software module
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_init(int unit)
{
#if defined(BCM_KATANA2_SUPPORT)
    int rv;
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        rv = bcm_kt2_subport_init(unit);
        if (SOC_FAILURE(rv)) {
            return rv;
        }
    }
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        return bcm_tr2_subport_init(unit);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport)) {
        return bcm_tr_subport_init(unit);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_subport_cleanup
 * Purpose:
 *      Cleanup the SUBPORT software module
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_cleanup(int unit)
{
#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        bcm_kt2_subport_cleanup(unit);
    }
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        return bcm_tr2_subport_cleanup(unit);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport)) {
        return bcm_tr_subport_cleanup(unit);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_subport_group_create
 * Purpose:
 *      Create a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      config - (IN) Subport group config information
 *      group  - (OUT) GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_group_create(int unit,
                             bcm_subport_group_config_t *config,
                             bcm_gport_t *group)
{
    if (config == NULL) {
        return (BCM_E_PARAM);
    }
    if (config->flags & BCM_SUBPORT_GROUP_WITH_ID) {
        if (group == NULL) {
            return (BCM_E_PORT);
        }
        if (!BCM_GPORT_IS_SUBPORT_GROUP(*group)) {
            return (BCM_E_PORT);
        }
    }
#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
            (config->flags & BCM_SUBPORT_GROUP_TYPE_LINKPHY)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
            (config->flags & BCM_SUBPORT_GROUP_TYPE_SUBPORT_TAG))) {
        return bcm_kt2_subport_group_create(unit, config, group);
    }
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        return bcm_tr2_subport_group_create(unit, config, group);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport)) {
        return bcm_tr_subport_group_create(unit, config, group);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */

    return BCM_E_UNAVAIL;
}

#define _TMP_UINT_BUF_COUNT  16

#if defined (BCM_TRX_SUPPORT) && defined(INCLUDE_L3)

STATIC int 
_bcm_esw_subport_port_traverse_cb(int unit, 
                                  bcm_gport_t port, 
                                  bcm_subport_config_t *config,  
                                  void *user_data)
{   
	uint32 *data = (uint32 *)user_data; 
    
    if (config->group == data[0]) {
	    if (data[1] >= (_TMP_UINT_BUF_COUNT - 2)) {
	    	return BCM_E_NONE;
	    }
	    data[1]++;
	    data[data[1]+1] = port;
	}
    
    return BCM_E_NONE;
}

#endif
/*
 * Function:
 *      bcm_subport_group_destroy
 * Purpose:
 *      Destroy a subport group
 * Parameters:
 *      unit  - (IN) Device Number
 *      group - (IN) GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_group_destroy(int unit, bcm_gport_t group)
{
#if (defined(BCM_KATANA2_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT) || defined BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    uint32 sub_ports[_TMP_UINT_BUF_COUNT];
    uint32 i;
#endif
    
    if (!BCM_GPORT_IS_SUBPORT_GROUP(group)) {
        return (BCM_E_PORT);
    }
    
#if defined(BCM_KATANA2_SUPPORT) && defined(INCLUDE_L3)
    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
            _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(group)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
            _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(group))) {

        while (TRUE) {
        	sub_ports[0] = (uint32)group;
        	sub_ports[1] = 0;
	        bcm_kt2_subport_port_traverse(unit, 
	            _bcm_esw_subport_port_traverse_cb, sub_ports); 

	        for (i = 0; i < sub_ports[1]; i++) {
	        	bcm_kt2_subport_port_delete(unit, 
	        	        (bcm_gport_t)sub_ports[i+2]);
	        }
	        	
	        if (sub_ports[1] != (_TMP_UINT_BUF_COUNT - 2)) {
	        	break;
	        } 	
        } 
        return bcm_kt2_subport_group_destroy(unit, group);
    }
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
    	while (TRUE) {
        	sub_ports[0] = (uint32)group;
        	sub_ports[1] = 0;
	        bcm_tr2_subport_port_traverse(unit, 
	            _bcm_esw_subport_port_traverse_cb, sub_ports); 

	        for (i = 0; i < sub_ports[1]; i++) {
	        	bcm_tr2_subport_port_delete(unit, 
	        	        (bcm_gport_t)sub_ports[i+2]);
	        }
	        	
	        if (sub_ports[1] != (_TMP_UINT_BUF_COUNT - 2)) {
	        	break;
	        } 	
        }   
        return bcm_tr2_subport_group_destroy(unit, group);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport)) {
    	while (TRUE) {
        	sub_ports[0] = (uint32)group;
        	sub_ports[1] = 0;
	        bcm_tr_subport_port_traverse(unit, 
	            _bcm_esw_subport_port_traverse_cb, sub_ports); 

	        for (i = 0; i < sub_ports[1]; i++) {
	        	bcm_tr_subport_port_delete(unit, 
	        	        (bcm_gport_t)sub_ports[i+2]);
	        }
	        	
	        if (sub_ports[1] != (_TMP_UINT_BUF_COUNT - 2)) {
	        	break;
	        } 	
        } 
        return bcm_tr_subport_group_destroy(unit, group);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_subport_group_get
 * Purpose:
 *      Get a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      group  - (IN) GPORT (generic port) identifier
 *      config - (OUT) Subport group config information
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_group_get(int unit, bcm_gport_t group,
                          bcm_subport_group_config_t *config)
{
    if (!BCM_GPORT_IS_SUBPORT_GROUP(group)) {
        return (BCM_E_PORT);
    }
#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
            _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(group)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
            _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(group))) {
        return bcm_kt2_subport_group_get(unit, group, config);
    }
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        return bcm_tr2_subport_group_get(unit, group, config);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport)) {
        return bcm_tr_subport_group_get(unit, group, config);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_subport_port_add
 * Purpose:
 *      Add a subport to a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      config - (IN) Subport config information
 *      port   - (OUT) GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_port_add(int unit, bcm_subport_config_t *config,
                         bcm_gport_t *port)
{
#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
            _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(config->group)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
            _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(config->group))) {
        return bcm_kt2_subport_port_add(unit, config, port);
    }
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        return bcm_tr2_subport_port_add(unit, config, port);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport)) {
        return bcm_tr_subport_port_add(unit, config, port);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_subport_port_delete
 * Purpose:
 *      Delete a subport 
 * Parameters:
 *      unit   - (IN) Device Number
 *      port   - (IN) GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_port_delete(int unit, bcm_gport_t port)
{
    if (!BCM_GPORT_IS_SUBPORT_PORT(port)) {
        return (BCM_E_PORT);
    }
#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
            _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, port)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
            _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(unit, port))) {
        return bcm_kt2_subport_port_delete(unit, port);
    }
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        return bcm_tr2_subport_port_delete(unit, port);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport)) {
        return bcm_tr_subport_port_delete(unit, port);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_subport_port_get
 * Purpose:
 *      Get a subport
 * Parameters:
 *      unit   - (IN) Device Number
 *      port   - (IN) GPORT (generic port) identifier
 *      config - (OUT) Subport config information
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_port_get(int unit, bcm_gport_t port,
                         bcm_subport_config_t *config)
{
#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
            _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, port)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
            _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(unit, port))) {
        return bcm_kt2_subport_port_get(unit, port, config);
    }
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        return bcm_tr2_subport_port_get(unit, port, config);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport)) {
        return bcm_tr_subport_port_get(unit, port, config);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_subport_port_traverse
 * Purpose:
 *      Traverse all valid subports and call the
 *      supplied callback routine.
 * Parameters:
 *      unit      - Device Number
 *      cb        - User callback function, called once per subport.
 *      user_data - cookie
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_port_traverse(int unit,
                              bcm_subport_port_traverse_cb cb,
                              void *user_data)
{
#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        bcm_kt2_subport_port_traverse(unit, cb, user_data);
    }
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        return bcm_tr2_subport_port_traverse(unit, cb, user_data);
    }
#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_subport)) {
        return bcm_tr_subport_port_traverse(unit, cb, user_data);
    }
#endif /* BCM_TRX_SUPPORT && INCLUDE_L3 */
    return BCM_E_UNAVAIL;
}

#if defined(BCM_XGS_SUPPORT)
/*
 * Function:
 *      bcm_esw_subport_group_traverse
 * Purpose:
 *      Traverse all valid subports under given subport group and call the
 *      supplied callback routine.
 * Parameters:
 *      unit          (IN)Device Number
 *      subport_group (IN) Subport group GPORT (generic port identifier)
 *      cb             User callback function, called once per subport.
 *      user_data      cookie
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_group_traverse(int unit,
                              bcm_gport_t subport_group,
                              bcm_subport_port_traverse_cb cb,
                              void *user_data)
{
#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
            _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_GROUP(subport_group)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
            _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_GROUP(subport_group))) {
        return bcm_kt2_subport_group_traverse(unit,
            subport_group, cb, user_data);
    }
#endif /* BCM_KATANA2_SUPPORT */
    return BCM_E_UNAVAIL;
}

/**
 * Function:
 *      bcm_esw_subport_group_linkphy_config_get
 * Purpose:
 *      Get ALL or selective LinkPHY configuration attributes 
 *      on a given port GPORT.
 * Parameters:
 * Parameters:
 *      uint (IN) BCM device number
 *      port (IN) port GPORT
 *      linkphy_cfg (OUT) LinkPHY configuration 
 * Returns:
 *      BCM_E_xxx
 */

int 
bcm_esw_subport_group_linkphy_config_get(
    int unit, 
    bcm_gport_t port, 
    bcm_subport_group_linkphy_config_t *linkphy_config)
{
#if defined(BCM_KATANA2_SUPPORT)
    int rv, port_out;
    soc_info_t *si = &SOC_INFO(unit);

    if (soc_feature(unit, soc_feature_linkphy_coe)) {
        rv = _bcm_esw_port_gport_validate(unit, port, &port_out);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Invalid port gport 0x%x for LinkPHY config\n"),
                       port));
            return rv;
        }

        if (!BCM_PBMP_MEMBER(si->linkphy_pbm, port_out)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: port %d is not member of pbmp_linkphy\n"),
                       port_out));
            return BCM_E_PORT;
        }

        return bcm_kt2_subport_group_linkphy_config_get(unit, port_out,
                   linkphy_config);
    }
#endif /* BCM_KATANA2_SUPPORT */
    return BCM_E_UNAVAIL;
}

/**
 * Function:
 *      bcm_esw_subport_group_linkphy_config_set
 * Purpose:
 *      Set ALL or selective LinkPHY configuration attributes 
 *      on a given port GPORT.
 * Parameters:
 *      uint (IN) BCM device number
 *      port (IN) port GPORT
 *      linkphy_cfg (IN) LinkPHY configuration 
 * Returns:
 *      BCM_E_xxx
 */

int
bcm_esw_subport_group_linkphy_config_set(
    int unit,
    bcm_gport_t port,
    bcm_subport_group_linkphy_config_t *linkphy_config)
{
#if defined(BCM_KATANA2_SUPPORT)
    int rv, port_out;
    soc_info_t *si = &SOC_INFO(unit);

    if (soc_feature(unit, soc_feature_linkphy_coe)) {
        rv = _bcm_esw_port_gport_validate(unit, port, &port_out);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: Invalid port gport 0x%x for LinkPHY config\n"),
                       port));
            return rv;
        }

        if (!BCM_PBMP_MEMBER(si->linkphy_pbm, port_out)) {
            LOG_ERROR(BSL_LS_BCM_SUBPORT,
                      (BSL_META_U(unit,
                                  "ERROR: port %d is not member of pbmp_linkphy\n"),
                       port_out));
            return BCM_E_PORT;
        }

        return bcm_kt2_subport_group_linkphy_config_set(unit, port_out,
                   linkphy_config);
    }
#endif /* BCM_KATANA2_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_kt2_subport_port_stat_set
 * Purpose:
 *      Set the statistics value for a given subport port and statistics type
 * Parameters:
 *      unit      - (IN) Device Number
 *      port      - (IN) subport port
 *      stream_id - (IN) stream ID array index (if LinkPHY subport)
 *      stat_type - (IN) statistics type
 *      val       - (IN) value
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_port_stat_set(
    int unit, 
    bcm_gport_t port, 
    int stream_id, 
    bcm_subport_stat_t stat_type, 
    uint64 val)
{
#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
            _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, port)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
            _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(unit, port))) {
        return bcm_kt2_subport_port_stat_set(unit, port,
                    stream_id, stat_type, val);
    }
#endif /* BCM_KATANA2_SUPPORT */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_kt2_subport_port_stat_get
 * Purpose:
 *      Get the statistics value for a given subport port and statistics type
 * Parameters:
 *      unit      - (IN) Device Number
 *      port      - (IN) subport port
 *      stream_id - (IN) stream ID array index (if LinkPHY subport)
 *      stat_type - (IN) statistics type
 *      val       - (OUT) value
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_subport_port_stat_get(
    int unit, 
    bcm_gport_t port, 
    int stream_id, 
    bcm_subport_stat_t stat_type, 
    uint64 *val)
{
#if defined(BCM_KATANA2_SUPPORT)
    if ((soc_feature(unit, soc_feature_linkphy_coe) &&
            _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, port)) ||
        (soc_feature(unit, soc_feature_subtag_coe) &&
            _BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(unit, port))) {
        return bcm_kt2_subport_port_stat_get(unit, port,
                    stream_id, stat_type, val);
    }
#endif /* BCM_KATANA2_SUPPORT */
    return BCM_E_UNAVAIL;
}


#endif /* BCM_XGS_SUPPORT */
