/*
 * $Id: dma.h 1.55.26.1 Broadcom SDK $
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
 * File:        socdma.h
 * Purpose:     Maps out structures used for DMA operations and
 *              exports routines.
 */

#ifndef _SOC_DMA_H
#define _SOC_DMA_H

#include <sal/types.h>
#include <sal/core/sync.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>

#include <shared/types.h>

#include <soc/types.h>
#include <soc/cmic.h>
#include <soc/dcb.h>
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif

#if defined(BE_HOST) && defined(LE_HOST)
#error "Define either BE_HOST or LE_HOST. Both BE_HOST and LE_HOST should not be defined at the same time"
#endif

#define SOC_DMA_ROUND_LEN(x)    (((x) + 3) & ~3)

/* Type for DMA channel number */

typedef _shr_dma_chan_t    dma_chan_t;

/*
 * Defines:
 *      SOC_DMA_DV_FREE_CNT/SOC_DMA_DV_FREE_SIZE
 * Purpose:
 *      Defines the number of free DV's that the DMA code will cache
 *      to avoid calling alloc/free routines. FREE_CNT is number of 
 *      dv_t's, and FREE_SIZE is the number of DCBs in the DV. 
 * Notes:
 *      Allocation requests for DV's with < FREE_SIZE dcbs MAY result in 
 *      an allocation of a DV with FREE_SIZE dcbs. Allocations with 
 *      > FREE_SIZE will always result in calls to memory allocation
 *      routines.
 */

#define SOC_DMA_DV_FREE_CNT     16
#define SOC_DMA_DV_FREE_SIZE    8

#define SOC_DV_PKTS_MAX         16      /* Determines alloc size of
                                           DMA'able data in DVs */

typedef int8    dvt_t;                  /* DV Type definition */

#define         DV_NONE         0       /* Disable/Invalid */
#define         DV_TX           1       /* Transmit data */
#define         DV_RX           2       /* Receive data  */

#define SOC_DMA_DV_TX_FREE_SIZE 3
#define SOC_DMA_DV_RX_FREE_SIZE 1

/*
 * Typedef:
 *      soc_tx_param_t
 * Purpose:
 *      Strata XGS hardware specific info related to packet TX;
 *      Will be used as a vehicle for passing information to
 *      the DCB-Add-TX operation.
 *      Information is per packet.  The data buffer pointer and
 *      byte count is passed separately.
 */

typedef struct soc_tx_param_s {
    uint8             cos;          /* The local COS# to use */
    uint8             prio_int;     /* Internal priority of the packet */
    uint8             src_port;     /* Header/tag ONLY.  Use rx_port below */
    uint8             src_mod;
    uint8             dest_port;    /* Header/tag ONLY.  Use tx_pbmp below */
    uint8             dest_mod;
    uint8             opcode;
    uint8             pfm;          /* see BCM_PORT_PFM_* */

    soc_pbmp_t        tx_pbmp;      /* Target ports */
    soc_pbmp_t        tx_upbmp;     /* Untagged ports */
    soc_pbmp_t        tx_l3pbmp;    /* L3 ports */

    /* Uses SOC_DMA and SOC_DMA_F flags from below */
    uint32            flags;
    /* Other new headers/tags may be added here */
} soc_tx_param_t;

/*
 * Typedef:
 *      dv_t
 * Purpose:
 *      Maps out the I/O Vec structure used to pass DMA chains to the
 *      the SOC DMA routines "DMA I/O Vector".
 * Notes:
 *      To start a DMA request, the caller must fill in:
 *              dv_op: operation to perform (DV_RX or DV_TX).
 *              dv_flags: Set DV_F_NOTIFY_DSC for descriptor done callout
 *                        Set DV_F_NOTIFY_CHN for chain done callout
 *                        Set DV_F_WAIT to suspend in dma driver
 *              dv_valid: # valid DCB entries (this field is initialized
 *                      by soc_dma_dv_alloc, and set properly if
 *                      soc_dma_add_dcb is called to build chain).
 *              dv_done_chain: NULL if no callout for chain done, or the
 *                      address of the routine to call when chain done
 *                      is seen. It is called with 2 parameters, the
 *                      unit # and a pointer to the DV chain done has been
 *                      seen on.
 *              dv_done_desc: NULL for synchronous call, or the address of
 *                      the function to call on descriptor done. The function
 *                      is with 3 parameters, the unit #, a pointer to
 *                      the DV structure, and a pointer to the DCB completed.
 *                      One call is made for EVERY DCB, and only if the
 *                      DCB is DONE.
 *              dv_done_packet: NULL if no callout for packet done, or the
 *                      address of the routine to call when packet done
 *                      is seen. It has the same prototype as dv_done_desc.
 *              dv_public1 - 4: Not used by DMA routines,
 *                      for use by caller.
 *
 *     Scatter/gather is implemented through multiple DCBs pointing to
 *     different buffers with the S/G bit set.  End of S/G chain (end of
 *     packet) is indicated by having S/G bit clear in the DCB.
 *
 *     Chains of packets can be associated with a single DV.  This
 *     keeps the HW busy DMA'ing packets even as interrupts are
 *     processed.  DVs can be chained (a software construction)
 *     which will start a new DV from this file rather than calling
 *     back.  This is not done much in our code.
 */

typedef struct dv_s {
    struct dv_s *dv_next,               /* Queue pointers if required */
                *dv_chain;              /* Pointer to next DV in chain */
    int         dv_unit;                /* Unit dv is allocated on */
    uint32      dv_magic;               /* Used to indicate valid */
    dvt_t       dv_op;                  /* Operation to be performed */
    dma_chan_t  dv_channel;             /* Channel queued on */
    int         dv_flags;               /* Flags for operation */
    /* Fix soc_dma_dv_reset if you add flags */
#   define      DV_F_NOTIFY_DSC         0x01    /* Notify on dsc done */
#   define      DV_F_NOTIFY_CHN         0x02    /* Notify on chain done */
#   define      DV_F_COMBINE_DCB        0x04    /* Combine DCB where poss. */
#   define      DV_F_NEEDS_REFILL       0x10    /* Needs to be refilled */
    int         dv_cnt;                 /* # descriptors allocated */
    int         dv_vcnt;                /* # descriptors valid */
    int         dv_dcnt;                /* # descriptors done */
    void        (*dv_done_chain)(int u, struct dv_s *dv_chain);
    void        (*dv_done_desc)(int u, struct dv_s *dv, dcb_t *dcb);
    void        (*dv_done_packet)(int u, struct dv_s *dv, dcb_t *dcb);
    any_t       dv_public1;             /* For caller */
    any_t       dv_public2;             /* For caller */
    any_t       dv_public3;             /* For caller */
    any_t       dv_public4;             /* For caller */

    /*
     * Information for SOC_TX_PKT_PROPERTIES.
     * Normally, packets are completely independent across a DMA TX chain.
     * In order to program the cmic dma tx register effeciently, the data
     * in soc_tx_param must be consistent for all packets in the chain.
     */
    soc_tx_param_t   tx_param;

    /*
     * Buffer for gather-inserted data.  Possibly includes:
     *     HiGig hdr     (12 bytes)
     *     HiGig2 hdr    (16 bytes) (HG2/SL mutually exclusive)
     *     SL tag        (4 bytes)
     *     VLAN tag      (4 bytes)
     *     Dbl VLAN tag  (4 bytes)
     */
    uint8       *dv_dmabuf;
    uint32      dv_dma_buf_size;
#define SOC_DMA_BUF_LEN         24      /* Maximum header size from above */
#define SOC_DV_DMABUF_SIZE      (SOC_DV_PKTS_MAX * SOC_DMA_BUF_LEN)
#define SOC_DV_HG_HDR(dv, i)    (&((dv)->dv_dmabuf[SOC_DMA_BUF_LEN*i+0]))
#define SOC_DV_SL_TAG(dv, i)    (&((dv)->dv_dmabuf[SOC_DMA_BUF_LEN*i+12]))
#define SOC_DV_VLAN_TAG(dv, i)  (&((dv)->dv_dmabuf[SOC_DMA_BUF_LEN*i+16]))
#define SOC_DV_VLAN_TAG2(dv, i) (&((dv)->dv_dmabuf[SOC_DMA_BUF_LEN*i+20]))

/* Optionally allocated for Tx alignment */
#define SOC_DV_TX_ALIGN(dv, i) (&((dv)->dv_dmabuf[dv->dv_dma_buf_size + 4 * i]))

    dcb_t       *dv_dcb;
} dv_t;

/*
 * Typedef:
 *      sdc_t (SOC DMA Control)
 * Purpose:
 *      Each DMA channel on each SOC has one of these structures to
 *      track the currently active or queued operations.
 */

typedef struct sdc_s {
    dma_chan_t  sc_channel;             /* Channel # for reverse lookup */
    dvt_t       sc_type;                /* DV type that we accept */
    uint8       sc_flags;               /* See SDMA_CONFIG_XXX */
    dv_t        *sc_q;                  /* Request queue head */
    dv_t        *sc_q_tail;             /* Request queue tail */
    dv_t        *sc_dv_active;          /* Pointer to individual active DV */
    int         sc_q_cnt;               /* # requests queued + active */
} sdc_t;

/* Do not alter ext_dcb flag below */
#define SET_NOTIFY_CHN_ONLY(flags) do { \
    (flags) |=  DV_F_NOTIFY_CHN; \
    (flags) &= ~DV_F_NOTIFY_DSC; \
} while (0)


/* Try to avoid all other flags */
#define         SOC_DMA_F_PKT_PROP      0x10000000    /* 1 << 28 */

/*
 * Note DMA_F_INTR is NOT a normal flag.
 *    Interrupt mode is the default behavior and is ! POLLED mode.
 */
#define         SOC_DMA_F_INTR          0x00 /* Interrupt Mode */

#define         SOC_DMA_F_MBM           0x01 /* Modify bit MAP */
#define         SOC_DMA_F_POLL          0x02 /* POLL mode */
#define         SOC_DMA_F_TX_DROP       0x04 /* Drop if no ports */
#define         SOC_DMA_F_JOIN          0x08 /* Allow low level DV joins */
#define         SOC_DMA_F_DEFAULT       0x10 /* Default channel for type */
#define         SOC_DMA_F_CLR_CHN_DONE  0x20 /* Clear Chain-done on start */
#define         SOC_DMA_F_INTR_ON_DESC  0x40 /* Interrupt per descriptor */

extern int      soc_dma_init(int unit);
extern int      soc_dma_attach(int unit, int reset);
extern int      soc_dma_detach(int unit);

extern int      soc_dma_chan_config(int unit, dma_chan_t chan, dvt_t type, 
                                    uint32 flags);
extern dvt_t    soc_dma_chan_dvt_get(int unit, dma_chan_t chan);

extern dv_t     *soc_dma_dv_alloc(int unit, dvt_t, int cnt);
extern dv_t     *soc_dma_dv_alloc_by_port(int unit, dvt_t op, int cnt, 
                                          int pkt_to_ports);
extern void     soc_dma_dv_free(int unit, dv_t *);
extern void     soc_dma_dv_reset(dvt_t, dv_t *);
extern int      soc_dma_dv_join(dv_t *dv_list, dv_t *dv_add);

extern int      soc_dma_desc_add(dv_t *, sal_vaddr_t, uint16, pbmp_t, 
                                 pbmp_t, pbmp_t, uint32 flags, uint32 *hgh);
extern void soc_dma_dump_dv_dcb(int unit, char *pfx, dv_t *dv_chain, int index);

#define         SOC_DMA_COS(_x)         ((_x) << 0)
#define         SOC_DMA_COS_GET(_x)     (((_x) >> 0) & 7)
#define         SOC_DMA_CRC_REGEN       (1 << 3)
#define         SOC_DMA_CRC_GET(_x)     (((_x) >> 3) & 1)
#define         SOC_DMA_RLD             (1 << 4)
#define         SOC_DMA_HG              (1 << 22)
#define         SOC_DMA_STATS           (1 << 23)
#define         SOC_DMA_PURGE           (1 << 24)

/* Max 7 bits for mod, 6 bits for port and 1 for trunk indicator */
#define _SDP_MSK 0x3f           /* 10:5 */
#define _SDP_S 5                /* 10:5 */
#define _SDM_MSK 0x7f           /* 17:11 */
#define _SDM_S 11               /* 17:11 */
#define _SDT_MSK 0x1            /* 18:18 */
#define _SDT_S 18               /* 18:18 */
#define _SMHOP_MSK 0x7          /* 21:19 */
#define _SMHOP_S 19             /* 21:19 */

/* 
 * type 9 descriptor, Higig, stats, purge bits
 */

#define _SHG_MSK 0x1            /* 22:22 */
#define _SHG_S 22               /* 22:22 */
#define _SSTATS_MSK 0x1         /* 23:23 */
#define _SSTATS_S 23            /* 23:23 */
#define _SPURGE_MSK 0x1         /* 24:24 */
#define _SPURGE_S 24            /* 24:24 */


#define SOC_DMA_DPORT_GET(flags) \
            SOC_SM_FLAGS_GET(flags, _SDP_S, _SDP_MSK)
#define SOC_DMA_DPORT_SET(flags, val)  \
            SOC_SM_FLAGS_SET(flags, val, _SDP_S, _SDP_MSK)
       
#define SOC_DMA_DMOD_GET(flags) \
            SOC_SM_FLAGS_GET(flags, _SDM_S, _SDM_MSK)
#define SOC_DMA_DMOD_SET(flags, val) \
            SOC_SM_FLAGS_SET(flags, val, _SDM_S, _SDM_MSK)

#define SOC_DMA_DTGID_GET(flags) \
            SOC_SM_FLAGS_GET(flags, _SDT_S, _SDT_MSK)
#define SOC_DMA_DTGID_SET(flags, val) \
            SOC_SM_FLAGS_SET(flags, val, _SDT_S, _SDT_MSK)

#define SOC_DMA_MHOP_GET(flags) \
            SOC_SM_FLAGS_GET(flags, _SMHOP_S, _SMHOP_MSK)
#define SOC_DMA_MHOP_SET(flags, val) \
            SOC_SM_FLAGS_SET(flags, val, _SMHOP_S, _SMHOP_MSK)

#define SOC_DMA_HG_GET(flags) \
            SOC_SM_FLAGS_GET(flags, _SHG_S, _SHG_MSK)
#define SOC_DMA_HG_SET(flags, val) \
            SOC_SM_FLAGS_SET(flags, val, _SHG_S, _SHG_MSK)

#define SOC_DMA_STATS_GET(flags) \
            SOC_SM_FLAGS_GET(flags, _SSTATS_S, _SSTATS_MSK)
#define SOC_DMA_STATS_SET(flags, val) \
            SOC_SM_FLAGS_SET(flags, val, _SSTATS_S, _SSTATS_MSK)

#define SOC_DMA_PURGE_GET(flags) \
            SOC_SM_FLAGS_GET(flags, _SPURGE_S, _SPURGE_MSK)
#define SOC_DMA_PURGE_SET(flags, val) \
            SOC_SM_FLAGS_SET(flags, val, _SPURGE_S, _SPURGE_MSK)

extern int      soc_dma_rld_desc_add(dv_t *, sal_vaddr_t);
extern void     soc_dma_desc_end_packet(dv_t *);

extern int      soc_dma_start(int unit, dma_chan_t, dv_t *);
extern int      soc_dma_abort_channel_total(int unit, dma_chan_t channel);
extern int      soc_dma_abort_dv(int unit, dv_t *);
extern int      soc_dma_abort(int unit);

/* Wait on synchronous send - requires a context */
extern int      soc_dma_wait(int unit, dv_t *dv_chain);
extern int      soc_dma_wait_timeout(int unit, dv_t *dv_chain, int usec);

extern void     soc_dma_higig_dump(int unit, char *pfx, uint8 *addr,
                                   int len, int pkt_len, int *ether_offset);
extern void     soc_dma_ether_dump(int unit, char *pfx, uint8 *addr,
                                   int len, int offset);

extern int      soc_dma_dv_valid(dv_t *dv);
extern void     soc_dma_dump_dv(int unit, char *pfx, dv_t *);
extern void     soc_dma_dump_pkt(int unit, char *pfx, uint8 *addr, int len,
                                 int decode);

/* Interrupt Routines */

extern void     soc_dma_done_desc(int unit, uint32 chan);
extern void     soc_dma_done_chain(int unit, uint32 chan);

#ifdef  BCM_XGS3_SWITCH_SUPPORT
int soc_dma_tx_purge(int unit, dma_chan_t c);
#endif  /* BCM_XGS3_SWITCH_SUPPORT */

/*
 * Simplified API for ROM TX/RX polled mode.
 * See $SDK/doc/dma_rom.txt for more information.
 */

#ifndef SOC_DMA_ROM_TX_CHANNEL
#define SOC_DMA_ROM_TX_CHANNEL 2
#endif

#ifndef SOC_DMA_ROM_RX_CHANNEL
#define SOC_DMA_ROM_RX_CHANNEL 3
#endif

extern int      soc_dma_rom_init(int unit, int max_packet_rx, int tx, int rx); 
extern int      soc_dma_rom_detach(int unit);
extern dcb_t    *soc_dma_rom_dcb_alloc(int unit, int psize); 
extern int      soc_dma_rom_dcb_free(int unit, dcb_t *dcb); 
extern int      soc_dma_rom_tx_start(int unit, dcb_t *dcb); 
extern int      soc_dma_rom_tx_poll(int unit, int *done); 
extern int      soc_dma_rom_tx_abort(int unit); 
extern int      soc_dma_rom_rx_poll(int unit, dcb_t **dcb); 

/*
 * Atomic memory allocation functions that are safe to call
 * from interrupt context when BCM_TX_FROM_INTR is defined.
 * This memory does not necessarily need to be DMA-safe.
 *
 * Currently the assumption is that soc_cm_salloc/sfree are
 * atomic if BCM_TX_FROM_INTR is defined.
 *
 */
#ifdef BCM_TX_FROM_INTR
#define SOC_DMA_NEEDS_ATOMIC_ALLOC
#endif

#if defined(SAL_HAS_ATOMIC_ALLOC)
/* Dedicated functions provided by SAL */
extern void *soc_at_alloc(int dev, int size, const char *name);
extern void soc_at_free(int dev, void *ptr);
#elif defined(SOC_DMA_NEEDS_ATOMIC_ALLOC) && !defined(SAL_ALLOC_IS_ATOMIC)
/* Use CM DMA allocator for atomic allocation */
#define soc_at_alloc(_u, _sz, _nm) soc_cm_salloc(_u, _sz, _nm)
#define soc_at_free(_u, _ptr) soc_cm_sfree(_u, _ptr)
#else
/* */
#define soc_at_alloc(_u, _sz, _nm) sal_alloc(_sz, _nm)
#define soc_at_free(_u, _ptr) sal_free(_ptr)
#endif

#endif  /* !_SOC_DMA_H */
