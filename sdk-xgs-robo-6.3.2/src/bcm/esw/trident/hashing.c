/*
 * $Id: hashing.c 1.15 Broadcom SDK $
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
 * File:        hashing.c
 * Purpose:     Hash calcualtions for trunk and ECMP packets.
 */

#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/hash.h>
#include <bcm/switch.h>
#include <bcm/debug.h>
#include <bcm/error.h>

#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/switch.h>

typedef struct bcm_rtag7_base_hash_s {
    uint32 rtag7_hash16_value_a_0;      /* hash a0 result */
    uint32 rtag7_hash16_value_a_1;      /* hash a1 result */
    uint32 rtag7_hash16_value_b_0;      /* hash b0 result */
    uint32 rtag7_hash16_value_b_1;      /* hash b1 result */
    uint32 rtag7_macro_flow_id;         /* hash micro flow result */
    uint32 rtag7_port_lbn;     	        /* port LBN value from the port table*/
    uint32 rtag7_lbid_hash;             /* LBID hash calculation*/
    bcm_port_t dev_src_port;            /* Local source port */
    bcm_port_t src_port;                /* System source port */
    bcm_module_t src_modid;             /* System source modid */
    uint8 is_nonuc;                     /* Non-unicast */
    uint8 hash_a_valid;                 /* Hash A result use valid input */
    uint8 hash_b_valid;                 /* Hash B result use valid input */
    uint8 lbid_hash_valid;              /* LBID Hash value is valid */
} bcm_rtag7_base_hash_t;

#define   ETHERTYPE_IPV6 0x86dd /* ipv6 ethertype */
#define   ETHERTYPE_IPV4 0x0800 /* ipv4 ethertype */
#define   ETHERTYPE_MIN  0x0600 /* minimum ethertype for hashing */

#define   IP_PROT_TCP 0x6  /* TCP protocol number */
#define   IP_PROT_UDP 0x11 /* TCP protocol number */

#define RTAG7_L2_ONLY         0x0
#define RTAG7_UNKNOWN_HIGIG   0x1
#define RTAG7_MPLS            0x2
#define RTAG7_MIM             0x3
#define RTAG7_IPV4            0x4
#define RTAG7_IPV6            0x5
#define RTAG7_FCOE            0x6
#define RTAG7_TRILL           0x7

STATIC uint16
_bcm_hash_crc16(uint32 hash_words[7], int n)
{
    uint8 data[30];
    int i;
    uint16 crc;

    sal_memset(data, 0, 30);
    
    for (i = 0; i < n; i++) {
        data[i] = (hash_words[i/4] >> ((i % 4) * 8)) & 0xff;
    }

    crc = soc_draco_crc16(data, n);
    return crc;
}

/* CRC16 CCITT: Polynomial x^16 + x^12 + x^5 + 1 */

static int	        _shr_crc16_ccitt_table_inited;
static uint16           _shr_crc16_ccitt_table[256];

STATIC uint16
_shr_crc16_ccitt(int crc, unsigned char *data, int len)
{
    int			i;
    int		        j;
    uint32	        accum;
    uint32              poly = 0x8408;

    if (!_shr_crc16_ccitt_table_inited) {
        for (i = 0; i < 256; i++) {
            accum = i;

            for (j = 0; j < 8; j++) {
                accum = (accum & 1) ? (accum >> 1 ^ poly) : (accum >> 1);
            }
            _shr_crc16_ccitt_table[i] = accum;
        }
        _shr_crc16_ccitt_table_inited = 1;
    }

    for (i = 0; i < len; i++) {
	crc = (crc >> 8) ^
            _shr_crc16_ccitt_table[((data[i] & 0x000000FF) << 0) ^
                                   (crc & 0x000000FF)];
    }

    return crc;
}

STATIC uint16
_bcm_hash_crc16_ccitt(uint32 hash_words[7], int n)
{
    uint8 data[30];
    int i;
    uint16 crc;

    sal_memset(data, 0, 30);
    
    for (i = 0; i < n; i++) {
        data[i] = (hash_words[i/4] >> ((i % 4) * 8)) & 0xff;
    }

    crc = _shr_crc16_ccitt(0, data, n);
    crc = _shr_bit_rev16(crc);
    return crc;
}

static int    _shr_crc32_cast_table_inited;
static uint32 _shr_crc32_cast_table[256];

/*
 * The following function implements polynomial:
 *  x^32 + x^28 + x^27 + x^26 + x^25 + x^23 + x^22 + x^20 + 
 *  x^19 + x^18 + x^14 + x^13 + x^11 + x^10 + x^9 + x^8 + x^6 + 1
 */
static uint32
_shr_crc32_castagnoli(unsigned int crc, unsigned char *data, int len)
{
    int			i;

    if (!_shr_crc32_cast_table_inited) {
	int		j;
	unsigned int		accum;

	for (i = 0; i < 256; i++) {
	    accum = i;

	    for (j = 0; j < 8; j++) {
		if (accum & 1) {
		    accum = accum >> 1 ^ 0x82F63B78UL;
		} else {
		    accum = accum >> 1;
		}
	    }

	    _shr_crc32_cast_table[i] = _shr_swap32(accum);
	}

	_shr_crc32_cast_table_inited = 1;
    }

    for (i = 0; i < len; i++) {
	crc = crc << 8 ^ _shr_crc32_cast_table[crc >> 24 ^ data[i]];
    }

    return crc;
}

STATIC uint32
_bcm_hash_crc32_castagnoli(uint32 hash_words[7], int n)
{
    uint8 data[30];
    int i;
    uint32 crc;

    sal_memset(data, 0, 30);
    
    for (i = 0; i < n; i++) {
        data[i] = (hash_words[i/4] >> ((i % 4) * 8)) & 0xff;
    }

    crc = _shr_crc32_castagnoli(0, data, n);
    crc = _shr_bit_rev_by_byte_word32(crc);
    return crc;
}


STATIC int
main__do_rtag7_hashing(int unit,
                       bcm_switch_pkt_info_t *pkt_info,
                       bcm_rtag7_base_hash_t *hash_res)
{
    /* regular var declaration */
    uint32		hash_src_port;
    uint32		hash_src_modid;
    uint32		rtag7_a_sel;
    bcm_ip_t	        sip_a;               /* Source IPv4 address. */
    bcm_ip_t	        dip_a;               /* Destination IPv4 address. */
    uint32		rtag7_b_sel;
    bcm_ip_t	        sip_b;               /* Source IPv4 address. */
    bcm_ip_t	        dip_b;               /* Destination IPv4 address. */
    uint32		hash_field_bmap_a;   /* block A chosen bitmap*/
    uint32		hash_field_bmap_b;   /* block B chosen bitmap*/
	
    uint32 hash_word[7];
    uint32 hash[13];
    int    elem, elem_bit;
    uint32 hash_element_valid_bits;
    /* int    ip6_addrs = FALSE, ip4_addrs = FALSE; */
    int    sip_valid = FALSE, dip_valid = FALSE;
    uint32 macro_flow_id;
	
    uint32 hash_crc16_bisync;
    uint32 hash_crc16_ccitt;
    uint32 hash_crc32;
    uint32 hash_xor16;
    uint32 hash_xor8;
    uint32 hash_xor4;
    uint32 hash_xor2;
    uint32 hash_xor1;
    uint32 xor32;

    /* register var declaration */
    uint8 rtag7_disable_hash_ipv4_a;
    uint8 rtag7_disable_hash_ipv6_a; 
   
    uint8 rtag7_disable_hash_ipv4_b;
    uint8 rtag7_disable_hash_ipv6_b;
    uint8 rtag7_ipv6_addr_use_lsb_a;
    uint8 rtag7_ipv6_addr_use_lsb_b;	
    uint8 rtag7_pre_processing_enable_a;
    uint8 rtag7_pre_processing_enable_b;	
    uint8 rtag7_en_flow_label_ipv6_a;
    uint8 rtag7_en_flow_label_ipv6_b;	
    uint8 rtag7_en_bin_12_overlay_a;
    uint8 rtag7_en_bin_12_overlay_b;	
    uint32 rtag7_hash_seed_a;
    uint32 rtag7_hash_seed_b;	
    uint32 rtag7_hash_a0_function_select;
    uint32 rtag7_hash_a1_function_select;	
    uint32 rtag7_hash_b0_function_select;
    uint32 rtag7_hash_b1_function_select;	
    uint32 rtag7_macro_flow_hash_function_select;
    uint8 rtag7_macro_flow_hash_byte_sel;
	
    uint64 rtag7_hash_control;
    uint32 rtag7_hash_control_2, rtag7_hash_control_3;
    uint32 rtag7_hash_seed_a_reg, rtag7_hash_seed_b_reg;
    soc_reg_t sc_reg;
    soc_field_t sc_field;
    uint32 sc_val;

    SOC_IF_ERROR_RETURN
        (READ_RTAG7_HASH_CONTROLr(unit, &rtag7_hash_control));
    SOC_IF_ERROR_RETURN
        (READ_RTAG7_HASH_CONTROL_2r(unit, &rtag7_hash_control_2));

    SOC_IF_ERROR_RETURN
        (READ_RTAG7_HASH_CONTROL_3r(unit, &rtag7_hash_control_3));
    SOC_IF_ERROR_RETURN
        (READ_RTAG7_HASH_SEED_Ar(unit, &rtag7_hash_seed_a_reg));
    SOC_IF_ERROR_RETURN
        (READ_RTAG7_HASH_SEED_Br(unit, &rtag7_hash_seed_b_reg));

    /* RTAG7 hash registers */
    rtag7_disable_hash_ipv4_a =
        soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, rtag7_hash_control,
                          DISABLE_HASH_IPV4_Af);
    rtag7_disable_hash_ipv6_a =
        soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, rtag7_hash_control,
                          DISABLE_HASH_IPV6_Af);
    rtag7_disable_hash_ipv4_b =
        soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, rtag7_hash_control,
                          DISABLE_HASH_IPV4_Bf);
    rtag7_disable_hash_ipv6_b =
        soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, rtag7_hash_control,
                          DISABLE_HASH_IPV6_Bf);
    rtag7_ipv6_addr_use_lsb_a =
        soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, rtag7_hash_control,
                          IPV6_COLLAPSED_ADDR_SELECT_Af);
    rtag7_ipv6_addr_use_lsb_b =
        soc_reg64_field32_get(unit, RTAG7_HASH_CONTROLr, rtag7_hash_control,
                          IPV6_COLLAPSED_ADDR_SELECT_Bf);
	
    rtag7_pre_processing_enable_a =
        soc_reg_field_get(unit, RTAG7_HASH_CONTROL_3r, rtag7_hash_control_3,
                          HASH_PRE_PROCESSING_ENABLE_Af);
    rtag7_pre_processing_enable_b =
        soc_reg_field_get(unit, RTAG7_HASH_CONTROL_3r, rtag7_hash_control_3,
                          HASH_PRE_PROCESSING_ENABLE_Bf);
    rtag7_hash_a0_function_select =
        soc_reg_field_get(unit, RTAG7_HASH_CONTROL_3r, rtag7_hash_control_3,
                          HASH_A0_FUNCTION_SELECTf);
    rtag7_hash_a1_function_select =
        soc_reg_field_get(unit, RTAG7_HASH_CONTROL_3r, rtag7_hash_control_3,
                          HASH_A1_FUNCTION_SELECTf);
    rtag7_hash_b0_function_select =
        soc_reg_field_get(unit, RTAG7_HASH_CONTROL_3r, rtag7_hash_control_3,
                          HASH_B0_FUNCTION_SELECTf);
    rtag7_hash_b1_function_select =
        soc_reg_field_get(unit, RTAG7_HASH_CONTROL_3r, rtag7_hash_control_3,
                          HASH_B1_FUNCTION_SELECTf);
	
    if (SOC_REG_FIELD_VALID(unit, RTAG7_HASH_CONTROL_2r,
                            ENABLE_FLOW_LABEL_IPV6_Af)) {
        rtag7_en_flow_label_ipv6_a =
            soc_reg_field_get(unit, RTAG7_HASH_CONTROL_2r,
                              rtag7_hash_control_2,
                              ENABLE_FLOW_LABEL_IPV6_Af);
    } else {
        rtag7_en_flow_label_ipv6_a = 0; /* A0 bincomp value */
    }
    if (SOC_REG_FIELD_VALID(unit, RTAG7_HASH_CONTROL_2r,
                            ENABLE_FLOW_LABEL_IPV6_Bf)) {
        rtag7_en_flow_label_ipv6_b =
            soc_reg_field_get(unit, RTAG7_HASH_CONTROL_2r,
                              rtag7_hash_control_2,
                              ENABLE_FLOW_LABEL_IPV6_Bf);
    } else {
        rtag7_en_flow_label_ipv6_b = 0; /* A0 bincomp value */
    }
    rtag7_en_bin_12_overlay_a =
        soc_reg_field_get(unit, RTAG7_HASH_CONTROL_2r, rtag7_hash_control_2,
                          ENABLE_BIN_12_OVERLAY_Af);
    rtag7_en_bin_12_overlay_b =
        soc_reg_field_get(unit, RTAG7_HASH_CONTROL_2r, rtag7_hash_control_2,
                          ENABLE_BIN_12_OVERLAY_Bf);
    if (SOC_REG_FIELD_VALID(unit, RTAG7_HASH_CONTROL_2r,
                            MACRO_FLOW_HASH_FUNC_SELf)) {
        rtag7_macro_flow_hash_function_select =
            soc_reg_field_get(unit, RTAG7_HASH_CONTROL_2r,
                              rtag7_hash_control_2,
                              MACRO_FLOW_HASH_FUNC_SELf);
    } else {
        rtag7_macro_flow_hash_function_select = 0; /* A0 bincomp value */
    }
    if (SOC_REG_FIELD_VALID(unit, RTAG7_HASH_CONTROL_2r,
                            MACRO_FLOW_HASH_BYTE_SELf)) {
        rtag7_macro_flow_hash_byte_sel =
            soc_reg_field_get(unit, RTAG7_HASH_CONTROL_2r,
                              rtag7_hash_control_2,
                              MACRO_FLOW_HASH_BYTE_SELf);
    } else {
        rtag7_macro_flow_hash_byte_sel = 0; /* A0 bincomp value */
    }

    rtag7_hash_seed_a =
        soc_reg_field_get(unit, RTAG7_HASH_SEED_Ar, rtag7_hash_seed_a_reg,
                          HASH_SEED_Af);
    rtag7_hash_seed_b =
        soc_reg_field_get(unit, RTAG7_HASH_SEED_Br, rtag7_hash_seed_b_reg,
                          HASH_SEED_Bf);

    /* RTAG7 tables */
    /* the first step is to choose the paket type and to fold the ipv6
     * addresses in case of IPv6 packet*/	

    /* option A ip folding for rtag 7 */
    if ((_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, ETHERTYPE) &&
         (pkt_info->ethertype == ETHERTYPE_IPV6)) &&
               (rtag7_disable_hash_ipv6_a == 0)) {

      /* ip6_addrs = TRUE; */
        rtag7_a_sel = RTAG7_IPV6;
        
        /* This section will fold the Ipv6 address to single 32 bit address
         * by using of of the methose LSB or xor */
        if (rtag7_ipv6_addr_use_lsb_a) {
            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_IPV6)) {
                sip_a = (pkt_info->sip6[12] << 24) |
                    (pkt_info->sip6[13] << 16) |
                    (pkt_info->sip6[14] << 8) |
                    (pkt_info->sip6[15]);
                sip_valid = TRUE;
            } else {
                sip_a = 0;
            }
            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_IPV6)) {
                dip_a = (pkt_info->dip6[12] << 24) |
                    (pkt_info->dip6[13] << 16) |
                    (pkt_info->dip6[14] << 8) |
                    (pkt_info->dip6[15]);
                dip_valid = TRUE;
            } else {
                dip_a = 0;
            }
        } else {   
            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_IPV6)) {
                sip_a =
                    ((pkt_info->sip6[12] << 24) |
                     (pkt_info->sip6[13] << 16) |
                     (pkt_info->sip6[14] << 8) |
                     (pkt_info->sip6[15])) ^
                    ((pkt_info->sip6[8] << 24) |
                     (pkt_info->sip6[9] << 16) |
                     (pkt_info->sip6[10] << 8) |
                     (pkt_info->sip6[11])) ^
                    ((pkt_info->sip6[4] << 24) |
                     (pkt_info->sip6[5] << 16) |
                     (pkt_info->sip6[6] << 8) |
                     (pkt_info->sip6[7])) ^
                    ((pkt_info->sip6[0] << 24) |
                     (pkt_info->sip6[1] << 16) |
                     (pkt_info->sip6[2] << 8) |
                     (pkt_info->sip6[3]));
                sip_valid = TRUE;
            } else {
                sip_a = 0;
            }

            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_IPV6)) {
                dip_a =
                    ((pkt_info->dip6[12] << 24) |
                     (pkt_info->dip6[13] << 16) |
                     (pkt_info->dip6[14] << 8) |
                     (pkt_info->dip6[15])) ^
                    ((pkt_info->dip6[8] << 24) |
                     (pkt_info->dip6[9] << 16) |
                     (pkt_info->dip6[10] << 8) |
                     (pkt_info->dip6[11])) ^
                    ((pkt_info->dip6[4] << 24) |
                     (pkt_info->dip6[5] << 16) |
                     (pkt_info->dip6[6] << 8) |
                     (pkt_info->dip6[7])) ^
                    ((pkt_info->dip6[0] << 24) |
                     (pkt_info->dip6[1] << 16) |
                     (pkt_info->dip6[2] << 8) |
                     (pkt_info->dip6[3]));
                dip_valid = TRUE;
            } else {
                dip_a = 0;
            }
        }
    } else if ((_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, ETHERTYPE) &&
                (pkt_info->ethertype == ETHERTYPE_IPV4)) && 
               (rtag7_disable_hash_ipv4_a == 0)) {

      /*        ip4_addrs = TRUE; */
        rtag7_a_sel = RTAG7_IPV4;

        /* only one IPv4 hdr */
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_IP)) {
            sip_a = pkt_info->sip;
            sip_valid = TRUE;
        } else {
            sip_a = 0;
        }
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_IP)) {
            dip_a = pkt_info->dip;
            dip_valid = TRUE;
        } else {
            dip_a = 0;
        }
    } else {
        rtag7_a_sel = RTAG7_L2_ONLY;
        sip_a = dip_a = 0;
    }

    /* End of option A ip folding for rtag 7 */

    /* option B ip folding for rtag 7 */
    if ((_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, ETHERTYPE) &&
         (pkt_info->ethertype == ETHERTYPE_IPV6)) &&
               (rtag7_disable_hash_ipv6_b == 0)) {

      /*         ip6_addrs = TRUE; */
        rtag7_b_sel = RTAG7_IPV6;
        
        /* This section will fold the Ipv6 address to single 32 bit address
         * by using of of the methose LSB or xor */
        if (rtag7_ipv6_addr_use_lsb_b) {
            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_IPV6)) {
                sip_b = (pkt_info->sip6[12] << 24) |
                    (pkt_info->sip6[13] << 16) |
                    (pkt_info->sip6[14] << 8) |
                    (pkt_info->sip6[15]);
                sip_valid = TRUE;
            } else {
                sip_b = 0;
            }
            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_IPV6)) {
                dip_b = (pkt_info->dip6[12] << 24) |
                    (pkt_info->dip6[13] << 16) |
                    (pkt_info->dip6[14] << 8) |
                    (pkt_info->dip6[15]);
                dip_valid = TRUE;
            } else {
                dip_b = 0;
            }
        } else {   
            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_IPV6)) {
                sip_b =
                    ((pkt_info->sip6[12] << 24) |
                     (pkt_info->sip6[13] << 16) |
                     (pkt_info->sip6[14] << 8) |
                     (pkt_info->sip6[15])) ^
                    ((pkt_info->sip6[8] << 24) |
                     (pkt_info->sip6[9] << 16) |
                     (pkt_info->sip6[10] << 8) |
                     (pkt_info->sip6[11])) ^
                    ((pkt_info->sip6[4] << 24) |
                     (pkt_info->sip6[5] << 16) |
                     (pkt_info->sip6[6] << 8) |
                     (pkt_info->sip6[7])) ^
                    ((pkt_info->sip6[0] << 24) |
                     (pkt_info->sip6[1] << 16) |
                     (pkt_info->sip6[2] << 8) |
                     (pkt_info->sip6[3]));
                sip_valid = TRUE;
            } else {
                sip_b = 0;
            }

            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_IPV6)) {
                dip_b =
                    ((pkt_info->dip6[12] << 24) |
                     (pkt_info->dip6[13] << 16) |
                     (pkt_info->dip6[14] << 8) |
                     (pkt_info->dip6[15])) ^
                    ((pkt_info->dip6[8] << 24) |
                     (pkt_info->dip6[9] << 16) |
                     (pkt_info->dip6[10] << 8) |
                     (pkt_info->dip6[11])) ^
                    ((pkt_info->dip6[4] << 24) |
                     (pkt_info->dip6[5] << 16) |
                     (pkt_info->dip6[6] << 8) |
                     (pkt_info->dip6[7])) ^
                    ((pkt_info->dip6[0] << 24) |
                     (pkt_info->dip6[1] << 16) |
                     (pkt_info->dip6[2] << 8) |
                     (pkt_info->dip6[3]));
                dip_valid = TRUE;
            } else {
                dip_b = 0;
            }
        }      
    } else if ((_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, ETHERTYPE) &&
                (pkt_info->ethertype == ETHERTYPE_IPV4)) && 
               (rtag7_disable_hash_ipv4_b == 0)) {
			   
      /*         ip4_addrs = TRUE; */
        rtag7_b_sel = RTAG7_IPV4;

        /* only one IPv4 hdr */
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_IP)) {
            sip_b = pkt_info->sip;
            sip_valid = TRUE;
        } else {
            sip_b = 0;
        }
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_IP)) {
            dip_b = pkt_info->dip;
            dip_valid = TRUE;
        } else {
            dip_b = 0;
        }
    } else {	
        rtag7_b_sel = RTAG7_L2_ONLY;
        sip_b = dip_b = 0;
    }
    /* End of option B ip folding for rtag 7 */



    /*
     * =============
     * RTAG7 HASHING 
     * =============
     */

    /* The inputs to the RTAG7 hash function were reduced in the begining
     * of this function.  
     * This logic takes the hash inputs and computes both A and B RTAG7
     * hash values.
     */

    /* Change source modid/port to PORT32 space for hashing */
    hash_src_port  = hash_res->src_port;
    hash_src_modid = hash_res->src_modid;

    /* Only if the packet was sourced by this chip, make sure to use
     * PORT32 space modid and port if dual modid is enabled
     * Need to do this because of front panel ports.
     *
     * Already handled by API gport resolve */

    /* initialize the variables */   
    sal_memset(hash,0x0,(sizeof(uint32))*13);
    sal_memset(hash_word,0x0,(sizeof(uint32))*7);
	
    hash_crc16_bisync = 0;
    hash_crc16_ccitt = 0;
    hash_crc32      = 0;
    hash_xor16      = 0;
    hash_xor8       = 0;
    hash_xor4       = 0;
    hash_xor2       = 0;
    hash_xor1       = 0;
    xor32           = 0;

    hash[12] = 0;	
    hash[3] = hash_src_port;
    hash[2] = hash_src_modid & 0xff;
    hash[1] = 0;
    hash[0] = 0;

    hash_element_valid_bits = 0x1fff; /* Init to valid fields above */

    /* option A hash calculation for rtag 7 */
    
    if ((rtag7_a_sel == RTAG7_IPV6) || (rtag7_a_sel == RTAG7_IPV4)) {

        if (sip_valid) {
            hash[11] = (sip_a >> 16) & 0xffff;
            hash[10] =  sip_a & 0xffff;
        } else {
            hash_element_valid_bits &= ~0xc00;
        }
        if (dip_valid) {
            hash[9]  = (dip_a >> 16) & 0xffff;
            hash[8]  =  dip_a & 0xffff;
        } else {
            hash_element_valid_bits &= ~0x300;
        }

        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, VLAN)) {
            hash[7]  = pkt_info->vid & 0xfff;
        } else {
            hash_element_valid_bits &= ~0x80;
        }
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_L4_PORT)) {
            hash[6]  = pkt_info->src_l4_port;
        } else {
            hash_element_valid_bits &= ~0x40;
        }
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_L4_PORT)) {
            hash[5]  = pkt_info->dst_l4_port;
        } else {
            hash_element_valid_bits &= ~0x20;
        }
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, PROTOCOL)) {
            hash[4]  = pkt_info->protocol;
        } else {
            hash_element_valid_bits &= ~0x10;
        }

        if ((rtag7_a_sel == RTAG7_IPV6) && rtag7_en_flow_label_ipv6_a) {
            BCM_VERB(("Unit %d - Hash calculation: The system is set to use ipv6 flow label and the code can't get this info\n", unit));
        }

        if (rtag7_a_sel == RTAG7_IPV4) {  /* Use IPv4 BITMAPs */
            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, PROTOCOL) &&
                ((pkt_info->protocol == IP_PROT_TCP) ||
                (pkt_info->protocol == IP_PROT_UDP))) { 
                if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_L4_PORT) &&
                    _BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_L4_PORT) &&
                    (pkt_info->src_l4_port == pkt_info->dst_l4_port)) {
                    sc_reg = RTAG7_IPV4_TCP_UDP_HASH_FIELD_BMAP_1r;
                    sc_field = IPV4_TCP_UDP_SRC_EQ_DST_FIELD_BITMAP_Af;
                    BCM_VERB(("Unit %d - Hash calculation: Bitmap is block A IPv4 TCP=UDP\n", unit));
                } else {
                    sc_reg = RTAG7_IPV4_TCP_UDP_HASH_FIELD_BMAP_2r;
                    sc_field = IPV4_TCP_UDP_FIELD_BITMAP_Af;
                    BCM_VERB(("Unit %d - Hash calculation: Bitmap is block A IPv4 L4 tcp/udp\n", unit));
                }
            } else {
                sc_reg = RTAG7_HASH_FIELD_BMAP_1r;
                sc_field = IPV4_FIELD_BITMAP_Af;
                BCM_VERB(("Unit %d - Hash calculation: Bitmap is block A IPv4 \n", unit));
            }

        } else { /* Use IPv6 BITMAPs */
            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, PROTOCOL) &&
                ((pkt_info->protocol == IP_PROT_TCP) ||
                (pkt_info->protocol == IP_PROT_UDP))) { 
                if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_L4_PORT) &&
                    _BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_L4_PORT) &&
                    (pkt_info->src_l4_port == pkt_info->dst_l4_port)) {
                    sc_reg = RTAG7_IPV6_TCP_UDP_HASH_FIELD_BMAP_1r;
                    sc_field = IPV6_TCP_UDP_SRC_EQ_DST_FIELD_BITMAP_Af;
                    BCM_VERB(("Unit %d - Hash calculation: Bitmap is block A IPv6 TCP=UDP\n", unit));
                } else {
                    sc_reg = RTAG7_IPV6_TCP_UDP_HASH_FIELD_BMAP_2r;
                    sc_field = IPV6_TCP_UDP_FIELD_BITMAP_Af;
                    BCM_VERB(("Unit %d - Hash calculation: Bitmap is block A IPv6 L4 tcp/udp\n", unit));
                }
            } else {
                sc_reg = RTAG7_HASH_FIELD_BMAP_2r;
                sc_field = IPV6_FIELD_BITMAP_Af;
                BCM_VERB(("Unit %d - Hash calculation: Bitmap is block A IPv6 \n", unit));
            }
        }
        
    } else { /* Catch-all for RTAG7_L2_ONLY */

        /* L2 only hash key */
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_MAC)) {
            hash[11] = (pkt_info->src_mac[0] << 8) | pkt_info->src_mac[1];
            hash[10] = (pkt_info->src_mac[2] << 8) | pkt_info->src_mac[3];
            hash[9] = (pkt_info->src_mac[4] << 8) | pkt_info->src_mac[5];
        } else {
            hash_element_valid_bits &= ~0xe00;
        }
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_MAC)) {
            hash[8] = (pkt_info->dst_mac[0] << 8) | pkt_info->dst_mac[1];
            hash[7] = (pkt_info->dst_mac[2] << 8) | pkt_info->dst_mac[3];
            hash[6] = (pkt_info->dst_mac[4] << 8) | pkt_info->dst_mac[5];
        } else {
            hash_element_valid_bits &= ~0x1c0;
        }

        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, ETHERTYPE)) {
            hash[5] = (pkt_info->ethertype >= ETHERTYPE_MIN) ?
                pkt_info->ethertype : 0;
        } else {
            hash_element_valid_bits &= ~0x20;
        }

        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, VLAN)) {
            hash[4] = pkt_info->vid & 0xfff;
        } else {
            hash_element_valid_bits &= ~0x10;
        }

        sc_reg = RTAG7_HASH_FIELD_BMAP_3r;
        sc_field = L2_FIELD_BITMAP_Af;
        BCM_VERB(("Unit %d - Hash calculation: Bitmap is block A L2\n", unit));
    }

    BCM_IF_ERROR_RETURN
        (soc_reg32_get(unit, sc_reg, REG_PORT_ANY, 0, &sc_val));
    hash_field_bmap_a = soc_reg_field_get(unit, sc_reg, sc_val, sc_field);

    /* Pre-Processing */

    if (rtag7_pre_processing_enable_a) {

        hash[12] = hash[12] ^ (hash[12] >> 3) ^ (hash[12] << 2);
        hash[11] = hash[11] ^ (hash[11] >> 3) ^ (hash[11] << 2);
        hash[10] = hash[10] ^ (hash[10] >> 3) ^ (hash[10] << 2);
        hash[9] = hash[9] ^ (hash[9] >> 3) ^ (hash[9] << 2);
        hash[8] = hash[8] ^ (hash[8] >> 3) ^ (hash[8] << 2);
        hash[7] = hash[7] ^ (hash[7] >> 3) ^ (hash[7] << 2);
        hash[6] = hash[6] ^ (hash[6] >> 3) ^ (hash[6] << 2);
        hash[5] = hash[5] ^ (hash[5] >> 3) ^ (hash[5] << 2);
        hash[4] = hash[4] ^ (hash[4] >> 3) ^ (hash[4] << 2);
        hash[3] = hash[3] ^ (hash[3] >> 3) ^ (hash[3] << 2);
        hash[2] = hash[2] ^ (hash[2] >> 3) ^ (hash[2] << 2);
        hash[1] = hash[1] ^ (hash[1] >> 3) ^ (hash[1] << 2);
        hash[0] = hash[0] ^ (hash[0] >> 3) ^ (hash[0] << 2);
    
    }

    if (((hash_field_bmap_a >> 12) & 0x1) == 0) {
        hash[12] = 0;
    }    

    if (rtag7_en_bin_12_overlay_a) {
        hash_word[6] = (rtag7_hash_seed_a & 0xffff0000) | (hash[12] & 0xffff);
    } else {
        hash_word[6] = rtag7_hash_seed_a;
    }    

    hash_res->hash_a_valid = TRUE;
    for (elem = 11; elem >= 0; elem--) {
        elem_bit = 1 << elem;
        if (0 != (hash_field_bmap_a & elem_bit)) {
            if (0 != (hash_element_valid_bits & elem_bit)) {
                hash_word[elem/2] |=
                    ((hash[elem] & 0xffff) << (16 * (elem % 2)));
            } else {
                hash_res->hash_a_valid = FALSE;
                break;
            }
        }
    }

    /*Calculation*/
    xor32 = hash_word[6] ^ hash_word[5] ^ hash_word[4] ^ hash_word[3] ^
        hash_word[2] ^ hash_word[1] ^ hash_word[0];

    hash_xor16 = (xor32 & 0xffff) ^ ((xor32 >> 16) & 0xffff);  
    hash_xor8 = (hash_xor16 & 0xff) ^ ((hash_xor16 >> 8) & 0xff);
    hash_xor4 = (hash_xor8 & 0xf) ^ ((hash_xor8 >> 4) & 0xf);
    hash_xor2 = (hash_xor4 & 0x3) ^ ((hash_xor4 >> 2) & 0x3);
    hash_xor1 = (hash_xor2 & 0x1) ^ ((hash_xor2 >> 1) & 0x1);

    hash_crc16_bisync = _bcm_hash_crc16(hash_word, 28);
    hash_crc16_ccitt = _bcm_hash_crc16_ccitt(hash_word, 28);
    hash_crc32 = _bcm_hash_crc32_castagnoli(hash_word, 28);

    switch (rtag7_hash_a0_function_select) {
    case 0: /* reserved */
        hash_res->rtag7_hash16_value_a_0 = 0;
        break;
    case 1: /* reserved */
        hash_res->rtag7_hash16_value_a_0 = 0;
        break;
    case 2: /* reserved */
        hash_res->rtag7_hash16_value_a_0 = 0;
        break;
    case 3: /* CRC16 BISYNC */
        hash_res->rtag7_hash16_value_a_0 = hash_crc16_bisync;
        break;
    case 4: /* CRC16_XOR1 */ 
        hash_res->rtag7_hash16_value_a_0 = hash_xor1 & 0x1;
        hash_res->rtag7_hash16_value_a_0 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 5: /* CRC16_XOR2 */
        hash_res->rtag7_hash16_value_a_0 = hash_xor2 & 0x3;
        hash_res->rtag7_hash16_value_a_0 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 6: /* CRC16_XOR4 */ 
        hash_res->rtag7_hash16_value_a_0 = hash_xor4 & 0xf;
        hash_res->rtag7_hash16_value_a_0 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 7: /* CRC16_XOR8 */
        hash_res->rtag7_hash16_value_a_0 = hash_xor8 & 0xff;
        hash_res->rtag7_hash16_value_a_0 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 8: /* XOR16 */
        hash_res->rtag7_hash16_value_a_0 = hash_xor16;
        break;
    case 9: /* CRC16_CCITT */
        hash_res->rtag7_hash16_value_a_0 = hash_crc16_ccitt;
        break;
    case 10: /* CRC32_LO */
        hash_res->rtag7_hash16_value_a_0 = hash_crc32 & 0xffff;
        break;
    case 11: /* CRC32_HI */
        hash_res->rtag7_hash16_value_a_0 = (hash_crc32 >> 16) & 0xffff;
        break;
    default: /* reserved */
        hash_res->rtag7_hash16_value_a_0 = 0;
        break;
    }

    switch (rtag7_hash_a1_function_select) {
    case 0: /* reserved */
        hash_res->rtag7_hash16_value_a_1 = 0;
        break;
    case 1: /* reserved */
        hash_res->rtag7_hash16_value_a_1 = 0;
        break;
    case 2: /* reserved */
        hash_res->rtag7_hash16_value_a_1 = 0;
        break;
    case 3: /* CRC16 BISYNC */
        hash_res->rtag7_hash16_value_a_1 = hash_crc16_bisync;
        break;
    case 4: /* CRC16_XOR1 */ 
        hash_res->rtag7_hash16_value_a_1 = hash_xor1 & 0x1;
        hash_res->rtag7_hash16_value_a_1 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 5: /* CRC16_XOR2 */
        hash_res->rtag7_hash16_value_a_1 = hash_xor2 & 0x3;
        hash_res->rtag7_hash16_value_a_1 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 6: /* CRC16_XOR4 */ 
        hash_res->rtag7_hash16_value_a_1 = hash_xor4 & 0xf;
        hash_res->rtag7_hash16_value_a_1 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 7: /* CRC16_XOR8 */
        hash_res->rtag7_hash16_value_a_1 = hash_xor8 & 0xff;
        hash_res->rtag7_hash16_value_a_1 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 8: /* XOR16 */
        hash_res->rtag7_hash16_value_a_1 = hash_xor16;
        break;
    case 9: /* CRC16_CCITT */
        hash_res->rtag7_hash16_value_a_1 = hash_crc16_ccitt;
        break;
    case 10: /* CRC32_LO */
        hash_res->rtag7_hash16_value_a_1 = hash_crc32 & 0xffff;
        break;
    case 11: /* CRC32_HI */
        hash_res->rtag7_hash16_value_a_1 = (hash_crc32 >> 16) & 0xffff;
        break;
    default: /* reserved */
        hash_res->rtag7_hash16_value_a_1 = 0;
        break;
    }

    /* End of option A hash calculation for rtag 7 */

    /* Generation of Macro Flow Based HASH Select */
    /* This is used for Macro flow based Hash function selection for all consumers of RTAG7 HASH */
    /* Macro flow based hash function selection improves distribution among members */
    /* Refer Each application (ECMP, LAG, HG-TRUNK) on how this is being used */
    switch (rtag7_macro_flow_hash_function_select) {
    case 0: /* reserved */
        macro_flow_id = 0;
        break;
    case 1: /* reserved */
        macro_flow_id = 0;
        break;
    case 2: /* reserved */
        macro_flow_id = 0;
        break;
    case 3: /* CRC16 BISYNC */
        macro_flow_id = hash_crc16_bisync;
        break;
    case 4: /* CRC16_XOR1 */ 
        macro_flow_id = hash_xor1 & 0x1;
        macro_flow_id |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 5: /* CRC16_XOR2 */
        macro_flow_id = hash_xor2 & 0x3;
        macro_flow_id |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 6: /* CRC16_XOR4 */ 
        macro_flow_id = hash_xor4 & 0xf;
        macro_flow_id |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 7: /* CRC16_XOR8 */
        macro_flow_id = hash_xor8 & 0xff;
        macro_flow_id |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 8: /* XOR16 */
        macro_flow_id = hash_xor16;
        break;
    case 9: /* CRC16_CCITT */
        macro_flow_id = hash_crc16_ccitt;
        break;
    case 10: /* CRC32_LO */
        macro_flow_id = hash_crc32 & 0xffff;
        break;
    case 11: /* CRC32_HI */
        macro_flow_id = (hash_crc32 >> 16) & 0xffff;
        break;
    default: /* reserved */
        macro_flow_id = 0;
        break;
    }
    hash_res->rtag7_macro_flow_id = (rtag7_macro_flow_hash_byte_sel) ?  
        ((macro_flow_id >> 8) & 0xff) : (macro_flow_id & 0xff);

    /* option B hash calculation for rtag 7 */

    /* initialize the variables */   
	
    sal_memset(hash,0x0,(sizeof(uint32))*13);
    sal_memset(hash_word,0x0,(sizeof(uint32))*7);
	
    hash_crc16_bisync = 0;
    hash_crc16_ccitt = 0;
    hash_crc32      = 0;
    hash_xor16      = 0;
    hash_xor8       = 0;
    hash_xor4       = 0;
    hash_xor2       = 0;
    hash_xor1       = 0;
    xor32           = 0;
 
    hash[12] = 0;	
    hash[3] = hash_src_port;
    hash[2] = hash_src_modid & 0xff;
    hash[1] = 0;
    hash[0] = 0; 

    hash_element_valid_bits = 0x1fff; /* Init to valid fields above */

    /* option B hash calculation for rtag 7 */
    
    if ((rtag7_b_sel == RTAG7_IPV6) || (rtag7_b_sel == RTAG7_IPV4)) {
        if (sip_valid) {
            hash[11] = (sip_b >> 16) & 0xffff;
            hash[10] =  sip_b & 0xffff;
        } else {
            hash_element_valid_bits &= ~0xc00;
        }
        if (dip_valid) {
            hash[9]  = (dip_b >> 16) & 0xffff;
            hash[8]  =  dip_b & 0xffff;
        } else {
            hash_element_valid_bits &= ~0x300;
        }

        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, VLAN)) {
            hash[7]  = pkt_info->vid & 0xfff;
        } else {
            hash_element_valid_bits &= ~0x80;
        }
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_L4_PORT)) {
            hash[6]  = pkt_info->src_l4_port;
        } else {
            hash_element_valid_bits &= ~0x40;
        }
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_L4_PORT)) {
            hash[5]  = pkt_info->dst_l4_port;
        } else {
            hash_element_valid_bits &= ~0x20;
        }
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, PROTOCOL)) {
            hash[4]  = pkt_info->protocol;
        } else {
            hash_element_valid_bits &= ~0x10;
        }

        if ((rtag7_b_sel == RTAG7_IPV6) && rtag7_en_flow_label_ipv6_b) {
            BCM_VERB(("Unit %d - Hash calculation: The system is set to use ipv6 flow label and the code can't get this info\n", unit));
        }

        if (rtag7_b_sel == RTAG7_IPV4) {  /* Use IPv4 BITMAPs */
            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, PROTOCOL) &&
                ((pkt_info->protocol == IP_PROT_TCP) ||
                (pkt_info->protocol == IP_PROT_UDP))) { 
                if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_L4_PORT) &&
                    _BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_L4_PORT) &&
                    (pkt_info->src_l4_port == pkt_info->dst_l4_port)) {
                    sc_reg = RTAG7_IPV4_TCP_UDP_HASH_FIELD_BMAP_1r;
                    sc_field = IPV4_TCP_UDP_SRC_EQ_DST_FIELD_BITMAP_Bf;
                    BCM_VERB(("Unit %d - Hash calculation: Bitmap is block B IPv4 TCP=UDP\n", unit));
                } else {
                    sc_reg = RTAG7_IPV4_TCP_UDP_HASH_FIELD_BMAP_2r;
                    sc_field = IPV4_TCP_UDP_FIELD_BITMAP_Bf;
                    BCM_VERB(("Unit %d - Hash calculation: Bitmap is block B IPv4 L4 tcp/udp\n", unit));
                }
            } else {
                sc_reg = RTAG7_HASH_FIELD_BMAP_1r;
                sc_field = IPV4_FIELD_BITMAP_Bf;
                BCM_VERB(("Unit %d - Hash calculation: Bitmap is block B IPv4\n", unit));
            }

        } else { /* Use IPv6 BITMAPs */

            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, PROTOCOL) &&
                ((pkt_info->protocol == IP_PROT_TCP) ||
                (pkt_info->protocol == IP_PROT_UDP))) { 
                if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_L4_PORT) &&
                    _BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_L4_PORT) &&
                    (pkt_info->src_l4_port == pkt_info->dst_l4_port)) {
                    sc_reg = RTAG7_IPV6_TCP_UDP_HASH_FIELD_BMAP_1r;
                    sc_field = IPV6_TCP_UDP_SRC_EQ_DST_FIELD_BITMAP_Bf;
                    BCM_VERB(("Unit %d - Hash calculation: Bitmap is block B IPv6 TCP=UDP\n", unit));
                } else {
                    sc_reg = RTAG7_IPV6_TCP_UDP_HASH_FIELD_BMAP_2r;
                    sc_field = IPV6_TCP_UDP_FIELD_BITMAP_Bf;
                    BCM_VERB(("Unit %d - Hash calculation: Bitmap is block B IPv6 TCP=UDP\n", unit));
                }
            } else {
                sc_reg = RTAG7_HASH_FIELD_BMAP_2r;
                sc_field = IPV6_FIELD_BITMAP_Bf;
                BCM_VERB(("Unit %d - Hash calculation: Bitmap is block B IPv6\n", unit));
            }
        }
    } else { /* Catch-all for RTAG7_L2_ONLY */

        /* L2 only hash key */
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_MAC)) {
            hash[11] = (pkt_info->src_mac[0] << 8) | pkt_info->src_mac[1];
            hash[10] = (pkt_info->src_mac[2] << 8) | pkt_info->src_mac[3];
            hash[9] = (pkt_info->src_mac[4] << 8) | pkt_info->src_mac[5];
        } else {
            hash_element_valid_bits &= ~0xe00;
        }
        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_MAC)) {
            hash[8] = (pkt_info->dst_mac[0] << 8) | pkt_info->dst_mac[1];
            hash[7] = (pkt_info->dst_mac[2] << 8) | pkt_info->dst_mac[3];
            hash[6] = (pkt_info->dst_mac[4] << 8) | pkt_info->dst_mac[5];
        } else {
            hash_element_valid_bits &= ~0x1c0;
        }

        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, ETHERTYPE)) {
            hash[5] = (pkt_info->ethertype >= ETHERTYPE_MIN) ?
                pkt_info->ethertype : 0;
        } else {
            hash_element_valid_bits &= ~0x20;
        }

        if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, VLAN)) {
            hash[4] = pkt_info->vid & 0xfff;
        } else {
            hash_element_valid_bits &= ~0x10;
        }

        sc_reg = RTAG7_HASH_FIELD_BMAP_3r;
        sc_field = L2_FIELD_BITMAP_Bf;
        BCM_VERB(("Unit %d - Hash calculation: Bitmap is block B L2\n", unit));
    }

    BCM_IF_ERROR_RETURN
        (soc_reg32_get(unit, sc_reg, REG_PORT_ANY, 0, &sc_val));
    hash_field_bmap_b = soc_reg_field_get(unit, sc_reg, sc_val, sc_field);

    /* Pre-Processing */

    if (rtag7_pre_processing_enable_b) {

        hash[12] = hash[12] ^ (hash[12] >> 3) ^ (hash[12] << 2);
        hash[11] = hash[11] ^ (hash[11] >> 3) ^ (hash[11] << 2);
        hash[10] = hash[10] ^ (hash[10] >> 3) ^ (hash[10] << 2);
        hash[9] = hash[9] ^ (hash[9] >> 3) ^ (hash[9] << 2);
        hash[8] = hash[8] ^ (hash[8] >> 3) ^ (hash[8] << 2);
        hash[7] = hash[7] ^ (hash[7] >> 3) ^ (hash[7] << 2);
        hash[6] = hash[6] ^ (hash[6] >> 3) ^ (hash[6] << 2);
        hash[5] = hash[5] ^ (hash[5] >> 3) ^ (hash[5] << 2);
        hash[4] = hash[4] ^ (hash[4] >> 3) ^ (hash[4] << 2);
        hash[3] = hash[3] ^ (hash[3] >> 3) ^ (hash[3] << 2);
        hash[2] = hash[2] ^ (hash[2] >> 3) ^ (hash[2] << 2);
        hash[1] = hash[1] ^ (hash[1] >> 3) ^ (hash[1] << 2);
        hash[0] = hash[0] ^ (hash[0] >> 3) ^ (hash[0] << 2);
    
    }

    if (((hash_field_bmap_b >> 12) & 0x1) == 0) {
        hash[12] = 0;
    }    

    if (rtag7_en_bin_12_overlay_b) {
        hash_word[6] = (rtag7_hash_seed_b & 0xffff0000) | (hash[12] & 0xffff);
    } else {
        hash_word[6] = rtag7_hash_seed_b;
    }    

    hash_res->hash_b_valid = TRUE;
    for (elem = 11; elem >= 0; elem--) {
        elem_bit = 1 << elem;
        if (0 != (hash_field_bmap_b & elem_bit)) {
            if (0 != (hash_element_valid_bits & elem_bit)) {
                hash_word[elem/2] |=
                    ((hash[elem] & 0xffff) << (16 * (elem % 2)));
            } else {
                hash_res->hash_b_valid = FALSE;
                break;
            }
        }
    }

    xor32 = hash_word[6] ^ hash_word[5] ^ hash_word[4] ^ hash_word[3] ^ hash_word[2] ^ hash_word[1] ^ hash_word[0];
    hash_xor16 = (xor32 & 0xffff) ^ ((xor32 >> 16) & 0xffff);  
    hash_xor8 = (hash_xor16 & 0xff) ^ ((hash_xor16 >> 8) & 0xff);
    hash_xor4 = (hash_xor8 & 0xf) ^ ((hash_xor8 >> 4) & 0xf);
    hash_xor2 = (hash_xor4 & 0x3) ^ ((hash_xor4 >> 2) & 0x3);
    hash_xor1 = (hash_xor2 & 0x1) ^ ((hash_xor2 >> 1) & 0x1);

    hash_crc16_bisync = _bcm_hash_crc16(hash_word, 28);
    hash_crc16_ccitt = _bcm_hash_crc16_ccitt(hash_word, 28);
    hash_crc32 = _bcm_hash_crc32_castagnoli(hash_word, 28);

    switch (rtag7_hash_b0_function_select) {
    case 0: /* reserved */
        hash_res->rtag7_hash16_value_b_0 = 0;
        break;
    case 1: /* reserved */
        hash_res->rtag7_hash16_value_b_0 = 0;
        break;
    case 2: /* reserved */
        hash_res->rtag7_hash16_value_b_0 = 0;
        break;
    case 3: /* CRC16 BISYNC */
        hash_res->rtag7_hash16_value_b_0 = hash_crc16_bisync;
        break;
    case 4: /* CRC16_XOR1 */ 
        hash_res->rtag7_hash16_value_b_0 = hash_xor1 & 0x1;
        hash_res->rtag7_hash16_value_b_0 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 5: /* CRC16_XOR2 */
        hash_res->rtag7_hash16_value_b_0 = hash_xor2 & 0x3;
        hash_res->rtag7_hash16_value_b_0 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 6: /* CRC16_XOR4 */ 
        hash_res->rtag7_hash16_value_b_0 = hash_xor4 & 0xf;
        hash_res->rtag7_hash16_value_b_0 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 7: /* CRC16_XOR8 */
        hash_res->rtag7_hash16_value_b_0 = hash_xor8 & 0xff;
        hash_res->rtag7_hash16_value_b_0 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 8: /* XOR16 */
        hash_res->rtag7_hash16_value_b_0 = hash_xor16;
        break;
    case 9: /* CRC16_CCITT */
        hash_res->rtag7_hash16_value_b_0 = hash_crc16_ccitt;
        break;
    case 10: /* CRC32_LO */
        hash_res->rtag7_hash16_value_b_0 = hash_crc32 & 0xffff;
        break;
    case 11: /* CRC32_HI */
        hash_res->rtag7_hash16_value_b_0 = (hash_crc32 >> 16) & 0xffff;
        break;
    default: /* reserved */
        hash_res->rtag7_hash16_value_b_0 = 0;
        break;
    }

    switch (rtag7_hash_b1_function_select) {
    case 0: /* reserved */
        hash_res->rtag7_hash16_value_b_1 = 0;
        break;
    case 1: /* reserved */
        hash_res->rtag7_hash16_value_b_1 = 0;
        break;
    case 2: /* reserved */
        hash_res->rtag7_hash16_value_b_1 = 0;
        break;
    case 3: /* CRC16 BISYNC */
        hash_res->rtag7_hash16_value_b_1 = hash_crc16_bisync;
        break;
    case 4: /* CRC16_XOR1 */ 
        hash_res->rtag7_hash16_value_b_1 = hash_xor1 & 0x1;
        hash_res->rtag7_hash16_value_b_1 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 5: /* CRC16_XOR2 */
        hash_res->rtag7_hash16_value_b_1 = hash_xor2 & 0x3;
        hash_res->rtag7_hash16_value_b_1 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 6: /* CRC16_XOR4 */ 
        hash_res->rtag7_hash16_value_b_1 = hash_xor4 & 0xf;
        hash_res->rtag7_hash16_value_b_1 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 7: /* CRC16_XOR8 */
        hash_res->rtag7_hash16_value_b_1 = hash_xor8 & 0xff;
        hash_res->rtag7_hash16_value_b_1 |= (((hash_crc16_bisync >> 8) & 0xff) << 8);
        break;
    case 8: /* XOR16 */
        hash_res->rtag7_hash16_value_b_1 = hash_xor16;
        break;
    case 9: /* CRC16_CCITT */
        hash_res->rtag7_hash16_value_b_1 = hash_crc16_ccitt;
        break;
    case 10: /* CRC32_LO */
        hash_res->rtag7_hash16_value_b_1 = hash_crc32 & 0xffff;
        break;
    case 11: /* CRC32_HI */
        hash_res->rtag7_hash16_value_b_1 = (hash_crc32 >> 16) & 0xffff;
        break;
    default: /* reserved */
        hash_res->rtag7_hash16_value_b_1 = 0;
        break;
    }

    /* End of option B hash calculation for rtag 7 */
    return BCM_E_NONE;
}

/*----------------------------------------------------------------*/



STATIC int
select_hash_subfield(int hash_sub_sel, uint32 *selected_subfield,
                     bcm_rtag7_base_hash_t *hash_Base)
{
    int rv = BCM_E_NONE;
    *selected_subfield = 0;

    switch (hash_sub_sel) {
        case 0: /* select pkt hash A */
            *selected_subfield = hash_Base->rtag7_hash16_value_a_0;
            if (!hash_Base->hash_a_valid) {
                /* Missing paramter in input */
                rv = BCM_E_PARAM;
            }
            break;

        case 1: /* select pkt hash B */
            *selected_subfield = hash_Base->rtag7_hash16_value_b_0;
            if (!hash_Base->hash_b_valid) {
                /* Missing paramter in input */
                rv = BCM_E_PARAM;
            }
            break;

        case 2: /* select port LBN */
            *selected_subfield = hash_Base->rtag7_port_lbn;
            break;

        case 3: /* select destination port id or hash A */
            
            *selected_subfield = hash_Base->rtag7_hash16_value_a_0;
            if (!hash_Base->hash_a_valid) {
                /* Missing paramter in input */
                rv = BCM_E_PARAM;
            }
            break;

        case 4: /* select LBID from module header or from SW1 stage */ 
            *selected_subfield = hash_Base->rtag7_lbid_hash; 
            if (!hash_Base->lbid_hash_valid) {
                /* LBID setting isn't RTAG7 */
                rv = BCM_E_CONFIG;
            }
            break;

        case 5: /* select LBID from SW1 stage */
            *selected_subfield = hash_Base->rtag7_lbid_hash;
            if (!hash_Base->lbid_hash_valid) {
                /* LBID setting isn't RTAG7 */
                rv = BCM_E_CONFIG;
            }
            break;

        case 6:
            *selected_subfield = hash_Base->rtag7_hash16_value_a_1;
            if (!hash_Base->hash_a_valid) {
                /* Missing paramter in input */
                rv = BCM_E_PARAM;
            }
            break;

        case 7: 
            *selected_subfield = hash_Base->rtag7_hash16_value_b_1;
            if (!hash_Base->hash_b_valid) {
                /* Missing paramter in input */
                rv = BCM_E_PARAM;
            }
            break;
    }

    return rv;
}


STATIC int
main__compute_lbid(int unit, bcm_rtag7_base_hash_t *hash_Base)
{
    /*regular var declaration*/
    uint32 lbid_hash_sub_sel;
    uint32 lbid_hash_offset;
    uint32 lbid_hash_value = 0;

    /*register var declaration*/
    int lbid_rtag;
    uint8 rtag7_hash_cfg_sel_lbid;
    uint32 rtag7_hash_lbid_reg;
    int pc_out, sc_val, lbid_idx, rv = BCM_E_NONE;
	
    /*register read*/
    BCM_IF_ERROR_RETURN
        (bcm_esw_port_control_get(unit, hash_Base->dev_src_port,
                                  bcmPortControlLoadBalancingNumber,
                                  &pc_out)); 
    rtag7_hash_cfg_sel_lbid = pc_out;
    BCM_IF_ERROR_RETURN
        (bcm_esw_switch_control_get(unit, bcmSwitchLoadBalanceHashSelect,
                                    &sc_val));
    lbid_rtag = sc_val;
	
    /* ******************* LBID computation ************************* */

    if (lbid_rtag == 7) {
        lbid_idx = (0 != rtag7_hash_cfg_sel_lbid) ? 1 : 0;
        SOC_IF_ERROR_RETURN
            (READ_RTAG7_HASH_LBIDr(unit, lbid_idx, &rtag7_hash_lbid_reg));
        if (hash_Base->is_nonuc) {
            lbid_hash_sub_sel =
                soc_reg_field_get(unit, RTAG7_HASH_LBIDr,
                                  rtag7_hash_lbid_reg, SUB_SEL_NONUCf);
            lbid_hash_offset =
                soc_reg_field_get(unit, RTAG7_HASH_LBIDr,
                                  rtag7_hash_lbid_reg, OFFSET_NONUCf);
        } else {
            lbid_hash_sub_sel =
                soc_reg_field_get(unit, RTAG7_HASH_LBIDr,
                                  rtag7_hash_lbid_reg, SUB_SEL_UCf);
            lbid_hash_offset =
                soc_reg_field_get(unit, RTAG7_HASH_LBIDr,
                                  rtag7_hash_lbid_reg, OFFSET_UCf);
        }		
        /* Only UC hash selects apply to ECMP */
        switch (lbid_hash_sub_sel) {
		
        case 0: /* use pkt hash A */
            lbid_hash_value = hash_Base->rtag7_hash16_value_a_0;
            if (!hash_Base->hash_a_valid) {
                /* Missing paramter in input */
                rv = BCM_E_PARAM;
            }
            break;
		
        case 1: /* use pkt hash B */
            lbid_hash_value = hash_Base->rtag7_hash16_value_b_0;
            if (!hash_Base->hash_b_valid) {
                /* Missing paramter in input */
                rv = BCM_E_PARAM;
            }
            break;
		
        case 2: /* Use LBN */
            lbid_hash_value = hash_Base->rtag7_port_lbn;
            break;
		
        case 3: /* Use Destination PORTID or hash A */
            lbid_hash_value = hash_Base->rtag7_hash16_value_a_0;
            if (!hash_Base->hash_a_valid) {
                /* Missing paramter in input */
                rv = BCM_E_PARAM;
            }
            break;
		
        case 4: /* Use LBID */
            /* Higig lookup use mh_higig2_lbid */
				
            lbid_hash_value = 0;
            break;
		
        case 5:
            lbid_hash_value = 0;
            break;
		
        case 6:
            lbid_hash_value = hash_Base->rtag7_hash16_value_a_1;
            if (!hash_Base->hash_a_valid) {
                /* Missing paramter in input */
                rv = BCM_E_PARAM;
            }
            break;
		
        case 7:
            lbid_hash_value = hash_Base->rtag7_hash16_value_b_1;
            if (!hash_Base->hash_b_valid) {
                /* Missing paramter in input */
                rv = BCM_E_PARAM;
            }
            break;
        }
		
        /*
         * set up the hash value so that the LSB bits are
         * barrel shifted in case offset > 8.
         */
        lbid_hash_value |= ((lbid_hash_value & 0xffff) << 16);
		
        /* Only UC hash offsets apply to ECMP */
        lbid_hash_value = (lbid_hash_value >> lbid_hash_offset);
		
        hash_Base->rtag7_lbid_hash = (lbid_hash_value & 0xff);
        hash_Base->lbid_hash_valid = TRUE;

    } else { /* LBID rtag is 0-6 */        
        /* this function not support the rtag 0-6 */
		
        BCM_VERB(("Unit %d - Hash calculation: This function doesn't support rtag 0 6 pls change register ING_CONFIG.LBID_RTAG to value 7\n", unit));
        hash_Base->rtag7_lbid_hash = 0;
        hash_Base->lbid_hash_valid = FALSE;
    }
    return rv;
    /* ******************* END of LBID computation ************************* */
}

#ifdef INCLUDE_L3
STATIC int
compute_ecmp_hash(int unit, bcm_rtag7_base_hash_t *hash_Base,
                  uint32 *hash_val)
{
    /*regular var declaration*/
    uint32 hash_sub_sel;
    uint32 hash_offset;
    uint32 hash_subfield;
    uint32 rtag7_ecmp_hash_value;
    uint8 rtag7_ecmp_use_macro_flow_hash;
    uint8 ecmp_hash_16bits;
    uint8 ecmp_hash_use_rtag7;
    uint32 rtag7_hash_ecmp;
    uint32 ing_config_2, hash_control;
    int pc_out, ecmp_idx;
    rtag7_flow_based_hash_entry_t rt7_fbh_entry;

    /*register read*/
    BCM_IF_ERROR_RETURN
        (bcm_esw_port_control_get(unit, hash_Base->dev_src_port,
                                  bcmPortControlECMPHashSet,
                                  &pc_out));
    ecmp_idx = (0 != pc_out) ? 1 : 0;

    SOC_IF_ERROR_RETURN
        (READ_RTAG7_HASH_ECMPr(unit, ecmp_idx, &rtag7_hash_ecmp));
    if (soc_reg_field_valid(unit, RTAG7_HASH_ECMPr, USE_FLOW_HASHf)) {
        rtag7_ecmp_use_macro_flow_hash =
            soc_reg_field_get(unit, RTAG7_HASH_ECMPr,
                              rtag7_hash_ecmp, USE_FLOW_HASHf);
    } else {
        /* A0 bincomp value */
        rtag7_ecmp_use_macro_flow_hash = 0;
    }

    if (SOC_REG_IS_VALID(unit, ING_CONFIG_2r)) {
        SOC_IF_ERROR_RETURN
            (READ_ING_CONFIG_2r(unit, &ing_config_2));
        ecmp_hash_16bits =
            soc_reg_field_get(unit, ING_CONFIG_2r, ing_config_2,
                              ECMP_HASH_16BITSf);
    } else {
        ecmp_hash_16bits = 0;
    }

    SOC_IF_ERROR_RETURN
        (READ_HASH_CONTROLr(unit, &hash_control));
    ecmp_hash_use_rtag7 =
        soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                          ECMP_HASH_USE_RTAG7f);

    /* RTAG7 hash computation */

    if (rtag7_ecmp_use_macro_flow_hash) {
        SOC_IF_ERROR_RETURN
            (soc_mem_read(unit, RTAG7_FLOW_BASED_HASHm, MEM_BLOCK_ANY,
                          (hash_Base->rtag7_macro_flow_id & 0xff),
                          &rt7_fbh_entry));

        /* Generating RTAG7 Macro Flow Based Hash Subsel and Offset for use in RSEL1 Stage */
        /* For future designs this logic can be placed in RSEL1 */

        hash_sub_sel = soc_mem_field32_get(unit, RTAG7_FLOW_BASED_HASHm,
                                           &rt7_fbh_entry, SUB_SEL_ECMPf);
        hash_offset  = soc_mem_field32_get(unit, RTAG7_FLOW_BASED_HASHm,
                                           &rt7_fbh_entry, OFFSET_ECMPf);

    } else if (SOC_REG_IS_VALID(unit, RTAG7_HASH_ECMPr)) {
        hash_sub_sel =
            soc_reg_field_get(unit, RTAG7_HASH_ECMPr,
                              rtag7_hash_ecmp, SUB_SELf);
        hash_offset =
            soc_reg_field_get(unit, RTAG7_HASH_ECMPr,
                              rtag7_hash_ecmp, OFFSETf);
    } else {
        /* A0 bincomp values */
        hash_sub_sel = 0;
        hash_offset  = 0;
    }

    BCM_IF_ERROR_RETURN
        (select_hash_subfield(hash_sub_sel, &hash_subfield, hash_Base));

    if (ecmp_hash_16bits) {
        /* Barrel shift the hash subfield, then take the LSB 16 bits */
        rtag7_ecmp_hash_value = ((hash_subfield >> hash_offset) | (hash_subfield << (16 - hash_offset))) & 0xffff; 
    } else {
        /* Trident-A0/A1 ECMP mode - SDK restricts to 256 members or less */
        /* Barrel shift the hash subfield, then take the LSB 10 bits */
        rtag7_ecmp_hash_value = ((hash_subfield >> hash_offset) | (hash_subfield << (16 - hash_offset))) & 0x3ff; 
    }
	
    /* Select between non-RTAG7 hash value and RTAG7 hash value */

    if (ecmp_hash_use_rtag7 == 0) {
        BCM_VERB(("Unit %d - Hash calculation:  non rtag7 calc not supported\n", unit));
        *hash_val =  0;
    } else {
        *hash_val =  rtag7_ecmp_hash_value;
    }

    return BCM_E_NONE;
    /* ***************** END of ECMP HASH CALCULATIONS.  ************************ */
}
#endif /* INCLUDE_L3 */

STATIC int
compute_rtag7_hash_trunk(int unit , bcm_rtag7_base_hash_t *hash_Base,
                         uint32 *hash_value)
{
    uint32 hash_sub_sel;
    uint32 hash_offset;    
    uint32 hash_subfield;
    uint32 rtag7_hash_trunk;
    uint32 rtag7_hash_trunk_value;
    int offset_shift;
    int pc_out, trunk_idx;

    BCM_IF_ERROR_RETURN
        (bcm_esw_port_control_get(unit, hash_Base->dev_src_port,
                                  bcmPortControlTrunkHashSet,
                                  &pc_out));
    trunk_idx = (0 != pc_out) ? 1 : 0;
    SOC_IF_ERROR_RETURN
        (READ_RTAG7_HASH_TRUNKr(unit, trunk_idx, &rtag7_hash_trunk));

    /* Compute RTAG7 hash for trunking */
    if(hash_Base->is_nonuc) {
        hash_sub_sel =
            soc_reg_field_get(unit, RTAG7_HASH_TRUNKr,
                              rtag7_hash_trunk, SUB_SEL_NONUCf);
        hash_offset =
            soc_reg_field_get(unit, RTAG7_HASH_TRUNKr,
                              rtag7_hash_trunk, OFFSET_NONUCf);
        offset_shift = 0xff;
    } else {
        hash_sub_sel =
            soc_reg_field_get(unit, RTAG7_HASH_TRUNKr,
                              rtag7_hash_trunk, SUB_SEL_UCf);
        hash_offset =
            soc_reg_field_get(unit, RTAG7_HASH_TRUNKr,
                              rtag7_hash_trunk, OFFSET_UCf);
        offset_shift = 0x3ff;
    }        

    BCM_IF_ERROR_RETURN
        (select_hash_subfield(hash_sub_sel, &hash_subfield, hash_Base));

    /* Barrel shift the hash subfield, then take the LSB 10 bits for UC and 8 bits for nonUC */
    rtag7_hash_trunk_value = ((hash_subfield >> hash_offset) | (hash_subfield << (16 - hash_offset))) & offset_shift; 
	
    /* Compute the trunk_member_table index
     *  trunk_index = trunk_hash % (trunk_group_size + 1);
     *   Irsel2_rsel2__trunk_member_table_index =
     *        (trunk_base_ptr + trunk_index) & 0x7ff;
     */

    *hash_value = rtag7_hash_trunk_value;
    return BCM_E_NONE;
}
 
STATIC int
get_hash_trunk(int unit, int tgid, uint32 hash_index, bcm_gport_t *dst_gport)
{
    uint32 trunk_base;
    uint32 trunk_group_size;    
    uint32 rtag;
    uint32 trunk_index;
    uint32 trunk_member_table_index;
    bcm_module_t mod_id;
    bcm_port_t port_id;
    trunk_member_entry_t trunk_member_entry;
    trunk_group_entry_t  tg_entry;
    _bcm_gport_dest_t   dest;
    
    BCM_IF_ERROR_RETURN
        (READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tgid, &tg_entry));

    trunk_base = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, BASE_PTRf);
    trunk_group_size = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry,
                                                    TG_SIZEf);
    rtag = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, RTAGf);
                
    if (rtag != 7){
         BCM_VERB(("Unit %d - Hash calculation: uport only RTAG7 calc no support for rtag %d\n", unit, rtag));
    }

    trunk_index = hash_index % (trunk_group_size + 1);
    trunk_member_table_index = (trunk_base + trunk_index) & 0x7ff;
    BCM_VERB(("\tTrunk HW index 0x%08x\n", trunk_index));
    BCM_VERB(("\tTrunk group size 0x%08x\n", trunk_group_size));


    BCM_IF_ERROR_RETURN
        (READ_TRUNK_MEMBERm(unit, MEM_BLOCK_ANY, trunk_member_table_index,
                            &trunk_member_entry));

    mod_id = soc_TRUNK_MEMBERm_field32_get(unit, &trunk_member_entry,
                                           MODULE_IDf);
    port_id = soc_TRUNK_MEMBERm_field32_get(unit, &trunk_member_entry,
                                            PORT_NUMf);

    BCM_IF_ERROR_RETURN
        (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET, mod_id, port_id,
                                        &(dest.modid), &(dest.port)));
    dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
    BCM_IF_ERROR_RETURN(_bcm_esw_gport_construct(unit, &dest, dst_gport));

    return BCM_E_NONE;  
}

#ifdef INCLUDE_L3
STATIC int
get_hash_ecmp(int unit, int ecmp_group, uint32 hash_index, bcm_if_t *nh_id)
{
    uint8 ecmp_hash_16bits;
    uint32 ecmp_hash_field_upper_bits_count;
                
    ecmp_count_entry_t ecmpc_entry;
    ecmp_entry_t ecmp_entry;
    uint32 ecmp_ptr = 0;
    uint32 ecmp_count = 0;
    uint32 ing_config_2, hash_control;
                
    uint32 ecmp_mask;
    uint32 ecmp_offset;
    uint32 ecmp_index;

    /*register read*/
    if (SOC_REG_IS_VALID(unit, ING_CONFIG_2r)) {
        SOC_IF_ERROR_RETURN
            (READ_ING_CONFIG_2r(unit, &ing_config_2));
        ecmp_hash_16bits =
            soc_reg_field_get(unit, ING_CONFIG_2r, ing_config_2,
                              ECMP_HASH_16BITSf);
    } else {
        ecmp_hash_16bits = 0;
    }

    if (SOC_REG_FIELD_VALID(unit, HASH_CONTROLr,
                            ECMP_HASH_FIELD_UPPER_BITS_COUNTf)) {
        SOC_IF_ERROR_RETURN
            (READ_HASH_CONTROLr(unit, &hash_control));
        ecmp_hash_field_upper_bits_count =
            soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                              ECMP_HASH_FIELD_UPPER_BITS_COUNTf);
    } else {
        ecmp_hash_field_upper_bits_count = 0x6; /* A0 bincomp value */
    }
                
    /* pick up the ecmp count and base_ptr */
    /* Trident+ ECMP Mode where ECMP resolution is done in one step using a 16Mod10 */
    BCM_IF_ERROR_RETURN
        (READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, ecmp_group, &ecmpc_entry));

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     * A ECMP_COUNT value of 0 implies 1 entry 
     * a value of 1 implies 2 entries etc...
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     */
     /* ECMP Mask is expected to be set to 0x3ff for TridentA0/A1 mode */
    if (ecmp_hash_16bits == 0) {
        /* Trident-A0/A1 ECMP mode - SDK restricts to 256 members or less */
        ecmp_mask = 0x3ff;
        ecmp_count =
            soc_L3_ECMP_COUNTm_field32_get(unit, &ecmpc_entry,
                                           COUNT_0f);
        ecmp_ptr =
            soc_L3_ECMP_COUNTm_field32_get(unit, &ecmpc_entry,
                                           BASE_PTR_0f);
   } else {
        ecmp_ptr = soc_L3_ECMP_COUNTm_field32_get(unit, &ecmpc_entry,
                                                  BASE_PTRf);
        ecmp_count = soc_L3_ECMP_COUNTm_field32_get(unit, &ecmpc_entry,
                                                  COUNTf);
        switch (ecmp_hash_field_upper_bits_count) {
        case 0:
            ecmp_mask = 0x3ff;
            break;
        case 1:
            ecmp_mask = 0x7ff;
            break;
        case 2:
            ecmp_mask = 0xfff;
            break;
        case 3:
            ecmp_mask = 0x1fff;
            break;
        case 4:
            ecmp_mask = 0x3fff;
            break;
        case 5:
            ecmp_mask = 0x7fff;
            break;
        case 6:
            ecmp_mask = 0xffff;
            break;
        default:
            ecmp_mask = 0xffff;
            break;
        }
    }

    ecmp_offset = ((hash_index & ecmp_mask) % (ecmp_count + 1)) & 0x3FF;
    BCM_VERB(("\tECMP offset 0x%08x\n", ecmp_offset));
    ecmp_index = (ecmp_ptr + ecmp_offset) & 0xfff;
    BCM_IF_ERROR_RETURN
        (READ_L3_ECMPm(unit, MEM_BLOCK_ANY, ecmp_index, &ecmp_entry));

    *nh_id =
        soc_L3_ECMPm_field32_get(unit, &ecmp_entry,
                                 NEXT_HOP_INDEXf) & 0x3fff;
    BCM_VERB(("\tECMP next hop HW index 0x%08x\n", *nh_id));

    return BCM_E_NONE;  
}
#endif /* INCLUDE_L3 */

int
_bcm_switch_pkt_info_ecmp_hash_get(int unit,
                                   bcm_switch_pkt_info_t *pkt_info,
                                   bcm_gport_t *dst_gport,
                                   bcm_if_t *dst_intf)
{
    bcm_rtag7_base_hash_t hash_res;
    int pc_out;
    bcm_gport_t     port;
    bcm_trunk_t     tid;
    int             id;
    uint32          hash_value;

    if (!_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_GPORT)) {
        BCM_VERB(("Unit %d - Hash calculation: source gport value missing\n", unit));
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, pkt_info->src_gport,
                                &(hash_res.src_modid),
                                &(hash_res.src_port), 
                                &tid, &id));
    if ((-1 != id) || (-1 != tid)) {
        /* Must be single system port */
        return BCM_E_PORT;
    }

    /* Load balancing retrieval */
    BCM_IF_ERROR_RETURN
        (bcm_esw_port_local_get(unit, pkt_info->src_gport, &port));
    hash_res.dev_src_port = port;
    BCM_IF_ERROR_RETURN
        (bcm_esw_port_control_get(unit, port,
                                  bcmPortControlLoadBalancingNumber,
                                  &pc_out)); 
    hash_res.rtag7_port_lbn = pc_out;

    /* check if the packet is UC or not by checking bit 40 in the mac DA */
    hash_res.is_nonuc = BCM_MAC_IS_MCAST(pkt_info->dst_mac);
	
    BCM_IF_ERROR_RETURN
        (main__do_rtag7_hashing(unit, pkt_info, &hash_res));
    BCM_IF_ERROR_RETURN
        (main__compute_lbid(unit, &hash_res));

    BCM_VERB(("Unit %d - Hash status: \n", unit));
    if (hash_res.hash_a_valid) {
        BCM_VERB(("\tRTAG7 A0 0x%08x\n", hash_res.rtag7_hash16_value_a_0));
        BCM_VERB(("\tRTAG7 A1 0x%08x\n", hash_res.rtag7_hash16_value_a_1));
    } else {
        BCM_VERB(("\tRTAG7 A hashes invalid due to missing packet info\n"));
    }
    if (hash_res.hash_b_valid) {
        BCM_VERB(("\tRTAG7 B0 0x%08x\n", hash_res.rtag7_hash16_value_b_0));
        BCM_VERB(("\tRTAG7 B1 0x%08x\n", hash_res.rtag7_hash16_value_b_1));
    } else {
        BCM_VERB(("\tRTAG7 B hashes invalid due to missing packet info\n"));
    }
    BCM_VERB(("\tRTAG7 LBN 0x%08x\n", hash_res.rtag7_port_lbn));
    if (hash_res.lbid_hash_valid){
        BCM_VERB(("\tRTAG7 LBID 0x%08x\n", hash_res.rtag7_lbid_hash));
    } else {
        BCM_VERB(("\tRTAG7 LBID not valid due to non-RTAG7 configuration\n"));
    }

#ifdef INCLUDE_L3
    if (0 != (pkt_info->flags & BCM_SWITCH_PKT_INFO_HASH_MULTIPATH)) {
        bcm_if_t        nh_id;
        BCM_IF_ERROR_RETURN
            (compute_ecmp_hash(unit, &hash_res, &hash_value));
        BCM_VERB(("\tECMP Hash value 0x%08x\n", hash_value));

        BCM_IF_ERROR_RETURN
            (get_hash_ecmp(unit,
                           pkt_info->mpintf - BCM_XGS3_MPATH_EGRESS_IDX_MIN,
                           hash_value, &nh_id));
        /* Convert to egress object format */
        *dst_intf = nh_id + BCM_XGS3_EGRESS_IDX_MIN;
    } else
#endif /* INCLUDE_L3 */
    if (0 != (pkt_info->flags & BCM_SWITCH_PKT_INFO_HASH_TRUNK)) {
        bcm_trunk_t trunk = BCM_TRUNK_INVALID;
        int member_count;

        if (!BCM_GPORT_IS_TRUNK(pkt_info->trunk_gport)) {
            return BCM_E_PORT;
        }
        trunk = BCM_GPORT_TRUNK_GET(pkt_info->trunk_gport);
        BCM_IF_ERROR_RETURN
            (bcm_esw_trunk_get(unit, trunk, NULL, 0, NULL, &member_count));
        if (0 == member_count) {
            /* Return failure for no trunk members */
            return BCM_E_FAIL;
        }

        BCM_IF_ERROR_RETURN
            (compute_rtag7_hash_trunk(unit, &hash_res, &hash_value));
        BCM_VERB(("\tTrunk Hash value 0x%08x\n", hash_value));
        BCM_IF_ERROR_RETURN
            (get_hash_trunk(unit, BCM_GPORT_TRUNK_GET(pkt_info->trunk_gport),
                            hash_value, dst_gport));
    } else {
        return BCM_E_PARAM;
    }
	
    return BCM_E_NONE;
}
