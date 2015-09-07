/*
 * $Id: failover.c 1.1 Broadcom SDK $
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
 * Katana2 failover API
 */

#include <soc/defs.h>
#include <sal/core/libc.h>

#if defined(BCM_KATANA2_SUPPORT) &&  defined(INCLUDE_L3)

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/debug.h>
#include <bcm/types.h>
#include <bcm/error.h>
#include <bcm/failover.h>
#include <bcm_int/esw/katana2.h>
#include <bcm_int/esw/failover.h>
#include <bcm_int/esw/triumph2.h>


STATIC int
_bcm_kt2_failover_nhi_get(int unit, bcm_gport_t port, int *nh_index)
{
    int vp=0xFFFF;
    ing_dvp_table_entry_t dvp;

    if (!BCM_GPORT_IS_MPLS_PORT(port) &&
         !BCM_GPORT_IS_MIM_PORT(port) ) {
        return BCM_E_PARAM;
    }
    
    if ( BCM_GPORT_IS_MPLS_PORT(port)) {
         vp = BCM_GPORT_MPLS_PORT_ID_GET(port);
    } else if ( BCM_GPORT_IS_MIM_PORT(port)) {
         vp = BCM_GPORT_MIM_PORT_ID_GET(port);
    }

    if (vp >= soc_mem_index_count(unit, SOURCE_VPm)) {
        return BCM_E_PARAM;
    }
    BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));

    /* Next-hop index is used for multicast replication */
    *nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
    return BCM_E_NONE;
}
/*
 * Function:
 *        bcm_kt2_failover_status_set
 * Purpose:
 *        Set the parameters for a failover object
 * Parameters:
 *        IN :  unit
 *           IN :  failover_id
 *           IN :  value
 * Returns:
 *        BCM_E_XXX
 */

int
bcm_kt2_failover_status_set(int unit,
                                     bcm_failover_element_t *failover,
                                     int value)
{
    int rv = BCM_E_UNAVAIL;
    int nh_index;
    initial_prot_group_table_entry_t  prot_group_entry;
    rx_prot_group_table_entry_t  rx_prot_group_entry;
    initial_prot_nhi_table_1_entry_t   prot_nhi_1_entry;

    if ((value < 0) || (value > 1)) {
       return BCM_E_PARAM;
    }

    if (failover->failover_id != BCM_FAILOVER_INVALID) {
         /* Group protection for Port and Tunnel: Egress and Ingress */
         BCM_IF_ERROR_RETURN( 
              bcm_tr2_failover_id_validate ( unit, failover->failover_id ));

         BCM_IF_ERROR_RETURN (soc_mem_read(unit, INITIAL_PROT_GROUP_TABLEm, 
                   MEM_BLOCK_ANY, failover->failover_id, &prot_group_entry));

         soc_mem_field32_set(unit, INITIAL_PROT_GROUP_TABLEm, 
                   &prot_group_entry, REPLACE_ENABLEf, value);

         BCM_IF_ERROR_RETURN(
              soc_mem_write(unit, INITIAL_PROT_GROUP_TABLEm,
                   MEM_BLOCK_ALL, failover->failover_id, &prot_group_entry));

         BCM_IF_ERROR_RETURN (soc_mem_read(unit, RX_PROT_GROUP_TABLEm, 
               MEM_BLOCK_ANY, failover->failover_id, &rx_prot_group_entry));

         soc_mem_field32_set(unit, RX_PROT_GROUP_TABLEm, 
               &rx_prot_group_entry, DROP_DATA_ENABLEf, value);

         rv =   soc_mem_write(unit, RX_PROT_GROUP_TABLEm,
               MEM_BLOCK_ALL, failover->failover_id,  &rx_prot_group_entry);

    }else if (failover->intf != BCM_IF_INVALID) {
        /* Only Egress is applicable */
        if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, failover->intf)) {
            nh_index = failover->intf - BCM_XGS3_EGRESS_IDX_MIN;
        } else {
            nh_index = failover->intf  - BCM_XGS3_DVP_EGRESS_IDX_MIN;
        }

         BCM_IF_ERROR_RETURN (soc_mem_read(unit, INITIAL_PROT_NHI_TABLE_1m, 
                              MEM_BLOCK_ANY, nh_index, &prot_nhi_1_entry));

         soc_mem_field32_set(unit, INITIAL_PROT_NHI_TABLE_1m, 
                             &prot_nhi_1_entry, REPLACE_ENABLEf, value);

         rv = soc_mem_write(unit, INITIAL_PROT_NHI_TABLE_1m,
                       MEM_BLOCK_ALL, nh_index, &prot_nhi_1_entry);

    } else if (failover->port != BCM_GPORT_INVALID) {
         /* Individual protection for Pseudo-wire: Egress and Ingress */
         BCM_IF_ERROR_RETURN (
               _bcm_kt2_failover_nhi_get(unit, failover->port , &nh_index));

         BCM_IF_ERROR_RETURN (soc_mem_read(unit, INITIAL_PROT_NHI_TABLE_1m, 
                              MEM_BLOCK_ANY, nh_index, &prot_nhi_1_entry));

         soc_mem_field32_set(unit, INITIAL_PROT_NHI_TABLE_1m, 
                             &prot_nhi_1_entry, REPLACE_ENABLEf, value);

         rv = soc_mem_write(unit, INITIAL_PROT_NHI_TABLE_1m,
                            MEM_BLOCK_ALL, nh_index, &prot_nhi_1_entry);

    }
    return rv;
}


/*
 * Function:
 *        bcm_kt2_failover_status_get
 * Purpose:
 *        Get the parameters for a failover object
 * Parameters:
 *        IN :  unit
 *           IN :  failover_id
 *           OUT :  value
 * Returns:
 *        BCM_E_XXX
 */

int
bcm_kt2_failover_status_get(int unit,
                                     bcm_failover_element_t *failover,
                                     int  *value)
{
    initial_prot_group_table_entry_t  prot_group_entry;
    rx_prot_group_table_entry_t  rx_prot_group_entry;
    initial_prot_nhi_table_1_entry_t   prot_nhi_1_entry;
    int nh_index;


    if (failover->failover_id != BCM_FAILOVER_INVALID) {

         BCM_IF_ERROR_RETURN(
            bcm_tr2_failover_id_validate ( unit, failover->failover_id ));

         BCM_IF_ERROR_RETURN (soc_mem_read(unit, INITIAL_PROT_GROUP_TABLEm, 
                   MEM_BLOCK_ANY, failover->failover_id, &prot_group_entry));

         *value =   soc_mem_field32_get(unit, INITIAL_PROT_GROUP_TABLEm,
                   &prot_group_entry, REPLACE_ENABLEf);

         BCM_IF_ERROR_RETURN (soc_mem_read(unit, RX_PROT_GROUP_TABLEm, 
               MEM_BLOCK_ANY, failover->failover_id, &rx_prot_group_entry));

         *value &=   soc_mem_field32_get(unit, RX_PROT_GROUP_TABLEm,
               &rx_prot_group_entry, DROP_DATA_ENABLEf);

    } else if (failover->intf != BCM_IF_INVALID) {

        if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, failover->intf)) {
            nh_index = failover->intf - BCM_XGS3_EGRESS_IDX_MIN;
        } else {
            nh_index = failover->intf  - BCM_XGS3_DVP_EGRESS_IDX_MIN;
        }

         BCM_IF_ERROR_RETURN (soc_mem_read(unit, INITIAL_PROT_NHI_TABLE_1m, 
                             MEM_BLOCK_ANY, nh_index, &prot_nhi_1_entry));

         *value =   soc_mem_field32_get(unit, INITIAL_PROT_NHI_TABLE_1m,
                   &prot_nhi_1_entry, REPLACE_ENABLEf);

    } else if (failover->port != BCM_GPORT_INVALID) {

         BCM_IF_ERROR_RETURN (
               _bcm_kt2_failover_nhi_get(unit, failover->port , &nh_index));

         BCM_IF_ERROR_RETURN (soc_mem_read(unit, INITIAL_PROT_NHI_TABLE_1m, 
                             MEM_BLOCK_ANY, nh_index, &prot_nhi_1_entry));

         *value =   soc_mem_field32_get(unit, INITIAL_PROT_NHI_TABLE_1m,
                   &prot_nhi_1_entry, REPLACE_ENABLEf);

    }
    return BCM_E_NONE;
}

/*
 * Function:
 *        bcm_kt2_failover_prot_nhi_cleanup
 * Purpose:
 *        Cleanup  the  entry  for  PROT_NHI
 * Parameters:
 *        IN :  unit
 *           IN :  Primary Next Hop Index
 * Returns:
 *        BCM_E_XXX
 */

int
bcm_kt2_failover_prot_nhi_cleanup  (int unit, int nh_index)
{
    initial_prot_nhi_table_entry_t    prot_nhi_entry;
    initial_prot_nhi_table_1_entry_t  prot_nhi_1_entry;
    int rv;

    rv = soc_mem_read(unit, INITIAL_PROT_NHI_TABLEm, 
                             MEM_BLOCK_ANY, nh_index, &prot_nhi_entry);

    if (rv < 0){
        return BCM_E_NOT_FOUND;
    }

    sal_memset(&prot_nhi_entry, 0, sizeof(initial_prot_nhi_table_entry_t));

    rv = soc_mem_write(unit, INITIAL_PROT_NHI_TABLEm,
                       MEM_BLOCK_ALL, nh_index, &prot_nhi_entry);

    if (rv < 0){
        return rv;
    }

     rv = soc_mem_read(unit, INITIAL_PROT_NHI_TABLE_1m, 
                             MEM_BLOCK_ANY, nh_index, &prot_nhi_1_entry);

    if (rv < 0){
        return BCM_E_NOT_FOUND;
    }

    sal_memset(&prot_nhi_1_entry, 0, sizeof(initial_prot_nhi_table_1_entry_t));

    rv = soc_mem_write(unit, INITIAL_PROT_NHI_TABLE_1m,
                       MEM_BLOCK_ALL, nh_index, &prot_nhi_1_entry);

    return rv;

}

#endif /* defined(BCM_KATANA2_SUPPORT) &&  defined(INCLUDE_L3) */
