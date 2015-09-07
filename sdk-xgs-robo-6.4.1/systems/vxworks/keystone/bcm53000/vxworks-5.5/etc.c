/*
 * Common [OS-independent] portion of
 * Broadcom Home Networking Division 10/100 Mbit/s Ethernet
 * Device Driver.
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
 * $Id: etc.c,v 1.3 Broadcom SDK $
 */

#include <typedefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <proto/ethernet.h>
#include <proto/802.1d.h>
#include <bcmenetrxh.h>
#include <bcmenetphy.h>
#include <et_dbg.h>
#include <etc.h>
#include <et_export.h>
#include <bcmutils.h>

uint32 et_msg_level =
#ifdef BCMDBG
    1;
#else
    0;
#endif /* BCMDBG */

#ifdef CFG_QUICKTURN
#define QT_TEST    /* define for the MDIO test on quickturn */
#endif

/* local prototypes */
static void etc_loopback(etc_info_t *etc, int on);
#ifdef BCMDBG
static void etc_dumpetc(etc_info_t *etc, struct bcmstrbuf *b);
#endif /* BCMDBG */


/* 802.1d priority to traffic class mapping. queues correspond one-to-one
 * with traffic classes.
 */
uint32 up2tc[NUMPRIO] = {
    TC_BE,      /* 0    BE    TC_BE    Best Effort */
    TC_BK,      /* 1    BK    TC_BK    Background */
    TC_BK,      /* 2    --    TC_BK    Background */
    TC_BE,      /* 3    EE    TC_BE    Best Effort */
    TC_CL,      /* 4    CL    TC_CL    Controlled Load */
    TC_CL,      /* 5    VI    TC_CL    Controlled Load */
    TC_VO,      /* 6    VO    TC_VO    Voice */
    TC_VO       /* 7    NC    TC_VO    Voice */
};

uint32 priq_selector[] = {
    [0x0] = TC_NONE, [0x1] = TC_BK, [0x2] = TC_BE, [0x3] = TC_BE,
    [0x4] = TC_CL,   [0x5] = TC_CL, [0x6] = TC_CL, [0x7] = TC_CL,
    [0x8] = TC_VO,   [0x9] = TC_VO, [0xa] = TC_VO, [0xb] = TC_VO,
    [0xc] = TC_VO,   [0xd] = TC_VO, [0xe] = TC_VO, [0xf] = TC_VO
};

/* find the chip opsvec for this chip */
struct chops*
etc_chipmatch(uint vendor, uint device)
{
    {
        extern struct chops bcmgmac_et_chops;

        if (bcmgmac_et_chops.id(vendor, device))
            return (&bcmgmac_et_chops);
    }
    return (NULL);
}

void*
etc_attach(void *et, uint vendor, uint device, uint unit, void *osh, void *regsva)
{
    etc_info_t *etc;
    int i;

    ET_TRACE(("et%d: etc_attach: vendor 0x%x device 0x%x\n", unit, vendor, device));

    /* some code depends on packed structures */
    ASSERT(sizeof(struct ether_addr) == ETHER_ADDR_LEN);
    ASSERT(sizeof(struct ether_header) == ETHER_HDR_LEN);

    /* allocate etc_info_t state structure */
    if ((etc = (etc_info_t*) MALLOC(osh, sizeof(etc_info_t))) == NULL) {
        ET_ERROR(("et%d: etc_attach: out of memory, malloced %d bytes\n", unit,
                  MALLOCED(osh)));
        return (NULL);
    }
    bzero((char*)etc, sizeof(etc_info_t));

    etc->et = et;
    etc->unit = unit;
    etc->osh = osh;
    etc->vendorid = (uint16) vendor;
    etc->deviceid = (uint16) device;
    etc->forcespeed = ET_AUTO;
    etc->linkstate = FALSE;
    /* Set Promisc mode as deafult for testing */
    etc->promisc = 1;
    etc->pkt_mem = MEMORY_DDRRAM;
    /* separate header buffer memory type */
    etc->pkthdr_mem = MEMORY_DDRRAM;
    /* flow control */
    etc->flowcntl_mode = FLOW_CTRL_MODE_DISABLE; /* default mode */
    etc->flowcntl_auto_on_thresh = FLWO_CTRL_AUTO_MODE_DEFAULT_ON_THRESH;
    etc->flowcntl_auto_off_thresh = FLWO_CTRL_AUTO_MODE_DEFAULT_OFF_THRESH;
    etc->flowcntl_cpu_pause_on = 0;
    for (i=0; i < NUMRXQ; i++) {
        etc->flowcntl_rx_on_thresh[i] = 0;
        etc->flowcntl_rx_off_thresh[i] = 0;
        etc->en_rxsephdr[i] = 0;
    }

    /* tx qos */
    etc->txqos_mode = TXQOS_MODE_1STRICT_3WRR;
    for (i=0; i < NUMTXQ; i++) {
        etc->txqos_weight[i] = TXQOS_DEFAULT_WEIGHT;
    }

    /* tpid */
    for (i=0; i < NUM_STAG_TPID; i++) {
        etc->tpid[i] = STAG_TPID_DEFAULT;
    }

    /* set chip opsvec */
    etc->chops = etc_chipmatch(vendor, device);
    ASSERT(etc->chops);

    /* chip attach */
    if ((etc->ch = (*etc->chops->attach)(etc, osh, regsva)) == NULL) {
        ET_ERROR(("et%d: chipattach error\n", unit));
        goto fail;
    }

    return ((void*)etc);

fail:
    etc_detach(etc);
    return (NULL);
}

void
etc_detach(etc_info_t *etc)
{
    if (etc == NULL)
        return;

    /* free chip private state */
    if (etc->ch) {
        (*etc->chops->detach)(etc->ch);
        etc->chops = etc->ch = NULL;
    }

    MFREE(etc->osh, etc, sizeof(etc_info_t));
}

void
etc_reset(etc_info_t *etc)
{
    ET_TRACE(("et%d: etc_reset\n", etc->unit));

    etc->reset++;

    /* reset the chip */
    (*etc->chops->reset)(etc->ch);

    /* free any posted tx packets */
    (*etc->chops->txreclaim)(etc->ch, TRUE);

    /* free any posted rx packets */
    (*etc->chops->rxreclaim)(etc->ch);

}

void
etc_init(etc_info_t *etc, uint options)
{
    ET_TRACE(("et%d: etc_init\n", etc->unit));

    ASSERT(etc->pioactive == NULL);
    ASSERT(!ETHER_ISNULLADDR(&etc->cur_etheraddr));
    ASSERT(!ETHER_ISMULTI(&etc->cur_etheraddr));

    /* init the chip */
    (*etc->chops->init)(etc->ch, options);
}

/* mark interface up */
void
etc_up(etc_info_t *etc)
{
    etc->up = TRUE;

    et_init(etc->et, ET_INIT_DEF_OPTIONS);
}

/* mark interface down */
uint
etc_down(etc_info_t *etc, int reset)
{
    uint callback;

    callback = 0;

    ET_FLAG_DOWN(etc);

    if (reset)
        et_reset(etc->et);

    /* suppress link state changes during power management mode changes */
    if (etc->linkstate) {
        etc->linkstate = FALSE;
        if (!etc->pm_modechange)
            et_link_down(etc->et);
    }

    return (callback);
}

#ifdef QT_TEST
#define QT_MDIO_PHYADDR 0x1     
#endif /* QT_TEST */

/* common ioctl handler.  return: 0=ok, -1=error */
int
etc_ioctl(etc_info_t *etc, int cmd, void *arg)
{
    int error;
    int val;
    int *vec = (int*)arg;

    error = 0;

    val = arg ? *(int*)arg : 0;

    ET_TRACE(("et%d: etc_ioctl: cmd 0x%x\n", etc->unit, cmd));

    switch (cmd) {
    case ETCUP:
        et_up(etc->et);
        break;

    case ETCDOWN:
        et_down(etc->et, TRUE);
        break;

    case ETCLOOP:
        etc_loopback(etc, val);
        break;

    case ETCDUMP:
        if (et_msg_level & 0x10000)
            bcmdumplog((char *)arg, 4096);
#ifdef BCMDBG
        else
        {
            struct bcmstrbuf b;
            bcm_binit(&b, (char*)arg, 4096);
            et_dump(etc->et, &b);
        }
#endif /* BCMDBG */
        break;

    case ETCSETMSGLEVEL:
        et_msg_level = val;
        break;

    case ETCPROMISC:
        etc_promisc(etc, val);
        break;

    case ETCQOS:
        etc_qos(etc, val);
        break;

    case ETCSPEED:
        if ((val != ET_AUTO) && (val != ET_10HALF) && (val != ET_10FULL) &&
            (val != ET_100HALF) && (val != ET_100FULL) && (val != ET_1000FULL))
            goto err;
        etc->forcespeed = val;

        /* explicitly reset the phy */
        (*etc->chops->phyreset)(etc->ch, etc->phyaddr);

        /* request restart autonegotiation if we're reverting to adv mode */
        if ((etc->forcespeed == ET_AUTO) & etc->advertise)
            etc->needautoneg = TRUE;

        et_init(etc->et, ET_INIT_DEF_OPTIONS);
        break;

    case ETCPHYRD:
        if (vec) {

#ifdef QT_TEST
            vec[1] = (*etc->chops->phyrd)(etc->ch, QT_MDIO_PHYADDR, vec[0]);
#else   /* QT_TEST */
            vec[1] = (*etc->chops->phyrd)(etc->ch, etc->phyaddr, vec[0]);
#endif  /* QT_TEST */
            ET_TRACE(("etc_ioctl: ETCPHYRD of reg 0x%x => 0x%x\n", vec[0], vec[1]));
        }
        break;

    case ETCPHYRD2:
        if (vec) {
            uint phyaddr, reg;
            phyaddr = vec[0] >> 16;
            if (phyaddr < MAXEPHY) {
                reg = vec[0] & 0xffff;
                vec[1] = (*etc->chops->phyrd)(etc->ch, phyaddr, reg);
                ET_TRACE(("etc_ioctl: ETCPHYRD2 of phy 0x%x, reg 0x%x => 0x%x\n",
                          phyaddr, reg, vec[1]));
            }
        }
        break;

    case ETCPHYWR:
        if (vec) {
            ET_TRACE(("etc_ioctl: ETCPHYWR to reg 0x%x <= 0x%x\n", vec[0], vec[1]));
#ifdef QT_TEST
            (*etc->chops->phywr)(etc->ch, QT_MDIO_PHYADDR, vec[0], (uint16)vec[1]);
#else   /* QT_TEST */
            (*etc->chops->phywr)(etc->ch, etc->phyaddr, vec[0], (uint16)vec[1]);
#endif  /* QT_TEST */
        }
        break;

    case ETCPHYWR2:
        if (vec) {
            uint phyaddr, reg;
            phyaddr = vec[0] >> 16;
            if (phyaddr < MAXEPHY) {
                reg = vec[0] & 0xffff;
                (*etc->chops->phywr)(etc->ch, phyaddr, reg, (uint16)vec[1]);
                ET_TRACE(("etc_ioctl: ETCPHYWR2 to phy 0x%x, reg 0x%x <= 0x%x\n",
                          phyaddr, reg, vec[1]));
            }
        }
        break;
    case ETCCFPRD:
        (*etc->chops->cfprd)(etc->ch, arg);
        break;
    case ETCCFPWR:
        (*etc->chops->cfpwr)(etc->ch, arg);
        break;
    case ETCCFPFIELDRD:
        (*etc->chops->cfpfldrd)(etc->ch, arg);
        break;
    case ETCCFPFIELDWR:
        (*etc->chops->cfpfldwr)(etc->ch, arg);
        break;
    case ETCCFPUDFRD:
        (*etc->chops->cfpudfrd)(etc->ch, arg);
        break;
    case ETCCFPUDFWR:
        (*etc->chops->cfpudfwr)(etc->ch, arg);
        break;
    case ETCPKTMEMSET:
        if ((val != MEMORY_DDRRAM) && (val != MEMORY_SOCRAM) 
            && (val != MEMORY_PCIMEM))
            goto err;
        etc->pkt_mem = val;

        et_init(etc->et, ET_INIT_DEF_OPTIONS);
        break;
    case ETCPKTHDRMEMSET:
        if ((val != MEMORY_DDRRAM) && (val != MEMORY_SOCRAM) 
            && (val != MEMORY_PCIMEM))
            goto err;
        etc->pkthdr_mem = val;

        et_init(etc->et, ET_INIT_DEF_OPTIONS);
        break;
    case ETCRXRATE:
         if (vec) {
            uint chan, pps;
            chan = vec[0];
            pps = vec[1];
            error = (*etc->chops->rxrateset)(etc->ch, chan, pps);
            if (!error) {
                etc->rx_pps[chan] = pps;
            }
        } else {
            goto err;
        }
        break;
    case ETCTXRATE:
         if (vec) {
            uint chan, rate, burst;
            chan = vec[0];
            rate = vec[1];
            burst = vec[2];
            error = (*etc->chops->txrateset)(etc->ch, chan, rate, burst);
            if (!error) {
                etc->tx_rate[chan] = rate;
                etc->tx_burst[chan] = burst;
            }
        }else {
            goto err;
        }
        break;
    case ETCFLOWCTRLMODE:
        if (vec) {
            uint mode;

            mode = vec[0];
            error = (*etc->chops->flowctrlmodeset)(etc->ch, mode);
            if (!error) {
                etc->flowcntl_mode = mode;
                et_init(etc->et, ET_INIT_DEF_OPTIONS);
            }
        }else {
            goto err;
        }
        break;
    case ETCFLOWCTRLCPUSET:
        if (vec) {
            uint pause_on;

            pause_on = vec[0];
            error = (*etc->chops->flowctrlcpuset)(etc->ch, pause_on);
            if (!error) {
                etc->flowcntl_cpu_pause_on= pause_on;
            }
        }else {
            goto err;
        }
        break;
    case ETCFLOWCTRLAUTOSET:
        if (vec) {
            uint on_th, off_th;

            on_th = vec[0];
            off_th = vec[1];
            error = (*etc->chops->flowctrlautoset)(etc->ch, on_th, off_th);
            if (!error) {
                etc->flowcntl_auto_on_thresh= on_th;
                etc->flowcntl_auto_off_thresh= off_th;
            }
        }else {
            goto err;
        }
        break;
    case ETCFLOWCTRLRXCHANSET:
        if (vec) {
            uint rxchan, on_th, off_th;

            rxchan = vec[0];
            if (rxchan >= NUMRXQ) {
                goto err;
            }
            on_th = vec[1];
            off_th = vec[2];
            error = (*etc->chops->flowctrlrxchanset)(etc->ch, rxchan, on_th, off_th);
            if (!error) {
                etc->flowcntl_rx_on_thresh[rxchan]= on_th;
                etc->flowcntl_rx_off_thresh[rxchan]= off_th;
            }
        }else {
            goto err;
        }
        break;
    case ETCTPID:
         if (vec) {
            uint id, tpid;
            id = vec[0];
            tpid = vec[1];
            error = (*etc->chops->tpidset)(etc->ch, id, tpid);
            if (!error) {
                etc->tpid[id] = tpid;
            }
        } else {
            goto err;
        }
        break;
    case ETCPVTAG:
        if (vec) {
            uint ptag;
            ptag = vec[0];
            error = (*etc->chops->pvtagset)(etc->ch, ptag);
            if (!error) {
                etc->ptag = ptag;
            }
        } else {
            goto err;
        }
        break;

    /* RX separate header */
    case ETCRXSEPHDR:
        if (vec) {
            uint enable;
            uint i;
            enable = vec[0];
            error = (*etc->chops->rxsephdrset)(etc->ch, enable);
            if (!error) {
                for (i=0; i < NUMRXQ; i++) {
                    etc->en_rxsephdr[i]= enable;
                }
            }
        } else {
            goto err;
        }
        et_init(etc->et, ET_INIT_DEF_OPTIONS);
        break;
    case ETCTXQOSMODE:
        if (vec) {
            uint mode;

            mode = vec[0];
            error = (*etc->chops->txqosmodeset)(etc->ch, mode);
            if (!error) {
                etc->txqos_mode = mode;
            }
        }else {
            goto err;
        }
        break;
    case ETCTXQOSWEIGHTSET:
        if (vec) {
            uint queue, weight;

            queue = vec[0];
            if (queue >= NUMTXQ) {
                goto err;
            }
            weight = vec[1];
            error = (*etc->chops->txqosweightset)(etc->ch, queue, weight);
            if (!error) {
                etc->txqos_weight[queue]= weight;
            }
        }else {
            goto err;
        }
        break;


    default:
    err:
        error = -1;
    }

    return (error);
}

/* called once per second */
void
etc_watchdog(etc_info_t *etc)
{
    uint16 status;
    uint16 lpa;

    etc->now++;

    /* no local phy registers */
    if (etc->phyaddr == EPHY_NOREG) {
        etc->linkstate = TRUE;
        etc->duplex = 1;
        /* keep emac txcontrol duplex bit consistent with current phy duplex */
        (*etc->chops->duplexupd)(etc->ch);
        return;
    }

    status = (*etc->chops->phyrd)(etc->ch, etc->phyaddr, 1);
    /* check for bad mdio read */
    if (status == 0xffff) {
        ET_ERROR(("et%d: etc_watchdog: bad mdio read: phyaddr %d mdcport %d\n",
            etc->unit, etc->phyaddr, etc->mdcport));
        return;
    }

    if ((etc->forcespeed == ET_AUTO) && (!etc->ext_config)) {
        uint16 adv, adv2 = 0, status2 = 0, estatus;

        adv = (*etc->chops->phyrd)(etc->ch, etc->phyaddr, 4);
        lpa = (*etc->chops->phyrd)(etc->ch, etc->phyaddr, 5);

        /* read extended status register. if we are 1000BASE-T
         * capable then get our advertised capabilities and the
         * link partner capabilities from 1000BASE-T control and
         * status registers.
         */
        estatus = (*etc->chops->phyrd)(etc->ch, etc->phyaddr, 15);
        if ((estatus != 0xffff) && (estatus & EST_1000TFULL)) {
            /* read 1000BASE-T control and status registers */
            adv2 = (*etc->chops->phyrd)(etc->ch, etc->phyaddr, 9);
            status2 = (*etc->chops->phyrd)(etc->ch, etc->phyaddr, 10);
    }

    /* update current speed and duplex */
        if ((adv2 & ADV_1000FULL) && (status2 & LPA_1000FULL)) {
            etc->speed = 1000;
            etc->duplex = 1;
        } else if ((adv2 & ADV_1000HALF) && (status2 & LPA_1000HALF)) {
            etc->speed = 1000;
            etc->duplex = 0;
        } else if ((adv & ADV_100FULL) && (lpa & LPA_100FULL)) {
        etc->speed = 100;
        etc->duplex = 1;
    } else if ((adv & ADV_100HALF) && (lpa & LPA_100HALF)) {
        etc->speed = 100;
        etc->duplex = 0;
    } else if ((adv & ADV_10FULL) && (lpa & LPA_10FULL)) {
        etc->speed = 10;
        etc->duplex = 1;
    } else {
        etc->speed = 10;
        etc->duplex = 0;
    }
    }

    /* monitor link state */
    if (!etc->linkstate && (status & STAT_LINK)) {
        etc->linkstate = TRUE;
        if (etc->pm_modechange)
            etc->pm_modechange = FALSE;
        else
            et_link_up(etc->et);
    } else if (etc->linkstate && !(status & STAT_LINK)) {
        etc->linkstate = FALSE;
        if (!etc->pm_modechange)
            et_link_down(etc->et);
    }

    /* keep emac txcontrol duplex bit consistent with current phy duplex */
    (*etc->chops->duplexupd)(etc->ch);

    /* check for remote fault error */
    if (status & STAT_REMFAULT) {
        ET_ERROR(("et%d: remote fault\n", etc->unit));
    }

    /* check for jabber error */
    if (status & STAT_JAB) {
        ET_ERROR(("et%d: jabber\n", etc->unit));
    }

    /*
     * Read chip mib counters occationally before the 16bit ones can wrap.
     * We don't use the high-rate mib counters.
     */
    if ((etc->now % 30) == 0)
        (*etc->chops->statsupd)(etc->ch);
}

static void
etc_loopback(etc_info_t *etc, int on)
{
    ET_TRACE(("et%d: etc_loopback: %d\n", etc->unit, on));

    etc->loopbk = (bool) on;
    et_init(etc->et, ET_INIT_DEF_OPTIONS);
}

void
etc_promisc(etc_info_t *etc, uint on)
{
    ET_TRACE(("et%d: etc_promisc: %d\n", etc->unit, on));

    etc->promisc = (bool) on;
    et_init(etc->et, ET_INIT_DEF_OPTIONS);
}

void
etc_qos(etc_info_t *etc, uint on)
{
    ET_TRACE(("et%d: etc_qos: %d\n", etc->unit, on));

    etc->qos = (bool) on;
    et_init(etc->et, ET_INIT_DEF_OPTIONS);
}

#ifdef BCMDBG
void
etc_dump(etc_info_t *etc, struct bcmstrbuf *b)
{
    etc_dumpetc(etc, b);
    (*etc->chops->dump)(etc->ch, b);
}

static void
etc_dumpetc(etc_info_t *etc, struct bcmstrbuf *b)
{
    char perm[32], cur[32];
    uint i;

    printf("etc 0x%x et 0x%x unit %d msglevel %d speed/duplex %d%s\n",
        (ulong)etc, (ulong)etc->et, etc->unit, et_msg_level,
        etc->speed, (etc->duplex? "full": "half"));
    printf("up %d promisc %d loopbk %d forcespeed %d advertise 0x%x "
                   "needautoneg %d\n",
            etc->up, etc->promisc, etc->loopbk, etc->forcespeed, etc->advertise,
                   etc->needautoneg);
    printf("piomode %d pioactive 0x%x nmulticast %d allmulti %d qos %d\n",
 etc->piomode, (ulong)etc->pioactive, etc->nmulticast, etc->allmulti, etc->qos);
    printf("vendor 0x%x device 0x%x rev %d coreunit %d phyaddr %d mdcport %d\n",
        etc->vendorid, etc->deviceid, etc->chiprev,
        etc->coreunit, etc->phyaddr, etc->mdcport);

    printf("perm_etheraddr %s cur_etheraddr %s\n",
        bcm_ether_ntoa(&etc->perm_etheraddr, perm),
        bcm_ether_ntoa(&etc->cur_etheraddr, cur));

    if (etc->nmulticast) {
        printf("multicast: ");
        for (i = 0; i < etc->nmulticast; i++)
            printf("%s ", bcm_ether_ntoa(&etc->multicast[i], cur));
        printf("\n");
    }

    printf("linkstate %d\n", etc->linkstate);
    printf("\n");

    printf("flow control mode is ", etc->flowcntl_mode);
    switch (etc->flowcntl_mode) {
        case FLOW_CTRL_MODE_DISABLE:
            printf(" Disabled.\n");
            break;
        case FLOW_CTRL_MODE_AUTO:
            printf(" Auto mode.\n");
            break;
        case FLOW_CTRL_MODE_CPU:
            printf(" CPU mode.\n");
            break;
        case FLOW_CTRL_MODE_MIX:
            printf(" Auto and CPU mode.\n");
            break;
        default:
            printf("Unknow.\n");
            break;
    }
    
    printf("flow control auto mode : on threshod= %d, off threshold = %d\n", 
        etc->flowcntl_auto_on_thresh, etc->flowcntl_auto_off_thresh);
    printf("flow control cpu mode : pause_on = %d\n", 
        etc->flowcntl_cpu_pause_on);
    for (i=0; i < NUMRXQ; i++) {
        printf("RX channel flow control : channel %d, on threshod= %d, off threshold = %d\n", 
            i, etc->flowcntl_rx_on_thresh[i], etc->flowcntl_rx_off_thresh[i]);
        printf("RX separate header is %s\n", (etc->en_rxsephdr[i])?"enabled":"disabled");
    }
    printf("\n");
    
    printf("TX qos mode is ");
    switch (etc->txqos_mode) {
        case TXQOS_MODE_1STRICT_3WRR:
            printf ("Q3 Strict, Q2 WRR, Q1 WRR, Q0 WRR\n");
            break;
        case TXQOS_MODE_2STRICT_2WRR:
            printf ("Q3 Strict, Q2 Strict, Q1 WRR, Q0 WRR\n");
            break;
        case TXQOS_MODE_3STRICT_1WRR:
            printf ("Q3 Strict, Q2 Strict, Q1 Strict, Q0 WRR\n");
            break;
        case TXQOS_MODE_ALL_STRICT:
            printf ("Q3~Q0 all Strict\n");
            break;
        default:
            printf(" Unknow\n");
            break;
    }
    for (i=0; i < (NUMTXQ-1); i++) {
        printf("TX qos : queue %d, weight = %d\n", 
            i, etc->txqos_weight[i]);
    }
    printf("\n");

    /* TPID */
    for (i =0; i < NUM_STAG_TPID; i++) {
        printf("STAG TPID %d = 0x%x\n", i, etc->tpid[i]);
    }
    printf("\n");

    /* refresh stat counters */
    (*etc->chops->statsupd)(etc->ch);

    /* summary stat counter line */
    /* use sw frame and byte counters -- hw mib counters wrap too quickly */
    printf("txframe %d txbyte %d txerror %d rxframe %d rxbyte %d rxerror %d\n",
        etc->txframe, etc->txbyte, etc->txerror,
        etc->rxframe, etc->rxbyte, etc->rxerror);
    
    /* transmit & receive stat counters */
    /* hardware mib pkt and octet counters wrap too quickly to be useful */
    (*etc->chops->dumpmib)(etc->ch, (char *)b->buf);

    printf("txnobuf %d reset %d dmade %d dmada %d dmape %d\n",
                   etc->txnobuf, etc->reset, etc->dmade, etc->dmada, etc->dmape);

    /* hardware mib pkt and octet counters wrap too quickly to be useful */
    printf("rxnobuf %d rxdmauflo %d rxoflo %d rxbadlen %d\n",
                   etc->rxnobuf, etc->rxdmauflo, etc->rxoflo, etc->rxbadlen);

    printf("\n");
}
#endif /* BCMDBG */

uint
etc_totlen(etc_info_t *etc, void *p)
{
    uint total;

    total = 0;
    for (; p; p = PKTNEXT(etc->osh, p))
        total += PKTLEN(etc->osh, p);
    return (total);
}

#ifdef BCMDBG
void
etc_prhdr(char *msg, struct ether_header *eh, uint len, int unit)
{
    char da[32], sa[32];

    if (msg && (msg[0] != '\0'))
        printf("et%d: %s: ", unit, msg);
    else
        printf("et%d: ", unit);

    printf("dst %s src %s type 0x%x len %d\n",
        bcm_ether_ntoa((struct ether_addr *)eh->ether_dhost, da),
        bcm_ether_ntoa((struct ether_addr *)eh->ether_shost, sa),
        ntoh16(eh->ether_type),
        len);
}
void
etc_prhex(char *msg, uchar *buf, uint nbytes, int unit)
{
    if (msg && (msg[0] != '\0'))
        printf("et%d: %s:\n", unit, msg);
    else
        printf("et%d:\n", unit);

    prhex(NULL, buf, nbytes);
}
#endif /* BCMDBG */

uint32
etc_up2tc(uint32 up)
{
    extern uint32 up2tc[];

    /* Checking if the priority is overflow , return the max  queue number */ 
    if (up > MAXPRIO) {
        return TC_VO;
    }

    return (up2tc[up]);
}

uint32
etc_priq(uint32 txq_state)
{
    extern uint32 priq_selector[];

    /* Checking if the input value is overflow */
    if (txq_state > ((0x1 << NUMTXQ)-1)) {
        txq_state &= ((0x1 << NUMTXQ)-1); 
    }

    return (priq_selector[txq_state]);
}

