/*
 * $Id: rpc.c 1.31 Broadcom SDK $
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
 * BCM Dispatch Remote Procedure Call Utilities
 */

#include <sdk_config.h>
#include <shared/alloc.h>
#include <sal/core/libc.h>
#include <sal/core/sync.h>
#include <sal/core/thread.h>

#include <shared/idents.h>

#include <bcm/types.h>
#include <bcm/error.h>
#include <bcm/init.h>
#include <bcm/debug.h>

#include <appl/cpudb/cpudb.h>
#include <appl/cputrans/atp.h>

#include <bcm_int/control.h>
#include <bcm_int/rpc/pack.h>
#include <bcm_int/rpc/rpc.h>
#include <bcm_int/rpc/server.h>
#include <bcm_int/client_dispatch.h>

#ifdef  BCM_RPC_SUPPORT

#define RPC_CLIENT_ID       SHARED_CLIENT_ID_RPC

#ifndef RPC_THREAD_STACK
#define RPC_THREAD_STACK    SAL_THREAD_STKSZ
#endif

#ifndef RPC_THREAD_PRIO
#define RPC_THREAD_PRIO     50
#endif

#ifndef RPC_REPLY_TIMEOUT
#define RPC_REPLY_TIMEOUT sal_sem_FOREVER
#endif

#define RPC_SLEEP   sal_sem_take(_rpc_server_sem, sal_sem_FOREVER)
#define RPC_WAKE    sal_sem_give(_rpc_server_sem)
#define RPC_SLOCK   sal_mutex_take(_rpc_server_lock, sal_mutex_FOREVER)
#define RPC_SUNLOCK sal_mutex_give(_rpc_server_lock)
#define RPC_CLOCK   sal_mutex_take(_rpc_client_lock, sal_mutex_FOREVER)
#define RPC_CUNLOCK sal_mutex_give(_rpc_client_lock)

#define RPC_OUT(stuff)  BCM_DEBUG(BCM_DBG_NORMAL, stuff)
#define RPC_ERR(stuff)  BCM_DEBUG(BCM_DBG_ERR, stuff)

/* client rpc request: stored on the requesting thread's stack */
typedef struct _rpc_creq_s {
    struct _rpc_creq_s  *next;
    sal_sem_t           sem;
    int                 unit;
    uint32              seq;
    uint8               *rbuf;
    void                *cookie;
#ifdef  BROADCOM_DEBUG
    sal_usecs_t         time;
#endif  /* BROADCOM_DEBUG */
} _rpc_creq_t;

static sal_mutex_t  _rpc_client_lock;
static uint32       _rpc_seq;
static _rpc_creq_t  *_rpc_creq;

static sal_mutex_t  _rpc_server_lock;
static sal_sem_t    _rpc_server_sem;
static sal_thread_t _rpc_server_thread = SAL_THREAD_ERROR;
static int          _rpc_thread_exit;
static bcm_rpc_sreq_t  *_rpc_sreq;
static bcm_rpc_sreq_t  *_rpc_sreq_tail;

int  _rpc_nexthop = 0;

#ifdef  BROADCOM_DEBUG
#define RPC_COUNT(_c)  ++(_c)
static volatile int _rpc_count_c_request;
static volatile int _rpc_count_c_reply;
static volatile int _rpc_count_c_fail;
static volatile int _rpc_count_c_timeout;
static volatile int _rpc_count_c_noreq;
static volatile int _rpc_count_c_detach;
static volatile int _rpc_count_s_request;
static volatile int _rpc_count_s_reply;
static volatile int _rpc_count_s_rretry;
static volatile int _rpc_count_s_rerr;
static volatile int _rpc_count_s_run;
static volatile int _rpc_count_s_wrongver;
static volatile int _rpc_count_s_nokey;
#else
#define RPC_COUNT(_c)
#endif  /* BROADCOM_DEBUG */

/*
 * Allocate DMAable memory suitable for an RPC packet.
 * Fill in an RPC header with appropriate arguments.
 * This memory is freed in bcm_rpc_request or bcm_rpc_reply.
 */
uint8 *
bcm_rpc_setup(uint8 dir, uint32 *key, uint32 len, uint32 seq, uint32 arg)
{
    uint8  *pkt, *pp;
    int    i;

    pkt = atp_tx_data_alloc(BCM_RPC_HLEN+len);
    if (pkt == NULL) {
        return NULL;
    }

    pp = pkt;
    BCM_PACK_U32(pp, seq);
    BCM_PACK_U8(pp, dir);
    BCM_PACK_U8(pp, BCM_RPC_VERSION);
    BCM_PACK_U16(pp, BCM_RPC_HLEN+len);
    for (i = 0; i < BCM_RPC_LOOKUP_KEYLEN; i++) {
        BCM_PACK_U32(pp, key ? key[i] : 0);
    }

    /*
     * arg is:
     * unit for client requests
     * return value for server responses
     */
    BCM_PACK_U32(pp, arg);

    return pkt;
}

/*
 * Free the payload of a received packet
 */
void
bcm_rpc_free(uint8 *buf, void *cookie)
{
    atp_rx_free((void *)buf, cookie);
}

/*
 * Unlink a request
 */
STATIC void
_bcm_rpc_unlink_request(_rpc_creq_t *req)
{
    _rpc_creq_t *preq;

    /* take req out of _rpc_creq */
    RPC_CLOCK;
    if (_rpc_creq == req) {
        _rpc_creq = req->next;
    } else {
        for (preq = _rpc_creq; preq; preq = preq->next) {
            if (preq->next == req) {
                preq->next = req->next;
                break;
            }
        }
    }
    RPC_CUNLOCK;
}

/*
 * Send a request, wait the a reply
 */
int
bcm_rpc_request(int unit, uint8 *buf, int len, uint8 **rbuf,
                void **cookie)
{
    int     rv;
    uint32  seq;
    uint8   *bp;
    cpudb_key_t cpu;
    _rpc_creq_t req;

    if (_rpc_client_lock == NULL) {
        atp_tx_data_free(buf);
        return BCM_E_UNAVAIL;
    }

    RPC_COUNT(_rpc_count_c_request);

    req.sem = sal_sem_create("bcm_rpc_send", sal_sem_BINARY, 0);
    if (req.sem == NULL) {
        RPC_COUNT(_rpc_count_c_fail);
        atp_tx_data_free(buf);
        return BCM_E_MEMORY;
    }
    req.unit = unit;
    req.rbuf = NULL;
#ifdef  BROADCOM_DEBUG
    req.time = sal_time_usecs();
#endif  /* BROADCOM_DEBUG */

    /* link req on stack into _rpc_creq list */
    RPC_CLOCK;
    seq = ++_rpc_seq;
    req.seq = seq;
    req.next = _rpc_creq;
    _rpc_creq = &req;
    RPC_CUNLOCK;

    bp = buf;
    BCM_PACK_U32(bp, seq);

    cpu = *(cpudb_key_t *)BCM_CONTROL(unit)->drv_control;
    rv = atp_tx(cpu, RPC_CLIENT_ID, buf, len, 0, NULL, NULL);
    atp_tx_data_free(buf);
    if (rv < 0) {
        /* take req out of _rpc_creq on error */
        _bcm_rpc_unlink_request(&req);
        sal_sem_destroy(req.sem);
        RPC_COUNT(_rpc_count_c_fail);
        return rv;
    }

    /* wait for reply */
    sal_sem_take(req.sem, RPC_REPLY_TIMEOUT);
    sal_sem_destroy(req.sem);

    if (req.rbuf == NULL) {  /* no response buffer */
        /* This request normally would have been unlinked when req.sem
           was given, but if sal_sem_take returns in error then the
           request record needs to be be unlinked here.
        */
        _bcm_rpc_unlink_request(&req);
        RPC_COUNT(_rpc_count_c_timeout);
        return BCM_E_TIMEOUT;
    }
    *rbuf = req.rbuf;
    *cookie = req.cookie;
    return BCM_E_NONE;
}

#ifndef BCM_RPC_REPLY_RETRY_COUNT
#define BCM_RPC_REPLY_RETRY_COUNT 3
#endif

#ifndef BCM_RPC_REPLY_RETRY_DELAY
#define BCM_RPC_REPLY_RETRY_DELAY 10000
#endif




/*
 * Send a reply
 */
int
bcm_rpc_reply(cpudb_key_t cpu, uint8 *buf, int len)
{
    int  rv = BCM_E_INTERNAL;
    int  retry = BCM_RPC_REPLY_RETRY_COUNT;

    RPC_COUNT(_rpc_count_s_reply);
    while (retry) {
        rv = atp_tx(cpu, RPC_CLIENT_ID, buf, len, 0, NULL, NULL);
        if (rv != BCM_E_RESOURCE) {
            break;
        } else {
            RPC_COUNT(_rpc_count_s_rretry);
            /* Retry atp_tx() on resource errors before giving up */
            retry--;
            sal_usleep(BCM_RPC_REPLY_RETRY_DELAY);
        }
    }
    atp_tx_data_free(buf);
#ifdef  BROADCOM_DEBUG
    if (BCM_FAILURE(rv)) {
        RPC_COUNT(_rpc_count_s_rerr);
    }
#endif  /* BROADCOM_DEBUG */
    
    return rv;
}

/*
 * Fail any old version 1 requests with a generic BCM_E_UNAVAIL error
 */
#define BCM_RPC_V1_HLEN  14
STATIC void
_bcm_rpc_unavail_v1(cpudb_key_t r_cpu, uint8 *r_pkt, void *r_cookie)
{
    uint32  seq;
    uint8   dir, ver;
    uint32  sig;
    uint16  entry;
    uint8   *pp;

    /* decode the v1 RPC header */
    pp = r_pkt;
    BCM_UNPACK_U32(pp, seq);
    BCM_UNPACK_U8(pp, dir);
    BCM_UNPACK_U8(pp, ver);
    BCM_UNPACK_U32(pp, sig);
    BCM_UNPACK_U16(pp, entry);

    if (dir != 'C') {
        RPC_ERR(("RPC: Old version %d entry %d non-request discarded\n",
                 ver, entry));
        return;
    }

    RPC_ERR
        (("RPC: Old version %d entry %d request failed with BCM_E_UNAVAIL\n",
          ver, entry));

    /* construct a v1 RPC reply */
    r_pkt = atp_tx_data_alloc(BCM_RPC_V1_HLEN+4);
    pp = r_pkt;
    BCM_PACK_U32(pp, seq);
    BCM_PACK_U8(pp, 'S');
    BCM_PACK_U8(pp, ver);
    BCM_PACK_U32(pp, sig);
    BCM_PACK_U16(pp, BCM_RPC_V1_HLEN+4);
    BCM_PACK_U32(pp, BCM_E_UNAVAIL);
    bcm_rpc_reply(r_cpu, r_pkt, pp-r_pkt);
}

#undef BCM_RX_DFLT_UNIT

/*
 * Packet handler.
 * ATP callback function that handles receipt of both request and
 * reply RPC packets.
 */
bcm_rx_t
bcm_rpc_pkt_handler(cpudb_key_t cpu,
                    int client_id,
                    bcm_pkt_t *pkt,
                    uint8 *buf,
                    int buf_len,
                    void *cookie)
{
    uint32  seq;
    uint8   dir, ver;
    uint8   *bp;
    int     i;
    _rpc_creq_t  *req, *preq;
    bcm_rpc_sreq_t  *sreq;

    COMPILER_REFERENCE(client_id);
    COMPILER_REFERENCE(buf_len);
    COMPILER_REFERENCE(cookie);

    /* decode header */
    bp = buf;
    BCM_UNPACK_U32(bp, seq);
    BCM_UNPACK_U8(bp, dir);
    BCM_UNPACK_U8(bp, ver);

    if (ver != BCM_RPC_VERSION) {
        RPC_COUNT(_rpc_count_s_wrongver);
        if (ver == 1) {
            _bcm_rpc_unavail_v1(cpu, buf, cookie);
            return BCM_RX_HANDLED;
        }
        RPC_ERR(("RPC: Version %d packet received\n", ver));
        return BCM_RX_NOT_HANDLED;  /* wrong version */
    }

    switch (dir) {
    case 'S':  /* reply from server */
        /* find request in the list */
        RPC_CLOCK;
        preq = NULL;
        req = _rpc_creq;
        while (req != NULL) {
            if (req->seq == seq) {  /* match: unlink request */
                if (preq == NULL) {
                    _rpc_creq = req->next;
                } else {
                    preq->next = req->next;
                }
                break;
            }
            preq = req;
            req = req->next;
        }
        RPC_CUNLOCK;
        if (req != NULL) {
            req->next = NULL;
            req->rbuf = buf;
            req->cookie = pkt;
            sal_sem_give(req->sem);  /* wake waiting thread */
            RPC_COUNT(_rpc_count_c_reply);
            return BCM_RX_HANDLED_OWNED;
        }
        RPC_COUNT(_rpc_count_c_noreq);
        return BCM_RX_HANDLED;  /* no matching request */

    case 'C':  /* request from client */
        /* add request to server thread's queue */
        RPC_COUNT(_rpc_count_s_request);
        sreq = sal_alloc(sizeof(*sreq), "bcm_rpc_server_req");
        if (sreq == NULL) {
            return BCM_RX_HANDLED;  /* failure */
        }
        sreq->next = NULL;
        sreq->cpu = cpu;
        sreq->buf = buf;
        sreq->cookie = pkt;
        bp += BCM_PACKLEN_U16;  /* skip len */
        for (i = 0; i < BCM_RPC_LOOKUP_KEYLEN; i++) {
            BCM_UNPACK_U32(bp, sreq->rpckey[i]);
        }
        RPC_SLOCK;
        if (_rpc_sreq_tail == NULL) {
            _rpc_sreq = _rpc_sreq_tail = sreq;
        } else {
            _rpc_sreq_tail->next = sreq;
            _rpc_sreq_tail = sreq;
        }
        RPC_SUNLOCK;
        RPC_WAKE;  /* wake up server */

        return BCM_RX_HANDLED_OWNED;

    default:
        RPC_COUNT(_rpc_count_s_wrongver);
        RPC_ERR(("RPC: Version %d packet has unexpected direction (%d)\n",
                 ver, dir));
        return BCM_RX_NOT_HANDLED;  /* unknown direction */
    }
}

/*
 * Binary search the server lookup table for a matching request key.
 * Run the server rpc routine that is matched.
 */
void
bcm_rpc_run(bcm_rpc_sreq_t *sreq)
{
    uint32                 key0;
    _bcm_server_routine_t  rtn;
    int                    lo, hi, new, i, match;
    _bcm_server_lookup_t   *sarr;

    key0 = sreq->rpckey[0];
    sarr = _bcm_server_lookup;
    lo = -1;
    hi = BCM_RPC_LOOKUP_COUNT;
    match = 0;
    while (hi-lo > 1) {
        new = (hi + lo) / 2;
        if (sarr[new].key[0] > key0) {
            hi = new;
        } else if (sarr[new].key[0] < key0) {
            lo = new;
        } else {
            /* key0 is equal, check the rest */
            match = 1;
            for (i = 1; i < BCM_RPC_LOOKUP_KEYLEN; i++) {
                if (sarr[new].key[i] > sreq->rpckey[i]) {
                    hi = new;
                    match = 0;
                    break;
                } else if (sarr[new].key[i] < sreq->rpckey[i]) {
                    lo = new;
                    match = 0;
                    break;
                }
            }
            if (match) {
                lo = new;
                break;
            }
        }
    }
    if (match) {
        rtn = sarr[lo].routine;
    } else {
        RPC_COUNT(_rpc_count_s_nokey);
        rtn = _bcm_server_unavail;
    }
    rtn(sreq->cpu, sreq->buf, sreq->cookie);
}

/*
 * Server thread.
 * Receives request packets, processes the each request.
 * The process routines will send replies.
 */
void
bcm_rpc_thread(void *cookie)
{
    bcm_rpc_sreq_t  *sreq, *freq;

    COMPILER_REFERENCE(cookie);

    _rpc_thread_exit = 0;
    for (;;) {
        RPC_SLEEP;

        /* grab a set of requests off the server queue */
        RPC_SLOCK;
        sreq = _rpc_sreq;
        _rpc_sreq = _rpc_sreq_tail = NULL;
        RPC_SUNLOCK;
        if (sreq == NULL) {  /* nothing to do */
            if (_rpc_thread_exit) {
                _rpc_server_thread = SAL_THREAD_ERROR;
                sal_thread_exit(0);
                return;
            }
            continue;
        }

        /* run the requests */
        while (sreq != NULL) {
            RPC_COUNT(_rpc_count_s_run);
            bcm_rpc_run(sreq);
            freq = sreq;
            sreq = sreq->next;
            sal_free(freq);
        }
    }
}

int
bcm_rpc_start(void)
{
    int  rv, flags;

    if (_rpc_server_thread != SAL_THREAD_ERROR) {
        return BCM_E_BUSY;
    }
    _rpc_client_lock = sal_mutex_create("bcm_rpc_client");
    _rpc_server_lock = sal_mutex_create("bcm_rpc_server");
    _rpc_server_sem = sal_sem_create("bcm_rpc_server", sal_sem_BINARY, 0);

    _rpc_server_thread = sal_thread_create("bcmRPC",
                                           RPC_THREAD_STACK,
                                           RPC_THREAD_PRIO,
                                           bcm_rpc_thread,
                                           NULL);
    if (_rpc_server_thread == SAL_THREAD_ERROR) {
        sal_sem_destroy(_rpc_server_sem);
        sal_mutex_destroy(_rpc_server_lock);
        sal_mutex_destroy(_rpc_client_lock);
        _rpc_server_lock = NULL;
        _rpc_client_lock = NULL;
        return BCM_E_RESOURCE;
    }

    /* can be configured to use nexthop or cpu2cpu */
    if (_rpc_nexthop) {
        flags = ATP_F_REASSEM_BUF | ATP_F_NEXT_HOP;
    } else {
        flags = ATP_F_REASSEM_BUF;
    }
    rv = atp_register(RPC_CLIENT_ID, flags,
                      bcm_rpc_pkt_handler, NULL, -1, -1);
    return rv;
}

int
bcm_rpc_stop(void)
{
    _rpc_creq_t  *req, *nreq;

    if (_rpc_server_thread == SAL_THREAD_ERROR) {
        return BCM_E_NONE;
    }
    atp_unregister(RPC_CLIENT_ID);

    /* destroy thread */
    _rpc_thread_exit = 1;
    RPC_WAKE;
    sal_thread_yield();
    while (_rpc_server_thread != SAL_THREAD_ERROR) {
        RPC_WAKE;
        sal_usleep(10000);
    }

    /* fail any queued client requests */
    RPC_CLOCK;
    req = _rpc_creq;
    _rpc_creq = NULL;
    RPC_CUNLOCK;
    while (req != NULL) {
        nreq = req->next;
        sal_sem_give(req->sem);  /* wake waiting thread */
        req = nreq;
    }

    sal_sem_destroy(_rpc_server_sem);
    sal_mutex_destroy(_rpc_server_lock);
    sal_mutex_destroy(_rpc_client_lock);
    _rpc_server_lock = NULL;
    _rpc_client_lock = NULL;
    return BCM_E_NONE;
}

/*
 * A bcm unit has been detached.  Find any queued
 * rpc requests for that unit and fail them.
 *
 * Note:  A unit value of (-1) indicates to remove queued
 * rpc request in all units.
 */
int
bcm_rpc_detach(int unit)
{
    _rpc_creq_t  *req, *nreq, *preq;

    if (_rpc_client_lock == NULL) {
        return BCM_E_UNAVAIL;
    }

    /* find request in the list */
    RPC_CLOCK;
    preq = NULL;
    req = _rpc_creq;
    while (req != NULL) {
        nreq = req->next;
        if ((req->unit == unit) || (unit == -1)) {
            if (preq == NULL) {
                _rpc_creq = req->next;
            } else {
                preq->next = req->next;
            }
            req->next = NULL;
            sal_sem_give(req->sem);  /* wake waiting thread */
            RPC_COUNT(_rpc_count_c_detach);
        } else {
            preq = req;
        }
        req = nreq;
    }
    RPC_CUNLOCK;
    return BCM_E_NONE;
}

/*
 * Dispatchable attach/detach routines
 */
int
_bcm_client_attach(int unit, char *subtype)
{
    cpudb_key_t  *cpu;
    int          rv;
    bcm_info_t   info;

    if (subtype == NULL) {
        return BCM_E_CONFIG;
    }
    cpu = sal_alloc(sizeof(*cpu), "bcm_client_attach");
    if (cpu == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(cpu, 0, sizeof(*cpu));

    rv = cpudb_key_parse(subtype, cpu);
    if (rv < 0) {
        sal_free((void *)cpu);
        return rv;
    }

    BCM_CONTROL(unit)->drv_control = (void *)cpu;
    BCM_CONTROL(unit)->capability |= BCM_CAPA_REMOTE;

    /* call bcm_client_info_get directly - do not dispatch */
    rv = bcm_client_info_get(unit, &info);
    if (rv < 0) {
        sal_free((void *)cpu);
        return rv;
    }
    BCM_CONTROL(unit)->chip_vendor = info.vendor;
    BCM_CONTROL(unit)->chip_device = info.device;
    BCM_CONTROL(unit)->chip_revision = info.revision;
    BCM_CONTROL(unit)->capability |= info.capability;

    return BCM_E_NONE;
}

int
_bcm_client_detach(int unit)
{
    /* free cpudb key */
    if (BCM_CONTROL(unit)->drv_control != NULL) {
        sal_free(BCM_CONTROL(unit)->drv_control);
    }
    return BCM_E_NONE;
}

/* match BCM control subtype strings for client types */
int
_bcm_client_match(int unit, char *keystr_a, char *keystr_b)
{
    cpudb_key_t a,b;

    COMPILER_REFERENCE(unit);
    cpudb_key_parse(keystr_a, &a);
    cpudb_key_parse(keystr_b, &b);

    return CPUDB_KEY_COMPARE(a, b);
}

/*
 * Function:
 *     bcm_rpc_cleanup
 * Purpose:
 *     Clear pending RPC requests for specified destination CPU.
 * Parameters:
 *     cpu    - CPU key to clear pending requests for.
 * Returns:
 *     BCM_E_NONE  Success
 */
int
bcm_rpc_cleanup(cpudb_key_t cpu)
{
    int          unit;
    cpudb_key_t  *unit_cpu;

    /* Clear pending RPC requests for all units residing in given CPU */
    for (unit = 0; unit < BCM_CONTROL_MAX; unit++) {
        if (!BCM_UNIT_VALID(unit)) {
            continue;
        }
        /* Check for units matching cpu key */
        if ((unit_cpu = (cpudb_key_t *)BCM_CONTROL(unit)->drv_control)
            == NULL) {
            continue;
        }
        if (CPUDB_KEY_COMPARE(*unit_cpu, cpu) == 0) {
            bcm_rpc_detach(unit);
        }
    }
        
    return BCM_E_NONE;
}


#ifdef  BROADCOM_DEBUG
void
bcm_rpc_dump(void)
{
    _rpc_creq_t  *creq;
    bcm_rpc_sreq_t  *sreq;
    int          i;
    char         keybuf[CPUDB_KEY_STRING_LEN];
    sal_usecs_t  now;

    RPC_OUT(("RPC Client request %u reply %u fail %u timeout %u\n",
             _rpc_count_c_request,
             _rpc_count_c_reply,
             _rpc_count_c_fail,
             _rpc_count_c_timeout));
    RPC_OUT(("RPC Client missing request %u detach remove %u seq %u\n",
             _rpc_count_c_noreq,
             _rpc_count_c_detach,
             _rpc_seq));
    RPC_OUT(("RPC Server request %u reply %u run %u wrongver %u nokey %u "
             "repretry %u reperr %u \n",
             _rpc_count_s_request,
             _rpc_count_s_reply,
             _rpc_count_s_run,
             _rpc_count_s_wrongver,
             _rpc_count_s_nokey,
             _rpc_count_s_rretry,
             _rpc_count_s_rerr));

    now = sal_time_usecs();
    RPC_OUT(("RPC Client Requests: time now %u\n", now));
    i = 0;
    for (creq = _rpc_creq; creq != NULL; creq = creq->next) {
        i += 1;
        RPC_OUT(("%d:\tunit %d seq %u cookie %p time %u (%u ago)\n",
                 i, creq->unit, creq->seq, creq->cookie,
                 creq->time, now - creq->time));
    }
    
    RPC_OUT(("RPC Server Requests:\n"));
    i = 0;
    for (sreq = _rpc_sreq; sreq != NULL; sreq = sreq->next) {
        i += 1;
        cpudb_key_format(sreq->cpu, keybuf, sizeof(keybuf));
        RPC_OUT(("%d:\tcpu %s buf %p cookie %p rpckey0 %x\n",
                 i, keybuf, sreq->buf, sreq->cookie, sreq->rpckey[0]));
    }
}
#endif  /* BROADCOM_DEBUG */

#endif  /* BCM_RPC_SUPPORT */
