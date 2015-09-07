/*
 * $Id: meter.c,v 1.8 Broadcom SDK $
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
 * File: 	meter.c
 * Purpose: 	Trident placeholder for metering, FP metering.
 */

#include <soc/defs.h>
#if defined(BCM_TRIDENT_SUPPORT)

#include <sal/core/libc.h>
#include <bcm_int/esw/trident.h>

#define METER_GRANULARITY_DEFAULT       3
#define METER_GRANULARITY_NUM           8
#define METER_KBITS_SEC_QUANTUM_MIN     8       /* 8000 bits/second */
#define METER_BITS_BURST_MIN            (256 * 8) /* 256 bytes */
#define METER_PACKET_SEC_QUANTUM_MIN    1
#define METER_MMU_PACKET_BURST_MIN      256     /* 0.256 packet, use 1000x */
#define METER_FP_PACKET_BURST_MIN       512     /* 0.512 packet, use 1000x */
#define METER_PACKET_BURST_DIVISOR      1000    /* use 1000x for calculation */
#define METER_NL_SEGMENT_GRANULARITY    256
#define METER_NL_BUCKET_POWER_SHIFT     8
#define METER_NL_BUCKET_RANGE_NUM       16
#define METER_NL_BUCKET_SEGMENT_MASK    0xff
#define METER_NL_BUCKET_POWER_MASK      0xf
#define METER_NL_BUCKET_MAC_ENCODE_MAX  0xff80 /* 2 ** 0xf * (1 + 0xff/256) */

STATIC void
_bcm_td_granularity_params(int unit, int granularity, uint32 flags,
                           uint32 *rate_unit_size, uint32 *burst_unit_size)
{
    uint32 gran_multiple;
    static const uint32 fp_pkt_mode_gran_multiple[] = {
        1, 2, 4, 8, 16, 32, 64, 512
    };
    static const uint32 mmu_pkt_mode_gran_multiple[] = {
        1, 2, 4, 8, 16, 64, 256, 1024
    };

    if (flags & _BCM_TD_METER_FLAG_PACKET_MODE) {
        if (flags &_BCM_TD_METER_FLAG_FP_POLICER) {
            gran_multiple = fp_pkt_mode_gran_multiple[granularity];
            *burst_unit_size = METER_FP_PACKET_BURST_MIN * gran_multiple;
        } else {
            gran_multiple = mmu_pkt_mode_gran_multiple[granularity];
            *burst_unit_size = METER_MMU_PACKET_BURST_MIN * gran_multiple;
        }
        *rate_unit_size = METER_PACKET_SEC_QUANTUM_MIN * gran_multiple;
    } else {
        gran_multiple = 1 << granularity;
        *rate_unit_size = METER_KBITS_SEC_QUANTUM_MIN * gran_multiple;
        *burst_unit_size = METER_BITS_BURST_MIN * gran_multiple;
    }
}

/*
 * Function:
 *	_bcm_td_kbits_to_bucket_encoding
 * Description:
 *      Routine that converts desired burst size into leaky bucketsize
 *      register value.
 * Parameters:
 *      rate_quantum - Desired rate of ongoing traffic
 *      burst_quantum - Desired size of maximum burst traffic
 *      flags - paramters indicating capabilities and configuration of meter
 *      refresh_bitsize - number of bits for the refresh rate field
 *      bucket_max_bitsize - number of bits for the bucket max field
 *      refresh_rate - (OUT) calculated value for the refresh rate field
 *      bucket_max - (OUT) calculated value for the bucket max field
 *      granularity - (OUT) calculated value for the granularity field
 * Returns:
 *	BCM_E_*
 * Notes:
 *
 * bandwidth(byte) mode
 * gran  refresh      refresh            bucket-size-granularity
 *       byte/refresh kbit/second        bits
 *                     min <-> max         base <-> max
 *                                                  (linear)   / (ITU mode)
 * 0     1/128           8 <-> 0x1ffff8    2048 <-> 0x7ff800   / 0x7fc0000
 * 1     1/64           16 <-> 0x3ffff0    4096 <-> 0xfff000   / 0xff80000
 * 2     1/32           32 <-> 0x7fffe0    8192 <-> 0x1ffe000  / 0x1ff00000
 * 3     1/16           64 <-> 0xffffc0   16384 <-> 0x3ffc000  / 0x3fe00000
 * 4     1/8           128 <-> 0x1ffff80  32768 <-> 0x7ff8000  / 0x7fc00000
 * 5     1/4           256 <-> 0x3ffff00  65536 <-> 0xfff0000  / 0xff800000
 * 6     1/2           512 <-> 0x7fffe00 131072 <-> 0x1ffe0000 / 0x1ff000000
 * 7     1            1024 <-> 0xffffc00 262144 <-> 0x3ffc0000 / 0x3fe000000
 * (max value is based on 18-bit refresh setting and 12-bit burst setting)
 *
 * packet-shapinp mode (for trident mmu)
 * gran  refresh        refresh           bucket-size-granularity
 *       packet/refresh packet/second     packet
 *                      min <-> max          base <-> max
 *                                                    (linear) / (ITU mode)
 * 0     1/256000         1 <-> 0x3ffff     0.256 <-> 0x418    / 0x4168
 * 1     1/128000         2 <-> 0x7fffe     0.512 <-> 0x830    / 0x82d0
 * 2     1/64000          4 <-> 0xffffc     1.024 <-> 0x1061   / 0x105a1
 * 3     1/32000          8 <-> 0x1ffff8    2.048 <-> 0x20c2   / 0x20b43
 * 4     1/16000         16 <-> 0x3ffff0    4.096 <-> 0x4185   / 0x41687
 * 5     1/4000          64 <-> 0xffffc0   16.384 <-> 0x10614  / 0x105a1c
 * 6     1/1000         256 <-> 0x3ffff00  65.536 <-> 0x41851  / 0x416872
 * 7     1/250         1024 <-> 0xffffc00 262.144 <-> 0x106147 / 0x105a1ca
 *
 * packet-policing mode (for trident fp)
 * gran  refresh        refresh           bucket-size-granularity
 *       packet/refresh packet/second     packet
 *                      min <-> max          base <-> max
 *                                                    (linear) / (ITU mode)
 * 0     1/128000         1 <-> 0x7ffff     0.512 <-> 0x830    / 0x82d0
 * 1     1/64000          2 <-> 0xffffe     1.024 <-> 0x1061   / 0x105a1
 * 2     1/32000          4 <-> 0x1ffffc    2.048 <-> 0x20c2   / 0x20b43
 * 3     1/16000          8 <-> 0x3ffff8    4.096 <-> 0x4185   / 0x41687
 * 4     1/8000          16 <-> 0x7ffff0    8.192 <-> 0x830a   / 0x82d0e
 * 5     1/4000          32 <-> 0xffffe0   16.384 <-> 0x10614  / 0x105a1c
 * 6     1/2000          64 <-> 0x1ffffc0  32.768 <-> 0x20c28  / 0x20b439
 * 7     1/250          512 <-> 0xffffe00 262.144 <-> 0x106147 / 0x105a1ca
 */
int
_bcm_td_rate_to_bucket_encoding(int unit, uint32 rate_quantum,
                                uint32 burst_quantum, uint32 flags,
                                int refresh_bitsize, int bucket_max_bitsize,
                                uint32 *refresh_rate, uint32 *bucketsize,
                                uint32 *granularity)
{
    int gran, gran_min, gran_max;
    uint32 i;
    uint32 burst, encoding, buckettop, bucket_segment_size;
    uint32 refresh_mask = 0;
    uint32 bucket_mask = 0;
    uint32 refresh_max = 0;
    uint32 bucket_max = 0;
    uint32 rate_unit_size = 0;
    uint32 burst_unit_size = 0;

    if ((refresh_rate == NULL) || (bucketsize == NULL)) {
        return BCM_E_INTERNAL;
    }

    if ((burst_quantum == 0) &&
        (!(flags & _BCM_TD_METER_FLAG_FP_POLICER) && (rate_quantum == 0))) {
        /* Disabled */
        *refresh_rate = 0;
        *bucketsize = 0;
        *granularity = METER_GRANULARITY_DEFAULT;
        return BCM_E_NONE;
    }

    refresh_mask = 0xffffffff >> (32 - refresh_bitsize);
    bucket_mask = 0xffffffff >> (32 - bucket_max_bitsize);
    if (flags & _BCM_TD_METER_FLAG_PACKET_MODE) {
        /* Minimum packet burst is .512 packet.  We'll work in integers. */
        burst = burst_quantum * METER_PACKET_BURST_DIVISOR;
    } else {
        if (burst_quantum > (0xffffffff / 1000)) {
            burst = 0xffffffff;
        } else {
            burst = burst_quantum * 1000;
        }
    }

    gran_min = 0;
    gran_max = METER_GRANULARITY_NUM - 1;

    for (gran = gran_min; gran <= gran_max; gran++) {
        _bcm_td_granularity_params(unit, gran, flags, &rate_unit_size,
                                   &burst_unit_size);
        refresh_max = refresh_mask * rate_unit_size;
        if (flags & _BCM_TD_METER_FLAG_NON_LINEAR) {
            bucket_max = METER_NL_BUCKET_MAC_ENCODE_MAX * burst_unit_size;
        } else {
            bucket_max = (bucket_mask + 1) * burst_unit_size;
        }

        if ((rate_quantum <= refresh_max) && (burst <= bucket_max)) {
            break;
        }
    }

    if (gran > gran_max) {
        /* Saturate! */
        gran = gran_max;
        if (rate_quantum > refresh_max) {
            rate_quantum = refresh_max;
        }
        if (burst > bucket_max) {
            burst = bucket_max;
        }
    }

    *granularity = gran;

    if (rate_quantum > (0xffffffff - (rate_unit_size - 1))) {
        rate_quantum = 0xffffffff - (rate_unit_size - 1);
    }

    *refresh_rate = (rate_quantum + (rate_unit_size - 1)) / rate_unit_size;
    if (*refresh_rate > refresh_mask) {
        *refresh_rate = refresh_mask;
    }

    if (flags & _BCM_TD_METER_FLAG_NON_LINEAR) {
        if (burst <= burst_unit_size) {
            *bucketsize = burst ? 1 : 0; /* 0 kbits means disable */
        } else {
            encoding = bucket_mask;
            buckettop = burst_unit_size;
            for (i = 0; i < METER_NL_BUCKET_RANGE_NUM; i++, buckettop *= 2) {
                bucket_segment_size =
                    buckettop / METER_NL_SEGMENT_GRANULARITY;
                if (burst <= (buckettop * 2 - bucket_segment_size) ||
                    (buckettop == 0)) {
                    if (buckettop == 0) {
                        /* Saturate */
                        buckettop = 0xffffffff;
                    }
                    encoding =
                        (burst - buckettop + (bucket_segment_size - 1)) /
                        bucket_segment_size;
                    encoding |= i << METER_NL_BUCKET_POWER_SHIFT;
                    break;
                }
            }
            *bucketsize = (i < METER_NL_BUCKET_RANGE_NUM) ? encoding :
                bucket_mask;
        }
    } else {
        encoding = (burst + (burst_unit_size - 1)) / burst_unit_size;
        if (encoding > bucket_mask) {
            encoding = bucket_mask;
        }
        *bucketsize = encoding;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_td_bucket_encoding_to_rate
 * Description:
 *      Routine that converts leaky bucketsize
 *      register value into its associated burst size.
 * Parameters:
 *      refresh_rate - refresh rate field
 *      bucket_max - bucket max field
 *      granularity - granularity field
 *      flags - paramters indicating capabilities and configuration of meter
 *      rate_quantum - (OUT) Configured rate of ongoing traffic
 *      burst_quantum - (OUT) Configured size of maximum burst traffic
 * Returns:
 *	Max size burst traffic in kbits.
 * Notes:
 *      None.
 */

uint32
_bcm_td_bucket_encoding_to_rate(int unit, uint32 refresh_rate,
                                uint32 bucket_max, uint32 granularity,
                                uint32 flags, uint32 *rate_quantum,
                                uint32 *burst_quantum)
{
    int gran;
    uint32 power, segment, bucketsize;
    uint32 rate_unit_size, burst_unit_size;

    if ((rate_quantum == NULL) || (burst_quantum == NULL)) {
        return BCM_E_INTERNAL;
    }

    gran = granularity;
    _bcm_td_granularity_params(unit, gran, flags, &rate_unit_size,
                               &burst_unit_size);

    *rate_quantum = refresh_rate * rate_unit_size;

    if (flags & _BCM_TD_METER_FLAG_NON_LINEAR) {
        if (bucket_max == 0) { /* 0 means disabled */
            bucketsize = 0;
        } else {
            segment = bucket_max & METER_NL_BUCKET_SEGMENT_MASK;
            power = (bucket_max >> METER_NL_BUCKET_POWER_SHIFT) &
                METER_NL_BUCKET_POWER_MASK;

            /* Calculate raw bits */
            bucketsize = (1 << power) *
                (burst_unit_size / METER_NL_SEGMENT_GRANULARITY) *
                (METER_NL_SEGMENT_GRANULARITY + segment);
        }
    } else {
        bucketsize = bucket_max * burst_unit_size;
    }

    if (flags & _BCM_TD_METER_FLAG_PACKET_MODE) {
        *burst_quantum = bucketsize / METER_PACKET_BURST_DIVISOR;
    } else {
        *burst_quantum = bucketsize / 1000;
    }

    return BCM_E_NONE;
}

uint32 _bcm_td_default_burst_size(int unit, int port, uint32 kbits_sec)
{
    soc_info_t *si = &SOC_INFO(unit);
    int proposed1, proposed2, proposed, max_port_speed;
    int mtu = 9216, numq;

    max_port_speed = (IS_CPU_PORT(unit,port))? 1000 : si->port_speed_max[port];
    max_port_speed *= 1000;

    if (max_port_speed <= 0) {
        return 0;
    }

    numq = IS_HG_PORT(unit, port) ? 10 : 8;
    if (max_port_speed > kbits_sec) {
        proposed1 = (mtu * numq)/(max_port_speed/kbits_sec);
    } else {
        proposed1 = (mtu * numq)/((max_port_speed + kbits_sec - 1)/kbits_sec);
    }

    proposed2 = kbits_sec/(8 * 256);
    
    proposed = (proposed1 > proposed2) ? proposed1 : proposed2;
    proposed = (proposed * 8)/1000;
    return (proposed > 0) ? proposed : 1;
}

#else /* BCM_TRIDENT_SUPPORT */
int _td_meter_not_empty;
#endif  /* BCM_TRIDENT_SUPPORT */
