/*
 * $Id: t_util.h 1.5 Broadcom SDK $
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
 * File:        t_util.h
 * Purpose:     Useful transport macros
 */

#ifndef   _T_UTIL_H_
#define   _T_UTIL_H_


#define PACK_SHORT(buf, val) \
    do {                                               \
        uint16 v2;                                     \
        v2 = bcm_htons(val);                           \
        sal_memcpy(buf, &v2, sizeof(uint16));           \
    } while (0)

#define PACK_LONG(buf, val) \
    do {                                               \
        uint32 v2;                                     \
        v2 = bcm_htonl(val);                           \
        sal_memcpy(buf, &v2, sizeof(uint32));           \
    } while (0)

#define UNPACK_SHORT(buf, val) \
    do {                                               \
        sal_memcpy(&(val), buf, sizeof(uint16));         \
        val = bcm_ntohs(val);                          \
    } while (0)

#define UNPACK_LONG(buf, val) \
    do {                                               \
        sal_memcpy(&(val), buf, sizeof(uint32));         \
        val = bcm_ntohl(val);                          \
    } while (0)


#define	CT_INCR_PACK_U8(_buf, _var) \
		*_buf++ = (_var)
#define	CT_INCR_UNPACK_U8(_buf, _var) \
		_var = *_buf++

#define	CT_INCR_PACK_U16(_buf, _var) \
		*_buf++ = ((_var) >> 8) & 0xff; \
		*_buf++ = (_var) & 0xff;
#define	CT_INCR_UNPACK_U16(_buf, _var) \
		_var  = *_buf++ << 8; \
		_var |= *_buf++; 

#define	CT_INCR_PACK_U32(_buf, _var) \
		*_buf++ = ((_var) >> 24) & 0xff; \
		*_buf++ = ((_var) >> 16) & 0xff; \
		*_buf++ = ((_var) >> 8) & 0xff; \
		*_buf++ = (_var) & 0xff;
#define	CT_INCR_UNPACK_U32(_buf, _var) \
		_var  = *_buf++ << 24; \
		_var |= *_buf++ << 16; \
		_var |= *_buf++ << 8; \
		_var |= *_buf++; 

#endif /* _T_UTIL_H_ */
