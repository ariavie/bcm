/*
 * $Id: dma.c,v 1.65 Broadcom SDK $
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
 * Purpose:     SOC DMA LLC (Link Layer) driver; used for sending
 *              and receiving packets over PCI (and later, the uplink).
 *
 * NOTE:  DV chains are different than hardware chains. The hardware
 * chain bit is used to keep DMA going through multiple packets.
 * This corresponds to the sequence of DCBs (and usually multiple
 * packets) of one DV.  DVs are chained in a linked list in software.
 * The dv->dv_chain member (and dv_chain parameters) implement this
 * linked list.
 *   To improve the performance of packets receiving, we introduced the 
 * method to mitigate descriptor done interrupts. When we detect the 
 * packet storm is coming, we will disable descriptor done interrupt and
 * simultaneously start the monitor timer which will reenable the disabled
 * interrupt while storm is stopped.  
 */

#include <shared/bsl.h>

#include <sal/core/boot.h>
#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <shared/bsl.h>

#include <soc/drv.h>
#include <soc/dcb.h>
#include <soc/dcbformats.h>     /* only for 5670 crc erratum warning */
#include <soc/pbsmh.h>
#include <soc/higig.h>
#include <soc/debug.h>
#include <soc/cm.h>
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif
#ifdef INCLUDE_KNET
#include <soc/knet.h>
#endif

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(PORTMOD_SUPPORT)

#define DV_MAGIC_NUMBER 0xba5eba11

#define DMA_CHAIN_DONE_TIMEOUT_USEC         (4000)
#define DMA_CHAIN_DONE_INTERVAL_USEC        (1000)

typedef struct sdc_intr_mitigate_s {
    uint32        rx_chain_intr;
    uint32        rx_chain_intr_prev;    
    sal_usecs_t   cur_tm;
    sal_usecs_t   prev_tm; 
    char          sem_name[16];  
    sal_sem_t     thread_sem;
    sal_thread_t  thread_id;
    int           thread_running;
    int           mitigation_started;
} sdc_intr_mitigate_t;

typedef struct soc_dma_intr_mitigate_s {
    int                 enabled;    
    sdc_intr_mitigate_t chans_intr_mitigation[N_DMA_CHAN];  
} soc_dma_intr_mitigate_t;

static soc_dma_intr_mitigate_t soc_dma_intr_mitigation[SOC_MAX_NUM_DEVICES];

STATIC void
soc_dma_monitor_thread(void *p)
{
    uint32 param = PTR_TO_INT(p);
    int unit = param >> 16 & 0xffff;
    int chan = param & 0xffff;
    sdc_intr_mitigate_t *sdc_mtg = 
        &soc_dma_intr_mitigation[unit].chans_intr_mitigation[chan];
    
    while (1) {   
        if (sdc_mtg->thread_running == 0) {
            sal_sem_destroy(sdc_mtg->thread_sem);
            sal_memset(sdc_mtg, 0, sizeof(* sdc_mtg));
            break;
        }          
        if (!sdc_mtg->mitigation_started) {
            sal_sem_take(sdc_mtg->thread_sem, sal_sem_FOREVER);
            continue;
        }       
        sal_usleep(DMA_CHAIN_DONE_TIMEOUT_USEC);
        if (sdc_mtg->rx_chain_intr != sdc_mtg->rx_chain_intr_prev) {
            sdc_mtg->rx_chain_intr_prev = sdc_mtg->rx_chain_intr;
        } else { 
            /*enable descriptor done interrupt*/
#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm)) {
                (void)soc_cmicm_intr0_enable(unit, 
                              IRQ_CMCx_DESC_DONE(chan));
            } else 
#endif
            {       
                (void)soc_intr_enable(unit, IRQ_DESC_DONE(chan));
            }
            sdc_mtg->mitigation_started = 0; 
            sdc_mtg->rx_chain_intr_prev = sdc_mtg->rx_chain_intr = 0;
        }       
    }
}



/*
 * Function:
 *      soc_dma_channel
 * Purpose:
 *      Choose a channel to send a DMA request over.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      channel - -1 --> choose any channel.
 *                0-3 --> select given channel.
 * Returns:
 *      Pointer to soc_dma_start structure to use, NULL if failed.
 * Notes:
 *      If no channel is specified then the default channel for that
 *      operation type is chosen.
 */

STATIC sdc_t *
soc_dma_channel(int unit, dma_chan_t channel, dv_t *dv_chain)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    sdc_t               *cd;

    if (channel < 0) {                  /* Choose a channel */
        switch(dv_chain->dv_op) {
        case DV_TX:     return(soc->soc_dma_default_tx);
        case DV_RX:     return(soc->soc_dma_default_rx);
        default:        return(NULL);
        }
    } else if (channel < 0 || channel >= soc->soc_max_channels) {
        return NULL;
    } else {
        cd = &soc->soc_channels[channel];
        return((cd->sc_type == dv_chain->dv_op) ? cd : NULL);
    }
}

/*
 * Function:
 *      soc_dma_start_channel
 * Purpose:
 *      Start the next queued DMA (ignoring channel state)
 * Parameters:
 *      unit - StrataSwitch unit #
 *      sc - pointer to channel to start operation on.
 * Returns:
 *      nothing.
 * Notes:
 *      Assumes SOC_DMA_LOCK held on entry (or called from interrupt
 *      handler).
 *
 *      Sets sc_dv_active, but NOT sc_dv_q. Safe to call if
 *      no pending DMA to start, in which case sc_dv_active is set to NULL.
 */

STATIC void
soc_dma_start_channel(int unit, sdc_t *sc)
{
    dv_t *dv;
    
    if ((dv = sc->sc_dv_active = sc->sc_q) == NULL) {
        sc->sc_q_tail = NULL;
        return;
    }

#ifdef INCLUDE_KNET
    if (SOC_KNET_MODE(unit)) {
        return;
    }
#endif

#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        int cmc = SOC_PCI_CMC(unit);
        uint32 val;

        /* Set up DMA descriptor address */
        soc_pci_write(unit, CMIC_CMCx_DMA_DESCy_OFFSET(cmc, sc->sc_channel),
                      soc_cm_l2p(unit, sc->sc_q->dv_dcb));

        if (sc->sc_flags & SOC_DMA_F_CLR_CHN_DONE) {
            /* Clearing CMIC_CMC(0..2)_CH(0..3)_DMA_CTRL.ENABLE will clear CMIC_CMC(0..2)_DMA_STAT.CHAIN_DONE bit. */
            val = soc_pci_read(unit,CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, sc->sc_channel));
            val &= ~PKTDMA_ENABLE;
            soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, sc->sc_channel), val);
        } else {
            sc->sc_flags |= SOC_DMA_F_CLR_CHN_DONE;
        }
            
        /* Start DMA */
        val = soc_pci_read(unit,CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, sc->sc_channel));
        val |= PKTDMA_ENABLE;
        soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, sc->sc_channel), val);

        SDK_CONFIG_MEMORY_BARRIER;

        if (!(sc->sc_flags & SOC_DMA_F_POLL)) {
            (void)soc_cmicm_intr0_enable(unit, IRQ_CMCx_CHAIN_DONE(sc->sc_channel));
        }
    } else
#endif /* BCM_CMICM_SUPPORT */
    {
        /* Set up DMA descriptor address */
        soc_pci_write(unit, CMIC_DMA_DESC(sc->sc_channel),
                      soc_cm_l2p(unit, sc->sc_q->dv_dcb));
        /* Start DMA (Actually starts when the done bit is cleared below) */
        soc_pci_write(unit, CMIC_DMA_STAT,
                      DS_DMA_EN_SET(sc->sc_channel));

        SDK_CONFIG_MEMORY_BARRIER; 

        /*
         * Clear CHAIN_DONE if required, and re-enable the
         * interrupt. This is required to support multiple DMA
         */

        if (sc->sc_flags & SOC_DMA_F_CLR_CHN_DONE) {
            soc_pci_write(unit, CMIC_DMA_STAT,
                          DS_CHAIN_DONE_CLR(sc->sc_channel));
        } else {
            sc->sc_flags |= SOC_DMA_F_CLR_CHN_DONE;
        }
        if (!(sc->sc_flags & SOC_DMA_F_POLL)) {
            (void)soc_intr_enable(unit, IRQ_CHAIN_DONE(sc->sc_channel));
        }
    }
}

/* microsec */
#ifdef BCM_ICS
/* Same time on Quickturn as well */
#define DMA_ABORT_TIMEOUT 10000
#else
#define DMA_ABORT_TIMEOUT \
    (SAL_BOOT_SIMULATION ? 10000000 : 10000)
#endif

#ifdef  BCM_XGS3_SWITCH_SUPPORT
/*
 * Function:
 *      soc_dma_tx_purge
 * Purpose:
 *      Purge the partial packet from the Pipeline left from TX DMA abort.
 * Parameters:
 *      unit         - Unit on which being transmitted
 * Returns:
 *      SOC_E_XXX
 */
int
soc_dma_tx_purge(int unit, dma_chan_t c)
{
    int         rv = SOC_E_NONE;
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
#endif

#ifdef INCLUDE_KNET
    if (SOC_KNET_MODE(unit)) {
        return SOC_E_NONE;
    }
#endif

    if (soc_feature(unit, soc_feature_txdma_purge)) {
        dv_t        dvs;
        uint32      ctrl;
        dv_t        *dv = &dvs;
        dcb_t       *dcb;
        soc_control_t       *soc = SOC_CONTROL(unit);

        if (soc->tx_purge_pkt == NULL) {
            return SOC_E_NONE;
        }
        sal_memset(dv, 0, sizeof(*dv));
        dv->dv_unit    = unit;
        dcb = dv->dv_dcb = soc->tx_purge_pkt;

        dv->dv_op      = DV_TX;
        dv->dv_channel = c;
        dv->dv_cnt     = 1;

        rv = SOC_DCB_ADDTX(unit, dv,
            (sal_vaddr_t) SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, dv->dv_vcnt),
            64,             /* Some Non Zero Lenght */
            PBMP_ALL(unit), /* Not used */
            PBMP_ALL(unit), /* Not used */
            PBMP_ALL(unit), /* Not used */
            SOC_DMA_PURGE,  /* Purge packet */
            NULL            /* No Higig Header required */
            );
            /* Mark the end of the packet */
            SOC_DCB_SG_SET(dv->dv_unit, dcb, 0);
        if (rv >= 0) {
            sal_usecs_t start_time;
            int         diff_time;
            uint32      dma_stat;
            uint32      dma_state; 

            soc_cm_sflush(unit, dv->dv_dcb, SOC_DCB_SIZE(unit));

#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm)) {
                soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, c),
                        soc_pci_read(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc,c)) | PKTDMA_DIRECTION);
                soc_pci_write(unit, CMIC_CMCx_DMA_DESCy_OFFSET(cmc, c),
                              soc_cm_l2p(unit, dv->dv_dcb));
                /* Start DMA */
                soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, c),
                    soc_pci_read(unit,CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc ,c)) | PKTDMA_ENABLE);

                start_time = sal_time_usecs();
                diff_time = 0;
                dma_state = (DS_CMCx_DMA_DESC_DONE(c) | DS_CMCx_DMA_CHAIN_DONE(c));
                do {
                    sal_udelay(SAL_BOOT_SIMULATION ? 1000 : 10);
                    dma_stat = soc_pci_read(unit, CMIC_CMCx_DMA_STAT_OFFSET(cmc)) &
                               (DS_CMCx_DMA_DESC_DONE(c) | DS_CMCx_DMA_CHAIN_DONE(c));
                    diff_time = SAL_USECS_SUB(sal_time_usecs(), start_time);
                    if (diff_time > DMA_ABORT_TIMEOUT) { /* 10 Sec(QT)/10 msec */
                        rv = SOC_E_TIMEOUT;
                        break;
                    } else if (diff_time < 0) {
                        /* Restart in case system time changed */
                        start_time = sal_time_usecs();
                    }
                } while (dma_stat != dma_state);
                /* Clear CHAIN_DONE and DESC_DONE */
                soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, c),
                    soc_pci_read(unit,CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, c)) & ~PKTDMA_ENABLE);

                dma_stat = soc_pci_read(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc));
                soc_pci_write(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc), (dma_stat | DS_DESCRD_CMPLT_CLR(c)));
                soc_pci_write(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc), dma_stat);
            } else
#endif /* CMICM Support */
            {
                ctrl = soc_pci_read(unit, CMIC_DMA_CTRL);
                soc_pci_write(unit, CMIC_DMA_CTRL, ctrl | DC_MEM_TO_SOC(c));
                soc_pci_write(unit, CMIC_DMA_DESC(c),
                              soc_cm_l2p(unit, dv->dv_dcb));
                /* Start DMA */
                soc_pci_write(unit, CMIC_DMA_STAT, DS_DMA_EN_SET(c));

                start_time = sal_time_usecs();
                diff_time = 0;
                dma_state = (DS_DESC_DONE_TST(c) | DS_CHAIN_DONE_TST(c));
                do {
                    sal_udelay(SAL_BOOT_SIMULATION ? 1000 : 10);
                    dma_stat = soc_pci_read(unit, CMIC_DMA_STAT) &
                               (DS_DESC_DONE_TST(c) | DS_CHAIN_DONE_TST(c));
                    diff_time = SAL_USECS_SUB(sal_time_usecs(), start_time);
                    if (diff_time > DMA_ABORT_TIMEOUT) { /* 10 Sec(QT)/10 msec */
                        rv = SOC_E_TIMEOUT;
                        break;
                    } else if (diff_time < 0) {
                        /* Restart in case system time changed */
                        start_time = sal_time_usecs();
                    }
                } while (dma_stat != dma_state);
                /* Clear CHAIN_DONE and DESC_DONE */
                soc_pci_write(unit, CMIC_DMA_STAT, DS_DMA_EN_CLR(c));
                soc_pci_write(unit, CMIC_DMA_STAT, DS_DESC_DONE_CLR(c));
                soc_pci_write(unit, CMIC_DMA_STAT, DS_CHAIN_DONE_CLR(c));
                soc_pci_write(unit, CMIC_DMA_CTRL, ctrl);
            }
            SDK_CONFIG_MEMORY_BARRIER; 
        }
    }
    return rv;
}
#endif  /* BCM_XGS3_SWITCH_SUPPORT */

/*
 * Function:
 *      soc_dma_abort_channel
 * Purpose:
 *      Abort currently active DMA on the specified channel.
 * Parameters:
 *      unit - SOC unit #
 *      channel - channel number to abort.
 * Returns:
 *      SOC_E_NONE   - Success
 *      SOC_E_TIMEOUT - failed (Timeout)
 * Notes:
 *      This routine does not dequeue any operation, only
 *      terminate activity on the specified channel.
 *
 *      No check is made for active operation, but this
 *      routine should succeed regardless. Assumes SOC_DMA_LOCK
 *      held for manipulation of CMIC_DMA_CTRL.
 */

STATIC int
soc_dma_abort_channel(int unit, dma_chan_t channel)
{
    int		        to;
    uint32              ctrl;
    int                 rv = SOC_E_NONE;

    LOG_INFO(BSL_LS_SOC_DMA,
             (BSL_META_U(unit,
                         "soc_dma_abort_channel: c=%d\n"), channel));

#ifdef INCLUDE_KNET
    if (SOC_KNET_MODE(unit)) {
        return soc_knet_hw_reset(unit, channel);
    }
#endif

    /*
     * Write the abort, wait for active to clear, then clear abort.
     */
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        int cmc = SOC_PCI_CMC(unit);
        uint32 val;

        if ((soc_pci_read(unit, CMIC_CMCx_DMA_STAT_OFFSET(cmc)) & DS_CMCx_DMA_ACTIVE(channel)) != 0) {
            ctrl = soc_pci_read(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, channel));
            ctrl |= PKTDMA_ENABLE;
            soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, channel), ctrl);
            soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, channel), ctrl | PKTDMA_ABORT); 
            SDK_CONFIG_MEMORY_BARRIER; 
            
            to = soc_property_get(unit, spn_PDMA_TIMEOUT_USEC, 500000);
            while ((to >= 0) && 
                   ((soc_pci_read(unit, CMIC_CMCx_DMA_STAT_OFFSET(cmc)) & DS_CMCx_DMA_ACTIVE(channel)) != 0)) {
                sal_udelay(1000);
                to -= 1000;
            }
            if ((soc_pci_read(unit, CMIC_CMCx_DMA_STAT_OFFSET(cmc)) & DS_CMCx_DMA_ACTIVE(channel)) != 0) {
                LOG_ERROR(BSL_LS_SOC_DMA,
                          (BSL_META_U(unit,
                                      "soc_dma_abort_channel unit %d: "
                                      "channel %d abort timeout\n"),
                           unit, channel));
                rv = SOC_E_TIMEOUT;
                if (SOC_WARM_BOOT(unit)) {
                    return rv;
                }
            }
        }
        ctrl = soc_pci_read(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, channel));
        ctrl &= ~(PKTDMA_ENABLE|PKTDMA_ABORT); /* clearing enable will also clear CHAIN_DONE */
        soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, channel), ctrl); 

        val = soc_pci_read(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc));
        soc_pci_write(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc), (val | DS_DESCRD_CMPLT_CLR(channel)));
        soc_pci_write(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc), val);
        SDK_CONFIG_MEMORY_BARRIER; 
    } else
#endif /* BCM_CMICM_SUPPORT */
    {
        soc_pci_write(unit, CMIC_DMA_STAT, DS_DMA_EN_CLR(channel));

        SDK_CONFIG_MEMORY_BARRIER; 

        ctrl = soc_pci_read(unit, CMIC_DMA_CTRL);
        assert((ctrl & DC_ABORT_DMA(channel)) == 0);
        soc_pci_write(unit, CMIC_DMA_CTRL, ctrl | DC_ABORT_DMA(channel));

        SDK_CONFIG_MEMORY_BARRIER; 

        to = soc_property_get(unit, spn_PDMA_TIMEOUT_USEC, 500000);

        while ((to >= 0) && 
               ((soc_pci_read(unit, CMIC_DMA_STAT) & DS_DMA_ACTIVE(channel)) != 0)) {
            sal_udelay(1000);
            to -= 1000;
        }

        if ((soc_pci_read(unit, CMIC_DMA_STAT) & DS_DMA_ACTIVE(channel)) != 0) {
            LOG_ERROR(BSL_LS_SOC_DMA,
                      (BSL_META_U(unit,
                                  "soc_dma_abort_channel unit %d: "
                                  "channel %d abort timeout\n"),
                       unit, channel));
            rv = SOC_E_TIMEOUT;
            if (SOC_WARM_BOOT(unit)) {
                return rv;
            }
        }

        soc_pci_write(unit, CMIC_DMA_CTRL, ctrl);
        soc_pci_write(unit, CMIC_DMA_STAT, DS_DESC_DONE_CLR(channel));
        soc_pci_write(unit, CMIC_DMA_STAT, DS_CHAIN_DONE_CLR(channel));
        SDK_CONFIG_MEMORY_BARRIER; 
    }

#ifdef  BCM_XGS3_SWITCH_SUPPORT
    /*
     * Send a purge packet to flush any partial packet in the ingress
     * for TX DMA channel abvort
     */
    if (!(SAL_BOOT_SIMULATION) && (rv >= 0) &&
        (soc_dma_chan_dvt_get(unit, channel) == DV_TX)) {
        rv = soc_dma_tx_purge(unit, channel);
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */

    return rv;
}

/*
 * Function:
 *      soc_dma_abort_dv
 * Purpose:
 *      Abort active DMA operations.
 * Parameters:
 *      unit - unit number to abort DMA on
 *      dv - dv chain to abort.
 * Returns:
 *      SOC_E_NONE - Operation aborted.
 *      SOC_E_TIMEOUT - operation not located, or time-out waiting
 *                      for abort.
 */

int
soc_dma_abort_dv(int unit, dv_t *dv_chain)
{
    uint32        soc_dma_lock;
    soc_control_t *soc = SOC_CONTROL(unit);
    sdc_t         *sc;
    int           rv;

    SOC_DMA_LOCK(soc_dma_lock); /* Be sure nothing changes */
    if ((dv_chain->dv_channel < 0) ||
        (dv_chain->dv_channel >= soc->soc_max_channels)) {
        SOC_DMA_UNLOCK(soc_dma_lock);
        return (SOC_E_TIMEOUT);
    }

    sc = &soc->soc_channels[dv_chain->dv_channel];

    /*
     * Check if active operation. soc_dma_start_channel will update the
     * dv_active field if the operation is active, otherwise it is not
     * required to update it.
     */
    if (sc->sc_q == NULL) {
        rv = SOC_E_TIMEOUT;
    } else if (dv_chain == sc->sc_q) {
        rv = soc_dma_abort_channel(unit, sc->sc_channel);
        sc->sc_q = dv_chain->dv_next;   /* Remove from Queue */
        sc->sc_q_cnt--;                 /* Decrement current Q length */
        /* Updates state and active values */
        soc_dma_start_channel(unit, sc); /* Try to start next operation */
    } else {
        dv_t    *dv;
        /*
         * Look down list for operation, if not found, it may have completed
         * already.
         */
        for (dv = sc->sc_q;
             (dv->dv_next != dv_chain) && (dv->dv_next != NULL);
             dv = dv->dv_next)
            ;
        if (dv->dv_next == dv_chain) {
            dv->dv_next = dv->dv_next->dv_next; /* Off list */
            if (sc->sc_q_tail == dv_chain) { /* was on tail? */
                sc->sc_q_tail = dv;          /* then make previous on tail */
            }
            sc->sc_q_cnt--;
            rv = SOC_E_NONE;
        } else {
            rv = SOC_E_TIMEOUT;
        }
    }

    SOC_DMA_UNLOCK(soc_dma_lock);
    return (rv);
}

/*
 * Function:
 *      soc_dma_abort_channel_total
 * Purpose:
 *      Abort ALL active DMA on the specified channel.
 * Parameters:
 *      unit - SOC unit #
 *      channel - channel number to abort.
 * Returns:
 *      # of operations aborted.
 * Notes:
 *      This routine _does_ dequeue any operations on the channel.
 *
 */
int
soc_dma_abort_channel_total(int unit, dma_chan_t channel)
{
    uint32        soc_dma_lock;
    soc_control_t *soc = SOC_CONTROL(unit);
    sdc_t         *sc;    /* Channel pointer */
    int           rv = 0; /* Return value */

    SOC_DMA_LOCK(soc_dma_lock); /* Be sure nothing changes */
    sc = &soc->soc_channels[channel];
    soc_dma_abort_channel(unit, channel);

    /* Clear up all queued DMA operations */

    while (sc->sc_q != NULL) {
        assert(sc->sc_q->dv_channel >= 0);
        sc->sc_q->dv_channel = -sc->sc_q->dv_channel;
        sc->sc_q = sc->sc_q->dv_next;
        sc->sc_q_cnt--;
        rv++;
    }
    sc->sc_dv_active = NULL;
    sc->sc_q_tail   = NULL;
    assert(sc->sc_q == NULL);
    assert(sc->sc_q_cnt == 0);
    SOC_DMA_UNLOCK(soc_dma_lock);
    return rv;
}

/*
 * Function:
 *      soc_dma_abort
 * Purpose:
 *      Abort ALL dma active on a CMIC.
 * Parameters:
 *      unit - unit #.
 * Returns:
 *      # of operations aborted.
 */

int
soc_dma_abort(int unit)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    dma_chan_t          c;                     /* Channel # */
    int                 rv = 0;        /* Return value */

    for (c = 0; c < soc->soc_max_channels; c++) {
        rv += soc_dma_abort_channel_total(unit, c);
    }
    return (rv);
}

#define DP_FMT  "%sdata[%04x]: "        /* dump line start format */
#define DP_BPL  16                      /* dumped bytes per line */

/*
 * Function:
 *      soc_dma_higig_dump
 * Purpose:
 *      Dump the HIGIG header of a packet in human readable form
 * Parameters:
 *      pfx - prefix for all output strings
 *      addr - pointer to data bytes of packet
 *      len - length of data to dump
 *      ether_offset - (OUT) return offset of start of ether packet
 * Returns:
 *      Nothing
 */

#if defined(BCM_XGS_FABRIC_SUPPORT) || defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_ARAD_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
void
soc_dma_higig_dump(int unit, char *pfx, uint8 *addr, int len, int pkt_len,
                   int *ether_offset)
{
    soc_higig_hdr_t *xgh = (soc_higig_hdr_t *)addr;

    if (soc_higig_field_get(unit, xgh, HG_hgi) == SOC_HIGIG_HGI) {
        if (!len) {
            len = pkt_len + SOC_HIGIG_HDR_SIZE;
        }
        LOG_CLI((BSL_META_U(unit,
                            "%sHIGIG Frame:"
                 " len=%d (header=%d payload=%d)\n"),
                 pfx, len, SOC_HIGIG_HDR_SIZE,
                 len - SOC_HIGIG_HDR_SIZE));
        soc_higig_dump(unit, pfx, xgh);
        LOG_CLI((BSL_META_U(unit,
                            "%s802.3 Ether-II VLAN-Tagged Payload (%d bytes)\n"),
                 pfx, len - SOC_HIGIG_HDR_SIZE));
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
    } else if ((SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit) ||
                        SOC_IS_HURRICANE2(unit) || SOC_IS_KATANA2(unit) ||
                        SOC_IS_GREYHOUND(unit)) &&
               (soc_pbsmh_field_get(unit,
                    (soc_pbsmh_hdr_t *)xgh,
                     PBSMH_start) == SOC_PBSMH_START_INTERNAL)) {

        soc_pbsmh_dump(unit, pfx, (soc_pbsmh_hdr_t *)xgh);
#endif  /* BCM_TRIUMPH3_SUPPORT */
#ifdef  BCM_XGS3_SWITCH_SUPPORT
    } else if ((SOC_IS_XGS3_SWITCH(unit)) &&
               (soc_pbsmh_field_get(unit,
                                    (soc_pbsmh_hdr_t *)xgh,
                                    PBSMH_start) == SOC_PBSMH_START)) {
        soc_pbsmh_dump(unit, pfx, (soc_pbsmh_hdr_t *)xgh);
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
#ifdef BCM_HIGIG2_SUPPORT
    } else if (soc_feature(unit, soc_feature_higig2)) {
        /* Not Higig/Higig+/PBS - assume Higig2 */
        if (!len) {
            len = pkt_len + SOC_HIGIG2_HDR_SIZE;
        }
        LOG_CLI((BSL_META_U(unit,
                            "%sHIGIG2 Frame:"
                 " len=%d (header=%d payload=%d)\n"),
                 pfx, len, SOC_HIGIG2_HDR_SIZE,
                 len - SOC_HIGIG2_HDR_SIZE));
        soc_higig2_dump(unit, pfx, (soc_higig2_hdr_t *)xgh);
        LOG_CLI((BSL_META_U(unit,
                            "%s802.3 Ether-II VLAN-Tagged Payload (%d bytes)\n"),
                 pfx, len - SOC_HIGIG2_HDR_SIZE));
#endif /* BCM_HIGIG2_SUPPORT */
#ifdef BCM_XGS_SUPPORT
    } else if (xgh->overlay1.hgi == SOC_BCM5632_HGI) {
        soc_bcm5632_hdr_t *bhdr = (soc_bcm5632_hdr_t *)addr;
        if (!len) {
            len = pkt_len + SOC_BCM5632_HDR_SIZE;
        }
        /* BCM5632 format */
        LOG_CLI((BSL_META_U(unit,
                            "%sBCM5632 Frame:"
                 " len=%d (header=%d payload=%d)\n"),
                 pfx, len, SOC_BCM5632_HDR_SIZE,
                 len - SOC_BCM5632_HDR_SIZE));
        LOG_CLI((BSL_META_U(unit,
                            "%s0x%02x%02x%02x%02x <D_PORTID=%d>\n"),
                 pfx,
                 bhdr->overlay0.bytes[0],
                 bhdr->overlay0.bytes[1],
                 bhdr->overlay0.bytes[2],
                 bhdr->overlay0.bytes[3],
                 bhdr->overlay1.d_portid));
        LOG_CLI((BSL_META_U(unit,
                            "%s0x%02x%02x%02x%02x <S_PORTID=%d> "
                 "<LEN=%d> START=<0x%x>\n"),
                 pfx,
                 bhdr->overlay0.bytes[4],
                 bhdr->overlay0.bytes[5],
                 bhdr->overlay0.bytes[6],
                 bhdr->overlay0.bytes[7],
                 bhdr->overlay1.s_portid,
                 (bhdr->overlay1.length_hi << 8 |
                 bhdr->overlay1.length_lo),
                 bhdr->overlay1.start));
#endif /* BCM_XGS_SUPPORT */
    }

    /* Calculate ether_offset if given */
    if (ether_offset) {
#ifdef BCM_HIGIG2_SUPPORT
        if (soc_higig_field_get(unit, xgh, HG_start) == SOC_HIGIG2_START) {
            *ether_offset = sizeof(soc_higig2_hdr_t);
        } else 
#endif
        if (soc_higig_field_get(unit, xgh, HG_hgi) == SOC_HIGIG_HGI) {
            *ether_offset = sizeof(soc_higig_hdr_t);
#ifdef BCM_XGS_SUPPORT
        } else if (xgh->overlay1.hgi == SOC_BCM5632_HGI) {
            *ether_offset = sizeof (soc_bcm5632_hdr_t);
#endif /* BCM_XGS_SUPPORT */
        } else {
            *ether_offset = 0;
        }
    }
}
#endif /* defined(BCM_XGS_FABRIC_SUPPORT) || defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) */


void
soc_dma_ether_dump(int unit, char *pfx, uint8 *addr, int len, int offset)
{
    int         i = 0, j;

    if (len > DP_BPL && (DP_BPL & 1) == 0) {
        char    linebuf[128], *s;
        /* Show first line with MAC addresses in curly braces */
        s = linebuf;
        sal_sprintf(s, DP_FMT "{", pfx, i);
        while (*s != 0) s++;
        for (i = offset; i < offset + 6; i++) {
            sal_sprintf(s, "%02x", addr[i]);
            while (*s != 0) s++;
        }
        sal_sprintf(s, "} {");
        while (*s != 0) s++;
        for (; i < offset + 12; i++) {
            sal_sprintf(s, "%02x", addr[i]);
            while (*s != 0) s++;
        }
        sal_sprintf(s, "}");
        while (*s != 0) s++;
        for (; i < offset + DP_BPL; i += 2) {
            sal_sprintf(s, " %02x%02x", addr[i], addr[i + 1]);
            while (*s != 0) s++;
        }
        LOG_CLI((BSL_META_U(unit,
                            "%s\n"), linebuf));
    }

    for (; i < len; i += DP_BPL) {
        char    linebuf[128], *s;
        s = linebuf;
        sal_sprintf(s, DP_FMT, pfx, i);
        while (*s != 0) s++;
        for (j = i; j < i + DP_BPL && j < len; j++) {
            sal_sprintf(s, "%02x%s", addr[j], j & 1 ? " " : "");
            while (*s != 0) s++;
        }
        LOG_CLI((BSL_META_U(unit,
                            "%s\n"), linebuf));
    }

}


/*
 * Function:
 *      soc_dma_dump_pkt
 * Purpose:
 *      Dump packet data in human readable form
 * Parameters:
 *      pfx - prefix for all output strings
 *      addr - pointer to data bytes of packet
 *      len - length of data to dump
 *      decode - assume this is start of packet and decode fields
 * Returns:
 *      Nothing
 */

void
soc_dma_dump_pkt(int unit, char *pfx, uint8 *addr, int len, int decode)
{
    int ether_offset;

    COMPILER_REFERENCE(unit);

    if (!addr || !pfx) {
        LOG_CLI((BSL_META_U(unit,
                            "<ERROR>\n")));
        return;
    }

    if (len == 0) {
        LOG_CLI((BSL_META_U(unit,
                            DP_FMT "<NONE>\n"), pfx, 0));
    }

    ether_offset = 0;

#ifdef BCM_HERCULES_SUPPORT
    /* All hercules chips have module header prepended to frame */
    if (SOC_IS_HERCULES(unit) && decode) {
        soc_dma_higig_dump(unit, pfx, addr, len, 0, &ether_offset);
    }
#endif /* BCM_HERCULES_SUPPORT */

    soc_dma_ether_dump(unit, pfx, addr, len, ether_offset);
}

#undef DP_FMT
#undef DP_BPL


/*
 * Function:
 *      soc_dma_dv_valid
 * Purpose:
 *      Check if a DV is (probably) valid
 * Parameters:
 *      dv - The dv to examine
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Does not guarantee a DV is valid, but will detect
 *      most _invalid_ DVs.
 */

int
soc_dma_dv_valid(dv_t *dv)
{
    if (dv->dv_magic != DV_MAGIC_NUMBER) {
        return FALSE;
    }

    return TRUE;
}


/*
 * Function:
 *      soc_dma_dump_dv
 * Purpose:
 *      Dump a "dv" structure and all the DCB fields.
 * Parameters:
 *      dv_chain - pointer to dv list to dump.
 * Returns:
 *      Nothing.
 */

void
soc_dma_dump_dv(int unit, char *pfx, dv_t *dv_chain)
{
    static char *channel[] = {"0","1","2","3"};
    char        tmps[128];
    char        *op_name;
    int         i;
    dv_t        *dv;
    char *      chan_str;
    int len = 0;
    int len_tmps = 0;

    for (dv = dv_chain; dv; dv = dv->dv_chain) {

        if (!soc_dma_dv_valid(dv)) {
            LOG_CLI((BSL_META_U(unit,
                                "%sdv@%p appears invalid\n"), pfx, (void *)dv));
            break;
        }

        switch(dv_chain->dv_op) {
        case DV_NONE:   op_name = "None";       break;
        case DV_TX:     op_name = "TX";         break;
        case DV_RX:     op_name = "RX";         break;
        default:        op_name = "*ERR*";      break;
        }

        tmps[0] = '\0';
        len_tmps = sal_strlen(tmps);

        if (dv_chain->dv_flags & DV_F_NOTIFY_DSC) {
            len = sal_strlen("notify-dsc ");
            sal_strncpy(tmps + len_tmps , "notify-dsc ", len);
            *(tmps + len_tmps + len) = '\0';
        }
        if (dv_chain->dv_flags & DV_F_NOTIFY_CHN) {
            len = sal_strlen("notify-chn ");
            sal_strncpy(tmps + len_tmps, "notify-chn ", len);
            *(tmps + len_tmps + len) = '\0';
        }
        if (dv_chain->dv_done_packet) {
            len = sal_strlen("notify-pkt ");
            sal_strncpy(tmps + len_tmps, "notify-pkt ", len);
            *(tmps + len_tmps + len) = '\0';
        }
        if (dv_chain->dv_flags & DV_F_COMBINE_DCB) {
            len = sal_strlen("combine-dcb ");
            sal_strncpy(tmps + len_tmps, "combine-dcb ", len);
            *(tmps + len_tmps + len) = '\0';
        }

        LOG_CLI((BSL_META_U(unit,
                            "%sdv@%p unit %d dcbtype-%d op=%s vcnt=%d dcnt=%d "
                 "cnt=%d\n"),
                 pfx, (void *)dv, unit, SOC_DCB_TYPE(unit),
                 op_name, dv->dv_vcnt, dv->dv_dcnt, dv->dv_cnt));
        if (dv->dv_channel == -1) {
            chan_str = "any";
        } else if  (dv->dv_channel < -1 || dv->dv_channel > 3) {
            chan_str = "illegal";
        } else {
            chan_str = channel[dv->dv_channel];
        }
        LOG_CLI((BSL_META_U(unit,
                            "%s    chan=%s chain=%p flags=0x%x-->%s\n"),
                 pfx, chan_str,
                 (void *)dv->dv_chain,
                 dv->dv_flags, tmps));
        LOG_CLI((BSL_META_U(unit,
                            "%s    user1 %p. user2 %p. user3 %p. user4 %p\n"),
                 pfx, dv->dv_public1.ptr, dv->dv_public2.ptr,
                 dv->dv_public3.ptr, dv->dv_public4.ptr));
        if (dv->tx_param.flags != 0) {
            soc_tx_param_t *prm = &dv->tx_param;
            LOG_CLI((BSL_META_U(unit,
                                "%s    tx-param flags 0x%x cos %d sp.sm %d.%d\n"),
                     pfx, prm->flags, prm->cos, prm->src_port,
                     prm->src_mod));
        }
        for (i = 0; i < dv->dv_vcnt; i++) {
            dcb_t         *dcb;
            sal_vaddr_t   addr;

            dcb = SOC_DCB_IDX2PTR(unit, dv->dv_dcb, i);
            addr = SOC_DCB_ADDR_GET(unit, dcb);
            LOG_CLI((BSL_META_U(unit,
                                "%sdcb[%d] @%p:\n"), pfx, i, dcb));
#ifdef BROADCOM_DEBUG
            SOC_DCB_DUMP(dv->dv_unit, dcb, pfx, 
                         (dv_chain->dv_op == DV_TX) ? 1 : 0);
#endif /* BROADCOM_DEBUG */
            if (bsl_check(bslLayerSoc, bslSourcePacket, bslSeverityNormal, unit)) {
                int decode;

                decode = (i == 0 ||
                          !SOC_DCB_SG_GET(dv->dv_unit,
                                          SOC_DCB_IDX2PTR(dv->dv_unit,
                                                          dv->dv_dcb, i - 1)));

                if (dv->dv_op == DV_TX) {
                    soc_dma_dump_pkt(unit, pfx, (uint8 *) addr,
                                     SOC_DCB_REQCOUNT_GET(unit, dcb),
                                     decode);
                } else if (dv->dv_op == DV_RX && SOC_DCB_DONE_GET(unit, dcb)) {
                    soc_dma_dump_pkt(unit, pfx, (uint8 *) addr,
                                     SOC_DCB_XFERCOUNT_GET(unit, dcb),
                                     decode);
                }
            }
        }
    }
}

void
soc_dma_dump_dv_dcb(int unit, char *pfx, dv_t *dv_chain, int index) /* , dcb_t *dcb) */
{
    static char *channel[] = {"0","1","2","3"};
    char        tmps[128];
    char        *op_name;
    int         i;
    dv_t        *dv;
    char *      chan_str;
    int len = 0;
    int len_tmps = 0;

    dv = dv_chain;
    {
        if (!soc_dma_dv_valid(dv)) {
            LOG_CLI((BSL_META_U(unit,
                                "%sdv@%p appears invalid\n"), pfx, (void *)dv));
            return ;
        }
        switch(dv_chain->dv_op) {
        case DV_NONE:   op_name = "None";       break;
        case DV_TX:     op_name = "TX";         break;
        case DV_RX:     op_name = "RX";         break;
        default:        op_name = "*ERR*";      break;
        }
        tmps[0] = '\0';
        len_tmps = sal_strlen(tmps);

        if (dv_chain->dv_flags & DV_F_NOTIFY_DSC) {
            len = sal_strlen("notify-dsc ");
            sal_strncpy(tmps + len_tmps , "notify-dsc ", len);
            *(tmps + len_tmps + len) = '\0';
        }
        if (dv_chain->dv_flags & DV_F_NOTIFY_CHN) {
            len = sal_strlen("notify-chn ");
            sal_strncpy(tmps + len_tmps, "notify-chn ", len);
            *(tmps + len_tmps + len) = '\0';
        }
        if (dv_chain->dv_done_packet) {
            len = sal_strlen("notify-pkt ");
            sal_strncpy(tmps + len_tmps, "notify-pkt ", len);
            *(tmps + len_tmps + len) = '\0';
        }
        if (dv_chain->dv_flags & DV_F_COMBINE_DCB) {
            len = sal_strlen("combine-dcb ");
            sal_strncpy(tmps + len_tmps, "combine-dcb ", len);
            *(tmps + len_tmps + len) = '\0';
        }

        LOG_CLI((BSL_META_U(unit,
                            "%sdv@%p unit %d dcbtype-%d op=%s vcnt=%d dcnt=%d "
                 "cnt=%d\n"),
                 pfx, (void *)dv, unit, SOC_DCB_TYPE(unit),
                 op_name, dv->dv_vcnt, dv->dv_dcnt, dv->dv_cnt));
        if (dv->dv_channel == -1) {
            chan_str = "any";
        } else if  (dv->dv_channel < -1 || dv->dv_channel > 3) {
            chan_str = "illegal";
        } else {
            chan_str = channel[dv->dv_channel];
        }
        LOG_CLI((BSL_META_U(unit,
                            "%s    chan=%s chain=%p flags=0x%x-->%s\n"),
                 pfx, chan_str,
                 (void *)dv->dv_chain,
                 dv->dv_flags, tmps));
        LOG_CLI((BSL_META_U(unit,
                            "%s    user1 %p. user2 %p. user3 %p. user4 %p\n"),
                 pfx, dv->dv_public1.ptr, dv->dv_public2.ptr,
                 dv->dv_public3.ptr, dv->dv_public4.ptr));
        if (dv->tx_param.flags != 0) {
            soc_tx_param_t *prm = &dv->tx_param;
            LOG_CLI((BSL_META_U(unit,
                                "%s    tx-param flags 0x%x cos %d sp.sm %d.%d\n"),
                     pfx, prm->flags, prm->cos, prm->src_port,
                     prm->src_mod));
        }
        i= index;
        {
            dcb_t         *dcb;
            sal_vaddr_t   addr;
            dcb = SOC_DCB_IDX2PTR(unit, dv->dv_dcb, i);
            addr = SOC_DCB_ADDR_GET(unit, dcb);
            LOG_CLI((BSL_META_U(unit,
                                "%sdcb[%d] @%p:\n"), pfx, i, dcb));
#ifdef BROADCOM_DEBUG
            SOC_DCB_DUMP(dv->dv_unit, dcb, pfx, 
                         (dv_chain->dv_op == DV_TX) ? 1 : 0);
#endif /* BROADCOM_DEBUG */
            {
                int decode;
                decode = (i == 0 ||
                          !SOC_DCB_SG_GET(dv->dv_unit,
                                          SOC_DCB_IDX2PTR(dv->dv_unit,
                                                          dv->dv_dcb, i - 1)));
                if (dv->dv_op == DV_TX) {
                    soc_dma_dump_pkt(unit, pfx, (uint8 *) addr,
                                     SOC_DCB_REQCOUNT_GET(unit, dcb),
                                     decode);
                } else if (dv->dv_op == DV_RX && SOC_DCB_DONE_GET(unit, dcb)) {
                    soc_dma_dump_pkt(unit, pfx, (uint8 *) addr,
                                     SOC_DCB_XFERCOUNT_GET(unit, dcb),
                                     decode);
                }
            }
        }
    }
}
/*
 * Function:
 *      soc_dma_start_dv (internal)
 * Purpose:
 *      Main part of soc_dma_start function.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      sc - Pointer to channel structure to start
 *      dv_chain - pointer to DV chain to start.
 * Returns:
 *      Nothing
 */

STATIC void
soc_dma_start_dv(int unit, sdc_t *sc, dv_t *dv_chain)
{
    uint32        soc_dma_lock;
    int           i;
    dv_t          *dv;
    dcb_t         *dcb;

    assert(sc->sc_type == dv_chain->dv_op);
    assert(!(dv_chain->dv_flags & DV_F_NOTIFY_CHN) ||
           dv_chain->dv_done_chain);
    assert(!(dv_chain->dv_flags & DV_F_NOTIFY_DSC) ||
           dv_chain->dv_done_desc);

#if defined(BROADCOM_DEBUG)
    /* Dump out info on request, before queued on channel */
    if (bsl_check(bslLayerSoc, bslSourceDma, bslSeverityNormal, unit)) {
        soc_dma_dump_dv(unit, "dma (before): ", dv_chain);
    }
#endif /* BROADCOM_DEBUG */

    /* Clean cache of any dirty data */

    for (dv = dv_chain; dv; dv = dv->dv_chain) {
        soc_cm_sflush(unit, dv->dv_dcb,
	                      dv->dv_vcnt * SOC_DCB_SIZE(unit));
		
        for (i = 0; i < dv->dv_vcnt; i++) {
            uint32      cnt;
            sal_vaddr_t addr;

            dcb = SOC_DCB_IDX2PTR(unit, dv->dv_dcb, i);
            cnt = SOC_DCB_REQCOUNT_GET(unit, dcb);
            addr = SOC_DCB_ADDR_GET(unit, dcb);
            if (dv_chain->dv_op == DV_TX) {
                soc_cm_sflush(unit, (void *)addr, cnt);
            } else {
                assert(dv_chain->dv_op == DV_RX);
            }
        }
    }

    /* Mark Channel # */

    dv_chain->dv_channel = sc->sc_channel;

    SOC_DMA_LOCK(soc_dma_lock);
    dv_chain->dv_next = NULL;
    if (sc->sc_q_cnt != 0) {
        sc->sc_q_tail->dv_next = dv_chain;
    } else {
        sc->sc_q = dv_chain;
    }
    sc->sc_q_tail = dv_chain;
    sc->sc_q_cnt++;
    if (sc->sc_dv_active == NULL) {     /* Start DMA if channel not active */
        soc_dma_start_channel(unit, sc);
    }
    SOC_DMA_UNLOCK(soc_dma_lock);

#ifdef INCLUDE_KNET
    if (SOC_KNET_MODE(unit)) {
        kcom_msg_dma_info_t kmsg;
        int wsize = BYTES2WORDS(SOC_DCB_SIZE(unit));
        int len;

        for (dv = dv_chain; dv; dv = dv->dv_chain) {
            sal_memset(&kmsg, 0, sizeof(kmsg));
            kmsg.hdr.opcode = KCOM_M_DMA_INFO;
            kmsg.hdr.unit = unit;
            if (dv_chain->dv_op == DV_TX) {
                kmsg.dma_info.type = KCOM_DMA_INFO_T_TX_DCB;
            } else {
                kmsg.dma_info.type = KCOM_DMA_INFO_T_RX_DCB;
            }
            kmsg.dma_info.cnt = dv->dv_vcnt;
            kmsg.dma_info.size = wsize;
            dcb = SOC_DCB_IDX2PTR(unit, dv->dv_dcb, 0);
            kmsg.dma_info.data.dcb_start = soc_cm_l2p(unit, dcb);
            len = sizeof(kmsg);
            soc_knet_cmd_req((kcom_msg_t *)&kmsg, len, sizeof(kmsg));
        }
    }
#endif
}

/*
 * Function:
 *      soc_dma_start
 * Purpose:
 *      Launch a SOC_DMA DMA operation.
 * Parameters:
 *      unit - unit number.
 *      channel - DMA channel to use (0-3) or -1 if any
 *                      channel ok.
 *      dv_chain - dma request description.
 * Returns:
 *      SOC_E_NONE - operation started.
 *      SOC_E_TIMEOUT - operation failed to be queued.
 */

int
soc_dma_start(int unit, dma_chan_t channel, dv_t *dv_chain)
{
    sdc_t       *sc;                    /* Channel pointer */

    /* Fire off DMA operation, let interrupt handler free channel */

    sc = soc_dma_channel(unit, channel, dv_chain);
    if (sc == NULL) {
        return(SOC_E_RESOURCE);
    }

    soc_dma_start_dv(unit, sc, dv_chain);
    return(SOC_E_NONE);
}

STATIC void
soc_dma_free_list(int unit);

/*
 * Function:
 *      soc_dma_dv_alloc
 * Purpose:
 *      Allocate and initialize a dv struct.
 * Parameters:
 *      op - operations iov requested for.
 *      cnt - number of DCBs required.
 * Notes:
 *      If a DV on the free list will accomodate the request,
 *      satisfy it from there to avoid extra alloc/free calls.
 */

dv_t *
soc_dma_dv_alloc(int unit, dvt_t op, int cnt)
{
    uint32        soc_dma_lock;
    soc_control_t *soc = SOC_CONTROL(unit);
    dv_t          *dv;
    uint32        dmabuf_size;
    int           *dv_free_cnt;
    dv_t          **dv_free;
    void          *info = NULL;

    assert(cnt > 0);    

    /* Always bump up DCB count to free list size */

    if (cnt < soc->soc_dv_size) {
        cnt = soc->soc_dv_size;
    }

    /* Check if we can use one off the free list */

    SOC_DMA_LOCK(soc_dma_lock);
    if (op == DV_TX) {
    	dv_free_cnt = &(soc->soc_dv_tx_free_cnt);
    	dv_free = &(soc->soc_dv_tx_free);    	
    } else if (op == DV_RX) {
    	dv_free_cnt = &(soc->soc_dv_rx_free_cnt);
    	dv_free = &(soc->soc_dv_rx_free);
    } else {
    	SOC_DMA_UNLOCK(soc_dma_lock);  
    	return NULL;
    }
    soc->stat.dv_alloc++;
    if ((cnt == soc->soc_dv_size) && ((*dv_free_cnt) > 0)) {
        dv = *(dv_free);
        *(dv_free) = dv->dv_chain;
        (*dv_free_cnt)--;
        soc->stat.dv_alloc_q++;
        SOC_DMA_UNLOCK(soc_dma_lock);        
        info = dv->dv_public1.ptr;
    } else {
        SOC_DMA_UNLOCK(soc_dma_lock);
        dv = soc_at_alloc(unit, sizeof(dv_t), "soc_dma_dv_alloc");
        if (dv == NULL) {
            /* to release the free list */
        	soc_dma_free_list(unit);
        	dv = soc_at_alloc(unit, sizeof(dv_t), "soc_dma_dv_alloc");
        	if (dv == NULL) {
	            return(NULL);
	        }
        }
        dmabuf_size = SOC_DV_DMABUF_SIZE;
        dv->dv_dma_buf_size = dmabuf_size;
        /* Optionally allocate one word per DCB for Tx alignment */
        if (soc_feature(unit, soc_feature_pkt_tx_align)) {
            dmabuf_size += 4 * cnt;
        }
        dv->dv_dmabuf = soc_cm_salloc(unit, dmabuf_size,
                                      "sdma_dmabuf_alloc");
        if (dv->dv_dmabuf == NULL) {
            /* to release the free list */
        	soc_dma_free_list(unit);
        	dv->dv_dmabuf = soc_cm_salloc(unit, dmabuf_size,
                                      "sdma_dmabuf_alloc");
            if (dv->dv_dmabuf == NULL) {
                soc_at_free(unit, dv);
                return(NULL);
            }
        }
        dv->dv_dcb = soc_cm_salloc(unit, SOC_DCB_SIZE(unit) * cnt,
                                   "sdma_dcb_alloc");
        if (dv->dv_dcb == NULL) {
            /* to release the free list */
        	soc_dma_free_list(unit);
        	dv->dv_dcb = soc_cm_salloc(unit, SOC_DCB_SIZE(unit) * cnt,
                                   "sdma_dcb_alloc");
            if (dv->dv_dcb == NULL) {
                soc_cm_sfree(unit, dv->dv_dmabuf);
                soc_at_free(unit, dv);
                return(NULL);
            }
        }
        dv->dv_unit = unit;
        dv->dv_cnt = cnt;
        dv->dv_flags = ((op == DV_TX) ? DV_F_COMBINE_DCB : 0);	   
	}
	
	dv->dv_done_packet = NULL;
	dv->dv_done_desc = NULL;
	dv->dv_done_chain = NULL;
	dv->dv_magic = DV_MAGIC_NUMBER;
	soc_dma_dv_reset(op, dv);   /* Reset standard fields */
    dv->dv_public1.ptr = info;
    return(dv);
}

/*
 * Function:
 *      soc_dma_dv_alloc_by_port
 * Purpose:
 *      Allocate and initialize a dv struct.
 * Parameters:
 *      op - operations iov requested for.
 *      cnt - number of DCBs required.
 *      pkt_to_ports - number of ports to send to (specify if 
 *                     sending more than SOC_DV_PKTS_MAX)
 * Notes:
 *      If a DV on the free list will accomodate the request,
 *      satisfy it from there to avoid extra alloc/free calls.
 */

dv_t *
soc_dma_dv_alloc_by_port(int unit, dvt_t op, int cnt, int pkt_to_ports)
{
    uint32        soc_dma_lock;
    soc_control_t *soc = SOC_CONTROL(unit);
    dv_t          *dv;
    uint32        dmabuf_size;
    int           *dv_free_cnt;
    dv_t          **dv_free;
    void          *info = NULL;
        
    if (pkt_to_ports <= 0) { /* not specified */
        return soc_dma_dv_alloc(unit, op, cnt);
    }
    assert(cnt > 0);

    /* Always bump up DCB count to free list size */

    if (cnt < soc->soc_dv_size) {
        cnt = soc->soc_dv_size;
    }    
    if (pkt_to_ports < SOC_DV_PKTS_MAX) {
    	pkt_to_ports = SOC_DV_PKTS_MAX;
    }
    /* Check if we can use one off the free list */

    SOC_DMA_LOCK(soc_dma_lock);
    if (op == DV_TX) {
    	dv_free_cnt = &(soc->soc_dv_tx_free_cnt);
    	dv_free = &(soc->soc_dv_tx_free);
    } else if (op == DV_RX) {
    	dv_free_cnt = &(soc->soc_dv_rx_free_cnt);
    	dv_free = &(soc->soc_dv_rx_free);
    } else {
    	SOC_DMA_UNLOCK(soc_dma_lock);
    	return NULL;
    }
    soc->stat.dv_alloc++;
    if ((cnt == soc->soc_dv_size) && 
    	((*dv_free_cnt) > 0) && 
    	(pkt_to_ports == SOC_DV_PKTS_MAX)) {
        dv = (*dv_free);
        (*dv_free) = dv->dv_chain;
        (*dv_free_cnt)--;
        soc->stat.dv_alloc_q++;
        SOC_DMA_UNLOCK(soc_dma_lock);
        info = dv->dv_public1.ptr;
    } else {
        SOC_DMA_UNLOCK(soc_dma_lock);
        dv = soc_at_alloc(unit, sizeof(dv_t), "soc_dma_dv_alloc");
        if (dv == NULL) {
        	/* to release the free list */
        	soc_dma_free_list(unit);
        	dv = soc_at_alloc(unit, sizeof(dv_t), "soc_dma_dv_alloc");
        	if (dv == NULL) {
	            return(NULL);
	        }
        } 
        dmabuf_size = pkt_to_ports * SOC_DMA_BUF_LEN;
        dv->dv_dma_buf_size = dmabuf_size;
        /* Optionally allocate one word per DCB for Tx alignment */
        if (soc_feature(unit, soc_feature_pkt_tx_align)) {
            dmabuf_size += 4 * cnt;
        }
        dv->dv_dmabuf = soc_cm_salloc(unit, dmabuf_size,
                                      "sdma_dmabuf_alloc");
        if (dv->dv_dmabuf == NULL) {
        	/* to release the free list */
        	soc_dma_free_list(unit);
        	dv->dv_dmabuf = soc_cm_salloc(unit, dmabuf_size,
                                      "sdma_dmabuf_alloc");
            if (dv->dv_dmabuf == NULL) {
                soc_at_free(unit, dv);
                return(NULL);
            }
        }
        dv->dv_dcb = soc_cm_salloc(unit, SOC_DCB_SIZE(unit) * cnt,
                                   "sdma_dcb_alloc");
        if (dv->dv_dcb == NULL) {
        	/* to release the free list */
        	soc_dma_free_list(unit);
        	dv->dv_dcb = soc_cm_salloc(unit, SOC_DCB_SIZE(unit) * cnt,
                                   "sdma_dcb_alloc");
            if (dv->dv_dcb == NULL) {
                soc_cm_sfree(unit, dv->dv_dmabuf);
                soc_at_free(unit, dv);
                return(NULL);
            }
        }
        dv->dv_unit = unit;
        dv->dv_cnt = cnt;
        dv->dv_flags = ((op == DV_TX) ? DV_F_COMBINE_DCB : 0);       	
    }
	dv->dv_done_packet = NULL;
	dv->dv_done_desc = NULL;
	dv->dv_done_chain = NULL;
	dv->dv_magic = DV_MAGIC_NUMBER;
	soc_dma_dv_reset(op, dv);   /* Reset standard fields */
    dv->dv_public1.ptr = info;
    return(dv);
}

/*
 * Function:
 *      soc_dma_dv_free
 * Purpose:
 *      Free a dv struct.
 * Parameters:
 *      dv - pointer to dv to free (NOT a dv chain).
 * Returns:
 *      Nothing.
 */

void
soc_dma_dv_free(int unit, dv_t *dv)
{
    uint32        soc_dma_lock;
    soc_control_t *soc = SOC_CONTROL(unit);
    int           *dv_free_cnt;
    dv_t          **dv_free;
    
 
    SOC_DMA_LOCK(soc_dma_lock);
    if (dv->dv_op == DV_TX) {
    	dv_free_cnt = &(soc->soc_dv_tx_free_cnt);
    	dv_free = &(soc->soc_dv_tx_free);
    } else {
    	dv_free_cnt = &(soc->soc_dv_rx_free_cnt);
    	dv_free = &(soc->soc_dv_rx_free);
    } 
    
    soc->stat.dv_free++;
    assert(dv->dv_magic == DV_MAGIC_NUMBER);
    if ((dv->dv_cnt == soc->soc_dv_size) && 
        ((*dv_free_cnt) < soc->soc_dv_cnt)) {
        assert(dv);
        assert(dv->dv_dcb);
        dv->dv_chain = (*dv_free);
        (*dv_free) = dv;
        (*dv_free_cnt)++;
        SOC_DMA_UNLOCK(soc_dma_lock);
    } else {
    	dv->dv_magic = 0;
        SOC_DMA_UNLOCK(soc_dma_lock);
        if (dv->dv_dcb) {
            soc_cm_sfree(unit, dv->dv_dcb);
        }
        soc_cm_sfree(unit, dv->dv_dmabuf);
        soc_at_free(unit, dv);
    }
}

/*
 * Function:
 *      soc_dma_process_done_desc
 * Purpose:
 *      Process and make callouts for completed descriptors.
 * Parameters:
 *      unit - SOC unit number
 *      dv_chain - pointer to currently active dv chain
 *      dv - pointer to next individual dv to process (MAY BE NULL).
 * Returns:
 *      Pointer to next descriptor that is not yet complete,
 *      NULL if list is complete.
 * Notes:
 *      This is done at INTERRUPT LEVEL so we know no more
 *      completion interrupts will arrive during processing.
 */

STATIC dv_t *
soc_dma_process_done_desc(int unit, dv_t *dv_chain, dv_t *dv)
{
    dcb_t       *dcb;
    int         i;
    int         tx = (dv_chain->dv_op == DV_TX);
    soc_stat_t  *stat = &SOC_CONTROL(unit)->stat;
    uint32      flags, count;

    for (; dv != NULL; dv = dv->dv_chain) {
        /* Process all DCBs in the current DV. */

        /* Clean cache of any dirty data */
        soc_cm_sinval(unit, dv->dv_dcb,
                      dv->dv_vcnt * SOC_DCB_SIZE(unit));

        for (i = dv->dv_dcnt; i < dv->dv_vcnt; i++) {
            dcb = SOC_DCB_IDX2PTR(unit, dv->dv_dcb, i);
            flags = SOC_DCB_INTRINFO(unit, dcb, tx, &count);
#ifdef INCLUDE_KNET
            if (SOC_KNET_MODE(unit) && !tx) {
                if ((count & SOC_DCB_KNET_DONE) == 0) {
                    /* Processing not complete in kernel */
                    dv->dv_dcnt = i;
                    return(dv);
                }
                /* Mask off indicator for kernel processing done */
                count &= ~SOC_DCB_KNET_DONE;
            }
#endif
            if (flags) {
                if ((dv_chain->dv_flags & DV_F_NOTIFY_DSC) &&
                    dv_chain->dv_done_desc) {
                    dv_chain->dv_done_desc(unit, dv, dcb);
                }

                if (tx) {
                    stat->dma_tbyt += count;
                    if (flags & SOC_DCB_INFO_PKTEND) {
                        if (dv_chain->dv_done_packet) {
                            dv_chain->dv_done_packet(unit, dv, dcb);
                        }
                        stat->dma_tpkt++;
                    }
                } else {
                    sal_vaddr_t addr = SOC_DCB_ADDR_GET(unit, dcb);
                    /* Clean cache lines associated with Rx buffer */
                    soc_cm_sinval(unit, (void *)addr, count);

                    #define MIN_PAYLOAD_SIZE (64-4-12) /* 4:VLAN, 12: DA+SA */
                    stat->dma_rbyt += count;
                    if (flags & SOC_DCB_INFO_PKTEND) {
                        if (dv_chain->dv_done_packet) {
                            dv_chain->dv_done_packet(unit, dv, dcb);
                        }
                        stat->dma_rpkt++;
                    }
                    /* pkt size > dma_len & !pkt_end */
                    else if (count >= MIN_PAYLOAD_SIZE) {
                        if (dv_chain->dv_done_packet) {
                            dv_chain->dv_done_packet(unit, dv, dcb);
                        }
                    }
                }
                dv->dv_dcnt = i + 1;
            } else {
                dv->dv_dcnt = i;
                return(dv);
            }
        }
    }

    return(dv);
}

/*
 * Function:
 *      soc_dma_done_desc
 * Purpose:
 *      Process a completion of a DMA descriptor.
 * Parameters:
 *      unit - soc unit
 *      c - channel number.
 * Returns:
 *      Nothing
 * Notes:
 *      INTERRUPT LEVEL ROUTINE
 */

void
soc_dma_done_desc(int unit, uint32 chan)
{
    dma_chan_t          c = (dma_chan_t)chan;
    soc_control_t       *soc = SOC_CONTROL(unit);
    sdc_t               *sc = &soc->soc_channels[c];
    dv_t                *dv_chain = sc->sc_q;   /* Pick up request */
    dv_t                *dv_active = sc->sc_dv_active;
#ifdef BCM_CMICM_SUPPORT
    uint32 val;
    int cmc = SOC_PCI_CMC(unit);
#endif

#ifdef INCLUDE_KNET
    if (SOC_KNET_MODE(unit)) {
        return;
    }
#endif

    assert(sc->sc_q_cnt);               /* Be sure there is at least one */
    assert(dv_chain);
    assert(dv_active);

    soc->stat.intr_desc++;

    /* Clear interrupt */

#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        val = soc_pci_read(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc));
        soc_pci_write(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc), (val | DS_DESCRD_CMPLT_CLR(c)));
        soc_pci_write(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc), val);
        /*  
         * Flush posted writes from PCI bridge
         */
        (void)soc_pci_read(unit, CMIC_CMCx_DMA_STAT_OFFSET(cmc));
    } else
#endif
    {
        soc_pci_write(unit, CMIC_DMA_STAT, DS_DESC_DONE_CLR(c));

        /*  
         * Flush posted writes from PCI bridge
         */
        (void)soc_pci_read(unit, CMIC_DMA_STAT);
    }

    /*
     * Reap done descriptors, and possibly notify callers of descriptor
     * completion.
     */
    sc->sc_dv_active =
          soc_dma_process_done_desc(unit, dv_chain, dv_active);
}

/*
 * Function:
 *      soc_dma_done_chain
 * Purpose:
 *      Process a completion of a DMA operation.
 * Parameters:
 *      s - pointer to socket structure.
 *      c - channel number.
 * Returns:
 *      Nothing
 * Notes:
 *      INTERRUPT LEVEL ROUTINE
 */

void
soc_dma_done_chain(int unit, uint32 chan)
{
    dma_chan_t    c = (dma_chan_t)chan;
    uint32        soc_dma_lock;
    soc_control_t *soc = SOC_CONTROL(unit);
    sdc_t         *sc = &soc->soc_channels[c];
    dv_t          *dv_chain;  /* Head of current chain */
    dv_t          *dv_active; /* Current DV needed if DV chaining */
    sdc_intr_mitigate_t *sdc_mtg;
    int            diff;

#ifdef INCLUDE_KNET
    if (SOC_KNET_MODE(unit)) {
        return;
    }
#endif

    sdc_mtg = &soc_dma_intr_mitigation[unit].chans_intr_mitigation[c];
    if (sdc_mtg->thread_running) {
        if (sdc_mtg->mitigation_started) {
            sdc_mtg->rx_chain_intr++;
        } else {
            sdc_mtg->cur_tm = sal_time_usecs();
            diff = SAL_USECS_SUB(sdc_mtg->cur_tm, sdc_mtg->prev_tm);
            if ((diff < DMA_CHAIN_DONE_INTERVAL_USEC) && (diff > 0)) {
                sdc_mtg->mitigation_started = 1;
                sdc_mtg->cur_tm = sdc_mtg->prev_tm = 0;
                sal_sem_give(sdc_mtg->thread_sem);
            } else {
                sdc_mtg->prev_tm = sdc_mtg->cur_tm;
            }
        }
    }

    SOC_DMA_LOCK(soc_dma_lock);

    assert(sc->sc_q_cnt > 0);           /* Be sure there is at least one */
    assert(sc->sc_q != NULL);           /* And a pointer too */

    soc->stat.intr_chain++;

    dv_chain    = sc->sc_q;             /* Pick up request */
    dv_active   = sc->sc_dv_active;     /* DV in processing - MAY BE NULL */
    sc->sc_q    = dv_chain->dv_next;    /* Unlink current DV */
    sc->sc_q_cnt--;                     /* Decrement */

    SOC_DMA_UNLOCK(soc_dma_lock);

    /*
     * To support multiple DMA on platforms that support it, the
     * CHAIN_DONE bit must be left pending until the start of the next
     * operation. Here we clear DESC_DONE, and mask the CHAIN_DONE
     * interrupt. CHAIN_DONE is cleared when we start the next
     * operation.
     */
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        int cmc = SOC_PCI_CMC(unit);
        uint32 val;   
        if (sdc_mtg->mitigation_started) {
            (void)soc_cmicm_intr0_disable(unit, 
                       IRQ_CMCx_DESC_DONE(c) | IRQ_CMCx_CHAIN_DONE(c));
        } else {
            (void)soc_cmicm_intr0_disable(unit, IRQ_CMCx_CHAIN_DONE(c));
        }
        val = soc_pci_read(unit,CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc,c));
        val &= ~PKTDMA_ENABLE;
        soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc,c), val);

        val = soc_pci_read(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc));
        soc_pci_write(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc), (val | DS_DESCRD_CMPLT_CLR(c)));
        soc_pci_write(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc), val);
        /*
         * Flush posted writes from PCI bridge
         */
        (void)soc_pci_read(unit, CMIC_CMCx_DMA_STAT_OFFSET(cmc));
    } else
#endif /* BCM_CMICM_SUPPORT */
    {
        if (sdc_mtg->mitigation_started) {
            (void)soc_intr_disable(unit, IRQ_DESC_DONE(c) | IRQ_CHAIN_DONE(c)); 
        } else {
            (void)soc_intr_disable(unit, IRQ_CHAIN_DONE(c));
        }

        soc_pci_write(unit, CMIC_DMA_STAT, DS_DMA_EN_CLR(c));
        soc_pci_write(unit, CMIC_DMA_STAT, DS_DESC_DONE_CLR(c));

        /*
         * Flush posted writes from PCI bridge
         */
        (void)soc_pci_read(unit, CMIC_DMA_STAT);
    }

    soc_dma_start_channel(unit, sc);

    soc_dma_process_done_desc(unit, dv_chain, dv_active);

    assert(dv_chain->dv_dcnt == dv_chain->dv_vcnt); 

    if (dv_chain->dv_flags & DV_F_NOTIFY_CHN) {
        if (dv_chain->dv_done_chain == NULL) {
            LOG_WARN(BSL_LS_SOC_DMA,
                     (BSL_META_U(unit,
                                 "_soc_dma_done_chain: NULL callback: "
                                 "unit=%d chain=%p\n"),
                      unit, (void *)dv_chain));
        } else {
            dv_chain->dv_done_chain(unit, dv_chain);
        }
    }
}

/*
 * Function:
 *      soc_dma_desc_end_packet
 * Purpose:
 *      Mark last descriptor in dv as end of packet.
 * Parameters:
 *      dv - pointer to DMA I/O Vector to be affected.
 * Returns:
 *      Nothing
 */

void
soc_dma_desc_end_packet(dv_t *dv)
{
    dcb_t       *d;

    if (dv->dv_vcnt > 0) {
        d = SOC_DCB_IDX2PTR(dv->dv_unit, dv->dv_dcb, dv->dv_vcnt - 1);
        SOC_DCB_SG_SET(dv->dv_unit, d, 0);
    }
}

/* Initialize basic port bitmaps and cos/crc (used to be flags) for
 * the SOC packet structure in the given DV.
 */
void
soc_dma_dv_pkt_init(dv_t *dv, soc_pbmp_t pbmp, soc_pbmp_t ut_pbmp,
                    soc_pbmp_t l3_pbmp, int cos, int crc)
{
    sal_memset(&dv->tx_param, 0, sizeof(soc_tx_param_t));
    dv->tx_param.cos = cos;
    dv->tx_param.prio_int = cos;  /* By default, same as COS */
    if (crc) {
        dv->tx_param.flags |= SOC_DMA_CRC_REGEN;
    }
    SOC_PBMP_ASSIGN(dv->tx_param.tx_pbmp, pbmp);
    SOC_PBMP_ASSIGN(dv->tx_param.tx_upbmp, ut_pbmp);
    SOC_PBMP_ASSIGN(dv->tx_param.tx_l3pbmp, l3_pbmp);

    dv->tx_param.src_mod = SOC_DEFAULT_DMA_SRCMOD_GET(dv->dv_unit);
    dv->tx_param.src_port = SOC_DEFAULT_DMA_SRCPORT_GET(dv->dv_unit);
    dv->tx_param.pfm = SOC_DEFAULT_DMA_PFM_GET(dv->dv_unit);
}

/*
 * Function:
 *      soc_dma_desc_add
 * Purpose:
 *      Add a DMA descriptor to a DMA chain independent of the
 *      descriptor type.
 * Parameters:
 *      dv - pointer to DMA I/O Vector to be filled in.
 *      addr/cnt/pbmp/ubmp/l3pbm - values to add to the DMA chain.
 *      flags - COS/CRC/ etc.
 * Returns:
 *      < 0 - SOC_E_XXXX error code
 *      >= 0 - # entries left that may be filled.
 * Notes:
 *      Calls the specific fastpath routine if it can, defaulting
 *      to a general routine.
 */

int
soc_dma_desc_add(dv_t *dv, sal_vaddr_t addr, uint16 cnt, pbmp_t pbmp,
                 pbmp_t ubmp, pbmp_t l3pbm, uint32 flags, uint32 *hgh)
{
    if (dv->dv_op == DV_TX) {
        return SOC_DCB_ADDTX(dv->dv_unit, dv, addr, cnt,
                                pbmp, ubmp, l3pbm, flags, hgh);
    } else {
        return SOC_DCB_ADDRX(dv->dv_unit, dv, addr, cnt, flags);
    }
}

/*
 * Function:
 *      soc_dma_rld_desc_add
 * Puporse:
 *      Append a RELOAD descriptor to the end of the current chain.
 * Parameters:
 *      dt - DCB type.
 *      dv - pointer to DV to add descriptor to.
 * Returns:
 *      < 0 - SOC_E_XXXX error code
 *      >= 0 - # entries left that may be filled.
 */

int
soc_dma_rld_desc_add(dv_t *dv, sal_vaddr_t addr)
{
    int         unit;
    dcb_t       *d;

    if (dv->dv_vcnt == dv->dv_cnt) {
        return SOC_E_FULL;
    }

    unit = dv->dv_unit;
    if (dv->dv_vcnt > 0) {              /* Set chain bit in previous */
        d = SOC_DCB_IDX2PTR(unit, dv->dv_dcb, dv->dv_vcnt - 1);
        SOC_DCB_CHAIN_SET(unit, d, 1);
    }

    d = SOC_DCB_IDX2PTR(unit, dv->dv_dcb, dv->dv_vcnt);
    SOC_DCB_INIT(unit, d);
    SOC_DCB_RELOAD_SET(unit, d, 1);
    SOC_DCB_ADDR_SET(unit, d, addr);

    dv->dv_vcnt++;
    return (dv->dv_cnt - dv->dv_vcnt);
}

/*
 * Function:
 *      soc_dma_dv_join
 * Purpose:
 *      Append src_chain to the end of dv_chain
 * Parameters:
 *      dv_chain - pointer to DV chain to be appended to.
 *      src_chain - pointer to DV chain to add.
 * Returns:
 *      SOC_E_NONE - success
 *      SOC_E_XXXX - error code.
 * Note:
 *      src_chain is consumed and should not be further referenced.
 *      If the last DCB in the chained list has the S/G
 *      bit set, then the S/G bit is set in the RLD dcb.
 *      The notification routines MUST be the same in all
 *      elements of the list (this condition is asserted)
 */

int
soc_dma_dv_join(dv_t *dv_chain, dv_t *src_chain)
{
    dcb_t       *d;
    int         unit;

    assert(dv_chain);
    assert(src_chain);

    unit = dv_chain->dv_unit;

    /* Go to last dv in chain */

    while (dv_chain->dv_chain) {
        dv_chain = dv_chain->dv_chain;
    }

    /* Make sure there's room for a reload descriptor */

    if (dv_chain->dv_vcnt == dv_chain->dv_cnt) {
        return SOC_E_FULL;
    }

    /* Ensure the same notification callbacks are used */

    assert(dv_chain->dv_done_chain == src_chain->dv_done_chain);
    assert(dv_chain->dv_done_desc == src_chain->dv_done_desc);

    /* Add reload descriptor */

    d = SOC_DCB_IDX2PTR(unit, dv_chain->dv_dcb, dv_chain->dv_vcnt);

    SOC_DCB_INIT(unit, d);
    SOC_DCB_ADDR_SET(src_chain->dv_unit, d, (sal_vaddr_t)src_chain->dv_dcb);
    SOC_DCB_RELOAD_SET(unit, d, 1);             /* Reload */
    SOC_DCB_CHAIN_SET(unit, d, 1);              /* Chain */

    /* Set chain bit in last DCB */

    if (dv_chain->dv_vcnt > 0) {
        d = SOC_DCB_IDX2PTR(unit, dv_chain->dv_dcb, dv_chain->dv_vcnt - 1);
        SOC_DCB_CHAIN_SET(unit, d, 1);
    }

    dv_chain->dv_vcnt++;                /* Increment valid count */

    /* Concatenate new chain */

    dv_chain->dv_chain = src_chain;

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_dma_dv_reset
 * Purpose:
 *      Reinitialize a dv struct to avoid free/alloc to reuse it.
 * Parameters:
 *      op - operation type requested.
 * Returns:
 *      Nothing.
 */

void
soc_dma_dv_reset(dvt_t op, dv_t *dv)
{
    uint32 soc_dma_lock;

    SOC_DMA_LOCK(soc_dma_lock);
    dv->dv_op      = op;
    dv->dv_channel = -1;
    dv->dv_vcnt    = 0;
    dv->dv_dcnt    = 0;
    /* don't clear all flags */
    dv->dv_flags   &= ~(DV_F_NOTIFY_DSC | DV_F_NOTIFY_CHN);
    dv->dv_chain   = NULL;
    dv->dv_public1.ptr = NULL;
    dv->dv_public2.ptr = NULL;
    dv->dv_public3.ptr = NULL;
    dv->dv_public4.ptr = NULL;
    sal_memset(&dv->tx_param, 0, sizeof(soc_tx_param_t));
    SOC_DMA_UNLOCK(soc_dma_lock);
}

/*
 * Function:
 *      soc_dma_chan_config
 * Purpose:
 *      Configure a particular DMA channel for RX/TX, and configure
 *      appropriate modes.
 * Parameters:
 *      unit - SOC unit #.
 *      c - channel #
 *      type - DV_TX/DV_RX
 *      flags - SOC_DMA_F_XXX
 * Returns:
 *      SOC_E_NONE - success
 *      SOC_E_XXXX - error
 */

int
soc_dma_chan_config(int unit, dma_chan_t c, dvt_t type, uint32 flags)
{
    uint32        soc_dma_lock;
    soc_control_t *soc = SOC_CONTROL(unit);
    sdc_t         *sc = &soc->soc_channels[c];
    uint32        bits, cr;

    /* Define locals and turn flags into easy to use things */
    int         f_mbm = (flags & SOC_DMA_F_MBM) != 0;
    int         f_interrupt = (flags & SOC_DMA_F_POLL) == 0;
    int         f_drop = (flags & SOC_DMA_F_TX_DROP) != 0;
    int         f_default = (flags & SOC_DMA_F_DEFAULT) != 0;
    int         f_desc_intr = (flags & SOC_DMA_F_INTR_ON_DESC) != 0;

    LOG_INFO(BSL_LS_SOC_DMA,
             (BSL_META_U(unit,
                         "soc_dma_chan_config: c=%d type=%d\n"), c, type));

    assert(c >= 0 && c < soc->soc_max_channels);
    assert(!(flags & ~(SOC_DMA_F_MBM | SOC_DMA_F_POLL | SOC_DMA_F_INTR_ON_DESC |
                       SOC_DMA_F_TX_DROP | SOC_DMA_F_DEFAULT)));

    SOC_DMA_LOCK(soc_dma_lock);

    if ((DV_NONE != sc->sc_type) && (sc->sc_q_cnt != 0)) {
        SOC_DMA_UNLOCK(soc_dma_lock);
        return SOC_E_BUSY;    /* Return busy if operations pending */
    }

#ifdef INCLUDE_KNET
    if (SOC_KNET_MODE(unit)) {
        /* Interrupt are handled by kernel */
        f_interrupt = FALSE;
    }
#endif

    /*
     * Clear defaults if the channel under configuration is the
     * current default
     */
    if ((sc->sc_type == DV_RX) && (soc->soc_dma_default_rx == sc)) {
        soc->soc_dma_default_rx->sc_flags &= ~SOC_DMA_F_DEFAULT;
        soc->soc_dma_default_rx = NULL;
    } else if ((sc->sc_type == DV_TX) && (soc->soc_dma_default_tx == sc)) {
        soc->soc_dma_default_tx->sc_flags &= ~SOC_DMA_F_DEFAULT;
        soc->soc_dma_default_tx = NULL;
    }

    /* Initialize dummy Q entry */

    sc->sc_q            = NULL;         /* No queued operations */
    sc->sc_dv_active    = NULL;         /* No active operations */
    sc->sc_q_cnt        = 0;            /* Queue count starts at 0 */

#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        int cmc = SOC_PCI_CMC(unit);

        /* Initial state of channel */
        sc->sc_flags = 0;                   /* Clear flags to start */
        (void)soc_cmicm_intr0_disable(unit, IRQ_CMCx_DESC_DONE(c) | IRQ_CMCx_CHAIN_DONE(c));

        cr = soc_pci_read(unit,CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc,c));
        cr &= ~PKTDMA_ENABLE; /* clearing enable will also clear CHAIN_DONE */
        soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc,c), cr);

        cr = soc_pci_read(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc));
        soc_pci_write(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc), (cr | DS_DESCRD_CMPLT_CLR(c)));
        soc_pci_write(unit, CMIC_CMCx_DMA_STAT_CLR_OFFSET(cmc), cr);

        /* Setup new mode */
        /* MBM Deprecated in CMICm */
        /* DROP_TX Deprecated in CMICm */
        bits = f_desc_intr ? PKTDMA_SEL_INTR_ON_DESC_OR_PKT : 0;

        if (type == DV_TX) {
            bits |= PKTDMA_DIRECTION;
            if (f_default) {
                soc->soc_dma_default_tx = sc;
            }
        } else if (type == DV_RX) {
            bits &= ~PKTDMA_DIRECTION;
            if (f_default) {
                soc->soc_dma_default_rx = sc;
            }
        } else if (type == DV_NONE) {
            f_interrupt = FALSE;            /* Force off */
        } else {
            assert(0);
        }
        
        if (f_interrupt) {
            (void)soc_cmicm_intr0_enable(unit, IRQ_CMCx_DESC_DONE(c) | IRQ_CMCx_CHAIN_DONE(c));
        }
        sc->sc_type = type;

        cr = soc_pci_read(unit,CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc,c));
        /* clear everything except Endianess */
        cr &= (PKTDMA_BIG_ENDIAN | PKTDMA_DESC_BIG_ENDIAN);
        cr |= bits;
#if 0
        if (type == DV_TX) {
            cr |= RLD_STATUS_UPD_DIS;
        }
#endif
        soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, c), cr);
    } else
#endif /* BCM_CMICM_SUPPORT */
    {
        /* Initial state of channel */

        sc->sc_flags = 0;                   /* Clear flags to start */
        (void)soc_intr_disable(unit, IRQ_DESC_DONE(c) | IRQ_CHAIN_DONE(c));
        soc_pci_write(unit, CMIC_DMA_STAT, DS_DMA_EN_CLR(c));
        soc_pci_write(unit, CMIC_DMA_STAT, DS_DESC_DONE_CLR(c));
        soc_pci_write(unit, CMIC_DMA_STAT, DS_CHAIN_DONE_CLR(c));

        /* Setup new mode */

        bits  = f_mbm ? DC_MOD_BITMAP(c) : DC_NO_MOD_BITMAP(c);
        bits |= f_drop ? DC_DROP_TX(c) : DC_NO_DROP_TX(c);
        bits |= f_desc_intr ? DC_INTR_ON_DESC(c) : DC_INTR_ON_PKT(c);

        if (type == DV_TX) {
            bits |= DC_MEM_TO_SOC(c);
            if (f_default) {
                soc->soc_dma_default_tx = sc;
            }
        } else if (type == DV_RX) {
            bits |= DC_SOC_TO_MEM(c);
            if (f_default) {
                soc->soc_dma_default_rx = sc;
            }
        } else if (type == DV_NONE) {
            f_interrupt = FALSE;            /* Force off */
        } else {
            assert(0);
        }

        if (f_interrupt) {
            (void)soc_intr_enable(unit, IRQ_DESC_DONE(c) | IRQ_CHAIN_DONE(c));
        }
        sc->sc_type = type;

        cr = soc_pci_read(unit, CMIC_DMA_CTRL);
        cr &= ~DC_CHAN_MASK(c);             /* Clear this channels bits */
        cr |= bits;                         /* Set new values */
        soc_pci_write(unit, CMIC_DMA_CTRL, cr);
    }
    sc->sc_flags = flags;

#ifdef  GNATS_2371_WAR
    /*
     * remember if any channels are in drop_tx mode (gnats 2371 workaround)
     */
    soc->dma_droptx = (cr & DC_DROP_TX_ALL) ? 1 : 0;
#endif  /* GNATS_2371_WAR */

    SOC_DMA_UNLOCK(soc_dma_lock);
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_dma_free_list
 * Purpose:
 *      Free everything on the cached DV list.
 * Parameters:
 *      unit - StrataSwitch unit #.
 * Returns:
 *      Nothing.
 */

#if (defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_CMIC_SUPPORT) || defined (BCM_ARAD_SUPPORT))
extern void tx_dv_free_cb(int unit, dv_t *dv) __attribute__ ((weak)); 
extern void rx_dv_free_cb(int unit, dv_t *dv) __attribute__ ((weak));
#else
static void (*tx_dv_free_cb)(int unit, dv_t *dv) = NULL; 
static void (*rx_dv_free_cb)(int unit, dv_t *dv) = NULL;
#endif

STATIC void
soc_dma_free_list(int unit)
{
    uint32        soc_dma_lock;
    soc_control_t *soc = SOC_CONTROL(unit);
    dv_t          *dv;

    SOC_DMA_LOCK(soc_dma_lock);
    soc->soc_dv_tx_free_cnt = 0;           /* Mark no TX free DVs on list */
    dv = soc->soc_dv_tx_free;              /* Pick up TX free list */
    soc->soc_dv_tx_free = NULL;            /* NULL list off soc structure */
    SOC_DMA_UNLOCK(soc_dma_lock);

    while (NULL != dv) {
        dv_t    *tdv = dv->dv_chain;
        if (tx_dv_free_cb) {
            tx_dv_free_cb(unit, dv);
        }
        soc_cm_sfree(unit, dv->dv_dcb);
        soc_cm_sfree(unit, dv->dv_dmabuf);
        soc_at_free(unit, dv);
        dv = tdv;
    }
    
    SOC_DMA_LOCK(soc_dma_lock);
    soc->soc_dv_rx_free_cnt = 0;           /* Mark no RX free DVs on list */
    dv = soc->soc_dv_rx_free;              /* Pick up RX free list */
    soc->soc_dv_rx_free = NULL;            /* NULL list off soc structure */
    SOC_DMA_UNLOCK(soc_dma_lock);

    while (NULL != dv) {
        dv_t    *rdv = dv->dv_chain;
        if (rx_dv_free_cb) {
            rx_dv_free_cb(unit, dv);
        }
        soc_cm_sfree(unit, dv->dv_dcb);
        soc_cm_sfree(unit, dv->dv_dmabuf);
        soc_at_free(unit, dv);
        dv = rdv;
    }
}

/*
 * Function:
 *      soc_dma_init
 * Purpose:
 *      Initialize the SOC DMA routines for a SOC unit.
 * Parameters:
 *      unit - SOC unit #
 * Returns:
 *      SOC_E_NONE - Success
 *      SOC_E_BUSY - DMA channel busy.
 *      SOC_E_XXX
 * Notes:
 *      This routine frees all DV's on the free list, and configures
 *      a default channel configuration.
 */

int
soc_dma_init(int unit)
{
    soc_dma_free_list(unit);           /* Free current DV list */

#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        int cmc = SOC_PCI_CMC(unit);
        uint32 val, ch;

        soc_pci_write(unit, CMIC_RXBUF_EPINTF_RELEASE_ALL_CREDITS_OFFSET, 0);
        soc_pci_write(unit, CMIC_RXBUF_EPINTF_RELEASE_ALL_CREDITS_OFFSET, 1);

        /* Known good state except Endianess*/
        for(ch=0; ch < N_DMA_CHAN; ch++) {
            val = soc_pci_read(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, ch)); 
            val &= (PKTDMA_BIG_ENDIAN | PKTDMA_DESC_BIG_ENDIAN); 
           
            soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, ch), val);
        }
    } else
#endif /* BCM_CMICM_SUPPORT */
    {
        soc_pci_write(unit, CMIC_DMA_CTRL, 0); /* Known good state */
    }

    SOC_IF_ERROR_RETURN
        (soc_dma_chan_config(unit, 0, DV_TX,
                             SOC_DMA_F_MBM | SOC_DMA_F_DEFAULT));
    SOC_IF_ERROR_RETURN
        (soc_dma_chan_config(unit, 1, DV_RX,
                             SOC_DMA_F_MBM | SOC_DMA_F_DEFAULT));
    SOC_IF_ERROR_RETURN
        (soc_dma_chan_config(unit, 2, DV_NONE, SOC_DMA_F_MBM));
    SOC_IF_ERROR_RETURN
        (soc_dma_chan_config(unit, 3, DV_NONE, SOC_DMA_F_MBM));

#ifdef INCLUDE_KNET
    if (SOC_KNET_MODE(unit)) {
        if (soc_feature(unit, soc_feature_cos_rx_dma)) {
            /* Enable additional Rx DMA channels by default */
            SOC_IF_ERROR_RETURN
                (soc_dma_chan_config(unit, 2, DV_RX, SOC_DMA_F_MBM));
            SOC_IF_ERROR_RETURN
                (soc_dma_chan_config(unit, 3, DV_RX, SOC_DMA_F_MBM));
#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm)) {
                /* COS-based DMA channel mapping always enabled */
            } else 
#endif
            {
                /* Enable COS-based DMA channel mapping */
                soc_pci_write(unit, CMIC_CONFIG, 
                              (soc_pci_read(unit, CMIC_CONFIG) |
                               CC_COS_QUALIFIED_DMA_RX_EN));
            }
        }
        soc_knet_hw_init(unit);
    }
#endif

    return(SOC_E_NONE);
}

#ifdef INCLUDE_KNET

STATIC void
soc_dma_done_knet(int unit, uint32 chan)
{
    dma_chan_t          c = (dma_chan_t)chan;
    soc_control_t       *soc = SOC_CONTROL(unit);
    sdc_t               *sc = &soc->soc_channels[c];
    dv_t                *dv_chain = sc->sc_q;   /* Pick up request */
    dv_t                *dv_active = sc->sc_dv_active;
    assert(sc->sc_q_cnt);               /* Be sure there is at least one */
    assert(dv_chain);
    assert(dv_active);
    assert(SOC_KNET_MODE(unit));

    /*
     * Reap done descriptors, and possibly notify callers of descriptor
     * completion.
     */
    sc->sc_dv_active =
          soc_dma_process_done_desc(unit, dv_chain, dv_active);

    if (dv_chain->dv_dcnt &&
        dv_chain->dv_dcnt == dv_chain->dv_vcnt) {

        sc->sc_q    = dv_chain->dv_next;    /* Unlink current DV */
        sc->sc_q_cnt--;                     /* Decrement */

        soc_dma_start_channel(unit, sc);

        if (dv_chain->dv_flags & DV_F_NOTIFY_CHN) {
            if (dv_chain->dv_done_chain) {
                dv_chain->dv_done_chain(unit, dv_chain);
            }
        }
    }
}

STATIC int
soc_dma_handle_knet_event(kcom_msg_t *kmsg, unsigned int len, void *cookie)
{
    if (kmsg->hdr.type == KCOM_MSG_TYPE_EVT &&
        kmsg->hdr.opcode == KCOM_M_DMA_INFO) {
        kcom_dma_info_t *dma_info = &kmsg->dma_info.dma_info;
        int unit = kmsg->hdr.unit;
        uint32 soc_dma_lock;
        soc_control_t *soc = SOC_CONTROL(unit);
        int chan;
        sdc_t *sc;
        dv_t *dv_active;

        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "soc_knet_handle_event: KCOM_M_DMA_INFO\n")));

        SOC_DMA_LOCK(soc_dma_lock);

        if (dma_info->flags & KCOM_DMA_INFO_F_TX_DONE) {
            chan = 0;
            sc = &soc->soc_channels[chan];
            do {
                dv_active = sc->sc_dv_active;
                if (dv_active && sc->sc_q_cnt) {
                    soc_dma_done_knet(unit, chan);
                }
            } while (sc->sc_dv_active != dv_active);
        }

        if (dma_info->flags & KCOM_DMA_INFO_F_RX_DONE) {
            chan = 1;
            sc = &soc->soc_channels[chan];
            do {
                dv_active = sc->sc_dv_active;
                if (dv_active && sc->sc_q_cnt) {
                    soc_dma_done_knet(unit, chan);
                }
            } while (sc->sc_dv_active != dv_active);
        }

        SOC_DMA_UNLOCK(soc_dma_lock);

        /* Handled */
        return 1;
    }
    /* Not handled */
    return 0;
}

#endif /* INCLUDE_KNET */

/*
 * Function:
 *      soc_dma_attach
 * Purpose:
 *      Setup DMA structures when a device is attached.
 * Parameters:
 *      unit - StrataSwitch unit #.
 * Returns:
 *      SOC_E_NONE - Attached successful.
 *      SOC_E_xxx  - Attach failed.
 * Notes:
 *      Initializes data structure without regards to the current fields,
 *      calling this routine without detach first may result in memory
 *      leaks.
 */

int
soc_dma_attach(int unit, int reset)
{
    uint32        soc_dma_lock;
    soc_control_t *soc = SOC_CONTROL(unit);
    int           i;
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
#endif

#ifdef INCLUDE_KNET
    if (soc_knet_init(unit) == SOC_E_NONE) {
        SOC_KNET_MODE_SET(unit, 1);
        soc_knet_rx_unregister(soc_dma_handle_knet_event);
        soc_knet_rx_register(soc_dma_handle_knet_event, NULL, 0);
    }
#endif

    soc->soc_dv_size   = SOC_DMA_DV_FREE_SIZE;
    soc->soc_dv_cnt    = SOC_DMA_DV_FREE_CNT;

    soc->stat.dv_alloc      = 0;        /* Init Alloc count */
    soc->stat.dv_free       = 0;        /* Init Free count */
    soc->stat.dv_alloc_q    = 0;        /* Init Alloc from Q count */
    soc->soc_dv_tx_free_cnt    = 0;        /* Init Free list Q Count */
    soc->soc_dv_rx_free_cnt    = 0;
    
    soc->soc_dma_default_tx = NULL;
    soc->soc_dma_default_rx = NULL;
    soc->soc_max_channels = N_DMA_CHAN;

#ifdef  BCM_XGS3_SWITCH_SUPPORT
    if (soc_feature(unit, soc_feature_txdma_purge)) {
        /*
         * Allocate a 64 byte packet for TX purge packet
         */
        if (soc->tx_purge_pkt == NULL) {
            soc->tx_purge_pkt = soc_cm_salloc(unit, 64, "tx_purge");
        }
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
         uint32 val, ch;
        /* Known good state except Endianess*/
        for(ch=0; ch < N_DMA_CHAN; ch++) {
            val = soc_pci_read(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, ch));  
             val &= (PKTDMA_BIG_ENDIAN | PKTDMA_DESC_BIG_ENDIAN); 
             /*
             if (SOC_IS_ARAD(unit)) {
                 val |= (PKTDMA_BIG_ENDIAN | PKTDMA_DESC_BIG_ENDIAN);  
             } 
             */ 
            soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc, ch), val);
        }
    } else
#endif /* CMICM Support */
    {
        soc_pci_write(unit, CMIC_DMA_CTRL, 0); /* Known good state */
    }

    for (i = 0; i < soc->soc_max_channels; i++) {
        sdc_t   *s = &soc->soc_channels[i];
        int rv;

        if (! reset) {
            /*
             * soc_dma_abort_channel does not require any data structures be set
             * up, it clears hardware state only.
             */
            SOC_DMA_LOCK(soc_dma_lock);
            rv = soc_dma_abort_channel(unit, i);
            SOC_DMA_UNLOCK(soc_dma_lock);
            SOC_IF_ERROR_RETURN(rv);
        }

        sal_memset(s, 0, sizeof(*s));           /* Start off 0 */
        s->sc_type    = DV_NONE;
        s->sc_channel = i;
    }    
    SOC_IF_ERROR_RETURN(soc_dma_init(unit));
   
#ifdef INCLUDE_KNET
    if (SOC_KNET_MODE(unit)) {
        return SOC_E_NONE;
    }
#endif 
 
    if (soc_property_get(unit, spn_DCB_INTR_MITIGATE_ENABLE, 0) && 
        (!soc_dma_intr_mitigation[unit].enabled)) { 
        char thread_name[32];        
        sal_memset(&soc_dma_intr_mitigation[unit], 0, 
                   sizeof(soc_dma_intr_mitigation[unit]));
        soc_dma_intr_mitigation[unit].enabled = 1;
        for (i = 0; i < soc->soc_max_channels; i++) {
            sdc_t   *s = &soc->soc_channels[i];  
            sdc_intr_mitigate_t *sdc_mtg;
            if (s->sc_type == DV_RX) {
                sdc_mtg = 
                    &soc_dma_intr_mitigation[unit].chans_intr_mitigation[i];
                sal_sprintf(sdc_mtg->sem_name, "semDmaM%d_%d", 
                            unit, s->sc_channel);
                sdc_mtg->thread_sem = sal_sem_create(sdc_mtg->sem_name,
                                                     0, 0);
                sal_sprintf(thread_name, "bcmDmaIntrM%d_%d", 
                            unit, s->sc_channel);
                sdc_mtg->thread_running = 1;
                sdc_mtg->thread_id = sal_thread_create(thread_name, 4096,
                       soc_property_get(unit, spn_SOC_DMA_MONITOR_THREAD_PRI, 0),
                       (void (*)(void*))soc_dma_monitor_thread, 
                       INT_TO_PTR(unit << 16 | s->sc_channel));
            }
        }       
    }
    
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_dma_detach
 * Purpose:
 *      Abort DMA active on the specified channel, and free
 *      internal memory associated with DMA on the specified unit.
 *      It is up to the caller to ensure NO more DMAs are started.
 * Parameters:
 *      unit - StrataSwitch Unit number
 * Returns:
 *      SOC_E_TIMEOUT - indicates attempts to abort active
 *              operations failed. Device is detached, but operation
 *              is undefined at this point.
 *      SOC_E_NONE - Operation sucessful.
 */

int
soc_dma_detach(int unit)
{
    (void)soc_dma_abort(unit);
#ifdef  BCM_XGS3_SWITCH_SUPPORT
    if (soc_feature(unit, soc_feature_txdma_purge)) {
        if (SOC_CONTROL(unit)->tx_purge_pkt != NULL) {
            soc_cm_sfree(unit, SOC_CONTROL(unit)->tx_purge_pkt);
            SOC_CONTROL(unit)->tx_purge_pkt = NULL;
        }
    }
#endif  /* BCM_XGS3_SWITCH_SUPPORT */
    soc_dma_free_list(unit);
      
    if (soc_dma_intr_mitigation[unit].enabled) { 
        int i;
        for (i = 0; i < N_DMA_CHAN ; i++) { 
            sdc_intr_mitigate_t *sdc_mtg =
                &soc_dma_intr_mitigation[unit].chans_intr_mitigation[i];
            if (sdc_mtg->thread_running) { 
                sdc_mtg->mitigation_started = 0;                  
                sdc_mtg->thread_running = 0; 
                sdc_mtg->rx_chain_intr = sdc_mtg->rx_chain_intr_prev = 0;
                sal_sem_give(sdc_mtg->thread_sem);                   
            }
        }
        soc_dma_intr_mitigation[unit].enabled = 0; 
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_dma_poll_channel
 * Purpose:
 *      Poll one, or ALL of the channels.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      c - DMA channel to poll
 * Returns:
 *      Nothing
 */

STATIC void
soc_dma_poll_channel(int unit, dma_chan_t c)
{
    uint32        soc_dma_lock;
    soc_control_t *soc = SOC_CONTROL(unit);
    sdc_t         *sc = &soc->soc_channels[c];
#ifdef BCM_CMICM_SUPPORT
    int cmc = SOC_PCI_CMC(unit);
#endif

    if (((sc->sc_flags & SOC_DMA_F_POLL)) && (sc->sc_dv_active)) {
        /*
         * The _soc_dma_done_[desc|chain] routines expect to be called
         * ONLY from an interrupt handler since they may not always
         * grab the lock.
         */
        SOC_DMA_LOCK(soc_dma_lock);
#ifdef BCM_CMICM_SUPPORT
        if (soc_feature(unit, soc_feature_cmicm)) {
            if (soc_pci_read(unit, CMIC_CMCx_DMA_STAT_OFFSET(cmc)) & DS_CMCx_DMA_DESC_DONE(c)) {
                soc_dma_done_desc(unit, (uint32)c);
                soc->stat.intr_desc--;      /* undo increment in done_desc */
            }
            if (soc_pci_read(unit, CMIC_CMCx_DMA_STAT_OFFSET(cmc)) & DS_CMCx_DMA_CHAIN_DONE(c)) {
                soc_dma_done_chain(unit, (uint32)c);
                soc->stat.intr_chain--;     /* undo increment in done_chain */
            }
        } else
#endif
        {
            if (soc_pci_read(unit, CMIC_DMA_STAT) & DS_DESC_DONE_TST(c)) {
                soc_dma_done_desc(unit, (uint32)c);
                soc->stat.intr_desc--;      /* undo increment in done_desc */
            }
            if (soc_pci_read(unit, CMIC_DMA_STAT) & DS_CHAIN_DONE_TST(c)) {
                soc_dma_done_chain(unit, (uint32)c);
                soc->stat.intr_chain--;     /* undo increment in done_chain */
            }
        }
        SOC_DMA_UNLOCK(soc_dma_lock);
    }
}

/*
 * Function:
 *      soc_dma_poll
 * Purpose:
 *      Poll one, or all DMA channels.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      c    - DMA channel to poll, or -1
 * Returns:
 *      Nothing
 */

void
soc_dma_poll(int unit, dma_chan_t c)
{
    soc_control_t       *soc = SOC_CONTROL(unit);

    assert(c < soc->soc_max_channels);

    if (c < 0) {
        for (c = 0; c < soc->soc_max_channels; c++) {
            soc_dma_poll_channel(unit, c);
        }
    } else {
        soc_dma_poll_channel(unit, c);
    }
}

/*
 * Reuse DV field.
 */
#define dv_sem          dv_public4.ptr
#define dv_poll         dv_public4.ptr

/*
 * Function:
 *      soc_dma_wait_done (Internal only)
 * Purpose:
 *      Callout for DMA chain done.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      dv_chain - Chain completed.
 * Returns:
 *      Nothing
 */

STATIC void
soc_dma_wait_done(int unit, dv_t *dv_chain)
{
    COMPILER_REFERENCE(unit);
    sal_sem_give((sal_sem_t)dv_chain->dv_sem);
}

/*
 * Function:
 *      soc_dma_poll_done (internal)
 * Purpose:
 *      Callout for DMA chain done.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      dv_chain - Chain completed.
 * Returns:
 *      Nothing
 */

STATIC void
soc_dma_poll_done(int unit, dv_t *dv_chain)
{
    COMPILER_REFERENCE(unit);
    *(volatile int *)dv_chain->dv_poll = TRUE;
}

/*
 * Function:
 *      soc_dma_wait
 * Purpose:
 *      Start a DMA operation and wait for it's completion.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      dv_chain - pointer to dv chain to execute.
 *      usec - Time out in microseconds.  Same meanings as sal_sem_take
 * Returns:
 *      SOC_E_XXXX
 */

STATIC sal_tls_key_t *dma_sem_key = NULL;

STATIC void  
soc_dma_sem_free(void *param)
{
	sal_sem_t sem = (sal_sem_t)sal_tls_key_get(dma_sem_key);
	if (param) {
		;
	}
	if (sem == NULL) {
        return;
	}
	sal_sem_destroy(sem);
}

int
soc_dma_wait_timeout(int unit, dv_t *dv_chain, int usec)
{
    int         rv = SOC_E_NONE;
    sdc_t       *sc;
    volatile int done;
    sal_usecs_t start_time;
    int diff_time;

    if ((sc = soc_dma_channel(unit, -1, dv_chain)) == NULL) {
        return(SOC_E_RESOURCE);
    }

    /* If a polling channel, use POLLED mode */

    if ((sc->sc_flags & SOC_DMA_F_POLL) )     {
        dv_chain->dv_sem = NULL;
        dv_chain->dv_done_chain = soc_dma_poll_done;
        dv_chain->dv_poll = (void *) &done;
        done = FALSE;
    } else {
        if (dma_sem_key == NULL) {        	
	        dma_sem_key = sal_tls_key_create(soc_dma_sem_free);	        
        } 
        dv_chain->dv_sem = sal_tls_key_get(dma_sem_key);
        if (dv_chain->dv_sem == NULL) {
        	dv_chain->dv_sem = sal_sem_create("dv_sem", sal_sem_BINARY, 0);
	        if (!dv_chain->dv_sem) {
	            return(SOC_E_MEMORY);
	        }
        	sal_tls_key_set(dma_sem_key, (void *)dv_chain->dv_sem);
        }        
        dv_chain->dv_done_chain = soc_dma_wait_done;
    }

    SET_NOTIFY_CHN_ONLY(dv_chain->dv_flags);

    soc_dma_start_dv(unit, sc, dv_chain);

    start_time = sal_time_usecs();
    diff_time = 0;

    if ((sc->sc_flags & SOC_DMA_F_POLL) )     {        
        while (!done) {
            soc_dma_poll(unit, sc->sc_channel);
            if ((usec != sal_sem_FOREVER) && !done) {
                diff_time = SAL_USECS_SUB(sal_time_usecs(), start_time);
                if (diff_time > usec) {
                    rv = SOC_E_TIMEOUT;
                    break;
                } else if (diff_time < 0) {
                    /* Restart in case system time changed */
                    start_time = sal_time_usecs();
                }
            }
        }
    } else {
        if (sal_sem_take((sal_sem_t)dv_chain->dv_sem, sal_sem_FOREVER)) {
            (void)soc_dma_abort_dv(unit, dv_chain); /* Clean up */
            rv = SOC_E_TIMEOUT;
        }
        if (dma_sem_key == NULL) {
        	if (dv_chain->dv_sem != NULL) {
	            sal_sem_destroy((sal_sem_t)dv_chain->dv_sem);
	        }
        }
    }

    return(rv);
}

/*
 * Function:
 *      soc_dma_wait
 * Purpose:
 *      Start a DMA operation and wait for it's completion.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      dv_chain - pointer to dv chain to execute.
 * Returns:
 *      SOC_E_XXXX
 */

int
soc_dma_wait(int unit, dv_t *dv_chain)
{
    return soc_dma_wait_timeout(unit, dv_chain, sal_sem_FOREVER);
}

/*
 * Function:
 *      soc_dma_chan_dvt_get
 * Purpose:
 *      Get the DV type for the given channel
 * Parameters:
 *      unit
 *      chan
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Does not check if channel is valid
 */

dvt_t
soc_dma_chan_dvt_get(int unit, dma_chan_t chan)
{
    return SOC_CONTROL(unit)->soc_channels[chan].sc_type;
}

/***********************************************************************
 *
 * Simple ROM TX/RX Mode
 *
 * The following API calls may be used in lieue of the regular calls in
 * order to provide a simplified, polling-driven transmit/receive API.
 * These routines are very inefficient, but they are easy to use and
 * don't require interrupt processing.
 *
 * soc_dma_rom_init() - configure chip for this mode
 * soc_dma_rom_detach() - deconfigure
 * soc_dma_rom_tx_start() - start a packet transmit; returns immediately
 * soc_dma_rom_tx_poll() - check if transmit is done yet
 * soc_dma_rom_tx_abort() - abort a transmit in progress
 * soc_dma_rom_rx_poll() - get receive packet, if any; returns immediately
 *
 * soc_dma_rom_dcb_alloc() - utility routine for creating packet descriptor
 * soc_dma_rom_dcb_free() - utility routine for freeing packet descriptor
 *
 **********************************************************************/

/*
 * Variable:
 *      soc_dma_rom_control
 * Purpose:
 *      Global control variables for the soc_dma_rom API.
 */

static struct {
    struct {
        dv_t            *dv;
        volatile int    done;
    } tx, rx;
    int psize;
} soc_dma_rom_control[SOC_MAX_NUM_DEVICES];

/*
 * Function:
 *      soc_dma_rom_dcb_alloc
 * Purpose:
 *      Allocate a DCB packet structure for use with RX and TX functions.
 * Parameters:
 *      unit    -       unit number
 *      psize   -       packet buffer size
 * Returns:
 *      DCB pointer on success, NULL on failure.
 * Notes:
 *      Use this function to allocate a DCB structure and associated
 *      packet memory.  A packet buffer of size 'psize' will be
 *      allocated and associated with this DCB.
 */

dcb_t *
soc_dma_rom_dcb_alloc(int unit, int psize)
{
    void                *pkt;
    dcb_t               *dcb;

    if ((dcb = soc_cm_salloc(unit,
                             SOC_DCB_SIZE(unit),
                             "soc_dma_rom_dcb_alloc")) == NULL) {
        return NULL;
    }

    if ((pkt = soc_cm_salloc(unit,
                             psize,
                             "soc_dma_rom_dcb_alloc_packet")) == NULL) {
        soc_cm_sfree(unit, dcb);
        return NULL;
    }

    /* Setup DCB defaults */
    sal_memset(dcb, 0, SOC_DCB_SIZE(unit)); 
    SOC_DCB_INIT(unit, dcb);
    SOC_DCB_ADDR_SET(unit, dcb, (sal_vaddr_t)pkt);
    SOC_DCB_REQCOUNT_SET(unit, dcb, psize);
    SOC_DCB_SG_SET(unit, dcb, 0);
    SOC_DCB_CHAIN_SET(unit, dcb, 0);

    /* Everything else should be specified by the caller */
    return dcb;
}

/*
 * Function:
 *      soc_dma_rom_dcb_free
 * Purpose:
 *      Free a DCB packet structure allocated with soc_dma_rom_dcb_free()
 * Parameters:
 *      unit    -       unit number
 *      dcb     -       dcb pointer
 * Returns:
 *      SOC_E_XXX
 */

int
soc_dma_rom_dcb_free(int unit, dcb_t *dcb)
{
    if (dcb) {
        void            *pkt;

        pkt = (void *)SOC_DCB_ADDR_GET(unit, dcb);

        if (pkt != NULL) {
            soc_cm_sfree(unit, pkt);
        }

        soc_cm_sfree(unit, dcb);
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_dma_rom_dv_start_polled
 * Purpose:
 *      Start an asyncronous, polled DV DMA op
 * Parameters:
 *      unit    -       unit
 *      channel -       channel to start on
 *      dv      -       dv
 *      poll    -       pointer to poll-done variable
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Helper function for soc_dma_rom() rx and tx.
 */

static int
soc_dma_rom_dv_start_polled(int unit, int channel,
                             dv_t *dv, volatile void *poll)
{
    assert(dv);

    /* Setup chain-done notification */

    dv->dv_flags |= DV_F_NOTIFY_CHN;
    dv->dv_sem = NULL;
    dv->dv_done_chain = soc_dma_poll_done;
    dv->dv_poll = (void *)poll;

    /* Start up this chain */

    return soc_dma_start(unit, channel, dv);
}

/*
 * Function:
 *      soc_dma_rom_rx_start
 * Purpose:
 *      Start one packet DMA rx
 * Parameters:
 *      unit    -       unit
 * Returns:
 *      SOC_E_XXX
 */

static int
soc_dma_rom_rx_start(int unit)
{
    dv_t                *dv;
    int                 rv;

    /* Get our RX DV  */

    if ((dv = soc_dma_rom_control[unit].rx.dv) == NULL) {
        return SOC_E_INIT;
    }

    /* Reset our DV */

    soc_dma_dv_reset(DV_RX, dv);

    /* Allocate a DCB/packet structure */

    dv->dv_dcb = soc_dma_rom_dcb_alloc(unit,
                                       soc_dma_rom_control[unit].psize);

    /* Bring it on */

    if ((rv = soc_dma_rom_dv_start_polled(unit, SOC_DMA_ROM_RX_CHANNEL, dv,
                           &soc_dma_rom_control[unit].rx.done)) < 0) {
        /* D'oh */
        soc_dma_rom_dcb_free(unit, dv->dv_dcb);

        dv->dv_dcb = NULL;

        return rv;
    }

    return rv;
}



/*
 * Function:
 *      soc_dma_rom_init
 * Purpose:
 *      Initialize simple polled tx/rx (ROM mode).
 * Parameters:
 *      unit            - StrataSwitch unit #
 *      max_packet_rx   - Maximum RX packet size
 * Returns:
 *      SOC_E_XXX
 */

int
soc_dma_rom_init(int unit, int max_packet_rx, int tx, int rx)
{
    dv_t                *dv_rx, *dv_tx;

    sal_memset(&soc_dma_rom_control[unit], 0,
               sizeof(soc_dma_rom_control[unit]));

    /* Reconfigure TX/RX channels for polling */

    if (tx) {
        SOC_IF_ERROR_RETURN(
            soc_dma_chan_config(unit, SOC_DMA_ROM_TX_CHANNEL, DV_TX,
                                SOC_DMA_F_MBM | SOC_DMA_F_POLL | SOC_DMA_F_DEFAULT));
    }   
    
    if (rx) {
        SOC_IF_ERROR_RETURN(
            soc_dma_chan_config(unit, SOC_DMA_ROM_RX_CHANNEL, DV_RX,
                                SOC_DMA_F_MBM | SOC_DMA_F_POLL | SOC_DMA_F_DEFAULT));
    }   

    soc_dma_rom_control[unit].psize = max_packet_rx;

    /* Allocate our TX/RX DVs */

    if ((dv_rx = soc_dma_dv_alloc(unit, DV_RX, 1)) == NULL) {
        return SOC_E_MEMORY;
    }

    if ((dv_tx = soc_dma_dv_alloc(unit, DV_TX, 1)) == NULL) {
        soc_dma_dv_free(unit, dv_rx);
        return SOC_E_MEMORY;
    }

    /*
     * We play around with the DCB in these DVs quite a bit. Free the
     * originals so we can muck with it later.
     */

    soc_cm_sfree(unit, dv_rx->dv_dcb);
    soc_cm_sfree(unit, dv_tx->dv_dcb);

    /* Store our new DVs */

    soc_dma_rom_control[unit].rx.dv = dv_rx;
    soc_dma_rom_control[unit].tx.dv = dv_tx;

    /* Startup an RX */

    if (rx) {
        soc_dma_rom_rx_start(unit);
    }

    /* No TX's outstanding */

    soc_dma_rom_control[unit].tx.done = 1;

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_dma_rom_detach
 * Purpose:
 *      Finalize the polled TX/RX module
 * Parameters:
 *      unit - StrataSwitch unit #
 * Returns:
 *      SOC_E_XXX
 */

int
soc_dma_rom_detach(int unit)
{
    int                 rv;
    dv_t                *dv;

    /* Kill DMA */

    rv = soc_dma_detach(unit);

    /*
     * Free our buffers We need to free our custom packet DCBs, then
     * replace the DV's dcb pointer which we freed at init time with a
     * new pointer before we hand it back.
     */

    dv = soc_dma_rom_control[unit].rx.dv;

    /* Free our RX DCB */

    if (dv->dv_dcb) {
        soc_dma_rom_dcb_free(unit, dv->dv_dcb);
    }

    dv->dv_dcb = soc_cm_salloc(unit, SOC_DCB_SIZE(unit), "sdma_dv_alloc");

    dv = soc_dma_rom_control[unit].tx.dv;

    /*
     * Do not deallocate the DCB associated with TX.
     * This was allocated by the caller.
     */

    dv->dv_dcb = soc_cm_salloc(unit, SOC_DCB_SIZE(unit), "sdma_dv_alloc");

    sal_memset(&soc_dma_rom_control[unit], 0,
               sizeof(soc_dma_rom_control[unit]));

    return rv;
}

/*
 * Function:
 *      soc_dma_rom_tx_start
 * Purpose:
 *      Kick off a packet transmit (ROM mode)
 * Parameters:
 *      unit - StrataSwitch unit #
 *      dcb  - Device specific DMA descriptor block structure
 * Notes:
 *      This function will transmit one packet as described by the dcb
 *      parameter.
 *
 *      The DCB packet structure should be allocated using
 *      soc_dma_rom_dcb_alloc().
 *
 *      The caller must setup the DCB structure correctly for the chip
 *      on which the packet will be transmitted.  On the BCM5670 the
 *      packet data must contain the Higig header.
 *
 *      This routine does not wait for transmit complete.  Use
 *      soc_dma_rom_tx_poll() to determine transmit completion.
 *
 *      The application may not re-use the dcb or packet buffer memory
 *      or transmit any other packets until the transmit is complete.
 */

int
soc_dma_rom_tx_start(int unit, dcb_t *dcb)
{
    dv_t                *dv;

    /* Get our TX DV */

    if ((dv = soc_dma_rom_control[unit].tx.dv) == NULL) {
        return SOC_E_INIT;
    }

    /* Reset our DV */
    soc_dma_dv_reset(DV_TX, dv);

    /* Our DCB has already been allocated. Drop it in our DV. */

    dv->dv_dcb = dcb;
    dv->dv_vcnt = 1; 

    /* Send it */

    soc_dma_rom_control[unit].tx.done = 0;

    return soc_dma_rom_dv_start_polled(unit, SOC_DMA_ROM_TX_CHANNEL,
                                        dv,
                                        &soc_dma_rom_control[unit].tx.done);
}

/*
 * Function:
 *      soc_dma_rom_tx_poll
 * Purpose:
 *      Indicate whether previous transmit with soc_dma_rom_tx_start()
 *      has completed.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      done - (OUT) TRUE or FALSE, indicating whether transmit was done
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      After completion, the original DCB and packet memory may be reused.
 */

int
soc_dma_rom_tx_poll(int unit, int *done)
{
    soc_dma_poll(unit, SOC_DMA_ROM_TX_CHANNEL);

    *done = soc_dma_rom_control[unit].tx.done;

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_dma_rom_tx_abort
 * Purpose:
 *      Abort a transmit in progress.
 * Parameters:
 *      unit - StrataSwitch unit #
 * Returns:
 *      SOC_E_XXX
 */

int
soc_dma_rom_tx_abort(int unit)
{
    if (soc_dma_rom_control[unit].tx.done == 0) {
        soc_dma_rom_control[unit].tx.done = 1;
        return soc_dma_abort_channel(unit, SOC_DMA_ROM_TX_CHANNEL);
    }

    return SOC_E_NONE;
}


/*
 * Function:
 *      soc_dma_rom_rx_poll
 * Purpose:
 *      See if a packet is waiting, and it so, return it
 * Parameters:
 *      unit - StrataSwitch unit #
 *      dcb - (OUT) The packet's associated dcb, NULL if no packet ready.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The caller must parse the DCB structure to retrieve the
 *      the packet information.
 *
 *      The caller must call soc_dma_rom_dcb_free() when done with
 *      this packet.
 */

int
soc_dma_rom_rx_poll(int unit, dcb_t **dcb)
{
    /* Has our last RX DMA completed? */

    soc_dma_poll(unit, SOC_DMA_ROM_RX_CHANNEL);

    if (soc_dma_rom_control[unit].rx.done) {
        /* Packet received.  Pull out the DCB */

        *dcb = soc_dma_rom_control[unit].rx.dv->dv_dcb;
        soc_dma_rom_control[unit].rx.dv->dv_dcb = NULL;
        soc_dma_rom_control[unit].rx.done = 0;

        /* Start another DMA */

        soc_dma_rom_rx_start(unit);
    } else {
        *dcb = NULL;
    }

    return SOC_E_NONE;
}

#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(PORTMOD_SUPPORT) */
