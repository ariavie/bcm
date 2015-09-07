/*
 * $Id: 000d33f2649f29513278fb266a74e1b68749dd82 $
 *
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
/*
 * UDF initializers
 */

#include <sal/core/libc.h>
 
#include <bcm/udf.h>

/* 
 * Function:
 *      bcm_udf_info_t_init
 * Purpose:
 *      Initialize UDF Info struct
 * Parameters: 
 *      udf_info - Pointer to the struct to be init'ed
 */
void
bcm_udf_t_init(bcm_udf_t *udf_info)
{
    if (udf_info != NULL) {
        sal_memset(udf_info, 0, sizeof(*udf_info));
    }
    return;
}


/* 
 * Function:
 *      bcm_udf_alloc_hints_t_init
 * Purpose:
 *      Initialize UDF alloc hints struct
 * Parameters: 
 *      udf_hints - Pointer to the struct to be init'ed
 */
void
bcm_udf_alloc_hints_t_init(bcm_udf_alloc_hints_t *udf_hints)
{
    if (udf_hints != NULL) {
        sal_memset(udf_hints, 0, sizeof(*udf_hints));
    }
    return;
}


/*
 * Function:
 *      bcm_udf_pkt_format_info_t_init
 * Purpose:
 *      Initialize UDF packet format struct
 * Parameters:
 *      udf_format - Pointer to the struct to be init'ed
 */
void
bcm_udf_pkt_format_info_t_init(bcm_udf_pkt_format_info_t *pkt_format)
{
    if (pkt_format != NULL) {
        sal_memset(pkt_format, 0, sizeof(*pkt_format));

#if 0
        /* Will not be required with updated proposal - ANY -> 0 */
        udf_format->l2  = BCM_PKT_FORMAT_L2_ANY;
        udf_format->vlan_tag  = BCM_PKT_FORMAT_VLAN_TAG_ANY;
        udf_format->outer_ip  = BCM_PKT_FORMAT_IP_ANY;
        udf_format->inner_ip  = BCM_PKT_FORMAT_IP_ANY;
        udf_format->tunnel  = BCM_PKT_FORMAT_TUNNEL_ANY;
        udf_format->mpls  = BCM_PKT_FORMAT_MPLS_ANY;
        udf_format->fibre_chan_outer  = BCM_PKT_FORMAT_FIBRE_CHAN_ANY;
        udf_format->fibre_chan_inner  = BCM_PKT_FORMAT_FIBRE_CHAN_ANY;

        
#endif /* 0 */

    }

    return;
}

