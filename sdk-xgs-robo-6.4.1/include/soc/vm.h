/*
 * $Id: vm.h,v 1.5 Broadcom SDK $
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
#ifndef _SOC_VM_H
#define _SOC_VM_H

#include <shared/bitop.h>


#define VM_FLOW_INIT(unit) \
	if (VM_FLOW_USED(unit) == NULL) { return SOC_E_INIT; }
#define VM_FLOW_ID(unit, id) \
	if (id < 0 || id >= VM_FLOW_SIZE(unit)) { return SOC_E_PARAM; }

#define VM_FLOW_ID_GET_FROM_HEAD 0
#define VM_FLOW_ID_GET_FROM_TAIL 1
#define VM_FLOW_ID_GET_CPU_DEFAULT 2


#define VM_RANGER_INIT(unit) \
	if (VM_RANGER_USED(unit) == NULL) { return SOC_E_INIT; }

/* ram type */
typedef enum drv_vm_ram_e {
    DRV_VM_RAM_IVM_KEY_DATA = 1,
    DRV_VM_RAM_IVM_KEY_MASK,
    DRV_VM_RAM_IVM_ACT,
    DRV_VM_RAM_IVM_ALL,
    DRV_VM_RAM_IVM = DRV_VM_RAM_IVM_ALL,

    DRV_VM_RAM_EVM_KEY_DATA,
    DRV_VM_RAM_EVM_KEY_MASK,
    DRV_VM_RAM_EVM_ACT,
    DRV_VM_RAM_EVM_ALL,
    DRV_VM_RAM_EVM = DRV_VM_RAM_EVM_ALL
}drv_vm_ram_t;

/* qualify set */
typedef enum drv_vm_qualify_e {
    DRV_VM_QUAL_IVM_PORT_PROFILE,                 /* Ingress Port Profile */
    DRV_VM_QUAL_IVM_PORT_ID,                           /* Ingress Port ID */
    DRV_VM_QUAL_IVM_INGRESS_STAG_STAUS,    /* Ingress STag Status */
    DRV_VM_QUAL_IVM_INGRESS_CTAG_STAUS,    /* Ingress CTag Status */
    DRV_VM_QUAL_IVM_INGRESS_SVID,                 /* Ingress SVID */
    DRV_VM_QUAL_IVM_INGRESS_CVID,                 /* Ingress CVID */
    DRV_VM_QUAL_IVM_INGRESS_CVID_RANGE,    /* Ingress CVID Range */
    DRV_VM_QUAL_IVM_INGRESS_SPCP,                 /* Ingress SPCP */
    DRV_VM_QUAL_IVM_INGRESS_CPCP,                 /* Ingress CPCP */
    DRV_VM_QUAL_IVM_INGRESS_FRAME_TYPE,    /* Ingress Frame Type */
    DRV_VM_QUAL_IVM_INGRESS_ETHER_TYPE,     /* Ingress Ether Type */

    DRV_VM_QUAL_EVM_INGRESS_PORT_ID,          /* Ingress Port ID */
    DRV_VM_QUAL_EVM_INGRESS_VPORT_ID,         /* Ingress VPort ID */
    DRV_VM_QUAL_EVM_FLOW_ID,                          /* Flow ID */
    DRV_VM_QUAL_EVM_EGRESS_PORT_ID,            /* Egress Port ID */
    DRV_VM_QUAL_EVM_EGRESS_VPORT_ID,          /* Egress VPort ID */

    DRV_VM_QUAL_INVALID,
    DRV_VM_QUAL_COUNT
}drv_vm_qual_t;

/* field set */
typedef enum drv_vm_field_e {
    /* IVM Key */
    DRV_VM_FIELD_IVM_VALID,
    DRV_VM_FIELD_IVM_INGRESS_PORT_PROFILE,
    DRV_VM_FIELD_IVM_INGRESS_PORT_ID,
    DRV_VM_FIELD_IVM_INGRESS_STAG_STAUS,
    DRV_VM_FIELD_IVM_INGRESS_CTAG_STATUS,
    DRV_VM_FIELD_IVM_INGRESS_SVID,
    DRV_VM_FIELD_IVM_INGRESS_CVID,
    DRV_VM_FIELD_IVM_INGRESS_CVID_RANGE,
    DRV_VM_FIELD_IVM_INGRESS_SPCP,
    DRV_VM_FIELD_IVM_INGRESS_CPCP,
    DRV_VM_FIELD_IVM_FRAME_TYPE,
    DRV_VM_FIELD_IVM_ETHER_TYPE,
    DRV_VM_FIELD_IVM_RESERVED,

    /* EVM Key */
    DRV_VM_FIELD_EVM_VALID,
    DRV_VM_FIELD_EVM_INGRESS_PORT_ID,
    DRV_VM_FIELD_EVM_INGRESS_VPORT_ID,
    DRV_VM_FIELD_EVM_FLOW_ID,
    DRV_VM_FIELD_EVM_EGRESS_PORT_ID,
    DRV_VM_FIELD_EVM_EGRESS_VPORT_ID,

    /* IVM Action */
    DRV_VM_FIELD_IVM_SPCP_MARKING_POLICY,
    DRV_VM_FIELD_IVM_CPCP_MARKING_POLICY,
    DRV_VM_FIELD_IVM_VLAN_ID,
    DRV_VM_FIELD_IVM_FLOW_ID,
    DRV_VM_FIELD_IVM_VPORT_ID,
    DRV_VM_FIELD_IVM_VPORT_SPCP,
    DRV_VM_FIELD_IVM_VPORT_CPCP,
    DRV_VM_FIELD_IVM_VPORT_DP,
    DRV_VM_FIELD_IVM_VPORT_TC,

    /* EVMAction */
    DRV_VM_FIELD_EVM_STAG_ACT,
    DRV_VM_FIELD_EVM_CTAG_ACT,
    DRV_VM_FIELD_EVM_NEW_SVID,
    DRV_VM_FIELD_EVM_NEW_CVID,
    DRV_VM_FIELD_EVM_OUTER_PCP_REPLACE,
    DRV_VM_FIELD_EVM_NEW_OUTER_PCP,
    DRV_VM_FIELD_EVM_INNER_PCP_REPLACE,
    DRV_VM_FIELD_EVM_NEW_INNER_PCP,
    
    DRV_VM_FIELD_COUNT
}drv_vm_field_t;

/* action type */
typedef enum drv_vm_action_e {
    DRV_VM_ACT_IVM_SPCP_MARKING_POLICY = 1,
    DRV_VM_ACT_IVM_CPCP_MARKING_POLICY,
    DRV_VM_ACT_IVM_VLAN_ID,
    DRV_VM_ACT_IVM_FLOW_ID,
    DRV_VM_ACT_IVM_VPORT_ID,
    DRV_VM_ACT_IVM_VPORT_SPCP,
    DRV_VM_ACT_IVM_VPORT_CPCP,
    DRV_VM_ACT_IVM_VPORT_DP,
    DRV_VM_ACT_IVM_VPORT_TC,

    DRV_VM_ACT_EVM_STAG,
    DRV_VM_ACT_EVM_CTAG,
    DRV_VM_ACT_EVM_NEW_SVID,
    DRV_VM_ACT_EVM_NEW_CVID,

    DRV_VM_ACT_EVM_OUTER_PCP_REPLACE,
    DRV_VM_ACT_EVM_NEW_OUTER_PCP,
    DRV_VM_ACT_EVM_INNER_PCP_REPLACE,
    DRV_VM_ACT_EVM_NEW_INNER_PCP,

    DRV_VM_ACT_COUNT
}drv_vm_action_t;

/* Parameters for IVM action SPCP/CPCP marking policy. */
typedef enum drv_vm_action_pcp_mark_e {
    DRV_VM_ACTION_PCP_MARK_MAPPED,
    DRV_VM_ACTION_PCP_MARK_INGRESS_INNER_PCP,
    DRV_VM_ACTION_PCP_MARK_INGRESS_OUTER_PCP,
    DRV_VM_ACTION_PCP_MARK_PORT_DEFAULT,
    
    DRV_VM_ACTION_PCP_MARK_COUNT
}drv_vm_action_pcp_mark_t;

/* Parameters for EVM action STag/CTag. */
typedef enum drv_vm_action_tag_policy_e {
    DRV_VM_ACTION_TAG_AS_RECEIVED,
    DRV_VM_ACTION_TAG_AS_NORMALIZED,
    DRV_VM_ACTION_TAG_AS_COPY,
    DRV_VM_ACTION_TAG_AS_REMOVE,
    DRV_VM_ACTION_TAG_AS_REPLACE,
    
    DRV_VM_ACTION_TAG_COUNT
}drv_vm_action_tag_policy_t;

/* driver ivm entry format */
typedef struct drv_vm_entry_s {
    uint32  id;
    uint32  prio;
    uint32  key_data[2];
    uint32  key_mask[2];
    uint32  act_data[2];
    SHR_BITDCL  w[_SHR_BITDCLSIZE(DRV_VM_QUAL_COUNT)];

}drv_vm_entry_t;


#endif /* _SOC_VM_H */


