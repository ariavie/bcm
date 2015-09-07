/*
 * $Id: pack_pbmp.c,v 1.1 Broadcom SDK $
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
 * File:	pack_pbmp.c
 * Purpose:	BCM API Type Packers - port bitmaps
 */

#include <bcm/types.h>
#include <shared/pbmp.h>
#include <bcm_int/rpc/pack.h>

/*
 * Two different mechanisms for packing bcm_pbmp_t structures are provided.
 *
 * If BCM_RPC_PBMP_64 is not defined then rpc version 3 is in use and port
 * bitmaps are packed as:
 *	<uint8 nword><uint32 word[0]>...<uint32 word[nword-1]>
 * This packing method handles bcm implementations compiled with differing
 * lengths of port bitmaps and is potentially more efficient in packing size
 * (zero value trailing words are not packed)
 *
 * If BCM_RPC_PBMP_64 is defined, then rpc version 2 is in use and
 * port bitmaps are packed as:
 *	<uint32 word[0]><uint32 word[1]>
 * The second packing method is limited to 64 bit port bitmaps but is
 * backwards compatible with sdk-5.3 and sdk-5.4 releases.
 */

#ifndef	BCM_RPC_PBMP_64

uint8 *
_bcm_pack_pbmp(uint8 *buf, bcm_pbmp_t *var)
{
    uint8	nwords;
    int		i;

    /* back up over zero values words */
    for (i = _SHR_PBMP_WORD_MAX-1; i >= 0; i--) {
	if (_SHR_PBMP_WORD_GET(*var, i) != 0) {
	    break;
	}
    }
    nwords = i+1;
    BCM_PACK_U8(buf, nwords);
    for (i = 0; i < nwords; i++) {
	BCM_PACK_U32(buf, _SHR_PBMP_WORD_GET(*var, i));
    }
    return buf;
}

uint8 *
_bcm_unpack_pbmp(uint8 *buf, bcm_pbmp_t *var)
{
    uint8	nwords;
    uint32	pword;
    int		i;

    BCM_UNPACK_U8(buf, nwords);
    for (i = 0; i < nwords; i++) {
	BCM_UNPACK_U32(buf, pword);
	if (i < _SHR_PBMP_WORD_MAX) {
	    _SHR_PBMP_WORD_SET(*var, i, pword);
	}
    }
    /* clear any remaining words */
    for (i = nwords; i < _SHR_PBMP_WORD_MAX; i++) {
	_SHR_PBMP_WORD_SET(*var, i, 0);
    }
    return buf;
}

#else	/* BCM_RPC_PBMP_64 */

uint8 *
_bcm_pack_pbmp(uint8 *buf, bcm_pbmp_t *var)
{
    BCM_PACK_U32(buf, var->pbits[0]);
    BCM_PACK_U32(buf, var->pbits[1]);
    return buf;
}

uint8 *
_bcm_unpack_pbmp(uint8 *buf, bcm_pbmp_t *var)
{
    BCM_UNPACK_U32(buf, var->pbits[0]);
    BCM_UNPACK_U32(buf, var->pbits[1]);
    return buf;
}

#endif	/* BCM_RPC_PBMP_64 */
