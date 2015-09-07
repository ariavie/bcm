/*
 * $Id: tpacket.c,v 1.44 Broadcom SDK $
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
 * Packet tests that use the BCM api.
 */

#include <sal/types.h>
#include <sal/core/time.h>
#include <sal/core/stats.h>
#include <sal/appl/sal.h>
#include <shared/bsl.h>
#include <soc/util.h>
#include <shared/bsl.h>
#include <appl/diag/system.h>
#include <appl/diag/parse.h>

#include <bcm/error.h>
#include <bcm/l2.h>
#include <bcm/mcast.h>
#include <bcm/pkt.h>
#include <bcm/tx.h>
#include <bcm/rx.h>


#if (defined(INCLUDE_KNET) && defined(LINUX) && (!defined(__KERNEL__)))

#include <bcm/knet.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/mman.h>
    
#include <sched.h>
#include <sal/core/thread.h>
#endif

#include "testlist.h"


/*
#define SAND_BYTE_SWAP(x) ((((x) << 24)) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) >> 24)))
*/
#ifdef BCM_ARAD_SUPPORT
char tr40_test_usage[] = 
"TR40 (send packets) Usage:\n"
" \n"
#ifdef COMPILER_STRING_CONST_LIMIT
    "\nFull documentation cannot be displayed with -pendantic compiler\n";
#else
  "Tagged=<true/false>             -  Specifies Tagged flag (true) or not tagged (false) in tx descriptor\n"
  "VlanId=<value>                  -  Specifies Vlad Id \n"
  "TpId=<value>                    -  Specifies Type ID. Default: '0x8100'\n"
  "LengthStart=<value>             -  Specifies Length Start value (first tx packet length in a chain)\n"
  "LengthEnd=<value>               -  Specifies length End value (max tx packet length in a current chain) \n"
  "LengthInc=<value>               -  Specifies length increment value in n-packet in a current chain, current (n) packet lenght = length start + n * lenght increment \n"
  "PktsPerChain=<value>            -  Specifies number of packet will be sent in current chain starts from LengthStart, packet length is incrementing by LengthInc till LengthEnd  \n"
  "Chains=<value>                  -  Specifies number of chains will be sent (each chain includes PktsPerChain packets \n"
  "Array=<true/false>              -  Specifies if chain (>1 packets) will be sent by array (bcm_tx_array) or by list (bcm_tx_list)\n"
  "Operations<1<value<1000>        -  Specifies loop number to run of chains\n"
  "HIGig=<true/false>              -  Specifies Higig flag (true) or not mark (false) in tx descriptor\n"
  "PURge=<true/false>              -  Specifies Purge flag (true) or not mark (false) in tx descriptor\n"
  "CHNUMber=<value>                -  Specifies descriptor channel number (from 0 to 255), 0 by default\n"
  "MacDest=<xx:xx:xx:xx:xx:xx>     -  Specifies Destination Mac address\n"
  "MacSrc=<xx:xx:xx:xx:xx:xx>      -  Specifies Source Mac address\n"
  "Pattern=<0xvalue>               -  Specifies a pattern to use in packet payload(usefile=false)\n"
  "PatternInc=<value>              -  Specifies a packet increment pattern value (pattern + patterninc) uses in packet payload (usefile=false)\n"
  "PatternRandom=<true/false>      -  Creates random parrern in packet payload (usefile=false)\n"
  "Filename=<name string>          -  Specifies file name when file is in use (usefile=true)\n"
  "FileLen=<value>                 -  Specifies file length when file is in use (usefile=true)\n"
  "UseFile=<true/false>            -  Marks if file is used for packet send(true) or via build-in procedure \n"
  "\n"
;
#endif /*COMPILER_STRING_CONST_LIMIT*/
#endif

/*
#define SAND_BYTE_SWAP(x) ((((x) << 24)) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) >> 24)))
*/
#ifdef BCM_ARAD_SUPPORT
#define XD_FILE(xd)     ((xd)->xd_file != NULL && (xd)->xd_file[0] != 0)
#endif

#ifdef BCM_ARAD_SUPPORT
STATIC int8 dpp_tx_load_packet_tr(int unit, char *fname, int length, uint8 *packet_in);
#endif

typedef struct p_s {
    int     p_init;         /* TRUE --> initialized */
    int     p_opt;          /* Destination option */
#   define  P_O_UC_HIT  0   /* Unicast in ARL */
#   define  P_O_UC_MISS 1   /* Unicast not in ARL */
#   define  P_O_MC_HIT  2   /* L2 multicast in MARL/L2 */
#   define  P_O_MC_MISS 3   /* L2 multicast not in MARL/L2 */
#   define  P_O_BC      4   /* L2 Broadcast */
#   define  P_O_PORT    5   /* Send to specific ports */
#   define  P_O_DPP_PORT    6   /* Send to specific dpp */

    int     p_fp;           /* TRUE --> Use fastpath (new only) */
    int     p_tag;          /* TRUE --> tag packet */
    int     p_vid;          /* VLAN id if tagged */
    int     p_tpid;
    sal_mac_addr_t p_dst;       /* Dest mac address */
    sal_mac_addr_t p_src;       /* Source mac address */
    int     p_array;        /* Array? Bool; false -> list */
    int     ppc;            /* # packets per chain */
    int     chains;     /* # chains */
    int     p_cnt_start;        /* # started */
    volatile int chains_done;       /* # complete */
    int     p_cnt_accum;        /* Accumulated q length all samples */
    pbmp_t  p_pbm;          /* Ports to xmit on */
    pbmp_t  p_upbm;         /* Untabg ports (only if fast) */
    int     len_start;      /* Length start */
    int     len_end;        /* Length end */
    int     p_l_inc;        /* Length increment */
    int     p_ops;          /* # outstanding operations */
    sal_sem_t   p_sem_active;       /* Synchronize # active */
    sal_sem_t   p_sem_done;     /* Completion wait */
    int     p_ops_total;        /* Accumulated operations outstanding */
    enet_hdr_t  *p_tx;          /* Tx - Packet buffer */
    bcm_pkt_t   *p_pkt;                 /* The packet being tx'd */
    bcm_pkt_t   **p_pkt_array;          /* Array of pkt pointers */
#ifdef BCM_ARAD_SUPPORT
    char test_usage[20];
    uint32 xd_pat;       
    uint32 xd_pat_inc;   
    uint32 xd_pat_random;
    char   *xd_file;               /* File name of packet data */
    uint32 xd_filelen;             /* File length to get*/
    int    xd_usefile;             /* Flag indicates that just a file is in use for test*/
    uint32 purge_flag;
    uint32 higig_flag;
    int ch_number;
#endif
#if (defined(INCLUDE_KNET) && defined(LINUX) && (!defined(__KERNEL__)))
    int   p_use_socket;
    int   p_sock;
    int   p_netid;
    int   p_filterid;
#endif
    int   p_sync;
} p_t;

static  sal_mac_addr_t  tp_mac_uc = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
static  sal_mac_addr_t  tp_mac_mc = {0x01, 0x11, 0x22, 0x33, 0x44, 0x44};
static  sal_mac_addr_t  tp_mac_bc = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static  sal_mac_addr_t  tp_mac_dpp_src = {0x00, 0x00, 0x00, 0x00, 0x00, 0xe2};
static  sal_mac_addr_t  tp_mac_dpp_dst = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

#ifdef BCM_ARAD_SUPPORT
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
                          0x7b,0x7c,0x7d,0x7e,   0x7f,0x80,0x81,0x82  };
#endif /* BCM_ARAD_SUPPORT */

static p_t  *p_control[SOC_MAX_NUM_DEVICES];

#define PACKET_MAX_BUFFER   (16 * 1024) /* 16K max packet */
extern int bcm_common_tx(int unit, bcm_pkt_t *pkt, void *cookie);

#if (defined(INCLUDE_KNET) && defined(LINUX) && (!defined(__KERNEL__)))

STATIC int 
tx_bind_device(int sock, char* dev)
{
    int rv;
    struct sockaddr_ll addr;    
    struct ifreq req;

    
    sal_memset(&req, 0, sizeof(req));
    sal_strncpy(req.ifr_name, dev, sizeof(req.ifr_name));
    if ((rv = ioctl(sock, SIOCGIFINDEX, &req)) < 0) {
        cli_out("ioctl error %d\n", errno);
        return rv;
    }  

    if ((rv = setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, 
                        (void *)&req, sizeof(req))) < 0) {
        cli_out("bind_device setsockopt error %d\n", errno); 
        return rv;
    }

    sal_memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(0x8100);
    addr.sll_ifindex = req.ifr_ifindex;
    addr.sll_hatype = 0;
    addr.sll_pkttype = 0;
    addr.sll_halen = 0;
    if ((rv = bind(sock, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
        cli_out("bind error %d\n", errno);
        return rv;
    }
    return 0;
}

STATIC int
tx_set_promisc_up(int sock,  char* dev)
{
    struct ifreq req;       
    sal_strncpy(req.ifr_name, dev, sizeof(req.ifr_name));
    
    if (ioctl(sock, SIOCGIFFLAGS, &req)==-1) {
        cli_out("ioctl error");
        return -1;
    }
    req.ifr_flags |= (IFF_PROMISC | IFF_UP | IFF_RUNNING);
    if (ioctl(sock, SIOCSIFFLAGS, &req)==-1) {
        cli_out("ioctl error");
        return -1;
    }
    return 0;
}

STATIC int
tx_knetif_setup(int unit, p_t *p) 
{  
    int rv;
    
    bcm_knet_netif_t netif;
    bcm_knet_filter_t filter;
    
    bcm_knet_netif_t_init(&netif);
    netif.type = BCM_KNET_NETIF_T_TX_CPU_INGRESS;
    sal_memcpy(netif.mac_addr, tp_mac_uc, 6);
    
    p->p_netid = 0;
    rv = bcm_knet_netif_create(unit, &netif);
    if (BCM_SUCCESS(rv)) {
        p->p_netid = netif.id;
    } else {
        cli_out("bcm_knet_netif_create failed: %d\n", rv);
        return rv;
    }
    
    p->p_filterid = 0;
    bcm_knet_filter_t_init(&filter);
    filter.type = BCM_KNET_FILTER_T_RX_PKT;
    if (p->p_use_socket) {
        filter.dest_type = BCM_KNET_DEST_T_NETIF;
        filter.dest_id = p->p_netid; 
    } else {
        filter.dest_type = BCM_KNET_DEST_T_BCM_RX_API;
    }
    rv = bcm_knet_filter_create(unit, &filter);
    if (BCM_SUCCESS(rv)) {
        p->p_filterid = filter.id;
    } else {
        cli_out("bcm_knet_filter_create:%d\n", rv);
        return rv;
    }
    
    p->p_sock = socket(AF_PACKET, SOCK_RAW, htons(0x8100));

    tx_set_promisc_up(p->p_sock, netif.name);
   
    tx_bind_device(p->p_sock, netif.name);

    return rv;
}


STATIC void
tx_knetif_clean(int unit, p_t *p) 
{   
    
    if (p->p_sock > 0) {
        struct ifreq req;
        bcm_knet_netif_t netif;
        bcm_knet_netif_get(unit, p->p_netid, &netif);
       
        sal_strncpy(req.ifr_name, netif.name, sizeof(req.ifr_name));
        
        if (ioctl(p->p_sock, SIOCGIFFLAGS, &req)==-1) {
            cli_out("ioctl SIOCGIFFLAGS error");
        }
        req.ifr_flags &= (~(IFF_UP | IFF_RUNNING));
        if (ioctl(p->p_sock, SIOCSIFFLAGS, &req)==-1) {
            cli_out("ioctl SIOCSIFFLAGS error");
        }
        close(p->p_sock);
        p->p_sock = 0;
    }
    if (p->p_filterid > 0) {
        bcm_knet_filter_destroy(unit, p->p_filterid);
        p->p_filterid = 0;
    }
    
    if (p->p_netid > 0) {
        bcm_knet_netif_destroy(unit, p->p_netid);
        p->p_netid = 0;
    }
}

#endif




STATIC void
xmit_chain_done(int unit, bcm_pkt_t *pkt, void *cookie)
{
    p_t     *p = (p_t *)cookie;

    sal_sem_give(p->p_sem_active);  /* Allow next */
    if (++p->chains_done == p->chains) {
        sal_sem_give(p->p_sem_done);
    }
}

/* Note:  i != 0, at least not for linked lists */
#define PKT_PTR_NEXT(p, i, cur) (((p)->ppc > 1) ? \
    ((p)->p_array ? (p)->p_pkt_array[i] : (cur)->next) : NULL)

#define FOREACH_P_PKT(p, i, pkt) \
    for ((i) = 0, (pkt) = (p)->p_pkt; (i) < (p)->ppc; \
        (i)++, (pkt) = PKT_PTR_NEXT(p, i, pkt))

STATIC int
tpacket_setup(int unit, p_t *p)
{
    enet_hdr_t  *enet_hdr;
    uint8   *fill_addr;
    bcm_pkt_t   *pkt;
    int i;
    

    p->p_sem_active = sal_sem_create("tpacket_ops", 
                                     sal_sem_COUNTING,
                                     p->p_ops);
    p->p_sem_done   = sal_sem_create("tpacket_done", 
                                      sal_sem_BINARY, 
                                      0);

    if ((p->p_sem_active == NULL)) {
        test_error(unit, "Failed to allocate p_sem_active Tx Buffer\n");
        return(-1);
    }
    if ( (p->p_sem_done == NULL)) {
        test_error(unit, "Failed to allocate p_sem_done Tx Buffer\n");
        return(-1);
    }

    if (p->ppc > 1) { /* Multiple packets per chain */
        if (p->p_array) {
             bcm_pkt_blk_alloc(unit, p->ppc,
                               PACKET_MAX_BUFFER, BCM_TX_CRC_REGEN, 
                               &(p->p_pkt_array));
            if (!p->p_pkt_array) {
                test_error(unit, "Failed to allocate pkt array\n");
                return -1;
            }
            p->p_pkt = p->p_pkt_array[0];
        } else { /* Linked list */
            for (i = 0; i < p->ppc; i++) {
                bcm_pkt_alloc(unit, PACKET_MAX_BUFFER,
                              BCM_TX_CRC_REGEN, &pkt);
                if (!pkt) {
                    test_error(unit, "Failed to allocate pkt list\n");
                    return -1;
                }
                pkt->next = p->p_pkt;
                p->p_pkt = pkt;
            }
        }
    } else {
        bcm_pkt_alloc(unit, PACKET_MAX_BUFFER, BCM_TX_CRC_REGEN, &(p->p_pkt));
        if (!p->p_pkt) {
            test_error(unit, "Failed to allocate Tx Buffer\n");
            return(-1);
        }
        if (!p->p_sync) {
            p->p_pkt->call_back = xmit_chain_done;
        }
    }

    p->p_tx = (enet_hdr_t *)p->p_pkt->pkt_data[0].data;

    FOREACH_P_PKT(p, i, pkt) {
#ifdef BCM_ARAD_SUPPORT            
        if (SOC_IS_ARAD(unit)) {
            if (p->higig_flag > 0) {
            pkt->flags |= BCM_PKT_F_HGHDR;
            }
            if (p->purge_flag > 0) {
                pkt->flags |= BCM_TX_PURGE;
            }
            pkt->_dpp_hdr[0] = ((uint32)p->ch_number) /* << 24 */;
            pkt->_dpp_hdr[1] = 0;
            pkt->_dpp_hdr[2] = 0;
            pkt->_dpp_hdr[3] = 0; /* ((uint32)p->ch_number) ; */ /* ((uint32)p->ch_number) << 24; */
        }
#endif                  
        if (p->p_fp) {
            pkt->flags |= BCM_TX_FAST_PATH;
        } else if (SOC_IS_XGS3_SWITCH(unit)) {
            pkt->flags |= BCM_TX_ETHER;
        }

        enet_hdr = (enet_hdr_t *)BCM_PKT_DMAC(pkt);

        /* Fill in buffer according to request */
        ENET_SET_MACADDR(enet_hdr->en_dhost, p->p_dst);
        if (SOC_IS_ARAD(unit)) {
            ENET_SET_MACADDR(enet_hdr->en_shost, p->p_src);
        }
#ifdef BCM_ARAD_SUPPORT
        if ( !((XD_FILE(p) && (p->xd_usefile == TRUE))))  
#endif
        { 
            if (p->p_tag) {
                if (SOC_IS_ARAD(unit)) {
                    enet_hdr->en_tag_tpid = soc_htons(p->p_tpid); 
                } else {
                    enet_hdr->en_tag_tpid = soc_htons(ENET_DEFAULT_TPID);
                }
                enet_hdr->en_tag_ctrl = soc_htons(VLAN_CTRL(0, 0, p->p_vid));
            }
        }
        fill_addr = (uint8 *)enet_hdr + 2*sizeof(bcm_mac_t) + sizeof(uint32);
#ifdef BCM_ARAD_SUPPORT
        if (SOC_IS_ARAD(unit)) {

                    /* Store pattern */
            if (p->xd_pat_random) {
                packet_random_store(fill_addr, p->len_end -
                           (fill_addr - (uint8 *)enet_hdr));
            } else if (XD_FILE(p) && (p->xd_usefile == TRUE)) {
                
                fill_addr = (uint8 *)enet_hdr;
                if (dpp_tx_load_packet_tr(unit,p->xd_file, p->xd_filelen, fill_addr) != 0) {
                        LOG_ERROR(BSL_LS_APPL_TESTS,
                                  (BSL_META_U(unit,
                                              "Unable to load packet from file %s\n"),
                                   p->xd_file));
                        return CMD_FAIL;
                    }
            } else {
            sal_memcpy( fill_addr, mypacket, p->len_end -
                       (fill_addr - (uint8 *)enet_hdr));
            }
        } else 
#endif /* BCM_ARAD_SUPPORT */
        {
            sal_memset(fill_addr, 0xff, p->len_end -
                       (fill_addr - (uint8 *)enet_hdr));
        }
    /*  
        if (SOC_IS_ARAD(unit)) {
            unsigned int i=0;   
            uint32 *ppacket32;
            ppacket32 = (uint32 *)enet_hdr;
            for (i=0;i<p->len_end/4;i++)
            {
                ppacket32[i] = SAND_BYTE_SWAP(ppacket32[i]);
            }
        } */ 

    }

    return(0);
}


/*
 * Function:    tpacket_test
 * Purpose: Test basic PCI stuff on StrataSwitch.
 * Parameters:  unit - unit #.
 *      a - pointer to arguments.
 *      pa - ignored cookie.
 * Returns: 0
 */

int
tpacket_tx_test(int unit, args_t *a, void *pa)
{
    p_t     *p = (p_t *)pa;
    int     i, len, rv = BCM_E_NONE;
    sal_usecs_t  time_start, time_end;
    enet_hdr_t  *enet_hdr;
    bcm_pkt_t   *pkt;
    uint32 elapsed_us;
    uint32 bps; /* bytes/sec */
    int ql;
    uint32 pps;  /* Packets per second */
    uint32 tot_pkts;
    sal_cpu_stats_t   cpu_start;
    sal_cpu_stats_t   cpu_end;
    int idle, user, kernel;
    int idle_diff, user_diff, kernel_diff, total_diff;

    COMPILER_REFERENCE(a);

    tot_pkts = p->chains * p->ppc;


    cli_out("\n"
            "  Packet | Time   |     Rate       | Average  CPU %%            \n"
            "   Size  | (Sec)  |  p/s  |  MB/s  |  Queue | Idle /user /kern  \n"
            " --------+--------+-------+--------+--------+-------------------\n");

    for (len = p->len_start; len <= p->len_end; len += p->p_l_inc) {
        FOREACH_P_PKT(p, i, pkt) {
            pkt->pkt_data[0].len = len;
            enet_hdr = (enet_hdr_t *)BCM_PKT_DMAC(pkt);
#ifdef BCM_ARAD_SUPPORT
            if (!((XD_FILE(p) && (p->xd_usefile == TRUE))))  
#endif
            {
                if (p->p_tag) {
                    enet_hdr->en_tag_len = soc_htons(len);
                } else {
                    enet_hdr->en_untagged_len = soc_htons(len);
                }
            }
        }
        p->p_cnt_accum = 0;
        p->chains_done  = 0;

        /* *** Start Timer *** */

        time_start = sal_time_usecs();
        sal_cpu_stats_get(&cpu_start);
        /* *** GO *** */
    
#if (defined(INCLUDE_KNET) && defined(LINUX) && (!defined(__KERNEL__)))  
        if (p->p_use_socket) {
            tot_pkts = 0;
            for (i = 0; i < 100000; i++) {
                if (p->p_sync) {
                    rv = send(p->p_sock, p->p_pkt->_pkt_data.data, len, 0);
                } else {
                    rv = send(p->p_sock, p->p_pkt->_pkt_data.data, len, MSG_DONTWAIT);
                    if (rv <= 0) { 
                        i--;
                        sal_usleep(1000);
                    } 
                }
                if (rv > 0) {
                    tot_pkts++;
                }
            }
            
        } else {
#endif
    
            for (i = 0; i < p->chains; i++) {        
                if (!((p->ppc == 1) && p->p_sync)) {
                    if (sal_sem_take(p->p_sem_active, 10 * 1000000) < 0) {
                        sal_sem_give(p->p_sem_active);
                        test_error(unit, "Transmission appears stalled, semaphore release\n");
                        return(-1);
                    }
                }
                p->p_cnt_accum += i - p->chains_done;
                    switch(p->p_opt) {
                    case P_O_PORT:
                        if (p->ppc == 1) { /* Packets per chain */
                            rv = bcm_tx(unit, p->p_pkt, p);
                        } else if (p->p_array) {
                            rv = bcm_tx_array(unit, p->p_pkt_array, p->ppc,
                                      xmit_chain_done, p);
                        } else { /* Linked list, multiple packets per chain */
                            rv = bcm_tx_list(unit, p->p_pkt, xmit_chain_done, p);
                        }
                        if (rv < 0) {
                            test_error(unit, "packet send failed: %s\n",
                                   bcm_errmsg(rv));
                        }
                        break;
                    case P_O_DPP_PORT:
                        cli_out("send dpp packet start\n"); 
                        rv = bcm_tx(unit, p->p_pkt, p);
                        if (rv < 0) {
                            test_error(unit, "packet send failed: %s\n", bcm_errmsg(rv));
                        }
                        break;
                    default:
                        /* This is already caught elsewhere */
                        test_error(unit, "Only PortBitMap option supported "
                                   "for bcm_tx\n");
                        break;
                    }
                    if (rv < 0) {
                        /* If packet send failed, wait till the previous 
                         * packet transmit is done, before exiting the test.
                         */
                        if (sal_sem_take(p->p_sem_done, 10 * 1000000) < 0) {
                            sal_sem_give(p->p_sem_done);
                            return(-1);
                        }
                        return(-1);
                    }
            }
            if (!((p->ppc == 1) && p->p_sync)) {
                if (0 > sal_sem_take(p->p_sem_done, 10 * 1000000)) {
                    test_error(unit, "Timeout waiting for completion\n");
                    return(-1);
                }
            }
#if (defined(INCLUDE_KNET) && defined(LINUX) && (!defined(__KERNEL__)))
       }
#endif
        sal_cpu_stats_get(&cpu_end);
        time_end = sal_time_usecs();
        
        COMPILER_64_SUB_64(cpu_end.total, cpu_start.total);
        total_diff = (int)(u64_L(cpu_end.total));
        COMPILER_64_SUB_64(cpu_end.idle, cpu_start.idle);
        idle_diff = (int)(u64_L(cpu_end.idle));
        COMPILER_64_SUB_64(cpu_end.user, cpu_start.user);
        user_diff = (int)(u64_L(cpu_end.user));
        COMPILER_64_SUB_64(cpu_end.kernel, cpu_start.kernel);
        kernel_diff = (int)(u64_L(cpu_end.kernel));              

        if (total_diff != 0) {
            idle = idle_diff * 10000UL / total_diff;               
            user = user_diff * 10000UL / total_diff;                
            kernel = kernel_diff * 10000UL / total_diff;
        } else {
            idle = 0;               
            user = 0;                
            kernel = 0;
        }
        
        if (time_end == time_start) {
            cli_out("   %5d  | ****** | ***** | ******  | ***\n", len);
        } else {
            elapsed_us = SAL_USECS_SUB(time_end, time_start);
            /* Round off error limitting */
            if (tot_pkts > 2000) {
                pps = tot_pkts *1000 / (elapsed_us / 1000);
            } else {
                pps = tot_pkts * 1000000 / elapsed_us;
            }
    
            bps = pps * len;
            ql = (p->p_cnt_accum * 1000) / p->chains;
    
            cli_out("  %5d  | %2d.%03d | %5d | %2d.%03d | %2d.%03d | %2u.%02u/%2u.%02u/%2u.%02u \n",
                    len,
                    elapsed_us / 1000000, (elapsed_us % 1000000) / 1000,
                    pps,
                    bps / 1000000,(bps % 1000000) / 1000,
                    ql / 1000, ql % 1000, 
                    idle / 100, idle % 100,
                    user / 100, user % 100,
                    kernel / 100, kernel % 100);
    
            /* Sleep a bit to let current set of packets be handled */
            sal_usleep(500000);
        }
    }
    return(0);
}

/*ARGSUSED*/
int
tpacket_tx_done(int unit, void *pa)
/*
 * Function:    pci_test_done
 * Purpose: Restore all values to CMIC from soc structure.
 * Parameters:  u - unit #
 *      pa - cookie (Ignored)
 * Returns: 0 - OK
 *      -1 - failed
 */
{
    p_t     *p = p_control[unit];
#if defined (BCM_ESW_SUPPORT) || defined (BCM_ARAD_SUPPORT)
    int     rv;
#endif /* BCM_ESW_SUPPORT */
    bcm_pkt_t   *pkt;
    bcm_pkt_t   *next_pkt;

    if (p == NULL) {
        return 0;
    }

#if defined (BCM_ESW_SUPPORT) || defined (BCM_ARAD_SUPPORT)
    if (SOC_IS_ARAD (unit) ) {
        if (0 > (rv = soc_dma_abort_channel_total(unit,0))) { 
            cli_out("Warning: soc_dma_abort(%d) failed: %s\n",
                    unit, soc_errmsg(rv));
        } 
    } else {
#if !defined(INCLUDE_KNET) 
        if (0 > (rv = soc_dma_abort(unit))) { 
            cli_out("Warning: soc_dma_abort(%d) failed: %s\n",
                    unit, soc_errmsg(rv));
        }
#endif
    }
#endif /* BCM_ESW_SUPPORT */

    if (p->chains > 1) {
        if (p->p_array) {
            bcm_pkt_blk_free(unit, p->p_pkt_array, p->ppc);
        } else {
            pkt = p->p_pkt;
            while (pkt) {
                next_pkt = pkt->next;
                bcm_pkt_free(unit, pkt);
                pkt = next_pkt;
            }
        }
    } else {
        if (p->p_pkt != NULL) {
            bcm_pkt_free(unit, p->p_pkt);
        }
    }

    p->p_pkt_array = NULL;
    p->p_pkt = NULL;
    p->p_tx = NULL;

    if (p->p_sem_active != NULL) {
        sal_sem_destroy(p->p_sem_active);
        p->p_sem_active = NULL;
    }
    if (p->p_sem_done != NULL) {
        sal_sem_destroy(p->p_sem_done);
        p->p_sem_done = NULL;
    }

    /*
     * Don't free the p_control entry,
     * keep it around to save argument state
     */
#if (defined(INCLUDE_KNET) && defined(LINUX) && (!defined(__KERNEL__)))
    tx_knetif_clean(unit, p);
#endif
    return 0;
}

int
tpacket_tx_init(int unit, args_t *a, void **pa)
/*
 * Function:    packet_test_init
 * Purpose: Save all the current PCI Config registers to write
 *      on completion.
 * Parameters:  u - unit #
 *      a - pointer to args
 *      pa - Pointer to cookie
 * Returns: 0 - success, -1 - failed.
 */
{
#ifdef BCM_ARAD_SUPPORT
    char *xfile;
#endif  
    static char     *opt_list[] = {
    "UCHit", "UCMiss", "MCHit", "MCMiss", "Bcast", "PortBitMap", "DPPconfig"
    };
    p_t         *p;
    parse_table_t   pt;
    bcm_mcast_addr_t    mc;
    bcm_l2_addr_t   l2;
    int         rv;

    p = p_control[unit];
    if (p == NULL) {
    p = sal_alloc(sizeof(p_t), "tpacket");
    if (p == NULL) {
        test_error(unit, "ERROR: cannot allocate memory\n");
        return -1;
    }
    sal_memset(p, 0, sizeof(p_t));
    p_control[unit] = p;
    }

    if (!p->p_init) {           /* Init defaults first time */
    p->len_start = 64;
    p->len_end = 64;
    p->p_l_inc = 64;
    p->p_init  = TRUE;
    p->chains  = 1000;
    p->ppc     = 1;
    if (SOC_IS_ARAD(unit)) {
        p->p_vid   = 0;
    } else {
        p->p_vid   = 1;
    }
#ifdef BCM_ARAD_SUPPORT
    p->xd_pat          = 0x12345678;
    p->xd_pat_inc      = 1;
    p->xd_pat_random   = 0;
    p->purge_flag = FALSE;
    p->higig_flag = TRUE;
    p->ch_number=0;
    p->test_usage[0] = ' ';
    p->test_usage[1] = '\0';
    p->p_array = 0;
#else
    p->p_array = 1;
#endif
    p->p_ops   = 1;
    p->p_tag   = 1;
    p->p_opt   = P_O_PORT;
    p->p_tpid = ENET_DEFAULT_TPID;

    SOC_PBMP_CLEAR(p->p_pbm);
    SOC_PBMP_CLEAR(p->p_upbm);
    SOC_PBMP_OR(p->p_pbm, PBMP_PORT_ALL(unit));
    
    p->p_sync = 1;
    }

    parse_table_init(unit, &pt);
#ifdef BCM_ARAD_SUPPORT	
    if (SOC_IS_ARAD(unit)) {
    parse_table_add(&pt, "?", PQ_DFL|PQ_STRING, 0, &p->test_usage, 0);
    }
else
#endif
{
    parse_table_add(&pt, "FastPath", PQ_DFL|PQ_BOOL, 0, &p->p_fp, 0);
    parse_table_add(&pt, "DestMac", PQ_DFL|PQ_MULTI, 0, &p->p_opt, opt_list);
    parse_table_add(&pt, "PortBitMap", PQ_DFL|PQ_PBMP, 0, &p->p_pbm, 0);
    parse_table_add(&pt, "UntagPortBitMap", PQ_DFL|PQ_PBMP, 0, &p->p_upbm, 0);
}
    parse_table_add(&pt, "Tagged", PQ_DFL|PQ_BOOL, 0, &p->p_tag, 0);
    parse_table_add(&pt, "VlanId", PQ_DFL|PQ_INT, 0, &p->p_vid, 0);
    parse_table_add(&pt, "TpId", PQ_DFL|PQ_INT, 0, &p->p_tpid, 0);
    parse_table_add(&pt, "LengthStart", PQ_DFL|PQ_INT, 0, &p->len_start, 0);
    parse_table_add(&pt, "LengthEnd", PQ_DFL|PQ_INT, 0, &p->len_end, 0);
    parse_table_add(&pt, "LengthInc", PQ_DFL|PQ_INT, 0, &p->p_l_inc, 0);
    parse_table_add(&pt, "PortBitMap", PQ_DFL|PQ_PBMP, 0, &p->p_pbm, 0);
    parse_table_add(&pt, "UntagPortBitMap", PQ_DFL|PQ_PBMP, 0, &p->p_upbm, 0);
    parse_table_add(&pt, "PktsPerChain", PQ_DFL|PQ_INT, 0, &p->ppc, 0);
    parse_table_add(&pt, "Chains", PQ_DFL|PQ_INT, 0, &p->chains, 0);
    parse_table_add(&pt, "Array", PQ_DFL|PQ_BOOL, 0, &p->p_array, 0);
    parse_table_add(&pt, "Operations", PQ_DFL|PQ_INT, 0, &p->p_ops, 0);
#ifdef BCM_ARAD_SUPPORT
    parse_table_add(&pt, "HIGig",        PQ_DFL|PQ_BOOL ,0, &p->higig_flag,           0);
    parse_table_add(&pt, "PURge",        PQ_DFL|PQ_BOOL ,0, &p->purge_flag,           0);
    parse_table_add(&pt, "CHNUMber",    PQ_DFL|PQ_INT ,0, &p->ch_number,           0);
    parse_table_add(&pt, "MacDest",     PQ_DFL|PQ_MAC,  0,&p->p_dst,    NULL);
    parse_table_add(&pt, "MacSrc",     PQ_DFL|PQ_MAC,  0,&p->p_src,    NULL);
    parse_table_add(&pt, "Pattern",     PQ_DFL|PQ_HEX,  0, &p->xd_pat,            NULL);
    parse_table_add(&pt, "PatternInc",  PQ_DFL|PQ_INT,  0, &p->xd_pat_inc,    NULL);
    parse_table_add(&pt, "PatternRandom", PQ_DFL|PQ_BOOL, 0, &p->xd_pat_random,    NULL);
    parse_table_add(&pt, "Filename",    PQ_DFL|PQ_STRING,0, &p->xd_file,           NULL);
    parse_table_add(&pt, "FileLen",    PQ_DFL|PQ_INT ,0, &p->xd_filelen,           NULL);
    parse_table_add(&pt, "UseFile",    PQ_DFL|PQ_BOOL ,0, &p->xd_usefile,           NULL);
#endif
#if (defined(INCLUDE_KNET) && defined(LINUX) && (!defined(__KERNEL__)))
    parse_table_add(&pt, "Socket",        PQ_DFL|PQ_BOOL ,0, &p->p_use_socket,           0);
#endif
    parse_table_add(&pt, "Sync",        PQ_DFL|PQ_BOOL ,0, &p->p_sync,         0);
    if (parse_arg_eq(a, &pt) < 0 || ARG_CNT(a) != 0) {
    test_error(unit, "%s: Invalid option: %s\n",
           ARG_CMD(a), ARG_CUR(a) ? ARG_CUR(a) : "*");
    parse_arg_eq_done(&pt);
#ifdef BCM_ARAD_SUPPORT
       if (SOC_IS_ARAD(unit)) {
           cli_out("%s\n",tr40_test_usage);
       }
#endif
    return(-1);
    }

#ifdef BCM_ARAD_SUPPORT
    if (p->xd_file) {
        xfile = sal_strdup(p->xd_file);
    } else {
        xfile = NULL;
    }
#endif
    parse_arg_eq_done(&pt);
#ifdef BCM_ARAD_SUPPORT

    if (SOC_IS_ARAD(unit)) {
        if (p->test_usage[0] == '?') {
             cli_out("%s\n",tr40_test_usage);
        }
    }
    p->xd_file = xfile;
#endif

    if (p->p_ops > 1000) {
        test_error(unit, "Operations must be 1 <= # <= 1000\n");
        return(-1);
    }

    if ((p->p_opt != P_O_PORT) && (!SOC_IS_ARAD(unit))) {
        test_error(unit, "Only PortBitMap option supported for bcm_tx\n");
        return -1;
    }

    if (p->len_end > PACKET_MAX_BUFFER) {
        test_error(unit, "Max supported pkt size is %d\n", PACKET_MAX_BUFFER);
        p->len_end = PACKET_MAX_BUFFER;
        return -1;
    }

    if (tpacket_setup(unit, p) < 0) {
        (void)tpacket_tx_done(unit, p);
        return(-1);
    }

    /* Insert ARL/MARL entry */

    switch(p->p_opt) {
        case P_O_UC_HIT:
            bcm_l2_addr_t_init(&l2, tp_mac_uc, p->p_vid);
            l2.flags |= BCM_L2_STATIC;
            if (0 > (rv = bcm_l2_addr_add(unit, &l2))) {
                test_error(unit, "bcm_l2_addr_add failed: %s\n", bcm_errmsg(rv));
                return(-1);
            }
            ENET_SET_MACADDR(p->p_dst, tp_mac_uc);
            break;
        case P_O_UC_MISS:
            if (0 > (rv = bcm_l2_addr_delete(unit, tp_mac_uc, p->p_vid))) {
                test_error(unit, "bcm_l2_addr_delete failed: %s\n", bcm_errmsg(rv));
                return(-1);
            }
            ENET_SET_MACADDR(p->p_dst, tp_mac_uc);
            break;
        case P_O_MC_HIT:
            bcm_mcast_addr_t_init(&mc, tp_mac_mc, p->p_vid);
            if (0 > (rv = bcm_mcast_addr_add(unit, &mc))) {
                test_error(unit, "bcm_mcast_addr_add failed: %s\n", bcm_errmsg(rv));
                return(-1);
            }
            ENET_SET_MACADDR(p->p_dst, tp_mac_mc);
            break;
        case P_O_MC_MISS:
            if (0 > (rv = bcm_mcast_addr_remove(unit, tp_mac_mc, p->p_vid))) {
                if (rv != BCM_E_NOT_FOUND) {
                test_error(unit, "bcm_mcast_addr_remove failed: %s\n", bcm_errmsg(rv));
                return(-1);
                }
            }
            ENET_SET_MACADDR(p->p_dst, tp_mac_mc);
            break;

        case P_O_BC:
            ENET_SET_MACADDR(p->p_dst, tp_mac_bc);
            break;
        case P_O_PORT:
#ifdef BCM_ARAD_SUPPORT         
            if (SOC_IS_ARAD(unit)) {
                if (!p->p_dst) {
                    ENET_SET_MACADDR(p->p_dst, tp_mac_dpp_dst);
                }
                if (!p->p_src) {
                    ENET_SET_MACADDR(p->p_src, tp_mac_dpp_src);
                }
            } else 
#endif
            {
                ENET_SET_MACADDR(p->p_dst, tp_mac_bc);
            }
            
            break;
            
        case P_O_DPP_PORT:
            
            ENET_SET_MACADDR(p->p_dst, tp_mac_dpp_dst);
            ENET_SET_MACADDR(p->p_src, tp_mac_dpp_src);
            break;
        default:
            return(-1);
    }

    *pa = (void *)p;

#if (defined(INCLUDE_KNET) && defined(LINUX) && (!defined(__KERNEL__)))
    tx_knetif_setup(unit, p);
#endif

    return(0);
}

#ifdef BCM_ARAD_SUPPORT
STATIC int8 
dpp_tx_load_packet_tr(int unit, char *fname, int length, uint8 *packet_in)
{

#ifndef __KERNEL__
  FILE * pFile;
  long size;
  uint8  *ptr;
  int                 i;
  int8                c;

  ptr =  packet_in;
  pFile = sal_fopen(fname,"rb");
  if (pFile==NULL) return -1;
  else
  {
    fseek (pFile, 0, SEEK_END);
    size=ftell (pFile);
    if (fseek (pFile, 0, SEEK_SET) != 0){
        fclose (pFile);
        return -1;
    }
    if (size < length) {
        fclose (pFile);
        return -1;
    }
/*    cli_out("\nFile read:"); */
    for (i = 0; i < length; i++) {
       c = (int8)getc(pFile); 
       /* cli_out("%02x",(uint8)c); */
       *ptr++ = (uint8)c;
    }
/*    cli_out("\n"); */
    fclose (pFile);
  }
#endif /* __KERNEL__ */
  return 0;
}
#endif









