/* 
 * $Id: pack.h 1.4 Broadcom SDK $
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
 * File:        pack.h
 * Purpose:     Pack and unpack macros using Big-Endian (network byte order).
 *
 * Pack and unpack macros:
 *   _SHR_PACK_U8    : Packs a uint8, advances pointer
 *   _SHR_UNPACK_U8  : Unpacks a uint8, advances pointer
 *   _SHR_PACK_U16   : Packs a uint16, advances pointer
 *   _SHR_UNPACK_U16 : Unpacks a uint16, advances pointer
 *   _SHR_PACK_U32   : Packs a uint32, advances pointer
 *   _SHR_UNPACK_U32 : Unpacks a uint32, advances pointer
 *
 * The pack/unpack macros take (_buf, _var)
 * where,
 *     _buf  - is a pointer to buffer where to pack or unpack value;
 *             should be of type 'uint8 *', or 'char *'.
 *     _var  - is the variable with value to pack, or to unpack to.
 * 
 * NOTE:
 * All above macros increment the given buffer pointer based
 * on the corresponding type size.
 *
 * Avoid expressions in these macros (side effects).
 */

#ifndef   _SHR_PACK_H_
#define   _SHR_PACK_H_

/* Type length in bytes */
#define _SHR_PACKLEN_U8     1
#define _SHR_PACKLEN_U16    2
#define _SHR_PACKLEN_U32    4

#define _SHR_PACK_U8(_buf, _var)        \
    *_buf++ = (_var) & 0xff

#define _SHR_UNPACK_U8(_buf, _var)      \
    _var = *_buf++

#define _SHR_PACK_U16(_buf, _var)           \
    do {                                    \
        (_buf)[0] = ((_var) >> 8) & 0xff;   \
        (_buf)[1] = (_var) & 0xff;          \
        (_buf) += _SHR_PACKLEN_U16;         \
    } while (0)

#define _SHR_UNPACK_U16(_buf, _var)         \
    do {                                    \
        (_var) = (((_buf)[0] << 8) |        \
                  (_buf)[1]);               \
        (_buf) += _SHR_PACKLEN_U16;         \
    } while (0)

#define _SHR_PACK_U32(_buf, _var)           \
    do {                                    \
        (_buf)[0] = ((_var) >> 24) & 0xff;  \
        (_buf)[1] = ((_var) >> 16) & 0xff;  \
        (_buf)[2] = ((_var) >> 8) & 0xff;   \
        (_buf)[3] = (_var) & 0xff;          \
        (_buf) += _SHR_PACKLEN_U32;         \
    } while (0)

#define _SHR_UNPACK_U32(_buf, _var)         \
    do {                                    \
        (_var) = (((_buf)[0] << 24) |       \
                  ((_buf)[1] << 16) |       \
                  ((_buf)[2] << 8)  |       \
                  (_buf)[3]);               \
        (_buf) += _SHR_PACKLEN_U32;         \
    } while (0)

#endif /* _SHR_PACK_H_ */
