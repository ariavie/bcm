
/*
 * $Id: l2.c,v 1.17 Broadcom SDK $
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
 * L2 - Broadcom StrataSwitch Layer-2 switch common API.
 */

#include <soc/drv.h> 
#include <bcm/l2.h>
#include <shared/bsl.h>
char *bcm_l2_flags_str[] = BCM_L2_FLAGS_STR;
int bcm_l2_flags_count = sizeof(bcm_l2_flags_str)/sizeof(bcm_l2_flags_str[0]);
 /*
 * Function:
 *	bcm_esw_l2_addr_dump
 * Purpose:
 *	Dump a hardware-independent L2 address for debugging
 * Parameters:
 *	l2e - L2 address to dump
 */

void
bcm_l2_addr_dump(bcm_l2_addr_t *l2e)
{
    LOG_CLI((BSL_META("MAC_ADDR=%02x:%02x:%02x:%02x:%02x:%02x, "),
l2e->mac[0],l2e->mac[1],l2e->mac[2],
             l2e->mac[3],l2e->mac[4],l2e->mac[5]));
    LOG_CLI((BSL_META("VLAN_TAG=0x%x, PORT=%d, TGID=%d ENCAP=0x%x,\n"),
l2e->vid, l2e->port, l2e->tgid, l2e->encap_id));
    LOG_CLI((BSL_META("    MODID=%d, L2MC_IDX=%d,"),
l2e->modid, l2e->l2mc_group));
    LOG_CLI((BSL_META(" COS_DST=%d, COS_SRC=%d, \n    FLAGS=0x%08x\n"),
l2e->cos_dst, l2e->cos_src, l2e->flags));

}


/*
 * Function:
 *	bcm_l2_addr_t_init
 * Description:
 *	Initialize a bcm_l2_addr_t to a specified MAC address and VLAN,
 *	while zeroing all other fields.
 * Parameters:
 *	l2addr - Pointer to bcm_l2_addr_t
 * Returns:
 *	Nothing.
 */

void
bcm_l2_addr_t_init(bcm_l2_addr_t *l2addr, const sal_mac_addr_t mac, bcm_vlan_t vid)
{
    if (NULL != l2addr) {
        sal_memset(l2addr, 0, sizeof (*l2addr));
        sal_memcpy(l2addr->mac, mac, sizeof (sal_mac_addr_t));
        l2addr->vid = vid;
        l2addr->encap_id = -1;
    }
    return;
}


void
bcm_l2_auth_addr_t_init(bcm_l2_auth_addr_t *addr)
{
    if (NULL != addr) {
        sal_memset(addr, 0, sizeof (*addr));
    }
    return;
}



/*
 * Function:
 *      bcm_l2_cache_addr_init
 * Purpose:
 *      Intitialize an L2 cache address structure
 * Parameters:
 *      addr - (OUT) l2 cache address to initialize
 * Returns:
 *      nothing
 * Notes:
 *      All masks in the structure are cleared.
 */
void
bcm_l2_cache_addr_t_init(bcm_l2_cache_addr_t *addr)
{
    if (NULL != addr) {
        sal_memset(addr, 0, sizeof (*addr));
    }
    return;
}

/*
 * Function:
 *     bcm_l2_learn_limit_t_init
 * Description:
 *     Initialize an L2 learn limit structure
 * Parameters:
 *     limit       pointer to learn limit control info
 * Return: none
 */
void
bcm_l2_learn_limit_t_init(bcm_l2_learn_limit_t *limit)
{
    if (NULL != limit) {
        sal_memset(limit, 0, sizeof(bcm_l2_learn_limit_t));
    }
    return;
}

/*
 * Function:
 *     bcm_l2_learn_msgs_config_t_init
 * Description:
 *     Initialize an L2 learn message structure
 * Parameters:
 *     learn_msg_config       pointer to learn message info
 * Return: none
 */
void
bcm_l2_learn_msgs_config_t_init(bcm_l2_learn_msgs_config_t *learn_msg_config)
{
    if (NULL != learn_msg_config) {
        sal_memset(learn_msg_config, 0, sizeof(bcm_l2_learn_msgs_config_t));
        learn_msg_config->vlan = BCM_VLAN_INVALID;
    }
    return;
}


/*
 * Function:
 *     bcm_l2_addr_distribute_t_init
 * Description:
 *     Initialize an L2 learn distribution structure
 * Parameters:
 *     distribution       pointer to learn distribution info
 * Return: none
 */
void
bcm_l2_addr_distribute_t_init(bcm_l2_addr_distribute_t *distribution)
{
    if (NULL != distribution) {
        sal_memset(distribution, 0, sizeof(bcm_l2_addr_distribute_t));
    }
    return;
}


/*
 * Function:
 *      bcm_l2_egress_addr_t_init
 * Purpose:
 *      Intitialize an L2 egress structure
 * Parameters:
 *      addr - (OUT) l2 egress to initialize
 * Returns:
 *      nothing
 */
void
bcm_l2_egress_t_init(bcm_l2_egress_t *addr)
{
    if (NULL != addr) {
        sal_memset(addr, 0, sizeof (bcm_l2_egress_t));
    }
    return;
}

/*
 * Function:
 *      bcm_l2_station_t_init
 * Purpose:
 *      Intitialize an L2 station structure
 * Parameters:
 *      addr - (OUT) Pointer to L2 station address info.
 * Returns:
 *      nothing
 */
void
bcm_l2_station_t_init(bcm_l2_station_t *addr)
{
    if (NULL != addr) {
        sal_memset(addr, 0, sizeof (bcm_l2_station_t));
    }
    return;
}

/*
 * Function:
 *      bcm_l2_ring_t_init
 * Purpose:
 *      Intitialize an L2 ring structure
 * Parameters:
 *      addr - (OUT) Pointer to L2 ring structure.
 * Returns:
 *      nothing
 */
void
bcm_l2_ring_t_init(bcm_l2_ring_t *addr)
{
    if (NULL != addr) {
        sal_memset(addr, 0, sizeof (bcm_l2_ring_t));
    }
    return;
}

/*
 * Function:
 *      bcm_l2_mac_port_t_init
 * Purpose:
 *      Intitialize an L2 MAC port structure
 * Parameters:
 *      addr - (OUT) Pointer to L2 MAC port structure.
 * Returns:
 *      nothing
 */
void
bcm_l2_mac_port_t_init(bcm_l2_mac_port_t *mac_port)
{
    if (NULL != mac_port) {
        sal_memset(mac_port, 0, sizeof (bcm_l2_mac_port_t));
    }
    return;
}

/*
 * Function:
 *      bcm_l2_gport_forward_info_t_init
 * Purpose:
 *      Intitialize an L2 gportd forward info struct.
 * Parameters:
 *      addr - (OUT) Pointer to L2 gportd forward info struct.
 * Returns:
 *      nothing
 */
void bcm_l2_gport_forward_info_t_init(
    bcm_l2_gport_forward_info_t *forward_info)
{
    if (NULL != forward_info) {
        sal_memset(forward_info, 0, sizeof (bcm_l2_gport_forward_info_t));
    }
    return;
}


