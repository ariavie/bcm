/*
 * $Id: rx.c 1.3 Broadcom SDK $
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
 * File:    rx.c
 *
 * Purpose: 
 *
 * Functions:
 *      _bcm_ptp_rx_init
 *      _bcm_ptp_rx_stack_create
 *      _bcm_ptp_rx_clock_create
 *      _bcm_ptp_external_rx_response_free
 *      _bcm_ptp_internal_rx_response_free
 *      _bcm_ptp_rx_response_flush
 *      _bcm_ptp_rx_response_get
 *      _bcm_ptp_rx_thread
 *      _bcm_ptp_rx_callback
 *      _bcm_ptp_rx_message_destination_port_get
 *      _bcm_ptp_rx_message_source_clock_identity_get
 *      _bcm_ptp_rx_message_length_get
 *      _bcm_ptp_register_management_callback
 *      _bcm_ptp_register_event_callback
 *      _bcm_ptp_register_signal_callback
 *      _bcm_ptp_register_peers_callback
 *      _bcm_ptp_register_fault_callback
 *      _bcm_ptp_unregister_management_callback
 *      _bcm_ptp_unregister_event_callback
 *      _bcm_ptp_unregister_signal_callback
 *      _bcm_ptp_unregister_peers_callback
 *      _bcm_ptp_event_message_monitor
 *      _bcm_ptp_signal_handler_default
 */

#if defined(INCLUDE_PTP)

#ifdef BCM_HIDE_DISPATCHABLE
#undef BCM_HIDE_DISPATCHABLE
#endif

#include <soc/defs.h>
#include <soc/drv.h>

#include <bcm/pkt.h>
#include <bcm/tx.h>
#include <bcm/rx.h>
#include <bcm/error.h>
#include <bcm/ptp.h>
#include <bcm_int/common/ptp.h>

#if defined(BCM_PTP_INTERNAL_STACK_SUPPORT)
#include <soc/uc_msg.h>
#endif


void (*_bcm_ptp_arp_callback)(int unit, bcm_ptp_stack_id_t ptp_id,
                              int protocol, int src_addr_offset,
                              int payload_offset, int msg_len, uint8 *msg);

#define PTP_SDK_VERSION         0x01000000
#define PTP_UC_MIN_VERSION      0x01000000

#define PTP_RX_PACKET_MIN_SIZE_OCTETS     (14)
#define PTP_RX_TUNNEL_MSG_MIN_SIZE_OCTETS (11)
#define PTP_RX_EVENT_MSG_MIN_SIZE_OCTETS  (2)
#define PTP_RX_MGMT_MIN_SIZE              (0x64)
#define PTP_RX_UDP_PAYLOAD_OFFSET         (46)

/* PTP clock Rx data. */
typedef struct _bcm_ptp_clock_rx_data_s {
    _bcm_ptp_sem_t response_ready;
    uint8 * volatile response_data;
    volatile int response_len;
} _bcm_ptp_clock_rx_data_t;

/* Stack PTP Rx data arrays. */
typedef struct _bcm_ptp_stack_rx_array_s {
    _bcm_ptp_memstate_t memstate;

    bcm_mac_t host_mac;
    bcm_mac_t top_mac;
    int tpid;
    int vlan;

    _bcm_ptp_clock_rx_data_t *clock_data;
} _bcm_ptp_stack_rx_array_t;

/* Unit PTP Rx data arrays. */
typedef struct _bcm_ptp_unit_rx_array_s {
    _bcm_ptp_memstate_t memstate;

    bcm_ptp_cb management_cb;
    bcm_ptp_cb event_cb;
    bcm_ptp_cb signal_cb;
    bcm_ptp_cb fault_cb;
    bcm_ptp_cb peers_cb;
    uint8      *management_user_data;
    uint8      *event_user_data;
    uint8      *signal_user_data;
    uint8      *fault_user_data;
    uint8      *peers_user_data;

    _bcm_ptp_stack_rx_array_t *stack_array;
} _bcm_ptp_unit_rx_array_t;

static const _bcm_ptp_clock_rx_data_t rx_default;
static _bcm_ptp_unit_rx_array_t unit_rx_array[BCM_MAX_NUM_UNITS];



int _bcm_ptp_most_recent_clock_num;
int _bcm_ptp_most_recent_port;
bcm_ptp_protocol_t _bcm_ptp_most_recent_protocol;
int _bcm_ptp_most_recent_src_addr_offset;
int _bcm_ptp_most_recent_msg_offset;

#if defined(BCM_PTP_INTERNAL_STACK_SUPPORT)
static void _bcm_ptp_rx_thread(void *arg);
#endif

#if defined(BCM_PTP_EXTERNAL_STACK_SUPPORT)
static bcm_rx_t _bcm_ptp_rx_callback(
    int unit,
    bcm_pkt_t *pkt,
    void *cookie);

static int _bcm_ptp_rx_message_destination_port_get(
    uint8 *message,
    uint16 *dest_port);

static int _bcm_ptp_rx_message_source_clock_identity_get(
    uint8 *message,
    bcm_ptp_clock_identity_t *clock_identity);

static int _bcm_ptp_rx_message_length_get(
    uint8 *message,
    uint16 *message_len);

#endif /* defined(BCM_PTP_EXTERNAL_STACK_SUPPORT) */

#if defined(BCM_PTP_INTERNAL_STACK_SUPPORT) || defined(BCM_PTP_EXTERNAL_STACK_SUPPORT)
STATIC int _bcm_ptp_event_message_monitor(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int ev_data_len,
    uint8 *ev_data,
    int *ev_internal);
#endif

STATIC int _bcm_ptp_signal_handler_default(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_cb_type_t type,
    bcm_ptp_cb_msg_t *msg,
    void *user_data);

/*
 * Function:
 *      _bcm_ptp_rx_init
 * Purpose:
 *      Initialize the PTP Rx framework and data of a unit.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_rx_init(
    int unit)
{
    int rv = BCM_E_UNAVAIL;
    _bcm_ptp_stack_rx_array_t *stack_p;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    
    stack_p = sal_alloc(PTP_MAX_STACKS_PER_UNIT*
                        sizeof(_bcm_ptp_stack_rx_array_t),"Unit Rx arrays");

    if (!stack_p) {
        unit_rx_array[unit].memstate = PTP_MEMSTATE_FAILURE;
        return BCM_E_MEMORY;
    }

    unit_rx_array[unit].stack_array = stack_p;
    unit_rx_array[unit].memstate = PTP_MEMSTATE_INITIALIZED;

    unit_rx_array[unit].management_cb = NULL;
    unit_rx_array[unit].event_cb = NULL;
    unit_rx_array[unit].signal_cb = NULL;
    unit_rx_array[unit].fault_cb = NULL;

    unit_rx_array[unit].management_user_data = NULL;
    unit_rx_array[unit].event_user_data = NULL;
    unit_rx_array[unit].signal_user_data = NULL;
    unit_rx_array[unit].fault_user_data = NULL;

    /*
     * Add default callback function for tunneled PTP signaling messages
     */
    if (BCM_FAILURE(rv = _bcm_ptp_register_signal_callback(unit,
            _bcm_ptp_signal_handler_default, NULL))) {
        PTP_ERROR_FUNC("_bcm_ptp_register_signal_callback()");
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_rx_init_detach
 * Purpose:
 *      Shut down the PTP Rx framework and data of a unit.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_rx_detach(
    int unit)
{
    if(unit_rx_array[unit].stack_array) {
        sal_free(unit_rx_array[unit].stack_array);
        unit_rx_array[unit].stack_array = NULL;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_rx_stack_create
 * Purpose:
 *      Create the PTP Rx data of a PTP stack.
 * Parameters:
 *      unit     - (IN) Unit number.
 *      ptp_id   - (IN) PTP stack ID.
 *      host_mac - (IN) Host MAC address.
 *      top_mac  - (IN) ToP MAC address.
 *      tpid     - (IN) TPID for Host <-> ToP Communication
 *      vlan     - (IN) VLAN for Host <-> ToP Communication
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_rx_stack_create(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    bcm_mac_t *host_mac,
    bcm_mac_t *top_mac,
    int tpid,
    int vlan)
{
    int rv = BCM_E_UNAVAIL;
    _bcm_ptp_clock_rx_data_t *data_p;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (unit_rx_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) {
        SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
            "%s() failed %s\n", __func__,
            "_bcm_ptp_rx_stack_create(): memory state"));
        return BCM_E_UNAVAIL;
    }

    
    data_p = sal_alloc(PTP_MAX_CLOCK_INSTANCES*
                       sizeof(_bcm_ptp_clock_rx_data_t),
                       "PTP stack Rx array");

    if (!data_p) {
        unit_rx_array[unit].stack_array[ptp_id].memstate =
            PTP_MEMSTATE_FAILURE;
        SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "%s() failed %s\n", __func__,
            "_bcm_ptp_rx_stack_create(): not initialized"));
        return BCM_E_MEMORY;
    }

    unit_rx_array[unit].stack_array[ptp_id].clock_data = data_p;
    unit_rx_array[unit].stack_array[ptp_id].memstate =
        PTP_MEMSTATE_INITIALIZED;

#if defined(BCM_PTP_EXTERNAL_STACK_SUPPORT)
    if (SOC_HAS_PTP_EXTERNAL_STACK_SUPPORT(unit)) {
        sal_memcpy(unit_rx_array[unit].stack_array[ptp_id].host_mac, host_mac,
                   sizeof(bcm_mac_t));

        sal_memcpy(unit_rx_array[unit].stack_array[ptp_id].top_mac, top_mac,
                   sizeof(bcm_mac_t));

        unit_rx_array[unit].stack_array[ptp_id].tpid = tpid;
        unit_rx_array[unit].stack_array[ptp_id].vlan = vlan;

#ifndef CUSTOMER_CALLBACK
        if (!bcm_rx_active(unit)) {
            if (BCM_FAILURE(rv = bcm_rx_cfg_init(unit))) {
                PTP_ERROR_FUNC("bcm_rx_cfg_init()");
                return rv;
            }

            if (BCM_FAILURE(rv = bcm_rx_start(unit, NULL))) {
                PTP_ERROR_FUNC("bcm_rx_start()");
                return rv;
            }
        }

        if (BCM_FAILURE(rv = bcm_rx_register(unit, "BCM_PTP_Rx", _bcm_ptp_rx_callback,
                                             BCM_RX_PRIO_MAX, (void*)ptp_id, BCM_RCO_F_ALL_COS))) {
            PTP_ERROR_FUNC("bcm_rx_register()");
            return rv;
        }
#endif /* CUSTOMER_CALLBACK */
    }
#endif /* defined(BCM_PTP_EXTERNAL_STACK_SUPPORT) */

#if defined(BCM_PTP_INTERNAL_STACK_SUPPORT)
    if (SOC_HAS_PTP_INTERNAL_STACK_SUPPORT(unit)) {
        _bcm_ptp_stack_info_t *stack_p;
        _bcm_ptp_info_t *ptp_info_p;

        SET_PTP_INFO;
        stack_p = &ptp_info_p->stack_info[ptp_id];

        sal_thread_create("PTP Rx", SAL_THREAD_STKSZ,
                          soc_property_get(unit, spn_UC_MSG_THREAD_PRI, 50) + 1,
                          _bcm_ptp_rx_thread, stack_p);
    }
#endif

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_rx_clock_create
 * Purpose:
 *      Create the PTP Rx data of a PTP clock.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_rx_clock_create(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if ((unit_rx_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (unit_rx_array[unit].stack_array[ptp_id].memstate !=
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    unit_rx_array[unit].stack_array[ptp_id].clock_data[clock_num].response_data = 0;
    unit_rx_array[unit].stack_array[ptp_id].clock_data[clock_num].response_len = 0;

    unit_rx_array[unit].stack_array[ptp_id].clock_data[clock_num].response_ready =
        _bcm_ptp_sem_create("BCM_PTP_resp", sal_sem_BINARY, 0);

    return rv;
}



/* External version */
int
_bcm_ptp_external_rx_response_free(int unit, int ptp_id, uint8 *resp_data)
{
    return bcm_rx_free(unit, resp_data - PTP_RX_UDP_PAYLOAD_OFFSET);
}

/* Internal version */
int
_bcm_ptp_internal_rx_response_free(int unit, int ptp_id, uint8 *resp_data)
{
    _bcm_ptp_stack_info_t *stack_p;
    _bcm_ptp_info_t *ptp_info_p;
    int i;

    SET_PTP_INFO;
    stack_p = &ptp_info_p->stack_info[ptp_id];
    for (i = 0; i < BCM_PTP_MAX_BUFFERS; ++i) {
        if (stack_p->int_state.mboxes->mbox[i].data == resp_data) {
            if (stack_p->int_state.mboxes->status[i] != MBOX_STATUS_PENDING_HOST) {
                SOC_DEBUG_PRINT((DK_ERR, "Invalid mbox status on PTP rx response free (%d)\n",
                                 stack_p->int_state.mboxes->status[i]));
            }
            stack_p->int_state.mboxes->status[i] = MBOX_STATUS_EMPTY;
            
            
            
            soc_cm_sflush(unit, (void*)&stack_p->int_state.mboxes->status[i], sizeof(stack_p->int_state.mboxes->status[i]));
            return BCM_E_NONE;
        }
    }

    SOC_DEBUG_PRINT((DK_ERR, "Invalid PTP rx response free (%p vs %p)\n",
                     (void *)resp_data, (void*)stack_p->int_state.mboxes->mbox[i-1].data));

    return BCM_E_NOT_FOUND;
}


/*
 * Function:
 *      _bcm_ptp_rx_response_flush
 * Purpose:
 *      Flush prior Rx response.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_rx_response_flush(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num)
{
    int rv = BCM_E_UNAVAIL;
    int spl;

    uint8 *prior_data;
    _bcm_ptp_stack_info_t *stack_p;
    _bcm_ptp_info_t *ptp_info_p;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if ((unit_rx_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
            (unit_rx_array[unit].stack_array[ptp_id].memstate !=
            PTP_MEMSTATE_INITIALIZED)) {
        SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
            "%s() failed %s\n", __func__,
            "_bcm_ptp_rx_response_flush(): memory state"));
        return BCM_E_UNAVAIL;
    }

    rv = _bcm_ptp_sem_take(unit_rx_array[unit].stack_array[ptp_id]
            .clock_data[clock_num].response_ready, sal_mutex_NOWAIT);
    if (rv == BCM_E_NONE) {
        /*
         * Flush response.
         * NOTICE: Response already waiting is unexpected.
         */
        SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
            "%s() failed %s\n", __func__,
            "Flushed unexpected response"));

        /* Lock. */
        spl = sal_splhi();

        prior_data = unit_rx_array[unit].stack_array[ptp_id]
                                        .clock_data[clock_num].response_data;

        unit_rx_array[unit].stack_array[ptp_id]
                           .clock_data[clock_num].response_data = 0;

        /* Unlock. */
        sal_spl(spl);

        if (prior_data) {

            SET_PTP_INFO;
            stack_p = &ptp_info_p->stack_info[ptp_id];

            stack_p->rx_free(unit, ptp_id, prior_data);
        }
    }
    if (BCM_FAILURE(rv)) {
       PTP_ERROR_FUNC("_bcm_ptp_sem_take()");
    }

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_rx_response_get
 * Purpose:
 *      Get Rx response data for a PTP clock.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      usec       - (IN)  Semaphore timeout (usec).
 *      data       - (OUT) Response data.
 *      data_len   - (OUT) Response data size (octets).
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_rx_response_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int usec,
    uint8 **data,
    int *data_len)
{
    int rv = BCM_E_UNAVAIL;
    int spl;
    sal_usecs_t expiration_time = sal_time_usecs() + usec;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if ((unit_rx_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
            (unit_rx_array[unit].stack_array[ptp_id].memstate !=
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    rv = BCM_E_FAIL;
    /* ptp_printf("Await resp @ %d\n", (int)sal_time_usecs()); */

    while (BCM_FAILURE(rv) && (int32) (sal_time_usecs() - expiration_time) < 0) {
        rv = _bcm_ptp_sem_take(unit_rx_array[unit].stack_array[ptp_id].clock_data[clock_num].response_ready, usec);
    }
    if (BCM_FAILURE(rv)) {
        SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE, "Failed management Tx to ToP\n"));
        PTP_ERROR_FUNC("_bcm_ptp_sem_take()");
        return rv;
    }

    /* Lock. */
    spl = sal_splhi();

    *data = unit_rx_array[unit].stack_array[ptp_id]
               .clock_data[clock_num].response_data;

    *data_len = unit_rx_array[unit].stack_array[ptp_id]
                    .clock_data[clock_num].response_len;

    unit_rx_array[unit].stack_array[ptp_id]
        .clock_data[clock_num].response_data = 0;

    /* Unlock. */
    sal_spl(spl);

    return rv;
}


#if defined(BCM_PTP_INTERNAL_STACK_SUPPORT)
static void _bcm_ptp_rx_thread(void *arg)
{
    _bcm_ptp_stack_info_t *stack_p = arg;
    int unit = stack_p->unit;
    bcm_ptp_stack_id_t ptp_id = stack_p->stack_id;
    int rv = 0;
    _bcm_ptp_stack_rx_array_t *stack_rx = &unit_rx_array[unit].stack_array[ptp_id];
    bcm_ptp_cb_msg_t cb_msg;

    /* sal_usecs_t last_time = 0; */
    /* sal_usecs_t this_time = 0; */

    while (1) {
        int mbox;
        /* mos_msg_data_t rcv; */
        /* int rv = soc_cmic_uc_msg_receive(stack_p->unit, stack_p->int_state.core_num, MOS_MSG_CLASS_1588, &rcv, sal_sem_FOREVER); */
        /* if (rv) { */
        /*     /\* got error, so wait *\/ */
        /*     sal_usleep(100000); */
        /* } */

        /* The uc_msg is just a signal that there is a message somewhere to get, so look through all mboxes */

        sal_usleep(1000);  

        /* last_time = this_time; */
        /* this_time = sal_time_usecs(); */

        soc_cm_sinval(unit, (void*)&stack_p->int_state.mboxes->status[0], sizeof(stack_p->int_state.mboxes->status[0] * BCM_PTP_MAX_BUFFERS));

        for (mbox = 0; mbox < BCM_PTP_MAX_BUFFERS; ++mbox) {
            /* If there is something in the mbox for the host, we need to invalidate
             * the mbox memory in cache before reading it                            */
            switch (stack_p->int_state.mboxes->status[mbox]) {
            case MBOX_STATUS_ATTN_HOST_TUNNEL:
            case MBOX_STATUS_ATTN_HOST_EVENT:
            case MBOX_STATUS_ATTN_HOST_RESP:
                soc_cm_sinval(unit, (void*)&stack_p->int_state.mboxes->mbox[mbox], sizeof(stack_p->int_state.mboxes->mbox[mbox]));
                break;
            default:
                break;
            }

            switch (stack_p->int_state.mboxes->status[mbox]) {
            case MBOX_STATUS_ATTN_HOST_TUNNEL: {
                int cb_flags = 0;        
                unsigned clock_num = stack_p->int_state.mboxes->mbox[mbox].clock_num;
                int message_type = _bcm_ptp_uint16_read((uint8 *)stack_p->int_state.mboxes->mbox[mbox].data + MBOX_TUNNEL_MSG_TYPE_OFFSET);
                int port_num = _bcm_ptp_uint16_read((uint8 *)stack_p->int_state.mboxes->mbox[mbox].data + MBOX_TUNNEL_PORT_NUM_OFFSET);
                int protocol = _bcm_ptp_uint16_read((uint8 *)stack_p->int_state.mboxes->mbox[mbox].data + MBOX_TUNNEL_PROTOCOL_OFFSET);
                int src_addr_offset = _bcm_ptp_uint16_read((uint8 *)stack_p->int_state.mboxes->mbox[mbox].data + MBOX_TUNNEL_SRC_ADDR_OFFS_OFFSET);
                int ptp_offset = _bcm_ptp_uint16_read((uint8 *)stack_p->int_state.mboxes->mbox[mbox].data + MBOX_TUNNEL_PTP_OFFS_OFFSET);
                uint8 * cb_data = (uint8 *)stack_p->int_state.mboxes->mbox[mbox].data + MBOX_TUNNEL_PACKET_OFFSET;
                int cb_data_len = stack_p->int_state.mboxes->mbox[mbox].data_len - MBOX_TUNNEL_PACKET_OFFSET;

                bcm_ptp_clock_identity_t peer_clockIdentity;
                bcm_ptp_clock_port_address_t peer_portAddress;

                /* ptp_printf("Got Tunnel: MsgType:%d, Proto:%d Clk:%d\n", message_type, protocol, clock_num); */

                
                _bcm_ptp_most_recent_clock_num = clock_num;
                _bcm_ptp_most_recent_port = port_num;
                _bcm_ptp_most_recent_protocol = protocol;
                _bcm_ptp_most_recent_src_addr_offset = src_addr_offset;
                _bcm_ptp_most_recent_msg_offset = ptp_offset;
                cb_flags = 0;

                if (message_type == bcmPTP_MESSAGE_TYPE_SIGNALING ||
                    message_type == bcmPTP_MESSAGE_TYPE_MANAGEMENT) {
                    /* Peer information for counts of Rx signaling and external management messages. */
                    sal_memcpy(peer_clockIdentity,
                               cb_data + ptp_offset + PTP_PTPHDR_SRCPORT_OFFSET_OCTETS,
                               sizeof(bcm_ptp_clock_identity_t));

                    peer_portAddress.addr_type = protocol;
                    sal_memset(peer_portAddress.address, 0, BCM_PTP_MAX_NETW_ADDR_SIZE);
                    switch (protocol) {
                    case bcmPTPUDPIPv4:
                        sal_memcpy(peer_portAddress.address,
                                   cb_data + src_addr_offset, PTP_IPV4_ADDR_SIZE_BYTES);
                        break;
                    case bcmPTPUDPIPv6:
                        sal_memcpy(peer_portAddress.address,
                                   cb_data + src_addr_offset, PTP_IPV6_ADDR_SIZE_BYTES);
                        break;
                    case bcmPTPIEEE8023:
                        sal_memcpy(peer_portAddress.address,
                                   cb_data + src_addr_offset, PTP_MAC_ADDR_SIZE_BYTES);
                        break;
                    default:
                        ;
                    }
                }

                switch (message_type) {
                case bcmPTP_MESSAGE_TYPE_SIGNALING:
                    /* Peer dataset counts of Rx signaling messages. */
                    if (BCM_FAILURE(rv = _bcm_ptp_update_peer_counts(port_num,
                            &peer_clockIdentity, &peer_portAddress, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0))) {
                        PTP_ERROR_FUNC("_bcm_ptp_update_peer_counts()");
                    }

                    if (unit_rx_array[unit].signal_cb) {
                        sal_memset(&cb_msg, 0, sizeof(bcm_ptp_cb_msg_t));
                        cb_msg.length = cb_data_len;
                        cb_msg.data = cb_data;
                        unit_rx_array[unit].signal_cb(unit, ptp_id, clock_num, port_num,
                                                      bcmPTPCallbackTypeSignal, &cb_msg, 
                                                      unit_rx_array[unit].signal_user_data);
                    }
                    break;

                case bcmPTP_MESSAGE_TYPE_MANAGEMENT:
                    /* Peer dataset counts of Rx external management messages. */
                    if (BCM_FAILURE(rv = _bcm_ptp_update_peer_counts(port_num,
                            &peer_clockIdentity, &peer_portAddress, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0))) {
                        PTP_ERROR_FUNC("_bcm_ptp_update_peer_counts()");
                    }

                    if (unit_rx_array[unit].management_cb) {
                        sal_memset(&cb_msg, 0, sizeof(bcm_ptp_cb_msg_t));

                        cb_msg.flags = cb_flags;
                        cb_msg.protocol = protocol;
                        cb_msg.src_addr_offset = src_addr_offset;
                        cb_msg.msg_offset = ptp_offset;
                        cb_msg.length = cb_data_len;
                        cb_msg.data = cb_data;

                        if (unit_rx_array[unit].management_cb(unit, ptp_id, clock_num, port_num,
                                                              bcmPTPCallbackTypeManagement, &cb_msg,
                                                              unit_rx_array[unit].management_user_data)
                            == bcmPTPCallbackAccept) {
                            /* Tunnel message to ToP. */
                            
                            
                            if (BCM_FAILURE(rv =_bcm_ptp_tunnel_message_to_top(unit,
                                                   ptp_id, cb_data_len, cb_data))) {
                                PTP_ERROR_FUNC("_bcm_ptp_tunnel_message_to_top()");
                            }
                        }
                    }

                    break;
                case bcmPTP_MESSAGE_TYPE_ARP:
                    if (_bcm_ptp_arp_callback) {
                        _bcm_ptp_arp_callback(unit, ptp_id, protocol, src_addr_offset, ptp_offset, cb_data_len, cb_data);
                    }
                    break;

                default:
                    SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                                     "Improper PTP tunneled message type: %d in mbox %d\n", message_type, mbox));
                    break;
                }
                /* Free the mbox so the CMICm can use it again */
                stack_p->int_state.mboxes->status[mbox] = MBOX_STATUS_EMPTY;
                
                soc_cm_sflush(unit, (void*)&stack_p->int_state.mboxes->status[mbox],
                              sizeof(stack_p->int_state.mboxes->status[mbox]));

                break;
              }

            case MBOX_STATUS_ATTN_HOST_EVENT: {
                
                unsigned clock_num = stack_p->int_state.mboxes->mbox[mbox].clock_num;
                int cb_flags = 0;
                int cb_data_len = stack_p->int_state.mboxes->mbox[mbox].data_len;
                uint8 *cb_data = (uint8 *)stack_p->int_state.mboxes->mbox[mbox].data;
                int ev_internal = 0;

                if (BCM_FAILURE(rv = _bcm_ptp_event_message_monitor(unit, ptp_id,
                        cb_data_len, cb_data, &ev_internal))) {
                        PTP_ERROR_FUNC("_bcm_ptp_event_message_monitor()");
                }

                if (unit_rx_array[unit].event_cb && !ev_internal) {
                    sal_memset(&cb_msg, 0, sizeof(bcm_ptp_cb_msg_t));
                    cb_msg.length = cb_data_len;
                    cb_msg.data = cb_data;
                    cb_msg.flags = cb_flags;
                    unit_rx_array[unit].event_cb(unit, ptp_id, clock_num,
                                                 PTP_CLOCK_PORT_NUMBER_DEFAULT,
                                                 bcmPTPCallbackTypeEvent, &cb_msg,
                                                 unit_rx_array[unit].event_user_data);
                }

                stack_p->int_state.mboxes->status[mbox] = MBOX_STATUS_EMPTY;
                
                soc_cm_sflush(unit, (void*)&stack_p->int_state.mboxes->status[mbox],
                              sizeof(stack_p->int_state.mboxes->status[mbox]));
                break;
              }

            case MBOX_STATUS_ATTN_HOST_RESP: {
                /* This really should only be in mbox 0, but here for generality */
                int clock_num = stack_p->int_state.mboxes->mbox[mbox].clock_num;

                /* ptp_printf("Got HOST_RESP in mbox %d\n", mbox); */

                stack_rx->clock_data[clock_num].response_data = (uint8 *)stack_p->int_state.mboxes->mbox[mbox].data;
                stack_rx->clock_data[clock_num].response_len = stack_p->int_state.mboxes->mbox[mbox].data_len;

                stack_p->int_state.mboxes->status[mbox] = MBOX_STATUS_PENDING_HOST;
                
                soc_cm_sflush(unit, (void*)&stack_p->int_state.mboxes->status[mbox],
                              sizeof(stack_p->int_state.mboxes->status[mbox]));

                rv = _bcm_ptp_sem_give(stack_rx->clock_data[clock_num].response_ready);
                if (BCM_FAILURE(rv)) {
                    PTP_ERROR_FUNC("_bcm_ptp_sem_give()");
                }
                break;
              }
            }
        }
    }
}
#endif  /* defined(BCM_PTP_INTERNAL_STACK_SUPPORT) */

#if defined(BCM_PTP_EXTERNAL_STACK_SUPPORT)

bcm_rx_t
_bcm_ptp_rx_callback(
    int unit,
    bcm_pkt_t *pkt,
    void *cookie)
{
    int rv = BCM_E_UNAVAIL;
    int spl;

    uint8 *prior_data;
    uint16 udp_dest_port;
    uint16 message_len;

    int wrapClockNumber;
    int wrapPortNumber;
    bcm_ptp_message_type_t wrapMessageType;
    bcm_ptp_protocol_t wrapProtocol;
    uint16 wrapSrcAddrOffset;
    uint16 wrapPtpOffset;

    _bcm_ptp_stack_info_t *stack_p;
    _bcm_ptp_info_t *ptp_info_p;
    bcm_ptp_clock_identity_t cb_clock_identity;
    bcm_ptp_stack_id_t cb_ptp_id = (bcm_ptp_stack_id_t)cookie;
    int cb_clock_num;
    uint32 cb_flags;
    int ev_internal = 0;
    int vlan, tpid;
    int i = 0;

    bcm_ptp_clock_identity_t peer_clockIdentity;
    bcm_ptp_clock_port_address_t peer_portAddress;

    bcm_ptp_cb_msg_t cb_msg;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return BCM_RX_NOT_HANDLED;
    }

    if (unit_rx_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) {
        SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
            "%s() failed %s\n", __func__,
            "Rx unit data not initialized"));
        return BCM_RX_NOT_HANDLED;
    }

#ifdef PTP_RX_CALLBACK_DEBUG
    soc_cm_print("_bcm_ptp_rx_callback(%d,%d)\n", pkt->pkt_len, BCM_PKT_IEEE_LEN(pkt));
        _bcm_ptp_dump_hex(BCM_PKT_IEEE(pkt), BCM_PKT_IEEE_LEN(pkt), 0);
#endif

    if (pkt->pkt_data[0].len < pkt->pkt_len ||
            pkt->pkt_len < PTP_RX_PACKET_MIN_SIZE_OCTETS) {
        /*
         * Ignore packet.
         * NOTICE: inconsistent or incompatible packet length.
         */
        return BCM_RX_NOT_HANDLED;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_rx_message_length_get(BCM_PKT_IEEE(pkt),
            &message_len))) {
        PTP_ERROR_FUNC("_bcm_ptp_rx_message_length_get()");
        return BCM_RX_NOT_HANDLED;
    }

    /*
     * Parse packet data.
     */

    /** Get destination port from UDP header, which is a proxy for packet
     * type and subsequent handling.
     */
    if (BCM_FAILURE(rv = _bcm_ptp_rx_message_destination_port_get(
             BCM_PKT_IEEE(pkt), &udp_dest_port))) {
        PTP_ERROR_FUNC("_bcm_ptp_rx_message_destination_port_get()");
        return BCM_RX_NOT_HANDLED;
    }

    switch (udp_dest_port) {
    case (0x0140):
        /* Response message. */

        /* Sanity / Security check: Validate VLAN & MAC information for this clock */
        tpid = _bcm_ptp_uint16_read(BCM_PKT_IEEE(pkt) + 12);  /* 12: fixed offset for TPID/VLAN */
        vlan =  _bcm_ptp_uint16_read(BCM_PKT_IEEE(pkt) + 14);

        if (tpid != unit_rx_array[unit].stack_array[cb_ptp_id].tpid ||
            vlan != unit_rx_array[unit].stack_array[cb_ptp_id].vlan) {
            /* This is a PTP message, but not from our stack (since it is on the wrong VLAN). */
            return BCM_RX_NOT_HANDLED;
        }

        if (pkt->pkt_len < PTP_RX_MGMT_MIN_SIZE) {
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "%s() failed %s\n", __func__,
                "Bad response len"));
            return BCM_RX_NOT_HANDLED;
        }

        /*
         * Parse packet data.
         * Lookup the unit number, PTP stack ID, and PTP clock number
         * association of the packet based on the sender's PTP clock
         * identity.
         */
        if (BCM_FAILURE(rv = _bcm_ptp_rx_message_source_clock_identity_get(
                BCM_PKT_IEEE(pkt), &cb_clock_identity))) {
            PTP_ERROR_FUNC("_bcm_ptp_rx_message_source_clock_identity_get()");
            return BCM_RX_NOT_HANDLED;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_clock_lookup(cb_clock_identity, unit,
                                                   cb_ptp_id, &cb_clock_num))) {
            /* Not from one of this stack's clocks, so leave it */
            return BCM_RX_NOT_HANDLED;
        }

        /*
         * Ensure that Rx framework is initialized and Rx data structures are
         * created for the requisite unit and PTP stack.
         */
        if ((unit_rx_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
                (unit_rx_array[unit].stack_array[cb_ptp_id].memstate !=
                PTP_MEMSTATE_INITIALIZED)) {
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "%s() failed %s\n", __func__,
                "Rx unit/stack data not initialized"));
            return BCM_RX_NOT_HANDLED;
        }

        if (sal_memcmp(BCM_PKT_IEEE(pkt) + sizeof(bcm_mac_t),
                       unit_rx_array[unit].stack_array[cb_ptp_id].top_mac,
                       sizeof(bcm_mac_t)) != 0) {
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "%s() failed %s\n", __func__, "Bad response smac"));
        }

        if (sal_memcmp(BCM_PKT_IEEE(pkt),
                       unit_rx_array[unit].stack_array[cb_ptp_id].host_mac,
                       sizeof(bcm_mac_t)) != 0) {
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "%s() failed %s\n", __func__, "Bad response dmac"));
        }

        /* Lock. */
        spl = sal_splhi();

        prior_data = unit_rx_array[unit].stack_array[cb_ptp_id]
                                           .clock_data[cb_clock_num]
                                           .response_data;

        unit_rx_array[unit].stack_array[cb_ptp_id]
                              .clock_data[cb_clock_num]
                              .response_data = pkt->pkt_data[0].data + PTP_RX_UDP_PAYLOAD_OFFSET;

        unit_rx_array[unit].stack_array[cb_ptp_id]
                              .clock_data[cb_clock_num]
                              .response_len =  pkt->pkt_len - PTP_RX_UDP_PAYLOAD_OFFSET;

        /* Unlock. */
        sal_spl(spl);

        /*
         * Free unclaimed response.
         * NOTICE: If prior data exists, it must be freed.
         */
        if (prior_data) {
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "%s() failed %s\n", __func__, "Unclaimed response free'd."));

            SET_PTP_INFO;
            stack_p = &ptp_info_p->stack_info[cb_ptp_id];

            stack_p->rx_free(unit, cb_ptp_id, prior_data);
        }

        rv = _bcm_ptp_sem_give(unit_rx_array[unit].stack_array[cb_ptp_id]
                     .clock_data[cb_clock_num].response_ready);
        if (BCM_FAILURE(rv)) {
            PTP_ERROR_FUNC("_bcm_ptp_sem_give()");
        }

        return BCM_RX_HANDLED_OWNED;

        break;

    case (0x0141):
        /* Forwarded (tunnel) message. */
        if (message_len < PTP_RX_TUNNEL_MSG_MIN_SIZE_OCTETS) {
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "Invalid (too-short) tunnel message received (0x%04x)\n",
                message_len));
            return BCM_RX_HANDLED;
        }

        /*
         * Parse wrapping header.
         * Move cursor forward to "remove" wrapping header.
         *
         * NOTICE: Forwarded tunnel message prepends an 11-element header, which
         *         includes PTP message and addressing metadata.
         *         Wrapping Header Octet  0      : Instance number.
         *         Wrapping Header Octets 1...2  : Local (receiving) port number.
         *         Wrapping Header Octets 3...4  : Message type.
         *         Wrapping Header Octets 5...6  : Ethertype.
         *         Wrapping Header Octets 7...8  : Source address offset.
         *         Wrapping Header Octets 9...10 : PTP payload offset.
         */
        i = PTP_PTPHDR_START_IDX;

        wrapClockNumber = *(BCM_PKT_IEEE(pkt) + i);
        ++i;
        --message_len;

        wrapPortNumber = _bcm_ptp_uint16_read(BCM_PKT_IEEE(pkt) + i);
        i += sizeof(uint16);
        message_len -= sizeof(uint16);

        wrapMessageType = _bcm_ptp_uint16_read(BCM_PKT_IEEE(pkt) + i);
        i += sizeof(uint16);
        message_len -= sizeof(uint16);

        wrapProtocol = _bcm_ptp_uint16_read(BCM_PKT_IEEE(pkt) + i);
        i += sizeof(uint16);
        message_len -= sizeof(uint16);

        wrapSrcAddrOffset = _bcm_ptp_uint16_read(BCM_PKT_IEEE(pkt) + i);
        i += sizeof(uint16);
        message_len -= sizeof(uint16);

        wrapPtpOffset = _bcm_ptp_uint16_read(BCM_PKT_IEEE(pkt) + i);
        i += sizeof(uint16);
        message_len -= sizeof(uint16);

        
        _bcm_ptp_most_recent_clock_num = wrapClockNumber;
        _bcm_ptp_most_recent_port = wrapPortNumber;
        _bcm_ptp_most_recent_protocol = wrapProtocol;
        _bcm_ptp_most_recent_src_addr_offset = wrapSrcAddrOffset;
        _bcm_ptp_most_recent_msg_offset = wrapPtpOffset;

        /*
         * Parse packet data.
         * Lookup the unit number, PTP stack ID, and PTP clock number
         * association of the packet based on the recipient's PTP clock
         * identity.
         */
        if (wrapMessageType == bcmPTP_MESSAGE_TYPE_SIGNALING ||
            wrapMessageType == bcmPTP_MESSAGE_TYPE_MANAGEMENT) {
            /* PTP tunneled message. */
            sal_memcpy(cb_clock_identity,
                       BCM_PKT_IEEE(pkt) + i + wrapPtpOffset + PTP_PTPHDR_SIZE_OCTETS,
                       sizeof(bcm_ptp_clock_identity_t));

            /* Peer information for counts of Rx signaling and external management messages. */
            sal_memcpy(peer_clockIdentity,
                       BCM_PKT_IEEE(pkt) + i + wrapPtpOffset + PTP_PTPHDR_SRCPORT_OFFSET_OCTETS,
                       sizeof(bcm_ptp_clock_identity_t));

            peer_portAddress.addr_type = wrapProtocol;
            sal_memset(peer_portAddress.address, 0, BCM_PTP_MAX_NETW_ADDR_SIZE);
            switch (wrapProtocol) {
            case bcmPTPUDPIPv4:
                sal_memcpy(peer_portAddress.address,
                           BCM_PKT_IEEE(pkt) + i + wrapSrcAddrOffset, PTP_IPV4_ADDR_SIZE_BYTES);
                break;
            case bcmPTPUDPIPv6:
                sal_memcpy(peer_portAddress.address,
                           BCM_PKT_IEEE(pkt) + i + wrapSrcAddrOffset, PTP_IPV6_ADDR_SIZE_BYTES);
                break;
            case bcmPTPIEEE8023:
                sal_memcpy(peer_portAddress.address,
                           BCM_PKT_IEEE(pkt) + i + wrapSrcAddrOffset, PTP_MAC_ADDR_SIZE_BYTES);
                break;
            default:
                ;
            }
        } else {
            /* Non-PTP tunneled message (e.g. ARP). */

            
            sal_memset(cb_clock_identity, 0xff, sizeof(bcm_ptp_clock_identity_t));
        }

        if (BCM_FAILURE(rv = _bcm_ptp_clock_lookup(cb_clock_identity, unit,
                                                   cb_ptp_id, &cb_clock_num))) {
            /* Not from one of this stack's clocks, so leave it */
            return BCM_RX_NOT_HANDLED;
        }

        /*
         * Ensure that Rx framework is initialized and Rx data structures are
         * created for the requisite unit and PTP stack.
         */
        if ((unit_rx_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
                (unit_rx_array[unit].stack_array[cb_ptp_id].memstate !=
                PTP_MEMSTATE_INITIALIZED)) {
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "%s() failed %s\n", __func__, "Rx unit/stack data not initialized"));
            return BCM_RX_NOT_HANDLED;
        }

        switch (wrapMessageType) {
        case bcmPTP_MESSAGE_TYPE_SIGNALING:
            /* Peer dataset counts of Rx signaling messages. */
            if (BCM_FAILURE(rv = _bcm_ptp_update_peer_counts(wrapPortNumber,
                    &peer_clockIdentity, &peer_portAddress, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0))) {
                PTP_ERROR_FUNC("_bcm_ptp_update_peer_counts()");
            }

            
            cb_flags = 0;

            if (unit_rx_array[unit].signal_cb) {
                cb_msg.flags = cb_flags;
                cb_msg.protocol = wrapProtocol;
                cb_msg.src_addr_offset = wrapSrcAddrOffset;
                cb_msg.msg_offset = wrapPtpOffset;
                cb_msg.length = message_len;
                cb_msg.data = BCM_PKT_IEEE(pkt) + i;

                unit_rx_array[unit].signal_cb(unit, cb_ptp_id,
                    cb_clock_num, wrapPortNumber,
                    bcmPTPCallbackTypeSignal, &cb_msg,
                    unit_rx_array[unit].signal_user_data);
            }
            break;

        case bcmPTP_MESSAGE_TYPE_MANAGEMENT:
            /* Peer dataset counts of Rx external management messages. */
            if (BCM_FAILURE(rv = _bcm_ptp_update_peer_counts(wrapPortNumber,
                    &peer_clockIdentity, &peer_portAddress, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0))) {
                PTP_ERROR_FUNC("_bcm_ptp_update_peer_counts()");
            }

            
            cb_flags = 0;

            if (unit_rx_array[unit].management_cb) {
                cb_msg.flags = cb_flags;
                cb_msg.protocol = wrapProtocol;
                cb_msg.src_addr_offset = wrapSrcAddrOffset;
                cb_msg.msg_offset = wrapPtpOffset;
                cb_msg.length = message_len;
                cb_msg.data = BCM_PKT_IEEE(pkt) + i;

                if (unit_rx_array[unit].management_cb(unit, cb_ptp_id,
                        cb_clock_num, wrapPortNumber,
                        bcmPTPCallbackTypeManagement, &cb_msg,
                        unit_rx_array[unit].management_user_data) == bcmPTPCallbackAccept) {
                    /* Tunnel message to ToP. */
                    if (BCM_FAILURE(rv =_bcm_ptp_tunnel_message_to_top(unit,
                            cb_ptp_id, message_len, BCM_PKT_IEEE(pkt) + i))) {
                        PTP_ERROR_FUNC("_bcm_ptp_tunnel_message_to_top()");
                    }
                }
            }
            break;

        case bcmPTP_MESSAGE_TYPE_ARP:
            if (_bcm_ptp_arp_callback) {
                _bcm_ptp_arp_callback(unit, cb_ptp_id, wrapProtocol, wrapSrcAddrOffset,
                    wrapPtpOffset, message_len, BCM_PKT_IEEE(pkt) + i);
            }
            break;

        default:
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "Invalid tunnel message received: "
                "unknown/unsupported type (0x%02x)\n", wrapMessageType));
        }

        break;

    case (0x0142):
        /* Event message. */
        if (message_len < PTP_RX_EVENT_MSG_MIN_SIZE_OCTETS) {
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "Invalid (too-short) event message received (0x%04x)\n",
                      message_len));
            return BCM_RX_HANDLED;
        }

        
        cb_ptp_id = 0;
        cb_clock_num = 0;
        cb_flags = 0;

        if (BCM_FAILURE(rv = _bcm_ptp_event_message_monitor(unit, cb_ptp_id,
                message_len, BCM_PKT_IEEE(pkt) + PTP_PTPHDR_START_IDX, &ev_internal))) {
                PTP_ERROR_FUNC("_bcm_ptp_event_message_monitor()");
        }

        if (unit_rx_array[unit].event_cb && !ev_internal) {
            i = PTP_PTPHDR_START_IDX;

            cb_msg.flags = cb_flags;
            cb_msg.protocol = bcmPTPUDPIPv4;
            cb_msg.src_addr_offset = 0;
            cb_msg.msg_offset = 0;
            cb_msg.length = message_len;
            cb_msg.data = BCM_PKT_IEEE(pkt) + i;

            unit_rx_array[unit].event_cb(unit, cb_ptp_id,
                cb_clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT,
                bcmPTPCallbackTypeEvent, &cb_msg,
                unit_rx_array[unit].event_user_data);
        }

        break;

    default:
#ifdef PTP_RX_CALLBACK_DEBUG
        soc_cm_print("UDP packet dst port 0x%04x not handled\n", udp_dest_port);
#endif
        return BCM_RX_NOT_HANDLED;
    }

    return BCM_RX_HANDLED;
}


/*
 * Function:
 *      _bcm_ptp_rx_message_destination_port_get
 * Purpose:
 *      Get destination port number in UDP header.
 * Parameters:
 *      message   - (IN)  PTP management message.
 *      dest_port - (OUT) Destination port number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static int
_bcm_ptp_rx_message_destination_port_get(
    uint8 *message,
    uint16 *dest_port)
{
    int i = PTP_UDPHDR_START_IDX + PTP_UDPHDR_DESTPORT_OFFSET_OCTETS;

    *dest_port = _bcm_ptp_uint16_read(message + i);
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_ptp_rx_message_source_clock_identity_get
 * Purpose:
 *      Get PTP source clock identity in PTP common header.
 * Parameters:
 *      message        - (IN)  PTP management message.
 *      clock_identity - (OUT) PTP source clock identity.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
static int
_bcm_ptp_rx_message_source_clock_identity_get(
    uint8 *message,
    bcm_ptp_clock_identity_t *clock_identity)
{
    int i = PTP_PTPHDR_START_IDX + PTP_PTPHDR_SRCPORT_OFFSET_OCTETS;

    sal_memcpy(clock_identity, message + i, sizeof(bcm_ptp_clock_identity_t));
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_rx_message_length_get
 * Purpose:
 *      Get the length of a message.
 * Parameters:
 *      message     - (IN)  PTP management message.
 *      message_len - (OUT) Message length (octets).
 * Returns:
 *      BCM_E_XXX.
 * Notes:
 *      Message length is size of Rx packet excluding headers.
 */
static int
_bcm_ptp_rx_message_length_get(
    uint8 *message,
    uint16 *message_len)
{
    int i = PTP_UDPHDR_START_IDX + PTP_UDPHDR_MSGLEN_OFFSET_OCTETS;

    *message_len = _bcm_ptp_uint16_read(message + i) - PTP_UDPHDR_SIZE_OCTETS;
    return BCM_E_NONE;
}

#endif /* defined(BCM_PTP_EXTERNAL_STACK_SUPPORT) */

/*
 * Function:
 *      _bcm_ptp_register_management_callback
 * Purpose:
 *      Register a management callback function
 * Parameters:
 *      unit - (IN) Unit number.
 *      cb - (IN) A pointer to the callback function to call for the specified PTP events
 *      user_data - (IN) Pointer to user data to supply in the callback
 * Returns:
 *      BCM_E_XXX.
 * Notes:
 *      The unit is already locked by the calling function
 */
int
_bcm_ptp_register_management_callback(
    int unit,
    bcm_ptp_cb cb,
    void *user_data)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    unit_rx_array[unit].management_cb = cb;
    unit_rx_array[unit].management_user_data = user_data;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_register_event_callback
 * Purpose:
 *      Register a event callback function
 * Parameters:
 *      unit - (IN) Unit number.
 *      cb - (IN) A pointer to the callback function to call for the specified PTP events
 *      user_data - (IN) Pointer to user data to supply in the callback
 * Returns:
 *      BCM_E_XXX.
 * Notes:
 *      The unit is already locked by the calling function
 */
int
_bcm_ptp_register_event_callback(
    int unit,
    bcm_ptp_cb cb,
    void *user_data)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    unit_rx_array[unit].event_cb = cb;
    unit_rx_array[unit].event_user_data = user_data;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_register_signal_callback
 * Purpose:
 *      Register a signal callback function
 * Parameters:
 *      unit - (IN) Unit number.
 *      cb - (IN) A pointer to the callback function to call for the specified PTP events
 *      user_data - (IN) Pointer to user data to supply in the callback
 * Returns:
 *      BCM_E_XXX.
 * Notes:
 *      The unit is already locked by the calling function
 */
int
_bcm_ptp_register_signal_callback(
    int unit,
    bcm_ptp_cb cb,
    void *user_data)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    unit_rx_array[unit].signal_cb = cb;
    unit_rx_array[unit].signal_user_data = user_data;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_register_peers_callback
 * Purpose:
 *      Register callback function to periodically get the peer counters
 * Parameters:
 *      unit - (IN) Unit number.
 *      cb - (IN) A pointer to the callback function to call for the specified PTP events
 *      user_data - (IN) Pointer to user data to supply in the callback
 * Returns:
 *      BCM_E_XXX.
 * Notes:
 *      The unit is already locked by the calling function
 */
int
_bcm_ptp_register_peers_callback(
    int unit,
    bcm_ptp_cb cb,
    void *user_data)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    unit_rx_array[unit].peers_cb = cb;
    unit_rx_array[unit].peers_user_data = user_data;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_register_fault_callback
 * Purpose:
 *      Register a fault callback function
 * Parameters:
 *      unit - (IN) Unit number.
 *      cb - (IN) A pointer to the callback function to call for the specified PTP events
 *      user_data - (IN) Pointer to user data to supply in the callback
 * Returns:
 *      BCM_E_XXX.
 * Notes:
 *      The unit is already locked by the calling function
 */
int
_bcm_ptp_register_fault_callback(
    int unit,
    bcm_ptp_cb cb,
    void *user_data)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    unit_rx_array[unit].fault_cb = cb;
    unit_rx_array[unit].fault_user_data = user_data;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_unregister_management_callback
 * Purpose:
 *      Unregister a management callback function
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX.
 * Notes:
 *      The unit is already locked by the calling function
 */
int
_bcm_ptp_unregister_management_callback(
    int unit)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    unit_rx_array[unit].management_cb = NULL;
    unit_rx_array[unit].management_user_data = NULL;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_unregister_event_callback
 * Purpose:
 *      Unregister a event callback function
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX.
 * Notes:
 *      The unit is already locked by the calling function
 */
int
_bcm_ptp_unregister_event_callback(
    int unit)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    unit_rx_array[unit].event_cb = NULL;
    unit_rx_array[unit].event_user_data = NULL;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_unregister_signal_callback
 * Purpose:
 *      Unregister a signal callback function
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX.
 * Notes:
 *      The unit is already locked by the calling function
 */
int
_bcm_ptp_unregister_signal_callback(
    int unit)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    unit_rx_array[unit].signal_cb = NULL;
    unit_rx_array[unit].signal_user_data = NULL;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_unregister_peers_callback
 * Purpose:
 *      Unregister a check peers callback function
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX.
 * Notes:
 *      The unit is already locked by the calling function
 */
int
_bcm_ptp_unregister_peers_callback(
    int unit)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    unit_rx_array[unit].peers_cb = NULL;
    unit_rx_array[unit].peers_user_data = NULL;

    return rv;
}


/*
 * Function:
 *      _bcm_ptp_unregister_fault_callback
 * Purpose:
 *      Unregister a fault callback function
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX.
 * Notes:
 *      The unit is already locked by the calling function
 */
int
_bcm_ptp_unregister_fault_callback(
    int unit)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    unit_rx_array[unit].fault_cb = NULL;
    unit_rx_array[unit].fault_user_data = NULL;

    return rv;
}

#if defined(BCM_PTP_INTERNAL_STACK_SUPPORT) || defined(BCM_PTP_EXTERNAL_STACK_SUPPORT)
/*
 * Function:
 *      _bcm_ptp_event_message_monitor
 * Purpose:
 *      Monitor incoming event messages and perform basic operations req'd
 *      for maintenance of host caches.
 * Parameters:
 *      unit        - (IN)  Unit number.
 *      ptp_id      - (IN)  PTP stack ID.
 *      ev_data_len - (IN)  Event message data length (octets).
 *      ev_data     - (IN)  Event message data.
 *      ev_internal - (OUT) Internal-only event. Do not pass to event callbacks.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
STATIC int
_bcm_ptp_event_message_monitor(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int ev_data_len,
    uint8 *ev_data,
    int *ev_internal)
{
    int rv;

    uint8 event_clock_num;
    uint16 event_port_num;
    _bcm_ptp_event_t event_code;
    _bcm_ptp_port_state_t portState;

    int cursor = 0;

    event_code = (_bcm_ptp_event_t)_bcm_ptp_uint16_read(ev_data + cursor);
    cursor += sizeof(uint16);

    if (event_code == _bcm_ptp_state_change_event) {
        /* EVENT MONITOR: portState change. */
        *ev_internal = 0;

        /* Extract clock number and port number of event. */
        event_clock_num = ev_data[cursor++];
        event_port_num = _bcm_ptp_uint16_read(ev_data + cursor);
        cursor += sizeof(uint16);

        /* Advance cursor and extract portState of event. */
        cursor += BCM_PTP_CLOCK_EUID_IEEE1588_SIZE + 2;
        portState = (_bcm_ptp_port_state_t)ev_data[cursor];

        if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_port_state_set(unit, ptp_id,
                event_clock_num, event_port_num, portState))) {
            return rv;
        }
    } else if (event_code == _bcm_ptp_tdev_event) {
        /* EVENT MONITOR: TDEV data analysis. */
        *ev_internal = 1;

        if (BCM_FAILURE(rv = _bcm_ptp_ctdev_gateway(unit, ptp_id, PTP_CLOCK_NUMBER_DEFAULT,
                ev_data_len, ev_data))) {
            return rv;
        }
    }

    return BCM_E_NONE;
}
#endif

/*
 * Function:
 *      _bcm_ptp_signal_handler_default
 * Purpose:
 *      Default forwarded (tunneled) PTP signaling message callback handler.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP port number.
 *      type       - (IN) Callback function type.
 *      msg        - (IN) Callback message data.
 *      user_data  - (IN) Callback user data.
 * Returns:
 *      BCM_E_XXX (if failure)
 *      bcmPTPCallbackAccept (if success)
 * Notes:
 */
STATIC int
_bcm_ptp_signal_handler_default(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num, 
    uint32 clock_port, 
    bcm_ptp_cb_type_t type,
    bcm_ptp_cb_msg_t *msg,
    void *user_data)
{
    int rv = BCM_E_UNAVAIL;
    
    int port_num = _bcm_ptp_most_recent_port;
    bcm_ptp_protocol_t protocol = _bcm_ptp_most_recent_protocol;
    int src_addr_offset = _bcm_ptp_most_recent_src_addr_offset;
    int msg_offset = _bcm_ptp_most_recent_msg_offset;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

#ifdef PTP_RX_CALLBACK_DEBUG
    soc_cm_print("Signaling callback (Unit = %d, PTP Stack = %d)\n", unit, ptp_id);
    _bcm_ptp_dump_hex(msg->data, msg->length,0);
#endif

    return _bcm_ptp_process_incoming_signaling_msg(unit, ptp_id, clock_num, port_num, protocol, src_addr_offset,
                                                   msg_offset, msg->length, msg->data);
}

#endif /* defined(INCLUDE_PTP) */
