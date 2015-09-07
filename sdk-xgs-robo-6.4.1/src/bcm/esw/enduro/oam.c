/*
 * $Id: oam.c,v 1.145 Broadcom SDK $
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

#include <shared/bsl.h>

#include <sal/core/libc.h>
#include <soc/defs.h>
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/profile_mem.h>
#include <soc/hash.h>
#include <soc/l3x.h>
#include <soc/enduro.h>
#if defined(BCM_KATANA_SUPPORT)
#include <soc/katana.h>
#endif /* BCM_KATANA_SUPPORT */

#include <bcm/l3.h>
#include <bcm/oam.h>

#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
#include <bcm_int/esw/bhh.h>
#endif /* BCM_KATANA_SUPPORT */

#include <bcm_int/esw/oam.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw/enduro.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw_dispatch.h>

#if defined(BCM_ENDURO_SUPPORT)

#define BCM_OAM_LM_COUNTER_INDEX_INVALID  (-1)
#define BCM_OAM_LM_COUNTER_OFFSET         (8)
#define LEVEL_BIT_COUNT (3)
#define LEVEL_COUNT (1 << (LEVEL_BIT_COUNT))
#define MAX_ENDPOINT_LEVEL (LEVEL_COUNT - 1)
#define MANGLED_GROUP_NAME_LENGTH (BCM_OAM_GROUP_NAME_LENGTH)

#define UNSUPPORTED_ENDPOINT_FLAGS \
    (BCM_OAM_ENDPOINT_CCM_COPYTOCPU | \
     BCM_OAM_ENDPOINT_CCM_DROP | \
     BCM_OAM_ENDPOINT_DM_COPYTOCPU | \
     BCM_OAM_ENDPOINT_DM_DROP | \
     BCM_OAM_ENDPOINT_LB_COPYTOCPU | \
     BCM_OAM_ENDPOINT_LB_DROP | \
     BCM_OAM_ENDPOINT_LT_COPYTOCPU | \
     BCM_OAM_ENDPOINT_LT_DROP | \
     BCM_OAM_ENDPOINT_USE_QOS_MAP | \
     BCM_OAM_ENDPOINT_MATCH_INNER_VLAN | \
     BCM_OAM_ENDPOINT_REMOTE_DEFECT_TX | \
     BCM_OAM_ENDPOINT_CCM_COPYFIRSTTOCPU | \
     BCM_OAM_ENDPOINT_PRI_TAG)   

#define CHECK_INIT \
    if (!oam_info_p->initialized) \
    { \
        return BCM_E_INIT; \
    }

#define SET_OAM_INFO oam_info_p = &en_oam_info[unit];

#define SET_GROUP(_group_index_) group_p = \
    oam_info_p->groups + _group_index_;

#define SET_ENDPOINT(_endpoint_index_) endpoint_p = \
    oam_info_p->endpoints + _endpoint_index_;

#define VALIDATE_GROUP_INDEX(_group_index_) \
    if ((_group_index_) < 0 || (_group_index_) >= oam_info_p->group_count) \
    { \
        return BCM_E_PARAM; \
    } \

#define VALIDATE_ENDPOINT_INDEX(_endpoint_index_) \
    if ((_endpoint_index_) < 0 || \
        (_endpoint_index_) >= oam_info_p->endpoint_count) \
    { \
        return BCM_E_PARAM; \
    } \

#define SET_LMEP_FIELD(unit, field, flag, mask) \
    if (soc_mem_field_valid(unit, L3_ENTRY_IPV4_UNICASTm, field)) { \
        soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, field, \
            (flag & mask) ? 1 : 0); \
    } \

#define GET_LMEP_FIELD(unit, field, flag) \
    if (soc_mem_field_valid(unit, L3_ENTRY_IPV4_UNICASTm, field)) { \
        if (soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry, field)) { \
            endpoint_info->flags |= flag; \
        } \
    }

#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)

#define BCM_OAM_BHH_VALIDATE_END_POINT(oam_p, ep) \
    if(((ep) < (oam_info_p->remote_endpoint_count + \
                oam_info_p->local_tx_endpoint_count + \
                oam_info_p->local_rx_endpoint_count)) || \
        ((ep) >= oam_info_p->endpoint_count)) \
    { \
        return BCM_E_PARAM; \
    }

#define BCM_OAM_BHH_GET_UKERNEL_EP(oam_p, ep) \
        (ep  -  \
            (oam_p->remote_endpoint_count + \
            oam_p->local_tx_endpoint_count + \
            oam_p->local_rx_endpoint_count))

#define BCM_OAM_BHH_GET_SDK_EP(oam_p, ep) \
	    ((ep) + \
            (oam_p->remote_endpoint_count + \
            oam_p->local_tx_endpoint_count + \
            oam_p->local_rx_endpoint_count))
/*
 * Protos
 */
STATIC int _bcm_en_oam_bhh_hw_init(int unit);
STATIC int _bcm_en_oam_bhh_msg_send_receive(int unit, uint8 s_subclass,
                             uint16 s_len, uint32 s_data,
				 uint8 r_subclass, uint16 *r_len);
STATIC void _bcm_en_oam_bhh_callback_thread(void *param);

#endif

/* Cache of ING_SERVICE_PRI_MAP Profile Table */
static soc_profile_mem_t *ing_pri_map_profile[BCM_MAX_NUM_UNITS] = {NULL};
#define ING_SERVICE_PRI_MAP_PROFILE_DEFAULT  0

#if defined(BCM_KATANA_SUPPORT)
/* Cache of OAM Opcode Profile Table */
static soc_profile_mem_t *oam_opcode_profile[BCM_MAX_NUM_UNITS] = {NULL};
#define OAM_OPCODE_PROFILE_DEFAULT  0
#endif /* BCM_KATANA_SUPPORT */

#define SOC_MODID_BIT_POS(unit)   _shr_popcount((unsigned int)SOC_PORT_ADDR_MAX(unit))
#define SOC_MODID_MASK(unit)  (SOC_INFO(unit).modid_max << SOC_MODID_BIT_POS(unit))

static _bcm_oam_info_t en_oam_info[BCM_MAX_NUM_UNITS];
static int _bcm_en_oam_quantize_ccm_period(int period);

static _bcm_oam_fault_t en_group_faults[] =
{
    {CURRENT_XCON_CCM_DEFECTf, STICKY_XCON_CCM_DEFECTf,
        BCM_OAM_GROUP_FAULT_CCM_XCON, 0x08},

    {CURRENT_ERROR_CCM_DEFECTf, STICKY_ERROR_CCM_DEFECTf,
        BCM_OAM_GROUP_FAULT_CCM_ERROR, 0x04},

    {CURRENT_SOME_RMEP_CCM_DEFECTf, STICKY_SOME_RMEP_CCM_DEFECTf,
        BCM_OAM_GROUP_FAULT_CCM_TIMEOUT, 0x02},

    {CURRENT_SOME_RDI_DEFECTf, STICKY_SOME_RDI_DEFECTf,
        BCM_OAM_GROUP_FAULT_REMOTE, 0x01},

    {0, 0, 0, 0}
};

static _bcm_oam_fault_t en_endpoint_faults[] =
{
    {CURRENT_RMEP_PORT_STATUS_DEFECTf, STICKY_RMEP_PORT_STATUS_DEFECTf,
        BCM_OAM_ENDPOINT_FAULT_PORT_DOWN, 0x08},

    {CURRENT_RMEP_INTERFACE_STATUS_DEFECTf,
        STICKY_RMEP_INTERFACE_STATUS_DEFECTf,
        BCM_OAM_ENDPOINT_FAULT_INTERFACE_DOWN, 0x04},

    {CURRENT_RMEP_CCM_DEFECTf, STICKY_RMEP_CCM_DEFECTf,
        BCM_OAM_ENDPOINT_FAULT_CCM_TIMEOUT, 0x20},

    {CURRENT_RMEP_LAST_RDIf, STICKY_RMEP_LAST_RDIf,
        BCM_OAM_ENDPOINT_FAULT_REMOTE, 0x10},

    {0, 0, 0, 0}
};

typedef struct en_interrupt_enable_fields_s {
    soc_field_t field;
    uint32      value;
} en_interrupt_enable_fields_t;

#ifndef BCM_KATANA_SUPPORT
static _bcm_oam_interrupt_t en_interrupts[] =
{
    {ANY_RMEP_TLV_PORT_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_PORT_DOWN_INTRf, bcmOAMEventEndpointPortDown},

    {ANY_RMEP_TLV_PORT_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_PORT_UP_INTRf, bcmOAMEventEndpointPortUp},

    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_DOWN_INTRf, bcmOAMEventEndpointInterfaceDown},

    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_INTRf, bcmOAMEventEndpointInterfaceUp},

    {XCON_CCM_DEFECT_STATUSr, INVALIDf, FIRST_MA_INDEXf,
        ERROR_CCM_DEFECT_INTRf, bcmOAMEventGroupCCMxcon},

    {ERROR_CCM_DEFECT_STATUSr, INVALIDf, FIRST_MA_INDEXf,
        ERROR_CCM_DEFECT_INTRf, bcmOAMEventGroupCCMError},

    {SOME_RDI_DEFECT_STATUSr, FIRST_RMEP_INDEXf, FIRST_MA_INDEXf,
        SOME_RDI_DEFECT_INTRf, bcmOAMEventGroupRemote},

    {SOME_RMEP_CCM_DEFECT_STATUSr, FIRST_RMEP_INDEXf, FIRST_MA_INDEXf,
        SOME_RMEP_CCM_DEFECT_INTRf, bcmOAMEventGroupCCMTimeout},

    {INVALIDr, INVALIDf, 0}
};

static en_interrupt_enable_fields_t en_interrupt_enable_fields[bcmOAMEventCount] =
{
    /* This must be in the same order as the bcm_oam_event_type_t enum */
    { ANY_RMEP_TLV_PORT_DOWN_INT_ENABLEf, 1}, 
    { ANY_RMEP_TLV_PORT_UP_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_DOWN_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_UP_INT_ENABLEf, 1},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { XCON_CCM_DEFECT_INT_ENABLEf, 1},
    { ERROR_CCM_DEFECT_INT_ENABLEf, 1},
    { SOME_RDI_DEFECT_INT_ENABLEf, 1},
    { SOME_RMEP_CCM_DEFECT_INT_ENABLEf, 1},
    { INVALIDf, 0}
};
#else

static _bcm_oam_interrupt_t en_interrupts[] =
{
    {ANY_RMEP_TLV_PORT_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_PORT_DOWN_INT_ENABLEf, 
        bcmOAMEventEndpointPortDown},

    {ANY_RMEP_TLV_PORT_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_PORT_UP_INT_ENABLEf, 
        bcmOAMEventEndpointPortUp},

    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_DOWN_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceDown},

    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_DOWN_TO_UP_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceUp},

    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_TESTING_TO_UP_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceTestingToUp},        

    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UNKNOWN_TO_UP_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceUnknownToUp},   

    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_DORMANT_TO_UP_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceDormantToUp},  

    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_NOTPRESENT_TO_UP_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceNotPresentToUp}, 

    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_LLDOWN_TO_UP_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceLLDownToUp}, 

    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_TESTING_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceTesting}, 

    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_UNKNOWN_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceUnkonwn}, 

    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_DORMANT_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceDormant}, 

    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_NOTPRESENT_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceNotPresent}, 	

    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_LLDOWN_TRANSITION_INT_ENABLEf, 
        bcmOAMEventEndpointInterfaceLLDown}, 

    {XCON_CCM_DEFECT_STATUSr, INVALIDf, FIRST_MA_INDEXf,
        ERROR_CCM_DEFECT_INT_ENABLEf, 
        bcmOAMEventGroupCCMxcon},

    {ERROR_CCM_DEFECT_STATUSr, INVALIDf, FIRST_MA_INDEXf,
        ERROR_CCM_DEFECT_INT_ENABLEf, 
        bcmOAMEventGroupCCMError},

    {SOME_RDI_DEFECT_STATUSr, FIRST_RMEP_INDEXf, FIRST_MA_INDEXf,
        SOME_RDI_DEFECT_INT_ENABLEf, 
        bcmOAMEventGroupRemote},

    {SOME_RMEP_CCM_DEFECT_STATUSr, FIRST_RMEP_INDEXf, FIRST_MA_INDEXf,
        SOME_RMEP_CCM_DEFECT_INT_ENABLEf, 
        bcmOAMEventGroupCCMTimeout},

    {INVALIDr, INVALIDf, 0}
};


static en_interrupt_enable_fields_t en_interrupt_enable_fields[bcmOAMEventCount] =
{
    /* This must be in the same order as the bcm_oam_event_type_t enum */
    { ANY_RMEP_TLV_PORT_DOWN_INT_ENABLEf, 1}, 
    { ANY_RMEP_TLV_PORT_UP_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_UP_TO_DOWN_TRANSITION_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_DOWN_TO_UP_TRANSITION_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_TESTING_TO_UP_TRANSITION_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_UNKNOWN_TO_UP_TRANSITION_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_DORMANT_TO_UP_TRANSITION_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_NOTPRESENT_TO_UP_TRANSITION_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_LLDOWN_TO_UP_TRANSITION_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_UP_TO_TESTING_TRANSITION_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_UP_TO_UNKNOWN_TRANSITION_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_UP_TO_DORMANT_TRANSITION_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_UP_TO_NOTPRESENT_TRANSITION_INT_ENABLEf, 1},
    { ANY_RMEP_TLV_INTERFACE_UP_TO_LLDOWN_TRANSITION_INT_ENABLEf, 1},
    { XCON_CCM_DEFECT_INT_ENABLEf, 1},
    { ERROR_CCM_DEFECT_INT_ENABLEf, 1},
    { SOME_RDI_DEFECT_INT_ENABLEf, 1},
    { SOME_RMEP_CCM_DEFECT_INT_ENABLEf, 1},
    { INVALIDf, 0}
};

#endif

static uint32 en_ccm_periods[] =
{
    BCM_OAM_ENDPOINT_CCM_PERIOD_DISABLED,
    BCM_OAM_ENDPOINT_CCM_PERIOD_3MS,
    BCM_OAM_ENDPOINT_CCM_PERIOD_10MS,
    BCM_OAM_ENDPOINT_CCM_PERIOD_100MS,
    BCM_OAM_ENDPOINT_CCM_PERIOD_1S,
    BCM_OAM_ENDPOINT_CCM_PERIOD_10S,
    BCM_OAM_ENDPOINT_CCM_PERIOD_1M,
    BCM_OAM_ENDPOINT_CCM_PERIOD_10M,
    _BCM_OAM_ENDPOINT_CCM_PERIOD_UNDEFINED
};

static void *_bcm_en_oam_alloc_clear(unsigned int size, char *description)
{
    void *block_p;

    block_p = sal_alloc(size, description);

    if (block_p != NULL)
    {
        sal_memset(block_p, 0, size);
    }

    return block_p;
}

static int _bcm_en_oam_handle_interrupt(int unit, soc_field_t fault_field)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_interrupt_t *interrupt_p;
    uint32 interrupt_status;
    bcm_oam_group_t group_index;
    bcm_oam_endpoint_t endpoint_index;
    _bcm_oam_event_handler_t *event_handler_p;
    uint32 flags = 0;

    SET_OAM_INFO;

    CHECK_INIT;

    BCM_IF_ERROR_RETURN(bcm_esw_oam_lock(unit));

    for (interrupt_p = en_interrupts;
        interrupt_p->status_register != INVALIDr;
        ++interrupt_p)
    {
        if (BCM_FAILURE(soc_reg32_get(unit, interrupt_p->status_register,
                        REG_PORT_ANY, 0, &interrupt_status))) {
            continue;
        }

        if (!soc_reg_field_get(unit, interrupt_p->status_register,
                                     interrupt_status, VALIDf))
        {
            continue;
        }
        if (soc_reg_field_get(unit, interrupt_p->status_register,
            interrupt_status, VALIDf) &&
            oam_info_p->event_handler_count[interrupt_p->event_type] > 0)
        {
            /* Reset callback flag for each interrupt */
            flags = 0;
            group_index = (interrupt_p->group_index_field != INVALIDf) ?
                (int)soc_reg_field_get(unit, interrupt_p->status_register,
                    interrupt_status, interrupt_p->group_index_field) :
                BCM_OAM_GROUP_INVALID;

            endpoint_index = (interrupt_p->endpoint_index_field != INVALIDf) ?
                (int)soc_reg_field_get(unit, interrupt_p->status_register,
                    interrupt_status, interrupt_p->endpoint_index_field) :
                BCM_OAM_ENDPOINT_INVALID;

            if (endpoint_index != BCM_OAM_ENDPOINT_INVALID)
            {
                /* Find the logical endpoint for this RMEP */

                endpoint_index = oam_info_p->remote_endpoints[endpoint_index];
            }

            flags |= soc_reg_field_get(unit, interrupt_p->status_register,
                interrupt_status, MULTIf) ? BCM_OAM_EVENT_FLAGS_MULTIPLE : 0;

            for (event_handler_p = oam_info_p->event_handler_list_p;
                event_handler_p != NULL;
                event_handler_p = event_handler_p->next_p)
            {
                if (SHR_BITGET(event_handler_p->event_types.w,
                    interrupt_p->event_type))
                {
                    event_handler_p->cb(unit, flags, interrupt_p->event_type,
                        group_index, endpoint_index,
                        event_handler_p->user_data);
                }
            }
        }

        /* Clear interrupt */
        if (BCM_FAILURE(soc_reg32_set(unit, interrupt_p->status_register,
                        REG_PORT_ANY, 0, 0))) {
            continue;
        }
    }

    return bcm_esw_oam_unlock(unit);
}

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_0

/*
 * Function:
 *      bcm_en_oam_scache_alloc
 * Purpose:
 *      Allocate memory for OAM module in scache
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcm_en_oam_scache_alloc(int unit)
{
    _bcm_oam_info_t *oam_info_p;
    soc_scache_handle_t scache_handle;
    uint8 *oam_scache;
    int alloc_sz = 0;

    SET_OAM_INFO;
    alloc_sz = BCM_OAM_GROUP_NAME_LENGTH * (oam_info_p->group_count);

    /* Number of oam groups */
    alloc_sz += sizeof(int);

    /* VFP group, IFP VP group, IFP GLP group */
    alloc_sz += 3 * sizeof(bcm_field_group_t);

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_OAM, 0);
    BCM_IF_ERROR_RETURN(_bcm_esw_scache_ptr_get(unit, scache_handle, 1,
                        alloc_sz, &oam_scache, BCM_WB_DEFAULT_VERSION, NULL));
    return BCM_E_NONE;
}

static int _bcm_en_oam_warm_boot(int unit)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_group_t *group_p;
    int group_index;
    maid_reduction_entry_t maid_reduction_entry;
    ma_state_entry_t ma_state_entry;
    int maid_reduction_valid;
    int ma_state_valid;
    _bcm_oam_endpoint_t *endpoint_p;
    int l3_entry_count;
    int l3_entry_index;
    l3_entry_ipv4_unicast_entry_t l3_entry;
    uint32 level_bitmap;
    int level;
    int lmep_index;
    int stable_size;
    int glp;
    int port;
    int epidx;
    int group_count;
    bcm_vlan_t vlan;
    lmep_entry_t lmep_entry;
    rmep_entry_t rmep_entry;
    ma_index_entry_t ma_index_entry;
    uint8 *oam_scache;
    soc_scache_handle_t scache_handle;

    SET_OAM_INFO;

    /* Get groups */

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
    if (!SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit) && (stable_size > 0)) {

        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_OAM, 0);

        BCM_IF_ERROR_RETURN(_bcm_esw_scache_ptr_get(unit, scache_handle, 0, 0,
            &oam_scache, BCM_WB_DEFAULT_VERSION, NULL));

        /* Recover the FP groups */
        sal_memcpy(&oam_info_p->vfp_group, oam_scache, sizeof(bcm_field_group_t));
        oam_scache += sizeof(bcm_field_group_t);
        sal_memcpy(&oam_info_p->fp_vp_group, oam_scache, sizeof(bcm_field_group_t));
        oam_scache += sizeof(bcm_field_group_t);
        sal_memcpy(&oam_info_p->fp_glp_group, oam_scache, sizeof(bcm_field_group_t));
        oam_scache += sizeof(bcm_field_group_t);

        /* Recover the OAM groups */
        sal_memcpy(&group_count, oam_scache, sizeof(int));
        oam_scache += sizeof(int);
    }

    for (group_index = 0; group_index < oam_info_p->group_count;
        ++group_index)
    {
        SOC_IF_ERROR_RETURN(READ_MAID_REDUCTIONm(unit,
            MEM_BLOCK_ANY, group_index, &maid_reduction_entry));

        SOC_IF_ERROR_RETURN(READ_MA_STATEm(unit, MEM_BLOCK_ANY, group_index,
            &ma_state_entry));

        maid_reduction_valid = soc_MAID_REDUCTIONm_field32_get(unit,
            &maid_reduction_entry, VALIDf);

        ma_state_valid = soc_MA_STATEm_field32_get(unit,
            &ma_state_entry, VALIDf);

        if (maid_reduction_valid || ma_state_valid)
        {
            if (!maid_reduction_valid || !ma_state_valid)
            {
                return BCM_E_INTERNAL;
            }

            SET_GROUP(group_index);

            if (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit) || (stable_size == 0)) {
                /* Return zeros as the name */
                sal_memset(group_p->name, 0, BCM_OAM_GROUP_NAME_LENGTH);
            } else {
                /* Use stored values */
                /*
                 * COVERITY
                 * oam_scache has been initilized in _bcm_esw_scache_ptr_get
                 */
                 /* coverity[uninit_use_in_call : FALSE] */
                memcpy(group_p->name, oam_scache, BCM_OAM_GROUP_NAME_LENGTH);
                oam_scache += BCM_OAM_GROUP_NAME_LENGTH;
            }
            group_p->in_use = 1;
        }
    }

    /* Get endpoints */

    endpoint_p = oam_info_p->endpoints;
    l3_entry_count = soc_mem_index_count(unit, L3_ENTRY_IPV4_UNICASTm);

    for (l3_entry_index = 0; l3_entry_index < l3_entry_count; ++l3_entry_index)
    {
        SOC_IF_ERROR_RETURN(READ_L3_ENTRY_IPV4_UNICASTm(unit, MEM_BLOCK_ANY,
            l3_entry_index, &l3_entry));

        if (soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry, VALIDf))
        {
            switch (soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry, KEY_TYPEf))
            {
                case TR_L3_HASH_KEY_TYPE_RMEP:
                    endpoint_p->remote_index =
                        soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry,
                            RMEP__RMEP_PTRf);

                    SOC_IF_ERROR_RETURN(READ_RMEPm(unit, MEM_BLOCK_ANY,
                        endpoint_p->remote_index, &rmep_entry));

                    if (!soc_RMEPm_field32_get(unit, &rmep_entry, VALIDf))
                    {
                        return BCM_E_INTERNAL;
                    }

                    endpoint_p->in_use = 1;
                    endpoint_p->is_remote = 1;

                    endpoint_p->group_index = soc_RMEPm_field32_get(unit,
                        &rmep_entry, MAID_INDEXf);

                    endpoint_p->name = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit,
                        &l3_entry, RMEP__MEPIDf);

                    endpoint_p->level = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit,
                        &l3_entry, RMEP__MDLf);

                    endpoint_p->vlan = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit,
                        &l3_entry, RMEP__VIDf);

                    if (endpoint_p->vlan == 0) {
                        port = soc_L3_ENTRY_IPV4_UNICASTm_field32_get
                                              (unit, &l3_entry, RMEP__SGLPf);
#if defined(INCLUDE_L3)
                        if (_bcm_vp_used_get(unit, port, _bcmVpTypeMim) > 0) {
                            /* Associated with MIM VP */
                            endpoint_p->vp = port;
                        } else
#endif /* INCLUDE_L3 */
                        {
                            /* Associated with physical port */
                            endpoint_p->glp = port;
                        }
                    } else {
                        endpoint_p->glp = soc_L3_ENTRY_IPV4_UNICASTm_field32_get
                                              (unit, &l3_entry, RMEP__SGLPf);
                    }

                    SHR_BITSET(oam_info_p->remote_endpoints_in_use,
                        endpoint_p->remote_index);

                    ++endpoint_p;

                    break;

                case TR_L3_HASH_KEY_TYPE_LMEP:
                    level_bitmap = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit,
                        &l3_entry, LMEP__MDL_BITMAPf);

                    for (level = 0; level < LEVEL_COUNT; ++level)
                    {
                        if (level_bitmap & (1 << level))
                        {
                            /* There's an endpoint here at this level */

                            endpoint_p->local_rx_index =
                                soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit,
                                    &l3_entry, LMEP__MA_BASE_PTRf) <<
                                        LEVEL_BIT_COUNT | level;

                            SOC_IF_ERROR_RETURN(READ_MA_INDEXm(unit,
                                MEM_BLOCK_ANY, endpoint_p->local_rx_index,
                                &ma_index_entry));

                            endpoint_p->in_use = 1;
                            endpoint_p->is_remote = 0;
                            endpoint_p->local_rx_enabled = 1;

                            endpoint_p->group_index =
                                soc_MA_INDEXm_field32_get(unit,
                                    &ma_index_entry, MA_PTRf);
#if defined(BCM_KATANA_SUPPORT)
                            if (SOC_IS_KATANAX(unit)) {
                                endpoint_p->opcode_profile_index =
                                    soc_MA_INDEXm_field32_get(unit,
                                                              &ma_index_entry,
                                                              OAM_OPCODE_CONTROL_PROFILE_PTRf);
                            }
#endif /* BCM_KATANA_SUPPORT */
                            /* Name is not used for receive-only endpoints */
                            endpoint_p->name = 0xffff;

                            endpoint_p->level = level;

                            endpoint_p->vlan =
                                soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit,
                                    &l3_entry, LMEP__VIDf);

                            if (endpoint_p->vlan == 0) {
                                endpoint_p->vp = 
                                    soc_L3_ENTRY_IPV4_UNICASTm_field32_get
                                               (unit, &l3_entry, LMEP__SGLPf);
                            } else {
                                endpoint_p->glp = 
                                    soc_L3_ENTRY_IPV4_UNICASTm_field32_get
                                               (unit, &l3_entry, LMEP__SGLPf);
                            }

                            SHR_BITSET(oam_info_p->local_rx_endpoints_in_use,
                                endpoint_p->local_rx_index);

                            ++endpoint_p;
                        }
                    }

                    break;

                default:
                    continue;
            }
        }
    }

    for (lmep_index = 0; lmep_index < oam_info_p->local_tx_endpoint_count;
        ++lmep_index)
    {
        SOC_IF_ERROR_RETURN(READ_LMEPm(unit, MEM_BLOCK_ANY, lmep_index,
            &lmep_entry));

        group_index = soc_LMEPm_field32_get(unit,
            &lmep_entry, MAID_INDEXf);

        SOC_IF_ERROR_RETURN(READ_MAID_REDUCTIONm(unit,
            MEM_BLOCK_ANY, group_index, &maid_reduction_entry));

        if (soc_MAID_REDUCTIONm_field32_get(unit, &maid_reduction_entry, 
                                            VALIDf))
        {
            glp = soc_LMEPm_field32_get(unit, &lmep_entry, DESTf);
            vlan = soc_LMEPm_field32_get(unit, &lmep_entry, VLAN_IDf);

            /* Check if a receive entry exists for this LMEP */

            for (epidx = 0; epidx < oam_info_p->endpoint_count; epidx++)
            {
                SET_ENDPOINT(epidx);
                if (!endpoint_p->in_use) 
                {
                    continue;
                }
                if ((endpoint_p->vlan == vlan) && (endpoint_p->glp == glp) && 
                    (endpoint_p->name == 0xffff)) 
                {
                    break;
                }
            }
            if (epidx == oam_info_p->endpoint_count)
            {
                /* Receive entry not found - local TX only */ 
                /* Allocate a new ID */
                for (epidx = 0; epidx < oam_info_p->endpoint_count; epidx++)
                {
                    SET_ENDPOINT(epidx);
                    if (!endpoint_p->in_use) 
                    {
                        endpoint_p->glp = glp;
                        endpoint_p->vlan = vlan;
                        break;
                    }
                }
            }

            endpoint_p->in_use = 1;
            endpoint_p->is_remote = 0;
            endpoint_p->local_tx_enabled = 1;
            endpoint_p->local_tx_index = lmep_index;
            endpoint_p->name = soc_LMEPm_field32_get(unit, &lmep_entry,
                                                     MEPIDf);
            endpoint_p->level = soc_LMEPm_field32_get(unit, &lmep_entry,
                MDLf);
            endpoint_p->group_index = group_index;

            SHR_BITSET(oam_info_p->local_tx_endpoints_in_use, lmep_index);

            ++endpoint_p;
        }
    }
    return BCM_E_NONE;
}

static int _bcm_en_oam_field_group_process(int unit, bcm_field_group_t group,
                                           void *user_data)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_endpoint_t *endpoint_p;
    bcm_field_qset_t group_qset;
    int glp, local_id, i, j, q, match, entry_size, entry_count, rv = BCM_E_NONE;
    bcm_field_entry_t *entry_arr = NULL;
    uint32 param0, param1;
    bcm_gport_t gport_id;
    bcm_module_t module_id, mod_mask;
    bcm_port_t port_id, port_mask;
    bcm_trunk_t trunk_id;

    SET_OAM_INFO;

    /* Get this group's QSET */
    BCM_IF_ERROR_RETURN(bcm_esw_field_group_get(unit, group, &group_qset));

    /* Check for the VFP QSET for a match */
    match = 1;
    _FIELD_QSET_ITER(oam_info_p->vfp_qs, q) {
        if (!BCM_FIELD_QSET_TEST(group_qset, bcmFieldQualifyStageLookup) ||
            !BCM_FIELD_QSET_TEST(group_qset, q)) {
            match = 0;
            break;
        }
    }
    if (match && (oam_info_p->vfp_group == -1)) {
        /* Get number of entries */
        rv = bcm_esw_field_entry_multi_get(unit, group, 0, NULL, &entry_count);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        /* Get the actual entries - each entry belongs to an endpoint */
        entry_size = entry_count;
        entry_arr = sal_alloc(entry_size * sizeof(bcm_field_entry_t), 
                              "Entry array");
        if (entry_arr == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        } 
        rv = bcm_esw_field_entry_multi_get(unit, group, entry_size, entry_arr, 
                                           &entry_count);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        for (i = 0; i < entry_count; i++) {
            /* Check for bcmFieldActionIncomingMplsPortSet to find endpoint */
            rv = bcm_esw_field_action_get(unit, entry_arr[i], 
                                          bcmFieldActionIncomingMplsPortSet, 
                                          &param0, &param1);
            if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                goto cleanup;
            }
            if (BCM_SUCCESS(rv)) {
                /* Find endpoint that matches this VP */
                for (j = 0; j < oam_info_p->endpoint_count; j++) {
                    endpoint_p = oam_info_p->endpoints + j;
                    if (!endpoint_p->in_use) {
                        continue;
                    }
                    if (endpoint_p->vp == param0) {
                        endpoint_p->vfp_entry = entry_arr[i];
                        if (oam_info_p->vfp_group == -1) {
                            oam_info_p->vfp_group = group;
                            oam_info_p->vfp_entry_count = entry_count;
                        }
                        break;
                    }
                }
            }
            if (rv == BCM_E_NOT_FOUND) {
                /* The FP entry was not for an OAM endpoint */
                rv = BCM_E_NONE;
            }
        }
        sal_free(entry_arr);
        entry_arr = NULL;
    }

    /* Check for the IFP VP QSET for a match */
    match = 1;
    _FIELD_QSET_ITER(oam_info_p->fp_vp_qs, q) {
        if (!BCM_FIELD_QSET_TEST(group_qset, bcmFieldQualifyStageIngress) ||
            !BCM_FIELD_QSET_TEST(group_qset, q)) {
            match = 0;
            break;
        }
    }
    if (match && (oam_info_p->fp_vp_group == -1)) {
        /* Get number of entries */
        rv = bcm_esw_field_entry_multi_get(unit, group, 0, NULL, &entry_count);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        /* Get the actual entries - each entry belongs to an endpoint */
        entry_size = entry_count;
        entry_arr = sal_alloc(entry_size * sizeof(bcm_field_entry_t), 
                              "Entry array");
        if (entry_arr == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        } 
        rv = bcm_esw_field_entry_multi_get(unit, group, entry_size, entry_arr, 
                                           &entry_count);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        for (i = 0; i < entry_count; i++) {
            /* Check for SrcMimGport to find endpoint */
            rv = bcm_esw_field_qualify_SrcMimGport_get(unit, entry_arr[i], 
                                                       &gport_id);
            if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                goto cleanup;
            }
            if (BCM_SUCCESS(rv)) {
                /* Find endpoint that matches this VP with possible actions */
                for (j = 0; j < oam_info_p->endpoint_count; j++) {
                    endpoint_p = oam_info_p->endpoints + j;
                    if (!endpoint_p->in_use) {
                        continue;
                    }
                    if (endpoint_p->vp == BCM_GPORT_MIM_PORT_ID_GET(gport_id)) {
                        if (oam_info_p->fp_vp_group == -1) {
                            oam_info_p->fp_vp_group = group;
                            oam_info_p->fp_vp_entry_count = entry_count;
                        }
                        endpoint_p->fp_entry_tx = entry_arr[i];
                        rv = bcm_esw_field_action_get(unit, entry_arr[i], 
                                                     bcmFieldActionOamLmBasePtr, 
                                                     &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        } 
                        if (rv != BCM_E_NOT_FOUND) { 
                            endpoint_p->lm_counter_index = param0;
                            SHR_BITSET(oam_info_p->lm_counter_in_use, 
                                endpoint_p->lm_counter_index);
                        }
                        rv = bcm_esw_field_action_get(unit, entry_arr[i], 
                                          bcmFieldActionOamServicePriMappingPtr,
                                          &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        } 
                        if (rv != BCM_E_NOT_FOUND) {
                            endpoint_p->pri_map_index = param0;
                            endpoint_p->flags |= BCM_OAM_ENDPOINT_LOSS_MEASUREMENT;
                        }
                        rv = bcm_esw_field_action_get(unit, entry_arr[i],
                                          bcmFieldActionOamDmEnable,
                                          &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        } 
                        if (rv != BCM_E_NOT_FOUND) {
                            if (param0 != 0) {
                                endpoint_p->flags |= BCM_OAM_ENDPOINT_DELAY_MEASUREMENT;
                            }
                        }
                        rv = bcm_esw_field_action_get(unit, entry_arr[i],
                                          bcmFieldActionOamUpMep,
                                          &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        } 
                        if (rv != BCM_E_NOT_FOUND) {
                            if (param0 != 0) {
                                endpoint_p->flags |= BCM_OAM_ENDPOINT_UP_FACING;
                            }
                        }
                        break;
                    }
                }
            } 
            /* Check for DstMimGport to find endpoint */
            rv = bcm_esw_field_qualify_DstMimGport_get(unit, entry_arr[i], 
                                                       &gport_id);
            if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                goto cleanup;
            }
            if (BCM_SUCCESS(rv)) {
                /* Find endpoint that matches this VP */
                for (j = 0; j < oam_info_p->endpoint_count; j++) {
                    endpoint_p = oam_info_p->endpoints + j;
                    if (!endpoint_p->in_use) {
                        continue;
                    }
                    if (endpoint_p->vp == BCM_GPORT_MIM_PORT_ID_GET(gport_id)) {
                        if (oam_info_p->fp_vp_group == -1) {
                            oam_info_p->fp_vp_group = group;
                            oam_info_p->fp_vp_entry_count = entry_count;
                        }
                        endpoint_p->fp_entry_rx = entry_arr[i];
                        rv = bcm_esw_field_action_get(unit, entry_arr[i], 
                                                     bcmFieldActionOamLmBasePtr, 
                                                     &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        } 
                        if (rv != BCM_E_NOT_FOUND) { 
                            endpoint_p->lm_counter_index = param0;
                            SHR_BITSET(oam_info_p->lm_counter_in_use, 
                                endpoint_p->lm_counter_index);
                        }
                        rv = bcm_esw_field_action_get(unit, entry_arr[i], 
                                          bcmFieldActionOamServicePriMappingPtr,
                                          &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        } 
                        if (rv != BCM_E_NOT_FOUND) {
                            endpoint_p->pri_map_index = param0;
                        }
                        break;
                    }
                }
            }
            if (rv == BCM_E_NOT_FOUND) {
                /* The FP entry was not for an OAM endpoint */
                rv = BCM_E_NONE;
            }
        }
        sal_free(entry_arr);
        entry_arr = NULL;
    }

    /* Check for the IFP GLP QSET for a match */
    match = 1;
    _FIELD_QSET_ITER(oam_info_p->fp_glp_qs, q) {
        if (!BCM_FIELD_QSET_TEST(group_qset, bcmFieldQualifyStageIngress) ||
            !BCM_FIELD_QSET_TEST(group_qset, q)) {
            match = 0;
            break;
        }
    }
    if (match && (oam_info_p->fp_glp_group == -1)) {
        /* Get number of entries */
        rv = bcm_esw_field_entry_multi_get(unit, group, 0, NULL, &entry_count);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        /* Get the actual entries - each entry belongs to an endpoint */
        entry_size = entry_count;
        entry_arr = sal_alloc(entry_size * sizeof(bcm_field_entry_t), 
                              "Entry array");
        if (entry_arr == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        } 
        rv = bcm_esw_field_entry_multi_get(unit, group, entry_size, entry_arr, 
                                           &entry_count);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        for (i = 0; i < entry_count; i++) {
            /* Check for SrcPort to find endpoint */
            rv = bcm_esw_field_qualify_SrcPort_get(unit, entry_arr[i], 
                                                   &module_id, &mod_mask,
                                                   &port_id, &port_mask);
            if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                goto cleanup;
            }
            if (BCM_SUCCESS(rv)) {
                if (BCM_GPORT_IS_SET(port_id)) {
                    rv = _bcm_esw_gport_resolve(unit, port_id,
                        &module_id, &port_id, &trunk_id, &local_id);
                    if (BCM_FAILURE(rv)) {
                        goto cleanup;
                    }
                }
                glp = (module_id << SOC_MODID_BIT_POS(unit)) | port_id;
                /* Find endpoint that matches this GLP with possible actions */
                for (j = 0; j < oam_info_p->endpoint_count; j++) {
                    endpoint_p = oam_info_p->endpoints + j;
                    if (!endpoint_p->in_use) {
                        continue;
                    }
                    if (endpoint_p->glp == glp) {
                        if (oam_info_p->fp_glp_group == -1) {
                            oam_info_p->fp_glp_group = group;
                            oam_info_p->fp_glp_entry_count = entry_count;
                        }
                        endpoint_p->fp_entry_tx = entry_arr[i];
                        rv = bcm_esw_field_action_get(unit, entry_arr[i], 
                                                     bcmFieldActionOamLmBasePtr, 
                                                     &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        }
                        if (rv != BCM_E_NOT_FOUND) { 
                            endpoint_p->lm_counter_index = param0;
                            SHR_BITSET(oam_info_p->lm_counter_in_use, 
                                endpoint_p->lm_counter_index);
                        }
                        rv = bcm_esw_field_action_get(unit, entry_arr[i], 
                                          bcmFieldActionOamServicePriMappingPtr,
                                          &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        } 
                        if (rv != BCM_E_NOT_FOUND) {
                            endpoint_p->pri_map_index = param0;
                            endpoint_p->flags |= BCM_OAM_ENDPOINT_LOSS_MEASUREMENT;
                        }
                        rv = bcm_esw_field_action_get(unit, entry_arr[i],
                                          bcmFieldActionOamDmEnable,
                                          &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        } 
                        if (rv != BCM_E_NOT_FOUND) {
                            if (param0 != 0) {
                                endpoint_p->flags |= BCM_OAM_ENDPOINT_DELAY_MEASUREMENT;
                            }
                        }
                        rv = bcm_esw_field_action_get(unit, entry_arr[i],
                                          bcmFieldActionOamUpMep,
                                          &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        } 
                        if (rv != BCM_E_NOT_FOUND) {
                            if (param0 != 0) {
                                endpoint_p->flags |= BCM_OAM_ENDPOINT_UP_FACING;
                            }
                        }
                        break;
                    }
                }
            } 
            /* Check for DstPort to find endpoint */
            rv = bcm_esw_field_qualify_DstPort_get(unit, entry_arr[i], 
                                                   &module_id, &mod_mask,
                                                   &port_id, &port_mask);
            if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                goto cleanup;
            }
            if (BCM_SUCCESS(rv)) {
                if (BCM_GPORT_IS_SET(port_id)) {
                    rv = _bcm_esw_gport_resolve(unit, port_id,
                        &module_id, &port_id, &trunk_id, &local_id);
                    if (BCM_FAILURE(rv)) {
                        goto cleanup;
                    }
                }
                glp = (module_id << SOC_MODID_BIT_POS(unit)) | port_id;
                /* Find endpoint that matches this GLP */
                for (j = 0; j < oam_info_p->endpoint_count; j++) {
                    endpoint_p = oam_info_p->endpoints + j;
                    if (!endpoint_p->in_use) {
                        continue;
                    }
                    if (endpoint_p->glp == glp) {
                        if (oam_info_p->fp_glp_group == -1) {
                            oam_info_p->fp_glp_group = group;
                            oam_info_p->fp_glp_entry_count = entry_count;
                        }
                        endpoint_p->fp_entry_rx = entry_arr[i];
                        rv = bcm_esw_field_action_get(unit, entry_arr[i], 
                                                     bcmFieldActionOamLmBasePtr, 
                                                     &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        } 
                        if (rv != BCM_E_NOT_FOUND) { 
                            endpoint_p->lm_counter_index = param0;
                            SHR_BITSET(oam_info_p->lm_counter_in_use, 
                                endpoint_p->lm_counter_index);
                        }
                        rv = bcm_esw_field_action_get(unit, entry_arr[i], 
                                          bcmFieldActionOamServicePriMappingPtr,
                                          &param0, &param1);
                        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
                            goto cleanup;
                        } 
                        if (rv != BCM_E_NOT_FOUND) {
                            endpoint_p->pri_map_index = param0;
                        }
                        break;
                    }
                }
            }
            if (rv == BCM_E_NOT_FOUND) {
                /* The FP entry was not for an OAM endpoint */
                rv = BCM_E_NONE;
            }
        }
        sal_free(entry_arr);
        entry_arr = NULL;
    }

cleanup:
    if (entry_arr != NULL) {
        sal_free(entry_arr);
    }
    return rv;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

static void _bcm_en_oam_uninstall_vfp(int unit, _bcm_oam_endpoint_t *endpoint_p)
{
	_bcm_oam_info_t *oam_info_p;

	SET_OAM_INFO;

	if (endpoint_p->vfp_entry != 0) {
        (void)bcm_esw_field_entry_destroy(unit, endpoint_p->vfp_entry);
        oam_info_p->vfp_entry_count--;
        if (oam_info_p->vfp_entry_count == 0) {
            (void)bcm_esw_field_group_destroy(unit, oam_info_p->vfp_group);
            oam_info_p->vfp_group = 0;
        }
        endpoint_p->vfp_entry = 0;
    }
}

static void _bcm_en_oam_uninstall_fp(int unit,  _bcm_oam_endpoint_t *endpoint_p)
{
    int i;
    _bcm_oam_info_t *oam_info_p;

    SET_OAM_INFO;

    if (endpoint_p->fp_entry_tx != 0) {
    	(void)bcm_esw_field_entry_destroy(unit, endpoint_p->fp_entry_tx);
    }
    if (endpoint_p->fp_entry_rx != 0) {
        (void)bcm_esw_field_entry_destroy(unit, endpoint_p->fp_entry_rx);
    }
    if (endpoint_p->vp != 0) {
        if (endpoint_p->fp_entry_tx != 0) {
            oam_info_p->fp_vp_entry_count--;

            if (oam_info_p->fp_vp_entry_count == 0) {
               (void)bcm_esw_field_group_destroy(unit, oam_info_p->fp_vp_group);
                oam_info_p->fp_vp_group = 0;
            }
        }
        endpoint_p->vp = 0;  
    } else {
        for (i = 0; i < BCM_SWITCH_TRUNK_MAX_PORTCNT; i++) {
            if (endpoint_p->fp_entry_trunk[i]) {
                (void)bcm_esw_field_entry_destroy(unit, endpoint_p->fp_entry_trunk[i]);
                endpoint_p->fp_entry_trunk[i] = 0;
            }
        }
        if ((endpoint_p->fp_entry_tx != 0) || (endpoint_p->fp_entry_rx != 0)) {
            oam_info_p->fp_glp_entry_count--;

            if (oam_info_p->fp_glp_entry_count == 0) {
                (void)bcm_esw_field_group_destroy(unit, oam_info_p->fp_glp_group);
                oam_info_p->fp_glp_group = 0;
            }
        }
    }

    endpoint_p->fp_entry_tx = 0;
    endpoint_p->fp_entry_rx = 0;
}

static int _bcm_en_oam_install_vfp(int unit, int endpoint_index, 
    bcm_oam_endpoint_info_t *endpoint_info)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_endpoint_t *endpoint_p;
    bcm_gport_t port_id;
    bcm_mac_t mac_ones = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    
    SET_OAM_INFO;
    SET_ENDPOINT(endpoint_index);
    
    if (oam_info_p->vfp_group == 0) {
        /* Create group first */
        BCM_IF_ERROR_RETURN(bcm_esw_field_group_create(unit,
                                 oam_info_p->vfp_qs,
                                 BCM_FIELD_GROUP_PRIO_ANY,
                                 &oam_info_p->vfp_group));
    }
    BCM_IF_ERROR_RETURN    
        (bcm_esw_field_entry_create(unit,
                                    oam_info_p->vfp_group,
                                    &endpoint_p->vfp_entry));

    BCM_IF_ERROR_RETURN
        (bcm_esw_field_qualify_DstMac(unit, endpoint_p->vfp_entry,
                                      endpoint_info->src_mac_address, mac_ones));
        
    BCM_IF_ERROR_RETURN
        (bcm_esw_field_qualify_SrcMac(unit, endpoint_p->vfp_entry,
                                      endpoint_info->dst_mac_address, mac_ones));
                             
    BCM_IF_ERROR_RETURN
        (bcm_esw_field_qualify_OuterVlanId(unit, endpoint_p->vfp_entry,
                                           endpoint_info->vlan, 0x0fff));

    BCM_GPORT_MPLS_PORT_ID_SET(port_id, endpoint_p->vp);

    BCM_IF_ERROR_RETURN
      (bcm_esw_field_action_add(unit, endpoint_p->vfp_entry,
				bcmFieldActionIncomingMplsPortSet,
				port_id, 0));
  
    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->vfp_entry,
                                  bcmFieldActionOamPbbteLookupEnable,
                                  1, 0));
    oam_info_p->vfp_entry_count++;

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif
    
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_en_oam_trunk_install_fp
 * Purpose:
 *      Install fp entries for each port in the trunk for LM/DM higig packets sent 
 *      from CPU to hit and perform OAM operations. This is necessary because
 *      there is no trunk information in higig header.
 * Parameters:
 *      unit - (IN) Unit number.
 *      add_mdl - (IN) Whether to install mdl as key.
 *      endpoint_info - (IN) Pointer to an OAM endpoint structure
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static
int _bcm_en_oam_trunk_install_fp(int unit, int add_mdl, _bcm_oam_endpoint_t *endpoint_p)
{
    int rv = BCM_E_NONE;
    _bcm_oam_info_t *oam_info_p;
    bcm_field_group_t group;
    bcm_gport_t gport;
    bcm_ip6_t data, mask;
    int i, j;
    int member_count;
    bcm_trunk_member_t *member_array = NULL;

    SET_OAM_INFO;

    group = oam_info_p->fp_glp_group;

    BCM_IF_ERROR_RETURN
        (bcm_esw_trunk_get(unit, endpoint_p->glp & 0x7F, NULL,
                           0, NULL, &member_count));

    if (member_count > 0) {
        member_array = sal_alloc(sizeof(bcm_trunk_member_t) * member_count,
                "trunk member array");
        if (NULL == member_array) {
            return BCM_E_MEMORY;
        }
        rv = bcm_esw_trunk_get(unit, endpoint_p->glp & 0x7F, NULL,
                member_count, member_array, &member_count);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return rv;
        }
    }

    for (i = 0; i < member_count; i++) {

        rv = bcm_esw_field_entry_create(unit, group,
                &endpoint_p->fp_entry_trunk[i]);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return rv;
        }

        gport = member_array[i].gport;

        if (endpoint_p->flags & BCM_OAM_ENDPOINT_UP_FACING) {
            rv = bcm_esw_field_qualify_SrcPort(unit, endpoint_p->fp_entry_trunk[i],
                    0, 0x3f, gport, BCM_FIELD_EXACT_MATCH_MASK);
        } else {
            rv = bcm_esw_field_qualify_DstPort(unit, endpoint_p->fp_entry_trunk[i],
                    0, 0x3f, gport, BCM_FIELD_EXACT_MATCH_MASK);
        }
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return rv;
        }

        rv = bcm_esw_field_qualify_OuterVlanId(unit, endpoint_p->fp_entry_trunk[i],
                endpoint_p->vlan, 0xfff);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return rv;
        }

        if ((add_mdl != 0) && (endpoint_p->vp == 0))
        {
            /*
             * Add ethertype and MDL(with DstIp6) for smaller MDL on the same
             * (Port, VLAN) to trap LM packets to CPU.
             */
            for (j = 0; j < 16; j++) {
                data[j] = mask[j] = 0;
            }
            data[0] = (endpoint_p->level << 5);
            mask[0] = 0xE0;

            rv = bcm_esw_field_qualify_DstIp6(unit, endpoint_p->fp_entry_trunk[i],
                    data, mask);
            if (BCM_FAILURE(rv)) {
                sal_free(member_array);
                return rv;
            }

            rv = bcm_esw_field_qualify_EtherType(unit, endpoint_p->fp_entry_trunk[i],
                    0x8902, 0xffff);
            if (BCM_FAILURE(rv)) {
                sal_free(member_array);
                return rv;
            }
        }

        rv = bcm_esw_field_action_add(unit, endpoint_p->fp_entry_trunk[i],
                bcmFieldActionOamLmepEnable,
                1, 0);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return rv;
        }

        rv = bcm_esw_field_action_add(unit, endpoint_p->fp_entry_trunk[i],
                bcmFieldActionOamLmEnable,
                endpoint_p->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT? 1:0, 0);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return rv;
        }

        rv = bcm_esw_field_action_add(unit, endpoint_p->fp_entry_trunk[i],
                bcmFieldActionOamDmEnable,
                endpoint_p->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT? 1:0, 0);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return rv;
        }

        rv = bcm_esw_field_action_add(unit, endpoint_p->fp_entry_trunk[i],
                bcmFieldActionOamLmepMdl,
                endpoint_p->level, 0);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return rv;
        }

        rv = bcm_esw_field_action_add(unit, endpoint_p->fp_entry_trunk[i],
                bcmFieldActionOamUpMep,
                endpoint_p->flags & BCM_OAM_ENDPOINT_UP_FACING? 1:0, 0);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return rv;
        }

        if (endpoint_p->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT)
        {
            /* Assign LM counter and priority mapping table */
            rv = bcm_esw_field_action_add(unit, endpoint_p->fp_entry_trunk[i],
                    bcmFieldActionOamLmBasePtr,
                    endpoint_p->lm_counter_index, 0);
            if (BCM_FAILURE(rv)) {
                sal_free(member_array);
                return rv;
            }

            rv = bcm_esw_field_action_add(unit, endpoint_p->fp_entry_trunk[i],
                    bcmFieldActionOamServicePriMappingPtr,
                    endpoint_p->pri_map_index, 0);
            if (BCM_FAILURE(rv)) {
                sal_free(member_array);
                return rv;
            }
        }

        rv = bcm_esw_field_action_add(unit, endpoint_p->fp_entry_trunk[i],
                bcmFieldActionOamTx, 1, 0);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return rv;
        }
    }

    if (NULL != member_array) {
        sal_free(member_array);
    }
    return rv;
}

static
int _bcm_en_oam_install_fp(int unit, int add_mdl, _bcm_oam_endpoint_t *endpoint_p)
{
    _bcm_oam_info_t *oam_info_p;
    bcm_field_group_t group;
    bcm_gport_t gport;
    bcm_ip6_t data, mask;
    bcm_module_t module_id;
    bcm_port_t port_id;
    int i, mep_on_trunk = 0, skip_entry_rx = 0;

    SET_OAM_INFO;

    if ((endpoint_p->vp != 0) && (oam_info_p->fp_vp_group == 0)) {
        /* Create group first */
        BCM_IF_ERROR_RETURN(bcm_esw_field_group_create(unit,
                                 oam_info_p->fp_vp_qs,
                                 BCM_FIELD_GROUP_PRIO_ANY,
                                 &oam_info_p->fp_vp_group));
    }
    
    if ((endpoint_p->vp == 0) && (oam_info_p->fp_glp_group == 0)) {
        /* Create group first */
        BCM_IF_ERROR_RETURN(bcm_esw_field_group_create(unit,
                                 oam_info_p->fp_glp_qs,
                                 BCM_FIELD_GROUP_PRIO_ANY,
                                 &oam_info_p->fp_glp_group));
    }

    if (endpoint_p->vp) {
        group = oam_info_p->fp_vp_group;
    } else {
        group = oam_info_p->fp_glp_group;
    }

    if (endpoint_p->glp & (1 << SOC_TRUNK_BIT_POS(unit))) {
        mep_on_trunk = 1;
    }

    if ((mep_on_trunk != 0) &&
        (endpoint_p->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT) &&
        !(endpoint_p->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT) && 
        !(endpoint_p->flags & BCM_OAM_ENDPOINT_UP_FACING)) {
        /* 
         * Skip the entry to match DM packets sent from CPU for down meps that are 
         * created on trunk and only support delay measurement. Create entry for
         * each port in the trunk instead. This is only applied to DM down meps.
         * The entry is always required for LM MEP to record packet counts.
         */
        skip_entry_rx = 1;
    }

    BCM_IF_ERROR_RETURN
        (bcm_esw_field_entry_create(unit,
                                    group,
                                    &endpoint_p->fp_entry_tx));

    if (0 == skip_entry_rx) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_field_entry_create(unit,
                                        group,
                                        &endpoint_p->fp_entry_rx));
    }

    if (endpoint_p->vp != 0) {
    	BCM_GPORT_MIM_PORT_ID_SET(gport, endpoint_p->vp);

    	BCM_IF_ERROR_RETURN
    	    (bcm_esw_field_qualify_SrcMimGport(unit, endpoint_p->fp_entry_tx,
                                               gport));
        BCM_IF_ERROR_RETURN
            (bcm_esw_field_qualify_DstMimGport(unit, endpoint_p->fp_entry_rx,
                                               gport));
    } else {
        if (mep_on_trunk != 0) {
            /* Trunk */
            BCM_GPORT_TRUNK_SET(gport, endpoint_p->glp & 0x7F);
            BCM_IF_ERROR_RETURN
                (bcm_esw_field_qualify_SrcTrunk(unit, endpoint_p->fp_entry_tx,
                gport, BCM_FIELD_EXACT_MATCH_MASK));

            if (!skip_entry_rx) {
                BCM_IF_ERROR_RETURN
                (bcm_esw_field_qualify_DstTrunk(unit, endpoint_p->fp_entry_rx,
                    gport, BCM_FIELD_EXACT_MATCH_MASK));
            }
        } else {
            module_id = (endpoint_p->glp & SOC_MODID_MASK(unit)) >> SOC_MODID_BIT_POS(unit);
            port_id = (endpoint_p->glp & SOC_PORT_ADDR_MAX(unit));

            if (module_id != 0) {
                /* Modport */
                BCM_GPORT_MODPORT_SET(gport, module_id, port_id);
            } else {
                /* Local port */
                BCM_IF_ERROR_RETURN(bcm_esw_port_gport_get(unit, port_id, &gport));
            }
            BCM_IF_ERROR_RETURN
                (bcm_esw_field_qualify_SrcPort(unit, endpoint_p->fp_entry_tx,
                    0, 0x3f, gport, BCM_FIELD_EXACT_MATCH_MASK));
            BCM_IF_ERROR_RETURN
                (bcm_esw_field_qualify_DstPort(unit, endpoint_p->fp_entry_rx,
                    0, 0x3f, gport, BCM_FIELD_EXACT_MATCH_MASK));
        }

        BCM_IF_ERROR_RETURN
            (bcm_esw_field_qualify_OuterVlanId(unit, endpoint_p->fp_entry_tx,
                                               endpoint_p->vlan, 0xfff));

        if (0 == skip_entry_rx) {
            BCM_IF_ERROR_RETURN
            (bcm_esw_field_qualify_OuterVlanId(unit, endpoint_p->fp_entry_rx,
                                               endpoint_p->vlan, 0xfff));
        }
    }

    if ((add_mdl != 0) && (endpoint_p->vp == 0))
    {
        /*
         * Add ethertype and MDL(with DstIp6) for smaller MDL on the same
         * (Port, VLAN) to trap LM packets to CPU.
         */
        for (i = 0; i < 16; i++) {
            data[i] = mask[i] = 0;
        }
        data[0] = (endpoint_p->level << 5);
        mask[0] = 0xE0;

        BCM_IF_ERROR_RETURN
            (bcm_esw_field_qualify_DstIp6(unit, endpoint_p->fp_entry_tx,
                                          data, mask));
        BCM_IF_ERROR_RETURN
            (bcm_esw_field_qualify_EtherType(unit, endpoint_p->fp_entry_tx,
                                        0x8902, 0xffff));

        if (0 == skip_entry_rx) {
            BCM_IF_ERROR_RETURN
            (bcm_esw_field_qualify_DstIp6(unit, endpoint_p->fp_entry_rx,
                                          data, mask));
            BCM_IF_ERROR_RETURN
            (bcm_esw_field_qualify_EtherType(unit, endpoint_p->fp_entry_rx,
                                        0x8902, 0xffff));
        }
    }

    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_tx,
                                  bcmFieldActionOamLmepEnable,
                                  1, 0));
    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_tx,
            bcmFieldActionOamLmEnable,
            endpoint_p->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT? 1:0, 0));

    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_tx,
            bcmFieldActionOamDmEnable,
            endpoint_p->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT? 1:0, 0));

    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_tx,
                                  bcmFieldActionOamLmepMdl,
                                  endpoint_p->level, 0));

    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_tx,
             bcmFieldActionOamUpMep,
             endpoint_p->flags & BCM_OAM_ENDPOINT_UP_FACING? 1:0, 0));
    
    if (endpoint_p->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT)
    {
        /* Assign LM counter and priority mapping table */
        BCM_IF_ERROR_RETURN
            (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_tx,
                                  bcmFieldActionOamLmBasePtr,
                                  endpoint_p->lm_counter_index, 0));

        BCM_IF_ERROR_RETURN
            (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_tx,
                                  bcmFieldActionOamServicePriMappingPtr,
                                  endpoint_p->pri_map_index, 0));
    }

    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_tx,
             bcmFieldActionOamTx,
             endpoint_p->flags & BCM_OAM_ENDPOINT_UP_FACING ? 1:0, 0));

    if (0 == skip_entry_rx) {

    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_rx,
                                  bcmFieldActionOamLmepEnable,
                                  1, 0));
    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_rx,
            bcmFieldActionOamLmEnable,
            endpoint_p->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT? 1:0, 0));
    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_rx,
            bcmFieldActionOamDmEnable,
            endpoint_p->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT? 1:0, 0));
    
    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_rx,
                                  bcmFieldActionOamLmepMdl,
                                  endpoint_p->level, 0));
    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_rx,
                 bcmFieldActionOamUpMep,
                 endpoint_p->flags & BCM_OAM_ENDPOINT_UP_FACING? 1:0, 0));
    
    if (endpoint_p->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT)
    {
        /* Allocate LM counter and priority mapping table */
        BCM_IF_ERROR_RETURN
            (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_rx,
                                      bcmFieldActionOamLmBasePtr,
                                      endpoint_p->lm_counter_index, 0));
        BCM_IF_ERROR_RETURN
            (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_rx,
                                      bcmFieldActionOamServicePriMappingPtr,
                                      endpoint_p->pri_map_index, 0));
    }
    
    BCM_IF_ERROR_RETURN
        (bcm_esw_field_action_add(unit, endpoint_p->fp_entry_rx,
                 bcmFieldActionOamTx,
                 endpoint_p->flags & BCM_OAM_ENDPOINT_UP_FACING ? 0:1, 0));

    } /* if (0 == skip_entry_rx) */

    if (endpoint_p->vp != 0) {
        oam_info_p->fp_vp_entry_count++;
    } else {
        oam_info_p->fp_glp_entry_count++;
    }

    if ((mep_on_trunk != 0) &&
        !(endpoint_p->flags & BCM_OAM_ENDPOINT_UP_FACING)) {
        /* Create fp entry for each port in the trunk for down mep only. */
        BCM_IF_ERROR_RETURN
            (_bcm_en_oam_trunk_install_fp(unit, add_mdl, endpoint_p));
    }

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return BCM_E_NONE;
}

static void _bcm_en_oam_free_memory(int unit, _bcm_oam_info_t *oam_info_p)
{
    sal_free(oam_info_p->remote_endpoints);
    sal_free(oam_info_p->remote_endpoints_in_use);
    sal_free(oam_info_p->local_rx_endpoints_in_use);
    sal_free(oam_info_p->local_tx_endpoints_in_use);
    sal_free(oam_info_p->lm_counter_in_use);
    sal_free(oam_info_p->endpoints);
    sal_free(oam_info_p->groups);

    oam_info_p->remote_endpoints = NULL;
    oam_info_p->remote_endpoints_in_use  = NULL;
    oam_info_p->local_rx_endpoints_in_use = NULL;
    oam_info_p->local_tx_endpoints_in_use = NULL;
    oam_info_p->lm_counter_in_use = NULL;
    oam_info_p->endpoints = NULL;
    oam_info_p->groups = NULL;

#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
    if (SOC_IS_KATANAX(unit)) {
        soc_cm_sfree(oam_info_p->unit, oam_info_p->dma_buffer);
        soc_cm_sfree(oam_info_p->unit, oam_info_p->dmabuf_reply); 

        oam_info_p->dma_buffer = NULL;
        oam_info_p->dmabuf_reply = NULL; 
    }
#endif
}

static void _bcm_en_oam_event_unregister_all(_bcm_oam_info_t *oam_info_p)
{
    _bcm_oam_event_handler_t *event_handler_p;
    _bcm_oam_event_handler_t *event_handler_to_delete_p;

    event_handler_p = oam_info_p->event_handler_list_p;

    while (event_handler_p != NULL)
    {
        event_handler_to_delete_p = event_handler_p;
        event_handler_p = event_handler_p->next_p;

        sal_free(event_handler_to_delete_p);
    }
    oam_info_p->event_handler_list_p = NULL;
}

/*
 * Function:
 *      bcm_en_oam_init
 * Purpose:
 *      Initialize the OAM subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_init(
    int unit)
{
    _bcm_oam_info_t *oam_info_p;
    bcm_oam_endpoint_t endpoint_index;
    bcm_port_t port_index;
    uint32 register_value, temp_index;
    soc_mem_t mem;
    int entry_words;
    uint32 mem_entries[BCM_OAM_INTPRI_MAX];
    void *entries[1];
    int result, i;
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
    int uc;
    uint8 carrier_code[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    char *carrier_code_str;
    uint32 node_id = 0;
    shr_bhh_msg_ctrl_init_t msg_init;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int priority;
    sal_usecs_t timeout = 0;
#endif

    if (soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
    if (SOC_IS_KATANAX(unit)) {
	
	if(SOC_E_NONE != soc_cmic_uc_msg_active_wait(unit, 0))
	{
	    LOG_ERROR(BSL_LS_BCM_OAM,
                      (BSL_META_U(unit,
                                  "uKernel Not Ready, bhh not started\n")));
	    return BCM_E_NONE;
	}
    }
#endif
    }

    SET_OAM_INFO;

    if (oam_info_p->initialized)
    {
        /* Reset everything */
#ifdef BCM_KATANA_SUPPORT
        if (SOC_IS_KATANA(unit)) {
            soc_kt_oam_handler_register(unit, NULL);
        } else
#endif /* BCM_KATANA_SUPPORT */
        {
            soc_enduro_oam_handler_register(unit, NULL);
        }
        _bcm_en_oam_event_unregister_all(oam_info_p);

        BCM_IF_ERROR_RETURN(bcm_en_oam_group_destroy_all(unit));
        _bcm_en_oam_free_memory(unit, oam_info_p);
        if (ing_pri_map_profile[unit]) {
            SOC_IF_ERROR_RETURN(
                soc_profile_mem_destroy(unit, ing_pri_map_profile[unit]));
            sal_free(ing_pri_map_profile[unit]);
            ing_pri_map_profile[unit] = NULL;
        }
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANAX(unit)) {
            if (oam_opcode_profile[unit]) {
                SOC_IF_ERROR_RETURN(
		    soc_profile_mem_destroy(unit, oam_opcode_profile[unit]));
                sal_free(oam_opcode_profile[unit]);
                oam_opcode_profile[unit] = NULL;
            }
        }
#endif /* BCM_KATANA_SUPPORT */
    }

    oam_info_p->group_count = soc_mem_index_count(unit, MA_STATEm);

    oam_info_p->groups =
        _bcm_en_oam_alloc_clear(oam_info_p->group_count *
				sizeof(_bcm_oam_group_t), "_bcm_oam_group");

    if (oam_info_p->groups == NULL)
    {
        return BCM_E_MEMORY;
    }

    oam_info_p->remote_endpoint_count = soc_mem_index_count(unit, RMEPm);
    oam_info_p->local_tx_endpoint_count = soc_mem_index_count(unit, LMEPm);

    oam_info_p->local_rx_endpoint_count =
        soc_mem_index_count(unit, MA_INDEXm);

    oam_info_p->endpoint_count = oam_info_p->remote_endpoint_count +
        oam_info_p->local_tx_endpoint_count + oam_info_p->local_rx_endpoint_count;

    if (soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
    oam_info_p->unit = unit;

	/* Get SOC properties for BHH */
	oam_info_p->bhh_endpoint_count =
	    soc_property_get(unit, spn_BHH_NUM_SESSIONS, 256);

	carrier_code_str =
	    soc_property_get_str(unit, spn_BHH_CARRIER_CODE);
	if (carrier_code_str != NULL) {
	    /*
	     * Note that the carrier code is specified in colon separated
	     * MAC address format.
	     */
	    if (_shr_parse_macaddr(carrier_code_str, carrier_code) < 0)
		return BCM_E_INTERNAL;
	} 	    

	node_id =
	    soc_property_get(unit, spn_BHH_NODE_ID, 0);

    oam_info_p->endpoint_count += oam_info_p->bhh_endpoint_count;
#endif
    }

    oam_info_p->endpoints =
        _bcm_en_oam_alloc_clear(oam_info_p->endpoint_count *
				sizeof(_bcm_oam_endpoint_t), "_bcm_oam_endpoint");

    if (oam_info_p->endpoints == NULL)
    {
        _bcm_en_oam_free_memory(unit, oam_info_p);
        return BCM_E_MEMORY;
    }

    oam_info_p->local_tx_endpoints_in_use =
        _bcm_en_oam_alloc_clear(SHR_BITALLOCSIZE(oam_info_p->local_tx_endpoint_count),
				"local_tx_endpoints_in_use");

    if (oam_info_p->local_tx_endpoints_in_use == NULL)
    {
        _bcm_en_oam_free_memory(unit, oam_info_p);
        return BCM_E_MEMORY;
    }
        
    /* 
     * Both TX and RX for one MEP 
     */
    oam_info_p->lm_counter_count = 
        soc_mem_index_count(unit, OAM_LM_COUNTERSm) / 2;

    oam_info_p->lm_counter_in_use =
        _bcm_en_oam_alloc_clear(SHR_BITALLOCSIZE(oam_info_p->lm_counter_count),
				"lm_counter_in_use");

    if (oam_info_p->lm_counter_in_use == NULL)
    {
        _bcm_en_oam_free_memory(unit, oam_info_p);
        return BCM_E_MEMORY;
    }

    oam_info_p->local_rx_endpoints_in_use =
        _bcm_en_oam_alloc_clear(SHR_BITALLOCSIZE(oam_info_p->local_rx_endpoint_count),
				"local_rx_endpoints_in_use");

    if (oam_info_p->local_rx_endpoints_in_use == NULL)
    {
        _bcm_en_oam_free_memory(unit, oam_info_p);
        return BCM_E_MEMORY;
    }

    for (endpoint_index = 0; endpoint_index < oam_info_p->endpoint_count;
	 ++endpoint_index)
    {
        oam_info_p->endpoints[endpoint_index].lm_counter_index =
            BCM_OAM_LM_COUNTER_INDEX_INVALID;
    }

    oam_info_p->remote_endpoints_in_use =
        _bcm_en_oam_alloc_clear(SHR_BITALLOCSIZE(oam_info_p->remote_endpoint_count),
				"remote_endpoints_in_use");

    if (oam_info_p->remote_endpoints_in_use == NULL)
    {
        _bcm_en_oam_free_memory(unit, oam_info_p);
        return BCM_E_MEMORY;
    }

    oam_info_p->remote_endpoints =
        sal_alloc(oam_info_p->remote_endpoint_count * sizeof(bcm_oam_endpoint_t),
		  "rmep reverse lookup");

    if (oam_info_p->remote_endpoints == NULL)
    {
        _bcm_en_oam_free_memory(unit, oam_info_p);
        return BCM_E_MEMORY;
    }

    for (endpoint_index = 0; endpoint_index < oam_info_p->remote_endpoint_count;
	 ++endpoint_index)
    {
        oam_info_p->remote_endpoints[endpoint_index] = BCM_OAM_ENDPOINT_INVALID;
    }
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        soc_kt_oam_handler_register(unit, _bcm_en_oam_handle_interrupt);
    } else
#endif /* BCM_KATANA_SUPPORT */
    {
        soc_enduro_oam_handler_register(unit, _bcm_en_oam_handle_interrupt);
    }
#ifdef BCM_WARM_BOOT_SUPPORT
    if (!SOC_WARM_BOOT(unit)) {
        bcm_en_oam_scache_alloc(unit);
    }
    if (SOC_WARM_BOOT(unit))
    {
        _bcm_en_oam_warm_boot(unit);
    }
    else
#endif /* BCM_WARM_BOOT_SUPPORT */
    {
        /* Enable OAM message reception on all ports */
        PBMP_ALL_ITER(unit, port_index)
        {
            bcm_esw_port_control_set(unit, port_index,
				     bcmPortControlOAMEnable, 1);
        }

        /* Enable CCM reception timeouts */
        register_value = 0;

        soc_reg_field_set(unit, OAM_TIMER_CONTROLr, &register_value,
			  TIMER_ENABLEf, 1);

        soc_reg_field_set(unit, OAM_TIMER_CONTROLr, &register_value,
			  CLK_GRANf, 1);

        result = WRITE_OAM_TIMER_CONTROLr(unit, register_value);
        if (SOC_FAILURE(result))
        {
            _bcm_en_oam_free_memory(unit, oam_info_p);
            return result;
        }

        if (!SOC_IS_KATANA(unit)) 
        {
            /* Common information for outgoing CCM packets */
            result = WRITE_LMEP_COMMON_1r(unit, _BCM_OAM_MAC_DA_UPPER_32);
            if (SOC_FAILURE(result))
            {
                _bcm_en_oam_free_memory(unit, oam_info_p);
                return result;
            }
        }

        /* Enable CCM transmission */
        register_value = 0;

        soc_reg_field_set(unit, OAM_TX_CONTROLr, &register_value,
			  TX_ENABLEf, 1);
        soc_reg_field_set(unit, OAM_TX_CONTROLr, &register_value,
			  CMIC_BUF_ENABLEf, 1);

        result = WRITE_OAM_TX_CONTROLr(unit, register_value);
        if (SOC_FAILURE(result)) 
        {
            _bcm_en_oam_free_memory(unit, oam_info_p);
            return result;
        }
            
        register_value = 0;
        soc_reg_field_set(unit, LMEP_COMMON_2r, &register_value,
			  DA_15_3f, _BCM_OAM_MAC_DA_LOWER_13 >> 3);
        soc_reg_field_set(unit, LMEP_COMMON_2r, &register_value,
			  L3f, 1);

        result = WRITE_LMEP_COMMON_2r(unit, register_value);
        if (SOC_FAILURE(result))
        {
            _bcm_en_oam_free_memory(unit, oam_info_p);
            return result;
        }
    }
        
    oam_info_p->vfp_entry_count = 0;
        
    BCM_FIELD_QSET_INIT(oam_info_p->vfp_qs);
    BCM_FIELD_QSET_ADD(oam_info_p->vfp_qs, bcmFieldQualifyDstMac);
    BCM_FIELD_QSET_ADD(oam_info_p->vfp_qs, bcmFieldQualifySrcMac);
    BCM_FIELD_QSET_ADD(oam_info_p->vfp_qs, bcmFieldQualifyOuterVlanId);
    BCM_FIELD_QSET_ADD(oam_info_p->vfp_qs, bcmFieldQualifyStageLookup);
    
    oam_info_p->fp_vp_entry_count = 0;
    oam_info_p->fp_glp_entry_count = 0;

    BCM_FIELD_QSET_INIT(oam_info_p->fp_vp_qs);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_vp_qs, bcmFieldQualifyOuterVlanId);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_vp_qs, bcmFieldQualifyDstMimGport);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_vp_qs, bcmFieldQualifySrcMimGport);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_vp_qs, bcmFieldQualifyStageIngress);
        
    BCM_FIELD_QSET_INIT(oam_info_p->fp_glp_qs);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_glp_qs, bcmFieldQualifyOuterVlanId);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_glp_qs, bcmFieldQualifyDstPort);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_glp_qs, bcmFieldQualifyDstTrunk);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_glp_qs, bcmFieldQualifySrcPort);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_glp_qs, bcmFieldQualifySrcTrunk);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_glp_qs, bcmFieldQualifyEtherType);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_glp_qs, bcmFieldQualifyDstIp6);
    BCM_FIELD_QSET_ADD(oam_info_p->fp_glp_qs, bcmFieldQualifyStageIngress);

    /* 
     * Enable ingress filter for CPU port so LM/DM packets sent from CPU
     * can be processed.
     */
    bcm_esw_port_control_set(unit, CMIC_PORT(unit), 
			     bcmPortControlFilterIngress, 1);

    /* Initialize the ING_VLAN_TAG_ACTION_PROFILE table */
    if (ing_pri_map_profile[unit] == NULL) 
    {
        ing_pri_map_profile[unit] = 
            sal_alloc(sizeof(soc_profile_mem_t), "Ing Pri Map Profile Mem");
        if (ing_pri_map_profile[unit] == NULL) 
        {
            _bcm_en_oam_free_memory(unit, oam_info_p);
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(ing_pri_map_profile[unit]);
    }

    /* Create profile table cache (or re-init if it already exists) */
    mem = ING_SERVICE_PRI_MAPm;
    entry_words = sizeof(ing_service_pri_map_entry_t) / sizeof(uint32);
    SOC_IF_ERROR_RETURN(soc_profile_mem_create(unit, &mem, &entry_words, 1,
					       ing_pri_map_profile[unit]));

    /* Initialize the ING_SERVICE_PRI_MAP_PROFILE_DEFAULT. All priorities
     * Map to zero.
     */
    for (i = 0; i < BCM_OAM_INTPRI_MAX; i++) 
    {
        mem_entries[i] = 0;
        if (SOC_MEM_FIELD_VALID(unit, mem, OFFSET_VALIDf)) {
            soc_mem_field32_set(unit, mem, &mem_entries[i], OFFSET_VALIDf, 1);
        }
    }
    entries[0] = &mem_entries;
    SOC_IF_ERROR_RETURN(soc_profile_mem_add(unit, ing_pri_map_profile[unit],
					    (void *) &entries, BCM_OAM_INTPRI_MAX, &temp_index));

#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANAX(unit)) {
        /* Initialize the OAM_OPCODE_PROFILE table */
        if (oam_opcode_profile[unit] == NULL) {
            oam_opcode_profile[unit] =
		sal_alloc(sizeof(soc_profile_mem_t), "OAM Opcode profile Mem");
            if (oam_opcode_profile[unit] == NULL) {
                _bcm_en_oam_free_memory(unit, oam_info_p);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(oam_opcode_profile[unit]);
        }

        /* Create profile table cache (or re-init if it already exists) */
        mem = OAM_OPCODE_CONTROL_PROFILEm;
        entry_words = sizeof(oam_opcode_control_profile_entry_t) / sizeof(uint32);
        SOC_IF_ERROR_RETURN(soc_profile_mem_create(unit, &mem, &entry_words,
                                                   1,
                                                   oam_opcode_profile[unit]));
    }
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit))
    {
        /* Recover FP groups and entries associated with OAM */
        oam_info_p->vfp_group = -1;
        oam_info_p->fp_vp_group = -1;
        oam_info_p->fp_glp_group = -1;
        BCM_IF_ERROR_RETURN(bcm_esw_field_group_traverse(unit, 
							 _bcm_en_oam_field_group_process, NULL));
    }
#endif 

    /*
     * BHH init
     */
    if (soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
    if (SOC_IS_KATANAX(unit)) {
	if (oam_info_p->initialized)
	{
	    /* 
	     * Event Handler thread exit signal 
	     */
        if ( oam_info_p->event_thread_id ) {
	        oam_info_p->event_thread_kill = 1;
	        soc_cmic_uc_msg_receive_cancel(unit, oam_info_p->uc_num,
					   MOS_MSG_CLASS_BHH_EVENT);
        }

	    /*
	     * Send BHH Uninit message to uC
	     * Ignore error since that may indicate uKernel was reloaded.
	     */
	    result = _bcm_en_oam_bhh_msg_send_receive(unit,
						  MOS_MSG_SUBCLASS_BHH_UNINIT,
						  0, 0,
						  MOS_MSG_SUBCLASS_BHH_UNINIT_REPLY,
						  &reply_len);
	    if (BCM_SUCCESS(result) && (reply_len != 0)) {
		    return BCM_E_INTERNAL;
	    }

	    /* 
	     * Delete CPU COS queue mapping entries for BHH packets 
	     */
	    if (oam_info_p->cpu_cosq_ach_error_index >= 0) {
		BCM_IF_ERROR_RETURN
		    (bcm_esw_rx_cosq_mapping_delete(unit,
						    oam_info_p->cpu_cosq_ach_error_index));
		oam_info_p->cpu_cosq_ach_error_index = -1;
	    }
	    if (oam_info_p->cpu_cosq_invalid_error_index >= 0) {
		BCM_IF_ERROR_RETURN
		    (bcm_esw_rx_cosq_mapping_delete(unit,
						    oam_info_p->cpu_cosq_invalid_error_index));
		oam_info_p->cpu_cosq_invalid_error_index = -1;
	    }
	} else {
	    oam_info_p->cpu_cosq_ach_error_index = -1;
	    oam_info_p->cpu_cosq_invalid_error_index = -1;
	}

	/*
	 * Initialize HOST side
	 */
	oam_info_p->cpu_cosq =
	    soc_property_get(unit, spn_BHH_COSQ, 42);

	/*
	 * Allocate DMA buffers
	 *
	 * DMA buffer will be used to send and receive 'long' messages
	 * between SDK Host and uController (BTE).
	 */
	oam_info_p->dma_buffer_len = sizeof(shr_bhh_msg_ctrl_t);
	oam_info_p->dma_buffer = soc_cm_salloc(unit, oam_info_p->dma_buffer_len,
					       "BHH DMA buffer");
	if (!oam_info_p->dma_buffer) {
	    _bcm_en_oam_free_memory(unit, oam_info_p);
	    return BCM_E_MEMORY;
	}
	sal_memset(oam_info_p->dma_buffer, 0, oam_info_p->dma_buffer_len);

	oam_info_p->dmabuf_reply = soc_cm_salloc(unit, sizeof(shr_bhh_msg_ctrl_t),
						 "BHH uC reply");
	if (!oam_info_p->dmabuf_reply) {
	    _bcm_en_oam_free_memory(unit, oam_info_p);
	    return BCM_E_MEMORY;
	}
	sal_memset(oam_info_p->dmabuf_reply, 0, sizeof(shr_bhh_msg_ctrl_t));


	/*
	 * Initialize uController side
	 */

	/*
	 * Start BHH application in BTE (Broadcom Task Engine) uController.
	 * Determine which uController is running BHH  by choosing the first
	 * uC that returns successfully.
	 */
	for (uc = 0; uc < CMICM_NUM_UCS; uc++) {
	    result = soc_cmic_uc_appl_init(unit, uc, MOS_MSG_CLASS_BHH,
					   _BHH_UC_MSG_TIMEOUT_USECS,
					   BHH_SDK_VERSION,
					   BHH_UC_MIN_VERSION);
	    if (SOC_E_NONE == result) {
		    /* BHH started successfully */
		    oam_info_p->uc_num = uc;
		    break;
	    }
	}

	if (uc >= CMICM_NUM_UCS) {  /* Could not find or start BHH appl */
	    _bcm_en_oam_free_memory(unit, oam_info_p);
	    return BCM_E_INTERNAL;
	}

	/* RX DMA channel (0..3) local to the uC */
	oam_info_p->rx_channel = BCM_KT_BHH_RX_CHANNEL;


	/* Set control message data */
	sal_memset(&msg_init, 0, sizeof(msg_init));
	msg_init.num_sessions       = oam_info_p->bhh_endpoint_count;
	msg_init.rx_channel         = oam_info_p->rx_channel;
	msg_init.node_id            = node_id; 
	sal_memcpy(msg_init.carrier_code, carrier_code, SHR_BHH_CARRIER_CODE_LEN); 

	/* Pack control message data into DMA buffer */
	buffer     = oam_info_p->dma_buffer;
	buffer_ptr = shr_bhh_msg_ctrl_init_pack(buffer, &msg_init);
	buffer_len = buffer_ptr - buffer;

	/* Send BHH Init message to uC */
	result = _bcm_en_oam_bhh_msg_send_receive(unit,
						  MOS_MSG_SUBCLASS_BHH_INIT,
						  buffer_len, 0,
						  MOS_MSG_SUBCLASS_BHH_INIT_REPLY,
						  &reply_len);

	if (BCM_FAILURE(result) || (reply_len != 0)) {
	    _bcm_en_oam_free_memory(unit, oam_info_p);
	    return BCM_E_INTERNAL;
	}

	/*
	 * Initialize HW
	 */
	result = _bcm_en_oam_bhh_hw_init(unit);
	if (BCM_FAILURE(result)) {
	    BCM_IF_ERROR_RETURN(bcm_en_oam_detach(unit));
	    return result;
	}

	/*
	 * Start event message callback thread
	 */
	priority = soc_property_get(unit, spn_BHH_THREAD_PRI, BHH_THREAD_PRI_DFLT);

    timeout = sal_time_usecs() + 5000000;
    while (1) {
        if (oam_info_p->event_thread_id == NULL) {
            /*Either we are running afresh or BHH Event thread successfully exited
             *Create new BHH event thread.*/
            if (sal_thread_create("bcmBHH", SAL_THREAD_STKSZ,
                      priority,
                      _bcm_en_oam_bhh_callback_thread,
                      (void*)oam_info_p) == SAL_THREAD_ERROR) {
                BCM_IF_ERROR_RETURN(bcm_en_oam_detach(unit));
                return BCM_E_MEMORY;
            }
            break;
        } else {
            if (sal_time_usecs() < timeout){
                /*Give some time to already running BHH thread to schedule and exit*/
                sal_usleep(1000);
            } else {
                /*Timeout*/
                LOG_ERROR(BSL_LS_BCM_OAM,
                          (BSL_META_U(unit,
                                      "BHH event thread not exited.\n")));
                BCM_IF_ERROR_RETURN(bcm_en_oam_detach(unit));
                return BCM_E_INTERNAL;
            }
        }
    }

	/*
	 * End BHH init
	 */
    }
#endif
    }

    oam_info_p->initialized = 1;

    return BCM_E_NONE;
}

#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_BHH)

/*
 * Function:
 *      bcm_en_oam_bhh_hw_init
 * Purpose:
 *      Initialize the HW for BHH packet processing.
 *      Configure:
 *      - Copy to CPU BHH error packets
 *      - CPU COS Queue for BHH packets
 *      - RX DMA channel
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
_bcm_en_oam_bhh_hw_init(int unit)
{
    int rv = BCM_E_NONE;
     _bcm_oam_info_t *oam_info_p;
    uint32 val; 
    int index;
    int ach_error_index;
    int invalid_error_index;
    int cosq_map_size;
    bcm_rx_reasons_t reasons, reasons_mask;
    uint8 int_prio, int_prio_mask;
    uint32 packet_type, packet_type_mask;
    bcm_cos_queue_t cosq;
    bcm_rx_chan_t chan_id;

    SET_OAM_INFO;

    /*
     * Send BHH error packet to CPU
     *
     * Configure CPU_CONTROL_0 register
     */
    BCM_IF_ERROR_RETURN(READ_CPU_CONTROL_0r(unit, &val));
    soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                      BFD_UNKNOWN_VERSION_TOCPUf, 1);
    soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                      BFD_YOUR_DISCRIMINATOR_NOT_FOUND_TOCPUf, 1);
    soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                      BFD_UNKNOWN_CONTROL_PACKET_TOCPUf, 1);
    soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                      BFD_UNKNOWN_ACH_CHANNEL_TYPE_TOCPUf, 1);
    soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                      BFD_UNKNOWN_ACH_VERSION_TOCPUf, 1);
    BCM_IF_ERROR_RETURN(WRITE_CPU_CONTROL_0r(unit, val));

    /*
     * Set BFD ACH Channel Types
     */
#if 0
    BCM_IF_ERROR_RETURN(READ_BFD_RX_ACH_TYPE_MPLSTPr(unit, &val));
    soc_reg_field_set(unit, BFD_RX_ACH_TYPE_MPLSTPr, &val,
                      BFD_ACH_TYPE_MPLSTP_CVf,
                      SHR_BHH_ACH_CHANNEL_TYPE);
    BCM_IF_ERROR_RETURN(WRITE_BFD_RX_ACH_TYPE_MPLSTPr(unit, val));
#endif

    /*
     * Direct BHH packets to designated CPU COS Queue...or more accurately
     * BFD error packets.
     *
     * Reasons:
     *   - bcmRxReasonBFD:               BFD error
     *   - bcmRxReasonBfdUnknownVersion: BFD Unknown ACH
     * NOTE:
     *     The user input 'cpu_qid' (bcm_BHH_endpoint_t) could be
     *     used to select different CPU COS queue. Currently,
     *     all priorities are mapped into the same CPU COS Queue.
     */

    /* Find available entries in CPU COS queue map table */
    ach_error_index    = -1;   /* COSQ map index for error packets */
    invalid_error_index    = -1;   /* COSQ map index for error packets */
    BCM_IF_ERROR_RETURN
        (bcm_esw_rx_cosq_mapping_size_get(unit, &cosq_map_size));
    
    for (index = 0; index < cosq_map_size; index++) {
        rv = bcm_esw_rx_cosq_mapping_get(unit, index,
                                         &reasons, &reasons_mask,
                                         &int_prio, &int_prio_mask,
                                         &packet_type, &packet_type_mask,
                                         &cosq);
        if (rv == BCM_E_NOT_FOUND) {
            /* Assign first available index */
            rv = BCM_E_NONE;
            if (ach_error_index == -1) {
                ach_error_index = index;
            } else if (invalid_error_index == -1) {
                invalid_error_index = index;
                break;
            }
        }
        if (BCM_FAILURE(rv)) {
            return rv;
        }
    }

    if (ach_error_index == -1 || invalid_error_index == -1) {
        return BCM_E_FULL;
    }

    /* Set CPU COS Queue mapping */
    BCM_RX_REASON_CLEAR_ALL(reasons);
    BCM_RX_REASON_SET(reasons, bcmRxReasonBfd);  /* BFD Error */
    BCM_RX_REASON_SET(reasons, bcmRxReasonBfdUnknownVersion); /* Despite the name this reason */
                                                              /* code covers Unknown ACH */

    BCM_IF_ERROR_RETURN
        (bcm_esw_rx_cosq_mapping_set(unit, ach_error_index,
                                     reasons, reasons,
                                     0, 0, /* Any priority */
                                     0, 0, /* Any packet type */
                                     oam_info_p->cpu_cosq));
    oam_info_p->cpu_cosq_ach_error_index = ach_error_index;


    BCM_RX_REASON_CLEAR_ALL(reasons);
    BCM_RX_REASON_SET(reasons, bcmRxReasonBfd);  /* BFD Error */
    BCM_RX_REASON_SET(reasons, bcmRxReasonBfdInvalidPacket); 

    BCM_IF_ERROR_RETURN
        (bcm_esw_rx_cosq_mapping_set(unit, invalid_error_index,
                                     reasons, reasons,
                                     0, 0, /* Any priority */
                                     0, 0, /* Any packet type */
                                     oam_info_p->cpu_cosq));
    oam_info_p->cpu_cosq_invalid_error_index = invalid_error_index;


    /*
     * Assign RX DMA channel to CPU COS Queue
     * (This is the RX channel to listen on for BHH packets).
     *
     * DMA channels (12) are assigned 4 per processor:
     * (see /src/bcm/common/rx.c)
     *   channels 0..3  --> PCI host
     *   channels 4..7  --> uController 0
     *   chnanels 8..11 --> uController 1
     *
     * The uControllers designate the 4 local DMA channels as follows:
     *   local channel  0     --> TX
     *   local channel  1..3  --> RX
     *
     * Each uController application needs to use a different
     * RX DMA channel to listen on.
     */
    chan_id = (BCM_RX_CHANNELS * (SOC_ARM_CMC(unit, oam_info_p->uc_num))) +
        oam_info_p->rx_channel;

    rv = _bcm_common_rx_queue_channel_set(unit, oam_info_p->cpu_cosq, chan_id);
    if (rv != BCM_E_NONE ) {
       LOG_ERROR(BSL_LS_BCM_COMMON,
                 (BSL_META_U(unit,
                             "BHH(unit %d) Error: No BHH COS Queue available from uC%d - %s\n"),
                  unit, oam_info_p->uc_num, bcm_errmsg(BCM_E_CONFIG))); 
    }

    return rv;
}


/*
 * Function:
 *      _bcm_en_oam_bhh_find_l2x_entry
 */
STATIC int
_bcm_en_oam_bhh_find_l2x_entry(int unit, uint32 key, int key_type, int ses_type,
			       int *index, l2x_entry_t *l2_entry)
{
    l2x_entry_t l2_key;

    sal_memset(&l2_key, 0, sizeof(l2_key));

    soc_mem_field32_set(unit, L2Xm, &l2_key, KEY_TYPEf, key_type);
    soc_mem_field32_set(unit, L2Xm, &l2_key,
                        SESSION_IDENTIFIER_TYPEf, ses_type);

    if (ses_type) {
        soc_mem_field32_set(unit, L2Xm, &l2_key, LABELf, key);
    } else {
        soc_mem_field32_set(unit, L2Xm, &l2_key,
                            YOUR_DISCRIMINATORf, key);
    }

    return soc_mem_search(unit, L2Xm, MEM_BLOCK_ANY, index,
                          &l2_key, l2_entry, 0);

}

/*
 * Function:
 *      _bcm_en_oam_bhh_session_hw_delete
 * Purpose:
 *      Delete BHH Session in HW device.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      endpoint_p - (IN) Pointer to BHH endpoint structure.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
_bcm_en_oam_bhh_session_hw_delete(int unit, _bcm_oam_endpoint_t *endpoint_p)
{
    int rv = BCM_E_NONE;
    int l2_index = 0;
    l2x_entry_t l2_entry;
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
    mpls_entry_entry_t mpls_entry;
    mpls_entry_entry_t mpls_key;
    int mpls_index = 0;
#endif /* BCM_TRIUMPH_SUPPORT &&  BCM_MPLS_SUPPORT */


    switch(endpoint_p->type) {
    case bcmOAMEndpointTypeBHHMPLS:
    case bcmOAMEndpointTypeBHHMPLSVccv:
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
        SOC_IF_ERROR_RETURN(bcm_tr_mpls_lock (unit));

        sal_memset(&mpls_key, 0, sizeof(mpls_key));
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_key,
                                    MPLS_LABELf, endpoint_p->label);

        SOC_IF_ERROR_RETURN(soc_mem_search(unit, MPLS_ENTRYm,
                                           MEM_BLOCK_ANY, &mpls_index,
                                           &mpls_key, &mpls_entry, 0));

        if ((soc_MPLS_ENTRYm_field32_get(unit,
                                         &mpls_entry, VALIDf) != 0x1)) {
            bcm_tr_mpls_unlock (unit);
            return BCM_E_PARAM;
        }

        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry, BFD_ENABLEf, 0);

        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry, PW_CC_TYPEf, 0);

        SOC_IF_ERROR_RETURN(soc_mem_write(unit, MPLS_ENTRYm,
                                          MEM_BLOCK_ANY,
                                          mpls_index, &mpls_entry));

        bcm_tr_mpls_unlock (unit);

        if (BCM_SUCCESS(_bcm_en_oam_bhh_find_l2x_entry(unit,
						       endpoint_p->label,
						       4, 1,
						       &l2_index, &l2_entry))) {
            soc_mem_delete_index(unit, L2Xm, MEM_BLOCK_ANY, l2_index);

        } else {
            return BCM_E_PARAM;
        }
#endif /* BCM_TRIUMPH_SUPPORT &&  BCM_MPLS_SUPPORT */
        break;

    default:
        break;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_en_oam_bhh_msg_send_receive
 * Purpose:
 *      Sends given BHH control message to the uController.
 *      Receives and verifies expected reply.
 *      Performs DMA operation if required.
 * Parameters:
 *      unit        - (IN) Unit number.
 *      s_subclass  - (IN) BHH message subclass.
 *      s_len       - (IN) Value for 'len' field in message struct.
 *                         Length of buffer to flush if DMA send is required.
 *      s_data      - (IN) Value for 'data' field in message struct.
 *                         Ignored if message requires a DMA send/receive
 *                         operation.
 *      r_subclass  - (IN) Expected reply message subclass.
 *      r_len       - (OUT) Returns value in 'len' reply message field.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *     - The uc_msg 'len' and 'data' fields of mos_msg_data_t
 *       can take any arbitrary data.
 *
 *     BHH Long Control message:
 *     - BHH control messages that require send/receive of information
 *       that cannot fit in the uc_msg 'len' and 'data' fields need to
 *       use DMA operations to exchange information (long control message).
 *
 *     - BHH convention for long control messages for
 *        'mos_msg_data_t' fields:
 *          'len'    size of the DMA buffer to send to uController
 *          'data'   physical DMA memory address to send or receive
 *
 *      DMA Operations:
 *      - DMA read/write operation is performed when a long BHH control
 *        message is involved.
 *
 *      - Messages that require DMA operation (long control message)
 *        is indicated by MOS_MSG_DMA_MSG().
 *
 *      - Callers must 'pack' and 'unpack' corresponding information
 *        into/from DMA buffer indicated by BHH_INFO(unit)->dma_buffer.
 *
 */
STATIC int
_bcm_en_oam_bhh_msg_send_receive(int unit, uint8 s_subclass,
                             uint16 s_len, uint32 s_data,
                             uint8 r_subclass, uint16 *r_len)
{
    int rv = BCM_E_NONE;
     _bcm_oam_info_t *oam_info_p;
    mos_msg_data_t send, reply;
    uint8 *dma_buffer;
    int dma_buffer_len;
    uint32 uc_rv;

    SET_OAM_INFO;

    sal_memset(&send, 0, sizeof(send));
    sal_memset(&reply, 0, sizeof(reply));
    send.s.mclass = MOS_MSG_CLASS_BHH;
    send.s.subclass = s_subclass;
    send.s.len = bcm_htons(s_len);

    /*
     * Set 'data' to DMA buffer address if a DMA operation is
     * required for send or receive.
     */
    dma_buffer = oam_info_p->dma_buffer;
    dma_buffer_len = oam_info_p->dma_buffer_len;
    if (MOS_MSG_DMA_MSG(s_subclass) ||
        MOS_MSG_DMA_MSG(r_subclass)) {
        send.s.data = bcm_htonl(soc_cm_l2p(unit, dma_buffer));
    } else {
        send.s.data = bcm_htonl(s_data);
    }

    /* Flush DMA memory */
    if (MOS_MSG_DMA_MSG(s_subclass)) {
        soc_cm_sflush(unit, dma_buffer, s_len);
    }

    /* Invalidate DMA memory to read */
    if (MOS_MSG_DMA_MSG(r_subclass)) {
        soc_cm_sinval(unit, dma_buffer, dma_buffer_len);
    }

    rv = soc_cmic_uc_msg_send_receive(unit, oam_info_p->uc_num,
                                      &send, &reply,
                                      _BHH_UC_MSG_TIMEOUT_USECS);

    /* Check reply class, subclass */
    if ((rv != SOC_E_NONE) ||
        (reply.s.mclass != MOS_MSG_CLASS_BHH) ||
        (reply.s.subclass != r_subclass)) {
        return BCM_E_INTERNAL;
    }

    /* Convert BHH uController error code to BCM */
    uc_rv = bcm_ntohl(reply.s.data);
    switch(uc_rv) {
    case SHR_BHH_UC_E_NONE:
        rv = BCM_E_NONE;
        break;
    case SHR_BHH_UC_E_INTERNAL:
        rv = BCM_E_INTERNAL;
        break;
    case SHR_BHH_UC_E_MEMORY:
        rv = BCM_E_MEMORY;
        break;
    case SHR_BHH_UC_E_PARAM:
        rv = BCM_E_PARAM;
        break;
    case SHR_BHH_UC_E_RESOURCE:
        rv = BCM_E_RESOURCE;
        break;
    case SHR_BHH_UC_E_EXISTS:
        rv = BCM_E_EXISTS;
        break;
    case SHR_BHH_UC_E_NOT_FOUND:
        rv = BCM_E_NOT_FOUND;
        break;
    case SHR_BHH_UC_E_INIT:
        rv = BCM_E_INIT;
        break;
    default:
        rv = BCM_E_INTERNAL;
        break;
    }
        
    *r_len = bcm_ntohs(reply.s.len);

    return rv;
}

STATIC uint8 *
_bcm_en_oam_bhh_ach_header_pack(uint8 *buffer, _ach_header_t *ach)
{
    uint32  tmp;

    tmp = ((ach->f_nibble & 0xf) << 28) | ((ach->version & 0xf) << 24) |
        (ach->reserved << 16) | ach->channel_type;

    _BHH_ENCAP_PACK_U32(buffer, tmp);

    return buffer;
}

STATIC uint8 *
_bcm_en_oam_bhh_mpls_label_pack(uint8 *buffer, _mpls_label_t *mpls)
{
    uint32  tmp;

    tmp = ((mpls->label & 0xfffff) << 12) | ((mpls->exp & 0x7) << 9) |
        ((mpls->s & 0x1) << 8) | mpls->ttl;
    _BHH_ENCAP_PACK_U32(buffer, tmp);

    return buffer;
}

STATIC uint8 *
_bcm_en_oam_bhh_l2_header_pack(uint8 *buffer, _l2_header_t *l2)
{
    uint32  tmp;
    int     i;

    for (i = 0; i < _BHH_MAC_ADDR_LENGTH; i++) {
        _BHH_ENCAP_PACK_U8(buffer, l2->dst_mac[i]);
    }

    for (i = 0; i < _BHH_MAC_ADDR_LENGTH; i++) {
        _BHH_ENCAP_PACK_U8(buffer, l2->src_mac[i]);
    }

    /* Vlan Tag */
    tmp = (l2->vlan_tag.tpid << 16) | ((l2->vlan_tag.tci.prio & 0x7) << 13) |
        ((l2->vlan_tag.tci.cfi & 0x1) << 12) | (l2->vlan_tag.tci.vid & 0xfff);
    _BHH_ENCAP_PACK_U32(buffer, tmp);

    _BHH_ENCAP_PACK_U16(buffer, l2->etype);

    return buffer;
}



STATIC int
_bcm_en_oam_bhh_ach_header_get(uint32 packet_flags, _ach_header_t *ach)
{
    sal_memset(ach, 0, sizeof(*ach));

    ach->f_nibble = SHR_BHH_ACH_FIRST_NIBBLE;
    ach->version  = SHR_BHH_ACH_VERSION;
    ach->reserved = 0;

    ach->channel_type = SHR_BHH_ACH_CHANNEL_TYPE;

    return BCM_E_NONE;
}

STATIC int
_bcm_en_oam_bhh_mpls_label_get(uint32 label, uint8 exp, uint8 s, uint8 ttl,
                           _mpls_label_t *mpls)
{
    sal_memset(mpls, 0, sizeof(*mpls));

    mpls->label = label;
    mpls->exp   = exp;
    mpls->s     = s;
    if(ttl)
        mpls->ttl   = ttl;
    else
        mpls->ttl   = _BHH_MPLS_DFLT_TTL;

    return BCM_E_NONE;
}

STATIC int
_bcm_en_oam_bhh_mpls_gal_label_get(_mpls_label_t *mpls)
{
    return _bcm_en_oam_bhh_mpls_label_get(SHR_BHH_MPLS_GAL_LABEL,
                                      0, 1, 1, mpls);
}

STATIC int
_bcm_en_oam_bhh_mpls_router_alert_label_get(_mpls_label_t *mpls)
{
    return _bcm_en_oam_bhh_mpls_label_get(SHR_BHH_MPLS_ROUTER_ALERT_LABEL,
                                      0, 0, 0, mpls);
}

STATIC int
_bcm_en_oam_bhh_mpls_labels_get(int unit, _bcm_oam_endpoint_t *endpoint_p,
                                uint32 packet_flags,
                                int max_count, 
                                _mpls_label_t *pw_label,
                                _mpls_label_t *mpls,
                                int *mpls_count, bcm_if_t *l3_intf_id)
{
    int count = 0;
    bcm_l3_egress_t l3_egress;
    bcm_mpls_port_t mpls_port;
    bcm_mpls_egress_label_t label_array[_BHH_MPLS_MAX_LABELS];
    int label_count;
    int i = 0;

    /* Get L3 objects */
    bcm_l3_egress_t_init(&l3_egress);
    if (BCM_FAILURE(bcm_esw_l3_egress_get(unit, endpoint_p->egress_if, &l3_egress))) {
        return BCM_E_PARAM;
    }

    /* Look for a tunnel associated with this interface */
    if (BCM_SUCCESS
        (bcm_esw_mpls_tunnel_initiator_get(unit, l3_egress.intf,
                                           _BHH_MPLS_MAX_LABELS,
                                           label_array, &label_count))) {
        for (i = 0; (i < label_count) && (count < max_count); i++) {
            _bcm_en_oam_bhh_mpls_label_get(label_array[i].label,
                                       label_array[i].exp,
                                       0,
                                       label_array[i].ttl,
                                       &mpls[count++]);
        }
    }

        /* MPLS Router Alert */
    if (packet_flags & _BHH_ENCAP_PKT_MPLS_ROUTER_ALERT) {
        _bcm_en_oam_bhh_mpls_router_alert_label_get(&mpls[count++]);
    }

    /* Use GPORT to resolve interface */
    if (BCM_GPORT_IS_MPLS_PORT(endpoint_p->gport)) {
        /* Get mpls port and label info */
        bcm_mpls_port_t_init(&mpls_port);
        mpls_port.mpls_port_id = endpoint_p->gport;
        if (BCM_FAILURE
            (bcm_esw_mpls_port_get(unit, endpoint_p->vpn,
                                   &mpls_port))) {
            return BCM_E_PARAM;
        } else {
            if (endpoint_p->type == bcmOAMEndpointTypeBHHMPLSVccv &&
                endpoint_p->vccv_type == bcmOamBhhVccvTtl) {
                mpls_port.egress_label.ttl = 0x1;
            }

            _bcm_en_oam_bhh_mpls_label_get(mpls_port.egress_label.label,
                                       mpls_port.egress_label.exp,
                                       0,
                                       mpls_port.egress_label.ttl,
                                       &mpls[count++]);
        }
    }


    /* Set Bottom of Stack if there is no GAL label */
    if (!(packet_flags & _BHH_ENCAP_PKT_GAL)) {
        mpls[count-1].s = 1;
    }

    *mpls_count = count;

    return BCM_E_NONE;

}

STATIC int
_bcm_en_oam_bhh_l2_header_get(int unit, _bcm_oam_endpoint_t *endpoint_p,
                          bcm_port_t port, uint16 etype,
                          _l2_header_t *l2)
{
    uint16 tpid;
    bcm_l3_egress_t l3_egress;
    bcm_l3_intf_t l3_intf;
    bcm_vlan_t vid;

    sal_memset(l2, 0, sizeof(*l2));

    /* Get L3 interfaces */
    bcm_l3_egress_t_init(&l3_egress);
    bcm_l3_intf_t_init(&l3_intf);

    if (BCM_FAILURE
        (bcm_esw_l3_egress_get(unit, endpoint_p->egress_if, &l3_egress))) {
        return BCM_E_PARAM;
    }

    l3_intf.l3a_intf_id = l3_egress.intf;
    if (BCM_FAILURE(bcm_esw_l3_intf_get(unit, &l3_intf))) {
        return BCM_E_PARAM;
    }

    /* Get TPID */
    if (BCM_FAILURE(bcm_esw_port_tpid_get(unit, port, &tpid))) {
        return BCM_E_INTERNAL;
    }

    sal_memcpy(l2->dst_mac, l3_egress.mac_addr, _BHH_MAC_ADDR_LENGTH);
    sal_memcpy(l2->src_mac, l3_intf.l3a_mac_addr, _BHH_MAC_ADDR_LENGTH);
    l2->vlan_tag.tpid     = tpid;
    l2->vlan_tag.tci.prio = endpoint_p->vlan_pri;
    l2->vlan_tag.tci.cfi  = 0;
    l2->vlan_tag.tci.vid  = l3_intf.l3a_vid;
    if (BCM_SUCCESS(bcm_esw_port_untagged_vlan_get(unit, port, &vid))) {
        if (vid == l2->vlan_tag.tci.vid) {
            l2->vlan_tag.tpid = 0;  /* Set to 0 to indicate untagged */
        }
    }

    l2->etype             = etype;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_en_oam_bhh_encap_build_pack
 * Purpose:
 *      Builds and packs the BHH packet encapsulation for a given
 *      BHH tunnel type.
 * Parameters:
 *      unit            - (IN) Unit number.
 *      port            - (IN) Port.
 *      endpoint_config - (IN/OUT) Pointer to BHH endpoint structure.
 *      packet_flags    - (IN) Flags for building packet.
 *      buffer          - (OUT) Buffer returning BHH encapsulation.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The returning BHH encapsulation includes only all the
 *      encapsulation headers/labels and does not include
 *      the BHH control packet.
 */
STATIC int
_bcm_en_oam_bhh_encap_build_pack(int unit, bcm_port_t port,
                             _bcm_oam_endpoint_t *endpoint_p,
                             uint32 packet_flags,
                             uint8 *buffer,
			     uint32 *encap_length)
{
    uint8          *cur_ptr = buffer;
    uint16         etype = 0;
    bcm_if_t       l3_intf_id = -1;
    _ach_header_t  ach;
    _mpls_label_t  mpls_gal;
    _mpls_label_t  mpls_r_alert;
    _mpls_label_t  mpls_labels[_BHH_MPLS_MAX_LABELS];
    _mpls_label_t  pw_label;
    int            mpls_count = 0;
    _l2_header_t   l2;
    int i;


    /*
     * Get necessary headers/labels information.
     *
     * Following order is important since some headers/labels
     * may depend on previous header/label information.
     */
    if (packet_flags & _BHH_ENCAP_PKT_G_ACH) {
        BCM_IF_ERROR_RETURN
            (_bcm_en_oam_bhh_ach_header_get(packet_flags, &ach));
    }

    if (packet_flags & _BHH_ENCAP_PKT_GAL) {
        BCM_IF_ERROR_RETURN
            (_bcm_en_oam_bhh_mpls_gal_label_get(&mpls_gal));
    }

    if (packet_flags & _BHH_ENCAP_PKT_MPLS_ROUTER_ALERT) {
        BCM_IF_ERROR_RETURN
            (_bcm_en_oam_bhh_mpls_router_alert_label_get(&mpls_r_alert));
    }

    if (packet_flags & _BHH_ENCAP_PKT_MPLS) {
        etype = SHR_BHH_L2_ETYPE_MPLS_UCAST;
        BCM_IF_ERROR_RETURN
            (_bcm_en_oam_bhh_mpls_labels_get(unit, endpoint_p,
                                             packet_flags,    
                                             _BHH_MPLS_MAX_LABELS,
                                             &pw_label,
                                             mpls_labels, 
                                             &mpls_count,
                                             &l3_intf_id));
    }

    /* Always build L2 Header */
    BCM_IF_ERROR_RETURN
        (_bcm_en_oam_bhh_l2_header_get(unit, 
                                       endpoint_p,
                                       port, 
                                       etype, 
                                       &l2));


    /*
     * Pack header/labels into given buffer (network packet format).
     *
     * Following packing order must be followed to correctly
     * build the packet encapsulation.
     */
    cur_ptr = buffer;

    /* L2 Header is always present */
    cur_ptr = _bcm_en_oam_bhh_l2_header_pack(cur_ptr, &l2);

    if (packet_flags & _BHH_ENCAP_PKT_MPLS) {
	for (i = 0;i < mpls_count;i++) {
	    cur_ptr = _bcm_en_oam_bhh_mpls_label_pack(cur_ptr, &mpls_labels[i]);
	}
    }
    if (packet_flags & _BHH_ENCAP_PKT_GAL) {    
        cur_ptr = _bcm_en_oam_bhh_mpls_label_pack(cur_ptr, &mpls_gal);
    }

    if (packet_flags & _BHH_ENCAP_PKT_G_ACH) {
        cur_ptr = _bcm_en_oam_bhh_ach_header_pack(cur_ptr, &ach);
    }


    /* Set BHH encapsulation length */
    *encap_length = cur_ptr - buffer;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_en_oam_bhh_encap_data_dump
 * Purpose:
 *      Dumps buffer contents.
 * Parameters:
 *      buffer  - (IN) Buffer to dump data.
 *      length  - (IN) Length of buffer.
 * Returns:
 *      None
 */
void
_bcm_en_oam_bhh_encap_data_dump(uint8 *buffer, int length)
{
    int i;

    LOG_CLI((BSL_META("\nBHH encapsulation (length=%d):\n"), length));

    for (i = 0; i < length; i++) {
        if ((i % 16) == 0) {
            LOG_CLI((BSL_META("\n")));
        }
        LOG_CLI((BSL_META(" %02x"), buffer[i]));
    }

    LOG_CLI((BSL_META("\n")));
    return;
}

/*
 * Function:
 *      _bcm_en_oam_bhh_encap_create
 * Purpose:
 *      Creates a BHH packet encapsulation.
 * Parameters:
 *      unit            - (IN) Unit number.
 *      port_id         - (IN) Port.
 *      endpoint_config - (IN/OUT) Pointer to BHH endpoint structure.
 *      encap_data      - (OUT) Buffer returning BHH encapsulation.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The returning BHH encapsulation buffer includes all the
 *      corresponding headers/labels EXCEPT for the BHH control packet.
 */
STATIC int
_bcm_en_oam_bhh_encap_create(int unit, bcm_port_t port_id,
                         _bcm_oam_endpoint_t *endpoint_p,
                         uint8 *encap_data,
			 uint8  *encap_type,
			 uint32 *encap_length)
{
    uint32 packet_flags;

    packet_flags = 0;

    /*
     * Get BHH encapsulation packet format flags
     *
     * Also, perform the following for each BHH tunnel type:
     * - Check for valid parameter values
     * - Set specific values required by the BHH tunnel definition 
     *   (e.g. such as ttl=1,...)
     */
    switch (endpoint_p->type) {
    case bcmOAMEndpointTypeBHHMPLS:
        packet_flags |=
            (_BHH_ENCAP_PKT_MPLS |
             _BHH_ENCAP_PKT_GAL |
             _BHH_ENCAP_PKT_G_ACH);
        break;

    case bcmOAMEndpointTypeBHHMPLSVccv:
        switch(endpoint_p->vccv_type) {

        case bcmOamBhhVccvChannelAch:
            packet_flags |=
                (_BHH_ENCAP_PKT_MPLS |
                 _BHH_ENCAP_PKT_PW |
                 _BHH_ENCAP_PKT_G_ACH);
        break;

        case bcmOamBhhVccvRouterAlert:
            packet_flags |=
                (_BHH_ENCAP_PKT_MPLS |
                 _BHH_ENCAP_PKT_PW |
                 _BHH_ENCAP_PKT_MPLS_ROUTER_ALERT |
                 _BHH_ENCAP_PKT_G_ACH); 
            break;

        case bcmOamBhhVccvTtl:
            packet_flags |=
                (_BHH_ENCAP_PKT_MPLS |
                 _BHH_ENCAP_PKT_PW |
                 _BHH_ENCAP_PKT_G_ACH); 
            break;

	case bcmOamBhhVccvGal13:
            packet_flags |=
                (_BHH_ENCAP_PKT_MPLS |
                 _BHH_ENCAP_PKT_PW |
                 _BHH_ENCAP_PKT_GAL  |
                 _BHH_ENCAP_PKT_G_ACH);
            break;

	default:
	    return BCM_E_PARAM;
	    break;
	}
        break;

    default:
        return BCM_E_PARAM;
    }

    /* Build header/labels and pack in buffer */
    BCM_IF_ERROR_RETURN
        (_bcm_en_oam_bhh_encap_build_pack(unit, port_id,
                                      endpoint_p,
                                      packet_flags,
                                      encap_data,
	                              encap_length));

    /* Set encap type (indicates uC side that checksum is required) */
    *encap_type = SHR_BHH_ENCAP_TYPE_RAW;
                    
#ifdef _BHH_DEBUG_DUMP
    _bcm_en_oam_bhh_encap_data_dump(encap_data, *encap_length);
#endif

    return BCM_E_NONE;
}
    
/*
 * Function:
 *      _bcm_en_oam_bhh_encap_hw_set
 * Purpose:
 *      Sets BHH encapsulation type in HW device.
 * Parameters:
 *      unit            - (IN) Unit number.
 *      module_id       - (IN) Module id.
 *      port_id         - (IN) Port.
 *      is_local        - (IN) Indicates if module id is local.
 *      endpoint_config - (IN) Pointer to BHH endpoint structure.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
_bcm_en_oam_bhh_encap_hw_set(int unit, _bcm_oam_endpoint_t *endpoint_p,
                         bcm_module_t module_id, bcm_port_t port_id,
			     int is_local, bcm_cos_t int_pri)
{
    int rv = BCM_E_NONE;
    l2x_entry_t l2_entry;
    _bcm_oam_info_t *oam_info_p;
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
    bcm_l3_egress_t l3_egress;
    int ingress_if;
    int i;
    int num_entries;
    mpls_entry_entry_t mpls_entry;
    mpls_entry_entry_t mpls_key;
    int cc_type = 0;
    int cw_check = 0; /* NO_CW */
    int mpls_index = 0;
#endif /* BCM_TRIUMPH_SUPPORT &&  BCM_MPLS_SUPPORT */

    SET_OAM_INFO;

    COMPILER_REFERENCE(oam_info_p);

    switch(endpoint_p->type) {
    case bcmOAMEndpointTypeBHHMPLS:
    case bcmOAMEndpointTypeBHHMPLSVccv:

#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)

        /* Insert mpls label field to L2_ENTRY table. */
        sal_memset(&l2_entry, 0, sizeof(l2_entry));
        
        soc_mem_field32_set(unit, L2Xm, &l2_entry, KEY_TYPEf, 4);
        soc_mem_field32_set(unit, L2Xm, &l2_entry, SESSION_IDENTIFIER_TYPEf,
                            1);
        soc_mem_field32_set(unit, L2Xm, &l2_entry, LABELf,
                            endpoint_p->label);
        soc_mem_field32_set(unit, L2Xm, &l2_entry, VALIDf, 1);

        if (!is_local) {
            /* Set BHH_REMOTE = 1, DST_MOD, DST_PORT */
            soc_mem_field32_set(unit, L2Xm, &l2_entry, BFD_REMOTEf, 1);
            soc_mem_field32_set(unit, L2Xm, &l2_entry, BFD_DST_MODf,
                                module_id);
            soc_mem_field32_set(unit, L2Xm, &l2_entry, BFD_DST_PORTf,
                                port_id);
            soc_mem_field32_set(unit, L2Xm, &l2_entry, BFD_INT_PRIf,
                                int_pri);
        } else {
            soc_mem_field32_set(unit, L2Xm, &l2_entry, BFD_CPU_QUEUE_CLASSf,
                                endpoint_p->cpu_qid);
        }

        soc_mem_insert(unit, L2Xm, MEM_BLOCK_ANY, &l2_entry);

        SOC_IF_ERROR_RETURN(bcm_tr_mpls_lock (unit));

        sal_memset(&mpls_key, 0, sizeof(mpls_key));
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_key, MPLS_LABELf,
                                    endpoint_p->label);

        SOC_IF_ERROR_RETURN(soc_mem_search(unit, MPLS_ENTRYm,
                                           MEM_BLOCK_ANY, &mpls_index,
                                           &mpls_key, &mpls_entry, 0));

        if ((soc_MPLS_ENTRYm_field32_get(unit, &mpls_entry,
                                         VALIDf) != 0x1)) {
            bcm_tr_mpls_unlock (unit);
            return BCM_E_PARAM;
        }

        if (endpoint_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {
            if (endpoint_p->vccv_type == bcmOamBhhVccvChannelAch) {
                cc_type = 1;
                cw_check = 1; /* CW_NO_CHECK */
            } else if (endpoint_p->vccv_type == bcmOamBhhVccvRouterAlert) {
                cc_type = 2;
                cw_check = 1; /* CW_NO_CHECK */
            } else if (endpoint_p->vccv_type == bcmOamBhhVccvTtl) {
                cc_type = 3;
                cw_check = 1; /* CW_NO_CHECK */
            } 
        }

        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry,
                                    SESSION_IDENTIFIER_TYPEf, 1);
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry, BFD_ENABLEf, 1);
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry, PW_CC_TYPEf, cc_type);
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry, CW_CHECK_CTRLf, cw_check);
        SOC_IF_ERROR_RETURN(soc_mem_write(unit, MPLS_ENTRYm,
                                          MEM_BLOCK_ANY, mpls_index,
                                          &mpls_entry));

        /*
         * PW CC-3
         *
         * Set MPLS entry DECAP_USE_TTL=0 for corresponding
         * Tunnel Terminator label.
         */
        if (endpoint_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {
            if (endpoint_p->vccv_type == bcmOamBhhVccvTtl) {
                BCM_IF_ERROR_RETURN
                    (bcm_esw_l3_egress_get(unit, endpoint_p->egress_if,
                                           &l3_egress));

                /* Look for Tunnel Terminator label */
                num_entries = soc_mem_index_count(unit, MPLS_ENTRYm);
                for (i = 0; i < num_entries; i++) {
                    BCM_IF_ERROR_RETURN
                        (READ_MPLS_ENTRYm(unit, MEM_BLOCK_ANY, i, &mpls_entry));
                    if (!soc_MPLS_ENTRYm_field32_get(unit, &mpls_entry, VALIDf)) {
                        continue;
                    }
                    if (soc_MPLS_ENTRYm_field32_get(unit, &mpls_entry,
                                                    MPLS_ACTION_IF_BOSf) == 0x1) {
                        continue;    /* L2_SVP */
                    }

                    ingress_if = soc_MPLS_ENTRYm_field32_get(unit, &mpls_entry,
                                                             L3_IIFf);
                    if (ingress_if == l3_egress.intf) {
                        /* Label found */
                        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry,
                                                    DECAP_USE_TTLf, 0);
                        SOC_IF_ERROR_RETURN(soc_mem_write(unit, MPLS_ENTRYm,
                                                          MEM_BLOCK_ALL, i,
                                                          &mpls_entry));
                        break;
                    }
                }
            }
        }

        bcm_tr_mpls_unlock (unit);
#endif /* BCM_TRIUMPH_SUPPORT &&  BCM_MPLS_SUPPORT */
        break;

    default:
        rv = BCM_E_UNAVAIL;
        break;
        }

    return rv;
}

/* BHH Event Handler */
typedef struct _event_handler_s {
    struct _event_handler_s *next;
    bcm_oam_event_types_t event_types;
    bcm_oam_event_cb cb;
    void *user_data;
} _event_handler_t;

/*
 * Function:
 *      _bcm_en_oam_bhh_event_mask_set
 * Purpose:
 *      Set the BHH Events mask.
 *      Events are set per BHH module.
 * Parameters:
 *      unit        - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_XXX  Operation failed
 * Notes:
 */
STATIC int
_bcm_en_oam_bhh_event_mask_set(int unit)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_event_handler_t *event_handler_p;
    uint32 event_mask = 0;
    uint16 reply_len;

    SET_OAM_INFO;

    /* Get event mask from all callbacks */
    for (event_handler_p = oam_info_p->event_handler_list_p;
         event_handler_p != NULL;
         event_handler_p = event_handler_p->next_p) {

        if (SHR_BITGET(event_handler_p->event_types.w,
                       bcmOAMEventBHHLBTimeout)) {
            event_mask |= BHH_BTE_EVENT_LB_TIMEOUT;
        }
        if (SHR_BITGET(event_handler_p->event_types.w,
                       bcmOAMEventBHHLBDiscoveryUpdate)) {
            event_mask |= BHH_BTE_EVENT_LB_DISCOVERY_UPDATE;
        }
        if (SHR_BITGET(event_handler_p->event_types.w,
                       bcmOAMEventBHHCCMTimeout)) {
            event_mask |= BHH_BTE_EVENT_CCM_TIMEOUT;
        }
        if (SHR_BITGET(event_handler_p->event_types.w,
                       bcmOAMEventBHHCCMState)) {
            event_mask |= BHH_BTE_EVENT_STATE;
        }
    }

    /* Update BHH event mask in uKernel */
    if (event_mask != oam_info_p->event_mask) {
        /* Send BHH Event Mask message to uC */
        BCM_IF_ERROR_RETURN(_bcm_en_oam_bhh_msg_send_receive
			    (unit,
			     MOS_MSG_SUBCLASS_BHH_EVENT_MASK_SET,
			     0, event_mask,
			     MOS_MSG_SUBCLASS_BHH_EVENT_MASK_SET_REPLY,
			     &reply_len));

        if (reply_len != 0) {
            return BCM_E_INTERNAL;
        }
    }

    oam_info_p->event_mask = event_mask;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_en_oam_bhh_callback_thread
 * Purpose:
 *      Thread to listen for event messages from uController.
 * Parameters:
 *      param - Pointer to BFD info structure.
 * Returns:
 *      None
 */
STATIC void
_bcm_en_oam_bhh_callback_thread(void *param)
{
    int rv;
    _bcm_oam_info_t *oam_info_p = (_bcm_oam_info_t*)param;
    bcm_oam_event_types_t events;
    bcm_oam_event_type_t event_type;
    bhh_msg_event_t event_msg;
    int sess_id;
    uint32 event_mask;
    _bcm_oam_event_handler_t *event_handler_p;
    _bcm_oam_endpoint_t *endpoint_p;
    char thread_name[SAL_THREAD_NAME_MAX_LEN];
   
    thread_name[0] = 0;
    oam_info_p->event_thread_id   = sal_thread_self();
    oam_info_p->event_thread_kill = 0;
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(oam_info_p->unit,
                            "BHH callback thread starting\n")));
    sal_thread_name(oam_info_p->event_thread_id, thread_name, sizeof (thread_name));
    
    while (1) {
        /* Wait on notifications from uController */
        rv = soc_cmic_uc_msg_receive(oam_info_p->unit, oam_info_p->uc_num,
                                     MOS_MSG_CLASS_BHH_EVENT, &event_msg,
                                     sal_sem_FOREVER);

	event_handler_p = oam_info_p->event_handler_list_p;

        if (BCM_FAILURE(rv) || (oam_info_p->event_thread_kill)) {
            break;  /*  Thread exit */
        } else {

	    /* Get data from event message */
        sess_id = 
            BCM_OAM_BHH_GET_SDK_EP(oam_info_p, ((int)bcm_ntohs(event_msg.s.len)));

	    if (sess_id < 0 || 
		sess_id >= oam_info_p->endpoint_count) {
		    LOG_CLI((BSL_META_U(oam_info_p->unit,
                                        "****** Invalid sess_id:%d ******\n"), sess_id));
	    } else {
		SET_ENDPOINT(sess_id);
		event_mask = bcm_ntohl(event_msg.s.data);

		/* Set events */
		sal_memset(&events, 0, sizeof(events));

		if (event_mask & BHH_BTE_EVENT_LB_TIMEOUT) {
/*		    LOG_CLI((BSL_META_U(oam_info_p->unit,
                                        "****** OAM BHH LB Timeout ******\n")));*/

		    if (oam_info_p->event_handler_count[bcmOAMEventBHHLBTimeout] > 0)
			SHR_BITSET(events.w, bcmOAMEventBHHLBTimeout);
		}
		if (event_mask & BHH_BTE_EVENT_LB_DISCOVERY_UPDATE) {
/*		    LOG_CLI((BSL_META_U(oam_info_p->unit,
                                        "****** OAM BHH LB Discovery Update ******\n")));*/
		    if (oam_info_p->event_handler_count[bcmOAMEventBHHLBDiscoveryUpdate] > 0)
			SHR_BITSET(events.w, bcmOAMEventBHHLBDiscoveryUpdate);
		}
		if (event_mask & BHH_BTE_EVENT_CCM_TIMEOUT) {
/*		    LOG_CLI((BSL_META_U(oam_info_p->unit,
                                        "****** OAM BHH CCM Timeout ******\n")));*/
		    if (oam_info_p->event_handler_count[bcmOAMEventBHHCCMTimeout] > 0)
			SHR_BITSET(events.w, bcmOAMEventBHHCCMTimeout);
		}
		if (event_mask & BHH_BTE_EVENT_STATE) {
/*		    LOG_CLI((BSL_META_U(oam_info_p->unit,
                                        "****** OAM BHH State ******\n")));*/
		    if (oam_info_p->event_handler_count[bcmOAMEventBHHCCMState] > 0)
			SHR_BITSET(events.w, bcmOAMEventBHHCCMState);
		}

		/* Loop over registered callbacks,
		 * If any match the events field, then invoke
		 */
		for (event_handler_p = oam_info_p->event_handler_list_p;
		     event_handler_p != NULL;
		     event_handler_p = event_handler_p->next_p) {
		    for (event_type = bcmOAMEventBHHLBTimeout; event_type < bcmOAMEventCount; ++event_type) {
			if (SHR_BITGET(events.w, event_type)) {
			    if (SHR_BITGET(event_handler_p->event_types.w,
					   event_type)) {
				event_handler_p->cb(oam_info_p->unit, 
						    0,
						    event_type, 
						    endpoint_p->group_index, /* Group index */
						    sess_id, /* Endpoint index */
						    event_handler_p->user_data);
			    }
			}
		    }
		}
	    }
	}
    }
    if (BCM_FAILURE(rv)) { 
        LOG_ERROR(BSL_LS_BCM_OAM,
                  (BSL_META_U(oam_info_p->unit,
                              "AbnormalThreadExit:%s\n"), thread_name));  
    }
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(oam_info_p->unit,
                            "BHH callback thread exiting\n")));
    oam_info_p->event_thread_kill = 0;
    oam_info_p->event_thread_id   = NULL;
    sal_thread_exit(0);
}

/*
 * Function:
 *     bcm_en_oam_loopback_add
 * Purpose:
 *     
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_en_oam_loopback_add(int unit, bcm_oam_loopback_t *loopback_p)
{
    _bcm_oam_info_t *oam_info_p;
    int result = BCM_E_UNAVAIL;
    shr_bhh_msg_ctrl_loopback_add_t msg;
    _bcm_oam_endpoint_t *endpoint_p;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    uint32 flags = 0;
    int sess_id = 0;

    SET_OAM_INFO;

    CHECK_INIT;

    BCM_OAM_BHH_VALIDATE_END_POINT(oam_info_p, loopback_p->id);

    SET_ENDPOINT(loopback_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(oam_info_p, loopback_p->id);

    /*
     * Only BHH is supported
     */
    if (endpoint_p->type == bcmOAMEndpointTypeBHHMPLS ||
	endpoint_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {

	/*
	 * Convert host space flags to uKernel space flags
	 */
	if (loopback_p->flags & BCM_OAM_BHH_INC_REQUESTING_MEP_TLV)
	    flags |= BCM_BHH_INC_REQUESTING_MEP_TLV;

	if (loopback_p->flags & BCM_OAM_BHH_LBM_INGRESS_DISCOVERY_MEP_TLV)
	    flags |= BCM_BHH_LBM_INGRESS_DISCOVERY_MEP_TLV;
	else if (loopback_p->flags & BCM_OAM_BHH_LBM_EGRESS_DISCOVERY_MEP_TLV)
	    flags |= BCM_BHH_LBM_EGRESS_DISCOVERY_MEP_TLV;
	else if (loopback_p->flags & BCM_OAM_BHH_LBM_ICC_MEP_TLV)
	    flags |= BCM_BHH_LBM_ICC_MEP_TLV;
	else if (loopback_p->flags & BCM_OAM_BHH_LBM_ICC_MIP_TLV)
	    flags |= BCM_BHH_LBM_ICC_MIP_TLV;
	else
	    flags |= BCM_BHH_INC_REQUESTING_MEP_TLV; /* Default */

	if (loopback_p->flags & BCM_OAM_BHH_LBR_ICC_MEP_TLV)
	    flags |= BCM_BHH_LBR_ICC_MEP_TLV; 
	else if (loopback_p->flags & BCM_OAM_BHH_LBR_ICC_MIP_TLV)
	    flags |= BCM_BHH_LBR_ICC_MIP_TLV;

	msg.flags   = flags;
	msg.sess_id = sess_id;

	/*
	 * Set period
	 */
	msg.period  = en_ccm_periods[_bcm_en_oam_quantize_ccm_period(loopback_p->period)];

	/*
	 * Check TTL
	 */
	if (loopback_p->ttl == 0 || loopback_p->ttl > 255) {
	    return BCM_E_PARAM;
	}
	msg.ttl     = loopback_p->ttl;

	/* Pack control message data into DMA buffer */
	buffer     = oam_info_p->dma_buffer;
	buffer_ptr = shr_bhh_msg_ctrl_loopback_add_pack(buffer, &msg);
	buffer_len = buffer_ptr - buffer;

	/* Send BHH Session Update message to uC */
	BCM_IF_ERROR_RETURN
	    (_bcm_en_oam_bhh_msg_send_receive(unit,
					  MOS_MSG_SUBCLASS_BHH_LOOPBACK_ADD,
					  buffer_len, 0,
					  MOS_MSG_SUBCLASS_BHH_LOOPBACK_ADD_REPLY,
					  &reply_len));
	if (reply_len != 0) {
	    result =  BCM_E_INTERNAL;
	} else {
	    result = BCM_E_NONE;
	}
    }
    else {
        return BCM_E_UNAVAIL;
    }

    return result;
}



/*
 * Function:
 *     bcm_en_oam_loopback_get
 * Purpose:
 *     
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_en_oam_loopback_get(int unit, bcm_oam_loopback_t *loopback_p)
{
    _bcm_oam_info_t *oam_info_p;
    int result = BCM_E_UNAVAIL;
    shr_bhh_msg_ctrl_loopback_get_t msg;
    _bcm_oam_endpoint_t *endpoint_p;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int sess_id = 0;

    SET_OAM_INFO;

    CHECK_INIT;

    BCM_OAM_BHH_VALIDATE_END_POINT(oam_info_p, loopback_p->id);

    SET_ENDPOINT(loopback_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(oam_info_p, loopback_p->id);

    /*
     * Only BHH is supported
     */
    if (endpoint_p->type == bcmOAMEndpointTypeBHHMPLS ||
	endpoint_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {

	/* Send BHH Session Update message to uC */
	BCM_IF_ERROR_RETURN
	    (_bcm_en_oam_bhh_msg_send_receive(unit,
					  MOS_MSG_SUBCLASS_BHH_LOOPBACK_GET,
					  sess_id, 0,
					  MOS_MSG_SUBCLASS_BHH_LOOPBACK_GET_REPLY,
					  &reply_len));

	/* Pack control message data into DMA buffer */
	buffer     = oam_info_p->dma_buffer;
	buffer_ptr = shr_bhh_msg_ctrl_loopback_get_unpack(buffer, &msg);
	buffer_len = buffer_ptr - buffer;

	if (reply_len != buffer_len) {
	    result =  BCM_E_INTERNAL;
	} else {
	    /*
	     * Convert kernel space flags to host space flags
	     */
	    if (msg.flags & BCM_BHH_INC_REQUESTING_MEP_TLV)
		loopback_p->flags |= BCM_OAM_BHH_INC_REQUESTING_MEP_TLV;

	    if (msg.flags & BCM_BHH_LBM_INGRESS_DISCOVERY_MEP_TLV)
		loopback_p->flags |= BCM_OAM_BHH_LBM_INGRESS_DISCOVERY_MEP_TLV;
	    else if (msg.flags & BCM_BHH_LBM_EGRESS_DISCOVERY_MEP_TLV)
		loopback_p->flags |= BCM_OAM_BHH_LBM_EGRESS_DISCOVERY_MEP_TLV;
	    else if (msg.flags & BCM_BHH_LBM_ICC_MEP_TLV)
		loopback_p->flags |= BCM_OAM_BHH_LBM_ICC_MEP_TLV;
	    else if (msg.flags & BCM_BHH_LBM_ICC_MIP_TLV)
		loopback_p->flags |= BCM_OAM_BHH_LBM_ICC_MIP_TLV;

	    if (msg.flags & BCM_BHH_LBR_ICC_MEP_TLV)
		loopback_p->flags |= BCM_OAM_BHH_LBR_ICC_MEP_TLV;
	    else if (msg.flags & BCM_BHH_LBR_ICC_MIP_TLV)
		loopback_p->flags |= BCM_OAM_BHH_LBR_ICC_MIP_TLV;

	    loopback_p->period = msg.period;
	    loopback_p->ttl    = msg.ttl;
	    loopback_p->discovered_me.flags = msg.discovery_flags;
	    loopback_p->discovered_me.id    = msg.discovery_id;
	    loopback_p->discovered_me.ttl   = msg.discovery_ttl;
	    loopback_p->rx_count            = msg.rx_count;
	    loopback_p->tx_count            = msg.tx_count;
	    loopback_p->drop_count          = msg.drop_count;
	    loopback_p->unexpected_response = msg.unexpected_response;
	    loopback_p->out_of_sequence         = msg.out_of_sequence;
	    loopback_p->local_mipid_missmatch   = msg.local_mipid_missmatch;
	    loopback_p->remote_mipid_missmatch  = msg.remote_mipid_missmatch;
	    loopback_p->invalid_target_mep_tlv  = msg.invalid_target_mep_tlv;
	    loopback_p->invalid_mep_tlv_subtype = msg.invalid_mep_tlv_subtype;
	    loopback_p->invalid_tlv_offset      = msg.invalid_tlv_offset;

	    result = BCM_E_NONE;
	}
    }
    else {
        return BCM_E_UNAVAIL;
    }

    return result;
}

/*
 * Function:
 *     bcm_en_oam_loopback_delete
 * Purpose:
 *     
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_en_oam_loopback_delete(int unit, bcm_oam_loopback_t *loopback_p)
{
    _bcm_oam_info_t *oam_info_p;
    int result = BCM_E_UNAVAIL;
    shr_bhh_msg_ctrl_loopback_delete_t msg;
    _bcm_oam_endpoint_t *endpoint_p;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int sess_id = 0;

    SET_OAM_INFO;

    CHECK_INIT;

    BCM_OAM_BHH_VALIDATE_END_POINT(oam_info_p, loopback_p->id);

    SET_ENDPOINT(loopback_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(oam_info_p, loopback_p->id);

    /*
     * Only BHH is supported
     */
    if (endpoint_p->type == bcmOAMEndpointTypeBHHMPLS ||
	endpoint_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {

	msg.sess_id = sess_id;

	/* Pack control message data into DMA buffer */
	buffer     = oam_info_p->dma_buffer;
	buffer_ptr = shr_bhh_msg_ctrl_loopback_delete_pack(buffer, &msg);
	buffer_len = buffer_ptr - buffer;

	/* Send BHH Session Update message to uC */
	BCM_IF_ERROR_RETURN
	    (_bcm_en_oam_bhh_msg_send_receive(unit,
					  MOS_MSG_SUBCLASS_BHH_LOOPBACK_DELETE,
					  buffer_len, 0,
					  MOS_MSG_SUBCLASS_BHH_LOOPBACK_DELETE_REPLY,
					  &reply_len));
	if (reply_len != 0) {
	    result =  BCM_E_INTERNAL;
	} else {
	    result = BCM_E_NONE;
	}
    }
    else {
        return BCM_E_UNAVAIL;
    }

    return result;
}
#endif /* defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH) */


/*
 * Function:
 *      bcm_en_oam_detach
 * Purpose:
 *      Shut down the OAM subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_detach(
    int unit)
{
    _bcm_oam_info_t *oam_info_p;
    bcm_port_t port_index;
    int rv;
#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_BHH)
    uint16 reply_len;
#endif

    SET_OAM_INFO;

    if (!oam_info_p->initialized)
    {
        return BCM_E_NONE;
    }

    /* Disable CCM transmission */

    SOC_IF_ERROR_RETURN(WRITE_OAM_TX_CONTROLr(unit, 0));
    
    /* Disable OAM message reception on all ports */

    PBMP_ALL_ITER(unit, port_index)
    {
        BCM_IF_ERROR_RETURN(bcm_esw_port_control_set(unit,
            port_index, bcmPortControlOAMEnable, 0));
    }

    /*
     * BHH specific 
     */
    if (soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_BHH)
    if (SOC_IS_KATANAX(unit)) {
	/* 
	 * Event Handler thread exit signal 
	 */
	oam_info_p->event_thread_kill = 1;
	soc_cmic_uc_msg_receive_cancel(unit, oam_info_p->uc_num,
				       MOS_MSG_CLASS_BHH_EVENT);

	/*
	 * Send BHH Uninit message to uC
	 * Ignore error since that may indicate uKernel was reloaded.
	 */
	rv = _bcm_en_oam_bhh_msg_send_receive(unit,
					  MOS_MSG_SUBCLASS_BHH_UNINIT,
					  0, 0,
					  MOS_MSG_SUBCLASS_BHH_UNINIT_REPLY,
					  &reply_len);
	if (BCM_SUCCESS(rv) && (reply_len != 0)) {
	    return BCM_E_INTERNAL;
	}

	/* 
	 * Delete CPU COS queue mapping entries for BHH packets 
	 */
	if (oam_info_p->cpu_cosq_ach_error_index >= 0) {
	    BCM_IF_ERROR_RETURN
		(bcm_esw_rx_cosq_mapping_delete(unit,
						oam_info_p->cpu_cosq_ach_error_index));
	    oam_info_p->cpu_cosq_ach_error_index = -1;
	}
	if (oam_info_p->cpu_cosq_invalid_error_index >= 0) {
	    BCM_IF_ERROR_RETURN
		(bcm_esw_rx_cosq_mapping_delete(unit,
						oam_info_p->cpu_cosq_invalid_error_index));
	    oam_info_p->cpu_cosq_invalid_error_index = -1;
	}
    }
#endif
    }
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        soc_kt_oam_handler_register(unit, NULL);
    } else
#endif /* BCM_KATANA_SUPPORT */
    {
        soc_enduro_oam_handler_register(unit, NULL);
    }
    _bcm_en_oam_event_unregister_all(oam_info_p);

    _bcm_en_oam_free_memory(unit, oam_info_p);

    /* De-initialize the ING_SERVICE_PRI_MAP table */
    rv = soc_profile_mem_destroy(unit, ing_pri_map_profile[unit]);
    SOC_IF_ERROR_RETURN(rv);
    
    sal_free(ing_pri_map_profile[unit]);
    ing_pri_map_profile[unit] = NULL;
  
#if defined(BCM_KATANA_SUPPORT) 
    if (SOC_IS_KATANAX(unit)) {
        /* De-initialize the OAM_OPCODE_PROFILE table */
        rv = soc_profile_mem_destroy(unit, oam_opcode_profile[unit]);
        SOC_IF_ERROR_RETURN(rv);
    
        sal_free(oam_opcode_profile[unit]);
        oam_opcode_profile[unit] = NULL;
    }
#endif /* BCM_KATANA_SUPPORT */

    oam_info_p->initialized = 0;

    return BCM_E_NONE;
}

static void _bcm_en_oam_group_name_mangle(uint8 *name_p,
    uint8 *mangled_name_p)
{
    uint8 *byte_p = name_p + BCM_OAM_GROUP_NAME_LENGTH - 1;
    int bytes_left = BCM_OAM_GROUP_NAME_LENGTH;

    while (bytes_left > 0)
    {
        *mangled_name_p = *byte_p;

        ++mangled_name_p;
        --byte_p;
        --bytes_left;
    }
}

/*
 * Function:
 *      bcm_en_oam_group_create
 * Purpose:
 *      Create or replace an OAM group object
 * Parameters:
 *      unit - (IN) Unit number.
 *      group_info - (IN/OUT) Pointer to an OAM group structure
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_group_create(
    int unit, 
    bcm_oam_group_info_t *group_info)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_group_t *group_p;
    int replace;
    int group_index;
    int copy_to_cpu = 0;

#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
    shr_bhh_msg_ctrl_sess_set_t msg_sess;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    uint32 session_flags = 0;
    _bcm_oam_endpoint_t *endpoint_p;
    int endpoint_index;
#endif

    maid_reduction_entry_t maid_reduction_entry;
    ma_state_entry_t ma_state_entry;
    uint8 mangled_group_name[MANGLED_GROUP_NAME_LENGTH];

    SET_OAM_INFO;

    CHECK_INIT;

    replace = group_info->flags & BCM_OAM_GROUP_REPLACE;

    if (group_info->flags & BCM_OAM_GROUP_WITH_ID)
    {
        group_index = group_info->id;

        VALIDATE_GROUP_INDEX(group_index);

        if (!replace && (oam_info_p->groups[group_index].in_use))
        {
            return BCM_E_EXISTS;
        }
    }
    else
    {
        if (replace)
        {
            return BCM_E_PARAM;
        }

        for (group_index = 0; group_index < oam_info_p->group_count;
            ++group_index)
        {
            if (!oam_info_p->groups[group_index].in_use)
            {
                break;
            }
        }

        if (group_index >= oam_info_p->group_count)
        {
            return BCM_E_FULL;
        }

        group_info->id = group_index;
    }

    SET_GROUP(group_index);

    memcpy(group_p->name, group_info->name, BCM_OAM_GROUP_NAME_LENGTH);

    /* MAID_REDUCTION entry */

    _bcm_en_oam_group_name_mangle(group_p->name, mangled_group_name);

    sal_memset(&maid_reduction_entry, 0, sizeof(maid_reduction_entry_t));

    soc_MAID_REDUCTIONm_field32_set(unit, &maid_reduction_entry, REDUCED_MAIDf,
        soc_draco_crc32(mangled_group_name, MANGLED_GROUP_NAME_LENGTH));

    soc_MAID_REDUCTIONm_field32_set(unit, &maid_reduction_entry, SW_RDIf,
        (group_info->flags & BCM_OAM_GROUP_REMOTE_DEFECT_TX) ? 1 : 0);

    if (soc_feature(unit, soc_feature_bhh)) { 
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
        /* Send message to uC to set the Soft RDI, if RDI flag is set */
        if (group_info->flags & BCM_OAM_GROUP_REMOTE_DEFECT_TX) { 
      
            /* Get the endpoint info of the group */   
            for (endpoint_index = 0; endpoint_index < oam_info_p->endpoint_count;
                 ++endpoint_index) {
                 SET_ENDPOINT(endpoint_index);

                 if (endpoint_p->in_use && endpoint_p->group_index == group_index) { 
              
                     /* Set the RDI flag in session bits */ 
                     session_flags |= SHR_BHH_SESS_SET_F_RDI;
              
                     /* Get the session is from endpoint */
                     msg_sess.sess_id =
                         BCM_OAM_BHH_GET_UKERNEL_EP(oam_info_p,
                                                    endpoint_index);     
                        
                     /* Pack control message data into DMA buffer */
                     msg_sess.flags = session_flags;

                     buffer     = oam_info_p->dma_buffer;
                     buffer_ptr = shr_bhh_msg_ctrl_sess_set_pack(buffer, &msg_sess);
                     buffer_len = buffer_ptr - buffer;

                     /* Send BHH Session Update message to uC */
                     BCM_IF_ERROR_RETURN
                            (_bcm_en_oam_bhh_msg_send_receive(unit,
                                                      MOS_MSG_SUBCLASS_BHH_SESS_SET,
                                                      buffer_len, 0,
                                                      MOS_MSG_SUBCLASS_BHH_SESS_SET_REPLY,
                                                      &reply_len));
                     if (reply_len != 0) {
                         return BCM_E_INTERNAL;
                     }
                 }
            }
        }  
#endif
    }
    copy_to_cpu = ((group_info->flags & BCM_OAM_GROUP_COPY_TO_CPU) ? 1 : 0);
    soc_MAID_REDUCTIONm_field32_set(unit, &maid_reduction_entry,
                                        COPY_TO_CPUf, copy_to_cpu);

    soc_MAID_REDUCTIONm_field32_set(unit, &maid_reduction_entry, VALIDf, 1);

    SOC_IF_ERROR_RETURN(WRITE_MAID_REDUCTIONm(unit, MEM_BLOCK_ALL,
        group_index, &maid_reduction_entry));

    /* MA_STATE entry */
    sal_memset(&ma_state_entry, 0, sizeof(ma_state_entry_t));

    if (replace) {
        /* Keep defect status if group is being replaced */
        SOC_IF_ERROR_RETURN(READ_MA_STATEm(unit, MEM_BLOCK_ALL, group_index, 
            &ma_state_entry));
    } 

    soc_MA_STATEm_field32_set(unit, &ma_state_entry, LOWESTALARMPRIf,
								group_info->lowest_alarm_priority);

    soc_MA_STATEm_field32_set(unit, &ma_state_entry, VALIDf, 1);

    SOC_IF_ERROR_RETURN(WRITE_MA_STATEm(unit, MEM_BLOCK_ALL, group_index,
        &ma_state_entry));

    /* Local control block */

    group_p->in_use = 1;

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return BCM_E_NONE;
}

static int _bcm_en_oam_read_clear_faults(int unit, int entry_index,
    _bcm_oam_fault_t *faults, soc_mem_t mem, uint32 *entry_p,
    uint32 *fault_bits_p, uint32 *persistent_fault_bits_p,
    uint32 clear_persistent_fault_bits)
{
    _bcm_oam_fault_t *fault_p;
    uint32 ccm_read_control_reg_value = 0;
    uint32 clear_mask = 0; /* Mask to clear persistent faults*/

    for (fault_p = faults; fault_p->mask != 0; ++fault_p)
    {
        /* Current fault state */

        if (soc_mem_field32_get(unit, mem, entry_p,
            fault_p->current_field) != 0)
        {
            *fault_bits_p |= fault_p->mask;
        }

        /* Sticky fault state */

        if (soc_mem_field32_get(unit, mem, entry_p,
            fault_p->sticky_field) != 0)
        {
            *persistent_fault_bits_p |= fault_p->mask;

            /* Clear persistent faults if requested */

            if (clear_persistent_fault_bits)
            {
                clear_mask |= fault_p->clear_sticky_mask;
            }
        }
    }

    /* If any clear bits were set, do the clear now */

    if (clear_persistent_fault_bits && clear_mask)
    {
        soc_reg_field_set(unit, CCM_READ_CONTROLr, 
            &ccm_read_control_reg_value, BITS_TO_CLEARf, clear_mask);

        soc_reg_field_set(unit, CCM_READ_CONTROLr,
            &ccm_read_control_reg_value, ENABLE_CLEARf, 1);
        /*
         * MA_STATE table, MEMORYf should be 0.
         */  
        if (mem == MA_STATEm) {
            soc_reg_field_set(unit, CCM_READ_CONTROLr,
                &ccm_read_control_reg_value, MEMORYf, 0);
        } else {
            soc_reg_field_set(unit, CCM_READ_CONTROLr,
                &ccm_read_control_reg_value, MEMORYf, 1);
        }

        soc_reg_field_set(unit, CCM_READ_CONTROLr,
            &ccm_read_control_reg_value, INDEXf, entry_index);

        SOC_IF_ERROR_RETURN(WRITE_CCM_READ_CONTROLr(unit,
            ccm_read_control_reg_value));
    }

    return BCM_E_NONE;
}

static int _bcm_en_oam_get_group(int unit, int group_index,
    _bcm_oam_group_t *group_p, bcm_oam_group_info_t *group_info)
{
    maid_reduction_entry_t maid_reduction_entry;
    ma_state_entry_t ma_state_entry;

    group_info->id = group_index;

    memcpy(group_info->name, group_p->name, BCM_OAM_GROUP_NAME_LENGTH);

    /* MAID_REDUCTION entry */

    SOC_IF_ERROR_RETURN(READ_MAID_REDUCTIONm(unit, MEM_BLOCK_ANY, group_index,
        &maid_reduction_entry));

    if (soc_MAID_REDUCTIONm_field32_get(unit, &maid_reduction_entry,
        SW_RDIf) != 0)
    {
        group_info->flags |= BCM_OAM_GROUP_REMOTE_DEFECT_TX;
    }

	if (soc_MAID_REDUCTIONm_field32_get(unit, &maid_reduction_entry,
        COPY_TO_CPUf) != 0)
    {
        group_info->flags |= BCM_OAM_GROUP_COPY_TO_CPU;
    }

    /* MA_STATE entry */

    SOC_IF_ERROR_RETURN(READ_MA_STATEm(unit, MEM_BLOCK_ANY, group_index,
        &ma_state_entry));
	
	/* Get the Lowest Alarm Priority value*/
	group_info->lowest_alarm_priority =
		soc_MA_STATEm_field32_get(unit, &ma_state_entry, LOWESTALARMPRIf);

    return _bcm_en_oam_read_clear_faults(unit, group_index, en_group_faults,
        MA_STATEm, (uint32 *) &ma_state_entry, &group_info->faults,
        &group_info->persistent_faults, group_info->clear_persistent_faults);
}

/*
 * Function:
 *      bcm_en_oam_group_get
 * Purpose:
 *      Get an OAM group object
 * Parameters:
 *      unit - (IN) Unit number.
 *      group - (IN) The ID of the group object to get
 *      group_info - (OUT) Pointer to an OAM group structure to receive the data
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_group_get(
    int unit, 
    bcm_oam_group_t group, 
    bcm_oam_group_info_t *group_info)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_group_t *group_p;

    SET_OAM_INFO;

    CHECK_INIT;

    VALIDATE_GROUP_INDEX(group);

    SET_GROUP(group);

    if (!group_p->in_use)
    {
        return BCM_E_NOT_FOUND;
    }

    return _bcm_en_oam_get_group(unit, group, group_p, group_info);
}

static int _bcm_en_oam_destroy_group(int unit, int group_index,
    _bcm_oam_group_t *group_p)
{
    maid_reduction_entry_t maid_reduction_entry;
    ma_state_entry_t ma_state_entry;

    /* Remove all associated endpoints */

    bcm_en_oam_endpoint_destroy_all(unit, group_index);

    /* MAID_REDUCTION entry */

    soc_MAID_REDUCTIONm_field32_set(unit, &maid_reduction_entry, VALIDf, 0);

    SOC_IF_ERROR_RETURN(WRITE_MAID_REDUCTIONm(unit, MEM_BLOCK_ALL,
        group_index, &maid_reduction_entry));

    /* MA_STATE entry */

    soc_MA_STATEm_field32_set(unit, &ma_state_entry, VALIDf, 0);

    SOC_IF_ERROR_RETURN(WRITE_MA_STATEm(unit, MEM_BLOCK_ALL, group_index,
        &ma_state_entry));

    group_p->in_use = 0;

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_en_oam_group_destroy
 * Purpose:
 *      Destroy an OAM group object.  All OAM endpoints associated
 *      with the group will also be destroyed.
 * Parameters:
 *      unit - (IN) Unit number.
 *      group - (IN) The ID of the OAM group object to destroy
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_group_destroy(
    int unit, 
    bcm_oam_group_t group)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_group_t *group_p;

    SET_OAM_INFO;

    CHECK_INIT;

    VALIDATE_GROUP_INDEX(group);

    SET_GROUP(group);

    if (!group_p->in_use)
    {
        return BCM_E_NOT_FOUND;
    }

    return _bcm_en_oam_destroy_group(unit, group, group_p);
}

/*
 * Function:
 *      bcm_en_oam_group_destroy_all
 * Purpose:
 *      Destroy all OAM group objects.  All OAM endpoints will also be
 *      destroyed.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_group_destroy_all(
    int unit)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_group_t *group_p;
    int group_index;

    SET_OAM_INFO;

    CHECK_INIT;

    for (group_index = 0; group_index < oam_info_p->group_count;
        ++group_index)
    {
        SET_GROUP(group_index);

        if (group_p->in_use)
        {
            BCM_IF_ERROR_RETURN(_bcm_en_oam_destroy_group(unit, group_index,
                group_p));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_en_oam_group_traverse
 * Purpose:
 *      Traverse the entire set of OAM groups, calling a specified
 *      callback for each one
 * Parameters:
 *      unit - (IN) Unit number.
 *      cb - (IN) A pointer to the callback function to call for each OAM group
 *      user_data - (IN) Pointer to user data to supply in the callback
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_group_traverse(
    int unit, 
    bcm_oam_group_traverse_cb cb, 
    void *user_data)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_group_t *group_p;
    int group_index;
    bcm_oam_group_info_t group_info;

    SET_OAM_INFO;

    CHECK_INIT;

    if (cb == NULL)
    {
        return BCM_E_PARAM;
    }

    for (group_index = 0; group_index < oam_info_p->group_count;
        ++group_index)
    {
        SET_GROUP(group_index);

        if (group_p->in_use)
        {
            bcm_oam_group_info_t_init(&group_info);

            BCM_IF_ERROR_RETURN(_bcm_en_oam_get_group(unit, group_index,
                group_p, &group_info));

            BCM_IF_ERROR_RETURN(cb(unit, &group_info, user_data));
        }
    }

    return BCM_E_NONE;
}

static int _bcm_en_oam_find_free_endpoint(SHR_BITDCL *endpoints,
    int endpoint_count, int increment, int offset)
{
    int endpoint_index;

    for (endpoint_index = 0; endpoint_index < endpoint_count;
        endpoint_index += increment)
    {
        if (!SHR_BITGET(endpoints, endpoint_index + offset))
        {
            break;
        }
    }

    if (endpoint_index >= endpoint_count)
    {
        endpoint_index = -1;
    }

    return endpoint_index + offset;
}

static int _bcm_en_oam_quantize_ccm_period(int period)
{
    int quantized_period;

    if (period == 0)
    {
        quantized_period = 0;
    }
    else
    {
        /* Find closest supported period */

        for (quantized_period = 1;
            en_ccm_periods[quantized_period] !=
                _BCM_OAM_ENDPOINT_CCM_PERIOD_UNDEFINED; ++quantized_period)
        {
            if ((uint32)period < en_ccm_periods[quantized_period])
            {
                break;
            }
        }

        if (quantized_period > 1)
        {
            if (en_ccm_periods[quantized_period] ==
                _BCM_OAM_ENDPOINT_CCM_PERIOD_UNDEFINED)
            {
                /* Use the highest defined value */

                --quantized_period;
            }
            else
            {
                if (period - en_ccm_periods[quantized_period - 1] <
                    en_ccm_periods[quantized_period] - period)
                {
                    /* Closer to the lower value */

                    --quantized_period;
                }
            }
        }
    }

    return quantized_period;
}

static void _bcm_en_oam_make_rmep_key(int unit,
    l3_entry_ipv4_unicast_entry_t *l3_key_p, uint16 name, int level, bcm_vlan_t vlan,
    uint32 sglp)
{
    sal_memset(l3_key_p, 0, sizeof(l3_entry_ipv4_unicast_entry_t));

    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, l3_key_p, RMEP__MEPIDf, name);
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, l3_key_p, RMEP__MDLf, level);
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, l3_key_p, RMEP__VIDf, vlan);
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, l3_key_p, RMEP__SGLPf, sglp);
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, l3_key_p, KEY_TYPEf,
        TR_L3_HASH_KEY_TYPE_RMEP);
}

static int _bcm_en_oam_delete_rmep(int unit, _bcm_oam_endpoint_t *endpoint_p)
{
    _bcm_oam_info_t *oam_info_p;
    l3_entry_ipv4_unicast_entry_t l3_key;
    rmep_entry_t rmep_entry;
    ma_state_entry_t ma_state_entry;
    int some_rmep_ccm_defect_counter = 0;
    int current_some_rmep_ccm_defect = 0;
    int some_rdi_defect_counter = 0;
    int current_some_rdi_defect = 0;
    int cur_rmep_ccm_defect = 0;
    int cur_rmep_last_rdi = 0;


    SET_OAM_INFO;

    sal_memset(&ma_state_entry, 0, sizeof(ma_state_entry_t));
    sal_memset(&rmep_entry, 0, sizeof(rmep_entry_t));
    SOC_IF_ERROR_RETURN(READ_RMEPm(unit, MEM_BLOCK_ANY, endpoint_p->remote_index,
                                    &rmep_entry));
    cur_rmep_ccm_defect = soc_RMEPm_field32_get(unit, &rmep_entry,
                                                CURRENT_RMEP_CCM_DEFECTf);
    cur_rmep_last_rdi = soc_RMEPm_field32_get(unit, &rmep_entry,
                                                CURRENT_RMEP_LAST_RDIf);

    if (cur_rmep_ccm_defect || cur_rmep_last_rdi) {
        SOC_IF_ERROR_RETURN(READ_MA_STATEm(unit, MEM_BLOCK_ANY,
                                            endpoint_p->group_index,
                                            &ma_state_entry));

        some_rmep_ccm_defect_counter = soc_MA_STATEm_field32_get(unit,
                                                                &ma_state_entry,
                                                                SOME_RMEP_CCM_DEFECT_COUNTERf);
        current_some_rmep_ccm_defect = soc_MA_STATEm_field32_get(unit,
                                                                &ma_state_entry,
                                                                CURRENT_SOME_RMEP_CCM_DEFECTf);

        if (cur_rmep_ccm_defect && some_rmep_ccm_defect_counter > 0) {
            --some_rmep_ccm_defect_counter;
            soc_MA_STATEm_field32_set(unit, &ma_state_entry,
                                        SOME_RMEP_CCM_DEFECT_COUNTERf,
                                        some_rmep_ccm_defect_counter);

            if (some_rmep_ccm_defect_counter == 0) {
                current_some_rmep_ccm_defect = 0;
                soc_MA_STATEm_field32_set(unit, &ma_state_entry,
                                        CURRENT_SOME_RMEP_CCM_DEFECTf,
                                        current_some_rmep_ccm_defect);
            }
        }
        some_rdi_defect_counter = soc_MA_STATEm_field32_get(unit,
                                                        &ma_state_entry,
                                                        SOME_RDI_DEFECT_COUNTERf);
        current_some_rdi_defect = soc_MA_STATEm_field32_get(unit,
                                                        &ma_state_entry,
                                                        CURRENT_SOME_RDI_DEFECTf);
        if (cur_rmep_last_rdi && some_rdi_defect_counter > 0) {
            --some_rdi_defect_counter;
            soc_MA_STATEm_field32_set(unit, &ma_state_entry,
                                        SOME_RDI_DEFECT_COUNTERf,
                                        some_rdi_defect_counter);
            if (some_rdi_defect_counter == 0) {
                current_some_rdi_defect = 0;
                soc_MA_STATEm_field32_set(unit, &ma_state_entry,
                                            CURRENT_SOME_RDI_DEFECTf,
                                            current_some_rdi_defect);
            }
        }
        SOC_IF_ERROR_RETURN(WRITE_MA_STATEm(unit, MEM_BLOCK_ALL,
                                            endpoint_p->group_index,
                                            &ma_state_entry));
    }

    /* RMEP entry */
    SOC_IF_ERROR_RETURN(soc_mem_field32_modify(unit, RMEPm, 
                                        endpoint_p->remote_index, VALIDf, 0));

    /* L3 entry */
    if (endpoint_p->vp != 0) {
        _bcm_en_oam_make_rmep_key(unit, &l3_key, endpoint_p->name,
            endpoint_p->level, 0, endpoint_p->vp);
    } else {
        _bcm_en_oam_make_rmep_key(unit, &l3_key, endpoint_p->name,
            endpoint_p->level, endpoint_p->vlan, endpoint_p->glp);
    }
    
    SOC_IF_ERROR_RETURN(soc_mem_delete(unit, L3_ENTRY_IPV4_UNICASTm,
        MEM_BLOCK_ALL, &l3_key));

    /* Local control block */

    SHR_BITCLR(oam_info_p->remote_endpoints_in_use, endpoint_p->remote_index);

    oam_info_p->remote_endpoints[endpoint_p->remote_index] =
        BCM_OAM_ENDPOINT_INVALID;

    return BCM_E_NONE;
}

static int _bcm_en_oam_find_lmep(int unit, bcm_vlan_t vlan, uint32 sglp,
    int *index_p, l3_entry_ipv4_unicast_entry_t *l3_entry_p)
{
    l3_entry_ipv4_unicast_entry_t l3_key;
    sal_memset(&l3_key, 0, sizeof(l3_entry_ipv4_unicast_entry_t));

    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_key, LMEP__VIDf, vlan);

    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_key, LMEP__SGLPf, sglp);

    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_key, KEY_TYPEf,
        TR_L3_HASH_KEY_TYPE_LMEP);

    return soc_mem_search(unit, L3_ENTRY_IPV4_UNICASTm, MEM_BLOCK_ANY,
        index_p, &l3_key, l3_entry_p, 0);
}

/*
 * Function:
 *      _bcm_en_oam_lm_search
 * Purpose:
 *      Find how may LM endpoints have been created with the same port and VLAN
 * Parameters:
 *      unit - (IN) Unit number.
 *      current_endpoint_p - (IN) Pointer to an OAM endpoint structure
 *      add - (IN) 1 = create,  0 = destroy
 *      mdl_index - (OUT) Pointer to a sorted endpoint index
 * Returns:
 *      Number of endpoints that need to recreate FP entries
 * Notes:
 */
static int _bcm_en_oam_lm_search(
int unit, _bcm_oam_endpoint_t *current_endpoint_p, int add, int *mdl_index) 
{
    int i, j, k, max_index = 0;
    int level[LEVEL_COUNT];
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_endpoint_t *endpoint_p = NULL;

    if (!current_endpoint_p) {
        return 0;
    }

    for (i = 0; i < LEVEL_COUNT; i++) {
        mdl_index[i] = -1;
        level[i] = LEVEL_COUNT;
    }

    SET_OAM_INFO;

    for (i = 0; i < oam_info_p->endpoint_count; i++) {

        SET_ENDPOINT(i);

        if ((endpoint_p == current_endpoint_p) && (add == 0)) {
            /* Skip current endpoint in destroy case */
            continue;
        }

        if ((endpoint_p->in_use || (add != 0)) &&
           (endpoint_p->glp == current_endpoint_p->glp) &&
           (endpoint_p->vlan == current_endpoint_p->vlan) &&
           (endpoint_p->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT)) {
            /* Different MDL LM endpoints found. Save index in MDL order */
            for (j = 0; j < LEVEL_COUNT && mdl_index[j] != -1; j++) {
                if (endpoint_p->level < level[j]) {
                    for (k = max_index; k > j; k--) {
                        if (mdl_index[k-1] != -1) {
                            mdl_index[k] = mdl_index[k-1];
                            level[k] = level[k-1];
                        }
                    }
                    mdl_index[j] = i;
                    level[j] = endpoint_p->level;
                    break;
                } else if ((j == (max_index-1)) && (j < MAX_ENDPOINT_LEVEL)) {
                    mdl_index[j+1] = i;
                    level[j+1] = endpoint_p->level;
                    break;
                }
            }
            /* Use the same LM counter index */
            if ((endpoint_p != current_endpoint_p) && (add != 0)) {
                current_endpoint_p->lm_counter_index = endpoint_p->lm_counter_index;
                current_endpoint_p->pri_map_index = endpoint_p->pri_map_index;
            }

            /* Save first element for destroy case */
            if (max_index == 0) {
                mdl_index[0] = i;
                level[0] = endpoint_p->level;
            }
            max_index++;
            /* Destory entry and reinstall later */
            if (endpoint_p != current_endpoint_p) {
                _bcm_en_oam_uninstall_fp(unit, endpoint_p);
            }
        }
    }

    return max_index;
}

static int _bcm_en_oam_delete_lmep(int unit,
    _bcm_oam_endpoint_t *endpoint_p)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_endpoint_t *current_endpoint_p;
    int i, j, l3_index, num_lm, mdl_index[LEVEL_COUNT];
    l3_entry_ipv4_unicast_entry_t l3_entry;
    uint32 level_bitmap;
    int result = SOC_E_NONE;
    lmep_entry_t lmep_entry;

    SET_OAM_INFO;

    if (endpoint_p->local_tx_enabled)
    {
        /* LMEP entry */

        sal_memset(&lmep_entry, 0, sizeof(lmep_entry_t)); 
        WRITE_LMEPm(unit, MEM_BLOCK_ALL,
         endpoint_p->local_tx_index, &lmep_entry); 

        /* Local control block */

        SHR_BITCLR(oam_info_p->local_tx_endpoints_in_use,
            endpoint_p->local_tx_index);
    }

    if (endpoint_p->local_rx_enabled)
    {
        /* MA_INDEX entry doesn't need to be touched */

        /* L3 entry */

        L3_LOCK(unit);

        if (BCM_SUCCESS(_bcm_en_oam_find_lmep(unit, endpoint_p->vlan,
            endpoint_p->glp, &l3_index, &l3_entry)) || 
            BCM_SUCCESS(_bcm_en_oam_find_lmep(unit, 0,
            endpoint_p->vp, &l3_index, &l3_entry)))
        {
            level_bitmap = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit,
                &l3_entry, LMEP__MDL_BITMAPf);

            level_bitmap &= ~(1 << endpoint_p->level);

            if (level_bitmap != 0)
            {
                /* Still endpoints here at other levels */

                result = soc_mem_field32_modify(unit, L3_ENTRY_IPV4_UNICASTm,
                    l3_index, LMEP__MDL_BITMAPf, level_bitmap);
            }
            else
            {
                /* No endpoints left here at any level */

                result = soc_mem_field32_modify(unit, L3_ENTRY_IPV4_UNICASTm,
                    l3_index, VALIDf, 0);
            }
        }

        L3_UNLOCK(unit);

        SOC_IF_ERROR_RETURN(result);

        /* Local control block */
        SHR_BITCLR(oam_info_p->local_rx_endpoints_in_use,
            endpoint_p->local_rx_index);
            
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANAX(unit)) {
            result = soc_profile_mem_delete(unit, oam_opcode_profile[unit],
                                            endpoint_p->opcode_profile_index);
            endpoint_p->opcode_profile_index = 0;
        }
#endif /* BCM_KATANA_SUPPORT */
        
        _bcm_en_oam_uninstall_vfp(unit, endpoint_p);
        _bcm_en_oam_uninstall_fp(unit, endpoint_p);

        if (endpoint_p->lm_counter_index != BCM_OAM_LM_COUNTER_INDEX_INVALID) {
            num_lm = _bcm_en_oam_lm_search(unit, endpoint_p, 0, mdl_index);
            if (num_lm == 0) {
                SHR_BITCLR(oam_info_p->lm_counter_in_use,
                    endpoint_p->lm_counter_index);
                SOC_IF_ERROR_RETURN(
                    soc_profile_mem_delete(unit, ing_pri_map_profile[unit],
                    endpoint_p->pri_map_index*BCM_OAM_INTPRI_MAX));
            } else {
                for (i = 0; i < num_lm; i++) {
                    current_endpoint_p = oam_info_p->endpoints + mdl_index[i];
                	BCM_IF_ERROR_RETURN(
                	    _bcm_en_oam_install_fp(unit, (i == (num_lm-1)) ? 0 : 1, current_endpoint_p));
                    BCM_IF_ERROR_RETURN(
                        bcm_esw_field_entry_install(unit, current_endpoint_p->fp_entry_tx));
                    BCM_IF_ERROR_RETURN(
                        bcm_esw_field_entry_install(unit, current_endpoint_p->fp_entry_rx));
                    for (j = 0; j < BCM_SWITCH_TRUNK_MAX_PORTCNT; j++) {
                        if (current_endpoint_p->fp_entry_trunk[j]) {
                            BCM_IF_ERROR_RETURN(
                                bcm_esw_field_entry_install(unit, current_endpoint_p->fp_entry_trunk[j]));
                        }
                    }
                }
            }
            endpoint_p->lm_counter_index = BCM_OAM_LM_COUNTER_INDEX_INVALID;
            endpoint_p->pri_map_index = 0;
        }
    }
    return result;
}

#if defined(BCM_KATANA_SUPPORT)
static int _bcm_kt_set_oam_opcode_profile_entry(int unit,
    uint32 flags, uint32 *entry)
{
    uint32 i, opcode;

    for (i=0; i<31; i++) {

        opcode = flags & (1 << i);

        switch (opcode) {
            case BCM_OAM_OPCODE_CCM_IN_HW:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, CCM_PROCESS_IN_HWf, 1);
                break;

            case BCM_OAM_OPCODE_CCM_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, CCM_COPYTO_CPUf, 1);
                break;

            case BCM_OAM_OPCODE_CCM_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, CCM_DROPf, 1);
                break;

            case BCM_OAM_OPCODE_LBM_IN_HW:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, LBM_PROCESS_IN_HWf, 1);
                break;

            case BCM_OAM_OPCODE_LBM_UC_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, LBM_UC_COPYTO_CPUf, 1);
                break;
            case BCM_OAM_OPCODE_LBM_UC_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, LBM_UC_DROPf, 1);
                break;

            case BCM_OAM_OPCODE_LBM_MC_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, LBM_MC_COPYTO_CPUf, 1);
                break;

            case BCM_OAM_OPCODE_LBM_MC_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, LBM_MC_DROPf, 1);
                break;

            case BCM_OAM_OPCODE_LBR_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, LBR_COPYTO_CPUf, 1);
                break;

            case BCM_OAM_OPCODE_LBR_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, LBR_DROPf, 1);
                break;

            case BCM_OAM_OPCODE_LTM_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, LTM_COPYTO_CPUf, 1);
                break;

            case BCM_OAM_OPCODE_LTM_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, LTM_DROPf, 1);
                break;
            case BCM_OAM_OPCODE_LTR_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, LTR_COPYTO_CPUf, 1);
                break;

            case BCM_OAM_OPCODE_LTR_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, LTR_DROPf, 1);
                break;

            case BCM_OAM_OPCODE_LMEP_PKT_FWD:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, FWD_LMEP_PKTf, 1);
                break;

            case BCM_OAM_OPCODE_OTHER_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, OTHER_OPCODE_COPYTO_CPUf, 1);
                break;

            case BCM_OAM_OPCODE_OTHER_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry, OTHER_OPCODE_DROPf, 1);
                break;

            default:
                break;
        }
    }

    return BCM_E_NONE;
}

static int _bcm_kt_get_default_oam_opcode_profile_entry(int unit,
                                        uint32 *entry)
{
    uint32 opcode_flags;

    opcode_flags =  BCM_OAM_OPCODE_CCM_COPY_TO_CPU |
                    BCM_OAM_OPCODE_CCM_DROP |
                    BCM_OAM_OPCODE_LBM_UC_COPY_TO_CPU |
                    BCM_OAM_OPCODE_LBM_UC_DROP |
                    BCM_OAM_OPCODE_LBM_MC_COPY_TO_CPU |
                    BCM_OAM_OPCODE_LBM_MC_DROP |
                    BCM_OAM_OPCODE_LBR_COPY_TO_CPU |
                    BCM_OAM_OPCODE_LBR_DROP |
                    BCM_OAM_OPCODE_LTM_COPY_TO_CPU |
                    BCM_OAM_OPCODE_LTM_DROP |
                    BCM_OAM_OPCODE_LTR_COPY_TO_CPU |
                    BCM_OAM_OPCODE_LTR_DROP |
                    BCM_OAM_OPCODE_OTHER_COPY_TO_CPU |
                    BCM_OAM_OPCODE_OTHER_DROP;

    _bcm_kt_set_oam_opcode_profile_entry(unit, opcode_flags, entry);

    return BCM_E_NONE;
}

#endif /* BCM_KATANA_SUPPORT */

/*
 * Function:
 *      _bcm_en_oam_destroy_endpoint
 * Purpose:
 *      Destroy an OAM endpoint object
 * Parameters:
 *      unit - (IN) Unit number.
 *      endpoint_p - (IN) Pointer to an OAM endpoint structure
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static int _bcm_en_oam_destroy_endpoint(int unit,
					_bcm_oam_endpoint_t *endpoint_p)
{
#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_BHH)
    _bcm_oam_info_t *oam_info_p;
    uint16 reply_len;
    int sess_id = 0;

    SET_OAM_INFO;
#endif

    if (endpoint_p->type == bcmOAMEndpointTypeEthernet) 
    {
	if (endpoint_p->is_remote)
	{
	    BCM_IF_ERROR_RETURN(_bcm_en_oam_delete_rmep(unit, endpoint_p));
	}
	else
	{
	    BCM_IF_ERROR_RETURN(_bcm_en_oam_delete_lmep(unit, endpoint_p));
	}
    } 
    /*
     * BHH specific
     */
    else if (soc_feature(unit, soc_feature_bhh) && 
        ((endpoint_p->type == bcmOAMEndpointTypeBHHMPLS) ||
        (endpoint_p->type == bcmOAMEndpointTypeBHHMPLSVccv)))
    {
#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_BHH)
	if (endpoint_p->is_remote) {
	    /* 
	     * BHH uses same index for local and remote.  So, delete always goes through
         * local endpoint destory
	     */
         return BCM_E_NONE;
	} else {
        sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(oam_info_p, endpoint_p->bhh_endpoint_index);
	    /* 
	     * Delete BHH Session in HW 
	     */
	    BCM_IF_ERROR_RETURN
		(_bcm_en_oam_bhh_session_hw_delete(unit, endpoint_p));

	    /* 
	     * Send BHH Session Delete message to uC 
	     */
	    BCM_IF_ERROR_RETURN
		(_bcm_en_oam_bhh_msg_send_receive(unit,
						  MOS_MSG_SUBCLASS_BHH_SESS_DELETE,
						  sess_id, 0,
						  MOS_MSG_SUBCLASS_BHH_SESS_DELETE_REPLY,
						  &reply_len));

	    if (reply_len != 0) {
		return BCM_E_INTERNAL;
	    }
	}

	endpoint_p->bhh_endpoint_index = -1;
#else 
    return (BCM_E_UNAVAIL);
#endif
    }
    else {
        return (BCM_E_PARAM);
    }

    sal_memset(endpoint_p,0,sizeof(_bcm_oam_endpoint_t));
    endpoint_p->in_use = 0;
    endpoint_p->lm_counter_index = BCM_OAM_LM_COUNTER_INDEX_INVALID;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_en_oam_create_endpoint
 * Purpose:
 *      Create or replace an OAM endpoint object
 * Parameters:
 *      unit - (IN) Unit number.
 *      endpoint_info - (IN/OUT) Pointer to an OAM endpoint structure
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static int 
_bcm_en_oam_create_endpoint(
    int unit, 
    bcm_oam_endpoint_info_t *endpoint_info,
    _bcm_oam_endpoint_t **created_endpoint_p,
    int *num_lm, int *mdl_index)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_group_t *group_p;
    _bcm_oam_endpoint_t *endpoint_p, *current_endpoint_p;
    int i, replace, mdl_set = 0, replace_l3_entry = 0;
    int is_remote, is_local;
    int local_tx_enabled = 0;
    int local_rx_enabled = 0;
    int endpoint_index = 0;
    bcm_module_t module_id;
    bcm_port_t port_id;
    bcm_trunk_t trunk_id;
    int local_id;
    uint32 vp = 0;
    rmep_entry_t rmep_entry;
    lmep_entry_t lmep_entry;
    lmep_1_entry_t lmep1_entry;
    ma_index_entry_t ma_index_entry;
    l3_entry_ipv4_unicast_entry_t l3_entry, l3_key;
    int quantization_index;
    int l3_index;
    uint32 level_bitmap, mem_entries[BCM_OAM_INTPRI_MAX], temp_index;
    int result = SOC_E_NONE;
    void *entries[1];
    uint32 oam_current_time;
#if defined(BCM_KATANA_SUPPORT)
    uint32 opcode_entry = 0;
#if defined(INCLUDE_BHH)
    shr_bhh_msg_ctrl_sess_set_t msg_sess;
    shr_bhh_msg_ctrl_sess_enable_t msg_sess_enable;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int encap = 0;
    uint32 session_flags;
#endif /* INCLUDE_BHH */
#endif /* BCM_KATANA_SUPPORT */
    int rv = BCM_E_NONE;
    uint32 src_glp = _BCM_OAM_INVALID_INDEX;
    uint32 dst_glp = _BCM_OAM_INVALID_INDEX;
    bcm_trunk_member_t *member_array = NULL;
    int member_count;
    bcm_gport_t member_gport;
    bcm_trunk_t tgid_out;
    int id_out;
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
    int eth_oam_endpoint_count = 0;
#endif
    int endpoint_start_index = 0;
    int endpoint_end_index = 0;

    SET_OAM_INFO;

    CHECK_INIT;

    VALIDATE_GROUP_INDEX(endpoint_info->group);

    if (endpoint_info->level > MAX_ENDPOINT_LEVEL)
    {
        /* Exceeds supported MDL level 0 - 7. */
        return BCM_E_PARAM;
    }

    if (endpoint_info->flags & UNSUPPORTED_ENDPOINT_FLAGS)
    {
        return BCM_E_UNAVAIL;
    }

    if (endpoint_info->type != bcmOAMEndpointTypeEthernet)
    {
#if defined(BCM_ENDURO_SUPPORT)
        if (SOC_IS_ENDURO(unit))
        {
            /* Device supports Ethernet OAM only. */
            return BCM_E_UNAVAIL;
        }
#endif
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
        if ((endpoint_info->type != bcmOAMEndpointTypeBHHMPLS) &&
            (endpoint_info->type != bcmOAMEndpointTypeBHHMPLSVccv))
#endif
        {
            return BCM_E_UNAVAIL;
        }
    } 

    replace = ((endpoint_info->flags & BCM_OAM_ENDPOINT_REPLACE) != 0);

    if (endpoint_info->flags & BCM_OAM_ENDPOINT_REMOTE)
    {
        if (endpoint_info->flags &
            (BCM_OAM_ENDPOINT_CCM_RX | BCM_OAM_ENDPOINT_LOOPBACK |
	     BCM_OAM_ENDPOINT_DELAY_MEASUREMENT | BCM_OAM_ENDPOINT_LINKTRACE |
	     BCM_OAM_ENDPOINT_LOSS_MEASUREMENT | 
	     BCM_OAM_ENDPOINT_PORT_STATE_TX | 
	     BCM_OAM_ENDPOINT_INTERFACE_STATE_TX ))
        {
            /* Specified flags aren't valid for RMEPs */

            return BCM_E_PARAM;
        }

        is_remote = 1;
    }
    else
    {
        /* Local */
        is_remote = 0;

        local_tx_enabled = 
            (endpoint_info->ccm_period != BCM_OAM_ENDPOINT_CCM_PERIOD_DISABLED);

        local_rx_enabled = ((endpoint_info->flags &
			     (BCM_OAM_ENDPOINT_CCM_RX | BCM_OAM_ENDPOINT_LOOPBACK |
			      BCM_OAM_ENDPOINT_DELAY_MEASUREMENT |
			      BCM_OAM_ENDPOINT_LOSS_MEASUREMENT |
			      BCM_OAM_ENDPOINT_LINKTRACE)) != 0);
    }

    if (soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
    eth_oam_endpoint_count = oam_info_p->remote_endpoint_count +
        oam_info_p->local_tx_endpoint_count + oam_info_p->local_rx_endpoint_count;
#endif
    }

    if (endpoint_info->flags & BCM_OAM_ENDPOINT_WITH_ID)
    {
        endpoint_index = endpoint_info->id;

        if (soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
        if ((endpoint_info->type == bcmOAMEndpointTypeEthernet) &&
            (endpoint_index >= eth_oam_endpoint_count))
        {
            return BCM_E_PARAM;
        }
        else if (((endpoint_info->type == bcmOAMEndpointTypeBHHMPLS) ||
                  (endpoint_info->type == bcmOAMEndpointTypeBHHMPLSVccv)) &&
                 (endpoint_index < eth_oam_endpoint_count))
        {
            return BCM_E_PARAM;
        }
#endif
        }

        VALIDATE_ENDPOINT_INDEX(endpoint_index);

        if (replace && (!oam_info_p->endpoints[endpoint_index].in_use))
        {
            return BCM_E_NOT_FOUND;
        }
        else if (!replace && (oam_info_p->endpoints[endpoint_index].in_use))
        {
            return BCM_E_EXISTS;
        }
    }
    else
    {
        if (replace)
        {
            /* Replace specified with no ID */

            return BCM_E_PARAM;
        }

        endpoint_start_index = 0;
        endpoint_end_index = oam_info_p->endpoint_count;

        if (soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
        if(endpoint_info->type == bcmOAMEndpointTypeEthernet)
        {
            endpoint_start_index = 0;
            endpoint_end_index = eth_oam_endpoint_count;
        }
        else
        {
            endpoint_start_index = eth_oam_endpoint_count;
            endpoint_end_index = oam_info_p->endpoint_count;
        }
#endif
        }

        if (soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
        /* BHH uses local and remote same index */
        if((endpoint_info->type != bcmOAMEndpointTypeEthernet) &&
            (endpoint_info->flags & BCM_OAM_ENDPOINT_REMOTE))
        {
            endpoint_index = endpoint_info->local_id;
        }
#endif
        }

        /*
         * Non-remote BHH EP.  Get new ID 
         */
        if(!endpoint_index) {
            for (endpoint_index = endpoint_start_index; endpoint_index < endpoint_end_index;
	            ++endpoint_index)
            {
                if (!oam_info_p->endpoints[endpoint_index].in_use)
                {
                    break;
                }
            }

            if (endpoint_index >= oam_info_p->endpoint_count)
            {
                return BCM_E_FULL;
            }
        
            endpoint_info->id = endpoint_index;

        }
    }

    SET_ENDPOINT(endpoint_index);
    *created_endpoint_p = endpoint_p;
    SET_GROUP(endpoint_info->group);
    endpoint_p->type = endpoint_info->type;

    if (!group_p->in_use)
    {
        /* Associating to a nonexistent group */

        return BCM_E_PARAM;
    }

    /* Get Trunk ID or (Modid + Port) value from Gport */
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, endpoint_info->gport, &module_id,
                                &port_id, &trunk_id, &local_id));

    /* 
     * If Gport is Trunk type, _bcm_esw_gport_resolve() 
     * sets trunk_id. Using Trunk ID, get Dst Modid and Port value.
     */
    if (BCM_GPORT_IS_TRUNK(endpoint_info->gport)) {

        if (BCM_TRUNK_INVALID == trunk_id)  {
            /* Has to be a valid Trunk. */
            return (BCM_E_PARAM);
        }

        /* 
         * CCM Tx is enabled on a trunk member port.
         * trunk_index value is required to derive the Modid and Port info.
         */
        if (1 == local_tx_enabled
            && _BCM_OAM_INVALID_INDEX == endpoint_info->trunk_index) {
            /* Invalid Trunk member index passed. */
            return (BCM_E_PORT);
        }

        /* Get Trunk Info for the Trunk ID. */
        BCM_IF_ERROR_RETURN
            (bcm_esw_trunk_get(unit, trunk_id, NULL, 0, NULL, &member_count));
        if (0 == member_count) {
            /* No members have been added to the trunk group yet */
            return BCM_E_PARAM;
        }
        member_array = sal_alloc(sizeof(bcm_trunk_member_t) * member_count,
                "trunk member array");
        if (NULL == member_array) {
            return BCM_E_MEMORY;
        }
        rv = bcm_esw_trunk_get(unit, trunk_id, NULL,
                member_count, member_array, &member_count);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return rv;
        }

        /* Get the Modid and Port value using Trunk Index value. */
        if (endpoint_info->trunk_index >= member_count) {
            sal_free(member_array);
            return BCM_E_PARAM;
        }
        member_gport = member_array[endpoint_info->trunk_index].gport;
        sal_free(member_array);
        BCM_IF_ERROR_RETURN
            (_bcm_esw_gport_resolve(unit, member_gport, &module_id, &port_id,
                                    &tgid_out, &id_out));
        if ((-1 != tgid_out) || (-1 != id_out)) {
            return BCM_E_PARAM;
        }

        /* Calculate the SGLP and DGLP values to program the MEP */
        src_glp = ((1 << SOC_TRUNK_BIT_POS(unit)) | trunk_id);
        dst_glp = (module_id << SOC_MODID_BIT_POS(unit)) | port_id;
    }

    /* 
     * Application can resolve the trunk and pass the desginated
     * port as Gport value. Check if the Gport belongs to a trunk.
     */
    if ((BCM_TRUNK_INVALID == trunk_id)
        && (BCM_GPORT_IS_MODPORT(endpoint_info->gport)
        || BCM_GPORT_IS_LOCAL(endpoint_info->gport))) {

        /* When Gport is ModPort or Port type, _bcm_esw_gport_resolve()
         * returns Modid and Port value. Use these values to make the DGLP
         * value.
         */
        dst_glp = ((module_id << SOC_MODID_BIT_POS(unit)) | port_id);

        /* Use the Modid, Port value and determine if the port
         * belongs to a Trunk.
         */
        rv = bcm_esw_trunk_find(unit, module_id, port_id, &trunk_id);
        if (BCM_SUCCESS(rv)) {
            /* 
             * Port is member of a valid trunk.
             * Now create the SGLP value from Trunk ID.
             */
            src_glp = (1 << SOC_TRUNK_BIT_POS(unit)) | trunk_id;
        } else {
            /* Port not a member of trunk. DGLP and SGLP are the same. */
            src_glp = dst_glp;
        }
    }

    if (BCM_GPORT_IS_MIM_PORT(endpoint_info->gport) ||
        BCM_GPORT_IS_MPLS_PORT(endpoint_info->gport)) {
        vp = local_id;
        result = _bcm_esw_modid_is_local(unit, module_id, &is_local);
        if (is_local) {
            src_glp = dst_glp = ((module_id << SOC_MODID_BIT_POS(unit)) | port_id);
        } else {
            return BCM_E_PARAM;
        }
    }

    /* 
     * At this point, both src_glp and dst_glp should be valid.
     * Gport types other than TRUNK, MODPORT or LOCAL are not valid.
     */
    if (_BCM_OAM_INVALID_INDEX == src_glp
        || _BCM_OAM_INVALID_INDEX == dst_glp) {
        return (BCM_E_PORT);
    }

    quantization_index =
        _bcm_en_oam_quantize_ccm_period(endpoint_info->ccm_period);

 
    /*
     * Ethernet
     */
    if (endpoint_info->type == bcmOAMEndpointTypeEthernet)
    {  
	if (is_remote)
	{
	    if (replace && !endpoint_p->is_remote) {
		    /* Replacing Local to Remote, Delete existing LMEP */
		    BCM_IF_ERROR_RETURN(_bcm_en_oam_delete_lmep(unit, endpoint_p));
	    }

	    if ( !(replace) || (replace && !endpoint_p->is_remote) ) {
            /* Creating new Remote. Hence create new RMEP entry.
               Else if replace operation, reuse existing RMEP */
            endpoint_p->remote_index = _bcm_en_oam_find_free_endpoint(
                                       oam_info_p->remote_endpoints_in_use,
                                       oam_info_p->remote_endpoint_count, 1, 0);

            if (endpoint_p->remote_index < 0) {
                return BCM_E_FULL;
            }

            /* RMEP entry */

            sal_memset(&rmep_entry, 0, sizeof(rmep_entry_t));

            soc_RMEPm_field32_set(unit, &rmep_entry, MAID_INDEXf,
                      endpoint_info->group);

            /* 
             * The following steps are necessary to enable CCM timeout events
             * without having received any CCMs.
             */
            soc_RMEPm_field32_set(unit, &rmep_entry, RMEP_TIMESTAMP_VALIDf, 1);
            BCM_IF_ERROR_RETURN(READ_OAM_CURRENT_TIMEr(unit, &oam_current_time));
            soc_RMEPm_field32_set(unit, &rmep_entry, RMEP_TIMESTAMPf,
                                  oam_current_time);
            soc_RMEPm_field32_set(unit, &rmep_entry, RMEP_RECEIVED_CCMf,
                                  quantization_index);

            /* End of timeout setup */
            
            soc_RMEPm_field32_set(unit, &rmep_entry, VALIDf, 1);

            SOC_IF_ERROR_RETURN(WRITE_RMEPm(unit, MEM_BLOCK_ALL,
                                endpoint_p->remote_index, &rmep_entry));
        }

	    /* L3 entry */

	    if (replace && endpoint_p->is_remote) {
            if (vp != 0) {
                _bcm_en_oam_make_rmep_key(unit, &l3_key, endpoint_p->name,
                                          endpoint_p->level, 0, endpoint_p->vp);
            } else {
                _bcm_en_oam_make_rmep_key(unit, &l3_key, endpoint_p->name,
                        endpoint_p->level, endpoint_p->vlan, endpoint_p->glp);
            }
            rv = soc_mem_search(unit, L3_ENTRY_IPV4_UNICASTm, MEM_BLOCK_ANY,
                        &l3_index, &l3_key, &l3_entry, 0);
            /* See if key not changed. */
            if ((endpoint_info->name == endpoint_p->name) &&
                (endpoint_info->level == endpoint_p->level) &&
                (((endpoint_info->vlan == endpoint_p->vlan) &&
                (src_glp == endpoint_p->glp)) || (vp == endpoint_p->vp))) {
                if (BCM_E_NONE == rv) {
                    /* Replace current entry */
                    replace_l3_entry = 1;
                }
            } else {
                /* Delete replaced L3 RMEP. */
                SOC_IF_ERROR_RETURN(soc_mem_delete(unit, L3_ENTRY_IPV4_UNICASTm,
                                    MEM_BLOCK_ALL, &l3_key));
            }
	    }

	    sal_memset(&l3_entry, 0, sizeof(l3_entry_ipv4_unicast_entry_t));

	    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__CCMf,
						   quantization_index);

	    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__RMEP_PTRf,
						   endpoint_p->remote_index);

	    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__MEPIDf,
						   endpoint_info->name);

	    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__MDLf,
						   endpoint_info->level);

	    if (vp) {
#if defined(BCM_KATANA_SUPPORT)
		if (SOC_IS_KATANAX(unit)) {
		    if(endpoint_info->flags & BCM_OAM_ENDPOINT_PBB_TE) {
			soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry,
							       RMEP__ENTRY_IS_SVPf,
							       1);
		    }
		}
#endif /* BCM_KATANA_SUPPORT */
		soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__VIDf, 0);
		soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__SGLPf, vp);
	    } else {
		soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__VIDf,
						       endpoint_info->vlan);
		soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__SGLPf, src_glp);
	    }

	    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, KEY_TYPEf,
						   TR_L3_HASH_KEY_TYPE_RMEP);

	    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, VALIDf, 1);

	    if (replace_l3_entry) {
		SOC_IF_ERROR_RETURN(
		    soc_mem_write(unit, L3_ENTRY_IPV4_UNICASTm, MEM_BLOCK_ALL,
				  l3_index, &l3_entry));
	    } else {
		SOC_IF_ERROR_RETURN(
		    soc_mem_insert(unit, L3_ENTRY_IPV4_UNICASTm, MEM_BLOCK_ALL, &l3_entry));
	    }

	    /* Local control block */

	    SHR_BITSET(oam_info_p->remote_endpoints_in_use,
		       endpoint_p->remote_index);

	    oam_info_p->remote_endpoints[endpoint_p->remote_index] =
		endpoint_index;
	}
	else
	{
	    if (replace && endpoint_p->is_remote) {
		/* Delete original RMEP. */
		BCM_IF_ERROR_RETURN(_bcm_en_oam_delete_rmep(unit, endpoint_p));
	    }

	    if (local_tx_enabled)
	    {
		uint32 reversed_maid[BCM_OAM_GROUP_NAME_LENGTH / 4];
		int word_index;

		if (!(replace && endpoint_p->local_tx_enabled)) {
		    endpoint_p->local_tx_index =
			_bcm_en_oam_find_free_endpoint(oam_info_p->
						       local_tx_endpoints_in_use,
						       oam_info_p->local_tx_endpoint_count, 1, 0);

		    if (endpoint_p->local_tx_index < 0)
		    {
			return BCM_E_FULL;
		    }
		}

		/* LMEP entry */

		sal_memset(&lmep_entry, 0, sizeof(lmep_entry_t));

		soc_LMEPm_field32_set(unit, &lmep_entry, MAID_INDEXf,
				      endpoint_info->group);

		soc_LMEPm_mac_addr_set(unit, &lmep_entry, SAf,
				       endpoint_info->src_mac_address);

		soc_LMEPm_field32_set(unit, &lmep_entry, MDLf, endpoint_info->level);
		soc_LMEPm_field32_set(unit, &lmep_entry, MEPIDf, endpoint_info->name);

		soc_LMEPm_field32_set(unit, &lmep_entry, PRIORITYf,
				      endpoint_info->pkt_pri);

		soc_LMEPm_field32_set(unit, &lmep_entry, VLAN_IDf, endpoint_info->vlan);
		soc_LMEPm_field32_set(unit, &lmep_entry, CCMf, quantization_index);

		soc_LMEPm_field32_set(unit, &lmep_entry, DESTf, dst_glp);

		soc_LMEPm_field32_set(unit, &lmep_entry, MH_OPCODEf,
				      BCM_HG_OPCODE_UC);
            
		soc_LMEPm_field32_set(unit, &lmep_entry, INT_PRIf,
				      endpoint_info->int_pri);

		if (endpoint_info->flags & BCM_OAM_ENDPOINT_PORT_STATE_UPDATE) {
		    if (endpoint_info->port_state > BCM_OAM_PORT_TLV_UP) {
			return BCM_E_PARAM;
		    }
		    soc_LMEPm_field32_set(unit, &lmep_entry, PORT_TLVf,
					  (endpoint_info->port_state == BCM_OAM_PORT_TLV_UP) ? 1 : 0);
		}

		if (endpoint_info->flags & BCM_OAM_ENDPOINT_INTERFACE_STATE_UPDATE) {
		    if (SOC_IS_ENDURO(unit)) {
			if (endpoint_info->interface_state > BCM_OAM_INTERFACE_TLV_DOWN) {
			    return BCM_E_PARAM;
			}
			soc_LMEPm_field32_set(unit, &lmep_entry, INTERFACE_TLVf,
					      (endpoint_info->interface_state == BCM_OAM_INTERFACE_TLV_UP) ? 1 : 0);
		    } else {
			if (endpoint_info->interface_state > BCM_OAM_INTERFACE_TLV_LLDOWN) {
			    return BCM_E_PARAM;
			}
			soc_LMEPm_field32_set(unit, &lmep_entry, INTERFACE_TLVf,
					      endpoint_info->interface_state);
		    }
		}

		if ((endpoint_info->flags & BCM_OAM_ENDPOINT_PORT_STATE_TX) ||
		    (endpoint_info->flags & BCM_OAM_ENDPOINT_INTERFACE_STATE_TX)) {
		    soc_LMEPm_field32_set(unit, &lmep_entry, INSERT_TLVf, 1);
		}

		/* Word-reverse the MAID bytes for the hardware */

		for (word_index = 0;
		     word_index < (BCM_OAM_GROUP_NAME_LENGTH / 4);
		     ++word_index)
		{
		    reversed_maid[word_index] =
			   bcm_htonl(((uint32 *) group_p->name)
                        [((BCM_OAM_GROUP_NAME_LENGTH / 4) - 1) - word_index]);
		}

		soc_LMEPm_field_set(unit, &lmep_entry, MAIDf, reversed_maid);

		SOC_IF_ERROR_RETURN(WRITE_LMEPm(unit, MEM_BLOCK_ALL,
						endpoint_p->local_tx_index, &lmep_entry));

		/* LMEP_1 entry */

		sal_memset(&lmep1_entry, 0, sizeof(lmep_1_entry_t));

		soc_LMEP_1m_mac_addr_set(unit, &lmep1_entry, MACDAf,
					 endpoint_info->dst_mac_address);

		SOC_IF_ERROR_RETURN(WRITE_LMEP_1m(unit, MEM_BLOCK_ALL,
						  endpoint_p->local_tx_index, &lmep1_entry));
            
		/* Local control block */

		SHR_BITSET(oam_info_p->local_tx_endpoints_in_use,
			   endpoint_p->local_tx_index);
	    }

	    if (local_rx_enabled)
	    {
        	if (replace && endpoint_p->local_rx_enabled) {
		    /* Delete replaced L3 LMEP and keep LMEP entry. */
		    result = endpoint_p->local_tx_enabled;
		    endpoint_p->local_tx_enabled = 0;
		    BCM_IF_ERROR_RETURN(_bcm_en_oam_delete_lmep(unit, endpoint_p));
		    endpoint_p->local_tx_enabled = result;
		}

		/* L3 entry */
		L3_LOCK(unit);

		if (BCM_SUCCESS(_bcm_en_oam_find_lmep(unit, endpoint_info->vlan,
						      src_glp, &l3_index, &l3_entry)) ||
		    BCM_SUCCESS(_bcm_en_oam_find_lmep(unit, 0,
						      vp, &l3_index, &l3_entry)))
		{
		    rv = BCM_E_NONE;
		    /*
		     * If there is a  MATCH, check if this MD level is available at this
		     * index. If it's already occupied, report resource error.
		     * Else this MD level is available at this index.
		     */

		    level_bitmap = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit,
									  &l3_entry, LMEP__MDL_BITMAPf);

		    level_bitmap &= (1 << endpoint_info->level);

		    if (!level_bitmap) {
			endpoint_p->local_rx_index =
			    ((soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry,
								     LMEP__MA_BASE_PTRf) << 3) | endpoint_info->level);
			level_bitmap = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit,
									      &l3_entry, LMEP__MDL_BITMAPf);
			level_bitmap |= (1 << endpoint_info->level);
			rv = soc_mem_field32_modify(unit, L3_ENTRY_IPV4_UNICASTm,
						    l3_index, LMEP__MDL_BITMAPf, level_bitmap);
		    } else {
			rv = BCM_E_RESOURCE;
		    }
		} else {
		    rv = BCM_E_NONE;
		    /*
		     * If Entry is NOT found, select a new MA_INDEX for this MDL.
		     */

		    for (i = 0; i < oam_info_p->local_rx_endpoint_count; i += 8)
		    {
			SHR_BITTEST_RANGE(oam_info_p->local_rx_endpoints_in_use, 
					  i, 8, mdl_set);
			if (0 == mdl_set) {
			    break;
			}
		    }
		    if (i >= oam_info_p->local_rx_endpoint_count) {
			rv = BCM_E_FULL;
		    } else {
			endpoint_p->local_rx_index = (i + endpoint_info->level);
		    }

		    sal_memset(&l3_entry, 0, sizeof(l3_entry_ipv4_unicast_entry_t));

		    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry,
							   LMEP__MDL_BITMAPf, 1 << endpoint_info->level);

		    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry,
							   LMEP__MA_BASE_PTRf,
							   endpoint_p->local_rx_index >> LEVEL_BIT_COUNT);
                
		    SET_LMEP_FIELD(unit, LMEP__DM_ENABLEf, endpoint_info->flags,
				   BCM_OAM_ENDPOINT_DELAY_MEASUREMENT);
		    SET_LMEP_FIELD(unit, LMEP__CCM_ENABLEf, endpoint_info->flags,
				   BCM_OAM_ENDPOINT_CCM_RX);
		    SET_LMEP_FIELD(unit, LMEP__LB_ENABLEf, endpoint_info->flags,
				   BCM_OAM_ENDPOINT_LOOPBACK);
		    SET_LMEP_FIELD(unit, LMEP__LT_ENABLEf, endpoint_info->flags,
				   BCM_OAM_ENDPOINT_LINKTRACE);
		    if (vp) {
			soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, LMEP__VIDf,
							       0);

			soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, LMEP__SGLPf,
							       vp);
		    } else {
			soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, LMEP__VIDf,
							       endpoint_info->vlan);

			soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, LMEP__SGLPf,
							       src_glp);
		    }
		    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, KEY_TYPEf,
							   TR_L3_HASH_KEY_TYPE_LMEP);

		    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, VALIDf, 1);

		    rv = soc_mem_insert(unit, L3_ENTRY_IPV4_UNICASTm,
					MEM_BLOCK_ALL, &l3_entry);
		}

		L3_UNLOCK(unit);

		if (rv != BCM_E_NONE && local_tx_enabled)
		{
		    /*
		     * Clean LMEP TX configuration.
		     */
		    sal_memset(&lmep_entry, 0, sizeof(lmep_entry_t));
		    SOC_IF_ERROR_RETURN(WRITE_LMEPm(unit, MEM_BLOCK_ALL,
						    endpoint_p->local_tx_index, &lmep_entry));
		    /*
		     *  Local control block
		     */
		    SHR_BITCLR(oam_info_p->local_tx_endpoints_in_use,
			       endpoint_p->local_tx_index);

		    return rv;
		}

#if defined(BCM_KATANA_SUPPORT)
		if (SOC_IS_KATANAX(unit)) {
		    if (endpoint_info->opcode_flags) {
			SOC_IF_ERROR_RETURN
			    (_bcm_kt_set_oam_opcode_profile_entry(unit,
								  endpoint_info->opcode_flags,
								  &opcode_entry));
		    } else {
			SOC_IF_ERROR_RETURN
			    (_bcm_kt_get_default_oam_opcode_profile_entry(unit,
									  &opcode_entry));
		    }
		    soc_mem_lock(unit, OAM_OPCODE_CONTROL_PROFILEm);
		    entries[0] = &opcode_entry;
		    SOC_IF_ERROR_RETURN
			(soc_profile_mem_add(unit, oam_opcode_profile[unit],
					     (void *) &entries, 1, &temp_index));
		    endpoint_p->opcode_profile_index = temp_index;
		    soc_mem_unlock(unit, OAM_OPCODE_CONTROL_PROFILEm);
		}
#endif /* BCM_KATANA_SUPPORT */

		/* MA_INDEX entry */

		sal_memset(&ma_index_entry, 0, sizeof(ma_index_entry));

		soc_MA_INDEXm_field32_set(unit, &ma_index_entry, MA_PTRf,
					  endpoint_info->group);
#if defined(BCM_KATANA_SUPPORT)
		if (SOC_IS_KATANAX(unit)) {
		    soc_MA_INDEXm_field32_set(unit, &ma_index_entry,
					      OAM_OPCODE_CONTROL_PROFILE_PTRf,
					      endpoint_p->opcode_profile_index);
		    if (endpoint_info->opcode_flags & BCM_OAM_OPCODE_CCM_COPY_TO_CPU) {
			soc_MA_INDEXm_field32_set(unit,&ma_index_entry,
						  CPU_QUEUE_NUMf,
						  endpoint_info->cpu_qid);
		    }
		}
#endif /* BCM_KATANA_SUPPORT */

		SOC_IF_ERROR_RETURN(WRITE_MA_INDEXm(unit, MEM_BLOCK_ALL,
						    endpoint_p->local_rx_index, &ma_index_entry));

		SHR_BITSET(oam_info_p->local_rx_endpoints_in_use,
			   endpoint_p->local_rx_index);
	    } /* local_rx */
	}
	/* Local control block */

	endpoint_p->is_remote = is_remote;
	endpoint_p->local_tx_enabled = local_tx_enabled;
	endpoint_p->local_rx_enabled = local_rx_enabled;
	endpoint_p->group_index = endpoint_info->group;
	endpoint_p->name = endpoint_info->name;
	endpoint_p->level = endpoint_info->level;
	endpoint_p->vlan = endpoint_info->vlan;
	endpoint_p->vp = vp;
	endpoint_p->glp = src_glp;
	endpoint_p->flags = endpoint_info->flags;
#if defined(BCM_KATANA_SUPPORT)
	endpoint_p->opcode_flags = endpoint_info->opcode_flags;
#endif /* BCM_KATANA_SUPPORT */

	if (local_rx_enabled) {
	    if (endpoint_info->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT) {
		*num_lm = _bcm_en_oam_lm_search(unit, endpoint_p, 1, mdl_index);
		if (*num_lm == 1) {
		    /* Assign LM counter index for the first LM endpoint */
		    endpoint_p->lm_counter_index =
			_bcm_en_oam_find_free_endpoint(oam_info_p->lm_counter_in_use,
						       oam_info_p->lm_counter_count, BCM_OAM_LM_COUNTER_OFFSET, 0);

		    if (endpoint_p->lm_counter_index < 0) {
			return BCM_E_FULL;
		    }
        
		    SHR_BITSET(oam_info_p->lm_counter_in_use, 
			       endpoint_p->lm_counter_index);
                    
		    /* Assign Pri Mapping pointer */
		    for (i = 0; i < BCM_OAM_INTPRI_MAX ; i++) {
			mem_entries[i] = endpoint_info->pri_map[i];
			if (SOC_MEM_FIELD_VALID(unit, ING_SERVICE_PRI_MAPm, OFFSET_VALIDf)) {
			    soc_mem_field32_set(unit, ING_SERVICE_PRI_MAPm, &mem_entries[i],
						OFFSET_VALIDf, 1);
			}
		    }
		    soc_mem_lock(unit, ING_SERVICE_PRI_MAPm);
                
		    entries[0] = &mem_entries;
		    SOC_IF_ERROR_RETURN
			(soc_profile_mem_add(unit, ing_pri_map_profile[unit],
					     (void *) &entries, BCM_OAM_INTPRI_MAX, &temp_index));
		    endpoint_p->pri_map_index = temp_index/BCM_OAM_INTPRI_MAX;
		    soc_mem_unlock(unit, ING_SERVICE_PRI_MAPm);
		    BCM_IF_ERROR_RETURN(
			_bcm_en_oam_install_fp(unit, 0, endpoint_p));
		} else {
		    for (i = 0; i < *num_lm; i++) {
			current_endpoint_p = oam_info_p->endpoints + mdl_index[i];
			BCM_IF_ERROR_RETURN(
			    _bcm_en_oam_install_fp(unit,
						   (i == (*num_lm-1)) ? 0 : 1, current_endpoint_p));
		    }
		}
	    }

	    if ((endpoint_info->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT) &&
		!(endpoint_info->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT)) {
		BCM_IF_ERROR_RETURN(
		    _bcm_en_oam_install_fp(unit, 0, endpoint_p));
	    }

	    if (endpoint_info->flags & BCM_OAM_ENDPOINT_PBB_TE) {
		/* Install VFP */
		BCM_IF_ERROR_RETURN(
		    _bcm_en_oam_install_vfp(unit, endpoint_index, endpoint_info));
	    }
	}
    } 
    /*
     * BHH Specific
     */ 
    else if (soc_feature(unit, soc_feature_bhh) && 
        ((endpoint_info->type == bcmOAMEndpointTypeBHHMPLS) || 
         (endpoint_info->type == bcmOAMEndpointTypeBHHMPLSVccv))) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)

	/*
	 * The uKernel is not provisioned until both endpoints (local and remote)
	 * are provisioned in the host.
	 */
	if (is_remote) {
	    /*
	     * Find the local endpoint. If it can't be found then abort.
	     */
	    VALIDATE_ENDPOINT_INDEX(endpoint_info->local_id);

	    if (!oam_info_p->endpoints[endpoint_info->local_id].in_use) {
		return BCM_E_NOT_FOUND;
	    } else if (oam_info_p->endpoints[endpoint_info->local_id].flags & BCM_OAM_ENDPOINT_REMOTE) {
		return BCM_E_EXISTS;
	    }

	    /*
	     * Now that both ends are provisioned the uKernel can be 
	     * configured.
	     */
	    msg_sess_enable.sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(oam_info_p, 
                                                        endpoint_info->local_id);
	    msg_sess_enable.flags = 0;
	    msg_sess_enable.enable = 1;
	    msg_sess_enable.remote_mep_id = endpoint_info->name;

	    /* Pack control message data into DMA buffer */
	    buffer     = oam_info_p->dma_buffer;
	    buffer_ptr = shr_bhh_msg_ctrl_sess_enable_pack(buffer, &msg_sess_enable);
	    buffer_len = buffer_ptr - buffer;

	    /* Send BHH Session Update message to uC */
	    BCM_IF_ERROR_RETURN
		(_bcm_en_oam_bhh_msg_send_receive(unit,
					      MOS_MSG_SUBCLASS_BHH_SESS_ENABLE,
					      buffer_len, 0,
					      MOS_MSG_SUBCLASS_BHH_SESS_ENABLE_REPLY,
					      &reply_len));
	    if (reply_len != 0) {
		return BCM_E_INTERNAL;
	    }

	} else {

        endpoint_p->name = endpoint_info->name;
        endpoint_p->level = endpoint_info->level;
        endpoint_p->vccv_type = endpoint_info->vccv_type;
        endpoint_p->group_index = endpoint_info->group;
        endpoint_p->vlan = endpoint_info->vlan;
        endpoint_p->vpn  = endpoint_info->vpn;
        endpoint_p->vp = vp;
        endpoint_p->glp = src_glp;
	    endpoint_p->egress_if = endpoint_info->intf_id;
	    endpoint_p->cpu_qid   = endpoint_info->cpu_qid;
	    endpoint_p->label     = endpoint_info->mpls_label;
	    endpoint_p->gport     = endpoint_info->gport;
	    endpoint_p->flags     = endpoint_info->flags;
	    endpoint_p->local_tx_enabled = 0;
	    endpoint_p->local_rx_enabled = 0;

	    /* Get local port used for TX BFD packet */
	    BCM_IF_ERROR_RETURN
		(_bcm_esw_modid_is_local(unit, module_id, &is_local));
	    if (!is_local) {    /* HG port */
		BCM_IF_ERROR_RETURN
		    (bcm_esw_stk_modport_get(unit, module_id, &port_id));
	    }


	    /* Create or Update */
	    session_flags = (replace) ? 0 : SHR_BHH_SESS_SET_F_CREATE;

	    /* Set Endpoint config entry */
	    endpoint_p->bhh_endpoint_index = endpoint_index;


	    /* Set Encapsulation in HW */
	    if (!replace || (endpoint_info->flags & BCM_BHH_ENDPOINT_ENCAP_SET)) {
		BCM_IF_ERROR_RETURN
		    (_bcm_en_oam_bhh_encap_hw_set(unit, endpoint_p, module_id,
						  port_id, is_local, endpoint_info->int_pri));
		encap = 1;
	    }


	    /* Set control message data */
	    sal_memset(&msg_sess, 0, sizeof(msg_sess));

	    /*
	     * Set the BHH encapsulation data
	     *
	     * The function _bcm_en_oam_bhh_encap_create() is called first
	     * since this sets some fields in 'endpoint_p' which are
	     * used in the message.
	     */
	    if (encap) {
		BCM_IF_ERROR_RETURN
		    (_bcm_en_oam_bhh_encap_create(unit, 
					      port_id, 
					      endpoint_p,
					      msg_sess.encap_data,
					      &msg_sess.encap_type,
					      &msg_sess.encap_length));
	    }

	    /*
	     * Endpoint can be one of: CCM, LB, LM or DM. The default is CCM,
	     * all others modes must be specified via a config flag. 
	     * 
	     */
	    if (endpoint_info->flags & BCM_OAM_ENDPOINT_LOOPBACK)
		session_flags |= SHR_BHH_SESS_SET_F_LB;
	    else if (endpoint_info->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT)
		session_flags |= SHR_BHH_SESS_SET_F_DM;
	    else if (endpoint_info->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT)
		session_flags |= SHR_BHH_SESS_SET_F_LM;
	    else
		session_flags |= SHR_BHH_SESS_SET_F_CCM;

	    if (endpoint_info->flags & BCM_OAM_ENDPOINT_INTERMEDIATE)
		session_flags |= SHR_BHH_SESS_SET_F_MIP;

	    if (!local_tx_enabled)
		session_flags |= SHR_BHH_SESS_SET_F_PASSIVE;

        msg_sess.sess_id = 
            BCM_OAM_BHH_GET_UKERNEL_EP(oam_info_p, 
                                        endpoint_p->bhh_endpoint_index);
	    msg_sess.flags   = session_flags;

	    if (endpoint_p->level > _BCM_OAM_BHH_MEL_MAX)
		return BCM_E_PARAM;

            msg_sess.mel     = endpoint_p->level;
            msg_sess.mep_id  = endpoint_p->name;
	    sal_memcpy(msg_sess.meg_id, group_p->name, BCM_OAM_GROUP_NAME_LENGTH);

	    msg_sess.period = en_ccm_periods[quantization_index];

	    msg_sess.if_num  = endpoint_info->intf_id;
	    msg_sess.tx_port = port_id;
	    msg_sess.tx_cos  = endpoint_info->int_pri;
	    msg_sess.tx_pri  = endpoint_info->pkt_pri;
	    msg_sess.tx_qnum = SOC_INFO(unit).port_cosq_base[port_id] + endpoint_info->int_pri;

	    msg_sess.mpls_label = endpoint_info->mpls_label;

	    /* Pack control message data into DMA buffer */
	    buffer     = oam_info_p->dma_buffer;
	    buffer_ptr = shr_bhh_msg_ctrl_sess_set_pack(buffer, &msg_sess);
	    buffer_len = buffer_ptr - buffer;

	    /* Send BHH Session Update message to uC */
	    BCM_IF_ERROR_RETURN
		(_bcm_en_oam_bhh_msg_send_receive(unit,
					      MOS_MSG_SUBCLASS_BHH_SESS_SET,
					      buffer_len, 0,
					      MOS_MSG_SUBCLASS_BHH_SESS_SET_REPLY,
					      &reply_len));
	    if (reply_len != 0) {
		return BCM_E_INTERNAL;
	    }
	} /* is_remote */
#else
    return (BCM_E_UNAVAIL);
#endif /* defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH) */
    }
    else {
        return (BCM_E_PARAM);
    }

    endpoint_p->in_use = 1;

    return BCM_E_NONE;
}

int 
bcm_en_oam_endpoint_create(
    int unit, 
    bcm_oam_endpoint_info_t *endpoint_info)
{
    int i, j, rv, num_lm = 0, mdl_index[LEVEL_COUNT];
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_endpoint_t *created_endpoint_p = NULL, *current_endpoint_p;

    rv = _bcm_en_oam_create_endpoint(unit, endpoint_info, &created_endpoint_p,
                                     &num_lm, mdl_index);

    if (BCM_SUCCESS(rv) && (created_endpoint_p != NULL)) {
        if (endpoint_info->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT) {
            if (num_lm == 1) {
                BCM_IF_ERROR_RETURN(
                    bcm_esw_field_entry_install(unit, created_endpoint_p->fp_entry_tx));
                BCM_IF_ERROR_RETURN(
                    bcm_esw_field_entry_install(unit, created_endpoint_p->fp_entry_rx));
                for (j = 0; j < BCM_SWITCH_TRUNK_MAX_PORTCNT; j++) {
                    if (created_endpoint_p->fp_entry_trunk[j]) {
                        BCM_IF_ERROR_RETURN(
                            bcm_esw_field_entry_install(unit, created_endpoint_p->fp_entry_trunk[j]));
                    }
                }
            } else {
                SET_OAM_INFO;
                for (i = 0; i < num_lm; i++) {
                    current_endpoint_p = oam_info_p->endpoints + mdl_index[i];
                    BCM_IF_ERROR_RETURN(
                        bcm_esw_field_entry_install(unit, current_endpoint_p->fp_entry_tx));
                    BCM_IF_ERROR_RETURN(
                        bcm_esw_field_entry_install(unit, current_endpoint_p->fp_entry_rx));
                    for (j = 0; j < BCM_SWITCH_TRUNK_MAX_PORTCNT; j++) {
                        if (current_endpoint_p->fp_entry_trunk[j]) {
                            BCM_IF_ERROR_RETURN(
                                bcm_esw_field_entry_install(unit, current_endpoint_p->fp_entry_trunk[j]));
                        }
                    }
                }
            }
        }

        if ((endpoint_info->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT) &&
            !(endpoint_info->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT)) {
            if (created_endpoint_p->fp_entry_tx) {
                BCM_IF_ERROR_RETURN(
                bcm_esw_field_entry_install(unit, created_endpoint_p->fp_entry_tx));
            }
            if (created_endpoint_p->fp_entry_rx) {
                BCM_IF_ERROR_RETURN(
                bcm_esw_field_entry_install(unit, created_endpoint_p->fp_entry_rx));
            }
            for (j = 0; j < BCM_SWITCH_TRUNK_MAX_PORTCNT; j++) {
                if (created_endpoint_p->fp_entry_trunk[j]) {
                        BCM_IF_ERROR_RETURN(
                            bcm_esw_field_entry_install(unit, created_endpoint_p->fp_entry_trunk[j]));
                }
            }
        }

        if (endpoint_info->flags & BCM_OAM_ENDPOINT_PBB_TE) {
            BCM_IF_ERROR_RETURN(
                bcm_esw_field_entry_install(unit, created_endpoint_p->vfp_entry));
    
        }
    }
    return rv;
}

static int _bcm_en_oam_get_endpoint(int unit, _bcm_oam_info_t *oam_info_p, int endpoint_index,
    _bcm_oam_endpoint_t *endpoint_p, bcm_oam_endpoint_info_t *endpoint_info)
{
    rmep_entry_t rmep_entry;
    lmep_entry_t lmep_entry;
    l3_entry_ipv4_unicast_entry_t l3_key;
    l3_entry_ipv4_unicast_entry_t l3_entry;
    int l3_index;
    int quantization_index = 0;
    bcm_module_t module_id;
    bcm_port_t port_id;
    uint32 param0, param1;
    ing_service_pri_map_entry_t primap_entry;
    bcm_field_entry_t fp_entry;
    int i, rv = BCM_E_NONE;
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
    shr_bhh_msg_ctrl_sess_get_t msg;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int sess_id = 0;
    bcm_l3_egress_t l3_egress;
    bcm_l3_intf_t l3_intf;
#endif

    endpoint_info->id = endpoint_index;
    endpoint_info->group = endpoint_p->group_index;
    endpoint_info->name = endpoint_p->name;
    endpoint_info->level = endpoint_p->level;
    endpoint_info->vlan = endpoint_p->vlan;
    endpoint_info->type = endpoint_p->type;
    endpoint_info->flags = endpoint_p->flags;
#if defined(BCM_KATANA_SUPPORT)
    endpoint_info->opcode_flags = endpoint_p->opcode_flags;
#endif

#if defined(INCLUDE_L3)
    if (endpoint_p->vp != 0) {
        if (_bcm_vp_used_get(unit, endpoint_p->vp, _bcmVpTypeMim)) {
            BCM_GPORT_MIM_PORT_ID_SET(endpoint_info->gport, endpoint_p->vp);
        } else {
            BCM_GPORT_MPLS_PORT_ID_SET(endpoint_info->gport, endpoint_p->vp);
        }
    } else 
#endif /* INCLUDE_L3 */
    {
        if (endpoint_p->glp & (1 << SOC_TRUNK_BIT_POS(unit)))
        {
            /* Trunk */

            BCM_GPORT_TRUNK_SET(endpoint_info->gport, endpoint_p->glp & 0x7F);
        }
        else
        {
            module_id = (endpoint_p->glp & SOC_MODID_MASK(unit)) >> 
                                                      SOC_MODID_BIT_POS(unit);
            port_id = (endpoint_p->glp & SOC_PORT_ADDR_MAX(unit));

            if (module_id != 0)
            {
                /* Modport */

                BCM_GPORT_MODPORT_SET(endpoint_info->gport, module_id,
                    port_id);
            }
            else
            {
                /* Local port */

                BCM_IF_ERROR_RETURN(bcm_esw_port_gport_get(unit, port_id,
                    &(endpoint_info->gport)));
            }
        }
    }

    if (endpoint_p->type == bcmOAMEndpointTypeEthernet)  {
    if (endpoint_p->is_remote)
    {
        endpoint_info->flags |= BCM_OAM_ENDPOINT_REMOTE;

        /* RMEP entry */

        SOC_IF_ERROR_RETURN(READ_RMEPm(unit, MEM_BLOCK_ANY,
                                        endpoint_p->remote_index,
                                        &rmep_entry));

        /* Get endpoint faults and clear presistent faults if Clear is SET */
        BCM_IF_ERROR_RETURN(_bcm_en_oam_read_clear_faults(unit,
                                                            endpoint_p->remote_index,
                                                            en_endpoint_faults,
                                                            RMEPm,
                                                            (uint32 *) &rmep_entry,
                                                            &endpoint_info->faults,
                                                            &endpoint_info->persistent_faults,
                                                            endpoint_info->clear_persistent_faults));
        /* L3 entry */

        _bcm_en_oam_make_rmep_key(unit, &l3_key, endpoint_p->name, 
                                    endpoint_p->level, 
                                    endpoint_p->vlan,
                                    endpoint_p->glp);

        if (BCM_FAILURE(soc_mem_search(unit, L3_ENTRY_IPV4_UNICASTm, MEM_BLOCK_ANY,
            &l3_index, &l3_key, &l3_entry, 0)))
        {
            return BCM_E_INTERNAL;
        }

        quantization_index = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry,
            RMEP__CCMf);
    }
    else
    {
        /* Endpoint is local */

        if (endpoint_p->local_tx_enabled)
        {
            /* LMEP entry */

            SOC_IF_ERROR_RETURN(READ_LMEPm(unit, MEM_BLOCK_ANY,
                endpoint_p->local_tx_index, &lmep_entry));

            soc_LMEPm_mac_addr_get(unit, &lmep_entry, SAf,
                endpoint_info->src_mac_address);

            endpoint_info->pkt_pri = soc_LMEPm_field32_get(unit, &lmep_entry,
                PRIORITYf);

            endpoint_info->int_pri = soc_LMEPm_field32_get(unit, &lmep_entry,
                INT_PRIf);

            quantization_index = soc_LMEPm_field32_get(unit, &lmep_entry,
                CCMf);

            endpoint_info->port_state = 
                soc_LMEPm_field32_get(unit, &lmep_entry, PORT_TLVf) ? 
                BCM_OAM_PORT_TLV_UP : BCM_OAM_PORT_TLV_BLOCKED;

            if (SOC_IS_ENDURO(unit)) {
                endpoint_info->interface_state = 
                    soc_LMEPm_field32_get(unit, &lmep_entry, INTERFACE_TLVf) ? 
                    BCM_OAM_INTERFACE_TLV_UP : BCM_OAM_INTERFACE_TLV_DOWN;
            } else {
                endpoint_info->interface_state = 
                    soc_LMEPm_field32_get(unit, &lmep_entry, INTERFACE_TLVf);
            }

            if (soc_LMEPm_field32_get(unit, &lmep_entry, INSERT_TLVf)) {
                endpoint_info->flags |= (BCM_OAM_ENDPOINT_PORT_STATE_TX |
                                         BCM_OAM_ENDPOINT_INTERFACE_STATE_TX);
            }
        }

        if (endpoint_p->local_rx_enabled)
        {
            /* L3 entry */

            if (BCM_FAILURE(_bcm_en_oam_find_lmep(unit, endpoint_p->vlan,
                endpoint_p->glp, &l3_index, &l3_entry)))
            {
                return BCM_E_INTERNAL;
            }

            GET_LMEP_FIELD(unit, LMEP__DM_ENABLEf, BCM_OAM_ENDPOINT_DELAY_MEASUREMENT);
            GET_LMEP_FIELD(unit, LMEP__CCM_ENABLEf, BCM_OAM_ENDPOINT_CCM_RX);
            GET_LMEP_FIELD(unit, LMEP__LB_ENABLEf, BCM_OAM_ENDPOINT_LOOPBACK);
            GET_LMEP_FIELD(unit, LMEP__LT_ENABLEf, BCM_OAM_ENDPOINT_LINKTRACE);

            if (endpoint_p->fp_entry_tx != 0) {
                fp_entry = endpoint_p->fp_entry_tx;
            } else {
                fp_entry = endpoint_p->fp_entry_rx;
            }

            if (fp_entry != 0) {
                rv = bcm_esw_field_action_get(unit, fp_entry, 
                                          bcmFieldActionOamLmEnable, 
                                          &param0, &param1);
                if (BCM_SUCCESS(rv)) {
                    if (param0 != 0) {
                        endpoint_info->flags |= BCM_OAM_ENDPOINT_LOSS_MEASUREMENT;
                    }
                }
                
                rv = bcm_esw_field_action_get(unit, fp_entry, 
                                          bcmFieldActionOamUpMep, 
                                          &param0, &param1);
                if (BCM_SUCCESS(rv)) {
                    if (param0 != 0) {
                        endpoint_info->flags |= BCM_OAM_ENDPOINT_UP_FACING;
                    }
                }

                rv = bcm_esw_field_action_get(unit, fp_entry, 
                                          bcmFieldActionOamServicePriMappingPtr, 
                                          &param0, &param1);
                if (BCM_SUCCESS(rv)) {
                    for(i = 0; i < BCM_OAM_INTPRI_MAX; i++) {
                        SOC_IF_ERROR_RETURN(READ_ING_SERVICE_PRI_MAPm(unit, 
                                            MEM_BLOCK_ANY,
                                            param0*BCM_OAM_INTPRI_MAX+i,
                                            &primap_entry));
                        endpoint_info->pri_map[i] = 
                         (uint8)soc_ING_SERVICE_PRI_MAPm_field32_get(unit, &primap_entry, OFFSETf);
                    }
                }
            }

            if (endpoint_p->vfp_entry != 0) {
                endpoint_info->flags |= BCM_OAM_ENDPOINT_PBB_TE;
            }
        }
    }
    endpoint_info->ccm_period = en_ccm_periods[quantization_index];
    }

    if (soc_feature(unit, soc_feature_bhh) &&
        ((endpoint_p->type == bcmOAMEndpointTypeBHHMPLS) ||
         (endpoint_p->type == bcmOAMEndpointTypeBHHMPLSVccv))) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(oam_info_p, endpoint_index);

	BCM_IF_ERROR_RETURN
	    (_bcm_en_oam_bhh_msg_send_receive(unit,
					      MOS_MSG_SUBCLASS_BHH_SESS_GET,
					      sess_id, 0,
					      MOS_MSG_SUBCLASS_BHH_SESS_GET_REPLY,
					      &reply_len));

	/* Pack control message data into DMA buffer */
	buffer     = oam_info_p->dma_buffer;
	buffer_ptr = shr_bhh_msg_ctrl_sess_get_unpack(buffer, &msg);
	buffer_len = buffer_ptr - buffer;

	if (reply_len != buffer_len) {
	    return BCM_E_INTERNAL;
	} else {
	    endpoint_info->ccm_period = msg.period;
	    endpoint_info->int_pri = msg.tx_cos;
	    endpoint_info->pkt_pri = msg.tx_pri;
	}

	endpoint_info->intf_id    = endpoint_p->egress_if;
	endpoint_info->cpu_qid    =  endpoint_p->cpu_qid;
	endpoint_info->mpls_label = endpoint_p->label;
	endpoint_info->gport      = endpoint_p->gport;
	endpoint_info->vpn        = endpoint_p->vpn;
	endpoint_info->vccv_type  = endpoint_p->vccv_type;

	/*
	 * Get MAC address
	 */
	bcm_l3_egress_t_init(&l3_egress);
	bcm_l3_intf_t_init(&l3_intf);

	if (BCM_FAILURE
	    (bcm_esw_l3_egress_get(unit, endpoint_p->egress_if, &l3_egress))) {
	    return BCM_E_INTERNAL;
	}

	l3_intf.l3a_intf_id = l3_egress.intf;
	if (BCM_FAILURE(bcm_esw_l3_intf_get(unit, &l3_intf))) {
	    return BCM_E_INTERNAL;
	}

	sal_memcpy(endpoint_info->src_mac_address, l3_intf.l3a_mac_addr, _BHH_MAC_ADDR_LENGTH);
#else
    return BCM_E_UNAVAIL;
#endif
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_en_oam_endpoint_get
 * Purpose:
 *      Get an OAM endpoint object
 * Parameters:
 *      unit - (IN) Unit number.
 *      endpoint - (IN) The ID of the endpoint object to get
 *      endpoint_info - (OUT) Pointer to an OAM endpoint structure to receive the data
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_endpoint_get(
    int unit, 
    bcm_oam_endpoint_t endpoint, 
    bcm_oam_endpoint_info_t *endpoint_info)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_endpoint_t *endpoint_p;

    SET_OAM_INFO;

    CHECK_INIT;

    VALIDATE_ENDPOINT_INDEX(endpoint);

    SET_ENDPOINT(endpoint);

    if (!endpoint_p->in_use)
    {
        return BCM_E_NOT_FOUND;
    }

    return _bcm_en_oam_get_endpoint(unit, oam_info_p, endpoint, endpoint_p,
        endpoint_info);
}

/*
 * Function:
 *      bcm_en_oam_endpoint_destroy
 * Purpose:
 *      Destroy an OAM endpoint object
 * Parameters:
 *      unit - (IN) Unit number.
 *      endpoint - (IN) The ID of the OAM endpoint object to destroy
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_endpoint_destroy(
    int unit, 
    bcm_oam_endpoint_t endpoint)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_endpoint_t *endpoint_p;

    SET_OAM_INFO;

    CHECK_INIT;

    VALIDATE_ENDPOINT_INDEX(endpoint);

    SET_ENDPOINT(endpoint);

    if (!endpoint_p->in_use)
    {
        return BCM_E_NOT_FOUND;
    }

    return _bcm_en_oam_destroy_endpoint(unit, endpoint_p);
}

/*
 * Function:
 *      bcm_en_oam_endpoint_destroy_all
 * Purpose:
 *      Destroy all OAM endpoint objects associated with a given OAM
 *      group
 * Parameters:
 *      unit - (IN) Unit number.
 *      group - (IN) The OAM group whose endpoints should be destroyed
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_endpoint_destroy_all(
    int unit, 
    bcm_oam_group_t group)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_endpoint_t *endpoint_p;
    int endpoint_index;

    SET_OAM_INFO;

    CHECK_INIT;

    VALIDATE_GROUP_INDEX(group);

    for (endpoint_index = 0; endpoint_index < oam_info_p->endpoint_count;
	 ++endpoint_index)
    {
        SET_ENDPOINT(endpoint_index);

        if (endpoint_p->in_use && endpoint_p->group_index == group)
        {
            BCM_IF_ERROR_RETURN(_bcm_en_oam_destroy_endpoint(unit,
                endpoint_p));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_en_oam_endpoint_traverse
 * Purpose:
 *      Traverse the set of OAM endpoints associated with the
 *      specified group, calling a specified callback for each one
 * Parameters:
 *      unit - (IN) Unit number.
 *      group - (IN) The OAM group whose endpoints should be traversed
 *      cb - (IN) A pointer to the callback function to call for each OAM endpoint in the specified group
 *      user_data - (IN) Pointer to user data to supply in the callback
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_endpoint_traverse(
    int unit, 
    bcm_oam_group_t group, 
    bcm_oam_endpoint_traverse_cb cb, 
    void *user_data)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_endpoint_t *endpoint_p;
    int endpoint_index;
    bcm_oam_endpoint_info_t endpoint_info;

    SET_OAM_INFO;

    CHECK_INIT;

    VALIDATE_GROUP_INDEX(group);

    if (cb == NULL)
    {
        return BCM_E_PARAM;
    }

    for (endpoint_index = 0; endpoint_index < oam_info_p->endpoint_count;
        ++endpoint_index)
    {
        SET_ENDPOINT(endpoint_index);

        if (endpoint_p->in_use && endpoint_p->group_index == group)
        {
            bcm_oam_endpoint_info_t_init(&endpoint_info);
            BCM_IF_ERROR_RETURN(_bcm_en_oam_get_endpoint(unit,
		oam_info_p, endpoint_index, endpoint_p, &endpoint_info));
            BCM_IF_ERROR_RETURN(cb(unit, &endpoint_info, user_data));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_en_oam_event_register
 * Purpose:
 *      Register a callback for handling OAM events
 * Parameters:
 *      unit - (IN) Unit number.
 *      event_types - (IN) The set of OAM events for which the specified callback should be called
 *      cb - (IN) A pointer to the callback function to call for the specified OAM events
 *      user_data - (IN) Pointer to user data to supply in the callback
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_event_register(
    int unit, 
    bcm_oam_event_types_t event_types, 
    bcm_oam_event_cb cb, 
    void *user_data)
{
    _bcm_oam_info_t *oam_info_p;
    uint32 result;
    _bcm_oam_event_handler_t *event_handler_p;
    _bcm_oam_event_handler_t *previous_p = NULL;
    bcm_oam_event_type_t event_type;
    uint32 interrupt_control_register_value;
    int update_interrupt_control = 0;

    SET_OAM_INFO;

    CHECK_INIT;

    if (cb == NULL)
    {
        return BCM_E_PARAM;
    }

    SHR_BITTEST_RANGE(event_types.w, 0, bcmOAMEventCount, result);

    if (result == 0)
    {
        return BCM_E_PARAM;
    }

    for (event_handler_p = oam_info_p->event_handler_list_p;
        event_handler_p != NULL;
        event_handler_p = event_handler_p->next_p)
    {
        if (event_handler_p->cb == cb)
        {
            break;
        }

        previous_p = event_handler_p;
    }

    if (event_handler_p == NULL)
    {
        /* This handler hasn't been registered yet */

        event_handler_p = sal_alloc(sizeof(_bcm_oam_event_handler_t),
            "OAM event handler");

        if (event_handler_p == NULL)
        {
            return BCM_E_MEMORY;
        }

        event_handler_p->next_p = NULL;
        event_handler_p->cb = cb;

        SHR_BITCLR_RANGE(event_handler_p->event_types.w, 0, bcmOAMEventCount);

        if (previous_p != NULL)
        {
            previous_p->next_p = event_handler_p;
        }
        else
        {
            oam_info_p->event_handler_list_p = event_handler_p;
        }
    }

    SOC_IF_ERROR_RETURN(READ_CCM_INTERRUPT_CONTROLr(unit,
        &interrupt_control_register_value));

    for (event_type = 0; event_type < bcmOAMEventCount; ++event_type)
    {
        if (SHR_BITGET(event_types.w, event_type))
        {
            if (soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
	    /*
	     * BHH events are generated by the uKernel
	     */
                if ((event_type == bcmOAMEventBHHLBTimeout) ||
                    (event_type == bcmOAMEventBHHLBDiscoveryUpdate) ||
                    (event_type == bcmOAMEventBHHCCMTimeout) ||
                    (event_type == bcmOAMEventBHHCCMState)) {
                    SHR_BITSET(event_handler_p->event_types.w, event_type);
                    ++(oam_info_p->event_handler_count[event_type]);
                    continue;
                }
#endif
            }
            if (!soc_reg_field_valid(unit, CCM_INTERRUPT_CONTROLr,
                                    en_interrupt_enable_fields[event_type].field))
            {
                 continue;
            }
            if (!SHR_BITGET(event_handler_p->event_types.w, event_type))
            {
                /* This handler isn't handling this event yet */
                SHR_BITSET(event_handler_p->event_types.w, event_type);

                ++(oam_info_p->event_handler_count[event_type]);

                if (oam_info_p->event_handler_count[event_type] == 1) {
                    /* This is the first handler for this event */
                    update_interrupt_control = 1;
                    if (soc_reg_field_valid(unit, CCM_INTERRUPT_CONTROLr,
                            en_interrupt_enable_fields[event_type].field)) {
                        soc_reg_field_set(unit, CCM_INTERRUPT_CONTROLr,
                        &interrupt_control_register_value,
                        en_interrupt_enable_fields[event_type].field, 1);
                    }
                }
            }
        }
    }

    event_handler_p->user_data = user_data;

    /* Enable any needed interrupts not yet enabled */

    if (update_interrupt_control)
    {
        SOC_IF_ERROR_RETURN(WRITE_CCM_INTERRUPT_CONTROLr(unit,
            interrupt_control_register_value));
    }

    if (soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
        /* 
         * Update BHH Events mask 
         */
        if (SOC_IS_KATANAX(unit)) {
            return _bcm_en_oam_bhh_event_mask_set(unit);
        }
#endif
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_en_oam_event_unregister
 * Purpose:
 *      Unregister a callback for handling OAM events
 * Parameters:
 *      unit - (IN) Unit number.
 *      event_types - (IN) The set of OAM events for which the specified callback should not be called
 *      cb - (IN) A pointer to the callback function to unregister from the specified OAM events
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_en_oam_event_unregister(
    int unit, 
    bcm_oam_event_types_t event_types, 
    bcm_oam_event_cb cb)
{
     _bcm_oam_info_t *oam_info_p;
    uint32 result;
    _bcm_oam_event_handler_t *event_handler_p;
    _bcm_oam_event_handler_t *previous_p = NULL;
    bcm_oam_event_type_t event_type;
    uint32 interrupt_control_register_value;
    int update_interrupt_control = 0;

    SET_OAM_INFO;

    CHECK_INIT;

    if (cb == NULL)
    {
        return BCM_E_PARAM;
    }

    SHR_BITTEST_RANGE(event_types.w, 0, bcmOAMEventCount, result);

    if (result == 0)
    {
        return BCM_E_PARAM;
    }

    for (event_handler_p = oam_info_p->event_handler_list_p;
        event_handler_p != NULL;
        event_handler_p = event_handler_p->next_p)
    {
        if (event_handler_p->cb == cb)
        {
            break;
        }

        previous_p = event_handler_p;
    }

    if (event_handler_p == NULL)
    {
        return BCM_E_NOT_FOUND;
    }

    SOC_IF_ERROR_RETURN(READ_CCM_INTERRUPT_CONTROLr(unit,
        &interrupt_control_register_value));

    for (event_type = 0; event_type < bcmOAMEventCount; ++event_type)
    {
        if (SHR_BITGET(event_types.w, event_type))
        {
            if(soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
	        /*
	         * BHH events are generated by the uKernel
	         */
	        if ((event_type == bcmOAMEventBHHLBTimeout) ||
                (event_type == bcmOAMEventBHHLBDiscoveryUpdate) ||
                (event_type == bcmOAMEventBHHCCMTimeout) ||
                (event_type == bcmOAMEventBHHCCMState)) {
		    SHR_BITCLR(event_handler_p->event_types.w, event_type);
                --(oam_info_p->event_handler_count[event_type]);
		        continue;
	        }
#endif
            }
            if (!soc_reg_field_valid(unit, CCM_INTERRUPT_CONTROLr,
                                    en_interrupt_enable_fields[event_type].field))
            {
                continue;
            }
            if (oam_info_p->event_handler_count[event_type] > 0 &&
                SHR_BITGET(event_handler_p->event_types.w, event_type))
            {
                /* This handler has been handling this event */

                SHR_BITCLR(event_handler_p->event_types.w, event_type);

                --(oam_info_p->event_handler_count[event_type]);

                if (oam_info_p->event_handler_count[event_type] == 0)
                {
                	/* No more handlers for this event */

                    update_interrupt_control = 1;
                    if (soc_reg_field_valid(unit, CCM_INTERRUPT_CONTROLr,
                        en_interrupt_enable_fields[event_type].field)) {
                        soc_reg_field_set(unit, CCM_INTERRUPT_CONTROLr,
                        &interrupt_control_register_value,
                        en_interrupt_enable_fields[event_type].field, 0);
                    }
                }
            }
        }
    }

    /* Disable any interrupts that lost their last handler */

    if (update_interrupt_control)
    {
        SOC_IF_ERROR_RETURN(WRITE_CCM_INTERRUPT_CONTROLr(unit,
            interrupt_control_register_value));
    }

    SHR_BITTEST_RANGE(event_handler_p->event_types.w, 0, bcmOAMEventCount,
        result);

    if (result == 0)
    {
        /* No more events for this handler to handle */

        if (previous_p != NULL)
        {
            previous_p->next_p = event_handler_p->next_p;
        }
        else
        {
            oam_info_p->event_handler_list_p = event_handler_p->next_p;
        }

        sal_free(event_handler_p);
    }

    if (soc_feature(unit, soc_feature_bhh)) {
#if defined(BCM_KATANA_SUPPORT)  && defined(INCLUDE_BHH)
     /* 
      * Update BHH Events mask 
      */
    if (SOC_IS_KATANAX(unit)) {
	    return _bcm_en_oam_bhh_event_mask_set(unit);
    }
#endif
    }
    
    return BCM_E_NONE;
}

#if defined(BCM_WARM_BOOT_SUPPORT)
int _bcm_en_oam_sync(int unit)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_group_t *group_p;
    int allocation_size = 0;
    int group_index, group_count = 0;
    int stable_size;
    soc_scache_handle_t scache_handle;
    uint8 *oam_scache;

    SET_OAM_INFO;

    CHECK_INIT;

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
    if (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit) || (stable_size == 0)) {
        return BCM_E_NONE;
    }

    /* Determine how many group names need to be stored */

    for (group_index = 0; group_index < oam_info_p->group_count;
        ++group_index)
    {
        if (oam_info_p->groups[group_index].in_use)
        {
            allocation_size += BCM_OAM_GROUP_NAME_LENGTH;
            group_count++;
        }
    }
    allocation_size += sizeof(int);

    /* VFP group, IFP VP group, IFP GLP group */

    allocation_size += 3 * sizeof(bcm_field_group_t);

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_OAM, 0);

    BCM_IF_ERROR_RETURN(_bcm_esw_scache_ptr_get(unit, scache_handle, 1,
        allocation_size, &oam_scache, BCM_WB_DEFAULT_VERSION, NULL));

    if (oam_scache == NULL)
    {
        return BCM_E_MEMORY;
    }

    /* Store the FP groups */
    sal_memcpy(oam_scache, &oam_info_p->vfp_group, sizeof(bcm_field_group_t));
    oam_scache += sizeof(bcm_field_group_t);
    sal_memcpy(oam_scache, &oam_info_p->fp_vp_group, sizeof(bcm_field_group_t));
    oam_scache += sizeof(bcm_field_group_t);
    sal_memcpy(oam_scache, &oam_info_p->fp_glp_group, sizeof(bcm_field_group_t));
    oam_scache += sizeof(bcm_field_group_t);

    /* Store the OAM group names */
    sal_memcpy(oam_scache, &group_count, sizeof(int));
    oam_scache += sizeof(int);

    for (group_index = 0; group_index < oam_info_p->group_count;
        ++group_index)
    {
        SET_GROUP(group_index);

        if (group_p->in_use)
        {
            memcpy(oam_scache, group_p->name, BCM_OAM_GROUP_NAME_LENGTH);

            oam_scache += BCM_OAM_GROUP_NAME_LENGTH;
        }
    }

    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_oam_sw_dump
 * Purpose:
 *     Displays oam information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_en_oam_sw_dump(int unit)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_endpoint_t *endpoint_p;
    int group_idx, endpoint_idx;
    SHR_BITDCL word;
    SET_OAM_INFO;

    LOG_CLI((BSL_META_U(unit,
                        "\nSW Information OAM - Unit %d\n"), unit));
    LOG_CLI((BSL_META_U(unit,
                        "  Group Info    : \n")));

    for (group_idx = 0; group_idx < oam_info_p->group_count; group_idx++) {
        if (oam_info_p->groups[group_idx].in_use) {
            LOG_CLI((BSL_META_U(unit,
                                "Group %d is in use\n"), group_idx));
        }
    }

    LOG_CLI((BSL_META_U(unit,
                        "\n local_tx_endpoints_in_use \n")));
    for (word = 0; word < _SHR_BITDCLSIZE
         (oam_info_p->local_tx_endpoint_count); word++) {
        LOG_CLI((BSL_META_U(unit,
                            " word %d value %x "), word, 
                 oam_info_p->local_tx_endpoints_in_use[word]));
    }

    LOG_CLI((BSL_META_U(unit,
                        "\n local_rx_endpoints_in_use \n")));
    for (word = 0; word < _SHR_BITDCLSIZE
         (oam_info_p->local_rx_endpoint_count); word++) {
        LOG_CLI((BSL_META_U(unit,
                            " word %d value %x "), word, 
                 oam_info_p->local_rx_endpoints_in_use[word]));
    }

    LOG_CLI((BSL_META_U(unit,
                        "\n remote_endpoints_in_use \n")));
    for (word = 0; word < _SHR_BITDCLSIZE
         (oam_info_p->remote_endpoint_count); word++) {
        LOG_CLI((BSL_META_U(unit,
                            " word %d value %x "), word, 
                 oam_info_p->remote_endpoints_in_use[word]));
    }

    LOG_CLI((BSL_META_U(unit,
                        "\n Reverse RMEP lookup \n")));
    for (endpoint_idx = 0; endpoint_idx < oam_info_p->remote_endpoint_count; 
         endpoint_idx++) {
        if (oam_info_p->endpoints
            [oam_info_p->remote_endpoints[endpoint_idx]].in_use) {
            LOG_CLI((BSL_META_U(unit,
                                "RMEP %x \n"), 
                     oam_info_p->remote_endpoints[endpoint_idx]));
        } 
    }

    LOG_CLI((BSL_META_U(unit,
                        "\n lm_counter_in_use \n")));
    for (word = 0; word < _SHR_BITDCLSIZE
         (oam_info_p->lm_counter_count); word++) {
        LOG_CLI((BSL_META_U(unit,
                            " word %d value %x "), word, 
                 oam_info_p->lm_counter_in_use[word]));
    }

    LOG_CLI((BSL_META_U(unit,
                        "\n Endpoint Information \n")));
    for (endpoint_idx = 0; endpoint_idx < oam_info_p->endpoint_count; 
         endpoint_idx++) {
        SET_ENDPOINT(endpoint_idx);
        if (!endpoint_p->in_use) {
            continue;
        }
        LOG_CLI((BSL_META_U(unit,
                            "\n Endpoint index %d\n"), endpoint_idx));
        LOG_CLI((BSL_META_U(unit,
                            "\t Group index %d\n"), endpoint_p->group_index));
        LOG_CLI((BSL_META_U(unit,
                            "\t Name %x\n"), endpoint_p->name));
        LOG_CLI((BSL_META_U(unit,
                            "\t Level %d\n"), endpoint_p->level));
        LOG_CLI((BSL_META_U(unit,
                            "\t VLAN %d\n"), endpoint_p->vlan));
        LOG_CLI((BSL_META_U(unit,
                            "\t GLP %x\n"), endpoint_p->glp));
        LOG_CLI((BSL_META_U(unit,
                            "\t local_tx_enabled %d\n"), endpoint_p->local_tx_enabled));
        LOG_CLI((BSL_META_U(unit,
                            "\t local_rx_enabled %d\n"), endpoint_p->local_rx_enabled));
        LOG_CLI((BSL_META_U(unit,
                            "\t remote_index %d\n"), endpoint_p->remote_index));
        LOG_CLI((BSL_META_U(unit,
                            "\t local_tx_index %d\n"), endpoint_p->local_tx_index));
        LOG_CLI((BSL_META_U(unit,
                            "\t local_rx_index %d\n"), endpoint_p->local_rx_index));
        LOG_CLI((BSL_META_U(unit,
                            "\t VP %x\n"), endpoint_p->vp));
        LOG_CLI((BSL_META_U(unit,
                            "\t lm_counter_index %d\n"), endpoint_p->lm_counter_index));
        LOG_CLI((BSL_META_U(unit,
                            "\t pri_map_index %d\n"), endpoint_p->pri_map_index));
        LOG_CLI((BSL_META_U(unit,
                            "\t vfp_entry %d\n"), endpoint_p->vfp_entry));
        LOG_CLI((BSL_META_U(unit,
                            "\t fp_entry_tx %d\n"), endpoint_p->fp_entry_tx));
        LOG_CLI((BSL_META_U(unit,
                            "\t fp_entry_rx %d\n"), endpoint_p->fp_entry_rx));
    }
    return;
}
#endif
#endif /* defined(BCM_ENDURO) */
