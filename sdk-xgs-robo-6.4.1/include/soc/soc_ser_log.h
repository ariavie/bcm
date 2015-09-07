/*
 * $Id: 5f607d21f57c9c61c2e3e12f54184c20649ada97 $
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
 * File: soc_ser_log.h
 * Purpose: SER logging using a circular buffer
 */

#ifndef _SOC_SER_LOG_H_
#define _SOC_SER_LOG_H_

#include <soc/mem.h>

typedef enum {
    SOC_PARITY_TYPE_NONE,
    SOC_PARITY_TYPE_GENERIC,
    SOC_PARITY_TYPE_PARITY,
    SOC_PARITY_TYPE_ECC,
    SOC_PARITY_TYPE_HASH,
    SOC_PARITY_TYPE_EDATABUF,
    SOC_PARITY_TYPE_COUNTER,
    SOC_PARITY_TYPE_MXQPORT,
    SOC_PARITY_TYPE_SER
} soc_ser_parity_type_t;

typedef enum {
    SOC_SER_UNKNOWN = -1,
    SOC_SER_UNCORRECTED = 0,
    SOC_SER_CORRECTED = 1
} soc_ser_correct_t;

/* Used to indicate the format of the data/value portion of a TLV */
typedef enum {
    SOC_SER_LOG_TLV_NONE = 0,
    SOC_SER_LOG_TLV_MEMORY = 1,
    SOC_SER_LOG_TLV_REGISTER = 2,
    SOC_SER_LOG_TLV_CONTENTS = 3,
    SOC_SER_LOG_TLV_SER_FIFO = 4,
    SOC_SER_LOG_TLV_CACHE = 5,
    SOC_SER_LOG_TLV_GENERIC = 6
} soc_ser_log_tlv_type_t;

typedef enum {
    SOC_SER_LOG_ACC_ANY = -1,
    SOC_SER_LOG_ACC_GROUP = 0,
    SOC_SER_LOG_ACC_X = 1,
    SOC_SER_LOG_ACC_Y = 2,
    SOC_SER_LOG_ACC_SBS = 6
} soc_ser_log_acc_type_t;

/* Type length value header for elements of the log */
typedef struct soc_ser_log_tlv_hdr_s {
    int type;
    int length;
} soc_ser_log_tlv_hdr_t;

/* Data/value portion of memory TLVs */
typedef struct soc_ser_log_tlv_memory_s {
    soc_mem_t memory;
    int index;
} soc_ser_log_tlv_memory_t;

/* Data/value portion of register TLVs */
typedef struct soc_ser_log_tlv_register_s {
    soc_reg_t reg;
    int index;
    int port;
} soc_ser_log_tlv_register_t;

typedef enum {
    SOC_SER_LOG_FLAG_ERR_SRC = 1,
    SOC_SER_LOG_FLAG_MULTIBIT = 1 << 1,
    SOC_SER_LOG_FLAG_DOUBLEBIT = 1 << 2
} soc_ser_log_flag_t;

/* Data/value portion of generic TLVs */
typedef struct soc_ser_log_tlv_generic_s {
    soc_ser_log_flag_t      flags;
    sal_usecs_t             time;
    uint8                   boot_count;
    uint32                  address;
    soc_ser_log_acc_type_t  acc_type;
    soc_block_t             block_type;
    soc_ser_parity_type_t   parity_type;
    int                     ser_response_flag;
    int                     corrected;
} soc_ser_log_tlv_generic_t;

extern int soc_ser_log_init(int unit, void *location, int size);
extern int soc_ser_log_load(int unit, void *location);
extern int soc_ser_log_invalidate(int unit);
extern int soc_ser_log_get_entry_size(int unit, int id);
extern int soc_ser_log_get_entry(int unit, int id, int size, void *entry);
extern int soc_ser_log_create_entry(int unit, int size);
extern int soc_ser_log_find_recent(int unit, soc_mem_t mem, int index, sal_usecs_t time);
extern int soc_ser_log_print_entry(void *buffer);
extern int soc_ser_log_add_tlv( int unit, 
                                int id,
                                soc_ser_log_tlv_type_t type, 
                                int size, 
                                void * buffer);

extern int soc_ser_log_get_tlv( int unit, 
                                int id,
                                soc_ser_log_tlv_type_t type, 
                                int size, 
                                void * buffer);

extern int soc_ser_log_mod_tlv( int unit, 
                                int id,
                                soc_ser_log_tlv_type_t type, 
                                int size, 
                                void * buffer);
extern int soc_ser_log_find_tlv(void *buffer, soc_ser_log_tlv_type_t type);
extern int soc_ser_log_print_tlv(void *buffer);
extern int soc_ser_log_get_boot_count(int unit);
extern int soc_ser_log_print_all(int unit);
#endif
