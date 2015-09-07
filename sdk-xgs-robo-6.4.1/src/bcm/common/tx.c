/*
 * $Id: tx.c,v 1.106 Broadcom SDK $
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

#ifdef BCM_HIDE_DISPATCHABLE
#undef BCM_HIDE_DISPATCHABLE
#endif

#include <shared/bsl.h>

#include <assert.h>

#include <sal/types.h>
#include <sal/core/time.h>
#include <sal/core/boot.h>
#include <shared/bsl.h>
#include <soc/dma.h>
#include <soc/cm.h>
#include <soc/dcbformats.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/util.h>
#include <soc/error.h>
#include <soc/higig.h>
#include <soc/pbsmh.h>
#include <soc/enet.h>
#if defined(BCM_BRADLEY_SUPPORT)
#include <soc/bradley.h>
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/trident2.h>
#endif

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT)
#include <bcm_int/esw/tx.h>
#endif
#include <bcm_int/control.h>

#include <bcm/pkt.h>
#include <bcm/tx.h>
#include <bcm/stack.h>
#include <bcm/error.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT)
#include <bcm_int/common/multicast.h>
#include <bcm_int/esw/rcpu.h>
#include <bcm_int/esw/stack.h>
#endif
#include <bcm_int/api_xlate_port.h>
#include <bcm_int/common/tx.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT)
#include <bcm_int/esw/tx.h>
#endif
#if defined(INCLUDE_L3) && defined(BCM_XGS_SWITCH_SUPPORT)
#include <bcm_int/esw/firebolt.h>
#endif /* INCLUDE_L3 && BCM_XGS_SWITCH_SUPPORT */
#if defined(INCLUDE_L3) && defined(BCM_TRX_SUPPORT)
#include <bcm_int/esw/vpn.h>
#endif /* INCLUDE_L3 && BCM_TRX_SUPPORT */

#if defined(BCM_SBX_CMIC_SUPPORT)
#include <soc/sbx/sbx_drv.h>
#endif

#if defined (BCM_ARAD_SUPPORT)
#include <soc/dpp/ARAD/arad_action_cmd.h>
#endif


#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT) || defined (BCM_ARAD_SUPPORT)

#if defined(BROADCOM_DEBUG)
int bcm_tx_pkt_count[BCM_COS_COUNT];
int bcm_tx_pkt_count_bad_cos;
#endif /* BROADCOM_DEBUG */

/* Forward declarations of static functions */
STATIC dv_t *_tx_dv_alloc(int unit, int pkt_count, int dcb_count,
                          bcm_pkt_cb_f call_back, void *cookie, int pkt_cb);
STATIC void _tx_dv_free(int unit, dv_t *dv);

STATIC int  _tx_pkt_desc_add(int unit, bcm_pkt_t *pkt, dv_t *dv, int pkt_idx);

STATIC int  _bcm_tx_chain_send(int unit, dv_t *dv, int async);

STATIC void _bcm_tx_chain_done_cb(int unit, dv_t *dv);
STATIC void _bcm_tx_chain_done(int unit, dv_t *dv);
STATIC void _bcm_tx_packet_done_cb(int unit, dv_t *dv, dcb_t *dcb);
STATIC void _bcm_tx_packet_done(bcm_pkt_t *pkt);

STATIC void _bcm_tx_callback_thread(void *param);
#if defined (BCM_ARAD_SUPPORT)
STATIC INLINE int _bcm_tx_pkt_tag_setup(int unit, bcm_pkt_t *pkt);


#if defined (BCM_ARAD_SUPPORT) && !defined (BCM_ESW_SUPPORT) & !defined(BCM_SBX_CMIC_SUPPORT)
typedef struct stk_tag_s {
#if defined(LE_HOST)
    uint32 rsvp:2;
    uint32 stk_modid:5;
    uint32 ed:1;
    uint32 em:1;
    uint32 md:1;
    uint32 mirr:1;
    uint32 pfm:2;
    uint32 dst_rtag:3;
    uint32 dst_tgid:3;
    uint32 dst_t:1;
    uint32 src_rtag:3;
    uint32 src_tgid:3;
    uint32 src_t:1;
    uint32 stk_cnt:5;
#else
    uint32 stk_cnt:5;
    uint32 src_t:1;
    uint32 src_tgid:3;
    uint32 src_rtag:3;
    uint32 dst_t:1;
    uint32 dst_tgid:3;
    uint32 dst_rtag:3;
    uint32 pfm:2;
    uint32 mirr:1;
    uint32 md:1;
    uint32 em:1;
    uint32 ed:1;
    uint32 stk_modid:5;
    uint32 rsvp:2;
#endif
} stk_tag_t;

typedef struct tx_dv_info_s {
    volatile bcm_pkt_t    **pkt;
    int                     pkt_count;
    volatile uint8          pkt_done_cnt;
    bcm_pkt_cb_f            chain_done_cb;
    void                   *cookie;
} tx_dv_info_t;

#define TX_INFO_SET(dv, val)      ((dv)->dv_public1.ptr = val)
#define TX_INFO(dv)               ((tx_dv_info_t *)((dv)->dv_public1.ptr))
#define TX_INFO_PKT_ADD(dv, pkt) \
    TX_INFO(dv)->pkt[TX_INFO(dv)->pkt_count++] = (pkt)
#define TX_INFO_CUR_PKT(dv)       (TX_INFO(dv)->pkt[TX_INFO(dv)->pkt_done_cnt])
#define TX_INFO_PKT_MARK_DONE(dv) ((TX_INFO(dv)->pkt_done_cnt)++)

#define TX_DV_NEXT(dv)            ((dv_t *)((dv)->dv_public2.ptr))
#define TX_DV_NEXT_SET(dv, dv_next) \
    ((dv)->dv_public2.ptr = (void *)(dv_next))

#define TX_EXTRA_DCB_COUNT 4
#endif

#endif

#ifdef BCM_XGS3_SWITCH_SUPPORT

typedef struct _xgs3_async_queue_s {
    struct _xgs3_async_queue_s * next;
    int unit;
    bcm_pkt_t * pkt;
    void * cookie;
} _xgs3_async_queue_t;

static _xgs3_async_queue_t * _xgs3_async_head;
static _xgs3_async_queue_t * _xgs3_async_tail;

static sal_sem_t _xgs3_async_tx_sem;

STATIC void _xgs3_async_thread(void * param);
#endif

/* Some macros:
 *
 *     _ON_ERROR_GOTO:  Check the return code of an action;
 *                      if error, goto the label provided.
 *     _PROCESS_ERROR:  If rv indicates an error, free the DV (if non-NULL)
 *                      and display the "err_msg" if provided.
 */

#define _ON_ERROR_GOTO(act, rv, label) if (((rv) = (act)) < 0) goto label

#define _PROCESS_ERROR(unit, rv, dv, err_msg) \
    do { \
        if ((rv) < 0) { /* Error detected */ \
            if (dv != NULL) { \
                _tx_dv_free(unit, dv); \
            } \
            if (err_msg) { \
                LOG_ERROR(BSL_LS_BCM_TX, \
                          (BSL_META_U(unit, \
                                      "bcm_tx: %s\n"), err_msg)); \
            } \
        } \
    } while (0)

/* Call back synchronization and lists */

static sal_sem_t         tx_cb_sem;
volatile static dv_t    *dv_pend_first;
volatile static dv_t    *dv_pend_last;

volatile static bcm_pkt_t *pkt_pend_first;
volatile static bcm_pkt_t *pkt_pend_last;

#define TX_LOCK(_s)      _s = sal_splhi()
#define TX_UNLOCK(_s)    sal_spl(_s)

#define DEFAULT_TX_PRI 50               /* Default Thread Priority */

static uint8*            _null_crc_ptr;
static uint8*            _pkt_pad_ptr;
static int               _tx_init;
volatile static int      _tx_chain_send;
volatile static int      _tx_chain_done;
volatile static int      _tx_chain_done_intr;

#ifdef BCM_XGS3_SWITCH_SUPPORT
/* Lookup table: Return the first bit set. -1 if no bit set. */
static int8 bpos[256] = {
 /* 0x00 */ -1, /* 0x01 */  0, /* 0x02 */  1, /* 0x03 */  0,
 /* 0x04 */  2, /* 0x05 */  0, /* 0x06 */  1, /* 0x07 */  0,
 /* 0x08 */  3, /* 0x09 */  0, /* 0x0a */  1, /* 0x0b */  0,
 /* 0x0c */  2, /* 0x0d */  0, /* 0x0e */  1, /* 0x0f */  0,
 /* 0x10 */  4, /* 0x11 */  0, /* 0x12 */  1, /* 0x13 */  0,
 /* 0x14 */  2, /* 0x15 */  0, /* 0x16 */  1, /* 0x17 */  0,
 /* 0x18 */  3, /* 0x19 */  0, /* 0x1a */  1, /* 0x1b */  0,
 /* 0x1c */  2, /* 0x1d */  0, /* 0x1e */  1, /* 0x1f */  0,
 /* 0x20 */  5, /* 0x21 */  0, /* 0x22 */  1, /* 0x23 */  0,
 /* 0x24 */  2, /* 0x25 */  0, /* 0x26 */  1, /* 0x27 */  0,
 /* 0x28 */  3, /* 0x29 */  0, /* 0x2a */  1, /* 0x2b */  0,
 /* 0x2c */  2, /* 0x2d */  0, /* 0x2e */  1, /* 0x2f */  0,
 /* 0x30 */  4, /* 0x31 */  0, /* 0x32 */  1, /* 0x33 */  0,
 /* 0x34 */  2, /* 0x35 */  0, /* 0x36 */  1, /* 0x37 */  0,
 /* 0x38 */  3, /* 0x39 */  0, /* 0x3a */  1, /* 0x3b */  0,
 /* 0x3c */  2, /* 0x3d */  0, /* 0x3e */  1, /* 0x3f */  0,
 /* 0x40 */  6, /* 0x41 */  0, /* 0x42 */  1, /* 0x43 */  0,
 /* 0x44 */  2, /* 0x45 */  0, /* 0x46 */  1, /* 0x47 */  0,
 /* 0x48 */  3, /* 0x49 */  0, /* 0x4a */  1, /* 0x4b */  0,
 /* 0x4c */  2, /* 0x4d */  0, /* 0x4e */  1, /* 0x4f */  0,
 /* 0x50 */  4, /* 0x51 */  0, /* 0x52 */  1, /* 0x53 */  0,
 /* 0x54 */  2, /* 0x55 */  0, /* 0x56 */  1, /* 0x57 */  0,
 /* 0x58 */  3, /* 0x59 */  0, /* 0x5a */  1, /* 0x5b */  0,
 /* 0x5c */  2, /* 0x5d */  0, /* 0x5e */  1, /* 0x5f */  0,
 /* 0x60 */  5, /* 0x61 */  0, /* 0x62 */  1, /* 0x63 */  0,
 /* 0x64 */  2, /* 0x65 */  0, /* 0x66 */  1, /* 0x67 */  0,
 /* 0x68 */  3, /* 0x69 */  0, /* 0x6a */  1, /* 0x6b */  0,
 /* 0x6c */  2, /* 0x6d */  0, /* 0x6e */  1, /* 0x6f */  0,
 /* 0x70 */  4, /* 0x71 */  0, /* 0x72 */  1, /* 0x73 */  0,
 /* 0x74 */  2, /* 0x75 */  0, /* 0x76 */  1, /* 0x77 */  0,
 /* 0x78 */  3, /* 0x79 */  0, /* 0x7a */  1, /* 0x7b */  0,
 /* 0x7c */  2, /* 0x7d */  0, /* 0x7e */  1, /* 0x7f */  0,
 /* 0x80 */  7, /* 0x81 */  0, /* 0x82 */  1, /* 0x83 */  0,
 /* 0x84 */  2, /* 0x85 */  0, /* 0x86 */  1, /* 0x87 */  0,
 /* 0x88 */  3, /* 0x89 */  0, /* 0x8a */  1, /* 0x8b */  0,
 /* 0x8c */  2, /* 0x8d */  0, /* 0x8e */  1, /* 0x8f */  0,
 /* 0x90 */  4, /* 0x91 */  0, /* 0x92 */  1, /* 0x93 */  0,
 /* 0x94 */  2, /* 0x95 */  0, /* 0x96 */  1, /* 0x97 */  0,
 /* 0x98 */  3, /* 0x99 */  0, /* 0x9a */  1, /* 0x9b */  0,
 /* 0x9c */  2, /* 0x9d */  0, /* 0x9e */  1, /* 0x9f */  0,
 /* 0xa0 */  5, /* 0xa1 */  0, /* 0xa2 */  1, /* 0xa3 */  0,
 /* 0xa4 */  2, /* 0xa5 */  0, /* 0xa6 */  1, /* 0xa7 */  0,
 /* 0xa8 */  3, /* 0xa9 */  0, /* 0xaa */  1, /* 0xab */  0,
 /* 0xac */  2, /* 0xad */  0, /* 0xae */  1, /* 0xaf */  0,
 /* 0xb0 */  4, /* 0xb1 */  0, /* 0xb2 */  1, /* 0xb3 */  0,
 /* 0xb4 */  2, /* 0xb5 */  0, /* 0xb6 */  1, /* 0xb7 */  0,
 /* 0xb8 */  3, /* 0xb9 */  0, /* 0xba */  1, /* 0xbb */  0,
 /* 0xbc */  2, /* 0xbd */  0, /* 0xbe */  1, /* 0xbf */  0,
 /* 0xc0 */  6, /* 0xc1 */  0, /* 0xc2 */  1, /* 0xc3 */  0,
 /* 0xc4 */  2, /* 0xc5 */  0, /* 0xc6 */  1, /* 0xc7 */  0,
 /* 0xc8 */  3, /* 0xc9 */  0, /* 0xca */  1, /* 0xcb */  0,
 /* 0xcc */  2, /* 0xcd */  0, /* 0xce */  1, /* 0xcf */  0,
 /* 0xd0 */  4, /* 0xd1 */  0, /* 0xd2 */  1, /* 0xd3 */  0,
 /* 0xd4 */  2, /* 0xd5 */  0, /* 0xd6 */  1, /* 0xd7 */  0,
 /* 0xd8 */  3, /* 0xd9 */  0, /* 0xda */  1, /* 0xdb */  0,
 /* 0xdc */  2, /* 0xdd */  0, /* 0xde */  1, /* 0xdf */  0,
 /* 0xe0 */  5, /* 0xe1 */  0, /* 0xe2 */  1, /* 0xe3 */  0,
 /* 0xe4 */  2, /* 0xe5 */  0, /* 0xe6 */  1, /* 0xe7 */  0,
 /* 0xe8 */  3, /* 0xe9 */  0, /* 0xea */  1, /* 0xeb */  0,
 /* 0xec */  2, /* 0xed */  0, /* 0xee */  1, /* 0xef */  0,
 /* 0xf0 */  4, /* 0xf1 */  0, /* 0xf2 */  1, /* 0xf3 */  0,
 /* 0xf4 */  2, /* 0xf5 */  0, /* 0xf6 */  1, /* 0xf7 */  0,
 /* 0xf8 */  3, /* 0xf9 */  0, /* 0xfa */  1, /* 0xfb */  0,
 /* 0xfc */  2, /* 0xfd */  0, /* 0xfe */  1, /* 0xff */  0,
};

/*
 * Extract port from PBM
 */
#define _pbm2port(bmp) \
    ((bpos[(bmp >>  0) & 0xFF] != -1) ? (0  + bpos[(bmp >>  0) & 0xFF]) : \
     (bpos[(bmp >>  8) & 0xFF] != -1) ? (8  + bpos[(bmp >>  8) & 0xFF]) : \
     (bpos[(bmp >> 16) & 0xFF] != -1) ? (16 + bpos[(bmp >> 16) & 0xFF]) : \
     (bpos[(bmp >> 24) & 0xFF] != -1) ? (24 + bpos[(bmp >> 24) & 0xFF]) : \
     (-1) \
    )

/*
 * Function:
 *      _bcm_xgs3_tx_pipe_bypass_header_setup
 * Purpose:
 *      Setup up header required to steer raw packet data stream
 *      out of a port.
 * Parameters:
 *      unit - transmission unit
 *      pkt - To be updated
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The TX port is extracted from tx_pbmp. If port is a member
 *      of tx_upbmp then the VLAN TAG is removed.
 */
STATIC INLINE int
_bcm_xgs3_tx_pipe_bypass_header_setup(int unit, bcm_pkt_t *pkt)
{
    soc_pbsmh_hdr_t *pbh = (soc_pbsmh_hdr_t *)pkt->_pb_hdr;
    uint32 bmap = SOC_PBMP_WORD_GET(pkt->tx_pbmp, 0);
    int port = _pbm2port(bmap);
    int port_adj = 0;
    uint32 prio_val;
    uint32 src_mod;
    int qnum;
    int max_num_port = 0;

    if (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS) {
        max_num_port = SOC_MAX_NUM_PP_PORTS;
    } else {
        max_num_port = SOC_INFO(unit).port_num;
    }

    src_mod = (pkt->flags & BCM_TX_SRC_MOD) ?
        pkt->src_mod : SOC_DEFAULT_DMA_SRCMOD_GET(unit);
    prio_val = (pkt->flags & BCM_TX_PRIO_INT) ? pkt->prio_int : pkt->cos;

    if (port == -1) {
        if (soc_feature(unit, soc_feature_register_hi) || SOC_IS_TR_VL(unit)) {
            bmap = SOC_PBMP_WORD_GET(pkt->tx_pbmp, 1);
            port = _pbm2port(bmap);
            port_adj = 32;
            if (port == -1) {
                if (max_num_port > 64) {
                    bmap = SOC_PBMP_WORD_GET(pkt->tx_pbmp, 2);
                    port = _pbm2port(bmap);
                    port_adj = 64;
                    if (port == -1) {
                        if (max_num_port > 96) {
                            bmap = SOC_PBMP_WORD_GET(pkt->tx_pbmp, 3);
                            port = _pbm2port(bmap);
                            port_adj = 96;
                        }
                        if (port == -1) {
                            if (max_num_port > 128) {
                                bmap = SOC_PBMP_WORD_GET(pkt->tx_pbmp, 4);
                                port = _pbm2port(bmap);
                                port_adj = 128;
                            }
                            if (port == -1) {
                                if (max_num_port > 160) {
                                    bmap = SOC_PBMP_WORD_GET(pkt->tx_pbmp, 5);
                                    port = _pbm2port(bmap);
                                    port_adj = 160;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (port == -1) {
            return BCM_E_PARAM;
        }
    }
    if ((bmap & (~(1 << port))) != 0) {
        return BCM_E_PARAM;
    }
    port += port_adj;

    /* Check untag membership before translating */
    if (PBMP_MEMBER(pkt->tx_upbmp, port)) {
        pkt->flags |= BCM_PKT_F_TX_UNTAG;
    } else {
        pkt->flags &= (~BCM_PKT_F_TX_UNTAG);
    }

    BCM_API_XLATE_PORT_A2P(unit, &port);

    
    if ((SOC_DCB_TYPE(unit) == 23) || (SOC_DCB_TYPE(unit) == 26) || 
        (SOC_DCB_TYPE(unit) == 29) || (SOC_DCB_TYPE(unit) == 30) ||
        (SOC_DCB_TYPE(unit) == 31))  {
#ifdef BCM_KATANA2_SUPPORT
        if (SOC_DCB_TYPE(unit) == 29) {
            qnum = SOC_INFO(unit).port_cosq_base[port] + pkt->cos;
        } else 
#endif
        {
            qnum = SOC_INFO(unit).port_uc_cosq_base[port] + pkt->cos;
        }
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_DCB_TYPE(unit) == 26) {
            qnum = soc_td2_logical_qnum_hw_qnum(unit, port, qnum, 1);
        }
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
        if ((SOC_DCB_TYPE(unit) == 29) && (pkt->flags2 & BCM_PKT_F2_MA_PTR)) {
            /* OAM case */
            qnum = SOC_INFO(unit).port_cosq_base[port] + pkt->cos;
            PBS_MH_V8_W2_SMOD_COS_QNUM_SET(pbh, src_mod, 1, pkt->cos, qnum);
            soc_pbsmh_field_set(unit, pbh, PBSMH_pp_port, port);
            if(pkt->flags2 & BCM_PKT_F2_MEP_TYPE_UPMEP) {
                PBS_MH_V8_W0_OAM_UPMEP_START_SET(pbh);
            } else {
                PBS_MH_V8_W0_OAM_DOWNMEP_START_SET(pbh);
                PBS_MH_V8_W0_OAM_DOWNMEP_CTR_LOCATION_SET(pbh);
            }
            soc_pbsmh_field_set(unit, pbh, PBSMH_oam_ma_ptr, pkt->ma_ptr);
            if(pkt->flags2 & BCM_PKT_F2_TIMESTAMP_MODE) {
                soc_pbsmh_field_set(unit, pbh, PBSMH_ts_action, pkt->timestamp_mode);
            }
            if(pkt->flags2 & BCM_PKT_F2_SAMPLE_RDI) {
                soc_pbsmh_field_set(unit, pbh, PBSMH_sample_rdi, 1);
            }
            if((pkt->flags2 & BCM_PKT_F2_REPLACEMENT_OFFSET)) {
                soc_pbsmh_field_set(unit, pbh, PBSMH_oam_replacement_offset,
                                    pkt->oam_replacement_offset);
            }
            if(pkt->flags2 & BCM_PKT_F2_LM_COUNTER_INDEX) {
                soc_pbsmh_field_set(unit, pbh, PBSMH_lm_ctr1_index,
                                    pkt->oam_lm_counter_index);
                if(pkt->flags2 & BCM_PKT_F2_COUNTER_MODE_1) {
                    soc_pbsmh_field_set(unit, pbh, PBSMH_ctr1_action,
                                        pkt->counter_mode_1);
                }
            }
            if(pkt->flags2 & BCM_PKT_F2_LM_COUNTER_INDEX_2) {
                soc_pbsmh_field_set(unit, pbh, PBSMH_lm_ctr2_index,
                                    pkt->oam_lm_counter_index_2);
                if(pkt->flags2 & BCM_PKT_F2_COUNTER_MODE_2) {
                    soc_pbsmh_field_set(unit, pbh, PBSMH_ctr2_action,
                                        pkt->counter_mode_2);
                }
            }
        } else if (SOC_DCB_TYPE(unit) == 29) {
            PBS_MH_V7_W0_START_SET(pbh);
            PBS_MH_V8_W1_DPORT_IPRI_SET(pbh, port,prio_val);
            PBS_MH_V8_W2_SMOD_COS_QNUM_SET(pbh, src_mod, 1, pkt->cos, qnum);
            soc_pbsmh_field_set(unit, pbh, PBSMH_dst_port, port);
        } else
#endif
        {
            PBS_MH_V7_W0_START_SET(pbh);
            PBS_MH_V7_W1_DPORT_QNUM_SET(pbh, port, qnum);
            PBS_MH_V7_W2_SMOD_COS_QNUM_SET(pbh, src_mod, 1, pkt->cos, qnum,
                                       prio_val);
        }

        if (SOC_DCB_TYPE(unit) == 23) {
            if((pkt->flags2 & BCM_PKT_F2_REPLACEMENT_TYPE) &&
              (pkt->flags2 & BCM_PKT_F2_REPLACEMENT_OFFSET)) {
                soc_pbsmh_field_set(unit, pbh, PBSMH_oam_replacement_type, 
                                                     pkt->oam_replacement_type);
                soc_pbsmh_field_set(unit, pbh, PBSMH_oam_replacement_offset, 
                                                     pkt->oam_replacement_offset);
                if(pkt->flags2 & BCM_PKT_F2_LM_COUNTER_INDEX) {
                    soc_pbsmh_field_set(unit, pbh, PBSMH_lm_ctr_index, 
                                                     pkt->oam_lm_counter_index);
                }
            }
        }
    } else {
        PBS_MH_W0_START_SET(pbh);
        PBS_MH_W1_RSVD_SET(pbh);
        
        if ((SOC_DCB_TYPE(unit) == 11) || (SOC_DCB_TYPE(unit) == 12) ||
            (SOC_DCB_TYPE(unit) == 15) || (SOC_DCB_TYPE(unit) == 17) ||
            (SOC_DCB_TYPE(unit) == 18)) {
            PBS_MH_V2_W2_SMOD_DPORT_COS_SET(pbh, src_mod, port,
                                            pkt->cos, prio_val, 0);
            if (pkt->flags & BCM_PKT_F_TIMESYNC) {
                PBS_MH_V2_TS_PKT_SET(pbh);
            }
        } else if ((SOC_DCB_TYPE(unit) == 14) || (SOC_DCB_TYPE(unit) == 19) ||
                   (SOC_DCB_TYPE(unit) == 20)) {
            if (!(pkt->flags & BCM_TX_PRIO_INT)) {
                /* If BCM_TX_PRIO_INT is not specified, the int_pri
                 * is set to the cos value. However in version 3 of 
                 * the PBS_MH, cos is 6 bit, but int_prio is only 4.
                 * To be safe, set the int_prio to 0 if it wasn't 
                 * explicitly set as indicated by BCM_TX_PRIO_INT flag.
                 */
                prio_val = 0;
            }
            PBS_MH_V3_W2_SMOD_DPORT_COS_SET(pbh, src_mod, port,
                                            pkt->cos, prio_val, 0);
            if (pkt->flags & BCM_PKT_F_TIMESYNC) {
                PBS_MH_V3_TS_PKT_SET(pbh);
            }
        } else if (SOC_DCB_TYPE(unit) == 16) {
            PBS_MH_V4_W2_SMOD_DPORT_COS_SET(pbh, src_mod, port,
                                            pkt->cos, prio_val, 0);
        } else if (SOC_DCB_TYPE(unit) == 21) {
            PBS_MH_V5_W1_SMOD_SET(pbh, src_mod, 1, 0, 0);
            PBS_MH_V5_W2_DPORT_COS_SET(pbh, port, pkt->cos, prio_val);
            if (pkt->flags & BCM_PKT_F_TIMESYNC) {
                PBS_MH_V5_TS_PKT_SET(pbh);
            }
        } else if (SOC_DCB_TYPE(unit) == 24) {
            PBS_MH_V5_W1_SMOD_SET(pbh, src_mod, 1, 0, 0);
            PBS_MH_V6_W2_DPORT_COS_QNUM_SET(pbh, port, pkt->cos,
                     (SOC_INFO(unit).port_cosq_base[port] + pkt->cos),
                     prio_val);

            if (pkt->flags & BCM_PKT_F_TIMESYNC) {
                PBS_MH_V5_TS_PKT_SET(pbh);
            }
        } else if ((SOC_DCB_TYPE(unit) == 16) || (SOC_DCB_TYPE(unit) == 22)) {
            PBS_MH_V4_W2_SMOD_DPORT_COS_SET(pbh, src_mod, port,
                                            pkt->cos, prio_val, 0);
        } else {
            PBS_MH_V1_W2_SMOD_DPORT_COS_SET(pbh, src_mod, port,
                                            pkt->cos, prio_val, 0);
        }
    }
        if (pkt->flags & BCM_PKT_F_TIMESYNC) {
            if (SOC_DCB_TYPE(unit) == 23 || SOC_DCB_TYPE(unit) == 26 || \
                SOC_DCB_TYPE(unit) == 30 || SOC_DCB_TYPE(unit) == 31) {
                /* Triumph3 timestamp fields set */
                if (pkt->timestamp_flags & BCM_TX_TIMESYNC_ONE_STEP) {
                    PBS_MH_V7_TS_ONE_STEP_PKT_SET(pbh);

                    if ( pkt->timestamp_flags & 
                                    BCM_TX_TIMESYNC_ONE_STEP_INGRESS_SIGN) {
                        PBS_MH_V7_TS_ONE_STEP_INGRESS_SIGN_PKT_SET(pbh);
                    }

                    if ( pkt->timestamp_flags & 
                                    BCM_TX_TIMESYNC_ONE_STEP_HDR_START_OFFSET) {
                        PBS_MH_V7_TS_ONE_STEP_HDR_START_OFFSET_PKT_SET(pbh,
                                                         pkt->timestamp_offset);
                    }

                    if ( pkt->timestamp_flags & 
                                    BCM_TX_TIMESYNC_ONE_STEP_REGEN_UDP_CHKSUM) {
                        PBS_MH_V7_TS_ONE_STEP_HDR_START_REGEN_UDP_CHEKSUM_PKT_SET(pbh);
                    }
                } else {
                    /* Two-step time stamping */
                    PBS_MH_V7_TS_PKT_SET(pbh);
                }
            }  else if (SOC_DCB_TYPE(unit) == 24) {
                /* Katana timestamp fields set */
                if (pkt->timestamp_flags & BCM_TX_TIMESYNC_ONE_STEP) {
                    PBS_MH_V6_TS_ONE_STEP_PKT_SET(pbh);

                    if ( pkt->timestamp_flags & 
                                    BCM_TX_TIMESYNC_ONE_STEP_INGRESS_SIGN) {
                        PBS_MH_V6_TS_ONE_STEP_INGRESS_SIGN_PKT_SET(pbh);
                    }

                    if ( pkt->timestamp_flags & 
                                    BCM_TX_TIMESYNC_ONE_STEP_HDR_START_OFFSET) {
                        PBS_MH_V6_TS_ONE_STEP_HDR_START_OFFSET_PKT_SET(pbh, 
                                                         pkt->timestamp_offset);
                    }

                    if ( pkt->timestamp_flags & 
                                    BCM_TX_TIMESYNC_ONE_STEP_REGEN_UDP_CHKSUM) {
                        PBS_MH_V6_TS_ONE_STEP_HDR_START_REGEN_UDP_CHEKSUM_PKT_SET(pbh);
                    }
                } else {
                    /* Two-step time stamping */
                    PBS_MH_V6_TS_PKT_SET(pbh);
                }
            }
        }

    return BCM_E_NONE;
}
#endif

#ifdef BCM_HIGIG2_SUPPORT
/*
 * Function:
 *      _bcm_tx_gport_resolve
 * Purpose:
 *      Analyze the provided gport and convert it into Higig2 PPD2/3
 *      suitable format, if appropriate.
 * Parameters:
 *      gport - provided potential virtual port to resolve
 *      vp    - (OUT) resolved port info in Higig2 PD2/3 format
 *      physical - (OUT) TRUE if physical port, FALSE if virtual port
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC INLINE int
_bcm_tx_gport_resolve(bcm_gport_t gport, bcm_port_t *vp)
{
    bcm_port_t port;

    if (BCM_GPORT_IS_MPLS_PORT(gport)) {
        port = BCM_GPORT_MPLS_PORT_ID_GET(gport);
    } else if (BCM_GPORT_IS_MIM_PORT(gport)) {
        port = BCM_GPORT_MIM_PORT_ID_GET(gport);
    } else if (BCM_GPORT_IS_WLAN_PORT(gport)) {
        port = BCM_GPORT_WLAN_PORT_ID_GET(gport);
    } else if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        port = BCM_GPORT_VLAN_PORT_ID_GET(gport);
    } else if (BCM_GPORT_IS_SUBPORT_PORT(gport)) {
        port = BCM_GPORT_SUBPORT_PORT_GET(gport);
    } else if (BCM_GPORT_IS_NIV_PORT(gport)) {
        port = BCM_GPORT_NIV_PORT_ID_GET(gport);
    } else {
        /* Can't parse */
        return BCM_E_PORT;
    }

    *vp = port;
    return BCM_E_NONE;
}

STATIC INLINE int
_bcm_tx_pkt_stk_forward_to_hg2(int forward, int *fwd_type, int *multicast)
{
    /* Note:  All multicast types are translated to SOC_HIGIG_OP_IPMC
     * because the MC index must be translated into the IPMC range for
     * the FRC multicast field.
     */
    switch (forward) {
    case BCM_PKT_STK_FORWARD_CPU:
        *fwd_type = 0;
        *multicast = 0;
        break;
    case BCM_PKT_STK_FORWARD_L2_UNICAST:
        *fwd_type = 4;
        *multicast = 0;
        break;
    case BCM_PKT_STK_FORWARD_L3_UNICAST:
        *fwd_type = 2;
        *multicast = 0;
        break;
    case BCM_PKT_STK_FORWARD_L2_MULTICAST:
        *fwd_type = 4;
        *multicast = SOC_HIGIG_OP_IPMC;
        break;
    case BCM_PKT_STK_FORWARD_L2_MULTICAST_UNKNOWN:
        *fwd_type = 5;
        *multicast = SOC_HIGIG_OP_IPMC;
        break;
    case BCM_PKT_STK_FORWARD_L3_MULTICAST:
        *fwd_type = 2;
        *multicast = SOC_HIGIG_OP_IPMC;
        break;
    case BCM_PKT_STK_FORWARD_L3_MULTICAST_UNKNOWN:
        *fwd_type = 3;
        *multicast = SOC_HIGIG_OP_IPMC;
        break;
    case BCM_PKT_STK_FORWARD_L2_UNICAST_UNKNOWN:
        *fwd_type = 6;
        *multicast = SOC_HIGIG_OP_IPMC;
        break;
    case BCM_PKT_STK_FORWARD_BROADCAST:
        *fwd_type = 7;
        *multicast = SOC_HIGIG_OP_IPMC;
        break;
    default:
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}

#if defined(INCLUDE_L3) && defined(BCM_XGS3_SWITCH_SUPPORT)
STATIC INLINE int
_bcm_tx_pkt_stk_encap_to_repl_id(bcm_if_t encap, uint32 *repl_id)
{
    if (encap > 0) {
        if (encap < BCM_XGS3_EGRESS_IDX_MIN) {
            *repl_id = encap;
        } else if (encap < BCM_XGS3_MPATH_EGRESS_IDX_MIN) {
            *repl_id = encap - BCM_XGS3_EGRESS_IDX_MIN;
        } else if (encap < BCM_XGS3_MPLS_IF_EGRESS_IDX_MIN) {
            *repl_id = encap - BCM_XGS3_MPATH_EGRESS_IDX_MIN;
        } else if (encap < BCM_XGS3_DVP_EGRESS_IDX_MIN) {
            *repl_id = encap - BCM_XGS3_MPLS_IF_EGRESS_IDX_MIN;
        } else { 
            *repl_id = encap - BCM_XGS3_DVP_EGRESS_IDX_MIN;
        }
    } else {
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}
#endif /* INCLUDE_L3 && BCM_XGS3_SWITCH_SUPPORT */

/*
 * Function:
 *      _bcm_tx_hg2hdr_setup
 * Purpose:
 *      Setup HiGig2 header local buffer consistently with packet's
 *      data members.  Legacy HiGig header members are done by
 *      _bcm_tx_hghdr_setup.
 * Parameters:
 *      pkt - Packet to set up header from
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */

STATIC INLINE int
_bcm_tx_hg2hdr_setup(bcm_pkt_t *pkt)
{
    soc_higig2_hdr_t *xgh = (soc_higig2_hdr_t *)pkt->_higig;
    int unit = pkt->unit;
    uint32 vlan_val, prio_val, color, pfm, ppd = 0;
    uint32 src_port, src_mod;
    bcm_port_t dst_vp = 0, src_vp = 0;
    int multicast, fwd_type, mc_index_offset, mc_index;
#if defined(BCM_BRADLEY_SUPPORT)
    int bcast_size, mcast_size, ipmc_size;
#endif
    bcm_vlan_t vfi = 0;
#if defined(INCLUDE_L3) && defined(BCM_XGS3_SWITCH_SUPPORT)
    uint32 repl_id;
#endif /* INCLUDE_L3 && BCM_XGS3_SWITCH_SUPPORT */

    /* This must be first, so all of the following operate properly */
    soc_higig2_field_set(unit, xgh, HG_start, SOC_HIGIG2_START);

    /* Analyze the GPORTS */
    if (pkt->stk_flags & BCM_PKT_STK_F_DST_PORT) {
        if (pkt->stk_flags & BCM_PKT_STK_F_ENCAP_ID) {
            /* Can't mix dest port and encap ID */
            return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_tx_gport_resolve(pkt->dst_gport, &dst_vp));
    }
    if (pkt->stk_flags & BCM_PKT_STK_F_SRC_PORT) {
        BCM_IF_ERROR_RETURN
            (_bcm_tx_gport_resolve(pkt->src_gport, &src_vp));
    }

    /* What kind of PPD are we dealing with? */
    if (pkt->stk_flags &
        (BCM_PKT_STK_F_DEFERRED_DROP |
         BCM_PKT_STK_F_DEFERRED_CHANGE_PKT_PRIO |
         BCM_PKT_STK_F_DEFERRED_CHANGE_DSCP |
         BCM_PKT_STK_F_VLAN_TRANSLATE_NONE |
         BCM_PKT_STK_F_VLAN_TRANSLATE_UNCHANGED |
         BCM_PKT_STK_F_VLAN_TRANSLATE_CHANGED)) {
        ppd = 3;
    }
    if (dst_vp) {
        if (ppd > 2) { /* Conflict */
            return BCM_E_PARAM;
        }
        ppd = 2; /* Virtual dest port type */
    }
    if (pkt->stk_forward != BCM_PKT_STK_FORWARD_CPU) {
        if (ppd > 2) { /* Conflict */
            return BCM_E_PARAM;
        }
        ppd = 2; /* Virtual dest port type */
    }
    if (pkt->stk_flags & 
        (BCM_PKT_STK_F_TRUNK_FAILOVER | BCM_PKT_STK_F_DO_NOT_MODIFY)) {
        if (ppd > 2) { /* Conflict */
            return BCM_E_PARAM;
        }
    }
    if (pkt->stk_flags & BCM_PKT_STK_F_FAILOVER) {
        if (ppd > 2) { /* Conflict */
            return BCM_E_PARAM;
        }
        ppd = 2; /* Protection nexthop only availabe in PPD 2 */
    }
#if defined(INCLUDE_L3) && defined(BCM_TRX_SUPPORT)
    if (_BCM_VPN_IS_VPLS(pkt->vlan) || _BCM_VPN_IS_MIM(pkt->vlan)) {
        if (ppd > 2) { /* Conflict */
            return BCM_E_PARAM;
        }
        _BCM_VPN_GET(vfi, _BCM_VPN_TYPE_VFI, pkt->vlan);
        ppd = 2; /* Only PPD with VFI */
    }
#endif /* INCLUDE_L3 && BCM_TRX_SUPPORT */
    if (pkt->stk_flags & BCM_PKT_STK_F_CLASSIFICATION_TAG) {
        if (ppd == 2) { /* Conflict */
            return BCM_E_PARAM;
        }
        /*    coverity[new_values]    */
        if (ppd != 3) { /* Conflict */
            ppd = 1; /* Only PPD with VFI */
            if (pkt->stk_flags & (BCM_PKT_STK_F_PRESERVE_DSCP |
                                  BCM_PKT_STK_F_PRESERVE_PKT_PRIO)) {
                return BCM_E_PARAM;
            }
        }
    }
    if ((pkt->stk_flags & BCM_PKT_STK_F_MIRROR) ||
        (pkt->stk_flags & BCM_PKT_STK_F_ENCAP_ID) ){
        if ((ppd == 1) || (ppd == 3)) { /* Conflict */
            return BCM_E_PARAM;
        }
    }
    /* Otherwise, we will just use PPD0 */

    /* We must fill in the type before filling in the PPD-specific fields */
    soc_higig2_field_set(unit, xgh, HG_ppd_type, ppd);

    /* This field is common to all PPDs */
    soc_higig2_field_set(unit, xgh, HG_opcode, pkt->opcode);
    /* Get multicast from opcode.  Override if PPD2 */
    multicast = ((pkt->opcode == SOC_HIGIG_OP_CPU) ||
                 (pkt->opcode == SOC_HIGIG_OP_UC)) ? 0 : pkt->opcode;

    /* Now fill in by type */
    switch (ppd) {
    case 3:
        soc_higig2_field_set(unit, xgh, HG_src_vp, src_vp);
        soc_higig2_field_set(unit, xgh, HG_src_type, 0);
        if (pkt->stk_flags & BCM_PKT_STK_F_PRESERVE_DSCP) {
            soc_higig2_field_set(unit, xgh, HG_preserve_dscp, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_PRESERVE_PKT_PRIO) {
            soc_higig2_field_set(unit, xgh, HG_preserve_dot1p, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_DO_NOT_LEARN) {
            soc_higig2_field_set(unit, xgh, HG_donot_learn, 1);
        }
        soc_higig2_field_set(unit, xgh, HG_opcode, pkt->opcode);
        soc_higig2_field_set(unit, xgh, HG_data_container_type, 1);
        /* Container info */
        if (pkt->stk_flags & BCM_PKT_STK_F_CLASSIFICATION_TAG) {
            soc_higig2_field_set(unit, xgh, HG_ctag,
                                pkt->stk_classification_tag);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_DEFERRED_DROP) {
            soc_higig2_field_set(unit, xgh, HG_deferred_drop, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_DEFERRED_CHANGE_PKT_PRIO) {
            soc_higig2_field_set(unit, xgh, HG_deferred_change_pkt_pri, 1);
            soc_higig2_field_set(unit, xgh, HG_new_pkt_pri, pkt->stk_pkt_prio);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_DEFERRED_CHANGE_DSCP) {
            soc_higig2_field_set(unit, xgh, HG_deferred_change_dscp, 1);
            soc_higig2_field_set(unit, xgh, HG_new_dscp, pkt->stk_dscp);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_VLAN_TRANSLATE_CHANGED) {
            soc_higig2_field_set(unit, xgh, HG_vxlt_done, 3);
        } else if (pkt->stk_flags & BCM_PKT_STK_F_VLAN_TRANSLATE_UNCHANGED) {
            soc_higig2_field_set(unit, xgh, HG_vxlt_done, 1);
        } else if (pkt->stk_flags & BCM_PKT_STK_F_VLAN_TRANSLATE_NONE) {
            soc_higig2_field_set(unit, xgh, HG_vxlt_done, 0);
        }
        /* Must transmit VLAN tag with this format */
        pkt->stk_flags |= BCM_PKT_STK_F_TX_TAG;
        break;
    case 2:
        BCM_IF_ERROR_RETURN
            (_bcm_tx_pkt_stk_forward_to_hg2(pkt->stk_forward, &fwd_type,
                                            &multicast));
        soc_higig2_field_set(unit, xgh, HG_fwd_type, fwd_type);
        soc_higig2_field_set(unit, xgh, HG_multipoint, multicast ? 1 : 0);
#if defined(INCLUDE_L3) && defined(BCM_XGS3_SWITCH_SUPPORT)
        if (pkt->stk_flags & BCM_PKT_STK_F_ENCAP_ID) {
            BCM_IF_ERROR_RETURN
                (_bcm_tx_pkt_stk_encap_to_repl_id(pkt->stk_encap_id,
                                                  &repl_id));
            soc_higig2_field_set(unit, xgh, HG_replication_id, repl_id);
        } else
#endif /* INCLUDE_L3 && BCM_XGS3_SWITCH_SUPPORT */
        {
            if (multicast) {
                mc_index = _BCM_MULTICAST_ID_GET(pkt->multicast_group);
                soc_higig2_field_set(unit, xgh, HG_dst_vp, mc_index);
            } else {
                soc_higig2_field_set(unit, xgh, HG_dst_vp, dst_vp);
                soc_higig2_field_set(unit, xgh, HG_dst_type, 0);
            }
        }
        soc_higig2_field_set(unit, xgh, HG_src_vp, src_vp);
        soc_higig2_field_set(unit, xgh, HG_src_type, 0);
        soc_higig2_field_set(unit, xgh, HG_vni, vfi);
        if (pkt->stk_flags & BCM_PKT_STK_F_MIRROR) {
            soc_higig2_field_set(unit, xgh, HG_mirror, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_DO_NOT_MODIFY) {
            soc_higig2_field_set(unit, xgh, HG_donot_modify, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_DO_NOT_LEARN) {
            soc_higig2_field_set(unit, xgh, HG_donot_learn, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_TRUNK_FAILOVER) {
            soc_higig2_field_set(unit, xgh, HG_lag_failover, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_FAILOVER) {
            soc_higig2_field_set(unit, xgh, HG_protection_status, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_PRESERVE_DSCP) {
            soc_higig2_field_set(unit, xgh, HG_preserve_dscp, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_PRESERVE_PKT_PRIO) {
            soc_higig2_field_set(unit, xgh, HG_preserve_dot1p, 1);
        }
        /* Must transmit VLAN tag with this format */
        pkt->stk_flags |= BCM_PKT_STK_F_TX_TAG;
        break;
    case 1:
        if (pkt->stk_flags & BCM_PKT_STK_F_CLASSIFICATION_TAG) {
            soc_higig2_field_set(unit, xgh, HG_ctag,
                                pkt->stk_classification_tag);
        }
        vlan_val = BCM_PKT_VLAN_CONTROL(pkt);
        soc_higig2_field_set(unit, xgh, HG_vlan_tag, vlan_val);
        pfm = (pkt->flags & BCM_TX_PFM) ?
            pkt->pfm : SOC_DEFAULT_DMA_PFM_GET(unit);
        soc_higig2_field_set(unit, xgh, HG_pfm, pfm);
        break;
    case 0:
        if (!(pkt->rx_untagged & BCM_PKT_OUTER_UNTAGGED)) {
            soc_higig2_field_set(unit, xgh, HG_ingress_tagged, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_MIRROR) {
            soc_higig2_field_set(unit, xgh, HG_mirror_only, 1);
            soc_higig2_field_set(unit, xgh, HG_mirror, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_DO_NOT_MODIFY) {
            soc_higig2_field_set(unit, xgh, HG_donot_modify, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_DO_NOT_LEARN) {
            soc_higig2_field_set(unit, xgh, HG_donot_learn, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_TRUNK_FAILOVER) {
            soc_higig2_field_set(unit, xgh, HG_lag_failover, 1);
        }
        if (pkt->flags & BCM_PKT_F_ROUTED) {
            soc_higig2_field_set(unit, xgh, HG_l3, 1);
        }
        vlan_val = BCM_PKT_VLAN_CONTROL(pkt);
        soc_higig2_field_set(unit, xgh, HG_vlan_tag, vlan_val);
        pfm = (pkt->flags & BCM_TX_PFM) ?
            pkt->pfm : SOC_DEFAULT_DMA_PFM_GET(unit);
        soc_higig2_field_set(unit, xgh, HG_pfm, pfm);
        if (pkt->stk_flags & BCM_PKT_STK_F_PRESERVE_DSCP) {
            soc_higig2_field_set(unit, xgh, HG_preserve_dscp, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_PRESERVE_PKT_PRIO) {
            soc_higig2_field_set(unit, xgh, HG_preserve_dot1p, 1);
        }
#if defined(INCLUDE_L3) && defined(BCM_XGS3_SWITCH_SUPPORT)
        if (pkt->stk_flags & BCM_PKT_STK_F_ENCAP_ID) {
            BCM_IF_ERROR_RETURN
                (_bcm_tx_pkt_stk_encap_to_repl_id(pkt->stk_encap_id,
                                                  &repl_id));
            soc_higig2_field_set(unit, xgh, HG_replication_id, repl_id);
        }
#endif /* INCLUDE_L3 && BCM_XGS3_SWITCH_SUPPORT */
        break;
    /* Defensive Default */
    /* coverity[dead_error_begin] */
    default:
        return BCM_E_INTERNAL;
    }

    /* Finally, fill in the common FRC fields */
    prio_val = (pkt->flags & BCM_TX_PRIO_INT) ? pkt->prio_int : pkt->cos;
    soc_higig2_field_set(unit, xgh, HG_tc, prio_val);

    /* multicast from opcode or PPD2 flags */
    if (multicast) {
        mc_index_offset = 0;

#if defined(BCM_BRADLEY_SUPPORT)
        /* Higig2 has different ranges for different multicast types */
        
        BCM_IF_ERROR_RETURN
            (soc_hbx_higig2_mcast_sizes_get(unit, &bcast_size, &mcast_size,
                                            &ipmc_size));
        switch (multicast) {
        case SOC_HIGIG_OP_IPMC:
            mc_index_offset += mcast_size;
            /* Fall thru */
        case SOC_HIGIG_OP_MC:
            mc_index_offset += bcast_size;
            /* Fall thru */
        case SOC_HIGIG_OP_BC:
            break;
        default:
            /* Unknown opcode */
            return BCM_E_PARAM;
        }
#endif
        soc_higig2_field_set(unit, xgh, HG_mcst, 1);
        mc_index = _BCM_MULTICAST_ID_GET(pkt->multicast_group);
        soc_higig2_field_set(unit, xgh, HG_mgid,
                             mc_index + mc_index_offset);
    } else {
        soc_higig2_field_set(unit, xgh, HG_dst_mod, pkt->dest_mod);
        soc_higig2_field_set(unit, xgh, HG_dst_port, pkt->dest_port);
    }

    src_mod = (pkt->flags & BCM_TX_SRC_MOD) ?
        pkt->src_mod : SOC_DEFAULT_DMA_SRCMOD_GET(unit);
    soc_higig2_field_set(unit, xgh, HG_src_mod, src_mod);

    src_port = (pkt->flags & BCM_TX_SRC_PORT) ?
        pkt->src_port : SOC_DEFAULT_DMA_SRCPORT_GET(unit);
    soc_higig2_field_set(unit, xgh, HG_src_port, src_port);

    soc_higig2_field_set(unit, xgh, HG_lbid, pkt->stk_load_balancing_number);

    switch (pkt->color) {
    case bcmColorGreen:
        color = 0;
        break;
    case bcmColorYellow:
        color = 3;
        break;
    case bcmColorRed:
        color = 1;
        break;
    case bcmColorBlack:
    default:
        return BCM_E_PARAM;
    }
    soc_higig2_field_set(unit, xgh, HG_dp, color);
    
    return BCM_E_NONE;
}
#endif /* BCM_HIGIG2_SUPPORT */

/*
 * Function:
 *      _bcm_tx_hghdr_setup
 * Purpose:
 *      Setup HiGig header local buffer consistently with packet's
 *      data members.
 * Parameters:
 *      pkt - Packet to set up header from
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The VLAN value is generally gotten from packet data (determined
 *      by flags;  The HG priority field (aka HG_cos) is either the
 *      prio_int or cos field of the packet depending on the TX_PRIO_INT
 *      flag.
 */

STATIC INLINE int
_bcm_tx_hghdr_setup(bcm_pkt_t *pkt)
{
#ifdef  BCM_XGS_SUPPORT
    soc_higig_hdr_t *xgh = (soc_higig_hdr_t *)pkt->_higig;
    int unit = pkt->unit;
    uint32 vlan_val;
    uint32 prio_val;
    uint32 src_port;
    uint32 src_mod;
    uint32 pfm, color;

    sal_memset(xgh, 0, sizeof(pkt->_higig));
    soc_higig_field_set(unit, xgh, HG_start, SOC_HIGIG_START);
#ifdef BCM_HIGIG2_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {

        if (!BCM_PKT_TX_ETHER(pkt)) { /* Raw mode */
            bcm_pbmp_t tx_pbmp;
            int port;
            /*
             * Use HiGig2 format if port is configured for  HiGig2 encapsulation.
             * Note that tx_pbmp only contains a single port at this point.
             */
            BCM_PBMP_ASSIGN(tx_pbmp, pkt->tx_pbmp);
            BCM_API_XLATE_PORT_PBMP_A2P(unit, &tx_pbmp);
            BCM_PBMP_ITER(tx_pbmp, port) {
                /* Use cached info about HiGig2 instead of reading h/w registers */
                if (IS_HG2_ENABLED_PORT(unit, port)) {
                    return _bcm_tx_hg2hdr_setup(pkt);
                }
                break;
            }
        } else { /* Packet to be sent through ingress logic */
            /* If unit is Higig2 capable, set up Higig2 header */
            if (soc_feature(unit, soc_feature_higig2)) {
                return _bcm_tx_hg2hdr_setup(pkt);
            }
        }
    }
#if defined(BCM_SBX_CMIC_SUPPORT)
    if (SOC_IS_SIRIUS(unit)) {
        int port;
        /*
         * Use HiGig2 format if port is configured for  HiGig2 encapsulation.
         * Note that tx_pbmp only contains a single port at this point.
         */
        BCM_PBMP_ITER(pkt->tx_pbmp, port) {
            /* Use cached info about HiGig2 instead of reading h/w registers */
            if (IS_HG2_ENABLED_PORT(unit, port)) {
                return _bcm_tx_hg2hdr_setup(pkt);
            }
            break;
        }
    }
#endif /* BCM_SBX_CMIC_SUPPORT */
#endif /* BCM_HIGIG2_SUPPORT */
    soc_higig_field_set(unit, xgh, HG_hgi, SOC_HIGIG_HGI);
    soc_higig_field_set(unit, xgh, HG_opcode, pkt->opcode);
    soc_higig_field_set(unit, xgh, HG_hdr_format, SOC_HIGIG_HDR_DEFAULT);

    vlan_val = BCM_PKT_VLAN_CONTROL(pkt);
    soc_higig_field_set(unit, xgh, HG_vlan_tag, vlan_val);
    if ((pkt->opcode == SOC_HIGIG_OP_MC) ||
        (pkt->opcode == SOC_HIGIG_OP_IPMC)) {
        soc_higig_field_set(unit, xgh, HG_l2mc_ptr,
                            _BCM_MULTICAST_ID_GET(pkt->multicast_group));
    } else {
        soc_higig_field_set(unit, xgh, HG_dst_mod, pkt->dest_mod);
        soc_higig_field_set(unit, xgh, HG_dst_port, pkt->dest_port);
    }

    src_mod = (pkt->flags & BCM_TX_SRC_MOD) ?
        pkt->src_mod : SOC_DEFAULT_DMA_SRCMOD_GET(unit);
    soc_higig_field_set(unit, xgh, HG_src_mod, src_mod);

    src_port = (pkt->flags & BCM_TX_SRC_PORT) ?
        pkt->src_port : SOC_DEFAULT_DMA_SRCPORT_GET(unit);
    soc_higig_field_set(unit, xgh, HG_src_port, src_port);

    pfm = (pkt->flags & BCM_TX_PFM) ?
        pkt->pfm : SOC_DEFAULT_DMA_PFM_GET(unit);
    soc_higig_field_set(unit, xgh, HG_pfm, pfm);

    prio_val = (pkt->flags & BCM_TX_PRIO_INT) ? pkt->prio_int : pkt->cos;
    soc_higig_field_set(unit, xgh, HG_cos, prio_val);

    if (pkt->stk_flags & BCM_PKT_STK_F_CLASSIFICATION_TAG) {
        /* Overlay 2 */
        if ((pkt->stk_flags & (BCM_PKT_STK_F_MIRROR |
                              BCM_PKT_STK_F_DO_NOT_MODIFY |
                              BCM_PKT_STK_F_DO_NOT_LEARN |
                              BCM_PKT_STK_F_TRUNK_FAILOVER)) |
             (pkt->flags & BCM_PKT_F_ROUTED)) {
            /* Conflict */
            return BCM_E_PARAM;
        }
        soc_higig_field_set(unit, xgh, HG_ctag,
                            pkt->stk_classification_tag);
        soc_higig_field_set(unit, xgh, HG_hdr_format, 1);
    } else {
        /* Overlay 1 */
        if (pkt->stk_flags & BCM_PKT_STK_F_MIRROR) {
            soc_higig_field_set(unit, xgh, HG_mirror_only, 1);
            soc_higig_field_set(unit, xgh, HG_mirror, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_DO_NOT_MODIFY) {
            soc_higig_field_set(unit, xgh, HG_donot_modify, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_DO_NOT_LEARN) {
            soc_higig_field_set(unit, xgh, HG_donot_learn, 1);
        }
        if (pkt->stk_flags & BCM_PKT_STK_F_TRUNK_FAILOVER) {
            soc_higig_field_set(unit, xgh, HG_lag_failover, 1);
        }
        if (pkt->flags & BCM_PKT_F_ROUTED) {
            soc_higig_field_set(unit, xgh, HG_l3, 1);
        }
        /* Header format 0 */
    }

    switch (pkt->color) {
    case bcmColorGreen:
        color = 0;
        break;
    case bcmColorYellow:
        color = 3;
        break;
    case bcmColorRed:
        color = 1;
        break;
    case bcmColorBlack:
    default:
        return BCM_E_PARAM;
    }
    soc_higig_field_set(unit, xgh, HG_cng, color);

    return BCM_E_NONE;
#endif  /* BCM_XGS_SUPPORT */

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      _bcm_tx_sltag_setup
 * Purpose:
 *      Setup the SL tag consistently with data in packet.
 * Parameters:
 *      pkt - Packet to set up header from
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      
 */

STATIC INLINE int
_bcm_tx_sltag_setup(bcm_pkt_t *pkt)
{
    stk_tag_t   *sltag = (stk_tag_t *)pkt->_sltag;
    int         slcount;

    sal_memset(sltag, 0, sizeof(*sltag));
    if (pkt->opcode == BCM_HG_OPCODE_CPU) {
        slcount = 1;
    } else {
        slcount = 0;
    }
    sltag->stk_cnt = slcount;
    sltag->src_tgid = (pkt->src_port >> 3) & 0x7;
    sltag->src_rtag = pkt->src_port & 0x7;
    sltag->pfm = pkt->pfm;
    sltag->ed = 1;              
    sltag->stk_modid = pkt->src_mod;
    return BCM_E_NONE;
}

/*
 * Set up SL/HG tags according to system setup;
 * Assumes unit is local and packet is non-NULL
 */

STATIC INLINE int
_bcm_tx_pkt_tag_setup(int unit, bcm_pkt_t *pkt)
{
    if (!SOC_IS_ARAD(unit)) {
    BCM_PKT_HGHDR_CLR(pkt); /* Ignore user provided BCM_PKT_F_HGHDR flag */
    }

    if (SOC_IS_XGS3_SWITCH(unit)) {
        /*
         * XGS3: Higig header in packet data stream (Raw mode)
         */
        bcm_pbmp_t      tmp_pbm;
        SOC_PBMP_ASSIGN(tmp_pbm, PBMP_ST_ALL(unit));
        BCM_API_XLATE_PORT_PBMP_P2A(unit, &tmp_pbm);
        SOC_PBMP_AND(tmp_pbm, pkt->tx_pbmp);

        if ((!BCM_PKT_TX_ETHER(pkt)) &&
            (SOC_PBMP_NOT_NULL(tmp_pbm))) { /* Raw mode on HG port */
            BCM_PKT_HGHDR_REQUIRE(pkt);
        }

        /*
         * Construct higig header from discrete members in the pkt
         * if one is not provided.
         */
        if (!BCM_PKT_TX_HG_READY(pkt)) {
            BCM_IF_ERROR_RETURN(_bcm_tx_hghdr_setup(pkt));
        }
    } else if (SOC_IS_XGS12_FABRIC(unit)) {
        BCM_PKT_HGHDR_REQUIRE(pkt);
        if (!BCM_PKT_TX_HG_READY(pkt)) {
            BCM_IF_ERROR_RETURN(_bcm_tx_hghdr_setup(pkt));
        }
#if defined(BCM_SBX_CMIC_SUPPORT)
    } else if (SOC_IS_SIRIUS(unit)) {

	bcm_tx_pkt_setup(unit, pkt);

        if (!BCM_PKT_TX_ETHER(pkt)) {
            BCM_PKT_HGHDR_REQUIRE(pkt);

	    if (!BCM_PKT_TX_HG_READY(pkt)) {
	      BCM_IF_ERROR_RETURN(_bcm_tx_hghdr_setup(pkt));
	    }
        }
#endif
    } else if (SOC_SL_MODE(unit)) {


        BCM_PKT_SLTAG_REQUIRE(pkt);
        BCM_IF_ERROR_RETURN(_bcm_tx_sltag_setup(pkt));
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_tx_pkt_setup
 * Purpose:
 *      Default packet setup routine for transmit
 * Parameters:
 *      unit         - Unit on which being transmitted
 *      tx_pkt       - Packet to update
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      tx_pkt->tx_pbmp should be set before calling this function
 *      Currently, the main thing this does is force the HG header
 *      on Hercules.
 */

int
bcm_common_tx_pkt_setup(int unit, bcm_pkt_t *tx_pkt)
{
    if (tx_pkt == NULL) {
        return BCM_E_PARAM;
    }

    if (!BCM_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }
    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }
    if (!BCM_IS_LOCAL(unit)) {
        /* Don't need to do special setup for tunnelled packets */
        return BCM_E_NONE;
    }

    return _bcm_tx_pkt_tag_setup(unit, tx_pkt);
}

/*
 * Function:
 *      bcm_tx_init
 * Purpose:
 *      Initialize BCM TX API
 * Parameters:
 *      unit - transmission unit
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Currently, this just allocates shared memory space for the
 *      CRC append memory.  This memory is zero and used by all packets
 *      that require CRC append (allocate).
 *
 *      This pointer is only allocated once.  If allocation fails,
 *      BCM_E_MEMORY is returned.
 */

int
bcm_common_tx_init(int unit)
{
    sal_thread_t tx_tid = SAL_THREAD_ERROR;
#ifdef BCM_XGS3_SWITCH_SUPPORT
    sal_thread_t xgs3_async_tid = SAL_THREAD_ERROR;
#endif

    if (!BCM_CONTROL_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }
    if (!BCM_IS_LOCAL(unit)) {
        /* Set up TX tunnel receiver and transmitter */
        /* Currently this is done elsewhere */
        return BCM_E_NONE;
    }
    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    if (SOC_IS_RCPU_ONLY(unit)) {
        return BCM_E_NONE;
    }

    if (_null_crc_ptr == NULL) {
        _null_crc_ptr = soc_cm_salloc(unit, sizeof(uint32), "Static CRC");
        if (_null_crc_ptr == NULL) {
            goto error;
        }
        sal_memset(_null_crc_ptr, 0, sizeof(uint32));
    }

    if (tx_cb_sem == NULL) {
        tx_cb_sem = sal_sem_create("tx cb", sal_sem_BINARY, 0);
        if (tx_cb_sem == NULL) {
            goto error;
        }
    }

    /* Allocate a min size pkt for padding */
    if (_pkt_pad_ptr == NULL) {
        _pkt_pad_ptr = soc_cm_salloc(unit, ENET_MIN_PKT_SIZE, "TX Pkt Pad");
        if (_pkt_pad_ptr == NULL) {
            goto error;
        }
        sal_memset(_pkt_pad_ptr, 0, ENET_MIN_PKT_SIZE);
    }


    if (!_tx_init) {  /* Only create the thread once */

        /* Start up the tx callback handler thread */
        tx_tid = sal_thread_create("bcmTX", SAL_THREAD_STKSZ,
                                soc_property_get(unit,
                                                 spn_BCM_TX_THREAD_PRI,
                                                 DEFAULT_TX_PRI),
                                _bcm_tx_callback_thread, NULL);
        if (tx_tid == SAL_THREAD_ERROR) {
            goto error;
        }
#ifdef BCM_XGS3_SWITCH_SUPPORT

        if (SOC_IS_XGS3_SWITCH(unit)) {
            _xgs3_async_tx_sem = sal_sem_create("xgs3 tx async",
                    sal_sem_COUNTING, 0);
            if (_xgs3_async_tx_sem  == NULL) {
                goto error;
            }

            xgs3_async_tid = sal_thread_create("bcmXGS3AsyncTX",
                                               SAL_THREAD_STKSZ,
                                soc_property_get(unit,
                                                 spn_BCM_TX_THREAD_PRI,
                                                 DEFAULT_TX_PRI),
                                _xgs3_async_thread, NULL);
            if (xgs3_async_tid == SAL_THREAD_ERROR) {
                goto error;
            }
        }
#endif
    }
    _tx_init = TRUE;

    return BCM_E_NONE;

 error:

    /* Clean up any allocated resources */
    
#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (_xgs3_async_tx_sem) {
        sal_sem_destroy(_xgs3_async_tx_sem);
    }
#endif

    if (tx_tid != SAL_THREAD_ERROR) {
        sal_thread_destroy(tx_tid);
    }

    if (_pkt_pad_ptr) {
        soc_cm_sfree(unit, _pkt_pad_ptr);
    }

    if (tx_cb_sem) {
        sal_sem_destroy(tx_cb_sem);
    }

    if (_null_crc_ptr) {
        soc_cm_sfree(unit, _null_crc_ptr);
    }

    return BCM_E_MEMORY;
    
}

/* Check the packet flags for inconsistencies or unsupported configs */

STATIC int
_tx_flags_check(int unit, bcm_pkt_t *pkt)
{
    /* Check for acceptable/consistent flags */
    if (pkt->flags & BCM_TX_PRIO_INT) {
        if (SOC_IS_DRACO1(unit) || SOC_IS_LYNX(unit)) {
            if (pkt->cos != pkt->prio_int) {
                LOG_ERROR(BSL_LS_BCM_TX,
                          (BSL_META_U(unit,
                                      "Cannot set cos != prio_int on TX for 5690/74\n")));
                return BCM_E_PARAM;
            }
        }
    }

    return BCM_E_NONE;
}

STATIC INLINE int
_bcm_tx_cb_intr_enabled(void)
{
#ifdef TX_CB_INTR
   return BCM_E_NONE;
#else
   return BCM_E_UNAVAIL;
#endif
}
#ifdef BCM_XGS3_SWITCH_SUPPORT
STATIC void
_xgs3_calculate_tx_packet_size(int unit, dv_t *dv, int *bytes)
{
    int i;
    switch (SOC_DCB_TYPE(unit)) {
    default:
    case 9:
        for (i = 0; i < dv->dv_vcnt; i++) {
	    *bytes += ((dcb9_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
        }
	return;
#ifdef  BCM_EASYRIDER_SUPPORT
    case 10:
        for (i = 0; i < dv->dv_vcnt; i++) {
	    *bytes += ((dcb10_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
        }
	return;
#endif /* BCM_EASYRIDER_SUPPORT */
#ifdef  BCM_BRADLEY_SUPPORT
    case 11:
        for (i = 0; i < dv->dv_vcnt; i++) {
	    *bytes += ((dcb11_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
        }
	return;
#endif /* BCM_BRADLEY_SUPPORT */
#ifdef BCM_RAPTOR_SUPPORT
    case 12:
        for (i = 0; i < dv->dv_vcnt; i++) {
	    *bytes += ((dcb12_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
        }
	return;
#endif /* BCM_RAPTOR_SUPPORT */
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_FIREBOLT_SUPPORT)
    case 13:
        for (i = 0; i < dv->dv_vcnt; i++) {
	    *bytes += ((dcb13_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
        }
	return;
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_FIREBOLT_SUPPORT */
#ifdef  BCM_TRIUMPH_SUPPORT
    case 14:
        for (i = 0; i < dv->dv_vcnt; i++) {
	    *bytes += ((dcb14_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
        }
	return;
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_RAPTOR_SUPPORT
    case 15:
        for (i = 0; i < dv->dv_vcnt; i++) {
	    *bytes += ((dcb15_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
        }
	return;
#endif /* BCM_RAPTOR_SUPPORT */
#ifdef BCM_SCORPION_SUPPORT
    case 16:
        for (i = 0; i < dv->dv_vcnt; i++) {
	    *bytes += ((dcb16_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
        }
	return;
#endif /* BCM_SCORPION_SUPPORT */
#ifdef BCM_HAWKEYE_SUPPORT
	case 17:
        for (i = 0; i < dv->dv_vcnt; i++) {
	    *bytes += ((dcb17_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
        }
	return;
#endif /* BCM_HAWKEYE_SUPPORT */
#ifdef BCM_RAPTOR_SUPPORT
    case 18:
        for (i = 0; i < dv->dv_vcnt; i++) {
	    *bytes += ((dcb18_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
        }
	return;
#endif /* BCM_RAPTOR_SUPPORT */
#ifdef  BCM_TRIUMPH2_SUPPORT
    case 19:
        for (i = 0; i < dv->dv_vcnt; i++) {
	    *bytes += ((dcb19_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
        }
	return;
#endif /* BCM_TRIUMPH2_SUPPORT */
#ifdef  BCM_ENDURO_SUPPORT
    case 20:
    case 30:
      for (i = 0; i < dv->dv_vcnt; i++) {
          *bytes += ((dcb20_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
      }
      return;
#endif /* BCM_ENDURO_SUPPORT */
#ifdef  BCM_TRIDENT_SUPPORT
    case 21:
      for (i = 0; i < dv->dv_vcnt; i++) {
          *bytes += ((dcb21_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
      }
      return;
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef  BCM_SHADOW_SUPPORT
    case 22:
      for (i = 0; i < dv->dv_vcnt; i++) {
          *bytes += ((dcb22_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
      }
      return;
#endif /* BCM_SHADOW_SUPPORT */
#ifdef  BCM_TRIUMPH3_SUPPORT
    case 23:
      for (i = 0; i < dv->dv_vcnt; i++) {
          *bytes += ((dcb23_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
      }
      return;
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef  BCM_KATANA_SUPPORT
    case 24:
      for (i = 0; i < dv->dv_vcnt; i++) {
          *bytes += ((dcb24_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
      }
      return;
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
    case 26:
      for (i = 0; i < dv->dv_vcnt; i++) {
          *bytes += ((dcb26_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
      }
      return;
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef  BCM_KATANA2_SUPPORT
        case 29:
          for (i = 0; i < dv->dv_vcnt; i++) {
              *bytes += ((dcb29_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
          }
          return;
#endif /* BCM_KATANA2_SUPPORT */
#ifdef  BCM_GREYHOUND_SUPPORT
    case 31:
      for (i = 0; i < dv->dv_vcnt; i++) {
          *bytes += ((dcb31_t *)SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, i))->c_count;
      }
      return;
#endif /* BCM_GREYHOUND_SUPPORT */
    }
}
#endif /* BCM_XGS3_SWITCH_SUPPORT */

/*
 * Function:
 *      _bcm_tx
 * Purpose:
 *      Transmit a single packet
 * Parameters:
 *      unit - transmission unit
 *      pkt - The tx packet structure
 *      cookie - Callback cookie when tx done
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      To send unicast, broadcast or multicast packets from a switch
 *      chip, one must include all the outgoing ports in the port bitmap
 *      (pkt->pi_pbmp, pkt->pi_upbmp) and include the appropriate MAC DA
 *      in the packet data.  To send different types of packet from a
 *      fabric chip, it is different and the Higig opcode
 *      (BCM_HG_OPCODE_xxx) indicates the type of packet.
 *
 *      If the pkt->call_back function is non-null, executes async.
 *      Otherwise, will not return until DMA completes.
 *
 *      Packet format is described by the packet flags.  In particular,
 *      if the packet has a HiGig header (as indicated in the packet's
 *      flags) it will not have a VLAN tag set up for DMA.  Care should
 *      be taken as this should only happen for Hercules (5670, 71).
 *      No provision is made for forcing the addition of both a VLAN
 *      tag and a HiGig header.
 *
 *      If the unit is not local, TX tunnelling is automatically
 *      invoked.  CURRENT RESTRICTION:  Asynchronous tunnelled packets
 *      cannot receive cookie on callback.
 */

int
_bcm_tx(int unit, bcm_pkt_t *pkt, void *cookie)
{
    dv_t *dv = NULL;
    int rv = BCM_E_NONE;
    char fmt[SOC_PBMP_FMT_LEN];
    char *err_msg = NULL;
    const int pkt_unit = (int) pkt->unit;

    if (!BCM_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    if (!pkt || !pkt->pkt_data || (pkt->blk_count == 0) ||
        !BCM_UNIT_VALID(pkt_unit)
      ) {
        return BCM_E_PARAM;
    }

    if (bsl_check(bslLayerSoc, bslSourceTx, bslSeverityNormal, unit)) {
        bcm_pbmp_t tx_pbmp, tx_upbmp;
        BCM_PBMP_ASSIGN(tx_pbmp, pkt->tx_pbmp);
        BCM_PBMP_ASSIGN(tx_upbmp, pkt->tx_upbmp);
        BCM_API_XLATE_PORT_PBMP_A2P(unit, &tx_pbmp);
        BCM_API_XLATE_PORT_PBMP_A2P(unit, &tx_upbmp);
        LOG_INFO(BSL_LS_BCM_TX,
                 (BSL_META_U(unit,
                             "bcm_tx: pkt, u %d. len[0] %d to %s. "),
                             unit, pkt->pkt_data[0].len,
                  SOC_PBMP_FMT(tx_pbmp, fmt)
                  ));
        LOG_INFO(BSL_LS_BCM_TX,
                 (BSL_META_U(unit,
                             "%s. flags 0x%x\n"),
                             SOC_PBMP_FMT(tx_upbmp, fmt),
                  pkt->flags));
        LOG_INFO(BSL_LS_BCM_TX,
                 (BSL_META_U(unit,
                             "bcm_tx: dmod %d, dport %d, chan %d, op %d cos %d\n"),
                             pkt->dest_mod, pkt->dest_port, pkt->dma_channel, pkt->opcode,
                  pkt->cos));
        {
            uint16 i;
            LOG_INFO(BSL_LS_BCM_TX,
                     (BSL_META_U(unit,
                                 "bcm_tx: packet: ")));
            for (i=0; i<pkt->pkt_data[0].len ; i++) {
                LOG_INFO(BSL_LS_BCM_TX,
                         (BSL_META_U(unit,
                                     "%.2x"),pkt->pkt_data[0].data[i]));
            }
            LOG_INFO(BSL_LS_BCM_TX,
                     (BSL_META_U(unit,
                                 "\n")));
        }
    }
#ifdef  BCM_RPC_SUPPORT
    if (!BCM_IS_LOCAL(unit)) { /* Tunnel the packet to the remote CPU */
        bcm_cpu_tunnel_mode_t mode = BCM_CPU_TUNNEL_PACKET; /* default */

        if (pkt->call_back != NULL && cookie != NULL) {
            LOG_ERROR(BSL_LS_BCM_TX,
                      (BSL_META_U(unit,
                                  "bcm_tx ERROR:  "
                                  "Cookie non-NULL on async tunnel call\n")));
            return BCM_E_PARAM;
        }

        if (pkt->flags & BCM_TX_BEST_EFFORT) {
            mode = BCM_CPU_TUNNEL_PACKET_BEST_EFFORT;
        } else if (pkt->flags & BCM_TX_RELIABLE) {
            mode = BCM_CPU_TUNNEL_PACKET_RELIABLE;
        }

        return bcm_tx_cpu_tunnel(pkt, unit, 0, BCM_CPU_TUNNEL_F_PBMP, mode);
    }
#endif  /* BCM_RPC_SUPPORT */

    err_msg = "Bad flags for bcm_tx";
    /* coverity[overrun-call] */
    _ON_ERROR_GOTO(_tx_flags_check(unit, pkt), rv, error);

    err_msg = "Could not set up pkt for bcm_tx";
    /* coverity[overrun-call] */
    _ON_ERROR_GOTO(_bcm_tx_pkt_tag_setup(unit, pkt), rv, error);

#if defined (INCLUDE_RCPU) && defined(BCM_XGS3_SWITCH_SUPPORT)
    if ((SOC_UNIT_VALID(unit)) && (SOC_IS_RCPU_UNIT(unit))) {
        if (!BCM_PKT_TX_ETHER(pkt)) {
            BCM_IF_ERROR_RETURN(_bcm_xgs3_tx_pipe_bypass_header_setup(unit, pkt));
        }
        return _bcm_rcpu_tx(unit, pkt, cookie);
    }
#endif /* INCLUDE_RCPU && BCM_XGS3_SWITCH_SUPPORT */

    err_msg = "Could not allocate dv/dv info";
    dv = _tx_dv_alloc(unit, 1, pkt->blk_count + TX_EXTRA_DCB_COUNT,
                      NULL, cookie, pkt->call_back != NULL);
    if (!dv) {
        rv = BCM_E_MEMORY;
        goto error;
    }

    err_msg = "Could not setup or add pkt to DV";
    _ON_ERROR_GOTO(_tx_pkt_desc_add(unit, pkt, dv, 0), rv, error);

#ifdef BCM_XGS3_SWITCH_SUPPORT
	if ((pkt->flags & BCM_TX_NO_PAD) && (dv->dv_vcnt > 0)) {
            int bytes = 0;
            _xgs3_calculate_tx_packet_size(unit, dv, &bytes);

            if (SOC_IS_XGS3_SWITCH(unit)) {
                if ((BCM_PKT_NO_VLAN_TAG(pkt) && bytes < 60) ||
                    (!BCM_PKT_NO_VLAN_TAG(pkt) && BCM_PKT_HAS_HGHDR(pkt) && 
                     BCM_PKT_TX_FABRIC_MAPPED(pkt) && bytes < 60) ||
                    (!BCM_PKT_NO_VLAN_TAG(pkt) && !BCM_PKT_HAS_HGHDR(pkt) && 
                     !BCM_PKT_TX_FABRIC_MAPPED(pkt) && bytes < 64)) {
                    LOG_ERROR(BSL_LS_BCM_TX,
                              (BSL_META_U(unit,
                                          "bcm_tx: Discarding %s runt packet %s higig header %d\n"), 
                                          BCM_PKT_NO_VLAN_TAG(pkt) ? "untagged" : "tagged", 
                               BCM_PKT_HAS_HGHDR(pkt) ? "with" : "without", bytes));
                    err_msg = "";
                    rv = BCM_E_PARAM; 
                    goto error;
                }
            }	
        }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

    err_msg = "Could not send pkt";
    if ((dv->dv_vcnt > 0) || (SOC_IS_ARAD(unit))) {

        rv = _bcm_tx_chain_send(unit, dv, pkt->call_back != NULL); 
    } else {
        /* Call the registered callback if any */
        if (pkt->call_back != NULL) {
            (pkt->call_back)(unit, pkt, cookie);
        }
        if (dv != NULL) {
            _tx_dv_free(unit, dv);
        }
        rv = BCM_E_NONE;
    }

error:
    _PROCESS_ERROR(unit, rv, dv, err_msg);
    return rv;
}

/*
 * Function:
 *      _xgs3_async_thread
 * Purpose:
 *      Handles anync transmit for XGS3 devices
 * Parameters:
 */

#ifdef BCM_XGS3_SWITCH_SUPPORT
STATIC int
_xgs3_async_tx(int unit, bcm_pkt_t * pkt, void * cookie)
{
    _xgs3_async_queue_t * item;
    int intr_level;
    
    item = sal_alloc(sizeof(_xgs3_async_queue_t), "Async packet info");
    if (item == NULL) {
        LOG_ERROR(BSL_LS_BCM_TX,
                  (BSL_META_U(unit,
                              "Can't allocate packet info\n")));
        return BCM_E_MEMORY;
    }
    sal_memset(item, 0, sizeof(_xgs3_async_queue_t));

    item->unit = unit;
    item->pkt = pkt;
    item->cookie = cookie;
    item->next = NULL;

    intr_level = sal_splhi();

    if (_xgs3_async_head == NULL) {
        _xgs3_async_head = item;
    } else {
        _xgs3_async_tail->next = item;
    }
    _xgs3_async_tail = item;

    sal_spl(intr_level);

    sal_sem_give(_xgs3_async_tx_sem);
    return BCM_E_NONE;
}

STATIC int
_xgs3_async_queue_fetch(int * unit, bcm_pkt_t ** pkt, void ** cookie)
{
    _xgs3_async_queue_t * item;
    int intr_level;

    if (sal_sem_take(_xgs3_async_tx_sem, sal_sem_FOREVER) < 0) {
        LOG_ERROR(BSL_LS_BCM_TX,
                  (BSL_META("async fetch: Can't take async TX semaphore\n")));
        return BCM_E_RESOURCE;
    }

    intr_level = sal_splhi();
    
    item = _xgs3_async_head;
    _xgs3_async_head = item->next;
    if (_xgs3_async_head == NULL) {
        _xgs3_async_tail = NULL;
    }

    sal_spl(intr_level);

    *unit = item->unit;
    *pkt = item->pkt;
    *cookie = item->cookie;
    sal_free(item);

    return BCM_E_NONE;
}

typedef struct _xgs3_tx_cb_cookie_s {
    bcm_pkt_t *orig_pkt; 

    void *orig_cookie;
    void *cleanup_mem;
} _xgs3_tx_cb_cookie_t;

void _xgs3_tx_cb(int unit, bcm_pkt_t *pkt, void *cookie)
{
    _xgs3_tx_cb_cookie_t *mycookie = cookie;

    if (mycookie->orig_pkt->call_back != NULL) {
        (mycookie->orig_pkt->call_back)(unit, mycookie->orig_pkt, mycookie->orig_cookie);
    }

    if(mycookie->cleanup_mem != NULL) {
        sal_free(mycookie->cleanup_mem);
    }

    sal_free (cookie);

    return;
}

#ifdef BCM_SHADOW_SUPPORT
STATIC int
_shadow_tx(int unit, bcm_pkt_t *pkt, void *cookie)
{
    /* int pkt_cnt; */
    int port;

    int rv = BCM_E_NONE;
    bcm_pkt_t *packet_p;
    bcm_pkt_t packet;
    /* _xgs3_tx_cb_cookie_t *_xgs3_tx_cb_cookie; */

    packet_p = &packet;

    BCM_PBMP_ITER(pkt->tx_pbmp, port)
    {
        sal_memcpy(packet_p, pkt, sizeof(bcm_pkt_t));

        /* Set port control */
        soc_reg_field32_modify(unit, UFLOW_PORT_CONTROLr, 0, 
                               DEST_PORTf, port);

        BCM_PBMP_PORT_SET(packet_p->tx_pbmp, port);
        BCM_PBMP_PORT_SET(packet_p->tx_upbmp, port);
        BCM_PBMP_AND(packet_p->tx_upbmp, pkt->tx_upbmp);

        BCM_PBMP_CLEAR(packet_p->tx_l3pbmp);

        /* Check untag membership before translating */
        if (PBMP_MEMBER(pkt->tx_upbmp, port)) {
            packet_p->flags |= BCM_PKT_F_TX_UNTAG;
        } else {
            packet_p->flags &= (~BCM_PKT_F_TX_UNTAG);
        }

        packet_p->call_back = NULL; /* Synchronus send */

        rv = _bcm_tx(unit, packet_p, cookie);

        /* Revert back port control */
        soc_reg_field32_modify(unit, UFLOW_PORT_CONTROLr, 0, 
                               DEST_PORTf, 0);
    }
    return rv;
}
#endif /* BCM_SHADOW_SUPPORT */

STATIC int
_xgs3_tx(int unit, bcm_pkt_t *pkt, void *cookie)
{
    int pkt_cnt;
    int port;

    int rv;
    bcm_pkt_t *packets_p;
    bcm_pkt_t *packet_p;
    bcm_pkt_t **packet_pointers_p;
    bcm_pkt_t **packet_pointer_p;
    _xgs3_tx_cb_cookie_t *_xgs3_tx_cb_cookie;

    BCM_PBMP_COUNT(pkt->tx_pbmp, pkt_cnt);

    /* Create an array of packets, one per port */

    packets_p = sal_alloc(pkt_cnt * sizeof(bcm_pkt_t), "Packet copies");

    if (packets_p == NULL) {
        return BCM_E_MEMORY;
    }

    packet_pointers_p = sal_alloc(pkt_cnt * sizeof(bcm_pkt_t *),
        "Packet pointers");

    if (packet_pointers_p == NULL) {
        sal_free(packets_p);
        return BCM_E_MEMORY;
    }

    packet_p = packets_p;
    packet_pointer_p = packet_pointers_p;

    BCM_PBMP_ITER(pkt->tx_pbmp, port)
    {
        sal_memcpy(packet_p, pkt, sizeof(bcm_pkt_t));

        BCM_PBMP_PORT_SET(packet_p->tx_pbmp, port);

        BCM_PBMP_PORT_SET(packet_p->tx_upbmp, port);
        BCM_PBMP_AND(packet_p->tx_upbmp, pkt->tx_upbmp);

        BCM_PBMP_CLEAR(packet_p->tx_l3pbmp);

        packet_p->call_back = NULL;   /* We don't need individual Callbacks for each packet */

        *packet_pointer_p = packet_p;

        ++packet_p;
        ++packet_pointer_p;
    }

    if (NULL != pkt->call_back) {
        _xgs3_tx_cb_cookie = sal_alloc(sizeof(_xgs3_tx_cb_cookie_t), "Callback Cookie");
        if (_xgs3_tx_cb_cookie == NULL) {
            sal_free(packet_pointers_p);
            sal_free(packets_p); 
            return BCM_E_MEMORY;
        }
        _xgs3_tx_cb_cookie->orig_pkt = pkt; 
        _xgs3_tx_cb_cookie->orig_cookie = cookie;
        _xgs3_tx_cb_cookie->cleanup_mem = packets_p;

        rv = bcm_common_tx_array(unit, packet_pointers_p, pkt_cnt, &_xgs3_tx_cb,
                _xgs3_tx_cb_cookie);

        sal_free(packet_pointers_p);
        /* packets_p and _xgs3_tx_cb_cookie will be freed by _xgs3_tx_cb */

    } else {
        rv = bcm_common_tx_array(unit, packet_pointers_p, pkt_cnt, NULL, NULL);

        sal_free(packet_pointers_p);
        sal_free(packets_p);
    }

    return rv;
}

STATIC void
_xgs3_async_thread(void * param)
{
    int unit = 0;
    bcm_pkt_t * pkt;
    void * cookie;
    int rv;
    char thread_name[SAL_THREAD_NAME_MAX_LEN];
    sal_thread_t	thread;

    COMPILER_REFERENCE(param);

    while(1) {
        if ((rv = _xgs3_async_queue_fetch(&unit, &pkt, &cookie)) < 0) {
            LOG_ERROR(BSL_LS_BCM_TX,
                      (BSL_META_U(unit,
                                  "Async TX: fetch: %s\n"), bcm_errmsg(rv)));
            break;
        }
        if ((rv = _xgs3_tx(unit, pkt, cookie)) < 0) {
            LOG_ERROR(BSL_LS_BCM_TX,
                      (BSL_META_U(unit,
                                  "Async TX: tx: %s\n"), bcm_errmsg(rv)));
            break;
        }
    }

    thread = sal_thread_self();
    thread_name[0] = 0;
    sal_thread_name(thread, thread_name, sizeof (thread_name));
    LOG_ERROR(BSL_LS_BCM_TX,
              (BSL_META_U(unit,
                          "AbnormalThreadExit:%s\n"), thread_name));
    sal_thread_exit(0);
}
#endif

/*
 * Function:
 *      bcm_tx
 * Purpose:
 *      Wrapper for _bcm_tx to work around a lack of XGS3 support for
 *      transmitting to a bitmap.
 * Parameters:
 *      unit - transmission unit
 *      pkt - The tx packet structure
 *      cookie - Callback cookie when tx done
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      XGS3 devices cannot dispatch packets to multiple ports. To emulate this
 *      functionality, bcm_tx iterates over the requested bitmaps. The callback,
 *      if applicable, shall be called only once.
 */

int
bcm_common_tx(int unit, bcm_pkt_t *pkt, void *cookie)
{
    int rv = BCM_E_NONE;
#ifdef BCM_SHADOW_SUPPORT
    if (BCM_IS_LOCAL(unit) && SOC_IS_SHADOW(unit)) {
       /* In BCM88732 device the packets tx is handled differntly than an XGS
        * device, although there are many similarites hence most of the code
        * is reused.
        */
        _shadow_tx(unit, pkt, cookie);
    } else 
#endif
#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (BCM_IS_LOCAL(unit) && SOC_IS_XGS3_SWITCH(unit)) {
        int pkt_cnt;
        BCM_PBMP_COUNT(pkt->tx_pbmp, pkt_cnt);

        if ((pkt_cnt <= 1) || (BCM_PKT_TX_ETHER(pkt))) {
            return _bcm_tx(unit, pkt, cookie);
        } else if (pkt->call_back == NULL) {
            return _xgs3_tx(unit, pkt, cookie);
        } else {
            return _xgs3_async_tx(unit, pkt, cookie);
        }
    } else
#endif
    {
        rv = _bcm_tx(unit, pkt, cookie);
    }
    return rv;
}

/*
 * Function:
 *      bcm_tx_array
 * Purpose:
 *      Transmit an array of packets
 * Parameters:
 *      unit - transmission unit
 *      pkt - array of pointers to packets to transmit
 *      count - Number of packets in list
 *      all_done_cb - Callback function (if non-NULL) when all pkts tx'd
 *      cookie - Callback cookie.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If all_done_cb is non-NULL, the packets are sent asynchronously
 *      (the routine returns before all the pkts are sent)
 *
 *      The "packet callback" will be set according to the value
 *      in each packet's "call_back" member, so that must be initialized
 *      for all packets in the chain.
 *
 *      If any packet requires a callback, the packet-done callback is
 *      enabled for all packets in the chain.
 *
 *      This routine does not support tunnelling to a remote CPU for
 *      forwarding.
 *
 *      CURRENTLY:  The TX parameters, src mod, src port, PFM
 *      and internal priority, are determined by the first packet in
 *      the array.  This may change to break the array into subchains
 *      when differences are detected.
 */

int
bcm_common_tx_array(int unit, bcm_pkt_t **pkt, int count, bcm_pkt_cb_f all_done_cb,
             void *cookie)
{
    dv_t *dv = NULL;
    int rv = BCM_E_NONE;
    int tot_blks = 0; /* How many dcbs needed for packets */
    int i;
    char *err_msg = NULL;
    int pkt_cb = FALSE;  /* Are any pkts asking for callback? */
    volatile bcm_pkt_t *pkt_ptr;

    if (!pkt) {
        return BCM_E_PARAM;
    }

    if (!BCM_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    if (!BCM_IS_LOCAL(unit)) { /* Tunnel the packet to the remote CPU */
        LOG_ERROR(BSL_LS_BCM_TX,
                  (BSL_META_U(unit,
                              "bcm_tx_array ERROR:  Cannot tunnel\n")));
        return BCM_E_PARAM;
    }

#if defined (INCLUDE_RCPU) && defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_RCPU_UNIT(unit)) {
        for (i = 0; i < count; i++) {
            if (!pkt[i]) {
                return BCM_E_PARAM;
            }
            BCM_IF_ERROR_RETURN(_bcm_tx_pkt_tag_setup(unit, pkt[i]));
            if (!BCM_PKT_TX_ETHER(pkt[i])) {
                BCM_IF_ERROR_RETURN(_bcm_xgs3_tx_pipe_bypass_header_setup(unit, pkt[i]));
            }
        }
        return _bcm_rcpu_tx_array(unit, pkt, count, all_done_cb, cookie);
    }
#endif /* INCLUDE_RCPU && BCM_XGS3_SWITCH_SUPPORT */

    /* Count the blocks and check for per-packet callback */
    for (i = 0; i < count; i++) {
        if (!pkt[i]) {
            return BCM_E_PARAM;
        }
        tot_blks += pkt[i]->blk_count;
        if (pkt[i]->call_back) {
            pkt_cb = TRUE;
        }
    }

    err_msg = "Bad flags for bcm_tx_array";
    _ON_ERROR_GOTO(_tx_flags_check(unit, pkt[0]), rv, error);

    err_msg = "Could not set up pkt for bcm_tx_array";
    for (i = 0; i < count; i++) {
        _ON_ERROR_GOTO(_bcm_tx_pkt_tag_setup(unit, pkt[i]), rv, error);
    }

    /* Allocate the DV */
    err_msg = "Could not allocate dv/dv info";
    dv = _tx_dv_alloc(unit, count, tot_blks + count * TX_EXTRA_DCB_COUNT,
                      all_done_cb, cookie, pkt_cb);
    if (!dv) {
        rv = BCM_E_MEMORY;
        goto error;
    }

    err_msg = "Could not set up or add pkt to DV";
    for (i = 0; i < count; i++) {
        _ON_ERROR_GOTO(_tx_pkt_desc_add(unit, pkt[i], dv, i), rv, error);
    }

    err_msg = "Could not send array";
    if (dv->dv_vcnt > 0) {
        rv = _bcm_tx_chain_send(unit, dv, 0); /* all_done_cb != NULL);*/
		/* if (SOC_IS_ARAD(unit)) {
	        if (all_done_cb != NULL) {
                pkt_ptr = TX_INFO(dv)->pkt[0];
                (all_done_cb)(unit, (bcm_pkt_t *)pkt_ptr, cookie);
                
            }
		} */
    } else {
        /* Call the registered callback if any */
        if (all_done_cb != NULL) {
            pkt_ptr = TX_INFO(dv)->pkt[0];
            (all_done_cb)(unit, (bcm_pkt_t *)pkt_ptr, cookie);
        }
        if (dv != NULL) {
            _tx_dv_free(unit, dv);
        }
        rv = BCM_E_NONE;
    }
error:
    _PROCESS_ERROR(unit, rv, dv, err_msg);
    return rv;
}

/*
 * Function:
 *      bcm_tx_list
 * Purpose:
 *      Transmit a linked list of packets
 * Parameters:
 *      unit - transmission unit
 *      pkt - Pointer to linked list of packets
 *      all_done_cb - Callback function (if non-NULL) when all pkts tx'd
 *      cookie - Callback cookie.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If callback is non-NULL, the packets are sent asynchronously
 *      (the routine returns before all the pkts are sent)
 *
 *      The "packet callback" will be set according to the value
 *      in each packet's "call_back" member, so that must be initialized
 *      for all packets in the chain.
 *
 *      The "next" member of the packet is used for the linked list.
 *      CAREFUL:  The internal _next member is not used for this.
 *
 *      This routine does not support tunnelling to a remote CPU for
 *      forwarding.
 *
 *      The TX parameters, src mod, src port, PFM and internal priority,
 *      are currently determined by the first packet in the list.
 */

int
bcm_common_tx_list(int unit, bcm_pkt_t *pkt, bcm_pkt_cb_f all_done_cb, void *cookie)
{
    dv_t *dv = NULL;
    int rv = BCM_E_NONE;
    int tot_blks = 0; /* How many dcbs needed for packets */
    int count;
    bcm_pkt_t *cur_pkt;
    char *err_msg = NULL;
    int pkt_cb = FALSE;  /* Are any pkts asking for callback? */
    int i = 0;
    volatile bcm_pkt_t *pkt_ptr;

    if (!pkt) {
        return BCM_E_PARAM;
    }

    if (!BCM_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    if (!BCM_IS_LOCAL(unit)) { /* Tunnel the packet to the remote CPU */
        LOG_ERROR(BSL_LS_BCM_TX,
                  (BSL_META_U(unit,
                              "bcm_tx_list ERROR:  Cannot tunnel\n")));
        return BCM_E_PARAM;
    }

#if defined (INCLUDE_RCPU) && defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_RCPU_UNIT(unit)) {
        for (cur_pkt = pkt; cur_pkt; cur_pkt = cur_pkt->next) {
            BCM_IF_ERROR_RETURN(_bcm_tx_pkt_tag_setup(unit, cur_pkt));
            if (!BCM_PKT_TX_ETHER(cur_pkt)) {
                BCM_IF_ERROR_RETURN(_bcm_xgs3_tx_pipe_bypass_header_setup(unit, cur_pkt));
            }
        }
        return _bcm_rcpu_tx_list(unit, pkt, all_done_cb, cookie);
    }
#endif /* INCLUDE_RCPU && BCM_XGS3_SWITCH_SUPPORT */

    /* Count the blocks and check for per-packet callback */
    count = 0;
    for (cur_pkt = pkt; cur_pkt; cur_pkt = cur_pkt->next) {
        tot_blks += cur_pkt->blk_count;
        if (cur_pkt->call_back) {
            pkt_cb = TRUE;
        }
        ++count;
    }

    err_msg = "Bad flags for bcm_tx_list";
    _ON_ERROR_GOTO(_tx_flags_check(unit, pkt), rv, error);

    err_msg = "Could not set up pkt for bcm_tx_list";
    for (cur_pkt = pkt; cur_pkt != NULL; cur_pkt = cur_pkt->next) {
        _ON_ERROR_GOTO(_bcm_tx_pkt_tag_setup(unit, cur_pkt), rv, error);
    }

    /* Allocate the DV */
    err_msg = "Could not allocate dv/dv info";
    dv =_tx_dv_alloc(unit, count, tot_blks + count * TX_EXTRA_DCB_COUNT,
                     all_done_cb, cookie, pkt_cb);
    if (!dv) {
        rv = BCM_E_MEMORY;
        goto error;
    }

    err_msg = "Could not set up or add pkt to DV";
    for (i = 0, cur_pkt = pkt; cur_pkt; cur_pkt = cur_pkt->next, i++) {
        _ON_ERROR_GOTO(_tx_pkt_desc_add(unit, cur_pkt, dv, i), rv, error);
    }

    err_msg = "Could not send list";
    if (dv->dv_vcnt > 0) {
        rv = _bcm_tx_chain_send(unit, dv, all_done_cb != NULL);
        #if 0
		if (SOC_IS_ARAD(unit)) {
	        if (all_done_cb != NULL) {
                pkt_ptr = TX_INFO(dv)->pkt[0];
                (all_done_cb)(unit, (bcm_pkt_t *)pkt_ptr, cookie);
             }
		}
        #endif
    } else {
        /* Call the registered callback if any */
        if (all_done_cb != NULL) {
            pkt_ptr = TX_INFO(dv)->pkt[0];
            if (pkt_ptr == NULL) {
                LOG_VERBOSE(BSL_LS_BCM_TX,
                            (BSL_META_U(unit,
                                        "%s: NULL pkt\n"), FUNCTION_NAME()));
                pkt_ptr = pkt;
            } 
            (all_done_cb)(unit, (bcm_pkt_t *)pkt_ptr, cookie);
        }
        if (dv != NULL) {
            _tx_dv_free(unit, dv);
        }
        rv = BCM_E_NONE;
    }
error:
    _PROCESS_ERROR(unit, rv, dv, err_msg);
    return rv;
}

/*
 * Function:
 *      _bcm_tx_chain_send
 * Purpose:
 *      Send out a chain of one or more packets
 */

STATIC int
_bcm_tx_chain_send(int unit, dv_t *dv, int async)
{
    ++_tx_chain_send;

#if defined(BROADCOM_DEBUG)
    if (!bsl_check(bslLayerSoc, bslSourceDma, bslSeverityNormal, unit) &&
        bsl_check(bslLayerSoc, bslSourceTx, bslSeverityVerbose, unit)) {
        int        i, j, len, dv_cnt = 0;
        dcb_t     *dcb;
        uint8     *addr;
        char       linebuf[128], *s;
        dv_t       *dv_i;

        for (dv_i = dv; dv_i != NULL; dv_i = dv_i->dv_chain, dv_cnt++) {
            for (i = 0; i < dv_i->dv_vcnt; i++) {
                soc_dma_dump_dv_dcb(unit, "Dma descr: ", dv, i);
                dcb = SOC_DCB_IDX2PTR(unit, dv_i->dv_dcb, i);
                addr = (uint8*)SOC_DCB_ADDR_GET(unit, dcb);
                len = SOC_DCB_REQCOUNT_GET(unit, dcb);
                for (; i < len; i += 16) {
                    s = linebuf;
                    sal_sprintf(s, "TXDV %d data[%04x]: ", dv_cnt, i);
                    while (*s != 0) s++;
                    for (j = i; j < i + 16 && j < len; j++) {
                        sal_sprintf(s, "%02x%s", addr[j], j & 1 ? " " : "");
                        while (*s != 0) s++;
                    }
                    LOG_CLI((BSL_META_U(unit,
                                        "%s\n"), linebuf));
                }
            }
        }
    }
#endif  /* BROADCOM_DEBUG */

    if (async) { /* Send packet(s) asynchronously */
        LOG_INFO(BSL_LS_BCM_TX,
                 (BSL_META_U(unit,
                             "bcm_tx: async send\n")));
        dv->dv_flags |= DV_F_NOTIFY_CHN;
        SOC_IF_ERROR_RETURN(soc_dma_start(unit, -1, dv));
    } else { /* Send sync */
        LOG_INFO(BSL_LS_BCM_TX,
                 (BSL_META_U(unit,
                             "bcm_tx: sync send\n")));
        SOC_IF_ERROR_RETURN(soc_dma_wait(unit, dv));
        LOG_INFO(BSL_LS_BCM_TX,
                 (BSL_META_U(unit,
                             "bcm_tx: Sent synchronously.\n")));
        _bcm_tx_chain_done_cb(unit, dv);
    }

    return BCM_E_NONE;
}

/****************************************************************
 *
 * Map a destination MAC address to port bitmaps.  Uses
 * the dest_mac passed as a parameter.  Sets the
 * bitmaps in the pkt structure.
 *
 ****************************************************************/

/*
 * Function:
 *      bcm_tx_pkt_l2_map
 * Purpose:
 *      Resolve the packet's L2 destination and update the necessary
 *      fields of the packet.
 * Parameters:
 *      unit - Transmit unit
 *      pkt - Pointer to pkt being transmitted
 *      dest_mac - use for L2 lookup
 *      vid - use for L2 lookup
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Deprecated, not implemented.
 */

int
bcm_common_tx_pkt_l2_map(int unit, bcm_pkt_t *pkt, bcm_mac_t dest_mac, int vid)
{
    return BCM_E_UNAVAIL;
}

/****************************************************************
 *
 * Functions to setup DVs and DCBs.
 *    _tx_dv_alloc:         Allocate and init a TX DV
 *    _tx_pkt_desc_add:     Add all the descriptors for a packet to a DV
 *
 * Minor functions:
 *    _dcb_flags_get:       Calculate the DCB flags.
 *    _get_mac_vlan_ptrs:   Determine DMA pointers for src_mac and
 *                          vlan.  This is in case the src_mac is not
 *                          in the same pkt block as the dest_mac; or
 *                          the packet is untagged.  Also sets the
 *                          byte and block offsets for data that follows.
 *
 ****************************************************************/



/*
 * Function:
 *      _tx_dv_alloc
 * Purpose:
 *      Allocate a DV, a dv_info structure and initialize.
 * Parameters:
 *      unit - Strata unit number for allocation
 *      pkt_count - Number of packets to TX
 *      dcb_count - The number of DCBs to provide
 *      chain_done_cb - Chain done callback function
 *      cookie - User cookie used for chain done and packet callbacks.
 *      per_pkt_cb - Do we need to callback on each packet complete?
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Always sets the dv_done_chain callback; sets packet done
 *      call back according to per_pkt_cb parameter.
 */

STATIC dv_t *
_tx_dv_alloc(int unit, int pkt_count, int dcb_count,
             bcm_pkt_cb_f chain_done_cb, void *cookie, int per_pkt_cb)
{
    dv_t *dv;
    tx_dv_info_t *dv_info;
    volatile bcm_pkt_t **pkt;
    int packet_count_limit = SOC_DV_PKTS_MAX;

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        int port_count;
        /* On XGS3 we allow a larger DV chain to allow a single TX to 
           all ports */
        BCM_PBMP_COUNT(PBMP_PORT_ALL(unit), port_count);
        if (packet_count_limit < port_count) {
            packet_count_limit = port_count;
        }
    }
#endif 

    if (pkt_count > packet_count_limit) {
        LOG_ERROR(BSL_LS_BCM_TX,
                  (BSL_META_U(unit,
                              "TX array:  Cannot TX more than %d pkts. "
                              "Attempting %d.\n"), packet_count_limit, pkt_count));
        return NULL;
    }
    if ((dv = soc_dma_dv_alloc_by_port(unit, DV_TX, dcb_count, 
                                       pkt_count)) == NULL) {
        return NULL;
    }
    /*not a queued dv */
    if (TX_INFO(dv) == NULL) {    	
	    if ((dv_info = soc_at_alloc(unit, sizeof(tx_dv_info_t), "tx_dv")) == NULL) {
	        soc_dma_dv_free(unit, dv);
	        return NULL;
	    }	    
	    pkt = soc_at_alloc(unit, (sizeof(bcm_pkt_t *) * packet_count_limit), "tx_dv pkt");
	    if (pkt == NULL) {
	        soc_dma_dv_free(unit, dv);
	        soc_at_free(unit, dv_info);
	        return NULL;
	    }
	} else {
		dv_info = TX_INFO(dv);
		pkt = dv_info->pkt;
	}
	TX_INFO_SET(dv, dv_info);
	sal_memset(TX_INFO(dv), 0, sizeof(tx_dv_info_t));
	TX_INFO(dv)->pkt = pkt;
	sal_memset(TX_INFO(dv)->pkt, 0, (sizeof(bcm_pkt_t *) * packet_count_limit));
	TX_INFO(dv)->cookie = cookie; 
	TX_INFO(dv)->chain_done_cb = chain_done_cb;
	dv->dv_done_chain = _bcm_tx_chain_done_cb;
	if (per_pkt_cb) {
	    dv->dv_done_packet = _bcm_tx_packet_done_cb;
	} 
    return dv;
}

/*
 * Function:
 *      _tx_dv_free
 * Purpose:
 *      Free a DV and associated dv_info structure
 * Parameters:
 *      unit - Strata unit number for allocation
 *      dv - The DV to free
 * Returns:
 *      None
 */

STATIC void
_tx_dv_free(int unit, dv_t *dv)
{   
	tx_dv_info_t *dv_info;
	
    if (dv) {
    	dv_info = TX_INFO(dv);  	
    	soc_dma_dv_free(unit, dv);
    	/* for a none queued dv to release dv_info */
    	if (!soc_dma_dv_valid(dv)) {
    		if (dv_info) {
	            if (dv_info->pkt) {
	                soc_at_free(unit, dv_info->pkt);
	            }
	            soc_at_free(unit, dv_info);
	        }
	    }
        
    }
}

void 
tx_dv_free_cb(int unit, dv_t *dv) 
{ 
    tx_dv_info_t *dv_info;
	
    if (dv) {
    	if (dv->dv_done_chain == _bcm_tx_chain_done_cb) {
	    	dv_info = TX_INFO(dv);  	
	        if (dv_info) {
		        if (dv_info->pkt) {
		            soc_at_free(unit, dv_info->pkt);
		        }
		        soc_at_free(unit, dv_info);
		    }
		}
	}      
}

/* Set up the DV for bcm_tx;
 * Assert:  unit is local
 */
STATIC INLINE uint32
_dcb_flags_get(int unit, bcm_pkt_t *pkt, dv_t *dv)
{
    uint32 dcb_flags = 0;

    COMPILER_REFERENCE(dv);
    if (SOC_IS_XGS3_SWITCH(unit)) {
        /*
         * XGS3: HG bit in descriptor needs to be set
         *   1.     Raw mode HG/Ethernet TX (Unmodified Packet steering).
         *   2.     Fabric mapped TX
         * For fully mapped Ethernet transmit this should not be set.
         */
        if ((! BCM_PKT_TX_ETHER(pkt)) || /* Raw mode TX */
             BCM_TX_PKT_PROP_ANY_TST(pkt) || /* Fabric mapped TX */
             BCM_PKT_TX_HG_READY(pkt)) {
            SOC_DMA_HG_SET(dcb_flags, 1);
        }
        if (pkt->flags & BCM_TX_PURGE) {
            SOC_DMA_PURGE_SET(dcb_flags, 1);
        }

        return dcb_flags;
    } 
#if defined(BCM_SBX_CMIC_SUPPORT)
    else if (SOC_IS_SIRIUS(unit)) {
        /*
         * SIRIUS: HG bit in descriptor needs to be set
         *   1.     Raw mode HG/Ethernet TX (Unmodified Packet steering).
         *   2.     Fabric mapped TX
         * For fully mapped Ethernet transmit this should not be set.
         */
        if ((! BCM_PKT_TX_ETHER(pkt)) || /* Raw mode TX */
             BCM_TX_PKT_PROP_ANY_TST(pkt) || /* Fabric mapped TX */
             BCM_PKT_TX_HG_READY(pkt)) {
            SOC_DMA_HG_SET(dcb_flags, 1);
        }
        if (pkt->flags & BCM_TX_PURGE) {
            SOC_DMA_PURGE_SET(dcb_flags, 1);
        }
    }
#endif
#ifdef BCM_ARAD_SUPPORT
    else if (SOC_IS_ARAD(unit)) {
        if (pkt->flags & BCM_TX_PURGE) {
            SOC_DMA_PURGE_SET(dcb_flags, 1);
        }
        if (pkt->flags & BCM_PKT_F_HGHDR) {
            SOC_DMA_HG_SET(dcb_flags, 1);
        }
    }
#endif

    dcb_flags |= SOC_DMA_COS(pkt->cos);
    /* This level does not handle CRC append, but marks for HW to generate */
    if (pkt->flags & (BCM_TX_CRC_APPEND | BCM_TX_CRC_REGEN)) {
        dcb_flags |= SOC_DMA_CRC_REGEN;
    }

    return dcb_flags;
}

/*
 * Based on packet settings, get pointers to vlan, and src mac
 * Update the "current" block and byte offset
 */

STATIC INLINE void
_get_mac_vlan_ptrs(dv_t *dv, bcm_pkt_t *pkt, uint8 **src_mac,
                   uint8 **vlan_ptr, int *block_offset, int *byte_offset,
                   int pkt_idx)
{
    /* Assume everything is in block 0 */
    *src_mac = &pkt->pkt_data[0].data[sizeof(bcm_mac_t)];
    *block_offset = 0;

    if (BCM_PKT_NO_VLAN_TAG(pkt)) { /* Get VLAN from _vtag pkt member */
        *byte_offset = 2 * sizeof(bcm_mac_t);
        sal_memcpy(SOC_DV_VLAN_TAG(dv, pkt_idx), pkt->_vtag, sizeof(uint32));
        *vlan_ptr = SOC_DV_VLAN_TAG(dv, pkt_idx);

        if (pkt->pkt_data[0].len < 2 * sizeof(bcm_mac_t)) {
            /* Src MAC in block 1 */
            *src_mac = pkt->pkt_data[1].data;
            *block_offset = 1;
            *byte_offset = sizeof(bcm_mac_t);
        }
    } else { /* Packet has VLAN tag */
        *byte_offset = 2 * sizeof(bcm_mac_t) + sizeof(uint32);
        *vlan_ptr = &pkt->pkt_data[0].data[2 * sizeof(bcm_mac_t)];

        if (pkt->pkt_data[0].len < 2 * sizeof(bcm_mac_t)) {
            /* Src MAC in block 1; assume VLAN there too at first */
            *src_mac = pkt->pkt_data[1].data;
            *vlan_ptr = &pkt->pkt_data[1].data[sizeof(bcm_mac_t)];
            *block_offset = 1;
            *byte_offset = sizeof(bcm_mac_t) + sizeof(uint32);
            if (pkt->pkt_data[1].len < sizeof(bcm_mac_t) + sizeof(uint32)) {
                /* Oops, VLAN in block 2 */
                *vlan_ptr = pkt->pkt_data[2].data;
                *block_offset = 2;
                *byte_offset = sizeof(uint32);
            }
        } else if (pkt->pkt_data[0].len <
                   2 * sizeof(bcm_mac_t) + sizeof(uint32)) {
            /* VLAN in block 2 */
            *block_offset = 1;
            *byte_offset = sizeof(uint32);
            *vlan_ptr = pkt->pkt_data[1].data;
        }
    }
}

/* Set up the SOC layer TX packet according to flags and BCM pkt */

/* PFM is two bits */
#define PFM_MASK 0x3

STATIC INLINE void
_soc_tx_pkt_setup(int unit, bcm_pkt_t *pkt, soc_tx_param_t *tx_param)
{
    /* Decide which source mod/port to use */
    tx_param->src_mod  = (pkt->flags & BCM_TX_SRC_MOD) ? pkt->src_mod :
        SOC_DEFAULT_DMA_SRCMOD_GET(unit);
    tx_param->src_port = (pkt->flags & BCM_TX_SRC_PORT) ? pkt->src_port :
        SOC_DEFAULT_DMA_SRCPORT_GET(unit);
    tx_param->pfm = (pkt->flags & BCM_TX_PFM) ? pkt->pfm :
        SOC_DEFAULT_DMA_PFM_GET(unit);

    if ((tx_param->pfm & (~PFM_MASK)) != 0) {
        LOG_WARN(BSL_LS_BCM_TX,
                 (BSL_META_U(unit,
                             "bcm_tx: PFM too big, truncating\n")));
        tx_param->pfm &= PFM_MASK;
    }

    /*
     * the default ought to be BCM_PKT_VLAN_PRI(pkt),
     * but we need backwards compatibility with 5690
     */
    tx_param->prio_int = (pkt->flags & BCM_TX_PRIO_INT) ? pkt->prio_int :
               pkt->cos;

    tx_param->cos = pkt->cos;
}


/*
 * Function:
 *      _tx_pkt_desc_add
 * Purpose:
 *      Add all descriptors to a DV for a given packet.
 * Parameters:
 *      unit    - Strataswitch device ID
 *      pkt     - Pointer to packet
 *      dv      - DCB vector to update
 *      pkt_idx - Index of packet.
 *                Index value is 0, except for arrays or link lists.
 *                For arrays or link lists, the value must be the
 *                the index number of the packet.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Uses tx info pkt_count member to get packet index; then advances
 *      the pkt_count member.
 *
 *      See sdk/doc/pkt.txt for info about restrictions on MACs,
 *      and the VLAN not crossing block boundaries.
 *
 *      Assert:  unit is local
 */

STATIC int
_tx_pkt_desc_add(int unit, bcm_pkt_t *pkt, dv_t *dv, int pkt_idx)
{
    int byte_offset = 0;
    int pkt_len = 0;  /* Length calculation for byte padding */
    int tmp_len;
    int min_len = ENET_MIN_PKT_SIZE; /* Min length pkt before padded */
    uint8 *vlan_ptr = NULL;
    uint8 *src_mac;
    int block_offset = 0;
    int i;
    uint32 dcb_flags = 0;
    uint32 *pkt_hg_hdr = NULL;
    int clear_crc = FALSE;
    int crc_block_offset = 0;
    bcm_pbmp_t tx_pbmp, tx_upbmp, tx_l3pbmp;
    soc_persist_t *sop = SOC_PERSIST(unit);

    COMPILER_REFERENCE(pkt_idx); /* May not be ref'd depending on defines */

    /* Clear private UNTAG flag */
#ifdef BCM_SHADOW_SUPPORT
    /* BCM88732 device doesn't introduce any headers */
    if (!SOC_IS_SHADOW(unit)) {
#endif
        pkt->flags &= (~BCM_PKT_F_TX_UNTAG);
#ifdef BCM_SHADOW_SUPPORT
    }
#endif


    /* Calculate the DCB flags for this packet. */
    dcb_flags = _dcb_flags_get(unit, pkt, dv);

#if defined(BROADCOM_DEBUG)
    if (pkt->cos < BCM_COS_COUNT) { /* COS is unsigned */
        bcm_tx_pkt_count[pkt->cos]++;
    } else {
        bcm_tx_pkt_count_bad_cos++;
    }
#endif  /* BROADCOM_DEBUG */

    _soc_tx_pkt_setup(unit, pkt, &dv->tx_param);

    /* If the CRC is being regenerated and the transmit device
     * is a fabric, zero out the last four bytes by subtracting
     * four bytes from the end and appending four byte zero'd out 
     * tx descriptor.
     */
    if (SOC_IS_HERCULES1(unit) &&
            (pkt->flags & BCM_TX_CRC_REGEN) &&
            (!(pkt->flags & BCM_TX_CRC_ALLOC))) {

        /* Assumes the CRC is in the last block containing data */
        for (i = pkt->blk_count - 1; i > 0; i--) {
            if (pkt->pkt_data[i].len > 0) {
                break;
            }
        }
        byte_offset = pkt->pkt_data[i].len - ENET_FCS_SIZE;
        if (byte_offset < 0) {
            LOG_WARN(BSL_LS_BCM_TX,
                     (BSL_META_U(unit,
                                 "TX CRC clear: CRC not contiguous or runt pkt\n")));
        } else {
            clear_crc = TRUE;
            crc_block_offset = i;
        }
    }

    BCM_PBMP_ASSIGN(tx_pbmp, pkt->tx_pbmp);
    BCM_PBMP_ASSIGN(tx_upbmp, pkt->tx_upbmp);
    BCM_PBMP_ASSIGN(tx_l3pbmp, pkt->tx_l3pbmp);
    BCM_API_XLATE_PORT_PBMP_A2P(unit, &tx_pbmp);
    BCM_API_XLATE_PORT_PBMP_A2P(unit, &tx_upbmp);
    BCM_API_XLATE_PORT_PBMP_A2P(unit, &tx_l3pbmp);
#if defined(BCM_SBX_CMIC_SUPPORT)
    if (!SOC_IS_SIRIUS(unit)) {
#endif
    if (!(pkt->flags & BCM_TX_LINKDOWN_TRANSMIT)) {
        BCM_PBMP_AND(tx_pbmp, sop->lc_pbm_link);
    }
#if defined(BCM_SBX_CMIC_SUPPORT)
    }
#endif
    if (!SOC_IS_ARAD(unit)) {
        if (BCM_PBMP_IS_NULL(tx_pbmp) && !BCM_PBMP_IS_NULL(pkt->tx_pbmp)) {
            return BCM_E_NONE;
        }
    }

    /*
     * XGS3 devices do not take TXPBM/TXUBM.
     * This is emulated below using PBS TX mode.
     * Only used on 5675 now.
     */
#if defined(BCM_XGS12_SUPPORT)
    if (soc_feature(unit, soc_feature_tx_fast_path)) {
        /* Just set up the packet as is and return */
        if (pkt->flags & BCM_TX_FAST_PATH) {
            for (i = 0; i < pkt->blk_count; i++) {
                SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
                    (sal_vaddr_t) pkt->pkt_data[i].data, pkt->pkt_data[i].len,
                    tx_pbmp, tx_upbmp, tx_l3pbmp,
                    dcb_flags, (uint32 *)BCM_PKT_HG_HDR(pkt)));
            }

            /* Mark the end of the packet */
            soc_dma_desc_end_packet(dv);

            return BCM_E_NONE;
        }
    }
#endif /* BCM_XGS12_SUPPORT */

    if (pkt->pkt_data[0].len < sizeof(bcm_mac_t)) {
         LOG_INFO(BSL_LS_BCM_TX,
                  (BSL_META_U(unit,
                              "_tx_pkt_desc_add: pkt->pkt_data[0].len < sizeof(bcm_mac_t) exit ")));
        return BCM_E_PARAM;
    }

    /* Get pointers to srcmac and vlan; check if bad block count */
    _get_mac_vlan_ptrs(dv, pkt, &src_mac, &vlan_ptr, &block_offset,
                       &byte_offset, pkt_idx);
    if (block_offset >= pkt->blk_count) {
        LOG_INFO(BSL_LS_BCM_TX,
                 (BSL_META_U(unit,
                             "_tx_pkt_desc_add: block_offset >= pkt->blk_count exit ")));
        return BCM_E_PARAM;
    }

    if (byte_offset >= pkt->pkt_data[block_offset].len) {
        byte_offset = 0;
        block_offset++;
    }
    /* Set up pointer to the packet in TX info structure. */
    TX_INFO_PKT_ADD(dv, pkt);

#ifdef BCM_XGS3_SWITCH_SUPPORT
    /*
     *  XGS3: Decide whether to put Higig header or PB (Pipe Bypass)
     *  header in the TX descriptor
     *  1.      Fabric mapped mode (HG header in descriptor)
     *  2.      Raw Ethernet packet steered mode (PB header in descriptor)
     */
    if (!SOC_IS_SHADOW(unit) &&
        SOC_IS_XGS3_SWITCH(unit) &&
        (!BCM_PKT_TX_ETHER(pkt))) {
        /*
         *  Raw packet steered mode (PB header in descriptor)
         */
        BCM_IF_ERROR_RETURN(_bcm_xgs3_tx_pipe_bypass_header_setup(unit, pkt));

        pkt_hg_hdr = (uint32 *)BCM_PKT_PB_HDR(pkt);
    } else {
        pkt_hg_hdr = (uint32 *)BCM_PKT_HG_HDR(pkt);
        if (PBS_MH_START_IS_SET(pkt_hg_hdr)) {
            /* A PBSMH tag was supplied.
             * Clear the HG flags so the VLAN tag isn't stripped off.
             */
            BCM_PKT_HGHDR_CLR(pkt);
            (pkt)->flags &= (~BCM_TX_HG_READY);
        }
    }
#else
    pkt_hg_hdr = NULL;
#endif

#ifdef BCM_SHADOW_SUPPORT
    if(SOC_IS_SHADOW(unit)) {
        /* No Module header for BCM88732 device */
        pkt_hg_hdr = NULL;
        SOC_DMA_HG_SET(dcb_flags, 0);
    }
#endif

#ifdef  BCM_XGS_SUPPORT
    /*
     * BCM_PKT_F_HGHDR flag indicates higig header is part of
     * packet data stream.
     */

#ifdef BCM_SBX_CMIC_SUPPORT
    if (BCM_PKT_HAS_HGHDR(pkt) || 
	(SOC_IS_SIRIUS(unit) && BCM_PKT_HAS_SBX_RH(pkt))) 
#else
    if (BCM_PKT_HAS_HGHDR(pkt)
#ifdef BCM_ARAD_SUPPORT	 
	&& (!SOC_IS_ARAD(unit))
#endif	
	) 
#endif
    {
        uint8 *tmp_pkt_hg_hdr = BCM_PKT_HG_HDR(pkt);
        int hg_hdr_len = SOC_HIGIG_HDR_SIZE;
        /*
         *      XGS3: Raw Higig packet steered mode.
         *            Make higig header part of packet stream and PB
         *            header part of descriptor
         */
#ifdef BCM_HIGIG2_SUPPORT
        if (tmp_pkt_hg_hdr[0] == SOC_HIGIG2_START) {
            hg_hdr_len = SOC_HIGIG2_HDR_SIZE;
        }
#endif /* BCM_HIGIG2_SUPPORT */
#if defined(BCM_SBX_CMIC_SUPPORT)
	if (SOC_IS_SIRIUS(unit) && BCM_PKT_HAS_SBX_RH(pkt)) {
	    /* The Higig header here is actually the SBX route header */
  	    sal_memcpy(pkt->_higig, pkt->_sbx_rh, pkt->_sbx_hdr_len);
	    tmp_pkt_hg_hdr = pkt->_sbx_rh;
	    hg_hdr_len = pkt->_sbx_hdr_len;
	    SOC_DMA_HG_SET(dcb_flags, 1);
	}
#endif
        sal_memcpy(SOC_DV_HG_HDR(dv, pkt_idx), tmp_pkt_hg_hdr, hg_hdr_len);
#if defined(BCM_SBX_CMIC_SUPPORT)
	{
	    uint8 *tmp_pkt_pb_hdr = BCM_PKT_PB_HDR(pkt);
	    if (tmp_pkt_pb_hdr[0] != 0) {
#endif
		SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
		    (sal_vaddr_t) SOC_DV_HG_HDR(dv, pkt_idx), hg_hdr_len,
                    tx_pbmp, tx_upbmp, tx_l3pbmp,
		    dcb_flags, (uint32 *)BCM_PKT_PB_HDR(pkt)));
#if defined(BCM_SBX_CMIC_SUPPORT)
	    }
	}
#endif
    }
#endif  /* BCM_XGS_SUPPORT */

#ifdef BCM_ARAD_SUPPORT
    if (SOC_IS_ARAD(unit)) {
        pkt_hg_hdr = (uint32 *)((pkt)->_dpp_hdr);
    }
#endif
    /* Dest mac */
    SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
        (sal_vaddr_t) pkt->pkt_data[0].data, sizeof(bcm_mac_t),
        tx_pbmp, tx_upbmp, tx_l3pbmp,
        dcb_flags, pkt_hg_hdr));
    pkt_len = ENET_MAC_SIZE;
     
    /* Source mac */
    SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
        (sal_vaddr_t) src_mac, sizeof(bcm_mac_t),
        tx_pbmp, tx_upbmp, tx_l3pbmp,
        dcb_flags, pkt_hg_hdr));

    pkt_len += ENET_MAC_SIZE;

    /* VLAN tag */
    /* No VLAN tag for Fabric mapped Higig TX as well */
    if ((!BCM_PKT_HAS_HGHDR(pkt)
#ifdef BCM_ARAD_SUPPORT	 
	 ||  (BCM_PKT_HAS_HGHDR(pkt) && SOC_IS_ARAD(unit))
#endif
	 )&&
        !(pkt->flags & BCM_PKT_F_TX_UNTAG) &&
        !BCM_PKT_TX_FABRIC_MAPPED(pkt)) { /* No HG Hdr, so add VLAN tag */
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
            (sal_vaddr_t) vlan_ptr, sizeof(uint32),
            tx_pbmp, tx_upbmp, tx_l3pbmp,
            dcb_flags, pkt_hg_hdr));
    }
#if defined(BCM_SBX_CMIC_SUPPORT)
    else if ((SOC_IS_SIRIUS(unit)) && (vlan_ptr != NULL)) {
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
            (sal_vaddr_t) vlan_ptr, sizeof(uint32),
            pkt->tx_pbmp, pkt->tx_upbmp, pkt->tx_l3pbmp,
            dcb_flags, pkt_hg_hdr));
	pkt_len += sizeof(uint32);
    }
#endif
#ifdef BCM_HIGIG2_SUPPORT
    else if (pkt->stk_flags & BCM_PKT_STK_F_TX_TAG) {
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
            (sal_vaddr_t) vlan_ptr, sizeof(uint32),
            tx_pbmp, tx_upbmp, tx_l3pbmp,
            dcb_flags, pkt_hg_hdr));
    }
#endif /* BCM_HIGIG2_SUPPORT */

    /* SL TAG */
    if (pkt->flags & BCM_PKT_F_SLTAG) {
        sal_memcpy(SOC_DV_SL_TAG(dv, pkt_idx), pkt->_sltag, sizeof(uint32));
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
            (sal_vaddr_t)SOC_DV_SL_TAG(dv, pkt_idx), sizeof(uint32),
            tx_pbmp, tx_upbmp, tx_l3pbmp,
            dcb_flags, pkt_hg_hdr));
    }

    /*
     * If byte offset indicates we're not at the end of a packet's data
     * block, add a DCB for the rest of the current block.
     */
    if (byte_offset != 0) {
        tmp_len = pkt->pkt_data[block_offset].len - byte_offset;
        if (clear_crc && (block_offset == crc_block_offset)) {
            tmp_len -= ENET_FCS_SIZE;
        }
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
            (sal_vaddr_t) &pkt->pkt_data[block_offset].data[byte_offset],
            tmp_len, tx_pbmp, tx_upbmp, tx_l3pbmp,
            dcb_flags, pkt_hg_hdr));
        block_offset++;
        pkt_len += tmp_len;
    }

    /* Add DCBs for the remainder of the blocks. */
    for (i = block_offset; i < pkt->blk_count; i++) {
        tmp_len = pkt->pkt_data[i].len;
        if (clear_crc && (i == crc_block_offset)) {
            tmp_len -= ENET_FCS_SIZE;
        }
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
            (sal_vaddr_t) pkt->pkt_data[i].data,
            tmp_len, tx_pbmp, tx_upbmp, tx_l3pbmp,
            dcb_flags, pkt_hg_hdr));
        pkt_len += tmp_len;
    }

    /* If CRC allocated, adjust min length */
    if ((pkt->flags & BCM_TX_CRC_ALLOC) || clear_crc) {
        min_len = ENET_MIN_PKT_SIZE - ENET_FCS_SIZE;
    }

    /* Pad runt packets */
    if ((pkt_len < min_len) && !(pkt->flags & BCM_TX_NO_PAD)) {
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
               (sal_vaddr_t) _pkt_pad_ptr, min_len - pkt_len,
               tx_pbmp, tx_upbmp, tx_l3pbmp,
               dcb_flags, pkt_hg_hdr));
    }

    /* If "CRC allocate" set (includes append), add static ptr to it */
    if ((pkt->flags & BCM_TX_CRC_ALLOC) || clear_crc) {
        SOC_IF_ERROR_RETURN(SOC_DCB_ADDTX(unit, dv,
               (sal_vaddr_t) _null_crc_ptr, sizeof(uint32),
               tx_pbmp, tx_upbmp, tx_l3pbmp,
               dcb_flags, pkt_hg_hdr));
    }

    /* Mark the end of the packet */
    soc_dma_desc_end_packet(dv);

    return BCM_E_NONE;
}


/****************************************************************
 *
 * Functions to handle callbacks:
 *
 *   _bcm_tx_callback_thread: The non-interrupt thread that manages
 *                            packet done completions.
 *   _bcm_tx_chain_done_cb:   The interrupt handler callback for chain
 *                            done.  Adds dv-s to pending list.
 *   _bcm_tx_chain_done:      Makes user level call back and
 *                            frees the DV.
 *   _bcm_tx_packet_done_cb:  Interrupt handler for packet done.
 *                            Adds pkt to callback list if needed.
 *   _bcm_tx_packet_done:     Makes user level call back.
 ****************************************************************/


/*
 * Function:
 *      _bcm_tx_callback_thread
 * Purpose:
 *      Non-interrupt tx callback context
 * Parameters:
 *      param - ignored
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Currently assumes that the packet done interrupt will
 *      be handled before (or at same time) as the corresponding
 *      chain done interrupt.  This only matters when there's
 *      a per-packet callback since that refers to the cookie
 *      in the dv.
 */

STATIC void
_bcm_tx_callback_thread(void *param)
{
    int spl;
    dv_t *dv_list, *dv, *dv_next;
    bcm_pkt_t *pkt_list, *pkt, *pkt_next;

    COMPILER_REFERENCE(param);

    while (1) {
        if (sal_sem_take(tx_cb_sem, sal_sem_FOREVER) < 0) {
            LOG_ERROR(BSL_LS_BCM_TX,
                      (BSL_META("TX callback thread error\n")));
            break;
        }

        TX_LOCK(spl); /* Grab the lists from the interrupt handler */
        dv_list = (dv_t *)dv_pend_first;
        dv_pend_first = dv_pend_last = NULL;
        pkt_list = (bcm_pkt_t *)pkt_pend_first;
        pkt_pend_first = pkt_pend_last = NULL;
        TX_UNLOCK(spl);

        /*
         * Handle the per pkt callbacks first (which use DVs),
         * then the DVs.  See notes above
         */
        pkt = pkt_list;
        while (pkt) {
            pkt_next = pkt->_next;
            _bcm_tx_packet_done(pkt);
            pkt = pkt_next;
        }

        dv = dv_list;
        while (dv) {
            /* Chain done may deallocate dv */
            dv_next = TX_DV_NEXT(dv);
            _bcm_tx_chain_done(dv->dv_unit, dv);
            dv = dv_next;
        }
    }
    sal_thread_exit(0);
}

/*
 * Function:
 *      _bcm_tx_chain_done_cb
 * Purpose:
 *      Interrupt handler callback to schedule chain done processing
 * Parameters:
 *      unit - SOC unit #
 *      dv - pointer to dv that has completed.
 * Returns:
 *      Nothing
 * Notes:
 *      All the handler does is put the DV on a queue and
 *      check if the handler thread needs to be awakened.
 *
 *      This function is called in INTERRUPT CONTEXT,
 *      but is also called in non-interrupt context from
 *      _bcm_tx_chain_send, so TX_LOCK must be used.
 */

STATIC void
_bcm_tx_chain_done_cb(int unit, dv_t *dv)
{
    int spl;
	if (BCM_SUCCESS(_bcm_tx_cb_intr_enabled())) {
		_bcm_tx_chain_done(unit, dv);
	} else {
	    TX_LOCK(spl);
	    ++_tx_chain_done_intr;
	    dv->dv_unit = unit;
	    TX_DV_NEXT_SET(dv, NULL);
	    if (dv_pend_last) { /* Queue is non-empty */
	        TX_DV_NEXT_SET(dv_pend_last, dv);
	        dv_pend_last = dv;
	    } else { /* Empty queue; init first and last */
	        dv_pend_first = dv;
	        dv_pend_last = dv;
	    }
	    TX_UNLOCK(spl);
	    sal_sem_give(tx_cb_sem);
	}
}

/*
 * Function:
 *      _bcm_tx_chain_done
 * Purpose:
 *      Process the completion of a TX DMA chain.
 * Parameters:
 *      unit - SOC unit number.
 *      dv - pointer to completed DMA chain.
 *      dcb - ignored
 * Returns:
 *      Nothing.
 * Notes:
 */

STATIC void
_bcm_tx_chain_done(int unit, dv_t *dv)
{
    bcm_pkt_cb_f  callback;
    volatile bcm_pkt_t *pkt;
    void *cookie;

    assert(dv != NULL);

    ++_tx_chain_done;
    /*
     * Operation complete; call user's completion callback routine, if any.
     */

    callback = TX_INFO(dv)->chain_done_cb;
    if (callback) {
        pkt = TX_INFO(dv)->pkt[0];
        cookie = TX_INFO(dv)->cookie;
        callback(unit, (bcm_pkt_t *)pkt, cookie);
    }

    _tx_dv_free(unit, dv);
}

/*
 * Function:
 *      _bcm_tx_packet_done_cb
 * Purpose:
 *      Packet done interrupt callback
 * Parameters:
 *      unit - SOC unit number.
 *      dv - pointer to completed DMA chain.
 * Returns:
 *      Nothing.
 * Notes:
 *      Just checks to see if the packet's callback is set.  If
 *      it is, the packet is added to the packet pending list
 *      and the tx thread is woken up.
 *
 *      This will only be set up as a call back if some packet in the
 *      chain requires a callback.
 */

STATIC void
_bcm_tx_packet_done_cb(int unit, dv_t *dv, dcb_t *dcb)
{
    bcm_pkt_t *pkt;

    COMPILER_REFERENCE(dcb);
    assert(dv);
    assert(TX_INFO(dv));
    assert(TX_INFO(dv)->pkt_count > TX_INFO(dv)->pkt_done_cnt);

    pkt = (bcm_pkt_t *)TX_INFO_CUR_PKT(dv);
    pkt->_dv = dv;
    pkt->unit = unit;
    pkt->_next = NULL;

    /*
     * If callback present, add to list
     */
	if (BCM_SUCCESS(_bcm_tx_cb_intr_enabled())) {
		TX_INFO_PKT_MARK_DONE(dv);
		_bcm_tx_packet_done(pkt);
	} else {
	    if (pkt->call_back) {
	        /* Assumes interrupt context */
	        if (pkt_pend_last) { /* Queue is non-empty */
	            pkt_pend_last->_next = pkt;
	            pkt_pend_last = pkt;
	        } else { /* Empty queue; init first and last */
	            pkt_pend_first = pkt;
	            pkt_pend_last = pkt;
	        }
	        sal_sem_give(tx_cb_sem);  
	    }
	    TX_INFO_PKT_MARK_DONE(dv);
	}
}

STATIC void
_bcm_tx_packet_done(bcm_pkt_t *pkt)
{
#if defined (INCLUDE_RCPU) && defined(BCM_XGS3_SWITCH_SUPPORT)
    if (SOC_IS_RCPU_UNIT(pkt->unit)) {
        (pkt->call_back)(pkt->unit, pkt, pkt->_dv);
    } else {
        (pkt->call_back)(pkt->unit, pkt,
                         TX_INFO((dv_t *)(pkt->_dv))->cookie);
    }
#else
    (pkt->call_back)(pkt->unit, pkt,
                     TX_INFO((dv_t *)(pkt->_dv))->cookie);
#endif /* INCLUDE_RCPU && BCM_XGS3_SWITCH_SUPPORT */
}


#if defined (INCLUDE_RCPU) && defined(BCM_XGS3_SWITCH_SUPPORT)
/*
 * Function:
 *      _bcm_rcpu_tx_packet_done_cb
 * Purpose:
 *      Packet done callback from rcpu
 * Parameters:
 *      unit - SOC unit number.
 *      pkt  - packet pointer
 * Returns:
 *      Nothing.
 * Notes:
 *      The packet is added to the packet pending list
 *      and the tx thread is woken up.
 *
 *      This will only be set up as a call back if some packet in the
 *      chain requires a callback.
 *
 *      This function should be only called in Task context.
 */

void
_bcm_rcpu_tx_packet_done_cb(int unit, bcm_pkt_t *pkt)
{
    int spl;
    /*
     * If callback present, add to list
     */
    if (pkt->call_back) {
        TX_LOCK(spl); 
        /* Assumes interrupt context */
        if (pkt_pend_last) { /* Queue is non-empty */
            pkt_pend_last->_next = pkt;
            pkt_pend_last = pkt;
        } else { /* Empty queue; init first and last */
            pkt_pend_first = pkt;
            pkt_pend_last = pkt;
        }
        TX_UNLOCK(spl);

        sal_sem_give(tx_cb_sem);
    }
}
#endif /* INCLUDE_RCPU && BCM_XGS3_SWITCH_SUPPORT */

/****************************************************************
 *
 * TX DV info dump routine.
 *
 ****************************************************************/

#if defined(BROADCOM_DEBUG)
/*
 * Function:
 *      bcm_tx_show
 * Purpose:
 *      Display info about tx state
 * Parameters:
 *      unit - ignored
 * Returns:
 *      None
 * Notes:
 */

int
bcm_common_tx_show(int unit)
{

    COMPILER_REFERENCE(unit);

    LOG_CLI((BSL_META_U(unit,
                        "TX state:  chain_send %d. chain_done %d. "
                        "chain_done_intr %d\n"),
             _tx_chain_send,
             _tx_chain_done,
             _tx_chain_done_intr));
    LOG_CLI((BSL_META_U(unit,
                        "           pkt_pend_first %p. pkt_pend_last %p.\n"),
             (void *)pkt_pend_first,
             (void *)pkt_pend_last));
    LOG_CLI((BSL_META_U(unit,
                        "           dv_pend_first %p. dv_pend_last %p.\n"),
             (void *)dv_pend_first,
             (void *)dv_pend_last));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tx_dv_dump
 * Purpose:
 *      Display info about a DV that is setup for TX.
 * Parameters:
 *      unit - transmission unit
 *      dv - The DV to show info about
 * Returns:
 *      None
 * Notes:
 *      Mainly, dumps the tx_dv_info_t structure; then calls soc_dma_dump_dv
 */

int
bcm_common_tx_dv_dump(int unit, void *dv_p)
{
    tx_dv_info_t *dv_info;
    dv_t *dv = (dv_t *)dv_p;

    dv_info = TX_INFO(dv);
    if (dv_info != NULL) {
        LOG_CLI((BSL_META_U(unit,
                            "TX DV info:\n    DV %p. pkt count %d. done count %d.\n"),
                 (void *)dv, dv_info->pkt_count, dv_info->pkt_done_cnt));
#if !defined(__PEDANTIC__)
        LOG_CLI((BSL_META_U(unit,
                            "    cookie %p. cb %p\n"), (void *)dv_info->cookie,
                 (void *)dv_info->chain_done_cb));
#else /* !defined(__PEDANTIC__) */
        LOG_CLI((BSL_META_U(unit,
                            "    cookie %p. cb pointer unprintable\n"),
                 (void *)dv_info->cookie));
#endif /* !defined(__PEDANTIC__) */
    } else {
        LOG_CLI((BSL_META_U(unit,
                            "TX DV info is NULL\n"))); 
    }
    soc_dma_dump_dv(dv->dv_unit, "", dv);

    return BCM_E_NONE;
}

#endif  /* BROADCOM_DEBUG */


#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT) */


#ifdef  BCM_RPC_SUPPORT

/****************************************************************
 *
 * TX CPU tunnel support
 *
 ****************************************************************/

/*
 * Function:
 *      bcm_tx_cpu_tunnel_set/get
 * Purpose:
 *      Set/get the function used to tunnel packets to remote CPUs for TX
 * Returns:
 *      BCM_E_NONE
 * Notes:
 *      If set to NULL, tunneling won't happen and an error will be
 *      returned by bcm_tx if a remoted device is specified.
 */

static bcm_tx_cpu_tunnel_f tx_cpu_tunnel;

int
bcm_tx_cpu_tunnel_set(bcm_tx_cpu_tunnel_f f)
{
    tx_cpu_tunnel = f;

    return BCM_E_NONE;
}

int
bcm_tx_cpu_tunnel_get(bcm_tx_cpu_tunnel_f *f)
{
    if (f != NULL) {
        *f = tx_cpu_tunnel;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_tx_cpu_tunnel
 * Purpose:
 *      Call the programmed tx tunnel function, if programmed
 * Returns:
 *      BCM_E_INIT if tunnel function is not initialized
 *      Otherwise, returns the value from the tunnel call
 */

int
bcm_tx_cpu_tunnel(bcm_pkt_t *pkt,
                  int dest_unit,
                  int remote_port,
                  uint32 flags,
                  bcm_cpu_tunnel_mode_t mode)
{
    if (tx_cpu_tunnel != NULL) {
        return tx_cpu_tunnel(pkt, dest_unit, remote_port, flags, mode);
    }

    return BCM_E_INIT;
}

#endif /* BCM_RPC_SUPPORT */

