/*
 * Generic Broadcom Home Networking Division (HND) DMA engine SW interface
 * This supports the following chips: BCM42xx, 44xx, 47xx .
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
 * $Id: hnddma.h 1.4 Broadcom SDK $
 */

#ifndef	_hnddma_h_
#define	_hnddma_h_

typedef const struct hnddma_pub hnddma_t;

/* dma function type */
typedef void (*di_detach_t)(hnddma_t *dmah);
typedef bool (*di_txreset_t)(hnddma_t *dmah);
typedef bool (*di_rxreset_t)(hnddma_t *dmah);
typedef bool (*di_rxidle_t)(hnddma_t *dmah);
typedef void (*di_txinit_t)(hnddma_t *dmah);
typedef bool (*di_txenabled_t)(hnddma_t *dmah);
typedef void (*di_rxinit_t)(hnddma_t *dmah);
typedef void (*di_txsuspend_t)(hnddma_t *dmah);
typedef void (*di_txresume_t)(hnddma_t *dmah);
typedef bool (*di_txsuspended_t)(hnddma_t *dmah);
typedef bool (*di_txsuspendedidle_t)(hnddma_t *dmah);
typedef int (*di_txfast_t)(hnddma_t *dmah, void *p, bool commit);
typedef void (*di_fifoloopbackenable_t)(hnddma_t *dmah, bool on);
typedef bool  (*di_txstopped_t)(hnddma_t *dmah);
typedef bool  (*di_rxstopped_t)(hnddma_t *dmah);
typedef bool  (*di_rxenable_t)(hnddma_t *dmah);
typedef bool  (*di_rxenabled_t)(hnddma_t *dmah);
typedef void* (*di_rx_t)(hnddma_t *dmah);
typedef void (*di_rxfill_t)(hnddma_t *dmah);
typedef void (*di_txreclaim_t)(hnddma_t *dmah, bool forceall);
typedef void (*di_rxreclaim_t)(hnddma_t *dmah);
typedef	uintptr	(*di_getvar_t)(hnddma_t *dmah, const char *name);
typedef void* (*di_getnexttxp_t)(hnddma_t *dmah, bool forceall);
typedef void* (*di_getnextrxp_t)(hnddma_t *dmah, bool forceall);
typedef void* (*di_peeknexttxp_t)(hnddma_t *dmah);
typedef void (*di_txblock_t)(hnddma_t *dmah);
typedef void (*di_txunblock_t)(hnddma_t *dmah);
typedef uint (*di_txactive_t)(hnddma_t *dmah);
typedef void (*di_txrotate_t)(hnddma_t *dmah);
typedef void (*di_counterreset_t)(hnddma_t *dmah);
typedef char* (*di_dump_t)(hnddma_t *dmah, struct bcmstrbuf *b, bool dumpring);
typedef char* (*di_dumptx_t)(hnddma_t *dmah, struct bcmstrbuf *b, bool dumpring);
typedef char* (*di_dumprx_t)(hnddma_t *dmah, struct bcmstrbuf *b, bool dumpring);
typedef uint (*di_rxactive_t)(hnddma_t *dmah);
typedef uint (*di_txpending_t)(hnddma_t *dmah);
typedef void (*di_rxsephdrctrl_t)(hnddma_t *dmah, bool enable, 
        uint memtype, uint size);

/* dma opsvec */
typedef struct di_fcn_s {
	di_detach_t		detach;
	di_txinit_t             txinit;
	di_txreset_t		txreset;
	di_txenabled_t          txenabled;
	di_txsuspend_t          txsuspend;
	di_txresume_t           txresume;
	di_txsuspended_t        txsuspended;
	di_txsuspendedidle_t    txsuspendedidle;
	di_txfast_t             txfast;
	di_txstopped_t		txstopped;
	di_txreclaim_t          txreclaim;
	di_getnexttxp_t         getnexttxp;
	di_peeknexttxp_t        peeknexttxp;
	di_txblock_t            txblock;
	di_txunblock_t          txunblock;
	di_txactive_t           txactive;
	di_txrotate_t           txrotate;

	di_rxinit_t             rxinit;
	di_rxreset_t            rxreset;
	di_rxidle_t             rxidle;
	di_rxstopped_t		rxstopped;
	di_rxenable_t		rxenable;
	di_rxenabled_t		rxenabled;
	di_rx_t                 rx;
	di_rxfill_t             rxfill;
	di_rxreclaim_t          rxreclaim;
	di_getnextrxp_t         getnextrxp;

	di_fifoloopbackenable_t fifoloopbackenable;
	di_getvar_t             d_getvar;
	di_counterreset_t       counterreset;
	di_dump_t		dump;
	di_dumptx_t		dumptx;
	di_dumprx_t		dumprx;
	di_rxactive_t		rxactive;
	di_txpending_t		txpending;
        di_rxsephdrctrl_t	rxsephdrctrl;
	uint			endnum;
} di_fcn_t;

/*
 * Exported data structure (read-only)
 */
/* export structure */
struct hnddma_pub {
	di_fcn_t	di_fn;		/* DMA function pointers */
	uint		txavail;	/* # free tx descriptors */

	/* rx error counters */
	uint		rxgiants;	/* rx giant frames */
	uint		rxnobuf;	/* rx out of dma descriptors */
	/* tx error counters */
	uint		txnobuf;	/* tx out of dma descriptors */
};


extern hnddma_t * dma_attach(osl_t *osh, char *name, si_t *sih, void *dmaregstx, void *dmaregsrx,
                             uint ntxd, uint nrxd, uint rxbufsize, uint nrxpost, uint rxoffset,
                             uint *msg_level, uint pkt_mem, uint pkthdr_mem, uint en_rxsephdr);
#define dma_detach(di)			((di)->di_fn.detach(di))
#define dma_txreset(di)			((di)->di_fn.txreset(di))
#define dma_rxreset(di)			((di)->di_fn.rxreset(di))
#define dma_rxidle(di)			((di)->di_fn.rxidle(di))
#define dma_txinit(di)                  ((di)->di_fn.txinit(di))
#define dma_txenabled(di)               ((di)->di_fn.txenabled(di))
#define dma_rxinit(di)                  ((di)->di_fn.rxinit(di))
#define dma_txsuspend(di)               ((di)->di_fn.txsuspend(di))
#define dma_txresume(di)                ((di)->di_fn.txresume(di))
#define dma_txsuspended(di)             ((di)->di_fn.txsuspended(di))
#define dma_txsuspendedidle(di)         ((di)->di_fn.txsuspendedidle(di))
#define dma_txfast(di, p, commit)	((di)->di_fn.txfast(di, p, commit))
#define dma_fifoloopbackenable(di, on)      ((di)->di_fn.fifoloopbackenable(di, on))
#define dma_txstopped(di)               ((di)->di_fn.txstopped(di))
#define dma_rxstopped(di)               ((di)->di_fn.rxstopped(di))
#define dma_rxenable(di)                ((di)->di_fn.rxenable(di))
#define dma_rxenabled(di)               ((di)->di_fn.rxenabled(di))
#define dma_rx(di)                      ((di)->di_fn.rx(di))
#define dma_rxfill(di)                  ((di)->di_fn.rxfill(di))
#define dma_txreclaim(di, forceall)	((di)->di_fn.txreclaim(di, forceall))
#define dma_rxreclaim(di)               ((di)->di_fn.rxreclaim(di))
#define dma_getvar(di, name)		((di)->di_fn.d_getvar(di, name))
#define dma_getnexttxp(di, forceall)    ((di)->di_fn.getnexttxp(di, forceall))
#define dma_getnextrxp(di, forceall)    ((di)->di_fn.getnextrxp(di, forceall))
#define dma_peeknexttxp(di)             ((di)->di_fn.peeknexttxp(di))
#define dma_txblock(di)                 ((di)->di_fn.txblock(di))
#define dma_txunblock(di)               ((di)->di_fn.txunblock(di))
#define dma_txactive(di)                ((di)->di_fn.txactive(di))
#define dma_rxactive(di)                ((di)->di_fn.rxactive(di))
#define dma_txrotate(di)                ((di)->di_fn.txrotate(di))
#define dma_counterreset(di)            ((di)->di_fn.counterreset(di))
#define dma_txpending(di)		((di)->di_fn.txpending(di))
#define dma_rxsephdrctrl(di, enable, memtype, size)            ((di)->di_fn.rxsephdrctrl(di, enable, memtype, size))
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
#define dma_dump(di, buf, dumpring)	((di)->di_fn.dump(di, buf, dumpring))
#define dma_dumptx(di, buf, dumpring)	((di)->di_fn.dumptx(di, buf, dumpring))
#define dma_dumprx(di, buf, dumpring)	((di)->di_fn.dumprx(di, buf, dumpring))
#endif

/* return addresswidth allowed
 * This needs to be done after SB attach but before dma attach.
 * SB attach provides ability to probe backplane and dma core capabilities
 * This info is needed by DMA_ALLOC_CONSISTENT in dma attach
 */
extern uint dma_addrwidth(si_t *sih, void *dmaregs);

/* pio helpers */
extern void dma_txpioloopback(osl_t *osh, dma32regs_t *);


#define	RXSEPHDRSZ		64	/* rx separate header size */

#endif	/* _hnddma_h_ */
