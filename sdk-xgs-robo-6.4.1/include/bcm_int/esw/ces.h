/*
 * $Id: ces.h,v 1.25 Broadcom SDK $
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

#ifndef __BCM_INT_CES_H__
#define __BCM_INT_CES_H__

#include <bcm/ces.h>
#include <bcm/port.h>
#include <pub/nd_api.h>
#include <soc/shared/mos_msg_ces.h>

/*
 * Setting for ces16core
 */
#define BCM_CES_COREID 0x90b0 /* Arch:1 (0x01), ID:7 (0x10), REV:8 (0xb0) */
#define BCM_CES_PW_MAX 64
#define BCM_CES_RCLOCK_MAX 4 /* Max four instances in the CRM */

/*
 * Starting number of TDM ports
 */
#define BCM_CES_TDM_PORT_BASE_KATANA 39

/*
 * Macros
 */
#define bcm_ces_service_is_valid(s) ((s >= 0) && (s < (bcm_ces_service_t)BCM_CES_CIRCUIT_IDX_MAX)) 

/*
 * Control flags
 */
#define BCM_CES_FLAG_CONFIGURED 0x01 /* Configuration has been written to the driver */
#define BCM_CES_FLAG_FREE       0x02 /* Record is free */
#define BCM_CES_FLAG_MODIFIED   0x04 /* Config has been modified and not written to the driver */
#define BCM_CES_FLAG_ENABLED    0x08 /* Driver is enabled for this config */

#define BCM_CES_IS_CONFIGURED(p)  ((p)->flags & BCM_CES_FLAG_CONFIGURED)
#define BCM_CES_IS_FREE(p)        ((p)->flags & BCM_CES_FLAG_FREE)
#define BCM_CES_IS_MODIFIED(p)    ((p)->flags & BCM_CES_FLAG_MODIFIED)
#define BCM_CES_IS_ENABLED(p)     ((p)->flags & BCM_CES_FLAG_ENABLED)

#define BCM_CES_SET_CONFIGURED(p) BCM_CES_CLR_MODIFIED(p); (p)->flags |= BCM_CES_FLAG_CONFIGURED
#define BCM_CES_SET_FREE(p)       (p)->flags |= BCM_CES_FLAG_FREE
#define BCM_CES_SET_MODIFIED(p)   BCM_CES_CLR_CONFIGURED(p); (p)->flags |= BCM_CES_FLAG_MODIFIED
#define BCM_CES_SET_ENABLED(p)    (p)->flags |= BCM_CES_FLAG_ENABLED

#define BCM_CES_CLR_CONFIGURED(p) (p)->flags &= ~BCM_CES_FLAG_CONFIGURED
#define BCM_CES_CLR_FREE(p)       (p)->flags &= ~BCM_CES_FLAG_FREE
#define BCM_CES_CLR_MODIFIED(p)   (p)->flags &= ~BCM_CES_FLAG_MODIFIED
#define BCM_CES_CLR_ENABLED(p)    (p)->flags &= ~BCM_CES_FLAG_ENABLED
#define BCM_CES_CLR_ALL(p)        (p)->flags = 0x00

/*
 * Control structs
 */

/*
 * Warm boot support
 */
#ifdef BCM_WARM_BOOT_SUPPORT
typedef struct {
    bcm_ces_service_t ces_service;  
} bcm_ces_warm_boot_t;
#endif

/*
 * TDM control struct, one per TDM port (16)
 */
typedef struct {
    uint8                  flags;
    bcm_port_t             port;
    bcm_tdm_port_config_t  config;
    struct bcm_ces_service_record_s *serviceList[BCM_CES_SLOT_MAX];  
} bcm_tdm_port_record_t;

/*
 * CES Service control struct, one per CES service (64) per CES instance.
 */
typedef struct bcm_ces_service_record_s {
    bcm_ces_service_t         ces_service;
    bcm_tdm_port_record_t    *tdmRecord;  
    bcm_ces_service_config_t  config;
    bcm_ces_service_pm_stats_t pm_counts;
    uint16                    tx_control_word_mask;
    uint16                    tx_control_word;
    uint16                    rx_control_word;
    struct bcm_ces_rclock_record_s  *rclock;
} bcm_ces_service_record_t;

typedef struct bcm_ces_rclock_record_s { 
    bcm_ces_rclock_config_t   config;
    bcm_ces_service_record_t *service;
} bcm_ces_rclock_record_t;

/*
 * CES control struct, one per CES instance (1)
 */
typedef struct {
    uint8                  flags;
    sal_mutex_t            lock;   
    uint8                  crm_core;
    int                    uc_appl_initialized;
    uint8                  attached;
    bcm_ces_tdm_proto_t    protocol;
    sal_mac_addr_t         mac;          /* ces_mii_mac */
    uint32                 ces_mii_port; 
    uint32                 system_clock_rate;
    uint32                 common_ref_clock_rate;
    bcm_ces_cclk_config_t  cclk_config;
    ip_addr_t              ipv4_addr;
    ip6_addr_t             ipv6_addr;
    AgNdHandle             ndHandle;
    AgNdMsgConfigInit      ndInit;
    AgNdMsgConfigGlobal    ndGlobal;
    AgNdMsgConfigMac       ndMac;
    AgNdMsgConfigRpcUcode  ndUcode;
    AgNdMsgConfigRpcPolicy ndPolicy;
    bcm_ces_service_record_t *bcm_ces_service_records[BCM_CES_CIRCUIT_IDX_MAX];
#ifdef BCM_WARM_BOOT_SUPPORT
    bcm_ces_warm_boot_t    warm_boot_data[BCM_CES_CIRCUIT_IDX_MAX];
#endif
    bcm_ces_rclock_record_t rclock_record[BCM_CES_RCLOCK_MAX];
    void                  *dma_buffer;
    uint32                 dma_buffer_len;
    bcm_ces_event_cb      pboCallback; /* Packet buffer overflow callback */
    bcm_ces_event_cb      cwcCallback; /* Control word change callback */
    bcm_ces_event_cb      pscCallback; /* Packet sync change callback */
    bcm_ces_event_cb      fscCallback; /* FLL sampling complete callback */
    void                  *pboUserData; /* Packet buffer overflow user data */
    void                  *cwcUserData; /* Control word change user data */
    void                  *pscUserData; /* Packet sync change user data */
    void                  *fscUserData; /* FLL sampling complete user data */
} bcm_ces_service_global_config_t;


/*
 * RTP timestamp step size and error correction period values
 */
#define BCM_CES_RTP_TSS_TABLE_INDEX_1544  0
#define BCM_CES_RTP_TSS_TABLE_INDEX_2048  1
#define BCM_CES_RTP_TSS_TABLE_INDEX_10240 2
#define BCM_CES_RTP_TSS_TABLE_INDEX_16384 3
#define BCM_CES_RTP_TSS_TABLE_INDEX_19440 4
#define BCM_CES_RTP_TSS_TABLE_INDEX_25000 5
#define BCM_CES_RTP_TSS_TABLE_INDEX_125000 6
#define BCM_CES_RTP_TSS_TABLE_INDEX_CRC_MAX 7
#define BCM_CES_RTP_TSS_TABLE_INDEX_RTR_MAX 6

typedef struct {
    uint32 step_size; /* STPSZ */
    uint8  error_correction_enable; /* ECE */
    uint16 error_correction_period; /* ECP */
} bcm_ces_rtp_tss_table_t;


#define BCM_CES_TIMESTAMP_CLOCK_RATE  25000000L /* Fixed */
#define BCM_CES_SYSTEM_CLOCK_RATE     25000000L
#define BCM_CES_COMMON_REF_CLOCK_RATE  1544000L

#ifdef BCM_CES_CRM_SUPPORT   
/*
 * CRM messages
 */
#define _CES_INIT_RETRIES 2
#define _CES_NUM_CMICM    2
#define _CES_UC_MSG_TIMEOUT_USECS 1000000

#define BCM_CES_CRM_FLAG_NONE 0x00 /* */
#define BCM_CES_CRM_FLAG_INIT 0x01 /* The host has just (re)started */
#define BCM_CES_CRM_FLAG_WB   0x02 /* Warm boot */
#endif

/*
 * Prototypes
 */
int bcm_ces_service_find(int unit, 
			 bcm_ces_service_t ces_service,
			 bcm_ces_service_record_t **record);
int bcm_ces_port_to_circuit_id(int unit, bcm_port_t port);
bcm_tdm_port_record_t *bcm_esw_port_tdm_find_port(int unit, bcm_port_t port);
int bcm_esw_port_tdm_add_service(int unit,
				 bcm_tdm_port_record_t *record, 
				 bcm_ces_service_record_t *service,
				 int num_slots,
				 uint16 *slots);
int bcm_esw_port_tdm_delete_service(int unit,
				    bcm_tdm_port_record_t *record, 
				    bcm_ces_service_record_t *service);
int bcm_esw_port_tdm_framer_port_loopback_set(
    int unit, 
    bcm_port_t tdm_port, 
    int enable,
    int type,
    uint32 slot_mask,
    int activation_code,
    int deactivation_code);
int bcm_esw_port_tdm_framer_port_status(int unit,
					bcm_port_t tdm_port, 
					AgFramerPortStatus *ndFramerPortStatus);
int bcm_esw_port_tdm_framer_port_pm(int unit,
				bcm_port_t tdm_port, 
				AgFramerPortPm *ndFramerPortPm);
void bcm_esw_port_tdm_cas_replacement_set(int unit, bcm_tdm_port_record_t *record);

int bcm_ces_diag_set(int unit, AgNdMsgDiag *config);
int bcm_ces_diag_get(int unit, AgNdMsgDiag *config);
int bcm_esw_ces_mac_pm_get(int unit, AgNdMsgPmMac *stats);
int bcm_esw_ces_rpc_pm_set(int unit, AgNdMsgConfigRpcPolicy *policy);
int bcm_esw_ces_rpc_pm_get(int unit, AgNdMsgConfigRpcPolicy *policy, AgNdMsgPmGlobal *global);
#ifdef BCM_WARM_BOOT_SUPPORT
int _bcm_esw_ces_sync(int unit);
#endif
int bcm_esw_ces_framer_prbs_status(int unit, int port, int *status);
int bcm_esw_ces_framer_prbs_set(int unit, int port, int dir, int mode, int start, int end);
int _bcm_esw_ces_rclock_status_get(int unit, int rcr_index, bcm_ces_crm_status_msg_t *ces_rcr_status);
#endif /* __BCM_INT_CES_H__ */
