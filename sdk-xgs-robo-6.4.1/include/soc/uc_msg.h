/*
 * $Id: uc_msg.h,v 1.11 Broadcom SDK $
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
 * File:    uc_msg.h
 */

#ifndef _SOC_UC_MSG_H
#define _SOC_UC_MSG_H

/* Define how the shared msg headers find certain items */
#include <soc/shared/mos_msg_common.h>

/* Macros to set and clear the S/W interrupt bits in the correct CMC */
#define CMIC_CMC_SW_INTR_SET(unit, target)                              \
    soc_pci_write(unit,                                                 \
                  (target == CMICM_SW_INTR_HOST) ? CMIC_CMC0_SW_INTR_CONFIG_OFFSET : \
                  (target == CMICM_SW_INTR_UC0) ? CMIC_CMC1_SW_INTR_CONFIG_OFFSET : \
                  (target == CMICM_SW_INTR_UC1) ? CMIC_CMC2_SW_INTR_CONFIG_OFFSET : \
                  CMIC_RPE_SW_INTR_CONFIG_OFFSET,                       \
                  0x4 | CMICM_SW_INTR_HOST)


/* We only clear our own - but clear the right target */
#define CMIC_CMC_SW_INTR_CLEAR(unit, target)                            \
    soc_pci_write(unit, CMIC_CMC0_SW_INTR_CONFIG_OFFSET, target)

#define CMIC_MSG_PBUF_SIZE      256

/* seconds / nanoseconds time, corresponding to TAI/UTC/PTP/... time */
typedef struct soc_cmic_uc_time_s {
    uint64 seconds;
    uint32 nanoseconds;
} soc_cmic_uc_time_t;

/* Timestamp data associated with a particular event type */
typedef struct soc_cmic_uc_ts_data_s {
    uint64 hwts;              /* Most recent timestamp for event type      */
    uint64 prev_hwts;         /* Preceeding timestamp for event type       */
    uint64 hwts_ts1;          /* Corresponding TS1 timestamp (IPROC/CMICd) */
    soc_cmic_uc_time_t time;  /* Corresponding full time from firmware     */
} soc_cmic_uc_ts_data_t;

/* Routines */
extern void soc_cmic_sw_intr(int unit, uint32 rupt_num);
extern void soc_cmic_uc_msg_init(int unit);
extern int soc_cmic_uc_msg_start(int unit);
extern int soc_cmic_uc_msg_stop(int unit);
extern int soc_cmic_uc_msg_active_wait(int unit, uint32 host);
extern void soc_cmic_uc_msg_thread(void *unit_vp);
extern int soc_cmic_uc_msg_uc_start(int unit, int uC);
extern int soc_cmic_uc_msg_uc_stop(int unit, int uC);
extern int soc_cmic_uc_msg_send(int unit, int uC, mos_msg_data_t *msg,
                                sal_usecs_t timeout);
extern int soc_cmic_uc_msg_receive(int unit, int uC, uint8 class,
                                   mos_msg_data_t *msg, int timeout);
extern int soc_cmic_uc_msg_receive_cancel(int unit, int uC, int msg_class);
extern int soc_cmic_uc_msg_send_receive(int unit, int uC, mos_msg_data_t *send,
                                        mos_msg_data_t *reply,
                                        sal_usecs_t timeout);
extern int soc_cmic_uc_appl_init(int unit, int uC, int msg_class,
                                 sal_usecs_t timeout, uint32 version_info,
                                 uint32 min_appl_version);
#ifdef BCM_KATANA2_SUPPORT
extern uint32 soc_cmic_bs_reg_cache(int unit, int bs_num);
#endif

extern int soc_cmic_uc_msg_timestamp_get(int unit,  int event_id,
                                         soc_cmic_uc_ts_data_t *ts_data);

extern int soc_cmic_uc_msg_timestamp_enable(int unit,  int event_id);
extern int soc_cmic_uc_msg_timestamp_disable(int unit,  int event_id);

#endif
