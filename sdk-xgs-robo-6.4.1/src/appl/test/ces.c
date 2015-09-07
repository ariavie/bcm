/*
 * $Id: ces.c,v 1.6 Broadcom SDK $
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
 * Built-in Self Test for CES services
 */

#if defined(INCLUDE_CES)

#include <sal/types.h>
#include <soc/mem.h>
#include <shared/bsl.h>
#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include "testlist.h"
#include <bcm/ces.h>
#include <bcm/port.h>
#include <bcm_int/esw/ces.h>


#define BCM_CES_TEST_E_NONE          0
#define BCM_CES_TEST_E_NO_PKT_TX    -1
#define BCM_CES_TEST_E_NO_PKT_RX    -2
#define BCM_CES_TEST_E_PKT_TX_GT_RX -3
#define BCM_CES_TEST_E_PKT_RX_GT_TX -4
#define BCM_CES_TEST_E_PRBS_NO_LOCK -5
#define BCM_CES_TEST_E_PRBS_ERRORS  -6
#define BCM_CES_TEST_E_API_FAILED   -7


#define BCM_CES_TEST_LOCK_LOOP_COUNT   5
#define BCM_CES_TEST_LOCK_STABLE_COUNT 3

#define BCM_CES_TEST_UNSTRUCT_MAX 16
#define BCM_CES_TEST_STRUCT_MAX   64

/*
 * Structured test data
 */
typedef struct {
    int port;
    int start;
    int end;
} bcm_ces_test_struct_entry_t;

bcm_ces_test_struct_entry_t bcm_ces_test_struct_data[BCM_CES_TEST_STRUCT_MAX] = {
/* Port  FirstSlot  LastSlot */
    {39, 1, 24}, /* 0 */
    {40, 1, 12},
    {40, 13, 24},
    {41, 1, 6},
    {41, 7, 12},
    {41, 13, 18},
    {41, 19, 24},
    {42, 1, 3},
    {42, 4, 6},
    {42, 7, 9},
    {42, 10, 12}, /* 10 */
    {42, 13, 15},
    {42, 16, 18},
    {42, 19, 21},
    {42, 22, 24},
    {43, 1, 2},
    {43, 3, 4},
    {43, 5, 6},
    {43, 7, 8},
    {43, 9, 10},
    {43, 11, 12}, /* 20 */
    {43, 13, 14},
    {43, 15, 16},
    {43, 17, 18},
    {43, 19, 20},
    {43, 21, 22},
    {43, 23, 24},
    {44, 1, 1},
    {44, 2, 2},
    {44, 3, 3},
    {44, 4, 4}, /* 30 */
    {44, 5, 5},
    {44, 6, 6},
    {44, 7, 7},
    {44, 8, 8},
    {44, 9, 9},
    {44, 10, 10},
    {44, 11, 11},
    {44, 12, 12},
    {44, 13, 13},
    {44, 14, 14}, /* 40 */
    {44, 15, 15},
    {44, 16, 16},
    {44, 17, 17},
    {44, 18, 18},
    {44, 19, 19},
    {44, 20, 20},
    {44, 21, 21},
    {44, 22, 22},
    {44, 23, 23},
    {44, 24, 24}, /* 50 */
    {45, 1, 12},
    {45, 13, 24},
    {46, 1, 12},
    {46, 13, 24},
    {47, 1, 23},
    {47, 24, 24},
    {48, 1, 24},
    {49, 1, 24},
    {50, 1, 24},
    {51, 1, 24}, /* 60 */
    {52, 1, 24},
    {53, 1, 24},
    {54, 1, 24},
};
  
/*
 * Encap test data
 */
typedef struct {
    bcm_ces_encapsulation_t encap;
    int                     vlan;
    int                     ip_version;
    char                   *name;
} bcm_ces_test_encap_entry_t;

bcm_ces_test_encap_entry_t bcm_ces_test_encap_data[] = {
    {bcmCesEncapsulationIp,   0, bcmCesIpV4, "IPv4"},
    {bcmCesEncapsulationIp,   1, bcmCesIpV4, "IPv4+VLAN"},
    {bcmCesEncapsulationIp,   0, bcmCesIpV6, "IPv6"},
    {bcmCesEncapsulationIp,   1, bcmCesIpV6, "IPv6+VLAN"},
    {bcmCesEncapsulationMpls, 1, 0, "MPLS+VLAN"},
    {bcmCesEncapsulationEth,  0, 0, "Eth"},
    {bcmCesEncapsulationEth,  1, 0, "Eth+VLAN"},
    {bcmCesEncapsulationL2tp, 0, bcmCesIpV4, "IPv4-L2TP"},
    {bcmCesEncapsulationL2tp, 1, bcmCesIpV4, "IPv4-L2TP+VLAN"},
    {bcmCesEncapsulationL2tp, 0, bcmCesIpV6, "IPv6-L2TP"},
    {bcmCesEncapsulationL2tp, 1, bcmCesIpV6, "IPv6-L2TP+VLAN"},
};

char *bcm_ces_tdm_string[] = {
    "Unstruct", "Struct" };

void ces_test_msg(char *encap, char *tdm, char *msg) {
    cli_out("[Encap:%s  TDM:%s] %s\n", encap, tdm, msg);
}


/**
 * Function:
 *      ces_test_init()
 * Purpose:
 *      Reset CES
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      Nothing
 * Notes:
 */
int
ces_test_init(int u, args_t *a, void **p)
{

    if (bcm_ces_services_init(u) != BCM_E_NONE) {
	goto fail;
    }

    return 0;

 fail:
    return -1;
}

/**
 * Function:
 *      ces_test_ge0_loopback_set()
 * Purpose:
 *      Set ge0 loopback state 
 * Parameters:
 *      unit - (IN) Unit number.
 *      status - 1 = loopback, 0 = no loopback
 * Returns:
 *      Nothing
 * Notes:
 */
void ces_test_ge0_loopback_set(int unit, int status) {
    uint32 rval;

    READ_COMMAND_CONFIGr(unit, 1, &rval);
    soc_reg_field_set(unit, COMMAND_CONFIGr, &rval, LINE_LOOPBACKf, status);
    WRITE_COMMAND_CONFIGr(unit, 1, rval);
}


/**
 * Function:
 *      ces_test_prbs()
 * Purpose:
 *      Run a PRBS test on the specified service.
 * Parameters:
 *      unit - (IN) Unit number.
 *      service - CES service number
 * Returns:
 *      Nothing
 * Notes:
 */
int ces_test_prbs(int unit, bcm_ces_service_t service, uint32 time) {
    bcm_ces_service_config_t *config;
    int rc;
    int status;
    int j;
    int lock_stable;

    config = (bcm_ces_service_config_t *)sal_alloc(sizeof(bcm_ces_service_config_t), "CES Config");
    if (config == NULL) {
        cli_out("Out of memory\n");
        return -1;
    }

    /*
     * Get service configuration
     */
    rc = bcm_ces_service_config_get(unit, service, config);
    if (rc != BCM_E_NONE) {
        sal_free(config);
	return rc;
    }

    /*
     * Enable PRBS
     */
    rc = bcm_esw_ces_framer_prbs_set(unit, 
				     config->egress_channel_map.circuit_id[0], 
				     1, 
				     2, 
				     config->ingress_channel_map.slot[0], 
				     config->ingress_channel_map.slot[0] + config->ingress_channel_map.size - 1);

    /*
     * Test
     */
    j = 0;
    status = 0x0100;
    lock_stable = 0;
    cli_out("Waiting for PRBS pattern lock...");

    do {
	sal_sleep(1);
	j++;
	bcm_esw_ces_framer_prbs_status(unit, config->egress_channel_map.circuit_id[0], &status);
	if (status) {
	    lock_stable = 0;
	} else {
	    lock_stable++;
	}
    } while (j < BCM_CES_TEST_LOCK_LOOP_COUNT && lock_stable < BCM_CES_TEST_LOCK_STABLE_COUNT);

    if (status == 0) {
	cli_out("Locked, checking stability for %d seconds...", time);
	for (j = 0;j < time;j++) {

	    sal_sleep(1);
	    bcm_esw_ces_framer_prbs_status(unit, config->egress_channel_map.circuit_id[0], &status);
	    if (status) {
		if (status & 0x0100) {
		    cli_out(" Loss of lock");
		    rc = BCM_CES_TEST_E_PRBS_NO_LOCK;
		} else {
		    cli_out(" Errors:%d", (0x00FF & status));
		    rc = BCM_CES_TEST_E_PRBS_ERRORS;
		}
		break;
	    }
	}
	
	if (!status) {
	    cli_out("OK");
	}

    } else {
	if (status & 0x0100) {
	    cli_out(" No pattern lock");
	    rc = BCM_CES_TEST_E_PRBS_NO_LOCK;
	} else {
	    cli_out(" Errors:%d", (0x00FF & status));
	    rc = BCM_CES_TEST_E_PRBS_ERRORS;
	}
    }

    cli_out("\n");

    /*
     * Turn off PRBS
     */
#if 0
    bcm_esw_ces_framer_prbs_set(unit, config->egress_channel_map.circuit_id[0], 1, 0, 0, 0);
#else
    cli_out("<<<< PRBS STILL ON >>>>\n");
#endif

    sal_free(config);
    return rc;
}


/**
 * Function:
 *      ces_test_packet_status()
 * Purpose:
 *      Get the current packet Tx and Rx status for a given CES service
 * Parameters:
 *      unit - (IN) Unit number.
 *      service - CES service number
 * Returns:
 *      Nothing
 * Notes:
 */
int ces_test_packet_status(int unit, bcm_ces_service_t service_start, bcm_ces_service_t service_end, int tx, int rx) {
    int rc = BCM_CES_TEST_E_NONE;
    bcm_ces_service_pm_stats_t pm_counts;
    uint32 in;
    uint32 out;
    int i;

    cli_out("Verifying CES packet ");
    if (tx)
	cli_out("Tx and ");
    else
	cli_out("!Tx and ");
    if (rx)
	cli_out("Rx... ");
    else
	cli_out("!Rx... ");

    /*
     * The PM counts will show if packets are being sent and recieved.
     */
    sal_sleep(2);
    sal_memset(&pm_counts, 0, sizeof(bcm_ces_service_pm_stats_t));

    /*
     * Reset PM stats
     */
    for (i = service_start;i <= service_end;i++)
	rc = bcm_ces_service_pm_clear(unit, i);

    /*
     * Wait a moment for counts to be accumulated (note that
     * stats are harvested one every second).
     */
    sal_sleep(2);

    for (i = service_start;i <= service_end && rc == BCM_CES_TEST_E_NONE;i++) {
	rc = bcm_ces_service_pm_get(unit, i, &pm_counts);
	out = pm_counts.transmitted_packets;
	in = pm_counts.received_packets;

	if (in == 0 && rx) {
	    cli_out("Service %d not receiving packets (tx:%u rx:%u)\n", i, out, in);
	    rc =  BCM_CES_TEST_E_NO_PKT_RX;
	} else if (out == 0 && tx) {
	    cli_out("Service %d not transmitting packets (tx:%u rx:%u)\n", i, out, in);
	    rc = BCM_CES_TEST_E_NO_PKT_TX;
	} else if (in > 0 && !rx) {
	    cli_out("Service %d unexpectidly receiving packets (tx:%u rx:%u)\n", i, out, in);
	    rc = BCM_CES_TEST_E_NO_PKT_RX;
	} else if (out > 0 && !tx) {
	    cli_out("Service %d unexpectidly transmitting packets (tx:%u rx:%u)\n", i, out, in);
	    rc = BCM_CES_TEST_E_NO_PKT_TX;
	}
    }

    if (rc == BCM_CES_TEST_E_NONE)
	cli_out("OK\n");

    return rc;
}


/**
 * Function:
 *      ces_test_sync_status()
 * Purpose:
 *      Get the current sync status for a given CES service
 * Parameters:
 *      unit - (IN) Unit number.
 *      service - CES service number
 * Returns:
 *      Nothing
 * Notes:
 */
int ces_test_sync_status(int unit, bcm_ces_service_t service_start, bcm_ces_service_t service_end) {
    bcm_ces_service_egress_status_t status;
    int rc = 0;
    int i;

    cli_out("Verifying CES sync status...");

    for (i = service_start;i <= service_end;i++) { 
	rc = bcm_ces_egress_status_get(unit, i, &status);

	if (status.sync_state != bcmCesPacketSyncAops) {
	    cli_out("Service %2d: Loss of Packet Sync\n", i);
	    rc = -1;
	}
    }
    cli_out("OK\n");

    return rc;
}

/**
 * Function:
 *      ces_test_create_service()
 * Purpose:
 *      Create a CES service
 * Parameters:
 *      unit - (IN) Unit number.
 *      service - CES service number
 * Returns:
 *      Nothing
 * Notes:
 */
int ces_test_create_service(int unit, 
			    bcm_ces_service_t service, 
			    bcm_ces_encapsulation_t encap, 
			    int vlan,
			    int ip_version,
			    bcm_port_t port, 
			    uint32 start,
                            uint32 end) {
    bcm_ces_service_config_t *config;
    uint8 mac[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    uint8 ipv6Address[16] = {0x20, 0x01, 0x0d, 0xb8, 0x85, 0xa3, 0x00, 0x00, 
			     0x00, 0x00, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x34};
    int rc = 0;
    int i;
    uint32 num_slots;

    if (start == 0)
	num_slots = 0;
    else
	num_slots = end - start + 1;

    /*
     * Setup service configuration. In this instance it will be:
     *
     * encap: ETH loopback to self
     * vlan: None
     * Strict header: MAC
     * TDM Port: Second (tdm port 40)
     */
    config = (bcm_ces_service_config_t *)sal_alloc(sizeof(bcm_ces_service_config_t), "CES Config");
    if (config == NULL)
        return BCM_E_MEMORY;

    sal_memset(config, 0, sizeof(bcm_ces_service_config_t));

    /*
     * Channel maps
     */
    config->ingress_channel_map.first = 0;          /* The first entry is at index 0 */
    if (num_slots == 0) {
	config->ingress_channel_map.size = 1;           /* One entry in the table */
	config->ingress_channel_map.circuit_id[0] = port; /* TDM port */
	config->ingress_channel_map.slot[0] = 0;        /* Setting slot zero indicates that the whole T1 is used */
    } else {
	config->ingress_channel_map.size = num_slots;           /* One entry in the table */

	for (i = 0;i < num_slots;i++) {
	    config->ingress_channel_map.circuit_id[i] = port; /* TDM port */
	    config->ingress_channel_map.slot[i] = start;
	    start++;
	}
    }

    /*
     * The egress map is a copy of the ingress map
     */
    sal_memcpy(&config->egress_channel_map, &config->ingress_channel_map, sizeof(bcm_ces_channel_map_t));

    /*
     * Ingress channel config
     */
    config->ingress_channel_config.dba = FALSE;
    config->ingress_channel_config.auto_r_bit = TRUE;
    config->ingress_channel_config.mef_len_support = FALSE;
    config->ingress_channel_config.pbf_size = 2048;
    
    /*
     * Egress channel config
     */
    config->egress_channel_config.packet_sync_selector = 0;
    config->egress_channel_config.rtp_exists    = FALSE;
    config->egress_channel_config.frames_per_packet  = BCM_CES_FRAMES_PER_PACKET;

    config->egress_channel_config.jbf_ring_size = BCM_CES_JBF_RING_SIZE;
    config->egress_channel_config.jbf_win_size  = BCM_CES_JBF_WINDOW_SIZE;
    config->egress_channel_config.jbf_bop       = BCM_CES_JBF_BREAK_OUT_POINT;

    /*
     * Packet header
     */
    config->encapsulation = encap;
    config->header.encapsulation = encap;
    sal_memcpy(config->header.eth.source, mac, sizeof(mac));
    sal_memcpy(config->header.eth.destination, mac, sizeof(mac));

    switch(encap) {
    case bcmCesEncapsulationEth:
	config->header.vc_label = 0xAA00 + service;

	/*
	 * Strict header
	 */
	sal_memcpy(config->strict_data.eth.source, mac, sizeof(mac));
	break;

    case bcmCesEncapsulationIp:
	if (ip_version == bcmCesIpV4) {
	    config->header.ip_version       = bcmCesIpV4;
	    config->header.ipv4.source      = 0x0a0a0a0a;
	    config->header.ipv4.destination = 0x0a0a0a0a;
	    config->header.ipv4.ttl         = 10;

	    config->strict_data.ipv4.source = config->header.ipv4.destination;
	} else if (ip_version == bcmCesIpV6) {
	    config->header.ip_version       = bcmCesIpV6;
	    sal_memcpy(config->header.ipv6.source, ipv6Address, sizeof(ip6_addr_t));
	    sal_memcpy(config->header.ipv6.destination, ipv6Address, sizeof(ip6_addr_t));
	    config->header.ipv6.hop_limit = 255;

	    sal_memcpy(config->strict_data.ipv6.source, config->header.ipv6.destination, sizeof(ip6_addr_t));
	}

	config->header.udp.source       = 0xAA00 + service;
	config->header.udp.destination  = 0xAA00 + service;

	config->strict_data.ip_version  = config->header.ip_version;

	break;

    case bcmCesEncapsulationMpls:
	config->header.vc_label   = 0xAA00 + service;
	config->header.mpls_count = 1;
	config->header.mpls[0].label        = 0xAA00 + service;
	config->header.mpls[0].experimental = 0x12345678;
	config->header.mpls[0].ttl          = 10;
	break;

    case bcmCesEncapsulationL2tp:
	config->encapsulation = bcmCesEncapsulationL2tp;
	config->header.encapsulation = bcmCesEncapsulationL2tp;

	if (ip_version == bcmCesIpV4) {
	    config->header.ip_version       = bcmCesIpV4;
	    config->header.ipv4.source      = 0x0a0a0a0a;
	    config->header.ipv4.destination = 0x0a0a0a0a;
	    config->header.ipv4.ttl         = 10;

	    config->strict_data.ipv4.source = config->header.ipv4.destination;
	} else if (ip_version == bcmCesIpV6) {
	    config->header.ip_version       = bcmCesIpV6;
	    sal_memcpy(config->header.ipv6.source, ipv6Address, sizeof(ip6_addr_t));
	    sal_memcpy(config->header.ipv6.destination, ipv6Address, sizeof(ip6_addr_t));
	    config->header.ipv6.hop_limit = 255;

	    sal_memcpy(config->strict_data.ipv6.source, config->header.ipv6.destination, sizeof(ip6_addr_t));
	}

	config->header.vc_label                = 0xAA00 + service;
	config->header.l2tpv3_count            = 1;
	config->header.l2tpv3.udp_mode         = 1;           /* Using UDP */
	config->header.l2tpv3.header           = 0x30000;  /* Header */
	config->header.l2tpv3.session_local_id = 0xAA00 + service;    /* Session local ID */
	config->header.l2tpv3.session_peer_id  = 0xAA00 + service;     /* Session peer ID */
	config->header.l2tpv3.local_cookie1    = 1;       /* Local cookie 1 */
	config->header.l2tpv3.local_cookie2    = 2;       /* Local cookie 2 */
	config->header.l2tpv3.peer_cookie1     = 1;        /* Peer cookie 1 */
	config->header.l2tpv3.peer_cookie2     = 2;        /* Peer cookie 2 */

	config->header.udp.destination = 1701;
	config->header.udp.source      = 1701;

	/*
	 * Strict header
	 */
	config->strict_data.encapsulation = bcmCesEncapsulationL2tp;
	config->strict_data.l2tpv3_count = config->header.l2tpv3_count;
	config->strict_data.l2tpv3.local_cookie1 = config->header.l2tpv3.local_cookie1;
	config->strict_data.ip_version  = config->header.ip_version;
	break;
    }

    /*
     * VLAN?
     */
    if (vlan) {
	config->header.vlan_count = 1;
	config->header.vlan[0].vid = 5;
	config->header.vlan[0].priority = 1;

	config->strict_data.vlan_count = 1;
	config->strict_data.vlan[0].vid = config->header.vlan[0].vid;
    }

    /*
     * Create service
     */
    rc = bcm_ces_service_create(unit, BCM_CES_WITH_ID, config, &service);
    if (rc != BCM_E_NONE) {
	return -1;
    }

    sal_free(config);
    return rc;
}


/**
 * Function:
 *      ces_test_unstruct()
 * Purpose:
 *      Unstructured tests
 * Parameters:
 *      unit - (IN) Unit number.
 *      service - CES service number
 * Returns:
 *      Nothing
 * Notes:
 */
int
ces_test_unstruct(int u, args_t *a, void *p)
{
    int rc = 0;
    bcm_tdm_port_config_t    tdm_config; 
    bcm_ces_service_t service;
    int i;
    int e;
    int num_encap = sizeof(bcm_ces_test_encap_data) / sizeof(bcm_ces_test_encap_entry_t);
    bcm_port_t tdm_port = 39;

    COMPILER_REFERENCE(a);

    /*
     * Set TDM port in to unstructured mode
     */
    for (i = 0;i < BCM_CES_TEST_UNSTRUCT_MAX;i++) {
	    rc = bcm_port_tdm_config_get(u, tdm_port + i, &tdm_config);
	    tdm_config.b_structured = FALSE;
	    rc = bcm_port_tdm_config_set(u, tdm_port + i, &tdm_config);
    }

    for (e = 0;e < num_encap;e++) {

	cli_out("Creating %s test services...", bcm_ces_test_encap_data[e].name);

	/*
	 * Create sixteen services 
	 */
	for (i = 0;i < BCM_CES_TEST_UNSTRUCT_MAX;i++) {
	    /*
	     * Setup service configuration. In this instance it will be:
	     *
	     * encap: ETH loopback to self
	     * vlan: None
	     * Strict header: MAC
	     * TDM Port: 
	     */
	    service = i;

	    rc = ces_test_create_service(u, 
					 service, 
					 bcm_ces_test_encap_data[e].encap,
					 bcm_ces_test_encap_data[e].vlan,
					 bcm_ces_test_encap_data[e].ip_version,
					 tdm_port + i, 
					 0,
					 0);

	    if (rc != BCM_E_NONE) {
		cli_out("Service %2d create: Failed\n", service);
		return -1;
	    }

	    /*
	     * Enable service
	     */
	    rc = bcm_ces_service_enable_set(u, service, TRUE);
	    if (rc != BCM_E_NONE) {
		cli_out("Service %2d enable: Failed\n", service);
		return -1;
	    }
	}

	cli_out("OK\n");

	/*
	 * Setup ge port loopback
	 */
	ces_test_ge0_loopback_set(u, TRUE);


	/*
	 * Verify operation
	 */
	rc = ces_test_packet_status(u, 0, BCM_CES_TEST_UNSTRUCT_MAX - 1, TRUE, TRUE);

	if (rc != BCM_CES_TEST_E_NONE) {
	    cli_out(" Failed\n");
	    return -1;
	} 

	/*
	 * Sync status
	 */
	rc = ces_test_sync_status(u, service, BCM_CES_TEST_UNSTRUCT_MAX - 1);
    
	if (rc != BCM_CES_TEST_E_NONE) {
	    cli_out(" Failed\n");
	    return -1;
	}

	cli_out("Verifying CES data path...\n");

	for (i = 0;i < BCM_CES_TEST_UNSTRUCT_MAX;i++) {
	    service = i;

	    cli_out("Service %d: ", service);

	    /*
	     * Run PRBS test
	     */
	    rc = ces_test_prbs(u, service, 10);
	    if (rc != BCM_CES_TEST_E_NONE) {
		cli_out(" Failed\n");
		return -1;
	    } 
	}

	/*
	 * Remove loopback and verify that packets stop being received
	 */
	ces_test_ge0_loopback_set(u, FALSE);
	cli_out("Verifying CES broken data path...\n");

	rc = ces_test_packet_status(u, service, BCM_CES_TEST_UNSTRUCT_MAX - 1, TRUE, FALSE);
	if (rc != BCM_CES_TEST_E_NONE) {
	    cli_out(" Failed\n");
	    return -1;
	}

	bcm_ces_service_destroy_all(u);
    }

    return 0;
}


/**
 * Function:
 *      ces_test_struct()
 * Purpose:
 *      Structured tests
 * Parameters:
 *      unit - (IN) Unit number.
 *      service - CES service number
 * Returns:
 *      Nothing
 * Notes:
 */
int
ces_test_struct(int u, args_t *a, void *p)
{
    int rc = 0;
    bcm_ces_service_config_t config;
    bcm_ces_service_t service;
    int i;
    bcm_tdm_port_config_t portConfig;
    int e;
    int num_encap = sizeof(bcm_ces_test_encap_data) / sizeof(bcm_ces_test_encap_entry_t);
    bcm_port_t tdm_port = 39;

    COMPILER_REFERENCE(a);

    /*
     * Set TDM ports to structured
     */
    for (i = 0;i < BCM_CES_TEST_UNSTRUCT_MAX;i++, tdm_port++) {
	bcm_port_tdm_config_get(u, tdm_port, &portConfig);
	portConfig.b_structured = TRUE;
	bcm_port_tdm_config_set(u, tdm_port, &portConfig);
    }
 
    for (e = 0;e < num_encap;e++) {

	cli_out("Creating %s test services...", bcm_ces_test_encap_data[e].name);

	/*
	 * Create sixty four services 
	 *
	 * Service Port Slot(s)
	 * ------- ---- -------------------------
	 *       0   39 1 - 24
	 *       1   40 1 - 12
	 *       2   40 13 - 24
	 *       3   41 1 - 6
	 *       4   41 7 - 12
	 *       5   41 13 - 18
	 *       6   41 19 - 24
	 *       7   42 1 - 3
	 *       8   42 4 - 6
	 *       9   42 7 - 9
	 *      10   42 10 - 12
	 *      11   42 13 - 16
	 *      12   42 17 - 19
	 *      13   42 19 - 21
	 *      14   42 22 - 24
	 *      15   43 1 - 2
	 *      16   43 3 - 4
	 *      17   43 5 - 6
	 *      18   43 7 - 8
	 *      19   43 9 - 10
	 *      20   43 11 - 12
	 *      21   43 13 - 14
	 *      22   43 15 - 16
	 *      23   43 17 - 18
	 *      24   43 19 - 20
	 *      25   43 21 - 22
	 *      26   43 23 - 24
	 *      27   44 1
	 *      28   44 2
	 *      29   44 3
	 *      30   44 4
	 *      31   44 5
	 *      32   44 6
	 *      33   44 7
	 *      34   44 8
	 *      35   44 9
	 *      36   44 10
	 *      37   44 11
	 *      38   44 12
	 *      39   44 13
	 *      40   44 14
	 *      41   44 15
	 *      42   44 16
	 *      43   44 17
	 *      44   44 18
	 *      45   44 19
	 *      46   44 20
	 *      47   44 21
	 *      48   44 22
	 *      49   44 23
	 *      50   44 24
	 *      51   45 1 - 12
	 *      52   45 13 - 24
	 *      53   46 1 - 12
	 *      54   46 13 - 24
	 *      55   47 1 - 23
	 *      56   47 23 - 24
	 *      57   48 1 - 24
	 *      58   49 1 - 24
	 *      59   50 1 - 24
	 *      60   51 1 - 24
	 *      61   52 1 - 24
	 *      62   53 1 - 24
	 *      63   54
	 */
	for (i = 0;i < BCM_CES_TEST_STRUCT_MAX;i++) {

	    /*
	     * Setup service configuration. In this instance it will be:
	     *
	     * encap: ETH loopback to self
	     * vlan: None
	     * Strict header: MAC
	     * TDM Port: 
	     */
	    service = i;
	    sal_memset(&config, 0, sizeof(bcm_ces_service_config_t));

	    rc = ces_test_create_service(u, 
					 service, 
					 bcm_ces_test_encap_data[e].encap,
					 bcm_ces_test_encap_data[e].vlan,
					 bcm_ces_test_encap_data[e].ip_version,
					 bcm_ces_test_struct_data[i].port, 
					 bcm_ces_test_struct_data[i].start,
					 bcm_ces_test_struct_data[i].end);

	    if (rc != BCM_E_NONE) {
		cli_out("Failed\n");
		return -1;
	    }

	    /*
	     * Enable service
	     */
	    rc = bcm_ces_service_enable_set(u, service, TRUE);
	    if (rc != BCM_E_NONE) {
		cli_out("Service %2d create: Failed\n", service);
		return -1;
	    }
	}

	cli_out("OK\n");

	/*
	 * Setup ge port loopback
	 */
	ces_test_ge0_loopback_set(u, TRUE);


	/*
	 * Verify operation
	 */
	rc = ces_test_packet_status(u, service, BCM_CES_TEST_STRUCT_MAX - 1, TRUE, TRUE);

	/*
	 * Result
	 */
	if (rc != BCM_CES_TEST_E_NONE) {
	    cli_out(" Failed\n");
	    return -1;
	} else {
	    cli_out("OK\n");
	}

	/*
	 * Sync status
	 */
	rc = ces_test_sync_status(u, service, BCM_CES_TEST_UNSTRUCT_MAX - 1);

	if (rc != BCM_CES_TEST_E_NONE) {
	    cli_out(" Failed\n");
	    return -1;
	} 


	cli_out("Verifying CES data path...\n");

	for (i = 0;i < BCM_CES_TEST_STRUCT_MAX;i++) {
	    service = i;

	    cli_out("Service %d: ", service);

	    /*
	     * Run PRBS test
	     */
	    rc = ces_test_prbs(u, service, 10);
	    if (rc != BCM_CES_TEST_E_NONE) {
		cli_out(" Failed\n");
		return -1;
	    } 
	}

	/*
	 * Remove loopback and verify that packets stop being received
	 */
	ces_test_ge0_loopback_set(u, FALSE);
	cli_out("Verifying CES broken data path...\n");

	rc = ces_test_packet_status(u, service, service, TRUE, FALSE);
	if (rc != BCM_CES_TEST_E_NONE) {
	    cli_out(" Failed\n");
	    return -1;
	} 

	bcm_ces_service_destroy_all(u);
    }

    return 0;
}



int
ces_test_done(int u, void *p)
{
    int			rv = 0;

    /*
     * Clear ge0 loopback
     */
#if 0
    ces_test_ge0_loopback_set(u, FALSE);
#else
    cli_out("<<<< GE0 LOOPBACK STILL ON >>>>\n");
#endif

#if 0
    /*
     * Reset CES
     */
    bcm_ces_services_init(u);
#else
    cli_out("<<<< CES NOT CLEANED UP >>>>\n");
#endif
    if (p != NULL)
	sal_free(p);

    if (rv < 0) {
	test_error(u, "Post-CES reset failed: %s\n", soc_errmsg(rv));
	return -1;
    }

    return 0;
}
#else
int  __no_complilation_complaints_about_ces__3;
#endif /* BCM_ESW_SUPPORT etc */
