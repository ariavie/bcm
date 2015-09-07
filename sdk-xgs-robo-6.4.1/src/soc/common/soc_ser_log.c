/*
 * $Id: e5015461fda4b8542d4b06e06ef1e30e17a2eaa4 $
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
 * File: soc_ser_log.c
 * Purpose: SER logging using a circular buffer
 */

#include <shared/bsl.h>

#include <soc/soc_ser_log.h>
#include <soc/soc_log_buf.h>
#include <soc/error.h>
#include <sal/appl/sal.h>
#include <soc/util.h>

#define _SOC_SER_LOG_BUFFER_SIZE 500
static char _soc_ser_log_buffer[SOC_MAX_NUM_DEVICES][_SOC_SER_LOG_BUFFER_SIZE];

static void *_soc_ser_log_buffer_ptr[SOC_MAX_NUM_DEVICES];

typedef struct _soc_ser_log_criteria_s {
    soc_mem_t mem;
    int index;
    sal_usecs_t time;
} _soc_ser_log_criteria_t;

int
soc_ser_log_get_entry_size(int unit, int id)
{
    if (id == 0) {
        return SOC_E_PARAM;
    }
    return soc_log_buf_entry_get_size(_soc_ser_log_buffer_ptr[unit], id);
}

int
soc_ser_log_get_entry(int unit, int id, int size, void *entry)
{
    int entry_size;

    if (id == 0) {
        return SOC_E_PARAM;
    }

    entry_size = soc_log_buf_entry_get_size(_soc_ser_log_buffer_ptr[unit], id);

    if (entry_size > size) {
        return SOC_E_PARAM;
    }

    return soc_log_buf_entry_read(_soc_ser_log_buffer_ptr[unit],
        id, 0, entry_size, entry);
}

static int
_soc_ser_log_match_entry(void *criteria, void *entry)
{
    _soc_ser_log_criteria_t *crit = criteria;
    soc_ser_log_tlv_hdr_t *tlv_hdr = entry;
    soc_ser_log_tlv_memory_t *tlv_mem;
    soc_ser_log_tlv_generic_t *tlv_gen;
    _soc_ser_log_criteria_t entry_data;
    int found_gen = 0;
    int found_mem = 0;

    sal_memset(&entry_data, 0, sizeof(_soc_ser_log_criteria_t));

    while ((tlv_hdr->type != 0) && !(found_gen && found_mem)) {
        if (tlv_hdr->type == SOC_SER_LOG_TLV_GENERIC) {
            tlv_gen = (soc_ser_log_tlv_generic_t*)(((char*)tlv_hdr) +
                sizeof(soc_ser_log_tlv_hdr_t));
            entry_data.time = tlv_gen->time;
            found_gen = 1;
        } else if (tlv_hdr->type == SOC_SER_LOG_TLV_MEMORY) {
            tlv_mem = (soc_ser_log_tlv_memory_t*)(((char*)tlv_hdr) +
                sizeof(soc_ser_log_tlv_hdr_t));
            entry_data.index = tlv_mem->index;
            entry_data.mem = tlv_mem->memory;
            found_mem = 1;
        }
        tlv_hdr = (soc_ser_log_tlv_hdr_t*)((char*)tlv_hdr +
            tlv_hdr->length + sizeof(soc_ser_log_tlv_hdr_t));
    }

    if ((found_gen && found_mem) &&
        (crit->mem == entry_data.mem) &&
        (crit->index == entry_data.index) &&
        (SAL_USECS_SUB(crit->time, entry_data.time) <= 0)) {
        return 1;
    }

    return 0;
}

int 
soc_ser_log_find_recent(int unit, soc_mem_t mem, int index, sal_usecs_t time)
{
    char * buffer[_SOC_SER_LOG_BUFFER_SIZE];
    _soc_ser_log_criteria_t criteria;

    if (_soc_ser_log_buffer_ptr[unit] == NULL) {
        return 0;
    }

    criteria.mem = mem;
    criteria.index = index;
    /* Check within the last 5 seconds */
    criteria.time = SAL_USECS_SUB(time, SECOND_USEC * 5);
    
    return soc_log_buf_search(_soc_ser_log_buffer_ptr[unit], buffer,
        _SOC_SER_LOG_BUFFER_SIZE, &criteria, &_soc_ser_log_match_entry);
}

int
soc_ser_log_create_entry(int unit, int size)
{
    /* if someone tries to add an entry to a non-existant log, 
     * return the invalid id 
     */
    if (_soc_ser_log_buffer_ptr[unit] == NULL) {
        return 0;
    }
    return soc_log_buf_add(_soc_ser_log_buffer_ptr[unit], size);
}

/* Add tlv to a log, if it already exists overwrite it */
int
soc_ser_log_add_tlv(int unit, int id, soc_ser_log_tlv_type_t type,
    int size, void * buffer)
{
    int offset = 0;
    soc_ser_log_tlv_hdr_t tlv_hdr;

    soc_log_buf_entry_read(_soc_ser_log_buffer_ptr[unit],
        id, offset, sizeof(soc_ser_log_tlv_hdr_t), &tlv_hdr);
    while ((tlv_hdr.type != 0) &&
            (tlv_hdr.type != type)) {
        offset = offset + tlv_hdr.length + sizeof(soc_ser_log_tlv_hdr_t);
        soc_log_buf_entry_read(_soc_ser_log_buffer_ptr[unit], id, offset,
            sizeof(soc_ser_log_tlv_hdr_t), &tlv_hdr);
        if (tlv_hdr.type == type) {
            return SOC_E_PARAM;
        }
    }
    /* If we're adding rather than overwriting, make sure we have room */
    if ((tlv_hdr.type != type) &&
        (soc_log_buf_entry_get_size(_soc_ser_log_buffer_ptr[unit], id) < 
            (offset + sizeof(soc_ser_log_tlv_hdr_t) + size))) {
        return SOC_E_PARAM;
    }
    
    /* If we're trying to overwrite, but with a different size tlv, error */
    if ((tlv_hdr.type == type) && (tlv_hdr.length != size)) {
        return SOC_E_PARAM;
    }

    tlv_hdr.type = type;
    tlv_hdr.length = size;
    soc_log_buf_entry_write(_soc_ser_log_buffer_ptr[unit],
        id, offset, sizeof(soc_ser_log_tlv_hdr_t), &tlv_hdr);
    offset += sizeof(soc_ser_log_tlv_hdr_t);
    soc_log_buf_entry_write(_soc_ser_log_buffer_ptr[unit], 
        id, offset, size, buffer);

    return SOC_E_NONE;
}

int
soc_ser_log_mod_tlv(int unit,
                    int id, 
                    soc_ser_log_tlv_type_t type, 
                    int size, 
                    void * buffer)
{
    int offset = 0;
    soc_ser_log_tlv_hdr_t tlv_hdr;

    soc_log_buf_entry_read(_soc_ser_log_buffer_ptr[unit],
        id, offset, sizeof(soc_ser_log_tlv_hdr_t), &tlv_hdr);
    while (tlv_hdr.type != type) {
        if (tlv_hdr.type == 0) {
            return SOC_E_PARAM;
        }
        offset = offset + tlv_hdr.length + sizeof(soc_ser_log_tlv_hdr_t);
        soc_log_buf_entry_read(_soc_ser_log_buffer_ptr[unit], 
            id, offset, sizeof(soc_ser_log_tlv_hdr_t), &tlv_hdr);
    }
    if (size != tlv_hdr.length) {
        return SOC_E_PARAM;
    }

    soc_log_buf_entry_write(_soc_ser_log_buffer_ptr[unit], id, offset +
        sizeof(soc_ser_log_tlv_hdr_t), size, buffer);
    return SOC_E_NONE;
}

int
soc_ser_log_get_tlv(int unit,
                    int id, 
                    soc_ser_log_tlv_type_t type, 
                    int size, 
                    void * buffer)
{
    int offset = 0;
    soc_ser_log_tlv_hdr_t tlv_hdr;

    soc_log_buf_entry_read(_soc_ser_log_buffer_ptr[unit],
        id, offset, sizeof(soc_ser_log_tlv_hdr_t), &tlv_hdr);
    while (tlv_hdr.type != type) {
        if (tlv_hdr.type == 0) {
            return SOC_E_PARAM;
        }
        offset = offset + tlv_hdr.length + sizeof(soc_ser_log_tlv_hdr_t);
        soc_log_buf_entry_read(_soc_ser_log_buffer_ptr[unit],
            id, offset, sizeof(soc_ser_log_tlv_hdr_t), &tlv_hdr);
    }

    if (size < tlv_hdr.length) {
        return SOC_E_PARAM;
    }

    soc_log_buf_entry_read(_soc_ser_log_buffer_ptr[unit],
        id, offset+sizeof(soc_ser_log_tlv_hdr_t), tlv_hdr.length, &buffer);
    return SOC_E_NONE;
}

/* print an entry */
int
soc_ser_log_print_entry(void *buffer)
{
    soc_ser_log_tlv_hdr_t *tlv_hdr = buffer;

    while (tlv_hdr->type != 0) {
        soc_ser_log_print_tlv((char*)tlv_hdr);
        tlv_hdr = (soc_ser_log_tlv_hdr_t*)((char*)tlv_hdr +
            tlv_hdr->length + sizeof(soc_ser_log_tlv_hdr_t));
    }

    return SOC_E_NONE;
}

int
soc_ser_log_print_tlv(void *buffer)
{
    soc_ser_log_tlv_hdr_t *tlv_hdr = buffer;
    char *data = (char*)buffer + sizeof(soc_ser_log_tlv_hdr_t);
    int index;
    
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META("Tlv Header:\n")));
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META("\ttype: %d\n"), (int)(tlv_hdr->type)));
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META("\tlength: %d\n"), (int)(tlv_hdr->length)));
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META("\tvalue: \n")));
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META("Tlv Data:\n")));
    switch (tlv_hdr->type) {
    case SOC_SER_LOG_TLV_MEMORY:
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\tSOC_SER_LOG_TLV_MEMORY\n")));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tmemory: %d\n"),
                   (int)(((soc_ser_log_tlv_memory_t*)(data))->memory)));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tindex: %d\n"),
                   (int)(((soc_ser_log_tlv_memory_t*)(data))->index)));
        break;
    case SOC_SER_LOG_TLV_REGISTER:
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\tSOC_SER_LOG_TLV_REGISTER\n")));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tregister: %d\n"),
                   (int)(((soc_ser_log_tlv_register_t*)(data))->reg)));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tindex: %d\n"),
                   (int)(((soc_ser_log_tlv_register_t*)(data))->index)));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tport: %d\n"),
                   (int)(((soc_ser_log_tlv_register_t*)(data))->port)));
        break;
    case SOC_SER_LOG_TLV_CONTENTS:
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\tSOC_SER_LOG_TLV_CONTENTS\n\t\t")));
        for (index = 0; index < tlv_hdr->length; index++) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META("%02x "), *((char*)(data + index))));
            if ((index%16)==15) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META("\n\t\t")));
            }
        }
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\n")));
        break;
    case SOC_SER_LOG_TLV_SER_FIFO:
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\tSOC_SER_LOG_TLV_SER_FIFO\n\t\t")));
        for (index = 0; index < tlv_hdr->length; index++) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META("%02x "), *((char*)(data + index))));
            if ((index%16)==15) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META("\n\t\t")));
            }
        }
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\n")));
        break;
    case SOC_SER_LOG_TLV_CACHE:
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\tSOC_SER_LOG_TLV_CACHE\n\t\t")));
        for (index = 0; index < tlv_hdr->length; index++) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META("%02x "), *((char*)(data + index))));
            if ((index%16)==15) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META("\n\t\t")));
            }
        }
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\n")));
        break;
    case SOC_SER_LOG_TLV_GENERIC:
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\tSOC_SER_LOG_TLV_GENERIC\n")));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tflags: %d\n"),
                   (int)(((soc_ser_log_tlv_generic_t*)(data))->flags)));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\ttime: %d\n"),
                   (int)(((soc_ser_log_tlv_generic_t*)(data))->time)));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tboot_count: %d\n"),
                   (int)(((soc_ser_log_tlv_generic_t*)(data))->boot_count)));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\taddress: %d\n"),
                   (int)(((soc_ser_log_tlv_generic_t*)(data))->address)));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tacc_type: %d\n"),
                   (int)(((soc_ser_log_tlv_generic_t*)(data))->acc_type)));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tblock_type: %d\n"),
                   (int)(((soc_ser_log_tlv_generic_t*)(data))->block_type)));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tparity_type: %d\n"),
                   (int)(((soc_ser_log_tlv_generic_t*)(data))->parity_type)));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tser_response_flag: %d\n"),
                   (int)(((soc_ser_log_tlv_generic_t*)(data))->ser_response_flag)));
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\t\tcorrected: %d\n"),
                   (int)(((soc_ser_log_tlv_generic_t*)(data))->corrected)));
        break;
    default:
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("\tUnknown type\n")));
    }
    return SOC_E_NONE;
}

/* print everything */
int
soc_ser_log_print_all(int unit)
{
    int id = 0, size; 
    char * buffer[_SOC_SER_LOG_BUFFER_SIZE];
    
    do {
        id = soc_log_buf_get_next_id(_soc_ser_log_buffer_ptr[unit], id);

        if (id == 0) {
            return SOC_E_NONE;
        }

        size = soc_ser_log_get_entry_size(unit, id);
        if ((size <= 0) || (size > _SOC_SER_LOG_BUFFER_SIZE)) {
            return SOC_E_INTERNAL;
        }

        soc_ser_log_get_entry(unit, id, size, buffer);
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Log Entry ID:%d\n"), id));
        soc_ser_log_print_entry(buffer);
    } while (1);
    return SOC_E_NONE;
}

int
soc_ser_log_find_tlv(void *buffer, soc_ser_log_tlv_type_t type)
{
    soc_ser_log_tlv_hdr_t *tlv_hdr = buffer;
    int offset = 0;

    while ((tlv_hdr->type != type) && (tlv_hdr->type != 0)) {
        offset += tlv_hdr->length + sizeof(soc_ser_log_tlv_hdr_t);
        tlv_hdr = (soc_ser_log_tlv_hdr_t*)((char*)tlv_hdr +
            tlv_hdr->length + sizeof(soc_ser_log_tlv_hdr_t));
    }

    if (tlv_hdr->type == 0) {
        return SOC_E_PARAM;
    }
    return offset;
}

int
soc_ser_log_invalidate(int unit)
{
    sal_mutex_t mut = soc_log_buf_get_mutex(_soc_ser_log_buffer_ptr[unit]);
    if (mut == 0) {
        return SOC_E_INTERNAL;
    }
    soc_log_buf_set_mutex(_soc_ser_log_buffer_ptr[unit], 0);

    sal_mutex_destroy(mut);
    return SOC_E_NONE;
}

int
soc_ser_log_load(int unit, void *location)
{
    sal_mutex_t mut = sal_mutex_create("SER_LOG_MUTEX");

    if (mut == NULL) {
        return SOC_E_RESOURCE;
    }

    if (location == NULL) {
        return SOC_E_PARAM;
    }

    _soc_ser_log_buffer_ptr[unit] = location;

    return SOC_E_NONE;
}

int 
soc_ser_log_init(int unit, void *location, int size) 
{
    int buffer_length;

    sal_mutex_t mut = sal_mutex_create("SER_LOG_MUTEX");

    if (mut == NULL) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "SER Logging failed to create mutex\n")));
        return SOC_E_RESOURCE;
    }
   
    if (location == NULL) {
        _soc_ser_log_buffer_ptr[unit] = _soc_ser_log_buffer[unit];
        buffer_length = _SOC_SER_LOG_BUFFER_SIZE;
    } else {
        _soc_ser_log_buffer_ptr[unit] = location;
        buffer_length = size;
    }
    soc_log_buf_init(_soc_ser_log_buffer_ptr[unit], buffer_length, mut);

    return SOC_E_NONE;
}

int
soc_ser_log_get_boot_count(int unit)
{
    return soc_log_buf_get_boot_count(_soc_ser_log_buffer_ptr[unit]);
}
