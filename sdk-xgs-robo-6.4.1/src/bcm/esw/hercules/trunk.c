/*
 * $Id: trunk.c,v 1.21 Broadcom SDK $
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
 */

#include <soc/drv.h>
#include <soc/mem.h>

#include <bcm/error.h>
#include <bcm/trunk.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/trunk.h>
#include <bcm_int/esw/hercules.h>

#include <bcm_int/esw_dispatch.h>

/*
 * Purpose: Multiplexed trunking function for Hercules
 * Note: The parameters op and member are not used.
 */
int
bcm_hercules_trunk_modify(int unit, bcm_trunk_t tid,
		       bcm_trunk_info_t *trunk_info,
                       int member_count,
                       bcm_trunk_member_t *member_array,
		       trunk_private_t *t_info,
                       int op,
                       bcm_trunk_member_t *member)
{
    int		i;
    bcm_port_t	port;
    uint32	algo, bmap, val, oval;
    uint32 ing_cntlr;

    ing_cntlr = ING_CTRLr;

    bmap = 0;
    for (i = 0; i < member_count; i++) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_port_local_get(unit, member_array[i].gport, &port));
	if (!IS_PORT(unit, port)) {
	    return BCM_E_BADID;
	}
	bmap |= (1<<port);
    }
    if (trunk_info->psc <= 0) {
	trunk_info->psc = BCM_TRUNK_PSC_DEFAULT;

        trunk_info->psc |= (BCM_TRUNK_PSC_MACSA | BCM_TRUNK_PSC_MACDA |
                          BCM_TRUNK_PSC_IPMACSA | BCM_TRUNK_PSC_IPMACDA);
    }
    switch (trunk_info->psc & 0xf) {
    case BCM_TRUNK_PSC_SRCDSTMAC:
	algo = 0;
	break;
    case BCM_TRUNK_PSC_REDUNDANT:
	algo = 1;
	break;
    default:
	return BCM_E_PARAM;
    }

    val = 0;
    soc_reg_field_set(unit, ING_HGTRUNKr, &val, BMAPf, bmap);
    soc_reg_field_set(unit, ING_HGTRUNKr, &val, ALGORITHMf, algo);

    if (!algo) {
        if (trunk_info->psc & BCM_TRUNK_PSC_IPMACSA) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_IPMACSAf, 1);
        }
        if (trunk_info->psc & BCM_TRUNK_PSC_IPMACDA) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_IPMACDAf, 1);
        }
        if (trunk_info->psc & BCM_TRUNK_PSC_IPTYPE) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_IPTYPEf, 1);
        }
        if (trunk_info->psc & BCM_TRUNK_PSC_IPVID) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_IPVIDf, 1);
        }
        if (trunk_info->psc & BCM_TRUNK_PSC_IPDA) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_IPDAf, 1);
        }
        if (trunk_info->psc & BCM_TRUNK_PSC_IPSA) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_IPSAf, 1);
        }
        if (trunk_info->psc & BCM_TRUNK_PSC_L4SS) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_L4SSf, 1);
        }
        if (trunk_info->psc & BCM_TRUNK_PSC_L4DS) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_L4DSf, 1);
        }
        if (trunk_info->psc & BCM_TRUNK_PSC_MACSA) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_MACSAf, 1);
        }
        if (trunk_info->psc & BCM_TRUNK_PSC_MACDA) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_MACDAf, 1);
        }
        if (trunk_info->psc & BCM_TRUNK_PSC_TYPE) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_TYPEf, 1);
        }
        if (trunk_info->psc & BCM_TRUNK_PSC_VID) {
            soc_reg_field_set(unit, ING_HGTRUNKr, &val, EN_VIDf, 1);
        }
    }
	
    PBMP_PORT_ITER(unit, port) {
	SOC_IF_ERROR_RETURN(WRITE_ING_HGTRUNKr(unit, port, tid, val));
    }

    if (!algo) {
        int eqw[BCM_TRUNK_MAX_PORTCNT];

        sal_memset(eqw, 0, sizeof(eqw));
        BCM_IF_ERROR_RETURN(bcm_esw_trunk_pool_set(unit, -1, tid, 0, eqw));
    }

    if (bmap != 0) {
        PBMP_PORT_ITER(unit, port) {
            SOC_IF_ERROR_RETURN(soc_reg32_get(unit,
                                ing_cntlr, port, 0, &val));
            oval = val;
            soc_reg_field_set(unit, ing_cntlr, &val, TRUNKENf, 1);
            if (val != oval) {
                SOC_IF_ERROR_RETURN(soc_reg32_set(unit,
                                ing_cntlr, port, 0, val));
            }
        }
    }
    return BCM_E_NONE;
}

/*
 * Purpose: Multiplexed trunking function for Hercules
 */
int
bcm_hercules_trunk_destroy(int unit, bcm_trunk_t tid, trunk_private_t *t_info)
{
    bcm_port_t	port;

    PBMP_PORT_ITER(unit, port) {
	SOC_IF_ERROR_RETURN(WRITE_ING_HGTRUNKr(unit, port, tid, 0));
    }
    return BCM_E_NONE;
}

/*
 * Purpose: Multiplexed trunking function for Hercules
 */
int
bcm_hercules_trunk_get(int unit, bcm_trunk_t tid,
		       bcm_trunk_info_t *t_data,
                       int member_max, 
                       bcm_trunk_member_t *member_array,
                       int *member_count,
		       trunk_private_t *t_info)
{
    bcm_port_t	port;
    uint32	val;
    bcm_pbmp_t	pbmp;
    bcm_module_t mod = 0;
    int         num_ports;

    PBMP_PORT_ITER(unit, port) {
	SOC_IF_ERROR_RETURN(READ_ING_HGTRUNKr(unit, port, tid, &val));
	break;
    }

    SOC_PBMP_CLEAR(pbmp);
    SOC_PBMP_WORD_SET(pbmp, 0,
		      soc_reg_field_get(unit, ING_HGTRUNKr, val, BMAPf));
    num_ports = 0;
    PBMP_ITER(pbmp, port) {
        if (num_ports < member_max) {
            BCM_IF_ERROR_RETURN(_bcm_esw_trunk_gport_construct(unit, TRUE,
                        1, &port, &mod, &member_array[num_ports].gport));
        }
	num_ports += 1;
    }
    *member_count = num_ports;
	
    switch (soc_reg_field_get(unit, ING_HGTRUNKr, val, ALGORITHMf)) {
    case 0:
	t_data->psc = BCM_TRUNK_PSC_SRCDSTMAC;
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_IPMACSAf)) {
            t_data->psc |= BCM_TRUNK_PSC_IPMACSA;
        }
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_IPMACDAf)) {
            t_data->psc |= BCM_TRUNK_PSC_IPMACDA;
        }
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_IPTYPEf)) {
            t_data->psc |= BCM_TRUNK_PSC_IPTYPE;
        }
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_IPVIDf)) {
            t_data->psc |= BCM_TRUNK_PSC_IPVID;
        }
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_IPSAf)) {
            t_data->psc |= BCM_TRUNK_PSC_IPSA;
        }
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_IPDAf)) {
            t_data->psc |= BCM_TRUNK_PSC_IPDA;
        }
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_L4SSf)) {
            t_data->psc |= BCM_TRUNK_PSC_L4SS;
        }
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_L4DSf)) {
            t_data->psc |= BCM_TRUNK_PSC_L4DS;
        }
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_MACDAf)) {
            t_data->psc |= BCM_TRUNK_PSC_MACDA;
        }
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_MACSAf)) {
            t_data->psc |= BCM_TRUNK_PSC_MACSA;
        }
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_TYPEf)) {
            t_data->psc |= BCM_TRUNK_PSC_TYPE;
        }
        if (soc_reg_field_get(unit, ING_HGTRUNKr, val, EN_VIDf)) {
            t_data->psc |= BCM_TRUNK_PSC_VID;
        }
	break;
    case 1:
	t_data->psc = BCM_TRUNK_PSC_REDUNDANT;
	break;
    default:
	t_data->psc = -1;
	break;
    }
    t_data->dlf_index = -1;
    t_data->mc_index = -1;
    t_data->ipmc_index = -1;
    return BCM_E_NONE;
}

/*
 * Purpose: Multiplexed trunking function for Hercules
 */
int
bcm_hercules_trunk_mcast_join(int unit, bcm_trunk_t tid, bcm_vlan_t vid,
                              sal_mac_addr_t mac, trunk_private_t *t_info)
{
    return BCM_E_UNAVAIL;
}
