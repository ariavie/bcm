/*
 * $Id: util.h,v 1.17 Broadcom SDK $
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
 * Driver utility routines
 */

#ifndef _SOC_UTIL_H
#define _SOC_UTIL_H

#include <sal/core/time.h>
#include <shared/util.h>
#include <soc/ethdma.h>
/*
 * Timeout support based on SAL microsecond timer
 */
#define SOC_TIGHTLOOP_DELAY_LIMIT_USECS 10

typedef struct soc_timeout_s {
    sal_usecs_t		expire;
    sal_usecs_t		usec;
    int			min_polls;
    int			polls;
    sal_usecs_t		exp_delay;
} soc_timeout_t;

extern void soc_timeout_init(soc_timeout_t *to, uint32 usec, int min_polls);
extern int soc_timeout_check(soc_timeout_t *to);
extern sal_usecs_t soc_timeout_elapsed(soc_timeout_t *to);
extern int soc_tightdelay_timeout_check(soc_timeout_t *to);

extern void soc_bits_get(uint32 *str, uint32 minbit, uint32 maxbit, void *data_vp);

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
extern int _soc_max_blocks_per_entry_get(void);
#endif
#if defined(BCM_ROBO_SUPPORT)
extern int _soc_robo_max_blocks_per_entry_get(void);
#endif

/*
 * The following load/store routines interpret 2 or 4 arbitrarily
 * aligned bytes, usually in a packet buffer and representing a value in
 * network-endian byte order, as unsigned integer values.
 */

extern	uint32 soc_ntohl_load(const void *);
extern	uint16 soc_ntohs_load(const void *);
extern	uint32 soc_htonl_store(void *, uint32);
extern	uint16 soc_htons_store(void *, uint16);

extern  uint32 soc_letohl_load(const void *a);
extern  uint16 soc_letohs_load(const void *a);
extern  uint32 soc_htolel_store(void *a, uint32 v);
extern  uint16 soc_htoles_store(void *a, uint16 v);

extern uint32 cal_crc32(eth_dv_t *dv, int skip_brcmtag);

/* General operations for shift & mask for flags */
/* mask is always justified to low order bits; 2^n - 1 where n is width. */
#define SOC_SM_FLAGS_GET(flags, shift, mask) (((flags) >> (shift)) & (mask))
#define SOC_SM_FLAGS_SET(flags, val, shift, mask) \
                    do { \
                        ((flags) &= (~((uint32)((mask) << (shift))))); \
                        ((flags) |= (((val)&(mask)) << (shift))); \
                    } while (0)

/*
 * Macro:
 *	soc_htonl (soc_htons)
 * Purpose:
 *	Convert host-endian 32-bit value (16-bit value) to big-endian.
 * Notes:
 *	Only swaps if endian conversion is required for current host type.
 *
 * Macro:
 *	soc_ntohl (soc_ntohs)
 * Purpose:
 *	Convert big-endian 32-bit value (16-bit value) to host-endian.
 * Notes:
 *	Only swaps if endian conversion is required for current host type.
 */

#if defined(LE_HOST)

#define	soc_htonl(_l)	_shr_swap32(_l)
#define	soc_htons(_s)	_shr_swap16(_s)
#define	soc_ntohl(_l)	_shr_swap32(_l)
#define	soc_ntohs(_s)	_shr_swap16(_s)

#else /* BE_HOST */

#define	soc_htonl(_l)	(_l)
#define	soc_htons(_s)	(_s)
#define	soc_ntohl(_l)	(_l)
#define	soc_ntohs(_s)	(_s)

#endif /* BE_HOST */

#define MAX_DEV_NAME_LEN   25
#define MAX_FW_TYPES       8
#define NO_FW              ((uint8 *) -1)
#define MAX_FW_BUF_LEN     0x80000

typedef struct fw_desc_s {
    const char    *dev_name;
    uint8         *fw;
    int           fw_len;
} fw_desc_t;

extern void soc_phy_fw_init(void);
extern int soc_phy_fw_get(char *dev_name, uint8 **fw, int *fw_len);
extern void soc_phy_fw_put_all(void);

extern int (*soc_phy_fw_acquire)(const char *dev_name, uint8 **fw, int *fw_len);
extern int (*soc_phy_fw_release)(const char *dev_name, uint8 *fw, int fw_len);

#endif	/* !_SOC_UTIL_H */
