/*
 * $Id: ptp.h,v 1.2 Broadcom SDK $
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

#ifndef __BCM_INT_PTP_H__
#define __BCM_INT_PTP_H__

#include <shared/bsl.h>

#include <soc/defs.h>
#include <soc/drv.h>

#if defined(INCLUDE_PTP)

#include <bcm/ptp.h>

#ifndef BCM_PTP_MAX_BUFFERS
/* BCM_PTP_MAX_BUFFERS: number of message buffers usable by CMICm */
#define BCM_PTP_MAX_BUFFERS (48)
#endif

#ifndef BCM_PTP_MAX_LOG
/* BCM_PTP_MAX_LOG: size of debug log from CMICm */
#define BCM_PTP_MAX_LOG (1024)
#endif

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_common_ptp_sync(int unit);
#endif
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_ptp_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#if defined(BCM_KATANA_SUPPORT)
 #define PTP_CONDITIONAL_IS_KATANA(unit) (SOC_IS_KATANA(unit))
#else
 #define PTP_CONDITIONAL_IS_KATANA(unit) (0)
#endif

#if defined(BCM_TRIUMPH3_SUPPORT)
 #define PTP_CONDITIONAL_IS_TRIUMPH3(unit) (SOC_IS_TRIUMPH3(unit))
#else
 #define PTP_CONDITIONAL_IS_TRIUMPH3(unit) (0)
#endif

#if defined(BCM_TRIDENT2_SUPPORT)
 #define PTP_CONDITIONAL_IS_TRIDENT2(unit) (SOC_IS_TD2_TT2(unit))
#else
 #define PTP_CONDITIONAL_IS_TRIDENT2(unit) (0)
#endif

#if defined(BCM_ARAD_SUPPORT)
 #define PTP_CONDITIONAL_IS_ARAD(unit) (SOC_IS_ARAD(unit))
#else
 #define PTP_CONDITIONAL_IS_ARAD(unit) (0)
#endif

#if defined(BCM_KATANA2_SUPPORT)
 #define PTP_CONDITIONAL_IS_KATANA2(unit) (SOC_IS_KATANA2(unit))
#else
 #define PTP_CONDITIONAL_IS_KATANA2(unit) (0)
#endif

/* define SOC which support internal stack */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) || \
    defined(BCM_ARAD_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
 /* For compile-time decision: */
 #define BCM_PTP_INTERNAL_STACK_SUPPORT (1)
 /* For run-time per-unit decisions: */
#define SOC_HAS_PTP_INTERNAL_STACK_SUPPORT(unit)  (PTP_CONDITIONAL_IS_KATANA(unit) || PTP_CONDITIONAL_IS_TRIUMPH3(unit) || \
                                                   PTP_CONDITIONAL_IS_TRIDENT2(unit) || PTP_CONDITIONAL_IS_ARAD(unit) ||   \
                                                   PTP_CONDITIONAL_IS_KATANA2(unit)) 
#else
 #undef BCM_PTP_INTERNAL_STACK_SUPPORT
 #define SOC_HAS_PTP_INTERNAL_STACK_SUPPORT(unit)          (0)
#endif

/* define SOC which should include external stack support */
#if defined(PTP_KEYSTONE_STACK) && defined(BCM_ENDURO_SUPPORT)
 /* For compile-time decisions: */
 #define BCM_PTP_EXTERNAL_STACK_SUPPORT (1)
 /* For run-time per-unit decisions: */
 #define SOC_HAS_PTP_EXTERNAL_STACK_SUPPORT(unit)          (SOC_IS_ENDURO(unit))
#else
 #undef BCM_PTP_EXTERNAL_STACK_SUPPORT
 #define SOC_HAS_PTP_EXTERNAL_STACK_SUPPORT(unit)          (SOC_IS_ENDURO(unit))
#endif

#define PTP_ERROR(errmsg)      bsl_printf("%s() FAILED %s\n", \
                               FUNCTION_NAME(), errmsg)

#define PTP_ERROR_FUNC(func)   bsl_printf("%s() FAILED " \
                               func " returned %d : %s\n", \
                               FUNCTION_NAME(), rv, bcm_errmsg(rv))
                               
#define PTP_WARN(warnmsg)      bsl_printf("%s() failed %s\n", \
                               FUNCTION_NAME(), warnmsg)
                               
#define PTP_WARN_FUNC(func)    bsl_printf("%s() failed " \
                               func " returned %d : %s\n", \
                               FUNCTION_NAME(), rv, bcm_errmsg(rv))


#define CONFIG_RCPU_OFFSET (0)
#define CONFIG_RCPU_SIZE (4 + 4 + 4 + 4 + 4 + 4 + 6 + 2)
#define CONFIG_VLAN_OFFSET (CONFIG_RCPU_OFFSET + CONFIG_RCPU_SIZE)
#define CONFIG_VLAN_SIZE (2 + 2)
#define CONFIG_MPLS_OFFSET (CONFIG_VLAN_OFFSET + CONFIG_VLAN_SIZE)
#define CONFIG_MPLS_SIZE (2 + 2)
#define CONFIG_HOST_OFFSET (CONFIG_MPLS_OFFSET + CONFIG_MPLS_SIZE)
#define CONFIG_HOST_SIZE (6 + 2 + 6 + 2 + 4 + 4 + 2 + 2)
#define CONFIG_DEBUG_OFFSET (CONFIG_HOST_OFFSET + CONFIG_HOST_SIZE)
#define CONFIG_DEBUG_SIZE (4)
#define CONFIG_PPSOUT_OFFSET (CONFIG_DEBUG_OFFSET + CONFIG_DEBUG_SIZE)
#define CONFIG_PPSOUT_SIZE (4 + 4 + 4 + 4 + 4 + 4)
#define CONFIG_TIMESOURCE_OFFSET (CONFIG_PPSOUT_OFFSET + CONFIG_PPSOUT_SIZE)
#define CONFIG_TIMESOURCE_SIZE (4)
#define CONFIG_EXTRA_SIZE (256)

#define CONFIG_TOTAL_SIZE (CONFIG_RCPU_SIZE + CONFIG_VLAN_SIZE + CONFIG_MPLS_SIZE + \
                           CONFIG_HOST_SIZE + CONFIG_DEBUG_SIZE + CONFIG_PPSOUT_SIZE + \
                           CONFIG_TIMESOURCE_SIZE + CONFIG_EXTRA_SIZE)

#define INFO_BASE_ADDR    (0x19000bac)
#define CONFIG_BASE       (0x19000c2c)
                           
#define BOOT_STATUS_ADDR  (0x19000500)
#define FAULT_STATUS_ADDR (0x19000504)
#define MAX_BOOT_ITER (1000)    /* cycles of 1ms to wait for the ToP boot to complete */

/* 
 * All ports identifier.
 * Ref. IEEE Std. 1588-2008, Chapters 13.12.1 and 15.3.1. 
 */
#define PTP_IEEE1588_ALL_PORTS (0xffff) 

/* 
 * PTP management message actions.
 * Ref. IEEE Std. 1588-2008, Chapter 15.4.1.6, Table 38. 
 */
#define PTP_MGMTMSG_GET           (0)
#define PTP_MGMTMSG_SET           (1)
#define PTP_MGMTMSG_RESP          (2)
#define PTP_MGMTMSG_CMD           (3)
#define PTP_MGMTMSG_ACK           (4)
    
/* 
 * PTP management message ID (i.e. managementId) values.
 * Ref. IEEE Std. 1588-2008, Chapter 15.5.2.3, Table 40. 
 */
#define PTP_MGMTMSG_ID_CLOCK_DESCRIPTION                           (0x0001)
#define PTP_MGMTMSG_ID_USER_DESCRIPTION                            (0x0002)
                           
#define PTP_MGMTMSG_ID_DEFAULT_DATASET                             (0x2000)
#define PTP_MGMTMSG_ID_CURRENT_DATASET                             (0x2001)
#define PTP_MGMTMSG_ID_PARENT_DATASET                              (0x2002)
#define PTP_MGMTMSG_ID_TIME_PROPERTIES_DATASET                     (0x2003)
#define PTP_MGMTMSG_ID_PORT_DATASET                                (0x2004)
                           
#define PTP_MGMTMSG_ID_PRIORITY1                                   (0x2005)
#define PTP_MGMTMSG_ID_PRIORITY2                                   (0x2006)
#define PTP_MGMTMSG_ID_DOMAIN                                      (0x2007)                           
#define PTP_MGMTMSG_ID_SLAVE_ONLY                                  (0x2008)
#define PTP_MGMTMSG_ID_LOG_ANNOUNCE_INTERVAL                       (0x2009)
#define PTP_MGMTMSG_ID_ANNOUNCE_RECEIPT_TIMEOUT                    (0x200a)
#define PTP_MGMTMSG_ID_LOG_SYNC_INTERVAL                           (0x200b)
#define PTP_MGMTMSG_ID_VERSION_NUMBER                              (0x200c)

#define PTP_MGMTMSG_ID_ENABLE_PORT                                 (0x200d)
#define PTP_MGMTMSG_ID_DISABLE_PORT                                (0x200e)
                           
#define PTP_MGMTMSG_ID_TIME                                        (0x200f)                   
#define PTP_MGMTMSG_ID_CLOCK_ACCURACY                              (0x2010)
#define PTP_MGMTMSG_ID_UTC_PROPERTIES                              (0x2011)                           
#define PTP_MGMTMSG_ID_TRACEABILITY_PROPERTIES                     (0x2012)
#define PTP_MGMTMSG_ID_TIMESCALE_PROPERTIES                        (0x2013)
 
#define PTP_MGMTMSG_ID_UNICAST_MASTER_TABLE                        (0x2018)
#define PTP_MGMTMSG_ID_UNICAST_MASTER_MAX_TABLE_SIZE               (0x2019)

#define PTP_MGMTMSG_ID_ACCEPTABLE_MASTER_TABLE                     (0x201a)
#define PTP_MGMTMSG_ID_ACCEPTABLE_MASTER_TABLE_ENABLED             (0x201b)
#define PTP_MGMTMSG_ID_ACCEPTABLE_MASTER_MAX_TABLE_SIZE            (0x201c)
   
#define PTP_MGMTMSG_ID_TRANSPARENT_CLOCK_DEFAULT_DATASET           (0x4000)
#define PTP_MGMTMSG_ID_TRANSPARENT_CLOCK_PORT_DATASET              (0x4001)
#define PTP_MGMTMSG_ID_PRIMARY_DOMAIN                              (0x4002)
                           
#define PTP_MGMTMSG_ID_DELAY_MECHANISM                             (0x6000)
#define PTP_MGMTMSG_ID_LOG_MIN_PDELAY_REQ_INTERVAL                 (0x6001)
                           
/* 
 * Implementation-specific PTP management message IDs.
 * Hits the standard management message get/set code. 
 */
#define PTP_MGMTMSG_ID_MODULAR_SYSTEM_ENABLED                      (0xdff4)
#define PTP_MGMTMSG_ID_PHYSYNC_STATE                               (0xdff5)
#define PTP_MGMTMSG_ID_OUTPUT_SYNCE                                (0xdff6)
#define PTP_MGMTMSG_ID_CLOCK_CLASS                                 (0xdff7)
#define PTP_MGMTMSG_ID_TELECOM_PROFILE_ENABLED                     (0xdff8)
#define PTP_MGMTMSG_ID_PACKET_STATS                                (0xdff9)
#define PTP_MGMTMSG_ID_OUTPUT_SIGNALS                              (0xdffa)
#define PTP_MGMTMSG_ID_CHANNELS                                    (0xdffb)
#define PTP_MGMTMSG_ID_PEER_DATASET_FIRST                          (0xdffc)
#define PTP_MGMTMSG_ID_PEER_DATASET_NEXT                           (0xdffd)
#define PTP_MGMTMSG_ID_FOREIGN_MASTER_DATASET                      (0xdffe)
#define PTP_MGMTMSG_ID_LOG_MIN_DELAY_REQ_INTERVAL                  (0xdfff)  

/* 
 * Implementation-specific PTP management message IDs.
 * Following hit the custom handler that does not depend on having an
 * instantiated clock or correctly configured ports.
 */
#define PTP_MGMTMSG_ID_CREATE_CLOCK_INSTANCE                       (0xd000)  
#define PTP_MGMTMSG_ID_CONFIGURE_CLOCK_PORT                        (0xd002)  
#define PTP_MGMTMSG_ID_CONFIGURE_TOD_OUT                           (0xd004)  
#define PTP_MGMTMSG_ID_CONFIGURE_TOD_IN                            (0xd005)
#define PTP_MGMTMSG_ID_SYNC_PHY                                    (0xd006)

#define PTP_MGMTMSG_ID_SERVO_START                                 (0xd00d)
#define PTP_MGMTMSG_ID_SERVO_RESTART                               (0xd00e)
#define PTP_MGMTMSG_ID_SERVO_STOP                                  (0xd00f)
#define PTP_MGMTMSG_ID_SERVO_CONFIGURATION                         (0xd010)
#define PTP_MGMTMSG_ID_SERVO_IPDV_CONFIGURATION                    (0xd011)
#define PTP_MGMTMSG_ID_SERVO_IPDV_PERFORMANCE                      (0xd012)
#define PTP_MGMTMSG_ID_SERVO_PERFORMANCE                           (0xd013)
#define PTP_MGMTMSG_ID_SERVO_COUNTERS_CLEAR                        (0xd014)
#define PTP_MGMTMSG_ID_SERVO_STATUS                                (0xd015)
           
#define PTP_MGMTMSG_ID_PORT_LATENCY                                (0xd017)
#define PTP_MGMTMSG_ID_LOG_LEVEL                                   (0xd018)
                       
#define PTP_MGMTMSG_ID_TUNNEL_ARP                                  (0xd019)
#define PTP_MGMTMSG_ID_SHOW_SYSTEM_INFO                            (0xd01a)

#define PTP_MGMTMSG_ID_CHANNEL_STATUS                              (0xd01b)

/*
 * NOTE: Management message ID 0xd01c assigned in FW to servo/system
 *       frequency offset data for Keystone platforms.
 */

/* T-DPLL support. */
#define PTP_MGMTMSG_ID_TDPLL_BINDINGS                              (0xd01d)
#define PTP_MGMTMSG_ID_TDPLL_REFERENCE                             (0xd01e)

#define PTP_MGMTMSG_ID_TDPLL_PHASE_CONTROL                         (0xd01f)
#define PTP_MGMTMSG_ID_TDPLL_BANDWIDTH                             (0xd020)

#define PTP_MGMTMSG_ID_INPUT_CLOCK_ENABLED                         (0xd021)
#define PTP_MGMTMSG_ID_INPUT_CLOCK_L1MUX                           (0xd022)
#define PTP_MGMTMSG_ID_INPUT_CLOCK_FREQUENCY                       (0xd023)
#define PTP_MGMTMSG_ID_INPUT_CLOCK_MONITOR_DATA                    (0xd024)

#define PTP_MGMTMSG_ID_OUTPUT_CLOCK_ENABLED                        (0xd025)
#define PTP_MGMTMSG_ID_OUTPUT_CLOCK_SYNTH_FREQUENCY                (0xd026)
#define PTP_MGMTMSG_ID_OUTPUT_CLOCK_DERIV_FREQUENCY                (0xd027)

#define PTP_MGMTMSG_ID_HOLDOVER_DATA                               (0xd028)
#define PTP_MGMTMSG_ID_HOLDOVER_FREQUENCY                          (0xd029)
#define PTP_MGMTMSG_ID_HOLDOVER_MODE                               (0xd02a)
#define PTP_MGMTMSG_ID_HOLDOVER_RESET                              (0xd02b)

#define PTP_MGMTMSG_ID_ESMC_TUNNEL                                 (0xd02c)
#define PTP_MGMTMSG_ID_ESMC_MAC                                    (0xd02d)

/* Management message maximum response size (octets). */
#define PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS                             (1024)

/* Management message empty TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_NULL_SIZE_OCTETS                            (0) 
/* Management message minimum, non-empty TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS                             (2) 
/* Management message proprietary message TLV minimum payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_MIN_PROPRIETARY_MSG_SIZE_OCTETS             (6)
/* Management message proprietary message TLV indexed-get payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_INDEXED_PROPRIETARY_MSG_SIZE_OCTETS         (8)
/* Management message maximum USER_DESCIPTION TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_MAX_USER_DESCRIPTION_SIZE_OCTETS           (32)
/* Management message DEFAULT_DATA_SET TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_DEFAULT_DATASET_SIZE_OCTETS                (20) 
/* Management message CURRENT_DATA_SET TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_CURRENT_DATASET_SIZE_OCTETS                (18)
/* Management message PARENT_DATA_SET TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_PARENT_DATASET_SIZE_OCTETS                 (32) 
/* Management message TIME TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_TIME_SIZE_OCTETS                           (10) 
/* Management message UTC_PROPERTIES TLV payload size (octets). */                           
#define PTP_MGMTMSG_PAYLOAD_UTC_PROPERTIES_SIZE_OCTETS                  (4) 
/* Management message UNICAST_MASTER_TABLE TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_MAX_UNICAST_MASTER_TABLE_SIZE_OCTETS     (1404) 
/* Management message ACCEPTABLE_MASTER_TABLE TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_MAX_ACCEPTABLE_MASTER_TABLE_SIZE_OCTETS  (1404) 
/* Management message UNICAST_SLAVE_TABLE TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_MAX_UNICAST_SLAVE_TABLE_SIZE_OCTETS      (1404) 
/* Management message clock instance creation TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_CLOCK_INSTANCE_SIZE_OCTETS                 (49)
/* Management message configure clock port TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_CONFIGURE_CLOCK_PORT_SIZE_OCTETS          (177)
/* Management message PORT_LATENCY TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_PORT_LATENCY_SIZE_OCTETS                   (22) 
/* Management message LOG_LEVEL TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_LOG_LEVEL_SIZE_OCTETS                      (45)
/* Management message CHANNELS TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_CHANNELS_SIZE_OCTETS                       (46) 
/* Management message OUTPUT_SIGNALS TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_OUTPUT_SIGNALS_SIZE_OCTETS                (102) 
/* Management message CONFIGURE_TOD_OUT TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_CONFIGURE_TOD_OUT_SIZE_OCTETS             (167)
/* Management message CONFIGURE_TOD_IN TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_CONFIGURE_TOD_IN_SIZE_OCTETS              (178)
/* Management message SYNC_PHY TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_SYNC_PHY_SIZE_OCTETS                       (16)
/* Management message servo configuration TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_SERVO_CONFIGURATION_SIZE_OCTETS            (54)
/* Management message servo IPDV configuration TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_SERVO_IPDV_CONFIGURATION_SIZE_OCTETS       (14)
/* Management message servo performance data TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_SERVO_PERFORMANCE_SIZE_OCTETS             (223) 
/* Management message servo IPDV performance data TLV payload size (octets). */                           
#define PTP_MGMTMSG_PAYLOAD_SERVO_IPDV_PERFORMANCE_SIZE_OCTETS         (54) 
/* Management message servo status TVL payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_SERVO_STATUS_SIZE_OCTETS                   (11)
/* Management message modular system enabled TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_MODULAR_SYSTEM_ENABLED_SIZE_OCTETS          (8)
/* Management message PTP clock class TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_CLOCK_CLASS_SIZE_OCTETS                     (8)
/* Management message telecom profile enabled TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_TELECOM_PROFILE_ENABLED_SIZE_OCTETS         (8)
/* Management message packet statistics TLV minimum payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_PACKET_STATS_MIN_SIZE_OCTETS               (64)
/* Management message tunnel ARP TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_TUNNEL_ARP_SIZE_OCTETS                      (8)
/* Management message channel status get TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_CHANNEL_STATUS_SIZE_OCTETS                 (72)
/* Management message SyncE output state TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_OUTPUT_SYNCE_SIZE_OCTETS                    (4)
/* Management message T-DPLL logical DPLL bindings TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_TDPLL_BINDINGS_SIZE_OCTETS                 (56)
/* Management message T-DPLL logical DPLL reference clocks TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_TDPLL_REFERENCE_SIZE_OCTETS                (12)
/* Management message T-DPLL logical DPLL phase control TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_TDPLL_PHASE_CONTROL_SIZE_OCTETS            (12)
/* Management message T-DPLL logical DPLL bandwidth TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_TDPLL_BANDWIDTH_SIZE_OCTETS                (12)
/* Management message T-DPLL input clock enabled payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_INPUT_CLOCK_ENABLED_SIZE_OCTETS             (8)
/* Management message T-DPLL input clock L1 mux payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_INPUT_CLOCK_L1MUX_SIZE_OCTETS              (10)
/* Management message T-DPLL input clock frequency TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_INPUT_CLOCK_FREQUENCY_SIZE_OCTETS          (16)
/* Management message T-DPLL input clock monitoring data TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_INPUT_CLOCK_MONITOR_DATA_SIZE_OCTETS      (286)
/* Management message T-DPLL output clock enabled payload size (octets) */
#define PTP_MGMTMSG_PAYLOAD_OUTPUT_CLOCK_ENABLED_SIZE_OCTETS            (8)
/* Management message T-DPLL output clock synthesizer frequency TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_OUTPUT_CLOCK_SYNTH_FREQUENCY_SIZE_OCTETS   (16)
/* Management message T-DPLL output clock derivative frequency TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_OUTPUT_CLOCK_DERIV_FREQUENCY_SIZE_OCTETS   (12)
/* Management message T-DPLL holdover data TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_HOLDOVER_DATA_SIZE_OCTETS                  (32)
/* Management message T-DPLL holdover frequency TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_HOLDOVER_FREQUENCY_SIZE_OCTETS             (12)
/* Management message T-DPLL holdover mode TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_HOLDOVER_MODE_SIZE_OCTETS                   (8)
/* Management message T-DPLL reset TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_HOLDOVER_RESET_SIZE_OCTETS                  (8)
/* Management message ESMC tunnel TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_ESMC_TUNNEL_SIZE_OCTETS                     (8)
/* Management message ESMC MAC address TLV payload size (octets). */
#define PTP_MGMTMSG_PAYLOAD_ESMC_MAC_SIZE_OCTETS                       (14)

/* PTP Header parameters and constants.*/
/* L2 header size (octets). */
#define PTP_L2HDR_SIZE_OCTETS                                          (12) 
/* IEEE 802.1Q, VLAN tagging header size (octets). */
#define PTP_VLANHDR_SIZE_OCTETS                                         (4) 
/*
 * VLAN identifier (VID) size (octets). 
 * NOTICE: VID is 12-bit value. 
 */
#define PTP_VLANHDR_VID_SIZE_OCTETS                                     (2) 
/* VLAN identifier (VID) offset (octets). */
#define PTP_VLANHDR_VID_OFFSET_OCTETS                                   (2) 

/* Ethertype size (octets). */
#define PTP_ETHHDR_SIZE_OCTETS                                          (2) 

/* 
 * IPv4 header start index.
 * NOTICE: Assumes single VLAN tag.
 */
#define PTP_IPV4HDR_START_IDX                  (PTP_L2HDR_SIZE_OCTETS + \
                                                PTP_VLANHDR_SIZE_OCTETS + \
                                                  PTP_ETHHDR_SIZE_OCTETS)
                                                
/* IPv4 header offset of total message length (octets). */
#define PTP_IPV4HDR_MSGLEN_OFFSET_OCTETS                                (2) 
/* IPv4 header offset of source IP address (octets). */
#define PTP_IPV4HDR_SRC_IPADDR_OFFSET_OCTETS                           (12) 
/* IPv4 header offset of header checksum (octets). */
#define PTP_IPV4HDR_HDR_CHKSUM_OFFSET_OCTETS                           (10) 
/* 
 * IPv4 header size (octets). 
 * ASSUMPTION: Field 13 (Options) omitted. 
 */
#define PTP_IPV4HDR_SIZE_OCTETS                                        (20) 

#define PTP_UDPHDR_START_IDX                     (PTP_IPV4HDR_START_IDX + \
                                                  PTP_IPV4HDR_SIZE_OCTETS)
                                                  
/* UDP header size (octets). */
#define PTP_UDPHDR_SIZE_OCTETS                                          (8) 
/* UDP header offset of destination port (octets). */
#define PTP_UDPHDR_DESTPORT_OFFSET_OCTETS                               (2) 
/* UDP header offset of message length (octets). */
#define PTP_UDPHDR_MSGLEN_OFFSET_OCTETS                                 (4) 
/* UDP header offset of checksum (octets). */
#define PTP_UDPHDR_CHKSUM_OFFSET_OCTETS                                 (6) 

#define PTP_PTPHDR_START_IDX                      (PTP_UDPHDR_START_IDX + \
                                                   PTP_UDPHDR_SIZE_OCTETS)
/* PTP header size (octets). */
#define PTP_PTPHDR_SIZE_OCTETS                                         (34) 
/* PTP header offset of PTP message length (octets). */
#define PTP_PTPHDR_MSGLEN_OFFSET_OCTETS                                 (2) 
/* PTP header offset of PTP domain (octets). */
#define PTP_PTPHDR_DOMAIN_OFFSET_OCTETS                                 (4) 
/* PTP header offset of source port identity (octets). */
#define PTP_PTPHDR_SRCPORT_OFFSET_OCTETS                               (20) 
/* PTP header offset of PTP sequence ID (octets). */
#define PTP_PTPHDR_SEQID_OFFSET_OCTETS                                 (30) 

#define PTP_MGMTHDR_START_IDX                     (PTP_PTPHDR_START_IDX + \
                                                   PTP_PTPHDR_SIZE_OCTETS)
                                                   
/* Management header size (octets). */
#define PTP_MGMTHDR_SIZE_OCTETS                                        (14) 
/* Management header offset of management message actions (octets). */
#define PTP_MGMTHDR_ACTION_OFFSET_OCTETS                               (12) 

/* 
 * Message TLV header size (octets).  
 * NOTICE: Excludes message payload. 
 */
#define PTP_TLVHDR_SIZE_OCTETS                                          (6) 
/* Message TLV header offset of message TLV type (octets). */
#define PTP_TLVHDR_TYP_OFFSET_OCTETS                                    (0) 
/* Message TLV header offset of message TLV length (octets). */
#define PTP_TLVHDR_LEN_OFFSET_OCTETS                                    (2) 
/* 
 * Message TLV header offset of message TLV value prefix, 
 * i.e. management ID (octets). 
 */
#define PTP_TLVHDR_VAL_PREFIX_OFFSET_OCTETS                             (4) 
/* Message TLV header TLV value prefix size (octets). */
#define PTP_TLVHDR_VAL_PREFIX_SIZE_OCTETS                               (2) 

/* Management message maximum size (octets). */
#define PTP_MGMTMSG_TOTAL_SIZE_OCTETS                                (1500) 

#define PTP_MGMTMSG_TOTAL_HEADER_SIZE          (PTP_MGMTHDR_START_IDX + \
                                                PTP_MGMTHDR_SIZE_OCTETS + \
                                                PTP_TLVHDR_SIZE_OCTETS)

/* Payload maximum size (octets). */
#define PTP_MGMTMSG_PAYLOAD_MAX_SIZE_OCTETS  (PTP_MGMTMSG_TOTAL_SIZE_OCTETS - ( \
                                              PTP_L2HDR_SIZE_OCTETS + \
                                              PTP_VLANHDR_SIZE_OCTETS + \
                                              PTP_ETHHDR_SIZE_OCTETS + \
                                              PTP_IPV4HDR_SIZE_OCTETS + \
                                              PTP_UDPHDR_SIZE_OCTETS + \
                                              PTP_PTPHDR_SIZE_OCTETS + \
                                              PTP_MGMTHDR_SIZE_OCTETS + \
                                              PTP_TLVHDR_SIZE_OCTETS))
                                                 

#define PTP_SIGHDR_START_IDX                 (PTP_PTPHDR_START_IDX)
#define PTP_SIGHDR_TGTCLOCK_OFFSET_OCTETS    (PTP_PTPHDR_SIZE_OCTETS)
#define PTP_SIGHDR_TGTPORT_OFFSET_OCTETS     (PTP_PTPHDR_SIZE_OCTETS + \
                                              BCM_PTP_CLOCK_EUID_IEEE1588_SIZE)
/* 
 * PTP signaling message header size (octets).
 * NOTICE: Includes PTP header and target port identity. 
 * Ref. IEEE Std. 1588-2008, Sect. 13.12.2.
 */
#define PTP_SIGHDR_SIZE_OCTETS       (PTP_PTPHDR_SIZE_OCTETS + \
                                      BCM_PTP_CLOCK_EUID_IEEE1588_SIZE + 2)
                                                    

#define PTP_UCSIG_TLV_START_IDX                (PTP_L2HDR_SIZE_OCTETS + \
                                                PTP_VLANHDR_SIZE_OCTETS + \
                                                PTP_ETHHDR_SIZE_OCTETS + \
                                                PTP_IPV4HDR_SIZE_OCTETS + \
                                                PTP_UDPHDR_SIZE_OCTETS + \
                                                PTP_SIGHDR_SIZE_OCTETS)
/* 
 * PTP unicast transmission signaling message 
 * REQUEST_UNICAST_TRANSMISSION TLV size (octets). 
 */
#define PTP_UCSIG_REQUEST_TLV_SIZE_OCTETS   ((2 * PTP_MAX_NETW_ADDR_SIZE) + \
                                             PTP_MAX_L2_HEADER_LENGTH + 12)
/* 
 * PTP unicast transmission signaling message 
 * CANCEL_UNICAST_TRANSMISSION TLV size (octets).
 */
#define PTP_UCSIG_CANCEL_TLV_SIZE_OCTETS       (PTP_MAX_NETW_ADDR_SIZE + 7)    

/* 
 * PTP unicast transmission signaling message TLV 
 * offset of TLV length (octets). 
 */
#define PTP_UCSIG_TLVLEN_OFFSET_OCTETS                                  (2)
/* 
 * PTP unicast transmission signaling message TLV 
 * offset of message type (octets). 
 */
#define PTP_UCSIG_MSGTYPE_OFFSET_OCTETS                                 (4)
/* 
 * PTP unicast transmission signaling message TLV 
 * offset of interval parameter (octets). 
 */
#define PTP_UCSIG_INTERVAL_OFFSET_OCTETS                                (5)
/* 
 * PTP unicast transmission signaling message TLV
 * offset of duration parameter (octets). 
 */
#define PTP_UCSIG_DURATION_OFFSET_OCTETS                                (6)
/* 
 * PTP unicast transmission signaling message TLV 
 * offset of address type (octets). 
 */
#define PTP_UCSIG_ADDRTYPE_OFFSET_OCTETS                               (10)
/* 
 * PTP unicast transmission signaling message TLV 
 * offset of peer address (octets). 
 */
#define PTP_UCSIG_PEERADDR_OFFSET_OCTETS                               (11)
/* 
 * PTP unicast transmission signaling message TLV 
 * offset of L2 header length value (octets). 
 */
#define PTP_UCSIG_L2HDRLEN_OFFSET_OCTETS                               (27)
/* 
 * PTP unicast transmission signaling message TLV 
 * offset of L2 header (octets).
 */
#define PTP_UCSIG_L2HDR_OFFSET_OCTETS                                  (28)   

/* 
 * PTP cancel unicast transmission signaling message TLV 
 * offset of reserved field (octets). 
 */
#define PTP_UCSIG_CANCEL_RESERVED_OFFSET_OCTETS                         (5)
/* 
 * PTP cancel unicast transmission signaling message TLV 
 * offset of address type (octets). 
 * NOTICE: Address type and peer address data are custom 
 *         additions to TLV data.
 */
#define PTP_UCSIG_CANCEL_ADDRTYPE_OFFSET_OCTETS                         (6)
/* 
 * PTP cancel unicast transmission signaling message TLV 
 * offset of peer address (octets). 
 * NOTICE: Address type and peer address data are custom 
 *         additions to TLV data.
 */
#define PTP_UCSIG_CANCEL_PEERADDR_OFFSET_OCTETS                         (7)

/* 
 * PTP unicast transmission signaling message 
 * REQUEST_UNICAST_TRANSMISSION TLV total size (octets).
 */
#define PTP_UCSIG_REQUEST_TOTAL_SIZE_OCTETS  (PTP_UCSIG_TLV_START_IDX + \
                                              PTP_UCSIG_REQUEST_TLV_SIZE_OCTETS)

/* 
 * PTP unicast transmission signaling message 
 * CANCEL_UNICAST_TRANSMISSION TLV total size (octets).
 */
#define PTP_UCSIG_CANCEL_TOTAL_SIZE_OCTETS   (PTP_UCSIG_TLV_START_IDX + \
                                              PTP_UCSIG_CANCEL_TLV_SIZE_OCTETS)

/* PTP clock presets. */
#define PTP_CLOCK_PRESETS_ANNOUNCE_RECEIPT_TIMEOUT_MINIMUM              (2)
#define PTP_CLOCK_PRESETS_ANNOUNCE_RECEIPT_TIMEOUT_DEFAULT              (3)
#define PTP_CLOCK_PRESETS_ANNOUNCE_RECEIPT_TIMEOUT_MAXIMUM             (10)

#define PTP_CLOCK_PRESETS_LOG_ANNOUNCE_INTERVAL_MINIMUM                (-2)
#define PTP_CLOCK_PRESETS_LOG_ANNOUNCE_INTERVAL_DEFAULT                 (1)
#define PTP_CLOCK_PRESETS_LOG_ANNOUNCE_INTERVAL_MAXIMUM                 (4)
   
#define PTP_CLOCK_PRESETS_LOG_SYNC_INTERVAL_MINIMUM                    (-7)
#define PTP_CLOCK_PRESETS_LOG_SYNC_INTERVAL_DEFAULT                    (-5)
#define PTP_CLOCK_PRESETS_LOG_SYNC_INTERVAL_MAXIMUM                     (1)
      
#define PTP_CLOCK_PRESETS_LOG_MIN_DELAY_REQ_INTERVAL_MINIMUM           (-7)
#define PTP_CLOCK_PRESETS_LOG_MIN_DELAY_REQ_INTERVAL_DEFAULT           (-5)
#define PTP_CLOCK_PRESETS_LOG_MIN_DELAY_REQ_INTERVAL_MAXIMUM            (5)

#define PTP_CLOCK_PRESETS_DOMAIN_NUMBER_MINIMUM                         (0)
#define PTP_CLOCK_PRESETS_DOMAIN_NUMBER_DEFAULT                         (0)
#define PTP_CLOCK_PRESETS_DOMAIN_NUMBER_MAXIMUM                       (255)

#define PTP_CLOCK_PRESETS_PRIORITY1_MINIMUM                             (0)
#define PTP_CLOCK_PRESETS_PRIORITY1_DEFAULT                           (128)
#define PTP_CLOCK_PRESETS_PRIORITY1_MAXIMUM                           (255)

#define PTP_CLOCK_PRESETS_PRIORITY2_MINIMUM                             (0)
#define PTP_CLOCK_PRESETS_PRIORITY2_DEFAULT                           (128)
#define PTP_CLOCK_PRESETS_PRIORITY2_MAXIMUM                           (255)

#define PTP_CLOCK_PRESETS_TELECOM_LOG_ANNOUNCE_INTERVAL_MINIMUM        (-3)
#define PTP_CLOCK_PRESETS_TELECOM_LOG_ANNOUNCE_INTERVAL_DEFAULT         (1)
#define PTP_CLOCK_PRESETS_TELECOM_LOG_ANNOUNCE_INTERVAL_MAXIMUM         (4)
   
#define PTP_CLOCK_PRESETS_TELECOM_LOG_SYNC_INTERVAL_MINIMUM            (-7)
#define PTP_CLOCK_PRESETS_TELECOM_LOG_SYNC_INTERVAL_DEFAULT            (-5)
#define PTP_CLOCK_PRESETS_TELECOM_LOG_SYNC_INTERVAL_MAXIMUM             (4)
      
#define PTP_CLOCK_PRESETS_TELECOM_LOG_MIN_DELAY_REQ_INTERVAL_MINIMUM   (-7)
#define PTP_CLOCK_PRESETS_TELECOM_LOG_MIN_DELAY_REQ_INTERVAL_DEFAULT   (-5)
#define PTP_CLOCK_PRESETS_TELECOM_LOG_MIN_DELAY_REQ_INTERVAL_MAXIMUM    (4)

#define PTP_CLOCK_PRESETS_TELECOM_DOMAIN_NUMBER_MINIMUM                 (4)
#define PTP_CLOCK_PRESETS_TELECOM_DOMAIN_NUMBER_DEFAULT                 (4)
#define PTP_CLOCK_PRESETS_TELECOM_DOMAIN_NUMBER_MAXIMUM                (23)

/*
 * PTP clock configuration flags.
 * NOTE: Flags complement those defined in /include/bcm/ptp.h.
 */

/* Sync-triggered Delay Requests */
#define PTP_CLOCK_FLAGS_TRIGGERED_DELREQ                               (1u)
/* Fixed-interval Delay Requests */
#define PTP_CLOCK_FLAGS_FIXED_DELREQ                                   (2u)
/* One-way operation bit. */
#define PTP_CLOCK_FLAGS_ONEWAY_BIT                                     (3u)
/* Telecom profile bit. */
#define PTP_CLOCK_FLAGS_TELECOM_PROFILE_BIT                            (4u)
/* Bit 5 currently unused */
/* */
/* Alternate master Delay_Req messages bit. */
#define PTP_CLOCK_FLAGS_DELREQ_ALTMASTER_BIT                           (6u)
/* Set Sync OAM register, and override servo-reported phase */
#define PTP_CLOCK_FLAGS_SYNCOAM_BIT                                    (7u)

/* Not passed through to TOP: */
/* Multi-TLV signaling message support bit. */
#define PTP_CLOCK_FLAGS_SIGNALING_MULTI_TLV_BIT                       (30u)

/* PTP clock MAC address size (bytes). */
#define PTP_MAC_ADDR_SIZE_BYTES   (6)
/* PTP clock IPv4 address size. */
#define PTP_IPV4_ADDR_SIZE_BYTES  (4)
/* PTP clock IPv6 address size. */
#define PTP_IPV6_ADDR_SIZE_BYTES (16)
/* Maximum size of address (octets). */
#define PTP_MAX_NETW_ADDR_SIZE   (16)
/*
 * Maximum length of packet header before IPv4/IPv6 header.
 * NOTE: Device-side compliant L2 header size. 
 */
#define PTP_MAX_L2_HEADER_LENGTH (32)
/* Size of L2 headers used in messaging to TOP */
#define PTP_MSG_L2_HEADER_LENGTH (64)
/*
 * Maximum number of entries in a unicast master table of a PTP clock.
 * NOTICE: This value may not be configurable for some ToP Firmwares.
 */
#ifndef PTP_MAX_UNICAST_MASTER_TABLE_ENTRIES
#define PTP_MAX_UNICAST_MASTER_TABLE_ENTRIES      (10)
#endif

/* Maximum number of entries in an acceptable master table of a PTP clock.
 * NOTICE: This value may not be configurable for some ToP Firmwares.
 */
#ifndef PTP_MAX_ACCEPTABLE_MASTER_TABLE_ENTRIES
#define PTP_MAX_ACCEPTABLE_MASTER_TABLE_ENTRIES   (PTP_MAX_UNICAST_MASTER_TABLE_ENTRIES)
#endif

/* Maximum total number of unicast slave entries
 * NOTICE: This value may not be configurable for some ToP Firmwares.
 */
#if defined(PTP_KEYSTONE_STACK)
/* Smaller limits on peers & slaves for Keystone stack */
 #ifndef PTP_MAX_UNICAST_SLAVE_TABLE_ENTRIES
  #define PTP_MAX_UNICAST_SLAVE_TABLE_ENTRIES       (50)
 #endif /* PTP_MAX_UNICAST_SLAVE_TABLE_ENTRIES */
#else
 /* Non-Keystone stack */
 #ifndef PTP_MAX_UNICAST_SLAVE_TABLE_ENTRIES
  #define PTP_MAX_UNICAST_SLAVE_TABLE_ENTRIES       (250)   
 #endif /* PTP_MAX_UNICAST_SLAVE_TABLE_ENTRIES */
#endif /* PTP_KEYSTONE_STACK */

/* Maximum number of multicast slave stats entries.
 * NOTICE: This value may not be configurable for some ToP Firmwares.
 */
#ifndef PTP_MAX_MULTICAST_SLAVE_STATS_ENTRIES
#define PTP_MAX_MULTICAST_SLAVE_STATS_ENTRIES       (40)
#endif

/* Infinite unicast message transmission duration. */
#define PTP_UNICAST_NEVER_EXPIRE (0xffffffff)

/* Flags. */
#define PTP_SIGNALING_ANNOUNCE_SERVICE_BIT                             (0u)
#define PTP_SIGNALING_SYNC_SERVICE_BIT                                 (1u)
#define PTP_SIGNALING_DELAYRESP_SERVICE_BIT                            (2u)



/* Data protection macros. */
#define PTP_MUTEX_TAKE(__mutex__, __usec__)                                      \
    do {                                                                         \
        int __rv__ = _bcm_ptp_mutex_take(__mutex__, __usec__);                   \
        if (BCM_FAILURE(__rv__)) {                                               \
            bsl_printf("%s() failed _bcm_ptp_mutex_take() returned %d : %s\n", \
                       FUNCTION_NAME(), __rv__, bcm_errmsg(__rv__));    \
            return BCM_E_INTERNAL;                                               \
        }                                                                        \
    } while (0)

#define PTP_MUTEX_RELEASE_RETURN(__mutex__, __rv__) \
    do {                                            \
        _bcm_ptp_mutex_give(__mutex__);             \
        return __rv__;                              \
    } while (0)

/* Types. */
typedef enum _bcm_ptp_memstate_e
{
    PTP_MEMSTATE_FAILURE     = -1,
    PTP_MEMSTATE_STARTUP     =  0,
    PTP_MEMSTATE_INITIALIZED =  1
} _bcm_ptp_memstate_t;

/* dispatchable: initialize the transport mechanism to the ToP */
typedef int _bcm_ptp_transport_init_t(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    bcm_mac_t *src_mac,
    bcm_mac_t *dest_mac,
    bcm_ip_t src_ip,
    bcm_ip_t dest_ip,
    uint16 vlan,
    uint8 prio,
    bcm_pbmp_t top_bitmap);

typedef enum _bcm_ptp_transport_type_e {
    PTP_MESSAGE_TO_TOP,
    PTP_TUNNEL_TO_TOP,
    PTP_TUNNEL_FROM_TOP
} _bcm_ptp_transport_type_t;

/* dispatchable: transmit a command to the ToP */
typedef int _bcm_ptp_transport_tx_t(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    _bcm_ptp_transport_type_t transport,
    uint8 *hdr,
    int hdr_len,
    uint8 *message,
    int message_len);

/* dispatchable: wait (if applicable) until the last command to the ToP has been received */
typedef int _bcm_ptp_transport_tx_completion_t(
    int unit,
    bcm_ptp_stack_id_t ptp_id);

/* dispatchable: free a buffer containing a received response */
typedef int _bcm_ptp_transport_rx_free_t(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    uint8 *data);

/* dispatchable: tear down the transport mechanism to the ToP */
typedef int _bcm_ptp_transport_terminate_t(
    int unit,
    bcm_ptp_stack_id_t ptp_id);


/****************************************************/
/* constants shared with firmware */

#define MBOX_STATUS_EMPTY                    (0)
#define MBOX_STATUS_ATTN_TOP_CMD             (1)
#define MBOX_STATUS_ATTN_TOP_TUNNEL_TO_TOP   (2)
#define MBOX_STATUS_ATTN_TOP_TUNNEL_EXTERNAL (3)
#define MBOX_STATUS_ATTN_HOST_RESP           (4)
#define MBOX_STATUS_ATTN_HOST_EVENT          (5)
#define MBOX_STATUS_ATTN_HOST_TUNNEL         (6)
#define MBOX_STATUS_PENDING_TOP              (7)
#define MBOX_STATUS_PENDING_HOST             (8)

#define MBOX_TUNNEL_MSG_TYPE_OFFSET      (0)
#define MBOX_TUNNEL_PORT_NUM_OFFSET      (2)
#define MBOX_TUNNEL_PROTOCOL_OFFSET      (4)
#define MBOX_TUNNEL_SRC_ADDR_OFFS_OFFSET (6)
#define MBOX_TUNNEL_PTP_OFFS_OFFSET      (8)
#define MBOX_TUNNEL_PACKET_OFFSET        (10)

/* Servo fixed-point conversion factors. */
#define PTP_SERVO_OCXO_FREQ_OFFSET_FIXEDPOINT_SCALEFACTOR            (1000)
#define PTP_SERVO_FWD_FLOW_WEIGHT_FIXEDPOINT_SCALEFACTOR             (1000)
#define PTP_SERVO_FWD_FLOW_TRANSACTIONS_PCT_FIXEDPOINT_SCALEFACTOR   (1000)
#define PTP_SERVO_FWD_MIN_TDEV_FIXEDPOINT_SCALEFACTOR                (1000)
#define PTP_SERVO_FWD_MAFIE_FIXEDPOINT_SCALEFACTOR                   (1000)
#define PTP_SERVO_FWD_MIN_CLUSTER_WIDTH_FIXEDPOINT_SCALEFACTOR       (1000)
#define PTP_SERVO_FWD_MODE_WIDTH_FIXEDPOINT_SCALEFACTOR              (1000)
#define PTP_SERVO_REV_FLOW_WEIGHT_FIXEDPOINT_SCALEFACTOR             (1000)
#define PTP_SERVO_REV_FLOW_TRANSACTIONS_PCT_FIXEDPOINT_SCALEFACTOR   (1000)
#define PTP_SERVO_REV_MIN_TDEV_FIXEDPOINT_SCALEFACTOR                (1000)
#define PTP_SERVO_REV_MAFIE_FIXEDPOINT_SCALEFACTOR                   (1000)
#define PTP_SERVO_REV_MIN_CLUSTER_WIDTH_FIXEDPOINT_SCALEFACTOR       (1000)
#define PTP_SERVO_REV_MODE_WIDTH_FIXEDPOINT_SCALEFACTOR              (1000)
#define PTP_SERVO_FREQ_CORRECTION_FIXEDPOINT_SCALEFACTOR             (1000)
#define PTP_SERVO_PHASE_CORRECTION_FIXEDPOINT_SCALEFACTOR            (1000)
#define PTP_SERVO_TDEV_ESTIMATE_FIXEDPOINT_SCALEFACTOR               (1000)
#define PTP_SERVO_MDEV_ESTIMATE_FIXEDPOINT_SCALEFACTOR               (1000)
#define PTP_SERVO_RESIDUAL_PHASE_ERROR_FIXEDPOINT_SCALEFACTOR        (1000)
#define PTP_SERVO_MIN_ROUNDTRIP_DELAY_FIXEDPOINT_SCALEFACTOR         (1000)
#define PTP_SERVO_FWD_IPDVPCT_FIXEDPOINT_SCALEFACTOR                 (1000)
#define PTP_SERVO_FWD_IPDVMAX_FIXEDPOINT_SCALEFACTOR                 (1000)
#define PTP_SERVO_FWD_INTERPKT_JITTER_FIXEDPOINT_SCALEFACTOR         (1000)
#define PTP_SERVO_REV_IPDVPCT_FIXEDPOINT_SCALEFACTOR                 (1000)
#define PTP_SERVO_REV_IPDVMAX_FIXEDPOINT_SCALEFACTOR                 (1000)
#define PTP_SERVO_REV_INTERPKT_JITTER_FIXEDPOINT_SCALEFACTOR         (1000)

#define BCM_PTP_RXMAP_ACCEPT_MULTICAST   (1 << 0)
#define BCM_PTP_RXMAP_MATCH_OUTER_VLAN   (1 << 1)
#define BCM_PTP_RXMAP_MATCH_INNER_VLAN   (1 << 2)
#define BCM_PTP_RXMAP_MATCH_INGRESS_PORT (1 << 3)
#define BCM_PTP_RXMAP_MATCH_ANY_ADDR     (1 << 4)

/* Legacy flags that can be added to oscillator type instead */
#define BCM_PTP_FLAG_LOCK_ENDURO_FREQ_MULT   (64)
#define BCM_PTP_FLAG_IGNORE_SERVO_OUTPUT     (128)                  

typedef struct _bcm_ptp_servo_configuration_s {
    /* Local oscillator type. */
    bcm_ptp_osc_type_t osc_type;
    /* PTP transport mode. */
    bcm_ptp_transport_mode_t transport_mode;
    /* Bit-mapped phase mode of servo loop. */
    uint32 ptp_phase_mode;
    /* OCXO frequency offset */
    int32 freq_corr;
    /* Timestamp for frequency offset.*/
    bcm_ptp_timestamp_t freq_corr_time;
    /* Bridge time in nanoseconds. */
    uint32 bridge_time;
    uint32 flags;
} _bcm_ptp_servo_configuration_t;
 

/****************************************************/
/* structures shared with firmware */

/*   mbox 0 is for cmd/resp.  Others are for async event & msg to host */

typedef struct _bcm_ptp_internal_stack_mbox_s {
    volatile uint32 status;     /* MBOX_STATUS_x value */
    volatile uint32 clock_num;  /* clock number that the message is addressed to/from */
    volatile uint32 data_len;
    volatile uint32 rfu_mbox;
#if defined BCM_PTP_DMA_ALIGNMENT
    uint8 alignment[BCM_PTP_DMA_ALIGNMENT - 16];  /* 16 bytes in the above already */
#endif /* BCM_PTP_DMA_ALIGNMENT */
    volatile uint8 data[1536];  /* at offset of 'mbox_offset' from start of struct */
} _bcm_ptp_internal_stack_mbox_t;

#define GET_MBOX_STATUS(stack,idx)                                               \
    ( soc_cm_sinval(unit, (void*)&(stack)->int_state.mboxes->mbox[(idx)].status, \
                    sizeof((stack)->int_state.mboxes->mbox[(idx)].status)),      \
      soc_ntohl((stack)->int_state.mboxes->mbox[(idx)].status)          \
    )

#define SET_MBOX_STATUS(stack,idx,val)                                           \
    ( (stack)->int_state.mboxes->mbox[(idx)].status = soc_htonl(val),            \
      soc_cm_sflush(unit, (void*)&(stack)->int_state.mboxes->mbox[(idx)].status, \
                    sizeof((stack)->int_state.mboxes->mbox[(idx)].status))       \
    )

typedef struct _bcm_ptp_internal_stack_mboxes_s {
    volatile uint32 num_mboxes;                     /* == BCM_PTP_MAX_BUFFERS */
    volatile uint32 mbox_size;
    volatile uint32 mbox_offset;  /* for both 'mbox' field and 'data' field in mbo struct */
    volatile uint32 rfu;
#if defined BCM_PTP_DMA_ALIGNMENT
    uint8 alignment[BCM_PTP_DMA_ALIGNMENT - 16];  /* 16 bytes in the above already */
#endif /* BCM_PTP_DMA_ALIGNMENT */
    /* note: now at offset of 'mbox_offset' */
    volatile _bcm_ptp_internal_stack_mbox_t mbox[BCM_PTP_MAX_BUFFERS]; /* contents of mbox */
} _bcm_ptp_internal_stack_mboxes_t;

typedef struct _bcm_ptp_internal_stack_log_s {
    volatile uint32 size;                         /* allocated size of buf (below)              */
    volatile uint32 head;                         /* host-read, updated by CMICm                */
    volatile uint8 buf[4];                        /* '4' is placeholder.  Actual size is 'size' */
} _bcm_ptp_internal_stack_log_t;

typedef struct _bcm_ptp_internal_stack_state_s {
    /* network-byte-ordered physical memory addresses of the shared structures */
    volatile uint32 mbox_ptr;
    volatile uint32 log_ptr;

    _bcm_ptp_internal_stack_mboxes_t * mboxes;
    _bcm_ptp_internal_stack_log_t * log;

    /* Elements after this point are not accessed by firmware */
    int core_num;
    uint32 log_tail;
} _bcm_ptp_internal_stack_state_t;


/****************************************************/

#define PTP_MAX_STACKS_PER_UNIT       (1)
#define PTP_MAX_CLOCKS_PER_STACK      (1)
#define PTP_MAX_CLOCK_INSTANCES       (1)
#define PTP_MAX_CLOCK_INSTANCE_PORTS  (32)                  
#define PTP_MAX_ORDINARY_CLOCK_PORTS  (1)
#define PTP_MAX_TIME_SOURCES          (3)
#define PTP_MAX_OUTPUT_SIGNALS        (7)
#define PTP_GPIO_OUTPUT_SIGNALS       (6)
#if defined(BCM_PTP_INTERNAL_STACK_SUPPORT)
#define PTP_OUTPUT_SIGNAL_SYNCE       (6)
#else
#define PTP_OUTPUT_SIGNAL_SYNCE       (-1)
#endif
#define PTP_MAX_TOD_INPUTS            (1)
#define PTP_MAX_TOD_OUTPUTS           (2)

#define PTP_UNIT_NUMBER_DEFAULT       (0)
#define PTP_STACK_ID_DEFAULT          (0)
#define PTP_CLOCK_NUMBER_DEFAULT      (0)
#define PTP_CLOCK_PORT_NUMBER_DEFAULT (1)

#if defined(PTP_KEYSTONE_STACK)
/* Smaller limits on peers & slaves for Keystone stack */
#define _PTP_MAX_PEER_DATASET_ENTRIES           100
#define _PTP_MAX_PEER_DATASET_CHUNK_SIZE        10
#define _PTP_MAX_FOREIGN_MASTER_DATASET_ENTRIES 10
#else
/* Non-Keystone stack */
#define _PTP_MAX_PEER_DATASET_CHUNK_SIZE        10
#define _PTP_MAX_FOREIGN_MASTER_DATASET_ENTRIES 10
#define _PTP_MAX_PEER_DATASET_ENTRIES           PTP_MAX_UNICAST_SLAVE_TABLE_ENTRIES + \
                                                PTP_MAX_CLOCK_INSTANCE_PORTS + \
                                                _PTP_MAX_FOREIGN_MASTER_DATASET_ENTRIES
#endif  /* PTP_KEYSTONE_STACK */

/* Peer Dataset */
typedef struct bcm_ptp_peer_dataset_s {
    uint16 num_peers;
    bcm_ptp_peer_entry_t peer[_PTP_MAX_PEER_DATASET_ENTRIES];
} bcm_ptp_peer_dataset_t;


/*
 * PTP state enumeration.
 * Ref. IEEE Std. 1588-2008, Chapter 8.2.5.3.1.
 */
typedef enum _bcm_ptp_port_state_e {
    _bcm_ptp_state_initializing = 1,
    _bcm_ptp_state_faulty       = 2,
    _bcm_ptp_state_disabled     = 3,
    _bcm_ptp_state_listening    = 4,
    _bcm_ptp_state_pre_master   = 5,
    _bcm_ptp_state_master       = 6,
    _bcm_ptp_state_passive      = 7,
    _bcm_ptp_state_uncalibrated = 8,
    _bcm_ptp_state_slave        = 9
} _bcm_ptp_port_state_t;

/*
 * PTP Boot type enumeration.
 */
typedef enum _bcmPTPBootType_e {
    BCM_PTP_COLDSTART,
    BCM_PTP_WARMSTART
} _bcmPTPBootType_t;

/* Enumeration of events which can generate a callback */
typedef enum _bcm_ptp_event_e {
    _bcm_ptp_state_change_event    = 0x01,
    _bcm_ptp_master_change_event   = 0x02,
    _bcm_ptp_master_avail_event    = 0x03,
    _bcm_ptp_master_unavail_event  = 0x04,
    _bcm_ptp_slave_avail_event     = 0x05,
    _bcm_ptp_slave_unavail_event   = 0x06,
    _bcm_ptp_top_oom_event         = 0x07,
    _bcm_ptp_top_watchdog_event    = 0x08,
    _bcm_ptp_top_ready_event       = 0x09,
    _bcm_ptp_top_misc_event        = 0x0a,
    _bcm_ptp_top_tod_avail_event   = 0x0b,
    _bcm_ptp_top_tod_unavail_event = 0x0c,
    _bcm_ptp_ieee1588_warn_event   = 0x0d,
    _bcm_ptp_servo_state_event     = 0x0e,
    _bcm_ptp_pps_in_state_event    = 0x0f,
    _bcm_ptp_tdev_event            = 0x10,
    _bcm_ptp_servo_log_event       = 0x11,
    _bcm_ptp_last_event
} _bcm_ptp_event_t;

/*
 * Event support (STATE CHANGE).
 * Reason for a change in PTP port state.
 */
typedef enum _bcm_ptp_state_change_reason_e {
   /* Startup, instance creation. */
   _bcm_ptp_state_change_reason_startup         = 0,
   /* Port initialization. */
   _bcm_ptp_state_change_reason_port_init       = 1,
   /* Fault detected. */
   _bcm_ptp_state_change_reason_fault           = 2,
   /* BMCA state transition. */
   _bcm_ptp_state_change_reason_bmca            = 3,
   /* Enable port management message. */
   _bcm_ptp_state_change_reason_mnt_enable      = 4,
   /* Disable port management message. */
   _bcm_ptp_state_change_reason_mnt_disable     = 5,
   /* Network interface re-initialization. */
   _bcm_ptp_state_change_reason_netw_reinit     = 6,
   /* Timestamp difference, master-to-slave. */
   _bcm_ptp_state_change_reason_dt_master_slave = 7,
   _bcm_ptp_state_change_reason_last
} _bcm_ptp_state_change_reason_t;

/*
 * Event support (IEEE Std. 1588-2008 WARNING).
 * Reason for a warning related to IEEE Std. 1588-2008.
 */
typedef enum _bcm_ptp_ieee1588_warn_reason_e {
   /* Non-uniform logAnnounceInterval in a PTP domain. */
   _bcm_ptp_ieee1588_warn_reason_logAnnounceInterval = 0,
   _bcm_ptp_ieee1588_warn_reason_last
} _bcm_ptp_ieee1588_warn_reason_t;

/* Event support (PPS-IN STATE). */
typedef enum _bcm_ptp_pps_in_state_e {
   _bcm_ptp_pps_in_state_missing            = 0,
   _bcm_ptp_pps_in_state_active_missing_tod = 1,
   _bcm_ptp_pps_in_state_valid              = 2
} _bcm_ptp_pps_in_state_t;

/* PTP stack information. */
typedef struct _bcm_ptp_stack_info_s
{
    int in_use;
    _bcm_ptp_memstate_t memstate;
    bcm_ptp_external_stack_info_t ext_info;
    _bcm_ptp_internal_stack_state_t int_state;

    int unit;
    bcm_ptp_stack_id_t stack_id;
    int unicast_master_table_size;
    int acceptable_master_table_size;
    int unicast_slave_table_size;
    int multicast_slave_stats_size;

    uint32 flags;
#if defined(PTP_KEYSTONE_STACK)
    void *cookie;
    uint8 persistent_config[CONFIG_TOTAL_SIZE];
#endif    

    /* dispatch table */
    _bcm_ptp_transport_init_t *transport_init;
    _bcm_ptp_transport_tx_t *tx;                            /* transmit a message to the ToP */
    _bcm_ptp_transport_tx_completion_t *tx_completion;      /* wait until last tx has been received by ToP */
    _bcm_ptp_transport_rx_free_t *rx_free;                  /* release a received message from the ToP */
    _bcm_ptp_transport_terminate_t *transport_terminate;

} _bcm_ptp_stack_info_t;

typedef struct _bcm_ptp_info_s
{
    _bcm_ptp_memstate_t memstate;
    sal_mutex_t mutex;
    _bcm_ptp_stack_info_t *stack_info;
} _bcm_ptp_info_t;

extern _bcm_ptp_info_t _bcm_common_ptp_info[BCM_MAX_NUM_UNITS];
#define SET_PTP_INFO ptp_info_p = &_bcm_common_ptp_info[unit]

/* PTP clock and clock port information cache. */
typedef struct _bcm_ptp_clock_cache_s
{
    int in_use;
    int max_num_ports;   /* Set on initial clock configuration.  Clock can be reconfigured with fewer, but not more */
    bcm_ptp_clock_info_t  clock_info;
    bcm_ptp_signal_output_t signal_output[PTP_MAX_OUTPUT_SIGNALS];
    bcm_ptp_tod_input_t tod_input[PTP_MAX_TOD_INPUTS];
    bcm_ptp_tod_output_t tod_output[PTP_MAX_TOD_OUTPUTS];
    bcm_ptp_clock_port_info_t port_info[PTP_MAX_CLOCK_INSTANCE_PORTS];
    _bcm_ptp_port_state_t portState[PTP_MAX_CLOCK_INSTANCE_PORTS];
} _bcm_ptp_clock_cache_t;

/* PTP stack clock and port information caches. */
typedef struct _bcm_ptp_stack_cache_s
{
    _bcm_ptp_memstate_t memstate;
    _bcm_ptp_clock_cache_t *clock_array;
} _bcm_ptp_stack_cache_t;

/* Unit clock and port information caches. */
typedef struct _bcm_ptp_unit_cache_s
{
    _bcm_ptp_memstate_t memstate;
    _bcm_ptp_stack_cache_t *stack_array;
} _bcm_ptp_unit_cache_t;

/* PTP PCI setconfig */
typedef int (*bcm_ptp_pci_setconfig_t) (uint32 pci_dev_id, uint32 pciconfig_register, uint32 value);

typedef struct _bcm_ptp_ipdv_config_s {
    uint16  observation_interval; /* IPDV observation interval. */
    int32   threshold;            /* IPDV threshold (nsec).     */
    uint16  pacing_factor;        /* IPDV pacing factor.        */
} _bcm_ptp_ipdv_config_t;

typedef struct _bcm_ptp_ipdv_performance_s {
    uint64  fwd_pct;    /* Forward IPDV % below threshold.     */
    uint64  fwd_max;    /* Forward maximum IPDV (usec).        */
    uint64  fwd_jitter; /* Forward interpacket jitter (usec).  */
    uint64  rev_pct;    /* Reverse IPDV % below threshold.     */
    uint64  rev_max;    /* Reverse maximum IPDV (usec).        */
    uint64  rev_jitter; /* Reverse interpacket jitter (usec).  */
} _bcm_ptp_ipdv_performance_t;

typedef enum _bcm_ptp_channel_sub_status_e {
    _bcm_ptp_channel_status_ok = 0,
    _bcm_ptp_channel_status_fault = 1,
    _bcm_ptp_channel_status_disabled = 2
} _bcm_ptp_channel_sub_status_t;

typedef struct _bcm_ptp_channel_status_s {
    _bcm_ptp_channel_sub_status_t freq_status;
    _bcm_ptp_channel_sub_status_t time_status;
    int freq_weight;
    int time_weight;
    int freq_QL;
    int time_QL;
    int ql_read_externally;
    uint32 fault_map;
} _bcm_ptp_channel_status_t;


typedef struct _bcm_ptp_servo_performance_s {
    /* FLL state data. */
    bcm_ptp_servo_status_t status;
    /* Forward flow weight. */
    uint64 fwd_weight;         
    /* Forward flow transient-free out of 900 sec. */  
    uint32 fwd_trans_free_900;
    /* Forward flow transient-free out of 3600 sec. */  
    uint32 fwd_trans_free_3600;
    /* Forward flow transactions used (%). */
    uint64 fwd_pct;             
    /* Forward flow operational minimum TDEV (nsec). */
    uint64 fwd_min_Tdev;        
    /* Forward Mafie. */
    uint64 fwd_Mafie;
    /* Forward flow minimum cluster width (nsec).*/
    uint64 fwd_min_cluster_width;
    /* Forward flow mode width (nsec). */
    uint64 fwd_mode_width;       
    /* Reverse flow weight. */
    uint64 rev_weight;           
    /* Reverse flow transient-free out of 900 sec. */  
    uint32 rev_trans_free_900;
    /* Reverse flow transient-free out of 3600 sec. */  
    uint32 rev_trans_free_3600;
    /* Reverse flow transactions used (%). */
    uint64 rev_pct;
    /* Reverse flow operational minimum TDEV (nsec). */
    uint64 rev_min_Tdev;
    /* Reverse Mafie. */
    uint64 rev_Mafie;
    /* Reverse flow minimum cluster width (nsec).*/
    uint64 rev_min_cluster_width;
    /* Reverse flow mode width (nsec). */
    uint64 rev_mode_width;
    /* Frequency correction (ppb). */
    int64 freq_correction; 
    /* Phase correction (ppb).*/
    int64 phase_correction;     
    /* Output TDEV estimate (nsec). */
    uint64 tdev_estimate;  
    /* Output MDEV estimate (ppb). */
    uint64 mdev_estimate;  
    /* Residual phase error (nsec). */
    int64 residual_phase_error;   
    /* Minimum round trip delay (nsec). */
    uint64 min_round_trip_delay;
    /* Sync packet rate (pkts/sec). */
    uint16 fwd_pkt_rate;  
    /* Delay packet rate (pkts/sec). */
    uint16 rev_pkt_rate;
    /* IPDV performance data / metrics. */
    _bcm_ptp_ipdv_performance_t ipdv_data;   
} _bcm_ptp_servo_performance_t;

/* Extended Peer Info Structure.  Includes local port address, for use with wildcard port */
typedef struct _bcm_ptp_clock_peer_ext_s {
    bcm_ptp_clock_peer_t peer;
    bcm_ptp_clock_port_address_t local_address;
} _bcm_ptp_clock_peer_ext_t;

/* Telecom profile definitions and data types. */
#define PTP_TELECOM_NETWORKING_OPTION_DEFAULT (bcm_ptp_telecom_g8265_network_option_II)
#define PTP_TELECOM_MAX_NUMBER_PKTMASTERS (PTP_MAX_UNICAST_MASTER_TABLE_ENTRIES)
#define PTP_TELECOM_ANNOUNCE_RECEIPT_TIMEOUT_MSEC_DEFAULT                 (6000)
#define PTP_TELECOM_SYNC_RECEIPT_TIMEOUT_MSEC_DEFAULT                     (1000)
#define PTP_TELECOM_DELAYRESP_RECEIPT_TIMEOUT_MSEC_DEFAULT                (1000)
#define PTP_TELECOM_PDV_SCALED_ALLAN_VARIANCE_THRESHOLD_NSECSQ_DEFAULT (1000000000)
#define PTP_TELECOM_NON_REVERSION_MODE_DEFAULT                               (0)
#define PTP_TELECOM_WAIT_TO_RESTORE_SEC_DEFAULT                            (300)

#define PTP_TELECOM_EVENT_FIFO_SIZE                                         (30)
 
#define PTP_TELECOM_EVENT_PTSF_LOSS_ANNOUNCE_BIT                            (0u)
#define PTP_TELECOM_EVENT_PTSF_LOSS_SYNC_BIT                                (1u)
#define PTP_TELECOM_EVENT_PTSF_LOSS_SYNC_TS_BIT                             (2u)
#define PTP_TELECOM_EVENT_PTSF_LOSS_DELAY_RESP_BIT                          (3u)
#define PTP_TELECOM_EVENT_PTSF_UNUSABLE_BIT                                 (4u)
#define PTP_TELECOM_EVENT_QL_DEGRADED_BIT                                   (8u)
#define PTP_TELECOM_EVENT_GM_ADDED_BIT                                     (16u)
#define PTP_TELECOM_EVENT_GM_REMOVED_BIT                                   (17u)
#define PTP_TELECOM_EVENT_GM_SELECTED_BIT                                  (18u)

/* Telecom events. */
typedef struct _bcm_ptp_telecom_g8265_event_s {
    uint32 type;
    sal_time_t timestamp;
    bcm_ptp_telecom_g8265_pktmaster_t pktmaster;
} _bcm_ptp_telecom_g8265_event_t;

typedef struct _bcm_ptp_telecom_g8265_event_fifo_s {
    int head;
    _bcm_ptp_telecom_g8265_event_t events[PTP_TELECOM_EVENT_FIFO_SIZE];
} _bcm_ptp_telecom_g8265_event_fifo_t;


/* T-DPLL definitions and data types. */
#define TDPLL_INPUT_CLOCK_NUM_GPIO  (BCM_TDPLL_INPUT_CLOCK_NUM_GPIO)
#define TDPLL_INPUT_CLOCK_NUM_SYNCE (BCM_TDPLL_INPUT_CLOCK_NUM_SYNCE)
#define TDPLL_INPUT_CLOCK_NUM_1588  (BCM_TDPLL_INPUT_CLOCK_NUM_1588)
#define TDPLL_INPUT_CLOCK_NUM_MAX   (BCM_TDPLL_INPUT_CLOCK_NUM_MAX)

#define TDPLL_OUTPUT_CLOCK_NUM_BROADSYNC (BCM_TDPLL_OUTPUT_CLOCK_NUM_BROADSYNC)
#define TDPLL_OUTPUT_CLOCK_NUM_SYNCE     (BCM_TDPLL_OUTPUT_CLOCK_NUM_SYNCE)
#define TDPLL_OUTPUT_CLOCK_NUM_1588      (BCM_TDPLL_OUTPUT_CLOCK_NUM_1588)
#define TDPLL_OUTPUT_CLOCK_NUM_GPIO      (BCM_TDPLL_OUTPUT_CLOCK_NUM_GPIO)
#define TDPLL_OUTPUT_CLOCK_NUM_MAX       (BCM_TDPLL_OUTPUT_CLOCK_NUM_MAX)

#define TDPLL_DPLL_INSTANCE_NUM_MAX (BCM_TDPLL_DPLL_INSTANCE_NUM_MAX)

typedef struct bcm_tdpll_dpll_instance_data_s {
    bcm_tdpll_dpll_instance_t dpll_instance[TDPLL_DPLL_INSTANCE_NUM_MAX];
} bcm_tdpll_dpll_instance_data_t;

typedef struct bcm_tdpll_input_clock_monitor_options_s {
    uint32 interval;

    uint32 soft_warn_threshold_ppb;
    uint32 hard_accept_threshold_ppb;
    uint32 hard_reject_threshold_ppb;
} bcm_tdpll_input_clock_monitor_options_t;

typedef struct bcm_tdpll_input_clock_selector_options_s {
    int ql_enabled[TDPLL_DPLL_INSTANCE_NUM_MAX];
} bcm_tdpll_input_clock_selector_options_t;

typedef struct bcm_tdpll_input_clock_switching_options_s {
    int revertive[TDPLL_DPLL_INSTANCE_NUM_MAX];
} bcm_tdpll_input_clock_switching_options_t;

typedef struct bcm_tdpll_input_clock_selector_state_s {
    int prior_selected_clock[TDPLL_DPLL_INSTANCE_NUM_MAX];
    int selected_clock[TDPLL_DPLL_INSTANCE_NUM_MAX];
    int reference_clock[TDPLL_DPLL_INSTANCE_NUM_MAX];
} bcm_tdpll_input_clock_selector_state_t;

typedef struct bcm_tdpll_input_clock_data_s {
    int prescreen_valid[TDPLL_INPUT_CLOCK_NUM_MAX];
    bcm_tdpll_input_clock_t input_clock[TDPLL_INPUT_CLOCK_NUM_MAX];

    bcm_tdpll_input_clock_monitor_options_t monitor_options;
    bcm_tdpll_input_clock_selector_options_t selector_options;
    bcm_tdpll_input_clock_switching_options_t switching_options;

    bcm_tdpll_input_clock_selector_state_t selector_state;

    bcm_tdpll_input_clock_monitor_cb monitor_callback;
    bcm_tdpll_input_clock_selector_cb selector_callback;

} bcm_tdpll_input_clock_data_t;

typedef struct bcm_tdpll_output_clock_data_s {
    bcm_tdpll_output_clock_t output_clock[TDPLL_OUTPUT_CLOCK_NUM_MAX];
} bcm_tdpll_output_clock_data_t;

typedef struct bcm_tdpll_esmc_data_s {
    int rx_enable;
    int tx_enable[TDPLL_DPLL_INSTANCE_NUM_MAX];

    bcm_pbmp_t rx_pbmp[TDPLL_DPLL_INSTANCE_NUM_MAX];
    bcm_pbmp_t tx_pbmp[TDPLL_DPLL_INSTANCE_NUM_MAX];

    bcm_esmc_pdu_data_t pdu_data[TDPLL_DPLL_INSTANCE_NUM_MAX];

    bcm_esmc_network_option_t network_option;
    bcm_esmc_quality_level_t holdover_ql[TDPLL_DPLL_INSTANCE_NUM_MAX];
    uint8 holdover_ssm_code[TDPLL_DPLL_INSTANCE_NUM_MAX];
} bcm_tdpll_esmc_data_t;


extern int _bcm_ptp_external_stack_create(
    int unit, 
    bcm_ptp_stack_info_t *info,
    bcm_ptp_stack_id_t ptp_id);

extern int _bcm_ptp_internal_stack_create(
    int unit, 
    bcm_ptp_stack_info_t *info,
    bcm_ptp_stack_id_t ptp_id);

extern int
_bcm_ptp_clock_cache_init(
    int unit);

extern int
_bcm_ptp_clock_cache_detach(
    int unit);

extern int
_bcm_ptp_clock_cache_stack_create(
    int unit,
    bcm_ptp_stack_id_t ptp_id);

extern int
_bcm_ptp_clock_cache_info_create(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_clock_cache_info_get(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_clock_info_t *clock_info);

extern int _bcm_ptp_clock_cache_info_set(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    const bcm_ptp_clock_info_t clock_info);

extern int _bcm_ptp_clock_cache_signal_get(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int signal_num,
    bcm_ptp_signal_output_t *signal);

extern int _bcm_ptp_clock_cache_signal_set(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int signal_num,
    const bcm_ptp_signal_output_t *signal);

extern int _bcm_ptp_clock_cache_tod_input_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int tod_input_num,
    bcm_ptp_tod_input_t *tod_input);

extern int _bcm_ptp_clock_cache_tod_input_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int tod_input_num,
    const bcm_ptp_tod_input_t *tod_input);

extern int _bcm_ptp_clock_cache_tod_output_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int tod_output_num,
    bcm_ptp_tod_output_t *tod_output);

extern int _bcm_ptp_clock_cache_tod_output_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int tod_output_num,
    const bcm_ptp_tod_output_t *tod_output);

extern int _bcm_ptp_clock_port_cache_info_get(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_clock_port_info_t *port_info);

extern int _bcm_ptp_clock_port_cache_info_set(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    const bcm_ptp_clock_port_info_t port_info);

extern int _bcm_ptp_clock_cache_port_state_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    _bcm_ptp_port_state_t *portState);

extern int _bcm_ptp_clock_cache_port_state_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    const _bcm_ptp_port_state_t portState);

extern int _bcm_ptp_rx_init(
    int unit);

extern int _bcm_ptp_rx_detach(
    int unit);

extern int _bcm_ptp_rx_stack_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    bcm_mac_t *host_mac,
    bcm_mac_t *top_mac,
    int tpid,
    int vlan);

extern int _bcm_ptp_rx_clock_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_rx_response_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id, 
    int clock_num, 
    int usec, 
    uint8 **data, 
    int *data_len);

extern int _bcm_ptp_external_rx_response_free(
    int unit,
    int ptp_id,
    uint8 *resp_data);

extern int _bcm_ptp_internal_rx_response_free(
    int unit,
    int ptp_id,
    uint8 *resp_data);

extern int _bcm_ptp_rx_response_flush(
    int unit,
    bcm_ptp_stack_id_t ptp_id, 
    int clock_num);

extern int _bcm_ptp_management_init(
    int unit);

extern int _bcm_ptp_management_detach(
    int unit);

extern int _bcm_ptp_management_stack_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    bcm_mac_t *src_mac,
    bcm_mac_t *dest_mac,
    bcm_ip_t src_ip,
    bcm_ip_t dest_ip,
    uint16 vlan,
    uint8 prio,
    bcm_pbmp_t top_bitmap);

extern int _bcm_ptp_internal_management_stack_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id);

extern int _bcm_ptp_management_clock_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_management_message_send(
    int unit, 
    bcm_ptp_stack_id_t ptp_id, 
    int clock_num, 
    const bcm_ptp_port_identity_t* dest_clock_port, 
    uint8 action,
    uint16 cmd, 
    uint8* payload, 
    int payload_len,
    uint8* resp, 
    int* resp_len);

extern int _bcm_ptp_unicast_slave_subscribe(
    int unit, 
    bcm_ptp_stack_id_t ptp_id, 
    int clock_num, 
    uint32 clock_port, 
    _bcm_ptp_clock_peer_ext_t *slave_info,
    bcm_ptp_message_type_t msg_type,
    bcm_ptp_tlv_type_t tlv_type,
    int interval);

extern int _bcm_ptp_management_domain_get(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 *domain);


extern int _bcm_ptp_management_domain_set(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 domain);

extern int _bcm_ptp_management_message_domain_get(
	uint8 *message, 
	uint8 *domain);

extern int _bcm_ptp_management_message_domain_set(
	uint8 *message, 
	uint8 domain);

extern int _bcm_ptp_tunnel_message_to_top(
    int unit, 
    bcm_ptp_stack_id_t ptp_id, 
    int clock_index,
    int port_index,
    unsigned payload_len,
    uint8 *payload);

extern int _bcm_ptp_tunnel_message_to_world(
    int unit, 
    bcm_ptp_stack_id_t ptp_id, 
    int clock_index,
    unsigned payload_len,
    uint8 *payload);

extern int _bcm_ptp_unicast_master_table_init(
    int unit);

extern int _bcm_ptp_unicast_master_table_detach(
    int unit);

extern int _bcm_ptp_unicast_master_table_stack_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id);

extern int _bcm_ptp_unicast_master_table_clock_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_unicast_slave_table_init(
    int unit);

extern int _bcm_ptp_unicast_slave_table_detach(
    int unit);

extern int _bcm_ptp_unicast_slave_table_stack_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id);

extern int _bcm_ptp_unicast_slave_table_clock_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_acceptable_master_table_init(
    int unit);

extern int _bcm_ptp_acceptable_master_table_detach(
    int unit);

extern int _bcm_ptp_acceptable_master_table_stack_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id);

extern int _bcm_ptp_acceptable_master_table_clock_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_servo_start(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_servo_restart(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_servo_stop(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_servo_ipdv_configuration_get(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    _bcm_ptp_ipdv_config_t *config);

extern int _bcm_ptp_servo_ipdv_configuration_set(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    const _bcm_ptp_ipdv_config_t config);

extern int _bcm_ptp_servo_performance_get(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    _bcm_ptp_servo_performance_t *perf);

extern int _bcm_ptp_servo_ipdv_performance_get(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    _bcm_ptp_ipdv_performance_t *perf);
    
extern int _bcm_ptp_servo_performance_data_clear(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_telecom_g8265_map_QL_clockClass(
    bcm_ptp_telecom_g8265_quality_level_t QL,
    uint8 *clockClass);

extern int _bcm_ptp_telecom_g8265_map_clockClass_QL(
    uint8 clockClass,
    bcm_ptp_telecom_g8265_quality_level_t *QL);

extern int _bcm_ptp_telecom_g8265_verbose_level_set(
    uint32 flags);

extern int _bcm_ptp_clock_class_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 clockClass);

extern int _bcm_ptp_system_log_configure(int unit, bcm_ptp_stack_id_t ptp_id, int clock_num,
    uint32 debug_mask, uint64 udp_log_mask,
    bcm_mac_t src_mac, bcm_mac_t dest_mac,
    uint16 tpid, uint16 vid, uint8 ttl,
    bcm_ip_t src_addr, bcm_ip_t dest_addr,
    uint16 udp_port);

extern int _bcm_ptp_tunnel_arp_set(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    uint8 flag);

/* ESMC functions. */
extern int bcm_esmc_init(
    int unit,
    int stack_id);

extern int bcm_esmc_rx(
    int unit,
    int stack_id,
    int ingress_port,
    int ethertype,
    int esmc_pdu_len,
    uint8 *esmc_pdu);

extern int bcm_esmc_pdu_port_data_get(
    int unit,
    int stack_id,
    int port_num,
    bcm_esmc_pdu_data_t *pdu_data,
    sal_time_t *timestamp);

/* T-DPLL functions. */
extern int bcm_tdpll_dpll_instance_init(
    int unit,
    int stack_id);

extern int bcm_tdpll_dpll_reference_set(
    int unit,
    int stack_id,
    int num_dpll,
    int *dpll_ref);

extern int bcm_tdpll_input_clock_init(
    int unit,
    int stack_id);

extern int bcm_tdpll_input_clock_shutdown(
    int unit);

extern int bcm_tdpll_input_clock_port_lookup(
    int unit,
    int stack_id,
    int port_num,
    int *clock_index);

extern int bcm_tdpll_input_clock_mac_lookup(
    int unit,
    int stack_id,
    bcm_mac_t *mac,
    int *clock_index);

extern int bcm_tdpll_input_clock_reference_mac_get(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_mac_t *mac);

extern int bcm_tdpll_input_clock_dpll_use_get(
    int unit,
    int stack_id,
    int clock_index,
    int dpll_index,
    int *dpll_use);

extern int bcm_tdpll_input_clock_dpll_use_set(
    int unit,
    int stack_id,
    int clock_index,
    int dpll_index,
    int dpll_use);

extern int bcm_tdpll_input_clock_dpll_reference_get(
    int unit,
    int stack_id,
    int dpll_index,
    int *reference);

extern int bcm_tdpll_output_clock_init(
    int unit,
    int stack_id);

extern int bcm_tdpll_esmc_init(
    int unit,
    int stack_id);

extern int bcm_tdpll_esmc_holdover_event_send(
    int unit,
    int stack_id,
    int dpll_index);

extern int bcm_tdpll_esmc_switch_event_send(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_esmc_quality_level_t ql);

extern int _bcm_ptp_modular_init(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 flags);

extern int _bcm_ptp_modular_shutdown(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_ctdev_init(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 p);

extern int
_bcm_ptp_ctdev_gateway(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int ev_data_len,
    uint8 *ev_data);

extern int _bcm_ptp_ctdev_phase_accumulator(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    const int64 freq_corr_ppt);

extern int _bcm_ptp_ctdev_calculator(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num);

extern int _bcm_ptp_ctdev_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    unsigned timescale_sec,
    uint64 *tdev_psec);

extern uint64 _bcm_ptp_llu_div(
    uint64 num,
    uint16 den);

extern uint32 _bcm_ptp_llu_isqrt(
    uint64 x);

extern uint32 _bcm_ptp_xorshift_rand(
    void);

extern uint16 _bcm_ptp_uint16_read(
    uint8* buffer);

extern uint32 _bcm_ptp_uint32_read(
    uint8* buffer);

extern uint64 _bcm_ptp_uint64_read(
    uint8* buffer);

extern int64 _bcm_ptp_int64_read(
    uint8* buffer);

extern int64 _bcm_ptp_uint64_to_int64(
    const uint64 value);

extern uint64 _bcm_ptp_int64_to_uint64(
    const int64 value);

extern void _bcm_ptp_uint16_write(
    uint8* buffer, 
    const uint16 value);

extern void _bcm_ptp_uint32_write(
    uint8* buffer, 
    const uint32 value);

extern void _bcm_ptp_uint64_write(
    uint8* buffer, 
    const uint64 value);

extern void _bcm_ptp_int64_write(
    uint8* buffer, 
    const int64 value);

extern int _bcm_ptp_function_precheck(
    int unit, 
    bcm_ptp_stack_id_t ptp_id, 
    int clock_num, 
    uint32 clock_port);

extern int _bcm_ptp_clock_lookup(
    const bcm_ptp_clock_identity_t clock_identity,
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int *clock_num);

extern int _bcm_ptp_peer_address_convert(
    bcm_ptp_clock_peer_address_t *peer_addr,
    bcm_ptp_clock_port_address_t *port_addr);

extern int _bcm_ptp_port_address_convert(
     bcm_ptp_clock_port_address_t *port_addr,
     bcm_ptp_clock_peer_address_t *peer_addr);

extern int _bcm_ptp_peer_address_raw_compare(
    const bcm_ptp_clock_peer_address_t *a,
    uint8 *b,
    bcm_ptp_protocol_t protocol);

extern int _bcm_ptp_peer_address_compare(
    const bcm_ptp_clock_peer_address_t *a, 
    const bcm_ptp_clock_peer_address_t *b);

extern int _bcm_ptp_port_address_compare(
    const bcm_ptp_clock_port_address_t *a,
    const bcm_ptp_clock_port_address_t *b);

extern void _bcm_ptp_dump_hex(
    uint8 *buf,
    int len,
    int indent);

/* monotonic time in seconds */
extern sal_time_t _bcm_ptp_monotonic_time(void);

/* Prototypes for esw functions */
extern void esw_set_ext_stack_config_uint32(
    _bcm_ptp_stack_info_t *stack_p,
    uint32 offset, uint32 value);

extern void esw_set_ext_stack_config_array(
    _bcm_ptp_stack_info_t* stack_p,
    uint32 offset,
    const uint8 * array,
    int len);

extern uint8 _bcm_ptp_read_pcishared_uint8(
    bcm_ptp_external_stack_info_t *stack_p, 
    uint32 addr);

extern void _bcm_ptp_write_pcishared_uint8(
    bcm_ptp_external_stack_info_t *stack_p,
    uint32 addr, 
    uint8 val);

extern int _bcm_ptp_write_pcishared_uint32(
    void *cookie,
    uint32 addr,
    uint32 val);

extern int _bcm_ptp_read_pcishared_uint32(
    void *cookie,
    uint32 addr,
    uint32 *val);

extern void _bcm_ptp_write_pcishared_uint8_aligned_array(
    int pci_idx,
    uint32 addr,
    uint8 * array,
    int array_len);

int _bcm_ptp_ext_stack_reset(int pci_idx);
int _bcm_ptp_ext_stack_start(int pci_idx);

extern int _bcm_ptp_set_max_unicast_masters(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int max_masters);

extern int _bcm_ptp_set_max_acceptable_masters(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int max_masters);

extern int _bcm_ptp_set_max_unicast_slaves(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int max_slaves);

extern int _bcm_ptp_set_max_multicast_slave_stats(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int max_entries);


extern int _bcm_ptp_register_management_callback(
    int unit, 
    bcm_ptp_cb cb, 
    void *user_data);

extern int _bcm_ptp_register_event_callback(
    int unit, 
    bcm_ptp_cb cb, 
    void *user_data);

extern int _bcm_ptp_register_peers_callback(
    int unit, 
    bcm_ptp_cb cb, 
    void *user_data);

extern int _bcm_ptp_register_fault_callback(
    int unit, 
    bcm_ptp_cb cb, 
    void *user_data);

extern int _bcm_ptp_register_signal_callback(
    int unit, 
    bcm_ptp_cb cb, 
    void *user_data);

extern int _bcm_ptp_register_signaling_arbiter(
    int unit, 
    bcm_ptp_signaling_arbiter_t arb,
    void *user_data);

extern int _bcm_ptp_unregister_management_callback(
    int unit);

extern int _bcm_ptp_unregister_event_callback(
    int unit);

extern int _bcm_ptp_unregister_peers_callback(
    int unit);

extern int _bcm_ptp_unregister_fault_callback(
    int unit);

extern int _bcm_ptp_unregister_signal_callback(
    int unit);

extern int _bcm_ptp_unregister_signaling_arbiter(
    int unit);

/* external-specific transport functions */
extern _bcm_ptp_transport_init_t _bcm_ptp_external_transport_init;
extern _bcm_ptp_transport_tx_t _bcm_ptp_external_tx;
extern _bcm_ptp_transport_tx_completion_t _bcm_ptp_external_tx_completion;
extern _bcm_ptp_transport_terminate_t _bcm_ptp_external_transport_terminate;

/* internal-specific transport functions */
extern _bcm_ptp_transport_init_t _bcm_ptp_internal_transport_init;
extern _bcm_ptp_transport_tx_t _bcm_ptp_internal_tx;
extern _bcm_ptp_transport_tx_completion_t _bcm_ptp_internal_tx_completion;
extern _bcm_ptp_transport_terminate_t _bcm_ptp_internal_transport_terminate;

/* internal-specific signaling functions */

extern int _bcm_ptp_signaling_init( int unit, bcm_ptp_stack_id_t ptp_id);
extern int _bcm_ptp_signaling_manager(int unit, bcm_ptp_stack_id_t ptp_id,
    int clock_num, int en_telecom);

extern int _bcm_ptp_signaled_unicast_master_table_init(int unit, bcm_ptp_stack_id_t ptp_id);
extern int _bcm_ptp_signaled_unicast_master_table_reset(int unit, bcm_ptp_stack_id_t ptp_id);
extern int _bcm_ptp_signaled_unicast_slave_table_init(int unit, bcm_ptp_stack_id_t ptp_id);
extern int _bcm_ptp_signaled_unicast_slave_table_reset(int unit, bcm_ptp_stack_id_t ptp_id);
extern int _bcm_ptp_unicast_slave_host_reset(int unit, bcm_ptp_stack_id_t ptp_id, int clock_num, int port_num);
extern int _bcm_ptp_unicast_slave_stack_restore(int unit, bcm_ptp_stack_id_t ptp_id, int clock_num, int port_num);

extern int _bcm_ptp_construct_signaling_msg(bcm_ptp_port_identity_t *from_port,
                                            bcm_ptp_clock_identity_t to_clock,
                                            uint16 to_port, int domain, uint16 sequenceId,
                                            uint8* tlv_payload, int tlv_payload_len,
                                            uint8* udp_payload, int *udp_payload_len);

extern int _bcm_ptp_construct_signaling_request_tlv(int type, int interval, unsigned duration,
                                                    uint8* tlv_payload, int *tlv_payload_len);

extern int _bcm_ptp_construct_udp_packet(bcm_ptp_clock_peer_t *peer, bcm_ptp_clock_port_address_t *local_port,
                                         uint8* udp_payload, int udp_payload_len, uint8* packet, int *packet_len);

extern int _bcm_ptp_construct_signaling_cancel_tlv(int type, uint8* tlv_payload, int *tlv_payload_len);
extern int _bcm_ptp_construct_signaling_ack_cancel_tlv(int type, uint8* tlv_payload, int *tlv_payload_len);
extern int _bcm_ptp_construct_signaling_grant_tlv(int type, int interval, unsigned duration, int invite_renewal,
                                                  uint8* tlv_payload, int *tlv_payload_len);

extern int _bcm_ptp_signaling_message_append_tlv(uint8 *msg, int *msgLength,
    uint8 *tlv, int tlvLength);

extern bcm_ptp_callback_t _bcm_ptp_process_incoming_signaling_msg(
    int unit, bcm_ptp_stack_id_t ptp_id, int clock_num, int port_num, bcm_ptp_protocol_t protocol,
    int src_addr_offset, int ptp_msg_offset, unsigned data_length, uint8* data);

/* For use by diag shell */
extern _bcm_ptp_unit_cache_t _bcm_common_ptp_unit_array[BCM_MAX_NUM_UNITS];

/* For use by diag shell */
extern int _bcm_common_ptp_log_level_set(int unit, bcm_ptp_stack_id_t ptp_id, 
                int clock_num, uint32 level);

/* For use by diag shell */
extern int _bcm_common_ptp_input_channels_status_get(int unit,
                bcm_ptp_stack_id_t ptp_id, int clock_num,
                int *num_chan, _bcm_ptp_channel_status_t *chan_status);

/* For use by diag shell */
extern int _bcm_ptp_show_system_info(int unit,
                bcm_ptp_stack_id_t ptp_id,
                int clock_num);

/* For use by diag shell */
typedef struct _bcm_ptp_clock_description_s {
    uint16 size;
    uint8  *data;
} _bcm_ptp_clock_description_t;

extern int _bcm_ptp_clock_description_get(int unit, 
                                   bcm_ptp_stack_id_t ptp_id,
                                   int clock_num, 
                                   int port_num, 
                                   _bcm_ptp_clock_description_t *description);

extern int _bcm_ptp_update_peer_counts(int unit, uint16 localPortNum, bcm_ptp_clock_identity_t *clockID,
    bcm_ptp_clock_port_address_t *port_addr, int inInterrupt, int isMaster,
    unsigned announces, unsigned syncs, unsigned followups, unsigned delayreqs,
    unsigned delayresps, unsigned mgmts, unsigned signals, unsigned rejected);

extern int _bcm_ptp_update_peer_dataset(int unit, bcm_ptp_stack_id_t ptp_id, int clock_num, int port_num);

extern int _bcm_ptp_port_address_cmp(bcm_ptp_clock_port_address_t *a, bcm_ptp_clock_port_address_t *b);
extern int _bcm_ptp_is_clockid_null(bcm_ptp_clock_identity_t clockID);

/* Diag-shell reference C-TDEV alarm function. */
extern uint32
_bcm_ptp_ctdev_g823_mask(
    int tau);

/* For use protecting us from sal syncronization function issues */

int _bcm_ptp_enter_critical(void);
int _bcm_ptp_exit_critical(int level);
int _bcm_ptp_intr_context(void);

typedef sal_mutex_t _bcm_ptp_mutex_t;

_bcm_ptp_mutex_t _bcm_ptp_mutex_create(char *desc);
void _bcm_ptp_mutex_destroy(_bcm_ptp_mutex_t m);
int _bcm_ptp_mutex_take(_bcm_ptp_mutex_t m, int usec);
int _bcm_ptp_mutex_give(_bcm_ptp_mutex_t m);

typedef sal_sem_t _bcm_ptp_sem_t;

_bcm_ptp_sem_t _bcm_ptp_sem_create(char *desc, int binary, int initial_count);
void _bcm_ptp_sem_destroy(_bcm_ptp_sem_t b);
int _bcm_ptp_sem_take(_bcm_ptp_sem_t b, int usec);
int _bcm_ptp_sem_give(_bcm_ptp_sem_t b);

/* For use by diag shell, to register ARP handler if no other L3 handling is in place: */
extern void (*_bcm_ptp_arp_callback)(int unit, bcm_ptp_stack_id_t ptp_id, int protocol, int l3_offset, int payload_offset, int msg_len, uint8 *msg);

void ptp_printf(const char *, ...);

extern int _bcm_ptp_addr_len(int addr_type);
#endif /* INCLUDE_PTP */
#endif /* !__BCM_INT_PTP_H__ */
