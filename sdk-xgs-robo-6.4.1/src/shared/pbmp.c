/*
 * $Id: pbmp.c,v 1.3 Broadcom SDK $
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
 * Port Bitmap helper functions
 * Only used if _SHR_DEFINE_PBMP_FUNCTIONS is set in <shared/pbmp.h>
 * (everything is done inline in <shared/pbmp.h> otherwise)
 *
 * Well, actually _shr_pbmp_format() is always available
 */

#include <sal/types.h>
#include <shared/util.h>
#include <shared/pbmp.h>

#ifdef _SHR_DEFINE_PBMP_FUNCTIONS

/* returns 1 is the bitmap is empty */
int
_shr_pbmp_bmnull(_shr_pbmp_t *bmp)
{
    int	i;

    for (i = 0; i < _SHR_PBMP_WORD_MAX; i++) {
	if (_SHR_PBMP_WORD_GET(*bmp, i) != 0) {
	    return 0;
	}
    }
    return 1;
}

/* returns 1 is the two bitmaps are equal */
int
_shr_pbmp_bmeq(_shr_pbmp_t *bmp1, _shr_pbmp_t *bmp2)
{
    int	i;

    for (i = 0; i < _SHR_PBMP_WORD_MAX; i++) {
	if (_SHR_PBMP_WORD_GET(*bmp1, i) != _SHR_PBMP_WORD_GET(*bmp2, i)) {
	    return 0;
	}
    }
    return 1;
}

#endif /* _SHR_DEFINE_PBMP_FUNCTIONS */

/* format a bitmap into a static buffer suitable for printing */
char *
_shr_pbmp_format(_shr_pbmp_t bmp, char *buf)
{
    int		i;
    char	*bp;

    if (buf == NULL) {
	return buf;
    }
    buf[0] = '0';
    buf[1] = 'x';
    bp = &buf[2];
    for (i = _SHR_PBMP_WORD_MAX-1; i >= 0; i--) {
	_shr_format_integer(bp, _SHR_PBMP_WORD_GET(bmp, i), 8, 16);
	bp += 8;
    }
    return buf;
}

/*
 * decode a string in hex format into a bitmap
 * returns 0 on success, -1 on error
 */
int
_shr_pbmp_decode(char *s, _shr_pbmp_t *bmp)
{
    char	*e;
    uint32	v;
    int		p;

    _SHR_PBMP_CLEAR(*bmp);

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
	/* get end of string */
	s += 2;
	for (e = s; *e; e++)
	    ;
	e -= 1;
	/* back up to beginning of string, setting ports as we go */
	p = 0;
	while (e >= s) {
	    if (*e >= '0' && *e <= '9') {
		v = *e - '0';
	    } else if (*e >= 'a' && *e <= 'f') {
		v = *e - 'a' + 10;
	    } else if (*e >= 'A' && *e <= 'F') {
		v = *e - 'A' + 10;
	    } else {
		return -1;		/* error: invalid hex digits */
	    }
	    e -= 1;
	    /* now set a nibble's worth of ports */
	    if ((v & 1) && p < _SHR_PBMP_PORT_MAX) {
		_SHR_PBMP_PORT_ADD(*bmp, p);
	    }
	    p += 1;
	    if ((v & 2) && p < _SHR_PBMP_PORT_MAX) {
		_SHR_PBMP_PORT_ADD(*bmp, p);
	    }
	    p += 1;
	    if ((v & 4) && p < _SHR_PBMP_PORT_MAX) {
		_SHR_PBMP_PORT_ADD(*bmp, p);
	    }
	    p += 1;
	    if ((v & 8) && p < _SHR_PBMP_PORT_MAX) {
		_SHR_PBMP_PORT_ADD(*bmp, p);
	    }
	    p += 1;
	}
    } else {
	v = 0;
	while (*s >= '0' && *s <= '9') {
	    v = v * 10 + (*s++ - '0');
	}
	if (*s != '\0') {
	    return -1;			/* error: invalid decimal digits */
	}
	p = 0;
	while (v) {
	    if ((v & 1) && p < _SHR_PBMP_PORT_MAX) {
		_SHR_PBMP_PORT_ADD(*bmp, p);
	    }
	    v >>= 1;
	    p += 1;
	}
    }
    return 0;
}
