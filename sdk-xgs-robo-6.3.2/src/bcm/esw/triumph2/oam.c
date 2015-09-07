/*
 * $Id: oam.c 1.93 Broadcom SDK $
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

#include <sal/core/libc.h>

#include <soc/defs.h>
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/profile_mem.h>
#include <soc/debug.h>
#include <soc/hash.h>
#include <soc/l3x.h>
#include <soc/triumph2.h>

#include <bcm/l3.h>
#include <bcm/oam.h>

#include <bcm_int/esw/oam.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw/triumph2.h>

#include <bcm_int/esw_dispatch.h>

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)

#define LEVEL_BIT_COUNT 3
#define LEVEL_COUNT (1 << (LEVEL_BIT_COUNT))
#define MAX_ENDPOINT_LEVEL (LEVEL_COUNT - 1)
#define MANGLED_GROUP_NAME_LENGTH (BCM_OAM_GROUP_NAME_LENGTH)

#define UNSUPPORTED_ENDPOINT_FLAGS \
    (BCM_OAM_ENDPOINT_LOSS_MEASUREMENT)

#define CHECK_INIT \
    if (!oam_info_p->initialized) \
    { \
        return BCM_E_INIT; \
    }

#define SET_OAM_INFO oam_info_p = &oam_info[unit];

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


static _bcm_oam_info_t oam_info[BCM_MAX_NUM_UNITS];

static _bcm_oam_fault_t group_faults[] =
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

static _bcm_oam_fault_t endpoint_faults[] =
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

static _bcm_oam_interrupt_t interrupts[] =
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

static soc_field_t interrupt_enable_fields[bcmOAMEventCount] =
{
    /* This must be in the same order as the bcm_oam_event_type_t enum */

    ANY_RMEP_TLV_PORT_DOWN_INT_ENABLEf,
    ANY_RMEP_TLV_PORT_UP_INT_ENABLEf,
    ANY_RMEP_TLV_INTERFACE_DOWN_INT_ENABLEf,
    ANY_RMEP_TLV_INTERFACE_UP_INT_ENABLEf,
    INVALIDf,
    INVALIDf,
    INVALIDf,
    INVALIDf,
    INVALIDf,
    INVALIDf,
    INVALIDf,
    INVALIDf,
    INVALIDf,
    INVALIDf,
    XCON_CCM_DEFECT_INT_ENABLEf,
    ERROR_CCM_DEFECT_INT_ENABLEf,
    SOME_RDI_DEFECT_INT_ENABLEf,
    SOME_RMEP_CCM_DEFECT_INT_ENABLEf,
    INVALIDf,
};

static uint32 ccm_periods[] =
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

static void *_bcm_tr2x_oam_alloc_clear(unsigned int size, char *description)
{
    void *block_p;

    block_p = sal_alloc(size, description);

    if (block_p != NULL)
    {
        sal_memset(block_p, 0, size);
    }

    return block_p;
}

static int _bcm_tr2x_oam_handle_interrupt(int unit, soc_field_t fault_field)
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

    for (interrupt_p = interrupts;
        interrupt_p->status_register != INVALIDr;
        ++interrupt_p)
    {
        if (BCM_FAILURE(soc_reg32_get(unit, interrupt_p->status_register, 
                        REG_PORT_ANY, 0, &interrupt_status)))
        {
            continue;
        }

        if (soc_reg_field_get(unit, interrupt_p->status_register,
            interrupt_status, VALIDf) &&
            oam_info_p->event_handler_count[interrupt_p->event_type] > 0)
        {
            group_index = (interrupt_p->group_index_field != INVALIDf) ?
                soc_reg_field_get(unit, interrupt_p->status_register,
                    interrupt_status, interrupt_p->group_index_field) :
                BCM_OAM_GROUP_INVALID;

            endpoint_index = (interrupt_p->endpoint_index_field != INVALIDf) ?
                soc_reg_field_get(unit, interrupt_p->status_register,
                    interrupt_status, interrupt_p->endpoint_index_field) :
                BCM_OAM_ENDPOINT_INVALID;

            if (endpoint_index != BCM_OAM_ENDPOINT_INVALID)
            {
                /* Find the logical endpoint for this RMEP */

                endpoint_index = oam_info_p->remote_endpoints[endpoint_index];
            }

            flags = 0;
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
                        REG_PORT_ANY, 0, 0)))
        {
            continue;
        }
    }

    return bcm_esw_oam_unlock(unit);
}

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_1                SOC_SCACHE_VERSION(1,1)
#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_1

STATIC
int _bcm_oam_warm_boot(int unit)
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
    int epidx;
    bcm_vlan_t vlan;
    lmep_entry_t lmep_entry;
    rmep_entry_t rmep_entry;
    ma_index_entry_t ma_index_entry;
    uint8 *group_names_p;
    soc_scache_handle_t scache_handle;
    uint16 recovered_ver;
    uint32 alloc_size;

    SET_OAM_INFO;

    /* Calculate the scache memory required for OAM module */
    alloc_size = oam_info_p->group_count * (BCM_OAM_GROUP_NAME_LENGTH);

    /* Get groups */
    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
    if (!SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)
        && (stable_size > alloc_size)) {

        /* Point scache_handle to OAM module */
        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_OAM, 0);

        /* Get the OAM module base scache pointer value */
        BCM_IF_ERROR_RETURN(_bcm_esw_scache_ptr_get(unit, scache_handle,
            FALSE, alloc_size, &group_names_p, BCM_WB_DEFAULT_VERSION,
            &recovered_ver));
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
                 * group_names_p has been initilized in soc_scache_ptr_get 
                 */ 
                 /* coverity[uninit_use_in_call : FALSE] */
                sal_memcpy(group_p->name, group_names_p, BCM_OAM_GROUP_NAME_LENGTH);
                group_names_p += BCM_OAM_GROUP_NAME_LENGTH;
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

                    endpoint_p->glp = soc_L3_ENTRY_IPV4_UNICASTm_field32_get
                                           (unit, &l3_entry, RMEP__SGLPf);

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

                            /* Name is not used for receive-only endpoints */
                            endpoint_p->name = 0xffff;

                            endpoint_p->level = level;

                            endpoint_p->vlan =
                                soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit,
                                    &l3_entry, LMEP__VIDf);

                            endpoint_p->glp = 
                                soc_L3_ENTRY_IPV4_UNICASTm_field32_get
                                           (unit, &l3_entry, LMEP__SGLPf);

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

    /* Upgrade case */
    if (!SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)
        && (stable_size > alloc_size)
        && (recovered_ver < BCM_WB_VERSION_1_1)) {

        /* Set scache_handle to OAM module */
        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_OAM, 0);

        /* realloc to store max OAM group information */
        SOC_IF_ERROR_RETURN(soc_scache_realloc(unit, scache_handle,
            alloc_size));
    }

    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

static void _bcm_tr2x_oam_free_memory(_bcm_oam_info_t *oam_info_p)
{
    sal_free(oam_info_p->remote_endpoints);
    sal_free(oam_info_p->remote_endpoints_in_use);
    sal_free(oam_info_p->local_rx_endpoints_in_use);
    sal_free(oam_info_p->local_tx_endpoints_in_use);
    sal_free(oam_info_p->endpoints);
    sal_free(oam_info_p->groups);
}

static void _bcm_tr2x_oam_event_unregister_all(_bcm_oam_info_t *oam_info_p)
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
 *      bcm_tr2x_oam_init
 * Purpose:
 *      Initialize the OAM subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_tr2x_oam_init(
    int unit)
{
    _bcm_oam_info_t *oam_info_p;
    bcm_oam_endpoint_t endpoint_index;
    bcm_port_t port_index;
    uint32 register_value;
    int result;

    SET_OAM_INFO;

    if (oam_info_p->initialized)
    {
        /* Reset everything */

        _bcm_tr2x_oam_event_unregister_all(oam_info_p);

        BCM_IF_ERROR_RETURN(bcm_tr2x_oam_group_destroy_all(unit));
        _bcm_tr2x_oam_free_memory(oam_info_p);
    }

    oam_info_p->group_count = soc_mem_index_count(unit, MA_STATEm);

    oam_info_p->groups =
        _bcm_tr2x_oam_alloc_clear(oam_info_p->group_count *
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
        oam_info_p->local_tx_endpoint_count +
        oam_info_p->local_rx_endpoint_count;

    oam_info_p->endpoints =
        _bcm_tr2x_oam_alloc_clear(oam_info_p->endpoint_count *
            sizeof(_bcm_oam_endpoint_t), "_bcm_oam_endpoint");

    if (oam_info_p->endpoints == NULL)
    {
        _bcm_tr2x_oam_free_memory(oam_info_p);

        return BCM_E_MEMORY;
    }

    oam_info_p->local_tx_endpoints_in_use =
        _bcm_tr2x_oam_alloc_clear(SHR_BITALLOCSIZE(oam_info_p->
            local_tx_endpoint_count),
        "local_tx_endpoints_in_use");

    if (oam_info_p->local_tx_endpoints_in_use == NULL)
    {
        _bcm_tr2x_oam_free_memory(oam_info_p);

        return BCM_E_MEMORY;
    }

    oam_info_p->local_rx_endpoints_in_use =
        _bcm_tr2x_oam_alloc_clear(SHR_BITALLOCSIZE(oam_info_p->
            local_rx_endpoint_count),
        "local_rx_endpoints_in_use");

    if (oam_info_p->local_rx_endpoints_in_use == NULL)
    {
        _bcm_tr2x_oam_free_memory(oam_info_p);

        return BCM_E_MEMORY;
    }

    oam_info_p->remote_endpoints_in_use =
        _bcm_tr2x_oam_alloc_clear(SHR_BITALLOCSIZE(oam_info_p->
            remote_endpoint_count),
        "remote_endpoints_in_use");

    if (oam_info_p->remote_endpoints_in_use == NULL)
    {
        _bcm_tr2x_oam_free_memory(oam_info_p);

        return BCM_E_MEMORY;
    }

    oam_info_p->remote_endpoints =
        sal_alloc(oam_info_p->remote_endpoint_count *
            sizeof(bcm_oam_endpoint_t), "rmep reverse lookup");

    if (oam_info_p->remote_endpoints == NULL)
    {
        _bcm_tr2x_oam_free_memory(oam_info_p);

        return BCM_E_MEMORY;
    }

    for (endpoint_index = 0;
        endpoint_index < oam_info_p->remote_endpoint_count;
        ++endpoint_index)
    {
        oam_info_p->remote_endpoints[endpoint_index] =
            BCM_OAM_ENDPOINT_INVALID;
    }

    soc_triumph2_oam_handler_register(unit, _bcm_tr2x_oam_handle_interrupt);

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit))
    {
        result = _bcm_oam_warm_boot(unit);
        if (BCM_FAILURE(result)) {
            soc_triumph2_oam_handler_register(unit, NULL);
            _bcm_tr2x_oam_free_memory(oam_info_p);
            return (BCM_E_INTERNAL);
        }
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
            _bcm_tr2x_oam_free_memory(oam_info_p);

            return result;
        }

        /* Common information for outgoing CCM packets */

        result = WRITE_LMEP_COMMON_1r(unit, _BCM_OAM_MAC_DA_UPPER_32);

        if (SOC_FAILURE(result))
        {
            _bcm_tr2x_oam_free_memory(oam_info_p);

            return result;
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
            _bcm_tr2x_oam_free_memory(oam_info_p);

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
            _bcm_tr2x_oam_free_memory(oam_info_p);

            return result;
        }
    }

    oam_info_p->initialized = 1;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2x_oam_detach
 * Purpose:
 *      Shut down the OAM subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_tr2x_oam_detach(
    int unit)
{
    _bcm_oam_info_t *oam_info_p;
    bcm_port_t port_index;

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

    soc_triumph2_oam_handler_register(unit, NULL);

    _bcm_tr2x_oam_event_unregister_all(oam_info_p);

    _bcm_tr2x_oam_free_memory(oam_info_p);

    oam_info_p->initialized = 0;

    return BCM_E_NONE;
}

static void _bcm_tr2x_oam_group_name_mangle(uint8 *name_p,
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
 *      bcm_tr2x_oam_group_create
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
bcm_tr2x_oam_group_create(
    int unit, 
    bcm_oam_group_info_t *group_info)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_group_t *group_p;
    int replace;
    int group_index;
	int copy_to_cpu = 0;
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

    sal_memcpy(group_p->name, group_info->name, BCM_OAM_GROUP_NAME_LENGTH);

    /* MAID_REDUCTION entry */

    _bcm_tr2x_oam_group_name_mangle(group_p->name, mangled_group_name);

    sal_memset(&maid_reduction_entry, 0, sizeof(maid_reduction_entry_t));

    soc_MAID_REDUCTIONm_field32_set(unit, &maid_reduction_entry, REDUCED_MAIDf,
        soc_draco_crc32(mangled_group_name, MANGLED_GROUP_NAME_LENGTH));

    soc_MAID_REDUCTIONm_field32_set(unit, &maid_reduction_entry, SW_RDIf,
        (group_info->flags & BCM_OAM_GROUP_REMOTE_DEFECT_TX) ? 1 : 0);

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

static int _bcm_tr2x_oam_read_clear_faults(int unit, int entry_index,
    _bcm_oam_fault_t *faults, soc_mem_t mem, uint32 *entry_p,
    uint32 *fault_bits_p, uint32 *persistent_fault_bits_p,
    uint32 clear_persistent_fault_bits)
{
    _bcm_oam_fault_t *fault_p;
    uint32 ccm_read_control_reg_value = 0;

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

            if (clear_persistent_fault_bits & fault_p->mask)
            {
                soc_reg_field_set(unit, CCM_READ_CONTROLr,
                    &ccm_read_control_reg_value, BITS_TO_CLEARf,
                    fault_p->clear_sticky_mask);
            }
        }
    }

    /* If any clear bits were set, do the clear now */

    if (ccm_read_control_reg_value != 0)
    {
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

static int _bcm_tr2x_oam_get_group(int unit, int group_index,
    _bcm_oam_group_t *group_p, bcm_oam_group_info_t *group_info)
{
    maid_reduction_entry_t maid_reduction_entry;
    ma_state_entry_t ma_state_entry;

    group_info->id = group_index;

    sal_memcpy(group_info->name, group_p->name, BCM_OAM_GROUP_NAME_LENGTH);

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

    return _bcm_tr2x_oam_read_clear_faults(unit, group_index, group_faults,
        MA_STATEm, (uint32 *) &ma_state_entry, &group_info->faults,
        &group_info->persistent_faults, group_info->clear_persistent_faults);
}

/*
 * Function:
 *      bcm_tr2x_oam_group_get
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
bcm_tr2x_oam_group_get(
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

    return _bcm_tr2x_oam_get_group(unit, group, group_p, group_info);
}

static int _bcm_tr2x_oam_destroy_group(int unit, int group_index,
    _bcm_oam_group_t *group_p)
{
    maid_reduction_entry_t maid_reduction_entry;
    ma_state_entry_t ma_state_entry;

    /* Remove all associated endpoints */

    bcm_tr2x_oam_endpoint_destroy_all(unit, group_index);

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
 *      bcm_tr2x_oam_group_destroy
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
bcm_tr2x_oam_group_destroy(
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

    return _bcm_tr2x_oam_destroy_group(unit, group, group_p);
}

/*
 * Function:
 *      bcm_tr2x_oam_group_destroy_all
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
bcm_tr2x_oam_group_destroy_all(
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
            BCM_IF_ERROR_RETURN(_bcm_tr2x_oam_destroy_group(unit, group_index,
                group_p));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2x_oam_group_traverse
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
bcm_tr2x_oam_group_traverse(
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

            BCM_IF_ERROR_RETURN(_bcm_tr2x_oam_get_group(unit, group_index,
                group_p, &group_info));

            BCM_IF_ERROR_RETURN(cb(unit, &group_info, user_data));
        }
    }

    return BCM_E_NONE;
}

static int _bcm_tr2x_oam_find_free_endpoint(SHR_BITDCL *endpoints,
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
        endpoint_index = _BCM_OAM_INVALID_INDEX;
    }

    return endpoint_index + offset;
}

static int _bcm_tr2x_oam_quantize_ccm_period(int period)
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
            ccm_periods[quantized_period] !=
                _BCM_OAM_ENDPOINT_CCM_PERIOD_UNDEFINED; ++quantized_period)
        {
            if (period < ccm_periods[quantized_period])
            {
                break;
            }
        }

        if (quantized_period > 1)
        {
            if (ccm_periods[quantized_period] ==
                _BCM_OAM_ENDPOINT_CCM_PERIOD_UNDEFINED)
            {
                /* Use the highest defined value */

                --quantized_period;
            }
            else
            {
                if (period - ccm_periods[quantized_period - 1] <
                    ccm_periods[quantized_period] - period)
                {
                    /* Closer to the lower value */

                    --quantized_period;
                }
            }
        }
    }

    return quantized_period;
}

static void _bcm_tr2x_oam_make_rmep_key(int unit,
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

static int _bcm_tr2x_oam_delete_rmep(int unit, _bcm_oam_endpoint_t *endpoint_p)
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

    SOC_IF_ERROR_RETURN(
    soc_mem_field32_modify(unit, RMEPm, endpoint_p->remote_index, VALIDf, 0));

    /* L3 entry */

    _bcm_tr2x_oam_make_rmep_key(unit, &l3_key, endpoint_p->name,
        endpoint_p->level, endpoint_p->vlan, endpoint_p->glp);

    SOC_IF_ERROR_RETURN(soc_mem_delete(unit, L3_ENTRY_IPV4_UNICASTm,
        MEM_BLOCK_ALL, &l3_key));

    /* Local control block */

    SHR_BITCLR(oam_info_p->remote_endpoints_in_use, endpoint_p->remote_index);

    oam_info_p->remote_endpoints[endpoint_p->remote_index] =
        BCM_OAM_ENDPOINT_INVALID;

    return BCM_E_NONE;
}

static int _bcm_tr2x_oam_find_lmep(int unit, bcm_vlan_t vlan, uint32 sglp,
    int *index_p, l3_entry_ipv4_unicast_entry_t *l3_entry_p)
{
    l3_entry_ipv4_unicast_entry_t l3_key;

    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_key, LMEP__VIDf, vlan);

    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_key, LMEP__SGLPf, sglp);

    soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_key, KEY_TYPEf,
        TR_L3_HASH_KEY_TYPE_LMEP);

    return soc_mem_search(unit, L3_ENTRY_IPV4_UNICASTm, MEM_BLOCK_ANY,
        index_p, &l3_key, l3_entry_p, 0);
}

static int _bcm_tr2x_oam_find_free_local_endpoint_rx_index(int unit,
    SHR_BITDCL *endpoints, int endpoint_count, int increment, int offset,
    _bcm_oam_endpoint_t *endpoint_p)
{
    int endpoint_index;
    int mdl_set = 0;
    int rv = BCM_E_NONE;
    int l3_index = 0;
    uint32 level_bitmap = 0;
    l3_entry_ipv4_unicast_entry_t l3_entry;

    sal_memset(&l3_entry, 0, sizeof(l3_entry_ipv4_unicast_entry_t));

    /*
     * Check if an entry already exists for this VLAN + GLP pair.
     */
    _bcm_esw_l3_lock(unit);
    rv = _bcm_tr2x_oam_find_lmep(unit, endpoint_p->vlan, endpoint_p->glp,
             &l3_index, &l3_entry);
    if (rv == BCM_E_NOT_FOUND)
    {
        /*
         * If Entry is NOT found, select a new MA_INDEX for this MDL.
         */
        _bcm_esw_l3_unlock(unit);
        for (endpoint_index = 0; endpoint_index < endpoint_count;
            endpoint_index += increment)
        {
            SHR_BITTEST_RANGE(endpoints, endpoint_index, increment, mdl_set);
            if (0 == mdl_set)
            {
                break;
            }
        }
        if (endpoint_index >= endpoint_count)
        {
            return BCM_E_FULL;
        }
        endpoint_p->local_rx_index = (endpoint_index + offset);
        return BCM_E_NONE;
    } else if (rv >= 0) {
        /*
         * If there is a  MATCH, check if this MD level is available at this
         * index. If it's already occupied, report resource error.
         * Else this MD level is available at this index.
         */
        _bcm_esw_l3_unlock(unit);
        level_bitmap = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, 
                                            &l3_entry, LMEP__MDL_BITMAPf);
        level_bitmap &= (1 << offset);
        if (!level_bitmap) 
        {
            endpoint_p->local_rx_index =
                ((soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry,
                        LMEP__MA_BASE_PTRf) << 3) | offset);
            return BCM_E_NONE;
        } else {
            return BCM_E_RESOURCE;
        }
    } else {
        /*
         * L3_ENTRY_IPV4_UNICAST table search operation failed.
         */
        _bcm_esw_l3_unlock(unit);
        return rv;
    }
    return BCM_E_NONE;
}

static int _bcm_tr2x_oam_delete_lmep(int unit,
    _bcm_oam_endpoint_t *endpoint_p)
{
    _bcm_oam_info_t *oam_info_p;
    int l3_index;
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

        _bcm_esw_l3_lock(unit);

        if (BCM_SUCCESS(_bcm_tr2x_oam_find_lmep(unit, endpoint_p->vlan,
            endpoint_p->glp, &l3_index, &l3_entry)))
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

        _bcm_esw_l3_unlock(unit);

        SOC_IF_ERROR_RETURN(result);

        /* Local control block */

        SHR_BITCLR(oam_info_p->local_rx_endpoints_in_use,
            endpoint_p->local_rx_index);
    }

    return BCM_E_NONE;
}

static int _bcm_tr2x_oam_destroy_endpoint(int unit,
    _bcm_oam_endpoint_t *endpoint_p)
{
    if (endpoint_p->is_remote)
    {
        BCM_IF_ERROR_RETURN(_bcm_tr2x_oam_delete_rmep(unit, endpoint_p));
    }
    else
    {
        BCM_IF_ERROR_RETURN(_bcm_tr2x_oam_delete_lmep(unit, endpoint_p));
    }

    endpoint_p->in_use = 0;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2x_oam_endpoint_create
 * Purpose:
 *      Create or replace an OAM endpoint object
 * Parameters:
 *      unit - (IN) Unit number.
 *      endpoint_info - (IN/OUT) Pointer to an OAM endpoint structure
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_tr2x_oam_endpoint_create(
    int unit, 
    bcm_oam_endpoint_info_t *endpoint_info)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_group_t *group_p;
    _bcm_oam_endpoint_t *endpoint_p;
    int replace;
    int is_remote;
    int local_tx_enabled = 0;
    int local_rx_enabled = 0;
    int endpoint_index;
    bcm_module_t module_id;
    bcm_port_t port_id;
    bcm_trunk_t trunk_id;
    int local_id;
    rmep_entry_t rmep_entry;
    lmep_entry_t lmep_entry;
    ma_index_entry_t ma_index_entry;
    l3_entry_ipv4_unicast_entry_t l3_entry;
    int quantization_index;
    int l3_index;
    uint32 level_bitmap;
    int result = SOC_E_NONE;
    uint32 oam_current_time;
    int chk_ep_index = 0;
    int rv = BCM_E_NONE;
    uint32 src_glp = _BCM_OAM_INVALID_INDEX;
    uint32 dst_glp = _BCM_OAM_INVALID_INDEX;
    bcm_trunk_member_t *member_array = NULL;
    int member_count;
    bcm_gport_t member_gport;
    bcm_trunk_t tgid_out;
    int id_out;

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
        /* Invalid endpoint flag bit/s set. */
        return BCM_E_UNAVAIL;
    }

    if (endpoint_info->type != bcmOAMEndpointTypeEthernet)
    {
        /* Device supports Ethernet OAM only. */
        return BCM_E_UNAVAIL;
    }

    /* Replace operation. */
    replace = (endpoint_info->flags & BCM_OAM_ENDPOINT_REPLACE) ? 1 : 0;

    if (endpoint_info->flags & BCM_OAM_ENDPOINT_REMOTE)
    {
        if (endpoint_info->flags &
            (BCM_OAM_ENDPOINT_CCM_RX | BCM_OAM_ENDPOINT_LOOPBACK |
            BCM_OAM_ENDPOINT_DELAY_MEASUREMENT | BCM_OAM_ENDPOINT_LINKTRACE |
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

        local_tx_enabled = (endpoint_info->ccm_period !=
            BCM_OAM_ENDPOINT_CCM_PERIOD_DISABLED);

        local_rx_enabled = ((endpoint_info->flags &
            (BCM_OAM_ENDPOINT_CCM_RX | BCM_OAM_ENDPOINT_LOOPBACK |
            BCM_OAM_ENDPOINT_DELAY_MEASUREMENT |
            BCM_OAM_ENDPOINT_LINKTRACE)) != 0);
    }

    if (endpoint_info->flags & BCM_OAM_ENDPOINT_WITH_ID)
    {
        endpoint_index = endpoint_info->id;

        VALIDATE_ENDPOINT_INDEX(endpoint_index);

        if (replace && (!oam_info_p->endpoints[endpoint_index].in_use))
        {
            return BCM_E_NOT_FOUND;
        }
        else if (!replace && (oam_info_p->endpoints[endpoint_index].in_use))
        {
            return BCM_E_EXISTS;
        }

        /*
         * An MA_STATE/group entry is associated with one and only one LMEP
         * If an rx LMEP entry already existed for current group, return error
         */
        for (chk_ep_index = 0; chk_ep_index < oam_info_p->endpoint_count;
            ++chk_ep_index) {
            if (!replace && oam_info_p->endpoints[chk_ep_index].in_use
                && !is_remote
                && (oam_info_p->endpoints[chk_ep_index].group_index
                    == endpoint_info->group)
                && oam_info_p->endpoints[chk_ep_index].local_rx_enabled) {
                return BCM_E_RESOURCE;
            }
        }
    }
    else
    {
        if (replace)
        {
            /* Replace specified with no ID */

            return BCM_E_PARAM;
        }

        /*
         * An MA_STATE/group entry is associated with one and only one LMEP
         * If an rx LMEP entry already existed for current group, return error
         */
        for (endpoint_index = 0; endpoint_index < oam_info_p->endpoint_count;
             ++endpoint_index) {
            if (oam_info_p->endpoints[endpoint_index].in_use
                && !is_remote
                && (oam_info_p->endpoints[endpoint_index].group_index
                    == endpoint_info->group)
                && oam_info_p->endpoints[endpoint_index].local_rx_enabled) {
                return BCM_E_RESOURCE;
            }
        }

        for (endpoint_index = 0; endpoint_index < oam_info_p->endpoint_count;
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

    SET_ENDPOINT(endpoint_index);

    SET_GROUP(endpoint_info->group);

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
        dst_glp = (module_id << 6) | port_id;
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
        dst_glp = ((module_id << 6) | port_id);

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

    /* 
     * At this point, both src_glp and dst_glp should be valid.
     * Gport types other than TRUNK, MODPORT or LOCAL are not valid.
     */
    if (_BCM_OAM_INVALID_INDEX == src_glp
        || _BCM_OAM_INVALID_INDEX == dst_glp) {
        return (BCM_E_PORT);
    }

    if (replace)
    {
        /* Remove anything being replaced from tables */

        BCM_IF_ERROR_RETURN(_bcm_tr2x_oam_destroy_endpoint(unit, endpoint_p));
    }

    quantization_index = _bcm_tr2x_oam_quantize_ccm_period
                            (endpoint_info->ccm_period);

    endpoint_p->is_remote = is_remote;
    endpoint_p->local_tx_enabled = local_tx_enabled;
    endpoint_p->local_rx_enabled = local_rx_enabled;
    endpoint_p->group_index = endpoint_info->group;
    endpoint_p->name = endpoint_info->name;
    endpoint_p->level = endpoint_info->level;
    endpoint_p->vlan = endpoint_info->vlan;
    endpoint_p->glp = src_glp;

   if (is_remote)
    {
        endpoint_p->remote_index =
            _bcm_tr2x_oam_find_free_endpoint(oam_info_p->
                remote_endpoints_in_use,
            oam_info_p->remote_endpoint_count, 1, 0);

        if (endpoint_p->remote_index < 0)
        {
            return BCM_E_FULL;
        }

        /* RMEP entry */

        sal_memset(&rmep_entry, 0, sizeof(rmep_entry_t));

        soc_RMEPm_field32_set(unit, &rmep_entry, MAID_INDEXf,
            endpoint_info->group);

        /* The following steps are necessary to enable CCM timeout events
           without having received any CCMs */

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

        /* L3 entry */

        sal_memset(&l3_entry, 0, sizeof(l3_entry_ipv4_unicast_entry_t));

        soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__CCMf,
            quantization_index);

        soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__RMEP_PTRf,
            endpoint_p->remote_index);

        soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__MEPIDf,
            endpoint_info->name);

        soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__MDLf,
            endpoint_info->level);

        soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__VIDf,
            endpoint_info->vlan);

        soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, RMEP__SGLPf,
                                               src_glp);

        soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, KEY_TYPEf,
            TR_L3_HASH_KEY_TYPE_RMEP);

        soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, VALIDf, 1);

        SOC_IF_ERROR_RETURN(soc_mem_insert(unit, L3_ENTRY_IPV4_UNICASTm,
            MEM_BLOCK_ALL, &l3_entry));

        /* Local control block */

        SHR_BITSET(oam_info_p->remote_endpoints_in_use,
            endpoint_p->remote_index);

        oam_info_p->remote_endpoints[endpoint_p->remote_index] =
            endpoint_index;
    }
    else
    {
        if (local_tx_enabled)
        {
            uint32 reversed_maid[BCM_OAM_GROUP_NAME_LENGTH / 4];
            int word_index;

            endpoint_p->local_tx_index =
                _bcm_tr2x_oam_find_free_endpoint(oam_info_p->
                    local_tx_endpoints_in_use,
                oam_info_p->local_tx_endpoint_count, 1, 0);

            if (endpoint_p->local_tx_index < 0)
            {
                return BCM_E_FULL;
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

            soc_LMEPm_field32_set(unit, &lmep_entry, VLAN_IDf,
                endpoint_info->vlan);

            soc_LMEPm_field32_set(unit, &lmep_entry, CCMf, quantization_index);
            soc_LMEPm_field32_set(unit, &lmep_entry, DESTf, dst_glp);

            soc_LMEPm_field32_set(unit, &lmep_entry, MH_OPCODEf,
                BCM_HG_OPCODE_UC);

            soc_LMEPm_field32_set(unit, &lmep_entry, INT_PRIf,
                endpoint_info->int_pri);

            /* Set Port status TLV in CCM Tx packets. */
            if (endpoint_info->flags & BCM_OAM_ENDPOINT_PORT_STATE_UPDATE) {
                if ((endpoint_info->port_state != BCM_OAM_PORT_TLV_UP)
                    && (endpoint_info->port_state != BCM_OAM_PORT_TLV_BLOCKED)) {
                    return (BCM_E_PARAM);
                }
                soc_LMEPm_field32_set(unit, &lmep_entry, PORT_TLVf,
                    (endpoint_info->port_state == BCM_OAM_PORT_TLV_UP)
                     ? 1 : 0);
                }
                
            /* Set Interface status TLV in CCM Tx packets - 1-bit wide. */
            if (endpoint_info->flags
                & BCM_OAM_ENDPOINT_INTERFACE_STATE_UPDATE) {
                
                if ((endpoint_info->interface_state
                     != BCM_OAM_INTERFACE_TLV_UP)
                    && (endpoint_info->interface_state
                        != BCM_OAM_INTERFACE_TLV_DOWN)) {
                    return (BCM_E_PARAM);
                }
                soc_LMEPm_field32_set(unit, &lmep_entry, INTERFACE_TLVf,
                    (endpoint_info->interface_state
                     == BCM_OAM_INTERFACE_TLV_UP) ? 1: 0);
            }
            
            /*
             * When this bit is '1', both port and interface TLV values are set in
             * CCM Tx packets.
             */
            if ((endpoint_info->flags
                 & BCM_OAM_ENDPOINT_PORT_STATE_TX)
                || (endpoint_info->flags
                    & BCM_OAM_ENDPOINT_INTERFACE_STATE_TX)) {
                soc_LMEPm_field32_set(unit, &lmep_entry, INSERT_TLVf, 1);
            }

            /* Word-reverse the MAID bytes for the hardware */

            for (word_index = 0;
                word_index < (BCM_OAM_GROUP_NAME_LENGTH / 4);
                ++word_index)
            {
                reversed_maid[word_index] =
                    ((uint32 *) group_p->name)
                        [((BCM_OAM_GROUP_NAME_LENGTH / 4) - 1) - word_index];
            }

            soc_LMEPm_field_set(unit, &lmep_entry, MAIDf, reversed_maid);

            SOC_IF_ERROR_RETURN(WRITE_LMEPm(unit, MEM_BLOCK_ALL,
                endpoint_p->local_tx_index, &lmep_entry));

            /* Local control block */

            SHR_BITSET(oam_info_p->local_tx_endpoints_in_use,
                endpoint_p->local_tx_index);
        }

        if (local_rx_enabled)
        {
            rv = _bcm_tr2x_oam_find_free_local_endpoint_rx_index(unit,
                    oam_info_p->local_rx_endpoints_in_use,
                    oam_info_p->local_rx_endpoint_count,
                    8, endpoint_info->level, endpoint_p);
            if (rv != BCM_E_NONE)
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

            /* MA_INDEX entry */
            sal_memset(&ma_index_entry, 0, sizeof(ma_index_entry));

            soc_MA_INDEXm_field32_set(unit, &ma_index_entry, MA_PTRf,
                endpoint_info->group);

            SOC_IF_ERROR_RETURN(WRITE_MA_INDEXm(unit, MEM_BLOCK_ALL,
                endpoint_p->local_rx_index, &ma_index_entry));

            /* L3 entry */

            _bcm_esw_l3_lock(unit);

            if (BCM_SUCCESS(_bcm_tr2x_oam_find_lmep(unit, endpoint_info->vlan,
                src_glp, &l3_index, &l3_entry)))
            {
                /* There's already an entry for this vlan+glp */

                level_bitmap = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry,
                    LMEP__MDL_BITMAPf);

                level_bitmap |= (1 << endpoint_info->level);

                result = soc_mem_field32_modify(unit, L3_ENTRY_IPV4_UNICASTm,
                    l3_index, LMEP__MDL_BITMAPf, level_bitmap);

                _bcm_esw_l3_unlock(unit);
                
                SHR_BITSET(oam_info_p->local_rx_endpoints_in_use,
                    endpoint_p->local_rx_index);

                SOC_IF_ERROR_RETURN(result);
            }
            else
            {
                /* This is the first entry at this vlan+glp */

                _bcm_esw_l3_unlock(unit);

                sal_memset(&l3_entry, 0, sizeof(l3_entry_ipv4_unicast_entry_t));

                soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry,
                    LMEP__MDL_BITMAPf, 1 << endpoint_info->level);

                soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry,
                    LMEP__MA_BASE_PTRf,
                    endpoint_p->local_rx_index >> LEVEL_BIT_COUNT);

                soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry,
                    LMEP__DM_ENABLEf, (endpoint_info->flags &
                        BCM_OAM_ENDPOINT_DELAY_MEASUREMENT) ? 1 : 0);

                soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry,
                    LMEP__CCM_ENABLEf, (endpoint_info->flags &
                        BCM_OAM_ENDPOINT_CCM_RX) ? 1 : 0);

                soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry,
                    LMEP__LB_ENABLEf, (endpoint_info->flags &
                    BCM_OAM_ENDPOINT_LOOPBACK) ? 1 : 0);

                soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry,
                    LMEP__LT_ENABLEf, (endpoint_info->flags &
                    BCM_OAM_ENDPOINT_LINKTRACE) ? 1 : 0);

                soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, LMEP__VIDf,
                    endpoint_info->vlan);

                soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, LMEP__SGLPf,
                    src_glp);

                soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, KEY_TYPEf,
                    TR_L3_HASH_KEY_TYPE_LMEP);

                soc_L3_ENTRY_IPV4_UNICASTm_field32_set(unit, &l3_entry, VALIDf, 1);

                SOC_IF_ERROR_RETURN(soc_mem_insert(unit, L3_ENTRY_IPV4_UNICASTm,
                    MEM_BLOCK_ALL, &l3_entry));

                /* Local control block */

                SHR_BITSET(oam_info_p->local_rx_endpoints_in_use,
                    endpoint_p->local_rx_index);
            }
        }
    }

    /* Local control block */

    endpoint_p->in_use = 1;

    return BCM_E_NONE;
}

static int _bcm_tr2x_oam_get_endpoint(int unit, int endpoint_index,
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

    endpoint_info->id = endpoint_index;
    endpoint_info->group = endpoint_p->group_index;
    endpoint_info->name = endpoint_p->name;
    endpoint_info->level = endpoint_p->level;
    endpoint_info->vlan = endpoint_p->vlan;

    if (endpoint_p->glp & (1 << 13))
    {
        /* Trunk */

        BCM_GPORT_TRUNK_SET(endpoint_info->gport, endpoint_p->glp & 0x7F);
    }
    else
    {
        module_id = (endpoint_p->glp & 0x0FC0) >> 6;
        port_id = (endpoint_p->glp & 0x3F);

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

    if (endpoint_p->is_remote)
    {
        endpoint_info->flags |= BCM_OAM_ENDPOINT_REMOTE;

        /* RMEP entry */

        SOC_IF_ERROR_RETURN(READ_RMEPm(unit, MEM_BLOCK_ANY,
										endpoint_p->remote_index, 
										&rmep_entry)); 
		
        /* Get endpoint faults and clear presistent faults if Clear is SET */
		BCM_IF_ERROR_RETURN(_bcm_tr2x_oam_read_clear_faults(unit,
															endpoint_p->remote_index, 
															endpoint_faults, 
															RMEPm, 
															(uint32 *) &rmep_entry,
															&endpoint_info->faults,
															&endpoint_info->persistent_faults,
															endpoint_info->clear_persistent_faults));

        /* L3 entry */
        _bcm_tr2x_oam_make_rmep_key(unit, &l3_key, endpoint_p->name,
            endpoint_p->level, endpoint_p->vlan, endpoint_p->glp);

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
        }

        if (endpoint_p->local_rx_enabled)
        {
            /* L3 entry */

            if (BCM_FAILURE(_bcm_tr2x_oam_find_lmep(unit, endpoint_p->vlan,
                endpoint_p->glp, &l3_index, &l3_entry)))
            {
                return BCM_E_INTERNAL;
            }

            if (soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry,
                LMEP__DM_ENABLEf))
            {
                endpoint_info->flags |= BCM_OAM_ENDPOINT_DELAY_MEASUREMENT;
            }

            if (soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry,
                LMEP__CCM_ENABLEf))
            {
                endpoint_info->flags |= BCM_OAM_ENDPOINT_CCM_RX;
            }

            if (soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry,
                LMEP__LB_ENABLEf))
            {
                endpoint_info->flags |= BCM_OAM_ENDPOINT_LOOPBACK;
            }

            if (soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, &l3_entry,
                LMEP__LT_ENABLEf))
            {
                endpoint_info->flags |= BCM_OAM_ENDPOINT_LINKTRACE;
            }
        }
    }

    endpoint_info->ccm_period = ccm_periods[quantization_index];

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2x_oam_endpoint_get
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
bcm_tr2x_oam_endpoint_get(
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

    return _bcm_tr2x_oam_get_endpoint(unit, endpoint, endpoint_p,
        endpoint_info);
}

/*
 * Function:
 *      bcm_tr2x_oam_endpoint_destroy
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
bcm_tr2x_oam_endpoint_destroy(
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

    return _bcm_tr2x_oam_destroy_endpoint(unit, endpoint_p);
}

/*
 * Function:
 *      bcm_tr2x_oam_endpoint_destroy_all
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
bcm_tr2x_oam_endpoint_destroy_all(
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
            BCM_IF_ERROR_RETURN(_bcm_tr2x_oam_destroy_endpoint(unit,
                endpoint_p));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2x_oam_endpoint_traverse
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
bcm_tr2x_oam_endpoint_traverse(
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

            BCM_IF_ERROR_RETURN(_bcm_tr2x_oam_get_endpoint(unit,
                endpoint_index, endpoint_p, &endpoint_info));

            BCM_IF_ERROR_RETURN(cb(unit, &endpoint_info, user_data));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2x_oam_event_register
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
bcm_tr2x_oam_event_register(
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

    /* Check and return error for unsupported events. */
    SHR_BITTEST_RANGE(event_types.w,
                      bcmOAMEventBHHLBTimeout,
                      (bcmOAMEventCount - bcmOAMEventBHHLBTimeout),
                      result);
    if (0 != result) {
        return (BCM_E_PARAM);
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
            if (interrupt_enable_fields[event_type] == INVALIDf)
            {
                return BCM_E_PARAM;
            }

            if (!SHR_BITGET(event_handler_p->event_types.w, event_type))
            {
                /* This handler isn't handling this event yet */

                SHR_BITSET(event_handler_p->event_types.w, event_type);

                ++(oam_info_p->event_handler_count[event_type]);

                if (oam_info_p->event_handler_count[event_type] == 1)
                {
                    /* This is the first handler for this event */

                    update_interrupt_control = 1;

                    soc_reg_field_set(unit, CCM_INTERRUPT_CONTROLr,
                        &interrupt_control_register_value,
                        interrupt_enable_fields[event_type], 1);
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

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2x_oam_event_unregister
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
bcm_tr2x_oam_event_unregister(
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
            if (interrupt_enable_fields[event_type] == INVALIDf)
            {
                return BCM_E_UNAVAIL;
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

                    soc_reg_field_set(unit, CCM_INTERRUPT_CONTROLr,
                        &interrupt_control_register_value,
                        interrupt_enable_fields[event_type], 0);
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

    return BCM_E_NONE;
}

#if defined(BCM_WARM_BOOT_SUPPORT)
int _bcm_tr2x_oam_sync(int unit)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_group_t *group_p;
    int allocation_size = 0;
    int group_index;
    int stable_size;
    soc_scache_handle_t scache_handle;
    uint8 *group_names_p;
    int rv = BCM_E_INTERNAL;

    SET_OAM_INFO;

    CHECK_INIT;

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
    if (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit) || (stable_size == 0)) {
        return BCM_E_NONE;
    }

    /* Calculate memory size required to store group names on this device */
    allocation_size = (oam_info_p->group_count
                                * (BCM_OAM_GROUP_NAME_LENGTH));

    /* Set OAM module scache handle */
    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_OAM, 0);

    /* Check if memory has already been allocated */
    rv = _bcm_esw_scache_ptr_get(unit, scache_handle, 0, allocation_size,
                                 &group_names_p, BCM_WB_DEFAULT_VERSION,
                                 NULL);
    if (!SOC_WARM_BOOT(unit) && (BCM_E_NOT_FOUND == rv)) {
        /* Allocate scache memory as it has not been allocated */
        BCM_IF_ERROR_RETURN(_bcm_esw_scache_ptr_get(unit, scache_handle, 1,
                                                    allocation_size,
                                                    &group_names_p,
                                                    BCM_WB_DEFAULT_VERSION,
                                                    NULL));
        if (group_names_p == NULL) {
            return BCM_E_MEMORY;
        }
    }

    /* Save group names in the scache memory */
    for (group_index = 0; group_index < oam_info_p->group_count;
        ++group_index)
    {
        SET_GROUP(group_index);

        if (group_p->in_use)
        {
            sal_memcpy(group_names_p, group_p->name, BCM_OAM_GROUP_NAME_LENGTH);

            group_names_p += BCM_OAM_GROUP_NAME_LENGTH;
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
_bcm_tr2x_oam_sw_dump(int unit)
{
    _bcm_oam_info_t *oam_info_p;
    _bcm_oam_endpoint_t *endpoint_p;
    int group_idx, endpoint_idx;
    SHR_BITDCL word;

    SET_OAM_INFO;

    soc_cm_print("\nSW Information OAM - Unit %d\n", unit);
    soc_cm_print("  Group Info    : \n");
    
    for (group_idx = 0; group_idx < oam_info_p->group_count; group_idx++) {
        if (oam_info_p->groups[group_idx].in_use) {
            soc_cm_print("Group %d is in use\n", group_idx);
        }
    }

    soc_cm_print("\n local_tx_endpoints_in_use \n");
    for (word = 0; word < _SHR_BITDCLSIZE
         (oam_info_p->local_tx_endpoint_count); word++) {
        soc_cm_print(" word %d value %x ", word, 
                     oam_info_p->local_tx_endpoints_in_use[word]);
    }

    soc_cm_print("\n local_rx_endpoints_in_use \n");
    for (word = 0; word < _SHR_BITDCLSIZE
         (oam_info_p->local_rx_endpoint_count); word++) {
        soc_cm_print(" word %d value %x ", word, 
                     oam_info_p->local_rx_endpoints_in_use[word]);
    }

    soc_cm_print("\n remote_endpoints_in_use \n");
    for (word = 0; word < _SHR_BITDCLSIZE
         (oam_info_p->remote_endpoint_count); word++) {
        soc_cm_print(" word %d value %x ", word, 
                     oam_info_p->remote_endpoints_in_use[word]);
    }

    soc_cm_print("\n Reverse RMEP lookup \n");
    for (endpoint_idx = 0; endpoint_idx < oam_info_p->remote_endpoint_count; 
         endpoint_idx++) {
        if (oam_info_p->endpoints
            [oam_info_p->remote_endpoints[endpoint_idx]].in_use) {
            soc_cm_print("RMEP %x \n", 
                          oam_info_p->remote_endpoints[endpoint_idx]);
        } 
    }

    soc_cm_print("\n Endpoint Information \n");
    for (endpoint_idx = 0; endpoint_idx < oam_info_p->endpoint_count; 
         endpoint_idx++) {
        SET_ENDPOINT(endpoint_idx);
        if (!endpoint_p->in_use) {
            continue;
        }
        soc_cm_print("\n Endpoint index %d\n", endpoint_idx);
        soc_cm_print("\t Group index %d\n", endpoint_p->group_index);
        soc_cm_print("\t Name %x\n", endpoint_p->name);
        soc_cm_print("\t Level %d\n", endpoint_p->level);
        soc_cm_print("\t VLAN %d\n", endpoint_p->vlan);
        soc_cm_print("\t GLP %x\n", endpoint_p->glp);
        soc_cm_print("\t local_tx_enabled %d\n", endpoint_p->local_tx_enabled);
        soc_cm_print("\t local_rx_enabled %d\n", endpoint_p->local_rx_enabled);
        soc_cm_print("\t remote_index %d\n", endpoint_p->remote_index);
        soc_cm_print("\t local_tx_index %d\n", endpoint_p->local_tx_index);
        soc_cm_print("\t local_rx_index %d\n", endpoint_p->local_rx_index);
    }
    return;
}
#endif
#endif /* defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
          defined(BCM_VALKYRIE2_SUPPORT)*/
