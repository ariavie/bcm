/*
 * $Id: oam.c 1.72.2.1 Broadcom SDK $
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
 * File:
 *  oam.c
 *
 * Purpose:
 *  OAM implementation for Triumph3 family of devices.
 */

#include <sal/core/libc.h>

#include <soc/defs.h>
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/profile_mem.h>
#include <soc/debug.h>
#include <soc/hash.h>
#include <soc/l3x.h>
#include <soc/triumph3.h>
#include <soc/ism_hash.h>

#include <bcm/l3.h>
#include <bcm/oam.h>

#include <bcm_int/esw/oam.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw/triumph3.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw_dispatch.h>

#if defined(BCM_TRIUMPH3_SUPPORT)  && defined(INCLUDE_BHH)
#include <bcm_int/esw/bhh.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_HURRICANE2_SUPPORT)
#include <soc/hurricane2.h>
#endif
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)

/*
 * Device OAM control structure.
 */
_bcm_oam_control_t *_oam_control[SOC_MAX_NUM_DEVICES];

/* * * * * * * * * * * * * * * * * * *
 *            OAM MACROS             *
 * * * * * * * * * * * * * * * * * * */
/*
 * Macro:
 *     _BCM_OAM_IS_INIT (internal)
 * Purpose:
 *     Check that the unit is valid and confirm that the oam functions
 *     are initialized.
 * Parameters:
 *     unit - BCM device number
 * Notes:
 *     Results in return(BCM_E_UNIT), return(BCM_E_UNAVAIL), or
 *     return(BCM_E_INIT) if fails.
 */
#define _BCM_OAM_IS_INIT(unit)                                               \
            do {                                                             \
                if (!soc_feature(unit, soc_feature_oam)) {                   \
                    return (BCM_E_UNAVAIL);                                  \
                }                                                            \
                if (_oam_control[unit] == NULL) {                            \
                    OAM_ERR(("OAM(unit %d) Error: Module not initialized\n", \
                            unit));                                          \
                    return (BCM_E_INIT);                                     \
                }                                                            \
            } while (0)

/*
 *Macro:
 *     _BCM_OAM_LOCK
 * Purpose:
 *     Lock take the OAM control mutex
 * Parameters:
 *     control - Pointer to OAM control structure.
 */
#define _BCM_OAM_LOCK(control) \
            sal_mutex_take((control)->oc_lock, sal_mutex_FOREVER)

/*
 * Macro:
 *     _BCM_OAM_UNLOCK
 * Purpose:
 *     Lock take the OAM control mutex
 * Parameters:
 *     control - Pointer to OAM control structure.
 */
#define _BCM_OAM_UNLOCK(control) \
            sal_mutex_give((control)->oc_lock);
/*
 * Macro:
 *     _BCM_OAM_HASH_DATA_CLEAR
 * Purpose:
 *      Clear hash data memory occupied by one endpoint.
 * Parameters:
 *     _ptr_    - Pointer to endpoint hash data memory. 
 */
#define _BCM_OAM_HASH_DATA_CLEAR(_ptr_) \
            sal_memset(_ptr_, 0, sizeof(_bcm_oam_hash_data_t));

/*
 * Macro:
 *     _BCM_OAM_HASH_DATA_HW_IDX_INIT
 * Purpose:
 *     Initialize hardware indices to invalid index for an endpoint hash data.
 * Parameters:
 *     _ptr_    - Pointer to endpoint hash data memory. 
 */
#define _BCM_OAM_HASH_DATA_HW_IDX_INIT(_ptr_)                             \
            do {                                                          \
                    (_ptr_)->group_index = (_BCM_OAM_INVALID_INDEX);      \
                    (_ptr_)->remote_index = (_BCM_OAM_INVALID_INDEX);     \
                    (_ptr_)->profile_index = (_BCM_OAM_INVALID_INDEX);    \
                    (_ptr_)->pri_map_index = (_BCM_OAM_INVALID_INDEX);    \
                    (_ptr_)->lm_counter_index = (_BCM_OAM_INVALID_INDEX); \
                    (_ptr_)->local_tx_index = (_BCM_OAM_INVALID_INDEX);   \
                    (_ptr_)->local_rx_index = (_BCM_OAM_INVALID_INDEX);   \
            } while (0)
/*
 * Macro:
 *     _BCM_OAM_ALLOC
 * Purpose:
 *      Generic memory allocation routine.
 * Parameters:
 *    _ptr_     - Pointer to allocated memory.
 *    _ptype_   - Pointer type.
 *    _size_    - Size of heap memory to be allocated.
 *    _descr_   - Information about this memory allocation.
 */
#define _BCM_OAM_ALLOC(_ptr_,_ptype_,_size_,_descr_)                     \
            do {                                                         \
                if (NULL == (_ptr_)) {                                   \
                   (_ptr_) = (_ptype_ *) sal_alloc((_size_), (_descr_)); \
                }                                                        \
                if((_ptr_) != NULL) {                                    \
                    sal_memset((_ptr_), 0, (_size_));                    \
                }  else {                                                \
                    OAM_ERR(("OAM Error: Allocation failure %s\n",       \
                            (_descr_)));                                 \
                }                                                        \
            } while (0)

/*
 * Macro:
 *     _BCM_OAM_GROUP_INDEX_VALIDATE
 * Purpose:
 *     Validate OAM Group ID value.
 * Parameters:
 *     _group_ - Group ID value.
 */
#define _BCM_OAM_GROUP_INDEX_VALIDATE(_group_)                               \
            do {                                                             \
                if ((_group_) < 0 || (_group_) >= oc->group_count) {         \
                    OAM_ERR(("OAM(unit %d) Error: Invalid Group ID = %d.\n", \
                            unit, _group_));                                 \
                    return (BCM_E_PARAM);                                    \
                }                                                            \
            } while (0);

/*
 * Macro:
 *     _BCM_OAM_EP_INDEX_VALIDATE
 * Purpose:
 *     Validate OAM Endpoint ID value.
 * Parameters:
 *     _ep_ - Endpoint ID value.
 */
#define _BCM_OAM_EP_INDEX_VALIDATE(_ep_)                               \
            do {                                                       \
                if ((_ep_) < 0 || (_ep_) >= oc->ep_count) {            \
                    OAM_ERR(("OAM(unit %d) Error: Invalid Endpoint ID" \
                            " = %d.\n", unit, _ep_));                  \
                    return (BCM_E_PARAM);                              \
                }                                                      \
            } while (0);

/*
 * Macro:
 *     _BCM_OAM_RMEP_INDEX_VALIDATE
 * Purpose:
 *     Validate OAM Endpoint ID value.
 * Parameters:
 *     _ep_ - Endpoint ID value.
 */
#define _BCM_OAM_RMEP_INDEX_VALIDATE(_ep_)                              \
            do {                                                        \
                if ((_ep_) < 0 || (_ep_) >= oc->rmep_count) {           \
                    OAM_ERR(("OAM(unit %d) Error: Invalid RMEP Index"   \
                            " = %d.\n", unit, _ep_));                   \
                    return (BCM_E_PARAM);                               \
                }                                                       \
            } while (0);

/*
 * Macro:
 *     _BCM_OAM_KEY_PACK
 * Purpose:
 *     Pack the hash table look up key fields.
 * Parameters:
 *     _dest_ - Hash key buffer.
 *     _src_  - Hash key field to be packed.
 *     _size_ - Hash key field size in bytes.
 */
#define _BCM_OAM_KEY_PACK(_dest_,_src_,_size_)            \
            do {                                          \
                sal_memcpy((_dest_), (_src_), (_size_));  \
                (_dest_) += (_size_);                     \
            } while (0)
/*
 * Macro:
 *     _BCM_TR3_OAM_MOD_PORT_TO_GLP
 * Purpose:
 *     Construct hardware SGLP value from module ID, port ID and Trunk ID value.
 * Parameters:
 *     _modid_ - Module ID.
 *     _port_  - Port ID.
 *     _trunk_ - Trunk (1 - TRUE/0 - FALSE).
 *     _tgid_  - Trunk ID.
 */
#define _BCM_TR3_OAM_MOD_PORT_TO_GLP(_u_, _m_, _p_, _t_, _tgid_, _glp_)     \
    do {                                                                    \
        if ((_tgid_) != -1) {                                               \
            (_glp_) = (((0x1 & (_t_)) << SOC_TRUNK_BIT_POS(_u_))            \
                | ((soc_mem_index_count((_u_), TRUNK_GROUPm) - 1)           \
                & (_tgid_)));                                               \
        } else {                                                            \
            (_glp_) = (((0x1 & (_t_)) << SOC_TRUNK_BIT_POS(_u_))            \
                | ((SOC_MODID_MAX(_u_) & (_m_))                             \
                << (_shr_popcount((unsigned int) SOC_PORT_ADDR_MAX(_u_)))   \
                | (SOC_PORT_ADDR_MAX(_u_) & (_p_))));                       \
        }                                                                   \
        OAM_VVERB(("u:%d m:%d p:%d t:%d tgid:%d glp:%x\n",                  \
            _u_, _m_, _p_, _t_, _tgid_, _glp_));                            \
    } while (0)

/*
 * Macro:
 *     _BCM_TR3_OAM_MOD_PORT_TO_DGLP
 * Purpose:
 *     Construct hardware DGLP value from module ID, port ID and Trunk ID value.
 *     Add one to MOD shift as its 8 bit port and 8 bit mod in H/w 
 * Parameters:
 *     _modid_ - Module ID.
 *     _port_  - Port ID.
 *     _trunk_ - Trunk (1 - TRUE/0 - FALSE).
 *     _tgid_  - Trunk ID.
 */
#define _BCM_TR3_OAM_MOD_PORT_TO_DGLP(_u_, _m_, _p_, _t_, _tgid_, _glp_)    \
    do {                                                                    \
        if ((_tgid_) != -1) {                                               \
            (_glp_) = (((0x1 & (_t_)) << SOC_TRUNK_BIT_POS(_u_))            \
                | ((soc_mem_index_count((_u_), TRUNK_GROUPm) - 1)           \
                & (_tgid_)));                                               \
        } else {                                                            \
            (_glp_) = (((0x1 & (_t_)) << SOC_TRUNK_BIT_POS(_u_))            \
                | ((SOC_MODID_MAX(_u_) & (_m_))                             \
                << (_shr_popcount((unsigned int) SOC_PORT_ADDR_MAX(_u_))+1) \
                | (SOC_PORT_ADDR_MAX(_u_) & (_p_))));                       \
        }                                                                   \
        OAM_VVERB(("u:%d m:%d p:%d t:%d tgid:%d glp:%x\n",                  \
            _u_, _m_, _p_, _t_, _tgid_, _glp_));                            \
    } while (0)

/*
 * Macro:
 *     _BCM_OAM_GLP_XXX
 * Purpose:
 *     Get components of generic logical port value.
 * Parameters:
 *     _glp_ - Generic logical port.
 */
#define _BCM_OAM_GLP_TRUNK_BIT_GET(_glp_) (0x1 & ((_glp_) >> 15))
#define _BCM_OAM_GLP_TRUNK_ID_GET(_glp_)  (0xFF & (_glp_))
#define _BCM_OAM_GLP_MODULE_ID_GET(_glp_) (0xFF & ((_glp_) >> 7))
#define _BCM_OAM_GLP_PORT_GET(_glp_)      (0x7F & (_glp_))
/* To recover Module Id from H/w DGLP field */
#define _BCM_OAM_DGLP_MODULE_ID_GET(_glp_) (0xFF & ((_glp_) >> 8))

/*
 * Macro:
 *     _BCM_OAM_EP_LEVEL_XXX
 * Purpose:
 *     Maintenance domain level bit count and level max value.   
 * Parameters:
 *     _glp_ - Generic logical port.
 */
#define _BCM_OAM_EP_LEVEL_COUNT (1 << (_BCM_OAM_EP_LEVEL_BIT_COUNT))
#define _BCM_OAM_EP_LEVEL_MAX (_BCM_OAM_EP_LEVEL_COUNT - 1)

/*
 * Macro:
 *     BCM_WB_XXX
 * Purpose:
 *    OAM module scache version information.
 * Parameters:
 *    (major number, minor number)
 */
#ifdef BCM_WARM_BOOT_SUPPORT
#define BCM_WB_VERSION_1_0      SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION  BCM_WB_VERSION_1_0
#endif
/* * * * * * * * * * * * * * * * * * * * * * * * *
 *         OAM function prototypes               *
 * * * * * * * * * * * * * * * * * * * * * * * * */


/* * * * * * * * * * * * * * * * * * * * * * * * *
 *         OAM global data initialization      *
 * * * * * * * * * * * * * * * * * * * * * * * * */
#if defined(INCLUDE_BHH)

#define BHH_COSQ_INVALID          0xFFFF

/*
 * Macro:
 *     _BCM_OAM_BHH_IS_VALID (internal)
 * Purpose:
 *     Check that the BHH feature is available on this unit
 * Parameters:
 *     unit - BCM device number
 * Notes:
 *     Results in return(BCM_E_UNAVAIL), 
 */
#define _BCM_OAM_BHH_IS_VALID(unit)                                          \
            do {                                                             \
                if (!soc_feature(unit, soc_feature_bhh)) {                   \
                    return (BCM_E_UNAVAIL);                                  \
                }                                                            \
            } while (0)

#define BCM_OAM_BHH_GET_UKERNEL_EP(ep) \
        (ep - _BCM_OAM_BHH_ENDPOINT_OFFSET)

#define BCM_OAM_BHH_GET_SDK_EP(ep) \
        (ep + _BCM_OAM_BHH_ENDPOINT_OFFSET)

#define BCM_OAM_BHH_VALIDATE_EP(_ep_) \
            do {                                                       \
                if (((_ep_) < _BCM_OAM_BHH_ENDPOINT_OFFSET) ||         \
                    ((_ep_) >= (_BCM_OAM_BHH_ENDPOINT_OFFSET           \
                                         + oc->bhh_endpoint_count))) { \
                    OAM_ERR(("OAM(unit %d) Error: Invalid Endpoint ID" \
                            " = %d.\n", unit, _ep_));                  \
                    _BCM_OAM_UNLOCK(oc);                               \
                    return (BCM_E_PARAM);                              \
                }                                                      \
            } while (0);

#define BCM_OAM_BHH_ABS(x)   (((x) < 0) ? (-(x)) : (x))

STATIC int
_bcm_tr3_oam_bhh_session_hw_delete(int unit, _bcm_oam_hash_data_t *h_data_p);
STATIC int
_bcm_tr3_oam_bhh_msg_send_receive(int unit, uint8 s_subclass,
                                    uint16 s_len, uint32 s_data,
                                    uint8 r_subclass, uint16 *r_len);
STATIC int
_bcm_tr3_oam_bhh_hw_init(int unit);
STATIC void
_bcm_tr3_oam_bhh_callback_thread(void *param);
STATIC int
_bcm_tr3_oam_bhh_event_mask_set(int unit);
STATIC int
bcm_tr3_oam_bhh_endpoint_create(int unit, 
                                bcm_oam_endpoint_info_t *endpoint_info,
                                _bcm_oam_hash_key_t  *hash_key);
#endif /* INCLUDE_BHH */

/*
 * Triumph3 device OAM CCM intervals array initialization..
 */
STATIC uint32 _tr3_ccm_intervals[] =
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

/*   
 * OAM hardware interrupts to software events mapping array initialization.
 * _tr3_oam_interrupts[] =
 *     {Interrupt status register, Remote MEP index field, MA index field,
 *          CCM_INTERRUPT_CONTROLr - Interrupt status Field,
 *          OAM event type}. 
 */
STATIC _bcm_oam_interrupt_t _tr3_oam_interrupts[] =
{
    /* 1. Port down interrupt. */
    {ANY_RMEP_TLV_PORT_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_PORT_DOWN_INT_ENABLEf,
        bcmOAMEventEndpointPortDown},

    /* 2. Port up interrupt. */
    {ANY_RMEP_TLV_PORT_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_PORT_UP_INT_ENABLEf,
        bcmOAMEventEndpointPortUp},

    /* 3. Interface down interrupt. */
    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_DOWN_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceDown},

    /* 4. Interface up interrupt. */
    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_DOWN_TO_UP_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceUp},

    /* 5. Interface TLV Testing to Up interrupt. */
    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_TESTING_TO_UP_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceTestingToUp},

    /* 6. Interface TLV Unknown to Up interrupt. */
    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UNKNOWN_TO_UP_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceUnknownToUp},

    /* 7. Interface TLV Dormant to Up interrupt. */
    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_DORMANT_TO_UP_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceDormantToUp},

    /* 8. Interface TLV Not present to Up interrupt. */
    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_NOTPRESENT_TO_UP_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceNotPresentToUp},

    /* 9. Interface Link Layer Down to Up interrupt. */
    {ANY_RMEP_TLV_INTERFACE_UP_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_LLDOWN_TO_UP_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceLLDownToUp},

    /* 10. Interface up to testing transition interrupt. */
    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_TESTING_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceTesting},

    /* 11. Interface up to unknown transition interrupt. */
    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_UNKNOWN_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceUnkonwn},

    /* 11. Interface up to dormant transition interrupt. */
    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_DORMANT_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceDormant},

    /* 11. Interface up to not present transition interrupt. */
    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_NOTPRESENT_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceNotPresent},

    /* 11. Interface up to Link Layer Down transition interrupt. */
    {ANY_RMEP_TLV_INTERFACE_DOWN_STATUSr, FIRST_RMEP_INDEXf, INVALIDf,
        ANY_RMEP_TLV_INTERFACE_UP_TO_LLDOWN_TRANSITION_INT_ENABLEf,
        bcmOAMEventEndpointInterfaceLLDown},

    /* 12. Low MDL or unexpected MAID interrupt. */
    {XCON_CCM_DEFECT_STATUSr, INVALIDf, FIRST_MA_INDEXf,
        ERROR_CCM_DEFECT_INT_ENABLEf,
        bcmOAMEventGroupCCMxcon},

    /*
     * 13. Remote MEP lookup failed or CCM interval mismatch during Remote MEP
     *    lookup interrupt.
     */
    {ERROR_CCM_DEFECT_STATUSr, INVALIDf, FIRST_MA_INDEXf,
        ERROR_CCM_DEFECT_INT_ENABLEf,
        bcmOAMEventGroupCCMError},

    /*
     * 14. Some Remote defect indicator interrupt - aggregated health of remote
     *    MEPs.
     */
    {SOME_RDI_DEFECT_STATUSr, FIRST_RMEP_INDEXf, FIRST_MA_INDEXf,
        SOME_RDI_DEFECT_INT_ENABLEf,
        bcmOAMEventGroupRemote},

    /* 15. Aggregate health of remote MEP state machines interrupt. */
    {SOME_RMEP_CCM_DEFECT_STATUSr, FIRST_RMEP_INDEXf, FIRST_MA_INDEXf,
        SOME_RMEP_CCM_DEFECT_INT_ENABLEf,
        bcmOAMEventGroupCCMTimeout},

    /* Invalid Interrupt - Always Last */
    {INVALIDr, INVALIDf, 0, bcmOAMEventCount}
};

/*
 * 0AM Group faults array initialization.
 */
STATIC _bcm_oam_fault_t _tr3_oam_group_faults[] =
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

/*
 * 0AM Endpoint faults array initialization.
 */
STATIC _bcm_oam_fault_t _tr3_oam_endpoint_faults[] =
{
    {CURRENT_RMEP_PORT_STATUS_DEFECTf,
        STICKY_RMEP_PORT_STATUS_DEFECTf,
        BCM_OAM_ENDPOINT_FAULT_PORT_DOWN, 0x08},

    {CURRENT_RMEP_INTERFACE_STATUS_DEFECTf,
        STICKY_RMEP_INTERFACE_STATUS_DEFECTf,
        BCM_OAM_ENDPOINT_FAULT_INTERFACE_DOWN, 0x04},

    {CURRENT_RMEP_CCM_DEFECTf,
        STICKY_RMEP_CCM_DEFECTf,
        BCM_OAM_ENDPOINT_FAULT_CCM_TIMEOUT, 0x20},

    {CURRENT_RMEP_LAST_RDIf,
        STICKY_RMEP_LAST_RDIf,
        BCM_OAM_ENDPOINT_FAULT_REMOTE, 0x10},

    {0, 0, 0, 0}
};

typedef struct _bcm_tr3_oam_intr_en_fields_s {
    soc_field_t field;
    uint32      value;
} _bcm_tr3_oam_intr_en_fields_t;
 
STATIC _bcm_tr3_oam_intr_en_fields_t _tr3_oam_intr_en_fields[] =
{
    /*
     * Note:
     * The order of hardware field names in the below initialization
     * code must match the event enum order in bcm_oam_event_type_t.
     */
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
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0},
    { INVALIDf, 0}
};

/* For Ethernet OAM Loss & Delay Measurement */
typedef struct _bcm_oam_lm_dm_search_data_type_s {
    bcm_oam_endpoint_type_t match_type; /* Endpoint Type. */
    bcm_oam_endpoint_t  match_ep_id; /* Current Endpoint ID, being added. */
    bcm_vlan_t          match_vlan; /* Vlan on which the EP is added. */
    uint32              match_gport; /* Gport on which the EP is added. */
    int                 match_count; /* No. of existing matching EPs
                                        (exluding the current). */
    uint8               highest_level; /* Highest level of EP on vlan port. */
    bcm_oam_endpoint_t  highest_level_ep_id; /* Existing matching EP id. */
} _bcm_oam_lm_dm_search_data_type_t;

STATIC _bcm_oam_lm_dm_search_data_type_t _bcm_oam_lm_dm_search_data;

STATIC int _bcm_tr3_oam_loss_delay_measurement_delete(int unit, 
            _bcm_oam_control_t *oc, _bcm_oam_hash_data_t *hash_data);

            
/* * * * * * * * * * * * * * * * * * * * * * * * *
 *            Static local functions             *
 * * * * * * * * * * * * * * * * * * * * * * * * */
/*
 * Function:
 *     _bcm_tr3_oam_ccm_msecs_to_hw_encode
 * Purpose:
 *     Quanitze CCM interval from msecs to hardware encoding.
 * Parameters:
 *     period -  (IN) CCM interval in milli seconds.
 * Retruns:
 *     Hardware encoding for the specified CCM interval value.
 */
STATIC int
_bcm_tr3_oam_ccm_msecs_to_hw_encode(int period)
{
    int q_period = 0; /* Quantized CCM period value. */

    if (0 == period) {
        return (q_period);
    }

    /* Find closest supported period */
    for (q_period = 1; _tr3_ccm_intervals[q_period]
            != _BCM_OAM_ENDPOINT_CCM_PERIOD_UNDEFINED; ++q_period) {
        if (period < _tr3_ccm_intervals[q_period]) {
            break;
        }
    }

    if (_tr3_ccm_intervals[q_period]
        == _BCM_OAM_ENDPOINT_CCM_PERIOD_UNDEFINED) {
        /* Use the highest defined value */
        --q_period;
    } else {
        if ((period - _tr3_ccm_intervals[q_period - 1])
                < (_tr3_ccm_intervals[q_period] - period)) {
            /* Closer to the lower value */
            --q_period;
        }
    }

    return q_period;
}

#if defined(BCM_WARM_BOOT_SUPPORT)
/*
 * Function:
 *     _bcm_tr3_oam_ccm_hw_encode_to_msecs
 * Purpose:
 *     Get CCM interval in msecs for a given hardware encoded value.
 * Parameters:
 *     encode -  (IN) CCM interval hardware encoding.
 * Retruns:
 *     CCM interval in msecs.
 */
STATIC int
_bcm_tr3_oam_ccm_hw_encode_to_msecs(int encode)
{
    return (_tr3_ccm_intervals[encode]);
}
#endif

/*
 * Function:
 *     _bcm_tr3_oam_opcode_profile_entry_set
 * Purpose:
 *     Program the OAM opcode control profile fields.
 * Parameters:
 *     unit  - (IN) BCM device number
 *     flags - (IN) Bitmap of opcode control settings.
 *     entry - (IN/OUT) Pointer to opcode control profile table entry buffer.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_opcode_profile_entry_set(int unit,
                                      uint32 flags,
                                      uint32 *entry)
{
    uint32 ep_opcode;        /* Endpoint opcode flag bits. */
    uint32 opcode_count = 0; /* Number of bits set.        */
    int    bp;               /* bit position.              */

    /* Validate opcode flag bits. */
    if (flags & ~(_BCM_TR3_OAM_OPCODE_MASK)) {
        return (BCM_E_PARAM);
    }

    /* Get number of valid opcodes supported. */
    opcode_count = _shr_popcount(_BCM_TR3_OAM_OPCODE_MASK);

    /*
     * Iterate over opcode flag bits and set corresponding fields
     * in entry buffer.
     */
    for (bp = 0; bp < opcode_count; bp++) {
        ep_opcode = (flags & (1 << bp));

        switch (ep_opcode) {
            case BCM_OAM_OPCODE_CCM_IN_HW:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            CCM_PROCESS_IN_HWf,
                                                            1);
                break;

            case BCM_OAM_OPCODE_CCM_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            CCM_COPYTO_CPUf, 1);
                break;

            case BCM_OAM_OPCODE_CCM_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            CCM_DROPf, 1);
                break;

            case BCM_OAM_OPCODE_LBM_IN_HW:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            LBM_UC_PROCESS_IN_HWf,
                                                            1);
                break;

            case BCM_OAM_OPCODE_LBM_UC_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            LBM_UC_COPYTO_CPUf,
                                                            1);
                break;

            case BCM_OAM_OPCODE_LBM_MC_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            LBM_MC_DROPf, 1);
                break;

            case BCM_OAM_OPCODE_LBR_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            LBR_COPYTO_CPUf, 1);
                break;

            case BCM_OAM_OPCODE_LBR_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            LBR_DROPf, 1);
                break;

            case BCM_OAM_OPCODE_LTM_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            LTM_COPYTO_CPUf, 1);
                break;

            case BCM_OAM_OPCODE_LTM_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            LTM_DROPf, 1);
                break;

            case BCM_OAM_OPCODE_LTR_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            LTR_COPYTO_CPUf, 1);
                break;

            case BCM_OAM_OPCODE_LTR_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            LTR_DROPf, 1);
                break;

            case BCM_OAM_OPCODE_LMEP_PKT_FWD:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            FWD_LMEP_PKTf, 1);
                break;

            case BCM_OAM_OPCODE_OTHER_COPY_TO_CPU:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set
                    (unit, entry, OTHER_OPCODE_COPYTO_CPUf, 1);
                break;

            case BCM_OAM_OPCODE_OTHER_DROP:
                soc_OAM_OPCODE_CONTROL_PROFILEm_field32_set(unit, entry,
                                                            OTHER_OPCODE_DROPf,
                                                            1);
                break;

            default:
                break;
        }
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_tr3_oam_opcode_profile_entry_set
 * Purpose:
 *     Setup default OAM opcode control profile settings for an entry.
 * Parameters:
 *     unit  - (IN) BCM device number
 *     entry - (IN/OUT) Pointer to opcode control profile table entry buffer.
 * Retruns:
 *     BCM_E_XXX
 */
int
_bcm_tr3_oam_opcode_profile_entry_init(int unit,
                                       uint32 *entry)
{
    uint32 opcode; /* Opcode flag bits.        */
    int    rv;     /* Operation return status. */

    opcode =  (BCM_OAM_OPCODE_CCM_COPY_TO_CPU |
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
                    BCM_OAM_OPCODE_OTHER_DROP);

    rv = _bcm_tr3_oam_opcode_profile_entry_set(unit, opcode, entry);

    return (rv);
}

/*
 * Function:
 *     _bcm_oam_ep_hash_key_construct
 * Purpose:
 *     Construct hash table key for a given endpoint information.
 * Parameters:
 *     unit    - (IN) BCM device number
 *     oc      - (IN) Pointer to OAM control structure.
 *     ep_info - (IN) Pointer to endpoint information structure.
 *     key     - (IN/OUT) Pointer to hash key buffer.
 * Retruns:
 *     None
 */
STATIC void
_bcm_oam_ep_hash_key_construct(int unit,
                               _bcm_oam_control_t *oc,
                               bcm_oam_endpoint_info_t *ep_info,
                               _bcm_oam_hash_key_t *key)
{
    uint8  *loc = *key;
    uint32 direction = 1;

    sal_memset(key, 0, sizeof(_bcm_oam_hash_key_t));

    if (NULL != ep_info) {

        _BCM_OAM_KEY_PACK(loc, &ep_info->group, sizeof(ep_info->group));

        _BCM_OAM_KEY_PACK(loc, &ep_info->name, sizeof(ep_info->name));

        _BCM_OAM_KEY_PACK(loc, &ep_info->gport, sizeof(ep_info->gport));

        _BCM_OAM_KEY_PACK(loc, &ep_info->level, sizeof(ep_info->level));

        _BCM_OAM_KEY_PACK(loc, &ep_info->vlan, sizeof(ep_info->vlan));

        _BCM_OAM_KEY_PACK(loc, &direction, sizeof(direction));
    }

    /* End address should not exceed size of _bcm_oam_hash_key_t. */
    assert ((int) (loc - *key) <= sizeof(_bcm_oam_hash_key_t));
}

#ifdef BCM_HURRICANE2_SUPPORT 
/*
 * Function:
 *     _bcm_hu2_oam_lmep_key_construct
 * Purpose:
 *     Construct LMEP view lookup key for a given endpoint.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     h_data_p - (IN) Pointer to endpoint hash data memory.
 *     l3_key_p - (IN/OUT) Pointer to entry buffer.
 * Retruns:
 *     None
 */
STATIC void
_bcm_hu2_oam_lmep_key_construct(int unit,
                                const _bcm_oam_hash_data_t *h_data_p,
                                l3_entry_ipv4_unicast_entry_t *l3_key_p)
{
    /* Set the search key type. */
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set
        (unit, l3_key_p, KEY_TYPEf, SOC_MEM_KEY_L3_ENTRY_LMEP);

    /* Set VLAN ID in search key. */
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set
        (unit, l3_key_p, LMEP__VIDf, h_data_p->vlan);


    /* SGLP contains generic logical port. */
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set
        (unit, l3_key_p, LMEP__SGLPf, h_data_p->sglp);

}

/*
 * Function:
 *     _bcm_hu2_oam_rmep_key_construct
 * Purpose:
 *     Construct RMEP view lookup key for a given endpoint.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     h_data_p - (IN) Pointer to endpoint hash data memory.
 *     l3_key_p - (IN/OUT) Pointer to entry buffer.
 * Retruns:
 *     None
 */
STATIC void
_bcm_hu2_oam_rmep_key_construct(int unit,
                            const _bcm_oam_hash_data_t *h_data_p,
                            l3_entry_ipv4_unicast_entry_t *l3_key_p)
{

    /* Set the search key type. */
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set
        (unit, l3_key_p, KEY_TYPEf, SOC_MEM_KEY_L3_ENTRY_RMEP);

    /* Set endpoint name. */
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set
        (unit, l3_key_p, RMEP__MEPIDf, h_data_p->name);

    /* Set the maintenance domain level. */
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set
        (unit, l3_key_p, RMEP__MDLf, h_data_p->level);

    /*
     * Set endpoint port on which remote device CCM packets will be received.
     */
     /* SGLP contains generic logical port. */
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set
         (unit, l3_key_p, RMEP__SGLPf, h_data_p->sglp);

     /* Set VLAN ID in search key. */
    soc_L3_ENTRY_IPV4_UNICASTm_field32_set
            (unit, l3_key_p, RMEP__VIDf, h_data_p->vlan);
}
#endif /* BCM_HURRICANE2_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT 
/*
 * Function:
 *     _bcm_oam_lmep_key_construct
 * Purpose:
 *     Construct LMEP view lookup key for a given endpoint.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     h_data_p - (IN) Pointer to endpoint hash data memory.
 *     l3_key_p - (IN/OUT) Pointer to entry buffer.
 * Retruns:
 *     None
 */
STATIC void
_bcm_oam_lmep_key_construct(int unit,
                            const _bcm_oam_hash_data_t *h_data_p,
                            l3_entry_1_entry_t *l3_key_p)
{
    /* Set the search key type. */
    soc_L3_ENTRY_1m_field32_set
        (unit, l3_key_p, KEY_TYPEf, SOC_MEM_KEY_L3_ENTRY_1_LMEP_LMEP);

    /* Set VLAN ID in search key. */
    soc_L3_ENTRY_1m_field32_set
        (unit, l3_key_p, LMEP__VIDf, h_data_p->vlan);

    /* Set port type status in the search key. */
    if (BCM_GPORT_IS_MIM_PORT(h_data_p->gport)
        || BCM_GPORT_IS_MPLS_PORT(h_data_p->gport)) {

        if (h_data_p->flags & BCM_OAM_ENDPOINT_PBB_TE) {
            /* SOURCE_TYPEf is '1' for virtual ports. */
            soc_L3_ENTRY_1m_field32_set
                (unit, l3_key_p, LMEP__SOURCE_TYPEf, 1);
        }

        /* SGLP field contains virtual port. */
        soc_L3_ENTRY_1m_field32_set
            (unit, l3_key_p, LMEP__SGLPf, h_data_p->vp);

    } else {

        /* SGLP contains generic logical port. */
        soc_L3_ENTRY_1m_field32_set
            (unit, l3_key_p, LMEP__SGLPf, h_data_p->sglp);

        /* SGLP field contains generic logical port value. */
        soc_L3_ENTRY_1m_field32_set
                (unit, l3_key_p, LMEP__SOURCE_TYPEf, 0);
    }
}

/*
 * Function:
 *     _bcm_oam_rmep_key_construct
 * Purpose:
 *     Construct RMEP view lookup key for a given endpoint.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     h_data_p - (IN) Pointer to endpoint hash data memory.
 *     l3_key_p - (IN/OUT) Pointer to entry buffer.
 * Retruns:
 *     None
 */
STATIC void
_bcm_oam_rmep_key_construct(int unit,
                            const _bcm_oam_hash_data_t *h_data_p,
                            l3_entry_1_entry_t *l3_key_p)
{

    /* Set the search key type. */
    soc_L3_ENTRY_1m_field32_set
        (unit, l3_key_p, KEY_TYPEf, SOC_MEM_KEY_L3_ENTRY_1_RMEP_RMEP);

    /* Set endpoint name. */
    soc_L3_ENTRY_1m_field32_set
        (unit, l3_key_p, RMEP__MEPIDf, h_data_p->name);

    /* Set the maintenance domain level. */
    soc_L3_ENTRY_1m_field32_set
        (unit, l3_key_p, RMEP__MDLf, h_data_p->level);

    /*
     * Set endpoint port on which remote device CCM packets will be received.
     * Port value could be SVP or SGLP.
     */
    if (BCM_GPORT_IS_MIM_PORT(h_data_p->gport)
        || BCM_GPORT_IS_MPLS_PORT(h_data_p->gport)) {

        if (h_data_p->flags & BCM_OAM_ENDPOINT_PBB_TE) {
            soc_L3_ENTRY_1m_field32_set
                (unit, l3_key_p, RMEP__SOURCE_TYPEf, 1);
        }

        /* SGLP contains virtual port. */
        soc_L3_ENTRY_1m_field32_set
            (unit, l3_key_p, RMEP__SGLPf, h_data_p->vp);

        /* Set VLAN ID in search key. */
        soc_L3_ENTRY_1m_field32_set
            (unit, l3_key_p, RMEP__VIDf, 0);
    } else {

        /* SGLP contains generic logical port. */
        soc_L3_ENTRY_1m_field32_set
            (unit, l3_key_p, RMEP__SGLPf, h_data_p->sglp);

        /* Set VLAN ID in search key. */
        soc_L3_ENTRY_1m_field32_set
            (unit, l3_key_p, RMEP__VIDf, h_data_p->vlan);
    }
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(KEY_PRINT)
/*
 * Function:
 *     _bcm_oam_hash_key_print
 * Purpose:
 *     Print the contents of a hash key buffer. 
 * Parameters:
 *     hash_key - (IN) Pointer to hash key buffer.
 * Retruns:
 *     None
 */
STATIC void
_bcm_oam_hash_key_print(_bcm_oam_hash_key_t *hash_key)
{
    int i;
    soc_cm_print("HASH KEY:");
    for(i = 0; i < _OAM_HASH_KEY_SIZE; i++) {
        soc_cm_print(":%u", *(hash_key[i]));
    }
    soc_cm_print("\n");
}
#endif

/*
 * Function:
 *     _bcm_oam_control_get
 * Purpose:
 *     Lookup a OAM control config from a bcm device id.
 * Parameters:
 *     unit -  (IN)BCM unit number.
 *     oc   -  (OUT) OAM control structure.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_oam_control_get(int unit, _bcm_oam_control_t **oc)
{
    if (NULL == oc) {
        return (BCM_E_PARAM);
    }

    /* Ensure oam module is initialized. */
    _BCM_OAM_IS_INIT(unit);

    *oc = _oam_control[unit];

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_oam_group_endpoint_count_init
 * Purpose:
 *     Retrieves and initializes endpoint count information for this device.
 * Parameters:
 *     unit -  (IN) BCM unit number.
 *     oc   -  (IN) Pointer to device OAM control structure.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_oam_group_endpoint_count_init(int unit, _bcm_oam_control_t *oc)
{
    /* Input parameter check. */
    if (NULL == oc) {
        return (BCM_E_PARAM);
    }

    /*
     * Get endpoint hardware table index count values and
     * initialize device OAM control structure members variables.
     */
    oc->rmep_count = soc_mem_index_count(unit, RMEPm);
    oc->lmep_count = soc_mem_index_count(unit, LMEPm);
    oc->ma_idx_count = soc_mem_index_count(unit, MA_INDEXm);

    /* Max number of endpoints supported by the device. */
    oc->ep_count = (oc->rmep_count + oc->lmep_count + oc->ma_idx_count);

    OAM_VVERB(("OAM(unit %d) Info: Total No. endpoint Count = %d.\n",
               unit, oc->ep_count));

    /* Max number of MA Groups supported by device. */
    oc->group_count = soc_mem_index_count(unit, MA_STATEm);
    OAM_VVERB(("OAM(unit %d) Info: Total No. Group Count = %d.\n",
               unit, oc->group_count));

    return (BCM_E_NONE);
}

#ifdef BCM_HURRICANE2_SUPPORT
/*
 * Function:
 *     _bcm_hu2_oam_ccm_rx_timeout_enable
 * Purpose:
 *     Enable CCM Timer operations for endpoint state table.
 * Parameters:
 *     unit -  (IN) BCM unit number.
 *     state - () Enable/Disable.
 * Retruns:
 *     BCM_E_XXX
 * Note:
 *     RMEP_MA_STATE_REFRESH_INDEXr - CPU access for debug only.
 */
STATIC int
_bcm_hu2_oam_ccm_rx_timeout_set(int unit, uint8 state)
{
    int     rv;       /* Opreation return status.   */
    uint32  rval = 0; /* Register value.            */

    BCM_IF_ERROR_RETURN(READ_IARB_OAM_REFRESH_CONTROLr(unit, &rval));
    
    soc_reg_field_set(unit, IARB_OAM_REFRESH_CONTROLr, &rval,
        REFRESH_ENABLEf, state ? 1 : 0);

    rv = WRITE_IARB_OAM_REFRESH_CONTROLr(unit, rval);

    return (rv);
}
#endif /* BCM_HURRICANE2_SUPPORT */

/*
 * Function:
 *     _bcm_oam_ccm_rx_timeout_enable
 * Purpose:
 *     Enable CCM Timer operations for endpoint state table.
 * Parameters:
 *     unit -  (IN) BCM unit number.
 *     state - () Enable/Disable.
 * Retruns:
 *     BCM_E_XXX
 * Note:
 *     RMEP_MA_STATE_REFRESH_INDEXr - CPU access for debug only.
 */
STATIC int
_bcm_oam_ccm_rx_timeout_set(int unit, uint8 state)
{
    int     rv;       /* Opreation return status.   */
    uint32  rval = 0; /* Register value.            */

    /* Enable timer instructions to RMEP/MA_STATE Table. */
    soc_reg_field_set(unit, OAM_TIMER_CONTROLr, &rval,
                      TIMER_ENABLEf, state ? 1 : 0);

    /* Set Clock granularity to 250us ticks - 1. */
    soc_reg_field_set(unit, OAM_TIMER_CONTROLr, &rval,
                      CLK_GRANf, 1);

    rv = WRITE_OAM_TIMER_CONTROLr(unit, rval);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Timer enable - Failed.\n", unit));
        return (rv);
    }

    BCM_IF_ERROR_RETURN(READ_AUX_ARB_CONTROL_2r(unit, &rval));
    
    soc_reg_field_set(unit, AUX_ARB_CONTROL_2r, &rval,
        OAM_REFRESH_ENABLEf, state ? 1 : 0);

    BCM_IF_ERROR_RETURN(WRITE_AUX_ARB_CONTROL_2r(unit, rval));

    return (rv);
}

/*
 * Function:
 *     _bcm_oam_ccm_tx_config_enable
 * Purpose:
 *     Enable transmission of OAM PDUs on local endpoint.
 * Parameters:
 *     unit -  (IN) BCM unit number.
 *     state - (IN) Enable/Disable
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_oam_ccm_tx_config_set(int unit, uint8 state)
{
    int     rv;       /* Opreation return status.   */
    uint32  rval = 0; /* Register value.            */

    /* Enable OAM LMEP Tx. */
    soc_reg_field_set(unit, OAM_TX_CONTROLr, &rval,
                      TX_ENABLEf, state ? 1 : 0);

    /* Enable CMIC buffer. */
    soc_reg_field_set(unit, OAM_TX_CONTROLr, &rval,
                      CMIC_BUF_ENABLEf, state ? 1 : 0);

    rv = WRITE_OAM_TX_CONTROLr(unit, rval);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Tx config enable - Failed.\n", unit));
    }
    return (rv);
}

/*
 * Function:
 *     _bcm_oam_misc_config
 * Purpose:
 *     Miscellaneous OAM configurations:
 *         1. Disable L2 learning for OAM MAC address.
 *         2. Enable flooding of mcast packets on the vlan.
 *         3. Enable IFP lookup on the CPU port. 
 * Parameters:
 *     unit -  (IN) BCM unit number.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_oam_misc_config(int unit)
{
    int     rv;       /* Opreation return status.   */
    uint32  rval = 0; /* Register value.            */

    /* Disable L2 learning of OAM packets. */
    soc_reg_field_set(unit, LMEP_COMMONr, &rval,
                      L3f, 1);

    /* PFM 0: Flood MCAST packets on the vlan. */
    soc_reg_field_set(unit, LMEP_COMMONr, &rval,
                      PFMf, 0);

    rv = WRITE_LMEP_COMMONr(unit, rval);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Misc config - Failed.\n", unit));
        return (rv);
    }

    /*
     * Enable ingress FP for CPU port so LM/DM packets sent from CPU
     * can be processed.
     */
    rv = bcm_esw_port_control_set(unit, CMIC_PORT(unit),
                                  bcmPortControlFilterIngress, 1);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: bcm_esw_port_control_set"
                 " - Failed.\n", unit));
        return (rv);
    }
    return (rv);

}

/*
 * Function:
 *     _bcm_oam_profile_tables_init
 * Purpose:
 *     Create ingress service priority mapping profile table and setup a
 *     default profile. Create OAM opcode control profile table. 
 * Parameters:
 *     unit -  (IN) BCM unit number.
 *     oc   -  (IN) Pointer to OAM control structure.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_oam_profile_tables_init(int unit, _bcm_oam_control_t *oc)
{
    int         rv;            /* Opreation return status.    */
    soc_mem_t   mem;           /* Profiled table memory.      */
    int         entry_words;   /* Profile table word size.    */
    int         pri;           /* Priority                    */
    void        *entries[1];   /* Profile entry. */
    uint32      profile_index; /* Profile table index. */
    ing_service_pri_map_entry_t pri_ent[BCM_OAM_INTPRI_MAX]; /* profile */
                                                             /* entry */

    /* Ingress Service Priority Map profile table initialization. */
    soc_profile_mem_t_init(&oc->ing_service_pri_map);

    entry_words = (sizeof(ing_service_pri_map_entry_t) / sizeof(uint32));

    mem = ING_SERVICE_PRI_MAPm;
    rv = soc_profile_mem_create(unit, &mem, &entry_words, 1,
                                &oc->ing_service_pri_map);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: service map profile - Failed.\n",
                unit));
        return (rv);
    }

    /*
     * Initialize ingress priority map profile table.
     * All priorities priorities map to priority:'0'.
     */
    for (pri = 0; pri < BCM_OAM_INTPRI_MAX; pri++)
    {
        /* Clear ingress service pri map profile entry. */
        sal_memcpy(&pri_ent[pri], soc_mem_entry_null(unit, mem),
                   soc_mem_entry_words(unit, mem) * sizeof(uint32));

        if (SOC_MEM_FIELD_VALID(unit, mem, OFFSET_VALIDf)) {
            soc_mem_field32_set(unit, mem, &pri_ent[pri], OFFSET_VALIDf, 1);
        }
    }

    
    entries[0] = &pri_ent;
    rv = soc_profile_mem_add(unit, &oc->ing_service_pri_map,
                             (void *) &entries, BCM_OAM_INTPRI_MAX,
                             &profile_index);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: service map init - Failed.\n",
                unit));
        return (rv);
    }

    /* OAM Opcode Control profile table initialization. */
    soc_profile_mem_t_init(&oc->oam_opcode_control_profile);

    entry_words = sizeof(oam_opcode_control_profile_entry_t)
                         / sizeof(uint32);

    mem = OAM_OPCODE_CONTROL_PROFILEm;
    rv = soc_profile_mem_create(unit, &mem, &entry_words, 1,
                                &oc->oam_opcode_control_profile);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: opcode control profile - Failed.\n",
                unit));
        return (rv);
    }

    return (rv);
}

/*
 * Function:
 *     _bcm_tr3_oam_control_free
 * Purpose:
 *     Free OAM control structure resources allocated by this unit.
 * Parameters:
 *     unit -  (IN) BCM unit number.
 *     oc   -  (IN) Pointer to OAM control structure.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_control_free(int unit, _bcm_oam_control_t *oc)
{
    int status = 0;

    _oam_control[unit] = NULL;

    if (NULL == oc) {
        /* Module already un-initialized. */
        return (BCM_E_NONE);
    }

    /* Free protection mutex. */
    if (NULL != oc->oc_lock) {
        sal_mutex_destroy(oc->oc_lock);
    }

    /* Free hash data storage memory. */
    if (NULL != oc->oam_hash_data) {
        sal_free(oc->oam_hash_data);
    }

    /* Destory endpoint hash table. */
    if (NULL != oc->ma_mep_htbl) {
        status = shr_htb_destroy(&oc->ma_mep_htbl, NULL);
        if (BCM_FAILURE(status)) {
            soc_cm_debug(DK_ERR, "Freeing ma_mep_htbl failed\n");
        }
    }

    /* Destory group indices list. */
    if (NULL != oc->group_pool) {
        shr_idxres_list_destroy(oc->group_pool);
        oc->group_pool = NULL;
    }

    /* Destroy endpoint indices list. */
    if (NULL != oc->mep_pool) {
        shr_idxres_list_destroy(oc->mep_pool);
        oc->mep_pool = NULL;
    }

    /* Destroy local endpoint indices list. */
    if (NULL != oc->lmep_pool) {
        shr_idxres_list_destroy(oc->lmep_pool);
        oc->lmep_pool = NULL;
    }

    /* Destroy remote endpoint indices list. */
    if (NULL != oc->rmep_pool) {
        shr_idxres_list_destroy(oc->rmep_pool);
        oc->rmep_pool = NULL;
    }

    /* Destroy group indices list. */
    if (NULL != oc->ma_idx_pool) {
        shr_idxres_list_destroy(oc->ma_idx_pool);
        oc->ma_idx_pool = NULL;
    }

    /* Destroy lm counter indices list. */
    if (NULL != oc->lm_counter_pool) {
        shr_idxres_list_destroy(oc->lm_counter_pool);
        oc->lm_counter_pool = NULL;
    }

    if (soc_feature(unit, soc_feature_bhh)) {
#if defined(INCLUDE_BHH)
        /* Destroy bhh indices list. */
        if (NULL != oc->bhh_pool) {
            shr_idxres_list_destroy(oc->bhh_pool);
            oc->bhh_pool = NULL;
        }
        soc_cm_sfree(oc->unit, oc->dma_buffer);
        soc_cm_sfree(oc->unit, oc->dmabuf_reply); 
#endif
    }

    /* Free group memory. */
    if (NULL != oc->group_info) {
        sal_free(oc->group_info);
    }

    /* Free RMEP H/w to logical index mapping memory. */
    if (NULL != oc->remote_endpoints) {
        sal_free(oc->remote_endpoints);
    }

    /* Destroy ingress serivce priority mapping profile. */
    if (NULL != oc->ing_service_pri_map.tables) {
        soc_profile_mem_destroy(unit, &oc->ing_service_pri_map);
    }

    /* Destroy OAM opcode control profile. */
    if (NULL != oc->oam_opcode_control_profile.tables) {
        soc_profile_mem_destroy(unit, &oc->oam_opcode_control_profile);
    }

    /* Free OAM control structure memory. */
    sal_free(oc);

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_tr3_oam_handle_interrupt
 * Purpose:
 *     Process OAM interrupts generated by endpoints.
 * Parameters:
 *     unit  -  (IN) BCM unit number.
 *     field -  (IN) fault field.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_handle_interrupt(int unit, soc_field_t field)
{
    _bcm_oam_interrupt_t *intr;             /* OAM interrupt.                */
    _bcm_oam_control_t   *oc;               /* OAM control structure.        */
    uint32               intr_rval;         /* Interrupt register value.     */
    uint32               intr_cur_status;   /* Interrupt status.             */
    uint32               flags;             /* Interrupt flags.              */
    bcm_oam_group_t      grp_index;         /* MA group index.               */
    bcm_oam_endpoint_t   remote_ep_index;   /* Remote Endpoint index.        */
    int                  intr_multi;        /* Event occured multiple times. */
    int                  intr_count;        /* No. of times event detected.  */
    int                  rv;                /* Operation return status       */
    _bcm_oam_event_handler_t *e_handler;    /* Pointer to Event handler.     */


    /* Get OAM control structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    /* Loop through the supported interrupts for this device. */
    for (intr = _tr3_oam_interrupts; intr->status_register != INVALIDr;
         intr++) {

        rv = soc_reg32_get(unit, intr->status_register,
                           REG_PORT_ANY, 0, &intr_rval);
        if (BCM_FAILURE(rv)) {
            
            continue;
        }

        /* Get  status of interrupt from hardware. */
        intr_cur_status = soc_reg_field_get(unit, intr->status_register,
                                            intr_rval, VALIDf);
        if (0 == intr_cur_status) {
            /* This interrupt event is not valid, so continue. */
            continue;
        }

        /* Get this interrupt event counter value. */
        intr_count = oc->event_handler_cnt[intr->event_type];

        /* Check if the interrupt is set. */
        if ((1 == intr_cur_status) && (intr_count > 0)) {
            flags = 0;

            /* Get MA group index for this interrupt. */
            if (INVALIDf != intr->group_index_field ) {
                grp_index = soc_reg_field_get(unit, intr->status_register,
                                              intr_rval,
                                              intr->group_index_field);
            } else {
                /* Group not valid for this interrupt. */
                grp_index = BCM_OAM_GROUP_INVALID;
            }

            /* Get H/w index of RMEP for this interrupt. */
            if (INVALIDf != intr->endpoint_index_field ) {
                remote_ep_index = soc_reg_field_get(unit, intr->status_register,
                                              intr_rval,
                                              intr->endpoint_index_field);

                /* Get Logical index from H/w index for this RMEP. */
                remote_ep_index = oc->remote_endpoints[remote_ep_index];
            } else {
                /* Endpoint not valid for this interrupt. */
                remote_ep_index = BCM_OAM_ENDPOINT_INVALID;
            }

            /* Get interrupt MULTIf status. */
            intr_multi = soc_reg_field_get(unit, intr->status_register,
                                           intr_rval, MULTIf);
            if (1 == intr_multi) {
                /*
                 * Interrupt event asserted more than once.
                 * Set flags status bit to indicate event multiple occurance.
                 */
                flags |= BCM_OAM_EVENT_FLAGS_MULTIPLE;
            }

            /* Check and call all the handlers registerd for this event. */
            for (e_handler = oc->event_handler_list_p; e_handler != NULL;
                 e_handler = e_handler->next_p) {

                /* Check if an event handler is register for this event type. */
                if (SHR_BITGET(e_handler->event_types.w, intr->event_type)) {

                    /* Call the event handler with the call back parameters. */
                    e_handler->cb(unit, flags, intr->event_type, grp_index,
                                      remote_ep_index, e_handler->user_data);
                }
            }
        }

        rv = soc_reg32_set(unit, intr->status_register,
                           REG_PORT_ANY, 0, 0);
        if (BCM_FAILURE(rv)) {
            continue;
        }
    }

    _BCM_OAM_UNLOCK(oc);

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_tr3_oam_events_unregister
 * Purpose:
 *     Unregister all OAM events for this unit.
 * Parameters:
 *     unit -  (IN) BCM unit number.
 *     oc   -  (IN) Pointer to OAM control structure.
 * Retruns:
 *     BCM_E_NONE
 */
STATIC int
_bcm_tr3_oam_events_unregister(int unit, _bcm_oam_control_t *oc)
{
    _bcm_oam_event_handler_t *e_handler; /* Pointer to event handler list. */
    _bcm_oam_event_handler_t *e_delete;  /* Event handler to be freed.     */

    /* Control lock taken by the calling routine. */

    e_handler = oc->event_handler_list_p;

    while (e_handler != NULL) {
        e_delete = e_handler;
        e_handler = e_handler->next_p;
        sal_free(e_delete);
    }

    oc->event_handler_list_p = NULL;

    return (BCM_E_NONE);
}


/*
 * Function:
 *     _bcm_tr3_oam_clear_rmep
 * Purpose:
 *     Delete RMEP entry or Install fresh RMEP entry in the Hardware, clearing 
 *     the faults.
 *      
 * Parameters:
 *     unit       - (IN) BCM device number
 *     h_data_p   - (IN) Pointer to Endpoint info associated with RMEP entry 
 *                       being cleared. 
 *     valid      - (IN) 0 => Delete the H/w entry
 *                       1 => Install fresh entry with faults cleared.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_clear_rmep (int unit, _bcm_oam_hash_data_t *h_data_p, int valid)
{

    rmep_entry_t       rmep_entry;          /* RMEP table entry.              */
    int                rv = BCM_E_INTERNAL; /* Operation return status entry  */
    uint32             oam_cur_time;    /* Current time in H/w state machine  */

    OAM_VERB(("OAM(unit %d): %s, EP id %d, valid %d\n", 
               unit, __FUNCTION__, h_data_p->remote_index, valid));
    
    if (h_data_p == NULL) {
        OAM_ERR(("OAM(unit %d), ERR: %s Arg h_data_p NULL check failed\n",
                 unit, __FUNCTION__));
        return BCM_E_INTERNAL;
    }

    /* RMEP table programming. */
    sal_memset(&rmep_entry, 0, sizeof(rmep_entry_t));

    /* if valid==0 delete RMEP entry, else create a clean RMEP entry */
    if (!(valid)) {
        rv = WRITE_RMEPm(unit, MEM_BLOCK_ALL, h_data_p->remote_index,
                         &rmep_entry);
        if (rv != BCM_E_NONE) {
            OAM_ERR(("OAM(unit %d), ERR: %s Deleting RMEP entry failied\n",
                      unit, __FUNCTION__));
        }
        return (rv);
    }

    /* Set the MA group index. */
    soc_RMEPm_field32_set(unit, &rmep_entry, MAID_INDEXf, 
                          h_data_p->group_index);

    /*
     * The following steps are necessary to enable CCM timeout events
     * without having received any CCMs.
     */
    soc_RMEPm_field32_set(unit, &rmep_entry, RMEP_TIMESTAMP_VALIDf, 1);

    BCM_IF_ERROR_RETURN(READ_OAM_CURRENT_TIMEr(unit, &oam_cur_time));

    soc_RMEPm_field32_set(unit, &rmep_entry, RMEP_TIMESTAMPf, oam_cur_time);

    soc_RMEPm_field32_set(unit, &rmep_entry, RMEP_RECEIVED_CCMf,
                         _bcm_tr3_oam_ccm_msecs_to_hw_encode(h_data_p->period));
    /* End of timeout setup */


    soc_RMEPm_field32_set(unit, &rmep_entry, VALIDf, 1);

    rv = WRITE_RMEPm(unit, MEM_BLOCK_ALL, h_data_p->remote_index,
                     &rmep_entry);
    if (rv != BCM_E_NONE) {
        OAM_ERR(("OAM(unit %d), ERR: %s Clearing RMEP entry failied\n",
                  unit, __FUNCTION__));
        return (rv);
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *     _bcm_tr3_oam_clear_ma_state
 * Purpose:
 *     Delete MA_STATE entry or Install fresh MA_STATE entry in the Hardware, 
 *     Clearing the faults.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     group_info - (IN) Group info associated with MA_STATE entry being 
 *                       cleared.
 *     index      - (IN) H/w index of MA_STATE entry to be modified. 
 *     valid      - (IN) 0 => Delete the H/w entry
 *                       1 => Install fresh entry with faults cleared.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_clear_ma_state(int unit, _bcm_oam_group_data_t *group_info, 
                            int index, int valid)
{

    ma_state_entry_t       ma_state_entry;       /* MA State table entry.     */
    
    OAM_VERB(("OAM(unit %d): %s, *group_info %p, index %d, valid %d\n", 
               unit, __FUNCTION__, group_info, index, valid));
               
    if (group_info == NULL) {
        OAM_ERR(("OAM(unit %d), ERR: %s Arg group_info NULL check failed\n",
                 unit, __FUNCTION__));
        return BCM_E_INTERNAL;
    }

    sal_memset(&ma_state_entry, 0, sizeof(ma_state_entry_t));
    
    /* Mark the entry as valid. */
    soc_MA_STATEm_field32_set(unit, &ma_state_entry, VALIDf, valid);

    /* Set the lowest alarm priority info. */
    if (valid) {
        soc_MA_STATEm_field32_set(unit, &ma_state_entry, LOWESTALARMPRIf,
                                  group_info->lowest_alarm_priority);
    }

    /* Write group information to hardware table. */
    SOC_IF_ERROR_RETURN(WRITE_MA_STATEm(unit, MEM_BLOCK_ALL, index,
                                        &ma_state_entry));
    return BCM_E_NONE;
}


/*
 * Function:
 *     _bcm_tr3_oam_group_recreate
 * Purpose:
 *     Recreate MA_STATE and RMEP entries, due to current H/w limitation
 *     Currently there is a race condition between H/w and Software
 *     When RMEP is deleted and if it has any faults then, 
 *     MA_STATE remote fault counters need to be decremented by software.
 *     However it may so happen that after reading RMEP faults and just
 *     before RMEP deletion, the fault gets cleared and H/w already decremented
 *     the MA_STATE table, now when software decrements the MA_STATE it would
 *     get MA_STATE table into an inconsistent state.
 *     As a solution we always recreate the entire group on a RMEP deletion.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     group_id - (IN) Group Index, 
 *                as per current implementation this is same as MA_STATE index.
 * Returns:
 *     BCM_E_XXX
 * Expects:
 *      OAM LOCK
 */
STATIC int
_bcm_tr3_oam_group_recreate(int unit, int index)
{
    _bcm_oam_control_t      *oc;                /* OAM control structure.     */
    _bcm_oam_group_data_t   *group_info;        /* OAM group info             */
    _bcm_oam_ep_list_t      *cur_ep_ptr = NULL; /* Pointer to group's EP list */
                                                /* entry.                     */
    int                     rv = BCM_E_NONE;    /* Operation return status.   */

    /* Get OAM control structure handle. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Get the handle to group info */
    group_info = &(oc->group_info[index]);
    
    /*if unused Group, just clear the MA_STATE index, including the valid bit*/
    if (!(group_info->in_use)) {
        OAM_WARN(("OAM(unit %d), WARN: Recieved group recreate request for " 
                  "unused Group Id %d\n", unit, index));
    
        rv = _bcm_tr3_oam_clear_ma_state(unit, group_info, index, 0);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: MA_STATE clear failed group id %d - "
                     "%s.\n", unit, index, bcm_errmsg(rv)));
            return rv;
        }

    }
    
    /* 1. Traverse and delete the RMEPs in the group */
    cur_ep_ptr = *(group_info->ep_list);
    while (cur_ep_ptr) {
        if (cur_ep_ptr->ep_data_p->is_remote && cur_ep_ptr->ep_data_p->in_use) {
            rv = _bcm_tr3_oam_clear_rmep(unit, cur_ep_ptr->ep_data_p, 0);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: RMEP delete failed Ep id %d - "
                "%s.\n", unit, cur_ep_ptr->ep_data_p->ep_id, bcm_errmsg(rv)));
                return rv;
            }
        }
        cur_ep_ptr = cur_ep_ptr->next;
    }

    /* 2. Clear the MA_STATE table faults */
    rv = _bcm_tr3_oam_clear_ma_state(unit, group_info, index, 1);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: MA_STATE clear failed group id %d - "
                 "%s.\n", unit, index, bcm_errmsg(rv)));
        return rv;
    }

    /* 3. Recreate the RMEP table entries. Clearing the faults. */
    cur_ep_ptr = *(group_info->ep_list);
    while (cur_ep_ptr) {
        if (cur_ep_ptr->ep_data_p->is_remote && cur_ep_ptr->ep_data_p->in_use) {
            rv = _bcm_tr3_oam_clear_rmep(unit, cur_ep_ptr->ep_data_p, 1);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: RMEP clear failed EP id %d - "
                "%s.\n", unit, cur_ep_ptr->ep_data_p->ep_id, bcm_errmsg(rv)));
                return rv;
            }
        }
        cur_ep_ptr = cur_ep_ptr->next;
    }
    
    return rv;
}


/*
 * Function:
 *     _bcm_tr3_oam_ser_handler
 * Purpose:
 *     Soft Error Correction routine for MA_STATE and RMEP tables.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     mem        - (IN) Memory MA_STATE or RMEP.
 *     index      - (IN) Memory Index.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_ser_handler(int unit, soc_mem_t mem, int index)
{
    int                     rv = BCM_E_NONE;    /* Operation return status.   */
    _bcm_oam_control_t      *oc = NULL;         /* OAM control structure.     */
    bcm_oam_endpoint_t      remote_ep_index;    /* Remote Endpoint index.     */
    _bcm_oam_hash_data_t    *ep_data = NULL;    /* Remote Endpoint hash data  */

    OAM_VERB(("OAM(unit %d) SER on mem %s, index %d\n", 
              unit, SOC_MEM_NAME(unit, mem), index));
              
    /* Get OAM control structure handle. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    switch(mem){
        case MA_STATEm:
            _BCM_OAM_GROUP_INDEX_VALIDATE(index);
            _BCM_OAM_LOCK(oc);
            rv = _bcm_tr3_oam_group_recreate(unit, index);
            _BCM_OAM_UNLOCK(oc);
            break;
            
        case RMEPm:
            _BCM_OAM_RMEP_INDEX_VALIDATE(index);
            _BCM_OAM_LOCK(oc);
            /* Get the logical index from H/w RMEP index */
            remote_ep_index = oc->remote_endpoints[index];
            /* Get handle to Hash data */
            ep_data = &(oc->oam_hash_data[remote_ep_index]);
            
            if (!(ep_data->in_use)) {
                /* If endpoint not in use, just clear the RMEP entry in memory */
                OAM_WARN(("OAM(unit %d), WARN: Recieved Parity Error on"
                          "unused Remote Id %d\n", unit, remote_ep_index));
                /* Just clear this entry including valid bit */
                rv = _bcm_tr3_oam_clear_rmep(unit, ep_data, 0);
            } else {
                /* Get the group Id from EP data, call group recreate routine */
                rv = _bcm_tr3_oam_group_recreate(unit, ep_data->group_index);
            }
            _BCM_OAM_UNLOCK(oc);
            break;

        default:
            OAM_ERR(("OAM(unit %d), ERR: Invalid mem in OAM SER correction "
                     "routine %s\n", unit, SOC_MEM_NAME(unit, mem)));
            return BCM_E_INTERNAL;
    }

    OAM_VERB(("OAM(unit %d) SER completed on mem %s, index %d, rv %d\n", 
              unit, SOC_MEM_NAME(unit, mem), index, rv));
    
    return rv;
    
}


/*
 * Function:
 *     _bcm_tr3_oam_group_name_mangle
 * Purpose:
 *     Build tbe group name for hardware table write.
 * Parameters:
 *     name_p         -  (IN) OAM group name.
 *     mangled_name_p -  (IN/OUT) Buffer to write the group name in hardware
 *                       format.
 * Retruns:
 *     None
 */
STATIC void
_bcm_tr3_oam_group_name_mangle(uint8 *name_p,
                               uint8 *mangled_name_p)
{
    uint8  *byte_p;    /* Pointer to group name buffer.       */
    int    bytes_left; /* Number of bytes left in group name. */

    bytes_left = BCM_OAM_GROUP_NAME_LENGTH;
    byte_p = (name_p + BCM_OAM_GROUP_NAME_LENGTH - 1);

    while (bytes_left > 0) {
        *mangled_name_p = *byte_p;
        ++mangled_name_p;
        --byte_p;
        --bytes_left;
    }
}

/*
 * Function:
 *      _bcm_tr3_oam_read_clear_faults
 * Purpose:
 *     Clear OAM group and endpoint faults on hardware table read operation.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     index    - (IN) Group/Endpoint hardware table index
 *     mem      - (IN) Memory/table value
 *     entry    - (IN) Pointer to group/endpoint entry
 *     ma_mep   - (IN) Pointer to group/endpoint info structure
 * Returns:
 *     BCM_E_NONE      - No errors.
 *     BCM_E_XXX       - Otherwise.
 */
STATIC int
_bcm_tr3_oam_read_clear_faults(int unit, int index, soc_mem_t mem,
                               uint32 *entry, void *ma_rmep)
{
    bcm_oam_group_info_t    *group_info_p;  /* Pointer to group info         */
                                            /* structure.                    */
    bcm_oam_endpoint_info_t *ep_info_p;     /* Pointer to endpoint info      */
                                            /* structure.                    */
    _bcm_oam_fault_t        *faults_list;   /* Pointer to faults list.       */
    uint32                  *faults;        /* Faults flag bits.             */
    uint32                  *p_faults;      /* Persistent faults bits.       */
    uint32                  clear_p_faults; /* Clear persistent faults bits. */
    uint32                  rval = 0;       /* Hardware register value.      */

    OAM_VVERB(("OAM(unit %d) Info: _bcm_tr3_oam_read_clear_faults index=%d "
              "Table=%d.\n", unit, index, mem));

    /* Switch on memory name. */
    switch (mem) {
        /* OAM group state table. */
        case MA_STATEm:
            /* Set the pointer to the start of the group faults array. */
            faults_list = _tr3_oam_group_faults;

            /* Typecast to group information structure pointer. */
            group_info_p = (bcm_oam_group_info_t *) ma_rmep;

            /* Get group faults information. */
            faults = &group_info_p->faults;
            p_faults = &group_info_p->persistent_faults;
            clear_p_faults = group_info_p->clear_persistent_faults;
            break;

        /* OAM remote endpoint table. */
        case RMEPm:
            faults_list = _tr3_oam_endpoint_faults;
            ep_info_p = (bcm_oam_endpoint_info_t *) ma_rmep;
            faults = &ep_info_p->faults;
            p_faults = &ep_info_p->persistent_faults;
            clear_p_faults = ep_info_p->clear_persistent_faults;
            break;

        default:
            return (BCM_E_NONE);
    }

    /* Loop on list of valid faults. */
    for (;faults_list->mask != 0; ++faults_list) {

        /* Get  faults status. */
        if (0 != soc_mem_field32_get(unit, mem, entry,
                                     faults_list->current_field)) {
            *faults |= faults_list->mask;
        }

        /* Get sticky faults status. */
        if (0 != soc_mem_field32_get(unit, mem, entry,
                                     faults_list->sticky_field)) {

            *p_faults = faults_list->mask;

            /*
             * If user has request to clear persistent faults,
             * then set BITS_TO_CLEARf field in the register buffer.
             */
            if (clear_p_faults & *p_faults) {
                soc_reg_field_set(unit, CCM_READ_CONTROLr,
                                  &rval, BITS_TO_CLEARf,
                                  faults_list->clear_sticky_mask);
            }
        }
    }

    /* Check if faults need to be cleared. */
    if (0 != rval) {

        /* Enable clearing of faults. */
        soc_reg_field_set(unit, CCM_READ_CONTROLr, &rval, ENABLE_CLEARf, 1);

        
        if (MA_STATEm == mem) {
            soc_reg_field_set(unit, CCM_READ_CONTROLr, &rval, MEMORYf, 0);
        } else {
            soc_reg_field_set(unit, CCM_READ_CONTROLr, &rval, MEMORYf, 1);
        }

        soc_reg_field_set(unit, CCM_READ_CONTROLr, &rval, INDEXf, index);

        /* Update read control register. */
        BCM_IF_ERROR_RETURN(WRITE_CCM_READ_CONTROLr(unit, rval));
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr3_oam_get_group
 * Purpose:
 *     Get OAM group information.
 * Parameters:
 *     unit         - (IN) BCM device number
 *     group_index  - (IN) Group hardware table index
 *     group        - (IN) Pointer to group array list
 *     group_info   - (IN/OUT) Pointer to group info structure
 * Returns:
 *     BCM_E_NONE      - No errors.
 *     BCM_E_XXX       - Otherwise.
 */
STATIC int
_bcm_tr3_oam_get_group(int unit, bcm_oam_group_t group_index,
                       _bcm_oam_group_data_t *group_p,
                       bcm_oam_group_info_t *group_info)
{
    maid_reduction_entry_t  maid_reduction_entry; /* MAID reduction entry.    */
    ma_state_entry_t        ma_state_entry;       /* MA_STATE table entry.    */
    int                     rv;                   /* Operation return status. */

    group_info->id = group_index;

    sal_memcpy(group_info->name, group_p[group_index].name,
               BCM_OAM_GROUP_NAME_LENGTH);

    BCM_IF_ERROR_RETURN(READ_MAID_REDUCTIONm(unit, MEM_BLOCK_ANY,
                                             group_index,
                                             &maid_reduction_entry));

    if (1 == soc_MAID_REDUCTIONm_field32_get(unit, &maid_reduction_entry,
                                             SW_RDIf)) {
        group_info->flags |= BCM_OAM_GROUP_REMOTE_DEFECT_TX;
    }

    BCM_IF_ERROR_RETURN(READ_MA_STATEm(unit, MEM_BLOCK_ANY,
                                       group_index,
                                       &ma_state_entry));

    group_info->lowest_alarm_priority = soc_MA_STATEm_field32_get
                                            (unit, &ma_state_entry,
                                             LOWESTALARMPRIf);

    rv = _bcm_tr3_oam_read_clear_faults(unit, group_index,
                                        MA_STATEm,
                                        (uint32 *) &ma_state_entry,
                                        group_info);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Clean Faults Group ID=%d- Failed.\n",
                 unit, group_index));
        return (rv);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_oam_group_ep_list_add
 * Purpose:
 *     Add an endpoint to a group endpoint linked list.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     group_id - (IN) OAM group ID.
 *     ep_id    - (IN) OAM endpoint ID.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_oam_group_ep_list_add(int unit,
                           bcm_oam_group_t group_id,
                           bcm_oam_endpoint_t ep_id)
{
    _bcm_oam_control_t    *oc;       /* Pointer to OAM control structure. */
    _bcm_oam_group_data_t *group_p;  /* Pointer to group data.            */
    _bcm_oam_hash_data_t  *h_data_p; /* Pointer to endpoint hash data.    */
    _bcm_oam_ep_list_t    *ep_list_p = NULL; /* Pointer to endpoint list. */

    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    group_p = &oc->group_info[group_id];
    if (NULL == group_p) {
        OAM_ERR(("OAM(unit %d) Error: Group data access for GID=%d failed"
                " %s.\n", unit, group_id, bcm_errmsg(BCM_E_INTERNAL)));
        return (BCM_E_INTERNAL);
    }

    h_data_p = &oc->oam_hash_data[ep_id];
    if (NULL == h_data_p) {
        OAM_ERR(("OAM(unit %d) Error: Endpoint data access for EP=%d failed"
                " %s.\n", unit, ep_id, bcm_errmsg(BCM_E_INTERNAL)));
        return (BCM_E_INTERNAL);
    }

    _BCM_OAM_ALLOC(ep_list_p, _bcm_oam_ep_list_t, sizeof(_bcm_oam_ep_list_t),
                "EP list");
    if (NULL == ep_list_p) {
        OAM_ERR(("OAM(unit %d) Error: Endpoint list alloc for EP=%d failed"
                " %s.\n", unit, ep_id, bcm_errmsg(BCM_E_MEMORY)));
        return (BCM_E_MEMORY);
    }

    ep_list_p->prev = NULL;
    ep_list_p->ep_data_p = h_data_p;
    if (NULL == (*group_p->ep_list)) {
        /* Add endpoint as head node. */
        ep_list_p->next = NULL;
        *group_p->ep_list = ep_list_p;
    } else {
        /* Add the endpoint to the linked list. */
        ep_list_p->next = *group_p->ep_list;
        (*group_p->ep_list)->prev = ep_list_p;
        *group_p->ep_list = ep_list_p;
    }

    OAM_VVERB(("OAM(unit %d) Info: _bcm_oam_group_ep_list_add (GID=%d) (EP=%d).\n",
              unit, group_id, ep_id));
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_oam_group_ep_list_remove
 * Purpose:
 *     Remove an endpoint to a group endpoint linked list.
 * Parameters:
 *     unit     - (IN) BCM device number
 *     group_id - (IN) OAM group ID.
 *     ep_id    - (IN) OAM endpoint ID.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_oam_group_ep_list_remove(int unit,
                              bcm_oam_group_t group_id,
                              bcm_oam_endpoint_t ep_id)
{
    _bcm_oam_control_t    *oc;       /* Pointer to OAM control structure. */
    _bcm_oam_group_data_t *group_p;  /* Pointer to group data.            */
    _bcm_oam_hash_data_t  *h_data_p; /* Pointer to endpoint hash data.    */
    _bcm_oam_ep_list_t    *cur;      /* Current endpoint node pointer.    */
    _bcm_oam_ep_list_t    *del_node; /* Pointer to a node to be deleted.  */

    /* Control lock already taken by calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    group_p = &oc->group_info[group_id];
    cur =  *group_p->ep_list;
    if (NULL == cur) {
        /* No endpoints to remove from this group. */
        OAM_VVERB(("OAM(unit %d) Info: No endpoints to delete in group"
                 " GID:%d.\n", unit, group_id));
        return (BCM_E_NONE);
    }

    /* Check if head node needs to be deleated. */
    if (ep_id == cur->ep_data_p->ep_id) {
        /* Delete head node. */
        del_node = *group_p->ep_list;
        if ((*group_p->ep_list)->next) {
            *group_p->ep_list = (*group_p->ep_list)->next;
            (*group_p->ep_list)->prev = NULL;
        } else {
            *group_p->ep_list = NULL;
        }
        sal_free(del_node);
        OAM_VVERB(("OAM(unit %d) Info: Head node delete GID=%d - Success\n",
                 unit, group_id));
        return (BCM_E_NONE);
    }

    /* Traverse the list and delete the matching node. */
    while (NULL != cur->next->next) {
        h_data_p = cur->next->ep_data_p;
        if (NULL == h_data_p) {
            OAM_ERR(("OAM(unit %d) Error: Group=%d endpoints access failed -"
                    " %s.\n", unit, group_id, bcm_errmsg(BCM_E_INTERNAL)));
            return (BCM_E_INTERNAL);
        }

        if (ep_id == h_data_p->ep_id) {
            del_node = cur->next;
            cur->next = del_node->next;
            del_node->next->prev = cur;
            sal_free(del_node);
            OAM_VVERB(("OAM(unit %d) Info: Node delete GID=%d - Success\n",
                    unit, group_id));
            return (BCM_E_NONE);
        }
        cur = cur->next;
    }

    /* Check if tail node needs to be deleted. */
    h_data_p = cur->next->ep_data_p;
    if (ep_id == h_data_p->ep_id) {
        /* Delete tail node. */
        del_node = cur->next;
        cur->next = NULL;
        sal_free(del_node);
        OAM_VVERB(("OAM(unit %d) Info: Tail node delete GID=%d - Success\n",
                 unit, group_id));
        return (BCM_E_NONE);
    }

    return (BCM_E_NOT_FOUND);
}

/*
 * Function:
 *     _bcm_oam_remote_mep_hw_set
 * Purpose:
 *     Configure hardware tables for a remote endpoint.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     ep_info_p - (IN) Pointer to remote endpoint information.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_oam_remote_mep_hw_set(int unit, 
                           const bcm_oam_endpoint_info_t *ep_info_p)
{
    uint32 l3_entry[SOC_MAX_MEM_WORDS];     /* Remote view entry.            */
    rmep_entry_t         rmep_entry;        /* Remote MEP entry.             */
    uint32               oam_cur_time;      /* Current time.                 */
    _bcm_oam_hash_data_t *h_data_p = NULL;  /* Hash data pointer.            */
    int                  rv;                /* Operation return status.      */
    _bcm_oam_control_t   *oc;               /* Pointer to control structure. */
    const bcm_oam_endpoint_info_t *ep_p;    /* Pointer to endpoint info.     */
    soc_mem_t            mem;               /* L3 entry memory               */

    if (NULL == ep_info_p) {
        return (BCM_E_INTERNAL);
    }

    /* Initialize local endpoint info pointer. */
    ep_p = ep_info_p;

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Get endpoint hash data pointer. */
    h_data_p = &oc->oam_hash_data[ep_p->id];
    if (!(h_data_p->in_use)) {
        OAM_ERR(("OAM(unit %d) Error: %s EP valid check failed.\n", unit, __FUNCTION__));
        return (BCM_E_INTERNAL);
    }

    /* RMEP table programming. */
    sal_memset(&rmep_entry, 0, sizeof(rmep_entry_t));

    /* Set the MA group index. */
    soc_RMEPm_field32_set(unit, &rmep_entry, MAID_INDEXf,
                          ep_p->group);

    /*
     * The following steps are necessary to enable CCM timeout events
     * without having received any CCMs.
     */
    soc_RMEPm_field32_set(unit, &rmep_entry,
                          RMEP_TIMESTAMP_VALIDf, 1);

    BCM_IF_ERROR_RETURN
        (READ_OAM_CURRENT_TIMEr(unit, &oam_cur_time));

    soc_RMEPm_field32_set(unit, &rmep_entry, RMEP_TIMESTAMPf,
                          oam_cur_time);

    soc_RMEPm_field32_set(unit, &rmep_entry,
                          RMEP_RECEIVED_CCMf,
                         _bcm_tr3_oam_ccm_msecs_to_hw_encode(h_data_p->period));
    /* End of timeout setup */

    soc_RMEPm_field32_set(unit, &rmep_entry, VALIDf, 1);

    rv = WRITE_RMEPm(unit, MEM_BLOCK_ALL, h_data_p->remote_index,
                     &rmep_entry);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: RMEP table write failed EP=%d %s.\n",
                unit, ep_info_p->id, bcm_errmsg(rv)));
        return (rv);
    }
    if (SOC_IS_HURRICANE2(unit)) {
        mem = L3_ENTRY_IPV4_UNICASTm;
    } else {
        mem = L3_ENTRY_1m;
    }
    /* L3 unicast table programming. */
    sal_memset(&l3_entry, 0, sizeof(l3_entry));
    
    /* Set the CCM interval. */
    soc_mem_field32_set
        (unit, mem, &l3_entry, RMEP__CCMf,
        _bcm_tr3_oam_ccm_msecs_to_hw_encode(h_data_p->period));

    /* Set the entry hardware index. */
    soc_mem_field32_set(unit, mem, &l3_entry, RMEP__RMEP_PTRf,
                                h_data_p->remote_index);

    /*
     * Construct endpoint RMEP view key for L3 Table entry
     * insert operation.
     */
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        _bcm_hu2_oam_rmep_key_construct(unit, h_data_p, 
                                    (l3_entry_ipv4_unicast_entry_t *)l3_entry);
    } else 
#endif /* BCM_HURRICANE2_SUPPORT */
    {
#ifdef BCM_TRIUMPH3_SUPPORT

        _bcm_oam_rmep_key_construct(unit, h_data_p, 
                                   (l3_entry_1_entry_t *)l3_entry);
#endif /* BCM_HURRICANE2_SUPPORT */
    }
    /* Mark the entry as valid. */
    soc_mem_field32_set(unit, mem, &l3_entry, VALIDf, 1);

    /* Install entry in hardware. */
    rv = soc_mem_insert(unit, mem, 
                        MEM_BLOCK_ALL, &l3_entry);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: L3 table insert failed EP=%d %s.\n",
                unit, ep_p->id, bcm_errmsg(rv)));
        return (rv);
    }
    
    /* Add the H/w index to logical index mapping for RMEP */
    oc->remote_endpoints[h_data_p->remote_index] = ep_p->id; 

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_tr3_oam_find_lmep
 * Purpose:
 *     Search LMEP view table and return the match entry hardware index and
 *     match entry data.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     h_data_p   - (IN) Pointer to endpoint hash data.
 *     entry_idx  - (IN/OUT) Pointer to match entry hardware index.
 *     l3_entry_p - (IN/OUT) Pointer to match entry data.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_find_lmep(int unit, const _bcm_oam_hash_data_t *h_data_p,
                       int *entry_idx,
                       uint32 *l3_entry_p)
{
    uint32             l3_entry[SOC_MAX_MEM_WORDS];    /* L3 entry to search.*/
    int                rv;       /* Operation return status. */

    if (NULL == h_data_p || NULL == entry_idx || NULL == l3_entry_p) {
        return (BCM_E_INTERNAL);
    }

    sal_memset(&l3_entry, 0, sizeof(l3_entry));

    /* Construct endpoint LMEP view key for L3 Table entry search operation. */
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        _bcm_hu2_oam_lmep_key_construct(unit, h_data_p, 
                                    (l3_entry_ipv4_unicast_entry_t *)l3_entry);
    } else 
#endif /* BCM_HURRICANE2_SUPPORT */
    {
#ifdef BCM_TRIUMPH3_SUPPORT
        _bcm_oam_lmep_key_construct(unit, h_data_p, 
                                    (l3_entry_1_entry_t *)l3_entry);
#endif /* BCM_TRIUMPH3_SUPPORT */
    }
    /* Take L3 module protection mutex to block any updates. */
    _bcm_esw_l3_lock(unit);

    /* Perform the search in L3_ENTRY_1 table. */
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        rv = soc_mem_search(unit, L3_ENTRY_IPV4_UNICASTm, MEM_BLOCK_ANY, 
                            entry_idx, &l3_entry, l3_entry_p, 0);
    } else 
#endif /* BCM_HURRICANE2_SUPPORT */
    {
        rv = soc_mem_search(unit, L3_ENTRY_1m, MEM_BLOCK_ANY, entry_idx,
                        &l3_entry, l3_entry_p, 0);

    }
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: L3 entry lookup vlan=%d port=%x %s.\n",
                unit, h_data_p->vlan, h_data_p->sglp, bcm_errmsg(rv)));
    }

    /* Release L3 module protection mutex. */
    _bcm_esw_l3_unlock(unit);

    return (rv);
}

/*
 * Function:
 *     _bcm_oam_local_tx_mep_hw_set
 * Purpose:
 *     Configure hardware tables for a local CCM Tx enabled endpoint.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     ep_info_p - (IN) Pointer to remote endpoint information.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_oam_local_tx_mep_hw_set(int unit,
                             const bcm_oam_endpoint_info_t *ep_info_p)
{
    _bcm_oam_hash_data_t *hash_data; /* Pointer to endpoint hash data. */
    lmep_entry_t         entry;      /* LMEP table entry.              */
    lmep_da_entry_t      dmac_entry; /* LMEP table entry.              */
    int                  word;       /* Word index.                    */
    uint32               reversed_maid[BCM_OAM_GROUP_NAME_LENGTH / 4];
                                     /* Group name in Hw format.       */
    _bcm_oam_control_t   *oc;        /* Pointer to control structure.  */
    const bcm_oam_endpoint_info_t *ep_p;/* Pointer to endpoint info.   */

    if (NULL == ep_info_p) {
        return (BCM_E_INTERNAL);
    }

    /* Initialize local endpoint info pointer. */
    ep_p = ep_info_p;

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));


    /* Get the stored endpoint information from hash table. */
    hash_data = &oc->oam_hash_data[ep_p->id];
    if (!(hash_data->in_use)) {
        OAM_ERR(("OAM(unit %d) Error: %s EP valid check failed.\n", unit, __FUNCTION__));
        return (BCM_E_INTERNAL);
    }

    sal_memset(&entry, 0, sizeof(lmep_entry_t));

    /* Set the Group base index. */
    soc_LMEPm_field32_set(unit, &entry, MAID_INDEXf, ep_p->group);

    /* Set source MAC address to be used on transmitted packets. */
    soc_LMEPm_mac_addr_set(unit, &entry, SAf, ep_p->src_mac_address);

    /* Set the Maintenance Domain Level. */
    soc_LMEPm_field32_set(unit, &entry, MDLf, ep_p->level);

    /* Set endpoint name. */
    soc_LMEPm_field32_set(unit, &entry, MEPIDf, ep_p->name);

    /* Set packet priority value to be used in transmitted packet. */
    soc_LMEPm_field32_set(unit, &entry, LMEP_PRIORITYf,
                          ep_p->pkt_pri);

    /*
     * Set VLAN ID to be used in the transmitted packet.
     * For link level MEPs, this VLAN_ID == 0.
     */
    soc_LMEPm_field32_set(unit, &entry, VLAN_IDf, ep_p->vlan);

    /* Set interval between CCM packet transmissions. */
    soc_LMEPm_field32_set
        (unit, &entry, CCM_INTERVALf,
        _bcm_tr3_oam_ccm_msecs_to_hw_encode(ep_p->ccm_period));

    /* Set the destination port on which packet needs to Tx. */
    soc_LMEPm_field32_set(unit, &entry, DESTf, hash_data->dglp);

    /* Set HiGig opcode to Unicast packet opcode. */
    soc_LMEPm_field32_set(unit, &entry, MH_OPCODEf, BCM_HG_OPCODE_UC);

    /* Set CCM packet internal priority used for scheduling. */
    soc_LMEPm_field32_set(unit, &entry, INT_PRIf, ep_p->int_pri);

    /* Set Port status TLV in CCM Tx packets. */
    if (ep_p->flags & BCM_OAM_ENDPOINT_PORT_STATE_UPDATE) {
        if (ep_p->port_state > BCM_OAM_PORT_TLV_UP) {
            return (BCM_E_PARAM);
        }
        soc_LMEPm_field32_set(unit, &entry, PORT_TLVf,
                              (ep_p->port_state == BCM_OAM_PORT_TLV_UP)
                              ? 1 : 0);
    }

    /* Set Interface status TLV in CCM Tx packets - 3-bits wide. */
    if (ep_p->flags & BCM_OAM_ENDPOINT_INTERFACE_STATE_UPDATE) {
        if ((ep_p->interface_state < BCM_OAM_INTERFACE_TLV_UP)
            || (ep_p->interface_state > BCM_OAM_INTERFACE_TLV_LLDOWN)) {
            return (BCM_E_PARAM);
        }
        soc_LMEPm_field32_set(unit, &entry, INTERFACE_TLVf,
                              ep_p->interface_state);
    }

    /*
     * When this bit is '1', both port and interface TLV values are set in
     *  CCM Tx packets.
     */
    if ((ep_p->flags & BCM_OAM_ENDPOINT_PORT_STATE_TX)
        || (ep_p->flags & BCM_OAM_ENDPOINT_INTERFACE_STATE_TX)) {
        soc_LMEPm_field32_set(unit, &entry, INSERT_TLVf, 1);
    }

    /*
     * Construct group name for hardware.
     * i.e Word-reverse the MAID bytes for hardware.
     */
    for (word = 0; word < (BCM_OAM_GROUP_NAME_LENGTH / 4);
         ++word) {
        reversed_maid[word]
            = ((uint32 *) oc->group_info[ep_p->group].name)
                [((BCM_OAM_GROUP_NAME_LENGTH / 4) - 1) - word];
    }

    /* Set the group name. */
    soc_LMEPm_field_set(unit, &entry, MAIDf, reversed_maid);

    /* Write entry to hardware LMEP table. */
    SOC_IF_ERROR_RETURN
        (WRITE_LMEPm(unit, MEM_BLOCK_ALL, hash_data->local_tx_index,
                     &entry));

    /* LMEP_DA table programming. */
    sal_memset(&dmac_entry, 0, sizeof(lmep_da_entry_t));

    /* Set CCM packet Destination MAC address value. */
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        soc_mem_mac_addr_set(unit, LMEP_DAm, &dmac_entry, MACDAf,
                         ep_p->dst_mac_address);
 
    } else 
#endif /* BCM_HURRICANE2_SUPPORT */
    {
        soc_mem_mac_addr_set(unit, LMEP_DAm, &dmac_entry, DAf,
                         ep_p->dst_mac_address);
    }
    /*
     * Write the entry to hardware LMEP_DA table.
     * Use same hardware index as the LMEP table entry index.
     */
    SOC_IF_ERROR_RETURN
        (WRITE_LMEP_DAm(unit, MEM_BLOCK_ALL, hash_data->local_tx_index,
                       &dmac_entry));

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_oam_local_tx_mep_hw_set
 * Purpose:
 *     Configure hardware tables for a local CCM Rx enabled endpoint.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     ep_info_p - (IN) Pointer to remote endpoint information.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_oam_local_rx_mep_hw_set(int unit,
                             const bcm_oam_endpoint_info_t *ep_info_p)
{
    _bcm_oam_hash_data_t *h_data_p;      /* Endpoint hash data pointer.      */
    ma_index_entry_t     ma_idx_entry;   /* MA_INDEX table entry buffer.     */
    uint32               opcode_entry = 0; /* Opcode control profile entry.  */
    void                 *entries[1];    /* Pointer to opcode control entry. */
    uint32               profile_index;  /* opcode control profile index.    */
    uint32               l3_entry[SOC_MAX_MEM_WORDS];  /* L3 entry buffer.   */
    int                  l3_index = -1;  /* L3 entry hardware index.         */
    int                  rv;             /* Operation return status.         */
    uint8                mdl;            /* Maintenance domain level.        */
    _bcm_oam_control_t   *oc;            /* Pointer to control structure.    */
    const bcm_oam_endpoint_info_t *ep_p; /* Pointer to endpoint info.        */
    soc_mem_t            mem;            /* L3 entry memory                  */
    if (NULL == ep_info_p) {
        return (BCM_E_INTERNAL);
    }

    /* Initialize local endpoint info pointer. */
    ep_p = ep_info_p;

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Get endpoint hash data pointer. */
    h_data_p = &oc->oam_hash_data[ep_p->id];

    /* Construct the opcode control profile table entry. */
    if (ep_p->opcode_flags & _BCM_TR3_OAM_OPCODE_MASK) {
        /* Use application specified opcode control settings. */
        rv = _bcm_tr3_oam_opcode_profile_entry_set(unit,
                                                   ep_p->opcode_flags,
                                                   &opcode_entry);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Opcode profile set failed for EP=%d"
                    "  %s.\n", unit, ep_p->id, bcm_errmsg(rv)));
            return (rv);
        }
    } else {
        /* Use default opcode control profile settings. */
        rv = _bcm_tr3_oam_opcode_profile_entry_init(unit, &opcode_entry);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Opcode profile init failed for EP=%d"
                    "  %s.\n", unit, ep_p->id, bcm_errmsg(rv)));
            return (rv);
        }
    }

    /* Add entry to profile table.  */
    entries[0] = &opcode_entry;

    soc_mem_lock(unit, OAM_OPCODE_CONTROL_PROFILEm);

    rv = soc_profile_mem_add(unit, &oc->oam_opcode_control_profile,
                             (void *) &entries, 1, &profile_index);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Opcode profile add failed for EP=%d"
                "  %s.\n", unit, ep_p->id, bcm_errmsg(rv)));
        return (rv);
    }

    /* Release opcode control profile table lock. */
    soc_mem_unlock(unit, OAM_OPCODE_CONTROL_PROFILEm);

    /* Store endpoint OAM opcode profile table index value. */
    h_data_p->profile_index = profile_index;

    /*
     * MA_INDEX table programming.
     */

    /* Clear the entry data. */
    sal_memset(&ma_idx_entry, 0, sizeof(ma_index_entry_t));

    /* Set group index value. */
    soc_MA_INDEXm_field32_set(unit, &ma_idx_entry, MA_PTRf,
                              ep_p->group);

    /* Set OAM opcode control profile table index. */
    soc_MA_INDEXm_field32_set(unit, &ma_idx_entry,
                              OAM_OPCODE_CONTROL_PROFILE_PTRf,
                              h_data_p->profile_index);

    /* Set CPU CoS queue value. */
    if (ep_p->opcode_flags & BCM_OAM_OPCODE_CCM_COPY_TO_CPU) {
        soc_MA_INDEXm_field32_set(unit, &ma_idx_entry,
                                  INT_PRIf,
                                  ep_p->cpu_qid);
    }

    rv = WRITE_MA_INDEXm(unit, MEM_BLOCK_ALL, h_data_p->local_rx_index,
                         &ma_idx_entry);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: MA_INDEX table write failed for EP=%d"
                "  %s.\n", unit, ep_p->id, bcm_errmsg(rv)));
        return (rv);
    }

    /* L3 entry */
    _bcm_esw_l3_lock(unit);

    if (SOC_IS_HURRICANE2(unit)) {
        mem = L3_ENTRY_IPV4_UNICASTm;
    } else {
        mem = L3_ENTRY_1m;
    }

    /* Initialize L3 entry buffer. */
    sal_memset(&l3_entry, 0, sizeof(l3_entry));

    if (BCM_SUCCESS
            (_bcm_tr3_oam_find_lmep(unit, h_data_p, &l3_index, l3_entry))) {

        /* There's already an entry for this vlan + glp or 0 + vp */
        mdl = soc_mem_field32_get(unit, mem,  &l3_entry, LMEP__MDL_BITMAPf);

        mdl |= (1 << ep_p->level);

        rv = soc_mem_field32_modify(unit, mem, l3_index,
                                    LMEP__MDL_BITMAPf, mdl);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: L3_ENTRY table update failed for EP=%d"
                    "  %s.\n", unit, ep_p->id, bcm_errmsg(rv)));
            _bcm_esw_l3_unlock(unit);
            return (rv);
        }
    } else {

        /* This is the first entry at this vlan+glp */
        sal_memset(&l3_entry, 0, sizeof(l3_entry));

        soc_mem_field32_set(unit, mem, &l3_entry, LMEP__MDL_BITMAPf,
                                    (1 << ep_p->level));

        soc_mem_field32_set
            (unit, mem, &l3_entry, LMEP__MA_BASE_PTRf,
                (h_data_p->local_rx_index >> _BCM_OAM_EP_LEVEL_BIT_COUNT));

        if (ep_p->flags & BCM_OAM_ENDPOINT_MATCH_INNER_VLAN) {
            soc_mem_field32_set
                (unit, mem, &l3_entry, LMEP__EXP_TAG_STATUSf, 0x1);

            soc_mem_field32_set
                (unit, mem, &l3_entry, LMEP__EXP_TAG_STATUS_MASKf, 0x1);
        } else {
            soc_mem_field32_set
                (unit, mem, &l3_entry, LMEP__EXP_TAG_STATUSf, 0x2);

            soc_mem_field32_set
                (unit, mem, &l3_entry, LMEP__EXP_TAG_STATUS_MASKf, 0x3);
        }

        /* Construct LMEP view key for L3 Table insert operation. */
#ifdef BCM_HURRICANE2_SUPPORT
        if (SOC_IS_HURRICANE2(unit)) {
            _bcm_hu2_oam_lmep_key_construct(unit, h_data_p, 
                                   (l3_entry_ipv4_unicast_entry_t *)l3_entry);
        } else 
#endif /* BCM_HURRICANE2_SUPPORT */
        {
#ifdef BCM_TRIUMPH3_SUPPORT
            _bcm_oam_lmep_key_construct(unit, h_data_p, 
                                        (l3_entry_1_entry_t *)l3_entry);
#endif
        }
        soc_mem_field32_set(unit, mem, &l3_entry, VALIDf, 1);

        rv = soc_mem_insert(unit, mem, MEM_BLOCK_ALL, &l3_entry);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: L3_ENTRY table insert failed for EP=%d"
                    "  %s.\n", unit, ep_p->id, bcm_errmsg(rv)));
            _bcm_esw_l3_unlock(unit);
            return (rv);
        }
        _bcm_esw_l3_unlock(unit);
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_oam_rx_endpoint_reserve
 * Purpose:
 *     Reserve an hardware index in the group state table to maintain the state
 *     for a CCM Rx enabled endpoint.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     ep_info_p - (IN) Pointer to remote endpoint information.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_oam_rx_endpoint_reserve(int unit, 
                             const bcm_oam_endpoint_info_t *ep_info_p)
{
    _bcm_oam_hash_data_t   *h_data_p;     /* Endpoint hash data pointer.      */
    int                    rv;            /* Operation return status.         */
    uint32                 l3_entry[SOC_MAX_MEM_WORDS];   /* L3 entry buffer. */
    int                    l3_index = -1; /* L3 entry hardware index.         */
    int                    count = 0;     /* Successful Hw indices allocated. */
    uint8                  mdl = 0;       /* Maintenance domain level.        */
    int                    rx_index[1 << _BCM_OAM_EP_LEVEL_BIT_COUNT] = {0};
                                          /* Endpoint Rx hardware index.      */
    uint16                 ma_base_idx;   /* Base pointer to endpoint state   */
                                          /* table [MA_INDEX = (BASE + MDL)]. */
    _bcm_oam_control_t     *oc;           /* Pointer to control structure.    */
    const bcm_oam_endpoint_info_t *ep_p;  /* Pointer to endpoint information. */
    soc_mem_t              mem;           /* L3 entry memory                  */

    if (NULL == ep_info_p) {
        return (BCM_E_INTERNAL);
    }

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Initialize local endpoint information pointer. */
    ep_p = ep_info_p;

    /* Get endpoint hash data pointer. */
    h_data_p = &oc->oam_hash_data[ep_p->id];
    if (NULL == h_data_p) {
        return (BCM_E_INTERNAL);
    }

    
    /* Initialize L3 entry buffer. */
    sal_memset(&l3_entry, 0, sizeof(l3_entry));

    /* Find out if a matching entry already installed in hardware table. */
    rv = _bcm_tr3_oam_find_lmep(unit, h_data_p, &l3_index, l3_entry);
    if (BCM_FAILURE(rv)) {
        /*
         * If NO match found, allocate a new hardware index from MA_INDEX
         * pool.
         */

        /*
         * Endpoint MDL values can be (0-7) i.e 8 MDLs are supported per-MA
         * group endpoints. While allocating the base index, next 7 hardware
         * indices are also reserved.
         */
        rv = shr_idxres_list_alloc_set(oc->ma_idx_pool, 8,
                                       (shr_idxres_element_t *) rx_index,
                                       (shr_idxres_element_t *) &count);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: MA_INDEX index alloc failed EP:%d"
                    " %s.\n", unit, ep_p->id, bcm_errmsg(rv)));
            return (rv);
        } else {
            OAM_VVERB(("OAM(unit %d) Info: MA_INDEX alloc for EP:%d success."
                     " rx_idx_base:%p alloc-count:%d.\n", unit, ep_p->id,
                     rx_index, count));
        }

        /* Store hardware Rx index value for this endpoint. */
        h_data_p->local_rx_index = (rx_index[0] + ep_p->level);
    } else if (BCM_SUCCESS(rv)) {

        if (SOC_IS_HURRICANE2(unit)) {
            mem = L3_ENTRY_IPV4_UNICASTm;
        } else {
            mem = L3_ENTRY_1m;
        }
        /* Matching entry found, get installed entry MDL value. */
        mdl = soc_mem_field32_get(unit, mem, &l3_entry, LMEP__MDL_BITMAPf);

        /* Findout if MDLs are same. */
        if (0 == (mdl & (1 << ep_p->level))) {
            ma_base_idx = soc_mem_field32_get(unit, mem, &l3_entry,
                                                      LMEP__MA_BASE_PTRf);
            /*
             * MDLs are not same, so calculate Rx index for this new endpoint.
             * Endpoint MDL values can be (0-7). 8 MDLs are supported per-MA
             * group. Base Rx index written to hardware, so multiply by 8
             * i.e (<< _BCM_OAM_EP_LEVEL_BIT_COUNT) and add new endpoint MDL.
             */
            h_data_p->local_rx_index
                = ((ma_base_idx << _BCM_OAM_EP_LEVEL_BIT_COUNT)
                    | ep_p->level);
        } else {
            /* Rx index already taken return error. */
            OAM_ERR(("OAM(unit %d) Error: No free Rx index found for EP:%d"
                    " %s.\n", unit, ep_p->id, bcm_errmsg(rv)));
            return (BCM_E_RESOURCE);
        }
    }

    return (rv);
}

/*
 * Function:
 *     _bcm_tr3_oam_remote_endpoint_delete
 * Purpose:
 *     Delete a remote endpoint.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     h_data_p  - (IN) Pointer to endpoint hash data
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_remote_endpoint_delete(int unit,
                                    _bcm_oam_hash_data_t *h_data_p)
{
    _bcm_oam_control_t *oc;            /* Pointer to OAM control structure.  */
    uint32             l3_entry[SOC_MAX_MEM_WORDS];/* RMEP view table entry. */
    ma_state_entry_t   ma_state_entry; /* Group state machine table entry.   */
    rmep_entry_t       rmep_entry;     /* Remote endpoint table entry.       */
    int                rv;             /* Operation return status.           */
    uint32 some_rmep_ccm_defect_counter = 0; /* No. of RMEPs in MA with CCM  */
                                         /* defects.                         */
    uint32 some_rdi_defect_counter = 0;  /* No. of RMEPs in MA with RDI      */
                                         /* defects.                         */
    uint32 cur_some_rdi_defect = 0;      /* Any RMEP in MA with RDI defect.  */
    uint32 cur_some_rmep_ccm_defect = 0; /* Any RMEP in MA with CCM defect.  */
    uint32 cur_rmep_ccm_defect = 0;      /* RMEP lookup failed or CCM        */
                                         /* interval mismatch.               */
    uint32 cur_rmep_last_rdi = 0;        /* Last CCM RDI Rx from this RMEP.  */

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    sal_memset(&rmep_entry, 0, sizeof(rmep_entry_t));

    /* Get remote endpoint entry value from hardware. */
    rv = READ_RMEPm(unit, MEM_BLOCK_ANY, h_data_p->remote_index, &rmep_entry);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: RMEP table read failed for EP=%d"
                "%s.\n", unit, h_data_p->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    cur_rmep_ccm_defect
        = soc_RMEPm_field32_get(unit, &rmep_entry,
                                CURRENT_RMEP_CCM_DEFECTf);

    cur_rmep_last_rdi
        = soc_RMEPm_field32_get(unit, &rmep_entry,
                                CURRENT_RMEP_LAST_RDIf);

    sal_memset(&ma_state_entry, 0, sizeof(ma_state_entry));

    if ((0 != cur_rmep_ccm_defect) || (0 != cur_rmep_last_rdi)) {
        rv = READ_MA_STATEm(unit, MEM_BLOCK_ANY, h_data_p->group_index,
                            &ma_state_entry);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Group state (GID=%d) table read"
                    " failed - %s.\n", unit, h_data_p->ep_id, bcm_errmsg(rv)));
            return (rv);
        }

        some_rmep_ccm_defect_counter
            = soc_MA_STATEm_field32_get(unit, &ma_state_entry,
                                        SOME_RMEP_CCM_DEFECT_COUNTERf);
        cur_some_rmep_ccm_defect
            = soc_MA_STATEm_field32_get(unit, &ma_state_entry,
                                        CURRENT_SOME_RMEP_CCM_DEFECTf);

        if ((0 != cur_rmep_ccm_defect)
            && (some_rmep_ccm_defect_counter > 0)) {
            --some_rmep_ccm_defect_counter;
            soc_MA_STATEm_field32_set
                (unit, &ma_state_entry, SOME_RMEP_CCM_DEFECT_COUNTERf,
                 some_rmep_ccm_defect_counter);

            if (0 == some_rdi_defect_counter) {
                cur_some_rdi_defect = 0;
                soc_MA_STATEm_field32_set
                    (unit, &ma_state_entry, CURRENT_SOME_RMEP_CCM_DEFECTf,
                     cur_some_rmep_ccm_defect);
            }
        }

        some_rdi_defect_counter
            = soc_MA_STATEm_field32_get(unit, &ma_state_entry,
                                        SOME_RDI_DEFECT_COUNTERf);
        cur_some_rdi_defect
            = soc_MA_STATEm_field32_get(unit, &ma_state_entry,
                                        CURRENT_SOME_RDI_DEFECTf);

        if ((0 != cur_rmep_last_rdi)
            && (some_rdi_defect_counter > 0)) {
            --some_rdi_defect_counter;
            soc_MA_STATEm_field32_set
                (unit, &ma_state_entry, SOME_RDI_DEFECT_COUNTERf,
                 some_rdi_defect_counter);

            if (0 == some_rdi_defect_counter) {
                cur_some_rdi_defect = 0;
                soc_MA_STATEm_field32_set
                    (unit, &ma_state_entry, CURRENT_SOME_RDI_DEFECTf,
                     cur_some_rdi_defect);
            }
        }

        rv = WRITE_MA_STATEm(unit, MEM_BLOCK_ALL, h_data_p->group_index,
                             &ma_state_entry);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Group state (GID=%d) table write"
                    " failed - %s.\n", unit, h_data_p->group_index,
                    bcm_errmsg(rv)));
            return (rv);
        }
    }

    /* Clear RMEP table entry for this endpoint index. */
    sal_memset(&rmep_entry, 0, sizeof(rmep_entry_t));
    rv = WRITE_RMEPm(unit, MEM_BLOCK_ALL, h_data_p->remote_index,
                     &rmep_entry);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: RMEP table write index=%x (EP=%d)"
                " - %s.\n", unit, h_data_p->remote_index,
                h_data_p->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    sal_memset(&l3_entry, 0, sizeof(l3_entry));

    /* Construct endpoint RMEP view key for L3 Table entry delete operation. */
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        _bcm_hu2_oam_rmep_key_construct(unit, h_data_p, 
                                    (l3_entry_ipv4_unicast_entry_t *)l3_entry);
        rv = soc_mem_delete(unit, L3_ENTRY_IPV4_UNICASTm, 
                            MEM_BLOCK_ALL, &l3_entry);
    } else 
#endif /* BCM_HURRICANE2_SUPPORT */
    {
#ifdef BCM_TRIUMPH3_SUPPORT
        _bcm_oam_rmep_key_construct(unit, h_data_p, 
                                    (l3_entry_1_entry_t *)l3_entry);
        rv = soc_mem_delete(unit, L3_ENTRY_1m, MEM_BLOCK_ALL, &l3_entry);
#endif
    }
    if (BCM_FAILURE(rv) && (oc->init)) {
        OAM_ERR(("OAM(unit %d) Error: RMEP view update (EP=%d)"
                " - %s.\n", unit, h_data_p->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    /* Return ID back to free RMEP ID pool. */
    BCM_IF_ERROR_RETURN
        (shr_idxres_list_free(oc->rmep_pool, h_data_p->remote_index));

    /* Clear the H/w index to logical index Mapping for RMEP */
    oc->remote_endpoints[h_data_p->remote_index] = BCM_OAM_ENDPOINT_INVALID; 

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_tr3_oam_local_endpoint_delete
 * Purpose:
 *     Delete a local endpoint.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     h_data_p  - (IN) Pointer to endpoint hash data
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_local_endpoint_delete(int unit, _bcm_oam_hash_data_t *h_data_p)
{
    _bcm_oam_control_t *oc;            /* Pointer to OAM control structure. */
    uint32             l3_entry[SOC_MAX_MEM_WORDS];/* LMEP view table entry.*/ 
    lmep_entry_t       lmep_entry;     /* Local endpoint table entry.       */
    int                l3_index = -1;  /* L3 table hardware index.          */
    uint8              mdl;            /* Maintenance domain level.         */
    int                rv;             /* Operation return status.          */
    uint32             ma_base_index;  /* Endpoint tbl base index. */
    uint32 rx_index[1 << _BCM_OAM_EP_LEVEL_BIT_COUNT] = {0};
                                       /* Endpoint Rx hardware index.       */
    uint32 count;                      /* No. of Rx indices freed           */
                                       /* successfully.                     */
    int                idx;            /* Iterator variable.                */
    soc_mem_t          mem;            /* L3 entry memory                  */

    if (NULL == h_data_p){
        return (BCM_E_INTERNAL);
    }
    
    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    if (1 == h_data_p->local_tx_enabled) {

        /* Clear endpoint Tx config in hardware. */
        sal_memset(&lmep_entry, 0, sizeof(lmep_entry_t));

        rv = WRITE_LMEPm(unit, MEM_BLOCK_ALL, h_data_p->local_tx_index,
                         &lmep_entry);

        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: LMEP table write (EP=%d)"
                    " failed - %s.\n", unit, h_data_p->ep_id, bcm_errmsg(rv)));
            return (rv);
        }

        /* Return ID back to free LMEP ID pool. */
        BCM_IF_ERROR_RETURN
            (shr_idxres_list_free(oc->lmep_pool, h_data_p->local_tx_index));
    }

    if (1 == h_data_p->local_rx_enabled) {

        /* If LM DM is enabled, delete FP entries created for the same */
        if ( h_data_p->flags & 
             (BCM_OAM_ENDPOINT_LOSS_MEASUREMENT | 
              BCM_OAM_ENDPOINT_DELAY_MEASUREMENT) ) {
            rv = _bcm_tr3_oam_loss_delay_measurement_delete(unit, oc, h_data_p);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: LM DM delete failed (EP=%d)"
                        "- %s.\n", unit, h_data_p->ep_id, bcm_errmsg(rv)));
                return (rv);
            }
        }
        
        /* Delete OAM opcode profile entry for this endpoint. */
        rv = soc_profile_mem_delete(unit, &oc->oam_opcode_control_profile,
                                    h_data_p->profile_index);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Profile table update error (idx=%d)"
                    "- %s.\n", unit, h_data_p->profile_index, bcm_errmsg(rv)));
            return (rv);
        }
 
        /* Initialize L3 entry buffer. */
        sal_memset(&l3_entry, 0, sizeof(l3_entry));

        rv = _bcm_tr3_oam_find_lmep(unit, h_data_p, &l3_index, l3_entry);
        if (BCM_SUCCESS(rv)) {
            if (SOC_IS_HURRICANE2(unit)) {
                mem = L3_ENTRY_IPV4_UNICASTm;
            } else {
                mem = L3_ENTRY_1m;
            }
            /* Endpoint found, get MDL bitmap value. */
            mdl = soc_mem_field32_get(unit, mem, &l3_entry,
                                              LMEP__MDL_BITMAPf);

            /* Clear the MDL bit for this endpoint. */
            mdl &= ~(1 << h_data_p->level);

            /* Take L3 module protection mutex to block any updates. */
            _bcm_esw_l3_lock(unit);
            if (0 != mdl) {
                /* Valid endpoints exist for other MDLs at this index. */
                rv = soc_mem_field32_modify(unit, mem, l3_index,
                                            LMEP__MDL_BITMAPf, mdl);
            } else {
                rv = soc_mem_delete_index(unit, mem, MEM_BLOCK_ALL,
                                          l3_index);
            }
            _bcm_esw_l3_unlock(unit);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: LMEP view update (EP=%d) -"
                        " %s.\n", unit, h_data_p->ep_id, bcm_errmsg(rv)));
                return (rv);
            }

            /* This is the last Rx endpoint in this OAM group. */
            if (0 == mdl) {
                ma_base_index = soc_mem_field32_get(unit, mem, &l3_entry,
                                                            LMEP__MA_BASE_PTRf);
                /* Return Rx indices to free pool. */
                for (idx = 0; idx < (1 << _BCM_OAM_EP_LEVEL_BIT_COUNT); idx++) {
                    rx_index[idx] = (ma_base_index << _BCM_OAM_EP_LEVEL_BIT_COUNT)
                                        + idx;
                }

                rv = shr_idxres_list_free_set(oc->ma_idx_pool, 8,
                                              (shr_idxres_element_t *) rx_index,
                                              (shr_idxres_element_t *) &count);
                if (BCM_FAILURE(rv) || (8 != count)) {
                    OAM_ERR(("OAM(unit %d) Error: Rx index list free (EP=%d)"
                            " (count=%d).\n", unit, h_data_p->ep_id, count));
                    return (rv);
                }
            }
        } else if (BCM_FAILURE(rv) && (oc->init)) {
            OAM_ERR(("OAM(unit %d) Error: LMEP table write (EP=%d) -"
                    " %s.\n", unit, h_data_p->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
    }

    return (BCM_E_NONE);
}


/*
 * Function:
 *     _bcm_tr3_oam_endpoint_destroy
 * Purpose:
 *     Delete an endpoint and free all its allocated resources.
 * Parameters:
 *     unit   - (IN) BCM device number
 *     ep_id  - (IN) Endpoint ID value.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_endpoint_destroy(int unit,
                              bcm_oam_endpoint_t ep_id)
{
    _bcm_oam_control_t      *oc;        /* Pointer to OAM control structure. */
    _bcm_oam_hash_data_t    *h_data_p;  /* Pointer to endpoint data.         */
    _bcm_oam_hash_key_t     hash_key;   /* Hash key buffer for lookup.       */
    bcm_oam_endpoint_info_t ep_info;    /* Endpoint information.             */
    _bcm_oam_hash_data_t    h_data;     /* Pointer to endpoint data.         */
    int                     rv;         /* Operation return status.          */
#if defined(INCLUDE_BHH)
    uint16 reply_len;
    bcm_oam_endpoint_t bhh_pool_ep_id;
#endif

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Check endpoint status. */
    rv = shr_idxres_list_elem_state(oc->mep_pool, ep_id);
    if ((BCM_E_EXISTS != rv)) {
        /* Endpoint not in use. */
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    /* Get the hash data pointer. */
    h_data_p = &oc->oam_hash_data[ep_id];

    if(bcmOAMEndpointTypeEthernet == h_data_p->type)
    {
    /* Remove endpoint for group's endpoint list. */
    rv = _bcm_oam_group_ep_list_remove(unit, h_data_p->group_index,
                                       h_data_p->ep_id);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Remove from group list (EP=%d) -"
                " %s.\n", unit, ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    if (h_data_p->flags & BCM_OAM_ENDPOINT_REMOTE) {
        BCM_IF_ERROR_RETURN
                (_bcm_tr3_oam_remote_endpoint_delete(unit, h_data_p));

    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_oam_local_endpoint_delete(unit, h_data_p));
    }

    /* Return ID back to free MEP ID pool.*/
    BCM_IF_ERROR_RETURN(shr_idxres_list_free(oc->mep_pool, ep_id));

    /* Initialize endpoint info structure. */
    bcm_oam_endpoint_info_t_init(&ep_info);

    /* Set up endpoint information for key construction. */
    ep_info.group = h_data_p->group_index;
    ep_info.name = h_data_p->name;
    ep_info.gport = h_data_p->gport;
    ep_info.level = h_data_p->level;
    ep_info.vlan = h_data_p->vlan;

    /* Construct hash key for lookup + delete operation. */
    _bcm_oam_ep_hash_key_construct(unit, oc, &ep_info, &hash_key);

    /* Remove entry from hash table. */
    BCM_IF_ERROR_RETURN(shr_htb_find(oc->ma_mep_htbl, hash_key,
                                     (shr_htb_data_t *)&h_data,
                                     1));

    /* Clear the hash data memory previously occupied by this endpoint. */
    _BCM_OAM_HASH_DATA_CLEAR(h_data_p);
    }
    /*
     * BHH specific
     */
    else if (soc_feature(unit, soc_feature_bhh) && 
        ((bcmOAMEndpointTypeBHHMPLS == h_data_p->type) ||
         (bcmOAMEndpointTypeBHHMPLSVccv == h_data_p->type))) {
#if defined(INCLUDE_BHH)

        if (h_data_p->is_remote) {
            /* 
            * BHH uses same index for local and remote.  So, delete always goes through
            * local endpoint destory
            */
            return (BCM_E_NONE);
        } else {

            bhh_pool_ep_id = BCM_OAM_BHH_GET_UKERNEL_EP(ep_id);

            /* 
             * Delete BHH Session in HW 
             */
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_oam_bhh_session_hw_delete(unit, h_data_p));

            /* 
             * Send BHH Session Delete message to uC 
             */
            BCM_IF_ERROR_RETURN
            (_bcm_tr3_oam_bhh_msg_send_receive(unit, 
                          MOS_MSG_SUBCLASS_BHH_SESS_DELETE,
                          (int)bhh_pool_ep_id, 0,
                          MOS_MSG_SUBCLASS_BHH_SESS_DELETE_REPLY,
                          &reply_len));

            if (reply_len != 0) {
                return (BCM_E_INTERNAL);
            }

            h_data_p->in_use = 0;

            /* Remove endpoint for group's endpoint list. */
            rv = _bcm_oam_group_ep_list_remove(unit, h_data_p->group_index,
                                       h_data_p->ep_id);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Remove from group list (EP=%d) -"
                " %s.\n", unit, ep_id, bcm_errmsg(rv)));
                return (rv);
            }

            /* Return ID back to free MEP ID pool.*/
            BCM_IF_ERROR_RETURN(shr_idxres_list_free(oc->bhh_pool, bhh_pool_ep_id));
        }
#else
        return (BCM_E_UNAVAIL);
#endif /* INCLUDE_BHH */
    }
    else {
        return (BCM_E_PARAM);
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_tr3_oam_group_endpoints_destroy
 * Purpose:
 *     Delete all endpoints associated with a group and free all
 *     resources allocated by these endpoints. 
 * Parameters:
 *     unit      - (IN) BCM device number
 *     g_info_p  - (IN) Pointer to group information
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_group_endpoints_destroy(int unit,
                                     _bcm_oam_group_data_t *g_info_p)
{
    bcm_oam_endpoint_t    ep_id; /* Endpoint ID.                   */
    _bcm_oam_ep_list_t    *cur;  /* Pointer to endpoint list node. */
    int                   rv;    /* Operation return status.       */

    if (NULL == g_info_p) {
        return (BCM_E_INTERNAL);
    }

    /* Get the endpoint list head pointer. */
    cur = *(g_info_p->ep_list);
    if (NULL == cur) {
        OAM_VVERB(("OAM(unit %d) Info: No endpoints in group.\n", unit));
        return (BCM_E_NONE);
    }

    while (NULL != cur) {
        ep_id = cur->ep_data_p->ep_id;

        OAM_VVERB(("OAM(unit %d) Info: GID=%d EP:%d.\n",
                 unit, cur->ep_data_p->group_index, ep_id));

        cur = cur->next;

        rv = _bcm_tr3_oam_endpoint_destroy(unit, ep_id);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Endpoint destroy (EP=%d) - "
                    "%s.\n", unit, ep_id, bcm_errmsg(rv)));
            return (rv);
        }
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_tr3_oam_endpoint_gport_resolve
 * Purpose:
 *     Resolve an endpoint GPORT value to SGLP and DGLP value.
 * Parameters:
 *     unit       - (IN) BCM device number
 *     ep_info_p  - (IN) Pointer to endpoint information.
 *     src_glp    - (IN/OUT) Pointer to source generic logical port value.
 *     dst_glp    - (IN/OUT) Pointer to destination generic logical port value.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_endpoint_gport_resolve(int unit,
                                bcm_oam_endpoint_info_t *ep_info_p,
                                uint32 *src_glp,
                                uint32 *dst_glp)
{
    bcm_module_t       module_id;            /* Module ID           */
    bcm_port_t         port_id;              /* Port ID.            */
    bcm_trunk_t        trunk_id;             /* Trunk ID.           */
    int                local_id;             /* Hardware ID.        */
    int                tx_enabled = 0;       /* CCM Tx enabled.     */
    bcm_trunk_info_t   trunk_info;           /* Trunk information.  */
    bcm_trunk_member_t *member_array = NULL; /* Trunk member array. */
    int                member_count = 0;     /* Trunk Member count. */
    int                rv;                   /* Return status.      */
    uint8              glp_valid = 0;        /* Logical port valid. */

    /* Get Trunk ID or (Modid + Port) value from Gport */
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, ep_info_p->gport, &module_id,
                                &port_id, &trunk_id, &local_id));

    /* Set CCM endpoint Tx status only for local endpoints. */
    if (!(ep_info_p->flags & BCM_OAM_ENDPOINT_REMOTE)) {
        tx_enabled
            = (ep_info_p->ccm_period
                != BCM_OAM_ENDPOINT_CCM_PERIOD_DISABLED) ? 1 : 0;
    }

    /*
     * If Gport is Trunk type, _bcm_esw_gport_resolve()
     * sets trunk_id. Using Trunk ID, get Dst Modid and Port value.
     */
    if (BCM_GPORT_IS_TRUNK(ep_info_p->gport)) {

        if (BCM_TRUNK_INVALID == trunk_id)  {
            /* Has to be a valid Trunk. */
            return (BCM_E_PARAM);
        }

        /*
         * CCM Tx is enabled on a trunk member port.
         * trunk_index value is required to derive the Modid and Port info.
         */
        if (1 == tx_enabled
            && _BCM_OAM_INVALID_INDEX == ep_info_p->trunk_index) {
            /* Invalid Trunk member index passed. */
            return (BCM_E_PORT);
        }

        /* Construct Hw SGLP value. */
        _BCM_TR3_OAM_MOD_PORT_TO_GLP(unit, module_id, port_id, 1, trunk_id,
            *src_glp);

        /* Get count of ports in this trunk. */
        BCM_IF_ERROR_RETURN
            (bcm_esw_trunk_get(unit, trunk_id, NULL, 0, NULL, &member_count));
        if (0 == member_count) {
            /* No members have been added to the trunk group yet */
            return (BCM_E_PARAM);
        }

        _BCM_OAM_ALLOC(member_array, bcm_trunk_member_t,
                       sizeof(bcm_trunk_member_t) * member_count, "Trunk info");
        if (NULL == member_array) {
            return (BCM_E_MEMORY);
        }

        /* Get Trunk Info for the Trunk ID. */
        rv = bcm_esw_trunk_get(unit, trunk_id, &trunk_info, member_count,
                               member_array, &member_count);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return (rv);
        }

        /* Check if the input trunk_index is valid. */
        if (ep_info_p->trunk_index >= member_count) {
            sal_free(member_array);
            return (BCM_E_PARAM);
        }

        /* Get the Modid and Port value using Trunk Index value. */
        rv = _bcm_esw_gport_resolve
                (unit, member_array[ep_info_p->trunk_index].gport,
                 &module_id, &port_id, &trunk_id, &local_id);
        if (BCM_FAILURE(rv)) {
            sal_free(member_array);
            return (rv);
        }

        sal_free(member_array);

        /* Construct Hw DGLP value. */
        _BCM_TR3_OAM_MOD_PORT_TO_DGLP(unit, module_id, port_id, 0, -1,
            *dst_glp);
        glp_valid = 1;
    }

    /*
     * Application can resolve the trunk and pass the desginated
     * port as Gport value. Check if the Gport belongs to a trunk.
     */
    if ((BCM_TRUNK_INVALID == trunk_id)
        && (BCM_GPORT_IS_MODPORT(ep_info_p->gport)
        || BCM_GPORT_IS_LOCAL(ep_info_p->gport))) {

        /* When Gport is ModPort or Port type, _bcm_esw_gport_resolve()
         * returns Modid and Port value. Use these values to make the DGLP
         * value.
         */
        _BCM_TR3_OAM_MOD_PORT_TO_DGLP(unit, module_id, port_id, 0, -1,
            *dst_glp);

        /* Use the Modid, Port value and determine if the port
         * belongs to a Trunk.
         */
        rv = bcm_esw_trunk_find(unit, module_id, port_id, &trunk_id);
        if (BCM_SUCCESS(rv)) {
            /*
             * Port is member of a valid trunk.
             * Now create the SGLP value from Trunk ID.
             */
            _BCM_TR3_OAM_MOD_PORT_TO_GLP(unit, module_id, port_id, 1, trunk_id,
                *src_glp);

        } else {
            /* Port not a member of trunk. Construct SGLP */
            _BCM_TR3_OAM_MOD_PORT_TO_GLP(unit, module_id, port_id, 0, -1,
                                          *src_glp);
        }
        glp_valid = 1;
    }

    /*
     * At this point, both src_glp and dst_glp should be valid.
     * Gport types other than TRUNK, MODPORT or LOCAL are not valid.
     */
    if (0 == glp_valid) {
        return (BCM_E_PORT);
    }

    return (BCM_E_NONE);
}


/*
 * Function:
 *     _bcm_tr3_oam_endpoint_params_validate
 * Purpose:
 *     Validate an endpoint parameters.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     oc        - (IN) Pointer to OAM control structure.
 *     hash_key  - (IN) Pointer to endpoint hash key value.
 *     ep_info_p - (IN) Pointer to endpoint information.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_endpoint_params_validate(int unit,
                                  _bcm_oam_control_t *oc,
                                  _bcm_oam_hash_key_t *hash_key,
                                  bcm_oam_endpoint_info_t *ep_info_p)
{
    int                 rv;  /* Operation return status. */
    _bcm_oam_hash_data_t h_stored_data;

    OAM_VVERB(("OAM(unit %d) Info: _bcm_tr3_oam_endpoint_params_validate.\n", unit));

    /* Endpoint must be 802.1ag/Ethernet OAM type. */
    if ((bcmOAMEndpointTypeEthernet != ep_info_p->type)
#if defined(INCLUDE_BHH) 
        &&  /* BHH/Y.1731 OAM type */
        (bcmOAMEndpointTypeBHHMPLS != ep_info_p->type) &&
        (bcmOAMEndpointTypeBHHMPLSVccv != ep_info_p->type)
#endif /* INCLUDE_BHH */
        )
    {
        /* Other OAM types are not supported, return error. */
        return (BCM_E_UNAVAIL);
    }

    /*
     * Check and return error if invalid flag bits are set for remote
     * endpoint.
     */
    if ((ep_info_p->flags & BCM_OAM_ENDPOINT_REMOTE)
        && (ep_info_p->flags & _BCM_OAM_REMOTE_EP_INVALID_FLAGS_MASK)) {

        return (BCM_E_PARAM);

    }

    /* For replace operation, endpoint ID is required. */
    if ((ep_info_p->flags & BCM_OAM_ENDPOINT_REPLACE)
        && !(ep_info_p->flags & BCM_OAM_ENDPOINT_WITH_ID)) {

        return (BCM_E_PARAM);

    }

    /* Validate endpoint index value. */
    if (ep_info_p->flags & BCM_OAM_ENDPOINT_WITH_ID) {

        _BCM_OAM_EP_INDEX_VALIDATE(ep_info_p->id);
    }

    /* Validate endpoint group id. */
    _BCM_OAM_GROUP_INDEX_VALIDATE(ep_info_p->group);

    rv = shr_idxres_list_elem_state(oc->group_pool, ep_info_p->group);
    if (BCM_E_EXISTS != rv) {
        OAM_ERR(("OAM(unit %d) Error: Group (GID:%d) does not exist.\n",
                unit, ep_info_p->group));
        return (BCM_E_PARAM);
    }

    if(soc_feature(unit, soc_feature_bhh)) {
#if defined(INCLUDE_BHH)
        /*
         * BHH can have multiple LSPs/endpoint on same port/MDL 
         * BHH does not have h/w support.  So, return here 
         */
        if ((bcmOAMEndpointTypeBHHMPLS == ep_info_p->type) ||
            (bcmOAMEndpointTypeBHHMPLSVccv == ep_info_p->type)) {
            return (BCM_E_NONE);
        }
#endif
    }

#if defined(KEY_PRINT)
    _bcm_oam_hash_key_print(hash_key);
#endif

    /*
     * Lookup using hash key value.
     * Last param value '0' specifies keep the match entry.
     * Value '1' would mean remove the entry from the table.
     * Matched Params:
     *      Group Name + Group ID + Endpoint Name + VLAN + MDL + Gport.
     */
    rv = shr_htb_find(oc->ma_mep_htbl, *hash_key,
                      (shr_htb_data_t *)&h_stored_data, 0);
    if (BCM_SUCCESS(rv)
        && !(ep_info_p->flags & BCM_OAM_ENDPOINT_REPLACE)) {

        OAM_ERR(("OAM(unit %d) Error: Endpoint ID=%d %s.\n",
                unit, ep_info_p->id, bcm_errmsg(BCM_E_EXISTS)));

        /* Endpoint must not be in use expect for Replace operation. */
        return (BCM_E_EXISTS);

    } else {

        OAM_VVERB(("OAM(unit %d) Info: Endpoint ID=%d Available. %s.\n",
                 unit, ep_info_p->id, bcm_errmsg(rv)));

    }

    

    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_tr3_oam_fp_entry_id_allocate
 * Purpose:
 *     Allocate FP entry Id for Loss and Delay measurement
 * Parameters:
 *     unit         - (IN) BCM device number.
 *     fp_group     - (IN) Group Id under which the FP entry is created.
 *     prio         - (IN) Prioroty of FP Entry to be created.
 *     fp_entry_id  - (OUT) Created FP Entry Id
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_fp_entry_id_allocate(int unit, bcm_field_group_t fp_group, 
                                  int prio, bcm_field_entry_t *fp_entry_id) {

    int rv = 0;
    
    if (NULL == fp_entry_id){
        return (BCM_E_INTERNAL);
    }
    
    rv = bcm_esw_field_entry_create(unit, fp_group, fp_entry_id);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: FP Entry Tx creation (tx), %s.\n",
             unit, bcm_errmsg(rv)));
        return (rv);
    }

    rv = bcm_esw_field_entry_prio_set(unit, *fp_entry_id, prio);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: FP Entry Tx Prio Set (tx), %s.\n",
             unit, bcm_errmsg(rv)));
        return (rv);
    }

    return (rv);

}

/*
 * Function:
 *     _bcm_tr3_oam_fp_entry_action_add
 * Purpose:
 *     Adds actions to the fp entry
 * Parameters:
 *     unit         - (IN) BCM device number
 *     hash_data    - (IN) Pointer to endpoint hash data.
 *     fp_entry_id  - (IN) FP Entry id
 *     isTx         - (IN) Whether entry matches tx or rx flow
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_fp_entry_action_add(int unit, _bcm_oam_hash_data_t *hash_data, 
                                 bcm_field_entry_t fp_entry_id, uint8 isTx) {

    int rv = 0;
    
    if (NULL == hash_data){
        return (BCM_E_INTERNAL);
    }
    
    rv = bcm_esw_field_action_add(unit, fp_entry_id, 
                                  bcmFieldActionOamLmepEnable, 1, 0);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Adding action OamLmepEnable, EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    rv = bcm_esw_field_action_add(unit, fp_entry_id, bcmFieldActionOamLmEnable,
                hash_data->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT? 1:0, 0);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Adding action OamLmEnable, EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    rv = bcm_esw_field_action_add(unit, fp_entry_id, bcmFieldActionOamDmEnable, 
                hash_data->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT? 1:0, 0);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Adding action OamDmEnable, EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    
    if (hash_data->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT){
        rv = bcm_esw_field_action_add(unit, fp_entry_id, 
                bcmFieldActionOamDmTimeFormat, 
                (hash_data->ts_format == bcmOAMTimestampFormatNTP ? 
                BCM_FIELD_OAM_DM_TIME_FORMAT_NTP : 
                BCM_FIELD_OAM_DM_TIME_FORMAT_IEEE1588), 0);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Adding action DmTimeFormat, \
            EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
    }

    rv = bcm_esw_field_action_add(unit, fp_entry_id, bcmFieldActionOamLmepMdl, 
                                hash_data->level, 0);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Adding action LmepMdl (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    rv = bcm_esw_field_action_add(unit, fp_entry_id, bcmFieldActionOamUpMep, 
                        hash_data->flags & BCM_OAM_ENDPOINT_UP_FACING? 1:0, 0);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Adding action OamUpMep (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    /* In the case of UP MEP Tx rule is actually Rx and vice versa */
    rv = bcm_esw_field_action_add(unit, fp_entry_id, 
                                  bcmFieldActionOamTx, isTx, 0);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Adding action OamTx (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    if (hash_data->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT)
    {
        /* Assign LM counter and priority mapping table */
        rv = bcm_esw_field_action_add(unit, fp_entry_id, 
                    bcmFieldActionOamLmBasePtr, hash_data->lm_counter_index, 0);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Adding action OamUpMep (tx), \
            EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }

        rv = bcm_esw_field_action_add(unit, fp_entry_id, 
            bcmFieldActionOamServicePriMappingPtr, hash_data->pri_map_index, 0);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Adding action OamUpMep (tx), \
            EP=%d %s.\n", unit, hash_data->pri_map_index, bcm_errmsg(rv)));
            return (rv);
        }

    }

    return (rv);


}

/*
 * Function:
 *     _bcm_tr3_oam_fp_trunk_create
 * Purpose:
 *     Create FP Entry to match trunk memeber port from which
 *     LM/DM packets generated by CPU will be transmitted.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     oc        - (IN) Pointer to OAM control structure.
 *     hash_data - (IN) Pointer to endpoint hash data.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_fp_trunk_create(int unit, _bcm_oam_control_t *oc, 
                             _bcm_oam_hash_data_t *hash_data) {

    int prio = 0;
    int rv= 0;
    bcm_field_entry_t fp_entry = -1;
    bcm_module_t modid = 0;
    bcm_port_t local_port;
    bcm_ip6_t mdl_data, mdl_mask;

    if (NULL == oc || NULL == hash_data ){
        return (BCM_E_INTERNAL);
    }
    
    /* Group should already be created by now*/
    if( oc->fp_glp_group == -1 ){
         OAM_ERR(("OAM(unit %d) Error: FP Croup not present, EP=%d %s.\n",
                unit, hash_data->pri_map_index, bcm_errmsg(rv)));
         return (BCM_E_INTERNAL);
    }

    /* Create fp entry */
    /*******************/
    /*Adding 1 to avoid hitting default Prio 0 at MAX_MDL*/
    prio = (_BCM_OAM_MAX_MDL + 1) - hash_data->level; 
    
    rv = _bcm_tr3_oam_fp_entry_id_allocate(unit, oc->fp_glp_group, prio, 
                                           &(hash_data->fp_entry_trunk[0]));
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: FP Trunk Entry allocate (tx), EP=%d %s\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    fp_entry = hash_data->fp_entry_trunk[0];
    
    /* Add Qulaifiers */
    /******************/
    rv = bcm_esw_field_qualify_OuterVlanId(unit, fp_entry, hash_data->vlan, 
                                           BCM_FIELD_EXACT_MATCH_MASK);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Qualifying OuterVlanId (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    
    /* hash_data->dglp already contains resolved trunk index port, use it to get
     * port and module for qualifying LM/DM packet transmitted from CPU */
    OAM_VERB(("OAM(unit %d): %s - dglp %d.\n", 
    unit, __FUNCTION__, hash_data->dglp));
    local_port = _BCM_OAM_GLP_PORT_GET(hash_data->dglp);
    modid = _BCM_OAM_DGLP_MODULE_ID_GET(hash_data->dglp);
    OAM_VERB(("OAM(unit %d): %s - Local Port %d, modid %d.\n", 
    unit, __FUNCTION__, local_port, modid));
             
    rv = bcm_esw_field_qualify_DstPort(unit, fp_entry, modid, 
            BCM_FIELD_EXACT_MATCH_MASK, local_port, BCM_FIELD_EXACT_MATCH_MASK);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Qualifying DstPort (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    
    rv  = bcm_esw_field_qualify_EtherType(unit, fp_entry, _BCM_OAM_ETHER_TYPE, 
                                          BCM_FIELD_EXACT_MATCH_MASK);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: EtherType (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    sal_memset(&mdl_data, 0, sizeof(bcm_ip6_t));
    sal_memset(&mdl_mask, 0, sizeof(bcm_ip6_t));
    mdl_data[0] = (hash_data->level << 5);
    mdl_mask[0] = 0xE0;
    
    rv  = bcm_esw_field_qualify_DstIp6(unit, fp_entry, mdl_data, mdl_mask);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Qualifying MDL (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    
    /* Add Action */
    /**************/
    rv = _bcm_tr3_oam_fp_entry_action_add(unit, hash_data, fp_entry, 1);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Adding action (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    return (rv);
    
}

/*
 * Function:
 *     _bcm_tr3_oam_fp_create
 * Purpose:
 *     Create IFP entry for LOSS & DELAY Measurement
 * Parameters:
 *     unit      - (IN) BCM device number
 *     oc        - (IN-OUT) Pointer to OAM control structure.
 *     hash_data - (IN-OUT) Pointer to endpoint hash data.
 *     modify_entry - (OUT) Pointer to existing entries which were 
 *                     modified and need re-installation.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_fp_create(int unit, _bcm_oam_control_t *oc, 
                       _bcm_oam_hash_data_t *hash_data) {

    int rv = 0, i;
    _bcm_oam_hash_data_t *existing_max_level_ep_data = NULL;
    bcm_field_entry_t fp_entry_tx = -1, fp_entry_rx = -1;
    /* To hold any existing entries if modified */
    bcm_field_entry_t  modify_entry[2] = {-1, -1};
    uint32 gport = 0;
    bcm_trunk_t tgid = 0;
    bcm_module_t modid = 0;
    bcm_port_t local_port;
    int prio = 0;
    uint8 add_mdl = 0;
    bcm_ip6_t mdl_data, mdl_mask;
    int local_id; /* Hardware ID used in call to _bcm_esw_gport_resolve*/

    if ( (NULL == oc) || (NULL == hash_data) ){
        return (BCM_E_INTERNAL);
    }

    /* Create Group */
    /****************/
    if (oc->fp_glp_group == -1) {

        /* Prepare the Qualifier, use the same group for all 3: 
         * 1. physical port, 2. trunk and 3. virtual ports. 
         */
        BCM_FIELD_QSET_INIT(oc->fp_glp_qs);
        BCM_FIELD_QSET_ADD(oc->fp_glp_qs, bcmFieldQualifyStageIngress);
        
        BCM_FIELD_QSET_ADD(oc->fp_glp_qs, bcmFieldQualifyOuterVlanId);
        BCM_FIELD_QSET_ADD(oc->fp_glp_qs, bcmFieldQualifyDstPort);
        BCM_FIELD_QSET_ADD(oc->fp_glp_qs, bcmFieldQualifyDstTrunk);
        BCM_FIELD_QSET_ADD(oc->fp_glp_qs, bcmFieldQualifySrcPort);
        BCM_FIELD_QSET_ADD(oc->fp_glp_qs, bcmFieldQualifySrcTrunk);
        BCM_FIELD_QSET_ADD(oc->fp_glp_qs, bcmFieldQualifyEtherType);
        BCM_FIELD_QSET_ADD(oc->fp_glp_qs, bcmFieldQualifyDstIp6);
        BCM_FIELD_QSET_ADD(oc->fp_glp_qs, bcmFieldQualifyDstMimGport);
        BCM_FIELD_QSET_ADD(oc->fp_glp_qs, bcmFieldQualifySrcMimGport);

        /* Create fp group */
        rv = bcm_esw_field_group_create(unit, oc->fp_glp_qs, 
                                BCM_FIELD_GROUP_PRIO_ANY, &(oc->fp_glp_group));
         if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Group creation, EP=%d %s.\n",
                unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
         
    }

    /* Create the FP entry */
    /***********************/
    /* Entry priority is inversely proportional to level, In case of Multiple 
     * MEPs on same vlan port following is the scheme for installation.
     * -----Level 2-(Match port vlan ethertype and MDL)----
     * -----Level 4-(Match port vlan ethertype and MDL)----
     * -----Level 6-(Match port vlan)----------------------
     */
     
    /* Adding 1 to avoid hitting default Prio 0 at MAX_MDL */
    prio = (_BCM_OAM_MAX_MDL + 1) - hash_data->level;
    
    rv = _bcm_tr3_oam_fp_entry_id_allocate(unit, oc->fp_glp_group, prio, 
                                           &(hash_data->fp_entry_tx));
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: FP Entry allocate (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    
    rv = _bcm_tr3_oam_fp_entry_id_allocate(unit, oc->fp_glp_group, prio, 
                                           &(hash_data->fp_entry_rx));
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: FP Entry allocate (rx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }


    /* Add Qualifiers */
    /******************/    
    /* 1. VP Case */
    if (hash_data->vp != 0) {
        OAM_VERB(("OAM(unit %d): MEP on VP\n", unit));
        BCM_GPORT_MIM_PORT_ID_SET(gport, hash_data->vp);

        rv = bcm_esw_field_qualify_DstMimGport(unit, hash_data->fp_entry_tx, 
                                               gport);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Qualifying DstMimGport (tx), \
            EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }

    	rv = bcm_esw_field_qualify_SrcMimGport(unit, hash_data->fp_entry_rx, 
                                               gport);
    	if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Qualifying SrcMimGport (rx), \
            EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
	
    } else {
        
        /* If its not VP case then we need to qualify on outer vlan for rest */
         rv = bcm_esw_field_qualify_OuterVlanId(unit, hash_data->fp_entry_tx, 
                                hash_data->vlan, BCM_FIELD_EXACT_MATCH_MASK);
         if (BCM_FAILURE(rv)) {
             OAM_ERR(("OAM(unit %d) Error: Qualifying OuterVlanId (tx), \
             EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
             return (rv);
         }

        rv = bcm_esw_field_qualify_OuterVlanId(unit, hash_data->fp_entry_rx, 
                                hash_data->vlan, BCM_FIELD_EXACT_MATCH_MASK);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Qualifying OuterVlanId (rx), \
            EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
        
        rv = _bcm_esw_gport_resolve(unit, hash_data->gport, &modid, 
                                    &local_port, &tgid, &local_id);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: call to _bcm_esw_gport_resolve \
            failed, EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
        OAM_VERB(("OAM(unit %d): %s - Local Port %d, modid %d, trunkid %d.\n", 
                 unit, __FUNCTION__, local_port, modid, tgid));

        /* 2. Trunk Case */
        if (SOC_GPORT_IS_TRUNK(hash_data->gport)) {
            OAM_VERB(("OAM(unit %d): MEP on Trunk\n", unit));
            
            if (BCM_TRUNK_INVALID == tgid)  {
                OAM_ERR(("OAM(unit %d) Error: Invalid Trunkid, EP=%d %s.\n", 
                unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
            }

            rv  = bcm_esw_field_qualify_DstTrunk(unit, hash_data->fp_entry_tx, 
                                tgid, BCM_FIELD_EXACT_MATCH_MASK);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Qualifying DstTrunk (tx), \
                EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
            }
            
            rv  = bcm_esw_field_qualify_SrcTrunk(unit, hash_data->fp_entry_rx, 
                                    tgid, BCM_FIELD_EXACT_MATCH_MASK);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Qualifying SrcTrunk (rx), \
                EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
            }
            
            /*For Down MEP on LAG, Add FP Entry on primary port of the LAG also, 
             * as when packet is sent from CPU, Trunk Id cannot be specified,
             * and the packet will carry primary port of the LAG as destination
             */
            if (!(hash_data->flags & BCM_OAM_ENDPOINT_UP_FACING)){
                rv = _bcm_tr3_oam_fp_trunk_create(unit, oc, hash_data);
                if (BCM_FAILURE(rv)) {
                    OAM_ERR(("OAM(unit %d) Error: Creating Fp Entry on Trunk Tx\
                    port, EP=%d %s\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
                    return (rv);
                }
            }
            
        } else {
        
            /*3. Local Port*/
            OAM_VERB(("OAM(unit %d): MEP on Local Port\n", unit));

            rv = bcm_esw_field_qualify_DstPort(unit, hash_data->fp_entry_tx, 
                                        modid, BCM_FIELD_EXACT_MATCH_MASK, 
                                        local_port, BCM_FIELD_EXACT_MATCH_MASK);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Qualifying DstPort (tx), \
                EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
            }

            rv = bcm_esw_field_qualify_SrcPort(unit, hash_data->fp_entry_rx, 
                                        modid, BCM_FIELD_EXACT_MATCH_MASK, 
                                        local_port, BCM_FIELD_EXACT_MATCH_MASK);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Qualifying SrcPort (rx), \
                EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
            }
        }   
    }

    /* Check if we need Ethertype and MDL Qualifier:
     * 1. If this the first LM/DM MEP on port vlan, 
     *      Qualification on Ethertype and MDL not needed.
     * 2. If this is not the first LM/DM MEP on port vlan, and also not of 
     *      highest level, then add Ethertype and MDL Qualifiers and insert.
     * 3. If this is not the fisrt LM/DM MEP on port vlan, and also of 
     *      highest level, then modify existing highest MEP entry,
     *      Add Ethertype and MDL Qualifier to existing MEP entry and 
     *      insert the current one without Ethertype and MDL.
     */
    sal_memset(&mdl_data, 0, sizeof(bcm_ip6_t));
    sal_memset(&mdl_mask, 0, sizeof(bcm_ip6_t));
    mdl_mask[0] = 0xE0;
    OAM_VERB(("OAM(unit %d): %s _bcm_oam_lm_dm_search_data.match_count %d, \
        _bcm_oam_lm_dm_search_data.highest_level %d, hash_data->level %d\n", 
        unit, __FUNCTION__, _bcm_oam_lm_dm_search_data.match_count, 
        _bcm_oam_lm_dm_search_data.highest_level, hash_data->level));
    if ( (_bcm_oam_lm_dm_search_data.match_count) && 
         (_bcm_oam_lm_dm_search_data.highest_level > hash_data->level) ) {

        fp_entry_tx = hash_data->fp_entry_tx;
        fp_entry_rx = hash_data->fp_entry_rx;
        mdl_data[0] = hash_data->level << 5;
        add_mdl = 1;
        
    } else {

        if ( (_bcm_oam_lm_dm_search_data.match_count) && 
             (_bcm_oam_lm_dm_search_data.highest_level < hash_data->level) ) {

            existing_max_level_ep_data =  
             &oc->oam_hash_data[_bcm_oam_lm_dm_search_data.highest_level_ep_id];
            fp_entry_tx = existing_max_level_ep_data->fp_entry_tx;
            fp_entry_rx = existing_max_level_ep_data->fp_entry_rx;
            mdl_data[0] = existing_max_level_ep_data->level << 5;
            add_mdl = 1;
            modify_entry[0] = fp_entry_tx;
            modify_entry[1] = fp_entry_rx;
       
        }
    }
    if (add_mdl) {
        OAM_VERB(("OAM(unit %d): %s add_mdl=true\n", unit, __FUNCTION__));

        rv  = bcm_esw_field_qualify_EtherType(unit, fp_entry_tx, 
        _BCM_OAM_ETHER_TYPE, BCM_FIELD_EXACT_MATCH_MASK);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: EtherType (tx), EP=%d %s.\n",
                unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
        rv  = bcm_esw_field_qualify_EtherType(unit, fp_entry_rx, 
                            _BCM_OAM_ETHER_TYPE, BCM_FIELD_EXACT_MATCH_MASK);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: EtherType (rx), EP=%d %s.\n",
                unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }

        rv  = bcm_esw_field_qualify_DstIp6(unit, fp_entry_tx, 
                                            mdl_data, mdl_mask);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Qualifying MDL (tx), EP=%d %s.\n",
                unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
        rv  = bcm_esw_field_qualify_DstIp6(unit, fp_entry_rx, 
                                            mdl_data, mdl_mask);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Qualifying MDL (rx), EP=%d %s.\n",
                unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
    }


    /* Add action */
    /**************/
    /* In the case of UP MEP Tx rule is actually Rx and vice versa */
    rv = _bcm_tr3_oam_fp_entry_action_add(unit, 
                        hash_data, hash_data->fp_entry_tx, 
                        hash_data->flags & BCM_OAM_ENDPOINT_UP_FACING? 0:1);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Adding action (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    rv = _bcm_tr3_oam_fp_entry_action_add(unit, hash_data, 
    hash_data->fp_entry_rx, hash_data->flags & BCM_OAM_ENDPOINT_UP_FACING? 1:0);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Adding action (rx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    
    /* Install Created/Modified entries */
    /************************************/
    /* Tx Entry */
    rv = bcm_esw_field_entry_install(unit, hash_data->fp_entry_tx);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: FP Install failed (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    /* Rx Entry */
    rv = bcm_esw_field_entry_install(unit, hash_data->fp_entry_rx);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: FP Install failed (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    /* Trunk Entry */
    if (hash_data->fp_entry_trunk[0] != -1){
        rv = bcm_esw_field_entry_install(unit, hash_data->fp_entry_trunk[0]);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: FP Install failed (Trunk), \
            EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
    }
    /* Any existing entries that were modified */
    for (i=0; i<2; i++){
        if (modify_entry[i] != -1) {
            rv = bcm_esw_field_entry_install(unit, modify_entry[i]);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: FP Modify failed, EP=%d %s.\n",
                    unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
            }
        }
    }

    return (rv);
        
}

/*
 * Function:
 *     _bcm_tr3_oam_pri_map_profile_create
 * Purpose:
 *     Create ING_SERVICE_PRI_MAP entry for LM
 * Parameters:
 *     unit      - (IN) BCM device number
 *     oc        - (IN) Pointer to OAM control structure.
 *     hash_data - (IN) Pointer to endpoint hash data.
 *     endpoint_info - (IN) Pointer to endpoint info structure.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_pri_map_profile_create(int unit, _bcm_oam_control_t *oc, 
    _bcm_oam_hash_data_t *hash_data, bcm_oam_endpoint_info_t *endpoint_info) {

    int rv = 0;
    ing_service_pri_map_entry_t pri_ent[BCM_OAM_INTPRI_MAX];    /* profile */
    soc_mem_t   mem;                                /* Profiled table memory. */
    void *entries[1];                               /* Profile entry. */
    uint32 profile_index;                           /* Profile table index. */
    uint8 pri = 0;

    if ( (NULL == oc) || (NULL == hash_data) || (NULL == endpoint_info) ){
        return (BCM_E_INTERNAL);
    }
     
    /*
     * Initialize ingress priority map profile table.
     * All priorities priorities map to priority:'0'.
     */
    mem = ING_SERVICE_PRI_MAPm;
    for (pri = 0; pri < BCM_OAM_INTPRI_MAX; pri++) {
    
        /* Clear ingress service pri map profile entry. */
        sal_memcpy(&pri_ent[pri], soc_mem_entry_null(unit, mem),
                    soc_mem_entry_words(unit, mem) * sizeof(uint32));
                   
        if (SOC_MEM_FIELD_VALID(unit, mem, OFFSETf)) {
            soc_mem_field32_set(unit, mem, &pri_ent[pri], OFFSETf, 
                                endpoint_info->pri_map[pri]);
        }

        if (SOC_MEM_FIELD_VALID(unit, mem, OFFSET_VALIDf)) {
            soc_mem_field32_set(unit, mem, &pri_ent[pri], OFFSET_VALIDf, 1);
        }
    }
    entries[0] = &pri_ent;
    rv = soc_profile_mem_add(unit, &oc->ing_service_pri_map,
                         (void *) &entries, BCM_OAM_INTPRI_MAX, &profile_index);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: service map profile add, EP=%d %s.\n", 
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    hash_data->pri_map_index = profile_index;

    return (rv);
                      
}

/*
 * Function:
 *    _bcm_lm_dm_search_cb
 * Purpose:
 *    Call back function to match existing LM endpoints over same vlan port.
 *    Gives Existing Highest level enpoint, excluding the current(EP in context)
 * Parameters:
 *    unit      - (IN) BCM device number.
 *    hash_key  - (IN) Pointer to endpoint hash key value.
 *    data      - (IN) Pointer to endpoint hash data.
 * Retruns:
 *    BCM_E_XXX
 */
STATIC int
_bcm_lm_dm_search_cb(int unit, shr_htb_key_t key, shr_htb_data_t data) {
    int rv = 0;
    _bcm_oam_hash_data_t *h_data_ptr;
    
    h_data_ptr = (_bcm_oam_hash_data_t *)data;
    if ( ( h_data_ptr->flags & (BCM_OAM_ENDPOINT_LOSS_MEASUREMENT | 
                               BCM_OAM_ENDPOINT_DELAY_MEASUREMENT) ) && 
         (h_data_ptr->in_use == 1) && 
         (h_data_ptr->ep_id != _bcm_oam_lm_dm_search_data.match_ep_id) && 
         (h_data_ptr->type == _bcm_oam_lm_dm_search_data.match_type) &&
         (h_data_ptr->vlan == _bcm_oam_lm_dm_search_data.match_vlan) &&
         (h_data_ptr->gport == _bcm_oam_lm_dm_search_data.match_gport) ) {
            _bcm_oam_lm_dm_search_data.match_count++;
            if (h_data_ptr->level >= _bcm_oam_lm_dm_search_data.highest_level) {
             _bcm_oam_lm_dm_search_data.highest_level = h_data_ptr->level;
             _bcm_oam_lm_dm_search_data.highest_level_ep_id = h_data_ptr->ep_id;
            }
        }
    
    return rv;
}

/*
 * Function:
 *     _bcm_tr3_oam_loss_delay_measurement_add
 * Purpose:
 *     Create LM/DM IFP entry and install the same.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     oc        - (IN) Pointer to OAM control structure.
 *     hash_data - (IN) Pointer to endpoint hash data.
 *     endpoint_info - (IN) Pointer to endpoint info structure.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_loss_delay_measurement_add(int unit, _bcm_oam_control_t *oc, 
    _bcm_oam_hash_data_t *hash_data, bcm_oam_endpoint_info_t *endpoint_info) {

    int rv=0;
    /*Hash data of existing endpoint on same vlan port */
    _bcm_oam_hash_data_t *existing_ep_hash_data = NULL;
    
    if ( (NULL == oc) || (NULL == hash_data) || (NULL == endpoint_info) ){
        return (BCM_E_INTERNAL);
    }
    
    sal_memset(&_bcm_oam_lm_dm_search_data, 0, 
                sizeof(_bcm_oam_lm_dm_search_data_type_t));

    /*Set the match criteria for LM search*/
    _bcm_oam_lm_dm_search_data.match_type = hash_data->type;
    _bcm_oam_lm_dm_search_data.match_ep_id = hash_data->ep_id;
    _bcm_oam_lm_dm_search_data.match_vlan = hash_data->vlan;
    _bcm_oam_lm_dm_search_data.match_gport = hash_data->gport;
    _bcm_oam_lm_dm_search_data.highest_level = 0;
    _bcm_oam_lm_dm_search_data.highest_level_ep_id = 0;
    
    /* Iterate over existing enpoint to find 
    if port vlan matching entry already exists */
    rv = shr_htb_iterate(unit, oc->ma_mep_htbl, _bcm_lm_dm_search_cb);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: LM Search failed, EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    
    OAM_VERB(("OAM(unit %d): Existing EP on vlan port count=%d, \
        Highest level=%d, EP=%d\n", unit, 
        _bcm_oam_lm_dm_search_data.match_count,
        _bcm_oam_lm_dm_search_data.highest_level, 
        _bcm_oam_lm_dm_search_data.highest_level_ep_id));    
    
    if (hash_data->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT) {
    
        /* If enpoint already exists on same vlan port, 
         * Use the Primap and Counter index of existing endpoint,
         * Its a hardware limitation that multi level endpoints 
         * on same vlan port cannot have different primap 
         */
        if (_bcm_oam_lm_dm_search_data.match_count > 0) {
          existing_ep_hash_data = 
          &oc->oam_hash_data[_bcm_oam_lm_dm_search_data.highest_level_ep_id];
          hash_data->pri_map_index = existing_ep_hash_data->pri_map_index;
          hash_data->lm_counter_index = existing_ep_hash_data->lm_counter_index;
            
        } else {
            /* Create ING_SERVICE_PRI_MAP profile for the endpoint */
            rv  = _bcm_tr3_oam_pri_map_profile_create(unit, oc, hash_data, 
                                                      endpoint_info);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: ING_SERVICE_PRI_MAP profile \
                creation, EP=%d %s\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
            }

            /* Allocate counter index. */
            rv = shr_idxres_list_alloc(oc->lm_counter_pool,
                        (shr_idxres_element_t *)&hash_data->lm_counter_index);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: lm counter idx alloc failed, \
                EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
            }
        }
    }

    /* Create Field entries / modify existing if needed. Also install*/
    rv  = _bcm_tr3_oam_fp_create(unit, oc, hash_data);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: FP Create failed, EP=%d %s.\n",
                 unit, hash_data->ep_id, bcm_errmsg(rv)));
                 
        /* In case of failure, free the counter index and primap 
         * only if the same has been newly created for this endpoint */
        if (hash_data->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT) {
    
            /* If another enpoint exists on same vlan port, 
             * Don't free primap and counter index, as they are shared */
            if (_bcm_oam_lm_dm_search_data.match_count == 0){
            
                /* Free ING_SERVICE_PRI_MAP profile */
                soc_profile_mem_delete(unit, &oc->ing_service_pri_map, 
                                       hash_data->pri_map_index);

                /* Free counter index. */
                shr_idxres_list_free( oc->lm_counter_pool,
                            (shr_idxres_element_t)hash_data->lm_counter_index);
            }
        }
        return (rv);
    }
    
    /* Increment the group FP count */
    oc->fp_glp_entry_cnt++;
    OAM_VERB(("OAM(unit %d): oc->fp_glp_entry_cnt=%d\n", 
              unit, oc->fp_glp_entry_cnt));
    return (rv);
}


/*
 * Function:
 *     _bcm_tr3_oam_loss_delay_measurement_delete
 * Purpose:
 *     Delete LM/DM IFP entry and install the same.
 * Parameters:
 *     unit      - (IN) BCM device number
 *     oc        - (IN) Pointer to OAM control structure.
 *     hash_data - (IN) Pointer to endpoint hash data.
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_loss_delay_measurement_delete(int unit, _bcm_oam_control_t *oc, 
                                            _bcm_oam_hash_data_t *hash_data) {

    int rv=0;
    /*Hash data of existing endpoint on same vlan port */
    _bcm_oam_hash_data_t *existing_ep_hash_data = NULL;
    
    if ( (NULL == oc) || (NULL == hash_data) ){
        return (BCM_E_INTERNAL);
    }    
    
    sal_memset(&_bcm_oam_lm_dm_search_data, 0, 
               sizeof(_bcm_oam_lm_dm_search_data_type_t));

    /*Set the match criteria for LM search*/
    _bcm_oam_lm_dm_search_data.match_type = hash_data->type;
    _bcm_oam_lm_dm_search_data.match_ep_id = hash_data->ep_id;
    _bcm_oam_lm_dm_search_data.match_vlan = hash_data->vlan;
    _bcm_oam_lm_dm_search_data.match_gport = hash_data->gport;
    _bcm_oam_lm_dm_search_data.highest_level = 0;
    _bcm_oam_lm_dm_search_data.highest_level_ep_id = 0;
    
    /* Iterate over existing enpoint to find if 
       other port vlan matching entry already exists */
    rv = shr_htb_iterate(unit, oc->ma_mep_htbl, _bcm_lm_dm_search_cb);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: LM Search failed, EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    
    OAM_VERB(("OAM(unit %d): Existing EP on vlan port count=%d, \
               Highest level=%d, EP=%d\n", 
              unit, _bcm_oam_lm_dm_search_data.match_count,
                    _bcm_oam_lm_dm_search_data.highest_level, 
                    _bcm_oam_lm_dm_search_data.highest_level_ep_id));    
    
    if (hash_data->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT) {
    
        /* If another enpoint exists on same vlan port, 
            Don't free primap and counter index, as they are shared */
        if (_bcm_oam_lm_dm_search_data.match_count == 0){
        
            /* Free ING_SERVICE_PRI_MAP profile */
            rv = soc_profile_mem_delete(unit, 
                            &oc->ing_service_pri_map, hash_data->pri_map_index);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: ING_SERVICE_PRI_MAP profile \
                deletion, EP=%d %s.\n",unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
            }

            /* Free counter index. */
            rv = shr_idxres_list_free( oc->lm_counter_pool, 
                            (shr_idxres_element_t)hash_data->lm_counter_index);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: lm counter idx free failed, \
                EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
            }
        }
    }

    /* If the EP we are deleting is at the lowest level and
       other EP exist at higher level
       Modify the other EP to remove Ethertype and Level match criteria. */
    if ( _bcm_oam_lm_dm_search_data.match_count > 0 && 
         _bcm_oam_lm_dm_search_data.highest_level < hash_data->level ) {
    
        existing_ep_hash_data = 
            &oc->oam_hash_data[_bcm_oam_lm_dm_search_data.highest_level_ep_id];

        rv = bcm_esw_field_qualifier_delete(unit, 
                existing_ep_hash_data->fp_entry_tx, bcmFieldQualifyEtherType);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Eth Type dequlaify (tx), EP=%d %s.\n",
                    unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
        }

        rv = bcm_esw_field_qualifier_delete(unit, 
                existing_ep_hash_data->fp_entry_tx, bcmFieldQualifyDstIp6);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Level dequlaify (tx), EP=%d %s.\n",
                    unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
        }

        rv = bcm_esw_field_qualifier_delete(unit, 
                existing_ep_hash_data->fp_entry_rx, bcmFieldQualifyEtherType);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Eth Type dequlaify (rx), EP=%d %s.\n",
                    unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
        }

        rv = bcm_esw_field_qualifier_delete(unit, 
                existing_ep_hash_data->fp_entry_rx, bcmFieldQualifyDstIp6);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Level dequlaify (rx), EP=%d %s.\n",
                    unit, hash_data->ep_id, bcm_errmsg(rv)));
                return (rv);
        }

        /* Install Modified entries */
        rv = bcm_esw_field_entry_install(unit, 
                                         existing_ep_hash_data->fp_entry_tx);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: FP Install failed (tx), EP=%d %s.\n",
                unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }

        rv = bcm_esw_field_entry_install(unit, 
                                         existing_ep_hash_data->fp_entry_rx);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: FP Install failed (tx), EP=%d %s.\n",
                unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
        
    }

    /* Delete FP entries for this EP*/
    /* Tx Entry */
    rv = bcm_esw_field_entry_destroy(unit, hash_data->fp_entry_tx);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: FP Destroy failed (tx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    hash_data->fp_entry_tx = -1;
    
    /* Rx Entry */
    rv = bcm_esw_field_entry_destroy(unit, hash_data->fp_entry_rx);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: FP Destroy failed (rx), EP=%d %s.\n",
            unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }
    hash_data->fp_entry_rx = -1;
    
    /* Trunk Entry */
    if (hash_data->fp_entry_trunk[0] != -1) {
        rv = bcm_esw_field_entry_destroy(unit, hash_data->fp_entry_trunk[0]);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: FP Destroy failed (Trunk), \
                     EP=%d %s.\n", unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
        hash_data->fp_entry_trunk[0] = -1;
    }
    
    /* Decrement the group FP count */
    oc->fp_glp_entry_cnt--;
    OAM_VERB(("OAM(unit %d): oc->fp_glp_entry_cnt=%d\n", 
              unit, oc->fp_glp_entry_cnt));
    /* If count goes 0 delete the group */
    if (!(oc->fp_glp_entry_cnt)) {
        rv = bcm_esw_field_group_destroy(unit, oc->fp_glp_group);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: FP Group Destroy failed, EP=%d %s.\n",
                     unit, hash_data->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
    }

    return (rv);
}

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
_bcm_tr3_oam_sw_dump(int unit)
{
    soc_cm_print("OAM\n");
}
#endif

#if defined(BCM_WARM_BOOT_SUPPORT)
/*
 * Function:
 *     _bcm_tr3_oam_endpoint_alloc
 * Purpose:
 *     Allocate an endpoint memory element.
 * Parameters:
 *     ep_pp - (IN/OUT) Pointer to endpoint address pointer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_endpoint_alloc(bcm_oam_endpoint_info_t **ep_pp)
{
    bcm_oam_endpoint_info_t *ep_p = NULL;

    _BCM_OAM_ALLOC(ep_p, bcm_oam_endpoint_info_t,
                   sizeof(bcm_oam_endpoint_info_t), "Endpoint info");
    if (NULL == ep_p) {
        return (BCM_E_MEMORY);
    }
    
    *ep_pp = ep_p;

    return (BCM_E_NONE);

}

/*
 * Function:
 *     _bcm_tr3_oam_sync
 * Purpose:
 *     Store OAM configuration to level two storage cache memory.
 * Parameters:
 *     unit - (IN) Device unit number
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_tr3_oam_sync(int unit)
{
    _bcm_oam_control_t  *oc;
    int                 alloc_size = 0;
    int                 stable_size;
    soc_scache_handle_t scache_handle;
    uint8               *oam_scache;
    int                 grp_idx;
    _bcm_oam_group_data_t *group_p;   /* Pointer to group list.         */
    int                 group_count = 0;
    int                 rv;


    /* Get OAM module storage size. */
    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    /* If level 2 store is not configured return from here. */
    if (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit) || (stable_size == 0)) {
        return (BCM_E_NONE);                                                      
    }

    /* Get handle to control structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    /* Initialize to group array pointer. */
    group_p = oc->group_info;

    for (grp_idx = 0; grp_idx < oc->group_count; grp_idx++) {

        /* Check if the group is in use. */
        if (BCM_E_EXISTS
            == shr_idxres_list_elem_state(oc->group_pool, grp_idx)) {
            alloc_size += BCM_OAM_GROUP_NAME_LENGTH;
            group_count++;
        }
    }
    
    /* To store OAM group count. */
    alloc_size += sizeof(int);

    /* To store FP GID. */
    alloc_size += 3 * sizeof(bcm_field_group_t);

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_OAM, 0);

    /* Check if memory has already been allocated */
    rv = _bcm_esw_scache_ptr_get(unit,
                                 scache_handle,
                                 0,
                                 alloc_size,
                                 &oam_scache,
                                 BCM_WB_DEFAULT_VERSION,
                                 NULL
                                 );
    if (!SOC_WARM_BOOT(unit) && (BCM_E_NOT_FOUND == rv)) {
        rv = _bcm_esw_scache_ptr_get(unit,
                                     scache_handle,
                                     1,
                                     alloc_size,
                                     &oam_scache,
                                     BCM_WB_DEFAULT_VERSION,
                                     NULL
                                     );
        if (BCM_FAILURE(rv)
            || (NULL == oam_scache)) {
            goto cleanup;
        }
    } else if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    /* Store the FP groups */
    sal_memcpy(oam_scache, &oc->vfp_group, sizeof(bcm_field_group_t));
    oam_scache += sizeof(bcm_field_group_t);

    sal_memcpy(oam_scache, &oc->fp_vp_group, sizeof(bcm_field_group_t));
    oam_scache += sizeof(bcm_field_group_t);

    sal_memcpy(oam_scache, &oc->fp_glp_group, sizeof(bcm_field_group_t));
    oam_scache += sizeof(bcm_field_group_t);

    sal_memcpy(oam_scache, &group_count, sizeof(int));
    oam_scache += sizeof(int);

    for (grp_idx = 0; grp_idx < oc->group_count; ++grp_idx)
    {
        /* Check if the group is in use. */
        if (BCM_E_EXISTS
                == shr_idxres_list_elem_state(oc->group_pool, grp_idx)) {

            sal_memcpy(oam_scache, group_p[grp_idx].name,
                       BCM_OAM_GROUP_NAME_LENGTH);

            oam_scache += BCM_OAM_GROUP_NAME_LENGTH;
        }
    }

cleanup:
    _BCM_OAM_UNLOCK(oc);
    return (rv);
}

/*
 * Function:
 *     _bcm_tr3_oam_wb_group_recover
 * Purpose:
 *     Recover OAM group configuratoin.
 * Parameters:
 *     unit        - (IN) BCM device number
 *     stable_size - (IN) OAM module Level2 storage size.
 *     oam_scache  - (IN) Pointer to scache address pointer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_wb_group_recover(int unit, int stable_size, uint8 **oam_scache)
{
    int                    index;                    /* Hw table index.     */
    _bcm_oam_group_data_t  *group_p;                 /* Group info pointer. */
    maid_reduction_entry_t maid_entry;               /* Group entry info.   */
    ma_state_entry_t       ma_state_ent;             /* Group state info.   */
    int                    maid_reduction_valid = 0; /* Group valid.        */
    int                    ma_state_valid = 0;       /* Group state valid.  */
    _bcm_oam_control_t     *oc;                      /* Pointer to Control  */
                                                     /* structure.          */
    int                     rv;                      /* Operation return    */
                                                     /* status.             */

    /* Control lock taken by calling routine. */
    /* Get OAM control structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    for (index = 0; index < oc->group_count; index++) {

        rv = READ_MAID_REDUCTIONm(unit, MEM_BLOCK_ANY, index,
                                  &maid_entry);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: (GID=%d) MAID_REDUCTION table read"
                    " failed  - %s.\n", unit, index, bcm_errmsg(rv)));
            goto cleanup;
        }

        rv = READ_MA_STATEm(unit, MEM_BLOCK_ANY, index, &ma_state_ent);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: (GID=%d) MA_STATE table read"
                    " failed  - %s.\n", unit, index, bcm_errmsg(rv)));
            goto cleanup;
        }

        maid_reduction_valid
            = soc_MAID_REDUCTIONm_field32_get(unit, &maid_entry, VALIDf);

        ma_state_valid
            = soc_MA_STATEm_field32_get(unit, &ma_state_ent, VALIDf);

        if (maid_reduction_valid || ma_state_valid) {

            /* Entry must be valid in both the tables, else return error. */
            if (!maid_reduction_valid || !ma_state_valid) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }

            /* Get the group memory pointer. */
            group_p = &oc->group_info[index];
            if (NULL == group_p) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }

            if (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit) || (stable_size == 0)) {

                /* Set group name as zeros. */
                sal_memset(group_p->name, 0, BCM_OAM_GROUP_NAME_LENGTH);

            } else {

                /* Get the group name from stored info. */
                sal_memcpy(group_p->name, *oam_scache, BCM_OAM_GROUP_NAME_LENGTH);
                *oam_scache = (*oam_scache + BCM_OAM_GROUP_NAME_LENGTH);
            }

            /* Reserve the group index. */
            rv = shr_idxres_list_reserve(oc->group_pool, index, index);
            if (BCM_FAILURE(rv)) {
                rv = (rv == BCM_E_RESOURCE) ? (BCM_E_EXISTS) : rv;
                OAM_ERR(("OAM(unit %d) Error: (GID=%d) Index reserve "
                        " failed  - %s.\n", unit, index, bcm_errmsg(rv)));
                goto cleanup;
            }

            /* Create the linked list to maintain group endpoint information. */
            _BCM_OAM_ALLOC((group_p->ep_list), _bcm_oam_ep_list_t *,
                           sizeof(_bcm_oam_ep_list_t *), "EP list head");
            if (NULL == group_p->ep_list) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }

            /* Initialize head node.*/
            *group_p->ep_list =  NULL;
        }
    }

    return (BCM_E_NONE);

cleanup:

    if (BCM_E_EXISTS
        == shr_idxres_list_elem_state(oc->group_pool, index)) {
        shr_idxres_list_free(oc->group_pool, index);
    }

    return (rv);

}

/*
 * Function:
 *     _bcm_tr3_oam_rmep_recover
 * Purpose:
 *     Recover OAM remote endpoint configuration.
 * Parameters:
 *     unit        - (IN) BCM device number
 *     index       - (IN) Remote MEP hardware index.
 *     l3_entry    - (IN) RMEP view table entry pointer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_rmep_recover(int unit,
                          int index,
                          uint32 *l3_entry)
{
    rmep_entry_t            rmep_entry;      /* Remote table entry.           */
    _bcm_oam_hash_data_t    *h_data_p;       /* Endpoint hash data pointer.   */
    _bcm_oam_control_t      *oc;             /* Pointer to control structure. */
    int                     rv;              /* Operation return status.      */
    _bcm_gport_dest_t       gport_dest;      /* Gport specification.          */
    bcm_gport_t             gport;           /* Gport value.                  */
    uint8                   source_type;     /* Virtual or Logical Port.      */
    uint16                  glp;             /* Generic logical port value.   */
    bcm_oam_endpoint_info_t *ep_info = NULL; /* Endpoint information.         */
    _bcm_oam_hash_key_t     hash_key;        /* Hash key buffer for lookup.   */
    int                     ep_id;           /* Endpoint ID.                  */
    soc_mem_t               mem;             /* L3 entry memory               */

    OAM_VVERB(("OAM(unit %d) Info: _bcm_tr3_oam_rmep_recover.\n", unit));
    /*
     * Get OAM control structure.
     *     Note: Lock taken by calling routine.
     */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    sal_memset(&l3_entry, 0, sizeof(l3_entry));
    /* Allocate the next available endpoint index. */
    rv = shr_idxres_list_alloc(oc->mep_pool,
                               (shr_idxres_element_t *)&ep_id);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Endpoint alloc (EP=%d) - %s.\n",
                unit, ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    h_data_p = &oc->oam_hash_data[ep_id];
    if (NULL == h_data_p) {
        goto cleanup;
    }

    /* 
     * Clear the hash data element contents before
     * storing values.
     */
    _BCM_OAM_HASH_DATA_CLEAR(h_data_p);

    if (SOC_IS_HURRICANE2(unit)) {
        mem = L3_ENTRY_IPV4_UNICASTm;
    } else {
        mem = L3_ENTRY_1m;
    }

    /* Get RMEP table index from LMEP view entry. */
    h_data_p->remote_index
        = soc_mem_field32_get(unit, mem, l3_entry, RMEP__RMEP_PTRf);

    /* Get RMEP table entry contents. */
    rv = READ_RMEPm(unit, MEM_BLOCK_ANY, h_data_p->remote_index, &rmep_entry);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: RMEP (index=%d) read failed  - %s.\n",
                unit, h_data_p->remote_index, bcm_errmsg(rv)));
        goto cleanup;
    }

   /* Has to be a valid RMEP, return error otherwise. */
    if (!soc_RMEPm_field32_get(unit, &rmep_entry, VALIDf)) {
        rv = BCM_E_INTERNAL;
        goto cleanup;
    }
    
    h_data_p->ep_id = ep_id;

    h_data_p->is_remote = 1;

    h_data_p->flags |= BCM_OAM_ENDPOINT_REMOTE;

    h_data_p->group_index
        = soc_RMEPm_field32_get(unit, &rmep_entry, MAID_INDEXf);

    h_data_p->period
        = _bcm_tr3_oam_ccm_hw_encode_to_msecs
            ((int) soc_RMEPm_field32_get(unit, &rmep_entry,
                                         RMEP_RECEIVED_CCMf));

    h_data_p->name
        = soc_mem_field32_get(unit, mem, l3_entry, RMEP__MEPIDf);

    h_data_p->level
        = soc_mem_field32_get(unit, mem, l3_entry, RMEP__MDLf);

    h_data_p->vlan
        = soc_mem_field32_get(unit, mem, l3_entry, RMEP__VIDf);

    h_data_p->local_tx_index = _BCM_OAM_INVALID_INDEX;
    h_data_p->local_rx_index = _BCM_OAM_INVALID_INDEX;

    source_type
        = soc_mem_field32_get(unit, mem, l3_entry, RMEP__SOURCE_TYPEf);

    glp = soc_mem_field32_get(unit, mem, l3_entry, RMEP__SGLPf);

    /* Check if virtual port type. */
    if (1 == source_type) {
        /*
         * Virtual port type, construct gport value from VP.
         */
        h_data_p->vp = glp;
#if defined(INCLUDE_L3)
        if (_bcm_vp_used_get(unit, glp, _bcmVpTypeMim)) {
            BCM_GPORT_MIM_PORT_ID_SET(h_data_p->gport, h_data_p->vp);

         } else if (_bcm_vp_used_get(unit, glp, _bcmVpTypeMpls)) {

            BCM_GPORT_MPLS_PORT_ID_SET(h_data_p->gport, h_data_p->vp);

         } else {
            OAM_ERR(("OAM(unit %d) Error: Invalid Virtual Port (SVP=%d) "
                    "- %s.\n", unit, h_data_p->vp, bcm_errmsg(BCM_E_INTERNAL)));
            rv = BCM_E_INTERNAL;
            goto cleanup;
         }
#endif
    } else {
        /*
         * Generic logical port type, construct gport from GLP.
         */
        h_data_p->sglp = glp;

        _bcm_gport_dest_t_init(&gport_dest);

        if (_BCM_OAM_GLP_TRUNK_BIT_GET(glp)) {
            gport_dest.tgid = _BCM_OAM_GLP_TRUNK_ID_GET(glp);
            gport_dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
        } else {
            gport_dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
            gport_dest.modid = _BCM_OAM_GLP_MODULE_ID_GET(glp);
            gport_dest.port = _BCM_OAM_GLP_PORT_GET(glp);
        }

        rv = _bcm_esw_gport_construct(unit, &gport_dest, &gport);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Gport construct failed - %s.\n",
                    unit, bcm_errmsg(rv)));
            goto cleanup;
        }

        h_data_p->gport = gport;
    }

    rv = shr_idxres_list_reserve(oc->rmep_pool,
                                 h_data_p->remote_index,
                                 h_data_p->remote_index);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: (RMEP=%d) Index reserve failed  - %s.\n",
                unit, h_data_p->remote_index, bcm_errmsg(rv)));
        rv = (rv == BCM_E_RESOURCE) ? (BCM_E_EXISTS) : rv;
        goto cleanup;
    }

    h_data_p->in_use = 1;

    rv = _bcm_oam_group_ep_list_add(unit, h_data_p->group_index, ep_id);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    rv = _bcm_tr3_oam_endpoint_alloc(&ep_info);
    if (BCM_FAILURE(rv)) {
        _bcm_oam_group_ep_list_remove(unit, h_data_p->group_index, ep_id);
        goto cleanup;
    }

    bcm_oam_endpoint_info_t_init(ep_info);

    /* Set up endpoint information for key construction. */
    ep_info->group = h_data_p->group_index;
    ep_info->name = h_data_p->name;
    ep_info->gport = h_data_p->gport;
    ep_info->level = h_data_p->level;
    ep_info->vlan = h_data_p->vlan;

    /* Calculate hash key for hash table insert operation. */
    _bcm_oam_ep_hash_key_construct(unit, oc, ep_info, &hash_key);

    rv = shr_htb_insert(oc->ma_mep_htbl, hash_key, h_data_p);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Hash table insert failed "
                "EP=%d %s.\n", unit, h_data_p->ep_id, bcm_errmsg(rv)));
        _bcm_oam_group_ep_list_remove(unit, h_data_p->group_index, ep_id);
        goto cleanup;
    } else {
        OAM_VVERB(("OAM(unit %d) Info: Hash Tbl (EP=%d) inserted"
                  " - %s.\n", unit, ep_id, bcm_errmsg(rv)));
    }
    sal_free(ep_info);

    OAM_VVERB(("OAM(unit %d) Info: _bcm_tr3_oam_rmep_recover - done.\n", unit));

    return (rv);

cleanup:

    if (NULL != ep_info) {
        sal_free(ep_info);
    }

    /* Release the endpoint ID if in use. */
    if (BCM_E_EXISTS
        == shr_idxres_list_elem_state(oc->mep_pool, ep_id)) {
        shr_idxres_list_free(oc->mep_pool, ep_id);
    }

    /* Release the remote index if in use. */
    if ((NULL != h_data_p)
        && (BCM_E_EXISTS
            == shr_idxres_list_elem_state(oc->rmep_pool,
                                          h_data_p->remote_index))) {
        shr_idxres_list_free(oc->rmep_pool, h_data_p->remote_index);

        _BCM_OAM_HASH_DATA_CLEAR(h_data_p);
    }

    OAM_VVERB(("OAM(unit %d) Info: _bcm_tr3_oam_rmep_recover - error_done.\n",
              unit));
    return (rv);
}

/*
 * Function:
 *     _bcm_tr3_oam_lmep_rx_config_recover
 * Purpose:
 *     Recover OAM local endpoint Rx configuration.
 * Parameters:
 *     unit        - (IN) BCM device number
 *     index       - (IN) Remote MEP hardware index.
 *     l3_entry    - (IN) LMEP view table entry pointer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_lmep_rx_config_recover(int unit,
                                    int index,
                                    uint32 *l3_entry)
{
    _bcm_oam_hash_data_t    *h_data_p = NULL; /* Endpoint hash data pointer. */
    ma_index_entry_t        ma_idx_entry; /* IA_INDEX table entry.           */
    _bcm_oam_control_t      *oc;          /* Pointer to control structure.   */
    uint8                   mdl_bitmap;   /* Endpoint domain level bitmap.   */
    uint8                   mdl;          /* Maintenance domain level.       */
    int                     rv;           /* Operation return status.        */
    _bcm_gport_dest_t       gport_dest;   /* Gport specification.            */
    bcm_gport_t             gport;        /* Gport value.                    */
    uint8                   source_type;  /* Virtual or Logical Port.        */
    uint16                  glp;          /* Generic logical port value.     */
    int                     ep_id;        /* Endpoint ID.                    */
    _bcm_oam_hash_key_t     hash_key;     /* Hash key buffer for lookup.     */
    bcm_oam_endpoint_info_t *ep_info = NULL; /* Endpoint information.        */
    soc_mem_t               mem;           /* L3 entry memory                */
    
    OAM_VVERB(("OAM(unit %d) Info: _bcm_tr3_oam_lmep_rx_config_recover.\n",
              unit));
    /*
     * Get OAM control structure.
     *     Note: Lock taken by calling routine.
     */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    if (SOC_IS_HURRICANE2(unit)) {
        mem = L3_ENTRY_IPV4_UNICASTm;
    } else {
        mem = L3_ENTRY_1m;
    }

    mdl_bitmap
        = soc_mem_field32_get(unit, mem, l3_entry, LMEP__MDL_BITMAPf);
    
    for (mdl = 0; mdl < _BCM_OAM_EP_LEVEL_COUNT; mdl++) {

        if (mdl_bitmap & (1 << mdl)) {

            /* Allocate the next available endpoint index. */
            rv = shr_idxres_list_alloc(oc->mep_pool,
                                       (shr_idxres_element_t *)&ep_id);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Endpoint alloc (EP=%d) - %s.\n",
                        unit, ep_id, bcm_errmsg(rv)));
                goto cleanup;
            }

            h_data_p = &oc->oam_hash_data[ep_id];
            if (NULL == h_data_p) {
                goto cleanup;
            }

            /* 
             * Clear the hash data element contents before
             * storing values.
             */
            _BCM_OAM_HASH_DATA_CLEAR(h_data_p);

            _BCM_OAM_HASH_DATA_HW_IDX_INIT(h_data_p);

            h_data_p->ep_id = ep_id;

            h_data_p->local_rx_index
                = ((soc_mem_field32_get(unit, mem, l3_entry,
                        LMEP__MA_BASE_PTRf) << _BCM_OAM_EP_LEVEL_BIT_COUNT)
                        | mdl);

            rv = READ_MA_INDEXm
                    (unit, MEM_BLOCK_ANY, h_data_p->local_rx_index,
                     &ma_idx_entry);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: MA_INDEX (index:%d) read failed -"
                        " %s.\n", unit, h_data_p->local_rx_index,
                        bcm_errmsg(rv)));
                goto cleanup;
            }

            h_data_p->in_use = 1;

            h_data_p->is_remote = 0;

            h_data_p->local_rx_enabled = 1;

            h_data_p->flags |= BCM_OAM_ENDPOINT_CCM_RX;

            h_data_p->group_index
                = soc_MA_INDEXm_field32_get(unit, &ma_idx_entry, MA_PTRf);

            h_data_p->profile_index
                = soc_MA_INDEXm_field32_get
                    (unit, &ma_idx_entry, OAM_OPCODE_CONTROL_PROFILE_PTRf);

            h_data_p->name = 0xffff;
            
            h_data_p->level = mdl;

            h_data_p->vlan
                = soc_mem_field32_get(unit, mem, l3_entry, LMEP__VIDf);

            source_type
                = soc_mem_field32_get(unit, mem, l3_entry, LMEP__SOURCE_TYPEf);

            glp = soc_mem_field32_get(unit, mem, l3_entry, LMEP__SGLPf);


            /* Check if virtual port type. */
            if (1 == source_type) {
                /*
                 * Virtual port type, construct gport value from VP.
                 */
                h_data_p->vp = glp;
#if defined(INCLUDE_L3)
                if (_bcm_vp_used_get(unit, glp, _bcmVpTypeMim)) {

                    BCM_GPORT_MIM_PORT_ID_SET(h_data_p->gport, h_data_p->vp);

                } else if (_bcm_vp_used_get(unit, glp, _bcmVpTypeMpls)) {

                    BCM_GPORT_MPLS_PORT_ID_SET(h_data_p->gport, h_data_p->vp);

                } else {
                    OAM_ERR(("OAM(unit %d) Error: Invalid Virtual Port (SVP=%d)"
                            " - %s.\n", unit, h_data_p->vp,
                            bcm_errmsg(BCM_E_INTERNAL)));
                    return (BCM_E_INTERNAL);
                }
#endif
            } else {
                /*
                 * Generic logical port type, construct gport from GLP.
                 */
                h_data_p->sglp = glp;

                _bcm_gport_dest_t_init(&gport_dest);

                if (_BCM_OAM_GLP_TRUNK_BIT_GET(glp)) {
                    gport_dest.tgid = _BCM_OAM_GLP_TRUNK_ID_GET(glp);
                    gport_dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
                } else {
                    gport_dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
                    gport_dest.modid = _BCM_OAM_GLP_MODULE_ID_GET(glp);
                    gport_dest.port = _BCM_OAM_GLP_PORT_GET(glp);
                }

                rv = _bcm_esw_gport_construct(unit, &gport_dest, &gport);
                if (BCM_FAILURE(rv)) {
                    OAM_ERR(("OAM(unit %d) Error: Gport construct failed -"
                            " %s.\n", unit, bcm_errmsg(rv)));
                    goto cleanup;
                }

                h_data_p->gport = gport;
            }

            rv = shr_idxres_list_reserve(oc->ma_idx_pool,
                                         h_data_p->local_rx_index,
                                         h_data_p->local_rx_index);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: (RMEP=%d) Index reserve failed"
                        "  - %s.\n", unit, h_data_p->remote_index,
                            bcm_errmsg(rv)));
                rv = (rv == BCM_E_RESOURCE) ? (BCM_E_EXISTS) : rv;
                goto cleanup;
            }

            rv = _bcm_oam_group_ep_list_add(unit, h_data_p->group_index, ep_id);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }

            rv = _bcm_tr3_oam_endpoint_alloc(&ep_info);
            if (BCM_FAILURE(rv)) {
                _bcm_oam_group_ep_list_remove(unit,
                                              h_data_p->group_index,
                                              ep_id);
                goto cleanup;
            }

            bcm_oam_endpoint_info_t_init(ep_info);

            /* Set up endpoint information for key construction. */
            ep_info->group = h_data_p->group_index;
            ep_info->name = h_data_p->name;
            ep_info->gport = h_data_p->gport;
            ep_info->level = h_data_p->level;
            ep_info->vlan = h_data_p->vlan;

            /*
             * Calculate hash key for hash table insert
             * operation.
             */
            _bcm_oam_ep_hash_key_construct(unit, oc, ep_info, &hash_key);

            sal_free(ep_info);

            rv = shr_htb_insert(oc->ma_mep_htbl, hash_key, h_data_p);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Hash table insert"
                        " (EP=%d) failed - %s.\n", unit,
                        h_data_p->ep_id, bcm_errmsg(rv)));

                _bcm_oam_group_ep_list_remove(unit,
                                              h_data_p->group_index,
                                              ep_id);
                goto cleanup;
            } else {
                OAM_VVERB(("OAM(unit %d) Info: Hash Tbl (EP=%d)"
                          " inserted  - %s.\n", unit, ep_id,
                          bcm_errmsg(rv)));
            }
        }
    }

    OAM_VVERB(("OAM(unit %d) Info: _bcm_tr3_oam_lmep_rx_config_recover"
              " - done.\n", unit));
    return (BCM_E_NONE);

cleanup:

    if (NULL != ep_info) {
        sal_free(ep_info);
    }

    if (BCM_E_EXISTS
        == shr_idxres_list_elem_state(oc->mep_pool, ep_id)) {
        shr_idxres_list_free(oc->mep_pool, ep_id);
    }

    if (NULL != h_data_p
        && (BCM_E_EXISTS
            == shr_idxres_list_elem_state(oc->ma_idx_pool,
                                          h_data_p->local_rx_index))) {

        shr_idxres_list_free(oc->ma_idx_pool, h_data_p->local_rx_index);

        _BCM_OAM_HASH_DATA_CLEAR(h_data_p);
    }

    OAM_VVERB(("OAM(unit %d) Info: _bcm_tr3_oam_lmep_rx_config_recover"
              " - error_done.\n", unit));
    return (rv);
}

/*
 * Function:
 *     _bcm_tr3_oam_lmep_tx_config_recover
 * Purpose:
 *     Recover OAM local endpoint Tx configuration.
 * Parameters:
 *     unit        - (IN) BCM device number
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_lmep_tx_config_recover(int unit)
{
    _bcm_gport_dest_t       gport_dest;   /* Gport specification.             */
    bcm_gport_t             gport;        /* Gport value.                     */
    int                     index;        /* Hardware index.                  */
    lmep_entry_t            lmep_entry;   /* LMEP table entry buffer.         */
    maid_reduction_entry_t  maid_red_ent; /* MAID_REDUCTION table entry buf.  */ 
    _bcm_oam_hash_key_t     hash_key;     /* Hash key buffer for lookup.      */
    _bcm_oam_group_data_t   *g_info_p;    /* Group information pointer.       */
    _bcm_oam_control_t      *oc;          /* Pointer to control structure.    */
    bcm_module_t            modid;        /* Module ID.                       */
    bcm_port_t              port_id;      /* Port ID.                         */
    bcm_trunk_t             trunk_id;     /* Trunk ID.                        */
    uint32                  grp_idx;      /* Group index.                     */
    uint16                  glp;          /* Generic logical port.            */
    uint16                  vlan;         /* VLAN ID.                         */
    uint8                   level;        /* Maintenance domain level.        */
    _bcm_oam_ep_list_t      *cur;         /* Current head node pointer.       */
    int                     ep_id = -1;   /* Endpoint ID.                     */
    uint8                   match_found = 0; /* Matching endpoint found.      */
    int                     rv;           /* Operation return status.         */
    bcm_oam_endpoint_info_t *ep_info = NULL; /* Endpoint information.         */
    _bcm_oam_hash_data_t    *h_data_p = NULL; /* Endpoint hash data pointer.  */
    _bcm_oam_hash_data_t    *sh_data_p = NULL; /* Endpoint hash data pointer. */

    OAM_VVERB(("OAM(unit %d) Info: _bcm_tr3_oam_lmep_tx_config_recover.\n",
              unit));

    /*
     * Get OAM control structure.
     *     Note: Lock taken by calling routine.
     */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    rv = _bcm_tr3_oam_endpoint_alloc(&ep_info);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    /*
     * At this point, Remote MEP and Local MEP Rx config has been
     *  recovered. Now, recover the Tx config for Local MEPs.
     */
    for (index = 0; index < oc->lmep_count; index++) {

        /* Get the LMEP table entry. */
        rv = READ_LMEPm(unit, MEM_BLOCK_ANY, index, &lmep_entry);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: LMEP table read (index=%d) failed "
                    "- %s.\n", unit, index, bcm_errmsg(rv)));
            goto cleanup;
        }

        grp_idx = soc_LMEPm_field32_get(unit, &lmep_entry, MAID_INDEXf);

        rv = READ_MAID_REDUCTIONm(unit, MEM_BLOCK_ANY, grp_idx, &maid_red_ent);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: MAID_REDU read (GID=%d) failed "
                    "- %s.\n", unit, grp_idx, bcm_errmsg(rv)));
            goto cleanup;
        }

        if (soc_MAID_REDUCTIONm_field32_get(unit, &maid_red_ent, VALIDf)) {

            /* Get pointer to group memory. */
            g_info_p = &oc->group_info[grp_idx];
            if (NULL == g_info_p) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }

            vlan = soc_LMEPm_field32_get(unit, &lmep_entry, VLAN_IDf);

            glp = soc_LMEPm_field32_get(unit, &lmep_entry, DESTf);

            level = soc_LMEPm_field32_get(unit, &lmep_entry, MDLf);

            modid = _BCM_OAM_DGLP_MODULE_ID_GET(glp);

            port_id = _BCM_OAM_GLP_PORT_GET(glp);

            trunk_id = BCM_TRUNK_INVALID;
            rv = bcm_esw_trunk_find(unit, modid, port_id, &trunk_id);
            if (BCM_FAILURE(rv)
                && (BCM_E_NOT_FOUND != rv)) {
                goto cleanup;
            }

            _bcm_gport_dest_t_init(&gport_dest);

            if (BCM_TRUNK_INVALID != trunk_id) {
                gport_dest.tgid = _BCM_OAM_GLP_TRUNK_ID_GET(glp);
                gport_dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
            } else {
                gport_dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
                gport_dest.modid = _BCM_OAM_DGLP_MODULE_ID_GET(glp);
                gport_dest.port = _BCM_OAM_GLP_PORT_GET(glp);
            }

            rv = _bcm_esw_gport_construct(unit, &gport_dest, &gport);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Gport construct failed"
                        " - %s.\n", unit, bcm_errmsg(rv)));
                goto cleanup;
            }

            /* Get the endpoint list head pointer. */
            cur = *(g_info_p->ep_list);
            if (NULL != cur) {
                while (NULL != cur) {
                    h_data_p = cur->ep_data_p;
                    if (NULL == h_data_p) {
                        OAM_ERR(("OAM(unit %d) Error: Group (GID=%d) NULL"
                                " endpoint list.\n", unit, grp_idx));
                        rv = BCM_E_INTERNAL;
                        goto cleanup;
                    }

                    if (vlan == h_data_p->vlan
                        && gport == h_data_p->gport
                        && level == h_data_p->level
                        && 1 == h_data_p->local_rx_enabled) {
                        match_found = 1;
                        break;
                    }
                    cur = cur->next;
                }
            }

            if (1 == match_found) {

                bcm_oam_endpoint_info_t_init(ep_info);

                /* Set up endpoint information for key construction. */
                ep_info->group = h_data_p->group_index;
                ep_info->name = h_data_p->name;
                ep_info->gport = h_data_p->gport;
                ep_info->level = h_data_p->level;
                ep_info->vlan = h_data_p->vlan;

                /*
                 * Calculate hash key for hash table insert
                 * operation.
                 */
                _bcm_oam_ep_hash_key_construct(unit, oc, ep_info, &hash_key);

                _BCM_OAM_ALLOC(sh_data_p, _bcm_oam_hash_data_t,
                               sizeof(_bcm_oam_hash_data_t), "Hash data");

                if (NULL == sh_data_p) {
                    goto cleanup;
                }

                /*
                 * Delete insert done by Local Rx endpoint recovery code.
                 * Endpoint name has been recovered and will result
                 * in a different hash index.
                 */
                rv = shr_htb_find(oc->ma_mep_htbl, hash_key,
                                  (shr_htb_data_t *)sh_data_p, 1);
                if (BCM_E_NOT_FOUND == rv) {
                    goto cleanup;
                }

                sal_free(sh_data_p);

            } else {

                /* Allocate the next available endpoint index. */
                rv = shr_idxres_list_alloc(oc->mep_pool,
                                           (shr_idxres_element_t *)&ep_id);
                if (BCM_FAILURE(rv)) {
                    OAM_ERR(("OAM(unit %d) Error: Endpoint alloc (EP=%d)"
                            " - %s.\n", unit, ep_id, bcm_errmsg(rv)));
                    goto cleanup;
                }

                h_data_p = &oc->oam_hash_data[ep_id];
                if (NULL == h_data_p) {
                    rv = BCM_E_INTERNAL;
                    goto cleanup;
                }

                _BCM_OAM_HASH_DATA_CLEAR(h_data_p);

                _BCM_OAM_HASH_DATA_HW_IDX_INIT(h_data_p);

                rv = _bcm_oam_group_ep_list_add(unit, grp_idx, ep_id);
                if (BCM_FAILURE(rv)) {
                    OAM_ERR(("OAM(unit %d) Error: Adding (EP=%d)"
                            " to (GID=%d) failed - %s.\n",
                            unit, ep_id, grp_idx, bcm_errmsg(rv)));
                    goto cleanup;
                }

                h_data_p->ep_id = ep_id;
                h_data_p->group_index = grp_idx;
                h_data_p->local_rx_enabled = 0;
                h_data_p->vlan = vlan;
                h_data_p->sglp = glp;
                h_data_p->dglp = glp;
                h_data_p->gport = gport;
                h_data_p->level = level;
            }

            h_data_p->is_remote = 0;
            h_data_p->local_tx_enabled = 1;
            h_data_p->local_tx_index = index;
            h_data_p->name
                = soc_LMEPm_field32_get(unit, &lmep_entry, MEPIDf);

            rv = shr_idxres_list_reserve(oc->lmep_pool,
                                         h_data_p->local_tx_index,
                                         h_data_p->local_tx_index);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Tx index=%d reserve failed"
                        " - %s.\n", unit, index, bcm_errmsg(rv)));

                _bcm_oam_group_ep_list_remove(unit,
                                              h_data_p->group_index,
                                              h_data_p->ep_id);

                rv = (rv == BCM_E_RESOURCE) ? (BCM_E_EXISTS) : rv;
                goto cleanup;
            }

            bcm_oam_endpoint_info_t_init(ep_info);

            /* Set up endpoint information for key construction. */
            ep_info->group = h_data_p->group_index;
            ep_info->name = h_data_p->name;
            ep_info->gport = h_data_p->gport;
            ep_info->level = h_data_p->level;
            ep_info->vlan = h_data_p->vlan;

            /*
             * Calculate hash key for hash table insert
             * operation.
             */
            _bcm_oam_ep_hash_key_construct(unit, oc, ep_info, &hash_key);

            rv = shr_htb_insert(oc->ma_mep_htbl, hash_key, h_data_p);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Hash table insert"
                        " (EP=%d) failed - %s.\n", unit,
                        h_data_p->ep_id, bcm_errmsg(rv)));

                _bcm_oam_group_ep_list_remove(unit,
                                              h_data_p->group_index,
                                              h_data_p->ep_id);

                goto cleanup;
            } else {
                OAM_VVERB(("OAM(unit %d) Info: Hash Tbl (EP=%d)"
                          " inserted  - %s.\n", unit, h_data_p->ep_id,
                          bcm_errmsg(rv)));
            }

        }

        match_found = 0;
        h_data_p = NULL;
    }

    OAM_VVERB(("OAM(unit %d) Info: _bcm_tr3_oam_lmep_tx_config_recover"
              " - done.\n", unit));
    sal_free(ep_info);
    return (rv);

cleanup:

    if (NULL != sh_data_p) {
        sal_free(sh_data_p);
    }

    if (NULL != ep_info) {
        sal_free(ep_info);
    }

    if (0 == match_found && NULL != h_data_p) {
        /* Return endpoint index to MEP pool. */
        if (BCM_E_EXISTS
            == shr_idxres_list_elem_state(oc->mep_pool, ep_id)) {
            shr_idxres_list_free(oc->mep_pool, ep_id);
        }
        _BCM_OAM_HASH_DATA_CLEAR(h_data_p);
    }

    if (BCM_E_EXISTS
        == shr_idxres_list_elem_state(oc->lmep_pool, index)) {
        shr_idxres_list_free(oc->lmep_pool, index);
    }

    OAM_VVERB(("OAM(unit %d) Info: _bcm_tr3_oam_lmep_tx_config_recover"
              " - error_done.\n", unit));
    return (rv);
}

/*
 * Function:
 *     _bcm_tr3_oam_wb_endpoints_recover
 * Purpose:
 *     Recover OAM local endpoint Rx configuration.
 * Parameters:
 *     unit        - Device unit number.
 *     stable_size - OAM module Level2 memory size.
 *     oam_scache  - Pointer to secondary storage buffer pointer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_wb_endpoints_recover(int unit,
                                  int stable_size,
                                  uint8 **oam_scache)
{
    int                     index;        /* Hardware index.                  */
    uint32                  l3_ent_count; /* Max entries in L3_ENTRY_1 table. */
    uint32                  l3_entry[SOC_MAX_MEM_WORDS];   /* L3 table entry. */
    _bcm_oam_control_t      *oc;          /* Pointer to control structure.    */
    int                     rv;           /* Operation return status.         */
    soc_mem_t               mem;          /* L3 entry memory                  */

    /*
     * Get OAM control structure.
     *     Note: Lock taken by calling routine.
     */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    if (SOC_IS_HURRICANE2(unit)) {
        mem = L3_ENTRY_IPV4_UNICASTm;
        /* Get number of L3 table entries. */
        l3_ent_count = soc_mem_index_count(unit, L3_ENTRY_IPV4_UNICASTm);
    } else {
        mem = L3_ENTRY_1m;
        /* Get number of L3 table entries. */
        l3_ent_count = soc_mem_index_count(unit, L3_ENTRY_1m);
    }

    sal_memset(&l3_entry, 0, sizeof(l3_entry));

    /* Now get valid OAM endpoint entries. */
    for (index = 0; index < l3_ent_count; index++) {

        if (SOC_IS_HURRICANE2(unit)) {
            rv = READ_L3_ENTRY_IPV4_UNICASTm(unit, MEM_BLOCK_ANY, 
                                             index, &l3_entry);
        } else {
            rv = READ_L3_ENTRY_1m(unit, MEM_BLOCK_ANY, index, &l3_entry);
        }
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: L3_ENTRY (index=%d) read"
                    " failed  - %s.\n", unit, index, bcm_errmsg(rv)));
                return (rv);
        }

#ifdef BCM_HURRICANE2_SUPPORT
        if (SOC_IS_HURRICANE2(unit)) {
            if (soc_mem_field32_get(unit, mem, &l3_entry, VALIDf)) {
                switch (soc_mem_field32_get(unit, mem, &l3_entry, KEY_TYPEf)) {

                    case SOC_MEM_KEY_L3_ENTRY_RMEP:
                        rv = _bcm_tr3_oam_rmep_recover(unit, index, l3_entry);
                        if (BCM_FAILURE(rv)) {
                            OAM_ERR(("OAM(unit %d) Error: Remote endpoint"
                                    " (index=%d) reconstruct failed  - %s.\n",
                                    unit, index, bcm_errmsg(rv)));
                            return (rv);
                        }
                        break;

                    case SOC_MEM_KEY_L3_ENTRY_LMEP:
                        rv = _bcm_tr3_oam_lmep_rx_config_recover(unit,
                                                             index,
                                                             l3_entry);
                        if (BCM_FAILURE(rv)) {
                            OAM_ERR(("OAM(unit %d) Error: Local endpoint"
                                    " (index=%d) reconstruct failed  - %s.\n",
                                    unit, index, bcm_errmsg(rv)));
                            return (rv);
                        }
                        break;

                    default:
                        /* Not an OAM entry. */
                        continue;
                }
            }
        }  
#endif /* BCM_HURRICANE2_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_mem_field32_get(unit, mem, &l3_entry, VALIDf)) {
            switch (soc_mem_field32_get(unit, mem, &l3_entry, KEY_TYPEf)) {

                case SOC_MEM_KEY_L3_ENTRY_1_RMEP_RMEP:
                    rv = _bcm_tr3_oam_rmep_recover(unit, index, l3_entry);
                    if (BCM_FAILURE(rv)) {
                        OAM_ERR(("OAM(unit %d) Error: Remote endpoint"
                                    " (index=%d) reconstruct failed  - %s.\n",
                                    unit, index, bcm_errmsg(rv)));
                        return (rv);
                    }
                    break;

                case SOC_MEM_KEY_L3_ENTRY_1_LMEP_LMEP:
                    rv = _bcm_tr3_oam_lmep_rx_config_recover(unit,
                                                             index,
                                                             l3_entry);
                    if (BCM_FAILURE(rv)) {
                        OAM_ERR(("OAM(unit %d) Error: Local endpoint"
                                    " (index=%d) reconstruct failed  - %s.\n",
                                    unit, index, bcm_errmsg(rv)));
                        return (rv);
                    }
                    break;

                default:
                    /* Not an OAM entry. */
                    continue;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
    }
    rv = _bcm_tr3_oam_lmep_tx_config_recover(unit);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Endpoint Tx config recovery"
                " failed  - %s.\n", unit, bcm_errmsg(rv)));
        return (rv);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     _bcm_tr3_oam_reinit
 * Purpose:
 *     Reconstruct OAM module software state.
 * Parameters:
 *     unit - (IN) BCM device number
 * Retruns:
 *     BCM_E_XXX
 */
STATIC int
_bcm_tr3_oam_reinit(int unit)
{
    uint32              group_count;   /* Stored OAM group count.           */
    int                 stable_size;   /* Secondary storage size.           */
    uint8               *oam_scache;   /* Pointer to scache memory.         */
    soc_scache_handle_t scache_handle; /* Scache memory handler.            */
    _bcm_oam_control_t  *oc;           /* Pointer to OAM control structure. */
    int                 rv;            /* Operation return status.          */


    OAM_VVERB(("OAM(unit %d) Info: OAM warm boot recovery.....\n", unit));

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    /* Get OAM control structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    if (!SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit) && (stable_size > 0)) {

        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_OAM, 0);

        rv = _bcm_esw_scache_ptr_get(unit, scache_handle, 0, 0,
                                     &oam_scache, BCM_WB_DEFAULT_VERSION,
                                     NULL);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }

        /* Recover the FP groups */
        sal_memcpy(&oc->vfp_group, oam_scache, sizeof(bcm_field_group_t));
        oam_scache += sizeof(bcm_field_group_t);

        sal_memcpy(&oc->fp_vp_group, oam_scache, sizeof(bcm_field_group_t));
        oam_scache += sizeof(bcm_field_group_t);

        sal_memcpy(&oc->fp_glp_group, oam_scache, sizeof(bcm_field_group_t));
        oam_scache += sizeof(bcm_field_group_t);

        /* Recover the OAM groups */
        sal_memcpy(&group_count, oam_scache, sizeof(int));
        oam_scache += sizeof(int);
    }

    rv = _bcm_tr3_oam_wb_group_recover(unit, stable_size, &oam_scache);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Group recovery failed  - %s.\n",
                unit, bcm_errmsg(rv)));
        goto cleanup;
    }

    rv = _bcm_tr3_oam_wb_endpoints_recover(unit, stable_size, &oam_scache);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Endpoint recovery failed  - %s.\n",
                unit, bcm_errmsg(rv)));
        goto cleanup;
    }
    
cleanup:
    _BCM_OAM_UNLOCK(oc);
    return (rv);
}
#endif


/* * * * * * * * * * * * * * * * * * * *
 *            OAM BCM APIs             *
 * * * * * * * * * * * * * * * * * * * */

/*
 * Function: bcm_tr3_oam_init
 *
 * Purpose:
 *     Initialize OAM module.
 * Parameters:
 *     unit - (IN) BCM device number
 * Returns:
 *     BCM_E_UNIT    - Invalid BCM unit number.
 *     BCM_E_UNAVAIL - OAM not support on this device.
 *     BCM_E_MEMORY  - Allocation failure
 *     CM_E_XXX     - Error code from bcm_XX_oam_init()
 *     BCM_E_NONE    - Success
 */
int
bcm_tr3_oam_init(int unit)
{
    _bcm_oam_control_t *oc = NULL;  /* OAM control structure.     */
    int             rv;             /* Operation return value.    */
    uint32          size;           /* Size of memory allocation. */
    bcm_port_t      port;           /* Port number.               */
    bcm_oam_endpoint_t   ep_index;  /* Endpoint index.               */
#if defined(INCLUDE_BHH)
    int uc;
    uint8 carrier_code[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    char *carrier_code_str;
    uint32 node_id = 0;
    shr_bhh_msg_ctrl_init_t msg_init;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int priority;
#endif


    /* Ensure that the unit has OAM support. */
    if (!soc_feature(unit, soc_feature_oam)) {
        OAM_ERR(("OAM(unit %d) Error: OAM not supported \n", unit));
        return (BCM_E_UNAVAIL);
    }

    if (soc_feature(unit, soc_feature_bhh)) {
#ifdef INCLUDE_BHH
        
        if(SOC_E_NONE != soc_cmic_uc_msg_active_wait(unit, 0))
        {
            soc_cm_debug(DK_ERR, "uKernel Not Ready, bhh not started\n");
            return (BCM_E_NONE);
        }
#endif
    }

    /* Detach first if the module has been previously initialized. */
    if (NULL != _oam_control[unit]) {
        _oam_control[unit]->init = FALSE;

        /* COVERITY : Intentional stack use. Marking the event as intentional */
        /* coverity[stack_use_callee] */
        /* coverity[stack_use_overflow] */
        rv = bcm_tr3_oam_detach(unit);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Module deinit - %s.\n",
                    unit, bcm_errmsg(rv)));
            return (rv);
        }
    }

    /* Allocate OAM control memeory for this unit. */
    _BCM_OAM_ALLOC(oc, _bcm_oam_control_t, sizeof (_bcm_oam_control_t),
               "OAM control");
    if (NULL == oc) {
        return (BCM_E_MEMORY);
    }

    /* Get number of endpoints and groups supported by this unit. */
    rv = _bcm_oam_group_endpoint_count_init(unit, oc);
    if (BCM_FAILURE(rv)) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (rv);
    }

    if (soc_feature(unit, soc_feature_bhh)) {
#if defined(INCLUDE_BHH)
    oc->unit = unit;

    /* Get SOC properties for BHH */
    oc->bhh_endpoint_count = soc_property_get(unit, spn_BHH_NUM_SESSIONS, 128);

    carrier_code_str = soc_property_get_str(unit, spn_BHH_CARRIER_CODE);
    if (carrier_code_str != NULL) {
        /*
         * Note that the carrier code is specified in colon separated
         * MAC address format.
         */
        if (_shr_parse_macaddr(carrier_code_str, carrier_code) < 0) {
            _bcm_tr3_oam_control_free(unit, oc);
            return (BCM_E_INTERNAL);
        }
    }       

    node_id = soc_property_get(unit, spn_BHH_NODE_ID, 0);

    oc->ep_count += oc->bhh_endpoint_count;

    if(oc->ep_count >= _BCM_OAM_TRIUMPH3_ENDPOINT_MAX) {
        OAM_ERR(("OAM(unit %d) Error: OAM EP count %d not supported \n", 
                                                        unit, oc->ep_count));
        _bcm_tr3_oam_control_free(unit, oc);
        return (BCM_E_PARAM);
    }
#endif /* INCLUDE_BHH */
    }

    /* Mem_1: Allocate hash data memory */
    /* size = sizeof(_bcm_oam_hash_data_t) * oc->ep_count; */
    if (SOC_IS_HURRICANE2(unit)) {
        size = sizeof(_bcm_oam_hash_data_t) * _BCM_OAM_HURRICANE2_ENDPOINT_MAX;
    } else {
        size = sizeof(_bcm_oam_hash_data_t) * _BCM_OAM_TRIUMPH3_ENDPOINT_MAX;
    }
    _BCM_OAM_ALLOC(oc->oam_hash_data, _bcm_oam_hash_data_t, size, "Hash data");
    if (NULL == oc->oam_hash_data) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (BCM_E_MEMORY);
    }

    /* Mem_2: Allocate group memory */
    size = sizeof(_bcm_oam_group_data_t) * oc->group_count;

    _BCM_OAM_ALLOC(oc->group_info, _bcm_oam_group_data_t, size, "Group Info");

    if (NULL == oc->group_info) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (BCM_E_MEMORY);
    }

    /* Mem 3: Allocate RMEP H/w to logical index mapping memory */
    size = sizeof(bcm_oam_endpoint_t) * oc->rmep_count;
    _BCM_OAM_ALLOC(oc->remote_endpoints, bcm_oam_endpoint_t, size, "RMEP Mapping");
    if (NULL == oc->remote_endpoints) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (BCM_E_MEMORY);
    }
    /* Initialize the mapping to BCM_OAM_ENDPOINT_INVALID */ 
    for (ep_index = 0; ep_index < oc->rmep_count; ++ep_index) {
        oc->remote_endpoints[ep_index] = BCM_OAM_ENDPOINT_INVALID;
    }
    

    
    /* Mem_3: Create application endpoint list. */
    if (SOC_IS_HURRICANE2(unit)) {
        oc->ep_count = _BCM_OAM_HURRICANE2_ENDPOINT_MAX;
    } else {
        oc->ep_count = _BCM_OAM_TRIUMPH3_ENDPOINT_MAX;
    }

    rv = shr_idxres_list_create(&oc->mep_pool, 1, oc->ep_count - 1,
                                1, oc->ep_count -1, "endpoint pool");
    if (BCM_FAILURE(rv)) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (rv);
    }

    /* Mem_3: Create local MEP endpoint list. */
    rv = shr_idxres_list_create(&oc->lmep_pool, 1, oc->lmep_count - 1,
                                1, oc->lmep_count -1, "lmep pool");
    if (BCM_FAILURE(rv)) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (rv);
    }

    /* Mem_3: Create Remote MEP endpoint list. */
    rv = shr_idxres_list_create(&oc->rmep_pool, 1, oc->rmep_count - 1,
                                1, oc->rmep_count -1, "rmep pool");
    if (BCM_FAILURE(rv)) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (rv);
    }

    /* Mem_3: Create MEP Rx state tracker table index endpoint list. */
    rv = shr_idxres_list_create(&oc->ma_idx_pool, 8, oc->ma_idx_count - 1,
                                8, oc->ma_idx_count -1, "ma_idx pool");
    if (BCM_FAILURE(rv)) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (rv);

    }

    if(soc_feature(unit, soc_feature_bhh)) {
#if defined(INCLUDE_BHH)
    /* Mem_3: Create BHH endpoint list. */
    rv = shr_idxres_list_create(&oc->bhh_pool, 0, oc->bhh_endpoint_count - 1,
                                0, oc->bhh_endpoint_count - 1, "bhh pool");
    if (BCM_FAILURE(rv)) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (rv);
    }

    /* Reserve ep pool from mep_pool for BHH, manange BHH mep using bhh_pool */
    rv = shr_idxres_list_reserve(oc->mep_pool, _BCM_OAM_BHH_ENDPOINT_OFFSET, 
                _BCM_OAM_BHH_ENDPOINT_OFFSET + oc->bhh_endpoint_count);
                
    if (BCM_FAILURE(rv)) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (rv);
    }
#endif /* INCLUDE_BHH */
    }

    /* Mem_4: Create group list. */
    rv = shr_idxres_list_create(&oc->group_pool, 1, oc->group_count - 1,
                                1, oc->group_count - 1, "group pool");
    if (BCM_FAILURE(rv)) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (rv);
    }

    
    /* Mem_5: Create MA group and MEP hash table. */
    if (SOC_IS_HURRICANE2(unit)) {
        rv = shr_htb_create(&oc->ma_mep_htbl, oc->ep_count,
                            sizeof(_bcm_oam_hash_key_t), "MA/MEP Hash");
    } else {
        rv = shr_htb_create(&oc->ma_mep_htbl, _BCM_OAM_TRIUMPH3_ENDPOINT_MAX,
                            sizeof(_bcm_oam_hash_key_t), "MA/MEP Hash");
    }
    if (BCM_FAILURE(rv)) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (rv);
    }

    /* 
     * Both TX and RX for one MEP 
     */
    oc->lm_counter_cnt = 
        soc_mem_index_count(unit, OAM_LM_COUNTERSm) / 2;

    rv = shr_idxres_list_create(&oc->lm_counter_pool, 0, oc->lm_counter_cnt - 1,
                                0, oc->lm_counter_cnt - 1, "lm counter pool");
    if (BCM_FAILURE(rv)) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (rv);
    }

    for (ep_index = 0; ep_index < oc->ep_count; ++ep_index) {
        oc->oam_hash_data[ep_index].lm_counter_index =
                                    _BCM_OAM_LM_COUNTER_IDX_INVALID;
    }

    /* Create protection mutex. */
    oc->oc_lock = sal_mutex_create("oam_control.lock");
    if (NULL == oc->oc_lock) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (BCM_E_MEMORY);
    }
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        /* Register device OAM interrupt handler call back routine. */
        soc_hurricane2_oam_handler_register(unit, 
                                       _bcm_tr3_oam_handle_interrupt);
    } else
#endif
    { 
#ifdef BCM_TRIUMPH3_SUPPORT
        /* Register device OAM interrupt handler call back routine. */
        soc_tr3_oam_handler_register(unit, _bcm_tr3_oam_handle_interrupt);
        soc_tr3_oam_ser_handler_register(unit, _bcm_tr3_oam_ser_handler);
#endif

    }
    /* Set up the unit OAM control structure. */
    _oam_control[unit] = oc;

#if defined(BCM_WARM_BOOT_SUPPORT)
    if (SOC_WARM_BOOT(unit)) {
        rv = _bcm_tr3_oam_reinit(unit);
        if (BCM_FAILURE(rv)) {
            _bcm_tr3_oam_control_free(unit, oc);
            return (rv);
        }
    } else 
#endif
    {
        /* Enable OAM processing on all ports of this unit. */
        PBMP_ALL_ITER(unit, port) {
            rv = bcm_esw_port_control_set(unit, port, bcmPortControlOAMEnable,
                                          TRUE);
            if (BCM_FAILURE(rv)) {
                _bcm_tr3_oam_control_free(unit, oc);
                return (rv);
            }
        }

        /* Enable CCM Rx timeouts. */
#ifdef BCM_HURRICANE2_SUPPORT
        if (SOC_IS_HURRICANE2(unit)) {
            rv = _bcm_hu2_oam_ccm_rx_timeout_set(unit, 1);
        } else 
#endif /* BCM_HURRICANE2_SUPPORT */
        {
            rv = _bcm_oam_ccm_rx_timeout_set(unit, 1);
        }
        if (BCM_FAILURE(rv)) {
            _bcm_tr3_oam_control_free(unit, oc);
            return (rv);
        }

        /* Enable CCM Tx control. */
        if (!SOC_IS_HURRICANE2(unit)) {   
            rv = _bcm_oam_ccm_tx_config_set(unit, 1);
            if (BCM_FAILURE(rv)) {
                _bcm_tr3_oam_control_free(unit, oc);
                return (rv);
            }
        }

        /* Set up OAM module related profile tables. */
        rv = _bcm_oam_profile_tables_init(unit, oc);
        if (BCM_FAILURE(rv)) {
            _bcm_tr3_oam_control_free(unit, oc);
            return (rv);
        }
        
        /* Initailiza the FP group for LM/DM to -1 (Not created). */
        oc->fp_glp_group = -1;
        
        /* 
         * Misc config:
         * 1. Prevent L2 learn for OAM MACs.
         * 2. PFM setting for MCAST packets.
         */
        rv = _bcm_oam_misc_config(unit);
        if (BCM_FAILURE(rv)) {
            _bcm_tr3_oam_control_free(unit, oc);
            return (rv);
        }
    }

    /*
     * BHH init
     */
    if(soc_feature(unit, soc_feature_bhh)) {
#if defined(INCLUDE_BHH)
    /*
     * Initialize HOST side
     */
    oc->cpu_cosq = soc_property_get(unit, spn_BHH_COSQ, BHH_COSQ_INVALID);

    /*
     * Allocate DMA buffers
     *
     * DMA buffer will be used to send and receive 'long' messages
     * between SDK Host and uController (BTE).
     */
    oc->dma_buffer_len = sizeof(shr_bhh_msg_ctrl_t);
    oc->dma_buffer = soc_cm_salloc(unit, oc->dma_buffer_len,
                           "BHH DMA buffer");
    if (!oc->dma_buffer) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (BCM_E_MEMORY);
    }
    sal_memset(oc->dma_buffer, 0, oc->dma_buffer_len);

    oc->dmabuf_reply = soc_cm_salloc(unit, sizeof(shr_bhh_msg_ctrl_t),
                         "BHH uC reply");
    if (!oc->dmabuf_reply) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (BCM_E_MEMORY);
    }
    sal_memset(oc->dmabuf_reply, 0, sizeof(shr_bhh_msg_ctrl_t));


    /*
     * Initialize uController side
     */

    /*
     * Start BHH application in BTE (Broadcom Task Engine) uController.
     * Determine which uController is running BHH  by choosing the first
     * uC that returns successfully.
     */
    for (uc = 0; uc < CMICM_NUM_UCS; uc++) {
        rv = soc_cmic_uc_appl_init(unit, uc, MOS_MSG_CLASS_BHH,
                       _BHH_UC_MSG_TIMEOUT_USECS,
                       BHH_SDK_VERSION,
                       BHH_UC_MIN_VERSION);
        if (SOC_E_NONE == rv) {
            /* BHH started successfully */
            oc->uc_num = uc;
            break;
        }
    }

    if (uc >= CMICM_NUM_UCS) {  /* Could not find or start BHH appl */
        _bcm_tr3_oam_control_free(unit, oc);
        return (BCM_E_INTERNAL);
    }

    /* RX DMA channel (0..3) local to the uC */
    oc->rx_channel = BCM_KT_BHH_RX_CHANNEL;


    /* Set control message data */
    sal_memset(&msg_init, 0, sizeof(msg_init));
    msg_init.num_sessions       = oc->bhh_endpoint_count;
    msg_init.rx_channel         = oc->rx_channel;
    msg_init.node_id            = node_id; 
    sal_memcpy(msg_init.carrier_code, carrier_code, SHR_BHH_CARRIER_CODE_LEN); 

    /* Pack control message data into DMA buffer */
    buffer     = oc->dma_buffer;
    buffer_ptr = shr_bhh_msg_ctrl_init_pack(buffer, &msg_init);
    buffer_len = buffer_ptr - buffer;

    /* Send BHH Init message to uC */
    rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                          MOS_MSG_SUBCLASS_BHH_INIT,
                          buffer_len, 0,
                          MOS_MSG_SUBCLASS_BHH_INIT_REPLY,
                          &reply_len);

    if (BCM_FAILURE(rv) || (reply_len != 0)) {
        _bcm_tr3_oam_control_free(unit, oc);
        return (BCM_E_INTERNAL);
    }

    /*
     * Initialize HW
     */
    rv = _bcm_tr3_oam_bhh_hw_init(unit);
    if (BCM_FAILURE(rv)) {
        BCM_IF_ERROR_RETURN(bcm_tr3_oam_detach(unit));
        return (rv);
    }


    /*
     * Start event message callback thread
     */
    priority = BHH_THREAD_PRI_DFLT;

    if (oc->event_thread_id == NULL) {
        if (sal_thread_create("bcmBHH", SAL_THREAD_STKSZ, 
                  priority,
                  _bcm_tr3_oam_bhh_callback_thread,
                  (void*)oc) == SAL_THREAD_ERROR) {
            BCM_IF_ERROR_RETURN(bcm_tr3_oam_detach(unit));
            return (BCM_E_MEMORY);
        }
    }
    oc->event_thread_kill = 0;

    /*
     * End BHH init
     */
#endif
    }

    /* OAM initialization complete. */
    _oam_control[unit]->init = TRUE;
    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcm_tr3_oam_detach
 * Purpose:
 *     Shut down OAM subsystem
 * Parameters:
 *     unit - (IN) BCM device number
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_oam_detach(int unit)
{
    _bcm_oam_control_t *oc;  /* Pointer to OAM control structure. */
    bcm_port_t         port; /* Port number.                      */
    int                rv;   /* Operation return status.          */
#if defined(INCLUDE_BHH)
    uint16 reply_len;
    sal_usecs_t timeout = 0;
#endif


    /* Get the device OAM module handle. */
    oc = _oam_control[unit];
    if (NULL == oc) {
        /* Module already uninitialized. */
        return (BCM_E_NONE);
    }

    /* Unregister all OAM interrupt event handlers. */
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        /* Register device OAM interrupt handler call back routine. */
        soc_hurricane2_oam_handler_register(unit, NULL);
    } else
#endif
    {
#ifdef BCM_TRIUMPH3_SUPPORT
        soc_tr3_oam_handler_register(unit, NULL);
        soc_tr3_oam_ser_handler_register(unit, NULL);
#endif 
    }
    if (NULL != oc->oc_lock) {
        _BCM_OAM_LOCK(oc);
    }

    rv = _bcm_tr3_oam_events_unregister(unit, oc);
    if (BCM_FAILURE(rv)) {
        if (NULL != oc->oc_lock) {
            _BCM_OAM_UNLOCK(oc);
        }
        return (rv);
    }

    /* Disable CCM Rx Timeouts. */
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        rv = _bcm_hu2_oam_ccm_rx_timeout_set(unit, 1);
    } else 
#endif /* BCM_HURRICANE2_SUPPORT */
    {
        rv = _bcm_oam_ccm_rx_timeout_set(unit, 0);
    }
    if (BCM_FAILURE(rv)) {
        if (NULL != oc->oc_lock) {
            _BCM_OAM_UNLOCK(oc);
        }
        return (rv);
    }

    /* Disable CCM Tx control for the device. */
    if (!SOC_IS_HURRICANE2(unit)) {   
        rv = _bcm_oam_ccm_tx_config_set(unit, 0);
        if (BCM_FAILURE(rv)) {
            if (NULL != oc->oc_lock) {
                _BCM_OAM_UNLOCK(oc);
            }
            return (rv);
        }
    }

    /* Disable OAM PDU Rx processing on all ports. */
    PBMP_ALL_ITER(unit, port) {
        rv = bcm_esw_port_control_set(unit, port,
                                      bcmPortControlOAMEnable, 0);
        if (BCM_FAILURE(rv)) {
            if (NULL != oc->oc_lock) {
                _BCM_OAM_UNLOCK(oc);
            }
            return (rv);
        }
    }

    /*
     * BHH specific 
     */
    if (soc_feature(unit, soc_feature_bhh)) {
#if defined(INCLUDE_BHH)
    /* 
     * Event Handler thread exit signal 
     */
    if ( oc->event_thread_id ) {
        oc->event_thread_kill = 1;
        soc_cmic_uc_msg_receive_cancel(unit, oc->uc_num,
                        MOS_MSG_CLASS_BHH_EVENT);
    
        /*Ensure here itself that the callback thread exists as we are going
         *to free _bcm_oam_control_t below and we will loose reference to the thread*/
        timeout = sal_time_usecs() + 1000000;
        while (oc->event_thread_id != NULL) {
            if (sal_time_usecs() < timeout) {
                /*give some time to already running bfd thread to schedule and exit*/
                sal_usleep(1000);
            } else {
                /*timeout*/
                soc_cm_debug(DK_ERR, "BHH event thread did not exit.\n");
                return BCM_E_INTERNAL;
            }
        }
    }
    
    /*
     * Send BHH Uninit message to uC
     * Ignore error since that may indicate uKernel was reloaded.
     */
    rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                      MOS_MSG_SUBCLASS_BHH_UNINIT,
                      0, 0,
                      MOS_MSG_SUBCLASS_BHH_UNINIT_REPLY,
                      &reply_len);
    if (BCM_SUCCESS(rv) && (reply_len != 0)) {
        if (NULL != oc->oc_lock) {
            _BCM_OAM_UNLOCK(oc);
        }
        return BCM_E_INTERNAL;
    }

    /* 
     * Delete CPU COS queue mapping entries for BHH packets 
     */
    if (oc->cpu_cosq_ach_error_index >= 0) {
        rv = bcm_esw_rx_cosq_mapping_delete(unit,
                        oc->cpu_cosq_ach_error_index);
        if (BCM_FAILURE(rv)) {
            if (NULL != oc->oc_lock) {
                _BCM_OAM_UNLOCK(oc);
            }
            return (rv);
        }
        oc->cpu_cosq_ach_error_index = -1;
    }
    if (oc->cpu_cosq_invalid_error_index >= 0) {
        rv = bcm_esw_rx_cosq_mapping_delete(unit,
                        oc->cpu_cosq_invalid_error_index);
        if (BCM_FAILURE(rv)) {
            if (NULL != oc->oc_lock) {
                _BCM_OAM_UNLOCK(oc);
            }
            return (rv);
        }
        oc->cpu_cosq_invalid_error_index = -1;
    }

    BCM_IF_ERROR_RETURN(bcm_esw_field_data_qualifier_destroy(unit, 
                                                  oc->bhh_qual_opcode.qual_id));
    BCM_IF_ERROR_RETURN(bcm_esw_field_data_qualifier_destroy(unit, oc->bhh_qual_ach.qual_id));
    BCM_IF_ERROR_RETURN(bcm_esw_field_data_qualifier_destroy(unit, oc->bhh_qual_label.qual_id));
#endif /* INCLUDE_BHH */
    }

    /* Destroy all groups and assoicated endpoints and free the resources. */
    rv = bcm_tr3_oam_group_destroy_all(unit);
    if (BCM_FAILURE(rv)) {
        if (NULL != oc->oc_lock) {
            _BCM_OAM_UNLOCK(oc);
        }
        return (rv);
    }

    /* Release the protection mutex. */
    _BCM_OAM_UNLOCK(oc);

    /* Free OAM module allocated resources. */
    _bcm_tr3_oam_control_free(unit, oc);

    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcm_tr3_oam_group_create
 * Purpose:
 *     Create or replace an OAM group object
 * Parameters:
 *     unit       - (IN) BCM device number
 *     group_info - (IN/OUT) Pointer to an OAM group information.
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_oam_group_create(int unit, bcm_oam_group_info_t *group_info)
{
    ma_state_entry_t       ma_state_entry;       /* MA State table entry.     */
    uint8                  grp_name_hw_buf[BCM_OAM_GROUP_NAME_LENGTH]; /* Grp */
                                                 /* name.                     */
    maid_reduction_entry_t maid_reduction_entry; /* MA ID reduction table     */
    _bcm_oam_group_data_t  *ma_group;            /* Pointer to group info.    */
    _bcm_oam_control_t     *oc;                  /* OAM control structure.    */
    int                    rv;                   /* Operation return status.  */
                                                 /* entry.                    */
    uint8                  sw_rdi;               /* Remote defect indicator.  */

    /* Validate input parameter. */
    if (NULL == group_info) {
        return (BCM_E_PARAM);
    }

    /* Get OAM control structure handle. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Validate group id. */
    if (group_info->flags & BCM_OAM_GROUP_WITH_ID) {
        _BCM_OAM_GROUP_INDEX_VALIDATE(group_info->id);
    }

    _BCM_OAM_LOCK(oc);

    /*
     * If MA group create is called with replace flag bit set.
     *  - Check and return error if a group does not exist with the ID.
     */
    if (group_info->flags & BCM_OAM_GROUP_REPLACE) {
        if (group_info->flags & BCM_OAM_GROUP_WITH_ID) {

            /* Search the list with the MA Group ID value. */
            rv = shr_idxres_list_elem_state(oc->group_pool, group_info->id);
            if (BCM_E_EXISTS != rv) {
                _BCM_OAM_UNLOCK(oc);
                OAM_ERR(("OAM(unit %d) Error: Group does not exist.\n", unit));
                return (BCM_E_PARAM);
            }
        } else {
            _BCM_OAM_UNLOCK(oc);
            OAM_ERR(("OAM(unit %d) Error: Replace command needs a valid Group ID.\n",
                    unit));
            return (BCM_E_PARAM);
        }
    } else if (group_info->flags & BCM_OAM_GROUP_WITH_ID) {
        /*
         * If MA group create is called with ID flag bit set.
         *  - Check and return error if the ID is already in use.
         */
        rv = shr_idxres_list_reserve(oc->group_pool, group_info->id,
                                     group_info->id);
        if (BCM_FAILURE(rv)) {
            _BCM_OAM_UNLOCK(oc);
            return ((rv == BCM_E_RESOURCE) ? (BCM_E_EXISTS) : rv);
        }
    } else {
        /* Reserve the next available group index. */
         rv = shr_idxres_list_alloc(oc->group_pool,
                                    (shr_idxres_element_t *) &group_info->id);
        if (BCM_FAILURE(rv)) {
            _BCM_OAM_UNLOCK(oc);
            OAM_ERR(("OAM(unit %d) Error: Group allocation (GID=%d)"
                     " %s\n", unit, group_info->id, bcm_errmsg(rv)));
            return (rv);
        }
    }

    /* Get this group memory to store group information. */
    ma_group = &oc->group_info[group_info->id];

    /* Store the group name. */
    sal_memcpy(&ma_group->name, &group_info->name, BCM_OAM_GROUP_NAME_LENGTH);
    
    /* Store Lowest Alarm Priority */
    ma_group->lowest_alarm_priority = group_info->lowest_alarm_priority;

    if(!(group_info->flags & BCM_OAM_GROUP_REPLACE)) {
        _BCM_OAM_ALLOC((ma_group->ep_list),_bcm_oam_ep_list_t *,
               sizeof(_bcm_oam_ep_list_t *), "EP list head");

        /* Initialize head to NULL.*/
        *ma_group->ep_list =  NULL;
    }

    /*
     * Maintenance Association ID - Reduction table update.
     */

    /* Prepare group name for hardware write. */
    _bcm_tr3_oam_group_name_mangle(ma_group->name, grp_name_hw_buf);

    sal_memset(&maid_reduction_entry, 0, sizeof(maid_reduction_entry_t));

    /* Calculate CRC32 value for the group name string and set in entry. */
    soc_MAID_REDUCTIONm_field32_set
        (unit, &maid_reduction_entry, REDUCED_MAIDf,
         soc_draco_crc32(grp_name_hw_buf, BCM_OAM_GROUP_NAME_LENGTH));

    /* Check if software RDI flag bit needs to be set in hardware. */
    sw_rdi = ((group_info->flags & BCM_OAM_GROUP_REMOTE_DEFECT_TX) ? 1 : 0);

    /* Set RDI status for out going CCM PDUs. */
    soc_MAID_REDUCTIONm_field32_set(unit, &maid_reduction_entry, SW_RDIf,
                                    sw_rdi);

    /* Enable hardware lookup for this entry. */
    soc_MAID_REDUCTIONm_field32_set(unit, &maid_reduction_entry, VALIDf, 1);


    /* Write entry to hardware. */
    SOC_IF_ERROR_RETURN(WRITE_MAID_REDUCTIONm(unit, MEM_BLOCK_ALL,
                                              group_info->id,
                                              &maid_reduction_entry));

    /*
     * Maintenance Association State table update.
     */
    sal_memset(&ma_state_entry, 0, sizeof(ma_state_entry_t));

    /*
     * If it a group info replace operation, retain previous group
     * defect status.
     */
    if (group_info->flags & BCM_OAM_GROUP_REPLACE) {
        SOC_IF_ERROR_RETURN(READ_MA_STATEm(unit, MEM_BLOCK_ALL,
                                           group_info->id,
                                           &ma_state_entry));
    }

    /* Set the lowest alarm priority info. */
    soc_MA_STATEm_field32_set(unit, &ma_state_entry, LOWESTALARMPRIf,
                              group_info->lowest_alarm_priority);

    /* Mark the entry as valid. */
    soc_MA_STATEm_field32_set(unit, &ma_state_entry, VALIDf, 1);

    /* Write group information to hardware table. */
    SOC_IF_ERROR_RETURN(WRITE_MA_STATEm(unit, MEM_BLOCK_ALL, group_info->id,
                                        &ma_state_entry));

    /* Make the group as in used status. */
    ma_group->in_use = 1;

    _BCM_OAM_UNLOCK(oc);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcm_tr3_oam_group_get
 * Purpose:
 *     Get an OAM group object
 * Parameters:
 *     unit       - (IN) BCM device number
 *     group      - (IN) OAM Group ID.
 *     group_info - (OUT) Pointer to group information buffer.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_oam_group_get(int unit, bcm_oam_group_t group,
                      bcm_oam_group_info_t *group_info)
{
    _bcm_oam_control_t    *oc;      /* Pointer to OAM control structure. */
    _bcm_oam_group_data_t *group_p; /* Pointer to group list.            */
    int                   rv;       /* Operation return status.          */

    /* Validate input parameter. */
    if (NULL == group_info) {
        return (BCM_E_PARAM);
    }

    /* Get OAM device control structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Validate group index. */
    _BCM_OAM_GROUP_INDEX_VALIDATE(group);

    _BCM_OAM_LOCK(oc);

    /* Check if the group is in use. */
    rv = shr_idxres_list_elem_state(oc->group_pool, group);
    if (BCM_E_EXISTS != rv) {
        _BCM_OAM_UNLOCK(oc);
        OAM_ERR(("OAM(unit %d) Error: GID=%d - %s.\n",
            unit, group, bcm_errmsg(rv)));
        return (rv);
    }

    /* Get pointer to this group in the group list. */
    group_p = oc->group_info;

    /* Get the group information. */
    rv = _bcm_tr3_oam_get_group(unit, group, group_p, group_info);
    if (BCM_FAILURE(rv)) {
        _BCM_OAM_UNLOCK(oc);
        OAM_ERR(("OAM(unit %d) Error: bcm_tr3_oam_group_get Group ID=%d "
                 "- Failed.\n", unit, group));
        return (rv);
    }

    _BCM_OAM_UNLOCK(oc);
    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcm_tr3_oam_group_destroy
 * Purpose:
 *     Destroy an OAM group object and its associated endpoints.
 * Parameters:
 *     unit  - (IN) BCM device number
 *     group - (IN) The ID of the OAM group object to destroy
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_oam_group_destroy(int unit,
                          bcm_oam_group_t group)
{
    _bcm_oam_control_t     *oc;       /* Pointer to OAM control structure. */
    _bcm_oam_group_data_t  *g_info_p; /* Pointer to group list.            */
    int                    rv;        /* Operation return status.          */
    maid_reduction_entry_t maid_reduction_entry; /* MAID_REDUCTION entry.  */
    ma_state_entry_t       ma_state_entry; /* MA_STATE table entry.        */

    /* Get OAM device control structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Validate group index. */
    _BCM_OAM_GROUP_INDEX_VALIDATE(group);

    _BCM_OAM_LOCK(oc);

    /* Check if the group is in use. */
    rv = shr_idxres_list_elem_state(oc->group_pool, group);
    if (BCM_E_EXISTS != rv) {
        _BCM_OAM_UNLOCK(oc);
        OAM_ERR(("OAM(unit %d) Error: GID=%d - %s.\n",
            unit, group, bcm_errmsg(rv)));
        return (rv);
    }

    /* Get pointer to this group in the group list. */
    g_info_p = &oc->group_info[group];

    rv = _bcm_tr3_oam_group_endpoints_destroy(unit, g_info_p);
    if (BCM_FAILURE(rv)) {
        _BCM_OAM_UNLOCK(oc);
        OAM_ERR(("OAM(unit %d) Error: bcm_tr3_oam_endpoint_destroy_all"
                  " (GID=%d) - %s.\n", unit, group, bcm_errmsg(rv)));
        return (rv);
    }

    sal_memset(&maid_reduction_entry, 0, sizeof(maid_reduction_entry_t));
    rv = WRITE_MAID_REDUCTIONm(unit, MEM_BLOCK_ALL, group,
                          &maid_reduction_entry);
    if (BCM_FAILURE(rv)) {
        _BCM_OAM_UNLOCK(oc);
        OAM_VVERB(("OAM(unit %d) Error: MAID REDUCTION write "
                  "(GID=%d) - %s.\n", unit, group, bcm_errmsg(rv)));
        return (rv);
    }

    /*
     * Maintenance Association State table update.
     */
    sal_memset(&ma_state_entry, 0, sizeof(ma_state_entry_t));

    rv = WRITE_MA_STATEm(unit, MEM_BLOCK_ALL, group, &ma_state_entry);
    if (BCM_FAILURE(rv)) {
        _BCM_OAM_UNLOCK(oc);
        OAM_ERR(("OAM(unit %d) Error: MA STATE write "
                  "(GID=%d) - %s.\n", unit, group, bcm_errmsg(rv)));
        return (rv);
    }

    /* Return Group ID back to free group. */
    BCM_IF_ERROR_RETURN(shr_idxres_list_free(oc->group_pool, group));

    _BCM_OAM_UNLOCK(oc);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcm_tr3_oam_group_destroy_all
 * Purpose:
 *     Destroy all OAM group objects and their associated endpoints.
 * Parameters:
 *     unit  - (IN) BCM device number
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_oam_group_destroy_all(int unit)
{
    _bcm_oam_control_t *oc;   /* Pointer to OAM control structure.    */
    int                group; /* Maintenance Association group index. */
    int                rv;    /* Operation return status .            */

    /* Get device OAM control structure handle. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    for (group = 0; group < oc->group_count; group++) {

        /* Check if the group is in use. */
        rv = shr_idxres_list_elem_state(oc->group_pool, group);
        if (BCM_E_EXISTS != rv) {
            continue;
        }

        rv = bcm_tr3_oam_group_destroy(unit, group);
        if (BCM_FAILURE(rv)) {
            _BCM_OAM_UNLOCK(oc);
            OAM_ERR(("OAM(unit %d) Error: Group destroy failed "
                    "(GID=%d) - %s.\n", unit, group, bcm_errmsg(rv)));
            return (rv);
        }
    }

    _BCM_OAM_UNLOCK(oc);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);                                                      
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcm_tr3_oam_group_traverse
 * Purpose:
 *     Traverse the entire set of OAM groups, calling a specified
 *     callback for each one
 * Parameters:
 *     unit  - (IN) BCM device number
 *     cb        - (IN) Pointer to call back function.
 *     user_data - (IN) Pointer to user supplied data.
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_oam_group_traverse(int unit, bcm_oam_group_traverse_cb cb,
                           void *user_data)
{
    _bcm_oam_control_t    *oc;        /* OAM control structure pointer. */
    bcm_oam_group_info_t  group_info; /* Group information to be set.   */
    bcm_oam_group_t       grp_idx;    /* MA Group index.                */
    _bcm_oam_group_data_t *group_p;   /* Pointer to group list.         */
    int                   rv;         /* Operation return status.       */

    if (NULL == cb) {
        return (BCM_E_PARAM);
    }

    /* Get device OAM control structure handle. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    /* Initialize to group array pointer. */
    group_p = oc->group_info;

    for (grp_idx = 0; grp_idx < oc->group_count; grp_idx++) {

        /* Check if the group is in use. */
        if (BCM_E_EXISTS
            == shr_idxres_list_elem_state(oc->group_pool, grp_idx)) {

            /* Initialize the group information structure. */
            bcm_oam_group_info_t_init(&group_info);

            /* Retrieve group information and set in group_info structure. */
            rv = _bcm_tr3_oam_get_group(unit, grp_idx, group_p, &group_info);
            if (BCM_FAILURE(rv)) {
                _BCM_OAM_UNLOCK(oc);
                OAM_ERR(("OAM(unit %d) Error: _bcm_tr3_oam_get_group "
                           "(GID=%d) - %s.\n", unit, grp_idx, bcm_errmsg(rv)));
                return (rv);
            }

            /* Call the user call back routine with group information. */
            rv = cb(unit, &group_info, user_data);
            if (BCM_FAILURE(rv)) {
                _BCM_OAM_UNLOCK(oc);
                OAM_ERR(("OAM(unit %d) Error: User call back routine "
                           "(GID=%d) - %s.\n", unit, grp_idx, bcm_errmsg(rv)));
                return (rv);
            }
        }
    }

    _BCM_OAM_UNLOCK(oc);

    return (BCM_E_NONE);
}


/*
 * Function:
 *     bcm_tr3_oam_endpoint_create
 * Purpose:
 *     Create or replace an OAM endpoint object
 * Parameters:
 *     unit          - (IN) BCM device number
 *     endpoint_info - (IN/OUT) Pointer to endpoint information buffer.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_oam_endpoint_create(int unit, bcm_oam_endpoint_info_t *endpoint_info)
{
    _bcm_oam_hash_data_t *hash_data = NULL; /* Endpoint hash data pointer.  */
    _bcm_oam_hash_data_t h_data_stored;     /* Stored hash data.            */
    _bcm_oam_hash_key_t  hash_key;          /* Hash Key buffer.             */
    int                  ep_req_index;      /* Requested endpoint index.    */
    int                  rv;                /* Operation return status.     */
    _bcm_oam_control_t   *oc;               /* Pointer to OAM control       */
                                            /* structure.                   */
    uint32               sglp = 0;          /* Source global logical port.  */
    uint32               dglp = 0;          /* Dest global logical port.    */
    int                  mep_ccm_tx = 0;    /* Endpoint CCM Tx status.      */
    int                  mep_ccm_rx = 0;    /* Endpoint CCM Rx status.      */
    int                  remote = 0;        /* Remote endpoint status.      */


    OAM_VVERB(("OAM(unit %d) Info: bcm_tr3_oam_endpoint_create "
              "Endpoint ID=%d.\n", unit, endpoint_info->id));

    /* Validate input parameter. */
    if (NULL == endpoint_info) {
        return (BCM_E_PARAM);
    }

    /* Get OAM Control Structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

#if defined(KEY_PRINT)
    _bcm_oam_hash_key_print(&hash_key);
#endif

    /* Calculate the hash key for given enpoint input parameters. */
    _bcm_oam_ep_hash_key_construct(unit, oc, endpoint_info, &hash_key);

    /* Validate endpoint input parameters. */
    rv = _bcm_tr3_oam_endpoint_params_validate(unit, oc, &hash_key,
                                               endpoint_info);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: (EP=%d) - %s.\n",
                unit, endpoint_info->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    if(soc_feature(unit, soc_feature_bhh)) {
#if defined(INCLUDE_BHH)
        if((endpoint_info->type == bcmOAMEndpointTypeBHHMPLS) ||
           (endpoint_info->type == bcmOAMEndpointTypeBHHMPLSVccv))
        {
            rv = bcm_tr3_oam_bhh_endpoint_create(unit, endpoint_info, &hash_key);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: BHH Endpoint create (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
            }
            _BCM_OAM_UNLOCK(oc);

            return (rv);
        }
#endif /* INCLUDE_BHH */
    }

    /* Resolve given endpoint gport value to Source GLP and Dest GLP values. */
    rv = _bcm_tr3_oam_endpoint_gport_resolve(unit, endpoint_info, &sglp, &dglp);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Gport resolve (EP=%d) - %s.\n",
                unit, endpoint_info->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    /* Get MEP CCM Tx status. */
    mep_ccm_tx
        = ((endpoint_info->ccm_period != BCM_OAM_ENDPOINT_CCM_PERIOD_DISABLED)
            ? 1 : 0);

    /* Get MEP CCM Rx status. */
    mep_ccm_rx
        = ((endpoint_info->flags & _BCM_OAM_EP_RX_ENABLE) ? 1 : 0);

    /* Get MEP remote endpoint status. */
    remote = (endpoint_info->flags & BCM_OAM_ENDPOINT_REMOTE) ? 1 : 0;

    /* Replace an existing endpoint. */
    if (endpoint_info->flags & BCM_OAM_ENDPOINT_REPLACE) {
        
        rv = _bcm_tr3_oam_endpoint_destroy(unit, endpoint_info->id);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Endpoint destroy (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
            _BCM_OAM_UNLOCK(oc);
            return (rv);
        }
    }

    /* Create a new endpoint with the requested ID. */
    if (endpoint_info->flags & BCM_OAM_ENDPOINT_WITH_ID) {
        ep_req_index = endpoint_info->id;

        rv = shr_idxres_list_reserve(oc->mep_pool, ep_req_index,
                                     ep_req_index);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: Endpoint reserve (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
            _BCM_OAM_UNLOCK(oc);
            return ((rv == BCM_E_RESOURCE) ? BCM_E_EXISTS : rv);
        }
    } else {

            /* Allocate the next available endpoint index. */
            rv = shr_idxres_list_alloc(oc->mep_pool,
                                   (shr_idxres_element_t *)&ep_req_index);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Endpoint alloc failed - %s.\n",
                    unit, bcm_errmsg(rv)));
                _BCM_OAM_UNLOCK(oc);
                return (rv);
            }
        /* Set the allocated endpoint id value. */
        endpoint_info->id =  ep_req_index;
    }

    /* Get the hash data pointer where the data is to be stored. */
    hash_data = &oc->oam_hash_data[ep_req_index];

    /* Clear the hash data element contents before storing the values. */
    _BCM_OAM_HASH_DATA_CLEAR(hash_data);
    hash_data->type = endpoint_info->type;
    hash_data->ep_id = endpoint_info->id;
    hash_data->is_remote = remote;
    hash_data->local_tx_enabled = mep_ccm_tx;
    hash_data->local_rx_enabled = mep_ccm_rx;
    hash_data->group_index = endpoint_info->group;
    hash_data->name  = endpoint_info->name;
    hash_data->level = endpoint_info->level;
    hash_data->vlan = endpoint_info->vlan;
    hash_data->gport = endpoint_info->gport;
    hash_data->sglp = sglp;
    hash_data->dglp = dglp;
    hash_data->flags = endpoint_info->flags;
    hash_data->period = endpoint_info->ccm_period;
    hash_data->in_use = 1;
    hash_data->fp_entry_tx = -1;
    hash_data->fp_entry_rx = -1;
    hash_data->fp_entry_trunk[0] = -1;
    hash_data->ts_format = endpoint_info->timestamp_format;

    /* Initialize hardware index as invalid indices. */
    hash_data->local_tx_index = _BCM_OAM_INVALID_INDEX;
    hash_data->local_rx_index = _BCM_OAM_INVALID_INDEX;
    hash_data->remote_index = _BCM_OAM_INVALID_INDEX;

    if (1 == remote) {
        /* Allocate the next available index for RMEP table. */
        rv = shr_idxres_list_alloc
                (oc->rmep_pool,
                 (shr_idxres_element_t *)&hash_data->remote_index);
        if (BCM_FAILURE(rv)) {
            _BCM_OAM_UNLOCK(oc);
            OAM_ERR(("OAM(unit %d) Error: RMEP index alloc failed EP:%d %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
            return (rv);
        }
    } else {

        /* Allocate the next available index for LMEP table. */
        if (1 == mep_ccm_tx) {
            rv = shr_idxres_list_alloc
                    (oc->lmep_pool,
                     (shr_idxres_element_t *)&hash_data->local_tx_index);
            if (BCM_FAILURE(rv)) {
                _BCM_OAM_UNLOCK(oc);
                OAM_ERR(("OAM(unit %d) Error: LMEP Tx index alloc failed EP:%d "
                        "%s.\n", unit, endpoint_info->id, bcm_errmsg(rv)));
                return (rv);
            }
        }

        /* Allocate the next available index for MA_INDEX table. */
        if (1 == mep_ccm_rx) {
            rv =  _bcm_oam_rx_endpoint_reserve(unit, endpoint_info);
            if (BCM_FAILURE(rv)) {
                if (1 == mep_ccm_tx) {
                    /* Return Tx index to the LMEP pool. */
                    shr_idxres_list_free(oc->lmep_pool,
                                         hash_data->local_tx_index);
                }

                /* Return endpoint index to MEP pool. */
                shr_idxres_list_free(oc->mep_pool, endpoint_info->id);

                _BCM_OAM_UNLOCK(oc);
                OAM_ERR(("OAM(unit %d) Error: LMEP Rx index alloc failed EP:%d "
                        "%s.\n", unit, endpoint_info->id, bcm_errmsg(rv)));
                return (rv);
            }
        }
    }


    rv = shr_htb_insert(oc->ma_mep_htbl, hash_key, hash_data);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Hash table insert failed EP=%d %s.\n",
                unit, endpoint_info->id, bcm_errmsg(rv)));
        return (rv);
    }

    if (1 == remote) {
        rv = _bcm_oam_remote_mep_hw_set(unit, endpoint_info);
        if (BCM_FAILURE(rv)) {
            OAM_VVERB(("OAM(unit %d) Error: Remote MEP set failed EP=%d %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
            /*
             * If not a replace operation, return the allocated indices
             * to free pool.
             */
            if (!(endpoint_info->flags & BCM_OAM_ENDPOINT_REPLACE)) {
                shr_idxres_list_free(oc->mep_pool, endpoint_info->id);
                shr_idxres_list_free(oc->rmep_pool, hash_data->remote_index);

                /* Return entry to hash data free pool. */
                shr_htb_find(oc->ma_mep_htbl, hash_key,
                            (shr_htb_data_t *) &h_data_stored, 1);

                /* Clear contents of hash data element. */
                _BCM_OAM_HASH_DATA_CLEAR(hash_data);
            }
            _BCM_OAM_UNLOCK(oc);
            return (rv);
        }
    } else {
        if (mep_ccm_tx) {
            rv = _bcm_oam_local_tx_mep_hw_set(unit, endpoint_info);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Tx config failed for EP=%d %s.\n",
                        unit, endpoint_info->id, bcm_errmsg(rv)));
                /*
                 * If not a replace operation, return the allocated indices
                 * to free pool.
                 */
                if (!(endpoint_info->flags & BCM_OAM_ENDPOINT_REPLACE)) {
                    shr_idxres_list_free(oc->mep_pool, endpoint_info->id);
                    shr_idxres_list_free(oc->lmep_pool,
                                         hash_data->local_tx_index);

                    /* Return entry to hash data entry to free pool. */
                    shr_htb_find(oc->ma_mep_htbl, hash_key,
                                 (shr_htb_data_t *)&h_data_stored, 1);

                    /* Clear contents of hash data element. */
                    _BCM_OAM_HASH_DATA_CLEAR(hash_data);
                }
                _BCM_OAM_UNLOCK(oc);
                return (rv);
            }
        }

        if (mep_ccm_rx) {
            rv = _bcm_oam_local_rx_mep_hw_set(unit, endpoint_info);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: Rx config failed for EP=%d %s.\n",
                        unit, endpoint_info->id, bcm_errmsg(rv)));
                /*
                 * If not a replace operation, return the allocated indices
                 * to free pool.
                 */
                if (!(endpoint_info->flags & BCM_OAM_ENDPOINT_REPLACE)) {
                    shr_idxres_list_free(oc->ma_idx_pool,
                                         hash_data->local_rx_index);

                    shr_idxres_list_free(oc->mep_pool, endpoint_info->id);

                    if (1 == mep_ccm_tx) {
                        shr_idxres_list_free(oc->lmep_pool,
                                             hash_data->local_tx_index);
                    }

                    /* Return entry to hash data entry to free pool. */
                    shr_htb_find(oc->ma_mep_htbl, hash_key,
                                 (shr_htb_data_t *)&h_data_stored, 1);

                    /* Clear contents of hash data element. */
                    _BCM_OAM_HASH_DATA_CLEAR(hash_data);

                }

                /* Clear contents of hash data element. */
                _BCM_OAM_UNLOCK(oc);
                return (rv);
            } else {
                if ( endpoint_info->flags & 
                     (BCM_OAM_ENDPOINT_LOSS_MEASUREMENT | 
                      BCM_OAM_ENDPOINT_DELAY_MEASUREMENT)) {
                      
                    rv  = _bcm_tr3_oam_loss_delay_measurement_add(unit, 
                                                oc, hash_data, endpoint_info);
                    if (BCM_FAILURE(rv)) {
                        OAM_ERR(("OAM(unit %d) Error: FP insert failed for \
                        LM DM over EP=%d %s.\n", unit, endpoint_info->id, 
                        bcm_errmsg(rv)));
                        
                        /*
                         * If not a replace operation, return the allocated 
                         * indices to free pool.
                         */
                        if (!(endpoint_info->flags & BCM_OAM_ENDPOINT_REPLACE)){
                            shr_idxres_list_free(oc->ma_idx_pool,
                                                 hash_data->local_rx_index);

                           shr_idxres_list_free(oc->mep_pool,endpoint_info->id);

                            if (1 == mep_ccm_tx) {
                                shr_idxres_list_free(oc->lmep_pool,
                                                     hash_data->local_tx_index);
                            }

                            /* Return entry to hash data entry to free pool. */
                            shr_htb_find(oc->ma_mep_htbl, hash_key,
                                         (shr_htb_data_t *)&h_data_stored, 1);

                            /* Clear contents of hash data element. */
                            _BCM_OAM_HASH_DATA_CLEAR(hash_data);

                        }

                        /* Unlock OAM Control Structure. */
                        _BCM_OAM_UNLOCK(oc);
                        return (rv);
                    }
                }
            }
        }
    }

    rv = _bcm_oam_group_ep_list_add(unit, endpoint_info->group,
                                    endpoint_info->id);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Tx config failed for EP=%d %s.\n",
                unit, endpoint_info->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    _BCM_OAM_UNLOCK(oc);

    return (BCM_E_NONE);
}

 /*
 * Function:
 *      bcm_tr3_oam_endpoint_get
 * Purpose:
 *      Get an OAM endpoint object
 * Parameters:
 *     unit          - (IN) BCM device number
 *     endpoint      - (IN) Endpoint ID
 *     endpoint_info - (OUT) Pointer to OAM endpoint information buffer.
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_oam_endpoint_get(int unit, bcm_oam_endpoint_t endpoint,
                         bcm_oam_endpoint_info_t *endpoint_info)
{
    _bcm_oam_hash_data_t *h_data_p;  /* Pointer to endpoint hash data.    */
    int                  rv;         /* Operation return status.          */
    _bcm_oam_control_t   *oc;        /* Pointer to OAM control structure. */
    rmep_entry_t         rmep_entry; /* Remote MEP entry buffer.          */
    lmep_entry_t         lmep_entry; /* Local MEP entry buffer.           */
    lmep_da_entry_t      dmac_entry; /* Local MEP entry buffer.           */
#if defined(INCLUDE_BHH)
    shr_bhh_msg_ctrl_sess_get_t msg;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int sess_id = 0;
    bcm_l3_egress_t l3_egress;
    bcm_l3_intf_t l3_intf;
#endif


    if (NULL == endpoint_info) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Validate endpoint index value. */
    _BCM_OAM_EP_INDEX_VALIDATE(endpoint);

    _BCM_OAM_LOCK(oc);

    /* Check endpoint status. */
    rv = shr_idxres_list_elem_state(oc->mep_pool, endpoint);
    if ((BCM_E_EXISTS != rv)) {
        /* Endpoint not in use. */
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, endpoint, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    h_data_p =  &oc->oam_hash_data[endpoint];
    if (NULL == h_data_p) {
        _BCM_OAM_UNLOCK(oc);
        return (BCM_E_INTERNAL);
    }

    OAM_VVERB(("OAM(unit %d) Info: Endpoint (EP=%d) remote=%d local_tx=%d"
              "local_tx_idx=%d local_rx_en=%d local_rx_idx=%d\n",
              unit, endpoint, h_data_p->is_remote, h_data_p->local_tx_enabled,
              h_data_p->local_tx_index, h_data_p->local_rx_enabled,
              h_data_p->local_rx_index));

    if (bcmOAMEndpointTypeEthernet == h_data_p->type) {
        if (1 == h_data_p->is_remote) {
            sal_memset(&rmep_entry, 0, sizeof(rmep_entry_t));

            /* Get hardware table entry information.  */
            rv = READ_RMEPm(unit, MEM_BLOCK_ANY, h_data_p->remote_index,
                            &rmep_entry);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: RMEP table read failed for"
                         " EP=%d %s.\n", unit, endpoint, bcm_errmsg(rv)));
                _BCM_OAM_UNLOCK(oc);
                return (rv);
            }

            rv = _bcm_tr3_oam_read_clear_faults(unit, h_data_p->remote_index,
                                                RMEPm, (uint32 *) &rmep_entry,
                                                (void *) endpoint_info);
            if (BCM_FAILURE(rv)) {
                OAM_ERR(("OAM(unit %d) Error: RMEP table read failed for"
                         " EP=%d %s.\n", unit, endpoint, bcm_errmsg(rv)));
                _BCM_OAM_UNLOCK(oc);
                return (rv);
            }
        } else {
            if (1 == h_data_p->local_tx_enabled) {
                sal_memset(&lmep_entry, 0, sizeof(lmep_entry_t));

                /* Get hardware table entry information.  */
                rv = READ_LMEPm(unit, MEM_BLOCK_ANY, h_data_p->local_tx_index,
                                &lmep_entry);
                if (BCM_FAILURE(rv)) {
                    OAM_ERR(("OAM(unit %d) Error: LMEP table read failed for"
                             " EP=%d %s.\n", unit, endpoint, bcm_errmsg(rv)));
                    _BCM_OAM_UNLOCK(oc);
                    return (rv);
                }

                soc_LMEPm_mac_addr_get(unit, &lmep_entry, SAf,
                                       endpoint_info->src_mac_address);
                endpoint_info->pkt_pri = soc_LMEPm_field32_get(unit,
                                                               &lmep_entry,
                                                               LMEP_PRIORITYf);
                endpoint_info->int_pri = soc_LMEPm_field32_get(unit,
                                                               &lmep_entry,
                                                               INT_PRIf);
                endpoint_info->port_state = (soc_LMEPm_field32_get(unit,
                                                &lmep_entry, PORT_TLVf)
                                                ? BCM_OAM_PORT_TLV_UP
                                                    : BCM_OAM_PORT_TLV_BLOCKED);
                endpoint_info->interface_state = soc_LMEPm_field32_get(unit,
                                                    &lmep_entry, INTERFACE_TLVf);

                sal_memset(&dmac_entry, 0, sizeof(lmep_da_entry_t));

                /* Get hardware table entry information. */
                rv = READ_LMEP_DAm(unit, MEM_BLOCK_ANY, h_data_p->local_tx_index,
                                   &dmac_entry);
                if (BCM_FAILURE(rv)) {
                    OAM_ERR(("OAM(unit %d) Error: LMEP_DA table read failed for"
                             " EP=%d %s.\n", unit, endpoint, bcm_errmsg(rv)));
                    _BCM_OAM_UNLOCK(oc);
                    return (rv);
                }

#ifdef BCM_HURRICANE2_SUPPORT
                if (SOC_IS_HURRICANE2(unit)) {
                    soc_mem_mac_addr_get(unit, LMEP_DAm, &dmac_entry, MACDAf,
                                         endpoint_info->dst_mac_address);
 
                } else 
#endif /* BCM_HURRICANE2_SUPPORT */
                {
                    soc_mem_mac_addr_get(unit, LMEP_DAm, &dmac_entry, DAf,
                                         endpoint_info->dst_mac_address);
                }
            }
        }
    }
    else if (soc_feature(unit, soc_feature_bhh) && 
            ((h_data_p->type == bcmOAMEndpointTypeBHHMPLS) || 
             (h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv))) {
#if defined(INCLUDE_BHH)
        sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(endpoint);
        rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                                               MOS_MSG_SUBCLASS_BHH_SESS_GET,
                                               sess_id, 0,
                                               MOS_MSG_SUBCLASS_BHH_SESS_GET_REPLY,
                                               &reply_len);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("OAM(unit %d) Error: ukernel msg failed for"
                     " EP=%d %s.\n", unit, endpoint, bcm_errmsg(rv)));
            _BCM_OAM_UNLOCK(oc);
            return (rv);
        }

        /* Pack control message data into DMA buffer */
        buffer = oc->dma_buffer;
        buffer_ptr = shr_bhh_msg_ctrl_sess_get_unpack(buffer, &msg);
        buffer_len = buffer_ptr - buffer;

        if (reply_len != buffer_len) {
            rv = BCM_E_INTERNAL;
            OAM_ERR(("OAM(unit %d) Error: ukernel msg failed for"
                        " EP=%d %s.\n", unit, endpoint, bcm_errmsg(rv)));
            _BCM_OAM_UNLOCK(oc);
            return (rv);
        } else {
            endpoint_info->ccm_period = msg.period;
            endpoint_info->int_pri = msg.tx_cos;
            endpoint_info->pkt_pri = msg.tx_pri;
        }

        endpoint_info->intf_id    = h_data_p->egress_if;
        endpoint_info->cpu_qid    =  h_data_p->cpu_qid;
        endpoint_info->mpls_label = h_data_p->label;
        endpoint_info->gport      = h_data_p->gport;
        endpoint_info->vpn        = h_data_p->vpn;
        endpoint_info->vccv_type  = h_data_p->vccv_type;

        /*
         * Get MAC address
         */
        bcm_l3_egress_t_init(&l3_egress);
        bcm_l3_intf_t_init(&l3_intf);

        if (BCM_FAILURE
            (bcm_esw_l3_egress_get(unit, h_data_p->egress_if, &l3_egress))) {
            _BCM_OAM_UNLOCK(oc);
            return (BCM_E_INTERNAL);
        }

        l3_intf.l3a_intf_id = l3_egress.intf;
        if (BCM_FAILURE(bcm_esw_l3_intf_get(unit, &l3_intf))) {
            _BCM_OAM_UNLOCK(oc);
            return (BCM_E_INTERNAL);
        }

        sal_memcpy(endpoint_info->src_mac_address, l3_intf.l3a_mac_addr, _BHH_MAC_ADDR_LENGTH);
#else
        return (BCM_E_UNAVAIL);
#endif /* INCLUDE_BHH */
    }
    else {
        return (BCM_E_PARAM);
    }

    endpoint_info->id = endpoint;
    endpoint_info->group = h_data_p->group_index;
    endpoint_info->name = h_data_p->name;
    endpoint_info->vlan = h_data_p->vlan;
    endpoint_info->level = h_data_p->level;
    endpoint_info->gport = h_data_p->gport;
    endpoint_info->ccm_period = h_data_p->period;
    endpoint_info->flags |= h_data_p->flags;
    endpoint_info->flags &= ~(BCM_OAM_ENDPOINT_WITH_ID);
    endpoint_info->type = h_data_p->type;

    _BCM_OAM_UNLOCK(oc);

    return (BCM_E_NONE);
}


/*
 * Function:
 *     bcm_tr3_oam_endpoint_destroy
 * Purpose:
 *     Destroy an OAM endpoint object
 * Parameters:
 *     unit     - (IN) BCM device number
 *     endpoint - (IN) Endpoint ID to destroy.
 * result =s:
 *     BCM_E_XXX
 */
int
bcm_tr3_oam_endpoint_destroy(int unit, bcm_oam_endpoint_t endpoint)
{
    _bcm_oam_control_t *oc; /* Pointer to OAM control structure. */
    int rv;                 /* Operation return status.          */

    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    rv = _bcm_tr3_oam_endpoint_destroy(unit, endpoint);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Endpoint destroy EP=%d failed - "
                 "%s.\n", unit, endpoint, bcm_errmsg(rv)));
    }

    _BCM_OAM_UNLOCK(oc);
    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_endpoint_destroy_all
 * Purpose:
 *     Destroy all OAM endpoint objects associated with a group.
 * Parameters:
 *     unit  - (IN) BCM device number
 *     group - (IN) The OAM group whose endpoints should be destroyed
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_tr3_oam_endpoint_destroy_all(int unit, bcm_oam_group_t group)
{
    _bcm_oam_control_t    *oc; /* Pointer to OAM control structure. */
    int                   rv;  /* Operation return status.          */
    _bcm_oam_group_data_t *g_info_p;

    /* Get OAM device control structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Validate group index. */
    _BCM_OAM_GROUP_INDEX_VALIDATE(group);

    _BCM_OAM_LOCK(oc);

    /* Check if the group is in use. */
    rv = shr_idxres_list_elem_state(oc->group_pool, group);
    if (BCM_E_EXISTS != rv) {
        _BCM_OAM_UNLOCK(oc);
        OAM_ERR(("OAM(unit %d) Error: Group ID=%d does not exist.\n",
                unit, group));
        return (rv);
    }

    /* Get the group data pointer. */
    g_info_p = &oc->group_info[group];
    rv = _bcm_tr3_oam_group_endpoints_destroy(unit, g_info_p);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Group (GID=%d) endpoints destroy"
                " failed - %s.\n", unit, group, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }
    _BCM_OAM_UNLOCK(oc);
    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_endpoint_traverse
 * Purpose:
 *     Traverse the set of OAM endpoints associated with the
 *     specified group, calling a specified callback for each one
 * Parameters:
 *     unit      - (IN) BCM device number
 *     group     - (IN) The OAM group whose endpoints should be traversed
 *     cb        - (IN) A pointer to the callback function to call for each OAM
 *                      endpoint in the specified group
 *     user_data - (IN) Pointer to user data to supply in the callback
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_oam_endpoint_traverse(int unit, bcm_oam_group_t group,
                              bcm_oam_endpoint_traverse_cb cb,
                              void *user_data)
{
    _bcm_oam_control_t      *oc; /* Pointer to OAM control structure. */
    int                     rv;  /* Operation return status.          */
    bcm_oam_endpoint_info_t ep_info;
    _bcm_oam_hash_data_t    *h_data_p;
    _bcm_oam_ep_list_t      *cur;
    _bcm_oam_group_data_t   *g_info_p;

    /* Validate input parameter. */
    if (NULL == cb) {
        return (BCM_E_PARAM);
    }

    /* Get OAM device control structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Validate group index. */
    _BCM_OAM_GROUP_INDEX_VALIDATE(group);

    _BCM_OAM_LOCK(oc);

    /* Check if the group is in use. */
    rv = shr_idxres_list_elem_state(oc->group_pool, group);
    if (BCM_E_EXISTS != rv) {
        _BCM_OAM_UNLOCK(oc);
        OAM_ERR(("OAM(unit %d) Error: Group ID=%d does not exist.\n",
                unit, group));
        return (rv);
    }

    /* Get the group data pointer. */
    g_info_p = &oc->group_info[group];

    /* Get the endpoint list head pointer. */
    cur = *(g_info_p->ep_list);
    if (NULL == cur) {
        _BCM_OAM_UNLOCK(oc);
        OAM_VVERB(("OAM(unit %d) Info: No endpoints in group GID=%d.\n",
                 unit, group));
        return (BCM_E_NONE);
    }

    /* Traverse from Head to Tail */
    while (NULL != cur) {
        h_data_p = cur->ep_data_p;
        if (NULL == h_data_p) {
            OAM_ERR(("OAM(unit %d) Error: Group=%d endpoints access failed -"
                    " %s.\n", unit, group, bcm_errmsg(BCM_E_INTERNAL)));
            _BCM_OAM_UNLOCK(oc);
            return (BCM_E_INTERNAL);
        }
        bcm_oam_endpoint_info_t_init(&ep_info);

        rv = bcm_tr3_oam_endpoint_get(unit, h_data_p->ep_id, &ep_info);
        if (BCM_FAILURE(rv)) {
            _BCM_OAM_UNLOCK(oc);
            OAM_ERR(("OAM(unit %d) Error: EP=%d info get failed %s.\n",
                    unit, h_data_p->ep_id, bcm_errmsg(rv)));
            return (rv);
        }

        /* Get the next node handle before calling the callback as current
         * node may move to the head if modification is performed in the 
         * callback */
        cur = cur->next;

        rv = cb(unit, &ep_info, user_data);
        if (BCM_FAILURE(rv)) {
            _BCM_OAM_UNLOCK(oc);
            OAM_ERR(("OAM(unit %d) Error: EP=%d callback failed - %s.\n",
                    unit, h_data_p->ep_id, bcm_errmsg(rv)));
            return (rv);
        }
    }
    _BCM_OAM_UNLOCK(oc);
    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_event_register
 * Purpose:
 *     Register a callback for handling OAM events
 * Parameters:
 *     unit        - (IN) BCM device number
 *     event_types - (IN) The set of OAM events for which the specified
 *                        callback should be called.
 *     cb          - (IN) A pointer to the callback function to call for
 *                        the specified OAM events
 *     user_data   - (IN) Pointer to user data to supply in the callback
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_oam_event_register(int unit, bcm_oam_event_types_t event_types,
                           bcm_oam_event_cb cb, void *user_data)
{
    _bcm_oam_control_t       *oc;
    _bcm_oam_event_handler_t *event_h_p;
    _bcm_oam_event_handler_t *prev_p = NULL;
    bcm_oam_event_type_t     e_type;
    uint32                   rval;
    int                      hw_update = 0;
    uint32                   event_bmp;
    int                      rv = BCM_E_NONE;     /* Operation return status. */

    /* Validate event callback input parameter. */
    if (NULL == cb) {
        return (BCM_E_PARAM);
    }

    /* Check if an event is set for register in the events bitmap. */
    SHR_BITTEST_RANGE(event_types.w, 0, bcmOAMEventCount, event_bmp);
    if (0 == event_bmp) {
        /* No events specified. */
        OAM_ERR(("OAM(unit %d) Error: No events specified for register.\n",
                 unit));
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    for (event_h_p = oc->event_handler_list_p; event_h_p != NULL;
         event_h_p = event_h_p->next_p) {
        if (event_h_p->cb == cb) {
            break;
        }
        prev_p = event_h_p;
    }

    if (NULL == event_h_p) {

        _BCM_OAM_ALLOC(event_h_p, _bcm_oam_event_handler_t,
             sizeof(_bcm_oam_event_handler_t), "OAM event handler");

        if (NULL == event_h_p) {
            _BCM_OAM_UNLOCK(oc);
            OAM_ERR(("OAM(unit %d) Error: Event handler alloc failed -"
                    " %s.\n", unit, bcm_errmsg(BCM_E_MEMORY)));
            return (BCM_E_MEMORY);
        }

        event_h_p->next_p = NULL;
        event_h_p->cb = cb;

        SHR_BITCLR_RANGE(event_h_p->event_types.w, 0, bcmOAMEventCount);
        if (prev_p != NULL) {
            prev_p->next_p = event_h_p;
        } else {
            oc->event_handler_list_p = event_h_p;
        }
    }

    rv = READ_CCM_INTERRUPT_CONTROLr(unit, &rval);
    if (BCM_FAILURE(rv)) {
        _BCM_OAM_UNLOCK(oc);
        OAM_ERR(("OAM(unit %d) Error: CCM interrupt control read failed -"
                " %s.\n", unit, bcm_errmsg(rv)));
        return (rv);
    }

    for (e_type = 0; e_type < bcmOAMEventCount; ++e_type) {
        if (SHR_BITGET(event_types.w, e_type)) {
            if(soc_feature(unit, soc_feature_bhh)) {
#if defined(INCLUDE_BHH)
                /*
                * BHH events are generated by the uKernel
                */
                if ((e_type == bcmOAMEventBHHLBTimeout) ||
                    (e_type == bcmOAMEventBHHLBDiscoveryUpdate) ||
                    (e_type == bcmOAMEventBHHCCMTimeout) ||
                    (e_type == bcmOAMEventBHHCCMState)) {
                    SHR_BITSET(event_h_p->event_types.w, e_type);
                    oc->event_handler_cnt[e_type] += 1;
                    continue;
                }
#endif
            }
            if (!soc_reg_field_valid
                    (unit, CCM_INTERRUPT_CONTROLr,
                     _tr3_oam_intr_en_fields[e_type].field)) {
                continue;
            }

            if (!SHR_BITGET(event_h_p->event_types.w, e_type)) {
                /* Add this event to the registered events list. */
                SHR_BITSET(event_h_p->event_types.w, e_type);
                oc->event_handler_cnt[e_type] += 1;
                if (1 == oc->event_handler_cnt[e_type]) {
                    hw_update = 1;
                     soc_reg_field_set
                        (unit, CCM_INTERRUPT_CONTROLr, &rval,
                         _tr3_oam_intr_en_fields[e_type].field, 1);
                }
            }
        }
    }

    event_h_p->user_data = user_data;

    if (1 == hw_update) {
        rv = WRITE_CCM_INTERRUPT_CONTROLr(unit, rval);
        if (BCM_FAILURE(rv)) {
            _BCM_OAM_UNLOCK(oc);
            OAM_ERR(("OAM(unit %d) Error: CCM interrupt control write failed -"
                " %s.\n", unit, bcm_errmsg(rv)));
            return (rv);
        }
    }

    if (soc_feature(unit, soc_feature_bhh)) {
#if defined(INCLUDE_BHH)
        /* 
         * Update BHH Events mask 
         */
        rv = _bcm_tr3_oam_bhh_event_mask_set(unit);
#endif /* INCLUDE_BHH */
    }

    _BCM_OAM_UNLOCK(oc);

    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_event_unregister
 * Purpose:
 *     Remove a registered event from the event handler list.
 * Parameters:
 *     unit        - (IN) BCM device number
 *     event_types - (IN) The set of OAM events for which the specified
 *                        callback should not be called
 *     cb          - (IN) A pointer to the callback function to unregister
 *                        from the specified OAM events
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_oam_event_unregister(int unit, bcm_oam_event_types_t event_types,
                             bcm_oam_event_cb cb)
{
 
    _bcm_oam_control_t       *oc;
    _bcm_oam_event_handler_t *event_h_p;
    _bcm_oam_event_handler_t *prev_p = NULL;
    bcm_oam_event_type_t     e_type;
    uint32                   rval;
    int                      hw_update = 0;
    uint32                   event_bmp;
    int                      rv = BCM_E_NONE;     /* Operation return status. */
  
    /* Validate event callback input parameter. */
    if (NULL == cb) {
        return (BCM_E_PARAM);
    }

    /* Check if an event is set for unregister in the events bitmap. */
    SHR_BITTEST_RANGE(event_types.w, 0, bcmOAMEventCount, event_bmp);
    if (0 == event_bmp) {
        /* No events specified. */
        OAM_ERR(("OAM(unit %d) Error: No events specified for register.\n",
                 unit));
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    for (event_h_p = oc->event_handler_list_p; event_h_p != NULL;
         event_h_p = event_h_p->next_p) {
        if (event_h_p->cb == cb) {
            break;
        }
        prev_p = event_h_p;
    }

    if (NULL == event_h_p) {
        _BCM_OAM_UNLOCK(oc);
        return (BCM_E_NOT_FOUND);
    }
 
    rv = READ_CCM_INTERRUPT_CONTROLr(unit, &rval);
    if (BCM_FAILURE(rv)) {
        _BCM_OAM_UNLOCK(oc);
        OAM_ERR(("OAM(unit %d) Error: CCM interrupt control read failed -"
                " %s.\n", unit, bcm_errmsg(rv)));
        return (rv);
    }

    for (e_type = 0; e_type < bcmOAMEventCount; ++e_type) {

        if (SHR_BITGET(event_types.w, e_type)) {
            if(soc_feature(unit, soc_feature_bhh)) {
#if defined(INCLUDE_BHH)
                /*
                * BHH events are generated by the uKernel
                */
                if ((e_type == bcmOAMEventBHHLBTimeout) ||
                    (e_type == bcmOAMEventBHHLBDiscoveryUpdate) ||
                    (e_type == bcmOAMEventBHHCCMTimeout) ||
                    (e_type == bcmOAMEventBHHCCMState)) {
                    SHR_BITSET(event_h_p->event_types.w, e_type);
                    oc->event_handler_cnt[e_type] -= 1;
                    continue;
                }
#endif
            }

            if (!soc_reg_field_valid
                    (unit, CCM_INTERRUPT_CONTROLr,
                     _tr3_oam_intr_en_fields[e_type].field)) {
                _BCM_OAM_UNLOCK(oc);
                return (BCM_E_UNAVAIL);
            }

            if ((oc->event_handler_cnt[e_type] > 0)
                && SHR_BITGET(event_h_p->event_types.w, e_type)) {

                /* Remove this event from the registered events list. */
                SHR_BITCLR(event_h_p->event_types.w, e_type);

                oc->event_handler_cnt[e_type] -= 1;

                if (0 == oc->event_handler_cnt[e_type]) {
                    hw_update = 0;
                     soc_reg_field_set
                        (unit, CCM_INTERRUPT_CONTROLr, &rval,
                         _tr3_oam_intr_en_fields[e_type].field, 0);
                }
            }
        }
    }

    if (1 == hw_update) {
        rv = WRITE_CCM_INTERRUPT_CONTROLr(unit, rval);
        if (BCM_FAILURE(rv)) {
            _BCM_OAM_UNLOCK(oc);
            OAM_ERR(("OAM(unit %d) Error: CCM interrupt control write failed -"
                " %s.\n", unit, bcm_errmsg(rv)));
            return (rv);
        }
    }

    SHR_BITTEST_RANGE(event_h_p->event_types.w, 0, bcmOAMEventCount, event_bmp);

    if (0 == event_bmp) {

        if (NULL != prev_p) {

            prev_p->next_p = event_h_p->next_p;
            
        } else {

            oc->event_handler_list_p = event_h_p->next_p;

        }
        sal_free(event_h_p);
    }

    if(soc_feature(unit, soc_feature_bhh)) {
#if defined(INCLUDE_BHH)
        /* 
         * Update BHH Events mask 
         */
        rv = _bcm_tr3_oam_bhh_event_mask_set(unit);
#endif /* INCLUDE_BHH */
    }

    _BCM_OAM_UNLOCK(oc);

    return (rv);
}

#if defined(INCLUDE_BHH)
/*
 * Function:
 *      _bcm_tr3_oam_bhh_fp_set
 * Purpose:
 *      Sets BHH FP for LM/DM in HW device.
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
_bcm_tr3_oam_bhh_fp_set(int unit, _bcm_oam_hash_data_t *hash_data, uint32 nhi,
                        bcm_oam_endpoint_info_t *endpoint_info)
{
    int rv = BCM_E_NONE;
    _bcm_oam_control_t *oc;
    bcm_gport_t gport;
    bcm_module_t module_id;
    bcm_port_t port_id;
    bcm_trunk_t trunk_id;
    int         local_id;
    void *entries[1];
    int i;
    uint32 mem_entries[BCM_OAM_INTPRI_MAX], profile_index;
    bcm_mpls_label_t label = hash_data->label << 12;
    uint32 label_mask = SHR_BHH_MPLS_LABEL_MASK;
    uint16 ach_type = SHR_BHH_ACH_CHANNEL_TYPE;
    uint16 ach_mask = 0xFFFF;
    uint8 op_code;
    uint8 op_mask;

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    if(oc->bhh_lmdm_group == 0) {
        BCM_IF_ERROR_RETURN(bcm_esw_field_group_create(unit, 
                            oc->bhh_lmdm_qset, 
                            BCM_FIELD_GROUP_PRIO_ANY, &oc->bhh_lmdm_group));
        oc->bhh_lmdm_entry_count = 0;
    }

    rv = _bcm_esw_gport_resolve(unit,
            hash_data->gport, &module_id, &port_id, &trunk_id, &local_id);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("BHH(unit %d) Error: Gport resolve (EP=%d) - %s.\n",
                unit, hash_data->ep_id, bcm_errmsg(rv)));
        return (rv);
    }

    if (module_id != 0) {
        /* Modport */
        BCM_GPORT_MODPORT_SET(gport, module_id, port_id);
    } else {
        /* Local port */
        BCM_IF_ERROR_RETURN(bcm_esw_port_gport_get(unit, port_id, &gport));
    }

    if(hash_data->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT) {
        BCM_IF_ERROR_RETURN(bcm_esw_field_entry_create(unit,
                            oc->bhh_lmdm_group,
                            &hash_data->bhh_dm_entry_rx));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_SrcPort(unit, 
                            hash_data->bhh_dm_entry_rx,
                            0, 0x3f, gport, BCM_FIELD_EXACT_MATCH_MASK));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_OuterVlanId(unit, 
                            hash_data->bhh_dm_entry_rx,
                            hash_data->vlan, 0x0fff));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_data(unit, 
                            hash_data->bhh_dm_entry_rx,
                            oc->bhh_qual_label.qual_id,
                            (uint8 *)&label, (uint8 *)&label_mask, 
                            SHR_BHH_MPLS_LABEL_LENGTH));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_data(unit, 
                            hash_data->bhh_dm_entry_rx,
                            oc->bhh_qual_ach.qual_id, 
                            (uint8 *)&ach_type, (uint8 *)&ach_mask, 
                            SHR_BHH_ACH_TYPE_LENGTH));
        op_code = SHR_BHH_OPCODE_DM_PREFIX;
        op_mask = SHR_BHH_OPCODE_DM_MASK;
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_data(unit, 
                            hash_data->bhh_dm_entry_rx,
                            oc->bhh_qual_opcode.qual_id, 
                            &op_code, &op_mask, SHR_BHH_OPCODE_LEN));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit, 
                            hash_data->bhh_dm_entry_rx,
                            bcmFieldActionOamDmEnable, 1, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit, 
                            hash_data->bhh_dm_entry_rx,
                            bcmFieldActionOamDmTimeFormat, 1, 0)); /* NTP */
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit, 
                            hash_data->bhh_dm_entry_rx,
                            bcmFieldActionOamLmepEnable, 0, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit,
                            hash_data->bhh_dm_entry_rx,
                            bcmFieldActionOamLmDmSampleEnable, 1, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_entry_install(unit, 
                            hash_data->bhh_dm_entry_rx));
        if(hash_data->bhh_dm_entry_rx)
            oc->bhh_lmdm_entry_count++;
    }

    if(hash_data->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT) {
        /* Allocate the next available endpoint index. */
        rv = shr_idxres_list_alloc(oc->lm_counter_pool,
                          (shr_idxres_element_t *)&hash_data->lm_counter_index);
        if (BCM_FAILURE(rv)) {
            rv = (rv == BCM_E_RESOURCE) ? (BCM_E_FULL) : rv;
                OAM_ERR(("BHH(unit %d) Error: lm counter idx alloc failed - %s.\n",
                    unit, bcm_errmsg(rv)));
            return (rv);
        }

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
        SOC_IF_ERROR_RETURN(soc_profile_mem_add(unit, 
                            &oc->ing_service_pri_map,
                            (void *) &entries, BCM_OAM_INTPRI_MAX, &profile_index));
        hash_data->pri_map_index = profile_index/BCM_OAM_INTPRI_MAX;
        soc_mem_unlock(unit, ING_SERVICE_PRI_MAPm);

        /* policy to get lm counter value into DCBs*/
        BCM_IF_ERROR_RETURN(bcm_esw_field_entry_create(unit,
                            oc->bhh_lmdm_group,
                            &hash_data->bhh_lm_entry_rx));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_SrcPort(unit, 
                            hash_data->bhh_lm_entry_rx,
                            0, 0x3f, gport, BCM_FIELD_EXACT_MATCH_MASK));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_OuterVlanId(unit, 
                            hash_data->bhh_lm_entry_rx,
                            hash_data->vlan, 0x0fff));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_data(unit, 
                            hash_data->bhh_lm_entry_rx,
                            oc->bhh_qual_label.qual_id,
                            (uint8 *)&label, (uint8 *)&label_mask, 
                            SHR_BHH_MPLS_LABEL_LENGTH));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_data(unit,
                            hash_data->bhh_lm_entry_rx,
                            oc->bhh_qual_ach.qual_id, 
                            (uint8 *)&ach_type, (uint8 *)&ach_mask, 
                            SHR_BHH_ACH_TYPE_LENGTH));
        op_code = SHR_BHH_OPCODE_LM_PREFIX;
        op_mask = SHR_BHH_OPCODE_LM_MASK;
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_data(unit,
                            hash_data->bhh_lm_entry_rx,
                            oc->bhh_qual_opcode.qual_id, 
                            &op_code, &op_mask, SHR_BHH_OPCODE_LEN));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit,
                            hash_data->bhh_lm_entry_rx,
                            bcmFieldActionOamLmEnable, 1, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit, 
                            hash_data->bhh_lm_entry_rx,
                            bcmFieldActionOamLmepEnable, 0, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit,
                            hash_data->bhh_lm_entry_rx,
                            bcmFieldActionOamLmBasePtr,
                            hash_data->lm_counter_index, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit,
                            hash_data->bhh_lm_entry_rx,
                            bcmFieldActionOamServicePriMappingPtr,
                            hash_data->pri_map_index, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit,
                            hash_data->bhh_lm_entry_rx,
                            bcmFieldActionOamTx, 0, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit,
                            hash_data->bhh_lm_entry_rx,
                            bcmFieldActionOamLmDmSampleEnable, 1, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_entry_install(unit, 
                            hash_data->bhh_lm_entry_rx));
        if(hash_data->bhh_lm_entry_rx)
            oc->bhh_lmdm_entry_count++;

        /* Count the Rx packets */

        BCM_IF_ERROR_RETURN(bcm_esw_field_entry_create(unit,
                            oc->bhh_lmdm_group,
                            &hash_data->bhh_entry_pkt_rx));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_SrcPort(unit, 
                            hash_data->bhh_entry_pkt_rx,
                            0, 0x3f, gport, BCM_FIELD_EXACT_MATCH_MASK));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_OuterVlanId(unit, 
                            hash_data->bhh_entry_pkt_rx, 
                            hash_data->vlan, 0x0fff));

        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_data(unit, 
                            hash_data->bhh_entry_pkt_rx,
                            oc->bhh_qual_label.qual_id,
                            (uint8 *)&label, (uint8 *)&label_mask, 
                            SHR_BHH_MPLS_LABEL_LENGTH));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit, 
                            hash_data->bhh_entry_pkt_rx,
                            bcmFieldActionOamLmEnable, 1, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit, 
                            hash_data->bhh_entry_pkt_rx,
                            bcmFieldActionOamLmepEnable,
                            1, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit, 
                            hash_data->bhh_entry_pkt_rx,
                            bcmFieldActionOamLmBasePtr,
                            hash_data->lm_counter_index, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit,
                            hash_data->bhh_entry_pkt_rx,
                            bcmFieldActionOamServicePriMappingPtr,
                            hash_data->pri_map_index, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit,
                            hash_data->bhh_entry_pkt_rx,
                            bcmFieldActionOamTx, 0, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_entry_install(unit, 
                            hash_data->bhh_entry_pkt_rx));
        if(hash_data->bhh_entry_pkt_rx)
            oc->bhh_lmdm_entry_count++;

        /* Count the Tx packets */

        BCM_IF_ERROR_RETURN(bcm_esw_field_entry_create(unit,
                            oc->bhh_lmdm_group,
                            &hash_data->bhh_entry_pkt_tx));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_DstPort(unit, 
                            hash_data->bhh_entry_pkt_tx,
                            0, 0x3f, gport, BCM_FIELD_EXACT_MATCH_MASK));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_OuterVlanId(unit, 
                            hash_data->bhh_entry_pkt_tx,
                            hash_data->vlan, 0x0fff));
        BCM_IF_ERROR_RETURN(bcm_esw_field_qualify_DstL3Egress(unit, 
                            hash_data->bhh_entry_pkt_tx, endpoint_info->intf_id));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit, 
                            hash_data->bhh_entry_pkt_tx,
                            bcmFieldActionOamLmEnable, 1, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit,
                            hash_data->bhh_entry_pkt_tx,
                            bcmFieldActionOamLmepEnable, 1, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit, 
                            hash_data->bhh_entry_pkt_tx,
                            bcmFieldActionOamLmBasePtr,
                            hash_data->lm_counter_index, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit, 
                            hash_data->bhh_entry_pkt_tx,
                            bcmFieldActionOamServicePriMappingPtr,
                            hash_data->pri_map_index, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_action_add(unit, 
                            hash_data->bhh_entry_pkt_tx,
                            bcmFieldActionOamTx, 1, 0));
        BCM_IF_ERROR_RETURN(bcm_esw_field_entry_install(unit, 
                            hash_data->bhh_entry_pkt_tx));
        if(hash_data->bhh_entry_pkt_tx)
            oc->bhh_lmdm_entry_count++;

    }


    return rv;
}

/*
 * Function:
 *      _bcm_tr3_oam_bhh_encap_hw_set
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
_bcm_tr3_oam_bhh_encap_hw_set(int unit, _bcm_oam_hash_data_t *h_data_p,
                         bcm_module_t module_id, bcm_port_t port_id,
                         int is_local, bcm_oam_endpoint_info_t *endpoint_info)
{
    int rv = BCM_E_NONE;
    l2_entry_1_entry_t l2_entry_1;
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
    bcm_l3_egress_t l3_egress;
    int ingress_if;
    int i;
    int num_entries;
    mpls_entry_1_entry_t mpls_entry;
    mpls_entry_1_entry_t mpls_key;
    int cc_type = 0;
    int cw_check = 0; /* NO_CW */
    int mpls_index = 0;
    int nhi = 0;
#endif /* BCM_TRIUMPH_SUPPORT &&  BCM_MPLS_SUPPORT */

    switch(h_data_p->type) {
    case bcmOAMEndpointTypeBHHMPLS:
    case bcmOAMEndpointTypeBHHMPLSVccv:

#if defined(BCM_TRIUMPH3_SUPPORT) && defined(BCM_MPLS_SUPPORT)

        /* Insert mpls label field to L2_ENTRY table. */
        sal_memset(&l2_entry_1, 0, sizeof(l2_entry_1));
        
        soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_entry_1, KEY_TYPEf, 12);
        
        soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_entry_1, 
                                              BFD__SESSION_IDENTIFIER_TYPEf, 1);
       
        soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_entry_1, BFD__LABELf,
                                                    h_data_p->label);
        soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_entry_1, VALIDf, 1);

        if (!is_local) {
            /* Set BHH_REMOTE = 1, DST_MOD, DST_PORT */
            soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_entry_1, BFD__REMOTEf, 1);
            soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_entry_1, BFD__MODULE_IDf,
                                module_id); 
            soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_entry_1, BFD__DGPPf,
                                port_id);
            soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_entry_1, BFD__INT_PRIf,
                                endpoint_info->int_pri);
        } else {
#if 0

            soc_mem_field32_set(unit, L2Xm, &l2_entry, BFD_RX_SESSION_INDEXf,
                                h_data_p->bhh_endpoint_index);
#endif
            soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_entry_1, BFD__INT_PRIf,
                                h_data_p->cpu_qid); 
        }

        soc_mem_insert(unit, L2_ENTRY_1m, MEM_BLOCK_ANY, &l2_entry_1);

        SOC_IF_ERROR_RETURN(bcm_tr_mpls_lock (unit));

        sal_memset(&mpls_key, 0, sizeof(mpls_key));
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_key, MPLS__MPLS_LABELf,
                                    h_data_p->label);
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_key, KEY_TYPEf, 16);
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_key, VALIDf, 1);

        SOC_IF_ERROR_RETURN(soc_mem_search(unit, MPLS_ENTRYm,
                                           MEM_BLOCK_ANY, &mpls_index,
                                           &mpls_key, &mpls_entry, 0));

        if ((soc_MPLS_ENTRYm_field32_get(unit, &mpls_entry,
                                         VALIDf) != 0x1)) {
            bcm_tr_mpls_unlock (unit);
            return (BCM_E_PARAM);
        }

        if (h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {
            if (h_data_p->vccv_type == bcmOamBhhVccvChannelAch) {
                cc_type = 1;
                cw_check = 1; /* CW_NO_CHECK */
            } else if (h_data_p->vccv_type == bcmOamBhhVccvRouterAlert) {
                cc_type = 2;
                cw_check = 1; /* CW_NO_CHECK */
            } else if (h_data_p->vccv_type == bcmOamBhhVccvTtl) {
                cc_type = 3;
                cw_check = 1; /* CW_NO_CHECK */
            }    
        }

        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry,
                                    MPLS__SESSION_IDENTIFIER_TYPEf, 1);
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry, MPLS__BFD_ENABLEf, 1);
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry, MPLS__PW_CC_TYPEf, cc_type);
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry, MPLS__CW_CHECK_CTRLf, cw_check);
        SOC_IF_ERROR_RETURN(soc_mem_write(unit, MPLS_ENTRYm,
                                          MEM_BLOCK_ANY, mpls_index,
                                          &mpls_entry));
         /*
         * PW CC-3
         *
         * Set MPLS entry DECAP_USE_TTL=0 for corresponding
         * Tunnel Terminator label.
         */
        if (h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {
            if (h_data_p->vccv_type == bcmOamBhhVccvTtl) {
                BCM_IF_ERROR_RETURN
                    (bcm_esw_l3_egress_get(unit, h_data_p->egress_if,
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
                                              MPLS__MPLS_ACTION_IF_BOSf) == 0x1) {
                        continue;    /* L2_SVP */
                    }

                    ingress_if = soc_MPLS_ENTRYm_field32_get(unit, &mpls_entry,
                                                             MPLS__L3_IIFf);
                    if (ingress_if == l3_egress.intf) {
                        /* Label found */
                        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry,
                                                    MPLS__DECAP_USE_TTLf, 0);
                        SOC_IF_ERROR_RETURN(soc_mem_write(unit, MPLS_ENTRYm,
                                                          MEM_BLOCK_ALL, i,
                                                          &mpls_entry));
                        break;
                    }
                }
            }
        }
        bcm_tr_mpls_unlock (unit);
#endif /* BCM_TRIUMPH3_SUPPORT &&  BCM_MPLS_SUPPORT */
        break;

    default:
        rv = BCM_E_UNAVAIL;
        break;
    }

    
    if((h_data_p->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT) ||
        (h_data_p->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT))
    {
        rv = _bcm_tr3_oam_bhh_fp_set(unit, h_data_p, nhi, endpoint_info);
    }

    return (rv);
}

/*
 * Function:
 *      _bcm_tr3_oam_bhh_encap_data_dump
 * Purpose:
 *      Dumps buffer contents.
 * Parameters:
 *      buffer  - (IN) Buffer to dump data.
 *      length  - (IN) Length of buffer.
 * Returns:
 *      None
 */
void
_bcm_tr3_oam_bhh_encap_data_dump(uint8 *buffer, int length)
{
    int i;

    soc_cm_print("\nBHH encapsulation (length=%d):\n", length);

    for (i = 0; i < length; i++) {
        if ((i % 16) == 0) {
            soc_cm_print("\n");
        }
        soc_cm_print(" %02x", buffer[i]);
    }

    soc_cm_print("\n");
    return;
}

STATIC int
_bcm_tr3_oam_bhh_ach_header_get(uint32 packet_flags, _ach_header_t *ach)
{
    sal_memset(ach, 0, sizeof(*ach));

    ach->f_nibble = SHR_BHH_ACH_FIRST_NIBBLE;
    ach->version  = SHR_BHH_ACH_VERSION;
    ach->reserved = 0;

    ach->channel_type = SHR_BHH_ACH_CHANNEL_TYPE;

    return (BCM_E_NONE);
}

STATIC int
_bcm_tr3_oam_bhh_mpls_label_get(uint32 label, uint8 exp, uint8 s, uint8 ttl,
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

    return (BCM_E_NONE);
}

STATIC int
_bcm_tr3_oam_bhh_mpls_gal_label_get(_mpls_label_t *mpls)
{
    return _bcm_tr3_oam_bhh_mpls_label_get(SHR_BHH_MPLS_GAL_LABEL,
                                      0, 1, 1, mpls);
}

STATIC int
_bcm_tr3_oam_bhh_mpls_router_alert_label_get(_mpls_label_t *mpls)
{
    return _bcm_tr3_oam_bhh_mpls_label_get(SHR_BHH_MPLS_ROUTER_ALERT_LABEL,
                                      0, 0, 0, mpls);
}

STATIC int
_bcm_tr3_oam_bhh_mpls_labels_get(int unit, _bcm_oam_hash_data_t *h_data_p,
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
    if (BCM_FAILURE(bcm_esw_l3_egress_get(unit, h_data_p->egress_if, &l3_egress))) {
        return (BCM_E_PARAM);
    }

    /* Look for a tunnel associated with this interface */
    if (BCM_SUCCESS
        (bcm_esw_mpls_tunnel_initiator_get(unit, l3_egress.intf,
                                           _BHH_MPLS_MAX_LABELS,
                                           label_array, &label_count))) {
        for (i = 0; (i < label_count) && (count < max_count); i++) {
            _bcm_tr3_oam_bhh_mpls_label_get(label_array[i].label,
                                       label_array[i].exp,
                                       0,
                                       label_array[i].ttl,
                                       &mpls[count++]);
        }
    }

        /* MPLS Router Alert */
    if (packet_flags & _BHH_ENCAP_PKT_MPLS_ROUTER_ALERT) {
        _bcm_tr3_oam_bhh_mpls_router_alert_label_get(&mpls[count++]);
    }

    /* Use GPORT to resolve interface */
    if (BCM_GPORT_IS_MPLS_PORT(h_data_p->gport)) {
        /* Get mpls port and label info */
        bcm_mpls_port_t_init(&mpls_port);
        mpls_port.mpls_port_id = h_data_p->gport;
        if (BCM_FAILURE
            (bcm_esw_mpls_port_get(unit, h_data_p->vpn,
                                   &mpls_port))) {
            return (BCM_E_PARAM);
        } else {
            if (h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv &&
                h_data_p->vccv_type == bcmOamBhhVccvTtl) {
                mpls_port.egress_label.ttl = 0x1;
            }

            _bcm_tr3_oam_bhh_mpls_label_get(mpls_port.egress_label.label,
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

    return (BCM_E_NONE);

}

STATIC int
_bcm_tr3_oam_bhh_l2_header_get(int unit, _bcm_oam_hash_data_t *h_data_p,
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
        (bcm_esw_l3_egress_get(unit, h_data_p->egress_if, &l3_egress))) {
        return (BCM_E_PARAM);
    }

    l3_intf.l3a_intf_id = l3_egress.intf;
    if (BCM_FAILURE(bcm_esw_l3_intf_get(unit, &l3_intf))) {
        return (BCM_E_PARAM);
    }

    /* Get TPID */
    if (BCM_FAILURE(bcm_esw_port_tpid_get(unit, port, &tpid))) {
        return (BCM_E_INTERNAL);
    }

    sal_memcpy(l2->dst_mac, l3_egress.mac_addr, _BHH_MAC_ADDR_LENGTH);
    sal_memcpy(l2->src_mac, l3_intf.l3a_mac_addr, _BHH_MAC_ADDR_LENGTH);
    l2->vlan_tag.tpid     = tpid;
    l2->vlan_tag.tci.prio = h_data_p->vlan_pri;
    l2->vlan_tag.tci.cfi  = 0;
    l2->vlan_tag.tci.vid  = l3_intf.l3a_vid;
    if (BCM_SUCCESS(bcm_esw_port_untagged_vlan_get(unit, port, &vid))) {
        if (vid == l2->vlan_tag.tci.vid) {
            l2->vlan_tag.tpid = 0;  /* Set to 0 to indicate untagged */
        }
    }
    l2->etype             = etype;

    return (BCM_E_NONE);
}

STATIC uint8 *
_bcm_tr3_oam_bhh_ach_header_pack(uint8 *buffer, _ach_header_t *ach)
{
    uint32  tmp;

    tmp = ((ach->f_nibble & 0xf) << 28) | ((ach->version & 0xf) << 24) |
        (ach->reserved << 16) | ach->channel_type;

    _BHH_ENCAP_PACK_U32(buffer, tmp);

    return (buffer);
}

STATIC uint8 *
_bcm_tr3_oam_bhh_mpls_label_pack(uint8 *buffer, _mpls_label_t *mpls)
{
    uint32  tmp;

    tmp = ((mpls->label & 0xfffff) << 12) | ((mpls->exp & 0x7) << 9) |
        ((mpls->s & 0x1) << 8) | mpls->ttl;
    _BHH_ENCAP_PACK_U32(buffer, tmp);

    return (buffer);
}

STATIC uint8 *
_bcm_tr3_oam_bhh_l2_header_pack(uint8 *buffer, _l2_header_t *l2)
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

    return (buffer);
}

/*
 * Function:
 *      _bcm_tr3_oam_bhh_encap_build_pack
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
_bcm_tr3_oam_bhh_encap_build_pack(int unit, bcm_port_t port,
                             _bcm_oam_hash_data_t *h_data_p,
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
            (_bcm_tr3_oam_bhh_ach_header_get(packet_flags, &ach));
    }

    if (packet_flags & _BHH_ENCAP_PKT_GAL) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_oam_bhh_mpls_gal_label_get(&mpls_gal));
    }

    if (packet_flags & _BHH_ENCAP_PKT_MPLS_ROUTER_ALERT) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_oam_bhh_mpls_router_alert_label_get(&mpls_r_alert));
    }

    if (packet_flags & _BHH_ENCAP_PKT_MPLS) {
        etype = SHR_BHH_L2_ETYPE_MPLS_UCAST;
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_oam_bhh_mpls_labels_get(unit, h_data_p,
                                             packet_flags,    
                                             _BHH_MPLS_MAX_LABELS,
                                             &pw_label,
                                             mpls_labels, 
                                             &mpls_count,
                                             &l3_intf_id));
    }

    /* Always build L2 Header */
    BCM_IF_ERROR_RETURN
        (_bcm_tr3_oam_bhh_l2_header_get(unit, 
                                       h_data_p,
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
    cur_ptr = _bcm_tr3_oam_bhh_l2_header_pack(cur_ptr, &l2);

    if (packet_flags & _BHH_ENCAP_PKT_MPLS) {
        for (i = 0;i < mpls_count;i++) {
            cur_ptr = _bcm_tr3_oam_bhh_mpls_label_pack(cur_ptr, &mpls_labels[i]);
        }
    }

    if (packet_flags & _BHH_ENCAP_PKT_GAL) {    
        cur_ptr = _bcm_tr3_oam_bhh_mpls_label_pack(cur_ptr, &mpls_gal);
    }

    if (packet_flags & _BHH_ENCAP_PKT_G_ACH) {
        cur_ptr = _bcm_tr3_oam_bhh_ach_header_pack(cur_ptr, &ach);
    }


    /* Set BHH encapsulation length */
    *encap_length = cur_ptr - buffer;

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_tr3_oam_bhh_encap_create
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
_bcm_tr3_oam_bhh_encap_create(int unit, bcm_port_t port_id,
                         _bcm_oam_hash_data_t *h_data_p,
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
    switch (h_data_p->type) {
    case bcmOAMEndpointTypeBHHMPLS:
        packet_flags |=
            (_BHH_ENCAP_PKT_MPLS |
             _BHH_ENCAP_PKT_GAL |
             _BHH_ENCAP_PKT_G_ACH);
        break;

    case bcmOAMEndpointTypeBHHMPLSVccv:
        switch(h_data_p->vccv_type) {

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
        return (BCM_E_PARAM);
        break;
    }
        break;

    default:
        return (BCM_E_PARAM);
    }

    /* Build header/labels and pack in buffer */
    BCM_IF_ERROR_RETURN
        (_bcm_tr3_oam_bhh_encap_build_pack(unit, port_id,
                                      h_data_p,
                                      packet_flags,
                                      encap_data,
                                  encap_length));

    /* Set encap type (indicates uC side that checksum is required) */
    *encap_type = SHR_BHH_ENCAP_TYPE_RAW;
                    
#ifdef _BHH_DEBUG_DUMP
    _bcm_tr3_oam_bhh_encap_data_dump(encap_data, *encap_length);
#endif

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_tr3_oam_bhh_endpoint_create
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
int
bcm_tr3_oam_bhh_endpoint_create(int unit, 
                                bcm_oam_endpoint_info_t *endpoint_info,
                                _bcm_oam_hash_key_t  *hash_key)
{
    _bcm_oam_control_t *oc;
    _bcm_oam_hash_data_t *hash_data = NULL; /* Endpoint hash data pointer.  */
    _bcm_oam_group_data_t *group_p;  /* Pointer to group data.            */
    int                  ep_req_index;      /* Requested endpoint index.    */
    int                  rv = BCM_E_NONE;   /* Operation return status.     */
    int                  is_remote = 0;        /* Remote endpoint status.   */
    int                  is_replace;
    int                  is_local = 0;         
    uint32               sglp = 0;          /* Source global logical port.  */
    uint32               dglp = 0;          /* Dest global logical port.    */
    bcm_module_t         module_id;         /* Module ID                    */
    bcm_port_t           port_id;
    bcm_trunk_t          trunk_id;
    int                  local_id;
    shr_bhh_msg_ctrl_sess_set_t msg_sess;
    shr_bhh_msg_ctrl_sess_enable_t msg_sess_enable;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int encap = 0;
    uint32 session_flags;
    int bhh_pool_ep_idx = 0;

    OAM_VVERB(("BHH(unit %d) Info: bcm_tr3_oam_bhh_endpoint_create"
              "Endpoint ID=%d.\n", unit, endpoint_info->id));

    _BCM_OAM_BHH_IS_VALID(unit);

    /* Validate input parameter. */
    if (NULL == endpoint_info) {
        return (BCM_E_PARAM);
    }

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Get the Trunk, Port and Modid info for this Gport */
    rv = _bcm_esw_gport_resolve(unit,
            endpoint_info->gport, &module_id, &port_id, &trunk_id, &local_id);
    if (BCM_FAILURE(rv)) {
        OAM_ERR(("BHH(unit %d) Error: Gport resolve (EP=%d) - %s.\n",
                unit, endpoint_info->id, bcm_errmsg(rv)));
        return (rv);
    }

    /* Get MEP remote endpoint status. */
    is_remote = (endpoint_info->flags & BCM_OAM_ENDPOINT_REMOTE) ? 1 : 0;

    is_replace = ((endpoint_info->flags & BCM_OAM_ENDPOINT_REPLACE) != 0);

    if(((is_replace) || (endpoint_info->flags & BCM_OAM_ENDPOINT_WITH_ID)) &&
       (endpoint_info->id < _BCM_OAM_BHH_ENDPOINT_OFFSET)) {
        return (BCM_E_PARAM);
    }

    if((is_remote) && (endpoint_info->local_id < _BCM_OAM_BHH_ENDPOINT_OFFSET)) {
        return (BCM_E_PARAM);
    }

    /* Create a new endpoint with the requested ID. */
    if (endpoint_info->flags & BCM_OAM_ENDPOINT_WITH_ID) {
        hash_data = &oc->oam_hash_data[endpoint_info->id];

        if (is_replace && !hash_data->in_use)
        {
            return (BCM_E_NOT_FOUND);
        }
        else if (!is_replace && hash_data->in_use)
        {
            return (BCM_E_EXISTS);
        }

        ep_req_index = endpoint_info->id;
        bhh_pool_ep_idx = BCM_OAM_BHH_GET_UKERNEL_EP(ep_req_index);

        if((ep_req_index < _BCM_OAM_BHH_ENDPOINT_OFFSET) ||
           (ep_req_index >= (_BCM_OAM_BHH_ENDPOINT_OFFSET + oc->bhh_endpoint_count))) {
            rv = BCM_E_PARAM;
        }

        if(BCM_FAILURE(rv)) {
            OAM_ERR(("BHH(unit %d) Error: Endpoint check (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
            return (rv);
        }

        if(!is_replace) {
            rv = shr_idxres_list_reserve(oc->bhh_pool, bhh_pool_ep_idx,
                                     bhh_pool_ep_idx);
            if (BCM_FAILURE(rv)) {
                rv = (rv == BCM_E_RESOURCE) ? (BCM_E_EXISTS) : rv;
                OAM_ERR(("BHH(unit %d) Error: Endpoint reserve (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
                return (rv);
            }
        }
    } else {

        if (is_replace)
        {
            /* Replace specified with no ID */

            return (BCM_E_PARAM);
        }

        /* BHH uses local and remote same index */
        if(endpoint_info->flags & BCM_OAM_ENDPOINT_REMOTE) {
            ep_req_index = endpoint_info->local_id;
            bhh_pool_ep_idx = BCM_OAM_BHH_GET_UKERNEL_EP(ep_req_index);
        }
        else {
            /* Allocate the next available endpoint index. */
            rv = shr_idxres_list_alloc(oc->bhh_pool,
                                   (shr_idxres_element_t *)&bhh_pool_ep_idx);
            if (BCM_FAILURE(rv)) {
                rv = (rv == BCM_E_RESOURCE) ? (BCM_E_FULL) : rv;
                OAM_ERR(("BHH(unit %d) Error: Endpoint alloc failed - %s.\n",
                    unit, bcm_errmsg(rv)));
                return (rv);
            }
            /* Set the allocated endpoint id value. */
            ep_req_index = BCM_OAM_BHH_GET_SDK_EP(bhh_pool_ep_idx);
        }
        endpoint_info->id =  ep_req_index;
    }

    /* Get the hash data pointer where the data is to be stored. */
    hash_data = &oc->oam_hash_data[ep_req_index];
    group_p = &oc->group_info[endpoint_info->group];

    /*
     *The uKernel is not provisioned until both endpoints (local and remote)
     * are provisioned in the host.
     */
    if (is_remote) {

        if (!hash_data->in_use) {
            return (BCM_E_NOT_FOUND);
        } else if (hash_data->flags & BCM_OAM_ENDPOINT_REMOTE) {
            return (BCM_E_EXISTS);
        }

        /*
         * Now that both ends are provisioned the uKernel can be 
         * configured.
         */
        msg_sess_enable.sess_id = bhh_pool_ep_idx;
        msg_sess_enable.flags = 0;
        msg_sess_enable.enable = 1;
        msg_sess_enable.remote_mep_id = endpoint_info->name;

        /* Pack control message data into DMA buffer */
        buffer     = oc->dma_buffer;
        buffer_ptr = shr_bhh_msg_ctrl_sess_enable_pack(buffer, &msg_sess_enable);
        buffer_len = buffer_ptr - buffer;

        /* Send BHH Session Update message to uC */
        rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                          MOS_MSG_SUBCLASS_BHH_SESS_ENABLE,
                          buffer_len, 0,
                          MOS_MSG_SUBCLASS_BHH_SESS_ENABLE_REPLY,
                          &reply_len);
        if (BCM_FAILURE(rv) || (reply_len != 0)) {
            OAM_ERR(("BHH(unit %d) Error: Endpoint destroy (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
            return (BCM_E_INTERNAL);
        }
    } else {

        /* Clear the hash data element contents before storing the values. */
        _BCM_OAM_HASH_DATA_CLEAR(hash_data);
        hash_data->type = endpoint_info->type;
        hash_data->ep_id = endpoint_info->id;
        hash_data->group_index = endpoint_info->group;
        hash_data->level = endpoint_info->level;
        hash_data->vlan = endpoint_info->vlan;
        hash_data->gport = endpoint_info->gport;
        hash_data->sglp = sglp;
        hash_data->dglp = dglp;
        hash_data->flags = endpoint_info->flags;
        hash_data->period = endpoint_info->ccm_period;
        hash_data->vccv_type = endpoint_info->vccv_type;
        hash_data->vpn  = endpoint_info->vpn;
        hash_data->name      = endpoint_info->name;
        hash_data->egress_if = endpoint_info->intf_id;
        hash_data->cpu_qid   = endpoint_info->cpu_qid;
        hash_data->label     = endpoint_info->mpls_label;
        hash_data->gport     = endpoint_info->gport;
        hash_data->flags     = endpoint_info->flags;
        hash_data->local_tx_enabled = 0;
        hash_data->local_rx_enabled = 0;

        /* Initialize hardware index as invalid indices. */
        hash_data->local_tx_index = _BCM_OAM_INVALID_INDEX;
        hash_data->local_rx_index = _BCM_OAM_INVALID_INDEX;
        hash_data->remote_index = _BCM_OAM_INVALID_INDEX;

#if 0
    /* hash collision!!! Is this needed for BHH? */
        rv = shr_htb_insert(oc->ma_mep_htbl, hash_key, hash_data);
        if (BCM_FAILURE(rv)) {
            OAM_ERR(("BHH(unit %d) Error: Hash table insert failed EP=%d %s.\n",
                unit, endpoint_info->id, bcm_errmsg(rv)));
            return (rv);
        }
#endif


        /* Get local port used for TX BFD packet */
        rv = _bcm_esw_modid_is_local(unit, module_id, &is_local);
        if(BCM_FAILURE(rv)) {
            /* Return ID back to free MEP ID pool.*/
            BCM_IF_ERROR_RETURN(shr_idxres_list_free(oc->bhh_pool, bhh_pool_ep_idx));
            OAM_ERR(("BHH(unit %d) Error: Endpoint destroy (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
            return (rv);
        }

        if (!is_local) {    /* HG port */
            rv = bcm_esw_stk_modport_get(unit, module_id, &port_id);
            if(BCM_FAILURE(rv)) {
                /* Return ID back to free MEP ID pool.*/
                BCM_IF_ERROR_RETURN(shr_idxres_list_free(oc->bhh_pool, bhh_pool_ep_idx));
                OAM_ERR(("BHH(unit %d) Error: Endpoint destroy (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
                return (rv);
            }
        }

        /* Create or Update */
        session_flags = (is_replace) ? 0 : SHR_BHH_SESS_SET_F_CREATE;

        /* Set Endpoint config entry */
        hash_data->bhh_endpoint_index = ep_req_index;

        /* Set Encapsulation in HW */
        if (!is_replace || (endpoint_info->flags & BCM_BHH_ENDPOINT_ENCAP_SET)) {
            rv = _bcm_tr3_oam_bhh_encap_hw_set(unit, hash_data, module_id,
                          port_id, is_local, endpoint_info);
            if(BCM_FAILURE(rv)) {
                /* Return ID back to free MEP ID pool.*/
                BCM_IF_ERROR_RETURN(shr_idxres_list_free(oc->bhh_pool, bhh_pool_ep_idx));
                OAM_ERR(("BHH(unit %d) Error: Endpoint destroy (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
                return (rv);
            }
            encap = 1;
        }

        /* Set control message data */
        sal_memset(&msg_sess, 0, sizeof(msg_sess));

        /*
         * Set the BHH encapsulation data
         *
         * The function _bcm_tr3_oam_bhh_encap_create() is called first
         * since this sets some fields in 'hash_data' which are
         * used in the message.
         */
        if (encap) {
            rv = _bcm_tr3_oam_bhh_encap_create(unit, 
                          port_id, 
                          hash_data,
                          msg_sess.encap_data,
                          &msg_sess.encap_type,
                          &msg_sess.encap_length);

            if(BCM_FAILURE(rv))
            {
                /* Return ID back to free MEP ID pool.*/
                BCM_IF_ERROR_RETURN(shr_idxres_list_free(oc->bhh_pool, bhh_pool_ep_idx));
                OAM_ERR(("BHH(unit %d) Error: Endpoint destroy (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
                return (rv);
            }
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

/*
        if (!local_tx_enabled)
            session_flags |= SHR_BHH_SESS_SET_F_PASSIVE;*/

        msg_sess.sess_id = bhh_pool_ep_idx;
        msg_sess.flags   = session_flags;

        if (hash_data->level > _BCM_OAM_BHH_MEL_MAX) {
            /* Return ID back to free MEP ID pool.*/
            BCM_IF_ERROR_RETURN(shr_idxres_list_free(oc->bhh_pool, bhh_pool_ep_idx));
            rv = BCM_E_PARAM;
            OAM_ERR(("BHH(unit %d) Error: Endpoint destroy (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
            return (rv);
        }

        msg_sess.mel     = hash_data->level;
        msg_sess.mep_id  = hash_data->name;
        sal_memcpy(msg_sess.meg_id, group_p->name, BCM_OAM_GROUP_NAME_LENGTH);

        msg_sess.period = _tr3_ccm_intervals[
                    _bcm_tr3_oam_ccm_msecs_to_hw_encode(endpoint_info->ccm_period)];

        msg_sess.if_num  = endpoint_info->intf_id;
        msg_sess.tx_port = port_id;
        msg_sess.tx_cos  = endpoint_info->int_pri;
        msg_sess.tx_pri  = endpoint_info->pkt_pri;
        msg_sess.tx_qnum = SOC_INFO(unit).port_uc_cosq_base[port_id] + endpoint_info->int_pri;
        msg_sess.lm_counter_index 
                  = hash_data->lm_counter_index | _BCM_OAM_LM_COUNTER_TX_OFFSET;

        msg_sess.mpls_label = endpoint_info->mpls_label;

        /* Pack control message data into DMA buffer */
        buffer     = oc->dma_buffer;
        buffer_ptr = shr_bhh_msg_ctrl_sess_set_pack(buffer, &msg_sess);
        buffer_len = buffer_ptr - buffer;

        /* Send BHH Session Update message to uC */
        rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                          MOS_MSG_SUBCLASS_BHH_SESS_SET,
                          buffer_len, 0,
                          MOS_MSG_SUBCLASS_BHH_SESS_SET_REPLY,
                          &reply_len);
        if (BCM_FAILURE(rv) || (reply_len != 0)) {
            /* Return ID back to free MEP ID pool.*/
            BCM_IF_ERROR_RETURN(shr_idxres_list_free(oc->bhh_pool, bhh_pool_ep_idx));
            OAM_ERR(("BHH(unit %d) Error: Endpoint destroy (EP=%d) - %s.\n",
                    unit, endpoint_info->id, bcm_errmsg(rv)));
            return (rv);
        }

        rv = _bcm_oam_group_ep_list_add(unit, endpoint_info->group,
                                    endpoint_info->id);
        if (BCM_FAILURE(rv)) {
            /* Return ID back to free MEP ID pool.*/
            BCM_IF_ERROR_RETURN(shr_idxres_list_free(oc->bhh_pool, bhh_pool_ep_idx));
            OAM_ERR(("OAM(unit %d) Error: Tx config failed for EP=%d %s.\n",
            unit, endpoint_info->id, bcm_errmsg(rv)));
            return (rv);
        }
    } 
    hash_data->in_use = 1;

    return (rv);
}

/*
 * Function:
 *      _bcm_tr3_oam_bhh_fp_init
 * Purpose:
 *      Initialize the fp for BHH LM/DM packet processing.
 *      Configure:
 *      - Qualifiers QSET
 *      - Create Groups
 * Parameters:
 *      unit - (IN) Unit number.
 *      oc   - (IN) OAM control structure.    
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
_bcm_tr3_oam_bhh_fp_init(int unit)
{
    int rv = BCM_E_NONE;
    _bcm_oam_control_t   *oc;            /* OAM control structure.        */

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    bcm_field_data_packet_format_t_init(&oc->bhh_pkt_fmt);
    oc->bhh_pkt_fmt.relative_offset = 0;
    oc->bhh_pkt_fmt.l2              = BCM_FIELD_DATA_FORMAT_L2_ANY;
    oc->bhh_pkt_fmt.vlan_tag        = BCM_FIELD_DATA_FORMAT_VLAN_TAG_ANY;
    oc->bhh_pkt_fmt.outer_ip        = BCM_FIELD_DATA_FORMAT_IP_NONE;
    oc->bhh_pkt_fmt.inner_ip        = BCM_FIELD_DATA_FORMAT_IP_NONE;
    oc->bhh_pkt_fmt.tunnel          = BCM_FIELD_DATA_FORMAT_TUNNEL_MPLS;
    oc->bhh_pkt_fmt.mpls            = BCM_FIELD_DATA_FORMAT_MPLS_ANY;

    /* match mpls label */
    bcm_field_data_qualifier_t_init(&oc->bhh_qual_label);
    oc->bhh_qual_label.offset_base = bcmFieldDataOffsetBaseOuterL3Header;
    oc->bhh_qual_label.offset      = 0;
    oc->bhh_qual_label.length      = SHR_BHH_MPLS_LABEL_LENGTH;
    BCM_IF_ERROR_RETURN(bcm_esw_field_data_qualifier_create(unit, 
                                                          &oc->bhh_qual_label));
    BCM_IF_ERROR_RETURN(bcm_esw_field_data_qualifier_packet_format_add(unit, 
                                 oc->bhh_qual_label.qual_id, &oc->bhh_pkt_fmt));

    /* match ACH channel type 0x8902 */
    bcm_field_data_qualifier_t_init(&oc->bhh_qual_ach);
    oc->bhh_qual_ach.offset_base = bcmFieldDataOffsetBaseOuterL3Header;
    oc->bhh_qual_ach.offset      = 10;
    oc->bhh_qual_ach.length      = SHR_BHH_ACH_TYPE_LENGTH;
    BCM_IF_ERROR_RETURN(bcm_esw_field_data_qualifier_create(unit, &oc->bhh_qual_ach));
    BCM_IF_ERROR_RETURN(bcm_esw_field_data_qualifier_packet_format_add(unit, 
                                     oc->bhh_qual_ach.qual_id, &oc->bhh_pkt_fmt));

    /* match opcode 45 or 46 or 47 */
    bcm_field_data_qualifier_t_init(&oc->bhh_qual_opcode);
    oc->bhh_qual_opcode.offset_base = bcmFieldDataOffsetBaseOuterL3Header;
    oc->bhh_qual_opcode.offset      = 13;
    oc->bhh_qual_opcode.length      = SHR_BHH_OPCODE_LEN;
    BCM_IF_ERROR_RETURN(bcm_esw_field_data_qualifier_create(unit, &oc->bhh_qual_opcode));
    BCM_IF_ERROR_RETURN(bcm_esw_field_data_qualifier_packet_format_add(unit, 
                                     oc->bhh_qual_opcode.qual_id, &oc->bhh_pkt_fmt));

    BCM_FIELD_QSET_INIT(oc->bhh_lmdm_qset);
    BCM_FIELD_QSET_ADD(oc->bhh_lmdm_qset, bcmFieldQualifySrcPort);
    BCM_FIELD_QSET_ADD(oc->bhh_lmdm_qset, bcmFieldQualifyDstPort);
    BCM_FIELD_QSET_ADD(oc->bhh_lmdm_qset, bcmFieldQualifyOuterVlanId);
    BCM_FIELD_QSET_ADD(oc->bhh_lmdm_qset, bcmFieldQualifyDstL3Egress);
    BCM_IF_ERROR_RETURN(bcm_esw_field_qset_data_qualifier_add(unit, 
                        &oc->bhh_lmdm_qset, 
                        oc->bhh_qual_label.qual_id));
    BCM_IF_ERROR_RETURN(bcm_esw_field_qset_data_qualifier_add(unit, 
                        &oc->bhh_lmdm_qset, 
                        oc->bhh_qual_ach.qual_id));
    BCM_IF_ERROR_RETURN(bcm_esw_field_qset_data_qualifier_add(unit, 
                        &oc->bhh_lmdm_qset, 
                        oc->bhh_qual_opcode.qual_id));

    return rv;
}

/*
 * Function:
 *      _bcm_tr3_oam_bhh_hw_init
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
_bcm_tr3_oam_bhh_hw_init(int unit)
{
    int rv = BCM_E_NONE;
    _bcm_oam_control_t   *oc;               /* OAM control structure.        */
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
    int num_cosq = 0;
    int min_cosq, max_cosq;
    int i;

    /* Get OAM Control Structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    /*
     * Send BHH error packet to CPU
     *
     * Configure CPU_CONTROL_0 register
     */
    rv = READ_CPU_CONTROL_0r(unit, &val);
    if(BCM_FAILURE(rv)) {
        OAM_ERR(("BHH(unit %d) Error:hw init. Read CPU Control Reg %s.\n",
                unit, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                      BFD_UNKNOWN_VERSION_TOCPUf, 1);
    soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                      BFD_YOUR_DISCRIMINATOR_NOT_FOUND_TOCPUf, 1);
    soc_reg_field_set(unit, CPU_CONTROL_0r, &val,
                      BFD_UNKNOWN_CONTROL_PACKET_TOCPUf, 1);

    rv = WRITE_CPU_CONTROL_0r(unit, val);
    if(BCM_FAILURE(rv)) {
        OAM_ERR(("BHH(unit %d) Error:hw init. Write CPU Control_0 Reg %s.\n",
                unit, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    /*
     * Configure CPU_CONTROL_M register
     */

    rv = READ_CPU_CONTROL_Mr(unit, &val);
    if(BCM_FAILURE(rv)) {
        OAM_ERR(("BHH(unit %d) Error:hw init. Read CPU Control Reg %s.\n",
                unit, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }
    soc_reg_field_set(unit, CPU_CONTROL_Mr, &val, 
                        MPLS_UNKNOWN_ACH_TYPE_TO_CPUf, 1); 
    soc_reg_field_set(unit, CPU_CONTROL_Mr, &val, 
                        MPLS_UNKNOWN_ACH_VERSION_TOCPUf, 1); 
    rv = WRITE_CPU_CONTROL_Mr(unit, val);
    if(BCM_FAILURE(rv)) {
        OAM_ERR(("BHH(unit %d) Error:hw init. Write CPU Control_m Reg %s.\n",
                unit, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    /* 
     * Get COSQ for BHH
     */
     min_cosq = 0;
     for (i = 0; i < SOC_CMCS_NUM(unit); i++) {
         num_cosq = NUM_CPU_ARM_COSQ(unit, i);
         if (i == oc->uc_num + 1) {
             break;
         }
         min_cosq += num_cosq;
     }
     max_cosq = min_cosq + num_cosq - 1;

     if(max_cosq < min_cosq) {
        _BCM_OAM_UNLOCK(oc);
         return (BCM_E_CONFIG);
     }

    /* check user configured COSq */
    if (oc->cpu_cosq != BHH_COSQ_INVALID) {
        if ((oc->cpu_cosq < min_cosq) ||
            (oc->cpu_cosq > max_cosq)) {
            _BCM_OAM_UNLOCK(oc);
            return (BCM_E_CONFIG);
        }
        min_cosq = max_cosq = oc->cpu_cosq;
    }

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
    chan_id = (BCM_RX_CHANNELS * (SOC_ARM_CMC(unit, oc->uc_num))) +
        oc->rx_channel;

    for (i = max_cosq; i >= min_cosq; i--) {
        rv = _bcm_common_rx_queue_channel_set(unit, i, chan_id);
        if(BCM_SUCCESS(rv)) {
            oc->cpu_cosq = i;
            break;
        }
    }

    if (i < min_cosq) {
        rv = BCM_E_RESOURCE;
        OAM_ERR(("BHH(unit %d) Error:hw init. queue channel set %s.\n",
                unit, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

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
    rv = bcm_esw_rx_cosq_mapping_size_get(unit, &cosq_map_size);
    if(BCM_FAILURE(rv)) {
        OAM_ERR(("BHH(unit %d) Error:hw init. cosq maps size %s.\n",
                unit, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }
    
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
            OAM_ERR(("BHH(unit %d) Error:hw init. cosq maps get %s.\n",
                unit, bcm_errmsg(rv)));
            _BCM_OAM_UNLOCK(oc);
            return (rv);
        }
    }

    if (ach_error_index == -1 || invalid_error_index == -1) {
        OAM_ERR(("BHH(unit %d) Error:hw init. ACH error %s.\n",
                unit, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (BCM_E_FULL);
    }

    /* Set CPU COS Queue mapping */
    BCM_RX_REASON_CLEAR_ALL(reasons);
#if 0
    BCM_RX_REASON_SET(reasons, bcmRxReasonBfd);  /* BFD Error */
#endif
    BCM_RX_REASON_SET(reasons, bcmRxReasonMplsUnknownAch); /* Despite the name this reason */
                                                              /* code covers Unknown ACH */

    rv = bcm_esw_rx_cosq_mapping_set(unit, ach_error_index,
                                     reasons, reasons,
                                     0, 0, /* Any priority */
                                     0, 0, /* Any packet type */
                                     oc->cpu_cosq);
    if(BCM_FAILURE(rv)) {
        OAM_ERR(("BHH(unit %d) Error:hw init. cosq map set %s.\n",
                unit, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }
    oc->cpu_cosq_ach_error_index = ach_error_index;


#if 0
    BCM_RX_REASON_CLEAR_ALL(reasons);
    BCM_RX_REASON_SET(reasons, bcmRxReasonBfd);  /* BFD Error */
    BCM_RX_REASON_SET(reasons, bcmRxReasonBfdInvalidPacket); 

    rv = bcm_esw_rx_cosq_mapping_set(unit, invalid_error_index,
                                     reasons, reasons,
                                     0, 0, /* Any priority */
                                     0, 0, /* Any packet type */
                                     oc->cpu_cosq);
    if(BCM_FAILURE(rv)) {
        OAM_ERR(("BHH(unit %d) Error:hw init. cosq map set %s.\n",
                unit, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }
    oc->cpu_cosq_invalid_error_index = invalid_error_index;
#endif

    rv = _bcm_tr3_oam_bhh_fp_init(unit);

    _BCM_OAM_UNLOCK(oc);
    return (rv);
}

/*
 * Function:
 *      _bcm_tr3_oam_bhh_find_l2_entry_1
 */
STATIC int
_bcm_tr3_oam_bhh_find_l2_entry_1(int unit, uint32 key, int key_type, int ses_type,
                   int *index, l2_entry_1_entry_t *l2_entry_1)
{
    l2_entry_1_entry_t l2_key;

    sal_memset(&l2_key, 0, sizeof(l2_key));

    soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_key, KEY_TYPEf, key_type);
    soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_key,
                        BFD__SESSION_IDENTIFIER_TYPEf, ses_type);

    if (ses_type) {
        soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_key, BFD__LABELf, key);
    } else {
        soc_mem_field32_set(unit, L2_ENTRY_1m, &l2_key,
                            BFD__YOUR_DISCRIMINATORf, key);
    }

    return soc_mem_search(unit, L2_ENTRY_1m, MEM_BLOCK_ANY, index,
                          &l2_key, l2_entry_1, 0);

}

/*
 * Function:
 *      _bcm_tr3_oam_bhh_fp_delete
 * Purpose:
 *      Sets BHH FP for LM/DM in HW device.
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
_bcm_tr3_oam_bhh_fp_delete(int unit, _bcm_oam_hash_data_t *h_data_p)
{
    int rv = BCM_E_NONE;
    _bcm_oam_control_t *oc;

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    if(h_data_p->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT) {
        if (h_data_p->bhh_entry_pkt_rx) {
            (void)bcm_esw_field_entry_destroy(unit, h_data_p->bhh_entry_pkt_rx);
            oc->bhh_lmdm_entry_count--;
        }
        if (h_data_p->bhh_entry_pkt_tx) {
            (void)bcm_esw_field_entry_destroy(unit, h_data_p->bhh_entry_pkt_tx);
            oc->bhh_lmdm_entry_count--;
        }
        if (h_data_p->bhh_lm_entry_rx) {
            (void)bcm_esw_field_entry_destroy(unit, h_data_p->bhh_lm_entry_rx);
            oc->bhh_lmdm_entry_count--;
        }
    }

    if(h_data_p->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT) {
        if (h_data_p->bhh_dm_entry_rx) {
            (void)bcm_esw_field_entry_destroy(unit, h_data_p->bhh_dm_entry_rx);
            oc->bhh_lmdm_entry_count--;
        }
    }

    if (h_data_p->lm_counter_index != _BCM_OAM_LM_COUNTER_IDX_INVALID) {
        /* Return ID back to free lm counter pool.*/
        BCM_IF_ERROR_RETURN(shr_idxres_list_free(oc->lm_counter_pool, 
                                                 h_data_p->lm_counter_index));
        h_data_p->lm_counter_index = _BCM_OAM_LM_COUNTER_IDX_INVALID;
        SOC_IF_ERROR_RETURN(soc_profile_mem_delete(unit, 
                &oc->ing_service_pri_map,
                (h_data_p->pri_map_index * BCM_OAM_INTPRI_MAX)));
    }

    if(oc->bhh_lmdm_entry_count == 0) {
        (void)bcm_esw_field_group_destroy(unit, oc->bhh_lmdm_group);
        oc->bhh_lmdm_group = 0;
    }

    h_data_p->bhh_entry_pkt_tx = 0;
    h_data_p->bhh_entry_pkt_rx = 0;
    h_data_p->bhh_lm_entry_rx = 0;
    h_data_p->bhh_dm_entry_rx = 0;

    return rv;
}


/*
 * Function:
 *      _bcm_tr3_oam_bhh_session_hw_delete
 * Purpose:
 *      Delete BHH Session in HW device.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      h_data_p- (IN) Pointer to BHH endpoint structure.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
_bcm_tr3_oam_bhh_session_hw_delete(int unit, _bcm_oam_hash_data_t *h_data_p)
{
    int rv = BCM_E_NONE;
    int l2_index = 0;
    l2_entry_1_entry_t l2_entry_1;
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
    mpls_entry_1_entry_t mpls_entry;
    mpls_entry_1_entry_t mpls_key;
    int mpls_index = 0;
#endif /* BCM_TRIUMPH_SUPPORT &&  BCM_MPLS_SUPPORT */


    switch(h_data_p->type) {
    case bcmOAMEndpointTypeBHHMPLS:
    case bcmOAMEndpointTypeBHHMPLSVccv:
#if defined(BCM_TRIUMPH_SUPPORT) && defined(BCM_MPLS_SUPPORT)
        SOC_IF_ERROR_RETURN(bcm_tr_mpls_lock (unit));

        sal_memset(&mpls_key, 0, sizeof(mpls_key));
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_key,
                                    MPLS__MPLS_LABELf, h_data_p->label);
        soc_MPLS_ENTRYm_field32_set(unit, &mpls_key, KEY_TYPEf, 16);

        SOC_IF_ERROR_RETURN(soc_mem_search(unit, MPLS_ENTRYm,
                                           MEM_BLOCK_ANY, &mpls_index,
                                           &mpls_key, &mpls_entry, 0));

        if ((soc_MPLS_ENTRYm_field32_get(unit,
                                         &mpls_entry, VALIDf) != 0x1)) {
            bcm_tr_mpls_unlock (unit);
            return (BCM_E_PARAM);
        }

        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry, MPLS__BFD_ENABLEf, 0);

        soc_MPLS_ENTRYm_field32_set(unit, &mpls_entry, MPLS__PW_CC_TYPEf, 0);

        SOC_IF_ERROR_RETURN(soc_mem_write(unit, MPLS_ENTRYm,
                                          MEM_BLOCK_ANY,
                                          mpls_index, &mpls_entry));
        
        bcm_tr_mpls_unlock (unit);

        if (BCM_SUCCESS(_bcm_tr3_oam_bhh_find_l2_entry_1(unit,
                               h_data_p->label,
                               12, 1,
                               &l2_index, &l2_entry_1))) {
            soc_mem_delete_index(unit, L2_ENTRY_1m, MEM_BLOCK_ANY, l2_index);

        } else {
            return (BCM_E_PARAM);
        }
#endif /* BCM_TRIUMPH_SUPPORT &&  BCM_MPLS_SUPPORT */
        break;

    default:
        break;
    }

    if((h_data_p->flags & BCM_OAM_ENDPOINT_LOSS_MEASUREMENT) ||
        (h_data_p->flags & BCM_OAM_ENDPOINT_DELAY_MEASUREMENT))
    {
        rv = _bcm_tr3_oam_bhh_fp_delete(unit, h_data_p);
    }

    return (rv);
}

/*
 * Function:
 *      _bcm_tr3_oam_bhh_msg_send_receive
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
_bcm_tr3_oam_bhh_msg_send_receive(int unit, uint8 s_subclass,
                                    uint16 s_len, uint32 s_data,
                                    uint8 r_subclass, uint16 *r_len)
{
    int rv;
    _bcm_oam_control_t *oc;
    mos_msg_data_t send, reply;
    uint8 *dma_buffer;
    int dma_buffer_len;
    uint32 uc_rv;

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    sal_memset(&send, 0, sizeof(send));
    sal_memset(&reply, 0, sizeof(reply));
    send.s.mclass = MOS_MSG_CLASS_BHH;
    send.s.subclass = s_subclass;
    send.s.len = bcm_htons(s_len);

    /*
     * Set 'data' to DMA buffer address if a DMA operation is
     * required for send or receive.
     */
    dma_buffer = oc->dma_buffer;
    dma_buffer_len = oc->dma_buffer_len;
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

    rv = soc_cmic_uc_msg_send_receive(unit, oc->uc_num,
                                      &send, &reply,
                                      _BHH_UC_MSG_TIMEOUT_USECS);

    /* Check reply class, subclass */
    if ((rv != SOC_E_NONE) ||
        (reply.s.mclass != MOS_MSG_CLASS_BHH) ||
        (reply.s.subclass != r_subclass)) {
        return (BCM_E_INTERNAL);
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

    return (rv);
}

/*
 * Function:
 *      _bcm_tr3_oam_bhh_callback_thread
 * Purpose:
 *      Thread to listen for event messages from uController.
 * Parameters:
 *      param - Pointer to BFD info structure.
 * Returns:
 *      None
 */
STATIC void
_bcm_tr3_oam_bhh_callback_thread(void *param)
{
    int rv;
    _bcm_oam_control_t *oc = (_bcm_oam_control_t *)param;
    bcm_oam_event_types_t events;
    bcm_oam_event_type_t event_type;
    bhh_msg_event_t event_msg;
    int sess_id;
    uint32 event_mask;
    _bcm_oam_event_handler_t *event_handler_p;
    _bcm_oam_hash_data_t *h_data_p;
    int ep_id = 0;

    oc->event_thread_id   = sal_thread_self();
    oc->event_thread_kill = 0;
    soc_cm_debug(DK_VERBOSE, "BHH callback thread starting\n");

    while (1) {
        /* Wait on notifications from uController */
        rv = soc_cmic_uc_msg_receive(oc->unit, oc->uc_num,
                                     MOS_MSG_CLASS_BHH_EVENT, &event_msg,
                                     sal_sem_FOREVER);

        event_handler_p = oc->event_handler_list_p;

        if (BCM_FAILURE(rv) || (oc->event_thread_kill)) {
            break;  /*  Thread exit */
        } else {

        /* Get data from event message */
        sess_id = (int)bcm_ntohs(event_msg.s.len);
        ep_id = BCM_OAM_BHH_GET_SDK_EP(sess_id);

        if (sess_id < 0 || 
            sess_id >= oc->ep_count) {
            soc_cm_print("%s: Invalid sess_id:%d \n", __func__, sess_id);
        } else {
            /* Check endpoint status. */
        rv = shr_idxres_list_elem_state(oc->bhh_pool, sess_id);
        if ((BCM_E_EXISTS != rv)) {
            /* Endpoint not in use. */
            OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                            oc->unit, ep_id, bcm_errmsg(rv)));
        }
        h_data_p = &oc->oam_hash_data[ep_id];
        event_mask = bcm_ntohl(event_msg.s.data);

        /* Set events */
        sal_memset(&events, 0, sizeof(events));

        if (event_mask & BHH_BTE_EVENT_LB_TIMEOUT) {
/*          soc_cm_print("%s: ****** OAM BHH LB Timeout ******\n", __func__);*/

            if (oc->event_handler_cnt[bcmOAMEventBHHLBTimeout] > 0)
            SHR_BITSET(events.w, bcmOAMEventBHHLBTimeout);
        }
        if (event_mask & BHH_BTE_EVENT_LB_DISCOVERY_UPDATE) {
/*          soc_cm_print("%s: ****** OAM BHH LB Discovery Update ******\n", __func__);*/
            if (oc->event_handler_cnt[bcmOAMEventBHHLBDiscoveryUpdate] > 0)
            SHR_BITSET(events.w, bcmOAMEventBHHLBDiscoveryUpdate);
        }
        if (event_mask & BHH_BTE_EVENT_CCM_TIMEOUT) {
/*          soc_cm_print("%s: ****** OAM BHH CCM Timeout ******\n", __func__);*/
            if (oc->event_handler_cnt[bcmOAMEventBHHCCMTimeout] > 0)
            SHR_BITSET(events.w, bcmOAMEventBHHCCMTimeout);
        }
        if (event_mask & BHH_BTE_EVENT_STATE) {
/*          soc_cm_print("%s: ****** OAM BHH State ******\n", __func__);*/
            if (oc->event_handler_cnt[bcmOAMEventBHHCCMState] > 0)
            SHR_BITSET(events.w, bcmOAMEventBHHCCMState);
        }

        /* Loop over registered callbacks,
         * If any match the events field, then invoke
         */
        for (event_handler_p = oc->event_handler_list_p;
             event_handler_p != NULL;
             event_handler_p = event_handler_p->next_p) {
            for (event_type = bcmOAMEventBHHLBTimeout; event_type < bcmOAMEventCount; ++event_type) {
            if (SHR_BITGET(events.w, event_type)) {
                if (SHR_BITGET(event_handler_p->event_types.w,
                       event_type)) {
                    event_handler_p->cb(oc->unit, 
                            0,
                            event_type, 
                            h_data_p->group_index, /* Group index */
                            ep_id, /* Endpoint index */
                            event_handler_p->user_data);
                }
            }
            }
        }
        }
    }
    }

    oc->event_thread_kill = 0;
    oc->event_thread_id   = NULL;
    soc_cm_debug(DK_VERBOSE, "BHH callback thread stopped\n");
    sal_thread_exit(0);
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
 *      _bcm_tr3_oam_bhh_event_mask_set
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
_bcm_tr3_oam_bhh_event_mask_set(int unit)
{
    _bcm_oam_control_t *oc;
    _bcm_oam_event_handler_t *event_handler_p;
    uint32 event_mask = 0;
    uint16 reply_len;
    int rv = BCM_E_NONE;

    /* Lock already taken by the calling routine. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    /* Get event mask from all callbacks */
    for (event_handler_p = oc->event_handler_list_p;
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
    if (event_mask != oc->event_mask) {
        /* Send BHH Event Mask message to uC */
        rv = _bcm_tr3_oam_bhh_msg_send_receive
                (unit,
                 MOS_MSG_SUBCLASS_BHH_EVENT_MASK_SET,
                 0, event_mask,
                 MOS_MSG_SUBCLASS_BHH_EVENT_MASK_SET_REPLY,
                 &reply_len);

        if(BCM_SUCCESS(rv) && (reply_len != 0)) {
            rv = BCM_E_INTERNAL;
        }
    }

    oc->event_mask = event_mask;

    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_loopback_add
 * Purpose:
 *     
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_tr3_oam_loopback_add(int unit, bcm_oam_loopback_t *loopback_p)
{
    _bcm_oam_control_t *oc;
    _bcm_oam_hash_data_t *h_data_p;
    int rv = BCM_E_NONE;
    shr_bhh_msg_ctrl_loopback_add_t msg;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    uint32 flags = 0;
    int sess_id = 0;

    /* Get OAM Control Structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    BCM_OAM_BHH_VALIDATE_EP(loopback_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(loopback_p->id);

    /* Check endpoint status. */
    rv = shr_idxres_list_elem_state(oc->bhh_pool, sess_id);
    if ((BCM_E_EXISTS != rv)) {
        /* Endpoint not in use. */
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, loopback_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    h_data_p = &oc->oam_hash_data[loopback_p->id];

    /*
     * Only BHH is supported
     */
    if (h_data_p->type == bcmOAMEndpointTypeBHHMPLS ||
        h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {

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
    msg.period  = _tr3_ccm_intervals[_bcm_tr3_oam_ccm_msecs_to_hw_encode(loopback_p->period)];

    /*
     * Check TTL
     */
    if (loopback_p->ttl == 0 || loopback_p->ttl > 255) {
        _BCM_OAM_UNLOCK(oc);
        return (BCM_E_PARAM);
    }
    msg.ttl     = loopback_p->ttl;

    /* Pack control message data into DMA buffer */
    buffer     = oc->dma_buffer;
    buffer_ptr = shr_bhh_msg_ctrl_loopback_add_pack(buffer, &msg);
    buffer_len = buffer_ptr - buffer;

    /* Send BHH Session Update message to uC */
    rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                      MOS_MSG_SUBCLASS_BHH_LOOPBACK_ADD,
                      buffer_len, 0,
                      MOS_MSG_SUBCLASS_BHH_LOOPBACK_ADD_REPLY,
                      &reply_len);

    if(BCM_SUCCESS(rv) && (reply_len != 0)) 
        rv =  BCM_E_INTERNAL;
    }
    else {
        return BCM_E_UNAVAIL;
    }

    _BCM_OAM_UNLOCK(oc);
    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_loopback_get
 * Purpose:
 *     
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_tr3_oam_loopback_get(int unit, bcm_oam_loopback_t *loopback_p)
{
    _bcm_oam_control_t *oc;
    int rv = BCM_E_NONE;
    shr_bhh_msg_ctrl_loopback_get_t msg;
    _bcm_oam_hash_data_t *h_data_p;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int sess_id = 0;

    /* Get OAM Control Structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    BCM_OAM_BHH_VALIDATE_EP(loopback_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(loopback_p->id);

    /* Check endpoint status. */
    rv = shr_idxres_list_elem_state(oc->bhh_pool, sess_id);
    if ((BCM_E_EXISTS != rv)) {
        /* Endpoint not in use. */
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, loopback_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    h_data_p = &oc->oam_hash_data[loopback_p->id];

    /*
     * Only BHH is supported
     */
    if (h_data_p->type == bcmOAMEndpointTypeBHHMPLS ||
        h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {

    /* Send BHH Session Update message to uC */
    rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                      MOS_MSG_SUBCLASS_BHH_LOOPBACK_GET,
                      sess_id, 0,
                      MOS_MSG_SUBCLASS_BHH_LOOPBACK_GET_REPLY,
                      &reply_len);
    if(BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, loopback_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    /* Pack control message data into DMA buffer */
    buffer     = oc->dma_buffer;
    buffer_ptr = shr_bhh_msg_ctrl_loopback_get_unpack(buffer, &msg);
    buffer_len = buffer_ptr - buffer;


    if (reply_len != buffer_len) {
        rv =  BCM_E_INTERNAL;
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

        rv = BCM_E_NONE;
    }
    }
    else {
        return BCM_E_UNAVAIL;
    }

    _BCM_OAM_UNLOCK(oc);

    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_loopback_delete
 * Purpose:
 *     
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_tr3_oam_loopback_delete(int unit, bcm_oam_loopback_t *loopback_p)
{
    _bcm_oam_control_t *oc;
    int rv = BCM_E_NONE;
    shr_bhh_msg_ctrl_loopback_delete_t msg;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int sess_id = 0;
    _bcm_oam_hash_data_t *h_data_p;

    /* Get OAM Control Structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    BCM_OAM_BHH_VALIDATE_EP(loopback_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(loopback_p->id);

    /* Check endpoint status. */
    rv = shr_idxres_list_elem_state(oc->bhh_pool, sess_id);
    if ((BCM_E_EXISTS != rv)) {
        /* Endpoint not in use. */
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, loopback_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    h_data_p = &oc->oam_hash_data[loopback_p->id];

    /*
     * Only BHH is supported
     */
    if (h_data_p->type == bcmOAMEndpointTypeBHHMPLS ||
        h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {

        msg.sess_id = sess_id;

        /* Pack control message data into DMA buffer */
        buffer     = oc->dma_buffer;
        buffer_ptr = shr_bhh_msg_ctrl_loopback_delete_pack(buffer, &msg);
        buffer_len = buffer_ptr - buffer;

        /* Send BHH Session Update message to uC */
        rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                      MOS_MSG_SUBCLASS_BHH_LOOPBACK_DELETE,
                      buffer_len, 0,
                      MOS_MSG_SUBCLASS_BHH_LOOPBACK_DELETE_REPLY,
                      &reply_len);

        if(BCM_SUCCESS(rv) && (reply_len != 0))
            rv =  BCM_E_INTERNAL;
    }
    else {
        return BCM_E_UNAVAIL;
    }

    _BCM_OAM_UNLOCK(oc);

    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_loss_add
 * Purpose:
 *        Loss Measurement add     
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_tr3_oam_loss_add(int unit, bcm_oam_loss_t *loss_p)
{
    _bcm_oam_control_t *oc;
    _bcm_oam_hash_data_t *h_data_p;
    int rv = BCM_E_NONE;
    shr_bhh_msg_ctrl_loss_add_t msg;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    uint32 flags = 0;
    int sess_id = 0;

    /* Get OAM Control Structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    BCM_OAM_BHH_VALIDATE_EP(loss_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(loss_p->id);

    /* Check endpoint status. */
    rv = shr_idxres_list_elem_state(oc->bhh_pool, sess_id);
    if ((BCM_E_EXISTS != rv)) {
        /* Endpoint not in use. */
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, loss_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    h_data_p = &oc->oam_hash_data[loss_p->id];

    /*
     * Only BHH is supported
     */
    if (h_data_p->type == bcmOAMEndpointTypeBHHMPLS ||
        h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {

	/*
	 * Convert host space flags to uKernel space flags
	 */
	if (loss_p->flags & BCM_OAM_LOSS_SINGLE_ENDED)
	    flags |= BCM_BHH_LM_SINGLE_ENDED;

	if (loss_p->flags & BCM_OAM_LOSS_TX_ENABLE)
	    flags |= BCM_BHH_LM_TX_ENABLE;

	msg.flags   = flags;
	msg.sess_id = sess_id;

	/*
	 * Set period
	 */
	msg.period  = _tr3_ccm_intervals[_bcm_tr3_oam_ccm_msecs_to_hw_encode(loss_p->period)];
    msg.int_pri = loss_p->int_pri;
    msg.pkt_pri = loss_p->pkt_pri;

    /* Pack control message data into DMA buffer */
    buffer     = oc->dma_buffer;
    buffer_ptr = shr_bhh_msg_ctrl_loss_add_pack(buffer, &msg);
    buffer_len = buffer_ptr - buffer;

    /* Send BHH Session Update message to uC */
    rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                      MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_ADD,
                      buffer_len, 0,
                      MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_ADD_REPLY,
                      &reply_len);

    if(BCM_SUCCESS(rv) && (reply_len != 0)) 
        rv =  BCM_E_INTERNAL;
    }
    else {
        return BCM_E_UNAVAIL;
    }

    _BCM_OAM_UNLOCK(oc);
    return (rv);
}

int
bcm_oam_bhh_convert_ep_to_time_spec(bcm_time_spec_t* bts, int sec, int ns)
{
    int rv = BCM_E_NONE;

    if (bts != NULL) {
        /* if both seconds and nanoseconds are negative or both positive,
         * then the ucode's subtraction is ok.
         * if seconds and nanoseconds have different signs, then "borrow"
         * 1000000000 nanoseconds from seconds.
         */
        if ((sec < 0) && (ns > 0)) {
            ns -= 1000000000;
            sec += 1;
        } else if ((sec > 0) && (ns < 0)) {
            ns += 1000000000;
            sec -= 1;
        }

        if (ns < 0) {
            /* if still negative, then something else is wrong.
             * the nanoseconds field is the difference between two
             * (non-negative) time-stamps.
             */
            rv = BCM_E_INTERNAL;
        }

        /* if seconds is negative then set the bts is-negative flag,
         * and use the absolute value of seconds & nanoseconds.
         */
        bts->isnegative  = (sec < 0 ? 1 : 0);
        bts->seconds     = BCM_OAM_BHH_ABS(sec);
        bts->nanoseconds = BCM_OAM_BHH_ABS(ns);
    } else {
        rv = BCM_E_INTERNAL;
    }

    return rv;
}

#if 0
/* d (difference) = m (minuend) - s (subtrahend) */
int
bcm_oam_bhh_time_spec_subtract(bcm_time_spec_t* d, bcm_time_spec_t* m, bcm_time_spec_t* s)
{
    int rv = BCM_E_NONE;
    int32 d_ns = 0;
    int32 d_s = 0;

    if ((d != NULL) && (m != NULL) && (s != NULL)) {
        /* subtract the nanoseconds first, then borrow is necessary. */
        d_ns = m->nanoseconds - s->nanoseconds;
        if (d_ns < 0) {
            m->seconds = m->seconds - 1;
            d_ns = 1000000000 + d_ns;
        }
        if (d_ns < 0) {
            /* if still negative, then error */
            rv = BCM_E_INTERNAL;
            d_ns = abs(d_ns);
        }
        d->nanoseconds = d_ns;

        /* subtract the seconds next, check for negative. */
        d_s = m->seconds - s->seconds;
        if (d_s < 0) {
            d->isnegative = TRUE;
            d_s = abs(d_s);
        } else {
            d->isnegative = FALSE;
        }
        d->seconds = d_s;

    } else {
        rv = BCM_E_INTERNAL;
    }

    return rv;
}
#endif

/*
 * Function:
 *     bcm_tr3_oam_loss_get
 * Purpose:
 *     Loss Measurement get
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_tr3_oam_loss_get(int unit, bcm_oam_loss_t *loss_p)
{
    _bcm_oam_control_t *oc;
    int rv = BCM_E_NONE;
    shr_bhh_msg_ctrl_loss_get_t msg;
    _bcm_oam_hash_data_t *h_data_p;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int sess_id = 0;

    /* Get OAM Control Structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    BCM_OAM_BHH_VALIDATE_EP(loss_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(loss_p->id);

    /* Check endpoint status. */
    rv = shr_idxres_list_elem_state(oc->bhh_pool, sess_id);
    if ((BCM_E_EXISTS != rv)) {
        /* Endpoint not in use. */
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, loss_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    h_data_p = &oc->oam_hash_data[loss_p->id];

    /*
     * Only BHH is supported
     */
    if (h_data_p->type == bcmOAMEndpointTypeBHHMPLS ||
        h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {

    /* Send BHH Session Update message to uC */
    rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                      MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_GET,
                      sess_id, 0,
                      MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_GET_REPLY,
                      &reply_len);
    if(BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, loss_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    /* Pack control message data into DMA buffer */
    buffer     = oc->dma_buffer;
    buffer_ptr = shr_bhh_msg_ctrl_loss_get_unpack(buffer, &msg);
    buffer_len = buffer_ptr - buffer;

    if (reply_len != buffer_len) {
        rv =  BCM_E_INTERNAL;
    } else {
        /*
         * Convert kernel space flags to host space flags
         */
        if (msg.flags & BCM_BHH_LM_SINGLE_ENDED)
            loss_p->flags |= BCM_OAM_LOSS_SINGLE_ENDED;

        if (msg.flags & BCM_BHH_LM_TX_ENABLE)
            loss_p->flags |= BCM_OAM_LOSS_TX_ENABLE;

        loss_p->period = msg.period;
        loss_p->loss_threshold = msg.loss_threshold;             
        loss_p->loss_nearend = msg.loss_nearend;               
        loss_p->loss_farend = msg.loss_farend;    
        loss_p->tx_nearend = msg.tx_nearend;            
        loss_p->rx_nearend = msg.rx_nearend;              
        loss_p->tx_farend = msg.tx_farend;               
        loss_p->rx_farend = msg.rx_farend;               
        loss_p->rx_oam_packets = msg.rx_oam_packets;          
        loss_p->tx_oam_packets = msg.tx_oam_packets;          
        loss_p->int_pri = msg.int_pri;
        loss_p->pkt_pri = msg.pkt_pri;         

        rv = BCM_E_NONE;
    }
    }
    else {
        return BCM_E_UNAVAIL;
    }

    _BCM_OAM_UNLOCK(oc);

    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_loss_delete
 * Purpose:
 *    Loss Measurement Delete 
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_tr3_oam_loss_delete(int unit, bcm_oam_loss_t *loss_p)
{
    _bcm_oam_control_t *oc;
    int rv = BCM_E_NONE;
    shr_bhh_msg_ctrl_loss_delete_t msg;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int sess_id = 0;
    _bcm_oam_hash_data_t *h_data_p;

    /* Get OAM Control Structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    BCM_OAM_BHH_VALIDATE_EP(loss_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(loss_p->id);

    /* Check endpoint status. */
    rv = shr_idxres_list_elem_state(oc->bhh_pool, sess_id);
    if ((BCM_E_EXISTS != rv)) {
        /* Endpoint not in use. */
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, loss_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    h_data_p = &oc->oam_hash_data[loss_p->id];

    /*
     * Only BHH is supported
     */
    if (h_data_p->type == bcmOAMEndpointTypeBHHMPLS ||
        h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {

        msg.sess_id = sess_id;

        /* Pack control message data into DMA buffer */
        buffer     = oc->dma_buffer;
        buffer_ptr = shr_bhh_msg_ctrl_loss_delete_pack(buffer, &msg);
        buffer_len = buffer_ptr - buffer;

        /* Send BHH Session Update message to uC */
        rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                      MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_DELETE,
                      buffer_len, 0,
                      MOS_MSG_SUBCLASS_BHH_LOSS_MEASUREMENT_DELETE_REPLY,
                      &reply_len);

        if(BCM_SUCCESS(rv) && (reply_len != 0))
            rv =  BCM_E_INTERNAL;
    }
    else {
        return BCM_E_UNAVAIL;
    }

    _BCM_OAM_UNLOCK(oc);

    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_delay_add
 * Purpose:
 *     Delay Measurement add     
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_tr3_oam_delay_add(int unit, bcm_oam_delay_t *delay_p)
{
    _bcm_oam_control_t *oc;
    _bcm_oam_hash_data_t *h_data_p;
    int rv = BCM_E_NONE;
    shr_bhh_msg_ctrl_delay_add_t msg;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    uint32 flags = 0;
    int sess_id = 0;

    /* Get OAM Control Structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    BCM_OAM_BHH_VALIDATE_EP(delay_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(delay_p->id);

    /* Check endpoint status. */
    rv = shr_idxres_list_elem_state(oc->bhh_pool, sess_id);
    if ((BCM_E_EXISTS != rv)) {
        /* Endpoint not in use. */
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, delay_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    h_data_p = &oc->oam_hash_data[delay_p->id];

    /*
     * Only BHH is supported
     */
    if (h_data_p->type == bcmOAMEndpointTypeBHHMPLS ||
        h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {
    /*
     * Convert host space flags to uKernel space flags
     */
    if (delay_p->flags & BCM_OAM_DELAY_ONE_WAY)
        flags |= BCM_BHH_DM_ONE_WAY;

    if (delay_p->flags & BCM_OAM_DELAY_TX_ENABLE)
        flags |= BCM_BHH_DM_TX_ENABLE;

    msg.flags   = flags;
    msg.sess_id = sess_id;

    h_data_p->ts_format = delay_p->timestamp_format;

    if(delay_p->timestamp_format == bcmOAMTimestampFormatIEEE1588v1)
        msg.dm_format = BCM_BHH_DM_TYPE_PTP;
    else
        msg.dm_format = BCM_BHH_DM_TYPE_NTP;

    /*
     * Set period
     */
    msg.period  = _tr3_ccm_intervals[_bcm_tr3_oam_ccm_msecs_to_hw_encode(delay_p->period)];

    /* Pack control message data into DMA buffer */
    buffer     = oc->dma_buffer;
    buffer_ptr = shr_bhh_msg_ctrl_delay_add_pack(buffer, &msg);
    buffer_len = buffer_ptr - buffer;

    /* Send BHH Session Update message to uC */
    rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                      MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_ADD,
                      buffer_len, 0,
                      MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_ADD_REPLY,
                      &reply_len);

    if(BCM_SUCCESS(rv) && (reply_len != 0)) 
        rv =  BCM_E_INTERNAL;
    }
    else {
        return BCM_E_UNAVAIL;
    }

    _BCM_OAM_UNLOCK(oc);
    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_delay_get
 * Purpose:
 *     Delay Measurements get 
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_tr3_oam_delay_get(int unit, bcm_oam_delay_t *delay_p)
{
    _bcm_oam_control_t *oc;
    int rv = BCM_E_NONE;
    shr_bhh_msg_ctrl_delay_get_t msg;
    _bcm_oam_hash_data_t *h_data_p;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int sess_id = 0;

    /* Get OAM Control Structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    BCM_OAM_BHH_VALIDATE_EP(delay_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(delay_p->id);

    /* Check endpoint status. */
    rv = shr_idxres_list_elem_state(oc->bhh_pool, sess_id);
    if ((BCM_E_EXISTS != rv)) {
        /* Endpoint not in use. */
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, delay_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    h_data_p = &oc->oam_hash_data[delay_p->id];

    /*
     * Only BHH is supported
     */
    if (h_data_p->type == bcmOAMEndpointTypeBHHMPLS ||
        h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {

    /* Send BHH Session Update message to uC */
    rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                      MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_GET,
                      sess_id, 0,
                      MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_GET_REPLY,
                      &reply_len);
    if(BCM_FAILURE(rv)) {
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, delay_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    /* Pack control message data into DMA buffer */
    buffer     = oc->dma_buffer;
    buffer_ptr = shr_bhh_msg_ctrl_delay_get_unpack(buffer, &msg);
    buffer_len = buffer_ptr - buffer;


    if (reply_len != buffer_len) {
        rv =  BCM_E_INTERNAL;
    } else {
        /*
         * Convert kernel space flags to host space flags
         */
        if (msg.flags & BCM_BHH_DM_ONE_WAY)
            delay_p->flags |= BCM_OAM_DELAY_ONE_WAY;

        if (msg.flags & BCM_BHH_DM_TX_ENABLE)
            delay_p->flags |= BCM_OAM_DELAY_TX_ENABLE;

        delay_p->period = msg.period;

        if(msg.dm_format == BCM_BHH_DM_TYPE_PTP)
            delay_p->timestamp_format = bcmOAMTimestampFormatIEEE1588v1;
        else
            delay_p->timestamp_format = bcmOAMTimestampFormatNTP;

        rv = bcm_oam_bhh_convert_ep_to_time_spec(&(delay_p->delay), 
                                       msg.delay_seconds, msg.delay_nanoseconds);
        if(BCM_SUCCESS(rv)) {
            rv = bcm_oam_bhh_convert_ep_to_time_spec(&(delay_p->txf), 
                                       msg.txf_seconds, msg.txf_nanoseconds);
        }
        if(BCM_SUCCESS(rv)) {
            rv = bcm_oam_bhh_convert_ep_to_time_spec(&(delay_p->rxf), 
                                       msg.rxf_seconds, msg.rxf_nanoseconds);
        }
        if(BCM_SUCCESS(rv)) {
            rv = bcm_oam_bhh_convert_ep_to_time_spec(&(delay_p->txb), 
                                       msg.txb_seconds, msg.txb_nanoseconds);
        }
        if(BCM_SUCCESS(rv)) {
            rv = bcm_oam_bhh_convert_ep_to_time_spec(&(delay_p->rxb), 
                                       msg.rxb_seconds, msg.rxb_nanoseconds);
        }
        delay_p->rx_oam_packets = msg.rx_oam_packets;
        delay_p->tx_oam_packets = msg.tx_oam_packets;
        delay_p->int_pri = msg.int_pri;
        delay_p->pkt_pri = msg.pkt_pri;

        rv = BCM_E_NONE;
    }
    }
    else {
        return BCM_E_UNAVAIL;
    }

    _BCM_OAM_UNLOCK(oc);

    return (rv);
}

/*
 * Function:
 *     bcm_tr3_oam_delay_delete
 * Purpose:
 *     Delay Measurement Delete 
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
int
bcm_tr3_oam_delay_delete(int unit, bcm_oam_delay_t *delay_p)
{
    _bcm_oam_control_t *oc;
    int rv = BCM_E_NONE;
    shr_bhh_msg_ctrl_delay_delete_t msg;
    uint8 *buffer, *buffer_ptr;
    uint16 buffer_len, reply_len;
    int sess_id = 0;
    _bcm_oam_hash_data_t *h_data_p;

    /* Get OAM Control Structure. */
    BCM_IF_ERROR_RETURN(_bcm_oam_control_get(unit, &oc));

    _BCM_OAM_LOCK(oc);

    BCM_OAM_BHH_VALIDATE_EP(delay_p->id);

    sess_id = BCM_OAM_BHH_GET_UKERNEL_EP(delay_p->id);

    /* Check endpoint status. */
    rv = shr_idxres_list_elem_state(oc->bhh_pool, sess_id);
    if ((BCM_E_EXISTS != rv)) {
        /* Endpoint not in use. */
        OAM_ERR(("OAM(unit %d) Error: Endpoint EP=%d %s.\n",
                unit, delay_p->id, bcm_errmsg(rv)));
        _BCM_OAM_UNLOCK(oc);
        return (rv);
    }

    h_data_p = &oc->oam_hash_data[delay_p->id];

    /*
     * Only BHH is supported
     */
    if (h_data_p->type == bcmOAMEndpointTypeBHHMPLS ||
        h_data_p->type == bcmOAMEndpointTypeBHHMPLSVccv) {

        msg.sess_id = sess_id;

        /* Pack control message data into DMA buffer */
        buffer     = oc->dma_buffer;
        buffer_ptr = shr_bhh_msg_ctrl_delay_delete_pack(buffer, &msg);
        buffer_len = buffer_ptr - buffer;

        /* Send BHH Session Update message to uC */
        rv = _bcm_tr3_oam_bhh_msg_send_receive(unit,
                      MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_DELETE,
                      buffer_len, 0,
                      MOS_MSG_SUBCLASS_BHH_DELAY_MEASUREMENT_DELETE_REPLY,
                      &reply_len);

        if(BCM_SUCCESS(rv) && (reply_len != 0))
            rv =  BCM_E_INTERNAL;
    }
    else {
        return BCM_E_UNAVAIL;
    }

    _BCM_OAM_UNLOCK(oc);

    return (rv);
}

#endif /* defined(INCLUDE_BHH) */
#endif
