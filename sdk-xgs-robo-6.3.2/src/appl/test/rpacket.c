/*
 * $Id: rpacket.c 1.82 Broadcom SDK $
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
 * Packet receive test that uses the BCM api.
 *
 * December, 2002:  Options added to support RX APIs.
 *    This also adds:
 *        interrupt/non-interrupt handling;
 *        packet stealing;
 *        if stealing, freeing packets with DPC (always with interrupt,
 *               optionally with non-interrupt) or immediately;
 *
 *    When stealing packets, the system will crash if the rate is
 *    high enough:  Over 4K pkts/sec on Mousse, and 6K pkts/sec on BMW.
 *    This does not appear to be related to DPC.
 *
 *    Current max rates for RX API, no packet stealing:
 *       BMW (8245) on 5690, interrupt:           11.6 K pkts/sec
 *       BMW, non-interrupt:                      11.6 K pkts/sec
 *       Mousse, non-interrupt:                   10.4 K pkts/sec
 *
 *    Update on max rates for RX API:
 */

#include <sal/types.h>
#include <sal/core/time.h>
#include <sal/appl/sal.h>
#include <sal/core/dpc.h>

#include <soc/util.h>
#include <soc/drv.h>
#ifdef BCM_FE2000_SUPPORT
#include <soc/sbx/sbx_drv.h>
#ifdef BCM_FE2000_SUPPORT
#ifdef BCM_FE2000_P3_SUPPORT
#include <soc/sbx/g2p3/g2p3.h>
#endif /* def BCM_FE2000_P3_SUPPORT */
#endif /* def BCM_FE2000_SUPPORT */
#endif

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/shell.h>

#include <bcm/error.h>
#include <bcm/l2.h>
#include <bcm/mcast.h>
#include <bcm/port.h>
#include <bcm/link.h>
#include <bcm/field.h>
#include <bcm/rx.h>
#include <bcm/pkt.h>
#include <bcm/stack.h>
#include <bcm/vlan.h>

#include <bcm_int/common/rx.h>
#include <bcm_int/common/tx.h>
#if defined (BCM_PETRA_SUPPORT)
#include <soc/dpp/drv.h>
#endif

#include "testlist.h"

#if defined(BCM_FIELD_SUPPORT) || defined(BCM_FE2000_SUPPORT) || \
    defined(BCM_ROBO_SUPPORT) || defined (BCM_PETRA_SUPPORT)

typedef struct p_s {
    int                         p_init; /* TRUE --> initialized */
    int                         p_port[2]; /* Active port */
    int                         p_time; /* Duration (seconds) */
    int                         p_done; /* Finished test - just drain */
    bcm_port_info_t             p_port_info[2]; /* saved port config */
    int                         p_pkts_per_sec;
    int                         p_max_q_len;
    int                         p_per_cos; /* Run tests with per-cos settings */
    int                         p_burst;
    uint32                      reg_flags;  /* Flags used for register */
    int                         p_intr_cb;
    int                         p_dump_rx;
#if defined(BCM_FIELD_SUPPORT)
    bcm_field_entry_t           p_field_entry[2];
#endif /* BCM_FIELD_SUPPORT */
    volatile int                p_cnt_done; /* # complete */
    int                         p_l_start; /* Length start */
    int                         p_l_end; /* Length end */
    int                         p_l_inc; /* Length increment */
    int                         p_free_buffer;
    bcm_pkt_t                   *p_pkt; /* Tx - Packet buffer */
    volatile int                p_received;
    bcm_rx_cfg_t                p_rx_cfg;
    int                         p_txrx_unit;
} p_t;

static sal_mac_addr_t rp_mac_dest = {0x00, 0x11, 0x22, 0x33, 0x44, 0x99};
static sal_mac_addr_t rp_mac_src  = {0x00, 0x11, 0x22, 0x33, 0x44, 0x66};



static uint8 mypacket[] = {


/*
                          0x0,0x0,0x0,0x0,       0x0,0x1,0x0,0x0,
                          0x0,0x0,0x0,0xe2,      0x81,0x00,0x00,0x00,
                          
                          0x90,0x00,*/0x1,0x2, 




/*                          0x0,0x0,0x0,0x0,       0x0,0x1,0x0,0x0,
                          0x0,0x0,0x0,0xe2,      0x90,0x00,0x1,0x2,
                          
                          0x03,0x4,0x5,0x6, */     0x7,0x8,0x9,0xa,
                          0xb,0xc,0xd,0xe,       0xf,0x10,0x11,0x12,
                          
                          0x13,0x14,0x15,0x16,   0x17,0x18,0x19,0x1a,
                          0x1b,0x1c,0x1d,0x1e,   0x1f,0x20,0x21,0x22,
                          
                          0x23,0x24,0x25,0x26,   0x27,0x28,0x29,0x2a,
                          0x2b,0x2c,0x2d,0x2e,   0x2f,0x30,0x31,0x32,
                          
                          0x33,0x34,0x35,0x36,   0x37,0x38,0x39,0x3a,
                          0x3b,0x3c,0x3d,0x3e,   0x3f,0x40,0x41,0x42,

                          0x43,0x44,0x45,0x46,   0x47,0x48,0x49,0x4a,
                          0x4b,0x4c,0x4d,0x4e,   0x4f,0x50,0x51,0x52,


                          0x53,0x54,0x55,0x56,   0x57,0x58,0x59,0x5a,
                          0x5b,0x5c,0x5d,0x5e,   0x5f,0x60,0x61,0x62,

                          0x63,0x64,0x65,0x66,   0x67,0x68,0x69,0x6a,
                          0x6b,0x6c,0x6d,0x6e,   0x6f,0x70,0x71,0x72,

                          0x73,0x74,0x75,0x76,   0x77,0x78,0x79,0x7a,
                          0x7b,0x7c,0x7d,0x7e,   0x7f,0x80,0x81,0x82 };



static p_t *p_control[SOC_MAX_NUM_DEVICES];

#define RP_RECEIVER_PRIO                255    /* Priority of rx handler */
#define RP_MAX_PKT_LENGTH               2048
#define RP_RX_CHANNEL                   1      /* DMA channel */

#ifdef BCM_ROBO_SUPPORT
/* Configurate rate value */ 
#define IMP_MIN_RATE_PPS   384    /* Packet Per Second */
#define IMP_MIN_RATE_BPS   256    /* KBits Per Second */
#define IMP_BURST_SIZE 48

static int imp_rate_pps = IMP_MIN_RATE_PPS;
static int imp_rate_default = 0;
static int imp_burst_size_default = 0;

static bcm_port_info_t  port_info[SOC_MAX_NUM_DEVICES][SOC_MAX_NUM_PORTS];
#endif /* BCM_ROBO_SUPPORT */

#define RP_CHK(rv, f)                      \
   if (BCM_FAILURE((rv))) {                \
       printk("call to %s line %d failed:%d %s\n", #f, __LINE__,\
                                       (rv), bcm_errmsg((rv))); \
    }


#define RP_MAX_PPC SOC_DV_PKTS_MAX
#ifdef BCM_FE2000_SUPPORT
#undef RP_MAX_PPC
#define RP_MAX_PPC SBX_MAX_RXLOAD_FIFO
#endif

/*
 * The following function may be called in interrupt context.
 * Does no packet classification; assume we're to see all pkts.
 */

STATIC bcm_rx_t
rpacket_rx_receive(int unit, bcm_pkt_t *pkt, void *vp)
{
    p_t         *p = (p_t *)vp;

    if (p->p_done) {
        /*
         * Just drain packets.
         */
        return BCM_RX_HANDLED;
    }

    p->p_received++;                    /* Count it */

    if (p->p_free_buffer) {
        if (p->p_intr_cb) { /* Queue in interrupt context */
            bcm_rx_free_enqueue(unit, pkt->_pkt_data.data);
        } else {
            bcm_rx_free(unit, pkt->alloc_ptr);
        }
        return BCM_RX_HANDLED_OWNED;
    }

    return BCM_RX_HANDLED;
}

STATIC INLINE int
rpacket_register(int unit, p_t *p)
{
    if (SOC_IS_ARAD(unit)) {
        return _bcm_common_rx_register(unit, "rpkt-rx", rpacket_rx_receive, RP_RECEIVER_PRIO, p, p->reg_flags);

    } else {
        return bcm_rx_register(unit, "rpkt-rx", rpacket_rx_receive,
                           RP_RECEIVER_PRIO, p, p->reg_flags);
	}
}

STATIC INLINE int
rpacket_unregister(int unit, p_t *p)
{
    if (SOC_IS_ARAD(unit)) {
        return _bcm_common_rx_unregister(unit, rpacket_rx_receive, RP_RECEIVER_PRIO);
    } else {
        return bcm_rx_unregister(unit, rpacket_rx_receive, RP_RECEIVER_PRIO);
    }
}

STATIC int
rpacket_receiver_activate(int unit, p_t *p)
{
    int rv;

    /* Set up common attributes first */
    if (bcm_rx_active(unit)) {
        printk("Stopping active RX.\n");
        rv = bcm_rx_stop(unit, NULL);
        if (!BCM_SUCCESS(rv)) {
            printk("Unable to stop RX: %s\n", bcm_errmsg(rv));
            return -1;
        }
    }

    if (!soc_feature(unit, soc_feature_packet_rate_limit)) {
        /* Only set the burst size if the unit doesn't have
         * CPU packet rate limiting feature in HW. Otherwise,
         * packets are dropped causing the test to fail.
         */
        if ( SOC_IS_ARAD(unit)) { 
            rv = _bcm_common_rx_burst_set(unit, p->p_burst);
        } else {
            rv = bcm_rx_burst_set(unit, p->p_burst);
        }
        if (BCM_FAILURE(rv)) {
            printk("Unable to set RX burst limit: %s\n",
                   bcm_errmsg(rv));
        }
    }

    if (p->p_per_cos) {
        bcm_rx_cos_rate_set(unit, 0, p->p_pkts_per_sec);
        bcm_rx_cos_burst_set(unit, 0, p->p_burst);
        p->p_rx_cfg.global_pps = 0;
        p->p_rx_cfg.max_burst = 0;
    } else {
        bcm_rx_cos_rate_set(unit, BCM_RX_COS_ALL, 0);
        bcm_rx_cos_burst_set(unit, BCM_RX_COS_ALL, 0);
        p->p_rx_cfg.global_pps = p->p_pkts_per_sec;
        p->p_rx_cfg.max_burst = p->p_burst;
    }
    if (p->p_max_q_len >= 0) {
        printk("Setting MAX Q length to %d\n", p->p_max_q_len);
        bcm_rx_cos_max_len_set(unit, BCM_RX_COS_ALL, p->p_max_q_len);
    }
    if (SOC_IS_ARAD(unit)) {
        rv = _bcm_common_rx_start(unit, &p->p_rx_cfg  );
    } else {
        rv = bcm_rx_start(unit, &p->p_rx_cfg  );
    }
    if (!BCM_SUCCESS(rv)) {
        printk("Unable to Start RX: %s\n", bcm_errmsg(rv));
        return -1;
    }

    
#ifdef BCM_FE2000_SUPPORT
    p->reg_flags = BCM_RCO_F_ALL_COS;
#else
    p->reg_flags = BCM_RCO_F_ALL_COS +
        (p->p_intr_cb ? BCM_RCO_F_INTR : 0);
#endif /* BCM_FE2000_SUPPORT */

    return 0;
}



STATIC int
rpacket_setup(int unit, p_t *p)
{
    int         got_port;
    int         rv = BCM_E_UNAVAIL;
    uint8       *fill_addr;
    soc_port_t  port;
    int         num_test_ports = 1;
    int         port_idx;
    pbmp_t      gxe_pbm;
    pbmp_t      tmp_pbmp;
    int         speed = 0;
    int         txrx_modid = 0;
#ifdef BCM_ROBO_SUPPORT
#if defined(BCM_FIELD_SUPPORT)
    pbmp_t      redirect_pbmp;
#endif /* BCM_FIELD_SUPPORT */
#endif /* BCM_ROBO_SUPPORT */
#ifdef BCM_FE2000_SUPPORT
    int         modid;
#ifdef BCM_FE2000_P3_SUPPORT
    soc_sbx_g2p3_epv2e_t p3epv2e;
#endif /* def BCM_FE2000_P3_SUPPORT */
#endif /* def BCM_FE2000_SUPPORT */
#if defined(BCM_NORTHSTAR_SUPPORT)||defined(BCM_NORTHSTARPLUS_SUPPORT)
     pbmp_t wan_pbmp;
#endif /* BCM_NORTHSTAR_SUPPORT||BCM_NORTHSTARPLUS_SUPPORT */

    p->p_txrx_unit = unit;

#ifdef BCM_FE2000_SUPPORT
    if (SOC_IS_SBX_FE(unit)) {
        num_test_ports = 2;
    }
#endif /* def BCM_FE2000_SUPPORT */

#ifdef BCM_ROBO_SUPPORT
    /*
     * Pick a port, and redirect the packet back out that port with the
     * FFP (for bcm5395 and bcm53115).  On other ROBO chips, 2 ports are chosen, 
     * and redirect one to the other.
     */
    if (SOC_IS_ROBO(unit)) {
        if (SOC_IS_VULCAN(unit) ||
            SOC_IS_STARFIGHTER(unit) || SOC_IS_POLAR(unit) || 
            SOC_IS_NORTHSTAR(unit) || SOC_IS_NORTHSTARPLUS(unit)) {
            num_test_ports = 1;
        } else {
            num_test_ports = 2;
        }
    }
#endif /* def BCM_ROBO_SUPPORT */

    /*
     * Pick a port, and redirect the packet back out that port with the
     * FFP.  On SBX, 2 ports are chosen, and redirect one to the other.  Single
     * port redirection drops packets on ingress due to the split horizon check
     */

    got_port = FALSE;
    BCM_PBMP_ASSIGN(gxe_pbm, PBMP_GE_ALL(unit));
    BCM_PBMP_OR(gxe_pbm, PBMP_XE_ALL(unit));

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(unit)) {
        if (SOC_IS_VO(unit)) {
            BCM_PBMP_REMOVE(gxe_pbm,SOC_INFO(unit).gmii_pbm);
        }
       /* If there are no GE ports, then selects FE ports for some ROBO chips */
        if (BCM_PBMP_IS_NULL(gxe_pbm)) {
            BCM_PBMP_ASSIGN(gxe_pbm, PBMP_FE_ALL(unit));
        }
    }
#endif /* def BCM_ROBO_SUPPORT */

#if defined(BCM_NORTHSTAR_SUPPORT)||defined(BCM_NORTHSTARPLUS_SUPPORT)
    if (SOC_IS_NORTHSTAR(unit)||SOC_IS_NORTHSTARPLUS(unit)) {
        wan_pbmp = soc_property_get_pbmp(unit, spn_PBMP_WAN_PORT, 0);
        SOC_PBMP_AND(wan_pbmp, PBMP_ALL(unit));
    }
#endif /* BCM_NORTHSTAR_SUPPORT||BCM_NORTHSTARPLUS_SUPPORT */


    port_idx = 0;
    PBMP_ITER(gxe_pbm, port) {
        if (IS_ST_PORT(unit, port)) {
            continue;
        }
#if defined(BCM_NORTHSTAR_SUPPORT)||defined(BCM_NORTHSTARPLUS_SUPPORT)
        if (SOC_IS_NORTHSTAR(unit)||SOC_IS_NORTHSTARPLUS(unit)) {
            if (SOC_PBMP_MEMBER(wan_pbmp, port)) {
                soc_cm_print("Skip WAN port(port%d)\n",port);
                continue;
            }
        }
#endif /* BCM_NORTHSTAR_SUPPORT||BCM_NORTHSTARPLUS_SUPPORT */
 
        rv = bcm_port_info_save(unit, port, &p->p_port_info[port_idx]);
        RP_CHK(rv, bcm_port_info_save);

        if (BCM_SUCCESS(rv)) {
            rv = bcm_port_linkscan_set(unit, port, BCM_LINKSCAN_MODE_NONE);
            RP_CHK(rv, bcm_port_linkscan_set);
        }
#ifdef BCM_PETRA_SUPPORT        
	    if ((SOC_DPP_PP_ENABLE(unit) && SOC_IS_ARAD(unit)) || (!SOC_IS_ARAD(unit))) 
#endif            
        {
	        if (BCM_SUCCESS(rv)) {
	            rv = bcm_port_speed_max(unit, port, &speed);
	            RP_CHK(rv, bcm_port_speed_max);
	        }
	        if (BCM_SUCCESS(rv)) {
	            rv = bcm_port_speed_set(unit, port, speed);
	            RP_CHK(rv, bcm_port_speed_set);
	        }
        }
        if (BCM_SUCCESS(rv)) {
           if (SOC_IS_ROBO(unit)) {
#ifdef BCM_ROBO_SUPPORT
               rv = bcm_port_loopback_set(unit, port, BCM_PORT_LOOPBACK_PHY);
#endif /* BCM_ROBO_SUPPORT */
           } else {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_FE2000_SUPPORT) || defined(BCM_PETRA_SUPPORT)
               rv = bcm_port_loopback_set(unit, port, BCM_PORT_LOOPBACK_MAC);
#endif /* BCM_ESW_SUPPORT  || defined(BCM_FE2000_SUPPORT) */
           }
           RP_CHK(rv, bcm_port_loopback_set);
        }
        if (BCM_SUCCESS(rv)) {
            rv = bcm_port_pause_set(unit, port, 0, 0);
            RP_CHK(rv, bcm_port_pause_set);
        }

        if ((got_port = BCM_SUCCESS(rv))) {
            p->p_port[port_idx++] = port;

            if (port_idx >= num_test_ports) {
                break;
            }
            got_port = BCM_E_NOT_FOUND;            
        }
    }

    if (!got_port) {
        test_error(unit, "Unable to find suitable XE/GE port.\n");
        return -1;
    }
    
#ifdef BCM_FE2000_SUPPORT
    if (SOC_IS_SBX_FE(unit)) {
        int node_id, fab_port; /* don't cares, just want the QE's Mod & unit */

        rv = bcm_stk_modid_get(unit, &modid);
        if (BCM_FAILURE(rv)) {
            test_error(unit, "Failed to find module id\n");
            return -1;
        }

        rv = soc_sbx_node_port_get (unit, modid, p->p_port[0] ,
                                    &p->p_txrx_unit,
                                    &node_id, &fab_port);
        if (BCM_FAILURE(rv)) {
            test_error(unit, "Failed to find QE Module\n");
            return -1;
        }
        SOC_SBX_MODID_FROM_NODE(node_id, txrx_modid);

    /*
     *  Set the egress port/vid to strip so the mac header doesn't accumulate
     *  to an uncontrollable size.  By default, the RCE forward does not strip,
     *  an the header is added on egress, for each packet
     */
    switch (SOC_SBX_CONTROL(unit)->ucodetype) {
#ifdef BCM_FE2000_P3_SUPPORT
    case SOC_SBX_UCODE_TYPE_G2P3:
        for (port_idx = 0; port_idx < num_test_ports; port_idx++) {
                       rv = soc_sbx_g2p3_epv2e_get(unit, 
                                                   1, 
                                                   p->p_port[port_idx], 
                                                   &p3epv2e);
            p3epv2e.strip = TRUE;
                       rv = soc_sbx_g2p3_epv2e_set(unit, 
                                                   1, 
                                                   p->p_port[port_idx], 
                                                   &p3epv2e);
        }
        break;
#endif /* def BCM_FE2000_P3_SUPPORT */
    default:
        test_error(unit, "Unsupported microcode on FE2K unit\n");
        return -1;
    }
    }
#endif /* def BCM_FE2000_SUPPORT */

    /* Disable all other ports (can run with active links plugged in) */
    BCM_PBMP_ASSIGN(tmp_pbmp, PBMP_E_ALL(unit));
    for (port_idx=0; port_idx < num_test_ports; port_idx++) {
        BCM_PBMP_PORT_REMOVE(tmp_pbmp, p->p_port[port_idx]);
    }

    PBMP_ITER(tmp_pbmp, port) {
#ifdef BCM_ROBO_SUPPORT
#if defined(BCM_NORTHSTAR_SUPPORT)||defined(BCM_NORTHSTARPLUS_SUPPORT)
        if (SOC_IS_NORTHSTAR(unit)||SOC_IS_NORTHSTARPLUS(unit)) {
            if (SOC_PBMP_MEMBER(wan_pbmp, port)) {
                soc_cm_print("Skip storing WAN port(%d) status\n", port);
                continue;
            }
        }
#endif /* BCM_NORTHSTAR_SUPPORT||BCM_NORTHSTARPLUS_SUPPORT */

       /* Save the current settings of non-testing ports  */
       if (SOC_IS_ROBO(unit)) {
            rv = bcm_port_info_save(unit, port, &port_info[unit][port]);
            RP_CHK(rv, bcm_port_info_save);
        }
#endif /* BCM_ROBO_SUPPORT */
        rv = bcm_port_enable_set(unit, port, FALSE);
        RP_CHK(rv, bcm_port_enable_set);
    }

#if defined(BCM_FIELD_SUPPORT)
    if (soc_feature(unit, soc_feature_field)) {
        if (SOC_IS_ESW(unit)) {
#if defined(BCM_ESW_SUPPORT) || defined(BCM_FE2000_SUPPORT)
            bcm_field_qset_t        qset;
            bcm_field_group_t       fg;
            bcm_field_entry_t       fent[2];
            int                     stat_id = -1;
            bcm_field_stat_t        stat_type = bcmFieldStatPackets;
    
    
            BCM_FIELD_QSET_INIT(qset);
    
#ifdef BCM_FE2000_SUPPORT
            if (SOC_IS_SBX_FE(unit)) {
                BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);
                BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngressQoS);
            } else
#endif /* BCM_FE2000_SUPPORT */
            {
                BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPort);
            }
    
            rv = bcm_field_group_create(unit, qset, BCM_FIELD_GROUP_PRIO_ANY, &fg);
    
            for (port_idx=0; port_idx < num_test_ports; port_idx++) {
                if (BCM_SUCCESS(rv)) {
                    rv = bcm_field_entry_create(unit, fg, &fent[port_idx]);
                }
    
                if (BCM_SUCCESS(rv)) {
                    p->p_field_entry[port_idx] = fent[port_idx];
    
#ifdef BCM_FE2000_SUPPORT
                    if (SOC_IS_SBX_FE(unit)) {
                        BCM_PBMP_CLEAR(tmp_pbmp);
                        BCM_PBMP_PORT_ADD(tmp_pbmp, p->p_port[port_idx]);
    
                        rv = bcm_field_qualify_InPorts(unit, fent[port_idx],
                                                       tmp_pbmp, PBMP_ALL(unit));
                        RP_CHK(rv, bcm_field_qualify_InPorts);
                    } else
#endif /* BCM_FE2000_SUPPORT */
                    {
                        rv = bcm_field_qualify_InPort(unit, fent[port_idx],
                                                      p->p_port[port_idx],
                                                      BCM_FIELD_EXACT_MATCH_MASK);
                    }
                }
            }
    
            if (BCM_SUCCESS(rv)) {
                for (port_idx=0; port_idx <  num_test_ports; port_idx++) {
                    rv = bcm_field_stat_create(unit, fg, 1, &stat_type, &stat_id);
                    RP_CHK(rv, bcm_field_stat_create);

                    rv = bcm_field_entry_stat_attach(unit, fent[port_idx], stat_id);
                    RP_CHK(rv, bcm_field_entry_stat_attach);
                    stat_id = -1;
                }
            }
    
            for (port_idx=0; port_idx < num_test_ports; port_idx++) {

                if (BCM_SUCCESS(rv)) {
                    rv = bcm_field_action_add(unit, fent[port_idx], 
                                              bcmFieldActionCopyToCpu,
                                              0, 0);
                    RP_CHK(rv, bcm_field_action_add);
                }

                if (BCM_SUCCESS(rv)) {

                    int port = p->p_port[port_idx];
#ifdef BCM_FE2000_SUPPORT
                    if (SOC_IS_SBX_FE(unit)) {
                        port = p->p_port[!port_idx];
                    }
#endif /* BCM_FE2000_SUPPORT */
                    rv = bcm_field_action_add(unit, fent[port_idx], 
                                              bcmFieldActionRedirect,
                                              0, port);
                    RP_CHK(rv, bcm_field_action_add);
                }
            }
#endif /* BCM_ESW_SUPPORT || BCM_FE2000_SUPPORT */
        } else {
#ifdef BCM_ROBO_SUPPORT
            if (SOC_IS_VULCAN(unit) ||
                SOC_IS_STARFIGHTER(unit) || SOC_IS_POLAR(unit) || 
                SOC_IS_NORTHSTAR(unit) || SOC_IS_NORTHSTARPLUS(unit)) {
                bcm_field_qset_t        qset;
                bcm_field_group_t       fg;
                bcm_field_entry_t       fent[2];     
                int                     stat_id = -1;
                bcm_field_stat_t        stat_type = bcmFieldStatPackets;
        
                BCM_FIELD_QSET_INIT(qset);
        
                BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPort);
        
                if (SOC_IS_VULCAN(unit) || SOC_IS_STARFIGHTER(unit) ||
                    SOC_IS_POLAR(unit) || SOC_IS_NORTHSTAR(unit) ||
                    SOC_IS_NORTHSTARPLUS(unit)) {
                    BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);
                }
    
                rv = bcm_field_group_create(unit, qset, BCM_FIELD_GROUP_PRIO_ANY, &fg);
        
                for (port_idx=0; port_idx < num_test_ports; port_idx++) {
                    if (BCM_SUCCESS(rv)) {
                        rv = bcm_field_entry_create(unit, fg, &fent[port_idx]);
                    }
        
                    if (BCM_SUCCESS(rv)) {
                        p->p_field_entry[port_idx] = fent[port_idx];
    
                        rv = bcm_field_qualify_InPort(unit, fent[port_idx],
                                                      p->p_port[port_idx],
                                                      BCM_FIELD_EXACT_MATCH_MASK);
                    }
                }
        
                /* For BCM53115 : the default qualify IP_type is IPv4.
                 * Tx a packet(NonIP type) from CPU : need to add bcmFieldIpTypeNonIP to qualify.
                 */
                if (SOC_IS_VULCAN(unit) || SOC_IS_STARFIGHTER(unit) ||
                    SOC_IS_POLAR(unit) || SOC_IS_NORTHSTAR(unit) ||
                    SOC_IS_NORTHSTARPLUS(unit)) {
                    if (BCM_SUCCESS(rv)) {
                        rv = bcm_field_qualify_IpType(unit, fent[0], bcmFieldIpTypeNonIp);
                    }
                }
        
                if (BCM_SUCCESS(rv)) {
                    for (port_idx=0; port_idx <  num_test_ports; port_idx++) {

                        rv = bcm_field_stat_create(unit, fg, 1, &stat_type, &stat_id);
                        RP_CHK(rv, bcm_field_stat_create);

                        rv = bcm_field_entry_stat_attach(unit, fent[port_idx], stat_id);
                        RP_CHK(rv, bcm_field_entry_stat_attach);
                        stat_id = -1;
                    }
                }
        
                /* ROBO chips :
                 *  The bcmFieldActionCopyToCpu and bcmFieldActionRedirect actions are incompatible,
                 *  using bcmFieldActionRedirectPbmp to do the same behavior.
                 *  Now support bcmFieldActionRedirectPbmp : BCM5395, BCM53115.
                 */
                BCM_PBMP_ASSIGN(redirect_pbmp, PBMP_CMIC(unit));
                for (port_idx=0; port_idx < num_test_ports; port_idx++) {
                    BCM_PBMP_PORT_ADD(redirect_pbmp, p->p_port[port_idx]);
                }
                if (BCM_SUCCESS(rv)) {
                    rv = bcm_field_action_add(unit, fent[0], bcmFieldActionRedirectPbmp,
                                                SOC_PBMP_WORD_GET(redirect_pbmp, 0), 0);
                }
                /* For BCM53115 : need to enable bcmFieldActionLoopback 
                 * if forwarding the packet to the receiving.
                 */
                if (SOC_IS_VULCAN(unit) || SOC_IS_STARFIGHTER(unit) ||
                    SOC_IS_POLAR(unit) || SOC_IS_NORTHSTAR(unit) ||
                    SOC_IS_NORTHSTARPLUS(unit)) {
                    if (BCM_SUCCESS(rv)) {
                        rv = bcm_field_action_add(unit, fent[0], bcmFieldActionLoopback,
                                                    TRUE, 0);
                    }
                }
            }
#endif
        }
    }

#endif /* BCM_FIELD_SUPPORT */
    if (BCM_FAILURE(rv)) {
        test_error(unit, "Unable to configure filter: %s\n", bcm_errmsg(rv));
        return -1;
    }

    /* Disable learning on that port */
#ifdef BCM_PETRA_SUPPORT        
	if ((SOC_DPP_PP_ENABLE(unit) && SOC_IS_ARAD(unit)) || (!SOC_IS_ARAD(unit))) 
#endif        
    {
	    for (port_idx=0; port_idx < num_test_ports; port_idx++) {
	        rv = bcm_port_learn_set(unit, p->p_port[port_idx], BCM_PORT_LEARN_FWD);

	#ifdef BCM_FE2000_SUPPORT
	        /* SBX controls learning per vlan, not per port */
	        if (BCM_FAILURE(rv)) {
	            bcm_vlan_control_vlan_t vlan_ctl;
	            rv = bcm_vlan_control_vlan_get(unit, 1, &vlan_ctl);
	            vlan_ctl.flags |= BCM_VLAN_LEARN_DISABLE;
	            rv = bcm_vlan_control_vlan_set(unit, 1, vlan_ctl);
	            RP_CHK(rv, bcm_vlan_control_vlan_set);

	        }
	#endif /* BCM_FE2000_SUPPORT */
	    }
    }
    /* Create Maximum size packet requested */
    bcm_pkt_alloc(p->p_txrx_unit , p->p_l_end, 0, &(p->p_pkt));
    if (p->p_pkt == NULL) {
        test_error(unit, "Failed to allocate Tx packet\n");
        return -1;
    }


    /* Fill in buffer */
    BCM_PKT_HDR_DMAC_SET(p->p_pkt, rp_mac_dest);
    BCM_PKT_HDR_SMAC_SET(p->p_pkt, rp_mac_src);
    BCM_PKT_HDR_TPID_SET(p->p_pkt, ENET_DEFAULT_TPID);
    BCM_PKT_HDR_VTAG_CONTROL_SET(p->p_pkt, VLAN_CTRL(0, 0, 1));

    /* Fill address starts 2 * MAC + TPID + VTAG */
    fill_addr = BCM_PKT_DMAC(p->p_pkt) + 6 + 6 + 2 + 2;
    if (SOC_IS_ARAD(unit)) {
        sal_memcpy( fill_addr, mypacket, p->p_l_end -
                   (fill_addr - BCM_PKT_DMAC(p->p_pkt)));
    } else {
        sal_memset(fill_addr, 0xff, p->p_l_end -
                   (fill_addr - BCM_PKT_DMAC(p->p_pkt)));
    }

    /* Set the dest port for the packet 
     *  For all devices, the packet will target the first port
     */
    BCM_PKT_PORT_SET(p->p_pkt, p->p_port[0], FALSE, FALSE);
    p->p_pkt->dest_mod = txrx_modid;
    p->p_pkt->flags = BCM_TX_CRC_REGEN;
    p->p_pkt->opcode = BCM_PKT_OPCODE_UC;

    /* Set up the proper configuration */
    rv = rpacket_receiver_activate(p->p_txrx_unit, p);
    if (!BCM_SUCCESS(rv)) {
        test_error(unit, "Could not setup receiver\n");
        return -1;
    }

    return 0;
}


/*
 * Function:
 *      rpacket_test
 * Purpose:
 *      Test packet reception interface.
 * Parameters:
 *      u - unit #.
 *      a - pointer to arguments.
 *      pa - ignored cookie.
 * Returns:
 *      0 on success, -1 on failure
 * Notes:
 *      There remains an issue when stealing packets that the
 *      system will crash when the rate is high enough.  This has
 *      been experimentally determined to be between 4-5K pkts/sec
 *      on Mousse (8240) and 6-7K pkts/sec on BMW (8245).
 */

int
rpacket_test(int unit, args_t *a, void *pa)
{
    p_t         *p = (p_t *)pa;
    int         l, rv = BCM_E_UNAVAIL;
    int         pps;
    COMPILER_DOUBLE     time_start, time_end;
    COMPILER_DOUBLE     td, bps;
    int td_int, td_frag;
    int test_rv = 0;
#if defined(BCM_ROBO_SUPPORT) || defined(BCM_FIELD_SUPPORT)
    int num_test_ports = 1;
    int port_idx = 0;
#endif
    int tx_pkt_idx;
    int tx_pkt_cnt = 1;

#ifdef BCM_FE2000_SUPPORT
    if (SOC_IS_SBX_FE(unit)) {
#if defined(BCM_FIELD_SUPPORT)
        num_test_ports = 2;
#endif
        tx_pkt_cnt = 5;
    }
#endif /* BCM_FE2000_SUPPORT */

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(unit)) {
        if (SOC_IS_VULCAN(unit) ||
            SOC_IS_STARFIGHTER(unit) || SOC_IS_POLAR(unit) || 
            SOC_IS_NORTHSTAR(unit) || SOC_IS_NORTHSTARPLUS(unit)) {
            num_test_ports = 1;
        } else {
            rv = BCM_E_NONE;
            num_test_ports = 2;
        }
    }
#endif /* BCM_ROBO_SUPPORT */

    printk("\n"
           "Rate: %d%s. %s. %sIntr. %d PPC. Packets are%s freed. Burst %d\n",
           p->p_pkts_per_sec,
           p->p_pkts_per_sec ? "" : " (no limit)",
           "RX",
           p->p_intr_cb ? "" : "Non-",
           p->p_rx_cfg.pkts_per_chain,
           p->p_free_buffer ? "" : " not",
           p->p_burst);
    printk("\n"
           "  Packet |   Total   |  Time   |       Rate       \n"
           "   Size  |  Packets  |  (Sec)  |   p/s   |  MB/s  \n"
           " --------+-----------+---------+---------+--------\n");
    rv = 0;
    /* Transmit packets */
    for (l = p->p_l_start; l <= p->p_l_end; l += p->p_l_inc) {
#if defined(BCM_FIELD_SUPPORT)
        if (soc_feature(unit, soc_feature_field)) {
            if (SOC_IS_ESW(unit)) {
                for (port_idx=0; port_idx < num_test_ports; port_idx++) {
                    rv = bcm_field_entry_install(unit, p->p_field_entry[port_idx]);
                    RP_CHK(rv, bcm_field_entry_install);
                }
            } else {
#ifdef BCM_ROBO_SUPPORT
                if (SOC_IS_VULCAN(unit) ||
                    SOC_IS_STARFIGHTER(unit) || SOC_IS_POLAR(unit) || 
                    SOC_IS_NORTHSTAR(unit) || SOC_IS_NORTHSTARPLUS(unit)) {
                    for (port_idx=0; port_idx < num_test_ports; port_idx++) {
                        rv = bcm_field_entry_install(unit, p->p_field_entry[port_idx]);
                        RP_CHK(rv, bcm_field_entry_install);
                    }
                }
#endif /* BCM_ROBO_SUPPORT */
            }
        }
#endif
        if (BCM_FAILURE(rv)) {
            test_error(unit, "Unable to install filter: %s\n", bcm_errmsg(rv));
            test_rv = -1;
            goto done;
        }

        /* Register the callback handler */
        rv = rpacket_register(p->p_txrx_unit, p);
        if (BCM_FAILURE(rv)) {
            test_error(unit, "Unable to register handler, iter %d: %s\n",
                       l, bcm_errmsg(rv));
            test_rv = -1;
            goto done;
        }

        p->p_received = 0;              /* Start clean */

#ifdef BCM_ROBO_SUPPORT
        if (SOC_IS_ROBO(unit)) {
            if ((!SOC_IS_VULCAN(unit)) &&
                (!SOC_IS_STARFIGHTER(unit)) && (!SOC_IS_POLAR(unit)) && 
                (!SOC_IS_NORTHSTAR(unit)) && (!SOC_IS_NORTHSTARPLUS(unit))) {
                for (port_idx=0; port_idx < num_test_ports; port_idx++) {
                    rv = bcm_port_enable_set(unit, p->p_port[port_idx], 1);
                    if (BCM_FAILURE(rv)) {
                        test_error(unit, "Unable to enable port %d: %s\n", 
                            p->p_port[port_idx], bcm_errmsg(rv));
                        test_rv = -1;
                        goto done;
                    }
                }
                /* Sleep a bit to let current set of port be handled */
                sal_sleep(3);
            }
        }
#endif /* BCM_ROBO_SUPPORT */

        /* Send packet */
#ifdef COMPILER_HAS_DOUBLE
        time_start = SAL_TIME_DOUBLE();
#else
        time_start = sal_time_usecs();
#endif

        /* Set the transmit length (indicated to tx) and set in hdr */
        BCM_PKT_TX_LEN_SET(p->p_pkt, l);
        BCM_PKT_HDR_TAGGED_LEN_SET(p->p_pkt, l); /* Set length */

        for (tx_pkt_idx = 0; tx_pkt_idx < tx_pkt_cnt; tx_pkt_idx++) {
            rv = bcm_tx(p->p_txrx_unit, p->p_pkt, NULL);
            if (rv < 0) {
                test_error(unit, "Failed to send packet: %s\n", bcm_errmsg(rv));
                test_rv = -1;
                goto done;
            }
        }

        sal_sleep(p->p_time);

#ifdef COMPILER_HAS_DOUBLE
        time_end = SAL_TIME_DOUBLE();
#else
        time_end = sal_time_usecs();
#endif

        /* Unregister the handler, let discard take over */
        rv = rpacket_unregister(p->p_txrx_unit, p);
        if (BCM_FAILURE(rv)) {
            test_error(unit, "Unable to unregister handler, iter %d: %s\n",
                       l, bcm_errmsg(rv));
            test_rv = -1;
            goto done;
        }
        
        /*
         * Stop the packets from flowing.
         */
#if defined(BCM_FIELD_SUPPORT)
        if (soc_feature(unit, soc_feature_field)) {
            if (SOC_IS_ESW(unit)) {
                for (port_idx=0; port_idx < num_test_ports; port_idx++) {
                    rv = bcm_field_entry_remove(unit, p->p_field_entry[port_idx]);
                    RP_CHK(rv, bcm_field_entry_remove);
                }
            } else {
#ifdef BCM_ROBO_SUPPORT
                if (SOC_IS_VULCAN(unit) ||
                    SOC_IS_STARFIGHTER(unit) || SOC_IS_POLAR(unit) || 
                    SOC_IS_NORTHSTAR(unit) || SOC_IS_NORTHSTARPLUS(unit)) {
                    for (port_idx=0; port_idx < num_test_ports; port_idx++) {
                        rv = bcm_field_entry_remove(unit, p->p_field_entry[port_idx]);
                        RP_CHK(rv, bcm_field_entry_remove);
                    }
                }
#endif /* BCM_ROBO_SUPPORT */
            }
        }
#endif
        if (BCM_FAILURE(rv)) {
            test_error(unit, "Unable to remove filter: %s\n",
                       bcm_errmsg(rv));
            test_rv = -1;
            goto done;
        }

#ifdef BCM_ROBO_SUPPORT
        if (SOC_IS_ROBO(unit)) {
            if ((!SOC_IS_VULCAN(unit)) &&
                (!SOC_IS_STARFIGHTER(unit)) && (!SOC_IS_POLAR(unit)) && 
                (!SOC_IS_NORTHSTAR(unit)) && (!SOC_IS_NORTHSTARPLUS(unit))) {
                for (port_idx=0; port_idx < num_test_ports; port_idx++) {
                    rv = bcm_port_enable_set(unit, p->p_port[port_idx], 0);
                    if (BCM_FAILURE(rv)) {
                        test_error(unit, "Unable to disable port %d: %s\n", 
                            p->p_port[port_idx], bcm_errmsg(rv));
                        test_rv = -1;
                        goto done;
                    }
                }
            }
        }
#endif /* BCM_ROBO_SUPPORT */

        if (p->p_received == 0) {
            test_error(unit, "Not receiving packets as expected\n");
            test_rv = -1;
            goto done;
        } else {
#ifdef COMPILER_HAS_DOUBLE
            td = time_end - time_start;
            pps = td ? p->p_received / td : 0;
            bps = td ? ((COMPILER_DOUBLE)p->p_received * (COMPILER_DOUBLE)l
                   * (COMPILER_DOUBLE)1000) / td : 0;
            bps /= ((COMPILER_DOUBLE)(1024 * 1024));
            td_int = (int)td;
            td_frag = (int)(td * 100) % 100,
#else
            td = SAL_USECS_SUB(time_end, time_start);
            pps = (td / 1000) ? (1000 * p->p_received) / (td / 1000) : 0;
            bps = (td / 1024)  ? (p->p_received * l) / (td / 1024) : 0;
            bps = (bps * 1000) / 1024;
            td = td / 10000;
            td_frag = td % 100;
            td_int = td / 100;
#endif

            printk("  %5d  | %8d  | %4d.%02d | %6d  | %3d.%03d\n",
                   l,
                   p->p_received,
                   td_int,
                   td_frag,
                   (int)pps,
                   (int)bps / 1000,
                   (int)bps % 1000);
        }

        /* Sleep a bit to let current set of packets be handled */
        sal_usleep(500000);

    }

done:
    if (p->p_dump_rx) {
#ifdef  BROADCOM_DEBUG
        bcm_rx_show(unit);
#endif  /* BROADCOM_DEBUG */
    }
    return test_rv;
}


STATIC int
rpacket_receiver_deactivate(int unit, p_t *p)
{
    return 0;
}



/*ARGSUSED*/
int
rpacket_done(int unit, void *pa)
/*
 * Function:    rpacket_done
 * Purpose:     Clean up after rpacket test.
 * Parameters:  unit - unit #
 *              pa - cookie (as returned by rpacket_init();
 * Returns:     0 - OK
 *              -1 - failed
 */
{
    p_t         *p = p_control[unit];
    int         rv;
    int         port_idx;
    int         num_test_ports = 1;
#ifdef BCM_ROBO_SUPPORT
    pbmp_t t_pbm;
    int no_que = 0;
    bcm_port_t  port = 0;
#endif /* BCM_ROBO_SUPPORT */
#if defined(BCM_NORTHSTAR_SUPPORT)||defined(BCM_NORTHSTARPLUS_SUPPORT)
    pbmp_t wan_pbmp;
#endif /* BCM_NORTHSTAR_SUPPORT||BCM_NORTHSTARPLUS_SUPPORT */

#ifdef BCM_FE2000_SUPPORT
    if (SOC_IS_SBX_FE(unit)) {
        num_test_ports = 2;
        port_idx = 0;
    }
#endif /* BCM_FE2000_SUPPORT */

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(unit)) {
        if (SOC_IS_VULCAN(unit) ||
            SOC_IS_STARFIGHTER(unit) || SOC_IS_POLAR(unit) || 
            SOC_IS_NORTHSTAR(unit) || SOC_IS_NORTHSTARPLUS(unit)) {
            num_test_ports = 1;
        } else {
            num_test_ports = 2;
        }
    }
#endif /* def BCM_ROBO_SUPPORT */

    if (p == NULL) {
        return 0;
    }

    if (p->p_pkt != NULL) {
        bcm_pkt_free(unit, p->p_pkt);
        p->p_pkt = NULL;
    }

    rv = bcm_rx_unregister(p->p_txrx_unit, rpacket_rx_receive, RP_RECEIVER_PRIO);

    rv = rpacket_receiver_deactivate(p->p_txrx_unit, p);
    if (BCM_FAILURE(rv)) {
        test_error(unit, "Unable to deactivate receiver.\n");
        return -1;
    }

    /* Restore port */
    for (port_idx = 0; port_idx < num_test_ports; port_idx++) {
        rv = bcm_port_info_restore(unit, p->p_port[port_idx], 
                                   &p->p_port_info[port_idx]);
        if (BCM_FAILURE(rv)) {
            test_error(unit, "Unable to restore port %d: %s\n",
                       p->p_port[port_idx], bcm_errmsg(rv));
            return -1;
        }
    }

#ifdef BCM_ROBO_SUPPORT
#if defined(BCM_NORTHSTAR_SUPPORT)||defined(BCM_NORTHSTARPLUS_SUPPORT)
    if (SOC_IS_NORTHSTAR(unit)||SOC_IS_NORTHSTARPLUS(unit)) {
        wan_pbmp = soc_property_get_pbmp(unit, spn_PBMP_WAN_PORT, 0);
    }   
#endif /* BCM_NORTHSTAR_SUPPORT||BCM_NORTHSTARPLUS_SUPPORT */

   /* Restore all other non-testing ports */
   if (SOC_IS_ROBO(unit)) {
        BCM_PBMP_ASSIGN(t_pbm, PBMP_E_ALL(unit));
        for (port_idx=0; port_idx < num_test_ports; port_idx++) {
            BCM_PBMP_PORT_REMOVE(t_pbm, p->p_port[port_idx]);
        }
#if defined(BCM_NORTHSTAR_SUPPORT)||defined(BCM_NORTHSTARPLUS_SUPPORT)
        if (SOC_IS_NORTHSTAR(unit)||SOC_IS_NORTHSTARPLUS(unit)) {
            BCM_PBMP_REMOVE(t_pbm, wan_pbmp);
        }
#endif /* BCM_NORTHSTAR_SUPPORT||BCM_NORTHSTARPLUS_SUPPORT */
    
        PBMP_ITER(t_pbm, port) {
            rv = bcm_port_info_restore(unit, port, &port_info[unit][port]);
            if (BCM_FAILURE(rv)) {
                test_error(unit, "Unable to restore port %d: %s\n", port, bcm_errmsg(rv));
                return -1;
            }
        }
    }
#endif /* BCM_ROBO_SUPPORT */

#ifdef KEYSTONE
   /* Do dma reinit after rpacket test done */
   if (SOC_IS_ROBO(unit)) {
        soc_eth_dma_reinit(unit);
   }
#endif

    /*
     * Try to remove filter, ignore error since we don't know how far
     * we got in initialization.
     */
#if defined(BCM_FIELD_SUPPORT)
    if (soc_feature(unit, soc_feature_field)) {
        int i = 0;
        if (SOC_IS_ESW(unit)) {
            for (i = 0; i < num_test_ports; i++) {
                rv = bcm_field_entry_remove(unit, p->p_field_entry[i]);
                RP_CHK(rv, bcm_field_entry_remove);
                rv = bcm_field_entry_destroy(unit, p->p_field_entry[i]);
                RP_CHK(rv, bcm_field_entry_destroy);
            }
        } else {
#ifdef BCM_ROBO_SUPPORT
            if (SOC_IS_VULCAN(unit) ||
                SOC_IS_STARFIGHTER(unit) || SOC_IS_POLAR(unit) || 
                SOC_IS_NORTHSTAR(unit) || SOC_IS_NORTHSTARPLUS(unit)) {
                for (i = 0; i < num_test_ports; i++) {
                    rv = bcm_field_entry_remove(unit, p->p_field_entry[i]);
                    RP_CHK(rv, bcm_field_entry_remove);
                    rv = bcm_field_entry_destroy(unit, p->p_field_entry[i]);
                    RP_CHK(rv, bcm_field_entry_destroy);
                }
            }
#endif /* BCM_ROBO_SUPPORT */
        }
    }
#endif

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(unit)) {
        if (SOC_IS_VULCAN(unit) || SOC_IS_STARFIGHTER(unit) ||
            SOC_IS_POLAR(unit) || SOC_IS_NORTHSTAR(unit) || 
            SOC_IS_NORTHSTARPLUS(unit)) {
            /* IMP_SW_PROTECT */
            /* Restore IMP port egress rate to previous status */
            BCM_PBMP_CLEAR(t_pbm);
            BCM_PBMP_ASSIGN(t_pbm, PBMP_CMIC(unit));
    
            DRV_RATE_SET
                (unit, t_pbm, no_que, DRV_RATE_CONTROL_DIRECTION_EGRESSS, 
                0, 0, imp_rate_default, imp_burst_size_default);
        }
    }
#endif /* BCM_ROBO_SUPPORT */

#ifdef IPROC_CMICD
    /* Do dma reinit after rpacket test done */
    if (SOC_IS_ROBO(unit)) {
        soc_eth_dma_reinit(unit);
        sal_sleep(3);
    }
#endif

    /*
     * Don't free the p_control entry,
     * keep it around to save argument state
     */
    return 0;
}

/*
 * Function:
 *      rpacket_init
 * Purpose:
 *      Initialize the rpacket test.
 * Parameters:
 *      u - unit #
 *      a - pointer to args
 *      pa - Pointer to cookie
 * Returns:
 *      0 - success, -1 - failed.
 */

int
rpacket_init(int u, args_t *a, void **pa)
{
    p_t                 *p = p_control[u];
    parse_table_t       pt;

#ifdef BCM_ROBO_SUPPORT
    int imp_port;
    pbmp_t t_pbm;
    uint32 limit = 0, burst_size = 0, flags = 0;
    int no_que = 0;
#endif /* BCM_ROBO_SUPPORT */

    if (p == NULL) {
        p = sal_alloc(sizeof(p_t), "rpacket");
        if (p == NULL) {
            test_error(u, "ERROR: cannot allocate memory\n");
            return -1;
        }
        sal_memset(p, 0, sizeof(p_t));
        p_control[u] = p;
    }

    if (!p->p_init) {                   /* Init defaults first time */
        p->p_l_start = 64;
        p->p_l_end = 1522;
        p->p_l_inc = 64;
        p->p_time  = 2;
        p->p_pkts_per_sec = 5000;
        p->p_max_q_len = -1;
        p->p_per_cos = FALSE;
        p->p_burst = 100;
       p->p_intr_cb = TRUE;
       p->p_dump_rx = FALSE;

        /* Init the RX cfg here.  Not much exposed */
       if (SOC_IS_ROBO(u)) {
#ifdef BCM_ROBO_SUPPORT
           p->p_rx_cfg.pkt_size = ROBO_RX_PKT_SIZE_DFLT; 
           p->p_rx_cfg.pkts_per_chain = 1;
#endif /* BCM_ROBO_SUPPORT */
       } else {
           p->p_rx_cfg.pkt_size = 8 * 1024;
#if defined(BCM_ESW_SUPPORT) || defined(BCM_PETRA_SUPPORT)
           p->p_rx_cfg.pkts_per_chain = 4;
#endif /* BCM_ESW_SUPPORT */
#ifdef BCM_FE2000_SUPPORT
           p->p_rx_cfg.pkts_per_chain = 16;
#endif /* BCM_FE2000_SUPPORT */
       }
       p->p_rx_cfg.global_pps = p->p_pkts_per_sec;
       p->p_rx_cfg.max_burst = p->p_burst;
#ifdef BCM_ROBO_SUPPORT
       p->p_rx_cfg.chan_cfg[0].chains = 4;
       p->p_rx_cfg.chan_cfg[0].flags = 0;
       p->p_rx_cfg.chan_cfg[0].cos_bmp = 0xff;
#else
       p->p_rx_cfg.chan_cfg[1].chains = 4;
       p->p_rx_cfg.chan_cfg[1].flags = 0;
       p->p_rx_cfg.chan_cfg[1].cos_bmp = 0xff;
#endif
       /* Not initializing alloc/free functions */
        p->p_init  = TRUE;
    }

    parse_table_init(u, &pt);
    parse_table_add(&pt, "Time", PQ_DFL|PQ_INT, 0, &p->p_time, 0);
    parse_table_add(&pt, "LengthStart", PQ_DFL|PQ_INT, 0, &p->p_l_start, 0);
    parse_table_add(&pt, "LengthEnd", PQ_DFL|PQ_INT, 0, &p->p_l_end, 0);
    parse_table_add(&pt, "LengthInc", PQ_DFL|PQ_INT, 0, &p->p_l_inc, 0);
    parse_table_add(&pt, "FreeBuffer", PQ_DFL|PQ_BOOL, 0, &p->p_free_buffer,
                    0);
    parse_table_add(&pt, "QLen", PQ_DFL|PQ_INT, 0, &p->p_max_q_len, 0);
    parse_table_add(&pt, "PERCos", PQ_DFL|PQ_INT, 0, &p->p_per_cos, 0);
    parse_table_add(&pt, "Rate", PQ_DFL|PQ_INT, 0, &p->p_pkts_per_sec, 0);
    parse_table_add(&pt, "Burst", PQ_DFL|PQ_INT, 0, &p->p_burst, 0);
    parse_table_add(&pt, "PktsPerChain", PQ_DFL|PQ_INT, 0,
                    &p->p_rx_cfg.pkts_per_chain, 0);
#ifdef BCM_ROBO_SUPPORT
    parse_table_add(&pt, "Chains", PQ_DFL|PQ_INT, 0,
                   &p->p_rx_cfg.chan_cfg[0].chains, 0);
#else
    parse_table_add(&pt, "Chains", PQ_DFL|PQ_INT, 0,
                    &p->p_rx_cfg.chan_cfg[1].chains, 0);
#endif
    parse_table_add(&pt, "useINTR", PQ_DFL|PQ_BOOL, 0, &p->p_intr_cb, 0);
    parse_table_add(&pt, "DumpRX", PQ_DFL|PQ_BOOL, 0, &p->p_dump_rx, 0);

    if (parse_arg_eq(a, &pt) < 0 || ARG_CNT(a) != 0) {
        test_error(u, "%s: Invalid option: %s\n",
                   ARG_CMD(a), ARG_CUR(a) ? ARG_CUR(a) : "*");
        parse_arg_eq_done(&pt);
        return -1;
    }
    parse_arg_eq_done(&pt);

    if (p->p_time < 1) {
        test_error(u, "%s: Invalid duration: %d (must be 1 <= time)\n",
                   ARG_CMD(a), p->p_time);
        return -1;
    }

    if (p->p_rx_cfg.pkts_per_chain > RP_MAX_PPC) {
        soc_cm_print("Too many pkts/chain (%d).  Setting to max (%d)\n",
                     p->p_rx_cfg.pkts_per_chain, RP_MAX_PPC);
        p->p_rx_cfg.pkts_per_chain = RP_MAX_PPC;
    }

    if (rpacket_setup(u, p) < 0) {
        (void)rpacket_done(u, p);
        return -1;
    }

    *pa = (void *)p;

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(u)) {
        /* IMP_SW_PROTECT */
        imp_port = CMIC_PORT(u);
        BCM_PBMP_CLEAR(t_pbm);
        BCM_PBMP_ASSIGN(t_pbm, PBMP_CMIC(u));
        if (SOC_IS_VULCAN(u) || SOC_IS_STARFIGHTER(u) ||
            SOC_IS_POLAR(u) || SOC_IS_NORTHSTAR(u) ||
            SOC_IS_NORTHSTARPLUS(u)) {
            /* Record IMP port previous egress rate : restore back while testing done */
            if (DRV_RATE_GET
                (u, imp_port, no_que, DRV_RATE_CONTROL_DIRECTION_EGRESSS, 
                &flags, NULL, &limit, &burst_size) < 0) {
                return -1;
            }
    
            imp_rate_default = limit;
            imp_burst_size_default = burst_size;
    
            SOC_IF_ERROR_RETURN(DRV_RATE_SET
                (u, t_pbm, no_que, DRV_RATE_CONTROL_DIRECTION_EGRESSS, 
                0, 0, imp_rate_pps, IMP_BURST_SIZE));

        }
    }
#endif /* BCM_ROBO_SUPPORT */

    return 0;
}

#endif /* BCM_FIELD_SUPPORT || BCM_FE2000_SUPPORT || 
          BCM_ROBO_SUPPORT || BCM_PETRA_SUPPORT */
