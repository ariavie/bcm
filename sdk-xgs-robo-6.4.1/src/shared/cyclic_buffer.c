/*
 * $Id: cyclic_buffer.c,v 1.2 Broadcom SDK $
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
 *
 * SHARED CYCLIC BUFFER
 */
 

#include <shared/error.h>
#include <shared/cyclic_buffer.h>
#include <sal/core/alloc.h>
#include <sal/core/libc.h>
#include <soc/error.h>


int 
cyclic_buffer_create(int unit, cyclic_buffer_t* buffer, int max_entry_size, int max_buffered_elements, char* buffer_name)
{
    if(NULL == buffer) {
        return _SHR_E_PARAM;
    }

    buffer->elements = (uint8*)sal_alloc(max_buffered_elements*max_entry_size,buffer_name);
    if(NULL == buffer->elements) {
        return _SHR_E_MEMORY;
    }

    buffer->oldest = 0;
    buffer->count = 0;
    buffer->max_allowed = max_buffered_elements;
    buffer->entry_size = max_entry_size;

    return _SHR_E_NONE;
}

int 
cyclic_buffer_destroy(int unit, cyclic_buffer_t* buffer)
{
    if(NULL == buffer) {
        return _SHR_E_PARAM;
    }

    if(NULL != buffer->elements) {
        SOC_FREE(buffer->elements);
    }
    buffer->elements = NULL;
    buffer->oldest = 0;
    buffer->count = 0;
    buffer->max_allowed = 0;
    buffer->entry_size = 0;

    return _SHR_E_NONE;
}

int 
cyclic_buffer_add(int unit, cyclic_buffer_t* buffer, const void* new_element)
{
    int rc, is_full;
    int free_pos;
    
    if(NULL == buffer || NULL == new_element) {
        return _SHR_E_PARAM;
    }

    if(NULL == buffer->elements) {
        return _SHR_E_INIT;
    }

    rc = cyclic_buffer_is_full(unit, buffer, &is_full);
    _SHR_E_IF_ERROR_RETURN(rc);
    if(is_full) {
        return _SHR_E_FULL;
    }

    free_pos = (buffer->oldest + buffer->count) % buffer->max_allowed;
    sal_memcpy(&(buffer->elements[free_pos*buffer->entry_size]), (const uint8*)new_element, buffer->entry_size);
    buffer->count++;
    
    return _SHR_E_NONE;
}

int 
cyclic_buffer_get(int unit, cyclic_buffer_t* buffer, void* received_element)
{
    int rc, is_empty;

    if(NULL == buffer || NULL == received_element) {
        return _SHR_E_PARAM;
    }

    if(NULL == buffer->elements) {
        return _SHR_E_INIT;
    }

    rc = cyclic_buffer_is_empty(unit, buffer, &is_empty);
    _SHR_E_IF_ERROR_RETURN(rc);
    if(is_empty) {
        return _SHR_E_EMPTY;
    }

    sal_memcpy((uint8*)received_element, &(buffer->elements[buffer->oldest*buffer->entry_size]), buffer->entry_size);
    buffer->oldest = (buffer->oldest+1) % buffer->max_allowed;
    buffer->count--;

    return _SHR_E_NONE;
}

int 
cyclic_buffer_is_empty(int unit, const cyclic_buffer_t* buffer, int* is_empty)
{
    if(NULL == buffer || NULL == is_empty) {
        return _SHR_E_PARAM;
    }

    if(NULL == buffer->elements) {
        return _SHR_E_INIT;
    }

    *is_empty = (0 == buffer->count ? 1 : 0);

    return _SHR_E_NONE;
}

int 
cyclic_buffer_is_full(int unit, const cyclic_buffer_t* buffer, int* is_full)
{

    if(NULL == buffer || NULL == is_full) {
        return _SHR_E_PARAM;
    }

    if(NULL == buffer->elements) {
        return _SHR_E_INIT;
    }

    *is_full = (buffer->max_allowed == buffer->count ? 1 : 0);

    return _SHR_E_NONE;
}

int 
cyclic_buffer_cells_count(int unit, const cyclic_buffer_t* buffer, int* count)
{
    if(NULL == buffer || NULL == count) {
        return _SHR_E_PARAM;
    }

    if(NULL == buffer->elements) {
        return _SHR_E_INIT;
    }

    *count = buffer->count;

    return _SHR_E_NONE;
}


