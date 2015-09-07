/*
 * $Id: mbox.c,v 1.10 Broadcom SDK $
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
 * Shared Memory Mailbox infrastruction - SHM mbox communications & debug logging
 */

#include <shared/bsl.h>

#include <bcm_int/common/mbox.h>

#include <soc/drv.h>
#include <soc/uc_msg.h>
#include <shared/bsl.h>
#include <sal/core/libc.h>
#include <sal/core/dpc.h>

#include <bcm/types.h>
#include <bcm/error.h>

#define _MBOX_ERROR_FUNC(func)                                  \
    LOG_CLI((BSL_META("%s() FAILED "                            \
                      func " returned %d : %s\n"),              \
             FUNCTION_NAME(), rv, bcm_errmsg(rv)))


#define _BCM_MBOX_HEXDUMP_LINE_WIDTH_OCTETS   (100)

#if defined(BCM_CMICM_SUPPORT)
#define _BCM_MBOX_LOCAL_DEBUGBUFSIZE          (1024)
static _bcm_bs_internal_comm_info_t mbox_info;

static char local_debugbuf[_BCM_MBOX_LOCAL_DEBUGBUFSIZE];
static int local_head = 0;

static char output_debugbuf[_BCM_MBOX_LOCAL_DEBUGBUFSIZE * 2];
static int local_tail = 0;
#endif /* BCM_CMICM_SUPPORT */

static uint32 _cmicm_debug_flags = 0;

#if defined(BCM_CMICM_SUPPORT)
STATIC void _bcm_mbox_rx_thread(void *arg);

STATIC void _bcm_mbox_debug_poll(void* owner, void* time_as_ptr, void *unit_as_ptr,
                                 void *unused_2, void* unused_3);
#endif /* BCM_CMICM_SUPPORT */

/*
 * Function:
 *      _cmicm_dump_hex
 * Purpose:
 *      Print hexadecimal buffer.
 * Parameters:
 *      buf - (IN) buffer to be printed
 *      len - (IN) message length
 *      indent - (IN) number of spaces to indent
 * Returns:
 *      none
 * Notes:
 */
void
_bcm_dump_hex(uint8 *buf, int len, int indent)
{
    char line[_BCM_MBOX_HEXDUMP_LINE_WIDTH_OCTETS];
    int i,j;
    int linepos = 0;

    /*
     * Limit number of spaces to indent based on line width and the
     * maximum hexadecimal data width (32 x 3 + 1 null terminator).
     */
    indent = indent > (_BCM_MBOX_HEXDUMP_LINE_WIDTH_OCTETS - 97) ?
             (_BCM_MBOX_HEXDUMP_LINE_WIDTH_OCTETS - 97) : indent;

    for (j = 0; j < indent; j++) {
        sal_sprintf(line+linepos++," ");
    }
    for (i = 0; i < len; ++i) {
        sal_sprintf(line + linepos, "%02x", *buf++);
        linepos += 2;
        sal_sprintf(line + linepos, "  ");
        ++linepos;

        if ((i & 0x1f) == 0x1f) {
            LOG_CLI((BSL_META("%s\n"), line));
            line[0] = 0;
            linepos = 0;
            for (j = 0; j < indent; j++) {
                sal_sprintf(line+linepos++," ");
            }
        }
    }

    /* output last line if it wasn't complete */
    if (len & 0x1f) {
        LOG_CLI((BSL_META("%s\n"), line));
    }
}


/* ************************************ Transport **************************************/


int
_bcm_mbox_comm_init(int unit, int appl_type)
{
#if defined(BCM_CMICM_SUPPORT)
    int rv = BCM_E_NONE;
    int timeout_usec = 1900000;
    int max_num_cores = 2;
    int result;
    int c;
    int i;

    /* Init the system if this is the first time in */
    if (mbox_info.unit_state == NULL) {
        mbox_info.unit_state = soc_cm_salloc(unit, sizeof(_bcm_bs_internal_stack_state_t) * BCM_MAX_NUM_UNITS,
                                            "mbox_info_unit_state");
        sal_memset(mbox_info.unit_state, 0, sizeof(_bcm_bs_internal_stack_state_t) * BCM_MAX_NUM_UNITS);
    }

    /* Init the unit if this is the first time for the unit */
    if (mbox_info.unit_state[unit].mboxes == NULL) {
        /* allocate space for mboxes */
        mbox_info.unit_state[unit].mboxes = soc_cm_salloc(unit, sizeof(_bcm_bs_internal_stack_mboxes_t), "bs msg");

        if (!mbox_info.unit_state[unit].mboxes) {
            return BCM_E_MEMORY;
        }

        /* clear state of message mboxes */
        mbox_info.unit_state[unit].mboxes->num_buffers = soc_ntohl(_BCM_MBOX_MAX_BUFFERS);
        for (i = 0; i < _BCM_MBOX_MAX_BUFFERS; ++i) {
            mbox_info.unit_state[unit].mboxes->status[i] = soc_htonl(_BCM_MBOX_MS_EMPTY);
        }

        mbox_info.comm_available = sal_sem_create("BCM BS comms", sal_sem_BINARY, 0);
        rv = sal_sem_give(mbox_info.comm_available);

        mbox_info.unit_state[unit].response_ready = sal_sem_create("CMICM_resp", sal_sem_BINARY, 0);

        sal_thread_create("CMICM Rx", SAL_THREAD_STKSZ,
                          soc_property_get(unit, spn_UC_MSG_THREAD_PRI, 50) + 1,
                          _bcm_mbox_rx_thread,  INT_TO_PTR(unit));

        /* allocate space for debug log */
        /* size is the size of the structure without the placeholder space for debug->buf, plus the real space for it */
        mbox_info.unit_state[unit].log = soc_cm_salloc(unit, sizeof(_bcm_bs_internal_stack_log_t)
                                                       - sizeof(mbox_info.unit_state[unit].log->buf)
                                                       + _BCM_MBOX_MAX_LOG, "bs log");
        if (!mbox_info.unit_state[unit].log) {
            soc_cm_sfree(unit, mbox_info.unit_state[unit].mboxes);
            return BCM_E_MEMORY;
        }

        /* initialize debug */
        mbox_info.unit_state[unit].log->size = soc_htonl(_BCM_MBOX_MAX_LOG);
        mbox_info.unit_state[unit].log->head = 0;
        mbox_info.unit_state[unit].log->tail = 0;

        /* set up the network-byte-order pointers so that CMICm can access the shared memory */
        mbox_info.unit_state[unit].mbox_ptr = soc_htonl(soc_cm_l2p(unit, (void*)mbox_info.unit_state[unit].mboxes));
        mbox_info.unit_state[unit].log_ptr = soc_htonl(soc_cm_l2p(unit, (void*)mbox_info.unit_state[unit].log));

        /* LOG_CLI((BSL_META_U(unit,
                               "DEBUG SPACE: %p\n"), (void *)mbox_info.unit_state[unit].log->buf)); */
 
        rv = BCM_E_UNAVAIL;
        for (c = max_num_cores - 1; c >= 0; c--) {
            /* LOG_CLI((BSL_META_U(unit,
                                   "Trying BS on core %d\n"), c)); */
            result = soc_cmic_uc_appl_init(unit, c, MOS_MSG_CLASS_BS, timeout_usec,
                                           _BCM_MBOX_SDK_VERSION, _BCM_MBOX_UC_MIN_VERSION);

            if (SOC_E_NONE == result){
                /* uKernel communcations started successfully, so run the init */
                /* Note: the length of this message is unused, and can be overloaded */
                mos_msg_data_t start_msg;
                start_msg.s.mclass = MOS_MSG_CLASS_BS;
                start_msg.s.subclass = MOS_MSG_SUBCLASS_MBOX_CONFIG;
                _shr_uint16_write((uint8*)(&(start_msg.s.len)), (uint16) appl_type);

                start_msg.s.data = bcm_htonl(soc_cm_l2p(unit, (void*)&mbox_info.unit_state[unit]));

                if (BCM_FAILURE(rv = soc_cmic_uc_msg_send(unit, c, &start_msg, timeout_usec))) {
                    _MBOX_ERROR_FUNC("soc_cmic_uc_msg_send()");
                }

                mbox_info.unit_state[unit].core_num = c;
                break;
            }

            /* LOG_CLI((BSL_META_U(unit,
                                   "No response on core %d\n"), c)); */
        }

        if (BCM_FAILURE(rv)) {
            LOG_CLI((BSL_META_U(unit,
                                "No response from CMICm core(s)\n")));
            return rv;
        }

        _bcm_mbox_debug_poll(INT_TO_PTR(&_bcm_mbox_debug_poll), INT_TO_PTR(1000), INT_TO_PTR(unit), 0, 0);
    }

    return rv;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}


int
_bcm_mbox_tx(
    int unit,
    uint32 node_num,
    _bcm_mbox_transport_type_t transport,
    uint8 *message,
    int message_len)
{
#if defined(BCM_CMICM_SUPPORT)
    int rv = BCM_E_NONE;

    mos_msg_data_t uc_msg;

    int wait_iter = 0;

    /* LOG_CLI((BSL_META_U(unit,
                           "cmic_tx Len:%d\n"), message_len)); */
    /* _bcm_dump_hex(message, message_len, 4); */

    if (mbox_info.unit_state[unit].mboxes->status[0] != soc_htonl(_BCM_MBOX_MS_EMPTY)) {
        /* char this_thread_name[100]; */
        /* sal_thread_name(sal_thread_self(), this_thread_name, 100); */
        /* ptp_printf("******* Contention, status %d (%s is sending, %s wants to send)\n", */
        /*            mbox_info.unit_state[unit].mboxes->status[0], holding_thread_name, this_thread_name); */
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Contention\n")));
    }

    /* wait for to-TOP buffer to be free, if it is not already */
    while (mbox_info.unit_state[unit].mboxes->status[0] != soc_htonl(_BCM_MBOX_MS_EMPTY) && wait_iter < 100000) {
        ++wait_iter;
        sal_usleep(1);
    }

    if (mbox_info.unit_state[unit].mboxes->status[0] != soc_htonl(_BCM_MBOX_MS_EMPTY)) {
        LOG_CLI((BSL_META_U(unit,
                            "TOP message buffer in use on Tx, re-pinging\n")));
        rv = soc_cmic_uc_msg_send(unit, mbox_info.unit_state[unit].core_num, &uc_msg, 1000000);
        return BCM_E_FAIL;
    }
    /* sal_thread_name(sal_thread_self(), holding_thread_name, 100); */

    if (wait_iter > 0) {
        /* ptp_printf("Wait to send outgoing to ToP: %d\n", wait_iter); */
    }

    /* load the mailbox */
    sal_memcpy((uint8 *)mbox_info.unit_state[unit].mboxes->mbox[0].data, message, message_len);
    mbox_info.unit_state[unit].mboxes->mbox[0].data_len = soc_htonl(message_len);
    mbox_info.unit_state[unit].mboxes->mbox[0].node_num = 0;  

    /* finish mbox load by setting status */
    switch (transport) {
    case _BCM_MBOX_MESSAGE:
        mbox_info.unit_state[unit].mboxes->status[0] = soc_htonl(_BCM_MBOX_MS_CMD);
        break;
    case _BCM_MBOX_TUNNEL_TO:
        mbox_info.unit_state[unit].mboxes->status[0] = soc_htonl(_BCM_MBOX_MS_TUNNEL_TO);
        break;
    case _BCM_MBOX_TUNNEL_OUT:
        mbox_info.unit_state[unit].mboxes->status[0] = soc_htonl(_BCM_MBOX_MS_TUNNEL_OUT);
        break;
    default:
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Unknown transport type"));
    }

    /* Send a notification to the CMICm */
    sal_memset(&uc_msg, 0, sizeof(uc_msg));
    uc_msg.s.mclass = MOS_MSG_CLASS_BS;
    uc_msg.s.subclass = MOS_MSG_SUBCLASS_MBOX_CMDRESP;
    uc_msg.s.len = message_len;
    uc_msg.s.data = 0;

    rv = soc_cmic_uc_msg_send(unit, mbox_info.unit_state[unit].core_num, &uc_msg, 1000000);
    return rv;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}


int
_bcm_mbox_tx_completion(
    int unit,
    uint32 node_num)
{
#if defined(BCM_CMICM_SUPPORT)
    int iter;

    for (iter = 0; iter < 10000; ++iter) {
        if (mbox_info.unit_state[unit].mboxes->status[0] == soc_htonl(_BCM_MBOX_MS_EMPTY)) {
            /* sal_strcpy(holding_thread_name, "none"); */
            return BCM_E_NONE;
        }
        sal_usleep(1);
    }

    /* sal_strcpy(holding_thread_name + strlen(holding_thread_name), "-over"); */
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Failed async Tx to ToP.  No clear\n")));
    return BCM_E_TIMEOUT;
#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}


int
_bcm_mbox_rx_response_free(int unit, uint8 *resp_data)
{
#if defined(BCM_CMICM_SUPPORT)
    unsigned i;

    for (i = 0; i < _BCM_MBOX_MAX_BUFFERS; ++i) {
        if (mbox_info.unit_state[unit].mboxes->mbox[i].data == resp_data) {
            mbox_info.unit_state[unit].mboxes->status[i] = soc_htonl(_BCM_MBOX_MS_EMPTY);
            return BCM_E_NONE;
        }
    }

    LOG_CLI((BSL_META_U(unit,
                        "Invalid CMICM rx response free (%p)\n"), (void *)resp_data));

    return BCM_E_NOT_FOUND;
#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
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
_bcm_mbox_rx_response_get(
    int unit,
    int node_num,
    int usec,
    uint8 **data,
    int *data_len)
{
#if defined(BCM_CMICM_SUPPORT)
    int rv = BCM_E_UNAVAIL;
    int spl;
    sal_usecs_t expiration_time = sal_time_usecs() + usec;

    /* LOG_CLI((BSL_META_U(unit,
                           "cmic_rx_get\n"))); */

    rv = BCM_E_FAIL;
    /* ptp_printf("Await resp @ %d\n", (int)sal_time_usecs()); */

    while (BCM_FAILURE(rv) && (int32) (sal_time_usecs() - expiration_time) < 0) {
        rv = sal_sem_take(mbox_info.unit_state[unit].response_ready, usec);
    }
    if (BCM_FAILURE(rv)) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Failed management Tx to ToP\n")));
        _MBOX_ERROR_FUNC("_bcm_ptp_sem_take()");
        return rv;
    }

    /* Lock. */
    spl = sal_splhi();

    *data     = mbox_info.unit_state[unit].response_data;
    *data_len = mbox_info.unit_state[unit].response_len;

    mbox_info.unit_state[unit].response_data = 0;

    /* Unlock. */
    sal_spl(spl);

    return rv;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}


#if defined(BCM_CMICM_SUPPORT)

STATIC
void
_bcm_mbox_rx_thread(void *arg)
{
    int rv = 0;

    int unit = PTR_TO_INT(arg);

    while (1) {
        int mbox;


        /* The uc_msg is just a signal that there is a message somewhere to get, so look through all mboxes */

        sal_usleep(10000);  


        for (mbox = 0; mbox < _BCM_MBOX_MAX_BUFFERS; ++mbox) {

            switch (soc_ntohl(mbox_info.unit_state[unit].mboxes->status[mbox])) {
            case _BCM_MBOX_MS_TUNNEL_IN:
                break;

            case _BCM_MBOX_MS_EVENT:
                break;

            case _BCM_MBOX_MS_RESP:
                {

                    mbox_info.unit_state[unit].response_data = (uint8*)mbox_info.unit_state[unit].mboxes->mbox[mbox].data;
                    mbox_info.unit_state[unit].response_len  = soc_ntohl(mbox_info.unit_state[unit].mboxes->mbox[mbox].data_len);

                    rv = sal_sem_give(mbox_info.unit_state[unit].response_ready);
                    if (BCM_FAILURE(rv)) {
                        _MBOX_ERROR_FUNC("_bcm_ptp_sem_give()");
                    }

                    
                    mbox_info.unit_state[unit].mboxes->status[mbox] = soc_htonl(_BCM_MBOX_MS_EMPTY);
                }
                break;
            }
        }
    }
}

#endif /* BCM_CMICM_SUPPORT */


int
_bcm_mbox_txrx(
    int unit,
    uint32 node_num,
    _bcm_mbox_transport_type_t transport,
    uint8 *out_data,
    int out_len,
    uint8 *in_data,
    int *in_len)
{
#if defined(BCM_CMICM_SUPPORT)
    int rv;
    uint8 *response_data;
    int response_len;

    /* LOG_CLI((BSL_META_U(unit,
                           "cmic_txrx tx Len:%d\n"), out_len)); */
    /* _bcm_dump_hex(out_data, out_len, 4); */

    int max_response_len = (in_len) ? *in_len : 0;
    if (in_len) {
        *in_len = 0;
    }

    rv = sal_sem_take(mbox_info.comm_available, _BCM_MBOX_RESPONSE_TIMEOUT_US);
    if (BCM_FAILURE(rv)) {
        _MBOX_ERROR_FUNC("sal_sem_take()");
        return rv;
    }

    rv = _bcm_mbox_tx(unit, node_num, _BCM_MBOX_MESSAGE, out_data, out_len);

    if (rv != BCM_E_NONE) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "Tx failed"));
        goto release_mgmt_lock;
    }

    /*
     * Get rx buffer, either from rx callback or from cmicm wait task
     * NOTICE: This call will return an rx response buffer that we will need to
     *         release by notifying the Rx section
     */
    rv = _bcm_mbox_rx_response_get(unit, node_num, _BCM_MBOX_RESPONSE_TIMEOUT_US,
                                   &response_data, &response_len);
    if (BCM_FAILURE(rv)) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "No Response"));
        goto release_mgmt_lock;
    }

    
    if (in_data && in_len) {
        if (response_len > max_response_len) {
            response_len = max_response_len;
        }

        *in_len = response_len;
        sal_memcpy(in_data, response_data, response_len);
    }

    /* LOG_CLI((BSL_META_U(unit,
                           "cmic_txrx rx Len:%d\n"), *in_len)); */
    /* _bcm_dump_hex(in_data, *in_len, 4); */

    rv = BCM_E_NONE;

/* dispose_of_resp: */
    _bcm_mbox_rx_response_free(unit, response_data);

release_mgmt_lock:
    rv = sal_sem_give(mbox_info.comm_available);
    if (BCM_FAILURE(rv)) {
        _MBOX_ERROR_FUNC("sal_sem_give()");
    }

    return rv;

#else  /* BCM_CMICM_SUPPORT */
    return BCM_E_UNAVAIL;
#endif /* BCM_CMICM_SUPPORT */
}

#if defined(BCM_CMICM_SUPPORT)
/* Dump anything new in the debug buffers.
 *  owner:         The owner if this is being called as a DPC.
 *  time_as_ptr:   Recurrence time if a follow-up DPC to this func should be scheduled.
 *  unit_as_ptr:   unit
 */
STATIC void
_bcm_mbox_debug_poll(void* owner, void* time_as_ptr, void *unit_as_ptr, void *unused_2, void* unused_3)
{
    int callback_time = (int)(size_t)time_as_ptr;
    int out_idx = 0;

    /* output the local debug first */
    while (local_tail != local_head) {
        char c = local_debugbuf[local_tail++];

        if (c) {
            output_debugbuf[out_idx++] = c;
        }

        if (local_tail == _BCM_MBOX_LOCAL_DEBUGBUFSIZE) {
            local_tail = 0;
        }
    }

    /* CMICM shared-mem output */
    {
        int unit = (int)(size_t)unit_as_ptr;
        uint32 head, size;
        if (soc_feature(unit, soc_feature_cmicm) || soc_feature(unit, soc_feature_iproc)) {
            /* head is written in externally, so will be in network byte order
             * tail is local, so we'll keep it in local endianness
             * size is read externally, so is also network byte order
             */
            head = soc_htonl(mbox_info.unit_state[unit].log->head);
            size = soc_htonl(mbox_info.unit_state[unit].log->size);

            while (mbox_info.unit_state[unit].log->tail != head) {
                char c = mbox_info.unit_state[unit].log->buf[mbox_info.unit_state[unit].log->tail++];
                if (c) {
                    output_debugbuf[out_idx++] = c;
                }
                if (mbox_info.unit_state[unit].log->tail == size) {
                    mbox_info.unit_state[unit].log->tail = 0;
                }
            }
        }
    }


    if (out_idx) {
        output_debugbuf[out_idx] = 0;
        if (_cmicm_debug_flags) {
            LOG_CLI((BSL_META("%s"), output_debugbuf));
        }
    }

    if (callback_time) {
        sal_dpc_time(callback_time, &_bcm_mbox_debug_poll,
                     0, time_as_ptr, unit_as_ptr, 0, 0);
    }
}

#endif /* BCM_CMICM_SUPPORT */
int
_bcm_mbox_debug_flag_set(uint32 flags)
{
    _cmicm_debug_flags = flags;

    return BCM_E_NONE;
}


int
_bcm_mbox_debug_flag_get(uint32 *flags)
{
    *flags = _cmicm_debug_flags;

    return BCM_E_NONE;
}
