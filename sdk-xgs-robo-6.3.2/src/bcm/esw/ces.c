/*
 * $Id: ces.c 1.56 Broadcom SDK $
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
 */

#ifdef INCLUDE_CES
#include <soc/defs.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <sal/core/libc.h>
#include <bcm/error.h>
#include <bcm_int/esw/ces.h>
#include <pub/nd_api.h>
#include <pub/cls_mp_handler.h>
#include <shared/alloc.h>
#include <soc/shared/mos_msg_common.h>
#include <soc/shared/mos_msg_ces.h>
#include <soc/shared/ces_pack.h>
#include <soc/uc_msg.h>
#include <sal/appl/sal.h>
#include <bcm/types.h>
#include <shared/pbmp.h>
#include <bcm_int/api_xlate_port.h>
#include <bcm_int/esw/link.h>
#undef BCM_HIDE_DISPATCHABLE
#include <bcm/link.h>
#include <bcm/port.h>
#include <soc/dport.h>
#ifdef BCM_WARM_BOOT_SUPPORT
#include <bcm/module.h>
#include <bcm_int/esw/switch.h>
#endif /* BCM_WARM_BOOT_SUPPORT */

#define CES_SDK_VERSION         0x01000000
#define CES_UC_MIN_VERSION      0x01000000

#define BCM_CES_IPV4_TOS     0
#define BCM_CES_IPV4_TTL     10
#define BCM_CES_IP_VERSION   4
#define BCM_CES_UDP_DESTINATION 0xef00
#define BCM_CES_UDP_SOURCE 0xef00
#define BCM_CES_VLAN_COUNT 1
#define BCM_CES_MPLS_COUNT 0

#define BCM_CES_E1_SLOT_MAX 32
#define BCM_CES_T1_SLOT_MAX 24

/*
 * RTP Timestamp step siae and error correction period values.
 * Table is indexed using [common reference clock rate index][RTP timestamp rate index]
 */
bcm_ces_rtp_tss_table_t bcm_ces_rtp_tss_table[BCM_CES_RTP_TSS_TABLE_INDEX_CRC_MAX][BCM_CES_RTP_TSS_TABLE_INDEX_RTR_MAX] = {
    {{0x01000000, FALSE, 0}, {0x00, FALSE, 0}, {0x06A1D2E7, TRUE, 193}, {0x0A9C84A5, TRUE, 193}, {0x0C973663, TRUE, 193}, {0x103113E6, TRUE, 193}},
    {{0x00, FALSE, 0}, {0x01000000, FALSE, 0}, {0x05000000, FALSE, 0}, {0x08000000, FALSE, 0}, {0x097E0000, FALSE, 0}, {0x0C350000, FALSE, 0}},
    {{0x00269999, FALSE, 0}, {0x00333333, FALSE, 0}, {0x01000000, FALSE, 0}, {0x0199999A, TRUE, 5}, {0x01E60000, FALSE, 0}, {0x02710000, FALSE, 0}},
    {{0x00182000, FALSE, 0}, {0x00200000, FALSE, 0}, {0x00A00000, FALSE, 0}, {0x01000000, FALSE, 0}, {0x012FC000, FALSE, 0}, {0x0186A000, FALSE, 0}},
    {{0x00145520, TRUE, 1215}, {0x001AF835, TRUE, 1215}, {0x0086D906, TRUE, 1215}, {0x00D7C1A3, TRUE, 1215}, {0x01000000, FALSE, 0}, {0x014937D6, TRUE, 1215}},
    {{0x000FCF81, TRUE, 3125}, {0x0014F8B6, TRUE, 3125}, {0x0068DB8C, TRUE, 3125}, {0x00A7C5AD, TRUE, 3125}, {0x00C710CC, TRUE, 3125}, {0x01000000, FALSE, 0}},
    {{0x00032981, TRUE, 15625}, {0x000431BE, TRUE, 15625}, {0x0014F8B6, TRUE, 15625}, {0x00218DF0, TRUE, 15625}, {0x0027D029, TRUE, 15625}, {0x00333334, TRUE, 15625}},
};

/*
 * Prototypes
 */
int bcm_esw_ces_service_rclock_config_get(
    int unit, 
    bcm_ces_service_t ces_service, 
    bcm_ces_rclock_config_t *config);
int bcm_esw_ces_service_rclock_config_set(
    int unit, 
    bcm_ces_service_t ces_service, 
    bcm_ces_rclock_config_t *config);
int bcm_esw_ces_service_destroy_all(int unit);
int bcm_esw_ces_service_enable_set(int unit, 
				   bcm_ces_service_t ces_service, 
				   int enable);
int bcm_esw_ces_service_free_tdm(int unit, 
				 bcm_ces_service_t ces_service);
int bcm_esw_ces_services_cclk_config_set(int unit, 
					 bcm_ces_cclk_config_t *config);

/*
 * Define:
 *	CES_LOCK/CES_UNLOCK
 * Purpose:
 *	Serialization Macros for access to control structure.
 */

#define CES_LOCK(unit) \
        sal_mutex_take(ces_ctrl->lock, sal_mutex_FOREVER)

#define CES_UNLOCK(unit) \
        sal_mutex_give(ces_ctrl->lock)

#define CES_CHECK_INIT(unit) \
        if (ces_ctrl == NULL) { \
            return(BCM_E_INIT); \
        }


#ifdef INCLUDE_BCM_API_XLATE_PORT
#define XLATE_P2A(_u,_p) BCM_API_XLATE_PORT_P2A(_u,&_p) == 0
#else
#define XLATE_P2A(_u,_p) 1
#endif

#define DPORT_BCM_PBMP_ITER(_u,_pbm,_dport,_p) \
        for ((_dport) = 0, (_p) = -1; (_dport) < SOC_DPORT_MAX; (_dport)++) \
                if (((_p) = soc_dport_to_port(_u,_dport)) >= 0 && \
                    XLATE_P2A(_u,_p) && SOC_PBMP_MEMBER((_pbm),(_p)))

/**
 * Function:
 *      bcm_ces_service_config_t_init()
 * Purpose:
 *      Initialize service control structure
 * Parameters:
 * Returns:
 *      Result
 * Notes:
 */
void bcm_ces_service_config_t_init(bcm_ces_service_config_t *config) {

    if (config != NULL) {
        sal_memset(config, 0, sizeof(bcm_ces_service_config_t));
    }
    return;
}

/**
 * Function:
 *      bcm_ces_cclk_config_t_init()
 * Purpose:
 *      Initialize common clock control structure
 * Parameters:
 * Returns:
 *      Result
 * Notes:
 */
void bcm_ces_cclk_config_t_init(bcm_ces_cclk_config_t *config) {

    if (config != NULL) {
        sal_memset(config, 0, sizeof(bcm_ces_cclk_config_t));
    }
    return;
}


/**
 * Function:
 *      bcm_ces_rclock_config_t_init()
 * Purpose:
 *      Initialize rclock control structure
 * Parameters:
 * Returns:
 *      Result
 * Notes:
 */
void bcm_ces_rclock_config_t_init(bcm_ces_rclock_config_t *config) {

    if (config != NULL) {
        sal_memset(config, 0, sizeof(bcm_ces_rclock_config_t));
    }
    return;
}


/**
 * Function:
 *      bcm_esw_ces_enable_mii_port()
 * Purpose:
 *      Used to switch ge0 to CES mode
 * Parameters:
 * Returns:
 *      Result
 * Notes:
 */
#define CES_LINK 1
int bcm_esw_ces_enable_mii_port(int unit, uint32 mii_port, int enable) {
    extern int bcm_esw_port_speed_set(int,bcm_port_t,int);
#ifdef CES_LINK
extern int bcm_port_config_get(
    int unit, 
    bcm_port_config_t *config);
    pbmp_t      pbm_sw, pbm_hw, pbm_none, pbm_force;
    pbmp_t	pbm_temp;
    pbmp_t	pbm_sw_pre,pbm_hw_pre,pbm_none_pre;
    int us, rv;
    bcm_port_config_t   pcfg;
    soc_port_t		port, dport;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    if (bcm_port_config_get(unit, &pcfg) != BCM_E_NONE) {
	return BCM_E_INTERNAL;
    }

    /*
     * First get current linkscan state.  (us == 0 if disabled).
     */

    if ((rv = bcm_linkscan_enable_get(unit, &us)) < 0) {
	return BCM_E_INTERNAL;
    }

    /*
     * Get linkscan port bitmap
     */
    BCM_PBMP_CLEAR(pbm_sw);
    BCM_PBMP_CLEAR(pbm_hw);
    BCM_PBMP_CLEAR(pbm_force);

    DPORT_BCM_PBMP_ITER(unit, pcfg.port, dport, port) {
	int		mode;

	if ((rv = bcm_linkscan_mode_get(unit, port, &mode)) < 0) {
	    soc_cm_print("failed to get mode for port %d\n", port);
	} else {
	    switch (mode) {
	    case BCM_LINKSCAN_MODE_SW:
		BCM_PBMP_PORT_ADD(pbm_sw, port);
		break;
	    case BCM_LINKSCAN_MODE_HW:
		BCM_PBMP_PORT_ADD(pbm_hw, port);
		break;
	    default:
		break;
	    }
	}
    }

    BCM_PBMP_ASSIGN(pbm_sw_pre, pbm_sw);
    BCM_PBMP_ASSIGN(pbm_hw_pre, pbm_hw);
    BCM_PBMP_REMOVE(pbm_sw_pre, PBMP_HG_SUBPORT_ALL(unit));
    BCM_PBMP_REMOVE(pbm_hw_pre, PBMP_HG_SUBPORT_ALL(unit));

    /*
     * Remove ge0 from the bitmap
     */
    BCM_PBMP_CLEAR(pbm_temp);
    BCM_PBMP_PORT_SET(pbm_temp, mii_port);
    BCM_PBMP_REMOVE(pbm_sw, pbm_temp);

    /*
     * Turn off linkscan
     */
    if ((rv = bcm_linkscan_enable_set(unit, 0)) < 0) {
	return BCM_E_INTERNAL;
    }

    /*
     * Force link up
     */
    _bcm_esw_link_force(unit, mii_port, TRUE, TRUE); 

    /*
     *
     */
    BCM_PBMP_AND(pbm_sw, pcfg.port);
    BCM_PBMP_AND(pbm_hw, pcfg.port);
    BCM_PBMP_ASSIGN(pbm_none, pcfg.port);
    BCM_PBMP_REMOVE(pbm_sw, PBMP_HG_SUBPORT_ALL(unit));
    BCM_PBMP_REMOVE(pbm_hw, PBMP_HG_SUBPORT_ALL(unit));
    BCM_PBMP_REMOVE(pbm_none, PBMP_HG_SUBPORT_ALL(unit));

    BCM_PBMP_ASSIGN(pbm_temp, pbm_sw);
    BCM_PBMP_OR(pbm_temp, pbm_hw);
    BCM_PBMP_XOR(pbm_none, pbm_temp);
    BCM_PBMP_AND(pbm_force, pcfg.port);

    BCM_PBMP_ASSIGN(pbm_temp, pbm_sw);
    BCM_PBMP_AND(pbm_temp, pbm_hw);

    if ((rv = bcm_linkscan_mode_set_pbm(unit, pbm_sw,
					BCM_LINKSCAN_MODE_SW)) < 0) {
	return BCM_E_INTERNAL;
    }


    if ((rv = bcm_linkscan_mode_set_pbm(unit, pbm_hw,
					BCM_LINKSCAN_MODE_HW)) < 0) {
	return BCM_E_INTERNAL;
    }

    /* Only set the port to mode_none state if it is not previously in this mode */
    BCM_PBMP_ASSIGN(pbm_none_pre, pcfg.port);
    BCM_PBMP_ASSIGN(pbm_temp, pbm_sw_pre);
    BCM_PBMP_OR(pbm_temp, pbm_hw_pre);
    BCM_PBMP_XOR(pbm_none_pre, pbm_temp);
    BCM_PBMP_XOR(pbm_none_pre, pbm_none); 
    BCM_PBMP_AND(pbm_none, pbm_none_pre);  /* the one changed from 0 to 1 */
    
    if ((rv = bcm_linkscan_mode_set_pbm(unit, pbm_none,   
                                          BCM_LINKSCAN_MODE_NONE)) < 0) {   
	return BCM_E_INTERNAL;
    }   

    /*
     * Turn on linkscan
     */
    if ((rv = bcm_linkscan_enable_set(unit, us)) < 0) {
	return BCM_E_INTERNAL;
    }
  
#endif

    
    /*
     * The CES MII can only link with GE0. This is acheived by setting the speed
     * of GE0 to 100MBps
     */
    if (enable)
	return bcm_esw_port_speed_set(unit, mii_port, 100);
    else
	return bcm_esw_port_speed_set(unit, mii_port, 1000);
}


/**
 * Function:
 *      bcm_esw_ces_crm_msg_send
 * Purpose:
 *      Send message to CRM
 * Parameters:
 * Returns:
 *      Result
 * Notes:
 */
int bcm_esw_ces_crm_dma_msg_send_receive(int unit, uint8 t_subclass, uint8 *data, 
					 int size, uint8 r_subclass, uint16 *r_len) {
    mos_msg_data_t tx;         /* 64-bit message */
    mos_msg_data_t rx;       /* 64-bit reply */
    uint8 *dma_buffer;
    int dma_buffer_len;
    int ret = BCM_E_NONE;
    int res;
    uint32 uc_rv;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    uint32 s_data = 0;

    /*
     * Build the message
     */
    sal_memset(&tx, 0, sizeof(tx));
    sal_memset(&rx, 0, sizeof(rx));
    tx.s.mclass    = MOS_MSG_CLASS_CES; /* CES message class */
    tx.s.subclass  = t_subclass; /* CES subclass */
    tx.s.len       = bcm_htons(size);

    /* 
     * The contents of this message will be a buffer pointer 
     */
    dma_buffer = ces_ctrl->dma_buffer;
    dma_buffer_len = ces_ctrl->dma_buffer_len;

    switch(t_subclass) {
    case MOS_MSG_SUBCLASS_CES_CRM_INIT:
	bcm_ces_crm_init_msg_pack(dma_buffer, (bcm_ces_crm_init_msg_t*)data);
	break;

    case MOS_MSG_SUBCLASS_CES_CRM_CONFIG:
	bcm_ces_crm_cclk_config_msg_pack(dma_buffer, (bcm_ces_crm_cclk_config_msg_t*)data);
	break;

    case MOS_MSG_SUBCLASS_CES_CRM_RCLOCK_CONFIG:
	bcm_ces_crm_rclock_config_msg_pack(dma_buffer, (bcm_ces_crm_rclock_config_msg_t*)data);
	break;

    case MOS_MSG_SUBCLASS_CES_CRM_STATUS:
	break;

    default:
	soc_cm_print("%s: Unhandled message type:%d\n", __func__, t_subclass);
	return BCM_E_INTERNAL;
	break;
    }

    if (MOS_MSG_DMA_MSG(t_subclass) ||
        MOS_MSG_DMA_MSG(r_subclass)) {
        tx.s.data = bcm_htonl(soc_cm_l2p(unit, dma_buffer));
    } else {
        tx.s.data = bcm_htonl(s_data);
    }

    if (MOS_MSG_DMA_MSG(t_subclass)) {
	soc_cm_sflush(unit, dma_buffer, size);
    }

    if (MOS_MSG_DMA_MSG(r_subclass)) {
	soc_cm_sinval(unit, dma_buffer, dma_buffer_len);
    }
    
    /* 
     * Send the msg then wait up to 1 sec for a reply 
     */
    res = soc_cmic_uc_msg_send_receive(unit, ces_ctrl->crm_core, &tx, &rx, 1000000);

    if (res != SOC_E_NONE ||
	rx.s.mclass != MOS_MSG_CLASS_CES ||
	rx.s.subclass != r_subclass) {
	soc_cm_print("%s: Received message type error or missmatch, mclass:%d  subclass:%d\n", 
		     __func__, rx.s.mclass, rx.s.subclass);
	ret = BCM_E_INTERNAL;
    }


    uc_rv = bcm_ntohl(rx.s.data);

    switch(uc_rv) {
    case UC_CES_E_NONE:
        ret = BCM_E_NONE;
        break;
    case UC_CES_E_INTERNAL:
        ret = BCM_E_INTERNAL;
        break;
    case UC_CES_E_MEMORY:
        ret = BCM_E_MEMORY;
        break;
    case UC_CES_E_PARAM:
        ret = BCM_E_PARAM;
        break;
    case UC_CES_E_RESOURCE:
        ret = BCM_E_RESOURCE;
        break;
    case UC_CES_E_EXISTS:
        ret = BCM_E_EXISTS;
        break;
    case UC_CES_E_NOT_FOUND:
        ret = BCM_E_NOT_FOUND;
        break;
    case UC_CES_E_INIT:
        ret = BCM_E_INIT;
        break;
    default:
        ret = BCM_E_INTERNAL;
        break;
    }
        
    *r_len = bcm_ntohs(rx.s.len);

    return ret;
}

#define BCM_CES_CRM 1
#if defined(BCM_CES_CRM)
/**
 * Function:
 *      bcm_esw_ces_crm_init
 * Purpose:
 *      Identify CRM location and say hello
 * Parameters:
 * Returns:
 *      Result
 * Notes:
 */
static int bcm_esw_ces_crm_init(int unit, uint8 flags) {
    bcm_ces_crm_init_msg_t init_msg;
    uint16 reply_len;
    int c, found=0;
    int result = BCM_E_NONE;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    /*
     * Although this message is small I'm using the DMA transfer
     * so that if the message grows there will be little to change.
     */
    sal_memset(&init_msg, 0, sizeof(bcm_ces_crm_init_msg_t));
    init_msg.flags = flags;

    /* Determine which BTE is running CES.  Try uC to see if the
     * applications start there */
    for (c=0; c < _CES_NUM_CMICM; c++) {
        result = soc_cmic_uc_appl_init(unit, c, MOS_MSG_CLASS_CES,
                                       _CES_UC_MSG_TIMEOUT_USECS,
                                       CES_SDK_VERSION, 
				       CES_UC_MIN_VERSION);
        if (SOC_E_NONE == result) {
            /* BFD started successfully */
	    ces_ctrl->crm_core = c;
            found = 1;
            break;
        }
    }

    if(found) {
	/*uC found, send init message*/
	result = bcm_esw_ces_crm_dma_msg_send_receive(unit,
						      MOS_MSG_SUBCLASS_CES_CRM_INIT, 
						      (uint8*)&init_msg, 
						      sizeof(bcm_ces_crm_init_msg_t),
						      MOS_MSG_SUBCLASS_CES_CRM_INIT_REPLY,
						      &reply_len); 
	if (result != SOC_E_NONE) {
	    soc_cm_print("INIT, failed result: 0x%x\n", result);
	    result =  BCM_E_INTERNAL;
	}
    } else {
       result =  BCM_E_INTERNAL;
    }

    return result;
}
#endif


/**
 * Function:
 *      bcm_esw_ces_pmi_cb
 * Purpose:
 *      CES PM counter callback. Called once per second
 * Parameters:
 * Returns:
 *      Result
 * Notes:
 */
AgResult bcm_esw_ces_pmi_cb(void) {
    int unit = 0; 
    AgResult n_ret;
    AgNdMsgPmIngress ndPmIngress;
    AgNdMsgPmEgress  ndPmEgress;
    bcm_ces_service_record_t *record;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    int i, j;

    /* SOC_DEBUG_PRINT((DK_VERBOSE,"%s:\n", __func__)); */
    if (!BCM_CES_IS_CONFIGURED(ces_ctrl))
	return AG_S_OK;

    
    if (ces_ctrl->attached == FALSE)
	return AG_S_OK;

    /*
     * Loop through all service records.
     */
    for (i = 0;i < BCM_CES_CIRCUIT_IDX_MAX;i++) {
	if (ces_ctrl->bcm_ces_service_records[i] == NULL)
	    continue;

	record = ces_ctrl->bcm_ces_service_records[i];

	if (!BCM_CES_IS_ENABLED(&(record->config)))
	    continue;

        if (ces_ctrl->attached == FALSE) {
            soc_cm_print("%s: Not attached, aborting\n", __func__);
            return AG_S_OK;
        }

	CES_LOCK(unit);
	/*
	 * Ingress stats
	 */
	sal_memset(&ndPmIngress, 0, sizeof(ndPmIngress));
	ndPmIngress.n_channel_id = i;
	n_ret = ag_nd_device_read(ces_ctrl->ndHandle, AG_ND_OPCODE_PM_INGRESS, &ndPmIngress);
	if (!AG_SUCCEEDED(n_ret)) {
	    CES_UNLOCK(unit);
	    continue;
	}

	record->pm_counts.transmitted_bytes += ndPmIngress.n_tpe_pwe_out_byte_counter;
	record->pm_counts.transmitted_packets += ndPmIngress.n_tpe_pwe_out_packet_counter;

	/*
	 * Egress stats
	 */
	sal_memset(&ndPmEgress, 0, sizeof(ndPmEgress));
	ndPmEgress.n_channel_id = i;
	n_ret = ag_nd_device_read(ces_ctrl->ndHandle, AG_ND_OPCODE_PM_EGRESS, &ndPmEgress);
	if (!AG_SUCCEEDED(n_ret)) {
	    CES_UNLOCK(unit);
	    continue;
	}

	if (record->pm_counts.jbf_depth_min == 0 ||
	    ndPmEgress.n_jbf_depth_min < record->pm_counts.jbf_depth_min)
	    record->pm_counts.jbf_depth_min =  ndPmEgress.n_jbf_depth_min;

	if (ndPmEgress.n_jbf_depth_max > record->pm_counts.jbf_depth_max)
	    record->pm_counts.jbf_depth_max =  ndPmEgress.n_jbf_depth_max;

	record->pm_counts.jbf_underruns += ndPmEgress.n_jbf_underruns;
	record->pm_counts.jbf_missing_packets  += ndPmEgress.n_jbf_missing_packets;
	record->pm_counts.jbf_dropped_ooo_packets += ndPmEgress.n_jbf_dropped_ooo_packets;
	record->pm_counts.jbf_reordered_ooo_packets += ndPmEgress.n_jbf_reordered_ooo_packets;
	record->pm_counts.jbf_bad_length_packets += ndPmEgress.n_jbf_bad_length_packets;
	record->pm_counts.received_bytes     += ndPmEgress.n_rpc_pwe_in_bytes;
	record->pm_counts.received_packets   += ndPmEgress.n_rpc_pwe_in_packets;

	for (j = 0;j < AG_ND_RPC_CCNT_MAX;j++) {
	    record->pm_counts.rpc_channel_specific[j] += ndPmEgress.a_rpc_channel_specific[j];
	}
	CES_UNLOCK(unit);
    }

    return AG_S_OK;
}



/**
 * Function:
 *      bcm_esw_ces_pbi_cb
 * Purpose:
 *      CES PBF overflow callback
 * Parameters:
 *      n_channel_id - (IN) Channel ID
 *      n_user_data  - (IN) User data
 * Returns:
 *      Result
 * Notes:
 */
AgResult bcm_esw_ces_pbi_cb(AG_U32 n_channel_id, AG_U32 n_user_data) {
    int unit = (int)n_user_data; 
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_service_record_t *record;
    int n_ret;

    if (!BCM_CES_IS_CONFIGURED(ces_ctrl))
	return AG_S_OK;


    if (ces_ctrl->attached == FALSE)
	return AG_S_OK;

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, n_channel_id, &record);
    if (n_ret != BCM_E_NONE)
	return AG_S_OK;

    if (BCM_CES_IS_FREE(&record->config))
	return AG_S_OK;

    SOC_DEBUG_PRINT((DK_WARN,"%s: PBF overflow on service:%lu\n", 
		     __func__,
		     n_channel_id));

    if (ces_ctrl->pboCallback != NULL)
	n_ret = ces_ctrl->pboCallback(unit,
				      bcmCesEventPacketBufferOverflow, 
				      n_channel_id,
				      0,
				      ces_ctrl->pboUserData);  
    return AG_S_OK;
}

/**
 * Function:
 *      bcm_esw_ces_cwi_cb
 * Purpose:
 *      CES control word change callback
 * Parameters:
 *      n_channel_id - (IN) Channel ID
 *      n_user_data  - (IN) User data
 *      n_cw         - (IN) Control word
 * Returns:
 *      Result
 * Notes:
 */
AgResult bcm_esw_ces_cwi_cb(AG_U32 n_channel_id, AG_U32 n_user_data, AG_U16 n_cw) {
    int unit = (int)n_user_data; 
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_service_record_t *record;
    int n_ret;

    if (!BCM_CES_IS_CONFIGURED(ces_ctrl))
	return AG_S_OK;

    if (ces_ctrl->attached == FALSE)
	return AG_S_OK;

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, n_channel_id, &record);
    if (n_ret != BCM_E_NONE)
	return AG_S_OK;

    if (BCM_CES_IS_FREE(&record->config))
	return AG_S_OK;

    /*
     * Save value 
     */
    record->rx_control_word = n_cw;

    SOC_DEBUG_PRINT((DK_VERBOSE,"%s: Service:%lu CES control word bits changed to:0x%04x\n", 
		     __func__,
		     n_channel_id,
		     n_cw));

    if (ces_ctrl->cwcCallback != NULL)
	n_ret = ces_ctrl->cwcCallback(unit,
				      bcmCesEventControlWordChange, 
				      n_channel_id,
				      (uint32)n_cw,
				      ces_ctrl->cwcUserData);  

    return AG_S_OK;
}

/**
 * Function:
 *      bcm_esw_ces_psi_cb
 * Purpose:
 *      CES Packet sync status change
 * Parameters:
 *      n_channel_if - (IN) Channel ID
 *      n_user_data  - (IN) User data
 *      n_sync       - (IN) Current sync state
 * Returns:
 *      Result
 * Notes:
 */
AgResult bcm_esw_ces_psi_cb(AG_U32 n_channel_id, AG_U32 n_user_data, AgNdSyncType n_sync) {
    int unit = (int)n_user_data; 
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_service_record_t *record;
    int n_ret;

    if (!BCM_CES_IS_CONFIGURED(ces_ctrl))
	return AG_S_OK;


    if (ces_ctrl->attached == FALSE)
	return AG_S_OK;

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, n_channel_id, &record);
    if (n_ret != BCM_E_NONE)
	return AG_S_OK;

    if (BCM_CES_IS_FREE(&record->config))
	return AG_S_OK;

    SOC_DEBUG_PRINT((DK_VERBOSE,"%s: Service:%lu sync status changed to:%s\n", 
		     __func__,
		     n_channel_id,
		     (n_sync == 0 ? "LOPS":"AOPS")));
    if (!BCM_CES_IS_CONFIGURED(ces_ctrl))
	return AG_S_OK;

    if (ces_ctrl->pscCallback != NULL)
	n_ret = ces_ctrl->pscCallback(unit,
				      bcmCesEventPacketSyncChange, 
				      n_channel_id,
				      (uint32)n_sync,
				      ces_ctrl->pscUserData);  
    return AG_S_OK;
}


/**
 * Function:
 *      bcm_esw_ces_tpi_cb
 * Purpose:
 *      CES FLL sampling period complete status change
 * Parameters:
 *      n_channel_if - (IN) Channel ID
 *      n_user_data  - (IN) User data
 *      n_sync       - (IN) Current sync state
 * Returns:
 *      Result
 * Notes:
 */
AgResult bcm_esw_ces_tpi_cb(AG_U32 n_channel_id, AG_U32 n_user_data) {
    int unit = (int)n_user_data; 
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_service_record_t *record;
    int n_ret;

    if (!BCM_CES_IS_CONFIGURED(ces_ctrl))
	return AG_S_OK;


    if (ces_ctrl->attached == FALSE)
	return AG_S_OK;

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, n_channel_id, &record);
    if (n_ret != BCM_E_NONE)
	return AG_S_OK;

    if (BCM_CES_IS_FREE(&record->config))
	return AG_S_OK;

    SOC_DEBUG_PRINT((DK_VERBOSE,"%s: Service:%lu FLL sample complete\n", 
		     __func__,
		     n_channel_id));

    if (!BCM_CES_IS_CONFIGURED(ces_ctrl))
	return AG_S_OK;

    if (ces_ctrl->fscCallback != NULL)
	n_ret = ces_ctrl->fscCallback(unit,
				      bcmCesEventFLLSamplingComplete, 
				      n_channel_id,
				      0,
				      ces_ctrl->fscUserData);  
    return AG_S_OK;
}


/**
 * Function:
 *      bcm_ces_port_to_circuit_id
 * Purpose:
 *      Converts platform port number to zero based circuit_id
 * Parameters:
 *      unit - (IN) Unit number.
 *      port
 * Returns:
 *      circuit id
 * Notes:
 */
int bcm_ces_port_to_circuit_id(int unit, bcm_port_t port) {
    if (SOC_IS_KATANA(unit))
	return (port - BCM_CES_TDM_PORT_BASE_KATANA);
    else
	return port;
}

/**
 * Function:
 *      bcm_ces_circuit_id_to_port
 * Purpose:
 *      Converts zero based circuit_id to platform port number
 * Parameters:
 *      unit - (IN) Unit number.
 *      port
 * Returns:
 *      circuit id
 * Notes:
 */
bcm_port_t bcm_ces_circuit_id_to_port(int unit, int circuit_id) {
    if (SOC_IS_KATANA(unit))
	return (circuit_id + BCM_CES_TDM_PORT_BASE_KATANA);
    else
	return circuit_id;
}

/**
 * Function:
 *      bcm_ces_service_get_service
 * Purpose:
 *      Find free channel
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service
 * Returns:
 *      circuit id
 * Notes:
 */
int bcm_ces_service_get_service(int unit, 
				bcm_ces_service_t *ces_service,
				bcm_ces_service_record_t **record,
                                int use_id) {
    int i;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    if (use_id == 0) {
	/*
	 * Find first unused circuit.
	 */
	for (i = 0;i < BCM_CES_CIRCUIT_IDX_MAX;i++) {
	    if (ces_ctrl->bcm_ces_service_records[i] == NULL) {
		return BCM_E_INTERNAL;
	    } else if (BCM_CES_IS_FREE(&ces_ctrl->bcm_ces_service_records[i]->config)) {
		*record = ces_ctrl->bcm_ces_service_records[i];

		/*
		 * Mark as used.
		 */
		BCM_CES_CLR_ALL(&(*record)->config);
		*ces_service = (*record)->ces_service;
		return BCM_E_NONE;
	    }
	}
    } else {
	/*
	 * Check that service number is valid
	 */
	if (!bcm_ces_service_is_valid(*ces_service)) {
	    *record = NULL;
	    return BCM_E_BADID;
	}

	if (ces_ctrl->bcm_ces_service_records[*ces_service] == NULL) {
	    return BCM_E_INTERNAL;
	} else if (BCM_CES_IS_FREE(&ces_ctrl->bcm_ces_service_records[*ces_service]->config)) {
	    *record = ces_ctrl->bcm_ces_service_records[*ces_service];

	    /*
	     * Mark as used.
	     */
	    BCM_CES_CLR_ALL(&(*record)->config);
	    return BCM_E_NONE;
	} else {
	    return BCM_E_UNAVAIL;
	}
    }

    /*
     * All services taken
     */
    return BCM_E_FULL;
}

/**
 * Function:
 *      bcm_ces_service_init_record
 * Purpose:
 *      Initialize service record with default values
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service
 *      record - (IN) allocated record
 * Returns:
 *      BCM_E_
 * Notes:
 */
int bcm_ces_service_init_record(int unit, 
				bcm_ces_service_t ces_service)
{
    bcm_ces_service_record_t *record;
    int n_ret;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    int i;


    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (record == NULL)
	return BCM_E_MEMORY;

    /*
     * Mark service record as free
     */
    sal_memset(record, 0, sizeof(bcm_ces_service_record_t));
    BCM_CES_CLR_ALL(&record->config);
    BCM_CES_SET_FREE(&record->config);

    record->ces_service = ces_service;

    /*
     * Set default values
     *
     * Channel maps are initially empty so there's nothing to
     * do there.
     */

    /*
     * Ingress channel config
     */
    record->config.ingress_channel_config.dba = FALSE;
    record->config.ingress_channel_config.auto_r_bit = 0;
    record->config.ingress_channel_config.mef_len_support = 0;
    record->config.ingress_channel_config.pbf_size = BCM_CES_PACKET_BUFFER_SIZE;

    /*
     * Egress channel config
     */
    record->config.egress_channel_config.packet_sync_selector = 0;
    record->config.egress_channel_config.rtp_exists = FALSE;
    record->config.egress_channel_config.frames_per_packet = BCM_CES_FRAMES_PER_PACKET;
    record->config.egress_channel_config.jbf_ring_size = BCM_CES_JBF_RING_SIZE;
    record->config.egress_channel_config.jbf_win_size = BCM_CES_JBF_WINDOW_SIZE;
    record->config.egress_channel_config.jbf_bop = BCM_CES_JBF_BREAK_OUT_POINT;

    /*
     * Header
     */
    sal_memcpy(record->config.header.eth.source, ces_ctrl->mac, sizeof(sal_mac_addr_t));
    sal_memcpy(record->config.header.eth.destination, ces_ctrl->mac, sizeof(sal_mac_addr_t));

    record->config.header.encapsulation = bcmCesEncapsulationEth;
    record->config.header.ipv4.tos = BCM_CES_IPV4_TOS;
    record->config.header.ipv4.ttl = BCM_CES_IPV4_TTL;
    record->config.header.ipv4.source      = ces_ctrl->ipv4_addr;
    record->config.header.ipv4.destination = ces_ctrl->ipv4_addr;
    record->config.header.vc_label = 0xab00 + ces_service;
    record->config.header.udp.destination = BCM_CES_UDP_DESTINATION; 
    record->config.header.udp.source = BCM_CES_UDP_SOURCE;
    record->config.header.ip_version = BCM_CES_IP_VERSION;
    record->config.header.vlan_count = BCM_CES_VLAN_COUNT;
    record->config.header.vlan[0].priority = 1;
    record->config.header.vlan[0].vid =0x05;
    record->config.header.mpls_count = BCM_CES_MPLS_COUNT;
    record->config.header.rtp_exists = FALSE;
    record->config.header.udp_chksum = 0;

    /*
     * Strict data for ETH encap
     */
    record->config.strict_data.encapsulation = record->config.header.encapsulation;
    sal_memset(&record->config.strict_data, 0, sizeof(bcm_ces_packet_header_t));
    memcpy(record->config.strict_data.eth.destination, record->config.header.eth.source, 
           sizeof(sal_mac_addr_t));

    record->config.strict_data.vlan_count = record->config.header.vlan_count;

    for (i = 0;i < record->config.header.vlan_count;i++) {
	record->config.strict_data.vlan[i].vid = record->config.header.vlan[i].vid;
    }


    /*
     * Config
     */
    record->config.encapsulation = bcmCesEncapsulationEth;

    return BCM_E_NONE;
}


/**
 * Function:
 *      bcm_ces_service_allocate_record
 * Purpose:
 *      Allocate memory for service record
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service
 *      record - (OUT) allocated record
 * Returns:
 *      BCM_E_
 * Notes:
 */
int bcm_ces_service_allocate_record(int unit, 
				    bcm_ces_service_t ces_service)
{
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_service_record_t *record;

    record = (bcm_ces_service_record_t*)sal_alloc(sizeof(bcm_ces_service_record_t),"CES Service record");
    ces_ctrl->bcm_ces_service_records[ces_service] = record;
    
    if (record == NULL)
	return BCM_E_MEMORY;

    /*
     * Init record
     */
    bcm_ces_service_init_record(unit, ces_service);

    return BCM_E_NONE;
}


 
/**
 * Function:
 *      bcm_ces_service_free_record
 * Purpose:
 *      Free memory for circuit record
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service
 *
 * Returns:
 *      BCM_E_
 * Notes:
 */
int bcm_ces_service_free_record(int unit, 
				bcm_ces_service_t ces_service)
{
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_service_record_t *record = ces_ctrl->bcm_ces_service_records[ces_service];

    if (record != NULL)
    {
	/*
	 * Disable
	 */
	if (BCM_CES_IS_ENABLED(&record->config)) {
	    bcm_esw_ces_service_enable_set(unit, ces_service, 0);
	}

	/*
	 * Free the TDM resources
	 */
	bcm_esw_ces_service_free_tdm(unit, ces_service);

	/*
	 * Mark service as free
	 */
	BCM_CES_CLR_ALL(&record->config);
	BCM_CES_SET_FREE(&record->config);
	return BCM_E_NONE;
    }

    return BCM_E_INTERNAL;
}

/**
 * Function:
 *      bcm_ces_service_find
 * Purpose:
 *      Find service record form service id
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service
 *
 * Returns:
 *      BCM_E_
 * Notes:
 */
int bcm_ces_service_find(int unit, 
			 bcm_ces_service_t ces_service,
			 bcm_ces_service_record_t **record)
{
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    /*
     * Check that service number is valid
     */
    if (!bcm_ces_service_is_valid(ces_service)) {
	*record = NULL;
	return BCM_E_BADID;
    }

    /*
     * find service
     */
    if (ces_ctrl->bcm_ces_service_records[ces_service] != NULL) {
	*record = ces_ctrl->bcm_ces_service_records[ces_service]; 
	return BCM_E_NONE;
    }

    *record = NULL;
    return BCM_E_INTERNAL;
}

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_0

/**
 * Function:
 *      bcm_esw_ces_warm_boot
 * Purpose:
 *      Warm boot CES module
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE      Operation completed successfully
 *      BCM_E_UNAVAIL   The CES driver failed to initialize correctly
 *      BCM_E_MEMORY    Insufficient memory
 *      BCM_E_INTERNAL  An internal error occurred
 * Notes:
 *      Warm boots the CES device driver and allocates service control structures 
 */
int _bcm_esw_ces_reinit(int unit) {
    soc_scache_handle_t scache_handle;
    uint8 *ces_scache_ptr;

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_CES, 0);

    BCM_IF_ERROR_RETURN
        (_bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 0, &ces_scache_ptr, 
                                 BCM_WB_DEFAULT_VERSION, NULL));

    /*
     * Call BATM init indicating warm boot and inform about valid services.
     */

    /*
     * Recover config from the device.
     */
    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_esw_ces_sync
 * Purpose:
 *      Sync warm boot data 
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE      Operation completed successfully
 *      BCM_E_UNAVAIL   The CES driver failed to initialize correctly
 *      BCM_E_MEMORY    Insufficient memory
 *      BCM_E_INTERNAL  An internal error occurred
 * Notes:
 *      Sync method. Called automatically from switch code.
 */
int _bcm_esw_ces_sync(int unit) {
    soc_scache_handle_t scache_handle;
    uint8 *ces_scache_ptr;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    int i;
    bcm_ces_service_record_t *record;
    bcm_ces_warm_boot_t *wb_record;

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_CES, 0);

    BCM_IF_ERROR_RETURN
        (_bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 0, &ces_scache_ptr, 
                                 BCM_WB_DEFAULT_VERSION, NULL));

    /*
     * Write configured CES services to warm boot cache
     */
    sal_memset(ces_scache_ptr, 0, sizeof(bcm_ces_warm_boot_t));
    wb_record = (bcm_ces_warm_boot_t*)ces_scache_ptr;

    for (i = 0;i < BCM_CES_CIRCUIT_IDX_MAX;i++) {
	if (ces_ctrl->bcm_ces_service_records[i] != NULL) {
	    record = ces_ctrl->bcm_ces_service_records[i]; 
	    wb_record->ces_service = record->ces_service;
	    wb_record++;
	}
    }

    return BCM_E_NONE;
}

#endif /* WARM_BOOT */


/**
 * Function:
 *      bcm_ces_init
 * Purpose:
 *      Initialize CES module
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE      Operation completed successfully
 *      BCM_E_UNAVAIL   The CES driver failed to initialize correctly
 *      BCM_E_MEMORY    Insufficient memory
 *      BCM_E_INTERNAL  An internal error occurred
 * Notes:
 *      Initializes the CES device driver and allocates service control structures 
 */
int bcm_esw_ces_init(int unit)
{
    AgNdMsgOpen p_msg;
    uint32 addr;
    AgResult n_res;
    AgResult n_ret;
    bcm_ces_service_global_config_t **ces_ctrl = (bcm_ces_service_global_config_t **)&(SOC_CONTROL(unit)->ces_ctrl);
    bcm_ces_service_global_config_t *config;
    char *mac_str;
    char *ip_str;
    int ret;
    int i;
    uint32 coreid;
    char *str;
    uint32 mii_port;
#ifdef BCM_WARM_BOOT_SUPPORT
    soc_scache_handle_t scache_handle;
    uint32              ces_scache_size;
    uint8              *ces_scache_ptr;
#endif

    /*
     * Initialize CES16Core
     */
    if ((n_res = ag_nd_module_create()) != AG_S_OK)
    {
	if (n_res == AG_E_ND_MODULE_CREATED)
	    return BCM_E_NONE;

	return BCM_E_UNAVAIL;
    }

    /*
     * Get the MII port
     */
    mii_port = soc_property_get(unit, spn_CES_MII_PORT_NUMBER, 1);

    
    if (mii_port != 1) {
	mii_port = 1; 
    }


    /*
     * Check that CES is not already initialized
     */
    if (*ces_ctrl != NULL) {
	return BCM_E_NONE;
    }

    /*
     * Initialize control structs
     */
    *ces_ctrl = (bcm_ces_service_global_config_t*)sal_alloc(sizeof(bcm_ces_service_global_config_t),"CES Service");
    config = *ces_ctrl;

    if (*ces_ctrl == NULL)
	return BCM_E_MEMORY;

    sal_memset(*ces_ctrl, 0, sizeof(bcm_ces_service_global_config_t));

    config->lock = sal_mutex_create("bcm_ces_LOCK");
    if (config->lock == NULL) {
	sal_free(config);
        return BCM_E_MEMORY;
    }


    /*
     * Allocate DMA buffers
     *
     * DMA buffer will be used to send and receive 'long' messages
     * between SDK Host and uController (BTE).
     */
    config->dma_buffer_len = CES_CONTROL_MESSAGE_MAX_SIZE;
    config->dma_buffer = soc_cm_salloc(unit, config->dma_buffer_len,
                                         "CES Tx DMA buffer");
    if (!config->dma_buffer) {
        return BCM_E_MEMORY;
    }
    sal_memset(config->dma_buffer, 0, config->dma_buffer_len);


    config->ces_mii_port = mii_port;


    /*
     * Allocate and initialize service records.
     */
    for (i = 0;i < BCM_CES_CIRCUIT_IDX_MAX;i++)
    {
	if ((ret = bcm_ces_service_allocate_record(unit, i)) != BCM_E_NONE)
	{
	    return ret;
	}
    }

    /*
     * Base address
     */
    addr = soc_reg_addr(unit, COREIDr, REG_PORT_ANY, 0);
    SOC_DEBUG_PRINT((DK_VERBOSE, "CES base address:0x%08x\n", addr));


#ifdef PLISIM
    
    ret = soc_reg32_write(unit, addr, BCM_CES_COREID);
    if (ret < 0) {
        return BCM_E_INTERNAL;
    }
#endif

    /*
     * Read the CES version. If this does not give the expected value
     * then we are done. (The BATM driver will not initialize correctly)
     */
    ret = soc_reg32_read(unit, addr, &coreid);
    if (ret < 0) {
        return BCM_E_INTERNAL;
    }

    SOC_DEBUG_PRINT((DK_VERBOSE, "CES CORID:0x%04x\n", (uint16)coreid));

    if (coreid != BCM_CES_COREID) {
	SOC_DEBUG_PRINT((DK_VERBOSE, "CES CORID:0x%04x does not match expected value:0x%04x\n", 
			 (uint16)coreid, (uint16)BCM_CES_COREID));
	return BCM_E_INTERNAL;
    }

    /*
     * Open the CES device
     */
    sal_memset(&p_msg, 0, sizeof(AgNdMsgOpen));
    p_msg.n_base   = addr;
    p_msg.b_use_hw = AG_TRUE;
    p_msg.n_ext_mem_bank0_size = 0;
    p_msg.n_ext_mem_bank1_size = 0; 

    if (ag_nd_device_open(&p_msg, &config->ndHandle) != AG_S_OK)
    {
	ag_nd_module_remove();
	return BCM_E_UNAVAIL;
    }

#ifdef BCM_WARM_BOOT_SUPPORT    
    ces_scache_size = sizeof(bcm_ces_warm_boot_t) * BCM_CES_CIRCUIT_IDX_MAX; 

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_CES, 0);

    ret = _bcm_esw_scache_ptr_get(unit, scache_handle,
				  (0 == SOC_WARM_BOOT(unit)),
				  ces_scache_size, &ces_scache_ptr, 
				  BCM_WB_DEFAULT_VERSION, NULL);
    if (BCM_FAILURE(ret) && (ret != BCM_E_NOT_FOUND)) {
        return ret;
    }

    if (SOC_WARM_BOOT(unit)) {
        return _bcm_esw_ces_reinit(unit);
    }

#endif

    /*
     * Populate config with values from soc properties and default values.
     */
    config->ndInit.e_one_sec_pulse_direction = AG_ND_ONE_SECOND_PULSE_DIRECTION_OUT; 
    config->ndInit.e_bit_order = AG_ND_TDM_PAYLOAD_BIT_ORDERING_STANDARD;

    config->ndInit.n_packet_max = 0;           /* Not used in ces16core */
    config->ndInit.n_pw_max = BCM_CES_PW_MAX;
    config->ndInit.n_pbf_max = 0;              /* Not used in ces16core */

    config->ndInit.b_isr_mode = AG_FALSE;       
    config->ndInit.n_isr_task_priority = 10;
    config->ndInit.n_isr_task_wakeup = 100; 
    config->ndInit.b_ptp_support = AG_TRUE;
    config->ndInit.p_cb_pmi = bcm_esw_ces_pmi_cb; 
    config->ndInit.p_cb_pbi = bcm_esw_ces_pbi_cb;
    config->ndInit.p_cb_cwi = bcm_esw_ces_cwi_cb;
    config->ndInit.p_cb_psi = bcm_esw_ces_psi_cb;
    config->ndInit.p_cb_tpi = bcm_esw_ces_tpi_cb;
    config->ndInit.n_user_data_pbi = unit;
    config->ndInit.n_user_data_cwi = unit;
    config->ndInit.n_user_data_psi = unit;
    config->ndInit.n_user_data_tpi = unit;

    n_ret = ag_nd_device_write(config->ndHandle, AG_ND_OPCODE_CONFIG_INIT, &config->ndInit);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }


    /*
     * Setup default clock configuration. 
     *
     * - Framer clock enabled
     * - Reference Clock 1 selected
     * - T1/E1 selected from soc TDM_PROTO property (T1 default)
     * - Reference Clock 1 set to Internal 25MHz osc
     * 
     * With this configurtion all other parameters are ignored
     * hence they are not set here.
     */
    str = soc_property_get_str(unit, spn_CES_PORT_TDM_PROTO);
    if (str != NULL)
    {
	if ( sal_strcmp(str, "T1") == 0 )
	    config->protocol = bcmCesTdmProtocolT1;
	else if ( sal_strcmp(str, "E1") == 0 )
	    config->protocol = bcmCesTdmProtocolE1;
	else
	{
	    
	    return BCM_E_PARAM;
	}
    }
    else
	config->protocol = bcmCesTdmProtocolT1;

    /*
     * System clock rate
     */
    config->system_clock_rate = soc_property_get(unit, spn_CES_SYSTEM_CLOCK_RATE, BCM_CES_SYSTEM_CLOCK_RATE);
    config->common_ref_clock_rate = soc_property_get(unit, spn_CES_COMMON_REF_CLOCK_RATE, BCM_CES_COMMON_REF_CLOCK_RATE);


    /*
     * Get ces_mii_mac from soc properties
     */
    sal_memset(config->mac, 0, sizeof(sal_mac_addr_t));

    mac_str = soc_property_get_str(unit, spn_CES_MII_MAC);
    if (mac_str != NULL) {
	if (_shr_parse_macaddr(mac_str, config->mac) < 0 ) {
	    return BCM_E_INTERNAL;                 
	}
    } else {
	config->mac[0] = 0x00;
	config->mac[1] = 0xF1;
	config->mac[2] = 0xF2;
	config->mac[3] = 0xF3;
	config->mac[4] = 0xF4;
	config->mac[5] = 0xF5;
    }


    /*
     * Get the IPv4 address
     */
    ip_str = soc_property_get_str(unit, spn_CES_IPV4_ADDRESS);

    if (ip_str != NULL) {
	_shr_parse_ipaddr(ip_str, &config->ipv4_addr);
    } else {
	config->ipv4_addr = 0x0A0A0A0A;
    }

    
    ip_str = soc_property_get_str(unit, spn_CES_IPV6_ADDRESS);
    if (ip_str != NULL) {
	
#if 0
	if (bcm_ces_parse_ipv6addr(ip_str, config->ipv6_addr) < 0 ) {
	    return BCM_E_INTERNAL;                 
	}
#else
	sal_memset(config->ipv6_addr, 0x55, sizeof(ip6_addr_t));
#endif
    } else {
	sal_memset(config->ipv6_addr, 0xAA, sizeof(ip6_addr_t));
    }

    /*
     * MAC
     */
    n_ret = ag_nd_device_read(config->ndHandle, AG_ND_OPCODE_CONFIG_MAC, &config->ndMac);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    config->ndMac.n_frame_length = 1500; 
    config->ndMac.x_command_config.b_no_length_check     = AG_TRUE;
    config->ndMac.x_command_config.b_enable_mac_receive  = AG_TRUE;
    config->ndMac.x_command_config.b_enable_mac_transmit = AG_TRUE;
    config->ndMac.x_command_config.b_enable_promiscuous_mode = AG_TRUE;
    config->ndMac.x_command_config.b_fwd_pause_frames        = AG_TRUE;
    config->ndMac.x_command_config.b_enable_frame_padding    = AG_TRUE;

    sal_memcpy(config->ndMac.a_mac, config->mac, sizeof(sal_mac_addr_t));

    config->ndMac.n_rx_section_empty = BCM_CES_RX_SECTION_EMPTY;
    config->ndMac.n_rx_section_full  = BCM_CES_RX_SECTION_FULL;
    config->ndMac.n_tx_section_empty = BCM_CES_TX_SECTION_EMPTY;
    config->ndMac.n_tx_section_full  = BCM_CES_TX_SECTION_FULL;
    config->ndMac.n_rx_almost_empty  = BCM_CES_RX_ALMOST_EMPTY;
    config->ndMac.n_rx_almost_full   = BCM_CES_RX_ALMOST_FULL;
    config->ndMac.n_tx_almost_empty  = BCM_CES_TX_ALMOST_EMPTY;
    config->ndMac.n_tx_almost_full   = BCM_CES_TX_ALMOST_FULL;


    /*
     * global
     */
    n_ret = ag_nd_device_read(config->ndHandle, AG_ND_OPCODE_CONFIG_GLOBAL, &config->ndGlobal);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    config->ndGlobal.n_filler_byte = BCM_CES_FILLER_BYTE;
    config->ndGlobal.n_idle_byte   = BCM_CES_IDLE_BYTE;
    
    config->ndGlobal.a_lops_threshold_table[0] = BCM_CES_LOPS_THRESHOLD_TABLE;
    config->ndGlobal.a_aops_threshold_table[0] = BCM_CES_AOPS_THRESHOLD_TABLE; 
    config->ndGlobal.n_cw_mask = BCM_CES_CW_L | BCM_CES_CW_R | BCM_CES_CW_M;

    config->ndGlobal.n_cas_e1_idle_pattern = BCM_CES_CAS_T1_IDLE_PATTERN;
    config->ndGlobal.n_cas_t1_idle_pattern = BCM_CES_CAS_T1_IDLE_PATTERN;
    config->ndGlobal.n_cas_no_change_delay = BCM_CES_CAS_NO_CHANGE_DELAY;
    config->ndGlobal.n_cas_change_delay    = BCM_CES_CAS_CHANGE_DELAY;
    config->ndGlobal.n_cas_sqn_window      = BCM_CES_CAS_SQN_WINDOW;
    config->ndGlobal.n_rai_detect          = bcmCesRaiDetectM | bcmCesRaiDetectR;

    /*
     * policy
     */
    ag_nd_memset(&config->ndPolicy, 0, sizeof(config->ndPolicy));
    config->ndPolicy.x_policy_matrix.n_status_polarity = 0xffffffff;


    /*
     * ucode
     */
    sal_memcpy(config->ndUcode.a_dest_ipv6, config->ipv6_addr, sizeof(ip6_addr_t));
    sal_memcpy(config->ndUcode.a_dest_mac, config->mac, sizeof(sal_mac_addr_t));
    config->ndUcode.b_ecid_direct = AG_FALSE;
    config->ndUcode.b_mpls_direct = AG_FALSE;
    config->ndUcode.b_udp_direct  = AG_FALSE;
    config->ndUcode.n_dest_ipv4   = config->ipv4_addr;
    config->ndUcode.n_vlan_mask   = 0x0FFF;

    /*
     * Mark as configured
     */
    BCM_CES_SET_CONFIGURED(config);
    config->attached = TRUE;

    return BCM_E_NONE;
}


/**
 * Function:
 *      bcm_esw_ces_services_init
 * Purpose:
 *      Initialize CES services subsystem.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_INTERNAL An internal error occured
 * Notes:
 */
int 
bcm_esw_ces_services_init(int unit) {
    AgResult    n_ret;
    bcm_ces_service_global_config_t *config = SOC_CONTROL(unit)->ces_ctrl;
    int i;
    int ret;
    AgNdMsgConfigRpcPolicy policy;
    bcm_ces_cclk_config_t cclk_config;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    CES_LOCK(unit);

    /*
     * Grab ge0 for CES
     */
    if ((ret = bcm_esw_ces_enable_mii_port(unit, config->ces_mii_port, TRUE)) != BCM_E_NONE) {
	SOC_DEBUG_PRINT((DK_VERBOSE, "Failed to set GE0 port speed\n"));
	CES_UNLOCK(unit);
	return BCM_E_INTERNAL;
    }

#if defined(BCM_CES_CRM)
    if (!ces_ctrl->uc_appl_initialized) {
	/* 
	 * Find which core is running the CRM and say hello to the CRM
	 */
	ret = bcm_esw_ces_crm_init(unit, BCM_CES_CRM_FLAG_INIT | (SOC_WARM_BOOT(unit) ? BCM_CES_CRM_FLAG_WB:0x00));
	if (ret != BCM_E_NONE) {
	    SOC_DEBUG_PRINT((DK_ERR, "CES failed to locate CRM\n"));
	    CES_UNLOCK(unit);
	    return ret; 
	}
    
	ces_ctrl->uc_appl_initialized = TRUE;
    }
#else
    soc_cm_print("**** CES Ignoring uKernel ****\n");
#endif

    /*
     * Clock config
     */
    sal_memset(&cclk_config, 0, sizeof(bcm_ces_cclk_config_t));
    cclk_config.cclk_enable      = TRUE;                           /* Enable clock to framer */
    cclk_config.cclk_select      = AG_ND_CCLK_SELECT_REF_CLK_1;    /* Common clock source select. */
    cclk_config.ref_clk_proto    = config->protocol;               /* Selects reference clock protocol. */
    cclk_config.ref_clk_1_select = AG_ND_REF_CLK_SELECT_NOMAD_BRG; /* Reference clock 1 source select */
    cclk_config.ref_clk_2_select = bcmCesCclkSelectRefClk1; /* Reference clock 1 source select */
    cclk_config.ref_clk_1_port   = bcm_ces_circuit_id_to_port(unit, 0);
    cclk_config.ref_clk_2_port   = bcm_ces_circuit_id_to_port(unit, 1);

    ret = bcm_esw_ces_services_cclk_config_set(unit, &cclk_config);
    if (ret != BCM_E_NONE) {
	CES_UNLOCK(unit);
	return ret;
    }

    n_ret = ag_nd_device_write(config->ndHandle, AG_ND_OPCODE_CONFIG_MAC, &config->ndMac);
    if (!AG_SUCCEEDED(n_ret))
    {
	CES_UNLOCK(unit);
	return BCM_E_INTERNAL;
    }

    n_ret = ag_nd_device_write(config->ndHandle, AG_ND_OPCODE_CONFIG_GLOBAL, &config->ndGlobal);
    if (!AG_SUCCEEDED(n_ret))
    {
	CES_UNLOCK(unit);
	return BCM_E_INTERNAL;
    }

    n_ret = ag_nd_device_write(config->ndHandle, AG_ND_OPCODE_CONFIG_RPC_POLICY, &config->ndPolicy);
    if (!AG_SUCCEEDED(n_ret))
    {
	CES_UNLOCK(unit);
	return BCM_E_INTERNAL;
    }

    n_ret = ag_nd_device_write(config->ndHandle, AG_ND_OPCODE_CONFIG_RPC_UCODE, &config->ndUcode);
    if (!AG_SUCCEEDED(n_ret))
    {
	CES_UNLOCK(unit);
	return BCM_E_INTERNAL;
    }

    
    bcm_esw_ces_service_destroy_all(unit);

    /*
     * Initialize service records.
     */
    for (i = 0;i < BCM_CES_CIRCUIT_IDX_MAX;i++)
    {
	if ((ret = bcm_ces_service_init_record(unit, i)) != BCM_E_NONE)
	{
	    CES_UNLOCK(unit);
	    return ret;
	}
    }

    /*
     * Set RPC default policy
     */
    sal_memset(&policy, 0, sizeof(AgNdMsgConfigRpcPolicy));

    policy.x_policy_matrix.n_status_polarity     = 0xFFFFFFFF;
    policy.x_policy_matrix.n_drop_unconditional  = 0xF1F25EC0;
    policy.x_policy_matrix.n_drop_if_not_forward = 0x00000000;
    policy.x_policy_matrix.n_forward_to_ptp      = 0x00000000;
    policy.x_policy_matrix.n_forward_to_host_high = 0x00000000;
    policy.x_policy_matrix.n_forward_to_host_low  = 0x00000000;
    policy.x_policy_matrix.n_channel_counter_1 = 0x11004000;
    policy.x_policy_matrix.n_channel_counter_2 = 0x02000000;
    policy.x_policy_matrix.n_channel_counter_3 = 0x04000000;
    policy.x_policy_matrix.n_channel_counter_4 = 0x00008000;
    policy.x_policy_matrix.n_global_counter_1 = 0xF0000000;
    policy.x_policy_matrix.n_global_counter_2 = 0x0F000000;
    policy.x_policy_matrix.n_global_counter_3 = 0x00F00000;
    policy.x_policy_matrix.n_global_counter_4 = 0x000F0000;
    policy.x_policy_matrix.n_global_counter_5 = 0x0000F000;
    policy.x_policy_matrix.n_global_counter_6 = 0x00000F00;
    policy.x_policy_matrix.n_global_counter_7 = 0x000000F0;
    policy.x_policy_matrix.n_global_counter_8 = 0x0000000F;

    if (bcm_esw_ces_rpc_pm_set(unit, &policy)!= BCM_E_NONE) {
	CES_UNLOCK(unit);
	return BCM_E_INTERNAL;
    }

    /*
     * Initialize the rclock instances
     */
    memset(config->rclock_record, 0 , sizeof(bcm_ces_rclock_record_t) * BCM_CES_RCLOCK_MAX);

    CES_UNLOCK(unit);
    return BCM_E_NONE; 
}

/**
 * Function:
 *      bcm_esw_ces_detach
 * Purpose:
 *      Shut down the CES subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_esw_ces_detach(int unit)
{
    int result = BCM_E_UNAVAIL;
    int i;
    bcm_ces_service_global_config_t *config = SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    if (ces_ctrl == NULL)
	return BCM_E_NONE;
    

    if (soc_feature(unit, soc_feature_ces)) {

	if (config != NULL) {
	    
            config->attached = FALSE;
	    CES_LOCK(unit);

	    if (BCM_CES_IS_CONFIGURED(config)) {
		/*
		 * Detach and cleanup CES driver
		 */
		ag_nd_device_close(config->ndHandle);
		ag_nd_module_remove();

		/*
		 * Deallocate service records.
		 */
		for (i = 0;i < BCM_CES_CIRCUIT_IDX_MAX;i++) {
		    if (config->bcm_ces_service_records[i] != NULL)
			sal_free(config->bcm_ces_service_records[i]);
		}
	    }

            /*
             * Unlock mutex before destroying
             */
            CES_UNLOCK(unit);
	    sal_mutex_destroy(ces_ctrl->lock);
	    sal_free(config);
	    SOC_CONTROL(unit)->ces_ctrl = NULL;
	}
    }


    return result;
}

/**
 * Function:
 *      bcm_ces_services_clear
 * Purpose:
 *      Initialize CES services subsystem without affecting current
 *      hardware state.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_services_clear(
    int unit)
{
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    return BCM_E_UNAVAIL; 
}

/**
 * Function:
 *      bcm_esw_ces_service_config_channelizer
 * Purpose:
 *      Utility method to configure a channelizer
 * Parameters:
 *      unit - (IN) Unit number.
 *      config - (IN) <UNDEF>
 *      ces_service - (OUT) CES Channel
 *      path - (IN)
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_service_config_channelizer(
    int unit, 
    bcm_ces_service_record_t *record,
    bcm_ces_channel_map_t *map,
    int path)
{
    AgResult n_ret;
    AgNdMsgConfigChannelizer ndChannelizer;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;
    int i;

    /*
     * chi
     */
    sal_memset(&ndChannelizer, 0, sizeof(AgNdMsgConfigChannelizer));
    ndChannelizer.e_path        = path;
    ndChannelizer.n_channel_id  = (AgNdChannel)record->ces_service;
    ndChannelizer.x_map.n_first = map->first;
    ndChannelizer.x_map.n_size  = map->size;
    for (i = map->first;i < (map->first + map->size);i++)
    {
	ndChannelizer.x_map.a_ts[i].n_circuit_id = (AgNdCircuit) bcm_ces_port_to_circuit_id(unit, map->circuit_id[i]);
	ndChannelizer.x_map.a_ts[i].n_slot_idx = map->slot[i];
    }

    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_CHANNELIZER, &ndChannelizer);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_ces_copy_header
 * Purpose:
 *      Copy header from BRCM to BATM format
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 * Notes:
 */
void bcm_ces_copy_header(bcm_ces_packet_header_t *src, AgNdPacketHeader *dest) {
    int i;

    sal_memcpy(dest->x_eth.a_source, src->eth.source, sizeof(sal_mac_addr_t));
    sal_memcpy(dest->x_eth.a_destination, src->eth.destination, sizeof(sal_mac_addr_t));
    dest->e_encapsulation = src->encapsulation;

    dest->x_ip4.n_tos = src->ipv4.tos;
    dest->x_ip4.n_ttl = src->ipv4.ttl;
    dest->x_ip4.n_source = src->ipv4.source;
    dest->x_ip4.n_destination = src->ipv4.destination;

    dest->x_vc_label.n_label = src->vc_label;
    dest->n_ip_version = src->ip_version;

    dest->n_vlan_count = src->vlan_count;
    for (i = 0;i < src->vlan_count;i++) {
	dest->a_vlan[i].n_priority = src->vlan[i].priority;
	dest->a_vlan[i].n_vid = src->vlan[i].vid;
    }

    dest->n_mpls_count = src->mpls_count;
    for (i = 0;i < src->mpls_count;i++) {
	 dest->a_mpls[i].n_label = src->mpls[i].label;
	 dest->a_mpls[i].n_expiremental = src->mpls[i].experimental;
	 dest->a_mpls[i].n_ttl = src->mpls[i].ttl;
    }

    dest->x_udp.n_destination = src->udp.destination;
    dest->x_udp.n_source = src->udp.source;

    dest->b_rtp_exists = src->rtp_exists;
    dest->x_rtp.n_pt = src->rtp_pt;
    dest->x_rtp.n_ssrc = src->rtp_ssrc;

    dest->b_udp_chksum = src->udp_chksum;

    dest->n_l2tpv3_count = src->l2tpv3_count; 
    dest->x_l2tpv3.b_udp_mode = src->l2tpv3.udp_mode;
    dest->x_l2tpv3.n_header = src->l2tpv3.header;
    dest->x_l2tpv3.n_session_local_id = src->l2tpv3.session_local_id;
    dest->x_l2tpv3.n_session_peer_id = src->l2tpv3.session_peer_id;
    dest->x_l2tpv3.n_local_cookie1   = src->l2tpv3.local_cookie1;
    dest->x_l2tpv3.n_local_cookie2   = src->l2tpv3.local_cookie2;
    dest->x_l2tpv3.n_peer_cookie1    = src->l2tpv3.peer_cookie1;
    dest->x_l2tpv3.n_peer_cookie2    = src->l2tpv3.peer_cookie2;

    sal_memcpy(dest->x_ip6.a_source, src->ipv6.source, 16);
    sal_memcpy(dest->x_ip6.a_destination, src->ipv6.destination, 16);
    dest->x_ip6.n_traffic_class = src->ipv6.traffic_class;
    dest->x_ip6.n_flow_label = src->ipv6.flow_label;
    dest->x_ip6.n_hop_limit  = src->ipv6.hop_limit;
}

/**
 * Function:
 *      bcm_ces_init_set_mac_address
 * Purpose:
 *      Replace the MAC address in the uCode. This is the system MAC (source address).
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (IN) <UNDEF>
 *      config - (IN) <UNDEF>
 *      ces_service - (OUT) CES Channel
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_BADID Invalid service ID was used
 *      BCM_E_PARAM The config parameter was NULL
 *      BCM_E_INTERNAL An internal error occurred
 * Notes:
 */
AgResult bcm_ces_init_set_mac_address(bcm_ces_service_global_config_t *ctrl, AG_U8 *p_mac_address) {
    AgResult res;
    AgNdMsgConfigRpcUcode x_rpc_msg;

    sal_memset(&x_rpc_msg,0x00,sizeof(AgNdMsgConfigRpcUcode));
    res = ag_nd_device_read(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_RPC_UCODE, &x_rpc_msg);

    /*set mac address*/
    sal_memcpy(x_rpc_msg.a_dest_mac,p_mac_address,6);

    res = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_RPC_UCODE, &x_rpc_msg);
    if (AG_FAILED(res)) {
       /* BOARD_ERR_LOG0("CAgFPGAFWNemo::set_mac_address  Error configuring RPC machine");* */
        return res;
    }

    return AG_S_OK;
}

/**
 * Function:
 *      bcm_ces_init_set_ip_address_in_RPC_machine
 * Purpose:
 *      Replace IP address in uCode. This is the system IP (source address).
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (IN) <UNDEF>
 *      config - (IN) <UNDEF>
 *      ces_service - (OUT) CES Channel
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_BADID Invalid service ID was used
 *      BCM_E_PARAM The config parameter was NULL
 *      BCM_E_INTERNAL An internal error occurred
 * Notes:
 */
AgResult bcm_ces_init_set_ip_address_in_RPC_machine(bcm_ces_service_global_config_t *ctrl, AG_U32 IP_address) {
    AgResult res = AG_S_OK;
    AgNdMsgConfigRpcUcode x_rpc_msg;

    sal_memset(&x_rpc_msg,0x00,sizeof(AgNdMsgConfigRpcUcode));
    res = ag_nd_device_read(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_RPC_UCODE, &x_rpc_msg);
    /*Set IP address*/
    x_rpc_msg.n_dest_ipv4 = IP_address;

    res = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_RPC_UCODE, &x_rpc_msg);
    if (AG_FAILED(res)) {
        /*BOARD_ERR_LOG0("Error configuring RPC machine");*/
        return res;
    }

    return res;
}

/**
 * Function:
 *      bcm_ces_init_set_ip_address_in_RPC_machine
 * Purpose:
 *      Replace IP address in uCode. This is the system IP (source address).
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (IN) <UNDEF>
 *      config - (IN) <UNDEF>
 *      ces_service - (OUT) CES Channel
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_BADID Invalid service ID was used
 *      BCM_E_PARAM The config parameter was NULL
 *      BCM_E_INTERNAL An internal error occurred
 * Notes:
 */
int bcm_ces_get_tss_index(uint32 clock) {
    if (clock <= 1544000L)
	return BCM_CES_RTP_TSS_TABLE_INDEX_1544;
    else if (clock <= 2048000L)
	return BCM_CES_RTP_TSS_TABLE_INDEX_2048;
    else if (clock <= 10240000L)
	return BCM_CES_RTP_TSS_TABLE_INDEX_10240;
    else if (clock <= 16384000L)
	return BCM_CES_RTP_TSS_TABLE_INDEX_16384;
    else if (clock <= 19440000L)
	return BCM_CES_RTP_TSS_TABLE_INDEX_19440;
    else if (clock <= 25000000L)
	return BCM_CES_RTP_TSS_TABLE_INDEX_25000;
    else if (clock <= 125000000L)
	return BCM_CES_RTP_TSS_TABLE_INDEX_125000;

    return BCM_CES_RTP_TSS_TABLE_INDEX_1544;
}


/**
 * Function:
 *      bcm_ces_channel_map_valid
 * Purpose:
 *      Validate circuits in channel map
 * Parameters:
 *      protocol - TDM protocol
 *      map      - channel map
 * Returns:
 *      TRUE - OK
 *      FALSE - Somethng is wrong
 * Notes:
 */
int 
bcm_ces_channel_map_valid(int unit, bcm_ces_channel_map_t *m, bcm_ces_tdm_proto_t protocol) { 
    int i;
    int max_slot;
    int circuit_id;

    if (protocol == bcmCesTdmProtocolE1)
	max_slot = 32;
    else
	max_slot = 24;

    if (!((m->first >= 0) && (m->first < (BCM_CES_SLOT_MAX * BCM_CES_CIRCUIT_IDX_MAX))))
	return FALSE;

    if (m->size > max_slot)
	return FALSE;

    for (i = m->first;i < (m->first + m->size);i++) { 
	circuit_id = bcm_ces_port_to_circuit_id(unit, m->circuit_id[i]);

        if (circuit_id >= BCM_CES_TDM_MAX || circuit_id < 0) 
	    return FALSE; 
    } 

    for (i = 0;i < (BCM_CES_SLOT_MAX * BCM_CES_CIRCUIT_IDX_MAX);i++) { 
        if (m->slot[i] > max_slot) 
	    return FALSE; 

	if (m->slot[i] == 0)
	    break;
    } 

    return TRUE;
}

/**
 * Function:
 *      bcm_ces_channel_ingress_valid()
 * Purpose:
 *      Validate circuits in channel map
 * Parameters:
 *      protocol - TDM protocol
 *      map      - channel map
 * Returns:
 *      TRUE - OK
 *      FALSE - Somethng is wrong
 * Notes:
 */
int 
bcm_ces_channel_ingress_valid(bcm_ces_ingress_channel_config_t *m) { 
    
    if ((m->dba < 0) || (m->dba > 1)) {
	return FALSE;
    }

    return TRUE;
}


/**
 * Function:
 *      bcm_esw_ces_header_valid()
 * Purpose:
 *      Validate mac address
 * Parameters:
 *      protocol - TDM protocol
 *      header   - Header
 * Returns:
 *      TRUE - OK
 *      FALSE - Somethng is wrong
 * Notes:
 */
int 
bcm_esw_ces_header_valid(bcm_ces_packet_header_t *header) {
    int i;


    /*
     * All zero MACs? 
     */
    for (i = 0;i < 6;i++) {
	if (*(header->eth.source + i) != 0)
	    break;
    }

    if (i >= 6)
	return FALSE;

    /*
     * All zero MACs? 
     */
    for (i = 0;i < 6;i++) {
	if (*(header->eth.destination + i) != 0)
	    break;
    }

    if (i >= 6)
	return FALSE;

    /*
     * Vlan count
     */
    if (header->vlan_count < 0 || header->vlan_count >  BCM_CES_PROTO_VLAN_TAG_MAX)
	return FALSE;

    /*
     * MPLS count
     */
    if (header->mpls_count < 0 || header->mpls_count > BCM_CES_PROTO_MPLS_LABEL_MAX)
	return FALSE;

    /*
     * L2TP count
     */
    if (header->l2tpv3_count < 0 || header->l2tpv3_count > BCM_CES_PROTO_L2TPV3_MAX)
	return FALSE;

    /*
     * Encap
     */
    switch(header->encapsulation) {
    case bcmCesEncapsulationIp:
	/*
	 * Valid version?
	 */
	if (header->ip_version != bcmCesIpV4 &&
	    header->ip_version != bcmCesIpV6)
	    return FALSE;

	/*
	 * Valid IP addresses
	 */
	if (header->ipv4.source == 0 ||
	    header->ipv4.destination == 0)
	    return FALSE;

	/*
	 * TTL?
	 */
	if (header->ipv4.ttl == 0)
	    return FALSE;

	/*
	 * TOS?
	 */

	/*
	 * For TCP encap the ID is passed in the udp source and dest
	 * fields.
	 *
	 * UDP?
	 */
	if (header->udp.source == 0 ||
	    header->udp.destination == 0)
	    return FALSE;

	break;

    case bcmCesEncapsulationMpls:
	if (header->mpls_count == 0)
	    return FALSE;

	for (i = 0;i < header->mpls_count;i++) {
	    if (header->mpls[i].label == 0)
		return FALSE;
	    if (header->mpls[i].ttl == 0)
		return FALSE;
	}
	break;

    case bcmCesEncapsulationEth:
	if (header->vc_label == 0)
	    return FALSE;
	break;

    case bcmCesEncapsulationL2tp:
	/*
	 * Valid version?
	 */
	if (header->ip_version != bcmCesIpV4 &&
	    header->ip_version != bcmCesIpV6)
	    return FALSE;

	/*
	 * Valid IP addresses
	 */
	if (header->ip_version == bcmCesIpV4 &&
	    (header->ipv4.source == 0 ||
	     header->ipv4.destination == 0))
	    return FALSE;

	/*
	 * TTL?
	 */
	if (header->ipv4.ttl == 0)
	    return FALSE;

	/*
	 * Check L2TP version
	 */
	if (header->l2tpv3.header != 0x30000)
	    return FALSE;

	/*
	 * Check that local and peer session IDs are set
	 */
	if (header->l2tpv3.session_local_id == 0 ||
	    header->l2tpv3.session_peer_id == 0)
	    return FALSE;

	/*
	 * Check that local and peer session IDs are set
	 */
	if (header->l2tpv3_count >= 2 && 
	    (header->l2tpv3.local_cookie1 == 0 ||
	     header->l2tpv3.peer_cookie1 == 0))
	    return FALSE;

	/*
	 * Check that local and peer session IDs are set
	 */
	if (header->l2tpv3_count == 3 && 
	    (header->l2tpv3.local_cookie2 == 0 ||
	     header->l2tpv3.peer_cookie2 == 0))
	    return FALSE;

	/*
	 * If UDP is being used make sure the port #s are correct
	 */
	if (header->l2tpv3.udp_mode == 1 &&
	    (header->udp.source != 1701 ||
	     header->udp.destination != 1701))
	    return FALSE;

	break;

    default:
	return FALSE;
	break;
    }


    return TRUE;
}

/**
 * Function:
 *      bcm_esw_ces_service_create
 * Purpose:
 *      Allocate and configure a CES service.
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (IN) <UNDEF>
 *      config - (IN) <UNDEF>
 *      ces_service - (OUT) CES Channel
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_BADID Invalid service ID was used
 *      BCM_E_PARAM The config parameter was NULL
 *      BCM_E_INTERNAL An internal error occurred
 * Notes:
 */
int 
bcm_esw_ces_service_create(
    int unit, 
    int flags, 
    bcm_ces_service_config_t *config, 
    bcm_ces_service_t *ces_service)
{
    AgResult n_ret;
    bcm_ces_service_record_t *record;
    int i;
    bcm_ces_channel_map_t *map;
    AgNdMsgConfigCircuitEnable     ndCircuitEnable;
    AgNdMsgConfigChannelEgress     ndChannelEgress;
    AgNdMsgConfigChannelIngress    ndChannelIngress;
    AgNdMsgConfigNemoCircuit       ndCircuit;
    AgFramerConfig                 ndFramer;
    AgNdMsgConfigCasChannelIngress ndCasIngress;
    AgNdMsgConfigNemoDcrConfigurations ndDcrConfig;
    AgNdMsgConfigNemoDcrClkSource      ndDcrClkSource;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;
    int egressChannelMapChange   = TRUE;
    int ingressChannelMapChange  = TRUE;
    int channelEgressChange      = TRUE;
    int channelIngressChange     = TRUE;
    int channelizerEgressChange  = TRUE;
    int channelizerIngressChange = TRUE;
    int nemoCircuitChange        = TRUE;
    int headerChange             = TRUE;
    int strictDataChange         = TRUE;
    int destMacChange            = TRUE;
    int encapsulationChange      = TRUE;
    int enabled                  = FALSE;
    uint32 tcr;
    uint32 scr;
    uint32 tcr_index;
    uint32 crc_index;
    int16 modPorts[BCM_CES_SLOT_MAX];
    int j;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);
    CES_LOCK(unit);

    sal_memset(modPorts, -1, sizeof(modPorts));

    map = (bcm_ces_channel_map_t*)sal_alloc(sizeof(bcm_ces_channel_map_t), "CES Channel map");
    if (map == NULL)
        return BCM_E_MEMORY;

    /*
     * If this is a new service then get a service (channel) 
     * and allocate a service record.
     */
    if (!(flags & BCM_CES_WITH_ID) && !(flags & BCM_CES_TDM_UPDATE_WITH_ID)) {
	n_ret = bcm_ces_service_get_service(unit, ces_service, &record, FALSE);
	if (n_ret != BCM_E_NONE) {
	    CES_UNLOCK(unit);
            sal_free(map);
	    return n_ret;
	}
    } else if ((flags & BCM_CES_WITH_ID) && !(flags & BCM_CES_TDM_UPDATE_WITH_ID) &&
	       (n_ret = bcm_ces_service_get_service(unit, ces_service, &record, TRUE) == BCM_E_NONE)) {
	/*
	 * Create service with specified ID
	 *
	 * Supress the flag. It will now be used just to indicate a modify
	 */
	flags &= ~BCM_CES_WITH_ID;
    } else {
	/*
	 * Modify existing service record
	 */

	n_ret = bcm_ces_service_find(unit, *ces_service, &record);
	if (n_ret != BCM_E_NONE) {
	    CES_UNLOCK(unit);
            sal_free(map);
	    return n_ret;
	}

	if (BCM_CES_IS_FREE(&record->config)) {
	    CES_UNLOCK(unit);
            sal_free(map);
	    return BCM_E_UNAVAIL;
	}

	/*
	 * If enabled then disable.
	 */
	if (BCM_CES_IS_ENABLED(&record->config)) {
	    enabled = TRUE;
	    bcm_esw_ces_service_enable_set(unit, *ces_service, 0);
	}

	if (flags & BCM_CES_WITH_ID) {
	    /*
	     * Identify configuration changes
	     */
	    if (sal_memcmp(&config->ingress_channel_map, 
			   &record->config.ingress_channel_map, 
			   sizeof(bcm_ces_channel_map_t)) == 0)
		ingressChannelMapChange  = FALSE;

	    if (sal_memcmp(&config->egress_channel_map, 
			   &record->config.egress_channel_map, 
			   sizeof(bcm_ces_channel_map_t)) == 0)
		egressChannelMapChange  = FALSE;

	    if (sal_memcmp(&config->header, 
			   &record->config.header, 
			   sizeof(bcm_ces_packet_header_t)) == 0)
		headerChange  = FALSE;

	    if (sal_memcmp(&config->strict_data, 
			   &record->config.strict_data, 
			   sizeof(bcm_ces_packet_header_t)) == 0)
		strictDataChange  = FALSE;

	    if (sal_memcmp(&config->egress_channel_config, 
			   &record->config.egress_channel_config, 
			   sizeof(bcm_ces_egress_channel_config_t)) == 0)
		channelEgressChange = FALSE;

	    if (sal_memcmp(&config->ingress_channel_config, 
			   &record->config.ingress_channel_config, 
			   sizeof(bcm_ces_ingress_channel_config_t)) == 0)
		channelIngressChange = FALSE;


	    if (sal_memcmp(&config->dest_mac, 
			   &record->config.dest_mac, 
			   sizeof(bcm_mac_t)) == 0)
		destMacChange = FALSE;

	    if (sal_memcmp(&config->encapsulation, 
			   &record->config.encapsulation, 
			   sizeof(bcm_ces_encapsulation_t)) == 0)
		encapsulationChange = FALSE;


	    channelizerIngressChange = FALSE;
	    channelizerEgressChange  = FALSE;

	    nemoCircuitChange        = FALSE; 

	    /*
	     * If there are no changes then we are done
	     */
	    if (egressChannelMapChange  == FALSE &&
		ingressChannelMapChange  == FALSE &&
		channelEgressChange      == FALSE &&
		channelIngressChange     == FALSE &&
		channelizerEgressChange  == FALSE &&
		channelizerIngressChange == FALSE &&
		nemoCircuitChange        == FALSE &&
		headerChange             == FALSE &&
		strictDataChange         == FALSE &&
		destMacChange            == FALSE &&
		encapsulationChange      == FALSE) {
                sal_free(map);

		return BCM_E_NONE;
            }

	    /*
	     * If the channel mapping has changed then disable and remove existing
	     * channel mapping.  
	     */
	    if (ingressChannelMapChange || egressChannelMapChange) {

		if (ingressChannelMapChange) {

		    /*
		     * Remove ingress channel map
		     */
		    sal_memset(map, 0, sizeof(bcm_ces_channel_map_t));
		    n_ret = bcm_esw_ces_service_config_channelizer(unit, record, map, AG_ND_PATH_INGRESS);
		}

		if (egressChannelMapChange) {

		    /*
		     * Remove channel map
		     */
		    sal_memset(map, 0, sizeof(bcm_ces_channel_map_t));
		    n_ret = bcm_esw_ces_service_config_channelizer(unit, record, map, AG_ND_PATH_EGRESS);
		}
	    }
	} else if (flags & BCM_CES_TDM_UPDATE_WITH_ID) {
	    /*
	     * TDM only update. Config does not have to be provided.
	     */
	    if (config == NULL)
		config = &record->config;
	    egressChannelMapChange   = FALSE;
	    ingressChannelMapChange  = FALSE;
	    channelEgressChange      = FALSE;
	    channelIngressChange     = FALSE;
	    channelizerEgressChange  = FALSE;
	    channelizerIngressChange = FALSE;
	    headerChange             = FALSE;
	    strictDataChange         = FALSE;
	    destMacChange            = FALSE;
	    encapsulationChange      = FALSE;
	    nemoCircuitChange        = TRUE;
	}
    }

    /*
     * Config must be set 
     */
    if (config == NULL) {
        sal_free(map);

        return(BCM_E_PARAM);
    }

    /*
     * Make sure that no random flags are set
     */
    BCM_CES_CLR_ALL(config);

    /*
     * Save the requested config
     */
    if (config != &record->config) {
	sal_memcpy(&record->config, config, sizeof(bcm_ces_service_config_t)); 
	BCM_CES_SET_MODIFIED(&record->config);
    }
    
    if (!flags)
	record->ces_service = *ces_service;

    /*
     * Check channel maps
     */
    if (!bcm_ces_channel_map_valid(unit, &config->ingress_channel_map, ctrl->protocol) ||
	!bcm_ces_channel_map_valid(unit, &config->egress_channel_map, ctrl->protocol)) {
	
	if (!(flags & BCM_CES_WITH_ID))
	    BCM_CES_SET_FREE(&record->config);

        sal_free(map);
	return BCM_E_PARAM;
    }

    /*
     * For each circuit in the ingress channel map:
     * - Disable
     * - Config
     */
    for (i = config->ingress_channel_map.first;i < (config->ingress_channel_map.first + config->ingress_channel_map.size);i++)
    {
	/*
	 * Has the port state already been modified?
	 */
	j = 0;
	while(modPorts[j] != -1 && j < BCM_CES_SLOT_MAX) {
	    if (modPorts[j] == (int16)record->config.ingress_channel_map.circuit_id[i])
		break;
	    j++;
	}

        if (j < BCM_CES_SLOT_MAX) {
            if (modPorts[j] != (int16)record->config.ingress_channel_map.circuit_id[i]) {
                /*
                 * Make sure that the circuit is disabled
                 */
                ndCircuitEnable.n_circuit_id = (AgNdCircuit) bcm_ces_port_to_circuit_id(unit, config->ingress_channel_map.circuit_id[i]);
                ndCircuitEnable.b_enable = AG_FALSE;
                n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_CIRCUIT_ENABLE, &ndCircuitEnable);
                if (!AG_SUCCEEDED(n_ret))
                {
                    SOC_DEBUG_PRINT((DK_VERBOSE, "AG_ND_OPCODE_CONFIG_CIRCUIT_ENABLE failed\n"));
                    if (!(flags & BCM_CES_WITH_ID))
                        bcm_ces_service_free_record(unit, *ces_service);
                    CES_UNLOCK(unit);
                    sal_free(map);
                    return BCM_E_INTERNAL;
                }

                /*
                 * Disable framer. Note that the enable also can set parameters so recover
                 * the existing config before disabling.
                 */
                sal_memset(&ndFramer, 0, sizeof(AgFramerConfig));
                ndFramer.n_circuit_id = (AgNdCircuit) bcm_ces_port_to_circuit_id(unit, config->ingress_channel_map.circuit_id[i]);
                n_ret = ag_nd_device_read(ctrl->ndHandle, AG_ND_OPCODE_FRAMER_PORT_CONFIG, &ndFramer);
                if (!AG_SUCCEEDED(n_ret)) {
                    SOC_DEBUG_PRINT((DK_VERBOSE, "AG_ND_OPCODE_FRAMER_PORT_CONFIG failed\n"));
                    if (!(flags & BCM_CES_WITH_ID))
                        bcm_ces_service_free_record(unit, *ces_service);
                    CES_UNLOCK(unit);
                    sal_free(map);
                    return BCM_E_INTERNAL;
                }

                ndFramer.b_enable = FALSE;

                n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_FRAMER_PORT_CONFIG, &ndFramer);
                if (!AG_SUCCEEDED(n_ret)) {
                    SOC_DEBUG_PRINT((DK_VERBOSE, "AG_ND_OPCODE_FRAMER_PORT_CONFIG failed\n"));
                    if (!(flags & BCM_CES_WITH_ID))
                        bcm_ces_service_free_record(unit, *ces_service);
                    CES_UNLOCK(unit);
                    sal_free(map);
                    return BCM_E_INTERNAL;
                }


                /*
                 * Find associated TDM port
                 */
                if ((record->tdmRecord = bcm_esw_port_tdm_find_port(unit,(AgNdCircuit) config->ingress_channel_map.circuit_id[i])) == NULL)
                {
                    SOC_DEBUG_PRINT((DK_VERBOSE, "bcm_esw_port_tdm_find_port() failed\n"));
                    if (!(flags & BCM_CES_WITH_ID))
                        bcm_ces_service_free_record(unit, *ces_service);
                    CES_UNLOCK(unit);
                    sal_free(map);
                    return BCM_E_INTERNAL;
                }

                if (ingressChannelMapChange || egressChannelMapChange) {
                    /*
                     * Add to TDM port
                     */
                    n_ret = bcm_esw_port_tdm_add_service(unit,
                                                         record->tdmRecord, 
                                                         record,
                                                         config->ingress_channel_map.size, 
                                                         config->ingress_channel_map.slot);
                    if (n_ret != BCM_E_NONE)
                    {
                        SOC_DEBUG_PRINT((DK_VERBOSE, "bcm_esw_port_tdm_add_service() failed\n"));
                        bcm_esw_ces_service_free_tdm(unit, *ces_service);
                        if (!(flags & BCM_CES_WITH_ID))
                            bcm_ces_service_free_record(unit, *ces_service);
                        CES_UNLOCK(unit);
                        sal_free(map);
                        return BCM_E_INTERNAL;
                    }
                }

                if (nemoCircuitChange) {

                    /*
                     * Recover circuit configuration
                     */
                    sal_memset(&ndCircuit, 0, sizeof(AgNdCircuit));
                    ndCircuit.n_circuit_id     = (AgNdCircuit) bcm_ces_port_to_circuit_id(unit, record->tdmRecord->port);

                    if (flags & BCM_CES_TDM_UPDATE_WITH_ID) {
                        n_ret = ag_nd_device_read(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_NEMO_CIRCUIT, &ndCircuit);
                        if (!AG_SUCCEEDED(n_ret)) {
                            SOC_DEBUG_PRINT((DK_VERBOSE, "AG_ND_OPCODE_CONFIG_NEMO_CIRCUIT failed\n"));
                            bcm_esw_ces_service_free_tdm(unit, *ces_service);
                            if (!(flags & BCM_CES_WITH_ID))
                                bcm_ces_service_free_record(unit, *ces_service);
                            CES_UNLOCK(unit);
                            sal_free(map);
                            return BCM_E_INTERNAL;
                        }
                    }

                    ndCircuit.n_circuit_id     = (AgNdCircuit) bcm_ces_port_to_circuit_id(unit, record->tdmRecord->port);
                    ndCircuit.e_tdm_protocol   = record->tdmRecord->config.e_protocol;
                    ndCircuit.b_structured     = record->tdmRecord->config.b_structured;
                    ndCircuit.b_octet_aligned  = record->tdmRecord->config.b_octet_aligned;
                    ndCircuit.b_signaling_enable = record->tdmRecord->config.b_signaling_enable;
                    ndCircuit.b_T1_D4_framing  = record->tdmRecord->config.b_T1_D4_framing;
                    ndCircuit.e_clk_rx_select  = record->tdmRecord->config.e_clk_rx_select;
                    ndCircuit.e_clk_tx_select  = record->tdmRecord->config.e_clk_tx_select;

                    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_NEMO_CIRCUIT, &ndCircuit);
                    if (!AG_SUCCEEDED(n_ret))
                    {
                        SOC_DEBUG_PRINT((DK_VERBOSE, "AG_ND_OPCODE_CONFIG_NEMO_CIRCUIT failed\n"));
                        bcm_esw_ces_service_free_tdm(unit, *ces_service);
                        if (!(flags & BCM_CES_WITH_ID))
                            bcm_ces_service_free_record(unit, *ces_service);
                        CES_UNLOCK(unit);
                        sal_free(map);
                        return BCM_E_INTERNAL;
                    }

                    /*
                     * Framer configuration
                     */
                    sal_memset(&ndFramer, 0, sizeof(AgFramerConfig));
                    ndFramer.n_circuit_id     = (AgNdCircuit) bcm_ces_port_to_circuit_id(unit, record->tdmRecord->port);
                    ndFramer.b_enable = FALSE;
                    ndFramer.b_t1 = (record->tdmRecord->config.e_protocol == bcmCesTdmProtocolT1 ? 1:0);
                    ndFramer.b_esf = !(record->tdmRecord->config.b_T1_D4_framing);
                    ndFramer.b_transmit_crc = record->tdmRecord->config.b_rxcrc;
                    ndFramer.b_recive_crc   = record->tdmRecord->config.b_txcrc;
                    ndFramer.n_singling_format = record->tdmRecord->config.n_signaling_format; 
                    ndFramer.b_master = record->tdmRecord->config.b_master;

                    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_FRAMER_PORT_CONFIG, &ndFramer);
                    if (!AG_SUCCEEDED(n_ret))
                    {
                        SOC_DEBUG_PRINT((DK_VERBOSE, "AG_ND_OPCODE_FRAMER_PORT_CONFIG failed\n"));
                        bcm_esw_ces_service_free_tdm(unit, *ces_service);
                        if (!(flags & BCM_CES_WITH_ID))
                            bcm_ces_service_free_record(unit, *ces_service);
                        CES_UNLOCK(unit);
                        sal_free(map);
                        return BCM_E_INTERNAL;
                    }

                }
                modPorts[j] = (int16)record->config.ingress_channel_map.circuit_id[i];
            }
        }
    }


    /*
     * Channel Ingress
     */
    if (channelIngressChange || headerChange) {
	/*
	 * Validate config 
	 */
	if (!bcm_ces_channel_ingress_valid(&config->ingress_channel_config)) {
	    bcm_esw_ces_service_free_tdm(unit, *ces_service);
	    if (!(flags & BCM_CES_WITH_ID))
		BCM_CES_SET_FREE(&record->config);

	    CES_UNLOCK(unit);
            sal_free(map);
	    return BCM_E_PARAM;
	}

        sal_memset(&ndChannelIngress, 0, sizeof(AgNdMsgConfigChannelIngress)); 
	ndChannelIngress.n_channel_id   = (AgNdChannel)*ces_service;

	ndChannelIngress.b_redundancy_enabled = AG_FALSE;
	ndChannelIngress.b_redundancy_config = 0x00;

	ndChannelIngress.n_channel_id   = (AgNdChannel)*ces_service;

	if (config->ingress_channel_map.slot[0] != 0) 
	    ndChannelIngress.n_payload_size = config->ingress_channel_map.size * config->egress_channel_config.frames_per_packet;
	else if (ctrl->protocol == bcmCesTdmProtocolE1)
	    ndChannelIngress.n_payload_size = BCM_CES_E1_SLOT_MAX * config->egress_channel_config.frames_per_packet;
	else
	    ndChannelIngress.n_payload_size = BCM_CES_T1_SLOT_MAX * config->egress_channel_config.frames_per_packet;

	ndChannelIngress.b_dba          = config->ingress_channel_config.dba;
	ndChannelIngress.n_pbf_size     = config->ingress_channel_config.pbf_size;


	/*
	 * If L2TP is being used then use IP encapsulation 
	 */
	if (config->header.encapsulation == bcmCesEncapsulationL2tp)
	    ndChannelIngress.x_header.e_encapsulation = bcmCesEncapsulationIp;
	else
	    ndChannelIngress.x_header.e_encapsulation = config->header.encapsulation;

	/*
	 * Check that header fields are valid
	 */
	if (!bcm_esw_ces_header_valid(&config->header)) {
	    bcm_esw_ces_service_free_tdm(unit, *ces_service);
	    if (!(flags & BCM_CES_WITH_ID))
		BCM_CES_SET_FREE(&record->config);

	    CES_UNLOCK(unit);
            sal_free(map);
	    return BCM_E_PARAM;
	}


	/*
	 * common for all encapsulations
	 */
	sal_memcpy(ndChannelIngress.x_header.x_eth.a_source, config->header.eth.source, 6);
	sal_memcpy(ndChannelIngress.x_header.x_eth.a_destination, config->header.eth.destination, 6);

	ndChannelIngress.x_header.b_udp_chksum = config->header.udp_chksum;

	/*
	 * Make sure that required information is available for 
	 * the selected encapsulation type.
	 */
	switch(config->header.encapsulation)
	{
	case bcmCesEncapsulationEth:
	    ndChannelIngress.x_header.n_mpls_count = 0;
	    ndChannelIngress.x_header.n_l2tpv3_count = 0;
	    break;

	case bcmCesEncapsulationIp:
	    ndChannelIngress.x_header.n_mpls_count = 0;
	    ndChannelIngress.x_header.n_l2tpv3_count = 0;
	    break;

	case bcmCesEncapsulationMpls:
	    ndChannelIngress.x_header.n_l2tpv3_count = 0;

	    if (config->header.mpls_count == 0 ||
		config->header.mpls_count > BCM_CES_PROTO_MPLS_LABEL_MAX) {
		
                sal_free(map);
		return BCM_E_PARAM;
	    }

	    ndChannelIngress.x_header.n_mpls_count = config->header.mpls_count; 
	    for (i = 0;i < config->header.mpls_count;i++) {
		ndChannelIngress.x_header.a_mpls[i].n_label = config->header.mpls[i].label;
		ndChannelIngress.x_header.a_mpls[i].n_expiremental = config->header.mpls[i].experimental;
		ndChannelIngress.x_header.a_mpls[i].n_ttl = config->header.mpls[i].ttl;
	    }
	    break;

	case bcmCesEncapsulationL2tp:
	    ndChannelIngress.x_header.n_mpls_count = 0;
	    break;
	}

	/*
	 * If VLAN count greate than 0 then setup vlan
	 */
	if (config->header.vlan_count > 0) {
	    if (config->header.vlan_count > BCM_CES_PROTO_VLAN_TAG_MAX) {
		SOC_DEBUG_PRINT((DK_VERBOSE, "VLAN count out of range\n"));
		bcm_esw_ces_service_free_tdm(unit, *ces_service);
		if (!(flags & BCM_CES_WITH_ID))
		    bcm_ces_service_free_record(unit, *ces_service);
		CES_UNLOCK(unit);
                sal_free(map);
		return BCM_E_PARAM;
	    }

	    ndChannelIngress.x_header.n_vlan_count = config->header.vlan_count;

	    for (i = 0;i <  config->header.vlan_count;i++) {
		ndChannelIngress.x_header.a_vlan[i].n_priority = config->header.vlan[i].priority;
		ndChannelIngress.x_header.a_vlan[i].n_vid = config->header.vlan[i].vid;
	    }

	} else {
	    ndChannelIngress.x_header.n_vlan_count = 0;
	}

	/*
	 * VC Label
	 */
	ndChannelIngress.x_header.x_vc_label.n_label = config->header.vc_label;


	/*
	 * RTP 
	 */
	ndChannelIngress.x_header.b_rtp_exists = config->header.rtp_exists;
	ndChannelIngress.x_header.x_rtp.n_pt = config->header.rtp_pt;
	ndChannelIngress.x_header.x_rtp.n_ssrc = config->header.rtp_ssrc;
	if (config->egress_channel_config.rtp_exists != config->header.rtp_exists) {
	    config->egress_channel_config.rtp_exists = config->header.rtp_exists;
	    channelEgressChange = TRUE;
	}


	/*
	 * IP setup. 
	 */
	ndChannelIngress.x_header.n_ip_version = config->header.ip_version;

	if (config->header.encapsulation == bcmCesEncapsulationIp ||
	    config->header.encapsulation == bcmCesEncapsulationL2tp) {

	    switch (config->header.ip_version) {
	    case 4:
		ndChannelIngress.x_header.x_ip4.n_destination = config->header.ipv4.destination;
		ndChannelIngress.x_header.x_ip4.n_source      = config->header.ipv4.source;
		ndChannelIngress.x_header.x_ip4.n_tos         = config->header.ipv4.tos;
		ndChannelIngress.x_header.x_ip4.n_ttl         = config->header.ipv4.ttl;

		break;

	    case 6:
		ndChannelIngress.x_header.x_ip6.n_traffic_class = config->header.ipv6.traffic_class;
		ndChannelIngress.x_header.x_ip6.n_flow_label    = config->header.ipv6.flow_label;
		ndChannelIngress.x_header.x_ip6.n_hop_limit     = config->header.ipv6.hop_limit;

		sal_memcpy(ndChannelIngress.x_header.x_ip6.a_destination,
			   config->header.ipv6.destination,
			   sizeof(ip6_addr_t));
		sal_memcpy(ndChannelIngress.x_header.x_ip6.a_source,
			   config->header.ipv6.source,
			   sizeof(ip6_addr_t));
		break;

	    default:
		
                sal_free(map);
		return BCM_E_PARAM;
		break;
	    }

	    if (config->header.l2tpv3_count) {
		ndChannelIngress.x_header.n_l2tpv3_count = config->header.l2tpv3_count;
		ndChannelIngress.x_header.x_l2tpv3.b_udp_mode = config->header.l2tpv3.udp_mode;
		ndChannelIngress.x_header.x_l2tpv3.n_header = config->header.l2tpv3.header;
		ndChannelIngress.x_header.x_l2tpv3.n_session_local_id = config->header.l2tpv3.session_local_id;
		ndChannelIngress.x_header.x_l2tpv3.n_session_peer_id = config->header.l2tpv3.session_peer_id;
		ndChannelIngress.x_header.x_l2tpv3.n_local_cookie1 = config->header.l2tpv3.local_cookie1;
		ndChannelIngress.x_header.x_l2tpv3.n_local_cookie2 = config->header.l2tpv3.local_cookie2;
		ndChannelIngress.x_header.x_l2tpv3.n_peer_cookie1 = config->header.l2tpv3.peer_cookie1;
		ndChannelIngress.x_header.x_l2tpv3.n_peer_cookie2 = config->header.l2tpv3.peer_cookie2;
		
		if (config->header.l2tpv3.udp_mode) {
		    ndChannelIngress.x_header.x_udp.n_destination = config->header.udp.destination;
		    ndChannelIngress.x_header.x_udp.n_source      = config->header.udp.source;
		}
	    } else {
		ndChannelIngress.x_header.x_udp.n_destination = config->header.udp.destination;
		ndChannelIngress.x_header.x_udp.n_source      = config->header.udp.source;
	    }
	} else {
	    ndChannelIngress.x_header.x_udp.n_destination = config->header.udp.destination;
	    ndChannelIngress.x_header.x_udp.n_source      = config->header.udp.source;
	}


	ndChannelIngress.n_pbf_addr = 0x00;

	/*
	 * If the RTP header is included then enable timestamp generation
	 */
	sal_memset(&ndDcrConfig, 0, sizeof(AgNdMsgConfigNemoDcrConfigurations));
	tcr = BCM_CES_TIMESTAMP_CLOCK_RATE;
	scr = ces_ctrl->system_clock_rate;
	tcr_index = bcm_ces_get_tss_index(BCM_CES_TIMESTAMP_CLOCK_RATE);
	crc_index = bcm_ces_get_tss_index(ces_ctrl->common_ref_clock_rate);

	if (config->header.rtp_exists) {
            ndDcrConfig.b_ts_generation_enable  = TRUE;
	    ndDcrConfig.b_fast_phase_enable     = TRUE;
	    ndDcrConfig.b_err_correction_enable = bcm_ces_rtp_tss_table[crc_index][tcr_index].error_correction_enable;
	    ndDcrConfig.n_prime_ts_sample_period       = 0;
	    ndDcrConfig.n_rtp_tsx_ts_step_size         = bcm_ces_rtp_tss_table[crc_index][tcr_index].step_size;
            ndDcrConfig.n_rtp_tsx_fast_phase_step_size = (tcr/scr) * 256;
	    ndDcrConfig.n_rtp_tsx_error_corrrection_period = bcm_ces_rtp_tss_table[crc_index][tcr_index].error_correction_period;

	} else {
	    ndDcrConfig.b_ts_generation_enable  = FALSE;
	    ndDcrConfig.b_fast_phase_enable     = FALSE;
	    ndDcrConfig.b_err_correction_enable = FALSE;
	}

	n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_NEMO_DCR_CONFIGURATIONS, &ndDcrConfig);
	if (!AG_SUCCEEDED(n_ret))
	{
	    SOC_DEBUG_PRINT((DK_VERBOSE, "AG_ND_OPCODE_CONFIG_NEMO_DCR_CONFIGURATIONS failed\n"));
	    bcm_esw_ces_service_free_tdm(unit, *ces_service);
	    if (!(flags & BCM_CES_WITH_ID))
		bcm_ces_service_free_record(unit, *ces_service);
	    CES_UNLOCK(unit);
            sal_free(map);
	    return BCM_E_INTERNAL;
	}

	/*
	 * DCR common reference clock source selection.
	 *
	 * Fixed for now to use external clock input #1
	 */
	memset(&ndDcrClkSource, 0, sizeof(AgNdMsgConfigNemoDcrClkSource));
	ndDcrClkSource.e_clk_source = 0x00;
	ndDcrClkSource.n_tdm_port = 0;

	n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_NEMO_DCR_CLK_SOURCE, &ndDcrClkSource);
	if (!AG_SUCCEEDED(n_ret))
	{
	    SOC_DEBUG_PRINT((DK_VERBOSE, "AG_ND_OPCODE_CONFIG_NEMO_DCR_CLK_SOURCE failed\n"));
	    bcm_esw_ces_service_free_tdm(unit, *ces_service);
	    if (!(flags & BCM_CES_WITH_ID))
		bcm_ces_service_free_record(unit, *ces_service);
	    CES_UNLOCK(unit);
            sal_free(map);
	    return BCM_E_INTERNAL;
	}

	n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_CHANNEL_INGRESS, &ndChannelIngress);
	if (!AG_SUCCEEDED(n_ret))
	{
	    SOC_DEBUG_PRINT((DK_VERBOSE, "AG_ND_OPCODE_CONFIG_CHANNEL_INGRESS failed\n"));
	    bcm_esw_ces_service_free_tdm(unit, *ces_service);
	    if (!(flags & BCM_CES_WITH_ID))
		bcm_ces_service_free_record(unit, *ces_service);
	    CES_UNLOCK(unit);
            sal_free(map);
	    return BCM_E_INTERNAL;
	}


	/*
	 * Setup CAS header
	 *
	 * Clone the service header and then adjust the labels to +1 from 
	 * the service
	 *
	 */
	ndCasIngress.n_channel_id = *ces_service;
	sal_memcpy(&ndCasIngress.x_header, &ndChannelIngress.x_header, sizeof(AgNdPacketHeader));

	/*
	 * UDP checksum not required on CAS header
	 */
	ndCasIngress.x_header.b_udp_chksum = FALSE;

	switch(config->header.encapsulation)
	{
	case bcmCesEncapsulationEth:
	    ndCasIngress.x_header.x_vc_label.n_label = ndCasIngress.x_header.x_vc_label.n_label + 1;
	    break;

	case bcmCesEncapsulationIp:
	    ndCasIngress.x_header.x_udp.n_source = ndCasIngress.x_header.x_udp.n_source + 1;
	    ndCasIngress.x_header.x_udp.n_destination = ndCasIngress.x_header.x_udp.n_destination + 1;
	    break;

	case bcmCesEncapsulationMpls:
	    ndCasIngress.x_header.a_mpls[0].n_label = ndCasIngress.x_header.a_mpls[0].n_label + 1;
	    break;

	case bcmCesEncapsulationL2tp:
	    ndCasIngress.x_header.x_l2tpv3.n_session_local_id = ndCasIngress.x_header.x_l2tpv3.n_session_local_id + 1;
	    ndCasIngress.x_header.x_l2tpv3.n_session_peer_id = ndCasIngress.x_header.x_l2tpv3.n_session_peer_id + 1;
	    break;

	default:
	    break;
	}

	n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_CAS_CHANNEL_INGRESS, &ndCasIngress);
	if (!AG_SUCCEEDED(n_ret))
	{
	    SOC_DEBUG_PRINT((DK_VERBOSE, "AG_ND_OPCODE_CONFIG_CAS_CHANNEL_INGRESS failed\n"));
	    bcm_esw_ces_service_free_tdm(unit, *ces_service);
	    if (!(flags & BCM_CES_WITH_ID))
		bcm_ces_service_free_record(unit, *ces_service);
	    CES_UNLOCK(unit);
            sal_free(map);
	    return BCM_E_INTERNAL;
	}

    }

    /*
     * Channel Egress
     */
    if (channelEgressChange || strictDataChange) {
	sal_memset(&ndChannelEgress, 0, sizeof(AgNdMsgConfigChannelEgress));
	ndChannelEgress.n_channel_id    = (AgNdChannel) *ces_service;

	ndChannelEgress.n_channel_id    = (AgNdChannel) *ces_service;
	ndChannelEgress.n_packet_sync_selector = config->egress_channel_config.packet_sync_selector;
	ndChannelEgress.b_rtp_exists    = config->egress_channel_config.rtp_exists;

	if (config->egress_channel_map.slot[0] != 0)
	    ndChannelEgress.n_payload_size = config->egress_channel_map.size * config->egress_channel_config.frames_per_packet;
	else if (ctrl->protocol == bcmCesTdmProtocolE1)
	    ndChannelEgress.n_payload_size = BCM_CES_E1_SLOT_MAX * config->egress_channel_config.frames_per_packet;
	else
	    ndChannelEgress.n_payload_size = BCM_CES_T1_SLOT_MAX * config->egress_channel_config.frames_per_packet;

	ndChannelEgress.n_jbf_ring_size = config->egress_channel_config.jbf_ring_size;
	ndChannelEgress.n_jbf_win_size  = config->egress_channel_config.jbf_win_size;
	ndChannelEgress.n_jbf_bop       = config->egress_channel_config.jbf_bop;
	ndChannelEgress.b_drop_on_valid = AG_FALSE;
	ndChannelEgress.n_jbf_addr      = 0x00;

	bcm_ces_copy_header(&record->config.strict_data, &ndChannelEgress.x_strict_data); 

	/*
	 * If L2TP is being used then use IP encapsulation 
	 */
	if (config->header.encapsulation == bcmCesEncapsulationL2tp)
	    ndChannelEgress.x_strict_data.e_encapsulation = bcmCesEncapsulationIp;
	else
	    ndChannelEgress.x_strict_data.e_encapsulation = config->header.encapsulation;

	n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_CHANNEL_EGRESS, &ndChannelEgress);
	if (!AG_SUCCEEDED(n_ret))
	{
	    SOC_DEBUG_PRINT((DK_VERBOSE, "AG_ND_OPCODE_CONFIG_CHANNEL_EGRESS failed\n"));
	    bcm_esw_ces_service_free_tdm(unit, *ces_service);
	    if (!(flags & BCM_CES_WITH_ID))
		bcm_ces_service_free_record(unit, *ces_service);
	    CES_UNLOCK(unit);
            sal_free(map);
	    return BCM_E_INTERNAL;
	}
    }

    /*
     * Channelizer Ingress
     */
    if (channelizerIngressChange) {
	n_ret = bcm_esw_ces_service_config_channelizer(unit, record, &config->ingress_channel_map, AG_ND_PATH_INGRESS);
	if (n_ret != BCM_E_NONE)
	{
	    SOC_DEBUG_PRINT((DK_VERBOSE, "bcm_esw_ces_service_config_channelizer() failed\n"));
	    bcm_esw_ces_service_free_tdm(unit, *ces_service);
	    if (!(flags & BCM_CES_WITH_ID))
		bcm_ces_service_free_record(unit, *ces_service);
	    CES_UNLOCK(unit);
            sal_free(map);
	    return BCM_E_INTERNAL;
	}
    }

    /*
     * Channelizer Egress
     */
    if (channelizerEgressChange) {
	n_ret = bcm_esw_ces_service_config_channelizer(unit, record, &config->egress_channel_map, AG_ND_PATH_EGRESS);
	if (n_ret != BCM_E_NONE)
	{
	    SOC_DEBUG_PRINT((DK_VERBOSE, "bcm_esw_ces_service_config_channelizer() failed\n"));
	    bcm_esw_ces_service_free_tdm(unit, *ces_service);
	    if (!(flags & BCM_CES_WITH_ID))
		bcm_ces_service_free_record(unit, *ces_service);
	    CES_UNLOCK(unit);
            sal_free(map);
	    return BCM_E_INTERNAL;
	}
    }

    /*
     * If this is a modification request and the channel 
     * was enabled then it must be reenabled.
     */
    if (enabled)
	bcm_esw_ces_service_enable_set(unit, *ces_service, 1);

	
    /*
     * Mark as configured
     */
    BCM_CES_SET_CONFIGURED(&record->config);
    BCM_CES_SET_CONFIGURED(record->tdmRecord);
    CES_UNLOCK(unit);

    sal_free(map);
    return BCM_E_NONE; 
}

/**
 * Function:
 *      bcm_esw_ces_service_enable_set
 * Purpose:
 *      Enables or disables a CES service.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service - (IN) CES Channel
 *      enable - (IN) Boolean 1==enable, 0==disable
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_BADID Invalid service ID was used
 *      BCM_E_INTERNAL An internal error occurred
 * Notes:
 */
int 
bcm_esw_ces_service_enable_set(
    int unit, 
    bcm_ces_service_t ces_service, 
    int enable)
{
    int n_ret;
    bcm_ces_service_record_t *record;
    int i;
    AgNdMsgConfigCircuitEnable     ndCircuitEnable;
    AgNdMsgConfigChannelEnable     ndChannelEnable;
    AgNdMsgConfigRpcMapLabelToChid ndMap;
    AgFramerConfig                 ndFramer;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;
    int16 modPorts[BCM_CES_SLOT_MAX];
    int j;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
#if defined(BCM_CES_CRM)
    uint16 reply_len;
#endif
    bcm_ces_crm_rclock_config_msg_t rclock_msg;
    bcm_ces_rclock_config_t sconfig, *config = &sconfig;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_UNAVAIL;

    sal_memset(modPorts, -1, sizeof(modPorts));

    
/*    soc_cm_print("Enable/Disable S:%d E:%d\n", ces_service, enable);*/
    CES_LOCK(unit);

    /*
     * Set enable state for each circuit and framer. The port may have 
     * multiple entries in the map so make sure that only the first one
     * has the enable state changed and ignore the others.
     */
    for (i = record->config.ingress_channel_map.first; i < (record->config.ingress_channel_map.first + record->config.ingress_channel_map.size);i++)
    {
	/*
	 * Has the port state already been modified?
	 */
	j = 0;
	while(modPorts[j] != -1 && j < BCM_CES_SLOT_MAX) {
	    if (modPorts[j] == (int16)record->config.ingress_channel_map.circuit_id[i])
		break;
	    j++;
	}

        if (j < BCM_CES_SLOT_MAX) {
            if (modPorts[j] != (int16)record->config.ingress_channel_map.circuit_id[i]) {
                sal_memset(&ndFramer, 0, sizeof(AgFramerConfig));
                ndFramer.n_circuit_id = (AgNdCircuit)bcm_ces_port_to_circuit_id(unit, record->config.ingress_channel_map.circuit_id[i]);
                n_ret = ag_nd_device_read(ctrl->ndHandle, AG_ND_OPCODE_FRAMER_PORT_CONFIG, &ndFramer);
                if (!AG_SUCCEEDED(n_ret)) {
                    soc_cm_print("%s: 1\n", __func__);
                    CES_UNLOCK(unit);
                    return BCM_E_INTERNAL;
                }

                ndFramer.b_enable = enable;
                n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_FRAMER_PORT_CONFIG, &ndFramer);
                if (!AG_SUCCEEDED(n_ret))
                {
                    soc_cm_print("%s: 2\n", __func__);
                    CES_UNLOCK(unit);
                    return BCM_E_INTERNAL;
                }

                ndCircuitEnable.b_enable = enable;
                ndCircuitEnable.n_circuit_id = (AgNdCircuit)bcm_ces_port_to_circuit_id(unit, record->config.ingress_channel_map.circuit_id[i]);
                n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_CIRCUIT_ENABLE, &ndCircuitEnable);
                if (!AG_SUCCEEDED(n_ret))
                {
                    soc_cm_print("%s: 3\n", __func__);
                    CES_UNLOCK(unit);
                    return BCM_E_INTERNAL;
                }

                modPorts[j] = (int16)record->config.ingress_channel_map.circuit_id[i];
            }
        }
    }

    sal_memset(&ndMap, 0, sizeof(AgNdMsgConfigRpcMapLabelToChid));
    ndMap.e_encapsulation = record->config.header.encapsulation;
    ndMap.b_enable = enable;
    ndMap.n_channel_id = (AgNdChannel) ces_service;

    /*
     * Set the label from the appropriate field
     */
    switch(ndMap.e_encapsulation)
    {
    case AG_ND_ENCAPSULATION_ETH:
	ndMap.n_label = record->config.header.vc_label;
	break;

    case AG_ND_ENCAPSULATION_IP:
	if (record->config.header.l2tpv3_count) {
	    ndMap.n_label = record->config.header.l2tpv3.session_local_id;
	    ndMap.e_encapsulation = AG_ND_ENCAPSULATION_L2TP;
	} else {
	    ndMap.n_label = record->config.header.udp.destination;
	}
	break;

    case AG_ND_ENCAPSULATION_MPLS:
	ndMap.n_label = record->config.header.mpls[0].label;
	break;

    case AG_ND_ENCAPSULATION_L2TP:
	ndMap.n_label = record->config.header.l2tpv3.session_local_id;
	break;

    default:
	/* Not handled */
    case AG_ND_ENCAPSULATION_PTP_IP:
    case AG_ND_ENCAPSULATION_PTP_EHT:
    case AG_ND_ENCAPSULATION_MAX:
	break;
    }
 
    /*
     * Enable classifier before channels are enabled.
     */
    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_RPC_MAP_LABEL_TO_CHID, &ndMap);
    if (!AG_SUCCEEDED(n_ret))
    {
#if 0
	soc_cm_print("%s:4 Service:%2d Encap:%d Label:%04lx Enable:%lu ret:0x%08x\n", 
		     __func__,
		     ndMap.n_channel_id,
		     ndMap.e_encapsulation,
		     ndMap.n_label,
		     ndMap.b_enable,
		     n_ret);
#endif
	CES_UNLOCK(unit);
	return BCM_E_INTERNAL;
    }

    ndChannelEnable.b_enable = enable;
    ndChannelEnable.n_channel_id = (AgNdChannel) ces_service;
    ndChannelEnable.e_path = AG_ND_PATH_EGRESS;
    ndChannelEnable.n_jb_size_milli = 5;

    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_CHANNEL_ENABLE, &ndChannelEnable);
    if (!AG_SUCCEEDED(n_ret))
    {
	CES_UNLOCK(unit);
	return BCM_E_INTERNAL;
    }

    ndChannelEnable.b_enable = enable;
    ndChannelEnable.n_channel_id = (AgNdChannel) ces_service;
    ndChannelEnable.e_path = AG_ND_PATH_INGRESS;
    ndChannelEnable.n_jb_size_milli = 5;

    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_CHANNEL_ENABLE, &ndChannelEnable);
    if (!AG_SUCCEEDED(n_ret))
    {
	CES_UNLOCK(unit);
	return BCM_E_INTERNAL;
    }


    /*
     * Set enabled flag appropriately
     */
    if (enable)
    {
	BCM_CES_SET_ENABLED(&record->config);
	BCM_CES_SET_ENABLED(record->tdmRecord);
    }
    else
    {
	BCM_CES_CLR_ENABLED(&record->config);
	BCM_CES_CLR_ENABLED(record->tdmRecord);
    }


    CES_UNLOCK(unit);

    /*
     * CRM enable disable this session if required
     */
    {
#if defined(BCM_CES_CRM)
        /*If rclock available, set it*/
        n_ret = bcm_esw_ces_service_rclock_config_get(unit,ces_service,config);   /*The message in not set from CLI*/
        if (n_ret == BCM_E_NONE)
#endif        
        {
            /*Rclock found*/
            sal_memset(&rclock_msg, 0, sizeof(bcm_ces_crm_rclock_config_msg_t));
            
            rclock_msg.flags         = 0x00;
            /*From rcr config*/
            rclock_msg.b_enable      = config->enable;
            rclock_msg.output_brg    = config->output_brg;
            rclock_msg.rclock        = config->rclock;
            rclock_msg.port          = bcm_ces_port_to_circuit_id(unit, record->tdmRecord->port);
            rclock_msg.recovery_type = config->recovery_type;
            
            /*From service*/
            
            rclock_msg.e_protocol    = ctrl->protocol;
            rclock_msg.b_structured  = record->tdmRecord->config.b_structured;
            rclock_msg.service       = ces_service;

	    if (ctrl->protocol == bcmCesTdmProtocolT1) {
		/* 
		 * T1
		 */
		if (record->tdmRecord->config.b_structured) 
		    rclock_msg.tdm_clocks_per_packet = soc_htons(record->config.egress_channel_config.frames_per_packet * 193);
		else
		    rclock_msg.tdm_clocks_per_packet = soc_htons(record->config.egress_channel_config.frames_per_packet * 192);
	    } else {
		/*
		 * E1
		 */
		rclock_msg.tdm_clocks_per_packet = soc_htons(record->config.egress_channel_config.frames_per_packet * 256);
	    }

            /*If this is a service disabled, stop recovery*/
            if(!enable)
		rclock_msg.b_enable      = enable;
#if 0      
            soc_cm_print("%s: rclock config flags=%x enbale=%d  t1-e1=%x struct=%x brgout=%x rclock=%x port=%x recovery=%x service=%x \n",
			 __FUNCTION__,
			 rclock_msg.flags,
			 rclock_msg.b_enable,
			 rclock_msg.e_protocol,
			 rclock_msg.b_structured,
			 rclock_msg.output_brg,
			 rclock_msg.rclock,
			 rclock_msg.port,
			 rclock_msg.recovery_type,
			 rclock_msg.service);
#endif        
        
#if defined(BCM_CES_CRM)
            n_ret =  bcm_esw_ces_crm_dma_msg_send_receive(unit, 
							  MOS_MSG_SUBCLASS_CES_CRM_RCLOCK_CONFIG, 
							  (uint8*)&rclock_msg, 
							  sizeof(bcm_ces_crm_rclock_config_msg_t),
							  MOS_MSG_SUBCLASS_CES_CRM_RCLOCK_CONFIG_REPLY,
							  &reply_len);
#endif
        }
    }

    return BCM_E_NONE; 
}

/**
 * Function:
 *      bcm_esw_ces_service_free_tdm
 * Purpose:
 *      Free TDM resources associated with a service
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service - (IN) CES Channel
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_service_free_tdm(
    int unit, 
    bcm_ces_service_t ces_service)
{
    bcm_ces_service_record_t *record;
    int n_ret;
    bcm_ces_channel_map_t *map;


    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_UNAVAIL;

    map = (bcm_ces_channel_map_t*)sal_alloc(sizeof(bcm_ces_channel_map_t), "CES Channel map");
    if (map == NULL)
        return BCM_E_MEMORY;


    /*
     * Free the TDM resources
     */
    if (record->tdmRecord != NULL) {
	/*
	 * Provision channel with no TDM
	 */
	sal_memset(map, 0, sizeof(bcm_ces_channel_map_t));
	n_ret = bcm_esw_ces_service_config_channelizer(unit, record, map, AG_ND_PATH_INGRESS);
	n_ret = bcm_esw_ces_service_config_channelizer(unit, record, map, AG_ND_PATH_EGRESS);

	bcm_esw_port_tdm_delete_service(unit, record->tdmRecord, record);
	record->tdmRecord = NULL;
    }

    sal_free(map);
    return BCM_E_NONE; 
}


/**
 * Function:
 *      bcm_esw_ces_service_destroy
 * Purpose:
 *      Deallocate a CES service.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service - (IN) CES Channel
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_BADID Invalid service ID was used
 *      BCM_E_INTERNAL An internal error occurred
 * Notes:
 */
int 
bcm_esw_ces_service_destroy(
    int unit, 
    bcm_ces_service_t ces_service)
{
    bcm_ces_service_record_t *record;
    bcm_ces_rclock_config_t   rclock_config;
    int n_ret;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_BADID;

    CES_LOCK(unit);

    /*
     * Disable
     */
    if (BCM_CES_IS_ENABLED(&record->config)) {
	bcm_esw_ces_service_enable_set(unit, ces_service, 0);
    }


    /*
     * If there is an associated rclock record then disable that also
     */
    if (record->rclock) {
	bcm_esw_ces_service_rclock_config_get(unit, ces_service, &rclock_config);
	rclock_config.enable = 0;
	bcm_esw_ces_service_rclock_config_set(unit, ces_service, &rclock_config);
	record->rclock->service = NULL;
	record->rclock = NULL;
    }

    /*
     * Free the TDM resources
     */
    bcm_esw_ces_service_free_tdm(unit, ces_service);

    /*
     * Reset flags and mark as free
     */
    BCM_CES_CLR_ALL(&record->config);
    BCM_CES_SET_FREE(&record->config);
    CES_UNLOCK(unit);

    return BCM_E_NONE; 
}

/**
 * Function:
 *      bcm_esw_ces_service_destroy_all
 * Purpose:
 *      Deallocate all CES services.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_service_destroy_all(
    int unit)
{
    int i;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Find all allocated circuits and free them
     */
    for (i = 0;i < BCM_CES_CIRCUIT_IDX_MAX;i++)
    {
	if (ces_ctrl->bcm_ces_service_records[i] != NULL)
	{
	    bcm_esw_ces_service_destroy(unit,  (bcm_ces_service_t)i);
	}
    }

    return BCM_E_NONE;
}


/**
 * Function:
 *      bcm_esw_ces_service_enable_get
 * Purpose:
 *      Get CES service enable state.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service - (IN) CES Channel
 *      enable - (OUT) Boolean 1 == enabled, 0 == disabled
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_BADID Invalid service ID was used
 *      BCM_E_INTERNAL An internal error occurred
 * Notes:
 */
int 
bcm_esw_ces_service_enable_get(
    int unit, 
    bcm_ces_service_t ces_service, 
    int *enable)
{
    int n_ret;
    bcm_ces_service_record_t *record;
bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_UNAVAIL;

    *enable = (BCM_CES_IS_ENABLED(&record->config) ? 1:0);
    return BCM_E_NONE; 
}


/**
 * Function:
 *      bcm_esw_ces_service_config_get
 * Purpose:
 *      Get the current configuration parameters for a CES service.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service - (IN) CES Channel
 *      config - (OUT) Service config.
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 *      BCM_E_BADID Invalid service ID was used
 *      BCM_E_INTERNAL An internal error occurred
 * Notes:
 */
int 
bcm_esw_ces_service_config_get(
    int unit, 
    bcm_ces_service_t ces_service, 
    bcm_ces_service_config_t *config)
{
    int n_ret;
    bcm_ces_service_record_t *record;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    sal_memcpy(config, &record->config, sizeof(bcm_ces_service_config_t));
    return BCM_E_NONE; 
}

/**
 * Function:
 *      bcm_esw_ces_services_traverse
 * Purpose:
 *      List all currently configured CES services.
 * Parameters:
 *      unit - (IN) Unit number.
 *      cb - (IN) Call back function pointer
 *      user_data - (IN) User data
 * Returns:
 *      BCM_E_NONE Operation completed successfully
 * Notes:
 */
int 
bcm_esw_ces_services_traverse(
    int unit, 
    bcm_ces_service_traverse_cb cb, 
    void *user_data)
{
    int i;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);


    for (i = 0;i < BCM_CES_CIRCUIT_IDX_MAX;i++)
    {
	if (ces_ctrl->bcm_ces_service_records[i] != NULL)
	{
	    cb(unit, (void*)&ces_ctrl->bcm_ces_service_records[i]->config);
	}
    }

    return BCM_E_NONE; 
}


/**
 * Function:
 *      bcm_esw_ces_service_ingress_cas_enable_set
 * Purpose:
 *      Enable or disable CAS signalling for a CES service.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service - (IN) CES Channel
 *      enable - (IN) Enable
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_service_ingress_cas_enable_set(
    int unit, 
    bcm_ces_service_t ces_service, 
    int enable)
{
    int n_ret;
    bcm_ces_service_record_t *record;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;
    AgNdMsgConfigCasChannelEnable ndCasEnable;
    AgNdMsgConfigRpcMapLabelToChid ndMap;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_UNAVAIL;

    /*
     * Set mapping
     */
    sal_memset(&ndMap, 0, sizeof(AgNdMsgConfigRpcMapLabelToChid));
    ndMap.e_encapsulation = record->config.header.encapsulation;
    ndMap.b_enable = enable;
    ndMap.n_channel_id = (AgNdChannel) ces_service;

    /*
     * Set the label from the appropriate field
     */
    switch(ndMap.e_encapsulation)
    {
    case AG_ND_ENCAPSULATION_ETH:
	ndMap.n_label = record->config.header.vc_label + 1;
	break;

    case AG_ND_ENCAPSULATION_IP:
	if (record->config.header.l2tpv3_count) {
	    ndMap.n_label = record->config.header.l2tpv3.session_local_id + 1;
	    ndMap.e_encapsulation = AG_ND_ENCAPSULATION_L2TP;
	} else {
	    ndMap.n_label = record->config.header.udp.destination + 1;
	}
	break;

    case AG_ND_ENCAPSULATION_MPLS:
	ndMap.n_label = record->config.header.mpls[0].label + 1;
	break;

    case AG_ND_ENCAPSULATION_L2TP:
	ndMap.n_label = record->config.header.l2tpv3.session_local_id + 1;
	break;

    default:
	/* Not handled */
    case AG_ND_ENCAPSULATION_PTP_IP:
    case AG_ND_ENCAPSULATION_PTP_EHT:
    case AG_ND_ENCAPSULATION_MAX:
	break;
    }

    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_RPC_MAP_LABEL_TO_CHID, &ndMap);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    ndCasEnable.b_enable = (enable ? TRUE:FALSE);
    ndCasEnable.n_channel_id = ces_service;
    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_CAS_CHANNEL_ENABLE, &ndCasEnable);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    return BCM_E_NONE; 
}


/**
 * Function:
 *      bcm_esw_ces_service_ingress_cas_enable_get
 * Purpose:
 *      Get the CAS signalling enable status for a CES service.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service - (IN) CES Channel
 *      enable - (OUT) Enable state
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_service_ingress_cas_enable_get(
    int unit, 
    bcm_ces_service_t ces_service, 
    int *enable)
{
    int n_ret;
    bcm_ces_service_record_t *record;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;
    AgNdMsgConfigCasChannelEnable ndCasEnable;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_UNAVAIL;

    ndCasEnable.n_channel_id = ces_service;
    n_ret = ag_nd_device_read(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_CAS_CHANNEL_ENABLE, &ndCasEnable);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    *enable = ndCasEnable.b_enable;
    return BCM_E_NONE; 
}


/**
 * Function:
 *      bcm_esw_ces_service_cas_tx
 * Purpose:
 *      Number of CAS packets to schedule for transmission on a CES
 *      service.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service - (IN) CES Channel
 *      packet_count - (IN) Number of CAS packets to schedule
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_service_cas_packet_enable(
    int unit, 
    bcm_ces_service_t ces_service, 
    bcm_ces_cas_packet_control_t *packet_control)
{
    int n_ret;
    bcm_ces_service_record_t *record;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;
    AgNdMsgCommandCasChannelTx ndCasTx;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_UNAVAIL;

    ndCasTx.n_channel_id = ces_service;
    ndCasTx.n_packets = packet_control->packet_count;
    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_COMMAND_CAS_CHANNEL_TX, &ndCasTx);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    return BCM_E_NONE; 
}


/**
 * Function:
 *      bcm_esw_ces_egress_status_get
 * Purpose:
 *      Gets the CES seervice egress status.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service - (IN) CES Channel
 *      sync_state - (OUT) Sync state
 *      jbf_state - (OUT) Jitter buffer state
 *      ces_cw - (OUT) CES cw
 *      trimming - (OUT) Trimming state
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_egress_status_get(
    int unit, 
    bcm_ces_service_t ces_service, 
    bcm_ces_service_egress_status_t *status)
{
    AgResult n_ret;
    AgNdMsgStatusEgress ndStatusEgress;
    bcm_ces_service_record_t *record;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_UNAVAIL;

    ndStatusEgress.n_channel_id = ces_service;
    n_ret = ag_nd_device_read(ctrl->ndHandle, AG_ND_OPCODE_STATUS_EGRESS, &ndStatusEgress);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    status->sync_state = ndStatusEgress.e_sync_state;
    status->jbf_state  = ndStatusEgress.e_jbf_state;
    status->ces_cw     = ndStatusEgress.n_ces_cw;
    status->trimming   = ndStatusEgress.b_trimming;

    return BCM_E_NONE; 
}


/**
 * Function:
 *      bcm_ces_services_cclk_config_set
 * Purpose:
 *      Set the clock configuration parameters that are common to all
 *      CES services.
 * Parameters:
 *      unit - (IN) Unit number.
 *      config - (IN) config
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_services_cclk_config_set(
    int unit, 
    bcm_ces_cclk_config_t *config)
{
    AgResult n_ret;
    AgNdMsgConfigNemoCClk ndConfigNemoCClk;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_crm_cclk_config_msg_t crm_msg;
#if defined(BCM_CES_CRM)
    uint16 reply_len;
#endif
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    int circuit_id;

    CES_CHECK_INIT(unit);

    config->ref_clk_1_brg = 0; /* Not used */
    config->ref_clk_2_brg = 0; /* Not used */
    config->ext_clk_1_dir = 0; /* Must be zero */
    config->ext_clk_2_dir = 1; /* Must be one */
    config->ref_clk_proto = ctrl->protocol;

    if (config->cclk_enable < 0 ||
	config->cclk_enable > 1)
	return BCM_E_PARAM;

    if (config->cclk_select < bcmCesCclkSelectRefClk1 ||
	config->cclk_select > bcmCesCclkSelectExternalClk)
	return BCM_E_PARAM;

    if (config->ref_clk_1_select < bcmCesRefClkSelectRclk ||
	config->ref_clk_1_select > bcmCesRefClkSelectPtp)
	return BCM_E_PARAM;

    if (config->ref_clk_2_select < bcmCesRefClkSelectRclk ||
	config->ref_clk_2_select > bcmCesRefClkSelectPtp)
	return BCM_E_PARAM;

    circuit_id = bcm_ces_port_to_circuit_id(unit, config->ref_clk_1_port);

    if (circuit_id >= BCM_CES_TDM_MAX || circuit_id < 0) 
	return BCM_E_PARAM; 

    circuit_id = bcm_ces_port_to_circuit_id(unit, config->ref_clk_2_port);

    if (circuit_id >= BCM_CES_TDM_MAX || circuit_id < 0) 
	return BCM_E_PARAM; 


    /*
     * Save config
     */
    sal_memcpy(&ctrl->cclk_config, config, sizeof(bcm_ces_cclk_config_t));

    
    

    /*
     * Make message
     */
    sal_memset(&ndConfigNemoCClk, 0, sizeof(AgNdMsgConfigNemoCClk));
    ndConfigNemoCClk.b_cclk_enable = ctrl->cclk_config.cclk_enable;
    ndConfigNemoCClk.e_cclk_select = ctrl->cclk_config.cclk_select;
    ndConfigNemoCClk.e_ref_clk_proto = ctrl->cclk_config.ref_clk_proto;
    ndConfigNemoCClk.e_ref_clk_1_select = ctrl->cclk_config.ref_clk_1_select;
    ndConfigNemoCClk.e_ref_clk_2_select = ctrl->cclk_config.ref_clk_2_select;
    ndConfigNemoCClk.n_ref_clk_1_port = bcm_ces_port_to_circuit_id(unit, ctrl->cclk_config.ref_clk_1_port);
    ndConfigNemoCClk.n_ref_clk_2_port = bcm_ces_port_to_circuit_id(unit, ctrl->cclk_config.ref_clk_2_port);
    ndConfigNemoCClk.n_ref_clk_1_brg = ctrl->cclk_config.ref_clk_1_brg;
    ndConfigNemoCClk.n_ref_clk_2_brg = ctrl->cclk_config.ref_clk_2_brg;
    ndConfigNemoCClk.e_ext_clk_1_dir = ctrl->cclk_config.ext_clk_1_dir;
    ndConfigNemoCClk.e_ext_clk_2_dir = ctrl->cclk_config.ext_clk_2_dir;
    ndConfigNemoCClk.e_ref_clk_1_ptp = AG_ND_PTP_CLK_1;
    ndConfigNemoCClk.e_ref_clk_2_ptp = AG_ND_PTP_CLK_2;

    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_NEMO_CCLK, &ndConfigNemoCClk);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    /*
     * Inform CRM of the clock configuration
     */
    sal_memset(&crm_msg, 0, sizeof(bcm_ces_crm_cclk_config_msg_t));
    crm_msg.flags = 0x00;
    crm_msg.e_cclk_select      = ctrl->cclk_config.cclk_select;
    crm_msg.e_ref_clk_1_select = ctrl->cclk_config.ref_clk_1_select;
    crm_msg.e_ref_clk_2_select = ctrl->cclk_config.ref_clk_2_select;
    crm_msg.n_ref_clk_1_port   = soc_htonl(ctrl->cclk_config.ref_clk_1_port);
    crm_msg.n_ref_clk_2_port   = soc_htonl(ctrl->cclk_config.ref_clk_2_port);
    crm_msg.e_protocol         = ctrl->protocol;
#if defined(BCM_CES_CRM)
    n_ret =  bcm_esw_ces_crm_dma_msg_send_receive(unit, 
						  MOS_MSG_SUBCLASS_CES_CRM_CONFIG, 
						  (uint8*)&crm_msg, sizeof(bcm_ces_crm_cclk_config_msg_t),
						  MOS_MSG_SUBCLASS_CES_CRM_CONFIG_REPLY,
						  &reply_len);
#endif

    return n_ret; 
}




/**
 * Function:
 *      bcm_ces_services_cclk_config_Get
 * Purpose:
 *      Get the clock configuration parameters that are common to all
 *      CES services.
 * Parameters:
 *      unit - (IN) Unit number.
 *      config - (OUT) config
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_services_cclk_config_get(
    int unit, 
    bcm_ces_cclk_config_t *config)
{
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Make config
     */
    sal_memcpy(config, &ctrl->cclk_config, sizeof(bcm_ces_cclk_config_t));

    return BCM_E_NONE; 
}


/**
 * Function:
 *      bcm_ces_service_rclock_config_set
 * Purpose:
 *      Set the rclock timing configuration parameters for a CES
 *      service.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service - (IN) CES Channel
 *      config - (IN) config
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_service_rclock_config_set(
    int unit, 
    bcm_ces_service_t ces_service, 
    bcm_ces_rclock_config_t *config)
{
    AgResult n_ret;
    bcm_ces_service_record_t *record;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_rclock_record_t *rclock;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    int circuit_id;

    CES_CHECK_INIT(unit);

    /*
     * Find rclock instance
     */
    if (config->rclock >= BCM_CES_RCLOCK_MAX)
	return BCM_E_BADID;

    rclock = &ctrl->rclock_record[config->rclock];

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    /*
     * If there is no TDM associated with the service then there
     * is no service...
     */
    if (record->tdmRecord == NULL)
	return BCM_E_INTERNAL;

    /*
     * Validate config parameters
     */
    if (config->enable < 0 || config->enable > 1)
	return BCM_E_PARAM;

    if (config->output_brg > 2)
	return BCM_E_PARAM;

    circuit_id = bcm_ces_port_to_circuit_id(unit, config->port);

    if (circuit_id >= BCM_CES_TDM_MAX || circuit_id < 0) 
	return BCM_E_PARAM;

    if (config->recovery_type > 1)
	return BCM_E_PARAM;

    /*
     * If the current service already has a different rclock instance associated
     * with it and that instance is enabled then fail.
     */
    if (record->rclock != NULL && record->rclock != rclock) {
	if (record->rclock->config.enable)
	    return BCM_E_BADID;

	record->rclock->service = NULL;
    }

    /*
     * If the new service is different to the old service and the 
     * rclock is disabled then disconnect the old before adding the new,
     * else fail.
     */
    if (record != rclock->service && rclock->service != NULL) {
	if (rclock->config.enable)
	    return BCM_E_BADID;

	rclock->service->rclock = NULL;
    }

    rclock->service = record;
    record->rclock = rclock;

    /*
     * Save a copy
     */
    sal_memcpy(&rclock->config, config, sizeof(bcm_ces_rclock_config_t));

    /*
     * If service is enabled then disable and re-enable the service to apply changes.
     */
    if (BCM_CES_IS_ENABLED(&record->config)) {
	bcm_esw_ces_service_enable_set(unit, ces_service, FALSE);
	bcm_esw_ces_service_enable_set(unit, ces_service, TRUE);
    }

    return BCM_E_NONE;  
}


/*
 * Function:
 *      bcm_ces_service_rclock_config_get
 * Purpose:
 *      Get the rclock configuration parameters for a CES
 *      service.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ces_service - (IN) CES Channel
 *      config - (OUT) config
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_service_rclock_config_get(
    int unit, 
    bcm_ces_service_t ces_service, 
    bcm_ces_rclock_config_t *config)
{
    AgResult n_ret;
    bcm_ces_service_record_t *record;
    bcm_ces_rclock_record_t *rclock;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_BADID;

    /*
     * If there is no valid rclock associated then return an error
     */
    if (record->rclock == NULL)
	return BCM_E_BADID;

    rclock =  record->rclock;

    /*
     * Save a copy
     */
    sal_memcpy(config, &rclock->config, sizeof(bcm_ces_rclock_config_t));

#if 0
    soc_cm_print("%s: rclock record config: enbale=%d   outputbrg=%x rclock=%x port=%x recovery=%x service=%x \n",__FUNCTION__,
    rclock->config.enable,
    rclock->config.output_brg,
    rclock->config.rclock,
    rclock->config.port,
    rclock->config.recovery_type,
    ces_service);


    if(config->enable)
    {
      _bcm_esw_ces_rclock_status_get(unit,config->rclock); 
    }
#endif

    return BCM_E_NONE; 
}

/**
 * Function:
 *      _bcm_esw_ces_rclock_status_get()
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit - (IN) Unit number.
 *      config - (OUT) config
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int _bcm_esw_ces_rclock_status_get(int unit, int rcr_index, bcm_ces_crm_status_msg_t *ces_rcr_status) {
    int res = BCM_E_NONE;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    uint8 *buffer;
    int temp_state;
    uint16 reply_len;

    res =  bcm_esw_ces_crm_dma_msg_send_receive(unit, 
						MOS_MSG_SUBCLASS_CES_CRM_STATUS,
						(uint8*)&ces_rcr_status,
						rcr_index,
						MOS_MSG_SUBCLASS_CES_CRM_STATUS_REPLY,
						&reply_len);

    if (res == BCM_E_NONE) {
	buffer = ces_ctrl->dma_buffer;
	bcm_ces_crm_status_msg_unpack(buffer, ces_rcr_status);

	temp_state = ces_rcr_status->clock_state;
	if( (temp_state < 0) || (temp_state > RCR_STATE_FAST_ACQUISITION) )
	    temp_state = RCR_STATE_FAST_ACQUISITION +1;
#if 0
	soc_cm_print("\nRcr=%d: state %d-%s , seconds: active %d , locked %d  ,  freq: %d.%d\n\n", 
		     ces_rcr_status.rclock,
		     ces_rcr_status.clock_state,
		     ces_rcr_AgRcrStatesMsg[temp_state],
		     ces_rcr_status.seconds_active,
		     ces_rcr_status.seconds_locked,
		     ces_rcr_status.calculated_frequency_w,
		     ces_rcr_status.calculated_frequency_f);
#endif
    }

    return res;
}


/**
 * Function:
 *      bcm_ces_diag_get()
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit - (IN) Unit number.
 *      config - (OUT) config
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_ces_diag_get(
    int unit, 
    AgNdMsgDiag *config)
{
    AgResult n_ret;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;

    n_ret = ag_nd_device_read(ctrl->ndHandle, AG_ND_OPCODE_DIAG, config);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    return BCM_E_NONE; 
}


/**
 * Function:
 *      bcm_ces_diag_set()
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit - (IN) Unit number.
 *      config - (OUT) config
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_ces_diag_set(
    int unit, 
    AgNdMsgDiag *config)
{
    AgResult n_ret;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;

    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_DIAG, config);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    return BCM_E_NONE; 
}

/**
 * Function:
 *      bcm_esw_ces_attach_ethernet()
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit - (IN) Unit number.
 *      config - (OUT) config
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_attach_ethernet(int unit, int port) {

    

    /*
     * Enable CES MII and attach to pipeline
     */

    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_esw_ces_ethernet_config_set
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit - (IN) Unit number.
 *      port - (IN) Port number
 *      config - (IN) config
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_ethernet_config_set(int unit, int port, bcm_ces_mac_cmd_config_t *config) {
    AgResult n_ret;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;

    ctrl->ndMac.x_command_config.b_rx_error_discard_enable          = config->b_rx_error_discard_enable;
    ctrl->ndMac.x_command_config.b_no_length_check                  = config->b_no_length_check;      
    ctrl->ndMac.x_command_config.b_control_frame_enable             = config->b_control_frame_enable; 
    ctrl->ndMac.x_command_config.b_node_wake_up_request_indication  = config->b_node_wake_up_request_indication;
    ctrl->ndMac.x_command_config.b_put_core_in_sleep_mode           = config->b_put_core_in_sleep_mode;       
    ctrl->ndMac.x_command_config.b_enable_magic_packet_detection    = config->b_enable_magic_packet_detection;
    ctrl->ndMac.x_command_config.b_software_reset                   = config->b_software_reset;               
    ctrl->ndMac.x_command_config.b_is_late_collision_condition      = config->b_is_late_collision_condition;  
    ctrl->ndMac.x_command_config.b_is_excessive_collision_condition = config->b_is_excessive_collision_condition; 
    ctrl->ndMac.x_command_config.b_enable_half_duplex               = config->b_enable_half_duplex;           
    ctrl->ndMac.x_command_config.b_insert_mac_addr_on_transmit      = config->b_insert_mac_addr_on_transmit;  
    ctrl->ndMac.x_command_config.b_ignore_pause_frame_quanta        = config->b_ignore_pause_frame_quanta;    
    ctrl->ndMac.x_command_config.b_fwd_pause_frames                 = config->b_fwd_pause_frames;             
    ctrl->ndMac.x_command_config.b_fwd_crc_field                    = config->b_fwd_crc_field;                
    ctrl->ndMac.x_command_config.b_enable_frame_padding             = config->b_enable_frame_padding;         
    ctrl->ndMac.x_command_config.b_enable_promiscuous_mode          = config->b_enable_promiscuous_mode;      
    ctrl->ndMac.x_command_config.b_enable_mac_receive               = config->b_enable_mac_receive;           
    ctrl->ndMac.x_command_config.b_enable_mac_transmit              = config->b_enable_mac_transmit; 

    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_MAC, &ctrl->ndMac);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_esw_ces_ethernet_config_get
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit - (IN) Unit number.
 *      port - (IN) Port number
 *      config - (OUT) config
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_ethernet_config_get(int unit, int port, bcm_ces_mac_cmd_config_t *config) {
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;

    config->b_rx_error_discard_enable          = ctrl->ndMac.x_command_config.b_rx_error_discard_enable;  
    config->b_no_length_check                  = ctrl->ndMac.x_command_config.b_no_length_check;      
    config->b_control_frame_enable             = ctrl->ndMac.x_command_config.b_control_frame_enable; 
    config->b_node_wake_up_request_indication  = ctrl->ndMac.x_command_config.b_node_wake_up_request_indication;
    config->b_put_core_in_sleep_mode           = ctrl->ndMac.x_command_config.b_put_core_in_sleep_mode;       
    config->b_enable_magic_packet_detection    = ctrl->ndMac.x_command_config.b_enable_magic_packet_detection;
    config->b_software_reset                   = ctrl->ndMac.x_command_config.b_software_reset;               
    config->b_is_late_collision_condition      = ctrl->ndMac.x_command_config.b_is_late_collision_condition;  
    config->b_is_excessive_collision_condition = ctrl->ndMac.x_command_config.b_is_excessive_collision_condition; 
    config->b_enable_half_duplex               = ctrl->ndMac.x_command_config.b_enable_half_duplex;           
    config->b_insert_mac_addr_on_transmit      = ctrl->ndMac.x_command_config.b_insert_mac_addr_on_transmit;  
    config->b_ignore_pause_frame_quanta        = ctrl->ndMac.x_command_config.b_ignore_pause_frame_quanta;    
    config->b_fwd_pause_frames                 = ctrl->ndMac.x_command_config.b_fwd_pause_frames;             
    config->b_fwd_crc_field                    = ctrl->ndMac.x_command_config.b_fwd_crc_field;                
    config->b_enable_frame_padding             = ctrl->ndMac.x_command_config.b_enable_frame_padding;         
    config->b_enable_promiscuous_mode          = ctrl->ndMac.x_command_config.b_enable_promiscuous_mode;      
    config->b_enable_mac_receive               = ctrl->ndMac.x_command_config.b_enable_mac_receive;           
    config->b_enable_mac_transmit              = ctrl->ndMac.x_command_config.b_enable_mac_transmit; 

    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_esw_ces_mac_pm_get
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit - (IN) Unit number.
 *      port - (IN) Port number
 *      config - (IN) config
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_mac_pm_get(int unit, AgNdMsgPmMac *stats) {
    AgResult n_ret;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;


    n_ret = ag_nd_device_read(ctrl->ndHandle, AG_ND_OPCODE_PM_MAC, stats);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_esw_ces_rpc_pm_set
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit   - (IN) Unit number.
 *      global - (IN) Global PM stats config struct
 *      
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_rpc_pm_set(int unit, AgNdMsgConfigRpcPolicy *policy) {
    AgResult n_ret;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;

    /*
     * Set RPC global PM count registers
     */
    n_ret = ag_nd_device_write(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_RPC_POLICY, policy);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}


/**
 * Function:
 *      bcm_esw_ces_rpc_pm_get
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit   - (IN) Unit number.
 *      global - (IN) Global PM stats struct
 *      
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_rpc_pm_get(int unit, 
		       AgNdMsgConfigRpcPolicy *policy,
		       AgNdMsgPmGlobal *global) {
    AgResult n_ret;
    bcm_ces_service_global_config_t *ctrl = SOC_CONTROL(unit)->ces_ctrl;

    /*
     * Read global count registers
     */
    n_ret = ag_nd_device_read(ctrl->ndHandle, AG_ND_OPCODE_PM_GLOBAL, global);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    /*
     * Read policy
     */
    n_ret = ag_nd_device_read(ctrl->ndHandle, AG_ND_OPCODE_CONFIG_RPC_POLICY, policy);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }


    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_esw_ces_service_pm_get
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit   - (IN) Unit number.
 *      global - (IN) Global PM stats struct
 *      
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_service_pm_get(int unit, bcm_ces_service_t ces_service, bcm_ces_service_pm_stats_t *pm_counts) {
    AgResult n_ret;
    bcm_ces_service_record_t *record;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_UNAVAIL;

    sal_memcpy(pm_counts, &record->pm_counts, sizeof(bcm_ces_service_pm_stats_t));
    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_esw_ces_service_pm_clear
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit   - (IN) Unit number.
 *      global - (IN) Global PM stats struct
 *      
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_esw_ces_service_pm_clear(int unit, bcm_ces_service_t ces_service) {

    AgResult n_ret;
    bcm_ces_service_record_t *record;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_UNAVAIL;

    CES_LOCK(unit);
    sal_memset(&record->pm_counts, 0, sizeof(bcm_ces_service_pm_stats_t));
    CES_UNLOCK(unit);

    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_esw_ces_framer_prbs_set
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit   - (IN) Unit number.
 *      global - (IN) Global PM stats struct
 *      
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int
bcm_esw_ces_framer_prbs_set(int unit, int port, int dir, int mode, int start, int end) {
    uint32 rval;
    int batm_port = bcm_ces_port_to_circuit_id(unit, port);
    int i;
    uint32 ts = 0;
    int rv;

    /*
     * Setup timeslot selection
     */
    if (start != 0) {
	for (i = start; i <= end;i++)
	    ts |= (1 << i);
    }

    if (batm_port < 8) {
	/*
	 * Setup the Framer PRBS
	 */
	rval = 0;
	WRITE_FPRBCFG2r(unit, rval);
	WRITE_FRPRTSEL2_1r(unit, rval);
	WRITE_FRPRTSEL2_2r(unit, rval);

	rv = READ_FPRBCFG1r(unit, &rval);
        if (rv < 0)
            return BCM_E_INTERNAL;

	soc_reg_field_set(unit, FPRBCFG1r, &rval, BATM_DIRf, dir);
	soc_reg_field_set(unit, FPRBCFG1r, &rval, BATM_PRBMSf, mode);
	soc_reg_field_set(unit, FPRBCFG1r, &rval, BATM_PORTSELf, batm_port);
	if (start == 0) {
	    soc_reg_field_set(unit, FPRBCFG1r, &rval, BATM_UFPf, 1);
	    soc_reg_field_set(unit, FPRBCFG1r, &rval, BATM_TSGf, 0);
	} else {
	    soc_reg_field_set(unit, FPRBCFG1r, &rval, BATM_UFPf, 0);
	    soc_reg_field_set(unit, FPRBCFG1r, &rval, BATM_TSGf, 1);
	}
	WRITE_FPRBCFG1r(unit, rval);

	/*
	 * Setup timeslot selection
	 */
	if (start != 0) {
	    WRITE_FRPRTSEL1_1r(unit, ts & 0xFFFF);
	    WRITE_FRPRTSEL1_2r(unit, (ts >> 16) & 0xFFFF);
	}
    } else {
	/*
	 * Setup the Framer PRBS
	 */
	rval = 0;
	WRITE_FPRBCFG1r(unit, rval);
	WRITE_FRPRTSEL1_1r(unit, rval);
	WRITE_FRPRTSEL1_2r(unit, rval);

	rv = READ_FPRBCFG2r(unit, &rval);
        if (rv < 0)
            return BCM_E_INTERNAL;
	soc_reg_field_set(unit, FPRBCFG2r, &rval, BATM_DIRf, dir);
	soc_reg_field_set(unit, FPRBCFG2r, &rval, BATM_PRBMSf, mode);
	soc_reg_field_set(unit, FPRBCFG2r, &rval, BATM_PORTSELf, batm_port % 8);
	if (start == 0) {
	    soc_reg_field_set(unit, FPRBCFG2r, &rval, BATM_UFPf, 1);
	    soc_reg_field_set(unit, FPRBCFG2r, &rval, BATM_TSGf, 0);
	} else {
	    soc_reg_field_set(unit, FPRBCFG2r, &rval, BATM_UFPf, 0);
	    soc_reg_field_set(unit, FPRBCFG2r, &rval, BATM_TSGf, 1);
	}
	WRITE_FPRBCFG2r(unit, rval);

        /*
	 * Setup timeslot selection
	 */
	if (start != 0) {
	    WRITE_FRPRTSEL2_1r(unit, ts & 0xFFFF);
	    WRITE_FRPRTSEL2_2r(unit, (ts >> 16) & 0xFFFF);
	}
    }

    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_esw_ces_framer_prbs_status
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit   - (IN) Unit number.
 *      global - (IN) Global PM stats struct
 *      
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int
bcm_esw_ces_framer_prbs_status(int unit, int port, int *status) {
    uint32 rval;
    int batm_port = bcm_ces_port_to_circuit_id(unit, port);
    int rv;

    if (batm_port < 8) {
	/*
	 * Get status
	 */
	rv = READ_FRPRBSTAT1r(unit, &rval);
    } else {
	rv = READ_FRPRBSTAT2r(unit, &rval);
    }

    if (rv < 0) {
        *status = 0;
        rv = BCM_E_INTERNAL;
    } else {
        *status = rval & 0x1FF;
    }

    return rv;
}


/**
 * Function:
 *      bcm_esw_ces_service_control_word_set
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit   - (IN) Unit number.
 *      
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int
bcm_esw_ces_service_control_word_set(int unit, bcm_ces_service_t ces_service, uint16 tx_control_word_mask, uint16 tx_control_word) {
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_service_record_t *record;
    int n_ret;
    AgNdMsgConfigCw msg;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_BADID;

    /*
     * Save setting
     */
    record->tx_control_word_mask = tx_control_word_mask;
    record->tx_control_word      = (tx_control_word & tx_control_word_mask);

    sal_memset(&msg, 0, sizeof(AgNdMsgConfigCw));
    msg.n_channel_id = ces_service;
    msg.n_bit_selector = (tx_control_word_mask & (BCM_CES_CW_L | BCM_CES_CW_R | BCM_CES_CW_M));
    msg.n_l_bit  = ((tx_control_word & tx_control_word_mask) & BCM_CES_CW_L) >> BCM_CES_CW_L_START_BIT;
    msg.n_r_bit  = ((tx_control_word & tx_control_word_mask) & BCM_CES_CW_R) >> BCM_CES_CW_R_START_BIT;
    msg.n_m_bits = ((tx_control_word & tx_control_word_mask) & BCM_CES_CW_M) >> BCM_CES_CW_M_START_BIT;

    SOC_DEBUG_PRINT((DK_VERBOSE,"%s: Service:%d mask:0x%04x LBit:0x%01x RBit:0x%01x MBit:0x%01x\n",
		     __func__,
		     msg.n_channel_id,
		     msg.n_bit_selector,
		     msg.n_l_bit,
		     msg.n_r_bit,
		     msg.n_m_bits));

    n_ret = ag_nd_device_write(ces_ctrl->ndHandle, AG_ND_OPCODE_CONFIG_CONTROL_WORD, &msg);
    if (!AG_SUCCEEDED(n_ret))
    {
	return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/**
 * Function:
 *      bcm_esw_ces_service_control_word_get
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit   - (IN) Unit number.
 *      
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int
bcm_esw_ces_service_control_word_get(int unit, bcm_ces_service_t ces_service, uint16 *tx_control_word_mask, uint16 *tx_control_word, uint16 *rx_control_word) {
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    bcm_ces_service_record_t *record;
    int n_ret;

    CES_CHECK_INIT(unit);

    /*
     * Find service record.
     */
    n_ret = bcm_ces_service_find(unit, ces_service, &record);
    if (n_ret != BCM_E_NONE)
	return n_ret;

    if (BCM_CES_IS_FREE(&record->config))
	return BCM_E_BADID;

    *tx_control_word_mask = record->tx_control_word_mask;
    *tx_control_word      = record->tx_control_word;
    *rx_control_word      = record->rx_control_word;

    return BCM_E_NONE;
}


/**
 * Function:
 *      bcm_esw_ces_cb_register
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit   - (IN) Unit number.
 *      
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int
bcm_esw_ces_cb_register(int unit, 
			bcm_ces_event_types_t events, 
			bcm_ces_event_cb callback, 
			void *user_data) {
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    SOC_DEBUG_PRINT((DK_VERBOSE,"%s: Events:0x%02x cb:%p user_data:%p\n",
		     __func__,
		     events,
		     (void *)callback,
		     user_data));

    if (callback == NULL)
	return BCM_E_PARAM;

    if (events & ~(bcmCesEventPacketBufferOverflow | 
		   bcmCesEventControlWordChange  | 
		   bcmCesEventPacketSyncChange | 
		   bcmCesEventFLLSamplingComplete)) {
	return BCM_E_PARAM;
    }

    if (events & bcmCesEventPacketBufferOverflow) {
	if (ces_ctrl->pboCallback == NULL) {
	    ces_ctrl->pboCallback = callback;
	    ces_ctrl->pboUserData = user_data;
	} else {
	    return BCM_E_EXISTS;
	}
    }

    if (events & bcmCesEventControlWordChange) {
	if (ces_ctrl->cwcCallback == NULL) {
	    ces_ctrl->cwcCallback = callback;
	    ces_ctrl->cwcUserData = user_data;
	} else {
	    return BCM_E_EXISTS;
	}
    }

    if (events & bcmCesEventPacketSyncChange) {
	if (ces_ctrl->pscCallback == NULL) {
	    ces_ctrl->pscCallback = callback;
	    ces_ctrl->pscUserData = user_data;
	} else {
	    return BCM_E_EXISTS;
	}
    }

    if (events & bcmCesEventFLLSamplingComplete) {
	if (ces_ctrl->fscCallback == NULL) {
	    ces_ctrl->fscCallback = callback;
	    ces_ctrl->fscUserData = user_data;
	} else {
	    return BCM_E_EXISTS;
	}
    }

    return BCM_E_NONE;
}


/**
 * Function:
 *      bcm_esw_ces_cb_unregister
 * Purpose:
 *      
 *      
 * Parameters:
 *      unit   - (IN) Unit number.
 *      
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int bcm_esw_ces_cb_unregister(int unit, 
			  bcm_ces_event_types_t events, 
			  bcm_ces_event_cb callback) {

    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    CES_CHECK_INIT(unit);

    if (events & ~(bcmCesEventPacketBufferOverflow | 
		   bcmCesEventControlWordChange  | 
		   bcmCesEventPacketSyncChange | 
		   bcmCesEventFLLSamplingComplete)) {
	return BCM_E_PARAM;
    }

    if (events & bcmCesEventPacketBufferOverflow) {
	if (ces_ctrl->pboCallback == callback) {
	    ces_ctrl->pboCallback = NULL;
	    ces_ctrl->pboUserData = NULL;
	} else {
	    return BCM_E_EXISTS;
	}
    }

    if (events & bcmCesEventControlWordChange) {
	if (ces_ctrl->cwcCallback == callback) {
	    ces_ctrl->cwcCallback = NULL;
	    ces_ctrl->cwcUserData = NULL;
	} else {
	    return BCM_E_EXISTS;
	}
    }

    if (events & bcmCesEventPacketSyncChange) {
	if (ces_ctrl->pscCallback == callback) {
	    ces_ctrl->pscCallback = NULL;
	    ces_ctrl->pscUserData = NULL;
	} else {
	    return BCM_E_EXISTS;
	}
    }

    if (events & bcmCesEventFLLSamplingComplete) {
	if (ces_ctrl->fscCallback == callback) {
	    ces_ctrl->fscCallback = NULL;
	    ces_ctrl->fscUserData = NULL;
	} else {
	    return BCM_E_EXISTS;
	}
    }

    return BCM_E_NONE;
}


#else
int __no_complilation_complaints_about_ces__0;
#endif /* INCLUDE_CES */
