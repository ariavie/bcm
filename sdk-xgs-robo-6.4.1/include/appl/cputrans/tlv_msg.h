/* 
 * $Id: tlv_msg.h,v 1.3 Broadcom SDK $
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
 * File:        tlv_msg.h
 * Purpose:     TLV Message Utility
 */

#ifndef   _TLV_MSG_H_
#define   _TLV_MSG_H_

#include <sal/types.h>
#include <bcm/types.h>


/*
 *    TLV MESSAGE
 *
 *   +--------------+-----------------------------------------+--------------+
 *   |    HEADER    |                  TLV(s)                 | TLV_TYPE_END |
 *   +--------------+-----------------------------------------+--------------+
 *
 *
 *   TLV
 *    0    7 8           23 24                     
 *   +------+--------------+-----------------------------------+
 *   | TYPE |    LENGTH    |              VALUE                |
 *   +------+--------------+-----------------------------------+
 *                         |<------------- LENGTH------------->|
 *
 *
 *   TLV Message
 *   - The TLV message header is optional and is defined by the application.
 *   - A TLV message contains 0 or more TLV elements.
 *   - A TLV message is terminated by a special TLV of type TLV_TYPE_END,
 *     and there is no length field.
 *
 *   TLV Fields
 *   Type   - indicates type of information, defined as "tlv_msg_type_t".
 *            Type 0 is reserved.
 *   Length - is the size, in bytes, of the data to follow for this element,
 *            defined as "tlv_msg_length_t".
 *   Value  - is the variable sized set of bytes which contains
 *            data for this element.
 *
 *   NOTES:
 *   - Application must know the header format and 'get' all values
 *     expected in the header before retrieving the TLVs.
 *   - Application does not need to 'get' all values in the TLV in order
 *     to move to the next TLV.
 */

typedef uint8  tlv_msg_type_t;
typedef uint16 tlv_msg_length_t;

/* TLV type to indicates end of message */
#define TLV_TYPE_END            0

typedef struct tlv_msg_s {
    uint8            *start;          /* Start of message in buffer */
    uint8            *end;            /* End of message in buffer */
    uint8            *buffer_end;     /* End of buffer */
    uint8            *cur_ptr;        /* Current ptr in buffer to write/read */
    uint8            *tlv_length_ptr; /* Ptr to current TLV length in buffer */
    tlv_msg_length_t  tlv_length;     /* Length for current TLV */
    int               tlv_left;       /* Bytes left to read in current TLV */
} tlv_msg_t;


/* TLV message struct initializer */
extern void tlv_msg_t_init(tlv_msg_t *msg);

/* TLV message buffer */
extern int tlv_msg_buffer_set(tlv_msg_t *msg, uint8 *buffer, int length);

/* TLVs */
extern int tlv_msg_add(tlv_msg_t *msg, tlv_msg_type_t type);
extern int tlv_msg_get(tlv_msg_t *msg, tlv_msg_type_t *type,
                       tlv_msg_length_t *length);

/*
 * Value adders
 *
 * Adds values into a TLV message.  If there are multiple values associated
 * with a TLV message header or a TLV type, the routine of the correct
 * type must be called for each one.
 */
extern int tlv_msg_uint8_add(tlv_msg_t *msg, uint8 value);
extern int tlv_msg_uint16_add(tlv_msg_t *msg, uint16 value);
extern int tlv_msg_uint32_add(tlv_msg_t *msg, uint32 value);
extern int tlv_msg_string_add(tlv_msg_t *msg, const char *value);

/*
 * Value getters
 *
 * Gets values from a TLV message.  If there are multiple values
 * associated with a TLV message header or a TLV type, the
 * getter must be called in the same order and use the same type
 * as the corresponding adder.
 */
extern int tlv_msg_uint8_get(tlv_msg_t *msg, uint8 *value);
extern int tlv_msg_uint16_get(tlv_msg_t *msg, uint16 *value);
extern int tlv_msg_uint32_get(tlv_msg_t *msg, uint32 *value);
extern int tlv_msg_string_get(tlv_msg_t *msg, int value_max, char *value);

extern int tlv_msg_length(tlv_msg_t *msg, int *length);

extern int tlv_msg_resize(tlv_msg_t *msg, uint8 *buffer, int length);


#endif /* _TLV_MSG_H_ */
