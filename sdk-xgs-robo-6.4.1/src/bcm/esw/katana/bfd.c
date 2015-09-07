/*
 * $Id: bfd.c,v 1.137 Broadcom SDK $
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
 *
 * File:       bfd.c
 * Purpose:    Bidirectional Forwarding Detection APIs.
 *
 */

#include <soc/defs.h>

#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_BFD)

#include <soc/hash.h>
#include <bcm_int/esw/bfd.h>
#include <bcm_int/esw/xgs5.h>
#include <bcm_int/esw/katana.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm/error.h>
#include <bcm/bfd.h>


/*
 * Function Vectors
 */
static bcm_esw_bfd_drv_t bcm_kt_bfd_drv = {
    /* init                     */ bcmi_kt_bfd_init,
    /* detach                   */ bcmi_xgs5_bfd_detach,
    /* endpoint_create          */ bcmi_xgs5_bfd_endpoint_create,
    /* endpoint_get             */ bcmi_xgs5_bfd_endpoint_get,
    /* endpoint_destroy         */ bcmi_xgs5_bfd_endpoint_destroy,
    /* endpoint_destroy_all     */ bcmi_xgs5_bfd_endpoint_destroy_all,
    /* endpoint_poll            */ bcmi_xgs5_bfd_endpoint_poll,
    /* event_register           */ bcmi_xgs5_bfd_event_register,
    /* event_unregister         */ bcmi_xgs5_bfd_event_unregister,
    /* endpoint_stat_get        */ bcmi_xgs5_bfd_endpoint_stat_get,
    /* auth_sha1_set            */ bcmi_xgs5_bfd_auth_sha1_set,
    /* auth_sha1_get            */ bcmi_xgs5_bfd_auth_sha1_get,
    /* auth_simple_password_set */ bcmi_xgs5_bfd_auth_simple_password_set,
    /* auth_simple_password_get */ bcmi_xgs5_bfd_auth_simple_password_get,
#ifdef BCM_WARM_BOOT_SUPPORT
    /* bfd_sync                 */ bcmi_xgs5_bfd_sync,
#endif /* BCM_WARM_BOOT_SUPPORT */
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
    /* bfd_sw_dump              */ bcmi_xgs5_bfd_sw_dump,
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */
};


/*
 * Device Specific HW Tables
 */

/* Device programming routines */
static bcm_xgs5_bfd_hw_calls_t bcm_kt_bfd_hw_calls = {
    /* l3_tnl_term_entry_init   */ _bcm_tr2_l3_tnl_term_entry_init,
    /* mpls_lock                */ bcm_tr_mpls_lock,
    /* mpls_unlock              */ bcm_tr_mpls_unlock,
};

/* L2 Table */
static bcm_xgs5_bfd_l2_table_t bcm_kt_bfd_l2_table = {
    /* mem                */  L2Xm,
    /* key_type           */  KEY_TYPEf,
    /* bfd_key_type       */  0x4,
    /* valid              */  VALIDf,
    /* static             */  BFD__STATIC_BITf,
    /* session_id_type    */  BFD__SESSION_IDENTIFIER_TYPEf,
    /* your_discr         */  BFD__YOUR_DISCRIMINATORf,
    /* label              */  BFD__LABELf,
    /* session_index      */  BFD__BFD_RX_SESSION_INDEXf,
    /* cpu_queue_class    */  BFD__BFD_CPU_QUEUE_CLASSf,
    /* remote             */  BFD__BFD_REMOTEf,
    /* dst_module         */  BFD__BFD_DST_MODf,
    /* dst_port           */  BFD__BFD_DST_PORTf,
    /* int_pri            */  BFD__BFD_INT_PRIf,
};

/* L3 IPv4 Unicast Table */
static bcm_xgs5_bfd_l3_ipv4_table_t bcm_kt_bfd_l3_ipv4_table = {
    /* mem                */ L3_ENTRY_IPV4_UNICASTm, 
    /* vrf_id             */ VRF_IDf, 
    /* ip_addr            */ IP_ADDRf,
    /* key_type           */ KEY_TYPEf, 
    /* local_address      */ IPV4UC__LOCAL_ADDRESSf,
    /* bfd_enable         */ IPV4UC__BFD_ENABLEf,
};

/* L3 IPv6 Unicast Table */
static bcm_xgs5_bfd_l3_ipv6_table_t bcm_kt_bfd_l3_ipv6_table = {
    /* mem                */ L3_ENTRY_IPV6_UNICASTm, 
    /* ip_addr_lwr_64     */ IP_ADDR_LWR_64f,
    /* ip_addr_upr_64     */ IP_ADDR_UPR_64f,
    /* key_type_0         */ KEY_TYPE_0f,
    /* key_type_1         */ KEY_TYPE_1f,
    /* vrf_id             */ VRF_IDf,
    /* local_address      */ IPV6UC__LOCAL_ADDRESSf,
    /* bfd_enable         */ IPV6UC__BFD_ENABLEf,
};

/* L3 Tunnel Table */
static bcm_xgs5_bfd_l3_tunnel_table_t bcm_kt_bfd_l3_tunnel_table = {
    /* mem                */ L3_TUNNELm,
    /* bfd_enable         */ BFD_ENABLEf,
};

/* MPLS Table */
static bcm_xgs5_bfd_mpls_table_t bcm_kt_bfd_mpls_table = {
    /* mem                */ MPLS_ENTRYm,
    /* valid              */ VALIDf,
    /* key_type           */ KEY_TYPEf,
    /* key_type_value     */ 0x0,
    /* mpls_label         */ MPLS__MPLS_LABELf,
    /* session_id_type    */ MPLS__SESSION_IDENTIFIER_TYPEf,
    /* bfd_enable         */ MPLS__BFD_ENABLEf,
    /* cw_check_ctrl      */ MPLS__CW_CHECK_CTRLf,
    /* pw_cc_type         */ MPLS__PW_CC_TYPEf,
    /* mpls_action_if_bos */ MPLS__MPLS_ACTION_IF_BOSf,
    /* l3_iif             */ MPLS__L3_IIFf,
    /* decap_use_ttl      */ MPLS__DECAP_USE_TTLf,
};

/* HW Definitions */
static bcm_xgs5_bfd_hw_defs_t    bcm_kt_bfd_hw_defs;


/*
 * Function:
 *      bcmi_kt_bfd_init
 * Purpose:
 *      Initialize the BFD subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_MEMORY Unable to allocate memory for internal control structures
 *      BCM_E_INTERNAL Failed to initialize
 * Notes:
 *      BFD initialization will return BCM_E_NONE for the following
 *      error conditions (i.e. feature not supported):
 *      - uKernel is not loaded
 *      - BFD application is not found in any of the uC uKernel
 */
int
bcmi_kt_bfd_init(int unit)
{

    /* HW Definition Tables */
    sal_memset(&bcm_kt_bfd_hw_defs, 0, sizeof(bcm_kt_bfd_hw_defs));
    bcm_kt_bfd_hw_defs.hw_call   = &bcm_kt_bfd_hw_calls;
    bcm_kt_bfd_hw_defs.l2        = &bcm_kt_bfd_l2_table;
    bcm_kt_bfd_hw_defs.l3_ipv4   = &bcm_kt_bfd_l3_ipv4_table;
    bcm_kt_bfd_hw_defs.l3_ipv6   = &bcm_kt_bfd_l3_ipv6_table;
    bcm_kt_bfd_hw_defs.l3_tunnel = &bcm_kt_bfd_l3_tunnel_table;
    bcm_kt_bfd_hw_defs.mpls      = &bcm_kt_bfd_mpls_table;

    /* Initialize Common XGS5 BFD module */
    BCM_IF_ERROR_RETURN
        (bcmi_xgs5_bfd_init(unit, &bcm_kt_bfd_drv, &bcm_kt_bfd_hw_defs));

    return BCM_E_NONE;
}

#else   /* BCM_KATANA_SUPPORT && INCLUDE_BFD */
int bcm_esw_kt_bfd_not_empty;
#endif  /* BCM_KATANA_SUPPORT && INCLUDE_BFD */

