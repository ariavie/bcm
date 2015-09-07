/*
 * local version of endian.h - byte order defines
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
 *  $Id: bcmendian.h,v 1.1 Broadcom SDK $
*/

#ifndef _BCMENDIAN_H_
#define _BCMENDIAN_H_

#include "typedefs.h"

/* Byte swap a 16 bit value */
#define BCMSWAP16(val) \
	((uint16)(\
		(((uint16)(val) & (uint16)0x00ffU) << 8) | \
		(((uint16)(val) & (uint16)0xff00U) >> 8)))

/* Byte swap a 32 bit value */
#define BCMSWAP32(val) \
	((uint32)(\
		(((uint32)(val) & (uint32)0x000000ffUL) << 24) | \
		(((uint32)(val) & (uint32)0x0000ff00UL) <<  8) | \
		(((uint32)(val) & (uint32)0x00ff0000UL) >>  8) | \
		(((uint32)(val) & (uint32)0xff000000UL) >> 24)))

/* 2 Byte swap a 32 bit value */
#define BCMSWAP32BY16(val) \
	((uint32)(\
		(((uint32)(val) & (uint32)0x0000ffffUL) << 16) | \
		(((uint32)(val) & (uint32)0xffff0000UL) >> 16)))


static INLINE uint16
bcmswap16(uint16 val)
{
	return BCMSWAP16(val);
}

static INLINE uint32
bcmswap32(uint32 val)
{
	return BCMSWAP32(val);
}

static INLINE uint32
bcmswap32by16(uint32 val)
{
	return BCMSWAP32BY16(val);
}

/* buf	- start of buffer of shorts to swap */
/* len  - byte length of buffer */
static INLINE void
bcmswap16_buf(uint16 *buf, uint len)
{
	len = len/2;

	while (len--) {
		*buf = bcmswap16(*buf);
		buf++;
	}
}

#ifndef hton16
#ifndef IL_BIGENDIAN
#define HTON16(i) BCMSWAP16(i)
#define	hton16(i) bcmswap16(i)
#define	hton32(i) bcmswap32(i)
#define	ntoh16(i) bcmswap16(i)
#define	ntoh32(i) bcmswap32(i)
#define ltoh16(i) (i)
#define ltoh32(i) (i)
#define htol16(i) (i)
#define htol32(i) (i)
#else
#define HTON16(i) (i)
#define	hton16(i) (i)
#define	hton32(i) (i)
#define	ntoh16(i) (i)
#define	ntoh32(i) (i)
#define	ltoh16(i) bcmswap16(i)
#define	ltoh32(i) bcmswap32(i)
#define htol16(i) bcmswap16(i)
#define htol32(i) bcmswap32(i)
#endif /* IL_BIGENDIAN */
#endif /* hton16 */

#ifndef IL_BIGENDIAN
#define ltoh16_buf(buf, i)
#define htol16_buf(buf, i)
#else
#define ltoh16_buf(buf, i) bcmswap16_buf((uint16*)buf, i)
#define htol16_buf(buf, i) bcmswap16_buf((uint16*)buf, i)
#endif /* IL_BIGENDIAN */

/*
* store 16-bit value to unaligned little endian byte array.
*/
static INLINE void
htol16_ua_store(uint16 val, uint8 *bytes)
{
	bytes[0] = val&0xff;
	bytes[1] = val>>8;
}

/*
* store 32-bit value to unaligned little endian byte array.
*/
static INLINE void
htol32_ua_store(uint32 val, uint8 *bytes)
{
	bytes[0] = val&0xff;
	bytes[1] = (val>>8)&0xff;
	bytes[2] = (val>>16)&0xff;
	bytes[3] = val>>24;
}

/*
* store 16-bit value to unaligned network(big) endian byte array.
*/
static INLINE void
hton16_ua_store(uint16 val, uint8 *bytes)
{
	bytes[1] = val&0xff;
	bytes[0] = val>>8;
}

/*
* store 32-bit value to unaligned network(big) endian byte array.
*/
static INLINE void
hton32_ua_store(uint32 val, uint8 *bytes)
{
	bytes[3] = val&0xff;
	bytes[2] = (val>>8)&0xff;
	bytes[1] = (val>>16)&0xff;
	bytes[0] = val>>24;
}

/*
* load 16-bit value from unaligned little endian byte array.
*/
static INLINE uint16
ltoh16_ua(void *bytes)
{
	return (((uint8*)bytes)[1]<<8)+((uint8 *)bytes)[0];
}

/*
* load 32-bit value from unaligned little endian byte array.
*/
static INLINE uint32
ltoh32_ua(void *bytes)
{
	return (((uint8*)bytes)[3]<<24)+(((uint8*)bytes)[2]<<16)+
	       (((uint8*)bytes)[1]<<8)+((uint8*)bytes)[0];
}

/*
* load 16-bit value from unaligned big(network) endian byte array.
*/
static INLINE uint16
ntoh16_ua(void *bytes)
{
	return (((uint8*)bytes)[0]<<8)+((uint8*)bytes)[1];
}

/*
* load 32-bit value from unaligned big(network) endian byte array.
*/
static INLINE uint32
ntoh32_ua(void *bytes)
{
	return (((uint8*)bytes)[0]<<24)+(((uint8*)bytes)[1]<<16)+
	       (((uint8*)bytes)[2]<<8)+((uint8*)bytes)[3];
}

#define ltoh_ua(ptr) (\
	sizeof(*(ptr)) == sizeof(uint8) ?  *(uint8 *)ptr : \
	sizeof(*(ptr)) == sizeof(uint16) ? (((uint8 *)ptr)[1]<<8)+((uint8 *)ptr)[0] : \
	(((uint8 *)ptr)[3]<<24)+(((uint8 *)ptr)[2]<<16)+(((uint8 *)ptr)[1]<<8)+((uint8 *)ptr)[0] \
)

#define ntoh_ua(ptr) (\
	sizeof(*(ptr)) == sizeof(uint8) ?  *(uint8 *)ptr : \
	sizeof(*(ptr)) == sizeof(uint16) ? (((uint8 *)ptr)[0]<<8)+((uint8 *)ptr)[1] : \
	(((uint8 *)ptr)[0]<<24)+(((uint8 *)ptr)[1]<<16)+(((uint8 *)ptr)[2]<<8)+((uint8 *)ptr)[3] \
)

#endif /* _BCMENDIAN_H_ */
